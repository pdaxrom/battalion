/*
 * Copyright (c) 2026 Battalion contributors.
 */
/* Renderer-neutral editing operations for canonical map v2 objects. */

#include "map_editor_model.h"

#include <errno.h>
#include <math.h>
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
    editor->selectedObject = -1;
    editor->selectedWaypoint = -1;
    editor->selectedGeometryPoint = -1;
    editor->selectedTrigger = -1;
    editor->selectedProperty = -1;
    }

static int objectIdExists(const struct BattalionMapPackage *map,
                          const char *id, size_t ignoredIndex)
    {
    size_t index;

    if (id[0] == '\0')
        return 0;
    for (index = 0; index < map->objectCount; index++)
        if (index != ignoredIndex && strcmp(map->objects[index].id, id) == 0)
            return 1;
    return 0;
    }

static int makeObjectId(const struct BattalionMapPackage *map,
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
        if (!objectIdExists(map, id, map->objectCount))
            return 1;
        }
    return 0;
    }

static const struct EntityPropertyDefinition *findPropertyDefinition(
    const struct EntityArchetype *archetype, const char *name)
    {
    size_t index;

    for (index = 0; index < archetype->propertyCount; index++)
        if (strcmp(archetype->properties[index].name, name) == 0)
            return &archetype->properties[index];
    return NULL;
    }

static int validPropertyValue(
    const struct EntityPropertyDefinition *definition, const char *value)
    {
    char *end;

    if (definition->type == ENTITY_PROPERTY_STRING)
        return 1;
    if (definition->type == ENTITY_PROPERTY_BOOLEAN)
        return strcmp(value, "true") == 0 || strcmp(value, "false") == 0;
    errno = 0;
    if (definition->type == ENTITY_PROPERTY_INTEGER)
        {
        (void)strtol(value, &end, 10);
        return errno == 0 && end != value && *end == '\0';
        }
    if (definition->type == ENTITY_PROPERTY_FLOAT)
        {
        (void)strtod(value, &end);
        return errno == 0 && end != value && *end == '\0';
        }
    return 0;
    }

static void freeObjectContents(struct BattalionMapObject *object)
    {
    free(object->waypoints);
    free(object->geometry);
    free(object->properties);
    free(object->triggers);
    memset(object, 0, sizeof(*object));
    }

void mapEditorModelInitialize(struct MapEditorModel *editor)
    {
    memset(editor, 0, sizeof(*editor));
    battalionMapPackageInit(&editor->map);
    entityCatalogInitialize(&editor->catalog);
    factionCatalogInitialize(&editor->factions);
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
    factionCatalogDestroy(&editor->factions);
    memset(editor, 0, sizeof(*editor));
    }

int mapEditorLoadCatalog(struct MapEditorModel *editor,
                         const char *assetRoot, const char *catalogPath)
    {
    char error[MAP_EDITOR_STATUS_SIZE];
    struct EntityCatalog loaded;
    struct FactionCatalog loadedFactions;
    size_t index;

    entityCatalogInitialize(&loaded);
    factionCatalogInitialize(&loadedFactions);
    if (!factionCatalogLoad(assetRoot, "factions/catalog.txt",
                            &loadedFactions, error, sizeof(error)))
        {
        setStatus(editor, error);
        return 0;
        }
    if (!entityCatalogLoad(assetRoot, catalogPath, &loaded,
                           error, sizeof(error)))
        {
        factionCatalogDestroy(&loadedFactions);
        setStatus(editor, error);
        return 0;
        }
    if (!copyText(editor->assetRoot, sizeof(editor->assetRoot), assetRoot))
        {
        entityCatalogDestroy(&loaded);
        factionCatalogDestroy(&loadedFactions);
        setStatus(editor, "Asset root path is too long");
        return 0;
        }
    entityCatalogDestroy(&editor->catalog);
    factionCatalogDestroy(&editor->factions);
    editor->catalog = loaded;
    editor->factions = loadedFactions;
    editor->selectedArchetype = -1;
    for (index = 0; index < loaded.count; index++)
        if (loaded.entries[index].editorPlaceable)
            {
            editor->selectedArchetype = (int)index;
            break;
            }
    setStatus(editor, "Archetype catalog loaded");
    return 1;
    }

