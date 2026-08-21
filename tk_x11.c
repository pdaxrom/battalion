/*
 * (c) Copyright 1993, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED 
 * Permission to use, copy, modify, and distribute this software for 
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that 
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. 
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * US Government Users Restricted Rights 
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/keysym.h>

#include "battalion.h"

#if defined(__cplusplus) || defined(c_plusplus)
#define class c_class
#endif


/******************************************************************************/

static struct _WINDOWINFO
    {
    int x, y;
    int width, height;
    GLenum type;
    } windInfo = { -1, -1, 100, 100, 0};
    
static Display *display		= 0;
static XVisualInfo *visualInfo	= 0;
static Window window		= 0;
static int screen		= 0; 
static GLXContext context	= 0;



static GLenum (*KeyDownFunc)(int, GLenum)	    = NULL;
static GLenum (*KeyUpFunc)(int)			    = NULL;
static GLenum (*MouseDownFunc)(int, int, GLenum)    = NULL;
static GLenum (*MouseUpFunc)(int, int, GLenum)	    = NULL;
static GLenum (*MouseMoveFunc)(int, int, GLenum)    = NULL;
static void (*FocusFunc)(int)                       = NULL;
static void (*ExposeFunc)(int, int)		    = NULL;
static void (*ReshapeFunc)(int, int)		    = NULL;
static void (*DisplayFunc)(void)			    = NULL;
static void (*IdleFunc)(void)			    = NULL;

static int lastEventType = -1;
static Colormap colorMap;
static Cursor hiddenCursor;
static Atom wmDeleteWindow;
static int pointerIsGrabbed;
static int mouseDeltaX;
static int mouseDeltaY;

static int keyFromKeysym(KeySym key)
{
    switch (key) {
      case XK_Control_L: return TK_CONTROL_L;
      case XK_Control_R: return TK_CONTROL_R;
      case XK_Left: return TK_LEFT;
      case XK_Up: return TK_UP;
      case XK_Right: return TK_RIGHT;
      case XK_Down: return TK_DOWN;
      case XK_Prior: return TK_PAGEUP;
      case XK_Next: return TK_PAGEDOWN;
      case XK_F1: return TK_F1;
      case XK_F2: return TK_F2;
      case XK_F3: return TK_F3;
      case XK_F4: return TK_F4;
      case XK_H: return TK_H;
      case XK_h: return TK_h;
      case XK_Tab: return TK_TAB;
      case XK_Return: return TK_RETURN;
      case XK_Escape: return TK_ESCAPE;
      case XK_space: return TK_SPACE;
      case XK_1: return TK_1;
      case XK_2: return TK_2;
      case XK_3: return TK_3;
      case XK_4: return TK_4;
      case XK_6: return TK_6;
      case XK_7: return TK_7;
      case XK_8: return TK_8;
      case XK_9: return TK_9;
      case XK_A: return TK_A;
      case XK_D: return TK_D;
      case XK_I: return TK_I;
      case XK_J: return TK_J;
      case XK_K: return TK_K;
      case XK_L: return TK_L;
      case XK_P: return TK_P;
      case XK_S: return TK_S;
      case XK_W: return TK_W;
      case XK_a: return TK_a;
      case XK_d: return TK_d;
      case XK_i: return TK_i;
      case XK_j: return TK_j;
      case XK_k: return TK_k;
      case XK_l: return TK_l;
      case XK_p: return TK_p;
      case XK_s: return TK_s;
      case XK_w: return TK_w;
      default: return GL_FALSE;
    }
}

