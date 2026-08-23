/*
 * Copyright (c) 2026 Battalion contributors.
 */
/* External battlefield map selection                                 */
/***********************************************************************/

#include "map.h"
#include "map_package.h"

#include <ctype.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

#define MAP_SELECTOR_SIZE 128
#define MAP_PATH_SIZE 1024

static char requestedMap[MAP_SELECTOR_SIZE];
static char resolvedMapName[MAP_SELECTOR_SIZE] = "default";
static char resolvedMapId[32] = "default";

static int sameName(const char *left, const char *right)
    {
    while (*left && *right)
        {
        if (tolower((unsigned char)*left) != tolower((unsigned char)*right))
            return 0;
        left++;
        right++;
        }
    return *left == *right;
    }

static int parseMapNumber(const char *selector, int *number)
    {
    const char *cursor;
    char *end;
    long value;

    if (!selector[0])
        return 0;
    for (cursor = selector; *cursor; cursor++)
        if (!isdigit((unsigned char)*cursor))
            return 0;
    value = strtol(selector, &end, 10);
    if (*end || value < 0 || value > 99)
        return 0;
    *number = (int)value;
    return 1;
    }

static int buildPackagePathForId(const char *dataPath, const char *id,
                                 char *path, size_t pathSize)
    {
    const char *prefix = "maps/";
    const char *suffix = "/map.json";

    if (strlen(dataPath) + strlen(prefix) + strlen(id) + strlen(suffix) + 1 >
        pathSize)
        return 0;
    strcpy(path, dataPath);
    strcat(path, prefix);
    strcat(path, id);
    strcat(path, suffix);
    return 1;
    }

static int resolvePackage(const char *dataPath, const char *id,
                          const char *selector, char *error,
                          size_t errorSize)
    {
    struct BattalionMapPackage map;
    char path[MAP_PATH_SIZE];
    int matches;

    if (!buildPackagePathForId(dataPath, id, path, sizeof(path)))
        return 0;
    battalionMapPackageInit(&map);
    if (!battalionMapPackageLoad(path, &map, error, errorSize))
        return 0;
    matches = selector == NULL || !selector[0] ||
              sameName(selector, map.id) || sameName(selector, map.name) ||
              (sameName(selector, "default") && sameName(map.id, "default"));
    if (matches)
        {
        strncpy(resolvedMapId, map.id, sizeof(resolvedMapId) - 1);
        resolvedMapId[sizeof(resolvedMapId) - 1] = '\0';
        strncpy(resolvedMapName, map.name, sizeof(resolvedMapName) - 1);
        resolvedMapName[sizeof(resolvedMapName) - 1] = '\0';
        }
    battalionMapPackageFree(&map);
    return matches;
    }

void battalionMapRequest(const char *selector)
    {
    if (selector == NULL)
        selector = "";
    strncpy(requestedMap, selector, sizeof(requestedMap) - 1);
    requestedMap[sizeof(requestedMap) - 1] = '\0';
    }

int battalionMapResolve(const char *dataPath, char *error, size_t errorSize)
    {
    DIR *directory;
    struct dirent *entry;
    char mapsPath[MAP_PATH_SIZE];
    char id[3];
    int number;

    if (!requestedMap[0] || sameName(requestedMap, "default"))
        return resolvePackage(dataPath, "default", "default", error, errorSize);

    if (parseMapNumber(requestedMap, &number))
        {
        id[0] = (char)('0' + number / 10);
        id[1] = (char)('0' + number % 10);
        id[2] = '\0';
        if (resolvePackage(dataPath, id, id, error, errorSize))
            return 1;
        }

    if (strlen(dataPath) + strlen("maps") + 1 <= sizeof(mapsPath))
        {
        strcpy(mapsPath, dataPath);
        strcat(mapsPath, "maps");
        directory = opendir(mapsPath);
        if (directory != NULL)
            {
            while ((entry = readdir(directory)) != NULL)
                if (entry->d_name[0] != '.' &&
                    resolvePackage(dataPath, entry->d_name, requestedMap,
                                   error, errorSize))
                    {
                    closedir(directory);
                    return 1;
                    }
            closedir(directory);
            }
        }

    if (error != NULL && errorSize > 0)
        {
        strncpy(error, "Unknown battlefield map: ", errorSize - 1);
        error[errorSize - 1] = '\0';
        if (strlen(error) + strlen(requestedMap) + 1 < errorSize)
            strcat(error, requestedMap);
        }
    return 0;
    }

int battalionMapBuildPackagePath(const char *dataPath,
                                 char *path, size_t pathSize)
    {
    return buildPackagePathForId(dataPath, resolvedMapId, path, pathSize);
    }

const char *battalionMapId(void)
    {
    return resolvedMapId;
    }

const char *battalionMapName(void)
    {
    return resolvedMapName;
    }

void battalionMapPrintAvailable(const char *dataPath, FILE *stream)
    {
    DIR *directory;
    struct dirent *entry;
    struct BattalionMapPackage map;
    char mapsPath[MAP_PATH_SIZE];
    char packagePath[MAP_PATH_SIZE];
    char error[160];

    if (strlen(dataPath) + strlen("maps") + 1 > sizeof(mapsPath))
        return;
    strcpy(mapsPath, dataPath);
    strcat(mapsPath, "maps");
    directory = opendir(mapsPath);
    if (directory == NULL)
        return;
    fprintf(stream, "Available maps:\n");
    while ((entry = readdir(directory)) != NULL)
        {
        if (entry->d_name[0] == '.' ||
            !buildPackagePathForId(dataPath, entry->d_name, packagePath,
                                   sizeof(packagePath)))
            continue;
        battalionMapPackageInit(&map);
        if (battalionMapPackageLoad(packagePath, &map, error, sizeof(error)))
            fprintf(stream, "  %s  %s\n", map.id, map.name);
        battalionMapPackageFree(&map);
        }
    closedir(directory);
    }
