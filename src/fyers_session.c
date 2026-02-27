/**
 * @file fyers_session.c
 * @brief Session management and OAuth authentication implementation
 */

#include "fyers_session.h"
#include "fyers_config.h"
#include "fyers_http_client.h"
#include "fyers_logger.h"
#include "fyers_model.h"
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <cjson/cJSON.h>
#include <curl/curl.h>

struct fyers_session {
    char* client_id;
    char* redirect_uri;
    char* response_type;
    char* state;
    char* secret_key;
    char* grant_type;
    char* auth_code;
    char* access_token;
    fyers_http_client_t* http_client;
    fyers_logger_t* logger;
};

fyers_session_t* fyers_session_create(const char* client_id,
                                       const char* redirect_uri,
                                       const char* secret_key) {
    if (!client_id || !redirect_uri || !secret_key) {
        return NULL;
    }

    fyers_session_t* session = (fyers_session_t*)malloc(sizeof(fyers_session_t));
    if (!session) {
        return NULL;
    }

    // Use default values for common OAuth parameters
    const char* response_type = "code";
    const char* state = "sample_state";
    const char* grant_type = "authorization_code";

    session->client_id = strdup(client_id);
    session->redirect_uri = strdup(redirect_uri);
    session->response_type = strdup(response_type);
    session->state = strdup(state);
    session->secret_key = strdup(secret_key);
    session->grant_type = strdup(grant_type);
    session->auth_code = NULL;
    session->access_token = NULL;

    session->logger = fyers_logger_create("FyersSession", FYERS_LOG_INFO, NULL);
    session->http_client = fyers_http_client_create(session->logger, session->logger);

    return session;
}

void fyers_session_destroy(fyers_session_t* session) {
    if (!session) {
        return;
    }

    free(session->client_id);
    free(session->redirect_uri);
    free(session->response_type);
    free(session->state);
    free(session->secret_key);
    free(session->grant_type);
    if (session->auth_code) {
        free(session->auth_code);
    }
    if (session->access_token) {
        free(session->access_token);
    }

    if (session->http_client) {
        fyers_http_client_destroy(session->http_client);
    }
    if (session->logger) {
        fyers_logger_destroy(session->logger);
    }

    free(session);
}

fyers_error_t generate_authcode(fyers_session_t* session
                                              ) {
    char url_buffer[2048];
    size_t buffer_size = sizeof(url_buffer);
    
    if (!session) {
        printf("session is null\n");
        return FYERS_ERROR_INVALID_PARAM;
    }
    
    CURL* curl = curl_easy_init();
    if (!curl) {
        printf("curl is null\n");
        return FYERS_ERROR_MEMORY;
    }
    
    char* encoded_client_id = curl_easy_escape(curl, session->client_id, 0);
    char* encoded_redirect_uri = curl_easy_escape(curl, session->redirect_uri, 0);
    char* encoded_response_type = curl_easy_escape(curl, session->response_type, 0);
    char* encoded_state = curl_easy_escape(curl, session->state, 0);

    if (!encoded_client_id || !encoded_redirect_uri || !encoded_response_type || !encoded_state) {
        curl_free(encoded_client_id);
        curl_free(encoded_redirect_uri);
        curl_free(encoded_response_type);
        curl_free(encoded_state);
        curl_easy_cleanup(curl);
        printf("some encoded string is/are null\n");
        return FYERS_ERROR_MEMORY;
    }

    snprintf(url_buffer, buffer_size, "%s%s?client_id=%s&redirect_uri=%s&response_type=%s&state=%s",
             FYERS_API_BASE_URL,
             FYERS_ENDPOINT_AUTH,
             encoded_client_id,
             encoded_redirect_uri,
             encoded_response_type,
             encoded_state);

    printf("%s\n", url_buffer);

    curl_free(encoded_client_id);
    curl_free(encoded_redirect_uri);
    curl_free(encoded_response_type);
    curl_free(encoded_state);
    curl_easy_cleanup(curl);

    return FYERS_OK;
}

fyers_error_t fyers_session_set_authcode(fyers_session_t* session,
                                         const char* auth_code) {
    if (!session || !auth_code) {
        return FYERS_ERROR_INVALID_PARAM;
    }

    if (session->auth_code) {
        free(session->auth_code);
    }

    // URL decode the auth code if needed (auth codes from redirect URLs may be URL-encoded)
    CURL* curl = curl_easy_init();
    if (!curl) {
        return FYERS_ERROR;
    }

    // Try to unescape (URL decode) the auth code
    int decoded_len = 0;
    char* decoded = curl_easy_unescape(curl, auth_code, 0, &decoded_len);
    
    if (decoded && decoded_len > 0) {
        session->auth_code = (char*)malloc(decoded_len + 1);
        if (session->auth_code) {
            memcpy(session->auth_code, decoded, decoded_len);
            session->auth_code[decoded_len] = '\0';
        }
        curl_free(decoded);
    } else {
        // If unescape fails or returns same, just copy the original
        session->auth_code = strdup(auth_code);
    }
    
    curl_easy_cleanup(curl);
    
    if (!session->auth_code) {
        return FYERS_ERROR_MEMORY;
    }
    
    // Trim whitespace
    char* start = session->auth_code;
    char* end = start + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end-- = '\0';
    }
    while (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r') {
        start++;
    }
    if (start != session->auth_code) {
        memmove(session->auth_code, start, strlen(start) + 1);
    }
    
    if (session->logger) {
        fyers_logger_debug(session->logger, "Auth code set (length: %zu): %s", 
                          strlen(session->auth_code), session->auth_code);
    }
    
    return FYERS_OK;
}