static float colorMaps[] = {
    0.000000, 1.000000, 0.000000, 1.000000, 0.000000, 1.000000, 
    0.000000, 1.000000, 0.333333, 0.776471, 0.443137, 0.556863, 
    0.443137, 0.556863, 0.219608, 0.666667, 0.666667, 0.333333, 
    0.666667, 0.333333, 0.666667, 0.333333, 0.666667, 0.333333, 
    0.666667, 0.333333, 0.666667, 0.333333, 0.666667, 0.333333, 
    0.666667, 0.333333, 0.039216, 0.078431, 0.117647, 0.156863, 
    0.200000, 0.239216, 0.278431, 0.317647, 0.356863, 0.400000, 
    0.439216, 0.478431, 0.517647, 0.556863, 0.600000, 0.639216, 
    0.678431, 0.717647, 0.756863, 0.800000, 0.839216, 0.878431, 
    0.917647, 0.956863, 0.000000, 0.000000, 0.000000, 0.000000, 
    0.000000, 0.000000, 0.000000, 0.000000, 0.247059, 0.247059, 
    0.247059, 0.247059, 0.247059, 0.247059, 0.247059, 0.247059, 
    0.498039, 0.498039, 0.498039, 0.498039, 0.498039, 0.498039, 
    0.498039, 0.498039, 0.749020, 0.749020, 0.749020, 0.749020, 
    0.749020, 0.749020, 0.749020, 0.749020, 1.000000, 1.000000, 
    1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 
    0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 
    0.000000, 0.000000, 0.247059, 0.247059, 0.247059, 0.247059, 
    0.247059, 0.247059, 0.247059, 0.247059, 0.498039, 0.498039, 
    0.498039, 0.498039, 0.498039, 0.498039, 0.498039, 0.498039, 
    0.749020, 0.749020, 0.749020, 0.749020, 0.749020, 0.749020, 
    0.749020, 0.749020, 1.000000, 1.000000, 1.000000, 1.000000, 
    1.000000, 1.000000, 1.000000, 1.000000, 0.000000, 0.000000, 
    0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 
    0.247059, 0.247059, 0.247059, 0.247059, 0.247059, 0.247059, 
    0.247059, 0.247059, 0.498039, 0.498039, 0.498039, 0.498039, 
    0.498039, 0.498039, 0.498039, 0.498039, 0.749020, 0.749020, 
    0.749020, 0.749020, 0.749020, 0.749020, 0.749020, 0.749020, 
    1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 
    1.000000, 1.000000, 0.000000, 0.000000, 0.000000, 0.000000, 
    0.000000, 0.000000, 0.000000, 0.000000, 0.247059, 0.247059, 
    0.247059, 0.247059, 0.247059, 0.247059, 0.247059, 0.247059, 
    0.498039, 0.498039, 0.498039, 0.498039, 0.498039, 0.498039, 
    0.498039, 0.498039, 0.749020, 0.749020, 0.749020, 0.749020, 
    0.749020, 0.749020, 0.749020, 0.749020, 1.000000, 1.000000, 
    1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 
    0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 
    0.000000, 0.000000, 0.247059, 0.247059, 0.247059, 0.247059, 
    0.247059, 0.247059, 0.247059, 0.247059, 0.498039, 0.498039, 
    0.498039, 0.498039, 0.498039, 0.498039, 0.498039, 0.498039, 
    0.749020, 0.749020, 0.749020, 0.749020, 0.749020, 0.749020, 
    0.749020, 0.749020, 1.000000, 1.000000, 1.000000, 1.000000, 
    1.000000, 1.000000, 1.000000, 1.000000, 0.000000, 0.000000, 
    1.000000, 1.000000, 0.000000, 0.000000, 1.000000, 1.000000, 
    0.333333, 0.443137, 0.776471, 0.556863, 0.443137, 0.219608, 
    0.556863, 0.666667, 0.666667, 0.333333, 0.666667, 0.333333, 
    0.666667, 0.333333, 0.666667, 0.333333, 0.666667, 0.333333, 
    0.666667, 0.333333, 0.666667, 0.333333, 0.666667, 0.333333, 
    0.039216, 0.078431, 0.117647, 0.156863, 0.200000, 0.239216, 
    0.278431, 0.317647, 0.356863, 0.400000, 0.439216, 0.478431, 
    0.517647, 0.556863, 0.600000, 0.639216, 0.678431, 0.717647, 
    0.756863, 0.800000, 0.839216, 0.878431, 0.917647, 0.956863, 
    0.000000, 0.141176, 0.282353, 0.427451, 0.568627, 0.713726, 
    0.854902, 1.000000, 0.000000, 0.141176, 0.282353, 0.427451, 
    0.568627, 0.713726, 0.854902, 1.000000, 0.000000, 0.141176, 
    0.282353, 0.427451, 0.568627, 0.713726, 0.854902, 1.000000, 
    0.000000, 0.141176, 0.282353, 0.427451, 0.568627, 0.713726, 
    0.854902, 1.000000, 0.000000, 0.141176, 0.282353, 0.427451, 
    0.568627, 0.713726, 0.854902, 1.000000, 0.000000, 0.141176, 
    0.282353, 0.427451, 0.568627, 0.713726, 0.854902, 1.000000, 
    0.000000, 0.141176, 0.282353, 0.427451, 0.568627, 0.713726, 
    0.854902, 1.000000, 0.000000, 0.141176, 0.282353, 0.427451, 
    0.568627, 0.713726, 0.854902, 1.000000, 0.000000, 0.141176, 
    0.282353, 0.427451, 0.568627, 0.713726, 0.854902, 1.000000, 
    0.000000, 0.141176, 0.282353, 0.427451, 0.568627, 0.713726, 
    0.854902, 1.000000, 0.000000, 0.141176, 0.282353, 0.427451, 
    0.568627, 0.713726, 0.854902, 1.000000, 0.000000, 0.141176, 
    0.282353, 0.427451, 0.568627, 0.713726, 0.854902, 1.000000, 
    0.000000, 0.141176, 0.282353, 0.427451, 0.568627, 0.713726, 
    0.854902, 1.000000, 0.000000, 0.141176, 0.282353, 0.427451, 
    0.568627, 0.713726, 0.854902, 1.000000, 0.000000, 0.141176, 
    0.282353, 0.427451, 0.568627, 0.713726, 0.854902, 1.000000, 
    0.000000, 0.141176, 0.282353, 0.427451, 0.568627, 0.713726, 
    0.854902, 1.000000, 0.000000, 0.141176, 0.282353, 0.427451, 
    0.568627, 0.713726, 0.854902, 1.000000, 0.000000, 0.141176, 
    0.282353, 0.427451, 0.568627, 0.713726, 0.854902, 1.000000, 
    0.000000, 0.141176, 0.282353, 0.427451, 0.568627, 0.713726, 
    0.854902, 1.000000, 0.000000, 0.141176, 0.282353, 0.427451, 
    0.568627, 0.713726, 0.854902, 1.000000, 0.000000, 0.141176, 
    0.282353, 0.427451, 0.568627, 0.713726, 0.854902, 1.000000, 
    0.000000, 0.141176, 0.282353, 0.427451, 0.568627, 0.713726, 
    0.854902, 1.000000, 0.000000, 0.141176, 0.282353, 0.427451, 
    0.568627, 0.713726, 0.854902, 1.000000, 0.000000, 0.141176, 
    0.282353, 0.427451, 0.568627, 0.713726, 0.854902, 1.000000, 
    0.000000, 0.141176, 0.282353, 0.427451, 0.568627, 0.713726, 
    0.854902, 1.000000, 0.000000, 0.000000, 0.000000, 0.000000, 
    1.000000, 1.000000, 1.000000, 1.000000, 0.333333, 0.443137, 
    0.443137, 0.219608, 0.776471, 0.556863, 0.556863, 0.666667, 
    0.666667, 0.333333, 0.666667, 0.333333, 0.666667, 0.333333, 
    0.666667, 0.333333, 0.666667, 0.333333, 0.666667, 0.333333, 
    0.666667, 0.333333, 0.666667, 0.333333, 0.039216, 0.078431, 
    0.117647, 0.156863, 0.200000, 0.239216, 0.278431, 0.317647, 
    0.356863, 0.400000, 0.439216, 0.478431, 0.517647, 0.556863, 
    0.600000, 0.639216, 0.678431, 0.717647, 0.756863, 0.800000, 
    0.839216, 0.878431, 0.917647, 0.956863, 0.000000, 0.000000, 
    0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 
    0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 
    0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 
    0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 
    0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 
    0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 
    0.000000, 0.000000, 0.247059, 0.247059, 0.247059, 0.247059, 
    0.247059, 0.247059, 0.247059, 0.247059, 0.247059, 0.247059, 
    0.247059, 0.247059, 0.247059, 0.247059, 0.247059, 0.247059, 
    0.247059, 0.247059, 0.247059, 0.247059, 0.247059, 0.247059, 
    0.247059, 0.247059, 0.247059, 0.247059, 0.247059, 0.247059, 
    0.247059, 0.247059, 0.247059, 0.247059, 0.247059, 0.247059, 
    0.247059, 0.247059, 0.247059, 0.247059, 0.247059, 0.247059, 
    0.498039, 0.498039, 0.498039, 0.498039, 0.498039, 0.498039, 
    0.498039, 0.498039, 0.498039, 0.498039, 0.498039, 0.498039, 
    0.498039, 0.498039, 0.498039, 0.498039, 0.498039, 0.498039, 
    0.498039, 0.498039, 0.498039, 0.498039, 0.498039, 0.498039, 
    0.498039, 0.498039, 0.498039, 0.498039, 0.498039, 0.498039, 
    0.498039, 0.498039, 0.498039, 0.498039, 0.498039, 0.498039, 
    0.498039, 0.498039, 0.498039, 0.498039, 0.749020, 0.749020, 
    0.749020, 0.749020, 0.749020, 0.749020, 0.749020, 0.749020, 
    0.749020, 0.749020, 0.749020, 0.749020, 0.749020, 0.749020, 
    0.749020, 0.749020, 0.749020, 0.749020, 0.749020, 0.749020, 
    0.749020, 0.749020, 0.749020, 0.749020, 0.749020, 0.749020, 
    0.749020, 0.749020, 0.749020, 0.749020, 0.749020, 0.749020, 
    0.749020, 0.749020, 0.749020, 0.749020, 0.749020, 0.749020, 
    0.749020, 0.749020, 1.000000, 1.000000, 1.000000, 1.000000, 
    1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 
    1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 
    1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 
    1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 
    1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 
    1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 
};

