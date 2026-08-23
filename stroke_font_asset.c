/*
 * Copyright (c) 2026 Battalion contributors.
 */
/* Loader for external renderer-neutral stroke-font geometry.          */
/***********************************************************************/

#include "stroke_font_asset.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void setFontError(char *error, size_t errorSize, const char *message,
                         int line)
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

void strokeFontAssetInitialize(struct StrokeFontAsset *font)
    {
    font->scale = 0.0f;
    font->glyphs = NULL;
    font->glyphCount = 0;
    }

void strokeFontAssetDestroy(struct StrokeFontAsset *font)
    {
    size_t glyph;

    if (font == NULL)
        return;
    for (glyph = 0; glyph < font->glyphCount; glyph++)
        free(font->glyphs[glyph].commands);
    free(font->glyphs);
    strokeFontAssetInitialize(font);
    }

static int glyphCodeExists(const struct StrokeFontAsset *font, int code)
    {
    size_t glyph;

    for (glyph = 0; glyph < font->glyphCount; glyph++)
        if (font->glyphs[glyph].code == code)
            return 1;
    return 0;
    }

static int appendGlyph(struct StrokeFontAsset *font, int code)
    {
    struct StrokeFontGlyph *expanded;
    struct StrokeFontGlyph *glyph;

    expanded = (struct StrokeFontGlyph *)realloc(
        font->glyphs, (font->glyphCount + 1) * sizeof(*expanded));
    if (expanded == NULL)
        return 0;
    font->glyphs = expanded;
    glyph = &font->glyphs[font->glyphCount++];
    glyph->code = code;
    glyph->commands = NULL;
    glyph->commandCount = 0;
    return 1;
    }

static int appendCommand(struct StrokeFontGlyph *glyph, int type,
                         float x, float y)
    {
    struct StrokeFontCommand *expanded;
    struct StrokeFontCommand *command;

    expanded = (struct StrokeFontCommand *)realloc(
        glyph->commands, (glyph->commandCount + 1) * sizeof(*expanded));
    if (expanded == NULL)
        return 0;
    glyph->commands = expanded;
    command = &glyph->commands[glyph->commandCount++];
    command->type = type;
    command->x = x;
    command->y = y;
    return 1;
    }

static int commandType(const char *directive)
    {
    if (strcmp(directive, "begin") == 0)
        return STROKE_COMMAND_BEGIN;
    if (strcmp(directive, "next") == 0)
        return STROKE_COMMAND_NEXT;
    if (strcmp(directive, "end") == 0)
        return STROKE_COMMAND_END;
    if (strcmp(directive, "advance") == 0)
        return STROKE_COMMAND_ADVANCE;
    return 0;
    }

int strokeFontAssetLoad(const char *path, struct StrokeFontAsset *font,
                        char *error, size_t errorSize)
    {
    char lineText[256];
    char directive[32];
    char extra;
    FILE *file;
    struct StrokeFontGlyph *glyph;
    float x;
    float y;
    int version;
    int code;
    int type;
    int line;
    int header;
    int scale;
    int current;
    int drawing;
    int advance;

    if (path == NULL || font == NULL)
        return 0;
    strokeFontAssetDestroy(font);
    file = fopen(path, "r");
    if (file == NULL)
        {
        setFontError(error, errorSize, "Could not open stroke font", 0);
        return 0;
        }
    line = 0;
    header = 0;
    scale = 0;
    current = -1;
    drawing = 0;
    advance = 0;
    while (fgets(lineText, sizeof(lineText), file) != NULL)
        {
        line++;
        if (sscanf(lineText, " %31s", directive) != 1 ||
            directive[0] == '#')
            continue;
        if (!header)
            {
            if (sscanf(lineText, " battalion-stroke-font %d %c",
                       &version, &extra) != 1 || version != 1)
                {
                setFontError(error, errorSize,
                             "Invalid stroke-font header", line);
                goto failed;
                }
            header = 1;
            continue;
            }
        if (strcmp(directive, "scale") == 0)
            {
            if (scale || current >= 0 ||
                sscanf(lineText, " scale %f %c", &font->scale, &extra) != 1 ||
                font->scale <= 0.0f)
                {
                setFontError(error, errorSize,
                             "Invalid stroke-font scale", line);
                goto failed;
                }
            scale = 1;
            continue;
            }
        if (strcmp(directive, "glyph") == 0)
            {
            if (!scale || current >= 0 ||
                sscanf(lineText, " glyph %d %c", &code, &extra) != 1 ||
                code < 0 || code > 255 || glyphCodeExists(font, code) ||
                !appendGlyph(font, code))
                {
                setFontError(error, errorSize,
                             "Invalid or duplicate stroke glyph", line);
                goto failed;
                }
            current = (int)font->glyphCount - 1;
            drawing = 0;
            advance = 0;
            continue;
            }
        if (strcmp(directive, "end") == 0 &&
            sscanf(lineText, " end %f %f %c", &x, &y, &extra) != 2)
            {
            if (current < 0 || drawing || !advance ||
                sscanf(lineText, " end %c", &extra) == 1)
                {
                setFontError(error, errorSize,
                             "Invalid end of stroke glyph", line);
                goto failed;
                }
            current = -1;
            continue;
            }
        type = commandType(directive);
        if (current < 0 || type == 0 ||
            sscanf(lineText, " %31s %f %f %c", directive, &x, &y,
                   &extra) != 3 ||
            (type == STROKE_COMMAND_BEGIN && drawing) ||
            (type == STROKE_COMMAND_NEXT && !drawing) ||
            (type == STROKE_COMMAND_END && !drawing) ||
            (type == STROKE_COMMAND_ADVANCE && (drawing || advance)))
            {
            setFontError(error, errorSize,
                         "Invalid stroke-font command", line);
            goto failed;
            }
        glyph = &font->glyphs[current];
        if (!appendCommand(glyph, type, x, y))
            {
            setFontError(error, errorSize,
                         "Out of memory loading stroke font", line);
            goto failed;
            }
        if (type == STROKE_COMMAND_BEGIN)
            drawing = 1;
        else if (type == STROKE_COMMAND_END)
            drawing = 0;
        else if (type == STROKE_COMMAND_ADVANCE)
            advance = 1;
        }
    fclose(file);
    if (!header || !scale || current >= 0 || font->glyphCount == 0)
        {
        setFontError(error, errorSize, "Incomplete stroke-font asset", line);
        strokeFontAssetDestroy(font);
        return 0;
        }
    return 1;

failed:
    fclose(file);
    strokeFontAssetDestroy(font);
    return 0;
    }
