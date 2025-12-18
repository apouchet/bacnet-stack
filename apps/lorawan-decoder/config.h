/**
 * @file
 * @brief Configuration module for LoRaWAN MQTT Decoder Service
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef LORAWAN_DECODER_CONFIG_H
#define LORAWAN_DECODER_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

/* Default configuration paths */
#define CONFIG_SENSORS_MAP_PATH "/var/config/scada/sensors_map.json"
#define CONFIG_SENSORS_DECODER_PATH "/var/config/scada/sensors/lora"
#define CONFIG_SENSORS_DECODER_PATH_ALT "/etc/scada/sensors/lora"

/* MQTT defaults */
#define MQTT_DEFAULT_HOST "localhost"
#define MQTT_DEFAULT_PORT 1883
#define MQTT_DEFAULT_CLIENT_ID "lorawan-decoder"
#define MQTT_DEFAULT_KEEPALIVE 60
#define MQTT_DEFAULT_QOS 0

/* MQTT subscribe topics */
#define MQTT_TOPIC_UPLINK "lora/+/+/+/up"
#define MQTT_TOPIC_DOWNLINK "lora/+/+/+/down"

/* MQTT publish topic base */
#define MQTT_TOPIC_PUBLISH_BASE "scada/lorawan"

/* Logging defaults */
#define LOG_DEFAULT_LEVEL 1 /* INFO */

/**
 * @brief MQTT configuration structure
 */
typedef struct mqtt_config {
    char host[256];
    uint16_t port;
    char username[128];
    char password[128];
    char client_id[128];
    int keepalive;
    int qos;
    bool use_tls;
    char ca_cert[512];
    char client_cert[512];
    char client_key[512];
} mqtt_config_t;

/**
 * @brief Paths configuration structure
 */
typedef struct paths_config {
    char sensors_map[512];
    char sensors_decoder[512];
    char sensors_decoder_alt[512];
} paths_config_t;

/**
 * @brief Logging configuration structure
 */
typedef struct log_config {
    int level;  /* 0=DEBUG, 1=INFO, 2=WARNING, 3=ERROR */
    bool json_output;
    char log_file[512];
} log_config_t;

/**
 * @brief Main application configuration structure
 */
typedef struct app_config {
    mqtt_config_t mqtt;
    paths_config_t paths;
    log_config_t logging;
    bool foreground;
} app_config_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize configuration with defaults
 * @param config Pointer to configuration structure to initialize
 */
void config_init_defaults(app_config_t *config);

/**
 * @brief Parse command line arguments
 * @param config Pointer to configuration structure to update
 * @param argc Argument count
 * @param argv Argument values
 * @return true on success, false on failure or if help was requested
 */
bool config_parse_args(app_config_t *config, int argc, char *argv[]);

/**
 * @brief Print configuration to stdout (for debugging)
 * @param config Pointer to configuration structure
 */
void config_print(const app_config_t *config);

/**
 * @brief Print usage/help information
 * @param program_name Name of the program (argv[0])
 */
void config_print_usage(const char *program_name);

#ifdef __cplusplus
}
#endif

#endif /* LORAWAN_DECODER_CONFIG_H */
