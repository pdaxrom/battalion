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

#include <stdio.h>

static int require(int condition, const char *message)
    {
    if (!condition)
        {
        fprintf(stderr, "%s\n", message);
        return 0;
        }
    return 1;
    }

int main(void)
    {
    float beamOrigin[3];
    float beamDirection[3];
    float beamEndpoint[3];

    if (!require(worldDistanceSquared(10.0f, -4.0f, 13.0f, 0.0f) ==
                     25.0f,
                 "world distance must not depend on the camera origin"))
        return 1;
    if (!require(worldCylindersOverlap(10.0f, 10.0f, 0.5f, 0.0f, 1.0f,
                                       10.6f, 10.0f, 0.25f, 0.2f, 0.8f),
                 "overlapping solid cylinders were not detected"))
        return 1;
    if (!require(!worldCylindersOverlap(10.0f, 10.0f, 0.5f, 1.1f, 1.5f,
                                        10.6f, 10.0f, 0.25f, 0.2f, 0.8f),
                 "an airborne actor was blocked by an object below it"))
        return 1;
    if (!require(worldCylindersOverlap(10.0f, 10.0f, 0.5f, -1.0f, 0.3f,
                                       10.0f, 10.0f, 0.5f, -1.0f, 0.5f),
                 "a ground creature passed through a building"))
        return 1;
    if (!require(!worldCylindersOverlap(10.0f, 10.0f, 0.5f, 0.0f, 0.4f,
                                        10.0f, 10.0f, 0.5f, -1.0f, -0.7f),
                 "an airborne player was blocked by a ground vehicle"))
        return 1;
    if (!require(worldCylindersOverlap(10.0f, 10.0f, 0.5f, 0.0f, 0.4f,
                                       10.0f, 10.0f, 0.5f, -1.0f, 0.5f),
                 "an airborne player passed through a tall building"))
        return 1;
    if (!require(!worldVerticalIntervalsOverlap(1.0f, 1.5f, 0.0f, 1.0f),
                 "touching vertical bounds must not be a collision"))
        return 1;
    if (!require(worldSegmentIntersectsCylinder(
                     0.0f, 0.0f, 0.0f, 0.0f, -2.0f, 10.0f,
                     0.0f, 4.0f, 0.31f, -1.1f, -0.5f),
                 "a descending beam passed through a ground vehicle"))
        return 1;
    if (!require(worldSegmentIntersectsCylinder(
                     -2.0f, -0.8f, 4.0f, 2.0f, -0.8f, 4.0f,
                     0.0f, 4.0f, 0.31f, -1.1f, -0.5f),
                 "a horizontal beam passed through a ground vehicle"))
        return 1;
    if (!require(!worldSegmentIntersectsCylinder(
                     0.0f, 0.2f, 0.0f, 0.0f, 0.2f, 10.0f,
                     0.0f, 4.0f, 0.31f, -1.1f, -0.5f),
                 "a beam above a ground vehicle caused damage"))
        return 1;
    if (!require(!worldSegmentIntersectsCylinder(
                     0.0f, -0.8f, 0.0f, 0.0f, -0.8f, 2.0f,
                     0.0f, 4.0f, 0.31f, -1.1f, -0.5f),
                 "a beam damaged a vehicle beyond its endpoint"))
        return 1;
    beamOrigin[0] = 0.0f;
    beamOrigin[1] = 0.0f;
    beamOrigin[2] = 0.0f;
    beamDirection[0] = 0.0f;
    beamDirection[1] = -0.5f;
    beamDirection[2] = 1.0f;
    worldClippedRayEndpoint(beamOrigin, beamDirection, 4.0f, -1.0f,
                            beamEndpoint);
    if (!require(beamEndpoint[0] == 0.0f &&
                 beamEndpoint[1] == -1.0f && beamEndpoint[2] == 2.0f,
                 "a downward beam was visually extended past the ground"))
        return 1;
    if (!require(worldSegmentIntersectsCylinder(
                     beamOrigin[0], beamOrigin[1], beamOrigin[2],
                     beamEndpoint[0], beamEndpoint[1], beamEndpoint[2],
                     0.0f, 2.0f, 0.31f, -1.1f, -0.5f),
                 "a vehicle touching the visible beam endpoint was missed"))
        return 1;
    return 0;
    }
