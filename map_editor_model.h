/*
 * Copyright (c) 2026 Battalion contributors.
 */
/****************************************************************/
/* Renderer-neutral editing operations for Battalion map packages. */
/****************************************************************/

#ifndef BATTALION_MAP_EDITOR_MODEL_H
#define BATTALION_MAP_EDITOR_MODEL_H

#include "entity_catalog.h"
#include "faction_catalog.h"
#include "map_package.h"

#include <stddef.h>

#define MAP_EDITOR_PATH_SIZE 1024
#define MAP_EDITOR_STATUS_SIZE 256

struct MapEditorModel
    {
    struct BattalionMapPackage map;
    struct EntityCatalog catalog;
    struct FactionCatalog factions;
    char assetRoot[MAP_EDITOR_PATH_SIZE];
    char path[MAP_EDITOR_PATH_SIZE];
    char status[MAP_EDITOR_STATUS_SIZE];
    int selectedObject;
    int selectedWaypoint;
    int selectedGeometryPoint;
    int selectedTrigger;
    int selectedProperty;
    int selectedArchetype;
    int dirty;
    int gridVisible;
    int snapEnabled;
    float gridSize;
    float cameraX;
    float cameraZ;
    float zoom;
    };

void mapEditorModelInitialize(struct MapEditorModel *editor);
void mapEditorModelDestroy(struct MapEditorModel *editor);

int mapEditorLoadCatalog(struct MapEditorModel *editor,
                         const char *assetRoot, const char *catalogPath);
int mapEditorNew(struct MapEditorModel *editor,
                 const char *id, const char *name);
int mapEditorLoad(struct MapEditorModel *editor, const char *path);
int mapEditorSave(struct MapEditorModel *editor, const char *path);
int mapEditorValidate(struct MapEditorModel *editor);

int mapEditorSelectArchetypeForPlacement(struct MapEditorModel *editor,
                                         size_t archetypeIndex);
int mapEditorAddSelectedObject(struct MapEditorModel *editor,
                               float x, float z);
int mapEditorAddObject(struct MapEditorModel *editor,
                       size_t archetypeIndex, float x, float z);
int mapEditorDeleteObject(struct MapEditorModel *editor, size_t objectIndex);
int mapEditorMoveObject(struct MapEditorModel *editor, size_t objectIndex,
                        float x, float z);
int mapEditorSetObjectId(struct MapEditorModel *editor, size_t objectIndex,
                         const char *id);
int mapEditorSetObjectProperty(struct MapEditorModel *editor,
                               size_t objectIndex, size_t propertyIndex,
                               const char *value);

int mapEditorAddWaypoint(struct MapEditorModel *editor, size_t objectIndex,
                         float x, float z);
int mapEditorMoveWaypoint(struct MapEditorModel *editor, size_t objectIndex,
                          size_t waypointIndex, float x, float z);
int mapEditorDeleteWaypoint(struct MapEditorModel *editor, size_t objectIndex,
                            size_t waypointIndex);

int mapEditorAddGeometryPoint(struct MapEditorModel *editor,
                              size_t objectIndex, float x, float z);
int mapEditorMoveGeometryPoint(struct MapEditorModel *editor,
                               size_t objectIndex, size_t pointIndex,
                               float x, float z);
int mapEditorDeleteGeometryPoint(struct MapEditorModel *editor,
                                 size_t objectIndex, size_t pointIndex);

int mapEditorAddTrigger(struct MapEditorModel *editor, size_t objectIndex,
                        const char *event, int targetType,
                        const char *target);
int mapEditorDeleteTrigger(struct MapEditorModel *editor, size_t objectIndex,
                           size_t triggerIndex);

float mapEditorSnapValue(const struct MapEditorModel *editor, float value);
void mapEditorWorldToScreen(const struct MapEditorModel *editor,
                            float viewX, float viewY,
                            float viewWidth, float viewHeight,
                            float worldX, float worldZ,
                            float *screenX, float *screenY);
void mapEditorScreenToWorld(const struct MapEditorModel *editor,
                            float viewX, float viewY,
                            float viewWidth, float viewHeight,
                            float screenX, float screenY,
                            float *worldX, float *worldZ);
void mapEditorFitContents(struct MapEditorModel *editor,
                          float viewWidth, float viewHeight);

#endif
