#include "camera_view.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

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

static float projectedSquarePixelRatio(int width, int height)
    {
    float aspect;
    float projectedWidth;
    float projectedHeight;

    aspect = cameraViewportAspect(width, height);
    projectedWidth = (1.0f / aspect) * (float)width;
    projectedHeight = (float)height;
    return projectedWidth / projectedHeight;
    }

int main(void)
    {
    struct CameraViewport viewport;
    struct ChaseCameraFrame frame;
    struct ChaseCameraFrame widerFrame;
    struct OverviewCameraFrame overviewFrame;
    float fraction;
    int stereoLayout;

    if (!require(closeEnough(cameraViewportAspect(640, 480),
                             4.0f / 3.0f) &&
                 closeEnough(cameraViewportAspect(1280, 720),
                             16.0f / 9.0f) &&
                 closeEnough(cameraViewportAspect(2560, 1080),
                             64.0f / 27.0f),
                 "viewport aspect must support common display shapes"))
        return 1;
    if (!require(closeEnough(projectedSquarePixelRatio(640, 480), 1.0f) &&
                 closeEnough(projectedSquarePixelRatio(1280, 720), 1.0f) &&
                 closeEnough(projectedSquarePixelRatio(2560, 1080), 1.0f) &&
                 closeEnough(projectedSquarePixelRatio(1280, 360), 1.0f),
                 "perspective must preserve a square in mono and stereo viewports"))
        return 1;
    if (!require(closeEnough(cameraViewportAspect(0, 720), 1.0f) &&
                 closeEnough(cameraViewportAspect(1280, 0), 1.0f),
                 "invalid viewport dimensions must use a safe aspect"))
        return 1;
    if (!require(cameraStereoLayoutFromName("off", &stereoLayout) &&
                 stereoLayout == CAMERA_STEREO_OFF &&
                 cameraStereoLayoutFromName("top-bottom", &stereoLayout) &&
                 stereoLayout == CAMERA_STEREO_TOP_BOTTOM &&
                 cameraStereoLayoutFromName("side-by-side", &stereoLayout) &&
                 stereoLayout == CAMERA_STEREO_SIDE_BY_SIDE &&
                 !cameraStereoLayoutFromName("unknown", &stereoLayout),
                 "stereo layout names must be parsed strictly"))
        return 1;
    if (!require(strcmp(cameraStereoLayoutName(CAMERA_STEREO_OFF),
                        "off") == 0 &&
                 strcmp(cameraStereoLayoutName(CAMERA_STEREO_TOP_BOTTOM),
                        "top-bottom") == 0 &&
                 strcmp(cameraStereoLayoutName(CAMERA_STEREO_SIDE_BY_SIDE),
                        "side-by-side") == 0,
                 "stereo layouts must have stable command-line names"))
        return 1;
    if (!require(cameraStereoViewport(CAMERA_STEREO_TOP_BOTTOM,
                                      CAMERA_STEREO_RIGHT_EYE,
                                      1280, 721, &viewport) &&
                 viewport.x == 0 && viewport.y == 0 &&
                 viewport.width == 1280 && viewport.height == 360,
                 "top-bottom right-eye viewport is incorrect"))
        return 1;
    if (!require(cameraStereoViewport(CAMERA_STEREO_TOP_BOTTOM,
                                      CAMERA_STEREO_LEFT_EYE,
                                      1280, 721, &viewport) &&
                 viewport.x == 0 && viewport.y == 360 &&
                 viewport.width == 1280 && viewport.height == 361,
                 "top-bottom left-eye viewport is incorrect"))
        return 1;
    if (!require(cameraStereoViewport(CAMERA_STEREO_SIDE_BY_SIDE,
                                      CAMERA_STEREO_LEFT_EYE,
                                      1281, 720, &viewport) &&
                 viewport.x == 0 && viewport.y == 0 &&
                 viewport.width == 640 && viewport.height == 720,
                 "side-by-side left-eye viewport is incorrect"))
        return 1;
    if (!require(cameraStereoViewport(CAMERA_STEREO_SIDE_BY_SIDE,
                                      CAMERA_STEREO_RIGHT_EYE,
                                      1281, 720, &viewport) &&
                 viewport.x == 640 && viewport.y == 0 &&
                 viewport.width == 641 && viewport.height == 720,
                 "side-by-side right-eye viewport is incorrect"))
        return 1;
    if (!require(!cameraStereoViewport(CAMERA_STEREO_OFF,
                                       CAMERA_STEREO_LEFT_EYE,
                                       1280, 720, &viewport) &&
                 !cameraStereoViewport(CAMERA_STEREO_TOP_BOTTOM, 2,
                                       1280, 720, &viewport),
                 "invalid stereo viewport requests must be rejected"))
        return 1;

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
