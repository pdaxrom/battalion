/*
 * Copyright (c) 2026 Battalion contributors.
 */

#include "entity_catalog.h"

#include <stdio.h>
#include <string.h>

static int manifestContainsModel(const char *key)
    {
    FILE *file;
    char line[1024];
    char directive[32];
    char manifestKey[128];

    file = fopen("assets/manifest.txt", "r");
    if (file == NULL)
        return 0;
    while (fgets(line, sizeof(line), file) != NULL)
        if (sscanf(line, "%31s %127s", directive, manifestKey) == 2 &&
            strcmp(directive, "model") == 0 &&
            strcmp(manifestKey, key) == 0)
            {
            fclose(file);
            return 1;
            }
    fclose(file);
    return 0;
    }

static int manifestContainsAnimation(const char *key)
    {
    FILE *file;
    char line[1024];
    char directive[32];
    char manifestKey[128];

    if (key[0] == '\0')
        return 1;
    file = fopen("assets/manifest.txt", "r");
    if (file == NULL)
        return 0;
    while (fgets(line, sizeof(line), file) != NULL)
        if (sscanf(line, "%31s %127s", directive, manifestKey) == 2 &&
            strcmp(directive, "animation") == 0 &&
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
    struct EntityCatalog catalog;
    struct EntityCatalog invalidCatalog;
    const struct EntityArchetype *tank;
    const struct EntityArchetype *helicopter;
    const struct EntityArchetype *soldier;
    const struct EntityArchetype *monster;
    const struct EntityArchetype *flutter;
    const struct EntityArchetype *vapour;
    const struct EntityArchetype *techs;
    const struct EntityArchetype *airplane;
    const struct EntityArchetype *fighter;
    char error[256];
    size_t index;

    entityCatalogInitialize(&catalog);
    if (!entityCatalogLoad("assets", "entities/catalog.txt", &catalog,
                           error, sizeof(error)))
        {
        fprintf(stderr, "%s\n", error);
        return 1;
        }
    for (index = 0; index < catalog.count; index++)
        {
        if (!manifestContainsModel(catalog.entries[index].model) ||
            !manifestContainsAnimation(catalog.entries[index].idleAnimation) ||
            !manifestContainsAnimation(catalog.entries[index].moveAnimation) ||
            !manifestContainsAnimation(
                catalog.entries[index].backwardAnimation) ||
            !manifestContainsAnimation(catalog.entries[index].attackAnimation))
            {
            fprintf(stderr, "entity model is absent from manifest: %s\n",
                    catalog.entries[index].id);
            entityCatalogDestroy(&catalog);
            return 1;
            }
        }
    tank = entityCatalogFind(&catalog, "tank-standard");
    if (tank == NULL || tank->kind != ENTITY_KIND_VEHICLE ||
        tank->weapon != ENTITY_WEAPON_SHELL ||
        tank->controlMode != ENTITY_CONTROL_STEERED ||
        strcmp(tank->aimYawNode, "turret") != 0 ||
        strcmp(tank->aimPitchNode, "barrel") != 0 ||
        strcmp(tank->muzzleNode, "muzzle") != 0 ||
        tank->firstPersonEyeHeight != 0.28f ||
        tank->projectileSpeed < 0.299f || tank->aimPitchDefault != 0.0f ||
        tank->firstPersonVisible ||
        tank->targetMode != ENTITY_TARGET_ALL ||
        tank->sensorCount != 4 || tank->reactionRuleCount != 6 ||
        tank->sensors[1].type != ENTITY_SENSOR_VISION ||
        tank->sensors[2].type != ENTITY_SENSOR_HEARING ||
        tank->reactionRules[0].reaction != ENTITY_REACTION_ATTACK ||
        tank->reactionRules[0].phase != ENTITY_SENSOR_PHASE_ENTER ||
        strcmp(tank->reactionRules[0].emit, "base-alarm") != 0 ||
        tank->spawnWeight != 20 ||
        !entityArchetypeIsHostileTo(tank, "monster") ||
        entityArchetypeIsHostileTo(tank, "army"))
        {
        entityCatalogDestroy(&catalog);
        return 1;
        }
    helicopter = entityCatalogFind(&catalog, "helo-standard");
    soldier = entityCatalogFind(&catalog, "beam-soldier");
    monster = entityCatalogFind(&catalog, "googelon");
    flutter = entityCatalogFind(&catalog, "flutter");
    vapour = entityCatalogFind(&catalog, "vapour");
    techs = entityCatalogFind(&catalog, "techs");
    airplane = entityCatalogFind(&catalog, "airplane-bomber");
    fighter = entityCatalogFind(&catalog, "fighter");
    if (airplane == NULL || fighter == NULL || helicopter == NULL ||
        airplane->tactic != ENTITY_TACTIC_FLYBY ||
        airplane->tacticRadius != 20.0f ||
        fighter->tactic != ENTITY_TACTIC_FLYBY ||
        fighter->burstCount != 4 || helicopter->spawnWeight != 17)
        {
        entityCatalogDestroy(&catalog);
        return 1;
        }

    {
    const struct EntityArchetype *moira;
    struct EntityArchetype resolvedMoira;

    moira = entityCatalogFind(&catalog, "moira-brown");
    if (moira == NULL || moira->kind != ENTITY_KIND_NPC ||
        !moira->controllable || strcmp(moira->displayName, "Moira Brown") != 0 ||
        strcmp(moira->moveAnimation, "moira-forward") != 0 ||
        strcmp(moira->backwardAnimation, "moira-backward") != 0 ||
        moira->weapon != ENTITY_WEAPON_NONE || moira->height != 0.165f ||
        !moira->heightSpecified || moira->scaleSpecified ||
        moira->scale != 1.0f ||
        moira->modelYawOffset != 180.0f ||
        moira->collisionHeight != 0.165f ||
        moira->view1HeightFraction != 0.34f ||
        moira->view2HeightFraction != 0.25f)
        {
        entityCatalogDestroy(&catalog);
        return 1;
        }
    resolvedMoira = *moira;
    if (!entityArchetypeResolveScale(&resolvedMoira, 126.03356f) ||
        resolvedMoira.scale < 0.001308f ||
        resolvedMoira.scale > 0.001310f)
        {
        entityCatalogDestroy(&catalog);
        return 1;
        }
    }

    if (monster == NULL || flutter == NULL || techs == NULL ||
        strcmp(monster->aimYawNode, "head") != 0 ||
        strcmp(monster->aimPitchNode, "head") != 0 ||
        strcmp(flutter->aimYawNode, "head") != 0 ||
        strcmp(flutter->aimPitchNode, "head") != 0 ||
        strcmp(techs->aimYawNode, "head") != 0 ||
        strcmp(techs->aimPitchNode, "head") != 0)
        {
        entityCatalogDestroy(&catalog);
        return 1;
        }
    if (helicopter == NULL || soldier == NULL || monster == NULL ||
        flutter == NULL ||
        vapour == NULL || techs == NULL ||
        !helicopter->controllable || !soldier->controllable ||
        helicopter->modelYawOffset != 180.0f ||
        helicopter->playerBottom != 1.0f ||
        helicopter->playerHeight != 1.4f ||
        !monster->controllable || !vapour->controllable ||
        helicopter->movement != ENTITY_MOVEMENT_AIR ||
        soldier->kind != ENTITY_KIND_NPC ||
        monster->kind != ENTITY_KIND_CREATURE ||
        monster->renderNodeCount != 5 ||
        monster->health != 8 || monster->spawnWeight != 0 ||
        monster->animationSpeed != 1.0f ||
        strcmp(monster->muzzleNode, "muzzle") != 0 ||
        flutter->animationSpeed != 1.0f ||
        flutter->health != 8 || flutter->spawnWeight != 0 ||
        strcmp(flutter->muzzleNode, "muzzle") != 0 ||
        strcmp(helicopter->muzzleNode, "muzzle") != 0 ||
        strcmp(soldier->muzzleNode, "muzzle") != 0 ||
        strcmp(vapour->muzzleNode, "muzzle") != 0 ||
        strcmp(techs->muzzleNode, "muzzle") != 0 ||
        strcmp(monster->renderNodes[0], "body") != 0 ||
        vapour->weapon != ENTITY_WEAPON_FIRE ||
        vapour->health != 8 || vapour->spawnWeight != 0 ||
        vapour->collisionMode != ENTITY_COLLISION_PHASE ||
        strcmp(vapour->idleAnimation, "vapour-cloud") != 0 ||
        vapour->shortcut != '8' ||
        vapour->networkCode != 1 ||
        vapour->sensorCount != 4 ||
        techs->weapon != ENTITY_WEAPON_PLASMA ||
        techs->health != 8 || techs->spawnWeight != 0 ||
        strcmp(monster->weaponColor, "blue") != 0 ||
        entityCatalogFindShortcut(&catalog, '6') != monster ||
        entityCatalogFindNetworkCode(&catalog, 1) != vapour ||
        strcmp(catalog.fallback, "googelon") != 0 ||
        entityCatalogControllableCount(&catalog) != 8 ||
        entityCatalogControllableAt(&catalog, 0) != monster)
        {
        entityCatalogDestroy(&catalog);
        return 1;
        }
    entityCatalogDestroy(&catalog);

    entityCatalogInitialize(&invalidCatalog);
    if (entityCatalogLoad("tests/fixtures/entity-catalog-missing-muzzle",
                          "catalog.txt", &invalidCatalog,
                          error, sizeof(error)))
        {
        fprintf(stderr, "armed entity without muzzle-node was accepted\n");
        entityCatalogDestroy(&invalidCatalog);
        return 1;
        }
    entityCatalogDestroy(&invalidCatalog);

    entityCatalogInitialize(&invalidCatalog);
    if (entityCatalogLoad("tests/fixtures/entity-catalog-numeric-muzzle",
                          "catalog.txt", &invalidCatalog,
                          error, sizeof(error)))
        {
        fprintf(stderr, "obsolete numeric muzzle directives were accepted\n");
        entityCatalogDestroy(&invalidCatalog);
        return 1;
        }
    entityCatalogDestroy(&invalidCatalog);
    return 0;
    }
