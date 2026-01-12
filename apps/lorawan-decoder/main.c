/**
 * @file
 * @brief LoRaWAN MQTT Decoder Service - Main Entry Point
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 *
 * This is the main entry point for the LoRaWAN MQTT Decoder Service.
 * The service:
 * - Listens to LoRaWAN MQTT traffic (application/+/device/+/event/up and down)
 * - Dynamically loads JavaScript decoders based on device mapping
 * - Decodes payloads and enriches original frames
 * - Republishes enriched frames to scada/lorawan/{deveui}/up|down
 *
 * This is a production-grade service designed for industrial SCADA environments.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

/* Application modules */
#include "config.h"
#include "logging.h"
#include "sensor_map.h"
#include "decoder_cache.h"
#include "js_engine.h"
#include "mqtt_handler.h"
#include "frame_processor.h"

/* Application version */
#define APP_VERSION "1.0.0"
#define APP_NAME "lorawan-decoder"

/* Sensor map reload interval in seconds */
#define SENSOR_MAP_RELOAD_INTERVAL 10

/* Global state */
static volatile sig_atomic_t g_running = 1;
static app_config_t g_config;
static sensor_map_t g_sensor_map;
static decoder_cache_t g_decoder_cache;
static frame_processor_t g_processor;

/* Timers */
static time_t g_last_sensor_map_check = 0;

/**
 * @brief Signal handler for graceful shutdown
 */
static void signal_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTERM) {
        LOG_INFO("Received signal %d, shutting down...", signum);
        g_running = 0;
    }
}

/**
 * @brief Set up signal handlers
 */
