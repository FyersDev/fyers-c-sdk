#include "fyers_data_ws.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

static const char* g_symbols[] = {
    "NSE:SBIN-EQ",
    "NSE:RELIANCE-EQ",
};
static const size_t g_symbol_count = sizeof(g_symbols) / sizeof(g_symbols[0]);

void on_message(fyers_data_ws_t* ws, const char* json_message) {
    printf("Response: %s\n", json_message);
}

void on_error(fyers_data_ws_t* ws, int code, const char* message) {
    printf("Error [%d]: %s\n", code, message);
}

static void* unsubscribe_after_delay(void* arg) {
    fyers_data_ws_t* ws = (fyers_data_ws_t*)arg;
    sleep(30);
    printf("Calling unsubscribe after 30 seconds...\n");
    fyers_error_t err = fyers_data_ws_unsubscribe(
        ws, g_symbols, g_symbol_count,
        FYERS_DATA_TYPE_SYMBOL_UPDATE, 11
    );
    if (err != FYERS_OK) {
        printf("Unsubscribe returned error: %d\n", err);
    }
    return NULL;
}

void on_connect(fyers_data_ws_t* ws) {
    fyers_data_ws_subscribe(ws, g_symbols, g_symbol_count, FYERS_DATA_TYPE_SYMBOL_UPDATE, 11);

    /* Start thread to unsubscribe after 30 seconds */
    pthread_t tid;
    if (pthread_create(&tid, NULL, unsubscribe_after_delay, ws) == 0) {
        pthread_detach(tid);
    }
}

void on_close(fyers_data_ws_t* ws, const char* reason) {
    printf("Connection closed: %s\n", reason);
}

int main() {
    fyers_init();
    
    const char* access_token = "Z0G0WQQT6T-101:eyJ...";
    
    fyers_data_ws_t* ws = fyers_data_ws_create(
        access_token,
        false,      // write_to_file
        NULL,            // log_path
        false,           // litemode
        true,           // reconnect
        on_message,
        on_error,
        on_connect,
        on_close,
        5        // reconnect_retry
    );
    
    if (!ws) {
        fprintf(stderr, "Failed to create WebSocket instance\n");
        return 1;
    }
    
    if (fyers_data_ws_connect(ws) != FYERS_OK) {
        fprintf(stderr, "Failed to connect\n");
        fyers_data_ws_destroy(ws);
        return 1;
    }
    
    fyers_data_ws_keep_running(ws);
    
    return 0;
}