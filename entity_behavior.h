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

#ifndef BATTALION_ENTITY_BEHAVIOR_H
#define BATTALION_ENTITY_BEHAVIOR_H

#include "entity_catalog.h"

#include <stddef.h>

#define ENTITY_MAX_CONTACTS 8
#define ENTITY_MAX_EVENTS 16
#define ENTITY_MAX_PENDING_TRIGGERS 8
#define ENTITY_MAX_SENSOR_DETECTIONS \
    (ENTITY_MAX_SENSORS * ENTITY_MAX_CONTACTS)

#define ENTITY_PATROL_LOOP 1
#define ENTITY_PATROL_PINGPONG 2
#define ENTITY_PATROL_ONCE 3

struct EntityWaypoint
    {
    float x;
    float z;
    };

struct EntityContact
    {
    char id[ENTITY_NAME_SIZE];
    char faction[ENTITY_NAME_SIZE];
    float x;
    float z;
    int active;
    int visible;
    int audible;
    };

struct EntityEvent
    {
    char name[ENTITY_NAME_SIZE];
    struct EntityContact contact;
    };

struct EntityPendingTrigger
    {
    char name[ENTITY_NAME_SIZE];
    struct EntityContact contact;
    };

struct EntitySensorDetection
    {
    int sensorIndex;
    struct EntityContact contact;
    };

struct EntityAgent
    {
    const struct EntityArchetype *archetype;
    const struct EntityWaypoint *waypoints;
    size_t waypointCount;
    size_t waypointIndex;
    int waypointDirection;
    int patrolMode;
    int reaction;
    int active;
    int cooldown;
    int lostCounter;
    int contactCount;
    int fireRequested;
    int damaged;
    float x;
    float z;
    float heading;
    float homeX;
    float homeZ;
    float targetX;
    float targetZ;
    int hasTarget;
    struct EntityContact target;
    struct EntityContact contacts[ENTITY_MAX_CONTACTS];
    struct EntityEvent events[ENTITY_MAX_EVENTS];
    size_t eventCount;
    struct EntityPendingTrigger pending[ENTITY_MAX_PENDING_TRIGGERS];
    size_t pendingCount;
    struct EntitySensorDetection previousDetections[
        ENTITY_MAX_SENSOR_DETECTIONS];
    size_t previousDetectionCount;
    };

void entityAgentInitialize(struct EntityAgent *agent,
                           const struct EntityArchetype *archetype,
                           float x, float z, float heading,
                           const struct EntityWaypoint *waypoints,
                           size_t waypointCount, int patrolMode);
int entityAgentQueueTrigger(struct EntityAgent *agent, const char *event,
                            const struct EntityContact *contact);
void entityAgentStep(struct EntityAgent *agent,
                     const struct EntityContact *contacts,
                     size_t contactCount);
int entitySteeredHeadingDelta(int leftPressed, int rightPressed,
                              float steerRate);
void entityAimDirection(int horizontalTenths, int verticalTenths,
                        float length, float *x, float *y, float *z);

#endif
