/*
 * Copyright (c) 2026 Battalion contributors.
 */
/****************************************************************/
/* Battalion map editor MVP.                                      */
/****************************************************************/

#include "map_editor_gui.h"
#include "map_editor_model.h"
#include "tk.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EDITOR_LEFT_WIDTH 250.0f
#define EDITOR_RIGHT_WIDTH 310.0f
#define EDITOR_TOP_HEIGHT 36.0f
#define EDITOR_MIN_VIEW 120.0f

#define EDITOR_TOOL_SELECT 0
#define EDITOR_TOOL_ACTOR 1
#define EDITOR_TOOL_WAYPOINT 2
#define EDITOR_TOOL_ROAD 3
#define EDITOR_TOOL_STRUCTURE 4

#define EDITOR_FILE_NONE 0
#define EDITOR_FILE_OPEN 1
#define EDITOR_FILE_SAVE 2

int paused;

static struct MapEditorModel editor;
static struct MapEditorGui gui;
static struct MapEditorGuiInput guiInput;
static int windowWidth = 1280;
static int windowHeight = 800;
static int editorTool;
static int draggingActor = -1;
static int draggingWaypoint = -1;
static int draggingLegacyKind = MAP_EDITOR_LEGACY_NONE;
static int draggingLegacyIndex = -1;
static int viewportLeftPressed;
static int placementRoadType;
static int placementStructureType;
static int placementStructureShape;
static int deleteRequested;
static int fileDialogMode;
static int newDialogVisible;
static char filePath[MAP_EDITOR_PATH_SIZE];
static char newMapId[32] = "untitled";
static char newMapName[128] = "Untitled";
static struct nk_rect viewportRect;

static void copyStatus(const char *message)
    {
    strncpy(editor.status, message, sizeof(editor.status) - 1);
    editor.status[sizeof(editor.status) - 1] = '\0';
    }

static int editText(struct nk_context *context, char *value, int capacity)
    {
    char before[MAP_EDITOR_PATH_SIZE];

    if (capacity > (int)sizeof(before))
        capacity = (int)sizeof(before);
    strncpy(before, value, (size_t)capacity);
    before[capacity - 1] = '\0';
    nk_edit_string_zero_terminated(
        context, NK_EDIT_FIELD | NK_EDIT_SIG_ENTER,
        value, capacity, nk_filter_ascii);
    return strcmp(before, value) != 0;
    }

static void selectActor(int index)
    {
    editor.selectedActor = index;
    editor.selectedWaypoint = -1;
    editor.selectedTrigger = -1;
    editor.selectedLegacyKind = MAP_EDITOR_LEGACY_NONE;
    editor.selectedLegacyIndex = -1;
    draggingActor = -1;
    draggingWaypoint = -1;
    draggingLegacyKind = MAP_EDITOR_LEGACY_NONE;
    draggingLegacyIndex = -1;
    }

static void selectLegacyObject(int kind, int index)
    {
    editor.selectedActor = -1;
    editor.selectedWaypoint = -1;
    editor.selectedTrigger = -1;
    editor.selectedLegacyKind = kind;
    editor.selectedLegacyIndex = index;
    draggingActor = -1;
    draggingWaypoint = -1;
    draggingLegacyKind = MAP_EDITOR_LEGACY_NONE;
    draggingLegacyIndex = -1;
    }

static const char *archetypeLabel(const struct EntityArchetype *archetype)
    {
    return archetype->displayName[0] != '\0' ?
           archetype->displayName : archetype->id;
    }

static void saveCurrentMap(void)
    {
    if (editor.path[0] == '\0')
        {
        fileDialogMode = EDITOR_FILE_SAVE;
        if (filePath[0] == '\0')
            sprintf(filePath, "assets/maps/%s/map.json", editor.map.id);
        }
    else
        mapEditorSave(&editor, editor.path);
    }

static void runMakeTest(void)
    {
    int result;

    copyStatus("Running make test...");
    result = system("make test");
    if (result == 0)
        copyStatus("make test passed");
    else
        copyStatus("make test failed; see terminal output");
    }

static void drawTopBar(struct nk_context *context)
    {
    if (nk_begin(context, "MainMenu",
                 nk_rect(0.0f, 0.0f, (float)windowWidth,
                         EDITOR_TOP_HEIGHT),
                 NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BACKGROUND))
        {
        nk_menubar_begin(context);
        nk_layout_row_begin(context, NK_STATIC, 28.0f, 6);
        nk_layout_row_push(context, 52.0f);
        if (nk_menu_begin_label(context, "File", NK_TEXT_LEFT,
                                nk_vec2(190.0f, 190.0f)))
            {
            nk_layout_row_dynamic(context, 24.0f, 1);
            if (nk_menu_item_label(context, "New v2", NK_TEXT_LEFT))
                newDialogVisible = 1;
            if (nk_menu_item_label(context, "Open...", NK_TEXT_LEFT))
                {
                fileDialogMode = EDITOR_FILE_OPEN;
                if (editor.path[0] != '\0')
                    strcpy(filePath, editor.path);
                }
            if (nk_menu_item_label(context, "Save", NK_TEXT_LEFT))
                saveCurrentMap();
            if (nk_menu_item_label(context, "Save As...", NK_TEXT_LEFT))
                {
                fileDialogMode = EDITOR_FILE_SAVE;
                if (editor.path[0] != '\0')
                    strcpy(filePath, editor.path);
                }
            if (nk_menu_item_label(context, "Quit", NK_TEXT_LEFT))
                tkQuit();
            nk_menu_end(context);
            }
        nk_layout_row_push(context, 52.0f);
        if (nk_menu_begin_label(context, "Edit", NK_TEXT_LEFT,
                                nk_vec2(220.0f, 150.0f)))
            {
            nk_layout_row_dynamic(context, 24.0f, 1);
            if (nk_menu_item_label(context, "Select tool", NK_TEXT_LEFT))
                editorTool = EDITOR_TOOL_SELECT;
            if (nk_menu_item_label(context, "Delete selection", NK_TEXT_LEFT))
                deleteRequested = 1;
            if (nk_menu_item_label(context, "Add waypoint", NK_TEXT_LEFT) &&
                editor.selectedActor >= 0)
                editorTool = EDITOR_TOOL_WAYPOINT;
            nk_menu_end(context);
            }
        nk_layout_row_push(context, 52.0f);
        if (nk_menu_begin_label(context, "View", NK_TEXT_LEFT,
                                nk_vec2(190.0f, 180.0f)))
            {
            nk_bool checked;
            nk_layout_row_dynamic(context, 24.0f, 1);
            checked = editor.gridVisible;
            if (nk_checkbox_label(context, "Grid", &checked))
                editor.gridVisible = checked;
            checked = editor.snapEnabled;
            if (nk_checkbox_label(context, "Snap", &checked))
                editor.snapEnabled = checked;
            if (nk_menu_item_label(context, "Fit contents", NK_TEXT_LEFT))
                mapEditorFitContents(&editor,
                    viewportRect.w > 0.0f ? viewportRect.w : 700.0f,
                    viewportRect.h > 0.0f ? viewportRect.h : 600.0f);
            if (nk_menu_item_label(context, "Zoom in", NK_TEXT_LEFT))
                editor.zoom = editor.zoom < 160.0f ? editor.zoom * 1.25f :
                              editor.zoom;
            if (nk_menu_item_label(context, "Zoom out", NK_TEXT_LEFT))
                editor.zoom = editor.zoom > 1.0f ? editor.zoom / 1.25f :
                              editor.zoom;
            nk_menu_end(context);
            }
        nk_layout_row_push(context, 92.0f);
        if (nk_menu_begin_label(context, "Validate", NK_TEXT_LEFT,
                                nk_vec2(220.0f, 100.0f)))
            {
            nk_layout_row_dynamic(context, 24.0f, 1);
            if (nk_menu_item_label(context, "Validate map", NK_TEXT_LEFT))
                mapEditorValidate(&editor);
            if (nk_menu_item_label(context, "Run make test", NK_TEXT_LEFT))
                runMakeTest();
            nk_menu_end(context);
            }
        nk_layout_row_push(context, 240.0f);
        nk_label(context,
                 editorTool == EDITOR_TOOL_ACTOR ? "Tool: place actor" :
                 editorTool == EDITOR_TOOL_WAYPOINT ? "Tool: add waypoint" :
                 editorTool == EDITOR_TOOL_ROAD ? "Tool: place road" :
                 editorTool == EDITOR_TOOL_STRUCTURE ?
                     (placementStructureType == 0 ? "Tool: place tree" :
                      placementStructureType == 1 ? "Tool: place building" :
                      placementStructureType == 2 ? "Tool: place lake" :
                      "Tool: place hill") :
                 "Tool: select/move",
                 NK_TEXT_LEFT);
        nk_layout_row_push(context,
                           (float)windowWidth - 504.0f > 80.0f ?
                           (float)windowWidth - 504.0f : 80.0f);
        nk_label(context, editor.status, NK_TEXT_RIGHT);
        nk_layout_row_end(context);
        nk_menubar_end(context);
        }
    nk_end(context);
    }

