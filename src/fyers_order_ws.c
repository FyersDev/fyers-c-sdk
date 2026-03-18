/**
 * @file fyers_order_ws.c
 * @brief Fyers Order WebSocket implementation for real-time order/trade/position updates
 */

 #include "fyers_order_ws.h"
 #include "fyers_logger.h"
 #include "fyers_http_client.h"
 #include <libwebsockets.h>
 #include <stdlib.h>
 #include <string.h>
 #include <stdio.h>
 #include <pthread.h>
 #include "fyers_platform.h"
 #include <cjson/cJSON.h>
 
 // Constants
 #define ORDER_WS_URL "wss://socket.fyers.in/trade/v3"
 #define MAX_RECONNECT_ATTEMPTS 50
 #define PING_INTERVAL_SEC 10
 
 // Forward declarations
 struct fyers_order_ws;
 static void* order_reconnection_thread(void* arg);
 
 // Main WebSocket structure
 struct fyers_order_ws {
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
     
     // Configuration
     bool write_to_file;
     char* log_path;
     bool restart_flag;
     
     // Callbacks
     fyers_order_ws_on_trades_t on_trades;
     fyers_order_ws_on_positions_t on_positions;
     fyers_order_ws_on_orders_t on_orders;
     fyers_order_ws_on_general_t on_general;
     fyers_order_ws_on_error_t on_error;
     fyers_order_ws_on_connect_t on_connect;
     fyers_order_ws_on_close_t on_close;
     
     // Threading
     pthread_t ws_thread;
     pthread_t ping_thread;
     pthread_t reconnect_thread;
     bool ws_thread_started;
     pthread_mutex_t ws_mutex;
     bool thread_running;
     bool ping_thread_started;
     bool reconnect_thread_active;
     
     // Logger
     fyers_logger_t* logger;
 };
 
 // Ping thread
 static void* ping_thread_order(void* arg) {
     fyers_order_ws_t* ws = (fyers_order_ws_t*)arg;
     
     while (ws->thread_running) {
         sleep(PING_INTERVAL_SEC);
         
         if (!ws->thread_running) {
             break;
         }
         
         pthread_mutex_lock(&ws->ws_mutex);
         if (ws->lws_wsi && ws->connected) {
             // Request WRITEABLE callback to send ping (proper libwebsockets way)
             lws_callback_on_writable(ws->lws_wsi);
         }
         pthread_mutex_unlock(&ws->ws_mutex);
     }
     
     return NULL;
 }
 
 // Field name mapper structure
 typedef struct {
     const char* from;  // Server field name (snake_case)
     const char* to;    // User field name (camelCase)
 } field_mapper_t;
 
 // Order mapper (from map.json order_mapper)
 static const field_mapper_t order_mapper[] = {
     {"client_id", "clientId"},
     {"id", "id"},
     {"id_parent", "parentId"},
     {"id_exchange", "exchOrdId"},
     {"qty", "qty"},
     {"qty_remaining", "remainingQuantity"},
     {"qty_filled", "filledQty"},
     {"price_limit", "limitPrice"},
     {"price_stop", "stopPrice"},
     {"price_traded", "tradedPrice"},
     {"ord_type", "type"},
     {"fy_token", "fyToken"},
     {"exchange", "exchange"},
     {"segment", "segment"},
     {"symbol", "symbol"},
     {"instrument", "instrument"},
     {"oms_msg", "message"},
     {"offline_flag", "offlineOrder"},
     {"time_oms", "orderDateTime"},
     {"validity", "orderValidity"},
     {"product_type", "productType"},
     {"tran_side", "side"},
     {"org_ord_status", "status"},
     {"ord_source", "source"},
     {"symbol_exch", "ex_sym"},
     {"symbol_desc", "description"},
     {"ordertag", "orderTag"},
     {NULL, NULL}
 };
 
 // Trade mapper (from map.json trade_mapper)
 static const field_mapper_t trade_mapper[] = {
     {"id_fill", "tradeNumber"},
     {"id", "orderNumber"},
     {"qty_traded", "tradedQty"},
     {"price_traded", "tradePrice"},
     {"traded_val", "tradeValue"},
     {"product_type", "productType"},
     {"client_id", "clientId"},
     {"id_exchange", "exchangeOrderNo"},
     {"ord_type", "orderType"},
     {"tran_side", "side"},
     {"symbol", "symbol"},
     {"fill_time", "orderDateTime"},
     {"fy_token", "fyToken"},
     {"exchange", "exchange"},
     {"segment", "segment"},
     {"ordertag", "orderTag"},
     {NULL, NULL}
 };
 
 // Position mapper (from map.json position_mapper)
 static const field_mapper_t position_mapper[] = {
     {"symbol", "symbol"},
     {"id", "id"},
     {"buy_avg", "buyAvg"},
     {"buy_qty", "buyQty"},
     {"buy_val", "buyVal"},
     {"sell_avg", "sellAvg"},
     {"sell_qty", "sellQty"},
     {"sell_val", "sellVal"},
     {"net_avg", "netAvg"},
     {"net_qty", "netQty"},
     {"tran_side", "side"},
     {"qty", "qty"},
     {"product_type", "productType"},
     {"pl_realized", "realized_profit"},
     {"rbirefrate", "rbiRefRate"},
     {"fy_token", "fyToken"},
     {"exchange", "exchange"},
     {"segment", "segment"},
     {"day_buy_qty", "dayBuyQty"},
     {"day_sell_qty", "daySellQty"},
     {"cf_buy_qty", "cfBuyQty"},
     {"cf_sell_qty", "cfSellQty"},
     {"qty_multiplier", "qtyMulti_com"},
     {"pl_total", "pl"},
     {"cross_curr_flag", "crossCurrency"},
     {"pl_unrealized", "unrealized_profit"},
     {NULL, NULL}
 };
 
 // Transform field names using mapper (matching Python __parse_order_data, __parse_trade_data, __parse_position_data)
 static cJSON* transform_fields(cJSON* source_obj, const field_mapper_t* mapper) {
     if (!source_obj || !mapper) {
         return NULL;
     }
     
     cJSON* result = cJSON_CreateObject();
     if (!result) {
         return NULL;
     }
     
     // Copy "s" field if present
     cJSON* s_field = cJSON_GetObjectItem(source_obj, "s");
     if (s_field) {
         cJSON_AddItemToObject(result, "s", cJSON_Duplicate(s_field, 1));
     }
     
     // Transform fields using mapper
     for (int i = 0; mapper[i].from; i++) {
         cJSON* source_field = cJSON_GetObjectItem(source_obj, mapper[i].from);
         if (source_field) {
             cJSON_AddItemToObject(result, mapper[i].to, cJSON_Duplicate(source_field, 1));
         }
     }
     
     return result;
 }
 
 // Parse order data (matching Python __parse_order_data)
 static cJSON* parse_order_data(fyers_order_ws_t* ws, cJSON* msg) {
     cJSON* orders_obj = cJSON_GetObjectItem(msg, "orders");
     if (!orders_obj) {
         return NULL;
     }
     
     // Transform field names
     cJSON* order_data = transform_fields(orders_obj, order_mapper);
     if (!order_data) {
         return NULL;
     }
     
     // Add orderNumStatus field (matching Python: id + ":" + str(org_ord_status))
     cJSON* id_field = cJSON_GetObjectItem(orders_obj, "id");
     cJSON* status_field = cJSON_GetObjectItem(orders_obj, "org_ord_status");
     if (id_field && status_field && cJSON_IsString(id_field) && cJSON_IsNumber(status_field)) {
         char order_num_status[256];
         snprintf(order_num_status, sizeof(order_num_status), "%s:%d", 
                  cJSON_GetStringValue(id_field), (int)cJSON_GetNumberValue(status_field));
         cJSON_AddStringToObject(order_data, "orderNumStatus", order_num_status);
     }
     
     // Create result with "s" and "orders"
     cJSON* result = cJSON_CreateObject();
     cJSON* s_field = cJSON_GetObjectItem(msg, "s");
     if (s_field) {
         cJSON_AddItemToObject(result, "s", cJSON_Duplicate(s_field, 1));
     }
     cJSON_AddItemToObject(result, "orders", order_data);
     
     return result;
 }
 
 // Parse trade data (matching Python __parse_trade_data)
 static cJSON* parse_trade_data(fyers_order_ws_t* ws, cJSON* msg) {
     cJSON* trades_obj = cJSON_GetObjectItem(msg, "trades");
     if (!trades_obj) {
         return NULL;
     }
     
     // Transform field names
     cJSON* trade_data = transform_fields(trades_obj, trade_mapper);
     if (!trade_data) {
         return NULL;
     }
     
     // Create result with "s" and "trades"
     cJSON* result = cJSON_CreateObject();
     cJSON* s_field = cJSON_GetObjectItem(msg, "s");
     if (s_field) {
         cJSON_AddItemToObject(result, "s", cJSON_Duplicate(s_field, 1));
     }
     cJSON_AddItemToObject(result, "trades", trade_data);
     
     return result;
 }
 
 // Parse position data (matching Python __parse_position_data)
 static cJSON* parse_position_data(fyers_order_ws_t* ws, cJSON* msg) {
     cJSON* positions_obj = cJSON_GetObjectItem(msg, "positions");
     if (!positions_obj) {
         return NULL;
     }
     
     // Transform field names
     cJSON* position_data = transform_fields(positions_obj, position_mapper);
     if (!position_data) {
         return NULL;
     }
     
     // Create result with "s" and "positions"
     cJSON* result = cJSON_CreateObject();
     cJSON* s_field = cJSON_GetObjectItem(msg, "s");
     if (s_field) {
         cJSON_AddItemToObject(result, "s", cJSON_Duplicate(s_field, 1));
     }
     cJSON_AddItemToObject(result, "positions", position_data);
     
     return result;
 }
 
 // Parse and route messages (matching Python __on_message)
 static void handle_order_message(fyers_order_ws_t* ws, const char* message) {
     if (!ws || !message) {
         return;
     }
     
     // Skip "pong" messages
     if (strcmp(message, "pong") == 0) {
         return;
     }
     
     cJSON* json = cJSON_Parse(message);
     if (!json) {
         return;
     }
     
     // Route to appropriate callback with field transformation
     cJSON* parsed_result = NULL;
     char* json_str = NULL;
     
     if (cJSON_GetObjectItem(json, "orders")) {
         parsed_result = parse_order_data(ws, json);
         if (parsed_result && ws->on_orders) {
             json_str = cJSON_PrintUnformatted(parsed_result);
             if (json_str) {
                 ws->on_orders(ws, json_str);
                 free(json_str);
             }
         }
     } else if (cJSON_GetObjectItem(json, "positions")) {
         parsed_result = parse_position_data(ws, json);
         if (parsed_result && ws->on_positions) {
             json_str = cJSON_PrintUnformatted(parsed_result);
             if (json_str) {
                 ws->on_positions(ws, json_str);
                 free(json_str);
             }
         }
     } else if (cJSON_GetObjectItem(json, "trades")) {
         parsed_result = parse_trade_data(ws, json);
         if (parsed_result && ws->on_trades) {
             json_str = cJSON_PrintUnformatted(parsed_result);
             if (json_str) {
                 ws->on_trades(ws, json_str);
                 free(json_str);
             }
         }
     } else {
         // General messages - pass as-is
         if (ws->on_general) {
             json_str = cJSON_PrintUnformatted(json);
             if (json_str) {
                 ws->on_general(ws, json_str);
                 free(json_str);
         }
     }
     }
     
     if (parsed_result) {
         cJSON_Delete(parsed_result);
     }
     cJSON_Delete(json);
 }
 
 // WebSocket protocol callbacks
 static int callback_fyers_order_ws(struct lws* wsi, enum lws_callback_reasons reason,
                                     void* user, void* in, size_t len) {
     fyers_order_ws_t* ws = (fyers_order_ws_t*)user;
     
     if (!ws) {
         return 0;
     }
     
     switch (reason) {
         case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
             // Add authorization header (matching Python header={"authorization": self.__access_token})
             {
                 unsigned char **p = (unsigned char **)in, *end = (*p) + len;
                 // Try using WSI_TOKEN_HTTP_AUTHORIZATION first (more reliable)
                 if (lws_add_http_header_by_token(wsi,
                         WSI_TOKEN_HTTP_AUTHORIZATION,
                         (const unsigned char *)ws->access_token,
                         (int)strlen(ws->access_token), p, end)) {
                     // Fallback to by_name if token approach fails
                     if (lws_add_http_header_by_name(wsi,
                             (const unsigned char *)"authorization",
                             (const unsigned char *)ws->access_token,
                             (int)strlen(ws->access_token), p, end)) {
                         return -1;
                     }
                 }
             }
             break;
             
         case LWS_CALLBACK_CLIENT_ESTABLISHED:
             ws->connected = true;
             ws->lws_wsi = wsi;
             ws->reconnect_attempts = 0;
             ws->reconnect_delay = 0;
             
             // Start ping thread only on first connection (reused on reconnect)
             if (!ws->ping_thread_started) {
                 pthread_create(&ws->ping_thread, NULL, ping_thread_order, ws);
                 ws->ping_thread_started = true;
             }
             break;
             
         case LWS_CALLBACK_CLIENT_WRITEABLE:
             // Send ping message when writeable
             pthread_mutex_lock(&ws->ws_mutex);
             if (ws->lws_wsi && ws->connected) {
                 // Prepare ping message with LWS_PRE padding
                 size_t ping_size = 4;  // "ping"
                 size_t total_size = LWS_PRE + ping_size;
                 uint8_t* send_buf = (uint8_t*)malloc(total_size);
                 if (send_buf) {
                     memcpy(send_buf + LWS_PRE, "ping", ping_size);
                     lws_write(ws->lws_wsi, send_buf + LWS_PRE, ping_size, LWS_WRITE_TEXT);
                     free(send_buf);
                 }
             }
             pthread_mutex_unlock(&ws->ws_mutex);
             break;
             
         case LWS_CALLBACK_CLIENT_RECEIVE:
             if (in && len > 0) {
                 char* message = (char*)malloc(len + 1);
                 if (message) {
                     memcpy(message, in, len);
                     message[len] = '\0';
                     handle_order_message(ws, message);
                     free(message);
                 }
             }
             break;
             
         case LWS_CALLBACK_CLIENT_CLOSED:
             ws->connected = false;
             ws->lws_wsi = NULL;
             
             if (ws->on_error) {
                 ws->on_error(ws, "Connection to remote host was lost.");
             }
             
             if (ws->restart_flag && ws->reconnect_attempts < ws->max_reconnect_attempts) {
                 ws->reconnect_thread_active = true;
                 if (pthread_create(&ws->reconnect_thread, NULL, order_reconnection_thread, ws) != 0) {
                     ws->reconnect_thread_active = false;
                     if (ws->on_close) {
                         ws->on_close(ws, "Connection closed");
                     }
                 }
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
                 ws->on_error(ws, "Connection error");
             }
             if (ws->restart_flag && ws->reconnect_attempts < ws->max_reconnect_attempts) {
                 ws->reconnect_thread_active = true;
                 if (pthread_create(&ws->reconnect_thread, NULL, order_reconnection_thread, ws) != 0) {
                     ws->reconnect_thread_active = false;
                 }
             }
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
 // Use empty string as protocol name - server will negotiate (matching Python behavior)
 static struct lws_protocols order_protocols[] = {
     {
         "",  // Empty protocol name - let server choose (matching Python behavior)
         callback_fyers_order_ws,
         0,
         4096,
     },
     { NULL, NULL, 0, 0 }
 };
 
 // Create new client connection for order WS (reuses existing lws_context)
 static struct lws* create_order_client_connection(fyers_order_ws_t* ws) {
     if (!ws || !ws->lws_context) return NULL;
     
     struct lws_client_connect_info ccinfo;
     memset(&ccinfo, 0, sizeof(ccinfo));
     ccinfo.context = ws->lws_context;
     ccinfo.address = "socket.fyers.in";
     ccinfo.port = 443;
     ccinfo.path = "/trade/v3";
     ccinfo.host = "socket.fyers.in";
     ccinfo.origin = "socket.fyers.in";
     ccinfo.protocol = "";  // Empty protocol - let server negotiate (matching Python)
     ccinfo.ssl_connection = LCCSCF_USE_SSL | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
     ccinfo.userdata = ws;
     
     return lws_client_connect_via_info(&ccinfo);
 }
 
 // Reconnection thread - matches Python __on_close reconnect logic
 static void* order_reconnection_thread(void* arg) {
     fyers_order_ws_t* ws = (fyers_order_ws_t*)arg;
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
     
     if (!ws->thread_running || !ws->restart_flag) {
         ws->reconnect_thread_active = false;
         return NULL;
     }
     
     // Python: reconnect_attempts += 1
     ws->reconnect_attempts++;
     
     // Python: self.connect()
     struct lws* new_wsi = create_order_client_connection(ws);
     if (!new_wsi) {
         if (ws->on_error) {
             ws->on_error(ws, "Reconnection failed");
         }
         ws->reconnect_thread_active = false;
         return NULL;
     }
     
     // Wait for connection (up to 30 seconds)
     int wait_count = 0;
     while (ws->thread_running && wait_count < 300) {
         usleep(100000);
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
     
     ws->reconnect_attempts = 0;
     ws->reconnect_delay = 0;
     
     if (ws->on_connect) {
         ws->on_connect(ws);
     }
     
     ws->reconnect_thread_active = false;
     return NULL;
 }
 
 // WebSocket service thread
 static void* ws_service_thread_order(void* arg) {
     fyers_order_ws_t* ws = (fyers_order_ws_t*)arg;
     
     while (ws->thread_running && ws->lws_context) {
         lws_service(ws->lws_context, 50);
     }
     
     return NULL;
 }
 
 // Public API implementation
 fyers_order_ws_t* fyers_order_ws_create(
     const char* access_token,
     bool write_to_file,
     const char* log_path,
     fyers_order_ws_on_trades_t on_trades,
     fyers_order_ws_on_positions_t on_positions,
     fyers_order_ws_on_orders_t on_orders,
     fyers_order_ws_on_general_t on_general,
     fyers_order_ws_on_error_t on_error,
     fyers_order_ws_on_connect_t on_connect,
     fyers_order_ws_on_close_t on_close,
     bool reconnect,
     int reconnect_retry) {
     
     if (!access_token) {
         return NULL;
     }
     
     fyers_order_ws_t* ws = (fyers_order_ws_t*)calloc(1, sizeof(fyers_order_ws_t));
     if (!ws) {
         return NULL;
     }
     
     ws->access_token = strdup(access_token);
     ws->write_to_file = write_to_file;
     ws->log_path = log_path ? strdup(log_path) : NULL;
     ws->should_reconnect = reconnect;
     ws->max_reconnect_attempts = (reconnect_retry < MAX_RECONNECT_ATTEMPTS) ? reconnect_retry : MAX_RECONNECT_ATTEMPTS;
     ws->on_trades = on_trades;
     ws->on_positions = on_positions;
     ws->on_orders = on_orders;
     ws->on_general = on_general;
     ws->on_error = on_error;
     ws->on_connect = on_connect;
     ws->on_close = on_close;
     ws->restart_flag = reconnect;
     ws->connected = false;
     ws->thread_running = true;
     ws->ping_thread_started = false;
     ws->reconnect_thread_active = false;
     
     // Initialize mutex
     pthread_mutex_init(&ws->ws_mutex, NULL);
     
     // Create logger
     ws->logger = fyers_logger_create("FyersOrderSocket", FYERS_LOG_DEBUG, log_path);
     
     return ws;
 }
 
 void fyers_order_ws_destroy(fyers_order_ws_t* ws) {
     if (!ws) {
         return;
     }
     
     ws->thread_running = false;
     
     // Close connection
     fyers_order_ws_close(ws);
     
     // Cleanup threads (use flags; pthread_t may be a struct on Windows)
     if (ws->ws_thread_started) {
         pthread_join(ws->ws_thread, NULL);
         ws->ws_thread_started = false;
     }
     // Ping thread is only started after connection established
     if (ws->ping_thread_started) {
         pthread_join(ws->ping_thread, NULL);
     }
     if (ws->reconnect_thread_active) {
         pthread_join(ws->reconnect_thread, NULL);
         ws->reconnect_thread_active = false;
     }
     
     // Cleanup resources
     free(ws->access_token);
     free(ws->log_path);
     
     // Cleanup mutex
     pthread_mutex_destroy(&ws->ws_mutex);
     
     // Cleanup logger
     if (ws->logger) {
         fyers_logger_destroy(ws->logger);
     }
     
     // Cleanup LWS context
     if (ws->lws_context) {
         lws_context_destroy(ws->lws_context);
     }
     
     free(ws);
 }
 
 fyers_error_t fyers_order_ws_connect(fyers_order_ws_t* ws) {
     if (!ws) {
         return FYERS_ERROR_INVALID_PARAM;
     }
     
     // Suppress libwebsockets logging (only show errors and warnings)
     lws_set_log_level(LLL_ERR | LLL_WARN, NULL);
     
     struct lws_context_creation_info info;
     memset(&info, 0, sizeof(info));
     
     info.port = CONTEXT_PORT_NO_LISTEN;
     info.protocols = order_protocols;
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
     ccinfo.path = "/trade/v3";
     ccinfo.host = "socket.fyers.in";
     ccinfo.origin = "socket.fyers.in";
     ccinfo.protocol = order_protocols[0].name;
     // Use SSL but skip certificate verification (matching Python sslopt={"cert_reqs": ssl.CERT_NONE})
     ccinfo.ssl_connection = LCCSCF_USE_SSL | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
     // Authorization header will be added in LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER callback
     ccinfo.userdata = ws;
     
     ws->lws_wsi = lws_client_connect_via_info(&ccinfo);
     if (!ws->lws_wsi) {
         lws_context_destroy(ws->lws_context);
         ws->lws_context = NULL;
         return FYERS_ERROR;
     }
     
     // Start service thread
     // Ping thread will be started in LWS_CALLBACK_CLIENT_ESTABLISHED (matching Python flow)
     ws->ws_thread_started = (pthread_create(&ws->ws_thread, NULL, ws_service_thread_order, ws) == 0);
     
     // Wait for connection to establish (check connected flag)
     // Python waits 2 seconds, but we'll wait up to 5 seconds for actual connection
     int wait_count = 0;
     while (wait_count < 50 && !ws->connected) {  // 50 * 100ms = 5 seconds
         usleep(100000);  // 100ms
         wait_count++;
     }
     
     if (ws->connected) {
         // Wait additional 2 seconds matching Python flow (connect() waits 2 seconds before calling on_open())
         sleep(2);
         
         // Call on_connect callback (matching Python's on_open() call from connect())
         if (ws->on_connect) {
             ws->on_connect(ws);
         }
     
     return FYERS_OK;
     } else {
         return FYERS_ERROR_NETWORK;
     }
 }
 
 void fyers_order_ws_close(fyers_order_ws_t* ws) {
     if (!ws) {
         return;
     }
     
     ws->restart_flag = false;
     ws->should_reconnect = false;
     ws->thread_running = false;
     
     pthread_mutex_lock(&ws->ws_mutex);
     if (ws->lws_wsi && ws->connected) {
         // Use lws_set_timeout to trigger close, which is safer than lws_close_reason
         lws_set_timeout(ws->lws_wsi, NO_PENDING_TIMEOUT, LWS_TO_KILL_ASYNC);
         ws->lws_wsi = NULL;
     }
     ws->connected = false;
     pthread_mutex_unlock(&ws->ws_mutex);
 }
 
 fyers_error_t fyers_order_ws_subscribe(fyers_order_ws_t* ws, const char* data_types) {
     if (!ws || !data_types || !ws->connected) {
         return FYERS_ERROR_INVALID_PARAM;
     }
     
     // Parse data types and convert to socket types
     // "OnOrders" -> "orders", "OnTrades" -> "trades", etc.
     cJSON* json = cJSON_CreateObject();
     cJSON* slist = cJSON_CreateArray();
     
     char* types_copy = strdup(data_types);
     char* saveptr = NULL;
     char* token = strtok_r(types_copy, ",", &saveptr);
     
     while (token) {
         // Trim whitespace
         while (*token == ' ') token++;
         char* end = token + strlen(token) - 1;
         while (end > token && *end == ' ') *end-- = '\0';
         
         // Map to socket type
         if (strcmp(token, "OnOrders") == 0) {
             cJSON_AddItemToArray(slist, cJSON_CreateString("orders"));
         } else if (strcmp(token, "OnTrades") == 0) {
             cJSON_AddItemToArray(slist, cJSON_CreateString("trades"));
         } else if (strcmp(token, "OnPositions") == 0) {
             cJSON_AddItemToArray(slist, cJSON_CreateString("positions"));
         } else if (strcmp(token, "OnGeneral") == 0) {
             cJSON_AddItemToArray(slist, cJSON_CreateString("edis"));
             cJSON_AddItemToArray(slist, cJSON_CreateString("pricealerts"));
             cJSON_AddItemToArray(slist, cJSON_CreateString("login"));
         }
         
         token = strtok_r(NULL, ",", &saveptr);
     }
     
     free(types_copy);
     
     cJSON_AddItemToObject(json, "T", cJSON_CreateString("SUB_ORD"));
     cJSON_AddItemToObject(json, "SLIST", slist);
     cJSON_AddItemToObject(json, "SUB_T", cJSON_CreateNumber(1));
     
     char* json_str = cJSON_Print(json);
     cJSON_Delete(json);
     
     if (!json_str) {
         return FYERS_ERROR_MEMORY;
     }
     
     // Send message using proper libwebsockets mechanism (with LWS_PRE padding)
     pthread_mutex_lock(&ws->ws_mutex);
     if (ws->lws_wsi && ws->connected) {
         size_t msg_len = strlen(json_str);
         size_t total_size = LWS_PRE + msg_len;
         uint8_t* send_buf = (uint8_t*)malloc(total_size);
         if (send_buf) {
             memcpy(send_buf + LWS_PRE, json_str, msg_len);
             lws_write(ws->lws_wsi, send_buf + LWS_PRE, msg_len, LWS_WRITE_TEXT);
             free(send_buf);
         }
     }
     pthread_mutex_unlock(&ws->ws_mutex);
     
     free(json_str);
     
     return FYERS_OK;
 }
 
 fyers_error_t fyers_order_ws_unsubscribe(fyers_order_ws_t* ws, const char* data_types) {
     if (!ws || !data_types || !ws->connected) {
         return FYERS_ERROR_INVALID_PARAM;
     }
     
     // Similar to subscribe but with SUB_T = -1
     cJSON* json = cJSON_CreateObject();
     cJSON* slist = cJSON_CreateArray();
     
     char* types_copy = strdup(data_types);
     char* saveptr = NULL;
     char* token = strtok_r(types_copy, ",", &saveptr);
     
     while (token) {
         while (*token == ' ') token++;
         char* end = token + strlen(token) - 1;
         while (end > token && *end == ' ') *end-- = '\0';
         
         if (strcmp(token, "OnOrders") == 0) {
             cJSON_AddItemToArray(slist, cJSON_CreateString("orders"));
         } else if (strcmp(token, "OnTrades") == 0) {
             cJSON_AddItemToArray(slist, cJSON_CreateString("trades"));
         } else if (strcmp(token, "OnPositions") == 0) {
             cJSON_AddItemToArray(slist, cJSON_CreateString("positions"));
         } else if (strcmp(token, "OnGeneral") == 0) {
             cJSON_AddItemToArray(slist, cJSON_CreateString("edis"));
             cJSON_AddItemToArray(slist, cJSON_CreateString("pricealerts"));
             cJSON_AddItemToArray(slist, cJSON_CreateString("login"));
         }
         
         token = strtok_r(NULL, ",", &saveptr);
     }
     
     free(types_copy);
     
     cJSON_AddItemToObject(json, "T", cJSON_CreateString("SUB_ORD"));
     cJSON_AddItemToObject(json, "SLIST", slist);
     cJSON_AddItemToObject(json, "SUB_T", cJSON_CreateNumber(-1));
     
     char* json_str = cJSON_Print(json);
     cJSON_Delete(json);
     
     if (!json_str) {
         return FYERS_ERROR_MEMORY;
     }
     
     // Send message using proper libwebsockets mechanism (with LWS_PRE padding)
     pthread_mutex_lock(&ws->ws_mutex);
     if (ws->lws_wsi && ws->connected) {
         size_t msg_len = strlen(json_str);
         size_t total_size = LWS_PRE + msg_len;
         uint8_t* send_buf = (uint8_t*)malloc(total_size);
         if (send_buf) {
             memcpy(send_buf + LWS_PRE, json_str, msg_len);
             lws_write(ws->lws_wsi, send_buf + LWS_PRE, msg_len, LWS_WRITE_TEXT);
             free(send_buf);
         }
     }
     pthread_mutex_unlock(&ws->ws_mutex);
     
     free(json_str);
     
     return FYERS_OK;
 }
 
 bool fyers_order_ws_is_connected(fyers_order_ws_t* ws) {
     if (!ws) {
         return false;
     }
     
     return ws->connected;
 }
 
 void fyers_order_ws_keep_running(fyers_order_ws_t* ws) {
     if (!ws) {
         return;
     }
     
     /* Block until close/destroy - do NOT exit on disconnect so reconnection can retry up to 50 times */
     while (ws->thread_running) {
         sleep(1);
     }
 }
 
 