int mapEditorNew(struct MapEditorModel *editor,
                 const char *id, const char *name)
    {
    struct BattalionMapPackage fresh;

    battalionMapPackageInit(&fresh);
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
    size_t objectIndex;
    size_t propertyIndex;
    const struct BattalionMapObject *object;
    const struct EntityArchetype *archetype;
    const struct EntityPropertyDefinition *definition;
    const struct BattalionMapObject *route;
    const struct EntityArchetype *routeArchetype;
    const struct BattalionMapObject *safeZone;
    const struct EntityArchetype *safeZoneArchetype;
    const char *resolvedFaction;
    size_t routeIndex;

    if (!battalionMapPackageValidate(&editor->map, error, sizeof(error)))
        {
        setStatus(editor, error);
        return 0;
        }
    for (objectIndex = 0; objectIndex < editor->map.objectCount;
         objectIndex++)
        {
        object = &editor->map.objects[objectIndex];
        archetype = entityCatalogFind(&editor->catalog, object->archetype);
        if (archetype == NULL)
            {
            setStatus(editor, "Map object has an unknown archetype");
            return 0;
            }
        if (!archetype->editorPlaceable ||
            archetype->placement == ENTITY_PLACEMENT_NONE)
            {
            setStatus(editor, "Map contains a non-placeable archetype");
            return 0;
            }
        resolvedFaction = object->faction[0] != '\0' ? object->faction :
                          archetype->faction;
        if (entityArchetypeIsActor(archetype) &&
            factionCatalogFind(&editor->factions, resolvedFaction) == NULL)
            {
            setStatus(editor, "Map object has an unknown faction");
            return 0;
            }
        if (archetype->kind == ENTITY_KIND_ROUTE)
            {
            if (object->waypointCount < 2 || object->patrolRoute[0] != '\0')
                {
                setStatus(editor, "Patrol route needs at least two points");
                return 0;
                }
            }
        else if (object->waypointCount > 0)
            {
            setStatus(editor, "Patrol points belong to a route object");
            return 0;
            }
        if (archetype->kind == ENTITY_KIND_ZONE &&
            object->geometryCount < 3)
            {
            setStatus(editor, "Guard zone needs at least three points");
            return 0;
            }
        if (object->patrolRoute[0] != '\0')
            {
            route = NULL;
            for (routeIndex = 0; routeIndex < editor->map.objectCount;
                 routeIndex++)
                if (strcmp(editor->map.objects[routeIndex].id,
                           object->patrolRoute) == 0)
                    route = &editor->map.objects[routeIndex];
            routeArchetype = route == NULL ? NULL : entityCatalogFind(
                &editor->catalog, route->archetype);
            if (routeArchetype == NULL ||
                routeArchetype->kind != ENTITY_KIND_ROUTE ||
                route->waypointCount < 2)
                {
                setStatus(editor, "Actor patrol-route is not a valid route");
                return 0;
                }
            }
        if (object->alarmResponse != ENTITY_ALARM_INHERIT)
            {
            if (!entityArchetypeIsActor(archetype) ||
                archetype->movement == ENTITY_MOVEMENT_STATIONARY)
                {
                setStatus(editor,
                          "Alarm response belongs to a moving actor");
                return 0;
                }
            if (object->alarmResponse == ENTITY_ALARM_RETREAT ||
                object->alarmResponse == ENTITY_ALARM_RANDOM)
                {
                safeZone = NULL;
                for (routeIndex = 0;
                     routeIndex < editor->map.objectCount; routeIndex++)
                    if (strcmp(editor->map.objects[routeIndex].id,
                               object->safeZone) == 0)
                        safeZone = &editor->map.objects[routeIndex];
                safeZoneArchetype = safeZone == NULL ? NULL :
                    entityCatalogFind(&editor->catalog,
                                      safeZone->archetype);
                if (safeZoneArchetype == NULL ||
                    safeZoneArchetype->kind != ENTITY_KIND_ZONE)
                    {
                    setStatus(editor,
                              "Actor safe-zone is not an area zone");
                    return 0;
                    }
                }
            }
        for (propertyIndex = 0; propertyIndex < object->propertyCount;
             propertyIndex++)
            {
            definition = findPropertyDefinition(
                archetype, object->properties[propertyIndex].name);
            if (definition == NULL || !validPropertyValue(
                    definition, object->properties[propertyIndex].value))
                {
                setStatus(editor, "Map object has an invalid property");
                return 0;
                }
            }
        }
    setStatus(editor, "Map is valid");
    return 1;
    }

