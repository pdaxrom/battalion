/*
 * Copyright (c) 2026 Battalion contributors.
 */
/* Data-driven gameplay entity archetypes.                             */
/***********************************************************************/

#include "entity_catalog.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void setEntityError(char *error, size_t errorSize,
                           const char *message)
    {
    if (error == NULL || errorSize == 0)
        return;
    strncpy(error, message, errorSize - 1);
    error[errorSize - 1] = '\0';
    }

static int validName(const char *name)
    {
    const unsigned char *cursor;

    if (name == NULL || name[0] == '\0')
        return 0;
    for (cursor = (const unsigned char *)name; *cursor; cursor++)
        if (!(islower(*cursor) || isdigit(*cursor) || *cursor == '-'))
            return 0;
    return 1;
    }

static int validRelativePath(const char *path)
    {
    const unsigned char *cursor;

    if (path == NULL || path[0] == '\0' || path[0] == '/' ||
        strstr(path, "..") != NULL)
        return 0;
    for (cursor = (const unsigned char *)path; *cursor; cursor++)
        if (!(islower(*cursor) || isdigit(*cursor) || *cursor == '-' ||
              *cursor == '/' || *cursor == '.'))
            return 0;
    return 1;
    }

static int copyName(char *target, const char *source)
    {
    if (!validName(source) || strlen(source) >= ENTITY_NAME_SIZE)
        return 0;
    strcpy(target, source);
    return 1;
    }

static int copyDisplayName(char *target, const char *source)
    {
    const unsigned char *cursor;
    size_t length;

    if (source == NULL)
        return 0;
    length = strlen(source);
    if (length == 0 || length >= ENTITY_NAME_SIZE)
        return 0;
    for (cursor = (const unsigned char *)source; *cursor; cursor++)
        if (*cursor < 32 || *cursor > 126)
            return 0;
    strcpy(target, source);
    return 1;
    }

static int parseInteger(const char *text, int *value)
    {
    char *end;
    long parsed;

    errno = 0;
    parsed = strtol(text, &end, 10);
    if (errno || *end || parsed < -2147483647L || parsed > 2147483647L)
        return 0;
    *value = (int)parsed;
    return 1;
    }

static int parseFloat(const char *text, float *value)
    {
    char *end;
    double parsed;

    errno = 0;
    parsed = strtod(text, &end);
    if (errno || *end)
        return 0;
    *value = (float)parsed;
    return 1;
    }

static int parseBoolean(const char *text, int *value)
    {
    if (strcmp(text, "yes") == 0 || strcmp(text, "true") == 0)
        {
        *value = 1;
        return 1;
        }
    if (strcmp(text, "no") == 0 || strcmp(text, "false") == 0)
        {
        *value = 0;
        return 1;
        }
    return 0;
    }

static int kindValue(const char *text)
    {
    if (strcmp(text, "vehicle") == 0)
        return ENTITY_KIND_VEHICLE;
    if (strcmp(text, "npc") == 0)
        return ENTITY_KIND_NPC;
    if (strcmp(text, "object") == 0)
        return ENTITY_KIND_OBJECT;
    if (strcmp(text, "device") == 0)
        return ENTITY_KIND_DEVICE;
    if (strcmp(text, "structure") == 0)
        return ENTITY_KIND_STRUCTURE;
    if (strcmp(text, "creature") == 0)
        return ENTITY_KIND_CREATURE;
    if (strcmp(text, "road") == 0)
        return ENTITY_KIND_ROAD;
    if (strcmp(text, "terrain") == 0)
        return ENTITY_KIND_TERRAIN;
    if (strcmp(text, "projectile") == 0)
        return ENTITY_KIND_PROJECTILE;
    if (strcmp(text, "effect") == 0)
        return ENTITY_KIND_EFFECT;
    if (strcmp(text, "route") == 0)
        return ENTITY_KIND_ROUTE;
    if (strcmp(text, "zone") == 0)
        return ENTITY_KIND_ZONE;
    return 0;
    }

static int movementValue(const char *text)
    {
    if (strcmp(text, "stationary") == 0)
        return ENTITY_MOVEMENT_STATIONARY;
    if (strcmp(text, "ground") == 0)
        return ENTITY_MOVEMENT_GROUND;
    if (strcmp(text, "air") == 0)
        return ENTITY_MOVEMENT_AIR;
    return -1;
    }

static int collisionModeValue(const char *text)
    {
    if (strcmp(text, "none") == 0)
        return ENTITY_COLLISION_NONE;
    if (strcmp(text, "solid") == 0)
        return ENTITY_COLLISION_SOLID;
    if (strcmp(text, "phase") == 0)
        return ENTITY_COLLISION_PHASE;
    return -1;
    }

static int controlModeValue(const char *text)
    {
    if (strcmp(text, "free") == 0)
        return ENTITY_CONTROL_FREE;
    if (strcmp(text, "steered") == 0)
        return ENTITY_CONTROL_STEERED;
    return 0;
    }

static int weaponValue(const char *text)
    {
    if (strcmp(text, "none") == 0)
        return ENTITY_WEAPON_NONE;
    if (strcmp(text, "projectile") == 0)
        return ENTITY_WEAPON_PROJECTILE;
    if (strcmp(text, "beam") == 0)
        return ENTITY_WEAPON_BEAM;
    return -1;
    }

static int placementValue(const char *text)
    {
    if (strcmp(text, "none") == 0)
        return ENTITY_PLACEMENT_NONE;
    if (strcmp(text, "point") == 0)
        return ENTITY_PLACEMENT_POINT;
    if (strcmp(text, "tile") == 0)
        return ENTITY_PLACEMENT_TILE;
    if (strcmp(text, "polyline") == 0)
        return ENTITY_PLACEMENT_POLYLINE;
    if (strcmp(text, "area") == 0)
        return ENTITY_PLACEMENT_AREA;
    return -1;
    }

static int renderStyleValue(const char *text)
    {
    if (strcmp(text, "model") == 0)
        return ENTITY_RENDER_MODEL;
    if (strcmp(text, "particle-plume") == 0)
        return ENTITY_RENDER_PARTICLE_PLUME;
    if (strcmp(text, "particle-burst") == 0)
        return ENTITY_RENDER_PARTICLE_BURST;
    if (strcmp(text, "none") == 0)
        return ENTITY_RENDER_NONE;
    return 0;
    }

static int propertyTypeValue(const char *text)
    {
    if (strcmp(text, "bool") == 0)
        return ENTITY_PROPERTY_BOOLEAN;
    if (strcmp(text, "int") == 0)
        return ENTITY_PROPERTY_INTEGER;
    if (strcmp(text, "float") == 0)
        return ENTITY_PROPERTY_FLOAT;
    if (strcmp(text, "string") == 0)
        return ENTITY_PROPERTY_STRING;
    return 0;
    }

static int tacticValue(const char *text)
    {
    if (strcmp(text, "standoff") == 0)
        return ENTITY_TACTIC_STANDOFF;
    if (strcmp(text, "flyby") == 0)
        return ENTITY_TACTIC_FLYBY;
    return 0;
    }

static int collisionFootprintValue(const char *text)
    {
    if (strcmp(text, "radius") == 0)
        return ENTITY_COLLISION_FOOTPRINT_RADIUS;
    if (strcmp(text, "model") == 0)
        return ENTITY_COLLISION_FOOTPRINT_MODEL;
    return 0;
    }

