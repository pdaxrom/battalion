/*
 * Copyright (c) 2026 Battalion contributors.
 */
/****************************************************************/
/* Renderer-neutral editing operations for Battalion map packages. */
/****************************************************************/

#include "map_editor_model.h"

#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

static void setStatus(struct MapEditorModel *editor, const char *message)
    {
    strncpy(editor->status, message, sizeof(editor->status) - 1);
    editor->status[sizeof(editor->status) - 1] = '\0';
    }

static int copyText(char *target, size_t targetSize, const char *source)
    {
    size_t length;

    if (target == NULL || source == NULL || targetSize == 0)
        return 0;
    length = strlen(source);
    if (length >= targetSize)
        return 0;
    memcpy(target, source, length + 1);
    return 1;
    }

static int ensureParentDirectories(const char *path)
    {
    char directory[MAP_EDITOR_PATH_SIZE];
    size_t index;
    size_t length;

    length = strlen(path);
    if (length >= sizeof(directory))
        return 0;
    strcpy(directory, path);
    for (index = 1; index < length; index++)
        if (directory[index] == '/')
            {
            directory[index] = '\0';
            if (directory[0] != '\0' && mkdir(directory, 0777) != 0 &&
                errno != EEXIST)
                return 0;
            directory[index] = '/';
            }
    return 1;
    }

static void resetSelection(struct MapEditorModel *editor)
    {
    editor->selectedActor = -1;
    editor->selectedWaypoint = -1;
    editor->selectedTrigger = -1;
    editor->selectedLegacyKind = MAP_EDITOR_LEGACY_NONE;
    editor->selectedLegacyIndex = -1;
    }

static int actorIdExists(const struct BattalionMapPackage *map,
                         const char *id)
    {
    size_t index;

    for (index = 0; index < map->actorCount; index++)
        if (strcmp(map->actors[index].id, id) == 0)
            return 1;
    return 0;
    }

static int makeActorId(const struct BattalionMapPackage *map,
                       const char *archetype, char *id, size_t idSize)
    {
    char suffix[24];
    size_t baseLength;
    unsigned long ordinal;

    for (ordinal = 1; ordinal < 1000000UL; ordinal++)
        {
        sprintf(suffix, "-%lu", ordinal);
        baseLength = strlen(archetype);
        if (baseLength + strlen(suffix) >= idSize)
            baseLength = idSize - strlen(suffix) - 1;
        memcpy(id, archetype, baseLength);
        strcpy(id + baseLength, suffix);
        if (!actorIdExists(map, id))
            return 1;
        }
    return 0;
    }

void mapEditorModelInitialize(struct MapEditorModel *editor)
    {
    memset(editor, 0, sizeof(*editor));
    battalionMapPackageInit(&editor->map);
    entityCatalogInitialize(&editor->catalog);
    resetSelection(editor);
    editor->selectedArchetype = -1;
    editor->gridVisible = 1;
    editor->snapEnabled = 1;
    editor->gridSize = 0.5f;
    editor->zoom = 12.0f;
    setStatus(editor, "Ready");
    }

void mapEditorModelDestroy(struct MapEditorModel *editor)
    {
    battalionMapPackageFree(&editor->map);
    entityCatalogDestroy(&editor->catalog);
    memset(editor, 0, sizeof(*editor));
    }

int mapEditorLoadCatalog(struct MapEditorModel *editor,
                         const char *assetRoot, const char *catalogPath)
    {
    char error[MAP_EDITOR_STATUS_SIZE];
    struct EntityCatalog loaded;

    entityCatalogInitialize(&loaded);
    if (!entityCatalogLoad(assetRoot, catalogPath, &loaded,
                           error, sizeof(error)))
        {
        setStatus(editor, error);
        return 0;
        }
    if (!copyText(editor->assetRoot, sizeof(editor->assetRoot), assetRoot))
        {
        entityCatalogDestroy(&loaded);
        setStatus(editor, "Asset root path is too long");
        return 0;
        }
    entityCatalogDestroy(&editor->catalog);
    editor->catalog = loaded;
    editor->selectedArchetype = loaded.count > 0 ? 0 : -1;
    setStatus(editor, "Archetype catalog loaded");
    return 1;
    }

