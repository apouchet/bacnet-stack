/**
 * @file
 * @brief Frame processor module for LoRaWAN MQTT Decoder Service
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 *
 * This module processes LoRaWAN frames, calls JavaScript decoders,
 * enriches frames with decoded data, and republishes them.
 */
#ifndef LORAWAN_DECODER_FRAME_PROCESSOR_H
#define LORAWAN_DECODER_FRAME_PROCESSOR_H

#include <stdbool.h>
#include <stddef.h>
#include "sensor_map.h"
#include "decoder_cache.h"

/**
 * @brief Frame direction
 */
typedef enum frame_direction {
    FRAME_DIRECTION_UPLINK,
    FRAME_DIRECTION_DOWNLINK,
    FRAME_DIRECTION_UNKNOWN
} frame_direction_t;

/**
 * @brief Frame processor context
 */
typedef struct frame_processor {
    sensor_map_t *sensor_map;
    decoder_cache_t *decoder_cache;
} frame_processor_t;

/**
 * @brief Processing statistics
 */
typedef struct frame_stats {
    unsigned long frames_received;
    unsigned long frames_decoded;
    unsigned long frames_forwarded;
    unsigned long frames_errors;
} frame_stats_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the frame processor
 * @param processor Pointer to processor context
 * @param sensor_map Pointer to sensor map
 * @param decoder_cache Pointer to decoder cache
 * @return true on success, false on failure
 */
bool frame_processor_init(
    frame_processor_t *processor,
    sensor_map_t *sensor_map,
    decoder_cache_t *decoder_cache);

/**
 * @brief Process an incoming MQTT message
 * @param processor Pointer to processor context
 * @param topic MQTT topic
 * @param payload Message payload (JSON)
 * @param payload_len Payload length
 * @return true if processed successfully, false on error
 */
bool frame_processor_handle_message(
    frame_processor_t *processor,
    const char *topic,
    const void *payload,
    size_t payload_len);

/**
 * @brief Get processing statistics
 * @param stats Output statistics structure
 */
void frame_processor_get_stats(frame_stats_t *stats);

/**
 * @brief Cleanup frame processor
 * @param processor Pointer to processor context
 */
void frame_processor_cleanup(frame_processor_t *processor);

/**
 * @brief Extract direction from topic
 * @param topic MQTT topic
 * @return Frame direction
 */
frame_direction_t frame_processor_get_direction(const char *topic);

#ifdef __cplusplus
}
#endif

#endif /* LORAWAN_DECODER_FRAME_PROCESSOR_H */
