/**
 * @file
 * @brief Sensor map module implementation for BACnet MQTT Server
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <json-c/json.h>
#include "sensor_map.h"
#include "logging.h"

/**
 * @brief Initialize sensor map
 */
bool sensor_map_init(sensor_map_t *map, const char *filepath)
{
    if (!map || !filepath) {
        return false;
    }

    memset(map, 0, sizeof(sensor_map_t));
    strncpy(map->filepath, filepath, sizeof(map->filepath) - 1);

    return sensor_map_load(map);
}

/**
 * @brief Normalize a devEUI string
 *
 * Converts to lowercase and keeps only hex characters and dashes.
 */
bool sensor_map_normalize_deveui(
    const char *deveui,
    char *normalized,
    size_t normalized_size)
{
    size_t i, j = 0;

    if (!deveui || !normalized || normalized_size == 0) {
        return false;
    }

    for (i = 0; deveui[i] != '\0' && j < normalized_size - 1; i++) {
        char c = deveui[i];
        if (isxdigit((unsigned char)c) || c == '-') {
            normalized[j++] = (char)tolower((unsigned char)c);
        }
    }
    normalized[j] = '\0';

    return j > 0;
}

/**
 * @brief Load sensor map from JSON file
 */
bool sensor_map_load(sensor_map_t *map)
{
    struct json_object *root = NULL;
    struct json_object *entry_obj;
    struct json_object *value_obj;
    int array_len, i;
    struct stat st;

    if (!map || strlen(map->filepath) == 0) {
        return false;
    }

    /* Check if file exists */
    if (stat(map->filepath, &st) != 0) {
        LOG_WARNING("Sensor map file not found: %s", map->filepath);
        return false;
    }

    /* Parse JSON file */
    root = json_object_from_file(map->filepath);
    if (!root) {
        LOG_ERROR("Failed to parse sensor map file: %s", map->filepath);
        return false;
    }

    /* Verify it's an array */
    if (!json_object_is_type(root, json_type_array)) {
        LOG_ERROR("Sensor map must be a JSON array: %s", map->filepath);
        json_object_put(root);
        return false;
    }

    /* Clear existing entries */
    map->count = 0;

    /* Parse array entries */
    array_len = json_object_array_length(root);
    for (i = 0; i < array_len && map->count < SENSOR_MAP_MAX_ENTRIES; i++) {
        entry_obj = json_object_array_get_idx(root, (size_t)i);
        if (!entry_obj || !json_object_is_type(entry_obj, json_type_object)) {
            continue;
        }

        sensor_map_entry_t *entry = &map->entries[map->count];
        memset(entry, 0, sizeof(sensor_map_entry_t));

        /* Get "id" field */
        if (json_object_object_get_ex(entry_obj, "id", &value_obj)) {
            const char *id_str = json_object_get_string(value_obj);
            if (id_str) {
                sensor_map_normalize_deveui(
                    id_str, entry->id, sizeof(entry->id));
            }
        }

        /* Get "sensor" field */
        if (json_object_object_get_ex(entry_obj, "sensor", &value_obj)) {
            const char *sensor_str = json_object_get_string(value_obj);
            if (sensor_str) {
                strncpy(entry->sensor, sensor_str, sizeof(entry->sensor) - 1);
            }
        }

        /* Get "src" field */
        if (json_object_object_get_ex(entry_obj, "src", &value_obj)) {
            const char *src_str = json_object_get_string(value_obj);
            if (src_str) {
                strncpy(entry->src, src_str, sizeof(entry->src) - 1);
            }
        }

        /* Only add if we have at least id and sensor */
        if (strlen(entry->id) > 0 && strlen(entry->sensor) > 0) {
            map->count++;
            LOG_DEBUG(
                "Loaded sensor map entry: %s -> %s (%s)", entry->id,
                entry->sensor, entry->src);
        }
    }

    json_object_put(root);
    map->last_loaded = time(NULL);

    LOG_INFO(
        "Loaded %zu sensor mappings from %s", map->count, map->filepath);

    return true;
}

/**
 * @brief Check if reload is needed based on file modification time
 */
bool sensor_map_check_reload(sensor_map_t *map)
{
    struct stat st;

    if (!map || strlen(map->filepath) == 0) {
        return false;
    }

    if (stat(map->filepath, &st) != 0) {
        return false;
    }

    /* Check if file was modified after last load */
    if (st.st_mtime > map->last_loaded) {
        LOG_INFO("Sensor map file modified, reloading...");
        return sensor_map_load(map);
    }

    return false;
}

/**
 * @brief Look up a sensor by devEUI
 */
const sensor_map_entry_t *sensor_map_lookup(
    const sensor_map_t *map,
    const char *deveui)
{
    char normalized[DEVEUI_MAX_LEN];
    size_t i;

    if (!map || !deveui) {
        return NULL;
    }

    if (!sensor_map_normalize_deveui(deveui, normalized, sizeof(normalized))) {
        return NULL;
    }

    for (i = 0; i < map->count; i++) {
        if (strcmp(map->entries[i].id, normalized) == 0) {
            return &map->entries[i];
        }
    }

    return NULL;
}

/**
 * @brief Get number of entries in the map
 */
size_t sensor_map_count(const sensor_map_t *map)
{
    if (!map) {
        return 0;
    }
    return map->count;
}

/**
 * @brief Cleanup sensor map resources
 */
void sensor_map_cleanup(sensor_map_t *map)
{
    if (!map) {
        return;
    }
    memset(map, 0, sizeof(sensor_map_t));
}
