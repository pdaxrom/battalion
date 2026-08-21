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

#include <stdlib.h>
#include <string.h>

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
