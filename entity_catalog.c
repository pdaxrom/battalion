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
    if (strcmp(text, "solid") == 0)
        return ENTITY_COLLISION_SOLID;
    if (strcmp(text, "phase") == 0)
        return ENTITY_COLLISION_PHASE;
    return 0;
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
    if (strcmp(text, "shell") == 0)
        return ENTITY_WEAPON_SHELL;
    if (strcmp(text, "rocket") == 0)
        return ENTITY_WEAPON_ROCKET;
    if (strcmp(text, "beam") == 0)
        return ENTITY_WEAPON_BEAM;
    if (strcmp(text, "fire") == 0)
        return ENTITY_WEAPON_FIRE;
    if (strcmp(text, "plasma") == 0)
        return ENTITY_WEAPON_PLASMA;
    return -1;
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
    entry->controlMode = ENTITY_CONTROL_FREE;
    entry->weapon = ENTITY_WEAPON_NONE;
    entry->networkCode = -1;
    strcpy(entry->weaponColor, "blue");
    entry->targetMode = ENTITY_TARGET_NEAREST;
    entry->maxTargets = 1;
    entry->health = 1;
    entry->score = 10;
    entry->fireInterval = 120;
    entry->lostFrames = 180;
    entry->maxActive = 0;
    entry->scale = 1.0f;
    entry->modelYawOffset = 0.0f;
    entry->speed = 0.03f;
    entry->turnRate = 0.2f;
    entry->collisionRadius = 0.25f;
    entry->collisionHeight = 0.3f;
    entry->weaponRange = 6.0f;
    entry->weaponMinRange = 0.5f;
    entry->projectileSpeed = 0.08f;
    entry->accuracy = 10.0f;
    entry->forgetRange = 12.0f;
    entry->animationSpeed = 0.05f;
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
    entry->muzzleForward = 0.0f;
    entry->muzzleHeight = 0.0f;
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

