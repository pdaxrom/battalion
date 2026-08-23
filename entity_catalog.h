/*
 * Copyright (c) 2026 Battalion contributors.
 */
/* Data-driven gameplay entity archetypes.                             */
/***********************************************************************/

#ifndef BATTALION_ENTITY_CATALOG_H
#define BATTALION_ENTITY_CATALOG_H

#include <stddef.h>

#define ENTITY_NAME_SIZE 64
#define ENTITY_PATH_SIZE 256
#define ENTITY_MAX_TARGETS 8
#define ENTITY_MAX_SENSORS 8
#define ENTITY_MAX_REACTION_RULES 32
#define ENTITY_MAX_RENDER_NODES 32
#define ENTITY_MAX_PROPERTIES 32
#define ENTITY_MAX_SURFACE_SOUNDS 8
#define ENTITY_DESCRIPTION_SIZE 256
#define ENTITY_PROPERTY_VALUE_SIZE 128

#define ENTITY_KIND_VEHICLE 1
#define ENTITY_KIND_NPC 2
#define ENTITY_KIND_OBJECT 3
#define ENTITY_KIND_DEVICE 4
#define ENTITY_KIND_STRUCTURE 5
#define ENTITY_KIND_CREATURE 6
#define ENTITY_KIND_ROAD 7
#define ENTITY_KIND_TERRAIN 8
#define ENTITY_KIND_PROJECTILE 9
#define ENTITY_KIND_EFFECT 10
#define ENTITY_KIND_ROUTE 11
#define ENTITY_KIND_ZONE 12

#define ENTITY_MOVEMENT_STATIONARY 0
#define ENTITY_MOVEMENT_GROUND 1
#define ENTITY_MOVEMENT_AIR 2

#define ENTITY_COLLISION_NONE 0
#define ENTITY_COLLISION_SOLID 1
#define ENTITY_COLLISION_PHASE 2

#define ENTITY_CONTROL_FREE 1
#define ENTITY_CONTROL_STEERED 2

#define ENTITY_WEAPON_NONE 0
#define ENTITY_WEAPON_PROJECTILE 1
#define ENTITY_WEAPON_BEAM 2

#define ENTITY_PLACEMENT_NONE 0
#define ENTITY_PLACEMENT_POINT 1
#define ENTITY_PLACEMENT_TILE 2
#define ENTITY_PLACEMENT_POLYLINE 3
#define ENTITY_PLACEMENT_AREA 4

#define ENTITY_RENDER_MODEL 1
#define ENTITY_RENDER_PARTICLE_PLUME 2
#define ENTITY_RENDER_PARTICLE_BURST 3
#define ENTITY_RENDER_NONE 4

#define ENTITY_PROPERTY_BOOLEAN 1
#define ENTITY_PROPERTY_INTEGER 2
#define ENTITY_PROPERTY_FLOAT 3
#define ENTITY_PROPERTY_STRING 4

#define ENTITY_SENSOR_PROXIMITY 1
#define ENTITY_SENSOR_VISION 2
#define ENTITY_SENSOR_HEARING 3
#define ENTITY_SENSOR_DAMAGE 4
#define ENTITY_SENSOR_AREA 5

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

#define ENTITY_TACTIC_STANDOFF 1
#define ENTITY_TACTIC_FLYBY 2

#define ENTITY_COLLISION_FOOTPRINT_RADIUS 1
#define ENTITY_COLLISION_FOOTPRINT_MODEL 2

#define ENTITY_GROUNDING_MODEL_MIN 1
#define ENTITY_GROUNDING_ORIGIN 2

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

struct EntityPropertyDefinition
    {
    char name[ENTITY_NAME_SIZE];
    char defaultValue[ENTITY_PROPERTY_VALUE_SIZE];
    int type;
    };

struct EntitySurfaceSound
    {
    char contactTag[ENTITY_NAME_SIZE];
    char sound[ENTITY_NAME_SIZE];
    int interval;
    };

