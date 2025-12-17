/**
 * @file
 * @brief MQTT handler module for LoRaWAN MQTT Decoder Service
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 *
 * This module handles MQTT subscribe/publish operations using libmosquitto.
 */
#ifndef LORAWAN_DECODER_MQTT_HANDLER_H
#define LORAWAN_DECODER_MQTT_HANDLER_H

#include <stdbool.h>
#include <stddef.h>
#include "config.h"

/**
 * @brief Message callback function type
 */
typedef void (*mqtt_message_callback_t)(
    const char *topic,
    const void *payload,
    size_t payload_len,
    void *userdata);

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize MQTT client
 * @param config MQTT configuration
 * @return true on success, false on failure
 */
bool mqtt_handler_init(const mqtt_config_t *config);

/**
 * @brief Connect to MQTT broker
 * @return true on success, false on failure
 */
bool mqtt_handler_connect(void);

/**
 * @brief Disconnect from MQTT broker
 */
void mqtt_handler_disconnect(void);

/**
 * @brief Subscribe to a topic
 * @param topic Topic pattern to subscribe
 * @param qos Quality of service (0, 1, or 2)
 * @return true on success, false on failure
 */
bool mqtt_handler_subscribe(const char *topic, int qos);

/**
 * @brief Publish a message
 * @param topic Topic to publish to
 * @param payload Message payload
 * @param payload_len Payload length
 * @param qos Quality of service
 * @param retain Retain flag
 * @return true on success, false on failure
 */
bool mqtt_handler_publish(
    const char *topic,
    const void *payload,
    size_t payload_len,
    int qos,
    bool retain);

/**
 * @brief Set message callback
 * @param callback Callback function
 * @param userdata User data passed to callback
 */
void mqtt_handler_set_callback(mqtt_message_callback_t callback, void *userdata);

/**
 * @brief Check if connected
 * @return true if connected
 */
bool mqtt_handler_is_connected(void);

/**
 * @brief Shutdown MQTT client
 */
void mqtt_handler_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LORAWAN_DECODER_MQTT_HANDLER_H */
