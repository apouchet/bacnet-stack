/**
 * @file
 * @brief Sensor map module implementation for LoRaWAN MQTT Decoder Service
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <cjson/cJSON.h>
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
    FILE *fp = NULL;
    char *file_content = NULL;
    long file_size;
    cJSON *root = NULL;
    cJSON *entry_obj = NULL;
    struct stat st;

    if (!map || strlen(map->filepath) == 0) {
        return false;
    }

    /* Check if file exists */
    if (stat(map->filepath, &st) != 0) {
        LOG_WARNING("Sensor map file not found: %s", map->filepath);
        return false;
    }

    /* Read file contents */
    fp = fopen(map->filepath, "r");
    if (!fp) {
        LOG_ERROR("Failed to open sensor map file: %s", map->filepath);
        return false;
    }

    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size <= 0) {
        LOG_ERROR("Sensor map file is empty: %s", map->filepath);
        fclose(fp);
        return false;
    }

    file_content = (char *)malloc((size_t)file_size + 1);
    if (!file_content) {
        LOG_ERROR("Failed to allocate memory for sensor map");
        fclose(fp);
        return false;
    }

    if (fread(file_content, 1, (size_t)file_size, fp) != (size_t)file_size) {
        LOG_ERROR("Failed to read sensor map file: %s", map->filepath);
        free(file_content);
        fclose(fp);
        return false;
    }
    file_content[file_size] = '\0';
    fclose(fp);

    /* Parse JSON */
    root = cJSON_Parse(file_content);
    free(file_content);

    if (!root) {
        LOG_ERROR("Failed to parse sensor map JSON: %s", map->filepath);
        return false;
    }

    /* Verify it's an array */
    if (!cJSON_IsArray(root)) {
        LOG_ERROR("Sensor map must be a JSON array: %s", map->filepath);
        cJSON_Delete(root);
        return false;
    }

    /* Clear existing entries */
    map->count = 0;

    /* Parse array entries */
    cJSON_ArrayForEach(entry_obj, root) {
        cJSON *id_obj = NULL;
        cJSON *sensor_obj = NULL;
        cJSON *src_obj = NULL;
        sensor_map_entry_t *entry = NULL;

        if (map->count >= SENSOR_MAP_MAX_ENTRIES) {
            LOG_WARNING("Sensor map max entries reached (%d)", 
                       SENSOR_MAP_MAX_ENTRIES);
            break;
        }

        if (!cJSON_IsObject(entry_obj)) {
            continue;
        }

        entry = &map->entries[map->count];
        memset(entry, 0, sizeof(sensor_map_entry_t));

        /* Get "id" field */
        id_obj = cJSON_GetObjectItemCaseSensitive(entry_obj, "id");
        if (cJSON_IsString(id_obj) && id_obj->valuestring) {
            sensor_map_normalize_deveui(
                id_obj->valuestring, entry->id, sizeof(entry->id));
        }

        /* Get "sensor" field */
        sensor_obj = cJSON_GetObjectItemCaseSensitive(entry_obj, "sensor");
        if (cJSON_IsString(sensor_obj) && sensor_obj->valuestring) {
            strncpy(entry->sensor, sensor_obj->valuestring, 
                   sizeof(entry->sensor) - 1);
        }

        /* Get "src" field */
        src_obj = cJSON_GetObjectItemCaseSensitive(entry_obj, "src");
        if (cJSON_IsString(src_obj) && src_obj->valuestring) {
            strncpy(entry->src, src_obj->valuestring, sizeof(entry->src) - 1);
        }

        /* Only add if we have at least id and sensor */
        if (strlen(entry->id) > 0 && strlen(entry->sensor) > 0) {
            map->count++;
            LOG_DEBUG("Loaded sensor map entry: %s -> %s (%s)", 
                     entry->id, entry->sensor, entry->src);
        }
    }

    cJSON_Delete(root);
    map->last_loaded = time(NULL);

    LOG_INFO("Loaded %zu sensor mappings from %s", map->count, map->filepath);

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
 * @brief Look up a sensor by devEUI (only lora sources)
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
            /* Only return entries where src == "lora" */
            if (strcmp(map->entries[i].src, "lora") == 0) {
                return &map->entries[i];
            }
            LOG_DEBUG("Sensor %s found but src is '%s', not 'lora'", 
                     deveui, map->entries[i].src);
            return NULL;
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
