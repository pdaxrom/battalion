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

#ifndef BATTALION_ASSET_BINDINGS_H
#define BATTALION_ASSET_BINDINGS_H

#include <stddef.h>

#define ASSET_BINDING_MODEL 1
#define ASSET_BINDING_TEXTURE 2
#define ASSET_BINDING_ANIMATION 3
#define ASSET_BINDING_FILE 4

struct AssetBinding
    {
    int type;
    char *slot;
    char *assetKey;
    };

struct AssetBindings
    {
    struct AssetBinding *entries;
    size_t count;
    };

void assetBindingsInitialize(struct AssetBindings *bindings);
void assetBindingsDestroy(struct AssetBindings *bindings);
int assetBindingsLoad(const char *path, struct AssetBindings *bindings,
                      char *error, size_t errorSize);
const char *assetBindingsFind(const struct AssetBindings *bindings,
                              int type, const char *slot);

#endif
