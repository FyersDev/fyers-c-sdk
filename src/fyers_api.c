/**
 * @file fyers_api.c
 * @brief Main API initialization and cleanup
 */

#include "fyers_api.h"
#include <curl/curl.h>
#include <stdlib.h>

static fyers_log_callback_t g_log_callback = NULL;
static fyers_log_level_t g_log_level = FYERS_LOG_ERROR;
static int g_initialized = 0;

// Internal auto-initialization function
// curl_global_init is thread-safe and idempotent, so this is safe
static void fyers_auto_init(void) {
    if (g_initialized == 0) {
        CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
        if (res == CURLE_OK) {
            g_initialized = 1;
        }
    }
}

// Export auto-init for use in other modules
void fyers_ensure_init(void) {
    fyers_auto_init();
}

fyers_error_t fyers_init(void) {
    CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (res == CURLE_OK) {
        g_initialized = 1;
        return FYERS_OK;
    }
    return FYERS_ERROR;
}

void fyers_cleanup(void) {
    if (g_initialized == 1) {
        curl_global_cleanup();
        g_initialized = 0;
    }
}

void fyers_set_log_callback(fyers_log_callback_t callback) {
    g_log_callback = callback;
}

void fyers_set_log_level(fyers_log_level_t level) {
    g_log_level = level;
}

