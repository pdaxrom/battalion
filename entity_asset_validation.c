/*
 * Copyright (c) 2026 Battalion contributors.
 */
/* Shared validation of catalog references against loaded runtime assets. */

#include "entity_asset_validation.h"

#include <stdio.h>
#include <string.h>

static int assetError(char *error, size_t errorSize,
                      const struct EntityArchetype *archetype,
                      const char *role, const char *asset)
    {
    char message[ENTITY_DESCRIPTION_SIZE];

    sprintf(message, "Entity '%s' has unavailable %s '%s'",
            archetype->id, role, asset);
    if (error != NULL && errorSize > 0)
        {
        strncpy(error, message, errorSize - 1);
        error[errorSize - 1] = '\0';
        }
    return 0;
    }

static int validateModelNodes(const struct EntityArchetype *archetype,
                              const struct AssetManifestModel *model,
                              const char *role,
                              char *error, size_t errorSize)
    {
    size_t index;

    if (archetype->aimYawNode[0] != '\0' &&
        assetModelFindNode(&model->model, archetype->aimYawNode) < 0)
        return assetError(error, errorSize, archetype,
                          "aim-yaw node", archetype->aimYawNode);
    if (archetype->aimPitchNode[0] != '\0' &&
        assetModelFindNode(&model->model, archetype->aimPitchNode) < 0)
        return assetError(error, errorSize, archetype,
                          "aim-pitch node", archetype->aimPitchNode);
    if (archetype->muzzleNode[0] != '\0' &&
        assetModelFindNode(&model->model, archetype->muzzleNode) < 0)
        return assetError(error, errorSize, archetype,
                          "muzzle node", archetype->muzzleNode);
    for (index = 0; index < archetype->renderNodeCount; index++)
        if (assetModelFindNode(&model->model,
                               archetype->renderNodes[index]) < 0)
            return assetError(error, errorSize, archetype,
                              role, archetype->renderNodes[index]);
    return 1;
    }

static int validateModel(const struct EntityArchetype *archetype,
                         const struct AssetManifest *manifest,
                         const char *key, const char *role,
                         char *error, size_t errorSize)
    {
    const struct AssetManifestModel *model;

    if (key[0] == '\0')
        return 1;
    model = assetManifestFindModel(manifest, key);
    if (model == NULL || model->displayList == 0)
        return assetError(error, errorSize, archetype, role, key);
    return validateModelNodes(archetype, model, "render node",
                              error, errorSize);
    }

static int validateAnimation(const struct EntityArchetype *archetype,
                             const struct AssetManifest *manifest,
                             const char *key, const char *role,
                             char *error, size_t errorSize)
    {
    const struct AssetManifestModel *model;

    if (key[0] == '\0')
        return 1;
    if (!assetManifestFindAnimation(manifest, key, &model, NULL, NULL))
        return assetError(error, errorSize, archetype, role, key);
    return validateModelNodes(archetype, model, "render node",
                              error, errorSize);
    }

static int validateTexture(const struct EntityArchetype *archetype,
                           const struct AssetManifest *manifest,
                           char *error, size_t errorSize)
    {
    size_t index;
    int nodeListed;

    if (archetype->texture[0] == '\0')
        return 1;
    if (assetManifestFindTexture(manifest, archetype->texture) == NULL)
        return assetError(error, errorSize, archetype,
                          "texture", archetype->texture);
    if (archetype->textureNode[0] == '\0')
        return assetError(error, errorSize, archetype,
                          "texture node", "(empty)");
    nodeListed = 0;
    for (index = 0; index < archetype->renderNodeCount; index++)
        if (strcmp(archetype->renderNodes[index],
                   archetype->textureNode) == 0)
            nodeListed = 1;
    if (!nodeListed)
        return assetError(error, errorSize, archetype,
                          "unrendered texture node", archetype->textureNode);
    return 1;
    }

static int validateArchetype(const struct EntityArchetype *archetype,
                             const struct AssetManifest *manifest,
                             char *error, size_t errorSize)
    {
    return validateModel(archetype, manifest, archetype->model,
                         "model", error, errorSize) &&
           validateModel(archetype, manifest, archetype->lowModel,
                         "low model", error, errorSize) &&
           validateModel(archetype, manifest, archetype->seasonalModel,
                         "seasonal model", error, errorSize) &&
           validateModel(archetype, manifest, archetype->seasonalLowModel,
                         "seasonal low model", error, errorSize) &&
           validateAnimation(archetype, manifest, archetype->idleAnimation,
                             "idle animation", error, errorSize) &&
           validateAnimation(archetype, manifest, archetype->lowAnimation,
                             "low animation", error, errorSize) &&
           validateAnimation(archetype, manifest,
                             archetype->seasonalAnimation,
                             "seasonal animation", error, errorSize) &&
           validateAnimation(archetype, manifest,
                             archetype->seasonalLowAnimation,
                             "seasonal low animation", error, errorSize) &&
           validateAnimation(archetype, manifest, archetype->moveAnimation,
                             "move animation", error, errorSize) &&
           validateAnimation(archetype, manifest,
                             archetype->backwardAnimation,
                             "backward animation", error, errorSize) &&
           validateAnimation(archetype, manifest, archetype->attackAnimation,
                             "attack animation", error, errorSize) &&
           validateTexture(archetype, manifest, error, errorSize);
    }

int entityCatalogValidateAssets(const struct EntityCatalog *catalog,
                                const struct AssetManifest *manifest,
                                char *error, size_t errorSize)
    {
    size_t index;

    if (catalog == NULL || manifest == NULL)
        {
        if (error != NULL && errorSize > 0)
            {
            strncpy(error, "Entity catalog and manifest are required",
                    errorSize - 1);
            error[errorSize - 1] = '\0';
            }
        return 0;
        }
    for (index = 0; index < catalog->count; index++)
        if (!validateArchetype(&catalog->entries[index], manifest,
                               error, errorSize))
            return 0;
    return 1;
    }