int mapEditorNew(struct MapEditorModel *editor,
                 const char *id, const char *name)
    {
    struct BattalionMapPackage fresh;

    battalionMapPackageInit(&fresh);
    fresh.version = 2;
    if (!copyText(fresh.id, sizeof(fresh.id), id) ||
        !copyText(fresh.name, sizeof(fresh.name), name))
        {
        setStatus(editor, "Map id or name is too long");
        return 0;
        }
    battalionMapPackageFree(&editor->map);
    editor->map = fresh;
    editor->path[0] = '\0';
    resetSelection(editor);
    editor->cameraX = 0.0f;
    editor->cameraZ = 0.0f;
    editor->zoom = 12.0f;
    editor->dirty = 1;
    setStatus(editor, "New v2 map");
    return 1;
    }

int mapEditorLoad(struct MapEditorModel *editor, const char *path)
    {
    struct BattalionMapPackage loaded;
    char error[MAP_EDITOR_STATUS_SIZE];

    battalionMapPackageInit(&loaded);
    if (!battalionMapPackageLoad(path, &loaded, error, sizeof(error)))
        {
        setStatus(editor, error);
        return 0;
        }
    if (!copyText(editor->path, sizeof(editor->path), path))
        {
        battalionMapPackageFree(&loaded);
        setStatus(editor, "Map path is too long");
        return 0;
        }
    battalionMapPackageFree(&editor->map);
    editor->map = loaded;
    resetSelection(editor);
    editor->dirty = 0;
    setStatus(editor, "Map loaded");
    return 1;
    }

int mapEditorValidate(struct MapEditorModel *editor)
    {
    char error[MAP_EDITOR_STATUS_SIZE];
    size_t index;

    if (!battalionMapPackageValidate(&editor->map, error, sizeof(error)))
        {
        setStatus(editor, error);
        return 0;
        }
    for (index = 0; index < editor->map.actorCount; index++)
        if (entityCatalogFind(&editor->catalog,
                              editor->map.actors[index].archetype) == NULL)
            {
            setStatus(editor, "Map actor has an unknown archetype");
            return 0;
            }
    setStatus(editor, "Map is valid");
    return 1;
    }

int mapEditorSave(struct MapEditorModel *editor, const char *path)
    {
    char error[MAP_EDITOR_STATUS_SIZE];

    if (path == NULL || path[0] == '\0')
        path = editor->path;
    if (path[0] == '\0')
        {
        setStatus(editor, "Choose a map.json path before saving");
        return 0;
        }
    if (!mapEditorValidate(editor))
        return 0;
    if (!ensureParentDirectories(path))
        {
        setStatus(editor, "Could not create the map directory");
        return 0;
        }
    if (!battalionMapPackageWrite(path, &editor->map,
                                  error, sizeof(error)))
        {
        setStatus(editor, error);
        return 0;
        }
    if (!copyText(editor->path, sizeof(editor->path), path))
        {
        setStatus(editor, "Map saved, but path is too long to remember");
        return 0;
        }
    editor->dirty = 0;
    setStatus(editor, "Map saved");
    return 1;
    }

float mapEditorSnapValue(const struct MapEditorModel *editor, float value)
    {
    float scaled;

    if (!editor->snapEnabled || editor->gridSize <= 0.0f)
        return value;
    scaled = value / editor->gridSize;
    if (scaled >= 0.0f)
        scaled = (float)floor((double)(scaled + 0.5f));
    else
        scaled = (float)ceil((double)(scaled - 0.5f));
    return scaled * editor->gridSize;
    }

