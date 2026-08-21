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

#ifndef BATTALION_MAP_H
#define BATTALION_MAP_H

#include <stddef.h>
#include <stdio.h>

void battalionMapRequest(const char *selector);
int battalionMapResolve(const char *dataPath, char *error, size_t errorSize);
int battalionMapBuildPackagePath(const char *dataPath,
                                 char *path, size_t pathSize);
const char *battalionMapId(void);
const char *battalionMapName(void);
void battalionMapPrintAvailable(const char *dataPath, FILE *stream);

#endif
