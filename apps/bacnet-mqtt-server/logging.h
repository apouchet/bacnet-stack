/**
 * @file
 * @brief Logging module for BACnet MQTT Server
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_MQTT_LOGGING_H
#define BACNET_MQTT_LOGGING_H

#include <stdbool.h>

/* Log levels */
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_ERROR 3

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the logging subsystem
 * @param level Minimum log level to output
 * @param json_output If true, output logs in JSON format
 * @param log_file Optional log file path (NULL for stdout)
 */
void logging_init(int level, bool json_output, const char *log_file);

/**
 * @brief Shutdown the logging subsystem
 */
void logging_shutdown(void);

/**
 * @brief Set the log level at runtime
 * @param level New log level
 */
void logging_set_level(int level);

/**
 * @brief Get the current log level
 * @return Current log level
 */
int logging_get_level(void);

/**
 * @brief Log a message at the specified level
 * @param level Log level
 * @param file Source file name
 * @param line Source line number
 * @param fmt Printf-style format string
 * @param ... Format arguments
 */
void logging_log(
    int level,
    const char *file,
    int line,
    const char *fmt,
    ...) __attribute__((format(printf, 4, 5)));

/* Convenience macros */
#define LOG_DEBUG(fmt, ...) \
    logging_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_INFO(fmt, ...) \
    logging_log(LOG_LEVEL_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_WARNING(fmt, ...) \
    logging_log(LOG_LEVEL_WARNING, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_ERROR(fmt, ...) \
    logging_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* BACNET_MQTT_LOGGING_H */
