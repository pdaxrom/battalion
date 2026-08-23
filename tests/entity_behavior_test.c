/*
 * Copyright (c) 2026 Battalion contributors.
 */

#include "entity_behavior.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int nearValue(float left, float right)
    {
    return fabs((double)(left - right)) < 0.0001;
    }

static void contact(struct EntityContact *value, const char *id,
                    float x, float z, int visible, int audible)
    {
    memset(value, 0, sizeof(*value));
    strcpy(value->id, id);
    strcpy(value->faction, "monster");
    value->x = x;
    value->z = z;
    value->active = 1;
    value->visible = visible;
    value->audible = audible;
    }

int main(void)
    {
    struct EntityCatalog catalog;
    const struct EntityArchetype *tank;
    struct EntityWaypoint path[2];
    struct EntityAgent guard;
    struct EntityAgent receiver;
    struct EntityAgent phaseAgent;
    struct EntityAgent damageAgent;
    struct EntityArchetype phaseArchetype;
    struct EntityArchetype bodyFacingArchetype;
    struct EntityArchetype flybyArchetype;
    struct EntityAgent flybyAgent;
    struct EntityContact contacts[2];
    char error[256];
    int index;
    float aimX;
    float aimY;
    float aimZ;

    if (entitySteeredHeadingDelta(1, 0, 8.0f) != 8 ||
        entitySteeredHeadingDelta(0, 1, 8.0f) != -8 ||
        entitySteeredHeadingDelta(0, 0, 8.0f) != 0 ||
        entitySteeredHeadingDelta(1, 1, 8.0f) != 0)
        {
        fprintf(stderr, "steered player control direction failed\n");
        return 1;
        }
    entityAimDirection(0, 0, 0.08f, &aimX, &aimY, &aimZ);
    if (!nearValue(aimX, 0.0f) || !nearValue(aimY, 0.0f) ||
        !nearValue(aimZ, 0.08f))
        {
        fprintf(stderr, "forward aim direction failed\n");
        return 1;
        }
    entityAimDirection(900, -30, 1.0f, &aimX, &aimY, &aimZ);
    if (aimX < 0.99f || aimY <= 0.0f || !nearValue(aimZ, 0.0f))
        {
        fprintf(stderr, "turned and elevated aim direction failed\n");
        return 1;
        }

    entityCatalogInitialize(&catalog);
    if (!entityCatalogLoad("assets", "entities/catalog.txt", &catalog,
                           error, sizeof(error)))
        {
        fprintf(stderr, "%s\n", error);
        return 1;
        }
    tank = entityCatalogFind(&catalog, "tank-standard");
    if (!nearValue(entityListenerHeadingRadians(tank, 900, 0),
                   3.14159265358979323846f * 0.5f))
        {
        fprintf(stderr, "turret listener orientation failed\n");
        return 1;
        }
    flybyArchetype = *tank;
    flybyArchetype.tactic = ENTITY_TACTIC_FLYBY;
    flybyArchetype.tacticRadius = 5.0f;
    flybyArchetype.speed = 1.0f;
    flybyArchetype.weaponRange = 10.0f;
    entityAgentInitialize(&flybyAgent, &flybyArchetype,
                          0.0f, 0.0f, 0.0f,
                          NULL, 0, ENTITY_PATROL_LOOP);
    contact(&contacts[0], "flyby-target", 0.0f, 2.0f, 1, 1);
    entityAgentStep(&flybyAgent, contacts, 1);
    if (flybyAgent.reaction != ENTITY_REACTION_ATTACK ||
        flybyAgent.z < 0.99f)
        {
        fprintf(stderr, "flyby tactic stopped inside weapon range\n");
        return 1;
        }
    bodyFacingArchetype = *tank;
    bodyFacingArchetype.aimYawNode[0] = '\0';
    bodyFacingArchetype.aimPitchNode[0] = '\0';
    if (!nearValue(entityListenerHeadingRadians(&bodyFacingArchetype,
                                                900, 0), 0.0f))
        {
        fprintf(stderr, "body listener orientation failed\n");
        return 1;
        }
    path[0].x = 0.0f;
    path[0].z = 0.0f;
    path[1].x = 2.0f;
    path[1].z = 0.0f;
    entityAgentInitialize(&guard, tank, 0.0f, 0.0f, 0.0f,
                          path, 2, ENTITY_PATROL_LOOP);
    for (index = 0; index < 5; index++)
        entityAgentStep(&guard, NULL, 0);
    if (guard.x <= 0.0f || guard.reaction != ENTITY_REACTION_OBSERVE)
        {
        fprintf(stderr, "patrol failed: x=%g reaction=%d\n",
                guard.x, guard.reaction);
        return 1;
        }

    contact(&contacts[0], "intruder-a", guard.x, guard.z + 1.0f, 1, 0);
    contact(&contacts[1], "intruder-b", guard.x + 1.0f, guard.z, 1, 1);
    entityAgentStep(&guard, contacts, 2);
    if (guard.reaction != ENTITY_REACTION_ATTACK ||
        guard.contactCount < 2 || guard.eventCount != 2 ||
        strcmp(guard.events[0].name, "base-alarm") != 0)
        {
        fprintf(stderr, "sensor failed: reaction=%d contacts=%d events=%lu\n",
                guard.reaction, guard.contactCount,
                (unsigned long)guard.eventCount);
        return 1;
        }

    entityAgentInitialize(&receiver, tank, 2.0f, 2.0f, 0.0f,
                          NULL, 0, ENTITY_PATROL_LOOP);
    receiver.active = 0;
    if (!entityAgentQueueTrigger(&receiver, guard.events[0].name,
                                 &guard.events[0].contact))
        {
        fprintf(stderr, "trigger queue failed\n");
        return 1;
        }
    entityAgentStep(&guard, contacts, 2);
    if (guard.eventCount != 0)
        {
        fprintf(stderr, "enter sensor repeated while contact stayed\n");
        return 1;
        }
    entityAgentStep(&receiver, NULL, 0);
    if (!receiver.active || receiver.reaction != ENTITY_REACTION_ATTACK ||
        !receiver.hasTarget ||
        strcmp(receiver.target.id, "intruder-a") != 0)
        {
        fprintf(stderr, "trigger failed: reaction=%d target=%s\n",
                receiver.reaction, receiver.target.id);
        return 1;
        }

    phaseArchetype = *tank;
    phaseArchetype.sensorCount = 1;
    strcpy(phaseArchetype.sensors[0].id, "test-zone");
    strcpy(phaseArchetype.sensors[0].factionFilter, "hostile");
    phaseArchetype.sensors[0].type = ENTITY_SENSOR_PROXIMITY;
    phaseArchetype.sensors[0].range = 3.0f;
    phaseArchetype.sensors[0].angle = 360.0f;
    phaseArchetype.targetMode = ENTITY_TARGET_NEAREST;
    phaseArchetype.reactionRuleCount = 4;
    strcpy(phaseArchetype.reactionRules[0].event, "test-zone");
    strcpy(phaseArchetype.reactionRules[0].emit, "zone-entered");
    phaseArchetype.reactionRules[0].source = ENTITY_RULE_SENSOR;
    phaseArchetype.reactionRules[0].phase = ENTITY_SENSOR_PHASE_ENTER;
    phaseArchetype.reactionRules[0].reaction = ENTITY_REACTION_ALERT;
    strcpy(phaseArchetype.reactionRules[1].event, "test-zone");
    strcpy(phaseArchetype.reactionRules[1].emit, "zone-stayed");
    phaseArchetype.reactionRules[1].source = ENTITY_RULE_SENSOR;
    phaseArchetype.reactionRules[1].phase = ENTITY_SENSOR_PHASE_STAY;
    phaseArchetype.reactionRules[1].reaction = ENTITY_REACTION_PURSUE;
    strcpy(phaseArchetype.reactionRules[2].event, "test-zone");
    strcpy(phaseArchetype.reactionRules[2].emit, "zone-exited");
    phaseArchetype.reactionRules[2].source = ENTITY_RULE_SENSOR;
    phaseArchetype.reactionRules[2].phase = ENTITY_SENSOR_PHASE_EXIT;
    phaseArchetype.reactionRules[2].reaction = ENTITY_REACTION_OBSERVE;
    strcpy(phaseArchetype.reactionRules[3].event, "shutdown");
    strcpy(phaseArchetype.reactionRules[3].emit, "none");
    phaseArchetype.reactionRules[3].source = ENTITY_RULE_TRIGGER;
    phaseArchetype.reactionRules[3].phase = ENTITY_SENSOR_PHASE_ANY;
    phaseArchetype.reactionRules[3].reaction = ENTITY_REACTION_DEACTIVATE;
    entityAgentInitialize(&phaseAgent, &phaseArchetype, 0.0f, 0.0f, 0.0f,
                          NULL, 0, ENTITY_PATROL_LOOP);
    contact(&contacts[0], "phase-contact", 0.0f, 1.0f, 1, 1);
    entityAgentStep(&phaseAgent, contacts, 1);
    if (phaseAgent.eventCount != 1 ||
        strcmp(phaseAgent.events[0].name, "zone-entered") != 0)
        {
        fprintf(stderr, "sensor enter phase failed\n");
        return 1;
        }
    entityAgentStep(&phaseAgent, contacts, 1);
    if (phaseAgent.eventCount != 1 ||
        strcmp(phaseAgent.events[0].name, "zone-stayed") != 0)
        {
        fprintf(stderr, "sensor stay phase failed\n");
        return 1;
        }
    entityAgentStep(&phaseAgent, NULL, 0);
    if (phaseAgent.eventCount != 1 ||
        strcmp(phaseAgent.events[0].name, "zone-exited") != 0 ||
        phaseAgent.events[0].contact.active)
        {
        fprintf(stderr, "sensor exit phase failed\n");
        return 1;
        }
    if (!entityAgentQueueTrigger(&phaseAgent, "shutdown", &contacts[0]))
        {
        fprintf(stderr, "deactivation trigger queue failed\n");
        return 1;
        }
    entityAgentStep(&phaseAgent, contacts, 1);
    if (phaseAgent.active || phaseAgent.hasTarget ||
        phaseAgent.reaction != ENTITY_REACTION_OBSERVE)
        {
        fprintf(stderr, "deactivation trigger failed\n");
        return 1;
        }

    entityAgentInitialize(&damageAgent, tank, 0.0f, 0.0f, 0.0f,
                          NULL, 0, ENTITY_PATROL_LOOP);
    contact(&contacts[0], "real-attacker", 2.0f, 1.0f, 1, 1);
    entityAgentNotifyDamage(&damageAgent, &contacts[0]);
    entityAgentStep(&damageAgent, NULL, 0);
    if (damageAgent.eventCount != 1 || !damageAgent.hasTarget ||
        strcmp(damageAgent.events[0].name, "base-alarm") != 0 ||
        strcmp(damageAgent.events[0].contact.id, "real-attacker") != 0 ||
        strcmp(damageAgent.target.id, "real-attacker") != 0)
        {
        fprintf(stderr, "damage source contact was not preserved\n");
        return 1;
        }

    entityCatalogDestroy(&catalog);
    return 0;
    }
