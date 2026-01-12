/**
 * @file
 * @brief Logging module implementation for BACnet MQTT Server
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include "logging.h"

/* Module state */
static int current_level = LOG_LEVEL_INFO;
static bool json_mode = false;
static FILE *log_file = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Level names for output */
static const char *level_names[] = { "DEBUG", "INFO", "WARNING", "ERROR" };

/**
 * @brief Initialize logging subsystem
 */
void logging_init(int level, bool json_output, const char *filepath)
{
    pthread_mutex_lock(&log_mutex);

    current_level = level;
    json_mode = json_output;

    if (filepath && strlen(filepath) > 0) {
        log_file = fopen(filepath, "a");
        if (!log_file) {
            fprintf(stderr, "Warning: Cannot open log file: %s\n", filepath);
            log_file = stdout;
        }
    } else {
        log_file = stdout;
    }

    pthread_mutex_unlock(&log_mutex);
}

/**
 * @brief Shutdown logging subsystem
 */
void logging_shutdown(void)
{
    pthread_mutex_lock(&log_mutex);

    if (log_file && log_file != stdout && log_file != stderr) {
        fclose(log_file);
    }
    log_file = NULL;

    pthread_mutex_unlock(&log_mutex);
}

/**
 * @brief Set log level at runtime
 */
void logging_set_level(int level)
{
    pthread_mutex_lock(&log_mutex);
    current_level = level;
    pthread_mutex_unlock(&log_mutex);
}

/**
 * @brief Get current log level
 */
int logging_get_level(void)
{
    int level;
    pthread_mutex_lock(&log_mutex);
    level = current_level;
    pthread_mutex_unlock(&log_mutex);
    return level;
}

/**
 * @brief Extract filename from path
 */
static const char *basename_const(const char *path)
{
    const char *last_slash = strrchr(path, '/');
    if (last_slash) {
        return last_slash + 1;
    }
    return path;
}

/**
 * @brief Log a message
 */
void logging_log(
    int level,
    const char *file,
    int line,
    const char *fmt,
    ...)
{
    va_list args;
    time_t now;
    struct tm *tm_info;
    char timestamp[32];
    char message[1024];
    FILE *output;

    /* Check level */
    pthread_mutex_lock(&log_mutex);
    if (level < current_level) {
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    output = log_file ? log_file : stdout;

    /* Get timestamp */
    time(&now);
    tm_info = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    /* Format message */
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    /* Output based on mode */
    if (json_mode) {
        fprintf(
            output,
            "{\"timestamp\":\"%s\",\"level\":\"%s\",\"file\":\"%s\","
            "\"line\":%d,\"message\":\"%s\"}\n",
            timestamp,
            (level >= 0 && level <= 3) ? level_names[level] : "UNKNOWN",
            basename_const(file), line, message);
    } else {
        fprintf(
            output, "[%s] [%s] %s:%d: %s\n", timestamp,
            (level >= 0 && level <= 3) ? level_names[level] : "UNKNOWN",
            basename_const(file), line, message);
    }

    fflush(output);
    pthread_mutex_unlock(&log_mutex);
}
