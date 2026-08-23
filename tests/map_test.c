/* Copyright (c) 2026 Battalion contributors. */
#include "map.h"
#include "map_package.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int sameObject(const struct BattalionMapObject *a,
                      const struct BattalionMapObject *b)
    {
    size_t i;
    if (strcmp(a->id, b->id) || strcmp(a->archetype, b->archetype) ||
        strcmp(a->description, b->description) || strcmp(a->group, b->group) ||
        strcmp(a->faction, b->faction) ||
        strcmp(a->patrolRoute, b->patrolRoute) ||
        strcmp(a->safeZone, b->safeZone) ||
        a->x != b->x || a->y != b->y || a->z != b->z ||
        a->heading != b->heading || a->scale != b->scale ||
        a->active != b->active || a->patrolMode != b->patrolMode ||
        a->alarmResponse != b->alarmResponse ||
        a->safeWaitSeconds != b->safeWaitSeconds ||
        a->waypointCount != b->waypointCount ||
        a->geometryCount != b->geometryCount ||
        a->propertyCount != b->propertyCount ||
        a->triggerCount != b->triggerCount)
        return 0;
    for (i = 0; i < a->waypointCount; i++)
        if (a->waypoints[i].x != b->waypoints[i].x ||
            a->waypoints[i].z != b->waypoints[i].z) return 0;
    for (i = 0; i < a->geometryCount; i++)
        if (a->geometry[i].x != b->geometry[i].x ||
            a->geometry[i].z != b->geometry[i].z) return 0;
    for (i = 0; i < a->propertyCount; i++)
        if (strcmp(a->properties[i].name, b->properties[i].name) ||
            strcmp(a->properties[i].value, b->properties[i].value)) return 0;
    for (i = 0; i < a->triggerCount; i++)
        if (strcmp(a->triggers[i].event, b->triggers[i].event) ||
            strcmp(a->triggers[i].target, b->triggers[i].target) ||
            a->triggers[i].targetType != b->triggers[i].targetType) return 0;
    return 1;
    }

static int samePackage(const struct BattalionMapPackage *a,
                       const struct BattalionMapPackage *b)
    {
    size_t i;
    if (a->version != b->version || strcmp(a->id, b->id) ||
        strcmp(a->name, b->name) || a->objectCount != b->objectCount)
        return 0;
    for (i = 0; i < a->objectCount; i++)
        if (!sameObject(&a->objects[i], &b->objects[i])) return 0;
    return 1;
    }

static int checkObjectRoundTrip(void)
    {
    struct BattalionMapPackage source;
    struct BattalionMapPackage loaded;
    struct BattalionMapObject *object;
    char error[256];
    int result;
    battalionMapPackageInit(&source);
    battalionMapPackageInit(&loaded);
    strcpy(source.id, "object-test");
    strcpy(source.name, "object test");
    source.objectCount = 4;
    source.objects = (struct BattalionMapObject *)calloc(
        source.objectCount, sizeof(struct BattalionMapObject));
    if (source.objects == NULL) return 0;
    object = &source.objects[0];
    strcpy(object->id, "guard-a");
    strcpy(object->archetype, "tank-standard");
    strcpy(object->description, "guards the road");
    strcpy(object->group, "guards");
    strcpy(object->faction, "player");
    strcpy(object->patrolRoute, "route-a");
    strcpy(object->safeZone, "safe-a");
    object->x = 1.0f; object->y = 0.25f; object->z = 2.0f;
    object->heading = 0.5f; object->scale = 1.25f; object->active = 1;
    object->patrolMode = ENTITY_PATROL_PINGPONG;
    object->alarmResponse = ENTITY_ALARM_RETREAT;
    object->safeWaitSeconds = 7.5f;
    object->geometryCount = 2;
    object->geometry = (struct BattalionMapPoint *)calloc(
        2, sizeof(struct BattalionMapPoint));
    object->propertyCount = 1;
    object->properties = (struct BattalionMapProperty *)calloc(
        1, sizeof(struct BattalionMapProperty));
    object->triggerCount = 1;
    object->triggers = (struct BattalionMapTrigger *)calloc(
        1, sizeof(struct BattalionMapTrigger));
    if (!object->geometry || !object->properties ||
        !object->triggers) { battalionMapPackageFree(&source); return 0; }
    object->geometry[0].x = -1.0f; object->geometry[0].z = -2.0f;
    object->geometry[1].x = 1.0f; object->geometry[1].z = 2.0f;
    strcpy(object->properties[0].name, "locked");
    strcpy(object->properties[0].value, "true");
    strcpy(object->triggers[0].event, "base-alarm");
    strcpy(object->triggers[0].target, "guards");
    object->triggers[0].targetType = BATTALION_MAP_TRIGGER_GROUP;
    object = &source.objects[1];
    strcpy(object->id, "route-a");
    strcpy(object->archetype, "logic-patrol-route");
    object->x = 3.0f; object->z = 4.0f; object->scale = 1.0f;
    object->active = 1; object->patrolMode = ENTITY_PATROL_LOOP;
    object->waypointCount = 2;
    object->waypoints = (struct EntityWaypoint *)calloc(
        2, sizeof(struct EntityWaypoint));
    if (!object->waypoints)
        { battalionMapPackageFree(&source); return 0; }
    object->waypoints[0].x = 3.0f; object->waypoints[0].z = 4.0f;
    object->waypoints[1].x = 6.0f; object->waypoints[1].z = 7.0f;
    object = &source.objects[2];
    strcpy(object->id, "safe-a");
    strcpy(object->archetype, "logic-safe-zone");
    object->x = 4.0f; object->z = 4.0f; object->scale = 1.0f;
    object->active = 1; object->patrolMode = ENTITY_PATROL_LOOP;
    object->geometryCount = 3;
    object->geometry = (struct BattalionMapPoint *)calloc(
        3, sizeof(struct BattalionMapPoint));
    if (!object->geometry)
        { battalionMapPackageFree(&source); return 0; }
    object->geometry[0].x = 3.0f; object->geometry[0].z = 3.0f;
    object->geometry[1].x = 5.0f; object->geometry[1].z = 3.0f;
    object->geometry[2].x = 4.0f; object->geometry[2].z = 5.0f;
    object = &source.objects[3];
    strcpy(object->id, "road-a");
    strcpy(object->archetype, "road-straight-x");
    object->x = 5.0f; object->z = 6.0f; object->scale = 1.0f;
    object->active = 1; object->patrolMode = ENTITY_PATROL_LOOP;
    result = battalionMapPackageWrite("build/tests/map-objects.json", &source,
                                      error, sizeof(error)) &&
             battalionMapPackageLoad("build/tests/map-objects.json", &loaded,
                                     error, sizeof(error)) &&
             samePackage(&source, &loaded);
    if (!result) fprintf(stderr, "object package failed: %s\n", error);
    battalionMapPackageFree(&loaded);
    battalionMapPackageFree(&source);
    return result;
    }