static void archetypePalette(struct nk_context *context)
    {
    size_t index;

    nk_layout_row_dynamic(context, 22.0f, 1);
    nk_label(context, "Archetypes", NK_TEXT_LEFT);
    for (index = 0; index < editor.catalog.count; index++)
        {
        nk_bool selected;
        const struct EntityArchetype *entry;

        entry = &editor.catalog.entries[index];
        selected = editor.selectedArchetype == (int)index;
        if (nk_selectable_label(context, archetypeLabel(entry),
                                NK_TEXT_LEFT, &selected) && selected)
            editor.selectedArchetype = (int)index;
        }
    nk_layout_row_dynamic(context, 28.0f, 1);
    if (nk_button_label(context, "Place actor") &&
        editor.selectedArchetype >= 0)
        editorTool = EDITOR_TOOL_ACTOR;
    nk_layout_row_dynamic(context, 24.0f, 1);
    if (nk_button_label(context, "Select / move (Esc)"))
        editorTool = EDITOR_TOOL_SELECT;
    nk_layout_row_dynamic(context, 22.0f, 1);
    nk_label(context, "Map objects", NK_TEXT_LEFT);
    nk_layout_row_dynamic(context, 24.0f, 2);
    if (nk_button_label(context, "Road"))
        editorTool = EDITOR_TOOL_ROAD;
    if (nk_button_label(context, "Tree"))
        {
        placementStructureType = 0;
        placementStructureShape = 0;
        editorTool = EDITOR_TOOL_STRUCTURE;
        }
    if (nk_button_label(context, "Building"))
        {
        placementStructureType = 1;
        placementStructureShape = 0;
        editorTool = EDITOR_TOOL_STRUCTURE;
        }
    nk_layout_row_dynamic(context, 24.0f, 3);
    if (nk_button_label(context, "Lake"))
        {
        placementStructureType = 2;
        placementStructureShape = 0;
        editorTool = EDITOR_TOOL_STRUCTURE;
        }
    if (nk_button_label(context, "Hill"))
        {
        placementStructureType = 3;
        placementStructureShape = 0;
        editorTool = EDITOR_TOOL_STRUCTURE;
        }
    if (editorTool == EDITOR_TOOL_ROAD)
        {
        nk_layout_row_dynamic(context, 24.0f, 1);
        nk_property_int(context, "road type", 0, &placementRoadType,
                        3, 1, 1.0f);
        }
    else if (editorTool == EDITOR_TOOL_STRUCTURE)
        {
        nk_layout_row_dynamic(context, 24.0f, 1);
        nk_property_int(context, "shape", 0, &placementStructureShape,
                        placementStructureType == 0 ? 1 :
                        placementStructureType == 1 ? 37 :
                        placementStructureType == 2 ? 4 : 13,
                        1, 1.0f);
        }
    }

static void objectRow(struct nk_context *context, const char *label,
                      int kind, int index)
    {
    nk_bool selected;

    selected = kind == MAP_EDITOR_LEGACY_NONE ?
               editor.selectedActor == index :
               editor.selectedLegacyKind == kind &&
               editor.selectedLegacyIndex == index;
    if (nk_selectable_label(context, label, NK_TEXT_LEFT, &selected) &&
        selected)
        {
        if (kind == MAP_EDITOR_LEGACY_NONE)
            selectActor(index);
        else
            selectLegacyObject(kind, index);
        editorTool = EDITOR_TOOL_SELECT;
        }
    }

static void objectList(struct nk_context *context)
    {
    char label[192];
    size_t index;

    nk_layout_row_dynamic(context, 22.0f, 1);
    if (nk_tree_push(context, NK_TREE_TAB, "Actors", NK_MAXIMIZED))
        {
        for (index = 0; index < editor.map.actorCount; index++)
            {
            sprintf(label, "%s : %s",
                    editor.map.actors[index].id,
                    editor.map.actors[index].archetype);
            objectRow(context, label, MAP_EDITOR_LEGACY_NONE, (int)index);
            }
        nk_tree_pop(context);
        }
    if (nk_tree_push(context, NK_TREE_TAB, "Roads", NK_MINIMIZED))
        {
        for (index = 0; index < editor.map.roadCount; index++)
            {
            sprintf(label, "road %lu  %.2f, %.2f  type %d",
                    (unsigned long)index,
                    editor.map.roads[index].x, editor.map.roads[index].z,
                    editor.map.roads[index].type);
            objectRow(context, label, MAP_EDITOR_LEGACY_ROAD, (int)index);
            }
        nk_tree_pop(context);
        }
    if (nk_tree_push(context, NK_TREE_TAB, "Structures", NK_MINIMIZED))
        {
        for (index = 0; index < editor.map.structureCount; index++)
            {
            sprintf(label, "structure %lu  %.2f, %.2f  %d/%d",
                    (unsigned long)index,
                    editor.map.structures[index].x,
                    editor.map.structures[index].z,
                    editor.map.structures[index].type,
                    editor.map.structures[index].shape);
            objectRow(context, label, MAP_EDITOR_LEGACY_STRUCTURE,
                      (int)index);
            }
        nk_tree_pop(context);
        }
    }

static void drawLeftPanel(struct nk_context *context)
    {
    float height;

    height = (float)windowHeight - EDITOR_TOP_HEIGHT;
    if (height < 1.0f) height = 1.0f;
    if (nk_begin(context, "Palette and objects",
                 nk_rect(0.0f, EDITOR_TOP_HEIGHT,
                         EDITOR_LEFT_WIDTH, height),
                 NK_WINDOW_BORDER | NK_WINDOW_TITLE))
        {
        archetypePalette(context);
        nk_layout_row_dynamic(context, 8.0f, 1);
        nk_spacing(context, 1);
        objectList(context);
        }
    nk_end(context);
    }

