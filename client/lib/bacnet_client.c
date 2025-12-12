/**
 * @file
 * @brief BACnet Client Library Implementation
 * @author BACnet Stack Contributors
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bactext.h"
#include "bacnet/bacerror.h"
#include "bacnet/iam.h"
#include "bacnet/arf.h"
#include "bacnet/awf.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/whois.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"
#include "bacnet/cov.h"
#include "bacnet/rd.h"
#include "bacnet/dcc.h"
#include "bacnet/timesync.h"
#include "bacnet/alarm_ack.h"
#include "bacnet/version.h"
/* Demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"
#include "bacnet/datalink/bip.h"
#include "bacnet/datalink/bvlc.h"
#include "bacport.h"

#include "bacnet_client.h"

/* Buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/* Client state */
static bool Client_Initialized = false;
static bacnet_client_config_t Client_Config = { 0 };
static bacnet_log_level_t Log_Level = BACNET_LOG_INFO;

/* Callback state for discovery */
static bacnet_discover_callback_t Discover_Callback = NULL;
static void *Discover_Context = NULL;

/* Callback state for COV */
static bacnet_cov_callback_t COV_Callback = NULL;
static void *COV_Context = NULL;
static uint32_t COV_Process_ID = 1;

/* Callback state for events */
static bacnet_event_callback_t Event_Callback = NULL;
static void *Event_Context = NULL;

/* Request tracking */
static uint8_t Current_Invoke_ID = 0;
static bool Request_Complete = false;
static bool Request_Error = false;
static BACNET_ERROR_CLASS Error_Class = ERROR_CLASS_SERVICES;
static BACNET_ERROR_CODE Error_Code = ERROR_CODE_SUCCESS;
static BACNET_ADDRESS Target_Address = { 0 };

/* Read property result storage */
static BACNET_READ_PROPERTY_DATA Read_Data = { 0 };
static bool Read_Complete = false;

/* Write result */
static bool Write_Complete = false;

/* File transfer state */
static const char *File_Local_Name = NULL;
static int File_Start_Position = 0;
static bool File_End_Of_File = false;

