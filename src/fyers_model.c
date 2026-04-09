/**
 * @file fyers_model.c
 * @brief Fyers API model implementation
 */

#include "fyers_model.h"
#include "fyers_config.h"
#include "fyers_http_client.h"
#include "fyers_logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <cjson/cJSON.h>
#include <curl/curl.h>

struct fyers_model {
    char* client_id;
    char* access_token;
    char* header;
    bool is_async;
    fyers_logger_t* api_logger;
    fyers_logger_t* request_logger;
    fyers_http_client_t* http_client;
};

fyers_model_t* fyers_model_create(const char* client_id,
                                   const char* access_token,
                                   bool is_async,
                                   const char* log_path,
                                   fyers_log_level_t log_level) {
    if (!client_id || !access_token) {
        return NULL;
    }

    fyers_model_t* model = (fyers_model_t*)calloc(1, sizeof(fyers_model_t));
    if (!model) {
        return NULL;
    }

    model->client_id = strdup(client_id);
    model->access_token = strdup(access_token);
    model->is_async = is_async;

    // Create header: "client_id:access_token"
    size_t header_len = strlen(client_id) + strlen(access_token) + 2;
    model->header = (char*)malloc(header_len);
    snprintf(model->header, header_len, "%s:%s", client_id, access_token);

    model->api_logger = fyers_logger_create("FyersAPI", log_level, log_path);
    if (model->api_logger) {
        fyers_logger_debug(model->api_logger, "Model header created: '%s' (client_id='%s', access_token length=%zu)",
                          model->header, client_id, strlen(access_token));
    }
    model->request_logger = fyers_logger_create("FyersAPIRequest", FYERS_LOG_DEBUG, log_path);
    model->http_client = fyers_http_client_create(model->api_logger, model->request_logger);

    return model;
}

void fyers_model_destroy(fyers_model_t* model) {
    if (!model) {
        return;
    }

    free(model->client_id);
    free(model->access_token);
    free(model->header);

    if (model->api_logger) {
        fyers_logger_destroy(model->api_logger);
    }
    if (model->request_logger) {
        fyers_logger_destroy(model->request_logger);
    }
    if (model->http_client) {
        fyers_http_client_destroy(model->http_client);
    }

    free(model);
}

void fyers_response_destroy(fyers_response_t* response) {
    if (!response) {
        return;
    }

    if (response->data) {
        free(response->data);
    }

    free(response);
}

// Helper function to convert JSON to URL-encoded query string
static char* json_to_query_string(const char* json_str) {
    if (!json_str) {
        return NULL;
    }

    cJSON* json = cJSON_Parse(json_str);
    if (!json || !cJSON_IsObject(json)) {
        if (json) {
            cJSON_Delete(json);
        }
        return NULL;
    }

    // Create a temporary curl handle for URL encoding
    CURL* curl = curl_easy_init();
    if (!curl) {
        cJSON_Delete(json);
        return NULL;
    }

    char* query_str = (char*)malloc(2048);
    if (!query_str) {
        curl_easy_cleanup(curl);
        cJSON_Delete(json);
        return NULL;
    }
    query_str[0] = '\0';

    cJSON* item = NULL;
    cJSON_ArrayForEach(item, json) {
        if (!cJSON_IsString(item) && !cJSON_IsNumber(item)) {
            continue;
        }

        const char* key = item->string;
        if (!key) {
            continue;
        }

        // Get value as string
        char value_str[256];
        if (cJSON_IsString(item)) {
            snprintf(value_str, sizeof(value_str), "%s", cJSON_GetStringValue(item));
        } else if (cJSON_IsNumber(item)) {
            if (cJSON_IsNumber(item) && item->valuedouble == (double)item->valueint) {
                snprintf(value_str, sizeof(value_str), "%d", item->valueint);
            } else {
                snprintf(value_str, sizeof(value_str), "%.0f", item->valuedouble);
            }
        } else {
            continue;
        }

        // URL encode key and value
        char* encoded_key = curl_easy_escape(curl, key, 0);
        char* encoded_value = curl_easy_escape(curl, value_str, 0);

        if (encoded_key && encoded_value) {
            if (strlen(query_str) > 0) {
                strncat(query_str, "&", 2048 - strlen(query_str) - 1);
            }
            strncat(query_str, encoded_key, 2048 - strlen(query_str) - 1);
            strncat(query_str, "=", 2048 - strlen(query_str) - 1);
            strncat(query_str, encoded_value, 2048 - strlen(query_str) - 1);
        }

        curl_free(encoded_key);
        curl_free(encoded_value);
    }

    curl_easy_cleanup(curl);
    cJSON_Delete(json);

    return query_str;
}

