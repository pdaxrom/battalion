#ifndef BATTALION_CAMERA_VIEW_H
#define BATTALION_CAMERA_VIEW_H

struct ChaseCameraFrame
    {
    float distance;
    float nearClip;
    float eyeY;
    float targetY;
    float targetZ;
    };

struct OverviewCameraFrame
    {
    float distance;
    float nearClip;
    float centerY;
    };

int chaseCameraFrame(float objectBottom, float objectHeight,
                     float heightFraction,
                     struct ChaseCameraFrame *frame);
int overviewCameraFrame(float objectBottom, float objectHeight,
                        float heightFraction, float pitchDegrees,
                        struct OverviewCameraFrame *frame);

#endif
