#include "map.h"
#include "map_package.h"

#include <stdio.h>
#include <string.h>

static int samePackage(const struct BattalionMapPackage *left,
                       const struct BattalionMapPackage *right)
    {
    size_t index;

    if (strcmp(left->id, right->id) != 0 ||
        strcmp(left->name, right->name) != 0 ||
        left->roadCount != right->roadCount ||
        left->structureCount != right->structureCount ||
        left->unitCount != right->unitCount)
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
    for (index = 0; index < left->unitCount; index++)
        if (left->units[index].x != right->units[index].x ||
            left->units[index].z != right->units[index].z ||
            left->units[index].type != right->units[index].type)
            return 0;
    return 1;
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
    return 0;
    }
