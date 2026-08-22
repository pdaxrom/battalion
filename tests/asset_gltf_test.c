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
        "\"nodes\":[{\"name\":\"root\",\"translation\":[2,0,0],\"children\":[1]},"
        "{\"name\":\"moving-part\",\"mesh\":0,\"translation\":[0,3,0]}],"
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
	     loaded.nodes[0].name != NULL &&
	     strcmp(loaded.nodes[0].name, "root") == 0 &&
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
    struct AssetPose startPose;
    struct AssetPose movingPose;
    char error[160];
    int treadNode;
    int muzzleNode;
    float muzzleMatrix[16];
    int result;

    assetModelInitialize(&model);
    assetPoseInitialize(&startPose);
    assetPoseInitialize(&movingPose);
    if (!assetModelLoadGLB("assets/models/vehicles/tank.glb", &model,
			   error, sizeof(error)))
	{
	fprintf(stderr, "exported tank load failed: %s\n", error);
	return 0;
	}
    result = model.meshCount == 5 &&
             model.meshes[0].vertexCount == 72 &&
             model.meshes[1].vertexCount == 30 &&
             model.meshes[2].vertexCount == 30 &&
             model.meshes[3].vertexCount == 36 &&
             model.meshes[4].vertexCount == 36 &&
             model.nodeCount == 30 &&
             model.sceneRootCount == 1 &&
             model.sceneRoots[0] == 0 &&
             model.nodes[0].mesh == -1 &&
             model.nodes[0].childCount == 27 &&
             model.nodes[0].children[0] == 1 &&
             model.nodes[0].children[1] == 2 &&
             model.nodes[0].children[2] == 3 &&
             model.nodes[3].childCount == 1 &&
             model.nodes[3].children[0] == 4 &&
             model.nodes[4].childCount == 1 &&
             model.nodes[4].children[0] == 29 &&
             model.animationCount == 1 &&
             model.animations[0].trackCount == 24 &&
             strcmp(model.animations[0].name, "tracks") == 0 &&
             assetModelFindNode(&model, "body") == 0 &&
             assetModelFindNode(&model, "tracks") == 1 &&
             assetModelFindNode(&model, "upper-hull") == 2 &&
             assetModelFindNode(&model, "turret") == 3 &&
             assetModelFindNode(&model, "barrel") == 4 &&
             assetModelFindNode(&model, "muzzle") == 29 &&
             assetModelFindNode(&model, "tread-left-00") == 5 &&
             assetModelFindNode(&model, "tread-right-11") == 28;
    treadNode = assetModelFindNode(&model, "tread-left-00");
    muzzleNode = assetModelFindNode(&model, "muzzle");
    if (result)
        result = assetPoseSample(&model, 0, 0.0f, 1, &startPose) &&
                 assetPoseSample(&model, 0, 0.3f, 1, &movingPose) &&
                 treadNode >= 0 &&
                 muzzleNode >= 0 &&
                 assetModelNodeWorldMatrix(&model, muzzleNode, &startPose,
                                           muzzleMatrix) &&
                 nearValue(muzzleMatrix[12], 0.0f) &&
                 nearValue(muzzleMatrix[13], 0.195f) &&
                 nearValue(muzzleMatrix[14], 0.24f) &&
                 (!nearValue(startPose.nodes[treadNode].translation[1],
                             movingPose.nodes[treadNode].translation[1]) ||
                  !nearValue(startPose.nodes[treadNode].translation[2],
                             movingPose.nodes[treadNode].translation[2]));
    assetPoseDestroy(&movingPose);
    assetPoseDestroy(&startPose);
    assetModelDestroy(&model);
    if (!result)
	fprintf(stderr, "unexpected exported tank structure\n");
    return result;
    }

