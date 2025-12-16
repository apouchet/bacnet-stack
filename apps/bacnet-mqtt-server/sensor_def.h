/**
 * @file
 * @brief Sensor definition module for BACnet MQTT Server
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 *
 * This module handles loading and parsing sensor definition JSON files
 * that describe the properties each sensor type can report.
 */
#ifndef BACNET_MQTT_SENSOR_DEF_H
#define BACNET_MQTT_SENSOR_DEF_H

#include <stdbool.h>
#include <stdint.h>

/* Maximum number of properties per sensor definition */
#define SENSOR_DEF_MAX_PROPERTIES 32

/* Maximum lengths for string fields */
#define PROPERTY_NAME_MAX_LEN 64
#define PROPERTY_TYPE_MAX_LEN 32
#define PROPERTY_UNITS_MAX_LEN 64
#define SENSOR_DESC_MAX_LEN 256

/**
 * @brief Property data types
 */
typedef enum property_data_type {
    PROP_TYPE_UNKNOWN = 0,
    PROP_TYPE_FLOAT,
    PROP_TYPE_INT,
    PROP_TYPE_UINT8,
    PROP_TYPE_UINT16,
    PROP_TYPE_UINT32,
    PROP_TYPE_INT8,
    PROP_TYPE_INT16,
    PROP_TYPE_INT32,
    PROP_TYPE_BOOL,
    PROP_TYPE_STRING
} property_data_type_t;

/**
 * @brief Property definition structure
 */
typedef struct property_def {
    char name[PROPERTY_NAME_MAX_LEN];
    property_data_type_t type;
    char units[PROPERTY_UNITS_MAX_LEN];
} property_def_t;

/**
 * @brief Sensor definition structure
 */
typedef struct sensor_def {
    char description[SENSOR_DESC_MAX_LEN];
    property_def_t properties[SENSOR_DEF_MAX_PROPERTIES];
    size_t property_count;
    char sensor_type[128]; /* The sensor type path (e.g., "nexelec/X565LS") */
} sensor_def_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Load a sensor definition from file
 * @param def Pointer to sensor definition structure to populate
 * @param base_path Base path for sensor definitions
 * @param src Source type (e.g., "lora")
 * @param sensor_type Sensor type path (e.g., "nexelec/X565LS")
 * @return true on success, false on failure
 */
bool sensor_def_load(
    sensor_def_t *def,
    const char *base_path,
    const char *src,
    const char *sensor_type);

/**
 * @brief Check if a property is defined in the sensor definition
 * @param def Pointer to sensor definition
 * @param property_name Name of the property to check
 * @return Pointer to property definition, or NULL if not found
 */
const property_def_t *sensor_def_find_property(
    const sensor_def_t *def,
    const char *property_name);

/**
 * @brief Parse a property type string into enum
 * @param type_str Type string (e.g., "float", "uint8", "string")
 * @return Property data type enum value
 */
property_data_type_t sensor_def_parse_type(const char *type_str);

/**
 * @brief Get a string representation of a property type
 * @param type Property data type
 * @return String representation
 */
const char *sensor_def_type_to_string(property_data_type_t type);

/**
 * @brief Free resources used by a sensor definition
 * @param def Pointer to sensor definition
 */
void sensor_def_cleanup(sensor_def_t *def);

#ifdef __cplusplus
}
#endif

#endif /* BACNET_MQTT_SENSOR_DEF_H */
