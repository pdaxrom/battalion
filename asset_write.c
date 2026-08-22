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
/* Deterministic writer for renderer-neutral GLB assets.               */
/***********************************************************************/

#include "asset.h"
#include "asset_write.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GLB_JSON_CHUNK 0x4e4f534aUL
#define GLB_BIN_CHUNK  0x004e4942UL

struct JsonBuilder
    {
    char *text;
    size_t length;
    size_t capacity;
    int failed;
    };

struct MeshWriteInfo
    {
    size_t positionOffset;
    size_t positionLength;
    size_t normalOffset;
    size_t normalLength;
    size_t texcoordOffset;
    size_t texcoordLength;
    size_t colorOffset;
    size_t colorLength;
    float minimum[3];
    float maximum[3];
    };

struct TrackWriteInfo
    {
    size_t timeOffset;
    size_t timeLength;
    size_t valueOffset;
    size_t valueLength;
    };

static void setWriteError(char *error, size_t errorSize, const char *message)
    {
    if (error == NULL || errorSize == 0)
        return;
    strncpy(error, message, errorSize - 1);
    error[errorSize - 1] = '\0';
    }

static int writeU32(FILE *file, unsigned long value)
    {
    unsigned char bytes[4];

    bytes[0] = (unsigned char)(value & 255);
    bytes[1] = (unsigned char)((value >> 8) & 255);
    bytes[2] = (unsigned char)((value >> 16) & 255);
    bytes[3] = (unsigned char)((value >> 24) & 255);
    return fwrite(bytes, 1, sizeof(bytes), file) == sizeof(bytes);
    }

static int writeFloat(FILE *file, float value)
    {
    union FloatBits
        {
        float value;
        unsigned char bytes[4];
        } bits;
    unsigned char output[4];
    unsigned short endianTest;

    bits.value = value;
    endianTest = 1;
    if (*(unsigned char *)&endianTest)
        return fwrite(bits.bytes, 1, 4, file) == 4;
    output[0] = bits.bytes[3];
    output[1] = bits.bytes[2];
    output[2] = bits.bytes[1];
    output[3] = bits.bytes[0];
    return fwrite(output, 1, 4, file) == 4;
    }

static int builderReserve(struct JsonBuilder *builder, size_t extra)
    {
    char *expanded;
    size_t needed;
    size_t capacity;

    if (builder->failed)
        return 0;
    if (extra > ((size_t)-1) - builder->length - 1)
        {
        builder->failed = 1;
        return 0;
        }
    needed = builder->length + extra + 1;
    if (needed <= builder->capacity)
        return 1;
    capacity = builder->capacity ? builder->capacity : 2048;
    while (capacity < needed && capacity <= ((size_t)-1) / 2)
        capacity *= 2;
    if (capacity < needed)
        {
        builder->failed = 1;
        return 0;
        }
    expanded = (char *)realloc(builder->text, capacity);
    if (expanded == NULL)
        {
        builder->failed = 1;
        return 0;
        }
    builder->text = expanded;
    builder->capacity = capacity;
    return 1;
    }

static void builderAppendBytes(struct JsonBuilder *builder,
                               const char *text, size_t length)
    {
    if (!builderReserve(builder, length))
        return;
    memcpy(builder->text + builder->length, text, length);
    builder->length += length;
    builder->text[builder->length] = '\0';
    }

static void builderAppend(struct JsonBuilder *builder, const char *text)
    {
    builderAppendBytes(builder, text, strlen(text));
    }

static void builderAppendFormat(struct JsonBuilder *builder,
                                const char *format, ...)
    {
    char local[1024];
    va_list arguments;
    int length;

    if (builder->failed)
        return;
    /* All callers use short, internal format strings containing numeric
       values.  vsprintf is part of C89; vsnprintf is not available on the
       oldest systems supported by Battalion. */
    va_start(arguments, format);
    length = vsprintf(local, format, arguments);
    va_end(arguments);
    if (length < 0 || (size_t)length >= sizeof(local))
        {
        builder->failed = 1;
        return;
        }
    builderAppendBytes(builder, local, (size_t)length);
    }

