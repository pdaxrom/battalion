/*
 * Copyright (c) 2026 Battalion contributors.
 */
/* Renderer-neutral faction identities and directional relationships. */

#include "faction_catalog.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FACTION_LINE_SIZE 256

static void setError(char *error, size_t errorSize, const char *message)
    {
    if (error == NULL || errorSize == 0)
        return;
    strncpy(error, message, errorSize - 1);
    error[errorSize - 1] = '\0';
    }

static int validId(const char *text)
    {
    const unsigned char *cursor;

    if (text == NULL || text[0] == '\0' ||
        strlen(text) >= FACTION_ID_SIZE)
        return 0;
    cursor = (const unsigned char *)text;
    while (*cursor != '\0')
        {
        if (!(islower(*cursor) || isdigit(*cursor) || *cursor == '-'))
            return 0;
        cursor++;
        }
    return 1;
    }

void factionCatalogInitialize(struct FactionCatalog *catalog)
    {
    if (catalog == NULL)
        return;
    catalog->entries = NULL;
    catalog->count = 0;
    catalog->playerFaction[0] = '\0';
    }

void factionCatalogDestroy(struct FactionCatalog *catalog)
    {
    size_t index;

    if (catalog == NULL)
        return;
    for (index = 0; index < catalog->count; index++)
        free(catalog->entries[index].relations);
    free(catalog->entries);
    factionCatalogInitialize(catalog);
    }

const struct FactionDefinition *factionCatalogFind(
    const struct FactionCatalog *catalog, const char *id)
    {
    size_t index;

    if (catalog == NULL || id == NULL)
        return NULL;
    for (index = 0; index < catalog->count; index++)
        if (strcmp(catalog->entries[index].id, id) == 0)
            return &catalog->entries[index];
    return NULL;
    }

static struct FactionDefinition *mutableFaction(
    struct FactionCatalog *catalog, const char *id)
    {
    return (struct FactionDefinition *)factionCatalogFind(catalog, id);
    }

static int relationValue(const char *text)
    {
    if (strcmp(text, "neutral") == 0)
        return FACTION_RELATION_NEUTRAL;
    if (strcmp(text, "ally") == 0)
        return FACTION_RELATION_ALLY;
    if (strcmp(text, "hostile") == 0)
        return FACTION_RELATION_HOSTILE;
    return -1;
    }

static int addFaction(struct FactionCatalog *catalog,
                      const char *id, const char *color)
    {
    struct FactionDefinition *entries;
    struct FactionDefinition *entry;

    if (!validId(id) || !validId(color) || factionCatalogFind(catalog, id))
        return 0;
    entries = (struct FactionDefinition *)realloc(
        catalog->entries, (catalog->count + 1) * sizeof(*entries));
    if (entries == NULL)
        return 0;
    catalog->entries = entries;
    entry = &entries[catalog->count++];
    memset(entry, 0, sizeof(*entry));
    strcpy(entry->id, id);
    strcpy(entry->editorColor, color);
    return 1;
    }

static int addRelation(struct FactionCatalog *catalog,
                       const char *sourceId, const char *targetId,
                       const char *relationText)
    {
    struct FactionDefinition *source;
    struct FactionRelation *relations;
    int relation;
    size_t index;

    source = mutableFaction(catalog, sourceId);
    relation = relationValue(relationText);
    if (source == NULL || factionCatalogFind(catalog, targetId) == NULL ||
        relation < 0)
        return 0;
    for (index = 0; index < source->relationCount; index++)
        if (strcmp(source->relations[index].target, targetId) == 0)
            return 0;
    relations = (struct FactionRelation *)realloc(
        source->relations,
        (source->relationCount + 1) * sizeof(*relations));
    if (relations == NULL)
        return 0;
    source->relations = relations;
    strcpy(relations[source->relationCount].target, targetId);
    relations[source->relationCount].relation = relation;
    source->relationCount++;
    return 1;
    }

