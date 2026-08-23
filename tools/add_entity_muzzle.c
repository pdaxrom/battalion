/*
 * Copyright (c) 2026 Battalion contributors.
 */
/****************************************************************/
/* Adds an authored muzzle transform to a glTF model.             */
/****************************************************************/

#include "asset.h"
#include "asset_write.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *copyText(const char *text)
    {
    char *copy;

    copy = (char *)malloc(strlen(text) + 1);
    if (copy != NULL)
        strcpy(copy, text);
    return copy;
    }

static int addMuzzle(struct AssetModel *model, float x, float y, float z,
                     float yawDegrees, float pitchDegrees)
    {
    struct AssetNode *nodes;
    struct AssetNode *muzzle;
    struct AssetNode *parent;
    int *children;
    size_t nodeIndex;
    size_t index;
    int attached;
    float halfAngle;
    float halfPitch;
    float sineYaw;
    float cosineYaw;
    float sinePitch;
    float cosinePitch;

    nodeIndex = (size_t)assetModelFindNode(model, "muzzle");
    if (nodeIndex == (size_t)-1)
        {
        nodeIndex = model->nodeCount;
        nodes = (struct AssetNode *)realloc(
            model->nodes, (model->nodeCount + 1) * sizeof(*model->nodes));
        if (nodes == NULL)
            return 0;
        model->nodes = nodes;
        muzzle = &model->nodes[nodeIndex];
        memset(muzzle, 0, sizeof(*muzzle));
        muzzle->name = copyText("muzzle");
        if (muzzle->name == NULL)
            return 0;
        model->nodeCount++;

        }
    if (model->sceneRootCount == 0)
        return 0;
    for (index = 0; index < model->sceneRootCount; index++)
        if (model->sceneRoots[index] == (int)nodeIndex)
            {
            memmove(&model->sceneRoots[index], &model->sceneRoots[index + 1],
                    (model->sceneRootCount - index - 1) *
                        sizeof(*model->sceneRoots));
            model->sceneRootCount--;
            break;
            }
    parent = &model->nodes[model->sceneRoots[0]];
    attached = 0;
    for (index = 0; index < parent->childCount; index++)
        if (parent->children[index] == (int)nodeIndex)
            attached = 1;
    if (!attached)
        {
        children = (int *)realloc(
            parent->children,
            (parent->childCount + 1) * sizeof(*parent->children));
        if (children == NULL)
            return 0;
        parent->children = children;
        parent->children[parent->childCount++] = (int)nodeIndex;
        }
    muzzle = &model->nodes[nodeIndex];
    muzzle->translation[0] = x;
    muzzle->translation[1] = y;
    muzzle->translation[2] = z;
    halfAngle = yawDegrees * 0.00872664625997164788f;
    halfPitch = pitchDegrees * 0.00872664625997164788f;
    sineYaw = (float)sin((double)halfAngle);
    cosineYaw = (float)cos((double)halfAngle);
    sinePitch = (float)sin((double)halfPitch);
    cosinePitch = (float)cos((double)halfPitch);
    muzzle->rotation[0] = cosineYaw * sinePitch;
    muzzle->rotation[1] = sineYaw * cosinePitch;
    muzzle->rotation[2] = -sineYaw * sinePitch;
    muzzle->rotation[3] = cosineYaw * cosinePitch;
    muzzle->scale[0] = 1.0f;
    muzzle->scale[1] = 1.0f;
    muzzle->scale[2] = 1.0f;
    return 1;
    }

int main(int argc, char **argv)
    {
    struct AssetModel model;
    char error[256];
    float x;
    float y;
    float z;
    float yaw;
    float pitch;
    int result;

    if (argc != 7)
        {
        fprintf(stderr,
                "usage: %s model.glb x y z yaw-degrees pitch-degrees\n",
                argv[0]);
        return 2;
        }
    x = (float)atof(argv[2]);
    y = (float)atof(argv[3]);
    z = (float)atof(argv[4]);
    yaw = (float)atof(argv[5]);
    pitch = (float)atof(argv[6]);
    assetModelInitialize(&model);
    result = assetModelLoadGLB(argv[1], &model, error, sizeof(error)) &&
             addMuzzle(&model, x, y, z, yaw, pitch) &&
             assetModelWriteGLB(argv[1], &model, error, sizeof(error));
    if (!result)
        fprintf(stderr, "%s: %s\n", argv[1], error);
    assetModelDestroy(&model);
    return result ? 0 : 1;
    }