static void builderAppendJsonString(struct JsonBuilder *builder,
                                    const char *text)
    {
    const unsigned char *character;
    char escaped[7];

    builderAppend(builder, "\"");
    for (character = (const unsigned char *)text; *character; character++)
        {
        if (*character == '"' || *character == '\\')
            {
            escaped[0] = '\\';
            escaped[1] = (char)*character;
            builderAppendBytes(builder, escaped, 2);
            }
        else if (*character == '\n')
            builderAppend(builder, "\\n");
        else if (*character == '\r')
            builderAppend(builder, "\\r");
        else if (*character == '\t')
            builderAppend(builder, "\\t");
        else if (*character < 32)
            {
            sprintf(escaped, "\\u%04x", *character);
            builderAppend(builder, escaped);
            }
        else
            builderAppendBytes(builder, (const char *)character, 1);
        }
    builderAppend(builder, "\"");
    }

static void builderAppendFloatArray(struct JsonBuilder *builder,
                                    const float *values, int count)
    {
    int component;

    builderAppend(builder, "[");
    for (component = 0; component < count; component++)
        {
        if (component)
            builderAppend(builder, ",");
        builderAppendFormat(builder, "%.9g", values[component]);
        }
    builderAppend(builder, "]");
    }

static int addFloatBlock(size_t *binaryLength, size_t count,
                         size_t components, size_t *offset, size_t *length)
    {
    size_t floats;
    size_t bytes;

    if (components == 0 || count > ((size_t)-1) / components)
        return 0;
    floats = count * components;
    if (floats > ((size_t)-1) / sizeof(float))
        return 0;
    bytes = floats * sizeof(float);
    if (bytes > ((size_t)-1) - *binaryLength)
        return 0;
    *offset = *binaryLength;
    *length = bytes;
    *binaryLength += bytes;
    return *binaryLength <= 0xffffffffUL;
    }

static int animationTrackComponents(const struct AssetAnimationTrack *track)
    {
    if (track->path == ASSET_ANIMATION_ROTATION)
        return 4;
    if (track->path == ASSET_ANIMATION_TRANSLATION ||
        track->path == ASSET_ANIMATION_SCALE)
        return 3;
    return 0;
    }

static const char *animationPathName(int path)
    {
    if (path == ASSET_ANIMATION_TRANSLATION)
        return "translation";
    if (path == ASSET_ANIMATION_ROTATION)
        return "rotation";
    return "scale";
    }

static int validateNodes(const struct AssetModel *model)
    {
    size_t nodeIndex;
    size_t child;
    const struct AssetNode *node;

    if (model->nodeCount > 0 && model->nodes == NULL)
        return 0;
    for (nodeIndex = 0; nodeIndex < model->nodeCount; nodeIndex++)
        {
        node = &model->nodes[nodeIndex];
        if (node->mesh < -1 || node->mesh >= (int)model->meshCount)
            return 0;
        if (node->childCount > 0 && node->children == NULL)
            return 0;
        for (child = 0; child < node->childCount; child++)
            if (node->children[child] < 0 ||
                node->children[child] >= (int)model->nodeCount)
                return 0;
        }
    for (nodeIndex = 0; nodeIndex < model->sceneRootCount; nodeIndex++)
        if (model->sceneRoots == NULL || model->sceneRoots[nodeIndex] < 0 ||
            model->sceneRoots[nodeIndex] >= (int)model->nodeCount)
            return 0;
    return 1;
    }

