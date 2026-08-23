/*
 * Copyright (c) 2026 Battalion contributors.
 */
#include "map_editor_gui.h"
#include "renderer/backend.h"

#include <stdio.h>
#include <string.h>

static int drawCalls;
static int drawVertices;
static int scissorDraws;
static float testDisplayScale = 2.0f;

float tkDisplayScale(void)
    {
    return testDisplayScale;
    }

int rendererBackendInitialize(void *nativeView)
    {
    (void)nativeView;
    return 1;
    }

void rendererBackendResize(int width, int height)
    {
    (void)width;
    (void)height;
    }

void rendererBackendShutdown(void)
    {
    }

void rendererBackendClear(unsigned int buffers,
                          const float color[4], float depth)
    {
    (void)buffers;
    (void)color;
    (void)depth;
    }

void rendererBackendDraw(enum RendererPrimitive primitive,
                         const RendererVertex *vertices,
                         size_t vertexCount,
                         const RendererDrawState *state)
    {
    (void)primitive;
    if (vertices == NULL || state == NULL || vertexCount == 0)
        return;
    drawCalls++;
    drawVertices += (int)vertexCount;
    if (state->scissorEnabled && state->scissor[2] > 0 &&
        state->scissor[3] > 0)
        scissorDraws++;
    }

unsigned int rendererBackendCreateLuminanceTexture(
    int width, int height, const unsigned char *pixels)
    {
    (void)width;
    (void)height;
    (void)pixels;
    return 1;
    }

unsigned int rendererBackendCreateRGBA8Texture(
    int width, int height, const unsigned char *pixels)
    {
    (void)width;
    (void)height;
    (void)pixels;
    return 1;
    }

void rendererBackendPresent(void)
    {
    }

int main(void)
    {
    struct MapEditorGui gui;
    struct MapEditorGuiInput input;
    struct nk_context *context;

    memset(&gui, 0, sizeof(gui));
    memset(&input, 0, sizeof(input));
    if (!mapEditorGuiInitialize(&gui))
        {
        fprintf(stderr, "map editor gui init failed\n");
        return 1;
        }
    if (gui.font.height !=
        MAP_EDITOR_GUI_FONT_DEVICE_PIXELS / testDisplayScale)
        {
        fprintf(stderr, "unexpected map editor font height: %.1f\n",
                gui.font.height);
        return 1;
        }
    input.mouseX = 40;
    input.mouseY = 40;
    input.mouseLeft = 1;
    input.backspace = 1;
    input.text[0] = 'B';
    input.textLength = 1;
    mapEditorGuiInput(&gui, &input);
    context = &gui.context;
    if (context->input.keyboard.text_len != 1 ||
        context->input.keyboard.text[0] != 'B' ||
        !nk_input_is_key_pressed(&context->input, NK_KEY_BACKSPACE) ||
        !nk_input_is_mouse_pressed(&context->input, NK_BUTTON_LEFT))
        {
        fprintf(stderr, "map editor gui input was not forwarded\n");
        return 1;
        }
    if (nk_begin(context, "test", nk_rect(10, 10, 260, 180),
                 NK_WINDOW_BORDER | NK_WINDOW_TITLE))
        {
        nk_layout_row_dynamic(context, 24.0f, 1);
        nk_label(context, "Battalion map editor", NK_TEXT_LEFT);
        nk_layout_row_dynamic(context, 28.0f, 1);
        nk_button_label(context, "Save");
        }
    nk_end(context);
    mapEditorGuiRender(&gui, 640, 480);
    mapEditorGuiDestroy(&gui);
    if (drawCalls <= 0 || drawVertices <= 0 || scissorDraws <= 0)
        {
        fprintf(stderr,
                "map editor gui produced no clipped draw stream: %d/%d/%d\n",
                drawCalls, drawVertices, scissorDraws);
        return 1;
        }
    return 0;
    }
