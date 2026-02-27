/**
 * @file fyers_session.h
 * @brief Session management and OAuth authentication
 */

#ifndef FYERS_SESSION_H
#define FYERS_SESSION_H

#include "fyers_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a new session instance
 * @param client_id Application client ID
 * @param redirect_uri Redirect URI registered with the app
 * @param secret_key Application secret key
 * @return Session instance or NULL on error
 * 
 * Note: Uses default values for response_type ("code"), state ("sample_state"), 
 *       and grant_type ("authorization_code")
 */
FYERS_API fyers_session_t* fyers_session_create(const char* client_id,
                                       const char* redirect_uri,
                                       const char* secret_key);

/**
 * @brief Destroy session instance
 * @param session Session instance
 */
FYERS_API void fyers_session_destroy(fyers_session_t* session);

/**
 * @brief Generate authentication URL
 * @param session Session instance
 * @param url_buffer Buffer to store the URL
 * @param buffer_size Size of the buffer
 * @return FYERS_OK on success
 */
FYERS_API fyers_error_t generate_authcode(fyers_session_t* session);

/**
 * @brief Set auth code received from OAuth callback
 * @param session Session instance
 * @param auth_code Auth code from OAuth callback
 * @return FYERS_OK on success
 */
FYERS_API fyers_error_t fyers_session_set_authcode(fyers_session_t* session,
                                          const char* auth_code);

/**
 * @brief Set access token manually
 * @param session Session instance
 * @param access_token Access token string
 * @return FYERS_OK on success
 */
FYERS_API fyers_error_t fyers_session_set_access_token(fyers_session_t* session,
                                             const char* access_token);

/**
 * @brief Generate access token from auth code
 * @param session Session instance
 * @return API response
 */
FYERS_API fyers_response_t* generate_token(fyers_session_t* session);
/**
 * @brief Generate access token from refresh token
 * @param session Session instance
 * @param refresh_token Refresh token
 * @param pin User PIN
 * @return API response (caller must call fyers_response_destroy), or NULL on error (invalid params or failed to call API)
 */
FYERS_API fyers_response_t* generate_token_with_refresh_token(fyers_session_t* session, const char* refresh_token, const char* pin);

/**
 * @brief Get client ID from session
 * @param session Session instance
 * @return Client ID string or NULL
 */
FYERS_API const char* fyers_session_get_client_id(fyers_session_t* session);

/**
 * @brief Get access token from session
 * @param session Session instance
 * @return Access token string or NULL if not generated yet
 */
FYERS_API const char* fyers_session_get_access_token(fyers_session_t* session);

#ifdef __cplusplus
}
#endif

#endif // FYERS_SESSION_H

