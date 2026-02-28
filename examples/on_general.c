#include "fyers_order_ws.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void on_trades(fyers_order_ws_t* ws, const char* json_message) {
    printf("Trade Response: %s\n", json_message);
}

void on_positions(fyers_order_ws_t* ws, const char* json_message) {
    printf("Position Response: %s\n", json_message);
}

void on_orders(fyers_order_ws_t* ws, const char* json_message) {
    printf("Order Response: %s\n", json_message);
}

void on_general(fyers_order_ws_t* ws, const char* json_message) {
    printf("General Response: %s\n", json_message);
}

void on_error(fyers_order_ws_t* ws, const char* message) {
    printf("Error: %s\n", message);
}

void on_connect(fyers_order_ws_t* ws) {
    // const char* data_type = "OnOrders"
    // const char* data_type = "OnTrades";
    // const char* data_type = "OnPositions";
    // const char* data_type = "OnGeneral"
    const char* data_type = "OnOrders,OnTrades,OnPositions,OnGeneral";
    
    fyers_order_ws_subscribe(ws, data_type);
}

void on_close(fyers_order_ws_t* ws, const char* reason) {
    printf("Connection closed: %s\n", reason);
}

int main() {
    fyers_init();

    const char* access_token = "Z0G0WQQT6T-101:eyJ...";

    fyers_order_ws_t* ws = fyers_order_ws_create(
        access_token,
        false,     // write_to_file
    NULL,               // log_path
        on_trades,
        on_positions,
        on_orders,
        on_general,
        on_error,
        on_connect,
        on_close,
        true,          // reconnect
        5        // reconnect_retry
    );
    
    if (!ws) {
        fprintf(stderr, "Failed to create WebSocket instance\n");
        return 1;
    }
    
    // Connect
    if (fyers_order_ws_connect(ws) != FYERS_OK) {
        fprintf(stderr, "Failed to connect\n");
        fyers_order_ws_destroy(ws);
        return 1;
    }
    
    fyers_order_ws_keep_running(ws);

    return 0;
}
