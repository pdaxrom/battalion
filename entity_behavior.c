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
/* Generic sensor, trigger, reaction, and patrol controller.           */
/***********************************************************************/

#include "entity_behavior.h"

#include <math.h>
#include <string.h>

#define ENTITY_PI 3.14159265358979323846
#define ENTITY_TWO_PI (2.0 * ENTITY_PI)

static float distanceSquared(float x1, float z1, float x2, float z2)
    {
    float x;
    float z;

    x = x1 - x2;
    z = z1 - z2;
    return x * x + z * z;
    }

static float normalizeAngle(float angle)
    {
    while (angle > (float)ENTITY_PI)
        angle -= (float)ENTITY_TWO_PI;
    while (angle < (float)-ENTITY_PI)
        angle += (float)ENTITY_TWO_PI;
    return angle;
    }

int entitySteeredHeadingDelta(int leftPressed, int rightPressed,
                              float steerRate)
    {
    return (int)((leftPressed - rightPressed) * steerRate);
    }

void entityAimDirection(int horizontalTenths, int verticalTenths,
                        float length, float *x, float *y, float *z)
    {
    double horizontal;
    double vertical;
    float horizontalLength;

    horizontal = horizontalTenths * ENTITY_PI / 1800.0;
    vertical = verticalTenths * ENTITY_PI / 1800.0;
    horizontalLength = length * (float)cos(vertical);
    *x = horizontalLength * (float)sin(horizontal);
    *y = -length * (float)sin(vertical);
    *z = horizontalLength * (float)cos(horizontal);
    }

static int factionMatches(const struct EntityArchetype *archetype,
                          const struct EntitySensor *sensor,
                          const struct EntityContact *contact)
    {
    if (strcmp(sensor->factionFilter, "any") == 0)
        return 1;
    if (strcmp(sensor->factionFilter, "hostile") == 0)
        return entityArchetypeIsHostileTo(archetype, contact->faction);
    return strcmp(sensor->factionFilter, contact->faction) == 0;
    }

static int sensorDetects(const struct EntityAgent *agent,
                         const struct EntitySensor *sensor,
                         const struct EntityContact *contact)
    {
    float squared;
    float bearing;
    float difference;
    float halfAngle;

    if (!contact->active || !factionMatches(agent->archetype, sensor, contact))
        return 0;
    squared = distanceSquared(agent->x, agent->z, contact->x, contact->z);
    if (squared > sensor->range * sensor->range)
        return 0;
    if (sensor->type == ENTITY_SENSOR_PROXIMITY)
        return 1;
    if (sensor->type == ENTITY_SENSOR_HEARING)
        return contact->audible;
    if (sensor->type == ENTITY_SENSOR_VISION)
        {
        if (!contact->visible)
            return 0;
        bearing = (float)atan2((double)(contact->x - agent->x),
                               (double)(contact->z - agent->z));
        difference = normalizeAngle(bearing - agent->heading);
        halfAngle = sensor->angle * (float)ENTITY_PI / 360.0f;
        return (float)fabs((double)difference) <= halfAngle;
        }
    return 0;
    }

static void addEvent(struct EntityAgent *agent, const char *name,
                     const struct EntityContact *contact)
    {
    size_t index;
    struct EntityEvent *event;

    if (strcmp(name, "none") == 0 || agent->eventCount >= ENTITY_MAX_EVENTS)
        return;
    for (index = 0; index < agent->eventCount; index++)
        if (strcmp(agent->events[index].name, name) == 0 &&
            strcmp(agent->events[index].contact.id, contact->id) == 0)
            return;
    event = &agent->events[agent->eventCount++];
    strcpy(event->name, name);
    event->contact = *contact;
    }

static void applyReaction(struct EntityAgent *agent,
                          const struct EntityReactionRule *rule,
                          const struct EntityContact *contact)
    {
    int replaceReaction;

    replaceReaction = rule->reaction >= agent->reaction;
    if (rule->reaction == ENTITY_REACTION_ACTIVATE ||
        rule->reaction == ENTITY_REACTION_DEACTIVATE)
        replaceReaction = 1;
    if (replaceReaction)
        agent->reaction = rule->reaction;
    if (rule->reaction == ENTITY_REACTION_ACTIVATE)
        agent->active = 1;
    else if (rule->reaction == ENTITY_REACTION_DEACTIVATE)
        agent->active = 0;
    if (contact != NULL && contact->active)
        {
        agent->target = *contact;
        agent->targetX = contact->x;
        agent->targetZ = contact->z;
        agent->hasTarget = 1;
        }
    if (contact != NULL)
        addEvent(agent, rule->emit, contact);
    }

static void addDetectedContact(struct EntityAgent *agent,
                               const struct EntityContact *contact)
    {
    int index;

    for (index = 0; index < agent->contactCount; index++)
        if (strcmp(agent->contacts[index].id, contact->id) == 0)
            return;
    if (agent->contactCount >= agent->archetype->maxTargets ||
        agent->contactCount >= ENTITY_MAX_CONTACTS)
        return;
    agent->contacts[agent->contactCount++] = *contact;
    }

