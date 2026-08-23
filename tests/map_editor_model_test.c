/*
 * Copyright (c) 2026 Battalion contributors.
 */
#include "map_editor_model.h"

#include <stdio.h>
#include <string.h>

static int findArchetype(const struct EntityCatalog *catalog, const char *id)
    {
    size_t index;

    for (index = 0; index < catalog->count; index++)
        if (strcmp(catalog->entries[index].id, id) == 0)
            return (int)index;
    return -1;
    }

int main(void)
    {
    struct MapEditorModel editor;
    struct BattalionMapPackage roundTrip;
    char error[256];
    size_t roadCount;
    size_t structureCount;
    size_t actorCount;
    size_t firstActorTriggerCount;
    float screenX;
    float screenY;
    float worldX;
    float worldZ;
    int archetype;

    mapEditorModelInitialize(&editor);
    battalionMapPackageInit(&roundTrip);
    if (!mapEditorLoadCatalog(&editor, "./assets", "entities/catalog.txt"))
        {
        fprintf(stderr, "editor catalog: %s\n", editor.status);
        return 1;
        }
    archetype = findArchetype(&editor.catalog, "tank-standard");
    if (archetype < 0)
        {
        fprintf(stderr, "editor catalog did not contain tank-standard\n");
        return 1;
        }
    if (!mapEditorLoad(&editor, "./assets/maps/default/map.json"))
        {
        fprintf(stderr, "editor load: %s\n", editor.status);
        return 1;
        }
    roadCount = editor.map.roadCount;
    structureCount = editor.map.structureCount;
    actorCount = editor.map.actorCount;
    firstActorTriggerCount = editor.map.actors[0].triggerCount;
    editor.gridSize = 0.5f;
    editor.snapEnabled = 1;
    if (!mapEditorAddRoad(&editor, 10.24f, 11.26f, 2) ||
        editor.map.roadCount != roadCount + 1 ||
        !mapEditorMoveRoad(&editor, roadCount, 12.24f, 13.26f) ||
        editor.map.roads[roadCount].x != 12.0f ||
        editor.map.roads[roadCount].z != 13.5f ||
        !mapEditorAddStructure(&editor, 14.24f, 15.26f, 2, 4) ||
        editor.map.structureCount != structureCount + 1 ||
        !mapEditorMoveStructure(&editor, structureCount, 16.24f, 17.26f) ||
        editor.map.structures[structureCount].x != 16.0f ||
        editor.map.structures[structureCount].z != 17.5f)
        {
        fprintf(stderr, "editor map object mutation failed: %s\n",
                editor.status);
        return 1;
        }
    if (!mapEditorAddActor(&editor, (size_t)archetype, 1.24f, -2.26f) ||
        editor.map.actorCount != actorCount + 1 ||
        editor.map.actors[actorCount].x != 1.0f ||
        editor.map.actors[actorCount].z != -2.5f ||
        !mapEditorMoveActor(&editor, actorCount, 3.74f, 4.26f) ||
        editor.map.actors[actorCount].x != 3.5f ||
        editor.map.actors[actorCount].z != 4.5f)
        {
        fprintf(stderr, "editor actor mutation failed: %s\n", editor.status);
        return 1;
        }
    if (!mapEditorAddWaypoint(&editor, actorCount, 4.24f, 5.26f) ||
        !mapEditorAddWaypoint(&editor, actorCount, 6.0f, 7.0f) ||
        !mapEditorMoveWaypoint(&editor, actorCount, 0, 8.24f, 9.26f) ||
        editor.map.actors[actorCount].waypoints[0].x != 8.0f ||
        editor.map.actors[actorCount].waypoints[0].z != 9.5f)
        {
        fprintf(stderr, "editor waypoint mutation failed: %s\n",
                editor.status);
        return 1;
        }
    if (!mapEditorAddTrigger(&editor, actorCount, "self-test",
                             BATTALION_MAP_TRIGGER_ID,
                             editor.map.actors[actorCount].id) ||
        !mapEditorSetActorId(&editor, actorCount, "renamed-test-actor") ||
        strcmp(editor.map.actors[actorCount].triggers[0].target,
               "renamed-test-actor") != 0 ||
        !mapEditorAddTrigger(&editor, 0, "editor-test",
                             BATTALION_MAP_TRIGGER_ID,
                             editor.map.actors[actorCount].id) ||
        !mapEditorValidate(&editor))
        {
        fprintf(stderr, "editor trigger/validation failed: %s\n",
                editor.status);
        return 1;
        }
    mapEditorWorldToScreen(&editor, 10.0f, 20.0f, 800.0f, 600.0f,
                           12.5f, -7.25f, &screenX, &screenY);
    mapEditorScreenToWorld(&editor, 10.0f, 20.0f, 800.0f, 600.0f,
                           screenX, screenY, &worldX, &worldZ);
    if (worldX < 12.499f || worldX > 12.501f ||
        worldZ < -7.251f || worldZ > -7.249f)
        {
        fprintf(stderr, "editor world/screen transform failed\n");
        return 1;
        }
    if (!mapEditorSave(&editor, "build/tests/map-editor-roundtrip.json") ||
        !battalionMapPackageLoad("build/tests/map-editor-roundtrip.json",
                                 &roundTrip, error, sizeof(error)))
        {
        fprintf(stderr, "editor round trip failed: %s / %s\n",
                editor.status, error);
        return 1;
        }
    if (roundTrip.version != 2 || roundTrip.roadCount != roadCount + 1 ||
        roundTrip.structureCount != structureCount + 1 ||
        roundTrip.actorCount != actorCount + 1 ||
        roundTrip.actors[0].triggerCount != firstActorTriggerCount + 1 ||
        roundTrip.actors[actorCount].waypointCount != 2 ||
        roundTrip.actors[actorCount].triggerCount != 1)
        {
        fprintf(stderr, "editor round trip lost runtime map data\n");
        return 1;
        }
    battalionMapPackageFree(&roundTrip);
    if (!mapEditorDeleteRoad(&editor, roadCount) ||
        !mapEditorDeleteStructure(&editor, structureCount) ||
        editor.map.roadCount != roadCount ||
        editor.map.structureCount != structureCount ||
        !mapEditorDeleteTrigger(&editor, actorCount, 0) ||
        !mapEditorDeleteWaypoint(&editor, actorCount, 0) ||
        editor.map.actors[actorCount].waypointCount != 1 ||
        !mapEditorDeleteActor(&editor, actorCount) ||
        editor.map.actorCount != actorCount ||
        editor.map.actors[0].triggerCount != firstActorTriggerCount ||
        !mapEditorValidate(&editor))
        {
        fprintf(stderr, "editor deletion failed: %s\n", editor.status);
        return 1;
        }
    strcpy(editor.map.actors[0].id, "INVALID ID");
    if (mapEditorValidate(&editor))
        {
        fprintf(stderr, "editor accepted an invalid actor id\n");
        return 1;
        }
    strcpy(editor.map.actors[0].id, "guard-a");
    if (!mapEditorNew(&editor, "new-map", "New Map") ||
        editor.map.version != 2 || editor.map.actorCount != 0 ||
        !mapEditorSave(&editor,
                       "build/tests/map-editor-created/nested/map.json") ||
        !battalionMapPackageLoad(
                                 "build/tests/map-editor-created/nested/map.json",
                                 &roundTrip, error, sizeof(error)) ||
        roundTrip.version != 2)
        {
        fprintf(stderr, "editor new v2 map failed: %s\n", editor.status);
        return 1;
        }
    battalionMapPackageFree(&roundTrip);
    mapEditorModelDestroy(&editor);
    return 0;
    }
