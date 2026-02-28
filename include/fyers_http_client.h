/**
 * @file fyers_http_client.h
 * @brief HTTP client interface
 */

#ifndef FYERS_HTTP_CLIENT_H
#define FYERS_HTTP_CLIENT_H

#include "fyers_api.h"
#include "fyers_logger.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fyers_http_client fyers_http_client_t;

FYERS_API fyers_http_client_t* fyers_http_client_create(fyers_logger_t* api_logger,
                                               fyers_logger_t* request_logger);
FYERS_API void fyers_http_client_destroy(fyers_http_client_t* client);
FYERS_API fyers_response_t* fyers_http_client_get(fyers_http_client_t* client,
                                         const char* url,
                                         const char* header);
FYERS_API fyers_response_t* fyers_http_client_post(fyers_http_client_t* client,
                                          const char* url,
                                          const char* header,
                                          const char* data);
FYERS_API fyers_response_t* fyers_http_client_patch(fyers_http_client_t* client,
                                           const char* url,
                                           const char* header,
                                           const char* data);
FYERS_API fyers_response_t* fyers_http_client_delete(fyers_http_client_t* client,
                                            const char* url,
                                            const char* header,
                                            const char* data);
FYERS_API fyers_response_t* fyers_http_client_put(fyers_http_client_t* client,
                                         const char* url,
                                         const char* header,
                                         const char* data);

#ifdef __cplusplus
}
#endif

#endif // FYERS_HTTP_CLIENT_H

