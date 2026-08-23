/*
 * Copyright (c) 2026 Battalion contributors.
 */
#include "map.h"
#include "map_package.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int samePackage(const struct BattalionMapPackage *left,
                       const struct BattalionMapPackage *right)
    {
    size_t index;
    size_t nested;

    if (left->version != right->version ||
        strcmp(left->id, right->id) != 0 ||
        strcmp(left->name, right->name) != 0 ||
        left->roadCount != right->roadCount ||
        left->structureCount != right->structureCount ||
        left->actorCount != right->actorCount)
        return 0;
    for (index = 0; index < left->roadCount; index++)
        if (left->roads[index].x != right->roads[index].x ||
            left->roads[index].z != right->roads[index].z ||
            left->roads[index].type != right->roads[index].type)
            return 0;
    for (index = 0; index < left->structureCount; index++)
        if (left->structures[index].x != right->structures[index].x ||
            left->structures[index].z != right->structures[index].z ||
            left->structures[index].type != right->structures[index].type ||
            left->structures[index].shape != right->structures[index].shape)
            return 0;
    for (index = 0; index < left->actorCount; index++)
        {
        if (strcmp(left->actors[index].id, right->actors[index].id) != 0 ||
            strcmp(left->actors[index].archetype,
                   right->actors[index].archetype) != 0 ||
            strcmp(left->actors[index].group,
                   right->actors[index].group) != 0 ||
            left->actors[index].x != right->actors[index].x ||
            left->actors[index].z != right->actors[index].z ||
            left->actors[index].heading != right->actors[index].heading ||
            left->actors[index].active != right->actors[index].active ||
            left->actors[index].patrolMode !=
                right->actors[index].patrolMode ||
            left->actors[index].waypointCount !=
                right->actors[index].waypointCount ||
            left->actors[index].triggerCount !=
                right->actors[index].triggerCount)
            return 0;
        for (nested = 0; nested < left->actors[index].waypointCount; nested++)
            if (left->actors[index].waypoints[nested].x !=
                    right->actors[index].waypoints[nested].x ||
                left->actors[index].waypoints[nested].z !=
                    right->actors[index].waypoints[nested].z)
                return 0;
        for (nested = 0; nested < left->actors[index].triggerCount; nested++)
            if (strcmp(left->actors[index].triggers[nested].event,
                       right->actors[index].triggers[nested].event) != 0 ||
                strcmp(left->actors[index].triggers[nested].target,
                       right->actors[index].triggers[nested].target) != 0 ||
                left->actors[index].triggers[nested].targetType !=
                    right->actors[index].triggers[nested].targetType)
                return 0;
        }
    return 1;
    }

static int checkActors(void)
    {
    struct BattalionMapPackage source;
    struct BattalionMapPackage loaded;
    char error[256];
    int result;

    battalionMapPackageInit(&source);
    battalionMapPackageInit(&loaded);
    source.version = 2;
    strcpy(source.id, "actor-test");
    strcpy(source.name, "actor test");
    source.actorCount = 2;
    source.actors = (struct BattalionMapActor *)calloc(
        source.actorCount, sizeof(struct BattalionMapActor));
    if (source.actors == NULL)
        return 0;
    strcpy(source.actors[0].id, "guard-a");
    strcpy(source.actors[0].archetype, "tank-standard");
    strcpy(source.actors[0].group, "guards");
    source.actors[0].x = 1.0f;
    source.actors[0].z = 2.0f;
    source.actors[0].heading = 0.5f;
    source.actors[0].active = 1;
    source.actors[0].patrolMode = ENTITY_PATROL_PINGPONG;
    source.actors[0].waypointCount = 2;
    source.actors[0].waypoints = (struct EntityWaypoint *)calloc(
        2, sizeof(struct EntityWaypoint));
    source.actors[0].triggerCount = 2;
    source.actors[0].triggers = (struct BattalionMapTrigger *)calloc(
        2, sizeof(struct BattalionMapTrigger));
    if (source.actors[0].waypoints == NULL ||
        source.actors[0].triggers == NULL)
        {
        battalionMapPackageFree(&source);
        return 0;
        }
    source.actors[0].waypoints[0].x = 1.0f;
    source.actors[0].waypoints[0].z = 2.0f;
    source.actors[0].waypoints[1].x = 4.0f;
    source.actors[0].waypoints[1].z = 5.0f;
    strcpy(source.actors[0].triggers[0].event, "base-alarm");
    strcpy(source.actors[0].triggers[0].target, "guard-b");
    source.actors[0].triggers[0].targetType = BATTALION_MAP_TRIGGER_ID;
    strcpy(source.actors[0].triggers[1].event, "base-alarm");
    strcpy(source.actors[0].triggers[1].target, "guards");
    source.actors[0].triggers[1].targetType = BATTALION_MAP_TRIGGER_GROUP;

    strcpy(source.actors[1].id, "guard-b");
    strcpy(source.actors[1].archetype, "tank-standard");
    strcpy(source.actors[1].group, "guards");
    source.actors[1].x = 6.0f;
    source.actors[1].z = 7.0f;
    source.actors[1].active = 0;
    source.actors[1].patrolMode = ENTITY_PATROL_LOOP;

    result = battalionMapPackageWrite("build/tests/map-actors.json",
                                      &source, error, sizeof(error)) &&
             battalionMapPackageLoad("build/tests/map-actors.json",
                                     &loaded, error, sizeof(error)) &&
             samePackage(&source, &loaded);
    if (!result)
        fprintf(stderr, "actor package failed: %s\n", error);
    battalionMapPackageFree(&loaded);
    battalionMapPackageFree(&source);
    return result;
    }

