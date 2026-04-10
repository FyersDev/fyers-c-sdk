/**
 * @file auth_example.c
 * @brief Example demonstrating OAuth authentication flow
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fyers_api.h"
#include "fyers_session.h"
#include "fyers_model.h"
#include <cjson/cJSON.h>


void fy_generate_authcode(fyers_session_t* session) {
    if (generate_authcode(session) != FYERS_OK) {
        fprintf(stderr, "Failed to generate auth URL\n");
        fyers_session_destroy(session);
        fyers_cleanup();
    }
}

void fy_generate_token(fyers_session_t* session, const char* auth_code) {
    fyers_session_set_authcode(session, auth_code);

    fyers_response_t* response = generate_token(session);
    if (!response) {
        fprintf(stderr, "Failed to generate token\n");
        fyers_session_destroy(session);
        fyers_cleanup();
        return;
    }
    printf("response: %s\n", response->data);
    fyers_response_destroy(response);
}

void fy_generate_token_with_refresh_token(fyers_session_t* session, const char* refresh_token, const char* pin) {
    fyers_response_t* response = generate_token_with_refresh_token(session, refresh_token, pin);
    if (!response) {
        fprintf(stderr, "Failed to generate token (API call error)\n");
        fyers_session_destroy(session);
        fyers_cleanup();
        return;
    }
    printf("response: %s\n", response->data);
    fyers_response_destroy(response);
}

void fy_get_profile(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsiZDoxIiwiZDoyIiwieDowIiwieDoxIiwieDoyIl0sImF0X2hhc2giOiJnQUFBQUFCcFZSSHhtTThHT1RjSnVLM2FZRHp4VUFVdGRpUHFKM3pyaDBCdzVWd3AwNlVlMkdQSVJhdW5lUmk2aXJjRFRORDRrLThoc3FaeWd3WFlQSUczbmZHSF9SMW9nY3poemczVjl3Y3BOamx4cWRIcl9oYz0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMCwiZXhwIjoxNzY3MjI3NDAwLCJpYXQiOjE3NjcxODI4MzMsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzE4MjgzMywic3ViIjoiYWNjZXNzX3Rva2VuIn0.wsJ4qlwv079H6ck0_-jnWoehSrPkeK5zGcu6lD2_VCY");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return;
    }
    
    
    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );
    
    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }
    
    printf("Getting fy_get_profile\n");
    fyers_response_t* response = fyers_model_get_profile(model);
    if (response) printf("fy_get_profile response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

void fy_get_funds(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFZsdl9HR2ZGTmdVRkI5Wk43UWRqQUQ2c0FWMjNQRTVlemdOVDVFcDJCektWbzlaYnREekVNdmhrQ1BKS0hoZHBjVFNkN3RHYkRVa2U0dmNpWmJsZnViWlpIRGF0RENIcXgtYlllTnBxQmdHbzFDVT0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3MzEzODAwLCJpYXQiOjE3NjcyNjczMjcsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzI2NzMyNywic3ViIjoiYWNjZXNzX3Rva2VuIn0.g-FR8t4rmhEJqjCdwUISIzz8Jh4qFgaSyx3t_d09Tq");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return;
    }
    
    
    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );
    
    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    printf("Getting fy_get_funds\n");
    fyers_response_t* response = fyers_model_get_funds(model);
    if (response) printf("fy_get_funds response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

void fy_get_holdings(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFZsdl9HR2ZGTmdVRkI5Wk43UWRqQUQ2c0FWMjNQRTVlemdOVDVFcDJCektWbzlaYnREekVNdmhrQ1BKS0hoZHBjVFNkN3RHYkRVa2U0dmNpWmJsZnViWlpIRGF0RENIcXgtYlllTnBxQmdHbzFDVT0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3MzEzODAwLCJpYXQiOjE3NjcyNjczMjcsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzI2NzMyNywic3ViIjoiYWNjZXNzX3Rva2VuIn0.g-FR8t4rmhEJqjCdwUISIzz8Jh4qFgaSyx3t_d09Tq");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return;
    }
    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    printf("Getting fy_get_holdings\n");
    fyers_response_t* response = fyers_model_get_holdings(model);
    if (response) printf("fy_get_holdings response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

void fy_get_positions(fyers_session_t* session) {
    // fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFZsdl9HR2ZGTmdVRkI5Wk43UWRqQUQ2c0FWMjNQRTVlemdOVDVFcDJCektWbzlaYnREekVNdmhrQ1BKS0hoZHBjVFNkN3RHYkRVa2U0dmNpWmJsZnViWlpIRGF0RENIcXgtYlllTnBxQmdHbzFDVT0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3MzEzODAwLCJpYXQiOjE3NjcyNjczMjcsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzI2NzMyNywic3ViIjoiYWNjZXNzX3Rva2VuIn0.g-FR8t4rmhEJqjCdwUISIzz8Jh4qFgaSyx3t_d09Tqw");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return;
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    printf("Getting fy_get_positions\n");
    fyers_response_t* response = fyers_model_get_positions(model);
    if (response) printf("fy_get_positions response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

void fy_logout(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFZsdl9HR2ZGTmdVRkI5Wk43UWRqQUQ2c0FWMjNQRTVlemdOVDVFcDJCektWbzlaYnREekVNdmhrQ1BKS0hoZHBjVFNkN3RHYkRVa2U0dmNpWmJsZnViWlpIRGF0RENIcXgtYlllTnBxQmdHbzFDVT0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3MzEzODAwLCJpYXQiOjE3NjcyNjczMjcsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzI2NzMyNywic3ViIjoiYWNjZXNzX3Rva2VuIn0.g-FR8t4rmhEJqjCdwUISIzz8Jh4qFgaSyx3t_d09Tqw");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    printf("Getting fy_logout\n");
    fyers_response_t* response = fyers_model_logout(model);
    if (response) printf("fy_logout response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

void fy_get_orderbook(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFcwQ3JvQXRvT0ZqaFRPRUItZUpTQnJnQ0tZNWlhcGxwTW5HZkFiWk9YbzA3ZGdBSm0zSUd4STF1VFhUTWlRVlhQX3pqTHFxTGZhdnlPeEhNU1ZQdzVUSHNnakMwZzJJS3FFYmpob0V2OTdtLTRMND0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3NjU5NDAwLCJpYXQiOjE3Njc1ODgwMTEsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzU4ODAxMSwic3ViIjoiYWNjZXNzX3Rva2VuIn0.h0hpe___B_l80dr9Gra4iB0s9PYq7Jcj-18QBFug09A");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return;
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    printf("Getting fy_get_orderbook\n");
    fyers_response_t* response = fyers_model_get_orderbook(model, NULL, NULL);
    if (response) printf("fy_get_orderbook response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

void fy_get_orderbook_by_order_ids(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFcwQ3JvQXRvT0ZqaFRPRUItZUpTQnJnQ0tZNWlhcGxwTW5HZkFiWk9YbzA3ZGdBSm0zSUd4STF1VFhUTWlRVlhQX3pqTHFxTGZhdnlPeEhNU1ZQdzVUSHNnakMwZzJJS3FFYmpob0V2OTdtLTRMND0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3NjU5NDAwLCJpYXQiOjE3Njc1ODgwMTEsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzU4ODAxMSwic3ViIjoiYWNjZXNzX3Rva2VuIn0.h0hpe___B_l80dr9Gra4iB0s9PYq7Jcj-18QBFug09A");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return;
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    const char* order_ids = "26010200134575";
    printf("Getting fy_get_orderbook_by_order_id\n");
    fyers_response_t* response = fyers_model_get_orderbook(model, order_ids, NULL);
    if (response) printf("fy_get_orderbook_by_order_id response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

void fy_get_orderbook_by_order_tag(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFcwQ3JvQXRvT0ZqaFRPRUItZUpTQnJnQ0tZNWlhcGxwTW5HZkFiWk9YbzA3ZGdBSm0zSUd4STF1VFhUTWlRVlhQX3pqTHFxTGZhdnlPeEhNU1ZQdzVUSHNnakMwZzJJS3FFYmpob0V2OTdtLTRMND0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3NjU5NDAwLCJpYXQiOjE3Njc1ODgwMTEsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzU4ODAxMSwic3ViIjoiYWNjZXNzX3Rva2VuIn0.h0hpe___B_l80dr9Gra4iB0s9PYq7Jcj-18QBFug09A");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return;
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    const char* order_tag = "2:Untagged";
    printf("Getting fy_get_orderbook_by_order_tag\n");
    fyers_response_t* response = fyers_model_get_orderbook(model, NULL, order_tag);
    if (response) printf("fy_get_orderbook_by_order_tag response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

void fy_get_tradebook(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFcwQ3JvQXRvT0ZqaFRPRUItZUpTQnJnQ0tZNWlhcGxwTW5HZkFiWk9YbzA3ZGdBSm0zSUd4STF1VFhUTWlRVlhQX3pqTHFxTGZhdnlPeEhNU1ZQdzVUSHNnakMwZzJJS3FFYmpob0V2OTdtLTRMND0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3NjU5NDAwLCJpYXQiOjE3Njc1ODgwMTEsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzU4ODAxMSwic3ViIjoiYWNjZXNzX3Rva2VuIn0.h0hpe___B_l80dr9Gra4iB0s9PYq7Jcj-18QBFug09A");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    printf("Getting fy_get_tradebook\n");
    fyers_response_t* response = fyers_model_get_tradebook(model, NULL);
    if (response) printf("fy_get_tradebook response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

void fy_get_tradebook_by_order_tag(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFcwQ3JvQXRvT0ZqaFRPRUItZUpTQnJnQ0tZNWlhcGxwTW5HZkFiWk9YbzA3ZGdBSm0zSUd4STF1VFhUTWlRVlhQX3pqTHFxTGZhdnlPeEhNU1ZQdzVUSHNnakMwZzJJS3FFYmpob0V2OTdtLTRMND0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3NjU5NDAwLCJpYXQiOjE3Njc1ODgwMTEsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzU4ODAxMSwic3ViIjoiYWNjZXNzX3Rva2VuIn0.h0hpe___B_l80dr9Gra4iB0s9PYq7Jcj-18QBFug09A");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    const char* order_tag = "2:Untagged";
    printf("Getting fy_get_tradebook_by_order_tag\n");
    fyers_response_t* response = fyers_model_get_tradebook(model, order_tag);
    if (response) printf("fy_get_tradebook_by_order_tag response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

void fy_get_quotes(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFcwQ3JvQXRvT0ZqaFRPRUItZUpTQnJnQ0tZNWlhcGxwTW5HZkFiWk9YbzA3ZGdBSm0zSUd4STF1VFhUTWlRVlhQX3pqTHFxTGZhdnlPeEhNU1ZQdzVUSHNnakMwZzJJS3FFYmpob0V2OTdtLTRMND0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3NjU5NDAwLCJpYXQiOjE3Njc1ODgwMTEsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzU4ODAxMSwic3ViIjoiYWNjZXNzX3Rva2VuIn0.h0hpe___B_l80dr9Gra4iB0s9PYq7Jcj-18QBFug09A");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    printf("Getting fy_get_quotes\n");
    fyers_response_t* response = fyers_model_get_quotes(model, "NSE:SBIN-EQ");
    if (response) printf("fy_get_quotes response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

// void fy_get_history(fyers_session_t* session) {
// }
 
void fy_place_order(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFcwQ3JvQXRvT0ZqaFRPRUItZUpTQnJnQ0tZNWlhcGxwTW5HZkFiWk9YbzA3ZGdBSm0zSUd4STF1VFhUTWlRVlhQX3pqTHFxTGZhdnlPeEhNU1ZQdzVUSHNnakMwZzJJS3FFYmpob0V2OTdtLTRMND0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3NjU5NDAwLCJpYXQiOjE3Njc1ODgwMTEsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzU4ODAxMSwic3ViIjoiYWNjZXNzX3Rva2VuIn0.h0hpe___B_l80dr9Gra4iB0s9PYq7Jcj-18QBFug09A");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    printf("Placing order\n");
    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "symbol", "NSE:SBIN-EQ");
    cJSON_AddNumberToObject(json, "qty", 1);
    cJSON_AddNumberToObject(json, "type", 2);
    cJSON_AddNumberToObject(json, "side", 1);
    cJSON_AddStringToObject(json, "productType", "INTRADAY");
    cJSON_AddNumberToObject(json, "limitPrice", 0);
    cJSON_AddNumberToObject(json, "stopPrice", 0);
    cJSON_AddStringToObject(json, "validity", "DAY");
    cJSON_AddNumberToObject(json, "disclosedQty", 0);
    cJSON_AddBoolToObject(json, "offlineOrder", false);
    cJSON_AddNumberToObject(json, "stopLoss", 0);
    cJSON_AddNumberToObject(json, "takeProfit", 0);
    cJSON_AddStringToObject(json, "orderTag", "tag1");
    cJSON_AddBoolToObject(json, "isSliceOrder", false);
    const char* params = cJSON_Print(json);
    fyers_response_t* response = fyers_model_place_order(model, params);
    if (response) printf("fy_place_order response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

void fy_multi_order(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFcwQ3JvQXRvT0ZqaFRPRUItZUpTQnJnQ0tZNWlhcGxwTW5HZkFiWk9YbzA3ZGdBSm0zSUd4STF1VFhUTWlRVlhQX3pqTHFxTGZhdnlPeEhNU1ZQdzVUSHNnakMwZzJJS3FFYmpob0V2OTdtLTRMND0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3NjU5NDAwLCJpYXQiOjE3Njc1ODgwMTEsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzU4ODAxMSwic3ViIjoiYWNjZXNzX3Rva2VuIn0.h0hpe___B_l80dr9Gra4iB0s9PYq7Jcj-18QBFug09A");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    printf("Placing multiple order\n");
    cJSON *arr = cJSON_CreateArray();

    cJSON *obj1 = cJSON_CreateObject();
    cJSON_AddStringToObject(obj1, "symbol", "NSE:ITC-EQ");
    cJSON_AddNumberToObject(obj1, "qty", 1);
    cJSON_AddNumberToObject(obj1, "type", 1);
    cJSON_AddNumberToObject(obj1, "side", 1);
    cJSON_AddStringToObject(obj1, "productType", "INTRADAY");
    cJSON_AddNumberToObject(obj1, "limitPrice", 165);
    cJSON_AddNumberToObject(obj1, "stopPrice", 0);
    cJSON_AddNumberToObject(obj1, "disclosedQty", 0);
    cJSON_AddStringToObject(obj1, "validity", "DAY");
    cJSON_AddBoolToObject(obj1, "offlineOrder", true);
    cJSON_AddNumberToObject(obj1, "stopLoss", 0);
    cJSON_AddNumberToObject(obj1, "takeProfit", 0);
    cJSON_AddStringToObject(obj1, "orderTag", "tag1");

    cJSON_AddItemToArray(arr, obj1);

    cJSON *obj2 = cJSON_CreateObject();
    cJSON_AddStringToObject(obj2, "symbol", "NSE:ITC-EQ");
    cJSON_AddNumberToObject(obj2, "qty", 1);
    cJSON_AddNumberToObject(obj2, "type", 1);
    cJSON_AddNumberToObject(obj2, "side", 1);
    cJSON_AddStringToObject(obj2, "productType", "INTRADAY");
    cJSON_AddNumberToObject(obj2, "limitPrice", 165.10);
    cJSON_AddNumberToObject(obj2, "stopPrice", 0);
    cJSON_AddNumberToObject(obj2, "disclosedQty", 0);
    cJSON_AddStringToObject(obj2, "validity", "DAY");
    cJSON_AddBoolToObject(obj2, "offlineOrder", true);
    cJSON_AddNumberToObject(obj2, "stopLoss", 0);
    cJSON_AddNumberToObject(obj2, "takeProfit", 0);
    cJSON_AddStringToObject(obj2, "orderTag", "tag1");

    cJSON_AddItemToArray(arr, obj2);

    char *params = cJSON_PrintUnformatted(arr);  
    fyers_response_t* response = fyers_model_place_multi_order(model, params);
    if (response) printf("fy_place_multi_order response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

void fy_place_multileg_order(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFcwQ3JvQXRvT0ZqaFRPRUItZUpTQnJnQ0tZNWlhcGxwTW5HZkFiWk9YbzA3ZGdBSm0zSUd4STF1VFhUTWlRVlhQX3pqTHFxTGZhdnlPeEhNU1ZQdzVUSHNnakMwZzJJS3FFYmpob0V2OTdtLTRMND0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3NjU5NDAwLCJpYXQiOjE3Njc1ODgwMTEsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzU4ODAxMSwic3ViIjoiYWNjZXNzX3Rva2VuIn0.h0hpe___B_l80dr9Gra4iB0s9PYq7Jcj-18QBFug09A");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );
    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    printf("Placing multileg order\n");
    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "orderTag", "tag1");
    cJSON_AddStringToObject(root, "productType", "MARGIN");
    cJSON_AddBoolToObject(root, "offlineOrder", false);
    cJSON_AddStringToObject(root, "orderType", "3L");
    cJSON_AddStringToObject(root, "validity", "IOC");
    
    // ---- legs object ----
    cJSON *legs = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "legs", legs);
    
    // ---- leg1 ----
    cJSON *leg1 = cJSON_CreateObject();
    cJSON_AddStringToObject(leg1, "symbol", "NSE:SBIN26JANFUT");
    cJSON_AddNumberToObject(leg1, "qty", 750);
    cJSON_AddNumberToObject(leg1, "side", 1);
    cJSON_AddNumberToObject(leg1, "type", 1);
    cJSON_AddNumberToObject(leg1, "limitPrice", 800);
    cJSON_AddItemToObject(legs, "leg1", leg1);
    
    // ---- leg2 ----
    cJSON *leg2 = cJSON_CreateObject();
    cJSON_AddStringToObject(leg2, "symbol", "NSE:SBIN26FEBFUT");
    cJSON_AddNumberToObject(leg2, "qty", 750);
    cJSON_AddNumberToObject(leg2, "side", 1);
    cJSON_AddNumberToObject(leg2, "type", 1);
    cJSON_AddNumberToObject(leg2, "limitPrice", 800);
    cJSON_AddItemToObject(legs, "leg2", leg2);
    
    // ---- leg3 ----
    cJSON *leg3 = cJSON_CreateObject();
    cJSON_AddStringToObject(leg3, "symbol", "NSE:SBIN26MARFUT");
    cJSON_AddNumberToObject(leg3, "qty", 750);
    cJSON_AddNumberToObject(leg3, "side", 1);
    cJSON_AddNumberToObject(leg3, "type", 1);
    cJSON_AddNumberToObject(leg3, "limitPrice", 3);
    cJSON_AddItemToObject(legs, "leg3", leg3);
    
    // ---- print ----
    char *params = cJSON_PrintUnformatted(root);    
    
    fyers_response_t* response = fyers_model_place_multileg_order(model, params);
    if (response) printf("fy_place_multileg_order response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

void fy_place_gtt_order(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFcwQ3JvQXRvT0ZqaFRPRUItZUpTQnJnQ0tZNWlhcGxwTW5HZkFiWk9YbzA3ZGdBSm0zSUd4STF1VFhUTWlRVlhQX3pqTHFxTGZhdnlPeEhNU1ZQdzVUSHNnakMwZzJJS3FFYmpob0V2OTdtLTRMND0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3NjU5NDAwLCJpYXQiOjE3Njc1ODgwMTEsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzU4ODAxMSwic3ViIjoiYWNjZXNzX3Rva2VuIn0.h0hpe___B_l80dr9Gra4iB0s9PYq7Jcj-18QBFug09A");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    printf("Placing single fy_place_gtt_order");

    cJSON *root = cJSON_CreateObject();

    cJSON_AddNumberToObject(root, "side", 1);
    cJSON_AddStringToObject(root, "symbol", "NSE:SBIN-EQ");
    cJSON_AddStringToObject(root, "productType", "CNC");

    // ---- orderInfo ----
    cJSON *orderInfo = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "orderInfo", orderInfo);

    // ---- leg1 ----
    cJSON *leg1 = cJSON_CreateObject();
    cJSON_AddNumberToObject(leg1, "price", 1000);
    cJSON_AddNumberToObject(leg1, "triggerPrice", 1000);
    cJSON_AddNumberToObject(leg1, "qty", 1);

    cJSON_AddItemToObject(orderInfo, "leg1", leg1);

    // ---- Convert to string ----
    char *params = cJSON_PrintUnformatted(root);


    fyers_response_t* response = fyers_model_place_gtt_order(model, params);
    if (response) printf("fy_place_gtt_order response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

void fy_place_oco_gtt_order(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFcwQ3JvQXRvT0ZqaFRPRUItZUpTQnJnQ0tZNWlhcGxwTW5HZkFiWk9YbzA3ZGdBSm0zSUd4STF1VFhUTWlRVlhQX3pqTHFxTGZhdnlPeEhNU1ZQdzVUSHNnakMwZzJJS3FFYmpob0V2OTdtLTRMND0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3NjU5NDAwLCJpYXQiOjE3Njc1ODgwMTEsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzU4ODAxMSwic3ViIjoiYWNjZXNzX3Rva2VuIn0.h0hpe___B_l80dr9Gra4iB0s9PYq7Jcj-18QBFug09A");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    printf("Getting fy_place_oco_gtt_order\n");

    cJSON *root = cJSON_CreateObject();

    cJSON_AddNumberToObject(root, "side", 1);
    cJSON_AddStringToObject(root, "symbol", "NSE:CHOLAFIN-EQ");
    cJSON_AddStringToObject(root, "productType", "CNC");

    // ---- orderInfo ----
    cJSON *orderInfo = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "orderInfo", orderInfo);

    // ---- leg1 ----
    cJSON *leg1 = cJSON_CreateObject();
    cJSON_AddNumberToObject(leg1, "price", 1010);
    cJSON_AddNumberToObject(leg1, "triggerPrice", 1010);
    cJSON_AddNumberToObject(leg1, "qty", 3);
    cJSON_AddItemToObject(orderInfo, "leg1", leg1);

    // ---- leg2 ----
    cJSON *leg2 = cJSON_CreateObject();
    cJSON_AddNumberToObject(leg2, "price", 1010);
    cJSON_AddNumberToObject(leg2, "triggerPrice", 1010);
    cJSON_AddNumberToObject(leg2, "qty", 3);
    cJSON_AddItemToObject(orderInfo, "leg2", leg2);

    // ---- stringify ----
    char *params = cJSON_PrintUnformatted(root);

    fyers_response_t* response = fyers_model_place_gtt_order(model,params);
    if (response) printf("fy_place_oco_gtt_order response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

void fy_modify_gtt_order(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFcwQ3JvQXRvT0ZqaFRPRUItZUpTQnJnQ0tZNWlhcGxwTW5HZkFiWk9YbzA3ZGdBSm0zSUd4STF1VFhUTWlRVlhQX3pqTHFxTGZhdnlPeEhNU1ZQdzVUSHNnakMwZzJJS3FFYmpob0V2OTdtLTRMND0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3NjU5NDAwLCJpYXQiOjE3Njc1ODgwMTEsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzU4ODAxMSwic3ViIjoiYWNjZXNzX3Rva2VuIn0.h0hpe___B_l80dr9Gra4iB0s9PYq7Jcj-18QBFug09A");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    printf("Getting fy_modify_gtt_order\n");

    cJSON *root = cJSON_CreateObject();

    // id
    cJSON_AddStringToObject(root, "id", "26010200001538");

    // ---- orderInfo ----
    cJSON *orderInfo = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "orderInfo", orderInfo);

    // ---- leg1 ----
    cJSON *leg1 = cJSON_CreateObject();
    cJSON_AddNumberToObject(leg1, "price", 1010);
    cJSON_AddNumberToObject(leg1, "triggerPrice", 1010);
    cJSON_AddNumberToObject(leg1, "qty", 5);
    cJSON_AddItemToObject(orderInfo, "leg1", leg1);

    // ---- leg2 ----
    cJSON *leg2 = cJSON_CreateObject();
    cJSON_AddNumberToObject(leg2, "price", 1010);
    cJSON_AddNumberToObject(leg2, "triggerPrice", 1010);
    cJSON_AddNumberToObject(leg2, "qty", 5);
    cJSON_AddItemToObject(orderInfo, "leg2", leg2);

    // ---- stringify ----
    char *params = cJSON_PrintUnformatted(root);

    fyers_response_t* response = fyers_model_modify_gtt_order(model,params);
    if (response) printf("fy_modify_gtt_order response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

void fy_cancel_gtt_order(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFcwQ3JvQXRvT0ZqaFRPRUItZUpTQnJnQ0tZNWlhcGxwTW5HZkFiWk9YbzA3ZGdBSm0zSUd4STF1VFhUTWlRVlhQX3pqTHFxTGZhdnlPeEhNU1ZQdzVUSHNnakMwZzJJS3FFYmpob0V2OTdtLTRMND0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3NjU5NDAwLCJpYXQiOjE3Njc1ODgwMTEsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzU4ODAxMSwic3ViIjoiYWNjZXNzX3Rva2VuIn0.h0hpe___B_l80dr9Gra4iB0s9PYq7Jcj-18QBFug09A");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    printf("Delete fy_cancel_gtt_order\n");

    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "id", "26010200001538");
    char *params = cJSON_PrintUnformatted(root);

    fyers_response_t* response = fyers_model_cancel_gtt_order(model,params);
    if (response) printf("fy_cancel_gtt_order response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);

}

void fy_get_gtt_orderbook(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFcwQ3JvQXRvT0ZqaFRPRUItZUpTQnJnQ0tZNWlhcGxwTW5HZkFiWk9YbzA3ZGdBSm0zSUd4STF1VFhUTWlRVlhQX3pqTHFxTGZhdnlPeEhNU1ZQdzVUSHNnakMwZzJJS3FFYmpob0V2OTdtLTRMND0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3NjU5NDAwLCJpYXQiOjE3Njc1ODgwMTEsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzU4ODAxMSwic3ViIjoiYWNjZXNzX3Rva2VuIn0.h0hpe___B_l80dr9Gra4iB0s9PYq7Jcj-18QBFug09A");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    printf("Getting fy_get_gtt_orderbook\n");

    fyers_response_t* response = fyers_model_get_gtt_orderbook(model);
    if (response) printf("fy_get_gtt_orderbook response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}


void fy_modify_order(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFcwQ3JvQXRvT0ZqaFRPRUItZUpTQnJnQ0tZNWlhcGxwTW5HZkFiWk9YbzA3ZGdBSm0zSUd4STF1VFhUTWlRVlhQX3pqTHFxTGZhdnlPeEhNU1ZQdzVUSHNnakMwZzJJS3FFYmpob0V2OTdtLTRMND0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3NjU5NDAwLCJpYXQiOjE3Njc1ODgwMTEsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzU4ODAxMSwic3ViIjoiYWNjZXNzX3Rva2VuIn0.h0hpe___B_l80dr9Gra4iB0s9PYq7Jcj-18QBFug09A");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "id", "26010500129133");
    cJSON_AddNumberToObject(root, "qty", 2);
    cJSON_AddNumberToObject(root, "type", 1);
    cJSON_AddNumberToObject(root, "side", 1);
    cJSON_AddNumberToObject(root, "limitPrice", 22.10);

    // Convert to string
    char *params = cJSON_PrintUnformatted(root);
    printf("Getting fy_modify_order\n");

    fyers_response_t* response = fyers_model_modify_order(model,params);
    if (response) printf("fy_modify_order response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
    
}
void fy_modify_mutli_order(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFcwQ3JvQXRvT0ZqaFRPRUItZUpTQnJnQ0tZNWlhcGxwTW5HZkFiWk9YbzA3ZGdBSm0zSUd4STF1VFhUTWlRVlhQX3pqTHFxTGZhdnlPeEhNU1ZQdzVUSHNnakMwZzJJS3FFYmpob0V2OTdtLTRMND0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3NjU5NDAwLCJpYXQiOjE3Njc1ODgwMTEsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzU4ODAxMSwic3ViIjoiYWNjZXNzX3Rva2VuIn0.h0hpe___B_l80dr9Gra4iB0s9PYq7Jcj-18QBFug09A");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    cJSON *arr = cJSON_CreateArray();

    // -------- object 1 --------
    cJSON *o1 = cJSON_CreateObject();
    cJSON_AddStringToObject(o1, "id", "26010500129133");
    cJSON_AddNumberToObject(o1, "type", 1);
    cJSON_AddNumberToObject(o1, "limitPrice", 22.11);
    cJSON_AddNumberToObject(o1, "stopPrice", 0);
    cJSON_AddNumberToObject(o1, "qty", 5);
    cJSON_AddItemToArray(arr, o1);

    // -------- object 2 --------
    cJSON *o2 = cJSON_CreateObject();
    cJSON_AddStringToObject(o2, "id", "26010500164960");
    cJSON_AddNumberToObject(o2, "type", 1);
    cJSON_AddNumberToObject(o2, "limitPrice", 13.01);
    cJSON_AddNumberToObject(o2, "stopPrice", 0);
    cJSON_AddNumberToObject(o2, "qty", 5);
    cJSON_AddItemToArray(arr, o2);

    // Convert to string
    char *params = cJSON_PrintUnformatted(arr);
    printf("Getting fy_modify_mutli_order\n");

    fyers_response_t* response = fyers_model_modify_order(model,params);
    if (response) printf("fy_modify_mutli_order response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);

}
void fy_cancel_order(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFcwQ3JvQXRvT0ZqaFRPRUItZUpTQnJnQ0tZNWlhcGxwTW5HZkFiWk9YbzA3ZGdBSm0zSUd4STF1VFhUTWlRVlhQX3pqTHFxTGZhdnlPeEhNU1ZQdzVUSHNnakMwZzJJS3FFYmpob0V2OTdtLTRMND0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3NjU5NDAwLCJpYXQiOjE3Njc1ODgwMTEsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzU4ODAxMSwic3ViIjoiYWNjZXNzX3Rva2VuIn0.h0hpe___B_l80dr9Gra4iB0s9PYq7Jcj-18QBFug09A");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "id", "26010500129133");

    char *params = cJSON_PrintUnformatted(root);
    printf("Getting fy_cancel_order\n");

    fyers_response_t* response = fyers_model_cancel_order(model,params);
    if (response) printf("fy_cancel_order response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);

}
void fy_cancel_mutli_order(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFcwQ3JvQXRvT0ZqaFRPRUItZUpTQnJnQ0tZNWlhcGxwTW5HZkFiWk9YbzA3ZGdBSm0zSUd4STF1VFhUTWlRVlhQX3pqTHFxTGZhdnlPeEhNU1ZQdzVUSHNnakMwZzJJS3FFYmpob0V2OTdtLTRMND0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3NjU5NDAwLCJpYXQiOjE3Njc1ODgwMTEsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzU4ODAxMSwic3ViIjoiYWNjZXNzX3Rva2VuIn0.h0hpe___B_l80dr9Gra4iB0s9PYq7Jcj-18QBFug09A");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    cJSON *arr = cJSON_CreateArray();

    // -------- object 1 --------
    cJSON *o1 = cJSON_CreateObject();
    cJSON_AddStringToObject(o1, "id", "26010500212409");
    cJSON_AddItemToArray(arr, o1);

    // -------- object 2 --------
    cJSON *o2 = cJSON_CreateObject();
    cJSON_AddStringToObject(o2, "id", "26010500164960");
    cJSON_AddItemToArray(arr, o2);

    // Convert to string
    char *params = cJSON_PrintUnformatted(arr);
    printf("Getting fy_cancel_mutli_order\n");

    fyers_response_t* response = fyers_model_cancel_order(model,params);
    if (response) printf("fy_cancel_mutli_order response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);

}
void fy_exit_all_positions(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFcwQ3JvQXRvT0ZqaFRPRUItZUpTQnJnQ0tZNWlhcGxwTW5HZkFiWk9YbzA3ZGdBSm0zSUd4STF1VFhUTWlRVlhQX3pqTHFxTGZhdnlPeEhNU1ZQdzVUSHNnakMwZzJJS3FFYmpob0V2OTdtLTRMND0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3NjU5NDAwLCJpYXQiOjE3Njc1ODgwMTEsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzU4ODAxMSwic3ViIjoiYWNjZXNzX3Rva2VuIn0.h0hpe___B_l80dr9Gra4iB0s9PYq7Jcj-18QBFug09A");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "exit_all", 1);

    char *params = cJSON_PrintUnformatted(root);
    printf("Getting fy_exit_all_positions\n");

    fyers_response_t* response = fyers_model_exit_positions(model,params);
    if (response) printf("fy_exit_all_positions response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);

}

void fy_exit_position_by_segment(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFcwQ3JvQXRvT0ZqaFRPRUItZUpTQnJnQ0tZNWlhcGxwTW5HZkFiWk9YbzA3ZGdBSm0zSUd4STF1VFhUTWlRVlhQX3pqTHFxTGZhdnlPeEhNU1ZQdzVUSHNnakMwZzJJS3FFYmpob0V2OTdtLTRMND0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3NjU5NDAwLCJpYXQiOjE3Njc1ODgwMTEsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzU4ODAxMSwic3ViIjoiYWNjZXNzX3Rva2VuIn0.h0hpe___B_l80dr9Gra4iB0s9PYq7Jcj-18QBFug09A");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "segment", 1);
    cJSON_AddNumberToObject(root, "side", 1);
    cJSON_AddStringToObject(root, "productType","INTRADAY");

    char *params = cJSON_PrintUnformatted(root);
    printf("Getting fy_exit_position_by_segment\n");

    fyers_response_t* response = fyers_model_exit_positions(model,params);
    if (response) printf("fy_exit_position_by_segment response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);

}
void fy_exit_position_by_id(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFcwQ3JvQXRvT0ZqaFRPRUItZUpTQnJnQ0tZNWlhcGxwTW5HZkFiWk9YbzA3ZGdBSm0zSUd4STF1VFhUTWlRVlhQX3pqTHFxTGZhdnlPeEhNU1ZQdzVUSHNnakMwZzJJS3FFYmpob0V2OTdtLTRMND0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3NjU5NDAwLCJpYXQiOjE3Njc1ODgwMTEsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzU4ODAxMSwic3ViIjoiYWNjZXNzX3Rva2VuIn0.h0hpe___B_l80dr9Gra4iB0s9PYq7Jcj-18QBFug09A");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "id", "NSE:SBIN-EQ-BO");

    char *params = cJSON_PrintUnformatted(root);
    printf("Getting fy_exit_position_by_id\n");

    fyers_response_t* response = fyers_model_exit_positions(model,params);
    if (response) printf("fy_exit_position_by_id response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);

}
void fy_pending_order_cancel(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFcwQ3JvQXRvT0ZqaFRPRUItZUpTQnJnQ0tZNWlhcGxwTW5HZkFiWk9YbzA3ZGdBSm0zSUd4STF1VFhUTWlRVlhQX3pqTHFxTGZhdnlPeEhNU1ZQdzVUSHNnakMwZzJJS3FFYmpob0V2OTdtLTRMND0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3NjU5NDAwLCJpYXQiOjE3Njc1ODgwMTEsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzU4ODAxMSwic3ViIjoiYWNjZXNzX3Rva2VuIn0.h0hpe___B_l80dr9Gra4iB0s9PYq7Jcj-18QBFug09A");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "pending_orders_cancel", 1);

    char *params = cJSON_PrintUnformatted(root);
    printf("Getting fy_pending_order_cancel\n");

    fyers_response_t* response = fyers_model_exit_positions(model,params);
    if (response) printf("fy_pending_order_cancel response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);

}
void fy_convert_position(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFcwQ3JvQXRvT0ZqaFRPRUItZUpTQnJnQ0tZNWlhcGxwTW5HZkFiWk9YbzA3ZGdBSm0zSUd4STF1VFhUTWlRVlhQX3pqTHFxTGZhdnlPeEhNU1ZQdzVUSHNnakMwZzJJS3FFYmpob0V2OTdtLTRMND0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3NjU5NDAwLCJpYXQiOjE3Njc1ODgwMTEsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzU4ODAxMSwic3ViIjoiYWNjZXNzX3Rva2VuIn0.h0hpe___B_l80dr9Gra4iB0s9PYq7Jcj-18QBFug09A");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "symbol","MCX:SILVERMIC20NOVFUT-INTRADAY");
    cJSON_AddNumberToObject(root, "pending_orders_cancel", 1);
    cJSON_AddNumberToObject(root, "positionSide", 1);
    cJSON_AddNumberToObject(root, "convertQty", 1);
    cJSON_AddStringToObject(root, "convertFrom","INTRADAY");
    cJSON_AddStringToObject(root, "convertTo","CNC");
    cJSON_AddNumberToObject(root, "overnight", 1);
    
    char *params = cJSON_PrintUnformatted(root);
    printf("Getting fy_pending_order_cancel\n");

    fyers_response_t* response = fyers_model_exit_positions(model,params);
    if (response) printf("fy_convert_position response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

void fy_get_broker_config(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFcwQ3JvQXRvT0ZqaFRPRUItZUpTQnJnQ0tZNWlhcGxwTW5HZkFiWk9YbzA3ZGdBSm0zSUd4STF1VFhUTWlRVlhQX3pqTHFxTGZhdnlPeEhNU1ZQdzVUSHNnakMwZzJJS3FFYmpob0V2OTdtLTRMND0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3NjU5NDAwLCJpYXQiOjE3Njc1ODgwMTEsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzU4ODAxMSwic3ViIjoiYWNjZXNzX3Rva2VuIn0.h0hpe___B_l80dr9Gra4iB0s9PYq7Jcj-18QBFug09A");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    printf("Getting fy_get_broker_config\n");

    fyers_response_t* response = fyers_model_get_market_status(model);
    if (response) printf("fy_get_broker_config response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

void fy_get_history(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFcwQ3JvQXRvT0ZqaFRPRUItZUpTQnJnQ0tZNWlhcGxwTW5HZkFiWk9YbzA3ZGdBSm0zSUd4STF1VFhUTWlRVlhQX3pqTHFxTGZhdnlPeEhNU1ZQdzVUSHNnakMwZzJJS3FFYmpob0V2OTdtLTRMND0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3NjU5NDAwLCJpYXQiOjE3Njc1ODgwMTEsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzU4ODAxMSwic3ViIjoiYWNjZXNzX3Rva2VuIn0.h0hpe___B_l80dr9Gra4iB0s9PYq7Jcj-18QBFug09A");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    printf("Getting fy_get_history\n");

    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "symbol", "NSE:SBIN-EQ");
    cJSON_AddStringToObject(json, "resolution", "D");
    cJSON_AddStringToObject(json, "date_format", "0");
    cJSON_AddStringToObject(json, "range_from", "1690895316");
    cJSON_AddStringToObject(json, "range_to", "1691068173");
    cJSON_AddStringToObject(json, "cont_flag", "1");
    const char* params = cJSON_Print(json);

    fyers_response_t* response = fyers_model_get_history(model, params);
    if (response) printf("fy_get_history response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

void fy_get_depth(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFcwQ3JvQXRvT0ZqaFRPRUItZUpTQnJnQ0tZNWlhcGxwTW5HZkFiWk9YbzA3ZGdBSm0zSUd4STF1VFhUTWlRVlhQX3pqTHFxTGZhdnlPeEhNU1ZQdzVUSHNnakMwZzJJS3FFYmpob0V2OTdtLTRMND0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJmYmM2YmE3MGE3YWI2MzEwNDZlYzAxOTNiODgxY2M5NTAyMjhiMmRjNjI0YzYwNDc1NzJkNDAwMyIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY3NjU5NDAwLCJpYXQiOjE3Njc1ODgwMTEsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2NzU4ODAxMSwic3ViIjoiYWNjZXNzX3Rva2VuIn0.h0hpe___B_l80dr9Gra4iB0s9PYq7Jcj-18QBFug09A");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    printf("Getting fy_get_depth\n");
    fyers_response_t* response = fyers_model_get_depth(model, "NSE:SBIN-EQ", 1);
    if (response) printf("fy_get_depth response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

void fy_get_option_chain(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eb..");

    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    printf("Getting fy_get_option_chain\n");
    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "symbol", "NSE:TCS-EQ");
    cJSON_AddNumberToObject(json, "strikecount", 7);
    cJSON_AddStringToObject(json, "greeks", "1");
    const char* params = cJSON_Print(json);


    fyers_response_t* response = fyers_model_get_option_chain(model, params);
    if (response) printf("fy_get_option_chain response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

void fy_create_alert(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFpLYU1iRlJBTWVXZnVuTExRa2ttd2tFM3ZIdlJRbzdxUVBmTjBmRm5uVXZXRDRndVpnekw4X2NNSTZBRWtCUktPemhqSTVqQlBFYy05SERIUEZGUW1tMER6NE9YOUZCZmRwLXQ1MllJQmZfWXpqQT0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJjMDFlNDQ3ZDc1YzNjZDIwYTQ3YTQ0ZjA3NjJmNTM3Mjc5YjJmZWY4NGY4NzY4MTk4NTQwOTdkMiIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY4MjY0MjAwLCJpYXQiOjE3NjgyMDM5MTYsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2ODIwMzkxNiwic3ViIjoiYWNjZXNzX3Rva2VuIn0.GRxdraCQNX0vYBYlTQs3YkRUINit-rZvuJqbHAUSM3E");
    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    printf("Creating fy_create_alert\n");
    cJSON* json = cJSON_CreateObject();

    cJSON_AddStringToObject(json, "agent", "fyers-api");
    cJSON_AddNumberToObject(json, "alert-type", 1);
    cJSON_AddStringToObject(json, "name", "NSE:NIFTY50-INDEX");
    cJSON_AddStringToObject(json, "symbol", "NSE:NIFTY50-INDEX");
    cJSON_AddStringToObject(json, "comparisonType", "LTP");
    cJSON_AddStringToObject(json, "condition", "LTE");
    cJSON_AddNumberToObject(json, "value", 25421.45);

    const char* params = cJSON_Print(json);

    fyers_response_t* response = fyers_model_create_alert(model, params);
    if (response) printf("fy_create_alert response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

void fy_get_alert(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFpLYU1iRlJBTWVXZnVuTExRa2ttd2tFM3ZIdlJRbzdxUVBmTjBmRm5uVXZXRDRndVpnekw4X2NNSTZBRWtCUktPemhqSTVqQlBFYy05SERIUEZGUW1tMER6NE9YOUZCZmRwLXQ1MllJQmZfWXpqQT0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJjMDFlNDQ3ZDc1YzNjZDIwYTQ3YTQ0ZjA3NjJmNTM3Mjc5YjJmZWY4NGY4NzY4MTk4NTQwOTdkMiIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY4MjY0MjAwLCJpYXQiOjE3NjgyMDM5MTYsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2ODIwMzkxNiwic3ViIjoiYWNjZXNzX3Rva2VuIn0.GRxdraCQNX0vYBYlTQs3YkRUINit-rZvuJqbHAUSM3E");
    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    printf("Getting fy_get_alert\n");
    fyers_response_t* response = fyers_model_get_alert(model);
    if (response) printf("fy_get_alert response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

void fy_update_alert(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFpLYU1iRlJBTWVXZnVuTExRa2ttd2tFM3ZIdlJRbzdxUVBmTjBmRm5uVXZXRDRndVpnekw4X2NNSTZBRWtCUktPemhqSTVqQlBFYy05SERIUEZGUW1tMER6NE9YOUZCZmRwLXQ1MllJQmZfWXpqQT0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJjMDFlNDQ3ZDc1YzNjZDIwYTQ3YTQ0ZjA3NjJmNTM3Mjc5YjJmZWY4NGY4NzY4MTk4NTQwOTdkMiIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY4MjY0MjAwLCJpYXQiOjE3NjgyMDM5MTYsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2ODIwMzkxNiwic3ViIjoiYWNjZXNzX3Rva2VuIn0.GRxdraCQNX0vYBYlTQs3YkRUINit-rZvuJqbHAUSM3E");
    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    printf("Updating fy_update_alert\n");
    cJSON* json = cJSON_CreateObject();

    cJSON_AddStringToObject(json, "alertId", "6134468");
    cJSON_AddStringToObject(json, "agent", "fyers-api");
    cJSON_AddNumberToObject(json, "alert-type", 1);
    cJSON_AddStringToObject(json, "name", "NSE:NIFTY50-INDEX");
    cJSON_AddStringToObject(json, "symbol", "NSE:NIFTY50-INDEX");
    cJSON_AddStringToObject(json, "comparisonType", "LTP");
    cJSON_AddStringToObject(json, "condition", "LTE");
    cJSON_AddNumberToObject(json, "value", 25423.45);

    const char* params = cJSON_Print(json);

    fyers_response_t* response = fyers_model_update_alert(model, params);
    if (response) printf("fy_update_alert response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

void fy_delete_alert(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFpLYU1iRlJBTWVXZnVuTExRa2ttd2tFM3ZIdlJRbzdxUVBmTjBmRm5uVXZXRDRndVpnekw4X2NNSTZBRWtCUktPemhqSTVqQlBFYy05SERIUEZGUW1tMER6NE9YOUZCZmRwLXQ1MllJQmZfWXpqQT0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJjMDFlNDQ3ZDc1YzNjZDIwYTQ3YTQ0ZjA3NjJmNTM3Mjc5YjJmZWY4NGY4NzY4MTk4NTQwOTdkMiIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY4MjY0MjAwLCJpYXQiOjE3NjgyMDM5MTYsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2ODIwMzkxNiwic3ViIjoiYWNjZXNzX3Rva2VuIn0.GRxdraCQNX0vYBYlTQs3YkRUINit-rZvuJqbHAUSM3E");
    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }

    printf("Deleting fy_delete_alert\n");
    cJSON* json = cJSON_CreateObject();

    cJSON_AddStringToObject(json, "alertId", "4115571");
    cJSON_AddStringToObject(json, "agent", "fyers-api");
    const char* params = cJSON_Print(json);
    fyers_response_t* response = fyers_model_delete_alert(model, params);
    if (response) printf("fy_delete_alert response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

void fy_toggle_alert(fyers_session_t* session) {
    fyers_session_set_access_token(session, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOlsieDowIiwieDoxIl0sImF0X2hhc2giOiJnQUFBQUFCcFpLYU1iRlJBTWVXZnVuTExRa2ttd2tFM3ZIdlJRbzdxUVBmTjBmRm5uVXZXRDRndVpnekw4X2NNSTZBRWtCUktPemhqSTVqQlBFYy05SERIUEZGUW1tMER6NE9YOUZCZmRwLXQ1MllJQmZfWXpqQT0iLCJkaXNwbGF5X25hbWUiOiIiLCJvbXMiOiJLMSIsImhzbV9rZXkiOiJjMDFlNDQ3ZDc1YzNjZDIwYTQ3YTQ0ZjA3NjJmNTM3Mjc5YjJmZWY4NGY4NzY4MTk4NTQwOTdkMiIsImlzRGRwaUVuYWJsZWQiOiJZIiwiaXNNdGZFbmFibGVkIjoiWSIsImZ5X2lkIjoiWUswNDM5MSIsImFwcFR5cGUiOjEwMSwiZXhwIjoxNzY4MjY0MjAwLCJpYXQiOjE3NjgyMDM5MTYsImlzcyI6ImFwaS5meWVycy5pbiIsIm5iZiI6MTc2ODIwMzkxNiwic3ViIjoiYWNjZXNzX3Rva2VuIn0.GRxdraCQNX0vYBYlTQs3YkRUINit-rZvuJqbHAUSM3E");
    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    
    if (!client_id || !access_token) {
        fprintf(stderr, "Missing client_id or access_token in session\n");
        return; 
    }

    fyers_model_t* model = fyers_model_create(
        client_id,
        access_token,
        false,
        NULL,
        FYERS_LOG_INFO
    );

    if (!model) {
        fprintf(stderr, "Failed to create model\n");
        return;
    }
    printf("Toggling fy_toggle_alert\n");
    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "alertId", "4115559");
    const char* params = cJSON_Print(json);

    fyers_response_t* response = fyers_model_toggle_alert(model, params);
    if (response) printf("fy_toggle_alert response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);

}

static fyers_model_t* create_model_from_session(fyers_session_t* session) {
    const char* client_id = fyers_session_get_client_id(session);
    const char* access_token = fyers_session_get_access_token(session);
    if (!client_id || !access_token) return NULL;
    return fyers_model_create(client_id, access_token, false, NULL, FYERS_LOG_INFO);
}

static void print_response(const char* name, fyers_response_t* response) {
    if (response)
        printf("%s response: %s\n", name, response->data ? response->data : "(null)");
    else
        printf("%s: (null response)\n", name);
}

// --- CreateSmartOrderLimit ---
// CreateSmartOrderLimitRequest: Symbol, Side, Qty, ProductType, LimitPrice, OrderType, EndTime, OnExp, (optional StopPrice, Hpr, Lpr, Mpp)
void example_create_smart_order_limit(fyers_session_t* session) {
    fyers_model_t* model = create_model_from_session(session);
    if (!model) return;
    cJSON* req = cJSON_CreateObject();
    cJSON_AddStringToObject(req, "symbol", "NSE:IDEA-EQ");
    cJSON_AddNumberToObject(req, "side", 1);
    cJSON_AddNumberToObject(req, "qty", 1);
    cJSON_AddStringToObject(req, "productType", "CNC");
    cJSON_AddNumberToObject(req, "limitPrice", 10.15);
    cJSON_AddNumberToObject(req, "orderType", 1);
    cJSON_AddNumberToObject(req, "endTime", 1771577294);
    cJSON_AddNumberToObject(req, "onExp", 2);
    char* req_str = cJSON_PrintUnformatted(req);
    fyers_response_t* response = fyers_model_create_smart_order_limit(model, req_str);
    print_response("createSmartOrderLimit", response);
    if (response) fyers_response_destroy(response);
    fyers_model_destroy(model);
    cJSON_free(req_str);
    cJSON_Delete(req);
}

// --- CreateSmartOrderStep ---
void example_create_smart_order_step(fyers_session_t* session) {
    fyers_model_t* model = create_model_from_session(session);
    if (!model) return;
    cJSON* req = cJSON_CreateObject();
    cJSON_AddStringToObject(req, "symbol", "NSE:TCS-EQ");
    cJSON_AddNumberToObject(req, "side", 1);
    cJSON_AddNumberToObject(req, "qty", 10);
    cJSON_AddStringToObject(req, "productType", "CNC");
    cJSON_AddNumberToObject(req, "initQty", 2);
    cJSON_AddNumberToObject(req, "avgqty", 2);
    cJSON_AddNumberToObject(req, "avgdiff", 5);
    cJSON_AddNumberToObject(req, "direction", 1);
    cJSON_AddNumberToObject(req, "limitPrice", 750);
    cJSON_AddNumberToObject(req, "orderType", 1);
    cJSON_AddNumberToObject(req, "startTime", 1771566494);
    cJSON_AddNumberToObject(req, "endTime", 1771577294);
    cJSON_AddNumberToObject(req, "hpr", 800);
    cJSON_AddNumberToObject(req, "lpr", 700);
    cJSON_AddNumberToObject(req, "mpp", 1);
    char* req_str = cJSON_PrintUnformatted(req);
    fyers_response_t* response = fyers_model_create_smart_order_step(model, req_str);
    print_response("createSmartOrderStep", response);
    if (response) fyers_response_destroy(response);
    fyers_model_destroy(model);
    cJSON_free(req_str);
    cJSON_Delete(req);
}

// --- CreateSmartOrderSIP ---
// Required: symbol, productType ("CNC"|"MTF"), freq (1=Daily,2=Weekly,3=Monthly,6=Quarterly), sip_day (1-28), qty OR amount.
// Conditional: sip_time (required if freq=1; Unix time within market hours).
// Optional: imd_start, endTime, hpr, lpr, step_up_freq (3|5), step_up_qty, step_up_amount, exp_qty.
void example_create_smart_order_sip(fyers_session_t* session) {
    fyers_model_t* model = create_model_from_session(session);
    if (!model) return;
    cJSON* req = cJSON_CreateObject();
    cJSON_AddStringToObject(req, "symbol", "NSE:SBIN-EQ");
    cJSON_AddNumberToObject(req, "side", 1);
    cJSON_AddStringToObject(req, "productType", "CNC");
    cJSON_AddNumberToObject(req, "freq", 3);           /* 1=Daily, 2=Weekly, 3=Monthly, 6=Quarterly */
    cJSON_AddNumberToObject(req, "sip_day", 15);       /* 1-28 */
    cJSON_AddNumberToObject(req, "amount", 5000);     /* qty OR amount required; max 999999 */
    /* sip_time: required when freq=1 (Daily); Unix timestamp within market hours. Omit for freq 2/3/6. */
    /* cJSON_AddNumberToObject(req, "sip_time", 1730000400); */
    cJSON_AddBoolToObject(req, "imd_start", false);
    cJSON_AddNumberToObject(req, "endTime", 0);       /* 0 = no end date; or Unix timestamp */
    cJSON_AddNumberToObject(req, "hpr", 900);         /* skip if price above */
    cJSON_AddNumberToObject(req, "lpr", 600);         /* skip if price below */
    cJSON_AddNumberToObject(req, "step_up_freq", 3);  /* 3 or 5; 0 = no step-up */
    cJSON_AddNumberToObject(req, "step_up_amount", 500);
    /* step_up_qty, exp_qty optional */
    char* req_str = cJSON_PrintUnformatted(req);
    fyers_response_t* response = fyers_model_create_smart_order_sip(model, req_str);
    print_response("createSmartOrderSIP", response);
    if (response) fyers_response_destroy(response);
    fyers_model_destroy(model);
    cJSON_free(req_str);
    cJSON_Delete(req);
}