static void setup_signals(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

/**
 * @brief MQTT message callback
 */
static void mqtt_message_handler(
    const char *topic,
    const void *payload,
    size_t payload_len,
    void *userdata)
{
    (void)userdata;

    frame_processor_handle_message(&g_processor, topic, payload, payload_len);
}

/**
 * @brief Print startup banner
 */
static void print_banner(void)
{
    printf("\n");
    printf("======================================\n");
    printf("  %s v%s\n", APP_NAME, APP_VERSION);
    printf("  LoRaWAN MQTT Decoder Service\n");
    printf("======================================\n");
    printf("\n");
}

/**
 * @brief Periodic tasks
 */
static void periodic_tasks(void)
{
    time_t now = time(NULL);

    /* Check sensor map for reload */
    if (now - g_last_sensor_map_check >= SENSOR_MAP_RELOAD_INTERVAL) {
        g_last_sensor_map_check = now;
        sensor_map_check_reload(&g_sensor_map);
    }
}

/* MQTT connection timeout in seconds */
#define MQTT_CONNECTION_TIMEOUT 10

/**
 * @brief Wait for MQTT connection with timeout
 */
static bool wait_for_mqtt_connection(int timeout_seconds)
{
    int elapsed_100ms = 0;
    int timeout_100ms = timeout_seconds * 10; /* Convert to 100ms intervals */

    while (!mqtt_handler_is_connected() && elapsed_100ms < timeout_100ms) {
        usleep(100000); /* 100ms */
        elapsed_100ms++;
        if (elapsed_100ms % 10 == 0) {
            LOG_INFO("Waiting for MQTT connection... (%d/%d s)", 
                    elapsed_100ms / 10, timeout_seconds);
        }
    }

    return mqtt_handler_is_connected();
}

/**
 * @brief Subscribe to LoRaWAN topics
 */
static bool subscribe_to_topics(void)
{
    bool success = true;

    if (!mqtt_handler_subscribe(MQTT_TOPIC_UPLINK, g_config.mqtt.qos)) {
        LOG_ERROR("Failed to subscribe to uplink topic");
        success = false;
    }

    if (!mqtt_handler_subscribe(MQTT_TOPIC_DOWNLINK, g_config.mqtt.qos)) {
        LOG_ERROR("Failed to subscribe to downlink topic");
        success = false;
    }

    return success;
}

/**
 * @brief Main entry point
 */
int main(int argc, char *argv[])
{
    bool sensor_map_loaded = false;

    /* Initialize with defaults */
    config_init_defaults(&g_config);

    /* Parse command line arguments */
    if (!config_parse_args(&g_config, argc, argv)) {
        return 0; /* Help or version was printed */
    }

    /* Initialize logging */
    logging_init(
        g_config.logging.level, 
        g_config.logging.json_output,
        g_config.logging.log_file);

    print_banner();

    if (g_config.logging.level <= LOG_LEVEL_DEBUG) {
        config_print(&g_config);
    }

    /* Set up signal handlers */
    setup_signals();

    /* Initialize JavaScript engine */
    if (!js_engine_init()) {
        LOG_ERROR("Failed to initialize JavaScript engine");
        return 1;
    }

    /* Initialize sensor map */
    if (sensor_map_init(&g_sensor_map, g_config.paths.sensors_map)) {
        sensor_map_loaded = true;
        LOG_INFO("Loaded %zu sensor mappings", sensor_map_count(&g_sensor_map));
    } else {
        LOG_WARNING("Failed to load sensor map from %s, continuing anyway",
                   g_config.paths.sensors_map);
    }

    /* Initialize decoder cache */
    if (!decoder_cache_init(&g_decoder_cache, 
                           g_config.paths.sensors_decoder,
                           g_config.paths.sensors_decoder_alt)) {
        LOG_ERROR("Failed to initialize decoder cache");
        goto cleanup;
    }

    /* Initialize frame processor */
    if (!frame_processor_init(&g_processor, 
                             sensor_map_loaded ? &g_sensor_map : NULL,
                             &g_decoder_cache)) {
        LOG_ERROR("Failed to initialize frame processor");
        goto cleanup;
    }

    /* Initialize MQTT client */
    if (!mqtt_handler_init(&g_config.mqtt)) {
        LOG_ERROR("Failed to initialize MQTT client");
        goto cleanup;
    }

    /* Set MQTT message callback */
    mqtt_handler_set_callback(mqtt_message_handler, NULL);

    /* Connect to MQTT broker */
    if (!mqtt_handler_connect()) {
        LOG_ERROR("Failed to connect to MQTT broker");
        goto cleanup;
    }

    /* Wait for connection */
    if (!wait_for_mqtt_connection(MQTT_CONNECTION_TIMEOUT)) {
        LOG_ERROR("MQTT connection timeout");
        goto cleanup;
    }

    /* Subscribe to LoRaWAN topics */
    if (!subscribe_to_topics()) {
        LOG_WARNING("Some subscriptions failed, continuing anyway");
    }

    LOG_INFO("LoRaWAN MQTT Decoder Service started");
    LOG_INFO("Listening on: %s, %s", MQTT_TOPIC_UPLINK, MQTT_TOPIC_DOWNLINK);
    LOG_INFO("Publishing to: %s/{deveui}/up|down", MQTT_TOPIC_PUBLISH_BASE);

    g_last_sensor_map_check = time(NULL);

    /* Main loop */
    while (g_running) {
        /* Run periodic tasks */
        periodic_tasks();

        /* Sleep to prevent CPU spinning */
        usleep(100000); /* 100ms */
    }

    /* Print final statistics */
    {
        frame_stats_t stats;
        frame_processor_get_stats(&stats);
        LOG_INFO("Final stats: received=%lu, decoded=%lu, forwarded=%lu, errors=%lu",
                stats.frames_received, stats.frames_decoded,
                stats.frames_forwarded, stats.frames_errors);
    }

cleanup:
    /* Shutdown */
    LOG_INFO("Shutting down...");

    mqtt_handler_shutdown();
    frame_processor_cleanup(&g_processor);
    decoder_cache_cleanup(&g_decoder_cache);
    sensor_map_cleanup(&g_sensor_map);
    js_engine_shutdown();
    logging_shutdown();

    printf("Goodbye!\n");
    return 0;
}