/******************************************************************************/

void tkCloseWindow(void)
    {

    if (display)
	{
	glFlush();
	glFinish();
	if (hiddenCursor)
	    XFreeCursor(display, hiddenCursor);
	XDestroyWindow(display, window);
	glXDestroyContext(display, context);
	XFreeColormap(display, colorMap);
	XFree((char *)visualInfo);
	display = 0;

	KeyDownFunc	= 0;
	KeyUpFunc	= 0;
	MouseDownFunc	= 0;
	MouseUpFunc	= 0;
	MouseMoveFunc	= 0;
	FocusFunc	= 0;

	lastEventType	= -1;
	pointerIsGrabbed = 0;
	mouseDeltaX = mouseDeltaY = 0;
	hiddenCursor = 0;
	}
    }

/******************************************************************************/

static GLenum DoNextEvent(void)
{
    XEvent current, ahead;
    KeySym ks;
    int key;
    GLenum mask;
    extern int paused;

    XNextEvent(display, &current);
    switch (current.type) {

      case ClientMessage:
	if ((Atom)current.xclient.data.l[0] == wmDeleteWindow)
	    tkQuit();
	return GL_FALSE;

      case FocusIn:
	if (FocusFunc)
	    (*FocusFunc)(1);
	return GL_FALSE;

      case FocusOut:
	if (FocusFunc)
	    (*FocusFunc)(0);
	else
	    paused = 1;
	return GL_FALSE;
    
      case MappingNotify:
	XRefreshKeyboardMapping((XMappingEvent *)&current);
	lastEventType = MappingNotify;

	return GL_FALSE;

      case UnmapNotify:
	if (FocusFunc)
	    (*FocusFunc)(0);
	else
	    paused = 1;
	return GL_FALSE;

      case Expose:
	while (XEventsQueued(current.xexpose.display, QueuedAfterReading) > 0)
	    {
	    XPeekEvent(current.xexpose.display, &ahead);
	    if (ahead.xexpose.window != current.xexpose.window || ahead.type != Expose)
		{
		break;
		}
	    XNextEvent(display, &current);
	    }
	    
	if (current.xexpose.count == 0)
	    {
	    if (ExposeFunc)
		(*ExposeFunc)(windInfo.width, windInfo.height);
	    if (lastEventType == ConfigureNotify)
		{
		lastEventType = Expose;
		return GL_FALSE;
		}
	    else
		{
		lastEventType = Expose;
		return GL_TRUE;
		}
	    }
	    
	return GL_FALSE;

      case ConfigureNotify:
	lastEventType	= ConfigureNotify;
	windInfo.width	= current.xconfigure.width;
	windInfo.height = current.xconfigure.height;
	if (ReshapeFunc)
	    (*ReshapeFunc)(windInfo.width, windInfo.height);
	return GL_TRUE;

      case ButtonPress:
	lastEventType = ButtonPress;

	    mask = 0;
	    if (current.xbutton.button == 1)
		{
		mask |= TK_LEFTBUTTON;
		}
	    if (current.xbutton.button == 2)
		{
		mask |= TK_MIDDLEBUTTON;
		}
	    if (current.xbutton.button == 3)
		{
		mask |= TK_RIGHTBUTTON;
		}
		
	    return MouseDownFunc ? (*MouseDownFunc)(current.xbutton.x, current.xbutton.y, mask) : GL_FALSE;

	
      case ButtonRelease:
	lastEventType = ButtonRelease;

	    mask = 0;
	    if (current.xbutton.button == 1)
		{
		mask |= TK_LEFTBUTTON;
		}
	    if (current.xbutton.button == 2)
		{
		mask |= TK_MIDDLEBUTTON;
		}
	    if (current.xbutton.button == 3)
		{
		mask |= TK_RIGHTBUTTON;
		}
		
	    return MouseUpFunc ? (*MouseUpFunc)(current.xbutton.x, current.xbutton.y, mask) : GL_FALSE;

      case MotionNotify:
	if (pointerIsGrabbed)
	    {
	    int centerX = windInfo.width / 2;
	    int centerY = windInfo.height / 2;
	    int deltaX = current.xmotion.x - centerX;
	    int deltaY = current.xmotion.y - centerY;

	    mouseDeltaX += deltaX;
	    mouseDeltaY += deltaY;
	    if (deltaX || deltaY)
		XWarpPointer(display, None, window, 0, 0, 0, 0,
		    centerX, centerY);
	    return GL_FALSE;
	    }
	if (MouseMoveFunc)
	    return (*MouseMoveFunc)(current.xmotion.x, current.xmotion.y, 0);
	return GL_FALSE;

      case KeyPress:
	lastEventType = KeyPress;
	ks = XLookupKeysym(&current.xkey, 0);
	key = keyFromKeysym(ks);
	mask = 0;
	if (current.xkey.state & ShiftMask)
	    mask |= TK_SHIFT;
	if (current.xkey.state & ControlMask)
	    mask |= TK_CONTROL;

	if (key)
	    return KeyDownFunc ? (*KeyDownFunc)(key, mask) : GL_FALSE;
	return GL_FALSE;

      case KeyRelease:
	if (XEventsQueued(display, QueuedAfterReading) > 0)
	    {
	    XPeekEvent(display, &ahead);
	    if ((ahead.type == KeyPress) &&
		(ahead.xkey.time == current.xkey.time) &&
		(ahead.xkey.keycode == current.xkey.keycode))
		return GL_FALSE;
	    }
	lastEventType = KeyRelease;
	ks = XLookupKeysym(&current.xkey, 0);
	key = keyFromKeysym(ks);

	if (key)
	    return KeyUpFunc ? (*KeyUpFunc)(key) : GL_FALSE;
	return GL_FALSE;

    }
    return GL_FALSE;
}

