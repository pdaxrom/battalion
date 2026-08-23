/*
 * Copyright (c) 2026 Battalion contributors.
 */
/* Versioned, renderer-neutral battlefield map packages.               */
/***********************************************************************/

#ifndef BATTALION_MAP_PACKAGE_H
#define BATTALION_MAP_PACKAGE_H

#include "entity_behavior.h"

#include <stddef.h>

#define BATTALION_MAP_DESCRIPTION_SIZE 256
#define BATTALION_MAP_PROPERTY_VALUE_SIZE 128

struct BattalionMapPoint
    {
    float x;
    float z;
    };

struct BattalionMapProperty
    {
    char name[ENTITY_NAME_SIZE];
    char value[BATTALION_MAP_PROPERTY_VALUE_SIZE];
    };

#define BATTALION_MAP_TRIGGER_ID 1
#define BATTALION_MAP_TRIGGER_GROUP 2

struct BattalionMapTrigger
    {
    char event[ENTITY_NAME_SIZE];
    char target[ENTITY_NAME_SIZE];
    int targetType;
    };

struct BattalionMapObject
    {
    char id[ENTITY_NAME_SIZE];
    char archetype[ENTITY_NAME_SIZE];
    char description[BATTALION_MAP_DESCRIPTION_SIZE];
    char group[ENTITY_NAME_SIZE];
    char faction[ENTITY_NAME_SIZE];
    char patrolRoute[ENTITY_NAME_SIZE];
    char safeZone[ENTITY_NAME_SIZE];
    float x;
    float y;
    float z;
    float heading;
    float scale;
    int active;
    int patrolMode;
    int alarmResponse;
    float safeWaitSeconds;
    struct EntityWaypoint *waypoints;
    size_t waypointCount;
    struct BattalionMapPoint *geometry;
    size_t geometryCount;
    struct BattalionMapProperty *properties;
    size_t propertyCount;
    struct BattalionMapTrigger *triggers;
    size_t triggerCount;
    };

struct BattalionMapPackage
    {
    int version;
    char id[32];
    char name[128];
    struct BattalionMapObject *objects;
    size_t objectCount;
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
