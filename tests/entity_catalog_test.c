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
    const struct EntityArchetype *road;
    const struct EntityArchetype *hill;
    const struct EntityArchetype *fastFood;
    const struct EntityArchetype *fastFoodAlternate;
    const struct EntityArchetype *explosion;
    const struct EntityArchetype *flameProjectile;
    const struct EntityArchetype *trailProjectile;
    const struct EntityArchetype *waterProjectile;
    const struct EntityArchetype *route;
    const struct EntityArchetype *zone;
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
        if ((catalog.entries[index].renderStyle == ENTITY_RENDER_MODEL &&
             !manifestContainsModel(catalog.entries[index].model)) ||
            (catalog.entries[index].lowModel[0] != '\0' &&
             !manifestContainsModel(catalog.entries[index].lowModel)) ||
            (catalog.entries[index].seasonalModel[0] != '\0' &&
             !manifestContainsModel(catalog.entries[index].seasonalModel)) ||
            (catalog.entries[index].seasonalLowModel[0] != '\0' &&
             !manifestContainsModel(
                 catalog.entries[index].seasonalLowModel)) ||
            !manifestContainsAnimation(catalog.entries[index].idleAnimation) ||
            !manifestContainsAnimation(catalog.entries[index].lowAnimation) ||
            !manifestContainsAnimation(
                catalog.entries[index].seasonalAnimation) ||
            !manifestContainsAnimation(
                catalog.entries[index].seasonalLowAnimation) ||
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
        tank->weapon != ENTITY_WEAPON_PROJECTILE ||
        strcmp(tank->projectile, "projectile-shell") != 0 ||
        tank->controlMode != ENTITY_CONTROL_STEERED ||
        strcmp(tank->aimYawNode, "turret") != 0 ||
        strcmp(tank->aimPitchNode, "barrel") != 0 ||
        strcmp(tank->muzzleNode, "muzzle") != 0 ||
        tank->firstPersonEyeHeight != 0.28f ||
        tank->projectileSpeed < 0.299f || tank->aimPitchDefault != 0.0f ||
        tank->firstPersonVisible ||
        tank->targetMode != ENTITY_TARGET_ALL ||
        tank->sensorCount != 4 || tank->reactionRuleCount != 7 ||
        tank->sensors[1].type != ENTITY_SENSOR_VISION ||
        tank->sensors[2].type != ENTITY_SENSOR_HEARING ||
        tank->reactionRules[0].reaction != ENTITY_REACTION_ATTACK ||
        tank->reactionRules[0].phase != ENTITY_SENSOR_PHASE_ENTER ||
        strcmp(tank->reactionRules[0].emit, "base-alarm") != 0 ||
        tank->spawnWeight != 20 ||
        strcmp(tank->moveSound, "engine-rumble") != 0 ||
        tank->moveSoundInterval != 18 ||
        strcmp(tank->contactAudioTag, "tracked") != 0)
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
    road = entityCatalogFind(&catalog, "road-paved-tile");
    hill = entityCatalogFind(&catalog, "hill-center");
    fastFood = entityCatalogFind(&catalog, "building-fast-food");
    fastFoodAlternate = entityCatalogFind(
        &catalog, "building-fast-food-alternate");
    explosion = entityCatalogFind(&catalog, "explosion-standard");
    flameProjectile = entityCatalogFind(&catalog, "projectile-flame");
    trailProjectile = entityCatalogFind(&catalog, "projectile-scar");
    waterProjectile = entityCatalogFind(&catalog, "projectile-water");
    route = entityCatalogFind(&catalog, "logic-patrol-route");
    zone = entityCatalogFind(&catalog, "logic-guard-zone");
    if (airplane == NULL || fighter == NULL || helicopter == NULL ||
        road == NULL || hill == NULL || fastFood == NULL ||
        fastFoodAlternate == NULL ||
        explosion == NULL || flameProjectile == NULL ||
        trailProjectile == NULL || waterProjectile == NULL ||
        route == NULL || zone == NULL ||
        !flameProjectile->projectileContinuous ||
        !flameProjectile->projectileCollidesGround ||
        flameProjectile->projectileTrailMarker ||
        !trailProjectile->projectileTrailMarker ||
        trailProjectile->projectileCollidesGround ||
        waterProjectile->projectileCollidesGround ||
        flameProjectile->renderStyle != ENTITY_RENDER_PARTICLE_PLUME ||
        flameProjectile->particleCount != 18 ||
        explosion->renderStyle != ENTITY_RENDER_PARTICLE_BURST ||
        explosion->particleCount != 32 ||
        explosion->kind != ENTITY_KIND_EFFECT ||
        explosion->effectDuration != 30 ||
        explosion->effectSoundFrame != 1 ||
        explosion->effectDamageFrame != 4 ||
        explosion->effectDamage != 1 ||
        strcmp(explosion->fireSound, "boom") != 0 ||
        road->surfaceSoundCount != 1 ||
        strcmp(road->surfaceSounds[0].contactTag, "tracked") != 0 ||
        strcmp(road->surfaceSounds[0].sound, "road-clank") != 0 ||
        road->surfaceSounds[0].interval != 20 ||
        hill->grounding != ENTITY_GROUNDING_ORIGIN ||
        fastFood->collisionFootprint !=
            ENTITY_COLLISION_FOOTPRINT_MODEL ||
        tank->collisionFootprint != ENTITY_COLLISION_FOOTPRINT_MODEL ||
        tank->spawnClearance != 0.15f ||
        fastFood->animationSpeed != 0.4f ||
        fastFoodAlternate->animationSpeed != 0.4f ||
        airplane->tactic != ENTITY_TACTIC_FLYBY ||
        airplane->tacticRadius != 20.0f ||
        fighter->tactic != ENTITY_TACTIC_FLYBY ||
        fighter->burstCount != 4 || helicopter->spawnWeight != 17 ||
        entityArchetypePlayerSpeed(helicopter) != 0.12f ||
        route->kind != ENTITY_KIND_ROUTE ||
        route->renderStyle != ENTITY_RENDER_NONE ||
        zone->kind != ENTITY_KIND_ZONE ||
        zone->sensors[0].type != ENTITY_SENSOR_AREA)
        {
        entityCatalogDestroy(&catalog);
        return 1;
        }

    {
    struct EntityArchetype footprint;

    footprint = *tank;
    footprint.modelFootprintRadius = 0.47f;
    if (entityArchetypeCollisionRadius(&footprint) != 0.47f)
        {
        entityCatalogDestroy(&catalog);
        return 1;
        }
    footprint.collisionHeight = 0.3f;
    footprint.modelMinimumY = -0.54f;
    footprint.modelMaximumY = 0.78f;
    if (entityArchetypeCollisionBottomOffset(&footprint) != -0.54f ||
        entityArchetypeCollisionTopOffset(&footprint) != 0.78f)
        {
        entityCatalogDestroy(&catalog);
        return 1;
        }
    footprint.collisionFootprint = ENTITY_COLLISION_FOOTPRINT_RADIUS;
    if (entityArchetypeCollisionRadius(&footprint) !=
        footprint.collisionRadius)
        {
        entityCatalogDestroy(&catalog);
        return 1;
        }
    if (entityArchetypeCollisionBottomOffset(&footprint) != 0.0f ||
        entityArchetypeCollisionTopOffset(&footprint) != 0.3f)
        {
        entityCatalogDestroy(&catalog);
        return 1;
        }
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
        helicopter->modelYawOffset != 0.0f ||
        strcmp(helicopter->aimYawNode, "gun-yaw") != 0 ||
        strcmp(helicopter->aimPitchNode, "gun-pitch") != 0 ||
        strcmp(helicopter->muzzleNode, "muzzle") != 0 ||
        helicopter->projectileSpeed != 0.3f ||
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
        vapour->weapon != ENTITY_WEAPON_PROJECTILE ||
        strcmp(vapour->projectile, "projectile-flame") != 0 ||
        vapour->health != 8 || vapour->spawnWeight != 0 ||
        vapour->collisionMode != ENTITY_COLLISION_PHASE ||
        strcmp(vapour->idleAnimation, "vapour-cloud") != 0 ||
        vapour->shortcut != '8' ||
        vapour->sensorCount != 4 ||
        techs->weapon != ENTITY_WEAPON_PROJECTILE ||
        strcmp(techs->projectile, "projectile-plasma") != 0 ||
        techs->health != 8 || techs->spawnWeight != 0 ||
        strcmp(monster->weaponColor, "blue") != 0 ||
        strcmp(monster->weaponCoreColor, "white") != 0 ||
        strcmp(monster->weaponTraceColor, "black") != 0 ||
        monster->weaponDamage != 1 || !monster->weaponDamagesActors ||
        !monster->weaponDestroysMapObjects ||
        strcmp(monster->destructionSound, "crash") != 0 ||
        monster->destructionDebrisCount != 7 ||
        monster->destructionFireCount != 3 ||
        monster->destructionEffectCount != 1 ||
        monster->debrisLife != 420 ||
        entityCatalogFindShortcut(&catalog, '6') != monster ||
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
