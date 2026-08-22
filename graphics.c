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
#include "dynamic_primitives.h"

    /*************/
    /* colours   */
    /*************/

    float colorolive[4];
    float planeGreen[4];
    float planeGreen2[4];
    float planeGreen3[4];
    float colorgreen[4];
    float colorblack[4];
    float colorred[4];
    float colorred_50[4];
    float colorred2[4];
    float colorred3[4];
    float colormaserblue[4];
    float colormaserblue2[4];
    float colorwhite[4];
    float colorwhite2[4];
    float colorsnow[4];
    float colorsnow2[4];
    float colorgrey1[4];
    float colorgrey1_50[4];
    float colorgrey2[4];
    float colorgrey3[4];
    float colorgrey3_50[4];
    float colorgrey4[4];
    float colorbrown[4];
    float colorbeige[4];
    float colorblue[4];
    float colorblue2[4];
    float colorwater[4];
    float coloryellow[4];
    float colormaseryellow[4];
    float colororange[4];
    float healthGreen[4];
    float treeColor[4];
    float treeColor2[4];
    
    /*************/
    /* objects   */
    /*************/

    GLuint 
	plaintank, slagtank, masertank, slagmasertank,  
	launchertank, slaglaunchertank, helo, slagHelo, 
	bigRotor, smallRotor,

	fighterBody,
	
	watershellObj,

	tankShadow, planeShadow, fighterShadow, 

	planeDotsObj, planeDotsObjD0,
    
	titleObj, titleObjOO, titleObjOOO, 
	lowDPlane, highDPlane, bigPlane, 
		
	strip1Obj, strip2Obj, strip3Obj, 
	strip1ObjD0, strip2ObjD0, strip3ObjD0, 

	building1Obj, building2Obj, building7Obj, building9Obj,  building9ObjOO,  
	building10Obj, building12Obj, building14Obj, building19Obj,  
	building20Obj, building21Obj, building31Obj, building32Obj,  
	building5Obj,
	building1ObjD0, building2ObjD0, building5ObjD0, building12ObjD0,
	building19ObjD0, building20ObjD0, building21ObjD0;

    static GLuint building0Models[2][2];
    static GLuint building4Models[3];
    static GLuint building6Low, building8Models[2], building11Models[2];
    static GLuint building22Obj, building23Obj, building24Obj, building25Obj;
    static GLuint building28Low, building29Models[2], building30Models[2];
    static GLuint building34Obj, building36Obj;
    static GLuint waterModels[2][2];
    static GLuint hillCenterModels[2];
    static GLuint hillSideModels[3][2];
    static GLuint hillCornerModels[2][2];
    static GLuint hillInsideModels[2][2];
    static GLuint effectSphereLow, effectSphereHigh, fireModel;
    static GLuint shadowRectangle, directionArrow;

    /***************************/
    /* texture mapping stuff   */
    /***************************/

    GLuint leftScreen, rightScreen;
    GLuint treeWood, roadtex, officeLogo;

    static struct AssetManifest externalAssets;
    static struct AssetBindings externalBindings;
    static struct AssetValues externalAnimationValues;
    static struct AssetValues externalEffectValues;
    static struct AssetValues externalPaletteValues;
    static const struct AssetManifestModel *externalHeloModels[2];
    static size_t externalHeloClips[2];
    static int externalHeloLoops[2];
    static struct AssetPose externalHeloPoses[2];
    static const struct AssetManifestModel *externalProjectileModels[4];
    static size_t externalProjectileClips[4];
    static int externalProjectileLoops[4];
    static struct AssetPose externalProjectilePoses[4];
    static const struct AssetManifestModel *externalSpecialProjectileModels[2];
    static size_t externalSpecialProjectileClips[2];
    static int externalSpecialProjectileLoops[2];
    static struct AssetPose externalSpecialProjectilePoses[2];
    static const struct AssetManifestModel *externalAirplaneModel;
    static size_t externalAirplaneClip;
    static int externalAirplaneLoop;
    static struct AssetPose externalAirplanePose;
    static const struct AssetManifestModel *externalGoogelonModel;
    static size_t externalGoogelonClip;
    static int externalGoogelonLoop;
    static struct AssetPose externalGoogelonPose;
    static const struct AssetManifestModel *externalFlutterModel;
    static size_t externalFlutterClip;
    static int externalFlutterLoop;
    static struct AssetPose externalFlutterPose;
    static const struct AssetManifestModel *externalTechsModel;
    static size_t externalTechsDeathClip;
    static int externalTechsDeathLoop;
    static struct AssetPose externalTechsPose;
    static const struct AssetManifestModel *externalHeroModels[2];
    static size_t externalHeroWalkClips[2];
    static size_t externalHeroFireClips[2];
    static int externalHeroWalkLoops[2];
    static int externalHeroFireLoops[2];
    static struct AssetPose externalHeroPoses[2];
    static const struct AssetManifestModel *externalMechaModel;
    static size_t externalMechaClip;
    static int externalMechaLoop;
    static struct AssetPose externalMechaPose;
    static const struct AssetManifestModel *externalBoomModels[2];
    static size_t externalBoomClips[2];
    static int externalBoomLoops[2];
    static struct AssetPose externalBoomPoses[2];
    static const struct AssetManifestModel *externalHunterModel;
    static struct AssetPose externalHunterPose;
    static const struct AssetManifestModel *externalPineTreeModel;
    static struct AssetPose externalPineTreePose;
    static const struct AssetManifestModel *externalRoundTreeModel;
    static struct AssetPose externalRoundTreePose;
    static const struct AssetManifestModel *externalRoadModels[4];
    static const struct AssetManifestModel *externalBuilding26Model;
    static struct AssetPose externalBuilding26Pose;
    static const struct AssetManifestModel *externalBuilding37Model;
    static struct AssetPose externalBuilding37Pose;
    static const struct AssetManifestModel *externalBuilding6Model;
    static size_t externalBuilding6Clip;
    static int externalBuilding6Loop;
    static struct AssetPose externalBuilding6Pose;
    static const struct AssetManifestModel *externalBuilding28Model;
    static size_t externalBuilding28Clip;
    static int externalBuilding28Loop;
    static struct AssetPose externalBuilding28Pose;
    static const struct AssetManifestModel *externalBuilding33Model;
    static size_t externalBuilding33Clip;
    static int externalBuilding33Loop;
    static struct AssetPose externalBuilding33Pose;
    static const struct AssetManifestModel *externalPowerTowerModel;
    static size_t externalPowerTowerClip;
    static int externalPowerTowerLoop;
    static struct AssetPose externalPowerTowerPose;
    static struct AssetPose externalEntityPose;
    

    struct road roadsOnPlane[MAXROADSONPLANE];

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

