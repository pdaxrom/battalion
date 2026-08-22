#!/bin/sh tools/irix-make -f Makefile.irix
##########################################################################
# This is a slightly better Makefile than before ... but its gonna need
# an overhaul very soon as its getting out of control.
#
# Makefile for v 1.3 of battalion (OpenGL/Mesa version)
#
# ajohnson@eecs.uic.edu
#
# if you are compiling this on an SGI the makefile should work fine,
# as it is. otherwise you may have to do a little bit of tweaking.
#
# the most important thing to know before you try and use this file is
# whether you have OpenGL or Brian Paul's Mesa graphics library installed
# on your machine. Without either of these you are not going to get very
# far.
#

# ------------------------------------------------------------

# Battalion build configuration
#
# Examples:
#   make                         # build for the host platform
#   make PLATFORM=linux
#   make PLATFORM=macos
#   make PLATFORM=sgi
#   make PLATFORM=linux AUDIO=none
#   make PLATFORM=linux GRAPHICS=mesa

.DEFAULT_GOAL := all

PROGRAM := battalion

CC ?= cc
PKG_CONFIG ?= pkg-config

HOST_OS := $(shell uname -s)
ifeq ($(HOST_OS),Darwin)
DEFAULT_PLATFORM := macos
else ifeq ($(HOST_OS),Linux)
DEFAULT_PLATFORM := linux
else ifeq ($(HOST_OS),SunOS)
DEFAULT_PLATFORM := solaris
else ifeq ($(HOST_OS),IRIX)
DEFAULT_PLATFORM := sgi
else ifeq ($(HOST_OS),IRIX64)
DEFAULT_PLATFORM := sgi
else ifeq ($(HOST_OS),HP-UX)
DEFAULT_PLATFORM := hpux
else ifeq ($(HOST_OS),AIX)
DEFAULT_PLATFORM := aix
else
DEFAULT_PLATFORM := unknown
endif

PLATFORM ?= $(DEFAULT_PLATFORM)
ifeq ($(PLATFORM),macos)
DEFAULT_GRAPHICS := metal
else
DEFAULT_GRAPHICS := opengl
endif
GRAPHICS ?= $(DEFAULT_GRAPHICS)

SUPPORTED_PLATFORMS := linux macos solaris sunos sgi hpux aix
SUPPORTED_GRAPHICS := opengl mesa metal gles

ifeq ($(filter $(PLATFORM),$(SUPPORTED_PLATFORMS)),)
$(error Unsupported PLATFORM '$(PLATFORM)'. Choose one of: $(SUPPORTED_PLATFORMS))
endif

ifeq ($(filter $(GRAPHICS),$(SUPPORTED_GRAPHICS)),)
$(error Unsupported GRAPHICS '$(GRAPHICS)'. Choose one of: $(SUPPORTED_GRAPHICS))
endif

ifeq ($(PLATFORM),macos)
ifneq ($(GRAPHICS),metal)
$(error PLATFORM=macos currently requires GRAPHICS=metal)
endif
else ifeq ($(GRAPHICS),metal)
$(error GRAPHICS=metal currently requires PLATFORM=macos)
endif

ifeq ($(GRAPHICS),gles)
$(error GRAPHICS=gles is reserved for the planned GLES backend and is not implemented yet)
endif

SOURCES := \
	battalion.c \
	map.c \
	audio.c \
	net.c \
	graphics.c \
	camera.c \
	camera_view.c \
	text.c \
	update.c \
	tk_x11.c \
	font.c \
	dynamic_primitives.c \
	map_package.c

SOURCES += asset.c asset_gltf.c asset_animation.c asset_skin.c asset_render.c \
	asset_manifest.c asset_bindings.c asset_tga.c asset_values.c stroke_font_asset.c \
	asset_png.c \
	audio_catalog.c entity_catalog.c entity_behavior.c world_coordinates.c

PLATFORM_CPPFLAGS :=
PLATFORM_LDFLAGS :=
PLATFORM_LDLIBS :=
DEFAULT_AUDIO := none

