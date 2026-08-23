/*
 * Copyright (c) 2026 Battalion contributors.
 */
/* Nuklear editor for the canonical Battalion map v2 object model. */

#include "map_editor_gui.h"
#include "map_editor_model.h"
#include "asset_animation.h"
#include "asset_manifest.h"
#include "asset_render.h"
#include "asset_values.h"
#include "tk.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EDITOR_LEFT_WIDTH 250.0f
#define EDITOR_RIGHT_WIDTH 330.0f
#define EDITOR_TOP_HEIGHT 32.0f
#define EDITOR_MIN_VIEW 120.0f
#define EDITOR_HIT_RADIUS 10.0f
#define EDITOR_PREVIEW_MAX 128

#define EDITOR_TOOL_SELECT 0
#define EDITOR_TOOL_PLACE 1
#define EDITOR_TOOL_WAYPOINT 2
#define EDITOR_TOOL_GEOMETRY 3

#define EDITOR_FILE_NONE 0
#define EDITOR_FILE_OPEN 1
#define EDITOR_FILE_SAVE 2

int paused;

static struct MapEditorModel editor;
static struct MapEditorGui gui;
static struct AssetValues editorPalette;
static struct AssetManifest editorAssets;
static struct MapEditorGuiInput guiInput;
static int windowWidth = 1280;
static int windowHeight = 800;
static int editorTool;
static int draggingObject = -1;
static int draggingWaypoint = -1;
static int draggingGeometryPoint = -1;
static int viewportLeftPressed;
static int deleteRequested;
static int fileDialogMode;
static int newDialogVisible;
static char filePath[MAP_EDITOR_PATH_SIZE];
static char newMapId[32] = "untitled";
static char newMapName[128] = "Untitled";
static struct nk_rect viewportRect;
static float previewAngle;

struct EditorPreviewRequest
    {
    struct nk_rect bounds;
    const struct EntityArchetype *archetype;
    int large;
    int selected;
    };

static struct EditorPreviewRequest previewRequests[EDITOR_PREVIEW_MAX];
static size_t previewRequestCount;

static void copyStatus(const char *message)
    {
    strncpy(editor.status, message, sizeof(editor.status) - 1);
    editor.status[sizeof(editor.status) - 1] = '\0';
    }

static int editText(struct nk_context *context, char *value, int capacity)
    {
    char before[MAP_EDITOR_PATH_SIZE];

    if (capacity > (int)sizeof(before)) capacity = (int)sizeof(before);
    strncpy(before, value, (size_t)capacity);
    before[capacity - 1] = '\0';
    nk_edit_string_zero_terminated(context,
        NK_EDIT_FIELD | NK_EDIT_SIG_ENTER, value, capacity, nk_filter_ascii);
    return strcmp(before, value) != 0;
    }

static const char *archetypeLabel(const struct EntityArchetype *archetype)
    {
    return archetype->displayName[0] ? archetype->displayName : archetype->id;
    }

static const struct EntityArchetype *selectedObjectArchetype(void)
    {
    if (editor.selectedObject < 0 ||
        (size_t)editor.selectedObject >= editor.map.objectCount)
        return NULL;
    return entityCatalogFind(
        &editor.catalog, editor.map.objects[editor.selectedObject].archetype);
    }

static void selectObject(int index)
    {
    editor.selectedObject = index;
    editor.selectedWaypoint = -1;
    editor.selectedGeometryPoint = -1;
    editor.selectedTrigger = -1;
    editor.selectedProperty = -1;
    draggingObject = -1;
    draggingWaypoint = -1;
    draggingGeometryPoint = -1;
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
    copyStatus(result == 0 ? "make test passed" :
               "make test failed; see terminal output");
    }

static const char *toolLabel(void)
    {
    static char label[160];

    if (editorTool == EDITOR_TOOL_PLACE)
        {
        if (editor.selectedArchetype >= 0 &&
            (size_t)editor.selectedArchetype < editor.catalog.count)
            {
            sprintf(label, "PLACE: %s (click map; Esc cancels)",
                    archetypeLabel(
                        &editor.catalog.entries[editor.selectedArchetype]));
            return label;
            }
        return "PLACE: choose an archetype";
        }
    if (editorTool == EDITOR_TOOL_WAYPOINT)
        return "WAYPOINTS: click map; Esc finishes";
    if (editorTool == EDITOR_TOOL_GEOMETRY)
        return "GEOMETRY: click map; Esc finishes";
    return "Tool: select/move";
    }

static void beginArchetypePlacement(size_t archetypeIndex)
    {
    char message[192];

    if (archetypeIndex >= editor.catalog.count)
        return;
    if (!mapEditorSelectArchetypeForPlacement(&editor, archetypeIndex))
        return;
    draggingObject = -1;
    draggingWaypoint = -1;
    draggingGeometryPoint = -1;
    editorTool = EDITOR_TOOL_PLACE;
    sprintf(message, "Placing %s: click the map; Esc cancels",
            archetypeLabel(&editor.catalog.entries[archetypeIndex]));
    copyStatus(message);
    }

static int intersectPreviewBounds(struct nk_rect bounds, struct nk_rect clip,
                                  struct nk_rect *result)
    {
    float left;
    float top;
    float right;
    float bottom;

    left = bounds.x > clip.x ? bounds.x : clip.x;
    top = bounds.y > clip.y ? bounds.y : clip.y;
    right = bounds.x + bounds.w < clip.x + clip.w ?
            bounds.x + bounds.w : clip.x + clip.w;
    bottom = bounds.y + bounds.h < clip.y + clip.h ?
             bounds.y + bounds.h : clip.y + clip.h;
    if (right <= left || bottom <= top)
        return 0;
    result->x = left;
    result->y = top;
    result->w = right - left;
    result->h = bottom - top;
    return 1;
    }

static void queuePreview(struct nk_context *context, struct nk_rect bounds,
                         const struct EntityArchetype *archetype,
                         int large, int selected)
    {
    struct nk_command_buffer *canvas;
    struct nk_rect clipped;

    if (archetype == NULL || previewRequestCount >= EDITOR_PREVIEW_MAX)
        return;
    canvas = nk_window_get_canvas(context);
    if (!intersectPreviewBounds(bounds, canvas->clip, &clipped))
        return;
    previewRequests[previewRequestCount].bounds = clipped;
    previewRequests[previewRequestCount].archetype = archetype;
    previewRequests[previewRequestCount].large = large;
    previewRequests[previewRequestCount].selected = selected;
    previewRequestCount++;
    }

static void drawTopBar(struct nk_context *context)
    {
    if (nk_begin(context, "MainMenu",
                 nk_rect(0.0f, 0.0f, (float)windowWidth,
                         EDITOR_TOP_HEIGHT),
                 NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BACKGROUND))
        {
        nk_menubar_begin(context);
        nk_layout_row_begin(context, NK_STATIC, 24.0f, 6);
        nk_layout_row_push(context, 48.0f);
        if (nk_menu_begin_label(context, "File", NK_TEXT_LEFT,
                                nk_vec2(180.0f, 170.0f)))
            {
            nk_layout_row_dynamic(context, 22.0f, 1);
            if (nk_menu_item_label(context, "New v2", NK_TEXT_LEFT))
                newDialogVisible = 1;
            if (nk_menu_item_label(context, "Open...", NK_TEXT_LEFT))
                fileDialogMode = EDITOR_FILE_OPEN;
            if (nk_menu_item_label(context, "Save", NK_TEXT_LEFT))
                saveCurrentMap();
            if (nk_menu_item_label(context, "Save As...", NK_TEXT_LEFT))
                fileDialogMode = EDITOR_FILE_SAVE;
            if (nk_menu_item_label(context, "Quit", NK_TEXT_LEFT)) tkQuit();
            nk_menu_end(context);
            }
        nk_layout_row_push(context, 48.0f);
        if (nk_menu_begin_label(context, "Edit", NK_TEXT_LEFT,
                                nk_vec2(180.0f, 120.0f)))
            {
            nk_layout_row_dynamic(context, 22.0f, 1);
            if (nk_menu_item_label(context, "Select / move", NK_TEXT_LEFT))
                {
                editorTool = EDITOR_TOOL_SELECT;
                copyStatus("Select/move tool");
                }
            if (nk_menu_item_label(context, "Delete selection", NK_TEXT_LEFT))
                deleteRequested = 1;
            nk_menu_end(context);
            }
        nk_layout_row_push(context, 48.0f);
        if (nk_menu_begin_label(context, "View", NK_TEXT_LEFT,
                                nk_vec2(180.0f, 170.0f)))
            {
            nk_bool checked;
            nk_layout_row_dynamic(context, 22.0f, 1);
            checked = editor.gridVisible;
            if (nk_checkbox_label(context, "Grid", &checked))
                editor.gridVisible = checked;
            checked = editor.snapEnabled;
            if (nk_checkbox_label(context, "Snap", &checked))
                editor.snapEnabled = checked;
            if (nk_menu_item_label(context, "Fit contents", NK_TEXT_LEFT))
                mapEditorFitContents(&editor, viewportRect.w, viewportRect.h);
            if (nk_menu_item_label(context, "Zoom in", NK_TEXT_LEFT))
                editor.zoom *= 1.25f;
            if (nk_menu_item_label(context, "Zoom out", NK_TEXT_LEFT))
                editor.zoom /= 1.25f;
            nk_menu_end(context);
            }
        nk_layout_row_push(context, 72.0f);
        if (nk_menu_begin_label(context, "Validate", NK_TEXT_LEFT,
                                nk_vec2(190.0f, 80.0f)))
            {
            nk_layout_row_dynamic(context, 22.0f, 1);
            if (nk_menu_item_label(context, "Validate map", NK_TEXT_LEFT))
                mapEditorValidate(&editor);
            if (nk_menu_item_label(context, "Run make test", NK_TEXT_LEFT))
                runMakeTest();
            nk_menu_end(context);
            }
        nk_layout_row_push(context, 340.0f);
        nk_label(context, toolLabel(), NK_TEXT_LEFT);
        nk_layout_row_push(context,
            (float)windowWidth > 590.0f ? (float)windowWidth - 590.0f : 80.0f);
        nk_label(context, editor.status, NK_TEXT_RIGHT);
        nk_layout_row_end(context);
        nk_menubar_end(context);
        }
    nk_end(context);
    }

