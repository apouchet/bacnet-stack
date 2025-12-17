/**
 * @file
 * @brief Decoder cache module implementation for LoRaWAN MQTT Decoder Service
 * @author GitHub Copilot
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "decoder_cache.h"
#include "logging.h"

/**
 * @brief Load decoder source code from file
 */
static char *load_decoder_file(const char *filepath, time_t *mtime)
{
    FILE *fp = NULL;
    char *content = NULL;
    long file_size;
    struct stat st;

    if (!filepath) {
        return NULL;
    }

    if (stat(filepath, &st) != 0) {
        return NULL;
    }

    if (mtime) {
        *mtime = st.st_mtime;
    }

    fp = fopen(filepath, "r");
    if (!fp) {
        LOG_WARNING("Cannot open decoder file: %s", filepath);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size <= 0 || file_size > JS_MAX_SOURCE_SIZE) {
        LOG_WARNING("Decoder file invalid size (%ld): %s", file_size, filepath);
        fclose(fp);
        return NULL;
    }

    content = (char *)malloc((size_t)file_size + 1);
    if (!content) {
        LOG_ERROR("Failed to allocate memory for decoder");
        fclose(fp);
        return NULL;
    }

    if (fread(content, 1, (size_t)file_size, fp) != (size_t)file_size) {
        LOG_WARNING("Failed to read decoder file: %s", filepath);
        free(content);
        fclose(fp);
        return NULL;
    }

    content[file_size] = '\0';
    fclose(fp);

    return content;
}

/**
 * @brief Build decoder file path for a sensor type
 */
static bool build_decoder_path(
    char *path,
    size_t path_size,
    const char *base_path,
    const char *sensor)
{
    int ret;

    if (!path || !base_path || !sensor || path_size == 0) {
        return false;
    }

    /* Path format: {base_path}/{sensor}-decoder */
    ret = snprintf(path, path_size, "%s/%s-decoder", base_path, sensor);
    
    return (ret > 0 && (size_t)ret < path_size);
}

/**
 * @brief Find cache entry by sensor type
 */
static decoder_cache_entry_t *find_cache_entry(
    decoder_cache_t *cache,
    const char *sensor)
{
    size_t i;

    for (i = 0; i < cache->count; i++) {
        if (cache->entries[i].valid && 
            strcmp(cache->entries[i].sensor, sensor) == 0) {
            return &cache->entries[i];
        }
    }

    return NULL;
}

/**
 * @brief Initialize decoder cache
 */
bool decoder_cache_init(
    decoder_cache_t *cache,
    const char *base_path,
    const char *alt_path)
{
    if (!cache) {
        return false;
    }

    memset(cache, 0, sizeof(decoder_cache_t));

    if (base_path) {
        strncpy(cache->base_path, base_path, sizeof(cache->base_path) - 1);
    }

    if (alt_path) {
        strncpy(cache->alt_path, alt_path, sizeof(cache->alt_path) - 1);
    }

    LOG_INFO("Decoder cache initialized (base=%s, alt=%s)", 
            cache->base_path, cache->alt_path);

    return true;
}

/**
 * @brief Load a decoder for a sensor type
 */
static decoder_cache_entry_t *load_decoder(
    decoder_cache_t *cache,
    const char *sensor)
{
    char path[DECODER_PATH_MAX_LEN];
    char *source_code = NULL;
    time_t mtime = 0;
    js_context_t *js_ctx = NULL;
    decoder_cache_entry_t *entry = NULL;
    size_t slot;

    /* Try primary path first */
    if (!build_decoder_path(path, sizeof(path), cache->base_path, sensor)) {
        LOG_ERROR("Failed to build decoder path for %s", sensor);
        return NULL;
    }

    source_code = load_decoder_file(path, &mtime);

    /* Try alternate path if primary failed */
    if (!source_code && strlen(cache->alt_path) > 0) {
        if (!build_decoder_path(path, sizeof(path), cache->alt_path, sensor)) {
            return NULL;
        }
        source_code = load_decoder_file(path, &mtime);
    }

    if (!source_code) {
        LOG_DEBUG("No decoder found for sensor: %s", sensor);
        return NULL;
    }

    /* Compile the JavaScript */
    js_ctx = js_context_create(source_code);
    free(source_code);

    if (!js_ctx) {
        LOG_ERROR("Failed to compile decoder for %s", sensor);
        return NULL;
    }

    /* Find a slot in the cache */
    if (cache->count < DECODER_CACHE_MAX_ENTRIES) {
        slot = cache->count;
        cache->count++;
    } else {
        /* Find oldest entry to replace */
        time_t oldest = cache->entries[0].last_used;
        size_t i;
        slot = 0;
        for (i = 1; i < cache->count; i++) {
            if (cache->entries[i].last_used < oldest) {
                oldest = cache->entries[i].last_used;
                slot = i;
            }
        }
        /* Destroy old entry */
        if (cache->entries[slot].js_ctx) {
            js_context_destroy(cache->entries[slot].js_ctx);
        }
    }

    entry = &cache->entries[slot];
    memset(entry, 0, sizeof(decoder_cache_entry_t));
    strncpy(entry->sensor, sensor, sizeof(entry->sensor) - 1);
    strncpy(entry->filepath, path, sizeof(entry->filepath) - 1);
    entry->js_ctx = js_ctx;
    entry->file_mtime = mtime;
    entry->last_used = time(NULL);
    entry->valid = true;

    LOG_INFO("Loaded decoder for %s from %s", sensor, path);

    return entry;
}

