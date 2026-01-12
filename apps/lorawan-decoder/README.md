# LoRaWAN MQTT Decoder Service

A production-grade LoRaWAN decoder service written in C, designed for industrial SCADA environments.

## Overview

This service:
- Listens to LoRaWAN MQTT traffic (ChirpStack v4 format: `application/+/device/+/event/up` and `application/+/device/+/event/down`)
- Dynamically loads JavaScript decoders based on device mapping
- Decodes payloads using the QuickJS JavaScript engine
- Enriches original frames with decoded data
- Republishes enriched frames to `scada/lorawan/{deveui}/up|down`

## Dependencies

Install the required development libraries:

```bash
# Ubuntu/Debian
sudo apt-get install libmosquitto-dev libjson-c-dev libquickjs

# Fedora/RHEL
sudo dnf install mosquitto-devel json-c-devel quickjs-devel
```

## Building

From the bacnet-stack root directory:

```bash
make lorawan-decoder
```

Or from this directory:

```bash
cd apps/lorawan-decoder
make
```

The binary will be placed in `bin/lorawan-decoder`.

## Configuration

### Command Line Options

```
Usage: lorawan-decoder [OPTIONS]

Options:
  -h, --host HOST         MQTT broker host (default: localhost)
  -p, --port PORT         MQTT broker port (default: 1883)
  -u, --username USER     MQTT username
  -P, --password PASS     MQTT password
  -i, --client-id ID      MQTT client ID (default: lorawan-decoder)
  -m, --sensors-map PATH  Path to sensors_map.json
  -d, --decoders PATH     Path to decoder files directory
  -l, --log-level LEVEL   Log level: debug, info, warning, error
  -j, --json-output       Output logs in JSON format
  -f, --foreground        Run in foreground (default)
  -v, --version           Print version and exit
  -?, --help              Print this help message
```

### Sensor Mapping File

The service loads sensor mappings from `/var/config/scada/sensors_map.json`:

```json
[
  {
    "id": "70-b3-d5-40-f7-5b-24-c2",
    "sensor": "nexelec/X565LS",
    "src": "lora"
  },
  {
    "id": "a8-40-41-12-34-56-78-9a",
    "sensor": "acme/TemperatureSensor",
    "src": "lora"
  }
]
```

Fields:
- `id`: Device EUI (hex string with dashes)
- `sensor`: Sensor type path (used to find decoder)
- `src`: Source type (must be "lora" to be processed)

### Decoder Files

Decoders are JavaScript files located at:
- Primary: `/var/config/scada/sensors/lora/{sensor}-decoder`
- Fallback: `/etc/scada/sensors/lora/{sensor}-decoder`

Example path for sensor `nexelec/X565LS`:
```
/var/config/scada/sensors/lora/nexelec/X565LS-decoder
```

## JavaScript Decoder Contract

### Uplink Decoder (Required)

```javascript
function decodeUplink(data) {
    // data is a Base64 encoded string
    // Return an object with decoded values
    return {
        temperature: 23.5,
        humidity: 65,
        battery: 95
    };
}
```

### Downlink Encoder (Optional)

```javascript
function encodeDownlink(data) {
    // data is the decoded object
    // Return the encoded payload
    return {
        bytes: [0x01, 0x02, 0x03],
        fPort: 10
    };
}
```

## Example Decoder

Here's a complete example decoder for a temperature sensor:

```javascript
// Example decoder for Nexelec X565LS sensor
// File: /var/config/scada/sensors/lora/nexelec/X565LS-decoder

function decodeUplink(data) {
    // Decode base64 to bytes
    var bytes = [];
    var decoded = atob(data);
    for (var i = 0; i < decoded.length; i++) {
        bytes.push(decoded.charCodeAt(i));
    }
    
    // Parse sensor data (example format)
    var result = {};
    
    if (bytes.length >= 4) {
        // Temperature: 2 bytes, signed, 0.1°C resolution
        var temp_raw = (bytes[0] << 8) | bytes[1];
        if (temp_raw > 32767) temp_raw -= 65536;
        result.temperature = temp_raw / 10.0;
        
        // Humidity: 1 byte, 0-100%
        result.humidity = bytes[2];
        
        // Battery: 1 byte, 0-100%
        result.battery = bytes[3];
    }
    
    return result;
}
```

