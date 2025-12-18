/**
 * @file
 * @brief Configuration module implementation for BACnet MQTT Server
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <yaml.h>
#include "config.h"
#include "logging.h"

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
    strncpy(
        config->mqtt.client_id, MQTT_DEFAULT_CLIENT_ID,
        sizeof(config->mqtt.client_id) - 1);
    strncpy(config->mqtt.topic, MQTT_DEFAULT_TOPIC, sizeof(config->mqtt.topic) - 1);
    config->mqtt.keepalive = MQTT_DEFAULT_KEEPALIVE;
    config->mqtt.qos = MQTT_DEFAULT_QOS;
    config->mqtt.use_tls = false;

    /* BACnet defaults */
    config->bacnet.device_instance = BACNET_DEFAULT_DEVICE_INSTANCE;
    config->bacnet.udp_port = BACNET_DEFAULT_UDP_PORT;
    strncpy(
        config->bacnet.vendor_name, "BACnet MQTT Gateway",
        sizeof(config->bacnet.vendor_name) - 1);
    config->bacnet.vendor_id = 0; /* Use 0 for unassigned vendor ID */
    strncpy(
        config->bacnet.model_name, "MQTT-Server",
        sizeof(config->bacnet.model_name) - 1);
    strncpy(config->bacnet.app_version, "1.0.0", sizeof(config->bacnet.app_version) - 1);
    strncpy(
        config->bacnet.device_name, "BACnet-MQTT-Server",
        sizeof(config->bacnet.device_name) - 1);

    /* Paths defaults */
    strncpy(
        config->paths.sensors_map, SENSORS_MAP_DEFAULT_PATH,
        sizeof(config->paths.sensors_map) - 1);
    strncpy(
        config->paths.sensors_base, SENSORS_DEF_BASE_PATH,
        sizeof(config->paths.sensors_base) - 1);

    /* Logging defaults */
    config->logging.level = LOG_DEFAULT_LEVEL;
    config->logging.json_output = false;

    /* Runtime defaults */
    config->foreground = false;
}

/**
 * @brief Helper to safely copy a YAML value to a string field
 */
static void yaml_copy_string(
    char *dest,
    size_t dest_size,
    const char *src)
{
    if (dest && src && dest_size > 0) {
        strncpy(dest, src, dest_size - 1);
        dest[dest_size - 1] = '\0';
    }
}

/**
 * @brief Parse MQTT section from YAML
 */
static void parse_mqtt_section(
    app_config_t *config,
    yaml_document_t *doc,
    yaml_node_t *node)
{
    yaml_node_pair_t *pair;

    if (!config || !doc || !node || node->type != YAML_MAPPING_NODE) {
        return;
    }

    for (pair = node->data.mapping.pairs.start;
         pair < node->data.mapping.pairs.top; pair++) {
        yaml_node_t *key = yaml_document_get_node(doc, pair->key);
        yaml_node_t *value = yaml_document_get_node(doc, pair->value);

        if (!key || !value || key->type != YAML_SCALAR_NODE) {
            continue;
        }

        const char *key_str = (const char *)key->data.scalar.value;
        const char *val_str =
            (value->type == YAML_SCALAR_NODE)
                ? (const char *)value->data.scalar.value
                : NULL;

        if (!val_str) {
            continue;
        }

        if (strcmp(key_str, "host") == 0) {
            yaml_copy_string(config->mqtt.host, sizeof(config->mqtt.host), val_str);
        } else if (strcmp(key_str, "port") == 0) {
            config->mqtt.port = (uint16_t)atoi(val_str);
        } else if (strcmp(key_str, "username") == 0) {
            yaml_copy_string(
                config->mqtt.username, sizeof(config->mqtt.username), val_str);
        } else if (strcmp(key_str, "password") == 0) {
            yaml_copy_string(
                config->mqtt.password, sizeof(config->mqtt.password), val_str);
        } else if (strcmp(key_str, "client_id") == 0) {
            yaml_copy_string(
                config->mqtt.client_id, sizeof(config->mqtt.client_id), val_str);
        } else if (strcmp(key_str, "topic") == 0) {
            yaml_copy_string(config->mqtt.topic, sizeof(config->mqtt.topic), val_str);
        } else if (strcmp(key_str, "keepalive") == 0) {
            config->mqtt.keepalive = atoi(val_str);
        } else if (strcmp(key_str, "qos") == 0) {
            config->mqtt.qos = atoi(val_str);
        } else if (strcmp(key_str, "use_tls") == 0) {
            config->mqtt.use_tls =
                (strcmp(val_str, "true") == 0 || strcmp(val_str, "1") == 0);
        } else if (strcmp(key_str, "ca_cert") == 0) {
            yaml_copy_string(
                config->mqtt.ca_cert, sizeof(config->mqtt.ca_cert), val_str);
        } else if (strcmp(key_str, "client_cert") == 0) {
            yaml_copy_string(
                config->mqtt.client_cert, sizeof(config->mqtt.client_cert), val_str);
        } else if (strcmp(key_str, "client_key") == 0) {
            yaml_copy_string(
                config->mqtt.client_key, sizeof(config->mqtt.client_key), val_str);
        }
    }
}