static const struct EntityReactionRule *findRule(
    const struct EntityArchetype *archetype, int source, const char *event)
    {
    size_t index;

    for (index = 0; index < archetype->reactionRuleCount; index++)
        if (archetype->reactionRules[index].source == source &&
            strcmp(archetype->reactionRules[index].event, event) == 0)
            return &archetype->reactionRules[index];
    return NULL;
    }

void entityAgentInitialize(struct EntityAgent *agent,
                           const struct EntityArchetype *archetype,
                           float x, float z, float heading,
                           const struct EntityWaypoint *waypoints,
                           size_t waypointCount, int patrolMode)
    {
    memset(agent, 0, sizeof(*agent));
    agent->archetype = archetype;
    agent->waypoints = waypoints;
    agent->waypointCount = waypointCount;
    agent->waypointDirection = 1;
    agent->patrolMode = patrolMode;
    agent->reaction = ENTITY_REACTION_OBSERVE;
    agent->active = 1;
    agent->x = agent->homeX = x;
    agent->z = agent->homeZ = z;
    agent->heading = heading;
    }

int entityAgentQueueTrigger(struct EntityAgent *agent, const char *event,
                            const struct EntityContact *contact)
    {
    struct EntityPendingTrigger *pending;

    if (agent == NULL || event == NULL || contact == NULL ||
        agent->pendingCount >= ENTITY_MAX_PENDING_TRIGGERS ||
        strlen(event) >= ENTITY_NAME_SIZE)
        return 0;
    pending = &agent->pending[agent->pendingCount++];
    strcpy(pending->name, event);
    pending->contact = *contact;
    return 1;
    }

static void processTriggers(struct EntityAgent *agent)
    {
    size_t index;
    const struct EntityReactionRule *rule;

    for (index = 0; index < agent->pendingCount; index++)
        {
        rule = findRule(agent->archetype, ENTITY_RULE_TRIGGER,
                        agent->pending[index].name);
        if (rule != NULL)
            applyReaction(agent, rule, &agent->pending[index].contact);
        }
    agent->pendingCount = 0;
    }

static int processSensors(struct EntityAgent *agent,
                          const struct EntityContact *contacts,
                          size_t contactCount)
    {
    const struct EntitySensor *sensor;
    const struct EntityReactionRule *rule;
    const struct EntityContact *nearest;
    const struct EntityContact *sensorNearest;
    size_t sensorIndex;
    size_t contactIndex;
    float nearestDistance;
    float sensorNearestDistance;
    float squared;
    int detected;

    detected = 0;
    nearest = NULL;
    nearestDistance = 0.0f;
    for (sensorIndex = 0; sensorIndex < agent->archetype->sensorCount;
         sensorIndex++)
        {
        sensor = &agent->archetype->sensors[sensorIndex];
        sensorNearest = NULL;
        sensorNearestDistance = 0.0f;
        if (sensor->type == ENTITY_SENSOR_DAMAGE)
            {
            if (agent->damaged)
                {
                rule = findRule(agent->archetype, ENTITY_RULE_SENSOR,
                                sensor->id);
                if (rule != NULL)
                    {
                    struct EntityContact damageContact;
                    memset(&damageContact, 0, sizeof(damageContact));
                    strcpy(damageContact.id, "damage-source");
                    strcpy(damageContact.faction, "hostile");
                    damageContact.x = agent->targetX;
                    damageContact.z = agent->targetZ;
                    damageContact.active = 1;
                    applyReaction(agent, rule, &damageContact);
                    }
                detected = 1;
                }
            continue;
            }
        rule = findRule(agent->archetype, ENTITY_RULE_SENSOR, sensor->id);
        for (contactIndex = 0; contactIndex < contactCount; contactIndex++)
            if (sensorDetects(agent, sensor, &contacts[contactIndex]))
                {
                squared = distanceSquared(agent->x, agent->z,
                                          contacts[contactIndex].x,
                                          contacts[contactIndex].z);
                if (nearest == NULL || squared < nearestDistance)
                    {
                    nearest = &contacts[contactIndex];
                    nearestDistance = squared;
                    }
                if (sensorNearest == NULL || squared < sensorNearestDistance)
                    {
                    sensorNearest = &contacts[contactIndex];
                    sensorNearestDistance = squared;
                    }
                detected = 1;
                addDetectedContact(agent, &contacts[contactIndex]);
                if (rule != NULL &&
                    agent->archetype->targetMode == ENTITY_TARGET_ALL)
                    applyReaction(agent, rule, &contacts[contactIndex]);
                }
        if (rule != NULL && sensorNearest != NULL &&
            agent->archetype->targetMode == ENTITY_TARGET_NEAREST)
            applyReaction(agent, rule, sensorNearest);
        }
    if (nearest != NULL)
        {
        agent->target = *nearest;
        agent->targetX = nearest->x;
        agent->targetZ = nearest->z;
        agent->hasTarget = 1;
        }
    agent->damaged = 0;
    return detected;
    }

