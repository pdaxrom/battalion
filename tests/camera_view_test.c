#include "camera_view.h"

#include <math.h>
#include <stdio.h>

#define TEST_PI 3.14159265358979323846

static int require(int condition, const char *message)
    {
    if (!condition)
        {
        fprintf(stderr, "%s\n", message);
        return 0;
        }
    return 1;
    }

static int closeEnough(float left, float right)
    {
    float difference;

    difference = left - right;
    if (difference < 0.0f)
        difference = -difference;
    return difference < 0.00001f;
    }

static float projectedHeightFraction(float bottom, float height,
                                     const struct ChaseCameraFrame *frame)
    {
    double viewPitch;
    double bottomPitch;
    double topPitch;
    double halfFov;

    viewPitch = atan2((double)(frame->targetY - frame->eyeY),
                      (double)(frame->distance + frame->targetZ));
    bottomPitch = atan2((double)(bottom - frame->eyeY),
                        (double)frame->distance) - viewPitch;
    topPitch = atan2((double)(bottom + height - frame->eyeY),
                     (double)frame->distance) - viewPitch;
    halfFov = 45.0 * TEST_PI / 180.0;
    return (float)((tan(topPitch) - tan(bottomPitch)) /
                   (2.0 * tan(halfFov)));
    }

int main(void)
    {
    struct ChaseCameraFrame frame;
    float fraction;

    if (!require(chaseCameraFrame(0.0f, 0.165f, &frame),
                 "human chase-camera frame was rejected"))
        return 1;
    if (!require(closeEnough(frame.distance, 0.2145f) &&
                 closeEnough(frame.eyeY, 0.14025f) &&
                 closeEnough(frame.targetY, 0.066f),
                 "human chase-camera frame has incorrect dimensions"))
        return 1;
    fraction = projectedHeightFraction(0.0f, 0.165f, &frame);
    if (!require(fraction >= (1.0f / 3.0f) && fraction < 0.4f,
                 "human must occupy at least one third of chase view"))
        return 1;
    if (!require(chaseCameraFrame(0.0f, 2.0f, &frame) &&
                 closeEnough(frame.distance, 1.4f) &&
                 closeEnough(frame.nearClip, 0.1f),
                 "large entities must retain the historical distance limit"))
        return 1;
    if (!require(!chaseCameraFrame(0.0f, 0.0f, &frame),
                 "zero-height camera target must be rejected"))
        return 1;
    return 0;
    }