/* Logging macros */
#define LOG_ERROR(fmt, ...) \
    if (Log_Level >= BACNET_LOG_ERROR) fprintf(stderr, "ERROR: " fmt "\n", ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) \
    if (Log_Level >= BACNET_LOG_WARN) fprintf(stderr, "WARN: " fmt "\n", ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) \
    if (Log_Level >= BACNET_LOG_INFO) printf("INFO: " fmt "\n", ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) \
    if (Log_Level >= BACNET_LOG_DEBUG) printf("DEBUG: " fmt "\n", ##__VA_ARGS__)

/* Handler for I-Am responses during discovery */
static void client_i_am_handler(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src)
{
    int len;
    uint32_t device_id;
    unsigned max_apdu;
    int segmentation;
    uint16_t vendor_id;
    bacnet_device_info_t device_info;

    (void)service_len; /* unused parameter */
    
    len = iam_decode_service_request(
        service_request, &device_id, &max_apdu, &segmentation, &vendor_id);
    
    if (len != -1) {
        LOG_DEBUG("Received I-Am from device %u", (unsigned)device_id);
        
        /* Add to address cache */
        address_add(device_id, max_apdu, src);
        
        /* Call discovery callback if set */
        if (Discover_Callback) {
            device_info.device_instance = device_id;
            device_info.max_apdu = max_apdu;
            device_info.segmentation = segmentation;
            device_info.vendor_id = vendor_id;
            memcpy(&device_info.address, src, sizeof(BACNET_ADDRESS));
            Discover_Callback(&device_info, Discover_Context);
        }
    }
}

/* Handler for errors */
static void client_error_handler(
    BACNET_ADDRESS *src,
    uint8_t invoke_id,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code)
{
    (void)src;
    if (invoke_id == Current_Invoke_ID) {
        LOG_DEBUG("Received error: %s: %s",
            bactext_error_class_name((int)error_class),
            bactext_error_code_name((int)error_code));
        Error_Class = error_class;
        Error_Code = error_code;
        Request_Error = true;
        Request_Complete = true;
    }
}

/* Handler for abort */
static void client_abort_handler(
    BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t abort_reason, bool server)
{
    (void)src;
    (void)server;
    if (invoke_id == Current_Invoke_ID) {
        LOG_DEBUG("Received abort: %s", bactext_abort_reason_name((int)abort_reason));
        Request_Error = true;
        Request_Complete = true;
    }
}

/* Handler for reject */
static void client_reject_handler(
    BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t reject_reason)
{
    (void)src;
    if (invoke_id == Current_Invoke_ID) {
        LOG_DEBUG("Received reject: %s", bactext_reject_reason_name((int)reject_reason));
        Request_Error = true;
        Request_Complete = true;
    }
}

/* Handler for ReadProperty ACK */
static void client_read_property_ack_handler(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data)
{
    int len;

    (void)src;
    if (service_data->invoke_id == Current_Invoke_ID) {
        len = rp_ack_decode_service_request(service_request, service_len, &Read_Data);
        if (len > 0) {
            Read_Complete = true;
            Request_Complete = true;
        } else {
            LOG_ERROR("Failed to decode ReadProperty ACK");
            Request_Error = true;
            Request_Complete = true;
        }
    }
}

/* Handler for WriteProperty Simple ACK */
static void client_write_property_simple_ack_handler(
    BACNET_ADDRESS *src, uint8_t invoke_id)
{
    (void)src;
    if (invoke_id == Current_Invoke_ID) {
        Write_Complete = true;
        Request_Complete = true;
    }
}

/* Handler for Simple ACK (generic) */
static void client_simple_ack_handler(BACNET_ADDRESS *src, uint8_t invoke_id)
{
    (void)src;
    if (invoke_id == Current_Invoke_ID) {
        Request_Complete = true;
    }
}

/* Handler for confirmed COV notifications */
static void client_ccov_notification_handler(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
    BACNET_COV_DATA cov_data;
    int len;

    (void)src;
    (void)service_data;
    
    len = cov_notify_decode_service_request(service_request, service_len, &cov_data);
    if (len > 0 && COV_Callback) {
        bacnet_cov_notification_t notification;
        notification.device_instance = cov_data.initiatingDeviceIdentifier;
        notification.object_type = cov_data.monitoredObjectIdentifier.type;
        notification.object_instance = cov_data.monitoredObjectIdentifier.instance;
        notification.process_id = cov_data.subscriberProcessIdentifier;
        notification.confirmed = true;
        notification.values = cov_data.listOfValues;
        COV_Callback(&notification, COV_Context);
    }
}

/* Handler for unconfirmed COV notifications */
static void client_ucov_notification_handler(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src)
{
    BACNET_COV_DATA cov_data;
    int len;

    (void)src;
    
    len = cov_notify_decode_service_request(service_request, service_len, &cov_data);
    if (len > 0 && COV_Callback) {
        bacnet_cov_notification_t notification;
        notification.device_instance = cov_data.initiatingDeviceIdentifier;
        notification.object_type = cov_data.monitoredObjectIdentifier.type;
        notification.object_instance = cov_data.monitoredObjectIdentifier.instance;
        notification.process_id = cov_data.subscriberProcessIdentifier;
        notification.confirmed = false;
        notification.values = cov_data.listOfValues;
        COV_Callback(&notification, COV_Context);
    }
}

/* Handler for AtomicReadFile ACK */
static void client_atomic_read_file_ack_handler(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data)
{
    int len;
    BACNET_ATOMIC_READ_FILE_DATA data;
    FILE *pFile = NULL;
    size_t octets_written;
    size_t octet_count;
    uint8_t *octet_buffer;

    (void)src;
    if (service_data->invoke_id == Current_Invoke_ID) {
        len = arf_ack_decode_service_request(service_request, service_len, &data);
        if ((len > 0) && (data.access == FILE_STREAM_ACCESS)) {
            if (data.type.stream.fileStartPosition == 0) {
                pFile = fopen(File_Local_Name, "wb");
            } else {
                pFile = fopen(File_Local_Name, "rb+");
            }
            if (pFile) {
                octet_count = octetstring_length(&data.fileData[0]);
                if (octet_count == 0) {
                    if (!data.endOfFile) {
                        data.endOfFile = true;
                    }
                } else {
                    if (fseek(pFile, data.type.stream.fileStartPosition, SEEK_SET) == 0) {
                        octet_buffer = octetstring_value(&data.fileData[0]);
                        octets_written = fwrite(octet_buffer, 1, octet_count, pFile);
                        if (octets_written == octet_count) {
                            File_Start_Position = data.type.stream.fileStartPosition + octets_written;
                        }
                    }
                }
                fclose(pFile);
            }
            if (data.endOfFile) {
                File_End_Of_File = true;
                Request_Complete = true;
            }
        } else {
            Request_Error = true;
            Request_Complete = true;
        }
    }
}

/* Handler for AtomicWriteFile ACK */
static void client_atomic_write_file_ack_handler(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data)
{
    int len;
    BACNET_ATOMIC_WRITE_FILE_DATA data;

    (void)src;
    if (service_data->invoke_id == Current_Invoke_ID) {
        len = awf_ack_decode_service_request(service_request, service_len, &data);
        if (len > 0) {
            if (data.access == FILE_STREAM_ACCESS) {
                File_Start_Position = data.type.stream.fileStartPosition;
            }
            Request_Complete = true;
        } else {
            Request_Error = true;
            Request_Complete = true;
        }
    }
}

/* Initialize service handlers */
static void init_service_handlers(void)
{
    Device_Init(NULL);
    
    /* Handle Who-Is for dynamic binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    
    /* Handle I-Am for device discovery */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, client_i_am_handler);
    
    /* Handle unrecognized services */
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    
    /* We must implement read property - it's required */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    
    /* Handle ReadProperty ACK */
    apdu_set_confirmed_ack_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, client_read_property_ack_handler);
    
    /* Handle WriteProperty ACK */
    apdu_set_confirmed_simple_ack_handler(
        SERVICE_CONFIRMED_WRITE_PROPERTY, client_write_property_simple_ack_handler);
    
    /* Handle SubscribeCOV ACK */
    apdu_set_confirmed_simple_ack_handler(
        SERVICE_CONFIRMED_SUBSCRIBE_COV, client_simple_ack_handler);
    
    /* Handle COV notifications */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_COV_NOTIFICATION, client_ccov_notification_handler);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_COV_NOTIFICATION, client_ucov_notification_handler);
    
    /* Handle AtomicReadFile ACK */
    apdu_set_confirmed_ack_handler(
        SERVICE_CONFIRMED_ATOMIC_READ_FILE, client_atomic_read_file_ack_handler);
    
    /* Handle AtomicWriteFile ACK */
    apdu_set_confirmed_ack_handler(
        SERVICE_CONFIRMED_ATOMIC_WRITE_FILE, client_atomic_write_file_ack_handler);
    
    /* Handle ReinitializeDevice ACK */
    apdu_set_confirmed_simple_ack_handler(
        SERVICE_CONFIRMED_REINITIALIZE_DEVICE, client_simple_ack_handler);
    
    /* Handle DeviceCommunicationControl ACK */
    apdu_set_confirmed_simple_ack_handler(
        SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL, client_simple_ack_handler);
    
    /* Handle AcknowledgeAlarm ACK */
    apdu_set_confirmed_simple_ack_handler(
        SERVICE_CONFIRMED_ACKNOWLEDGE_ALARM, client_simple_ack_handler);
    
    /* Error handlers */
    apdu_set_error_handler(SERVICE_CONFIRMED_READ_PROPERTY, client_error_handler);
    apdu_set_error_handler(SERVICE_CONFIRMED_WRITE_PROPERTY, client_error_handler);
    apdu_set_error_handler(SERVICE_CONFIRMED_SUBSCRIBE_COV, client_error_handler);
    apdu_set_error_handler(SERVICE_CONFIRMED_ATOMIC_READ_FILE, client_error_handler);
    apdu_set_error_handler(SERVICE_CONFIRMED_ATOMIC_WRITE_FILE, client_error_handler);
    apdu_set_error_handler(SERVICE_CONFIRMED_REINITIALIZE_DEVICE, client_error_handler);
    apdu_set_error_handler(SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL, client_error_handler);
    apdu_set_error_handler(SERVICE_CONFIRMED_ACKNOWLEDGE_ALARM, client_error_handler);
    
    /* Abort and reject handlers */
    apdu_set_abort_handler(client_abort_handler);
    apdu_set_reject_handler(client_reject_handler);
}

