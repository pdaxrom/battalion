/*
 * Copyright (c) 2026 Battalion contributors.
 */
/* Versioned, renderer-neutral battlefield map packages.               */
/***********************************************************************/

#ifndef BATTALION_MAP_PACKAGE_H
#define BATTALION_MAP_PACKAGE_H

#include "entity_behavior.h"

#include <stddef.h>

struct BattalionMapRoad
    {
    float x;
    float z;
    int type;
    };

struct BattalionMapStructure
    {
    float x;
    float z;
    int type;
    int shape;
    };

#define BATTALION_MAP_TRIGGER_ID 1
#define BATTALION_MAP_TRIGGER_GROUP 2

struct BattalionMapTrigger
    {
    char event[ENTITY_NAME_SIZE];
    char target[ENTITY_NAME_SIZE];
    int targetType;
    };

struct BattalionMapActor
    {
    char id[ENTITY_NAME_SIZE];
    char archetype[ENTITY_NAME_SIZE];
    char group[ENTITY_NAME_SIZE];
    float x;
    float z;
    float heading;
    int active;
    int patrolMode;
    struct EntityWaypoint *waypoints;
    size_t waypointCount;
    struct BattalionMapTrigger *triggers;
    size_t triggerCount;
    };

struct BattalionMapPackage
    {
    int version;
    char id[32];
    char name[128];
    struct BattalionMapRoad *roads;
    size_t roadCount;
    struct BattalionMapStructure *structures;
    size_t structureCount;
    struct BattalionMapActor *actors;
    size_t actorCount;
    };

void battalionMapPackageInit(struct BattalionMapPackage *map);
void battalionMapPackageFree(struct BattalionMapPackage *map);

int battalionMapPackageLoad(const char *path,
                            struct BattalionMapPackage *map,
                            char *error, size_t errorSize);
int battalionMapPackageWrite(const char *path,
                             const struct BattalionMapPackage *map,
                             char *error, size_t errorSize);
int battalionMapPackageValidate(const struct BattalionMapPackage *map,
                                char *error, size_t errorSize);

#endif
