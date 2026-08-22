/****************************************************************/
/* code copyright 1995-1996 Andrew Johnson - ALL RIGHTS RESERVED*/
/*                          ajohnson@eecs.uic.edu               */
/*                                                              */
/*                      Electronic Visualization Lab (M/C 154)  */
/*                      University of Illinois at Chicago       */
/*                      851 South Morgan St. Room 1120 SEO      */
/*                                                              */
/*                      (312) 996-3002     (312) 413-7585 FAX   */
/***********************************************************************/
/* Add authored moving tread plates and their animation to tank.glb.   */
/***********************************************************************/

#include "asset.h"
#include "asset_write.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TREADS_PER_SIDE 12
#define TREAD_COUNT (TREADS_PER_SIDE * 2)
#define TREAD_KEYFRAME_COUNT 25
#define TREAD_DURATION 1.2f

static char *copyText(const char *text)
    {
    char *copy;

    copy = (char *)malloc(strlen(text) + 1);
    if (copy != NULL)
        strcpy(copy, text);
    return copy;
    }

static void setVertex(struct AssetVertex *vertex,
                      float x, float y, float z,
                      float nx, float ny, float nz)
    {
    memset(vertex, 0, sizeof(*vertex));
    vertex->position[0] = x;
    vertex->position[1] = y;
    vertex->position[2] = z;
    vertex->normal[0] = nx;
    vertex->normal[1] = ny;
    vertex->normal[2] = nz;
    vertex->color[0] = 0.12f;
    vertex->color[1] = 0.12f;
    vertex->color[2] = 0.12f;
    vertex->color[3] = 1.0f;
    }

static void addTriangle(struct AssetMesh *mesh, size_t *at,
                        const float first[3], const float second[3],
                        const float third[3], const float normal[3])
    {
    setVertex(&mesh->vertices[*at], first[0], first[1], first[2],
              normal[0], normal[1], normal[2]);
    *at += 1;
    setVertex(&mesh->vertices[*at], second[0], second[1], second[2],
              normal[0], normal[1], normal[2]);
    *at += 1;
    setVertex(&mesh->vertices[*at], third[0], third[1], third[2],
              normal[0], normal[1], normal[2]);
    *at += 1;
    }

static void addFace(struct AssetMesh *mesh, size_t *at,
                    const float first[3], const float second[3],
                    const float third[3], const float fourth[3],
                    const float normal[3])
    {
    addTriangle(mesh, at, first, second, third, normal);
    addTriangle(mesh, at, first, third, fourth, normal);
    }

static int buildTreadMesh(struct AssetMesh *mesh)
    {
    static const float p000[3] = {-0.078f, -0.008f, -0.018f};
    static const float p001[3] = {-0.078f, -0.008f,  0.018f};
    static const float p010[3] = {-0.078f,  0.008f, -0.018f};
    static const float p011[3] = {-0.078f,  0.008f,  0.018f};
    static const float p100[3] = { 0.078f, -0.008f, -0.018f};
    static const float p101[3] = { 0.078f, -0.008f,  0.018f};
    static const float p110[3] = { 0.078f,  0.008f, -0.018f};
    static const float p111[3] = { 0.078f,  0.008f,  0.018f};
    static const float left[3] = {-1.0f, 0.0f, 0.0f};
    static const float right[3] = {1.0f, 0.0f, 0.0f};
    static const float down[3] = {0.0f, -1.0f, 0.0f};
    static const float up[3] = {0.0f, 1.0f, 0.0f};
    static const float back[3] = {0.0f, 0.0f, -1.0f};
    static const float front[3] = {0.0f, 0.0f, 1.0f};
    size_t at;

    memset(mesh, 0, sizeof(*mesh));
    mesh->vertices = (struct AssetVertex *)calloc(
        36, sizeof(*mesh->vertices));
    if (mesh->vertices == NULL)
        return 0;
    mesh->vertexCount = 36;
    mesh->primitive = ASSET_PRIMITIVE_TRIANGLES;
    mesh->lineWidth = 1.01f;
    mesh->pointSize = 2.0f;
    at = 0;
    addFace(mesh, &at, p000, p001, p011, p010, left);
    addFace(mesh, &at, p100, p110, p111, p101, right);
    addFace(mesh, &at, p000, p100, p101, p001, down);
    addFace(mesh, &at, p010, p011, p111, p110, up);
    addFace(mesh, &at, p000, p010, p110, p100, back);
    addFace(mesh, &at, p001, p101, p111, p011, front);
    return at == mesh->vertexCount;
    }

static void treadPosition(float phase, float *y, float *z)
    {
    float distance;

    phase -= (float)((int)phase);
    if (phase < 0.0f)
        phase += 1.0f;
    distance = phase * 1.024f;
    if (distance < 0.36f)
        {
        *y = 0.158f;
        *z = -0.18f + distance;
        }
    else if (distance < 0.512f)
        {
        *y = 0.158f - (distance - 0.36f);
        *z = 0.18f;
        }
    else if (distance < 0.872f)
        {
        *y = 0.006f;
        *z = 0.18f - (distance - 0.512f);
        }
    else
        {
        *y = 0.006f + (distance - 0.872f);
        *z = -0.18f;
        }
    }

