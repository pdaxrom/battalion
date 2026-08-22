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
/* Renderer-neutral linear blend skinning for glTF 2.0 assets.        */
/***********************************************************************/

#ifndef BATTALION_ASSET_SKIN_H
#define BATTALION_ASSET_SKIN_H

#include <stddef.h>

struct AssetModel;
struct AssetPose;
struct AssetVertex;

struct AssetSkinPalette
    {
    float *matrices;
    float *normalMatrices;
    size_t jointCount;
    };

void assetSkinPaletteInitialize(struct AssetSkinPalette *palette);
void assetSkinPaletteDestroy(struct AssetSkinPalette *palette);
int assetSkinPaletteBuild(const struct AssetModel *model, int meshNode,
                          const struct AssetPose *pose,
                          struct AssetSkinPalette *palette);
void assetSkinVertex(const struct AssetSkinPalette *palette,
                     const struct AssetVertex *vertex,
                     float position[3], float normal[3]);

#endif