static void appendBufferViews(struct JsonBuilder *json,
                              const struct AssetModel *model,
                              const struct MeshWriteInfo *meshes,
                              const struct TrackWriteInfo *tracks)
    {
    size_t meshIndex;
    size_t animationIndex;
    size_t trackIndex;
    size_t globalTrack;
    int first;

    builderAppend(json, ",\"bufferViews\":[");
    first = 1;
    for (meshIndex = 0; meshIndex < model->meshCount; meshIndex++)
        {
        if (!first)
            builderAppend(json, ",");
        first = 0;
        builderAppendFormat(json,
            "{\"buffer\":0,\"byteOffset\":%lu,\"byteLength\":%lu,"
            "\"target\":34962},{\"buffer\":0,\"byteOffset\":%lu,"
            "\"byteLength\":%lu,\"target\":34962},"
            "{\"buffer\":0,\"byteOffset\":%lu,\"byteLength\":%lu,"
            "\"target\":34962},{\"buffer\":0,\"byteOffset\":%lu,"
            "\"byteLength\":%lu,\"target\":34962}",
            (unsigned long)meshes[meshIndex].positionOffset,
            (unsigned long)meshes[meshIndex].positionLength,
            (unsigned long)meshes[meshIndex].normalOffset,
            (unsigned long)meshes[meshIndex].normalLength,
            (unsigned long)meshes[meshIndex].texcoordOffset,
            (unsigned long)meshes[meshIndex].texcoordLength,
            (unsigned long)meshes[meshIndex].colorOffset,
            (unsigned long)meshes[meshIndex].colorLength);
        }
    globalTrack = 0;
    for (animationIndex = 0; animationIndex < model->animationCount;
         animationIndex++)
        for (trackIndex = 0;
             trackIndex < model->animations[animationIndex].trackCount;
             trackIndex++, globalTrack++)
            {
            if (!first)
                builderAppend(json, ",");
            first = 0;
            builderAppendFormat(json,
                "{\"buffer\":0,\"byteOffset\":%lu,\"byteLength\":%lu},"
                "{\"buffer\":0,\"byteOffset\":%lu,\"byteLength\":%lu}",
                (unsigned long)tracks[globalTrack].timeOffset,
                (unsigned long)tracks[globalTrack].timeLength,
                (unsigned long)tracks[globalTrack].valueOffset,
                (unsigned long)tracks[globalTrack].valueLength);
            }
    builderAppend(json, "]");
    }

static void appendAccessors(struct JsonBuilder *json,
                            const struct AssetModel *model,
                            const struct MeshWriteInfo *meshes)
    {
    size_t meshIndex;
    size_t animationIndex;
    size_t trackIndex;
    size_t view;
    const struct AssetAnimationTrack *track;

    builderAppend(json, ",\"accessors\":[");
    view = 0;
    for (meshIndex = 0; meshIndex < model->meshCount; meshIndex++)
        {
        if (meshIndex)
            builderAppend(json, ",");
        builderAppendFormat(json,
            "{\"bufferView\":%lu,\"componentType\":5126,\"count\":%lu,"
            "\"type\":\"VEC3\",\"min\":[%.9g,%.9g,%.9g],"
            "\"max\":[%.9g,%.9g,%.9g]},"
            "{\"bufferView\":%lu,\"componentType\":5126,\"count\":%lu,"
            "\"type\":\"VEC3\"},"
            "{\"bufferView\":%lu,\"componentType\":5126,\"count\":%lu,"
            "\"type\":\"VEC2\"},"
            "{\"bufferView\":%lu,\"componentType\":5126,\"count\":%lu,"
            "\"type\":\"VEC4\"}",
            (unsigned long)view, (unsigned long)model->meshes[meshIndex].vertexCount,
            meshes[meshIndex].minimum[0], meshes[meshIndex].minimum[1],
            meshes[meshIndex].minimum[2], meshes[meshIndex].maximum[0],
            meshes[meshIndex].maximum[1], meshes[meshIndex].maximum[2],
            (unsigned long)(view + 1),
            (unsigned long)model->meshes[meshIndex].vertexCount,
            (unsigned long)(view + 2),
            (unsigned long)model->meshes[meshIndex].vertexCount,
            (unsigned long)(view + 3),
            (unsigned long)model->meshes[meshIndex].vertexCount);
        view += 4;
        }
    for (animationIndex = 0; animationIndex < model->animationCount;
         animationIndex++)
        for (trackIndex = 0;
             trackIndex < model->animations[animationIndex].trackCount;
             trackIndex++)
            {
            track = &model->animations[animationIndex].tracks[trackIndex];
            builderAppendFormat(json,
                ",{\"bufferView\":%lu,\"componentType\":5126,"
                "\"count\":%lu,\"type\":\"SCALAR\",\"min\":[%.9g],"
                "\"max\":[%.9g]},{\"bufferView\":%lu,"
                "\"componentType\":5126,\"count\":%lu,\"type\":\"VEC%d\"}",
                (unsigned long)view, (unsigned long)track->keyframeCount,
                track->times[0], track->times[track->keyframeCount - 1],
                (unsigned long)(view + 1),
                (unsigned long)track->keyframeCount, track->components);
            view += 2;
            }
    builderAppend(json, "]");
    }

