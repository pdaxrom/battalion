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
/* Renderer bridge for external asset meshes.                          */
/***********************************************************************/

#include "battalion.h"
#include "asset.h"
#include "asset_animation.h"
#include "asset_render.h"
#include "asset_skin.h"

#include <math.h>
#include <string.h>

static void identityMatrix(float matrix[16])
    {
    int component;

    for (component = 0; component < 16; component++)
	matrix[component] = component % 5 == 0 ? 1.0f : 0.0f;
    }

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

static void nodeMatrix(const struct AssetNode *node,
		       const struct AssetNodePose *pose, float matrix[16])
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

    identityMatrix(matrix);
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

static void prepareTextures(struct AssetModel *model)
    {
    struct AssetTexture *texture;
    size_t textureIndex;

    for (textureIndex = 0; textureIndex < model->textureCount; textureIndex++)
        {
        texture = &model->textures[textureIndex];
        if (texture->rendererTexture != 0 || texture->pixels == NULL ||
            texture->width <= 0 || texture->height <= 0)
            continue;
        glGenTextures(1, &texture->rendererTexture);
        glBindTexture(GL_TEXTURE_2D, texture->rendererTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                     texture->width, texture->height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, texture->pixels);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                        texture->repeat ? GL_REPEAT : GL_CLAMP);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                        texture->repeat ? GL_REPEAT : GL_CLAMP);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                        texture->linear ? GL_LINEAR : GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        texture->linear ? GL_LINEAR : GL_NEAREST);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        }
    }

static void drawMeshPart(const struct AssetMesh *mesh,
                         const struct AssetMeshPart *part,
                         const float matrix[16],
                         const struct AssetPose *pose,
                         const struct AssetSkinPalette *palette)
    {
    const struct AssetVertex *vertex;
    float position[3];
    float localPosition[3];
    float localNormal[3];
    float color[4];
    size_t vertexIndex;

    GLfloat savedSize;
    GLboolean blending;

    blending = glIsEnabled(GL_BLEND);
    if (part->blend && !blending)
	glEnable(GL_BLEND);

    if (mesh->primitive == ASSET_PRIMITIVE_LINES)
	{
	glGetFloatv(GL_LINE_WIDTH, &savedSize);
	if (mesh->lineWidth > 0.0f)
	    glLineWidth(mesh->lineWidth);
	glBegin(GL_LINES);
	}
    else if (mesh->primitive == ASSET_PRIMITIVE_POINTS)
	{
	glGetFloatv(GL_POINT_SIZE, &savedSize);
	if (mesh->pointSize > 0.0f)
	    glPointSize(mesh->pointSize);
	glBegin(GL_POINTS);
	}
    else
	glBegin(GL_TRIANGLES);
    for (vertexIndex = part->firstVertex;
         vertexIndex < part->firstVertex + part->vertexCount; vertexIndex++)
	{
	vertex = &mesh->vertices[vertexIndex];
	assetSkinVertex(palette, vertex, localPosition, localNormal);
	position[0] = matrix[0] * localPosition[0] +
	              matrix[4] * localPosition[1] +
	              matrix[8] * localPosition[2] + matrix[12];
	position[1] = matrix[1] * localPosition[0] +
	              matrix[5] * localPosition[1] +
	              matrix[9] * localPosition[2] + matrix[13];
	position[2] = matrix[2] * localPosition[0] +
	              matrix[6] * localPosition[1] +
	              matrix[10] * localPosition[2] + matrix[14];
	if (!mesh->useCurrentColor)
	    {
	    color[0] = vertex->color[0] +
	               (pose != NULL ? pose->colorOffset[0] : 0.0f);
	    color[1] = vertex->color[1] +
	               (pose != NULL ? pose->colorOffset[1] : 0.0f);
	    color[2] = vertex->color[2] +
	               (pose != NULL ? pose->colorOffset[2] : 0.0f);
	    color[0] = color[0] > 1.0f ? 1.0f : color[0];
	    color[1] = color[1] > 1.0f ? 1.0f : color[1];
	    color[2] = color[2] > 1.0f ? 1.0f : color[2];
	    color[3] = vertex->color[3];
	    glColor4fv(color);
	    }
	glTexCoord2fv(vertex->texcoord);
	glVertex3fv(position);
	}
    glEnd();
    if (mesh->primitive == ASSET_PRIMITIVE_LINES)
	glLineWidth(savedSize);
    else if (mesh->primitive == ASSET_PRIMITIVE_POINTS)
	glPointSize(savedSize);
    if (part->blend && !blending)
	glDisable(GL_BLEND);
    }

