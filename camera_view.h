#ifndef BATTALION_CAMERA_VIEW_H
#define BATTALION_CAMERA_VIEW_H

#define CAMERA_STEREO_OFF 0
#define CAMERA_STEREO_TOP_BOTTOM 1
#define CAMERA_STEREO_SIDE_BY_SIDE 2

#define CAMERA_STEREO_RIGHT_EYE 0
#define CAMERA_STEREO_LEFT_EYE 1

struct CameraViewport
    {
    int x;
    int y;
    int width;
    int height;
    };

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

float cameraViewportAspect(int width, int height);
int cameraStereoLayoutFromName(const char *name, int *layout);
const char *cameraStereoLayoutName(int layout);
int cameraStereoViewport(int layout, int eye, int width, int height,
                         struct CameraViewport *viewport);
int chaseCameraFrame(float objectBottom, float objectHeight,
                     float heightFraction,
                     struct ChaseCameraFrame *frame);
int overviewCameraFrame(float objectBottom, float objectHeight,
                        float heightFraction, float pitchDegrees,
                        struct OverviewCameraFrame *frame);

#endif
