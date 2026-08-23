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
/* Modifications copyright (c) 2026 Battalion contributors. */
/* graphics.c v 1.3                                                    */
/* high level graphics routines for battalion                          */
/***********************************************************************/

#include "battalion.h"

#include "asset.h"
#include "asset_animation.h"
#include "asset_bindings.h"
#include "asset_manifest.h"
#include "asset_render.h"
#include "asset_values.h"
#include "entity_asset_validation.h"
#include "dynamic_primitives.h"

    /*************/
    /* objects   */
    /*************/

    GLuint titleObj, titleObjOO, titleObjOOO;
    static GLuint lowDPlane, highDPlane, bigPlane;
    static GLuint directionArrow;

    /***************************/
    /* texture mapping stuff   */
    /***************************/

    static struct AssetManifest externalAssets;
    static struct AssetBindings externalBindings;
    static struct AssetValues externalEffectValues;
    static struct AssetValues externalPaletteValues;
    static struct AssetPose externalEntityPose;


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* create the objects to be used in the game                     */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const struct AssetManifestModel *requiredModelEntry(const char *key)
    {
    const struct AssetManifestModel *model;

    model = assetManifestFindModel(&externalAssets, key);
    if (model == NULL || model->displayList == 0)
	{
	fprintf(stderr, "Required external model is missing: %s\n", key);
	showError("Required model is missing from manifest.txt");
	exit(1);
	}
    return model;
    }
static GLuint requiredModel(const char *key)
    {
    const struct AssetManifestModel *model;

    model = requiredModelEntry(key);
    return model->displayList;
    }

static const char *requiredBinding(int type, const char *slot)
    {
    const char *key;

    key = assetBindingsFind(&externalBindings, type, slot);
    if (key == NULL)
	{
	fprintf(stderr, "Required render binding is missing: %s\n", slot);
	showError("Required render binding is missing from assets");
	exit(1);
	}
    return key;
    }

static GLuint requiredBoundModel(const char *slot)
    {
    return requiredModel(requiredBinding(ASSET_BINDING_MODEL, slot));
    }

static const struct AssetManifestFile *requiredBoundFile(const char *slot)
    {
    const char *key;
    const struct AssetManifestFile *file;

    key = requiredBinding(ASSET_BINDING_FILE, slot);
    file = assetManifestFindFile(&externalAssets, key);
    if (file == NULL)
	{
	fprintf(stderr, "Required bound file is missing: %s\n", slot);
	showError("Required file binding is missing from assets");
	exit(1);
	}
    return file;
    }

const char *assetFilePath(const char *slot)
    {
    return requiredBoundFile(slot)->path;
    }

GLuint assetModelDisplayList(const char *key)
    {
    const struct AssetManifestModel *model;

    model = assetManifestFindModel(&externalAssets, key);
    if (model == NULL)
        return 0;
    return model->displayList;
    }

int assetModelHasNode(const char *key, const char *nodeName)
    {
    const struct AssetManifestModel *model;

    if (nodeName == NULL || nodeName[0] == '\0')
        return 1;
    model = assetManifestFindModel(&externalAssets, key);
    return model != NULL && assetModelFindNode(&model->model, nodeName) >= 0;
    }

int assetAnimationAvailable(const char *key)
    {
    const struct AssetManifestModel *model;
    size_t clip;
    int loop;

    if (key == NULL || key[0] == '\0')
        return 1;
    return assetManifestFindAnimation(&externalAssets, key, &model, &clip,
                                      &loop);
    }

int assetEntityCatalogAvailable(const struct EntityCatalog *catalog,
                                char *error, size_t errorSize)
    {
    return entityCatalogValidateAssets(catalog, &externalAssets,
                                       error, errorSize);
    }

static const struct AssetValue *requiredEffectValue(const char *key,
                                                    size_t components)
    {
    const struct AssetValue *value;

    value = assetValuesFind(&externalEffectValues, key);
    if (value == NULL || value->componentCount != components)
        {
        fprintf(stderr, "Required effect value is missing or invalid: %s\n",
                key);
        showError("Required effect value is missing from assets");
        exit(1);
        }
    return value;
    }

static float effectScalar(const char *key)
    {
    return requiredEffectValue(key, 1)->components[0];
    }

const float *assetPaletteColor(const char *key)
    {
    const struct AssetValue *value;

    value = assetValuesFind(&externalPaletteValues, key);
    if (value == NULL || value->componentCount != 4)
        {
        fprintf(stderr, "Required palette colour is missing: %s\n", key);
        showError("Required palette colour is missing from assets");
        exit(1);
        }
    return value->components;
    }

