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
/* Minimal glTF 2.0 binary loader for renderer-neutral static meshes.  */
/***********************************************************************/

#include "asset.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GLB_JSON_CHUNK 0x4e4f534aUL
#define GLB_BIN_CHUNK  0x004e4942UL
#define GLTF_TRIANGLES 4
#define GLTF_LINES 1
#define GLTF_POINTS 0

enum JsonTokenType
    {
    JSON_UNDEFINED,
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_PRIMITIVE
    };

struct JsonToken
    {
    enum JsonTokenType type;
    int start;
    int end;
    int size;
    int parent;
    };

struct JsonParser
    {
    int position;
    int nextToken;
    int parent;
    };

struct GltfBufferView
    {
    size_t offset;
    size_t length;
    size_t stride;
    int buffer;
    };

struct GltfAccessor
    {
    size_t offset;
    size_t count;
    int bufferView;
    int componentType;
    int components;
    int normalized;
    };

struct GltfMaterial
    {
    float color[4];
    int blend;
    };

static void setAssetError(char *error, size_t errorSize, const char *message)
    {
    if (errorSize == 0)
        return;
    strncpy(error, message, errorSize - 1);
    error[errorSize - 1] = '\0';
    }

static unsigned long readU32(const unsigned char *bytes)
    {
    return (unsigned long)bytes[0] |
           ((unsigned long)bytes[1] << 8) |
           ((unsigned long)bytes[2] << 16) |
           ((unsigned long)bytes[3] << 24);
    }

static float readF32(const unsigned char *bytes)
    {
    unsigned char nativeBytes[4];
    unsigned short endianTest;
    float value;

    if (sizeof(value) != 4)
        return 0.0f;
    endianTest = 1;
    if (*(unsigned char *)&endianTest)
        memcpy(nativeBytes, bytes, 4);
    else
        {
        nativeBytes[0] = bytes[3];
        nativeBytes[1] = bytes[2];
        nativeBytes[2] = bytes[1];
        nativeBytes[3] = bytes[0];
        }
    memcpy(&value, nativeBytes, sizeof(value));
    return value;
    }

static struct JsonToken *allocateToken(struct JsonParser *parser,
                                       struct JsonToken *tokens,
                                       int tokenCapacity)
    {
    struct JsonToken *token;

    if (parser->nextToken >= tokenCapacity)
        return NULL;
    token = &tokens[parser->nextToken++];
    token->type = JSON_UNDEFINED;
    token->start = -1;
    token->end = -1;
    token->size = 0;
    token->parent = -1;
    return token;
    }

static int parseJsonString(struct JsonParser *parser, const char *json,
                           size_t length, struct JsonToken *tokens,
                           int tokenCapacity)
    {
    int start;
    struct JsonToken *token;

    start = parser->position + 1;
    parser->position++;
    while ((size_t)parser->position < length)
        {
        if (json[parser->position] == '"')
            {
            token = allocateToken(parser, tokens, tokenCapacity);
            if (token == NULL)
                return 0;
            token->type = JSON_STRING;
            token->start = start;
            token->end = parser->position;
            token->parent = parser->parent;
            if (parser->parent >= 0)
                tokens[parser->parent].size++;
            return 1;
            }
        if (json[parser->position] == '\\')
            {
            parser->position++;
            if ((size_t)parser->position >= length)
                return 0;
            }
        parser->position++;
        }
    return 0;
    }

static int parseJsonPrimitive(struct JsonParser *parser, const char *json,
                              size_t length, struct JsonToken *tokens,
                              int tokenCapacity)
    {
    int start;
    char value;
    struct JsonToken *token;

    start = parser->position;
    while ((size_t)parser->position < length)
        {
        value = json[parser->position];
        if (value == ',' || value == ']' || value == '}' ||
            value == ' ' || value == '\t' || value == '\r' || value == '\n')
            break;
        if ((unsigned char)value < 32)
            return 0;
        parser->position++;
        }

    if (start == parser->position)
        return 0;
    token = allocateToken(parser, tokens, tokenCapacity);
    if (token == NULL)
        return 0;
    token->type = JSON_PRIMITIVE;
    token->start = start;
    token->end = parser->position;
    token->parent = parser->parent;
    if (parser->parent >= 0)
        tokens[parser->parent].size++;
    parser->position--;
    return 1;
    }

static int parseJson(const char *json, size_t length,
                     struct JsonToken *tokens, int tokenCapacity)
    {
    struct JsonParser parser;
    struct JsonToken *token;
    enum JsonTokenType type;
    int index;
    char value;

    parser.position = 0;
    parser.nextToken = 0;
    parser.parent = -1;

    while ((size_t)parser.position < length)
        {
        value = json[parser.position];
        if (value == '{' || value == '[')
            {
            token = allocateToken(&parser, tokens, tokenCapacity);
            if (token == NULL)
                return -1;
            token->type = value == '{' ? JSON_OBJECT : JSON_ARRAY;
            token->start = parser.position;
            token->parent = parser.parent;
            if (parser.parent >= 0)
                tokens[parser.parent].size++;
            parser.parent = parser.nextToken - 1;
            }
        else if (value == '}' || value == ']')
            {
            type = value == '}' ? JSON_OBJECT : JSON_ARRAY;
            for (index = parser.nextToken - 1; index >= 0; index--)
                if (tokens[index].start >= 0 && tokens[index].end < 0)
                    {
                    if (tokens[index].type != type)
                        return -2;
                    tokens[index].end = parser.position + 1;
                    parser.parent = tokens[index].parent;
                    break;
                    }
            if (index < 0)
                return -2;
            }
        else if (value == '"')
            {
            if (!parseJsonString(&parser, json, length, tokens, tokenCapacity))
                return -2;
            }
        else if (value == ' ' || value == '\t' || value == '\r' ||
                 value == '\n' || value == ':' || value == ',')
            {
            }
        else if (!parseJsonPrimitive(&parser, json, length, tokens,
                                     tokenCapacity))
            return -2;
        parser.position++;
        }

    for (index = parser.nextToken - 1; index >= 0; index--)
        if (tokens[index].start >= 0 && tokens[index].end < 0)
            return -2;
    return parser.nextToken;
    }

static int tokenEquals(const char *json, const struct JsonToken *token,
                       const char *text)
    {
    size_t length;

    if (token->type != JSON_STRING)
        return 0;
    length = strlen(text);
    return (size_t)(token->end - token->start) == length &&
           strncmp(json + token->start, text, length) == 0;
    }

static int tokenIsTrue(const char *json, const struct JsonToken *token)
    {
    return token->type == JSON_PRIMITIVE &&
           token->end - token->start == 4 &&
           strncmp(json + token->start, "true", 4) == 0;
    }