static int checkPackage(const char *selector)
    {
    struct BattalionMapPackage map;
    struct BattalionMapPackage copy;
    char error[256];
    char path[1024];
    int result;
    battalionMapRequest(selector);
    if (!battalionMapResolve("./assets/", error, sizeof(error))) return 0;
    battalionMapPackageInit(&map);
    battalionMapPackageInit(&copy);
    result = battalionMapBuildPackagePath("./assets/", path, sizeof(path)) &&
             battalionMapPackageLoad(path, &map, error, sizeof(error)) &&
             battalionMapPackageWrite("build/tests/map-roundtrip.json", &map,
                                      error, sizeof(error)) &&
             battalionMapPackageLoad("build/tests/map-roundtrip.json", &copy,
                                     error, sizeof(error)) &&
             samePackage(&map, &copy);
    if (!result) fprintf(stderr, "map %s failed: %s\n", selector, error);
    battalionMapPackageFree(&copy);
    battalionMapPackageFree(&map);
    return result;
    }

static int checkPath(const char *selector, const char *path,
                     const char *id, const char *name)
    {
    char error[160];
    char actual[1024];
    int result;
    battalionMapRequest(selector);
    result = battalionMapResolve("./assets/", error, sizeof(error)) &&
             battalionMapBuildPackagePath("./assets/", actual,
                                          sizeof(actual)) &&
             strcmp(actual, path) == 0 &&
             strcmp(battalionMapId(), id) == 0 &&
             strcmp(battalionMapName(), name) == 0;
    if (!result)
        fprintf(stderr, "map selection '%s': %s\n", selector, error);
    return result;
    }

static int checkLegacyArraysRejected(void)
    {
    struct BattalionMapPackage map;
    char error[160];
    FILE *file;
    int accepted;
    file = fopen("build/tests/map-legacy-arrays.json", "wb");
    if (!file) return 0;
    fputs("{\"format\":\"battalion-map\",\"version\":2,"
          "\"id\":\"legacy\",\"name\":\"legacy\","
          "\"roads\":[],\"structures\":[],\"actors\":[],"
          "\"objects\":[]}", file);
    fclose(file);
    battalionMapPackageInit(&map);
    accepted = battalionMapPackageLoad("build/tests/map-legacy-arrays.json",
                                       &map, error, sizeof(error));
    battalionMapPackageFree(&map);
    return !accepted;
    }

int main(void)
    {
    char error[160];
    if (!checkPath("default", "./assets/maps/default/map.json",
                   "default", "default"))
        { fprintf(stderr, "default map selection failed\n"); return 1; }
    if (!checkPath("Andyville", "./assets/maps/01/map.json",
                   "01", "Andyville"))
        { fprintf(stderr, "Andyville map selection failed\n"); return 1; }
    if (!checkPath("2", "./assets/maps/02/map.json", "02", "Garden-City"))
        { fprintf(stderr, "Garden-City map selection failed\n"); return 1; }
    if (!checkPath("riverside-outpost",
                   "./assets/maps/riverside-outpost/map.json",
                   "riverside-outpost", "Riverside Outpost"))
        { fprintf(stderr, "Riverside Outpost selection failed\n"); return 1; }
    if (!checkPackage("default") || !checkPackage("Andyville") ||
        !checkPackage("2") || !checkPackage("riverside-outpost") ||
        !checkObjectRoundTrip() ||
        !checkLegacyArraysRejected())
        return 1;
    battalionMapRequest("missing-map");
    if (battalionMapResolve("./assets/", error, sizeof(error))) return 1;
    return 0;
    }
