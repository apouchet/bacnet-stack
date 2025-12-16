/**
 * @file
 * @brief MQTT client module for BACnet MQTT Server
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 *
 * This module handles MQTT connection, subscription, and message receiving.
 */
#ifndef BACNET_MQTT_CLIENT_H
#define BACNET_MQTT_CLIENT_H

#include <stdbool.h>
#include <stdint.h>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback function type for received messages
 * @param topic MQTT topic
 * @param payload Message payload
 * @param payload_len Payload length
 * @param userdata User data passed to callback
 */
typedef void (*mqtt_message_callback_t)(
    const char *topic,
    const void *payload,
    size_t payload_len,
    void *userdata);

/**
 * @brief Initialize the MQTT client
 * @param config MQTT configuration
 * @return true on success, false on failure
 */
bool mqtt_client_init(const mqtt_config_t *config);

/**
 * @brief Connect to the MQTT broker
 * @return true on success, false on failure
 */
bool mqtt_client_connect(void);

/**
 * @brief Disconnect from the MQTT broker
 */
void mqtt_client_disconnect(void);

/**
 * @brief Subscribe to a topic
 * @param topic Topic pattern to subscribe to
 * @param qos Quality of service level
 * @return true on success, false on failure
 */
bool mqtt_client_subscribe(const char *topic, int qos);

/**
 * @brief Set the message callback
 * @param callback Callback function
 * @param userdata User data to pass to callback
 */
void mqtt_client_set_callback(
    mqtt_message_callback_t callback,
    void *userdata);

/**
 * @brief Process MQTT events (non-blocking)
 * @param timeout_ms Timeout in milliseconds
 * @return true if events were processed, false on error
 */
bool mqtt_client_loop(int timeout_ms);

/**
 * @brief Check if connected to broker
 * @return true if connected
 */
bool mqtt_client_is_connected(void);

/**
 * @brief Shutdown the MQTT client
 */
void mqtt_client_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* BACNET_MQTT_CLIENT_H */
