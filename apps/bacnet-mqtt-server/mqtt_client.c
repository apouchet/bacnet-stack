/**
 * @file
 * @brief MQTT client module implementation for BACnet MQTT Server
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>
#include "mqtt_client.h"
#include "logging.h"

/* Module state */
static struct mosquitto *g_mosq = NULL;
static mqtt_config_t g_config;
static mqtt_message_callback_t g_message_callback = NULL;
static void *g_callback_userdata = NULL;
static bool g_connected = false;

/**
 * @brief Mosquitto connect callback
 */
static void on_connect(
    struct mosquitto *mosq,
    void *userdata,
    int result)
{
    (void)mosq;
    (void)userdata;

    if (result == 0) {
        g_connected = true;
        LOG_INFO("Connected to MQTT broker: %s:%u", g_config.host, g_config.port);

        /* Subscribe to configured topic */
        if (strlen(g_config.topic) > 0) {
            int rc = mosquitto_subscribe(g_mosq, NULL, g_config.topic, g_config.qos);
            if (rc == MOSQ_ERR_SUCCESS) {
                LOG_INFO("Subscribed to topic: %s", g_config.topic);
            } else {
                LOG_ERROR("Failed to subscribe to topic: %s (error=%d)",
                    g_config.topic, rc);
            }
        }
    } else {
        g_connected = false;
        LOG_ERROR("Failed to connect to MQTT broker: %s (error=%d)",
            mosquitto_connack_string(result), result);
    }
}

/**
 * @brief Mosquitto disconnect callback
 */
static void on_disconnect(
    struct mosquitto *mosq,
    void *userdata,
    int reason)
{
    (void)mosq;
    (void)userdata;

    g_connected = false;

    if (reason == 0) {
        LOG_INFO("Disconnected from MQTT broker (clean)");
    } else {
        LOG_WARNING("Disconnected from MQTT broker (reason=%d), will reconnect",
            reason);
    }
}

/**
 * @brief Mosquitto message callback
 */
static void on_message(
    struct mosquitto *mosq,
    void *userdata,
    const struct mosquitto_message *msg)
{
    (void)mosq;
    (void)userdata;

    if (!msg || !msg->topic) {
        return;
    }

    LOG_DEBUG("Received message on topic: %s (len=%d)", msg->topic, msg->payloadlen);

    if (g_message_callback) {
        g_message_callback(
            msg->topic,
            msg->payload,
            (size_t)msg->payloadlen,
            g_callback_userdata);
    }
}

/**
 * @brief Mosquitto log callback
 */
static void on_log(
    struct mosquitto *mosq,
    void *userdata,
    int level,
    const char *str)
{
    (void)mosq;
    (void)userdata;

    switch (level) {
        case MOSQ_LOG_DEBUG:
            LOG_DEBUG("MQTT: %s", str);
            break;
        case MOSQ_LOG_INFO:
        case MOSQ_LOG_NOTICE:
            LOG_DEBUG("MQTT: %s", str);
            break;
        case MOSQ_LOG_WARNING:
            LOG_WARNING("MQTT: %s", str);
            break;
        case MOSQ_LOG_ERR:
            LOG_ERROR("MQTT: %s", str);
            break;
        default:
            break;
    }
}

/**
 * @brief Initialize MQTT client
 */
