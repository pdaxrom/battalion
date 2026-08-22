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
/* battalion.c  v 1.3a                                                 */
/* main routines for battalion                                         */
/***********************************************************************/
/*                                                */
/* code within '#ifdef SOLARIS' by Johan Hagman   */
/*                                                */
/**************************************************/

#include "battalion.h"
#include "map.h"
#include "map_package.h"

/*****************/
/* network stuff */
/*****************/

int buildingBoomSoFarCounter;
int buildingBoomSoFar[800];

int buildingBoomThisFrameCounter;
int buildingBoomThisFrame[20];
int currentTankNumber;

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

int killtanks, killmtanks, killhelos;
int killCHHs, killmechags,  killheros,
killfighters, killplanes, killlaunchers;

int mainCounter;
int totalCounter;
int firingDelay;
int arrivalTime;
int showframes; /* also used to filter out repetitive sounds */
float accuracy;

int treeID;

/****************/
/* window stuff */
/****************/

long windowOriginX, windowOriginY, windowSizeX, windowSizeY;
long monitorType;
long windowWide, windowTall, windowLeft, windowTop;
GLuint viewL, viewR;
long zmax;
int doBigClear;

float lastTankY, lastTankTheta, lastTankRad;

int pointerGrab;

/*********/
/* lists */
/*********/

struct road * roadSystem;
struct projectile * projectFlight;
struct boom * projectboom;
struct tank * tanklist;
struct tank * slaglist;
struct tree * treelist;
struct monsterInfo Googelon;

static struct BattalionMapPackage battlefieldMap;
static struct EntityCatalog gameEntities;
static char requestedPlayerEntity[ENTITY_NAME_SIZE];
static int requestedView = -1;
static const struct EntityArchetype *playerEntityArchetype;
static size_t playerEntitySelection;

const struct EntityArchetype *selectedPlayerArchetype(void)
    {
    return playerEntityArchetype;
    }

const struct EntityArchetype *networkEntityArchetype(int networkCode)
    {
    return entityCatalogFindNetworkCode(&gameEntities, networkCode);
    }

size_t playerEntityOptionCount(void)
    {
    return entityCatalogControllableCount(&gameEntities);
    }

size_t selectedPlayerArchetypeNumber(void)
    {
    return playerEntitySelection;
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
            Googelon.dataArchetype = candidate;
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
    Googelon.dataArchetype = playerEntityArchetype;
    }

static const struct EntityArchetype *monsterArchetype(
    const struct monsterInfo *monster)
    {
    if (monster == NULL)
        return NULL;
    if (monster->dataArchetype != NULL)
        return monster->dataArchetype;
    if (monster->monster == DATAENTITY)
        return playerEntityArchetype;
    return networkEntityArchetype(monster->monster);
    }

