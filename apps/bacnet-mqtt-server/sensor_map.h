/**
 * @file
 * @brief Sensor map module for BACnet MQTT Server
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 *
 * This module handles loading and caching the sensors_map.json file
 * which maps devEUI identifiers to sensor types.
 */
#ifndef BACNET_MQTT_SENSOR_MAP_H
#define BACNET_MQTT_SENSOR_MAP_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/* Maximum number of sensors in the map */
#define SENSOR_MAP_MAX_ENTRIES 1024

/* Maximum lengths for string fields */
#define DEVEUI_MAX_LEN 32
#define SENSOR_TYPE_MAX_LEN 128
#define SENSOR_SRC_MAX_LEN 32

/**
 * @brief Sensor map entry structure
 */
typedef struct sensor_map_entry {
    char id[DEVEUI_MAX_LEN];          /* devEUI identifier */
    char sensor[SENSOR_TYPE_MAX_LEN]; /* sensor type (e.g., "nexelec/X565LS") */
    char src[SENSOR_SRC_MAX_LEN];     /* source type (e.g., "lora") */
} sensor_map_entry_t;

/**
 * @brief Sensor map structure
 */
typedef struct sensor_map {
    sensor_map_entry_t entries[SENSOR_MAP_MAX_ENTRIES];
    size_t count;
    time_t last_loaded;
    char filepath[512];
} sensor_map_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the sensor map module
 * @param map Pointer to sensor map structure
 * @param filepath Path to sensors_map.json file
 * @return true on success, false on failure
 */
bool sensor_map_init(sensor_map_t *map, const char *filepath);

/**
 * @brief Load or reload the sensor map from file
 * @param map Pointer to sensor map structure
 * @return true on success, false on failure
 */
bool sensor_map_load(sensor_map_t *map);

/**
 * @brief Check if the sensor map file has been modified and reload if needed
 * @param map Pointer to sensor map structure
 * @return true if reloaded, false if not needed or failed
 */
bool sensor_map_check_reload(sensor_map_t *map);

/**
 * @brief Look up a sensor entry by devEUI
 * @param map Pointer to sensor map structure
 * @param deveui devEUI identifier to look up
 * @return Pointer to sensor map entry, or NULL if not found
 */
const sensor_map_entry_t *sensor_map_lookup(
    const sensor_map_t *map,
    const char *deveui);

/**
 * @brief Get the number of entries in the sensor map
 * @param map Pointer to sensor map structure
 * @return Number of entries
 */
size_t sensor_map_count(const sensor_map_t *map);

/**
 * @brief Free resources used by the sensor map
 * @param map Pointer to sensor map structure
 */
void sensor_map_cleanup(sensor_map_t *map);

/**
 * @brief Normalize a devEUI string (lowercase, consistent format)
 * @param deveui Input devEUI string
 * @param normalized Output buffer for normalized devEUI
 * @param normalized_size Size of output buffer
 * @return true on success, false on failure
 */
bool sensor_map_normalize_deveui(
    const char *deveui,
    char *normalized,
    size_t normalized_size);

#ifdef __cplusplus
}
#endif

#endif /* BACNET_MQTT_SENSOR_MAP_H */
