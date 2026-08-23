/*
 * Copyright (c) 2026 Battalion contributors.
 */
#include "fixed_gl.h"
#include "backend.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MATRIX_STACK_SIZE 64
#define ATTRIB_STACK_SIZE 32

typedef struct MatrixStack {
    float value[MATRIX_STACK_SIZE][16];
    int top;
} MatrixStack;

typedef struct FixedState {
    float color[4];
    float texcoord[2];
    float clearColor[4];
    float clearDepth;
    float lineWidth;
    float pointSize;
    int viewport[4];
    int scissor[4];
    GLenum shadeModel;
    GLenum polygonMode;
    GLuint listBase;
    unsigned int texture;
    GLuint boundTexture;
    int textureEnabled;
    int textureLinear;
    int textureRepeat;
    int depthTest;
    int depthWrite;
    int blending;
    int cullFace;
    int fog;
    int lighting;
    int alphaTest;
    int scissorTest;
} FixedState;

enum CommandOp {
    CMD_BEGIN, CMD_END, CMD_VERTEX2, CMD_VERTEX3, CMD_COLOR3, CMD_COLOR4,
    CMD_TEXCOORD, CMD_MATRIX_MODE, CMD_LOAD_IDENTITY, CMD_PUSH_MATRIX,
    CMD_POP_MATRIX, CMD_TRANSLATE, CMD_ROTATE, CMD_SCALE, CMD_ORTHO,
    CMD_MULT_MATRIX,
    CMD_ENABLE, CMD_DISABLE, CMD_BLEND_FUNC, CMD_CULL_FACE, CMD_DEPTH_FUNC,
    CMD_DEPTH_MASK, CMD_DEPTH_RANGE, CMD_SHADE_MODEL, CMD_POLYGON_MODE, CMD_LINE_WIDTH,
    CMD_POINT_SIZE, CMD_FOGI, CMD_FOGF, CMD_HINT, CMD_PUSH_ATTRIB,
    CMD_POP_ATTRIB,
    CMD_CLEAR_COLOR, CMD_CLEAR_DEPTH, CMD_CLEAR, CMD_VIEWPORT, CMD_SCISSOR,
    CMD_CALL_LIST, CMD_LIST_BASE, CMD_BIND_TEXTURE, CMD_TEX_IMAGE, CMD_TEX_PARAMETER,
    CMD_TEX_ENV
};

typedef struct Command {
    enum CommandOp op;
    GLint i[8];
    GLfloat f[16];
    unsigned char *data;
    size_t dataSize;
    unsigned int cachedTexture;
} Command;

typedef struct DisplayList {
    Command *commands;
    size_t count;
    size_t capacity;
} DisplayList;

struct GLUquadric {
    int unused;
};

static int initialized;
static FixedState state;
static FixedState attribStack[ATTRIB_STACK_SIZE];
static int attribTop;
static MatrixStack modelview;
static MatrixStack projection;
static GLenum matrixMode;
static RendererVertex *vertices;
static size_t vertexCount;
static size_t vertexCapacity;
static GLenum primitiveMode;
static int insideBegin;
static DisplayList **displayLists;
static size_t displayListCapacity;
static GLuint nextList = 1;
static unsigned int *textureObjects;
static size_t textureObjectCapacity;
static GLuint nextTextureObject = 1;
static DisplayList *recordingList;
static int replayDepth;
static GLenum lastError = GL_NO_ERROR;


