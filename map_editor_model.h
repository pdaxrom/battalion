/*
 * Copyright (c) 2026 Battalion contributors.
 */
/****************************************************************/
/* Renderer-neutral editing operations for Battalion map packages. */
/****************************************************************/

#ifndef BATTALION_MAP_EDITOR_MODEL_H
#define BATTALION_MAP_EDITOR_MODEL_H

#include "entity_catalog.h"
#include "map_package.h"

#include <stddef.h>

#define MAP_EDITOR_PATH_SIZE 1024
#define MAP_EDITOR_STATUS_SIZE 256

#define MAP_EDITOR_LEGACY_NONE 0
#define MAP_EDITOR_LEGACY_ROAD 1
#define MAP_EDITOR_LEGACY_STRUCTURE 2

struct MapEditorModel
    {
    struct BattalionMapPackage map;
    struct EntityCatalog catalog;
    char assetRoot[MAP_EDITOR_PATH_SIZE];
    char path[MAP_EDITOR_PATH_SIZE];
    char status[MAP_EDITOR_STATUS_SIZE];
    int selectedActor;
    int selectedWaypoint;
    int selectedTrigger;
    int selectedArchetype;
    int selectedLegacyKind;
    int selectedLegacyIndex;
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

int mapEditorAddActor(struct MapEditorModel *editor,
                      size_t archetypeIndex, float x, float z);
int mapEditorDeleteActor(struct MapEditorModel *editor, size_t actorIndex);
int mapEditorMoveActor(struct MapEditorModel *editor, size_t actorIndex,
                       float x, float z);
int mapEditorSetActorId(struct MapEditorModel *editor, size_t actorIndex,
                        const char *id);

int mapEditorAddRoad(struct MapEditorModel *editor,
                     float x, float z, int type);
int mapEditorMoveRoad(struct MapEditorModel *editor, size_t roadIndex,
                      float x, float z);
int mapEditorDeleteRoad(struct MapEditorModel *editor, size_t roadIndex);

int mapEditorAddStructure(struct MapEditorModel *editor,
                          float x, float z, int type, int shape);
int mapEditorMoveStructure(struct MapEditorModel *editor,
                           size_t structureIndex, float x, float z);
int mapEditorDeleteStructure(struct MapEditorModel *editor,
                             size_t structureIndex);

int mapEditorAddWaypoint(struct MapEditorModel *editor, size_t actorIndex,
                         float x, float z);
int mapEditorMoveWaypoint(struct MapEditorModel *editor, size_t actorIndex,
                          size_t waypointIndex, float x, float z);
int mapEditorDeleteWaypoint(struct MapEditorModel *editor, size_t actorIndex,
                            size_t waypointIndex);

int mapEditorAddTrigger(struct MapEditorModel *editor, size_t actorIndex,
                        const char *event, int targetType,
                        const char *target);
int mapEditorDeleteTrigger(struct MapEditorModel *editor, size_t actorIndex,
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
