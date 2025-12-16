/**
 * @file
 * @brief Object factory module implementation for BACnet MQTT Server
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "object_factory.h"
#include "units_map.h"
#include "logging.h"
/* BACnet Stack includes */
#include "bacnet/bacstr.h"
#include "bacnet/basic/object/ai.h"
#include "bacnet/basic/object/csv.h"
#include "bacnet/basic/object/device.h"

/* Module state */
static object_registry_t *g_registry = NULL;

/**
 * @brief Initialize object factory
 */
bool object_factory_init(object_registry_t *registry)
{
    if (!registry) {
        return false;
    }

    g_registry = registry;

    /* Initialize BACnet object modules */
    Analog_Input_Init();
    CharacterString_Value_Init();

    LOG_INFO("Object factory initialized");
    return true;
}

/**
 * @brief Determine BACnet object type from property data type
 */
BACNET_OBJECT_TYPE object_factory_get_object_type(property_data_type_t prop_type)
{
    switch (prop_type) {
        case PROP_TYPE_FLOAT:
        case PROP_TYPE_INT:
        case PROP_TYPE_UINT8:
        case PROP_TYPE_UINT16:
        case PROP_TYPE_UINT32:
        case PROP_TYPE_INT8:
        case PROP_TYPE_INT16:
        case PROP_TYPE_INT32:
            return OBJECT_ANALOG_INPUT;
        case PROP_TYPE_STRING:
            return OBJECT_CHARACTERSTRING_VALUE;
        case PROP_TYPE_BOOL:
            /* Binary types could be added here in the future */
            return OBJECT_ANALOG_INPUT; /* Use AI for now */
        default:
            return OBJECT_ANALOG_INPUT;
    }
}

/**
 * @brief Create an Analog Input object
 */
bool object_factory_create_analog_input(
    object_registry_entry_t *entry,
    BACNET_ENGINEERING_UNITS units,
    float initial_value)
{
    uint32_t instance;

    if (!entry) {
        return false;
    }

    instance = entry->object_instance;

    /* Create the object in the BACnet stack */
    if (!Analog_Input_Valid_Instance(instance)) {
        if (Analog_Input_Create(instance) == BACNET_MAX_INSTANCE) {
            LOG_ERROR("Failed to create Analog Input instance %u", instance);
            return false;
        }
    }

    /* Set object properties */
    Analog_Input_Name_Set(instance, entry->object_name);
    Analog_Input_Description_Set(instance, entry->description);
    Analog_Input_Units_Set(instance, units);
    Analog_Input_Present_Value_Set(instance, initial_value);
    Analog_Input_Out_Of_Service_Set(instance, false);
    Analog_Input_Reliability_Set(instance, RELIABILITY_NO_FAULT_DETECTED);

    /* Set COV increment for triggering notifications */
    Analog_Input_COV_Increment_Set(instance, 0.1f);

    LOG_INFO(
        "Created Analog Input: %s (instance=%u, value=%.2f)",
        entry->object_name, instance, (double)initial_value);

    return true;
}

/**
 * @brief Update an Analog Input object value
 */
bool object_factory_update_analog_input(uint32_t instance, float value)
{
    if (!Analog_Input_Valid_Instance(instance)) {
        return false;
    }

    Analog_Input_Present_Value_Set(instance, value);

    LOG_DEBUG("Updated Analog Input %u to %.2f", instance, (double)value);

    return true;
}

/**
 * @brief Create a CharacterString Value object
 */
bool object_factory_create_csv(
    object_registry_entry_t *entry,
    const char *initial_value)
{
    uint32_t instance;
    BACNET_CHARACTER_STRING bac_value;

    if (!entry) {
        return false;
    }

    instance = entry->object_instance;

    /* Create the object in the BACnet stack */
    if (!CharacterString_Value_Valid_Instance(instance)) {
        if (CharacterString_Value_Create(instance) == BACNET_MAX_INSTANCE) {
            LOG_ERROR(
                "Failed to create CharacterString Value instance %u", instance);
            return false;
        }
    }

    /* Set object properties */
    CharacterString_Value_Name_Set(instance, entry->object_name);
    CharacterString_Value_Description_Set(instance, entry->description);
    if (initial_value) {
        characterstring_init_ansi(&bac_value, initial_value);
        CharacterString_Value_Present_Value_Set(instance, &bac_value);
    }

    LOG_INFO(
        "Created CharacterString Value: %s (instance=%u)",
        entry->object_name, instance);

    return true;
}

/**
 * @brief Update a CharacterString Value object
 */
bool object_factory_update_csv(uint32_t instance, const char *value)
{
    if (!CharacterString_Value_Valid_Instance(instance)) {
        return false;
    }

    if (value) {
        CharacterString_Value_Present_Value_Set(instance, value);
    }

    LOG_DEBUG("Updated CharacterString Value %u", instance);

    return true;
}

/**
 * @brief Create or update a BACnet object from MQTT data
 */
bool object_factory_create_or_update(
    object_registry_t *registry,
    const char *deveui,
    const char *property_name,
    const property_def_t *prop_def,
    const char *sensor_desc,
    double value)
{
    object_registry_entry_t *entry;
    BACNET_OBJECT_TYPE object_type;
    BACNET_ENGINEERING_UNITS units;
    bool is_new = false;

    if (!registry || !deveui || !property_name || !prop_def) {
        return false;
    }

    /* Determine object type */
    object_type = object_factory_get_object_type(prop_def->type);

    /* Get or create registry entry */
    entry = object_registry_lookup(registry, deveui, property_name);
    if (!entry) {
        entry = object_registry_register(
            registry, deveui, property_name, object_type, sensor_desc);
        if (!entry) {
            LOG_ERROR("Failed to register object for %s:%s", deveui, property_name);
            return false;
        }
        is_new = true;
    }

    /* Map units */
    units = units_map_to_bacnet(prop_def->units);

    /* Create or update based on object type */
    switch (object_type) {
        case OBJECT_ANALOG_INPUT:
            if (is_new) {
                if (!object_factory_create_analog_input(
                        entry, units, (float)value)) {
                    return false;
                }
            } else {
                if (!object_factory_update_analog_input(
                        entry->object_instance, (float)value)) {
                    return false;
                }
            }
            break;

        case OBJECT_CHARACTERSTRING_VALUE:
            /* For string values, we'd need to handle differently */
            if (is_new) {
                char str_value[32];
                snprintf(str_value, sizeof(str_value), "%.2f", value);
                if (!object_factory_create_csv(entry, str_value)) {
                    return false;
                }
            }
            break;

        default:
            LOG_WARNING("Unsupported object type: %d", object_type);
            return false;
    }

    /* Update registry entry */
    object_registry_update_value(entry, (float)value);

    return true;
}

/**
 * @brief Shutdown object factory
 */
void object_factory_shutdown(void)
{
    Analog_Input_Cleanup();
    CharacterString_Value_Cleanup();
    g_registry = NULL;

    LOG_INFO("Object factory shutdown");
}