static int groundingValue(const char *text)
    {
    if (strcmp(text, "model-min") == 0)
        return ENTITY_GROUNDING_MODEL_MIN;
    if (strcmp(text, "origin") == 0)
        return ENTITY_GROUNDING_ORIGIN;
    return 0;
    }

static int reactionValue(const char *text)
    {
    if (strcmp(text, "observe") == 0)
        return ENTITY_REACTION_OBSERVE;
    if (strcmp(text, "alert") == 0)
        return ENTITY_REACTION_ALERT;
    if (strcmp(text, "investigate") == 0)
        return ENTITY_REACTION_INVESTIGATE;
    if (strcmp(text, "pursue") == 0)
        return ENTITY_REACTION_PURSUE;
    if (strcmp(text, "attack") == 0)
        return ENTITY_REACTION_ATTACK;
    if (strcmp(text, "flee") == 0)
        return ENTITY_REACTION_FLEE;
    if (strcmp(text, "activate") == 0)
        return ENTITY_REACTION_ACTIVATE;
    if (strcmp(text, "deactivate") == 0)
        return ENTITY_REACTION_DEACTIVATE;
    return -1;
    }

static int sensorTypeValue(const char *text)
    {
    if (strcmp(text, "proximity") == 0)
        return ENTITY_SENSOR_PROXIMITY;
    if (strcmp(text, "vision") == 0)
        return ENTITY_SENSOR_VISION;
    if (strcmp(text, "hearing") == 0)
        return ENTITY_SENSOR_HEARING;
    if (strcmp(text, "damage") == 0)
        return ENTITY_SENSOR_DAMAGE;
    if (strcmp(text, "area") == 0)
        return ENTITY_SENSOR_AREA;
    return 0;
    }

static int targetModeValue(const char *text)
    {
    if (strcmp(text, "nearest") == 0)
        return ENTITY_TARGET_NEAREST;
    if (strcmp(text, "all") == 0)
        return ENTITY_TARGET_ALL;
    return -1;
    }

static void initializeArchetype(struct EntityArchetype *entry)
    {
    memset(entry, 0, sizeof(*entry));
    entry->movement = ENTITY_MOVEMENT_GROUND;
    entry->collisionMode = ENTITY_COLLISION_SOLID;
    entry->collisionFootprint = ENTITY_COLLISION_FOOTPRINT_RADIUS;
    entry->grounding = ENTITY_GROUNDING_MODEL_MIN;
    entry->placement = ENTITY_PLACEMENT_POINT;
    entry->editorPlaceable = 1;
    entry->controlMode = ENTITY_CONTROL_FREE;
    entry->weapon = ENTITY_WEAPON_NONE;
    entry->renderStyle = ENTITY_RENDER_MODEL;
    entry->tactic = ENTITY_TACTIC_STANDOFF;
    entry->burstCount = 1;
    strcpy(entry->renderColor, "white");
    entry->targetMode = ENTITY_TARGET_NEAREST;
    entry->maxTargets = 1;
    entry->health = 1;
    entry->score = 10;
    entry->fireInterval = 120;
    entry->moveSoundInterval = 20;
    entry->lostFrames = 180;
    entry->maxActive = 0;
    entry->scale = 1.0f;
    entry->height = 0.0f;
    entry->scaleSpecified = 0;
    entry->heightSpecified = 0;
    entry->modelYawOffset = 0.0f;
    entry->speed = 0.03f;
    entry->playerSpeed = 0.0f;
    entry->turnRate = 0.2f;
    entry->collisionRadius = 0.25f;
    entry->collisionHeight = 0.3f;
    entry->spawnClearance = 0.15f;
    entry->weaponRange = 6.0f;
    entry->weaponMinRange = 0.5f;
    entry->tacticRadius = 20.0f;
    entry->projectileSpeed = 0.08f;
    entry->projectileGravity = 0.0f;
    entry->projectileDamagesActors = 1;
    entry->projectileCollidesActors = 1;
    entry->projectileCollidesMapObjects = 1;
    entry->projectileDestroysMapObjects = 1;
    entry->projectileArmingFrames = 8;
    entry->projectileDamage = 2;
    entry->weaponDamage = 0;
    entry->weaponDamagesActors = 0;
    entry->weaponDestroysMapObjects = 0;
    entry->effectDuration = 0;
    entry->effectDamageFrame = 0;
    entry->effectSoundFrame = 0;
    entry->effectDamage = 0;
    entry->destructionDebrisCount = 0;
    entry->destructionFireCount = 0;
    entry->destructionEffectCount = 0;
    entry->debrisLife = 0;
    entry->effectFadeStart = 1.0f;
    entry->effectFadeRate = 0.0f;
    entry->destructible = 1;
    entry->castsShadow = 1;
    entry->accuracy = 10.0f;
    entry->forgetRange = 12.0f;
    entry->animationSpeed = 0.05f;
    entry->particleCount = 0;
    entry->particleCycleFrames = 0;
    entry->particleSize = 0.0f;
    entry->particleSpeed = 0.0f;
    entry->particleSpread = 0.0f;
    entry->particleGravity = 0.0f;
    entry->playerEnergy = 50.0f;
    entry->playerHeight = 1.3f;
    entry->playerWidth = 0.3f;
    entry->playerBottom = 0.0f;
    entry->playerRegen = 0.1f;
    entry->playerMoveCost = 0.1f;
    entry->playerFireCost = 0.1f;
    entry->playerTurnRate = 30.0f;
    entry->playerSteerRate = 18.0f;
    entry->aimPitchDefault = 0.0f;
    entry->aimPitchMin = -40.0f;
    entry->aimPitchMax = 40.0f;
    entry->firstPersonEyeHeight = 0.0f;
    entry->view1HeightFraction = 0.0f;
    entry->view2HeightFraction = 0.0f;
    }

void entityCatalogInitialize(struct EntityCatalog *catalog)
    {
    if (catalog == NULL)
        return;
    catalog->entries = NULL;
    catalog->count = 0;
    catalog->fallback[0] = '\0';
    }

void entityCatalogDestroy(struct EntityCatalog *catalog)
    {
    if (catalog == NULL)
        return;
    free(catalog->entries);
    entityCatalogInitialize(catalog);
    }

const struct EntityArchetype *entityCatalogFind(
    const struct EntityCatalog *catalog, const char *id)
    {
    size_t index;

    if (catalog == NULL || id == NULL)
        return NULL;
    for (index = 0; index < catalog->count; index++)
        if (strcmp(catalog->entries[index].id, id) == 0)
            return &catalog->entries[index];
    return NULL;
    }

const struct EntityArchetype *entityCatalogFindShortcut(
    const struct EntityCatalog *catalog, int shortcut)
    {
    size_t index;

    if (catalog == NULL || shortcut == 0)
        return NULL;
    for (index = 0; index < catalog->count; index++)
        if (catalog->entries[index].controllable &&
            catalog->entries[index].shortcut == shortcut)
            return &catalog->entries[index];
    return NULL;
    }

size_t entityCatalogControllableCount(const struct EntityCatalog *catalog)
    {
    size_t count;
    size_t index;

    if (catalog == NULL)
        return 0;
    count = 0;
    for (index = 0; index < catalog->count; index++)
        if (catalog->entries[index].controllable)
            count++;
    return count;
    }

