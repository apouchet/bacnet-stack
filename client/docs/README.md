# BACnet Client

A production-quality BACnet/IP client for Linux, built on the bacnet-stack library.

## Features

- **Device Discovery**: Find BACnet devices on the network using Who-Is/I-Am
- **Property Access**: Read and write BACnet properties
- **COV Subscriptions**: Subscribe to Change-of-Value notifications
- **File Transfer**: Read and write files using AtomicReadFile/AtomicWriteFile
- **Time Synchronization**: Send time sync to devices
- **Device Control**: Reinitialize devices, control communication
- **BBMD Support**: Register as foreign device with BBMD
- **JSON Output**: Machine-parseable output format
- **Library API**: Reusable C library for embedding in other applications

## Building

### Prerequisites

- GCC compiler
- GNU Make
- Linux (Ubuntu recommended)

### Build Steps

```bash
# From the repository root
cd client
make

# Or with debug symbols
make BUILD=debug
```

The built binary will be at `client/bin/bacnet-client`.

### Dependencies

The client uses the bacnet-stack library which is built automatically.

## Installation

```bash
sudo make install
```

This installs:
- `/usr/local/bin/bacnet-client` - CLI tool
- `/usr/local/lib/libbacnet_client.a` - Client library
- `/usr/local/include/bacnet/bacnet_client.h` - Library header

## Usage

### Environment Variables

- `BACNET_IFACE` - Network interface (default: auto-detect)
- `BACNET_IP_PORT` - UDP port (default: 47808)
- `BACNET_BBMD_PORT` - BBMD port
- `BACNET_BBMD_ADDRESS` - BBMD IP address

### Global Options

```
--timeout <ms>     APDU timeout in milliseconds (default: 3000)
--retries <n>      Number of retries (default: 3)
--verbose, -v      Increase verbosity
--log-level <lvl>  Set log level: error, warn, info, debug
--json             Output in JSON format
--config <file>    Configuration file path
--version          Show version information
--help, -h         Show help message
```

### Commands

#### Device Discovery (whois)

Discover BACnet devices on the network:

```bash
# Discover all devices
bacnet-client whois

# Discover specific device range
bacnet-client whois --device-instance 100-200

# With timeout
bacnet-client whois --timeout 5000

# JSON output
bacnet-client whois --json
```

Example output:
```
Discovering BACnet devices...
Device 1234: MaxAPDU=1476 Segmentation=segmented-both Vendor=260
  MAC: C0:A8:01:64:BA:C0

Found 1 device(s)
```

#### Read Property (read)

Read a property from a BACnet object:

```bash
# Read object name
bacnet-client read --device 1234 --object device:1234 --property object-name

# Read present value
bacnet-client read --device 1234 --object analog-input:1 --property present-value

# Read array element
bacnet-client read --device 1234 --object device:1234 --property object-list --index 1

# JSON output
bacnet-client read --device 1234 --object analog-input:1 --property present-value --json
```

#### Write Property (write)

Write a property to a BACnet object:

```bash
# Write numeric value
bacnet-client write --device 1234 --object analog-output:1 --property present-value --value 75.5

# Write boolean
bacnet-client write --device 1234 --object binary-output:1 --property present-value --value true

# Write with priority
bacnet-client write --device 1234 --object analog-output:1 --property present-value --value 75.5 --priority 8

# Write null (release priority)
bacnet-client write --device 1234 --object analog-output:1 --property present-value --value null --priority 8
```

#### COV Subscribe (cov-subscribe)

Subscribe to Change-of-Value notifications:

```bash
# Simple subscription
bacnet-client cov-subscribe --device 1234 --object analog-input:1 --lifetime 300

# Confirmed notifications
bacnet-client cov-subscribe --device 1234 --object analog-input:1 --lifetime 300 --confirmed

# With COV increment
bacnet-client cov-subscribe --device 1234 --object analog-input:1 --lifetime 300 --cov-increment 0.5
```

The client will display COV notifications as they arrive. Press Ctrl+C to unsubscribe and exit.

#### Time Synchronization (time-sync)

Send time synchronization to devices:

```bash
# Broadcast time sync
bacnet-client time-sync --device broadcast

# UTC time sync to specific device
bacnet-client time-sync --device 1234 --utc
```

#### File Read (file-read)

Read a file from a device:

```bash
bacnet-client file-read --device 1234 --file 1 --out /tmp/downloaded.txt
```

#### File Write (file-write)

Write a file to a device:

```bash
bacnet-client file-write --device 1234 --file 1 --in /tmp/upload.txt
```

#### Reinitialize Device (reinit)

Reinitialize a device:

```bash
# Cold start
bacnet-client reinit --device 1234 --option cold-start

# Warm start with password
bacnet-client reinit --device 1234 --option warm-start --password secret
```