static int testGroundUsesRuntimeColor(void)
    {
    static const char *paths[3] = {
        "assets/models/terrain/plane-ground-low.glb",
        "assets/models/terrain/plane-ground-high.glb",
        "assets/models/terrain/plane-ground-large.glb"
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
        source.meshes[mesh].blend = mesh == 2;
        for (vertex = 0; vertex < counts[mesh]; vertex++)
            {
            source.meshes[mesh].vertices[vertex].position[0] = (float)vertex;
            source.meshes[mesh].vertices[vertex].position[1] = (float)mesh;
            source.meshes[mesh].vertices[vertex].normal[1] = 1.0f;
            source.meshes[mesh].vertices[vertex].texcoord[0] =
                (float)vertex * 0.25f;
            source.meshes[mesh].vertices[vertex].texcoord[1] =
                (float)mesh * 0.5f;
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
             !loaded.meshes[2].useCurrentColor &&
             !loaded.meshes[0].blend && !loaded.meshes[1].blend &&
             loaded.meshes[2].blend &&
             nearValue(loaded.meshes[0].vertices[1].normal[1], 1.0f) &&
             nearValue(loaded.meshes[0].vertices[1].texcoord[0], 0.25f) &&
             nearValue(loaded.meshes[1].vertices[0].texcoord[1], 0.5f);
    remove("/tmp/battalion-primitives.glb");
    if (!result)
        fprintf(stderr, "primitive round-trip failed: %s\n", error);
    assetModelDestroy(&loaded);
    assetModelDestroy(&source);
    return result;
    }

static int testRoadModels(void)
    {
    static const char *paths[4] = {
        "assets/models/terrain/roads/road-type-0.glb",
        "assets/models/terrain/roads/road-type-1.glb",
        "assets/models/terrain/roads/road-type-2.glb",
        "assets/models/terrain/roads/road-type-3.glb"
    };
    struct AssetModel model;
    const struct AssetMesh *surfaceMesh;
    char error[160];
    int pathIndex;
    int surface;
    int marker;
    int textured;
    size_t vertex;

    assetModelInitialize(&model);
    for (pathIndex = 0; pathIndex < 4; pathIndex++)
        {
        if (!assetModelLoadGLB(paths[pathIndex], &model,
                               error, sizeof(error)))
            {
            fprintf(stderr, "road model load failed for %s: %s\n",
                    paths[pathIndex], error);
            return 0;
            }
        surface = assetModelFindNode(&model, "surface");
        marker = assetModelFindNode(&model, "marker");
        if (surface < 0 || model.nodes[surface].mesh < 0 ||
            (pathIndex < 3 && marker < 0) ||
            (pathIndex == 3 && marker >= 0))
            {
            fprintf(stderr, "road nodes are invalid in %s\n",
                    paths[pathIndex]);
            assetModelDestroy(&model);
            return 0;
            }
        surfaceMesh = &model.meshes[model.nodes[surface].mesh];
        textured = 0;
        for (vertex = 0; vertex < surfaceMesh->vertexCount; vertex++)
            if (!nearValue(surfaceMesh->vertices[vertex].texcoord[0], 0.0f) ||
                !nearValue(surfaceMesh->vertices[vertex].texcoord[1], 0.0f))
                textured = 1;
        if (!surfaceMesh->useCurrentColor || !textured ||
            surfaceMesh->vertexCount == 0 ||
            !nearValue(surfaceMesh->vertices[0].normal[1], 1.0f))
            {
            fprintf(stderr, "road surface attributes are invalid in %s\n",
                    paths[pathIndex]);
            assetModelDestroy(&model);
            return 0;
            }
        assetModelDestroy(&model);
        }
    return 1;
    }

static int testExportedHelicopter(void)
    {
    struct AssetModel model;
    struct AssetPose pose;
    char error[160];
    int result;

    assetModelInitialize(&model);
    assetPoseInitialize(&pose);
    if (!assetModelLoadGLB("assets/models/vehicles/helo.glb", &model,
			   error, sizeof(error)))
	{
	fprintf(stderr, "exported helicopter load failed: %s\n", error);
	return 0;
	}
    result = model.meshCount == 3 &&
	     model.meshes[0].vertexCount == 324 &&
	     model.meshes[1].vertexCount == 72 &&
	     model.meshes[2].vertexCount == 36 &&
	     model.nodeCount == 4 &&
	     model.sceneRootCount == 4 &&
	     assetModelFindNode(&model, "muzzle") == 3 &&
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
        "assets/models/vehicles/helo-slag.glb", &model,
        error, sizeof(error)) &&
        model.meshCount == 3 && model.nodeCount == 3 &&
        model.animationCount == 1 &&
        strcmp(model.animations[0].name, "rotors") == 0;
    if (!result)
        fprintf(stderr, "slag helicopter animation load failed: %s\n", error);
    assetModelDestroy(&model);
    return result;
    }

static int testAnimatedVapour(void)
    {
    struct AssetModel model;
    struct AssetPose pose;
    float muzzleMatrix[16];
    char error[160];
    int muzzleNode;
    int result;

    assetModelInitialize(&model);
    assetPoseInitialize(&pose);
    result = assetModelLoadGLB("assets/models/monsters/vapour.glb",
                               &model, error, sizeof(error)) &&
             model.meshCount == 1 && model.nodeCount == 46 &&
             model.meshes[0].blend &&
             nearValue(model.meshes[0].vertices[0].color[3], 0.2f) &&
             model.sceneRootCount == 46 && model.animationCount == 1 &&
             model.animations[0].name != NULL &&
             strcmp(model.animations[0].name, "cloud") == 0 &&
             model.animations[0].trackCount == 90 &&
             assetPoseSample(&model, 0, 0.8f, 1, &pose) &&
             pose.nodeCount == 46;
    muzzleNode = assetModelFindNode(&model, "muzzle");
    if (result)
        result = muzzleNode == 45 &&
                 assetModelNodeWorldMatrix(&model, muzzleNode, &pose,
                                           muzzleMatrix) &&
                 nearValue(muzzleMatrix[12], 0.0f) &&
                 nearValue(muzzleMatrix[13], -0.88f) &&
                 nearValue(muzzleMatrix[14], 0.0f) &&
                 nearValue(muzzleMatrix[8], 0.0f) &&
                 nearValue(muzzleMatrix[9], -1.0f) &&
                 nearValue(muzzleMatrix[10], 0.0f);
    if (!result)
        fprintf(stderr, "animated Vapour asset is invalid: %s\n", error);
    assetPoseDestroy(&pose);
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

static int testCompoundModels(void)
    {
    static const char *paths[6] = {
        "assets/models/vehicles/airplane.glb",
        "assets/models/monsters/googelon.glb",
        "assets/models/monsters/flutter.glb",
        "assets/models/monsters/techs.glb",
        "assets/models/effects/boom-high.glb",
        "assets/models/effects/boom-low.glb"
    };
    static const size_t meshCounts[6] = {3, 7, 6, 5, 10, 10};
    static const size_t nodeCounts[6] = {4, 8, 7, 6, 11, 11};
    static const char *requiredNodes[6] = {
        "propeller-right", "tail-simple", "wing-right",
        "head-simple", "frame-10", "frame-10"
    };
    static const char *clips[6] = {
        "propellers", "tail-sway", "wing-flap", "death", "explode",
        "explode"
    };
    struct AssetModel model;
    char error[160];
    int index;
    int valid;

    assetModelInitialize(&model);
    for (index = 0; index < 6; index++)
        {
        valid = assetModelLoadGLB(paths[index], &model,
                                  error, sizeof(error)) &&
                model.meshCount >= meshCounts[index] &&
                model.nodeCount >= nodeCounts[index] &&
                model.sceneRootCount >= 1 &&
                assetModelFindNode(&model, requiredNodes[index]) >= 0 &&
                ((clips[index] == NULL && model.animationCount == 0) ||
                 (clips[index] != NULL && model.animationCount == 1 &&
                  model.animations[0].name != NULL &&
                  strcmp(model.animations[0].name, clips[index]) == 0));
        if (!valid)
            {
            fprintf(stderr, "invalid compound model %s: %s\n",
                    paths[index], error);
            assetModelDestroy(&model);
            return 0;
            }
        assetModelDestroy(&model);
        }
    return 1;
    }

static int testCharacterModels(void)
    {
    static const char *paths[3] = {
        "assets/models/monsters/hero-typhoon.glb",
        "assets/models/monsters/hero-beam.glb",
        "assets/models/monsters/mecha.glb"
        };
    static const char *nodes[3] = {
        "leg-right", "forearm-left", "eye-intact"
        };
    static const char *firstClips[3] = {
        "walk", "walk", "eye-scan"
        };
    static const char *secondClips[2] = {"fire", "fire"};
    struct AssetModel model;
    struct AssetPose pose;
    char error[160];
    int index;

    assetModelInitialize(&model);
    assetPoseInitialize(&pose);
    for (index = 0; index < 3; index++)
        {
        if (!assetModelLoadGLB(paths[index], &model, error, sizeof(error)) ||
            model.meshCount == 0 || model.nodeCount == 0 ||
            model.sceneRootCount != 1 ||
            assetModelFindNode(&model, nodes[index]) < 0 ||
            model.animationCount != (index < 2 ? 2 : 1) ||
            strcmp(model.animations[0].name, firstClips[index]) != 0 ||
            (index < 2 &&
             strcmp(model.animations[1].name, secondClips[index]) != 0) ||
            !assetPoseSample(&model, 0, 1.0f, 1, &pose) ||
            (index < 2 && !assetPoseApply(&model, 1, 1.0f, 0, &pose)))
            {
            fprintf(stderr, "invalid character model %s: %s\n",
                    paths[index], error);
            assetPoseDestroy(&pose);
            assetModelDestroy(&model);
            return 0;
            }
        assetPoseDestroy(&pose);
        assetModelDestroy(&model);
        }
    return 1;
    }

static int testCreatureAimNodes(void)
    {
    static const char *paths[3] = {
        "assets/models/monsters/googelon.glb",
        "assets/models/monsters/flutter.glb",
        "assets/models/monsters/techs.glb"
        };
    struct AssetModel model;
    char error[160];
    int index;

    assetModelInitialize(&model);
    for (index = 0; index < 3; index++)
        {
        if (!assetModelLoadGLB(paths[index], &model, error, sizeof(error)) ||
            assetModelFindNode(&model, "head") < 0 ||
            assetModelFindNode(&model, "muzzle") < 0)
            {
            fprintf(stderr, "creature aim node missing in %s: %s\n",
                    paths[index], error);
            assetModelDestroy(&model);
            return 0;
            }
        assetModelDestroy(&model);
        }
    return 1;
    }

static int testProjectileAnimations(void)
    {
    static const char *paths[2] = {
        "assets/models/effects/projectile-techs.glb",
        "assets/models/effects/projectile-hero.glb"
        };
    struct AssetModel model;
    struct AssetPose pose;
    char error[160];
    int index;

    assetModelInitialize(&model);
    assetPoseInitialize(&pose);
    for (index = 0; index < 2; index++)
        {
        if (!assetModelLoadGLB(paths[index], &model, error, sizeof(error)) ||
            model.animationCount != 1 ||
            strcmp(model.animations[0].name, "spin") != 0 ||
            model.animations[0].trackCount != 1 ||
            model.animations[0].tracks[0].path !=
                ASSET_ANIMATION_ROTATION ||
            !assetPoseSample(&model, 0, 9.0f, 1, &pose) ||
            nearValue(pose.nodes[0].rotation[3], 1.0f))
            {
            fprintf(stderr, "invalid projectile animation %s: %s\n",
                    paths[index], error);
            assetPoseDestroy(&pose);
            assetModelDestroy(&model);
            return 0;
            }
        assetPoseDestroy(&pose);
        assetModelDestroy(&model);
        }
    return 1;
    }

int main(void)
    {
    const char *path;
    struct AssetModel model;
    struct AssetPose pose;
    char error[160];
    float minimum[3];
    float maximum[3];
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
        assetModelFindNode(&model, "root") != 0 ||
        assetModelFindNode(&model, "moving-part") != 1 ||
        !nearValue(model.nodes[0].translation[0], 2.0f) ||
        !nearValue(model.nodes[1].translation[1], 3.0f))
        {
        fprintf(stderr, "unexpected scene hierarchy\n");
        assetModelDestroy(&model);
        return 1;
        }
    if (!assetModelPoseBounds(&model, NULL, minimum, maximum) ||
        !nearValue(minimum[0], 2.0f) ||
        !nearValue(minimum[1], 3.0f) ||
        !nearValue(minimum[2], 0.0f) ||
        !nearValue(maximum[0], 3.0f) ||
        !nearValue(maximum[1], 4.0f) ||
        !nearValue(maximum[2], 0.0f))
        {
        fprintf(stderr, "unexpected rest-pose bounds\n");
        assetModelDestroy(&model);
        return 1;
        }
    if (!assetModelNodePoseBounds(&model, 1, NULL, minimum, maximum) ||
        !nearValue(minimum[0], 0.0f) ||
        !nearValue(minimum[1], 3.0f) ||
        !nearValue(maximum[0], 1.0f) ||
        !nearValue(maximum[1], 4.0f))
        {
        fprintf(stderr, "unexpected selected-node bounds\n");
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
        !nearValue(pose.nodes[1].translation[1], 3.0f) ||
        !assetModelPoseBounds(&model, &pose, minimum, maximum) ||
        !nearValue(minimum[0], 4.0f) ||
        !nearValue(minimum[1], 3.0f) ||
        !nearValue(maximum[0], 5.0f) ||
        !nearValue(maximum[1], 4.0f))
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
    if (!testAnimatedVapour())
	return 1;
    if (!testPrimitiveRoundTrip())
	return 1;
    if (!testRoadModels())
	return 1;
    if (!testExternalTextures())
	return 1;
    if (!testCompoundModels())
	return 1;
    if (!testCharacterModels())
	return 1;
    if (!testCreatureAimNodes())
	return 1;
    if (!testProjectileAnimations())
	return 1;
    return 0;
    }
