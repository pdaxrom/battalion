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
/* battalion.c  v 1.3a                                                 */
/* main routines for battalion                                         */
/***********************************************************************/
/*                                                */
/* code within '#ifdef SOLARIS' by Johan Hagman   */
/*                                                */
/**************************************************/

#include "battalion.h"
#include "camera_view.h"
#include "entity_wreck.h"
#include "map.h"
#include "map_package.h"

/*****************/
/* network stuff */
/*****************/

int destroyedMapObjectHistoryCount;
int *destroyedMapObjectHistory;

int destroyedMapObjectFrameCount;
int *destroyedMapObjectFrame;
static size_t destroyedMapObjectCapacity;
int currentActorNumber;
struct WorldEnvironment worldEnvironment;
int shadowsEnabled;

static int ensureDestroyedMapObjectCapacity(size_t required)
    {
    int *expanded;

    if (required <= destroyedMapObjectCapacity)
        return 1;
    expanded = (int *)realloc(
        destroyedMapObjectHistory, required * sizeof(*expanded));
    if (expanded == NULL)
        return 0;
    destroyedMapObjectHistory = expanded;
    expanded = (int *)realloc(
        destroyedMapObjectFrame, required * sizeof(*expanded));
    if (expanded == NULL)
        return 0;
    destroyedMapObjectFrame = expanded;
    destroyedMapObjectCapacity = required;
    return 1;
    }

static void recordDestroyedMapObject(int number)
    {
    int index;

    for (index = 0; index < destroyedMapObjectHistoryCount; index++)
        if (destroyedMapObjectHistory[index] == number)
            return;
    if ((size_t)destroyedMapObjectHistoryCount >=
        destroyedMapObjectCapacity)
        return;
    destroyedMapObjectHistory[destroyedMapObjectHistoryCount++] = number;
    if ((size_t)destroyedMapObjectFrameCount <
        destroyedMapObjectCapacity)
        destroyedMapObjectFrame[destroyedMapObjectFrameCount++] = number;
    }

/*********/
/* times */
/*********/

time_t beg, now;
time_t timePaused, time3d, timeSound, timeMusic, timeDetail;

/**************/
/* indicators */
/**************/

int frames;

int moreThanOne;

int viewW,  viewH;

int alone;
int oldview,  view;
int showOptions;
int paused;
int noSound;
int no3D;
int mode3D;
int mode;
int backdrop;
int itsChristmas;
int mapHeight;
int lod;

/* it would be nice to dynamically get these ... */
int XMAXSCREEN = 1920;

#ifdef SOLARIS
 int YMAXSCREEN = 1024;
#else
 int YMAXSCREEN = 1080;
#endif

/************/
/* counters */
/************/

int mainCounter;
int totalCounter;
int firingDelay;
int arrivalTime;
int showframes; /* also used to filter out repetitive sounds */
float accuracy;

int mapObjectRuntimeId;

/****************/
/* window stuff */
/****************/

long windowOriginX, windowOriginY, windowSizeX, windowSizeY;
long monitorType;
long windowWide, windowTall, windowLeft, windowTop;
GLuint viewL, viewR;
long zmax;

float lastActorY, lastActorTheta, lastActorRad;

int pointerGrab;

/*********/
/* lists */
/*********/

struct runtimeProjectile * runtimeProjectileList;
struct runtimeExplosion * runtimeExplosionList;
struct runtimeActor * actorlist;
struct runtimeActor * wrecklist;
struct runtimeMapObject * mapObjectList;
struct runtimePlayer playerEntityState;

static struct BattalionMapPackage battlefieldMap;
static struct EntityCatalog gameEntities;
static struct FactionCatalog gameFactions;
static char requestedPlayerEntity[ENTITY_NAME_SIZE];
static int requestedView = -1;
static int requestedStereoLayout = CAMERA_STEREO_OFF;
static const struct EntityArchetype *playerEntityArchetype;
static size_t playerEntitySelection;

const struct EntityArchetype *selectedPlayerArchetype(void)
    {
    return playerEntityArchetype;
    }

size_t playerEntityOptionCount(void)
    {
    return entityCatalogControllableCount(&gameEntities);
    }

size_t selectedPlayerArchetypeNumber(void)
    {
    return playerEntitySelection;
    }

const struct EntityArchetype *playerEntityOptionAt(size_t ordinal)
    {
    return entityCatalogControllableAt(&gameEntities, ordinal);
    }

static int setPlayerEntityArchetype(
    const struct EntityArchetype *archetype)
    {
    const struct EntityArchetype *candidate;
    size_t count;
    size_t ordinal;

    if (archetype == NULL || !archetype->controllable)
        return 0;
    count = entityCatalogControllableCount(&gameEntities);
    for (ordinal = 0; ordinal < count; ordinal++)
        {
        candidate = entityCatalogControllableAt(&gameEntities, ordinal);
        if (candidate == archetype)
            {
            playerEntityArchetype = candidate;
            playerEntitySelection = ordinal;
            playerEntityState.dataArchetype = candidate;
            return 1;
            }
        }
    return 0;
    }

static void stepPlayerEntitySelection(int direction)
    {
    size_t count;

    count = entityCatalogControllableCount(&gameEntities);
    if (count == 0)
        return;
    if (direction < 0)
        playerEntitySelection = playerEntitySelection == 0 ?
                                count - 1 : playerEntitySelection - 1;
    else
        playerEntitySelection = (playerEntitySelection + 1) % count;
    playerEntityArchetype = entityCatalogControllableAt(
        &gameEntities, playerEntitySelection);
    playerEntityState.dataArchetype = playerEntityArchetype;
    }

static const struct EntityArchetype *playerArchetype(
    const struct runtimePlayer *player)
    {
    if (player == NULL)
        return NULL;
    return player->dataArchetype;
    }

const struct EntityArchetype *runtimePlayerArchetype(
    const struct runtimePlayer *player)
    {
    return playerArchetype(player);
    }

static int playerHasSolidCollision(const struct runtimePlayer *player)
    {
    const struct EntityArchetype *archetype;

    archetype = playerArchetype(player);
    return archetype == NULL ||
           archetype->collisionMode == ENTITY_COLLISION_SOLID;
    }

static float actorCollisionRadius(const struct runtimeActor *actor)
    {
    if (actor != NULL && actor->dataArchetype != NULL &&
        actor->dataArchetype->collisionMode == ENTITY_COLLISION_SOLID)
        return entityArchetypeCollisionRadius(actor->dataArchetype);
    return 0.0f;
    }

static float actorCollisionBottom(const struct runtimeActor *actor)
    {
    if (actor != NULL && actor->dataArchetype != NULL)
        return actor->y + entityArchetypeCollisionBottomOffset(
            actor->dataArchetype);
    return actor == NULL ? PLANEY : actor->y;
    }

static float actorCollisionTop(const struct runtimeActor *actor)
    {
    if (actor != NULL && actor->dataArchetype != NULL)
        return actor->y + entityArchetypeCollisionTopOffset(
            actor->dataArchetype);
    return actor == NULL ? PLANEY : actor->y;
    }

static float mapObjectCollisionRadius(
    const struct runtimeMapObject *object)
    {
    if (object == NULL || object->dataArchetype == NULL ||
        object->dataArchetype->collisionMode == ENTITY_COLLISION_NONE)
        return 0.0f;
    return entityArchetypeCollisionRadius(object->dataArchetype) *
           object->instanceScale;
    }

static float mapObjectCollisionTop(const struct runtimeMapObject *object)
    {
    return object->y + object->height;
    }

/*********/
/* files */
/*********/

char dataPath[MAXPATH];
char fullPath[MAXPATH];
char scorefullPath[MAXPATH];

static int selectAssetRoot(const char *root)
    {
    char candidate[MAXPATH];
    char manifestPath[MAXPATH];
    FILE *manifestFile;
    size_t length;

    if (root == NULL)
        return 0;
    length = strlen(root);
    if (length == 0 || length + strlen("/manifest.txt") + 1 >
        sizeof(candidate))
        return 0;
    strcpy(candidate, root);
    if (candidate[length - 1] != '/')
        strcat(candidate, "/");
    if (strlen(candidate) + strlen("manifest.txt") + 1 >
        sizeof(manifestPath))
        return 0;
    strcpy(manifestPath, candidate);
    strcat(manifestPath, "manifest.txt");
    manifestFile = fopen(manifestPath, "rb");
    if (manifestFile == NULL)
        return 0;
    fclose(manifestFile);
    strcpy(dataPath, candidate);
    return 1;
    }

static void setGameEntityError(char *error, size_t errorSize,
                               const char *message)
    {
    if (errorSize == 0)
        return;
    strncpy(error, message, errorSize - 1);
    error[errorSize - 1] = '\0';
    }

static int loadGameEntityCatalog(char *error, size_t errorSize)
    {
    const char *catalogPath;
    const char *factionPath;
    const struct EntityArchetype *archetype;
    const struct EntityArchetype *routeArchetype;
    const struct EntityArchetype *safeZoneArchetype;
    const struct BattalionMapObject *mapObject;
    const struct BattalionMapObject *routeObject;
    const struct BattalionMapObject *safeZoneObject;
    const char *resolvedFaction;
    struct EntityArchetype *mutableArchetype;
    float modelHeight;
    float modelMinimum[3];
    float modelMaximum[3];
    float modelRadiusX;
    float modelRadiusZ;
    char boundsError[160];
    size_t index;
    size_t mapSearch;
    size_t sensorIndex;

    factionPath = assetFilePath("game-factions");
    factionCatalogInitialize(&gameFactions);
    if (!factionCatalogLoad(dataPath, factionPath, &gameFactions,
                            error, errorSize))
        return 0;
    catalogPath = assetFilePath("game-entities");
    entityCatalogInitialize(&gameEntities);
    if (!entityCatalogLoad(dataPath, catalogPath, &gameEntities,
                           error, errorSize))
        return 0;
    if (!assetEntityCatalogAvailable(&gameEntities, error, errorSize))
        return 0;
    for (index = 0; index < gameEntities.count; index++)
        {
        mutableArchetype = &gameEntities.entries[index];
        if (entityArchetypeIsActor(mutableArchetype) &&
            factionCatalogFind(&gameFactions,
                               mutableArchetype->faction) == NULL)
            {
            setGameEntityError(error, errorSize,
                               "Entity references an unknown faction");
            return 0;
            }
        for (sensorIndex = 0;
             sensorIndex < mutableArchetype->sensorCount; sensorIndex++)
            if (strcmp(mutableArchetype->sensors[sensorIndex].factionFilter,
                       "any") != 0 &&
                strcmp(mutableArchetype->sensors[sensorIndex].factionFilter,
                       "hostile") != 0 &&
                strcmp(mutableArchetype->sensors[sensorIndex].factionFilter,
                       "ally") != 0 &&
                factionCatalogFind(
                    &gameFactions,
                    mutableArchetype->sensors[sensorIndex].factionFilter) ==
                    NULL)
                {
                setGameEntityError(error, errorSize,
                                   "Entity sensor uses an unknown faction");
                return 0;
                }
        if (mutableArchetype->heightSpecified &&
            (!assetModelUnscaledHeight(mutableArchetype, &modelHeight) ||
             !entityArchetypeResolveScale(mutableArchetype, modelHeight)))
            {
            setGameEntityError(error, errorSize,
                               "Could not resolve entity height from model");
            return 0;
            }
        if (mutableArchetype->renderStyle == ENTITY_RENDER_MODEL &&
            !assetModelUnscaledBounds(mutableArchetype,
                                      modelMinimum, modelMaximum))
            {
            sprintf(boundsError, "Could not resolve model bounds for '%s'",
                    mutableArchetype->id);
            setGameEntityError(error, errorSize, boundsError);
            return 0;
            }
        if (mutableArchetype->renderStyle == ENTITY_RENDER_MODEL)
            {
            mutableArchetype->modelMinimumY =
                modelMinimum[1] * mutableArchetype->scale;
            mutableArchetype->modelMaximumY =
                modelMaximum[1] * mutableArchetype->scale;
            mutableArchetype->modelGroundOffset =
                mutableArchetype->grounding == ENTITY_GROUNDING_MODEL_MIN ?
                -mutableArchetype->modelMinimumY : 0.0f;
            modelRadiusX = (float)fabs((double)modelMinimum[0]);
            if ((float)fabs((double)modelMaximum[0]) > modelRadiusX)
                modelRadiusX = (float)fabs((double)modelMaximum[0]);
            modelRadiusZ = (float)fabs((double)modelMinimum[2]);
            if ((float)fabs((double)modelMaximum[2]) > modelRadiusZ)
                modelRadiusZ = (float)fabs((double)modelMaximum[2]);
            mutableArchetype->modelFootprintRadius =
                (float)sqrt((double)(modelRadiusX * modelRadiusX +
                                    modelRadiusZ * modelRadiusZ)) *
                mutableArchetype->scale;
            }
        }
    for (index = 0; index < battlefieldMap.objectCount; index++)
        {
        mapObject = &battlefieldMap.objects[index];
        archetype = entityCatalogFind(
            &gameEntities, mapObject->archetype);
        if (archetype == NULL)
            {
            setGameEntityError(error, errorSize,
                               "A map object has an unknown archetype");
            return 0;
            }
        if (entityArchetypeIsActor(archetype) &&
            mapObject->id[0] == '\0')
            {
            setGameEntityError(error, errorSize,
                               "A gameplay object has no instance id");
            return 0;
            }
        if (entityArchetypeIsActor(archetype))
            {
            resolvedFaction = mapObject->faction[0] != '\0' ?
                              mapObject->faction : archetype->faction;
            if (factionCatalogFind(&gameFactions, resolvedFaction) == NULL)
                {
                setGameEntityError(error, errorSize,
                                   "A map actor has an unknown faction");
                return 0;
                }
            if (mapObject->waypointCount > 0)
                {
                setGameEntityError(
                    error, errorSize,
                    "Actor patrol points must belong to a route object");
                return 0;
                }
            }
        if (archetype->kind == ENTITY_KIND_ROUTE &&
            (mapObject->id[0] == '\0' || mapObject->waypointCount < 2 ||
             mapObject->patrolRoute[0] != '\0'))
            {
            setGameEntityError(error, errorSize,
                               "A patrol route is invalid");
            return 0;
            }
        if (archetype->kind == ENTITY_KIND_ZONE &&
            mapObject->geometryCount < 3)
            {
            setGameEntityError(error, errorSize,
                               "A guard zone needs at least three points");
            return 0;
            }
        if (mapObject->patrolRoute[0] != '\0')
            {
            routeObject = NULL;
            for (mapSearch = 0; mapSearch < battlefieldMap.objectCount;
                 mapSearch++)
                if (strcmp(battlefieldMap.objects[mapSearch].id,
                           mapObject->patrolRoute) == 0)
                    routeObject = &battlefieldMap.objects[mapSearch];
            routeArchetype = routeObject == NULL ? NULL : entityCatalogFind(
                &gameEntities, routeObject->archetype);
            if (!entityArchetypeIsActor(archetype) || routeArchetype == NULL ||
                routeArchetype->kind != ENTITY_KIND_ROUTE ||
                routeObject->waypointCount < 2)
                {
                setGameEntityError(error, errorSize,
                                   "Actor references an invalid patrol route");
                return 0;
                }
            }
        if (mapObject->alarmResponse != ENTITY_ALARM_INHERIT)
            {
            if (!entityArchetypeIsActor(archetype) ||
                archetype->movement == ENTITY_MOVEMENT_STATIONARY)
                {
                setGameEntityError(
                    error, errorSize,
                    "Alarm response belongs to a moving actor");
                return 0;
                }
            if (mapObject->alarmResponse == ENTITY_ALARM_RETREAT ||
                mapObject->alarmResponse == ENTITY_ALARM_RANDOM)
                {
                safeZoneObject = NULL;
                for (mapSearch = 0;
                     mapSearch < battlefieldMap.objectCount; mapSearch++)
                    if (strcmp(battlefieldMap.objects[mapSearch].id,
                               mapObject->safeZone) == 0)
                        safeZoneObject = &battlefieldMap.objects[mapSearch];
                safeZoneArchetype = safeZoneObject == NULL ? NULL :
                    entityCatalogFind(&gameEntities,
                                      safeZoneObject->archetype);
                if (safeZoneArchetype == NULL ||
                    safeZoneArchetype->kind != ENTITY_KIND_ZONE)
                    {
                    setGameEntityError(
                        error, errorSize,
                        "Actor safe-zone is not an area zone");
                    return 0;
                    }
                }
            }
        }
    archetype = entityCatalogFind(&gameEntities, gameEntities.fallback);
    if (archetype == NULL || !setPlayerEntityArchetype(archetype))
        {
        archetype = entityCatalogControllableAt(&gameEntities, 0);
        if (!setPlayerEntityArchetype(archetype))
            {
            setGameEntityError(error, errorSize,
                               "Entity catalog has no controllable player");
            return 0;
            }
        }
    return 1;
    }

const struct EntityArchetype *gameEntityArchetype(const char *id)
    {
    return entityCatalogFind(&gameEntities, id);
    }

const struct FactionCatalog *runtimeFactionCatalog(void)
    {
    return &gameFactions;
    }

const char *runtimePlayerFaction(void)
    {
    return gameFactions.playerFaction[0] != '\0' ?
           gameFactions.playerFaction : "neutral";
    }

/*****************/
/* player stuff */
/*****************/

float offsetX; /* used to bank the active player archetype */
struct targetInfo * targets;


/*********************/
/* battlefield stuff */
/*********************/

GLfloat xrot, xsaverot, yrot, ysaverot;
static float overviewRotationPivotY;

int frameCount;
int outFrameCount;
time_t frameTime;

GLuint playerLookat;
GLuint overviewLookat;

struct playerInputState
    {
    int forward;
    int backward;
    int left;
    int right;
    int aimUp;
    int aimDown;
    int aimLeft;
    int aimRight;
    int fireMouse;
    int fireControlLeft;
    int fireControlRight;
    };

static struct playerInputState playerInput;
static int playerMoveActive;
static float playerMoveAngle;
static int playerBodyHeading;
static int playerProjectileTriggerLatched;
static int playerWeaponReloadRemaining;
static int showHelp;
static int showDemoMenu;
static int commandKeys[7];

static void clearPlayerInput(void);
static void setPointerCaptured(int);
static void inputFocusChanged(int);

float textLineWidth;

struct timeval startTime, endTime;

char playerName[16];
char playerHome[256];

struct score highScores[3];
int multipleHighScores;

struct runtimeProjectile * tproj,  *deadproj;
struct runtimeExplosion * tboom, *removedExplosion;

void * arena;

int runtimeExplosionSize;
int sizeActor;
int runtimeProjectileSize;

int netUp;
int client;

int biggestActorNumberSent;

int playerWeaponUsesReload(void)
    {
    if (playerEntityArchetype == NULL)
	return 0;
    return playerEntityArchetype->weapon == ENTITY_WEAPON_PROJECTILE;
    }

float playerWeaponReadiness(void)
    {
    float readiness;

    if (!playerWeaponUsesReload() ||
	playerEntityArchetype->fireInterval <= 0)
	return 1.0f;
    readiness = 1.0f -
	(float)playerWeaponReloadRemaining /
	(float)playerEntityArchetype->fireInterval;
    if (readiness < 0.0f)
	return 0.0f;
    if (readiness > 1.0f)
	return 1.0f;
    return readiness;
    }


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void goToHighDetail()
    {
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glEnable(GL_DEPTH_TEST);
    }

