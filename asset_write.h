/*
 * Copyright (c) 2026 Battalion contributors.
 */

#ifndef BATTALION_ASSET_WRITE_H
#define BATTALION_ASSET_WRITE_H

#include <stddef.h>

struct AssetModel;

int assetModelWriteGLB(const char *path, const struct AssetModel *model,
                       char *error, size_t errorSize);
int assetModelWriteStaticGLB(const char *path, const struct AssetModel *model,
                             char *error, size_t errorSize);

#endif