static void drawArchetypePalette(struct nk_context *context)
    {
    size_t index;
    char previousCategory[ENTITY_NAME_SIZE];

    previousCategory[0] = '\0';
    nk_layout_row_dynamic(context, 18.0f, 1);
    nk_label(context, "ARCHETYPES FROM CATALOG", NK_TEXT_LEFT);
    nk_layout_row_dynamic(context, 18.0f, 1);
    nk_label(context, "Choose one, then click the map", NK_TEXT_LEFT);
    nk_layout_row_dynamic(context, 24.0f, 1);
    if (nk_button_label(context, "Select / move (Esc)"))
        {
        editorTool = EDITOR_TOOL_SELECT;
        copyStatus("Select/move tool");
        }
    for (index = 0; index < editor.catalog.count; index++)
        {
        const struct EntityArchetype *entry;
        enum nk_widget_layout_states previewState;
        struct nk_command_buffer *canvas;
        struct nk_rect previewBounds;
        nk_bool selected;
        entry = &editor.catalog.entries[index];
        if (!entry->editorPlaceable || entry->placement == ENTITY_PLACEMENT_NONE)
            continue;
        if (strcmp(previousCategory, entry->editorCategory) != 0)
            {
            strncpy(previousCategory, entry->editorCategory,
                    sizeof(previousCategory) - 1);
            previousCategory[sizeof(previousCategory) - 1] = '\0';
            nk_layout_row_dynamic(context, 18.0f, 1);
            nk_label(context, previousCategory, NK_TEXT_LEFT);
            }
        selected = editorTool == EDITOR_TOOL_PLACE &&
                   editor.selectedArchetype == (int)index;
        nk_layout_row_begin(context, NK_STATIC, 54.0f, 2);
        nk_layout_row_push(context, 54.0f);
        previewState = nk_widget(&previewBounds, context);
        if (previewState != NK_WIDGET_INVALID)
            {
            canvas = nk_window_get_canvas(context);
            nk_fill_rect(canvas, previewBounds, 2.0f,
                         selected ? nk_rgb(53, 63, 72) :
                                    nk_rgb(24, 28, 32));
            nk_stroke_rect(canvas, previewBounds, 2.0f, 1.0f,
                           selected ? nk_rgb(255, 226, 90) :
                                      nk_rgb(65, 72, 78));
            queuePreview(context, previewBounds, entry, 0, selected);
            if (previewState == NK_WIDGET_VALID &&
                nk_input_is_mouse_click_in_rect(
                    &context->input, NK_BUTTON_LEFT, previewBounds))
                beginArchetypePlacement(index);
            }
        nk_layout_row_push(context, EDITOR_LEFT_WIDTH - 78.0f);
        if (nk_selectable_label(context, archetypeLabel(entry),
                                NK_TEXT_LEFT, &selected) && selected)
            beginArchetypePlacement(index);
        nk_layout_row_end(context);
        }
    }

static void drawObjectList(struct nk_context *context)
    {
    size_t index;
    char label[192];
    const struct BattalionMapObject *object;
    const struct EntityArchetype *archetype;
    nk_bool selected;

    nk_layout_row_dynamic(context, 18.0f, 1);
    nk_label(context, "MAP OBJECTS", NK_TEXT_LEFT);
    for (index = 0; index < editor.map.objectCount; index++)
        {
        object = &editor.map.objects[index];
        archetype = entityCatalogFind(&editor.catalog, object->archetype);
        sprintf(label, "%s  %s",
                object->id[0] ? object->id : "(no id)",
                archetype ? archetypeLabel(archetype) : object->archetype);
        selected = editor.selectedObject == (int)index;
        nk_layout_row_dynamic(context, 20.0f, 1);
        if (nk_selectable_label(context, label, NK_TEXT_LEFT, &selected) &&
            selected)
            {
            selectObject((int)index);
            editorTool = EDITOR_TOOL_SELECT;
            }
        }
    }

static void drawLeftPanel(struct nk_context *context)
    {
    float height;
    height = (float)windowHeight - EDITOR_TOP_HEIGHT;
    if (nk_begin(context, "Palette and objects",
                 nk_rect(0.0f, EDITOR_TOP_HEIGHT, EDITOR_LEFT_WIDTH, height),
                 NK_WINDOW_BORDER | NK_WINDOW_TITLE))
        {
        drawArchetypePalette(context);
        nk_layout_row_dynamic(context, 8.0f, 1);
        nk_spacing(context, 1);
        drawObjectList(context);
        }
    nk_end(context);
    }

static void editObjectIdentity(struct nk_context *context,
                               struct BattalionMapObject *object)
    {
    char editedId[ENTITY_NAME_SIZE];

    nk_layout_row_dynamic(context, 16.0f, 1);
    nk_label(context, "id", NK_TEXT_LEFT);
    strcpy(editedId, object->id);
    nk_layout_row_dynamic(context, 23.0f, 1);
    if (editText(context, editedId, sizeof(editedId)) &&
        !mapEditorSetObjectId(&editor, (size_t)editor.selectedObject,
                              editedId))
        copyStatus("Object id must be unique and valid");
    nk_layout_row_dynamic(context, 16.0f, 1);
    nk_label(context, "description", NK_TEXT_LEFT);
    nk_layout_row_dynamic(context, 23.0f, 1);
    if (editText(context, object->description, sizeof(object->description)))
        editor.dirty = 1;
    nk_layout_row_dynamic(context, 16.0f, 1);
    nk_label(context, "group", NK_TEXT_LEFT);
    nk_layout_row_dynamic(context, 23.0f, 1);
    if (editText(context, object->group, sizeof(object->group)))
        editor.dirty = 1;
    }

static void editObjectTransform(struct nk_context *context,
                                struct BattalionMapObject *object)
    {
    float oldX;
    float oldY;
    float oldZ;
    float oldHeading;
    float oldScale;
    nk_bool active;

    oldX = object->x; oldY = object->y; oldZ = object->z;
    oldHeading = object->heading; oldScale = object->scale;
    nk_layout_row_dynamic(context, 22.0f, 1);
    nk_property_float(context, "x", -100000.0f, &object->x, 100000.0f,
                      editor.gridSize, 0.1f);
    nk_property_float(context, "y", -100000.0f, &object->y, 100000.0f,
                      editor.gridSize, 0.1f);
    nk_property_float(context, "z", -100000.0f, &object->z, 100000.0f,
                      editor.gridSize, 0.1f);
    nk_property_float(context, "heading", -36000.0f, &object->heading,
                      36000.0f, 1.0f, 0.2f);
    nk_property_float(context, "scale", 0.001f, &object->scale,
                      1000.0f, 0.05f, 0.01f);
    if (oldX != object->x || oldY != object->y || oldZ != object->z ||
        oldHeading != object->heading || oldScale != object->scale)
        editor.dirty = 1;
    active = object->active;
    if (nk_checkbox_label(context, "active", &active))
        {
        object->active = active;
        editor.dirty = 1;
        }
    }

