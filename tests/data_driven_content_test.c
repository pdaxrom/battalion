/*
 * Copyright (c) 2026 Battalion contributors.
 */

#include "entity_catalog.h"
#include "audio_catalog.h"
#include "asset_values.h"

#include <stdio.h>
#include <string.h>

static int fileContains(const char *path, const char *pattern)
    {
    char line[2048];
    FILE *file;

    file = fopen(path, "r");
    if (file == NULL)
        {
        fprintf(stderr, "could not audit data-driven source: %s\n", path);
        return -1;
        }
    while (fgets(line, sizeof(line), file) != NULL)
        if (strstr(line, pattern) != NULL)
            {
            fclose(file);
            return 1;
            }
    fclose(file);
    return 0;
    }

static int auditCatalogIds(const struct EntityCatalog *catalog)
    {
    static const char *sources[] = {
        "battalion.c", "graphics.c", "text.c", "update.c", "net.c",
        "audio.c", "audio_spatial.c", "map.c", "map_package.c",
        "map_editor.c", "map_editor_gui.c", "map_editor_model.c",
        "entity_behavior.c", "camera.c", "renderer/fixed_gl.c",
        "renderer/metal.m",
        "assets/config/render.bindings"
    };
    size_t archetypeIndex;
    size_t sourceIndex;
    int found;

    for (archetypeIndex = 0; archetypeIndex < catalog->count;
         archetypeIndex++)
        for (sourceIndex = 0;
             sourceIndex < sizeof(sources) / sizeof(sources[0]);
             sourceIndex++)
            {
            found = fileContains(sources[sourceIndex],
                catalog->entries[archetypeIndex].id);
            if (found != 0)
                {
                if (found > 0)
                    fprintf(stderr,
                        "catalog archetype '%s' is hardcoded in %s\n",
                        catalog->entries[archetypeIndex].id,
                        sources[sourceIndex]);
                return 0;
                }
            }
    return 1;
    }

static int auditManifestContentKeys(void)
    {
    static const char *sources[] = {
        "battalion.c", "graphics.c", "text.c", "update.c", "net.c",
        "audio.c", "audio_spatial.c", "map.c", "map_package.c",
        "map_editor.c", "map_editor_gui.c", "map_editor_model.c",
        "entity_behavior.c", "camera.c", "renderer/fixed_gl.c",
        "renderer/metal.m"
    };
    FILE *manifest;
    char line[1024];
    char directive[32];
    char key[128];
    char literal[132];
    size_t sourceIndex;
    int found;

    manifest = fopen("assets/manifest.txt", "r");
    if (manifest == NULL)
        return 0;
    while (fgets(line, sizeof(line), manifest) != NULL)
        {
        if (sscanf(line, "%31s %127s", directive, key) != 2 ||
            (strcmp(directive, "model") != 0 &&
             strcmp(directive, "animation") != 0 &&
             strcmp(directive, "texture") != 0))
            continue;
        sprintf(literal, "\"%s\"", key);
        for (sourceIndex = 0;
             sourceIndex < sizeof(sources) / sizeof(sources[0]);
             sourceIndex++)
            {
            found = fileContains(sources[sourceIndex], literal);
            if (found != 0)
                {
                if (found > 0)
                    fprintf(stderr,
                        "manifest content key %s is hardcoded in %s\n",
                        key, sources[sourceIndex]);
                fclose(manifest);
                return 0;
                }
            }
        }
    fclose(manifest);
    return 1;
    }

