/**
 * @file
 * @brief Logging module implementation for LoRaWAN MQTT Decoder Service
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "logging.h"

/* Module state */
static int g_log_level = LOG_LEVEL_INFO;
static bool g_json_output = false;
static FILE *g_log_file = NULL;
static pthread_mutex_t g_log_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Log level names */
static const char *level_names[] = {
    "DEBUG",
    "INFO",
    "WARNING",
    "ERROR"
};

/* JSON-safe escaping for strings */
static void json_escape_string(const char *input, char *output, size_t output_size)
{
    size_t i, j = 0;
    
    if (!input || !output || output_size == 0) {
        return;
    }
    
    for (i = 0; input[i] != '\0' && j < output_size - 1; i++) {
        switch (input[i]) {
            case '"':
            case '\\':
                if (j < output_size - 2) {
                    output[j++] = '\\';
                    output[j++] = input[i];
                }
                break;
            case '\n':
                if (j < output_size - 2) {
                    output[j++] = '\\';
                    output[j++] = 'n';
                }
                break;
            case '\r':
                if (j < output_size - 2) {
                    output[j++] = '\\';
                    output[j++] = 'r';
                }
                break;
            case '\t':
                if (j < output_size - 2) {
                    output[j++] = '\\';
                    output[j++] = 't';
                }
                break;
            default:
                output[j++] = input[i];
                break;
        }
    }
    output[j] = '\0';
}

/**
 * @brief Initialize logging subsystem
 */
void logging_init(int level, bool json_output, const char *log_file)
{
    g_log_level = level;
    g_json_output = json_output;
    
    if (log_file && strlen(log_file) > 0) {
        g_log_file = fopen(log_file, "a");
        if (!g_log_file) {
            fprintf(stderr, "Failed to open log file: %s\n", log_file);
            g_log_file = stdout;
        }
    } else {
        g_log_file = stdout;
    }
}

/**
 * @brief Shutdown logging subsystem
 */
void logging_shutdown(void)
{
    if (g_log_file && g_log_file != stdout && g_log_file != stderr) {
        fclose(g_log_file);
    }
    g_log_file = NULL;
}

/**
 * @brief Set log level
 */
void logging_set_level(int level)
{
    g_log_level = level;
}

/**
 * @brief Get current log level
 */
int logging_get_level(void)
{
    return g_log_level;
}

/**
 * @brief Get basename of file path
 */
static const char *get_basename(const char *path)
{
    const char *base = strrchr(path, '/');
    return base ? base + 1 : path;
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
    char message[4096];
    char timestamp[64];
    time_t now;
    struct tm *tm_info;
    FILE *output;
    
    if (level < g_log_level) {
        return;
    }
    
    /* Format message */
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);
    
    /* Get timestamp */
    now = time(NULL);
    tm_info = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    /* Thread-safe output */
    pthread_mutex_lock(&g_log_mutex);
    
    output = g_log_file ? g_log_file : stdout;
    
    if (g_json_output) {
        char escaped_message[4096];
        json_escape_string(message, escaped_message, sizeof(escaped_message));
        fprintf(output, 
            "{\"timestamp\":\"%s\",\"level\":\"%s\",\"file\":\"%s\","
            "\"line\":%d,\"message\":\"%s\"}\n",
            timestamp, level_names[level], get_basename(file), 
            line, escaped_message);
    } else {
        fprintf(output, "[%s] [%s] %s:%d - %s\n",
            timestamp, level_names[level], get_basename(file), 
            line, message);
    }
    
    fflush(output);
    
    pthread_mutex_unlock(&g_log_mutex);
}
