#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>
#import <MetalKit/MetalKit.h>

#include "tk.h"
#include "renderer/backend.h"

#include <stdio.h>
#include <stdlib.h>

static struct {
    int x, y;
    int width, height;
    GLenum type;
} windowInfo = { -1, -1, 100, 100, 0 };

static NSWindow *mainWindow;
static MTKView *metalView;
static int running;
static int pointerIsGrabbed;
static int mouseX;
static int mouseY;

static GLenum (*KeyDownFunc)(int, GLenum);
static GLenum (*KeyUpFunc)(int);
static GLenum (*MouseDownFunc)(int, int, GLenum);
static GLenum (*MouseUpFunc)(int, int, GLenum);
static GLenum (*MouseMoveFunc)(int, int, GLenum);
static void (*ExposeFunc)(int, int);
static void (*ReshapeFunc)(int, int);
static void (*DisplayFunc)(void);
static void (*IdleFunc)(void);

extern int paused;

static GLenum modifierMask(NSEventModifierFlags flags)
{
    GLenum mask = 0;
    if (flags & NSEventModifierFlagShift)
        mask |= TK_SHIFT;
    if (flags & NSEventModifierFlagControl)
        mask |= TK_CONTROL;
    return mask;
}

static int keyFromEvent(NSEvent *event)
{
    NSString *characters = event.charactersIgnoringModifiers;
    unichar key;
    if (!characters.length)
        return 0;
    key = [characters characterAtIndex:0];
    switch (key) {
      case NSLeftArrowFunctionKey: return TK_LEFT;
      case NSUpArrowFunctionKey: return TK_UP;
      case NSRightArrowFunctionKey: return TK_RIGHT;
      case NSDownArrowFunctionKey: return TK_DOWN;
      case 0x0d: return TK_RETURN;
      case 0x1b: return TK_ESCAPE;
      default:
        return key < 128 ? (int)key : 0;
    }
}

static GLenum buttonMaskForEvent(NSEvent *event)
{
    switch (event.buttonNumber) {
      case 0: return TK_LEFTBUTTON;
      case 1: return TK_RIGHTBUTTON;
      case 2: return TK_MIDDLEBUTTON;
      default: return 0;
    }
}

@interface BattalionMetalView : MTKView
@end

@implementation BattalionMetalView

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)updateMouse:(NSEvent *)event
{
    if (pointerIsGrabbed) {
        mouseX += (int)event.deltaX;
        mouseY += (int)event.deltaY;
    } else {
        NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
        mouseX = (int)point.x;
        mouseY = (int)(self.bounds.size.height - point.y);
    }
    if (mouseX < 0) mouseX = 0;
    if (mouseY < 0) mouseY = 0;
    if (mouseX >= (int)self.bounds.size.width) mouseX = (int)self.bounds.size.width - 1;
    if (mouseY >= (int)self.bounds.size.height) mouseY = (int)self.bounds.size.height - 1;
}

- (void)keyDown:(NSEvent *)event
{
    int key = keyFromEvent(event);
    if (key && KeyDownFunc)
        (*KeyDownFunc)(key, modifierMask(event.modifierFlags));
}

- (void)keyUp:(NSEvent *)event
{
    int key = keyFromEvent(event);
    if (key && KeyUpFunc)
        (*KeyUpFunc)(key);
}

- (void)flagsChanged:(NSEvent *)event
{
    static int controlDown;
    int nowDown = (event.modifierFlags & NSEventModifierFlagControl) != 0;
    if (nowDown && !controlDown && KeyDownFunc)
        (*KeyDownFunc)(TK_CONTROL_L, TK_CONTROL);
    else if (!nowDown && controlDown && KeyUpFunc)
        (*KeyUpFunc)(TK_CONTROL_L);
    controlDown = nowDown;
}

- (void)mouseDown:(NSEvent *)event
{
    [self updateMouse:event];
    if (MouseDownFunc)
        (*MouseDownFunc)(mouseX, mouseY, buttonMaskForEvent(event));
}

