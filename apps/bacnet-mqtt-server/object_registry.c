/**
 * @file
 * @brief Object registry module implementation for BACnet MQTT Server
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "object_registry.h"
#include "sensor_map.h"
#include "logging.h"

/**
 * @brief Simple hash function for deterministic instance generation
 */
static uint32_t hash_string(const char *str)
{
    uint32_t hash = 5381;
    int c;

    while ((c = *str++) != 0) {
        hash = ((hash << 5) + hash) + (uint32_t)c;
    }

    return hash;
}

/**
 * @brief Initialize object registry
 */
bool object_registry_init(object_registry_t *registry, uint32_t base_instance)
{
    if (!registry) {
        return false;
    }

    memset(registry, 0, sizeof(object_registry_t));
    registry->next_instance = base_instance;

    return true;
}

/**
 * @brief Generate deterministic instance number from devEUI and property
 */
uint32_t object_registry_generate_instance(
    const char *deveui,
    const char *property_name)
{
    char combined[256];
    uint32_t hash;

    if (!deveui || !property_name) {
        return 0;
    }

    snprintf(combined, sizeof(combined), "%s:%s", deveui, property_name);
    hash = hash_string(combined);

    /* Ensure it's a valid BACnet instance (0 to 4194302) */
    return hash % 4194303;
}

/**
 * @brief Extract last 4 hex characters from devEUI
 */
bool object_registry_get_last4_deveui(
    const char *deveui,
    char *last4,
    size_t last4_size)
{
    char normalized[REGISTRY_DEVEUI_MAX_LEN];
    size_t len, i, j = 0;

    if (!deveui || !last4 || last4_size < 5) {
        return false;
    }

    /* Normalize: extract only hex digits */
    for (i = 0; deveui[i] != '\0' && j < sizeof(normalized) - 1; i++) {
        if (isxdigit((unsigned char)deveui[i])) {
            normalized[j++] = (char)tolower((unsigned char)deveui[i]);
        }
    }
    normalized[j] = '\0';
    len = j;

    if (len < 4) {
        /* If less than 4 hex chars, use what we have */
        strncpy(last4, normalized, last4_size - 1);
        last4[last4_size - 1] = '\0';
    } else {
        /* Get last 4 characters */
        strncpy(last4, normalized + len - 4, 4);
        last4[4] = '\0';
    }

    return strlen(last4) > 0;
}

/**
 * @brief Generate object name from property and devEUI
 */
bool object_registry_generate_name(
    const char *property_name,
    const char *deveui,
    char *object_name,
    size_t object_name_size)
{
    char last4[8];

    if (!property_name || !deveui || !object_name || object_name_size == 0) {
        return false;
    }

    if (!object_registry_get_last4_deveui(deveui, last4, sizeof(last4))) {
        return false;
    }

    snprintf(object_name, object_name_size, "%s-%s", property_name, last4);

    return true;
}

/**
 * @brief Look up a registry entry by devEUI and property
 */
object_registry_entry_t *object_registry_lookup(
    object_registry_t *registry,
    const char *deveui,
    const char *property_name)
{
    char normalized[REGISTRY_DEVEUI_MAX_LEN];
    size_t i;

    if (!registry || !deveui || !property_name) {
        return NULL;
    }

    if (!sensor_map_normalize_deveui(deveui, normalized, sizeof(normalized))) {
        return NULL;
    }

    for (i = 0; i < registry->count; i++) {
        if (registry->entries[i].active &&
            strcmp(registry->entries[i].deveui, normalized) == 0 &&
            strcmp(registry->entries[i].property_name, property_name) == 0) {
            return &registry->entries[i];
        }
    }

    return NULL;
}

/**
 * @brief Register a new object or get existing entry
 */
object_registry_entry_t *object_registry_register(
    object_registry_t *registry,
    const char *deveui,
    const char *property_name,
    BACNET_OBJECT_TYPE object_type,
    const char *sensor_desc)
{
    object_registry_entry_t *entry;
    char normalized[REGISTRY_DEVEUI_MAX_LEN];

    if (!registry || !deveui || !property_name) {
        return NULL;
    }

    /* Check if already registered */
    entry = object_registry_lookup(registry, deveui, property_name);
    if (entry) {
        return entry;
    }

    /* Check capacity */
    if (registry->count >= OBJECT_REGISTRY_MAX_ENTRIES) {
        LOG_ERROR("Object registry full, cannot register new object");
        return NULL;
    }

    /* Normalize devEUI */
    if (!sensor_map_normalize_deveui(deveui, normalized, sizeof(normalized))) {
        LOG_ERROR("Failed to normalize devEUI: %s", deveui);
        return NULL;
    }

    /* Create new entry */
    entry = &registry->entries[registry->count];
    memset(entry, 0, sizeof(object_registry_entry_t));

    strncpy(entry->deveui, normalized, sizeof(entry->deveui) - 1);
    strncpy(entry->property_name, property_name, sizeof(entry->property_name) - 1);
    entry->object_type = object_type;

    /* Generate deterministic instance number */
    entry->object_instance =
        object_registry_generate_instance(normalized, property_name);

    /* Generate object name */
    if (!object_registry_generate_name(
            property_name, normalized, entry->object_name,
            sizeof(entry->object_name))) {
        LOG_ERROR("Failed to generate object name");
        return NULL;
    }

    /* Set description */
    if (sensor_desc) {
        snprintf(
            entry->description, sizeof(entry->description), "%s [%s]",
            sensor_desc, normalized);
    } else {
        snprintf(
            entry->description, sizeof(entry->description), "%s [%s]",
            property_name, normalized);
    }

    entry->active = true;
    entry->last_update = time(NULL);

    registry->count++;

    LOG_INFO(
        "Registered object: %s (type=%d, instance=%u)", entry->object_name,
        entry->object_type, entry->object_instance);

    return entry;
}

/**
 * @brief Update value and timestamp for a registry entry
 */
bool object_registry_update_value(object_registry_entry_t *entry, float value)
{
    if (!entry) {
        return false;
    }

    entry->last_value = value;
    entry->last_update = time(NULL);

    return true;
}

/**
 * @brief Get number of registered objects
 */
size_t object_registry_count(const object_registry_t *registry)
{
    if (!registry) {
        return 0;
    }
    return registry->count;
}

/**
 * @brief Get registry entry by index
 */
object_registry_entry_t *object_registry_get_by_index(
    object_registry_t *registry,
    size_t index)
{
    if (!registry || index >= registry->count) {
        return NULL;
    }
    return &registry->entries[index];
}

/**
 * @brief Cleanup object registry
 */
void object_registry_cleanup(object_registry_t *registry)
{
    if (!registry) {
        return;
    }
    memset(registry, 0, sizeof(object_registry_t));
}
