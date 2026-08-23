/*
 * Copyright (c) 2026 Battalion contributors.
 */

#ifndef BATTALION_ASSET_BINDINGS_H
#define BATTALION_ASSET_BINDINGS_H

#include <stddef.h>

#define ASSET_BINDING_MODEL 1
#define ASSET_BINDING_TEXTURE 2
#define ASSET_BINDING_ANIMATION 3
#define ASSET_BINDING_FILE 4

struct AssetBinding
    {
    int type;
    char *slot;
    char *assetKey;
    };

struct AssetBindings
    {
    struct AssetBinding *entries;
    size_t count;
    };

void assetBindingsInitialize(struct AssetBindings *bindings);
void assetBindingsDestroy(struct AssetBindings *bindings);
int assetBindingsLoad(const char *path, struct AssetBindings *bindings,
                      char *error, size_t errorSize);
const char *assetBindingsFind(const struct AssetBindings *bindings,
                              int type, const char *slot);

#endif
