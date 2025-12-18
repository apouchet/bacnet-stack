/**
 * @file
 * @brief JavaScript engine abstraction for LoRaWAN MQTT Decoder Service
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 *
 * This module provides an abstraction layer over the QuickJS JavaScript engine
 * for executing decoder scripts safely.
 */
#ifndef LORAWAN_DECODER_JS_ENGINE_H
#define LORAWAN_DECODER_JS_ENGINE_H

#include <stdbool.h>
#include <stddef.h>

/* Maximum size for decoder source code */
#define JS_MAX_SOURCE_SIZE (256 * 1024)

/* Maximum size for result JSON */
#define JS_MAX_RESULT_SIZE (64 * 1024)

/* Forward declaration for opaque handle */
typedef struct js_context js_context_t;

/**
 * @brief Decoder execution result
 */
typedef struct js_result {
    bool success;
    char *json_result;    /* JSON string output (must be freed by caller) */
    char error_msg[512];  /* Error message if !success */
} js_result_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the JavaScript engine subsystem
 * @return true on success, false on failure
 */
bool js_engine_init(void);

/**
 * @brief Shutdown the JavaScript engine subsystem
 */
void js_engine_shutdown(void);

/**
 * @brief Create a new JavaScript context with decoder loaded
 * @param source_code JavaScript source code to load
 * @return Opaque context handle, or NULL on failure
 */
js_context_t *js_context_create(const char *source_code);

/**
 * @brief Destroy a JavaScript context
 * @param ctx Context to destroy
 */
void js_context_destroy(js_context_t *ctx);

/**
 * @brief Call the decodeUplink function with base64 data
 * @param ctx JavaScript context
 * @param base64_data Base64 encoded payload data
 * @param result Output result structure
 * @return true if execution succeeded, false on error
 */
bool js_decode_uplink(
    js_context_t *ctx,
    const char *base64_data,
    js_result_t *result);

/**
 * @brief Call the encodeDownlink function (optional)
 * @param ctx JavaScript context
 * @param json_data JSON data to encode
 * @param result Output result structure
 * @return true if execution succeeded, false on error
 */
bool js_encode_downlink(
    js_context_t *ctx,
    const char *json_data,
    js_result_t *result);

/**
 * @brief Check if context has decodeUplink function
 * @param ctx JavaScript context
 * @return true if function exists
 */
bool js_has_decode_uplink(js_context_t *ctx);

/**
 * @brief Check if context has encodeDownlink function
 * @param ctx JavaScript context
 * @return true if function exists
 */
bool js_has_encode_downlink(js_context_t *ctx);

/**
 * @brief Free a result's allocated memory
 * @param result Result to free
 */
void js_result_free(js_result_t *result);

#ifdef __cplusplus
}
#endif

#endif /* LORAWAN_DECODER_JS_ENGINE_H */
