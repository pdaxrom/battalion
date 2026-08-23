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

#include "world_coordinates.h"

#include <math.h>

float worldDistanceSquared(float firstX, float firstZ,
                           float secondX, float secondZ)
    {
    float deltaX;
    float deltaZ;

    deltaX = firstX - secondX;
    deltaZ = firstZ - secondZ;
    return deltaX * deltaX + deltaZ * deltaZ;
    }

int worldVerticalIntervalsOverlap(float firstBottom, float firstTop,
                                  float secondBottom, float secondTop)
    {
    return firstBottom < secondTop && firstTop > secondBottom;
    }

int worldCylindersOverlap(float firstX, float firstZ, float firstRadius,
                          float firstBottom, float firstTop,
                          float secondX, float secondZ, float secondRadius,
                          float secondBottom, float secondTop)
    {
    float combinedRadius;

    combinedRadius = firstRadius + secondRadius;
    return worldDistanceSquared(firstX, firstZ, secondX, secondZ) <
               combinedRadius * combinedRadius &&
           worldVerticalIntervalsOverlap(firstBottom, firstTop,
                                         secondBottom, secondTop);
    }

int worldSegmentIntersectsCylinder(float startX, float startY, float startZ,
                                   float endX, float endY, float endZ,
                                   float centerX, float centerZ, float radius,
                                   float bottom, float top)
    {
    float directionX;
    float directionY;
    float directionZ;
    float offsetX;
    float offsetZ;
    float horizontalA;
    float horizontalB;
    float horizontalC;
    float discriminant;
    float horizontalStart;
    float horizontalEnd;
    float verticalStart;
    float verticalEnd;
    float temporary;

    if (radius < 0.0f)
        return 0;
    if (bottom > top)
        {
        temporary = bottom;
        bottom = top;
        top = temporary;
        }
    directionX = endX - startX;
    directionY = endY - startY;
    directionZ = endZ - startZ;
    offsetX = startX - centerX;
    offsetZ = startZ - centerZ;
    horizontalA = directionX * directionX + directionZ * directionZ;
    horizontalB = 2.0f * (offsetX * directionX + offsetZ * directionZ);
    horizontalC = offsetX * offsetX + offsetZ * offsetZ - radius * radius;

    if (horizontalA <= 0.0000001f)
        {
        if (horizontalC > 0.0f)
            return 0;
        horizontalStart = 0.0f;
        horizontalEnd = 1.0f;
        }
    else
        {
        discriminant = horizontalB * horizontalB -
                       4.0f * horizontalA * horizontalC;
        if (discriminant < 0.0f)
            return 0;
        discriminant = (float)sqrt((double)discriminant);
        horizontalStart = (-horizontalB - discriminant) /
                          (2.0f * horizontalA);
        horizontalEnd = (-horizontalB + discriminant) /
                        (2.0f * horizontalA);
        }

    if (directionY > -0.0000001f && directionY < 0.0000001f)
        {
        if (startY < bottom || startY > top)
            return 0;
        verticalStart = 0.0f;
        verticalEnd = 1.0f;
        }
    else
        {
        verticalStart = (bottom - startY) / directionY;
        verticalEnd = (top - startY) / directionY;
        if (verticalStart > verticalEnd)
            {
            temporary = verticalStart;
            verticalStart = verticalEnd;
            verticalEnd = temporary;
            }
        }

    if (horizontalStart < verticalStart)
        horizontalStart = verticalStart;
    if (horizontalEnd > verticalEnd)
        horizontalEnd = verticalEnd;
    if (horizontalStart < 0.0f)
        horizontalStart = 0.0f;
    if (horizontalEnd > 1.0f)
        horizontalEnd = 1.0f;
    return horizontalStart <= horizontalEnd;
    }

void worldClippedRayEndpoint(const float origin[3],
                             const float direction[3], float range,
                             float minimumY, float endpoint[3])
    {
    float distance;
    float planeDistance;

    distance = range;
    if (distance < 0.0f)
        distance = 0.0f;
    if (direction[1] < -0.0000001f &&
        origin[1] + distance * direction[1] < minimumY)
        {
        planeDistance = (minimumY - origin[1]) / direction[1];
        if (planeDistance >= 0.0f && planeDistance < distance)
            distance = planeDistance;
        }
    endpoint[0] = origin[0] + distance * direction[0];
    endpoint[1] = origin[1] + distance * direction[1];
    endpoint[2] = origin[2] + distance * direction[2];
    }