- (void)rightMouseDown:(NSEvent *)event { [self mouseDown:event]; }
- (void)otherMouseDown:(NSEvent *)event { [self mouseDown:event]; }

- (void)mouseUp:(NSEvent *)event
{
    [self updateMouse:event];
    if (MouseUpFunc)
        (*MouseUpFunc)(mouseX, mouseY, buttonMaskForEvent(event));
}

- (void)rightMouseUp:(NSEvent *)event { [self mouseUp:event]; }
- (void)otherMouseUp:(NSEvent *)event { [self mouseUp:event]; }

- (void)mouseMoved:(NSEvent *)event
{
    [self updateMouse:event];
    if (MouseMoveFunc)
        (*MouseMoveFunc)(mouseX, mouseY, 0);
}

- (void)mouseDragged:(NSEvent *)event
{
    [self updateMouse:event];
    if (MouseMoveFunc)
        (*MouseMoveFunc)(mouseX, mouseY, TK_LEFTBUTTON);
}

- (void)rightMouseDragged:(NSEvent *)event { [self mouseDragged:event]; }
- (void)otherMouseDragged:(NSEvent *)event { [self mouseDragged:event]; }

- (void)setFrameSize:(NSSize)newSize
{
    NSSize backingSize;

    [super setFrameSize:newSize];
    backingSize = [self convertSizeToBacking:newSize];
    self.drawableSize = CGSizeMake(backingSize.width, backingSize.height);
    rendererBackendResize((int)backingSize.width, (int)backingSize.height);
    if (ReshapeFunc)
        (*ReshapeFunc)((int)newSize.width, (int)newSize.height);
}

@end

@interface BattalionWindowDelegate : NSObject <NSWindowDelegate>
@end

@implementation BattalionWindowDelegate
- (void)windowWillClose:(NSNotification *)notification
{
    (void)notification;
    running = 0;
    metalView.paused = YES;
    [NSApp stop:nil];
}
- (void)windowDidMiniaturize:(NSNotification *)notification
{
    (void)notification;
    paused = 1;
}
@end

static BattalionWindowDelegate *windowDelegate;

@interface BattalionFrameDriver : NSObject <MTKViewDelegate>
@end

@implementation BattalionFrameDriver
- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size
{
    (void)view;
    rendererBackendResize((int)size.width, (int)size.height);
}

- (void)drawInMTKView:(MTKView *)view
{
    (void)view;
    if (!running)
        return;
    if (IdleFunc)
        (*IdleFunc)();
    else if (DisplayFunc)
        (*DisplayFunc)();
}
@end

static BattalionFrameDriver *frameDriver;

void tkInitDisplayMode(GLenum type)
{
    windowInfo.type = type;
}

void tkInitPosition(int x, int y, int width, int height)
{
    windowInfo.x = x;
    windowInfo.y = y;
    windowInfo.width = width;
    windowInfo.height = height;
}

