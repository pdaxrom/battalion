/*
 * Copyright (c) 2026 Battalion contributors.
 */

#ifndef BATTALION_ASSET_VALUES_H
#define BATTALION_ASSET_VALUES_H

#include <stddef.h>

#define ASSET_VALUE_COMPONENTS 8

struct AssetValue
    {
    char *key;
    float components[ASSET_VALUE_COMPONENTS];
    size_t componentCount;
    };

struct AssetValues
    {
    struct AssetValue *values;
    size_t count;
    };

void assetValuesInitialize(struct AssetValues *values);
void assetValuesDestroy(struct AssetValues *values);
int assetValuesLoad(const char *path, struct AssetValues *values,
                    char *error, size_t errorSize);
const struct AssetValue *assetValuesFind(const struct AssetValues *values,
                                         const char *key);

#endif