static void matrixIdentity(float *m)
{
    memset(m, 0, sizeof(float) * 16);
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

static void matrixMultiply(float *out, const float *a, const float *b)
{
    float result[16];
    int row, column, k;

    for (column = 0; column < 4; ++column) {
        for (row = 0; row < 4; ++row) {
            result[column * 4 + row] = 0.0f;
            for (k = 0; k < 4; ++k)
                result[column * 4 + row] += a[k * 4 + row] * b[column * 4 + k];
        }
    }
    memcpy(out, result, sizeof(result));
}

static MatrixStack *currentMatrixStack(void)
{
    return matrixMode == GL_PROJECTION ? &projection : &modelview;
}

static void multiplyCurrent(const float *m)
{
    MatrixStack *stack = currentMatrixStack();
    matrixMultiply(stack->value[stack->top], stack->value[stack->top], m);
}

static void initializeState(void)
{
    if (initialized)
        return;

    memset(&state, 0, sizeof(state));
    state.color[0] = state.color[1] = state.color[2] = state.color[3] = 1.0f;
    state.clearColor[3] = 1.0f;
    state.clearDepth = 1.0f;
    state.lineWidth = state.pointSize = 1.0f;
    state.viewport[2] = 1;
    state.viewport[3] = 1;
    state.shadeModel = GL_SMOOTH;
    state.polygonMode = GL_FILL;
    state.depthWrite = 1;
    state.textureLinear = 0;
    state.textureRepeat = 0;
    matrixMode = GL_MODELVIEW;
    matrixIdentity(modelview.value[0]);
    matrixIdentity(projection.value[0]);
    initialized = 1;
}

static int isRecording(void)
{
    return recordingList != NULL && replayDepth == 0;
}

static Command *appendCommand(enum CommandOp op)
{
    Command *command;
    size_t capacity;

    if (!recordingList)
        return NULL;
    if (recordingList->count == recordingList->capacity) {
        capacity = recordingList->capacity ? recordingList->capacity * 2 : 64;
        recordingList->commands = (Command *)realloc(recordingList->commands,
                                                       capacity * sizeof(Command));
        recordingList->capacity = capacity;
    }
    command = &recordingList->commands[recordingList->count++];
    memset(command, 0, sizeof(*command));
    command->op = op;
    return command;
}

static void freeDisplayList(DisplayList *list)
{
    size_t i;
    if (!list)
        return;
    for (i = 0; i < list->count; ++i)
        free(list->commands[i].data);
    free(list->commands);
    free(list);
}

static void ensureDisplayListCapacity(GLuint id)
{
    size_t capacity, oldCapacity;
    if (id < displayListCapacity)
        return;
    oldCapacity = displayListCapacity;
    capacity = oldCapacity ? oldCapacity : 256;
    while (id >= capacity)
        capacity *= 2;
    displayLists = (DisplayList **)realloc(displayLists, capacity * sizeof(DisplayList *));
    memset(displayLists + oldCapacity, 0, (capacity - oldCapacity) * sizeof(DisplayList *));
    displayListCapacity = capacity;
}

static void ensureTextureObjectCapacity(GLuint id)
{
    size_t capacity, oldCapacity;
    if (id < textureObjectCapacity)
        return;
    oldCapacity = textureObjectCapacity;
    capacity = oldCapacity ? oldCapacity : 256;
    while (id >= capacity)
        capacity *= 2;
    textureObjects = (unsigned int *)realloc(
        textureObjects, capacity * sizeof(*textureObjects));
    memset(textureObjects + oldCapacity, 0,
           (capacity - oldCapacity) * sizeof(*textureObjects));
    textureObjectCapacity = capacity;
}

static void ensureVertexCapacity(size_t count)
{
    size_t capacity;
    if (count <= vertexCapacity)
        return;
    capacity = vertexCapacity ? vertexCapacity * 2 : 256;
    while (capacity < count)
        capacity *= 2;
    vertices = (RendererVertex *)realloc(vertices, capacity * sizeof(RendererVertex));
    vertexCapacity = capacity;
}

static void appendVertex(float x, float y, float z)
{
    RendererVertex *vertex;
    if (!insideBegin)
        return;
    ensureVertexCapacity(vertexCount + 1);
    vertex = &vertices[vertexCount++];
    vertex->position[0] = x;
    vertex->position[1] = y;
    vertex->position[2] = z;
    memcpy(vertex->color, state.color, sizeof(vertex->color));
    memcpy(vertex->texcoord, state.texcoord, sizeof(vertex->texcoord));
}

static void copyVertex(RendererVertex *destination, const RendererVertex *source)
{
    memcpy(destination, source, sizeof(*destination));
}

static void emitImmediate(void)
{
    RendererVertex *output;
    size_t outputCount, i, quadCount;
    enum RendererPrimitive backendPrimitive;
    RendererDrawState drawState;

    if (!vertexCount)
        return;

    output = NULL;
    outputCount = 0;
    backendPrimitive = RENDERER_TRIANGLES;

    if (state.polygonMode == GL_POINT && primitiveMode >= GL_TRIANGLES) {
        outputCount = vertexCount;
        output = (RendererVertex *)malloc(outputCount * sizeof(RendererVertex));
        memcpy(output, vertices, outputCount * sizeof(RendererVertex));
        backendPrimitive = RENDERER_POINTS;
    } else if (primitiveMode == GL_POINTS) {
        outputCount = vertexCount;
        output = (RendererVertex *)malloc(outputCount * sizeof(RendererVertex));
        memcpy(output, vertices, outputCount * sizeof(RendererVertex));
        backendPrimitive = RENDERER_POINTS;
    } else if (primitiveMode == GL_LINES) {
        outputCount = vertexCount - (vertexCount % 2);
        output = (RendererVertex *)malloc(outputCount * sizeof(RendererVertex));
        memcpy(output, vertices, outputCount * sizeof(RendererVertex));
        backendPrimitive = RENDERER_LINES;
    } else if (primitiveMode == GL_LINE_STRIP || primitiveMode == GL_LINE_LOOP) {
        size_t segmentCount = primitiveMode == GL_LINE_LOOP ? vertexCount : vertexCount - 1;
        outputCount = segmentCount * 2;
        output = (RendererVertex *)malloc(outputCount * sizeof(RendererVertex));
        for (i = 0; i < segmentCount; ++i) {
            copyVertex(&output[i * 2], &vertices[i]);
            copyVertex(&output[i * 2 + 1], &vertices[(i + 1) % vertexCount]);
        }
        backendPrimitive = RENDERER_LINES;
    } else if (primitiveMode == GL_TRIANGLES) {
        outputCount = vertexCount - (vertexCount % 3);
        output = (RendererVertex *)malloc(outputCount * sizeof(RendererVertex));
        memcpy(output, vertices, outputCount * sizeof(RendererVertex));
    } else if (primitiveMode == GL_TRIANGLE_STRIP) {
        outputCount = vertexCount >= 3 ? (vertexCount - 2) * 3 : 0;
        output = (RendererVertex *)malloc(outputCount * sizeof(RendererVertex));
        for (i = 0; i + 2 < vertexCount; ++i) {
            size_t a = i;
            size_t b = (i & 1) ? i + 2 : i + 1;
            size_t c = (i & 1) ? i + 1 : i + 2;
            copyVertex(&output[i * 3], &vertices[a]);
            copyVertex(&output[i * 3 + 1], &vertices[b]);
            copyVertex(&output[i * 3 + 2], &vertices[c]);
        }
    } else if (primitiveMode == GL_TRIANGLE_FAN) {
        outputCount = vertexCount >= 3 ? (vertexCount - 2) * 3 : 0;
        output = (RendererVertex *)malloc(outputCount * sizeof(RendererVertex));
        for (i = 0; i + 2 < vertexCount; ++i) {
            copyVertex(&output[i * 3], &vertices[0]);
            copyVertex(&output[i * 3 + 1], &vertices[i + 1]);
            copyVertex(&output[i * 3 + 2], &vertices[i + 2]);
        }
    } else if (primitiveMode == GL_QUADS) {
        quadCount = vertexCount / 4;
        outputCount = quadCount * 6;
        output = (RendererVertex *)malloc(outputCount * sizeof(RendererVertex));
        for (i = 0; i < quadCount; ++i) {
            RendererVertex *q = &vertices[i * 4];
            RendererVertex *o = &output[i * 6];
            copyVertex(&o[0], &q[0]); copyVertex(&o[1], &q[1]); copyVertex(&o[2], &q[2]);
            copyVertex(&o[3], &q[0]); copyVertex(&o[4], &q[2]); copyVertex(&o[5], &q[3]);
        }
    } else if (primitiveMode == GL_QUAD_STRIP) {
        quadCount = vertexCount >= 4 ? vertexCount / 2 - 1 : 0;
        outputCount = quadCount * 6;
        output = (RendererVertex *)malloc(outputCount * sizeof(RendererVertex));
        for (i = 0; i < quadCount; ++i) {
            RendererVertex *q = &vertices[i * 2];
            RendererVertex *o = &output[i * 6];
            copyVertex(&o[0], &q[0]); copyVertex(&o[1], &q[1]); copyVertex(&o[2], &q[3]);
            copyVertex(&o[3], &q[0]); copyVertex(&o[4], &q[3]); copyVertex(&o[5], &q[2]);
        }
    }

    if (outputCount) {
        memset(&drawState, 0, sizeof(drawState));
        matrixMultiply(drawState.mvp, projection.value[projection.top],
                       modelview.value[modelview.top]);
        memcpy(drawState.viewport, state.viewport, sizeof(state.viewport));
        memcpy(drawState.scissor, state.scissor, sizeof(state.scissor));
        drawState.scissorEnabled = state.scissorTest;
        drawState.texture = state.texture;
        drawState.textureEnabled = state.textureEnabled;
        drawState.textureLinear = state.textureLinear;
        drawState.textureRepeat = state.textureRepeat;
        drawState.depthTest = state.depthTest;
        drawState.depthWrite = state.depthWrite;
        drawState.blending = state.blending;
        drawState.cullBackFaces = state.cullFace;
        drawState.wireframe = state.polygonMode == GL_LINE;
        drawState.pointSize = state.pointSize;
        rendererBackendDraw(backendPrimitive, output, outputCount, &drawState);
    }
    free(output);
}


void glBegin(GLenum mode)
{
    Command *command;
    initializeState();
    if (isRecording()) {
        command = appendCommand(CMD_BEGIN);
        command->i[0] = (GLint)mode;
        return;
    }
    primitiveMode = mode;
    vertexCount = 0;
    insideBegin = 1;
}

void glEnd(void)
{
    if (isRecording()) {
        appendCommand(CMD_END);
        return;
    }
    if (!insideBegin)
        return;
    insideBegin = 0;
    emitImmediate();
}

void glVertex2f(GLfloat x, GLfloat y)
{
    Command *command;
    if (isRecording()) {
        command = appendCommand(CMD_VERTEX2);
        command->f[0] = x; command->f[1] = y;
        return;
    }
    appendVertex(x, y, 0.0f);
}

void glVertex3fv(const GLfloat *v)
{
    Command *command;
    if (isRecording()) {
        command = appendCommand(CMD_VERTEX3);
        memcpy(command->f, v, sizeof(float) * 3);
        return;
    }
    appendVertex(v[0], v[1], v[2]);
}

void glColor3fv(const GLfloat *v)
{
    Command *command;
    if (isRecording()) {
        command = appendCommand(CMD_COLOR3);
        memcpy(command->f, v, sizeof(float) * 3);
        return;
    }
    memcpy(state.color, v, sizeof(float) * 3);
    state.color[3] = 1.0f;
}

void glColor4fv(const GLfloat *v)
{
    Command *command;
    if (isRecording()) {
        command = appendCommand(CMD_COLOR4);
        memcpy(command->f, v, sizeof(float) * 4);
        return;
    }
    memcpy(state.color, v, sizeof(float) * 4);
}

void glTexCoord2fv(const GLfloat *v)
{
    Command *command;
    if (isRecording()) {
        command = appendCommand(CMD_TEXCOORD);
        memcpy(command->f, v, sizeof(float) * 2);
        return;
    }
    memcpy(state.texcoord, v, sizeof(float) * 2);
}

void glIndexf(GLfloat value)
{
    float color[3];
    color[0] = color[1] = color[2] = value;
    glColor3fv(color);
}

void glMatrixMode(GLenum mode)
{
    Command *command;
    if (isRecording()) {
        command = appendCommand(CMD_MATRIX_MODE); command->i[0] = (GLint)mode; return;
    }
    matrixMode = mode;
}

void glLoadIdentity(void)
{
    if (isRecording()) { appendCommand(CMD_LOAD_IDENTITY); return; }
    matrixIdentity(currentMatrixStack()->value[currentMatrixStack()->top]);
}

void glPushMatrix(void)
{
    MatrixStack *stack;
    if (isRecording()) { appendCommand(CMD_PUSH_MATRIX); return; }
    stack = currentMatrixStack();
    if (stack->top + 1 < MATRIX_STACK_SIZE) {
        memcpy(stack->value[stack->top + 1], stack->value[stack->top], sizeof(float) * 16);
        ++stack->top;
    }
}

void glPopMatrix(void)
{
    if (isRecording()) { appendCommand(CMD_POP_MATRIX); return; }
    if (currentMatrixStack()->top > 0)
        --currentMatrixStack()->top;
}

void glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
    float m[16]; Command *command;
    if (isRecording()) {
        command = appendCommand(CMD_TRANSLATE);
        command->f[0] = x; command->f[1] = y; command->f[2] = z; return;
    }
    matrixIdentity(m); m[12] = x; m[13] = y; m[14] = z; multiplyCurrent(m);
}

