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
/* Renderer-neutral asset model lifetime.                              */
/***********************************************************************/

#include "asset.h"
#include "asset_animation.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

struct AssetBounds
    {
    float minimum[3];
    float maximum[3];
    int valid;
    };

static void boundsIdentityMatrix(float matrix[16])
    {
    int component;

    for (component = 0; component < 16; component++)
        matrix[component] = component % 5 == 0 ? 1.0f : 0.0f;
    }

static void boundsMultiplyMatrix(float result[16], const float left[16],
                                 const float right[16])
    {
    float product[16];
    int column;
    int row;
    int inner;

    for (column = 0; column < 4; column++)
        for (row = 0; row < 4; row++)
            {
            product[column * 4 + row] = 0.0f;
            for (inner = 0; inner < 4; inner++)
                product[column * 4 + row] +=
                    left[inner * 4 + row] * right[column * 4 + inner];
            }
    memcpy(result, product, sizeof(product));
    }

static void boundsNodeMatrix(const struct AssetNode *node,
                             const struct AssetNodePose *pose,
                             float matrix[16])
    {
    float x;
    float y;
    float z;
    float w;
    float length;

    if (node->hasMatrix)
        {
        memcpy(matrix, node->matrix, sizeof(node->matrix));
        return;
        }
    x = pose != NULL ? pose->rotation[0] : node->rotation[0];
    y = pose != NULL ? pose->rotation[1] : node->rotation[1];
    z = pose != NULL ? pose->rotation[2] : node->rotation[2];
    w = pose != NULL ? pose->rotation[3] : node->rotation[3];
    length = (float)sqrt((double)(x * x + y * y + z * z + w * w));
    if (length > 0.0f)
        {
        x /= length;
        y /= length;
        z /= length;
        w /= length;
        }
    boundsIdentityMatrix(matrix);
    matrix[0] = (1.0f - 2.0f * (y * y + z * z)) *
        (pose != NULL ? pose->scale[0] : node->scale[0]);
    matrix[1] = (2.0f * (x * y + z * w)) *
        (pose != NULL ? pose->scale[0] : node->scale[0]);
    matrix[2] = (2.0f * (x * z - y * w)) *
        (pose != NULL ? pose->scale[0] : node->scale[0]);
    matrix[4] = (2.0f * (x * y - z * w)) *
        (pose != NULL ? pose->scale[1] : node->scale[1]);
    matrix[5] = (1.0f - 2.0f * (x * x + z * z)) *
        (pose != NULL ? pose->scale[1] : node->scale[1]);
    matrix[6] = (2.0f * (y * z + x * w)) *
        (pose != NULL ? pose->scale[1] : node->scale[1]);
    matrix[8] = (2.0f * (x * z + y * w)) *
        (pose != NULL ? pose->scale[2] : node->scale[2]);
    matrix[9] = (2.0f * (y * z - x * w)) *
        (pose != NULL ? pose->scale[2] : node->scale[2]);
    matrix[10] = (1.0f - 2.0f * (x * x + y * y)) *
        (pose != NULL ? pose->scale[2] : node->scale[2]);
    matrix[12] = pose != NULL ? pose->translation[0] : node->translation[0];
    matrix[13] = pose != NULL ? pose->translation[1] : node->translation[1];
    matrix[14] = pose != NULL ? pose->translation[2] : node->translation[2];
    }

static int findNodeWorldMatrix(const struct AssetModel *model,
                               int currentNode, int wantedNode,
                               const struct AssetPose *pose,
                               const float parentMatrix[16], size_t depth,
                               float result[16])
    {
    const struct AssetNode *node;
    const struct AssetNodePose *nodePose;
    float localMatrix[16];
    float worldMatrix[16];
    size_t child;

    if (currentNode < 0 || currentNode >= (int)model->nodeCount ||
        depth >= model->nodeCount)
        return 0;
    node = &model->nodes[currentNode];
    nodePose = pose != NULL && pose->nodeCount == model->nodeCount ?
        &pose->nodes[currentNode] : NULL;
    boundsNodeMatrix(node, nodePose, localMatrix);
    boundsMultiplyMatrix(worldMatrix, parentMatrix, localMatrix);
    if (currentNode == wantedNode)
        {
        memcpy(result, worldMatrix, sizeof(worldMatrix));
        return 1;
        }
    for (child = 0; child < node->childCount; child++)
        if (findNodeWorldMatrix(model, node->children[child], wantedNode,
                                pose, worldMatrix, depth + 1, result))
            return 1;
    return 0;
    }

static void includeBoundsPosition(struct AssetBounds *bounds,
                                  const float position[3])
    {
    int component;

    if (!bounds->valid)
        {
        memcpy(bounds->minimum, position, sizeof(bounds->minimum));
        memcpy(bounds->maximum, position, sizeof(bounds->maximum));
        bounds->valid = 1;
        return;
        }
    for (component = 0; component < 3; component++)
        {
        if (position[component] < bounds->minimum[component])
            bounds->minimum[component] = position[component];
        if (position[component] > bounds->maximum[component])
            bounds->maximum[component] = position[component];
        }
    }

static void includeBoundsMesh(struct AssetBounds *bounds,
                              const struct AssetMesh *mesh,
                              const float matrix[16])
    {
    const struct AssetVertex *vertex;
    float position[3];
    size_t vertexIndex;

    for (vertexIndex = 0; vertexIndex < mesh->vertexCount; vertexIndex++)
        {
        vertex = &mesh->vertices[vertexIndex];
        position[0] = matrix[0] * vertex->position[0] +
                      matrix[4] * vertex->position[1] +
                      matrix[8] * vertex->position[2] + matrix[12];
        position[1] = matrix[1] * vertex->position[0] +
                      matrix[5] * vertex->position[1] +
                      matrix[9] * vertex->position[2] + matrix[13];
        position[2] = matrix[2] * vertex->position[0] +
                      matrix[6] * vertex->position[1] +
                      matrix[10] * vertex->position[2] + matrix[14];
        includeBoundsPosition(bounds, position);
        }
    }

