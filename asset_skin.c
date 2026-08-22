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

#include "asset.h"
#include "asset_skin.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static void multiplyMatrix(float result[16], const float left[16],
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

static int inverseMatrix(const float matrix[16], float inverse[16])
    {
    float value[16];
    float determinant;
    int component;

    value[0] = matrix[5] * matrix[10] * matrix[15] -
               matrix[5] * matrix[11] * matrix[14] -
               matrix[9] * matrix[6] * matrix[15] +
               matrix[9] * matrix[7] * matrix[14] +
               matrix[13] * matrix[6] * matrix[11] -
               matrix[13] * matrix[7] * matrix[10];
    value[4] = -matrix[4] * matrix[10] * matrix[15] +
               matrix[4] * matrix[11] * matrix[14] +
               matrix[8] * matrix[6] * matrix[15] -
               matrix[8] * matrix[7] * matrix[14] -
               matrix[12] * matrix[6] * matrix[11] +
               matrix[12] * matrix[7] * matrix[10];
    value[8] = matrix[4] * matrix[9] * matrix[15] -
               matrix[4] * matrix[11] * matrix[13] -
               matrix[8] * matrix[5] * matrix[15] +
               matrix[8] * matrix[7] * matrix[13] +
               matrix[12] * matrix[5] * matrix[11] -
               matrix[12] * matrix[7] * matrix[9];
    value[12] = -matrix[4] * matrix[9] * matrix[14] +
                matrix[4] * matrix[10] * matrix[13] +
                matrix[8] * matrix[5] * matrix[14] -
                matrix[8] * matrix[6] * matrix[13] -
                matrix[12] * matrix[5] * matrix[10] +
                matrix[12] * matrix[6] * matrix[9];
    value[1] = -matrix[1] * matrix[10] * matrix[15] +
               matrix[1] * matrix[11] * matrix[14] +
               matrix[9] * matrix[2] * matrix[15] -
               matrix[9] * matrix[3] * matrix[14] -
               matrix[13] * matrix[2] * matrix[11] +
               matrix[13] * matrix[3] * matrix[10];
    value[5] = matrix[0] * matrix[10] * matrix[15] -
               matrix[0] * matrix[11] * matrix[14] -
               matrix[8] * matrix[2] * matrix[15] +
               matrix[8] * matrix[3] * matrix[14] +
               matrix[12] * matrix[2] * matrix[11] -
               matrix[12] * matrix[3] * matrix[10];
    value[9] = -matrix[0] * matrix[9] * matrix[15] +
               matrix[0] * matrix[11] * matrix[13] +
               matrix[8] * matrix[1] * matrix[15] -
               matrix[8] * matrix[3] * matrix[13] -
               matrix[12] * matrix[1] * matrix[11] +
               matrix[12] * matrix[3] * matrix[9];
    value[13] = matrix[0] * matrix[9] * matrix[14] -
                matrix[0] * matrix[10] * matrix[13] -
                matrix[8] * matrix[1] * matrix[14] +
                matrix[8] * matrix[2] * matrix[13] +
                matrix[12] * matrix[1] * matrix[10] -
                matrix[12] * matrix[2] * matrix[9];
    value[2] = matrix[1] * matrix[6] * matrix[15] -
               matrix[1] * matrix[7] * matrix[14] -
               matrix[5] * matrix[2] * matrix[15] +
               matrix[5] * matrix[3] * matrix[14] +
               matrix[13] * matrix[2] * matrix[7] -
               matrix[13] * matrix[3] * matrix[6];
    value[6] = -matrix[0] * matrix[6] * matrix[15] +
               matrix[0] * matrix[7] * matrix[14] +
               matrix[4] * matrix[2] * matrix[15] -
               matrix[4] * matrix[3] * matrix[14] -
               matrix[12] * matrix[2] * matrix[7] +
               matrix[12] * matrix[3] * matrix[6];
    value[10] = matrix[0] * matrix[5] * matrix[15] -
                matrix[0] * matrix[7] * matrix[13] -
                matrix[4] * matrix[1] * matrix[15] +
                matrix[4] * matrix[3] * matrix[13] +
                matrix[12] * matrix[1] * matrix[7] -
                matrix[12] * matrix[3] * matrix[5];
    value[14] = -matrix[0] * matrix[5] * matrix[14] +
                matrix[0] * matrix[6] * matrix[13] +
                matrix[4] * matrix[1] * matrix[14] -
                matrix[4] * matrix[2] * matrix[13] -
                matrix[12] * matrix[1] * matrix[6] +
                matrix[12] * matrix[2] * matrix[5];
    value[3] = -matrix[1] * matrix[6] * matrix[11] +
               matrix[1] * matrix[7] * matrix[10] +
               matrix[5] * matrix[2] * matrix[11] -
               matrix[5] * matrix[3] * matrix[10] -
               matrix[9] * matrix[2] * matrix[7] +
               matrix[9] * matrix[3] * matrix[6];
    value[7] = matrix[0] * matrix[6] * matrix[11] -
               matrix[0] * matrix[7] * matrix[10] -
               matrix[4] * matrix[2] * matrix[11] +
               matrix[4] * matrix[3] * matrix[10] +
               matrix[8] * matrix[2] * matrix[7] -
               matrix[8] * matrix[3] * matrix[6];
    value[11] = -matrix[0] * matrix[5] * matrix[11] +
                matrix[0] * matrix[7] * matrix[9] +
                matrix[4] * matrix[1] * matrix[11] -
                matrix[4] * matrix[3] * matrix[9] -
                matrix[8] * matrix[1] * matrix[7] +
                matrix[8] * matrix[3] * matrix[5];
    value[15] = matrix[0] * matrix[5] * matrix[10] -
                matrix[0] * matrix[6] * matrix[9] -
                matrix[4] * matrix[1] * matrix[10] +
                matrix[4] * matrix[2] * matrix[9] +
                matrix[8] * matrix[1] * matrix[6] -
                matrix[8] * matrix[2] * matrix[5];

    determinant = matrix[0] * value[0] + matrix[1] * value[4] +
                  matrix[2] * value[8] + matrix[3] * value[12];
    if (determinant > -0.0000001f && determinant < 0.0000001f)
        return 0;
    determinant = 1.0f / determinant;
    for (component = 0; component < 16; component++)
        inverse[component] = value[component] * determinant;
    return 1;
    }