static int objectValue(const char *json, const struct JsonToken *tokens,
                       int tokenCount, int object, const char *key)
    {
    int index;
    int keyToken;
    int expectingKey;

    if (object < 0 || object >= tokenCount ||
        tokens[object].type != JSON_OBJECT)
        return -1;

    expectingKey = 1;
    keyToken = -1;
    for (index = object + 1;
         index < tokenCount && tokens[index].start < tokens[object].end;
         index++)
        {
        if (tokens[index].parent != object)
            continue;
        if (expectingKey)
            {
            keyToken = index;
            expectingKey = 0;
            }
        else
            {
            if (tokenEquals(json, &tokens[keyToken], key))
                return index;
            expectingKey = 1;
            }
        }
    return -1;
    }

static int arrayItem(const struct JsonToken *tokens, int tokenCount,
                     int array, int item)
    {
    int index;
    int current;

    if (array < 0 || array >= tokenCount || tokens[array].type != JSON_ARRAY)
        return -1;
    current = 0;
    for (index = array + 1;
         index < tokenCount && tokens[index].start < tokens[array].end;
         index++)
        if (tokens[index].parent == array)
            {
            if (current == item)
                return index;
            current++;
            }
    return -1;
    }

static int arrayLength(const struct JsonToken *tokens, int tokenCount, int array)
    {
    int index;
    int count;

    if (array < 0 || array >= tokenCount || tokens[array].type != JSON_ARRAY)
        return 0;
    count = 0;
    for (index = array + 1;
         index < tokenCount && tokens[index].start < tokens[array].end;
         index++)
        if (tokens[index].parent == array)
            count++;
    return count;
    }

static double tokenNumber(const char *json, const struct JsonToken *token)
    {
    char number[64];
    int length;

    length = token->end - token->start;
    if (length <= 0)
        return 0.0;
    if (length >= (int)sizeof(number))
        length = sizeof(number) - 1;
    memcpy(number, json + token->start, length);
    number[length] = '\0';
    return strtod(number, NULL);
    }

static int tokenInteger(const char *json, const struct JsonToken *tokens,
                        int token, int defaultValue)
    {
    if (token < 0)
        return defaultValue;
    return (int)tokenNumber(json, &tokens[token]);
    }

static int tokenFloatArray(const char *json, const struct JsonToken *tokens,
                           int tokenCount, int array, float *values,
                           int valueCount)
    {
    int component;
    int valueToken;

    if (arrayLength(tokens, tokenCount, array) != valueCount)
        return 0;
    for (component = 0; component < valueCount; component++)
        {
        valueToken = arrayItem(tokens, tokenCount, array, component);
        if (valueToken < 0)
            return 0;
        values[component] = (float)tokenNumber(json, &tokens[valueToken]);
        }
    return 1;
    }

static char *duplicateTokenString(const char *json,
                                  const struct JsonToken *token)
    {
    char *text;
    size_t length;

    if (token == NULL || token->type != JSON_STRING)
        return NULL;
    length = (size_t)(token->end - token->start);
    text = (char *)malloc(length + 1);
    if (text == NULL)
        return NULL;
    memcpy(text, json + token->start, length);
    text[length] = '\0';
    return text;
    }

static size_t componentSize(int componentType)
    {
    switch (componentType)
        {
        case 5120:
        case 5121:
            return 1;
        case 5122:
        case 5123:
            return 2;
        case 5125:
        case 5126:
            return 4;
        default:
            return 0;
        }
    }

static int componentCount(const char *json, const struct JsonToken *token)
    {
    if (tokenEquals(json, token, "SCALAR")) return 1;
    if (tokenEquals(json, token, "VEC2")) return 2;
    if (tokenEquals(json, token, "VEC3")) return 3;
    if (tokenEquals(json, token, "VEC4")) return 4;
    if (tokenEquals(json, token, "MAT4")) return 16;
    return 0;
    }

static int readBufferViews(const char *json, const struct JsonToken *tokens,
                           int tokenCount, struct GltfBufferView **result,
                           int *resultCount)
    {
    int array;
    int count;
    int index;
    int object;
    struct GltfBufferView *views;

    array = objectValue(json, tokens, tokenCount, 0, "bufferViews");
    count = arrayLength(tokens, tokenCount, array);
    if (count <= 0)
        return 0;
    views = (struct GltfBufferView *)calloc(count, sizeof(*views));
    if (views == NULL)
        return 0;

    for (index = 0; index < count; index++)
        {
        object = arrayItem(tokens, tokenCount, array, index);
        views[index].buffer = tokenInteger(json, tokens,
            objectValue(json, tokens, tokenCount, object, "buffer"), 0);
        views[index].offset = (size_t)tokenInteger(json, tokens,
            objectValue(json, tokens, tokenCount, object, "byteOffset"), 0);
        views[index].length = (size_t)tokenInteger(json, tokens,
            objectValue(json, tokens, tokenCount, object, "byteLength"), 0);
        views[index].stride = (size_t)tokenInteger(json, tokens,
            objectValue(json, tokens, tokenCount, object, "byteStride"), 0);
        }
    *result = views;
    *resultCount = count;
    return 1;
    }

static int readAccessors(const char *json, const struct JsonToken *tokens,
                         int tokenCount, struct GltfAccessor **result,
                         int *resultCount)
    {
    int array;
    int count;
    int index;
    int object;
    int typeToken;
    int normalizedToken;
    struct GltfAccessor *accessors;

    array = objectValue(json, tokens, tokenCount, 0, "accessors");
    count = arrayLength(tokens, tokenCount, array);
    if (count <= 0)
        return 0;
    accessors = (struct GltfAccessor *)calloc(count, sizeof(*accessors));
    if (accessors == NULL)
        return 0;

    for (index = 0; index < count; index++)
        {
        object = arrayItem(tokens, tokenCount, array, index);
        accessors[index].bufferView = tokenInteger(json, tokens,
            objectValue(json, tokens, tokenCount, object, "bufferView"), -1);
        accessors[index].offset = (size_t)tokenInteger(json, tokens,
            objectValue(json, tokens, tokenCount, object, "byteOffset"), 0);
        accessors[index].count = (size_t)tokenInteger(json, tokens,
            objectValue(json, tokens, tokenCount, object, "count"), 0);
        accessors[index].componentType = tokenInteger(json, tokens,
            objectValue(json, tokens, tokenCount, object, "componentType"), 0);
        typeToken = objectValue(json, tokens, tokenCount, object, "type");
        accessors[index].components = typeToken >= 0 ?
            componentCount(json, &tokens[typeToken]) : 0;
        normalizedToken = objectValue(json, tokens, tokenCount, object,
                                      "normalized");
        accessors[index].normalized = normalizedToken >= 0 &&
            strncmp(json + tokens[normalizedToken].start, "true", 4) == 0;
        }
    *result = accessors;
    *resultCount = count;
    return 1;
    }

