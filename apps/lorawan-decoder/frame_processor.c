/**
 * @file
 * @brief Frame processor implementation for LoRaWAN MQTT Decoder Service
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 *
 * This module implements the core frame processing logic:
 * 1. Parse incoming JSON frame
 * 2. Extract devEUI and data
 * 3. Look up sensor mapping
 * 4. Load/cache decoder
 * 5. Execute JavaScript decoder
 * 6. Enrich frame with decoded data
 * 7. Republish to output topic
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include "frame_processor.h"
#include "mqtt_handler.h"
#include "js_engine.h"
#include "logging.h"
#include "config.h"

/* Statistics */
static frame_stats_t g_stats = {0};

/**
 * @brief Extract direction from topic
 *
 * Topic format: lora/+/+/+/up or lora/+/+/+/down
 */
frame_direction_t frame_processor_get_direction(const char *topic)
{
    const char *last_slash = NULL;

    if (!topic) {
        return FRAME_DIRECTION_UNKNOWN;
    }

    /* Find last component of topic */
    last_slash = strrchr(topic, '/');
    if (!last_slash) {
        return FRAME_DIRECTION_UNKNOWN;
    }

    last_slash++; /* Skip the slash */

    if (strcmp(last_slash, "up") == 0) {
        return FRAME_DIRECTION_UPLINK;
    } else if (strcmp(last_slash, "down") == 0) {
        return FRAME_DIRECTION_DOWNLINK;
    }

    return FRAME_DIRECTION_UNKNOWN;
}

/**
 * @brief Build output topic
 */
static bool build_output_topic(
    char *topic,
    size_t topic_size,
    const char *deveui,
    frame_direction_t direction)
{
    const char *dir_str = NULL;
    int ret;

    if (!topic || !deveui || topic_size == 0) {
        return false;
    }

    if (direction == FRAME_DIRECTION_UPLINK) {
        dir_str = "up";
    } else if (direction == FRAME_DIRECTION_DOWNLINK) {
        dir_str = "down";
    } else {
        return false;
    }

    ret = snprintf(topic, topic_size, "%s/%s/%s", 
                  MQTT_TOPIC_PUBLISH_BASE, deveui, dir_str);
    
    return (ret > 0 && (size_t)ret < topic_size);
}

/**
 * @brief Initialize frame processor
 */
bool frame_processor_init(
    frame_processor_t *processor,
    sensor_map_t *sensor_map,
    decoder_cache_t *decoder_cache)
{
    if (!processor) {
        return false;
    }

    memset(processor, 0, sizeof(frame_processor_t));
    memset(&g_stats, 0, sizeof(frame_stats_t));

    processor->sensor_map = sensor_map;
    processor->decoder_cache = decoder_cache;

    LOG_INFO("Frame processor initialized");
    return true;
}

/**
 * @brief Process an incoming MQTT message
 */