static void editFaction(struct nk_context *context,
                        struct BattalionMapObject *object,
                        const struct EntityArchetype *archetype)
    {
    const char *selected;
    size_t index;

    if (!entityArchetypeIsActor(archetype))
        return;
    selected = object->faction[0] != '\0' ? object->faction :
               archetype->faction;
    nk_layout_row_dynamic(context, 16.0f, 1);
    nk_label(context, "faction", NK_TEXT_LEFT);
    nk_layout_row_dynamic(context, 23.0f, 1);
    if (nk_combo_begin_label(context, selected, nk_vec2(220.0f, 180.0f)))
        {
        nk_layout_row_dynamic(context, 20.0f, 1);
        if (nk_combo_item_label(context, "inherit archetype", NK_TEXT_LEFT))
            {
            object->faction[0] = '\0';
            editor.dirty = 1;
            }
        for (index = 0; index < editor.factions.count; index++)
            if (nk_combo_item_label(context,
                    editor.factions.entries[index].id, NK_TEXT_LEFT))
                {
                strcpy(object->faction, editor.factions.entries[index].id);
                editor.dirty = 1;
                }
        nk_combo_end(context);
        }
    }

static void editObjectProperties(struct nk_context *context,
                                 struct BattalionMapObject *object,
                                 const struct EntityArchetype *archetype)
    {
    size_t index;
    char label[96];
    struct BattalionMapProperty *property;
    const struct EntityPropertyDefinition *definition;

    if (!nk_tree_push(context, NK_TREE_TAB, "Properties", NK_MAXIMIZED))
        return;
    if (object->propertyCount == 0)
        {
        nk_layout_row_dynamic(context, 18.0f, 1);
        nk_label(context, "No archetype properties", NK_TEXT_LEFT);
        }
    for (index = 0; index < object->propertyCount; index++)
        {
        property = &object->properties[index];
        definition = NULL;
        if (index < archetype->propertyCount &&
            strcmp(archetype->properties[index].name, property->name) == 0)
            definition = &archetype->properties[index];
        sprintf(label, "%s%s", property->name,
                definition ? "" : " (unknown)");
        nk_layout_row_dynamic(context, 16.0f, 1);
        nk_label(context, label, NK_TEXT_LEFT);
        nk_layout_row_dynamic(context, 23.0f, 1);
        if (editText(context, property->value, sizeof(property->value)))
            editor.dirty = 1;
        }
    nk_tree_pop(context);
    }

static void drawObjectAudio(struct nk_context *context,
                            const struct EntityArchetype *archetype)
    {
    char label[192];
    size_t index;

    if (archetype->fireSound[0] == '\0' &&
        archetype->destructionSound[0] == '\0' &&
        archetype->moveSound[0] == '\0' &&
        archetype->contactAudioTag[0] == '\0' &&
        archetype->surfaceSoundCount == 0)
        return;
    if (!nk_tree_push(context, NK_TREE_TAB, "Audio", NK_MINIMIZED))
        return;
    if (archetype->fireSound[0] != '\0')
        {
        sprintf(label, "fire: %s", archetype->fireSound);
        nk_layout_row_dynamic(context, 18.0f, 1);
        nk_label(context, label, NK_TEXT_LEFT);
        }
    if (archetype->destructionSound[0] != '\0')
        {
        sprintf(label, "destruction: %s", archetype->destructionSound);
        nk_layout_row_dynamic(context, 18.0f, 1);
        nk_label(context, label, NK_TEXT_LEFT);
        }
    if (archetype->moveSound[0] != '\0')
        {
        sprintf(label, "move: %s / %d frames", archetype->moveSound,
                archetype->moveSoundInterval);
        nk_layout_row_dynamic(context, 18.0f, 1);
        nk_label(context, label, NK_TEXT_LEFT);
        }
    if (archetype->contactAudioTag[0] != '\0')
        {
        sprintf(label, "contact tag: %s", archetype->contactAudioTag);
        nk_layout_row_dynamic(context, 18.0f, 1);
        nk_label(context, label, NK_TEXT_LEFT);
        }
    for (index = 0; index < archetype->surfaceSoundCount; index++)
        {
        sprintf(label, "%s -> %s / %d frames",
                archetype->surfaceSounds[index].contactTag,
                archetype->surfaceSounds[index].sound,
                archetype->surfaceSounds[index].interval);
        nk_layout_row_dynamic(context, 18.0f, 1);
        nk_label(context, label, NK_TEXT_LEFT);
        }
    nk_tree_pop(context);
    }

static void editPatrol(struct nk_context *context,
                       struct BattalionMapObject *object,
                       const struct EntityArchetype *archetype)
    {
    size_t index;
    char label[96];
    const char *modes[3];
    const char *responses[5];
    const char *selected;
    int mode;
    int response;
    float oldWait;

    if (archetype->kind == ENTITY_KIND_ROUTE)
        {
        if (!nk_tree_push(context, NK_TREE_TAB, "Route points",
                          NK_MAXIMIZED))
            return;
        for (index = 0; index < object->waypointCount; index++)
            {
            nk_bool selectedPoint;
            sprintf(label, "%lu: %.2f, %.2f", (unsigned long)index,
                    object->waypoints[index].x, object->waypoints[index].z);
            selectedPoint = editor.selectedWaypoint == (int)index;
            nk_layout_row_dynamic(context, 19.0f, 1);
            if (nk_selectable_label(context, label, NK_TEXT_LEFT,
                                    &selectedPoint) && selectedPoint)
                editor.selectedWaypoint = (int)index;
            }
        nk_layout_row_dynamic(context, 24.0f, 2);
        if (nk_button_label(context, "Add on map"))
            editorTool = EDITOR_TOOL_WAYPOINT;
        if (nk_button_label(context, "Delete") &&
            editor.selectedWaypoint >= 0)
            mapEditorDeleteWaypoint(&editor, (size_t)editor.selectedObject,
                                    (size_t)editor.selectedWaypoint);
        nk_tree_pop(context);
        return;
        }
    if (!entityArchetypeIsActor(archetype) ||
        archetype->movement == ENTITY_MOVEMENT_STATIONARY)
        return;
    if (!nk_tree_push(context, NK_TREE_TAB, "Patrol assignment",
                      NK_MAXIMIZED))
        return;
    nk_layout_row_dynamic(context, 16.0f, 1);
    nk_label(context, "shared route", NK_TEXT_LEFT);
    nk_layout_row_dynamic(context, 23.0f, 1);
    selected = object->patrolRoute[0] != '\0' ? object->patrolRoute : "none";
    if (nk_combo_begin_label(context, selected, nk_vec2(220.0f, 180.0f)))
        {
        nk_layout_row_dynamic(context, 20.0f, 1);
        if (nk_combo_item_label(context, "none", NK_TEXT_LEFT))
            {
            object->patrolRoute[0] = '\0';
            editor.dirty = 1;
            }
        for (index = 0; index < editor.map.objectCount; index++)
            {
            const struct EntityArchetype *routeArchetype;
            routeArchetype = entityCatalogFind(
                &editor.catalog, editor.map.objects[index].archetype);
            if (routeArchetype != NULL &&
                routeArchetype->kind == ENTITY_KIND_ROUTE &&
                editor.map.objects[index].id[0] != '\0' &&
                nk_combo_item_label(context, editor.map.objects[index].id,
                                    NK_TEXT_LEFT))
                {
                strcpy(object->patrolRoute, editor.map.objects[index].id);
                editor.dirty = 1;
                }
            }
        nk_combo_end(context);
        }
    modes[0] = "loop"; modes[1] = "pingpong"; modes[2] = "once";
    mode = object->patrolMode == ENTITY_PATROL_PINGPONG ? 1 :
           object->patrolMode == ENTITY_PATROL_ONCE ? 2 : 0;
    mode = nk_combo(context, modes, 3, mode, 22, nk_vec2(180.0f, 80.0f));
    if (object->patrolMode != (mode == 1 ? ENTITY_PATROL_PINGPONG :
                              mode == 2 ? ENTITY_PATROL_ONCE :
                                          ENTITY_PATROL_LOOP))
        {
        object->patrolMode = mode == 1 ? ENTITY_PATROL_PINGPONG :
                             mode == 2 ? ENTITY_PATROL_ONCE :
                                         ENTITY_PATROL_LOOP;
        editor.dirty = 1;
        }
    responses[0] = "inherit archetype";
    responses[1] = "continue patrol";
    responses[2] = "attack intruder";
    responses[3] = "retreat to safe zone";
    responses[4] = "random";
    response = object->alarmResponse;
    nk_layout_row_dynamic(context, 16.0f, 1);
    nk_label(context, "alarm response", NK_TEXT_LEFT);
    response = nk_combo(context, responses, 5, response, 22,
                        nk_vec2(220.0f, 120.0f));
    if (response != object->alarmResponse)
        {
        object->alarmResponse = response;
        if (response != ENTITY_ALARM_RETREAT &&
            response != ENTITY_ALARM_RANDOM)
            {
            object->safeZone[0] = '\0';
            object->safeWaitSeconds = 0.0f;
            }
        else if (object->safeWaitSeconds <= 0.0f)
            object->safeWaitSeconds = 5.0f;
        editor.dirty = 1;
        }
    if (object->alarmResponse == ENTITY_ALARM_RETREAT ||
        object->alarmResponse == ENTITY_ALARM_RANDOM)
        {
        nk_layout_row_dynamic(context, 16.0f, 1);
        nk_label(context, "safe zone", NK_TEXT_LEFT);
        selected = object->safeZone[0] != '\0' ? object->safeZone : "none";
        nk_layout_row_dynamic(context, 23.0f, 1);
        if (nk_combo_begin_label(context, selected,
                                 nk_vec2(220.0f, 180.0f)))
            {
            nk_layout_row_dynamic(context, 20.0f, 1);
            if (nk_combo_item_label(context, "none", NK_TEXT_LEFT))
                {
                object->safeZone[0] = '\0';
                editor.dirty = 1;
                }
            for (index = 0; index < editor.map.objectCount; index++)
                {
                const struct EntityArchetype *zoneArchetype;
                zoneArchetype = entityCatalogFind(
                    &editor.catalog, editor.map.objects[index].archetype);
                if (zoneArchetype != NULL &&
                    zoneArchetype->kind == ENTITY_KIND_ZONE &&
                    editor.map.objects[index].id[0] != '\0' &&
                    nk_combo_item_label(context,
                        editor.map.objects[index].id, NK_TEXT_LEFT))
                    {
                    strcpy(object->safeZone,
                           editor.map.objects[index].id);
                    editor.dirty = 1;
                    }
                }
            nk_combo_end(context);
            }
        oldWait = object->safeWaitSeconds;
        nk_layout_row_dynamic(context, 22.0f, 1);
        nk_property_float(context, "wait game seconds", 0.0f,
                          &object->safeWaitSeconds, 3600.0f, 1.0f, 0.2f);
        if (oldWait != object->safeWaitSeconds)
            editor.dirty = 1;
        }
    nk_tree_pop(context);
    }