static void appendMeshes(struct JsonBuilder *json,
                         const struct AssetModel *model)
    {
    size_t meshIndex;
    int mode;

    builderAppend(json, ",\"meshes\":[");
    for (meshIndex = 0; meshIndex < model->meshCount; meshIndex++)
        {
        if (meshIndex)
            builderAppend(json, ",");
        if (model->meshes[meshIndex].primitive == ASSET_PRIMITIVE_LINES)
            mode = 1;
        else if (model->meshes[meshIndex].primitive == ASSET_PRIMITIVE_POINTS)
            mode = 0;
        else
            mode = 4;
        builderAppendFormat(json,
            "{\"name\":\"mesh%lu\",\"primitives\":[{\"attributes\":{" 
            "\"POSITION\":%lu,\"NORMAL\":%lu,\"TEXCOORD_0\":%lu,"
            "\"COLOR_0\":%lu},\"mode\":%d,\"material\":%lu,"
            "\"extras\":{\"battalionLineWidth\":%.9g,"
            "\"battalionPointSize\":%.9g,"
            "\"battalionUseCurrentColor\":%s}}]}",
            (unsigned long)meshIndex, (unsigned long)(meshIndex * 4),
            (unsigned long)(meshIndex * 4 + 1),
            (unsigned long)(meshIndex * 4 + 2),
            (unsigned long)(meshIndex * 4 + 3), mode,
            (unsigned long)meshIndex,
            model->meshes[meshIndex].lineWidth,
            model->meshes[meshIndex].pointSize,
            model->meshes[meshIndex].useCurrentColor ? "true" : "false");
        }
    builderAppend(json, "]");
    }

static void appendMaterials(struct JsonBuilder *json,
                            const struct AssetModel *model)
    {
    size_t meshIndex;

    builderAppend(json, ",\"materials\":[");
    for (meshIndex = 0; meshIndex < model->meshCount; meshIndex++)
        {
        if (meshIndex)
            builderAppend(json, ",");
        builderAppendFormat(json,
            "{\"name\":\"material%lu\",\"pbrMetallicRoughness\":{"
            "\"baseColorFactor\":[1,1,1,1]},\"alphaMode\":\"%s\"}",
            (unsigned long)meshIndex,
            model->meshes[meshIndex].blend ? "BLEND" : "OPAQUE");
        }
    builderAppend(json, "]");
    }