/**
 * @brief Parse BACnet section from YAML
 */
static void parse_bacnet_section(
    app_config_t *config,
    yaml_document_t *doc,
    yaml_node_t *node)
{
    yaml_node_pair_t *pair;

    if (!config || !doc || !node || node->type != YAML_MAPPING_NODE) {
        return;
    }

    for (pair = node->data.mapping.pairs.start;
         pair < node->data.mapping.pairs.top; pair++) {
        yaml_node_t *key = yaml_document_get_node(doc, pair->key);
        yaml_node_t *value = yaml_document_get_node(doc, pair->value);

        if (!key || !value || key->type != YAML_SCALAR_NODE) {
            continue;
        }

        const char *key_str = (const char *)key->data.scalar.value;
        const char *val_str =
            (value->type == YAML_SCALAR_NODE)
                ? (const char *)value->data.scalar.value
                : NULL;

        if (!val_str) {
            continue;
        }

        if (strcmp(key_str, "device_instance") == 0) {
            config->bacnet.device_instance = (uint32_t)strtoul(val_str, NULL, 0);
        } else if (strcmp(key_str, "udp_port") == 0) {
            config->bacnet.udp_port = (uint16_t)atoi(val_str);
        } else if (strcmp(key_str, "network_interface") == 0) {
            yaml_copy_string(
                config->bacnet.network_interface,
                sizeof(config->bacnet.network_interface), val_str);
        } else if (strcmp(key_str, "device_name") == 0) {
            yaml_copy_string(
                config->bacnet.device_name, sizeof(config->bacnet.device_name),
                val_str);
        } else if (strcmp(key_str, "vendor_name") == 0) {
            yaml_copy_string(
                config->bacnet.vendor_name, sizeof(config->bacnet.vendor_name),
                val_str);
        } else if (strcmp(key_str, "vendor_id") == 0) {
            config->bacnet.vendor_id = (uint16_t)atoi(val_str);
        } else if (strcmp(key_str, "model_name") == 0) {
            yaml_copy_string(
                config->bacnet.model_name, sizeof(config->bacnet.model_name),
                val_str);
        } else if (strcmp(key_str, "app_version") == 0) {
            yaml_copy_string(
                config->bacnet.app_version, sizeof(config->bacnet.app_version),
                val_str);
        } else if (strcmp(key_str, "description") == 0) {
            yaml_copy_string(
                config->bacnet.description, sizeof(config->bacnet.description),
                val_str);
        } else if (strcmp(key_str, "location") == 0) {
            yaml_copy_string(
                config->bacnet.location, sizeof(config->bacnet.location), val_str);
        }
    }
}

/**
 * @brief Parse paths section from YAML
 */
static void parse_paths_section(
    app_config_t *config,
    yaml_document_t *doc,
    yaml_node_t *node)
{
    yaml_node_pair_t *pair;

    if (!config || !doc || !node || node->type != YAML_MAPPING_NODE) {
        return;
    }

    for (pair = node->data.mapping.pairs.start;
         pair < node->data.mapping.pairs.top; pair++) {
        yaml_node_t *key = yaml_document_get_node(doc, pair->key);
        yaml_node_t *value = yaml_document_get_node(doc, pair->value);

        if (!key || !value || key->type != YAML_SCALAR_NODE) {
            continue;
        }

        const char *key_str = (const char *)key->data.scalar.value;
        const char *val_str =
            (value->type == YAML_SCALAR_NODE)
                ? (const char *)value->data.scalar.value
                : NULL;

        if (!val_str) {
            continue;
        }

        if (strcmp(key_str, "sensors_map") == 0) {
            yaml_copy_string(
                config->paths.sensors_map, sizeof(config->paths.sensors_map),
                val_str);
        } else if (strcmp(key_str, "sensors_base") == 0) {
            yaml_copy_string(
                config->paths.sensors_base, sizeof(config->paths.sensors_base),
                val_str);
        }
    }
}

