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
/* External asset manifest and eager runtime asset loading.             */
/***********************************************************************/

#include "battalion.h"
#include "asset_manifest.h"
#include "asset_render.h"
#include "asset_tga.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSET_MANIFEST_FILE "manifest.txt"

static void setError(char *error, size_t errorSize, const char *message)
    {
    if (error == NULL || errorSize == 0)
        return;
    strncpy(error, message, errorSize - 1);
    error[errorSize - 1] = '\0';
    }

static void setEntryError(char *error, size_t errorSize, const char *prefix,
                          const char *entry)
    {
    size_t used;

    if (error == NULL || errorSize == 0)
        return;
    strncpy(error, prefix, errorSize - 1);
    error[errorSize - 1] = '\0';
    used = strlen(error);
    if (used + 2 < errorSize)
        {
        error[used++] = ':';
        error[used++] = ' ';
        error[used] = '\0';
        }
    if (used + 1 < errorSize)
        strncat(error, entry, errorSize - used - 1);
    }

static char *copyString(const char *text)
    {
    char *copy;
    size_t length;

    length = strlen(text);
    copy = (char *)malloc(length + 1);
    if (copy != NULL)
        memcpy(copy, text, length + 1);
    return copy;
    }

static int buildPath(char *path, size_t pathSize, const char *dataPath,
                     const char *relative)
    {
    size_t dataLength;
    size_t relativeLength;

    dataLength = strlen(dataPath);
    relativeLength = strlen(relative);
    if (dataLength + relativeLength + 1 > pathSize)
        return 0;
    memcpy(path, dataPath, dataLength);
    memcpy(path + dataLength, relative, relativeLength + 1);
    return 1;
    }

static int validAssetName(const char *text, int path)
    {
    const unsigned char *cursor;

    if (text == NULL || text[0] == '\0' || text[0] == '/' ||
        (path && strstr(text, "..") != NULL))
        return 0;
    for (cursor = (const unsigned char *)text; *cursor; cursor++)
        if (isupper(*cursor) || *cursor == '\\' ||
            !(islower(*cursor) || isdigit(*cursor) || *cursor == '-' ||
              *cursor == '_' || *cursor == '.' || (path && *cursor == '/')))
            return 0;
    return 1;
    }

static int validClipName(const char *text)
    {
    const unsigned char *cursor;

    if (text == NULL || text[0] == '\0')
        return 0;
    for (cursor = (const unsigned char *)text; *cursor; cursor++)
        if (*cursor < 33 || *cursor > 126 || *cursor == '\\')
            return 0;
    return 1;
    }

void assetManifestInitialize(struct AssetManifest *manifest)
    {
    manifest->models = NULL;
    manifest->modelCount = 0;
    manifest->textures = NULL;
    manifest->textureCount = 0;
    manifest->animations = NULL;
    manifest->animationCount = 0;
    manifest->files = NULL;
    manifest->fileCount = 0;
    }

void assetManifestDestroy(struct AssetManifest *manifest)
    {
    size_t index;

    if (manifest == NULL)
        return;
    for (index = 0; index < manifest->modelCount; index++)
        {
        free(manifest->models[index].key);
        free(manifest->models[index].path);
        assetModelDestroy(&manifest->models[index].model);
        }
    for (index = 0; index < manifest->textureCount; index++)
        {
        free(manifest->textures[index].key);
        free(manifest->textures[index].path);
        }
    for (index = 0; index < manifest->animationCount; index++)
        {
        free(manifest->animations[index].key);
        free(manifest->animations[index].modelKey);
        free(manifest->animations[index].clipName);
        }
    for (index = 0; index < manifest->fileCount; index++)
        {
        free(manifest->files[index].key);
        free(manifest->files[index].path);
        }
    free(manifest->models);
    free(manifest->textures);
    free(manifest->animations);
    free(manifest->files);
    assetManifestInitialize(manifest);
    }

const struct AssetManifestModel *assetManifestFindModel(
    const struct AssetManifest *manifest, const char *key)
    {
    size_t index;

    for (index = 0; index < manifest->modelCount; index++)
        if (strcmp(manifest->models[index].key, key) == 0)
            return &manifest->models[index];
    return NULL;
    }

const struct AssetManifestTexture *assetManifestFindTexture(
    const struct AssetManifest *manifest, const char *key)
    {
    size_t index;

    for (index = 0; index < manifest->textureCount; index++)
        if (strcmp(manifest->textures[index].key, key) == 0)
            return &manifest->textures[index];
    return NULL;
    }

const struct AssetManifestFile *assetManifestFindFile(
    const struct AssetManifest *manifest, const char *key)
    {
    size_t index;

    for (index = 0; index < manifest->fileCount; index++)
        if (strcmp(manifest->files[index].key, key) == 0)
            return &manifest->files[index];
    return NULL;
    }