void makeObjects(char * dataPath)
    {
    char assetError[256];
    char bindingsPath[MAXPATH];
    char valuesPath[MAXPATH];
    char fontPath[MAXPATH];
    char uiPath[MAXPATH];
    const struct AssetManifestFile *valuesFile;

    assetManifestInitialize(&externalAssets);
    assetBindingsInitialize(&externalBindings);
    assetValuesInitialize(&externalEffectValues);
    assetValuesInitialize(&externalPaletteValues);
    assetPoseInitialize(&externalEntityPose);
    if (!assetManifestLoad(dataPath, &externalAssets,
                           assetError, sizeof(assetError)))
        {
        fprintf(stderr, "Could not load external assets: %s\n", assetError);
        showError("Cannot load manifest.txt");
        exit(1);
        }

    valuesFile = assetManifestFindFile(&externalAssets, "render-bindings");
    if (valuesFile == NULL ||
        strlen(dataPath) + strlen(valuesFile->path) + 1 > sizeof(bindingsPath))
        {
        showError("Render binding asset path is missing or too long");
        exit(1);
        }
    strcpy(bindingsPath, dataPath);
    strcat(bindingsPath, valuesFile->path);
    if (!assetBindingsLoad(bindingsPath, &externalBindings,
                           assetError, sizeof(assetError)))
        {
        fprintf(stderr, "Could not load render bindings: %s\n", assetError);
        showError("Cannot load render bindings");
        exit(1);
        }

#define LOAD_VALUE_FILE(slot, values, description)                         \
    valuesFile = requiredBoundFile(slot);                                  \
    if (strlen(dataPath) + strlen(valuesFile->path) + 1 > sizeof(valuesPath)) \
        {                                                                  \
        showError(description " asset path is too long");               \
        exit(1);                                                           \
        }                                                                  \
    strcpy(valuesPath, dataPath);                                          \
    strcat(valuesPath, valuesFile->path);                                  \
    if (!assetValuesLoad(valuesPath, values, assetError, sizeof(assetError))) \
        {                                                                  \
        fprintf(stderr, "Could not load %s values: %s\n",               \
                description, assetError);                                  \
        showError("Cannot load renderer values");                        \
        exit(1);                                                           \
        }

    LOAD_VALUE_FILE("config-effects", &externalEffectValues, "effect");
    LOAD_VALUE_FILE("config-palette", &externalPaletteValues, "palette");
#undef LOAD_VALUE_FILE
    titleObj = requiredBoundModel("ui-title-high");
    titleObjOO = requiredBoundModel("ui-title-medium");
    titleObjOOO = requiredBoundModel("ui-title-low");
    lowDPlane = requiredBoundModel("terrain-ground-low");
    highDPlane = requiredBoundModel("terrain-ground-high");
    bigPlane = requiredBoundModel("terrain-ground-large");
    directionArrow = requiredBoundModel("ui-direction-arrow");

    valuesFile = requiredBoundFile("font-stroke");
    if (strlen(dataPath) + strlen(valuesFile->path) + 1 > sizeof(fontPath))
        {
        showError("Stroke-font asset path is too long");
        exit(1);
        }
    strcpy(fontPath, dataPath);
    strcat(fontPath, valuesFile->path);
    valuesFile = requiredBoundFile("ui-layout");
    if (strlen(dataPath) + strlen(valuesFile->path) + 1 > sizeof(uiPath))
        {
        showError("UI asset path is too long");
        exit(1);
        }
    strcpy(uiPath, dataPath);
    strcat(uiPath, valuesFile->path);
    initFonts(fontPath, uiPath);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw the procedural sky and projected runtime shadows         */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void drawSkyDisc(float x, float y, float radius,
                        const float color[4])
    {
    int segment;
    float angle;
    float vertices[26][3];

    vertices[0][0] = x;
    vertices[0][1] = y;
    vertices[0][2] = 0.0f;
    for (segment = 0; segment <= 24; segment++)
        {
        angle = (float)(segment * 2.0 * PI / 24.0);
        vertices[segment + 1][0] =
            x + (float)cos((double)angle) * radius;
        vertices[segment + 1][1] =
            y + (float)sin((double)angle) * radius;
        vertices[segment + 1][2] = 0.0f;
        }
    dynamicDrawTriangleFan3(vertices, 26, color);
    }

void drawEnvironmentSky(const struct WorldEnvironment *environment)
    {
    float bottomColors[4][3];
    float bottomVertices[4][3];
    float topColors[4][3];
    float topVertices[4][3];
    float starColor[4];
    float starVertices[72][3];
    float starX;
    float starY;
    float sunX;
    float sunY;
    float moonX;
    float moonY;
    int component;
    int star;
    unsigned long starHash;

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    bottomVertices[0][0] = -1.0f; bottomVertices[0][1] = -1.0f;
    bottomVertices[0][2] = 0.5f;
    bottomVertices[1][0] = 1.0f; bottomVertices[1][1] = -1.0f;
    bottomVertices[1][2] = 0.5f;
    bottomVertices[2][0] = 1.0f; bottomVertices[2][1] = 0.0f;
    bottomVertices[2][2] = 0.5f;
    bottomVertices[3][0] = -1.0f; bottomVertices[3][1] = 0.0f;
    bottomVertices[3][2] = 0.5f;
    topVertices[0][0] = -1.0f; topVertices[0][1] = 0.0f;
    topVertices[0][2] = 0.5f;
    topVertices[1][0] = 1.0f; topVertices[1][1] = 0.0f;
    topVertices[1][2] = 0.5f;
    topVertices[2][0] = 1.0f; topVertices[2][1] = 1.0f;
    topVertices[2][2] = 0.5f;
    topVertices[3][0] = -1.0f; topVertices[3][1] = 1.0f;
    topVertices[3][2] = 0.5f;
    for (component = 0; component < 3; component++)
        {
        bottomColors[0][component] = environment->skyHorizon[component] * 0.55f;
        bottomColors[1][component] = bottomColors[0][component];
        bottomColors[2][component] = environment->skyHorizon[component];
        bottomColors[3][component] = environment->skyHorizon[component];
        topColors[0][component] = environment->skyHorizon[component];
        topColors[1][component] = environment->skyHorizon[component];
        topColors[2][component] = environment->skyZenith[component];
        topColors[3][component] = environment->skyZenith[component];
        }
    dynamicDrawQuad3(bottomVertices, bottomColors);
    dynamicDrawQuad3(topVertices, topColors);

    starColor[0] = 0.78f;
    starColor[1] = 0.84f;
    starColor[2] = 1.0f;
    starColor[3] = environment->starVisibility;
    for (star = 0; star < 72; star++)
        {
        starHash = (unsigned long)(star + 1) * 2654435761UL;
        starX = -0.96f +
            (float)(starHash & 65535UL) * (1.92f / 65535.0f);
        starHash = starHash * 1664525UL + 1013904223UL;
        starY = 0.03f +
            (float)(starHash & 65535UL) * (0.94f / 65535.0f);
        starVertices[star][0] = starX;
        starVertices[star][1] = starY;
        starVertices[star][2] = 0.0f;
        }
    dynamicDrawPoints3(starVertices, 72, starColor, 1.8f);

    sunX = environment->sunDirection[0] * 0.72f;
    sunY = environment->sunDirection[1] * 0.72f;
    moonX = environment->moonDirection[0] * 0.72f;
    moonY = environment->moonDirection[1] * 0.72f;
    if (environment->sunVisibility > 0.001f)
        drawSkyDisc(sunX, sunY, 0.065f, environment->sunColor);
    if (environment->moonVisibility > 0.001f)
        drawSkyDisc(moonX, moonY, 0.052f, environment->moonColor);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();
    }

void drawEnvironmentShadow(float x, float z, float bottom,
                           float radius, float height,
                           const struct WorldEnvironment *environment)
    {
    float angle;
    float centerX;
    float centerZ;
    float color[4];
    float directionLength;
    float lightHeight;
    float majorRadius;
    float minorRadius;
    float offsetLength;
    float perpendicularX;
    float perpendicularZ;
    float shadowX;
    float shadowZ;
    float vertices[22][3];
    int segment;

    (void)bottom;
    if (!shadowsEnabled || environment->shadowStrength <= 0.001f ||
        radius <= 0.0f ||
        height <= 0.0f)
        return;
    lightHeight = environment->lightDirection[1];
    if (lightHeight < 0.16f)
        lightHeight = 0.16f;
    shadowX = -environment->lightDirection[0] / lightHeight;
    shadowZ = -environment->lightDirection[2] / lightHeight;
    directionLength = (float)sqrt((double)(shadowX * shadowX +
                                           shadowZ * shadowZ));
    if (directionLength < 0.0001f)
        {
        shadowX = 1.0f;
        shadowZ = 0.0f;
        directionLength = 1.0f;
        }
    shadowX /= directionLength;
    shadowZ /= directionLength;
    offsetLength = height * directionLength * 0.45f;
    if (offsetLength > height * 3.5f)
        offsetLength = height * 3.5f;
    centerX = x + shadowX * offsetLength * 0.5f;
    centerZ = z + shadowZ * offsetLength * 0.5f;
    majorRadius = radius + offsetLength * 0.55f;
    minorRadius = radius * 0.88f;
    perpendicularX = -shadowZ;
    perpendicularZ = shadowX;
    color[0] = 0.01f;
    color[1] = 0.015f;
    color[2] = 0.025f;
    color[3] = environment->shadowStrength;
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);
    glDepthMask(GL_FALSE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    vertices[0][0] = centerX;
    vertices[0][1] = PLANEY + 0.004f;
    vertices[0][2] = centerZ;
    for (segment = 0; segment <= 20; segment++)
        {
        angle = (float)(segment * 2.0 * PI / 20.0);
        vertices[segment + 1][0] = centerX +
            shadowX * (float)cos((double)angle) * majorRadius +
            perpendicularX * (float)sin((double)angle) * minorRadius;
        vertices[segment + 1][1] = PLANEY + 0.004f;
        vertices[segment + 1][2] = centerZ +
            shadowZ * (float)cos((double)angle) * majorRadius +
            perpendicularZ * (float)sin((double)angle) * minorRadius;
        }
    dynamicDrawTriangleFan3(vertices, 22, color);
    glPopAttrib();
    }

void drawWorldGround(float xshift, float zshift,
		     int detail, int itsChristmas, int view)
    {
    float theta, radius;
    float angle;

    /******************************/
    /* 'where is the town' arrow  */
    /******************************/

    if (itsChristmas)
	glColor3fv(assetPaletteColor("snow"));
    else
	glColor3fv(assetPaletteColor("plane-green"));


    if (view == MAPVIEW)
	glCallList(bigPlane);
    else if (detail <= 1)
	glCallList(lowDPlane);
    else
	glCallList(highDPlane);

    radius = sqrt(xshift * xshift + zshift * zshift);

    if (radius > 36)
	{
        /**********************/
        /* draw town pointer  */
        /**********************/

	if (zshift == 0)
	    zshift = NEARZERO;

	theta = atan(xshift / zshift);

	if (zshift > 0)
	    theta += PI;

	angle =  theta * RAD_TO_SMALL_DEG;

	glPushMatrix();
	    glRotatef(angle, 0, 1, 0);

	    glCallList(directionArrow);
	glPopMatrix();
	}
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw every catalog-backed map object                           */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void drawCatalogVisual(const struct EntityArchetype *archetype,
                              int counter, int detail, int seasonal,
                              float instanceScale);
static void drawCatalogVisualAlpha(const struct EntityArchetype *archetype,
                                   int counter, int detail, int seasonal,
                                   float instanceScale, float alpha);

void drawMapObjects(const struct runtimeMapObject *objects, int counter,
               int detail, int seasonal, int view,
               float viewerX, float viewerZ)
    {
    const struct runtimeMapObject *object;

    if (objects == NULL)
        return;
    for (object = objects->next; object != NULL; object = object->next)
        {
        if (object->dataArchetype == NULL || !object->intact)
            continue;
        if (view != MAPVIEW &&
            (fabs(object->x - viewerX) > PLANESIZE ||
             fabs(object->z - viewerZ) > PLANESIZE))
            continue;
        if (object->dataArchetype->castsShadow)
            drawEnvironmentShadow(
                object->x, object->z, object->y,
                entityArchetypeCollisionRadius(object->dataArchetype) *
                    object->instanceScale,
                object->height, &worldEnvironment);
        glPushMatrix();
        glTranslatef(object->x, object->y, object->z);
        glRotatef(object->heading, 0.0f, 1.0f, 0.0f);
        drawCatalogVisual(object->dataArchetype, counter, detail,
                          seasonal, object->instanceScale);
        glPopMatrix();
        }
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw all actors currently dying                                */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
static void applyExternalEntityAim(const struct AssetModel *model,
                                   const struct EntityArchetype *archetype,
                                   const struct runtimeActor *actor,
                                   struct AssetPose *pose)
    {
    int node;

    if (archetype->aimYawNode[0] != '\0')
        {
        node = assetModelFindNode(model, archetype->aimYawNode);
        if (node >= 0)
            assetPoseRotateNode(pose, (size_t)node, actor->aimYaw, 1);
        }
    if (archetype->aimPitchNode[0] != '\0')
        {
        node = assetModelFindNode(model, archetype->aimPitchNode);
        if (node >= 0)
            assetPoseRotateNode(pose, (size_t)node, actor->aimPitch, 0);
        }
    }

static void drawExternalEntityModel(
    const struct AssetManifestModel *model,
    const struct EntityArchetype *archetype,
    const struct AssetPose *pose)
    {
    size_t renderNode;

    if (archetype->renderNodeCount == 0)
        {
        assetModelDraw(&model->model, pose);
        return;
        }
    for (renderNode = 0; renderNode < archetype->renderNodeCount;
         renderNode++)
        assetModelDrawNode(
            &model->model,
            assetModelFindNode(&model->model,
                               archetype->renderNodes[renderNode]),
            pose);
    }

static const char *catalogVisualModelKey(
    const struct EntityArchetype *archetype, int detail, int seasonal)
    {
    if (seasonal && detail == 0 && archetype->seasonalLowModel[0] != '\0')
        return archetype->seasonalLowModel;
    if (seasonal && archetype->seasonalModel[0] != '\0')
        return archetype->seasonalModel;
    if (detail == 0 && archetype->lowModel[0] != '\0')
        return archetype->lowModel;
    return archetype->model;
    }

static const char *catalogVisualAnimationKey(
    const struct EntityArchetype *archetype, int detail, int seasonal)
    {
    if (seasonal && detail == 0 &&
        archetype->seasonalLowAnimation[0] != '\0')
        return archetype->seasonalLowAnimation;
    if (seasonal && archetype->seasonalAnimation[0] != '\0')
        return archetype->seasonalAnimation;
    if (detail == 0 && archetype->lowAnimation[0] != '\0')
        return archetype->lowAnimation;
    return archetype->idleAnimation;
    }

static void drawCatalogVisual(const struct EntityArchetype *archetype,
                              int counter, int detail, int seasonal,
                              float instanceScale)
    {
    drawCatalogVisualAlpha(archetype, counter, detail, seasonal,
                           instanceScale, 1.0f);
    }

static float particleNoise(int seed)
    {
    unsigned long value;

    value = (unsigned long)(seed + 1) * 1103515245UL + 12345UL;
    value = value * 1664525UL + 1013904223UL;
    return (float)(value & 65535UL) / 65535.0f;
    }

static void interpolateParticleColor(const float start[4],
                                     const float middle[4],
                                     const float end[4], float phase,
                                     float alpha, float color[4])
    {
    const float *from;
    const float *to;
    float localPhase;
    int component;

    if (phase < 0.5f)
        {
        from = start;
        to = middle;
        localPhase = phase * 2.0f;
        }
    else
        {
        from = middle;
        to = end;
        localPhase = (phase - 0.5f) * 2.0f;
        }
    for (component = 0; component < 3; component++)
        color[component] = from[component] +
            (to[component] - from[component]) * localPhase;
    color[3] = alpha;
    }

static void drawParticleDiamond(float x, float y, float z, float width,
                                float height, const float color[4])
    {
    float vertices[6][3];

    vertices[0][0] = x; vertices[0][1] = y; vertices[0][2] = z;
    vertices[1][0] = x; vertices[1][1] = y + height;
    vertices[1][2] = z;
    vertices[2][0] = x + width; vertices[2][1] = y;
    vertices[2][2] = z;
    vertices[3][0] = x; vertices[3][1] = y - height;
    vertices[3][2] = z;
    vertices[4][0] = x - width; vertices[4][1] = y;
    vertices[4][2] = z;
    memcpy(vertices[5], vertices[1], sizeof(vertices[5]));
    dynamicDrawTriangleFan3(vertices, 6, color);

    vertices[2][0] = x; vertices[2][2] = z + width;
    vertices[4][0] = x; vertices[4][2] = z - width;
    dynamicDrawTriangleFan3(vertices, 6, color);
    }

static void drawCatalogParticles(const struct EntityArchetype *archetype,
                                 int counter, float instanceScale,
                                 float visualAlpha)
    {
    const float *startColor;
    const float *middleColor;
    const float *endColor;
    float color[4];
    float phase;
    float angle;
    float radius;
    float size;
    float x;
    float y;
    float z;
    float directionY;
    float horizontal;
    float age;
    int cycle;
    int generation;
    int particle;
    int seed;

    startColor = assetPaletteColor(archetype->particleStartColor);
    middleColor = assetPaletteColor(archetype->particleMiddleColor);
    endColor = assetPaletteColor(archetype->particleEndColor);
    cycle = archetype->particleCycleFrames;
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDepthMask(GL_FALSE);
    for (particle = 0; particle < archetype->particleCount; particle++)
        {
        if (archetype->renderStyle == ENTITY_RENDER_PARTICLE_PLUME)
            {
            age = (float)((counter + particle * cycle /
                           archetype->particleCount) % cycle);
            phase = age / (float)cycle;
            generation = (counter + particle * cycle /
                          archetype->particleCount) / cycle;
            seed = particle * 97 + generation * 131;
            angle = particleNoise(seed) * 2.0f * PI;
            radius = archetype->particleSpread *
                (0.2f + phase * 0.8f) * particleNoise(seed + 17);
            x = (float)cos((double)angle) * radius;
            z = (float)sin((double)angle) * radius;
            y = age * archetype->particleSpeed;
            size = archetype->particleSize *
                (0.75f + particleNoise(seed + 29) * 0.5f) *
                (1.0f - phase * 0.35f);
            interpolateParticleColor(startColor, middleColor, endColor,
                                     phase,
                                     visualAlpha * (1.0f - phase * 0.72f),
                                     color);
            }
        else
            {
            phase = (float)counter / (float)cycle;
            if (phase > 1.0f)
                phase = 1.0f;
            seed = particle * 193 + 41;
            directionY = -0.18f + particleNoise(seed + 7) * 1.18f;
            horizontal = (float)sqrt((double)(1.0f -
                directionY * directionY));
            angle = particleNoise(seed + 19) * 2.0f * PI;
            radius = (float)counter * archetype->particleSpeed *
                (0.55f + particleNoise(seed + 31) * 0.7f);
            x = (float)cos((double)angle) * horizontal * radius;
            z = (float)sin((double)angle) * horizontal * radius;
            y = directionY * radius - archetype->particleGravity *
                (float)(counter * counter);
            size = archetype->particleSize *
                (0.65f + particleNoise(seed + 43) * 0.7f) *
                (1.0f - phase * 0.55f);
            interpolateParticleColor(startColor, middleColor, endColor,
                                     phase,
                                     visualAlpha * (1.0f - phase), color);
            }
        if (archetype->renderStyle == ENTITY_RENDER_PARTICLE_PLUME)
            drawParticleDiamond(x, y, z, size * instanceScale,
                                size * instanceScale * (1.25f + phase),
                                color);
        else
            drawParticleDiamond(x, y, z, size * instanceScale,
                                size * instanceScale, color);
        }
    if (archetype->renderStyle == ENTITY_RENDER_PARTICLE_BURST &&
        counter < cycle / 3)
        {
        phase = (float)counter / ((float)cycle / 3.0f);
        size = archetype->particleSize * instanceScale *
            (2.8f - phase * 1.8f);
        interpolateParticleColor(startColor, middleColor, endColor,
                                 phase * 0.5f,
                                 visualAlpha * (1.0f - phase), color);
        drawParticleDiamond(0.0f, 0.0f, 0.0f, size, size, color);
        }
    glPopAttrib();
    }

static void drawCatalogVisualAlpha(const struct EntityArchetype *archetype,
                                   int counter, int detail, int seasonal,
                                   float instanceScale, float alpha)
    {
    const struct AssetManifestModel *model;
    const struct AssetManifestTexture *texture;
    const char *animationKey;
    const char *modelKey;
    const struct AssetPose *pose;
    size_t clip;
    size_t nodeIndex;
    int loop;
    int node;
    float color[4];

    if (archetype == NULL)
        return;
    if (archetype->renderStyle != ENTITY_RENDER_MODEL)
        {
        drawCatalogParticles(archetype, counter, instanceScale, alpha);
        return;
        }
    modelKey = catalogVisualModelKey(archetype, detail, seasonal);
    animationKey = catalogVisualAnimationKey(archetype, detail, seasonal);
    model = assetManifestFindModel(&externalAssets, modelKey);
    pose = NULL;
    if (animationKey[0] != '\0' &&
        assetManifestFindAnimation(&externalAssets, animationKey,
                                   &model, &clip, &loop) &&
        assetPoseSample(&model->model, clip,
                        (float)counter * archetype->animationSpeed,
                        loop, &externalEntityPose))
        pose = &externalEntityPose;
    glPushMatrix();
    glScalef(archetype->scale * instanceScale,
             archetype->scale * instanceScale,
             archetype->scale * instanceScale);
    memcpy(color, assetPaletteColor(archetype->renderColor), sizeof(color));
    color[3] *= alpha;
    glColor4fv(color);
    if (model == NULL)
        {
        glPopMatrix();
        return;
        }
    texture = archetype->texture[0] != '\0' ?
        assetManifestFindTexture(&externalAssets, archetype->texture) : NULL;
    if (archetype->renderNodeCount == 0)
        assetModelDraw(&model->model, pose);
    else
        for (nodeIndex = 0; nodeIndex < archetype->renderNodeCount;
             nodeIndex++)
            {
            node = assetModelFindNode(
                &model->model, archetype->renderNodes[nodeIndex]);
            if (node < 0)
                continue;
            if (texture != NULL &&
                strcmp(archetype->renderNodes[nodeIndex],
                       archetype->textureNode) == 0)
                {
                glCallList(texture->displayList);
                glEnable(GL_TEXTURE_2D);
                assetModelDrawNode(&model->model, node, pose);
                glDisable(GL_TEXTURE_2D);
                }
            else
                assetModelDrawNode(&model->model, node, pose);
            }
    glPopMatrix();
    }

static int externalEntityModelPose(
    const struct runtimeActor *actor, int counter,
    const struct AssetManifestModel **selectedModel,
    const struct AssetPose **selectedPose)
    {
    const struct EntityArchetype *archetype;
    const struct AssetManifestModel *model;
    const char *animation;
    size_t clip;
    int loop;

    archetype = actor->dataArchetype;
    if (archetype == NULL)
        return 0;
    animation = archetype->idleAnimation;
    if (actor->dataAgent.reaction == ENTITY_REACTION_ATTACK &&
        archetype->attackAnimation[0] != '\0')
        animation = archetype->attackAnimation;
    else if (actor->walkingBackwards &&
             archetype->backwardAnimation[0] != '\0')
        animation = archetype->backwardAnimation;
    else if (actor->walking && archetype->moveAnimation[0] != '\0')
        animation = archetype->moveAnimation;
    if (animation[0] != '\0' &&
        assetManifestFindAnimation(&externalAssets, animation, &model,
                                   &clip, &loop) &&
        assetPoseSample(&model->model, clip,
                        (float)counter * archetype->animationSpeed,
                        loop, &externalEntityPose))
        {
        applyExternalEntityAim(&model->model, archetype, actor,
                               &externalEntityPose);
        *selectedModel = model;
        *selectedPose = &externalEntityPose;
        return 1;
        }
    model = assetManifestFindModel(&externalAssets, archetype->model);
    if (model == NULL || !assetPoseReset(&model->model, &externalEntityPose))
        return 0;
    applyExternalEntityAim(&model->model, archetype, actor,
                           &externalEntityPose);
    *selectedModel = model;
    *selectedPose = &externalEntityPose;
    return 1;
    }

int configuredEntityMuzzlePose(const struct EntityArchetype *archetype,
                               int moving, int attacking, int counter,
                               float aimYaw, float aimPitch,
                               float position[3], float direction[3])
    {
    struct runtimeActor entity;
    const struct AssetManifestModel *model;
    const struct AssetPose *pose;
    float matrix[16];
    float radians;
    float sine;
    float cosine;
    float length;
    float modelX;
    float modelZ;
    int node;

    if (archetype == NULL || position == NULL || direction == NULL ||
        archetype->muzzleNode[0] == '\0')
        return 0;
    memset(&entity, 0, sizeof(entity));
    entity.dataArchetype = archetype;
    entity.dataAgent.reaction = attacking ? ENTITY_REACTION_ATTACK :
                                           ENTITY_REACTION_OBSERVE;
    entity.walking = moving;
    entity.aimYaw = aimYaw;
    entity.aimPitch = aimPitch;
    if (!externalEntityModelPose(&entity, counter, &model, &pose))
        return 0;
    node = assetModelFindNode(&model->model, archetype->muzzleNode);
    if (node < 0 || !assetModelNodeWorldMatrix(&model->model, node, pose,
                                               matrix))
        return 0;

    radians = archetype->modelYawOffset * DEG_TO_RAD;
    sine = (float)sin((double)radians);
    cosine = (float)cos((double)radians);
    modelX = matrix[12];
    modelZ = matrix[14];
    position[0] = archetype->scale *
        (cosine * modelX + sine * modelZ);
    position[1] = archetype->scale * matrix[13];
    position[2] = archetype->scale *
        (-sine * modelX + cosine * modelZ);

    modelX = matrix[8];
    modelZ = matrix[10];
    direction[0] = cosine * modelX + sine * modelZ;
    direction[1] = matrix[9];
    direction[2] = -sine * modelX + cosine * modelZ;
    length = (float)sqrt((double)(direction[0] * direction[0] +
                                  direction[1] * direction[1] +
                                  direction[2] * direction[2]));
    if (length <= 0.000001f)
        return 0;
    direction[0] /= length;
    direction[1] /= length;
    direction[2] /= length;
    return 1;
    }

static void drawExternalEntity(const struct runtimeActor *actor, int counter)
    {
    const struct EntityArchetype *archetype;
    const struct AssetManifestModel *model;
    const struct AssetPose *pose;

    archetype = actor->dataArchetype;
    if (archetype == NULL)
        return;
    glRotatef(archetype->modelYawOffset, 0, 1, 0);
    if (externalEntityModelPose(actor, counter, &model, &pose))
        drawExternalEntityModel(model, archetype, pose);
    }

static void mergeEntityBounds(float minimum[3], float maximum[3],
                              const float nodeMinimum[3],
                              const float nodeMaximum[3], int *valid)
    {
    int component;

    if (!*valid)
        {
        memcpy(minimum, nodeMinimum, 3 * sizeof(*minimum));
        memcpy(maximum, nodeMaximum, 3 * sizeof(*maximum));
        *valid = 1;
        return;
        }
    for (component = 0; component < 3; component++)
        {
        if (nodeMinimum[component] < minimum[component])
            minimum[component] = nodeMinimum[component];
        if (nodeMaximum[component] > maximum[component])
            maximum[component] = nodeMaximum[component];
        }
    }

int assetModelUnscaledBounds(const struct EntityArchetype *archetype,
                             float minimum[3], float maximum[3])
    {
    const struct AssetManifestModel *model;
    float nodeMinimum[3];
    float nodeMaximum[3];
    size_t renderNode;
    int node;
    int valid;

    if (archetype == NULL || minimum == NULL || maximum == NULL ||
        archetype->renderStyle != ENTITY_RENDER_MODEL)
        return 0;
    model = assetManifestFindModel(&externalAssets, archetype->model);
    if (model == NULL)
        return 0;
    valid = 0;
    if (archetype->renderNodeCount == 0)
        valid = assetModelPoseBounds(&model->model, NULL, minimum, maximum);
    else
        for (renderNode = 0; renderNode < archetype->renderNodeCount;
             renderNode++)
            {
            node = assetModelFindNode(&model->model,
                                      archetype->renderNodes[renderNode]);
            if (node >= 0 && assetModelNodePoseBounds(
                    &model->model, node, NULL, nodeMinimum, nodeMaximum))
                mergeEntityBounds(minimum, maximum, nodeMinimum, nodeMaximum,
                                  &valid);
            }
    if (!valid)
        return 0;
    return 1;
    }

int assetModelUnscaledVerticalBounds(
    const struct EntityArchetype *archetype, float *minimumY,
    float *maximumY)
    {
    float minimum[3];
    float maximum[3];

    if (minimumY == NULL || maximumY == NULL ||
        !assetModelUnscaledBounds(archetype, minimum, maximum))
        return 0;
    *minimumY = minimum[1];
    *maximumY = maximum[1];
    return 1;
    }

int assetModelUnscaledHeight(const struct EntityArchetype *archetype,
                             float *height)
    {
    float minimumY;
    float maximumY;

    if (height == NULL ||
        !assetModelUnscaledVerticalBounds(archetype, &minimumY, &maximumY))
        return 0;
    if (maximumY <= minimumY)
        return 0;
    *height = maximumY - minimumY;
    return 1;
    }

static void transformEntityBounds(const struct EntityArchetype *archetype,
                                  const float sourceMinimum[3],
                                  const float sourceMaximum[3],
                                  float minimum[3], float maximum[3])
    {
    float radians;
    float sine;
    float cosine;
    float source[3];
    float transformed[3];
    int corner;
    int component;

    radians = archetype->modelYawOffset * DEG_TO_RAD;
    sine = (float)sin((double)radians);
    cosine = (float)cos((double)radians);
    for (corner = 0; corner < 8; corner++)
        {
        for (component = 0; component < 3; component++)
            source[component] = corner & (1 << component) ?
                sourceMaximum[component] : sourceMinimum[component];
        transformed[0] = archetype->scale *
            (cosine * source[0] + sine * source[2]);
        transformed[1] = archetype->scale * source[1];
        transformed[2] = archetype->scale *
            (-sine * source[0] + cosine * source[2]);
        if (corner == 0)
            {
            memcpy(minimum, transformed, 3 * sizeof(*minimum));
            memcpy(maximum, transformed, 3 * sizeof(*maximum));
            }
        else
            for (component = 0; component < 3; component++)
                {
                if (transformed[component] < minimum[component])
                    minimum[component] = transformed[component];
                if (transformed[component] > maximum[component])
                    maximum[component] = transformed[component];
                }
        }
    }

int configuredPlayerEntityBounds(const struct EntityArchetype *archetype,
                                 int moving, int backwards, int attacking,
                                 int counter,
                                 float aimYaw, float aimPitch,
                                 float minimum[3], float maximum[3])
    {
    struct runtimeActor entity;
    const struct AssetManifestModel *model;
    const struct AssetPose *pose;
    float modelMinimum[3];
    float modelMaximum[3];
    float nodeMinimum[3];
    float nodeMaximum[3];
    size_t renderNode;
    int node;
    int valid;

    if (archetype == NULL || minimum == NULL || maximum == NULL)
        return 0;
    memset(&entity, 0, sizeof(entity));
    entity.dataArchetype = archetype;
    entity.dataAgent.reaction = attacking ? ENTITY_REACTION_ATTACK :
                                           ENTITY_REACTION_OBSERVE;
    entity.walking = moving;
    entity.walkingBackwards = backwards;
    entity.aimYaw = aimYaw;
    entity.aimPitch = aimPitch;
    if (!externalEntityModelPose(&entity, counter, &model, &pose))
        return 0;
    valid = 0;
    if (archetype->renderNodeCount == 0)
        valid = assetModelPoseBounds(&model->model, pose,
                                     modelMinimum, modelMaximum);
    else
        for (renderNode = 0; renderNode < archetype->renderNodeCount;
             renderNode++)
            {
            node = assetModelFindNode(&model->model,
                                      archetype->renderNodes[renderNode]);
            if (assetModelNodePoseBounds(&model->model, node, pose,
                                         nodeMinimum, nodeMaximum))
                mergeEntityBounds(modelMinimum, modelMaximum,
                                  nodeMinimum, nodeMaximum, &valid);
            }
    if (!valid)
        return 0;
    transformEntityBounds(archetype, modelMinimum, modelMaximum,
                          minimum, maximum);
    return 1;
    }

void drawConfiguredPlayerEntity(const struct EntityArchetype *archetype,
                                int moving, int backwards, int attacking,
                                int counter, float aimYaw, float aimPitch)
    {
    struct runtimeActor entity;

    if (archetype == NULL)
        return;
    memset(&entity, 0, sizeof(entity));
    entity.dataArchetype = archetype;
    entity.dataAgent.reaction = attacking ? ENTITY_REACTION_ATTACK :
                                           ENTITY_REACTION_OBSERVE;
    entity.walking = moving;
    entity.walkingBackwards = backwards;
    entity.aimYaw = aimYaw;
    entity.aimPitch = aimPitch;
    glScalef(archetype->scale, archetype->scale, archetype->scale);
    drawExternalEntity(&entity, counter);
    }

static void drawDebrisFragment(const struct runtimeActor *fragment,
                               int detail, int seasonal)
    {
    float size;

    if (fragment->visualArchetype == NULL)
        return;
    size = fragment->debrisSize;
    if (fragment->visualArchetype->castsShadow)
        drawEnvironmentShadow(fragment->x, fragment->z, fragment->y,
                              size * 0.7f, size * 1.4f,
                              &worldEnvironment);
    glTranslatef(fragment->x, fragment->y, fragment->z);
    if (fragment->debrisGrounded)
        {
        glRotatef(11.0f + fragment->subtype * 13.0f, 1, 0, 1);
        glRotatef(fragment->subtype * 47.0f, 0, 1, 0);
        }
    else
        {
        glRotatef(fragment->count * (13.0f + fragment->subtype * 2.0f),
                  1, 0, 1);
        glRotatef(fragment->count * (19.0f + fragment->subtype),
                  0, 1, 0);
        }
    drawCatalogVisual(fragment->visualArchetype, fragment->count,
                      detail, seasonal, size);
    }

void drawRuntimeDebris(struct runtimeActor *allWrecks,
                       float viewerX, float viewerZ,
                       int detail, int seasonal)
    {
    struct runtimeActor *fragment;

    for (fragment = allWrecks->next; fragment != NULL;
         fragment = fragment->next)
        if (fabs(fragment->x - viewerX) <= PLANESIZE &&
            fabs(fragment->z - viewerZ) <= PLANESIZE)
            {
            glPushMatrix();
            drawDebrisFragment(fragment, detail, seasonal);
            glPopMatrix();
            }
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw a beam segment using only archetype and renderer settings */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void drawRuntimeBeamSegment(
    const struct EntityArchetype *archetype, int detail,
    const float origin[3], const float endpoint[3])
    {
    const struct AssetValue *lineCounts;
    const float *coreColor;
    const float *outerColor;
    const float *traceColor;
    float jitter;
    float jittered[3];
    float traceOrigin[3];
    float traceEndpoint[3];
    int lineCount;
    int line;
    GLfloat lineWidthNow;
    GLint shadeNow;

    if (archetype == NULL || origin == NULL || endpoint == NULL)
        return;
    glGetFloatv(GL_LINE_WIDTH, &lineWidthNow);
    glGetIntegerv(GL_SHADE_MODEL, &shadeNow);
    lineCounts = requiredEffectValue("beam-line-counts", 3);
    jitter = effectScalar("beam-jitter");
    coreColor = assetPaletteColor(archetype->weaponCoreColor);
    outerColor = assetPaletteColor(archetype->weaponColor);
    traceColor = archetype->weaponTraceColor[0] != '\0' ?
        assetPaletteColor(archetype->weaponTraceColor) : NULL;
    if (detail <= 0)
        lineCount = (int)lineCounts->components[0];
    else if (detail == 1)
        lineCount = (int)lineCounts->components[1];
    else
        lineCount = (int)lineCounts->components[2];

    glLineWidth(effectScalar("beam-core-line-width"));
    dynamicDrawLine3(origin, endpoint, coreColor, coreColor);
    if (detail >= 0)
        glEnable(GL_BLEND);
    if (detail > 0)
        glShadeModel(GL_SMOOTH);
    glLineWidth(effectScalar("beam-outer-line-width"));
    for (line = 0; line < lineCount; line++)
        {
        jittered[0] = endpoint[0] + randy(jitter);
        jittered[1] = endpoint[1] + randy(jitter);
        jittered[2] = endpoint[2] + randy(jitter);
        dynamicDrawLine3(origin, jittered, outerColor, outerColor);
        }
    if (detail >= 0)
        glDisable(GL_BLEND);
    if (detail > 0 && traceColor != NULL)
        {
        traceOrigin[0] = origin[0];
        traceOrigin[1] = GROUND_OVERLAY_Y;
        traceOrigin[2] = origin[2];
        traceEndpoint[0] = endpoint[0];
        traceEndpoint[1] = GROUND_OVERLAY_Y;
        traceEndpoint[2] = endpoint[2];
        glLineWidth(effectScalar("beam-trace-line-width"));
        dynamicDrawLine3(traceOrigin, traceEndpoint,
                         traceColor, traceColor);
        }

    glShadeModel(shadeNow);
    glLineWidth(lineWidthNow);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw all actors on the battlefield                             */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawRuntimeActors(struct runtimeActor *allActors, int counter,
                       struct runtimePlayer viewer,
                       int detail, int view)

    {
    struct runtimeActor * tempactor;
    float angle2;
    float muzzleLocal[3];
    float muzzleDirection[3];
    float muzzleWorld[3];
    float beamTarget[3];
    float muzzleRadians;
    float muzzleSine;
    float muzzleCosine;

    for(tempactor = allActors->next;tempactor != NULL;tempactor = tempactor->next)
	{
	if (tempactor->dataArchetype != NULL &&
	    (((fabs(tempactor->x - viewer.x) <= PLANESIZE) &&
	      (fabs(tempactor->z - viewer.z) <= PLANESIZE)) ||
	     view == MAPVIEW))
	    {
	    angle2 = tempactor->theta * RAD_TO_SMALL_DEG;
	    if (tempactor->dataArchetype->castsShadow)
	drawEnvironmentShadow(
		    tempactor->x, tempactor->z, tempactor->y,
		    entityArchetypeCollisionRadius(tempactor->dataArchetype),
		    tempactor->dataArchetype->collisionHeight,
		    &worldEnvironment);
	    glPushMatrix();
	    glTranslatef(tempactor->x, tempactor->y, tempactor->z);
	    glRotatef(angle2, 0.0f, 1.0f, 0.0f);
	    glScalef(tempactor->dataArchetype->scale,
	             tempactor->dataArchetype->scale,
	             tempactor->dataArchetype->scale);
	    drawExternalEntity(tempactor, counter);
	    glPopMatrix();
	    if (tempactor->weaponActiveFrames > 0 &&
		tempactor->dataArchetype->weapon == ENTITY_WEAPON_BEAM &&
		tempactor->dataAgent.hasTarget)
		{
		if (configuredEntityMuzzlePose(
			tempactor->dataArchetype, tempactor->walking, 1, counter,
			tempactor->aimYaw, tempactor->aimPitch,
			muzzleLocal, muzzleDirection))
		    {
		    muzzleRadians = angle2 * DEG_TO_RAD;
		    muzzleSine = (float)sin((double)muzzleRadians);
		    muzzleCosine = (float)cos((double)muzzleRadians);
		    muzzleWorld[0] = tempactor->x +
			muzzleCosine * muzzleLocal[0] +
			muzzleSine * muzzleLocal[2];
		    muzzleWorld[1] = tempactor->y + muzzleLocal[1];
		    muzzleWorld[2] = tempactor->z -
			muzzleSine * muzzleLocal[0] +
			muzzleCosine * muzzleLocal[2];
		    beamTarget[0] = tempactor->dataAgent.targetX;
		    beamTarget[1] = tempactor->dataAgent.targetY;
		    beamTarget[2] = tempactor->dataAgent.targetZ;
		    drawRuntimeBeamSegment(tempactor->dataArchetype, detail,
		                           muzzleWorld, beamTarget);
		    }
		}
	    }
	}
    }


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw timed catalog effect instances                            */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawRuntimeExplosions(struct runtimeExplosion * explosions, int detail)
    {
    float alpha;
    struct runtimeExplosion  * explosion;

    if (detail >= 2)
	glEnable(GL_BLEND);

    for(explosion = explosions->next;explosion != NULL;explosion = explosion->next)
	{
	alpha = 1.0f;
	if (detail >= 2 && explosion->archetype->effectFadeRate > 0.0f)
	    alpha = (explosion->archetype->effectFadeStart - explosion->count) *
	            explosion->archetype->effectFadeRate;
	if (alpha < 0.0f) alpha = 0.0f;
	if (alpha > 1.0f) alpha = 1.0f;

	glPushMatrix();
	    glTranslatef(explosion->x, explosion->y, explosion->z);
	    drawCatalogVisualAlpha(explosion->archetype, explosion->count,
	                           detail, 0, 1.0f, alpha);
	glPopMatrix();
	}

    if (detail >= 2)
	glDisable(GL_BLEND);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw a directed runtime beam                                  */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawRuntimeBeam(const struct EntityArchetype *archetype, int detail,
	const float muzzleOrigin[3], const float muzzleDirection[3])
    {
    float endpoint[3];

    if (archetype == NULL || muzzleOrigin == NULL ||
		muzzleDirection == NULL)
	return;
    worldClippedRayEndpoint(muzzleOrigin, muzzleDirection,
                            archetype->weaponRange, PLANEY, endpoint);
    drawRuntimeBeamSegment(archetype, detail, muzzleOrigin, endpoint);
    }


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw projectiles in flight                                    */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawRuntimeProjectiles(struct runtimeProjectile *allProjs, int counter,
		    int itsChristmas, int detail,
		    const struct runtimePlayer *viewer, int currentView)
    {
    struct runtimeProjectile *temp;
    float currentTrail[3];
    float previousTrail[3];
    float x, y, z;
    int hasPreviousTrail;
    int visualIndex;
    float scarDistance;

    GLfloat lineWidthNow;
    glGetFloatv(GL_LINE_WIDTH, &lineWidthNow);

    hasPreviousTrail = 0;
    visualIndex = 0;
    glLineWidth(effectScalar("trail-line-width"));
    scarDistance = effectScalar("trail-link-distance");
    for (temp = allProjs->next;temp != NULL;temp = temp->next)
	if ((viewer == NULL ||
	     (fabs(temp->x - viewer->x) <= PLANESIZE &&
	      fabs(temp->z - viewer->z) <= PLANESIZE)) &&
	    !(currentView == FIRSTPERSONVIEW && viewer != NULL &&
	      ((temp->owner == viewer && temp->count <
	        temp->archetype->projectileArmingFrames) ||
	       (fabs(temp->x - viewer->x) <= viewer->width &&
	        fabs(temp->z - viewer->z) <= viewer->width &&
	        temp->y >= viewer->bottom && temp->y <= viewer->height))))
	    {
	    x = temp->x;
	    y = temp->y;
	    z = temp->z;

	    if (temp->archetype->projectileTrailMarker)
		{
		currentTrail[0] = x;
		currentTrail[1] = GROUND_OVERLAY_Y;
		currentTrail[2] = z;
		if (hasPreviousTrail &&
		    fabs(currentTrail[0] - previousTrail[0]) < scarDistance &&
		    fabs(currentTrail[2] - previousTrail[2]) < scarDistance)
		    dynamicDrawLine3(currentTrail, previousTrail,
		                     assetPaletteColor(
		                         temp->archetype->renderColor),
		                     assetPaletteColor(
		                         temp->archetype->renderColor));
		memcpy(previousTrail, currentTrail, sizeof(previousTrail));
		hasPreviousTrail = 1;
		}
	    else
		{
		glPushMatrix();
		glTranslatef(x, y, z);
		drawCatalogVisual(temp->archetype, counter + visualIndex * 7, detail,
		                  itsChristmas, 1.0f);
		glPopMatrix();
		}
	    visualIndex++;
	    }

    glLineWidth(lineWidthNow);
    }