static void includeBoundsNode(const struct AssetModel *model, int nodeIndex,
                              const struct AssetPose *pose,
                              const float parentMatrix[16], size_t depth,
                              struct AssetBounds *bounds)
    {
    const struct AssetNode *node;
    const struct AssetNodePose *nodePose;
    float localMatrix[16];
    float worldMatrix[16];
    size_t child;

    if (nodeIndex < 0 || nodeIndex >= (int)model->nodeCount ||
        depth >= model->nodeCount)
        return;
    nodePose = pose != NULL && pose->nodeCount == model->nodeCount ?
        &pose->nodes[nodeIndex] : NULL;
    if (nodePose != NULL && !nodePose->visible)
        return;
    node = &model->nodes[nodeIndex];
    boundsNodeMatrix(node, nodePose, localMatrix);
    boundsMultiplyMatrix(worldMatrix, parentMatrix, localMatrix);
    if (node->mesh >= 0 && node->mesh < (int)model->meshCount)
        includeBoundsMesh(bounds, &model->meshes[node->mesh], worldMatrix);
    for (child = 0; child < node->childCount; child++)
        includeBoundsNode(model, node->children[child], pose, worldMatrix,
                          depth + 1, bounds);
    }

static int copyAssetBounds(const struct AssetBounds *bounds,
                           float minimum[3], float maximum[3])
    {
    if (!bounds->valid)
        return 0;
    memcpy(minimum, bounds->minimum, sizeof(bounds->minimum));
    memcpy(maximum, bounds->maximum, sizeof(bounds->maximum));
    return 1;
    }

void assetModelInitialize(struct AssetModel *model)
    {
    model->meshes = NULL;
    model->meshCount = 0;
    model->nodes = NULL;
    model->nodeCount = 0;
    model->sceneRoots = NULL;
    model->sceneRootCount = 0;
    model->animations = NULL;
    model->animationCount = 0;
    }

void assetModelDestroy(struct AssetModel *model)
    {
    size_t mesh;
    size_t node;
    size_t animation;
    size_t track;

    if (model == NULL)
        return;
    for (mesh = 0; mesh < model->meshCount; mesh++)
        free(model->meshes[mesh].vertices);
    for (node = 0; node < model->nodeCount; node++)
        {
        free(model->nodes[node].name);
        free(model->nodes[node].children);
        }
    for (animation = 0; animation < model->animationCount; animation++)
        {
        free(model->animations[animation].name);
        for (track = 0; track < model->animations[animation].trackCount; track++)
            {
            free(model->animations[animation].tracks[track].times);
            free(model->animations[animation].tracks[track].values);
            }
        free(model->animations[animation].tracks);
        }
    free(model->meshes);
    free(model->nodes);
    free(model->sceneRoots);
    free(model->animations);
    assetModelInitialize(model);
    }

int assetModelFindNode(const struct AssetModel *model, const char *name)
    {
    size_t node;

    if (model == NULL || name == NULL)
        return -1;
    for (node = 0; node < model->nodeCount; node++)
        if (model->nodes[node].name != NULL &&
            strcmp(model->nodes[node].name, name) == 0)
            return (int)node;
    return -1;
    }

int assetModelPoseBounds(const struct AssetModel *model,
                         const struct AssetPose *pose,
                         float minimum[3], float maximum[3])
    {
    struct AssetBounds bounds;
    float identity[16];
    size_t mesh;
    size_t root;

    if (model == NULL || minimum == NULL || maximum == NULL)
        return 0;
    memset(&bounds, 0, sizeof(bounds));
    boundsIdentityMatrix(identity);
    if (model->nodeCount > 0 && model->sceneRootCount > 0)
        for (root = 0; root < model->sceneRootCount; root++)
            includeBoundsNode(model, model->sceneRoots[root], pose, identity,
                              0, &bounds);
    else
        for (mesh = 0; mesh < model->meshCount; mesh++)
            includeBoundsMesh(&bounds, &model->meshes[mesh], identity);
    return copyAssetBounds(&bounds, minimum, maximum);
    }

int assetModelNodePoseBounds(const struct AssetModel *model, int node,
                             const struct AssetPose *pose,
                             float minimum[3], float maximum[3])
    {
    struct AssetBounds bounds;
    float identity[16];

    if (model == NULL || minimum == NULL || maximum == NULL ||
        node < 0 || node >= (int)model->nodeCount)
        return 0;
    memset(&bounds, 0, sizeof(bounds));
    boundsIdentityMatrix(identity);
    includeBoundsNode(model, node, pose, identity, 0, &bounds);
    return copyAssetBounds(&bounds, minimum, maximum);
    }

int assetModelNodeWorldMatrix(const struct AssetModel *model, int node,
                              const struct AssetPose *pose,
                              float matrix[16])
    {
    float identity[16];
    size_t root;

    if (model == NULL || matrix == NULL || node < 0 ||
        node >= (int)model->nodeCount)
        return 0;
    boundsIdentityMatrix(identity);
    for (root = 0; root < model->sceneRootCount; root++)
        if (findNodeWorldMatrix(model, model->sceneRoots[root], node, pose,
                                identity, 0, matrix))
            return 1;
    return 0;
    }
