/*
 * Copyright (c) 2026 Battalion contributors.
 */

#include "asset_manifest.h"
#include "entity_asset_validation.h"
#include "entity_catalog.h"
#include "renderer/backend.h"

#include <stdio.h>

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
    (void)vertices;
    (void)vertexCount;
    (void)state;
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
    struct AssetManifest manifest;
    struct EntityCatalog catalog;
    char error[256];
    int valid;

    assetManifestInitialize(&manifest);
    entityCatalogInitialize(&catalog);
    valid = assetManifestLoad("assets/", &manifest, error, sizeof(error)) &&
            entityCatalogLoad("assets", "entities/catalog.txt", &catalog,
                              error, sizeof(error)) &&
            entityCatalogValidateAssets(&catalog, &manifest,
                                        error, sizeof(error));
    if (!valid)
        fprintf(stderr, "%s\n", error);
    entityCatalogDestroy(&catalog);
    assetManifestDestroy(&manifest);
    return valid ? 0 : 1;
    }