/**
 * @brief Parse logging section from YAML
 */
static void parse_logging_section(
    app_config_t *config,
    yaml_document_t *doc,
    yaml_node_t *node)
{
    yaml_node_pair_t *pair;

    if (!config || !doc || !node || node->type != YAML_MAPPING_NODE) {
        return;
    }

    for (pair = node->data.mapping.pairs.start;
         pair < node->data.mapping.pairs.top; pair++) {
        yaml_node_t *key = yaml_document_get_node(doc, pair->key);
        yaml_node_t *value = yaml_document_get_node(doc, pair->value);

        if (!key || !value || key->type != YAML_SCALAR_NODE) {
            continue;
        }

        const char *key_str = (const char *)key->data.scalar.value;
        const char *val_str =
            (value->type == YAML_SCALAR_NODE)
                ? (const char *)value->data.scalar.value
                : NULL;

        if (!val_str) {
            continue;
        }

        if (strcmp(key_str, "level") == 0) {
            if (strcmp(val_str, "debug") == 0 || strcmp(val_str, "DEBUG") == 0) {
                config->logging.level = 0;
            } else if (
                strcmp(val_str, "info") == 0 || strcmp(val_str, "INFO") == 0) {
                config->logging.level = 1;
            } else if (
                strcmp(val_str, "warning") == 0 ||
                strcmp(val_str, "WARNING") == 0) {
                config->logging.level = 2;
            } else if (
                strcmp(val_str, "error") == 0 || strcmp(val_str, "ERROR") == 0) {
                config->logging.level = 3;
            } else {
                config->logging.level = atoi(val_str);
            }
        } else if (strcmp(key_str, "json_output") == 0) {
            config->logging.json_output =
                (strcmp(val_str, "true") == 0 || strcmp(val_str, "1") == 0);
        } else if (strcmp(key_str, "log_file") == 0) {
            yaml_copy_string(
                config->logging.log_file, sizeof(config->logging.log_file),
                val_str);
        }
    }
}

/**
 * @brief Load configuration from YAML file
 */
bool config_load(app_config_t *config, const char *filepath)
{
    FILE *file;
    yaml_parser_t parser;
    yaml_document_t document;
    yaml_node_t *root, *key, *value;
    yaml_node_pair_t *pair;
    bool success = false;

    if (!config || !filepath) {
        return false;
    }

    file = fopen(filepath, "r");
    if (!file) {
        LOG_WARNING("Cannot open config file: %s", filepath);
        return false;
    }

    if (!yaml_parser_initialize(&parser)) {
        LOG_ERROR("Failed to initialize YAML parser");
        fclose(file);
        return false;
    }

    yaml_parser_set_input_file(&parser, file);

    if (!yaml_parser_load(&parser, &document)) {
        LOG_ERROR("Failed to parse YAML file: %s", filepath);
        yaml_parser_delete(&parser);
        fclose(file);
        return false;
    }

    root = yaml_document_get_root_node(&document);
    if (!root || root->type != YAML_MAPPING_NODE) {
        LOG_ERROR("Invalid YAML structure in: %s", filepath);
        yaml_document_delete(&document);
        yaml_parser_delete(&parser);
        fclose(file);
        return false;
    }

    /* Parse each section */
    for (pair = root->data.mapping.pairs.start;
         pair < root->data.mapping.pairs.top; pair++) {
        key = yaml_document_get_node(&document, pair->key);
        value = yaml_document_get_node(&document, pair->value);

        if (!key || key->type != YAML_SCALAR_NODE) {
            continue;
        }

        const char *section = (const char *)key->data.scalar.value;

        if (strcmp(section, "mqtt") == 0) {
            parse_mqtt_section(config, &document, value);
        } else if (strcmp(section, "bacnet") == 0) {
            parse_bacnet_section(config, &document, value);
        } else if (strcmp(section, "paths") == 0) {
            parse_paths_section(config, &document, value);
        } else if (strcmp(section, "logging") == 0) {
            parse_logging_section(config, &document, value);
        }
    }

    success = true;

    yaml_document_delete(&document);
    yaml_parser_delete(&parser);
    fclose(file);

    LOG_INFO("Configuration loaded from: %s", filepath);
    return success;
}