void goToLowDetail()
    {
    glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    glDisable(GL_DEPTH_TEST);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void updateScores(char *path, int score, int mode, char *name)
    {
    char player[MAXSTRING];
    char separator;
    FILE *file;
    int existing;
    int insertion;
    int index;

    if (mode == DEMOMODE)
        strcpy(player, "demo");
    else
        strcpy(player, name);
    file = fopen(path, "r");
    if (file == NULL)
        return;
    for (index = 0; index < 3; index++)
        if (fscanf(file, "%d%c%255s", &highScores[index].number,
                   &separator, highScores[index].name) != 3)
            {
            highScores[index].number = -1;
            highScores[index].name[0] = 0;
            }
    fclose(file);

    existing = -1;
    if (!multipleHighScores)
        for (index = 0; index < 3; index++)
            if (same(player, highScores[index].name))
                existing = index;
    if (existing >= 0 && highScores[existing].number >= score)
        return;
    if (existing >= 0)
        for (index = existing; index < 2; index++)
            highScores[index] = highScores[index + 1];

    insertion = 3;
    for (index = 0; index < 3; index++)
        if (score > highScores[index].number)
            {
            insertion = index;
            break;
            }
    if (insertion < 3)
        {
        for (index = 2; index > insertion; index--)
            highScores[index] = highScores[index - 1];
        highScores[insertion].number = score;
        strcpy(highScores[insertion].name, player);
        }

    file = fopen(path, "w");
    if (file == NULL)
        return;
    for (index = 0; index < 3; index++)
        fprintf(file, "%d %s\n", highScores[index].number,
                highScores[index].name[0] ? highScores[index].name : "-");
    fclose(file);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* determine whether an actor is blocked                          */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

struct ActorObstacleContext
    {
    struct runtimeActor *actor;
    struct runtimeActor *allActors;
    };

static int actorPositionBlocked(void *value, float x, float z)
    {
    struct ActorObstacleContext *context;
    struct runtimeActor *other;
    struct runtimeMapObject *object;
    float bottom;
    float radius;
    float top;

    context = (struct ActorObstacleContext *)value;
    if (context == NULL || context->actor == NULL ||
        context->allActors == NULL)
        return 0;
    radius = actorCollisionRadius(context->actor);
    if (radius <= 0.0f)
        return 0;
    bottom = actorCollisionBottom(context->actor);
    top = actorCollisionTop(context->actor);
    for (other = context->allActors->next; other != NULL;
         other = other->next)
        if (other != context->actor && other->damage > 0 &&
            actorCollisionRadius(other) > 0.0f &&
            worldCylindersOverlap(
                x, z, radius, bottom, top,
                other->x, other->z, actorCollisionRadius(other),
                actorCollisionBottom(other), actorCollisionTop(other)))
            return 1;
    for (object = mapObjectList->next; object != NULL;
         object = object->next)
        if (object->intact && mapObjectCollisionRadius(object) > 0.0f &&
            worldCylindersOverlap(
                x, z, radius, bottom, top,
                object->x, object->z, mapObjectCollisionRadius(object),
                object->y, mapObjectCollisionTop(object)))
            return 1;
    return 0;
    }

int checkForBlock(struct runtimeActor * tactor, float rad, float theta, float y)
    {
    struct ActorObstacleContext context;
    float nextX;
    float nextZ;

    (void)rad;
    (void)y;
    nextX = tactor->x + (float)sin((double)theta) * tactor->speed;
    nextZ = tactor->z + (float)cos((double)theta) * tactor->speed;
    context.actor = tactor;
    context.allActors = actorlist;
    return actorPositionBlocked(&context, nextX, nextZ);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* increase player's score based on kill                        */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void addScore(int points, struct runtimePlayer *m)
    {
    if (m != NULL && points > 0)
        m->score += points;
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* add a new explosion                                           */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void spawnExplosionInternal(const struct EntityArchetype *archetype,
                            float x, float y, float z,
                            struct runtimePlayer *m, int damaging)
    {
    struct runtimeExplosion * explosionNext;
    struct runtimeExplosion explosion;

    if (archetype == NULL || archetype->kind != ENTITY_KIND_EFFECT)
        return;
    explosionNext = (struct runtimeExplosion *) amalloc(runtimeExplosionSize, arena);

    if (explosionNext != NULL)
	{
	explosion.owner	= m;
	explosion.count	= 1;
	explosion.damaging = damaging;
	explosion.archetype = archetype;
	explosion.x	= x;

	if (y >= PLANEY)
	    explosion.y = y;
	else
	    explosion.y = PLANEY;

	explosion.z	    = z;
	explosion.next	    = runtimeExplosionList->next;
	*explosionNext	    = explosion;
	runtimeExplosionList->next   = explosionNext;
	}
    }

static void spawnConfiguredExplosion(const char *effectId,
                                     float x, float y, float z,
                                     struct runtimePlayer *m, int damaging)
    {
    const struct EntityArchetype *effect;

    effect = entityCatalogFind(&gameEntities, effectId);
    spawnExplosionInternal(effect, x, y, z, m, damaging);
    }

static void spawnDestructionEffects(
    const struct EntityArchetype *sourceArchetype,
    float x, float y, float z, float radius, float height,
    struct runtimePlayer *owner, int damaging)
    {
    const struct EntityArchetype *effect;
    int effectIndex;

    if (sourceArchetype == NULL ||
        sourceArchetype->destructionEffectCount <= 0)
        return;
    effect = entityCatalogFind(&gameEntities,
                               sourceArchetype->destructionEffect);
    for (effectIndex = 0;
         effectIndex < sourceArchetype->destructionEffectCount;
         effectIndex++)
        if (effectIndex == 0)
            spawnExplosionInternal(effect, x, y, z, owner, damaging);
        else
            spawnExplosionInternal(
                effect, x + randy(radius), y + randy(height * 0.5f),
                z + randy(radius), owner, 0);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* add a new projectile                                          */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void spawnProjectile(float x, float y, float z,
                   const struct EntityArchetype *archetype,
		   float xv, float yv, float zv, float accuracy,
                   struct runtimePlayer *m)
    {
    struct runtimeProjectile * temp;
    struct runtimeProjectile tempProj;

    if (archetype == NULL || archetype->kind != ENTITY_KIND_PROJECTILE)
        return;
    doSoundKeyAt(archetype->fireSound, x, y, z);

    temp = (struct runtimeProjectile *) amalloc(runtimeProjectileSize, arena);

    if (temp != NULL)
	{
	tempProj.count = 0;

	tempProj.owner = m;

	tempProj.x = x;
	tempProj.y = y;
	tempProj.z = z;

	tempProj.archetype = archetype;

	tempProj.xv = xv;
	tempProj.yv = yv;
	tempProj.zv = zv;

	if (accuracy != 0)
	    {
	    tempProj.xv += randy(5.0)/accuracy;
	    tempProj.yv += randy(5.0)/accuracy;
	    tempProj.zv += randy(5.0)/accuracy;
	    }


	tempProj.next = runtimeProjectileList->next;
	*temp = tempProj;
	runtimeProjectileList->next = temp;
	}
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* update all projectiles on the battlefield                     */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int projectilePathHitsActor(const struct runtimeActor *actor,
	float startX, float startY, float startZ,
	float endX, float endY, float endZ)
    {
    float distance;
    float dx;
    float dy;
    float dz;
    float amount;
    int step;
    int steps;

    dx = endX - startX;
    dy = endY - startY;
    dz = endZ - startZ;
    distance = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));
    steps = (int)(distance / 0.1f) + 1;
    for (step = 1; step <= steps; step++)
	{
	amount = (float)step / (float)steps;
	if (actorHit(actor,
		startX + dx * amount,
		startY + dy * amount,
		startZ + dz * amount))
	    return 1;
	}
    return 0;
    }

static int projectilePathHitsMapObject(const struct runtimeMapObject *structure,
	float startX, float startY, float startZ,
	float endX, float endY, float endZ)
    {
    float distance;
    float dx;
    float dy;
    float dz;
    float x;
    float y;
    float z;
    float amount;
    int step;
    int steps;

    if (!structure->intact || structure->dataArchetype == NULL ||
        structure->dataArchetype->collisionMode == ENTITY_COLLISION_NONE)
	return 0;
    dx = endX - startX;
    dy = endY - startY;
    dz = endZ - startZ;
    distance = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));
    steps = (int)(distance / 0.1f) + 1;
    for (step = 1; step <= steps; step++)
	{
	amount = (float)step / (float)steps;
	x = startX + dx * amount;
	y = startY + dy * amount;
	z = startZ + dz * amount;
	if ((fabs(structure->x - x) <
	     mapObjectCollisionRadius(structure)) &&
	    (fabs(structure->z - z) <
	     mapObjectCollisionRadius(structure)) &&
	    y >= structure->y && y <= structure->y + structure->height)
	    return 1;
	}
    return 0;
    }

static int dataEntityLineOfSight(const struct runtimeActor *source,
	float targetX, float targetY, float targetZ)
    {
    const struct runtimeMapObject *structure;
    float sourceY;

    sourceY = source->y + source->dataArchetype->collisionHeight * 0.5f;
    for (structure = mapObjectList->next; structure != NULL;
	 structure = structure->next)
	{
	/* A structure containing either endpoint is not an intervening
	   occluder.  The existing projectile footprint is also the established
	   structure footprint used by perception. */
	if ((fabs(structure->x - source->x) <
	     mapObjectCollisionRadius(structure) &&
	     fabs(structure->z - source->z) <
	     mapObjectCollisionRadius(structure)) ||
	    (fabs(structure->x - targetX) <
	     mapObjectCollisionRadius(structure) &&
	     fabs(structure->z - targetZ) <
	     mapObjectCollisionRadius(structure)))
	    continue;
	if (projectilePathHitsMapObject(structure,
		source->x, sourceY, source->z,
		targetX, targetY, targetZ))
	    return 0;
	}
    return 1;
    }

static int dataEntityRecentlyFired(const struct runtimeActor *actor)
    {
    int threshold;

    if (actor->dataArchetype->weapon == ENTITY_WEAPON_NONE ||
	 actor->dataAgent.cooldown <= 0)
	return 0;
    threshold = actor->dataArchetype->fireInterval - 3;
    if (threshold < 1)
	threshold = 1;
    return actor->dataAgent.cooldown >= threshold;
    }

static int projectileDestroysMapObject(
        const struct EntityArchetype *projectileArchetype,
	const struct runtimeMapObject *structure)
    {
    if (projectileArchetype == NULL || structure->dataArchetype == NULL ||
        !projectileArchetype->projectileDestroysMapObjects ||
        !structure->dataArchetype->destructible)
	return 0;
    return 1;
    }

static int projectilePathHitsActorList(
    const struct runtimeActor *actors,
    float x0, float y0, float z0, float x1, float y1, float z1)
    {
    const struct runtimeActor *actor;

    if (actors == NULL)
        return 0;
    for (actor = actors->next; actor != NULL; actor = actor->next)
        if (projectilePathHitsActor(actor, x0, y0, z0, x1, y1, z1))
            return 1;
    return 0;
    }

void updateRuntimeProjectiles()
    {
    float px, py, pz;
    float qx, qy, qz;
    float floor, ceiling;
    int dead;
    int inPlay, playerHit, groundHit;
    struct targetInfo * temptarget;
    struct runtimePlayer playerState;
    float playerX, playerZ;

    struct runtimeProjectile * pproj, *killer, *temp;
    struct runtimeMapObject *mapObject;

    float maxX, maxZ, minX, minZ;

    maxX = minX = playerEntityState.x;
    maxZ = minZ = playerEntityState.z;

    /******************************************************************************/
    /* figure out the range of valid shells bounded by all monsters + PLANESIZE   */
    /******************************************************************************/

    for(temptarget = targets->next; temptarget != NULL; temptarget = temptarget->next)
	{
	if (temptarget->x > maxX)
	    maxX = temptarget->x;
	if (temptarget->x < minX)
	    minX = temptarget->x;
	if (temptarget->z > maxZ)
	    maxZ = temptarget->z;
	if (temptarget->z < minZ)
	    minZ = temptarget->z;
	}

    maxX += (PLANESIZE+1);
    maxZ += (PLANESIZE+1);
    minX -= (PLANESIZE+1);
    minZ -= (PLANESIZE+1);

    /**********************************/
    /* process each shell in turn     */
    /**********************************/

    floor   = PLANEY + 0.1;
    ceiling = PLANEY + 10;

    temp = runtimeProjectileList;
    while (temp->next != NULL)
	{
	dead = 0;

	pproj = temp->next;

	pproj->count += 1;

	if (pproj->count > pproj->archetype->projectileArmingFrames)
	    pproj->count = (char)pproj->archetype->projectileArmingFrames;

	/**********************************/
	/* is projectile still in play?   */
	/**********************************/

	px = pproj->x;
	py = pproj->y;
	pz = pproj->z;

	qx = pproj->x;
	qy = pproj->y;
	qz = pproj->z;

	playerHit = 0;

	/**************************************************/
	/* did the projectile hit the player's player?   */
	/**************************************************/

	if (pproj->archetype->projectileDamagesActors)
    	    {
	    if ((playerEntityState.dead != 1) &&
		(px <= playerEntityState.x + playerEntityState.width) &&
		(pz <= playerEntityState.z + playerEntityState.width) &&
		(px >= playerEntityState.x - playerEntityState.width) &&
		(pz >= playerEntityState.z - playerEntityState.width) &&
		(py <= playerEntityState.height) && (py >= playerEntityState.bottom))
		    {
		    if (pproj->owner != &playerEntityState)
			{
			playerEntityState.energyRemaining -=
			    pproj->archetype->projectileDamage;
			playerHit = 1;

    /*
			if (pproj->owner != NULL)
			    pproj->owner->score += 1;
    */
			}
		    }

	    /********************************************/
	    /* did the projectile hit another target?   */
	    /********************************************/

	    for(temptarget = targets->next;temptarget != NULL;temptarget = temptarget->next)
		{
		playerState = temptarget->player;
		playerX = temptarget->x;
		playerZ = temptarget->z;

		if ( (playerState.dead != 1) &&
		    (qx <= (playerX+playerState.width)) && (qz <= (playerZ+playerState.width)) &&
		    (qx >= (playerX-playerState.width)) && (qz >= (playerZ-playerState.width)) &&
		    (qy <= playerState.height) && (qy >= playerState.bottom))
			{
			if (pproj->owner != &(temptarget->player))
			    {
			    temptarget->player.energyRemaining -=
			        pproj->archetype->projectileDamage;
			    playerHit = 1;

    /*
			    if (pproj->owner != NULL)
				pproj->owner->score += 1;
    */
			    }
			}
		}
	    }

	if (!playerHit && (py > floor) && (py < ceiling) &&
	    (px > minX) && (pz > minZ) && (px < maxX) && (pz < maxZ))
	    inPlay = 1;
	else
	    inPlay = 0;

	if ((px >= minX) && (pz >= minZ) &&
	    (px <= maxX) && (pz <= maxZ) && (py <= floor) &&
	    pproj->archetype->projectileCollidesGround)
	    groundHit = 1;
	else
	    groundHit = 0;

	if ((playerHit || groundHit) &&
	    pproj->archetype->impactEffect[0] != '\0')
	    spawnConfiguredExplosion(pproj->archetype->impactEffect,
	        pproj->x, pproj->y, pproj->z, pproj->owner, 1);


	if (!inPlay)
	    dead = 1;

	if (inPlay)
	    {
	    /**********************************/
	    /* update projectile position     */
	    /**********************************/

	    px = pproj->x += pproj->xv;
	    py = pproj->y += pproj->yv;
	    pz = pproj->z += pproj->zv;

	    if (pproj->y < PLANEY)
		py = pproj->y = PLANEY;

	    /**********************************/
	    /* shells affected by gravity     */
	    /**********************************/

	    pproj->yv -= pproj->archetype->projectileGravity;

	    /**********************************/
	    /* check for a actor hit           */
	    /**********************************/

	    if (pproj->archetype->projectileCollidesActors &&
		((pproj->count >=
		  pproj->archetype->projectileArmingFrames) ||
		 (pproj->owner == &playerEntityState)))
		{
		if (projectilePathHitsActorList(
		        actorlist, qx, qy, qz, px, py, pz) ||
		    projectilePathHitsActorList(
		        wrecklist, qx, qy, qz, px, py, pz))
		    {
		    spawnConfiguredExplosion(pproj->archetype->impactEffect,
		        px, py, pz, pproj->owner, 1);
		    dead = 1;
		    }
		}

	    /**********************************/
	    /* check for a map-object hit     */
	    /**********************************/

	    if (pproj->archetype->projectileCollidesMapObjects)
		{
		for (mapObject = mapObjectList->next; mapObject != NULL;
		     mapObject = mapObject->next)
		    {
		    if (projectilePathHitsMapObject(
			mapObject,
			qx, qy, qz, px, py, pz))
			    {
			    spawnConfiguredExplosion(pproj->archetype->impactEffect,
			        px, py, pz, pproj->owner, 1);
			    dead = 1;

			    if (projectileDestroysMapObject(
				pproj->archetype, mapObject))
				{
				if (mapObject->intact && pproj->owner != NULL)
				    pproj->owner->score +=
				        mapObject->dataArchetype->score;

				mapObject->intact = 0;
				}
			    }
		    }
		}
	    }

	/************************************/
	/* if projectile is dead, remove it */
	/************************************/

	if (dead)
	    {
	    killer = temp->next;
	    temp->next = temp->next->next;
	    afree(killer, arena);
	    }
	else
	    temp = temp->next;
	}
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* update the state of all map objects                            */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void updateMapObjects(struct runtimeMapObject *objects, int seasonal)
    {
    struct runtimeMapObject *object;
    struct runtimeMapObject *previous;
    const struct EntityArchetype *debris;
    const struct EntityArchetype *fire;
    const struct EntityArchetype *emitter;
    float Vplane;
    int fragment;
    int fragmentCount;
    int dead;

    (void)seasonal;
    destroyedMapObjectFrameCount = 0;
    Vplane = sqrt(GRAVITY * 1.5) * 0.7071;
    previous = objects;
    while (previous->next != NULL)
	{
	dead = 0;
	object = previous->next;
	object->rad = sqrt((object->x - playerEntityState.x) *
	                   (object->x - playerEntityState.x) +
	                   (object->z - playerEntityState.z) *
	                   (object->z - playerEntityState.z));
	object->theta = computeAngle(object->z, playerEntityState.z,
	                             object->x, playerEntityState.x);
	emitter = entityCatalogFind(&gameEntities,
	                            object->dataArchetype->emitterProjectile);
	if (object->intact && emitter != NULL)
	    spawnProjectile(object->x,
	                  object->y + object->height,
	                  object->z, emitter,
	                  2.0f * randy(0.005f), 0.05f,
	                  2.0f * randy(0.005f), 0.0f, NULL);

	if (!object->intact && object->dataArchetype->destructible)
	    {
	    doSoundKeyAt(object->dataArchetype->destructionSound,
	                 object->x, object->y, object->z);
	    spawnDestructionEffects(
	        object->dataArchetype,
	        object->x, object->y + object->height * 0.5f,
	        object->z, mapObjectCollisionRadius(object),
	        object->height, NULL, 1);
	    dead = 1;
	    recordDestroyedMapObject(object->number);
	    debris = entityCatalogFind(
	        &gameEntities, object->dataArchetype->destructionProjectile);
	    fire = entityCatalogFind(
	        &gameEntities, object->dataArchetype->destructionFireProjectile);
	    fragmentCount = object->dataArchetype->destructionDebrisCount;
	    for (fragment = 0; fragment < fragmentCount; fragment++)
		if (debris != NULL)
		    spawnProjectile(
		        object->x + randy(mapObjectCollisionRadius(object)),
		        object->y + randy(object->height * 0.5f) +
		            object->height * 0.5f,
		        object->z + randy(mapObjectCollisionRadius(object)),
		        debris, sin(randy(PI)) * Vplane, Vplane,
		        cos(randy(PI)) * Vplane, 75.0f, NULL);
	    for (fragment = 0;
	         fragment < object->dataArchetype->destructionFireCount;
	         fragment++)
		if (fire != NULL)
		    spawnProjectile(object->x + randy(0.25f), PLANEY + 0.14f,
		                  object->z + randy(0.25f), fire,
		                  0.0f, -0.0005f, 0.0f, 10000.0f, NULL);
	    }
	if (dead)
    	    {
	    previous->next = object->next;
	    afree(object, arena);
	    }
	else
	    previous = previous->next;
	}
    }
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* update all of the vehicles                                    */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void initializeContact(struct EntityContact *contact,
                              const char *id, const char *faction,
                              float x, float y, float z, int active,
                              int visible, int audible)
    {
    memset(contact, 0, sizeof(*contact));
    strcpy(contact->id, id);
    strcpy(contact->faction, faction);
    contact->x = x;
    contact->y = y;
    contact->z = z;
    contact->active = active;
    contact->visible = visible;
    contact->audible = audible;
    }

static void notifyDataEntityDamage(struct runtimeActor *actor, const char *id,
	const char *faction, float x, float z)
    {
    struct EntityContact source;

    initializeContact(&source, id, faction, x, PLANEY, z, 1, 1, 1);
    entityAgentNotifyDamage(&actor->dataAgent, &source);
    }

static void notifyDataEntityDamageFromPlayer(struct runtimeActor *actor,
	struct runtimePlayer *owner, float x, float z)
    {
    struct targetInfo *target;
    char id[ENTITY_NAME_SIZE];
    int number;

    if (owner == &playerEntityState)
	{
	notifyDataEntityDamage(actor, "player",
	    runtimePlayerFaction(), x, z);
	return;
	}
    number = 0;
    for (target = targets->next; target != NULL; target = target->next)
	{
	if (owner == &target->player)
	    {
	    sprintf(id, "target-%d", number);
	    notifyDataEntityDamage(
	        actor, id,
	        runtimePlayerFaction(), x, z);
	    return;
	    }
	number++;
	}
    notifyDataEntityDamage(actor, "environment", "neutral", x, z);
    }

static struct runtimeActor *findDataActor(struct runtimeActor *allActors, const char *id)
    {
    struct runtimeActor *actor;

    for (actor = allActors->next; actor != NULL; actor = actor->next)
        if (strcmp(actor->dataId, id) == 0)
            return actor;
    return NULL;
    }

static void routeDataEntityEvents(struct runtimeActor *source,
                                  struct runtimeActor *allActors)
    {
    const struct BattalionMapTrigger *route;
    struct runtimeActor *receiver;
    size_t eventIndex;
    size_t routeIndex;

    if (source->dataObject == NULL)
        return;
    for (eventIndex = 0; eventIndex < source->dataAgent.eventCount;
         eventIndex++)
        for (routeIndex = 0; routeIndex < source->dataObject->triggerCount;
             routeIndex++)
            {
            route = &source->dataObject->triggers[routeIndex];
            if (strcmp(route->event,
                       source->dataAgent.events[eventIndex].name) != 0)
                continue;
            for (receiver = allActors->next; receiver != NULL;
                 receiver = receiver->next)
                {
                if (receiver->dataObject == NULL)
                    continue;
                if ((route->targetType == BATTALION_MAP_TRIGGER_ID &&
                     strcmp(receiver->dataId, route->target) == 0) ||
                    (route->targetType == BATTALION_MAP_TRIGGER_GROUP &&
                     strcmp(receiver->dataGroup, route->target) == 0))
                    entityAgentQueueTrigger(
                        &receiver->dataAgent,
                        source->dataAgent.events[eventIndex].name,
                        &source->dataAgent.events[eventIndex].contact);
                }
            }
    }

static size_t buildDataEntityContacts(struct runtimeActor *source,
                                      struct runtimeActor *allActors,
                                      struct runtimePlayer *player,
                                      struct targetInfo *targetList,
                                      struct EntityContact *contacts,
                                      size_t capacity)
    {
    struct targetInfo *target;
    struct runtimeActor *actor;
    size_t count;
    int targetNumber;
    int audible;
    int visible;
    int playerFired;
    float targetY;
    char id[ENTITY_NAME_SIZE];

    count = 0;
    playerFired = 0;
    if (playerEntityArchetype != NULL &&
	playerEntityArchetype->weapon != ENTITY_WEAPON_NONE &&
	playerWeaponReloadRemaining > 0)
	{
	int threshold;

	threshold = playerEntityArchetype->fireInterval - 3;
	if (threshold < 1)
	    threshold = 1;
	playerFired = playerWeaponReloadRemaining >= threshold;
	}
    if (count < capacity)
	{
	targetY = (player->height + player->bottom) * 0.5f;
	visible = dataEntityLineOfSight(source, player->x, targetY, player->z);
	audible = player->moving || player->beamOn || playerFired;
	initializeContact(&contacts[count++], "player",
		  runtimePlayerFaction(), player->x, targetY, player->z,
			  player->dead != 1, visible, audible);
	}
    targetNumber = 0;
    for (target = targetList->next; target != NULL && count < capacity;
         target = target->next)
	{
	sprintf(id, "target-%d", targetNumber++);
	targetY = (target->player.height + target->player.bottom) * 0.5f;
	visible = dataEntityLineOfSight(source, target->x, targetY,
					target->z);
	initializeContact(&contacts[count++], id,
			  runtimePlayerFaction(),
			  target->x, targetY, target->z,
			  target->player.dead != 1, visible,
			  target->player.moving ||
			  target->player.beamOn);
        }
    for (actor = allActors->next; actor != NULL && count < capacity;
         actor = actor->next)
	if (actor != source && actor->dataArchetype != NULL)
	    if (actor->dataArchetype->kind != ENTITY_KIND_ZONE)
	    {
	    targetY = actor->y + actor->dataArchetype->collisionHeight * 0.5f;
	    visible = dataEntityLineOfSight(source, actor->x, targetY, actor->z);
	    audible = actor->walking || actor->weaponActiveFrames > 0 ||
		       dataEntityRecentlyFired(actor);
	    initializeContact(&contacts[count++], actor->dataId,
			      actor->dataAgent.faction,
			      actor->x, targetY, actor->z, actor->damage > 0,
			      visible, audible);
	    }
    return count;
    }

static void damageDataEntityTarget(struct runtimeActor *source,
                                   struct runtimeActor *allActors,
                                   struct runtimePlayer *player,
                                   struct targetInfo *targetList)
    {
    struct targetInfo *target;
    struct runtimeActor *actor;
    int targetNumber;
    char id[ENTITY_NAME_SIZE];

    if (strcmp(source->dataAgent.target.id, "player") == 0)
        {
        player->energyRemaining -= 0.3f;
        return;
        }
    targetNumber = 0;
    for (target = targetList->next; target != NULL; target = target->next)
        {
        sprintf(id, "target-%d", targetNumber++);
        if (strcmp(source->dataAgent.target.id, id) == 0)
            {
            target->player.energyRemaining -= 0.3f;
            return;
            }
        }
    actor = findDataActor(allActors, source->dataAgent.target.id);
    if (actor != NULL)
        {
        actor->damage--;
        notifyDataEntityDamage(actor, source->dataId,
			       source->dataAgent.faction,
			       source->x, source->z);
        }
    }

static int configuredMuzzleWorld(
    const struct EntityArchetype *archetype, int moving, int attacking,
    int counter, float entityX, float entityY, float entityZ,
    float bodyHeading, float aimYaw, float aimPitch,
    float origin[3], float direction[3])
    {
    float localOrigin[3];
    float localDirection[3];
    float radians;
    float sine;
    float cosine;

    if (!configuredEntityMuzzlePose(archetype, moving, attacking, counter,
                                    aimYaw, aimPitch,
                                    localOrigin, localDirection))
        return 0;
    radians = bodyHeading * DEG_TO_RAD;
    sine = (float)sin((double)radians);
    cosine = (float)cos((double)radians);
    origin[0] = entityX + cosine * localOrigin[0] +
                sine * localOrigin[2];
    origin[1] = entityY + localOrigin[1];
    origin[2] = entityZ - sine * localOrigin[0] +
                cosine * localOrigin[2];
    direction[0] = cosine * localDirection[0] +
                   sine * localDirection[2];
    direction[1] = localDirection[1];
    direction[2] = -sine * localDirection[0] +
                   cosine * localDirection[2];
    return 1;
    }

static int configuredPlayerMuzzleWorld(float origin[3], float direction[3])
    {
    float bodyHeading;
    float aimYaw;

    if (playerEntityArchetype == NULL)
        return 0;
    bodyHeading = playerEntityState.headHorzRotate * 0.1f;
    aimYaw = 0.0f;
    if (playerEntityArchetype->controlMode == ENTITY_CONTROL_STEERED)
        {
        bodyHeading = playerBodyHeading * 0.1f;
        aimYaw = (playerEntityState.headHorzRotate - playerBodyHeading) * 0.1f;
        }
    return configuredMuzzleWorld(
        playerEntityArchetype, playerEntityState.moving, playerEntityState.beamOn,
        mainCounter, playerEntityState.x,
        PLANEY + playerEntityArchetype->verticalOffset, playerEntityState.z,
        bodyHeading, aimYaw, playerEntityState.headVertRotate * 0.1f,
        origin, direction);
    }

static const struct EntityArchetype *weaponProjectile(
    const struct EntityArchetype *archetype)
    {
    if (archetype == NULL || archetype->weapon != ENTITY_WEAPON_PROJECTILE)
        return NULL;
    return entityCatalogFind(&gameEntities, archetype->projectile);
    }

static void updatePlayerProjectile(
    float x, float z, int counter, const struct runtimePlayer *state,
    struct runtimePlayer *owner)
    {
    const struct EntityArchetype *archetype;
    float origin[3];
    float direction[3];
    const struct EntityArchetype *projectile;

    archetype = playerArchetype(state);
    if (archetype == NULL || archetype->weapon == ENTITY_WEAPON_NONE ||
        counter % archetype->fireInterval != 0 ||
        !configuredMuzzleWorld(
            archetype, state->moving, state->beamOn, counter,
            x, PLANEY + archetype->verticalOffset, z,
            state->headHorzRotate * 0.1f, 0.0f,
            state->headVertRotate * 0.1f, origin, direction))
        return;
    projectile = weaponProjectile(archetype);
    if (projectile == NULL)
        return;
    spawnProjectile(origin[0], origin[1], origin[2], projectile,
                  direction[0] * archetype->projectileSpeed,
                  direction[1] * archetype->projectileSpeed,
                  direction[2] * archetype->projectileSpeed,
                  archetype->accuracy, owner);
    }

static void aimDataEntityAtTarget(struct runtimeActor *actor)
    {
    const struct EntityArchetype *archetype;
    float origin[3];
    float direction[3];
    float deltaX;
    float deltaY;
    float deltaZ;
    float horizontal;
    float bodyHeading;
    float targetHeading;
    float aimYaw;
    float aimPitch;

    archetype = actor->dataArchetype;
    actor->aimYaw = 0.0f;
    actor->aimPitch = archetype->aimPitchDefault;
    if (!actor->dataAgent.hasTarget)
        return;
    deltaX = actor->dataAgent.targetX - actor->x;
    deltaZ = actor->dataAgent.targetZ - actor->z;
    horizontal = (float)sqrt((double)(deltaX * deltaX + deltaZ * deltaZ));
    if (horizontal <= 0.0001f)
        return;
    bodyHeading = actor->theta * RAD_TO_SMALL_DEG;
    targetHeading = (float)(atan2((double)deltaX, (double)deltaZ) /
                                  DEG_TO_RAD);
    aimYaw = targetHeading - bodyHeading;
    while (aimYaw > 180.0f)
        aimYaw -= 360.0f;
    while (aimYaw < -180.0f)
        aimYaw += 360.0f;
    if (archetype->aimYawNode[0] != '\0')
        actor->aimYaw = aimYaw;
    if (archetype->aimPitchNode[0] == '\0' ||
        !configuredMuzzleWorld(archetype, actor->walking, 1, mainCounter,
                               actor->x, actor->y, actor->z, bodyHeading,
                               actor->aimYaw, 0.0f, origin, direction))
        return;
    deltaY = actor->dataAgent.targetY - origin[1];
    aimPitch = (float)(-atan2((double)deltaY, (double)horizontal) /
                             DEG_TO_RAD);
    if (aimPitch < archetype->aimPitchMin)
        aimPitch = archetype->aimPitchMin;
    if (aimPitch > archetype->aimPitchMax)
        aimPitch = archetype->aimPitchMax;
    actor->aimPitch = aimPitch;
    }

static void fireDataEntity(struct runtimeActor *actor, struct runtimeActor *allActors,
                           struct runtimePlayer *player,
                           struct targetInfo *targetList)
    {
    const struct EntityArchetype *archetype;
    float origin[3];
    float direction[3];
    float bodyHeading;
    const struct EntityArchetype *projectile;
    int burst;

    archetype = actor->dataArchetype;
    actor->weaponActiveFrames = 0;
    if (!actor->dataAgent.fireRequested || !actor->dataAgent.hasTarget)
        return;
    if (archetype->weapon == ENTITY_WEAPON_BEAM)
        {
        actor->weaponActiveFrames = 2;
        damageDataEntityTarget(actor, allActors, player, targetList);
        doSoundKeyAt(archetype->fireSound, actor->x, actor->y, actor->z);
        return;
        }
    bodyHeading = actor->theta * RAD_TO_SMALL_DEG;
    if (!configuredMuzzleWorld(archetype, actor->walking, 1, mainCounter,
                               actor->x, actor->y, actor->z, bodyHeading,
                               actor->aimYaw, actor->aimPitch,
                               origin, direction))
        return;
    projectile = weaponProjectile(archetype);
    if (projectile == NULL)
        return;
    for (burst = 0; burst < archetype->burstCount; burst++)
        spawnProjectile(origin[0], origin[1], origin[2], projectile,
                      direction[0] * archetype->projectileSpeed,
                      direction[1] * archetype->projectileSpeed,
                      direction[2] * archetype->projectileSpeed,
                      archetype->accuracy, NULL);
    }

static const struct EntitySurfaceSound *surfaceSoundAt(
    const struct EntityArchetype *movingArchetype, float x, float z)
    {
    const struct BattalionMapObject *object;
    const struct EntityArchetype *surface;
    const struct EntitySurfaceSound *rule;
    float radius;
    size_t objectIndex;
    size_t ruleIndex;

    if (movingArchetype == NULL ||
        movingArchetype->contactAudioTag[0] == '\0')
        return NULL;
    for (objectIndex = 0; objectIndex < battlefieldMap.objectCount;
         objectIndex++)
        {
        object = &battlefieldMap.objects[objectIndex];
        if (!object->active)
            continue;
        surface = entityCatalogFind(&gameEntities, object->archetype);
        if (surface == NULL || surface->surfaceSoundCount == 0)
            continue;
        radius = entityArchetypeCollisionRadius(surface) * object->scale;
        if (radius <= 0.0f || fabs(x - object->x) > radius ||
            fabs(z - object->z) > radius)
            continue;
        for (ruleIndex = 0; ruleIndex < surface->surfaceSoundCount;
             ruleIndex++)
            {
            rule = &surface->surfaceSounds[ruleIndex];
            if (strcmp(rule->contactTag,
                       movingArchetype->contactAudioTag) == 0)
                return rule;
            }
        }
    return NULL;
    }

static int surfaceSoundDue(const struct EntityArchetype *movingArchetype,
                           int counter)
    {
    const struct EntityArchetype *surface;
    const struct EntitySurfaceSound *rule;
    size_t archetypeIndex;
    size_t ruleIndex;

    if (movingArchetype == NULL ||
        movingArchetype->contactAudioTag[0] == '\0')
        return 0;
    for (archetypeIndex = 0; archetypeIndex < gameEntities.count;
         archetypeIndex++)
        {
        surface = &gameEntities.entries[archetypeIndex];
        for (ruleIndex = 0; ruleIndex < surface->surfaceSoundCount;
             ruleIndex++)
            {
            rule = &surface->surfaceSounds[ruleIndex];
            if (strcmp(rule->contactTag,
                       movingArchetype->contactAudioTag) == 0 &&
                counter % rule->interval == 0)
                return 1;
            }
        }
    return 0;
    }

static void updateMovementAudio(const struct EntityArchetype *archetype,
                                float x, float y, float z, int moving,
                                int counter)
    {
    const struct EntitySurfaceSound *surfaceSound;

    if (!moving || archetype == NULL)
        return;
    if (archetype->moveSound[0] != '\0' &&
        counter % archetype->moveSoundInterval == 0)
        doSoundKeyAt(archetype->moveSound, x, y, z);
    if (!surfaceSoundDue(archetype, counter))
        return;
    surfaceSound = surfaceSoundAt(archetype, x, z);
    if (surfaceSound != NULL && counter % surfaceSound->interval == 0)
        doSoundKeyAt(surfaceSound->sound, x, y, z);
    }

static void updateDataEntity(struct runtimeActor *actor, struct runtimeActor *allActors,
                             struct runtimePlayer *player,
                             struct targetInfo *targetList)
    {
    struct EntityContact *contacts;
    struct ActorObstacleContext obstacleContext;
    struct runtimeActor *contactActor;
    struct targetInfo *contactPlayer;
    size_t capacity;
    size_t contactCount;
    float oldX;
    float oldZ;

    capacity = 1;
    for (contactActor = allActors->next; contactActor != NULL;
         contactActor = contactActor->next)
        capacity++;
    for (contactPlayer = targetList->next; contactPlayer != NULL;
         contactPlayer = contactPlayer->next)
        capacity++;
    contacts = (struct EntityContact *)malloc(capacity * sizeof(*contacts));
    if (contacts == NULL)
        return;
    oldX = actor->dataAgent.x;
    oldZ = actor->dataAgent.z;
    contactCount = buildDataEntityContacts(
        actor, allActors, player, targetList, contacts,
        capacity);
    entityAgentStep(&actor->dataAgent, contacts, contactCount);
    obstacleContext.actor = actor;
    obstacleContext.allActors = allActors;
    entityAgentResolveObstacle(
        &actor->dataAgent, oldX, oldZ, actorPositionBlocked,
        &obstacleContext, (actor->number & 1) ? -1 : 1);
    free(contacts);
    actor->x = actor->dataAgent.x;
    actor->z = actor->dataAgent.z;
    actor->theta = actor->dataAgent.heading;
    actor->walking = oldX != actor->x || oldZ != actor->z;
    if (actor->dataAgent.hasTarget)
        actor->rad = (float)sqrt((double)(
            (actor->x - actor->dataAgent.targetX) *
            (actor->x - actor->dataAgent.targetX) +
            (actor->z - actor->dataAgent.targetZ) *
            (actor->z - actor->dataAgent.targetZ)));
    else
        actor->rad = 0;
    aimDataEntityAtTarget(actor);
    fireDataEntity(actor, allActors, player, targetList);
    }

static void updateConfiguredPlayerWeapon(void)
    {
    const struct EntityArchetype *archetype;
    float origin[3];
    float direction[3];
    const struct EntityArchetype *projectile;

    archetype = playerEntityArchetype;
    if (archetype == NULL || archetype->weapon == ENTITY_WEAPON_NONE)
        return;
    if (!configuredPlayerMuzzleWorld(origin, direction))
        return;
    projectile = weaponProjectile(archetype);
    if (archetype->weapon == ENTITY_WEAPON_PROJECTILE &&
        projectile != NULL &&
        !projectile->projectileContinuous)
        {
        if (playerProjectileTriggerLatched)
            return;
        playerProjectileTriggerLatched = 1;
	if (playerWeaponReloadRemaining > 0)
	    return;
        }
    if (archetype->weapon == ENTITY_WEAPON_BEAM)
        {
        updateBeam(&playerEntityState, targets, actorlist,
                   mapObjectList, &playerEntityState,
                   archetype, origin, direction);
        playerEntityState.energyRemaining -= archetype->playerFireCost;
        return;
        }
    if (projectile == NULL)
        return;
    spawnProjectile(origin[0], origin[1], origin[2], projectile,
                  direction[0] * archetype->projectileSpeed,
                  direction[1] * archetype->projectileSpeed,
                  direction[2] * archetype->projectileSpeed,
                  archetype->accuracy, &playerEntityState);
    if (!projectile->projectileContinuous)
	playerWeaponReloadRemaining = archetype->fireInterval;
    playerEntityState.energyRemaining -= archetype->playerFireCost;
    }

static void updatePlayerBeam(
    struct runtimePlayer *gameMonster, struct targetInfo *targetList,
    float x, float z, struct runtimeActor *allActors,
    struct runtimeMapObject *allMapObjects, struct runtimePlayer *source)
    {
    const struct EntityArchetype *archetype;
    float origin[3];
    float direction[3];

    archetype = playerArchetype(source);
    if (archetype == NULL || !configuredMuzzleWorld(
        archetype, source->moving, source->beamOn, mainCounter,
        x, PLANEY + archetype->verticalOffset, z,
        source->headHorzRotate * 0.1f, 0.0f,
        source->headVertRotate * 0.1f, origin, direction))
        return;
    updateBeam(gameMonster, targetList, allActors,
               allMapObjects, source,
               archetype, origin, direction);
    }

static void updatePlayerWeapon(
    float x, float z, struct runtimePlayer *source,
    struct runtimePlayer *owner, struct runtimeMapObject *allMapObjects)
    {
    const struct EntityArchetype *archetype;

    archetype = playerArchetype(source);
    if (archetype == NULL || archetype->weapon == ENTITY_WEAPON_NONE)
        return;
    if (archetype->weapon == ENTITY_WEAPON_BEAM)
        updatePlayerBeam(
            &playerEntityState, targets, x, z, actorlist,
            allMapObjects, source);
    else if (archetype->weapon == ENTITY_WEAPON_PROJECTILE)
        updatePlayerProjectile(
            x, z, mainCounter, source, owner);
    source->energyRemaining -= archetype->playerFireCost;
    }

static void spawnDataEntityDebris(struct runtimeActor *source,
                                  struct runtimeActor *allWrecks)
    {
    const struct EntityArchetype *archetype;
    const struct EntityArchetype *debrisVisual;
    struct runtimeActor *fragment;
    float angle;
    float blastRadius;
    float collisionHeight;
    float fragmentSize;
    float speed;
    float sourceX;
    float sourceY;
    float sourceZ;
    int fromAir;
    int i;
    int sourceNumber;

    archetype = source->dataArchetype;
    debrisVisual = entityCatalogFind(
        &gameEntities, archetype->destructionProjectile);
    sourceX = source->x;
    sourceY = source->y;
    sourceZ = source->z;
    sourceNumber = source->number;
    collisionHeight = archetype->collisionHeight;
    blastRadius = entityArchetypeCollisionRadius(archetype);
    if (blastRadius < 0.2f)
        blastRadius = 0.2f;
    fragmentSize = blastRadius * 0.36f;
    if (fragmentSize < 0.05f)
        fragmentSize = 0.05f;
    if (fragmentSize > 0.16f)
        fragmentSize = 0.16f;
    fromAir = archetype->movement == ENTITY_MOVEMENT_AIR;

    doSoundKeyAt(archetype->destructionSound,
                 sourceX, sourceY, sourceZ);
    for (i = 0; i < archetype->destructionDebrisCount; i++)
        {
        if (i == 0)
            fragment = source;
        else
            fragment = (struct runtimeActor *)amalloc(sizeActor, arena);
        if (fragment == NULL)
            continue;
        memset(fragment, 0, sizeof(*fragment));
        angle = (float)(rand() % 6283) * 0.001f;
        speed = 0.018f + (float)(rand() % 28) * 0.001f;
        fragment->x = sourceX + randy(blastRadius * 0.28f);
        fragment->y = sourceY + collisionHeight *
            (0.24f + (float)(rand() % 52) * 0.01f);
        fragment->z = sourceZ + randy(blastRadius * 0.28f);
        fragment->xv = sin(angle) * speed;
        fragment->yv = 0.035f + (float)(rand() % 45) * 0.001f;
        fragment->zv = cos(angle) * speed;
        fragment->theta = angle;
        fragment->count = 0;
        fragment->number = sourceNumber;
        fragment->subtype = (char)i;
        fragment->dataArchetype = archetype;
        fragment->visualArchetype = debrisVisual;
        fragment->debrisFromAir = fromAir;
        fragment->debrisFireDelay = i * 5;
        fragment->debrisSize = fragmentSize *
            (0.72f + (float)(rand() % 57) * 0.01f);
        fragment->next = allWrecks->next;
        allWrecks->next = fragment;
        }
    if (archetype->destructionDebrisCount == 0)
        afree(source, arena);
    spawnDestructionEffects(
        archetype, sourceX, sourceY + collisionHeight * 0.5f, sourceZ,
        blastRadius, collisionHeight, NULL, 1);
    }

static int updateDataEntityDebris(struct runtimeActor *fragment)
    {
    const struct EntityArchetype *fireProjectile;
    int landed;

    landed = entityWreckStep(
        &fragment->x, &fragment->y, &fragment->z,
        &fragment->xv, &fragment->yv, &fragment->zv,
        PLANEY + fragment->debrisSize * 0.55f,
        GRAVITY, &fragment->debrisGrounded);
    if (landed && fragment->debrisFromAir)
        spawnConfiguredExplosion(fragment->dataArchetype->destructionEffect,
            fragment->x, fragment->y, fragment->z, NULL, 0);

    if (fragment->debrisGrounded &&
        fragment->subtype < fragment->dataArchetype->destructionFireCount)
        {
        fragment->debrisFireDelay--;
        if (fragment->debrisFireDelay <= 0)
            {
            fireProjectile = entityCatalogFind(
                &gameEntities,
                fragment->dataArchetype->wreckFireProjectile);
            spawnProjectile(fragment->x, PLANEY + 0.14f, fragment->z,
                          fireProjectile, 0.0f, -0.0005f, 0.0f,
                          10000.0f, NULL);
            fragment->debrisFireDelay = 68 + fragment->subtype * 7;
            }
        }
    return fragment->count >= fragment->dataArchetype->debrisLife;
    }

void updateActors(struct runtimeActor *allActors, float width, float height,
                 float bottom, int dead,
                 struct runtimeActor *wrecks, float accuracy, int firingDelay,
                 int counter, struct runtimePlayer *player,
                 struct targetInfo *targetList)
    {
    struct runtimeActor *killeractor;
    struct runtimeActor *actor;
    struct runtimeActor *previous;

    (void)accuracy;
    (void)firingDelay;
    previous = allActors;
    while (previous->next != NULL)
        {
        actor = previous->next;
        if (actor->dataArchetype == NULL)
            {
            showError("Discarding an actor without a catalog archetype");
            previous->next = actor->next;
            afree(actor, arena);
            continue;
            }
        actor->count--;
        updateDataEntity(actor, allActors, player, targetList);
        updateMovementAudio(actor->dataArchetype, actor->x, actor->y,
                            actor->z, actor->walking, counter);
        if (!dead && actorCollisionRadius(actor) > 0.0f &&
            factionCatalogHostile(&gameFactions, actor->dataAgent.faction,
                                  runtimePlayerFaction()) &&
            worldCylindersOverlap(
                actor->x, actor->z, actorCollisionRadius(actor),
                actorCollisionBottom(actor), actorCollisionTop(actor),
                player->x, player->z, width, bottom, height))
            {
            player->energyRemaining -= 0.75f;
            actor->damage--;
            notifyDataEntityDamage(
                actor, "player",
                runtimePlayerFaction(),
                                   player->x, player->z);
            }
        if (actor->damage <= 0)
            {
            killeractor = actor;
            previous->next = killeractor->next;
            spawnDataEntityDebris(killeractor, wrecks);
            }
        else
            previous = previous->next;
        }

    /* Events are delivered after every agent has completed this step. */
    for (actor = allActors->next; actor != NULL; actor = actor->next)
        routeDataEntityEvents(actor, allActors);
    }

void updateWrecks(struct runtimeActor *allWrecks)
    {
    struct runtimeActor *fragment;
    struct runtimeActor *previous;

    previous = allWrecks;
    while (previous->next != NULL)
        {
        fragment = previous->next;
        fragment->count++;
        if (fragment->dataArchetype == NULL ||
            updateDataEntityDebris(fragment))
            {
            previous->next = fragment->next;
            afree(fragment, arena);
            }
        else
            previous = fragment;
        }
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* a new military vehicle is entering the game                   */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int mapAreaContains(void *context, float x, float z)
    {
    const struct BattalionMapObject *object;
    const struct BattalionMapPoint *first;
    const struct BattalionMapPoint *second;
    float crossingX;
    size_t firstIndex;
    size_t secondIndex;
    int inside;

    object = (const struct BattalionMapObject *)context;
    if (object == NULL || object->geometryCount < 3)
        return 0;
    inside = 0;
    secondIndex = object->geometryCount - 1;
    for (firstIndex = 0; firstIndex < object->geometryCount; firstIndex++)
        {
        first = &object->geometry[firstIndex];
        second = &object->geometry[secondIndex];
        if ((first->z > z) != (second->z > z))
            {
            crossingX = (second->x - first->x) * (z - first->z) /
                        (second->z - first->z) + first->x;
            if (x < crossingX)
                inside = !inside;
            }
        secondIndex = firstIndex;
        }
    return inside;
    }

static int addConfiguredEntity(
    const struct EntityArchetype *archetype,
    const char *id, const char *group,
    float x, float z, float heading, int active,
    const struct EntityWaypoint *waypoints, size_t waypointCount,
    int patrolMode, const char *faction,
    const struct BattalionMapObject *object,
    struct runtimeActor *allActors)
    {
    struct runtimeActor *actor;
    const struct BattalionMapObject *safeZone;
    size_t index;

    if (archetype == NULL)
        return 0;
    actor = (struct runtimeActor *)amalloc(sizeActor, arena);
    if (actor == NULL)
        return 0;
    memset(actor, 0, sizeof(*actor));
    actor->x = x;
    actor->z = z;
    actor->y = PLANEY + archetype->verticalOffset;
    actor->theta = heading;
    actor->range = archetype->weaponRange;
    actor->minrange = archetype->weaponMinRange;
    actor->finalR = archetype->weaponRange;
    actor->speed = archetype->speed;
    actor->damage = archetype->health;
    actor->number = currentActorNumber++;
    actor->dataArchetype = archetype;
    actor->dataObject = object;
    strcpy(actor->dataId, id);
    strcpy(actor->dataGroup, group);
    entityAgentInitialize(&actor->dataAgent, archetype,
                          x, z, heading, waypoints, waypointCount,
                          patrolMode, &gameFactions, faction);
    if (archetype->kind == ENTITY_KIND_ZONE)
        entityAgentSetAreaSensor(&actor->dataAgent, mapAreaContains,
                                 (void *)object);
    if (object != NULL &&
        object->alarmResponse != ENTITY_ALARM_INHERIT)
        {
        safeZone = NULL;
        for (index = 0; index < battlefieldMap.objectCount; index++)
            if (strcmp(battlefieldMap.objects[index].id,
                       object->safeZone) == 0)
                safeZone = &battlefieldMap.objects[index];
        entityAgentSetAlarmPolicy(
            &actor->dataAgent, object->alarmResponse,
            safeZone != NULL ? safeZone->x : 0.0f,
            safeZone != NULL ? safeZone->z : 0.0f,
            object->safeWaitSeconds, (unsigned long)actor->number + 1UL);
        }
    actor->dataAgent.active = active;
    actor->next = allActors->next;
    allActors->next = actor;
    return 1;
    }

static int addMapObject(const struct BattalionMapObject *object,
                        struct runtimeActor *allActors)
    {
    const struct EntityArchetype *archetype;
    const struct BattalionMapObject *route;
    const struct EntityWaypoint *waypoints;
    const char *faction;
    size_t waypointCount;
    size_t index;

    archetype = entityCatalogFind(&gameEntities, object->archetype);
    if (!entityArchetypeIsActor(archetype))
        return 1;
    route = NULL;
    waypoints = NULL;
    waypointCount = 0;
    if (object->patrolRoute[0] != '\0')
        for (index = 0; index < battlefieldMap.objectCount; index++)
            if (strcmp(battlefieldMap.objects[index].id,
                       object->patrolRoute) == 0)
                route = &battlefieldMap.objects[index];
    if (route != NULL)
        {
        waypoints = route->waypoints;
        waypointCount = route->waypointCount;
        }
    faction = object->faction[0] != '\0' ? object->faction :
              archetype->faction;
    return addConfiguredEntity(
        archetype, object->id, object->group,
        object->x, object->z, object->heading * DEG_TO_RAD, object->active,
        waypoints, waypointCount, object->patrolMode,
        faction, object, allActors);
    }

static int configuredEntityCount(struct runtimeActor *allActors,
                                 const struct EntityArchetype *archetype)
    {
    struct runtimeActor *actor;
    int count;

    count = 0;
    for (actor = allActors->next; actor != NULL; actor = actor->next)
        if (actor->dataArchetype == archetype)
            count++;
    return count;
    }

static const struct EntityArchetype *selectSpawnArchetype(
    struct runtimeActor *allActors, int counter)
    {
    const struct EntityArchetype *archetype;
    size_t index;
    int totalWeight;
    int selected;

    totalWeight = 0;
    for (index = 0; index < gameEntities.count; index++)
        {
        archetype = &gameEntities.entries[index];
        if (archetype->spawnWeight > 0 && counter >= archetype->spawnAfter &&
            (archetype->maxActive == 0 ||
             configuredEntityCount(allActors, archetype) <
                 archetype->maxActive))
            totalWeight += archetype->spawnWeight;
        }
    if (totalWeight == 0)
        return NULL;
    selected = rand() % totalWeight;
    for (index = 0; index < gameEntities.count; index++)
        {
        archetype = &gameEntities.entries[index];
        if (archetype->spawnWeight <= 0 || counter < archetype->spawnAfter ||
            (archetype->maxActive > 0 &&
             configuredEntityCount(allActors, archetype) >=
                 archetype->maxActive))
            continue;
        if (selected < archetype->spawnWeight)
            return archetype;
        selected -= archetype->spawnWeight;
        }
    return NULL;
    }

static int configuredSpawnBlocked(float x, float z,
                                  const struct EntityArchetype *archetype,
                                  struct runtimeActor *allActors,
                                  struct targetInfo *targetList)
    {
    struct runtimeMapObject *tree;
    struct runtimeActor *actor;
    struct targetInfo *target;
    float bottom;
    float radius;
    float top;

    bottom = PLANEY + archetype->verticalOffset +
             entityArchetypeCollisionBottomOffset(archetype);
    top = PLANEY + archetype->verticalOffset +
          entityArchetypeCollisionTopOffset(archetype);
    radius = entityArchetypeCollisionRadius(archetype) +
             archetype->spawnClearance;

    for (tree = mapObjectList->next; tree != NULL; tree = tree->next)
        if (tree->intact && mapObjectCollisionRadius(tree) > 0.0f &&
            worldCylindersOverlap(
                x, z, radius, bottom, top,
                tree->x, tree->z, mapObjectCollisionRadius(tree),
                tree->y, mapObjectCollisionTop(tree)))
            return 1;
    for (actor = allActors->next; actor != NULL; actor = actor->next)
        if (actor->damage > 0 && actorCollisionRadius(actor) > 0.0f &&
            worldCylindersOverlap(
                x, z, radius, bottom, top,
                actor->x, actor->z, actorCollisionRadius(actor),
                actorCollisionBottom(actor), actorCollisionTop(actor)))
            return 1;
    if (playerEntityState.dead != 1 && worldCylindersOverlap(
            x, z, radius, bottom, top,
            playerEntityState.x, playerEntityState.z,
            playerEntityState.width,
            playerEntityState.bottom, playerEntityState.height))
        return 1;
    for (target = targetList->next; target != NULL; target = target->next)
        if (target->player.dead != 1 && worldCylindersOverlap(
                x, z, radius, bottom, top,
                target->x, target->z, target->player.width,
                target->player.bottom, target->player.height))
            return 1;
    return 0;
    }

static int addRandomConfiguredEntity(struct targetInfo *targetList,
                                     struct runtimeActor *allActors, int counter)
    {
    const struct EntityArchetype *archetype;
    struct targetInfo *target;
    float minX;
    float maxX;
    float minZ;
    float maxZ;
    float x;
    float z;
    float heading;
    struct EntityContact spawnContact;
    int side;
    int attempt;
    char id[ENTITY_NAME_SIZE];

    archetype = selectSpawnArchetype(allActors, counter);
    if (archetype == NULL)
        return 0;
    minX = playerEntityState.x - (PLANESIZE + 1);
    maxX = playerEntityState.x + (PLANESIZE + 1);
    minZ = playerEntityState.z - (PLANESIZE + 1);
    maxZ = playerEntityState.z + (PLANESIZE + 1);
    for (target = targetList->next; target != NULL; target = target->next)
        {
        if (target->x - PLANESIZE - 1 < minX)
            minX = target->x - PLANESIZE - 1;
        if (target->x + PLANESIZE + 1 > maxX)
            maxX = target->x + PLANESIZE + 1;
        if (target->z - PLANESIZE - 1 < minZ)
            minZ = target->z - PLANESIZE - 1;
        if (target->z + PLANESIZE + 1 > maxZ)
            maxZ = target->z + PLANESIZE + 1;
        }
    for (attempt = 0; attempt < 1000; attempt++)
        {
        side = rand() % 4;
        x = minX + (maxX - minX) * (float)(rand() % 1001) / 1000.0f;
        z = minZ + (maxZ - minZ) * (float)(rand() % 1001) / 1000.0f;
        if (side == 0) z = minZ;
        else if (side == 1) x = minX;
        else if (side == 2) z = maxZ;
        else x = maxX;
        if (!configuredSpawnBlocked(
                x, z, archetype, allActors, targetList))
            {
            heading = (float)atan2((double)(playerEntityState.x - x),
                                   (double)(playerEntityState.z - z));
            sprintf(id, "spawn-%d", currentActorNumber);
            if (!addConfiguredEntity(
                    archetype, id, "", x, z, heading, 1,
                    NULL, 0, ENTITY_PATROL_LOOP, archetype->faction,
                    NULL, allActors))
                return 0;
            initializeContact(&spawnContact, "player",
                              runtimePlayerFaction(),
                              playerEntityState.x,
                              (playerEntityState.height +
                               playerEntityState.bottom) * 0.5f,
                              playerEntityState.z, 1, 0, 0);
            entityAgentQueueTrigger(&allActors->next->dataAgent,
                                    "spawned", &spawnContact);
            return 1;
            }
        }
    return 0;
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* update all of the explosions                                  */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void updateRuntimeExplosions(struct runtimeExplosion * explosions, struct runtimeActor * actorlist)
    {
    struct runtimeExplosion * currentExplosion,  * removedExplosion, *explosion;
    struct runtimeActor *tempactor;

    explosion = explosions;

    while (explosion->next != NULL)
	{
	currentExplosion = explosion->next;
	currentExplosion->count += 1;

	/***********************/
	/* make some noise     */
	/***********************/

	if (currentExplosion->damaging && currentExplosion->archetype->fireSound[0] != '\0' &&
	    currentExplosion->count == currentExplosion->archetype->effectSoundFrame)
	    doSoundKeyAt(currentExplosion->archetype->fireSound,
	                 currentExplosion->x, currentExplosion->y, currentExplosion->z);

	/**********************************/
	/* check for another actor hit     */
	/**********************************/

	if (currentExplosion->damaging && currentExplosion->archetype->effectDamage > 0 &&
	    currentExplosion->count == currentExplosion->archetype->effectDamageFrame)
	    {
	    for(tempactor = actorlist; tempactor->next != NULL; tempactor = tempactor->next)
		if (actorHit(tempactor->next, currentExplosion->x, currentExplosion->y, currentExplosion->z))
		    {
		    tempactor->next->damage -= currentExplosion->archetype->effectDamage;
		    notifyDataEntityDamageFromPlayer(tempactor->next,
			currentExplosion->owner, currentExplosion->x, currentExplosion->z);
		    if (tempactor->next->dataArchetype != NULL)
			addScore(tempactor->next->dataArchetype->score,
			         currentExplosion->owner);
		    }
	    }

	/*******************/
	/* remove boom     */
	/*******************/

	if (currentExplosion->count >= currentExplosion->archetype->effectDuration)
	    {
	    removedExplosion = explosion->next;
	    explosion->next = explosion->next->next;
	    afree(removedExplosion, arena);
	    }
	else
	    explosion = explosion->next;
	}
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* reset everything for a new game                               */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void setPlayConditions()
    {
    struct runtimeExplosion * explosion,  *explosionNext;
    struct runtimeActor * tactor,  *tempactor;
    struct runtimeMapObject * object,  *mapObjectNode;
    struct runtimeProjectile * killer,  *temp;
    struct targetInfo * ttarget, *temptarg;

    int mapObjectNumber;
    size_t mapRecord;
    const struct BattalionMapObject *mapObject;
    const struct EntityArchetype *mapArchetype;

    if (mode == DEMOMODE)
	showDemoMenu = 0;

    moreThanOne = 0;

    setUnconnected();

    if (!ensureDestroyedMapObjectCapacity(battlefieldMap.objectCount))
	{
	showError("Out of memory tracking destroyed map objects");
	exit(1);
	}

    /******************************/
    /* reset all the linked lists */
    /******************************/

    tempactor = actorlist;
    while(tempactor != NULL)
	    {
	    tactor = tempactor;
	    tempactor= tempactor->next;
	    afree(tactor, arena);
	    }

    explosion = runtimeExplosionList;
    while (explosion != NULL)
	    {
	    explosionNext = explosion;
	    explosion = explosion->next;
	    afree(explosionNext, arena);
	    }

    temp = runtimeProjectileList;
    while (temp != NULL)
	    {
	    killer = temp;
	    temp = temp->next;
	    afree(killer, arena);
	    }

    tempactor = wrecklist;
    while(tempactor != NULL)
	    {
	    tactor = tempactor;
	    tempactor = tempactor->next;
	    afree(tactor, arena);
	    }

    mapObjectNode = mapObjectList;
    while(mapObjectNode != NULL)
	    {
	    object = mapObjectNode;
	    mapObjectNode = mapObjectNode->next;
	    afree(object, arena);
	    }

    temptarg = targets;
    while(temptarg != NULL)
	    {
	    ttarget = temptarg;
	    temptarg= temptarg->next;
	    afree(ttarget, arena);
	    }


    /******************************/
    /* set up the dummy head nodes*/
    /******************************/

    runtimeProjectileList = (struct runtimeProjectile *) acalloc(1, runtimeProjectileSize, arena);
    runtimeExplosionList = (struct runtimeExplosion *) acalloc(1, runtimeExplosionSize, arena);
    actorlist = (struct runtimeActor *) acalloc(1, sizeActor, arena);
    wrecklist = (struct runtimeActor *) acalloc(1, sizeActor, arena);
    mapObjectList = (struct runtimeMapObject *) acalloc(1, sizeof(struct runtimeMapObject), arena);
    targets = (struct targetInfo *) acalloc(
	1, sizeof(struct targetInfo), arena);
    if (runtimeProjectileList == NULL || runtimeExplosionList == NULL || actorlist == NULL ||
	wrecklist == NULL || mapObjectList == NULL || targets == NULL)
	{
	showError("Out of memory initializing gameplay lists");
	exit(1);
	}

    /************************************************************/
    /* read in data file of structures (trees, buildings, etc.) */
    /************************************************************/

    mapObjectRuntimeId = 0;
    for (mapRecord = 0; mapRecord < battlefieldMap.objectCount; mapRecord++)
	    {
	    mapObject = &battlefieldMap.objects[mapRecord];
	    mapArchetype = entityCatalogFind(&gameEntities,
	                                     mapObject->archetype);
	    if (!entityArchetypeIsMapObject(mapArchetype))
		continue;
	    mapObjectNode = (struct runtimeMapObject *)amalloc(sizeof(struct runtimeMapObject), arena);
	    if (mapObjectNode == NULL)
		continue;
	    memset(mapObjectNode, 0, sizeof(*mapObjectNode));
	    mapObjectNode->x = mapObject->x;
	    mapObjectNode->y = PLANEY + mapObject->y +
	                  mapArchetype->verticalOffset +
	                  mapArchetype->modelGroundOffset * mapObject->scale;
	    mapObjectNode->z = mapObject->z;
	    mapObjectNode->height = mapArchetype->collisionHeight * mapObject->scale;
	    mapObjectNode->death2 = -1;
	    mapObjectNode->intact = mapObject->active;
	    mapObjectNode->heading = mapObject->heading;
	    mapObjectNode->instanceScale = mapObject->scale;
	    mapObjectNode->dataArchetype = mapArchetype;
	    mapObjectNode->dataObject = mapObject;
	    mapObjectNode->number = mapObjectRuntimeId++;
	    mapObjectNode->next = mapObjectList->next;
	    mapObjectList->next = mapObjectNode;
	    }

	/**************************************************************************/
	/* eliminate all trees that have been destroyed already in a network game */
	/**************************************************************************/

	if (netUp)
	    for (mapObjectNode = mapObjectList->next; mapObjectNode != NULL; mapObjectNode = mapObjectNode->next)
		for(mapObjectNumber=0; mapObjectNumber< destroyedMapObjectHistoryCount; mapObjectNumber++)
		    if (mapObjectNode->number == destroyedMapObjectHistory[mapObjectNumber])
			mapObjectNode->intact = 0;

    /***********************************************/
    /* read in data file of prepositioned vehicles */
    /***********************************************/

    if (!netUp && !client)
	{
	for (mapRecord = 0; mapRecord < battlefieldMap.objectCount;
	     mapRecord++)
	    if (!addMapObject(&battlefieldMap.objects[mapRecord], actorlist))
		showError("Could not create a data-driven map object");
	}

    /*********************************/
    /* reset general game parmeters  */
    /*********************************/

    accuracy		    = STARTACCURACY;
    arrivalTime		    = ARRIVALRATE;
    firingDelay		    = STARTFIRINGDELAY;
    mainCounter		    = 0;
    playerWeaponReloadRemaining = 0;

    xrot = xsaverot	    = 300;
    yrot = ysaverot	    = 0;

    if (!netUp && !client)
	destroyedMapObjectHistoryCount = 0;

    playerEntityState = resetPlayerState(playerEntityState);
    playerBodyHeading = playerEntityState.headHorzRotate;


/***********************************************************
 code to print out the state of the shared memory arena

	{
	struct mallinfo mi;

	mi = amallinfo(arena);
	fprintf(stderr, "--------------------------------------------------------------------------------\n");
	fprintf(stderr, "%8d %8d %8d %8d %8d %8d %8d %8d %8d -start\n",
	    mi.arena, mi.ordblks, mi.smblks, mi.hblkhd,  mi.hblks,
	    mi.usmblks, mi.fsmblks, mi.uordblks, mi.fordblks);
	fprintf(stderr, "--------------------------------------------------------------------------------\n");
	fflush(stderr);
	}

***********************************************************/
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void addRandomCatalogEntity()
    {
    addRandomConfiguredEntity(targets, actorlist, mainCounter);
    }


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* initialize everything at the start of battalion               */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void initialization()
    {
    struct passwd *passwd;
    struct tm* theTime;
    FILE * hiScoreFile;
    int i;
    GLint gdtmp;
    GLint tmp[4];
    GLenum type;
    char textBuffer[80];
    char * dataPtr;
    char * playerPointer;
    char scoredataPath[MAXPATH];
    char garbage;

    /******************************/
    /* set up sone network stuff  */
    /******************************/

    setHostAddr("");
    setPortNumber(DEFAULT_PORT);

    setUnconnected();

    netUp			    = 0;
    client			    = 0;

    destroyedMapObjectHistoryCount	    = 0;
    destroyedMapObjectFrameCount    = 0;
    currentActorNumber		    = 1;
    biggestActorNumberSent	    = 0;

    totalCounter		    = 0;
    runtimeExplosionSize	    = sizeof(struct runtimeExplosion);
    sizeActor	    = sizeof(struct runtimeActor);
    runtimeProjectileSize  = sizeof(struct runtimeProjectile);

    /************************************/
    /* set up the window                */
    /************************************/

    /* TK_INDIRECT gives me a 50% boost in speed! on a an indy */

    /* removing the depth buffer from this line has no effect */
    /* switching to TK_SINGLE almost works in Vector graphics mode - BIG speedup there */

    type = TK_INDIRECT | TK_DOUBLE /* TK_SINGLE*/ | TK_RGB| TK_DEPTH;
    tkInitDisplayMode(type);

#ifdef MESAVERSION
    tkInitPosition(-1, -1, 352, 275);
#else
    tkInitPosition(-1, -1, 1280, 720);
#endif

    tkIdleFunc(id);
    tkExposeFunc(reshape);
    tkReshapeFunc(reshape);
    tkKeyDownFunc(processKey);
    tkKeyUpFunc(processKeyRelease);
    tkMouseDownFunc(MouseDown);
    tkMouseUpFunc(MouseUp);
    tkFocusFunc(inputFocusChanged);

    if (tkInitWindow("battalion") == GL_FALSE)
	{
	fprintf(stderr, "could not open a window\n");
	exit(1);
	}

    pointerGrab = 0;

    /************************************/
    /* get information on the player    */
    /************************************/

    strcpy(playerName, "Anonymous");

    /* playerPointer = cuserid(NULL); not safe anymore */
    playerPointer = getlogin();

    if (playerPointer == NULL)
	playerPointer = getlogin();

    if (playerPointer != NULL)
	strcpy(playerName, playerPointer);

    passwd = getpwuid(getuid());
    if (passwd != NULL && passwd->pw_dir != NULL && *(passwd->pw_dir) != '\0')
	{
	strcpy(playerHome, passwd->pw_dir);
	playerHome[strlen(playerHome)] = '\0';
	}
    else
	{
	playerHome[0] = '\0';
	}

    /************************************/
    /* set up the graphics              */
    /************************************/

    /* if line widths are integral Mesa chokes in a large window */
    glLineWidth(1.01);

    glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);
    glHint(GL_POINT_SMOOTH_HINT, GL_FASTEST);

    glFogi(GL_FOG_MODE,  GL_LINEAR);

    glDisable(GL_POINT_SMOOTH);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_POLYGON_SMOOTH);

    glDisable(GL_ALPHA_TEST);
    glDisable(GL_AUTO_NORMAL);
    glDisable(GL_BLEND);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHTING);
    glDisable(GL_LINE_STIPPLE);
    glDisable(GL_NORMALIZE);
    glDisable(GL_POLYGON_STIPPLE);

    glDisable(GL_FOG);

    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);


/* this helps zbuffering in mesa */

#ifdef MESAVERSION
    glDepthRange(0,1);
    glDepthFunc(GL_LEQUAL);
#endif


    glPointSize(2);
    goToHighDetail();

    glMatrixMode(GL_MODELVIEW);


    glClearDepth(1);
    glClearColor(0, 0, 0, 1);

    glBlendFunc(GL_SRC_ALPHA,  GL_ONE_MINUS_SRC_ALPHA);

     /**************************/
    /* stereo stuff           */
    /**************************/

	/* OGLXXX
	 * getsize not supported -- See Window Manager
	 * getsize(&windowSizeX, &windowSizeY);
	 */

	/* OGLXXX
	 * getorigin not supported -- See Window Manager
	 * getorigin(&windowOriginX, &windowOriginY);
	 */

	glGetIntegerv(GL_VIEWPORT, tmp);
	viewL=tmp[0];
	viewR=tmp[0]+tmp[2]-1;

	/* OGLXXX
	 * getmonitor not supported
	 * getmonitor()
	 */
    monitorType = 0; /*THIS IS BAD*/


    playerLookat =  makePlayerLookat();
    overviewLookat =  makeOverviewLookat();

    /*************************/
    /* initialize indicators */
    /*************************/

    alone	    = 0;
    noSound	    = 0;
    turnSoundOff();
    turnMusicOff();
    no3D	    = 0;
    mode3D	    = 0;
    showOptions	    = 0;
    showDemoMenu    = 0;
    backdrop	    = 0;
    showframes	    = 0;
    paused	    = 0;

#ifdef MESAVERSION
    lod		    = 1;
#else
    lod		    = 2;
#endif

#ifdef DETAIL
    lod		    = DETAIL;
#endif

    mode	    = DEMOMODE;
    view	    = OMNISCIENTVIEW;
    oldview	    = 99;

    timePaused	    = 0;
    time3d	    = 0;
    timeSound	    = 0;
    timeMusic	    = 0;
    timeDetail	    = 0;

    mapHeight	    = 35;

    /************************************/
    /* initialize all the linked lists  */
    /************************************/

    runtimeProjectileList   = NULL;
    runtimeExplosionList	    = NULL;
    actorlist	    = NULL;
    wrecklist	    = NULL;
    mapObjectList	    = NULL;

    initSounds();

    /************************************/
    /* get the current date/time        */
    /************************************/

    beg = time(NULL);
    theTime = localtime(&beg);
    worldEnvironmentSetHour(&worldEnvironment,
        (float)theTime->tm_hour + (float)theTime->tm_min / 60.0f);

    /*************************************/
    /* if its xmas use the xmas graphics */
    /*************************************/

    if ((theTime->tm_mday == 25) && (theTime->tm_mon == 11))
	itsChristmas = 1;
    else
	itsChristmas = 0;


    /**************************************/
    /* initialize random number generator */
    /**************************************/

    srand( (unsigned) beg);


    /************************************/
    /* check graphics hardware          */
    /************************************/

    glGetIntegerv(GL_DEPTH_BITS, &gdtmp);
    if (gdtmp == GL_FALSE)
	showError("Z-buffer not available on this machine");

    glGetIntegerv(GL_RED_BITS, &gdtmp);
    if (gdtmp == GL_FALSE)
	showError("Double buffered RGB not available on this machine");

    /********************************************/
    /* find where all the data files are stored */
    /********************************************/

    dataPtr = getenv("BATTALIONDATADIR");
    if (!selectAssetRoot(dataPtr) && !selectAssetRoot("./assets"))
	{
	showError("Could not find assets/manifest.txt\n");
	exit(1);
	}

    /************************************/
    /* build all the objects            */
    /************************************/

    if (!battalionMapResolve(dataPath, textBuffer, sizeof(textBuffer)))
	{
	showError(textBuffer);
	exit(1);
	}

    battalionMapPackageInit(&battlefieldMap);
    if (!battalionMapBuildPackagePath(dataPath, fullPath, sizeof(fullPath)))
	{
	showError("Map package path is too long");
	exit(1);
	}
    if (!battalionMapPackageLoad(fullPath, &battlefieldMap,
				 textBuffer, sizeof(textBuffer)))
	{
	showError(textBuffer);
	exit(1);
	}
    if (strcmp(battlefieldMap.id, battalionMapId()) ||
	strcmp(battlefieldMap.name, battalionMapName()))
	{
	showError("Map package identity does not match selection");
	exit(1);
	}

    makeObjects(dataPath);
    if (!loadGameEntityCatalog(textBuffer, sizeof(textBuffer)))
	{
	showError(textBuffer);
	exit(1);
	}

    /***************************************/
    /* see if this machine can play sounds */
    /***************************************/

    checkSound(dataPath);


    /************************************/
    /* see if a high score file exists  */
    /* if not try and create one        */
    /************************************/

    multipleHighScores = 1;

    /* If BATTALIONSCOREUNIQUE is defined, only put the
       top entry for each user in the high score list.
     */

    if (getenv("BATTALIONSCOREUNIQUE"))
	{
	multipleHighScores = 0;
	}

    hiScoreFile = NULL;

    dataPtr = getenv("BATTALIONSCOREDIR");
    if (dataPtr != NULL)
	strcpy(scoredataPath, dataPtr);
    else
	strcpy(scoredataPath, "/var/tmp");

    if (scoredataPath[strlen(scoredataPath)-1] != '/')
	strcat(scoredataPath, "/");

    strcpy(scorefullPath, scoredataPath);
    strcat(scorefullPath, "battalion_hiscore");
    hiScoreFile = fopen(scorefullPath, "r");

    if (hiScoreFile == NULL)
	{
	hiScoreFile = fopen(scorefullPath, "w");

	if (hiScoreFile == NULL)
	    {
	    sprintf(textBuffer, "Couldn't create high score file: %s", scorefullPath);
	    showError(textBuffer);
	    }
	else
	    {
	    for(i=0;i<3;i++)
		fprintf(hiScoreFile, "-1 -\n");

	    for (i = 0; i < 3; i++)
		{
		highScores[i].number = -1;
		highScores[i].name[0] = 0;
		}

	    fclose(hiScoreFile);

# ifdef SOLARIS

	    /* Make hiscore file read/writeable for everyone */
	    if (chmod(scorefullPath, 0666))
		{
		sprintf(textBuffer, "Couldn't change permission of high score file: %s", scorefullPath);
		showError(textBuffer);
		}
# endif
	    }
	}
    else
	{
	for (i = 0; i < 3; i++)
	    if (fscanf(hiScoreFile, "%d%c%255s",
	               &highScores[i].number, &garbage,
	               highScores[i].name) != 3)
		{
		highScores[i].number = -1;
		highScores[i].name[0] = 0;
		}

	fclose(hiScoreFile);
	}


    playerEntityState.dataArchetype = playerEntityArchetype;

    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* go to 3d-video mode                                           */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void goto3d(void)
    {
    mode3D = CAMERA_STEREO_TOP_BOTTOM;
    no3D = 0;
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* go to 1d-video mode                                           */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void goto1d(void)
    {
    mode3D = CAMERA_STEREO_OFF;
    }

static void setStereoLayout(int layout)
    {
    if (layout != CAMERA_STEREO_TOP_BOTTOM &&
        layout != CAMERA_STEREO_SIDE_BY_SIDE)
        layout = CAMERA_STEREO_OFF;
    mode3D = layout;
    no3D = 0;
    }

static void cycleStereoLayout(void)
    {
    if (mode3D == CAMERA_STEREO_OFF)
        setStereoLayout(CAMERA_STEREO_TOP_BOTTOM);
    else if (mode3D == CAMERA_STEREO_TOP_BOTTOM)
        setStereoLayout(CAMERA_STEREO_SIDE_BY_SIDE);
    else
        setStereoLayout(CAMERA_STEREO_OFF);
    }


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* check the current location of the mouse in the window         */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void updatePlayerFire(void)
    {
    int firePressed;

    firePressed = playerInput.fireMouse || playerInput.fireControlLeft ||
	          playerInput.fireControlRight;
    playerEntityState.beamOn = (mode == PLAYMODE) && !paused &&
	firePressed;
    if (!firePressed)
	playerProjectileTriggerLatched = 0;
    }

void checkMouse()
    {
    float mouseDeltaX, mouseDeltaY;
    int forward, sideways, steering;
    int steeringDelta;
    int pitchMinimum, pitchMaximum;
    float horizontalAim, verticalAim;

    mouseDeltaX = mouseDeltaY = 0;
    tkGetMouseDelta(&mouseDeltaX, &mouseDeltaY);

    playerEntityState.movingForward = playerEntityState.movingBackward = 0;
    playerMoveActive = 0;
    offsetX = 0;

    if ((mode == PLAYMODE) && !paused)
	{
	horizontalAim = pointerGrab ? -mouseDeltaX * 4.0 : 0;
	verticalAim = pointerGrab ? mouseDeltaY * 4.0 : 0;

	if (playerInput.aimLeft)
	    horizontalAim += 18;
	if (playerInput.aimRight)
	    horizontalAim -= 18;
	if (playerInput.aimUp)
	    verticalAim -= 18;
	if (playerInput.aimDown)
	    verticalAim += 18;

	if ((horizontalAim != 0) || (verticalAim != 0))
	    playerEntityState.moving = 1;

	playerEntityState.headHorzRotate += (int) horizontalAim;
	playerEntityState.headVertRotate += (int) verticalAim;
	if (playerEntityArchetype != NULL)
	    {
	    pitchMinimum = (int)(playerEntityArchetype->aimPitchMin * 10.0f);
	    pitchMaximum = (int)(playerEntityArchetype->aimPitchMax * 10.0f);
	    if (playerEntityState.headVertRotate < pitchMinimum)
		playerEntityState.headVertRotate = pitchMinimum;
	    if (playerEntityState.headVertRotate > pitchMaximum)
		playerEntityState.headVertRotate = pitchMaximum;
	    }

	if (playerEntityState.xspeed != 0)
	    offsetX = -horizontalAim / playerEntityState.xspeed;
	if (offsetX > 1)
	    offsetX = 1;
	else if (offsetX < -1)
	    offsetX = -1;

	forward = playerInput.forward - playerInput.backward;
	sideways = playerInput.right - playerInput.left;
	if (playerEntityArchetype != NULL &&
	    playerEntityArchetype->controlMode == ENTITY_CONTROL_STEERED)
	    {
	    steering = playerInput.left - playerInput.right;
	    if (steering != 0)
		{
		steeringDelta = entitySteeredHeadingDelta(
		    playerInput.left, playerInput.right,
		    playerEntityArchetype->playerSteerRate);
		playerBodyHeading += steeringDelta;
		playerEntityState.headHorzRotate += steeringDelta;
		playerEntityState.moving = 1;
		}
	    if (playerBodyHeading < 0)
		playerBodyHeading += 3600;
	    if (playerBodyHeading >= 3600)
		playerBodyHeading -= 3600;
	    if (forward != 0)
		{
		playerMoveAngle = playerBodyHeading * BIG_DEG_TO_RAD;
		if (forward < 0)
		    playerMoveAngle += PI;
		playerMoveActive = 1;
		playerEntityState.moving = 1;
		if (forward < 0)
		    playerEntityState.movingBackward = 1;
		else
		    playerEntityState.movingForward = 1;
		}
	    }
	else
	    {
	    if ((view == PLAYERVIEW) || (view == FIRSTPERSONVIEW))
		sideways = -sideways;
	    if (forward || sideways)
		{
		playerMoveAngle = playerEntityState.headHorzRotate * BIG_DEG_TO_RAD +
		    atan2((double) sideways, (double) forward);
		playerMoveActive = 1;
		playerEntityState.moving = 1;
		if ((forward < 0) && !sideways)
		    playerEntityState.movingBackward = 1;
		else
		    playerEntityState.movingForward = 1;
		}
	    }
	}

    updatePlayerFire();
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void clearPlayerInput(void)
    {
    float unusedX, unusedY;

    memset(&playerInput, 0, sizeof(playerInput));
    playerMoveActive = 0;
    playerEntityState.movingForward = playerEntityState.movingBackward = 0;
    playerEntityState.beamOn = 0;
    playerProjectileTriggerLatched = 0;
    tkGetMouseDelta(&unusedX, &unusedY);
    }

static void setPointerCaptured(int captured)
    {
    float unusedX, unusedY;

    if (captured && !pointerGrab)
	{
	grabPointer();
	pointerGrab = 1;
	}
    else if (!captured && pointerGrab)
	{
	unGrabPointer();
	pointerGrab = 0;
	}
    tkGetMouseDelta(&unusedX, &unusedY);
    }

static void inputFocusChanged(int focused)
    {
    if (focused)
	return;

    clearPlayerInput();
    memset(commandKeys, 0, sizeof(commandKeys));
    setPointerCaptured(0);
    if (mode == PLAYMODE)
	paused = 1;
    }

static int setPlayerKeyState(int key, int pressed)
    {
    switch(key)
	{
	case TK_w:
	case TK_W:	playerInput.forward = pressed; break;
	case TK_s:
	case TK_S:	playerInput.backward = pressed; break;
	case TK_a:
	case TK_A:	playerInput.left = pressed; break;
	case TK_d:
	case TK_D:	playerInput.right = pressed; break;
	case TK_i:
	case TK_I:	playerInput.aimUp = pressed; break;
	case TK_j:
	case TK_J:	playerInput.aimLeft = pressed; break;
	case TK_k:
	case TK_K:	playerInput.aimDown = pressed; break;
	case TK_l:
	case TK_L:	playerInput.aimRight = pressed; break;
	case TK_CONTROL_L:	playerInput.fireControlLeft = pressed; break;
	case TK_CONTROL_R:	playerInput.fireControlRight = pressed; break;
	default:	return 0;
	}
    return 1;
    }

static int commandKeyIndex(int key)
    {
    switch(key)
	{
	case TK_F1: return 0;
	case TK_F2: return 1;
	case TK_F3: return 2;
	case TK_F4: return 3;
	case TK_p:
	case TK_P: return 4;
	case TK_v:
	case TK_V: return 5;
	case TK_F5: return 6;
	default: return -1;
	}
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLenum MouseDown(int mouseX, int mouseY, GLenum button)
   {
    (void) mouseX;
    (void) mouseY;

    if ((mode == PLAYMODE) && (button == TK_LEFTBUTTON))
	{
	if (paused || !pointerGrab)
	    {
	    clearPlayerInput();
	    paused = 0;
	    setPointerCaptured(1);
	    }
	else
	    playerInput.fireMouse = 1;
	}

    updatePlayerFire();

    return 0;
   }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLenum MouseUp(int mouseX, int mouseY, GLenum button)
   {
    (void) mouseX;
    (void) mouseY;

    if (button == TK_LEFTBUTTON)
	playerInput.fireMouse = 0;

    updatePlayerFire();

    return 0;
   }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void demoKeys(int key)
    {
    const struct EntityArchetype *shortcutEntity;

    shortcutEntity = entityCatalogFindShortcut(&gameEntities, key);
    if (shortcutEntity != NULL)
	{
	setPlayerEntityArchetype(shortcutEntity);
	mode = PLAYMODE;
	showOptions = 0;
	showHelp = 0;
	setPlayConditions();
	clearPlayerInput();
	setPointerCaptured(1);
	return;
	}

    switch(key)
	{
	case TK_SPACE:  showOptions = 1;
			break;

	case TK_UP:
	case TK_LEFT:   if (showOptions)
				stepPlayerEntitySelection(-1);
			break;

	case TK_DOWN:
	case TK_RIGHT:  if (showOptions)
				stepPlayerEntitySelection(1);
			break;

	case TK_RETURN: if (showOptions && playerEntityArchetype != NULL)
			    {
			    mode = PLAYMODE;
			    showOptions = 0;
			    showHelp = 0;
			    setPlayConditions();
			    clearPlayerInput();
			    setPointerCaptured(1);
			    }
			break;

	case TK_ESCAPE: showOptions = 0;
			showDemoMenu = 1;
			break;

	}
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void playKeys(int key)
    {
    switch(key)
	{
	case TK_1:	view = PLAYERVIEW;
			    break;

	case TK_2:	view = OMNISCIENTVIEW;
			if (oldview != OMNISCIENTVIEW)
			    {
			    yrot = ysaverot;
			    xrot = xsaverot;
			    }

			    break;

	case TK_3:	view = ARMYVIEW;
			    break;

	case TK_4:	view = MAPVIEW;
			    break;

	case TK_5:	view = FIRSTPERSONVIEW;
			    break;

	case TK_UP:	xrot -= 8;
			    break;

	case TK_LEFT:	yrot += 8;
			    break;

	case TK_DOWN:	xrot += 8;
			    break;

	case TK_RIGHT:	yrot -= 8;
			    break;

	}
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLenum processKey(int key, GLenum mask)
    {
    int commandIndex;

    (void) mask;

    if (setPlayerKeyState(key, 1))
	{
	updatePlayerFire();
	return GL_TRUE;
	}

    commandIndex = commandKeyIndex(key);
    if (commandIndex >= 0)
	{
	if (commandKeys[commandIndex])
	    return GL_TRUE;
	commandKeys[commandIndex] = 1;
	}

    if (mode == DEMOMODE)
	demoKeys(key);

    if (mode == PLAYMODE)
	 playKeys(key);

    switch(key)
	{
	case TK_p:
	case TK_P:	    paused = !paused;
			    timePaused = now;
			    clearPlayerInput();
			    if (mode == PLAYMODE)
				setPointerCaptured(!paused);
			    break;

	case TK_F4:	    toggleMusic();
			    timeMusic = now;
			    break;

	case TK_F5:	    shadowsEnabled = !shadowsEnabled;
			    break;

	case '[':	    worldEnvironmentAdvance(&worldEnvironment, -1.0f);
			    break;

	case ']':	    worldEnvironmentAdvance(&worldEnvironment, 1.0f);
			    break;

	case TK_F3:	    toggleSound();
			    timeSound = now;
			    break;

	case TK_F2:	    lod++;
			    if (lod > 2)
				lod = -1;
			    timeDetail = now;

			    if (lod == -1)
				goToLowDetail();
			    else
				goToHighDetail();
			    break;

	case TK_v:
	case TK_V:	    cycleStereoLayout();
			    time3d = now;
			    break;

#ifdef SGIVERSION
	case TK_h:
	case TK_H:
			    {
			    const char *showcaseFile;
			    char showcasePath[MAXPATH];
			    pid_t child;

			    if (mode3D)
				goto1d();
			    showcaseFile = assetFilePath("help-sgi-showcase");
			    if (strlen(dataPath) + strlen(showcaseFile) + 1 >
				sizeof(showcasePath))
				{
				showError("Showcase asset path is too long");
				break;
				}
			    strcpy(showcasePath, dataPath);
			    strcat(showcasePath, showcaseFile);
			    child = fork();
			    if (child == 0)
				{
				execlp("showcase", "showcase", "-bfv",
				       showcasePath, (char *)0);
				_exit(127);
				}
			    }
			    break;
#endif

	case TK_PAGEDOWN:   if (mapHeight > 1)
				mapHeight -= 1;
			    break;

	case TK_PAGEUP:     if (mapHeight < 45)
				mapHeight += 1;
			    break;

	case TK_F1:	    showHelp = !showHelp;
			    if (showHelp && (mode == PLAYMODE))
				{
				paused = 1;
				clearPlayerInput();
				setPointerCaptured(0);
				}
			    break;

	case TK_ESCAPE:	    showHelp = 0;
			    if (mode == PLAYMODE)
				{
				paused = 1;
				clearPlayerInput();
				setPointerCaptured(0);
				}
	}


    return GL_TRUE;
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* processKeyRelease                                             */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLenum processKeyRelease(int key)
    {
    int commandIndex;

    commandIndex = commandKeyIndex(key);
    if (commandIndex >= 0)
	commandKeys[commandIndex] = 0;

    setPlayerKeyState(key, 0);
    updatePlayerFire();

    return GL_TRUE;
    }


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* check the user's input                                        */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
void checkInput ()
    {
    int yes3D,  no3D,  yesDump;

    yes3D = no3D = yesDump = 0;



		case VKEY:	if (now > (time3d + 4))
				    {
				    if (mode3D)
					no3D = 1;
				    else
					yes3D = 1;
				    time3d = now;
				    }
				     do not put a qreset here!
				break;


    if ((yes3D) && (!no3D))
	goto3d();

    if ((mode3D) && (no3D))
	goto1d();

    }
*/

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* deal with a dead player (not necessarily your player)       */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void updateDeadPlayer(float x, float z, struct runtimePlayer * playerState)
    {
    const struct EntityArchetype *archetype;
    const struct EntityArchetype *debris;
    const struct EntityArchetype *fire;
    float Vplane;
    int fragment;

    archetype = playerArchetype(playerState);
    playerState->dead = 1;

    if (playerState == &playerEntityState) /* its the player's player that dies */
	view = OMNISCIENTVIEW;

    if (playerState->energyRemaining < 0)
	playerState->energyRemaining = 0;

    /************************/
    /* update dying player */
    /************************/

    playerState->deadCount += 1;

    if (playerState->deadCount > 45)
	{
	playerState->deadCount = 45;
	playerState->timeDead += 1;
	}

    if (playerState->deadCount == 44)
	{
	if (archetype == NULL)
	    return;
	doSoundKeyAt(archetype->destructionSound, x,
	          (playerState->bottom + playerState->height) * 0.5f, z);
	spawnDestructionEffects(
	    archetype, x,
	    (playerState->bottom + playerState->height) * 0.5f, z,
	    entityArchetypeCollisionRadius(archetype),
	    archetype->collisionHeight,
	    playerState, 1);
	debris = entityCatalogFind(&gameEntities,
	                              archetype->destructionProjectile);
	fire = entityCatalogFind(&gameEntities,
	                            archetype->wreckFireProjectile);
	Vplane = sqrt(GRAVITY * 1.5) * 0.7071;
	for (fragment = 0;
	     fragment < archetype->destructionDebrisCount; fragment++)
	    spawnProjectile(x + randy(entityArchetypeCollisionRadius(archetype)),
	                  PLANEY + archetype->verticalOffset +
	                      randy(archetype->collisionHeight * 0.5f) +
	                      archetype->collisionHeight * 0.5f,
	                  z + randy(entityArchetypeCollisionRadius(archetype)), debris,
	                  sin(randy(PI)) * Vplane, Vplane,
	                  cos(randy(PI)) * Vplane, 75.0f, NULL);
	for (fragment = 0;
	     fragment < archetype->destructionFireCount; fragment++)
	    spawnProjectile(x + randy(entityArchetypeCollisionRadius(archetype)),
	                  PLANEY + 0.14f,
	                  z + randy(entityArchetypeCollisionRadius(archetype)), fire,
	                  0.0f, -0.001f, 0.0f, 10000.0f, NULL);
	}
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* update all of the other monsters currently in the game        */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void updateTargets()
    {
    struct runtimeMapObject * mapObjectNode;
    struct runtimeActor * tempactor;
    struct targetInfo * temptarget;
    struct targetInfo * temptarget2;
    struct runtimePlayer  playerState;
    struct runtimeMapObject object;
    float mAngle, obsangle;
    float mRad, mAng;
    float tx, tz;
    float trad, rad;
    int blocked, nearAngle;

    for(temptarget = targets->next; temptarget != NULL; temptarget = temptarget->next)
	{
	playerState  = temptarget->player;
	tx	    = temptarget->x;
	tz	    = temptarget->z;

    /************************************/
    /* deal with the networked monsters */
    /************************************/

    if (temptarget->net_clientFrom != LOCAL_AI_PLAYER)
	{
	playerState.energyRemaining	= temptarget->net_energyRemaining;
	playerState.beamOn		= temptarget->net_beamOn;

	nearAngle = temptarget->net_headHorzRotate;
	if (abs(playerState.headHorzRotate -
	        (temptarget->net_headHorzRotate + 3600)) <
	    abs(playerState.headHorzRotate - nearAngle))
	    nearAngle = temptarget->net_headHorzRotate + 3600;
	if (abs(playerState.headHorzRotate -
	        (temptarget->net_headHorzRotate - 3600)) <
	    abs(playerState.headHorzRotate - nearAngle))
	    nearAngle = temptarget->net_headHorzRotate - 3600;

	playerState.headHorzRotate	= 0.7 * playerState.headHorzRotate + 0.3 * nearAngle;

	playerState.headVertRotate	= 0.9 * playerState.headVertRotate + 0.1 * temptarget->net_headVertRotate;
	temptarget->x			= 0.9 * temptarget->x + 0.1 * temptarget->net_x;
	temptarget->z			= 0.9 * temptarget->z + 0.1 * temptarget->net_z;

	if (playerState.energyRemaining > 0)
	    {
	    playerState.rot1 = cos(playerState.moveCount) * 0.02;
	    playerState.rot2 = sin(playerState.moveCount) * 0.05;
	    }
	else
	    {
	    updateDeadPlayer(temptarget->x, temptarget->z, &playerState);
	    }

	playerState.moving = 1;
	playerState.moveCount += 1;

	/************************************************/
	/* update the weapons of the networked monsters */
	/************************************************/

	if (playerState.beamOn)
	    updatePlayerWeapon(
		temptarget->x, temptarget->z, &playerState,
		&(temptarget->player), mapObjectList);

	temptarget->player = playerState;
	}
    else
	{
	/**********************************************/
	/* deal with the computer controlled monsters */
	/**********************************************/

	{
	const struct EntityArchetype *movementArchetype;

	movementArchetype = playerArchetype(&playerState);
	mRad = movementArchetype == NULL ? playerState.width + 0.5f :
	       entityArchetypeCollisionRadius(movementArchetype) + 0.5f;
	mAng = 0.8f;
	}

	if (((playerState.movingForward || playerState.movingBackward)) && (!playerState.dead))
	    {
	    playerState.moving = 1;

	    playerState.energyRemaining -= playerState.moveCost; /* takes energy to move, not to turn */

	    if (playerState.movingForward)
		mAngle = (playerState.headHorzRotate * BIG_DEG_TO_RAD);
	    if (playerState.movingBackward)
		mAngle = (playerState.headHorzRotate + 1800) * BIG_DEG_TO_RAD;



	    blocked = 0;

	    if (playerHasSolidCollision(&playerState))
		{
		/****************************************************/
		/* cant move through trees until they are destroyed */
		/****************************************************/

		mapObjectNode = mapObjectList->next;
		while ((mapObjectNode != NULL) && (!blocked))
		    {
		    object = *mapObjectNode;

		    if (object.intact)
			trad = sqrt((object.x - tx) * (object.x - tx) + (object.z - tz) * (object.z - tz));
		    else
			trad = mRad;

		    if (object.intact && mapObjectCollisionRadius(&object) > 0.0f &&
		        (trad < mRad + mapObjectCollisionRadius(&object)) &&
		        worldVerticalIntervalsOverlap(
			playerState.bottom, playerState.height,
			object.y, mapObjectCollisionTop(&object)))
			{
			obsangle = computeAngle(object.z,  tz, object.x,  tx);

			if (obsangle < 0)
			    obsangle += TWOPI;

			if ((fabs(mAngle + PI - obsangle) < mAng) || (fabs(mAngle - PI - obsangle) < mAng))
				{
			blocked = 1;
				}
			}

		    mapObjectNode = mapObjectNode->next;
		    }

		/****************************************************/
		/* cant move through actors until they are destroyed */
		/****************************************************/

		tempactor = actorlist->next;
		while ((tempactor != NULL) && !blocked)
		    {

		    trad = sqrt((tempactor->x - tx) * (tempactor->x - tx) + (tempactor->z - tz) * (tempactor->z - tz));

		    if (trad < playerState.width + actorCollisionRadius(tempactor) &&
		        worldVerticalIntervalsOverlap(
			playerState.bottom, playerState.height,
			actorCollisionBottom(tempactor),
			actorCollisionTop(tempactor)))
			{
			obsangle = computeAngle(tempactor->z, tz, tempactor->x, tx);

			if (obsangle < 0)
			    obsangle += TWOPI;

			if ((fabs(mAngle + PI - obsangle) < .75) || (fabs(mAngle - PI - obsangle) < .75))
			    {
			    blocked = 1;
			    }
			}

		    tempactor = tempactor->next;
		    }
		}

	    /***********************************************************************************/
	    /* cant move through other targets until they are destroyed or move out of the way */
	    /***********************************************************************************/

	    if (playerHasSolidCollision(&temptarget->player))
		{
		temptarget2 = targets->next;
		while ((temptarget2 != NULL) && (!blocked))
		    {
		    if ((temptarget2 != temptarget) &&
		        playerHasSolidCollision(&temptarget2->player) &&
		        (temptarget2->player.dead != 1))
			{
			rad = sqrt((temptarget2->x - tx) * (temptarget2->x - tx) + (temptarget2->z - tz) * (temptarget2->z - tz));

			if (rad < playerState.width +
			          temptarget2->player.width &&
			    worldVerticalIntervalsOverlap(
				playerState.bottom, playerState.height,
				temptarget2->player.bottom,
				temptarget2->player.height))
			    {
			    obsangle = computeAngle(temptarget2->z, tz, temptarget2->x, tx);

			    if (obsangle < 0)
				obsangle += TWOPI;

			    if ((fabs(mAngle + PI - obsangle) < mAng) || (fabs(mAngle - PI - obsangle) < mAng))
				blocked = 1;
			    }
			}
		    temptarget2 = temptarget2->next;
		    }

		/**************************************/
		/* cant move through player's player */
		/**************************************/

		rad = sqrt((tx - playerEntityState.x) * (tx - playerEntityState.x) +
		           (tz - playerEntityState.z) * (tz - playerEntityState.z));

		if ((rad < playerState.width + playerEntityState.width) &&
		    playerHasSolidCollision(&playerEntityState) &&
		    (playerEntityState.dead != 1) &&
		    worldVerticalIntervalsOverlap(
			playerState.bottom, playerState.height,
			playerEntityState.bottom, playerEntityState.height))
		    {
		    obsangle = computeAngle(playerEntityState.z, tz,
		                            playerEntityState.x, tx);

		    if (obsangle < 0)
			obsangle += TWOPI;

		    if ((fabs(mAngle + PI - obsangle) < mAng) || (fabs(mAngle - PI - obsangle) < mAng))
			blocked = 1;
		    }
		}

	    if (!blocked)
		{
		temptarget->z += cos(mAngle) * playerState.speed;
		temptarget->x += sin(mAngle) * playerState.speed;
		}
	}

	/**********************************/
	/* update target Weapons          */
	/**********************************/

	if (playerState.beamOn)
	    updatePlayerWeapon(
		temptarget->x, temptarget->z, &playerState,
		&(temptarget->player), mapObjectList);

	/*****************/
	/* update energy */
	/*****************/

	if (playerState.energyRemaining > 0)
	    {
	    playerState.energyRemaining += playerState.regenRate;

	    if (playerState.energyRemaining > MAXLIFE)
		playerState.energyRemaining = MAXLIFE;

	    playerState.rot1 = cos(playerState.moveCount) * 0.02;
	    playerState.rot2 = sin(playerState.moveCount) * 0.05;
	    }
	else
	    {
	    updateDeadPlayer(temptarget->x, temptarget->z, &playerState);
	    }

	if (playerState.moving)
	    playerState.moveCount += 1;


	temptarget->player = playerState;
	}
    updateMovementAudio(playerArchetype(&temptarget->player),
                        temptarget->x, temptarget->player.y,
                        temptarget->z,
                        temptarget->player.moving, mainCounter);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* update everything                                             */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void doUpdate()
    {
    struct runtimeMapObject * mapObjectNode;
    struct runtimeActor * tempactor;
    struct targetInfo * temptarget;
    struct targetInfo * temptarg;
    float mAngle;
    float xv,  zv;
    float nextX, nextZ;
    int blocked;

    worldEnvironmentAdvance(&worldEnvironment, 0.0025f);

    if (playerWeaponReloadRemaining > 0)
	playerWeaponReloadRemaining -= 1;

    if (mode == DEMOMODE && playerEntityState.moveCount >= 150)
	showDemoMenu = 1;

    audioSetListener(playerEntityState.x,
	(playerEntityState.bottom + playerEntityState.height) * 0.5f,
	playerEntityState.z, entityListenerHeadingRadians(
	    playerEntityArchetype, playerEntityState.headHorzRotate,
	    playerBodyHeading));

    /*************************************************/
    /* reclaim space from finished sounds            */
    /*************************************************/

   if (!noSound)
	{
	flushSounds();

	if (getMusicOn())
	    doSound(MUSIC);
	else
	    soundKiller(MUSIC);
	}

    /*********************/
    /* add a new vehicle */
    /*********************/

    if (!client)
	{
	if (((mainCounter % arrivalTime) == 0) && !playerEntityState.dead)
	    {
		    addRandomCatalogEntity();

	    if ((mode == DEMOMODE) && !(rand() % 4) && (playerEntityState.moveCount >= 150))
		view = (view + 1) % 3;
	    }
	}

    /**********************************/
    /* player on the move            */
    /**********************************/

    if (playerEntityState.movingForward || playerEntityState.movingBackward)
	{
	playerEntityState.moving = 1;

	playerEntityState.energyRemaining -= playerEntityState.moveCost; /* takes energy to move, not to turn */

	if (playerMoveActive)
	    mAngle = playerMoveAngle;
	else if (playerEntityState.movingForward)
	    mAngle = (playerEntityState.headHorzRotate * BIG_DEG_TO_RAD);
	else if (playerEntityState.movingBackward)
	    mAngle = (playerEntityState.headHorzRotate + 1800) * BIG_DEG_TO_RAD;

	zv = cos(mAngle) * playerEntityState.speed;
	xv = sin(mAngle) * playerEntityState.speed;
	nextX = playerEntityState.x + xv;
	nextZ = playerEntityState.z + zv;
	blocked = 0;

	if (playerHasSolidCollision(&playerEntityState))
	    {
	    mapObjectNode = mapObjectList->next;
	    while ((mapObjectNode != NULL) && (!blocked))
		{
		if (mapObjectNode->intact &&
		    mapObjectCollisionRadius(mapObjectNode) > 0.0f &&
		    worldCylindersOverlap(
			nextX, nextZ, playerEntityState.width,
			playerEntityState.bottom, playerEntityState.height,
			mapObjectNode->x, mapObjectNode->z,
			mapObjectCollisionRadius(mapObjectNode),
			mapObjectNode->y, mapObjectCollisionTop(mapObjectNode)))
		    blocked = 1;
		mapObjectNode = mapObjectNode->next;
		}

	    tempactor = actorlist->next;
	    while ((tempactor != NULL) && !blocked)
		{
		if (tempactor->damage > 0 &&
		    worldCylindersOverlap(
			nextX, nextZ, playerEntityState.width,
			playerEntityState.bottom, playerEntityState.height,
			tempactor->x, tempactor->z,
			actorCollisionRadius(tempactor),
			actorCollisionBottom(tempactor),
			actorCollisionTop(tempactor)))
		    blocked = 1;
		tempactor = tempactor->next;
		}

	    temptarget = targets->next;
	    while ((temptarget != NULL) && (!blocked))
		{
		if (playerHasSolidCollision(&temptarget->player) &&
		    temptarget->player.dead != 1 &&
		    worldCylindersOverlap(
			nextX, nextZ, playerEntityState.width,
			playerEntityState.bottom, playerEntityState.height,
			temptarget->x, temptarget->z,
			temptarget->player.width,
			temptarget->player.bottom,
			temptarget->player.height))
		    blocked = 1;
		temptarget = temptarget->next;
		}
	    }

	if (!blocked)
	    {
	    playerEntityState.x = nextX;
	    playerEntityState.z = nextZ;
	    }
	}

    updateMovementAudio(playerEntityArchetype, playerEntityState.x,
                        playerEntityState.y, playerEntityState.z,
                        playerEntityState.moving, mainCounter);

    /**********************/
    /* update structures  */
    /**********************/

    if (mapObjectList->next != NULL)
	updateMapObjects(mapObjectList, itsChristmas);

    /****************************/
    /* update all the targets   */
    /****************************/

    if (targets->next != NULL)
	updateTargets();


    /************************/
    /* update the vehicles  */
    /************************/

     if (actorlist->next != NULL)
	updateActors(actorlist, playerEntityState.width, playerEntityState.height, playerEntityState.bottom,
	    playerEntityState.dead, wrecklist, accuracy,
	    firingDelay, mainCounter, &playerEntityState, targets);


    /****************************/
    /* update burning wreck fragments */
    /****************************/

    if (wrecklist->next != NULL)
	updateWrecks(wrecklist);


    /**************************/
    /* update the projectiles */
    /**************************/

    if (runtimeProjectileList->next != NULL)
	updateRuntimeProjectiles();


    /**********************************/
    /* update player Weapons         */
    /**********************************/

    if (playerEntityState.beamOn)
	{
	turnBeamOnSinceLast();
	updateConfiguredPlayerWeapon();
	}

    /*************************/
    /* update the explosions */
    /*************************/

    if (runtimeExplosionList->next != NULL)
	updateRuntimeExplosions(runtimeExplosionList, actorlist);


    /***********************/
    /* update the player  */
    /***********************/

    if (!playerEntityState.beamOn && playerEntityArchetype != NULL)
	soundKillerKey(playerEntityArchetype->fireSound);


    /****************************/
    /* update player's energy  */
    /****************************/

    if (playerEntityState.energyRemaining > 0)
	{
	playerEntityState.energyRemaining += playerEntityState.regenRate;

	if (playerEntityState.energyRemaining > MAXLIFE)
	    playerEntityState.energyRemaining = MAXLIFE;

	playerEntityState.rot1 = cos(playerEntityState.moveCount) * 0.02;
	playerEntityState.rot2 = sin(playerEntityState.moveCount) * 0.05;
	}
    else
	{
	updateDeadPlayer(playerEntityState.x, playerEntityState.z, &playerEntityState);
	}

    if (playerEntityState.moving)
	playerEntityState.moveCount += 1;

    /*************************************/
    /* update the high-score list        */
    /*************************************/

   if (playerEntityState.dead && (playerEntityState.deadCount == 1))
	updateScores(scorefullPath, playerEntityState.score, mode, playerName);

    /*********************************/
    /* deal with dead targets        */
    /*********************************/

    temptarget = targets;
    while(temptarget->next != NULL)
	{
	    {
	    if (temptarget->next->player.timeDead > 250)
		{
		for(tboom=runtimeExplosionList->next; tboom != NULL; tboom=tboom->next)
		    {
		    if (tboom->owner == &(temptarget->next->player))
			tboom->owner = NULL;
		    }

		for(tproj=runtimeProjectileList->next; tproj != NULL; tproj=tproj->next)
		    {
		    if (tproj->owner == &(temptarget->next->player))
			tproj->owner = NULL;
		    }

		temptarg = temptarget->next;
		temptarget->next = temptarget->next->next;

		afree(temptarg, arena);
		}
	    else
		temptarget = temptarget->next;
	    }
	}

   if ((mode == DEMOMODE) && (playerEntityState.timeDead > 400))
	{
	stepPlayerEntitySelection(1);
	setPlayConditions();
	}

    /*************************************/
    /* vehicles arrive faster later on   */
    /*************************************/

    arrivalTime = (int) (ARRIVALRATE - (mainCounter * 0.0005));
    if (arrivalTime < 20)
	arrivalTime = 20;

    /*************************************/
    /* vehicles more accurate later on   */
    /*************************************/

    accuracy = (STARTACCURACY + (mainCounter * 0.002));
    if (accuracy > 500)
	accuracy = 500;

    /*************************************/
    /* vehicles fire faster later on     */
    /*************************************/

    firingDelay = (int) (STARTFIRINGDELAY - (mainCounter * 0.0005));
    if (firingDelay < 43)
	firingDelay = 43;

    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* view the scene from the player's point of view               */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static float currentViewportAspect(void)
    {
    GLint viewport[4];

    glGetIntegerv(GL_VIEWPORT, viewport);
    return cameraViewportAspect(viewport[2], viewport[3]);
    }

void goToPlayerView(int eyeball)
    {
    float cameraDistance;
    float cameraX;
    float cameraY;
    float lookZ;
    float nearClip;
    float lookY;
    float playerBottom;
    float playerHeight;
    struct ChaseCameraFrame chaseFrame;

    playerBottom = playerEntityState.bottom;
    playerHeight = playerEntityState.height - playerEntityState.bottom;
    cameraDistance = 1.4f;
    nearClip = 0.1f;
    cameraY = playerBottom + playerHeight * 0.75f + 0.20f;
    lookY = cameraY - 0.05f;
    lookZ = 0.4f;
    if (playerEntityArchetype != NULL)
	{
	playerBottom = PLANEY + playerEntityArchetype->verticalOffset;
	playerHeight = playerEntityArchetype->collisionHeight;
	if (chaseCameraFrame(playerBottom, playerHeight,
	                     playerEntityArchetype->view1HeightFraction,
	                     &chaseFrame))
	    {
	    cameraDistance = chaseFrame.distance;
	    nearClip = chaseFrame.nearClip;
	    cameraY = chaseFrame.eyeY;
	    lookY = chaseFrame.targetY;
	    lookZ = chaseFrame.targetZ;
	    }
	}

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(90, currentViewportAspect(), nearClip, 15);
    glMatrixMode(GL_MODELVIEW);

    cameraX = 0.0f;
    if (eyeball == 0)
	cameraX = 0.015f;
    else if (eyeball == 1)
	cameraX = -0.015f;

    switch(eyeball){
	case 2: gluLookAt(0, cameraY, -cameraDistance,
			  0, lookY, lookZ, 0, 1, 0);
		break;

	case 0: gluLookAt(cameraX, cameraY, -cameraDistance,
			  cameraX, lookY, lookZ, 0, 1, 0);
		break;

	case 1: gluLookAt(cameraX, cameraY, -cameraDistance,
			  cameraX, lookY, lookZ, 0, 1, 0);
		break;

	}

    if (lod >= 2)
	{
	glFogf(GL_FOG_START, (GLfloat)MONSTERFOGSTART);
	glFogf(GL_FOG_END, (GLfloat)MONSTERFOGEND);

	glEnable(GL_FOG);
	}

    yrot = - (GLfloat) playerEntityState.headHorzRotate;
    xrot = 0;
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* true first-person view, separate from the original close view */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void goToFirstPersonView(int eyeball)
    {
    float eyeX;
    float eyeY;
    float lookY;
    float lookZ;
    float pitch;

    if (playerEntityArchetype == NULL)
	{
	goToPlayerView(eyeball);
	return;
	}
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(90, currentViewportAspect(), 0.1, 15);
    glMatrixMode(GL_MODELVIEW);

    eyeX = 0.0f;
    if (eyeball == 0)
	eyeX = 0.015f;
    else if (eyeball == 1)
	eyeX = -0.015f;
    eyeY = PLANEY + playerEntityArchetype->firstPersonEyeHeight;
    pitch = playerEntityState.headVertRotate * BIG_DEG_TO_RAD;
    lookY = eyeY - (float)sin((double)pitch);
    lookZ = 0.1f + (float)cos((double)pitch);
    gluLookAt(eyeX, eyeY, 0.1, eyeX, lookY, lookZ, 0, 1, 0);

    if (lod >= 2)
	{
	glFogf(GL_FOG_START, (GLfloat)MONSTERFOGSTART);
	glFogf(GL_FOG_END, (GLfloat)MONSTERFOGEND);
	glEnable(GL_FOG);
	}
    yrot = - (GLfloat) playerEntityState.headHorzRotate;
    xrot = 0;
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* view teh scene from the most recently added military vehicle  */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void goToArmyView(int eyeball)
    {
    /*look throgh 0,0 to opposite side and assign the opposite to the
    OTHER one making 2 parallel lines! */

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, currentViewportAspect(), 0.1, 20);
    glMatrixMode(GL_MODELVIEW);

    switch(eyeball){
	case 2: gluLookAt(lastActorRad * sin(lastActorTheta), lastActorY,
		lastActorRad*cos(lastActorTheta), 0, lastActorY, 0, 0,1,0);
		break;

	case 0: gluLookAt(lastActorRad * sin(lastActorTheta + 0.002),
		lastActorY, lastActorRad * cos(lastActorTheta + 0.002),
		-lastActorRad * sin(lastActorTheta-0.002), lastActorY,
		-lastActorRad * cos(lastActorTheta-0.002), 0,1,0);
		break;

	case 1: gluLookAt(lastActorRad * sin(lastActorTheta - 0.002),
		lastActorY, lastActorRad * cos(lastActorTheta - 0.002),
		-lastActorRad*sin(lastActorTheta+0.002), lastActorY,
		-lastActorRad * cos(lastActorTheta+0.002), 0,1,0);
		break;
	}

    if (lod >= 2)
	{
	glFogi(GL_FOG_START,  ARMYFOGSTART);
	glFogi(GL_FOG_END,  ARMYFOGEND);
	glEnable(GL_FOG);
	}

    xrot = yrot = 0;
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* view the scene from above                                     */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void goToMapView()
    {
	float aspect;

				 /* used to get overhead map of the town */
				/* also set plane size to 40 */
			       /* and eliminate prepositioned vehicles */
			      /* and hide motion dots */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    aspect = currentViewportAspect();
    glOrtho((GLdouble)mapHeight * aspect,
	   -(GLdouble)mapHeight * aspect, mapHeight,
	   -mapHeight, 0.1, 50);
    glMatrixMode(GL_MODELVIEW);

    gluLookAt(0, mapHeight, 0, 0, PLANEY, 0,  0, 0, 1);
    xrot = yrot = 0;
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* view the scene from omniscient 3rd person                     */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void goToOverView(int eyeball)
    {
    float cameraDistance;
    float centerY;
    float nearClip;
    struct OverviewCameraFrame overviewFrame;

    view = OMNISCIENTVIEW;

    cameraDistance = 9.0f;
    nearClip = 0.1f;
    centerY = 0.0f;
    if (playerEntityArchetype != NULL &&
        overviewCameraFrame(PLANEY + playerEntityArchetype->verticalOffset,
                            playerEntityArchetype->collisionHeight,
                            playerEntityArchetype->view2HeightFraction,
                            xrot * 0.1f,
                            &overviewFrame))
	{
	cameraDistance = overviewFrame.distance;
	nearClip = overviewFrame.nearClip;
	centerY = overviewFrame.centerY;
	}
    overviewRotationPivotY = centerY;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, currentViewportAspect(), nearClip, 25);
    glMatrixMode(GL_MODELVIEW);

    switch(eyeball){
	case 2: if (playerEntityArchetype != NULL)
		    gluLookAt(0, centerY, cameraDistance,
		              0, centerY, 0, 0, 1, 0);
		else
		    glCallList(overviewLookat);
		break;

	case 0: gluLookAt( 0.05, centerY, cameraDistance,
			   0.05, centerY, 0, 0,1,0);
		break;

	case 1: gluLookAt(-0.05, centerY, cameraDistance,
			  -0.05, centerY, 0, 0,1,0);
		break;
	}

    if (lod >= 2)
	{
	glFogi(GL_FOG_START,  OVERVIEWFOGSTART);
	glFogi(GL_FOG_END,  OVERVIEWFOGEND);
	glEnable(GL_FOG);
	}
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw a given player at a given location                      */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawPlayerEntity(float x, struct runtimePlayer player, float z,
			int isTarget, int view)
    {
    const struct EntityArchetype *archetype;
    float aimPitch;
    float aimYaw;
    float bodyHeading;

    if (fabs(x - playerEntityState.x) > PLANESIZE ||
        fabs(z - playerEntityState.z) > PLANESIZE)
        return;
    archetype = playerArchetype(&player);
    if (archetype == NULL ||
        (!isTarget && view == FIRSTPERSONVIEW &&
         !archetype->firstPersonVisible))
        return;
    if (!player.dead && archetype->castsShadow)
        drawEnvironmentShadow(
            x, z, PLANEY + archetype->verticalOffset,
            entityArchetypeCollisionRadius(archetype),
            archetype->collisionHeight,
            &worldEnvironment);
    glPushMatrix();
    glTranslatef(x, PLANEY + archetype->verticalOffset, z);
    if (player.energyRemaining <= 0)
        {
        glTranslatef(0.0f, -player.deadCount * 0.015f, 0.0f);
        glRotatef(-player.deadCount * 2.0f, 1.0f, 0.0f, 0.0f);
        }
    bodyHeading = player.headHorzRotate * 0.1f;
    aimYaw = 0.0f;
    aimPitch = player.headVertRotate * 0.1f;
    if (!isTarget && archetype->controlMode == ENTITY_CONTROL_STEERED)
        {
        bodyHeading = playerBodyHeading * 0.1f;
        aimYaw = (player.headHorzRotate - playerBodyHeading) * 0.1f;
        }
    glRotatef(bodyHeading, 0.0f, 1.0f, 0.0f);
    drawConfiguredPlayerEntity(
        archetype, player.moving, player.movingBackward,
        player.beamOn, mainCounter, aimYaw, aimPitch);
    glPopMatrix();
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw everything                                               */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void doDrawing (int eyeball)
    {
    const struct EntityArchetype *beamArchetype;
    float beamOrigin[3];
    float beamDirection[3];
    int hasBeamMuzzle;
    GLenum glErrorVal;
    GLint tmp[4];
    struct targetInfo * temptarget;

    /***********************/
    /* check for GL errors */
    /***********************/

    glErrorVal = glGetError();
    if (glErrorVal != GL_NO_ERROR)
	showError((char *) gluErrorString(glErrorVal));

/*    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPopAttrib(); why was this sequence here????
*/
    /* if line widths are integral Mesa chokes in a large window */
    glLineWidth(2.01);

    if (lod >= 2)
	glShadeModel(GL_SMOOTH);
    else
	glShadeModel(GL_FLAT);


    /****************************************************************/
    /* MUST enable dithering or the 8-bit machines look REALLY ugly */
    /****************************************************************/

    glEnable(GL_DITHER);

    glPushMatrix();

    /**********************************/
    /* setting viewing location       */
    /**********************************/

    if (view == PLAYERVIEW)
	goToPlayerView(eyeball);
    else if (view == FIRSTPERSONVIEW)
	goToFirstPersonView(eyeball);
    else if ((view == ARMYVIEW) && (lastActorY > NOACTORONPLANE))
	goToArmyView(eyeball);
    else if (view == MAPVIEW)
	goToMapView();
    else /*view == OMNISCIENTVIEW */
	goToOverView(eyeball);

    drawEnvironmentSky(&worldEnvironment);

    /***********************************/
    /* rotate and draw the battlefield */
    /***********************************/

    if (xrot != 0)
	{
	if (view == OMNISCIENTVIEW && playerEntityArchetype != NULL)
	    glTranslatef(0, overviewRotationPivotY, 0);
	glRotatef(0.1 * xrot, 1,0,0);
	if (view == OMNISCIENTVIEW && playerEntityArchetype != NULL)
	    glTranslatef(0, -overviewRotationPivotY, 0);
	}

    if (yrot != 0)
    	glRotatef(0.1 * yrot, 0,1,0);

    glDisable(GL_DEPTH_TEST);

    drawWorldGround(-playerEntityState.x, -playerEntityState.z, lod,
		    itsChristmas, view);

    if (lod != -1)
	glEnable(GL_DEPTH_TEST);

    /* Gameplay state remains in map-world coordinates.  Only the rendered
       world is rebased around the camera/player. */
    glPushMatrix();
    glTranslatef(-playerEntityState.x, 0, -playerEntityState.z);

    /**********************************/
    /* draw burning wreck fragments   */
    /**********************************/

    if (wrecklist->next != NULL)
	drawRuntimeDebris(wrecklist, playerEntityState.x, playerEntityState.z,
	                  lod, itsChristmas);


    /******************************************************/
    /* draw the vehicles                                  */
    /* must come before draw structures for hero entrance */
    /******************************************************/

    if (actorlist->next != NULL)
	drawRuntimeActors(actorlist, mainCounter, playerEntityState, lod, view);



    /**********************************/
    /* draw all projectiles in flight */
    /**********************************/

    if (runtimeProjectileList->next != NULL)
	drawRuntimeProjectiles(runtimeProjectileList, mainCounter,
	                       itsChristmas, lod, &playerEntityState, view);



    /********************/
    /* draw structures  */
    /********************/

    if (mapObjectList->next != NULL)
	drawMapObjects(mapObjectList, mainCounter, lod, itsChristmas, view,
	               playerEntityState.x, playerEntityState.z);



    /**********************************/
    /* draw monsterBeam               */
    /**********************************/

    beamArchetype = playerArchetype(&playerEntityState);
    if (playerEntityState.beamOn && beamArchetype != NULL &&
	beamArchetype->weapon == ENTITY_WEAPON_BEAM)
	{
	hasBeamMuzzle = configuredPlayerMuzzleWorld(
	    beamOrigin, beamDirection);
	if (hasBeamMuzzle)
	    drawRuntimeBeam(beamArchetype, lod, beamOrigin, beamDirection);
	}

    for (temptarget = targets->next;temptarget != NULL;temptarget = temptarget->next)
	{
	beamArchetype = playerArchetype(&temptarget->player);
	if (temptarget->player.beamOn && beamArchetype != NULL &&
	    beamArchetype->weapon == ENTITY_WEAPON_BEAM)
	    {
	    hasBeamMuzzle = configuredMuzzleWorld(
		beamArchetype, temptarget->player.moving,
		temptarget->player.beamOn, mainCounter,
		temptarget->x, PLANEY + beamArchetype->verticalOffset,
		temptarget->z,
		temptarget->player.headHorzRotate * 0.1f, 0.0f,
		temptarget->player.headVertRotate * 0.1f,
		beamOrigin, beamDirection);
	    if (hasBeamMuzzle)
		drawRuntimeBeam(beamArchetype, lod, beamOrigin, beamDirection);
	    }
	}



    /**********************************/
    /* draw the targets               */
    /**********************************/

    for(temptarget = targets->next;temptarget != NULL;temptarget = temptarget->next)
	drawPlayerEntity(temptarget->x, temptarget->player, temptarget->z, 1,view);


    /**********************************/
    /* draw the player's player      */
    /**********************************/

    drawPlayerEntity(playerEntityState.x, playerEntityState, playerEntityState.z, 0,view);

    /**********************************/
    /* draw explosions                */
    /*                                */
    /* explosions must be last so     */
    /* other things can be seen       */
    /* throught them                  */
    /**********************************/

    if (runtimeExplosionList->next != NULL)
	{
	if (lod == -1)
	    {
	    glPushAttrib(GL_POLYGON_BIT);
	    glPolygonMode(GL_FRONT_AND_BACK,GL_POINT);
	    drawRuntimeExplosions(runtimeExplosionList, lod);
	    glPopAttrib();
	    }
	else
	    drawRuntimeExplosions(runtimeExplosionList, lod);
	}

    glPopMatrix();

    glDisable(GL_FOG);

    glPopMatrix();

    if (!backdrop)
	{
	glGetIntegerv(GL_VIEWPORT, tmp);
	viewL = tmp[0];
	viewR = viewL+tmp[2] - 1;

	glPushMatrix();

	/**********************************/
	/* print the overlays             */
	/**********************************/

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, currentViewportAspect(), 0.1, 20);
	glMatrixMode(GL_MODELVIEW);

	switch(eyeball){
	    case 2: glCallList(overviewLookat);
		    break;
	    case 0: gluLookAt( 0.065, 0, 9,  0.065, 0, 0, 0,1,0);
		    break;
	    case 1: gluLookAt(-0.065, 0, 9, -0.065, 0, 0, 0,1,0);
		    break;
	    }

	textLineWidth = (viewR-viewL) / 300.0;
	if (textLineWidth < 1)
	    textLineWidth = 1.01;

if (lod == -1)
textLineWidth = 1.01;

	/* if line widths are integral Mesa chokes in a large window */
	glLineWidth(textLineWidth);

	/* showText(playerEntityState.energyRemaining,  playerEntityState.score,
	(long) (viewR-viewL), paused);   */

	showText(targets, playerEntityState.energyRemaining,  playerEntityState.score,
	    showframes, paused, pointerGrab, playerWeaponUsesReload(),
	    playerWeaponReadiness(), playerEntityState.x, playerEntityState.z);

	if (mode == DEMOMODE)
	    {
	    if (!(playerEntityState.timeDead > 100) && !showOptions)
		{
		if (!showDemoMenu)
			showScores(itsChristmas, highScores, playerEntityState,
			           mainCounter, offsetX, lod);
		else if (!showHelp)
		    showText2((long) (viewR-viewL), getSoundOn(), noSound, getMusicOn(),
			mode3D, no3D, lod, paused, itsChristmas, pointerGrab);
		}


	    showText3(lod);

	    if (showOptions)
		doOptions(playerEntityState, (long) (viewR-viewL),  mainCounter, itsChristmas, offsetX, lod);
	    else
		showText4();

	    if ((playerEntityState.timeDead > 100) && !showOptions)
		doSummary(playerEntityState.timeDead, (long)(viewR - viewL),
		          playerEntityState.score, mainCounter, lod,
		          moreThanOne);
	    }

	if (showHelp)
	    showText2((long) (viewR-viewL), getSoundOn(), noSound, getMusicOn(),
		mode3D, no3D, lod, paused, itsChristmas, pointerGrab);

	glPopMatrix();
	}
    }


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* computer controlled player for demo-mode                     */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

struct runtimePlayer autopilot(float centerX, float centerZ,
                                 struct runtimePlayer playerState)
    {
    const struct EntityArchetype *archetype;
    struct runtimeActor *actor;
    struct runtimeMapObject *structure;
    float targetX;
    float targetY;
    float targetZ;
    float nearest;
    float deltaX;
    float deltaZ;
    float distance;
    float desiredHeading;
    float headingDelta;
    float preferredRange;
    float minimumRange;
    float verticalDelta;
    int turnStep;
    int pitchStep;
    int hasTarget;

    archetype = playerArchetype(&playerState);
    if (archetype == NULL)
        return playerState;
    targetX = centerX;
    targetY = PLANEY;
    targetZ = centerZ;
    nearest = BIGNUM;
    hasTarget = 0;

    for (structure = mapObjectList->next; structure != NULL;
         structure = structure->next)
        if (structure->intact && structure->dataArchetype != NULL &&
            structure->dataArchetype->destructible)
            {
            deltaX = structure->x - centerX;
            deltaZ = structure->z - centerZ;
            distance = (float)sqrt((double)(deltaX * deltaX +
                                             deltaZ * deltaZ));
            if (distance < nearest)
                {
                nearest = distance;
                targetX = structure->x;
                targetY = structure->y +
                    structure->dataArchetype->collisionHeight * 0.5f;
                targetZ = structure->z;
                hasTarget = 1;
                }
            }

    for (actor = actorlist->next; actor != NULL; actor = actor->next)
        if (actor->damage > 0 && actor->dataArchetype != NULL &&
            actor->dataArchetype->kind != ENTITY_KIND_ZONE &&
            factionCatalogHostile(&gameFactions, runtimePlayerFaction(),
                                  actor->dataAgent.faction))
            {
            deltaX = actor->x - centerX;
            deltaZ = actor->z - centerZ;
            distance = (float)sqrt((double)(deltaX * deltaX +
                                             deltaZ * deltaZ));
            if (distance < nearest)
                {
                nearest = distance;
                targetX = actor->x;
                targetY = actor->y +
                    actor->dataArchetype->collisionHeight * 0.5f;
                targetZ = actor->z;
                hasTarget = 1;
                }
            }

    playerState.movingForward = 0;
    playerState.movingBackward = 0;
    playerState.moving = 0;
    playerState.beamOn = 0;
    if (!hasTarget)
        return playerState;

    deltaX = targetX - centerX;
    deltaZ = targetZ - centerZ;
    desiredHeading = (float)(atan2((double)deltaX, (double)deltaZ) *
                             RAD_TO_BIG_DEG);
    headingDelta = desiredHeading - playerState.headHorzRotate;
    while (headingDelta > 1800.0f)
        headingDelta -= 3600.0f;
    while (headingDelta < -1800.0f)
        headingDelta += 3600.0f;
    turnStep = (int)(headingDelta / 15.0f);
    if (turnStep > (int)(playerState.xspeed * 0.75f))
        turnStep = (int)(playerState.xspeed * 0.75f);
    if (turnStep < (int)(-playerState.xspeed * 0.75f))
        turnStep = (int)(-playerState.xspeed * 0.75f);
    playerState.headHorzRotate += turnStep;
    offsetX = 0.8f * offsetX - 0.01f * turnStep;

    minimumRange = archetype->weaponMinRange;
    if (minimumRange < entityArchetypeCollisionRadius(archetype))
        minimumRange = entityArchetypeCollisionRadius(archetype);
    preferredRange = archetype->tacticRadius;
    if (archetype->tactic != ENTITY_TACTIC_STANDOFF)
        preferredRange = archetype->weaponRange * 0.7f;
    if (preferredRange < minimumRange)
        preferredRange = minimumRange;

    if (playerState.energyRemaining > archetype->playerMoveCost)
        {
        if (archetype->tactic == ENTITY_TACTIC_FLYBY)
            playerState.movingForward = 1;
        else if (nearest > preferredRange)
            playerState.movingForward = 1;
        else if (nearest < minimumRange &&
                 archetype->collisionMode != ENTITY_COLLISION_PHASE)
            playerState.movingBackward = 1;
        }
    playerState.moving =
        playerState.movingForward || playerState.movingBackward;

    if (archetype->weapon != ENTITY_WEAPON_NONE &&
        playerState.energyRemaining > archetype->playerFireCost &&
        nearest <= archetype->weaponRange &&
        nearest >= archetype->weaponMinRange &&
        abs(turnStep) < 6)
        playerState.beamOn = 1;

    verticalDelta = targetY -
        (PLANEY + archetype->verticalOffset +
         archetype->collisionHeight * 0.5f);
    if (nearest < NEARZERO)
        nearest = NEARZERO;
    desiredHeading = (float)(-atan2((double)verticalDelta,
                                    (double)nearest) * RAD_TO_BIG_DEG);
    pitchStep = (int)((desiredHeading - playerState.headVertRotate) / 15.0f);
    playerState.headVertRotate += pitchStep;
    return playerState;
    }

/**********************************/

void reshape( int width, int height )
    {
    viewW = width;
    viewH = height;
    glViewport(0, 0, width, height);
    }

/**********************************/

static void doDisplay(void)
    {
    int eye;
    struct CameraViewport stereoViewport;

    glClearColor(worldEnvironment.skyHorizon[0],
                 worldEnvironment.skyHorizon[1],
                 worldEnvironment.skyHorizon[2], 1.0f);

    /**********************************/
    /* draw everything                */
    /**********************************/

    if (!mode3D)
	{
	/**********************************/
	/* draw in mono                   */
	/**********************************/

	if (lod == -1)
	    glClear(GL_COLOR_BUFFER_BIT);
	else
	    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

 	doDrawing(2);
 	}
    else
	{
	/**********************************/
	/* draw in stereo                 */
	/**********************************/

	glViewport(0, 0, viewW, viewH);
	glScissor(0, 0, viewW, viewH);
	if (lod == -1)
	    glClear(GL_COLOR_BUFFER_BIT);
	else
	    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	for (eye = CAMERA_STEREO_RIGHT_EYE;
	     eye <= CAMERA_STEREO_LEFT_EYE; eye++)
	    if (cameraStereoViewport(mode3D, eye, viewW, viewH,
	                             &stereoViewport))
		{
		glViewport(stereoViewport.x, stereoViewport.y,
		           stereoViewport.width, stereoViewport.height);
		glScissor(stereoViewport.x, stereoViewport.y,
		          stereoViewport.width, stereoViewport.height);
		doDrawing(eye);
		}
	glViewport(0, 0, viewW, viewH);
	glScissor(0, 0, viewW, viewH);
	}
    tkSwapBuffers();
    }


/**********************************/

void id()
    {
    struct runtimeActor * tactor;
    struct targetInfo * temptarget;

    if (targets->next != NULL)
	{
	for(temptarget = targets->next; temptarget != NULL; temptarget = temptarget->next)
	    {
	    temptarget->player.moving	= 0;
	    temptarget->player.movingForward	= 0;
	    temptarget->player.movingBackward	= 0;

	    temptarget->player.beamOn		= 0;
	    }
	}

    if (mode == DEMOMODE)
	{
	playerEntityState.beamOn = 0;
	}

    playerEntityState.moving = 0;

    checkMouse();

    /**************************************/
    /* update the frames / second counter */
    /**************************************/

    now = time(NULL);
    if (now != beg)
	{
	showframes  = frames;
	frames	    = 0;
	beg	    = now;
	}
    frames += 1;


    totalCounter += 1;

    if (!paused)
	mainCounter += 1;


    if (playerEntityState.dead && (mode == PLAYMODE))
	{
	mode = DEMOMODE;
	clearPlayerInput();
	setPointerCaptured(0);
	}

    oldview = view;

    if ((view == PLAYERVIEW) ||
	(view == FIRSTPERSONVIEW)) /* must be before view change!!!!*/
	yrot = (GLfloat) -playerEntityState.headHorzRotate;
    else if (view == OMNISCIENTVIEW)
	{
	ysaverot = yrot;
	xsaverot = xrot;
	}

    if ((mode == DEMOMODE) && !playerEntityState.dead && !paused)
	playerEntityState = autopilot(playerEntityState.x, playerEntityState.z, playerEntityState);


    for(temptarget = targets->next; temptarget != NULL; temptarget = temptarget->next)
	temptarget->player = autopilot(temptarget->x, temptarget->z, temptarget->player);


    /**********************************/
    /* update everything              */
    /**********************************/

    if (netUp)
	processNetwork(now, mainCounter, totalCounter, mapObjectList,
		       actorlist, arena, playerEntityState, destroyedMapObjectHistoryCount,
		       destroyedMapObjectFrameCount, destroyedMapObjectHistory,
		       destroyedMapObjectFrame);

    if (client)
	processClient(now, mainCounter, totalCounter, mapObjectList,
		      actorlist, arena, sizeActor, playerEntityState,
		      destroyedMapObjectFrameCount, destroyedMapObjectFrame);

    if (!paused)
	doUpdate();

#ifdef SOLARIS
    else	/* sleep a while */
	{
	static struct timespec rqtp, rmtp;
	rqtp.tv_sec = 0;
	rqtp.tv_nsec = 500 * 1000000;	/* 500 ms delay */
	nanosleep(&rqtp, &rmtp);
	}
#endif
#ifdef SGIVERSION
    else
	sleep(1);
#endif

    /*************************************************/
    /* rotate the plane in demo mode omniscient view */
    /*************************************************/

    if ((view == OMNISCIENTVIEW) && (mode == DEMOMODE))
	{
	if (!paused)
	    yrot += 10;
	xrot = 300;
	}

    /************************************************/
    /* find newest vehicle visible on the plane     */
    /************************************************/

    lastActorY = NOACTORONPLANE;
    lastActorTheta = lastActorRad = 0;

    for(tactor = actorlist->next; (tactor != NULL) && (lastActorY <= NOACTORONPLANE); tactor = tactor->next)
	if ((fabs(tactor->x - playerEntityState.x) <= PLANESIZE) &&
	    (fabs(tactor->z - playerEntityState.z) <= PLANESIZE) &&
	    (tactor->damage > 0) && (tactor->rad < 10))
	    {
	    lastActorY	    = tactor->y + 0.4;
	    lastActorTheta   = tactor->theta + PI;
	    lastActorRad	    = tactor->rad + 1.5;
	    }

    /************************************************/
    /* ensure all player variables are within range*/
    /************************************************/

    if (xrot > 600)
	xrot = 600;

    if (xrot < 0)
	xrot = 0;

    if (yrot < 0)
	yrot += 3600;

    if (yrot >= 3600)
	yrot -= 3600;

    if (playerEntityState.headHorzRotate < 0)
	playerEntityState.headHorzRotate += 3600;

    if (playerEntityState.headHorzRotate >= 3600)
	playerEntityState.headHorzRotate -= 3600;

    if (playerEntityArchetype != NULL)
	{
	if (playerEntityState.headVertRotate <
	    (int)(playerEntityArchetype->aimPitchMin * 10.0f))
	    playerEntityState.headVertRotate =
		(int)(playerEntityArchetype->aimPitchMin * 10.0f);
	if (playerEntityState.headVertRotate >
	    (int)(playerEntityArchetype->aimPitchMax * 10.0f))
	    playerEntityState.headVertRotate =
		(int)(playerEntityArchetype->aimPitchMax * 10.0f);
	}
    else
	{
	if (playerEntityState.headVertRotate < -400)
	    playerEntityState.headVertRotate = -400;
	if (playerEntityState.headVertRotate > 400)
	    playerEntityState.headVertRotate = 400;
	}

    if (targets->next != NULL)
	{
	for(temptarget = targets->next; temptarget != NULL; temptarget = temptarget->next)
	    {
	    if (temptarget->player.headHorzRotate < 0)
		temptarget->player.headHorzRotate += 3600;

	    if (temptarget->player.headHorzRotate >= 3600)
		temptarget->player.headHorzRotate -= 3600;

	    if (temptarget->player.headVertRotate < -400)
		temptarget->player.headVertRotate = -400;

	    if (temptarget->player.headVertRotate > 400)
		temptarget->player.headVertRotate = 400;
	    }
	}

    /************************************************/
    /* draw everything                              */
    /************************************************/

    doDisplay();

    /***************************************************/
    /* limit the game to about 20fps for fast machines */
    /***************************************************/


    gettimeofday(&endTime, NULL);


    while (((endTime.tv_sec - startTime.tv_sec) * 1000 +
	    (endTime.tv_usec - startTime.tv_usec) * 0.001) < 45)
	{
#ifdef SGIVERSION
	sginap(1);
# endif
# ifdef SOLARIS
	static struct timespec rqtp, rmtp;

	rqtp.tv_sec = 0;
	rqtp.tv_nsec = 2 * 1000000;	/* 2 ms delay */
	nanosleep(&rqtp, &rmtp);
# endif
#if defined(LINUXVERSION) || defined(MACOSVERSION)
	usleep(2000); /* 2ms delay */
#endif
	gettimeofday(&endTime, NULL);
	}

    startTime = endTime;
   }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void showCommands(char * progName)
    {
    fprintf (stderr, "\n\n");

#ifdef SOLARIS
    fprintf (stderr, "Usage: %s [-a] [-b] [-d level] [-m] [-s] [--map name] [--stereo mode] [--no-audio-smoothing] [--no-shadows] [-help]\n", progName);
#else
    fprintf (stderr, "Usage: %s [-a] [-b] [-d level] [-m] [-s] [--map name] [--stereo mode] [--no-audio-smoothing] [--no-shadows] [-c] [-h #] [-n] [-p #] [-help]\n", progName);
#endif

    fprintf (stderr, "  -a)lone        fight alone\n");
    fprintf (stderr, "  -b)ackdrop     hide the overlays\n");
    fprintf (stderr, "  -d)etail n     set level-of-detail from 0 (low) to 2 (high)\n");
    fprintf (stderr, "  -m)usic        initially turn on the music\n");
    fprintf (stderr, "  -s)ound        initially turn on the sound effects\n");
    fprintf (stderr, "  --map name     select a battlefield by number or city name\n");
    fprintf (stderr, "  --player id    select a controllable entity archetype\n");
    fprintf (stderr, "  --view n       select view 0, 1, 2, 4, or 5\n");
    fprintf (stderr, "  --stereo mode  select off, top-bottom, or side-by-side\n");
    fprintf (stderr, "                  V cycles stereo mode while running\n");
    fprintf (stderr, "  --no-audio-smoothing  disable spatial audio interpolation\n");
    fprintf (stderr, "  --audio-smoothing     enable spatial audio interpolation\n");
    fprintf (stderr, "  --no-shadows          disable dynamic projected shadows\n");
    fprintf (stderr, "  --shadows             enable dynamic projected shadows\n");
    fprintf (stderr, "  F5 toggles shadows; [ and ] change time of day\n");
    fprintf (stderr, "  -help          show this list of command line options\n\n");

    battalionMapPrintAvailable(dataPath, stderr);
    fprintf (stderr, "\n");

    fprintf (stderr, "Available player entities:\n");
    {
    size_t entityIndex;
    for (entityIndex = 0; entityIndex < gameEntities.count; entityIndex++)
	if (gameEntities.entries[entityIndex].controllable)
	    fprintf(stderr, "  %s\n", gameEntities.entries[entityIndex].id);
    }
    fprintf (stderr, "\n");

#ifdef SOLARIS
    fprintf (stderr, "Multiplayer options not yet supported:\n");
#else
     fprintf (stderr, "   Multiplayer options:\n");
#endif

    fprintf (stderr, "  -c)lient       declare yourself as a client in a net game\n");
    fprintf (stderr, "  -h)ost #       give the address of the host in a net game\n");
    fprintf (stderr, "  -n)etwork      declare yourself as a server in a net game\n");
    fprintf (stderr, "  -p)ort #       declare the port # being used in a net game\n");

    exit(1);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* main                                                          */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void preparseMapArguments(int argc, char **argv)
    {
    int argument;

    for (argument = 1; argument < argc; argument++)
	if ((strcmp(argv[argument], "--map") == 0) ||
	    (strcmp(argv[argument], "-map") == 0) ||
	    (strcmp(argv[argument], "-level") == 0) ||
	    (strcmp(argv[argument], "-l") == 0))
	    {
	    if ((argument + 1) >= argc)
		{
		fprintf(stderr, "%s requires a map number or city name\n", argv[argument]);
		exit(1);
		}
	    battalionMapRequest(argv[++argument]);
	    }
	else if (strcmp(argv[argument], "--player") == 0)
	    {
	    if ((argument + 1) >= argc ||
		strlen(argv[argument + 1]) >= sizeof(requestedPlayerEntity))
		{
		fprintf(stderr, "--player requires an entity archetype\n");
		exit(1);
		}
	    strcpy(requestedPlayerEntity, argv[++argument]);
	    }
	else if (strcmp(argv[argument], "--view") == 0)
	    {
	    if ((argument + 1) >= argc)
		{
		fprintf(stderr, "--view requires 0, 1, 2, 4, or 5\n");
		exit(1);
		}
	    argument++;
	    if (strcmp(argv[argument], "0") == 0)
		requestedView = OMNISCIENTVIEW;
	    else if (strcmp(argv[argument], "1") == 0)
		requestedView = PLAYERVIEW;
	    else if (strcmp(argv[argument], "2") == 0)
		requestedView = ARMYVIEW;
	    else if (strcmp(argv[argument], "4") == 0)
		requestedView = MAPVIEW;
	    else if (strcmp(argv[argument], "5") == 0)
		requestedView = FIRSTPERSONVIEW;
	    else
		{
		fprintf(stderr, "Unknown view: %s\n", argv[argument]);
		exit(1);
		}
	    }
	else if (strcmp(argv[argument], "--stereo") == 0)
	    {
	    if ((argument + 1) >= argc ||
	        !cameraStereoLayoutFromName(argv[argument + 1],
	                                    &requestedStereoLayout))
		{
		fprintf(stderr,
		        "--stereo requires off, top-bottom, or side-by-side\n");
		exit(1);
		}
	    argument++;
	    }
    }

static int selectRequestedPlayerEntity(void)
    {
    const struct EntityArchetype *requested;

    if (requestedPlayerEntity[0] == '\0')
        return 1;
    requested = entityCatalogFind(&gameEntities, requestedPlayerEntity);
    if (!setPlayerEntityArchetype(requested))
        return 0;
    mode = PLAYMODE;
    showOptions = 0;
    showHelp = 0;
    return 1;
    }

main (int argc, char*argv[])
    {
    register int argument;
    int port;
    char portName[40];

#if defined(SGIVERSION) || defined(SOLARIS) || defined(LINUXVERSION) || defined(MACOSVERSION)

    gettimeofday(&startTime, NULL);

#endif

    frames = 0;

#if defined(SGIVERSION) || defined(SUNVERSION) || defined(SOLARIS)
    shadowsEnabled = 0;
#else
    shadowsEnabled = 1;
#endif

    preparseMapArguments(argc, argv);
    initialization();
    if (!selectRequestedPlayerEntity())
	{
	fprintf(stderr, "Unknown or non-controllable player entity: %s\n",
		requestedPlayerEntity);
	exit(1);
	}

    for (argument=1;argument < argc;argument++)
	{
	if (((strcmp(argv[argument], "-sound") == 0) ||
	    (strcmp(argv[argument], "-s") == 0) )&& !noSound)
	    turnSoundOn();

	else if (((strcmp(argv[argument], "-music") == 0) ||
	    (strcmp(argv[argument], "-m") == 0)) && !noSound)
	    turnMusicOn();

	else if ((strcmp(argv[argument], "-alone") == 0) ||
	    (strcmp(argv[argument], "-a") == 0))
	    alone = 1;

	else if ((strcmp(argv[argument], "--map") == 0) ||
		 (strcmp(argv[argument], "-map") == 0) ||
		 (strcmp(argv[argument], "-level") == 0) ||
		 (strcmp(argv[argument], "-l") == 0))
	    {
	    if ((argument + 1) < argc)
		argument++;
	    else
		showCommands(argv[0]);
	    }

	else if (strcmp(argv[argument], "--player") == 0)
	    {
	    if ((argument + 1) < argc)
		argument++;
	    else
		showCommands(argv[0]);
	    }

	else if (strcmp(argv[argument], "--view") == 0)
	    {
	    if ((argument + 1) < argc)
		argument++;
	    else
		showCommands(argv[0]);
	    }

	else if (strcmp(argv[argument], "--stereo") == 0)
	    {
	    if ((argument + 1) < argc)
		argument++;
	    else
		showCommands(argv[0]);
	    }

	else if (strcmp(argv[argument], "--no-audio-smoothing") == 0)
	    audioSetSmoothingEnabled(0);

	else if (strcmp(argv[argument], "--audio-smoothing") == 0)
	    audioSetSmoothingEnabled(1);

	else if (strcmp(argv[argument], "--no-shadows") == 0)
	    shadowsEnabled = 0;

	else if (strcmp(argv[argument], "--shadows") == 0)
	    shadowsEnabled = 1;

	else if ((strcmp(argv[argument], "-detail") == 0) ||
		 (strcmp(argv[argument], "-d") == 0))
	    {
	    if ((argument + 1) < argc)
		{
		lod = atoi(argv[argument + 1]);
		if (lod < 0 || lod > 2)
		    showCommands(argv[0]);
		argument++;
		}
	    else
		showCommands(argv[0]);
	    }

	else if ((strcmp(argv[argument], "-network") == 0) ||
	    (strcmp(argv[argument], "-net") == 0) ||
	    (strcmp(argv[argument], "-n") == 0))
	    netUp = 1;

	else if ((strcmp(argv[argument], "-backdrop") == 0) ||
	    (strcmp(argv[argument], "-b") == 0))
	    backdrop = 1;

	else if ((strcmp(argv[argument], "-client") == 0) ||
	    (strcmp(argv[argument], "-c") == 0))
	    client = 1;

	else if ((strcmp(argv[argument], "-host") == 0) ||
		 (strcmp(argv[argument], "-h") == 0))
	    {
	    if ((argument+1) < argc)
		{
		setHostAddr(argv[argument+1]);
		argument++;
		}
	    else
		showCommands(argv[0]);
	    }
	else if ((strcmp(argv[argument], "-port") == 0) ||
		   (strcmp(argv[argument], "-p") == 0))
	    {
	    if ((argument+1) < argc)
		{
		strcpy(portName, argv[argument+1]);
		sscanf(portName, "%d", &port);
		setPortNumber(port);
		argument++;
		}
	    else
		showCommands(argv[0]);
	    }
	else if ((strcmp(argv[argument], "-help") == 0))
		{
		showCommands(argv[0]);
		}
	else
	    {
	    showCommands(argv[0]);
	    }
	}

    if ((netUp || client) && playerEntityArchetype == NULL)
	{
	showError("Selected player entity is unavailable");
	exit(1);
	}

    setStereoLayout(requestedStereoLayout);

    /**********************************************/
    /* set starting conditions for chosen player */
    /* (here so network options are used          */
    /**********************************************/

    setPlayConditions();
    if (requestedView >= 0)
	view = requestedView;
    if (requestedPlayerEntity[0] != '\0')
	setPointerCaptured(1);


    /******************/
    /* set up network */
    /******************/

    if ((netUp == 1) && (client == 1))
	{
	showError("must be either a host or a client, not both");
	exit(1);
	}

    if (netUp)
	{
	netUp = setUpNetwork();
	}

    if (client)
	{
	client = setUpClient();
	}

    tkExec();

return 0;

}
