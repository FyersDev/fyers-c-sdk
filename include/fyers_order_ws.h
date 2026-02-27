/**
 * @file fyers_order_ws.h
 * @brief Fyers Order WebSocket API for real-time order/trade/position updates
 * @version 3.1.8
 */

#ifndef FYERS_ORDER_WS_H
#define FYERS_ORDER_WS_H

#include "fyers_api.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration
typedef struct fyers_order_ws fyers_order_ws_t;

// Callback function types (matching Python SDK)
typedef void (*fyers_order_ws_on_trades_t)(fyers_order_ws_t* ws, const char* json_message);
typedef void (*fyers_order_ws_on_positions_t)(fyers_order_ws_t* ws, const char* json_message);
typedef void (*fyers_order_ws_on_orders_t)(fyers_order_ws_t* ws, const char* json_message);
typedef void (*fyers_order_ws_on_general_t)(fyers_order_ws_t* ws, const char* json_message);
typedef void (*fyers_order_ws_on_error_t)(fyers_order_ws_t* ws, const char* message);
typedef void (*fyers_order_ws_on_connect_t)(fyers_order_ws_t* ws);
typedef void (*fyers_order_ws_on_close_t)(fyers_order_ws_t* ws, const char* reason);

/**
 * @brief Create a new Fyers Order WebSocket instance
 * @param access_token Access token (format: "client_id:token" or just "token")
 * @param write_to_file Write logs to file (true) or stdout (false)
 * @param log_path Path for log files (NULL for current directory)
 * @param on_trades Callback for trade updates
 * @param on_positions Callback for position updates
 * @param on_orders Callback for order updates
 * @param on_general Callback for general updates (edis, pricealerts, login)
 * @param on_error Callback for errors
 * @param on_connect Callback when connected
 * @param on_close Callback when connection closes
 * @param reconnect Enable automatic reconnection
 * @param reconnect_retry Maximum number of reconnection attempts
 * @return WebSocket instance or NULL on error
 */
FYERS_API fyers_order_ws_t* fyers_order_ws_create(
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
    int reconnect_retry
);

/**
 * @brief Destroy WebSocket instance
 * @param ws WebSocket instance
 */
FYERS_API void fyers_order_ws_destroy(fyers_order_ws_t* ws);

/**
 * @brief Connect to the WebSocket server
 * @param ws WebSocket instance
 * @return FYERS_OK on success, error code otherwise
 */
FYERS_API fyers_error_t fyers_order_ws_connect(fyers_order_ws_t* ws);

/**
 * @brief Close the WebSocket connection
 * @param ws WebSocket instance
 */
FYERS_API void fyers_order_ws_close(fyers_order_ws_t* ws);

/**
 * @brief Subscribe to data types
 * @param ws WebSocket instance
 * @param data_types Comma-separated string of data types: "OnOrders", "OnTrades", "OnPositions", "OnGeneral"
 * @return FYERS_OK on success, error code otherwise
 */
FYERS_API fyers_error_t fyers_order_ws_subscribe(fyers_order_ws_t* ws, const char* data_types);

/**
 * @brief Unsubscribe from data types
 * @param ws WebSocket instance
 * @param data_types Comma-separated string of data types to unsubscribe from
 * @return FYERS_OK on success, error code otherwise
 */
FYERS_API fyers_error_t fyers_order_ws_unsubscribe(fyers_order_ws_t* ws, const char* data_types);

/**
 * @brief Check if WebSocket is connected
 * @param ws WebSocket instance
 * @return true if connected, false otherwise
 */
FYERS_API bool fyers_order_ws_is_connected(fyers_order_ws_t* ws);

/**
 * @brief Keep the WebSocket running (blocks until closed)
 * @param ws WebSocket instance
 */
FYERS_API void fyers_order_ws_keep_running(fyers_order_ws_t* ws);

#ifdef __cplusplus
}
#endif

#endif // FYERS_ORDER_WS_H

