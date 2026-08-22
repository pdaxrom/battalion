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

#ifndef BATTALION_DYNAMIC_PRIMITIVES_H
#define BATTALION_DYNAMIC_PRIMITIVES_H

void dynamicDrawLine3(const float start[3], const float end[3],
                      const float startColor[3], const float endColor[3]);
void dynamicDrawQuad3(const float vertices[4][3],
                      const float colors[4][3]);

#endif
