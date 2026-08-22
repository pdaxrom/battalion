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

#ifndef BATTALION_ASSET_H
#define BATTALION_ASSET_H

#include <stddef.h>

struct AssetVertex
    {
    float position[3];
    float normal[3];
    float texcoord[2];
    float color[4];
    };

struct AssetMesh
    {
    struct AssetVertex *vertices;
    size_t vertexCount;
    int primitive;
    float lineWidth;
    float pointSize;
    int useCurrentColor;
    int blend;
    };

#define ASSET_PRIMITIVE_TRIANGLES 0
#define ASSET_PRIMITIVE_LINES     1
#define ASSET_PRIMITIVE_POINTS    2

struct AssetNode
    {
    char *name;
    int mesh;
    int *children;
    size_t childCount;
    float translation[3];
    float rotation[4];
    float scale[3];
    float matrix[16];
    int hasMatrix;
    };

#define ASSET_ANIMATION_TRANSLATION 1
#define ASSET_ANIMATION_ROTATION    2
#define ASSET_ANIMATION_SCALE       3

#define ASSET_INTERPOLATION_LINEAR  1
#define ASSET_INTERPOLATION_STEP    2

struct AssetAnimationTrack
    {
    int node;
    int path;
    int interpolation;
    int components;
    float *times;
    float *values;
    size_t keyframeCount;
    };

struct AssetAnimation
    {
    char *name;
    struct AssetAnimationTrack *tracks;
    size_t trackCount;
    float duration;
    };

struct AssetModel
    {
    struct AssetMesh *meshes;
    size_t meshCount;
    struct AssetNode *nodes;
    size_t nodeCount;
    int *sceneRoots;
    size_t sceneRootCount;
    struct AssetAnimation *animations;
    size_t animationCount;
    };

void assetModelInitialize(struct AssetModel *model);
void assetModelDestroy(struct AssetModel *model);
int assetModelFindNode(const struct AssetModel *model, const char *name);
int assetModelLoadGLB(const char *path, struct AssetModel *model,
                      char *error, size_t errorSize);

#endif