# Unix/X11 defaults. They can be overridden on the command line when the
# libraries live outside the compiler's normal search path.
X11_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags x11 xext 2>/dev/null)
X11_LIBS ?= $(shell $(PKG_CONFIG) --libs x11 xext 2>/dev/null)
OPENGL_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags gl glu 2>/dev/null)
OPENGL_LIBS ?= $(shell $(PKG_CONFIG) --libs gl glu 2>/dev/null)

ifeq ($(strip $(X11_LIBS)),)
X11_LIBS := -lX11 -lXext
endif
ifeq ($(strip $(OPENGL_LIBS)),)
OPENGL_LIBS := -lGL -lGLU
endif

ifeq ($(PLATFORM),linux)
PLATFORM_CPPFLAGS += -DLINUXVERSION $(X11_CFLAGS) $(OPENGL_CFLAGS)
PLATFORM_LDLIBS += $(OPENGL_LIBS) $(X11_LIBS) -lm
DEFAULT_AUDIO := oss
endif

# macOS uses a native Cocoa window and the Metal renderer. The fixed-function
# compatibility frontend is shared with the planned GLES backend.
ifeq ($(PLATFORM),macos)
SOURCES := $(filter-out tk_x11.c,$(SOURCES))
SOURCES += tk_macos.m renderer/fixed_gl.c renderer/metal.m
PLATFORM_CPPFLAGS += -DMACOSVERSION -DRENDERER_METAL
PLATFORM_LDLIBS += -framework AppKit -framework Metal -framework MetalKit \
	-framework QuartzCore -framework CoreGraphics -lm
endif

ifeq ($(PLATFORM),solaris)
PLATFORM_CPPFLAGS += -DSOLARIS $(X11_CFLAGS) $(OPENGL_CFLAGS)
PLATFORM_LDLIBS += $(OPENGL_LIBS) $(X11_LIBS) -lnsl -lsocket -lm
endif

ifeq ($(PLATFORM),sunos)
PLATFORM_CPPFLAGS += -DSUNVERSION $(X11_CFLAGS) $(OPENGL_CFLAGS)
PLATFORM_LDLIBS += $(OPENGL_LIBS) $(X11_LIBS) -lm
endif

ifeq ($(PLATFORM),sgi)
PLATFORM_CPPFLAGS += -DSGIVERSION $(X11_CFLAGS) $(OPENGL_CFLAGS)
PLATFORM_LDLIBS += $(OPENGL_LIBS) $(X11_LIBS) -lm
endif

ifeq ($(PLATFORM),hpux)
PLATFORM_CPPFLAGS += -DHPVERSION $(X11_CFLAGS) $(OPENGL_CFLAGS)
PLATFORM_LDLIBS += $(OPENGL_LIBS) $(X11_LIBS) -lm
endif

ifeq ($(PLATFORM),aix)
PLATFORM_CPPFLAGS += -DAIXVERSION $(X11_CFLAGS) $(OPENGL_CFLAGS)
PLATFORM_LDLIBS += $(OPENGL_LIBS) $(X11_LIBS) -lm
endif

ifeq ($(GRAPHICS),mesa)
GRAPHICS_CPPFLAGS := -DMESAVERSION
endif

AUDIO ?= $(DEFAULT_AUDIO)
SUPPORTED_AUDIO := none oss sun sgi

ifeq ($(filter $(AUDIO),$(SUPPORTED_AUDIO)),)
$(error Unsupported AUDIO '$(AUDIO)'. Choose one of: $(SUPPORTED_AUDIO))
endif

ifeq ($(AUDIO),oss)
ifneq ($(PLATFORM),linux)
$(error AUDIO=oss is only supported with PLATFORM=linux)
endif
AUDIO_CPPFLAGS := -DLINUXAUDIO
SOURCES += soundIt.c
endif