static void archetypeCombo(struct nk_context *context,
                           struct BattalionMapActor *actor)
    {
    const struct EntityArchetype *current;
    const char *label;
    size_t index;

    current = entityCatalogFind(&editor.catalog, actor->archetype);
    label = current != NULL ? archetypeLabel(current) : actor->archetype;
    if (nk_combo_begin_label(context, label, nk_vec2(260.0f, 240.0f)))
        {
        nk_layout_row_dynamic(context, 22.0f, 1);
        for (index = 0; index < editor.catalog.count; index++)
            if (nk_combo_item_label(context,
                                    archetypeLabel(&editor.catalog.entries[index]),
                                    NK_TEXT_LEFT))
                {
                strcpy(actor->archetype, editor.catalog.entries[index].id);
                editor.dirty = 1;
                }
        nk_combo_end(context);
        }
    }

static void waypointProperties(struct nk_context *context,
                               struct BattalionMapActor *actor)
    {
    char label[96];
    size_t index;

    if (!nk_tree_push(context, NK_TREE_TAB, "Patrol waypoints", NK_MAXIMIZED))
        return;
    nk_layout_row_dynamic(context, 22.0f, 1);
    for (index = 0; index < actor->waypointCount; index++)
        {
        nk_bool selected;
        selected = editor.selectedWaypoint == (int)index;
        sprintf(label, "%lu: %.2f, %.2f", (unsigned long)index,
                actor->waypoints[index].x, actor->waypoints[index].z);
        if (nk_selectable_label(context, label, NK_TEXT_LEFT, &selected) &&
            selected)
            editor.selectedWaypoint = (int)index;
        }
    nk_layout_row_dynamic(context, 26.0f, 2);
    if (nk_button_label(context, "Add on map"))
        editorTool = EDITOR_TOOL_WAYPOINT;
    if (nk_button_label(context, "Delete") &&
        editor.selectedWaypoint >= 0)
        mapEditorDeleteWaypoint(&editor, (size_t)editor.selectedActor,
                                (size_t)editor.selectedWaypoint);
    if (editor.selectedWaypoint >= 0 &&
        (size_t)editor.selectedWaypoint < actor->waypointCount)
        {
        float oldX;
        float oldZ;
        struct EntityWaypoint *waypoint;
        waypoint = &actor->waypoints[editor.selectedWaypoint];
        oldX = waypoint->x;
        oldZ = waypoint->z;
        nk_layout_row_dynamic(context, 24.0f, 1);
        nk_property_float(context, "x", -100000.0f,
                          &waypoint->x, 100000.0f, editor.gridSize, 0.1f);
        nk_property_float(context, "z", -100000.0f,
                          &waypoint->z, 100000.0f, editor.gridSize, 0.1f);
        if (oldX != waypoint->x || oldZ != waypoint->z)
            editor.dirty = 1;
        }
    nk_tree_pop(context);
    }

static void triggerProperties(struct nk_context *context,
                              struct BattalionMapActor *actor)
    {
    char label[160];
    const char *types[2];
    size_t index;

    if (!nk_tree_push(context, NK_TREE_TAB, "Trigger routes", NK_MAXIMIZED))
        return;
    nk_layout_row_dynamic(context, 22.0f, 1);
    for (index = 0; index < actor->triggerCount; index++)
        {
        nk_bool selected;
        selected = editor.selectedTrigger == (int)index;
        sprintf(label, "%s -> %s%s",
                actor->triggers[index].event,
                actor->triggers[index].targetType ==
                    BATTALION_MAP_TRIGGER_GROUP ? "group:" : "id:",
                actor->triggers[index].target);
        if (nk_selectable_label(context, label, NK_TEXT_LEFT, &selected) &&
            selected)
            editor.selectedTrigger = (int)index;
        }
    nk_layout_row_dynamic(context, 26.0f, 2);
    if (nk_button_label(context, "Add route"))
        mapEditorAddTrigger(&editor, (size_t)editor.selectedActor,
                            "event", BATTALION_MAP_TRIGGER_ID, actor->id);
    if (nk_button_label(context, "Delete") &&
        editor.selectedTrigger >= 0)
        mapEditorDeleteTrigger(&editor, (size_t)editor.selectedActor,
                               (size_t)editor.selectedTrigger);
    if (editor.selectedTrigger >= 0 &&
        (size_t)editor.selectedTrigger < actor->triggerCount)
        {
        int selectedType;
        struct BattalionMapTrigger *trigger;
        trigger = &actor->triggers[editor.selectedTrigger];
        nk_layout_row_dynamic(context, 18.0f, 1);
        nk_label(context, "event", NK_TEXT_LEFT);
        nk_layout_row_dynamic(context, 24.0f, 1);
        if (editText(context, trigger->event, sizeof(trigger->event)))
            editor.dirty = 1;
        types[0] = "target-id";
        types[1] = "target-group";
        selectedType = trigger->targetType == BATTALION_MAP_TRIGGER_GROUP ?
                       1 : 0;
        selectedType = nk_combo(context, types, 2, selectedType,
                                24, nk_vec2(260.0f, 100.0f));
        if (trigger->targetType != (selectedType == 1 ?
                                   BATTALION_MAP_TRIGGER_GROUP :
                                   BATTALION_MAP_TRIGGER_ID))
            {
            trigger->targetType = selectedType == 1 ?
                                  BATTALION_MAP_TRIGGER_GROUP :
                                  BATTALION_MAP_TRIGGER_ID;
            editor.dirty = 1;
            }
        nk_layout_row_dynamic(context, 18.0f, 1);
        nk_label(context, types[selectedType], NK_TEXT_LEFT);
        nk_layout_row_dynamic(context, 24.0f, 1);
        if (editText(context, trigger->target, sizeof(trigger->target)))
            editor.dirty = 1;
        }
    nk_tree_pop(context);
    }

static void selectedActorProperties(struct nk_context *context)
    {
    struct BattalionMapActor *actor;
    float oldValue;
    int oldActive;
    int mode;
    const char *modes[3];

    if (editor.selectedActor < 0 ||
        (size_t)editor.selectedActor >= editor.map.actorCount)
        {
        nk_layout_row_dynamic(context, 24.0f, 1);
        nk_label(context, "No object selected", NK_TEXT_LEFT);
        return;
        }
    actor = &editor.map.actors[editor.selectedActor];
    nk_layout_row_dynamic(context, 18.0f, 1);
    nk_label(context, "id", NK_TEXT_LEFT);
    nk_layout_row_dynamic(context, 24.0f, 1);
    {
    char editedId[ENTITY_NAME_SIZE];
    strcpy(editedId, actor->id);
    if (editText(context, editedId, sizeof(editedId)))
        mapEditorSetActorId(&editor, (size_t)editor.selectedActor, editedId);
    }
    nk_layout_row_dynamic(context, 18.0f, 1);
    nk_label(context, "archetype", NK_TEXT_LEFT);
    nk_layout_row_dynamic(context, 24.0f, 1);
    archetypeCombo(context, actor);
    nk_layout_row_dynamic(context, 18.0f, 1);
    nk_label(context, "group", NK_TEXT_LEFT);
    nk_layout_row_dynamic(context, 24.0f, 1);
    if (editText(context, actor->group, sizeof(actor->group)))
        editor.dirty = 1;
    oldValue = actor->x;
    nk_property_float(context, "x", -100000.0f, &actor->x,
                      100000.0f, editor.gridSize, 0.1f);
    if (oldValue != actor->x) editor.dirty = 1;
    oldValue = actor->z;
    nk_property_float(context, "z", -100000.0f, &actor->z,
                      100000.0f, editor.gridSize, 0.1f);
    if (oldValue != actor->z) editor.dirty = 1;
    oldValue = actor->heading;
    nk_property_float(context, "heading", -6.2831853f, &actor->heading,
                      6.2831853f, 0.0872665f, 0.01f);
    if (oldValue != actor->heading) editor.dirty = 1;
    nk_layout_row_dynamic(context, 24.0f, 1);
    oldActive = actor->active;
    nk_checkbox_label(context, "active", (nk_bool *)&actor->active);
    if (oldActive != actor->active) editor.dirty = 1;
    modes[0] = "loop"; modes[1] = "pingpong"; modes[2] = "once";
    mode = actor->patrolMode == ENTITY_PATROL_PINGPONG ? 1 :
           actor->patrolMode == ENTITY_PATROL_ONCE ? 2 : 0;
    mode = nk_combo(context, modes, 3, mode, 24, nk_vec2(260.0f, 110.0f));
    if (actor->patrolMode != (mode == 1 ? ENTITY_PATROL_PINGPONG :
                              mode == 2 ? ENTITY_PATROL_ONCE :
                              ENTITY_PATROL_LOOP))
        {
        actor->patrolMode = mode == 1 ? ENTITY_PATROL_PINGPONG :
                            mode == 2 ? ENTITY_PATROL_ONCE :
                            ENTITY_PATROL_LOOP;
        editor.dirty = 1;
        }
    nk_layout_row_dynamic(context, 28.0f, 1);
    if (nk_button_label(context, "Delete actor"))
        mapEditorDeleteActor(&editor, (size_t)editor.selectedActor);
    if (editor.selectedActor >= 0)
        {
        actor = &editor.map.actors[editor.selectedActor];
        waypointProperties(context, actor);
        triggerProperties(context, actor);
        }
    }