bool mqtt_client_init(const mqtt_config_t *config)
{
    int rc;

    if (!config) {
        return false;
    }

    /* Copy configuration */
    memcpy(&g_config, config, sizeof(mqtt_config_t));

    /* Initialize mosquitto library */
    rc = mosquitto_lib_init();
    if (rc != MOSQ_ERR_SUCCESS) {
        LOG_ERROR("Failed to initialize mosquitto library: %d", rc);
        return false;
    }

    /* Create mosquitto client */
    g_mosq = mosquitto_new(g_config.client_id, true, NULL);
    if (!g_mosq) {
        LOG_ERROR("Failed to create mosquitto client");
        mosquitto_lib_cleanup();
        return false;
    }

    /* Set callbacks */
    mosquitto_connect_callback_set(g_mosq, on_connect);
    mosquitto_disconnect_callback_set(g_mosq, on_disconnect);
    mosquitto_message_callback_set(g_mosq, on_message);
    mosquitto_log_callback_set(g_mosq, on_log);

    /* Set credentials if provided */
    if (strlen(g_config.username) > 0) {
        rc = mosquitto_username_pw_set(
            g_mosq, g_config.username,
            strlen(g_config.password) > 0 ? g_config.password : NULL);
        if (rc != MOSQ_ERR_SUCCESS) {
            LOG_ERROR("Failed to set MQTT credentials: %d", rc);
        }
    }

    /* Configure TLS if enabled */
    if (g_config.use_tls) {
        rc = mosquitto_tls_set(
            g_mosq,
            strlen(g_config.ca_cert) > 0 ? g_config.ca_cert : NULL,
            NULL,
            strlen(g_config.client_cert) > 0 ? g_config.client_cert : NULL,
            strlen(g_config.client_key) > 0 ? g_config.client_key : NULL,
            NULL);
        if (rc != MOSQ_ERR_SUCCESS) {
            LOG_ERROR("Failed to configure TLS: %d", rc);
        }
    }

    /* Enable automatic reconnection */
    mosquitto_reconnect_delay_set(g_mosq, 1, 30, true);

    LOG_INFO("MQTT client initialized (client_id=%s)", g_config.client_id);
    return true;
}

/**
 * @brief Connect to MQTT broker
 */
bool mqtt_client_connect(void)
{
    int rc;

    if (!g_mosq) {
        return false;
    }

    rc = mosquitto_connect_async(
        g_mosq, g_config.host, g_config.port, g_config.keepalive);
    if (rc != MOSQ_ERR_SUCCESS) {
        LOG_ERROR("Failed to start connection to %s:%u (error=%d)",
            g_config.host, g_config.port, rc);
        return false;
    }

    /* Start the network loop thread */
    rc = mosquitto_loop_start(g_mosq);
    if (rc != MOSQ_ERR_SUCCESS) {
        LOG_ERROR("Failed to start MQTT loop: %d", rc);
        return false;
    }

    LOG_INFO("Connecting to MQTT broker: %s:%u", g_config.host, g_config.port);
    return true;
}

/**
 * @brief Disconnect from MQTT broker
 */
void mqtt_client_disconnect(void)
{
    if (g_mosq) {
        mosquitto_disconnect(g_mosq);
    }
    g_connected = false;
}

/**
 * @brief Subscribe to a topic
 */
bool mqtt_client_subscribe(const char *topic, int qos)
{
    int rc;

    if (!g_mosq || !topic) {
        return false;
    }

    rc = mosquitto_subscribe(g_mosq, NULL, topic, qos);
    if (rc != MOSQ_ERR_SUCCESS) {
        LOG_ERROR("Failed to subscribe to topic: %s (error=%d)", topic, rc);
        return false;
    }

    LOG_INFO("Subscribed to topic: %s (qos=%d)", topic, qos);
    return true;
}

/**
 * @brief Set message callback
 */
void mqtt_client_set_callback(mqtt_message_callback_t callback, void *userdata)
{
    g_message_callback = callback;
    g_callback_userdata = userdata;
}

/**
 * @brief Process MQTT events (for single-threaded mode)
 */
bool mqtt_client_loop(int timeout_ms)
{
    int rc;

    if (!g_mosq) {
        return false;
    }

    rc = mosquitto_loop(g_mosq, timeout_ms, 1);
    if (rc != MOSQ_ERR_SUCCESS && rc != MOSQ_ERR_CONN_LOST &&
        rc != MOSQ_ERR_NO_CONN) {
        LOG_ERROR("MQTT loop error: %d", rc);
        return false;
    }

    return true;
}

/**
 * @brief Check if connected
 */
bool mqtt_client_is_connected(void)
{
    return g_connected;
}

/**
 * @brief Shutdown MQTT client
 */
void mqtt_client_shutdown(void)
{
    if (g_mosq) {
        mosquitto_loop_stop(g_mosq, true);
        mosquitto_disconnect(g_mosq);
        mosquitto_destroy(g_mosq);
        g_mosq = NULL;
    }

    mosquitto_lib_cleanup();
    g_connected = false;
    g_message_callback = NULL;
    g_callback_userdata = NULL;

    LOG_INFO("MQTT client shutdown");
}
