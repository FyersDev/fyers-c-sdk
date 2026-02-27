/**
 * @file fyers_api.h
 * @brief Main header file for Fyers API C SDK
 * @version 3.1.8
 */

#ifndef FYERS_API_H
#define FYERS_API_H

#include "fyers_export.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Forward declarations
typedef struct fyers_session fyers_session_t;
typedef struct fyers_model fyers_model_t;

// Error codes
typedef enum {
    FYERS_OK = 0,
    FYERS_ERROR = -1,
    FYERS_ERROR_INVALID_PARAM = -2,
    FYERS_ERROR_MEMORY = -3,
    FYERS_ERROR_NETWORK = -4,
    FYERS_ERROR_AUTH = -5,
    FYERS_ERROR_TOKEN_EXPIRED = -99,
    FYERS_ERROR_INVALID_TOKEN = -300
} fyers_error_t;

// Log levels
typedef enum {
    FYERS_LOG_ERROR = 0,
    FYERS_LOG_WARNING = 1,
    FYERS_LOG_INFO = 2,
    FYERS_LOG_DEBUG = 3
} fyers_log_level_t;

// Order types
typedef enum {
    FYERS_ORDER_TYPE_LIMIT = 1,
    FYERS_ORDER_TYPE_MARKET = 2,
    FYERS_ORDER_TYPE_STOP_LOSS = 3,
    FYERS_ORDER_TYPE_STOP_LIMIT = 4
} fyers_order_type_t;

// Order side
typedef enum {
    FYERS_SIDE_BUY = 1,
    FYERS_SIDE_SELL = -1
} fyers_side_t;

// Product types
typedef enum {
    FYERS_PRODUCT_CNC,
    FYERS_PRODUCT_INTRADAY,
    FYERS_PRODUCT_MARGIN,
    FYERS_PRODUCT_CO,
    FYERS_PRODUCT_BO,
    FYERS_PRODUCT_MTF
} fyers_product_type_t;

// Validity types
typedef enum {
    FYERS_VALIDITY_DAY,
    FYERS_VALIDITY_IOC
} fyers_validity_t;

// Callback function types
typedef void (*fyers_log_callback_t)(fyers_log_level_t level, const char* message);
typedef void (*fyers_error_callback_t)(int code, const char* message);
typedef void (*fyers_message_callback_t)(const char* message);
typedef void (*fyers_connect_callback_t)(void);
typedef void (*fyers_close_callback_t)(const char* reason);

// Response structure
typedef struct {
    char* data;
    size_t size;
    int status_code;
    fyers_error_t error;
} fyers_response_t;

// Initialize and cleanup
FYERS_API fyers_error_t fyers_init(void);
FYERS_API void fyers_cleanup(void);
FYERS_API void fyers_ensure_init(void);  /* Auto-initialization (called internally) */

// Set global log callback
FYERS_API void fyers_set_log_callback(fyers_log_callback_t callback);

// Set log level
FYERS_API void fyers_set_log_level(fyers_log_level_t level);

// WebSocket APIs (forward declarations)
// Include full headers for complete API
#include "fyers_data_ws.h"
#include "fyers_order_ws.h"

#ifdef __cplusplus
}
#endif

#endif // FYERS_API_H