static int structureShapeMaximum(int type)
    {
    if (type == 0) return 1;
    if (type == 1) return 37;
    if (type == 2) return 4;
    return 13;
    }

static const char *structureTypeName(int type)
    {
    if (type == 0) return "Tree";
    if (type == 1) return "Building";
    if (type == 2) return "Lake";
    if (type == 3) return "Hill";
    return "Structure";
    }

static void selectedLegacyProperties(struct nk_context *context)
    {
    float oldX;
    float oldZ;
    int oldType;
    int oldShape;
    int index;

    index = editor.selectedLegacyIndex;
    if (editor.selectedLegacyKind == MAP_EDITOR_LEGACY_ROAD &&
        index >= 0 && (size_t)index < editor.map.roadCount)
        {
        struct BattalionMapRoad *road;

        road = &editor.map.roads[index];
        nk_layout_row_dynamic(context, 22.0f, 1);
        nk_label(context, "Road", NK_TEXT_LEFT);
        oldX = road->x;
        oldZ = road->z;
        oldType = road->type;
        nk_property_float(context, "x", -100000.0f, &road->x,
                          100000.0f, editor.gridSize, 0.1f);
        nk_property_float(context, "z", -100000.0f, &road->z,
                          100000.0f, editor.gridSize, 0.1f);
        nk_property_int(context, "type", 0, &road->type, 3, 1, 1.0f);
        if (oldX != road->x || oldZ != road->z || oldType != road->type)
            editor.dirty = 1;
        nk_layout_row_dynamic(context, 26.0f, 1);
        if (nk_button_label(context, "Delete road"))
            mapEditorDeleteRoad(&editor, (size_t)index);
        }
    else if (editor.selectedLegacyKind == MAP_EDITOR_LEGACY_STRUCTURE &&
             index >= 0 && (size_t)index < editor.map.structureCount)
        {
        struct BattalionMapStructure *structure;

        structure = &editor.map.structures[index];
        nk_layout_row_dynamic(context, 22.0f, 1);
        nk_label(context, structureTypeName(structure->type), NK_TEXT_LEFT);
        oldX = structure->x;
        oldZ = structure->z;
        oldType = structure->type;
        oldShape = structure->shape;
        nk_property_float(context, "x", -100000.0f, &structure->x,
                          100000.0f, editor.gridSize, 0.1f);
        nk_property_float(context, "z", -100000.0f, &structure->z,
                          100000.0f, editor.gridSize, 0.1f);
        nk_property_int(context, "type", 0, &structure->type,
                        3, 1, 1.0f);
        if (structure->shape > structureShapeMaximum(structure->type))
            structure->shape = structureShapeMaximum(structure->type);
        nk_property_int(context, "shape", 0, &structure->shape,
                        structureShapeMaximum(structure->type), 1, 1.0f);
        if (oldX != structure->x || oldZ != structure->z ||
            oldType != structure->type || oldShape != structure->shape)
            editor.dirty = 1;
        nk_layout_row_dynamic(context, 26.0f, 1);
        if (nk_button_label(context, "Delete structure"))
            mapEditorDeleteStructure(&editor, (size_t)index);
        }
    }

static void drawRightPanel(struct nk_context *context)
    {
    float x;
    float height;

    x = (float)windowWidth - EDITOR_RIGHT_WIDTH;
    height = (float)windowHeight - EDITOR_TOP_HEIGHT;
    if (height < 1.0f) height = 1.0f;
    if (nk_begin(context, "Properties",
                 nk_rect(x, EDITOR_TOP_HEIGHT, EDITOR_RIGHT_WIDTH, height),
                 NK_WINDOW_BORDER | NK_WINDOW_TITLE))
        {
        nk_layout_row_dynamic(context, 18.0f, 1);
        nk_label(context, "map id", NK_TEXT_LEFT);
        nk_layout_row_dynamic(context, 24.0f, 1);
        if (editText(context, editor.map.id, sizeof(editor.map.id)))
            editor.dirty = 1;
        nk_layout_row_dynamic(context, 18.0f, 1);
        nk_label(context, "map name", NK_TEXT_LEFT);
        nk_layout_row_dynamic(context, 24.0f, 1);
        if (editText(context, editor.map.name, sizeof(editor.map.name)))
            editor.dirty = 1;
        nk_layout_row_dynamic(context, 18.0f, 1);
        nk_label(context, editor.path[0] ? editor.path : "not saved",
                 NK_TEXT_LEFT);
        nk_layout_row_dynamic(context, 24.0f, 1);
        {
        float oldGrid;
        oldGrid = editor.gridSize;
        nk_property_float(context, "grid", 0.05f, &editor.gridSize,
                          100.0f, 0.05f, 0.01f);
        if (editor.gridSize < 0.05f) editor.gridSize = 0.05f;
        if (oldGrid != editor.gridSize) copyStatus("Grid spacing changed");
        }
        nk_layout_row_dynamic(context, 8.0f, 1);
        nk_spacing(context, 1);
        if (editor.selectedLegacyKind != MAP_EDITOR_LEGACY_NONE)
            selectedLegacyProperties(context);
        else
            selectedActorProperties(context);
        }
    nk_end(context);
    }

static void worldPoint(float x, float z, float *screenX, float *screenY)
    {
    mapEditorWorldToScreen(&editor,
                           viewportRect.x, viewportRect.y,
                           viewportRect.w, viewportRect.h,
                           x, z, screenX, screenY);
    }

static int pointInsideViewport(float x, float y, float margin)
    {
    return x >= viewportRect.x - margin &&
           y >= viewportRect.y - margin &&
           x <= viewportRect.x + viewportRect.w + margin &&
           y <= viewportRect.y + viewportRect.h + margin;
    }