static int auditRetiredContentCode(void)
    {
    static const char *sources[] = {
        "battalion.c", "graphics.c", "text.c", "update.c", "net.c",
        "audio.c", "audio_spatial.c", "map.c", "map_package.c",
        "map_editor.c", "map_editor_gui.c", "map_editor_model.c",
        "camera.c", "renderer/fixed_gl.c", "renderer/metal.m"
    };
    static const char *forbidden[] = {
        "Googelon", "GOOGELON", "VAPOUR", "FLUTTER", "TECHS",
        "DATAENTITY", "LOAD_MODEL(", "drawBuilding",
        "applyExternalPalette(", "struct tank", "updateTanks(",
        "drawTanks(", "requiredEntry(",
        "networkCode", "network-code", "struct road", "drawRoads(",
        "roadSystem", "struct tree", "drawtrees(", "updatetrees(",
        "allTreesOnPlane", "allTreesEverywhere", "struct monsterInfo",
        "drawBooms(", "drawProjectiles(", "drawMaser(",
        "drawAMonster(", "effect-boom-", "boom-fade",
        "config-animation", "struct runtimeFire", "firePrimitives",
        "effect-fire", "effect-sphere-low", "allActorsOnPlane",
        "quickActors(", "MAXACTORSONPLANE", "maserCount", "treeID",
        "buildingBoom", "projectile-mode", "ENTITY_PROJECTILE_FLAME",
        "ENTITY_PROJECTILE_WATER", "ENTITY_PROJECTILE_SCAR", "MAXBEAMS",
        "ENTITY_KIND_ROAD", "ENTITY_KIND_STRUCTURE", "ENTITY_KIND_TERRAIN",
        "ENTITY_KIND_VEHICLE", "ENTITY_KIND_CREATURE"
    };
    size_t sourceIndex;
    size_t patternIndex;
    int found;

    for (sourceIndex = 0;
         sourceIndex < sizeof(sources) / sizeof(sources[0]); sourceIndex++)
        for (patternIndex = 0;
             patternIndex < sizeof(forbidden) / sizeof(forbidden[0]);
             patternIndex++)
            {
            found = fileContains(sources[sourceIndex],
                                 forbidden[patternIndex]);
            if (found != 0)
                {
                if (found > 0)
                    fprintf(stderr, "retired content pattern '%s' in %s\n",
                        forbidden[patternIndex], sources[sourceIndex]);
                return 0;
                }
            }
    return 1;
    }

static int auditFrozenNetworkTokens(void)
    {
    static const char *tokens[] = {
        "MESG_OBJECT_DESTROYED\t\"bld\"",
        "MESG_PLAYER_STATE\t\"upd\"",
        "MESG_JOIN\t\t\"req\"",
        "MESG_HEARTBEAT\t\t\"Iliv\"",
        "MESG_REACQUIRE\t\t\"yo\"",
        "MESG_ACKNOWLEDGE\t\"ack\"",
        "MESG_ACTOR_SPAWN\t\"newTank\"",
        "MESG_ACTOR_STATE\t\"updateTank\"",
        "MESG_RELAY\t\t\"client\""
    };
    size_t index;

    for (index = 0; index < sizeof(tokens) / sizeof(tokens[0]); index++)
        if (fileContains("battalion.h", tokens[index]) != 1)
            {
            fprintf(stderr, "network wire token changed: %s\n", tokens[index]);
            return 0;
            }
    return 1;
    }