// --- CreateSmartOrderTrail --- API: symbol, side, qty, productType, stopPrice, jump_diff, orderType, mpp
void example_create_smart_order_trail(fyers_session_t* session) {
    fyers_model_t* model = create_model_from_session(session);
    if (!model) return;
    cJSON* req = cJSON_CreateObject();
    cJSON_AddStringToObject(req, "symbol", "NSE:SBIN-EQ");
    cJSON_AddNumberToObject(req, "side", -1);
    cJSON_AddNumberToObject(req, "qty", 1);
    cJSON_AddStringToObject(req, "productType", "CNC");
    cJSON_AddNumberToObject(req, "stopPrice", 740);
    cJSON_AddNumberToObject(req, "jump_diff", 5);
    cJSON_AddNumberToObject(req, "orderType", 2);
    cJSON_AddNumberToObject(req, "mpp", 1);
    char* req_str = cJSON_PrintUnformatted(req);
    fyers_response_t* response = fyers_model_create_smart_order_trail(model, req_str);
    print_response("createSmartOrderTrail", response);
    if (response) fyers_response_destroy(response);
    fyers_model_destroy(model);
    cJSON_free(req_str);
    cJSON_Delete(req);
}

// --- ModifySmartOrder --- (use flowId of an active smart order; send only fields that apply to flow type)
void example_modify_smart_order(fyers_session_t* session) {
    fyers_model_t* model = create_model_from_session(session);
    if (!model) return;
    cJSON* req = cJSON_CreateObject();
    cJSON_AddStringToObject(req, "flowId", "fcc404d8-f15f-426b-849a-cd85e0454a34");
    cJSON_AddNumberToObject(req, "amount", 4000);
    cJSON_AddStringToObject(req, "productType", "CNC");
    char* req_str = cJSON_PrintUnformatted(req);
    fyers_response_t* response = fyers_model_modify_smart_order(model, req_str);
    print_response("modifySmartOrder", response);
    if (response) fyers_response_destroy(response);
    fyers_model_destroy(model);
    cJSON_free(req_str);
    cJSON_Delete(req);
}

