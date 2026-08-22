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
/* Unit and packed-humanoid regression tests for glTF skinning.       */
/***********************************************************************/

#include "asset.h"
#include "asset_animation.h"
#include "asset_skin.h"
#include "entity_catalog.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void identity(float matrix[16])
    {
    int component;

    for (component = 0; component < 16; component++)
        matrix[component] = component % 5 == 0 ? 1.0f : 0.0f;
    }

static void initializeNode(struct AssetNode *node)
    {
    memset(node, 0, sizeof(*node));
    node->mesh = -1;
    node->skin = -1;
    node->rotation[3] = 1.0f;
    node->scale[0] = node->scale[1] = node->scale[2] = 1.0f;
    identity(node->matrix);
    }

static int closeValue(float actual, float expected)
    {
    return (float)fabs((double)(actual - expected)) < 0.0001f;
    }

static int finiteValue(float value)
    {
    return value == value && value > -100000000.0f && value < 100000000.0f;
    }

static int makeModel(struct AssetModel *model, int jointCount, int skinCount,
                     int meshNodeCount)
    {
    int nodeCount;
    int node;
    int child;

    assetModelInitialize(model);
    nodeCount = 1 + meshNodeCount + jointCount;
    model->nodes = (struct AssetNode *)calloc((size_t)nodeCount,
                                               sizeof(*model->nodes));
    model->skins = (struct AssetSkin *)calloc((size_t)skinCount,
                                               sizeof(*model->skins));
    model->sceneRoots = (int *)calloc(1, sizeof(*model->sceneRoots));
    if (model->nodes == NULL || model->skins == NULL ||
        model->sceneRoots == NULL)
        return 0;
    model->nodeCount = (size_t)nodeCount;
    model->skinCount = (size_t)skinCount;
    model->sceneRootCount = 1;
    model->sceneRoots[0] = 0;
    for (node = 0; node < nodeCount; node++)
        initializeNode(&model->nodes[node]);
    model->nodes[0].children = (int *)calloc((size_t)(nodeCount - 1),
                                             sizeof(int));
    if (model->nodes[0].children == NULL)
        return 0;
    model->nodes[0].childCount = (size_t)(nodeCount - 1);
    for (child = 1; child < nodeCount; child++)
        model->nodes[0].children[child - 1] = child;
    return 1;
    }

static int allocateSkin(struct AssetSkin *skin, int jointCount)
    {
    int joint;

    skin->skeleton = -1;
    skin->joints = (int *)calloc((size_t)jointCount, sizeof(*skin->joints));
    skin->inverseBindMatrices = (float *)calloc(
        (size_t)jointCount * 16, sizeof(*skin->inverseBindMatrices));
    if (skin->joints == NULL || skin->inverseBindMatrices == NULL)
        return 0;
    skin->jointCount = (size_t)jointCount;
    for (joint = 0; joint < jointCount; joint++)
        identity(skin->inverseBindMatrices + joint * 16);
    return 1;
    }

static void initializeVertex(struct AssetVertex *vertex)
    {
    memset(vertex, 0, sizeof(*vertex));
    vertex->normal[1] = 1.0f;
    vertex->color[0] = vertex->color[1] = vertex->color[2] =
        vertex->color[3] = 1.0f;
    }

static int testOneBone(void)
    {
    struct AssetModel model;
    struct AssetSkinPalette palette;
    struct AssetVertex vertex;
    float position[3];
    float normal[3];
    int success;

    success = makeModel(&model, 1, 1, 1) &&
              allocateSkin(&model.skins[0], 1);
    if (!success)
        return 0;
    model.nodes[1].skin = 0;
    model.skins[0].joints[0] = 2;
    model.nodes[2].translation[0] = 2.0f;
    initializeVertex(&vertex);
    vertex.position[0] = 1.0f;
    vertex.weights[0] = 1.0f;
    assetSkinPaletteInitialize(&palette);
    success = assetSkinPaletteBuild(&model, 1, NULL, &palette);
    if (success)
        {
        assetSkinVertex(&palette, &vertex, position, normal);
        success = closeValue(position[0], 3.0f) &&
                  closeValue(position[1], 0.0f) &&
                  closeValue(normal[1], 1.0f);
        }
    assetSkinPaletteDestroy(&palette);
    assetModelDestroy(&model);
    return success;
    }

static int testWeightedBones(void)
    {
    struct AssetModel model;
    struct AssetSkinPalette palette;
    struct AssetVertex vertex;
    float position[3];
    float normal[3];
    int success;

    success = makeModel(&model, 2, 1, 1) &&
              allocateSkin(&model.skins[0], 2);
    if (!success)
        return 0;
    model.nodes[1].skin = 0;
    model.skins[0].joints[0] = 2;
    model.skins[0].joints[1] = 3;
    model.nodes[2].translation[0] = 2.0f;
    model.nodes[3].translation[1] = 4.0f;
    initializeVertex(&vertex);
    vertex.joints[0] = 0;
    vertex.joints[1] = 1;
    vertex.weights[0] = 0.25f;
    vertex.weights[1] = 0.75f;
    assetSkinPaletteInitialize(&palette);
    success = assetSkinPaletteBuild(&model, 1, NULL, &palette);
    if (success)
        {
        assetSkinVertex(&palette, &vertex, position, normal);
        success = closeValue(position[0], 0.5f) &&
                  closeValue(position[1], 3.0f);
        }
    assetSkinPaletteDestroy(&palette);
    assetModelDestroy(&model);
    return success;
    }