static struct GltfMaterial *readMaterials(const char *json,
                                          const struct JsonToken *tokens,
                                          int tokenCount, int *resultCount)
    {
    int array;
    int count;
    int index;
    int object;
    int pbr;
    int colorArray;
    int component;
    int colorToken;
    int alphaMode;
    struct GltfMaterial *materials;

    array = objectValue(json, tokens, tokenCount, 0, "materials");
    count = arrayLength(tokens, tokenCount, array);
    if (count <= 0)
        {
        *resultCount = 0;
        return NULL;
        }
    materials = (struct GltfMaterial *)calloc(count, sizeof(*materials));
    if (materials == NULL)
        {
        *resultCount = -1;
        return NULL;
        }

    for (index = 0; index < count; index++)
        {
        materials[index].color[0] = 1.0f;
        materials[index].color[1] = 1.0f;
        materials[index].color[2] = 1.0f;
        materials[index].color[3] = 1.0f;
        object = arrayItem(tokens, tokenCount, array, index);
        pbr = objectValue(json, tokens, tokenCount, object,
                          "pbrMetallicRoughness");
        colorArray = objectValue(json, tokens, tokenCount, pbr,
                                 "baseColorFactor");
        for (component = 0; component < 4; component++)
            {
            colorToken = arrayItem(tokens, tokenCount, colorArray, component);
            if (colorToken >= 0)
                materials[index].color[component] =
                    (float)tokenNumber(json, &tokens[colorToken]);
            }
        alphaMode = objectValue(json, tokens, tokenCount, object,
                                "alphaMode");
        materials[index].blend = alphaMode >= 0 &&
            tokenEquals(json, &tokens[alphaMode], "BLEND");
        }
    *resultCount = count;
    return materials;
    }

static double readComponent(const unsigned char *bytes, int componentType,
                            int normalized)
    {
    signed char signedByte;
    short signedShort;
    unsigned short unsignedShort;
    unsigned long unsignedInt;

    switch (componentType)
        {
        case 5120:
            signedByte = (signed char)bytes[0];
            if (normalized)
                return signedByte < -127 ? -1.0 : signedByte / 127.0;
            return signedByte;
        case 5121:
            return normalized ? bytes[0] / 255.0 : bytes[0];
        case 5122:
            signedShort = (short)((unsigned short)bytes[0] |
                                  ((unsigned short)bytes[1] << 8));
            if (normalized)
                return signedShort < -32767 ? -1.0 : signedShort / 32767.0;
            return signedShort;
        case 5123:
            unsignedShort = (unsigned short)bytes[0] |
                            ((unsigned short)bytes[1] << 8);
            return normalized ? unsignedShort / 65535.0 : unsignedShort;
        case 5125:
            unsignedInt = readU32(bytes);
            return normalized ? unsignedInt / 4294967295.0 : unsignedInt;
        case 5126:
            return readF32(bytes);
        default:
            return 0.0;
        }
    }

static const unsigned char *accessorElement(
                        const unsigned char *binary, size_t binaryLength,
                        const struct GltfBufferView *views, int viewCount,
                        const struct GltfAccessor *accessor, size_t element,
                        size_t *componentByteSize)
    {
    const struct GltfBufferView *view;
    size_t size;
    size_t elementSize;
    size_t stride;
    size_t available;
    size_t relativeOffset;

    if (accessor->bufferView < 0 || accessor->bufferView >= viewCount ||
        element >= accessor->count || accessor->components <= 0)
        return NULL;
    view = &views[accessor->bufferView];
    if (view->buffer != 0 || view->offset > binaryLength ||
        view->length > binaryLength - view->offset)
        return NULL;
    size = componentSize(accessor->componentType);
    if (size == 0 || (size_t)accessor->components > ((size_t)-1) / size)
        return NULL;
    elementSize = size * (size_t)accessor->components;
    stride = view->stride ? view->stride : elementSize;
    if (stride < elementSize || accessor->offset > view->length)
        return NULL;
    available = view->length - accessor->offset;
    if (elementSize > available || element > (available - elementSize) / stride)
        return NULL;
    relativeOffset = accessor->offset + element * stride;
    *componentByteSize = size;
    return binary + view->offset + relativeOffset;
    }

static int readAccessor(const unsigned char *binary, size_t binaryLength,
                        const struct GltfBufferView *views, int viewCount,
                        const struct GltfAccessor *accessor, size_t element,
                        float *values, int valueCount)
    {
    const unsigned char *elementData;
    size_t size;
    int component;

    if (valueCount > accessor->components)
        return 0;
    elementData = accessorElement(binary, binaryLength, views, viewCount,
                                  accessor, element, &size);
    if (elementData == NULL)
        return 0;

    for (component = 0; component < valueCount; component++)
        values[component] = (float)readComponent(elementData + component * size,
                                                  accessor->componentType,
                                                  accessor->normalized);
    return 1;
    }

static int readAccessorIndex(const unsigned char *binary, size_t binaryLength,
                             const struct GltfBufferView *views, int viewCount,
                             const struct GltfAccessor *accessor, size_t element,
                             size_t *result)
    {
    const unsigned char *elementData;
    size_t size;

    if (accessor->components != 1 ||
        (accessor->componentType != 5121 &&
         accessor->componentType != 5123 &&
         accessor->componentType != 5125))
        return 0;
    elementData = accessorElement(binary, binaryLength, views, viewCount,
                                  accessor, element, &size);
    if (elementData == NULL)
        return 0;
    if (accessor->componentType == 5121)
        *result = elementData[0];
    else if (accessor->componentType == 5123)
        *result = (size_t)elementData[0] | ((size_t)elementData[1] << 8);
    else
        *result = (size_t)readU32(elementData);
    return 1;
    }

static size_t countPrimitiveVertices(const char *json,
                                     const struct JsonToken *tokens,
                                     int tokenCount,
                                     const struct GltfAccessor *accessors,
                                     int accessorCount, int primitive)
    {
    int mode;
    int indices;
    int attributes;
    int position;

    mode = tokenInteger(json, tokens,
        objectValue(json, tokens, tokenCount, primitive, "mode"),
        GLTF_TRIANGLES);
    if (mode != GLTF_TRIANGLES && mode != GLTF_LINES && mode != GLTF_POINTS)
        return 0;
    indices = tokenInteger(json, tokens,
        objectValue(json, tokens, tokenCount, primitive, "indices"), -1);
    if (indices >= 0 && indices < accessorCount)
        return ((mode == GLTF_TRIANGLES && accessors[indices].count % 3 == 0) ||
                (mode == GLTF_LINES && accessors[indices].count % 2 == 0) ||
                mode == GLTF_POINTS) ? accessors[indices].count : 0;
    attributes = objectValue(json, tokens, tokenCount, primitive, "attributes");
    position = tokenInteger(json, tokens,
        objectValue(json, tokens, tokenCount, attributes, "POSITION"), -1);
    if (position >= 0 && position < accessorCount)
        return ((mode == GLTF_TRIANGLES && accessors[position].count % 3 == 0) ||
                (mode == GLTF_LINES && accessors[position].count % 2 == 0) ||
                mode == GLTF_POINTS) ? accessors[position].count : 0;
    return 0;
    }