// --- CancelSmartOrder ---
void example_cancel_smart_order(fyers_session_t* session) {
    fyers_model_t* model = create_model_from_session(session);
    if (!model) return;
    cJSON* req = cJSON_CreateObject();
    cJSON_AddStringToObject(req, "flowId", "fcc404d8-f15f-426b-849a-cd85e0454a34");
    char* req_str = cJSON_PrintUnformatted(req);
    fyers_response_t* response = fyers_model_cancel_smart_order(model, req_str);
    print_response("cancelSmartOrder", response);
    if (response) fyers_response_destroy(response);
    fyers_model_destroy(model);
    cJSON_free(req_str);
    cJSON_Delete(req);
}

// --- PauseSmartOrder ---
void example_pause_smart_order(fyers_session_t* session) {
    fyers_model_t* model = create_model_from_session(session);
    if (!model) return;
    cJSON* req = cJSON_CreateObject();
    cJSON_AddStringToObject(req, "flowId", "fcc404d8-f15f-426b-849a-cd85e0454a34");
    char* req_str = cJSON_PrintUnformatted(req);
    fyers_response_t* response = fyers_model_pause_smart_order(model, req_str);
    print_response("pauseSmartOrder", response);
    if (response) fyers_response_destroy(response);
    fyers_model_destroy(model);
    cJSON_free(req_str);
    cJSON_Delete(req);
}

