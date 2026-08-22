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
/* Add the authored Googelon walk animation to googelon.glb.           */
/***********************************************************************/

#include "asset.h"
#include "asset_write.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LEG_KEYFRAME_COUNT 65
#define WALK_CYCLES 6.0
#define TWO_PI 6.28318530717958647692

static char *copyText(const char *text)
    {
    char *copy;

    copy = (char *)malloc(strlen(text) + 1);
    if (copy != NULL)
        strcpy(copy, text);
    return copy;
    }

static int findAnimation(const struct AssetModel *model, const char *name)
    {
    size_t index;

    for (index = 0; index < model->animationCount; index++)
        if (model->animations[index].name != NULL &&
            strcmp(model->animations[index].name, name) == 0)
            return (int)index;
    return -1;
    }

static int copyTrack(struct AssetAnimationTrack *destination,
                     const struct AssetAnimationTrack *source)
    {
    size_t valueCount;

    memset(destination, 0, sizeof(*destination));
    destination->node = source->node;
    destination->path = source->path;
    destination->interpolation = source->interpolation;
    destination->components = source->components;
    destination->keyframeCount = source->keyframeCount;
    valueCount = source->keyframeCount * (size_t)source->components;
    destination->times = (float *)malloc(
        source->keyframeCount * sizeof(*destination->times));
    destination->values = (float *)malloc(
        valueCount * sizeof(*destination->values));
    if (destination->times == NULL || destination->values == NULL)
        return 0;
    memcpy(destination->times, source->times,
           source->keyframeCount * sizeof(*destination->times));
    memcpy(destination->values, source->values,
           valueCount * sizeof(*destination->values));
    return 1;
    }

static int buildLegTrack(struct AssetAnimationTrack *track, int node,
                         float duration, int side)
    {
    double phase;
    float sign;
    size_t keyframe;

    memset(track, 0, sizeof(*track));
    track->node = node;
    track->path = ASSET_ANIMATION_TRANSLATION;
    track->interpolation = ASSET_INTERPOLATION_LINEAR;
    track->components = 3;
    track->keyframeCount = LEG_KEYFRAME_COUNT;
    track->times = (float *)calloc(
        LEG_KEYFRAME_COUNT, sizeof(*track->times));
    track->values = (float *)calloc(
        LEG_KEYFRAME_COUNT * 3, sizeof(*track->values));
    if (track->times == NULL || track->values == NULL)
        return 0;
    sign = side < 0 ? -1.0f : 1.0f;
    for (keyframe = 0; keyframe < LEG_KEYFRAME_COUNT; keyframe++)
        {
        track->times[keyframe] = duration * (float)keyframe /
            (float)(LEG_KEYFRAME_COUNT - 1);
        phase = TWO_PI * WALK_CYCLES * (double)keyframe /
            (double)(LEG_KEYFRAME_COUNT - 1);
        track->values[keyframe * 3] = sign * 0.2f;
        track->values[keyframe * 3 + 1] =
            sign * (float)cos(phase) * 0.02f;
        track->values[keyframe * 3 + 2] =
            sign * (float)sin(phase) * 0.05f;
        }
    return 1;
    }

static int addWalkAnimation(struct AssetModel *model)
    {
    struct AssetAnimation *animations;
    struct AssetAnimation *source;
    struct AssetAnimation *walk;
    int leftLeg;
    int rightLeg;
    int sourceIndex;
    size_t index;
    size_t walkIndex;

    if (findAnimation(model, "walk") >= 0)
        return 1;
    sourceIndex = findAnimation(model, "tail-sway");
    leftLeg = assetModelFindNode(model, "leg-left");
    rightLeg = assetModelFindNode(model, "leg-right");
    if (sourceIndex < 0 || leftLeg < 0 || rightLeg < 0)
        return 0;

    walkIndex = model->animationCount;
    animations = (struct AssetAnimation *)realloc(
        model->animations,
        (model->animationCount + 1) * sizeof(*model->animations));
    if (animations == NULL)
        return 0;
    model->animations = animations;
    source = &model->animations[sourceIndex];
    walk = &model->animations[walkIndex];
    memset(walk, 0, sizeof(*walk));
    walk->name = copyText("walk");
    walk->duration = source->duration;
    walk->trackCount = source->trackCount + 2;
    walk->tracks = (struct AssetAnimationTrack *)calloc(
        walk->trackCount, sizeof(*walk->tracks));
    if (walk->name == NULL || walk->tracks == NULL)
        return 0;
    model->animationCount += 1;

    for (index = 0; index < source->trackCount; index++)
        if (!copyTrack(&walk->tracks[index], &source->tracks[index]))
            return 0;
    if (!buildLegTrack(&walk->tracks[source->trackCount], leftLeg,
                       walk->duration, 1) ||
        !buildLegTrack(&walk->tracks[source->trackCount + 1], rightLeg,
                       walk->duration, -1))
        return 0;
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
        "assets/models/monsters/googelon.glb";
    outputPath = argc > 2 ? argv[2] : inputPath;
    assetModelInitialize(&model);
    error[0] = '\0';
    result = assetModelLoadGLB(inputPath, &model, error, sizeof(error));
    if (result && !addWalkAnimation(&model))
        {
        strcpy(error, "could not add Googelon walk animation");
        result = 0;
        }
    if (result)
        result = assetModelWriteGLB(outputPath, &model,
                                    error, sizeof(error));
    if (!result)
        fprintf(stderr, "Could not generate animated Googelon: %s\n",
                error);
    assetModelDestroy(&model);
    return result ? 0 : 1;
    }
