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
/* External battlefield map selection                                 */
/***********************************************************************/

#include "map.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define MAP_SELECTOR_SIZE 128
#define MAP_PATH_SIZE 1024

static char requestedMap[MAP_SELECTOR_SIZE];
static char resolvedMapName[MAP_SELECTOR_SIZE] = "default";
static char resolvedMapId[8] = "default";
static int resolvedMapNumber = -1;

static void trimLine(char *line)
    {
    size_t length;

    length = strlen(line);
    while (length > 0 && (line[length - 1] == '\n' ||
                          line[length - 1] == '\r' ||
                          isspace((unsigned char)line[length - 1])))
        line[--length] = '\0';
    }

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

static int openLevelList(const char *dataPath, FILE **levelFile,
                         char *path, size_t pathSize)
    {
    const char *suffix;

    suffix = "data/battalion.levels";
    if (strlen(dataPath) + strlen(suffix) + 1 > pathSize)
        return 0;

    strcpy(path, dataPath);
    strcat(path, suffix);
    *levelFile = fopen(path, "r");
    return *levelFile != NULL;
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
    FILE *levelFile;
    char path[MAP_PATH_SIZE];
    char line[MAP_SELECTOR_SIZE];
    int levelCount;
    int level;
    int requestedNumber;

    resolvedMapNumber = -1;
    strcpy(resolvedMapName, "default");
    strcpy(resolvedMapId, "default");

    if (!requestedMap[0] || sameName(requestedMap, "default"))
        return 1;

    if (!openLevelList(dataPath, &levelFile, path, sizeof(path)))
        {
        if (errorSize > 0)
            {
            strncpy(error, "Could not load data/battalion.levels", errorSize - 1);
            error[errorSize - 1] = '\0';
            }
        return 0;
        }

    if (fscanf(levelFile, "%d", &levelCount) != 1)
        {
        fclose(levelFile);
        if (errorSize > 0)
            {
            strncpy(error, "Invalid data/battalion.levels", errorSize - 1);
            error[errorSize - 1] = '\0';
            }
        return 0;
        }

    fgets(line, sizeof(line), levelFile);
    requestedNumber = -1;
    parseMapNumber(requestedMap, &requestedNumber);

    for (level = 0; level < levelCount; level++)
        {
        if (fgets(line, sizeof(line), levelFile) == NULL)
            break;
        trimLine(line);

        if (level == requestedNumber || sameName(line, requestedMap))
            {
            resolvedMapNumber = level;
            resolvedMapId[0] = (char)('0' + level / 10);
            resolvedMapId[1] = (char)('0' + level % 10);
            resolvedMapId[2] = '\0';
            strncpy(resolvedMapName, line, sizeof(resolvedMapName) - 1);
            resolvedMapName[sizeof(resolvedMapName) - 1] = '\0';
            fclose(levelFile);
            return 1;
            }
        }

    fclose(levelFile);
    if (errorSize > 0)
        {
        strncpy(error, "Unknown battlefield map: ", errorSize - 1);
        error[errorSize - 1] = '\0';
        if (strlen(error) + strlen(requestedMap) + 1 < errorSize)
            strcat(error, requestedMap);
        }
    return 0;
    }

int battalionMapBuildPath(const char *dataPath, const char *kind,
                          char *path, size_t pathSize)
    {
    char filename[64];
    char level[3];

    if (kind == NULL || strlen(kind) + 13 > sizeof(filename))
        return 0;
    strcpy(filename, "data/");
    strcat(filename, kind);
    if (resolvedMapNumber < 0)
        strcat(filename, ".data");
    else
        {
        level[0] = (char)('0' + resolvedMapNumber / 10);
        level[1] = (char)('0' + resolvedMapNumber % 10);
        level[2] = '\0';
        strcat(filename, level);
        strcat(filename, ".data");
        }

    if (strlen(dataPath) + strlen(filename) + 1 > pathSize)
        return 0;

    strcpy(path, dataPath);
    strcat(path, filename);
    return 1;
    }

int battalionMapBuildPackagePath(const char *dataPath,
                                 char *path, size_t pathSize)
    {
    const char *prefix;
    const char *suffix;

    prefix = "maps/";
    suffix = "/map.json";
    if (strlen(dataPath) + strlen(prefix) + strlen(resolvedMapId) +
        strlen(suffix) + 1 > pathSize)
        return 0;
    strcpy(path, dataPath);
    strcat(path, prefix);
    strcat(path, resolvedMapId);
    strcat(path, suffix);
    return 1;
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
    FILE *levelFile;
    char path[MAP_PATH_SIZE];
    char line[MAP_SELECTOR_SIZE];
    int count;
    int level;

    if (!openLevelList(dataPath, &levelFile, path, sizeof(path)))
        return;

    if (fscanf(levelFile, "%d", &count) != 1)
        {
        fclose(levelFile);
        return;
        }
    fgets(line, sizeof(line), levelFile);

    fprintf(stream, "Available maps:\n");
    for (level = 0; level < count; level++)
        {
        if (fgets(line, sizeof(line), levelFile) == NULL)
            break;
        trimLine(line);
        fprintf(stream, "  %02d  %s\n", level, line);
        }
    fprintf(stream, "  default  current unnumbered DATA files\n");
    fclose(levelFile);
    }
