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
    return 0;
    }