static struct nk_color actorColor(const struct BattalionMapActor *actor)
    {
    const struct EntityArchetype *archetype;

    archetype = entityCatalogFind(&editor.catalog, actor->archetype);
    if (archetype == NULL) return nk_rgb(220, 60, 80);
    if (archetype->kind == ENTITY_KIND_VEHICLE) return nk_rgb(70, 190, 110);
    if (archetype->kind == ENTITY_KIND_CREATURE) return nk_rgb(210, 90, 190);
    if (archetype->kind == ENTITY_KIND_NPC) return nk_rgb(80, 170, 230);
    if (archetype->kind == ENTITY_KIND_STRUCTURE) return nk_rgb(220, 150, 60);
    return nk_rgb(170, 170, 190);
    }

static void drawGrid(struct nk_command_buffer *canvas)
    {
    float minimumX;
    float minimumZ;
    float maximumX;
    float maximumZ;
    float step;
    float value;
    float screenX;
    float screenY;

    if (!editor.gridVisible || editor.gridSize <= 0.0f)
        return;
    mapEditorScreenToWorld(&editor, viewportRect.x, viewportRect.y,
                           viewportRect.w, viewportRect.h,
                           viewportRect.x, viewportRect.y + viewportRect.h,
                           &minimumX, &minimumZ);
    mapEditorScreenToWorld(&editor, viewportRect.x, viewportRect.y,
                           viewportRect.w, viewportRect.h,
                           viewportRect.x + viewportRect.w, viewportRect.y,
                           &maximumX, &maximumZ);
    step = editor.gridSize;
    while (step * editor.zoom < 8.0f)
        step *= 2.0f;
    value = (float)floor((double)(minimumX / step)) * step;
    while (value <= maximumX)
        {
        worldPoint(value, 0.0f, &screenX, &screenY);
        nk_stroke_line(canvas, screenX, viewportRect.y,
                       screenX, viewportRect.y + viewportRect.h,
                       1.0f, nk_rgb(48, 55, 61));
        value += step;
        }
    value = (float)floor((double)(minimumZ / step)) * step;
    while (value <= maximumZ)
        {
        worldPoint(0.0f, value, &screenX, &screenY);
        nk_stroke_line(canvas, viewportRect.x, screenY,
                       viewportRect.x + viewportRect.w, screenY,
                       1.0f, nk_rgb(48, 55, 61));
        value += step;
        }
    worldPoint(0.0f, 0.0f, &screenX, &screenY);
    nk_stroke_line(canvas, screenX, viewportRect.y,
                   screenX, viewportRect.y + viewportRect.h,
                   1.0f, nk_rgb(105, 75, 75));
    nk_stroke_line(canvas, viewportRect.x, screenY,
                   viewportRect.x + viewportRect.w, screenY,
                   1.0f, nk_rgb(75, 90, 110));
    }

static void drawLegacyMapObjects(struct nk_command_buffer *canvas)
    {
    float x;
    float y;
    float size;
    size_t index;

    size = editor.zoom * 0.42f;
    if (size < 2.0f) size = 2.0f;
    if (size > 10.0f) size = 10.0f;
    for (index = 0; index < editor.map.roadCount; index++)
        {
        float roadWidth;
        float roadHeight;

        worldPoint(editor.map.roads[index].x, editor.map.roads[index].z,
                   &x, &y);
        if (pointInsideViewport(x, y, size))
            {
            roadWidth = editor.map.roads[index].type == 0 ? size :
                        size * 2.0f;
            roadHeight = editor.map.roads[index].type == 1 ? size :
                         size * 2.0f;
            nk_fill_rect(canvas,
                         nk_rect(x - roadWidth, y - roadHeight,
                                 roadWidth * 2.0f, roadHeight * 2.0f),
                         0.0f, nk_rgb(92, 98, 101));
            if (editor.selectedLegacyKind == MAP_EDITOR_LEGACY_ROAD &&
                editor.selectedLegacyIndex == (int)index)
                nk_stroke_rect(canvas,
                               nk_rect(x - size - 3.0f, y - size - 3.0f,
                                       size * 2.0f + 6.0f,
                                       size * 2.0f + 6.0f),
                               0.0f, 2.0f, nk_rgb(75, 210, 235));
            }
        }
    for (index = 0; index < editor.map.structureCount; index++)
        {
        worldPoint(editor.map.structures[index].x,
                   editor.map.structures[index].z, &x, &y);
        if (pointInsideViewport(x, y, size))
            {
            float objectSize;
            struct nk_color color;

            objectSize = size * 1.6f;
            color = editor.map.structures[index].type == 0 ?
                    nk_rgb(70, 145, 80) :
                    editor.map.structures[index].type == 2 ?
                    nk_rgb(55, 125, 190) :
                    editor.map.structures[index].type == 3 ?
                    nk_rgb(120, 110, 65) : nk_rgb(173, 132, 76);
            nk_fill_rect(canvas,
                         nk_rect(x - objectSize, y - objectSize,
                                 objectSize * 2.0f, objectSize * 2.0f),
                         1.0f, color);
            if (editor.selectedLegacyKind == MAP_EDITOR_LEGACY_STRUCTURE &&
                editor.selectedLegacyIndex == (int)index)
                nk_stroke_rect(canvas,
                               nk_rect(x - objectSize - 3.0f,
                                       y - objectSize - 3.0f,
                                       objectSize * 2.0f + 6.0f,
                                       objectSize * 2.0f + 6.0f),
                               0.0f, 2.0f, nk_rgb(75, 210, 235));
            }
        }
    }

static void drawActors(struct nk_command_buffer *canvas)
    {
    float actorX;
    float actorY;
    float previousX;
    float previousY;
    float waypointX;
    float waypointY;
    float radius;
    size_t actorIndex;
    size_t waypointIndex;

    radius = editor.zoom * 0.55f;
    if (radius < 6.0f) radius = 6.0f;
    if (radius > 16.0f) radius = 16.0f;
    for (actorIndex = 0; actorIndex < editor.map.actorCount; actorIndex++)
        {
        const struct BattalionMapActor *actor;
        actor = &editor.map.actors[actorIndex];
        worldPoint(actor->x, actor->z, &actorX, &actorY);
        if (!pointInsideViewport(actorX, actorY, radius))
            continue;
        if ((int)actorIndex == editor.selectedActor)
            {
            previousX = actorX;
            previousY = actorY;
            for (waypointIndex = 0;
                 waypointIndex < actor->waypointCount; waypointIndex++)
                {
                worldPoint(actor->waypoints[waypointIndex].x,
                           actor->waypoints[waypointIndex].z,
                           &waypointX, &waypointY);
                nk_stroke_line(canvas, previousX, previousY,
                               waypointX, waypointY, 2.0f,
                               nk_rgb(235, 195, 78));
                nk_fill_circle(canvas,
                               nk_rect(waypointX - 5.0f, waypointY - 5.0f,
                                       10.0f, 10.0f),
                               waypointIndex ==
                                   (size_t)editor.selectedWaypoint ?
                                   nk_rgb(255, 240, 120) :
                                   nk_rgb(215, 175, 60));
                previousX = waypointX;
                previousY = waypointY;
                }
            }
        nk_fill_circle(canvas,
                       nk_rect(actorX - radius, actorY - radius,
                               radius * 2.0f, radius * 2.0f),
                       actorColor(actor));
        if ((int)actorIndex == editor.selectedActor)
            nk_stroke_circle(canvas,
                             nk_rect(actorX - radius - 3.0f,
                                     actorY - radius - 3.0f,
                                     radius * 2.0f + 6.0f,
                                     radius * 2.0f + 6.0f),
                             3.0f, nk_rgb(255, 226, 92));
        nk_stroke_line(canvas, actorX, actorY,
                       actorX + (float)sin((double)actor->heading) *
                                    radius * 1.7f,
                       actorY - (float)cos((double)actor->heading) *
                                    radius * 1.7f,
                       2.0f, nk_rgb(245, 245, 245));
        nk_draw_text(canvas,
                     nk_rect(actorX + radius + 4.0f, actorY - 9.0f,
                             180.0f, 18.0f),
                     actor->id, (int)strlen(actor->id),
                     &gui.font, nk_rgba(0, 0, 0, 0),
                     actor->active ? nk_rgb(230, 233, 235) :
                                     nk_rgb(125, 128, 132));
        }
    }

