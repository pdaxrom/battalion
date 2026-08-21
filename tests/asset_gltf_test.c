#include "asset.h"
#include "asset_animation.h"
#include "asset_write.h"
#include "asset_tga.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void writeU32(FILE *file, unsigned long value)
    {
    unsigned char bytes[4];

    bytes[0] = (unsigned char)(value & 255);
    bytes[1] = (unsigned char)((value >> 8) & 255);
    bytes[2] = (unsigned char)((value >> 16) & 255);
    bytes[3] = (unsigned char)((value >> 24) & 255);
    fwrite(bytes, 1, 4, file);
    }

static int writeFixture(const char *path)
    {
    static const char jsonText[] =
        "{\"asset\":{\"version\":\"2.0\"},"
        "\"buffers\":[{\"byteLength\":76}],"
        "\"bufferViews\":["
        "{\"buffer\":0,\"byteOffset\":0,\"byteLength\":36},"
        "{\"buffer\":0,\"byteOffset\":36,\"byteLength\":6},"
        "{\"buffer\":0,\"byteOffset\":44,\"byteLength\":8},"
        "{\"buffer\":0,\"byteOffset\":52,\"byteLength\":24}],"
        "\"accessors\":["
        "{\"bufferView\":0,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
        "{\"bufferView\":1,\"componentType\":5123,\"count\":3,\"type\":\"SCALAR\"},"
        "{\"bufferView\":2,\"componentType\":5126,\"count\":2,\"type\":\"SCALAR\"},"
        "{\"bufferView\":3,\"componentType\":5126,\"count\":2,\"type\":\"VEC3\"}],"
        "\"materials\":[{\"pbrMetallicRoughness\":{"
        "\"baseColorFactor\":[1,0.5,0.25,1]}}],"
        "\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0},"
        "\"indices\":1,\"material\":0}]}],"
        "\"nodes\":[{\"translation\":[2,0,0],\"children\":[1]},"
        "{\"mesh\":0,\"translation\":[0,3,0]}],"
        "\"scenes\":[{\"nodes\":[0]}],\"scene\":0,"
        "\"animations\":[{\"name\":\"move\",\"samplers\":[{"
        "\"input\":2,\"output\":3,\"interpolation\":\"LINEAR\"}],"
        "\"channels\":[{\"sampler\":0,\"target\":{\"node\":1,"
        "\"path\":\"translation\"}}]}]}";
    static const float positions[9] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };
    static const unsigned char indices[8] = {0, 0, 1, 0, 2, 0, 0, 0};
    static const float times[2] = {0.0f, 2.0f};
    static const float translations[6] = {
        0.0f, 3.0f, 0.0f,
        4.0f, 3.0f, 0.0f
    };
    FILE *file;
    size_t jsonLength;
    size_t paddedJsonLength;
    unsigned long totalLength;
    size_t index;

    jsonLength = strlen(jsonText);
    paddedJsonLength = (jsonLength + 3) & ~(size_t)3;
    totalLength = (unsigned long)(12 + 8 + paddedJsonLength + 8 +
                                  sizeof(positions) + sizeof(indices) +
                                  sizeof(times) + sizeof(translations));

    file = fopen(path, "wb");
    if (file == NULL)
        return 0;
    fwrite("glTF", 1, 4, file);
    writeU32(file, 2);
    writeU32(file, totalLength);
    writeU32(file, (unsigned long)paddedJsonLength);
    writeU32(file, 0x4e4f534aUL);
    fwrite(jsonText, 1, jsonLength, file);
    for (index = jsonLength; index < paddedJsonLength; index++)
        fputc(' ', file);
    writeU32(file, (unsigned long)(sizeof(positions) + sizeof(indices) +
                                  sizeof(times) + sizeof(translations)));
    writeU32(file, 0x004e4942UL);
    fwrite(positions, 1, sizeof(positions), file);
    fwrite(indices, 1, sizeof(indices), file);
    fwrite(times, 1, sizeof(times), file);
    fwrite(translations, 1, sizeof(translations), file);
    fclose(file);
    return 1;
    }

static int nearValue(float left, float right)
    {
    return fabs(left - right) < 0.0001;
    }