/******************************************************************************/

/********************************************
Andy's new shorter version of the tkExec
function to try and speed things up a bit
********************************************/
void tkExec(void)
    {
    while (GL_TRUE)
	{
	if (IdleFunc)
	    (*IdleFunc)();
	
	while (XPending(display))
	    DoNextEvent();
	}
    }


/******************************************************************************/

void tkGetMouseLoc(int *x, int *y)
    {
    int junk;

    *x = 0;
    *y = 0;
    XQueryPointer(display, window, (Window *)&junk, (Window *)&junk,
		  &junk, &junk, x, y, (unsigned int *)&junk);
    }

void tkGetMouseDelta(float *x, float *y)
    {
    *x = (float)mouseDeltaX;
    *y = (float)mouseDeltaY;
    mouseDeltaX = mouseDeltaY = 0;
    }

/******************************************************************************/

static XVisualInfo *FindVisual(GLenum type)
    {
    int list[32];
    int i;

    i = 0;

    list[i++] = GLX_LEVEL;
    list[i++] = 0;

    if (TK_IS_DOUBLE(type)) {
	list[i++] = (int) GLX_DOUBLEBUFFER;
    }

    if (TK_IS_RGB(type))
	{
	list[i++] = GLX_RGBA;
	list[i++] = GLX_RED_SIZE;
	list[i++] = 1;
	list[i++] = GLX_GREEN_SIZE;
	list[i++] = 1;
	list[i++] = GLX_BLUE_SIZE;
	list[i++] = 1;

	if (TK_HAS_ALPHA(type))
	    {
	    list[i++] = GLX_ALPHA_SIZE;
	    list[i++] = 1;
	    }

	if (TK_HAS_ACCUM(type))
	    {
	    list[i++] = GLX_ACCUM_RED_SIZE;
	    list[i++] = 1;
	    list[i++] = GLX_ACCUM_GREEN_SIZE;
	    list[i++] = 1;
	    list[i++] = GLX_ACCUM_BLUE_SIZE;
	    list[i++] = 1;
	    
	    if (TK_HAS_ALPHA(type))
		{
		list[i++] = GLX_ACCUM_ALPHA_SIZE;
		list[i++] = 1;
	    }
	    }
	}
    else if (TK_IS_INDEX(type))
	{
	list[i++] = GLX_BUFFER_SIZE;
	list[i++] = 1;
	}

    if (TK_HAS_DEPTH(type))
	{
	list[i++] = GLX_DEPTH_SIZE;
	list[i++] = 1;
	}

    if (TK_HAS_STENCIL(type))
	{
	list[i++] = GLX_STENCIL_SIZE;
	list[i++] = 1;
	}

    list[i] = None;
    
    return glXChooseVisual(display, screen, (int *) list);
    }

