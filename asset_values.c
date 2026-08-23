/*
 * Copyright (c) 2026 Battalion contributors.
 */
/* Small renderer-neutral numeric configuration asset loader.           */
/***********************************************************************/

#include "asset_values.h"

#include <ctype.h>
#include <errno.h>
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

static void setLineError(char *error, size_t errorSize, const char *message,
                         int line)
    {
    char number[32];

    setError(error, errorSize, message);
    if (error == NULL || errorSize == 0)
        return;
    sprintf(number, " at line %d", line);
    strncat(error, number, errorSize - strlen(error) - 1);
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

static int validKey(const char *key)
    {
    const unsigned char *cursor;

    if (key == NULL || key[0] == '\0')
        return 0;
    for (cursor = (const unsigned char *)key; *cursor; cursor++)
        if (!(islower(*cursor) || isdigit(*cursor) || *cursor == '-' ||
              *cursor == '_'))
            return 0;
    return 1;
    }

void assetValuesInitialize(struct AssetValues *values)
    {
    values->values = NULL;
    values->count = 0;
    }

void assetValuesDestroy(struct AssetValues *values)
    {
    size_t index;

    if (values == NULL)
        return;
    for (index = 0; index < values->count; index++)
        free(values->values[index].key);
    free(values->values);
    assetValuesInitialize(values);
    }

const struct AssetValue *assetValuesFind(const struct AssetValues *values,
                                         const char *key)
    {
    size_t index;

    for (index = 0; index < values->count; index++)
        if (strcmp(values->values[index].key, key) == 0)
            return &values->values[index];
    return NULL;
    }

static int appendValue(struct AssetValues *values, const char *key,
                       char **components, size_t componentCount,
                       char *error, size_t errorSize, int line)
    {
    struct AssetValue *expanded;
    struct AssetValue *entry;
    char *end;
    double parsed;
    size_t index;

    if (!validKey(key) || componentCount == 0 ||
        componentCount > ASSET_VALUE_COMPONENTS ||
        assetValuesFind(values, key) != NULL)
        {
        setLineError(error, errorSize,
                     "Invalid or duplicate numeric asset key", line);
        return 0;
        }
    expanded = (struct AssetValue *)realloc(
        values->values, (values->count + 1) * sizeof(*expanded));
    if (expanded == NULL)
        {
        setError(error, errorSize, "Out of memory loading numeric assets");
        return 0;
        }
    values->values = expanded;
    entry = &values->values[values->count];
    memset(entry, 0, sizeof(*entry));
    entry->key = copyString(key);
    if (entry->key == NULL)
        {
        setError(error, errorSize, "Out of memory copying numeric asset key");
        return 0;
        }
    for (index = 0; index < componentCount; index++)
        {
        errno = 0;
        parsed = strtod(components[index], &end);
        if (errno != 0 || end == components[index] || *end != '\0')
            {
            free(entry->key);
            entry->key = NULL;
            setLineError(error, errorSize,
                         "Invalid numeric asset component", line);
            return 0;
            }
        entry->components[index] = (float)parsed;
        }
    entry->componentCount = componentCount;
    values->count++;
    return 1;
    }

int assetValuesLoad(const char *path, struct AssetValues *values,
                    char *error, size_t errorSize)
    {
    char lineText[1024];
    char *directive;
    char *key;
    char *component;
    char *components[ASSET_VALUE_COMPONENTS + 1];
    FILE *file;
    size_t componentCount;
    int header;
    int line;

    if (path == NULL || values == NULL)
        return 0;
    assetValuesDestroy(values);
    file = fopen(path, "r");
    if (file == NULL)
        {
        setError(error, errorSize, "Could not open numeric asset file");
        return 0;
        }
    header = 0;
    line = 0;
    while (fgets(lineText, sizeof(lineText), file) != NULL)
        {
        line++;
        directive = strtok(lineText, " \t\r\n");
        if (directive == NULL || directive[0] == '#')
            continue;
        if (!header)
            {
            key = strtok(NULL, " \t\r\n");
            if (strcmp(directive, "battalion-values") != 0 || key == NULL ||
                strcmp(key, "1") != 0 || strtok(NULL, " \t\r\n") != NULL)
                {
                setLineError(error, errorSize,
                             "Numeric asset header must be battalion-values 1",
                             line);
                goto failed;
                }
            header = 1;
            continue;
            }
        if (strcmp(directive, "value") != 0)
            {
            setLineError(error, errorSize,
                         "Unknown numeric asset directive", line);
            goto failed;
            }
        key = strtok(NULL, " \t\r\n");
        componentCount = 0;
        while ((component = strtok(NULL, " \t\r\n")) != NULL)
            {
            if (componentCount >= ASSET_VALUE_COMPONENTS)
                {
                setLineError(error, errorSize,
                             "Too many numeric asset components", line);
                goto failed;
                }
            components[componentCount++] = component;
            }
        if (!appendValue(values, key, components, componentCount,
                         error, errorSize, line))
            goto failed;
        }
    fclose(file);
    if (!header)
        {
        setError(error, errorSize, "Numeric asset file is empty");
        assetValuesDestroy(values);
        return 0;
        }
    return 1;

failed:
    fclose(file);
    assetValuesDestroy(values);
    return 0;
    }
