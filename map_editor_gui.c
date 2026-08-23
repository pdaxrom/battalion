/*
 * Copyright (c) 2026 Battalion contributors.
 */
/****************************************************************/
/* Minimal Nuklear adapter for Battalion's shared GL frontend.     */
/****************************************************************/

#define NK_IMPLEMENTATION
#include "map_editor_gui.h"

#include "tk.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static float textWidth(nk_handle handle, float height,
                       const char *text, int length)
    {
    (void)handle;
    (void)text;
    return (float)length * height * 0.75f;
    }

static void setColor(struct nk_color color)
    {
    float value[4];

    value[0] = (float)color.r / 255.0f;
    value[1] = (float)color.g / 255.0f;
    value[2] = (float)color.b / 255.0f;
    value[3] = (float)color.a / 255.0f;
    glColor4fv(value);
    }

static void drawLine(float x0, float y0, float x1, float y1,
                     float thickness, struct nk_color color)
    {
    setColor(color);
    glLineWidth(thickness);
    glBegin(GL_LINES);
    glVertex2f(x0, y0);
    glVertex2f(x1, y1);
    glEnd();
    }

static void drawRect(float x, float y, float width, float height,
                     struct nk_color color)
    {
    setColor(color);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
    }

static void drawRectOutline(float x, float y, float width, float height,
                            float thickness, struct nk_color color)
    {
    setColor(color);
    glLineWidth(thickness);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
    }

static void drawCircle(float x, float y, float width, float height,
                       float thickness, struct nk_color color, int filled)
    {
    float centerX;
    float centerY;
    float angle;
    int index;

    centerX = x + width * 0.5f;
    centerY = y + height * 0.5f;
    setColor(color);
    glLineWidth(thickness);
    glBegin(filled ? GL_TRIANGLE_FAN : GL_LINE_LOOP);
    if (filled)
        glVertex2f(centerX, centerY);
    for (index = 0; index < 24; index++)
        {
        angle = (float)(2.0 * M_PI * (double)index / 24.0);
        glVertex2f(centerX + (float)cos((double)angle) * width * 0.5f,
                   centerY + (float)sin((double)angle) * height * 0.5f);
        }
    if (filled)
        glVertex2f(centerX + width * 0.5f, centerY);
    glEnd();
    }

static void drawArc(float centerX, float centerY, float radius,
                    float minimum, float maximum, float thickness,
                    struct nk_color color, int filled)
    {
    float angle;
    int index;

    setColor(color);
    glLineWidth(thickness);
    glBegin(filled ? GL_TRIANGLE_FAN : GL_LINE_STRIP);
    if (filled)
        glVertex2f(centerX, centerY);
    for (index = 0; index <= 24; index++)
        {
        angle = minimum + (maximum - minimum) * (float)index / 24.0f;
        glVertex2f(centerX + (float)cos((double)angle) * radius,
                   centerY + (float)sin((double)angle) * radius);
        }
    glEnd();
    }

static void drawCurve(const struct nk_command_curve *curve)
    {
    float inverse;
    float t;
    float x;
    float y;
    int index;

    setColor(curve->color);
    glLineWidth((float)curve->line_thickness);
    glBegin(GL_LINE_STRIP);
    for (index = 0; index <= 24; index++)
        {
        t = (float)index / 24.0f;
        inverse = 1.0f - t;
        x = inverse * inverse * inverse * curve->begin.x +
            3.0f * inverse * inverse * t * curve->ctrl[0].x +
            3.0f * inverse * t * t * curve->ctrl[1].x +
            t * t * t * curve->end.x;
        y = inverse * inverse * inverse * curve->begin.y +
            3.0f * inverse * inverse * t * curve->ctrl[0].y +
            3.0f * inverse * t * t * curve->ctrl[1].y +
            t * t * t * curve->end.y;
        glVertex2f(x, y);
        }
    glEnd();
    }