bool frame_processor_handle_message(
    frame_processor_t *processor,
    const char *topic,
    const void *payload,
    size_t payload_len)
{
    struct json_object *root = NULL;
    struct json_object *deveui_obj = NULL;
    struct json_object *data_obj = NULL;
    struct json_object *decoded_obj = NULL;
    const char *deveui = NULL;
    const char *data = NULL;
    const sensor_map_entry_t *sensor_entry = NULL;
    js_context_t *js_ctx = NULL;
    js_result_t js_result = {0};
    frame_direction_t direction;
    char output_topic[256];
    const char *output_json = NULL;
    bool result = false;
    char *payload_str = NULL;

    if (!processor || !topic || !payload || payload_len == 0) {
        return false;
    }

    g_stats.frames_received++;

    /* Determine direction from topic */
    direction = frame_processor_get_direction(topic);
    if (direction == FRAME_DIRECTION_UNKNOWN) {
        LOG_DEBUG("Unknown direction in topic: %s", topic);
        g_stats.frames_errors++;
        return false;
    }

    /* Create null-terminated string from payload */
    payload_str = (char *)malloc(payload_len + 1);
    if (!payload_str) {
        LOG_ERROR("Failed to allocate memory for payload");
        g_stats.frames_errors++;
        return false;
    }
    memcpy(payload_str, payload, payload_len);
    payload_str[payload_len] = '\0';

    /* Parse JSON */
    root = json_tokener_parse(payload_str);
    free(payload_str);
    payload_str = NULL;

    if (!root) {
        LOG_WARNING("Invalid JSON payload, discarding");
        g_stats.frames_errors++;
        return false;
    }

    /* Extract devEUI */
    if (!json_object_object_get_ex(root, "deveui", &deveui_obj) ||
        !json_object_is_type(deveui_obj, json_type_string)) {
        LOG_DEBUG("No deveui in message, forwarding unchanged");
        goto forward_unchanged;
    }
    deveui = json_object_get_string(deveui_obj);

    /* Extract data */
    if (!json_object_object_get_ex(root, "data", &data_obj) ||
        !json_object_is_type(data_obj, json_type_string)) {
        LOG_DEBUG("No data in message from %s, forwarding unchanged", deveui);
        goto forward_unchanged;
    }
    data = json_object_get_string(data_obj);

    /* Look up sensor mapping */
    if (processor->sensor_map) {
        sensor_entry = sensor_map_lookup(processor->sensor_map, deveui);
    }

    if (!sensor_entry) {
        LOG_DEBUG("No sensor mapping for %s, forwarding unchanged", deveui);
        goto forward_unchanged;
    }

    /* Get decoder from cache */
    if (processor->decoder_cache) {
        js_ctx = decoder_cache_get(processor->decoder_cache, sensor_entry->sensor);
    }

    if (!js_ctx) {
        LOG_DEBUG("No decoder for sensor %s, forwarding unchanged", 
                 sensor_entry->sensor);
        goto forward_unchanged;
    }

    /* Execute decoder based on direction */
    if (direction == FRAME_DIRECTION_UPLINK) {
        if (!js_has_decode_uplink(js_ctx)) {
            LOG_DEBUG("Decoder has no decodeUplink function");
            goto forward_unchanged;
        }

        if (!js_decode_uplink(js_ctx, data, &js_result)) {
            LOG_WARNING("Decoder error for %s: %s", deveui, js_result.error_msg);
            
            /* Add error to frame */
            json_object_object_add(root, "decoderError", 
                json_object_new_string(js_result.error_msg));
            goto forward_with_error;
        }
    } else {
        /* For downlink, we might use encodeDownlink if available */
        if (js_has_encode_downlink(js_ctx)) {
            if (!js_encode_downlink(js_ctx, data, &js_result)) {
                LOG_WARNING("Encoder error for %s: %s", deveui, js_result.error_msg);
                json_object_object_add(root, "decoderError", 
                    json_object_new_string(js_result.error_msg));
                goto forward_with_error;
            }
        } else if (js_has_decode_uplink(js_ctx)) {
            /* Fall back to decodeUplink for downlink */
            if (!js_decode_uplink(js_ctx, data, &js_result)) {
                LOG_WARNING("Decoder error for %s: %s", deveui, js_result.error_msg);
                json_object_object_add(root, "decoderError", 
                    json_object_new_string(js_result.error_msg));
                goto forward_with_error;
            }
        } else {
            LOG_DEBUG("No decode/encode function available");
            goto forward_unchanged;
        }
    }

    /* Add decoded data to frame */
    if (js_result.json_result) {
        struct json_object *parsed_result = json_tokener_parse(js_result.json_result);
        if (parsed_result) {
            decoded_obj = json_object_new_object();
            if (decoded_obj) {
                json_object_object_add(decoded_obj, "data", parsed_result);
                if (direction == FRAME_DIRECTION_UPLINK) {
                    json_object_object_add(root, "uplinkDecoded", decoded_obj);
                } else {
                    json_object_object_add(root, "downlinkDecoded", decoded_obj);
                }
            } else {
                json_object_put(parsed_result);
            }
        }
    }

    g_stats.frames_decoded++;

forward_with_error:
forward_unchanged:
    /* Build output topic */
    if (!build_output_topic(output_topic, sizeof(output_topic), 
                           deveui ? deveui : "unknown", direction)) {
        LOG_ERROR("Failed to build output topic");
        goto cleanup;
    }

    /* Serialize and publish */
    output_json = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PLAIN);
    if (!output_json) {
        LOG_ERROR("Failed to serialize output JSON");
        goto cleanup;
    }

    if (mqtt_handler_publish(output_topic, output_json, strlen(output_json), 
                            0, false)) {
        g_stats.frames_forwarded++;
        LOG_DEBUG("Published to %s", output_topic);
        result = true;
    } else {
        g_stats.frames_errors++;
    }

cleanup:
    /* Note: output_json is managed by json-c and freed with json_object_put */
    js_result_free(&js_result);
    if (root) {
        json_object_put(root);
    }

    return result;
}

/**
 * @brief Get processing statistics
 */
void frame_processor_get_stats(frame_stats_t *stats)
{
    if (stats) {
        memcpy(stats, &g_stats, sizeof(frame_stats_t));
    }
}

/**
 * @brief Cleanup frame processor
 */
void frame_processor_cleanup(frame_processor_t *processor)
{
    if (!processor) {
        return;
    }

    LOG_INFO("Frame processor stats: received=%lu, decoded=%lu, "
            "forwarded=%lu, errors=%lu",
            g_stats.frames_received, g_stats.frames_decoded,
            g_stats.frames_forwarded, g_stats.frames_errors);

    memset(processor, 0, sizeof(frame_processor_t));
}
