/**
 * @file
 * @brief BACnet Client Library API
 * @author BACnet Stack Contributors
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_CLIENT_H
#define BACNET_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include "bacnet/bacdef.h"
#include "bacnet/bacenum.h"
#include "bacnet/bacapp.h"
#include "bacnet/rp.h"
#include "bacnet/cov.h"
#include "bacnet/event.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Error codes for client operations */
typedef enum {
    BACNET_CLIENT_OK = 0,
    BACNET_CLIENT_ERROR_INVALID_PARAM,
    BACNET_CLIENT_ERROR_TIMEOUT,
    BACNET_CLIENT_ERROR_NO_DEVICE,
    BACNET_CLIENT_ERROR_NETWORK,
    BACNET_CLIENT_ERROR_REJECTED,
    BACNET_CLIENT_ERROR_ABORTED,
    BACNET_CLIENT_ERROR_BACNET,
    BACNET_CLIENT_ERROR_NOT_INITIALIZED,
    BACNET_CLIENT_ERROR_BUSY
} bacnet_client_error_t;

/** Log levels */
typedef enum {
    BACNET_LOG_ERROR = 0,
    BACNET_LOG_WARN,
    BACNET_LOG_INFO,
    BACNET_LOG_DEBUG
} bacnet_log_level_t;

/** Client configuration structure */
typedef struct {
    uint32_t device_instance;       /**< Our device instance number */
    uint16_t udp_port;              /**< UDP port (default 47808) */
    char *interface;                /**< Network interface name (NULL for auto) */
    uint32_t timeout_ms;            /**< APDU timeout in milliseconds */
    uint8_t retries;                /**< Number of retries */
    bacnet_log_level_t log_level;   /**< Logging level */
    bool json_output;               /**< Output in JSON format */
} bacnet_client_config_t;

/** Device information from I-Am */
typedef struct {
    uint32_t device_instance;
    unsigned max_apdu;
    int segmentation;
    uint16_t vendor_id;
    BACNET_ADDRESS address;
} bacnet_device_info_t;

/** Read property result */
typedef struct {
    BACNET_OBJECT_TYPE object_type;
    uint32_t object_instance;
    BACNET_PROPERTY_ID property;
    int32_t array_index;
    BACNET_APPLICATION_DATA_VALUE *value;
    int value_count;
    bacnet_client_error_t error;
    BACNET_ERROR_CLASS error_class;
    BACNET_ERROR_CODE error_code;
} bacnet_read_result_t;

/** COV notification data */
typedef struct {
    uint32_t device_instance;
    BACNET_OBJECT_TYPE object_type;
    uint32_t object_instance;
    uint32_t process_id;
    bool confirmed;
    BACNET_PROPERTY_VALUE *values;
} bacnet_cov_notification_t;

/** Callback type definitions */
typedef void (*bacnet_discover_callback_t)(
    const bacnet_device_info_t *device, void *context);
typedef void (*bacnet_read_callback_t)(
    const bacnet_read_result_t *result, void *context);
typedef void (*bacnet_write_callback_t)(
    bacnet_client_error_t error, void *context);
typedef void (*bacnet_cov_callback_t)(
    const bacnet_cov_notification_t *notification, void *context);
typedef void (*bacnet_event_callback_t)(
    const BACNET_EVENT_NOTIFICATION_DATA *event, void *context);

/**
 * @brief Get default client configuration
 * @param config Pointer to configuration structure to fill
 */
void bacnet_client_config_default(bacnet_client_config_t *config);

/**
 * @brief Initialize the BACnet client
 * @param config Pointer to configuration structure (NULL for defaults)
 * @return BACNET_CLIENT_OK on success, error code otherwise
 */
bacnet_client_error_t bacnet_client_init(const bacnet_client_config_t *config);

/**
 * @brief Shutdown the BACnet client and release resources
 */
void bacnet_client_shutdown(void);

/**
 * @brief Check if client is initialized
 * @return true if initialized
 */
bool bacnet_client_is_initialized(void);

/**
 * @brief Run the client event loop for a specified time
 * @param timeout_ms Time to run in milliseconds
 * @return Number of packets processed
 */
int bacnet_client_run(unsigned timeout_ms);

/**
 * @brief Send Who-Is broadcast and collect responses
 * @param min_instance Minimum device instance (-1 for all)
 * @param max_instance Maximum device instance (-1 for all)
 * @param timeout_ms Time to wait for responses
 * @param callback Function to call for each discovered device
 * @param context User context passed to callback
 * @return BACNET_CLIENT_OK on success
 */
bacnet_client_error_t bacnet_client_discover(
    int32_t min_instance,
    int32_t max_instance,
    unsigned timeout_ms,
    bacnet_discover_callback_t callback,
    void *context);

/**
 * @brief Read a property from a device
 * @param device_instance Target device instance
 * @param object_type Object type
 * @param object_instance Object instance
 * @param property Property identifier
 * @param array_index Array index (BACNET_ARRAY_ALL for entire array)
 * @param result Pointer to result structure
 * @return BACNET_CLIENT_OK on success
 */
bacnet_client_error_t bacnet_client_read(
    uint32_t device_instance,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property,
    int32_t array_index,
    bacnet_read_result_t *result);