static int monsterHasSolidCollision(const struct monsterInfo *monster)
    {
    const struct EntityArchetype *archetype;

    archetype = monsterArchetype(monster);
    return archetype == NULL ||
           archetype->collisionMode == ENTITY_COLLISION_SOLID;
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

static int entityAssetsAvailable(const struct EntityArchetype *archetype)
    {
    size_t node;

    if (assetModelDisplayList(archetype->model) == 0)
        return 0;
    if (archetype->destroyedModel[0] != '\0' &&
        assetModelDisplayList(archetype->destroyedModel) == 0)
        return 0;
    if (archetype->shadowModel[0] != '\0' &&
        assetModelDisplayList(archetype->shadowModel) == 0)
        return 0;
    if (!assetModelHasNode(archetype->model, archetype->aimYawNode) ||
        !assetModelHasNode(archetype->model, archetype->aimPitchNode))
        return 0;
    for (node = 0; node < archetype->renderNodeCount; node++)
        if (!assetModelHasNode(archetype->model,
                               archetype->renderNodes[node]))
            return 0;
    return assetAnimationAvailable(archetype->idleAnimation) &&
           assetAnimationAvailable(archetype->moveAnimation) &&
           assetAnimationAvailable(archetype->attackAnimation);
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
    const struct EntityArchetype *archetype;
    size_t index;

    catalogPath = assetFilePath("game-entities");
    entityCatalogInitialize(&gameEntities);
    if (!entityCatalogLoad(dataPath, catalogPath, &gameEntities,
                           error, errorSize))
        return 0;
    for (index = 0; index < gameEntities.count; index++)
        if (!entityAssetsAvailable(&gameEntities.entries[index]))
            {
            setGameEntityError(error, errorSize,
                               "An entity references missing assets");
            return 0;
            }
    for (index = 0; index < battlefieldMap.actorCount; index++)
        {
        archetype = entityCatalogFind(
            &gameEntities, battlefieldMap.actors[index].archetype);
        if (archetype == NULL)
            {
            setGameEntityError(error, errorSize,
                               "A map actor has an unknown archetype");
            return 0;
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

/*****************/
/* monster stuff */
/*****************/

float offsetX; /* needed global to bank flutter */
struct targetInfo * targets;


/*********************/
/* battlefield stuff */
/*********************/

float globalxshift, globalzshift;

GLfloat xrot, xsaverot, yrot, ysaverot;

struct tree allTreesOnPlane[MAXTREESONPLANE];
struct tree * allTreesOnPlanePtrs[MAXTREESONPLANE];

/* needed for the targets to blow up trees off the player's plane */
int numTreesEverywhere;
struct tree allTreesEverywhere[MAXTREESONPLANE];
struct tree * allTreesEverywherePtrs[MAXTREESONPLANE];

int numTreesOnPlane;

struct fireType fires[MAXFIRESONPLANE];
int flameCount;

struct tank * allTanks2OnPlane[MAXTANKSONPLANE];
struct tank allTanksOnPlane[MAXTANKSONPLANE];
int numTanksOnPlane;

int frameCount;
int outFrameCount;
time_t frameTime;

GLuint monsterLookat;
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
static int commandKeys[5];

static void clearPlayerInput(void);
static void setPointerCaptured(int);
static void inputFocusChanged(int);

float textLineWidth;

struct timeval startTime, endTime;

char playerName[16];
char playerHome[256];

struct score G[3];
struct score F[3];
struct score V[3];
struct score T[3];
int multipleHighScores;

struct projectile * tproj,  *deadproj;
struct boom * tboom, *deadboom;

void * arena;
void * sharedmem;

int sizeBoom;
int sizeTank;
int sizeProjectile;

int netUp;
int client;

int biggestTankNumberSent;

int playerWeaponUsesReload(void)
    {
    if (Googelon.monster != DATAENTITY || playerEntityArchetype == NULL)
	return 0;
    return playerEntityArchetype->weapon == ENTITY_WEAPON_SHELL ||
	   playerEntityArchetype->weapon == ENTITY_WEAPON_ROCKET ||
	   playerEntityArchetype->weapon == ENTITY_WEAPON_PLASMA;
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

void updateScores(char * scorefullPath, int monster, int monsterScore, int mode, char * playerName)
    {
    char thaName[MAXSTRING];
    FILE * hiScoreFile;
    char garbage;
    struct score * currentOne;
    int c;
    int position;
    int current;

    if (mode == DEMOMODE)
	strcpy(thaName, "demo");
    else
	strcpy(thaName, playerName);

    hiScoreFile = fopen(scorefullPath, "r");

    if (hiScoreFile != NULL)
	{
	for(c=0;c<3;c++)
	    fscanf(hiScoreFile, "%d%c%255s", &(G[c].number), &garbage, G[c].name);
	for(c=0;c<3;c++)
	    fscanf(hiScoreFile, "%d%c%255s", &(V[c].number), &garbage, V[c].name);
	for(c=0;c<3;c++)
	    fscanf(hiScoreFile, "%d%c%255s", &(F[c].number), &garbage, F[c].name);
	for(c=0;c<3;c++)
	    fscanf(hiScoreFile, "%d%c%255s", &(T[c].number), &garbage, T[c].name);

	fclose(hiScoreFile);

	switch (monster){
	    case GOOGELON:	currentOne = G;
				break;

	    case TECHS:		currentOne = T;
				break;

	    case VAPOUR:	currentOne = V;
				break;

	    case FLUTTER:	currentOne = F;
				break;

	    case DATAENTITY: currentOne = G;
				break;
	    }

	/* where does the new score belong */
	/***********************************/

	if (monsterScore > currentOne[0].number)
	    position = 0;

	else if (monsterScore > currentOne[1].number)
	    position = 1;

	else if (monsterScore > currentOne[2].number)
	    position = 2;


	if (multipleHighScores)
	    {
	    switch (position) {
		case 0: currentOne[2].number = currentOne[1].number;
			strcpy(currentOne[2].name, currentOne[1].name);

			currentOne[1].number = currentOne[0].number;
			strcpy(currentOne[1].name, currentOne[0].name);

			currentOne[0].number = monsterScore;
			strcpy(currentOne[0].name, thaName);
			break;

		case 1: currentOne[2].number = currentOne[1].number;
			strcpy(currentOne[2].name, currentOne[1].name);

			currentOne[1].number = monsterScore;
			strcpy(currentOne[1].name, thaName);
			break;

		case 2: currentOne[2].number = monsterScore;
			strcpy(currentOne[2].name, thaName);
			break;
		}
	    }
	else
	    {
	    /* does the player currently have a score for this monster */
	    /***********************************************************/

	    current = -1;

	    if (same(thaName, currentOne[0].name))
		current = 0;
	    else if(same(thaName, currentOne[1].name))
		current =  1;
	    else if (same(thaName, currentOne[2].name))
		current =  2;

	    switch (position) {
		case 0: switch (current)
			    {
			    case 0:	currentOne[0].number = monsterScore;
					strcpy(currentOne[0].name, thaName);
					break;

			    case 1:	currentOne[1].number = currentOne[0].number;
					strcpy(currentOne[1].name, currentOne[0].name);

					currentOne[0].number = monsterScore;
					strcpy(currentOne[0].name, thaName);
					break;

			    case -1:
			    case 2:	currentOne[2].number = currentOne[1].number;
					strcpy(currentOne[2].name, currentOne[1].name);

					currentOne[1].number = currentOne[0].number;
					strcpy(currentOne[1].name, currentOne[0].name);

					currentOne[0].number = monsterScore;
					strcpy(currentOne[0].name, thaName);
					break;
			    }


			break;

		case 1: switch (current)
			    {
			    case 0:	break;

			    case 1:	currentOne[1].number = monsterScore;
					strcpy(currentOne[1].name, thaName);
					break;

			    case -1:
			    case 2:	currentOne[2].number = currentOne[1].number;
					strcpy(currentOne[2].name, currentOne[1].name);

					currentOne[1].number = monsterScore;
					strcpy(currentOne[1].name, thaName);
					break;
			    }


			break;

		case 2: switch (current)
			    {
			    case 0:	break;

			    case 1:	break;

			    case -1:
			    case 2:	currentOne[2].number = monsterScore;
					strcpy(currentOne[2].name, thaName);
					break;
			    }

			break;
		}
	    }

	hiScoreFile = fopen(scorefullPath, "w");

	for(c=0;c<3;c++)
	    fprintf(hiScoreFile, "%d%c%s\n", G[c].number, ' ', G[c].name);
	for(c=0;c<3;c++)
	    fprintf(hiScoreFile, "%d%c%s\n", V[c].number, ' ', V[c].name);
	for(c=0;c<3;c++)
	    fprintf(hiScoreFile, "%d%c%s\n", F[c].number, ' ', F[c].name);
	for(c=0;c<3;c++)
	    fprintf(hiScoreFile, "%d%c%s\n", T[c].number, ' ', T[c].name);

	fclose(hiScoreFile);
	}
    }


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void quickTanks()
    {
    struct tank *temptank;

    /**********************************************************/
    /* copy all vehicles everywhere into quick-access array   */
    /**********************************************************/

    numTanksOnPlane = 0;
    for(temptank = tanklist->next;(temptank != NULL);temptank = temptank->next)
	{
	    allTanks2OnPlane[numTanksOnPlane] = temptank;
	    allTanksOnPlane[numTanksOnPlane] = *temptank;
	    numTanksOnPlane += 1;
	    if (numTanksOnPlane >= MAXTANKSONPLANE)
		{
		showError("Too many tanks on plane");
		numTanksOnPlane -= 1;
		}
	}

    for(temptank = slaglist->next;(temptank != NULL);temptank = temptank->next)
	{
	    allTanks2OnPlane[numTanksOnPlane] = temptank;
	    allTanksOnPlane[numTanksOnPlane] = *temptank;
	    numTanksOnPlane += 1;
	    if (numTanksOnPlane >= MAXTANKSONPLANE)
		{
		showError("Too Many Tanks on Plane");
		numTanksOnPlane -= 1;
		}
	}

    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* determine whether a vehicle is blocked                        */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int checkForBlock(struct tank * ttank, float rad, float theta, float y)
    {
    register int tankCounter;

    struct tank* ttank2;
    struct tree * temptree;
    struct targetInfo * temptarget;
    float tankInRad, tankOutRad;
    float treeInRad, treeOutRad;
    float raddy;
    float angle;
    float targx,  targz;
    float rad2;
    float angDiff;
    int useTarg;
    int blocked;

    tankInRad	= rad + 0.2;
    tankOutRad	= rad - 0.8;
    treeInRad	= rad + 0.3;
    treeOutRad	= rad - 1.2;


    blocked = 0;

    /**********************************************/
    /* is the vehicle blocked by another vehicle? */
    /**********************************************/

    tankCounter = 0;
    while ((tankCounter < numTanksOnPlane) && (!blocked))
	{
	ttank2 = allTanks2OnPlane[tankCounter];
	rad2 = ttank2->rad;

	if ((ttank->goforit == ttank2->goforit) &&
	    (tankInRad >= rad2)  &&	     /* check radius */
	    (tankOutRad <= rad2) &&
	    (ttank2 != ttank)    &&		    /* not same tank */
	    (fabs(theta - ttank2->theta) < 0.22) &&   /* check angle */
	    (fabs(ttank2->y - y) <= 0.6)	    /* check altitude */
	    )
		blocked = 1;

	tankCounter += 1;
	}

    /**********************************************************/
    /* is the vehicle blocked by a structure (tree,building)? */
    /**********************************************************/

    useTarg = 0;
    for(temptarget = targets->next; temptarget != NULL; temptarget = temptarget->next)
	if (&(temptarget->monster) == ttank->goforit)
	    {
	    targx = temptarget->x;
	    targz = temptarget->z;
	    useTarg = 1;
	    }

	for(temptree = treelist->next;(temptree != NULL) && (!blocked); temptree = temptree->next)
	    {
	    if (!temptree->intact)
		continue;

	    if (useTarg) /* radius to the target */
		{
		raddy = sqrt((temptree->x - targx) * (temptree->x - targx) + (temptree->z - targz) * (temptree->z - targz));
		}
	    else
		{
		raddy = temptree->rad; /* radius and theta to the player's monster */
		}

	    if ((treeInRad >= raddy) && (treeOutRad <= raddy) && (y <= (temptree->height + 0.5)))
		{
		if (useTarg) /* theta to the target */
		    {
		    angle = computeAngle(temptree->z, targz, temptree->x, targx);
		    }
		else
		    {
		    angle = temptree->theta;
		    }

		if (angle < 0)
		    angle += TWOPI;
		else if (angle >= TWOPI)
		    angle -= TWOPI;

		if (theta < 0)
		    theta += TWOPI;
		else if (theta >= TWOPI)
		    theta -= TWOPI;

		angDiff = theta - angle;

		if ((fabs(angDiff) < 0.31) || (fabs(angDiff + TWOPI) < 0.31) ||
		    (fabs(angDiff - TWOPI) < 0.31))
		    blocked = 1;
		}
	    }

    return (blocked);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* increase monster's score based on kill                        */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void addScore(int tank, struct monsterInfo * m)
    {
    switch(tank){
	case TANK:	killtanks += 1;
			if (m!= NULL)
			    m->monsterScore += 5;
			break;

	case LAUNCHER:	killlaunchers += 1;
			if (m!= NULL)
			    m->monsterScore += 10;
			break;

	case MASERTANK: killmtanks += 1;
			if (m!= NULL)
			    m->monsterScore += 15;
			break;

	case HELO:	killhelos += 1;
			if (m!= NULL)
			    m->monsterScore += 8;
			break;

	case MECHAG:	killmechags += 1;
			break;

	case CHH:	killCHHs += 1;
			break;

	case HERO:	killheros += 1;
			break;

	case AIRPLANE:	killplanes += 1;
			if (m!= NULL)
			    m->monsterScore += 25;
			break;

	case FIGHTER:	killfighters += 1;
			if (m!= NULL)
			    m->monsterScore += 20;
			break;

	case BUILDING:	if (m!= NULL)
			    m->monsterScore += 50;
			break;

	case PARTIALM:	if (m!= NULL)
			    m->monsterScore += 4;
			break;

	case PARTIALH:	if (m!= NULL)
			    m->monsterScore += 5;
			break;

	case PARTIALC:	if (m!= NULL)
			    m->monsterScore += 3;
			break;

	case MONSTER:	if (m!= NULL)
			    m->monsterScore += 1;
			break;

	default:	showError("Bogus Score! (addScore)");
			break;
	}
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* add a new explosion                                           */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void addBoom(float x,  float y,  float z, struct monsterInfo* m)
    {
    struct boom * tempboom2;
    struct boom tempboom;

    tempboom2 = (struct boom *) amalloc(sizeBoom, arena);

    if (tempboom2 != NULL)
	{
	tempboom.mine	= m;
	tempboom.count	= 1;
	tempboom.x	= x;

	if (y >= PLANEY)
	    tempboom.y = y;
	else
	    tempboom.y = PLANEY;

	tempboom.z	    = z;
	tempboom.next	    = projectboom->next;
	*tempboom2	    = tempboom;
	projectboom->next   = tempboom2;
	}
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* add a new projectile                                          */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void addProjectile( float x,  float y,  float z, int type,
		    float xv,  float yv,  float zv, float accuracy, struct monsterInfo * m)
    {
    struct projectile * temp;
    struct projectile tempProj;


    switch(type){
	case PROJTANK:	    doSound(TANKFIRE);
			    break;

	case PROJROCKET:    doSound(HELOROCKET);
			    break;

	case PROJTECHS:
	case PROJHERO:	    doSound(TECHSHOOT);
			    break;

	}

    temp = (struct projectile *) amalloc(sizeProjectile, arena);

    if (temp != NULL)
	{
	tempProj.count = 0;

	tempProj.mine = m;

	tempProj.x = x;
	tempProj.y = y;
	tempProj.z = z;

	tempProj.type = (char) type;

	tempProj.xv = xv;
	tempProj.yv = yv;
	tempProj.zv = zv;

	if (accuracy != 0)
	    {
	    tempProj.xv += randy(5.0)/accuracy;
	    tempProj.yv += randy(5.0)/accuracy;
	    tempProj.zv += randy(5.0)/accuracy;
	    }


	tempProj.next = projectFlight->next;
	*temp = tempProj;
	projectFlight->next = temp;
	}
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* update all projectiles on the battlefield                     */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int projectilePathHitsTank(struct tank *tank,
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
	if (tankHit(tank,
		startX + dx * amount,
		startY + dy * amount,
		startZ + dz * amount))
	    return 1;
	}
    return 0;
    }

static int projectilePathHitsStructure(const struct tree *structure,
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

    if (!structure->intact || structure->type == 2)
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
	if ((fabs(structure->x - x) < TREEBLASTR) &&
	    (fabs(structure->z - z) < TREEBLASTR) &&
	    (fabs(structure->y + structure->height - y) <
		TREEBLASTHEIGHTR))
	    return 1;
	}
    return 0;
    }

static int dataEntityLineOfSight(const struct tank *source,
	float targetX, float targetY, float targetZ)
    {
    const struct tree *structure;
    float sourceY;

    sourceY = source->y + source->dataArchetype->collisionHeight * 0.5f;
    for (structure = treelist->next; structure != NULL;
	 structure = structure->next)
	{
	/* A structure containing either endpoint is not an intervening
	   occluder.  The existing projectile footprint is also the established
	   structure footprint used by perception. */
	if ((fabs(structure->x - source->x) < TREEBLASTR &&
	     fabs(structure->z - source->z) < TREEBLASTR) ||
	    (fabs(structure->x - targetX) < TREEBLASTR &&
	     fabs(structure->z - targetZ) < TREEBLASTR))
	    continue;
	if (projectilePathHitsStructure(structure,
		source->x, sourceY, source->z,
		targetX, targetY, targetZ))
	    return 0;
	}
    return 1;
    }

static int dataEntityRecentlyFired(const struct tank *tank)
    {
    int threshold;

    if (tank->dataArchetype->weapon == ENTITY_WEAPON_NONE ||
	 tank->dataAgent.cooldown <= 0)
	return 0;
    threshold = tank->dataArchetype->fireInterval - 3;
    if (threshold < 1)
	threshold = 1;
    return tank->dataAgent.cooldown >= threshold;
    }

static int projectileDestroysStructure(int projectileType,
	const struct tree *structure)
    {
    if (structure->type != 0 && structure->type != 1)
	return 0;
    return projectileType == PROJTANK || projectileType == PROJROCKET ||
	   projectileType == PROJTECHS || projectileType == PROJFIRE ||
	   projectileType == PROJHERO;
    }

void updateProjectiles()
    {
    float px, py, pz;
    float qx, qy, qz;
    float floor, ceiling;
    int i, dead;
    int inPlay, monsterHit, groundHit;
    struct targetInfo * temptarget;
    struct monsterInfo thaMonster;
    float monsterX, monsterZ;

    struct projectile * pproj, *killer, *temp;

    int tankCounter, treeCounter;
    float maxX, maxZ, minX, minZ;

    maxX = maxZ = minX = minZ = 0;

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

    flameCount = 0;

    temp = projectFlight;
    while (temp->next != NULL)
	{
	dead = 0;

	pproj = temp->next;

	pproj->count += 1;

	if (pproj->count > ARMCOUNT)
	    pproj->count = ARMCOUNT;

	/**********************************/
	/* is projectile still in play?   */
	/**********************************/

	px = fabs(pproj->x);
	py = pproj->y;
	pz = fabs(pproj->z);

	qx = pproj->x;
	qy = pproj->y;
	qz = pproj->z;

	monsterHit = 0;

	/**************************************************/
	/* did the projectile hit the player's monster?   */
	/**************************************************/

	if ((pproj->type != PROJWATER) && (pproj->type != PROJFIRE) && (pproj->type != PROJSCAR))
    	    {
	    if ((Googelon.monsterIsDead != 1) &&
		(px <= Googelon.width) && (pz <= Googelon.width) &&
		(py <= Googelon.height) && (py >= Googelon.bottom))
		    {
		    if ((pproj->mine != &Googelon) &&
		        ((pproj->type != PROJTECHS) ||
		        ((pproj->type == PROJTECHS) &&
		         (pproj->mine != &Googelon))))
			{
			Googelon.energyRemaining -= BLASTPOINTS;
			monsterHit = 1;

    /*
			if (pproj->mine != NULL)
			    pproj->mine->monsterScore += 1;
    */
			}
		    }

	    /********************************************/
	    /* did the projectile hit another target?   */
	    /********************************************/

	    for(temptarget = targets->next;temptarget != NULL;temptarget = temptarget->next)
		{
		thaMonster = temptarget->monster;
		monsterX = temptarget->x;
		monsterZ = temptarget->z;

		if ( (thaMonster.monsterIsDead != 1) &&
		    (qx <= (monsterX+thaMonster.width)) && (qz <= (monsterZ+thaMonster.width)) &&
		    (qx >= (monsterX-thaMonster.width)) && (qz >= (monsterZ-thaMonster.width)) &&
		    (qy <= thaMonster.height) && (qy >= thaMonster.bottom))
			{
			if ((pproj->mine != &(temptarget->monster)) &&
			    ((pproj->type != PROJTECHS) ||
			    ((pproj->type == PROJTECHS) &&
			     (pproj->mine != &(temptarget->monster)))))
			    {
			    temptarget->monster.energyRemaining -= BLASTPOINTS;
			    monsterHit = 1;

    /*
			    if (pproj->mine != NULL)
				pproj->mine->monsterScore += 1;
    */
			    }
			}
		}
	    }

	if (!monsterHit && (py > floor) && (py < ceiling) &&
	    (px > minX) && (pz > minZ) && (px < maxX) && (pz < maxZ))
	    inPlay = 1;
	else
	    inPlay = 0;

	if ((px <= PLANESIZE) && (pz <= PLANESIZE) && (py <= floor) &&
	    (pproj->type != PROJWATER) && (pproj->type != PROJSCAR))
	    groundHit = 1;
	else
	    groundHit = 0;

	if (monsterHit || groundHit)
	    addBoom(pproj->x, pproj->y, pproj->z, pproj->mine);


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

	    /*****************************/
	    /* update all the flames     */
	    /*****************************/

	    if (pproj->type == PROJFIRE)
		{
		for (i=0;i<2;i++)
		    {
		    fires[flameCount].x		= pproj->x + randy(0.15);
		    fires[flameCount].z		= pproj->z + randy(0.15);
		    if (lod == 0) {
			fires[flameCount].xwid	 = 0.07 + randy(0.06);
			fires[flameCount].zwid	 = 0.07 + randy(0.06);
			fires[flameCount].height = 0.15 + randy(0.15);
		    } else {
			fires[flameCount].xwid	 = 0.1 + randy(0.07);
			fires[flameCount].zwid	 = 0.1 + randy(0.07);
			fires[flameCount].height = 0.2 + randy(0.15);
		    }
		    do
			fires[flameCount].col	= 0.9 + randy(0.25);
		    while (fires[flameCount].col > 1);

		    flameCount += 1;
		    if (flameCount >= MAXFIRESONPLANE)
			{
			showError("Too many fires on plane");
			flameCount -= 1;
			}
		    }
		}

	    /**********************************/
	    /* shells affected by gravity     */
	    /**********************************/

	    if ((pproj->type == PROJTANK) || (pproj->type == PROJWATER))
		pproj->yv -= GRAVITY;

	    /**********************************/
	    /* check for a tank hit           */
	    /**********************************/

	    if ((pproj->type != PROJSCAR) && (pproj->type != PROJWATER)
		&& ((pproj->count >= ARMCOUNT) ||
		    (pproj->mine == &Googelon) ||
		    (pproj->type == PROJTECHS) ||
		    (pproj->type == PROJFIRE)))
		{
		tankCounter = 0;
		while (tankCounter < numTanksOnPlane)
		    {
		    if (projectilePathHitsTank(
			&(allTanksOnPlane[tankCounter]),
			qx, qy, qz, px, py, pz))
			{
			addBoom(px, py, pz, pproj->mine);
			dead = 1;
			}
		    tankCounter += 1;
		    }
		}

	    /**********************************/
	    /* check for a tree hit           */
	    /**********************************/

	    if ((pproj->type != PROJSCAR) && (pproj->type != PROJWATER))
		{
		treeCounter = 0;

		while (treeCounter < numTreesEverywhere)
		    {
		    if (projectilePathHitsStructure(
			&(allTreesEverywhere[treeCounter]),
			qx, qy, qz, px, py, pz))
			    {
			    addBoom(px, py, pz, pproj->mine);
			    dead = 1;

			    if (projectileDestroysStructure(
				pproj->type,
				&(allTreesEverywhere[treeCounter])))
				{
				if (allTreesEverywherePtrs[treeCounter]->intact && (allTreesEverywherePtrs[treeCounter]->type == 1))
				    addScore(BUILDING, pproj->mine);

				allTreesEverywherePtrs[treeCounter]->intact = 0;
				}
			    }
		    treeCounter += 1;
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
/* update the state of all the structures                        */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void updatetrees(struct tree * allTrees, int itsChristmas)
    {
    struct tree *temptree, *ttree;
    float x, y, z;
    float Vplane;
    int i, j, k, jmax, l;
    int dead;

    buildingBoomThisFrameCounter = 0;

    Vplane = sqrt(GRAVITY * 1.5) * 0.7071;

    ttree = allTrees;

    while (ttree->next != NULL)
	{
	dead = 0;
	temptree = ttree->next;

	x = temptree->x;
	y = temptree->y;
	z = temptree->z;

	temptree->rad = sqrt(x * x + z * z);

	temptree->theta = computeAngle(z, 0, x, 0);


	/*****************************/
	/* update special structures */
	/*****************************/

	if (temptree->type == 1)
	    {
	    /***********************/
	    /* hero beam-in effect */
	    /***********************/

	    if (temptree->treeshape == 33)
		{
		temptree->deathCount += 1;
		if (temptree->deathCount >= 20)
		    temptree->intact = 0;
		}

	    /************/
	    /* fountain */
	    /************/

	    if ((temptree->treeshape == 29) && (temptree->intact) && (!itsChristmas))
		{
		addProjectile(x, PLANEY+0.6, z, PROJWATER,
			  2*randy(0.005), .05, 2*randy(0.005), 0, NULL);
		}

	    /****************/
	    /* power towers */
	    /****************/

	    if ((temptree->treeshape == 3) && !(temptree->intact))
		{
		temptree->deathCount += 1;

		if (temptree->death2 == -1)
		    {
		    doSound(CRASH);

		    buildingBoomThisFrame[buildingBoomThisFrameCounter++] = temptree->number;
		    buildingBoomSoFar[buildingBoomSoFarCounter++] = temptree->number;

		    temptree->death2 = rand() % 360;
		    temptree->height = -0.4;
		    }

		if (temptree->deathCount > 40)
		    temptree->deathCount = 40;
		}
	    }

	/**********************************************/
	/* buildings / trees disappearing / exploding */
	/**********************************************/

	if (!(temptree->intact) && (((temptree->type == 1) && (temptree->treeshape != 3)) || (temptree->type == 0)))
	    {
	    doSound(CRASH);
	    dead = 1;

	    buildingBoomThisFrame[buildingBoomThisFrameCounter++] = temptree->number;
	    buildingBoomSoFar[buildingBoomSoFarCounter++] = temptree->number;

	    switch(temptree->treeshape)
		{
		case	1:
		case	11:
		case	14:
		case	22:
		case	23:
		case	24: jmax = 5;
			    break;
		case	33: jmax = 0;
			    break;
		default:    jmax = 3;
			    break;
	    	}


	    /**********************/
	    /* structures explode */
	    /**********************/

	    if (temptree->type == 1)
		{
		for (i=-1; i<2; i++)
		    for (j=0; j<jmax; j++)
			for (k=-1; k<2; k++)
			    if ((rand() % 100) < 50)
				{
				addProjectile( x + i * 0.2, y + (j * 0.2) + 0.1, z + k * 0.2, PROJTANK,
				    sin(randy(PI)) * Vplane, Vplane, cos(randy(PI)) * Vplane, 75, NULL);
				}

		if (temptree->treeshape == 9)  /* add water into water tower */
		    {
		    for(l=0;l<30;l++)
			addProjectile(x, PLANEY+1, z, PROJWATER,
			  2*randy(0.02), randy(0.05), 2*randy(0.02), 0, NULL);
		    }
 		}


	    addProjectile( x - 0.2,  PLANEY + 0.14,  z + 0.2, PROJFIRE, 0,  -0.0005,  0, 10000, NULL);
	    addProjectile( x + 0.2,  PLANEY + 0.14,  z + 0.2, PROJFIRE, 0,  -0.0005,  0, 10000, NULL);
	    addProjectile( x      ,  PLANEY + 0.14,  z - 0.2, PROJFIRE, 0,  -0.0005,  0, 10000, NULL);
	    }

	if (dead)
    	    {
	    ttree->next = temptree->next;
	    afree(temptree, arena);
	    }
	else
	    ttree = ttree->next;
	}
    }
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* update all of the vehicles                                    */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void initializeContact(struct EntityContact *contact,
                              const char *id, const char *faction,
                              float x, float z, int active,
                              int visible, int audible)
    {
    memset(contact, 0, sizeof(*contact));
    strcpy(contact->id, id);
    strcpy(contact->faction, faction);
    contact->x = x;
    contact->z = z;
    contact->active = active;
    contact->visible = visible;
    contact->audible = audible;
    }

static void notifyDataEntityDamage(struct tank *tank, const char *id,
	const char *faction, float x, float z)
    {
    struct EntityContact source;

    initializeContact(&source, id, faction, x, z, 1, 1, 1);
    entityAgentNotifyDamage(&tank->dataAgent, &source);
    }

static void notifyDataEntityDamageFromMonster(struct tank *tank,
	struct monsterInfo *owner, float x, float z)
    {
    struct targetInfo *target;
    char id[ENTITY_NAME_SIZE];
    int number;

    if (owner == &Googelon)
	{
	notifyDataEntityDamage(tank, "player",
	    playerEntityArchetype == NULL ? "neutral" :
	    playerEntityArchetype->faction, x, z);
	return;
	}
    number = 0;
    for (target = targets->next; target != NULL; target = target->next)
	{
	if (owner == &target->monster)
	    {
	    sprintf(id, "target-%d", number);
	    notifyDataEntityDamage(tank, id, "monster", x, z);
	    return;
	    }
	number++;
	}
    notifyDataEntityDamage(tank, "environment", "neutral", x, z);
    }

static struct tank *findDataActor(struct tank *allTanks, const char *id)
    {
    struct tank *tank;

    for (tank = allTanks->next; tank != NULL; tank = tank->next)
        if (tank->type == DATAENTITY && strcmp(tank->dataId, id) == 0)
            return tank;
    return NULL;
    }

static void routeDataEntityEvents(struct tank *source,
                                  struct tank *allTanks)
    {
    const struct BattalionMapTrigger *route;
    struct tank *receiver;
    size_t eventIndex;
    size_t routeIndex;

    if (source->dataActor == NULL)
        return;
    for (eventIndex = 0; eventIndex < source->dataAgent.eventCount;
         eventIndex++)
        for (routeIndex = 0; routeIndex < source->dataActor->triggerCount;
             routeIndex++)
            {
            route = &source->dataActor->triggers[routeIndex];
            if (strcmp(route->event,
                       source->dataAgent.events[eventIndex].name) != 0)
                continue;
            for (receiver = allTanks->next; receiver != NULL;
                 receiver = receiver->next)
                {
                if (receiver->type != DATAENTITY ||
                    receiver->dataActor == NULL)
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

static size_t buildDataEntityContacts(struct tank *source,
                                      struct tank *allTanks,
                                      struct monsterInfo *player,
                                      struct targetInfo *targetList,
                                      struct EntityContact *contacts,
                                      size_t capacity)
    {
    struct targetInfo *target;
    struct tank *tank;
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
	visible = dataEntityLineOfSight(source, 0, targetY, 0);
	audible = player->monsterMoving || player->beamOn || playerFired;
	initializeContact(&contacts[count++], "player",
		  playerEntityArchetype == NULL ? "neutral" :
		  playerEntityArchetype->faction, 0, 0,
			  player->monsterIsDead != 1, visible, audible);
	}
    targetNumber = 0;
    for (target = targetList->next; target != NULL && count < capacity;
         target = target->next)
	{
	sprintf(id, "target-%d", targetNumber++);
	targetY = (target->monster.height + target->monster.bottom) * 0.5f;
	visible = dataEntityLineOfSight(source, target->x, targetY,
					target->z);
	initializeContact(&contacts[count++], id, "monster",
			  target->x, target->z,
			  target->monster.monsterIsDead != 1, visible,
			  target->monster.monsterMoving ||
			  target->monster.beamOn);
        }
    for (tank = allTanks->next; tank != NULL && count < capacity;
         tank = tank->next)
	if (tank != source && tank->type == DATAENTITY)
	    {
	    targetY = tank->y + tank->dataArchetype->collisionHeight * 0.5f;
	    visible = dataEntityLineOfSight(source, tank->x, targetY, tank->z);
	    audible = tank->walking || tank->maserCount > 0 ||
		       dataEntityRecentlyFired(tank);
	    initializeContact(&contacts[count++], tank->dataId,
			      tank->dataArchetype->faction,
			      tank->x, tank->z, tank->damage > 0,
			      visible, audible);
	    }
    return count;
    }

static void damageDataEntityTarget(struct tank *source,
                                   struct tank *allTanks,
                                   struct monsterInfo *player,
                                   struct targetInfo *targetList)
    {
    struct targetInfo *target;
    struct tank *tank;
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
            target->monster.energyRemaining -= 0.3f;
            return;
            }
        }
    tank = findDataActor(allTanks, source->dataAgent.target.id);
    if (tank != NULL)
        {
        tank->damage--;
        notifyDataEntityDamage(tank, source->dataId,
			       source->dataArchetype->faction,
			       source->x, source->z);
        }
    }

static void fireDataEntity(struct tank *tank, struct tank *allTanks,
                           struct monsterInfo *player,
                           struct targetInfo *targetList)
    {
    const struct EntityArchetype *archetype;
    float deltaX;
    float deltaZ;
    float distance;
    float velocityX;
    float velocityZ;
    float spawnX;
    float spawnZ;
    int projectileType;

    archetype = tank->dataArchetype;
    tank->maserCount = 0;
    if (!tank->dataAgent.fireRequested || !tank->dataAgent.hasTarget)
        return;
    if (archetype->weapon == ENTITY_WEAPON_BEAM)
        {
        tank->maserCount = 2;
        damageDataEntityTarget(tank, allTanks, player, targetList);
        doSound(TANKMASER);
        return;
        }
    if (archetype->weapon == ENTITY_WEAPON_FIRE)
        {
        addProjectile(tank->x, tank->y + 0.12f, tank->z, PROJFIRE,
                      0, -archetype->projectileSpeed, 0,
                      archetype->accuracy, NULL);
        return;
        }
    deltaX = tank->dataAgent.targetX - tank->x;
    deltaZ = tank->dataAgent.targetZ - tank->z;
    distance = (float)sqrt((double)(deltaX * deltaX + deltaZ * deltaZ));
    if (distance <= 0.0001f)
        return;
    velocityX = deltaX * archetype->projectileSpeed / distance;
    velocityZ = deltaZ * archetype->projectileSpeed / distance;
    if (archetype->weapon == ENTITY_WEAPON_ROCKET)
        projectileType = PROJROCKET;
    else if (archetype->weapon == ENTITY_WEAPON_PLASMA)
        projectileType = PROJTECHS;
    else
        projectileType = PROJTANK;
    spawnX = tank->x + deltaX * archetype->muzzleForward / distance;
    spawnZ = tank->z + deltaZ * archetype->muzzleForward / distance;
    addProjectile(spawnX,
                  tank->y + archetype->muzzleHeight,
                  spawnZ, projectileType,
                  velocityX, 0, velocityZ, archetype->accuracy, NULL);
    }

static void updateDataEntity(struct tank *tank, struct tank *allTanks,
                             struct monsterInfo *player,
                             struct targetInfo *targetList)
    {
    struct EntityContact contacts[MAXTANKSONPLANE + MAXTARGETS + 1];
    size_t contactCount;
    float oldX;
    float oldZ;

    oldX = tank->dataAgent.x;
    oldZ = tank->dataAgent.z;
    contactCount = buildDataEntityContacts(
        tank, allTanks, player, targetList, contacts,
        sizeof(contacts) / sizeof(contacts[0]));
    entityAgentStep(&tank->dataAgent, contacts, contactCount);
    tank->x = tank->dataAgent.x;
    tank->z = tank->dataAgent.z;
    tank->theta = tank->dataAgent.heading;
    tank->walking = oldX != tank->x || oldZ != tank->z;
    if (tank->dataAgent.hasTarget)
        tank->rad = (float)sqrt((double)(
            (tank->x - tank->dataAgent.targetX) *
            (tank->x - tank->dataAgent.targetX) +
            (tank->z - tank->dataAgent.targetZ) *
            (tank->z - tank->dataAgent.targetZ)));
    else
        tank->rad = 0;
    fireDataEntity(tank, allTanks, player, targetList);
    }

static void updateConfiguredPlayerWeapon(void)
    {
    const struct EntityArchetype *archetype;
    float xVelocity;
    float yVelocity;
    float zVelocity;
    float muzzleX;
    float muzzleY;
    float muzzleZ;
    int projectileType;

    archetype = playerEntityArchetype;
    if (archetype == NULL || archetype->weapon == ENTITY_WEAPON_NONE)
        return;
    if (archetype->weapon != ENTITY_WEAPON_BEAM &&
        archetype->weapon != ENTITY_WEAPON_FIRE)
        {
        if (playerProjectileTriggerLatched)
            return;
        playerProjectileTriggerLatched = 1;
	if (playerWeaponReloadRemaining > 0)
	    return;
        }
    if (archetype->weapon == ENTITY_WEAPON_BEAM)
        {
        updateBeam(&Googelon, targets, 0, 0, tanklist,
                   allTreesOnPlanePtrs, numTreesOnPlane, &Googelon);
        Googelon.energyRemaining -= archetype->playerFireCost;
        return;
        }
    if (archetype->weapon == ENTITY_WEAPON_FIRE)
        addProjectile(0,
                      PLANEY + archetype->verticalOffset +
                          archetype->muzzleHeight,
                      0, PROJFIRE,
                      0, -archetype->projectileSpeed, 0,
                      archetype->accuracy, &Googelon);
    else
        {
        entityAimDirection(Googelon.headHorzRotate,
                           Googelon.headVertRotate,
                           archetype->projectileSpeed,
                           &xVelocity, &yVelocity, &zVelocity);
        entityAimDirection(Googelon.headHorzRotate,
                           Googelon.headVertRotate,
                           archetype->muzzleForward,
                           &muzzleX, &muzzleY, &muzzleZ);
        if (archetype->weapon == ENTITY_WEAPON_ROCKET)
            projectileType = PROJROCKET;
        else if (archetype->weapon == ENTITY_WEAPON_PLASMA)
            projectileType = PROJTECHS;
        else
            projectileType = PROJTANK;
        addProjectile(muzzleX,
                      PLANEY + archetype->verticalOffset +
                          archetype->muzzleHeight + muzzleY,
                      muzzleZ, projectileType,
                      xVelocity, yVelocity, zVelocity,
                      0, &Googelon);
	playerWeaponReloadRemaining = archetype->fireInterval;
        }
    Googelon.energyRemaining -= archetype->playerFireCost;
    }

void updateTanks(struct tank * allTanks, float width, float height,
		float bottom, int dead, int monster,
		struct tank* slag, float accuracy, int firingDelay,
		int mainCounter, struct monsterInfo * m, struct targetInfo * targets)
    {
    float xv,  yv,  zv;
    float Vplane, angle;
    int blocked, maserOn;
    struct tank *killertank,  *ttank, *temptank;
    float energyLoss;
    float tankx,  tankz;
    int tanktype;

    float rad;
    float radToNearestTarget;
    struct targetInfo * nearestTarget;

    int aircraft;
    struct targetInfo * temptarget;
    struct monsterInfo * nearestMonster;

    float centerX,  centerZ;
    float nearAngle;


    int targetCounter, targetc;
    float targetxs[MAXTARGETS+1];
    float targetzs[MAXTARGETS+1];
    struct targetInfo * targetinfs[MAXTARGETS+1];

    /****************************************/
    /* move targets into quick access array */
    /****************************************/

    targetCounter = 0;
    for(temptarget = targets->next;temptarget != NULL;temptarget = temptarget->next)
	{
	targetxs[targetCounter]	    = temptarget->x;
	targetzs[targetCounter]	    = temptarget->z;
	targetinfs[targetCounter]   = temptarget;
	targetCounter++;
	}

    centerX	= 0;
    centerZ	= 0;

    maserOn	= 0;
    energyLoss	= 0;

    temptank	= allTanks;

    while(temptank->next != NULL)
	{
	ttank = temptank->next;
	ttank->count -= 1;

	tankx = ttank->x;
	tankz = ttank->z;
	tanktype = ttank->type;

	if (tanktype == DATAENTITY)
	    {
	    updateDataEntity(ttank, allTanks, m, targets);
	    if (!dead &&
		fabs(ttank->x) <= width && fabs(ttank->z) <= width &&
		ttank->y <= height && ttank->y >= bottom)
		{
		m->energyRemaining -= 0.75f;
		ttank->damage--;
		notifyDataEntityDamage(ttank, "player", "monster", 0, 0);
		}
	    if (ttank->damage <= 0)
		{
		addBoom(ttank->x, ttank->y, ttank->z, NULL);
		killertank = temptank->next;
		temptank->next = killertank->next;
		if (ttank->dataArchetype->destroyedModel[0] != '\0')
		    {
		    killertank->next = slag->next;
		    slag->next = killertank;
		    killertank->count = 1;
		    }
		else
		    afree(killertank, arena);
		}
	    else
		temptank = temptank->next;
	    continue;
	    }

	radToNearestTarget = 10000;
	nearestTarget = NULL;

	/************************************************/
	/* need to compute radius to the nearest target */
	/************************************************/

	for(targetc = 0; targetc < targetCounter; targetc++)
	    {
	    rad = sqrt((tankx-targetxs[targetc]) * (tankx-targetxs[targetc]) + (tankz-targetzs[targetc]) * (tankz-targetzs[targetc]));
	    if (rad < radToNearestTarget)
		{
		radToNearestTarget  = rad;
		nearestTarget	    = targetinfs[targetc];
		nearestMonster	    = &(nearestTarget->monster);
		centerX		    = nearestTarget->x;
		centerZ		    = nearestTarget->z;
		}
	    }

	/**********************************************/
	/* need to compute radius to player's monster */
	/**********************************************/

	rad = sqrt(tankx * tankx + tankz * tankz);

	if (rad < radToNearestTarget)
	    {
	    radToNearestTarget	= rad;
	    nearestTarget	= NULL;
	    nearestMonster	= m;
	    centerX		= 0;
	    centerZ		= 0;
	    }


	ttank->rad = radToNearestTarget;
	ttank->goforit = nearestMonster;


	if (tanktype == MECHAG) /* mechaG shoots 5X as fast*/
	    ttank->count -= 4;

	if (tanktype == CHH) /* 009CHH shoots 2X as fast*/
	    ttank->count -= 1;



	if (ttank->z == 0)
	    ttank->z = NEARZERO;

	angle = computeAngle(tankz, centerZ, tankx, centerX);

	/**************************/
	/* deal with the aircraft */
	/**************************/

	if ((tanktype == AIRPLANE) || (tanktype == FIGHTER))
	    aircraft = 1;
	else
	    aircraft = 0;

	if ((!aircraft) || (aircraft && (ttank->rad > (PLANESIZE*2.5))))
	    {
	    nearAngle = angle;
	    if (fabs(ttank->theta - (angle + TWOPI)) < fabs(ttank->theta - nearAngle))
		nearAngle = angle + TWOPI;
	    if (fabs(ttank->theta - (angle - TWOPI)) < fabs(ttank->theta - nearAngle))
		nearAngle = angle - TWOPI;

	    ttank->theta = 0.8 * ttank->theta + 0.2 * nearAngle;

	    if (ttank->theta < 0)
		ttank->theta += TWOPI;
	    else if (ttank->theta >= TWOPI)
		ttank->theta -= TWOPI;
	    }


	if (aircraft && (ttank->rad > (PLANESIZE*2.5)))
	    ttank->count = 0;


	if ((tanktype == MASERTANK) || (tanktype == CHH) || (tanktype == MECHAG))
	    {
	    ttank->rand1 = randy(0.025);
	    ttank->rand2 = randy(0.025);
	    ttank->rand3 = randy(0.025);
	    }

	/**********************************/
	/* move vehicle                   */
	/**********************************/

	if ((ttank->rad > ttank->finalR) || (tanktype == AIRPLANE) || (tanktype == FIGHTER))
	    {

	    /**********************************/
	    /* check for obstacles            */
	    /**********************************/

	    if ((tanktype == AIRPLANE) || (tanktype == FIGHTER))
		blocked = 0;
	    else
		blocked = checkForBlock(ttank, ttank->rad,  ttank->theta,  ttank->y);

	    if (!blocked)
		{
		Vplane = ttank->speed;

		ttank->walking += 1;

		ttank->zv = cos(ttank->theta) * Vplane;
		ttank->xv = sin(ttank->theta) * Vplane;

		ttank->x += ttank->xv;
		ttank->z += ttank->zv;
		}
	    }

	if ((!dead) && (nearestMonster->monsterIsDead != 1) && (ttank->rad <= ttank->range) && (ttank->rad >= ttank->minrange))
	{
	/**********************************/
	/* mechaGoogelon firing           */
	/**********************************/

	if ((tanktype == MECHAG))
	    {
	    if ((ttank->count <= 0) && ((rand() % 5) == 0) && (nearestMonster->monsterIsDead != 1))
		{
		xv = - (ttank->x-centerX) / 12;
		yv = ((height - bottom) * 0.5 + bottom - ttank->y) / 12;
		zv = - (ttank->z-centerZ) / 12;

		addProjectile( ttank->x, ttank->y, ttank->z, PROJROCKET,
		    xv, yv, zv, accuracy*0.33, NULL);

		ttank->count = firingDelay;
		}

	    if ((ttank->count <= 0) && (ttank->maserCount <= 0) && ((rand() % 10) == 0))
		{
		ttank->maserCount = 10 + (rand() % 5);
		ttank->count = firingDelay;
		}
	    if ((ttank->maserCount > 0) && (ttank->rad <= ttank->range))
		{
		maserOn = 1;
		ttank->maserCount -= 1;

		nearestMonster->energyRemaining -= 0.5;
		}
	    }

	/**********************************/
	/* 009 CHH firing                 */
	/**********************************/

	if ((tanktype == CHH))
	    {
	    if ((ttank->count <= 0) && ((rand() % 5) == 0))
		{
		xv = - (ttank->x-centerX) / 12;
		yv = ((height - bottom) * 0.5 + bottom - ttank->y) / 12;
		zv = - (ttank->z-centerZ) / 12;

		addProjectile( ttank->x, ttank->y, ttank->z, PROJROCKET,
		    xv, yv, zv, accuracy*0.5, NULL);

		ttank->count = firingDelay;
		}

	    if ((ttank->count <= 0) && (ttank->maserCount <= 0) && ((rand() % 10) == 0))
		{
		ttank->maserCount = 10 + (rand() % 5);
		ttank->count = firingDelay;
		}
	    if ((ttank->maserCount > 0) && (ttank->rad <= ttank->range))
		{
		maserOn = 1;
		ttank->maserCount -= 1;

		nearestMonster->energyRemaining -= 0.5;
		}
	    }

	/**********************************/
	/* tank firing shell              */
	/**********************************/

	if ((tanktype == TANK) && (ttank->count <= 0) && ((rand() % 5) == 0))
	    {
	    ttank->count = firingDelay;

	    if (monster == FLUTTER)
		Vplane = sqrt(GRAVITY * ttank->rad) * 0.8071;
	    else
		Vplane = sqrt(GRAVITY * ttank->rad) * 0.7071;

	    xv = sin(ttank->theta) * Vplane;
	    zv = cos(ttank->theta) * Vplane;

	    addProjectile( ttank->x, ttank->y + 0.15, ttank->z, PROJTANK,
		xv, Vplane, zv, accuracy, NULL);
	    }

	/************************************/
	/* airplane 'dropping' bomb         */
	/************************************/

	if ((tanktype == AIRPLANE) && (mainCounter % 2))
	    {
	    addProjectile(ttank->x, ttank->y-0.1, ttank->z, PROJTANK,  0, 0, 0,  0, NULL);
	    }

	/************************************/
	/* fighter launchine missile        */
	/************************************/

	if ((tanktype == FIGHTER) && (ttank->count <= 0))
	    {
	    ttank->count = 1000; /* way bug number */

	    xv = - (ttank->x-centerX) / 15;
	    yv = ((height - bottom) * 0.5 + bottom - ttank->y) / 15;
	    zv = - (ttank->z-centerZ) / 15;

	    addProjectile(ttank->x, ttank->y-0.1, ttank->z, PROJROCKET, xv, yv, zv, accuracy * 0.75, NULL);
	    addProjectile(ttank->x, ttank->y-0.1, ttank->z, PROJROCKET, xv, yv, zv, accuracy * 0.75, NULL);
	    addProjectile(ttank->x, ttank->y-0.1, ttank->z, PROJROCKET, xv, yv, zv, accuracy * 0.75, NULL);
	    addProjectile(ttank->x, ttank->y-0.1, ttank->z, PROJROCKET, xv, yv, zv, accuracy * 0.75, NULL);
	    }

	/**********************************/
	/* helo firing rocket             */
	/**********************************/

	if ((tanktype == HELO) && (ttank->count <= 0) && ((rand() % 5) == 0))
	    {
	    ttank->count = firingDelay;

	    xv = - (ttank->x-centerX) / 15;
	    yv = ((height - bottom) * 0.5 + bottom - ttank->y) / 15;
	    zv = - (ttank->z-centerZ) / 15;

	    addProjectile( ttank->x, ttank->y-0.15, ttank->z, PROJROCKET,
		xv, yv, zv, accuracy*2, NULL);
	    }

	/**********************************/
	/* launcher firing rocket         */
	/**********************************/

	if ((tanktype == LAUNCHER) && (ttank->count <= 0) && ((rand() % 5) == 0))
	    {
	    ttank->count = firingDelay/3;

	    yv = ((height - bottom) * 0.5 + bottom - ttank->y) / 15;

	    xv = - (ttank->x-centerX) / 15;
	    zv = - (ttank->z-centerZ) / 15;

	    addProjectile( ttank->x, ttank->y+0.2, ttank->z, PROJROCKET,
		xv, yv, zv, accuracy*0.4, NULL);
	    }

 	/**********************************/
	/* hero firing maser / shells     */
	/**********************************/

	if ( (tanktype == HERO))
	    {
	    if ((ttank->count <= 0) && (ttank->maserCount <= 0) && ((rand() % 5) == 0))
		{
		ttank->maserCount = 40;
		}

	    if (ttank->maserCount > 0)
		{
		ttank->maserCount -= 1;
		if (ttank->maserCount == 0)
		    ttank->count = firingDelay;

		if ((ttank->maserCount > 10) && (ttank->maserCount < 30) && (ttank->subtype == 1))
		    {
		    maserOn = 1;

		    nearestMonster->energyRemaining -= 0.5;
		    }

		if ((ttank->maserCount > 10) && (ttank->maserCount < 30) && (ttank->subtype == 0))
		    {
		    xv = - (ttank->x-centerX) / 15;
		    yv = ((height - bottom) *0.5 + bottom - ttank->y) / 15;
		    zv = - (ttank->z-centerZ) / 15;

		    addProjectile(ttank->x, ttank->y + 0.5, ttank->z, PROJHERO, xv, yv, zv, accuracy, NULL);
		    addProjectile(ttank->x, ttank->y + 0.5, ttank->z, PROJHERO, xv, yv, zv, accuracy, NULL);
		    }
		}
	    }

	/**********************************/
	/* tank firing maser              */
	/**********************************/

	if (tanktype == MASERTANK)
	    {
	    if (!client)
		{
		if ((ttank->count <= 0) && (ttank->maserCount <= 0) && ((rand() % 5) == 0))
		    {
		    ttank->maserCount = 20 + (rand() % 5);
		    }
		}

	    if (ttank->maserCount > 0)
		{
	    	maserOn = 1;
		ttank->maserCount -= 1;
		if (ttank->maserCount == 0)
		    ttank->count = firingDelay;

		nearestMonster->energyRemaining -= 0.3;
		}
	    }
	}

	/**********************************/
	/* tank being crushed             */
	/**********************************/

	if ((ttank->x <= (centerX+width)) && (ttank->z <= (centerZ+width)) &&
	    (ttank->x >= (centerX-width)) && (ttank->z >= (centerZ-width)) &&
	    (ttank->y <= height) && (ttank->y >= bottom) && (!dead))
	    {
	    nearestMonster->energyRemaining -= 0.75;

	    ttank->damage -= 1;

	    switch (tanktype)
		{
		case TANK:
		case LAUNCHER:
		case MASERTANK:
		case AIRPLANE:
		case FIGHTER:
		case HELO:	    addScore(tanktype, nearestMonster);
				    break;

		case MECHAG:	    addScore(PARTIALM, nearestMonster);
				    break;

		case CHH:	    addScore(PARTIALC, nearestMonster);
				    break;

		case HERO:	    addScore(PARTIALH, nearestMonster);
				    break;
		}
	    }

	/**********************************/
	/* tank is dead                   */
	/**********************************/

	if (ttank->damage <= 0)
	    {
	    addBoom(ttank->x, ttank->y, ttank->z, NULL);

	    killertank = temptank->next;
	    temptank->next = temptank->next->next;

	    switch (ttank->type)
		{
		case MECHAG:
		case CHH:
		case HERO:	    addScore(ttank->type, NULL);
		}

	    if ((ttank->damage <= -99) || (tanktype == MECHAG) || (tanktype == CHH) || (tanktype == HERO) ||
		(tanktype == AIRPLANE) || (tanktype == FIGHTER))
		{
		killertank->next = slag->next;
		slag->next = killertank;
		killertank->count = 1;
		}
	    else
		{
		afree(killertank, arena);

		}
	    }
	else
	    if (ttank->rad > 20 * PLANESIZE)
		{
		killertank = temptank->next;
		temptank->next = temptank->next->next;
		afree(killertank, arena);

		}
	else
	    temptank = temptank->next;
	}

    /* Deliver data-driven events only after every agent has completed its
       simulation step.  Receivers consume this queue on the next step, so
       trigger timing no longer depends on linked-list order. */
    for (ttank = allTanks->next; ttank != NULL; ttank = ttank->next)
	if (ttank->type == DATAENTITY)
	    routeDataEntityEvents(ttank, allTanks);

    /**********************************************************/
    /* if some vehicle is firing a maser then do maser sound  */
    /**********************************************************/

    if (maserOn)
	doSound(TANKMASER);
    else
	soundKiller(TANKMASER);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* update all the vehicles in the process of dying               */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void updateSlagTanks(struct tank * allSlags)
    {
    int i, j, k;
    float x, y, z;
    float zv,  xv;
    float Vplane;
    int dead;
    struct tank *killertank, *ttank, *temptank;
    int boom1,  boom2;

    temptank = allSlags;

    while(temptank->next != NULL)
	{
	ttank = temptank->next;

	ttank->count += 1;

	dead = 0;

	if (ttank->z == 0)
	    ttank->z = NEARZERO;

	if (((ttank->type == HELO) || (ttank->type == CHH)) && (ttank->y <= PLANEY+0.1))
	    {
	    addBoom(ttank->x, PLANEY, ttank->z, NULL);
	    dead = 1;
	    }

	if (((ttank->type == HELO) || (ttank->type == CHH) || (ttank->type == AIRPLANE) || (ttank->type == FIGHTER)) && (ttank->y > PLANEY))
	    {
	    ttank->y -= 0.04;
	    }

	if ((ttank->type == AIRPLANE) || (ttank->type == FIGHTER))
	    {
	    ttank->x += sin(ttank->theta) * TANKV;
	    ttank->z += cos(ttank->theta) * TANKV;

	    if (ttank->count > 5)
		dead = 1;
	    }


	if ((ttank->type != HELO) && (ttank->type != CHH) && (ttank->type != AIRPLANE)
	&& (ttank->type != FIGHTER) && (ttank->count >= 40))
	    {
	    dead = 1;
	    }

	x = ttank->x;
	y = ttank->y;
	z = ttank->z;

	if (ttank->count % 2)
	    {
	    switch(ttank->type){
		case MECHAG:	addBoom(x + randy(0.15), y + (rand() % 1000) / 1000.0 - 0.4,
				z + randy(0.15), NULL);
				break;
		case AIRPLANE:
		case FIGHTER:
		case CHH:	addBoom(x + randy(0.25), y + randy(0.25), z + randy(0.25), NULL);
				break;
		}
	    }


	if (((ttank->type == AIRPLANE) || (ttank->type == FIGHTER)) && (ttank->count >= 5))
	    {
	    Vplane = sqrt(GRAVITY * 1.5) * 0.7071;

		for (i=-1;i<2;i++)
		    for (j=-1;j<2;j++)
			for (k=-1;k<2;k++)
			    {
			    if ((rand() % 100) < 30)
				{
				zv = cos((rand() % 100) * 0.0628) * Vplane;
				xv = sin((rand() % 100) * 0.0628) * Vplane;

				addProjectile(x+i*0.2, y+(j*0.3), z+k*0.2, PROJTANK,
					    xv,  Vplane,  zv, 75, NULL);
				}
			    }
	    }

	if ((ttank->type == HERO) && (ttank->count >= 40))
	    {
	    Vplane = sqrt(GRAVITY * 1.5) * 0.7071;

		for (i=-1;i<2;i++)
		    for (j=-1;j<2;j++)
			for (k=-1;k<2;k++)
			    {
			    zv = cos((rand() % 100) * 0.0628) * Vplane;
			    xv = sin((rand() % 100) * 0.0628) * Vplane;

			    if (ttank->subtype == 1)
				{
				boom1 = rand() % 3;
				boom2 = rand() % 3;
				}
			    else
				{
				boom1 = PROJTANK;
				boom2 = PROJHERO;
				}

			    addProjectile(x+i*0.2, y+(j*0.3)+0.1, z+k*0.2, boom1,
					    xv,  Vplane,  zv, 75, NULL);
			    zv = cos(rand()%100*0.0628) * Vplane;
			    xv = sin(rand()%100*0.0628) * Vplane;

			    addProjectile(x+i*0.2, y+(j*0.3)+0.1, z+k*0.2, boom2,
					    xv,  Vplane,  zv, 75, NULL);
			    }
	    }

	if ((ttank->type == MECHAG) && (ttank->count >= 40))
	    {
	    Vplane = sqrt(GRAVITY * 1.5) * 1.4;

		for (i=-1;i<2;i++)
		    for (j=-1;j<2;j++)
			for (k=-1;k<2;k++)
			    {
			    zv = cos((rand() % 100) * 0.0628) * Vplane;
			    xv = sin((rand() % 100) * 0.0628) * Vplane;

			    addProjectile(x+i*0.2, y+(j*0.3)+.1, z+k*0.2, PROJTANK, xv, Vplane, zv, 75, NULL);
			    }
	    }

	if (dead)
	    {
	    killertank = temptank->next;
	    temptank->next = temptank->next->next;
	    afree(killertank, arena);
	    }
	else
	    temptank = temptank->next;
	}
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* a new military vehicle is entering the game                   */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void addNewTank(struct targetInfo * targets, float startx, float startz, int starttype, struct tank * tanklist,
		struct tree * treelist, int mainCounter, int firingDelay, struct monsterInfo * G)
    {
    float amount,amount2;
    float xabs,zabs;
    int attempts;
    int type,  side;
    int blocked;
    int howManyOfEach;
    float minX, minZ, maxX, maxZ;
    float ttankx, ttankz;
    struct targetInfo * temptarget;
    char newtype;

    struct targetInfo * nearestTarget;
    float radToNearestTarget;
    struct monsterInfo * nearestMonster;
    float centerX, centerZ;
    float rad;

    struct tank * temptank,  *ttank;
    struct tree * temptree;

    /*********************************/
    /* decide on type of vehicle     */
    /*********************************/

/* disable the adding of vehicles in networked games */

if (netUp || client)
    return;


if ((starttype != -1) && (netUp || client))
    return;

    type = rand() % 100;

if (netUp)
    {
    if (type > 50)
	newtype = MASERTANK;
    }
else
    {
    if ((type < 28) && (mainCounter  > 750))
	newtype = MASERTANK;
    else if ((type >= 28) && (type < 45))
	newtype	= HELO;
    else if ((type >= 45) && (type < 60))
	newtype	= LAUNCHER;
    else if ((type >= 60) && (type < 65) && (mainCounter  > 1000))
	newtype	= AIRPLANE;
    else if ((type >= 65) && (type < 70) && (mainCounter  > 750))
	newtype	= FIGHTER;
    else if ((type >= 90) && (type < 92) && (mainCounter  > 3000))
	newtype	= HERO;
    else if ((type >= 92) && (type < 97) && (mainCounter  > 2000))
	newtype	= CHH;
    else if ((type >= 97) && (mainCounter  > 1250))
	newtype	= MECHAG;
    else
	newtype = TANK;
    }

if ((!netUp) || (newtype == MASERTANK))
{
    /***************************************************************/
    /* ensure max of 1 bomber, 2 mechagoogelons, 2 heros on screen */
    /***************************************************************/

    howManyOfEach = 0;

    if ((newtype == AIRPLANE) || (newtype == FIGHTER) || (newtype == MECHAG) || (newtype == HERO))
	{
	for(ttank = tanklist->next; ttank != NULL; ttank = ttank->next)
	    if (ttank->type == newtype)
		howManyOfEach += 1;

	if ((((newtype == AIRPLANE) || (newtype == FIGHTER))  && (howManyOfEach >= 1))
	|| ((newtype != AIRPLANE) && (newtype != FIGHTER) && (howManyOfEach >= 2)))
	    newtype = TANK;
	}

    /************************************/
    /* choose where vehicle will appear */
    /************************************/

    maxX = 0;
    maxZ = 0;
    minX = 0;
    minZ = 0;


    for(temptarget = targets->next;temptarget != NULL;temptarget = temptarget->next)
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

	if (fabs(maxX) > fabs(minX))
	    xabs = fabs(maxX);
	else
	    xabs = fabs(minX);


	if (fabs(maxZ) > fabs(minZ))
	    zabs = fabs(maxZ);
	else
	    zabs = fabs(minZ);


    blocked = 1;
    attempts = 0;

    temptank = (struct tank *) amalloc(sizeTank, arena);

    if (temptank != NULL)
	{
	while ((blocked) && (attempts < 1000))
	    {
	    attempts += 1;
	    side = rand() % 4;
	    amount = randy(xabs);
	    amount2 = randy(zabs);

	    switch(side)
		{
		case 0: temptank->x = amount;
			temptank->z = minZ;
			break;

		case 1: temptank->z = amount2;
			temptank->x = minX;
			break;

		case 2: temptank->x = amount;
			temptank->z = maxZ;
			break;

		case 3: temptank->z = amount2;
			temptank->x = maxX;
			break;
		}

	    temptank->y = PLANEY;

	    if (newtype == HERO)
		{
		temptank->x /= (2+randy(0.5));
		temptank->z /= (2+randy(0.5));
		}

	    /***********************/
	    /* check for obstacles */
	    /***********************/

	    ttankx = temptank->x;
	    ttankz = temptank->z;

	    /************************************************/
	    /* need to compute radius to the nearest target */
	    /************************************************/

	    radToNearestTarget = 10000;
	    nearestTarget = NULL;

	    for(temptarget = targets->next; temptarget != NULL; temptarget = temptarget->next)
		{
		rad = sqrt((ttankx - temptarget->x) * (ttankx - temptarget->x) + (ttankz - temptarget->z) * (ttankz - temptarget->z));
		if (rad < radToNearestTarget)
		    {
		    radToNearestTarget  = rad;
		    nearestTarget	= temptarget;
		    nearestMonster	= &(nearestTarget->monster);
		    centerX		= nearestTarget->x;
		    centerZ		= nearestTarget->z;
		    }
		}

	    /**********************************************/
	    /* need to compute radius to player's monster */
	    /**********************************************/

	    rad = sqrt(ttankx * ttankx + ttankz * ttankz);

	    if (rad < radToNearestTarget)
		{
		radToNearestTarget  = rad;
		nearestTarget	    = NULL;
		nearestMonster	    = G;
		centerX		    = 0;
		centerZ		    = 0;
		}


	    temptank->rad = radToNearestTarget;
	    temptank->goforit = nearestMonster;

	    temptank->theta = computeAngle(ttankz, centerZ, ttankx, centerX);

	    blocked = checkForBlock(temptank, temptank->rad, temptank->theta, temptank->y);

	    if (rad > 2 * PLANESIZE)
		blocked = 1;
	}

	temptank->goforit = NULL;


	/**************************************/
	/* set up specific vehicle attributes */
	/**************************************/

	if (attempts >= 1500)
	    {
	    showError("Nowhere to put a new vehicle");
	    afree(temptank, arena);
	    }
	else
	    {
	    temptank->finalR = (rand() % 1000) * 0.002 + 1.0;
	    temptank->type = newtype;

	    if (starttype != -1)
		{
		temptank->type = (char) starttype;
		temptank->x = startx;
		temptank->z = startz;
		}

	    temptank->count = firingDelay;
	    temptank->maserCount = 0;
	    temptank->walking = 0;

	    temptank->number = currentTankNumber++;

	    temptank->subtype = (char) (rand() % 2);

	    switch (temptank->type)
		{
		case TANK:	    temptank->range	= 6;
				    temptank->damage	= 1;
				    temptank->minrange	= 0.5;
				    temptank->speed	= TANKV;
				    break;

		case LAUNCHER:	    temptank->range	= 4;
				    temptank->damage	= 1;
				    temptank->minrange	= 0.5;
				    temptank->speed	= 0.8 * TANKV;
				    temptank->count	= firingDelay/3;
				    break;

		case MASERTANK:	    temptank->range	= 3;
				    temptank->damage	= 1;
				    temptank->minrange	= 0.5;
				    temptank->speed	= 1.2 * TANKV;
				    break;

		case HELO:	    temptank->range	= 4;
				    temptank->y		= PLANEY + 1.0 + randy(0.2);
				    temptank->damage	= 1;
				    temptank->minrange	= 0.5;
				    temptank->speed	= 1.5 * TANKV;
				    break;

		case MECHAG:	    temptank->range	= 4.5;
				    temptank->y		= PLANEY +0.4;
				    temptank->damage	= 50;
				    temptank->minrange	= 0.5;
				    temptank->speed	= TANKV;
				    break;

		case CHH:	    temptank->range	= 4.5;
				    temptank->y		= PLANEY + 1.0 + randy(0.2);
				    temptank->damage	= 20;
				    temptank->minrange	= 0.5;
				    temptank->speed	= 1.2 * TANKV;
				    break;

		case HERO:	    temptank->range	= 4;
				    temptank->y		= PLANEY +0.58;
				    temptank->damage	= 100;
				    temptank->minrange	= 0.5;
				    temptank->speed	= TANKV;

				    temptree = (struct tree *) amalloc(sizeof(struct tree), arena);
				    if (temptree != NULL)
					{
					temptree->x		= temptank->x;
					temptree->z		= temptank->z;
					temptree->type		= 1;
					temptree->treeshape	= 33;
					temptree->y		= PLANEY;
					temptree->deathCount	= 0;
					temptree->death2	= -1;
					temptree->intact	= 1;
					temptree->height	= buildingHeight(temptree->type, temptree->treeshape);

					temptree->next		= treelist->next;
					treelist->next		= temptree;
					}
				    break;

		case AIRPLANE:	    temptank->range	= 1.5;
				    temptank->y		= PLANEY +2.5 + randy(0.25);
				    temptank->damage	= 1;
				    temptank->minrange	= 0.0;
				    temptank->speed	= 4 * TANKV;
				    temptank->finalR	= -1;
				    break;

		case FIGHTER:	    temptank->range	= 5;
				    temptank->y		= PLANEY + 2.0 + randy(0.2);
				    temptank->damage	= 1;
				    temptank->minrange	= 4.5;
				    temptank->speed	= 6 * TANKV;
				    temptank->finalR	= -1;
				    temptank->count = 0;
				    break;

		default:	showError("Bogus Vehicle Type! (addNewTank)");
				break;
		}

	    temptank->next = tanklist->next;
	    tanklist->next = temptank;
	    }
	}
}
    }

static int addConfiguredEntity(
    const struct EntityArchetype *archetype,
    const char *id, const char *group,
    float x, float z, float heading, int active,
    const struct EntityWaypoint *waypoints, size_t waypointCount,
    int patrolMode, const struct BattalionMapActor *actor,
    struct tank *allTanks)
    {
    struct tank *tank;

    if (archetype == NULL)
        return 0;
    tank = (struct tank *)amalloc(sizeTank, arena);
    if (tank == NULL)
        return 0;
    memset(tank, 0, sizeof(*tank));
    tank->x = x;
    tank->z = z;
    tank->y = PLANEY + archetype->verticalOffset;
    tank->theta = heading;
    tank->range = archetype->weaponRange;
    tank->minrange = archetype->weaponMinRange;
    tank->finalR = archetype->weaponRange;
    tank->speed = archetype->speed;
    tank->damage = archetype->health;
    tank->number = currentTankNumber++;
    tank->type = DATAENTITY;
    tank->dataArchetype = archetype;
    tank->dataActor = actor;
    strcpy(tank->dataId, id);
    strcpy(tank->dataGroup, group);
    entityAgentInitialize(&tank->dataAgent, archetype,
                          x, z, heading, waypoints, waypointCount,
                          patrolMode);
    tank->dataAgent.active = active;
    tank->next = allTanks->next;
    allTanks->next = tank;
    return 1;
    }

static int addMapActor(const struct BattalionMapActor *actor,
                       struct tank *allTanks)
    {
    const struct EntityArchetype *archetype;

    archetype = entityCatalogFind(&gameEntities, actor->archetype);
    return addConfiguredEntity(
        archetype, actor->id, actor->group,
        actor->x, actor->z, actor->heading, actor->active,
        actor->waypoints, actor->waypointCount, actor->patrolMode,
        actor, allTanks);
    }

static int configuredEntityCount(struct tank *allTanks,
                                 const struct EntityArchetype *archetype)
    {
    struct tank *tank;
    int count;

    count = 0;
    for (tank = allTanks->next; tank != NULL; tank = tank->next)
        if (tank->type == DATAENTITY && tank->dataArchetype == archetype)
            count++;
    return count;
    }

static const struct EntityArchetype *selectSpawnArchetype(
    struct tank *allTanks, int counter)
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
             configuredEntityCount(allTanks, archetype) <
                 archetype->maxActive))
            totalWeight += archetype->spawnWeight;
        }
    if (totalWeight == 0 || rand() % (totalWeight + 100) >= totalWeight)
        return NULL;
    selected = rand() % totalWeight;
    for (index = 0; index < gameEntities.count; index++)
        {
        archetype = &gameEntities.entries[index];
        if (archetype->spawnWeight <= 0 || counter < archetype->spawnAfter ||
            (archetype->maxActive > 0 &&
             configuredEntityCount(allTanks, archetype) >=
                 archetype->maxActive))
            continue;
        if (selected < archetype->spawnWeight)
            return archetype;
        selected -= archetype->spawnWeight;
        }
    return NULL;
    }

static int configuredSpawnBlocked(float x, float z, float radius,
                                  struct tank *allTanks)
    {
    struct tree *tree;
    struct tank *tank;
    float deltaX;
    float deltaZ;
    float otherRadius;

    for (tree = treelist->next; tree != NULL; tree = tree->next)
        if (tree->intact)
            {
            deltaX = tree->x - x;
            deltaZ = tree->z - z;
            if (deltaX * deltaX + deltaZ * deltaZ <
                (radius + 0.5f) * (radius + 0.5f))
                return 1;
            }
    for (tank = allTanks->next; tank != NULL; tank = tank->next)
        {
        otherRadius = tank->type == DATAENTITY ?
            tank->dataArchetype->collisionRadius : 0.3f;
        deltaX = tank->x - x;
        deltaZ = tank->z - z;
        if (deltaX * deltaX + deltaZ * deltaZ <
            (radius + otherRadius) * (radius + otherRadius))
            return 1;
        }
    return 0;
    }

static int addRandomConfiguredEntity(struct targetInfo *targetList,
                                     struct tank *allTanks, int counter)
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
    int side;
    int attempt;
    char id[ENTITY_NAME_SIZE];

    archetype = selectSpawnArchetype(allTanks, counter);
    if (archetype == NULL)
        return 0;
    minX = minZ = -(PLANESIZE + 1);
    maxX = maxZ = PLANESIZE + 1;
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
        if (!configuredSpawnBlocked(x, z, archetype->collisionRadius,
                                    allTanks))
            {
            heading = (float)atan2((double)-x, (double)-z);
            sprintf(id, "spawn-%d", currentTankNumber);
            return addConfiguredEntity(
                archetype, id, "", x, z, heading, 1,
                NULL, 0, ENTITY_PATROL_LOOP, NULL, allTanks);
            }
        }
    return 0;
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* update all of the explosions                                  */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void updateBooms(struct boom * allBooms, struct tank * tanklist)
    {
    struct boom * bboom,  * killerb, *tempboom;
    struct tank *temptank;
    int tanktype;

    tempboom = allBooms;

    while (tempboom->next != NULL)
	{
	bboom = tempboom->next;
	bboom->count += 1;

	/***********************/
	/* make some noise     */
	/***********************/

	if ((bboom->count == 2))
	    doSound(EXPLOSION);

	/**********************************/
	/* check for another tank hit     */
	/**********************************/

	if ((bboom->count == 4))
	    {
	    for(temptank = tanklist; temptank->next != NULL; temptank = temptank->next)
		if (tankHit(temptank->next, bboom->x, bboom->y, bboom->z))
		    {
		    tanktype		    = temptank->next->type;
		    temptank->next->damage -= 1;
		    if (tanktype == DATAENTITY)
			notifyDataEntityDamageFromMonster(temptank->next,
				bboom->mine, bboom->x, bboom->z);
		    switch (tanktype)
			{
			case TANK:
			case LAUNCHER:
			case MASERTANK:
			case AIRPLANE:
			case FIGHTER:
			case HELO:	    addScore(tanktype, bboom->mine);
					    break;

			case MECHAG:	    addScore(PARTIALM, bboom->mine);
					    break;

			case CHH:	    addScore(PARTIALC, bboom->mine);
					    break;

			case HERO:	    addScore(PARTIALH, bboom->mine);
					    break;

			case DATAENTITY: if (bboom->mine != NULL)
						bboom->mine->monsterScore +=
						    temptank->next->dataArchetype->score;
					    break;
			}
		    }
	    }

	/*******************/
	/* remove boom     */
	/*******************/

	if (bboom->count >= 7)
	    {
	    killerb = tempboom->next;
	    tempboom->next = tempboom->next->next;
	    afree(killerb, arena);
	    }
	else
	    tempboom = tempboom->next;
	}
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void buildHillPart(struct tree unitHill, int shape, float x, float z)
    {
    struct tree * temptree;

    temptree = (struct tree *) amalloc(sizeof(struct tree), arena);

    if (temptree != NULL)
	{
	*temptree = unitHill;
	temptree->treeshape = shape;
	temptree->x	    = x;
	temptree->z	    = z;
	temptree->height    = buildingHeight(temptree->type, temptree->treeshape);
	temptree->number    = treeID++;

	temptree->next	    = treelist->next;
	treelist->next	    = temptree;
	}
    }


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* reset everything for a new game                               */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void setPlayConditions()
    {
    struct boom * tempboom,  *tempboom2;
    struct tank * ttank,  *temptank;
    struct tree * ttree,  *temptree;
    struct projectile * killer,  *temp;
    struct targetInfo * ttarget, *temptarg;

    float x, z;
    int type, shape;
    int buildingNumber;
    size_t mapRecord;

    struct tree unitHill;

    unitHill.type	    = 3;
    unitHill.y		    = PLANEY;
    unitHill.deathCount	    = 0;
    unitHill.death2	    = -1;
    unitHill.intact	    = 1;

    moreThanOne = 0;

    setUnconnected();

    /******************************/
    /* reset all the linked lists */
    /******************************/

    temptank = tanklist;
    while(temptank != NULL)
	    {
	    ttank = temptank;
	    temptank= temptank->next;
	    afree(ttank, arena);
	    }

    tempboom = projectboom;
    while (tempboom != NULL)
	    {
	    tempboom2 = tempboom;
	    tempboom = tempboom->next;
	    afree(tempboom2, arena);
	    }

    temp = projectFlight;
    while (temp != NULL)
	    {
	    killer = temp;
	    temp = temp->next;
	    afree(killer, arena);
	    }

    temptank = slaglist;
    while(temptank != NULL)
	    {
	    ttank = temptank;
	    temptank = temptank->next;
	    afree(ttank, arena);
	    }

    temptree = treelist;
    while(temptree != NULL)
	    {
	    ttree = temptree;
	    temptree = temptree->next;
	    afree(ttree, arena);
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

    projectFlight = (struct projectile *) amalloc(sizeProjectile, arena);
    projectFlight->next = NULL;

    projectboom = (struct boom *) amalloc(sizeBoom, arena);
    projectboom->next = NULL;

    tanklist = (struct tank *) amalloc(sizeTank, arena);
    tanklist->next = NULL;

    slaglist = (struct tank *) amalloc(sizeTank, arena);
    slaglist->next = NULL;

    treelist = (struct tree *) amalloc(sizeof(struct tree), arena);
    treelist->next = NULL;

    targets = (struct targetInfo *) amalloc(sizeof(struct targetInfo), arena);
    targets->next = NULL;

    /************************************************************/
    /* read in data file of structures (trees, buildings, etc.) */
    /************************************************************/

    treeID = 0;
    mapRecord = 0;
    for (;;)
	    {
	    if (mapRecord >= battlefieldMap.structureCount)
		break;
	    x = battlefieldMap.structures[mapRecord].x;
	    z = battlefieldMap.structures[mapRecord].z;
	    type = battlefieldMap.structures[mapRecord].type;
	    shape = battlefieldMap.structures[mapRecord].shape;
	    mapRecord++;

	    temptree = (struct tree *) amalloc(sizeof(struct tree), arena);
	    if (temptree != NULL)
		{
		temptree->x		    = x;
		temptree->z		    = z;
		temptree->type		    = (char) type;
		temptree->treeshape	    = (char) shape;
		temptree->y		    = PLANEY;
		temptree->deathCount	    = 0;
		temptree->death2	    = -1;
		temptree->intact	    = 1;
		temptree->height	    = buildingHeight(temptree->type, temptree->treeshape);
		temptree->number	    = treeID++;

		if ((type == 1) && (shape == 31))
		    addNewTank(targets, x, z, 1, tanklist, treelist, mainCounter, firingDelay, &Googelon);
		if ((type == 1) && (shape == 32))
		    addNewTank(targets, x, z, 2, tanklist, treelist, mainCounter, firingDelay, &Googelon);

		if ((type == 3) && (shape == 13))
		    {
		    /* create all the parts of the unit-hill */

		    buildHillPart(unitHill, 0, x,     z);
		    buildHillPart(unitHill, 2, x-0.6, z);
		    buildHillPart(unitHill, 1, x,     z+0.6);
		    buildHillPart(unitHill, 4, x+0.6, z);
		    buildHillPart(unitHill, 3, x,     z-0.6);

		    buildHillPart(unitHill, 7, x-0.6, z-0.6);
		    buildHillPart(unitHill, 5, x+0.6, z+0.6);
		    buildHillPart(unitHill, 8, x+0.6, z-0.6);

		    *temptree = unitHill;
		    temptree->treeshape	    = 6;
		    temptree->x		    = x-0.6;
		    temptree->z		    = z+0.6;
		    temptree->height	    = buildingHeight(temptree->type, temptree->treeshape);
		    }

		temptree->next	    = treelist->next;
		treelist->next	    = temptree;
		}
	    }

	/**************************************************************************/
	/* eliminate all trees that have been destroyed already in a network game */
	/**************************************************************************/

	if (netUp)
	    for (temptree = treelist->next; temptree != NULL; temptree = temptree->next)
		for(buildingNumber=0; buildingNumber< buildingBoomSoFarCounter; buildingNumber++)
		    if (temptree->number == buildingBoomSoFar[buildingNumber])
			temptree->intact = 0;

    /***********************************************/
    /* read in data file of prepositioned vehicles */
    /***********************************************/

    if (!netUp && !client)
	{
	    mapRecord = 0;
	    for (;;)
		{
		if (mapRecord >= battlefieldMap.unitCount)
		    break;
		x = battlefieldMap.units[mapRecord].x;
		z = battlefieldMap.units[mapRecord].z;
		type = battlefieldMap.units[mapRecord].type;
		mapRecord++;
		addNewTank(targets, x, z, type, tanklist, treelist,
			   mainCounter, firingDelay, &Googelon);
		}

	for (mapRecord = 0; mapRecord < battlefieldMap.actorCount;
	     mapRecord++)
	    if (!addMapActor(&battlefieldMap.actors[mapRecord], tanklist))
		showError("Could not create a data-driven map actor");
	}

    /*********************************/
    /* reset general game parmeters  */
    /*********************************/

    accuracy		    = STARTACCURACY;
    arrivalTime		    = ARRIVALRATE;
    firingDelay		    = STARTFIRINGDELAY;
    mainCounter		    = 0;
    playerWeaponReloadRemaining = 0;

    killtanks = killmtanks = killhelos = 0;
    killCHHs = killheros = killmechags = 0;
    killlaunchers = killfighters = killplanes = 0;

    xrot = xsaverot	    = 300;
    yrot = ysaverot	    = 0;

    globalxshift	    = 0;
    globalzshift	    = 0;

    if (!netUp && !client)
	buildingBoomSoFarCounter = 0;

    Googelon = resetMonsterParameters(Googelon);
    playerBodyHeading = Googelon.headHorzRotate;


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
    addRandomConfiguredEntity(targets, tanklist, mainCounter);
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
    float garb1, garb2;
    int i, garb3;
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

    buildingBoomSoFarCounter	    = 0;
    buildingBoomThisFrameCounter    = 0;
    currentTankNumber		    = 1;
    biggestTankNumberSent	    = 0;

    totalCounter		    = 0;

    /************************************/
    /* set up memory arena              */
    /************************************/

#ifdef SGIVERSION

    sharedmem = calloc(1, ARENASIZE);
    arena = acreate(sharedmem, ARENASIZE, 0, NULL, NULL);
    amallopt(M_FREEHD, 1, arena);
    amallopt(M_CLRONFREE, 0, arena);
    amallopt(M_MXFAST, 16, arena);

#endif

    sizeBoom	    = sizeof(struct boom);
    sizeTank	    = sizeof(struct tank);
    sizeProjectile  = sizeof(struct projectile);

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


    monsterLookat =  makeMonsterLookat();
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
    doBigClear	    = 0;
    showOptions	    = 0;
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

    projectFlight   = NULL;
    projectboom	    = NULL;
    tanklist	    = NULL;
    slaglist	    = NULL;
    treelist	    = NULL;

    roadSystem	    = NULL;

    initSounds();

    /************************************/
    /* get the current date/time        */
    /************************************/

    beg = time(NULL);
    theTime = localtime(&beg);

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

    glGetIntegerv(GL_STEREO, &gdtmp);
    if (gdtmp == GL_FALSE)
	no3D = 1;


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

    /************************************/
    /* read in data file of roads       */
    /************************************/

    garb3 = (int)battlefieldMap.roadCount;
    roadSystem = (struct road *) acalloc(garb3 + 1,
				       sizeof(struct road), arena);
    if (roadSystem == NULL)
	showError("Could not allocate array for map roads\n");
    else
	{
	roadSystem[garb3].type = NULLROAD;
	for (i = garb3 - 1; i >= 0; i--)
	    {
	    roadSystem[i].x = battlefieldMap.roads[garb3 - 1 - i].x;
	    roadSystem[i].y = battlefieldMap.roads[garb3 - 1 - i].z;
	    roadSystem[i].type =
		(char)battlefieldMap.roads[garb3 - 1 - i].type;
	    }
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
	    for(i=0;i<12;i++)
		fprintf(hiScoreFile, "-1 -\n");

	    G[0].number = G[1].number = G[2].number = -1;
	    F[0].number = F[1].number = F[2].number = -1;
	    V[0].number = V[1].number = V[2].number = -1;
	    T[0].number = T[1].number = T[2].number = -1;

	    G[0].name[0] = G[1].name[0] = G[2].name[0] = 0;
	    F[0].name[0] = F[1].name[0] = F[2].name[0] = 0;
	    V[0].name[0] = V[1].name[0] = V[2].name[0] = 0;
	    T[0].name[0] = T[1].name[0] = T[2].name[0] = 0;

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
	fscanf(hiScoreFile, "%d%c%255s", &(G[0].number), &garbage, G[0].name);
	fscanf(hiScoreFile, "%d%c%255s", &(G[1].number), &garbage, G[1].name);
	fscanf(hiScoreFile, "%d%c%255s", &(G[2].number), &garbage, G[2].name);

	fscanf(hiScoreFile, "%d%c%255s", &(V[0].number), &garbage, V[0].name);
	fscanf(hiScoreFile, "%d%c%255s", &(V[1].number), &garbage, V[1].name);
	fscanf(hiScoreFile, "%d%c%255s", &(V[2].number), &garbage, V[2].name);

	fscanf(hiScoreFile, "%d%c%255s", &(F[0].number), &garbage, F[0].name);
	fscanf(hiScoreFile, "%d%c%255s", &(F[1].number), &garbage, F[1].name);
	fscanf(hiScoreFile, "%d%c%255s", &(F[2].number), &garbage, F[2].name);

	fscanf(hiScoreFile, "%d%c%255s", &(T[0].number), &garbage, T[0].name);
	fscanf(hiScoreFile, "%d%c%255s", &(T[1].number), &garbage, T[1].name);
	fscanf(hiScoreFile, "%d%c%255s", &(T[2].number), &garbage, T[2].name);

	fclose(hiScoreFile);
	}


    Googelon.monster = DATAENTITY;
    Googelon.dataArchetype = playerEntityArchetype;

    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* go to 3d-video mode                                           */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void goto3d(void)
    {
    GLint gdtmp;

	/* OGLXXX
	 * getsize not supported -- See Window Manager
	 * getsize(&windowWide,  &windowTall);
	 */

	/* OGLXXX
	 * getorigin not supported -- See Window Manager
	 * getorigin(&windowLeft,  &windowTop);
	 */

	/* OGLXXX
	 * getgdesc other posiblilties:
	 * 	glxGetConfig();
	 * 	glxGetCurrentContext();
	 * 	glxGetCurrentDrawable();
	 * GLint gdtmp;
	 */
    if ((glGetIntegerv(GL_STEREO, &gdtmp), gdtmp))
	    {
	/* OGLXXX
	 * setmonitor not supported
	 * setmonitor(STR_RECT)
	 */

	/* OGLXXX
	 * getgdesc other posiblilties:
	 * 	glxGetConfig();
	 * 	glxGetCurrentContext();
	 * 	glxGetCurrentDrawable();
	 * GLint gdtmp;
	 * getgdesc other posiblilties:
	 * 	glxGetConfig();
	 * 	glxGetCurrentContext();
	 * 	glxGetCurrentDrawable();
	 * GLint gdtmp;
	 * see window manager
	 * see window manager
	 * prefposition not supported -- See Window Manager
	 * prefposition(0, (glGetIntegerv(XXX_XPMAX, &gdtmp), gdtmp), 0, (glGetIntegerv(XXX_YPMAX, &gdtmp), gdtmp))
	 */

	    }
	/* OGLXXX
	 * winconstraints not supported -- See Window Manager
	 * winconstraints()
	 */
	/* OGLXXX
	 * setvaluator not supported -- See Events
	 * setvaluator(MOUSEY, YMAXSTEREO/2, 0, YMAXSTEREO)
	 */

    mode3D = 1;
    doBigClear = 2;
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* go to 1d-video mode                                           */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void goto1d(void)
    {
    GLint gdtmp;

	/* OGLXXX
	 * getgdesc other posiblilties:
	 * 	glxGetConfig();
	 * 	glxGetCurrentContext();
	 * 	glxGetCurrentDrawable();
	 * GLint gdtmp;
	 */
	if ((glGetIntegerv(GL_STEREO, &gdtmp), gdtmp))
	/* OGLXXX
	 * setmonitor not supported
	 * setmonitor((short) monitorType)
	 */

	/* OGLXXX
	 * winposition not supported -- See Window Manager
	 * winposition(windowLeft, windowLeft + windowWide, windowTop, windowTop + windowTall)
	 */

	/* OGLXXX
	 * keepaspect not supported -- See Window Manager
	 * keepaspect(1280, 1000)
	 */

	/* OGLXXX
	 * winconstraints not supported -- See Window Manager
	 * winconstraints()
	 */

	/* OGLXXX
	 * getgdesc other posiblilties:
	 * 	glxGetConfig();
	 * 	glxGetCurrentContext();
	 * 	glxGetCurrentDrawable();
	 * GLint gdtmp;
	 * see window manager
	 * getvaluator not supported -- See Events
	 * getvaluator(MOUSEY)
	 * setvaluator not supported -- See Events
	 * setvaluator(MOUSEY, (short) ???, 0, (short) (glGetIntegerv(XXX_YPMAX, &gdtmp), gdtmp))
	 */

 	mode3D = 0;
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
    Googelon.beamOn = (mode == PLAYMODE) && !paused &&
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

    Googelon.monsterGo = Googelon.monsterBack = 0;
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
	    Googelon.monsterMoving = 1;

	Googelon.headHorzRotate += (int) horizontalAim;
	Googelon.headVertRotate += (int) verticalAim;
	if (Googelon.monster == DATAENTITY && playerEntityArchetype != NULL)
	    {
	    pitchMinimum = (int)(playerEntityArchetype->aimPitchMin * 10.0f);
	    pitchMaximum = (int)(playerEntityArchetype->aimPitchMax * 10.0f);
	    if (Googelon.headVertRotate < pitchMinimum)
		Googelon.headVertRotate = pitchMinimum;
	    if (Googelon.headVertRotate > pitchMaximum)
		Googelon.headVertRotate = pitchMaximum;
	    }

	if (Googelon.xspeed != 0)
	    offsetX = -horizontalAim / Googelon.xspeed;
	if (offsetX > 1)
	    offsetX = 1;
	else if (offsetX < -1)
	    offsetX = -1;

	forward = playerInput.forward - playerInput.backward;
	sideways = playerInput.right - playerInput.left;
	if (Googelon.monster == DATAENTITY &&
	    playerEntityArchetype != NULL &&
	    playerEntityArchetype->controlMode == ENTITY_CONTROL_STEERED)
	    {
	    steering = playerInput.left - playerInput.right;
	    if (steering != 0)
		{
		steeringDelta = entitySteeredHeadingDelta(
		    playerInput.left, playerInput.right,
		    playerEntityArchetype->playerSteerRate);
		playerBodyHeading += steeringDelta;
		Googelon.headHorzRotate += steeringDelta;
		Googelon.monsterMoving = 1;
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
		Googelon.monsterMoving = 1;
		if (forward < 0)
		    Googelon.monsterBack = 1;
		else
		    Googelon.monsterGo = 1;
		}
	    }
	else
	    {
	    if ((view == MONSTERVIEW) || (view == FIRSTPERSONVIEW))
		sideways = -sideways;
	    if (forward || sideways)
		{
		playerMoveAngle = Googelon.headHorzRotate * BIG_DEG_TO_RAD +
		    atan2((double) sideways, (double) forward);
		playerMoveActive = 1;
		Googelon.monsterMoving = 1;
		if ((forward < 0) && !sideways)
		    Googelon.monsterBack = 1;
		else
		    Googelon.monsterGo = 1;
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
    Googelon.monsterGo = Googelon.monsterBack = 0;
    Googelon.beamOn = 0;
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
	Googelon.monster = DATAENTITY;
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
			    Googelon.monster = DATAENTITY;
			    setPlayConditions();
			    clearPlayerInput();
			    setPointerCaptured(1);
			    }
			break;

	case TK_ESCAPE: showOptions = 0;
			break;

	}
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void playKeys(int key)
    {
    switch(key)
	{
	case TK_1:	view = MONSTERVIEW;
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
/* deal with a dead monster (not necessarily your monster)       */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void updateDeadMonster(float x, float z, struct monsterInfo * thaMonster)
    {
    float Vplane;
    int i, j, k;

    thaMonster->monsterIsDead = 1;

    if ((x==0) && (z==0)) /* its the player's monster that dies */
	view = OMNISCIENTVIEW;

    if (thaMonster->energyRemaining < 0)
	thaMonster->energyRemaining = 0;

    /************************/
    /* update dying monster */
    /************************/

    thaMonster->deadCount += 1;

    if (thaMonster->deadCount > 45)
	{
	thaMonster->deadCount = 45;
	thaMonster->timeDead += 1;
	}

    if (thaMonster->deadCount == 44)
	{
	if (thaMonster->monster != VAPOUR)
	    {
	    doSound(CRASH);
	    }

	if (thaMonster->monster == FLUTTER)
	    {
	    addProjectile( x-0.2,  PLANEY+0.25,  z+0.2, 3, 0,  -0.001,  0, 10000, NULL);
	    addProjectile( x+0.2,  PLANEY+0.25,  z+0.2, 3, 0,  -0.001,  0, 10000, NULL);
	    addProjectile( x    ,  PLANEY+0.25,  z,     3, 0,  -0.001,  0, 10000, NULL);
	    addProjectile( x-0.2,  PLANEY+0.25,  z-0.2, 3, 0,  -0.001,  0, 10000, NULL);
	    addProjectile( x+0.2,  PLANEY+0.25,  z-0.2, 3, 0,  -0.001,  0, 10000, NULL);
	    }

	if (thaMonster->monster == TECHS)
	    {
	    addProjectile( x-0.2,  PLANEY+0.2,  z+0.2, 3, 0,  -0.001,  0, 10000, NULL);
	    addProjectile( x+0.2,  PLANEY+0.2,  z+0.2, 3, 0,  -0.001,  0, 10000, NULL);
	    addProjectile( x    ,  PLANEY+0.2,  z,     3, 0,  -0.001,  0, 10000, NULL);
	    addProjectile( x-0.2,  PLANEY+0.2,  z-0.2, 3, 0,  -0.001,  0, 10000, NULL);
	    addProjectile( x+0.2,  PLANEY+0.2,  z-0.2, 3, 0,  -0.001,  0, 10000, NULL);

	    Vplane = sqrt(GRAVITY * 1.5) * 0.7071;

		for (i=-1;i<2;i++)
		    for (j=0;j<3;j++)
			for (k=-1;k<2;k++)
				if ((rand() % 100) < 75)
				    {
				addProjectile( x+i*.35, PLANEY+(j*.35)+.1, z+k*.35, 0,
				    sin(randy(PI)) * Vplane, Vplane, cos(randy(PI)) * Vplane, 75, NULL);
				    }
	    }
	}
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* update all of the other monsters currently in the game        */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void updateTargets()
    {
    struct tree * temptree;
    struct tank * temptank;
    struct targetInfo * temptarget;
    struct targetInfo * temptarget2;
    struct monsterInfo  thaMonster;
    struct tree ttree;
    float mAngle, obsangle;
    float mRad, mAng;
    float tx, tz;
    float trad, rad;
    int blocked, nearAngle;

    for(temptarget = targets->next; temptarget != NULL; temptarget = temptarget->next)
	{
	thaMonster  = temptarget->monster;
	tx	    = temptarget->x;
	tz	    = temptarget->z;

    /************************************/
    /* deal with the networked monsters */
    /************************************/

    if (temptarget->net_clientFrom != COMPUTER_MONSTER)
	{
	thaMonster.energyRemaining	= temptarget->net_energyRemaining;
	thaMonster.beamOn		= temptarget->net_beamOn;

	nearAngle = temptarget->net_headHorzRotate;
	if (fabs(thaMonster.headHorzRotate - (temptarget->net_headHorzRotate + 3600)) < fabs(thaMonster.headHorzRotate - nearAngle))
	    nearAngle = temptarget->net_headHorzRotate + 3600;
	if (fabs(thaMonster.headHorzRotate - (temptarget->net_headHorzRotate - 3600)) < fabs(thaMonster.headHorzRotate - nearAngle))
	    nearAngle = temptarget->net_headHorzRotate - 3600;

	thaMonster.headHorzRotate	= 0.7 * thaMonster.headHorzRotate + 0.3 * nearAngle;

	thaMonster.headVertRotate	= 0.9 * thaMonster.headVertRotate + 0.1 * temptarget->net_headVertRotate;
	temptarget->x			= 0.9 * temptarget->x + 0.1 * temptarget->net_x;
	temptarget->z			= 0.9 * temptarget->z + 0.1 * temptarget->net_z;

	if (thaMonster.energyRemaining > 0)
	    {
	    thaMonster.rot1 = cos(thaMonster.moveCount) * 0.02;
	    thaMonster.rot2 = sin(thaMonster.moveCount) * 0.05;
	    }
	else
	    {
	    updateDeadMonster(temptarget->x, temptarget->z, &thaMonster);
	    }

	thaMonster.monsterMoving = 1;
	thaMonster.moveCount += 1;

	/************************************************/
	/* update the weapons of the networked monsters */
	/************************************************/

	if (thaMonster.beamOn)
	    switch (thaMonster.monster){
		case GOOGELON:	    updateBeam(&Googelon, targets, temptarget->x, temptarget->z, tanklist, allTreesEverywherePtrs,
					    numTreesEverywhere, &thaMonster);

				    thaMonster.energyRemaining -= 0.15;
				    break;

		case TECHS:	    updateGun(temptarget->x, temptarget->z, thaMonster.headHorzRotate,
					thaMonster.headVertRotate, mainCounter, &(temptarget->monster));
				    thaMonster.energyRemaining -= 0.09;
				    break;

		case VAPOUR:	    updateVap(temptarget->x, temptarget->z, mainCounter, &(temptarget->monster));
				    thaMonster.energyRemaining -= 0.1;
				    break;

		case FLUTTER:	    updateBeam(&Googelon, targets, temptarget->x, temptarget->z, tanklist, allTreesEverywherePtrs,
					    numTreesEverywhere, &thaMonster);

				    thaMonster.energyRemaining -= 0.15;
				    break;

		default:	    showError("Bogus Monster! (Update Targets)");
				    break;
	    }

	temptarget->monster = thaMonster;
	}
    else
	{
	/**********************************************/
	/* deal with the computer controlled monsters */
	/**********************************************/

	if (thaMonster.monster == FLUTTER)
	    {
	    mRad = 1.2;
	    mAng = 1.0;
	    }
	else
	    {
	    mRad = 0.8;
	    mAng = 0.8;
	    }

	if (((thaMonster.monsterGo || thaMonster.monsterBack)) && (!thaMonster.monsterIsDead))
	    {
	    thaMonster.monsterMoving = 1;

	    thaMonster.energyRemaining -= thaMonster.moveCost; /* takes energy to move, not to turn */

	    if (thaMonster.monsterGo)
		mAngle = (thaMonster.headHorzRotate * BIG_DEG_TO_RAD);
	    if (thaMonster.monsterBack)
		mAngle = (thaMonster.headHorzRotate + 1800) * BIG_DEG_TO_RAD;



	    blocked = 0;

	    if (monsterHasSolidCollision(&thaMonster))
		{
		/****************************************************/
		/* cant move through trees until they are destroyed */
		/****************************************************/

		temptree = treelist->next;
		while ((temptree != NULL) && (!blocked))
		    {
		    ttree = *temptree;

		    if (ttree.intact)
			trad = sqrt((ttree.x - tx) * (ttree.x - tx) + (ttree.z - tz) * (ttree.z - tz));
		    else
			trad = mRad;

		    if (ttree.intact && (trad < mRad))
			{
			obsangle = computeAngle(ttree.z,  tz, ttree.x,  tx);

			if (obsangle < 0)
			    obsangle += TWOPI;

			if ((fabs(mAngle + PI - obsangle) < mAng) || (fabs(mAngle - PI - obsangle) < mAng))
				{
				if (thaMonster.monster != FLUTTER)
				    blocked = 1;
				else if ((buildingHeight(ttree.type, ttree.treeshape) > 0) ||
				    ((ttree.type == 1) && (ttree.treeshape == 3) && (ttree.intact == 1)))
				    blocked = 1;
				}
			}

		    temptree = temptree->next;
		    }

		/****************************************************/
		/* cant move through tanks until they are destroyed */
		/****************************************************/

		temptank = tanklist->next;
		while ((temptank != NULL) && !blocked)
		    {

		    trad = sqrt((temptank->x - tx) * (temptank->x - tx) + (temptank->z - tz) * (temptank->z - tz));

		    if (trad < thaMonster.width)
			{
			obsangle = computeAngle(temptank->z, tz, temptank->x, tx);

			if (obsangle < 0)
			    obsangle += TWOPI;

			if ((fabs(mAngle + PI - obsangle) < .75) || (fabs(mAngle - PI - obsangle) < .75))
			    {
			    if ((temptank->y <= thaMonster.height) && (temptank->y >= thaMonster.bottom))
				blocked = 1;
			    if ((temptank->type == HERO) || (temptank->type == MECHAG))
				blocked = 1;
			    }
			}

		    temptank = temptank->next;
		    }
		}

	    /***********************************************************************************/
	    /* cant move through other targets until they are destroyed or move out of the way */
	    /***********************************************************************************/

	    if (monsterHasSolidCollision(&temptarget->monster))
		{
		temptarget2 = targets->next;
		while ((temptarget2 != NULL) && (!blocked))
		    {
		    if ((temptarget2 != temptarget) &&
		        monsterHasSolidCollision(&temptarget2->monster) &&
		        (temptarget2->monster.monsterIsDead != 1))
			{
			rad = sqrt((temptarget2->x - tx) * (temptarget2->x - tx) + (temptarget2->z - tz) * (temptarget2->z - tz));

			if (rad < mRad)
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
		/* cant move through player's monster */
		/**************************************/

		rad = sqrt(tx * tx + tz * tz);

		if ((rad < mRad) && monsterHasSolidCollision(&Googelon) &&
		    (Googelon.monsterIsDead != 1))
		    {
		    obsangle = computeAngle(-temptarget->z, 0, -temptarget->x, 0);

		    if (obsangle < 0)
			obsangle += TWOPI;

		    if ((fabs(mAngle + PI - obsangle) < mAng) || (fabs(mAngle - PI - obsangle) < mAng))
			blocked = 1;
		    }
		}

	    if (!blocked)
		{
		temptarget->z += cos(mAngle) * thaMonster.speed;
		temptarget->x += sin(mAngle) * thaMonster.speed;
		}
	}

	/**********************************/
	/* update target Weapons          */
	/**********************************/

	if (thaMonster.beamOn)
	    switch (thaMonster.monster){
		case GOOGELON:	    updateBeam(&Googelon, targets, temptarget->x, temptarget->z, tanklist, allTreesEverywherePtrs,
					    numTreesEverywhere, &thaMonster);

				    thaMonster.energyRemaining -= 0.15;
				    break;

		case TECHS:	    updateGun(temptarget->x, temptarget->z, thaMonster.headHorzRotate,
					thaMonster.headVertRotate, mainCounter, &(temptarget->monster));
				    thaMonster.energyRemaining -= 0.09;
				    break;

		case VAPOUR:	    updateVap(temptarget->x, temptarget->z, mainCounter, &(temptarget->monster));
				    thaMonster.energyRemaining -= 0.1;
				    break;

		case FLUTTER:	    updateBeam(&Googelon, targets, temptarget->x, temptarget->z, tanklist, allTreesEverywherePtrs,
					    numTreesEverywhere, &thaMonster);

				    thaMonster.energyRemaining -= 0.15;
				    break;

		default:	    showError("Bogus Monster! (Update Targets)");
				    break;
	    }

	/*****************/
	/* update energy */
	/*****************/

	if (thaMonster.energyRemaining > 0)
	    {
	    thaMonster.energyRemaining += thaMonster.regenRate;

	    if (thaMonster.energyRemaining > MAXLIFE)
		thaMonster.energyRemaining = MAXLIFE;

	    thaMonster.rot1 = cos(thaMonster.moveCount) * 0.02;
	    thaMonster.rot2 = sin(thaMonster.moveCount) * 0.05;
	    }
	else
	    {
	    updateDeadMonster(temptarget->x, temptarget->z, &thaMonster);
	    }

	if (thaMonster.monsterMoving)
	    thaMonster.moveCount += 1;


	temptarget->monster = thaMonster;
	}
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* update everything                                             */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void doUpdate()
    {
    struct boom * tempboom;
    struct tree * temptree;
    struct tank * temptank;
    struct targetInfo * temptarget;
    struct targetInfo * temptarg;
    struct projectile * temp;
    struct tree ttree;
    float rad;
    float mAngle, obsangle;
    float xv,  zv;
    float mRad, mAng;
    int blocked;

    if (playerWeaponReloadRemaining > 0)
	playerWeaponReloadRemaining -= 1;

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

    /*************************************************/
    /* add another monster if the time is right      */
    /*************************************************/

    if ((!netUp) && (!client) && (!alone))
	{
	if (mode == DEMOMODE)
	    {
	    if ((mainCounter > 500) && ((mainCounter % 4000) ==  1500))
		addRandomCatalogEntity();
	    }
	else
	    if ((mainCounter > 1000) && ((mainCounter % 9000) ==  8000))
		addRandomCatalogEntity();
	}

    /*********************/
    /* add a new vehicle */
    /*********************/

    if (!client)
	{
	if (((mainCounter % arrivalTime) == 0) && !Googelon.monsterIsDead)
	    {
		    if (!addRandomConfiguredEntity(targets, tanklist,
		                                   mainCounter))
			addNewTank(targets, 0, 0, -1, tanklist, treelist,
			           mainCounter, firingDelay, &Googelon);

	    if ((mode == DEMOMODE) && !(rand() % 4) && (Googelon.moveCount >= 150))
		view = (view + 1) % 3;
	    }
	}

    /**********************************/
    /* monster on the move            */
    /**********************************/

    if (Googelon.monster == FLUTTER)
	{
	mRad = 1.2;
	mAng = 1.0;
	}
    else
	{
	mRad = 0.8;
	mAng = 0.8;
	}

    if (Googelon.monsterGo || Googelon.monsterBack)
	{
	Googelon.monsterMoving = 1;

	Googelon.energyRemaining -= Googelon.moveCost; /* takes energy to move, not to turn */

	if (playerMoveActive)
	    mAngle = playerMoveAngle;
	else if (Googelon.monsterGo)
	    mAngle = (Googelon.headHorzRotate * BIG_DEG_TO_RAD);
	else if (Googelon.monsterBack)
	    mAngle = (Googelon.headHorzRotate + 1800) * BIG_DEG_TO_RAD;


	blocked = 0;

	if (monsterHasSolidCollision(&Googelon))
	    {
	    /****************************************************/
	    /* cant move through trees until they are destroyed */
	    /****************************************************/

	    temptree = treelist->next;
	    while ((temptree != NULL) && (!blocked))
		{
		ttree = *temptree;

		if (ttree.intact && (ttree.rad < mRad))
		    {
		    obsangle = ttree.theta;
		    if (obsangle < 0)
			obsangle += TWOPI;

		    if ((fabs(mAngle + PI - obsangle) < mAng) || (fabs(mAngle - PI - obsangle) < mAng))
			{
			if (Googelon.monster != FLUTTER)
			    blocked = 1;
			else if ((buildingHeight(ttree.type, ttree.treeshape) > 0) ||
			    ((ttree.type == 1) && (ttree.treeshape == 3) && (ttree.intact == 1)))
			    blocked = 1;
			}
		    }

		temptree = temptree->next;
		}

	    /****************************************************/
	    /* cant move through tanks until they are destroyed */
	    /****************************************************/

	    temptank = tanklist->next;
	    while ((temptank != NULL) && !blocked)
		{
		if (temptank->rad < Googelon.width)
		    {
		    obsangle = temptank->theta;
		    if (obsangle < 0)
			obsangle += TWOPI;

		    if ((fabs(mAngle + PI - obsangle) < .75) || (fabs(mAngle - PI - obsangle) < .75))
			{
			if ((temptank->y <= Googelon.height) && (temptank->y >= Googelon.bottom))
			    blocked = 1;
			if ((temptank->type == HERO) || (temptank->type == MECHAG))
			    blocked = 1;
			}
		    }
		temptank = temptank->next;
		}

	    /***********************************************************************************/
	    /* cant move through other targets until they are destroyed or move out of the way */
	    /***********************************************************************************/

	    temptarget = targets->next;
	    while ((temptarget != NULL) && (!blocked))
		{
		rad = sqrt(temptarget->x * temptarget->x + temptarget->z * temptarget->z);

		if ((rad < mRad) &&
		    monsterHasSolidCollision(&temptarget->monster) &&
		    (temptarget->monster.monsterIsDead != 1))
		    {
		    obsangle = computeAngle(temptarget->z, 0, temptarget->x, 0);

		    if (obsangle < 0)
			obsangle += TWOPI;

		    if ((fabs(mAngle + PI - obsangle) < mAng) || (fabs(mAngle - PI - obsangle) < mAng))
			    blocked = 1;
		    }

		temptarget = temptarget->next;
		}
	    }

	/***************************************/
	/* update position of everything else  */
	/***************************************/

	if (!blocked)
	    {
	    zv = cos(mAngle) * Googelon.speed;
	    xv = sin(mAngle) * Googelon.speed;

	    for(temptank = tanklist->next;temptank != NULL;temptank = temptank->next)
		{
		temptank->x -= xv;
		temptank->z -= zv;
		if (temptank->type == DATAENTITY)
		    entityAgentTranslate(&temptank->dataAgent, -xv, -zv);
		}
	    for(tempboom = projectboom->next;tempboom != NULL;tempboom = tempboom->next)
		{
		tempboom->x -= xv;
		tempboom->z -= zv;
		}
	    for(temp = projectFlight->next;temp != NULL;temp = temp->next)
		{
		temp->x -= xv;
		temp->z -= zv;
		}
	    for(temptank = slaglist->next;temptank != NULL;temptank = temptank->next)
		{
		temptank->x -= xv;
		temptank->z -= zv;
		}
	    for(temptree = treelist->next;temptree != NULL;temptree = temptree->next)
		{
		temptree->x -= xv;
		temptree->z -= zv;
		}
	    for(temptarget = targets->next;temptarget != NULL;temptarget = temptarget->next)
		    {
		    temptarget->x -= xv;
		    temptarget->z -= zv;

		    temptarget->net_x -= xv;
		    temptarget->net_z -= zv;
		    }

	    globalxshift -= xv;
	    globalzshift -= zv;
	    }
	}

    /**********************/
    /* update structures  */
    /**********************/

    if (treelist->next != NULL)
	updatetrees(treelist, itsChristmas);

    /**********************************/
    /* isolate the trees on the plane */
    /*  must come after update trees  */
    /**********************************/

    numTreesOnPlane = 0;
    numTreesEverywhere = 0;

    for(temptree = treelist->next; temptree != NULL; temptree = temptree->next)
	{
	allTreesEverywhere[numTreesEverywhere]	    = *temptree;
	allTreesEverywherePtrs[numTreesEverywhere]  = temptree;
	numTreesEverywhere += 1;

	if ((fabs(temptree->x) <= PLANESIZE) &&  (fabs(temptree->z) <= PLANESIZE))
	    {
	    allTreesOnPlane[numTreesOnPlane]	    = *temptree;
	    allTreesOnPlanePtrs[numTreesOnPlane]    = temptree;
	    numTreesOnPlane += 1;

	    if (numTreesOnPlane >= MAXTREESONPLANE)
		{
		showError("Too many trees on the plane");
		numTreesOnPlane -= 1;
		}
	    }
	}

    /****************************/
    /* update all the targets   */
    /****************************/

    if (targets->next != NULL)
	updateTargets();


    /************************/
    /* update the vehicles  */
    /************************/

     if (tanklist->next != NULL)
	updateTanks(tanklist, Googelon.width, Googelon.height, Googelon.bottom,
	    Googelon.monsterIsDead, Googelon.monster, slaglist, accuracy,
	    firingDelay, mainCounter, &Googelon, targets);


    /****************************/
    /* update the slagging tank */
    /****************************/

    if (slaglist->next != NULL)
	updateSlagTanks(slaglist);


    /**************************/
    /* update the projectiles */
    /**************************/

    quickTanks();

    if (projectFlight->next != NULL)
	updateProjectiles();


    /**********************************/
    /* update monster Weapons         */
    /**********************************/

    if (Googelon.beamOn)
	{
	turnBeamOnSinceLast();
	switch (Googelon.monster)
	    {
	    case GOOGELON:	updateBeam(&Googelon, targets, 0, 0, tanklist, allTreesOnPlanePtrs, numTreesOnPlane, &Googelon);
				Googelon.energyRemaining -= 0.15;
				break;

	    case TECHS:		updateGun(0, 0, Googelon.headHorzRotate, Googelon.headVertRotate, mainCounter, &Googelon);
				Googelon.energyRemaining -= 0.09;
				break;

	    case VAPOUR:	updateVap(0, 0, mainCounter, &Googelon);
				Googelon.energyRemaining -= 0.1;
				break;

	    case FLUTTER:	updateBeam(&Googelon, targets,0, 0, tanklist, allTreesOnPlanePtrs, numTreesOnPlane, &Googelon);
				Googelon.energyRemaining -= 0.15;
				break;

	    case DATAENTITY: updateConfiguredPlayerWeapon();
				break;

	    default:		showError("Bogus Monster! (doUpdate)");
				break;
	    }
	}

    /*************************/
    /* update the explosions */
    /*************************/

    if (projectboom->next != NULL)
	updateBooms(projectboom, tanklist);


    /***********************/
    /* update the monster  */
    /***********************/

    if (!Googelon.beamOn)
	soundKiller(MONSTERBEAM);


    /****************************/
    /* update monster's energy  */
    /****************************/

    if (Googelon.energyRemaining > 0)
	{
	Googelon.energyRemaining += Googelon.regenRate;

	if (Googelon.energyRemaining > MAXLIFE)
	    Googelon.energyRemaining = MAXLIFE;

	Googelon.rot1 = cos(Googelon.moveCount) * 0.02;
	Googelon.rot2 = sin(Googelon.moveCount) * 0.05;
	}
    else
	{
	updateDeadMonster(0, 0, &Googelon);
	}

    if (Googelon.monsterMoving)
	Googelon.moveCount += 1;

    /*************************************/
    /* update the high-score list        */
    /*************************************/

   if (Googelon.monsterIsDead && (Googelon.deadCount == 1))
	updateScores(scorefullPath, Googelon.monster, Googelon.monsterScore, mode, playerName);

    /*********************************/
    /* deal with dead targets        */
    /*********************************/

    temptarget = targets;
    while(temptarget->next != NULL)
	{
	    {
	    if (temptarget->next->monster.timeDead > 250)
		{
		for(tboom=projectboom->next; tboom != NULL; tboom=tboom->next)
		    {
		    if (tboom->mine == &(temptarget->next->monster))
			tboom->mine = NULL;
		    }

		for(tproj=projectFlight->next; tproj != NULL; tproj=tproj->next)
		    {
		    if (tproj->mine == &(temptarget->next->monster))
			tproj->mine = NULL;
		    }

		temptarg = temptarget->next;
		temptarget->next = temptarget->next->next;

		if ((temptarg->monster.monster == FLUTTER) ||(temptarg->monster.monster == GOOGELON))
		    {
		    addProjectile( temptarg->x-0.2, PLANEY+0.25, temptarg->z+0.2, 3, 0, -0.003, 0, 10000, NULL);
		    addProjectile( temptarg->x+0.2, PLANEY+0.25, temptarg->z+0.2, 3, 0, -0.003, 0, 10000, NULL);
		    addProjectile( temptarg->x+0.2, PLANEY+0.25, temptarg->z-0.2, 3, 0, -0.003, 0, 10000, NULL);
		    addProjectile( temptarg->x-0.2, PLANEY+0.25, temptarg->z-0.2, 3, 0, -0.003, 0, 10000, NULL);
		    }

		afree(temptarg, arena);
		}
	    else
		temptarget = temptarget->next;
	    }
	}

   if ((mode == DEMOMODE) && (Googelon.timeDead > 400))
	{
	stepPlayerEntitySelection(1);
	Googelon.monster = DATAENTITY;
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
/* view the scene from the monster's point of view               */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void goToMonsterView(int eyeball)
    {
    float aspect;
    float cameraX;
    float cameraY;
    float lookY;
    float playerBottom;
    float playerHeight;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    aspect = viewH > 0 ? (float)viewW / (float)viewH : 1.0f;
    gluPerspective(90, aspect, 0.1, 15);
    glMatrixMode(GL_MODELVIEW);

    playerBottom = Googelon.bottom;
    playerHeight = Googelon.height - Googelon.bottom;
    if (Googelon.monster == DATAENTITY && playerEntityArchetype != NULL)
	{
	playerBottom = PLANEY + playerEntityArchetype->verticalOffset;
	playerHeight = playerEntityArchetype->collisionHeight;
	}
    cameraY = playerBottom + playerHeight * 0.75f + 0.20f;
    lookY = cameraY - 0.05f;
    cameraX = 0.0f;
    if (eyeball == 0)
	cameraX = 0.015f;
    else if (eyeball == 1)
	cameraX = -0.015f;

    switch(eyeball){
	case 2: gluLookAt(0, cameraY, -1.4f,
			  0, lookY, 0.4f, 0, 1, 0);
		break;

	case 0: gluLookAt(cameraX, cameraY, -1.4f,
			  cameraX, lookY, 0.4f, 0, 1, 0);
		break;

	case 1: gluLookAt(cameraX, cameraY, -1.4f,
			  cameraX, lookY, 0.4f, 0, 1, 0);
		break;

	}

    if (lod >= 2)
	{
	glFogi(GL_FOG_START,  MONSTERFOGSTART);
	glFogi(GL_FOG_END,  MONSTERFOGEND);

	glEnable(GL_FOG);
	}

    yrot = - (GLfloat) Googelon.headHorzRotate;
    xrot = 0;
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* true first-person view, separate from the original close view */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void goToFirstPersonView(int eyeball)
    {
    float aspect;
    float eyeX;
    float eyeY;
    float lookY;
    float lookZ;
    float pitch;

    if (Googelon.monster != DATAENTITY || playerEntityArchetype == NULL)
	{
	goToMonsterView(eyeball);
	return;
	}
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    aspect = viewH > 0 ? (float)viewW / (float)viewH : 1.0f;
    gluPerspective(90, aspect, 0.1, 15);
    glMatrixMode(GL_MODELVIEW);

    eyeX = 0.0f;
    if (eyeball == 0)
	eyeX = 0.015f;
    else if (eyeball == 1)
	eyeX = -0.015f;
    eyeY = PLANEY + playerEntityArchetype->firstPersonEyeHeight;
    pitch = Googelon.headVertRotate * BIG_DEG_TO_RAD;
    lookY = eyeY - (float)sin((double)pitch);
    lookZ = 0.1f + (float)cos((double)pitch);
    gluLookAt(eyeX, eyeY, 0.1, eyeX, lookY, lookZ, 0, 1, 0);

    if (lod >= 2)
	{
	glFogi(GL_FOG_START, MONSTERFOGSTART);
	glFogi(GL_FOG_END, MONSTERFOGEND);
	glEnable(GL_FOG);
	}
    yrot = - (GLfloat) Googelon.headHorzRotate;
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
    gluPerspective(60,  1,   0.1,   20);
    glMatrixMode(GL_MODELVIEW);

    switch(eyeball){
	case 2: gluLookAt(lastTankRad * sin(lastTankTheta), lastTankY,
		lastTankRad*cos(lastTankTheta), 0, lastTankY, 0, 0,1,0);
		break;

	case 0: gluLookAt(lastTankRad * sin(lastTankTheta + 0.002),
		lastTankY, lastTankRad * cos(lastTankTheta + 0.002),
		-lastTankRad * sin(lastTankTheta-0.002), lastTankY,
		-lastTankRad * cos(lastTankTheta-0.002), 0,1,0);
		break;

	case 1: gluLookAt(lastTankRad * sin(lastTankTheta - 0.002),
		lastTankY, lastTankRad * cos(lastTankTheta - 0.002),
		-lastTankRad*sin(lastTankTheta+0.002), lastTankY,
		-lastTankRad * cos(lastTankTheta+0.002), 0,1,0);
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
				 /* used to get overhead map of the town */
				/* also set plane size to 40 */
			       /* and eliminate prepositioned vehicles */
			      /* and hide motion dots */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(mapHeight*viewW/viewH, -mapHeight*viewW/viewH, mapHeight,
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
    float centerY;

    view = OMNISCIENTVIEW;


    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60,  1,   0.1,   25);
    glMatrixMode(GL_MODELVIEW);

    if (Googelon.monster == DATAENTITY && playerEntityArchetype != NULL)
	centerY = PLANEY + playerEntityArchetype->verticalOffset +
	          playerEntityArchetype->collisionHeight * 0.5f;
    else
	centerY = 0;

    switch(eyeball){
	case 2: if (Googelon.monster == DATAENTITY)
		    gluLookAt(0, centerY, 9, 0, centerY, 0, 0, 1, 0);
		else
		    glCallList(overviewLookat);
		break;

	case 0: gluLookAt( 0.05, centerY, 9,
			   0.05, centerY, 0, 0,1,0);
		break;

	case 1: gluLookAt(-0.05, centerY, 9,
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
/* draw a given monster at a given location                      */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void drawAMonster(float x, struct monsterInfo monster, float z,
			int isTarget, int view)
    {
    const struct EntityArchetype *networkArchetype;
    float aimPitch;
    float aimYaw;
    float bodyHeading;

    if ((fabs(x) <= PLANESIZE) && (fabs(z) <= PLANESIZE))
	{
	glPushMatrix();
	    glTranslatef(x,  0,  z);
	    switch(monster.monster){
		case GOOGELON:  if (monster.energyRemaining <= 0)
				    {
				    glTranslatef(0, -monster.deadCount * 0.015, 0);
				    glRotatef(-monster.deadCount * 2, 1,0,0);
				    glRotatef(-monster.deadCount * 2, 0,1,0);
				    }
				if ((!isTarget) && (view == MONSTERVIEW))
				    drawSimpleMonster(monster, mainCounter, itsChristmas, lod);
				else
				    drawMonster(monster, mainCounter, itsChristmas, lod);
				break;

		case VAPOUR:    networkArchetype =
				    networkEntityArchetype(VAPOUR);
				if (networkArchetype != NULL)
				    {
				    if (monster.energyRemaining <= 0)
					{
					glTranslatef(0,
					             -monster.deadCount * 0.015f, 0);
					glRotatef(-monster.deadCount * 2.0f,
					          1, 0, 0);
					}
				    glTranslatef(0, PLANEY +
				        networkArchetype->verticalOffset, 0);
				    glRotatef(monster.headHorzRotate * 0.1f,
				              0, 1, 0);
				    drawConfiguredPlayerEntity(
				        networkArchetype, monster.monsterMoving,
				        monster.beamOn, mainCounter,
				        monster.energyRemaining <= 0, 0.0f, 0.0f);
				    }
				break;

		case TECHS:	if (monster.energyRemaining <= 0)
				    {
				    glTranslatef(0, -monster.deadCount * 0.015, 0);
				    glRotatef(-monster.deadCount * 2, 1,0,0);
				    }
				if ((!isTarget) && (view == MONSTERVIEW))
				    drawSimpleTechs(monster,lod);
				else
				    drawTechs(monster,lod);
				break;

		case FLUTTER:   if (monster.energyRemaining <= 0)
				    {
				    glTranslatef(0, -monster.deadCount * 0.025, 0);
				    glRotatef(-monster.deadCount * 2.5, 0,1,0);
				    }
				if (isTarget)
				    drawFlutter(monster, mainCounter, itsChristmas,
					    offsetX, OMNISCIENTVIEW, lod);
				else
				    drawFlutter(monster, mainCounter, itsChristmas,
					    offsetX, view, lod);

				break;

		case DATAENTITY: networkArchetype =
				     monsterArchetype(&monster);
				if (networkArchetype == NULL)
				    break;
				if ((!isTarget) &&
				     (view == FIRSTPERSONVIEW) &&
				     !networkArchetype->firstPersonVisible)
				    break;
				if (monster.energyRemaining <= 0)
				    {
				    glTranslatef(0, -monster.deadCount * 0.015f, 0);
				    glRotatef(-monster.deadCount * 2.0f, 1, 0, 0);
				    }
				glTranslatef(0,
				             PLANEY + networkArchetype->verticalOffset,
				             0);
				bodyHeading = monster.headHorzRotate * 0.1f;
				aimYaw = 0.0f;
				aimPitch = monster.headVertRotate * 0.1f;
				if ((!isTarget) &&
				    networkArchetype->controlMode ==
				        ENTITY_CONTROL_STEERED)
				    {
				    bodyHeading = playerBodyHeading * 0.1f;
				    aimYaw = (monster.headHorzRotate -
				              playerBodyHeading) * 0.1f;
				    }
				glRotatef(bodyHeading,
				          0, 1, 0);
				drawConfiguredPlayerEntity(
				    networkArchetype, monster.monsterMoving,
				    monster.beamOn, mainCounter,
				    monster.energyRemaining <= 0,
				    aimYaw, aimPitch);
				break;
	    }
	glPopMatrix();
	}
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw everything                                               */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void doDrawing (int eyeball)
    {
    const struct EntityArchetype *beamArchetype;
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

    if (view == MONSTERVIEW)
	goToMonsterView(eyeball);
    else if (view == FIRSTPERSONVIEW)
	goToFirstPersonView(eyeball);
    else if ((view == ARMYVIEW) && (lastTankY > NOTANKONPLANE))
	goToArmyView(eyeball);
    else if (view == MAPVIEW)
	goToMapView();
    else /*view == OMNISCIENTVIEW */
	goToOverView(eyeball);

    /***********************************/
    /* rotate and draw the battlefield */
    /***********************************/

    if (xrot != 0)
	glRotatef(0.1 * xrot, 1,0,0);

    if (yrot != 0)
    	glRotatef(0.1 * yrot, 0,1,0);

    glDisable(GL_DEPTH_TEST);

    drawBattlefield(roadSystem, globalxshift, globalzshift, lod,
		itsChristmas, view);

    if (lod != -1)
	glEnable(GL_DEPTH_TEST);

    /**********************************/
    /* draw the slagging vehicles     */
    /**********************************/

    if (slaglist->next != NULL)
	drawSlagTanks(slaglist, mainCounter, lod);


    /******************************************************/
    /* draw the vehicles                                  */
    /* must come before draw structures for hero entrance */
    /******************************************************/

    if (tanklist->next != NULL)
	drawTanks(tanklist, mainCounter, Googelon, targets, lod,
		view, viewW);



    /**********************************/
    /* draw all projectiles in flight */
    /**********************************/

    if (projectFlight->next != NULL)
	drawProjectiles(projectFlight, mainCounter, fires,
	flameCount, itsChristmas, lod, &Googelon, view);



    /********************/
    /* draw structures  */
    /********************/

    if (treelist->next != NULL)
	if (view == MAPVIEW)
	    drawtrees(allTreesEverywhere, numTreesEverywhere, mainCounter,
				lod, itsChristmas, view);
	else
	    drawtrees(allTreesOnPlane, numTreesOnPlane, mainCounter,
				lod, itsChristmas, view);



    /**********************************/
    /* draw monsterBeam               */
    /**********************************/

    beamArchetype = monsterArchetype(&Googelon);
    if (Googelon.beamOn && beamArchetype != NULL &&
	beamArchetype->weapon == ENTITY_WEAPON_BEAM)
	drawBeam(0, 0, Googelon.headHorzRotate, Googelon.headVertRotate,
		 beamArchetype, lod);

    for (temptarget = targets->next;temptarget != NULL;temptarget = temptarget->next)
	{
	beamArchetype = monsterArchetype(&temptarget->monster);
	if (temptarget->monster.beamOn && beamArchetype != NULL &&
	    beamArchetype->weapon == ENTITY_WEAPON_BEAM)
	    drawBeam(temptarget->x, temptarget->z,
			temptarget->monster.headHorzRotate,
			temptarget->monster.headVertRotate,
			beamArchetype, lod);
	}



    /*************************************************/
    /* draw explosions to be seen through the VAPOUR */
    /*************************************************/
/*I suppose technically if ANY monster on the field is the vapour this
 * should be done but that may be too costly
 */

    if ((Googelon.monster == VAPOUR) && (projectboom->next != NULL))
	{
	if (lod == -1)
	    {
	    glPushAttrib(GL_POLYGON_BIT);
	    glPolygonMode(GL_FRONT_AND_BACK,GL_POINT);
	    drawBooms(projectboom, lod);
	    glPopAttrib();
	    }
	else
	    drawBooms(projectboom, lod);
	}



    /**********************************/
    /* draw the targets               */
    /**********************************/

    for(temptarget = targets->next;temptarget != NULL;temptarget = temptarget->next)
	drawAMonster(temptarget->x, temptarget->monster, temptarget->z, 1,view);


    /**********************************/
    /* draw the player's monster      */
    /**********************************/

    drawAMonster(0, Googelon, 0, 0,view);

    /**********************************/
    /* draw explosions                */
    /*                                */
    /* explosions must be last so     */
    /* other things can be seen       */
    /* throught them                  */
    /**********************************/

    if (projectboom->next != NULL)
	{
	if (lod == -1)
	    {
	    glPushAttrib(GL_POLYGON_BIT);
	    glPolygonMode(GL_FRONT_AND_BACK,GL_POINT);
	    drawBooms(projectboom, lod);
	    glPopAttrib();
	    }
	else
	    drawBooms(projectboom, lod);
	}

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
	gluPerspective(60,  1,   0.1,   20);
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

	/* showText(Googelon.energyRemaining,  Googelon.monsterScore,
	(long) (viewR-viewL), paused);   */

	showText(targets, Googelon.energyRemaining,  Googelon.monsterScore,
	    showframes, paused, pointerGrab, playerWeaponUsesReload(),
	    playerWeaponReadiness());

	if (mode == DEMOMODE)
	    {
	    if (!(Googelon.timeDead > 100) && !showOptions)
		{
		if (Googelon.moveCount < 150)
		    showScores(itsChristmas, G, V, T, F, Googelon, mainCounter, offsetX, lod);
		else if (!showHelp)
		    showText2((long) (viewR-viewL), getSoundOn(), noSound, getMusicOn(),
			mode3D, no3D, lod, paused, itsChristmas, pointerGrab);
		}


	    showText3(lod);

	    if (showOptions)
		doOptions(Googelon, (long) (viewR-viewL),  mainCounter, itsChristmas, offsetX, lod);
	    else
		showText4();

	    if ((Googelon.timeDead > 100) && !showOptions)
		doSummary(Googelon.monster,  Googelon.timeDead,
		    (long) (viewR-viewL), killtanks, killmtanks,
		    killhelos, killCHHs, killmechags, killheros,
		    killplanes, killlaunchers, killfighters,
		    mainCounter, lod, moreThanOne);
	    }

	if (showHelp)
	    showText2((long) (viewR-viewL), getSoundOn(), noSound, getMusicOn(),
		mode3D, no3D, lod, paused, itsChristmas, pointerGrab);

	glPopMatrix();
	}

   tkSwapBuffers();

    }


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* computer controlled monster for demo-mode                     */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

struct monsterInfo autopilot(float centerX, float centerZ, struct monsterInfo thaMonster)
    {
    struct tank * ttank;
    struct tree * ttree;

    struct enemy et, el, em, eh, ec, eg, ehe, ep, eb, ef, eall, einside;

    float alphaBig;
    float modHead;
    float y, ang, r, modR;
    int speed, speed2;
    int t;
    float rad;
    float tx, tz;
    float zdiff;
    float xdiff;

    t = -1;

    et.r = el.r = em.r = eh.r = ec.r = eg.r =
    ehe.r = ep.r = eb.r = ef.r = eall.r = BIGNUM;

    /*************************************/
    /* find nearest building to destroy  */
    /*************************************/

    for(ttree = treelist->next; ttree != NULL; ttree = ttree->next)
	if ((ttree->type == 1)  && (ttree->intact == 1))
	    {
	    zdiff = ttree->z-centerZ;
	    xdiff = ttree->x-centerX;

	    rad =  sqrt(xdiff * xdiff + zdiff * zdiff);

	    if (rad < eb.r)
		{
		eb.r    = rad;
		eb.y    = ttree->y;
		eb.ang  = computeAngle(zdiff, 0, xdiff, 0);
		}
	    }

    /*********************************/
    /* find nearest military threat  */
    /*********************************/

    eall = eb;
    einside = eb;

    for(ttank = tanklist->next; ttank != NULL; ttank = ttank->next)
	    {
	    tx = ttank->x;
	    tz = ttank->z;
	    rad =  sqrt((tx-centerX) * (tx-centerX) + (tz-centerZ) * (tz-centerZ));

	    if (rad < eall.r)
		{
		eall.r   = rad;
		eall.y   = ttank->y;
		eall.ang = computeAngle(tz, centerZ, tx, centerX);
		}

	    if ((rad < einside.r) && (fabs(tx) < fabs(centerX)) && (fabs(tz) < fabs(centerZ)))
		{
		 einside.r   = rad;
		 einside.y   = ttank->y;
		 einside.ang = computeAngle(tz, centerZ, tx, centerX);
		}

	    if (rad <= PLANESIZE)
		{

		switch(ttank->type){
			case TANK	:   if (rad < et.r)
						{
						et.r   = rad;
						et.y   = ttank->y;
						et.ang = computeAngle(tz, centerZ, tx, centerX);
						}
					    break;

			case LAUNCHER	:   if (rad < el.r)
						{
						el.r   = rad;
						el.y   = ttank->y;
						el.ang = computeAngle(tz, centerZ, tx, centerX);
						}
					    break;

			case MASERTANK	:   if (rad < em.r)
						{
						em.r   = rad;
						em.y   = ttank->y;
						em.ang = computeAngle(tz, centerZ, tx, centerX);
						}
					    break;

			case HELO	:   if (rad < eh.r)
						{
						eh.r   = rad;
						eh.y   = ttank->y;
						eh.ang = computeAngle(tz, centerZ, tx, centerX);
						}
					    break;

			case CHH	:   if (rad < ec.r)
						{
						ec.r   = rad;
						ec.y   = ttank->y;
						ec.ang = computeAngle(tz, centerZ, tx, centerX);
						}
					    break;

			case MECHAG	:   if (rad < eg.r)
						{
						eg.r   = rad;
						eg.y   = ttank->y + 0.5;
						eg.ang = computeAngle(tz, centerZ, tx, centerX);
						}
					    break;

			case HERO	:   if (rad < ehe.r)
						{
						ehe.r   = rad;
						ehe.y   = ttank->y + 0.5;
						ehe.ang = computeAngle(tz, centerZ, tx, centerX);
						}
					    break;

			case AIRPLANE	:   if (rad < ep.r)
						{
						ep.r   = rad;
						ep.y   = ttank->y + 0.5;
						ep.ang = computeAngle(tz, centerZ, tx, centerX);
						}
					    break;

			case FIGHTER	:   if (rad < ef.r)
						{
						ef.r   = rad;
						ef.y   = ttank->y + 0.5;
						ef.ang = computeAngle(tz, centerZ, tx, centerX);
						}
					    break;

			case DATAENTITY: if (rad < et.r)
						{
						et.r   = rad;
						et.y   = ttank->y;
						et.ang = computeAngle(tz, centerZ,
						                      tx, centerX);
						}
					    break;

			default:	    showError("Bogus Vehicle! (autopilot)");
					    break;
			}

		    }

		}

    /****************************/
    /* choose target to attack  */
    /****************************/

    if (ehe.r != BIGNUM)
	    {
	    r 	= ehe.r;
	    y 	= ehe.y;
	    ang = ehe.ang;
	    t 	= HERO;
	    }
    else if (eg.r != BIGNUM)
	    {
	    r 	= eg.r;
	    y 	= eg.y;
	    ang = eg.ang;
	    t 	= MECHAG;
	    }
    else if (ec.r != BIGNUM)
	    {
	    r 	= ec.r;
	    y 	= ec.y;
	    ang = ec.ang;
	    t 	= CHH;
	    }
    else if (em.r != BIGNUM)
	    {
	    r 	= em.r;
	    y 	= em.y;
	    ang = em.ang;
	    t 	= MASERTANK;
	    }
    else if (el.r != BIGNUM)
	    {
	    r 	= el.r;
	    y 	= el.y;
	    ang = el.ang;
	    t 	= LAUNCHER;
	    }
    else  if ((eb.r < et.r) && (eb.r < eh.r))
	    {
	    r 	= eb.r;
	    y 	= eb.y + 0.5;
	    ang = eb.ang;
	    t 	= HERO; /* building */
	    }
    else if (eh.r < et.r)
	    {
	    r 	= eh.r;
	    y 	= eh.y;
	    ang = eh.ang;
	    t 	= HELO;
	    }
    else  if ((ep.r < et.r) && (et.r != BIGNUM))
	    {
	    r 	= ep.r;
	    y 	= ep.y;
	    ang = ep.ang;
	    t 	= AIRPLANE;
	    }
    else  if ((ef.r < et.r) && (et.r != BIGNUM))
	    {
	    r 	= ef.r;
	    y 	= ef.y;
	    ang = ef.ang;
	    t 	= FIGHTER;
	    }
    else if (et.r < BIGNUM)
	    {
	    r 	= et.r;
	    y 	= et.y;
	    ang = et.ang;
	    t 	= TANK;
	    }
    else if (einside.r < BIGNUM)
	    {
	    r 	= einside.r;
	    y 	= einside.y;
	    ang = einside.ang;
	    t 	= HERO;
	    }
    else
	    {
	    r 	= eall.r;
	    y 	= eall.y;
	    ang = eall.ang;
	    t 	= HERO;
	    }

    /*********************/
    /* move the monster  */
    /*********************/

    if (ang < 0)
	ang += TWOPI;

    modHead = (thaMonster.headHorzRotate+1800);
    if (modHead > 3600)
	modHead -= 3600;

    speed = (int) (ang * RAD_TO_BIG_DEG - modHead);
    if (speed > 1800)
	speed -= 3600;
    else if (speed < - 1800)
	speed += 3600;

    speed = speed / 15;
    if (speed > 0.75 * thaMonster.xspeed)
	speed = (int) (0.75 * thaMonster.xspeed);
    if (speed < -0.75 * thaMonster.xspeed)
	speed = (int) (-0.75 * thaMonster.xspeed);

    thaMonster.headHorzRotate += speed;
    offsetX = 0.8*offsetX - 0.2*(speed * 0.05); /* used to bank flutter in demo mode*/

    if ((thaMonster.energyRemaining > 15) && (fabs(speed) < 6) && (r < 3.5))
	thaMonster.beamOn = 1;

	switch(thaMonster.monster){
	case GOOGELON:
	case TECHS:
	case DATAENTITY:
	    if ((thaMonster.energyRemaining > 5) && (t != -1)  && (t != HERO) &&
		(t != MECHAG) && ((fabs(speed) < 3) || (r < 1)))
		thaMonster.monsterGo = 1;

	    else if (((t == MECHAG)  || (t == HERO)) && (fabs(speed) < 5) && (r > 3))
		thaMonster.monsterGo = 1;

	    else if (((t == MECHAG)  || (t == HERO)) && (fabs(speed) < 5) && (r < .75))
		thaMonster.monsterBack = 1;
	    break;

	case VAPOUR:
	    if (((t == MECHAG) || (t == CHH) || (t == HERO)) && (r < 0.2))
		/*stay put on top of the enemy*/;

	    else if ((thaMonster.energyRemaining > 5) && ((fabs(speed) < 10) || (r < 4)))
		thaMonster.monsterGo = 1;
	    break;

	case FLUTTER:
	    if ((thaMonster.energyRemaining > 5) && (t != -1)  && (t != HERO) &&
		(t != MECHAG) && ((fabs(speed) < 3) || (r < 1)))
		thaMonster.monsterGo = 1;
	    else if (((t == MECHAG) || (t == HERO)) && (fabs(speed) < 5) && (r > 3))
		thaMonster.monsterGo = 1;
	    else if (((t == MECHAG) || (t == HERO)) && (fabs(speed) < 5) && (r < .75))
		thaMonster.monsterBack = 1;
	    else if (!(((t == MECHAG) || (t == HERO)) && (fabs(speed) < 5) && (r < 1)))
		thaMonster.monsterGo = 1;
	    break;
    }

    /************************/
    /* tilt monster's head  */
    /************************/

    y = thaMonster.height - 0.1 - y;

    if (thaMonster.monsterGo)
	modR = r - 1.3;
    else
	modR = r - 0.6;

    if (modR <= 0)
	modR = NEARZERO;

    alphaBig = atan(y / modR) * RAD_TO_BIG_DEG;

    speed2 = (int) (fabs(alphaBig - thaMonster.headVertRotate))/15;

    if (alphaBig > thaMonster.headVertRotate)
	thaMonster.headVertRotate += speed2;
    else
	thaMonster.headVertRotate -= speed2;

    thaMonster.monsterMoving = 1;

    return(thaMonster);
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
	short Height3d,  Width3d;

	/**********************************/
	/* draw in stereo                 */
	/**********************************/

	if (doBigClear)
	    {
	    glViewport(0,  0, XMAXSCREEN+1, YMAXSCREEN+1);
	    glScissor(0,  0, XMAXSCREEN+1, YMAXSCREEN+1);

	    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	    doBigClear -= 1;
	    }

	Height3d = (short) windowTall/4;
	Width3d = (short) windowWide/2;

	glViewport((short) (XMAXSCREEN/2 - Width3d),
	    (short) (YMAXSTEREO/2 - Height3d),
	    ( (short) (XMAXSCREEN/2 + Width3d))-((short) (XMAXSCREEN/2 - Width3d))+1,
	    ( (short) (YMAXSTEREO/2 + Height3d))-( (short) (YMAXSTEREO/2 - Height3d))+1);

	glScissor((short) (XMAXSCREEN/2 - Width3d),  (short) (YMAXSTEREO/2 - Height3d),
	    ( (short) (XMAXSCREEN/2 + Width3d))-((short) (XMAXSCREEN/2 - Width3d))+1,
	    ( (short) (YMAXSTEREO/2 + Height3d))-( (short) (YMAXSTEREO/2 - Height3d))+1);

	glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

	doDrawing(0);

	glViewport((short) (XMAXSCREEN/2 - Width3d),
	    (short) (YOFFSET+YMAXSTEREO/2 - Height3d),
	    ( (short) (XMAXSCREEN/2 + Width3d))-((short) (XMAXSCREEN/2 - Width3d))+1,
	    ((short) (YOFFSET+YMAXSTEREO/2 + Height3d))-( (short) (YOFFSET+YMAXSTEREO/2 - Height3d))+1);

	glScissor((short) (XMAXSCREEN/2 - Width3d),  (short) (YOFFSET+YMAXSTEREO/2 - Height3d),
	    ( (short) (XMAXSCREEN/2 + Width3d))-((short) (XMAXSCREEN/2 - Width3d))+1,
	    ((short) (YOFFSET+YMAXSTEREO/2 + Height3d))-( (short) (YOFFSET+YMAXSTEREO/2 - Height3d))+1);

	glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

	doDrawing(1);
	}
    }


/**********************************/

void id()
    {
    struct tank * ttank;
    struct targetInfo * temptarget;

    if (targets->next != NULL)
	{
	for(temptarget = targets->next; temptarget != NULL; temptarget = temptarget->next)
	    {
	    temptarget->monster.monsterMoving	= 0;
	    temptarget->monster.monsterGo	= 0;
	    temptarget->monster.monsterBack	= 0;

	    temptarget->monster.beamOn		= 0;
	    }
	}

    if (mode == DEMOMODE)
	{
	Googelon.beamOn = 0;
	}

    Googelon.monsterMoving = 0;

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


    if (Googelon.monsterIsDead && (mode == PLAYMODE))
	{
	mode = DEMOMODE;
	clearPlayerInput();
	setPointerCaptured(0);
	}

    oldview = view;

    if ((view == MONSTERVIEW) ||
	(view == FIRSTPERSONVIEW)) /* must be before view change!!!!*/
	yrot = (GLfloat) -Googelon.headHorzRotate;
    else if (view == OMNISCIENTVIEW)
	{
	ysaverot = yrot;
	xsaverot = xrot;
	}

    if ((mode == DEMOMODE) && !Googelon.monsterIsDead && !paused)
	Googelon = autopilot(0, 0, Googelon);


    for(temptarget = targets->next; temptarget != NULL; temptarget = temptarget->next)
	temptarget->monster = autopilot(temptarget->x, temptarget->z, temptarget->monster);


    /**********************************/
    /* update everything              */
    /**********************************/

    if (netUp)
	processNetwork(now, mainCounter, totalCounter, treelist,
		       tanklist, arena, Googelon, buildingBoomSoFarCounter,
		       buildingBoomThisFrameCounter, buildingBoomSoFar,
		       buildingBoomThisFrame);

    if (client)
	processClient(now, mainCounter, totalCounter, treelist,
		      tanklist, arena, sizeTank, Googelon,
		      buildingBoomThisFrameCounter, buildingBoomThisFrame);

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

    lastTankY = NOTANKONPLANE;
    lastTankTheta = lastTankRad = 0;

    for(ttank = tanklist->next; (ttank != NULL) && (lastTankY <= NOTANKONPLANE); ttank = ttank->next)
	if ((fabs(ttank->x) <= PLANESIZE) && (fabs(ttank->z) <= PLANESIZE) && (ttank->damage > 0)
  	    && (ttank->rad < 10))
	    {
	    lastTankY	    = ttank->y + 0.4;
	    lastTankTheta   = ttank->theta + PI;
	    lastTankRad	    = ttank->rad + 1.5;
	    }

    /************************************************/
    /* ensure all monster variables are within range*/
    /************************************************/

    if (xrot > 600)
	xrot = 600;

    if (xrot < 0)
	xrot = 0;

    if (yrot < 0)
	yrot += 3600;

    if (yrot >= 3600)
	yrot -= 3600;

    if (Googelon.headHorzRotate < 0)
	Googelon.headHorzRotate += 3600;

    if (Googelon.headHorzRotate >= 3600)
	Googelon.headHorzRotate -= 3600;

    if (Googelon.monster == DATAENTITY && playerEntityArchetype != NULL)
	{
	if (Googelon.headVertRotate <
	    (int)(playerEntityArchetype->aimPitchMin * 10.0f))
	    Googelon.headVertRotate =
		(int)(playerEntityArchetype->aimPitchMin * 10.0f);
	if (Googelon.headVertRotate >
	    (int)(playerEntityArchetype->aimPitchMax * 10.0f))
	    Googelon.headVertRotate =
		(int)(playerEntityArchetype->aimPitchMax * 10.0f);
	}
    else
	{
	if (Googelon.headVertRotate < -400)
	    Googelon.headVertRotate = -400;
	if (Googelon.headVertRotate > 400)
	    Googelon.headVertRotate = 400;
	}

    if (targets->next != NULL)
	{
	for(temptarget = targets->next; temptarget != NULL; temptarget = temptarget->next)
	    {
	    if (temptarget->monster.headHorzRotate < 0)
		temptarget->monster.headHorzRotate += 3600;

	    if (temptarget->monster.headHorzRotate >= 3600)
		temptarget->monster.headHorzRotate -= 3600;

	    if (temptarget->monster.headVertRotate < -400)
		temptarget->monster.headVertRotate = -400;

	    if (temptarget->monster.headVertRotate > 400)
		temptarget->monster.headVertRotate = 400;
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
    fprintf (stderr, "Usage: %s [-a] [-b] [-d level] [-m] [-s] [--map name] [-help]\n", progName);
#else
    fprintf (stderr, "Usage: %s [-a] [-b] [-d level] [-m] [-s] [--map name] [-c] [-h #] [-n] [-p #] [-help]\n", progName);
#endif

    fprintf (stderr, "  -a)lone        fight alone\n");
    fprintf (stderr, "  -b)ackdrop     hide the overlays\n");
    fprintf (stderr, "  -d)etail n     set level-of-detail from 0 (low) to 2 (high)\n");
    fprintf (stderr, "  -m)usic        initially turn on the music\n");
    fprintf (stderr, "  -s)ound        initially turn on the sound effects\n");
    fprintf (stderr, "  --map name     select a battlefield by number or city name\n");
    fprintf (stderr, "  --player id    select a controllable entity archetype\n");
    fprintf (stderr, "  --view n       select view 0, 1, 2, 4, or 5\n");
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
		requestedView = MONSTERVIEW;
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
    }

static int selectRequestedPlayerEntity(void)
    {
    const struct EntityArchetype *requested;

    if (requestedPlayerEntity[0] == '\0')
        return 1;
    requested = entityCatalogFind(&gameEntities, requestedPlayerEntity);
    if (!setPlayerEntityArchetype(requested))
        return 0;
    Googelon.monster = DATAENTITY;
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

    if ((netUp || client) &&
	(playerEntityArchetype == NULL ||
	 playerEntityArchetype->networkCode < 0))
	{
	showError("Selected player entity has no network-code");
	exit(1);
	}

    /**********************************************/
    /* set starting conditions for chosen monster */
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