/**
 * @brief Get or load a decoder for a sensor type
 */
js_context_t *decoder_cache_get(
    decoder_cache_t *cache,
    const char *sensor)
{
    decoder_cache_entry_t *entry = NULL;
    struct stat st;

    if (!cache || !sensor) {
        return NULL;
    }

    /* Look for existing entry */
    entry = find_cache_entry(cache, sensor);

    if (entry) {
        /* Check if file was modified */
        if (stat(entry->filepath, &st) == 0 && st.st_mtime > entry->file_mtime) {
            LOG_INFO("Decoder file changed, reloading: %s", sensor);
            if (entry->js_ctx) {
                js_context_destroy(entry->js_ctx);
                entry->js_ctx = NULL;
            }
            entry->valid = false;
            entry = NULL;
        } else {
            entry->last_used = time(NULL);
            return entry->js_ctx;
        }
    }

    /* Load the decoder */
    entry = load_decoder(cache, sensor);
    
    return entry ? entry->js_ctx : NULL;
}

/**
 * @brief Check if a decoder exists for a sensor type
 */
bool decoder_cache_exists(
    decoder_cache_t *cache,
    const char *sensor)
{
    char path[DECODER_PATH_MAX_LEN];
    struct stat st;

    if (!cache || !sensor) {
        return false;
    }

    /* Check primary path */
    if (build_decoder_path(path, sizeof(path), cache->base_path, sensor)) {
        if (stat(path, &st) == 0) {
            return true;
        }
    }

    /* Check alternate path */
    if (strlen(cache->alt_path) > 0) {
        if (build_decoder_path(path, sizeof(path), cache->alt_path, sensor)) {
            if (stat(path, &st) == 0) {
                return true;
            }
        }
    }

    return false;
}

/**
 * @brief Invalidate and reload a specific decoder
 */
bool decoder_cache_reload(
    decoder_cache_t *cache,
    const char *sensor)
{
    decoder_cache_entry_t *entry = NULL;

    if (!cache || !sensor) {
        return false;
    }

    entry = find_cache_entry(cache, sensor);
    if (entry) {
        if (entry->js_ctx) {
            js_context_destroy(entry->js_ctx);
            entry->js_ctx = NULL;
        }
        entry->valid = false;
    }

    entry = load_decoder(cache, sensor);
    return (entry != NULL);
}

/**
 * @brief Cleanup decoder cache
 */
void decoder_cache_cleanup(decoder_cache_t *cache)
{
    size_t i;

    if (!cache) {
        return;
    }

    for (i = 0; i < cache->count; i++) {
        if (cache->entries[i].js_ctx) {
            js_context_destroy(cache->entries[i].js_ctx);
            cache->entries[i].js_ctx = NULL;
        }
    }

    memset(cache, 0, sizeof(decoder_cache_t));
    LOG_INFO("Decoder cache cleaned up");
}

/**
 * @brief Get cache statistics
 */
void decoder_cache_stats(
    const decoder_cache_t *cache,
    size_t *count)
{
    size_t i;

    if (!cache || !count) {
        return;
    }

    *count = 0;
    for (i = 0; i < cache->count; i++) {
        if (cache->entries[i].valid) {
            (*count)++;
        }
    }
}
