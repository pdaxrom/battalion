/* Copyright (c) 2026 Battalion contributors. */
#include "map_editor_model.h"

#include <stdio.h>
#include <string.h>

static int findArchetype(const struct EntityCatalog *catalog, const char *id)
    {
    size_t index;
    for (index = 0; index < catalog->count; index++)
        if (strcmp(catalog->entries[index].id, id) == 0) return (int)index;
    return -1;
    }

int main(void)
    {
    struct MapEditorModel editor;
    struct BattalionMapPackage roundTrip;
    char error[256];
    size_t objectCount;
    size_t objectIndex;
    size_t routeIndex;
    float screenX;
    float screenY;
    float worldX;
    float worldZ;
    int tank;
    int road;
    int route;
    int safeZoneObject;
    int retreatObject;

    mapEditorModelInitialize(&editor);
    battalionMapPackageInit(&roundTrip);
    if (!mapEditorLoadCatalog(&editor, "./assets", "entities/catalog.txt") ||
        !mapEditorLoad(&editor, "./assets/maps/default/map.json"))
        {
        fprintf(stderr, "editor setup: %s\n", editor.status);
        return 1;
        }
    tank = findArchetype(&editor.catalog, "tank-standard");
    road = findArchetype(&editor.catalog, "road-straight-x");
    route = findArchetype(&editor.catalog, "logic-patrol-route");
    if (tank < 0 || road < 0 || route < 0) return 1;
    objectCount = editor.map.objectCount;
    editor.gridSize = 0.5f;
    editor.snapEnabled = 1;
    if (!mapEditorAddObject(&editor, (size_t)road, 10.24f, 11.26f) ||
        editor.map.objectCount != objectCount + 1 ||
        !mapEditorMoveObject(&editor, objectCount, 12.24f, 13.26f) ||
        editor.map.objects[objectCount].x != 12.0f ||
        editor.map.objects[objectCount].z != 13.5f)
        return 1;
    objectIndex = editor.map.objectCount;
    editor.selectedObject = 0;
    if (!mapEditorSelectArchetypeForPlacement(&editor, (size_t)tank) ||
        editor.selectedObject != -1 || editor.selectedArchetype != tank ||
        !mapEditorAddSelectedObject(&editor, 1.24f, -2.26f) ||
        editor.map.objects[objectIndex].x != 1.0f ||
        editor.map.objects[objectIndex].z != -2.5f ||
        strcmp(editor.map.objects[objectIndex].archetype,
               "tank-standard") != 0 ||
        !mapEditorMoveObject(&editor, objectIndex, 3.74f, 4.26f) ||
        editor.map.objects[objectIndex].x != 3.5f ||
        editor.map.objects[objectIndex].z != 4.5f)
        return 1;
    routeIndex = editor.map.objectCount;
    if (!mapEditorAddObject(&editor, (size_t)route, 4.0f, 5.5f) ||
        !mapEditorAddWaypoint(&editor, routeIndex, 4.24f, 5.26f) ||
        !mapEditorAddWaypoint(&editor, routeIndex, 6.0f, 7.0f) ||
        !mapEditorMoveWaypoint(&editor, routeIndex, 0, 8.24f, 9.26f) ||
        editor.map.objects[routeIndex].waypoints[0].x != 8.0f ||
        editor.map.objects[routeIndex].waypoints[0].z != 9.5f)
        return 1;
    strcpy(editor.map.objects[objectIndex].patrolRoute,
           editor.map.objects[routeIndex].id);
    if (!mapEditorAddGeometryPoint(&editor, objectCount, 12.0f, 13.5f) ||
        !mapEditorMoveGeometryPoint(&editor, objectCount, 0, 14.24f, 15.26f) ||
        editor.map.objects[objectCount].geometry[0].x != 14.0f ||
        editor.map.objects[objectCount].geometry[0].z != 15.5f)
        return 1;
    if (!mapEditorAddTrigger(&editor, objectIndex, "self-test",
                             BATTALION_MAP_TRIGGER_ID,
                             editor.map.objects[objectIndex].id) ||
        !mapEditorSetObjectId(&editor, objectIndex, "renamed-test-object") ||
        strcmp(editor.map.objects[objectIndex].triggers[0].target,
               "renamed-test-object") || !mapEditorValidate(&editor))
        {
        fprintf(stderr, "editor validation: %s\n", editor.status);
        return 1;
        }
    mapEditorWorldToScreen(&editor, 10.0f, 20.0f, 800.0f, 600.0f,
                           12.5f, -7.25f, &screenX, &screenY);
    mapEditorScreenToWorld(&editor, 10.0f, 20.0f, 800.0f, 600.0f,
                           screenX, screenY, &worldX, &worldZ);
    if (worldX < 12.499f || worldX > 12.501f ||
        worldZ < -7.251f || worldZ > -7.249f) return 1;
    if (!mapEditorSave(&editor, "build/tests/map-editor-roundtrip.json") ||
        !battalionMapPackageLoad("build/tests/map-editor-roundtrip.json",
                                 &roundTrip, error, sizeof(error)) ||
        roundTrip.objectCount != objectCount + 3 ||
        roundTrip.objects[routeIndex].waypointCount != 2 ||
        strcmp(roundTrip.objects[objectIndex].patrolRoute,
               roundTrip.objects[routeIndex].id) != 0 ||
        roundTrip.objects[objectIndex].triggerCount != 1)
        return 1;
    battalionMapPackageFree(&roundTrip);
    if (!mapEditorDeleteTrigger(&editor, objectIndex, 0) ||
        mapEditorDeleteObject(&editor, routeIndex) ||
        !mapEditorDeleteGeometryPoint(&editor, objectCount, 0) ||
        !mapEditorDeleteObject(&editor, objectIndex) ||
        !mapEditorDeleteWaypoint(&editor, routeIndex - 1, 0) ||
        !mapEditorDeleteObject(&editor, routeIndex - 1) ||
        !mapEditorDeleteObject(&editor, objectCount) ||
        editor.map.objectCount != objectCount || !mapEditorValidate(&editor))
        return 1;
    if (!mapEditorNew(&editor, "new-map", "New Map") ||
        editor.map.objectCount != 0 ||
        !mapEditorSave(&editor,
                       "build/tests/map-editor-created/nested/map.json"))
        return 1;
    if (!mapEditorLoad(&editor,
                       "./assets/maps/riverside-outpost/map.json") ||
        !mapEditorValidate(&editor))
        {
        fprintf(stderr, "showcase map validation: %s\n", editor.status);
        return 1;
        }
    safeZoneObject = -1;
    retreatObject = -1;
    for (objectIndex = 0; objectIndex < editor.map.objectCount; objectIndex++)
        {
        if (strcmp(editor.map.objects[objectIndex].id,
                   "base-safe-zone") == 0)
            safeZoneObject = (int)objectIndex;
        if (strcmp(editor.map.objects[objectIndex].id,
                   "base-guard-bravo") == 0)
            retreatObject = (int)objectIndex;
        }
    if (safeZoneObject < 0 || retreatObject < 0 ||
        editor.map.objects[retreatObject].alarmResponse !=
            ENTITY_ALARM_RETREAT ||
        !mapEditorSetObjectId(&editor, (size_t)safeZoneObject,
                              "renamed-safe-zone") ||
        strcmp(editor.map.objects[retreatObject].safeZone,
               "renamed-safe-zone") != 0 ||
        mapEditorDeleteObject(&editor, (size_t)safeZoneObject))
        {
        fprintf(stderr, "safe-zone editor reference handling failed\n");
        return 1;
        }
    mapEditorModelDestroy(&editor);
    return 0;
    }