static int appendModel(struct AssetManifest *manifest, const char *dataPath,
                       const char *key, const char *relative,
                       char *error, size_t errorSize)
    {
    struct AssetManifestModel *expanded;
    struct AssetManifestModel *entry;
    char path[MAXPATH];
    size_t count;

    if (!validAssetName(key, 0) || !validAssetName(relative, 1))
        {
        setEntryError(error, errorSize, "Invalid lowercase model entry", key);
        return 0;
        }
    if (assetManifestFindModel(manifest, key) != NULL)
        {
        setEntryError(error, errorSize, "Duplicate model key", key);
        return 0;
        }
    if (!buildPath(path, sizeof(path), dataPath, relative))
        {
        setEntryError(error, errorSize, "Model path is too long", relative);
        return 0;
        }
    count = manifest->modelCount + 1;
    expanded = (struct AssetManifestModel *)realloc(
        manifest->models, count * sizeof(*expanded));
    if (expanded == NULL)
        {
        setError(error, errorSize, "Out of memory adding manifest model");
        return 0;
        }
    manifest->models = expanded;
    entry = &manifest->models[manifest->modelCount];
    memset(entry, 0, sizeof(*entry));
    assetModelInitialize(&entry->model);
    entry->key = copyString(key);
    entry->path = copyString(relative);
    if (entry->key == NULL || entry->path == NULL)
        {
        free(entry->key);
        free(entry->path);
        setError(error, errorSize, "Out of memory copying manifest model");
        return 0;
        }
    if (!assetModelLoadGLB(path, &entry->model, error, errorSize))
        {
        free(entry->key);
        free(entry->path);
        entry->key = entry->path = NULL;
        return 0;
        }
    entry->displayList = assetModelCreateDisplayList(&entry->model);
    if (entry->displayList == 0)
        {
        assetModelDestroy(&entry->model);
        free(entry->key);
        free(entry->path);
        entry->key = entry->path = NULL;
        setEntryError(error, errorSize,
                      "Could not create display list for model", key);
        return 0;
        }
    manifest->modelCount = count;
    return 1;
    }

static int appendTexture(struct AssetManifest *manifest, const char *dataPath,
                         const char *key, const char *relative,
                         const char *filter, const char *wrap,
                         char *error, size_t errorSize)
    {
    struct AssetManifestTexture *expanded;
    struct AssetManifestTexture *entry;
    struct AssetImage image;
    char path[MAXPATH];
    size_t count;

    if (!validAssetName(key, 0) || !validAssetName(relative, 1))
        {
        setEntryError(error, errorSize,
                      "Invalid lowercase texture entry", key);
        return 0;
        }
    if (assetManifestFindTexture(manifest, key) != NULL)
        {
        setEntryError(error, errorSize, "Duplicate texture key", key);
        return 0;
        }
    if (strcmp(filter, "nearest") != 0 && strcmp(filter, "linear") != 0)
        {
        setEntryError(error, errorSize, "Invalid texture filter", filter);
        return 0;
        }
    if (strcmp(wrap, "clamp") != 0 && strcmp(wrap, "repeat") != 0)
        {
        setEntryError(error, errorSize, "Invalid texture wrap", wrap);
        return 0;
        }
    if (!buildPath(path, sizeof(path), dataPath, relative))
        {
        setEntryError(error, errorSize, "Texture path is too long", relative);
        return 0;
        }
    assetImageInitialize(&image);
    if (!assetImageLoadTGA(path, &image, error, errorSize))
        return 0;
    if (!assetImageMakePowerOfTwo(&image, error, errorSize))
        {
        assetImageDestroy(&image);
        return 0;
        }
    count = manifest->textureCount + 1;
    expanded = (struct AssetManifestTexture *)realloc(
        manifest->textures, count * sizeof(*expanded));
    if (expanded == NULL)
        {
        assetImageDestroy(&image);
        setError(error, errorSize, "Out of memory adding manifest texture");
        return 0;
        }
    manifest->textures = expanded;
    entry = &manifest->textures[manifest->textureCount];
    memset(entry, 0, sizeof(*entry));
    entry->key = copyString(key);
    entry->path = copyString(relative);
    entry->linear = strcmp(filter, "linear") == 0;
    entry->repeat = strcmp(wrap, "repeat") == 0;
    if (entry->key == NULL || entry->path == NULL)
        {
        free(entry->key);
        free(entry->path);
        assetImageDestroy(&image);
        setError(error, errorSize, "Out of memory copying manifest texture");
        return 0;
        }
    entry->displayList = glGenLists(1);
    glNewList(entry->displayList, GL_COMPILE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, (const GLvoid *)image.pixels);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    entry->repeat ? GL_REPEAT : GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                    entry->repeat ? GL_REPEAT : GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    entry->linear ? GL_LINEAR : GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    entry->linear ? GL_LINEAR : GL_NEAREST);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEndList();
    assetImageDestroy(&image);
    if (entry->displayList == 0)
        {
        free(entry->key);
        free(entry->path);
        setEntryError(error, errorSize,
                      "Could not create display list for texture", key);
        return 0;
        }
    manifest->textureCount = count;
    return 1;
    }