ifeq ($(AUDIO),sun)
ifneq ($(filter $(PLATFORM),solaris sunos),$(PLATFORM))
$(error AUDIO=sun requires PLATFORM=solaris or PLATFORM=sunos)
endif
SUN_AUDIO_PREFIX ?= /usr/demo/SOUND
AUDIO_CPPFLAGS := -DSUNAUDIO -I$(SUN_AUDIO_PREFIX)/include
AUDIO_LDFLAGS := -L$(SUN_AUDIO_PREFIX)/lib
AUDIO_LDLIBS := -laudio
endif

ifeq ($(AUDIO),sgi)
ifneq ($(PLATFORM),sgi)
$(error AUDIO=sgi requires PLATFORM=sgi)
endif
AUDIO_CPPFLAGS := -DSGIAUDIO
AUDIO_LDLIBS := -laudio -laudiofile
endif

BUILD_DIR ?= build/$(PLATFORM)-$(GRAPHICS)-$(AUDIO)
TARGET := $(BUILD_DIR)/$(PROGRAM)
OBJECTS := $(addprefix $(BUILD_DIR)/,$(SOURCES))
OBJECTS := $(OBJECTS:.c=.o)
OBJECTS := $(OBJECTS:.m=.o)
DEPS := $(OBJECTS:.o=.d)

# The C engine targets ISO C89 and defines tentative globals in headers.
# Objective-C uses separate flags because current Apple framework headers use
# newer language features. Historical vendor compilers receive neither set.
CC_VERSION := $(shell $(CC) --version 2>/dev/null)
ifneq ($(or $(findstring clang,$(CC_VERSION)),$(findstring GCC,$(CC_VERSION)),$(findstring gcc,$(CC_VERSION))),)
COMPAT_CFLAGS ?= -std=c89 -fcommon
COMPAT_OBJCFLAGS ?= -fcommon
DEPFLAGS ?= -MMD -MP
C89_CHECK_FLAGS ?= -std=c89 -pedantic-errors -fsyntax-only \
	-Wno-strict-prototypes -Wno-dangling-else -Wno-pointer-sign \
	-Wno-pointer-to-int-cast -Wno-int-to-pointer-cast
else
COMPAT_CFLAGS ?=
COMPAT_OBJCFLAGS ?=
DEPFLAGS ?=
C89_CHECK_FLAGS ?=
endif
CFLAGS ?= -O2

