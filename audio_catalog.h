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

#ifndef BATTALION_AUDIO_CATALOG_H
#define BATTALION_AUDIO_CATALOG_H

#include <stddef.h>

struct AudioCatalogEntry
    {
    char *key;
    char *backend;
    char *path;
    int slot;
    };

struct AudioCatalog
    {
    struct AudioCatalogEntry *entries;
    size_t count;
    };

void audioCatalogInitialize(struct AudioCatalog *catalog);
void audioCatalogDestroy(struct AudioCatalog *catalog);
int audioCatalogLoad(const char *dataPath, const char *catalogPath,
                     struct AudioCatalog *catalog,
                     char *error, size_t errorSize);
const struct AudioCatalogEntry *audioCatalogFind(
    const struct AudioCatalog *catalog, const char *key,
    const char *backend);
int audioCatalogBuildPath(const char *dataPath,
                          const struct AudioCatalogEntry *entry,
                          char *path, size_t pathSize);

#endif
