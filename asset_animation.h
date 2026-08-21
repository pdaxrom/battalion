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
/* Renderer-neutral sampling of animation clips loaded from assets.    */
/***********************************************************************/

#ifndef BATTALION_ASSET_ANIMATION_H
#define BATTALION_ASSET_ANIMATION_H

#include <stddef.h>

struct AssetModel;

struct AssetNodePose
    {
    float translation[3];
    float rotation[4];
    float scale[3];
    int visible;
    };

struct AssetPose
    {
    struct AssetNodePose *nodes;
    size_t nodeCount;
    float colorOffset[3];
    };

void assetPoseInitialize(struct AssetPose *pose);
void assetPoseDestroy(struct AssetPose *pose);
int assetPoseReset(const struct AssetModel *model, struct AssetPose *pose);
int assetPoseSample(const struct AssetModel *model, size_t animationIndex,
                    float timeSeconds, int loop, struct AssetPose *pose);
int assetPoseApply(const struct AssetModel *model, size_t animationIndex,
                   float timeSeconds, int loop, struct AssetPose *pose);

#endif
