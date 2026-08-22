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
/* Data-driven gameplay entity archetypes.                             */
/***********************************************************************/

#ifndef BATTALION_ENTITY_CATALOG_H
#define BATTALION_ENTITY_CATALOG_H

#include <stddef.h>

#define ENTITY_NAME_SIZE 64
#define ENTITY_PATH_SIZE 256
#define ENTITY_MAX_HOSTILE_FACTIONS 8
#define ENTITY_MAX_SENSORS 8
#define ENTITY_MAX_REACTION_RULES 32
#define ENTITY_MAX_RENDER_NODES 32

#define ENTITY_KIND_VEHICLE 1
#define ENTITY_KIND_NPC 2
#define ENTITY_KIND_OBJECT 3
#define ENTITY_KIND_DEVICE 4
#define ENTITY_KIND_STRUCTURE 5
#define ENTITY_KIND_CREATURE 6

#define ENTITY_MOVEMENT_STATIONARY 0
#define ENTITY_MOVEMENT_GROUND 1
#define ENTITY_MOVEMENT_AIR 2

#define ENTITY_COLLISION_SOLID 1
#define ENTITY_COLLISION_PHASE 2

#define ENTITY_CONTROL_FREE 1
#define ENTITY_CONTROL_STEERED 2

#define ENTITY_WEAPON_NONE 0
#define ENTITY_WEAPON_SHELL 1
#define ENTITY_WEAPON_ROCKET 2
#define ENTITY_WEAPON_BEAM 3
#define ENTITY_WEAPON_FIRE 4
#define ENTITY_WEAPON_PLASMA 5

#define ENTITY_SENSOR_PROXIMITY 1
#define ENTITY_SENSOR_VISION 2
#define ENTITY_SENSOR_HEARING 3
#define ENTITY_SENSOR_DAMAGE 4

#define ENTITY_RULE_SENSOR 1
#define ENTITY_RULE_TRIGGER 2

#define ENTITY_SENSOR_PHASE_ANY 0
#define ENTITY_SENSOR_PHASE_ENTER 1
#define ENTITY_SENSOR_PHASE_STAY 2
#define ENTITY_SENSOR_PHASE_EXIT 3

#define ENTITY_REACTION_OBSERVE 0
#define ENTITY_REACTION_ALERT 1
#define ENTITY_REACTION_INVESTIGATE 2
#define ENTITY_REACTION_PURSUE 3
#define ENTITY_REACTION_ATTACK 4
#define ENTITY_REACTION_FLEE 5
#define ENTITY_REACTION_ACTIVATE 6
#define ENTITY_REACTION_DEACTIVATE 7

#define ENTITY_TARGET_NEAREST 0
#define ENTITY_TARGET_ALL 1

struct EntitySensor
    {
    char id[ENTITY_NAME_SIZE];
    char factionFilter[ENTITY_NAME_SIZE];
    int type;
    float range;
    float angle;
    };

struct EntityReactionRule
    {
    char event[ENTITY_NAME_SIZE];
    char emit[ENTITY_NAME_SIZE];
    int source;
    int phase;
    int reaction;
    };

struct EntityArchetype
    {
    char id[ENTITY_NAME_SIZE];
    char model[ENTITY_NAME_SIZE];
    char destroyedModel[ENTITY_NAME_SIZE];
    char shadowModel[ENTITY_NAME_SIZE];
    char idleAnimation[ENTITY_NAME_SIZE];
    char moveAnimation[ENTITY_NAME_SIZE];
    char attackAnimation[ENTITY_NAME_SIZE];
    char aimYawNode[ENTITY_NAME_SIZE];
    char aimPitchNode[ENTITY_NAME_SIZE];
    char muzzleNode[ENTITY_NAME_SIZE];
    char weaponColor[ENTITY_NAME_SIZE];
    char renderNodes[ENTITY_MAX_RENDER_NODES][ENTITY_NAME_SIZE];
    size_t renderNodeCount;
    char faction[ENTITY_NAME_SIZE];
    char hostileFactions[ENTITY_MAX_HOSTILE_FACTIONS][ENTITY_NAME_SIZE];
    size_t hostileFactionCount;
    struct EntitySensor sensors[ENTITY_MAX_SENSORS];
    size_t sensorCount;
    struct EntityReactionRule reactionRules[ENTITY_MAX_REACTION_RULES];
    size_t reactionRuleCount;
    int kind;
    int movement;
    int collisionMode;
    int controlMode;
    int weapon;
    int targetMode;
    int maxTargets;
    int health;
    int score;
    int fireInterval;
    int lostFrames;
    int spawnWeight;
    int spawnAfter;
    int maxActive;
    int controllable;
    int firstPersonVisible;
    int shortcut;
    int networkCode;
    float scale;
    float modelYawOffset;
    float verticalOffset;
    float speed;
    float turnRate;
    float collisionRadius;
    float collisionHeight;
    float weaponRange;
    float weaponMinRange;
    float projectileSpeed;
    float accuracy;
    float forgetRange;
    float animationSpeed;
    float playerEnergy;
    float playerHeight;
    float playerWidth;
    float playerBottom;
    float playerRegen;
    float playerMoveCost;
    float playerFireCost;
    float playerTurnRate;
    float playerSteerRate;
    float aimPitchDefault;
    float aimPitchMin;
    float aimPitchMax;
    float firstPersonEyeHeight;
    };

struct EntityCatalog
    {
    struct EntityArchetype *entries;
    size_t count;
    char fallback[ENTITY_NAME_SIZE];
    };

void entityCatalogInitialize(struct EntityCatalog *catalog);
void entityCatalogDestroy(struct EntityCatalog *catalog);
int entityCatalogLoad(const char *assetRoot, const char *catalogPath,
                      struct EntityCatalog *catalog,
                      char *error, size_t errorSize);
const struct EntityArchetype *entityCatalogFind(
    const struct EntityCatalog *catalog, const char *id);
const struct EntityArchetype *entityCatalogFindShortcut(
    const struct EntityCatalog *catalog, int shortcut);
const struct EntityArchetype *entityCatalogFindNetworkCode(
    const struct EntityCatalog *catalog, int networkCode);
size_t entityCatalogControllableCount(const struct EntityCatalog *catalog);
const struct EntityArchetype *entityCatalogControllableAt(
    const struct EntityCatalog *catalog, size_t ordinal);
int entityArchetypeIsHostileTo(const struct EntityArchetype *archetype,
                               const char *faction);

#endif
