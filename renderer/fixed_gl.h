#ifndef BATTALION_FIXED_GL_H
#define BATTALION_FIXED_GL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef void GLvoid;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;

#define GL_FALSE                         0
#define GL_TRUE                          1
#define GL_NO_ERROR                      0

#define GL_POINTS                        0x0000
#define GL_LINES                         0x0001
#define GL_LINE_LOOP                     0x0002
#define GL_LINE_STRIP                    0x0003
#define GL_TRIANGLES                     0x0004
#define GL_TRIANGLE_STRIP                 0x0005
#define GL_TRIANGLE_FAN                   0x0006
#define GL_QUADS                         0x0007
#define GL_QUAD_STRIP                    0x0008

#define GL_DEPTH_BUFFER_BIT              0x00000100
#define GL_COLOR_BUFFER_BIT              0x00004000
#define GL_ALL_ATTRIB_BITS               0x000fffff
#define GL_POLYGON_BIT                   0x00000008
#define GL_LIGHTING_BIT                  0x00000040
#define GL_LIST_BIT                      0x00020000

#define GL_ALPHA_TEST                    0x0bc0
#define GL_BLEND                         0x0be2
#define GL_CULL_FACE                     0x0b44
#define GL_DEPTH_TEST                    0x0b71
#define GL_DITHER                        0x0bd0
#define GL_FOG                           0x0b60
#define GL_LIGHTING                      0x0b50
#define GL_LINE_SMOOTH                   0x0b20
#define GL_LINE_STIPPLE                  0x0b24
#define GL_NORMALIZE                     0x0ba1
#define GL_POINT_SMOOTH                  0x0b10
#define GL_POLYGON_SMOOTH                0x0b41
#define GL_POLYGON_STIPPLE               0x0b42
#define GL_TEXTURE_2D                    0x0de1
#define GL_TEXTURE_BINDING_2D            0x8069
#define GL_COLOR_MATERIAL                0x0b57
#define GL_AUTO_NORMAL                   0x0d80

#define GL_FRONT_AND_BACK                0x0408
#define GL_BACK                          0x0405
#define GL_POINT                         0x1b00
#define GL_LINE                          0x1b01
#define GL_FILL                          0x1b02
#define GL_FLAT                          0x1d00
#define GL_SMOOTH                        0x1d01
#define GL_LEQUAL                        0x0203
#define GL_SRC_ALPHA                     0x0302
#define GL_ONE_MINUS_SRC_ALPHA           0x0303

#define GL_MODELVIEW                     0x1700
#define GL_PROJECTION                    0x1701
#define GL_VIEWPORT                      0x0ba2
#define GL_SHADE_MODEL                   0x0b54
#define GL_LINE_WIDTH                    0x0b21
#define GL_POINT_SIZE                    0x0b11
#define GL_DEPTH_BITS                    0x0d56
#define GL_RED_BITS                      0x0d52
#define GL_STEREO                        0x0c33

#define GL_FOG_MODE                      0x0b65
#define GL_FOG_START                     0x0b63
#define GL_FOG_END                       0x0b64
#define GL_LINEAR                        0x2601
#define GL_NEAREST                       0x2600
#define GL_FASTEST                       0x1101
#define GL_POINT_SMOOTH_HINT             0x0c51
#define GL_LINE_SMOOTH_HINT              0x0c52
#define GL_POLYGON_SMOOTH_HINT           0x0c53

#define GL_COMPILE                       0x1300
#define GL_UNSIGNED_BYTE                 0x1401
#define GL_LUMINANCE                     0x1909
#define GL_RGB                           0x1907
#define GL_RGBA                          0x1908
#define GL_TEXTURE_ENV                   0x2300
#define GL_TEXTURE_ENV_MODE              0x2200
#define GL_MODULATE                      0x2100
#define GL_TEXTURE_MAG_FILTER            0x2800
#define GL_TEXTURE_MIN_FILTER            0x2801
#define GL_TEXTURE_WRAP_S                0x2802
#define GL_TEXTURE_WRAP_T                0x2803
#define GL_CLAMP                         0x2900
#define GL_REPEAT                        0x2901

void glBegin(GLenum mode);
void glEnd(void);
void glVertex2f(GLfloat x, GLfloat y);
void glVertex3fv(const GLfloat *v);
void glColor3fv(const GLfloat *v);
void glColor4fv(const GLfloat *v);
void glTexCoord2fv(const GLfloat *v);
void glIndexf(GLfloat value);

void glMatrixMode(GLenum mode);
void glLoadIdentity(void);
void glPushMatrix(void);
void glPopMatrix(void);
void glTranslatef(GLfloat x, GLfloat y, GLfloat z);
void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void glScalef(GLfloat x, GLfloat y, GLfloat z);
void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top,
             GLdouble nearValue, GLdouble farValue);

void glEnable(GLenum capability);
void glDisable(GLenum capability);
GLboolean glIsEnabled(GLenum capability);
void glBlendFunc(GLenum source, GLenum destination);
void glCullFace(GLenum mode);
void glDepthFunc(GLenum function);
void glDepthRange(GLclampd nearValue, GLclampd farValue);
void glShadeModel(GLenum mode);
void glPolygonMode(GLenum face, GLenum mode);
void glLineWidth(GLfloat width);
void glPointSize(GLfloat size);
void glFogi(GLenum name, GLint value);
void glHint(GLenum target, GLenum mode);
void glPushAttrib(GLbitfield mask);
void glPopAttrib(void);

void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void glClearDepth(GLclampd depth);
void glClear(GLbitfield buffers);
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
void glGetIntegerv(GLenum name, GLint *value);
void glGetFloatv(GLenum name, GLfloat *value);
GLenum glGetError(void);
void glFlush(void);
void glFinish(void);

GLuint glGenLists(GLsizei range);
void glNewList(GLuint list, GLenum mode);
void glEndList(void);
void glCallList(GLuint list);
void glCallLists(GLsizei count, GLenum type, const GLvoid *lists);
void glDeleteLists(GLuint list, GLsizei range);
void glListBase(GLuint base);

void glTexImage2D(GLenum target, GLint level, GLint internalFormat,
                  GLsizei width, GLsizei height, GLint border, GLenum format,
                  GLenum type, const GLvoid *pixels);
void glGenTextures(GLsizei count, GLuint *textures);
void glBindTexture(GLenum target, GLuint texture);
void glDeleteTextures(GLsizei count, const GLuint *textures);
void glTexParameterf(GLenum target, GLenum name, GLfloat value);
void glTexEnvf(GLenum target, GLenum name, GLfloat value);

typedef struct GLUquadric GLUquadricObj;
GLUquadricObj *gluNewQuadric(void);
void gluDeleteQuadric(GLUquadricObj *quadric);
void gluSphere(GLUquadricObj *quadric, GLdouble radius, GLint slices, GLint stacks);
void gluDisk(GLUquadricObj *quadric, GLdouble innerRadius, GLdouble outerRadius,
             GLint slices, GLint loops);
void gluPerspective(GLdouble fieldOfView, GLdouble aspect, GLdouble nearValue,
                    GLdouble farValue);
void gluLookAt(GLdouble eyeX, GLdouble eyeY, GLdouble eyeZ,
               GLdouble centerX, GLdouble centerY, GLdouble centerZ,
               GLdouble upX, GLdouble upY, GLdouble upZ);
const GLubyte *gluErrorString(GLenum error);

#ifdef __cplusplus
}
#endif

#endif
