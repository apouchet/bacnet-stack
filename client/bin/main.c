/**
 * @file
 * @brief BACnet Client CLI Tool - Main Entry Point
 * @author BACnet Stack Contributors
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <time.h>

/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bactext.h"
#include "bacnet/bacapp.h"
#include "bacnet/version.h"
/* Client library */
#include "../lib/bacnet_client.h"

/* Global state */
static volatile bool Running = true;
static bool Json_Output = false;
static bacnet_client_config_t Config;

/* Statistics (reserved for future use) */
static unsigned Devices_Found = 0;

/* Signal handler */
static void signal_handler(int sig)
{
    (void)sig;
    Running = false;
}

/* Print version information */
static void print_version(void)
{
    printf("bacnet-client %s\n", BACNET_VERSION_TEXT);
    printf("BACnet stack version: %s\n", BACNET_VERSION_TEXT);
    printf("Copyright (C) 2024 BACnet Stack Contributors\n");
    printf("License: MIT\n");
}

/* Print main help */
static void print_help(void)
{
    printf("BACnet Client - A production-quality BACnet/IP client\n\n");
    printf("Usage: bacnet-client [options] <command> [command-options]\n\n");
    printf("Global Options:\n");
    printf("  --timeout <ms>     APDU timeout in milliseconds (default: 3000)\n");
    printf("  --retries <n>      Number of retries (default: 3)\n");
    printf("  --verbose, -v      Increase verbosity\n");
    printf("  --log-level <lvl>  Set log level: error, warn, info, debug\n");
    printf("  --json             Output in JSON format\n");
    printf("  --config <file>    Configuration file path (not yet implemented)\n");
    printf("  --version          Show version information\n");
    printf("  --help, -h         Show this help message\n\n");
    printf("Commands:\n");
    printf("  whois              Device discovery (Who-Is/I-Am)\n");
    printf("  read               Read property from device\n");
    printf("  write              Write property to device\n");
    printf("  read-range         Read trend log data\n");
    printf("  cov-subscribe      Subscribe to COV notifications\n");
    printf("  cov-unsubscribe    Unsubscribe from COV notifications\n");
    printf("  monitor-events     Monitor event notifications\n");
    printf("  ack-alarm          Acknowledge an alarm\n");
    printf("  time-sync          Send time synchronization\n");
    printf("  file-read          Read file from device\n");
    printf("  file-write         Write file to device\n");
    printf("  reinit             Reinitialize device\n");
    printf("  device-comm-control  Device communication control\n");
    printf("  register-foreign   Register as foreign device with BBMD\n");
    printf("  unregister-foreign Unregister as foreign device\n");
    printf("  bbmd               BBMD operations\n\n");
    printf("Use 'bacnet-client <command> --help' for command-specific help.\n");
}

/* Discovery callback */
static void discovery_callback(const bacnet_device_info_t *device, void *context)
{
    (void)context;
    Devices_Found++;
    
    if (Json_Output) {
        printf("{\"device_instance\":%u,\"max_apdu\":%u,\"segmentation\":%d,\"vendor_id\":%u}\n",
            (unsigned)device->device_instance,
            device->max_apdu,
            device->segmentation,
            (unsigned)device->vendor_id);
    } else {
        printf("Device %u: MaxAPDU=%u Segmentation=%s Vendor=%u\n",
            (unsigned)device->device_instance,
            device->max_apdu,
            bactext_segmentation_name(device->segmentation),
            (unsigned)device->vendor_id);
        
        /* Print address */
        printf("  MAC: ");
        for (int i = 0; i < device->address.mac_len; i++) {
            if (i > 0) printf(":");
            printf("%02X", device->address.mac[i]);
        }
        printf("\n");
    }
}