static fyers_response_t* make_get_request(fyers_model_t* model,
                                           const char* endpoint,
                                           const char* params,
                                           bool data_flag) {
    char url[1024];
    if (data_flag) {
        snprintf(url, sizeof(url), "%s%s", FYERS_DATA_API_BASE_URL, endpoint);
    } else {
        snprintf(url, sizeof(url), "%s%s", FYERS_API_BASE_URL, endpoint);
    }

    if (params) {
        strncat(url, "?", sizeof(url) - strlen(url) - 1);
        strncat(url, params, sizeof(url) - strlen(url) - 1);
    }

    return fyers_http_client_get(model->http_client, url, model->header);
}

static fyers_response_t* make_post_request(fyers_model_t* model,
                                            const char* endpoint,
                                            const char* data) {
    char url[512];
    snprintf(url, sizeof(url), "%s%s", FYERS_API_BASE_URL, endpoint);
    return fyers_http_client_post(model->http_client, url, model->header, data);
}

static fyers_response_t* make_patch_request(fyers_model_t* model,
                                             const char* endpoint,
                                             const char* data) {
    char url[512];
    snprintf(url, sizeof(url), "%s%s", FYERS_API_BASE_URL, endpoint);
    return fyers_http_client_patch(model->http_client, url, model->header, data);
}

static fyers_response_t* make_delete_request(fyers_model_t* model,
                                              const char* endpoint,
                                              const char* data) {
    char url[512];
    snprintf(url, sizeof(url), "%s%s", FYERS_API_BASE_URL, endpoint);
    return fyers_http_client_delete(model->http_client, url, model->header, data);
}

static fyers_response_t* make_put_request(fyers_model_t* model,
                                           const char* endpoint,
                                           const char* data) {
    char url[512];
    snprintf(url, sizeof(url), "%s%s", FYERS_API_BASE_URL, endpoint);
    return fyers_http_client_put(model->http_client, url, model->header, data);
}

// User APIs
fyers_response_t* fyers_model_get_profile(fyers_model_t* model) {
    return make_get_request(model, FYERS_ENDPOINT_PROFILE, NULL, false);
}

fyers_response_t* fyers_model_get_funds(fyers_model_t* model) {
    return make_get_request(model, FYERS_ENDPOINT_FUNDS, NULL, false);
}

fyers_response_t* fyers_model_get_holdings(fyers_model_t* model) {
    return make_get_request(model, FYERS_ENDPOINT_HOLDINGS, NULL, false);
}

// Transaction APIs
fyers_response_t* fyers_model_get_tradebook(fyers_model_t* model, const char* order_tag) {
    char params[512] = "";
    if (order_tag) {
        snprintf(params, sizeof(params), "order_tag=%s", order_tag);
    }
    return make_get_request(model, FYERS_ENDPOINT_TRADEBOOK, NULL, false);
}

fyers_response_t* fyers_model_get_orderbook(fyers_model_t* model, const char* order_ids, const char* order_tag) {
    char params[512] = "";
    if (order_ids) {
        snprintf(params, sizeof(params), "id=%s", order_ids);
    } else if (order_tag) {
        snprintf(params, sizeof(params), "order_tag=%s", order_tag);
    }
    return make_get_request(model, FYERS_ENDPOINT_ORDERBOOK, order_ids ? params : NULL, false);
}

fyers_response_t* fyers_model_get_positions(fyers_model_t* model) {
    return make_get_request(model, FYERS_ENDPOINT_POSITIONS, NULL, false);
}

fyers_response_t* fyers_model_get_market_status(fyers_model_t* model) {
    return make_get_request(model, FYERS_ENDPOINT_MARKET_STATUS, NULL, true);
}

// Order placement
fyers_response_t* fyers_model_place_order(fyers_model_t* model, const char* order_json) {
    return make_post_request(model, FYERS_ENDPOINT_ORDERS, order_json);
}

fyers_response_t* fyers_model_place_multi_order(fyers_model_t* model, const char* order_json) {
    return make_post_request(model, FYERS_ENDPOINT_MULTI_ORDERS, order_json);
}

fyers_response_t* fyers_model_place_basket_orders(fyers_model_t* model, const char* orders_json) {
    return make_post_request(model, FYERS_ENDPOINT_MULTI_ORDERS, orders_json);
}

