/**
 * @file
 * @brief MQTT processor module for BACnet MQTT Server
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 *
 * This module processes incoming MQTT messages and creates/updates
 * BACnet objects accordingly.
 */
#ifndef BACNET_MQTT_PROCESSOR_H
#define BACNET_MQTT_PROCESSOR_H

#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "sensor_map.h"
#include "object_registry.h"

/**
 * @brief MQTT processor context
 */
typedef struct mqtt_processor {
    sensor_map_t *sensor_map;
    object_registry_t *registry;
    char sensors_base_path[512];
} mqtt_processor_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the MQTT processor
 * @param processor Pointer to processor context
 * @param sensor_map Pointer to sensor map
 * @param registry Pointer to object registry
 * @param sensors_base_path Base path for sensor definition files
 * @return true on success, false on failure
 */
bool mqtt_processor_init(
    mqtt_processor_t *processor,
    sensor_map_t *sensor_map,
    object_registry_t *registry,
    const char *sensors_base_path);

/**
 * @brief Process an MQTT message
 * @param processor Pointer to processor context
 * @param topic MQTT topic
 * @param payload Message payload (JSON)
 * @param payload_len Payload length
 * @return true if message was processed, false if ignored
 */
bool mqtt_processor_handle_message(
    mqtt_processor_t *processor,
    const char *topic,
    const void *payload,
    size_t payload_len);

/**
 * @brief Cleanup processor resources
 * @param processor Pointer to processor context
 */
void mqtt_processor_cleanup(mqtt_processor_t *processor);

#ifdef __cplusplus
}
#endif

#endif /* BACNET_MQTT_PROCESSOR_H */