// --- ResumeSmartOrder ---
void example_resume_smart_order(fyers_session_t* session) {
    fyers_model_t* model = create_model_from_session(session);
    if (!model) return;
    cJSON* req = cJSON_CreateObject();
    cJSON_AddStringToObject(req, "flowId", "fcc404d8-f15f-426b-849a-cd85e0454a34");
    char* req_str = cJSON_PrintUnformatted(req);
    fyers_response_t* response = fyers_model_resume_smart_order(model, req_str);
    print_response("resumeSmartOrder", response);
    if (response) fyers_response_destroy(response);
    fyers_model_destroy(model);
    cJSON_free(req_str);
    cJSON_Delete(req);
}

// --- GetSmartOrderBookWithFilter --- (pass NULL for no filter, or query string e.g. "exchange=NSE&page_no=1&page_size=20")
void example_get_smart_order_book(fyers_session_t* session) {
    fyers_model_t* model = create_model_from_session(session);
    if (!model) return;
    fyers_response_t* response = fyers_model_get_smart_order_book(model);
    print_response("getSmartOrderBookWithFilter", response);
    if (response) fyers_response_destroy(response);
    fyers_model_destroy(model);
}

// --- CreateSmartExitTrigger --- (type: 1=Only Alert, 2=Exit with Alert, 3=Exit+Wait for Recovery; waitTime required for type 3)
void example_create_smart_exit_trigger(fyers_session_t* session) {
    fyers_model_t* model = create_model_from_session(session);
    if (!model) return;
    cJSON* req = cJSON_CreateObject();
    cJSON_AddStringToObject(req, "name", "Auto Exit Strategy");
    cJSON_AddNumberToObject(req, "type", 2);
    cJSON_AddNumberToObject(req, "profitRate", 615.01);
    cJSON_AddNumberToObject(req, "lossRate", 0);
    char* req_str = cJSON_PrintUnformatted(req);
    fyers_response_t* response = fyers_model_create_smart_exit_trigger(model, req_str);
    print_response("createSmartExitTrigger", response);
    if (response) fyers_response_destroy(response);
    fyers_model_destroy(model);
    cJSON_free(req_str);
    cJSON_Delete(req);
}

