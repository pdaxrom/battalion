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

#ifndef BATTALION_ASSET_VALUES_H
#define BATTALION_ASSET_VALUES_H

#include <stddef.h>

#define ASSET_VALUE_COMPONENTS 8

struct AssetValue
    {
    char *key;
    float components[ASSET_VALUE_COMPONENTS];
    size_t componentCount;
    };

struct AssetValues
    {
    struct AssetValue *values;
    size_t count;
    };

void assetValuesInitialize(struct AssetValues *values);
void assetValuesDestroy(struct AssetValues *values);
int assetValuesLoad(const char *path, struct AssetValues *values,
                    char *error, size_t errorSize);
const struct AssetValue *assetValuesFind(const struct AssetValues *values,
                                         const char *key);

#endif
