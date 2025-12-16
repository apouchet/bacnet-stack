# BACnet MQTT Server

A dynamic BACnet server (responder) that creates and updates BACnet objects based on incoming MQTT messages and JSON configuration files.

## Overview

This application bridges MQTT-based IoT sensors (particularly LoRaWAN sensors) to the BACnet building automation protocol. It:

- Subscribes to MQTT topics for sensor data
- Dynamically creates BACnet objects based on sensor definitions
- Updates BACnet object values when new MQTT messages arrive
- Supports COV (Change of Value) notifications
- Provides full BACnet server functionality (Who-Is, ReadProperty, etc.)

## Architecture

```
┌─────────────────┐     ┌─────────────────────┐     ┌─────────────────┐
│   MQTT Broker   │────▶│  bacnet-mqtt-server │────▶│  BACnet Client  │
│  (Mosquitto)    │     │                     │     │  (BAS, SCADA)   │
└─────────────────┘     └─────────────────────┘     └─────────────────┘
                                  │
                                  ▼
                        ┌─────────────────────┐
                        │   JSON Config Files │
                        │  - sensors_map.json │
                        │  - sensor/*.json    │
                        └─────────────────────┘
```

## MQTT to BACnet Flow

```
MQTT Message                     BACnet Objects
────────────                     ──────────────
{                                ┌────────────────────────┐
  "deveui": "70-b3-d5...24c2",   │ Analog Input           │
  "uplinkDecoded": {             │   Name: temperature-24c2│
    "data": {                    │   Present Value: 23.4  │
      "temperature": 23.4,  ──▶  │   Units: degrees-celsius│
      "humidity": 49.5,     ──▶  └────────────────────────┘
      "co2": 838            ──▶  ┌────────────────────────┐
    }                            │ Analog Input           │
  }                              │   Name: humidity-24c2  │
}                                │   Present Value: 49.5  │
                                 │   Units: percent-RH    │
                                 └────────────────────────┘
                                 ┌────────────────────────┐
                                 │ Analog Input           │
                                 │   Name: co2-24c2       │
                                 │   Present Value: 838   │
                                 │   Units: ppm           │
                                 └────────────────────────┘
```

## Installation

### Prerequisites

```bash
# Install required libraries
sudo apt-get install libmosquitto-dev libjson-c-dev libyaml-dev
```

### Build

```bash
# Build the BACnet library and application
cd bacnet-stack
make LEGACY=true -C apps bacnet-mqtt-server

# Binary is created at bin/bacnet-mqtt-server
```

### Install

```bash
# Copy binary
sudo cp bin/bacnet-mqtt-server /usr/local/bin/

# Create config directory
sudo mkdir -p /etc/bacnet-mqtt-server
sudo cp apps/bacnet-mqtt-server/examples/config.yaml /etc/bacnet-mqtt-server/

# Create sensor config directories
sudo mkdir -p /var/config/scada/sensors/lora
sudo cp apps/bacnet-mqtt-server/examples/sensors_map.json /var/config/scada/
sudo cp -r apps/bacnet-mqtt-server/examples/sensors/* /var/config/scada/sensors/

# Install systemd service (optional)
sudo cp apps/bacnet-mqtt-server/examples/bacnet-mqtt-server.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable bacnet-mqtt-server
```

## Configuration

### Main Configuration File

Location: `/etc/bacnet-mqtt-server/config.yaml`

```yaml
mqtt:
  host: localhost
  port: 1883
  topic: "scada/lorawan/#"
  
bacnet:
  device_instance: 260001
  udp_port: 47808
  device_name: "BACnet-MQTT-Server"

paths:
  sensors_map: "/var/config/scada/sensors_map.json"
  sensors_base: "/var/config/scada/sensors"

logging:
  level: info
```

### Sensor Map File

Location: `/var/config/scada/sensors_map.json`

Maps device EUIs to sensor types:

```json
[
  {
    "id": "70-b3-d5-40-f7-5b-24-c2",
    "sensor": "nexelec/X565LS",
    "src": "lora"
  }
]
```

### Sensor Definition Files

Location: `/var/config/scada/sensors/{src}/{sensor}.json`

Defines properties for each sensor type:

```json
{
  "description": "X565LS - Multi-sensor",
  "properties": {
    "temperature": {
      "type": "float",
      "units": "degrees-celsius"
    },
    "humidity": {
      "type": "float",
      "units": "percent-relative-humidity"
    },
    "co2": {
      "type": "uint8",
      "units": "parts-per-million"
    }
  }
}
```

## Usage

### Command Line Options