const struct EntityArchetype *entityCatalogControllableAt(
    const struct EntityCatalog *catalog, size_t ordinal)
    {
    size_t index;

    if (catalog == NULL)
        return NULL;
    for (index = 0; index < catalog->count; index++)
        if (catalog->entries[index].controllable)
            {
            if (ordinal == 0)
                return &catalog->entries[index];
            ordinal--;
            }
    return NULL;
    }

int entityArchetypeIsActor(const struct EntityArchetype *archetype)
    {
    if (archetype == NULL)
        return 0;
    return archetype->kind == ENTITY_KIND_VEHICLE ||
           archetype->kind == ENTITY_KIND_NPC ||
           archetype->kind == ENTITY_KIND_CREATURE ||
           archetype->kind == ENTITY_KIND_DEVICE ||
           archetype->kind == ENTITY_KIND_OBJECT ||
           archetype->kind == ENTITY_KIND_ZONE;
    }

int entityArchetypeIsMapObject(const struct EntityArchetype *archetype)
    {
    if (archetype == NULL)
        return 0;
    return archetype->kind == ENTITY_KIND_ROAD ||
           archetype->kind == ENTITY_KIND_TERRAIN ||
           archetype->kind == ENTITY_KIND_STRUCTURE;
    }

float entityArchetypeCollisionRadius(
    const struct EntityArchetype *archetype)
    {
    if (archetype == NULL)
        return 0.0f;
    if (archetype->collisionFootprint == ENTITY_COLLISION_FOOTPRINT_MODEL &&
        archetype->modelFootprintRadius > 0.0f)
        return archetype->modelFootprintRadius;
    return archetype->collisionRadius;
    }

float entityArchetypeCollisionBottomOffset(
    const struct EntityArchetype *archetype)
    {
    float bottom;

    if (archetype == NULL)
        return 0.0f;
    bottom = 0.0f;
    if (archetype->collisionFootprint == ENTITY_COLLISION_FOOTPRINT_MODEL &&
        archetype->renderStyle == ENTITY_RENDER_MODEL &&
        archetype->modelMinimumY < bottom)
        bottom = archetype->modelMinimumY;
    return bottom;
    }

float entityArchetypeCollisionTopOffset(
    const struct EntityArchetype *archetype)
    {
    float top;

    if (archetype == NULL)
        return 0.0f;
    top = archetype->collisionHeight;
    if (archetype->collisionFootprint == ENTITY_COLLISION_FOOTPRINT_MODEL &&
        archetype->renderStyle == ENTITY_RENDER_MODEL &&
        archetype->modelMaximumY > top)
        top = archetype->modelMaximumY;
    return top;
    }

float entityArchetypePlayerSpeed(const struct EntityArchetype *archetype)
    {
    if (archetype == NULL)
        return 0.0f;
    return archetype->playerSpeed > 0.0f ?
           archetype->playerSpeed : archetype->speed;
    }

