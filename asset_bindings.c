/****************************************************************/
/* code copyright 1995-1996 Andrew Johnson - ALL RIGHTS RESERVED*/
/*                          ajohnson@eecs.uic.edu               */
/*                                                              */
/*                      Electronic Visualization Lab (M/C 154)  */
/*                      University of Illinois at Chicago       */
/*                      851 South Morgan St. Room 1120 SEO      */
/*                      Chicago, IL 60607-7053                  */
/*                                                              */
/*                      (312) 996-3002     (312) 413-7585 FAX   */
/***********************************************************************/
/* Maps stable engine render slots to replaceable manifest entries.     */
/***********************************************************************/

#include "asset_bindings.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void setError(char *error, size_t errorSize, const char *message)
    {
    if (error == NULL || errorSize == 0)
        return;
    strncpy(error, message, errorSize - 1);
    error[errorSize - 1] = '\0';
    }

static int validName(const char *name)
    {
    const unsigned char *cursor;

    if (name == NULL || name[0] == '\0')
        return 0;
    for (cursor = (const unsigned char *)name; *cursor; cursor++)
        if (!(islower(*cursor) || isdigit(*cursor) || *cursor == '-'))
            return 0;
    return 1;
    }

static char *copyString(const char *text)
    {
    char *copy;
    size_t length;

    length = strlen(text);
    copy = (char *)malloc(length + 1);
    if (copy != NULL)
        memcpy(copy, text, length + 1);
    return copy;
    }

static int bindingType(const char *name)
    {
    if (strcmp(name, "model") == 0)
        return ASSET_BINDING_MODEL;
    if (strcmp(name, "texture") == 0)
        return ASSET_BINDING_TEXTURE;
    if (strcmp(name, "animation") == 0)
        return ASSET_BINDING_ANIMATION;
    if (strcmp(name, "file") == 0)
        return ASSET_BINDING_FILE;
    return 0;
    }

void assetBindingsInitialize(struct AssetBindings *bindings)
    {
    bindings->entries = NULL;
    bindings->count = 0;
    }

void assetBindingsDestroy(struct AssetBindings *bindings)
    {
    size_t index;

    if (bindings == NULL)
        return;
    for (index = 0; index < bindings->count; index++)
        {
        free(bindings->entries[index].slot);
        free(bindings->entries[index].assetKey);
        }
    free(bindings->entries);
    assetBindingsInitialize(bindings);
    }

const char *assetBindingsFind(const struct AssetBindings *bindings,
                              int type, const char *slot)
    {
    size_t index;

    if (bindings == NULL || slot == NULL)
        return NULL;
    for (index = 0; index < bindings->count; index++)
        if (bindings->entries[index].type == type &&
            strcmp(bindings->entries[index].slot, slot) == 0)
            return bindings->entries[index].assetKey;
    return NULL;
    }

static int appendBinding(struct AssetBindings *bindings, int type,
                         const char *slot, const char *assetKey,
                         char *error, size_t errorSize)
    {
    struct AssetBinding *expanded;
    struct AssetBinding *entry;

    if (type == 0 || !validName(slot) || !validName(assetKey))
        {
        setError(error, errorSize, "Invalid lowercase render binding");
        return 0;
        }
    if (assetBindingsFind(bindings, type, slot) != NULL)
        {
        setError(error, errorSize, "Duplicate render binding slot");
        return 0;
        }
    expanded = (struct AssetBinding *)realloc(
        bindings->entries, (bindings->count + 1) * sizeof(*expanded));
    if (expanded == NULL)
        {
        setError(error, errorSize, "Out of memory adding render binding");
        return 0;
        }
    bindings->entries = expanded;
    entry = &bindings->entries[bindings->count];
    entry->type = type;
    entry->slot = copyString(slot);
    entry->assetKey = copyString(assetKey);
    if (entry->slot == NULL || entry->assetKey == NULL)
        {
        free(entry->slot);
        free(entry->assetKey);
        setError(error, errorSize, "Out of memory copying render binding");
        return 0;
        }
    bindings->count++;
    return 1;
    }

int assetBindingsLoad(const char *path, struct AssetBindings *bindings,
                      char *error, size_t errorSize)
    {
    char line[512];
    char directive[32];
    char slot[128];
    char assetKey[128];
    char extra[2];
    FILE *file;
    int header;
    int fields;

    if (path == NULL || bindings == NULL)
        {
        setError(error, errorSize, "Render binding path is required");
        return 0;
        }
    assetBindingsDestroy(bindings);
    file = fopen(path, "r");
    if (file == NULL)
        {
        setError(error, errorSize, "Could not open render bindings");
        return 0;
        }
    header = 0;
    while (fgets(line, sizeof(line), file) != NULL)
        {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        if (!header)
            {
            fields = sscanf(line, "%31s %127s %1s", directive, slot, extra);
            if (fields != 2 || strcmp(directive, "battalion-bindings") != 0 ||
                strcmp(slot, "1") != 0)
                {
                setError(error, errorSize,
                         "Render bindings require battalion-bindings 1");
                goto failed;
                }
            header = 1;
            continue;
            }
        fields = sscanf(line, "%31s %127s %127s %1s", directive, slot,
                        assetKey, extra);
        if (fields != 3 ||
            !appendBinding(bindings, bindingType(directive), slot, assetKey,
                           error, errorSize))
            goto failed;
        }
    fclose(file);
    if (!header || bindings->count == 0)
        {
        setError(error, errorSize, "Render bindings are empty");
        assetBindingsDestroy(bindings);
        return 0;
        }
    return 1;

failed:
    fclose(file);
    assetBindingsDestroy(bindings);
    return 0;
    }