int mapEditorAddActor(struct MapEditorModel *editor,
                      size_t archetypeIndex, float x, float z)
    {
    struct BattalionMapActor *actors;
    struct BattalionMapActor *actor;
    const struct EntityArchetype *archetype;

    if (archetypeIndex >= editor->catalog.count)
        {
        setStatus(editor, "Choose an archetype first");
        return 0;
        }
    archetype = &editor->catalog.entries[archetypeIndex];
    actors = (struct BattalionMapActor *)realloc(
        editor->map.actors,
        (editor->map.actorCount + 1) * sizeof(struct BattalionMapActor));
    if (actors == NULL)
        {
        setStatus(editor, "Out of memory adding actor");
        return 0;
        }
    editor->map.actors = actors;
    actor = &editor->map.actors[editor->map.actorCount];
    memset(actor, 0, sizeof(*actor));
    if (!makeActorId(&editor->map, archetype->id,
                     actor->id, sizeof(actor->id)))
        {
        setStatus(editor, "Could not generate a unique actor id");
        return 0;
        }
    strcpy(actor->archetype, archetype->id);
    actor->x = mapEditorSnapValue(editor, x);
    actor->z = mapEditorSnapValue(editor, z);
    actor->active = 1;
    actor->patrolMode = ENTITY_PATROL_LOOP;
    editor->map.actorCount++;
    editor->map.version = 2;
    editor->selectedActor = (int)(editor->map.actorCount - 1);
    editor->selectedWaypoint = -1;
    editor->selectedTrigger = -1;
    editor->dirty = 1;
    setStatus(editor, "Actor added");
    return 1;
    }

int mapEditorDeleteActor(struct MapEditorModel *editor, size_t actorIndex)
    {
    char deletedId[ENTITY_NAME_SIZE];
    char deletedGroup[ENTITY_NAME_SIZE];
    struct BattalionMapActor *actor;
    struct BattalionMapActor *other;
    size_t otherIndex;
    size_t routeIndex;
    size_t groupIndex;
    int groupSurvives;

    if (actorIndex >= editor->map.actorCount)
        return 0;
    actor = &editor->map.actors[actorIndex];
    strcpy(deletedId, actor->id);
    strcpy(deletedGroup, actor->group);
    groupSurvives = 0;
    if (deletedGroup[0] != '\0')
        for (groupIndex = 0; groupIndex < editor->map.actorCount;
             groupIndex++)
            if (groupIndex != actorIndex &&
                strcmp(editor->map.actors[groupIndex].group,
                       deletedGroup) == 0)
                groupSurvives = 1;
    for (otherIndex = 0; otherIndex < editor->map.actorCount; otherIndex++)
        {
        if (otherIndex == actorIndex)
            continue;
        other = &editor->map.actors[otherIndex];
        routeIndex = 0;
        while (routeIndex < other->triggerCount)
            {
            struct BattalionMapTrigger *route;
            int removeRoute;

            route = &other->triggers[routeIndex];
            removeRoute =
                (route->targetType == BATTALION_MAP_TRIGGER_ID &&
                 strcmp(route->target, deletedId) == 0) ||
                (!groupSurvives && deletedGroup[0] != '\0' &&
                 route->targetType == BATTALION_MAP_TRIGGER_GROUP &&
                 strcmp(route->target, deletedGroup) == 0);
            if (removeRoute)
                {
                if (routeIndex + 1 < other->triggerCount)
                    memmove(&other->triggers[routeIndex],
                            &other->triggers[routeIndex + 1],
                            (other->triggerCount - routeIndex - 1) *
                                sizeof(struct BattalionMapTrigger));
                other->triggerCount--;
                }
            else
                routeIndex++;
            }
        if (other->triggerCount == 0)
            {
            free(other->triggers);
            other->triggers = NULL;
            }
        }
    free(actor->waypoints);
    free(actor->triggers);
    if (actorIndex + 1 < editor->map.actorCount)
        memmove(&editor->map.actors[actorIndex],
                &editor->map.actors[actorIndex + 1],
                (editor->map.actorCount - actorIndex - 1) *
                    sizeof(struct BattalionMapActor));
    editor->map.actorCount--;
    if (editor->map.actorCount == 0)
        {
        free(editor->map.actors);
        editor->map.actors = NULL;
        }
    resetSelection(editor);
    editor->dirty = 1;
    setStatus(editor, "Actor deleted");
    return 1;
    }