struct EntityArchetype
    {
    char id[ENTITY_NAME_SIZE];
    char displayName[ENTITY_NAME_SIZE];
    char description[ENTITY_DESCRIPTION_SIZE];
    char model[ENTITY_NAME_SIZE];
    char lowModel[ENTITY_NAME_SIZE];
    char seasonalModel[ENTITY_NAME_SIZE];
    char seasonalLowModel[ENTITY_NAME_SIZE];
    char idleAnimation[ENTITY_NAME_SIZE];
    char lowAnimation[ENTITY_NAME_SIZE];
    char seasonalAnimation[ENTITY_NAME_SIZE];
    char seasonalLowAnimation[ENTITY_NAME_SIZE];
    char moveAnimation[ENTITY_NAME_SIZE];
    char backwardAnimation[ENTITY_NAME_SIZE];
    char attackAnimation[ENTITY_NAME_SIZE];
    char aimYawNode[ENTITY_NAME_SIZE];
    char aimPitchNode[ENTITY_NAME_SIZE];
    char muzzleNode[ENTITY_NAME_SIZE];
    char weaponColor[ENTITY_NAME_SIZE];
    char weaponCoreColor[ENTITY_NAME_SIZE];
    char weaponTraceColor[ENTITY_NAME_SIZE];
    char projectile[ENTITY_NAME_SIZE];
    char impactProjectile[ENTITY_NAME_SIZE];
    char wreckFireProjectile[ENTITY_NAME_SIZE];
    char emitterProjectile[ENTITY_NAME_SIZE];
    char destructionProjectile[ENTITY_NAME_SIZE];
    char destructionFireProjectile[ENTITY_NAME_SIZE];
    char impactEffect[ENTITY_NAME_SIZE];
    char destructionEffect[ENTITY_NAME_SIZE];
    char fireSound[ENTITY_NAME_SIZE];
    char destructionSound[ENTITY_NAME_SIZE];
    char moveSound[ENTITY_NAME_SIZE];
    char contactAudioTag[ENTITY_NAME_SIZE];
    char texture[ENTITY_NAME_SIZE];
    char textureNode[ENTITY_NAME_SIZE];
    char renderColor[ENTITY_NAME_SIZE];
    char particleStartColor[ENTITY_NAME_SIZE];
    char particleMiddleColor[ENTITY_NAME_SIZE];
    char particleEndColor[ENTITY_NAME_SIZE];
    char editorColor[ENTITY_NAME_SIZE];
    char editorCategory[ENTITY_NAME_SIZE];
    char renderNodes[ENTITY_MAX_RENDER_NODES][ENTITY_NAME_SIZE];
    size_t renderNodeCount;
    char faction[ENTITY_NAME_SIZE];
    struct EntitySensor sensors[ENTITY_MAX_SENSORS];
    size_t sensorCount;
    struct EntityReactionRule reactionRules[ENTITY_MAX_REACTION_RULES];
    size_t reactionRuleCount;
    struct EntityPropertyDefinition properties[ENTITY_MAX_PROPERTIES];
    size_t propertyCount;
    struct EntitySurfaceSound surfaceSounds[ENTITY_MAX_SURFACE_SOUNDS];
    size_t surfaceSoundCount;
    int kind;
    int renderStyle;
    int placement;
    int editorPlaceable;
    int movement;
    int collisionMode;
    int collisionFootprint;
    int grounding;
    int controlMode;
    int weapon;
    int projectileCollidesGround;
    int projectileContinuous;
    int projectileTrailMarker;
    int projectileDamagesActors;
    int projectileCollidesActors;
    int projectileCollidesMapObjects;
    int projectileDestroysMapObjects;
    int projectileArmingFrames;
    int projectileDamage;
    int weaponDamage;
    int weaponDamagesActors;
    int weaponDestroysMapObjects;
    int effectDuration;
    int effectDamageFrame;
    int effectSoundFrame;
    int effectDamage;
    int destructionDebrisCount;
    int destructionFireCount;
    int destructionEffectCount;
    int debrisLife;
    int destructible;
    int castsShadow;
    int tactic;
    int burstCount;
    int targetMode;
    int maxTargets;
    int health;
    int score;
    int fireInterval;
    int moveSoundInterval;
    int lostFrames;
    int spawnWeight;
    int spawnAfter;
    int maxActive;
    int controllable;
    int firstPersonVisible;
    int shortcut;
    int particleCount;
    int particleCycleFrames;
    float scale;
    float height;
    int scaleSpecified;
    int heightSpecified;
    float modelYawOffset;
    float verticalOffset;
    float modelMinimumY;
    float modelMaximumY;
    float modelGroundOffset;
    float modelFootprintRadius;
    float speed;
    float playerSpeed;
    float turnRate;
    float collisionRadius;
    float collisionHeight;
    float spawnClearance;
    float weaponRange;
    float weaponMinRange;
    float tacticRadius;
    float projectileSpeed;
    float projectileGravity;
    float effectFadeStart;
    float effectFadeRate;
    float accuracy;
    float forgetRange;
    float animationSpeed;
    float particleSize;
    float particleSpeed;
    float particleSpread;
    float particleGravity;
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
    float view1HeightFraction;
    float view2HeightFraction;
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
size_t entityCatalogControllableCount(const struct EntityCatalog *catalog);
const struct EntityArchetype *entityCatalogControllableAt(
    const struct EntityCatalog *catalog, size_t ordinal);
int entityArchetypeIsActor(const struct EntityArchetype *archetype);
int entityArchetypeIsMapObject(const struct EntityArchetype *archetype);
float entityArchetypeCollisionRadius(
    const struct EntityArchetype *archetype);
float entityArchetypeCollisionBottomOffset(
    const struct EntityArchetype *archetype);
float entityArchetypeCollisionTopOffset(
    const struct EntityArchetype *archetype);
float entityArchetypePlayerSpeed(const struct EntityArchetype *archetype);
int entityArchetypeResolveScale(struct EntityArchetype *archetype,
                                float modelHeight);

#endif