int mapEditorSave(struct MapEditorModel *editor, const char *path)
    {
    char error[MAP_EDITOR_STATUS_SIZE];

    if (!mapEditorValidate(editor))
        return 0;
    if (!ensureParentDirectories(path))
        {
        setStatus(editor, "Cannot create map directory");
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
        setStatus(editor, "Saved path is too long");
        return 0;
        }
    editor->dirty = 0;
    setStatus(editor, "Map saved");
    return 1;
    }

float mapEditorSnapValue(const struct MapEditorModel *editor, float value)
    {
    if (!editor->snapEnabled || editor->gridSize <= 0.0f)
        return value;
    return (float)floor((double)(value / editor->gridSize) + 0.5) *
           editor->gridSize;
    }

int mapEditorSelectArchetypeForPlacement(struct MapEditorModel *editor,
                                         size_t archetypeIndex)
    {
    const struct EntityArchetype *archetype;

    if (editor == NULL || archetypeIndex >= editor->catalog.count)
        return 0;
    archetype = &editor->catalog.entries[archetypeIndex];
    if (!archetype->editorPlaceable ||
        archetype->placement == ENTITY_PLACEMENT_NONE)
        return 0;
    editor->selectedArchetype = (int)archetypeIndex;
    resetSelection(editor);
    setStatus(editor, "Archetype selected for placement");
    return 1;
    }

int mapEditorAddSelectedObject(struct MapEditorModel *editor,
                               float x, float z)
    {
    if (editor == NULL || editor->selectedArchetype < 0)
        return 0;
    return mapEditorAddObject(editor, (size_t)editor->selectedArchetype,
                              x, z);
    }

int mapEditorAddObject(struct MapEditorModel *editor,
                       size_t archetypeIndex, float x, float z)
    {
    struct BattalionMapObject *objects;
    struct BattalionMapObject *object;
    const struct EntityArchetype *archetype;
    size_t propertyIndex;

    if (archetypeIndex >= editor->catalog.count)
        return 0;
    archetype = &editor->catalog.entries[archetypeIndex];
    if (!archetype->editorPlaceable ||
        archetype->placement == ENTITY_PLACEMENT_NONE)
        return 0;
    objects = (struct BattalionMapObject *)realloc(
        editor->map.objects,
        (editor->map.objectCount + 1) * sizeof(struct BattalionMapObject));
    if (objects == NULL)
        return 0;
    editor->map.objects = objects;
    object = &objects[editor->map.objectCount];
    memset(object, 0, sizeof(*object));
    copyText(object->archetype, sizeof(object->archetype), archetype->id);
    copyText(object->description, sizeof(object->description),
             archetype->description);
    object->x = mapEditorSnapValue(editor, x);
    object->z = mapEditorSnapValue(editor, z);
    object->scale = 1.0f;
    object->active = 1;
    object->patrolMode = ENTITY_PATROL_LOOP;
    if (!makeObjectId(&editor->map, archetype->id,
                      object->id, sizeof(object->id)))
        return 0;
    object->propertyCount = archetype->propertyCount;
    if (object->propertyCount > 0)
        object->properties = (struct BattalionMapProperty *)calloc(
            object->propertyCount, sizeof(struct BattalionMapProperty));
    if (object->propertyCount > 0 && object->properties == NULL)
        return 0;
    for (propertyIndex = 0; propertyIndex < object->propertyCount;
         propertyIndex++)
        {
        copyText(object->properties[propertyIndex].name,
                 sizeof(object->properties[propertyIndex].name),
                 archetype->properties[propertyIndex].name);
        copyText(object->properties[propertyIndex].value,
                 sizeof(object->properties[propertyIndex].value),
                 archetype->properties[propertyIndex].defaultValue);
        }
    editor->selectedObject = (int)editor->map.objectCount;
    editor->map.objectCount++;
    editor->selectedWaypoint = -1;
    editor->selectedGeometryPoint = -1;
    editor->selectedTrigger = -1;
    editor->selectedProperty = -1;
    editor->dirty = 1;
    setStatus(editor, "Object added");
    return 1;
    }