static int addAnimatedTreads(struct AssetModel *model)
    {
    struct AssetAnimation *animation;
    struct AssetAnimationTrack *track;
    struct AssetMesh *meshes;
    struct AssetNode *nodes;
    struct AssetNode *root;
    int *children;
    float phase;
    float y;
    float z;
    char name[32];
    size_t animationIndex;
    size_t firstNode;
    size_t keyframe;
    size_t meshIndex;
    size_t nodeIndex;
    size_t tread;

    if (assetModelFindNode(model, "tread-left-00") >= 0)
        return 1;
    if (model->nodeCount < 1 || model->nodes[0].hasMatrix)
        return 0;

    meshIndex = model->meshCount;
    meshes = (struct AssetMesh *)realloc(
        model->meshes, (model->meshCount + 1) * sizeof(*model->meshes));
    if (meshes == NULL)
        return 0;
    model->meshes = meshes;
    memset(&model->meshes[meshIndex], 0, sizeof(model->meshes[meshIndex]));
    if (!buildTreadMesh(&model->meshes[meshIndex]))
        return 0;
    model->meshCount += 1;

    firstNode = model->nodeCount;
    nodes = (struct AssetNode *)realloc(
        model->nodes, (model->nodeCount + TREAD_COUNT) * sizeof(*model->nodes));
    if (nodes == NULL)
        return 0;
    model->nodes = nodes;
    memset(&model->nodes[firstNode], 0,
           TREAD_COUNT * sizeof(*model->nodes));
    model->nodeCount += TREAD_COUNT;

    root = &model->nodes[0];
    children = (int *)realloc(
        root->children, (root->childCount + TREAD_COUNT) * sizeof(*children));
    if (children == NULL)
        return 0;
    root->children = children;

    animationIndex = model->animationCount;
    animation = (struct AssetAnimation *)realloc(
        model->animations,
        (model->animationCount + 1) * sizeof(*model->animations));
    if (animation == NULL)
        return 0;
    model->animations = animation;
    animation = &model->animations[animationIndex];
    memset(animation, 0, sizeof(*animation));
    animation->name = copyText("tracks");
    animation->duration = TREAD_DURATION;
    animation->trackCount = TREAD_COUNT;
    animation->tracks = (struct AssetAnimationTrack *)calloc(
        TREAD_COUNT, sizeof(*animation->tracks));
    if (animation->name == NULL || animation->tracks == NULL)
        return 0;
    model->animationCount += 1;

    for (tread = 0; tread < TREAD_COUNT; tread++)
        {
        nodeIndex = firstNode + tread;
        sprintf(name, "tread-%s-%02lu",
                tread < TREADS_PER_SIDE ? "left" : "right",
                (unsigned long)(tread % TREADS_PER_SIDE));
        model->nodes[nodeIndex].name = copyText(name);
        model->nodes[nodeIndex].mesh = (int)meshIndex;
        model->nodes[nodeIndex].rotation[3] = 1.0f;
        model->nodes[nodeIndex].scale[0] = 1.0f;
        model->nodes[nodeIndex].scale[1] = 1.0f;
        model->nodes[nodeIndex].scale[2] = 1.0f;
        phase = (float)(tread % TREADS_PER_SIDE) /
                (float)TREADS_PER_SIDE;
        treadPosition(phase, &y, &z);
        model->nodes[nodeIndex].translation[0] =
            tread < TREADS_PER_SIDE ? -0.12f : 0.12f;
        model->nodes[nodeIndex].translation[1] = y;
        model->nodes[nodeIndex].translation[2] = z;
        root->children[root->childCount + tread] = (int)nodeIndex;

        track = &animation->tracks[tread];
        track->node = (int)nodeIndex;
        track->path = ASSET_ANIMATION_TRANSLATION;
        track->interpolation = ASSET_INTERPOLATION_LINEAR;
        track->components = 3;
        track->keyframeCount = TREAD_KEYFRAME_COUNT;
        track->times = (float *)calloc(
            TREAD_KEYFRAME_COUNT, sizeof(*track->times));
        track->values = (float *)calloc(
            TREAD_KEYFRAME_COUNT * 3, sizeof(*track->values));
        if (model->nodes[nodeIndex].name == NULL ||
            track->times == NULL || track->values == NULL)
            return 0;
        for (keyframe = 0; keyframe < TREAD_KEYFRAME_COUNT; keyframe++)
            {
            track->times[keyframe] =
                (float)keyframe * TREAD_DURATION /
                (float)(TREAD_KEYFRAME_COUNT - 1);
            phase = (float)(tread % TREADS_PER_SIDE) /
                    (float)TREADS_PER_SIDE;
            if (keyframe < TREAD_KEYFRAME_COUNT - 1)
                phase += (float)keyframe /
                         (float)(TREAD_KEYFRAME_COUNT - 1);
            treadPosition(phase, &y, &z);
            track->values[keyframe * 3] =
                tread < TREADS_PER_SIDE ? -0.12f : 0.12f;
            track->values[keyframe * 3 + 1] = y;
            track->values[keyframe * 3 + 2] = z;
            }
        }
    root->childCount += TREAD_COUNT;
    return 1;
    }

int main(int argc, char **argv)
    {
    struct AssetModel model;
    const char *inputPath;
    const char *outputPath;
    char error[256];
    int result;

    inputPath = argc > 1 ? argv[1] :
        "assets/models/vehicles/tank.glb";
    outputPath = argc > 2 ? argv[2] : inputPath;
    assetModelInitialize(&model);
    error[0] = '\0';
    result = assetModelLoadGLB(inputPath, &model, error, sizeof(error));
    if (result && !addAnimatedTreads(&model))
        {
        strcpy(error, "could not add animated tank treads");
        result = 0;
        }
    if (result)
        result = assetModelWriteGLB(outputPath, &model,
                                    error, sizeof(error));
    if (!result)
        fprintf(stderr, "Could not generate animated tank: %s\n", error);
    assetModelDestroy(&model);
    return result ? 0 : 1;
    }
