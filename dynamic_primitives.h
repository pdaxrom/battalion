/*
 * Copyright (c) 2026 Battalion contributors.
 */

#ifndef BATTALION_DYNAMIC_PRIMITIVES_H
#define BATTALION_DYNAMIC_PRIMITIVES_H

#include <stddef.h>

void dynamicDrawLine3(const float start[3], const float end[3],
                      const float startColor[3], const float endColor[3]);
void dynamicDrawQuad3(float vertices[4][3],
                      float colors[4][3]);
void dynamicDrawSolidQuad3(float vertices[4][3],
                           const float color[4]);
void dynamicDrawLineLoop3(float vertices[4][3],
                          const float color[4]);
void dynamicDrawPoints3(const float (*vertices)[3], size_t count,
                        const float color[4], float pointSize);
void dynamicDrawTriangleFan3(const float (*vertices)[3], size_t count,
                             const float color[4]);

#endif