static int fillPrimitive(const char *json, const struct JsonToken *tokens,
                         int tokenCount, const unsigned char *binary,
                         size_t binaryLength,
                         const struct GltfBufferView *views, int viewCount,
                         const struct GltfAccessor *accessors, int accessorCount,
                         const struct GltfMaterial *materials, int materialCount,
                         int primitive, struct AssetVertex *vertices,
                         size_t vertexCount)
    {
    int attributes;
    int positionIndex;
    int normalIndex;
    int texcoordIndex;
    int colorIndex;
    int jointsIndex;
    int weightsIndex;
    int indicesIndex;
    int materialIndex;
    size_t output;
    size_t source;
    float color[4];
    float vertexColor[4];
    float jointValues[4];
    float weightSum;
    int influence;
    const struct GltfAccessor *position;
    const struct GltfAccessor *normal;
    const struct GltfAccessor *texcoord;
    const struct GltfAccessor *vertexColors;
    const struct GltfAccessor *joints;
    const struct GltfAccessor *weights;
    const struct GltfAccessor *indices;

    attributes = objectValue(json, tokens, tokenCount, primitive, "attributes");
    positionIndex = tokenInteger(json, tokens,
        objectValue(json, tokens, tokenCount, attributes, "POSITION"), -1);
    normalIndex = tokenInteger(json, tokens,
        objectValue(json, tokens, tokenCount, attributes, "NORMAL"), -1);
    texcoordIndex = tokenInteger(json, tokens,
        objectValue(json, tokens, tokenCount, attributes, "TEXCOORD_0"), -1);
    colorIndex = tokenInteger(json, tokens,
        objectValue(json, tokens, tokenCount, attributes, "COLOR_0"), -1);
    jointsIndex = tokenInteger(json, tokens,
        objectValue(json, tokens, tokenCount, attributes, "JOINTS_0"), -1);
    weightsIndex = tokenInteger(json, tokens,
        objectValue(json, tokens, tokenCount, attributes, "WEIGHTS_0"), -1);
    indicesIndex = tokenInteger(json, tokens,
        objectValue(json, tokens, tokenCount, primitive, "indices"), -1);
    materialIndex = tokenInteger(json, tokens,
        objectValue(json, tokens, tokenCount, primitive, "material"), -1);

    if (positionIndex < 0 || positionIndex >= accessorCount)
        return 0;
    position = &accessors[positionIndex];
    if (position->componentType != 5126 || position->components != 3)
        return 0;
    normal = normalIndex >= 0 && normalIndex < accessorCount ?
        &accessors[normalIndex] : NULL;
    texcoord = texcoordIndex >= 0 && texcoordIndex < accessorCount ?
        &accessors[texcoordIndex] : NULL;
    vertexColors = colorIndex >= 0 && colorIndex < accessorCount ?
        &accessors[colorIndex] : NULL;
    joints = jointsIndex >= 0 && jointsIndex < accessorCount ?
        &accessors[jointsIndex] : NULL;
    weights = weightsIndex >= 0 && weightsIndex < accessorCount ?
        &accessors[weightsIndex] : NULL;
    indices = indicesIndex >= 0 && indicesIndex < accessorCount ?
        &accessors[indicesIndex] : NULL;
    if ((joints == NULL) != (weights == NULL))
        return 0;
    if (joints != NULL &&
        (joints->components != 4 || joints->normalized ||
         (joints->componentType != 5121 && joints->componentType != 5123)))
        return 0;
    if (weights != NULL &&
        (weights->components != 4 ||
         (weights->componentType != 5126 &&
          !((weights->componentType == 5121 ||
             weights->componentType == 5123) && weights->normalized))))
        return 0;

    color[0] = color[1] = color[2] = color[3] = 1.0f;
    if (materialIndex >= 0 && materialIndex < materialCount)
        memcpy(color, materials[materialIndex].color, sizeof(color));

    for (output = 0; output < vertexCount; output++)
        {
        source = output;
        if (indices != NULL && !readAccessorIndex(binary, binaryLength,
            views, viewCount, indices, output, &source))
            return 0;
        if (source >= position->count)
            return 0;

        if (!readAccessor(binary, binaryLength, views, viewCount, position,
                          source, vertices[output].position, 3))
            return 0;
        vertices[output].normal[0] = 0.0f;
        vertices[output].normal[1] = 1.0f;
        vertices[output].normal[2] = 0.0f;
        if (normal != NULL && (normal->components < 3 ||
            !readAccessor(binary, binaryLength, views, viewCount, normal,
                          source, vertices[output].normal, 3)))
            return 0;
        vertices[output].texcoord[0] = 0.0f;
        vertices[output].texcoord[1] = 0.0f;
        if (texcoord != NULL && (texcoord->components < 2 ||
            !readAccessor(binary, binaryLength, views, viewCount, texcoord,
                          source, vertices[output].texcoord, 2)))
            return 0;

        vertexColor[0] = vertexColor[1] = vertexColor[2] = vertexColor[3] = 1.0f;
        if (vertexColors != NULL)
            {
            if (vertexColors->components < 3 ||
                !readAccessor(binary, binaryLength, views, viewCount,
                              vertexColors, source, vertexColor,
                              vertexColors->components > 3 ? 4 : 3))
                return 0;
            }
        vertices[output].color[0] = color[0] * vertexColor[0];
        vertices[output].color[1] = color[1] * vertexColor[1];
        vertices[output].color[2] = color[2] * vertexColor[2];
        vertices[output].color[3] = color[3] * vertexColor[3];
        if (joints != NULL)
            {
            if (source >= joints->count || source >= weights->count ||
                !readAccessor(binary, binaryLength, views, viewCount, joints,
                              source, jointValues, 4) ||
                !readAccessor(binary, binaryLength, views, viewCount, weights,
                              source, vertices[output].weights, 4))
                return 0;
            weightSum = 0.0f;
            for (influence = 0; influence < 4; influence++)
                {
                if (jointValues[influence] < 0.0f)
                    return 0;
                vertices[output].joints[influence] =
                    (unsigned int)jointValues[influence];
                if (vertices[output].weights[influence] < 0.0f)
                    return 0;
                weightSum += vertices[output].weights[influence];
                }
            if (weightSum > 0.0000001f)
                for (influence = 0; influence < 4; influence++)
                    vertices[output].weights[influence] /= weightSum;
            }
        }
    return 1;
    }

static void initializeNode(struct AssetNode *node)
    {
    int component;

    node->name = NULL;
    node->mesh = -1;
    node->skin = -1;
    node->children = NULL;
    node->childCount = 0;
    node->translation[0] = node->translation[1] = node->translation[2] = 0.0f;
    node->rotation[0] = node->rotation[1] = node->rotation[2] = 0.0f;
    node->rotation[3] = 1.0f;
    node->scale[0] = node->scale[1] = node->scale[2] = 1.0f;
    for (component = 0; component < 16; component++)
        node->matrix[component] = component % 5 == 0 ? 1.0f : 0.0f;
    node->hasMatrix = 0;
    }

