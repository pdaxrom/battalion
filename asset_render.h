/*
 * Copyright (c) 2026 Battalion contributors.
 */

#ifndef BATTALION_ASSET_RENDER_H
#define BATTALION_ASSET_RENDER_H

#include <stddef.h>

struct AssetModel;
struct AssetPose;

void assetModelDraw(const struct AssetModel *model, const struct AssetPose *pose);
void assetModelDrawNode(const struct AssetModel *model, int node,
                        const struct AssetPose *pose);
unsigned int assetModelCreateDisplayList(const struct AssetModel *model);
unsigned int assetLoadGLBDisplayList(const char *path,
                                     char *error, size_t errorSize);

#endif