/******************************************************************************/

static int MakeVisualType(XVisualInfo *vi)
    {
    GLenum mask;
    int x, y, z;

    mask = 0;

    glXGetConfig(display, vi, GLX_RGBA, &x);
    
    if (x) {
	mask |= TK_RGB;
	
	glXGetConfig(display, vi, GLX_ALPHA_SIZE, &x);
	if (x > 0)
	    {
	    mask |= TK_ALPHA;
	    }
	    
	glXGetConfig(display, vi, GLX_ACCUM_RED_SIZE, &x);
	glXGetConfig(display, vi, GLX_ACCUM_GREEN_SIZE, &y);
	glXGetConfig(display, vi, GLX_ACCUM_BLUE_SIZE, &z);
	
	if (x > 0 && y > 0 && z > 0)
	    {
	    mask |= TK_ACCUM;
	    }
	}
    else
	{
	mask |= TK_INDEX;
	}

    glXGetConfig(display, vi, GLX_DOUBLEBUFFER, &x);
    if (x)
	{
	mask |= TK_DOUBLE;
	}
    else
	{
	mask |= TK_SINGLE;
	}

    glXGetConfig(display, vi, GLX_DEPTH_SIZE, &x);
    if (x > 0)
	{
	mask |= TK_DEPTH;
	}

    glXGetConfig(display, vi, GLX_STENCIL_SIZE, &x);
    if (x > 0)
	{
	mask |= TK_STENCIL;
	}

    if (glXIsDirect(display, context))
	{
	mask |= TK_DIRECT;
	}
    else
	{
	mask |= TK_INDIRECT;
	}

    return mask;
    }

/******************************************************************************/

static int WaitForMapNotify(Display *d, XEvent *e, char *arg)
    {
    Display *display;
    char *arguments;
    
    display = d;
    arguments = arg;
    
	if (e->type == MapNotify && e->xmap.window == window) {
	    return GL_TRUE;
	}
	return GL_FALSE;
    }

/******************************************************************************/

void tkInitPosition(int x, int y, int width, int height)
    {
    windInfo.x	    = x;
    windInfo.y	    = y;
    windInfo.width  = width;
    windInfo.height = height;
    }

/******************************************************************************/

void tkInitDisplayMode(GLenum type)
    {
    windInfo.type = type;
    }

/******************************************************************************/

GLenum tkInitWindow(char *title)
{
    XSetWindowAttributes wa;
    XSizeHints sh;
    XEvent e;
    int erb, evb;
    unsigned long mask;

    if (!display)
	{
	display = XOpenDisplay(0);
	if (!display)
	    {
	    fprintf(stderr, "Can't connect to display!\n");
	    return GL_FALSE;
	    }
	    
	if (!glXQueryExtension(display, &erb, &evb))
	    {
	    fprintf(stderr, "No glx extension!\n");
	    return GL_FALSE;
	    }
	    
	screen = DefaultScreen(display);
	}

    visualInfo = FindVisual(windInfo.type);
    
    if (!visualInfo)
	{
	fprintf(stderr, "Window type not found!\n");
	return GL_FALSE;
	}

    context = glXCreateContext(display, visualInfo, None,
			       (TK_IS_DIRECT(windInfo.type)) ? GL_TRUE
							     : GL_FALSE);
    if (!context)
	{
	fprintf(stderr, "Can't create a context!\n");
	return GL_FALSE;
	}

    windInfo.type = MakeVisualType(visualInfo);

    if (TK_IS_INDEX(windInfo.type))
	{
	if (visualInfo->class != StaticColor && visualInfo->class != StaticGray)
	    {
	    colorMap = XCreateColormap(display, RootWindow(display, screen),
				       visualInfo->visual, AllocAll);
	    }
	else
	    {
	    colorMap = XCreateColormap(display, RootWindow(display, screen),
				       visualInfo->visual, AllocNone);
	    }
	    
	wa.colormap = colorMap;
	tkSetRGBMap(256, colorMaps);
	wa.background_pixel = 7;
	wa.border_pixel = 0;
	}
    else
	{
	colorMap = XCreateColormap(display, RootWindow(display, screen),
				   visualInfo->visual, AllocNone);
	wa.colormap = colorMap;
	tkSetRGBMap(256, colorMaps);
	wa.background_pixel = 0xFFFFFFFF;
	wa.border_pixel = 0;
	}
	
    wa.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask |

		    KeyReleaseMask |

	    ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
	    FocusChangeMask;

    mask = CWBackPixel | CWBorderPixel | CWEventMask | CWColormap;

    window = XCreateWindow(display, RootWindow(display, screen), 0, 0,
			   windInfo.width, windInfo.height, 0,
			   visualInfo->depth, InputOutput, visualInfo->visual,
			   mask, &wa);

    wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wmDeleteWindow, 1);
    
    if ((windInfo.x != -1) && (windInfo.y != -1))
	{
	sh.flags    = USPosition;
	sh.x	    = windInfo.x + 10;
	sh.y	    = windInfo.y + 10;
	
	XSetStandardProperties(display, window, title, title, None, 0, 0, &sh);
	}
    else
	{
	XSetStandardProperties(display, window, title, title, None, 0, 0, 0);
	}

    XMapWindow(display, window);
    XIfEvent(display, &e, WaitForMapNotify, 0);

    XSetWMColormapWindows(display, window, &window, 1);

    if (!glXMakeCurrent(display, window, context))
	{
	fprintf(stderr, "Can't make window current drawable!\n");
	return GL_FALSE;
	}

    XFlush(display);

    return GL_TRUE;
}