## MQTT Topics

### Input Topics (Subscribe)

- `lora/+/+/+/up` - Uplink messages
- `lora/+/+/+/down` - Downlink messages

### Input Message Format

```json
{
  "data": "rQGDGyDuwMAHKybRSoHMK+A=",
  "deveui": "70-b3-d5-40-f7-5b-24-c2",
  "port": 56,
  "fcnt": 129,
  "rssi": -85,
  "snr": 7.5
}
```

### Output Topics (Publish)

- `scada/lorawan/{deveui}/up` - Enriched uplink messages
- `scada/lorawan/{deveui}/down` - Enriched downlink messages

### Output Message Format (Uplink)

```json
{
  "data": "rQGDGyDuwMAHKybRSoHMK+A=",
  "deveui": "70-b3-d5-40-f7-5b-24-c2",
  "port": 56,
  "fcnt": 129,
  "rssi": -85,
  "snr": 7.5,
  "uplinkDecoded": {
    "data": {
      "temperature": 23.5,
      "humidity": 65,
      "battery": 95
    }
  }
}
```

### Output Message Format (Downlink)

```json
{
  "data": "AQIDBAUGBw==",
  "deveui": "70-b3-d5-40-f7-5b-24-c2",
  "port": 10,
  "downlinkDecoded": {
    "data": {
      "command": "set_interval",
      "value": 300
    }
  }
}
```

### Error Handling

If the decoder fails, an error field is added:

```json
{
  "data": "rQGDGyDuwMAHKybRSoHMK+A=",
  "deveui": "70-b3-d5-40-f7-5b-24-c2",
  "port": 56,
  "decoderError": "Decoder error: ReferenceError: undefined variable 'x'"
}
```

## Runtime Flow

### Uplink Processing

1. Message received on `lora/app1/device1/sensor1/up`
2. Parse JSON, extract `deveui` and `data`
3. Look up sensor mapping by `deveui`
4. If no mapping or `src != "lora"`, forward unchanged
5. Load decoder from `{path}/{sensor}-decoder`
6. If no decoder file, forward unchanged
7. Execute `decodeUplink(data)`
8. Add result to frame as `uplinkDecoded.data`
9. Publish to `scada/lorawan/{deveui}/up`

### Downlink Processing

1. Message received on `lora/app1/device1/sensor1/down`
2. Parse JSON, extract `deveui` and `data`
3. Look up sensor mapping by `deveui`
4. If no mapping or `src != "lora"`, forward unchanged
5. Load decoder from `{path}/{sensor}-decoder`
6. If `encodeDownlink` exists, use it; otherwise use `decodeUplink`
7. Add result to frame as `downlinkDecoded.data`
8. Publish to `scada/lorawan/{deveui}/down`

## Error Handling

- **Invalid JSON**: Message is logged and discarded
- **Missing `data` field**: Frame is forwarded unchanged
- **No sensor mapping**: Frame is forwarded unchanged
- **No decoder file**: Frame is forwarded unchanged
- **JavaScript error**: Error is attached to frame as `decoderError`
- **Decoder crash**: Does not terminate the process

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                     lorawan-decoder                             │
├─────────────────────────────────────────────────────────────────┤
│  main.c              - Application entry point and event loop  │
│  config.c/h          - Configuration management                │
│  logging.c/h         - Logging utilities                       │
│  mqtt_handler.c/h    - MQTT subscribe/publish (libmosquitto)   │
│  sensor_map.c/h      - Sensor mapping lookup (json-c)          │
│  decoder_cache.c/h   - Decoder caching and management          │
│  js_engine.c/h       - JavaScript engine abstraction (QuickJS) │
│  frame_processor.c/h - Frame enrichment and processing         │
└─────────────────────────────────────────────────────────────────┘
```

## License

MIT License - See LICENSE file for details.