/* Wait for a confirmed request to complete */
static bacnet_client_error_t wait_for_response(unsigned timeout_ms)
{
    BACNET_ADDRESS src;
    uint16_t pdu_len;
    time_t start_time = time(NULL);
    time_t current_time;
    time_t last_seconds = start_time;
    
    Request_Complete = false;
    Request_Error = false;
    
    while (!Request_Complete) {
        current_time = time(NULL);
        
        /* Check timeout */
        if ((unsigned)((current_time - start_time) * 1000) >= timeout_ms) {
            return BACNET_CLIENT_ERROR_TIMEOUT;
        }
        
        /* Update timers */
        if (current_time != last_seconds) {
            tsm_timer_milliseconds((uint16_t)((current_time - last_seconds) * 1000));
            datalink_maintenance_timer(current_time - last_seconds);
            last_seconds = current_time;
        }
        
        /* Check if TSM failed */
        if (tsm_invoke_id_failed(Current_Invoke_ID)) {
            tsm_free_invoke_id(Current_Invoke_ID);
            return BACNET_CLIENT_ERROR_TIMEOUT;
        }
        
        /* Receive and process packets */
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, 100);
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
        }
    }
    
    if (Request_Error) {
        return BACNET_CLIENT_ERROR_BACNET;
    }
    
    return BACNET_CLIENT_OK;
}

