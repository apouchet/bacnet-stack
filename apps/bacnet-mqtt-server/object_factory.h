/**
 * @file
 * @brief Object factory module for BACnet MQTT Server
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 *
 * This module handles dynamic creation and updating of BACnet objects.
 */
#ifndef BACNET_MQTT_OBJECT_FACTORY_H
#define BACNET_MQTT_OBJECT_FACTORY_H

#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacdef.h"
#include "object_registry.h"
#include "sensor_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the object factory
 * @param registry Pointer to object registry
 * @return true on success, false on failure
 */
bool object_factory_init(object_registry_t *registry);

/**
 * @brief Create or update a BACnet object from MQTT data
 * @param registry Pointer to object registry
 * @param deveui Device EUI
 * @param property_name Property name
 * @param prop_def Property definition from sensor definition
 * @param sensor_desc Sensor description
 * @param value Value to set
 * @return true on success, false on failure
 */
bool object_factory_create_or_update(
    object_registry_t *registry,
    const char *deveui,
    const char *property_name,
    const property_def_t *prop_def,
    const char *sensor_desc,
    double value);

/**
 * @brief Determine BACnet object type from property data type
 * @param prop_type Property data type
 * @return BACnet object type
 */
BACNET_OBJECT_TYPE object_factory_get_object_type(property_data_type_t prop_type);

/**
 * @brief Create an Analog Input object
 * @param entry Registry entry for the object
 * @param units BACnet engineering units
 * @param initial_value Initial present value
 * @return true on success, false on failure
 */
bool object_factory_create_analog_input(
    object_registry_entry_t *entry,
    BACNET_ENGINEERING_UNITS units,
    float initial_value);

/**
 * @brief Update an Analog Input object value
 * @param instance Object instance number
 * @param value New value
 * @return true on success, false on failure
 */
bool object_factory_update_analog_input(uint32_t instance, float value);

/**
 * @brief Create a CharacterString Value object
 * @param entry Registry entry for the object
 * @param initial_value Initial string value
 * @return true on success, false on failure
 */
bool object_factory_create_csv(
    object_registry_entry_t *entry,
    const char *initial_value);

/**
 * @brief Update a CharacterString Value object
 * @param instance Object instance number
 * @param value New string value
 * @return true on success, false on failure
 */
bool object_factory_update_csv(uint32_t instance, const char *value);

/**
 * @brief Shutdown object factory and cleanup resources
 */
void object_factory_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* BACNET_MQTT_OBJECT_FACTORY_H */