static int hitActor(float mouseX, float mouseY)
    {
    float x;
    float y;
    float deltaX;
    float deltaY;
    float bestDistance;
    float distance;
    int best;
    size_t index;

    best = -1;
    bestDistance = 18.0f * 18.0f;
    for (index = 0; index < editor.map.actorCount; index++)
        {
        worldPoint(editor.map.actors[index].x,
                   editor.map.actors[index].z, &x, &y);
        deltaX = x - mouseX;
        deltaY = y - mouseY;
        distance = deltaX * deltaX + deltaY * deltaY;
        if (distance < bestDistance)
            {
            best = (int)index;
            bestDistance = distance;
            }
        }
    return best;
    }

static int hitLegacyObject(float mouseX, float mouseY, int *kind)
    {
    float x;
    float y;
    float deltaX;
    float deltaY;
    float distance;
    float bestDistance;
    int best;
    size_t index;

    best = -1;
    *kind = MAP_EDITOR_LEGACY_NONE;
    bestDistance = 16.0f * 16.0f;
    for (index = 0; index < editor.map.roadCount; index++)
        {
        worldPoint(editor.map.roads[index].x, editor.map.roads[index].z,
                   &x, &y);
        deltaX = x - mouseX;
        deltaY = y - mouseY;
        distance = deltaX * deltaX + deltaY * deltaY;
        if (distance < bestDistance)
            {
            best = (int)index;
            *kind = MAP_EDITOR_LEGACY_ROAD;
            bestDistance = distance;
            }
        }
    for (index = 0; index < editor.map.structureCount; index++)
        {
        worldPoint(editor.map.structures[index].x,
                   editor.map.structures[index].z, &x, &y);
        deltaX = x - mouseX;
        deltaY = y - mouseY;
        distance = deltaX * deltaX + deltaY * deltaY;
        if (distance < bestDistance)
            {
            best = (int)index;
            *kind = MAP_EDITOR_LEGACY_STRUCTURE;
            bestDistance = distance;
            }
        }
    return best;
    }

static int hitWaypoint(float mouseX, float mouseY)
    {
    float x;
    float y;
    float deltaX;
    float deltaY;
    float distance;
    int best;
    size_t index;
    const struct BattalionMapActor *actor;

    if (editor.selectedActor < 0 ||
        (size_t)editor.selectedActor >= editor.map.actorCount)
        return -1;
    actor = &editor.map.actors[editor.selectedActor];
    best = -1;
    for (index = 0; index < actor->waypointCount; index++)
        {
        worldPoint(actor->waypoints[index].x, actor->waypoints[index].z,
                   &x, &y);
        deltaX = x - mouseX;
        deltaY = y - mouseY;
        distance = deltaX * deltaX + deltaY * deltaY;
        if (distance < 12.0f * 12.0f)
            best = (int)index;
        }
    return best;
    }

static void zoomAtMouse(float amount)
    {
    float beforeX;
    float beforeZ;
    float afterX;
    float afterZ;
    float oldZoom;

    if (amount == 0.0f)
        return;
    mapEditorScreenToWorld(&editor,
                           viewportRect.x, viewportRect.y,
                           viewportRect.w, viewportRect.h,
                           gui.context.input.mouse.pos.x,
                           gui.context.input.mouse.pos.y,
                           &beforeX, &beforeZ);
    oldZoom = editor.zoom;
    if (amount > 0.0f) editor.zoom *= 1.15f;
    else editor.zoom /= 1.15f;
    if (editor.zoom < 0.5f) editor.zoom = 0.5f;
    if (editor.zoom > 200.0f) editor.zoom = 200.0f;
    if (oldZoom == editor.zoom)
        return;
    mapEditorScreenToWorld(&editor,
                           viewportRect.x, viewportRect.y,
                           viewportRect.w, viewportRect.h,
                           gui.context.input.mouse.pos.x,
                           gui.context.input.mouse.pos.y,
                           &afterX, &afterZ);
    editor.cameraX += beforeX - afterX;
    editor.cameraZ += beforeZ - afterZ;
    }

static void interactViewport(void)
    {
    const struct nk_input *input;
    float worldX;
    float worldZ;
    int hovered;
    int hit;
    int hitKind;

    input = &gui.context.input;
    hovered = nk_input_is_mouse_hovering_rect(input, viewportRect);
    if (hovered && input->mouse.scroll_delta.y != 0.0f)
        zoomAtMouse(input->mouse.scroll_delta.y);
    if (hovered &&
        (nk_input_is_mouse_down(input, NK_BUTTON_RIGHT) ||
         nk_input_is_mouse_down(input, NK_BUTTON_MIDDLE)))
        {
        editor.cameraX -= input->mouse.delta.x / editor.zoom;
        editor.cameraZ += input->mouse.delta.y / editor.zoom;
        }
    if (hovered && viewportLeftPressed)
        {
        mapEditorScreenToWorld(&editor,
                               viewportRect.x, viewportRect.y,
                               viewportRect.w, viewportRect.h,
                               input->mouse.pos.x, input->mouse.pos.y,
                               &worldX, &worldZ);
        if (editorTool == EDITOR_TOOL_ACTOR &&
            editor.selectedArchetype >= 0)
            {
            mapEditorAddActor(&editor,
                              (size_t)editor.selectedArchetype,
                              worldX, worldZ);
            editorTool = EDITOR_TOOL_SELECT;
            }
        else if (editorTool == EDITOR_TOOL_WAYPOINT &&
                 editor.selectedActor >= 0)
            {
            mapEditorAddWaypoint(&editor,
                                 (size_t)editor.selectedActor,
                                 worldX, worldZ);
            editorTool = EDITOR_TOOL_SELECT;
            }
        else if (editorTool == EDITOR_TOOL_ROAD)
            mapEditorAddRoad(&editor, worldX, worldZ, placementRoadType);
        else if (editorTool == EDITOR_TOOL_STRUCTURE)
            mapEditorAddStructure(&editor, worldX, worldZ,
                                  placementStructureType,
                                  placementStructureShape);
        else
            {
            hit = hitWaypoint(input->mouse.pos.x, input->mouse.pos.y);
            if (hit >= 0)
                {
                editor.selectedWaypoint = hit;
                draggingWaypoint = hit;
                draggingActor = -1;
                }
            else
                {
                hit = hitActor(input->mouse.pos.x, input->mouse.pos.y);
                if (hit >= 0)
                    {
                    selectActor(hit);
                    draggingActor = hit;
                    }
                else
                    {
                    hit = hitLegacyObject(input->mouse.pos.x,
                                          input->mouse.pos.y, &hitKind);
                    if (hit >= 0)
                        {
                        selectLegacyObject(hitKind, hit);
                        draggingLegacyKind = hitKind;
                        draggingLegacyIndex = hit;
                        }
                    else
                        selectActor(-1);
                    }
                }
            }
        }
    if (nk_input_is_mouse_down(input, NK_BUTTON_LEFT))
        {
        mapEditorScreenToWorld(&editor,
                               viewportRect.x, viewportRect.y,
                               viewportRect.w, viewportRect.h,
                               input->mouse.pos.x, input->mouse.pos.y,
                               &worldX, &worldZ);
        if (draggingWaypoint >= 0 && editor.selectedActor >= 0)
            mapEditorMoveWaypoint(&editor,
                                  (size_t)editor.selectedActor,
                                  (size_t)draggingWaypoint,
                                  worldX, worldZ);
        else if (draggingActor >= 0)
            mapEditorMoveActor(&editor, (size_t)draggingActor,
                               worldX, worldZ);
        else if (draggingLegacyIndex >= 0 &&
                 draggingLegacyKind == MAP_EDITOR_LEGACY_ROAD)
            mapEditorMoveRoad(&editor, (size_t)draggingLegacyIndex,
                              worldX, worldZ);
        else if (draggingLegacyIndex >= 0 &&
                 draggingLegacyKind == MAP_EDITOR_LEGACY_STRUCTURE)
            mapEditorMoveStructure(&editor, (size_t)draggingLegacyIndex,
                                   worldX, worldZ);
        }
    else
        {
        draggingActor = -1;
        draggingWaypoint = -1;
        draggingLegacyKind = MAP_EDITOR_LEGACY_NONE;
        draggingLegacyIndex = -1;
        }
    }