static int auditEntityAudio(const struct EntityCatalog *entities)
    {
    struct AudioCatalog audio;
    char error[256];
    size_t index;
    size_t ruleIndex;

    audioCatalogInitialize(&audio);
    if (!audioCatalogLoad("assets/", "assets/audio/catalog.txt", &audio,
                          error, sizeof(error)))
        {
        fprintf(stderr, "%s\n", error);
        return 0;
        }
    for (index = 0; index < entities->count; index++)
        if (entities->entries[index].fireSound[0] != '\0' &&
            audioCatalogFind(&audio, entities->entries[index].fireSound,
                             "pcm") == NULL)
            {
            fprintf(stderr, "entity '%s' uses unknown fire sound '%s'\n",
                    entities->entries[index].id,
                    entities->entries[index].fireSound);
            audioCatalogDestroy(&audio);
            return 0;
            }
        else if (entities->entries[index].destructionSound[0] != '\0' &&
                 audioCatalogFind(&audio,
                     entities->entries[index].destructionSound,
                     "pcm") == NULL)
            {
            fprintf(stderr,
                    "entity '%s' uses unknown destruction sound '%s'\n",
                    entities->entries[index].id,
                    entities->entries[index].destructionSound);
            audioCatalogDestroy(&audio);
            return 0;
            }
        else if (entities->entries[index].moveSound[0] != '\0' &&
                 audioCatalogFind(&audio,
                     entities->entries[index].moveSound, "pcm") == NULL)
            {
            fprintf(stderr, "entity '%s' uses unknown move sound '%s'\n",
                    entities->entries[index].id,
                    entities->entries[index].moveSound);
            audioCatalogDestroy(&audio);
            return 0;
            }
        else
            for (ruleIndex = 0;
                 ruleIndex < entities->entries[index].surfaceSoundCount;
                 ruleIndex++)
                if (audioCatalogFind(&audio,
                        entities->entries[index].surfaceSounds[ruleIndex].sound,
                        "pcm") == NULL)
                    {
                    fprintf(stderr,
                        "entity '%s' uses unknown surface sound '%s'\n",
                        entities->entries[index].id,
                        entities->entries[index].surfaceSounds[ruleIndex].sound);
                    audioCatalogDestroy(&audio);
                    return 0;
                    }
    audioCatalogDestroy(&audio);
    return 1;
    }

static int validPaletteReference(const struct AssetValues *palette,
                                 const char *entityId,
                                 const char *role, const char *key)
    {
    const struct AssetValue *value;

    if (key[0] == '\0')
        return 1;
    value = assetValuesFind(palette, key);
    if (value == NULL || value->componentCount < 3)
        {
        fprintf(stderr, "entity '%s' uses unknown %s color '%s'\n",
                entityId, role, key);
        return 0;
        }
    return 1;
    }

static int auditEntityColors(const struct EntityCatalog *entities)
    {
    struct AssetValues palette;
    char error[256];
    size_t index;

    assetValuesInitialize(&palette);
    if (!assetValuesLoad("assets/config/palette.values", &palette,
                         error, sizeof(error)))
        {
        fprintf(stderr, "%s\n", error);
        return 0;
        }
    for (index = 0; index < entities->count; index++)
        {
        const struct EntityArchetype *entry;
        entry = &entities->entries[index];
        if (!validPaletteReference(&palette, entry->id, "render",
                                   entry->renderColor) ||
            !validPaletteReference(&palette, entry->id, "editor",
                                   entry->editorColor) ||
            !validPaletteReference(&palette, entry->id, "weapon",
                                   entry->weaponColor) ||
            !validPaletteReference(&palette, entry->id, "weapon core",
                                   entry->weaponCoreColor) ||
            !validPaletteReference(&palette, entry->id, "weapon trace",
                                   entry->weaponTraceColor) ||
            !validPaletteReference(&palette, entry->id, "particle start",
                                   entry->particleStartColor) ||
            !validPaletteReference(&palette, entry->id, "particle middle",
                                   entry->particleMiddleColor) ||
            !validPaletteReference(&palette, entry->id, "particle end",
                                   entry->particleEndColor))
            {
            assetValuesDestroy(&palette);
            return 0;
            }
        }
    assetValuesDestroy(&palette);
    return 1;
    }

int main(void)
    {
    struct EntityCatalog catalog;
    char error[256];
    int valid;

    entityCatalogInitialize(&catalog);
    if (!entityCatalogLoad("assets", "entities/catalog.txt", &catalog,
                           error, sizeof(error)))
        {
        fprintf(stderr, "%s\n", error);
        return 1;
        }
    valid = auditCatalogIds(&catalog) && auditManifestContentKeys() &&
            auditRetiredContentCode() && auditFrozenNetworkTokens() &&
            auditEntityAudio(&catalog) && auditEntityColors(&catalog);
    entityCatalogDestroy(&catalog);
    return valid ? 0 : 1;
    }