/******************************************************************************/

static int Ignore(Display *parm1, XErrorEvent *parm2)
    {
    Display *display;
    XErrorEvent *error;
    
    display = parm1;
    error = parm2;
    
	return 0;
    }

/******************************************************************************/

void tkSetRGBMap(int size, float *rgb)
{
    XErrorHandler old_handler;
    XColor c;
    int rShift, gShift, bShift, max, i;

    old_handler = XSetErrorHandler(Ignore);

    switch (visualInfo->class) {
      case DirectColor:
	max = (size > visualInfo->colormap_size) ? visualInfo->colormap_size
						 : size;
	for (i = 0; i < max; i++)
	    {
	    rShift = ffs((unsigned int)visualInfo->red_mask) - 1;
	    gShift = ffs((unsigned int)visualInfo->green_mask) - 1;
	    bShift = ffs((unsigned int)visualInfo->blue_mask) - 1;
	    c.pixel = ((i << rShift) & visualInfo->red_mask) |
		      ((i << gShift) & visualInfo->green_mask) |
		      ((i << bShift) & visualInfo->blue_mask);
		      
	    c.red = (unsigned short)(rgb[i] * 65535.0 + 0.5);
	    c.green = (unsigned short)(rgb[size+i] * 65535.0 + 0.5);
	    c.blue = (unsigned short)(rgb[size*2+i] * 65535.0 + 0.5);
	    
	    c.flags = DoRed | DoGreen | DoBlue;
	    XStoreColor(display, colorMap, &c);
	    }
	break;
	
      case GrayScale:
      case PseudoColor:
	max = (size > visualInfo->colormap_size) ? visualInfo->colormap_size
						 : size;
	for (i = 0; i < max; i++) {
	    c.pixel = i;
	    c.red = (unsigned short)(rgb[i] * 65535.0 + 0.5);
	    c.green = (unsigned short)(rgb[size+i] *
				       65535.0 + 0.5);
	    c.blue = (unsigned short)(rgb[size*2+i] *
				      65535.0 + 0.5);
	    c.flags = DoRed | DoGreen | DoBlue;
	    XStoreColor(display, colorMap, &c);
	}
	break;
    }

    XSync(display, 0);
    XSetErrorHandler(old_handler);
}

/******************************************************************************/

void tkSwapBuffers(void)
{
    if (display) {
	glXSwapBuffers(display, window);
    }
}

/******************************************************************************/

/*
 * Stuff below added by Johan Hagman for pointer
 * grabbing in Solaris battalion
 */

void unGrabPointer()
{
    if (!pointerIsGrabbed)
	return;
    XUngrabPointer(display, CurrentTime);
    XUndefineCursor(display, window);
    pointerIsGrabbed = 0;
    mouseDeltaX = mouseDeltaY = 0;
}


void grabPointer()
{
    Pixmap blank;
    XColor color;
    char data = 0;

    if (pointerIsGrabbed)
	return;

    /* Grab the pointer so you cannot move the  mouse out of the main
     * window. Also set the cursor to a new cursor with no shape.
     */
    if (XGrabPointer(display, window, True,
			ButtonReleaseMask | ButtonPressMask | PointerMotionMask,
			GrabModeAsync, GrabModeAsync, window,
			None, CurrentTime) != GrabSuccess)
    {
	/* Error while grab - not too bad but let user know */	
	fprintf(stderr, "Warning: Pointer grab unsuccessful.\n");
	return;
    }

    if (!hiddenCursor)
	{
	memset(&color, 0, sizeof(color));
	blank = XCreateBitmapFromData(display, window, &data, 1, 1);
	hiddenCursor = XCreatePixmapCursor(display, blank, blank,
		&color, &color, 0, 0);
	XFreePixmap(display, blank);
	}
    XDefineCursor(display, window, hiddenCursor);
    pointerIsGrabbed = 1;
    mouseDeltaX = mouseDeltaY = 0;
    XWarpPointer(display, None, window, 0, 0, 0, 0,
	windInfo.width / 2, windInfo.height / 2);
    XFlush(display);
}

/******************************************************************************/

void tkQuit(void)
{
    tkCloseWindow();
    exit(0);
}

void tkExposeFunc(void (*Func)(int, int))
{
    ExposeFunc = Func;
}

void tkReshapeFunc(void (*Func)(int, int))
{
    ReshapeFunc = Func;
}

void tkDisplayFunc(void (*Func)(void))
{
    DisplayFunc = Func;
}

void tkKeyDownFunc(GLenum (*Func)(int, GLenum))
{
    KeyDownFunc = Func;
}

void tkKeyUpFunc(GLenum (*Func)(int))
{
    KeyUpFunc = Func;
}

void tkMouseDownFunc(GLenum (*Func)(int, int, GLenum))
{
    MouseDownFunc = Func;
}

void tkMouseUpFunc(GLenum (*Func)(int, int, GLenum))
{
    MouseUpFunc = Func;
}

void tkMouseMoveFunc(GLenum (*Func)(int, int, GLenum))
{
    MouseMoveFunc = Func;
}

void tkFocusFunc(void (*Func)(int))
{
    FocusFunc = Func;
}

void tkIdleFunc(void (*Func)(void))
{
    IdleFunc = Func;
}

/******************************************************************************/