static void drawViewport(struct nk_context *context)
    {
    float x;
    float width;
    float height;

    x = EDITOR_LEFT_WIDTH;
    width = (float)windowWidth - EDITOR_LEFT_WIDTH - EDITOR_RIGHT_WIDTH;
    height = (float)windowHeight - EDITOR_TOP_HEIGHT;
    if (width < EDITOR_MIN_VIEW) width = EDITOR_MIN_VIEW;
    if (height < 1.0f) height = 1.0f;
    if (nk_begin(context, "Map viewport",
                 nk_rect(x, EDITOR_TOP_HEIGHT, width, height),
                 NK_WINDOW_BORDER | NK_WINDOW_TITLE |
                 NK_WINDOW_NO_SCROLLBAR))
        {
        struct nk_command_buffer *canvas;
        canvas = nk_window_get_canvas(context);
        viewportRect = nk_window_get_content_region(context);
        nk_fill_rect(canvas, viewportRect, 0.0f, nk_rgb(31, 36, 41));
        interactViewport();
        drawGrid(canvas);
        drawLegacyMapObjects(canvas);
        drawActors(canvas);
        }
    nk_end(context);
    }

static void drawFileDialog(struct nk_context *context)
    {
    const char *title;

    if (fileDialogMode == EDITOR_FILE_NONE)
        return;
    title = fileDialogMode == EDITOR_FILE_OPEN ? "Open map.json" :
                                                "Save map.json";
    if (nk_begin(context, title,
                 nk_rect((float)windowWidth * 0.5f - 260.0f,
                         (float)windowHeight * 0.5f - 90.0f,
                         520.0f, 180.0f),
                 NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_MOVABLE))
        {
        nk_layout_row_dynamic(context, 20.0f, 1);
        nk_label(context, "Runtime path (assets/maps/<id>/map.json)",
                 NK_TEXT_LEFT);
        nk_layout_row_dynamic(context, 28.0f, 1);
        editText(context, filePath, sizeof(filePath));
        nk_layout_row_dynamic(context, 30.0f, 2);
        if (nk_button_label(context,
                            fileDialogMode == EDITOR_FILE_OPEN ?
                            "Open" : "Save"))
            {
            if (fileDialogMode == EDITOR_FILE_OPEN)
                {
                if (mapEditorLoad(&editor, filePath))
                    {
                    mapEditorFitContents(&editor,
                                         viewportRect.w, viewportRect.h);
                    fileDialogMode = EDITOR_FILE_NONE;
                    }
                }
            else if (mapEditorSave(&editor, filePath))
                fileDialogMode = EDITOR_FILE_NONE;
            }
        if (nk_button_label(context, "Cancel"))
            fileDialogMode = EDITOR_FILE_NONE;
        }
    nk_end(context);
    }

static void drawNewDialog(struct nk_context *context)
    {
    if (!newDialogVisible)
        return;
    if (nk_begin(context, "New Battalion map v2",
                 nk_rect((float)windowWidth * 0.5f - 230.0f,
                         (float)windowHeight * 0.5f - 120.0f,
                         460.0f, 240.0f),
                 NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_MOVABLE))
        {
        nk_layout_row_dynamic(context, 18.0f, 1);
        nk_label(context, "id", NK_TEXT_LEFT);
        nk_layout_row_dynamic(context, 26.0f, 1);
        editText(context, newMapId, sizeof(newMapId));
        nk_layout_row_dynamic(context, 18.0f, 1);
        nk_label(context, "name", NK_TEXT_LEFT);
        nk_layout_row_dynamic(context, 26.0f, 1);
        editText(context, newMapName, sizeof(newMapName));
        nk_layout_row_dynamic(context, 30.0f, 2);
        if (nk_button_label(context, "Create") &&
            mapEditorNew(&editor, newMapId, newMapName))
            {
            newDialogVisible = 0;
            filePath[0] = '\0';
            }
        if (nk_button_label(context, "Cancel"))
            newDialogVisible = 0;
        }
    nk_end(context);
    }

static void processDeleteRequest(void)
    {
    if (!deleteRequested)
        return;
    deleteRequested = 0;
    if (nk_item_is_any_active(&gui.context))
        return;
    if (editor.selectedActor >= 0 &&
        (size_t)editor.selectedActor < editor.map.actorCount)
        {
        if (editor.selectedWaypoint >= 0)
            mapEditorDeleteWaypoint(&editor,
                                    (size_t)editor.selectedActor,
                                    (size_t)editor.selectedWaypoint);
        else
            mapEditorDeleteActor(&editor, (size_t)editor.selectedActor);
        }
    else if (editor.selectedLegacyIndex >= 0 &&
             editor.selectedLegacyKind == MAP_EDITOR_LEGACY_ROAD)
        mapEditorDeleteRoad(&editor, (size_t)editor.selectedLegacyIndex);
    else if (editor.selectedLegacyIndex >= 0 &&
             editor.selectedLegacyKind == MAP_EDITOR_LEGACY_STRUCTURE)
        mapEditorDeleteStructure(&editor,
                                 (size_t)editor.selectedLegacyIndex);
    }