int mapEditorSetActorId(struct MapEditorModel *editor, size_t actorIndex,
                        const char *id)
    {
    char oldId[ENTITY_NAME_SIZE];
    size_t otherIndex;
    size_t routeIndex;

    if (actorIndex >= editor->map.actorCount || id == NULL ||
        strlen(id) >= sizeof(editor->map.actors[actorIndex].id))
        return 0;
    strcpy(oldId, editor->map.actors[actorIndex].id);
    if (strcmp(oldId, id) == 0)
        return 1;
    strcpy(editor->map.actors[actorIndex].id, id);
    for (otherIndex = 0; otherIndex < editor->map.actorCount; otherIndex++)
        for (routeIndex = 0;
             routeIndex < editor->map.actors[otherIndex].triggerCount;
             routeIndex++)
            if (editor->map.actors[otherIndex].triggers[routeIndex].targetType ==
                    BATTALION_MAP_TRIGGER_ID &&
                strcmp(editor->map.actors[otherIndex]
                           .triggers[routeIndex].target, oldId) == 0)
                strcpy(editor->map.actors[otherIndex]
                           .triggers[routeIndex].target, id);
    editor->dirty = 1;
    setStatus(editor, "Actor id and trigger routes updated");
    return 1;
    }

int mapEditorMoveActor(struct MapEditorModel *editor, size_t actorIndex,
                       float x, float z)
    {
    if (actorIndex >= editor->map.actorCount)
        return 0;
    editor->map.actors[actorIndex].x = mapEditorSnapValue(editor, x);
    editor->map.actors[actorIndex].z = mapEditorSnapValue(editor, z);
    editor->dirty = 1;
    return 1;
    }

int mapEditorAddRoad(struct MapEditorModel *editor,
                     float x, float z, int type)
    {
    struct BattalionMapRoad *roads;
    struct BattalionMapRoad *road;

    roads = (struct BattalionMapRoad *)realloc(
        editor->map.roads,
        (editor->map.roadCount + 1) * sizeof(struct BattalionMapRoad));
    if (roads == NULL)
        {
        setStatus(editor, "Out of memory adding road");
        return 0;
        }
    editor->map.roads = roads;
    road = &editor->map.roads[editor->map.roadCount];
    road->x = mapEditorSnapValue(editor, x);
    road->z = mapEditorSnapValue(editor, z);
    road->type = type;
    editor->selectedLegacyKind = MAP_EDITOR_LEGACY_ROAD;
    editor->selectedLegacyIndex = (int)editor->map.roadCount;
    editor->map.roadCount++;
    editor->selectedActor = -1;
    editor->selectedWaypoint = -1;
    editor->selectedTrigger = -1;
    editor->dirty = 1;
    setStatus(editor, "Road added");
    return 1;
    }

int mapEditorMoveRoad(struct MapEditorModel *editor, size_t roadIndex,
                      float x, float z)
    {
    if (roadIndex >= editor->map.roadCount)
        return 0;
    editor->map.roads[roadIndex].x = mapEditorSnapValue(editor, x);
    editor->map.roads[roadIndex].z = mapEditorSnapValue(editor, z);
    editor->dirty = 1;
    return 1;
    }