static int readNodes(const char *json, const struct JsonToken *tokens,
                     int tokenCount, struct AssetModel *model)
    {
    int nodesArray;
    int nodeCount;
    int nodeIndex;
    int object;
    int childrenArray;
    int childCount;
    int child;
    int array;
    int nameToken;
    struct AssetNode *node;

    nodesArray = objectValue(json, tokens, tokenCount, 0, "nodes");
    nodeCount = arrayLength(tokens, tokenCount, nodesArray);
    if (nodeCount <= 0)
        return 1;
    model->nodes = (struct AssetNode *)calloc(nodeCount, sizeof(*model->nodes));
    if (model->nodes == NULL)
        return 0;
    model->nodeCount = (size_t)nodeCount;

    for (nodeIndex = 0; nodeIndex < nodeCount; nodeIndex++)
        {
        node = &model->nodes[nodeIndex];
        initializeNode(node);
        object = arrayItem(tokens, tokenCount, nodesArray, nodeIndex);
        nameToken = objectValue(json, tokens, tokenCount, object, "name");
        if (nameToken >= 0)
            {
            node->name = duplicateTokenString(json, &tokens[nameToken]);
            if (node->name == NULL)
                return 0;
            }
        node->mesh = tokenInteger(json, tokens,
            objectValue(json, tokens, tokenCount, object, "mesh"), -1);
        node->skin = tokenInteger(json, tokens,
            objectValue(json, tokens, tokenCount, object, "skin"), -1);
        if (node->mesh < -1 || node->mesh >= (int)model->meshCount)
            return 0;

        array = objectValue(json, tokens, tokenCount, object, "matrix");
        if (array >= 0)
            {
            if (!tokenFloatArray(json, tokens, tokenCount, array,
                                 node->matrix, 16))
                return 0;
            node->hasMatrix = 1;
            }
        else
            {
            array = objectValue(json, tokens, tokenCount, object,
                                "translation");
            if (array >= 0 && !tokenFloatArray(json, tokens, tokenCount, array,
                                               node->translation, 3))
                return 0;
            array = objectValue(json, tokens, tokenCount, object, "rotation");
            if (array >= 0 && !tokenFloatArray(json, tokens, tokenCount, array,
                                               node->rotation, 4))
                return 0;
            array = objectValue(json, tokens, tokenCount, object, "scale");
            if (array >= 0 && !tokenFloatArray(json, tokens, tokenCount, array,
                                               node->scale, 3))
                return 0;
            }

        childrenArray = objectValue(json, tokens, tokenCount, object,
                                    "children");
        childCount = arrayLength(tokens, tokenCount, childrenArray);
        if (childCount > 0)
            {
            node->children = (int *)calloc(childCount, sizeof(*node->children));
            if (node->children == NULL)
                return 0;
            node->childCount = (size_t)childCount;
            for (child = 0; child < childCount; child++)
                {
                node->children[child] = tokenInteger(json, tokens,
                    arrayItem(tokens, tokenCount, childrenArray, child), -1);
                if (node->children[child] < 0 ||
                    node->children[child] >= nodeCount)
                    return 0;
                }
            }
        }
    return 1;
    }

static int readSkins(const char *json, const struct JsonToken *tokens,
                     int tokenCount, const unsigned char *binary,
                     size_t binaryLength,
                     const struct GltfBufferView *views, int viewCount,
                     const struct GltfAccessor *accessors, int accessorCount,
                     struct AssetModel *model)
    {
    int skinsArray;
    int skinCount;
    int skinIndex;
    int skinObject;
    int jointsArray;
    int jointCount;
    int jointIndex;
    int matrixAccessorIndex;
    int nameToken;
    int component;
    struct AssetSkin *skin;
    const struct GltfAccessor *matrixAccessor;

    skinsArray = objectValue(json, tokens, tokenCount, 0, "skins");
    skinCount = arrayLength(tokens, tokenCount, skinsArray);
    if (skinCount <= 0)
        {
        size_t nodeIndex;
        for (nodeIndex = 0; nodeIndex < model->nodeCount; nodeIndex++)
            if (model->nodes[nodeIndex].skin >= 0)
                return 0;
        return 1;
        }
    model->skins = (struct AssetSkin *)calloc(skinCount,
                                               sizeof(*model->skins));
    if (model->skins == NULL)
        return 0;
    model->skinCount = (size_t)skinCount;

    for (skinIndex = 0; skinIndex < skinCount; skinIndex++)
        {
        skin = &model->skins[skinIndex];
        skin->skeleton = -1;
        skinObject = arrayItem(tokens, tokenCount, skinsArray, skinIndex);
        nameToken = objectValue(json, tokens, tokenCount, skinObject, "name");
        if (nameToken >= 0)
            {
            skin->name = duplicateTokenString(json, &tokens[nameToken]);
            if (skin->name == NULL)
                return 0;
            }
        skin->skeleton = tokenInteger(json, tokens,
            objectValue(json, tokens, tokenCount, skinObject, "skeleton"), -1);
        if (skin->skeleton < -1 || skin->skeleton >= (int)model->nodeCount)
            return 0;
        jointsArray = objectValue(json, tokens, tokenCount, skinObject,
                                  "joints");
        jointCount = arrayLength(tokens, tokenCount, jointsArray);
        if (jointCount <= 0)
            return 0;
        skin->joints = (int *)calloc(jointCount, sizeof(*skin->joints));
        skin->inverseBindMatrices = (float *)calloc(
            (size_t)jointCount * 16, sizeof(*skin->inverseBindMatrices));
        if (skin->joints == NULL || skin->inverseBindMatrices == NULL)
            return 0;
        skin->jointCount = (size_t)jointCount;
        for (jointIndex = 0; jointIndex < jointCount; jointIndex++)
            {
            skin->joints[jointIndex] = tokenInteger(json, tokens,
                arrayItem(tokens, tokenCount, jointsArray, jointIndex), -1);
            if (skin->joints[jointIndex] < 0 ||
                skin->joints[jointIndex] >= (int)model->nodeCount)
                return 0;
            for (component = 0; component < 16; component++)
                skin->inverseBindMatrices[jointIndex * 16 + component] =
                    component % 5 == 0 ? 1.0f : 0.0f;
            }
        matrixAccessorIndex = tokenInteger(json, tokens,
            objectValue(json, tokens, tokenCount, skinObject,
                        "inverseBindMatrices"), -1);
        if (matrixAccessorIndex >= 0)
            {
            if (matrixAccessorIndex >= accessorCount)
                return 0;
            matrixAccessor = &accessors[matrixAccessorIndex];
            if (matrixAccessor->componentType != 5126 ||
                matrixAccessor->components != 16 ||
                matrixAccessor->count != (size_t)jointCount)
                return 0;
            for (jointIndex = 0; jointIndex < jointCount; jointIndex++)
                if (!readAccessor(binary, binaryLength, views, viewCount,
                                  matrixAccessor, (size_t)jointIndex,
                                  skin->inverseBindMatrices + jointIndex * 16,
                                  16))
                    return 0;
            }
        }
    {
    size_t nodeIndex;
    for (nodeIndex = 0; nodeIndex < model->nodeCount; nodeIndex++)
        if (model->nodes[nodeIndex].skin < -1 ||
            model->nodes[nodeIndex].skin >= skinCount ||
            (model->nodes[nodeIndex].skin >= 0 &&
             model->nodes[nodeIndex].mesh < 0))
            return 0;
        else if (model->nodes[nodeIndex].skin >= 0)
            {
            const struct AssetMesh *mesh;
            const struct AssetSkin *nodeSkin;
            size_t vertexIndex;
            int influence;

            mesh = &model->meshes[model->nodes[nodeIndex].mesh];
            nodeSkin = &model->skins[model->nodes[nodeIndex].skin];
            for (vertexIndex = 0; vertexIndex < mesh->vertexCount;
                 vertexIndex++)
                for (influence = 0; influence < 4; influence++)
                    if (mesh->vertices[vertexIndex].weights[influence] > 0.0f &&
                        mesh->vertices[vertexIndex].joints[influence] >=
                            nodeSkin->jointCount)
                        return 0;
            }
    }
    return 1;
    }

