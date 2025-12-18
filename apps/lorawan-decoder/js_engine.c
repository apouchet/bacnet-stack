/**
 * @file
 * @brief JavaScript engine implementation using QuickJS
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 *
 * This module implements the JavaScript engine abstraction using QuickJS,
 * providing safe execution of decoder scripts with proper error handling.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <quickjs/quickjs.h>
#include "js_engine.h"
#include "logging.h"

/**
 * @brief JavaScript context structure (opaque)
 */
struct js_context {
    JSRuntime *rt;
    JSContext *ctx;
    bool has_decode_uplink;
    bool has_encode_downlink;
};

/**
 * @brief Initialize the JavaScript engine subsystem
 */
bool js_engine_init(void)
{
    LOG_INFO("JavaScript engine (QuickJS) initialized");
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
 * @brief Get exception message from QuickJS context
 */
static char *get_exception_message(JSContext *ctx)
{
    JSValue exception = JS_GetException(ctx);
    const char *str = JS_ToCString(ctx, exception);
    char *result = NULL;
    
    if (str) {
        result = strdup(str);
        JS_FreeCString(ctx, str);
    }
    
    JS_FreeValue(ctx, exception);
    return result;
}

/**
 * @brief Base64 decoding table
 */
static const int b64_table[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
    52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
    15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
    -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
    41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

/**
 * @brief Decode base64 string to bytes
 */
static int base64_decode(const char *input, unsigned char *output, size_t *out_len)
{
    size_t len = strlen(input);
    size_t i, j = 0;
    int v;

    for (i = 0; i < len; i += 4) {
        int a = b64_table[(unsigned char)input[i]];
        int b = (i + 1 < len) ? b64_table[(unsigned char)input[i + 1]] : -1;
        int c = (i + 2 < len) ? b64_table[(unsigned char)input[i + 2]] : -1;
        int d = (i + 3 < len) ? b64_table[(unsigned char)input[i + 3]] : -1;

        if (a == -1 || b == -1) break;

        v = (a << 18) | (b << 12);
        output[j++] = (v >> 16) & 0xFF;

        if (c != -1) {
            v |= (c << 6);
            output[j++] = (v >> 8) & 0xFF;
        }

        if (d != -1) {
            v |= d;
            output[j++] = v & 0xFF;
        }
    }

    *out_len = j;
    return 0;
}

/**
 * @brief Create a new JavaScript context with decoder loaded
 */
js_context_t *js_context_create(const char *source_code)
{
    js_context_t *js_ctx = NULL;
    JSRuntime *rt = NULL;
    JSContext *ctx = NULL;
    JSValue result;
    JSValue global_obj;
    JSValue func_val;

    if (!source_code) {
        LOG_ERROR("Cannot create JS context: NULL source code");
        return NULL;
    }

    js_ctx = (js_context_t *)calloc(1, sizeof(js_context_t));
    if (!js_ctx) {
        LOG_ERROR("Failed to allocate JS context");
        return NULL;
    }

    /* Create QuickJS runtime */
    rt = JS_NewRuntime();
    if (!rt) {
        LOG_ERROR("Failed to create QuickJS runtime");
        free(js_ctx);
        return NULL;
    }

    /* Create QuickJS context */
    ctx = JS_NewContext(rt);
    if (!ctx) {
        LOG_ERROR("Failed to create QuickJS context");
        JS_FreeRuntime(rt);
        free(js_ctx);
        return NULL;
    }

    js_ctx->rt = rt;
    js_ctx->ctx = ctx;

    /* Evaluate the source code to define functions */
    result = JS_Eval(ctx, source_code, strlen(source_code), "<decoder>", 
                     JS_EVAL_TYPE_GLOBAL);
    
    if (JS_IsException(result)) {
        char *err = get_exception_message(ctx);
        LOG_ERROR("Failed to evaluate decoder: %s", err ? err : "unknown");
        free(err);
        JS_FreeValue(ctx, result);
        JS_FreeContext(ctx);
        JS_FreeRuntime(rt);
        free(js_ctx);
        return NULL;
    }
    JS_FreeValue(ctx, result);

    /* Check if decodeUplink function exists */
    global_obj = JS_GetGlobalObject(ctx);
    
    func_val = JS_GetPropertyStr(ctx, global_obj, "decodeUplink");
    js_ctx->has_decode_uplink = JS_IsFunction(ctx, func_val);
    JS_FreeValue(ctx, func_val);

    /* Check if encodeDownlink function exists */
    func_val = JS_GetPropertyStr(ctx, global_obj, "encodeDownlink");
    js_ctx->has_encode_downlink = JS_IsFunction(ctx, func_val);
    JS_FreeValue(ctx, func_val);

    JS_FreeValue(ctx, global_obj);

    if (!js_ctx->has_decode_uplink) {
        LOG_WARNING("Decoder does not have decodeUplink function");
    }

    LOG_DEBUG("JS context created (decodeUplink=%d, encodeDownlink=%d)",
             js_ctx->has_decode_uplink, js_ctx->has_encode_downlink);

    return js_ctx;
}

/**
 * @brief Destroy a JavaScript context
 */
void js_context_destroy(js_context_t *js_ctx)
{
    if (!js_ctx) {
        return;
    }

    if (js_ctx->ctx) {
        JS_FreeContext(js_ctx->ctx);
    }

    if (js_ctx->rt) {
        JS_FreeRuntime(js_ctx->rt);
    }

    free(js_ctx);
}

/**
 * @brief Convert QuickJS value to JSON string
 */
static char *js_value_to_json(JSContext *ctx, JSValue val)
{
    JSValue json_val;
    const char *json_str;
    char *result = NULL;

    /* Use JSON.stringify to convert value to JSON */
    json_val = JS_JSONStringify(ctx, val, JS_UNDEFINED, JS_UNDEFINED);
    
    if (JS_IsException(json_val)) {
        JS_FreeValue(ctx, json_val);
        return NULL;
    }

    json_str = JS_ToCString(ctx, json_val);
    if (json_str) {
        result = strdup(json_str);
        JS_FreeCString(ctx, json_str);
    }
    
    JS_FreeValue(ctx, json_val);
    return result;
}

/**
 * @brief Call the decodeUplink function with base64 data
 */
bool js_decode_uplink(
    js_context_t *js_ctx,
    const char *base64_data,
    js_result_t *result)
{
    JSContext *ctx;
    JSValue global_obj;
    JSValue func_val;
    JSValue arg_val;
    JSValue ret_val;

    if (!result) {
        return false;
    }

    memset(result, 0, sizeof(js_result_t));

    if (!js_ctx || !js_ctx->ctx) {
        strncpy(result->error_msg, "Invalid JS context", 
               sizeof(result->error_msg) - 1);
        return false;
    }

    if (!js_ctx->has_decode_uplink) {
        strncpy(result->error_msg, "No decodeUplink function", 
               sizeof(result->error_msg) - 1);
        return false;
    }

    if (!base64_data) {
        strncpy(result->error_msg, "NULL base64 data", 
               sizeof(result->error_msg) - 1);
        return false;
    }

    ctx = js_ctx->ctx;

    /* Get the decodeUplink function */
    global_obj = JS_GetGlobalObject(ctx);
    func_val = JS_GetPropertyStr(ctx, global_obj, "decodeUplink");

    if (!JS_IsFunction(ctx, func_val)) {
        JS_FreeValue(ctx, func_val);
        JS_FreeValue(ctx, global_obj);
        strncpy(result->error_msg, "decodeUplink is not a function", 
               sizeof(result->error_msg) - 1);
        return false;
    }

    /* Build input object: { bytes: [...] } from base64 */
    unsigned char bytes[256];
    size_t bytes_len = 0;
    base64_decode(base64_data, bytes, &bytes_len);

    JSValue input_obj = JS_NewObject(ctx);
    JSValue bytes_array = JS_NewArray(ctx);
    for (size_t i = 0; i < bytes_len; i++) {
        JS_SetPropertyUint32(ctx, bytes_array, i, JS_NewInt32(ctx, bytes[i]));
    }
    JS_SetPropertyStr(ctx, input_obj, "bytes", bytes_array);

    arg_val = input_obj;

    /* Call decodeUplink(input) */
    ret_val = JS_Call(ctx, func_val, global_obj, 1, &arg_val);

    JS_FreeValue(ctx, arg_val);
    JS_FreeValue(ctx, func_val);

    if (JS_IsException(ret_val)) {
        char *err = get_exception_message(ctx);
        snprintf(result->error_msg, sizeof(result->error_msg),
                "Decoder error: %s", err ? err : "unknown");
        free(err);
        JS_FreeValue(ctx, ret_val);
        JS_FreeValue(ctx, global_obj);
        return false;
    }

    /* Convert result to JSON */
    result->json_result = js_value_to_json(ctx, ret_val);
    JS_FreeValue(ctx, ret_val);
    JS_FreeValue(ctx, global_obj);

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
    js_context_t *js_ctx,
    const char *json_data,
    js_result_t *result)
{
    JSContext *ctx;
    JSValue global_obj;
    JSValue func_val;
    JSValue arg_val;
    JSValue ret_val;

    if (!result) {
        return false;
    }

    memset(result, 0, sizeof(js_result_t));

    if (!js_ctx || !js_ctx->ctx) {
        strncpy(result->error_msg, "Invalid JS context", 
               sizeof(result->error_msg) - 1);
        return false;
    }

    if (!js_ctx->has_encode_downlink) {
        strncpy(result->error_msg, "No encodeDownlink function", 
               sizeof(result->error_msg) - 1);
        return false;
    }

    if (!json_data) {
        strncpy(result->error_msg, "NULL JSON data", 
               sizeof(result->error_msg) - 1);
        return false;
    }

    ctx = js_ctx->ctx;

    /* Get the encodeDownlink function */
    global_obj = JS_GetGlobalObject(ctx);
    func_val = JS_GetPropertyStr(ctx, global_obj, "encodeDownlink");

    if (!JS_IsFunction(ctx, func_val)) {
        JS_FreeValue(ctx, func_val);
        JS_FreeValue(ctx, global_obj);
        strncpy(result->error_msg, "encodeDownlink is not a function", 
               sizeof(result->error_msg) - 1);
        return false;
    }

    /* Parse JSON input */
    arg_val = JS_ParseJSON(ctx, json_data, strlen(json_data), "<input>");
    
    if (JS_IsException(arg_val)) {
        char *err = get_exception_message(ctx);
        snprintf(result->error_msg, sizeof(result->error_msg),
                "JSON parse error: %s", err ? err : "unknown");
        free(err);
        JS_FreeValue(ctx, arg_val);
        JS_FreeValue(ctx, func_val);
        JS_FreeValue(ctx, global_obj);
        return false;
    }

    /* Call the function */
    ret_val = JS_Call(ctx, func_val, global_obj, 1, &arg_val);

    JS_FreeValue(ctx, arg_val);
    JS_FreeValue(ctx, func_val);

    if (JS_IsException(ret_val)) {
        char *err = get_exception_message(ctx);
        snprintf(result->error_msg, sizeof(result->error_msg),
                "Encoder error: %s", err ? err : "unknown");
        free(err);
        JS_FreeValue(ctx, ret_val);
        JS_FreeValue(ctx, global_obj);
        return false;
    }

    /* Convert result to JSON */
    result->json_result = js_value_to_json(ctx, ret_val);
    JS_FreeValue(ctx, ret_val);
    JS_FreeValue(ctx, global_obj);

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