int mapEditorDeleteRoad(struct MapEditorModel *editor, size_t roadIndex)
    {
    if (roadIndex >= editor->map.roadCount)
        return 0;
    if (roadIndex + 1 < editor->map.roadCount)
        memmove(&editor->map.roads[roadIndex],
                &editor->map.roads[roadIndex + 1],
                (editor->map.roadCount - roadIndex - 1) *
                    sizeof(struct BattalionMapRoad));
    editor->map.roadCount--;
    if (editor->map.roadCount == 0)
        {
        free(editor->map.roads);
        editor->map.roads = NULL;
        }
    resetSelection(editor);
    editor->dirty = 1;
    setStatus(editor, "Road deleted");
    return 1;
    }

int mapEditorAddStructure(struct MapEditorModel *editor,
                          float x, float z, int type, int shape)
    {
    struct BattalionMapStructure *structures;
    struct BattalionMapStructure *structure;

    structures = (struct BattalionMapStructure *)realloc(
        editor->map.structures,
        (editor->map.structureCount + 1) *
            sizeof(struct BattalionMapStructure));
    if (structures == NULL)
        {
        setStatus(editor, "Out of memory adding structure");
        return 0;
        }
    editor->map.structures = structures;
    structure = &editor->map.structures[editor->map.structureCount];
    structure->x = mapEditorSnapValue(editor, x);
    structure->z = mapEditorSnapValue(editor, z);
    structure->type = type;
    structure->shape = shape;
    editor->selectedLegacyKind = MAP_EDITOR_LEGACY_STRUCTURE;
    editor->selectedLegacyIndex = (int)editor->map.structureCount;
    editor->map.structureCount++;
    editor->selectedActor = -1;
    editor->selectedWaypoint = -1;
    editor->selectedTrigger = -1;
    editor->dirty = 1;
    setStatus(editor, "Structure added");
    return 1;
    }

int mapEditorMoveStructure(struct MapEditorModel *editor,
                           size_t structureIndex, float x, float z)
    {
    if (structureIndex >= editor->map.structureCount)
        return 0;
    editor->map.structures[structureIndex].x =
        mapEditorSnapValue(editor, x);
    editor->map.structures[structureIndex].z =
        mapEditorSnapValue(editor, z);
    editor->dirty = 1;
    return 1;
    }

int mapEditorDeleteStructure(struct MapEditorModel *editor,
                             size_t structureIndex)
    {
    if (structureIndex >= editor->map.structureCount)
        return 0;
    if (structureIndex + 1 < editor->map.structureCount)
        memmove(&editor->map.structures[structureIndex],
                &editor->map.structures[structureIndex + 1],
                (editor->map.structureCount - structureIndex - 1) *
                    sizeof(struct BattalionMapStructure));
    editor->map.structureCount--;
    if (editor->map.structureCount == 0)
        {
        free(editor->map.structures);
        editor->map.structures = NULL;
        }
    resetSelection(editor);
    editor->dirty = 1;
    setStatus(editor, "Structure deleted");
    return 1;
    }

int mapEditorAddWaypoint(struct MapEditorModel *editor, size_t actorIndex,
                         float x, float z)
    {
    struct BattalionMapActor *actor;
    struct EntityWaypoint *waypoints;

    if (actorIndex >= editor->map.actorCount)
        return 0;
    actor = &editor->map.actors[actorIndex];
    waypoints = (struct EntityWaypoint *)realloc(
        actor->waypoints,
        (actor->waypointCount + 1) * sizeof(struct EntityWaypoint));
    if (waypoints == NULL)
        {
        setStatus(editor, "Out of memory adding waypoint");
        return 0;
        }
    actor->waypoints = waypoints;
    actor->waypoints[actor->waypointCount].x = mapEditorSnapValue(editor, x);
    actor->waypoints[actor->waypointCount].z = mapEditorSnapValue(editor, z);
    actor->waypointCount++;
    editor->selectedWaypoint = (int)(actor->waypointCount - 1);
    editor->dirty = 1;
    setStatus(editor, "Waypoint added");
    return 1;
    }

