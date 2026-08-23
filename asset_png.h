/*
 * Copyright (c) 2026 Battalion contributors.
 */

#ifndef BATTALION_ASSET_PNG_H
#define BATTALION_ASSET_PNG_H

#include <stddef.h>

struct AssetImage;

int assetImageDecodePNG(const unsigned char *data, size_t length,
                        struct AssetImage *image,
                        char *error, size_t errorSize);

#endif
