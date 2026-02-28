/**
 * @file fyers_logger.h
 * @brief Logging functionality for Fyers API
 */

#ifndef FYERS_LOGGER_H
#define FYERS_LOGGER_H

#include "fyers_api.h"
// #include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fyers_logger fyers_logger_t;

/**
 * @brief Create a new logger instance
 * @param service_name Service name for logging
 * @param level Log level
 * @param log_path Path to log file (NULL for stdout)
 * @return Logger instance or NULL on error
 */
FYERS_API fyers_logger_t* fyers_logger_create(const char* service_name, 
                                     fyers_log_level_t level,
                                     const char* log_path);

/**
 * @brief Destroy logger instance
 * @param logger Logger instance
 */
FYERS_API void fyers_logger_destroy(fyers_logger_t* logger);

/**
 * @brief Log error message
 */
FYERS_API void fyers_logger_error(fyers_logger_t* logger, const char* format, ...);

/**
 * @brief Log warning message
 */
FYERS_API void fyers_logger_warning(fyers_logger_t* logger, const char* format, ...);

/**
 * @brief Log info message
 */
FYERS_API void fyers_logger_info(fyers_logger_t* logger, const char* format, ...);

/**
 * @brief Log debug message
 */
FYERS_API void fyers_logger_debug(fyers_logger_t* logger, const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif // FYERS_LOGGER_H