void assetSkinPaletteInitialize(struct AssetSkinPalette *palette)
    {
    if (palette == NULL)
        return;
    palette->matrices = NULL;
    palette->normalMatrices = NULL;
    palette->jointCount = 0;
    }

void assetSkinPaletteDestroy(struct AssetSkinPalette *palette)
    {
    if (palette == NULL)
        return;
    free(palette->matrices);
    free(palette->normalMatrices);
    assetSkinPaletteInitialize(palette);
    }

int assetSkinPaletteBuild(const struct AssetModel *model, int meshNode,
                          const struct AssetPose *pose,
                          struct AssetSkinPalette *palette)
    {
    const struct AssetNode *node;
    const struct AssetSkin *skin;
    float meshWorld[16];
    float inverseMesh[16];
    float jointWorld[16];
    float jointBind[16];
    float inverseJointBind[16];
    float *matrix;
    float *normal;
    size_t joint;

    if (model == NULL || palette == NULL || meshNode < 0 ||
        meshNode >= (int)model->nodeCount)
        return 0;
    node = &model->nodes[meshNode];
    if (node->skin < 0 || node->skin >= (int)model->skinCount)
        return 0;
    skin = &model->skins[node->skin];
    if (skin->jointCount == 0 ||
        !assetModelNodeWorldMatrix(model, meshNode, pose, meshWorld) ||
        !inverseMatrix(meshWorld, inverseMesh))
        return 0;

    assetSkinPaletteDestroy(palette);
    palette->matrices = (float *)calloc(skin->jointCount * 16,
                                        sizeof(*palette->matrices));
    palette->normalMatrices = (float *)calloc(skin->jointCount * 9,
                                              sizeof(*palette->normalMatrices));
    if (palette->matrices == NULL || palette->normalMatrices == NULL)
        {
        assetSkinPaletteDestroy(palette);
        return 0;
        }
    palette->jointCount = skin->jointCount;
    for (joint = 0; joint < skin->jointCount; joint++)
        {
        matrix = palette->matrices + joint * 16;
        normal = palette->normalMatrices + joint * 9;
        if (!assetModelNodeWorldMatrix(model, skin->joints[joint], pose,
                                       jointWorld))
            {
            assetSkinPaletteDestroy(palette);
            return 0;
            }
        multiplyMatrix(jointBind, jointWorld,
                       skin->inverseBindMatrices + joint * 16);
        multiplyMatrix(matrix, inverseMesh, jointBind);
        if (inverseMatrix(matrix, inverseJointBind))
            {
            normal[0] = inverseJointBind[0];
            normal[1] = inverseJointBind[1];
            normal[2] = inverseJointBind[2];
            normal[3] = inverseJointBind[4];
            normal[4] = inverseJointBind[5];
            normal[5] = inverseJointBind[6];
            normal[6] = inverseJointBind[8];
            normal[7] = inverseJointBind[9];
            normal[8] = inverseJointBind[10];
            }
        else
            {
            normal[0] = matrix[0]; normal[1] = matrix[4];
            normal[2] = matrix[8]; normal[3] = matrix[1];
            normal[4] = matrix[5]; normal[5] = matrix[9];
            normal[6] = matrix[2]; normal[7] = matrix[6];
            normal[8] = matrix[10];
            }
        }
    return 1;
    }

