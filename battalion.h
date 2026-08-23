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
/* header file v 1.3                                                   */
/***********************************************************************/

#define RECONNECT_TIME		15
#define IM_NOT_DEAD_YET_TIME	60
#define IM_NOT_DEAD_SEND_TIME	19
#define MAXLINE			256
#define LOCAL_AI_PLAYER	-1
#define PLAYER_STATE_RATE	5
#define DEFAULT_PORT 		6000
#define MAXCLIENTS	    	10
#define BATTALION_NETWORK_PROTOCOL 2

/* These wire tokens are frozen for compatibility with the UDP protocol. */
#define MESG_OBJECT_DESTROYED	"bld"
#define MESG_PLAYER_STATE	"upd"
#define MESG_JOIN		"req"
#define MESG_HEARTBEAT		"Iliv"
#define MESG_REACQUIRE		"yo"
#define MESG_ACKNOWLEDGE	"ack"
#define MESG_ACTOR_SPAWN	"newTank"
#define MESG_ACTOR_STATE	"updateTank"
#define MESG_RELAY		"client"

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <pwd.h>

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <signal.h>

#ifdef SGIVERSION
#include <bstring.h>
#include <malloc.h>
#endif

#ifdef SOLARIS
/* from Johan Hagman <Johan.Hagman@mailbox.swipnet.se> */
# include <sys/stat.h>
# include <sys/file.h>

/* BSD compatibility */
# define bzero(dst,len) memset(dst, 0, len)
#endif

#if defined(HPVERSION) || defined(SGIVERSION)
/* to get non-blocking I/O under HP-UX and strict-ANSI IRIX builds */
/* from Neil Harkin <T.N.Harkin@csc.liv.ac.uk> */
#ifndef FNDELAY
#define FNDELAY O_NDELAY
#endif
#endif



#ifdef MACOSVERSION
# include "renderer/fixed_gl.h"
#else
# include <GL/gl.h>
# include <GL/glu.h>
# include <GL/glx.h>
#endif

#include "tk.h"
#include "entity_behavior.h"
#include "world_coordinates.h"
#include "world_environment.h"


#include <time.h>
#include <sys/time.h>


/* added for the OpenGL version */
#ifndef TRUE
# define TRUE 1
#endif
#ifndef FALSE
# define FALSE 0
#endif

#define ENGLISH 0
#define JAPANESE 1

#define MAXTARGETS	    3

#define amalloc(x,y)	malloc(x)
#define afree(x,y)	free(x)
#define acalloc(n,x,y)	calloc(n,x)


#define MONSTERFOGSTART		2.5
#define MONSTERFOGEND		6

#define ARMYFOGSTART		4
#define ARMYFOGEND		12

#define OVERVIEWFOGSTART	6
#define OVERVIEWFOGEND		12



/**********************/
/* misc stuff         */
/**********************/

#ifndef PI
	#define PI	    3.14159265
#endif

#define TWOPI		    6.2831853

#ifdef MESAVERSION

    #define	CYLINDERSIDES	    6
    #define	MORECYLINDERSIDES   12

#else

    #define	CYLINDERSIDES	    12
    #define	MORECYLINDERSIDES   20

#endif

#define CYLINDER_EPSILON    0.0001
#define NEARZERO	    0.0001
#define BIGNUM		    99999

#define ARMCOUNT	    8
#define NULLROAD	    10

#define MAXSTRING	    256

/**********************/
/* game stuff         */
/**********************/

#ifdef MESAVERSION

    #define MAXTRIBUTES	    25

#else

    #define MAXTRIBUTES	    45

#endif

#define ARRIVALRATE	    62
#define STARTACCURACY	    200
#define STARTFIRINGDELAY    43

#define GRAVITY		    0.01
#define BEAMKILL	    0.31

#define BLASTR		    0.5
#define BLASTRX2	    1
#define BLASTRX04	    0.2
#define BLASTRX05	    0.25
#define BLASTRX06	    0.3
#define BLASTRX07	    0.35
#define BLASTRX075	    0.375

#define ACTORV		    0.03
#define MAXLIFE		    100
#define BLASTPOINTS	    2

/**********************/
/* battlefield stuff  */
/**********************/

#ifdef MESAVERSION

    #define PLANESIZE	    6

#else

    #define PLANESIZE	    8

#endif

				    /* height of the battlefield */
#define PLANEY		    -1
				    /* PLANEY + 0.003 */