void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    float m[16], length, c, s, t; Command *command;
    if (isRecording()) {
        command = appendCommand(CMD_ROTATE);
        command->f[0] = angle; command->f[1] = x; command->f[2] = y; command->f[3] = z; return;
    }
    length = (float)sqrt(x * x + y * y + z * z);
    if (length == 0.0f) return;
    x /= length; y /= length; z /= length;
    c = (float)cos(angle * M_PI / 180.0); s = (float)sin(angle * M_PI / 180.0); t = 1.0f - c;
    matrixIdentity(m);
    m[0] = t*x*x+c; m[4] = t*x*y-s*z; m[8] = t*x*z+s*y;
    m[1] = t*x*y+s*z; m[5] = t*y*y+c; m[9] = t*y*z-s*x;
    m[2] = t*x*z-s*y; m[6] = t*y*z+s*x; m[10] = t*z*z+c;
    multiplyCurrent(m);
}

void glScalef(GLfloat x, GLfloat y, GLfloat z)
{
    float m[16]; Command *command;
    if (isRecording()) {
        command = appendCommand(CMD_SCALE);
        command->f[0] = x; command->f[1] = y; command->f[2] = z; return;
    }
    matrixIdentity(m); m[0] = x; m[5] = y; m[10] = z; multiplyCurrent(m);
}