static int testStaticRoundTrip(const struct AssetModel *source)
    {
    const char *path;
    struct AssetModel loaded;
    char error[160];
    int result;

    path = "/tmp/battalion-asset-write-test.glb";
    if (!assetModelWriteStaticGLB(path, source, error, sizeof(error)))
	{
	fprintf(stderr, "write failed: %s\n", error);
	return 0;
	}
    assetModelInitialize(&loaded);
    result = assetModelLoadGLB(path, &loaded, error, sizeof(error));
    remove(path);
    if (!result)
	{
	fprintf(stderr, "round-trip load failed: %s\n", error);
	return 0;
	}
    result = loaded.meshCount == 1 &&
	     loaded.meshes[0].vertexCount == source->meshes[0].vertexCount &&
	     loaded.nodeCount == source->nodeCount &&
	     loaded.sceneRootCount == source->sceneRootCount &&
	     loaded.animationCount == 1 &&
	     loaded.animations[0].trackCount == 1 &&
	     loaded.animations[0].tracks[0].path ==
		 ASSET_ANIMATION_TRANSLATION &&
	     nearValue(loaded.animations[0].tracks[0].values[3], 4.0f) &&
	     nearValue(loaded.meshes[0].vertices[1].position[0],
		       source->meshes[0].vertices[1].position[0]) &&
	     nearValue(loaded.meshes[0].vertices[0].color[1],
		       source->meshes[0].vertices[0].color[1]);
    assetModelDestroy(&loaded);
    if (!result)
	fprintf(stderr, "unexpected round-trip vertex data\n");
    return result;
    }

static int testExportedTank(void)
    {
    struct AssetModel model;
    char error[160];
    int result;

    assetModelInitialize(&model);
    if (!assetModelLoadGLB("assets/models/tank.glb", &model,
			   error, sizeof(error)))
	{
	fprintf(stderr, "exported tank load failed: %s\n", error);
	return 0;
	}
    result = model.meshCount == 1 &&
	     model.meshes[0].vertexCount == 168 &&
	     model.nodeCount == 1 &&
	     model.sceneRootCount == 1 &&
	     model.sceneRoots[0] == 0 &&
	     model.nodes[0].mesh == 0;
    assetModelDestroy(&model);
    if (!result)
	fprintf(stderr, "unexpected exported tank structure\n");
    return result;
    }

static int testGroundUsesRuntimeColor(void)
    {
    static const char *paths[3] = {
        "assets/models/plane-ground-low.glb",
        "assets/models/plane-ground-high.glb",
        "assets/models/plane-ground-large.glb"
    };
    struct AssetModel model;
    char error[160];
    int pathIndex;
    size_t mesh;

    assetModelInitialize(&model);
    for (pathIndex = 0; pathIndex < 3; pathIndex++)
        {
        if (!assetModelLoadGLB(paths[pathIndex], &model,
                               error, sizeof(error)))
            {
            fprintf(stderr, "ground model load failed for %s: %s\n",
                    paths[pathIndex], error);
            return 0;
            }
        for (mesh = 0; mesh < model.meshCount; mesh++)
            if (!model.meshes[mesh].useCurrentColor)
                {
                fprintf(stderr, "ground model lost runtime colour: %s\n",
                        paths[pathIndex]);
                assetModelDestroy(&model);
                return 0;
                }
        assetModelDestroy(&model);
        }
    return 1;
    }