fyers_response_t* fyers_model_place_multileg_order(fyers_model_t* model, const char* order_json) {
    return make_post_request(model, FYERS_ENDPOINT_MULTILEG_ORDERS, order_json);
}

fyers_response_t* fyers_model_place_gtt_order(fyers_model_t* model, const char* order_json) {
    return make_post_request(model, FYERS_ENDPOINT_GTT_ORDERS, order_json);
}

// Order modification
fyers_response_t* fyers_model_modify_order(fyers_model_t* model, const char* order_json) {
    return make_patch_request(model, FYERS_ENDPOINT_ORDERS, order_json);
}

fyers_response_t* fyers_model_modify_basket_orders(fyers_model_t* model, const char* orders_json) {
    return make_patch_request(model, FYERS_ENDPOINT_MULTI_ORDERS, orders_json);
}

fyers_response_t* fyers_model_modify_gtt_order(fyers_model_t* model, const char* order_json) {
    return make_patch_request(model, FYERS_ENDPOINT_GTT_ORDERS, order_json);
}

// Order cancellation
fyers_response_t* fyers_model_cancel_order(fyers_model_t* model, const char* order_id) {
    return make_delete_request(model, FYERS_ENDPOINT_MULTI_ORDERS, order_id);;
}

fyers_response_t* fyers_model_cancel_basket_orders(fyers_model_t* model, const char* order_ids_json) {
    return make_delete_request(model, FYERS_ENDPOINT_MULTI_ORDERS, order_ids_json);
}

fyers_response_t* fyers_model_cancel_gtt_order(fyers_model_t* model, const char* order_json) {
    return make_delete_request(model, FYERS_ENDPOINT_GTT_ORDERS, order_json);
}

// Position management
fyers_response_t* fyers_model_exit_positions(fyers_model_t* model, const char* payload_json) {    
    return make_delete_request(model, FYERS_ENDPOINT_POSITIONS, payload_json);
}

fyers_response_t* fyers_model_exit_all_positions(fyers_model_t* model) {
    return fyers_model_exit_positions(model, NULL);
}

fyers_response_t* fyers_model_convert_position(fyers_model_t* model, const char* position_json) {
    return make_post_request(model, FYERS_ENDPOINT_POSITIONS, position_json);
}

// Market data APIs
fyers_response_t* fyers_model_get_history(fyers_model_t* model, const char* params_json) {
    char* query_str = json_to_query_string(params_json);
    if (!query_str) {
        // If JSON parsing fails, return NULL or try with original params
        return NULL;
    }
    
    fyers_response_t* response = make_get_request(model, FYERS_ENDPOINT_HISTORY, query_str, true);
    free(query_str);
    return response;
}

fyers_response_t* fyers_model_get_quotes(fyers_model_t* model, const char* symbols) {
    char params[512];
    snprintf(params, sizeof(params), "symbols=%s", symbols);
    return make_get_request(model, FYERS_ENDPOINT_QUOTES, params, true);
}

fyers_response_t* fyers_model_get_depth(fyers_model_t* model, const char* symbol, int ohlcv_flag) {
    char params[512];
    snprintf(params, sizeof(params), "symbol=%s&ohlcv_flag=%d", symbol, ohlcv_flag);
    return make_get_request(model, FYERS_ENDPOINT_DEPTH, params, true);
}

fyers_response_t* fyers_model_get_option_chain(fyers_model_t* model, const char* params_json) {
    char* query_str = json_to_query_string(params_json);
    if (!query_str) {
        // If JSON parsing fails, return NULL or try with original params
        return NULL;
    }
    
    fyers_response_t* response = make_get_request(model, FYERS_ENDPOINT_OPTION_CHAIN, query_str, true);
    free(query_str);
    return response;
}

// GTT APIs
fyers_response_t* fyers_model_get_gtt_orderbook(fyers_model_t* model) {
    return make_get_request(model, FYERS_ENDPOINT_GTT_ORDERBOOK, NULL, false);
}

// Price alerts
fyers_response_t* fyers_model_create_alert(fyers_model_t* model, const char* alert_json) {
    cJSON* alert = cJSON_Parse(alert_json);
    cJSON_AddStringToObject(alert, "agent", "fyers-api");
    char *alert_params = cJSON_PrintUnformatted(alert);

    return make_post_request(model, FYERS_ENDPOINT_PRICE_ALERT, alert_json);
}

fyers_response_t* fyers_model_get_alert(fyers_model_t* model) {
    return make_get_request(model, FYERS_ENDPOINT_PRICE_ALERT, NULL, false);
}