void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top,
             GLdouble nearValue, GLdouble farValue)
{
    float m[16]; Command *command;
    if (isRecording()) {
        command = appendCommand(CMD_ORTHO);
        command->f[0]=(float)left; command->f[1]=(float)right; command->f[2]=(float)bottom;
        command->f[3]=(float)top; command->f[4]=(float)nearValue; command->f[5]=(float)farValue; return;
    }
    matrixIdentity(m);
    m[0] = (float)(2.0 / (right-left)); m[5] = (float)(2.0 / (top-bottom));
    m[10] = (float)(-2.0 / (farValue-nearValue));
    m[12] = (float)(-(right+left)/(right-left)); m[13] = (float)(-(top+bottom)/(top-bottom));
    m[14] = (float)(-(farValue+nearValue)/(farValue-nearValue));
    multiplyCurrent(m);
}

static void setCapability(GLenum capability, int enabled)
{
    switch (capability) {
      case GL_TEXTURE_2D: state.textureEnabled = enabled; break;
      case GL_DEPTH_TEST: state.depthTest = enabled; break;
      case GL_BLEND: state.blending = enabled; break;
      case GL_CULL_FACE: state.cullFace = enabled; break;
      case GL_FOG: state.fog = enabled; break;
      case GL_LIGHTING: state.lighting = enabled; break;
      case GL_ALPHA_TEST: state.alphaTest = enabled; break;
      case GL_SCISSOR_TEST: state.scissorTest = enabled; break;
      default: break;
    }
}

void glEnable(GLenum capability)
{
    Command *command;
    if (isRecording()) { command=appendCommand(CMD_ENABLE); command->i[0]=(GLint)capability; return; }
    setCapability(capability, 1);
}

void glDisable(GLenum capability)
{
    Command *command;
    if (isRecording()) { command=appendCommand(CMD_DISABLE); command->i[0]=(GLint)capability; return; }
    setCapability(capability, 0);
}