const struct EntityArchetype *entityCatalogFindNetworkCode(
    const struct EntityCatalog *catalog, int networkCode)
    {
    size_t index;

    if (catalog == NULL || networkCode < 0)
        return NULL;
    for (index = 0; index < catalog->count; index++)
        if (catalog->entries[index].networkCode == networkCode)
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

int entityArchetypeIsHostileTo(const struct EntityArchetype *archetype,
                               const char *faction)
    {
    size_t index;

    if (archetype == NULL || faction == NULL)
        return 0;
    for (index = 0; index < archetype->hostileFactionCount; index++)
        if (strcmp(archetype->hostileFactions[index], faction) == 0)
            return 1;
    return 0;
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
    if (strcmp(directive, "destroyed-model") == 0)
        return entry->destroyedModel[0] == '\0' &&
               copyName(entry->destroyedModel, value);
    if (strcmp(directive, "shadow-model") == 0)
        return entry->shadowModel[0] == '\0' &&
               copyName(entry->shadowModel, value);
    if (strcmp(directive, "idle-animation") == 0)
        return entry->idleAnimation[0] == '\0' &&
               copyName(entry->idleAnimation, value);
    if (strcmp(directive, "move-animation") == 0)
        return entry->moveAnimation[0] == '\0' &&
               copyName(entry->moveAnimation, value);
    if (strcmp(directive, "attack-animation") == 0)
        return entry->attackAnimation[0] == '\0' &&
               copyName(entry->attackAnimation, value);
    if (strcmp(directive, "aim-yaw-node") == 0)
        return entry->aimYawNode[0] == '\0' &&
               copyName(entry->aimYawNode, value);
    if (strcmp(directive, "aim-pitch-node") == 0)
        return entry->aimPitchNode[0] == '\0' &&
               copyName(entry->aimPitchNode, value);
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
    if (strcmp(directive, "hostile-faction") == 0)
        {
        if (entry->hostileFactionCount >= ENTITY_MAX_HOSTILE_FACTIONS ||
            !copyName(entry->hostileFactions[entry->hostileFactionCount],
                      value))
            return 0;
        entry->hostileFactionCount++;
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
    if (strcmp(directive, "collision-mode") == 0)
        {
        parsed = collisionModeValue(value);
        if (parsed == 0)
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
    if (strcmp(directive, "weapon-color") == 0)
        return copyName(entry->weaponColor, value);
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
    if (strcmp(directive, "network-code") == 0)
        return entry->networkCode == -1 &&
               parseInteger(value, &entry->networkCode) &&
               entry->networkCode >= 0;
    if (strcmp(directive, "scale") == 0)
        return parseFloat(value, &entry->scale);
    if (strcmp(directive, "model-yaw-offset") == 0)
        return parseFloat(value, &entry->modelYawOffset);
    if (strcmp(directive, "vertical-offset") == 0)
        return parseFloat(value, &entry->verticalOffset);
    if (strcmp(directive, "speed") == 0)
        return parseFloat(value, &entry->speed);
    if (strcmp(directive, "turn-rate") == 0)
        return parseFloat(value, &entry->turnRate);
    if (strcmp(directive, "collision-radius") == 0)
        return parseFloat(value, &entry->collisionRadius);
    if (strcmp(directive, "collision-height") == 0)
        return parseFloat(value, &entry->collisionHeight);
    if (strcmp(directive, "weapon-range") == 0)
        return parseFloat(value, &entry->weaponRange);
    if (strcmp(directive, "weapon-min-range") == 0)
        return parseFloat(value, &entry->weaponMinRange);
    if (strcmp(directive, "projectile-speed") == 0)
        return parseFloat(value, &entry->projectileSpeed);
    if (strcmp(directive, "accuracy") == 0)
        return parseFloat(value, &entry->accuracy);
    if (strcmp(directive, "forget-range") == 0)
        return parseFloat(value, &entry->forgetRange);
    if (strcmp(directive, "animation-speed") == 0)
        return parseFloat(value, &entry->animationSpeed);
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
    if (strcmp(directive, "muzzle-forward") == 0)
        return parseFloat(value, &entry->muzzleForward);
    if (strcmp(directive, "muzzle-height") == 0)
        return parseFloat(value, &entry->muzzleHeight);
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

static int validArchetype(const struct EntityArchetype *entry,
                          int hasId, int hasKind, int hasModel,
                          int hasFaction)
    {
    if (!hasId || !hasKind || !hasModel || !hasFaction)
        return 0;
    if (entry->maxTargets < 1 ||
        entry->maxTargets > ENTITY_MAX_HOSTILE_FACTIONS ||
        entry->health < 1 || entry->score < 0 || entry->fireInterval < 1 ||
        entry->lostFrames < 0 || entry->spawnWeight < 0 ||
        entry->spawnAfter < 0 || entry->maxActive < 0 ||
        entry->scale <= 0.0f || entry->modelYawOffset < -360.0f ||
        entry->modelYawOffset > 360.0f || entry->speed < 0.0f ||
        entry->turnRate <= 0.0f || entry->collisionRadius <= 0.0f ||
        entry->collisionHeight <= 0.0f || entry->weaponRange < 0.0f ||
        entry->weaponMinRange < 0.0f ||
        entry->weaponMinRange > entry->weaponRange ||
        entry->projectileSpeed <= 0.0f || entry->accuracy < 0.0f ||
        entry->forgetRange < 0.0f || entry->animationSpeed <= 0.0f ||
        (entry->controllable &&
         (entry->playerEnergy <= 0.0f || entry->playerHeight <= 0.0f ||
          entry->playerWidth <= 0.0f || entry->playerRegen < 0.0f ||
          entry->playerMoveCost < 0.0f || entry->playerFireCost < 0.0f ||
          entry->playerTurnRate <= 0.0f ||
          entry->playerSteerRate <= 0.0f ||
          entry->aimPitchMin >= entry->aimPitchMax ||
          entry->aimPitchDefault < entry->aimPitchMin ||
          entry->aimPitchDefault > entry->aimPitchMax ||
          entry->firstPersonEyeHeight <= 0.0f ||
          (entry->weapon != ENTITY_WEAPON_NONE &&
           (entry->muzzleForward < 0.0f || entry->muzzleHeight <= 0.0f)))))
        return 0;
    if ((entry->aimYawNode[0] == '\0') !=
        (entry->aimPitchNode[0] == '\0'))
        return 0;
    if (entry->reactionRuleCount > 0 && entry->hostileFactionCount == 0)
        {
        size_t sensorIndex;
        int needsHostile;

        needsHostile = 0;
        for (sensorIndex = 0; sensorIndex < entry->sensorCount; sensorIndex++)
            if (strcmp(entry->sensors[sensorIndex].factionFilter,
                       "hostile") == 0)
                needsHostile = 1;
        if (needsHostile)
            return 0;
        }
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
        if (strcmp(directive, "sensor") == 0)
            {
            if (fields != 6 ||
                !addSensor(entry, value, argument2, argument3, argument4,
                           argument5))
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
    if (entry->networkCode >= 0)
        for (index = 0; index < catalog->count; index++)
            if (catalog->entries[index].networkCode == entry->networkCode)
                {
                setEntityError(error, errorSize,
                               "Duplicate entity network code");
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
