/**
 * @file
 * @brief Configuration module implementation for LoRaWAN MQTT Decoder Service
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "config.h"
#include "logging.h"

/* Application version */
#define APP_VERSION "1.0.0"

/**
 * @brief Initialize configuration with default values
 */
void config_init_defaults(app_config_t *config)
{
    if (!config) {
        return;
    }

    memset(config, 0, sizeof(app_config_t));

    /* MQTT defaults */
    strncpy(config->mqtt.host, MQTT_DEFAULT_HOST, sizeof(config->mqtt.host) - 1);
    config->mqtt.port = MQTT_DEFAULT_PORT;
    strncpy(config->mqtt.client_id, MQTT_DEFAULT_CLIENT_ID, 
            sizeof(config->mqtt.client_id) - 1);
    config->mqtt.keepalive = MQTT_DEFAULT_KEEPALIVE;
    config->mqtt.qos = MQTT_DEFAULT_QOS;
    config->mqtt.use_tls = false;

    /* Paths defaults */
    strncpy(config->paths.sensors_map, CONFIG_SENSORS_MAP_PATH, 
            sizeof(config->paths.sensors_map) - 1);
    strncpy(config->paths.sensors_decoder, CONFIG_SENSORS_DECODER_PATH, 
            sizeof(config->paths.sensors_decoder) - 1);
    strncpy(config->paths.sensors_decoder_alt, CONFIG_SENSORS_DECODER_PATH_ALT, 
            sizeof(config->paths.sensors_decoder_alt) - 1);

    /* Logging defaults */
    config->logging.level = LOG_DEFAULT_LEVEL;
    config->logging.json_output = false;

    config->foreground = true;
}

/**
 * @brief Print usage information
 */
void config_print_usage(const char *program_name)
{
    printf("LoRaWAN MQTT Decoder Service v%s\n\n", APP_VERSION);
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    printf("Options:\n");
    printf("  -h, --host HOST         MQTT broker host (default: %s)\n", 
           MQTT_DEFAULT_HOST);
    printf("  -p, --port PORT         MQTT broker port (default: %u)\n", 
           MQTT_DEFAULT_PORT);
    printf("  -u, --username USER     MQTT username\n");
    printf("  -P, --password PASS     MQTT password\n");
    printf("  -i, --client-id ID      MQTT client ID (default: %s)\n", 
           MQTT_DEFAULT_CLIENT_ID);
    printf("  -m, --sensors-map PATH  Path to sensors_map.json\n");
    printf("  -d, --decoders PATH     Path to decoder files directory\n");
    printf("  -l, --log-level LEVEL   Log level: debug, info, warning, error\n");
    printf("  -j, --json-output       Output logs in JSON format\n");
    printf("  -f, --foreground        Run in foreground (default)\n");
    printf("  -v, --version           Print version and exit\n");
    printf("  -?, --help              Print this help message\n\n");
    printf("Topics:\n");
    printf("  Subscribe: lora/+/+/+/up, lora/+/+/+/down\n");
    printf("  Publish:   scada/lorawan/{deveui}/up, scada/lorawan/{deveui}/down\n\n");
}

/**
 * @brief Parse command line arguments
 */
bool config_parse_args(app_config_t *config, int argc, char *argv[])
{
    int opt;
    int option_index = 0;

    static struct option long_options[] = {
        {"host", required_argument, 0, 'h'},
        {"port", required_argument, 0, 'p'},
        {"username", required_argument, 0, 'u'},
        {"password", required_argument, 0, 'P'},
        {"client-id", required_argument, 0, 'i'},
        {"sensors-map", required_argument, 0, 'm'},
        {"decoders", required_argument, 0, 'd'},
        {"log-level", required_argument, 0, 'l'},
        {"json-output", no_argument, 0, 'j'},
        {"foreground", no_argument, 0, 'f'},
        {"version", no_argument, 0, 'v'},
        {"help", no_argument, 0, '?'},
        {0, 0, 0, 0}
    };

    if (!config) {
        return false;
    }

    while ((opt = getopt_long(argc, argv, "h:p:u:P:i:m:d:l:jfv?", 
                              long_options, &option_index)) != -1) {
        switch (opt) {
            case 'h':
                strncpy(config->mqtt.host, optarg, 
                        sizeof(config->mqtt.host) - 1);
                break;
            case 'p':
                config->mqtt.port = (uint16_t)atoi(optarg);
                break;
            case 'u':
                strncpy(config->mqtt.username, optarg, 
                        sizeof(config->mqtt.username) - 1);
                break;
            case 'P':
                strncpy(config->mqtt.password, optarg, 
                        sizeof(config->mqtt.password) - 1);
                break;
            case 'i':
                strncpy(config->mqtt.client_id, optarg, 
                        sizeof(config->mqtt.client_id) - 1);
                break;
            case 'm':
                strncpy(config->paths.sensors_map, optarg, 
                        sizeof(config->paths.sensors_map) - 1);
                break;
            case 'd':
                strncpy(config->paths.sensors_decoder, optarg, 
                        sizeof(config->paths.sensors_decoder) - 1);
                break;
            case 'l':
                if (strcmp(optarg, "debug") == 0) {
                    config->logging.level = LOG_LEVEL_DEBUG;
                } else if (strcmp(optarg, "info") == 0) {
                    config->logging.level = LOG_LEVEL_INFO;
                } else if (strcmp(optarg, "warning") == 0) {
                    config->logging.level = LOG_LEVEL_WARNING;
                } else if (strcmp(optarg, "error") == 0) {
                    config->logging.level = LOG_LEVEL_ERROR;
                }
                break;
            case 'j':
                config->logging.json_output = true;
                break;
            case 'f':
                config->foreground = true;
                break;
            case 'v':
                printf("lorawan-decoder v%s\n", APP_VERSION);
                return false;
            case '?':
            default:
                config_print_usage(argv[0]);
                return false;
        }
    }

    return true;
}

/**
 * @brief Print current configuration
 */
void config_print(const app_config_t *config)
{
    if (!config) {
        return;
    }

    printf("Configuration:\n");
    printf("  MQTT:\n");
    printf("    Host: %s\n", config->mqtt.host);
    printf("    Port: %u\n", config->mqtt.port);
    printf("    Client ID: %s\n", config->mqtt.client_id);
    printf("    TLS: %s\n", config->mqtt.use_tls ? "enabled" : "disabled");
    printf("  Paths:\n");
    printf("    Sensors Map: %s\n", config->paths.sensors_map);
    printf("    Decoders: %s\n", config->paths.sensors_decoder);
    printf("  Logging:\n");
    printf("    Level: %d\n", config->logging.level);
    printf("    JSON output: %s\n", 
           config->logging.json_output ? "true" : "false");
}
