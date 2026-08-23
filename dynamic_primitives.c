/*
 * Copyright (c) 2026 Battalion contributors.
 */
/* Renderer-neutral entry points for geometry created from live state. */
/***********************************************************************/

#include "dynamic_primitives.h"

#ifdef MACOSVERSION
#include "renderer/fixed_gl.h"
#else
#include <GL/gl.h>
#endif

void dynamicDrawLine3(const float start[3], const float end[3],
                      const float startColor[3], const float endColor[3])
    {
    glBegin(GL_LINES);
    glColor3fv(startColor);
    glVertex3fv(start);
    glColor3fv(endColor);
    glVertex3fv(end);
    glEnd();
    }

void dynamicDrawQuad3(float vertices[4][3],
                      float colors[4][3])
    {
    int vertex;

    glBegin(GL_QUADS);
    for (vertex = 0; vertex < 4; vertex++)
        {
        glColor3fv(colors[vertex]);
        glVertex3fv(vertices[vertex]);
        }
    glEnd();
    }

void dynamicDrawSolidQuad3(float vertices[4][3],
                           const float color[4])
    {
    int vertex;

    glColor4fv(color);
    glBegin(GL_QUADS);
    for (vertex = 0; vertex < 4; vertex++)
        glVertex3fv(vertices[vertex]);
    glEnd();
    }

void dynamicDrawLineLoop3(float vertices[4][3],
                          const float color[4])
    {
    int vertex;

    glColor4fv(color);
    glBegin(GL_LINE_LOOP);
    for (vertex = 0; vertex < 4; vertex++)
        glVertex3fv(vertices[vertex]);
    glEnd();
    }

void dynamicDrawPoints3(const float (*vertices)[3], size_t count,
                        const float color[4], float pointSize)
    {
    size_t vertex;

    glColor4fv(color);
    glPointSize(pointSize);
    glBegin(GL_POINTS);
    for (vertex = 0; vertex < count; vertex++)
        glVertex3fv(vertices[vertex]);
    glEnd();
    }

void dynamicDrawTriangleFan3(const float (*vertices)[3], size_t count,
                             const float color[4])
    {
    size_t vertex;

    glColor4fv(color);
    glBegin(GL_TRIANGLE_FAN);
    for (vertex = 0; vertex < count; vertex++)
        glVertex3fv(vertices[vertex]);
    glEnd();
    }