static int appendAnimation(struct AssetManifest *manifest, const char *key,
                           const char *modelKey, const char *clipName,
                           const char *mode, char *error, size_t errorSize)
    {
    struct AssetManifestAnimation *expanded;
    struct AssetManifestAnimation *entry;
    size_t index;
    size_t count;

    if (!validAssetName(key, 0) || !validAssetName(modelKey, 0) ||
        !validClipName(clipName))
        {
        setEntryError(error, errorSize,
                      "Invalid animation entry", key);
        return 0;
        }
    for (index = 0; index < manifest->animationCount; index++)
        if (strcmp(manifest->animations[index].key, key) == 0)
            {
            setEntryError(error, errorSize, "Duplicate animation key", key);
            return 0;
            }
    if (strcmp(mode, "loop") != 0 && strcmp(mode, "once") != 0)
        {
        setEntryError(error, errorSize, "Invalid animation mode", mode);
        return 0;
        }
    count = manifest->animationCount + 1;
    expanded = (struct AssetManifestAnimation *)realloc(
        manifest->animations, count * sizeof(*expanded));
    if (expanded == NULL)
        {
        setError(error, errorSize, "Out of memory adding manifest animation");
        return 0;
        }
    manifest->animations = expanded;
    entry = &manifest->animations[manifest->animationCount];
    memset(entry, 0, sizeof(*entry));
    entry->key = copyString(key);
    entry->modelKey = copyString(modelKey);
    entry->clipName = copyString(clipName);
    entry->loop = strcmp(mode, "loop") == 0;
    if (entry->key == NULL || entry->modelKey == NULL ||
        entry->clipName == NULL)
        {
        free(entry->key);
        free(entry->modelKey);
        free(entry->clipName);
        setError(error, errorSize,
                 "Out of memory copying manifest animation");
        return 0;
        }
    manifest->animationCount = count;
    return 1;
    }

static int appendFile(struct AssetManifest *manifest, const char *dataPath,
                      const char *key, const char *relative,
                      char *error, size_t errorSize)
    {
    struct AssetManifestFile *expanded;
    struct AssetManifestFile *entry;
    char path[MAXPATH];
    FILE *file;
    size_t count;

    if (!validAssetName(key, 0) || !validAssetName(relative, 1))
        {
        setEntryError(error, errorSize, "Invalid lowercase file entry", key);
        return 0;
        }
    if (assetManifestFindFile(manifest, key) != NULL)
        {
        setEntryError(error, errorSize, "Duplicate file key", key);
        return 0;
        }
    if (!buildPath(path, sizeof(path), dataPath, relative))
        {
        setEntryError(error, errorSize, "Asset file path is too long", relative);
        return 0;
        }
    file = fopen(path, "rb");
    if (file == NULL)
        {
        setEntryError(error, errorSize, "Could not open asset file", relative);
        return 0;
        }
    fclose(file);
    count = manifest->fileCount + 1;
    expanded = (struct AssetManifestFile *)realloc(
        manifest->files, count * sizeof(*expanded));
    if (expanded == NULL)
        {
        setError(error, errorSize, "Out of memory adding manifest file");
        return 0;
        }
    manifest->files = expanded;
    entry = &manifest->files[manifest->fileCount];
    memset(entry, 0, sizeof(*entry));
    entry->key = copyString(key);
    entry->path = copyString(relative);
    if (entry->key == NULL || entry->path == NULL)
        {
        free(entry->key);
        free(entry->path);
        setError(error, errorSize, "Out of memory copying manifest file");
        return 0;
        }
    manifest->fileCount = count;
    return 1;
    }