static int testInverseBindMatrix(void)
    {
    struct AssetModel model;
    struct AssetSkinPalette palette;
    struct AssetVertex vertex;
    float position[3];
    float normal[3];
    int success;

    success = makeModel(&model, 1, 1, 1) &&
              allocateSkin(&model.skins[0], 1);
    if (!success)
        return 0;
    model.nodes[1].skin = 0;
    model.skins[0].joints[0] = 2;
    model.nodes[2].translation[0] = 5.0f;
    model.skins[0].inverseBindMatrices[12] = -2.0f;
    initializeVertex(&vertex);
    vertex.weights[0] = 1.0f;
    assetSkinPaletteInitialize(&palette);
    success = assetSkinPaletteBuild(&model, 1, NULL, &palette);
    if (success)
        {
        assetSkinVertex(&palette, &vertex, position, normal);
        success = closeValue(position[0], 3.0f);
        }
    assetSkinPaletteDestroy(&palette);
    assetModelDestroy(&model);
    return success;
    }

static int testIndependentSkins(void)
    {
    struct AssetModel model;
    struct AssetSkinPalette first;
    struct AssetSkinPalette second;
    struct AssetVertex vertex;
    float firstPosition[3];
    float secondPosition[3];
    float normal[3];
    int success;

    success = makeModel(&model, 2, 2, 2) &&
              allocateSkin(&model.skins[0], 1) &&
              allocateSkin(&model.skins[1], 1);
    if (!success)
        return 0;
    model.nodes[1].skin = 0;
    model.nodes[2].skin = 1;
    model.skins[0].joints[0] = 3;
    model.skins[1].joints[0] = 4;
    model.nodes[3].translation[0] = 3.0f;
    model.nodes[4].translation[0] = 14.0f;
    model.skins[0].inverseBindMatrices[12] = -1.0f;
    model.skins[1].inverseBindMatrices[12] = -10.0f;
    initializeVertex(&vertex);
    vertex.weights[0] = 1.0f;
    assetSkinPaletteInitialize(&first);
    assetSkinPaletteInitialize(&second);
    success = assetSkinPaletteBuild(&model, 1, NULL, &first) &&
              assetSkinPaletteBuild(&model, 2, NULL, &second);
    if (success)
        {
        assetSkinVertex(&first, &vertex, firstPosition, normal);
        assetSkinVertex(&second, &vertex, secondPosition, normal);
        success = closeValue(firstPosition[0], 2.0f) &&
                  closeValue(secondPosition[0], 4.0f) &&
                  !closeValue(firstPosition[0], secondPosition[0]);
        }
    assetSkinPaletteDestroy(&first);
    assetSkinPaletteDestroy(&second);
    assetModelDestroy(&model);
    return success;
    }

static int findAnimation(const struct AssetModel *model, const char *name)
    {
    size_t animation;

    for (animation = 0; animation < model->animationCount; animation++)
        if (model->animations[animation].name != NULL &&
            strcmp(model->animations[animation].name, name) == 0)
            return (int)animation;
    return -1;
    }

static float largestExtent(const float minimum[3], const float maximum[3])
    {
    float result;
    float extent;
    int component;

    result = 0.0f;
    for (component = 0; component < 3; component++)
        {
        extent = maximum[component] - minimum[component];
        if (extent > result)
            result = extent;
        }
    return result;
    }

static int validatePackedPose(const struct AssetModel *model,
                              const struct AssetPose *pose,
                              float referenceExtent)
    {
    struct AssetSkinPalette palette;
    const struct AssetMesh *mesh;
    float minimum[3];
    float maximum[3];
    float position[3];
    float normal[3];
    float extent;
    size_t node;
    size_t vertex;
    int component;
    int skinnedNodes;

    if (!assetModelPoseBounds(model, pose, minimum, maximum))
        return 0;
    extent = largestExtent(minimum, maximum);
    if (!finiteValue(extent) || extent <= referenceExtent * 0.2f ||
        extent >= referenceExtent * 3.0f)
        return 0;
    skinnedNodes = 0;
    for (node = 0; node < model->nodeCount; node++)
        if (model->nodes[node].skin >= 0)
            {
            if (model->nodes[node].mesh < 0 ||
                model->nodes[node].mesh >= (int)model->meshCount)
                return 0;
            assetSkinPaletteInitialize(&palette);
            if (!assetSkinPaletteBuild(model, (int)node, pose, &palette))
                return 0;
            mesh = &model->meshes[model->nodes[node].mesh];
            for (vertex = 0; vertex < mesh->vertexCount; vertex++)
                {
                assetSkinVertex(&palette, &mesh->vertices[vertex],
                                position, normal);
                for (component = 0; component < 3; component++)
                    if (!finiteValue(position[component]) ||
                        !finiteValue(normal[component]))
                        {
                        assetSkinPaletteDestroy(&palette);
                        return 0;
                        }
                }
            assetSkinPaletteDestroy(&palette);
            skinnedNodes++;
            }
    return skinnedNodes == 10;
    }

