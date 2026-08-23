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

#ifndef BATTALION_WORLD_COORDINATES_H
#define BATTALION_WORLD_COORDINATES_H

float worldDistanceSquared(float firstX, float firstZ,
                           float secondX, float secondZ);
int worldVerticalIntervalsOverlap(float firstBottom, float firstTop,
                                  float secondBottom, float secondTop);
int worldCylindersOverlap(float firstX, float firstZ, float firstRadius,
                          float firstBottom, float firstTop,
                          float secondX, float secondZ, float secondRadius,
                          float secondBottom, float secondTop);
int worldSegmentIntersectsCylinder(float startX, float startY, float startZ,
                                   float endX, float endY, float endZ,
                                   float centerX, float centerZ, float radius,
                                   float bottom, float top);
void worldClippedRayEndpoint(const float origin[3],
                             const float direction[3], float range,
                             float minimumY, float endpoint[3]);

#endif
