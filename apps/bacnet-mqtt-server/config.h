/**
 * @file
 * @brief Configuration module for BACnet MQTT Server
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_MQTT_CONFIG_H
#define BACNET_MQTT_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

/* Default configuration file path */
#define CONFIG_DEFAULT_PATH "/etc/bacnet-mqtt-server/config.yaml"

/* Default paths for sensor configuration */
#define SENSORS_MAP_DEFAULT_PATH "/var/config/scada/sensors_maps.json"
#define SENSORS_DEF_BASE_PATH "/var/config/scada/sensors"

/* MQTT defaults */
#define MQTT_DEFAULT_HOST "localhost"
#define MQTT_DEFAULT_PORT 1883
#define MQTT_DEFAULT_TOPIC "scada/lorawan/#"
#define MQTT_DEFAULT_CLIENT_ID "bacnet-mqtt-server"
#define MQTT_DEFAULT_KEEPALIVE 60
#define MQTT_DEFAULT_QOS 1

/* BACnet defaults */
#define BACNET_DEFAULT_DEVICE_INSTANCE 260001
#define BACNET_DEFAULT_UDP_PORT 47808

/* Logging defaults */
#define LOG_DEFAULT_LEVEL 2 /* INFO */

/**
 * @brief MQTT configuration structure
 */
typedef struct mqtt_config {
    char host[256];
    uint16_t port;
    char username[128];
    char password[128];
    char client_id[128];
    char topic[256];
    int keepalive;
    int qos;
    bool use_tls;
    char ca_cert[512];
    char client_cert[512];
    char client_key[512];
} mqtt_config_t;

/**
 * @brief BACnet configuration structure
 */
typedef struct bacnet_config {
    uint32_t device_instance;
    uint16_t udp_port;
    char network_interface[64];
    char device_name[64];
    char vendor_name[64];
    uint16_t vendor_id;
    char model_name[64];
    char app_version[32];
    char description[128];
    char location[128];
} bacnet_config_t;

/**
 * @brief Paths configuration structure
 */
typedef struct paths_config {
    char sensors_maps[512];
    char sensors_base[512];
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
    bacnet_config_t bacnet;
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
 * @brief Load configuration from YAML file
 * @param config Pointer to configuration structure to populate
 * @param filepath Path to the YAML configuration file
 * @return true on success, false on failure
 */
bool config_load(app_config_t *config, const char *filepath);

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

#endif /* BACNET_MQTT_CONFIG_H */