/********************************************
 * void tkExec(void)
    {
    GLenum flag;

    while (GL_TRUE)
	{
	if (IdleFunc)
	    {
	    if (IdleFunc)
		{
		(*IdleFunc)();
		}
	    flag = GL_FALSE;
	    while (XPending(display))
		{
		flag |= DoNextEvent();
		}
	    if (flag == GL_TRUE)
		{
		if (DisplayFunc)
		    {
		    (*DisplayFunc)();
		    }
		}
	    }
	else
	    {
	    if (DoNextEvent() == GL_TRUE)
		{
		if (DisplayFunc)
		    {
		    (*DisplayFunc)();
		    }
		}
	    }
	}
    }

************************************************/

/*
void tkSetOneColor(int index, float r, float g, float b)
{
    XErrorHandler old_handler;
    XColor c;
    int rShift, gShift, bShift;

    old_handler = XSetErrorHandler(Ignore);

    switch (visualInfo->class) {
      case DirectColor:
	rShift = ffs((unsigned int)visualInfo->red_mask) - 1;
	gShift = ffs((unsigned int)visualInfo->green_mask) - 1;
	bShift = ffs((unsigned int)visualInfo->blue_mask) - 1;
	c.pixel = ((index << rShift) & visualInfo->red_mask) |
		  ((index << gShift) & visualInfo->green_mask) |
		  ((index << bShift) & visualInfo->blue_mask);
	c.red = (unsigned short)(r * 65535.0 + 0.5);
	c.green = (unsigned short)(g * 65535.0 + 0.5);
	c.blue = (unsigned short)(b * 65535.0 + 0.5);
	c.flags = DoRed | DoGreen | DoBlue;
	XStoreColor(display, colorMap, &c);
	break;
      case GrayScale:
      case PseudoColor:
	if (index < visualInfo->colormap_size) {
	    c.pixel = index;
	    c.red = (unsigned short)(r * 65535.0 + 0.5);
	    c.green = (unsigned short)(g * 65535.0 + 0.5);
	    c.blue = (unsigned short)(b * 65535.0 + 0.5);
	    c.flags = DoRed | DoGreen | DoBlue;
	    XStoreColor(display, colorMap, &c);
	}
	break;
    }

    XSync(display, 0);
    XSetErrorHandler(old_handler);
}
*/
/******************************************************************************/
/*
void tkSetFogRamp(int density, int startIndex)
{
    XErrorHandler old_handler;
    XColor c[256];
    int rShift, gShift, bShift, intensity, fogValues, colorValues;
    int i, j, k;

    old_handler = XSetErrorHandler(Ignore);

    switch (visualInfo->class) {
      case DirectColor:
	fogValues = 1 << density;
	colorValues = 1 << startIndex;
	for (i = 0; i < colorValues; i++) {
	    for (j = 0; j < fogValues; j++) {
		k = i * fogValues + j;
		intensity = i * fogValues + j * colorValues;
		if (intensity > visualInfo->colormap_size) {
		    intensity = visualInfo->colormap_size;
		}
		intensity = (intensity << 8) | intensity;
		rShift = ffs((unsigned int)visualInfo->red_mask) - 1;
		gShift = ffs((unsigned int)visualInfo->green_mask) - 1;
		bShift = ffs((unsigned int)visualInfo->blue_mask) - 1;
		c[k].pixel = ((k << rShift) & visualInfo->red_mask) |
			     ((k << gShift) & visualInfo->green_mask) |
			     ((k << bShift) & visualInfo->blue_mask);
		c[k].red = (unsigned short)intensity;
		c[k].green = (unsigned short)intensity;
		c[k].blue = (unsigned short)intensity;
		c[k].flags = DoRed | DoGreen | DoBlue;
	    }
	}
	XStoreColors(display, colorMap, c, visualInfo->colormap_size);
	break;
      case GrayScale:
      case PseudoColor:
	fogValues = 1 << density;
	colorValues = 1 << startIndex;
	for (i = 0; i < colorValues; i++) {
	    for (j = 0; j < fogValues; j++) {
		k = i * fogValues + j;
		intensity = i * fogValues + j * colorValues;
		if (intensity > visualInfo->colormap_size) {
		    intensity = visualInfo->colormap_size;
		}
		intensity = (intensity << 8) | intensity;
		c[k].pixel = k;
		c[k].red = (unsigned short)intensity;
		c[k].green = (unsigned short)intensity;
		c[k].blue = (unsigned short)intensity;
		c[k].flags = DoRed | DoGreen | DoBlue;
	    }
	}
	XStoreColors(display, colorMap, c, visualInfo->colormap_size);
	break;
    }

    XSync(display, 0);
    XSetErrorHandler(old_handler);
}
*/
/******************************************************************************/
/*
void tkSetGreyRamp(void)
{
    XErrorHandler old_handler;
    XColor c[256];
    float intensity;
    int rShift, gShift, bShift, i;

    old_handler = XSetErrorHandler(Ignore);

    switch (visualInfo->class) {
      case DirectColor:
	for (i = 0; i < visualInfo->colormap_size; i++) {
	    intensity = (float)i / (float)visualInfo->colormap_size *
			65535.0 + 0.5;
	    rShift = ffs((unsigned int)visualInfo->red_mask) - 1;
	    gShift = ffs((unsigned int)visualInfo->green_mask) - 1;
	    bShift = ffs((unsigned int)visualInfo->blue_mask) - 1;
	    c[i].pixel = ((i << rShift) & visualInfo->red_mask) |
			 ((i << gShift) & visualInfo->green_mask) |
			 ((i << bShift) & visualInfo->blue_mask);
	    c[i].red = (unsigned short)intensity;
	    c[i].green = (unsigned short)intensity;
	    c[i].blue = (unsigned short)intensity;
	    c[i].flags = DoRed | DoGreen | DoBlue;
	}
	XStoreColors(display, colorMap, c, visualInfo->colormap_size);
	break;
      case GrayScale:
      case PseudoColor:
	for (i = 0; i < visualInfo->colormap_size; i++) {
	    intensity = (float)i / (float)visualInfo->colormap_size *
			65535.0 + 0.5;
	    c[i].pixel = i;
	    c[i].red = (unsigned short)intensity;
	    c[i].green = (unsigned short)intensity;
	    c[i].blue = (unsigned short)intensity;
	    c[i].flags = DoRed | DoGreen | DoBlue;
	}
	XStoreColors(display, colorMap, c, visualInfo->colormap_size);
	break;
    }

    XSync(display, 0);
    XSetErrorHandler(old_handler);
}
*/

