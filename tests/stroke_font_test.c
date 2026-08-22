#include "stroke_font_asset.h"

#include <math.h>
#include <stdio.h>

static const struct StrokeFontGlyph *findGlyph(
    const struct StrokeFontAsset *font, int code)
    {
    size_t glyph;

    for (glyph = 0; glyph < font->glyphCount; glyph++)
        if (font->glyphs[glyph].code == code)
            return &font->glyphs[glyph];
    return NULL;
    }

static int hasAdvance(const struct StrokeFontGlyph *glyph)
    {
    size_t command;

    for (command = 0; command < glyph->commandCount; command++)
        if (glyph->commands[command].type == STROKE_COMMAND_ADVANCE)
            return 1;
    return 0;
    }

int main(void)
    {
    struct StrokeFontAsset font;
    const struct StrokeFontGlyph *space;
    const struct StrokeFontGlyph *upperA;
    const struct StrokeFontGlyph *lowerA;
    char error[160];
    size_t glyph;

    strokeFontAssetInitialize(&font);
    if (!strokeFontAssetLoad("assets/fonts/stroke.font", &font,
                             error, sizeof(error)))
        {
        fprintf(stderr, "stroke font failed to load: %s\n", error);
        return 1;
        }
    space = findGlyph(&font, 32);
    upperA = findGlyph(&font, 65);
    lowerA = findGlyph(&font, 97);
    if (font.glyphCount != 124 || fabs(font.scale - 0.045f) > 0.0001f ||
        space == NULL || upperA == NULL || lowerA == NULL ||
        upperA->commandCount == 0 || lowerA->commandCount == 0)
        {
        fprintf(stderr, "unexpected external stroke-font contents\n");
        strokeFontAssetDestroy(&font);
        return 1;
        }
    for (glyph = 0; glyph < font.glyphCount; glyph++)
        if (!hasAdvance(&font.glyphs[glyph]))
            {
            fprintf(stderr, "glyph %d has no advance command\n",
                    font.glyphs[glyph].code);
            strokeFontAssetDestroy(&font);
            return 1;
            }
    strokeFontAssetDestroy(&font);
    return 0;
    }