GLboolean glIsEnabled(GLenum capability)
{
    switch (capability) {
      case GL_TEXTURE_2D: return (GLboolean)state.textureEnabled;
      case GL_DEPTH_TEST: return (GLboolean)state.depthTest;
      case GL_BLEND: return (GLboolean)state.blending;
      case GL_CULL_FACE: return (GLboolean)state.cullFace;
      case GL_FOG: return (GLboolean)state.fog;
      case GL_LIGHTING: return (GLboolean)state.lighting;
      case GL_ALPHA_TEST: return (GLboolean)state.alphaTest;
      case GL_SCISSOR_TEST: return (GLboolean)state.scissorTest;
      default: return GL_FALSE;
    }
}

#define RECORD_ENUM1(op, value) do { Command *c=appendCommand(op); c->i[0]=(GLint)(value); } while (0)

void glBlendFunc(GLenum source, GLenum destination)
{
    Command *c;
    if (isRecording()) { c=appendCommand(CMD_BLEND_FUNC); c->i[0]=(GLint)source; c->i[1]=(GLint)destination; }
}
void glCullFace(GLenum mode) { if (isRecording()) RECORD_ENUM1(CMD_CULL_FACE, mode); }
void glDepthFunc(GLenum function) { if (isRecording()) RECORD_ENUM1(CMD_DEPTH_FUNC, function); }
void glDepthMask(GLboolean enabled)
{
    Command *c;
    if (isRecording()) {
        c=appendCommand(CMD_DEPTH_MASK);
        c->i[0]=(GLint)enabled;
        return;
    }
    state.depthWrite=enabled != GL_FALSE;
}
void glDepthRange(GLclampd nearValue, GLclampd farValue)
{
    Command *c; if (isRecording()) { c=appendCommand(CMD_DEPTH_RANGE); c->f[0]=(float)nearValue; c->f[1]=(float)farValue; }
}
void glShadeModel(GLenum mode)
{
    if (isRecording()) { RECORD_ENUM1(CMD_SHADE_MODEL, mode); return; } state.shadeModel=mode;
}
void glPolygonMode(GLenum face, GLenum mode)
{
    Command *c; if (isRecording()) { c=appendCommand(CMD_POLYGON_MODE); c->i[0]=(GLint)face; c->i[1]=(GLint)mode; return; } state.polygonMode=mode;
}
void glLineWidth(GLfloat width)
{
    Command *c; if (isRecording()) { c=appendCommand(CMD_LINE_WIDTH); c->f[0]=width; return; } state.lineWidth=width;
}
void glPointSize(GLfloat size)
{
    Command *c; if (isRecording()) { c=appendCommand(CMD_POINT_SIZE); c->f[0]=size; return; } state.pointSize=size;
}
void glFogi(GLenum name, GLint value)
{
    Command *c; if (isRecording()) { c=appendCommand(CMD_FOGI); c->i[0]=(GLint)name; c->i[1]=value; }
}
void glFogf(GLenum name, GLfloat value)
{
    Command *c; if (isRecording()) { c=appendCommand(CMD_FOGF); c->i[0]=(GLint)name; c->f[0]=value; }
}
void glHint(GLenum target, GLenum mode)
{
    Command *c; if (isRecording()) { c=appendCommand(CMD_HINT); c->i[0]=(GLint)target; c->i[1]=(GLint)mode; }
}
void glPushAttrib(GLbitfield mask)
{
    Command *c; if (isRecording()) { c=appendCommand(CMD_PUSH_ATTRIB); c->i[0]=(GLint)mask; return; }
    if (attribTop < ATTRIB_STACK_SIZE) attribStack[attribTop++]=state;
}
void glPopAttrib(void)
{
    if (isRecording()) { appendCommand(CMD_POP_ATTRIB); return; }
    if (attribTop > 0) state=attribStack[--attribTop];
}

void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    Command *c; if (isRecording()) { c=appendCommand(CMD_CLEAR_COLOR); c->f[0]=red;c->f[1]=green;c->f[2]=blue;c->f[3]=alpha; return; }
    state.clearColor[0]=red;state.clearColor[1]=green;state.clearColor[2]=blue;state.clearColor[3]=alpha;
}
void glClearDepth(GLclampd depth)
{
    Command *c; if (isRecording()) { c=appendCommand(CMD_CLEAR_DEPTH); c->f[0]=(float)depth; return; } state.clearDepth=(float)depth;
}
void glClear(GLbitfield buffers)
{
    if (isRecording()) { RECORD_ENUM1(CMD_CLEAR, buffers); return; }
    rendererBackendClear(buffers, state.clearColor, state.clearDepth);
}
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    Command *c;
    initializeState();
    if (isRecording()) { c=appendCommand(CMD_VIEWPORT); c->i[0]=x;c->i[1]=y;c->i[2]=width;c->i[3]=height; return; }
    state.viewport[0]=x;state.viewport[1]=y;state.viewport[2]=width;state.viewport[3]=height;
}
void glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
    Command *c;
    if (isRecording())
        {
        c=appendCommand(CMD_SCISSOR);
        c->i[0]=x;c->i[1]=y;c->i[2]=width;c->i[3]=height;
        return;
        }
    state.scissor[0]=x;state.scissor[1]=y;
    state.scissor[2]=width;state.scissor[3]=height;
}
void glGetIntegerv(GLenum name, GLint *value)
{
    initializeState();
    switch (name) {
      case GL_VIEWPORT: memcpy(value,state.viewport,sizeof(state.viewport)); break;
      case GL_DEPTH_BITS: *value=32; break;
      case GL_RED_BITS: *value=8; break;
      case GL_STEREO: *value=GL_FALSE; break;
      case GL_SHADE_MODEL: *value=(GLint)state.shadeModel; break;
      case GL_TEXTURE_BINDING_2D: *value=(GLint)state.boundTexture; break;
      default: *value=0; break;
    }
}
void glGetFloatv(GLenum name, GLfloat *value)
{
    if (name == GL_LINE_WIDTH)
        *value = state.lineWidth;
    else if (name == GL_POINT_SIZE)
        *value = state.pointSize;
    else
        *value = 0.0f;
}
GLenum glGetError(void) { GLenum result=lastError; lastError=GL_NO_ERROR; return result; }
void glFlush(void) {}
void glFinish(void) {}

