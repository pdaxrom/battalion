/****************************************************************/
/* code copyright 1995-1996 Andrew Johnson - ALL RIGHTS RESERVED*/
/*                          ajohnson@eecs.uic.edu               */
/*                                                              */
/*                      Electronic Visualization Lab (M/C 154)  */
/*                      University of Illinois at Chicago       */
/*                      851 South Morgan St. Room 1120 SEO      */
/*                      Chicago, IL 60607-7053                  */
/*                                                              */
/*                      (312) 996-3002     (312) 413-7585 FAX   */
/***********************************************************************/
/* Renderer-neutral entry points for geometry created from live state. */
/***********************************************************************/

#include "dynamic_primitives.h"

#ifdef MACOSVERSION
#include "renderer/legacy_gl.h"
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

void dynamicDrawQuad3(const float vertices[4][3],
                      const float colors[4][3])
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
