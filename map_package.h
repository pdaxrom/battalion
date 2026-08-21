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
/* Versioned, renderer-neutral battlefield map packages.               */
/***********************************************************************/

#ifndef BATTALION_MAP_PACKAGE_H
#define BATTALION_MAP_PACKAGE_H

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

struct BattalionMapUnit
    {
    float x;
    float z;
    int type;
    };

struct BattalionMapPackage
    {
    char id[32];
    char name[128];
    struct BattalionMapRoad *roads;
    size_t roadCount;
    struct BattalionMapStructure *structures;
    size_t structureCount;
    struct BattalionMapUnit *units;
    size_t unitCount;
    };

void battalionMapPackageInit(struct BattalionMapPackage *map);
void battalionMapPackageFree(struct BattalionMapPackage *map);

int battalionMapPackageLoad(const char *path,
                            struct BattalionMapPackage *map,
                            char *error, size_t errorSize);
int battalionMapPackageReadLegacy(const char *dataPath,
                                  struct BattalionMapPackage *map,
                                  char *error, size_t errorSize);
int battalionMapPackageWrite(const char *path,
                             const struct BattalionMapPackage *map,
                             char *error, size_t errorSize);

#endif