GLuint glGenLists(GLsizei range)
{
    GLuint base=nextList; nextList+=(GLuint)range; ensureDisplayListCapacity(nextList); return base;
}

void glNewList(GLuint list, GLenum mode)
{
    (void)mode; initializeState(); ensureDisplayListCapacity(list);
    freeDisplayList(displayLists[list]);
    displayLists[list]=(DisplayList *)calloc(1,sizeof(DisplayList));
    recordingList=displayLists[list];
}

void glEndList(void) { recordingList=NULL; }

static void executeCommand(Command *c)
{
    switch (c->op) {
      case CMD_BEGIN: glBegin((GLenum)c->i[0]); break;
      case CMD_END: glEnd(); break;
      case CMD_VERTEX2: glVertex2f(c->f[0],c->f[1]); break;
      case CMD_VERTEX3: glVertex3fv(c->f); break;
      case CMD_COLOR3: glColor3fv(c->f); break;
      case CMD_COLOR4: glColor4fv(c->f); break;
      case CMD_TEXCOORD: glTexCoord2fv(c->f); break;
      case CMD_MATRIX_MODE: glMatrixMode((GLenum)c->i[0]); break;
      case CMD_LOAD_IDENTITY: glLoadIdentity(); break;
      case CMD_PUSH_MATRIX: glPushMatrix(); break;
      case CMD_POP_MATRIX: glPopMatrix(); break;
      case CMD_TRANSLATE: glTranslatef(c->f[0],c->f[1],c->f[2]); break;
      case CMD_ROTATE: glRotatef(c->f[0],c->f[1],c->f[2],c->f[3]); break;
      case CMD_SCALE: glScalef(c->f[0],c->f[1],c->f[2]); break;
      case CMD_ORTHO: glOrtho(c->f[0],c->f[1],c->f[2],c->f[3],c->f[4],c->f[5]); break;
      case CMD_MULT_MATRIX: multiplyCurrent(c->f); break;
      case CMD_ENABLE: glEnable((GLenum)c->i[0]); break;
      case CMD_DISABLE: glDisable((GLenum)c->i[0]); break;
      case CMD_BLEND_FUNC: glBlendFunc((GLenum)c->i[0],(GLenum)c->i[1]); break;
      case CMD_CULL_FACE: glCullFace((GLenum)c->i[0]); break;
      case CMD_DEPTH_FUNC: glDepthFunc((GLenum)c->i[0]); break;
      case CMD_DEPTH_MASK: glDepthMask((GLboolean)c->i[0]); break;
      case CMD_DEPTH_RANGE: glDepthRange(c->f[0],c->f[1]); break;
      case CMD_SHADE_MODEL: glShadeModel((GLenum)c->i[0]); break;
      case CMD_POLYGON_MODE: glPolygonMode((GLenum)c->i[0],(GLenum)c->i[1]); break;
      case CMD_LINE_WIDTH: glLineWidth(c->f[0]); break;
      case CMD_POINT_SIZE: glPointSize(c->f[0]); break;
      case CMD_FOGI: glFogi((GLenum)c->i[0],c->i[1]); break;
      case CMD_FOGF: glFogf((GLenum)c->i[0],c->f[0]); break;
      case CMD_HINT: glHint((GLenum)c->i[0],(GLenum)c->i[1]); break;
      case CMD_PUSH_ATTRIB: glPushAttrib((GLbitfield)c->i[0]); break;
      case CMD_POP_ATTRIB: glPopAttrib(); break;
      case CMD_CLEAR_COLOR: glClearColor(c->f[0],c->f[1],c->f[2],c->f[3]); break;
      case CMD_CLEAR_DEPTH: glClearDepth(c->f[0]); break;
      case CMD_CLEAR: glClear((GLbitfield)c->i[0]); break;
      case CMD_VIEWPORT: glViewport(c->i[0],c->i[1],c->i[2],c->i[3]); break;
      case CMD_SCISSOR: glScissor(c->i[0],c->i[1],c->i[2],c->i[3]); break;
      case CMD_CALL_LIST: glCallList((GLuint)c->i[0]); break;
      case CMD_LIST_BASE: glListBase((GLuint)c->i[0]); break;
      case CMD_BIND_TEXTURE: glBindTexture((GLenum)c->i[0],
                                           (GLuint)c->i[1]); break;
      case CMD_TEX_IMAGE:
        if (!c->cachedTexture) {
            if (c->i[2] == GL_RGBA)
                c->cachedTexture=rendererBackendCreateRGBA8Texture(c->i[0],c->i[1],c->data);
            else
                c->cachedTexture=rendererBackendCreateLuminanceTexture(c->i[0],c->i[1],c->data);
        }
        ensureTextureObjectCapacity(state.boundTexture);
        textureObjects[state.boundTexture]=c->cachedTexture;
        state.texture=c->cachedTexture;
        break;
      case CMD_TEX_PARAMETER: glTexParameterf((GLenum)c->i[0],(GLenum)c->i[1],c->f[0]); break;
      case CMD_TEX_ENV: glTexEnvf((GLenum)c->i[0],(GLenum)c->i[1],c->f[0]); break;
    }
}