static int readSceneRoots(const char *json, const struct JsonToken *tokens,
                          int tokenCount, struct AssetModel *model)
    {
    int scenesArray;
    int sceneCount;
    int sceneIndex;
    int sceneObject;
    int rootsArray;
    int rootCount;
    int root;
    unsigned char *isChild;
    size_t node;
    size_t child;

    if (model->nodeCount == 0)
        return 1;
    scenesArray = objectValue(json, tokens, tokenCount, 0, "scenes");
    sceneCount = arrayLength(tokens, tokenCount, scenesArray);
    sceneIndex = tokenInteger(json, tokens,
        objectValue(json, tokens, tokenCount, 0, "scene"), 0);
    if (sceneIndex >= 0 && sceneIndex < sceneCount)
        {
        sceneObject = arrayItem(tokens, tokenCount, scenesArray, sceneIndex);
        rootsArray = objectValue(json, tokens, tokenCount, sceneObject, "nodes");
        rootCount = arrayLength(tokens, tokenCount, rootsArray);
        if (rootCount > 0)
            {
            model->sceneRoots = (int *)calloc(rootCount,
                                               sizeof(*model->sceneRoots));
            if (model->sceneRoots == NULL)
                return 0;
            model->sceneRootCount = (size_t)rootCount;
            for (root = 0; root < rootCount; root++)
                {
                model->sceneRoots[root] = tokenInteger(json, tokens,
                    arrayItem(tokens, tokenCount, rootsArray, root), -1);
                if (model->sceneRoots[root] < 0 ||
                    model->sceneRoots[root] >= (int)model->nodeCount)
                    return 0;
                }
            return 1;
            }
        }

    isChild = (unsigned char *)calloc(model->nodeCount, sizeof(*isChild));
    if (isChild == NULL)
        return 0;
    for (node = 0; node < model->nodeCount; node++)
        for (child = 0; child < model->nodes[node].childCount; child++)
            isChild[model->nodes[node].children[child]] = 1;
    rootCount = 0;
    for (node = 0; node < model->nodeCount; node++)
        if (!isChild[node])
            rootCount++;
    if (rootCount > 0)
        model->sceneRoots = (int *)calloc(rootCount, sizeof(*model->sceneRoots));
    if (rootCount > 0 && model->sceneRoots == NULL)
        {
        free(isChild);
        return 0;
        }
    model->sceneRootCount = (size_t)rootCount;
    root = 0;
    for (node = 0; node < model->nodeCount; node++)
        if (!isChild[node])
            model->sceneRoots[root++] = (int)node;
    free(isChild);
    return rootCount > 0;
    }

static int animationPath(const char *json, const struct JsonToken *token)
    {
    if (tokenEquals(json, token, "translation"))
        return ASSET_ANIMATION_TRANSLATION;
    if (tokenEquals(json, token, "rotation"))
        return ASSET_ANIMATION_ROTATION;
    if (tokenEquals(json, token, "scale"))
        return ASSET_ANIMATION_SCALE;
    return 0;
    }

static int readAnimations(const char *json, const struct JsonToken *tokens,
                          int tokenCount, const unsigned char *binary,
                          size_t binaryLength,
                          const struct GltfBufferView *views, int viewCount,
                          const struct GltfAccessor *accessors,
                          int accessorCount, struct AssetModel *model)
    {
    int animationsArray;
    int animationCount;
    int animationIndex;
    int animationObject;
    int nameToken;
    int samplersArray;
    int samplerCount;
    int channelsArray;
    int channelCount;
    int channelIndex;
    int channelObject;
    int samplerIndex;
    int samplerObject;
    int inputIndex;
    int outputIndex;
    int interpolationToken;
    int targetObject;
    int targetNode;
    int pathToken;
    int path;
    int components;
    size_t keyframe;
    size_t valueCount;
    struct AssetAnimation *animation;
    struct AssetAnimationTrack *track;
    const struct GltfAccessor *input;
    const struct GltfAccessor *output;

    animationsArray = objectValue(json, tokens, tokenCount, 0, "animations");
    animationCount = arrayLength(tokens, tokenCount, animationsArray);
    if (animationCount <= 0)
        return 1;
    model->animations = (struct AssetAnimation *)calloc(
        animationCount, sizeof(*model->animations));
    if (model->animations == NULL)
        return 0;
    model->animationCount = (size_t)animationCount;

    for (animationIndex = 0; animationIndex < animationCount; animationIndex++)
        {
        animation = &model->animations[animationIndex];
        animationObject = arrayItem(tokens, tokenCount, animationsArray,
                                    animationIndex);
        nameToken = objectValue(json, tokens, tokenCount, animationObject,
                                "name");
        if (nameToken >= 0)
            {
            animation->name = duplicateTokenString(json, &tokens[nameToken]);
            if (animation->name == NULL)
                return 0;
            }
        samplersArray = objectValue(json, tokens, tokenCount, animationObject,
                                    "samplers");
        samplerCount = arrayLength(tokens, tokenCount, samplersArray);
        channelsArray = objectValue(json, tokens, tokenCount, animationObject,
                                    "channels");
        channelCount = arrayLength(tokens, tokenCount, channelsArray);
        if (channelCount <= 0)
            continue;
        animation->tracks = (struct AssetAnimationTrack *)calloc(
            channelCount, sizeof(*animation->tracks));
        if (animation->tracks == NULL)
            return 0;

        for (channelIndex = 0; channelIndex < channelCount; channelIndex++)
            {
            channelObject = arrayItem(tokens, tokenCount, channelsArray,
                                      channelIndex);
            samplerIndex = tokenInteger(json, tokens,
                objectValue(json, tokens, tokenCount, channelObject,
                            "sampler"), -1);
            targetObject = objectValue(json, tokens, tokenCount, channelObject,
                                       "target");
            targetNode = tokenInteger(json, tokens,
                objectValue(json, tokens, tokenCount, targetObject, "node"), -1);
            pathToken = objectValue(json, tokens, tokenCount, targetObject,
                                    "path");
            path = pathToken >= 0 ? animationPath(json, &tokens[pathToken]) : 0;
            if (path == 0)
                continue;
            if (samplerIndex < 0 || samplerIndex >= samplerCount ||
                targetNode < 0 || targetNode >= (int)model->nodeCount ||
                model->nodes[targetNode].hasMatrix)
                return 0;

            samplerObject = arrayItem(tokens, tokenCount, samplersArray,
                                      samplerIndex);
            inputIndex = tokenInteger(json, tokens,
                objectValue(json, tokens, tokenCount, samplerObject, "input"), -1);
            outputIndex = tokenInteger(json, tokens,
                objectValue(json, tokens, tokenCount, samplerObject, "output"), -1);
            if (inputIndex < 0 || inputIndex >= accessorCount ||
                outputIndex < 0 || outputIndex >= accessorCount)
                return 0;
            input = &accessors[inputIndex];
            output = &accessors[outputIndex];
            components = path == ASSET_ANIMATION_ROTATION ? 4 : 3;
            interpolationToken = objectValue(json, tokens, tokenCount,
                                              samplerObject, "interpolation");
            if (interpolationToken >= 0 &&
                tokenEquals(json, &tokens[interpolationToken], "CUBICSPLINE"))
                continue;
            if (interpolationToken >= 0 &&
                !tokenEquals(json, &tokens[interpolationToken], "LINEAR") &&
                !tokenEquals(json, &tokens[interpolationToken], "STEP"))
                return 0;
            if (input->componentType != 5126 || input->components != 1 ||
                output->componentType != 5126 ||
                output->components != components ||
                input->count == 0 || output->count != input->count)
                return 0;

            track = &animation->tracks[animation->trackCount];
            animation->trackCount++;
            track->interpolation = interpolationToken >= 0 &&
                tokenEquals(json, &tokens[interpolationToken], "STEP") ?
                ASSET_INTERPOLATION_STEP : ASSET_INTERPOLATION_LINEAR;
            track->node = targetNode;
            track->path = path;
            track->components = components;
            track->keyframeCount = input->count;
            if (input->count > ((size_t)-1) / (size_t)components)
                return 0;
            valueCount = input->count * (size_t)components;
            track->times = (float *)calloc(input->count, sizeof(*track->times));
            track->values = (float *)calloc(valueCount, sizeof(*track->values));
            if (track->times == NULL || track->values == NULL)
                return 0;
            for (keyframe = 0; keyframe < input->count; keyframe++)
                {
                if (!readAccessor(binary, binaryLength, views, viewCount,
                                  input, keyframe, &track->times[keyframe], 1) ||
                    !readAccessor(binary, binaryLength, views, viewCount,
                                  output, keyframe,
                                  track->values + keyframe * components,
                                  components))
                    return 0;
                if (track->times[keyframe] < 0.0f ||
                    (keyframe > 0 && track->times[keyframe] <=
                                     track->times[keyframe - 1]))
                    return 0;
                if (track->times[keyframe] > animation->duration)
                    animation->duration = track->times[keyframe];
                }
            }
        }
    return 1;
    }

