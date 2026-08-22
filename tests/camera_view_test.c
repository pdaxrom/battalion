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

static float projectedOverviewFraction(
    float height, float pitchDegrees,
    const struct OverviewCameraFrame *frame)
    {
    double cosine;
    double halfHeight;
    double sine;
    double top;
    double bottom;

    halfHeight = height * 0.5;
    cosine = cos(pitchDegrees * TEST_PI / 180.0);
    sine = sin(pitchDegrees * TEST_PI / 180.0);
    top = halfHeight * cosine /
          (frame->distance - halfHeight * sine);
    bottom = -halfHeight * cosine /
             (frame->distance + halfHeight * sine);
    return (float)((top - bottom) /
                   (2.0 * tan(30.0 * TEST_PI / 180.0)));
    }

int main(void)
    {
    struct ChaseCameraFrame frame;
    struct ChaseCameraFrame widerFrame;
    struct OverviewCameraFrame overviewFrame;
    float fraction;

    if (!require(chaseCameraFrame(0.0f, 0.165f, 0.34f, &frame),
                 "human chase-camera frame was rejected"))
        return 1;
    if (!require(closeEnough(frame.eyeY, 0.14025f) &&
                 closeEnough(frame.targetY, 0.066f),
                 "human chase-camera frame has incorrect dimensions"))
        return 1;
    fraction = projectedHeightFraction(0.0f, 0.165f, &frame);
    if (!require(fraction >= 0.3399f && fraction <= 0.3401f,
                 "chase view must honor its configured height fraction"))
        return 1;
    if (!require(chaseCameraFrame(0.0f, 0.165f, 0.25f, &widerFrame) &&
                 widerFrame.distance > frame.distance,
                 "a smaller chase fraction must move the camera back"))
        return 1;
    if (!require(chaseCameraFrame(0.0f, 2.0f, 0.34f, &frame) &&
                 closeEnough(frame.distance, 1.4f) &&
                 closeEnough(frame.nearClip, 0.1f),
                 "large entities must retain the historical distance limit"))
        return 1;
    if (!require(overviewCameraFrame(0.0f, 0.165f, 0.25f, 0.0f,
                                     &overviewFrame),
                 "human overview-camera frame was rejected"))
        return 1;
    fraction = 0.165f /
               (2.0f * overviewFrame.distance *
                (float)tan(30.0 * TEST_PI / 180.0));
    if (!require(fraction >= 0.2499f && fraction <= 0.2501f &&
                 closeEnough(overviewFrame.centerY, 0.0825f),
                 "overview must honor its configured height fraction"))
        return 1;
    if (!require(overviewCameraFrame(0.0f, 0.165f, 0.25f, 30.0f,
                                     &overviewFrame),
                 "pitched overview-camera frame was rejected"))
        return 1;
    fraction = projectedOverviewFraction(0.165f, 30.0f, &overviewFrame);
    if (!require(fraction >= 0.2499f && fraction <= 0.2501f,
                 "pitched overview must preserve configured framing"))
        return 1;
    if (!require(!chaseCameraFrame(0.0f, 0.0f, 0.34f, &frame) &&
                 !chaseCameraFrame(0.0f, 0.165f, 0.0f, &frame) &&
                 !overviewCameraFrame(0.0f, 0.165f, 1.0f, 30.0f,
                                      &overviewFrame),
                 "invalid camera framing values must be rejected"))
        return 1;
    return 0;
    }
