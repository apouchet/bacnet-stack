/**
 * @file
 * @brief Units mapping module for BACnet MQTT Server
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 *
 * This module maps JSON unit strings to BACnet engineering units.
 */
#ifndef BACNET_MQTT_UNITS_MAP_H
#define BACNET_MQTT_UNITS_MAP_H

#include <stdint.h>
#include "bacnet/bacenum.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Map a JSON units string to BACnet engineering units
 * @param units_str JSON units string (e.g., "degrees-celsius")
 * @return BACnet engineering units value
 */
BACNET_ENGINEERING_UNITS units_map_to_bacnet(const char *units_str);

/**
 * @brief Get the JSON units string for a BACnet engineering unit
 * @param units BACnet engineering units value
 * @return JSON units string, or "no-units" if not found
 */
const char *units_map_from_bacnet(BACNET_ENGINEERING_UNITS units);

#ifdef __cplusplus
}
#endif

#endif /* BACNET_MQTT_UNITS_MAP_H */
