/*
 * Copyright (c) 2026 Battalion contributors.
 */

#ifndef BATTALION_ASSET_TGA_H
#define BATTALION_ASSET_TGA_H

#include <stddef.h>

struct AssetImage
    {
    unsigned char *pixels;
    int width;
    int height;
    };

void assetImageInitialize(struct AssetImage *image);
void assetImageDestroy(struct AssetImage *image);
int assetImageLoadTGA(const char *path, struct AssetImage *image,
                      char *error, size_t errorSize);
int assetImageMakePowerOfTwo(struct AssetImage *image,
                             char *error, size_t errorSize);

#endif
