/****************************************************************/
/* code copyright 1995-1996 Andrew Johnson - ALL RIGHTS RESERVED*/
/*                          ajohnson@eecs.uic.edu               */
/*                                                              */
/*                      Electronic Visualization Lab (M/C 154)  */
/*                      University of Illinois at Chicago       */
/*                      851 South Morgan St. Room 1120 SEO      */
/*                      Chicago, IL 60607-7053                  */
/*                                                              */
/*                      (312) 996-3002     (312) 413-7585 FAX   */
/***********************************************************************/

#ifndef BATTALION_ASSET_MANIFEST_H
#define BATTALION_ASSET_MANIFEST_H

#include "asset.h"

#include <stddef.h>

struct AssetManifestModel
    {
    char *key;
    char *path;
    struct AssetModel model;
    unsigned int displayList;
    };

struct AssetManifestTexture
    {
    char *key;
    char *path;
    int linear;
    int repeat;
    unsigned int displayList;
    };

struct AssetManifestAnimation
    {
    char *key;
    char *modelKey;
    char *clipName;
    int loop;
    };

struct AssetManifestFile
    {
    char *key;
    char *path;
    };

struct AssetManifest
    {
    struct AssetManifestModel *models;
    size_t modelCount;
    struct AssetManifestTexture *textures;
    size_t textureCount;
    struct AssetManifestAnimation *animations;
    size_t animationCount;
    struct AssetManifestFile *files;
    size_t fileCount;
    };

void assetManifestInitialize(struct AssetManifest *manifest);
void assetManifestDestroy(struct AssetManifest *manifest);
int assetManifestLoad(const char *dataPath, struct AssetManifest *manifest,
                      char *error, size_t errorSize);
const struct AssetManifestModel *assetManifestFindModel(
    const struct AssetManifest *manifest, const char *key);
const struct AssetManifestTexture *assetManifestFindTexture(
    const struct AssetManifest *manifest, const char *key);
const struct AssetManifestFile *assetManifestFindFile(
    const struct AssetManifest *manifest, const char *key);
int assetManifestFindAnimation(const struct AssetManifest *manifest,
                               const char *key,
                               const struct AssetManifestModel **model,
                               size_t *clip, int *loop);

#endif