int assetManifestFindAnimation(const struct AssetManifest *manifest,
                               const char *key,
                               const struct AssetManifestModel **model,
                               size_t *clip, int *loop)
    {
    const struct AssetManifestAnimation *animation;
    const struct AssetManifestModel *modelEntry;
    size_t animationIndex;
    size_t clipIndex;

    for (animationIndex = 0; animationIndex < manifest->animationCount;
         animationIndex++)
        {
        animation = &manifest->animations[animationIndex];
        if (strcmp(animation->key, key) != 0)
            continue;
        modelEntry = assetManifestFindModel(manifest, animation->modelKey);
        if (modelEntry == NULL)
            return 0;
        for (clipIndex = 0; clipIndex < modelEntry->model.animationCount;
             clipIndex++)
            if (modelEntry->model.animations[clipIndex].name != NULL &&
                strcmp(modelEntry->model.animations[clipIndex].name,
                       animation->clipName) == 0)
                {
                if (model != NULL)
                    *model = modelEntry;
                if (clip != NULL)
                    *clip = clipIndex;
                if (loop != NULL)
                    *loop = animation->loop;
                return 1;
                }
        return 0;
        }
    return 0;
    }

static int validateAnimations(const struct AssetManifest *manifest,
                              char *error, size_t errorSize)
    {
    size_t index;

    for (index = 0; index < manifest->animationCount; index++)
        if (!assetManifestFindAnimation(manifest,
                                        manifest->animations[index].key,
                                        NULL, NULL, NULL))
            {
            setEntryError(error, errorSize,
                          "Animation references a missing model or clip",
                          manifest->animations[index].key);
            return 0;
            }
    return 1;
    }

int assetManifestLoad(const char *dataPath, struct AssetManifest *manifest,
                      char *error, size_t errorSize)
    {
    char manifestPath[MAXPATH];
    char line[1024];
    char *directive;
    char *key;
    char *path;
    char *option1;
    char *option2;
    FILE *file;
    int sawHeader;
    int lineNumber;

    if (dataPath == NULL || manifest == NULL)
        {
        setError(error, errorSize,
                 "Asset data path and manifest output are required");
        return 0;
        }
    assetManifestDestroy(manifest);
    if (!buildPath(manifestPath, sizeof(manifestPath), dataPath,
                   ASSET_MANIFEST_FILE))
        {
        setError(error, errorSize, "Asset manifest path is too long");
        return 0;
        }
    file = fopen(manifestPath, "r");
    if (file == NULL)
        {
        setEntryError(error, errorSize, "Could not open asset manifest",
                      manifestPath);
        return 0;
        }
    sawHeader = 0;
    lineNumber = 0;
    while (fgets(line, sizeof(line), file) != NULL)
        {
        lineNumber++;
        directive = strtok(line, " \t\r\n");
        if (directive == NULL || directive[0] == '#')
            continue;
        if (!sawHeader)
            {
            key = strtok(NULL, " \t\r\n");
            if (strcmp(directive, "battalion-assets") != 0 || key == NULL ||
                strcmp(key, "2") != 0)
                {
                setError(error, errorSize,
                         "Asset manifest must start with battalion-assets 2");
                goto failed;
                }
            sawHeader = 1;
            continue;
            }
        key = strtok(NULL, " \t\r\n");
        path = strtok(NULL, " \t\r\n");
        option1 = strtok(NULL, " \t\r\n");
        option2 = strtok(NULL, " \t\r\n");
        if (strcmp(directive, "model") == 0 && key != NULL && path != NULL &&
            option1 == NULL)
            {
            if (!appendModel(manifest, dataPath, key, path,
                             error, errorSize))
                goto failed;
            }
        else if (strcmp(directive, "texture") == 0 && key != NULL &&
                 path != NULL && option1 != NULL && option2 != NULL &&
                 strtok(NULL, " \t\r\n") == NULL)
            {
            if (!appendTexture(manifest, dataPath, key, path, option1,
                               option2, error, errorSize))
                goto failed;
            }
        else if (strcmp(directive, "animation") == 0 && key != NULL &&
                 path != NULL && option1 != NULL && option2 != NULL &&
                 strtok(NULL, " \t\r\n") == NULL)
            {
            if (!appendAnimation(manifest, key, path, option1, option2,
                                 error, errorSize))
                goto failed;
            }
        else if (strcmp(directive, "file") == 0 && key != NULL &&
                 path != NULL && option1 == NULL)
            {
            if (!appendFile(manifest, dataPath, key, path, error, errorSize))
                goto failed;
            }
        else
            {
            char number[32];
            sprintf(number, "%d", lineNumber);
            setEntryError(error, errorSize,
                          "Malformed asset manifest line", number);
            goto failed;
            }
        }
    fclose(file);
    if (!sawHeader)
        {
        setError(error, errorSize, "Asset manifest is empty");
        assetManifestDestroy(manifest);
        return 0;
        }
    if (!validateAnimations(manifest, error, errorSize))
        {
        assetManifestDestroy(manifest);
        return 0;
        }
    return 1;

failed:
    fclose(file);
    assetManifestDestroy(manifest);
    return 0;
    }
