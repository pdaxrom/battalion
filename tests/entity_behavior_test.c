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

#include "entity_behavior.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int nearValue(float left, float right)
    {
    return fabs((double)(left - right)) < 0.0001;
    }

static void contact(struct EntityContact *value, const char *id,
                    float x, float z, int visible, int audible)
    {
    memset(value, 0, sizeof(*value));
    strcpy(value->id, id);
    strcpy(value->faction, "monster");
    value->x = x;
    value->z = z;
    value->active = 1;
    value->visible = visible;
    value->audible = audible;
    }

int main(void)
    {
    struct EntityCatalog catalog;
    const struct EntityArchetype *tank;
    struct EntityWaypoint path[2];
    struct EntityAgent guard;
    struct EntityAgent receiver;
    struct EntityContact contacts[2];
    char error[256];
    int index;
    float aimX;
    float aimY;
    float aimZ;

    if (entitySteeredHeadingDelta(1, 0, 8.0f) != 8 ||
        entitySteeredHeadingDelta(0, 1, 8.0f) != -8 ||
        entitySteeredHeadingDelta(0, 0, 8.0f) != 0 ||
        entitySteeredHeadingDelta(1, 1, 8.0f) != 0)
        {
        fprintf(stderr, "steered player control direction failed\n");
        return 1;
        }
    entityAimDirection(0, 0, 0.08f, &aimX, &aimY, &aimZ);
    if (!nearValue(aimX, 0.0f) || !nearValue(aimY, 0.0f) ||
        !nearValue(aimZ, 0.08f))
        {
        fprintf(stderr, "forward aim direction failed\n");
        return 1;
        }
    entityAimDirection(900, -30, 1.0f, &aimX, &aimY, &aimZ);
    if (aimX < 0.99f || aimY <= 0.0f || !nearValue(aimZ, 0.0f))
        {
        fprintf(stderr, "turned and elevated aim direction failed\n");
        return 1;
        }

    entityCatalogInitialize(&catalog);
    if (!entityCatalogLoad("assets", "entities/catalog.txt", &catalog,
                           error, sizeof(error)))
        {
        fprintf(stderr, "%s\n", error);
        return 1;
        }
    tank = entityCatalogFind(&catalog, "tank-standard");
    path[0].x = 0.0f;
    path[0].z = 0.0f;
    path[1].x = 2.0f;
    path[1].z = 0.0f;
    entityAgentInitialize(&guard, tank, 0.0f, 0.0f, 0.0f,
                          path, 2, ENTITY_PATROL_LOOP);
    for (index = 0; index < 5; index++)
        entityAgentStep(&guard, NULL, 0);
    if (guard.x <= 0.0f || guard.reaction != ENTITY_REACTION_OBSERVE)
        {
        fprintf(stderr, "patrol failed: x=%g reaction=%d\n",
                guard.x, guard.reaction);
        return 1;
        }

    contact(&contacts[0], "intruder-a", guard.x, guard.z + 1.0f, 1, 0);
    contact(&contacts[1], "intruder-b", guard.x + 1.0f, guard.z, 1, 1);
    entityAgentStep(&guard, contacts, 2);
    if (guard.reaction != ENTITY_REACTION_ATTACK ||
        guard.contactCount < 2 || guard.eventCount != 2 ||
        strcmp(guard.events[0].name, "base-alarm") != 0)
        {
        fprintf(stderr, "sensor failed: reaction=%d contacts=%d events=%lu\n",
                guard.reaction, guard.contactCount,
                (unsigned long)guard.eventCount);
        return 1;
        }

    entityAgentInitialize(&receiver, tank, 20.0f, 20.0f, 0.0f,
                          NULL, 0, ENTITY_PATROL_LOOP);
    if (!entityAgentQueueTrigger(&receiver, guard.events[0].name,
                                 &guard.events[0].contact))
        {
        fprintf(stderr, "trigger queue failed\n");
        return 1;
        }
    entityAgentStep(&receiver, NULL, 0);
    if (receiver.reaction != ENTITY_REACTION_ATTACK ||
        !receiver.hasTarget ||
        strcmp(receiver.target.id, "intruder-a") != 0)
        {
        fprintf(stderr, "trigger failed: reaction=%d target=%s\n",
                receiver.reaction, receiver.target.id);
        return 1;
        }

    entityCatalogDestroy(&catalog);
    return 0;
    }
