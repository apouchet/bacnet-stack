/**
 * Example LoRaWAN Decoder - Generic Temperature Sensor
 *
 * A minimal decoder example for a simple temperature sensor.
 * Place this file at the appropriate path for your sensor.
 */

/**
 * Simple Base64 decoder
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
 * Decode uplink payload
 *
 * @param {string} data - Base64 encoded payload
 * @returns {object} - Decoded sensor values
 */
function decodeUplink(data) {
    var bytes = base64Decode(data);
    
    if (bytes.length < 2) {
        return { error: "Invalid payload length" };
    }
    
    // Simple format: 2 bytes signed temperature (0.01°C resolution)
    var tempRaw = (bytes[0] << 8) | bytes[1];
    if (tempRaw > 32767) tempRaw -= 65536;
    
    return {
        temperature: tempRaw / 100.0
    };
}