static int testMoiraRegression(void)
    {
    struct AssetModel model;
    struct AssetPose pose;
    struct EntityCatalog catalog;
    struct EntityArchetype resolvedMoira;
    const struct EntityArchetype *moira;
    char error[256];
    float minimum[3];
    float maximum[3];
    float referenceExtent;
    float time;
    int forward;
    int backward;
    int clip;
    int sample;
    int success;
    int texturedMeshes;
    int decodedTextures;
    size_t meshIndex;
    size_t textureIndex;

    assetModelInitialize(&model);
    assetPoseInitialize(&pose);
    error[0] = '\0';
    if (!assetModelLoadGLB("assets/models/npcs/moira-brown.glb", &model,
                           error, sizeof(error)))
        {
        fprintf(stderr, "Moira regression GLB failed to load: %s\n", error);
        return 0;
        }
    forward = findAnimation(&model, "Forward");
    backward = findAnimation(&model, "Backward");
    success = model.skinCount == 10 && forward >= 0 && backward >= 0 &&
              model.animations[forward].trackCount > 0 &&
              model.animations[backward].trackCount > 0 &&
              assetModelPoseBounds(&model, NULL, minimum, maximum);
    texturedMeshes = 0;
    for (meshIndex = 0; meshIndex < model.meshCount; meshIndex++)
        {
        if (model.meshes[meshIndex].partCount != 1 ||
            model.meshes[meshIndex].parts[0].vertexCount !=
                model.meshes[meshIndex].vertexCount)
            success = 0;
        if (model.meshes[meshIndex].partCount == 1 &&
            model.meshes[meshIndex].parts[0].texture >= 0)
            texturedMeshes++;
        }
    decodedTextures = 0;
    for (textureIndex = 0; textureIndex < model.textureCount; textureIndex++)
        if (model.textures[textureIndex].pixels != NULL)
            {
            if (model.textures[textureIndex].width <= 0 ||
                model.textures[textureIndex].height <= 0)
                success = 0;
            decodedTextures++;
            }
    if (texturedMeshes != 12 || decodedTextures != 9 ||
        model.textureCount != 18 || model.textures[0].width != 2048 ||
        model.textures[0].height != 2048 ||
        (model.textures[0].pixels[0] == 255 &&
         model.textures[0].pixels[1] == 255 &&
         model.textures[0].pixels[2] == 255))
        success = 0;
    entityCatalogInitialize(&catalog);
    moira = NULL;
    if (success && entityCatalogLoad("assets", "entities/catalog.txt",
                                     &catalog, error, sizeof(error)))
        moira = entityCatalogFind(&catalog, "moira-brown");
    if (moira == NULL)
        success = 0;
    else
        {
        resolvedMoira = *moira;
        if (!entityArchetypeResolveScale(&resolvedMoira,
                                         maximum[1] - minimum[1]) ||
            !closeValue((maximum[1] - minimum[1]) * resolvedMoira.scale,
                        0.165f))
            success = 0;
        }
    referenceExtent = success ? largestExtent(minimum, maximum) : 0.0f;
    if (referenceExtent <= 0.0f)
        success = 0;
    for (clip = 0; clip < 2 && success; clip++)
        for (sample = 0; sample <= 8 && success; sample++)
            {
            int animationIndex;

            animationIndex = clip == 0 ? forward : backward;
            time = model.animations[animationIndex].duration *
                   (float)sample / 8.0f;
            if (!assetPoseSample(&model, (size_t)animationIndex, time, 0,
                                 &pose) ||
                !validatePackedPose(&model, &pose, referenceExtent))
                success = 0;
            }
    assetPoseDestroy(&pose);
    assetModelDestroy(&model);
    entityCatalogDestroy(&catalog);
    return success;
    }

int main(void)
    {
    if (!testOneBone())
        {
        fprintf(stderr, "one-bone skin test failed\n");
        return 1;
        }
    if (!testWeightedBones())
        {
        fprintf(stderr, "weighted multi-bone skin test failed\n");
        return 1;
        }
    if (!testInverseBindMatrix())
        {
        fprintf(stderr, "inverse bind matrix test failed\n");
        return 1;
        }
    if (!testIndependentSkins())
        {
        fprintf(stderr, "independent incompatible skins test failed\n");
        return 1;
        }
    if (!testMoiraRegression())
        {
        fprintf(stderr, "packed humanoid Forward/Backward regression failed\n");
        return 1;
        }
    return 0;
    }