#define GROUND_OVERLAY_Y    -0.997
				    /* PLANEY + 0.001 */

/**********************/
/* views              */
/**********************/

#define OMNISCIENTVIEW	    0
#define PLAYERVIEW	    1
#define ARMYVIEW	    2
#define MAPVIEW		    4
#define FIRSTPERSONVIEW     5

#define NOACTORONPLANE	    -9999

/**********************/
/* modes              */
/**********************/

#define DEMOMODE	    0
#define PLAYMODE	    1

/**********************/
/* sounds             */
/**********************/

#define MUSIC		    8

/* #define MAXMUSIC 36 */
#define MAXMUSIC 5
#define MAXDEADMUSIC 5

#define MAXSOUNDS 25

extern int shadowsEnabled;
extern struct WorldEnvironment worldEnvironment;

/**********************/
/*limits              */
/**********************/

#define MAXPATH		    400
/**********************/
/* conversions        */
/**********************/

#define BIG_DEG_TO_RAD	    0.0017453292
			    /* (2 x pi) / 3600 */
#define RAD_TO_BIG_DEG	    572.9577951
			    /* 360 / (2 x pi) */
#define RAD_TO_SMALL_DEG    57.29577951
			    /* 3600 / (2 x pi) */
#define DEG_TO_RAD	    0.0174532925
			    /* Pi / 180 */

#define  computeAngle(x, y, z, w) (atan2( (z - w) ,  (x - y)) + PI)

#define  randy(x) (((rand() % 2001) - 1000) * x * 0.001)


struct runtimeProjectile
	{
	struct runtimeProjectile * next;   /* pointer to next projectile */
	struct runtimePlayer *owner;
	float x, y, z;		    /* location of projectile */
	float xv, yv, zv;	    /* velocity of projectile */
	char count;		    /* time after launch before arming */
	const struct EntityArchetype *archetype;
	};

struct runtimeExplosion
	{
	struct runtimeExplosion * next;	    /* pointer to next explosion */
	struct runtimePlayer *owner;
	float x, y, z;		    /* location of explosion */
	int count;		    /* explosion timer */
	int damaging;		    /* visual-only bursts do not alter gameplay */
	const struct EntityArchetype *archetype;
	};

struct BattalionMapObject;

struct runtimeMapObject
	{
	struct runtimeMapObject *next;
	float x, y, z;		    /* location of structure */
	float rad,  theta;
	float height;
	int deathCount;
	int death2;
	int number;
	char intact;
	float heading;
	float instanceScale;
	const struct EntityArchetype *dataArchetype;
	const struct BattalionMapObject *dataObject;
	};

struct runtimeActor
	{
	struct runtimeActor * next;
	struct runtimePlayer *targetPlayer;
	float x, y, z;		    /* location of vehicle */
	float xv, yv, zv;	    /* velocity of vehicle / wreck fragment */
	float range;
	float minrange;
	float finalR;
	float rad, theta;
	float aimYaw, aimPitch;
	float speed;
	float rand1, rand2, rand3;
	int count;
	int weaponActiveFrames;
	int damage;
	int walking;
	int walkingBackwards;
	int debrisGrounded;
	int debrisFromAir;
	int debrisFireDelay;
	float debrisSize;
	int number;
	char subtype;		    /* sub type (for hero) */
	const struct EntityArchetype *dataArchetype;
	const struct EntityArchetype *visualArchetype;
	const struct BattalionMapObject *dataObject;
	char dataId[ENTITY_NAME_SIZE];
	char dataGroup[ENTITY_NAME_SIZE];
	struct EntityAgent dataAgent;
	};

struct runtimePlayer
	{
	const struct EntityArchetype *dataArchetype;
	float a[MAXTRIBUTES][5];    /* reserved by the fixed network record */
	float x, y, z;              /* position in map-world coordinates */
	float energyRemaining;	    /* amount of energy remaining */
	float rot1,rot2;
	float speed;		    /* movement speed */
	float xspeed;		    /* turning rate */
	float moveCost;		    /* energy loss from moving */
	float height;		    /* height of player */
	float width;		    /* width of player */
	float bottom;		    /* bottom of player */
	float regenRate;	    /* rate of energy gain */
	int moveCount;		    /* for leg and wing rotation */
	int playerId;
	int headHorzRotate;	    /* horizontal head angle */
	int headVertRotate;	    /* vertical head angle */
	int score;
	int deadCount;
	int timeDead;		    /* time player has been dead */
	int dead;
	int beamOn;		    /* is the energy weapon on */
	int movingForward;
	int movingBackward;
	int moving;
	};