/*
      case MotionNotify:
	lastEventType = MotionNotify;
	if (MouseMoveFunc) {
	    GLenum mask;

	    mask = 0;
	    if (current.xmotion.state & Button1Mask) {
		mask |= TK_LEFTBUTTON;
	    }
	    if (current.xmotion.state & Button2Mask) {
		mask |= TK_MIDDLEBUTTON;
	    }
	    if (current.xmotion.state & Button3Mask) {
		mask |= TK_RIGHTBUTTON;
	    }
	    return (*MouseMoveFunc)(current.xmotion.x, current.xmotion.y, mask);
	} else {
	    return GL_FALSE;
	}
*/

/*
 float tkRGBMap[8][3] = {
    {
	0, 0, 0
    },
    {
	1, 0, 0
    },
    {
	0, 1, 0
    },
    {
	1, 1, 0
    },
    {
	0, 0, 1
    },
    {
	1, 0, 1
    },
    {
	0, 1, 1
    },
    {
	1, 1, 1
    }
};

 */
/******************************************************************************/
/*
void tkQuit(void)
{

    exit(0);
}
*/
 
 /******************************************************************************/
/*
void tkExposeFunc(void (*Func)(int, int))
{

    ExposeFunc = Func;
}
*/
/******************************************************************************/
/*
void tkReshapeFunc(void (*Func)(int, int))
{

    ReshapeFunc = Func;
}
*/
/******************************************************************************/
/*
void tkDisplayFunc(void (*Func)(void))
{

    DisplayFunc = Func;
}
*/
/******************************************************************************/
/*
void tkKeyDownFunc(GLenum (*Func)(int, GLenum))
{

    KeyDownFunc = Func;
}
*/
/******************************************************************************/
/*
void tkMouseDownFunc(GLenum (*Func)(int, int, GLenum))
{

    MouseDownFunc = Func;
}
*/
/******************************************************************************/
/*
void tkMouseUpFunc(GLenum (*Func)(int, int, GLenum))
{

    MouseUpFunc = Func;
}
*/
/******************************************************************************/
/*
void tkMouseMoveFunc(GLenum (*Func)(int, int, GLenum))
{

    MouseMoveFunc = Func;
}
*/
/******************************************************************************/
/*
void tkIdleFunc(void (*Func)(void))
{

    IdleFunc = Func;
}
*/
/******************************************************************************/
/*
GLint tkGetColorMapSize(void)
{

    if (!display) {
	return 0;
    } else {
	return visualInfo->colormap_size;
    }
}
*/

/******************************************************************************/
/*
Display *tkGetXDisplay(void)
    {
    return display;
    }
*/
/******************************************************************************/
/*
Window tkGetXWindow(void)
    {
    return window;
    }
*/


/*
static void (*ExposeFunc)(int, int)		    = NULL;
static void (*ReshapeFunc)(int, int)		    = NULL;
static void (*DisplayFunc)(void)		    = NULL;
static void (*IdleFunc)(void)			    = NULL;
*/




/*
	  case XK_0: 		key = TK_0;		break;
	  case XK_5: 		key = TK_5;		break;
	  case XK_B: 		key = TK_B;		break;
	  case XK_C: 		key = TK_C;		break;
	  case XK_E: 		key = TK_E;		break;
	  case XK_F: 		key = TK_F;		break;
	  case XK_G: 		key = TK_G;		break;
	  case XK_N: 		key = TK_N;		break;
	  case XK_O: 		key = TK_O;		break;
	  case XK_Q: 		key = TK_Q;		break;
	  case XK_R: 		key = TK_R;		break;
	  case XK_U: 		key = TK_U;		break;
	  case XK_V: 		key = TK_V;		break;
	  case XK_W: 		key = TK_W;		break;
	  case XK_X: 		key = TK_X;		break;
	  case XK_Y: 		key = TK_Y;		break;
	  case XK_b: 		key = TK_b;		break;
	  case XK_c: 		key = TK_c;		break;
	  case XK_e: 		key = TK_e;		break;
	  case XK_f: 		key = TK_f;		break;
	  case XK_g: 		key = TK_g;		break;
	  case XK_n: 		key = TK_n; 		break;
	  case XK_o: 		key = TK_o;		break;
	  case XK_r: 		key = TK_r;		break;
	  case XK_t: 		key = TK_t;		break;
	  case XK_u: 		key = TK_u;		break;
	  case XK_v: 		key = TK_v;		break;
	  case XK_x: 		key = TK_x;		break;
	  case XK_y: 		key = TK_y;		break;
	  case XK_Shift_L:	key = TK_SHIFT_L;	break;
	  case XK_Shift_R:	key = TK_SHIFT_R;	break;
	  case XK_Return: 	key = TK_RETURN;	break;
*/