// --- GetSmartExitTrigger --- (pass NULL for all, or flow_id for one)
void example_get_smart_exit_trigger(fyers_session_t* session) {
    fyers_model_t* model = create_model_from_session(session);
    if (!model) return;
    fyers_response_t* response = fyers_model_get_smart_exit_trigger(model);
    print_response("getSmartExitTrigger", response);
    if (response) fyers_response_destroy(response);
    fyers_model_destroy(model);
}

// --- UpdateSmartExitTrigger ---
void example_update_smart_exit_trigger(fyers_session_t* session) {
   
    fyers_model_t* model = create_model_from_session(session);
    if (!model) return;
    cJSON* req = cJSON_CreateObject();
    cJSON_AddStringToObject(req, "flowId", "9bb5eb72-8991-47e0-befb-7dacafba8668");
    cJSON_AddNumberToObject(req, "profitRate", 615.3);
    cJSON_AddNumberToObject(req, "lossRate", 614.90);
    cJSON_AddNumberToObject(req, "type", 3);
    cJSON_AddStringToObject(req, "name", "re-test");
    cJSON_AddNumberToObject(req, "waitTime", 5);
    char* req_str = cJSON_PrintUnformatted(req);
    fyers_response_t* response = fyers_model_update_smart_exit_trigger(model, req_str);
    print_response("updateSmartExitTrigger", response);
    if (response) fyers_response_destroy(response);
    fyers_model_destroy(model);
    cJSON_free(req_str);
    cJSON_Delete(req);
}