GLenum tkInitWindow(char *title)
{
    NSRect frame;
    NSSize backingSize;
    NSWindowStyleMask style;
    id<MTLDevice> metalDevice;

    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    metalDevice = MTLCreateSystemDefaultDevice();
    if (!metalDevice) {
        fprintf(stderr, "Metal is not available on this Mac.\n");
        return GL_FALSE;
    }

    frame = NSMakeRect(windowInfo.x >= 0 ? windowInfo.x : 100,
                       windowInfo.y >= 0 ? windowInfo.y : 100,
                       windowInfo.width, windowInfo.height);
    style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
            NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
    mainWindow = [[NSWindow alloc] initWithContentRect:frame
                                             styleMask:style
                                               backing:NSBackingStoreBuffered
                                                 defer:NO];
    mainWindow.title = [NSString stringWithUTF8String:title];
    windowDelegate = [[BattalionWindowDelegate alloc] init];
    mainWindow.delegate = windowDelegate;

    metalView = [[BattalionMetalView alloc] initWithFrame:NSMakeRect(0, 0, windowInfo.width, windowInfo.height)
                                                   device:metalDevice];
    metalView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    metalView.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    metalView.paused = YES;
    metalView.enableSetNeedsDisplay = NO;
    metalView.preferredFramesPerSecond = 60;
    metalView.framebufferOnly = YES;
    metalView.autoResizeDrawable = YES;
    mainWindow.contentView = metalView;
    backingSize = [metalView convertSizeToBacking:metalView.bounds.size];
    metalView.drawableSize = CGSizeMake(backingSize.width, backingSize.height);
    [mainWindow setAcceptsMouseMovedEvents:YES];
    [mainWindow makeFirstResponder:metalView];
    [mainWindow makeKeyAndOrderFront:nil];

    [NSApp finishLaunching];
    [NSApp activateIgnoringOtherApps:YES];
    if (!rendererBackendInitialize((__bridge void *)metalView)) {
        [mainWindow close];
        return GL_FALSE;
    }
    rendererBackendResize((int)backingSize.width, (int)backingSize.height);
    mouseX = windowInfo.width / 2;
    mouseY = windowInfo.height / 2;
    if (ReshapeFunc)
        (*ReshapeFunc)(windowInfo.width, windowInfo.height);
    running = 1;
    return GL_TRUE;
}

void tkExec(void)
{
    frameDriver = [[BattalionFrameDriver alloc] init];
    metalView.delegate = frameDriver;
    metalView.paused = NO;
    [NSApp run];
    metalView.paused = YES;
    metalView.delegate = nil;
    frameDriver = nil;
}

void tkCloseWindow(void)
{
    if (pointerIsGrabbed)
        unGrabPointer();
    metalView.paused = YES;
    rendererBackendShutdown();
    [mainWindow close];
    mainWindow = nil;
    metalView = nil;
    running = 0;
}

void tkQuit(void)
{
    tkCloseWindow();
    exit(0);
}

void tkSwapBuffers(void)
{
    rendererBackendPresent();
}

void tkGetMouseLoc(int *x, int *y)
{
    *x = mouseX;
    *y = mouseY;
}

void grabPointer(void)
{
    if (pointerIsGrabbed)
        return;
    pointerIsGrabbed = 1;
    CGDisplayHideCursor(CGMainDisplayID());
    CGAssociateMouseAndMouseCursorPosition(0);
}

void unGrabPointer(void)
{
    if (!pointerIsGrabbed)
        return;
    CGAssociateMouseAndMouseCursorPosition(1);
    CGDisplayShowCursor(CGMainDisplayID());
    pointerIsGrabbed = 0;
}

void tkExposeFunc(void (*func)(int, int)) { ExposeFunc = func; }
void tkReshapeFunc(void (*func)(int, int)) { ReshapeFunc = func; }
void tkDisplayFunc(void (*func)(void)) { DisplayFunc = func; }
void tkKeyDownFunc(GLenum (*func)(int, GLenum)) { KeyDownFunc = func; }
void tkKeyUpFunc(GLenum (*func)(int)) { KeyUpFunc = func; }
void tkMouseDownFunc(GLenum (*func)(int, int, GLenum)) { MouseDownFunc = func; }
void tkMouseUpFunc(GLenum (*func)(int, int, GLenum)) { MouseUpFunc = func; }
void tkMouseMoveFunc(GLenum (*func)(int, int, GLenum)) { MouseMoveFunc = func; }
void tkIdleFunc(void (*func)(void)) { IdleFunc = func; }

int tkGetColorMapSize(void) { return 256; }
void tkSetOneColor(int index, float r, float g, float b) { (void)index; (void)r; (void)g; (void)b; }
void tkSetFogRamp(int density, int startIndex) { (void)density; (void)startIndex; }
void tkSetGreyRamp(void) {}
void tkSetRGBMap(int size, float *rgb) { (void)size; (void)rgb; }

float tkRGBMap[8][3] = {
    {0,0,0}, {1,0,0}, {0,1,0}, {1,1,0},
    {0,0,1}, {1,0,1}, {0,1,1}, {1,1,1}
};