static int testPrimitiveRoundTrip(void)
    {
    struct AssetModel source;
    struct AssetModel loaded;
    char error[160];
    size_t mesh;
    size_t vertex;
    static const size_t counts[3] = {3, 2, 1};
    static const int primitives[3] = {
        ASSET_PRIMITIVE_TRIANGLES,
        ASSET_PRIMITIVE_LINES,
        ASSET_PRIMITIVE_POINTS
    };
    int result;

    assetModelInitialize(&source);
    assetModelInitialize(&loaded);
    source.meshes = (struct AssetMesh *)calloc(3, sizeof(*source.meshes));
    if (source.meshes == NULL)
        return 0;
    source.meshCount = 3;
    for (mesh = 0; mesh < 3; mesh++)
        {
        source.meshes[mesh].vertices = (struct AssetVertex *)calloc(
            counts[mesh], sizeof(struct AssetVertex));
        if (source.meshes[mesh].vertices == NULL)
            {
            assetModelDestroy(&source);
            return 0;
            }
        source.meshes[mesh].vertexCount = counts[mesh];
        source.meshes[mesh].primitive = primitives[mesh];
        source.meshes[mesh].lineWidth = 2.5f;
        source.meshes[mesh].pointSize = 3.5f;
        source.meshes[mesh].useCurrentColor = mesh == 1;
        for (vertex = 0; vertex < counts[mesh]; vertex++)
            {
            source.meshes[mesh].vertices[vertex].position[0] = (float)vertex;
            source.meshes[mesh].vertices[vertex].position[1] = (float)mesh;
            source.meshes[mesh].vertices[vertex].color[0] = 1.0f;
            source.meshes[mesh].vertices[vertex].color[3] = 1.0f;
            }
        }
    result = assetModelWriteGLB("/tmp/battalion-primitives.glb", &source,
                                error, sizeof(error)) &&
             assetModelLoadGLB("/tmp/battalion-primitives.glb", &loaded,
                               error, sizeof(error)) &&
             loaded.meshCount == 3 &&
             loaded.meshes[0].primitive == ASSET_PRIMITIVE_TRIANGLES &&
             loaded.meshes[1].primitive == ASSET_PRIMITIVE_LINES &&
             loaded.meshes[2].primitive == ASSET_PRIMITIVE_POINTS &&
             nearValue(loaded.meshes[1].lineWidth, 2.5f) &&
             nearValue(loaded.meshes[2].pointSize, 3.5f) &&
             !loaded.meshes[0].useCurrentColor &&
             loaded.meshes[1].useCurrentColor &&
             !loaded.meshes[2].useCurrentColor;
    remove("/tmp/battalion-primitives.glb");
    if (!result)
        fprintf(stderr, "primitive round-trip failed: %s\n", error);
    assetModelDestroy(&loaded);
    assetModelDestroy(&source);
    return result;
    }

static int testExportedHelicopter(void)
    {
    struct AssetModel model;
    struct AssetPose pose;
    char error[160];
    int result;

    assetModelInitialize(&model);
    assetPoseInitialize(&pose);
    if (!assetModelLoadGLB("assets/models/helo.glb", &model,
			   error, sizeof(error)))
	{
	fprintf(stderr, "exported helicopter load failed: %s\n", error);
	return 0;
	}
    result = model.meshCount == 3 &&
	     model.meshes[0].vertexCount == 324 &&
	     model.meshes[1].vertexCount == 72 &&
	     model.meshes[2].vertexCount == 36 &&
	     model.nodeCount == 3 &&
	     nearValue(model.nodes[2].translation[0], 0.04f) &&
	     nearValue(model.nodes[2].translation[2], 0.375f) &&
	     model.animationCount == 1 &&
	     model.animations[0].trackCount == 2 &&
	     strcmp(model.animations[0].name, "rotors") == 0 &&
	     assetPoseSample(&model, 0, 0.25f, 1, &pose) &&
	     nearValue(pose.nodes[1].rotation[1], -0.70710678f) &&
	     nearValue(pose.nodes[1].rotation[3], 0.70710678f) &&
	     nearValue(pose.nodes[2].rotation[0], -0.70710678f);
    assetPoseDestroy(&pose);
    assetModelDestroy(&model);
    if (!result)
	fprintf(stderr, "unexpected exported helicopter or rotor animation\n");
    return result;
    }

static int testExportedSlagHelicopter(void)
    {
    struct AssetModel model;
    char error[160];
    int result;

    assetModelInitialize(&model);
    result = assetModelLoadGLB(
        "assets/models/helo-slag.glb", &model,
        error, sizeof(error)) &&
        model.meshCount == 3 && model.nodeCount == 3 &&
        model.animationCount == 1 &&
        strcmp(model.animations[0].name, "rotors") == 0;
    if (!result)
        fprintf(stderr, "slag helicopter animation load failed: %s\n", error);
    assetModelDestroy(&model);
    return result;
    }

