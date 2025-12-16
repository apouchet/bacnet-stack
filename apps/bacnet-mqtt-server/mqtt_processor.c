/**
 * @file
 * @brief MQTT processor module implementation for BACnet MQTT Server
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include "mqtt_processor.h"
#include "sensor_def.h"
#include "object_factory.h"
#include "logging.h"

/**
 * @brief Initialize MQTT processor
 */
bool mqtt_processor_init(
    mqtt_processor_t *processor,
    sensor_map_t *sensor_map,
    object_registry_t *registry,
    const char *sensors_base_path)
{
    if (!processor || !sensor_map || !registry || !sensors_base_path) {
        return false;
    }

    memset(processor, 0, sizeof(mqtt_processor_t));
    processor->sensor_map = sensor_map;
    processor->registry = registry;
    strncpy(
        processor->sensors_base_path, sensors_base_path,
        sizeof(processor->sensors_base_path) - 1);

    LOG_INFO("MQTT processor initialized");
    return true;
}

/**
 * @brief Extract devEUI from JSON payload
 */
static bool extract_deveui(
    struct json_object *root,
    char *deveui,
    size_t deveui_size)
{
    struct json_object *deveui_obj;

    if (!json_object_object_get_ex(root, "deveui", &deveui_obj)) {
        return false;
    }

    const char *deveui_str = json_object_get_string(deveui_obj);
    if (!deveui_str || strlen(deveui_str) == 0) {
        return false;
    }

    strncpy(deveui, deveui_str, deveui_size - 1);
    deveui[deveui_size - 1] = '\0';

    return true;
}

/**
 * @brief Get uplinkDecoded.data object from JSON
 */
static struct json_object *get_uplink_data(struct json_object *root)
{
    struct json_object *uplink_obj;
    struct json_object *data_obj;

    if (!json_object_object_get_ex(root, "uplinkDecoded", &uplink_obj)) {
        return NULL;
    }

    if (!json_object_is_type(uplink_obj, json_type_object)) {
        return NULL;
    }

    if (!json_object_object_get_ex(uplink_obj, "data", &data_obj)) {
        return NULL;
    }

    if (!json_object_is_type(data_obj, json_type_object)) {
        return NULL;
    }

    return data_obj;
}

/**
 * @brief Process a single property from MQTT data
 */
static bool process_property(
    mqtt_processor_t *processor,
    const char *deveui,
    const char *property_name,
    struct json_object *value_obj,
    const property_def_t *prop_def,
    const char *sensor_desc)
{
    double value = 0.0;

    /* Extract numeric value from JSON */
    switch (json_object_get_type(value_obj)) {
        case json_type_double:
            value = json_object_get_double(value_obj);
            break;
        case json_type_int:
            value = (double)json_object_get_int64(value_obj);
            break;
        case json_type_boolean:
            value = json_object_get_boolean(value_obj) ? 1.0 : 0.0;
            break;
        case json_type_string:
            /* Try to parse as number */
            value = atof(json_object_get_string(value_obj));
            break;
        default:
            LOG_DEBUG("Unsupported JSON type for property: %s", property_name);
            return false;
    }

    LOG_DEBUG(
        "Processing property: %s = %.2f (deveui=%s)", property_name, value,
        deveui);

    /* Create or update BACnet object */
    return object_factory_create_or_update(
        processor->registry, deveui, property_name, prop_def, sensor_desc,
        value);
}

/**
 * @brief Process an MQTT message
 */
bool mqtt_processor_handle_message(
    mqtt_processor_t *processor,
    const char *topic,
    const void *payload,
    size_t payload_len)
{
    struct json_object *root = NULL;
    struct json_object *data_obj = NULL;
    char deveui[64];
    const sensor_map_entry_t *map_entry;
    sensor_def_t sensor_def;
    bool processed = false;

    if (!processor || !payload || payload_len == 0) {
        return false;
    }

    LOG_DEBUG("Processing MQTT message from topic: %s", topic);

    /* Parse JSON payload - use json_tokener_parse for simpler usage */
    root = json_tokener_parse((const char *)payload);
    if (!root) {
        LOG_DEBUG("Failed to parse JSON payload");
        return false;
    }

    /* Extract devEUI */
    if (!extract_deveui(root, deveui, sizeof(deveui))) {
        LOG_DEBUG("No devEUI in message");
        json_object_put(root);
        return false;
    }

    LOG_DEBUG("Received message for devEUI: %s", deveui);

    /* Check sensor map reload */
    sensor_map_check_reload(processor->sensor_map);

    /* Look up sensor in map */
    map_entry = sensor_map_lookup(processor->sensor_map, deveui);
    if (!map_entry) {
        LOG_DEBUG("Unknown devEUI: %s (not in sensor map)", deveui);
        json_object_put(root);
        return false;
    }

    LOG_DEBUG("Found sensor: %s (src=%s)", map_entry->sensor, map_entry->src);

    /* Load sensor definition */
    if (!sensor_def_load(
            &sensor_def, processor->sensors_base_path, map_entry->src,
            map_entry->sensor)) {
        LOG_DEBUG("No sensor definition for: %s/%s", map_entry->src,
            map_entry->sensor);
        json_object_put(root);
        return false;
    }

    /* Get uplink data */
    data_obj = get_uplink_data(root);
    if (!data_obj) {
        LOG_DEBUG("No uplinkDecoded.data in message");
        sensor_def_cleanup(&sensor_def);
        json_object_put(root);
        return false;
    }

    /* Process each property in the data that matches the sensor definition */
    json_object_object_foreach(data_obj, prop_name, prop_value)
    {
        const property_def_t *prop_def;

        /* Check if this property is defined in the sensor definition */
        prop_def = sensor_def_find_property(&sensor_def, prop_name);
        if (!prop_def) {
            LOG_DEBUG("Ignoring undefined property: %s", prop_name);
            continue;
        }

        /* Process the property */
        if (process_property(
                processor, deveui, prop_name, prop_value, prop_def,
                sensor_def.description)) {
            processed = true;
        }
    }

    if (processed) {
        LOG_INFO(
            "Processed MQTT message for devEUI=%s, sensor=%s", deveui,
            map_entry->sensor);
    }

    sensor_def_cleanup(&sensor_def);
    json_object_put(root);

    return processed;
}

/**
 * @brief Cleanup MQTT processor
 */
void mqtt_processor_cleanup(mqtt_processor_t *processor)
{
    if (!processor) {
        return;
    }
    memset(processor, 0, sizeof(mqtt_processor_t));
}