void glCallList(GLuint list)
{
    DisplayList *displayList; size_t i; Command *command;
    if (isRecording()) { command=appendCommand(CMD_CALL_LIST); command->i[0]=(GLint)list; return; }
    if (list>=displayListCapacity || !(displayList=displayLists[list])) return;
    ++replayDepth;
    for (i=0;i<displayList->count;++i) executeCommand(&displayList->commands[i]);
    --replayDepth;
}

void glCallLists(GLsizei count, GLenum type, const GLvoid *lists)
{
    const GLubyte *bytes=(const GLubyte *)lists; int i;
    if (type!=GL_UNSIGNED_BYTE) return;
    for (i=0;i<count;++i) glCallList(state.listBase+(GLuint)bytes[i]);
}
void glDeleteLists(GLuint list, GLsizei range)
{
    int i; for(i=0;i<range;++i) if(list+(GLuint)i<displayListCapacity) { freeDisplayList(displayLists[list+i]); displayLists[list+i]=NULL; }
}
void glListBase(GLuint base)
{
    Command *c; if(isRecording()){c=appendCommand(CMD_LIST_BASE);c->i[0]=(GLint)base;return;} state.listBase=base;
}

void glGenTextures(GLsizei count, GLuint *textures)
{
    int index;
    initializeState();
    if (count <= 0 || textures == NULL)
        return;
    ensureTextureObjectCapacity(nextTextureObject + (GLuint)count);
    for (index = 0; index < count; index++)
        textures[index] = nextTextureObject++;
}

void glBindTexture(GLenum target, GLuint texture)
{
    Command *command;
    if (target != GL_TEXTURE_2D)
        return;
    if (isRecording()) {
        command=appendCommand(CMD_BIND_TEXTURE);
        command->i[0]=(GLint)target;
        command->i[1]=(GLint)texture;
        return;
    }
    initializeState();
    ensureTextureObjectCapacity(texture);
    state.boundTexture=texture;
    state.texture=textureObjects[texture];
}

void glDeleteTextures(GLsizei count, const GLuint *textures)
{
    int index;
    if (textures == NULL)
        return;
    for (index = 0; index < count; index++)
        if (textures[index] < textureObjectCapacity) {
            textureObjects[textures[index]]=0;
            if (state.boundTexture == textures[index]) {
                state.boundTexture=0;
                state.texture=textureObjectCapacity > 0 ? textureObjects[0] : 0;
            }
        }
}

void glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width,
                  GLsizei height, GLint border, GLenum format, GLenum type,
                  const GLvoid *pixels)
{
    Command *c; size_t size; size_t components;
    (void)target;(void)level;(void)internalFormat;(void)border;(void)type;
    components=format==GL_RGBA?4u:format==GL_RGB?3u:1u;
    size=(size_t)width*(size_t)height*components;
    if(isRecording()){
        c=appendCommand(CMD_TEX_IMAGE);c->i[0]=width;c->i[1]=height;c->i[2]=(GLint)format;c->dataSize=size;
        c->data=(unsigned char *)malloc(size);memcpy(c->data,pixels,size);return;
    }
    ensureTextureObjectCapacity(state.boundTexture);
    if(format==GL_RGBA)
        state.texture=rendererBackendCreateRGBA8Texture(width,height,(const unsigned char *)pixels);
    else
        state.texture=rendererBackendCreateLuminanceTexture(width,height,(const unsigned char *)pixels);
    textureObjects[state.boundTexture]=state.texture;
}
void glTexParameterf(GLenum target, GLenum name, GLfloat value)
{
    Command *c; (void)target;
    if(isRecording()){c=appendCommand(CMD_TEX_PARAMETER);c->i[0]=(GLint)target;c->i[1]=(GLint)name;c->f[0]=value;return;}
    if(name==GL_TEXTURE_MAG_FILTER||name==GL_TEXTURE_MIN_FILTER) state.textureLinear=value==GL_LINEAR;
    if(name==GL_TEXTURE_WRAP_S||name==GL_TEXTURE_WRAP_T) state.textureRepeat=value==GL_REPEAT;
}
void glTexEnvf(GLenum target, GLenum name, GLfloat value)
{
    Command *c; if(isRecording()){c=appendCommand(CMD_TEX_ENV);c->i[0]=(GLint)target;c->i[1]=(GLint)name;c->f[0]=value;}
}

