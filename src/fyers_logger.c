/**
 * @file fyers_logger.c
 * @brief Logging implementation
 */

#include "fyers_logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>

struct fyers_logger {
    char* service_name;
    fyers_log_level_t level;
    FILE* log_file;
    pthread_mutex_t mutex;
};

static const char* log_level_strings[] = {
    "ERROR",
    "WARNING",
    "INFO",
    "DEBUG"
};

fyers_logger_t* fyers_logger_create(const char* service_name,
                                     fyers_log_level_t level,
                                     const char* log_path) {
    if (!service_name) {
        return NULL;
    }

    fyers_logger_t* logger = (fyers_logger_t*)malloc(sizeof(fyers_logger_t));
    if (!logger) {
        return NULL;
    }

    logger->service_name = strdup(service_name);
    logger->level = level;
    logger->log_file = NULL;

    if (pthread_mutex_init(&logger->mutex, NULL) != 0) {
        free(logger->service_name);
        free(logger);
        return NULL;
    }

    if (log_path) {
        char log_file_path[512];
        snprintf(log_file_path, sizeof(log_file_path), "%s/%s.log",
                 log_path, service_name);
        logger->log_file = fopen(log_file_path, "a");
    } else {
        logger->log_file = stdout;
    }

    if (!logger->log_file) {
        logger->log_file = stdout;
    }

    return logger;
}

void fyers_logger_destroy(fyers_logger_t* logger) {
    if (!logger) {
        return;
    }

    pthread_mutex_lock(&logger->mutex);

    if (logger->log_file && logger->log_file != stdout) {
        fclose(logger->log_file);
    }

    pthread_mutex_unlock(&logger->mutex);
    pthread_mutex_destroy(&logger->mutex);

    free(logger->service_name);
    free(logger);
}

static void fyers_logger_log(fyers_logger_t* logger,
                              fyers_log_level_t level,
                              const char* format,
                              va_list args) {
    if (!logger || level > logger->level) {
        return;
    }

    pthread_mutex_lock(&logger->mutex);

    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(logger->log_file, "[%s] [%s] [%s] ",
            timestamp, log_level_strings[level], logger->service_name);
    vfprintf(logger->log_file, format, args);
    fprintf(logger->log_file, "\n");
    fflush(logger->log_file);

    pthread_mutex_unlock(&logger->mutex);
}

void fyers_logger_error(fyers_logger_t* logger, const char* format, ...) {
    va_list args;
    va_start(args, format);
    fyers_logger_log(logger, FYERS_LOG_ERROR, format, args);
    va_end(args);
}

void fyers_logger_warning(fyers_logger_t* logger, const char* format, ...) {
    va_list args;
    va_start(args, format);
    fyers_logger_log(logger, FYERS_LOG_WARNING, format, args);
    va_end(args);
}

void fyers_logger_info(fyers_logger_t* logger, const char* format, ...) {
    va_list args;
    va_start(args, format);
    fyers_logger_log(logger, FYERS_LOG_INFO, format, args);
    va_end(args);
}

void fyers_logger_debug(fyers_logger_t* logger, const char* format, ...) {
    va_list args;
    va_start(args, format);
    fyers_logger_log(logger, FYERS_LOG_DEBUG, format, args);
    va_end(args);
}