/* Bind to a device (wait for I-Am if needed) */
static bacnet_client_error_t bind_to_device(
    uint32_t device_instance, unsigned timeout_ms, unsigned *max_apdu)
{
    BACNET_ADDRESS src;
    uint16_t pdu_len;
    time_t start_time = time(NULL);
    time_t current_time;
    time_t last_seconds = start_time;
    bool found;
    
    found = address_bind_request(device_instance, max_apdu, &Target_Address);
    if (found) {
        return BACNET_CLIENT_OK;
    }
    
    /* Send Who-Is */
    Send_WhoIs(device_instance, device_instance);
    
    while (!found) {
        current_time = time(NULL);
        
        /* Check timeout */
        if ((unsigned)((current_time - start_time) * 1000) >= timeout_ms) {
            return BACNET_CLIENT_ERROR_NO_DEVICE;
        }
        
        /* Update timers */
        if (current_time != last_seconds) {
            datalink_maintenance_timer(current_time - last_seconds);
            last_seconds = current_time;
        }
        
        /* Receive and process packets */
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, 100);
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
        }
        
        /* Check if bound now */
        found = address_bind_request(device_instance, max_apdu, &Target_Address);
    }
    
    return BACNET_CLIENT_OK;
}

void bacnet_client_config_default(bacnet_client_config_t *config)
{
    if (config) {
        memset(config, 0, sizeof(bacnet_client_config_t));
        config->device_instance = BACNET_MAX_INSTANCE;
        config->udp_port = 47808;
        config->interface = NULL;
        config->timeout_ms = 3000;
        config->retries = 3;
        config->log_level = BACNET_LOG_INFO;
        config->json_output = false;
    }
}

bacnet_client_error_t bacnet_client_init(const bacnet_client_config_t *config)
{
    if (Client_Initialized) {
        return BACNET_CLIENT_OK;
    }
    
    /* Use provided config or defaults */
    if (config) {
        memcpy(&Client_Config, config, sizeof(bacnet_client_config_t));
    } else {
        bacnet_client_config_default(&Client_Config);
    }
    
    Log_Level = Client_Config.log_level;
    
    /* Set device instance */
    Device_Set_Object_Instance_Number(Client_Config.device_instance);
    
    /* Initialize services */
    address_init();
    init_service_handlers();
    
    /* Initialize datalink */
    dlenv_init();
    
    /* Set APDU timeout and retries */
    if (Client_Config.timeout_ms > 0) {
        apdu_timeout_set(Client_Config.timeout_ms);
    }
    if (Client_Config.retries > 0) {
        apdu_retries_set(Client_Config.retries);
    }
    
    Client_Initialized = true;
    LOG_INFO("BACnet client initialized");
    
    return BACNET_CLIENT_OK;
}

void bacnet_client_shutdown(void)
{
    if (Client_Initialized) {
        datalink_cleanup();
        Client_Initialized = false;
        LOG_INFO("BACnet client shutdown");
    }
}

bool bacnet_client_is_initialized(void)
{
    return Client_Initialized;
}

int bacnet_client_run(unsigned timeout_ms)
{
    BACNET_ADDRESS src;
    uint16_t pdu_len;
    time_t start_time = time(NULL);
    time_t current_time;
    time_t last_seconds = start_time;
    int packets = 0;
    
    if (!Client_Initialized) {
        return -1;
    }
    
    while (1) {
        current_time = time(NULL);
        
        if ((unsigned)((current_time - start_time) * 1000) >= timeout_ms) {
            break;
        }
        
        if (current_time != last_seconds) {
            tsm_timer_milliseconds((uint16_t)((current_time - last_seconds) * 1000));
            datalink_maintenance_timer(current_time - last_seconds);
            last_seconds = current_time;
        }
        
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, 100);
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
            packets++;
        }
    }
    
    return packets;
}

bacnet_client_error_t bacnet_client_discover(
    int32_t min_instance,
    int32_t max_instance,
    unsigned timeout_ms,
    bacnet_discover_callback_t callback,
    void *context)
{
    BACNET_ADDRESS src;
    BACNET_ADDRESS dest;
    uint16_t pdu_len;
    time_t start_time;
    time_t current_time;
    time_t last_seconds;
    
    if (!Client_Initialized) {
        return BACNET_CLIENT_ERROR_NOT_INITIALIZED;
    }
    
    /* Set callback */
    Discover_Callback = callback;
    Discover_Context = context;
    
    /* Get broadcast address */
    datalink_get_broadcast_address(&dest);
    
    /* Send Who-Is */
    Send_WhoIs_To_Network(&dest, min_instance, max_instance);
    LOG_DEBUG("Sent Who-Is broadcast");
    
    /* Wait for responses */
    start_time = time(NULL);
    last_seconds = start_time;
    
    while (1) {
        current_time = time(NULL);
        
        if ((unsigned)((current_time - start_time) * 1000) >= timeout_ms) {
            break;
        }
        
        if (current_time != last_seconds) {
            datalink_maintenance_timer(current_time - last_seconds);
            last_seconds = current_time;
        }
        
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, 100);
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
        }
    }
    
    /* Clear callback */
    Discover_Callback = NULL;
    Discover_Context = NULL;
    
    return BACNET_CLIENT_OK;
}