struct targetInfo
	{
	struct runtimePlayer player;
	struct targetInfo * next;
	float x,z;
	float net_energyRemaining;
	float net_x,  net_z;
	long net_clientFrom;
	int net_headHorzRotate;
	int net_headVertRotate;
	int net_beamOn;
	int lastUpdateFrame;
	};

struct enemy
	{
	float r;		    /* range to enemy */
	float y;		    /* height of enemy */
	float ang;		    /* angle to enemy */
	};

struct score
    	{
	int number;
	char name[256];
    	};

/***************************
 * platform window toolkit
 ***************************/

void tkCloseWindow(void);
void tkExec(void);
void tkGetMouseLoc(int *x, int *y);
void tkInitPosition(int x, int y, int width, int height);
void tkInitDisplayMode(GLenum type);
GLenum tkInitWindow(char *title);
void tkSetRGBMap(int size, float *rgb);
void tkSwapBuffers(void);
void unGrabPointer();
void grabPointer();

/***************************
 * update.c
 * holds the update routines
 ***************************/

void showError(char *);
struct runtimePlayer resetPlayerState(struct runtimePlayer);

int same(char *, char *);
int different(char *, char *);


float centerText(char *,  float);

void updateMapObjects(struct runtimeMapObject *, int);
void updateBeam(struct runtimePlayer *, struct targetInfo *,
		struct runtimeActor *, struct runtimeMapObject *, struct runtimePlayer *,
		const struct EntityArchetype *, const float [3],
		const float [3]);
void updateActors(struct runtimeActor *, float, float, float, int, struct runtimeActor *,
		float, int, int, struct runtimePlayer *, struct targetInfo *);
void updateWrecks(struct runtimeActor *);
void updateRuntimeExplosions(struct runtimeExplosion *, struct runtimeActor *);
int actorHit(const struct runtimeActor *, float, float, float);


/****************************************************
 * audio.c
 * holds audio routines for battalion
 ****************************************************/

void turnSoundOff();
void turnSoundOn();
void toggleSound();
int getSoundOn();
void turnMusicOff();
void turnMusicOn();
void toggleMusic();
int getMusicOn();

void checkSound(char *);
void initSounds(void);
void flushSounds(void);
void soundKiller(int);
void soundKillerKey(const char *);
void doSound(int);
void doSoundKeyAt(const char *, float, float, float);
void audioSetListener(float, float, float, float);
void audioSetSmoothingEnabled(int);
int audioGetSmoothingEnabled(void);
void InitAudio(const char *, const char *, int);
void OutAudio(int);


/****************************************************
 * battalion.c
 * holds everything that hasnt been put somewhere else
 ****************************************************/

void goToHighDetail();
void goToLowDetail();

void updateScores(char *, int, int, char *);


int checkForBlock(struct runtimeActor *, float rad, float theta, float y);

void addScore(int, struct runtimePlayer *);
void spawnProjectile(float x, float y, float z,
		   const struct EntityArchetype *archetype,
		   float xv, float yv, float zv, float accuracy,
		   struct runtimePlayer *);
const struct EntityArchetype *gameEntityArchetype(const char *id);
const struct FactionCatalog *runtimeFactionCatalog(void);
const char *runtimePlayerFaction(void);
void updateRuntimeProjectiles(void);
void updateMapObjects(struct runtimeMapObject *, int);
void updateActors(struct runtimeActor *, float, float, float, int,
		struct runtimeActor*, float, int, int,
		struct runtimePlayer *, struct targetInfo *);
void updateWrecks(struct runtimeActor *);
void updateRuntimeExplosions(struct runtimeExplosion *, struct runtimeActor *);

void setPlayConditions(void);
void addRandomTarget(void);

void initialization(void);

void goto3d(void);
void goto1d(void);

void checkMouse(void);
void checkInput (void);
GLenum MouseDown(int, int, GLenum);
GLenum MouseUp(int, int, GLenum);
void demoKeys(int);
void playKeys(int);
GLenum processKey(int, GLenum);
GLenum processKeyRelease(int);
void checkInput (void);

void updateDeadPlayer(float, float, struct runtimePlayer * );
void updateTargets(void);

void doUpdate (void);

void goToPlayerView(int);
void goToFirstPersonView(int);
void goToArmyView(int);
void goToMapView(void);
void goToOverView(int);

