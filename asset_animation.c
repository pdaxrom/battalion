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
/* Renderer-neutral sampling of animation clips loaded from assets.    */
/***********************************************************************/

#include "asset.h"
#include "asset_animation.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

void assetPoseInitialize(struct AssetPose *pose)
    {
    pose->nodes = NULL;
    pose->nodeCount = 0;
    }

void assetPoseDestroy(struct AssetPose *pose)
    {
    if (pose == NULL)
        return;
    free(pose->nodes);
    assetPoseInitialize(pose);
    }

static void resetPose(const struct AssetModel *model, struct AssetPose *pose)
    {
    size_t node;

    for (node = 0; node < model->nodeCount; node++)
        {
        memcpy(pose->nodes[node].translation, model->nodes[node].translation,
               sizeof(pose->nodes[node].translation));
        memcpy(pose->nodes[node].rotation, model->nodes[node].rotation,
               sizeof(pose->nodes[node].rotation));
        memcpy(pose->nodes[node].scale, model->nodes[node].scale,
               sizeof(pose->nodes[node].scale));
        }
    }

static void normalizeQuaternion(float value[4])
    {
    float length;

    length = (float)sqrt((double)(value[0] * value[0] +
                                  value[1] * value[1] +
                                  value[2] * value[2] +
                                  value[3] * value[3]));
    if (length <= 0.0f)
        {
        value[0] = value[1] = value[2] = 0.0f;
        value[3] = 1.0f;
        return;
        }
    value[0] /= length;
    value[1] /= length;
    value[2] /= length;
    value[3] /= length;
    }

static void sampleTrack(const struct AssetAnimationTrack *track, float time,
                        float *destination)
    {
    size_t right;
    size_t left;
    int component;
    float amount;
    float range;
    float sign;

    if (time <= track->times[0] || track->keyframeCount == 1)
        {
        memcpy(destination, track->values,
               track->components * sizeof(*destination));
        return;
        }
    right = track->keyframeCount - 1;
    if (time >= track->times[right])
        {
        memcpy(destination, track->values + right * track->components,
               track->components * sizeof(*destination));
        return;
        }
    for (right = 1; right < track->keyframeCount; right++)
        if (time < track->times[right])
            break;
    left = right - 1;
    if (track->interpolation == ASSET_INTERPOLATION_STEP)
        {
        memcpy(destination, track->values + left * track->components,
               track->components * sizeof(*destination));
        return;
        }
    range = track->times[right] - track->times[left];
    amount = range > 0.0f ? (time - track->times[left]) / range : 0.0f;
    sign = 1.0f;
    if (track->path == ASSET_ANIMATION_ROTATION)
        {
        float dot;

        dot = 0.0f;
        for (component = 0; component < 4; component++)
            dot += track->values[left * 4 + component] *
                   track->values[right * 4 + component];
        if (dot < 0.0f)
            sign = -1.0f;
        }
    for (component = 0; component < track->components; component++)
        destination[component] =
            track->values[left * track->components + component] *
                (1.0f - amount) +
            sign * track->values[right * track->components + component] *
                amount;
    if (track->path == ASSET_ANIMATION_ROTATION)
        normalizeQuaternion(destination);
    }

int assetPoseSample(const struct AssetModel *model, size_t animationIndex,
                    float timeSeconds, int loop, struct AssetPose *pose)
    {
    const struct AssetAnimation *animation;
    const struct AssetAnimationTrack *track;
    float *destination;
    size_t trackIndex;

    if (model == NULL || pose == NULL || animationIndex >= model->animationCount)
        return 0;
    if (pose->nodeCount != model->nodeCount)
        {
        assetPoseDestroy(pose);
        pose->nodes = (struct AssetNodePose *)calloc(model->nodeCount,
                                                     sizeof(*pose->nodes));
        if (pose->nodes == NULL && model->nodeCount > 0)
            return 0;
        pose->nodeCount = model->nodeCount;
        }
    resetPose(model, pose);
    animation = &model->animations[animationIndex];
    if (timeSeconds < 0.0f)
        timeSeconds = 0.0f;
    if (loop && animation->duration > 0.0f)
        timeSeconds = (float)fmod((double)timeSeconds,
                                  (double)animation->duration);
    else if (timeSeconds > animation->duration)
        timeSeconds = animation->duration;

    for (trackIndex = 0; trackIndex < animation->trackCount; trackIndex++)
        {
        track = &animation->tracks[trackIndex];
        if (track->node < 0 || track->node >= (int)pose->nodeCount ||
            track->keyframeCount == 0)
            continue;
        if (track->path == ASSET_ANIMATION_TRANSLATION)
            destination = pose->nodes[track->node].translation;
        else if (track->path == ASSET_ANIMATION_ROTATION)
            destination = pose->nodes[track->node].rotation;
        else
            destination = pose->nodes[track->node].scale;
        sampleTrack(track, timeSeconds, destination);
        }
    return 1;
    }