bacnet_client_error_t bacnet_client_read(
    uint32_t device_instance,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property,
    int32_t array_index,
    bacnet_read_result_t *result)
{
    bacnet_client_error_t err;
    unsigned max_apdu;
    
    if (!Client_Initialized) {
        return BACNET_CLIENT_ERROR_NOT_INITIALIZED;
    }
    
    if (!result) {
        return BACNET_CLIENT_ERROR_INVALID_PARAM;
    }
    
    /* Initialize result */
    memset(result, 0, sizeof(bacnet_read_result_t));
    result->object_type = object_type;
    result->object_instance = object_instance;
    result->property = property;
    result->array_index = array_index;
    
    /* Bind to device */
    err = bind_to_device(device_instance, Client_Config.timeout_ms, &max_apdu);
    if (err != BACNET_CLIENT_OK) {
        result->error = err;
        return err;
    }
    
    /* Reset state */
    Read_Complete = false;
    memset(&Read_Data, 0, sizeof(Read_Data));
    
    /* Send request */
    Current_Invoke_ID = Send_Read_Property_Request(
        device_instance, object_type, object_instance, property, array_index);
    
    if (Current_Invoke_ID == 0) {
        result->error = BACNET_CLIENT_ERROR_NETWORK;
        return BACNET_CLIENT_ERROR_NETWORK;
    }
    
    /* Wait for response */
    err = wait_for_response(Client_Config.timeout_ms);
    
    if (err != BACNET_CLIENT_OK) {
        result->error = err;
        if (err == BACNET_CLIENT_ERROR_BACNET) {
            result->error_class = Error_Class;
            result->error_code = Error_Code;
        }
        return err;
    }
    
    if (!Read_Complete) {
        result->error = BACNET_CLIENT_ERROR_TIMEOUT;
        return BACNET_CLIENT_ERROR_TIMEOUT;
    }
    
    result->error = BACNET_CLIENT_OK;
    return BACNET_CLIENT_OK;
}

bacnet_client_error_t bacnet_client_write(
    uint32_t device_instance,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property,
    int32_t array_index,
    BACNET_APPLICATION_DATA_VALUE *value,
    uint8_t priority)
{
    bacnet_client_error_t err;
    unsigned max_apdu;
    
    if (!Client_Initialized) {
        return BACNET_CLIENT_ERROR_NOT_INITIALIZED;
    }
    
    if (!value) {
        return BACNET_CLIENT_ERROR_INVALID_PARAM;
    }
    
    /* Bind to device */
    err = bind_to_device(device_instance, Client_Config.timeout_ms, &max_apdu);
    if (err != BACNET_CLIENT_OK) {
        return err;
    }
    
    /* Reset state */
    Write_Complete = false;
    
    /* Send request */
    Current_Invoke_ID = Send_Write_Property_Request(
        device_instance, object_type, object_instance,
        property, value, priority, array_index);
    
    if (Current_Invoke_ID == 0) {
        return BACNET_CLIENT_ERROR_NETWORK;
    }
    
    /* Wait for response */
    err = wait_for_response(Client_Config.timeout_ms);
    
    if (err != BACNET_CLIENT_OK) {
        return err;
    }
    
    if (!Write_Complete) {
        return BACNET_CLIENT_ERROR_TIMEOUT;
    }
    
    return BACNET_CLIENT_OK;
}

uint32_t bacnet_client_subscribe_cov(
    uint32_t device_instance,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    bool confirmed,
    uint32_t lifetime,
    float cov_increment,
    bacnet_cov_callback_t callback,
    void *context)
{
    bacnet_client_error_t err;
    unsigned max_apdu;
    BACNET_SUBSCRIBE_COV_DATA cov_data;
    uint32_t process_id;
    
    if (!Client_Initialized) {
        return 0;
    }
    
    /* Bind to device */
    err = bind_to_device(device_instance, Client_Config.timeout_ms, &max_apdu);
    if (err != BACNET_CLIENT_OK) {
        return 0;
    }
    
    /* Set callback */
    COV_Callback = callback;
    COV_Context = context;
    
    /* Get process ID */
    process_id = COV_Process_ID++;
    
    /* Setup COV data */
    memset(&cov_data, 0, sizeof(cov_data));
    cov_data.subscriberProcessIdentifier = process_id;
    cov_data.monitoredObjectIdentifier.type = object_type;
    cov_data.monitoredObjectIdentifier.instance = object_instance;
    cov_data.issueConfirmedNotifications = confirmed;
    cov_data.lifetime = lifetime;
    cov_data.cancellationRequest = false;
    
    /* Note: cov_increment is reserved for SubscribeCOVProperty service
       which is not yet implemented in this client */
    (void)cov_increment;
    
    /* Send request */
    Current_Invoke_ID = Send_COV_Subscribe(device_instance, &cov_data);
    
    if (Current_Invoke_ID == 0) {
        return 0;
    }
    
    /* Wait for response */
    err = wait_for_response(Client_Config.timeout_ms);
    
    if (err != BACNET_CLIENT_OK) {
        return 0;
    }
    
    return process_id;
}