#### Device Communication Control (device-comm-control)

Control device communication:

```bash
# Enable communication
bacnet-client device-comm-control --device 1234 --enable

# Disable for 5 minutes
bacnet-client device-comm-control --device 1234 --disable --time 5
```

#### Register Foreign Device (register-foreign)

Register as a foreign device with a BBMD:

```bash
bacnet-client register-foreign --bbmd 192.168.1.1:47808 --ttl 60
```

## Library API

The client library can be used in other C applications:

```c
#include <bacnet/bacnet_client.h>

int main() {
    bacnet_client_config_t config;
    bacnet_client_config_default(&config);
    
    if (bacnet_client_init(&config) != BACNET_CLIENT_OK) {
        return 1;
    }
    
    // Discover devices
    bacnet_client_discover(-1, -1, 3000, my_callback, NULL);
    
    // Read property
    bacnet_read_result_t result;
    bacnet_client_read(1234, OBJECT_ANALOG_INPUT, 1, 
                       PROP_PRESENT_VALUE, BACNET_ARRAY_ALL, &result);
    
    bacnet_client_shutdown();
    return 0;
}
```

See `bacnet_client.h` for the complete API documentation.

## Configuration File

Create a YAML configuration file:

```yaml
# bacnet-client.yaml
network:
  interface: eth0
  port: 47808
  
timeouts:
  apdu: 3000
  retries: 3
  
bbmd:
  address: 192.168.1.1
  port: 47808
  ttl: 60
  
logging:
  level: info
```

Use with `--config`:

```bash
bacnet-client --config /etc/bacnet-client.yaml whois
```

## Docker

Build and run in a container:

```bash
# Build image
docker build -t bacnet-client .

# Run
docker run --rm --net=host bacnet-client whois
```

## Systemd Service

Install the systemd unit file:

```bash
sudo cp systemd/bacnet-client.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable bacnet-client
sudo systemctl start bacnet-client
```

## PICS Summary

### Supported BIBBs (BACnet Interoperability Building Blocks)

| BIBB | Name | Status |
|------|------|--------|
| DS-RP-B | Data Sharing - ReadProperty - B | ✅ Supported |
| DS-RPM-B | Data Sharing - ReadPropertyMultiple - B | ✅ Supported |
| DS-WP-B | Data Sharing - WriteProperty - B | ✅ Supported |
| DS-WPM-B | Data Sharing - WritePropertyMultiple - B | ✅ Supported |
| DS-COV-B | Data Sharing - COV - B | ✅ Supported |
| DM-DDB-B | Device Management - Dynamic Device Binding - B | ✅ Supported |
| DM-DOB-B | Device Management - Dynamic Object Binding - B | ✅ Supported |
| DM-DCC-B | Device Management - DeviceCommunicationControl - B | ✅ Supported |
| DM-RD-B | Device Management - ReinitializeDevice - B | ✅ Supported |
| DM-TS-B | Device Management - TimeSynchronization - B | ✅ Supported |
| DM-UTC-B | Device Management - UTCTimeSynchronization - B | ✅ Supported |
| AE-ACK-B | Alarm/Event - AcknowledgeAlarm - B | ✅ Supported |
| AE-INFO-B | Alarm/Event - GetEventInformation - B | Partial |
| T-ATR-B | Trending - AtomicReadFile - B | ✅ Supported |
| T-AWF-B | Trending - AtomicWriteFile - B | ✅ Supported |

### Network Layer

- BACnet/IP (Annex J) - ✅ Full support
- Foreign Device Registration - ✅ Supported
- BBMD Client - ✅ Supported
- MS/TP - Gateway only (via BACnet router)

### Segmentation

- Segmented requests - ✅ Supported (via bacnet-stack)
- Segmented responses - ✅ Supported (via bacnet-stack)

### Limitations

1. **BACnet/SC**: Not directly supported. Use a gateway/adapter.
2. **MS/TP**: Requires a BACnet router or gateway.
3. **Complex ACK parsing**: Some complex property types may not display fully.
4. **Private Transfer**: Basic support; vendor-specific encoding required.

## Troubleshooting

### No devices found

1. Check network interface: `export BACNET_IFACE=eth0`
2. Verify firewall allows UDP port 47808
3. Check routing and subnet configuration
4. Try increasing timeout: `--timeout 10000`

### Connection timeout

1. Verify device is online and responding
2. Check if device is on different subnet (may need BBMD)
3. Try unicast address if broadcast fails

### Permission denied

UDP port 47808 may require root privileges. Either:
- Run as root (not recommended)
- Use a port above 1024: `export BACNET_IP_PORT=47809`
- Configure capabilities: `setcap cap_net_bind_service=+ep bacnet-client`

## License

This software is released under the MIT License. See LICENSE file.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## Support

For issues and feature requests, please use the GitHub issue tracker.
