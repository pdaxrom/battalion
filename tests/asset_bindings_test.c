/*
 * Copyright (c) 2026 Battalion contributors.
 */

#include "asset_bindings.h"

#include <stdio.h>
#include <string.h>

static const char *typeName(int type)
    {
    if (type == ASSET_BINDING_MODEL)
        return "model";
    if (type == ASSET_BINDING_TEXTURE)
        return "texture";
    if (type == ASSET_BINDING_ANIMATION)
        return "animation";
    if (type == ASSET_BINDING_FILE)
        return "file";
    return "";
    }

static int manifestContains(int type, const char *key)
    {
    char line[1024];
    char directive[32];
    char manifestKey[128];
    FILE *file;

    file = fopen("assets/manifest.txt", "r");
    if (file == NULL)
        return 0;
    while (fgets(line, sizeof(line), file) != NULL)
        if (sscanf(line, "%31s %127s", directive, manifestKey) == 2 &&
            strcmp(directive, typeName(type)) == 0 &&
            strcmp(manifestKey, key) == 0)
            {
            fclose(file);
            return 1;
            }
    fclose(file);
    return 0;
    }

int main(void)
    {
    struct AssetBindings bindings;
    char error[256];
    size_t index;
    int valid;

    assetBindingsInitialize(&bindings);
    if (!assetBindingsLoad("assets/config/render.bindings", &bindings,
                           error, sizeof(error)))
        {
        fprintf(stderr, "%s\n", error);
        return 1;
        }
    valid = 1;
    for (index = 0; index < bindings.count; index++)
        if (!manifestContains(bindings.entries[index].type,
                              bindings.entries[index].assetKey))
            {
            fprintf(stderr, "binding target is absent from manifest: %s\n",
                    bindings.entries[index].assetKey);
            valid = 0;
            }
    if (assetBindingsFind(&bindings, ASSET_BINDING_MODEL,
                          "ui-title-high") == NULL ||
        assetBindingsFind(&bindings, ASSET_BINDING_FILE,
                          "game-entities") == NULL ||
        assetBindingsFind(&bindings, ASSET_BINDING_FILE,
                          "help-sgi-showcase") == NULL)
        valid = 0;
    assetBindingsDestroy(&bindings);
    return valid ? 0 : 1;
    }
