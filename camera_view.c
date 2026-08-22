/***********************************************************************/
/* Size-aware chase-camera framing for data-driven player entities.    */
/***********************************************************************/

#include "camera_view.h"

#define CHASE_DISTANCE_PER_HEIGHT 1.30f
#define CHASE_MAX_DISTANCE 1.40f
#define CHASE_EYE_HEIGHT 0.85f
#define CHASE_TARGET_HEIGHT 0.40f
#define CHASE_TARGET_AHEAD_RATIO (2.0f / 7.0f)
#define CHASE_NEAR_RATIO 0.25f
#define CHASE_MIN_NEAR 0.005f
#define CHASE_MAX_NEAR 0.10f

int chaseCameraFrame(float objectBottom, float objectHeight,
                     struct ChaseCameraFrame *frame)
    {
    float distance;
    float nearClip;

    if (frame == 0 || objectHeight <= 0.0f)
        return 0;
    distance = objectHeight * CHASE_DISTANCE_PER_HEIGHT;
    if (distance > CHASE_MAX_DISTANCE)
        distance = CHASE_MAX_DISTANCE;
    nearClip = distance * CHASE_NEAR_RATIO;
    if (nearClip < CHASE_MIN_NEAR)
        nearClip = CHASE_MIN_NEAR;
    if (nearClip > CHASE_MAX_NEAR)
        nearClip = CHASE_MAX_NEAR;
    frame->distance = distance;
    frame->nearClip = nearClip;
    frame->eyeY = objectBottom + objectHeight * CHASE_EYE_HEIGHT;
    frame->targetY = objectBottom + objectHeight * CHASE_TARGET_HEIGHT;
    frame->targetZ = distance * CHASE_TARGET_AHEAD_RATIO;
    return 1;
    }