.PHONY: all clean help print-config test c89-check

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(PLATFORM_LDFLAGS) $(AUDIO_LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS) $(PLATFORM_LDLIBS) $(AUDIO_LDLIBS)

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(PLATFORM_CPPFLAGS) $(GRAPHICS_CPPFLAGS) $(AUDIO_CPPFLAGS) $(COMPAT_CFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.m Makefile | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(PLATFORM_CPPFLAGS) $(GRAPHICS_CPPFLAGS) $(AUDIO_CPPFLAGS) $(COMPAT_OBJCFLAGS) $(CFLAGS) $(DEPFLAGS) -fobjc-arc -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

print-config:
	@echo "host:       $(HOST_OS)"
	@echo "platform:   $(PLATFORM)"
	@echo "graphics:   $(GRAPHICS)"
	@echo "audio:      $(AUDIO)"
	@echo "assets:     external (assets/manifest.txt)"
	@echo "maps:       external JSON packages (required)"
	@echo "compiler:   $(CC)"
	@echo "output:     $(TARGET)"

help:
	@echo "Usage: make [PLATFORM=<platform>] [GRAPHICS=<graphics>] [AUDIO=<audio>]"
	@echo
	@echo "Platforms: $(SUPPORTED_PLATFORMS)"
	@echo "Graphics:  $(SUPPORTED_GRAPHICS)"
	@echo "Audio:     $(SUPPORTED_AUDIO)"
	@echo "Assets:    external manifest (required)"
	@echo "Maps:      external JSON packages (required)"
	@echo
	@echo "Examples:"
	@echo "  make PLATFORM=linux"
	@echo "  make PLATFORM=macos"
	@echo "  make PLATFORM=sgi"
	@echo "  make PLATFORM=sgi AUDIO=sgi"
	@echo "  make PLATFORM=linux AUDIO=none"
	@echo "  make PLATFORM=linux GRAPHICS=mesa"
	@echo
	@echo "macOS uses the native Metal backend; XQuartz and OpenGL are not required."
	@echo "GRAPHICS=gles is part of the renderer design but is not implemented yet."

c89-check:
	@if test -n "$(C89_CHECK_FLAGS)"; then \
		$(CC) $(CPPFLAGS) $(PLATFORM_CPPFLAGS) $(GRAPHICS_CPPFLAGS) \
			$(AUDIO_CPPFLAGS) $(C89_CHECK_FLAGS) $(filter %.c,$(SOURCES)); \
	else \
		echo "C89 syntax check skipped: compiler has no known strict-C89 flags"; \
	fi

TEST_BUILD_DIR := build/tests
MAP_TEST := $(TEST_BUILD_DIR)/map-test
ASSET_TEST := $(TEST_BUILD_DIR)/asset-gltf-test
ASSET_SKIN_TEST := $(TEST_BUILD_DIR)/asset-skin-test
ASSET_CATALOG_TEST := $(TEST_BUILD_DIR)/asset-catalog-test
ASSET_VALUES_TEST := $(TEST_BUILD_DIR)/asset-values-test
STROKE_FONT_TEST := $(TEST_BUILD_DIR)/stroke-font-test
AUDIO_CATALOG_TEST := $(TEST_BUILD_DIR)/audio-catalog-test
ASSET_BINDINGS_TEST := $(TEST_BUILD_DIR)/asset-bindings-test
ENTITY_CATALOG_TEST := $(TEST_BUILD_DIR)/entity-catalog-test
ENTITY_BEHAVIOR_TEST := $(TEST_BUILD_DIR)/entity-behavior-test
NO_EMBEDDED_ASSETS_TEST := $(TEST_BUILD_DIR)/no-embedded-assets-test
WORLD_COORDINATES_TEST := $(TEST_BUILD_DIR)/world-coordinates-test
CAMERA_VIEW_TEST := $(TEST_BUILD_DIR)/camera-view-test

$(MAP_TEST): tests/map_test.c map.c map.h map_package.c map_package.h entity_behavior.c entity_behavior.h entity_catalog.c entity_catalog.h Makefile | $(TEST_BUILD_DIR)
	$(CC) -I. $(COMPAT_CFLAGS) $(CFLAGS) $< map.c map_package.c entity_behavior.c entity_catalog.c -lm -o $@

$(ASSET_TEST): tests/asset_gltf_test.c asset.c asset_gltf.c asset_animation.c asset_skin.c asset_write.c asset_tga.c asset_png.c asset.h asset_animation.h asset_skin.h asset_write.h asset_tga.h asset_png.h Makefile | $(TEST_BUILD_DIR)
	$(CC) -I. $(COMPAT_CFLAGS) $(CFLAGS) $< asset.c asset_gltf.c asset_animation.c asset_skin.c asset_write.c asset_tga.c asset_png.c -lm -o $@

$(ASSET_SKIN_TEST): tests/asset_skin_test.c asset.c asset_gltf.c asset_animation.c asset_skin.c entity_catalog.c asset_png.c asset_tga.c asset.h asset_animation.h asset_skin.h entity_catalog.h asset_png.h asset_tga.h Makefile | $(TEST_BUILD_DIR)
	$(CC) -I. $(COMPAT_CFLAGS) $(CFLAGS) $< asset.c asset_gltf.c asset_animation.c asset_skin.c entity_catalog.c asset_png.c asset_tga.c -lm -o $@

$(ASSET_CATALOG_TEST): tests/asset_catalog_test.c Makefile | $(TEST_BUILD_DIR)
	$(CC) -I. $(COMPAT_CFLAGS) $(CFLAGS) $< -o $@

$(ASSET_VALUES_TEST): tests/asset_values_test.c asset_values.c asset_values.h Makefile | $(TEST_BUILD_DIR)
	$(CC) -I. $(COMPAT_CFLAGS) $(CFLAGS) $< asset_values.c -lm -o $@

$(STROKE_FONT_TEST): tests/stroke_font_test.c stroke_font_asset.c stroke_font_asset.h Makefile | $(TEST_BUILD_DIR)
	$(CC) -I. $(COMPAT_CFLAGS) $(CFLAGS) $< stroke_font_asset.c -o $@

$(AUDIO_CATALOG_TEST): tests/audio_catalog_test.c audio_catalog.c audio_catalog.h Makefile | $(TEST_BUILD_DIR)
	$(CC) -I. $(COMPAT_CFLAGS) $(CFLAGS) $< audio_catalog.c -o $@

$(ASSET_BINDINGS_TEST): tests/asset_bindings_test.c asset_bindings.c asset_bindings.h Makefile | $(TEST_BUILD_DIR)
	$(CC) -I. $(COMPAT_CFLAGS) $(CFLAGS) $< asset_bindings.c -o $@

$(ENTITY_CATALOG_TEST): tests/entity_catalog_test.c entity_catalog.c entity_catalog.h Makefile | $(TEST_BUILD_DIR)
	$(CC) -I. $(COMPAT_CFLAGS) $(CFLAGS) $< entity_catalog.c -o $@

$(ENTITY_BEHAVIOR_TEST): tests/entity_behavior_test.c entity_behavior.c entity_behavior.h entity_catalog.c entity_catalog.h Makefile | $(TEST_BUILD_DIR)
	$(CC) -I. $(COMPAT_CFLAGS) $(CFLAGS) $< entity_behavior.c entity_catalog.c -lm -o $@

$(NO_EMBEDDED_ASSETS_TEST): tests/no_embedded_assets_test.c Makefile | $(TEST_BUILD_DIR)
	$(CC) -I. $(COMPAT_CFLAGS) $(CFLAGS) $< -o $@

$(WORLD_COORDINATES_TEST): tests/world_coordinates_test.c world_coordinates.c world_coordinates.h Makefile | $(TEST_BUILD_DIR)
	$(CC) -I. $(COMPAT_CFLAGS) $(CFLAGS) $< world_coordinates.c -o $@

$(CAMERA_VIEW_TEST): tests/camera_view_test.c camera_view.c camera_view.h Makefile | $(TEST_BUILD_DIR)
	$(CC) -I. $(COMPAT_CFLAGS) $(CFLAGS) $< camera_view.c -lm -o $@

$(TEST_BUILD_DIR):
	mkdir -p $@

test: c89-check $(MAP_TEST) $(ASSET_TEST) $(ASSET_SKIN_TEST) $(ASSET_CATALOG_TEST) $(ASSET_VALUES_TEST) $(STROKE_FONT_TEST) $(AUDIO_CATALOG_TEST) $(ASSET_BINDINGS_TEST) $(ENTITY_CATALOG_TEST) $(ENTITY_BEHAVIOR_TEST) $(NO_EMBEDDED_ASSETS_TEST) $(WORLD_COORDINATES_TEST) $(CAMERA_VIEW_TEST)
	./$(MAP_TEST)
	./$(ASSET_TEST)
	./$(ASSET_SKIN_TEST)
	./$(ASSET_CATALOG_TEST)
	./$(ASSET_VALUES_TEST)
	./$(STROKE_FONT_TEST)
	./$(AUDIO_CATALOG_TEST)
	./$(ASSET_BINDINGS_TEST)
	./$(ENTITY_CATALOG_TEST)
	./$(ENTITY_BEHAVIOR_TEST)
	./$(NO_EMBEDDED_ASSETS_TEST)
	./$(WORLD_COORDINATES_TEST)
	./$(CAMERA_VIEW_TEST)

clean:
	rm -rf build

-include $(DEPS)
