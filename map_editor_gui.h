/*
 * Copyright (c) 2026 Battalion contributors.
 */
/****************************************************************/
/* Minimal Nuklear adapter for Battalion's shared GL frontend.     */
/****************************************************************/

#ifndef BATTALION_MAP_EDITOR_GUI_H
#define BATTALION_MAP_EDITOR_GUI_H

#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR

#include "vendor/nuklear/nuklear.h"

#define MAP_EDITOR_GUI_TEXT_CAPACITY 32
#define MAP_EDITOR_GUI_FONT_DEVICE_PIXELS 12.0f

struct MapEditorGuiInput
    {
    int mouseX;
    int mouseY;
    int mouseLeft;
    int mouseRight;
    int mouseMiddle;
    int shift;
    int control;
    int left;
    int right;
    int backspace;
    int deleteKey;
    int enter;
    int tab;
    int home;
    int end;
    float wheel;
    unsigned int text[MAP_EDITOR_GUI_TEXT_CAPACITY];
    int textLength;
    };

struct MapEditorGui
    {
    struct nk_context context;
    struct nk_user_font font;
    int initialized;
    };

int mapEditorGuiInitialize(struct MapEditorGui *gui);
void mapEditorGuiDestroy(struct MapEditorGui *gui);
void mapEditorGuiInput(struct MapEditorGui *gui,
                       const struct MapEditorGuiInput *input);
int mapEditorGuiWantsMouse(const struct MapEditorGui *gui);
void mapEditorGuiRender(struct MapEditorGui *gui, int width, int height);

#endif