static int buildPath(const char *root, const char *relative,
                     char *path, size_t pathSize)
    {
    size_t rootLength;

    if (root == NULL || relative == NULL || relative[0] == '/' ||
        strstr(relative, "..") != NULL)
        return 0;
    rootLength = strlen(root);
    if (rootLength + strlen(relative) + 2 > pathSize)
        return 0;
    strcpy(path, root);
    if (rootLength > 0 && path[rootLength - 1] != '/')
        strcat(path, "/");
    strcat(path, relative);
    return 1;
    }

int factionCatalogLoad(const char *assetRoot, const char *relativePath,
                       struct FactionCatalog *catalog,
                       char *error, size_t errorSize)
    {
    struct FactionCatalog loaded;
    FILE *file;
    char path[1024];
    char line[FACTION_LINE_SIZE];
    char first[FACTION_ID_SIZE];
    char second[FACTION_ID_SIZE];
    char third[FACTION_ID_SIZE];
    char fourth[FACTION_ID_SIZE];
    int fields;
    int sawHeader;

    if (catalog == NULL || !buildPath(assetRoot, relativePath,
                                      path, sizeof(path)))
        {
        setError(error, errorSize, "Invalid faction catalog path");
        return 0;
        }
    file = fopen(path, "r");
    if (file == NULL)
        {
        setError(error, errorSize, "Could not open faction catalog");
        return 0;
        }
    factionCatalogInitialize(&loaded);
    sawHeader = 0;
    while (fgets(line, sizeof(line), file) != NULL)
        {
        first[0] = second[0] = third[0] = fourth[0] = '\0';
        fields = sscanf(line, "%63s %63s %63s %63s",
                        first, second, third, fourth);
        if (fields <= 0 || first[0] == '#')
            continue;
        if (!sawHeader)
            {
            if (fields == 2 && strcmp(first, "battalion-factions") == 0 &&
                strcmp(second, "1") == 0)
                {
                sawHeader = 1;
                continue;
                }
            fclose(file);
            factionCatalogDestroy(&loaded);
            setError(error, errorSize, "Invalid faction catalog header");
            return 0;
            }
        if (strcmp(first, "faction") == 0 && fields == 3 &&
            addFaction(&loaded, second, third))
            continue;
        if (strcmp(first, "player-faction") == 0 && fields == 2 &&
            loaded.playerFaction[0] == '\0' && validId(second))
            {
            strcpy(loaded.playerFaction, second);
            continue;
            }
        if (strcmp(first, "relation") == 0 && fields == 4 &&
            addRelation(&loaded, second, third, fourth))
            continue;
        fclose(file);
        factionCatalogDestroy(&loaded);
        setError(error, errorSize, "Invalid faction catalog directive");
        return 0;
        }
    fclose(file);
    if (!sawHeader || loaded.count == 0 ||
        loaded.playerFaction[0] == '\0' ||
        factionCatalogFind(&loaded, loaded.playerFaction) == NULL)
        {
        factionCatalogDestroy(&loaded);
        setError(error, errorSize, "Faction catalog has no player faction");
        return 0;
        }
    factionCatalogDestroy(catalog);
    *catalog = loaded;
    return 1;
    }

int factionCatalogRelation(const struct FactionCatalog *catalog,
                           const char *source, const char *target)
    {
    const struct FactionDefinition *definition;
    size_t index;

    if (catalog == NULL || source == NULL || target == NULL)
        return FACTION_RELATION_NEUTRAL;
    if (strcmp(source, target) == 0)
        return FACTION_RELATION_ALLY;
    definition = factionCatalogFind(catalog, source);
    if (definition == NULL || factionCatalogFind(catalog, target) == NULL)
        return FACTION_RELATION_NEUTRAL;
    for (index = 0; index < definition->relationCount; index++)
        if (strcmp(definition->relations[index].target, target) == 0)
            return definition->relations[index].relation;
    return FACTION_RELATION_NEUTRAL;
    }

int factionCatalogHostile(const struct FactionCatalog *catalog,
                          const char *source, const char *target)
    {
    return factionCatalogRelation(catalog, source, target) ==
           FACTION_RELATION_HOSTILE;
    }
