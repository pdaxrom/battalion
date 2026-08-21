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
/* Convert all legacy road/tree/tank map sets into JSON packages.      */
/***********************************************************************/

#include "map.h"
#include "map_package.h"

#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

static int makeDirectory(const char *path)
    {
    if (mkdir(path, 0755) == 0 || errno == EEXIST)
        return 1;
    fprintf(stderr, "Could not create %s: %s\n", path, strerror(errno));
    return 0;
    }

static int convertMap(const char *dataPath, const char *selector)
    {
    struct BattalionMapPackage map;
    char error[256];
    char path[1024];
    char directory[1024];
    char *slash;

    battalionMapRequest(selector);
    if (!battalionMapResolve(dataPath, error, sizeof(error)))
        return 0;
    battalionMapPackageInit(&map);
    if (!battalionMapPackageReadLegacy(dataPath, &map, error, sizeof(error)) ||
        !battalionMapBuildPackagePath(dataPath, path, sizeof(path)))
        {
        fprintf(stderr, "%s: %s\n", selector, error);
        battalionMapPackageFree(&map);
        return -1;
        }

    strcpy(directory, path);
    slash = strrchr(directory, '/');
    if (slash == NULL)
        {
        battalionMapPackageFree(&map);
        return -1;
        }
    *slash = '\0';
    if (!makeDirectory(directory) ||
        !battalionMapPackageWrite(path, &map, error, sizeof(error)))
        {
        fprintf(stderr, "%s: %s\n", selector, error);
        battalionMapPackageFree(&map);
        return -1;
        }
    printf("%s: %lu roads, %lu structures, %lu units -> %s\n",
           map.name, (unsigned long)map.roadCount,
           (unsigned long)map.structureCount,
           (unsigned long)map.unitCount, path);
    battalionMapPackageFree(&map);
    return 1;
    }

int main(int argc, char **argv)
    {
    char dataPath[768];
    char mapsPath[1024];
    char selector[8];
    int level;
    int result;
    size_t length;

    setlocale(LC_NUMERIC, "C");

    if (argc > 2)
        {
        fprintf(stderr, "Usage: %s [assets/]\n", argv[0]);
        return 2;
        }
    strncpy(dataPath, argc == 2 ? argv[1] : "assets/",
            sizeof(dataPath) - 2);
    dataPath[sizeof(dataPath) - 2] = '\0';
    length = strlen(dataPath);
    if (length == 0 || dataPath[length - 1] != '/')
        strcat(dataPath, "/");
    if (strlen(dataPath) + strlen("maps") + 1 > sizeof(mapsPath))
        return 2;
    strcpy(mapsPath, dataPath);
    strcat(mapsPath, "maps");
    if (!makeDirectory(mapsPath))
        return 1;

    if (convertMap(dataPath, "default") < 0)
        return 1;
    for (level = 0; level < 100; level++)
        {
        sprintf(selector, "%d", level);
        result = convertMap(dataPath, selector);
        if (result == 0)
            break;
        if (result < 0)
            return 1;
        }
    return 0;
    }