void assetSkinVertex(const struct AssetSkinPalette *palette,
                     const struct AssetVertex *vertex,
                     float position[3], float normal[3])
    {
    const float *matrix;
    const float *normalMatrix;
    float weight;
    float weightSum;
    float normalLength;
    int influence;

    position[0] = position[1] = position[2] = 0.0f;
    normal[0] = normal[1] = normal[2] = 0.0f;
    weightSum = 0.0f;
    if (palette != NULL)
        for (influence = 0; influence < 4; influence++)
            {
            weight = vertex->weights[influence];
            if (weight <= 0.0f ||
                vertex->joints[influence] >= palette->jointCount)
                continue;
            matrix = palette->matrices + vertex->joints[influence] * 16;
            normalMatrix = palette->normalMatrices +
                           vertex->joints[influence] * 9;
            position[0] += weight * (matrix[0] * vertex->position[0] +
                                      matrix[4] * vertex->position[1] +
                                      matrix[8] * vertex->position[2] +
                                      matrix[12]);
            position[1] += weight * (matrix[1] * vertex->position[0] +
                                      matrix[5] * vertex->position[1] +
                                      matrix[9] * vertex->position[2] +
                                      matrix[13]);
            position[2] += weight * (matrix[2] * vertex->position[0] +
                                      matrix[6] * vertex->position[1] +
                                      matrix[10] * vertex->position[2] +
                                      matrix[14]);
            normal[0] += weight * (normalMatrix[0] * vertex->normal[0] +
                                    normalMatrix[1] * vertex->normal[1] +
                                    normalMatrix[2] * vertex->normal[2]);
            normal[1] += weight * (normalMatrix[3] * vertex->normal[0] +
                                    normalMatrix[4] * vertex->normal[1] +
                                    normalMatrix[5] * vertex->normal[2]);
            normal[2] += weight * (normalMatrix[6] * vertex->normal[0] +
                                    normalMatrix[7] * vertex->normal[1] +
                                    normalMatrix[8] * vertex->normal[2]);
            weightSum += weight;
            }
    if (weightSum <= 0.0000001f)
        {
        memcpy(position, vertex->position, 3 * sizeof(*position));
        memcpy(normal, vertex->normal, 3 * sizeof(*normal));
        return;
        }
    position[0] /= weightSum;
    position[1] /= weightSum;
    position[2] /= weightSum;
    normalLength = (float)sqrt((double)(normal[0] * normal[0] +
                                        normal[1] * normal[1] +
                                        normal[2] * normal[2]));
    if (normalLength > 0.0000001f)
        {
        normal[0] /= normalLength;
        normal[1] /= normalLength;
        normal[2] /= normalLength;
        }
    }
