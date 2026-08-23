/*
 * Copyright (c) 2026 Battalion contributors.
 */

#include "faction_catalog.h"

#include <stdio.h>
#include <string.h>

int main(void)
    {
    struct FactionCatalog catalog;
    char error[256];

    factionCatalogInitialize(&catalog);
    if (!factionCatalogLoad("assets", "factions/catalog.txt", &catalog,
                            error, sizeof(error)))
        {
        fprintf(stderr, "%s\n", error);
        return 1;
        }
    if (catalog.count < 4 || strcmp(catalog.playerFaction, "player") != 0)
        {
        fprintf(stderr, "faction catalog contents are incomplete\n");
        return 1;
        }
    if (!factionCatalogHostile(&catalog, "army", "player") ||
        !factionCatalogHostile(&catalog, "player", "army") ||
        factionCatalogHostile(&catalog, "player", "monster") ||
        factionCatalogRelation(&catalog, "player", "monster") !=
            FACTION_RELATION_ALLY ||
        factionCatalogRelation(&catalog, "civilian", "civilian") !=
            FACTION_RELATION_ALLY ||
        factionCatalogRelation(&catalog, "unknown", "army") !=
            FACTION_RELATION_NEUTRAL)
        {
        fprintf(stderr, "faction relationship lookup failed\n");
        factionCatalogDestroy(&catalog);
        return 1;
        }
    factionCatalogDestroy(&catalog);
    return 0;
    }