int mapEditorDeleteObject(struct MapEditorModel *editor, size_t objectIndex)
    {
    size_t index;

    if (objectIndex >= editor->map.objectCount)
        return 0;
    if (editor->map.objects[objectIndex].id[0] != '\0')
        for (index = 0; index < editor->map.objectCount; index++)
            if (strcmp(editor->map.objects[index].patrolRoute,
                       editor->map.objects[objectIndex].id) == 0 ||
                strcmp(editor->map.objects[index].safeZone,
                       editor->map.objects[objectIndex].id) == 0)
                {
                setStatus(editor, "Object is referenced by an actor");
                return 0;
                }
    freeObjectContents(&editor->map.objects[objectIndex]);
    if (objectIndex + 1 < editor->map.objectCount)
        memmove(&editor->map.objects[objectIndex],
                &editor->map.objects[objectIndex + 1],
                (editor->map.objectCount - objectIndex - 1) *
                    sizeof(struct BattalionMapObject));
    editor->map.objectCount--;
    resetSelection(editor);
    editor->dirty = 1;
    setStatus(editor, "Object deleted");
    return 1;
    }

int mapEditorMoveObject(struct MapEditorModel *editor, size_t objectIndex,
                        float x, float z)
    {
    if (objectIndex >= editor->map.objectCount)
        return 0;
    editor->map.objects[objectIndex].x = mapEditorSnapValue(editor, x);
    editor->map.objects[objectIndex].z = mapEditorSnapValue(editor, z);
    editor->dirty = 1;
    return 1;
    }

int mapEditorSetObjectId(struct MapEditorModel *editor, size_t objectIndex,
                         const char *id)
    {
    size_t triggerIndex;
    size_t sourceIndex;
    char oldId[ENTITY_NAME_SIZE];

    if (objectIndex >= editor->map.objectCount || id == NULL ||
        id[0] == '\0' || strlen(id) >= ENTITY_NAME_SIZE ||
        objectIdExists(&editor->map, id, objectIndex))
        return 0;
    strcpy(oldId, editor->map.objects[objectIndex].id);
    strcpy(editor->map.objects[objectIndex].id, id);
    for (sourceIndex = 0; sourceIndex < editor->map.objectCount;
         sourceIndex++)
        {
        if (strcmp(editor->map.objects[sourceIndex].patrolRoute, oldId) == 0)
            strcpy(editor->map.objects[sourceIndex].patrolRoute, id);
        if (strcmp(editor->map.objects[sourceIndex].safeZone, oldId) == 0)
            strcpy(editor->map.objects[sourceIndex].safeZone, id);
        for (triggerIndex = 0;
             triggerIndex < editor->map.objects[sourceIndex].triggerCount;
             triggerIndex++)
            if (editor->map.objects[sourceIndex].triggers[triggerIndex].targetType ==
                    BATTALION_MAP_TRIGGER_ID &&
                strcmp(editor->map.objects[sourceIndex].triggers[triggerIndex].target,
                       oldId) == 0)
                strcpy(editor->map.objects[sourceIndex].triggers[triggerIndex].target,
                       id);
        }
    editor->dirty = 1;
    return 1;
    }

int mapEditorSetObjectProperty(struct MapEditorModel *editor,
                               size_t objectIndex, size_t propertyIndex,
                               const char *value)
    {
    struct BattalionMapObject *object;
    const struct EntityArchetype *archetype;
    const struct EntityPropertyDefinition *definition;

    if (objectIndex >= editor->map.objectCount || value == NULL)
        return 0;
    object = &editor->map.objects[objectIndex];
    if (propertyIndex >= object->propertyCount ||
        strlen(value) >= sizeof(object->properties[propertyIndex].value))
        return 0;
    archetype = entityCatalogFind(&editor->catalog, object->archetype);
    if (archetype == NULL)
        return 0;
    definition = findPropertyDefinition(
        archetype, object->properties[propertyIndex].name);
    if (definition == NULL || !validPropertyValue(definition, value))
        return 0;
    strcpy(object->properties[propertyIndex].value, value);
    editor->dirty = 1;
    return 1;
    }

static int addPoint(void **points, size_t *count, size_t pointSize,
                    float x, float z, int geometry,
                    struct MapEditorModel *editor)
    {
    char *grown;
    struct BattalionMapPoint *geometryPoint;
    struct EntityWaypoint *waypoint;

    grown = (char *)realloc(*points, (*count + 1) * pointSize);
    if (grown == NULL)
        return 0;
    *points = grown;
    if (geometry)
        {
        geometryPoint = &((struct BattalionMapPoint *)grown)[*count];
        geometryPoint->x = mapEditorSnapValue(editor, x);
        geometryPoint->z = mapEditorSnapValue(editor, z);
        }
    else
        {
        waypoint = &((struct EntityWaypoint *)grown)[*count];
        waypoint->x = mapEditorSnapValue(editor, x);
        waypoint->z = mapEditorSnapValue(editor, z);
        }
    (*count)++;
    editor->dirty = 1;
    return 1;
    }