static int assignDirective(struct EntityArchetype *entry,
                           const char *directive, const char *value,
                           int *hasId, int *hasKind, int *hasModel,
                           int *hasFaction)
    {
    int parsed;

    if (strcmp(directive, "id") == 0)
        {
        if (*hasId || !copyName(entry->id, value))
            return 0;
        *hasId = 1;
        return 1;
        }
    if (strcmp(directive, "kind") == 0)
        {
        parsed = kindValue(value);
        if (*hasKind || parsed == 0)
            return 0;
        entry->kind = parsed;
        *hasKind = 1;
        return 1;
        }
    if (strcmp(directive, "model") == 0)
        {
        if (*hasModel || !copyName(entry->model, value))
            return 0;
        *hasModel = 1;
        return 1;
        }
    if (strcmp(directive, "render-style") == 0)
        {
        parsed = renderStyleValue(value);
        if (parsed == 0)
            return 0;
        entry->renderStyle = parsed;
        return 1;
        }
    if (strcmp(directive, "model-low") == 0)
        return entry->lowModel[0] == '\0' && copyName(entry->lowModel, value);
    if (strcmp(directive, "model-seasonal") == 0)
        return entry->seasonalModel[0] == '\0' &&
               copyName(entry->seasonalModel, value);
    if (strcmp(directive, "model-low-seasonal") == 0)
        return entry->seasonalLowModel[0] == '\0' &&
               copyName(entry->seasonalLowModel, value);
    if (strcmp(directive, "idle-animation") == 0)
        return entry->idleAnimation[0] == '\0' &&
               copyName(entry->idleAnimation, value);
    if (strcmp(directive, "animation-low") == 0)
        return entry->lowAnimation[0] == '\0' &&
               copyName(entry->lowAnimation, value);
    if (strcmp(directive, "animation-seasonal") == 0)
        return entry->seasonalAnimation[0] == '\0' &&
               copyName(entry->seasonalAnimation, value);
    if (strcmp(directive, "animation-low-seasonal") == 0)
        return entry->seasonalLowAnimation[0] == '\0' &&
               copyName(entry->seasonalLowAnimation, value);
    if (strcmp(directive, "move-animation") == 0)
        return entry->moveAnimation[0] == '\0' &&
               copyName(entry->moveAnimation, value);
    if (strcmp(directive, "backward-animation") == 0)
        return entry->backwardAnimation[0] == '\0' &&
               copyName(entry->backwardAnimation, value);
    if (strcmp(directive, "attack-animation") == 0)
        return entry->attackAnimation[0] == '\0' &&
               copyName(entry->attackAnimation, value);
    if (strcmp(directive, "aim-yaw-node") == 0)
        return entry->aimYawNode[0] == '\0' &&
               copyName(entry->aimYawNode, value);
    if (strcmp(directive, "aim-pitch-node") == 0)
        return entry->aimPitchNode[0] == '\0' &&
               copyName(entry->aimPitchNode, value);
    if (strcmp(directive, "muzzle-node") == 0)
        return entry->muzzleNode[0] == '\0' &&
               copyName(entry->muzzleNode, value);
    if (strcmp(directive, "projectile") == 0)
        return entry->projectile[0] == '\0' &&
               copyName(entry->projectile, value);
    if (strcmp(directive, "impact-projectile") == 0)
        return entry->impactProjectile[0] == '\0' &&
               copyName(entry->impactProjectile, value);
    if (strcmp(directive, "wreck-fire-projectile") == 0)
        return entry->wreckFireProjectile[0] == '\0' &&
               copyName(entry->wreckFireProjectile, value);
    if (strcmp(directive, "emitter-projectile") == 0)
        return entry->emitterProjectile[0] == '\0' &&
               copyName(entry->emitterProjectile, value);
    if (strcmp(directive, "destruction-projectile") == 0)
        return entry->destructionProjectile[0] == '\0' &&
               copyName(entry->destructionProjectile, value);
    if (strcmp(directive, "destruction-fire-projectile") == 0)
        return entry->destructionFireProjectile[0] == '\0' &&
               copyName(entry->destructionFireProjectile, value);
    if (strcmp(directive, "impact-effect") == 0)
        return entry->impactEffect[0] == '\0' &&
               copyName(entry->impactEffect, value);
    if (strcmp(directive, "destruction-effect") == 0)
        return entry->destructionEffect[0] == '\0' &&
               copyName(entry->destructionEffect, value);
    if (strcmp(directive, "fire-sound") == 0)
        return entry->fireSound[0] == '\0' &&
               copyName(entry->fireSound, value);
    if (strcmp(directive, "destruction-sound") == 0)
        return entry->destructionSound[0] == '\0' &&
               copyName(entry->destructionSound, value);
    if (strcmp(directive, "move-sound") == 0)
        return entry->moveSound[0] == '\0' &&
               copyName(entry->moveSound, value);
    if (strcmp(directive, "contact-audio-tag") == 0)
        return entry->contactAudioTag[0] == '\0' &&
               copyName(entry->contactAudioTag, value);
    if (strcmp(directive, "texture") == 0)
        return entry->texture[0] == '\0' && copyName(entry->texture, value);
    if (strcmp(directive, "texture-node") == 0)
        return entry->textureNode[0] == '\0' &&
               copyName(entry->textureNode, value);
    if (strcmp(directive, "render-color") == 0)
        return copyName(entry->renderColor, value);
    if (strcmp(directive, "particle-start-color") == 0)
        return entry->particleStartColor[0] == '\0' &&
               copyName(entry->particleStartColor, value);
    if (strcmp(directive, "particle-middle-color") == 0)
        return entry->particleMiddleColor[0] == '\0' &&
               copyName(entry->particleMiddleColor, value);
    if (strcmp(directive, "particle-end-color") == 0)
        return entry->particleEndColor[0] == '\0' &&
               copyName(entry->particleEndColor, value);
    if (strcmp(directive, "editor-color") == 0)
        return entry->editorColor[0] == '\0' &&
               copyName(entry->editorColor, value);
    if (strcmp(directive, "editor-category") == 0)
        return entry->editorCategory[0] == '\0' &&
               copyName(entry->editorCategory, value);
    if (strcmp(directive, "render-node") == 0)
        {
        if (entry->renderNodeCount >= ENTITY_MAX_RENDER_NODES ||
            !copyName(entry->renderNodes[entry->renderNodeCount], value))
            return 0;
        entry->renderNodeCount++;
        return 1;
        }
    if (strcmp(directive, "faction") == 0)
        {
        if (*hasFaction || !copyName(entry->faction, value))
            return 0;
        *hasFaction = 1;
        return 1;
        }
    if (strcmp(directive, "movement") == 0)
        {
        parsed = movementValue(value);
        if (parsed < 0)
            return 0;
        entry->movement = parsed;
        return 1;
        }
    if (strcmp(directive, "placement") == 0)
        {
        parsed = placementValue(value);
        if (parsed < 0)
            return 0;
        entry->placement = parsed;
        return 1;
        }
    if (strcmp(directive, "editor-placeable") == 0)
        return parseBoolean(value, &entry->editorPlaceable);
    if (strcmp(directive, "collision-mode") == 0)
        {
        parsed = collisionModeValue(value);
        if (parsed < 0)
            return 0;
        entry->collisionMode = parsed;
        return 1;
        }
    if (strcmp(directive, "control-mode") == 0)
        {
        parsed = controlModeValue(value);
        if (parsed == 0)
            return 0;
        entry->controlMode = parsed;
        return 1;
        }
    if (strcmp(directive, "weapon") == 0)
        {
        parsed = weaponValue(value);
        if (parsed < 0)
            return 0;
        entry->weapon = parsed;
        return 1;
        }
    if (strcmp(directive, "projectile-collides-ground") == 0)
        return parseBoolean(value, &entry->projectileCollidesGround);
    if (strcmp(directive, "projectile-continuous") == 0)
        return parseBoolean(value, &entry->projectileContinuous);
    if (strcmp(directive, "projectile-trail-marker") == 0)
        return parseBoolean(value, &entry->projectileTrailMarker);
    if (strcmp(directive, "projectile-damages-actors") == 0)
        return parseBoolean(value, &entry->projectileDamagesActors);
    if (strcmp(directive, "projectile-collides-actors") == 0)
        return parseBoolean(value, &entry->projectileCollidesActors);
    if (strcmp(directive, "projectile-collides-map-objects") == 0)
        return parseBoolean(value, &entry->projectileCollidesMapObjects);
    if (strcmp(directive, "projectile-destroys-map-objects") == 0)
        return parseBoolean(value, &entry->projectileDestroysMapObjects);
    if (strcmp(directive, "projectile-arming-frames") == 0)
        return parseInteger(value, &entry->projectileArmingFrames);
    if (strcmp(directive, "projectile-damage") == 0)
        return parseInteger(value, &entry->projectileDamage);
    if (strcmp(directive, "weapon-damage") == 0)
        return parseInteger(value, &entry->weaponDamage);
    if (strcmp(directive, "weapon-damages-actors") == 0)
        return parseBoolean(value, &entry->weaponDamagesActors);
    if (strcmp(directive, "weapon-destroys-map-objects") == 0)
        return parseBoolean(value, &entry->weaponDestroysMapObjects);
    if (strcmp(directive, "effect-duration") == 0)
        return parseInteger(value, &entry->effectDuration);
    if (strcmp(directive, "effect-damage-frame") == 0)
        return parseInteger(value, &entry->effectDamageFrame);
    if (strcmp(directive, "effect-sound-frame") == 0)
        return parseInteger(value, &entry->effectSoundFrame);
    if (strcmp(directive, "effect-damage") == 0)
        return parseInteger(value, &entry->effectDamage);
    if (strcmp(directive, "destruction-debris-count") == 0)
        return parseInteger(value, &entry->destructionDebrisCount);
    if (strcmp(directive, "destruction-fire-count") == 0)
        return parseInteger(value, &entry->destructionFireCount);
    if (strcmp(directive, "destruction-effect-count") == 0)
        return parseInteger(value, &entry->destructionEffectCount);
    if (strcmp(directive, "debris-life") == 0)
        return parseInteger(value, &entry->debrisLife);
    if (strcmp(directive, "destructible") == 0)
        return parseBoolean(value, &entry->destructible);
    if (strcmp(directive, "casts-shadow") == 0)
        return parseBoolean(value, &entry->castsShadow);
    if (strcmp(directive, "collision-footprint") == 0)
        {
        parsed = collisionFootprintValue(value);
        if (parsed == 0)
            return 0;
        entry->collisionFootprint = parsed;
        return 1;
        }
    if (strcmp(directive, "grounding") == 0)
        {
        parsed = groundingValue(value);
        if (parsed == 0)
            return 0;
        entry->grounding = parsed;
        return 1;
        }
    if (strcmp(directive, "tactic") == 0)
        {
        parsed = tacticValue(value);
        if (parsed == 0)
            return 0;
        entry->tactic = parsed;
        return 1;
        }
    if (strcmp(directive, "burst-count") == 0)
        return parseInteger(value, &entry->burstCount);
    if (strcmp(directive, "weapon-color") == 0)
        return copyName(entry->weaponColor, value);
    if (strcmp(directive, "weapon-core-color") == 0)
        return copyName(entry->weaponCoreColor, value);
    if (strcmp(directive, "weapon-trace-color") == 0)
        return copyName(entry->weaponTraceColor, value);
    if (strcmp(directive, "target-mode") == 0)
        {
        parsed = targetModeValue(value);
        if (parsed < 0)
            return 0;
        entry->targetMode = parsed;
        return 1;
        }
    if (strcmp(directive, "max-targets") == 0)
        return parseInteger(value, &entry->maxTargets);
    if (strcmp(directive, "health") == 0)
        return parseInteger(value, &entry->health);
    if (strcmp(directive, "score") == 0)
        return parseInteger(value, &entry->score);
    if (strcmp(directive, "fire-interval") == 0)
        return parseInteger(value, &entry->fireInterval);
    if (strcmp(directive, "move-sound-interval") == 0)
        return parseInteger(value, &entry->moveSoundInterval);
    if (strcmp(directive, "lost-frames") == 0)
        return parseInteger(value, &entry->lostFrames);
    if (strcmp(directive, "spawn-weight") == 0)
        return parseInteger(value, &entry->spawnWeight);
    if (strcmp(directive, "spawn-after") == 0)
        return parseInteger(value, &entry->spawnAfter);
    if (strcmp(directive, "max-active") == 0)
        return parseInteger(value, &entry->maxActive);
    if (strcmp(directive, "controllable") == 0)
        return parseBoolean(value, &entry->controllable);
    if (strcmp(directive, "first-person-visible") == 0)
        return parseBoolean(value, &entry->firstPersonVisible);
    if (strcmp(directive, "shortcut") == 0)
        {
        if (entry->shortcut != 0 || strlen(value) != 1)
            return 0;
        entry->shortcut = (unsigned char)value[0];
        return 1;
        }
    if (strcmp(directive, "particle-count") == 0)
        return parseInteger(value, &entry->particleCount);
    if (strcmp(directive, "particle-cycle-frames") == 0)
        return parseInteger(value, &entry->particleCycleFrames);
    if (strcmp(directive, "scale") == 0)
        {
        if (entry->scaleSpecified || !parseFloat(value, &entry->scale))
            return 0;
        entry->scaleSpecified = 1;
        return 1;
        }
    if (strcmp(directive, "height") == 0)
        {
        if (entry->heightSpecified || !parseFloat(value, &entry->height))
            return 0;
        entry->heightSpecified = 1;
        return 1;
        }
    if (strcmp(directive, "model-yaw-offset") == 0)
        return parseFloat(value, &entry->modelYawOffset);
    if (strcmp(directive, "vertical-offset") == 0)
        return parseFloat(value, &entry->verticalOffset);
    if (strcmp(directive, "speed") == 0)
        return parseFloat(value, &entry->speed);
    if (strcmp(directive, "player-speed") == 0)
        return parseFloat(value, &entry->playerSpeed);
    if (strcmp(directive, "turn-rate") == 0)
        return parseFloat(value, &entry->turnRate);
    if (strcmp(directive, "collision-radius") == 0)
        return parseFloat(value, &entry->collisionRadius);
    if (strcmp(directive, "collision-height") == 0)
        return parseFloat(value, &entry->collisionHeight);
    if (strcmp(directive, "spawn-clearance") == 0)
        return parseFloat(value, &entry->spawnClearance);
    if (strcmp(directive, "weapon-range") == 0)
        return parseFloat(value, &entry->weaponRange);
    if (strcmp(directive, "weapon-min-range") == 0)
        return parseFloat(value, &entry->weaponMinRange);
    if (strcmp(directive, "tactic-radius") == 0)
        return parseFloat(value, &entry->tacticRadius);
    if (strcmp(directive, "projectile-speed") == 0)
        return parseFloat(value, &entry->projectileSpeed);
    if (strcmp(directive, "projectile-gravity") == 0)
        return parseFloat(value, &entry->projectileGravity);
    if (strcmp(directive, "effect-fade-start") == 0)
        return parseFloat(value, &entry->effectFadeStart);
    if (strcmp(directive, "effect-fade-rate") == 0)
        return parseFloat(value, &entry->effectFadeRate);
    if (strcmp(directive, "accuracy") == 0)
        return parseFloat(value, &entry->accuracy);
    if (strcmp(directive, "forget-range") == 0)
        return parseFloat(value, &entry->forgetRange);
    if (strcmp(directive, "animation-speed") == 0)
        return parseFloat(value, &entry->animationSpeed);
    if (strcmp(directive, "particle-size") == 0)
        return parseFloat(value, &entry->particleSize);
    if (strcmp(directive, "particle-speed") == 0)
        return parseFloat(value, &entry->particleSpeed);
    if (strcmp(directive, "particle-spread") == 0)
        return parseFloat(value, &entry->particleSpread);
    if (strcmp(directive, "particle-gravity") == 0)
        return parseFloat(value, &entry->particleGravity);
    if (strcmp(directive, "player-energy") == 0)
        return parseFloat(value, &entry->playerEnergy);
    if (strcmp(directive, "player-height") == 0)
        return parseFloat(value, &entry->playerHeight);
    if (strcmp(directive, "player-width") == 0)
        return parseFloat(value, &entry->playerWidth);
    if (strcmp(directive, "player-bottom") == 0)
        return parseFloat(value, &entry->playerBottom);
    if (strcmp(directive, "player-regen") == 0)
        return parseFloat(value, &entry->playerRegen);
    if (strcmp(directive, "player-move-cost") == 0)
        return parseFloat(value, &entry->playerMoveCost);
    if (strcmp(directive, "player-fire-cost") == 0)
        return parseFloat(value, &entry->playerFireCost);
    if (strcmp(directive, "player-turn-rate") == 0)
        return parseFloat(value, &entry->playerTurnRate);
    if (strcmp(directive, "player-steer-rate") == 0)
        return parseFloat(value, &entry->playerSteerRate);
    if (strcmp(directive, "aim-pitch-default") == 0)
        return parseFloat(value, &entry->aimPitchDefault);
    if (strcmp(directive, "aim-pitch-min") == 0)
        return parseFloat(value, &entry->aimPitchMin);
    if (strcmp(directive, "aim-pitch-max") == 0)
        return parseFloat(value, &entry->aimPitchMax);
    if (strcmp(directive, "first-person-eye-height") == 0)
        return parseFloat(value, &entry->firstPersonEyeHeight);
    if (strcmp(directive, "view-1-height-fraction") == 0)
        return parseFloat(value, &entry->view1HeightFraction);
    if (strcmp(directive, "view-2-height-fraction") == 0)
        return parseFloat(value, &entry->view2HeightFraction);
    return 0;
    }

