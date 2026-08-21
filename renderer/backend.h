#ifndef BATTALION_RENDERER_BACKEND_H
#define BATTALION_RENDERER_BACKEND_H

#include <stddef.h>

typedef struct RendererVertex {
    float position[3];
    float color[4];
    float texcoord[2];
} RendererVertex;

enum RendererPrimitive {
    RENDERER_TRIANGLES,
    RENDERER_LINES,
    RENDERER_POINTS
};

typedef struct RendererDrawState {
    float mvp[16];
    int viewport[4];
    unsigned int texture;
    int textureEnabled;
    int textureLinear;
    int textureRepeat;
    int depthTest;
    int depthWrite;
    int blending;
    int cullBackFaces;
    int wireframe;
    float pointSize;
} RendererDrawState;

/*
 * Driver contract shared by Metal and the planned GLES implementation.
 * nativeView is backend-owned (MTKView for Metal, EGL surface data for GLES).
 */
int rendererBackendInitialize(void *nativeView);
void rendererBackendResize(int width, int height);
void rendererBackendShutdown(void);
void rendererBackendClear(unsigned int buffers, const float color[4], float depth);
void rendererBackendDraw(enum RendererPrimitive primitive,
                         const RendererVertex *vertices,
                         size_t vertexCount,
                         const RendererDrawState *state);
unsigned int rendererBackendCreateLuminanceTexture(int width, int height,
                                                    const unsigned char *pixels);
void rendererBackendPresent(void);

#endif