static int buildStaticModel(const char *json, const struct JsonToken *tokens,
                            int tokenCount, const unsigned char *binary,
                            size_t binaryLength, struct AssetModel *model,
                            char *error, size_t errorSize)
    {
    struct GltfBufferView *views;
    struct GltfAccessor *accessors;
    struct GltfMaterial *materials;
    struct AssetMesh *mesh;
    int viewCount;
    int accessorCount;
    int materialCount;
    int meshesArray;
    int meshCount;
    int meshIndex;
    int meshObject;
    int primitivesArray;
    int primitiveCount;
    int primitiveIndex;
    int primitive;
    int mode;
    int meshPrimitive;
    int extras;
    int styleToken;
    int materialIndex;
    size_t meshVertices;
    size_t primitiveVertices;
    size_t outputOffset;
    int hasVertices;
    int success;

    views = NULL;
    accessors = NULL;
    materials = NULL;
    viewCount = accessorCount = materialCount = 0;
    hasVertices = 0;
    success = 0;

    if (!readBufferViews(json, tokens, tokenCount, &views, &viewCount) ||
        !readAccessors(json, tokens, tokenCount, &accessors, &accessorCount))
        {
        setAssetError(error, errorSize, "GLB has no supported buffer data");
        goto cleanup;
        }
    materials = readMaterials(json, tokens, tokenCount, &materialCount);
    if (materialCount < 0)
        {
        setAssetError(error, errorSize, "Out of memory loading GLB materials");
        goto cleanup;
        }

    meshesArray = objectValue(json, tokens, tokenCount, 0, "meshes");
    meshCount = arrayLength(tokens, tokenCount, meshesArray);
    if (meshCount <= 0)
        {
        setAssetError(error, errorSize, "GLB has no triangle meshes");
        goto cleanup;
        }

    model->meshes = (struct AssetMesh *)calloc(meshCount,
                                               sizeof(*model->meshes));
    if (model->meshes == NULL)
        {
        setAssetError(error, errorSize, "Out of memory loading GLB mesh");
        goto cleanup;
        }
    model->meshCount = (size_t)meshCount;

    for (meshIndex = 0; meshIndex < meshCount; meshIndex++)
        {
        mesh = &model->meshes[meshIndex];
        meshObject = arrayItem(tokens, tokenCount, meshesArray, meshIndex);
        primitivesArray = objectValue(json, tokens, tokenCount, meshObject,
                                      "primitives");
        primitiveCount = arrayLength(tokens, tokenCount, primitivesArray);
        meshVertices = 0;
        meshPrimitive = -1;
        for (primitiveIndex = 0; primitiveIndex < primitiveCount; primitiveIndex++)
            {
            primitive = arrayItem(tokens, tokenCount, primitivesArray,
                                  primitiveIndex);
            materialIndex = tokenInteger(json, tokens,
                objectValue(json, tokens, tokenCount, primitive, "material"),
                -1);
            if (materialIndex >= 0 && materialIndex < materialCount &&
                materials[materialIndex].blend)
                mesh->blend = 1;
            mode = tokenInteger(json, tokens,
                objectValue(json, tokens, tokenCount, primitive, "mode"),
                GLTF_TRIANGLES);
            if (mode == GLTF_LINES)
                mode = ASSET_PRIMITIVE_LINES;
            else if (mode == GLTF_POINTS)
                mode = ASSET_PRIMITIVE_POINTS;
            else if (mode == GLTF_TRIANGLES)
                mode = ASSET_PRIMITIVE_TRIANGLES;
            else
                {
                setAssetError(error, errorSize, "Unsupported GLB primitive mode");
                goto cleanup;
                }
            if (meshPrimitive >= 0 && meshPrimitive != mode)
                {
                setAssetError(error, errorSize,
                              "Mixed primitive modes in one GLB mesh");
                goto cleanup;
                }
            meshPrimitive = mode;
            primitiveVertices = countPrimitiveVertices(json, tokens, tokenCount,
                                                        accessors, accessorCount,
                                                        primitive);
            if (primitiveVertices > ((size_t)-1) - meshVertices)
                {
                setAssetError(error, errorSize, "GLB mesh is too large");
                goto cleanup;
                }
            meshVertices += primitiveVertices;
            }
        mesh->primitive = meshPrimitive < 0 ? ASSET_PRIMITIVE_TRIANGLES :
                                             meshPrimitive;
        if (primitiveCount > 0)
            {
            primitive = arrayItem(tokens, tokenCount, primitivesArray, 0);
            extras = objectValue(json, tokens, tokenCount, primitive, "extras");
            styleToken = objectValue(json, tokens, tokenCount, extras,
                                     "battalionLineWidth");
            if (styleToken >= 0)
                mesh->lineWidth = (float)tokenNumber(json, &tokens[styleToken]);
            styleToken = objectValue(json, tokens, tokenCount, extras,
                                     "battalionPointSize");
            if (styleToken >= 0)
                mesh->pointSize = (float)tokenNumber(json, &tokens[styleToken]);
            styleToken = objectValue(json, tokens, tokenCount, extras,
                                     "battalionUseCurrentColor");
            if (styleToken >= 0 && tokenIsTrue(json, &tokens[styleToken]))
                mesh->useCurrentColor = 1;
            }
        if (meshVertices == 0)
            continue;
        hasVertices = 1;
        mesh->vertices = (struct AssetVertex *)calloc(meshVertices,
                                                       sizeof(*mesh->vertices));
        if (mesh->vertices == NULL)
            {
            setAssetError(error, errorSize, "Out of memory loading GLB vertices");
            goto cleanup;
            }
        mesh->vertexCount = meshVertices;
        outputOffset = 0;
        for (primitiveIndex = 0; primitiveIndex < primitiveCount; primitiveIndex++)
            {
            primitive = arrayItem(tokens, tokenCount, primitivesArray,
                                  primitiveIndex);
            primitiveVertices = countPrimitiveVertices(json, tokens, tokenCount,
                                                        accessors, accessorCount,
                                                        primitive);
            if (primitiveVertices == 0)
                continue;
            if (!fillPrimitive(json, tokens, tokenCount, binary, binaryLength,
                               views, viewCount, accessors, accessorCount,
                               materials, materialCount, primitive,
                               mesh->vertices + outputOffset, primitiveVertices))
                {
                setAssetError(error, errorSize, "Unsupported or invalid GLB primitive");
                goto cleanup;
                }
            outputOffset += primitiveVertices;
            }
        }

    if (!hasVertices)
        {
        setAssetError(error, errorSize, "GLB has no supported triangle meshes");
        goto cleanup;
        }

    if (!readNodes(json, tokens, tokenCount, model) ||
        !readSkins(json, tokens, tokenCount, binary, binaryLength,
                   views, viewCount, accessors, accessorCount, model) ||
        !readSceneRoots(json, tokens, tokenCount, model) ||
        !readAnimations(json, tokens, tokenCount, binary, binaryLength,
                        views, viewCount, accessors, accessorCount, model))
        {
        setAssetError(error, errorSize,
                      "Invalid GLB scene, skin, or animation");
        goto cleanup;
        }
    success = 1;

cleanup:
    free(views);
    free(accessors);
    free(materials);
    if (!success)
        assetModelDestroy(model);
    return success;
    }

