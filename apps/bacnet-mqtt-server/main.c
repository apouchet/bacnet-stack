/**
 * @file
 * @brief BACnet MQTT Server - Dynamic BACnet server fed by MQTT + JSON
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 *
 * This is the main entry point for the BACnet MQTT Server daemon.
 * It creates a BACnet server that dynamically creates and updates
 * BACnet objects based on incoming MQTT messages and JSON configuration.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/apdu.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bactext.h"
#include "bacnet/dcc.h"
#include "bacnet/iam.h"
#include "bacnet/npdu.h"
#include "bacnet/version.h"
/* BACnet basic services */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"
#include "bacnet/datetime.h"
/* BACnet objects */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/ai.h"
#include "bacnet/basic/object/csv.h"
#include "bacnet/basic/object/nc.h"
#include "bacnet/basic/object/netport.h"

/* Application modules */
#include "config.h"
#include "logging.h"
#include "sensor_map.h"
#include "sensor_def.h"
#include "object_registry.h"
#include "object_factory.h"
#include "mqtt_client.h"
#include "mqtt_processor.h"

/* Application version */
#define APP_VERSION "1.0.0"
#define APP_NAME "bacnet-mqtt-server"

/* Global state */
static volatile sig_atomic_t g_running = 1;
static app_config_t g_config;
static sensor_map_t g_sensor_map;
static object_registry_t g_registry;
static mqtt_processor_t g_processor;

/* BACnet timers */
static struct mstimer BACnet_Task_Timer;
static struct mstimer BACnet_TSM_Timer;
static struct mstimer BACnet_Address_Timer;
static struct mstimer BACnet_Object_Timer;
static struct mstimer Sensor_Map_Timer;

/* BACnet receive buffer */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

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

    mqtt_processor_handle_message(&g_processor, topic, payload, payload_len);
}

/**
 * @brief Initialize BACnet service handlers
 */
static void init_service_handlers(void)
{
    /* Initialize the Device Object with NULL for custom object table */
    Device_Init(NULL);

    /* Set device properties from config */
    Device_Set_Object_Instance_Number(g_config.bacnet.device_instance);
    Device_Object_Name_ANSI_Init(g_config.bacnet.device_name);
    Device_Set_Description(
        g_config.bacnet.description, strlen(g_config.bacnet.description));
    Device_Set_Location(
        g_config.bacnet.location, strlen(g_config.bacnet.location));
    /* Note: Device_Set_Vendor_Name is declared but not implemented in bacnet-stack.
       Vendor name is set via BACNET_VENDOR_NAME define at compile time. */
    Device_Set_Vendor_Identifier(g_config.bacnet.vendor_id);
    Device_Set_Model_Name(
        g_config.bacnet.model_name, strlen(g_config.bacnet.model_name));
    Device_Set_Application_Software_Version(
        g_config.bacnet.app_version, strlen(g_config.bacnet.app_version));

    /* Set up Who-Is / I-Am handlers */
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_WHO_IS, handler_who_is_who_am_i_unicast);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS, handler_who_has);

    /* Set the handler for unrecognized services */
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);

    /* ReadProperty and ReadPropertyMultiple - mandatory */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROP_MULTIPLE, handler_read_property_multiple);

    /* WriteProperty */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_WRITE_PROPERTY, handler_write_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_WRITE_PROP_MULTIPLE, handler_write_property_multiple);

    /* COV subscription */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_SUBSCRIBE_COV, handler_cov_subscribe);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_COV_NOTIFICATION, handler_ucov_notification);

    /* Time synchronization */
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION, handler_timesync_utc);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION, handler_timesync);

    /* Device communication control */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);

    /* Reinitialize device */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_REINITIALIZE_DEVICE, handler_reinitialize_device);

    /* ReadRange for trend logs */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_RANGE, handler_read_range);

#if defined(INTRINSIC_REPORTING)
    /* Alarm and event handlers */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_ACKNOWLEDGE_ALARM, handler_alarm_ack);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_GET_EVENT_INFORMATION, handler_get_event_information);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_GET_ALARM_SUMMARY, handler_get_alarm_summary);
#endif

    /* Initialize timers */
    mstimer_set(&BACnet_Task_Timer, 1000UL);
    mstimer_set(&BACnet_TSM_Timer, 50UL);
    mstimer_set(&BACnet_Address_Timer, 60UL * 1000UL);
    mstimer_set(&BACnet_Object_Timer, 100UL);
    mstimer_set(&Sensor_Map_Timer, 10000UL); /* Check sensor map every 10s */

    /* Initialize timesync callback */
    handler_timesync_set_callback_set(&datetime_timesync);

    LOG_INFO("BACnet service handlers initialized");
}

/**
 * @brief BACnet task - process pending operations
 */