static unsigned char fontRow(char character, int row)
    {
    static const unsigned char blank[7] = {0,0,0,0,0,0,0};
    static const unsigned char question[7] = {14,17,1,2,4,0,4};
    static const unsigned char digits[10][7] = {
        {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14},
        {14,17,1,2,4,8,31}, {30,1,1,14,1,1,30},
        {2,6,10,18,31,2,2}, {31,16,30,1,1,17,14},
        {6,8,16,30,17,17,14}, {31,1,2,4,8,8,8},
        {14,17,17,14,17,17,14}, {14,17,17,15,1,2,12}
        };
    static const unsigned char letters[26][7] = {
        {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
        {14,17,16,16,16,17,14}, {30,17,17,17,17,17,30},
        {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
        {14,17,16,23,17,17,14}, {17,17,17,31,17,17,17},
        {14,4,4,4,4,4,14}, {7,2,2,2,18,18,12},
        {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17},
        {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16},
        {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
        {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4},
        {17,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
        {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31}
        };
    static const unsigned char colon[7] = {0,4,4,0,4,4,0};
    static const unsigned char dot[7] = {0,0,0,0,0,12,12};
    static const unsigned char comma[7] = {0,0,0,0,0,4,8};
    static const unsigned char dash[7] = {0,0,0,31,0,0,0};
    static const unsigned char underscore[7] = {0,0,0,0,0,0,31};
    static const unsigned char slash[7] = {1,1,2,4,8,16,16};
    static const unsigned char plus[7] = {0,4,4,31,4,4,0};
    static const unsigned char equals[7] = {0,0,31,0,31,0,0};
    static const unsigned char apostrophe[7] = {4,4,8,0,0,0,0};
    static const unsigned char quote[7] = {10,10,20,0,0,0,0};
    static const unsigned char leftParen[7] = {2,4,8,8,8,4,2};
    static const unsigned char rightParen[7] = {8,4,2,2,2,4,8};
    static const unsigned char leftBracket[7] = {14,8,8,8,8,8,14};
    static const unsigned char rightBracket[7] = {14,2,2,2,2,2,14};
    static const unsigned char lessThan[7] = {1,2,4,8,4,2,1};
    static const unsigned char greaterThan[7] = {16,8,4,2,4,8,16};
    static const unsigned char backslash[7] = {16,16,8,4,2,1,1};
    static const unsigned char hash[7] = {10,31,10,10,31,10,0};
    const unsigned char *glyph;

    if (row < 0 || row >= 7)
        return 0;
    if (character >= 'a' && character <= 'z')
        character = (char)(character - 'a' + 'A');
    glyph = question;
    if (character == ' ') glyph = blank;
    else if (character >= '0' && character <= '9')
        glyph = digits[character - '0'];
    else if (character >= 'A' && character <= 'Z')
        glyph = letters[character - 'A'];
    else if (character == ':') glyph = colon;
    else if (character == '.') glyph = dot;
    else if (character == ',') glyph = comma;
    else if (character == '-') glyph = dash;
    else if (character == '_') glyph = underscore;
    else if (character == '/') glyph = slash;
    else if (character == '+') glyph = plus;
    else if (character == '=') glyph = equals;
    else if (character == '\'') glyph = apostrophe;
    else if (character == '"') glyph = quote;
    else if (character == '(') glyph = leftParen;
    else if (character == ')') glyph = rightParen;
    else if (character == '[') glyph = leftBracket;
    else if (character == ']') glyph = rightBracket;
    else if (character == '<') glyph = lessThan;
    else if (character == '>') glyph = greaterThan;
    else if (character == '\\') glyph = backslash;
    else if (character == '#') glyph = hash;
    return glyph[row];
    }

static void drawText(const struct nk_command_text *text)
    {
    unsigned char bits;
    float scale;
    int character;
    int row;
    int column;

    setColor(text->foreground);
    scale = text->height / 8.0f;
    glBegin(GL_QUADS);
    for (character = 0; character < text->length; character++)
        for (row = 0; row < 7; row++)
            {
            bits = fontRow(text->string[character], row);
            for (column = 0; column < 5; column++)
                if (bits & (unsigned char)(1U << (4 - column)))
                    {
                    float x = text->x + character * scale * 6.0f +
                              column * scale;
                    float y = text->y + row * scale + 1.0f;
                    glVertex2f(x, y);
                    glVertex2f(x + scale, y);
                    glVertex2f(x + scale, y + scale);
                    glVertex2f(x, y + scale);
                    }
            }
    glEnd();
    }

int mapEditorGuiInitialize(struct MapEditorGui *gui)
    {
    memset(gui, 0, sizeof(*gui));
    gui->font.userdata = nk_handle_ptr(NULL);
    gui->font.height = MAP_EDITOR_GUI_FONT_DEVICE_PIXELS / tkDisplayScale();
    gui->font.width = textWidth;
    if (!nk_init_default(&gui->context, &gui->font))
        return 0;
    gui->initialized = 1;
    return 1;
    }

void mapEditorGuiDestroy(struct MapEditorGui *gui)
    {
    if (gui == NULL || !gui->initialized)
        return;
    nk_free(&gui->context);
    memset(gui, 0, sizeof(*gui));
    }

void mapEditorGuiInput(struct MapEditorGui *gui,
                       const struct MapEditorGuiInput *input)
    {
    int index;

    if (gui == NULL || !gui->initialized || input == NULL)
        return;
    nk_input_begin(&gui->context);
    nk_input_motion(&gui->context, input->mouseX, input->mouseY);
    nk_input_button(&gui->context, NK_BUTTON_LEFT,
                    input->mouseX, input->mouseY, input->mouseLeft);
    nk_input_button(&gui->context, NK_BUTTON_RIGHT,
                    input->mouseX, input->mouseY, input->mouseRight);
    nk_input_button(&gui->context, NK_BUTTON_MIDDLE,
                    input->mouseX, input->mouseY, input->mouseMiddle);
    nk_input_key(&gui->context, NK_KEY_SHIFT, input->shift);
    nk_input_key(&gui->context, NK_KEY_CTRL, input->control);
    nk_input_key(&gui->context, NK_KEY_LEFT, input->left);
    nk_input_key(&gui->context, NK_KEY_RIGHT, input->right);
    nk_input_key(&gui->context, NK_KEY_BACKSPACE, input->backspace);
    nk_input_key(&gui->context, NK_KEY_DEL, input->deleteKey);
    nk_input_key(&gui->context, NK_KEY_ENTER, input->enter);
    nk_input_key(&gui->context, NK_KEY_TAB, input->tab);
    nk_input_key(&gui->context, NK_KEY_TEXT_START, input->home);
    nk_input_key(&gui->context, NK_KEY_TEXT_END, input->end);
    for (index = 0; index < input->textLength; index++)
        nk_input_unicode(&gui->context, (nk_rune)input->text[index]);
    if (input->wheel != 0.0f)
        nk_input_scroll(&gui->context, nk_vec2(0.0f, input->wheel));
    nk_input_end(&gui->context);
    }

int mapEditorGuiWantsMouse(const struct MapEditorGui *gui)
    {
    if (gui == NULL || !gui->initialized)
        return 0;
    return nk_window_is_any_hovered(&gui->context) ||
           nk_item_is_any_active(&gui->context);
    }

static void renderPolygon(const struct nk_vec2i *points,
                          unsigned short count, float thickness,
                          struct nk_color color, int filled, int closed)
    {
    unsigned short index;

    if (count == 0)
        return;
    setColor(color);
    glLineWidth(thickness);
    glBegin(filled ? GL_TRIANGLE_FAN : closed ? GL_LINE_LOOP : GL_LINE_STRIP);
    for (index = 0; index < count; index++)
        glVertex2f(points[index].x, points[index].y);
    glEnd();
    }

void mapEditorGuiRender(struct MapEditorGui *gui, int width, int height)
    {
    const struct nk_command *command;

    if (gui == NULL || !gui->initialized || width <= 0 || height <= 0)
        return;
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, (double)width, (double)height, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    nk_foreach(command, &gui->context)
        {
        if (command->type == NK_COMMAND_SCISSOR)
            {
            const struct nk_command_scissor *value;
            value = (const struct nk_command_scissor *)command;
            glEnable(GL_SCISSOR_TEST);
            glScissor(value->x, height - value->y - value->h,
                      value->w, value->h);
            }
        else if (command->type == NK_COMMAND_LINE)
            {
            const struct nk_command_line *value;
            value = (const struct nk_command_line *)command;
            drawLine(value->begin.x, value->begin.y,
                     value->end.x, value->end.y,
                     value->line_thickness, value->color);
            }
        else if (command->type == NK_COMMAND_CURVE)
            drawCurve((const struct nk_command_curve *)command);
        else if (command->type == NK_COMMAND_RECT)
            {
            const struct nk_command_rect *value;
            value = (const struct nk_command_rect *)command;
            drawRectOutline(value->x, value->y, value->w, value->h,
                            value->line_thickness, value->color);
            }
        else if (command->type == NK_COMMAND_RECT_FILLED)
            {
            const struct nk_command_rect_filled *value;
            value = (const struct nk_command_rect_filled *)command;
            drawRect(value->x, value->y, value->w, value->h, value->color);
            }
        else if (command->type == NK_COMMAND_RECT_MULTI_COLOR)
            {
            const struct nk_command_rect_multi_color *value;
            value = (const struct nk_command_rect_multi_color *)command;
            drawRect(value->x, value->y, value->w, value->h, value->left);
            }
        else if (command->type == NK_COMMAND_CIRCLE ||
                 command->type == NK_COMMAND_CIRCLE_FILLED)
            {
            if (command->type == NK_COMMAND_CIRCLE)
                {
                const struct nk_command_circle *value;
                value = (const struct nk_command_circle *)command;
                drawCircle(value->x, value->y, value->w, value->h,
                           value->line_thickness, value->color, 0);
                }
            else
                {
                const struct nk_command_circle_filled *value;
                value = (const struct nk_command_circle_filled *)command;
                drawCircle(value->x, value->y, value->w, value->h,
                           1.0f, value->color, 1);
                }
            }
        else if (command->type == NK_COMMAND_ARC ||
                 command->type == NK_COMMAND_ARC_FILLED)
            {
            if (command->type == NK_COMMAND_ARC)
                {
                const struct nk_command_arc *value;
                value = (const struct nk_command_arc *)command;
                drawArc(value->cx, value->cy, value->r,
                        value->a[0], value->a[1], value->line_thickness,
                        value->color, 0);
                }
            else
                {
                const struct nk_command_arc_filled *value;
                value = (const struct nk_command_arc_filled *)command;
                drawArc(value->cx, value->cy, value->r,
                        value->a[0], value->a[1], 1.0f, value->color, 1);
                }
            }
        else if (command->type == NK_COMMAND_TRIANGLE ||
                 command->type == NK_COMMAND_TRIANGLE_FILLED)
            {
            if (command->type == NK_COMMAND_TRIANGLE)
                {
                const struct nk_command_triangle *value;
                struct nk_vec2i points[3];
                value = (const struct nk_command_triangle *)command;
                points[0] = value->a; points[1] = value->b;
                points[2] = value->c;
                renderPolygon(points, 3, value->line_thickness,
                              value->color, 0, 1);
                }
            else
                {
                const struct nk_command_triangle_filled *value;
                struct nk_vec2i points[3];
                value = (const struct nk_command_triangle_filled *)command;
                points[0] = value->a; points[1] = value->b;
                points[2] = value->c;
                renderPolygon(points, 3, 1.0f, value->color, 1, 1);
                }
            }
        else if (command->type == NK_COMMAND_POLYGON)
            {
            const struct nk_command_polygon *value;
            value = (const struct nk_command_polygon *)command;
            renderPolygon(value->points, value->point_count,
                          value->line_thickness, value->color, 0, 1);
            }
        else if (command->type == NK_COMMAND_POLYGON_FILLED)
            {
            const struct nk_command_polygon_filled *value;
            value = (const struct nk_command_polygon_filled *)command;
            renderPolygon(value->points, value->point_count,
                          1.0f, value->color, 1, 1);
            }
        else if (command->type == NK_COMMAND_POLYLINE)
            {
            const struct nk_command_polyline *value;
            value = (const struct nk_command_polyline *)command;
            renderPolygon(value->points, value->point_count,
                          value->line_thickness, value->color, 0, 0);
            }
        else if (command->type == NK_COMMAND_TEXT)
            drawText((const struct nk_command_text *)command);
        }
    glDisable(GL_SCISSOR_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();
    nk_clear(&gui->context);
    }
