/*
 * Copyright (c) 2026 Battalion contributors.
 */
/* Renderer-neutral faction identities and directional relationships. */

#ifndef BATTALION_FACTION_CATALOG_H
#define BATTALION_FACTION_CATALOG_H

#include <stddef.h>

#define FACTION_ID_SIZE 64

#define FACTION_RELATION_NEUTRAL 0
#define FACTION_RELATION_ALLY 1
#define FACTION_RELATION_HOSTILE 2

struct FactionRelation
    {
    char target[FACTION_ID_SIZE];
    int relation;
    };

struct FactionDefinition
    {
    char id[FACTION_ID_SIZE];
    char editorColor[FACTION_ID_SIZE];
    struct FactionRelation *relations;
    size_t relationCount;
    };

struct FactionCatalog
    {
    struct FactionDefinition *entries;
    size_t count;
    char playerFaction[FACTION_ID_SIZE];
    };

void factionCatalogInitialize(struct FactionCatalog *catalog);
void factionCatalogDestroy(struct FactionCatalog *catalog);
int factionCatalogLoad(const char *assetRoot, const char *relativePath,
                       struct FactionCatalog *catalog,
                       char *error, size_t errorSize);
const struct FactionDefinition *factionCatalogFind(
    const struct FactionCatalog *catalog, const char *id);
int factionCatalogRelation(const struct FactionCatalog *catalog,
                           const char *source, const char *target);
int factionCatalogHostile(const struct FactionCatalog *catalog,
                          const char *source, const char *target);

#endif