static void advanceWaypoint(struct EntityAgent *agent)
    {
    if (agent->waypointCount < 2)
        return;
    if (agent->patrolMode == ENTITY_PATROL_LOOP)
        agent->waypointIndex = (agent->waypointIndex + 1) %
                               agent->waypointCount;
    else if (agent->patrolMode == ENTITY_PATROL_PINGPONG)
        {
        if (agent->waypointDirection > 0 &&
            agent->waypointIndex + 1 >= agent->waypointCount)
            agent->waypointDirection = -1;
        else if (agent->waypointDirection < 0 && agent->waypointIndex == 0)
            agent->waypointDirection = 1;
        agent->waypointIndex = (size_t)((int)agent->waypointIndex +
                                       agent->waypointDirection);
        }
    else if (agent->patrolMode == ENTITY_PATROL_ONCE &&
             agent->waypointIndex + 1 < agent->waypointCount)
        agent->waypointIndex++;
    }

static void desiredDestination(struct EntityAgent *agent,
                               float *targetX, float *targetZ, int *move)
    {
    *move = 0;
    if (agent->hasTarget &&
        (agent->reaction == ENTITY_REACTION_INVESTIGATE ||
         agent->reaction == ENTITY_REACTION_PURSUE ||
         agent->reaction == ENTITY_REACTION_ATTACK))
        {
        *targetX = agent->targetX;
        *targetZ = agent->targetZ;
        *move = 1;
        }
    else if (agent->hasTarget && agent->reaction == ENTITY_REACTION_FLEE)
        {
        *targetX = agent->x + (agent->x - agent->targetX);
        *targetZ = agent->z + (agent->z - agent->targetZ);
        *move = 1;
        }
    else if (agent->waypointCount > 0)
        {
        *targetX = agent->waypoints[agent->waypointIndex].x;
        *targetZ = agent->waypoints[agent->waypointIndex].z;
        *move = 1;
        }
    }

static void moveAgent(struct EntityAgent *agent)
    {
    float destinationX;
    float destinationZ;
    float squared;
    float desiredHeading;
    float difference;
    float distance;
    float speed;
    int move;

    if (!agent->active ||
        agent->archetype->movement == ENTITY_MOVEMENT_STATIONARY)
        return;
    desiredDestination(agent, &destinationX, &destinationZ, &move);
    if (!move)
        return;
    squared = distanceSquared(agent->x, agent->z,
                              destinationX, destinationZ);
    if (!agent->hasTarget && squared <=
        agent->archetype->speed * agent->archetype->speed)
        {
        agent->x = destinationX;
        agent->z = destinationZ;
        advanceWaypoint(agent);
        return;
        }
    if (squared <= 0.000001f)
        return;
    if (agent->reaction == ENTITY_REACTION_ATTACK && agent->hasTarget &&
        squared <= agent->archetype->weaponRange *
                   agent->archetype->weaponRange &&
        squared >= agent->archetype->weaponMinRange *
                   agent->archetype->weaponMinRange)
        return;
    desiredHeading = (float)atan2((double)(destinationX - agent->x),
                                  (double)(destinationZ - agent->z));
    difference = normalizeAngle(desiredHeading - agent->heading);
    agent->heading = normalizeAngle(agent->heading +
        difference * agent->archetype->turnRate);
    distance = (float)sqrt((double)squared);
    speed = agent->archetype->speed;
    if (speed > distance)
        speed = distance;
    agent->x += (float)sin((double)agent->heading) * speed;
    agent->z += (float)cos((double)agent->heading) * speed;
    }

static void updateWeapon(struct EntityAgent *agent)
    {
    float squared;

    if (agent->cooldown > 0)
        agent->cooldown--;
    if (!agent->active || agent->reaction != ENTITY_REACTION_ATTACK ||
        !agent->hasTarget || agent->archetype->weapon == ENTITY_WEAPON_NONE)
        return;
    squared = distanceSquared(agent->x, agent->z,
                              agent->targetX, agent->targetZ);
    if (squared <= agent->archetype->weaponRange *
                   agent->archetype->weaponRange &&
        squared >= agent->archetype->weaponMinRange *
                   agent->archetype->weaponMinRange &&
        agent->cooldown == 0)
        {
        agent->fireRequested = 1;
        agent->cooldown = agent->archetype->fireInterval;
        }
    }

void entityAgentStep(struct EntityAgent *agent,
                     const struct EntityContact *contacts,
                     size_t contactCount)
    {
    int detected;

    if (agent == NULL || agent->archetype == NULL)
        return;
    agent->eventCount = 0;
    agent->contactCount = 0;
    agent->fireRequested = 0;
    processTriggers(agent);
    if (!agent->active)
        {
        agent->damaged = 0;
        return;
        }
    detected = processSensors(agent, contacts, contactCount);
    if (detected)
        agent->lostCounter = 0;
    else if (agent->hasTarget)
        {
        agent->lostCounter++;
        if (agent->lostCounter > agent->archetype->lostFrames)
            {
            agent->hasTarget = 0;
            agent->reaction = ENTITY_REACTION_OBSERVE;
            agent->lostCounter = 0;
            }
        }
    moveAgent(agent);
    updateWeapon(agent);
    }