static int checkPackage(const char *selector)
    {
    struct BattalionMapPackage json;
    struct BattalionMapPackage roundTrip;
    char error[256];
    char path[1024];
    int result;

    battalionMapRequest(selector);
    if (!battalionMapResolve("./assets/", error, sizeof(error)))
        return 0;
    battalionMapPackageInit(&json);
    battalionMapPackageInit(&roundTrip);
    result = battalionMapBuildPackagePath("./assets/", path,
                                           sizeof(path)) &&
             battalionMapPackageLoad(path, &json, error, sizeof(error)) &&
             battalionMapPackageWrite("build/tests/map-roundtrip.json", &json,
                                      error, sizeof(error)) &&
             battalionMapPackageLoad("build/tests/map-roundtrip.json",
                                     &roundTrip, error, sizeof(error)) &&
             samePackage(&json, &roundTrip);
    if (!result)
        fprintf(stderr, "map package %s failed: %s\n", selector, error);
    battalionMapPackageFree(&roundTrip);
    battalionMapPackageFree(&json);
    return result;
    }

static int checkPath(const char *selector, const char *expectedPath,
                     const char *expectedId, const char *expectedName)
    {
    char error[160];
    char path[1024];

    battalionMapRequest(selector);
    if (!battalionMapResolve("./assets/", error, sizeof(error)))
        {
        fprintf(stderr, "resolve %s failed: %s\n", selector, error);
        return 0;
        }
    if (!battalionMapBuildPackagePath("./assets/", path, sizeof(path)))
        return 0;
    if (strcmp(path, expectedPath) != 0)
        {
        fprintf(stderr, "path %s != %s\n", path, expectedPath);
        return 0;
        }
    if (strcmp(battalionMapId(), expectedId) != 0 ||
        strcmp(battalionMapName(), expectedName) != 0)
        {
        fprintf(stderr, "name %s != %s\n", battalionMapName(), expectedName);
        return 0;
        }
    return 1;
    }

static int checkLegacyUnitsRejected(void)
    {
    const char *path;
    struct BattalionMapPackage map;
    char error[160];
    FILE *file;
    int accepted;

    path = "build/tests/map-legacy-units.json";
    file = fopen(path, "wb");
    if (file == NULL)
        return 0;
    fputs("{\"format\":\"battalion-map\",\"version\":2,"
          "\"id\":\"legacy\",\"name\":\"legacy\","
          "\"roads\":[],\"structures\":[],"
          "\"units\":[{\"x\":0,\"z\":0,\"type\":0}],"
          "\"actors\":[]}", file);
    fclose(file);
    battalionMapPackageInit(&map);
    accepted = battalionMapPackageLoad(path, &map, error, sizeof(error));
    battalionMapPackageFree(&map);
    if (accepted)
        fprintf(stderr, "legacy units array was silently accepted\n");
    return !accepted;
    }

int main(void)
    {
    char error[160];
    char path[1024];

    if (!checkPath("default", "./assets/maps/default/map.json",
                   "default", "default"))
        return 1;
    if (!checkPath("Andyville", "./assets/maps/01/map.json",
                   "01", "Andyville"))
        return 1;
    if (!checkPath("2", "./assets/maps/02/map.json",
                   "02", "Garden-City"))
        return 1;

    battalionMapRequest("missing-map");
    if (battalionMapResolve("./assets/", error, sizeof(error)))
        {
        fprintf(stderr, "unknown map unexpectedly resolved\n");
        return 1;
        }
    if (battalionMapBuildPackagePath(
        "this-data-path-is-deliberately-too-long-for-the-map-path-buffer/"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/",
        path, sizeof(path)))
        {
        fprintf(stderr, "oversized map kind unexpectedly accepted\n");
        return 1;
        }
    if (!checkPackage("default") || !checkPackage("0") ||
        !checkPackage("1") || !checkPackage("2"))
        return 1;
    if (!checkActors())
        return 1;
    if (!checkLegacyUnitsRejected())
        return 1;
    return 0;
    }