static void editGeometry(struct nk_context *context,
                         struct BattalionMapObject *object,
                         const struct EntityArchetype *archetype)
    {
    size_t index;
    char label[96];
    if (archetype->kind == ENTITY_KIND_ROUTE)
        return;
    if (archetype->placement != ENTITY_PLACEMENT_POLYLINE &&
        archetype->placement != ENTITY_PLACEMENT_AREA)
        return;
    if (!nk_tree_push(context, NK_TREE_TAB, "Geometry", NK_MAXIMIZED))
        return;
    for (index = 0; index < object->geometryCount; index++)
        {
        nk_bool selected;
        sprintf(label, "%lu: %.2f, %.2f", (unsigned long)index,
                object->geometry[index].x, object->geometry[index].z);
        selected = editor.selectedGeometryPoint == (int)index;
        nk_layout_row_dynamic(context, 19.0f, 1);
        if (nk_selectable_label(context, label, NK_TEXT_LEFT, &selected) &&
            selected) editor.selectedGeometryPoint = (int)index;
        }
    nk_layout_row_dynamic(context, 24.0f, 2);
    if (nk_button_label(context, "Add on map")) editorTool = EDITOR_TOOL_GEOMETRY;
    if (nk_button_label(context, "Delete") &&
        editor.selectedGeometryPoint >= 0)
        mapEditorDeleteGeometryPoint(
            &editor, (size_t)editor.selectedObject,
            (size_t)editor.selectedGeometryPoint);
    nk_tree_pop(context);
    }

static void editTriggers(struct nk_context *context,
                         struct BattalionMapObject *object)
    {
    size_t index;
    char label[160];
    if (!nk_tree_push(context, NK_TREE_TAB, "Trigger routes", NK_MAXIMIZED))
        return;
    for (index = 0; index < object->triggerCount; index++)
        {
        nk_bool selected;
        sprintf(label, "%s -> %s:%s", object->triggers[index].event,
                object->triggers[index].targetType == BATTALION_MAP_TRIGGER_GROUP ?
                    "group" : "id", object->triggers[index].target);
        selected = editor.selectedTrigger == (int)index;
        nk_layout_row_dynamic(context, 19.0f, 1);
        if (nk_selectable_label(context, label, NK_TEXT_LEFT, &selected) &&
            selected) editor.selectedTrigger = (int)index;
        }
    nk_layout_row_dynamic(context, 24.0f, 2);
    if (nk_button_label(context, "Add route"))
        mapEditorAddTrigger(&editor, (size_t)editor.selectedObject, "event",
                            BATTALION_MAP_TRIGGER_ID, object->id);
    if (nk_button_label(context, "Delete") && editor.selectedTrigger >= 0)
        mapEditorDeleteTrigger(&editor, (size_t)editor.selectedObject,
                               (size_t)editor.selectedTrigger);
    if (editor.selectedTrigger >= 0 &&
        (size_t)editor.selectedTrigger < object->triggerCount)
        {
        struct BattalionMapTrigger *trigger;
        const char *types[2];
        int type;
        trigger = &object->triggers[editor.selectedTrigger];
        nk_layout_row_dynamic(context, 16.0f, 1);
        nk_label(context, "event", NK_TEXT_LEFT);
        nk_layout_row_dynamic(context, 23.0f, 1);
        if (editText(context, trigger->event, sizeof(trigger->event)))
            editor.dirty = 1;
        types[0] = "target-id"; types[1] = "target-group";
        type = trigger->targetType == BATTALION_MAP_TRIGGER_GROUP ? 1 : 0;
        type = nk_combo(context, types, 2, type, 22, nk_vec2(180.0f, 60.0f));
        trigger->targetType = type ? BATTALION_MAP_TRIGGER_GROUP :
                                     BATTALION_MAP_TRIGGER_ID;
        nk_layout_row_dynamic(context, 16.0f, 1);
        nk_label(context, "target", NK_TEXT_LEFT);
        nk_layout_row_dynamic(context, 23.0f, 1);
        if (editText(context, trigger->target, sizeof(trigger->target)))
            editor.dirty = 1;
        }
    nk_tree_pop(context);
    }

static void drawRightPanel(struct nk_context *context)
    {
    float x;
    float height;
    x = (float)windowWidth - EDITOR_RIGHT_WIDTH;
    height = (float)windowHeight - EDITOR_TOP_HEIGHT;
    if (nk_begin(context, "Properties",
                 nk_rect(x, EDITOR_TOP_HEIGHT, EDITOR_RIGHT_WIDTH, height),
                 NK_WINDOW_BORDER | NK_WINDOW_TITLE))
        {
        nk_layout_row_dynamic(context, 16.0f, 1);
        nk_label(context, "map id", NK_TEXT_LEFT);
        nk_layout_row_dynamic(context, 23.0f, 1);
        if (editText(context, editor.map.id, sizeof(editor.map.id)))
            editor.dirty = 1;
        nk_layout_row_dynamic(context, 16.0f, 1);
        nk_label(context, "map name", NK_TEXT_LEFT);
        nk_layout_row_dynamic(context, 23.0f, 1);
        if (editText(context, editor.map.name, sizeof(editor.map.name)))
            editor.dirty = 1;
        nk_layout_row_dynamic(context, 22.0f, 1);
        nk_property_float(context, "grid", 0.05f, &editor.gridSize,
                          100.0f, 0.05f, 0.02f);
        if (editor.selectedObject >= 0 &&
            (size_t)editor.selectedObject < editor.map.objectCount)
            {
            struct BattalionMapObject *object;
            const struct EntityArchetype *archetype;
            enum nk_widget_layout_states previewState;
            struct nk_command_buffer *canvas;
            struct nk_rect previewBounds;
            object = &editor.map.objects[editor.selectedObject];
            archetype = selectedObjectArchetype();
            nk_layout_row_dynamic(context, 18.0f, 1);
            nk_label(context, archetype ? archetypeLabel(archetype) :
                                        object->archetype, NK_TEXT_LEFT);
            if (archetype)
                {
                nk_layout_row_dynamic(context, 150.0f, 1);
                previewState = nk_widget(&previewBounds, context);
                if (previewState != NK_WIDGET_INVALID)
                    {
                    canvas = nk_window_get_canvas(context);
                    nk_fill_rect(canvas, previewBounds, 3.0f,
                                 nk_rgb(24, 28, 32));
                    nk_stroke_rect(canvas, previewBounds, 3.0f, 1.0f,
                                   nk_rgb(65, 72, 78));
                    queuePreview(context, previewBounds, archetype, 1, 1);
                    }
                nk_layout_row_dynamic(context, 34.0f, 1);
                nk_label_wrap(context, archetype->description);
                }
            editObjectIdentity(context, object);
            editObjectTransform(context, object);
            if (archetype) editFaction(context, object, archetype);
            if (archetype) editObjectProperties(context, object, archetype);
            if (archetype) drawObjectAudio(context, archetype);
            if (archetype) editPatrol(context, object, archetype);
            if (archetype) editGeometry(context, object, archetype);
            editTriggers(context, object);
            nk_layout_row_dynamic(context, 25.0f, 1);
            if (nk_button_label(context, "Delete object")) deleteRequested = 1;
            }
        else
            {
            nk_layout_row_dynamic(context, 22.0f, 1);
            nk_label(context, "No object selected", NK_TEXT_LEFT);
            }
        }
    nk_end(context);
    }