static void display(void)
    {
    float clearColor[4];

    mapEditorGuiInput(&gui, &guiInput);
    guiInput.wheel = 0.0f;
    guiInput.textLength = 0;
    clearColor[0] = 0.08f;
    clearColor[1] = 0.09f;
    clearColor[2] = 0.10f;
    clearColor[3] = 1.0f;
    glClearColor(clearColor[0], clearColor[1],
                 clearColor[2], clearColor[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawTopBar(&gui.context);
    drawLeftPanel(&gui.context);
    drawRightPanel(&gui.context);
    drawViewport(&gui.context);
    viewportLeftPressed = 0;
    drawFileDialog(&gui.context);
    drawNewDialog(&gui.context);
    processDeleteRequest();
    mapEditorGuiRender(&gui, windowWidth, windowHeight);
    tkSwapBuffers();
    }

static void reshape(int width, int height)
    {
    windowWidth = width;
    windowHeight = height;
    }

static GLenum keyDown(int key, GLenum modifiers)
    {
    guiInput.shift = (modifiers & TK_SHIFT) != 0;
    guiInput.control = (modifiers & TK_CONTROL) != 0;
    if (key == TK_LEFT) guiInput.left = 1;
    else if (key == TK_RIGHT) guiInput.right = 1;
    else if (key == TK_BACKSPACE) guiInput.backspace = 1;
    else if (key == TK_DELETE)
        {
        guiInput.deleteKey = 1;
        deleteRequested = 1;
        }
    else if (key == TK_RETURN) guiInput.enter = 1;
    else if (key == TK_TAB) guiInput.tab = 1;
    else if (key == TK_HOME) guiInput.home = 1;
    else if (key == TK_END) guiInput.end = 1;
    else if ((key == TK_s || key == TK_S) && guiInput.control)
        saveCurrentMap();
    else if ((key == TK_o || key == TK_O) && guiInput.control)
        fileDialogMode = EDITOR_FILE_OPEN;
    else if ((key == TK_n || key == TK_N) && guiInput.control)
        newDialogVisible = 1;
    else if (key == TK_ESCAPE)
        {
        editorTool = EDITOR_TOOL_SELECT;
        fileDialogMode = EDITOR_FILE_NONE;
        newDialogVisible = 0;
        }
    else if (key >= 32 && key < 127 && !guiInput.control &&
             guiInput.textLength < MAP_EDITOR_GUI_TEXT_CAPACITY)
        guiInput.text[guiInput.textLength++] = (unsigned int)key;
    return GL_TRUE;
    }

static GLenum keyUp(int key)
    {
    if (key == TK_LEFT) guiInput.left = 0;
    else if (key == TK_RIGHT) guiInput.right = 0;
    else if (key == TK_BACKSPACE) guiInput.backspace = 0;
    else if (key == TK_DELETE) guiInput.deleteKey = 0;
    else if (key == TK_RETURN) guiInput.enter = 0;
    else if (key == TK_TAB) guiInput.tab = 0;
    else if (key == TK_HOME) guiInput.home = 0;
    else if (key == TK_END) guiInput.end = 0;
    else if (key == TK_CONTROL_L || key == TK_CONTROL_R)
        guiInput.control = 0;
    else if (key == TK_SHIFT_L || key == TK_SHIFT_R)
        guiInput.shift = 0;
    return GL_TRUE;
    }

static GLenum mouseDown(int x, int y, GLenum button)
    {
    guiInput.mouseX = x;
    guiInput.mouseY = y;
    if (button & TK_WHEELUP) guiInput.wheel += 1.0f;
    if (button & TK_WHEELDOWN) guiInput.wheel -= 1.0f;
    if (button & TK_LEFTBUTTON)
        {
        guiInput.mouseLeft = 1;
        viewportLeftPressed = 1;
        }
    if (button & TK_RIGHTBUTTON) guiInput.mouseRight = 1;
    if (button & TK_MIDDLEBUTTON) guiInput.mouseMiddle = 1;
    return GL_TRUE;
    }

static GLenum mouseUp(int x, int y, GLenum button)
    {
    guiInput.mouseX = x;
    guiInput.mouseY = y;
    if (button & TK_LEFTBUTTON) guiInput.mouseLeft = 0;
    if (button & TK_RIGHTBUTTON) guiInput.mouseRight = 0;
    if (button & TK_MIDDLEBUTTON) guiInput.mouseMiddle = 0;
    return GL_TRUE;
    }

static GLenum mouseMove(int x, int y, GLenum button)
    {
    (void)button;
    guiInput.mouseX = x;
    guiInput.mouseY = y;
    return GL_TRUE;
    }

static int validateCommand(const char *assetRoot, const char *path)
    {
    struct MapEditorModel validationEditor;
    int result;

    mapEditorModelInitialize(&validationEditor);
    result = mapEditorLoadCatalog(&validationEditor, assetRoot,
                                  "entities/catalog.txt") &&
             mapEditorLoad(&validationEditor, path) &&
             mapEditorValidate(&validationEditor);
    if (result)
        printf("%s: valid battalion map\n", path);
    else
        fprintf(stderr, "%s: %s\n", path, validationEditor.status);
    mapEditorModelDestroy(&validationEditor);
    return result ? 0 : 1;
    }

static void usage(const char *program)
    {
    fprintf(stderr,
            "Usage: %s [--assets <path>] [map.json]\n"
            "       %s [--assets <path>] --validate <map.json>\n",
            program, program);
    }

int main(int argc, char **argv)
    {
    const char *assetRoot;
    const char *mapPath;
    const char *validationPath;
    int index;

    assetRoot = "./assets";
    mapPath = NULL;
    validationPath = NULL;
    for (index = 1; index < argc; index++)
        {
        if (strcmp(argv[index], "--assets") == 0 && index + 1 < argc)
            assetRoot = argv[++index];
        else if (strcmp(argv[index], "--validate") == 0 && index + 1 < argc)
            validationPath = argv[++index];
        else if (argv[index][0] != '-' && mapPath == NULL)
            mapPath = argv[index];
        else
            {
            usage(argv[0]);
            return 2;
            }
        }
    if (validationPath != NULL)
        return validateCommand(assetRoot, validationPath);

    mapEditorModelInitialize(&editor);
    if (!mapEditorLoadCatalog(&editor, assetRoot, "entities/catalog.txt"))
        {
        fprintf(stderr, "map_editor: %s\n", editor.status);
        mapEditorModelDestroy(&editor);
        return 1;
        }
    if (mapPath != NULL)
        {
        if (!mapEditorLoad(&editor, mapPath))
            {
            fprintf(stderr, "map_editor: %s\n", editor.status);
            mapEditorModelDestroy(&editor);
            return 1;
            }
        strncpy(filePath, mapPath, sizeof(filePath) - 1);
        filePath[sizeof(filePath) - 1] = '\0';
        mapEditorFitContents(&editor, 700.0f, 650.0f);
        }
    else
        mapEditorNew(&editor, newMapId, newMapName);
    if (!mapEditorGuiInitialize(&gui))
        {
        fprintf(stderr, "map_editor: could not initialize Nuklear\n");
        mapEditorModelDestroy(&editor);
        return 1;
        }

    memset(&guiInput, 0, sizeof(guiInput));
    viewportRect = nk_rect(EDITOR_LEFT_WIDTH, EDITOR_TOP_HEIGHT,
                           700.0f, 650.0f);
    tkInitDisplayMode(TK_RGB | TK_DOUBLE | TK_DEPTH | TK_ALPHA);
    tkInitPosition(80, 80, windowWidth, windowHeight);
    tkReshapeFunc(reshape);
    tkDisplayFunc(display);
    tkIdleFunc(display);
    tkKeyDownFunc(keyDown);
    tkKeyUpFunc(keyUp);
    tkMouseDownFunc(mouseDown);
    tkMouseUpFunc(mouseUp);
    tkMouseMoveFunc(mouseMove);
    if (tkInitWindow("Battalion Map Editor") == GL_FALSE)
        {
        fprintf(stderr, "map_editor: could not create window\n");
        mapEditorGuiDestroy(&gui);
        mapEditorModelDestroy(&editor);
        return 1;
        }
    tkExec();
    mapEditorGuiDestroy(&gui);
    mapEditorModelDestroy(&editor);
    return 0;
    }