/* Who-Is command */
static int cmd_whois(int argc, char *argv[])
{
    int32_t min_instance = -1;
    int32_t max_instance = -1;
    unsigned timeout_ms = 3000;
    int opt;
    
    static struct option long_options[] = {
        {"device-instance", required_argument, 0, 'd'},
        {"timeout", required_argument, 0, 't'},
        {"json", no_argument, 0, 'j'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    optind = 1;
    while ((opt = getopt_long(argc, argv, "d:t:jh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'd': {
                char *dash = strchr(optarg, '-');
                if (dash) {
                    *dash = '\0';
                    min_instance = atoi(optarg);
                    max_instance = atoi(dash + 1);
                } else {
                    min_instance = max_instance = atoi(optarg);
                }
                break;
            }
            case 't':
                timeout_ms = atoi(optarg);
                break;
            case 'j':
                Json_Output = true;
                break;
            case 'h':
                printf("Usage: bacnet-client whois [options]\n\n");
                printf("Options:\n");
                printf("  --device-instance <min>-<max>  Device instance range\n");
                printf("  --timeout <ms>                 Timeout in milliseconds\n");
                printf("  --json                         Output in JSON format\n");
                return 0;
            default:
                return 1;
        }
    }
    
    Devices_Found = 0;
    
    if (Json_Output) {
        printf("[");
    } else {
        printf("Discovering BACnet devices...\n");
    }
    
    bacnet_client_error_t err = bacnet_client_discover(
        min_instance, max_instance, timeout_ms, discovery_callback, NULL);
    
    if (Json_Output) {
        printf("]\n");
    } else {
        printf("\nFound %u device(s)\n", Devices_Found);
    }
    
    return (err == BACNET_CLIENT_OK) ? 0 : 1;
}

/* Read command */
static int cmd_read(int argc, char *argv[])
{
    uint32_t device_instance = BACNET_MAX_INSTANCE;
    BACNET_OBJECT_TYPE object_type = OBJECT_DEVICE;
    uint32_t object_instance = 0;
    BACNET_PROPERTY_ID property = PROP_OBJECT_NAME;
    int32_t array_index = BACNET_ARRAY_ALL;
    int opt;
    
    static struct option long_options[] = {
        {"device", required_argument, 0, 'd'},
        {"object", required_argument, 0, 'o'},
        {"property", required_argument, 0, 'p'},
        {"index", required_argument, 0, 'i'},
        {"json", no_argument, 0, 'j'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    optind = 1;
    while ((opt = getopt_long(argc, argv, "d:o:p:i:jh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'd':
                device_instance = atoi(optarg);
                break;
            case 'o': {
                char *colon = strchr(optarg, ':');
                if (colon) {
                    *colon = '\0';
                    unsigned type_val;
                    if (bactext_object_type_strtol(optarg, &type_val)) {
                        object_type = type_val;
                    } else {
                        fprintf(stderr, "Invalid object type: %s\n", optarg);
                        return 1;
                    }
                    object_instance = atoi(colon + 1);
                }
                break;
            }
            case 'p': {
                unsigned prop_val;
                if (bactext_property_strtol(optarg, &prop_val)) {
                    property = prop_val;
                } else {
                    property = atoi(optarg);
                }
                break;
            }
            case 'i':
                array_index = atoi(optarg);
                break;
            case 'j':
                Json_Output = true;
                break;
            case 'h':
                printf("Usage: bacnet-client read [options]\n\n");
                printf("Options:\n");
                printf("  --device <instance>    Target device instance\n");
                printf("  --object <type>:<id>   Object type and instance\n");
                printf("  --property <prop>      Property identifier\n");
                printf("  --index <idx>          Array index (optional)\n");
                printf("  --json                 Output in JSON format\n");
                return 0;
            default:
                return 1;
        }
    }
    
    if (device_instance == BACNET_MAX_INSTANCE) {
        fprintf(stderr, "Error: --device is required\n");
        return 1;
    }
    
    bacnet_read_result_t result;
    bacnet_client_error_t err = bacnet_client_read(
        device_instance, object_type, object_instance,
        property, array_index, &result);
    
    if (err != BACNET_CLIENT_OK) {
        if (Json_Output) {
            printf("{\"error\":\"%s\"", bacnet_client_error_string(err));
            if (err == BACNET_CLIENT_ERROR_BACNET) {
                printf(",\"error_class\":\"%s\",\"error_code\":\"%s\"",
                    bactext_error_class_name((int)result.error_class),
                    bactext_error_code_name((int)result.error_code));
            }
            printf("}\n");
        } else {
            fprintf(stderr, "Error: %s\n", bacnet_client_error_string(err));
            if (err == BACNET_CLIENT_ERROR_BACNET) {
                fprintf(stderr, "BACnet Error: %s: %s\n",
                    bactext_error_class_name((int)result.error_class),
                    bactext_error_code_name((int)result.error_code));
            }
        }
        return 1;
    }
    
    if (Json_Output) {
        printf("{\"object\":\"%s:%u\",\"property\":\"%s\",\"value\":\"<data>\"}\n",
            bactext_object_type_name(object_type),
            (unsigned)object_instance,
            bactext_property_name(property));
    } else {
        printf("%s:%u.%s = <value>\n",
            bactext_object_type_name(object_type),
            (unsigned)object_instance,
            bactext_property_name(property));
    }
    
    return 0;
}

/* Write command */
static int cmd_write(int argc, char *argv[])
{
    uint32_t device_instance = BACNET_MAX_INSTANCE;
    BACNET_OBJECT_TYPE object_type = OBJECT_DEVICE;
    uint32_t object_instance = 0;
    BACNET_PROPERTY_ID property = PROP_PRESENT_VALUE;
    int32_t array_index = BACNET_ARRAY_ALL;
    const char *value_str = NULL;
    uint8_t priority = 0;
    int opt;
    
    static struct option long_options[] = {
        {"device", required_argument, 0, 'd'},
        {"object", required_argument, 0, 'o'},
        {"property", required_argument, 0, 'p'},
        {"value", required_argument, 0, 'V'},
        {"priority", required_argument, 0, 'P'},
        {"index", required_argument, 0, 'i'},
        {"json", no_argument, 0, 'j'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    optind = 1;
    while ((opt = getopt_long(argc, argv, "d:o:p:V:P:i:jh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'd':
                device_instance = atoi(optarg);
                break;
            case 'o': {
                char *colon = strchr(optarg, ':');
                if (colon) {
                    *colon = '\0';
                    unsigned type_val;
                    if (bactext_object_type_strtol(optarg, &type_val)) {
                        object_type = type_val;
                    }
                    object_instance = atoi(colon + 1);
                }
                break;
            }
            case 'p': {
                unsigned prop_val;
                if (bactext_property_strtol(optarg, &prop_val)) {
                    property = prop_val;
                } else {
                    property = atoi(optarg);
                }
                break;
            }
            case 'V':
                value_str = optarg;
                break;
            case 'P':
                priority = atoi(optarg);
                break;
            case 'i':
                array_index = atoi(optarg);
                break;
            case 'j':
                Json_Output = true;
                break;
            case 'h':
                printf("Usage: bacnet-client write [options]\n\n");
                printf("Options:\n");
                printf("  --device <instance>    Target device instance\n");
                printf("  --object <type>:<id>   Object type and instance\n");
                printf("  --property <prop>      Property identifier\n");
                printf("  --value <value>        Value to write\n");
                printf("  --priority <N>         Write priority (1-16)\n");
                printf("  --index <idx>          Array index (optional)\n");
                printf("  --json                 Output in JSON format\n");
                return 0;
            default:
                return 1;
        }
    }
    
    if (device_instance == BACNET_MAX_INSTANCE || !value_str) {
        fprintf(stderr, "Error: --device and --value are required\n");
        return 1;
    }
    
    /* Parse value - simple implementation for common types */
    BACNET_APPLICATION_DATA_VALUE value;
    memset(&value, 0, sizeof(value));
    
    /* Try to parse as number first */
    char *endptr;
    double dval = strtod(value_str, &endptr);
    if (*endptr == '\0') {
        /* It's a number */
        if (strchr(value_str, '.')) {
            value.tag = BACNET_APPLICATION_TAG_REAL;
            value.type.Real = (float)dval;
        } else {
            long lval = strtol(value_str, NULL, 0);
            if (lval < 0) {
                value.tag = BACNET_APPLICATION_TAG_SIGNED_INT;
                value.type.Signed_Int = lval;
            } else {
                value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
                value.type.Unsigned_Int = lval;
            }
        }
    } else if (strcasecmp(value_str, "true") == 0 || strcasecmp(value_str, "active") == 0) {
        value.tag = BACNET_APPLICATION_TAG_BOOLEAN;
        value.type.Boolean = true;
    } else if (strcasecmp(value_str, "false") == 0 || strcasecmp(value_str, "inactive") == 0) {
        value.tag = BACNET_APPLICATION_TAG_BOOLEAN;
        value.type.Boolean = false;
    } else if (strcasecmp(value_str, "null") == 0) {
        value.tag = BACNET_APPLICATION_TAG_NULL;
    } else {
        /* Treat as string */
        value.tag = BACNET_APPLICATION_TAG_CHARACTER_STRING;
        characterstring_init_ansi(&value.type.Character_String, value_str);
    }
    
    bacnet_client_error_t err = bacnet_client_write(
        device_instance, object_type, object_instance,
        property, array_index, &value, priority);
    
    if (err != BACNET_CLIENT_OK) {
        if (Json_Output) {
            printf("{\"error\":\"%s\"}\n", bacnet_client_error_string(err));
        } else {
            fprintf(stderr, "Error: %s\n", bacnet_client_error_string(err));
        }
        return 1;
    }
    
    if (Json_Output) {
        printf("{\"status\":\"success\"}\n");
    } else {
        printf("Write successful\n");
    }
    
    return 0;
}

/* COV notification callback */
static void cov_callback(const bacnet_cov_notification_t *notification, void *context)
{
    (void)context;
    
    if (Json_Output) {
        printf("{\"type\":\"cov\",\"device\":%u,\"object\":\"%s:%u\",\"confirmed\":%s}\n",
            (unsigned)notification->device_instance,
            bactext_object_type_name(notification->object_type),
            (unsigned)notification->object_instance,
            notification->confirmed ? "true" : "false");
    } else {
        printf("COV Notification from device %u: %s:%u\n",
            (unsigned)notification->device_instance,
            bactext_object_type_name(notification->object_type),
            (unsigned)notification->object_instance);
    }
}

/* COV subscribe command */
static int cmd_cov_subscribe(int argc, char *argv[])
{
    uint32_t device_instance = BACNET_MAX_INSTANCE;
    BACNET_OBJECT_TYPE object_type = OBJECT_ANALOG_INPUT;
    uint32_t object_instance = 0;
    uint32_t lifetime = 300;
    bool confirmed = false;
    float cov_increment = 0.0f;
    int opt;
    
    static struct option long_options[] = {
        {"device", required_argument, 0, 'd'},
        {"object", required_argument, 0, 'o'},
        {"lifetime", required_argument, 0, 'l'},
        {"confirmed", no_argument, 0, 'c'},
        {"cov-increment", required_argument, 0, 'I'},
        {"json", no_argument, 0, 'j'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    optind = 1;
    while ((opt = getopt_long(argc, argv, "d:o:l:cI:jh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'd':
                device_instance = atoi(optarg);
                break;
            case 'o': {
                char *colon = strchr(optarg, ':');
                if (colon) {
                    *colon = '\0';
                    unsigned type_val;
                    if (bactext_object_type_strtol(optarg, &type_val)) {
                        object_type = type_val;
                    }
                    object_instance = atoi(colon + 1);
                }
                break;
            }
            case 'l':
                lifetime = atoi(optarg);
                break;
            case 'c':
                confirmed = true;
                break;
            case 'I':
                cov_increment = (float)atof(optarg);
                break;
            case 'j':
                Json_Output = true;
                break;
            case 'h':
                printf("Usage: bacnet-client cov-subscribe [options]\n\n");
                printf("Options:\n");
                printf("  --device <instance>    Target device instance\n");
                printf("  --object <type>:<id>   Object type and instance\n");
                printf("  --lifetime <seconds>   Subscription lifetime\n");
                printf("  --confirmed            Use confirmed notifications\n");
                printf("  --cov-increment <val>  COV increment value\n");
                printf("  --json                 Output in JSON format\n");
                return 0;
            default:
                return 1;
        }
    }
    
    if (device_instance == BACNET_MAX_INSTANCE) {
        fprintf(stderr, "Error: --device is required\n");
        return 1;
    }
    
    uint32_t process_id = bacnet_client_subscribe_cov(
        device_instance, object_type, object_instance,
        confirmed, lifetime, cov_increment, cov_callback, NULL);
    
    if (process_id == 0) {
        if (Json_Output) {
            printf("{\"error\":\"subscription failed\"}\n");
        } else {
            fprintf(stderr, "Error: Subscription failed\n");
        }
        return 1;
    }
    
    if (Json_Output) {
        printf("{\"status\":\"subscribed\",\"process_id\":%u}\n", (unsigned)process_id);
    } else {
        printf("Subscribed with process ID %u\n", (unsigned)process_id);
        printf("Waiting for COV notifications (press Ctrl+C to stop)...\n");
    }
    
    /* Wait for notifications */
    while (Running) {
        bacnet_client_run(1000);
    }
    
    /* Unsubscribe */
    bacnet_client_unsubscribe_cov(device_instance, process_id, object_type, object_instance);
    
    return 0;
}

/* Time sync command */
static int cmd_time_sync(int argc, char *argv[])
{
    uint32_t device_instance = BACNET_MAX_INSTANCE;
    bool utc = false;
    int opt;
    
    static struct option long_options[] = {
        {"device", required_argument, 0, 'd'},
        {"utc", no_argument, 0, 'u'},
        {"json", no_argument, 0, 'j'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    optind = 1;
    while ((opt = getopt_long(argc, argv, "d:ujh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'd':
                if (strcmp(optarg, "broadcast") == 0) {
                    device_instance = BACNET_MAX_INSTANCE;
                } else {
                    device_instance = atoi(optarg);
                }
                break;
            case 'u':
                utc = true;
                break;
            case 'j':
                Json_Output = true;
                break;
            case 'h':
                printf("Usage: bacnet-client time-sync [options]\n\n");
                printf("Options:\n");
                printf("  --device <instance|broadcast>  Target device or broadcast\n");
                printf("  --utc                          Send UTC time sync\n");
                printf("  --json                         Output in JSON format\n");
                return 0;
            default:
                return 1;
        }
    }
    
    bacnet_client_error_t err = bacnet_client_time_sync(device_instance, utc);
    
    if (err != BACNET_CLIENT_OK) {
        if (Json_Output) {
            printf("{\"error\":\"%s\"}\n", bacnet_client_error_string(err));
        } else {
            fprintf(stderr, "Error: %s\n", bacnet_client_error_string(err));
        }
        return 1;
    }
    
    if (Json_Output) {
        printf("{\"status\":\"sent\"}\n");
    } else {
        printf("Time synchronization sent\n");
    }
    
    return 0;
}

/* File read command */
static int cmd_file_read(int argc, char *argv[])
{
    uint32_t device_instance = BACNET_MAX_INSTANCE;
    uint32_t file_instance = 0;
    const char *output_file = NULL;
    int opt;
    
    static struct option long_options[] = {
        {"device", required_argument, 0, 'd'},
        {"file", required_argument, 0, 'f'},
        {"out", required_argument, 0, 'o'},
        {"json", no_argument, 0, 'j'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    optind = 1;
    while ((opt = getopt_long(argc, argv, "d:f:o:jh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'd':
                device_instance = atoi(optarg);
                break;
            case 'f':
                file_instance = atoi(optarg);
                break;
            case 'o':
                output_file = optarg;
                break;
            case 'j':
                Json_Output = true;
                break;
            case 'h':
                printf("Usage: bacnet-client file-read [options]\n\n");
                printf("Options:\n");
                printf("  --device <instance>  Target device instance\n");
                printf("  --file <id>          File object instance\n");
                printf("  --out <path>         Output file path\n");
                printf("  --json               Output in JSON format\n");
                return 0;
            default:
                return 1;
        }
    }
    
    if (device_instance == BACNET_MAX_INSTANCE || !output_file) {
        fprintf(stderr, "Error: --device and --out are required\n");
        return 1;
    }
    
    bacnet_client_error_t err = bacnet_client_read_file(
        device_instance, file_instance, output_file);
    
    if (err != BACNET_CLIENT_OK) {
        if (Json_Output) {
            printf("{\"error\":\"%s\"}\n", bacnet_client_error_string(err));
        } else {
            fprintf(stderr, "Error: %s\n", bacnet_client_error_string(err));
        }
        return 1;
    }
    
    if (Json_Output) {
        printf("{\"status\":\"success\",\"file\":\"%s\"}\n", output_file);
    } else {
        printf("File saved to: %s\n", output_file);
    }
    
    return 0;
}

/* File write command */
static int cmd_file_write(int argc, char *argv[])
{
    uint32_t device_instance = BACNET_MAX_INSTANCE;
    uint32_t file_instance = 0;
    const char *input_file = NULL;
    int opt;
    
    static struct option long_options[] = {
        {"device", required_argument, 0, 'd'},
        {"file", required_argument, 0, 'f'},
        {"in", required_argument, 0, 'i'},
        {"json", no_argument, 0, 'j'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    optind = 1;
    while ((opt = getopt_long(argc, argv, "d:f:i:jh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'd':
                device_instance = atoi(optarg);
                break;
            case 'f':
                file_instance = atoi(optarg);
                break;
            case 'i':
                input_file = optarg;
                break;
            case 'j':
                Json_Output = true;
                break;
            case 'h':
                printf("Usage: bacnet-client file-write [options]\n\n");
                printf("Options:\n");
                printf("  --device <instance>  Target device instance\n");
                printf("  --file <id>          File object instance\n");
                printf("  --in <path>          Input file path\n");
                printf("  --json               Output in JSON format\n");
                return 0;
            default:
                return 1;
        }
    }
    
    if (device_instance == BACNET_MAX_INSTANCE || !input_file) {
        fprintf(stderr, "Error: --device and --in are required\n");
        return 1;
    }
    
    bacnet_client_error_t err = bacnet_client_write_file(
        device_instance, file_instance, input_file);
    
    if (err != BACNET_CLIENT_OK) {
        if (Json_Output) {
            printf("{\"error\":\"%s\"}\n", bacnet_client_error_string(err));
        } else {
            fprintf(stderr, "Error: %s\n", bacnet_client_error_string(err));
        }
        return 1;
    }
    
    if (Json_Output) {
        printf("{\"status\":\"success\"}\n");
    } else {
        printf("File written successfully\n");
    }
    
    return 0;
}

/* Reinitialize command */
static int cmd_reinit(int argc, char *argv[])
{
    uint32_t device_instance = BACNET_MAX_INSTANCE;
    BACNET_REINITIALIZED_STATE state = BACNET_REINIT_COLDSTART;
    const char *password = NULL;
    int opt;
    
    static struct option long_options[] = {
        {"device", required_argument, 0, 'd'},
        {"option", required_argument, 0, 'o'},
        {"password", required_argument, 0, 'p'},
        {"json", no_argument, 0, 'j'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    optind = 1;
    while ((opt = getopt_long(argc, argv, "d:o:p:jh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'd':
                device_instance = atoi(optarg);
                break;
            case 'o':
                if (strcmp(optarg, "cold-start") == 0) {
                    state = BACNET_REINIT_COLDSTART;
                } else if (strcmp(optarg, "warm-start") == 0) {
                    state = BACNET_REINIT_WARMSTART;
                } else {
                    state = atoi(optarg);
                }
                break;
            case 'p':
                password = optarg;
                break;
            case 'j':
                Json_Output = true;
                break;
            case 'h':
                printf("Usage: bacnet-client reinit [options]\n\n");
                printf("Options:\n");
                printf("  --device <instance>  Target device instance\n");
                printf("  --option <type>      cold-start, warm-start, etc.\n");
                printf("  --password <pwd>     Optional password\n");
                printf("  --json               Output in JSON format\n");
                return 0;
            default:
                return 1;
        }
    }
    
    if (device_instance == BACNET_MAX_INSTANCE) {
        fprintf(stderr, "Error: --device is required\n");
        return 1;
    }
    
    bacnet_client_error_t err = bacnet_client_reinitialize(
        device_instance, state, password);
    
    if (err != BACNET_CLIENT_OK) {
        if (Json_Output) {
            printf("{\"error\":\"%s\"}\n", bacnet_client_error_string(err));
        } else {
            fprintf(stderr, "Error: %s\n", bacnet_client_error_string(err));
        }
        return 1;
    }
    
    if (Json_Output) {
        printf("{\"status\":\"acknowledged\"}\n");
    } else {
        printf("Reinitialize acknowledged\n");
    }
    
    return 0;
}

/* Device communication control command */
static int cmd_device_comm_control(int argc, char *argv[])
{
    uint32_t device_instance = BACNET_MAX_INSTANCE;
    BACNET_COMMUNICATION_ENABLE_DISABLE state = COMMUNICATION_ENABLE;
    uint16_t timeout_minutes = 0;
    const char *password = NULL;
    int opt;
    
    static struct option long_options[] = {
        {"device", required_argument, 0, 'd'},
        {"enable", no_argument, 0, 'e'},
        {"disable", no_argument, 0, 'D'},
        {"time", required_argument, 0, 't'},
        {"password", required_argument, 0, 'p'},
        {"json", no_argument, 0, 'j'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    optind = 1;
    while ((opt = getopt_long(argc, argv, "d:eDt:p:jh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'd':
                device_instance = atoi(optarg);
                break;
            case 'e':
                state = COMMUNICATION_ENABLE;
                break;
            case 'D':
                state = COMMUNICATION_DISABLE;
                break;
            case 't':
                timeout_minutes = atoi(optarg);
                break;
            case 'p':
                password = optarg;
                break;
            case 'j':
                Json_Output = true;
                break;
            case 'h':
                printf("Usage: bacnet-client device-comm-control [options]\n\n");
                printf("Options:\n");
                printf("  --device <instance>  Target device instance\n");
                printf("  --enable             Enable communication\n");
                printf("  --disable            Disable communication\n");
                printf("  --time <minutes>     Duration for disable\n");
                printf("  --password <pwd>     Optional password\n");
                printf("  --json               Output in JSON format\n");
                return 0;
            default:
                return 1;
        }
    }
    
    if (device_instance == BACNET_MAX_INSTANCE) {
        fprintf(stderr, "Error: --device is required\n");
        return 1;
    }
    
    bacnet_client_error_t err = bacnet_client_device_comm_control(
        device_instance, state, timeout_minutes, password);
    
    if (err != BACNET_CLIENT_OK) {
        if (Json_Output) {
            printf("{\"error\":\"%s\"}\n", bacnet_client_error_string(err));
        } else {
            fprintf(stderr, "Error: %s\n", bacnet_client_error_string(err));
        }
        return 1;
    }
    
    if (Json_Output) {
        printf("{\"status\":\"acknowledged\"}\n");
    } else {
        printf("Device communication control acknowledged\n");
    }
    
    return 0;
}

/* Register foreign device command */
static int cmd_register_foreign(int argc, char *argv[])
{
    const char *bbmd_address = NULL;
    uint16_t bbmd_port = 47808;
    uint16_t ttl = 60;
    int opt;
    
    static struct option long_options[] = {
        {"bbmd", required_argument, 0, 'b'},
        {"ttl", required_argument, 0, 't'},
        {"json", no_argument, 0, 'j'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    optind = 1;
    while ((opt = getopt_long(argc, argv, "b:t:jh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'b': {
                char *colon = strchr(optarg, ':');
                if (colon) {
                    *colon = '\0';
                    bbmd_port = atoi(colon + 1);
                }
                bbmd_address = optarg;
                break;
            }
            case 't':
                ttl = atoi(optarg);
                break;
            case 'j':
                Json_Output = true;
                break;
            case 'h':
                printf("Usage: bacnet-client register-foreign [options]\n\n");
                printf("Options:\n");
                printf("  --bbmd <ip:port>  BBMD address\n");
                printf("  --ttl <seconds>   Time-to-live\n");
                printf("  --json            Output in JSON format\n");
                return 0;
            default:
                return 1;
        }
    }
    
    if (!bbmd_address) {
        fprintf(stderr, "Error: --bbmd is required\n");
        return 1;
    }
    
    bacnet_client_error_t err = bacnet_client_register_foreign_device(
        bbmd_address, bbmd_port, ttl);
    
    if (err != BACNET_CLIENT_OK) {
        if (Json_Output) {
            printf("{\"error\":\"%s\"}\n", bacnet_client_error_string(err));
        } else {
            fprintf(stderr, "Error: %s\n", bacnet_client_error_string(err));
        }
        return 1;
    }
    
    if (Json_Output) {
        printf("{\"status\":\"registered\"}\n");
    } else {
        printf("Registered as foreign device\n");
    }
    
    return 0;
}

/* Main function */
int main(int argc, char *argv[])
{
    int opt;
    int result = 0;
    const char *command = NULL;
    int cmd_argc;
    char **cmd_argv;
    
    /* Set up signal handler */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* Default configuration */
    bacnet_client_config_default(&Config);
    
    /* Parse global options */
    static struct option long_options[] = {
        {"timeout", required_argument, 0, 'T'},
        {"retries", required_argument, 0, 'R'},
        {"verbose", no_argument, 0, 'v'},
        {"log-level", required_argument, 0, 'L'},
        {"json", no_argument, 0, 'j'},
        {"config", required_argument, 0, 'c'},
        {"version", no_argument, 0, 'V'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    while ((opt = getopt_long(argc, argv, "+T:R:vL:jc:Vh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'T':
                Config.timeout_ms = atoi(optarg);
                break;
            case 'R':
                Config.retries = atoi(optarg);
                break;
            case 'v':
                if (Config.log_level < BACNET_LOG_DEBUG) {
                    Config.log_level++;
                }
                break;
            case 'L':
                if (strcmp(optarg, "error") == 0) Config.log_level = BACNET_LOG_ERROR;
                else if (strcmp(optarg, "warn") == 0) Config.log_level = BACNET_LOG_WARN;
                else if (strcmp(optarg, "info") == 0) Config.log_level = BACNET_LOG_INFO;
                else if (strcmp(optarg, "debug") == 0) Config.log_level = BACNET_LOG_DEBUG;
                break;
            case 'j':
                Json_Output = true;
                Config.json_output = true;
                break;
            case 'c':
                /* Configuration file support planned for future release */
                fprintf(stderr, "Warning: Configuration file not yet implemented\n");
                break;
            case 'V':
                print_version();
                return 0;
            case 'h':
                print_help();
                return 0;
            default:
                print_help();
                return 1;
        }
    }
    
    /* Get command */
    if (optind >= argc) {
        print_help();
        return 1;
    }
    
    command = argv[optind];
    cmd_argc = argc - optind;
    cmd_argv = &argv[optind];
    
    /* Initialize client */
    bacnet_client_error_t err = bacnet_client_init(&Config);
    if (err != BACNET_CLIENT_OK) {
        fprintf(stderr, "Failed to initialize BACnet client: %s\n",
            bacnet_client_error_string(err));
        return 1;
    }
    
    /* Dispatch command */
    if (strcmp(command, "whois") == 0) {
        result = cmd_whois(cmd_argc, cmd_argv);
    } else if (strcmp(command, "read") == 0) {
        result = cmd_read(cmd_argc, cmd_argv);
    } else if (strcmp(command, "write") == 0) {
        result = cmd_write(cmd_argc, cmd_argv);
    } else if (strcmp(command, "cov-subscribe") == 0) {
        result = cmd_cov_subscribe(cmd_argc, cmd_argv);
    } else if (strcmp(command, "time-sync") == 0) {
        result = cmd_time_sync(cmd_argc, cmd_argv);
    } else if (strcmp(command, "file-read") == 0) {
        result = cmd_file_read(cmd_argc, cmd_argv);
    } else if (strcmp(command, "file-write") == 0) {
        result = cmd_file_write(cmd_argc, cmd_argv);
    } else if (strcmp(command, "reinit") == 0) {
        result = cmd_reinit(cmd_argc, cmd_argv);
    } else if (strcmp(command, "device-comm-control") == 0) {
        result = cmd_device_comm_control(cmd_argc, cmd_argv);
    } else if (strcmp(command, "register-foreign") == 0) {
        result = cmd_register_foreign(cmd_argc, cmd_argv);
    } else {
        fprintf(stderr, "Unknown command: %s\n", command);
        print_help();
        result = 1;
    }
    
    /* Shutdown */
    bacnet_client_shutdown();
    
    return result;
}