int mapEditorAddWaypoint(struct MapEditorModel *editor, size_t objectIndex,
                         float x, float z)
    {
    struct BattalionMapObject *object;
    const struct EntityArchetype *archetype;

    if (objectIndex >= editor->map.objectCount)
        return 0;
    object = &editor->map.objects[objectIndex];
    archetype = entityCatalogFind(&editor->catalog, object->archetype);
    if (archetype == NULL || archetype->kind != ENTITY_KIND_ROUTE)
        return 0;
    if (!addPoint((void **)&object->waypoints, &object->waypointCount,
                  sizeof(struct EntityWaypoint), x, z, 0, editor))
        return 0;
    editor->selectedWaypoint = (int)(object->waypointCount - 1);
    return 1;
    }

int mapEditorMoveWaypoint(struct MapEditorModel *editor, size_t objectIndex,
                          size_t waypointIndex, float x, float z)
    {
    struct BattalionMapObject *object;

    if (objectIndex >= editor->map.objectCount)
        return 0;
    object = &editor->map.objects[objectIndex];
    if (waypointIndex >= object->waypointCount)
        return 0;
    object->waypoints[waypointIndex].x = mapEditorSnapValue(editor, x);
    object->waypoints[waypointIndex].z = mapEditorSnapValue(editor, z);
    editor->dirty = 1;
    return 1;
    }

int mapEditorDeleteWaypoint(struct MapEditorModel *editor, size_t objectIndex,
                            size_t waypointIndex)
    {
    struct BattalionMapObject *object;

    if (objectIndex >= editor->map.objectCount)
        return 0;
    object = &editor->map.objects[objectIndex];
    if (waypointIndex >= object->waypointCount)
        return 0;
    if (waypointIndex + 1 < object->waypointCount)
        memmove(&object->waypoints[waypointIndex],
                &object->waypoints[waypointIndex + 1],
                (object->waypointCount - waypointIndex - 1) *
                    sizeof(struct EntityWaypoint));
    object->waypointCount--;
    editor->selectedWaypoint = -1;
    editor->dirty = 1;
    return 1;
    }

int mapEditorAddGeometryPoint(struct MapEditorModel *editor,
                              size_t objectIndex, float x, float z)
    {
    struct BattalionMapObject *object;

    if (objectIndex >= editor->map.objectCount)
        return 0;
    object = &editor->map.objects[objectIndex];
    if (!addPoint((void **)&object->geometry, &object->geometryCount,
                  sizeof(struct BattalionMapPoint), x, z, 1, editor))
        return 0;
    editor->selectedGeometryPoint = (int)(object->geometryCount - 1);
    return 1;
    }

int mapEditorMoveGeometryPoint(struct MapEditorModel *editor,
                               size_t objectIndex, size_t pointIndex,
                               float x, float z)
    {
    struct BattalionMapObject *object;

    if (objectIndex >= editor->map.objectCount)
        return 0;
    object = &editor->map.objects[objectIndex];
    if (pointIndex >= object->geometryCount)
        return 0;
    object->geometry[pointIndex].x = mapEditorSnapValue(editor, x);
    object->geometry[pointIndex].z = mapEditorSnapValue(editor, z);
    editor->dirty = 1;
    return 1;
    }

int mapEditorDeleteGeometryPoint(struct MapEditorModel *editor,
                                 size_t objectIndex, size_t pointIndex)
    {
    struct BattalionMapObject *object;

    if (objectIndex >= editor->map.objectCount)
        return 0;
    object = &editor->map.objects[objectIndex];
    if (pointIndex >= object->geometryCount)
        return 0;
    if (pointIndex + 1 < object->geometryCount)
        memmove(&object->geometry[pointIndex],
                &object->geometry[pointIndex + 1],
                (object->geometryCount - pointIndex - 1) *
                    sizeof(struct BattalionMapPoint));
    object->geometryCount--;
    editor->selectedGeometryPoint = -1;
    editor->dirty = 1;
    return 1;
    }

