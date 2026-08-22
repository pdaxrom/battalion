/***********************************************************************/
/* Size-aware chase-camera framing for data-driven player entities.    */
/***********************************************************************/

#include "camera_view.h"

#include <math.h>

#define CHASE_MAX_DISTANCE 1.40f
#define CHASE_EYE_HEIGHT 0.85f
#define CHASE_TARGET_HEIGHT 0.40f
#define CHASE_TARGET_AHEAD_RATIO (2.0f / 7.0f)
#define CHASE_MIN_DISTANCE_HEIGHTS 0.25f
#define CHASE_MAX_DISTANCE_HEIGHTS 64.0f
#define CHASE_NEAR_RATIO 0.25f
#define CHASE_MIN_NEAR 0.005f
#define CHASE_MAX_NEAR 0.10f
#define OVERVIEW_HALF_FOV_TANGENT 0.57735026919f
#define CAMERA_PI 3.14159265358979323846

static float clippedNearPlane(float distance)
    {
    float nearClip;

    nearClip = distance * CHASE_NEAR_RATIO;
    if (nearClip < CHASE_MIN_NEAR)
        nearClip = CHASE_MIN_NEAR;
    if (nearClip > CHASE_MAX_NEAR)
        nearClip = CHASE_MAX_NEAR;
    return nearClip;
    }

static float projectedChaseHeight(float height, float distance)
    {
    double viewPitch;
    double bottomPitch;
    double topPitch;

    viewPitch = atan2((double)((CHASE_TARGET_HEIGHT - CHASE_EYE_HEIGHT) *
                              height),
                      (double)(distance *
                               (1.0f + CHASE_TARGET_AHEAD_RATIO)));
    bottomPitch = atan2((double)(-CHASE_EYE_HEIGHT * height),
                        (double)distance) - viewPitch;
    topPitch = atan2((double)((1.0f - CHASE_EYE_HEIGHT) * height),
                     (double)distance) - viewPitch;
    return (float)((tan(topPitch) - tan(bottomPitch)) * 0.5);
    }

static float chaseDistance(float height, float heightFraction)
    {
    float low;
    float high;
    float middle;
    int iteration;

    low = height * CHASE_MIN_DISTANCE_HEIGHTS;
    high = height * CHASE_MAX_DISTANCE_HEIGHTS;
    for (iteration = 0; iteration < 32; iteration++)
        {
        middle = (low + high) * 0.5f;
        if (projectedChaseHeight(height, middle) > heightFraction)
            low = middle;
        else
            high = middle;
        }
    return high;
    }

int chaseCameraFrame(float objectBottom, float objectHeight,
                     float heightFraction,
                     struct ChaseCameraFrame *frame)
    {
    float distance;

    if (frame == 0 || objectHeight <= 0.0f ||
        heightFraction <= 0.0f || heightFraction >= 1.0f)
        return 0;
    distance = chaseDistance(objectHeight, heightFraction);
    if (distance > CHASE_MAX_DISTANCE)
        distance = CHASE_MAX_DISTANCE;
    frame->distance = distance;
    frame->nearClip = clippedNearPlane(distance);
    frame->eyeY = objectBottom + objectHeight * CHASE_EYE_HEIGHT;
    frame->targetY = objectBottom + objectHeight * CHASE_TARGET_HEIGHT;
    frame->targetZ = distance * CHASE_TARGET_AHEAD_RATIO;
    return 1;
    }

int overviewCameraFrame(float objectBottom, float objectHeight,
                        float heightFraction, float pitchDegrees,
                        struct OverviewCameraFrame *frame)
    {
    double cosine;
    float distance;
    double halfHeight;
    double root;
    double sine;

    if (frame == 0 || objectHeight <= 0.0f ||
        heightFraction <= 0.0f || heightFraction >= 1.0f ||
        pitchDegrees < 0.0f || pitchDegrees >= 90.0f)
        return 0;
    halfHeight = objectHeight * 0.5;
    cosine = cos(pitchDegrees * CAMERA_PI / 180.0);
    sine = sin(pitchDegrees * CAMERA_PI / 180.0);
    root = sqrt(halfHeight * halfHeight * cosine * cosine +
                4.0 * heightFraction * heightFraction *
                OVERVIEW_HALF_FOV_TANGENT * OVERVIEW_HALF_FOV_TANGENT *
                halfHeight * halfHeight * sine * sine);
    distance = (float)((halfHeight * cosine + root) /
        (2.0 * heightFraction * OVERVIEW_HALF_FOV_TANGENT));
    frame->distance = distance;
    frame->nearClip = clippedNearPlane(distance);
    frame->centerY = objectBottom + objectHeight * 0.5f;
    return 1;
    }