/**
 * @brief Print usage/help information
 */
void config_print_usage(const char *program_name)
{
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    printf("BACnet MQTT Server - Dynamic BACnet server fed by MQTT + JSON\n\n");
    printf("Options:\n");
    printf(
        "  -c, --config FILE      Configuration file path (default: %s)\n",
        CONFIG_DEFAULT_PATH);
    printf("  -f, --foreground       Run in foreground (don't daemonize)\n");
    printf("  -l, --log-level LEVEL  Log level: debug, info, warning, error\n");
    printf("  -d, --device-id ID     BACnet device instance number\n");
    printf("  -h, --help             Show this help message\n");
    printf("  -v, --version          Show version information\n");
    printf("\nEnvironment variables:\n");
    printf("  BACNET_IFACE           Network interface for BACnet/IP\n");
    printf("  BACNET_IP_PORT         UDP port for BACnet/IP (default: 47808)\n");
}

/**
 * @brief Parse command line arguments
 */
bool config_parse_args(app_config_t *config, int argc, char *argv[])
{
    int opt;
    static struct option long_options[] = {
        { "config", required_argument, 0, 'c' },
        { "foreground", no_argument, 0, 'f' },
        { "log-level", required_argument, 0, 'l' },
        { "device-id", required_argument, 0, 'd' },
        { "help", no_argument, 0, 'h' },
        { "version", no_argument, 0, 'v' },
        { 0, 0, 0, 0 }
    };

    if (!config) {
        return false;
    }

    while ((opt = getopt_long(argc, argv, "c:fl:d:hv", long_options, NULL)) !=
           -1) {
        switch (opt) {
            case 'c':
                if (!config_load(config, optarg)) {
                    fprintf(
                        stderr, "Warning: Could not load config file: %s\n",
                        optarg);
                }
                break;
            case 'f':
                config->foreground = true;
                break;
            case 'l':
                if (strcmp(optarg, "debug") == 0) {
                    config->logging.level = 0;
                } else if (strcmp(optarg, "info") == 0) {
                    config->logging.level = 1;
                } else if (strcmp(optarg, "warning") == 0) {
                    config->logging.level = 2;
                } else if (strcmp(optarg, "error") == 0) {
                    config->logging.level = 3;
                } else {
                    config->logging.level = atoi(optarg);
                }
                break;
            case 'd':
                config->bacnet.device_instance = (uint32_t)strtoul(optarg, NULL, 0);
                break;
            case 'h':
                config_print_usage(argv[0]);
                return false;
            case 'v':
                printf("bacnet-mqtt-server version 1.0.0\n");
                return false;
            default:
                config_print_usage(argv[0]);
                return false;
        }
    }

    return true;
}

/**
 * @brief Print configuration for debugging
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
    printf("    Topic: %s\n", config->mqtt.topic);
    printf("    TLS: %s\n", config->mqtt.use_tls ? "enabled" : "disabled");
    printf("  BACnet:\n");
    printf("    Device Instance: %u\n", config->bacnet.device_instance);
    printf("    UDP Port: %u\n", config->bacnet.udp_port);
    printf("    Device Name: %s\n", config->bacnet.device_name);
    printf("    Vendor Name: %s\n", config->bacnet.vendor_name);
    printf("    Model Name: %s\n", config->bacnet.model_name);
    printf("  Paths:\n");
    printf("    Sensors Map: %s\n", config->paths.sensors_map);
    printf("    Sensors Base: %s\n", config->paths.sensors_base);
    printf("  Logging:\n");
    printf("    Level: %d\n", config->logging.level);
    printf("    JSON Output: %s\n", config->logging.json_output ? "yes" : "no");
    printf("  Foreground: %s\n", config->foreground ? "yes" : "no");
}