int mapEditorAddTrigger(struct MapEditorModel *editor, size_t objectIndex,
                        const char *event, int targetType,
                        const char *target)
    {
    struct BattalionMapObject *object;
    struct BattalionMapTrigger *triggers;
    struct BattalionMapTrigger *trigger;

    if (objectIndex >= editor->map.objectCount || event == NULL ||
        target == NULL || event[0] == '\0' || target[0] == '\0' ||
        strlen(event) >= ENTITY_NAME_SIZE || strlen(target) >= ENTITY_NAME_SIZE ||
        (targetType != BATTALION_MAP_TRIGGER_ID &&
         targetType != BATTALION_MAP_TRIGGER_GROUP))
        return 0;
    object = &editor->map.objects[objectIndex];
    triggers = (struct BattalionMapTrigger *)realloc(
        object->triggers,
        (object->triggerCount + 1) * sizeof(struct BattalionMapTrigger));
    if (triggers == NULL)
        return 0;
    object->triggers = triggers;
    trigger = &triggers[object->triggerCount];
    memset(trigger, 0, sizeof(*trigger));
    strcpy(trigger->event, event);
    strcpy(trigger->target, target);
    trigger->targetType = targetType;
    object->triggerCount++;
    editor->selectedTrigger = (int)(object->triggerCount - 1);
    editor->dirty = 1;
    return 1;
    }

int mapEditorDeleteTrigger(struct MapEditorModel *editor, size_t objectIndex,
                           size_t triggerIndex)
    {
    struct BattalionMapObject *object;

    if (objectIndex >= editor->map.objectCount)
        return 0;
    object = &editor->map.objects[objectIndex];
    if (triggerIndex >= object->triggerCount)
        return 0;
    if (triggerIndex + 1 < object->triggerCount)
        memmove(&object->triggers[triggerIndex],
                &object->triggers[triggerIndex + 1],
                (object->triggerCount - triggerIndex - 1) *
                    sizeof(struct BattalionMapTrigger));
    object->triggerCount--;
    editor->selectedTrigger = -1;
    editor->dirty = 1;
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
    *screenY = viewY + viewHeight * 0.5f +
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
    *worldZ = editor->cameraZ +
              (screenY - viewY - viewHeight * 0.5f) / editor->zoom;
    }

static void includePoint(float x, float z, int *hasPoint,
                         float *minimumX, float *maximumX,
                         float *minimumZ, float *maximumZ)
    {
    if (!*hasPoint)
        {
        *minimumX = *maximumX = x;
        *minimumZ = *maximumZ = z;
        *hasPoint = 1;
        return;
        }
    if (x < *minimumX) *minimumX = x;
    if (x > *maximumX) *maximumX = x;
    if (z < *minimumZ) *minimumZ = z;
    if (z > *maximumZ) *maximumZ = z;
    }

void mapEditorFitContents(struct MapEditorModel *editor,
                          float viewWidth, float viewHeight)
    {
    size_t objectIndex;
    size_t pointIndex;
    int hasPoint;
    float minimumX;
    float maximumX;
    float minimumZ;
    float maximumZ;
    float width;
    float height;
    float zoomX;
    float zoomZ;

    hasPoint = 0;
    minimumX = maximumX = minimumZ = maximumZ = 0.0f;
    for (objectIndex = 0; objectIndex < editor->map.objectCount;
         objectIndex++)
        {
        const struct BattalionMapObject *object;

        object = &editor->map.objects[objectIndex];
        includePoint(object->x, object->z, &hasPoint,
                     &minimumX, &maximumX, &minimumZ, &maximumZ);
        for (pointIndex = 0; pointIndex < object->waypointCount; pointIndex++)
            includePoint(object->waypoints[pointIndex].x,
                         object->waypoints[pointIndex].z, &hasPoint,
                         &minimumX, &maximumX, &minimumZ, &maximumZ);
        for (pointIndex = 0; pointIndex < object->geometryCount; pointIndex++)
            includePoint(object->geometry[pointIndex].x,
                         object->geometry[pointIndex].z, &hasPoint,
                         &minimumX, &maximumX, &minimumZ, &maximumZ);
        }
    if (!hasPoint)
        return;
    editor->cameraX = (minimumX + maximumX) * 0.5f;
    editor->cameraZ = (minimumZ + maximumZ) * 0.5f;
    width = maximumX - minimumX;
    height = maximumZ - minimumZ;
    if (width < 1.0f) width = 1.0f;
    if (height < 1.0f) height = 1.0f;
    zoomX = (viewWidth - 40.0f) / width;
    zoomZ = (viewHeight - 40.0f) / height;
    editor->zoom = zoomX < zoomZ ? zoomX : zoomZ;
    if (editor->zoom < 1.0f) editor->zoom = 1.0f;
    if (editor->zoom > 80.0f) editor->zoom = 80.0f;
    }
