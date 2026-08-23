/*
 * Copyright (c) 2026 Battalion contributors.
 */
/* External audio catalog shared by the native audio backends.         */
/***********************************************************************/

#include "audio_catalog.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void setAudioError(char *error, size_t errorSize,
                          const char *message, int line)
    {
    char suffix[32];

    if (error == NULL || errorSize == 0)
        return;
    strncpy(error, message, errorSize - 1);
    error[errorSize - 1] = '\0';
    if (line > 0)
        {
        sprintf(suffix, " at line %d", line);
        strncat(error, suffix, errorSize - strlen(error) - 1);
        }
    }

static char *copyAudioString(const char *text)
    {
    char *copy;
    size_t length;

    length = strlen(text);
    copy = (char *)malloc(length + 1);
    if (copy != NULL)
        memcpy(copy, text, length + 1);
    return copy;
    }

static int validAudioName(const char *text, int path)
    {
    const unsigned char *cursor;

    if (text == NULL || text[0] == '\0' || text[0] == '/' ||
        (path && strstr(text, "..") != NULL))
        return 0;
    for (cursor = (const unsigned char *)text; *cursor; cursor++)
        if (isupper(*cursor) || *cursor == '\\' ||
            !(islower(*cursor) || isdigit(*cursor) || *cursor == '-' ||
              *cursor == '_' || *cursor == '.' || (path && *cursor == '/')))
            return 0;
    return 1;
    }

void audioCatalogInitialize(struct AudioCatalog *catalog)
    {
    catalog->entries = NULL;
    catalog->count = 0;
    }

void audioCatalogDestroy(struct AudioCatalog *catalog)
    {
    size_t index;

    if (catalog == NULL)
        return;
    for (index = 0; index < catalog->count; index++)
        {
        free(catalog->entries[index].key);
        free(catalog->entries[index].backend);
        free(catalog->entries[index].path);
        }
    free(catalog->entries);
    audioCatalogInitialize(catalog);
    }

const struct AudioCatalogEntry *audioCatalogFind(
    const struct AudioCatalog *catalog, const char *key,
    const char *backend)
    {
    size_t index;

    for (index = 0; index < catalog->count; index++)
        if (strcmp(catalog->entries[index].key, key) == 0 &&
            strcmp(catalog->entries[index].backend, backend) == 0)
            return &catalog->entries[index];
    return NULL;
    }

int audioCatalogBuildPath(const char *dataPath,
                          const struct AudioCatalogEntry *entry,
                          char *path, size_t pathSize)
    {
    size_t dataLength;
    size_t entryLength;

    if (dataPath == NULL || entry == NULL || path == NULL)
        return 0;
    dataLength = strlen(dataPath);
    entryLength = strlen(entry->path);
    if (dataLength + entryLength + 1 > pathSize)
        return 0;
    memcpy(path, dataPath, dataLength);
    memcpy(path + dataLength, entry->path, entryLength + 1);
    return 1;
    }

static int appendAudioEntry(struct AudioCatalog *catalog,
                            const char *key, const char *backend,
                            int slot, const char *path, int unique)
    {
    struct AudioCatalogEntry *expanded;
    struct AudioCatalogEntry *entry;

    expanded = (struct AudioCatalogEntry *)realloc(
        catalog->entries, (catalog->count + 1) * sizeof(*expanded));
    if (expanded == NULL)
        return 0;
    catalog->entries = expanded;
    entry = &catalog->entries[catalog->count];
    entry->key = copyAudioString(key);
    entry->backend = copyAudioString(backend);
    entry->path = copyAudioString(path);
    entry->slot = slot;
    entry->unique = unique;
    if (entry->key == NULL || entry->backend == NULL || entry->path == NULL)
        {
        free(entry->key);
        free(entry->backend);
        free(entry->path);
        return 0;
        }
    catalog->count++;
    return 1;
    }

int audioCatalogLoad(const char *dataPath, const char *catalogPath,
                     struct AudioCatalog *catalog,
                     char *error, size_t errorSize)
    {
    char lineText[512];
    char key[64];
    char backend[32];
    char path[256];
    char mode[32];
    char fullPath[512];
    char extra;
    FILE *file;
    FILE *asset;
    int line;
    int header;
    int version;
    int slot;
    int fields;
    int unique;

    if (dataPath == NULL || catalogPath == NULL || catalog == NULL)
        return 0;
    audioCatalogDestroy(catalog);
    file = fopen(catalogPath, "r");
    if (file == NULL)
        {
        setAudioError(error, errorSize, "Could not open audio catalog", 0);
        return 0;
        }
    line = 0;
    header = 0;
    while (fgets(lineText, sizeof(lineText), file) != NULL)
        {
        line++;
        if (sscanf(lineText, " %63s", key) != 1 || key[0] == '#')
            continue;
        if (!header)
            {
            if (sscanf(lineText, " battalion-audio %d %c",
                       &version, &extra) != 1 || version != 1)
                {
                setAudioError(error, errorSize,
                              "Invalid audio catalog header", line);
                goto failed;
                }
            header = 1;
            continue;
            }
        fields = sscanf(lineText, " entry %63s %31s %d %255s %31s %c",
                        key, backend, &slot, path, mode, &extra);
        unique = fields == 5 && strcmp(mode, "unique") == 0;
        if ((fields != 4 && fields != 5) ||
            (fields == 5 && strcmp(mode, "unique") != 0 &&
             strcmp(mode, "overlap") != 0) ||
            !validAudioName(key, 0) || !validAudioName(backend, 0) ||
            !validAudioName(path, 1) || slot < 0 || slot > 255 ||
            audioCatalogFind(catalog, key, backend) != NULL ||
            !appendAudioEntry(catalog, key, backend, slot, path, unique))
            {
            setAudioError(error, errorSize,
                          "Invalid or duplicate audio entry", line);
            goto failed;
            }
        if (!audioCatalogBuildPath(dataPath,
                                   &catalog->entries[catalog->count - 1],
                                   fullPath, sizeof(fullPath)))
            {
            setAudioError(error, errorSize, "Audio asset path is too long", line);
            goto failed;
            }
        asset = fopen(fullPath, "rb");
        if (asset == NULL)
            {
            setAudioError(error, errorSize, "Audio asset is missing", line);
            goto failed;
            }
        fclose(asset);
        }
    fclose(file);
    if (!header || catalog->count == 0)
        {
        setAudioError(error, errorSize, "Incomplete audio catalog", line);
        audioCatalogDestroy(catalog);
        return 0;
        }
    return 1;

failed:
    fclose(file);
    audioCatalogDestroy(catalog);
    return 0;
    }
