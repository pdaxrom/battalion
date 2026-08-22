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

int chaseCameraFrame(float objectBottom, float objectHeight,
                     struct ChaseCameraFrame *frame);

#endif