static const struct EntitySensor *findSensor(
    const struct EntityArchetype *entry, const char *id)
    {
    size_t index;

    for (index = 0; index < entry->sensorCount; index++)
        if (strcmp(entry->sensors[index].id, id) == 0)
            return &entry->sensors[index];
    return NULL;
    }

static int addSensor(struct EntityArchetype *entry, const char *id,
                     const char *type, const char *range,
                     const char *angle, const char *filter)
    {
    struct EntitySensor *sensor;
    int parsedType;

    parsedType = sensorTypeValue(type);
    if (entry->sensorCount >= ENTITY_MAX_SENSORS || parsedType == 0 ||
        !validName(id) || !validName(filter) ||
        findSensor(entry, id) != NULL)
        return 0;
    sensor = &entry->sensors[entry->sensorCount];
    memset(sensor, 0, sizeof(*sensor));
    if (!copyName(sensor->id, id) ||
        !copyName(sensor->factionFilter, filter) ||
        !parseFloat(range, &sensor->range) ||
        !parseFloat(angle, &sensor->angle) || sensor->range < 0.0f ||
        sensor->angle <= 0.0f || sensor->angle > 360.0f)
        return 0;
    sensor->type = parsedType;
    entry->sensorCount++;
    return 1;
    }

static int addReactionRule(struct EntityArchetype *entry, int source,
                           int phase, const char *event,
                           const char *reaction, const char *emit)
    {
    struct EntityReactionRule *rule;
    int parsedReaction;

    parsedReaction = reactionValue(reaction);
    if (entry->reactionRuleCount >= ENTITY_MAX_REACTION_RULES ||
        !validName(event) || !validName(emit) || parsedReaction < 0)
        return 0;
    if (source == ENTITY_RULE_SENSOR && findSensor(entry, event) == NULL)
        return 0;
    rule = &entry->reactionRules[entry->reactionRuleCount++];
    memset(rule, 0, sizeof(*rule));
    copyName(rule->event, event);
    copyName(rule->emit, emit);
    rule->source = source;
    rule->phase = phase;
    rule->reaction = parsedReaction;
    return 1;
    }