```
bacnet-mqtt-server [OPTIONS]

Options:
  -c, --config FILE      Configuration file path
  -f, --foreground       Run in foreground (don't daemonize)
  -l, --log-level LEVEL  Log level: debug, info, warning, error
  -d, --device-id ID     BACnet device instance number
  -h, --help             Show help message
  -v, --version          Show version information

Environment Variables:
  BACNET_IFACE           Network interface for BACnet/IP
  BACNET_IP_PORT         UDP port for BACnet/IP (default: 47808)
```

### Running the Server

```bash
# Run in foreground for testing
./bin/bacnet-mqtt-server --foreground --log-level debug

# Run with custom config
./bin/bacnet-mqtt-server --config /path/to/config.yaml --foreground

# Run as systemd service
sudo systemctl start bacnet-mqtt-server
sudo journalctl -u bacnet-mqtt-server -f
```

### Testing with BACnet Tools

```bash
# Discover the device
./bin/whois

# Read device properties
./bin/readprop 260001 8 75    # Object Name
./bin/readprop 260001 8 121   # Vendor Name

# Read dynamic Analog Input values
./bin/readprop 260001 0 <instance> 85   # Present Value
```

## BACnet Object Model

### Device Object

- Vendor Name: Configurable
- Vendor ID: Configurable  
- Model Name: Configurable
- Application Software Version: Configurable
- Protocol Services Supported: Who-Is, ReadProperty, ReadPropertyMultiple, WriteProperty, SubscribeCOV, etc.

### Dynamic Objects

For each `(devEUI, property)` pair:

| Property Type | BACnet Object Type |
|---------------|-------------------|
| float, double | Analog Input |
| int, uint8, etc. | Analog Input |
| string | CharacterString Value |
| bool | Analog Input (0.0 or 1.0) |

**Object Naming Convention:** `{property}-{last4(devEUI)}`

Example: `humidity-24c2` for humidity from device ending in 24c2

## Supported BACnet Services

### Mandatory (Implemented)

- Who-Is / I-Am
- ReadProperty
- ReadPropertyMultiple

### Optional (Implemented)

- WriteProperty
- WritePropertyMultiple
- SubscribeCOV
- UnconfirmedCOVNotification
- TimeSynchronization
- UTCTimeSynchronization
- DeviceCommunicationControl
- ReinitializeDevice
- ReadRange

### Alarm/Event Services

When built with `INTRINSIC_REPORTING`:
- GetAlarmSummary
- GetEventInformation
- AcknowledgeAlarm

## COV (Change of Value) Support

All dynamically created Analog Input objects support COV:

- Default COV increment: 0.1
- COV notifications triggered when Present_Value changes
- Supports multiple subscribers
- Automatic subscription expiration

## Limitations

1. **Binary Objects**: Currently not implemented. All boolean values are converted to Analog Inputs with 0.0/1.0 values.

2. **Trend Logs**: TrendLog objects are included but dynamic creation is not implemented yet.

3. **Vendor Name**: Must be set at compile time via `BACNET_VENDOR_NAME` define (runtime setting not available in current bacnet-stack).

4. **String Values**: CharacterString Value objects are created for string properties, but updating them with string values requires additional parsing logic.

## Extension Points

### Adding New Sensor Types

1. Create a new sensor definition JSON file in `/var/config/scada/sensors/lora/{vendor}/`
2. Add the sensor mapping to `sensors_map.json`
3. The server will automatically pick up the new configuration (checks every 10 seconds)

### Adding New Property Types

1. Extend `property_data_type_t` enum in `sensor_def.h`
2. Add parsing in `sensor_def_parse_type()`
3. Add object creation logic in `object_factory.c`

### Adding New BACnet Object Types

1. Include the object header in `object_factory.c`
2. Add initialization in `object_factory_init()`
3. Add creation logic for the new type

## Troubleshooting

### Cannot connect to MQTT broker

```bash
# Check broker is running
sudo systemctl status mosquitto

# Test connectivity
mosquitto_sub -h localhost -t "scada/lorawan/#" -v
```

### BACnet device not discovered

```bash
# Check network interface
ip addr show

# Set interface explicitly
export BACNET_IFACE=eth0
./bin/bacnet-mqtt-server --foreground
```

### Sensor data not creating objects

```bash
# Enable debug logging
./bin/bacnet-mqtt-server --foreground --log-level debug

# Check:
# 1. devEUI is in sensors_map.json
# 2. Sensor definition file exists
# 3. Property names match between MQTT data and definition
```

## License

SPDX-License-Identifier: MIT

Part of the BACnet Stack project: https://github.com/bacnet-stack/bacnet-stack