static void bacnet_task(void)
{
    BACNET_ADDRESS src = { 0 };
    uint16_t pdu_len = 0;
    unsigned timeout = 1; /* milliseconds */
    uint32_t elapsed_milliseconds = 0;
    uint32_t elapsed_seconds = 0;

    /* Receive and process BACnet messages */
    pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);
    if (pdu_len) {
        npdu_handler(&src, &Rx_Buf[0], pdu_len);
    }

    /* Handle 1 second tasks */
    if (mstimer_expired(&BACnet_Task_Timer)) {
        mstimer_reset(&BACnet_Task_Timer);
        elapsed_milliseconds = mstimer_interval(&BACnet_Task_Timer);
        elapsed_seconds = elapsed_milliseconds / 1000;

        dcc_timer_seconds(elapsed_seconds);
        datalink_maintenance_timer(elapsed_seconds);
        dlenv_maintenance_timer(elapsed_seconds);
        handler_cov_timer_seconds(elapsed_seconds);

#if defined(INTRINSIC_REPORTING)
        Device_local_reporting();
#endif
    }

    /* TSM timer */
    if (mstimer_expired(&BACnet_TSM_Timer)) {
        mstimer_reset(&BACnet_TSM_Timer);
        elapsed_milliseconds = mstimer_interval(&BACnet_TSM_Timer);
        tsm_timer_milliseconds(elapsed_milliseconds);
    }

    /* Address cache timer */
    if (mstimer_expired(&BACnet_Address_Timer)) {
        mstimer_reset(&BACnet_Address_Timer);
        elapsed_milliseconds = mstimer_interval(&BACnet_Address_Timer);
        elapsed_seconds = elapsed_milliseconds / 1000;
        address_cache_timer(elapsed_seconds);
    }

    /* Object timer */
    if (mstimer_expired(&BACnet_Object_Timer)) {
        mstimer_reset(&BACnet_Object_Timer);
        elapsed_milliseconds = mstimer_interval(&BACnet_Object_Timer);
        Device_Timer((uint16_t)elapsed_milliseconds);
    }

    /* COV task */
    handler_cov_task();

    /* Sensor map reload check */
    if (mstimer_expired(&Sensor_Map_Timer)) {
        mstimer_reset(&Sensor_Map_Timer);
        sensor_map_check_reload(&g_sensor_map);
    }
}

/**
 * @brief Print startup banner
 */
static void print_banner(void)
{
    printf("\n");
    printf("======================================\n");
    printf("  %s v%s\n", APP_NAME, APP_VERSION);
    printf("  BACnet Stack Version: %s\n", BACNET_VERSION_TEXT);
    printf("======================================\n");
    printf("\n");
}

/**
 * @brief Main entry point
 */
int main(int argc, char *argv[])
{
    bool load_default_config = true;

    /* Initialize with defaults */
    config_init_defaults(&g_config);

    /* Parse command line arguments */
    if (!config_parse_args(&g_config, argc, argv)) {
        return 0; /* Help or version was printed */
    }

    /* Try to load default config if no config was specified */
    if (load_default_config) {
        config_load(&g_config, CONFIG_DEFAULT_PATH);
    }

    /* Initialize logging */
    logging_init(
        g_config.logging.level, g_config.logging.json_output,
        g_config.logging.log_file);

    print_banner();

    if (g_config.logging.level <= LOG_LEVEL_DEBUG) {
        config_print(&g_config);
    }

    /* Set up signal handlers */
    setup_signals();

    /* Initialize sensor map */
    if (!sensor_map_init(&g_sensor_map, g_config.paths.sensors_map)) {
        LOG_WARNING("Failed to load sensor map, continuing anyway");
    }

    /* Initialize object registry */
    if (!object_registry_init(&g_registry, 1)) {
        LOG_ERROR("Failed to initialize object registry");
        return 1;
    }

    /* Initialize object factory */
    if (!object_factory_init(&g_registry)) {
        LOG_ERROR("Failed to initialize object factory");
        return 1;
    }

    /* Initialize MQTT processor */
    if (!mqtt_processor_init(
            &g_processor, &g_sensor_map, &g_registry,
            g_config.paths.sensors_base)) {
        LOG_ERROR("Failed to initialize MQTT processor");
        return 1;
    }

    /* Initialize BACnet address bindings */
    address_init();

    /* Initialize BACnet service handlers */
    init_service_handlers();

    /* Initialize datalink layer */
    dlenv_init();
    atexit(datalink_cleanup);

    /* Initialize MQTT client */
    if (!mqtt_client_init(&g_config.mqtt)) {
        LOG_ERROR("Failed to initialize MQTT client");
        return 1;
    }

    /* Set MQTT message callback */
    mqtt_client_set_callback(mqtt_message_handler, NULL);

    /* Connect to MQTT broker */
    if (!mqtt_client_connect()) {
        LOG_ERROR("Failed to connect to MQTT broker");
        mqtt_client_shutdown();
        return 1;
    }

    LOG_INFO(
        "BACnet MQTT Server started (Device Instance: %u, UDP Port: %u)",
        g_config.bacnet.device_instance, g_config.bacnet.udp_port);

    /* Broadcast I-Am on startup */
    Send_I_Am(&Handler_Transmit_Buffer[0]);

    /* Main loop */
    while (g_running) {
        /* Process BACnet tasks */
        bacnet_task();

        /* Small sleep to prevent CPU spinning */
        usleep(1000); /* 1ms */
    }

    /* Shutdown */
    LOG_INFO("Shutting down...");

    mqtt_client_shutdown();
    object_factory_shutdown();
    object_registry_cleanup(&g_registry);
    sensor_map_cleanup(&g_sensor_map);
    mqtt_processor_cleanup(&g_processor);
    logging_shutdown();

    printf("Goodbye!\n");
    return 0;
}