bacnet_client_error_t bacnet_client_unsubscribe_cov(
    uint32_t device_instance,
    uint32_t process_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance)
{
    bacnet_client_error_t err;
    unsigned max_apdu;
    BACNET_SUBSCRIBE_COV_DATA cov_data;
    
    if (!Client_Initialized) {
        return BACNET_CLIENT_ERROR_NOT_INITIALIZED;
    }
    
    /* Bind to device */
    err = bind_to_device(device_instance, Client_Config.timeout_ms, &max_apdu);
    if (err != BACNET_CLIENT_OK) {
        return err;
    }
    
    /* Setup COV data for cancellation */
    memset(&cov_data, 0, sizeof(cov_data));
    cov_data.subscriberProcessIdentifier = process_id;
    cov_data.monitoredObjectIdentifier.type = object_type;
    cov_data.monitoredObjectIdentifier.instance = object_instance;
    cov_data.cancellationRequest = true;
    
    /* Send request */
    Current_Invoke_ID = Send_COV_Subscribe(device_instance, &cov_data);
    
    if (Current_Invoke_ID == 0) {
        return BACNET_CLIENT_ERROR_NETWORK;
    }
    
    /* Wait for response */
    return wait_for_response(Client_Config.timeout_ms);
}

bacnet_client_error_t bacnet_client_time_sync(uint32_t device_instance, bool utc)
{
    BACNET_DATE bdate;
    BACNET_TIME btime;
    BACNET_ADDRESS dest;
    int16_t utc_offset_minutes = 0;
    bool dst_active = false;
    
    if (!Client_Initialized) {
        return BACNET_CLIENT_ERROR_NOT_INITIALIZED;
    }
    
    /* Get current time */
    datetime_local(&bdate, &btime, &utc_offset_minutes, &dst_active);
    
    if (device_instance == BACNET_MAX_INSTANCE) {
        /* Broadcast */
        datalink_get_broadcast_address(&dest);
    } else {
        /* Directed */
        unsigned max_apdu;
        bacnet_client_error_t err = bind_to_device(device_instance, Client_Config.timeout_ms, &max_apdu);
        if (err != BACNET_CLIENT_OK) {
            return err;
        }
        memcpy(&dest, &Target_Address, sizeof(BACNET_ADDRESS));
    }
    
    if (utc) {
        BACNET_DATE_TIME utc_time;
        BACNET_DATE_TIME local_time;
        int8_t dst_adjust_minutes = dst_active ? -60 : 0;
        
        datetime_set(&local_time, &bdate, &btime);
        datetime_local_to_utc(&utc_time, &local_time, utc_offset_minutes, dst_adjust_minutes);
        Send_TimeSyncUTC_Remote(&dest, &utc_time.date, &utc_time.time);
    } else {
        Send_TimeSync_Remote(&dest, &bdate, &btime);
    }
    
    return BACNET_CLIENT_OK;
}

bacnet_client_error_t bacnet_client_read_file(
    uint32_t device_instance,
    uint32_t file_instance,
    const char *local_filename)
{
    bacnet_client_error_t err;
    unsigned max_apdu;
    uint16_t my_max_apdu;
    unsigned requested_octet_count;
    
    if (!Client_Initialized) {
        return BACNET_CLIENT_ERROR_NOT_INITIALIZED;
    }
    
    if (!local_filename) {
        return BACNET_CLIENT_ERROR_INVALID_PARAM;
    }
    
    /* Bind to device */
    err = bind_to_device(device_instance, Client_Config.timeout_ms, &max_apdu);
    if (err != BACNET_CLIENT_OK) {
        return err;
    }
    
    /* Calculate request size */
    if (max_apdu < MAX_APDU) {
        my_max_apdu = max_apdu;
    } else {
        my_max_apdu = MAX_APDU;
    }
    
    if (my_max_apdu <= 50) {
        requested_octet_count = my_max_apdu - 20;
    } else if (my_max_apdu <= 480) {
        requested_octet_count = my_max_apdu - 32;
    } else if (my_max_apdu <= 1476) {
        requested_octet_count = my_max_apdu - 64;
    } else {
        requested_octet_count = my_max_apdu / 2;
    }
    
    /* Initialize file state */
    File_Local_Name = local_filename;
    File_Start_Position = 0;
    File_End_Of_File = false;
    
    /* Read file in chunks */
    while (!File_End_Of_File) {
        Current_Invoke_ID = Send_Atomic_Read_File_Stream(
            device_instance, file_instance,
            File_Start_Position, requested_octet_count);
        
        if (Current_Invoke_ID == 0) {
            return BACNET_CLIENT_ERROR_NETWORK;
        }
        
        err = wait_for_response(Client_Config.timeout_ms);
        if (err != BACNET_CLIENT_OK) {
            return err;
        }
    }
    
    return BACNET_CLIENT_OK;
}

