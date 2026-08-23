/*
 * Copyright (c) 2026 Battalion contributors.
 */

#ifndef BATTALION_STROKE_FONT_ASSET_H
#define BATTALION_STROKE_FONT_ASSET_H

#include <stddef.h>

#define STROKE_COMMAND_BEGIN   1
#define STROKE_COMMAND_NEXT    2
#define STROKE_COMMAND_END     3
#define STROKE_COMMAND_ADVANCE 4

struct StrokeFontCommand
    {
    int type;
    float x;
    float y;
    };

struct StrokeFontGlyph
    {
    int code;
    struct StrokeFontCommand *commands;
    size_t commandCount;
    };

struct StrokeFontAsset
    {
    float scale;
    struct StrokeFontGlyph *glyphs;
    size_t glyphCount;
    };

void strokeFontAssetInitialize(struct StrokeFontAsset *font);
void strokeFontAssetDestroy(struct StrokeFontAsset *font);
int strokeFontAssetLoad(const char *path, struct StrokeFontAsset *font,
                        char *error, size_t errorSize);

#endif
