/**
 * @file
 * @brief Object registry module for BACnet MQTT Server
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 *
 * This module maintains a registry mapping (devEUI, property) pairs
 * to BACnet object instances.
 */
#ifndef BACNET_MQTT_OBJECT_REGISTRY_H
#define BACNET_MQTT_OBJECT_REGISTRY_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "bacnet/bacdef.h"

/* Maximum number of registered objects */
#define OBJECT_REGISTRY_MAX_ENTRIES 4096

/* Maximum lengths for string fields */
#define REGISTRY_DEVEUI_MAX_LEN 32
#define REGISTRY_PROPERTY_MAX_LEN 64
#define REGISTRY_NAME_MAX_LEN 64
#define REGISTRY_DESC_MAX_LEN 256

/**
 * @brief Registry entry structure
 */
typedef struct object_registry_entry {
    char deveui[REGISTRY_DEVEUI_MAX_LEN];
    char property_name[REGISTRY_PROPERTY_MAX_LEN];
    char object_name[REGISTRY_NAME_MAX_LEN];
    char description[REGISTRY_DESC_MAX_LEN];
    BACNET_OBJECT_TYPE object_type;
    uint32_t object_instance;
    float last_value;
    time_t last_update;
    bool active;
} object_registry_entry_t;

/**
 * @brief Object registry structure
 */
typedef struct object_registry {
    object_registry_entry_t entries[OBJECT_REGISTRY_MAX_ENTRIES];
    size_t count;
    uint32_t next_instance; /* Next available instance number */
} object_registry_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the object registry
 * @param registry Pointer to registry structure
 * @param base_instance Starting instance number for objects
 * @return true on success, false on failure
 */
bool object_registry_init(object_registry_t *registry, uint32_t base_instance);

/**
 * @brief Look up a registry entry by devEUI and property name
 * @param registry Pointer to registry structure
 * @param deveui Device EUI
 * @param property_name Property name
 * @return Pointer to registry entry, or NULL if not found
 */
object_registry_entry_t *object_registry_lookup(
    object_registry_t *registry,
    const char *deveui,
    const char *property_name);

/**
 * @brief Register a new object or get existing entry
 * @param registry Pointer to registry structure
 * @param deveui Device EUI
 * @param property_name Property name
 * @param object_type BACnet object type
 * @param sensor_desc Sensor description
 * @return Pointer to registry entry (new or existing), or NULL on failure
 */
object_registry_entry_t *object_registry_register(
    object_registry_t *registry,
    const char *deveui,
    const char *property_name,
    BACNET_OBJECT_TYPE object_type,
    const char *sensor_desc);

/**
 * @brief Update the value and timestamp for a registry entry
 * @param entry Pointer to registry entry
 * @param value New value
 * @return true on success, false on failure
 */
bool object_registry_update_value(
    object_registry_entry_t *entry,
    float value);

/**
 * @brief Get the number of registered objects
 * @param registry Pointer to registry structure
 * @return Number of registered objects
 */
size_t object_registry_count(const object_registry_t *registry);

/**
 * @brief Get a registry entry by index
 * @param registry Pointer to registry structure
 * @param index Entry index
 * @return Pointer to registry entry, or NULL if invalid index
 */
object_registry_entry_t *object_registry_get_by_index(
    object_registry_t *registry,
    size_t index);

/**
 * @brief Generate a deterministic object instance number
 * @param deveui Device EUI
 * @param property_name Property name
 * @return Object instance number
 */
uint32_t object_registry_generate_instance(
    const char *deveui,
    const char *property_name);

/**
 * @brief Generate object name from property and devEUI
 * @param property_name Property name
 * @param deveui Device EUI
 * @param object_name Output buffer for object name
 * @param object_name_size Size of output buffer
 * @return true on success, false on failure
 */
bool object_registry_generate_name(
    const char *property_name,
    const char *deveui,
    char *object_name,
    size_t object_name_size);

/**
 * @brief Extract last 4 characters of devEUI (after normalization)
 * @param deveui Device EUI
 * @param last4 Output buffer for last 4 characters
 * @param last4_size Size of output buffer
 * @return true on success, false on failure
 */
bool object_registry_get_last4_deveui(
    const char *deveui,
    char *last4,
    size_t last4_size);

/**
 * @brief Cleanup the object registry
 * @param registry Pointer to registry structure
 */
void object_registry_cleanup(object_registry_t *registry);

#ifdef __cplusplus
}
#endif

#endif /* BACNET_MQTT_OBJECT_REGISTRY_H */