static int testExternalTextures(void)
    {
    static const char *paths[5] = {
        "assets/textures/screenleft.tga",
        "assets/textures/screenright.tga",
        "assets/textures/treewood.tga",
        "assets/textures/road.tga",
        "assets/textures/logo.tga"
    };
    static const int widths[5] = {128, 128, 96, 128, 95};
    static const int heights[5] = {128, 128, 96, 128, 64};
    struct AssetImage image;
    char error[160];
    int index;

    assetImageInitialize(&image);
    for (index = 0; index < 5; index++)
        {
        if (!assetImageLoadTGA(paths[index], &image, error, sizeof(error)) ||
            image.width != widths[index] || image.height != heights[index] ||
            image.pixels == NULL)
            {
            fprintf(stderr, "external TGA load failed for %s: %s\n",
                    paths[index], error);
            assetImageDestroy(&image);
            return 0;
            }
        assetImageDestroy(&image);
        }
    return 1;
    }

int main(void)
    {
    const char *path;
    struct AssetModel model;
    struct AssetPose pose;
    char error[160];
    int result;

    path = "/tmp/battalion-asset-gltf-test.glb";
    if (!writeFixture(path))
        return 1;

    assetModelInitialize(&model);
    assetPoseInitialize(&pose);
    result = assetModelLoadGLB(path, &model, error, sizeof(error));
    remove(path);
    if (!result)
        {
        fprintf(stderr, "load failed: %s\n", error);
        return 1;
        }
    if (model.meshCount != 1 || model.meshes[0].vertexCount != 3)
        {
        fprintf(stderr, "unexpected mesh size\n");
        assetModelDestroy(&model);
        return 1;
        }
    if (model.nodeCount != 2 || model.sceneRootCount != 1 ||
        model.sceneRoots[0] != 0 || model.nodes[0].childCount != 1 ||
        model.nodes[0].children[0] != 1 || model.nodes[1].mesh != 0 ||
        !nearValue(model.nodes[0].translation[0], 2.0f) ||
        !nearValue(model.nodes[1].translation[1], 3.0f))
        {
        fprintf(stderr, "unexpected scene hierarchy\n");
        assetModelDestroy(&model);
        return 1;
        }
    if (model.animationCount != 1 || model.animations[0].trackCount != 1 ||
        strcmp(model.animations[0].name, "move") != 0 ||
        !nearValue(model.animations[0].duration, 2.0f) ||
        model.animations[0].tracks[0].node != 1 ||
        model.animations[0].tracks[0].path != ASSET_ANIMATION_TRANSLATION ||
        model.animations[0].tracks[0].keyframeCount != 2 ||
        !nearValue(model.animations[0].tracks[0].values[3], 4.0f))
        {
        fprintf(stderr, "unexpected animation data\n");
        assetModelDestroy(&model);
        return 1;
        }
    if (!nearValue(model.meshes[0].vertices[1].position[0], 1.0f) ||
        !nearValue(model.meshes[0].vertices[2].position[1], 1.0f) ||
        !nearValue(model.meshes[0].vertices[0].color[1], 0.5f) ||
        !nearValue(model.meshes[0].vertices[0].color[2], 0.25f))
        {
        fprintf(stderr, "unexpected vertex data\n");
        assetModelDestroy(&model);
	return 1;
	}
    if (!testStaticRoundTrip(&model))
	{
	assetModelDestroy(&model);
	return 1;
	}
    if (!assetPoseSample(&model, 0, 1.0f, 0, &pose) ||
        pose.nodeCount != 2 ||
        !nearValue(pose.nodes[1].translation[0], 2.0f) ||
        !nearValue(pose.nodes[1].translation[1], 3.0f))
        {
        fprintf(stderr, "unexpected sampled pose\n");
        assetPoseDestroy(&pose);
        assetModelDestroy(&model);
        return 1;
        }
    assetPoseDestroy(&pose);
    assetModelDestroy(&model);
    if (!testExportedTank())
	return 1;
    if (!testGroundUsesRuntimeColor())
	return 1;
    if (!testExportedHelicopter())
	return 1;
    if (!testExportedSlagHelicopter())
	return 1;
    if (!testPrimitiveRoundTrip())
	return 1;
    if (!testExternalTextures())
	return 1;
    return 0;
    }