static int addPropertyDefinition(struct EntityArchetype *entry,
                                 const char *name, const char *type,
                                 const char *defaultValue)
    {
    struct EntityPropertyDefinition *property;
    size_t index;
    int parsedType;

    parsedType = propertyTypeValue(type);
    if (entry->propertyCount >= ENTITY_MAX_PROPERTIES ||
        !validName(name) || parsedType == 0 || defaultValue == NULL ||
        strlen(defaultValue) >= ENTITY_PROPERTY_VALUE_SIZE)
        return 0;
    for (index = 0; index < entry->propertyCount; index++)
        if (strcmp(entry->properties[index].name, name) == 0)
            return 0;
    property = &entry->properties[entry->propertyCount++];
    memset(property, 0, sizeof(*property));
    strcpy(property->name, name);
    strcpy(property->defaultValue, defaultValue);
    property->type = parsedType;
    return 1;
    }

static int addSurfaceSound(struct EntityArchetype *entry,
                           const char *contactTag, const char *sound,
                           const char *interval)
    {
    struct EntitySurfaceSound *rule;
    int parsedInterval;

    if (entry->surfaceSoundCount >= ENTITY_MAX_SURFACE_SOUNDS ||
        !validName(contactTag) || !validName(sound) ||
        !parseInteger(interval, &parsedInterval) || parsedInterval < 1)
        return 0;
    rule = &entry->surfaceSounds[entry->surfaceSoundCount++];
    memset(rule, 0, sizeof(*rule));
    copyName(rule->contactTag, contactTag);
    copyName(rule->sound, sound);
    rule->interval = parsedInterval;
    return 1;
    }

static int validArchetype(const struct EntityArchetype *entry,
                          int hasId, int hasKind, int hasModel,
                          int hasFaction)
    {
    (void)hasFaction;
    if (!hasId || !hasKind ||
        (entry->renderStyle == ENTITY_RENDER_MODEL && !hasModel) ||
        (entry->renderStyle != ENTITY_RENDER_MODEL && hasModel) ||
        entry->displayName[0] == '\0' ||
        entry->description[0] == '\0' ||
        (entry->editorPlaceable &&
         (entry->editorCategory[0] == '\0' ||
          entry->editorColor[0] == '\0')))
        return 0;
    if (entry->maxTargets < 1 ||
        entry->maxTargets > ENTITY_MAX_TARGETS ||
        entry->health < 1 || entry->score < 0 || entry->fireInterval < 1 ||
        entry->moveSoundInterval < 1 ||
        entry->burstCount < 1 || entry->burstCount > 32 ||
        entry->lostFrames < 0 || entry->spawnWeight < 0 ||
        entry->spawnAfter < 0 || entry->maxActive < 0 ||
        entry->scale <= 0.0f ||
        (entry->heightSpecified && entry->height <= 0.0f) ||
        (entry->scaleSpecified && entry->heightSpecified) ||
        entry->modelYawOffset < -360.0f ||
        entry->modelYawOffset > 360.0f || entry->speed < 0.0f ||
        entry->playerSpeed < 0.0f ||
        entry->turnRate <= 0.0f ||
        (entry->collisionFootprint == ENTITY_COLLISION_FOOTPRINT_MODEL &&
         entry->renderStyle != ENTITY_RENDER_MODEL) ||
        (entry->collisionMode != ENTITY_COLLISION_NONE &&
         (entry->collisionRadius <= 0.0f || entry->collisionHeight <= 0.0f)) ||
        entry->spawnClearance < 0.0f ||
        entry->weaponRange < 0.0f ||
        entry->weaponMinRange < 0.0f ||
        entry->weaponMinRange > entry->weaponRange ||
        entry->tacticRadius < 0.0f ||
        entry->projectileSpeed <= 0.0f || entry->projectileGravity < 0.0f ||
        entry->projectileArmingFrames < 0 || entry->projectileDamage < 0 ||
        entry->weaponDamage < 0 ||
        entry->effectDuration < 0 || entry->effectDamageFrame < 0 ||
        entry->effectSoundFrame < 0 || entry->effectDamage < 0 ||
        entry->destructionDebrisCount < 0 ||
        entry->destructionFireCount < 0 ||
        entry->destructionEffectCount < 0 || entry->debrisLife < 0 ||
        entry->effectFadeStart < 0.0f || entry->effectFadeRate < 0.0f ||
        entry->accuracy < 0.0f ||
        entry->forgetRange < 0.0f || entry->animationSpeed <= 0.0f ||
        entry->particleCount < 0 || entry->particleCount > 256 ||
        entry->particleCycleFrames < 0 ||
        entry->particleSize < 0.0f || entry->particleSpeed < 0.0f ||
        entry->particleSpread < 0.0f || entry->particleGravity < 0.0f ||
        (entry->controllable &&
         (entry->playerEnergy <= 0.0f || entry->playerHeight <= 0.0f ||
          entry->playerHeight <= entry->playerBottom ||
          entry->playerWidth <= 0.0f || entry->playerRegen < 0.0f ||
          entry->playerMoveCost < 0.0f || entry->playerFireCost < 0.0f ||
          entry->playerTurnRate <= 0.0f ||
          entry->playerSteerRate <= 0.0f ||
          entry->aimPitchMin >= entry->aimPitchMax ||
          entry->aimPitchDefault < entry->aimPitchMin ||
          entry->aimPitchDefault > entry->aimPitchMax ||
          entry->firstPersonEyeHeight <= 0.0f ||
          entry->view1HeightFraction <= 0.0f ||
          entry->view1HeightFraction >= 1.0f ||
          entry->view2HeightFraction <= 0.0f ||
          entry->view2HeightFraction >= 1.0f)))
        return 0;
    if ((entry->renderStyle == ENTITY_RENDER_PARTICLE_PLUME ||
         entry->renderStyle == ENTITY_RENDER_PARTICLE_BURST) &&
        (entry->particleStartColor[0] == '\0' ||
         entry->particleMiddleColor[0] == '\0' ||
         entry->particleEndColor[0] == '\0' ||
         entry->particleCount < 1 || entry->particleCycleFrames < 1 ||
         entry->particleSize <= 0.0f || entry->particleSpeed <= 0.0f ||
         entry->renderNodeCount != 0 || entry->texture[0] != '\0' ||
         entry->idleAnimation[0] != '\0' || entry->lowAnimation[0] != '\0'))
        return 0;
    if (entry->renderStyle == ENTITY_RENDER_NONE &&
        (entry->renderNodeCount != 0 || entry->texture[0] != '\0' ||
         entry->idleAnimation[0] != '\0' || entry->lowAnimation[0] != '\0' ||
         entry->seasonalAnimation[0] != '\0' ||
         entry->seasonalLowAnimation[0] != '\0' ||
         entry->moveAnimation[0] != '\0' ||
         entry->backwardAnimation[0] != '\0' ||
         entry->attackAnimation[0] != '\0'))
        return 0;
    if (entry->renderStyle == ENTITY_RENDER_MODEL &&
        (entry->particleStartColor[0] != '\0' ||
         entry->particleMiddleColor[0] != '\0' ||
         entry->particleEndColor[0] != '\0' || entry->particleCount != 0 ||
         entry->particleCycleFrames != 0 || entry->particleSize != 0.0f ||
         entry->particleSpeed != 0.0f || entry->particleSpread != 0.0f ||
         entry->particleGravity != 0.0f))
        return 0;
    if (entry->weapon != ENTITY_WEAPON_NONE &&
        entry->muzzleNode[0] == '\0')
        return 0;
    if (entry->weapon == ENTITY_WEAPON_BEAM &&
        (entry->weaponColor[0] == '\0' ||
         entry->weaponCoreColor[0] == '\0' ||
         (entry->weaponDamagesActors && entry->weaponDamage < 1)))
        return 0;
    if (entry->weapon != ENTITY_WEAPON_BEAM &&
        (entry->weaponDamage != 0 || entry->weaponDamagesActors ||
         entry->weaponDestroysMapObjects))
        return 0;
    if ((entry->destructionDebrisCount > 0 &&
         entry->destructionProjectile[0] == '\0') ||
        (entry->destructionFireCount > 0 &&
         entry->wreckFireProjectile[0] == '\0' &&
         entry->destructionFireProjectile[0] == '\0') ||
        (entry->destructionEffectCount > 0 &&
         entry->destructionEffect[0] == '\0'))
        return 0;
    if ((entry->weapon == ENTITY_WEAPON_PROJECTILE) !=
        (entry->projectile[0] != '\0'))
        return 0;
    if (entry->kind == ENTITY_KIND_PROJECTILE &&
        (entry->editorPlaceable || entry->placement != ENTITY_PLACEMENT_NONE))
        return 0;
    if (entry->kind != ENTITY_KIND_PROJECTILE &&
        (entry->projectileCollidesGround || entry->projectileContinuous ||
         entry->projectileTrailMarker))
        return 0;
    if (entry->kind == ENTITY_KIND_EFFECT &&
        (entry->effectDuration < 1 || entry->editorPlaceable ||
         entry->placement != ENTITY_PLACEMENT_NONE ||
         entry->effectDamageFrame > entry->effectDuration ||
         entry->effectSoundFrame > entry->effectDuration))
        return 0;
    if (entry->kind != ENTITY_KIND_EFFECT && entry->effectDuration != 0)
        return 0;
    if ((entry->aimYawNode[0] == '\0') !=
        (entry->aimPitchNode[0] == '\0'))
        return 0;
    if (entry->shortcut != 0 && !entry->controllable)
        return 0;
    {
    size_t ruleIndex;
    for (ruleIndex = 0; ruleIndex < entry->reactionRuleCount; ruleIndex++)
        if (entry->reactionRules[ruleIndex].reaction ==
                ENTITY_REACTION_ATTACK &&
            entry->weapon == ENTITY_WEAPON_NONE)
            return 0;
    }
    return 1;
    }