static GLuint requiredTexture(const char *key)
    {
    const struct AssetManifestTexture *texture;

    texture = assetManifestFindTexture(&externalAssets, key);
    if (texture == NULL || texture->displayList == 0)
	{
	fprintf(stderr, "Required external texture is missing: %s\n", key);
	showError("Required texture is missing from manifest.txt");
	exit(1);
	}
    return texture->displayList;
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

static const struct AssetManifestModel *requiredBoundModelEntry(
    const char *slot)
    {
    return requiredModelEntry(requiredBinding(ASSET_BINDING_MODEL, slot));
    }

static GLuint requiredBoundModel(const char *slot)
    {
    return requiredModel(requiredBinding(ASSET_BINDING_MODEL, slot));
    }

static GLuint requiredBoundTexture(const char *slot)
    {
    return requiredTexture(requiredBinding(ASSET_BINDING_TEXTURE, slot));
    }

static int requiredBoundAnimation(const char *slot,
                                  const struct AssetManifestModel **model,
                                  size_t *clip, int *loop)
    {
    const char *key;

    key = requiredBinding(ASSET_BINDING_ANIMATION, slot);
    return assetManifestFindAnimation(&externalAssets, key, model, clip, loop);
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

static int requiredNode(const struct AssetManifestModel *entry,
                        const char *name)
    {
    int node;

    node = assetModelFindNode(&entry->model, name);
    if (node < 0)
        {
        fprintf(stderr, "Required node is missing from model %s: %s\n",
                entry->key, name);
        showError("Required model node is missing");
        exit(1);
        }
    return node;
    }

static const struct AssetValue *requiredAnimationValue(const char *key,
                                                       size_t components)
    {
    const struct AssetValue *value;

    value = assetValuesFind(&externalAnimationValues, key);
    if (value == NULL || value->componentCount != components)
        {
        fprintf(stderr, "Required animation value is missing or invalid: %s\n",
                key);
        showError("Required animation value is missing from assets");
        exit(1);
        }
    return value;
    }

static float animationScalar(const char *key)
    {
    return requiredAnimationValue(key, 1)->components[0];
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

static void copyPaletteColor(const char *key, float color[4])
    {
    const struct AssetValue *value;

    value = assetValuesFind(&externalPaletteValues, key);
    if (value == NULL || value->componentCount != 4)
        {
        fprintf(stderr, "Required palette colour is missing: %s\n", key);
        showError("Required palette colour is missing from assets");
        exit(1);
        }
    memcpy(color, value->components, 4 * sizeof(float));
    }

static const float *paletteColor(const char *key)
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

static void applyExternalPalette(void)
    {
    copyPaletteColor("olive", colorolive);
    copyPaletteColor("plane-green", planeGreen);
    copyPaletteColor("plane-green-2", planeGreen2);
    copyPaletteColor("plane-green-3", planeGreen3);
    copyPaletteColor("green", colorgreen);
    copyPaletteColor("black", colorblack);
    copyPaletteColor("red", colorred);
    copyPaletteColor("red-50", colorred_50);
    copyPaletteColor("red-2", colorred2);
    copyPaletteColor("red-3", colorred3);
    copyPaletteColor("maser-blue", colormaserblue);
    copyPaletteColor("maser-blue-2", colormaserblue2);
    copyPaletteColor("white", colorwhite);
    copyPaletteColor("white-2", colorwhite2);
    copyPaletteColor("snow", colorsnow);
    copyPaletteColor("snow-2", colorsnow2);
    copyPaletteColor("grey-1", colorgrey1);
    copyPaletteColor("grey-1-50", colorgrey1_50);
    copyPaletteColor("grey-2", colorgrey2);
    copyPaletteColor("grey-3", colorgrey3);
    copyPaletteColor("grey-3-50", colorgrey3_50);
    copyPaletteColor("grey-4", colorgrey4);
    copyPaletteColor("brown", colorbrown);
    copyPaletteColor("beige", colorbeige);
    copyPaletteColor("blue", colorblue);
    copyPaletteColor("blue-2", colorblue2);
    copyPaletteColor("water", colorwater);
    copyPaletteColor("yellow", coloryellow);
    copyPaletteColor("maser-yellow", colormaseryellow);
    copyPaletteColor("orange", colororange);
    copyPaletteColor("health-green", healthGreen);
    copyPaletteColor("tree", treeColor);
    copyPaletteColor("tree-2", treeColor2);
    }

static int hasPrefix(const char *text, const char *prefix)
    {
    return strncmp(text, prefix, strlen(prefix)) == 0;
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
    assetValuesInitialize(&externalAnimationValues);
    assetValuesInitialize(&externalEffectValues);
    assetValuesInitialize(&externalPaletteValues);
    assetPoseInitialize(&externalHeloPoses[0]);
    assetPoseInitialize(&externalHeloPoses[1]);
    assetPoseInitialize(&externalProjectilePoses[0]);
    assetPoseInitialize(&externalProjectilePoses[1]);
    assetPoseInitialize(&externalProjectilePoses[2]);
    assetPoseInitialize(&externalProjectilePoses[3]);
    assetPoseInitialize(&externalSpecialProjectilePoses[0]);
    assetPoseInitialize(&externalSpecialProjectilePoses[1]);
    assetPoseInitialize(&externalAirplanePose);
    assetPoseInitialize(&externalGoogelonPose);
    assetPoseInitialize(&externalFlutterPose);
    assetPoseInitialize(&externalTechsPose);
    assetPoseInitialize(&externalHeroPoses[0]);
    assetPoseInitialize(&externalHeroPoses[1]);
    assetPoseInitialize(&externalMechaPose);
    assetPoseInitialize(&externalBoomPoses[0]);
    assetPoseInitialize(&externalBoomPoses[1]);
    assetPoseInitialize(&externalHunterPose);
    assetPoseInitialize(&externalPineTreePose);
    assetPoseInitialize(&externalRoundTreePose);
    assetPoseInitialize(&externalBuilding26Pose);
    assetPoseInitialize(&externalBuilding37Pose);
    assetPoseInitialize(&externalBuilding6Pose);
    assetPoseInitialize(&externalBuilding28Pose);
    assetPoseInitialize(&externalBuilding33Pose);
    assetPoseInitialize(&externalPowerTowerPose);
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

    valuesFile = requiredBoundFile("config-animation");
    if (valuesFile == NULL ||
        strlen(dataPath) + strlen(valuesFile->path) + 1 > sizeof(valuesPath))
        {
        showError("Animation value asset path is missing or too long");
        exit(1);
        }
    strcpy(valuesPath, dataPath);
    strcat(valuesPath, valuesFile->path);
    if (!assetValuesLoad(valuesPath, &externalAnimationValues,
                         assetError, sizeof(assetError)))
        {
        fprintf(stderr, "Could not load animation values: %s\n", assetError);
        showError("Cannot load animation values");
        exit(1);
        }

    valuesFile = requiredBoundFile("config-effects");
    if (valuesFile == NULL ||
        strlen(dataPath) + strlen(valuesFile->path) + 1 > sizeof(valuesPath))
        {
        showError("Effect value asset path is missing or too long");
        exit(1);
        }
    strcpy(valuesPath, dataPath);
    strcat(valuesPath, valuesFile->path);
    if (!assetValuesLoad(valuesPath, &externalEffectValues,
                         assetError, sizeof(assetError)))
        {
        fprintf(stderr, "Could not load effect values: %s\n", assetError);
        showError("Cannot load effect values");
        exit(1);
        }

    valuesFile = requiredBoundFile("config-palette");
    if (valuesFile == NULL ||
        strlen(dataPath) + strlen(valuesFile->path) + 1 > sizeof(valuesPath))
        {
        showError("Palette asset path is missing or too long");
        exit(1);
        }
    strcpy(valuesPath, dataPath);
    strcat(valuesPath, valuesFile->path);
    if (!assetValuesLoad(valuesPath, &externalPaletteValues,
                         assetError, sizeof(assetError)))
        {
        fprintf(stderr, "Could not load palette values: %s\n", assetError);
        showError("Cannot load palette values");
        exit(1);
        }
    applyExternalPalette();

    leftScreen = requiredBoundTexture("display-left");
    rightScreen = requiredBoundTexture("display-right");
    treeWood = requiredBoundTexture("tree-bark");
    roadtex = requiredBoundTexture("road-surface");
    officeLogo = requiredBoundTexture("office-sign");

#define LOAD_MODEL(variable, slot) variable = requiredBoundModel(slot)
    LOAD_MODEL(plaintank, "vehicle-tank-standard");
    LOAD_MODEL(slagtank, "vehicle-tank-standard-destroyed");
    LOAD_MODEL(masertank, "vehicle-tank-maser");
    LOAD_MODEL(slagmasertank, "vehicle-tank-maser-destroyed");
    LOAD_MODEL(launchertank, "vehicle-tank-launcher");
    LOAD_MODEL(slaglaunchertank, "vehicle-tank-launcher-destroyed");
    LOAD_MODEL(fighterBody, "vehicle-fighter-body");
    LOAD_MODEL(watershellObj, "projectile-water");
    LOAD_MODEL(tankShadow, "shadow-tank");
    LOAD_MODEL(planeShadow, "shadow-plane");
    LOAD_MODEL(fighterShadow, "shadow-fighter");
    LOAD_MODEL(titleObj, "ui-title-high");
    LOAD_MODEL(titleObjOO, "ui-title-medium");
    LOAD_MODEL(titleObjOOO, "ui-title-low");
    LOAD_MODEL(planeDotsObj, "vehicle-plane-dots-high");
    LOAD_MODEL(planeDotsObjD0, "vehicle-plane-dots-low");
    LOAD_MODEL(strip1Obj, "building-strip-1-high");
    LOAD_MODEL(strip1ObjD0, "building-strip-1-low");
    LOAD_MODEL(strip2Obj, "building-strip-2-high");
    LOAD_MODEL(strip2ObjD0, "building-strip-2-low");
    LOAD_MODEL(strip3Obj, "building-strip-3-high");
    LOAD_MODEL(strip3ObjD0, "building-strip-3-low");
    LOAD_MODEL(building1Obj, "building-type-1-high");
    LOAD_MODEL(building1ObjD0, "building-type-1-low");
    LOAD_MODEL(building2Obj, "building-type-2-high");
    LOAD_MODEL(building2ObjD0, "building-type-2-low");
    LOAD_MODEL(building5Obj, "building-type-5-high");
    LOAD_MODEL(building5ObjD0, "building-type-5-low");
    LOAD_MODEL(building7Obj, "building-type-7");
    LOAD_MODEL(building9Obj, "building-type-9");
    LOAD_MODEL(building9ObjOO, "building-type-9-alternate");
    LOAD_MODEL(building10Obj, "building-type-10");
    LOAD_MODEL(building12Obj, "building-type-12-high");
    LOAD_MODEL(building12ObjD0, "building-type-12-low");
    LOAD_MODEL(building14Obj, "building-type-14");
    LOAD_MODEL(building19Obj, "building-type-19-high");
    LOAD_MODEL(building19ObjD0, "building-type-19-low");
    LOAD_MODEL(building20Obj, "building-type-20-high");
    LOAD_MODEL(building20ObjD0, "building-type-20-low");
    LOAD_MODEL(building21Obj, "building-type-21-high");
    LOAD_MODEL(building21ObjD0, "building-type-21-low");
    LOAD_MODEL(building31Obj, "building-type-31");
    LOAD_MODEL(building32Obj, "building-type-32");
    LOAD_MODEL(lowDPlane, "terrain-ground-low");
    LOAD_MODEL(highDPlane, "terrain-ground-high");
    LOAD_MODEL(bigPlane, "terrain-ground-large");
    LOAD_MODEL(building0Models[0][0], "building-type-0-white-low");
    LOAD_MODEL(building0Models[0][1], "building-type-0-white-high");
    LOAD_MODEL(building0Models[1][0], "building-type-0-grey-low");
    LOAD_MODEL(building0Models[1][1], "building-type-0-grey-high");
    LOAD_MODEL(building4Models[0], "building-cooling-vector");
    LOAD_MODEL(building4Models[1], "building-cooling-low");
    LOAD_MODEL(building4Models[2], "building-cooling-high");
    LOAD_MODEL(building6Low, "building-fast-food-low");
    LOAD_MODEL(building8Models[0], "building-hangar-low");
    LOAD_MODEL(building8Models[1], "building-hangar-high");
    LOAD_MODEL(building11Models[0], "building-smokestacks-low");
    LOAD_MODEL(building11Models[1], "building-smokestacks-high");
    LOAD_MODEL(building22Obj, "building-office-tower-dark");
    LOAD_MODEL(building23Obj, "building-office-tower-light");
    LOAD_MODEL(building24Obj, "building-office-tower-white");
    LOAD_MODEL(building25Obj, "building-satellite-dish");
    LOAD_MODEL(building28Low, "building-fast-food-alternate-low");
    LOAD_MODEL(building29Models[0], "building-fountain");
    LOAD_MODEL(building29Models[1], "building-fountain-xmas");
    LOAD_MODEL(building30Models[0], "building-hangar-alternate-low");
    LOAD_MODEL(building30Models[1], "building-hangar-alternate-high");
    LOAD_MODEL(building34Obj, "building-office-smallest");
    LOAD_MODEL(building36Obj, "building-office-small");
    LOAD_MODEL(waterModels[0][0], "terrain-water-square");
    LOAD_MODEL(waterModels[0][1], "terrain-water-square-xmas");
    LOAD_MODEL(waterModels[1][0], "terrain-water-triangle");
    LOAD_MODEL(waterModels[1][1], "terrain-water-triangle-xmas");
    LOAD_MODEL(hillCenterModels[0], "terrain-hill-center");
    LOAD_MODEL(hillCenterModels[1], "terrain-hill-center-xmas");
    LOAD_MODEL(hillSideModels[0][0], "terrain-hill-side-0");
    LOAD_MODEL(hillSideModels[0][1], "terrain-hill-side-0-xmas");
    LOAD_MODEL(hillSideModels[1][0], "terrain-hill-side-1");
    LOAD_MODEL(hillSideModels[1][1], "terrain-hill-side-1-xmas");
    LOAD_MODEL(hillSideModels[2][0], "terrain-hill-side-2");
    LOAD_MODEL(hillSideModels[2][1], "terrain-hill-side-2-xmas");
    LOAD_MODEL(hillCornerModels[0][0], "terrain-hill-corner-0");
    LOAD_MODEL(hillCornerModels[0][1], "terrain-hill-corner-0-xmas");
    LOAD_MODEL(hillCornerModels[1][0], "terrain-hill-corner-1");
    LOAD_MODEL(hillCornerModels[1][1], "terrain-hill-corner-1-xmas");
    LOAD_MODEL(hillInsideModels[0][0], "terrain-hill-inside-0");
    LOAD_MODEL(hillInsideModels[0][1], "terrain-hill-inside-0-xmas");
    LOAD_MODEL(hillInsideModels[1][0], "terrain-hill-inside-1");
    LOAD_MODEL(hillInsideModels[1][1], "terrain-hill-inside-1-xmas");
    LOAD_MODEL(effectSphereLow, "effect-sphere-low");
    LOAD_MODEL(effectSphereHigh, "effect-sphere-high");
    LOAD_MODEL(fireModel, "effect-fire");
    LOAD_MODEL(shadowRectangle, "effect-shadow-rectangle");
    LOAD_MODEL(directionArrow, "ui-direction-arrow");
#undef LOAD_MODEL

    if (!requiredBoundAnimation(
	"vehicle-helicopter", &externalHeloModels[0],
	&externalHeloClips[0], &externalHeloLoops[0]) ||
	!requiredBoundAnimation(
	"vehicle-helicopter-destroyed", &externalHeloModels[1],
	&externalHeloClips[1], &externalHeloLoops[1]))
	{
	showError("Required helo-rotors animation is missing from external assets");
	exit(1);
	}

    if (!requiredBoundAnimation(
        "vehicle-airplane", &externalAirplaneModel,
        &externalAirplaneClip, &externalAirplaneLoop) ||
        !requiredBoundAnimation(
        "monster-googelon", &externalGoogelonModel,
        &externalGoogelonClip, &externalGoogelonLoop) ||
        !requiredBoundAnimation(
        "monster-flutter", &externalFlutterModel,
        &externalFlutterClip, &externalFlutterLoop))
        {
        showError("Required compound model animation is missing");
        exit(1);
        }
    if (!requiredBoundAnimation(
            "monster-techs-death", &externalTechsModel,
            &externalTechsDeathClip, &externalTechsDeathLoop) ||
        !requiredBoundAnimation(
            "effect-boom-high", &externalBoomModels[0],
            &externalBoomClips[0], &externalBoomLoops[0]) ||
        !requiredBoundAnimation(
            "effect-boom-low", &externalBoomModels[1],
            &externalBoomClips[1], &externalBoomLoops[1]))
        {
        showError("Required death animation is missing");
        exit(1);
        }
    externalHunterModel = requiredBoundModelEntry("vehicle-hunter");
    externalPineTreeModel = requiredBoundModelEntry("tree-pine");
    externalRoundTreeModel = requiredBoundModelEntry("tree-round");
    externalRoadModels[0] = requiredBoundModelEntry("road-type-0");
    externalRoadModels[1] = requiredBoundModelEntry("road-type-1");
    externalRoadModels[2] = requiredBoundModelEntry("road-type-2");
    externalRoadModels[3] = requiredBoundModelEntry("road-type-3");
    externalBuilding26Model = requiredBoundModelEntry("building-drive-in-screen");
    externalBuilding37Model = requiredBoundModelEntry("building-office-logo");

    if (!requiredBoundAnimation(
        "monster-hero-typhoon-walk", &externalHeroModels[0],
        &externalHeroWalkClips[0], &externalHeroWalkLoops[0]) ||
        !requiredBoundAnimation(
        "monster-hero-typhoon-fire", &externalHeroModels[0],
        &externalHeroFireClips[0], &externalHeroFireLoops[0]) ||
        !requiredBoundAnimation(
        "monster-hero-beam-walk", &externalHeroModels[1],
        &externalHeroWalkClips[1], &externalHeroWalkLoops[1]) ||
        !requiredBoundAnimation(
        "monster-hero-beam-fire", &externalHeroModels[1],
        &externalHeroFireClips[1], &externalHeroFireLoops[1]) ||
        !requiredBoundAnimation(
        "monster-mecha", &externalMechaModel,
        &externalMechaClip, &externalMechaLoop))
        {
        showError("Required hero animation is missing");
        exit(1);
        }

    requiredNode(externalAirplaneModel, "body");
    requiredNode(externalGoogelonModel, "head");
    requiredNode(externalGoogelonModel, "head-simple");
    requiredNode(externalGoogelonModel, "xmas-head");
    requiredNode(externalGoogelonModel, "xmas-tail");
    requiredNode(externalGoogelonModel, "shadow-body");
    requiredNode(externalGoogelonModel, "shadow-leg");
    requiredNode(externalGoogelonModel, "shadow-tail");
    requiredNode(externalFlutterModel, "wing-left");
    requiredNode(externalFlutterModel, "wing-right");
    requiredNode(externalTechsModel, "head");
    requiredNode(externalTechsModel, "head-simple");
    requiredNode(externalTechsModel, "hand-right");
    requiredNode(externalTechsModel, "hand-left");
    requiredNode(externalHunterModel, "damage-00");
    requiredNode(externalPineTreeModel, "pine-trunk");
    requiredNode(externalPineTreeModel, "pine-crown");
    requiredNode(externalPineTreeModel, "pine-crown-vector");
    requiredNode(externalPineTreeModel, "pine-lights");
    requiredNode(externalPineTreeModel, "pine-shadow");
    requiredNode(externalRoundTreeModel, "round-trunk");
    requiredNode(externalRoundTreeModel, "round-crown");
    requiredNode(externalRoundTreeModel, "round-shadow");
    requiredNode(externalRoadModels[0], "surface");
    requiredNode(externalRoadModels[0], "marker");
    requiredNode(externalRoadModels[1], "surface");
    requiredNode(externalRoadModels[1], "marker");
    requiredNode(externalRoadModels[2], "surface");
    requiredNode(externalRoadModels[2], "marker");
    requiredNode(externalRoadModels[3], "surface");
    requiredNode(externalBuilding26Model, "body");
    requiredNode(externalBuilding26Model, "screen");
    requiredNode(externalBuilding37Model, "body");
    requiredNode(externalBuilding37Model, "logo");

    if (!requiredBoundAnimation(
        "building-fast-food-sign", &externalBuilding6Model,
        &externalBuilding6Clip, &externalBuilding6Loop) ||
        !requiredBoundAnimation(
        "building-fast-food-alternate-sign", &externalBuilding28Model,
        &externalBuilding28Clip, &externalBuilding28Loop) ||
        !requiredBoundAnimation(
        "building-hero-entrance", &externalBuilding33Model,
        &externalBuilding33Clip, &externalBuilding33Loop) ||
        !requiredBoundAnimation(
        "building-power-tower", &externalPowerTowerModel,
        &externalPowerTowerClip, &externalPowerTowerLoop))
        {
        showError("Required structure animation is missing");
        exit(1);
        }

    if (!requiredBoundAnimation(
	"projectile-standard-low",
	&externalProjectileModels[0], &externalProjectileClips[0],
	&externalProjectileLoops[0]) ||
	!requiredBoundAnimation(
	"projectile-standard-high",
	&externalProjectileModels[1], &externalProjectileClips[1],
	&externalProjectileLoops[1]) ||
	!requiredBoundAnimation(
	"projectile-standard-low-xmas",
	&externalProjectileModels[2], &externalProjectileClips[2],
	&externalProjectileLoops[2]) ||
	!requiredBoundAnimation(
	"projectile-standard-high-xmas",
	&externalProjectileModels[3], &externalProjectileClips[3],
	&externalProjectileLoops[3]) ||
	!requiredBoundAnimation(
	"projectile-techs",
	&externalSpecialProjectileModels[0],
	&externalSpecialProjectileClips[0],
	&externalSpecialProjectileLoops[0]) ||
	!requiredBoundAnimation(
	"projectile-hero",
	&externalSpecialProjectileModels[1],
	&externalSpecialProjectileClips[1],
	&externalSpecialProjectileLoops[1]))
	{
	showError("Required projectile animation is missing from external assets");
	exit(1);
	}
    valuesFile = requiredBoundFile("font-stroke");
    if (valuesFile == NULL ||
        strlen(dataPath) + strlen(valuesFile->path) + 1 > sizeof(fontPath))
        {
        showError("Stroke-font asset path is missing or too long");
        exit(1);
        }
    strcpy(fontPath, dataPath);
    strcat(fontPath, valuesFile->path);
    valuesFile = requiredBoundFile("ui-layout");
    if (valuesFile == NULL ||
        strlen(dataPath) + strlen(valuesFile->path) + 1 > sizeof(uiPath))
        {
        showError("UI asset path is missing or too long");
        exit(1);
        }
    strcpy(uiPath, dataPath);
    strcat(uiPath, valuesFile->path);
    initFonts(fontPath, uiPath);
    return;
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw a rectangular shadow on the ground                       */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void shadow (float x, float z, float wx, float wz)
    {
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glScalef(wx, 1.0f, wz);
    glCallList(shadowRectangle);
    glPopMatrix();
    }

static void drawEffectSphere(const float sphere[4], GLuint model)
    {
    glPushMatrix();
    glTranslatef(sphere[0], sphere[1], sphere[2]);
    glScalef(sphere[3], sphere[3], sphere[3]);
    glCallList(model);
    glPopMatrix();
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw a  hunter                                                */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void makeCHH(int dam)
    {
    char node[16];
    int variant;

    variant = dam <= 0 ? 0 : (dam + 3) / 4;
    if (variant > 5)
        variant = 5;
    if (!assetPoseReset(&externalHunterModel->model, &externalHunterPose))
        return;
    sprintf(node, "damage-%02d", variant);
    assetModelDrawNode(&externalHunterModel->model,
                       requiredNode(externalHunterModel, node),
                       &externalHunterPose);
    }


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw a helicopter                                             */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawHelo(float counter,  int slag)
    {
    GLint shadeNow;
    int variant;
    
    glGetIntegerv(GL_SHADE_MODEL, &shadeNow);

    variant = slag ? 1 : 0;
    if (assetPoseSample(&externalHeloModels[variant]->model,
			externalHeloClips[variant],
			counter * animationScalar("helo-rotor-time-scale"),
			externalHeloLoops[variant],
			&externalHeloPoses[variant]))
	{
	glShadeModel(GL_FLAT);
	assetModelDraw(&externalHeloModels[variant]->model,
		       &externalHeloPoses[variant]);
	glShadeModel(shadeNow);
	return;
	}
	}

    
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw a bomber                                                 */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawAirplane(int counter)
    {
    if (assetPoseSample(&externalAirplaneModel->model,
                        externalAirplaneClip,
                        counter *
                            animationScalar("airplane-propeller-time-scale"),
                        externalAirplaneLoop, &externalAirplanePose))
        assetModelDraw(&externalAirplaneModel->model, &externalAirplanePose);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw a fighter                                                 */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawFighter(void)
    {
    glCallList(fighterBody);    
    }
	


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw Googelon                                                 */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawMonster(struct monsterInfo monster, int counter,
		int itsChristmas, int detail)
    {
    const struct AssetValue *leftLeg;
    const struct AssetValue *rightLeg;

    if (!assetPoseSample(&externalGoogelonModel->model,
                         externalGoogelonClip, (float)counter,
                         externalGoogelonLoop, &externalGoogelonPose))
        return;

    leftLeg = requiredAnimationValue("googelon-leg-left", 3);
    rightLeg = requiredAnimationValue("googelon-leg-right", 3);

    glPushMatrix();

	glRotatef(animationScalar("googelon-heading-scale") *
	          monster.headHorzRotate, 0, 1, 0);

	/********/
	/* head */
	/********/

	glPushMatrix();
	    glRotatef(animationScalar("googelon-head-pitch-scale") *
	              monster.headVertRotate, 1, 0, 0);

	    assetModelDrawNode(&externalGoogelonModel->model,
	                       requiredNode(externalGoogelonModel, "head"),
	                       &externalGoogelonPose);
	    	    
	    if (itsChristmas)
		assetModelDrawNode(&externalGoogelonModel->model,
		                   requiredNode(externalGoogelonModel,
		                                "xmas-head"),
		                   &externalGoogelonPose);
	    
	glPopMatrix();   

	/********/
	/* body */
	/********/

	assetModelDrawNode(&externalGoogelonModel->model,
	                   requiredNode(externalGoogelonModel, "body"),
	                   &externalGoogelonPose);
    
	/***********************/
	/* legs, feet and toes */
	/***********************/

	glPushMatrix();
	    glTranslatef(leftLeg->components[0],
	                 leftLeg->components[1] * monster.rot1,
	                 leftLeg->components[2] * monster.rot2);
	    assetModelDrawNode(&externalGoogelonModel->model,
	                       requiredNode(externalGoogelonModel, "leg-left"),
	                       &externalGoogelonPose);
	glPopMatrix();
	glPushMatrix();
	    glTranslatef(rightLeg->components[0],
	                 rightLeg->components[1] * monster.rot1,
	                 rightLeg->components[2] * monster.rot2);
	    assetModelDrawNode(&externalGoogelonModel->model,
	                       requiredNode(externalGoogelonModel, "leg-right"),
	                       &externalGoogelonPose);
	glPopMatrix();

	if ((!monster.monsterIsDead) && (detail > 0))
	      {
	      assetModelDrawNode(&externalGoogelonModel->model,
	                         requiredNode(externalGoogelonModel,
	                                      "shadow-body"),
	                         &externalGoogelonPose);
	      glPushMatrix();
	      glTranslatef(leftLeg->components[0], 0.0f,
	                   leftLeg->components[2] * monster.rot2);
	      assetModelDrawNode(&externalGoogelonModel->model,
	                         requiredNode(externalGoogelonModel,
	                                      "shadow-leg"),
	                         &externalGoogelonPose);
	      glTranslatef(rightLeg->components[0] - leftLeg->components[0],
	                   0.0f,
	                   (rightLeg->components[2] -
	                    leftLeg->components[2]) * monster.rot2);
	      assetModelDrawNode(&externalGoogelonModel->model,
	                         requiredNode(externalGoogelonModel,
	                                      "shadow-leg"),
	                         &externalGoogelonPose);
	      glPopMatrix();
	      }

	/********/
	/* tail */
	/********/
    
	    assetModelDrawNode(&externalGoogelonModel->model,
	                       requiredNode(externalGoogelonModel, "tail"),
	                       &externalGoogelonPose);

	    if (itsChristmas)
		assetModelDrawNode(&externalGoogelonModel->model,
		                   requiredNode(externalGoogelonModel,
		                                "xmas-tail"),
		                   &externalGoogelonPose);

	    if ((!monster.monsterIsDead) && (detail > 0))
		{
		assetModelDrawNode(&externalGoogelonModel->model,
		                   requiredNode(externalGoogelonModel,
		                                "shadow-tail"),
		                   &externalGoogelonPose);
		}

    glPopMatrix();
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw SIMPLIFIED Googelon (to save time in monsterview)        */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawSimpleMonster(struct monsterInfo monster, int counter,
		int itsChristmas, int detail)
    {
    const struct AssetValue *leftLeg;
    const struct AssetValue *rightLeg;

    if (!assetPoseSample(&externalGoogelonModel->model,
                         externalGoogelonClip, (float)counter,
                         externalGoogelonLoop, &externalGoogelonPose))
        return;

    leftLeg = requiredAnimationValue("googelon-leg-left", 3);
    rightLeg = requiredAnimationValue("googelon-leg-right", 3);

    /*********************/
    /* googelon's shadow */
    /*********************/

    glPushMatrix();

	glRotatef(animationScalar("googelon-heading-scale") *
	          monster.headHorzRotate, 0, 1, 0);

	/********/
	/* head */
	/********/

	glPushMatrix();
	    glRotatef(animationScalar("googelon-head-pitch-scale") *
	              monster.headVertRotate, 1, 0, 0);

	    assetModelDrawNode(&externalGoogelonModel->model,
	                       requiredNode(externalGoogelonModel, "head-simple"),
	                       &externalGoogelonPose);
	    	    
	    if (itsChristmas)
		assetModelDrawNode(&externalGoogelonModel->model,
		                   requiredNode(externalGoogelonModel,
		                                "xmas-head"),
		                   &externalGoogelonPose);
	    
	glPopMatrix();   

	/********/
	/* body */
	/********/

	assetModelDrawNode(&externalGoogelonModel->model,
	                   requiredNode(externalGoogelonModel, "body"),
	                   &externalGoogelonPose);
    
	/***********************/
	/* legs, feet and toes */
	/***********************/

	glPushMatrix();
	    glTranslatef(leftLeg->components[0],
	                 leftLeg->components[1] * monster.rot1,
	                 leftLeg->components[2] * monster.rot2);

	    assetModelDrawNode(&externalGoogelonModel->model,
	                       requiredNode(externalGoogelonModel, "leg-left"),
	                       &externalGoogelonPose);

	glPopMatrix();
	glPushMatrix();
	    glTranslatef(rightLeg->components[0],
	                 rightLeg->components[1] * monster.rot1,
	                 rightLeg->components[2] * monster.rot2);
	    assetModelDrawNode(&externalGoogelonModel->model,
	                       requiredNode(externalGoogelonModel, "leg-right"),
	                       &externalGoogelonPose);

	glPopMatrix();

	if ((!monster.monsterIsDead) && (detail > 0))
	      {
	      assetModelDrawNode(&externalGoogelonModel->model,
	                         requiredNode(externalGoogelonModel,
	                                      "shadow-body"),
	                         &externalGoogelonPose);
	      glPushMatrix();
	      glTranslatef(leftLeg->components[0], 0.0f,
	                   leftLeg->components[2] * monster.rot2);
	      assetModelDrawNode(&externalGoogelonModel->model,
	                         requiredNode(externalGoogelonModel,
	                                      "shadow-leg"),
	                         &externalGoogelonPose);
	      glTranslatef(rightLeg->components[0] - leftLeg->components[0],
	                   0.0f,
	                   (rightLeg->components[2] -
	                    leftLeg->components[2]) * monster.rot2);
	      assetModelDrawNode(&externalGoogelonModel->model,
	                         requiredNode(externalGoogelonModel,
	                                      "shadow-leg"),
	                         &externalGoogelonPose);
	      glPopMatrix();
	      }

	    /********/
	    /* tail */
	    /********/
    
	    assetModelDrawNode(&externalGoogelonModel->model,
	                       requiredNode(externalGoogelonModel, "tail-simple"),
	                       &externalGoogelonPose);

	    if (itsChristmas)
		assetModelDrawNode(&externalGoogelonModel->model,
		                   requiredNode(externalGoogelonModel,
		                                "xmas-tail"),
		                   &externalGoogelonPose);


    glPopMatrix();
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw Flutter                                                  */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawFlutter(struct monsterInfo monster, int counter,
		int itsChristmas, float offsetX, int thaView, int detail)
    {
    if (!assetPoseSample(&externalFlutterModel->model,
                         externalFlutterClip, (float)counter,
                         externalFlutterLoop, &externalFlutterPose))
        return;

    glPushMatrix();
	glRotatef(animationScalar("flutter-heading-scale") *
	          monster.headHorzRotate, 0, 1, 0);

	/* shadow */

	if (!monster.monsterIsDead)
	    {
	    glPushAttrib(GL_DEPTH_BUFFER_BIT);
	    glPushAttrib(GL_COLOR_BUFFER_BIT);
	    
	    if (thaView == MONSTERVIEW)
		{
		if (detail > 0)	
		    glEnable(GL_BLEND);		    
		glDisable(GL_DEPTH_TEST);
		}
    
	    assetModelDrawNode(&externalFlutterModel->model,
	                       requiredNode(externalFlutterModel, "shadow"),
	                       &externalFlutterPose);
		
	    glPopAttrib();
	    glPopAttrib();
	    }

   
 
	glPushMatrix();
	    glRotatef(offsetX * animationScalar("flutter-bank-scale"), 0, 0, 1);

	    if (itsChristmas)
		assetModelDrawNode(&externalFlutterModel->model,
	                           requiredNode(externalFlutterModel, "xmas"),
	                           &externalFlutterPose);
		
	    glPushMatrix();
		glRotatef(monster.headVertRotate *
		          animationScalar("flutter-head-pitch-scale"), 1, 0, 0);

		assetModelDrawNode(&externalFlutterModel->model,
	                           requiredNode(externalFlutterModel, "head"),
	                           &externalFlutterPose);
	    glPopMatrix();

	    assetModelDrawNode(&externalFlutterModel->model,
	                       requiredNode(externalFlutterModel, "body"),
	                       &externalFlutterPose);

	    glPushMatrix();
		assetModelDrawNode(&externalFlutterModel->model,
	                           requiredNode(externalFlutterModel,
	                                        "wing-left"),
	                           &externalFlutterPose);
	    glPopMatrix();
	    
	    glPushMatrix();
		assetModelDrawNode(&externalFlutterModel->model,
	                           requiredNode(externalFlutterModel,
	                                        "wing-right"),
	                           &externalFlutterPose);
	    glPopMatrix();

	    
	glPopMatrix();
	
    glPopMatrix();

    } 



/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw Techs                                                    */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawTechs(struct monsterInfo monster, int detail)
    {
    float deathStart;

    if (!assetPoseReset(&externalTechsModel->model, &externalTechsPose))
        return;
    deathStart = animationScalar("techs-death-start");
    glPushMatrix();

    glRotatef(animationScalar("techs-heading-scale") *
              monster.headHorzRotate, 0, 1, 0);
 
    if (monster.deadCount < deathStart)
	{    
	glPushMatrix();
	    glRotatef(animationScalar("techs-head-pitch-scale") *
	              monster.headVertRotate, 1, 0, 0);
	    assetModelDrawNode(&externalTechsModel->model,
	                       requiredNode(externalTechsModel, "head"),
	                       &externalTechsPose);
	glPopMatrix();   

	assetModelDrawNode(&externalTechsModel->model,
	                   requiredNode(externalTechsModel, "body"),
	                   &externalTechsPose);
	}
    
    if ((!monster.monsterIsDead) && (detail > 0))
	{
	assetModelDrawNode(&externalTechsModel->model,
	                   requiredNode(externalTechsModel, "shadow"),
	                   &externalTechsPose);
	}
	
    glPopMatrix();
		    
    if (monster.deadCount >= deathStart)
	{
	if (!assetPoseSample(&externalTechsModel->model,
	                     externalTechsDeathClip,
	                     (float)monster.timeDead,
	                     externalTechsDeathLoop, &externalTechsPose))
	    return;
	    assetModelDrawNode(&externalTechsModel->model,
	                       requiredNode(externalTechsModel, "hand-right"),
	                       &externalTechsPose);
	    assetModelDrawNode(&externalTechsModel->model,
	                       requiredNode(externalTechsModel, "hand-left"),
	                       &externalTechsPose);
	}
    }


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw SIMPLE Techs                                             */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawSimpleTechs(struct monsterInfo monster, int detail)
    {
    float deathStart;

    if (!assetPoseReset(&externalTechsModel->model, &externalTechsPose))
        return;
    deathStart = animationScalar("techs-death-start");
    glPushMatrix();

    glRotatef(animationScalar("techs-heading-scale") *
              monster.headHorzRotate, 0, 1, 0);
 
    if (monster.deadCount < deathStart)
	{    
	glPushMatrix();
	    glRotatef(animationScalar("techs-head-pitch-scale") *
	              monster.headVertRotate, 1, 0, 0);
	    assetModelDrawNode(&externalTechsModel->model,
	                       requiredNode(externalTechsModel, "head-simple"),
	                       &externalTechsPose);
	glPopMatrix();   

	assetModelDrawNode(&externalTechsModel->model,
	                   requiredNode(externalTechsModel, "body-simple"),
	                   &externalTechsPose);
	}
    
    if ((!monster.monsterIsDead) && (detail > 0))
	{
	assetModelDrawNode(&externalTechsModel->model,
	                   requiredNode(externalTechsModel, "shadow"),
	                   &externalTechsPose);
	}
	
    glPopMatrix();
		    
    }

  
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw the energy typhoon hero                                  */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void setDamagePart(const struct AssetManifestModel *model,
                          struct AssetPose *pose, const char *part,
                          int intact)
    {
    char name[64];
    int intactNode;
    int brokenNode;

    sprintf(name, "%s-intact", part);
    intactNode = requiredNode(model, name);
    sprintf(name, "%s-broken", part);
    brokenNode = requiredNode(model, name);
    pose->nodes[intactNode].visible = intact;
    pose->nodes[brokenNode].visible = !intact;
    }

static void setConfiguredDamage(const struct AssetManifestModel *model,
                                struct AssetPose *pose, int damage,
                                const char *prefix)
    {
    const struct AssetValue *value;
    size_t index;
    size_t prefixLength;
    size_t matched;

    prefixLength = strlen(prefix);
    matched = 0;
    for (index = 0; index < externalAnimationValues.count; index++)
        {
        value = &externalAnimationValues.values[index];
        if (!hasPrefix(value->key, prefix))
            continue;
        if (value->componentCount != 1 || value->key[prefixLength] == '\0')
            {
            fprintf(stderr, "Invalid damage animation value: %s\n",
                    value->key);
            showError("Damage animation value is invalid");
            exit(1);
            }
        setDamagePart(model, pose, value->key + prefixLength,
                      damage > value->components[0]);
        matched++;
        }
    if (matched == 0)
        {
        fprintf(stderr, "No damage values use prefix: %s\n", prefix);
        showError("Damage animation values are missing");
        exit(1);
        }
    }

static void drawHeroModel(int subtype, int power, int counter,
                          int mcount, int dCount)
    {
    const struct AssetManifestModel *model;
    struct AssetPose *pose;
    float fireFrame;
    float brightness;

    model = externalHeroModels[subtype];
    pose = &externalHeroPoses[subtype];
    fireFrame = (float)mcount;
    if (mcount < 0)
        fireFrame = 0.0f;
    else if (mcount > animationScalar("hero-fire-limit"))
        fireFrame = animationScalar("hero-fire-limit");
    if (!assetPoseSample(&model->model, externalHeroWalkClips[subtype],
                         (float)counter,
                         externalHeroWalkLoops[subtype], pose) ||
        !assetPoseApply(&model->model, externalHeroFireClips[subtype],
                        fireFrame,
                        externalHeroFireLoops[subtype], pose))
        return;
    setConfiguredDamage(model, pose, power, "hero-damage-");
    brightness = dCount * animationScalar("hero-death-brightness");
    pose->colorOffset[0] = brightness;
    pose->colorOffset[1] = brightness;
    pose->colorOffset[2] = brightness;
    assetModelDraw(&model->model, pose);
    }

void drawHeroDude(int power, int counter,
                  int mcount, int dCount, int detail)
    {
    (void)detail;
    drawHeroModel(0, power, counter, mcount, dCount);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw the energy beam hero                                     */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawHeroDude2(int power, int counter,
                   int mcount, int dCount, int detail)
    {
    (void)detail;
    drawHeroModel(1, power, counter, mcount, dCount);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw MechaGoogelon                                            */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawMechaMonster(int dam, int counter, int detail)
    {
    size_t node;

    if (!assetPoseSample(&externalMechaModel->model, externalMechaClip,
                         (float)counter, externalMechaLoop,
                         &externalMechaPose))
        return;
    setConfiguredDamage(externalMechaModel, &externalMechaPose, dam,
                        "mecha-damage-");
    for (node = 0; node < externalMechaModel->model.nodeCount; node++)
        if (externalMechaModel->model.nodes[node].name != NULL &&
            hasPrefix(externalMechaModel->model.nodes[node].name, "shadow-"))
            externalMechaPose.nodes[node].visible = detail > 0;
    assetModelDraw(&externalMechaModel->model, &externalMechaPose);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw generic square building                                  */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawBuilding0(float * colour, int detail)
    {
    int grey = colour == colorgrey1;
    glCallList(building0Models[grey][detail > 0]);
    }


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw generic fast food resteraunt                             */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
   
void drawBuilding6(int counter, float * c1,  float * c2, int detail)
    {
    const struct AssetManifestModel *model;
    struct AssetPose *pose;
    size_t clip;
    int loop;
    int alternate = c1 == colorgrey2;

    (void)c2;
    if (detail <= 0)
        {
        glCallList(alternate ? building28Low : building6Low);
        return;
        }
    model = alternate ? externalBuilding28Model : externalBuilding6Model;
    pose = alternate ? &externalBuilding28Pose : &externalBuilding6Pose;
    clip = alternate ? externalBuilding28Clip : externalBuilding6Clip;
    loop = alternate ? externalBuilding28Loop : externalBuilding6Loop;
    if (assetPoseSample(&model->model, clip, (float)counter, loop,
                        pose))
        assetModelDraw(&model->model, pose);
    }


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw warehouse / hanger                                       */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawBuilding8(float * c1,  float * c2, int detail)
    {
    (void)c1;
    glCallList(c2 == colorgrey2 ? building30Models[detail > 0] :
                                 building8Models[detail > 0]);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw cooling towers                                           */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawCoolingTower(int detail)
    {
    glCallList(building4Models[detail < 0 ? 0 : (detail == 0 ? 1 : 2)]);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw factory smokestacks                                      */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawBuilding11(int detail)
    {
    glCallList(building11Models[detail > 0]);
    }


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw office building                                          */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawBuilding22(float * color)
    {
    glCallList(color == colorgrey1 ? building22Obj :
               (color == colorgrey2 ? building23Obj : building24Obj));
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw smaller office building                                   */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawBuilding34(float * color)
    {
    (void)color;
    glCallList(building34Obj);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw small office building                                    */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawBuilding36(float * color)
    {
    (void)color;
    glCallList(building36Obj);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw office building with sign                                */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawBuilding37(float * color, int detail)
    {
    (void)color;
    if (!assetPoseReset(&externalBuilding37Model->model,
                        &externalBuilding37Pose))
        return;
    assetModelDrawNode(&externalBuilding37Model->model,
                       requiredNode(externalBuilding37Model, "body"),
                       &externalBuilding37Pose);
    if (detail >= 2)
        {
        glCallList(officeLogo);
        glEnable(GL_TEXTURE_2D);
        }
    assetModelDrawNode(&externalBuilding37Model->model,
                       requiredNode(externalBuilding37Model, "logo"),
                       &externalBuilding37Pose);
    glDisable(GL_TEXTURE_2D);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw satellite dish                                           */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawBuilding25()
    {
    glCallList(building25Obj);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw drive in theatre screen                                  */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
  
void drawBuilding26(int rorl, int detail)
    {
    if (!assetPoseReset(&externalBuilding26Model->model,
                        &externalBuilding26Pose))
        return;
    assetModelDrawNode(&externalBuilding26Model->model,
                       requiredNode(externalBuilding26Model, "body"),
                       &externalBuilding26Pose);
    if (detail >= 2)
        {
        glCallList(rorl == 0 ? leftScreen : rightScreen);
        glEnable(GL_TEXTURE_2D);
        assetModelDrawNode(&externalBuilding26Model->model,
                           requiredNode(externalBuilding26Model, "screen"),
                           &externalBuilding26Pose);
        glDisable(GL_TEXTURE_2D);
        }
    }


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw fountain                                                 */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawBuilding29(int itsChristmas, int detail)
    {
    (void)detail;
    glCallList(building29Models[itsChristmas != 0]);
    }

   
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw hero beam in effect                                      */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawBuilding33(int count, int detail)
    {
    if (detail > 0)
        glEnable(GL_BLEND);
    if (assetPoseSample(&externalBuilding33Model->model,
                        externalBuilding33Clip, (float)count,
                        externalBuilding33Loop, &externalBuilding33Pose))
        assetModelDraw(&externalBuilding33Model->model,
                       &externalBuilding33Pose);
    glDisable(GL_BLEND);
    }
 
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw power line tower                                         */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawTower(int death, int death2)
    {
    glPushMatrix();
    glRotatef((float)death2, 0.0f, 1.0f, 0.0f);
    if (assetPoseSample(&externalPowerTowerModel->model,
                        externalPowerTowerClip, (float)death,
                        externalPowerTowerLoop, &externalPowerTowerPose))
        assetModelDraw(&externalPowerTowerModel->model,
                       &externalPowerTowerPose);
    glPopMatrix();
    }



/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw the square playing field                                 */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawBattlefield(struct road * roads,  float xshift,  float zshift, 
		    int detail, int itsChristmas, int view)
    {
    float theta, radius;
    float angle;

    /******************************/
    /* 'where is the town' arrow  */
    /******************************/

    if (itsChristmas)
	glColor3fv(colorsnow);
    else
	glColor3fv(planeGreen);


    if (view == MAPVIEW)
	glCallList(bigPlane);    
    else if (detail <= 1)
	glCallList(lowDPlane);
    else
	glCallList(highDPlane);


    /***************/
    /* draw roads  */
    /***************/

    drawRoads(roads, xshift, zshift, itsChristmas, view, detail);
    
    
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
/* draw square lake segment                                      */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawLake0(int itsChristmas)
    {
    glCallList(waterModels[0][itsChristmas != 0]);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw triangular lake segment                                  */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawLake1(int itsChristmas)
    {
    glCallList(waterModels[1][itsChristmas != 0]);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw central hill segment                                     */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawHill0(int itsChristmas)
    {
    glCallList(hillCenterModels[itsChristmas != 0]);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw side of hill                                             */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawHill1(int sidemod, int itsChristmas)
    {
    if (sidemod < 0)
        sidemod = 0;
    if (sidemod > 2)
        sidemod = 2;
    glCallList(hillSideModels[sidemod][itsChristmas != 0]);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw corner of hill                                           */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawHill2(int sidemod, int itsChristmas)
    {
    glCallList(hillCornerModels[sidemod != 0][itsChristmas != 0]);
    }
    
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw inside corner of hill                                    */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawHill3(int sidemod, int itsChristmas)
    {
    glCallList(hillInsideModels[sidemod != 0][itsChristmas != 0]);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

 void drawWater(int shape,  int itsChristmas)
    {
    switch(shape){
	case 0:	    drawLake0(itsChristmas);
		    break;
		    
	case 1:	    drawLake1(itsChristmas);	/*   |/ */
		    break;
		    
	case 2:	    glRotatef(-90, 0, 1, 0);	/* \| */
		    drawLake1(itsChristmas);
		    break;
		    
	case 3:	    glRotatef(-180, 0, 1, 0);	/* /| */
		    drawLake1(itsChristmas);
		    break;
		    
	case 4:	    glRotatef(90, 0, 1, 0);	/* |\ */
		    drawLake1(itsChristmas);
		    break;
	}
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

 void drawHills(int shape,  int itsChristmas)
    {
    switch(shape){
	case 0:	    drawHill0(itsChristmas);
		    break;
		    
	case 1:	    drawHill1(0, itsChristmas); /* up neg Z */
		    break;
		    
	case 2:	    glRotatef(-90, 0, 1, 0);	/* up pos x */
		    drawHill1(1, itsChristmas);
		    break;
		    
	case 3:	    glRotatef(-180, 0, 1, 0);	/* up pos z */
		    drawHill1(2, itsChristmas);
		    break;
		    
	case 4:	    glRotatef(90, 0, 1, 0);	/* up neg x */
		    drawHill1(1, itsChristmas);
		    break;
		    
		    
	case 5:	    drawHill2(0,  itsChristmas); /* +x,+z corner */
		    break;
		    
	case 6:	    glRotatef(-90, 0, 1, 0);	/* up pos x */
		    drawHill2(0, itsChristmas);
		    break;
		    
	case 7:	    glRotatef(-180, 0, 1, 0);	/* up pos z */
		    drawHill2(1, itsChristmas);
		    break;
		    
	case 8:	    glRotatef(90, 0, 1, 0);	/* up neg x */
		    drawHill2(1, itsChristmas);
		    break;
		    
		    
	case 9:	    drawHill3(0, itsChristmas); /* up +x,-z */
		    break;
		    
	case 10:    glRotatef(-90, 0, 1, 0);	/* up +x,+z */
		    drawHill3(0, itsChristmas);
		    break;
		    
	case 11:    glRotatef(-180, 0, 1, 0);	/* up pos z */
		    drawHill3(1, itsChristmas);
		    break;
		    
	case 12:    glRotatef(90, 0, 1, 0);	/* up neg x */
		    drawHill3(1, itsChristmas);
		    break;

	}
    }


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw all structures on plane (buildings,trees,hills,lakes)    */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawtrees(struct tree * allTreesOnPlane, int numTreesOnPlane, int counter, 
		int detail, int itsChristmas, int view,
		float viewerX, float viewerZ)
    {
    
    float x, y, z;
    register int treeCounter;
    GLfloat lineWidthNow;
    GLint shadeNow;
    
    glGetIntegerv(GL_SHADE_MODEL, &shadeNow);
  
    for (treeCounter=0; treeCounter < numTreesOnPlane; treeCounter++)
	{
	if (detail >= 2)
	    glShadeModel(GL_SMOOTH);
	else
	    glShadeModel(GL_FLAT);


	if (((fabs(allTreesOnPlane[treeCounter].x - viewerX) <= PLANESIZE) &&
	     (fabs(allTreesOnPlane[treeCounter].z - viewerZ) <= PLANESIZE)) ||
	    (view == MAPVIEW))
	    {
	    x = allTreesOnPlane[treeCounter].x;
	    y = allTreesOnPlane[treeCounter].y;
	    z = allTreesOnPlane[treeCounter].z;

	    /**********************************/
	    /* tree                           */
	    /**********************************/

	    if (allTreesOnPlane[treeCounter].type == 0)
		{
		const struct AssetManifestModel *treeModel;
		struct AssetPose *treePose;
		int pine;

		pine = allTreesOnPlane[treeCounter].treeshape == 0 || itsChristmas;
		treeModel = pine ? externalPineTreeModel : externalRoundTreeModel;
		treePose = pine ? &externalPineTreePose : &externalRoundTreePose;
		if (!assetPoseReset(&treeModel->model, treePose))
		    continue;
		glPushMatrix();
		glTranslatef(x, y, z);
		if (detail >= 2)
		    {
		    glCallList(treeWood);
		    glEnable(GL_TEXTURE_2D);
		    }
		assetModelDrawNode(&treeModel->model,
		                   requiredNode(treeModel,
		                                pine ? "pine-trunk" : "round-trunk"),
		                   treePose);
		glDisable(GL_TEXTURE_2D);
		assetModelDrawNode(&treeModel->model,
		                   requiredNode(treeModel,
		                       pine ? (detail < 0 ? "pine-crown-vector" :
		                                            "pine-crown") :
		                              "round-crown"),
		                   treePose);
		if (pine && itsChristmas)
		    assetModelDrawNode(&treeModel->model,
		                       requiredNode(treeModel, "pine-lights"), treePose);
		glPopMatrix();

		if (detail > 0)
		    {
		    glPushMatrix();
		    glColor4fv(colorblack);
		    glTranslatef(x, pine ? 0.0f : SHADOWS, z);
		    assetModelDrawNode(&treeModel->model,
		                       requiredNode(treeModel,
		                                    pine ? "pine-shadow" :
		                                           "round-shadow"),
		                       treePose);
		    glPopMatrix();
		    }
		}

	    /**********************************/
	    /* building                       */
	    /**********************************/
	    	    
	    if (allTreesOnPlane[treeCounter].type == 1)
		{
		glPushMatrix();

		    glTranslatef(x,  y,  z);
		    
		    if (allTreesOnPlane[treeCounter].treeshape <= 17)
			switch(allTreesOnPlane[treeCounter].treeshape){
			    case 0:	drawBuilding0(colorwhite, detail);
					break;
			    case 1:	if (detail == 0)
					    glCallList(building1ObjD0);
					else
					    glCallList(building1Obj);
					break;
			    case 2:	if (detail == 0)
					    glCallList(building2ObjD0);
					else
					    glCallList(building2Obj);
					break;
			    case 3:	glGetFloatv(GL_LINE_WIDTH, &lineWidthNow);
					drawTower(allTreesOnPlane[treeCounter].deathCount &&
					          allTreesOnPlane[treeCounter].death2 ?
					          allTreesOnPlane[treeCounter].deathCount : 0,
					          allTreesOnPlane[treeCounter].death2);
					glLineWidth(lineWidthNow);
					break;
			    case 4:	glPushAttrib(GL_LIGHTING_BIT);
					if (detail <= 0)
					    {
					    glShadeModel(GL_FLAT);
					    drawCoolingTower(detail);
					    }
					else
					    {
					    glShadeModel(GL_SMOOTH);
					    drawCoolingTower(detail);    /* cooling tower */
					    }
					glPopAttrib();
					break;
			    case 5:	if (detail == 0)	 /* home */
					    glCallList(building5ObjD0);
					else
					    glCallList(building5Obj);
					break;
			    case 6:	drawBuilding6(counter, colorred, coloryellow, detail); /* fast food */
					break;
			    case 7:	glCallList(building7Obj);
					break;
			    case 8:	drawBuilding8(colorbeige,  colorbrown, detail);
					break;
			    case 9:	glPushAttrib(GL_LIGHTING_BIT);
					if (detail < 0)
					    {
					    glShadeModel(GL_FLAT);
					    glCallList(building9ObjOO);
					    }
					else if (detail == 0)
					    {
					    glShadeModel(GL_FLAT);
					    glCallList(building9Obj);
					    }
					else
					    {
					    glShadeModel(GL_SMOOTH);
					    glCallList(building9Obj);	    /*water tower*/
					    }
					glPopAttrib();
					break;
			    case 10:    glGetFloatv(GL_LINE_WIDTH, &lineWidthNow);
					glCallList(building10Obj);
					glLineWidth(lineWidthNow);
					break;
			    case 11:    glPushAttrib(GL_LIGHTING_BIT);
					if (detail <= 0)
					    glShadeModel(GL_FLAT);
					else
					    glShadeModel(GL_SMOOTH);
					drawBuilding11(detail);		    /* factory smokestacks*/
					glPopAttrib();
					break;
			    case 12:    if (detail == 0)
					    glCallList(building12ObjD0);
					else
					    glCallList(building12Obj);
					break;
			    case 13:    glRotatef(180, 0, 1, 0);
					if (detail == 0)
					    glCallList(building5ObjD0);
					else
					    glCallList(building5Obj);
					break;
			    case 14:    glCallList(building14Obj);
					break;
			    case 15:    glCallList(strip1Obj); 	/* strip mall */
					break;
			    case 16:    glCallList(strip2Obj); 	/* strip mall */
					break;
			    case 17:    glCallList(strip3Obj); 	/* strip mall */
					break;
			    }
		    else
			switch(allTreesOnPlane[treeCounter].treeshape){
			    case 18:    glRotatef(90, 0, 1, 0);
					drawBuilding0(colorgrey1, detail);
					break;
			    case 19:    if (detail == 0)
					    glCallList(building19ObjD0);
					else
					    glCallList(building19Obj);
					break;
			    case 20:    if (detail == 0)
					    glCallList(building20ObjD0);
					else
					    glCallList(building20Obj);
					break;
			    case 21:    if (detail == 0)
					    glCallList(building21ObjD0);
					else
					    glCallList(building21Obj);
					break;
			    case 22:    drawBuilding22(colorgrey1);
					break;
			    case 23:    drawBuilding22(colorgrey2);
					break;
			    case 24:    drawBuilding22(colorwhite);
					break;
			    case 25:    drawBuilding25();
					break;
			    case 26:    drawBuilding26(0, detail);
					break;
			    case 27:    drawBuilding26(1, detail);
					break;
			    case 28:    glRotatef(-90, 0, 1, 0);
					drawBuilding6(counter, colorgrey2, colorblue, detail);
					break;
			    case 29:    drawBuilding29(itsChristmas, detail);
					break;
			    case 30:    drawBuilding8(colorbeige,  colorgrey2, detail);
					break;
			    case 31:    glCallList(building31Obj); /* maser battery */
					break;
			    case 32:    glCallList(building32Obj); /* helo pad */
					break;
			    case 33:    drawBuilding33(allTreesOnPlane[treeCounter].deathCount, detail); /* hero entrance */
					break;
			    case 34:    drawBuilding34(colorblue2); /* smallest office building */
					break;
			    case 36:    drawBuilding36(colorblue2); /* small office building */
					break;
			    case 37:    drawBuilding37(colorblue2, detail);
					break;
			    default:    showError("Bogus Building in Tree List! (drawtrees)");
					break;
			}

		glPopMatrix();	
		}

	    /**********************************/
	    /* water                          */
	    /**********************************/
		
	    if (allTreesOnPlane[treeCounter].type == 2)
		{
		glPushMatrix();
		    glTranslatef(x,  ROADS,  z);
		    
		    drawWater(allTreesOnPlane[treeCounter].treeshape, itsChristmas);
		    
		glPopMatrix();
		}

	    /**********************************/
	    /* hills                          */
	    /**********************************/

	    if (allTreesOnPlane[treeCounter].type == 3)
		{
		glPushMatrix();
		    glTranslatef(x, PLANEY, z);		    
		    drawHills(allTreesOnPlane[treeCounter].treeshape, itsChristmas);		    
		glPopMatrix();
		}	    
	    }
	}
	glShadeModel(shadeNow);
    }


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw all tanks currently dying                                */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void multiplyQuaternion(const float left[4], const float right[4],
                               float result[4])
    {
    result[0] = left[3] * right[0] + left[0] * right[3] +
                left[1] * right[2] - left[2] * right[1];
    result[1] = left[3] * right[1] - left[0] * right[2] +
                left[1] * right[3] + left[2] * right[0];
    result[2] = left[3] * right[2] + left[0] * right[1] -
                left[1] * right[0] + left[2] * right[3];
    result[3] = left[3] * right[3] - left[0] * right[0] -
                left[1] * right[1] - left[2] * right[2];
    }

static void rotatePoseNode(struct AssetNodePose *node, float degrees,
                           int axis)
    {
    float controller[4];
    float result[4];
    float radians;
    float sine;

    radians = degrees * DEG_TO_RAD * 0.5f;
    sine = (float)sin((double)radians);
    controller[0] = controller[1] = controller[2] = 0.0f;
    controller[axis] = sine;
    controller[3] = (float)cos((double)radians);
    multiplyQuaternion(node->rotation, controller, result);
    memcpy(node->rotation, result, sizeof(result));
    }

static void applyExternalEntityAim(const struct AssetModel *model,
                                   const struct EntityArchetype *archetype,
                                   const struct tank *tank,
                                   struct AssetPose *pose)
    {
    int node;

    if (archetype->aimYawNode[0] != '\0')
        {
        node = assetModelFindNode(model, archetype->aimYawNode);
        if (node >= 0)
            rotatePoseNode(&pose->nodes[node], tank->aimYaw, 1);
        }
    if (archetype->aimPitchNode[0] != '\0')
        {
        node = assetModelFindNode(model, archetype->aimPitchNode);
        if (node >= 0)
            rotatePoseNode(&pose->nodes[node], tank->aimPitch, 0);
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

static void drawExternalEntity(const struct tank *tank, int counter,
                               int destroyed)
    {
    const struct EntityArchetype *archetype;
    const struct AssetManifestModel *model;
    const char *animation;
    size_t clip;
    int loop;

    archetype = tank->dataArchetype;
    if (archetype == NULL)
        return;
    glRotatef(archetype->modelYawOffset, 0, 1, 0);
    if (destroyed && archetype->destroyedModel[0] != '\0')
        {
        model = assetManifestFindModel(&externalAssets,
                                       archetype->destroyedModel);
        if (model != NULL)
            assetModelDraw(&model->model, NULL);
        return;
        }
    animation = archetype->idleAnimation;
    if (tank->dataAgent.reaction == ENTITY_REACTION_ATTACK &&
        archetype->attackAnimation[0] != '\0')
        animation = archetype->attackAnimation;
    else if (tank->walking && archetype->moveAnimation[0] != '\0')
        animation = archetype->moveAnimation;
    if (animation[0] != '\0' &&
        assetManifestFindAnimation(&externalAssets, animation, &model,
                                   &clip, &loop) &&
        assetPoseSample(&model->model, clip,
                        (float)counter * archetype->animationSpeed,
                        loop, &externalEntityPose))
        {
        applyExternalEntityAim(&model->model, archetype, tank,
                               &externalEntityPose);
        drawExternalEntityModel(model, archetype, &externalEntityPose);
        return;
        }
    model = assetManifestFindModel(&externalAssets, archetype->model);
    if (model != NULL &&
        assetPoseReset(&model->model, &externalEntityPose))
        {
        applyExternalEntityAim(&model->model, archetype, tank,
                               &externalEntityPose);
        drawExternalEntityModel(model, archetype, &externalEntityPose);
        }
    }

void drawConfiguredPlayerEntity(const struct EntityArchetype *archetype,
                                int moving, int attacking, int counter,
                                int destroyed, float aimYaw, float aimPitch)
    {
    struct tank entity;

    if (archetype == NULL)
        return;
    memset(&entity, 0, sizeof(entity));
    entity.dataArchetype = archetype;
    entity.dataAgent.reaction = attacking ? ENTITY_REACTION_ATTACK :
                                           ENTITY_REACTION_OBSERVE;
    entity.walking = moving;
    entity.aimYaw = aimYaw;
    entity.aimPitch = aimPitch;
    glScalef(archetype->scale, archetype->scale, archetype->scale);
    drawExternalEntity(&entity, counter, destroyed);
    }

void drawSlagTanks(struct tank * allSlags, int counter, int detail,
		   float viewerX, float viewerZ)
    {    
    const struct AssetValue *groundScale;
    const struct AssetValue *maserScale;
    int c;
    float angle2;
    float airRoll;
    float airPitch;
    float airYaw;
    struct tank * ttank;

    groundScale = requiredAnimationValue("slag-ground-scale", 3);
    maserScale = requiredAnimationValue("slag-maser-scale", 3);
    airRoll = animationScalar("slag-air-roll");
    airPitch = animationScalar("slag-air-pitch");
    airYaw = animationScalar("slag-air-yaw");
    
    for(ttank = allSlags->next;ttank != NULL;ttank = ttank->next)
	{
	if ((fabs(ttank->x - viewerX) <= PLANESIZE) &&
	    (fabs(ttank->z - viewerZ) <= PLANESIZE))
	    {
	    angle2 = ttank->theta * RAD_TO_SMALL_DEG;

	    c = ttank->count;
		    
	    glPushMatrix();
	
	    switch(ttank->type){
		case TANK:	glTranslatef(ttank->x, ttank->y +
				    animationScalar("slag-ground-position"), ttank->z);
				glScalef(groundScale->components[0] +
				             groundScale->components[1] * c,
				         groundScale->components[0] +
				             groundScale->components[2] * c,
				         groundScale->components[0] +
				             groundScale->components[1] * c);
				glRotatef(angle2, 0, 1, 0);
				glCallList(slagtank);
				break;
	
		case LAUNCHER:	glTranslatef(ttank->x, ttank->y +
				    animationScalar("slag-ground-position"), ttank->z);
				glScalef(groundScale->components[0] +
				             groundScale->components[1] * c,
				         groundScale->components[0] +
				             groundScale->components[2] * c,
				         groundScale->components[0] +
				             groundScale->components[1] * c);
				glRotatef(angle2, 0, 1, 0);
				glCallList(slaglaunchertank);
				break;
	
		    	    
		case MASERTANK:	glTranslatef(ttank->x, ttank->y +
				    animationScalar("slag-ground-position"), ttank->z);
				glScalef(maserScale->components[0] +
				             maserScale->components[1] * c,
				         maserScale->components[0] +
				             maserScale->components[2] * c,
				         maserScale->components[0] +
				             maserScale->components[1] * c);
				glRotatef(angle2, 0, 1, 0);
				glCallList(slagmasertank);
				break;
  
		case HELO:	glTranslatef(ttank->x, ttank->y, ttank->z);
				glRotatef(c * airPitch, 1, 0, 0);
				glRotatef(c * airRoll, 0, 0, 1);
				glRotatef(angle2 +
				    animationScalar("slag-air-facing") +
				    c * airYaw, 0, 1, 0);
				drawHelo((float)counter, 1);
				break;

		case CHH:	glTranslatef(ttank->x, ttank->y, ttank->z);
				glRotatef(c * airPitch, 1, 0, 0);
				glRotatef(c * airRoll, 0, 0, 1);
				glRotatef(angle2 +
				    animationScalar("slag-chh-facing") +
				    c * airYaw, 0, 1, 0);
				makeCHH(0);
				break;

		case MECHAG:	glTranslatef(ttank->x, 0, ttank->z);
				glRotatef(angle2, 0, 1, 0);
				drawMechaMonster(0, counter, detail);
				break;
				
		case HERO:	glTranslatef(ttank->x, 0, ttank->z);
				glRotatef(angle2, 0, 1, 0);
				if (ttank->subtype == 0)
				    drawHeroDude(0, ttank->walking, ttank->maserCount, c, detail);
				else
				    drawHeroDude2(0, ttank->walking, ttank->maserCount, c, detail);
				break;
				
		case AIRPLANE:	glTranslatef(ttank->x, ttank->y, ttank->z);
				glRotatef(c * airRoll, 0, 0, 1);
				glRotatef(angle2 +
				    animationScalar("slag-air-facing") +
				    c * airYaw, 0, 1, 0);
				glScalef(animationScalar("airplane-world-scale"),
				         animationScalar("airplane-world-scale"),
				         animationScalar("airplane-world-scale"));
				drawAirplane(counter);
				break;

		case FIGHTER:	glTranslatef(ttank->x, ttank->y, ttank->z);
				glRotatef(c * airRoll, 0, 0, 1);
				glRotatef(angle2 +
				    animationScalar("slag-air-facing") +
				    c * airYaw, 0, 1, 0);
				glScalef(animationScalar("fighter-world-scale"),
				         animationScalar("fighter-world-scale"),
				         animationScalar("fighter-world-scale"));
				drawFighter();
				break;

		case DATAENTITY: glTranslatef(ttank->x, ttank->y, ttank->z);
				glRotatef(angle2, 0, 1, 0);
				glScalef(ttank->dataArchetype->scale,
				         ttank->dataArchetype->scale,
				         ttank->dataArchetype->scale);
				drawExternalEntity(ttank, counter, 1);
				break;
				
		default:	showError("Bogus Vehicle in Tank List! (drawSlagTanks)");
				break;
		}
	    	    
	    glPopMatrix();
	    }
	}
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw maser beams                                              */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawMaser(float sourceX, float sourceY, float sourceZ,
		int shooter, float mX, float mZ, float mHeight, float mBottom, 
		float r1, float r2, float r3, int width, int view, int detail,
		const float *configuredColor)

    {
    float beamLoc1[3],  beamLoc2[3];
    float *beamColor;
    const struct AssetValue *innerWidths;
    const struct AssetValue *outerWidths;
    GLfloat lineWidthNow;

    beamLoc1[0] = sourceX;
    beamLoc1[1] = sourceY;
    beamLoc1[2] = sourceZ;

    beamLoc2[0] = mX + r1;
    beamLoc2[1] = 0.5 * (mHeight - mBottom) + mBottom + r2;
    beamLoc2[2] = mZ + r3;

    glGetFloatv(GL_LINE_WIDTH, &lineWidthNow);
    innerWidths = requiredEffectValue("maser-inner-width", 2);
    outerWidths = requiredEffectValue("maser-outer-width", 2);

    /* if line widths are integral Mesa chokes in a large window */
    if ((view == MONSTERVIEW) || (view == FIRSTPERSONVIEW) ||
	(view == ARMYVIEW))
	glLineWidth(width * innerWidths->components[0]);
    else
	glLineWidth(width * innerWidths->components[1]);
    
    dynamicDrawLine3(beamLoc1, beamLoc2, colorwhite, colorwhite);

    beamColor = colorwhite;
    switch(shooter)
	{
	case MASERTANK:	    beamColor = colormaserblue;
			    break;
			    
	case CHH:	    beamColor = colormaseryellow;
			    break;
			    
	case MECHAG:	    beamColor = colorred;
			    break;

	case DATAENTITY: beamColor = configuredColor == NULL ? colorred :
				 (float *)configuredColor;
			    break;
			    
	default:	    showError("Invalid Vehicle Type (drawMaser)");
			    break;
	}
	
    if (detail != -1)
	glEnable(GL_BLEND);
    
    /* if line widths are integral Mesa chokes in a large window */
    if ((view == MONSTERVIEW) || (view == FIRSTPERSONVIEW) ||
	(view == ARMYVIEW))
	glLineWidth(width * outerWidths->components[0]);
    else
	glLineWidth(width * outerWidths->components[1]);

    dynamicDrawLine3(beamLoc1, beamLoc2, beamColor, beamColor);
	
    glDisable(GL_BLEND);

    glLineWidth(lineWidthNow);
    } 

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw all tanks on the battlefield                             */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawTanks(struct tank * allTanks,  int counter, struct monsterInfo G,
		struct targetInfo * targs, int detail, int view, int wid)

    {
    struct tank cTank;
    struct tank * temptank;
    float angle2;
    int monsterIsDead;
    float height, bottom;
    float xloc,  zloc;
    float radToNearestTarget, rad;
    struct targetInfo * nearestTarget;
    struct monsterInfo * nearestMonster;
    struct targetInfo * temptarget;
    

    for(temptank = allTanks->next;temptank != NULL;temptank = temptank->next)
	{
	cTank = *temptank;
	
	if (((fabs(cTank.x - G.x) <= PLANESIZE) &&
	     (fabs(cTank.z - G.z) <= PLANESIZE)) || (view == MAPVIEW))
	    {
	    
	    
	/* need to find the nearest target */
	
	radToNearestTarget = 10000;
	nearestTarget = NULL;
    	for(temptarget = targs->next;temptarget != NULL;temptarget = temptarget->next)
	    {
	    rad = sqrt((cTank.x-temptarget->x) * (cTank.x-temptarget->x) + (cTank.z-temptarget->z) * (cTank.z-temptarget->z));
	    if (rad < radToNearestTarget)
		{
		radToNearestTarget = rad;
		nearestTarget = temptarget;
		nearestMonster = &(temptarget->monster);
		}
	    }

	/* need to compute radius to player's monster */

	rad = sqrt((cTank.x - G.x) * (cTank.x - G.x) +
	           (cTank.z - G.z) * (cTank.z - G.z));

	if (rad < radToNearestTarget)
	    {
	    radToNearestTarget = rad;
	    nearestMonster = NULL;
	    nearestTarget = NULL;
	    }

	if (nearestMonster == NULL)
	    {
	    monsterIsDead = G.monsterIsDead;
	    height = G.height;
	    bottom = G.bottom;
	    xloc = G.x;
	    zloc = G.z;
	    }
	else
	    {
	    monsterIsDead = nearestMonster->monsterIsDead;
	    height = nearestMonster->height;
	    bottom = nearestMonster->bottom;
	    xloc = nearestTarget->x;
	    zloc = nearestTarget->z;
	    }


	    /**********************************/
	    /* aim vehicle                    */
	    /**********************************/

	    angle2 = cTank.theta * RAD_TO_SMALL_DEG;
	    
	    /**********************************/
	    /* draw vehicle                   */
	    /**********************************/
		    
	    glPushMatrix();

	    switch (cTank.type)
		{
		case TANK:	glTranslatef(cTank.x, cTank.y, cTank.z);
  				glRotatef(angle2, 0, 1, 0);
				glCallList(plaintank);
				break;

		case LAUNCHER:	glTranslatef(cTank.x, cTank.y, cTank.z);
  				glRotatef(angle2, 0, 1, 0);
				glCallList(launchertank);
				break;
			
		case MASERTANK:	glTranslatef(cTank.x, cTank.y, cTank.z);
  				glRotatef(angle2, 0, 1, 0);
				glScalef(animationScalar("masertank-world-scale"),
				         animationScalar("masertank-world-scale"),
				         animationScalar("masertank-world-scale"));
				glCallList(masertank);
				break;

		case HELO:	glTranslatef(cTank.x, cTank.y, cTank.z);
  				glRotatef(angle2+180, 0, 1, 0);
				drawHelo((float)counter,  0);
				break;

		case AIRPLANE:	glTranslatef(cTank.x, cTank.y, cTank.z);
  				glRotatef(angle2+180, 0, 1, 0);
				glScalef(animationScalar("airplane-world-scale"),
				         animationScalar("airplane-world-scale"),
				         animationScalar("airplane-world-scale"));
				drawAirplane(counter);
				break;

		case FIGHTER:	glTranslatef(cTank.x, cTank.y, cTank.z);
  				glRotatef(angle2+180, 0, 1, 0);
				glScalef(animationScalar("fighter-world-scale"),
				         animationScalar("fighter-world-scale"),
				         animationScalar("fighter-world-scale"));
				drawFighter();
				break;

		case CHH:	glTranslatef(cTank.x, cTank.y, cTank.z);
  				glRotatef(angle2, 0, 1, 0);
				makeCHH(cTank.damage);
				break;
				
		case MECHAG:	glTranslatef(cTank.x, 0, cTank.z);
  				glRotatef(angle2, 0, 1, 0);
				drawMechaMonster(cTank.damage, counter, detail);
				break;

		case HERO:	glTranslatef(cTank.x, 0, cTank.z);
  				glRotatef(angle2, 0, 1, 0);
				if (cTank.subtype == 0)
				    drawHeroDude(cTank.damage, cTank.walking, cTank.maserCount, 0, detail);
				else
				    drawHeroDude2(cTank.damage, cTank.walking, cTank.maserCount, 0, detail);
				break;

		case DATAENTITY: glTranslatef(cTank.x, cTank.y, cTank.z);
				glRotatef(angle2, 0, 1, 0);
				glScalef(cTank.dataArchetype->scale,
				         cTank.dataArchetype->scale,
				         cTank.dataArchetype->scale);
				drawExternalEntity(&cTank, counter, 0);
				break;
				
		default:	showError("Bogus Vehicle in Tank List! (drawTanks)");
				break;
		}
	    
	    glPopMatrix();	

	    if (detail > 0)  	
		{

		/**********************************/
		/* draw shadow                    */
		/**********************************/

		glPushMatrix();
		    glTranslatef(cTank.x, 0, cTank.z);
		    glRotatef(angle2, 0, 1, 0);

		    switch (cTank.type){
		    case TANK:
		    case LAUNCHER:
		    case MASERTANK:	glCallList(tankShadow);
					break;
		    
		    case HELO:	    	glColor4fv(colorblack);
					shadow (0, 0, 0.14, 0.18);
					shadow (0, -0.22, 0.04, 0.1);
					break;
		    
		    case MECHAG:	break;
		    
		    case CHH:		glColor4fv(colorblack);
					shadow (0, 0, 0.14, 0.22);
					break;
		    
		    case HERO:		glColor4fv(colorblack);
					shadow (0, 0, 0.12, 0.08);
					break;
		    
		    case AIRPLANE:	glCallList(planeShadow);
					break;

		    case FIGHTER:	glCallList(fighterShadow);	
					break;

		    case DATAENTITY: if (cTank.dataArchetype->shadowModel[0] != '\0')
					    glCallList(assetModelDisplayList(
					        cTank.dataArchetype->shadowModel));
					break;

		    default:		showError("Bogus Vehicle in Tank List! (drawTanks)");
					break;
		    }
		glPopMatrix();
		}
  
	    if ((cTank.maserCount > 0) && (cTank.rad <= cTank.range) && (cTank.rad >= 0.5) && (!monsterIsDead))
		{
		/**********************************/
		/* mechaGoogelon firing maser     */
		/**********************************/

		if (cTank.type == MECHAG)
		    drawMaser(cTank.x, cTank.y + 0.8, cTank.z, cTank.type, xloc, zloc, height,
			    bottom, cTank.rand1, cTank.rand2, cTank.rand3, wid, view,
			    detail, NULL);
  
		/**********************************/
		/* 009CHH firing maser            */
		/**********************************/
	
		if (cTank.type == CHH)
		    drawMaser(cTank.x, cTank.y, cTank.z, cTank.type, xloc, zloc, height,
			    bottom, cTank.rand1, cTank.rand2, cTank.rand3, wid, view,
			    detail, NULL);
  		
		/**********************************/
		/* tank firing maser              */
		/**********************************/

		if (cTank.type == MASERTANK)
		    drawMaser(cTank.x, cTank.y + 0.375, cTank.z, cTank.type, xloc, zloc, height,
			    bottom, cTank.rand1, cTank.rand2, cTank.rand3, wid, view,
			    detail, NULL);
  
		/**********************************/
		/* hero firing its weapon         */
		/**********************************/

		if ((cTank.type == HERO) && (cTank.subtype == 1) && (cTank.maserCount > 10) && (cTank.maserCount < 30))
		    {
		    if (nearestTarget)
    			drawHeroWeapon(nearestTarget->x, nearestTarget->z, cTank.x, cTank.y + 0.275, cTank.z, colormaserblue, height, bottom);
		    else
			drawHeroWeapon(G.x, G.z, cTank.x, cTank.y + 0.275, cTank.z, colormaserblue, height, bottom);
		    }

		if (cTank.type == DATAENTITY && cTank.maserCount > 0 &&
		    cTank.dataArchetype->weapon == ENTITY_WEAPON_BEAM &&
		    cTank.dataAgent.hasTarget)
		    drawMaser(cTank.x, cTank.y + cTank.dataArchetype->muzzleHeight,
		              cTank.z, DATAENTITY,
		              cTank.dataAgent.targetX, cTank.dataAgent.targetZ,
		              PLANEY + 1.2, PLANEY, 0, 0, 0, wid, view, detail,
		              paletteColor(cTank.dataArchetype->weaponColor));

  		} 
	    }  
	}  
    }

 
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw hero energy beam                                         */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawHeroWeapon(float targetx, float targetz, float x, float y,
		    float z, float * color, float height, float bottom)
    {
    float vertices[4][3];
    float colors[4][3];
    float rad;
    float directionX, directionZ;
    float perpendicularX, perpendicularZ;
    float sourceX, sourceZ;
    float midPoint;
    float sourceRadius;
    float sourceWidth;
    float targetRadius;
    float verticalSourceRadius;
    float verticalTargetRadius;
    GLboolean backNow;
    GLint shadeNow;
    int vertex;
    
    glGetIntegerv(GL_SHADE_MODEL, &shadeNow);
    backNow = glIsEnabled(GL_CULL_FACE);
    
    directionX = x - targetx;
    directionZ = z - targetz;
    rad = sqrt(directionX * directionX + directionZ * directionZ);
    
    if (rad == 0)
	rad = NEARZERO;
    directionX /= rad;
    directionZ /= rad;
    perpendicularX = -directionZ;
    perpendicularZ = directionX;
    sourceWidth = effectScalar("hero-angle-radius");
    sourceRadius = effectScalar("hero-source-radius");
    targetRadius = effectScalar("hero-target-radius");
    verticalSourceRadius = effectScalar("hero-vertical-source-radius");
    verticalTargetRadius = effectScalar("hero-vertical-target-radius");
    
    midPoint = 0.5 * (height - bottom) + bottom;
    sourceX = x - directionX * sourceRadius;
    sourceZ = z - directionZ * sourceRadius;
    
    glShadeModel(GL_SMOOTH);
    glDisable(GL_CULL_FACE);

    vertices[0][0] = sourceX + perpendicularX * sourceWidth;
    vertices[0][1] = y;
    vertices[0][2] = sourceZ + perpendicularZ * sourceWidth;
    vertices[1][0] = sourceX - perpendicularX * sourceWidth;
    vertices[1][1] = y;
    vertices[1][2] = sourceZ - perpendicularZ * sourceWidth;
    vertices[2][0] = targetx - targetRadius;
    vertices[2][1] = midPoint;
    vertices[2][2] = targetz - targetRadius;
    vertices[3][0] = targetx + targetRadius;
    vertices[3][1] = midPoint;
    vertices[3][2] = targetz + targetRadius;
    for (vertex = 0; vertex < 4; vertex++)
	memcpy(colors[vertex], vertex < 2 ? color : colorwhite,
	       3 * sizeof(float));
    dynamicDrawQuad3(vertices, colors);

    /* make beam viewable from side */

    vertices[0][0] = sourceX;
    vertices[0][1] = y + verticalSourceRadius;
    vertices[0][2] = sourceZ;
    vertices[1][0] = vertices[0][0];
    vertices[1][1] = y - verticalSourceRadius;
    vertices[1][2] = vertices[0][2];
    vertices[2][0] = targetx;
    vertices[2][1] = midPoint - verticalTargetRadius;
    vertices[2][2] = targetz;
    vertices[3][0] = targetx;
    vertices[3][1] = midPoint + verticalTargetRadius;
    vertices[3][2] = targetz;
    dynamicDrawQuad3(vertices, colors);
	
    if (backNow)
	glEnable(GL_CULL_FACE);
    else
	glDisable(GL_CULL_FACE);
	
    glShadeModel(shadeNow);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw explosions                                               */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void drawBoomFrame(int lowDetail, int frame)
    {
    const struct AssetManifestModel *model;
    struct AssetPose *pose;
    int variant;

    variant = lowDetail ? 1 : 0;
    model = externalBoomModels[variant];
    pose = &externalBoomPoses[variant];
    if (assetPoseSample(&model->model, externalBoomClips[variant],
                        (float)frame, externalBoomLoops[variant], pose))
        assetModelDraw(&model->model, pose);
    }

void drawBooms(struct boom * boomers, int detail)
    {
    const struct AssetValue *fade;
    struct boom  * tempboom;
    struct boom b;

    fade = requiredAnimationValue("boom-fade", 2);

    if (detail >= 2)
	glEnable(GL_BLEND);

    glColor3fv(coloryellow);

    for(tempboom = boomers->next;tempboom != NULL;tempboom = tempboom->next)
	{
	b = *tempboom;
		
	if (detail >= 2)
	    {
	    coloryellow[3] = (fade->components[0] - b.count) *
	        fade->components[1];
	    glColor4fv(coloryellow);
	    }
	
	glPushMatrix();
	    glTranslatef(b.x, b.y, b.z);

	    drawBoomFrame(detail == 0, b.count);
	glPopMatrix();
	}
	
    if (detail >= 2)
	glDisable(GL_BLEND);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw googelon/Flutter's energy beam                           */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawBeam(float x, float z, int horz, int vert,
	const struct EntityArchetype *archetype, int detail)
    {
    float loc1[3],  loc2[3],  loc3[3];
    float c[4] = {0, 0, 0, 0};
    const float *col;
    float R;
    float beamRange;
    float beamHeight;
    float beamForward;
    float directionX;
    float directionZ;
    float jitter;
    const struct AssetValue *beamLines;
    const struct AssetValue *beamGreen;
    register int i, nlines;
    GLfloat lineWidthNow;

    GLint shadeNow;

    glGetIntegerv(GL_SHADE_MODEL, &shadeNow);

    glGetFloatv(GL_LINE_WIDTH, &lineWidthNow);
    beamRange = effectScalar("monster-beam-range");
    beamHeight = effectScalar("monster-beam-height");
    beamForward = 0.0f;
    if (archetype != NULL)
	{
	beamRange = archetype->weaponRange;
	beamHeight = archetype->verticalOffset + archetype->muzzleHeight;
	beamForward = archetype->muzzleForward;
	}
    jitter = effectScalar("monster-beam-jitter");
    beamLines = requiredEffectValue("monster-beam-lines", 3);
    beamGreen = requiredEffectValue("monster-beam-green", 2);
    
    col = paletteColor(archetype == NULL ? "blue" :
	               archetype->weaponColor);

	    
    /**********************************/
    /* draw beam                      */
    /**********************************/

    /* if line widths are integral Mesa chokes in a large window */
    glLineWidth(effectScalar("monster-beam-line-width"));

    if (vert < 175)
	R = beamRange;
    else
	R = beamHeight / (cos((900 - vert) * BIG_DEG_TO_RAD));

    if (R < 0)
	R = 0;
    if (R > beamRange)
	R = beamRange;

    if (vert < 0)
	loc3[1] = PLANEY + beamHeight +
	    R * sin(fabs(vert * BIG_DEG_TO_RAD));
    else
	loc3[1] = PLANEY + beamHeight -
	    R * sin(fabs(vert * BIG_DEG_TO_RAD));

    directionX = -cos((900 + horz) * BIG_DEG_TO_RAD);
    directionZ = sin((900 + horz) * BIG_DEG_TO_RAD);
    loc1[0] = x + beamForward * directionX;
    loc1[2] = z + beamForward * directionZ;
    loc1[1] = PLANEY + beamHeight;

/* this doesnt really work ... beam has xz value of 4 no matter what y is (> plane)
 * so beam will be longer than 4 as it gets higher up in the sky
 */
 
    loc3[0] = loc1[0] + R * directionX;
    loc3[2] = loc1[2] + R * directionZ;

    if (loc3[1] < PLANEY)
	loc3[1] = PLANEY;

    c[0] = col[0] * effectScalar("monster-beam-primary");
    c[1] = col[1] * effectScalar("monster-beam-primary");
    c[2] = col[2] * effectScalar("monster-beam-primary");
    c[3] = col[3];

    switch (detail)
	{
	case -1:
	case 0:	nlines = (int)beamLines->components[0];
		break;
		
	case 1:	nlines = (int)beamLines->components[1];
		glShadeModel(GL_SMOOTH);
		break;
		
	case 2:	nlines = (int)beamLines->components[2];
		glShadeModel(GL_SMOOTH);
		break;

	default:	nlines = (int)beamLines->components[0];
		break;
	}


    for (i = 0; i < nlines; i++)
	{
	loc2[0] = loc3[0] + randy(jitter);
	loc2[1] = loc3[1] + randy(jitter);
	loc2[2] = loc3[2] + randy(jitter);

	c[1] = col[1] * effectScalar("monster-beam-primary") +
	       beamGreen->components[0] + randy(beamGreen->components[1]);
	
	dynamicDrawLine3(loc2, loc1, c, (float *)col);
	}
	
    glShadeModel(shadeNow);

    if (detail > 0)
	{
	/**********************************/
	/* shadow of beam                 */
	/**********************************/

	loc1[1] = SHADOWS;
	loc3[1] = SHADOWS;

	dynamicDrawLine3(loc1, loc3, colorblack, colorblack);
	}

    glLineWidth(lineWidthNow);
    }


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw roads on the battlefield                                 */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawRoads(struct road * roads, float xshift, float zshift,
				int itsChristmas, int view, int detail)
    {
    register int i;
    float displaySize;
    int ropCounter;
    int roadType;

    float c, d;
    float cprime,  dprime;

    GLfloat lineWidthNow;
    glGetFloatv(GL_LINE_WIDTH, &lineWidthNow);
    

	
    /***********************************/
    /* Draw 'motion' dots on the plane */
    /***********************************/

    if (view != MAPVIEW)
	{
	 c = xshift - (int) xshift;
	 d = zshift - (int) zshift;
    
	if (itsChristmas)
	    glColor3fv(colorblack);
	else
	    glColor3fv(coloryellow);
	    
	if (c > 0)
	    cprime = c-1;
	else
	    cprime = c;
	
	if (d > 0)
	    dprime = d-1;
	else
	    dprime = d;
    
	/* if line widths are integral Mesa chokes in a large window */
	glLineWidth(1.01);
	
	glPushMatrix();
	    glTranslatef(cprime, 0, dprime);
	    if (detail == 0)
		glCallList(planeDotsObjD0);
	    else
		glCallList(planeDotsObj);
	glPopMatrix();
	}


    if (view == MAPVIEW)
	displaySize = 50;
    else
	displaySize = PLANESIZE+0.25;


    if (roads == NULL)
	{
	glLineWidth(lineWidthNow);
	return;
	}

    /***********************************/
    /* find all the roads on the plane */
    /***********************************/

    ropCounter = 0;
    for(i=0;roads[i].type != NULLROAD;i++)
	{
	if ((fabs(roads[i].x+xshift) <= displaySize) && (fabs(roads[i].y+zshift) <= displaySize))
	    {
	    roadsOnPlane[ropCounter] = roads[i];
	    ropCounter++;
	    
	    if (ropCounter >= MAXROADSONPLANE)
		ropCounter = MAXROADSONPLANE - 1;
	    }
	}

    /********************/
    /* Draw black roads */
    /********************/

    if (ropCounter > 0)
	{
	if (detail >= 2)
	    {
	    glColor3fv(colorwhite);
	    glCallList(roadtex);
	    glEnable(GL_TEXTURE_2D);
	    }
	else
	    glColor3fv(colorblack);

	for(i=0; i < ropCounter; i++)
	    if (roadsOnPlane[i].type >= 0 && roadsOnPlane[i].type < 3)
		{
		roadType = (int)roadsOnPlane[i].type;
		glPushMatrix();
		glTranslatef(roadsOnPlane[i].x + xshift, 0.0f,
		             roadsOnPlane[i].y + zshift);
		assetModelDrawNode(&externalRoadModels[roadType]->model,
		                   requiredNode(externalRoadModels[roadType],
		                                "surface"), NULL);
		glPopMatrix();
		}
	if (detail >= 2)
	    glDisable(GL_TEXTURE_2D);
	}

    /*******************/
    /* Draw grey roads */
    /*******************/

    if (ropCounter > 0)
	{
	glColor3fv(colorgrey4);

	for(i=0; i < ropCounter; i++)
	    if (roadsOnPlane[i].type >= 3)
		{
		glPushMatrix();
		glTranslatef(roadsOnPlane[i].x + xshift, 0.0f,
		             roadsOnPlane[i].y + zshift);
		assetModelDrawNode(&externalRoadModels[3]->model,
		                   requiredNode(externalRoadModels[3], "surface"),
		                   NULL);
		glPopMatrix();
		}
	}

    if (detail != 0)
	{
	/**************************/
	/* draw the yellow lines  */
	/**************************/

	for(i=0; i < ropCounter; i++)
	    if (roadsOnPlane[i].type >= 0 && roadsOnPlane[i].type < 3)
		{
		roadType = (int)roadsOnPlane[i].type;
		glPushMatrix();
		glTranslatef(roadsOnPlane[i].x + xshift, 0.0f,
		             roadsOnPlane[i].y + zshift);
		assetModelDrawNode(&externalRoadModels[roadType]->model,
		                   requiredNode(externalRoadModels[roadType],
		                                "marker"), NULL);
		glPopMatrix();
		}
	}

    glLineWidth(lineWidthNow);
    }
    
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw projectiles in flight                                    */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawProjectiles(struct projectile * allProjs, int counter, 
    struct fireType * fires, int fireCounter,
		    int itsChristmas, int detail,
		    const struct monsterInfo *viewer, int currentView)
    {
    struct projectile *temp;
    float col[4] = {1.0, 0, 0, 0};
    float v[3], v2[3];
    float x, y, z;
    register int i;
    int beamCounter;
    int projectileVariant;
    int special;
    float scarDistance;
    float heroScale;
    float plasmaScale;
    float standardScale;

    static float beamhitx[MAXBEAMS], beamhitz[MAXBEAMS];

    GLfloat lineWidthNow;
    glGetFloatv(GL_LINE_WIDTH, &lineWidthNow);

    projectileVariant = (detail > 0 ? 1 : 0) + (itsChristmas ? 2 : 0);
    standardScale = effectScalar("projectile-standard-scale");
    plasmaScale = effectScalar("projectile-plasma-scale");
    heroScale = effectScalar("projectile-hero-scale");
    if (!assetPoseSample(&externalProjectileModels[projectileVariant]->model,
			externalProjectileClips[projectileVariant],
			(float)counter,
			externalProjectileLoops[projectileVariant],
			&externalProjectilePoses[projectileVariant]))
	return;
    for (special = 0; special < 2; special++)
        if (!assetPoseSample(
                &externalSpecialProjectileModels[special]->model,
                externalSpecialProjectileClips[special], (float)counter,
                externalSpecialProjectileLoops[special],
                &externalSpecialProjectilePoses[special]))
            return;

    beamCounter = 0;
    for (temp = allProjs->next;temp != NULL;temp = temp->next)
	if ((viewer == NULL ||
	     (fabs(temp->x - viewer->x) <= PLANESIZE &&
	      fabs(temp->z - viewer->z) <= PLANESIZE)) &&
	    !(currentView == FIRSTPERSONVIEW && viewer != NULL &&
	      ((temp->mine == viewer && temp->count < ARMCOUNT) ||
	       (fabs(temp->x - viewer->x) <= viewer->width &&
	        fabs(temp->z - viewer->z) <= viewer->width &&
	        temp->y >= viewer->bottom && temp->y <= viewer->height))))
	    {
	    x = temp->x;
	    y = temp->y;
	    z = temp->z;
    
	    /**********************************/
	    /* draw bomb                      */
	    /**********************************/
		
	    switch(temp->type){
		case PROJTANK:
		case PROJROCKET:glPushMatrix();		/* white cube shells */
				    glTranslatef(x,  y,  z);
				    glScalef(standardScale, standardScale,
				             standardScale);
				    assetModelDraw(
					&externalProjectileModels[projectileVariant]->model,
					&externalProjectilePoses[projectileVariant]);
				glPopMatrix();
				if (detail > 0)
				    {
				    glColor4fv(colorblack);
				    shadow (x, z, 0.05, 0.05);
				    }
				break;
				
		case PROJTECHS: glPushMatrix();		/* techs shell */
				    glTranslatef(x,  y,  z);
				    glScalef(plasmaScale, plasmaScale,
				             plasmaScale);
				    assetModelDraw(
				        &externalSpecialProjectileModels[0]->model,
				        &externalSpecialProjectilePoses[0]);
				glPopMatrix();
				if (detail > 0)
				    {
				    glColor4fv(colorblack);
				    shadow (x, z, 0.05, 0.05);
				    }
				break;
				
		case PROJFIRE:  break;			/* flames */
								
		case PROJSCAR:  beamhitx[beamCounter] = x;	/* monster beam scar */
				beamhitz[beamCounter] = z;
				beamCounter += 1;
				if (beamCounter >= MAXBEAMS)
				    beamCounter -= 1;
				break;
				    
		case PROJWATER: glPushMatrix();		/* water */
				    glTranslatef(x,  y,  z);
				    glCallList(watershellObj);
				glPopMatrix();
				break;

		case PROJHERO:  glPushMatrix();
				    glTranslatef(x,  y,  z);
				    glScalef(heroScale, heroScale, heroScale);
				    assetModelDraw(
				        &externalSpecialProjectileModels[1]->model,
				        &externalSpecialProjectilePoses[1]);
				glPopMatrix();
				if (detail > 0)
				    {
				    glColor4fv(colorblack);
				    shadow (x, z, 0.05, 0.05);
				    }
				break;
    
		default:	showError("Bogus Projectile in Projectile List! (drawProjectiles)");
				break;
		}
	    }


    /**********************************/
    /* draw fires                     */
    /**********************************/
	    
    for (i=0;i<fireCounter;i++)
	{
	col[1] = fires[i].col;
	if ((viewer == NULL) ||
	    ((fabs(fires[i].x - viewer->x) <= PLANESIZE) &&
	     (fabs(fires[i].z - viewer->z) <= PLANESIZE)))
	    {
	    glColor4fv(col);
	    glPushMatrix();
	    glTranslatef(fires[i].x, PLANEY + fires[i].height, fires[i].z);
	    glScalef(fires[i].xwid, fires[i].height, fires[i].zwid);
	    glCallList(fireModel);
	    glPopMatrix();
	    }
	}

    /**********************************/
    /* draw scar from monster beam    */
    /**********************************/
    
    /* if line widths are integral Mesa chokes in a large window */
    glLineWidth(effectScalar("beam-scar-line-width"));
    scarDistance = effectScalar("beam-scar-link-distance");

    v[1] = v2[1] = SHADOWS;
    
    v2[0] = beamhitx[0];
    v2[2] = beamhitz[0];
    
    for(i=1; i<beamCounter; i++)
	{
	v[0] = beamhitx[i];
	v[2] = beamhitz[i];
	
	if ((fabs(v[0] - v2[0]) < scarDistance) &&
	    (fabs(v[2] - v2[2]) < scarDistance))
	    {
	    dynamicDrawLine3(v, v2, colorblack, colorblack);
	    }
	    
	v2[0] = v[0];
	v2[2] = v[2];	
	}

    glLineWidth(lineWidthNow);
    }


/*
#define J_SPACE 32
#define J_DASH 34

#define J_1 50
#define J_2 51
#define J_3 52
#define J_4 53
#define J_5 54
#define J_6 55
#define J_7 56
#define J_8 57
#define J_9 58


#define J_H_A 65

#define J_K_A 145
#define J_K_I 146
#define J_K_U 147
#define J_K_E 148
#define J_K_O 149

#define J_K_KA 150
#define J_K_KI 151
#define J_K_KU 152
#define J_K_KE 153
#define J_K_KO 154

#define J_K_SA 155
#define J_K_SI 156
#define J_K_SU 157
#define J_K_SE 158
#define J_K_SO 159

#define J_K_TA 160
#define J_K_TI 161
#define J_K_TU 162
#define J_K_TE 163
#define J_K_TO 164

#define J_K_NA 165
#define J_K_NI 166
#define J_K_NU 167
#define J_K_NE 168
#define J_K_NO 169

#define J_K_HA 170
#define J_K_HI 171
#define J_K_HU 172
#define J_K_HE 173
#define J_K_HO 174

#define J_K_MA 175
#define J_K_MI 176
#define J_K_MU 177
#define J_K_ME 178
#define J_K_MO 179

#define J_K_YA 180
#define J_K_YU 182
#define J_K_YO 184

#define J_K_RA 185
#define J_K_RI 186
#define J_K_RU 187
#define J_K_RE 188
#define J_K_RO 189

#define J_K_GA 200
#define J_K_GI 201
#define J_K_GU 202
#define J_K_GE 203
#define J_K_GO 204

#define J_K_ZA 205
#define J_K_ZI 206
#define J_K_ZU 207
#define J_K_ZE 208
#define J_K_ZO 209

#define J_K_DA 210
#define J_K_DE 213
#define J_K_DO 214

#define J_K_BA 215
#define J_K_BI 216
#define J_K_BU 217
#define J_K_BE 218
#define J_K_BO 219

#define J_K_PA 220
#define J_K_PI 221
#define J_K_PU 222
#define J_K_PE 223
#define J_K_PO 224


    char energyJapanese[]	= {J_K_E, J_K_NE, J_K_RU, J_K_GI, 0};
    char googelonJapanese[]	= {J_K_GU, J_K_GE, J_K_RO, J_SPACE, J_6, 0};
    char techsJapanese[]	= {J_K_TE, J_K_TU, J_K_KU, J_K_SU, J_SPACE, J_7, 0};
    char flutterJapanese[]  	= {J_K_HU, J_K_RA, J_K_TU, J_K_TA, J_SPACE, J_9, 0};
    char vapourJapanese[]   	= {J_K_ZA, J_DASH, J_K_BE, J_DASH, J_K_PA, J_DASH, J_SPACE, J_8, 0};
*/
