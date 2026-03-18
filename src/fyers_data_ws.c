/**
 * @file fyers_data_ws.c
 * @brief Fyers Data WebSocket implementation for real-time market data
 */

 #include "fyers_data_ws.h"
 #include "fyers_logger.h"
 #include "fyers_http_client.h"
 #include "fyers_config.h"
 #include "fyers_model.h"
 #include <libwebsockets.h>
 #include <stdlib.h>
 #include <string.h>
 #include <stdio.h>
 #include <stdint.h>
 #include <time.h>
 #include <pthread.h>
 #include "fyers_platform.h"
 #include <cjson/cJSON.h>
 #include <openssl/bio.h>
 #include <openssl/evp.h>
 #include <openssl/buffer.h>
 #include <math.h>
 
 // Constants
 #define DATA_WS_URL "wss://socket.fyers.in/hsm/v1-5/prod"
 #define SYMBOL_TOKEN_API "https://api-t1.fyers.in/data/symbol-token"
 #define SOURCE_STRING "CSDK-3.1.8"
 #define SYMBOL_LIMIT 5000
 #define MAX_RECONNECT_ATTEMPTS 50
 #define PING_INTERVAL_SEC 10
 #define CHUNK_SIZE 500
 #define SUBSCRIPTION_CHUNK_SIZE 1500
 
 // Message types
 #define MSG_TYPE_AUTH 1
 #define MSG_TYPE_SUBSCRIBE 4
 #define MSG_TYPE_UNSUBSCRIBE 5
 #define MSG_TYPE_DATA_FEED 6
 #define MSG_TYPE_PAUSE 7
 #define MSG_TYPE_RESUME 8
 #define MSG_TYPE_MODE 12
 
 // Data types
 #define DATA_TYPE_SNAPSHOT 83
 #define DATA_TYPE_FULL_MODE 85
 #define DATA_TYPE_LITE_MODE 76
 
 // Forward declarations
 struct fyers_data_ws;
 struct message_queue;
 
 // Message queue structure
 struct message_queue {
     uint8_t* data;
     size_t size;
     struct message_queue* next;
 };
 
 // Symbol mapping structure
 typedef struct {
     char* hsm_token;
     char* symbol;
 } symbol_map_t;
 
 // Topic mapping structure (Topic ID -> HSM token)
 typedef struct {
     uint16_t topic_id;
     char* hsm_token;
 } topic_map_t;
 
 // Response entry structure (HSM token -> response data)
 typedef struct {
     char* hsm_token;
     cJSON* data;
 } resp_entry_t;
 
 // Channel structure
 typedef struct {
     int channel_num;
     symbol_map_t* symbols;
     size_t symbol_count;
 } channel_info_t;
 
 // Main WebSocket structure
 struct fyers_data_ws {
     // Connection
     struct lws_context* lws_context;
     struct lws* lws_wsi;
     bool connected;
     bool should_reconnect;
     int reconnect_attempts;
     int max_reconnect_attempts;
     int reconnect_delay;
     
     // Authentication
     char* access_token;
     char* hsm_token;
     bool valid_token;
     
     // Configuration
     bool litemode;
     bool write_to_file;
     char* log_path;
     int channel_num;
     bool restart_flag;
     fyers_data_type_t data_type;  // Current data type (SymbolUpdate or DepthUpdate)
     
     // Callbacks
     fyers_data_ws_on_message_t on_message;
     fyers_data_ws_on_error_t on_error;
     fyers_data_ws_on_connect_t on_connect;
     fyers_data_ws_on_close_t on_close;
     
     // Threading
     pthread_t ws_thread;
     pthread_t ping_thread;
     pthread_t message_thread;
     bool ws_thread_started;
     bool ping_thread_started;
     bool message_thread_started;
     pthread_mutex_t ws_mutex;
     pthread_mutex_t message_mutex;
     pthread_cond_t message_cond;
     bool thread_running;
     
     // Message queue
     struct message_queue* message_queue_head;
     struct message_queue* message_queue_tail;
     
     // State
     int ack_count;
     int update_count;
     uint32_t last_message_num;
     
     // Symbol management
     symbol_map_t* symbol_token_map;
     size_t symbol_token_count;
     channel_info_t channels[31]; // Channels 1-30
     
     // Data parsing - Topic ID mappings
     topic_map_t* scrips_sym;  // Topic ID -> HSM token mapping
     topic_map_t* index_sym;
     topic_map_t* dp_sym;
     size_t scrips_sym_count;
     size_t index_sym_count;
     size_t dp_sym_count;
     size_t scrips_sym_capacity;
     size_t index_sym_capacity;
     size_t dp_sym_capacity;
     
     // Response data storage - maps HSM token to response data
     resp_entry_t* resp_map;
     size_t resp_map_count;
     size_t resp_map_capacity;
     
     // Field name arrays (from map.json)
     const char** data_val;  // 22 fields for scrips
     const char** index_val; // 8 fields for index
     const char** depthvalue; // 32 fields for depth
     const char** lite_val;  // 3 fields for lite
     
     // Active channel tracking
     int active_channel;
     int* scrips_per_channel; // Array of symbol counts per channel
     
     // Logger
     fyers_logger_t* logger;
     fyers_http_client_t* http_client;
     
     // Update tracking
     bool update_tick;
     
     // Reconnection thread (spawned on close when restart_flag)
     pthread_t reconnect_thread;
     bool reconnect_thread_active;
     
     // Last subscription for auto resubscribe after reconnect (matching Python: user subscribes in on_connect)
     char** last_subscription_symbols;
     size_t last_subscription_count;
     fyers_data_type_t last_subscription_data_type;
     int last_subscription_channel;
 };
 
 // Base64 decoding utility
 static int base64_decode(const char* input, size_t input_len, unsigned char* output, size_t* output_len) {
     BIO* bio, *b64;
     int decoded_len = 0;
     
     bio = BIO_new_mem_buf(input, (int)input_len);
     b64 = BIO_new(BIO_f_base64());
     bio = BIO_push(b64, bio);
     
     BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
     decoded_len = BIO_read(bio, output, (int)input_len);
     BIO_free_all(bio);
     
     if (decoded_len > 0) {
         *output_len = decoded_len;
         return 0;
     }
     return -1;
 }
 
 // Extract HSM token from JWT
 static int extract_hsm_token(const char* access_token, char** hsm_token_out) {
     if (!access_token || !hsm_token_out) {
         return -1;
     }
     
     // Handle "client_id:token" format
     const char* token = access_token;
     const char* colon = strchr(access_token, ':');
     if (colon) {
         token = colon + 1;
     }
     
     // Split JWT into parts
     char* token_copy = strdup(token);
     if (!token_copy) {
         return -1;
     }
     
     char* saveptr = NULL;
     char* header_part = strtok_r(token_copy, ".", &saveptr);
     char* payload_part = strtok_r(NULL, ".", &saveptr);
     
     if (!payload_part) {
         free(token_copy);
         return -1;
     }
     
     // Decode payload (add padding if needed)
     size_t payload_len = strlen(payload_part);
     size_t padding = (4 - (payload_len % 4)) % 4;
     char* padded_payload = (char*)malloc(payload_len + padding + 1);
     if (!padded_payload) {
         free(token_copy);
         return -1;
     }
     strcpy(padded_payload, payload_part);
     for (size_t i = 0; i < padding; i++) {
         strcat(padded_payload, "=");
     }
     
     // Base64 decode
     unsigned char* decoded = (unsigned char*)malloc(payload_len * 2);
     if (!decoded) {
         free(token_copy);
         free(padded_payload);
         return -1;
     }
     
     size_t decoded_len = 0;
     if (base64_decode(padded_payload, strlen(padded_payload), decoded, &decoded_len) != 0) {
         free(token_copy);
         free(padded_payload);
         free(decoded);
         return -1;
     }
     
     decoded[decoded_len] = '\0';
     
     // Parse JSON to extract hsm_key
     cJSON* json = cJSON_Parse((char*)decoded);
     if (!json) {
         free(token_copy);
         free(padded_payload);
         free(decoded);
         return -1;
     }
     
     // Check expiration
     cJSON* exp_item = cJSON_GetObjectItem(json, "exp");
     if (exp_item && cJSON_IsNumber(exp_item)) {
         time_t now = time(NULL);
         if (exp_item->valueint < now) {
             cJSON_Delete(json);
             free(token_copy);
             free(padded_payload);
             free(decoded);
             return -2; // Token expired
         }
     }
     
     // Extract hsm_key
     cJSON* hsm_key_item = cJSON_GetObjectItem(json, "hsm_key");
     if (!hsm_key_item || !cJSON_IsString(hsm_key_item)) {
         cJSON_Delete(json);
         free(token_copy);
         free(padded_payload);
         free(decoded);
         return -1;
     }
     
     *hsm_token_out = strdup(hsm_key_item->valuestring);
     
     cJSON_Delete(json);
     free(token_copy);
     free(padded_payload);
     free(decoded);
     
     return (*hsm_token_out) ? 0 : -1;
 }
 
 // Add message to queue
 static void add_message_to_queue(fyers_data_ws_t* ws, const uint8_t* data, size_t size) {
     if (!ws || !data || size == 0) {
         return;
     }
     
     struct message_queue* msg = (struct message_queue*)malloc(sizeof(struct message_queue));
     if (!msg) {
         return;
     }
     
     msg->data = (uint8_t*)malloc(size);
     if (!msg->data) {
         free(msg);
         return;
     }
     
     memcpy(msg->data, data, size);
     msg->size = size;
     msg->next = NULL;
     
     pthread_mutex_lock(&ws->message_mutex);
     if (ws->message_queue_tail) {
         ws->message_queue_tail->next = msg;
         ws->message_queue_tail = msg;
     } else {
         ws->message_queue_head = msg;
         ws->message_queue_tail = msg;
     }
     pthread_cond_signal(&ws->message_cond);
     
     // Request WRITEABLE callback if connected
     if (ws->lws_wsi && ws->connected) {
         lws_callback_on_writable(ws->lws_wsi);
     }
     
     pthread_mutex_unlock(&ws->message_mutex);
 }
 
 // Process message queue
 // Message queue thread - just manages the queue, doesn't send
 // Actual sending happens in LWS_CALLBACK_CLIENT_WRITEABLE callback
 static void* message_queue_thread(void* arg) {
     // This thread is kept for compatibility but doesn't do actual sending
     // Messages are sent via LWS_CALLBACK_CLIENT_WRITEABLE callback
     fyers_data_ws_t* ws = (fyers_data_ws_t*)arg;
     
     while (ws->thread_running) {
         pthread_mutex_lock(&ws->message_mutex);
         
         while (ws->thread_running && !ws->message_queue_head) {
             pthread_cond_wait(&ws->message_cond, &ws->message_mutex);
         }
         
         if (!ws->thread_running) {
             pthread_mutex_unlock(&ws->message_mutex);
             break;
         }
         
         // When messages are queued, request a WRITEABLE callback
         if (ws->message_queue_head && ws->lws_wsi && ws->connected) {
             lws_callback_on_writable(ws->lws_wsi);
         }
         
         pthread_mutex_unlock(&ws->message_mutex);
         
         // Small sleep to avoid busy waiting
         usleep(10000); // 10ms
     }
     
     return NULL;
 }
 
 // Ping thread
 static void* ping_thread(void* arg) {
     fyers_data_ws_t* ws = (fyers_data_ws_t*)arg;
     uint8_t ping_msg[] = {0, 1, 11};
     
     while (ws->thread_running) {
         sleep(PING_INTERVAL_SEC);
         
         if (!ws->thread_running) {
             break;
         }
         
         pthread_mutex_lock(&ws->ws_mutex);
         if (ws->lws_wsi && ws->connected) {
             add_message_to_queue(ws, ping_msg, sizeof(ping_msg));
             // Request WRITEABLE callback to send ping
             lws_callback_on_writable(ws->lws_wsi);
         }
         pthread_mutex_unlock(&ws->ws_mutex);
     }
     
     return NULL;
 }
 
 // Create authentication message
 static void create_auth_message(fyers_data_ws_t* ws, uint8_t* buffer, size_t* size) {
     if (!ws || !ws->hsm_token || !buffer || !size) {
         return;
     }
     
     size_t hsm_token_len = strlen(ws->hsm_token);
     size_t source_len = strlen(SOURCE_STRING);
     size_t total_size = 18 + hsm_token_len + source_len;
     
     if (*size < total_size) {
         *size = total_size;
         return;
     }
     
     size_t offset = 0;
     
     // Data length (total_size - 2)
     uint16_t data_len = htons((uint16_t)(total_size - 2));
     memcpy(buffer + offset, &data_len, 2);
     offset += 2;
     
     // Request type (1 = Auth)
     buffer[offset++] = MSG_TYPE_AUTH;
     
     // Field count (4)
     buffer[offset++] = 4;
     
     // Field 1: AuthToken
     buffer[offset++] = 1;
     uint16_t field1_size = htons((uint16_t)hsm_token_len);
     memcpy(buffer + offset, &field1_size, 2);
             offset += 2;
     memcpy(buffer + offset, ws->hsm_token, hsm_token_len);
     offset += hsm_token_len;
     
     // Field 2: Mode ('P' for production)
     buffer[offset++] = 2;
     buffer[offset++] = 0;
     buffer[offset++] = 1;
     buffer[offset++] = 'P';
     
     // Field 3: Flag (1)
     buffer[offset++] = 3;
     buffer[offset++] = 0;
     buffer[offset++] = 1;
     buffer[offset++] = 1;
     
     // Field 4: Source
     buffer[offset++] = 4;
     uint16_t field4_size = htons((uint16_t)source_len);
     memcpy(buffer + offset, &field4_size, 2);
     offset += 2;
     memcpy(buffer + offset, SOURCE_STRING, source_len);
     offset += source_len;
     
     *size = offset;
 }
 
 // Create subscription message
 static void create_subscription_message(fyers_data_ws_t* ws, const char** symbols, size_t symbol_count, uint8_t* buffer, size_t* size) {
     if (!ws || !symbols || symbol_count == 0 || !buffer || !size) {
         return;
     }
     
     // Calculate total size needed
     size_t symbols_data_size = 2; // Symbol count (2 bytes)
     for (size_t i = 0; i < symbol_count; i++) {
         symbols_data_size += 1 + strlen(symbols[i]); // Length byte + symbol string
     }
     
     size_t total_size = 18 + symbols_data_size;
     
     if (*size < total_size) {
         *size = total_size;
         return;
     }
     
     size_t offset = 0;
     
     // Data length
     uint16_t data_len = htons((uint16_t)(total_size - 2));
     memcpy(buffer + offset, &data_len, 2);
             offset += 2;
             
     // Request type (4 = Subscribe)
     buffer[offset++] = MSG_TYPE_SUBSCRIBE;
     
     // Field count (2)
     buffer[offset++] = 2;
     
     // Field 1: Symbols data
     buffer[offset++] = 1;
     uint16_t field1_size = htons((uint16_t)symbols_data_size);
     memcpy(buffer + offset, &field1_size, 2);
             offset += 2;
             
     // Symbol count
     buffer[offset++] = (symbol_count >> 8) & 0xFF;
     buffer[offset++] = symbol_count & 0xFF;
     
     // Symbols
     for (size_t i = 0; i < symbol_count; i++) {
         size_t sym_len = strlen(symbols[i]);
         buffer[offset++] = (uint8_t)sym_len;
         memcpy(buffer + offset, symbols[i], sym_len);
         offset += sym_len;
     }
     
     // Field 2: Channel number
     buffer[offset++] = 2;
     buffer[offset++] = 0;
     buffer[offset++] = 1;
     buffer[offset++] = (uint8_t)ws->channel_num;
     
     *size = offset;
 }
 
 // Create unsubscription message (request type 5, same format as subscribe but type 5)
 static void create_unsubscription_message(fyers_data_ws_t* ws, const char** symbols, size_t symbol_count, uint8_t* buffer, size_t* size) {
     if (!ws || !symbols || symbol_count == 0 || !buffer || !size) {
         return;
     }
 
     // Calculate total size needed (same layout as subscription)
     size_t symbols_data_size = 2; /* symbol count */
     for (size_t i = 0; i < symbol_count; i++) {
         symbols_data_size += 1 + strlen(symbols[i]);
     }
 
     size_t total_size = 18 + symbols_data_size;
 
     if (*size < total_size) {
         *size = total_size;
         return;
     }
 
     size_t offset = 0;
 
     // Data length
     uint16_t data_len = htons((uint16_t)(total_size - 2));
     memcpy(buffer + offset, &data_len, 2);
     offset += 2;
 
     // Request type (5 = Unsubscribe)
     buffer[offset++] = MSG_TYPE_UNSUBSCRIBE;
 
     // Field count (2)
     buffer[offset++] = 2;
 
     // Field 1: Symbols data
     buffer[offset++] = 1;
     uint16_t field1_size = htons((uint16_t)symbols_data_size);
     memcpy(buffer + offset, &field1_size, 2);
     offset += 2;
 
     // Symbol count (big-endian)
     buffer[offset++] = (symbol_count >> 8) & 0xFF;
     buffer[offset++] = symbol_count & 0xFF;
 
     for (size_t i = 0; i < symbol_count; i++) {
         size_t sym_len = strlen(symbols[i]);
         buffer[offset++] = (uint8_t)sym_len;
         memcpy(buffer + offset, symbols[i], sym_len);
         offset += sym_len;
     }
 
     // Field 2: Channel number
     buffer[offset++] = 2;
     buffer[offset++] = 0;
     buffer[offset++] = 1;
     buffer[offset++] = (uint8_t)ws->channel_num;
 
     *size = offset;
 }
 
 // Exchange segment mapping structure (from map.json exch_seg_dict)
 typedef struct {
     const char* code;  // e.g., "1010"
     const char* segment;  // e.g., "nse_cm"
 } exch_seg_entry_t;
 
 // Index mapping structure (from map.json index_dict)
 typedef struct {
     const char* symbol;  // e.g., "NSE:NIFTY50-INDEX"
     const char* token;   // e.g., "Nifty 50"
 } index_entry_t;
 
 // Initialize exchange segment dictionary (from map.json exch_seg_dict)
 static const char* get_exchange_segment(const char* ex_sg) {
     // Map from map.json exch_seg_dict
     static const exch_seg_entry_t exch_seg_dict[] = {
         {"1010", "nse_cm"},
         {"1011", "nse_fo"},
         {"1120", "mcx_fo"},
         {"1210", "bse_cm"},
         {"1012", "cde_fo"},
         {"1211", "bse_fo"},
         {"1212", "bcs_fo"},
         {"1020", "nse_com"},
         {NULL, NULL}
     };
     
     for (int i = 0; exch_seg_dict[i].code; i++) {
         if (strcmp(exch_seg_dict[i].code, ex_sg) == 0) {
             return exch_seg_dict[i].segment;
         }
     }
     return NULL;
 }
 
 // Initialize index dictionary (from map.json index_dict)
 static const char* get_index_token(const char* symbol) {
     // Map from map.json index_dict (subset - full list has ~100 entries)
     static const index_entry_t index_dict[] = {
         {"NSE:NIFTYINDIAMFG-INDEX", "NIFTY INDIA MFG"},
         {"NSE:NIFTY100ESG-INDEX", "NIFTY100 ESG"},
         {"NSE:NIFTYINDDIGITAL-INDEX", "NIFTY IND DIGITAL"},
         {"NSE:NIFTYMICROCAP250-INDEX", "NIFTY MICROCAP250"},
         {"NSE:NIFTYCONSRDURBL-INDEX", "NIFTY CONSR DURBL"},
         {"NSE:NIFTYHEALTHCARE-INDEX", "NIFTY HEALTHCARE"},
         {"NSE:NIFTYOILANDGAS-INDEX", "NIFTY OIL AND GAS"},
         {"NSE:NIFTY100ESGSECLDR-INDEX", "Nifty100ESGSecLdr"},
         {"NSE:NIFTY200MOMENTM30-INDEX", "Nifty200Momentm30"},
         {"NSE:NIFTYALPHALOWVOL-INDEX", "NIFTY AlphaLowVol"},
         {"NSE:NIFTY200QUALTY30-INDEX", "NIFTY200 QUALTY30"},
         {"NSE:NIFTYSMLCAP50-INDEX", "NIFTY SMLCAP 50"},
         {"NSE:MIDCPNIFTY-INDEX", "NIFTY MID SELECT"},
         {"NSE:NIFTYMIDSELECT-INDEX", "NIFTY MID SELECT"},
         {"NSE:NIFTYMIDCAP150-INDEX", "NIFTY MIDCAP 150"},
         {"NSE:NIFTY100 EQL WGT-INDEX", "NIFTY100 EQL Wgt"},
         {"NSE:NIFTY50 EQL WGT-INDEX", "NIFTY50 EQL Wgt"},
         {"NSE:NIFTYGSCOMPSITE-INDEX", "Nifty GS Compsite"},
         {"NSE:NIFTYGS1115YR-INDEX", "Nifty GS 11 15Yr"},
         {"NSE:NIFTYGS48YR-INDEX", "Nifty GS 4 8Yr"},
         {"NSE:NIFTYGS10YRCLN-INDEX", "Nifty GS 10Yr Cln"},
         {"NSE:NIFTYGS813YR-INDEX", "Nifty GS 8 13Yr"},
         {"NSE:NIFTYSMLCAP100-INDEX", "NIFTY SMLCAP 100"},
         {"NSE:NIFTYQUALITY30-INDEX", "NIFTY100 Qualty30"},
         {"NSE:NIFTYPVTBANK-INDEX", "Nifty Pvt Bank"},
         {"NSE:NIFTYPHARMA-INDEX", "Nifty Pharma"},
         {"NSE:NIFTYLARGEMID250-INDEX", "NIFTY LARGEMID250"},
         {"NSE:NIFTYGS15YRPLUS-INDEX", "Nifty GS 15YrPlus"},
         {"NSE:NIFTYPSUBANK-INDEX", "Nifty PSU Bank"},
         {"NSE:NIFTYSMLCAP250-INDEX", "NIFTY SMLCAP 250"},
         {"NSE:NIFTYENERGY-INDEX", "Nifty Energy"},
         {"NSE:NIFTYALPHA50-INDEX", "NIFTY Alpha 50"},
         {"NSE:NIFTYPSE-INDEX", "Nifty PSE"},
         {"NSE:NIFTYFINSRV2550-INDEX", "Nifty FinSrv25 50"},
         {"NSE:FINNIFTY-INDEX", "Nifty Fin Service"},
         {"NSE:NIFTYREALTY-INDEX", "Nifty Realty"},
         {"NSE:NIFTY500-INDEX", "Nifty 500"},
         {"NSE:NIFTY500MULTICAP-INDEX", "NIFTY500 MULTICAP"},
         {"NSE:NIFTYMIDCAP50-INDEX", "Nifty Midcap 50"},
         {"NSE:NIFTYTOTALMKT-INDEX", "NIFTY TOTAL MKT"},
         {"NSE:NIFTY50PR2XLEV-INDEX", "Nifty50 PR 2x Lev"},
         {"NSE:INDIAVIX-INDEX", "India VIX"},
         {"NSE:NIFTYDIVOPPS50-INDEX", "Nifty Div Opps 50"},
         {"NSE:NIFTYMNC-INDEX", "Nifty MNC"},
         {"NSE:NIFTY50VALUE20-INDEX", "Nifty50 Value 20"},
         {"NSE:NIFTY50-INDEX", "Nifty 50"},
         {"NSE:HANGSENG BEES-NAV-INDEX", "HangSeng BeES-NAV"},
         {"NSE:NIFTY100LIQ15-INDEX", "Nifty100 Liq 15"},
         {"NSE:NIFTY50TR2XLEV-INDEX", "Nifty50 TR 2x Lev"},
         {"NSE:NIFTY100-INDEX", "Nifty 100"},
         {"NSE:NIFTY100 LOWVOL30-INDEX", "NIFTY100 LowVol30"},
         {"NSE:NIFTYBANK-INDEX", "Nifty Bank"},
         {"NSE:NIFTYFMCG-INDEX", "Nifty FMCG"},
         {"NSE:NIFTYIT-INDEX", "Nifty IT"},
         {"NSE:NIFTYGS10YR-INDEX", "Nifty GS 10Yr"},
         {"NSE:NIFTYMIDCAP100-INDEX", "NIFTY MIDCAP 100"},
         {"NSE:NIFTYNEXT50-INDEX", "Nifty Next 50"},
         {"NSE:NIFTYNXT50-INDEX", "Nifty Next 50"},
         {"NSE:NIFTYM150QLTY50-INDEX", "NIFTY M150 QLTY50"},
         {"NSE:NIFTYSERVSECTOR-INDEX", "Nifty Serv Sector"},
         {"NSE:NIFTYMIDSML400-INDEX", "NIFTY MIDSML 400"},
         {"NSE:NIFTYAUTO-INDEX", "Nifty Auto"},
         {"NSE:NIFTYMETAL-INDEX", "Nifty Metal"},
         {"NSE:NIFTYINFRA-INDEX", "Nifty Infra"},
         {"NSE:NIFTYMEDIA-INDEX", "Nifty Media"},
         {"NSE:NIFTY50PR1XINV-INDEX", "Nifty50 PR 1x Inv"},
         {"NSE:NIFTY200-INDEX", "Nifty 200"},
         {"NSE:NIFTY50TR1XINV-INDEX", "Nifty50 TR 1x Inv"},
         {"NSE:NIFTYCPSE-INDEX", "Nifty CPSE"},
         {"NSE:NIFTYMIDLIQ15-INDEX", "Nifty Mid Liq 15"},
         {"NSE:NIFTYCOMMODITIES-INDEX", "Nifty Commodities"},
         {"NSE:NIFTYCONSUMPTION-INDEX", "Nifty Consumption"},
         {"NSE:NIFTY50DIVPOINT-INDEX", "Nifty50 Div Point"},
         {"NSE:NIFTYGROWSECT15-INDEX", "Nifty GrowSect 15"},
         {"BSE:100LARGECAPTMC-INDEX", "LCTMCI"},
         {"BSE:DFRG-INDEX", "DFRGRI"},
         {"BSE:QUALITY-INDEX", "BSEQUI"},
         {"BSE:DIVIDENDSTABILITY-INDEX", "BSEDSI"},
         {"BSE:250SMALLCAP-INDEX", "SML250"},
         {"BSE:150MIDCAP-INDEX", "MID150"},
         {"BSE:ESG100-INDEX", "ESG100"},
         {"BSE:SNXT50-INDEX", "SNXT50"},
         {"BSE:SNSX50-INDEX", "SNSX50"},
         {"BSE:UTILS-INDEX", "UTILS"},
         {"BSE:GREENEX-INDEX", "GREENX"},
         {"BSE:SENSEX-INDEX", "SENSEX"},
         {"BSE:REALTY-INDEX", "REALTY"},
         {"BSE:PRIVATEBANKS-INDEX", "BSEPBI"},
         {"BSE:CDGS-INDEX", "CDGS"},
         {"BSE:OILGAS-INDEX", "OILGAS"},
         {"BSE:ENERGY-INDEX", "ENERGY"},
         {"BSE:POWER-INDEX", "POWER"},
         {"BSE:500-INDEX", "BSE500"},
         {"BSE:100-INDEX", "BSE100"},
         {"BSE:PSU-INDEX", "BSEPSU"},
         {"BSE:HC-INDEX", "BSE HC"},
         {"BSE:400MIDSMALLCAP-INDEX", "MSL400"},
         {"BSE:BHRT22-INDEX", "BHRT22"},
         {"BSE:BANKEX-INDEX", "BANKEX"},
         {"BSE:ALLCAP-INDEX", "ALLCAP"},
         {"BSE:INFRA-INDEX", "INFRA"},
         {"BSE:CD-INDEX", "BSE CD"},
         {"BSE:MIDCAP-INDEX", "MIDCAP"},
         {"BSE:AUTO-INDEX", "AUTO"},
         {"BSE:BASMTR-INDEX", "BASMTR"},
         {"BSE:200-INDEX", "BSE200"},
         {"BSE:FIN-INDEX", "FIN"},
         {"BSE:CG-INDEX", "BSE CG"},
         {"BSE:ENHANCEDVALUE-INDEX", "BSEEVI"},
         {"BSE:TECK-INDEX", "TECK"},
         {"BSE:METAL-INDEX", "METAL"},
         {"BSE:CARBONEX-INDEX", "CARBON"},
         {"BSE:MIDSEL-INDEX", "MIDSEL"},
         {"BSE:SME IPO-INDEX", "SMEIPO"},
         {"BSE:MOMENTUM-INDEX", "BSEMOI"},
         {"BSE:TELCOM-INDEX", "TELCOM"},
         {"BSE:CPSE-INDEX", "CPSE"},
         {"BSE:250LARGEMIDCAP-INDEX", "LMI250"},
         {"BSE:SMLCAP-INDEX", "SMLCAP"},
         {"BSE:IT-INDEX", "BSE IT"},
         {"BSE:INDIAMANUFACTURING-INDEX", "MFG"},
         {"BSE:INDSTR-INDEX", "INDSTR"},
         {"BSE:LOWVOLATILITY-INDEX", "BSELVI"},
         {"BSE:LRGCAP-INDEX", "LRGCAP"},
         {"BSE:IPO-INDEX", "BSEIPO"},
         {"BSE:FMC-INDEX", "BSEFMC"},
         {"BSE:SMLSEL-INDEX", "SMLSEL"},
         {NULL, NULL}
     };
     
     for (int i = 0; index_dict[i].symbol; i++) {
         if (strcmp(index_dict[i].symbol, symbol) == 0) {
             return index_dict[i].token;
         }
     }
     return NULL;
 }
 
 // Initialize field name arrays (from map.json structure)
 static void init_field_names(fyers_data_ws_t* ws) {
     // data_val - 22 fields for scrips
     static const char* data_val_array[] = {
         "ltp", "vol_traded_today", "last_traded_time", "exch_feed_time",
         "bid_size", "ask_size", "bid_price", "ask_price", "last_traded_qty",
         "tot_buy_qty", "tot_sell_qty", "avg_trade_price", "OI", "low_price",
         "high_price", "Yhigh", "Ylow", "lower_ckt", "upper_ckt", "open_price",
         "prev_close_price", "type", "symbol"
     };
     
     // index_val - 8 fields for index
     static const char* index_val_array[] = {
         "ltp", "prev_close_price", "exch_feed_time", "high_price",
         "low_price", "open_price", "type", "symbol"
     };
     
     // depthvalue - 32 fields for depth
     static const char* depthvalue_array[] = {
         "bid_price1", "bid_price2", "bid_price3", "bid_price4", "bid_price5",
         "ask_price1", "ask_price2", "ask_price3", "ask_price4", "ask_price5",
         "bid_size1", "bid_size2", "bid_size3", "bid_size4", "bid_size5",
         "ask_size1", "ask_size2", "ask_size3", "ask_size4", "ask_size5",
         "bid_order1", "bid_order2", "bid_order3", "bid_order4", "bid_order5",
         "ask_order1", "ask_order2", "ask_order3", "ask_order4", "ask_order5",
         "type", "symbol"
     };
     
     // lite_val - 3 fields for lite mode
     static const char* lite_val_array[] = {
         "ltp", "symbol", "type"
     };
     
     ws->data_val = (const char**)data_val_array;
     ws->index_val = (const char**)index_val_array;
     ws->depthvalue = (const char**)depthvalue_array;
     ws->lite_val = (const char**)lite_val_array;
 }
 
 // Find or add topic mapping
 static char* get_topic_name(fyers_data_ws_t* ws, uint16_t topic_id, int type) {
     // type: 0=scrips, 1=index, 2=depth
     topic_map_t* map = NULL;
     size_t* count = NULL;
     size_t* capacity = NULL;
     
     if (type == 0) {
         map = ws->scrips_sym;
         count = &ws->scrips_sym_count;
         capacity = &ws->scrips_sym_capacity;
     } else if (type == 1) {
         map = ws->index_sym;
         count = &ws->index_sym_count;
         capacity = &ws->index_sym_capacity;
     } else {
         map = ws->dp_sym;
         count = &ws->dp_sym_count;
         capacity = &ws->dp_sym_capacity;
     }
     
     // Find existing mapping
     for (size_t i = 0; i < *count; i++) {
         if (map[i].topic_id == topic_id) {
             return map[i].hsm_token;
         }
     }
     
     return NULL; // Not found
 }
 
 // Add topic mapping
 static void add_topic_mapping(fyers_data_ws_t* ws, uint16_t topic_id, const char* hsm_token, int type) {
     topic_map_t* map = NULL;
     size_t* count = NULL;
     size_t* capacity = NULL;
     
     if (type == 0) {
         map = ws->scrips_sym;
         count = &ws->scrips_sym_count;
         capacity = &ws->scrips_sym_capacity;
     } else if (type == 1) {
         map = ws->index_sym;
         count = &ws->index_sym_count;
         capacity = &ws->index_sym_capacity;
     } else {
         map = ws->dp_sym;
         count = &ws->dp_sym_count;
         capacity = &ws->dp_sym_capacity;
     }
     
     // Resize if needed
     if (*count >= *capacity) {
         size_t new_capacity = (*capacity == 0) ? 16 : *capacity * 2;
         topic_map_t* new_map = (topic_map_t*)realloc(map, new_capacity * sizeof(topic_map_t));
         if (!new_map) return;
         map = new_map;
         *capacity = new_capacity;
         
         if (type == 0) ws->scrips_sym = map;
         else if (type == 1) ws->index_sym = map;
         else ws->dp_sym = map;
     }
     
     // Add mapping
     map[*count].topic_id = topic_id;
     map[*count].hsm_token = strdup(hsm_token);
     (*count)++;
 }
 
 // Get or create response data entry
 static cJSON* get_resp_data(fyers_data_ws_t* ws, const char* hsm_token) {
     // Find existing entry
     for (size_t i = 0; i < ws->resp_map_count; i++) {
         if (strcmp(ws->resp_map[i].hsm_token, hsm_token) == 0) {
             return ws->resp_map[i].data;
         }
     }
     
     // Create new entry
     if (ws->resp_map_count >= ws->resp_map_capacity) {
         size_t new_capacity = (ws->resp_map_capacity == 0) ? 16 : ws->resp_map_capacity * 2;
         resp_entry_t* new_map = (resp_entry_t*)realloc(ws->resp_map, new_capacity * sizeof(resp_entry_t));
         if (!new_map) return NULL;
         ws->resp_map = new_map;
         ws->resp_map_capacity = new_capacity;
     }
     
     ws->resp_map[ws->resp_map_count].hsm_token = strdup(hsm_token);
     ws->resp_map[ws->resp_map_count].data = cJSON_CreateObject();
     ws->resp_map_count++;
     
     return ws->resp_map[ws->resp_map_count - 1].data;
 }
 
 // Format and output response (matching Python format exactly)
 static void response_output(fyers_data_ws_t* ws, cJSON* data_resp, const char* data_type, const char* hsm_token) {
     if (!ws || !data_resp || !ws->on_message) {
         return;
     }
     
     cJSON* response = cJSON_CreateObject();
     cJSON* precision_item = cJSON_GetObjectItem(data_resp, "precision");
     cJSON* multiplier_item = cJSON_GetObjectItem(data_resp, "multiplier");
     
     int precision = (precision_item && cJSON_IsNumber(precision_item)) ? (int)cJSON_GetNumberValue(precision_item) : 0;
     int multiplier = (multiplier_item && cJSON_IsNumber(multiplier_item)) ? (int)cJSON_GetNumberValue(multiplier_item) : 1;
     double precision_mult = pow(10.0, precision) * (double)multiplier;
     if (precision_mult <= 0) precision_mult = 1.0;
     
     // Fields that need precision/multiplier adjustment (matching Python)
     const char* precision_fields[] = {
         "ltp", "bid_price", "ask_price", "avg_trade_price",
         "low_price", "high_price", "open_price", "prev_close_price"
     };
     size_t precision_fields_count = sizeof(precision_fields) / sizeof(precision_fields[0]);
     
     if (ws->litemode && !cJSON_GetObjectItem(data_resp, "bidPrice1")) {
         // Lite mode: only ltp from data_resp; type and symbol (complete) are added below from hsm_token and symbol_token_map
         cJSON* ltp_item = cJSON_GetObjectItem(data_resp, "ltp");
         if (ltp_item && cJSON_IsNumber(ltp_item) && precision_mult > 0) {
             double val = cJSON_GetNumberValue(ltp_item) / precision_mult;
             cJSON_AddNumberToObject(response, "ltp", val);
         }
         
         // Calculate change and change percent
         cJSON* prev_close = cJSON_GetObjectItem(response, "prev_close_price");
         cJSON* ltp = cJSON_GetObjectItem(response, "ltp");
         if (prev_close && ltp && cJSON_IsNumber(prev_close) && cJSON_IsNumber(ltp)) {
             double ch = cJSON_GetNumberValue(ltp) - cJSON_GetNumberValue(prev_close);
             double chp = (ch / cJSON_GetNumberValue(prev_close)) * 100.0;
             cJSON_AddNumberToObject(response, "ch", round(ch * 100) / 100.0);
             cJSON_AddNumberToObject(response, "chp", round(chp * 100) / 100.0);
         }
     } else {
         // Full mode processing
         const char** field_array = NULL;
         size_t field_count = 0;
         
         if (strcmp(data_type, "depth") == 0) {
             field_array = ws->depthvalue;
             field_count = 32;
         } else if (strcmp(data_type, "scrips") == 0) {
             field_array = ws->data_val;
             field_count = 22;
         } else if (strcmp(data_type, "index") == 0) {
             field_array = ws->index_val;
             field_count = 8;
         }
         
         if (field_array) {
             for (size_t i = 0; i < field_count; i++) {
                 const char* field = field_array[i];
                 cJSON* item = cJSON_GetObjectItem(data_resp, field);
                 if (!item) continue;
                 
                 // Skip symbol and type from data_resp; canonical values (complete symbol, type prefix) are added below
                 if (strcmp(field, "symbol") == 0 || strcmp(field, "type") == 0) {
                     continue;
                 }
                 // Exclude lower_ckt and upper_ckt from precision calculation (matching Python line 956)
                 if (strcmp(field, "lower_ckt") == 0 || strcmp(field, "upper_ckt") == 0) {
                     continue; // Skip - will be set to 0 later
                 }
                 
                 // Check if field needs precision adjustment
                 bool needs_precision = false;
                 for (size_t j = 0; j < precision_fields_count; j++) {
                     if (strcmp(field, precision_fields[j]) == 0) {
                         needs_precision = true;
                         break;
                     }
                 }
                 
                 if (needs_precision && cJSON_IsNumber(item) && precision_mult > 0) {
                     double val = cJSON_GetNumberValue(item) / precision_mult;
                     cJSON_AddNumberToObject(response, field, val);
                 } else {
                     cJSON_AddItemToObject(response, field, cJSON_Duplicate(item, 1));
                 }
             }
         }
         
         // Set defaults for scrips (matching Python lines 965-966)
         if (strcmp(data_type, "scrips") == 0) {
             // Set lower_ckt and upper_ckt to 0 (overwriting any parsed values)
             cJSON_AddNumberToObject(response, "lower_ckt", 0);
             cJSON_AddNumberToObject(response, "upper_ckt", 0);
             
             // Calculate change and change percent (matching Python lines 967-969)
             cJSON* prev_close = cJSON_GetObjectItem(response, "prev_close_price");
             cJSON* ltp = cJSON_GetObjectItem(response, "ltp");
             if (prev_close && ltp && cJSON_IsNumber(prev_close) && cJSON_IsNumber(ltp) && 
                 cJSON_GetNumberValue(prev_close) != 0) {
                 double ch = cJSON_GetNumberValue(ltp) - cJSON_GetNumberValue(prev_close);
                 double chp = (ch / cJSON_GetNumberValue(prev_close)) * 100.0;
                 cJSON_AddNumberToObject(response, "ch", round(ch * 10000) / 10000.0);
                 cJSON_AddNumberToObject(response, "chp", round(chp * 10000) / 10000.0);
             }
             
             // Remove unwanted fields (matching Python lines 970-973)
             cJSON_DeleteItemFromObject(response, "OI");
             cJSON_DeleteItemFromObject(response, "Yhigh");
             cJSON_DeleteItemFromObject(response, "Ylow");
         } else if (strcmp(data_type, "index") == 0) {
             // Calculate change and change percent for index
             cJSON* prev_close = cJSON_GetObjectItem(response, "prev_close_price");
             cJSON* ltp = cJSON_GetObjectItem(response, "ltp");
             if (prev_close && ltp && cJSON_IsNumber(prev_close) && cJSON_IsNumber(ltp)) {
                 double ch = cJSON_GetNumberValue(ltp) - cJSON_GetNumberValue(prev_close);
                 double chp = (ch / cJSON_GetNumberValue(prev_close)) * 100.0;
                 cJSON_AddNumberToObject(response, "ch", round(ch * 100) / 100.0);
                 cJSON_AddNumberToObject(response, "chp", round(chp * 100) / 100.0);
             }
         }
     }
     
     // Extract type prefix from HSM token (matching Python: "sf|nse_cm|3045" -> "sf")
     if (hsm_token && strlen(hsm_token) >= 2) {
         char type_prefix[3] = {0};
         memcpy(type_prefix, hsm_token, 2);
         cJSON_AddStringToObject(response, "type", type_prefix);
     }
     
     // Look up symbol from symbol_token_map (matching Python: self.symbol_token[topic_name])
     if (hsm_token) {
         for (size_t i = 0; i < ws->symbol_token_count; i++) {
             if (strcmp(ws->symbol_token_map[i].hsm_token, hsm_token) == 0) {
                 cJSON_AddStringToObject(response, "symbol", ws->symbol_token_map[i].symbol);
                 break;
             }
         }
     }
     
     // Convert to compact JSON string (matching Python dict format)
     char* json_str = cJSON_PrintUnformatted(response);
     if (json_str) {
         ws->on_message(ws, json_str);
         free(json_str);
     }
     cJSON_Delete(response);
 }
 
 // Parse mode response (type 12)
 static void parse_mode_response(fyers_data_ws_t* ws, const uint8_t* data, size_t len) {
     if (!ws || !data || len < 4) return;
     
     size_t offset = 3;
     uint8_t field_count = data[offset++];
     
     if (field_count >= 1) {
         offset++; // Skip field ID
         uint16_t field_length = ntohs(*(uint16_t*)(data + offset));
         offset += 2;
         
         if (offset + field_length <= len) {
             char* string_val = (char*)malloc(field_length + 1);
             memcpy(string_val, data + offset, field_length);
             string_val[field_length] = '\0';
             
             if (strcmp(string_val, "K") == 0) {
                 if (ws->on_message) {
                     if (ws->litemode) {
                         ws->on_message(ws, "{\"type\":\"lit\",\"code\":200,\"message\":\"Lite Mode On\",\"s\":\"ok\"}");
                     } else {
                         ws->on_message(ws, "{\"type\":\"ful\",\"code\":200,\"message\":\"Full Mode On\",\"s\":\"ok\"}");
                     }
                 }
                 
                 // Note: on_connect callback is called from fyers_data_ws_connect() after delay
                 // matching Python flow where on_open() is called from connect() after 2 seconds
             } else {
                 if (ws->on_error) {
                     ws->on_error(ws, 12001, "Mode change failed");
                 }
             }
             free(string_val);
         }
     }
 }
 
 // Parse datafeed response (type 6) - Main parsing function matching Python flow
 static void parse_datafeed_response(fyers_data_ws_t* ws, const uint8_t* data, size_t len) {
     if (!ws || !data || len < 9) return;
     
     // Handle ACK if needed
     if (ws->ack_count > 0) {
         ws->update_count++;
         uint32_t message_num = ntohl(*(uint32_t*)(data + 3));
         ws->last_message_num = message_num;
         
         if (ws->update_count == ws->ack_count) {
             // Send ACK message
             uint8_t ack_msg[11];
             uint16_t total_size = htons(9);
             memcpy(ack_msg, &total_size, 2);
             ack_msg[2] = 3; // ACK type
             ack_msg[3] = 1;  // Field count
             ack_msg[4] = 1;  // Field ID
             uint16_t field_size = htons(4);
             memcpy(ack_msg + 5, &field_size, 2);
             uint32_t msg_num = htonl(message_num);
             memcpy(ack_msg + 7, &msg_num, 4);
             add_message_to_queue(ws, ack_msg, 11);
             ws->update_count = 0;
         }
     }
     
     // Parse scrip count
     uint16_t scrip_count = ntohs(*(uint16_t*)(data + 7));
     size_t offset = 9;
     
     for (uint16_t scrip_idx = 0; scrip_idx < scrip_count && offset < len; scrip_idx++) {
         if (offset >= len) break;
         
         uint8_t data_type = data[offset++];
         
         if (data_type == DATA_TYPE_SNAPSHOT) {
             // Snapshot data (type 83)
             if (offset + 3 > len) break;
             
             uint16_t topic_id = ntohs(*(uint16_t*)(data + offset));
             offset += 2;
             
             uint8_t topic_name_len = data[offset++];
             if (offset + topic_name_len > len) break;
             
             char* topic_name = (char*)malloc(topic_name_len + 1);
             memcpy(topic_name, data + offset, topic_name_len);
             topic_name[topic_name_len] = '\0';
             offset += topic_name_len;
             
             // Determine type based on prefix
             int sym_type = -1;
             const char* data_type_str = NULL;
             const char** field_array = NULL;
             size_t field_count = 0;
             
             if (strncmp(topic_name, "dp", 2) == 0) {
                 sym_type = 2; // depth
                 data_type_str = "depth";
                 field_array = ws->depthvalue;
                 field_count = 32;
                 add_topic_mapping(ws, topic_id, topic_name, 2);
             } else if (strncmp(topic_name, "if", 2) == 0) {
                 sym_type = 1; // index
                 data_type_str = "index";
                 field_array = ws->index_val;
                 field_count = 8;
                 add_topic_mapping(ws, topic_id, topic_name, 1);
             } else if (strncmp(topic_name, "sf", 2) == 0) {
                 sym_type = 0; // scrips
                 data_type_str = "scrips";
                 field_array = ws->data_val;
                 field_count = 22;
                 add_topic_mapping(ws, topic_id, topic_name, 0);
             }
             
             if (sym_type >= 0 && field_array) {
                 cJSON* resp_data = get_resp_data(ws, topic_name);
                 
                 if (offset + 1 > len) {
                     free(topic_name);
                     break;
                 }
                 
                 uint8_t field_count_val = data[offset++];
                 
                 // Parse fields
                 for (uint8_t i = 0; i < field_count_val && i < field_count && offset + 4 <= len; i++) {
                     int32_t value = (int32_t)ntohl(*(uint32_t*)(data + offset));
                     offset += 4;
                     
                     if (value != -2147483648 && i < field_count) {
                         cJSON_AddNumberToObject(resp_data, field_array[i], value);
                     }
                 }
                 
                 // Skip 2 bytes
                 offset += 2;
                 
                 // Parse multiplier
                 if (offset + 2 <= len) {
                     uint16_t multiplier = ntohs(*(uint16_t*)(data + offset));
                     offset += 2;
                     cJSON_AddNumberToObject(resp_data, "multiplier", multiplier);
                 }
                 
                 // Parse precision
                 if (offset + 1 <= len) {
                     uint8_t precision = data[offset++];
                     cJSON_AddNumberToObject(resp_data, "precision", precision);
                 }
                 
                 // Parse exchange, exchange_token, symbol strings
                 const char* string_fields[] = {"exchange", "exchange_token", "symbol"};
                 for (int i = 0; i < 3 && offset < len; i++) {
                     uint8_t str_len = data[offset++];
                     if (offset + str_len <= len) {
                         char* str_val = (char*)malloc(str_len + 1);
                         memcpy(str_val, data + offset, str_len);
                         str_val[str_len] = '\0';
                         cJSON_AddStringToObject(resp_data, string_fields[i], str_val);
                         offset += str_len;
                         free(str_val);
                     }
                 }
                 
                 // Output response (type and symbol will be added in response_output)
                 response_output(ws, resp_data, data_type_str, topic_name);
             }
             
             free(topic_name);
             
         } else if (data_type == DATA_TYPE_FULL_MODE) {
             // Full mode update (type 85)
             if (offset + 3 > len) break;
             
             uint16_t topic_id = ntohs(*(uint16_t*)(data + offset));
             offset += 2;
             
             if (offset + 1 > len) break;
             uint8_t field_count_val = data[offset++];
             
             // Find topic name
             char* topic_name = NULL;
             const char* data_type_str = NULL;
             const char** field_array = NULL;
             size_t field_count = 0;
             
             topic_name = get_topic_name(ws, topic_id, 0);
             if (topic_name) {
                 data_type_str = "scrips";
                 field_array = ws->data_val;
                 field_count = 22;
             } else {
                 topic_name = get_topic_name(ws, topic_id, 1);
                 if (topic_name) {
                     data_type_str = "index";
                     field_array = ws->index_val;
                     field_count = 8;
                 } else {
                     topic_name = get_topic_name(ws, topic_id, 2);
                     if (topic_name) {
                         data_type_str = "depth";
                         field_array = ws->depthvalue;
                         field_count = 32;
                     }
                 }
             }
             
             if (topic_name && field_array) {
                 cJSON* resp_data = get_resp_data(ws, topic_name);
                 ws->update_tick = false;
                 
                 // Parse fields
                 for (uint8_t i = 0; i < field_count_val && i < field_count && offset + 4 <= len; i++) {
                     int32_t value = (int32_t)ntohl(*(uint32_t*)(data + offset));
                     offset += 4;
                     
                     if (value != -2147483648) {
                         cJSON* existing = cJSON_GetObjectItem(resp_data, field_array[i]);
                         if (existing && cJSON_IsNumber(existing)) {
                             int32_t old_val = (int32_t)cJSON_GetNumberValue(existing);
                             if (old_val != value) {
                                 cJSON_SetNumberValue(existing, value);
                                 ws->update_tick = true;
                             }
                         } else if (!existing) {
                             cJSON_AddNumberToObject(resp_data, field_array[i], value);
                             ws->update_tick = true;
                         }
                     }
                 }
                 
                 if (ws->update_tick && resp_data) {
                     response_output(ws, resp_data, data_type_str, topic_name);
                 }
             }
             
         } else if (data_type == DATA_TYPE_LITE_MODE) {
             // Lite mode update (type 76)
             if (offset + 3 > len) break;
             
             uint16_t topic_id = ntohs(*(uint16_t*)(data + offset));
             offset += 2;
             
             if (offset + 4 > len) break;
             int32_t value = (int32_t)ntohl(*(uint32_t*)(data + offset));
             offset += 4;
             
             // Find topic name
             char* topic_name = get_topic_name(ws, topic_id, 0);
             if (!topic_name) {
                 topic_name = get_topic_name(ws, topic_id, 1);
             }
             
             if (topic_name && value != -2147483648) {
                 cJSON* resp_data = get_resp_data(ws, topic_name);
                 if (resp_data) {
                     cJSON* ltp_item = cJSON_GetObjectItem(resp_data, ws->lite_val[0]);
                     
                     bool should_update = false;
                     if (!ltp_item) {
                         cJSON_AddNumberToObject(resp_data, ws->lite_val[0], value);
                         should_update = true;
                     } else if (cJSON_IsNumber(ltp_item)) {
                         int32_t old_val = (int32_t)cJSON_GetNumberValue(ltp_item);
                         if (old_val != value) {
                             cJSON_SetNumberValue(ltp_item, value);
                             should_update = true;
                         }
                     }
                     
                     if (should_update) {
                         // Type and symbol will be added in response_output
                         response_output(ws, resp_data, strncmp(topic_name, "if", 2) == 0 ? "index" : "scrips", topic_name);
                     }
                 }
             }
         }
     }
 }
 
 // Convert symbols to HSM tokens via REST API
 static fyers_error_t convert_symbols_to_hsm_tokens(fyers_data_ws_t* ws, const char** symbols, size_t symbol_count, 
                                                     symbol_map_t** result_map, size_t* result_count) {
     if (!ws || !symbols || symbol_count == 0 || !result_map || !result_count) {
         return FYERS_ERROR_INVALID_PARAM;
     }
     
     // Build JSON request
     cJSON* json = cJSON_CreateObject();
     cJSON* symbols_array = cJSON_CreateArray();
     
     for (size_t i = 0; i < symbol_count; i++) {
         cJSON_AddItemToArray(symbols_array, cJSON_CreateString(symbols[i]));
     }
     cJSON_AddItemToObject(json, "symbols", symbols_array);
     
     char* json_str = cJSON_Print(json);
     cJSON_Delete(json);
     
     if (!json_str) {
         return FYERS_ERROR_MEMORY;
     }
     
     // Make POST request
     fyers_response_t* response = fyers_http_client_post(ws->http_client, SYMBOL_TOKEN_API, ws->access_token, json_str);
     free(json_str);
     
     if (!response || response->error != FYERS_OK) {
         if (response) {
             fyers_response_destroy(response);
         }
         return FYERS_ERROR_NETWORK;
     }
     
     // Parse response
     cJSON* resp_json = cJSON_Parse(response->data);
     fyers_response_destroy(response);
     
     if (!resp_json) {
         return FYERS_ERROR;
     }
     
     cJSON* status = cJSON_GetObjectItem(resp_json, "s");
     if (!status || strcmp(status->valuestring, "ok") != 0) {
         cJSON* msg = cJSON_GetObjectItem(resp_json, "message");
         if (ws->on_error && msg) {
             ws->on_error(ws, -300, msg->valuestring);
         }
         cJSON_Delete(resp_json);
         return FYERS_ERROR;
     }
     
     cJSON* valid_symbols = cJSON_GetObjectItem(resp_json, "validSymbol");
     if (!valid_symbols) {
         cJSON_Delete(resp_json);
         return FYERS_ERROR;
     }
     
     // Allocate result map
     size_t valid_count = cJSON_GetArraySize(valid_symbols);
     *result_map = (symbol_map_t*)calloc(valid_count, sizeof(symbol_map_t));
     if (!*result_map) {
         cJSON_Delete(resp_json);
         return FYERS_ERROR_MEMORY;
     }
     
     // Process valid symbols
     cJSON* item = NULL;
     size_t idx = 0;
     cJSON_ArrayForEach(item, valid_symbols) {
         if (idx >= valid_count) break;
         
         const char* symbol = item->string;
         const char* fytoken = cJSON_GetStringValue(item);
         
         if (!symbol || !fytoken) continue;
         
         // Extract exchange segment (first 4 chars)
         if (strlen(fytoken) < 4) continue;
         
         char ex_sg[5] = {0};
         memcpy(ex_sg, fytoken, 4);
         
         // Look up exchange segment in exch_seg_dict
         const char* segment = get_exchange_segment(ex_sg);
         if (!segment) {
             // Unknown exchange segment, skip
             continue;
         }
         
         // Build HSM token based on symbol type (matching Python logic)
         char hsm_token[256];
         bool update_dict = true;
         const char* exch_token = NULL;
         
         // Check if symbol ends with "-INDEX"
         size_t symbol_len = strlen(symbol);
         bool is_index = (symbol_len > 6 && strcmp(symbol + symbol_len - 6, "-INDEX") == 0);
         
         if (is_index && ws->data_type != FYERS_DATA_TYPE_DEPTH_UPDATE) {
             // Index symbol - look up in index_dict
             const char* index_token = get_index_token(symbol);
             if (index_token) {
                 exch_token = index_token;
             } else {
                 // Fallback: extract from symbol name
                 // Format: "NSE:NIFTY50-INDEX" -> "NIFTY50"
                 const char* colon = strchr(symbol, ':');
                 if (colon) {
                     const char* dash = strchr(colon + 1, '-');
                     if (dash) {
                         size_t token_len = dash - colon - 1;
                         char* temp_token = (char*)malloc(token_len + 1);
                         memcpy(temp_token, colon + 1, token_len);
                         temp_token[token_len] = '\0';
                         exch_token = temp_token;
                     }
                 }
             }
             
             if (exch_token) {
                 snprintf(hsm_token, sizeof(hsm_token), "if|%s|%s", segment, exch_token);
                 if (exch_token != get_index_token(symbol)) {
                     free((void*)exch_token);  // Free if we allocated it
                 }
             } else {
                 continue;  // Could not determine exchange token
             }
         } else if (ws->data_type == FYERS_DATA_TYPE_DEPTH_UPDATE && !is_index) {
             // DepthUpdate for non-index symbols
             if (strlen(fytoken) < 10) continue;
             exch_token = fytoken + 10;
             snprintf(hsm_token, sizeof(hsm_token), "dp|%s|%s", segment, exch_token);
         } else if (ws->data_type == FYERS_DATA_TYPE_SYMBOL_UPDATE) {
             // SymbolUpdate - use exchange token from fytoken
             if (strlen(fytoken) < 10) continue;
             exch_token = fytoken + 10;
             snprintf(hsm_token, sizeof(hsm_token), "sf|%s|%s", segment, exch_token);
         } else if (ws->data_type == FYERS_DATA_TYPE_DEPTH_UPDATE && is_index) {
             // DepthUpdate for index symbols - skip (not supported)
             update_dict = false;
             continue;
         } else {
             // Default: SymbolUpdate
             if (strlen(fytoken) < 10) continue;
             exch_token = fytoken + 10;
             snprintf(hsm_token, sizeof(hsm_token), "sf|%s|%s", segment, exch_token);
         }
         
         if (!update_dict) {
             continue;
         }
         
         (*result_map)[idx].symbol = strdup(symbol);
         (*result_map)[idx].hsm_token = strdup(hsm_token);
         idx++;
     }
     
     *result_count = idx;
     cJSON_Delete(resp_json);
     
     return FYERS_OK;
 }
 
 // Free last subscription storage
 static void free_last_subscription(fyers_data_ws_t* ws) {
     if (!ws || !ws->last_subscription_symbols) return;
     for (size_t i = 0; i < ws->last_subscription_count; i++) {
         free(ws->last_subscription_symbols[i]);
     }
     free(ws->last_subscription_symbols);
     ws->last_subscription_symbols = NULL;
     ws->last_subscription_count = 0;
 }
 
 // Resubscribe using stored symbols with retry (for network stability after reconnect)
 static fyers_error_t resubscribe_with_retry(fyers_data_ws_t* ws) {
     if (!ws || !ws->last_subscription_symbols || ws->last_subscription_count == 0) {
         return FYERS_ERROR_INVALID_PARAM;
     }
     const int max_retries = 3;
     const int retry_delay_sec = 2;
     
     for (int attempt = 0; attempt < max_retries; attempt++) {
         if (!ws->thread_running) return FYERS_ERROR;
         
         if (attempt > 0) {
             sleep(retry_delay_sec);
         }
         
         ws->data_type = ws->last_subscription_data_type;
         symbol_map_t* hsm_map = NULL;
         size_t hsm_count = 0;
         fyers_error_t err = convert_symbols_to_hsm_tokens(ws,
             (const char**)ws->last_subscription_symbols,
             ws->last_subscription_count,
             &hsm_map, &hsm_count);
         
         if (err == FYERS_OK && hsm_count > 0) {
             const char** hsm_tokens = (const char**)malloc(hsm_count * sizeof(char*));
             if (!hsm_tokens) {
                 for (size_t i = 0; i < hsm_count; i++) {
                     free(hsm_map[i].symbol);
                     free(hsm_map[i].hsm_token);
                 }
                 free(hsm_map);
                 return FYERS_ERROR_MEMORY;
             }
             for (size_t i = 0; i < hsm_count; i++) {
                 hsm_tokens[i] = hsm_map[i].hsm_token;
             }
             
             uint8_t sub_msg[65536];
             size_t sub_size = sizeof(sub_msg);
             ws->channel_num = ws->last_subscription_channel;
             create_subscription_message(ws, hsm_tokens, hsm_count, sub_msg, &sub_size);
             add_message_to_queue(ws, sub_msg, sub_size);
             
             ws->symbol_token_map = hsm_map;
             ws->symbol_token_count = hsm_count;
             free(hsm_tokens);
             return FYERS_OK;
         }
         
         if (hsm_map && hsm_count > 0) {
             for (size_t i = 0; i < hsm_count; i++) {
                 free(hsm_map[i].symbol);
                 free(hsm_map[i].hsm_token);
             }
             free(hsm_map);
         }
     }
     return FYERS_ERROR_NETWORK;
 }
 
 // Clear state before reconnect (matching Python: scrips_per_channel, symbol_token)
 static void clear_reconnect_state(fyers_data_ws_t* ws) {
     if (!ws) return;
     
     pthread_mutex_lock(&ws->ws_mutex);
     
     // Clear symbol token map
     for (size_t i = 0; i < ws->symbol_token_count; i++) {
         free(ws->symbol_token_map[i].symbol);
         free(ws->symbol_token_map[i].hsm_token);
     }
     free(ws->symbol_token_map);
     ws->symbol_token_map = NULL;
     ws->symbol_token_count = 0;
     
     // Clear scrips per channel (matching Python: self.scrips_per_channel[self.channel_num] = [])
     if (ws->scrips_per_channel) {
         for (int i = 0; i < 31; i++) {
             ws->scrips_per_channel[i] = 0;
         }
     }
     
     // Clear topic mappings (new connection may have different topic IDs)
     for (size_t i = 0; i < ws->scrips_sym_count; i++) {
         free(ws->scrips_sym[i].hsm_token);
     }
     free(ws->scrips_sym);
     ws->scrips_sym = NULL;
     ws->scrips_sym_count = 0;
     ws->scrips_sym_capacity = 0;
     
     for (size_t i = 0; i < ws->index_sym_count; i++) {
         free(ws->index_sym[i].hsm_token);
     }
     free(ws->index_sym);
     ws->index_sym = NULL;
     ws->index_sym_count = 0;
     ws->index_sym_capacity = 0;
     
     for (size_t i = 0; i < ws->dp_sym_count; i++) {
         free(ws->dp_sym[i].hsm_token);
     }
     free(ws->dp_sym);
     ws->dp_sym = NULL;
     ws->dp_sym_count = 0;
     ws->dp_sym_capacity = 0;
     
     // Clear response map
     for (size_t i = 0; i < ws->resp_map_count; i++) {
         free(ws->resp_map[i].hsm_token);
         cJSON_Delete(ws->resp_map[i].data);
     }
     free(ws->resp_map);
     ws->resp_map = NULL;
     ws->resp_map_count = 0;
     ws->resp_map_capacity = 0;
     
     pthread_mutex_unlock(&ws->ws_mutex);
 }
 
 // Create new client connection (reuses existing lws_context)
 static struct lws* create_client_connection(fyers_data_ws_t* ws) {
     if (!ws || !ws->lws_context) return NULL;
     
     struct lws_client_connect_info ccinfo;
     memset(&ccinfo, 0, sizeof(ccinfo));
     ccinfo.context = ws->lws_context;
     ccinfo.address = "socket.fyers.in";
     ccinfo.port = 443;
     ccinfo.path = "/hsm/v1-5/prod";
     ccinfo.host = "socket.fyers.in";
     ccinfo.origin = "socket.fyers.in";
     ccinfo.protocol = "";  // Empty protocol - let server negotiate (matching Python)
     ccinfo.ssl_connection = LCCSCF_USE_SSL | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
     ccinfo.userdata = ws;
     
     return lws_client_connect_via_info(&ccinfo);
 }
 
 // Reconnection thread - matches Python __on_close reconnect logic
 static void* reconnection_thread(void* arg) {
     fyers_data_ws_t* ws = (fyers_data_ws_t*)arg;
     if (!ws) return NULL;
     
     int attempt_num = ws->reconnect_attempts + 1;  // 1-based for display (matching Python)
     
     // Log "Attempting reconnect X of Y" (matching Python)
     if (ws->logger) {
         if (ws->write_to_file) {
             fyers_logger_debug(ws->logger, "Attempting reconnect %d of %d...", attempt_num, ws->max_reconnect_attempts);
         } else {
             fyers_logger_info(ws->logger, "Attempting reconnect %d of %d...", attempt_num, ws->max_reconnect_attempts);
         }
     }
     
     // Python: if (reconnect_attempts) % 5 == 0: reconnect_delay += 5
     if ((ws->reconnect_attempts % 5) == 0) {
         ws->reconnect_delay += 5;
     }
     
     // Python: time.sleep(reconnect_delay)
     sleep((unsigned int)ws->reconnect_delay);
     
     // Check if we should abort (e.g. close_connection was called)
     if (!ws->thread_running || !ws->restart_flag) {
         ws->reconnect_thread_active = false;
         return NULL;
     }
     
     // Python: reconnect_attempts += 1
     ws->reconnect_attempts++;
     
     // Python: clear state
     clear_reconnect_state(ws);
     
     // Python: self.connect() -> create new connection
     struct lws* new_wsi = create_client_connection(ws);
     if (!new_wsi) {
         if (ws->on_error) {
             ws->on_error(ws, -1, "Reconnection failed");
         }
         ws->reconnect_thread_active = false;
         return NULL;
     }
     
     // Wait for connection to establish (up to 30 seconds)
     int wait_count = 0;
     while (ws->thread_running && wait_count < 300) {
         usleep(100000);  // 100ms
         wait_count++;
         if (ws->connected) break;
     }
     
     if (!ws->connected || !ws->thread_running) {
         ws->reconnect_thread_active = false;
         return NULL;
     }
     
     // Python: connect() waits 2 seconds then calls on_open()
     sleep(2);
     
     if (!ws->thread_running) {
         ws->reconnect_thread_active = false;
         return NULL;
     }
     
     // Reset reconnect state on successful reconnect (matching Python __on_open)
     ws->reconnect_attempts = 0;
     ws->reconnect_delay = 0;
     
     // Auto resubscribe with retry - network may need time to stabilize after reconnect
     // (Python: user subscribes in on_connect; we store and retry to handle CURL/HTTP flakiness)
     if (ws->last_subscription_symbols && ws->last_subscription_count > 0) {
         sleep(1);  // Extra delay for network stability
         if (ws->thread_running && resubscribe_with_retry(ws) != FYERS_OK) {
             if (ws->on_error) {
                 ws->on_error(ws, -1, "Re-subscribe failed after reconnect");
             }
         }
     }
     
     // Call on_connect so user can do additional setup (matching Python flow)
     if (ws->thread_running && ws->on_connect) {
         ws->on_connect(ws);
     }
     
     ws->reconnect_thread_active = false;
     return NULL;
 }
 
 // WebSocket protocol callbacks
 static int callback_fyers_data_ws(struct lws* wsi, enum lws_callback_reasons reason,
                                    void* user, void* in, size_t len) {
     fyers_data_ws_t* ws = (fyers_data_ws_t*)user;
     
     if (!ws) {
         return 0;
     }
     
     switch (reason) {
         case LWS_CALLBACK_CLIENT_ESTABLISHED:
             ws->connected = true;
             ws->lws_wsi = wsi;  // Ensure wsi is set
             
             // Reset reconnect state on successful connection (matching Python __on_open)
             ws->reconnect_attempts = 0;
             ws->reconnect_delay = 0;
             
             // Send authentication message
             {
                 uint8_t auth_msg[1024];
                 size_t auth_size = sizeof(auth_msg);
                 create_auth_message(ws, auth_msg, &auth_size);
                 add_message_to_queue(ws, auth_msg, auth_size);
             }
             
             // Send mode message (lite or full)
             {
                 uint8_t mode_msg[32];
                 size_t mode_size = sizeof(mode_msg);
                 // Create mode message (simplified)
                 mode_msg[0] = 0; mode_msg[1] = 0;
                 mode_msg[2] = 12; // Request type
                 mode_msg[3] = 2;  // Field count
                 // Field 1: Channel bits
                 mode_msg[4] = 1;
                 mode_msg[5] = 0; mode_msg[6] = 8;
                 uint64_t channel_bits = 1ULL << ws->channel_num;
                 memcpy(mode_msg + 7, &channel_bits, 8);
                 // Field 2: Mode (76 for lite, 70 for full)
                 mode_msg[15] = 2;
                 mode_msg[16] = 0; mode_msg[17] = 1;
                 mode_msg[18] = ws->litemode ? 76 : 70;
                 mode_size = 19;
                 add_message_to_queue(ws, mode_msg, mode_size);
             }
             
             // Request WRITEABLE callback to send queued messages
             lws_callback_on_writable(ws->lws_wsi);
             break;
             
         case LWS_CALLBACK_CLIENT_WRITEABLE:
             // Send queued messages
             pthread_mutex_lock(&ws->message_mutex);
             struct message_queue* msg = ws->message_queue_head;
             if (msg) {
                 // Remove from queue
                 ws->message_queue_head = msg->next;
                 if (!ws->message_queue_head) {
                     ws->message_queue_tail = NULL;
                 }
                 pthread_mutex_unlock(&ws->message_mutex);
                 
                 // Prepare buffer with LWS_PRE padding
                 size_t total_size = LWS_PRE + msg->size;
                 uint8_t* send_buf = (uint8_t*)malloc(total_size);
                 if (send_buf) {
                     memcpy(send_buf + LWS_PRE, msg->data, msg->size);
                     int n = lws_write(ws->lws_wsi, send_buf + LWS_PRE, msg->size, LWS_WRITE_BINARY);
                     free(send_buf);
                     
                     if (n < (int)msg->size) {
                         // Write failed or incomplete - put message back at head
                         pthread_mutex_lock(&ws->message_mutex);
                         msg->next = ws->message_queue_head;
                         ws->message_queue_head = msg;
                         if (!ws->message_queue_tail) {
                             ws->message_queue_tail = msg;
                         }
                         pthread_mutex_unlock(&ws->message_mutex);
                     } else {
                         // Successfully sent
                         free(msg->data);
                         free(msg);
                         
                         // Request another WRITEABLE callback if more messages queued
                         pthread_mutex_lock(&ws->message_mutex);
                         if (ws->message_queue_head) {
                             lws_callback_on_writable(ws->lws_wsi);
                         }
                         pthread_mutex_unlock(&ws->message_mutex);
                     }
                 } else {
                     free(msg->data);
                     free(msg);
                 }
             } else {
                 pthread_mutex_unlock(&ws->message_mutex);
             }
             break;
             
         case LWS_CALLBACK_CLIENT_RECEIVE:
             // Handle incoming binary data
             if (in && len > 0) {
                 const uint8_t* data = (const uint8_t*)in;
                 
                 // Parse response type
                 if (len >= 3) {
                     uint8_t resp_type = data[2];
                     
                     switch (resp_type) {
                         case MSG_TYPE_AUTH:
                             // Authentication response - parse properly matching Python
                             {
                                 size_t offset = 4;
                                 offset++; // Skip field ID
                                 if (offset + 2 <= len) {
                                     uint16_t field_length = ntohs(*(uint16_t*)(data + offset));
                                     offset += 2;
                                     if (offset + field_length <= len) {
                                         char* string_val = (char*)malloc(field_length + 1);
                                         memcpy(string_val, data + offset, field_length);
                                         string_val[field_length] = '\0';
                                         
                                         if (strcmp(string_val, "K") == 0) {
                                             if (ws->on_message) {
                                                 ws->on_message(ws, "{\"type\":\"cn\",\"code\":200,\"message\":\"Authentication done\",\"s\":\"ok\"}");
                                             }
                                         } else {
                                             if (ws->on_error) {
                                                 ws->on_error(ws, 11001, "Authentication failed");
                                             }
                                         }
                                         free(string_val);
                                         offset += field_length;
                                     }
                                 }
                                 
                                 // Extract ack_count
                                 offset++; // Skip field ID
                                 if (offset + 2 <= len) {
                                     uint16_t field_length = ntohs(*(uint16_t*)(data + offset));
                                     offset += 2;
                                     if (offset + 4 <= len) {
                                         ws->ack_count = ntohl(*(uint32_t*)(data + offset));
                                     }
                                 }
                             }
                             break;
                             
                         case MSG_TYPE_DATA_FEED:
                             // Data feed - parse binary format
                             parse_datafeed_response(ws, data, len);
                             break;
                             
                         case MSG_TYPE_MODE:
                             // Mode response (lite/full mode)
                             parse_mode_response(ws, data, len);
                             break;
                             
                         case MSG_TYPE_SUBSCRIBE:
                             // Subscription response - parse properly matching Python
                             {
                                 size_t offset = 5;
                                 if (offset + 2 <= len) {
                                     uint16_t field_length = ntohs(*(uint16_t*)(data + offset));
                                     offset += 2;
                                     if (offset + field_length <= len) {
                                         char* string_val = (char*)malloc(field_length + 1);
                                         memcpy(string_val, data + offset, field_length);
                                         string_val[field_length] = '\0';
                                         
                                         if (strcmp(string_val, "K") == 0) {
                                             if (ws->on_message) {
                                                 ws->on_message(ws, "{\"type\":\"sub\",\"code\":200,\"message\":\"Subscribed\",\"s\":\"ok\"}");
                                             }
                                         } else {
                                             if (ws->on_error) {
                                                 ws->on_error(ws, 11011, "subscription failed");
                                             }
                                         }
                                         free(string_val);
                                     }
                                 }
                             }
                             break;
 
                         case MSG_TYPE_UNSUBSCRIBE:
                             /* Unsubscription response - match Python __unsubscribe_resp */
                             {
                                 size_t offset = 5;
                                 if (offset + 2 <= len) {
                                     uint16_t field_length = ntohs(*(uint16_t*)(data + offset));
                                     offset += 2;
                                     if (offset + field_length <= len) {
                                         char* string_val = (char*)malloc(field_length + 1);
                                         if (string_val) {
                                             memcpy(string_val, data + offset, field_length);
                                             string_val[field_length] = '\0';
 
                                             if (strcmp(string_val, "K") == 0) {
                                                 if (ws->on_message) {
                                                     ws->on_message(ws, "{\"type\":\"unsub\",\"code\":200,\"message\":\"Unsubscribed\",\"s\":\"ok\"}");
                                                 }
                                             } else {
                                                 if (ws->on_error) {
                                                     ws->on_error(ws, 11012, "Unsubscribe failed");
                                                 }
                                             }
                                             free(string_val);
                                         }
                                     }
                                 }
                             }
                             break;
                     }
                 }
             }
             break;
             
         case LWS_CALLBACK_CLIENT_CLOSED:
             ws->connected = false;
             ws->lws_wsi = NULL;
             
             // Notify user of connection loss (matching Python "Error: Connection to remote host was lost")
             if (ws->on_error) {
                 ws->on_error(ws, -1, "Connection to remote host was lost.");
             }
             
             // Handle reconnection (matching Python __on_close)
             if (ws->restart_flag && ws->reconnect_attempts < ws->max_reconnect_attempts) {
                 // Spawn reconnection thread - do NOT block in callback
                 ws->reconnect_thread_active = true;
                 if (pthread_create(&ws->reconnect_thread, NULL, reconnection_thread, ws) != 0) {
                     ws->reconnect_thread_active = false;
                     if (ws->on_close) {
                         ws->on_close(ws, "Connection closed");
                     }
                 }
                 // Note: on_close is NOT called when reconnecting (matching Python)
             } else {
                 if (ws->restart_flag && ws->reconnect_attempts >= ws->max_reconnect_attempts) {
                     if (ws->logger) {
                         fyers_logger_info(ws->logger, "Max reconnect attempts reached. Connection abandoned.");
                     }
                 }
                 if (ws->on_close) {
                     ws->on_close(ws, "Connection closed");
                 }
             }
             break;
             
         case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
             ws->connected = false;
             ws->lws_wsi = NULL;
             if (ws->on_error) {
                 ws->on_error(ws, -1, "Connection error");
             }
             // Same reconnection logic as CLOSED (matching Python)
             if (ws->restart_flag && ws->reconnect_attempts < ws->max_reconnect_attempts) {
                 ws->reconnect_thread_active = true;
                 if (pthread_create(&ws->reconnect_thread, NULL, reconnection_thread, ws) != 0) {
                     ws->reconnect_thread_active = false;
                 }
             }
             break;
             
         case LWS_CALLBACK_WSI_CREATE:
         case LWS_CALLBACK_CLIENT_HTTP_REDIRECT:
         case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP:
             break;
             
         case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS:
             // Skip SSL certificate verification
             return 1;
             
         default:
             break;
     }
     
     return 0;
 }
 
 // WebSocket protocols
 // Use empty string as protocol name - server will negotiate
 static struct lws_protocols protocols[] = {
     {
         "",  // Empty protocol name - let server choose (matching Python behavior)
         callback_fyers_data_ws,
         0,
         4096,
     },
     { NULL, NULL, 0, 0 }
 };
 
 // WebSocket service thread
 static void* ws_service_thread(void* arg) {
     fyers_data_ws_t* ws = (fyers_data_ws_t*)arg;
     
     while (ws->thread_running && ws->lws_context) {
         lws_service(ws->lws_context, 50);
     }
     
     return NULL;
 }
 
 // Public API implementation
 fyers_data_ws_t* fyers_data_ws_create(
     const char* access_token,
     bool write_to_file,
     const char* log_path,
                                                 bool litemode,
                                                 bool reconnect,
     fyers_data_ws_on_message_t on_message,
     fyers_data_ws_on_error_t on_error,
     fyers_data_ws_on_connect_t on_connect,
     fyers_data_ws_on_close_t on_close,
     int reconnect_retry) {
     
     if (!access_token) {
         return NULL;
     }
 
     fyers_data_ws_t* ws = (fyers_data_ws_t*)calloc(1, sizeof(fyers_data_ws_t));
     if (!ws) {
         return NULL;
     }
 
     ws->access_token = strdup(access_token);
     ws->write_to_file = write_to_file;
     ws->log_path = log_path ? strdup(log_path) : NULL;
     ws->litemode = litemode;
     ws->should_reconnect = reconnect;
     ws->max_reconnect_attempts = (reconnect_retry < MAX_RECONNECT_ATTEMPTS) ? reconnect_retry : MAX_RECONNECT_ATTEMPTS;
     ws->on_message = on_message;
     ws->on_error = on_error;
     ws->on_connect = on_connect;
     ws->on_close = on_close;
     ws->channel_num = 11;
     ws->data_type = FYERS_DATA_TYPE_SYMBOL_UPDATE;  // Default to SymbolUpdate
     ws->restart_flag = reconnect;
     ws->connected = false;
     ws->thread_running = true;
     ws->reconnect_thread_active = false;
     
     // Initialize mutexes
     pthread_mutex_init(&ws->ws_mutex, NULL);
     pthread_mutex_init(&ws->message_mutex, NULL);
     pthread_cond_init(&ws->message_cond, NULL);
     
     // Initialize field names
     init_field_names(ws);
     
     // Initialize topic mappings
     ws->scrips_sym = NULL;
     ws->index_sym = NULL;
     ws->dp_sym = NULL;
     ws->scrips_sym_count = 0;
     ws->index_sym_count = 0;
     ws->dp_sym_count = 0;
     ws->scrips_sym_capacity = 0;
     ws->index_sym_capacity = 0;
     ws->dp_sym_capacity = 0;
     
     // Initialize response map
     ws->resp_map = NULL;
     ws->resp_map_count = 0;
     ws->resp_map_capacity = 0;
     
     // Initialize channel tracking
     ws->active_channel = 0;
     ws->scrips_per_channel = (int*)calloc(31, sizeof(int));
     ws->update_tick = false;
     
     ws->last_subscription_symbols = NULL;
     ws->last_subscription_count = 0;
     
     // Extract HSM token
     if (extract_hsm_token(access_token, &ws->hsm_token) != 0) {
         if (on_error) {
             on_error(ws, -300, "Invalid token");
         }
         fyers_data_ws_destroy(ws);
         return NULL;
     }
     ws->valid_token = true;
     
     // Create logger
     ws->logger = fyers_logger_create("FyersDataSocket", FYERS_LOG_DEBUG, log_path);
     
     // Create HTTP client for symbol conversion
     fyers_logger_t* api_logger = fyers_logger_create("FyersAPI", FYERS_LOG_INFO, log_path);
     fyers_logger_t* req_logger = fyers_logger_create("FyersAPIRequest", FYERS_LOG_DEBUG, log_path);
     ws->http_client = fyers_http_client_create(api_logger, req_logger);
     
     return ws;
 }
 
 void fyers_data_ws_destroy(fyers_data_ws_t* ws) {
     if (!ws) {
         return;
     }
 
     ws->thread_running = false;
     
     // Close connection
     fyers_data_ws_close(ws);
     
     // Cleanup threads (use flags; pthread_t may be a struct on Windows)
     if (ws->ws_thread_started) {
         pthread_join(ws->ws_thread, NULL);
         ws->ws_thread_started = false;
     }
     if (ws->ping_thread_started) {
         pthread_join(ws->ping_thread, NULL);
         ws->ping_thread_started = false;
     }
     if (ws->message_thread_started) {
         pthread_join(ws->message_thread, NULL);
         ws->message_thread_started = false;
     }
     if (ws->reconnect_thread_active) {
         pthread_join(ws->reconnect_thread, NULL);
         ws->reconnect_thread_active = false;
     }
     
     // Cleanup message queue
     pthread_mutex_lock(&ws->message_mutex);
     struct message_queue* msg = ws->message_queue_head;
     while (msg) {
         struct message_queue* next = msg->next;
         free(msg->data);
         free(msg);
         msg = next;
     }
     pthread_mutex_unlock(&ws->message_mutex);
     
     // Cleanup resources
     free(ws->access_token);
     free(ws->hsm_token);
     free(ws->log_path);
     
     // Cleanup symbol maps
     for (size_t i = 0; i < ws->symbol_token_count; i++) {
         free(ws->symbol_token_map[i].symbol);
         free(ws->symbol_token_map[i].hsm_token);
     }
     free(ws->symbol_token_map);
     
     free_last_subscription(ws);
     
     // Cleanup topic mappings
     for (size_t i = 0; i < ws->scrips_sym_count; i++) {
         free(ws->scrips_sym[i].hsm_token);
     }
     free(ws->scrips_sym);
     for (size_t i = 0; i < ws->index_sym_count; i++) {
         free(ws->index_sym[i].hsm_token);
     }
     free(ws->index_sym);
     for (size_t i = 0; i < ws->dp_sym_count; i++) {
         free(ws->dp_sym[i].hsm_token);
     }
     free(ws->dp_sym);
     
     // Cleanup response map
     for (size_t i = 0; i < ws->resp_map_count; i++) {
         free(ws->resp_map[i].hsm_token);
         cJSON_Delete(ws->resp_map[i].data);
     }
     free(ws->resp_map);
     
     // Cleanup channel tracking
     free(ws->scrips_per_channel);
     
     // Cleanup mutexes
     pthread_mutex_destroy(&ws->ws_mutex);
     pthread_mutex_destroy(&ws->message_mutex);
     pthread_cond_destroy(&ws->message_cond);
     
     // Cleanup logger and HTTP client
     if (ws->logger) {
         fyers_logger_destroy(ws->logger);
     }
     if (ws->http_client) {
         fyers_http_client_destroy(ws->http_client);
     }
     
     // Cleanup LWS context
     if (ws->lws_context) {
         lws_context_destroy(ws->lws_context);
     }
     
     free(ws);
 }
 
 fyers_error_t fyers_data_ws_connect(fyers_data_ws_t* ws) {
     if (!ws || !ws->valid_token) {
         return FYERS_ERROR_INVALID_PARAM;
     }
     
     // Suppress libwebsockets logging (only show errors and warnings)
     lws_set_log_level(LLL_ERR | LLL_WARN, NULL);
     
         struct lws_context_creation_info info;
         memset(&info, 0, sizeof(info));
     
         info.port = CONTEXT_PORT_NO_LISTEN;
         info.protocols = protocols;
         info.gid = -1;
         info.uid = -1;
     // Skip SSL certificate verification for self-signed certificates
         info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
     info.client_ssl_ca_filepath = NULL;  // Don't verify CA
     
     ws->lws_context = lws_create_context(&info);
     if (!ws->lws_context) {
             return FYERS_ERROR;
     }
     
     // Create client connection
     struct lws_client_connect_info ccinfo;
     memset(&ccinfo, 0, sizeof(ccinfo));
     ccinfo.context = ws->lws_context;
     ccinfo.address = "socket.fyers.in";
     ccinfo.port = 443;
     ccinfo.path = "/hsm/v1-5/prod";
     ccinfo.host = "socket.fyers.in";
     ccinfo.origin = "socket.fyers.in";
     // Use empty string protocol - let server negotiate (matching Python behavior)
     ccinfo.protocol = protocols[0].name;  // Empty string ""
     // Use SSL but skip certificate verification (matching Python sslopt={"cert_reqs": ssl.CERT_NONE})
     ccinfo.ssl_connection = LCCSCF_USE_SSL | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
     ccinfo.userdata = ws;
     
     ws->lws_wsi = lws_client_connect_via_info(&ccinfo);
     if (!ws->lws_wsi) {
         lws_context_destroy(ws->lws_context);
         ws->lws_context = NULL;
         return FYERS_ERROR;
     }
     
     // Start service thread
     ws->ws_thread_started = (pthread_create(&ws->ws_thread, NULL, ws_service_thread, ws) == 0);
     ws->ping_thread_started = (pthread_create(&ws->ping_thread, NULL, ping_thread, ws) == 0);
     ws->message_thread_started = (pthread_create(&ws->message_thread, NULL, message_queue_thread, ws) == 0);
     
     // Wait for connection to establish (check connected flag)
     // Python waits 2 seconds, but we'll wait up to 5 seconds for actual connection
     int wait_count = 0;
     while (wait_count < 50 && !ws->connected) {  // 50 * 100ms = 5 seconds
         usleep(100000);  // 100ms
         wait_count++;
     }
     
     if (ws->connected) {
         // Wait additional 2 seconds matching Python flow
         sleep(2);
         
         // Call on_connect callback (matching Python's on_open() call from connect())
         if (ws->on_connect) {
             ws->on_connect(ws);
         }
     } else {
         return FYERS_ERROR_NETWORK;
     }
     
     return FYERS_OK;
 }
 
 void fyers_data_ws_close(fyers_data_ws_t* ws) {
     if (!ws) {
         return;
     }
     
     ws->restart_flag = false;
     ws->should_reconnect = false;
     ws->thread_running = false;
     
     pthread_mutex_lock(&ws->ws_mutex);
     if (ws->lws_wsi && ws->connected) {
         // Gracefully close the WebSocket connection
         // Use lws_set_timeout to trigger close, which is safer than lws_close_reason
         lws_set_timeout(ws->lws_wsi, NO_PENDING_TIMEOUT, LWS_TO_KILL_ASYNC);
         ws->lws_wsi = NULL;
     }
     ws->connected = false;
     pthread_mutex_unlock(&ws->ws_mutex);
     
     pthread_cond_signal(&ws->message_cond);
 }
 
 fyers_error_t fyers_data_ws_subscribe(
     fyers_data_ws_t* ws,
     const char** symbols,
     size_t symbol_count,
     fyers_data_type_t data_type,
     int channel) {
 
     if (!ws || !symbols || symbol_count == 0) {
         return FYERS_ERROR_INVALID_PARAM;
     }
 
     ws->channel_num = channel;
     ws->data_type = data_type;  // Store data type for symbol conversion
     
     // Store for auto resubscribe after reconnect (matching Python: user subscribes in on_connect)
     free_last_subscription(ws);
     ws->last_subscription_symbols = (char**)malloc(symbol_count * sizeof(char*));
     if (ws->last_subscription_symbols) {
         for (size_t i = 0; i < symbol_count; i++) {
             ws->last_subscription_symbols[i] = strdup(symbols[i]);
         }
         ws->last_subscription_count = symbol_count;
         ws->last_subscription_data_type = data_type;
         ws->last_subscription_channel = channel;
     }
     
     // Convert symbols to HSM tokens
     symbol_map_t* hsm_map = NULL;
     size_t hsm_count = 0;
     
     fyers_error_t err = convert_symbols_to_hsm_tokens(ws, symbols, symbol_count, &hsm_map, &hsm_count);
     if (err != FYERS_OK) {
         return err;
     }
     
     // Extract HSM tokens
     const char** hsm_tokens = (const char**)malloc(hsm_count * sizeof(char*));
     if (!hsm_tokens) {
         // Free hsm_map
         for (size_t i = 0; i < hsm_count; i++) {
             free(hsm_map[i].symbol);
             free(hsm_map[i].hsm_token);
         }
         free(hsm_map);
         return FYERS_ERROR_MEMORY;
     }
     
     for (size_t i = 0; i < hsm_count; i++) {
         hsm_tokens[i] = hsm_map[i].hsm_token;
     }
     
     // Create subscription message
     uint8_t sub_msg[65536];
     size_t sub_size = sizeof(sub_msg);
     create_subscription_message(ws, hsm_tokens, hsm_count, sub_msg, &sub_size);
     
     // Send message
     add_message_to_queue(ws, sub_msg, sub_size);
     
     // Store symbol mapping
     ws->symbol_token_map = hsm_map;
     ws->symbol_token_count = hsm_count;
     
     free(hsm_tokens);
     
     return FYERS_OK;
 }
 
 fyers_error_t fyers_data_ws_unsubscribe(
     fyers_data_ws_t* ws,
     const char** symbols,
     size_t symbol_count,
     fyers_data_type_t data_type,
     int channel) {
 
     if (!ws || !symbols || symbol_count == 0) {
         return FYERS_ERROR_INVALID_PARAM;
     }
     if (!ws->valid_token) {
         if (ws->on_error) {
             ws->on_error(ws, FYERS_ERROR_INVALID_PARAM, "Invalid token");
         }
         return FYERS_ERROR_INVALID_PARAM;
     }
 
     ws->channel_num = channel;
     ws->data_type = data_type;
 
     /* Convert symbols to HSM tokens (same as subscribe) */
     symbol_map_t* hsm_map = NULL;
     size_t hsm_count = 0;
 
     fyers_error_t err = convert_symbols_to_hsm_tokens(ws, symbols, symbol_count, &hsm_map, &hsm_count);
     if (err != FYERS_OK) {
         return err;
     }
     if (hsm_count == 0) {
         if (ws->on_error) {
             ws->on_error(ws, -300, "No valid symbols to unsubscribe");
         }
         return FYERS_ERROR_INVALID_PARAM;
     }
 
     /* Send unsubscription in chunks of SUBSCRIPTION_CHUNK_SIZE (1500), matching Python */
     for (size_t i = 0; i < hsm_count; i += SUBSCRIPTION_CHUNK_SIZE) {
         size_t chunk_count = hsm_count - i;
         if (chunk_count > SUBSCRIPTION_CHUNK_SIZE) {
             chunk_count = SUBSCRIPTION_CHUNK_SIZE;
         }
 
         const char** chunk_tokens = (const char**)malloc(chunk_count * sizeof(char*));
         if (!chunk_tokens) {
             for (size_t j = 0; j < hsm_count; j++) {
                 free(hsm_map[j].symbol);
                 free(hsm_map[j].hsm_token);
             }
             free(hsm_map);
             return FYERS_ERROR_MEMORY;
         }
         for (size_t k = 0; k < chunk_count; k++) {
             chunk_tokens[k] = hsm_map[i + k].hsm_token;
         }
 
         uint8_t unsub_msg[65536];
         size_t unsub_size = sizeof(unsub_msg);
         create_unsubscription_message(ws, chunk_tokens, chunk_count, unsub_msg, &unsub_size);
         free(chunk_tokens);
 
         if (unsub_size > 0 && unsub_size <= sizeof(unsub_msg)) {
             add_message_to_queue(ws, unsub_msg, unsub_size);
         }
     }
 
     /* Free the temporary HSM map (we don't keep it for unsub) */
     for (size_t i = 0; i < hsm_count; i++) {
         free(hsm_map[i].symbol);
         free(hsm_map[i].hsm_token);
     }
     free(hsm_map);
 
     return FYERS_OK;
 }
 
 fyers_error_t fyers_data_ws_channel_resume(fyers_data_ws_t* ws, int channel) {
     if (!ws) {
         return FYERS_ERROR_INVALID_PARAM;
     }
     
     ws->channel_num = channel;
     // Send resume message
     return FYERS_OK;
 }
 
 bool fyers_data_ws_is_connected(fyers_data_ws_t* ws) {
     if (!ws) {
         return false;
     }
     
     return ws->connected;
 }
 
 void fyers_data_ws_keep_running(fyers_data_ws_t* ws) {
     if (!ws) {
         return;
     }
 
     /* Block until close/destroy - do NOT exit on disconnect so reconnection can retry up to 50 times */
     while (ws->thread_running) {
         sleep(1);
     }
 }
 
 