// --- ActivateDeactivateSmartExitTrigger ---
void example_activate_deactivate_smart_exit_trigger(fyers_session_t* session) {
   
    fyers_model_t* model = create_model_from_session(session);
    if (!model) return;
    cJSON* req = cJSON_CreateObject();
    cJSON_AddStringToObject(req, "flowId", "9bb5eb72-8991-47e0-befb-7dacafba8668");
    char* req_str = cJSON_PrintUnformatted(req);
    fyers_response_t* response = fyers_model_activate_deactivate_smart_exit_trigger(model, req_str);
    print_response("activateDeactivateSmartExitTrigger", response);
    if (response) fyers_response_destroy(response);
    fyers_model_destroy(model);
    cJSON_free(req_str);
    cJSON_Delete(req);
} 

void fy_get_orderbook_history(fyers_session_t* session) {
    fyers_model_t* model = create_model_from_session(session);
    if (!model) return;
    cJSON* req = cJSON_CreateObject();
    cJSON_AddStringToObject(req, "symbol", "NSE:MAZDOCK-EQ");
    char* req_str = cJSON_PrintUnformatted(req);
    printf("Getting fy_get_orderbook_by_order_id\n");
    fyers_response_t* response = fyers_model_get_order_history(model, req_str);
    if (response) printf("fy_get_orderbook_by_order_id response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

void fy_get_tradebook_history(fyers_session_t* session) {
    fyers_model_t* model = create_model_from_session(session);
    if (!model) return;
    cJSON* req = cJSON_CreateObject();
    cJSON_AddStringToObject(req, "symbol", "NSE:MAZDOCK-EQ");
    char* req_str = cJSON_PrintUnformatted(req);
    printf("Getting fy_get_tradebook_by_order_id\n");
    fyers_response_t* response = fyers_model_get_trade_history(model, req_str);
    if (response) printf("fy_get_tradebook_by_order_id response: %s\n", response->data ? response->data : "(null)");
    fyers_response_destroy(response);
    fyers_model_destroy(model);
}

int main() {
    // Replace with your app credentials
    // const char* client_id = "M0R4WW1PYU-100";
    const char* client_id = "";
    const char* redirect_uri = "https://trade.fyers.in/api-login/redirect-uri/index.html";
    const char* secret_key = "";
    const char* auth_code = "";
    const char* access_token = "";


    // Create session
    fyers_session_t* session = fyers_session_create(
        client_id,
        redirect_uri,
        secret_key
    );

    if (!session) {
        fprintf(stderr, "Failed to create session (check parameters or memory)\n");
        return 1;
    }

    fyers_session_set_access_token(session,access_token);

    // Authentication
    // fy_generate_authcode(session); // generate auth code
    // fy_generate_token(session, auth_code); // generate token
    // fy_generate_token_with_refresh_token(session, refresh_token, pin); // generate token with refresh token

    // User
    // fy_get_profile(session); // get profile
    // fy_get_funds(session); // get funds
    // fy_get_holdings(session); // get holdings
    // fy_logout(session); // logout

    // Transaction Info
    // fy_get_orderbook(session); // get orderbook
    // fy_get_orderbook_by_order_ids(session); // get orderbook by order ids
    // fy_get_orderbook_by_order_tag(session); // get orderbook by order tag
    // fy_get_positions(session); // get positions
    // fy_get_tradebook(session); // get tradebook
    // fy_get_tradebook_by_order_tag(session); // get tradebook by order tag


    //reports APIS
    // fy_get_orderbook_history(session); // get orderbook history
    // fy_get_tradebook_history(session); // get tradebook history

    // Order Placement
    // fy_place_order(session); // place order
    // fy_multi_order(session); // place multi order
    // fy_place_multileg_order(session); // place multileg order

    // GTT Orders
    // fy_place_gtt_order(session); // place gtt order
    // fy_place_oco_gtt_order(session); // place gtt oco order
    // fy_get_gtt_orderbook(session); // get gtt orderbook
    // fy_modify_gtt_order(session); // modify gtt order
    // fy_cancel_gtt_order(session); // cancel gtt order

    // Other Transactions
    // fy_modify_order(session); // modify order
    // fy_modify_mutli_order(session); // modify mutli order
    // fy_cancel_order(session); // cancel order
    // fy_cancel_mutli_order(session); // cancel order
    // fy_exit_all_positions(session); // exit positions
    // fy_exit_position_by_segment(session); // exit all positions
    // fy_exit_position_by_id(session); // exit position by Segment Side & productType
    // fy_pending_order_cancel(session); // pending order cancel
    // fy_convert_position(session); // convert position
    
    // Broker Config
    // fy_get_broker_config(session); // get broker config

    // Market Data
    // fy_get_history(session); // get history
    // fy_get_quotes(session); // get quotes
    // fy_get_depth(session); // get depth
    //fy_get_option_chain(session); // get option chain

    // Pirce Alerts
    // fy_create_alert(session); // create alert
    // fy_get_alert(session); // get alert
    // fy_update_alert(session); // update alert
    // fy_delete_alert(session); // delete alert
    // fy_toggle_alert(session); // toggle alert


    // example_create_smart_order_limit(session);
    // example_create_smart_order_step(session);
    //  example_create_smart_order_sip(session);
    // example_create_smart_order_trail(session);
    // example_modify_smart_order(session);
    // example_cancel_smart_order(session);
    //  example_pause_smart_order(session);
    // example_resume_smart_order(session);
    // example_get_smart_order_book(session);
    // example_create_smart_exit_trigger(session);
   // example_get_smart_exit_trigger(session);
   // example_update_smart_exit_trigger(session);   /* active example, same as Go snippet */
   // example_activate_deactivate_smart_exit_trigger(session);

    fyers_session_destroy(session);
    fyers_cleanup();
    return 0;
}