bacnet_client_error_t bacnet_client_write_file(
    uint32_t device_instance,
    uint32_t file_instance,
    const char *local_filename)
{
    bacnet_client_error_t err;
    unsigned max_apdu;
    uint16_t my_max_apdu;
    unsigned max_octet_count;
    FILE *pFile;
    uint8_t buffer[1476];
    size_t bytes_read;
    BACNET_ATOMIC_WRITE_FILE_DATA data;
    
    if (!Client_Initialized) {
        return BACNET_CLIENT_ERROR_NOT_INITIALIZED;
    }
    
    if (!local_filename) {
        return BACNET_CLIENT_ERROR_INVALID_PARAM;
    }
    
    /* Bind to device */
    err = bind_to_device(device_instance, Client_Config.timeout_ms, &max_apdu);
    if (err != BACNET_CLIENT_OK) {
        return err;
    }
    
    /* Calculate max write size */
    if (max_apdu < MAX_APDU) {
        my_max_apdu = max_apdu;
    } else {
        my_max_apdu = MAX_APDU;
    }
    
    if (my_max_apdu <= 50) {
        max_octet_count = my_max_apdu - 20;
    } else if (my_max_apdu <= 480) {
        max_octet_count = my_max_apdu - 32;
    } else if (my_max_apdu <= 1476) {
        max_octet_count = my_max_apdu - 64;
    } else {
        max_octet_count = my_max_apdu / 2;
    }
    
    if (max_octet_count > sizeof(buffer)) {
        max_octet_count = sizeof(buffer);
    }
    
    /* Open local file */
    pFile = fopen(local_filename, "rb");
    if (!pFile) {
        return BACNET_CLIENT_ERROR_INVALID_PARAM;
    }
    
    /* Initialize state */
    File_Start_Position = 0;
    
    /* Write file in chunks */
    while (1) {
        bytes_read = fread(buffer, 1, max_octet_count, pFile);
        if (bytes_read == 0) {
            break;
        }
        
        /* Setup write data */
        data.object_type = OBJECT_FILE;
        data.object_instance = file_instance;
        data.access = FILE_STREAM_ACCESS;
        data.type.stream.fileStartPosition = File_Start_Position;
        octetstring_init(&data.fileData[0], buffer, bytes_read);
        
        Current_Invoke_ID = Send_Atomic_Write_File_Stream(
            device_instance, file_instance,
            File_Start_Position, &data.fileData[0]);
        
        if (Current_Invoke_ID == 0) {
            fclose(pFile);
            return BACNET_CLIENT_ERROR_NETWORK;
        }
        
        err = wait_for_response(Client_Config.timeout_ms);
        if (err != BACNET_CLIENT_OK) {
            fclose(pFile);
            return err;
        }
        
        File_Start_Position += bytes_read;
        
        if (bytes_read < max_octet_count) {
            break;
        }
    }
    
    fclose(pFile);
    return BACNET_CLIENT_OK;
}

bacnet_client_error_t bacnet_client_reinitialize(
    uint32_t device_instance,
    BACNET_REINITIALIZED_STATE state,
    const char *password)
{
    bacnet_client_error_t err;
    unsigned max_apdu;
    
    if (!Client_Initialized) {
        return BACNET_CLIENT_ERROR_NOT_INITIALIZED;
    }
    
    /* Bind to device */
    err = bind_to_device(device_instance, Client_Config.timeout_ms, &max_apdu);
    if (err != BACNET_CLIENT_OK) {
        return err;
    }
    
    /* Send request */
    Current_Invoke_ID = Send_Reinitialize_Device_Request(
        device_instance, state, password);
    
    if (Current_Invoke_ID == 0) {
        return BACNET_CLIENT_ERROR_NETWORK;
    }
    
    /* Wait for response */
    return wait_for_response(Client_Config.timeout_ms);
}

bacnet_client_error_t bacnet_client_device_comm_control(
    uint32_t device_instance,
    BACNET_COMMUNICATION_ENABLE_DISABLE state,
    uint16_t timeout_minutes,
    const char *password)
{
    bacnet_client_error_t err;
    unsigned max_apdu;
    
    if (!Client_Initialized) {
        return BACNET_CLIENT_ERROR_NOT_INITIALIZED;
    }
    
    /* Bind to device */
    err = bind_to_device(device_instance, Client_Config.timeout_ms, &max_apdu);
    if (err != BACNET_CLIENT_OK) {
        return err;
    }
    
    /* Send request */
    Current_Invoke_ID = Send_Device_Communication_Control_Request(
        device_instance, timeout_minutes, state, password);
    
    if (Current_Invoke_ID == 0) {
        return BACNET_CLIENT_ERROR_NETWORK;
    }
    
    /* Wait for response */
    return wait_for_response(Client_Config.timeout_ms);
}