static struct nk_color objectColor(const struct EntityArchetype *archetype)
    {
    const struct AssetValue *value;
    float component;
    unsigned char bytes[3];
    size_t index;

    if (!archetype) return nk_rgb(180, 180, 180);
    value = assetValuesFind(&editorPalette, archetype->editorColor);
    if (value == NULL || value->componentCount < 3)
        return nk_rgb(255, 0, 255);
    for (index = 0; index < 3; index++)
        {
        component = value->components[index];
        if (component < 0.0f) component = 0.0f;
        if (component > 1.0f) component = 1.0f;
        bytes[index] = (unsigned char)(component * 255.0f);
        }
    return nk_rgb(bytes[0], bytes[1], bytes[2]);
    }

static const struct AssetManifestModel *previewModel(
    const struct EntityArchetype *archetype, int large)
    {
    const char *key;

    if (archetype == NULL ||
        archetype->renderStyle != ENTITY_RENDER_MODEL)
        return NULL;
    key = (!large && archetype->lowModel[0] != '\0') ?
          archetype->lowModel : archetype->model;
    if (key[0] == '\0')
        return NULL;
    return assetManifestFindModel(&editorAssets, key);
    }

static void drawPreviewFallback(struct nk_color color)
    {
    float rgba[4];
    static const float vertices[8][3] = {
        {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f},
        { 0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f},
        {-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f},
        { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}
        };
    static const int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0}, {4,5},{5,6},{6,7},{7,4},
        {0,4},{1,5},{2,6},{3,7}
        };
    int index;

    rgba[0] = (float)color.r / 255.0f;
    rgba[1] = (float)color.g / 255.0f;
    rgba[2] = (float)color.b / 255.0f;
    rgba[3] = 1.0f;
    glColor4fv(rgba);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    for (index = 0; index < 12; index++)
        {
        glVertex3fv(vertices[edges[index][0]]);
        glVertex3fv(vertices[edges[index][1]]);
        }
    glEnd();
    }

