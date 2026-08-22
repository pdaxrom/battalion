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
/* Build the animated Vapour cloud used by the external asset catalog. */
/***********************************************************************/

#include "asset.h"
#include "asset_write.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VAPOUR_PARTICLE_COUNT 45
#define VAPOUR_KEYFRAME_COUNT 9
#define VAPOUR_TRACKS_PER_PARTICLE 2
#define VAPOUR_DURATION 3.2f
#define VAPOUR_PI 3.14159265358979323846

static char *copyText(const char *text)
    {
    char *copy;

    copy = (char *)malloc(strlen(text) + 1);
    if (copy != NULL)
        strcpy(copy, text);
    return copy;
    }

static unsigned long nextRandom(unsigned long *state)
    {
    *state = (*state * 1103515245UL + 12345UL) & 0x7fffffffUL;
    return (*state / 65536UL) % 32768UL;
    }

static float randomSigned(unsigned long *state, float extent)
    {
    return ((float)nextRandom(state) / 16383.5f - 1.0f) * extent;
    }

static int allocateTrack(struct AssetAnimationTrack *track, int node,
                         int path, int components)
    {
    track->node = node;
    track->path = path;
    track->interpolation = ASSET_INTERPOLATION_LINEAR;
    track->components = components;
    track->keyframeCount = VAPOUR_KEYFRAME_COUNT;
    track->times = (float *)calloc(VAPOUR_KEYFRAME_COUNT,
                                  sizeof(*track->times));
    track->values = (float *)calloc(VAPOUR_KEYFRAME_COUNT * components,
                                   sizeof(*track->values));
    return track->times != NULL && track->values != NULL;
    }

static int buildVapour(struct AssetModel *vapour, struct AssetModel *sphere)
    {
    struct AssetAnimation *animation;
    struct AssetAnimationTrack *translation;
    struct AssetAnimationTrack *scale;
    struct AssetNode *node;
    unsigned long randomState;
    float baseX;
    float baseY;
    float baseZ;
    float phase;
    float time;
    float wave;
    char name[32];
    size_t particle;
    size_t keyframe;
    size_t vertex;

    vapour->meshes = sphere->meshes;
    vapour->meshCount = sphere->meshCount;
    sphere->meshes = NULL;
    sphere->meshCount = 0;
    if (vapour->meshCount != 1)
        return 0;
    vapour->meshes[0].useCurrentColor = 0;
    vapour->meshes[0].blend = 1;
    for (vertex = 0; vertex < vapour->meshes[0].vertexCount; vertex++)
        {
        vapour->meshes[0].vertices[vertex].color[0] = 1.0f;
        vapour->meshes[0].vertices[vertex].color[1] = 1.0f;
        vapour->meshes[0].vertices[vertex].color[2] = 1.0f;
        vapour->meshes[0].vertices[vertex].color[3] = 0.2f;
        }

    vapour->nodes = (struct AssetNode *)calloc(
        VAPOUR_PARTICLE_COUNT, sizeof(*vapour->nodes));
    vapour->sceneRoots = (int *)calloc(VAPOUR_PARTICLE_COUNT,
                                       sizeof(*vapour->sceneRoots));
    vapour->animations = (struct AssetAnimation *)calloc(
        1, sizeof(*vapour->animations));
    if (vapour->nodes == NULL || vapour->sceneRoots == NULL ||
        vapour->animations == NULL)
        return 0;
    vapour->nodeCount = VAPOUR_PARTICLE_COUNT;
    vapour->sceneRootCount = VAPOUR_PARTICLE_COUNT;
    vapour->animationCount = 1;

    animation = &vapour->animations[0];
    animation->name = copyText("cloud");
    animation->duration = VAPOUR_DURATION;
    animation->trackCount = VAPOUR_PARTICLE_COUNT *
                            VAPOUR_TRACKS_PER_PARTICLE;
    animation->tracks = (struct AssetAnimationTrack *)calloc(
        animation->trackCount, sizeof(*animation->tracks));
    if (animation->name == NULL || animation->tracks == NULL)
        return 0;

    randomState = 0x42415454UL;
    for (particle = 0; particle < VAPOUR_PARTICLE_COUNT; particle++)
        {
        node = &vapour->nodes[particle];
        sprintf(name, "particle-%02lu", (unsigned long)particle);
        node->name = copyText(name);
        node->mesh = 0;
        node->rotation[3] = 1.0f;
        node->scale[0] = node->scale[1] = node->scale[2] = 0.1f;
        vapour->sceneRoots[particle] = (int)particle;
        if (node->name == NULL)
            return 0;

        translation = &animation->tracks[particle * 2];
        scale = &animation->tracks[particle * 2 + 1];
        if (!allocateTrack(translation, (int)particle,
                           ASSET_ANIMATION_TRANSLATION, 3) ||
            !allocateTrack(scale, (int)particle,
                           ASSET_ANIMATION_SCALE, 3))
            return 0;

        baseX = randomSigned(&randomState, 0.2f);
        baseY = 0.78f + randomSigned(&randomState, 0.18f);
        baseZ = randomSigned(&randomState, 0.2f);
        phase = (float)particle * 2.39996323f;
        for (keyframe = 0; keyframe < VAPOUR_KEYFRAME_COUNT; keyframe++)
            {
            time = (float)keyframe * VAPOUR_DURATION /
                   (float)(VAPOUR_KEYFRAME_COUNT - 1);
            wave = (float)sin((double)(time * 2.0f * (float)VAPOUR_PI /
                                      VAPOUR_DURATION + phase));
            translation->times[keyframe] = time;
            translation->values[keyframe * 3] =
                baseX + 0.035f * wave;
            translation->values[keyframe * 3 + 1] =
                baseY + 0.18f * wave;
            translation->values[keyframe * 3 + 2] =
                baseZ + 0.035f * (float)cos((double)(
                    time * 2.0f * (float)VAPOUR_PI / VAPOUR_DURATION +
                    phase));
            scale->times[keyframe] = time;
            scale->values[keyframe * 3] = 0.11f + 0.07f * wave;
            scale->values[keyframe * 3 + 1] =
                scale->values[keyframe * 3];
            scale->values[keyframe * 3 + 2] =
                scale->values[keyframe * 3];
            }
        }
    return 1;
    }

int main(int argc, char **argv)
    {
    struct AssetModel sphere;
    struct AssetModel vapour;
    const char *sourcePath;
    const char *outputPath;
    char error[256];
    int result;

    sourcePath = argc > 1 ? argv[1] :
        "assets/models/effects/sphere-low.glb";
    outputPath = argc > 2 ? argv[2] :
        "assets/models/monsters/vapour.glb";
    assetModelInitialize(&sphere);
    assetModelInitialize(&vapour);
    error[0] = '\0';
    result = assetModelLoadGLB(sourcePath, &sphere, error, sizeof(error));
    if (result && !buildVapour(&vapour, &sphere))
        {
        strcpy(error, "could not construct Vapour model");
        result = 0;
        }
    if (result)
        result = assetModelWriteGLB(outputPath, &vapour,
                                    error, sizeof(error));
    if (!result)
        fprintf(stderr, "Could not generate Vapour asset: %s\n", error);
    assetModelDestroy(&vapour);
    assetModelDestroy(&sphere);
    return result ? 0 : 1;
    }