int assetModelLoadGLB(const char *path, struct AssetModel *model,
                      char *error, size_t errorSize)
    {
    FILE *file;
    long fileLength;
    unsigned char *fileData;
    unsigned char *jsonData;
    unsigned char *binaryData;
    size_t jsonLength;
    size_t binaryLength;
    size_t offset;
    size_t chunkLength;
    unsigned long chunkType;
    char *json;
    struct JsonToken *tokens;
    int tokenCapacity;
    int tokenCount;
    int result;

    assetModelInitialize(model);
    fileData = NULL;
    json = NULL;
    tokens = NULL;
    jsonData = binaryData = NULL;
    jsonLength = binaryLength = 0;
    result = 0;

    file = fopen(path, "rb");
    if (file == NULL)
        {
        setAssetError(error, errorSize, "GLB file not found");
        return 0;
        }
    if (fseek(file, 0, SEEK_END) != 0 || (fileLength = ftell(file)) < 20 ||
        fseek(file, 0, SEEK_SET) != 0)
        {
        fclose(file);
        setAssetError(error, errorSize, "Invalid GLB file length");
        return 0;
        }
    fileData = (unsigned char *)malloc((size_t)fileLength);
    if (fileData == NULL || fread(fileData, 1, (size_t)fileLength, file) !=
                            (size_t)fileLength)
        {
        fclose(file);
        free(fileData);
        setAssetError(error, errorSize, "Could not read GLB file");
        return 0;
        }
    fclose(file);

    if (memcmp(fileData, "glTF", 4) != 0 || readU32(fileData + 4) != 2 ||
        readU32(fileData + 8) != (unsigned long)fileLength)
        {
        setAssetError(error, errorSize, "Not a glTF 2.0 binary file");
        goto cleanup;
        }

    offset = 12;
    while (offset + 8 <= (size_t)fileLength)
        {
        chunkLength = (size_t)readU32(fileData + offset);
        chunkType = readU32(fileData + offset + 4);
        offset += 8;
        if (chunkLength > (size_t)fileLength - offset)
            {
            setAssetError(error, errorSize, "Invalid GLB chunk length");
            goto cleanup;
            }
        if (chunkType == GLB_JSON_CHUNK && jsonData == NULL)
            {
            jsonData = fileData + offset;
            jsonLength = chunkLength;
            }
        else if (chunkType == GLB_BIN_CHUNK && binaryData == NULL)
            {
            binaryData = fileData + offset;
            binaryLength = chunkLength;
            }
        offset += chunkLength;
        }

    if (jsonData == NULL || binaryData == NULL)
        {
        setAssetError(error, errorSize, "GLB requires JSON and BIN chunks");
        goto cleanup;
        }
    json = (char *)malloc(jsonLength + 1);
    if (json == NULL)
        {
        setAssetError(error, errorSize, "Out of memory reading GLB JSON");
        goto cleanup;
        }
    memcpy(json, jsonData, jsonLength);
    json[jsonLength] = '\0';

    tokenCapacity = (int)(jsonLength / 4) + 256;
    tokens = (struct JsonToken *)calloc(tokenCapacity, sizeof(*tokens));
    if (tokens == NULL)
        {
        setAssetError(error, errorSize, "Out of memory parsing GLB JSON");
        goto cleanup;
        }
    tokenCount = parseJson(json, jsonLength, tokens, tokenCapacity);
    if (tokenCount == -1)
        {
        free(tokens);
        tokenCapacity = (int)(jsonLength / 2) + 512;
        tokens = (struct JsonToken *)calloc(tokenCapacity, sizeof(*tokens));
        if (tokens != NULL)
            tokenCount = parseJson(json, jsonLength, tokens, tokenCapacity);
        }
    if (tokenCount <= 0 || tokens[0].type != JSON_OBJECT)
        {
        setAssetError(error, errorSize, "Invalid GLB JSON");
        goto cleanup;
        }

    result = buildStaticModel(json, tokens, tokenCount, binaryData,
                              binaryLength, model, error, errorSize);

cleanup:
    free(tokens);
    free(json);
    free(fileData);
    return result;
    }