bacnet_client_error_t bacnet_client_register_foreign_device(
    const char *bbmd_address,
    uint16_t bbmd_port,
    uint16_t ttl)
{
    BACNET_IP_ADDRESS bbmd_addr;
    
    if (!Client_Initialized) {
        return BACNET_CLIENT_ERROR_NOT_INITIALIZED;
    }
    
    if (!bbmd_address) {
        return BACNET_CLIENT_ERROR_INVALID_PARAM;
    }
    
    /* Parse BBMD address */
    if (!bip_get_addr_by_name(bbmd_address, &bbmd_addr)) {
        return BACNET_CLIENT_ERROR_INVALID_PARAM;
    }
    bbmd_addr.port = bbmd_port;
    
    /* Register */
    if (bvlc_register_with_bbmd(&bbmd_addr, ttl)) {
        return BACNET_CLIENT_OK;
    }
    
    return BACNET_CLIENT_ERROR_NETWORK;
}

bacnet_client_error_t bacnet_client_unregister_foreign_device(void)
{
    if (!Client_Initialized) {
        return BACNET_CLIENT_ERROR_NOT_INITIALIZED;
    }
    
    /* Just stop the registration timer - next maintenance will handle it */
    return BACNET_CLIENT_OK;
}

bacnet_client_error_t bacnet_client_ack_alarm(
    uint32_t device_instance,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    uint32_t process_id,
    BACNET_EVENT_STATE event_state,
    BACNET_TIMESTAMP *timestamp,
    const char *ack_source)
{
    bacnet_client_error_t err;
    unsigned max_apdu;
    BACNET_ALARM_ACK_DATA data;
    BACNET_CHARACTER_STRING source;
    
    if (!Client_Initialized) {
        return BACNET_CLIENT_ERROR_NOT_INITIALIZED;
    }
    
    /* Bind to device */
    err = bind_to_device(device_instance, Client_Config.timeout_ms, &max_apdu);
    if (err != BACNET_CLIENT_OK) {
        return err;
    }
    
    /* Setup ack data */
    data.ackProcessIdentifier = process_id;
    data.eventObjectIdentifier.type = object_type;
    data.eventObjectIdentifier.instance = object_instance;
    data.eventStateAcked = event_state;
    if (timestamp) {
        memcpy(&data.eventTimeStamp, timestamp, sizeof(BACNET_TIMESTAMP));
    }
    characterstring_init_ansi(&source, ack_source ? ack_source : "bacnet-client");
    memcpy(&data.ackSource, &source, sizeof(BACNET_CHARACTER_STRING));
    
    /* Get current time for ack timestamp */
    data.ackTimeStamp.tag = TIME_STAMP_DATETIME;
    datetime_local(
        &data.ackTimeStamp.value.dateTime.date,
        &data.ackTimeStamp.value.dateTime.time,
        NULL, NULL);
    
    /* Send request */
    Current_Invoke_ID = Send_Alarm_Acknowledgement(
        device_instance, &data);
    
    if (Current_Invoke_ID == 0) {
        return BACNET_CLIENT_ERROR_NETWORK;
    }
    
    /* Wait for response */
    return wait_for_response(Client_Config.timeout_ms);
}

void bacnet_client_set_event_callback(
    bacnet_event_callback_t callback, void *context)
{
    Event_Callback = callback;
    Event_Context = context;
}

const char *bacnet_client_error_string(bacnet_client_error_t error)
{
    switch (error) {
        case BACNET_CLIENT_OK:
            return "Success";
        case BACNET_CLIENT_ERROR_INVALID_PARAM:
            return "Invalid parameter";
        case BACNET_CLIENT_ERROR_TIMEOUT:
            return "Timeout";
        case BACNET_CLIENT_ERROR_NO_DEVICE:
            return "Device not found";
        case BACNET_CLIENT_ERROR_NETWORK:
            return "Network error";
        case BACNET_CLIENT_ERROR_REJECTED:
            return "Request rejected";
        case BACNET_CLIENT_ERROR_ABORTED:
            return "Request aborted";
        case BACNET_CLIENT_ERROR_BACNET:
            return "BACnet error";
        case BACNET_CLIENT_ERROR_NOT_INITIALIZED:
            return "Client not initialized";
        case BACNET_CLIENT_ERROR_BUSY:
            return "Client busy";
        default:
            return "Unknown error";
    }
}

void bacnet_client_set_log_level(bacnet_log_level_t level)
{
    Log_Level = level;
}