int entityArchetypeResolveScale(struct EntityArchetype *archetype,
                                float modelHeight)
    {
    float resolved;

    if (archetype == NULL)
        return 0;
    if (!archetype->heightSpecified)
        return archetype->scale > 0.0f;
    if (archetype->scaleSpecified || modelHeight <= 0.0f)
        return 0;
    resolved = archetype->height / modelHeight;
    if (resolved <= 0.0f)
        return 0;
    archetype->scale = resolved;
    return 1;
    }

static int loadArchetype(const char *path, struct EntityArchetype *entry,
                         char *error, size_t errorSize)
    {
    FILE *file;
    char line[512];
    char directive[64];
    char value[256];
    char argument2[256];
    char argument3[256];
    char argument4[256];
    char argument5[256];
    char extra[2];
    int header;
    int fields;
    int hasId;
    int hasKind;
    int hasModel;
    int hasFaction;

    file = fopen(path, "r");
    if (file == NULL)
        {
        setEntityError(error, errorSize, "Could not open entity description");
        return 0;
        }
    initializeArchetype(entry);
    header = hasId = hasKind = hasModel = hasFaction = 0;
    while (fgets(line, sizeof(line), file) != NULL)
        {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        fields = sscanf(line, "%63s %255s %255s %255s %255s %255s %1s",
                        directive, value, argument2, argument3, argument4,
                        argument5, extra);
        if (!header)
            {
            if (fields != 2 || strcmp(directive, "battalion-entity") != 0 ||
                strcmp(value, "1") != 0)
                goto invalid;
            header = 1;
            continue;
            }
        if (strncmp(line, "display-name ", 13) == 0)
            {
            char *display;
            size_t length;

            display = line + 13;
            length = strlen(display);
            while (length > 0 &&
                   (display[length - 1] == '\n' ||
                    display[length - 1] == '\r'))
                display[--length] = '\0';
            if (entry->displayName[0] != '\0' ||
                !copyDisplayName(entry->displayName, display))
                goto invalid;
            continue;
            }
        if (strncmp(line, "description ", 12) == 0)
            {
            char *description;
            size_t length;

            description = line + 12;
            length = strlen(description);
            while (length > 0 &&
                   (description[length - 1] == '\n' ||
                    description[length - 1] == '\r'))
                description[--length] = '\0';
            if (entry->description[0] != '\0' || length == 0 ||
                length >= sizeof(entry->description))
                goto invalid;
            strcpy(entry->description, description);
            continue;
            }
        if (strcmp(directive, "sensor") == 0)
            {
            if (fields != 6 ||
                !addSensor(entry, value, argument2, argument3, argument4,
                           argument5))
                goto invalid;
            }
        else if (strcmp(directive, "property") == 0)
            {
            if (fields != 4 ||
                !addPropertyDefinition(entry, value, argument2, argument3))
                goto invalid;
            }
        else if (strcmp(directive, "surface-sound") == 0)
            {
            if (fields != 4 ||
                !addSurfaceSound(entry, value, argument2, argument3))
                goto invalid;
            }
        else if (strcmp(directive, "on-enter") == 0 ||
                 strcmp(directive, "on-stay") == 0 ||
                 strcmp(directive, "on-exit") == 0 ||
                 strcmp(directive, "on-trigger") == 0)
            {
            int source;
            int phase;

            source = ENTITY_RULE_SENSOR;
            phase = ENTITY_SENSOR_PHASE_ANY;
            if (strcmp(directive, "on-trigger") == 0)
                source = ENTITY_RULE_TRIGGER;
            else if (strcmp(directive, "on-enter") == 0)
                phase = ENTITY_SENSOR_PHASE_ENTER;
            else if (strcmp(directive, "on-stay") == 0)
                phase = ENTITY_SENSOR_PHASE_STAY;
            else if (strcmp(directive, "on-exit") == 0)
                phase = ENTITY_SENSOR_PHASE_EXIT;
            if (fields != 4 ||
                !addReactionRule(entry, source, phase, value, argument2,
                                 argument3))
                goto invalid;
            }
        else if (fields != 2 ||
                 !assignDirective(entry, directive, value, &hasId,
                                  &hasKind, &hasModel, &hasFaction))
            goto invalid;
        }
    fclose(file);
    if (!header || !validArchetype(entry, hasId, hasKind, hasModel,
                                  hasFaction))
        {
        setEntityError(error, errorSize, "Invalid entity description values");
        return 0;
        }
    return 1;

invalid:
    fclose(file);
    setEntityError(error, errorSize, "Invalid entity description syntax");
    return 0;
    }