int mapEditorMoveWaypoint(struct MapEditorModel *editor, size_t actorIndex,
                          size_t waypointIndex, float x, float z)
    {
    struct BattalionMapActor *actor;

    if (actorIndex >= editor->map.actorCount)
        return 0;
    actor = &editor->map.actors[actorIndex];
    if (waypointIndex >= actor->waypointCount)
        return 0;
    actor->waypoints[waypointIndex].x = mapEditorSnapValue(editor, x);
    actor->waypoints[waypointIndex].z = mapEditorSnapValue(editor, z);
    editor->dirty = 1;
    return 1;
    }

int mapEditorDeleteWaypoint(struct MapEditorModel *editor, size_t actorIndex,
                            size_t waypointIndex)
    {
    struct BattalionMapActor *actor;

    if (actorIndex >= editor->map.actorCount)
        return 0;
    actor = &editor->map.actors[actorIndex];
    if (waypointIndex >= actor->waypointCount)
        return 0;
    if (waypointIndex + 1 < actor->waypointCount)
        memmove(&actor->waypoints[waypointIndex],
                &actor->waypoints[waypointIndex + 1],
                (actor->waypointCount - waypointIndex - 1) *
                    sizeof(struct EntityWaypoint));
    actor->waypointCount--;
    if (actor->waypointCount == 0)
        {
        free(actor->waypoints);
        actor->waypoints = NULL;
        }
    editor->selectedWaypoint = -1;
    editor->dirty = 1;
    setStatus(editor, "Waypoint deleted");
    return 1;
    }

int mapEditorAddTrigger(struct MapEditorModel *editor, size_t actorIndex,
                        const char *event, int targetType,
                        const char *target)
    {
    struct BattalionMapActor *actor;
    struct BattalionMapTrigger *triggers;
    struct BattalionMapTrigger *trigger;

    if (actorIndex >= editor->map.actorCount ||
        (targetType != BATTALION_MAP_TRIGGER_ID &&
         targetType != BATTALION_MAP_TRIGGER_GROUP))
        return 0;
    actor = &editor->map.actors[actorIndex];
    triggers = (struct BattalionMapTrigger *)realloc(
        actor->triggers,
        (actor->triggerCount + 1) * sizeof(struct BattalionMapTrigger));
    if (triggers == NULL)
        {
        setStatus(editor, "Out of memory adding trigger route");
        return 0;
        }
    actor->triggers = triggers;
    trigger = &actor->triggers[actor->triggerCount];
    memset(trigger, 0, sizeof(*trigger));
    if (!copyText(trigger->event, sizeof(trigger->event), event) ||
        !copyText(trigger->target, sizeof(trigger->target), target))
        {
        setStatus(editor, "Trigger event or target is too long");
        return 0;
        }
    trigger->targetType = targetType;
    actor->triggerCount++;
    editor->selectedTrigger = (int)(actor->triggerCount - 1);
    editor->dirty = 1;
    setStatus(editor, "Trigger route added");
    return 1;
    }

int mapEditorDeleteTrigger(struct MapEditorModel *editor, size_t actorIndex,
                           size_t triggerIndex)
    {
    struct BattalionMapActor *actor;

    if (actorIndex >= editor->map.actorCount)
        return 0;
    actor = &editor->map.actors[actorIndex];
    if (triggerIndex >= actor->triggerCount)
        return 0;
    if (triggerIndex + 1 < actor->triggerCount)
        memmove(&actor->triggers[triggerIndex],
                &actor->triggers[triggerIndex + 1],
                (actor->triggerCount - triggerIndex - 1) *
                    sizeof(struct BattalionMapTrigger));
    actor->triggerCount--;
    if (actor->triggerCount == 0)
        {
        free(actor->triggers);
        actor->triggers = NULL;
        }
    editor->selectedTrigger = -1;
    editor->dirty = 1;
    setStatus(editor, "Trigger route deleted");
    return 1;
    }