static void renderPreview(const struct EditorPreviewRequest *request)
    {
    const struct AssetManifestModel *model;
    struct AssetPose pose;
    struct nk_color color;
    float minimum[3];
    float maximum[3];
    float center[3];
    float dx;
    float dy;
    float dz;
    float radius;
    float aspect;
    float horizontal;
    float vertical;
    float rgba[4];
    int x;
    int y;
    int width;
    int height;

    x = (int)request->bounds.x + 2;
    y = windowHeight - (int)(request->bounds.y + request->bounds.h) + 2;
    width = (int)request->bounds.w - 4;
    height = (int)request->bounds.h - 4;
    if (width < 4 || height < 4)
        return;
    model = previewModel(request->archetype, request->large);
    if (model != NULL && assetModelPoseBounds(
            &model->model, NULL, minimum, maximum))
        {
        center[0] = (minimum[0] + maximum[0]) * 0.5f;
        center[1] = (minimum[1] + maximum[1]) * 0.5f;
        center[2] = (minimum[2] + maximum[2]) * 0.5f;
        dx = maximum[0] - minimum[0];
        dy = maximum[1] - minimum[1];
        dz = maximum[2] - minimum[2];
        radius = (float)sqrt((double)(dx * dx + dy * dy + dz * dz)) *
                 0.58f;
        }
    else
        {
        center[0] = center[1] = center[2] = 0.0f;
        radius = 0.9f;
        }
    if (radius < 0.001f)
        radius = 1.0f;
    aspect = (float)width / (float)height;
    horizontal = aspect >= 1.0f ? radius * aspect : radius;
    vertical = aspect >= 1.0f ? radius : radius / aspect;
    color = objectColor(request->archetype);
    rgba[0] = (float)color.r / 255.0f;
    rgba[1] = (float)color.g / 255.0f;
    rgba[2] = (float)color.b / 255.0f;
    rgba[3] = 1.0f;

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glViewport(x, y, width, height);
    glScissor(x, y, width, height);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-horizontal, horizontal, -vertical, vertical,
            -radius * 4.0f, radius * 4.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glRotatef(-22.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(previewAngle + request->archetype->modelYawOffset,
              0.0f, 1.0f, 0.0f);
    glTranslatef(-center[0], -center[1], -center[2]);
    glColor4fv(rgba);
    if (model != NULL)
        {
        assetPoseInitialize(&pose);
        pose.colorOffset[0] = 0.18f;
        pose.colorOffset[1] = 0.18f;
        pose.colorOffset[2] = 0.18f;
        assetModelDraw(&model->model, &pose);
        assetPoseDestroy(&pose);
        }
    else
        drawPreviewFallback(color);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();
    }

static void renderPreviews(void)
    {
    size_t index;

    for (index = 0; index < previewRequestCount; index++)
        renderPreview(&previewRequests[index]);
    previewAngle += 0.7f;
    if (previewAngle >= 360.0f)
        previewAngle -= 360.0f;
    }

static void worldPoint(float x, float z, float *screenX, float *screenY)
    {
    mapEditorWorldToScreen(&editor, viewportRect.x, viewportRect.y,
                           viewportRect.w, viewportRect.h,
                           x, z, screenX, screenY);
    }

static void drawGrid(struct nk_command_buffer *canvas)
    {
    float worldLeft;
    float worldTop;
    float worldRight;
    float worldBottom;
    float x;
    float z;
    float screenX;
    float screenY;
    float spacing;
    if (!editor.gridVisible || editor.gridSize <= 0.0f) return;
    mapEditorScreenToWorld(&editor, viewportRect.x, viewportRect.y,
                           viewportRect.w, viewportRect.h,
                           viewportRect.x, viewportRect.y,
                           &worldLeft, &worldTop);
    mapEditorScreenToWorld(&editor, viewportRect.x, viewportRect.y,
                           viewportRect.w, viewportRect.h,
                           viewportRect.x + viewportRect.w,
                           viewportRect.y + viewportRect.h,
                           &worldRight, &worldBottom);
    spacing = editor.gridSize;
    while (spacing * editor.zoom < 8.0f) spacing *= 2.0f;
    x = (float)floor((double)(worldLeft / spacing)) * spacing;
    while (x <= worldRight)
        {
        worldPoint(x, 0.0f, &screenX, &screenY);
        nk_stroke_line(canvas, screenX, viewportRect.y, screenX,
                       viewportRect.y + viewportRect.h, 1.0f,
                       x == 0.0f ? nk_rgb(94, 65, 65) : nk_rgb(48, 55, 61));
        x += spacing;
        }
    z = (float)floor((double)(worldTop / spacing)) * spacing;
    while (z <= worldBottom)
        {
        worldPoint(0.0f, z, &screenX, &screenY);
        nk_stroke_line(canvas, viewportRect.x, screenY,
                       viewportRect.x + viewportRect.w, screenY, 1.0f,
                       z == 0.0f ? nk_rgb(65, 76, 94) : nk_rgb(48, 55, 61));
        z += spacing;
        }
    }

static void drawObjectGeometry(struct nk_command_buffer *canvas,
                               const struct BattalionMapObject *object,
                               const struct EntityArchetype *archetype,
                               int selected)
    {
    size_t index;
    float x0;
    float y0;
    float x1;
    float y1;
    struct nk_color color;
    color = selected ? nk_rgb(255, 226, 90) : objectColor(archetype);
    for (index = 1; index < object->geometryCount; index++)
        {
        worldPoint(object->geometry[index - 1].x,
                   object->geometry[index - 1].z, &x0, &y0);
        worldPoint(object->geometry[index].x, object->geometry[index].z,
                   &x1, &y1);
        nk_stroke_line(canvas, x0, y0, x1, y1, 2.0f, color);
        }
    if (archetype && archetype->placement == ENTITY_PLACEMENT_AREA &&
        object->geometryCount > 2)
        {
        worldPoint(object->geometry[object->geometryCount - 1].x,
                   object->geometry[object->geometryCount - 1].z, &x0, &y0);
        worldPoint(object->geometry[0].x, object->geometry[0].z, &x1, &y1);
        nk_stroke_line(canvas, x0, y0, x1, y1, 2.0f, color);
        }
    if (selected)
        for (index = 0; index < object->geometryCount; index++)
            {
            worldPoint(object->geometry[index].x, object->geometry[index].z,
                       &x0, &y0);
            nk_fill_circle(canvas, nk_rect(x0 - 4.0f, y0 - 4.0f, 8.0f, 8.0f),
                           editor.selectedGeometryPoint == (int)index ?
                           nk_rgb(255, 255, 255) : color);
            }
    }

static const struct BattalionMapObject *editorObjectById(const char *id)
    {
    size_t index;

    if (id == NULL || id[0] == '\0')
        return NULL;
    for (index = 0; index < editor.map.objectCount; index++)
        if (strcmp(editor.map.objects[index].id, id) == 0)
            return &editor.map.objects[index];
    return NULL;
    }

static void drawPatrolRoute(struct nk_command_buffer *canvas,
                            const struct BattalionMapObject *route,
                            int selected)
    {
    float previousX;
    float previousY;
    float x;
    float y;
    size_t index;
    struct nk_color color;

    if (route == NULL || route->waypointCount == 0)
        return;
    color = selected ? nk_rgb(255, 226, 90) : nk_rgb(82, 200, 117);
    worldPoint(route->waypoints[0].x, route->waypoints[0].z,
               &previousX, &previousY);
    for (index = 0; index < route->waypointCount; index++)
        {
        worldPoint(route->waypoints[index].x, route->waypoints[index].z,
                   &x, &y);
        if (index > 0)
            nk_stroke_line(canvas, previousX, previousY, x, y,
                           selected ? 3.0f : 2.0f, color);
        nk_fill_circle(canvas, nk_rect(x - 4.0f, y - 4.0f, 8.0f, 8.0f),
            selected && editor.selectedWaypoint == (int)index ?
            nk_rgb(255, 255, 255) : color);
        previousX = x;
        previousY = y;
        }
    }

static void drawObjects(struct nk_command_buffer *canvas)
    {
    size_t index;
    const struct BattalionMapObject *object;
    const struct BattalionMapObject *assignedRoute;
    const struct BattalionMapObject *assignedSafeZone;
    const struct EntityArchetype *archetype;
    const struct EntityArchetype *safeZoneArchetype;
    float x;
    float y;
    int selected;
    struct nk_color color;
    char label[96];
    for (index = 0; index < editor.map.objectCount; index++)
        {
        object = &editor.map.objects[index];
        archetype = entityCatalogFind(&editor.catalog, object->archetype);
        selected = editor.selectedObject == (int)index;
        color = selected ? nk_rgb(255, 226, 90) : objectColor(archetype);
        drawObjectGeometry(canvas, object, archetype, selected);
        if (archetype != NULL && archetype->kind == ENTITY_KIND_ROUTE)
            drawPatrolRoute(canvas, object, selected);
        if (selected && object->patrolRoute[0] != '\0')
            {
            assignedRoute = editorObjectById(object->patrolRoute);
            drawPatrolRoute(canvas, assignedRoute, 1);
            }
        if (selected && object->safeZone[0] != '\0')
            {
            assignedSafeZone = editorObjectById(object->safeZone);
            safeZoneArchetype = assignedSafeZone == NULL ? NULL :
                entityCatalogFind(&editor.catalog,
                                  assignedSafeZone->archetype);
            if (assignedSafeZone != NULL)
                drawObjectGeometry(canvas, assignedSafeZone,
                                   safeZoneArchetype, 1);
            }
        worldPoint(object->x, object->z, &x, &y);
        if (archetype && archetype->placement == ENTITY_PLACEMENT_TILE)
            {
            float tile;
            tile = editor.zoom * 0.5f * object->scale;
            if (tile < 5.0f) tile = 5.0f;
            nk_fill_rect(canvas, nk_rect(x - tile * 0.5f, y - tile * 0.5f,
                                         tile, tile), 0.0f, color);
            }
        else
            nk_fill_circle(canvas, nk_rect(x - 5.0f, y - 5.0f, 10.0f, 10.0f),
                           color);
        if (selected)
            {
            nk_stroke_circle(canvas, nk_rect(x - 8.0f, y - 8.0f, 16.0f, 16.0f),
                             2.0f, nk_rgb(255, 255, 255));
            sprintf(label, "%s", object->id[0] ? object->id : object->archetype);
            nk_draw_text(canvas, nk_rect(x + 8.0f, y - 7.0f, 180.0f, 14.0f),
                         label, (int)strlen(label), &gui.font,
                         nk_rgba(0, 0, 0, 0), nk_rgb(240, 240, 240));
            }
        }
    }

static float squaredDistance(float x0, float y0, float x1, float y1)
    {
    float dx;
    float dy;
    dx = x0 - x1; dy = y0 - y1;
    return dx * dx + dy * dy;
    }

static int hitObject(float mouseX, float mouseY)
    {
    size_t index;
    float x;
    float y;
    float best;
    float distance;
    int result;
    best = EDITOR_HIT_RADIUS * EDITOR_HIT_RADIUS;
    result = -1;
    for (index = 0; index < editor.map.objectCount; index++)
        {
        worldPoint(editor.map.objects[index].x, editor.map.objects[index].z,
                   &x, &y);
        distance = squaredDistance(mouseX, mouseY, x, y);
        if (distance <= best) { best = distance; result = (int)index; }
        }
    return result;
    }

static int hitSelectedPoint(float mouseX, float mouseY, int geometry)
    {
    const struct BattalionMapObject *object;
    size_t index;
    size_t count;
    float x;
    float y;
    float worldX;
    float worldZ;
    if (editor.selectedObject < 0 ||
        (size_t)editor.selectedObject >= editor.map.objectCount) return -1;
    object = &editor.map.objects[editor.selectedObject];
    count = geometry ? object->geometryCount : object->waypointCount;
    for (index = 0; index < count; index++)
        {
        if (geometry) { worldX = object->geometry[index].x;
                        worldZ = object->geometry[index].z; }
        else { worldX = object->waypoints[index].x;
               worldZ = object->waypoints[index].z; }
        worldPoint(worldX, worldZ, &x, &y);
        if (squaredDistance(mouseX, mouseY, x, y) <= 64.0f)
            return (int)index;
        }
    return -1;
    }

static void zoomAtMouse(float wheel)
    {
    const struct nk_input *input;
    float beforeX;
    float beforeZ;
    float afterX;
    float afterZ;
    input = &gui.context.input;
    mapEditorScreenToWorld(&editor, viewportRect.x, viewportRect.y,
        viewportRect.w, viewportRect.h, input->mouse.pos.x, input->mouse.pos.y,
        &beforeX, &beforeZ);
    editor.zoom *= wheel > 0.0f ? 1.2f : (1.0f / 1.2f);
    if (editor.zoom < 1.0f) editor.zoom = 1.0f;
    if (editor.zoom > 160.0f) editor.zoom = 160.0f;
    mapEditorScreenToWorld(&editor, viewportRect.x, viewportRect.y,
        viewportRect.w, viewportRect.h, input->mouse.pos.x, input->mouse.pos.y,
        &afterX, &afterZ);
    editor.cameraX += beforeX - afterX;
    editor.cameraZ += beforeZ - afterZ;
    }

static void placeSelectedArchetype(float worldX, float worldZ)
    {
    const struct EntityArchetype *archetype;

    if (editor.selectedArchetype < 0 ||
        (size_t)editor.selectedArchetype >= editor.catalog.count)
        return;
    archetype = &editor.catalog.entries[editor.selectedArchetype];
    if (!mapEditorAddSelectedObject(&editor, worldX, worldZ))
        {
        copyStatus("Could not add object");
        return;
        }
    if (archetype->kind == ENTITY_KIND_ROUTE)
        {
        editorTool = EDITOR_TOOL_WAYPOINT;
        copyStatus("Route added: click the map to add patrol waypoints");
        }
    else if (archetype->placement == ENTITY_PLACEMENT_POLYLINE ||
             archetype->placement == ENTITY_PLACEMENT_AREA)
        {
        editorTool = EDITOR_TOOL_GEOMETRY;
        copyStatus("Object added: click the map to add geometry points");
        }
    else
        copyStatus("Object added: click the map to place another; Esc cancels");
    }

static void interactViewport(void)
    {
    const struct nk_input *input;
    float worldX;
    float worldZ;
    int hovered;
    int hit;
    input = &gui.context.input;
    hovered = nk_input_is_mouse_hovering_rect(input, viewportRect);
    if (hovered && input->mouse.scroll_delta.y != 0.0f)
        zoomAtMouse(input->mouse.scroll_delta.y);
    if (hovered && (nk_input_is_mouse_down(input, NK_BUTTON_RIGHT) ||
                    nk_input_is_mouse_down(input, NK_BUTTON_MIDDLE)))
        {
        editor.cameraX -= input->mouse.delta.x / editor.zoom;
        editor.cameraZ -= input->mouse.delta.y / editor.zoom;
        }
    if (hovered && viewportLeftPressed)
        {
        mapEditorScreenToWorld(&editor, viewportRect.x, viewportRect.y,
            viewportRect.w, viewportRect.h, input->mouse.pos.x,
            input->mouse.pos.y, &worldX, &worldZ);
        if (editorTool == EDITOR_TOOL_PLACE && editor.selectedArchetype >= 0)
            placeSelectedArchetype(worldX, worldZ);
        else if (editorTool == EDITOR_TOOL_WAYPOINT &&
                 editor.selectedObject >= 0)
            mapEditorAddWaypoint(&editor, (size_t)editor.selectedObject,
                                 worldX, worldZ);
        else if (editorTool == EDITOR_TOOL_GEOMETRY &&
                 editor.selectedObject >= 0)
            mapEditorAddGeometryPoint(&editor, (size_t)editor.selectedObject,
                                      worldX, worldZ);
        else
            {
            hit = hitSelectedPoint(input->mouse.pos.x, input->mouse.pos.y, 0);
            if (hit >= 0) { editor.selectedWaypoint = hit;
                            draggingWaypoint = hit; }
            else
                {
                hit = hitSelectedPoint(input->mouse.pos.x,
                                       input->mouse.pos.y, 1);
                if (hit >= 0) { editor.selectedGeometryPoint = hit;
                                draggingGeometryPoint = hit; }
                else
                    {
                    hit = hitObject(input->mouse.pos.x, input->mouse.pos.y);
                    selectObject(hit);
                    draggingObject = hit;
                    }
                }
            }
        }
    if (nk_input_is_mouse_down(input, NK_BUTTON_LEFT))
        {
        mapEditorScreenToWorld(&editor, viewportRect.x, viewportRect.y,
            viewportRect.w, viewportRect.h, input->mouse.pos.x,
            input->mouse.pos.y, &worldX, &worldZ);
        if (draggingWaypoint >= 0 && editor.selectedObject >= 0)
            mapEditorMoveWaypoint(&editor, (size_t)editor.selectedObject,
                                  (size_t)draggingWaypoint, worldX, worldZ);
        else if (draggingGeometryPoint >= 0 && editor.selectedObject >= 0)
            mapEditorMoveGeometryPoint(&editor, (size_t)editor.selectedObject,
                (size_t)draggingGeometryPoint, worldX, worldZ);
        else if (draggingObject >= 0)
            mapEditorMoveObject(&editor, (size_t)draggingObject, worldX, worldZ);
        }
    else
        {
        draggingObject = -1;
        draggingWaypoint = -1;
        draggingGeometryPoint = -1;
        }
    }

static void drawViewport(struct nk_context *context)
    {
    float width;
    float height;
    width = (float)windowWidth - EDITOR_LEFT_WIDTH - EDITOR_RIGHT_WIDTH;
    height = (float)windowHeight - EDITOR_TOP_HEIGHT;
    if (width < EDITOR_MIN_VIEW) width = EDITOR_MIN_VIEW;
    if (nk_begin(context, "Map viewport",
                 nk_rect(EDITOR_LEFT_WIDTH, EDITOR_TOP_HEIGHT, width, height),
                 NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR))
        {
        struct nk_command_buffer *canvas;
        canvas = nk_window_get_canvas(context);
        viewportRect = nk_window_get_content_region(context);
        nk_fill_rect(canvas, viewportRect, 0.0f, nk_rgb(31, 36, 41));
        interactViewport();
        drawGrid(canvas);
        drawObjects(canvas);
        }
    nk_end(context);
    }

static void drawFileDialog(struct nk_context *context)
    {
    const char *title;
    if (fileDialogMode == EDITOR_FILE_NONE) return;
    title = fileDialogMode == EDITOR_FILE_OPEN ? "Open map.json" :
                                                "Save map.json";
    if (nk_begin(context, title,
                 nk_rect((float)windowWidth * 0.5f - 250.0f,
                         (float)windowHeight * 0.5f - 80.0f,
                         500.0f, 160.0f),
                 NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_MOVABLE))
        {
        nk_layout_row_dynamic(context, 18.0f, 1);
        nk_label(context, "assets/maps/<id>/map.json", NK_TEXT_LEFT);
        nk_layout_row_dynamic(context, 24.0f, 1);
        editText(context, filePath, sizeof(filePath));
        nk_layout_row_dynamic(context, 26.0f, 2);
        if (nk_button_label(context, fileDialogMode == EDITOR_FILE_OPEN ?
                                    "Open" : "Save"))
            {
            if (fileDialogMode == EDITOR_FILE_OPEN)
                {
                if (mapEditorLoad(&editor, filePath))
                    {
                    mapEditorFitContents(&editor, viewportRect.w, viewportRect.h);
                    fileDialogMode = EDITOR_FILE_NONE;
                    }
                }
            else if (mapEditorSave(&editor, filePath))
                fileDialogMode = EDITOR_FILE_NONE;
            }
        if (nk_button_label(context, "Cancel")) fileDialogMode = EDITOR_FILE_NONE;
        }
    nk_end(context);
    }

static void drawNewDialog(struct nk_context *context)
    {
    if (!newDialogVisible) return;
    if (nk_begin(context, "New Battalion map v2",
                 nk_rect((float)windowWidth * 0.5f - 220.0f,
                         (float)windowHeight * 0.5f - 100.0f,
                         440.0f, 200.0f),
                 NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_MOVABLE))
        {
        nk_layout_row_dynamic(context, 16.0f, 1);
        nk_label(context, "id", NK_TEXT_LEFT);
        nk_layout_row_dynamic(context, 23.0f, 1);
        editText(context, newMapId, sizeof(newMapId));
        nk_layout_row_dynamic(context, 16.0f, 1);
        nk_label(context, "name", NK_TEXT_LEFT);
        nk_layout_row_dynamic(context, 23.0f, 1);
        editText(context, newMapName, sizeof(newMapName));
        nk_layout_row_dynamic(context, 26.0f, 2);
        if (nk_button_label(context, "Create") &&
            mapEditorNew(&editor, newMapId, newMapName))
            { newDialogVisible = 0; filePath[0] = '\0'; }
        if (nk_button_label(context, "Cancel")) newDialogVisible = 0;
        }
    nk_end(context);
    }

static void processDeleteRequest(void)
    {
    if (!deleteRequested) return;
    deleteRequested = 0;
    if (nk_item_is_any_active(&gui.context)) return;
    if (editor.selectedObject < 0) return;
    if (editor.selectedWaypoint >= 0)
        mapEditorDeleteWaypoint(&editor, (size_t)editor.selectedObject,
                                (size_t)editor.selectedWaypoint);
    else if (editor.selectedGeometryPoint >= 0)
        mapEditorDeleteGeometryPoint(&editor, (size_t)editor.selectedObject,
                                     (size_t)editor.selectedGeometryPoint);
    else
        mapEditorDeleteObject(&editor, (size_t)editor.selectedObject);
    }

static void display(void)
    {
    mapEditorGuiInput(&gui, &guiInput);
    guiInput.wheel = 0.0f;
    guiInput.textLength = 0;
    previewRequestCount = 0;
    glClearColor(0.08f, 0.09f, 0.10f, 1.0f);
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
    if (fileDialogMode == EDITOR_FILE_NONE && !newDialogVisible)
        renderPreviews();
    tkSwapBuffers();
    }

static void reshape(int width, int height)
    { windowWidth = width; windowHeight = height; }

static GLenum keyDown(int key, GLenum modifiers)
    {
    guiInput.shift = (modifiers & TK_SHIFT) != 0;
    guiInput.control = (modifiers & TK_CONTROL) != 0;
    if (key == TK_LEFT) guiInput.left = 1;
    else if (key == TK_RIGHT) guiInput.right = 1;
    else if (key == TK_BACKSPACE) guiInput.backspace = 1;
    else if (key == TK_DELETE) { guiInput.deleteKey = 1; deleteRequested = 1; }
    else if (key == TK_RETURN) guiInput.enter = 1;
    else if (key == TK_TAB) guiInput.tab = 1;
    else if (key == TK_HOME) guiInput.home = 1;
    else if (key == TK_END) guiInput.end = 1;
    else if ((key == TK_s || key == TK_S) && guiInput.control) saveCurrentMap();
    else if ((key == TK_o || key == TK_O) && guiInput.control)
        fileDialogMode = EDITOR_FILE_OPEN;
    else if ((key == TK_n || key == TK_N) && guiInput.control)
        newDialogVisible = 1;
    else if (key == TK_ESCAPE)
        {
        editorTool = EDITOR_TOOL_SELECT;
        fileDialogMode = EDITOR_FILE_NONE;
        newDialogVisible = 0;
        copyStatus("Select/move tool");
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
    else if (key == TK_CONTROL_L || key == TK_CONTROL_R) guiInput.control = 0;
    else if (key == TK_SHIFT_L || key == TK_SHIFT_R) guiInput.shift = 0;
    return GL_TRUE;
    }

static GLenum mouseDown(int x, int y, GLenum button)
    {
    guiInput.mouseX = x; guiInput.mouseY = y;
    if (button & TK_WHEELUP) guiInput.wheel += 1.0f;
    if (button & TK_WHEELDOWN) guiInput.wheel -= 1.0f;
    if (button & TK_LEFTBUTTON) { guiInput.mouseLeft = 1;
                                 viewportLeftPressed = 1; }
    if (button & TK_RIGHTBUTTON) guiInput.mouseRight = 1;
    if (button & TK_MIDDLEBUTTON) guiInput.mouseMiddle = 1;
    return GL_TRUE;
    }

static GLenum mouseUp(int x, int y, GLenum button)
    {
    guiInput.mouseX = x; guiInput.mouseY = y;
    if (button & TK_LEFTBUTTON) guiInput.mouseLeft = 0;
    if (button & TK_RIGHTBUTTON) guiInput.mouseRight = 0;
    if (button & TK_MIDDLEBUTTON) guiInput.mouseMiddle = 0;
    return GL_TRUE;
    }

static GLenum mouseMove(int x, int y, GLenum button)
    {
    (void)button; guiInput.mouseX = x; guiInput.mouseY = y; return GL_TRUE;
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
    if (result) printf("%s: valid battalion map\n", path);
    else fprintf(stderr, "%s: %s\n", path, validationEditor.status);
    mapEditorModelDestroy(&validationEditor);
    return result ? 0 : 1;
    }

static void usage(const char *program)
    {
    fprintf(stderr, "Usage: %s [--assets <path>] [map.json]\n"
                    "       %s [--assets <path>] --validate <map.json>\n",
                    program, program);
    }

int main(int argc, char **argv)
    {
    const char *assetRoot;
    const char *mapPath;
    const char *validationPath;
    char manifestRoot[MAP_EDITOR_PATH_SIZE];
    char palettePath[MAP_EDITOR_PATH_SIZE];
    char paletteError[256];
    size_t assetRootLength;
    int index;
    assetRoot = "./assets"; mapPath = NULL; validationPath = NULL;
    for (index = 1; index < argc; index++)
        if (strcmp(argv[index], "--assets") == 0 && index + 1 < argc)
            assetRoot = argv[++index];
        else if (strcmp(argv[index], "--validate") == 0 && index + 1 < argc)
            validationPath = argv[++index];
        else if (argv[index][0] != '-' && mapPath == NULL) mapPath = argv[index];
        else { usage(argv[0]); return 2; }
    if (validationPath) return validateCommand(assetRoot, validationPath);
    mapEditorModelInitialize(&editor);
    assetValuesInitialize(&editorPalette);
    if (!mapEditorLoadCatalog(&editor, assetRoot, "entities/catalog.txt"))
        {
        fprintf(stderr, "map_editor: %s\n", editor.status);
        assetValuesDestroy(&editorPalette);
        mapEditorModelDestroy(&editor);
        return 1;
        }
    assetRootLength = strlen(assetRoot);
    if (assetRootLength + 2 > sizeof(manifestRoot))
        {
        fprintf(stderr, "map_editor: asset path is too long\n");
        assetValuesDestroy(&editorPalette);
        mapEditorModelDestroy(&editor);
        return 1;
        }
    strcpy(manifestRoot, assetRoot);
    if (assetRootLength > 0 && manifestRoot[assetRootLength - 1] != '/')
        strcat(manifestRoot, "/");
    if (assetRootLength + strlen("/config/palette.values") + 1 >
        sizeof(palettePath))
        {
        fprintf(stderr, "map_editor: asset path is too long\n");
        assetValuesDestroy(&editorPalette);
        mapEditorModelDestroy(&editor);
        return 1;
        }
    strcpy(palettePath, assetRoot);
    if (assetRootLength > 0 && palettePath[assetRootLength - 1] != '/')
        strcat(palettePath, "/");
    strcat(palettePath, "config/palette.values");
    if (!assetValuesLoad(palettePath, &editorPalette,
                         paletteError, sizeof(paletteError)))
        {
        fprintf(stderr, "map_editor: %s\n", paletteError);
        assetValuesDestroy(&editorPalette);
        mapEditorModelDestroy(&editor);
        return 1;
        }
    if (mapPath)
        {
        if (!mapEditorLoad(&editor, mapPath))
            {
            fprintf(stderr, "map_editor: %s\n", editor.status);
            assetValuesDestroy(&editorPalette);
            mapEditorModelDestroy(&editor);
            return 1;
            }
        strncpy(filePath, mapPath, sizeof(filePath) - 1);
        filePath[sizeof(filePath) - 1] = '\0';
        mapEditorFitContents(&editor, 700.0f, 650.0f);
        }
    else mapEditorNew(&editor, newMapId, newMapName);
    memset(&guiInput, 0, sizeof(guiInput));
    assetManifestInitialize(&editorAssets);
    viewportRect = nk_rect(EDITOR_LEFT_WIDTH, EDITOR_TOP_HEIGHT, 700.0f, 650.0f);
    tkInitDisplayMode(TK_RGB | TK_DOUBLE | TK_DEPTH | TK_ALPHA);
    tkInitPosition(80, 80, windowWidth, windowHeight);
    tkReshapeFunc(reshape); tkDisplayFunc(display); tkIdleFunc(display);
    tkKeyDownFunc(keyDown); tkKeyUpFunc(keyUp);
    tkMouseDownFunc(mouseDown); tkMouseUpFunc(mouseUp); tkMouseMoveFunc(mouseMove);
    if (tkInitWindow("Battalion Map Editor") == GL_FALSE)
        {
        assetManifestDestroy(&editorAssets);
        assetValuesDestroy(&editorPalette);
        mapEditorModelDestroy(&editor);
        return 1;
        }
    if (!assetManifestLoad(manifestRoot, &editorAssets,
                           paletteError, sizeof(paletteError)))
        {
        fprintf(stderr, "map_editor: %s\n", paletteError);
        tkCloseWindow();
        assetManifestDestroy(&editorAssets);
        assetValuesDestroy(&editorPalette);
        mapEditorModelDestroy(&editor);
        return 1;
        }
    if (!mapEditorGuiInitialize(&gui))
        {
        assetManifestDestroy(&editorAssets);
        tkCloseWindow();
        assetValuesDestroy(&editorPalette);
        mapEditorModelDestroy(&editor);
        return 1;
        }
    tkExec();
    mapEditorGuiDestroy(&gui);
    assetManifestDestroy(&editorAssets);
    tkCloseWindow();
    assetValuesDestroy(&editorPalette);
    mapEditorModelDestroy(&editor);
    return 0;
    }
