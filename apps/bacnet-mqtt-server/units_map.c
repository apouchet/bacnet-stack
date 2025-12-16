/**
 * @file
 * @brief Units mapping module implementation for BACnet MQTT Server
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <string.h>
#include "units_map.h"
#include "bacnet/bacenum.h"

/**
 * @brief Units mapping table entry
 */
typedef struct units_mapping {
    const char *json_name;
    BACNET_ENGINEERING_UNITS bacnet_units;
} units_mapping_t;

/**
 * @brief Units mapping table
 */
static const units_mapping_t units_table[] = {
    /* Temperature */
    { "degrees-celsius", UNITS_DEGREES_CELSIUS },
    { "celsius", UNITS_DEGREES_CELSIUS },
    { "°C", UNITS_DEGREES_CELSIUS },
    { "degrees-fahrenheit", UNITS_DEGREES_FAHRENHEIT },
    { "fahrenheit", UNITS_DEGREES_FAHRENHEIT },
    { "°F", UNITS_DEGREES_FAHRENHEIT },
    { "degrees-kelvin", UNITS_KELVIN },
    { "kelvin", UNITS_KELVIN },
    { "K", UNITS_KELVIN },

    /* Humidity */
    { "percent-relative-humidity", UNITS_PERCENT_RELATIVE_HUMIDITY },
    { "percent-humidity", UNITS_PERCENT_RELATIVE_HUMIDITY },
    { "%RH", UNITS_PERCENT_RELATIVE_HUMIDITY },
    { "percent", UNITS_PERCENT },
    { "%", UNITS_PERCENT },

    /* Pressure */
    { "pascals", UNITS_PASCALS },
    { "Pa", UNITS_PASCALS },
    { "hectopascals", UNITS_HECTOPASCALS },
    { "hPa", UNITS_HECTOPASCALS },
    { "kilopascals", UNITS_KILOPASCALS },
    { "kPa", UNITS_KILOPASCALS },
    { "bars", UNITS_BARS },
    { "bar", UNITS_BARS },
    { "millibar", UNITS_MILLIBARS },
    { "millibars", UNITS_MILLIBARS },
    { "mbar", UNITS_MILLIBARS },
    { "psi", UNITS_POUNDS_FORCE_PER_SQUARE_INCH },

    /* Concentration */
    { "parts-per-million", UNITS_PARTS_PER_MILLION },
    { "ppm", UNITS_PARTS_PER_MILLION },
    { "parts-per-billion", UNITS_PARTS_PER_BILLION },
    { "ppb", UNITS_PARTS_PER_BILLION },
    { "micrograms-per-cubic-meter", UNITS_MICROGRAMS_PER_CUBIC_METER },
    { "ug/m3", UNITS_MICROGRAMS_PER_CUBIC_METER },

    /* Light */
    { "lux", UNITS_LUXES },
    { "lumens", UNITS_LUMENS },
    { "candelas", UNITS_CANDELAS },
    { "watts-per-square-meter", UNITS_WATTS_PER_SQUARE_METER },
    { "W/m2", UNITS_WATTS_PER_SQUARE_METER },

    /* Power */
    { "watts", UNITS_WATTS },
    { "W", UNITS_WATTS },
    { "kilowatts", UNITS_KILOWATTS },
    { "kW", UNITS_KILOWATTS },
    { "megawatts", UNITS_MEGAWATTS },
    { "MW", UNITS_MEGAWATTS },
    { "milliwatts", UNITS_MILLIWATTS },
    { "mW", UNITS_MILLIWATTS },

    /* Energy */
    { "watt-hours", UNITS_WATT_HOURS },
    { "Wh", UNITS_WATT_HOURS },
    { "kilowatt-hours", UNITS_KILOWATT_HOURS },
    { "kWh", UNITS_KILOWATT_HOURS },
    { "joules", UNITS_JOULES },
    { "J", UNITS_JOULES },
    { "kilojoules", UNITS_KILOJOULES },
    { "kJ", UNITS_KILOJOULES },

    /* Voltage/Current */
    { "volts", UNITS_VOLTS },
    { "V", UNITS_VOLTS },
    { "millivolts", UNITS_MILLIVOLTS },
    { "mV", UNITS_MILLIVOLTS },
    { "amperes", UNITS_AMPERES },
    { "A", UNITS_AMPERES },
    { "milliamperes", UNITS_MILLIAMPERES },
    { "mA", UNITS_MILLIAMPERES },

    /* Resistance/Frequency */
    { "ohms", UNITS_OHMS },
    { "kilohms", UNITS_KILOHMS },
    { "megohms", UNITS_MEGOHMS },
    { "hertz", UNITS_HERTZ },
    { "Hz", UNITS_HERTZ },
    { "kilohertz", UNITS_KILOHERTZ },
    { "kHz", UNITS_KILOHERTZ },

    /* Flow */
    { "liters-per-second", UNITS_LITERS_PER_SECOND },
    { "L/s", UNITS_LITERS_PER_SECOND },
    { "liters-per-minute", UNITS_LITERS_PER_MINUTE },
    { "L/min", UNITS_LITERS_PER_MINUTE },
    { "cubic-meters-per-hour", UNITS_CUBIC_METERS_PER_HOUR },
    { "m3/h", UNITS_CUBIC_METERS_PER_HOUR },

    /* Volume */
    { "liters", UNITS_LITERS },
    { "L", UNITS_LITERS },
    { "cubic-meters", UNITS_CUBIC_METERS },
    { "m3", UNITS_CUBIC_METERS },
    { "gallons", UNITS_US_GALLONS },

    /* Length */
    { "meters", UNITS_METERS },
    { "m", UNITS_METERS },
    { "centimeters", UNITS_CENTIMETERS },
    { "cm", UNITS_CENTIMETERS },
    { "millimeters", UNITS_MILLIMETERS },
    { "mm", UNITS_MILLIMETERS },
    { "feet", UNITS_FEET },
    { "ft", UNITS_FEET },
    { "inches", UNITS_INCHES },
    { "in", UNITS_INCHES },

    /* Time */
    { "seconds", UNITS_SECONDS },
    { "s", UNITS_SECONDS },
    { "minutes", UNITS_MINUTES },
    { "min", UNITS_MINUTES },
    { "hours", UNITS_HOURS },
    { "h", UNITS_HOURS },
    { "days", UNITS_DAYS },

    /* Speed */
    { "meters-per-second", UNITS_METERS_PER_SECOND },
    { "m/s", UNITS_METERS_PER_SECOND },
    { "kilometers-per-hour", UNITS_KILOMETERS_PER_HOUR },
    { "km/h", UNITS_KILOMETERS_PER_HOUR },

    /* Angle */
    { "degrees-angular", UNITS_DEGREES_ANGULAR },
    { "degrees", UNITS_DEGREES_ANGULAR },
    { "deg", UNITS_DEGREES_ANGULAR },
    { "radians", UNITS_RADIANS },
    { "rad", UNITS_RADIANS },

    /* Mass */
    { "kilograms", UNITS_KILOGRAMS },
    { "kg", UNITS_KILOGRAMS },
    { "grams", UNITS_GRAMS },
    { "g", UNITS_GRAMS },
    { "pounds-mass", UNITS_POUNDS_MASS },
    { "lb", UNITS_POUNDS_MASS },

    /* pH */
    { "ph", UNITS_PH },
    { "pH", UNITS_PH },

    /* Signal strength */
    { "decibels", UNITS_DECIBELS },
    { "dB", UNITS_DECIBELS },
    { "dBm", UNITS_DECIBELS_MILLIVOLT },

    /* Count */
    { "counts", UNITS_NO_UNITS },
    { "count", UNITS_NO_UNITS },

    /* No units / dimensionless */
    { "no-units", UNITS_NO_UNITS },
    { "", UNITS_NO_UNITS },
    { NULL, UNITS_NO_UNITS }
};

/**
 * @brief Map a JSON units string to BACnet engineering units
 */
BACNET_ENGINEERING_UNITS units_map_to_bacnet(const char *units_str)
{
    const units_mapping_t *entry;

    if (!units_str || strlen(units_str) == 0) {
        return UNITS_NO_UNITS;
    }

    for (entry = units_table; entry->json_name != NULL; entry++) {
        if (strcasecmp(units_str, entry->json_name) == 0) {
            return entry->bacnet_units;
        }
    }

    return UNITS_NO_UNITS;
}

/**
 * @brief Get the JSON units string for a BACnet engineering unit
 */
const char *units_map_from_bacnet(BACNET_ENGINEERING_UNITS units)
{
    const units_mapping_t *entry;

    for (entry = units_table; entry->json_name != NULL; entry++) {
        if (entry->bacnet_units == units) {
            return entry->json_name;
        }
    }

    return "no-units";
}