static void drawMesh(const struct AssetModel *model,
                     const struct AssetMesh *mesh, const float matrix[16],
                     const struct AssetPose *pose,
                     const struct AssetSkinPalette *palette)
    {
    struct AssetMeshPart fallbackPart;
    const struct AssetMeshPart *part;
    GLboolean texturing;
    size_t partIndex;

    texturing = glIsEnabled(GL_TEXTURE_2D);
    fallbackPart.firstVertex = 0;
    fallbackPart.vertexCount = mesh->vertexCount;
    fallbackPart.texture = -1;
    fallbackPart.blend = mesh->blend;
    for (partIndex = 0;
         partIndex < (mesh->partCount > 0 ? mesh->partCount : 1);
         partIndex++)
        {
        part = mesh->partCount > 0 ? &mesh->parts[partIndex] : &fallbackPart;
        if (part->texture >= 0 && part->texture < (int)model->textureCount &&
            model->textures[part->texture].rendererTexture != 0)
            {
            glBindTexture(GL_TEXTURE_2D,
                          model->textures[part->texture].rendererTexture);
            glEnable(GL_TEXTURE_2D);
            }
        else
            glDisable(GL_TEXTURE_2D);
        drawMeshPart(mesh, part, matrix, pose, palette);
        }
    if (texturing)
        glEnable(GL_TEXTURE_2D);
    else
        glDisable(GL_TEXTURE_2D);
    }

static void drawNode(const struct AssetModel *model, int nodeIndex,
		     const struct AssetPose *pose,
		     const float parentMatrix[16], size_t depth)
    {
    const struct AssetNode *node;
    float localMatrix[16];
    float worldMatrix[16];
    struct AssetSkinPalette palette;
    const struct AssetSkinPalette *selectedPalette;
    size_t child;

    if (nodeIndex < 0 || nodeIndex >= (int)model->nodeCount ||
	depth >= model->nodeCount)
	return;
    if (pose != NULL && pose->nodeCount == model->nodeCount &&
        !pose->nodes[nodeIndex].visible)
        return;
    node = &model->nodes[nodeIndex];
    nodeMatrix(node, pose != NULL && pose->nodeCount == model->nodeCount ?
	       &pose->nodes[nodeIndex] : NULL, localMatrix);
    multiplyMatrix(worldMatrix, parentMatrix, localMatrix);
    if (node->mesh >= 0 && node->mesh < (int)model->meshCount)
	{
	assetSkinPaletteInitialize(&palette);
	selectedPalette = assetSkinPaletteBuild(model, nodeIndex, pose,
	                                         &palette) ? &palette : NULL;
	drawMesh(model, &model->meshes[node->mesh], worldMatrix, pose,
                 selectedPalette);
	assetSkinPaletteDestroy(&palette);
	}
    for (child = 0; child < node->childCount; child++)
    	drawNode(model, node->children[child], pose, worldMatrix, depth + 1);
    }

void assetModelDraw(const struct AssetModel *model, const struct AssetPose *pose)
    {
    float identity[16];
    size_t meshIndex;
    size_t root;

    if (model == NULL)
	return;
    prepareTextures((struct AssetModel *)model);
    identityMatrix(identity);
    if (model->nodeCount > 0 && model->sceneRootCount > 0)
	for (root = 0; root < model->sceneRootCount; root++)
	    drawNode(model, model->sceneRoots[root], pose, identity, 0);
    else
	for (meshIndex = 0; meshIndex < model->meshCount; meshIndex++)
	    drawMesh(model, &model->meshes[meshIndex], identity, pose, NULL);
    }

void assetModelDrawNode(const struct AssetModel *model, int node,
                        const struct AssetPose *pose)
    {
    float identity[16];

    if (model == NULL || node < 0 || node >= (int)model->nodeCount)
        return;
    prepareTextures((struct AssetModel *)model);
    identityMatrix(identity);
    drawNode(model, node, pose, identity, 0);
    }

unsigned int assetModelCreateDisplayList(const struct AssetModel *model)
    {
    GLuint displayList;
    if (model == NULL || model->meshCount == 0)
        return 0;

    /* Upload textures immediately. Animated instances draw the model rather
       than replaying this static list, so uploads must not be list commands. */
    prepareTextures((struct AssetModel *)model);

    displayList = glGenLists(1);
    if (displayList == 0)
        return 0;

    glNewList(displayList, GL_COMPILE);
    assetModelDraw(model, NULL);
    glEndList();
    return displayList;
    }

unsigned int assetLoadGLBDisplayList(const char *path,
                                     char *error, size_t errorSize)
    {
    struct AssetModel model;
    unsigned int displayList;

    assetModelInitialize(&model);
    if (!assetModelLoadGLB(path, &model, error, errorSize))
	return 0;
    displayList = assetModelCreateDisplayList(&model);
    assetModelDestroy(&model);
    if (displayList == 0 && errorSize > 0)
	{
	strncpy(error, "Could not create display list for GLB", errorSize - 1);
	error[errorSize - 1] = '\0';
	}
    return displayList;
    }
