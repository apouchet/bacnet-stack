/**
 * Example LoRaWAN Decoder - Nexelec X565LS Temperature/Humidity Sensor
 *
 * This is an example decoder that demonstrates the decoder contract.
 * Place this file at:
 *   /var/config/scada/sensors/lora/nexelec/X565LS-decoder
 *
 * The decoder receives a Base64 encoded payload and must return
 * a JavaScript object with the decoded values.
 */

/**
 * Decode a Base64 string to array of bytes
 * Note: In Duktape, atob() may not be available, so we implement our own
 */
function base64Decode(base64) {
    var chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
    var bytes = [];
    var buffer = 0;
    var bits = 0;
    
    for (var i = 0; i < base64.length; i++) {
        var c = base64.charAt(i);
        if (c === '=') break;
        
        var charIndex = chars.indexOf(c);
        if (charIndex === -1) continue;
        
        buffer = (buffer << 6) | charIndex;
        bits += 6;
        
        while (bits >= 8) {
            bits -= 8;
            bytes.push((buffer >> bits) & 0xFF);
        }
    }
    
    return bytes;
}

/**
 * Convert signed 16-bit integer from two bytes
 */
function toSigned16(high, low) {
    var value = (high << 8) | low;
    if (value > 32767) {
        value -= 65536;
    }
    return value;
}

/**
 * Decode uplink payload
 *
 * @param {string} data - Base64 encoded payload
 * @returns {object} - Decoded sensor values
 */
function decodeUplink(data) {
    var bytes = base64Decode(data);
    var result = {};
    
    // Validate minimum payload length
    if (bytes.length < 4) {
        result.error = "Payload too short";
        result.raw_length = bytes.length;
        return result;
    }
    
    // Parse sensor data according to device protocol
    // Example format:
    // Bytes 0-1: Temperature (signed 16-bit, 0.1°C resolution)
    // Byte 2: Humidity (0-100%)
    // Byte 3: Battery level (0-100%)
    
    // Temperature
    var tempRaw = toSigned16(bytes[0], bytes[1]);
    result.temperature = tempRaw / 10.0;
    result.temperature_unit = "celsius";
    
    // Humidity
    result.humidity = bytes[2];
    result.humidity_unit = "percent";
    
    // Battery
    result.battery = bytes[3];
    result.battery_unit = "percent";
    
    // Optional: Additional fields if present
    if (bytes.length >= 6) {
        // Bytes 4-5: CO2 level (unsigned 16-bit, ppm)
        var co2 = (bytes[4] << 8) | bytes[5];
        result.co2 = co2;
        result.co2_unit = "ppm";
    }
    
    if (bytes.length >= 7) {
        // Byte 6: VOC index (0-500)
        result.voc_index = bytes[6];
    }
    
    // Add timestamp
    result.decoded_at = new Date().toISOString();
    
    return result;
}

/**
 * Encode downlink payload (optional)
 *
 * @param {object} data - Command data to encode
 * @returns {object} - Encoded payload with bytes and fPort
 */
function encodeDownlink(data) {
    var bytes = [];
    var fPort = 10; // Default downlink port
    
    // Example: Set reporting interval command
    if (data.command === "set_interval") {
        bytes.push(0x01); // Command ID for set_interval
        var interval = data.value || 300; // Default 5 minutes
        bytes.push((interval >> 8) & 0xFF);
        bytes.push(interval & 0xFF);
        fPort = 10;
    }
    // Example: Request status command
    else if (data.command === "request_status") {
        bytes.push(0x02); // Command ID for request_status
        fPort = 10;
    }
    // Example: Set threshold command
    else if (data.command === "set_threshold") {
        bytes.push(0x03); // Command ID for set_threshold
        var threshold = Math.round((data.temperature || 25) * 10);
        bytes.push((threshold >> 8) & 0xFF);
        bytes.push(threshold & 0xFF);
        fPort = 11;
    }
    else {
        return {
            error: "Unknown command: " + data.command
        };
    }
    
    return {
        bytes: bytes,
        fPort: fPort
    };
}
