/**
 * @file
 * @brief JavaScript engine implementation using Duktape
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 *
 * This module implements the JavaScript engine abstraction using Duktape,
 * providing safe execution of decoder scripts with proper error handling.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <duktape.h>
#include "js_engine.h"
#include "logging.h"

/**
 * @brief JavaScript context structure (opaque)
 */
struct js_context {
    duk_context *duk_ctx;
    bool has_decode_uplink;
    bool has_encode_downlink;
};

/**
 * @brief Custom fatal error handler for Duktape
 */
static void duk_fatal_handler(void *udata, const char *msg)
{
    (void)udata;
    LOG_ERROR("Duktape fatal error: %s", msg ? msg : "unknown");
    /* Don't abort - we want to handle errors gracefully */
}

/**
 * @brief Initialize the JavaScript engine subsystem
 */
bool js_engine_init(void)
{
    LOG_INFO("JavaScript engine (Duktape) initialized");
    return true;
}

/**
 * @brief Shutdown the JavaScript engine subsystem
 */
void js_engine_shutdown(void)
{
    LOG_INFO("JavaScript engine shutdown");
}

/**
 * @brief Create a new JavaScript context with decoder loaded
 */
js_context_t *js_context_create(const char *source_code)
{
    js_context_t *ctx = NULL;
    duk_context *duk_ctx = NULL;

    if (!source_code) {
        LOG_ERROR("Cannot create JS context: NULL source code");
        return NULL;
    }

    ctx = (js_context_t *)calloc(1, sizeof(js_context_t));
    if (!ctx) {
        LOG_ERROR("Failed to allocate JS context");
        return NULL;
    }

    /* Create Duktape heap with custom fatal handler */
    duk_ctx = duk_create_heap(NULL, NULL, NULL, NULL, duk_fatal_handler);
    if (!duk_ctx) {
        LOG_ERROR("Failed to create Duktape heap");
        free(ctx);
        return NULL;
    }

    ctx->duk_ctx = duk_ctx;

    /* Compile and execute the source code to define functions */
    if (duk_pcompile_string(duk_ctx, 0, source_code) != 0) {
        LOG_ERROR("Failed to compile decoder: %s", 
                 duk_safe_to_string(duk_ctx, -1));
        duk_destroy_heap(duk_ctx);
        free(ctx);
        return NULL;
    }

    /* Execute the compiled code */
    if (duk_pcall(duk_ctx, 0) != 0) {
        LOG_ERROR("Failed to execute decoder: %s", 
                 duk_safe_to_string(duk_ctx, -1));
        duk_destroy_heap(duk_ctx);
        free(ctx);
        return NULL;
    }
    duk_pop(duk_ctx); /* Pop execution result */

    /* Check if decodeUplink function exists */
    duk_get_global_string(duk_ctx, "decodeUplink");
    ctx->has_decode_uplink = duk_is_function(duk_ctx, -1);
    duk_pop(duk_ctx);

    /* Check if encodeDownlink function exists */
    duk_get_global_string(duk_ctx, "encodeDownlink");
    ctx->has_encode_downlink = duk_is_function(duk_ctx, -1);
    duk_pop(duk_ctx);

    if (!ctx->has_decode_uplink) {
        LOG_WARNING("Decoder does not have decodeUplink function");
    }

    LOG_DEBUG("JS context created (decodeUplink=%d, encodeDownlink=%d)",
             ctx->has_decode_uplink, ctx->has_encode_downlink);

    return ctx;
}

/**
 * @brief Destroy a JavaScript context
 */
void js_context_destroy(js_context_t *ctx)
{
    if (!ctx) {
        return;
    }

    if (ctx->duk_ctx) {
        duk_destroy_heap(ctx->duk_ctx);
    }

    free(ctx);
}

/**
 * @brief Convert Duktape value at stack top to JSON string
 */
static char *duk_value_to_json(duk_context *duk_ctx)
{
    const char *json_str = NULL;
    char *result = NULL;

    /* Use JSON.stringify to convert value to JSON */
    duk_dup(duk_ctx, -1);
    json_str = duk_json_encode(duk_ctx, -1);
    
    if (json_str) {
        result = strdup(json_str);
    }
    
    duk_pop(duk_ctx); /* Pop JSON string */
    
    return result;
}

/**
 * @brief Call the decodeUplink function with base64 data
 */
