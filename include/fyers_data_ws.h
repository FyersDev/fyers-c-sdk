/**
 * @file fyers_data_ws.h
 * @brief Fyers Data WebSocket API for real-time market data
 * @version 3.1.8
 */

#ifndef FYERS_DATA_WS_H
#define FYERS_DATA_WS_H

#include "fyers_api.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration
typedef struct fyers_data_ws fyers_data_ws_t;

// Data types for subscription
typedef enum {
    FYERS_DATA_TYPE_SYMBOL_UPDATE = 0,
    FYERS_DATA_TYPE_DEPTH_UPDATE = 1
} fyers_data_type_t;

// Callback function types
typedef void (*fyers_data_ws_on_message_t)(fyers_data_ws_t* ws, const char* json_message);
typedef void (*fyers_data_ws_on_error_t)(fyers_data_ws_t* ws, const char* json_error);
typedef void (*fyers_data_ws_on_connect_t)(fyers_data_ws_t* ws);
typedef void (*fyers_data_ws_on_close_t)(fyers_data_ws_t* ws, const char* reason);

/**
 * @brief Create a new Fyers Data WebSocket instance
 * @param access_token Access token (format: "client_id:token" or just "token")
 * @param write_to_file Write logs to file (true) or stdout (false)
 * @param log_path Path for log files (NULL for current directory)
 * @param litemode Enable lite mode (only LTP updates)
 * @param reconnect Enable automatic reconnection
 * @param on_message Callback for received messages
 * @param on_error Callback for errors
 * @param on_connect Callback when connected
 * @param on_close Callback when connection closes
 * @param reconnect_retry Maximum number of reconnection attempts
 * @return WebSocket instance or NULL on error
 */
FYERS_API fyers_data_ws_t* fyers_data_ws_create(
    const char* access_token,
    bool write_to_file,
    const char* log_path,
    bool litemode,
    bool reconnect,
    fyers_data_ws_on_message_t on_message,
    fyers_data_ws_on_error_t on_error,
    fyers_data_ws_on_connect_t on_connect,
    fyers_data_ws_on_close_t on_close,
    int reconnect_retry
);

/**
 * @brief Destroy WebSocket instance
 * @param ws WebSocket instance
 */
FYERS_API void fyers_data_ws_destroy(fyers_data_ws_t* ws);

/**
 * @brief Connect to the WebSocket server
 * @param ws WebSocket instance
 * @return FYERS_OK on success, error code otherwise
 */
FYERS_API fyers_error_t fyers_data_ws_connect(fyers_data_ws_t* ws);

/**
 * @brief Close the WebSocket connection
 * @param ws WebSocket instance
 */
FYERS_API void fyers_data_ws_close(fyers_data_ws_t* ws);

/**
 * @brief Subscribe to symbols for real-time updates
 * @param ws WebSocket instance
 * @param symbols Array of symbol strings (e.g., ["NSE:SBIN-EQ", "NSE:RELIANCE-EQ"])
 * @param symbol_count Number of symbols in the array
 * @param data_type Type of data to subscribe to (SymbolUpdate or DepthUpdate)
 * @param channel Channel number (default: 11)
 * @return FYERS_OK on success, error code otherwise
 */
FYERS_API fyers_error_t fyers_data_ws_subscribe(
    fyers_data_ws_t* ws,
    const char** symbols,
    size_t symbol_count,
    fyers_data_type_t data_type,
    int channel
);

/**
 * @brief Unsubscribe from symbols
 * @param ws WebSocket instance
 * @param symbols Array of symbol strings to unsubscribe from
 * @param symbol_count Number of symbols in the array
 * @param data_type Type of data subscription
 * @param channel Channel number
 * @return FYERS_OK on success, error code otherwise
 */
FYERS_API fyers_error_t fyers_data_ws_unsubscribe(
    fyers_data_ws_t* ws,
    const char** symbols,
    size_t symbol_count,
    fyers_data_type_t data_type,
    int channel
);

/**
 * @brief Resume a channel
 * @param ws WebSocket instance
 * @param channel Channel number to resume
 * @return FYERS_OK on success, error code otherwise
 */
FYERS_API fyers_error_t fyers_data_ws_channel_resume(fyers_data_ws_t* ws, int channel);

/**
 * @brief Check if WebSocket is connected
 * @param ws WebSocket instance
 * @return true if connected, false otherwise
 */
FYERS_API bool fyers_data_ws_is_connected(fyers_data_ws_t* ws);

/**
 * @brief Keep the WebSocket running (blocks until closed)
 * @param ws WebSocket instance
 */
FYERS_API void fyers_data_ws_keep_running(fyers_data_ws_t* ws);

/** Custom-message code: the Standard plan's symbol subscription limit. */
#define FYERS_SYMBOL_LIMIT_EXCEEDED_CODE 51001

/** A decoded HSM custom message (response type 51). */
typedef struct {
    int32_t code;         /**< Server-provided code, e.g. 51001. */
    char message[512];    /**< Server-provided message, NUL-terminated. */
    uint8_t ref_type;     /**< Response type of the related request, 0 if absent. */
} fyers_custom_message_t;

/**
 * @brief Decode an HSM custom message frame (response type 51)
 *
 * Layout: [packetLen:2][respType:1][fieldCount:1] then, per field,
 * [fieldID:1][fieldLen:2][value:fieldLen]. Field 1 is the code (int32),
 * field 2 the message (UTF-8) and field 3 the ref_type. Unknown field ids are
 * skipped for forward compatibility.
 *
 * @param data Frame bytes
 * @param len Length of @p data
 * @param out Receives the decoded message on success
 * @return true on success, false if the frame is not a custom message or is truncated
 */
FYERS_API bool fyers_data_ws_parse_custom_message(const uint8_t* data,
                                                  size_t len,
                                                  fyers_custom_message_t* out);

#ifdef __cplusplus
}
#endif

#endif // FYERS_DATA_WS_H

