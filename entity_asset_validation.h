/*
 * Copyright (c) 2026 Battalion contributors.
 */

#ifndef BATTALION_ENTITY_ASSET_VALIDATION_H
#define BATTALION_ENTITY_ASSET_VALIDATION_H

#include "asset_manifest.h"
#include "entity_catalog.h"

#include <stddef.h>

int entityCatalogValidateAssets(const struct EntityCatalog *catalog,
                                const struct AssetManifest *manifest,
                                char *error, size_t errorSize);

#endif