bool js_decode_uplink(
    js_context_t *ctx,
    const char *base64_data,
    js_result_t *result)
{
    duk_context *duk_ctx = NULL;

    if (!result) {
        return false;
    }

    memset(result, 0, sizeof(js_result_t));

    if (!ctx || !ctx->duk_ctx) {
        strncpy(result->error_msg, "Invalid JS context", 
               sizeof(result->error_msg) - 1);
        return false;
    }

    if (!ctx->has_decode_uplink) {
        strncpy(result->error_msg, "No decodeUplink function", 
               sizeof(result->error_msg) - 1);
        return false;
    }

    if (!base64_data) {
        strncpy(result->error_msg, "NULL base64 data", 
               sizeof(result->error_msg) - 1);
        return false;
    }

    duk_ctx = ctx->duk_ctx;

    /* Get the decodeUplink function */
    duk_get_global_string(duk_ctx, "decodeUplink");
    if (!duk_is_function(duk_ctx, -1)) {
        duk_pop(duk_ctx);
        strncpy(result->error_msg, "decodeUplink is not a function", 
               sizeof(result->error_msg) - 1);
        return false;
    }

    /* Push the base64 data as argument */
    duk_push_string(duk_ctx, base64_data);

    /* Call the function */
    if (duk_pcall(duk_ctx, 1) != 0) {
        const char *err = duk_safe_to_string(duk_ctx, -1);
        snprintf(result->error_msg, sizeof(result->error_msg),
                "Decoder error: %s", err ? err : "unknown");
        duk_pop(duk_ctx);
        return false;
    }

    /* Convert result to JSON */
    result->json_result = duk_value_to_json(duk_ctx);
    duk_pop(duk_ctx); /* Pop result */

    if (!result->json_result) {
        strncpy(result->error_msg, "Failed to convert result to JSON", 
               sizeof(result->error_msg) - 1);
        return false;
    }

    result->success = true;
    return true;
}

/**
 * @brief Call the encodeDownlink function
 */
bool js_encode_downlink(
    js_context_t *ctx,
    const char *json_data,
    js_result_t *result)
{
    duk_context *duk_ctx = NULL;

    if (!result) {
        return false;
    }

    memset(result, 0, sizeof(js_result_t));

    if (!ctx || !ctx->duk_ctx) {
        strncpy(result->error_msg, "Invalid JS context", 
               sizeof(result->error_msg) - 1);
        return false;
    }

    if (!ctx->has_encode_downlink) {
        strncpy(result->error_msg, "No encodeDownlink function", 
               sizeof(result->error_msg) - 1);
        return false;
    }

    if (!json_data) {
        strncpy(result->error_msg, "NULL JSON data", 
               sizeof(result->error_msg) - 1);
        return false;
    }

    duk_ctx = ctx->duk_ctx;

    /* Get the encodeDownlink function */
    duk_get_global_string(duk_ctx, "encodeDownlink");
    if (!duk_is_function(duk_ctx, -1)) {
        duk_pop(duk_ctx);
        strncpy(result->error_msg, "encodeDownlink is not a function", 
               sizeof(result->error_msg) - 1);
        return false;
    }

    /* Parse JSON and push as argument */
    duk_push_string(duk_ctx, json_data);
    duk_json_decode(duk_ctx, -1);

    /* Call the function */
    if (duk_pcall(duk_ctx, 1) != 0) {
        const char *err = duk_safe_to_string(duk_ctx, -1);
        snprintf(result->error_msg, sizeof(result->error_msg),
                "Encoder error: %s", err ? err : "unknown");
        duk_pop(duk_ctx);
        return false;
    }

    /* Convert result to JSON */
    result->json_result = duk_value_to_json(duk_ctx);
    duk_pop(duk_ctx); /* Pop result */

    if (!result->json_result) {
        strncpy(result->error_msg, "Failed to convert result to JSON", 
               sizeof(result->error_msg) - 1);
        return false;
    }

    result->success = true;
    return true;
}

/**
 * @brief Check if context has decodeUplink function
 */
bool js_has_decode_uplink(js_context_t *ctx)
{
    return ctx && ctx->has_decode_uplink;
}

/**
 * @brief Check if context has encodeDownlink function
 */
bool js_has_encode_downlink(js_context_t *ctx)
{
    return ctx && ctx->has_encode_downlink;
}

/**
 * @brief Free a result's allocated memory
 */
void js_result_free(js_result_t *result)
{
    if (result && result->json_result) {
        free(result->json_result);
        result->json_result = NULL;
    }
}