static int appendArchetype(struct EntityCatalog *catalog,
                           const struct EntityArchetype *entry,
                           char *error, size_t errorSize)
    {
    struct EntityArchetype *expanded;
    size_t index;

    if (entityCatalogFind(catalog, entry->id) != NULL)
        {
        setEntityError(error, errorSize, "Duplicate entity archetype id");
        return 0;
        }
    if (entry->shortcut != 0)
        for (index = 0; index < catalog->count; index++)
            if (catalog->entries[index].shortcut == entry->shortcut)
                {
                setEntityError(error, errorSize,
                               "Duplicate entity selection shortcut");
                return 0;
                }
    expanded = (struct EntityArchetype *)realloc(
        catalog->entries, (catalog->count + 1) * sizeof(*expanded));
    if (expanded == NULL)
        {
        setEntityError(error, errorSize, "Out of memory adding entity");
        return 0;
        }
    catalog->entries = expanded;
    catalog->entries[catalog->count++] = *entry;
    return 1;
    }

static int buildPath(char *target, size_t targetSize, const char *root,
                     const char *relative)
    {
    size_t rootLength;

    if (root == NULL || relative == NULL || !validRelativePath(relative))
        return 0;
    rootLength = strlen(root);
    if (rootLength + strlen(relative) + 2 > targetSize)
        return 0;
    strcpy(target, root);
    if (rootLength > 0 && target[rootLength - 1] != '/')
        strcat(target, "/");
    strcat(target, relative);
    return 1;
    }

static int validProjectileReference(const struct EntityCatalog *catalog,
                                    const char *id)
    {
    const struct EntityArchetype *projectile;

    if (id[0] == '\0')
        return 1;
    projectile = entityCatalogFind(catalog, id);
    return projectile != NULL && projectile->kind == ENTITY_KIND_PROJECTILE;
    }

static int validEffectReference(const struct EntityCatalog *catalog,
                                const char *id)
    {
    const struct EntityArchetype *effect;

    if (id[0] == '\0')
        return 1;
    effect = entityCatalogFind(catalog, id);
    return effect != NULL && effect->kind == ENTITY_KIND_EFFECT;
    }

int entityCatalogLoad(const char *assetRoot, const char *catalogPath,
                      struct EntityCatalog *catalog,
                      char *error, size_t errorSize)
    {
    FILE *file;
    char fullPath[ENTITY_PATH_SIZE];
    char descriptionPath[ENTITY_PATH_SIZE];
    char line[512];
    char directive[64];
    char value[256];
    char extra[2];
    int fields;
    int header;
    size_t index;
    const struct EntityArchetype *projectile;
    struct EntityArchetype entry;
    struct EntityCatalog loaded;

    if (catalog == NULL ||
        !buildPath(fullPath, sizeof(fullPath), assetRoot, catalogPath))
        {
        setEntityError(error, errorSize, "Invalid entity catalog path");
        return 0;
        }
    file = fopen(fullPath, "r");
    if (file == NULL)
        {
        setEntityError(error, errorSize, "Could not open entity catalog");
        return 0;
        }
    entityCatalogInitialize(&loaded);
    header = 0;
    while (fgets(line, sizeof(line), file) != NULL)
        {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        fields = sscanf(line, "%63s %255s %1s", directive, value, extra);
        if (fields != 2)
            goto invalid;
        if (!header)
            {
            if (strcmp(directive, "battalion-entities") != 0 ||
                strcmp(value, "1") != 0)
                goto invalid;
            header = 1;
            continue;
            }
        if (strcmp(directive, "fallback") == 0)
            {
            if (loaded.fallback[0] != '\0' ||
                !copyName(loaded.fallback, value))
                goto invalid;
            }
        else if (strcmp(directive, "entity") == 0)
            {
            if (!buildPath(descriptionPath, sizeof(descriptionPath),
                           assetRoot, value) ||
                !loadArchetype(descriptionPath, &entry, error, errorSize) ||
                !appendArchetype(&loaded, &entry, error, errorSize))
                goto failed;
            }
        else
            goto invalid;
        }
    fclose(file);
    if (!header || loaded.count == 0 ||
        (loaded.fallback[0] != '\0' &&
         entityCatalogFind(&loaded, loaded.fallback) == NULL))
        {
        setEntityError(error, errorSize, "Entity catalog is empty or invalid");
        entityCatalogDestroy(&loaded);
        return 0;
        }
    for (index = 0; index < loaded.count; index++)
        {
        if (loaded.entries[index].weapon == ENTITY_WEAPON_PROJECTILE)
            {
            projectile = entityCatalogFind(
                &loaded, loaded.entries[index].projectile);
            if (projectile == NULL ||
                projectile->kind != ENTITY_KIND_PROJECTILE)
                {
                setEntityError(error, errorSize,
                               "Weapon references an invalid projectile");
                entityCatalogDestroy(&loaded);
                return 0;
                }
            }
        if (!validProjectileReference(
                &loaded, loaded.entries[index].impactProjectile) ||
            !validProjectileReference(
                &loaded, loaded.entries[index].wreckFireProjectile) ||
            !validProjectileReference(
                &loaded, loaded.entries[index].emitterProjectile) ||
            !validProjectileReference(
                &loaded, loaded.entries[index].destructionProjectile) ||
            !validProjectileReference(
                &loaded, loaded.entries[index].destructionFireProjectile))
            {
            setEntityError(error, errorSize,
                           "Archetype references an invalid effect projectile");
            entityCatalogDestroy(&loaded);
            return 0;
            }
        if (!validEffectReference(&loaded, loaded.entries[index].impactEffect) ||
            !validEffectReference(
                &loaded, loaded.entries[index].destructionEffect))
            {
            setEntityError(error, errorSize,
                           "Archetype references an invalid runtime effect");
            entityCatalogDestroy(&loaded);
            return 0;
            }
        }
    entityCatalogDestroy(catalog);
    *catalog = loaded;
    return 1;

invalid:
    setEntityError(error, errorSize, "Invalid entity catalog syntax");
failed:
    fclose(file);
    entityCatalogDestroy(&loaded);
    return 0;
    }