fyers_response_t* fyers_model_update_alert(fyers_model_t* model, const char* alert_json) {
    return make_put_request(model, FYERS_ENDPOINT_PRICE_ALERT, alert_json);
}

fyers_response_t* fyers_model_delete_alert(fyers_model_t* model, const char* alert_json) {
    cJSON* json = cJSON_CreateObject();
    fyers_response_t* response = make_delete_request(model, FYERS_ENDPOINT_PRICE_ALERT, alert_json);
    return response;
}

fyers_response_t* fyers_model_toggle_alert(fyers_model_t* model, const char* alert_id) {
    fyers_response_t* response = make_put_request(model, FYERS_ENDPOINT_TOGGLE_ALERT, alert_id);
    return response;
}

// Smart order APIs
fyers_response_t* fyers_model_create_smart_order_limit(fyers_model_t* model, const char* request_json) {
    return make_post_request(model, FYERS_ENDPOINT_CREATE_SMARTORDER_LIMIT, request_json);
}

fyers_response_t* fyers_model_create_smart_order_step(fyers_model_t* model, const char* request_json) {
    return make_post_request(model, FYERS_ENDPOINT_CREATE_SMARTORDER_STEP, request_json);
}

fyers_response_t* fyers_model_create_smart_order_sip(fyers_model_t* model, const char* request_json) {
    return make_post_request(model, FYERS_ENDPOINT_CREATE_SMARTORDER_SIP, request_json);
}

fyers_response_t* fyers_model_create_smart_order_trail(fyers_model_t* model, const char* request_json) {
    return make_post_request(model, FYERS_ENDPOINT_CREATE_SMARTORDER_TRAIL, request_json);
}

fyers_response_t* fyers_model_modify_smart_order(fyers_model_t* model, const char* request_json) {
    return make_patch_request(model, FYERS_ENDPOINT_MODIFY_SMARTORDER, request_json);
}

fyers_response_t* fyers_model_cancel_smart_order(fyers_model_t* model, const char* request_json) {
    return make_delete_request(model, FYERS_ENDPOINT_CANCEL_SMARTORDER, request_json);
}

fyers_response_t* fyers_model_pause_smart_order(fyers_model_t* model, const char* request_json) {
    return make_patch_request(model, FYERS_ENDPOINT_PAUSE_SMARTORDER, request_json);
}

fyers_response_t* fyers_model_resume_smart_order(fyers_model_t* model, const char* request_json) {
    return make_patch_request(model, FYERS_ENDPOINT_RESUME_SMARTORDER, request_json);
}

fyers_response_t* fyers_model_get_smart_order_book(fyers_model_t* model) {
    return make_get_request(model, FYERS_ENDPOINT_SMARTORDER_ORDERBOOK, NULL, false);
}

fyers_response_t* fyers_model_create_smart_exit_trigger(fyers_model_t* model, const char* request_json) {
    return make_post_request(model, FYERS_ENDPOINT_SMART_EXIT_TRIGGER, request_json);
}

fyers_response_t* fyers_model_get_smart_exit_trigger(fyers_model_t* model) {
    char params[256] = "";
    return make_get_request(model, FYERS_ENDPOINT_SMART_EXIT_TRIGGER, NULL, false);
}

fyers_response_t* fyers_model_update_smart_exit_trigger(fyers_model_t* model, const char* request_json) {
    return make_put_request(model, FYERS_ENDPOINT_SMART_EXIT_TRIGGER, request_json);
}

fyers_response_t* fyers_model_activate_deactivate_smart_exit_trigger(fyers_model_t* model, const char* request_json) {
    return make_post_request(model, FYERS_ENDPOINT_ACTIVATE_SMART_EXIT_TRIGGER, request_json);
}

// Logout
fyers_response_t* fyers_model_logout(fyers_model_t* model) {
    return make_post_request(model, FYERS_ENDPOINT_LOGOUT, "{}");
}

fyers_response_t* fyers_model_get_order_history(fyers_model_t* model, const char* params_json) {
    char* query_str = json_to_query_string(params_json);
    
    fyers_response_t* response = make_get_request(model, FYERS_ENDPOINT_ORDER_HISTORY, query_str, false);
    free(query_str);
    return response;
}

fyers_response_t* fyers_model_get_trade_history(fyers_model_t* model, const char* params_json) {
    char* query_str = json_to_query_string(params_json);
    
    fyers_response_t* response = make_get_request(model, FYERS_ENDPOINT_TRADE_HISTORY, query_str, false);
    free(query_str);
    return response;
}