/**
 * @brief Write a property to a device
 * @param device_instance Target device instance
 * @param object_type Object type
 * @param object_instance Object instance
 * @param property Property identifier
 * @param array_index Array index (BACNET_ARRAY_ALL for entire property)
 * @param value Value to write
 * @param priority Write priority (0 for no priority)
 * @return BACNET_CLIENT_OK on success
 */
bacnet_client_error_t bacnet_client_write(
    uint32_t device_instance,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property,
    int32_t array_index,
    BACNET_APPLICATION_DATA_VALUE *value,
    uint8_t priority);

/**
 * @brief Subscribe to COV notifications
 * @param device_instance Target device instance
 * @param object_type Object type
 * @param object_instance Object instance
 * @param confirmed Use confirmed notifications
 * @param lifetime Subscription lifetime in seconds
 * @param cov_increment COV increment (0 for default)
 * @param callback Function to call for notifications
 * @param context User context passed to callback
 * @return Process ID for the subscription, or 0 on error
 */
uint32_t bacnet_client_subscribe_cov(
    uint32_t device_instance,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    bool confirmed,
    uint32_t lifetime,
    float cov_increment,
    bacnet_cov_callback_t callback,
    void *context);

/**
 * @brief Unsubscribe from COV notifications
 * @param device_instance Target device instance
 * @param process_id Process ID from subscribe
 * @param object_type Object type
 * @param object_instance Object instance
 * @return BACNET_CLIENT_OK on success
 */
bacnet_client_error_t bacnet_client_unsubscribe_cov(
    uint32_t device_instance,
    uint32_t process_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance);

/**
 * @brief Send time synchronization
 * @param device_instance Target device instance (BACNET_MAX_INSTANCE for broadcast)
 * @param utc Use UTC time synchronization
 * @return BACNET_CLIENT_OK on success
 */
bacnet_client_error_t bacnet_client_time_sync(
    uint32_t device_instance, bool utc);

/**
 * @brief Read a file from a device
 * @param device_instance Target device instance
 * @param file_instance File object instance
 * @param local_filename Local file to save to
 * @return BACNET_CLIENT_OK on success
 */
bacnet_client_error_t bacnet_client_read_file(
    uint32_t device_instance,
    uint32_t file_instance,
    const char *local_filename);

/**
 * @brief Write a file to a device
 * @param device_instance Target device instance
 * @param file_instance File object instance
 * @param local_filename Local file to read from
 * @return BACNET_CLIENT_OK on success
 */
bacnet_client_error_t bacnet_client_write_file(
    uint32_t device_instance,
    uint32_t file_instance,
    const char *local_filename);

/**
 * @brief Send reinitialize device request
 * @param device_instance Target device instance
 * @param state Reinitialized state
 * @param password Optional password (NULL if not needed)
 * @return BACNET_CLIENT_OK on success
 */
bacnet_client_error_t bacnet_client_reinitialize(
    uint32_t device_instance,
    BACNET_REINITIALIZED_STATE state,
    const char *password);

/**
 * @brief Send device communication control request
 * @param device_instance Target device instance
 * @param state Enable/disable state
 * @param timeout_minutes Duration in minutes (0 for infinite)
 * @param password Optional password (NULL if not needed)
 * @return BACNET_CLIENT_OK on success
 */
bacnet_client_error_t bacnet_client_device_comm_control(
    uint32_t device_instance,
    BACNET_COMMUNICATION_ENABLE_DISABLE state,
    uint16_t timeout_minutes,
    const char *password);

/**
 * @brief Register as a foreign device with a BBMD
 * @param bbmd_address BBMD IP address string
 * @param bbmd_port BBMD port
 * @param ttl Time-to-live in seconds
 * @return BACNET_CLIENT_OK on success
 */
bacnet_client_error_t bacnet_client_register_foreign_device(
    const char *bbmd_address,
    uint16_t bbmd_port,
    uint16_t ttl);

/**
 * @brief Unregister as a foreign device
 * @return BACNET_CLIENT_OK on success
 */
bacnet_client_error_t bacnet_client_unregister_foreign_device(void);

/**
 * @brief Acknowledge an alarm
 * @param device_instance Target device instance
 * @param object_type Object type
 * @param object_instance Object instance
 * @param process_id Process identifier
 * @param event_state Event state to acknowledge
 * @param timestamp Timestamp from notification
 * @param ack_source Acknowledgment source string
 * @return BACNET_CLIENT_OK on success
 */
bacnet_client_error_t bacnet_client_ack_alarm(
    uint32_t device_instance,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    uint32_t process_id,
    BACNET_EVENT_STATE event_state,
    BACNET_TIMESTAMP *timestamp,
    const char *ack_source);

/**
 * @brief Set event notification callback
 * @param callback Function to call for event notifications
 * @param context User context passed to callback
 */
void bacnet_client_set_event_callback(
    bacnet_event_callback_t callback, void *context);

/**
 * @brief Get error string for error code
 * @param error Error code
 * @return Human-readable error string
 */
const char *bacnet_client_error_string(bacnet_client_error_t error);

/**
 * @brief Set log level
 * @param level New log level
 */
void bacnet_client_set_log_level(bacnet_log_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* BACNET_CLIENT_H */