fyers_error_t fyers_session_set_access_token(fyers_session_t* session,
                                             const char* access_token) {
    if (!session || !access_token) {
        return FYERS_ERROR_INVALID_PARAM;
    }

    if (session->access_token) {
        free(session->access_token);
    }

    session->access_token = strdup(access_token);
    if (!session->access_token) {
        return FYERS_ERROR_MEMORY;
    }
    
    // Trim whitespace
    char* start = session->access_token;
    char* end = start + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end-- = '\0';
    }
    while (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r') {
        start++;
    }
    if (start != session->access_token) {
        memmove(session->access_token, start, strlen(start) + 1);
    }
    
    if (session->logger) {
        fyers_logger_debug(session->logger, "Access token set (length: %zu)", 
                          strlen(session->access_token));
    }
    
    return FYERS_OK;
}

static void compute_sha256(const char* input, unsigned char* output) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, input, strlen(input));
    unsigned int len = 0;
    EVP_DigestFinal_ex(ctx, output, &len);
    EVP_MD_CTX_free(ctx);
}

static void bytes_to_hex(const unsigned char* bytes, size_t len, char* hex) {
    for (size_t i = 0; i < len; i++) {
        sprintf(hex + (i * 2), "%02x", bytes[i]);
    }
    hex[len * 2] = '\0';
}

fyers_response_t* generate_token(fyers_session_t* session) {
    if (!session || !session->auth_code) {
        return NULL;
    }

    char hash_input[512];
    snprintf(hash_input, sizeof(hash_input), "%s:%s", session->client_id, session->secret_key);

    unsigned char hash[32];
    compute_sha256(hash_input, hash);

    char hash_hex[65];
    bytes_to_hex(hash, 32, hash_hex);

    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "grant_type", session->grant_type);
    cJSON_AddStringToObject(json, "appIdHash", hash_hex);
    cJSON_AddStringToObject(json, "code", session->auth_code);

    char* json_string = cJSON_Print(json);
    cJSON_Delete(json);

    if (!json_string) {
        return NULL;
    }

    if (session->logger) {
        fyers_logger_debug(session->logger, "Request URL: %s%s", FYERS_API_BASE_URL, FYERS_ENDPOINT_VALIDATE_AUTHCODE);
        fyers_logger_debug(session->logger, "grant_type: %s", session->grant_type);
        fyers_logger_debug(session->logger, "Auth code: %s", session->auth_code);
        fyers_logger_debug(session->logger, "appIdHash: %s", hash_hex);
    }

    char url[512];
    snprintf(url, sizeof(url), "%s%s", FYERS_API_BASE_URL, FYERS_ENDPOINT_VALIDATE_AUTHCODE);

    fyers_response_t* response = fyers_http_client_post(session->http_client, url, "", json_string);
    free(json_string);

    if (!response) {
        if (session->logger) {
            fyers_logger_error(session->logger, "Failed to get response from server");
        }
        return NULL;
    }

    return response;
}

fyers_response_t* generate_token_with_refresh_token(fyers_session_t* session, const char* refresh_token, const char* pin) {
    if (!session || !refresh_token || !pin) {
        return NULL;
    }

    char hash_input[512];
    snprintf(hash_input, sizeof(hash_input), "%s:%s", session->client_id, session->secret_key);

    unsigned char hash[32];
    compute_sha256(hash_input, hash);

    char hash_hex[65];
    bytes_to_hex(hash, 32, hash_hex);

    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "grant_type", "refresh_token");
    cJSON_AddStringToObject(json, "appIdHash", hash_hex);
    cJSON_AddStringToObject(json, "refresh_token", refresh_token);
    cJSON_AddStringToObject(json, "pin", pin);

    char* json_string = cJSON_Print(json);
    cJSON_Delete(json);

    if (!json_string) {
        return NULL;
    }

    if (session->logger) {
        fyers_logger_debug(session->logger, "Request URL: %s%s", FYERS_API_BASE_URL, FYERS_ENDPOINT_VALIDATE_AUTHCODE);
        fyers_logger_debug(session->logger, "grant_type: refresh_token");
        fyers_logger_debug(session->logger, "appIdHash: %s", hash_hex);
    }

    char url[512];
    snprintf(url, sizeof(url), "%s%s", FYERS_API_BASE_URL, FYERS_ENDPOINT_VALIDATE_REFRESH_TOKEN);

    fyers_response_t* response = fyers_http_client_post(session->http_client, url, "", json_string);
    free(json_string);

    if (!response) {
        if (session->logger) {
            fyers_logger_error(session->logger, "Failed to get response from server");
        }
        return NULL;
    }

    return response;
}

const char* fyers_session_get_client_id(fyers_session_t* session) {
    if (!session) {
        return NULL;
    }
    return session->client_id;
}

const char* fyers_session_get_access_token(fyers_session_t* session) {
    if (!session) {
        return NULL;
    }
    return session->access_token;
}