void mapEditorWorldToScreen(const struct MapEditorModel *editor,
                            float viewX, float viewY,
                            float viewWidth, float viewHeight,
                            float worldX, float worldZ,
                            float *screenX, float *screenY)
    {
    *screenX = viewX + viewWidth * 0.5f +
               (worldX - editor->cameraX) * editor->zoom;
    *screenY = viewY + viewHeight * 0.5f -
               (worldZ - editor->cameraZ) * editor->zoom;
    }

void mapEditorScreenToWorld(const struct MapEditorModel *editor,
                            float viewX, float viewY,
                            float viewWidth, float viewHeight,
                            float screenX, float screenY,
                            float *worldX, float *worldZ)
    {
    *worldX = editor->cameraX +
              (screenX - viewX - viewWidth * 0.5f) / editor->zoom;
    *worldZ = editor->cameraZ -
              (screenY - viewY - viewHeight * 0.5f) / editor->zoom;
    }

static void includePoint(float x, float z, int *hasPoint,
                         float *minimumX, float *minimumZ,
                         float *maximumX, float *maximumZ)
    {
    if (!*hasPoint)
        {
        *minimumX = *maximumX = x;
        *minimumZ = *maximumZ = z;
        *hasPoint = 1;
        }
    else
        {
        if (x < *minimumX) *minimumX = x;
        if (x > *maximumX) *maximumX = x;
        if (z < *minimumZ) *minimumZ = z;
        if (z > *maximumZ) *maximumZ = z;
        }
    }

void mapEditorFitContents(struct MapEditorModel *editor,
                          float viewWidth, float viewHeight)
    {
    float minimumX;
    float minimumZ;
    float maximumX;
    float maximumZ;
    float extentX;
    float extentZ;
    float zoomX;
    float zoomZ;
    int hasPoint;
    size_t index;
    size_t waypointIndex;

    minimumX = minimumZ = maximumX = maximumZ = 0.0f;
    hasPoint = 0;
    for (index = 0; index < editor->map.roadCount; index++)
        includePoint(editor->map.roads[index].x, editor->map.roads[index].z,
                     &hasPoint, &minimumX, &minimumZ, &maximumX, &maximumZ);
    for (index = 0; index < editor->map.structureCount; index++)
        includePoint(editor->map.structures[index].x,
                     editor->map.structures[index].z,
                     &hasPoint, &minimumX, &minimumZ, &maximumX, &maximumZ);
    for (index = 0; index < editor->map.actorCount; index++)
        {
        includePoint(editor->map.actors[index].x, editor->map.actors[index].z,
                     &hasPoint, &minimumX, &minimumZ, &maximumX, &maximumZ);
        for (waypointIndex = 0;
             waypointIndex < editor->map.actors[index].waypointCount;
             waypointIndex++)
            includePoint(editor->map.actors[index].waypoints[waypointIndex].x,
                         editor->map.actors[index].waypoints[waypointIndex].z,
                         &hasPoint, &minimumX, &minimumZ,
                         &maximumX, &maximumZ);
        }
    if (!hasPoint || viewWidth <= 40.0f || viewHeight <= 40.0f)
        {
        editor->cameraX = 0.0f;
        editor->cameraZ = 0.0f;
        editor->zoom = 12.0f;
        return;
        }
    editor->cameraX = (minimumX + maximumX) * 0.5f;
    editor->cameraZ = (minimumZ + maximumZ) * 0.5f;
    extentX = maximumX - minimumX;
    extentZ = maximumZ - minimumZ;
    if (extentX < 1.0f) extentX = 1.0f;
    if (extentZ < 1.0f) extentZ = 1.0f;
    zoomX = (viewWidth - 40.0f) / extentX;
    zoomZ = (viewHeight - 40.0f) / extentZ;
    editor->zoom = zoomX < zoomZ ? zoomX : zoomZ;
    if (editor->zoom < 2.0f) editor->zoom = 2.0f;
    if (editor->zoom > 80.0f) editor->zoom = 80.0f;
    }