static void appendNodesAndScene(struct JsonBuilder *json,
                                const struct AssetModel *model)
    {
    const struct AssetNode *node;
    size_t nodeCount;
    size_t nodeIndex;
    size_t child;
    size_t root;
    int property;

    nodeCount = model->nodeCount ? model->nodeCount : model->meshCount;
    builderAppend(json, ",\"nodes\":[");
    for (nodeIndex = 0; nodeIndex < nodeCount; nodeIndex++)
        {
        if (nodeIndex)
            builderAppend(json, ",");
        if (model->nodeCount == 0)
            {
            builderAppendFormat(json, "{\"mesh\":%lu}",
                                (unsigned long)nodeIndex);
            continue;
            }
        node = &model->nodes[nodeIndex];
        builderAppend(json, "{");
        property = 0;
        if (node->name != NULL)
            {
            builderAppend(json, "\"name\":");
            builderAppendJsonString(json, node->name);
            property = 1;
            }
        if (node->mesh >= 0)
            {
            if (property)
                builderAppend(json, ",");
            builderAppendFormat(json, "\"mesh\":%d", node->mesh);
            property = 1;
            }
        if (node->childCount > 0)
            {
            if (property)
                builderAppend(json, ",");
            builderAppend(json, "\"children\":[");
            for (child = 0; child < node->childCount; child++)
                {
                if (child)
                    builderAppend(json, ",");
                builderAppendFormat(json, "%d", node->children[child]);
                }
            builderAppend(json, "]");
            property = 1;
            }
        if (property)
            builderAppend(json, ",");
        if (node->hasMatrix)
            {
            builderAppend(json, "\"matrix\":");
            builderAppendFloatArray(json, node->matrix, 16);
            }
        else
            {
            builderAppend(json, "\"translation\":");
            builderAppendFloatArray(json, node->translation, 3);
            builderAppend(json, ",\"rotation\":");
            builderAppendFloatArray(json, node->rotation, 4);
            builderAppend(json, ",\"scale\":");
            builderAppendFloatArray(json, node->scale, 3);
            }
        builderAppend(json, "}");
        }
    builderAppend(json, "],\"scenes\":[{\"nodes\":[");
    if (model->nodeCount > 0 && model->sceneRootCount > 0)
        for (root = 0; root < model->sceneRootCount; root++)
            {
            if (root)
                builderAppend(json, ",");
            builderAppendFormat(json, "%d", model->sceneRoots[root]);
            }
    else
        for (root = 0; root < nodeCount; root++)
            {
            if (root)
                builderAppend(json, ",");
            builderAppendFormat(json, "%lu", (unsigned long)root);
            }
    builderAppend(json, "]}],\"scene\":0");
    }

static void appendAnimations(struct JsonBuilder *json,
                             const struct AssetModel *model)
    {
    const struct AssetAnimation *animation;
    const struct AssetAnimationTrack *track;
    size_t animationIndex;
    size_t trackIndex;
    size_t globalTrack;
    size_t accessor;

    if (model->animationCount == 0)
        return;
    builderAppend(json, ",\"animations\":[");
    globalTrack = 0;
    for (animationIndex = 0; animationIndex < model->animationCount;
         animationIndex++)
        {
        animation = &model->animations[animationIndex];
        if (animationIndex)
            builderAppend(json, ",");
        builderAppend(json, "{");
        if (animation->name != NULL)
            {
            builderAppend(json, "\"name\":");
            builderAppendJsonString(json, animation->name);
            builderAppend(json, ",");
            }
        builderAppend(json, "\"samplers\":[");
        for (trackIndex = 0; trackIndex < animation->trackCount;
             trackIndex++)
            {
            track = &animation->tracks[trackIndex];
            accessor = model->meshCount * 4 + globalTrack * 2;
            if (trackIndex)
                builderAppend(json, ",");
            builderAppendFormat(json,
                "{\"input\":%lu,\"output\":%lu,\"interpolation\":\"%s\"}",
                (unsigned long)accessor, (unsigned long)(accessor + 1),
                track->interpolation == ASSET_INTERPOLATION_STEP ?
                "STEP" : "LINEAR");
            globalTrack++;
            }
        builderAppend(json, "],\"channels\":[");
        for (trackIndex = 0; trackIndex < animation->trackCount;
             trackIndex++)
            {
            track = &animation->tracks[trackIndex];
            if (trackIndex)
                builderAppend(json, ",");
            builderAppendFormat(json,
                "{\"sampler\":%lu,\"target\":{\"node\":%d,\"path\":\"%s\"}}",
                (unsigned long)trackIndex, track->node,
                animationPathName(track->path));
            }
        builderAppend(json, "]}");
        }
    builderAppend(json, "]");
    }