void drawPlayerEntity(float, struct runtimePlayer, float, int, int);

void doDrawing (int);

struct runtimePlayer autopilot(float, float, struct runtimePlayer);

void reshape( int, int);
static void doDisplay(void);
void id(void);

void showCommands(char *);

/*******************************
 * net.c
 * holds the network primitives
 *******************************/

void turnBeamOnSinceLast(void);
void setUnconnected(void);
void setPortNumber(int);
void setHostAddr(char *);

void addNewNetworkActor(char *, struct runtimeActor *, void *, int);
void updateNetworkActor(char *, struct runtimeActor *);
void tellAboutAllActors(struct sockaddr_in *, int, struct runtimeActor *);

void updateRelayedPlayer(char *, void *);


int setUpClient(void);
int setUpNetwork(void);
void updateNetworkMapObjects(char *, struct runtimeMapObject *);
void updateNetworkPlayers(char *, long, void *);
int sendMessage(char *, struct sockaddr_in *, int);
void processClient(time_t, int, int, struct runtimeMapObject *, struct runtimeActor *,
    void * arena, int sizeActor, struct runtimePlayer, int, int *);
void tellAboutDestroyedMapObjects(struct sockaddr_in *, int, int, int *);
void processNetwork(time_t, int, int, struct runtimeMapObject *, struct runtimeActor *,
    void *, struct runtimePlayer, int, int, int *, int *);
void addNetworkPlayer(long, const struct EntityArchetype *, float, float,
    float, float, float, int, void *);

GLuint makePlayerLookat(void);
GLuint makeOverviewLookat(void);

/*****************************************
 * text.c
 * holds the textual overlay routines
 *****************************************/

void initFonts(const char *, const char *);
void showText(struct targetInfo *, float, int, long, int, int, int, float,
	float, float);
void showText2(long, int, int, int, int, int, int, int, int, int);
void showText3(int);
void showText4(void);

void doOptions(struct runtimePlayer, long, int, int, float, int);
void doSummary(int, long, int, int, int, int);

void showScores(int, struct score *, struct runtimePlayer, int, float, int);

/*****************************************
 * graphics.c
 * holds the higher level graphics routines
 *****************************************/

void makeObjects(char *);
const float *assetPaletteColor(const char *);
const char *assetFilePath(const char *);
GLuint assetModelDisplayList(const char *);
int assetModelHasNode(const char *, const char *);
int assetAnimationAvailable(const char *);
int assetEntityCatalogAvailable(const struct EntityCatalog *, char *, size_t);
int assetModelUnscaledHeight(const struct EntityArchetype *, float *);
int assetModelUnscaledVerticalBounds(const struct EntityArchetype *,
                                     float *, float *);
int assetModelUnscaledBounds(const struct EntityArchetype *,
                             float [3], float [3]);
void drawConfiguredPlayerEntity(const struct EntityArchetype *, int, int, int,
				int, float, float);
int configuredPlayerEntityBounds(const struct EntityArchetype *, int, int, int,
				 int, float, float, float [3], float [3]);
int configuredEntityMuzzlePose(const struct EntityArchetype *, int, int,
			       int, float, float, float [3], float [3]);
const struct EntityArchetype *selectedPlayerArchetype(void);
int playerWeaponUsesReload(void);
float playerWeaponReadiness(void);
size_t playerEntityOptionCount(void);
size_t selectedPlayerArchetypeNumber(void);
const struct EntityArchetype *playerEntityOptionAt(size_t ordinal);
const struct EntityArchetype *runtimePlayerArchetype(
    const struct runtimePlayer *player);

void drawEnvironmentSky(const struct WorldEnvironment *);
void drawEnvironmentShadow(float, float, float, float, float,
                           const struct WorldEnvironment *);

void drawWorldGround(float, float, int, int, int);

void drawMapObjects(const struct runtimeMapObject *, int, int, int, int,
                    float, float);

void drawRuntimeDebris(struct runtimeActor *, float, float, int, int);
void drawRuntimeActors(struct runtimeActor *, int, struct runtimePlayer,
                       int, int);
void drawRuntimeExplosions(struct runtimeExplosion *, int);
void drawRuntimeBeam(const struct EntityArchetype *, int,
                     const float [3], const float [3]);

void drawRuntimeProjectiles(struct runtimeProjectile *, int, int, int,
                            const struct runtimePlayer *, int);
