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
    const struct EntityArchetype *tank;
    const struct EntityArchetype *helicopter;
    const struct EntityArchetype *soldier;
    const struct EntityArchetype *monster;
    const struct EntityArchetype *flutter;
    const struct EntityArchetype *vapour;
    const struct EntityArchetype *techs;
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
            (catalog.entries[index].destroyedModel[0] != '\0' &&
            !manifestContainsModel(catalog.entries[index].destroyedModel)) ||
            (catalog.entries[index].shadowModel[0] != '\0' &&
             !manifestContainsModel(catalog.entries[index].shadowModel)) ||
            !manifestContainsAnimation(catalog.entries[index].idleAnimation) ||
            !manifestContainsAnimation(catalog.entries[index].moveAnimation) ||
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
        tank->firstPersonEyeHeight != 0.28f ||
        tank->muzzleForward < 0.239f || tank->muzzleForward > 0.241f ||
        tank->muzzleHeight < 0.194f || tank->muzzleHeight > 0.196f ||
        tank->projectileSpeed < 0.299f || tank->aimPitchDefault != 0.0f ||
        tank->firstPersonVisible ||
        tank->targetMode != ENTITY_TARGET_ALL ||
        tank->sensorCount != 4 || tank->reactionRuleCount != 6 ||
        tank->sensors[1].type != ENTITY_SENSOR_VISION ||
        tank->sensors[2].type != ENTITY_SENSOR_HEARING ||
        tank->reactionRules[0].reaction != ENTITY_REACTION_ATTACK ||
        tank->reactionRules[0].phase != ENTITY_SENSOR_PHASE_ENTER ||
        strcmp(tank->reactionRules[0].emit, "base-alarm") != 0 ||
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
        !monster->controllable || !vapour->controllable ||
        helicopter->movement != ENTITY_MOVEMENT_AIR ||
        soldier->kind != ENTITY_KIND_NPC ||
        monster->kind != ENTITY_KIND_CREATURE ||
        monster->renderNodeCount != 5 ||
        monster->animationSpeed != 1.0f ||
        monster->muzzleHeight != 0.2f ||
        flutter->animationSpeed != 1.0f ||
        flutter->muzzleHeight != 0.2f ||
        strcmp(monster->renderNodes[0], "body") != 0 ||
        vapour->weapon != ENTITY_WEAPON_FIRE ||
        vapour->collisionMode != ENTITY_COLLISION_PHASE ||
        strcmp(vapour->idleAnimation, "vapour-cloud") != 0 ||
        vapour->shortcut != '8' ||
        vapour->networkCode != 1 ||
        vapour->sensorCount != 4 ||
        techs->weapon != ENTITY_WEAPON_PLASMA ||
        strcmp(monster->weaponColor, "blue") != 0 ||
        entityCatalogFindShortcut(&catalog, '6') != monster ||
        entityCatalogFindNetworkCode(&catalog, 1) != vapour ||
        strcmp(catalog.fallback, "googelon") != 0 ||
        entityCatalogControllableCount(&catalog) != 7 ||
        entityCatalogControllableAt(&catalog, 0) != monster)
        {
        entityCatalogDestroy(&catalog);
        return 1;
        }
    entityCatalogDestroy(&catalog);
    return 0;
    }
