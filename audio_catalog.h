/*
 * Copyright (c) 2026 Battalion contributors.
 */

#ifndef BATTALION_AUDIO_CATALOG_H
#define BATTALION_AUDIO_CATALOG_H

#include <stddef.h>

struct AudioCatalogEntry
    {
    char *key;
    char *backend;
    char *path;
    int slot;
    int unique;
    };

struct AudioCatalog
    {
    struct AudioCatalogEntry *entries;
    size_t count;
    };

void audioCatalogInitialize(struct AudioCatalog *catalog);
void audioCatalogDestroy(struct AudioCatalog *catalog);
int audioCatalogLoad(const char *dataPath, const char *catalogPath,
                     struct AudioCatalog *catalog,
                     char *error, size_t errorSize);
const struct AudioCatalogEntry *audioCatalogFind(
    const struct AudioCatalog *catalog, const char *key,
    const char *backend);
int audioCatalogBuildPath(const char *dataPath,
                          const struct AudioCatalogEntry *entry,
                          char *path, size_t pathSize);

#endif
