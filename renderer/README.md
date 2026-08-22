# Renderer backends

The original X11/GLX platforms continue to compile the original fixed-function
OpenGL renderer directly. macOS uses a compatibility frontend instead:

```text
game gl*/glu* calls
        |
        v
fixed_gl.c (state, matrices, display lists, primitive conversion)
        |
        v
backend.h (vertex batches, textures, clear/present)
        |
        +---- metal.m (implemented)
        |
        +---- GLES backend (planned)
```

`backend.h` is deliberately API-neutral. A backend receives triangles, lines,
or points with an MVP matrix and the small state set used by Battalion. It does
not need to implement the fixed-function API or display lists.

## GLES plan

1. Add `renderer/gles.c` implementing `backend.h` with GLES 3 shaders, VBO
   streaming, luminance textures (`GL_R8`), depth, blending, culling, and
   wireframe emulation for triangle batches.
2. Add an EGL window/input backend (or a platform-specific native surface)
   beside `tk_x11.c` and `tk_macos.m`.
3. Wire `PLATFORM=<platform> GRAPHICS=gles` to `fixed_gl.c`, the GLES driver,
   and the matching window backend. The Makefile already reserves and validates
   the `gles` graphics value so it cannot silently fall back to desktop OpenGL.
4. Run the same renderer smoke scene through Metal and GLES and compare the
   resulting frame captures, then exercise the full demo loop and input path.

Keeping primitive conversion and display-list playback in `fixed_gl.c` means
the GLES work is a driver implementation, not a second port of the game.
