/**
 * @file
 * @brief Sensor definition module implementation for BACnet MQTT Server
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <json-c/json.h>
#include "sensor_def.h"
#include "logging.h"

/**
 * @brief Parse property type string to enum
 */
property_data_type_t sensor_def_parse_type(const char *type_str)
{
    if (!type_str) {
        return PROP_TYPE_UNKNOWN;
    }

    if (strcmp(type_str, "float") == 0 || strcmp(type_str, "double") == 0) {
        return PROP_TYPE_FLOAT;
    } else if (strcmp(type_str, "int") == 0 || strcmp(type_str, "integer") == 0) {
        return PROP_TYPE_INT;
    } else if (strcmp(type_str, "uint8") == 0) {
        return PROP_TYPE_UINT8;
    } else if (strcmp(type_str, "uint16") == 0) {
        return PROP_TYPE_UINT16;
    } else if (strcmp(type_str, "uint32") == 0) {
        return PROP_TYPE_UINT32;
    } else if (strcmp(type_str, "int8") == 0) {
        return PROP_TYPE_INT8;
    } else if (strcmp(type_str, "int16") == 0) {
        return PROP_TYPE_INT16;
    } else if (strcmp(type_str, "int32") == 0) {
        return PROP_TYPE_INT32;
    } else if (strcmp(type_str, "bool") == 0 || strcmp(type_str, "boolean") == 0) {
        return PROP_TYPE_BOOL;
    } else if (strcmp(type_str, "string") == 0) {
        return PROP_TYPE_STRING;
    }

    return PROP_TYPE_UNKNOWN;
}

/**
 * @brief Convert property type enum to string
 */
const char *sensor_def_type_to_string(property_data_type_t type)
{
    switch (type) {
        case PROP_TYPE_FLOAT:
            return "float";
        case PROP_TYPE_INT:
            return "int";
        case PROP_TYPE_UINT8:
            return "uint8";
        case PROP_TYPE_UINT16:
            return "uint16";
        case PROP_TYPE_UINT32:
            return "uint32";
        case PROP_TYPE_INT8:
            return "int8";
        case PROP_TYPE_INT16:
            return "int16";
        case PROP_TYPE_INT32:
            return "int32";
        case PROP_TYPE_BOOL:
            return "bool";
        case PROP_TYPE_STRING:
            return "string";
        default:
            return "unknown";
    }
}

/**
 * @brief Load sensor definition from JSON file
 */
bool sensor_def_load(
    sensor_def_t *def,
    const char *base_path,
    const char *src,
    const char *sensor_type)
{
    char filepath[1024];
    struct json_object *root = NULL;
    struct json_object *desc_obj;
    struct json_object *props_obj;
    struct stat st;

    if (!def || !base_path || !src || !sensor_type) {
        return false;
    }

    memset(def, 0, sizeof(sensor_def_t));
    strncpy(def->sensor_type, sensor_type, sizeof(def->sensor_type) - 1);

    /* Build the file path: {base_path}/{src}/{sensor_type}.json */
    snprintf(
        filepath, sizeof(filepath), "%s/%s/%s.json", base_path, src,
        sensor_type);

    /* Check if file exists */
    if (stat(filepath, &st) != 0) {
        LOG_DEBUG("Sensor definition file not found: %s", filepath);
        return false;
    }

    /* Parse JSON file */
    root = json_object_from_file(filepath);
    if (!root) {
        LOG_ERROR("Failed to parse sensor definition file: %s", filepath);
        return false;
    }

    /* Get description */
    if (json_object_object_get_ex(root, "description", &desc_obj)) {
        const char *desc_str = json_object_get_string(desc_obj);
        if (desc_str) {
            strncpy(def->description, desc_str, sizeof(def->description) - 1);
        }
    }

    /* Get properties object */
    if (!json_object_object_get_ex(root, "properties", &props_obj)) {
        LOG_ERROR(
            "Sensor definition missing 'properties' field: %s", filepath);
        json_object_put(root);
        return false;
    }

    if (!json_object_is_type(props_obj, json_type_object)) {
        LOG_ERROR("Sensor definition 'properties' must be an object: %s", filepath);
        json_object_put(root);
        return false;
    }

    /* Iterate over properties */
    json_object_object_foreach(props_obj, prop_name, prop_obj)
    {
        if (def->property_count >= SENSOR_DEF_MAX_PROPERTIES) {
            LOG_WARNING("Too many properties in sensor definition: %s", filepath);
            break;
        }

        if (!json_object_is_type(prop_obj, json_type_object)) {
            continue;
        }

        property_def_t *prop = &def->properties[def->property_count];
        memset(prop, 0, sizeof(property_def_t));

        strncpy(prop->name, prop_name, sizeof(prop->name) - 1);

        /* Get type */
        struct json_object *type_obj;
        if (json_object_object_get_ex(prop_obj, "type", &type_obj)) {
            const char *type_str = json_object_get_string(type_obj);
            prop->type = sensor_def_parse_type(type_str);
        } else {
            prop->type = PROP_TYPE_FLOAT; /* Default to float */
        }

        /* Get units */
        struct json_object *units_obj;
        if (json_object_object_get_ex(prop_obj, "units", &units_obj)) {
            const char *units_str = json_object_get_string(units_obj);
            if (units_str) {
                strncpy(prop->units, units_str, sizeof(prop->units) - 1);
            }
        }

        def->property_count++;
        LOG_DEBUG(
            "Loaded property: %s (type=%s, units=%s)", prop->name,
            sensor_def_type_to_string(prop->type), prop->units);
    }

    json_object_put(root);

    LOG_INFO(
        "Loaded sensor definition: %s with %zu properties", sensor_type,
        def->property_count);

    return true;
}

/**
 * @brief Find a property by name
 */
const property_def_t *sensor_def_find_property(
    const sensor_def_t *def,
    const char *property_name)
{
    size_t i;

    if (!def || !property_name) {
        return NULL;
    }

    for (i = 0; i < def->property_count; i++) {
        if (strcmp(def->properties[i].name, property_name) == 0) {
            return &def->properties[i];
        }
    }

    return NULL;
}

/**
 * @brief Cleanup sensor definition resources
 */
void sensor_def_cleanup(sensor_def_t *def)
{
    if (!def) {
        return;
    }
    memset(def, 0, sizeof(sensor_def_t));
}