GLUquadricObj *gluNewQuadric(void) { return (GLUquadricObj *)calloc(1,sizeof(GLUquadricObj)); }
void gluDeleteQuadric(GLUquadricObj *quadric) { free(quadric); }

static void multiplyCurrentOrRecord(const float *matrix)
{
    Command *command;
    if (isRecording()) {
        command = appendCommand(CMD_MULT_MATRIX);
        memcpy(command->f, matrix, sizeof(command->f));
        return;
    }
    multiplyCurrent(matrix);
}

void gluSphere(GLUquadricObj *quadric, GLdouble radius, GLint slices, GLint stacks)
{
    int stack,slice; double p0,p1,t;
    (void)quadric;
    for(stack=0;stack<stacks;++stack){
        p0=-M_PI/2.0+M_PI*stack/stacks;p1=-M_PI/2.0+M_PI*(stack+1)/stacks;
        glBegin(GL_QUAD_STRIP);
        for(slice=0;slice<=slices;++slice){
            GLfloat a[3],b[3];t=2.0*M_PI*slice/slices;
            a[0]=(GLfloat)(radius*cos(p0)*cos(t));a[1]=(GLfloat)(radius*sin(p0));a[2]=(GLfloat)(radius*cos(p0)*sin(t));
            b[0]=(GLfloat)(radius*cos(p1)*cos(t));b[1]=(GLfloat)(radius*sin(p1));b[2]=(GLfloat)(radius*cos(p1)*sin(t));
            glVertex3fv(a);glVertex3fv(b);
        }
        glEnd();
    }
}

void gluDisk(GLUquadricObj *quadric, GLdouble innerRadius, GLdouble outerRadius,
             GLint slices, GLint loops)
{
    int ring,slice; double r0,r1,t; (void)quadric;
    for(ring=0;ring<loops;++ring){
        r0=innerRadius+(outerRadius-innerRadius)*ring/loops;
        r1=innerRadius+(outerRadius-innerRadius)*(ring+1)/loops;
        glBegin(GL_QUAD_STRIP);
        for(slice=0;slice<=slices;++slice){
            GLfloat a[3],b[3];t=2.0*M_PI*slice/slices;
            a[0]=(GLfloat)(r0*cos(t));a[1]=(GLfloat)(r0*sin(t));a[2]=0;
            b[0]=(GLfloat)(r1*cos(t));b[1]=(GLfloat)(r1*sin(t));b[2]=0;
            glVertex3fv(a);glVertex3fv(b);
        }
        glEnd();
    }
}

void gluPerspective(GLdouble fieldOfView, GLdouble aspect, GLdouble nearValue, GLdouble farValue)
{
    float m[16],f; f=(float)(1.0/tan(fieldOfView*M_PI/360.0)); memset(m,0,sizeof(m));
    m[0]=f/(float)aspect;m[5]=f;m[10]=(float)((farValue+nearValue)/(nearValue-farValue));
    m[11]=-1.0f;m[14]=(float)((2.0*farValue*nearValue)/(nearValue-farValue));multiplyCurrentOrRecord(m);
}

void gluLookAt(GLdouble eyeX, GLdouble eyeY, GLdouble eyeZ, GLdouble centerX,
               GLdouble centerY, GLdouble centerZ, GLdouble upX, GLdouble upY,
               GLdouble upZ)
{
    double f[3],up[3],side[3],length; float m[16];
    f[0]=centerX-eyeX;f[1]=centerY-eyeY;f[2]=centerZ-eyeZ;
    length=sqrt(f[0]*f[0]+f[1]*f[1]+f[2]*f[2]);f[0]/=length;f[1]/=length;f[2]/=length;
    up[0]=upX;up[1]=upY;up[2]=upZ;length=sqrt(up[0]*up[0]+up[1]*up[1]+up[2]*up[2]);up[0]/=length;up[1]/=length;up[2]/=length;
    side[0]=f[1]*up[2]-f[2]*up[1];side[1]=f[2]*up[0]-f[0]*up[2];side[2]=f[0]*up[1]-f[1]*up[0];
    length=sqrt(side[0]*side[0]+side[1]*side[1]+side[2]*side[2]);side[0]/=length;side[1]/=length;side[2]/=length;
    up[0]=side[1]*f[2]-side[2]*f[1];up[1]=side[2]*f[0]-side[0]*f[2];up[2]=side[0]*f[1]-side[1]*f[0];
    matrixIdentity(m);m[0]=(float)side[0];m[4]=(float)side[1];m[8]=(float)side[2];
    m[1]=(float)up[0];m[5]=(float)up[1];m[9]=(float)up[2];m[2]=(float)-f[0];m[6]=(float)-f[1];m[10]=(float)-f[2];
    multiplyCurrentOrRecord(m);glTranslatef((float)-eyeX,(float)-eyeY,(float)-eyeZ);
}

const GLubyte *gluErrorString(GLenum error)
{
    (void)error; return (const GLubyte *)"renderer error";
}
