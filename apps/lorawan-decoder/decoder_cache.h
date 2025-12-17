/**
 * @file
 * @brief Decoder cache module for LoRaWAN MQTT Decoder Service
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 *
 * This module manages caching of compiled JavaScript decoders to avoid
 * reloading on every message.
 */
#ifndef LORAWAN_DECODER_CACHE_H
#define LORAWAN_DECODER_CACHE_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include "js_engine.h"

/* Maximum number of cached decoders */
#define DECODER_CACHE_MAX_ENTRIES 256

/* Maximum path length */
#define DECODER_PATH_MAX_LEN 512

/**
 * @brief Decoder cache entry
 */
typedef struct decoder_cache_entry {
    char sensor[128];           /* Sensor type (e.g., "nexelec/X565LS") */
    char filepath[DECODER_PATH_MAX_LEN];
    js_context_t *js_ctx;       /* Compiled JavaScript context */
    time_t file_mtime;          /* File modification time when loaded */
    time_t last_used;           /* Last usage time */
    bool valid;                 /* Entry is valid */
} decoder_cache_entry_t;

/**
 * @brief Decoder cache structure
 */
typedef struct decoder_cache {
    decoder_cache_entry_t entries[DECODER_CACHE_MAX_ENTRIES];
    size_t count;
    char base_path[DECODER_PATH_MAX_LEN];
    char alt_path[DECODER_PATH_MAX_LEN];
} decoder_cache_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the decoder cache
 * @param cache Pointer to decoder cache structure
 * @param base_path Base path for decoder files
 * @param alt_path Alternative base path for decoder files
 * @return true on success, false on failure
 */
bool decoder_cache_init(
    decoder_cache_t *cache,
    const char *base_path,
    const char *alt_path);

/**
 * @brief Get or load a decoder for a sensor type
 * @param cache Pointer to decoder cache
 * @param sensor Sensor type string (e.g., "nexelec/X565LS")
 * @return Pointer to JS context, or NULL if not found
 */
js_context_t *decoder_cache_get(
    decoder_cache_t *cache,
    const char *sensor);

/**
 * @brief Check if a decoder exists for a sensor type
 * @param cache Pointer to decoder cache
 * @param sensor Sensor type string
 * @return true if decoder exists
 */
bool decoder_cache_exists(
    decoder_cache_t *cache,
    const char *sensor);

/**
 * @brief Invalidate and reload a specific decoder
 * @param cache Pointer to decoder cache
 * @param sensor Sensor type string
 * @return true if reloaded successfully
 */
bool decoder_cache_reload(
    decoder_cache_t *cache,
    const char *sensor);

/**
 * @brief Cleanup decoder cache and free all resources
 * @param cache Pointer to decoder cache
 */
void decoder_cache_cleanup(decoder_cache_t *cache);

/**
 * @brief Get cache statistics
 * @param cache Pointer to decoder cache
 * @param count Output: number of cached entries
 */
void decoder_cache_stats(
    const decoder_cache_t *cache,
    size_t *count);

#ifdef __cplusplus
}
#endif

#endif /* LORAWAN_DECODER_CACHE_H */