int assetModelWriteGLB(const char *path, const struct AssetModel *model,
                       char *error, size_t errorSize)
    {
    struct MeshWriteInfo *meshInfo;
    struct TrackWriteInfo *trackInfo;
    struct JsonBuilder json;
    const struct AssetMesh *mesh;
    const struct AssetVertex *vertex;
    const struct AssetAnimation *animation;
    const struct AssetAnimationTrack *track;
    FILE *file;
    size_t meshIndex;
    size_t animationIndex;
    size_t trackIndex;
    size_t globalTrack;
    size_t totalTracks;
    size_t binaryLength;
    size_t paddedJsonLength;
    size_t totalLength;
    size_t vertexIndex;
    size_t keyframe;
    int component;
    int success;
    int result;

    meshInfo = NULL;
    trackInfo = NULL;
    file = NULL;
    success = 0;
    result = 0;
    memset(&json, 0, sizeof(json));
    if (path == NULL || model == NULL || model->meshCount == 0 ||
        model->meshes == NULL || !validateNodes(model))
        {
        setWriteError(error, errorSize, "GLB writer received an invalid model");
        return 0;
        }
    if (model->animationCount > 0 && model->animations == NULL)
        {
        setWriteError(error, errorSize, "GLB animation data is missing");
        return 0;
        }

    totalTracks = 0;
    for (animationIndex = 0; animationIndex < model->animationCount;
         animationIndex++)
        {
        animation = &model->animations[animationIndex];
        if (animation->trackCount > 0 && animation->tracks == NULL)
            {
            setWriteError(error, errorSize, "GLB animation has no track data");
            return 0;
            }
        if (animation->trackCount > ((size_t)-1) - totalTracks)
            {
            setWriteError(error, errorSize, "GLB has too many animation tracks");
            return 0;
            }
        totalTracks += animation->trackCount;
        }
    if (totalTracks > 0 && model->nodeCount == 0)
        {
        setWriteError(error, errorSize, "Animated GLB requires scene nodes");
        return 0;
        }

    meshInfo = (struct MeshWriteInfo *)calloc(model->meshCount,
                                               sizeof(*meshInfo));
    if (totalTracks > 0)
        trackInfo = (struct TrackWriteInfo *)calloc(totalTracks,
                                                     sizeof(*trackInfo));
    if (meshInfo == NULL || (totalTracks > 0 && trackInfo == NULL))
        {
        setWriteError(error, errorSize, "Out of memory preparing GLB metadata");
        goto cleanup;
        }

    binaryLength = 0;
    for (meshIndex = 0; meshIndex < model->meshCount; meshIndex++)
        {
        mesh = &model->meshes[meshIndex];
        if (mesh->vertices == NULL || mesh->vertexCount == 0 ||
            (mesh->primitive == ASSET_PRIMITIVE_TRIANGLES &&
             mesh->vertexCount % 3 != 0) ||
            (mesh->primitive == ASSET_PRIMITIVE_LINES &&
             mesh->vertexCount % 2 != 0) ||
            (mesh->primitive != ASSET_PRIMITIVE_TRIANGLES &&
             mesh->primitive != ASSET_PRIMITIVE_LINES &&
             mesh->primitive != ASSET_PRIMITIVE_POINTS) ||
            !addFloatBlock(&binaryLength, mesh->vertexCount, 3,
                           &meshInfo[meshIndex].positionOffset,
                           &meshInfo[meshIndex].positionLength) ||
            !addFloatBlock(&binaryLength, mesh->vertexCount, 3,
                           &meshInfo[meshIndex].normalOffset,
                           &meshInfo[meshIndex].normalLength) ||
            !addFloatBlock(&binaryLength, mesh->vertexCount, 2,
                           &meshInfo[meshIndex].texcoordOffset,
                           &meshInfo[meshIndex].texcoordLength) ||
            !addFloatBlock(&binaryLength, mesh->vertexCount, 4,
                           &meshInfo[meshIndex].colorOffset,
                           &meshInfo[meshIndex].colorLength))
            {
            setWriteError(error, errorSize,
                          "GLB writer received an invalid primitive mesh");
            goto cleanup;
            }
        memcpy(meshInfo[meshIndex].minimum, mesh->vertices[0].position,
               sizeof(meshInfo[meshIndex].minimum));
        memcpy(meshInfo[meshIndex].maximum, mesh->vertices[0].position,
               sizeof(meshInfo[meshIndex].maximum));
        for (vertexIndex = 1; vertexIndex < mesh->vertexCount; vertexIndex++)
            for (component = 0; component < 3; component++)
                {
                if (mesh->vertices[vertexIndex].position[component] <
                    meshInfo[meshIndex].minimum[component])
                    meshInfo[meshIndex].minimum[component] =
                        mesh->vertices[vertexIndex].position[component];
                if (mesh->vertices[vertexIndex].position[component] >
                    meshInfo[meshIndex].maximum[component])
                    meshInfo[meshIndex].maximum[component] =
                        mesh->vertices[vertexIndex].position[component];
                }
        }

    globalTrack = 0;
    for (animationIndex = 0; animationIndex < model->animationCount;
         animationIndex++)
        for (trackIndex = 0;
             trackIndex < model->animations[animationIndex].trackCount;
             trackIndex++, globalTrack++)
            {
            track = &model->animations[animationIndex].tracks[trackIndex];
            component = animationTrackComponents(track);
            if (component == 0 || track->components != component ||
                track->node < 0 || track->node >= (int)model->nodeCount ||
                model->nodes[track->node].hasMatrix ||
                track->keyframeCount == 0 || track->times == NULL ||
                track->values == NULL)
                {
                setWriteError(error, errorSize, "Invalid GLB animation track");
                goto cleanup;
                }
            for (keyframe = 0; keyframe < track->keyframeCount; keyframe++)
                if (track->times[keyframe] < 0.0f ||
                    (keyframe > 0 && track->times[keyframe] <=
                                     track->times[keyframe - 1]))
                    {
                    setWriteError(error, errorSize,
                                  "GLB animation times must be increasing");
                    goto cleanup;
                    }
            if (!addFloatBlock(&binaryLength, track->keyframeCount, 1,
                               &trackInfo[globalTrack].timeOffset,
                               &trackInfo[globalTrack].timeLength) ||
                !addFloatBlock(&binaryLength, track->keyframeCount,
                               (size_t)component,
                               &trackInfo[globalTrack].valueOffset,
                               &trackInfo[globalTrack].valueLength))
                {
                setWriteError(error, errorSize, "Animated GLB is too large");
                goto cleanup;
                }
            }

    builderAppend(&json,
        "{\"asset\":{\"generator\":\"Battalion asset tools\","
        "\"version\":\"2.0\"},\"buffers\":[{");
    builderAppendFormat(&json, "\"byteLength\":%lu}]",
                        (unsigned long)binaryLength);
    appendBufferViews(&json, model, meshInfo, trackInfo);
    appendAccessors(&json, model, meshInfo);
    appendMaterials(&json, model);
    appendMeshes(&json, model);
    appendNodesAndScene(&json, model);
    appendAnimations(&json, model);
    builderAppend(&json, "}");
    if (json.failed || json.length == 0)
        {
        setWriteError(error, errorSize, "Could not build GLB metadata");
        goto cleanup;
        }

    paddedJsonLength = (json.length + 3) & ~(size_t)3;
    if (paddedJsonLength < json.length ||
        binaryLength > ((size_t)-1) - paddedJsonLength - 28)
        {
        setWriteError(error, errorSize, "GLB exceeds the format limit");
        goto cleanup;
        }
    totalLength = 28 + paddedJsonLength + binaryLength;
    if (totalLength > 0xffffffffUL)
        {
        setWriteError(error, errorSize, "GLB exceeds the format limit");
        goto cleanup;
        }

    file = fopen(path, "wb");
    if (file == NULL)
        {
        setWriteError(error, errorSize, "Could not create GLB file");
        goto cleanup;
        }
    success = fwrite("glTF", 1, 4, file) == 4 &&
              writeU32(file, 2) && writeU32(file, (unsigned long)totalLength) &&
              writeU32(file, (unsigned long)paddedJsonLength) &&
              writeU32(file, GLB_JSON_CHUNK) &&
              fwrite(json.text, 1, json.length, file) == json.length;
    for (vertexIndex = json.length;
         success && vertexIndex < paddedJsonLength; vertexIndex++)
        success = fputc(' ', file) != EOF;
    success = success && writeU32(file, (unsigned long)binaryLength) &&
              writeU32(file, GLB_BIN_CHUNK);
    for (meshIndex = 0; success && meshIndex < model->meshCount; meshIndex++)
        {
        mesh = &model->meshes[meshIndex];
        for (vertexIndex = 0; success && vertexIndex < mesh->vertexCount;
             vertexIndex++)
            {
            vertex = &mesh->vertices[vertexIndex];
            for (component = 0; success && component < 3; component++)
                success = writeFloat(file, vertex->position[component]);
            }
        for (vertexIndex = 0; success && vertexIndex < mesh->vertexCount;
             vertexIndex++)
            {
            vertex = &mesh->vertices[vertexIndex];
            for (component = 0; success && component < 3; component++)
                success = writeFloat(file, vertex->normal[component]);
            }
        for (vertexIndex = 0; success && vertexIndex < mesh->vertexCount;
             vertexIndex++)
            {
            vertex = &mesh->vertices[vertexIndex];
            for (component = 0; success && component < 2; component++)
                success = writeFloat(file, vertex->texcoord[component]);
            }
        for (vertexIndex = 0; success && vertexIndex < mesh->vertexCount;
             vertexIndex++)
            {
            vertex = &mesh->vertices[vertexIndex];
            for (component = 0; success && component < 4; component++)
                success = writeFloat(file, vertex->color[component]);
            }
        }
    for (animationIndex = 0; success &&
         animationIndex < model->animationCount; animationIndex++)
        {
        animation = &model->animations[animationIndex];
        for (trackIndex = 0; success && trackIndex < animation->trackCount;
             trackIndex++)
            {
            track = &animation->tracks[trackIndex];
            for (keyframe = 0; success && keyframe < track->keyframeCount;
                 keyframe++)
                success = writeFloat(file, track->times[keyframe]);
            for (keyframe = 0; success &&
                 keyframe < track->keyframeCount * (size_t)track->components;
                 keyframe++)
                success = writeFloat(file, track->values[keyframe]);
            }
        }
    if (fclose(file) != 0)
        success = 0;
    file = NULL;
    if (!success)
        {
        remove(path);
        setWriteError(error, errorSize, "Could not write complete GLB file");
        }
    else
        result = 1;

cleanup:
    if (file != NULL)
        {
        fclose(file);
        remove(path);
        }
    free(json.text);
    free(meshInfo);
    free(trackInfo);
    return result;
    }

int assetModelWriteStaticGLB(const char *path, const struct AssetModel *model,
                             char *error, size_t errorSize)
    {
    return assetModelWriteGLB(path, model, error, errorSize);
    }
