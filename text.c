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
/* text.c v 1.3                                                        */
/* routines to display all the textual overlays for battalion          */
/***********************************************************************/

#include "battalion.h"
#include "asset_values.h"
#include "dynamic_primitives.h"

    /*************/
    /* colours   */
    /*************/

extern float colorolive[4];
extern float  planeGreen[4];
extern float  planeGreen2[4];
extern float  planeGreen3[4];
extern float  colorgreen[4];

extern float  colorblack[4];

extern float  colorred[4];
extern float  colorred_50[4];
extern float  colorred2[4];
extern float  colorred3[4];

extern float  colormaserblue[4];
extern float  colormaserblue2[4];

extern float  colorwhite[4];
extern float  colorwhite2[4];

extern float  colorsnow[4];
extern float  colorsnow2[4];

extern float  colorgrey1[4];
extern float  colorgrey1_50[4];
extern float  colorgrey2[4];
extern float  colorgrey3[4];
extern float  colorgrey3_50[4];
extern float  colorgrey4[4];

extern float  colorbrown[4];
extern float  colorbeige[4];

extern float  colorblue[4];
extern float  colorwater[4];

extern float  coloryellow[4];
extern float  colormaseryellow[4];
extern float  colororange[4];

extern float  healthGreen[4];

extern float  treeColor[4];
extern float  treeColor2[4];



extern GLuint titleObj;
extern GLuint titleObjOO;
extern GLuint titleObjOOO;

extern GLuint plaintank;
extern GLuint masertank;  
extern GLuint launchertank;

    GLuint strokeBase;
    char textline[80];
    static struct AssetValues externalUiValues;

static const struct AssetValue *requiredUiValue(const char *key,
                                                size_t components)
    {
    const struct AssetValue *value;

    value = assetValuesFind(&externalUiValues, key);
    if (value == NULL || value->componentCount != components)
        {
        fprintf(stderr, "Required UI value is missing or invalid: %s\n", key);
        showError("Required UI value is missing from assets");
        exit(1);
        }
    return value;
    }

static float uiScalar(const char *key)
    {
    return requiredUiValue(key, 1)->components[0];
    }

static void uiRectangle(const char *key, float vertices[4][3])
    {
    const struct AssetValue *value;
    float left;
    float bottom;
    float right;
    float top;
    float depth;

    value = requiredUiValue(key, 5);
    left = value->components[0];
    bottom = value->components[1];
    right = value->components[2];
    top = value->components[3];
    depth = value->components[4];
    vertices[0][0] = left;
    vertices[0][1] = top;
    vertices[0][2] = depth;
    vertices[1][0] = left;
    vertices[1][1] = bottom;
    vertices[1][2] = depth;
    vertices[2][0] = right;
    vertices[2][1] = bottom;
    vertices[2][2] = depth;
    vertices[3][0] = right;
    vertices[3][1] = top;
    vertices[3][2] = depth;
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void initFonts(const char *fontPath, const char *uiPath)
    {
    char error[160];

    assetValuesInitialize(&externalUiValues);
    if (!assetValuesLoad(uiPath, &externalUiValues, error, sizeof(error)))
	{
	fprintf(stderr, "Could not load external UI values: %s\n", error);
	showError("Can't load UI values");
	exit(1);
	}
    strokeBase = glGenLists(256);
    if (tkCreateStrokeFont(strokeBase, fontPath) == GL_FALSE)
	{
	showError("Can't create font");
	exit(1);
	}
    }


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw status display (energy and score)                        */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void showText(struct targetInfo * targets, float energy, int score,
	    long winX, int paused, int grabbed, int weaponUsesReload,
	    float weaponReadiness)
    {
    float pixtoSpace;
    struct targetInfo * temptarget;
    float drop;
    float energyBar[4][3];
    float *energyColor;
    float energyLeft;
    float energyRight;
    float weaponBar[4][3];
    float weaponLeft;
    float weaponRight;

static    char energyEnglish[]		= "energy:";
static    char pausedEnglish[]		= "<p>aused";
static    char grabbedEnglish[]		= "<g>rabbed";
static    char readyEnglish[]		= "ready";
static    char reloadEnglish[]		= "reload";

    uiRectangle("energy-bar", energyBar);
    energyLeft = energyBar[0][0];
    energyRight = energyBar[2][0];

    glPushAttrib(GL_DEPTH_BUFFER_BIT);
    
    glDisable(GL_DEPTH_TEST);
         
    pixtoSpace = 10.0 / winX;

    /* draw the energy bar */


    if (energy > (MAXLIFE / 3))
	energyColor = healthGreen;
    else if (energy > (MAXLIFE / 10))
	energyColor = colorbrown;
    else
	energyColor = colorred;
	
    energyBar[2][0] = energyBar[3][0] = energyLeft +
	energy / MAXLIFE * (energyRight - energyLeft);

    glPushAttrib(GL_POLYGON_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); 

    dynamicDrawSolidQuad3(energyBar, energyColor);


    energyBar[2][0] = energyBar[3][0] = energyRight;


    dynamicDrawLineLoop3(energyBar, colorwhite);

    if (weaponUsesReload)
	{
	uiRectangle("weapon-bar", weaponBar);
	weaponLeft = weaponBar[0][0];
	weaponRight = weaponBar[2][0];
	weaponBar[2][0] = weaponBar[3][0] = weaponLeft +
	    weaponReadiness * (weaponRight - weaponLeft);
	dynamicDrawSolidQuad3(weaponBar,
	    weaponReadiness >= 1.0f ? healthGreen : colorbrown);
	weaponBar[2][0] = weaponBar[3][0] = weaponRight;
	dynamicDrawLineLoop3(weaponBar, colorwhite);
	glPushMatrix();
	    glTranslatef(weaponLeft + 0.18f,
		weaponBar[1][1] + 0.04f, 0);
	    glScalef(0.65f, 0.65f, 1.0f);
	    tkDrawStr(strokeBase, weaponReadiness >= 1.0f ?
		readyEnglish : reloadEnglish);
	glPopMatrix();
	}

    /************************************************/
    /* draw energy bar for any target on the screen */
    /************************************************/

    if (targets->next != NULL)
	{
	drop = 0;
	for(temptarget = targets->next; temptarget != NULL; temptarget = temptarget->next)
	    if ((fabs(temptarget->x) <= PLANESIZE) && (fabs(temptarget->z) <= PLANESIZE))
	    {
	    drop -=0.6;
    
	    glPushMatrix();
		glTranslatef(0, drop, 0);
    
		energy = temptarget->monster.energyRemaining;
    
		if (energy > (MAXLIFE / 3))
		    energyColor = healthGreen;
		else if (energy > (MAXLIFE / 10))
		    energyColor = colorbrown;
		else
		    energyColor = colorred;
		    
		energyBar[2][0] = energyBar[3][0] = energyLeft +
		    energy / MAXLIFE * (energyRight - energyLeft);
	    
		dynamicDrawSolidQuad3(energyBar, energyColor);
	    
		energyBar[2][0] = energyBar[3][0] = energyRight;
	    
		dynamicDrawLineLoop3(energyBar, colorwhite);
    
	    glPopMatrix();
    
	    glPushMatrix();
		glTranslatef(-3.8,  4.4+drop,  0);
		
		switch(temptarget->monster.monster)
		    {
		    case GOOGELON:  tkDrawStr(strokeBase,"Googelon");
				    break;
		    case TECHS:	    tkDrawStr(strokeBase,"Techs");
				    break;
		    case VAPOUR:    tkDrawStr(strokeBase,"Vapour");
				    break;
		    case FLUTTER:   tkDrawStr(strokeBase,"Flutter");
				    break;
		    case DATAENTITY: if (selectedPlayerArchetype() != NULL)
					tkDrawStr(strokeBase,
					          (char *)selectedPlayerArchetype()->id);
				    break;
		    }
		glPopMatrix();
	    }	
	}
	
    glPopAttrib(); /* polygon drawing */
 
    glPushMatrix();
	glTranslatef(-3.8,  4.45,  0);
	tkDrawStr(strokeBase, energyEnglish);
    glPopMatrix();

    /***********************/
    /* print out the score */
    /***********************/


    sprintf(textline, "score: %1d", score);
    
    glPushMatrix();
	glTranslatef(1.9,  4.45,  0);
	tkDrawStr(strokeBase, textline);
    glPopMatrix();


    /****************************/
    /* print out the frame rate */
    /****************************/

#ifdef DEBUG
    sprintf(textline,  "%d",  winX);

    glPushMatrix();
	glTranslatef(0,  4.4,  0);
	tkDrawStr(strokeBase, textline);
    glPopMatrix();
#endif

    if (paused)
	{
	glPushMatrix();
	    glTranslatef(-0.7,  4.6,  0);	
	    tkDrawStr(strokeBase, pausedEnglish);
	glPopMatrix();

	}
	
    if (grabbed)
	{
	glPushMatrix();
	    glTranslatef(-0.85,  4.2,  0);	
	    tkDrawStr(strokeBase, grabbedEnglish);
	glPopMatrix();

	}
	
    glPopAttrib();
    }   

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void showScores(int itsChristmas, struct score* gscore, struct score* vscore,
		struct score* tscore, struct score* fscore, 
		struct monsterInfo monster, int counter,
		float offsetX, int detail)
    {
    char textString[256];
    struct monsterInfo tempMonster;
    GLfloat lineWidthNow;
    float scoreBox[4][3];

    uiRectangle("score-box", scoreBox);

glPushAttrib(GL_DEPTH_BUFFER_BIT);

    /***********************************/
    /* these monsters are always alive */
    /***********************************/
   
    tempMonster 		= monster;
    tempMonster.energyRemaining = 100;
    tempMonster.monsterIsDead 	= 0;
    tempMonster.deadCount 	= 0;
        	
    /*************************************/
    /* draw transparent brown background */
    /*************************************/

    glDisable(GL_DEPTH_TEST);

    if (detail <= 0)
	{
	dynamicDrawSolidQuad3(scoreBox, colorbrown);
 	}
    else
	{
	glEnable(GL_BLEND);
	dynamicDrawSolidQuad3(scoreBox, colorbrown);
     
	glDisable(GL_BLEND);
	}

    /*****************/
    /* draw monsters */
    /*****************/

    if (detail != -1)
	glEnable(GL_DEPTH_TEST);

    glGetFloatv(GL_LINE_WIDTH, &lineWidthNow);
    glLineWidth(uiScalar("overlay-model-line-width"));

	    
    glPushMatrix();
	glTranslatef(-0.8,  0.14,  7.2);
	glScalef(.2,  .2,  .2);
	drawMonster(tempMonster, counter, itsChristmas, detail);
    glPopMatrix();

    glPushMatrix();
	glTranslatef(-0.8,  -0.23,  7.2);
	glScalef(.2,  .2,  .2);
	drawConfiguredPlayerEntity(networkEntityArchetype(VAPOUR),
	                           1, 0, counter, 0, 0.0f, 0.0f);
    glPopMatrix();
	
    glPushMatrix();
	glTranslatef(+.14,  0.1,  7.2);
	glScalef(.2,  .2,  .2);
	drawTechs(tempMonster, detail);
    glPopMatrix();

    glPushMatrix();
	glTranslatef(+.14,  -0.26,  7.2);
	glScalef(.2,  .2,  .2);
	drawFlutter(tempMonster, counter, itsChristmas, offsetX, OMNISCIENTVIEW, detail);
    glPopMatrix();

    glLineWidth(lineWidthNow);


    /*************/
    /* draw text */
    /*************/

    glColor3fv(colorwhite); 

   glDisable(GL_DEPTH_TEST);

    glPushMatrix();
	glTranslatef(-2,  1.4,  0);
	tkDrawStr(strokeBase, "High Scores");
    glPopMatrix();


    /*************************************/


    if (gscore[0].number > -1)
        {
	sprintf(textString, "%5d %s", gscore[0].number, gscore[0].name); 
	glPushMatrix();
	    glTranslatef(-4,  0.5,  0);
	    tkDrawStr(strokeBase, textString);
	glPopMatrix();
        }
    else
	{
	glPushMatrix();
	    glTranslatef(-4,  0.5,  0);
	    tkDrawStr(strokeBase, "  none");
	glPopMatrix();
	}
    
    if (gscore[1].number > -1)
        {
	sprintf(textString, "%5d %s", gscore[1].number, gscore[1].name); 
	glPushMatrix();
	    glTranslatef(-4,  0,  0);
	    tkDrawStr(strokeBase, textString);
	glPopMatrix();
       }
    
    if (gscore[2].number > -1)
        {
	sprintf(textString, "%5d %s", gscore[2].number, gscore[2].name); 
	glPushMatrix();
	    glTranslatef(-4,  -0.5,  0);
	    tkDrawStr(strokeBase, textString);
	glPopMatrix();
        }

    /*************************************/

    if (vscore[0].number > -1)
        {
	sprintf(textString, "%5d %s", vscore[0].number, vscore[0].name); 
	glPushMatrix();
	    glTranslatef(-4,  -1.5,  0);
	    tkDrawStr(strokeBase, textString);
	glPopMatrix();
	}
    else
	{
	glPushMatrix();
	    glTranslatef(-4,  -1.5,  0);
	    tkDrawStr(strokeBase, "  none");
	glPopMatrix();
	}
	
    if (vscore[1].number > -1)
        {
	sprintf(textString, "%5d %s", vscore[1].number, vscore[1].name); 
	glPushMatrix();
	    glTranslatef(-4,  -2,  0);
	    tkDrawStr(strokeBase, textString);
	glPopMatrix();
	}

    if (vscore[2].number > -1)
        {
	sprintf(textString, "%5d %s", vscore[2].number, vscore[2].name); 
	glPushMatrix();
	    glTranslatef(-4,  -2.5,  0);
	    tkDrawStr(strokeBase, textString);
	glPopMatrix();
	}
	
    /*************************************/

    if (tscore[0].number > -1)
        {
	sprintf(textString, "%5d %s", tscore[0].number, tscore[0].name); 
	glPushMatrix();
	    glTranslatef(0.6,  0.5,  0);
	    tkDrawStr(strokeBase, textString);
	glPopMatrix();
	}
    else
	{
	glPushMatrix();
	    glTranslatef(0.6,  0.5,  0);
	    tkDrawStr(strokeBase, "  none");
	glPopMatrix();
	}
	
   if (tscore[1].number > -1)
        {
	sprintf(textString, "%5d %s", tscore[1].number, tscore[1].name); 
	glPushMatrix();
	    glTranslatef(0.6,  0,  0);
	    tkDrawStr(strokeBase, textString);
	glPopMatrix();
	}
	
   if (tscore[2].number > -1)
        {
	sprintf(textString, "%5d %s", tscore[2].number, tscore[2].name); 
	glPushMatrix();
	    glTranslatef(0.6,  -0.5,  0);
	    tkDrawStr(strokeBase, textString);
	glPopMatrix();
	}
	
    /*************************************/

    if (fscore[0].number > -1)
        {
	sprintf(textString, "%5d %s", fscore[0].number, fscore[0].name); 
	glPushMatrix();
	    glTranslatef(0.6,  -1.5,  0);
	    tkDrawStr(strokeBase, textString);
	glPopMatrix();
	}
    else
	{
	glPushMatrix();
	    glTranslatef(0.6,  -1.5,  0);
	    tkDrawStr(strokeBase, "  none");
	glPopMatrix();
	}

    if (fscore[1].number > -1)
        {
	sprintf(textString, "%5d %s", fscore[1].number, fscore[1].name); 
	glPushMatrix();
	    glTranslatef(0.6,  -2,  0);
	    tkDrawStr(strokeBase, textString);
	glPopMatrix();
	}
	
    if (fscore[2].number > -1)
        {	
	sprintf(textString, "%5d %s", fscore[2].number, fscore[2].name); 
	glPushMatrix();
	    glTranslatef(0.6,  -2.5,  0);
	    tkDrawStr(strokeBase, textString);
	glPopMatrix();
	}

    glPopAttrib();
    }
    
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw list of views/options/controls)                          */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void drawHelpString(char *string)
    {
    glPushMatrix();
    glScalef(0.70, 0.70, 1.0);
    tkDrawStr(strokeBase, string);
    glPopMatrix();
    }

void showText2(long winX, int soundOn, int noSound, int musicOn, 
		int mode3D, int no3D, int detail, int paused,
		int itsChristmas, int ptrGrab)
    {
    float pixtoSpace;
    float right;
    int garbage;

static    char viewsEnglish[]	= "Views:";
static    char views1English[]	= "-Monster";
static    char views2English[]	= "-Overview";
static    char views3English[]	= "-Army";
static    char views4English[]	= "-Map";
static    char views5English[]	= "-First Person";

static    char onstring[]		= " (ON)";
static    char offstring[]		= " (OFF)";
static    char linstring[]		= " (VEC)";
static    char lowstring[]		= " (LOW)";
static    char medstring[]		= " (MED)";
static    char highstring[]		= " (HIGH)";
static    char unavailablestring[] 	= " ( - )";

static    char moveEnglish[]		= "wasd";
static    char aimEnglish[]		= "mouse/ijkl";
static    char shootEnglish[]		= "lmb/ctrl";
static    char mapHeightEnglish[]	= "pgup/dn";

static    char grabEnglish[]		= "-Release Mouse";
/* static    char videoEnglish[]	= "-3D"; */

static    char rotateEnglish[]		= "-Rotate Overview";
static    char arrowsEnglish[]		= "arrows";
static    char mapHeightActionEnglish[] = "-Map Height";
static    char helpEnglish[]		= "-Help";

static    char pauseEnglish[]		= "-Pause";
static    char detailEnglish[]		= "-Detail";
static    char soundEnglish[]		= "-Sound";
static    char musicEnglish[]		= "-Music";

    glPushAttrib(GL_DEPTH_BUFFER_BIT);
    
    pixtoSpace = 3.0/ winX;
          
    glDisable(GL_DEPTH_TEST);


    if (itsChristmas)
	glColor3fv(planeGreen); 
    else
	glColor3fv(coloryellow); 

    /*********/
    /* views */
    /*********/

    glPushMatrix();
	glTranslatef(-5.0,  +0.7,  0);
	drawHelpString(viewsEnglish);
    glPopMatrix();

    glPushMatrix();
	glTranslatef(-4.8,  0.2,  0);
	drawHelpString("1");
	
	glTranslatef(0.5,  0,  0);
	drawHelpString(views1English);
    glPopMatrix();
    
    glPushMatrix();
	glTranslatef(-4.8,  -0.3,  0);
	drawHelpString("2");
	
	glTranslatef(0.5,  0,  0);
	drawHelpString(views2English);
    glPopMatrix();    
    
    glPushMatrix();
	glTranslatef(-4.8,  -0.8,  0);
	drawHelpString("3");
	
	glTranslatef(0.5,  0,  0);
	drawHelpString(views3English);
    glPopMatrix();    

    glPushMatrix();
	glTranslatef(-2.4,  0.2,  0);
	drawHelpString("4");

	glTranslatef(0.5,  0,  0);
	drawHelpString(views4English);
    glPopMatrix();

    glPushMatrix();
	glTranslatef(-2.4,  -0.3,  0);
	drawHelpString("5");

	glTranslatef(0.5,  0,  0);
	drawHelpString(views5English);
    glPopMatrix();



    /************/
    /* controls */
    /************/

    right = 0.0;


    glPushMatrix();
	glTranslatef(right,  -2,  0);
	drawHelpString("Controls:");
    glPopMatrix();

    glPushMatrix();
	glTranslatef(right,  -2.5,  0);
	drawHelpString(moveEnglish);
    glPopMatrix();
    glPushMatrix();
	glTranslatef(right + 2.1,  -2.5,  0);
	drawHelpString("-Move");
    glPopMatrix();
	

    glPushMatrix();
	glTranslatef(right,  -3,  0);
	drawHelpString(aimEnglish);
    glPopMatrix();
    glPushMatrix();
	glTranslatef(right + 2.1,  -3,  0);
	drawHelpString("-Aim");
    glPopMatrix();
   

    glPushMatrix();
	glTranslatef(right,  -3.5,  0);
	drawHelpString(shootEnglish);
    glPopMatrix();
    glPushMatrix();
	glTranslatef(right + 2.1,  -3.5,  0);
	drawHelpString("-Fire");
    glPopMatrix();


    glPushMatrix();
	glTranslatef(right,  -4,  0);
	drawHelpString(arrowsEnglish);
    glPopMatrix();
    glPushMatrix();
	glTranslatef(right+2.1,  -4,  0);
	drawHelpString(rotateEnglish);
    glPopMatrix();

    glPushMatrix();
	glTranslatef(right,  -4.5,  0);
	drawHelpString(mapHeightEnglish);
    glPopMatrix();
    glPushMatrix();
	glTranslatef(right+2.1,  -4.5,  0);
	drawHelpString(mapHeightActionEnglish);
    glPopMatrix();

    /***********/
    /* options */
    /***********/


    glPushMatrix();
	glTranslatef(-5,  -2,  0);
	drawHelpString("Options:");
    glPopMatrix();



    glPushMatrix();
	glTranslatef(-4.8,  -2.5,  0);
	drawHelpString("F2");
	glPopMatrix();
    glPushMatrix();
	glTranslatef(-3.6,  -2.5,  0);
	drawHelpString(detailEnglish);
    glPopMatrix();
    glPushMatrix();
	glTranslatef(-1.9,  -2.5,  0);
	switch (detail) {
	case -1:
	    drawHelpString(linstring);
	    break;
	case 0:
	    drawHelpString(lowstring);
	    break;
	case 1:
	    drawHelpString(medstring);
	    break;
	case 2:
	    drawHelpString(highstring);
	    break;
	}
    glPopMatrix();
    
    


    glPushMatrix();
	glTranslatef(-4.8,  -3,  0);
	drawHelpString("F3");
	glPopMatrix();
    glPushMatrix();
	glTranslatef(-3.6,  -3,  0);
	drawHelpString(soundEnglish);
    glPopMatrix();

    glPushMatrix();
	glTranslatef(-1.9,  -3,  0);

	if (noSound)
	    drawHelpString(unavailablestring);
	else
	    soundOn ? drawHelpString(onstring) : drawHelpString(offstring);
    glPopMatrix();


    glPushMatrix();
	glTranslatef(-4.8,  -3.5,  0);
	drawHelpString("F4");
	glPopMatrix();
    glPushMatrix();
	glTranslatef(-3.6,  -3.5,  0);
	drawHelpString(musicEnglish);
    glPopMatrix();

    glPushMatrix();
	glTranslatef(-1.9,  -3.5,  0);

	if (noSound)
	    drawHelpString(unavailablestring);
	else
	    musicOn ? drawHelpString(onstring) : drawHelpString(offstring);
    glPopMatrix();
	
	
    glPushMatrix();
	glTranslatef(-4.8,  -4,  0);
	drawHelpString("p");
	glPopMatrix();
    glPushMatrix();
	glTranslatef(-3.6,  -4,  0);
	drawHelpString(pauseEnglish);
    glPopMatrix();

    glPushMatrix();
	glTranslatef(-1.9,  -4,  0);
	paused ? drawHelpString(onstring) : drawHelpString(offstring);
    glPopMatrix();


    glPushMatrix();
	glTranslatef(-4.8,  -4.5,  0);
	drawHelpString("esc");
	glPopMatrix();
    glPushMatrix();
	glTranslatef(-3.6,  -4.5,  0);
	drawHelpString(grabEnglish);
    glPopMatrix();

    garbage = no3D + ptrGrab + mode3D;
/* someday the 3D from the Original GL version will return in
 * the OpenGL version ... but not yet

    glPushMatrix();
	glTranslatef(-4.8,  -4.5,  0);
	tkDrawStr(strokeBase, "v");

	glTranslatef(0.2,  0,  0);
	tkDrawStr(strokeBase, videoEnglish);
    glPopMatrix();

    glPushMatrix();
	glTranslatef(-2.65, -4.5,  0);

	if (no3D)
	    tkDrawStr(strokeBase,unavailablestring);
	else
	    mode3D ? tkDrawStr(strokeBase,onstring) : tkDrawStr(strokeBase,offstring);

    glPopMatrix();
*/

    glPushMatrix();
	glTranslatef(-4.8,  -5,  0);
	drawHelpString("F1");
	glPopMatrix();
    glPushMatrix();
	glTranslatef(-3.6,  -5,  0);
	drawHelpString(helpEnglish);
    glPopMatrix();

    glPopAttrib();
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw the 'andy johnson's battalion' title                     */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void showText3(int detail)
    {
static    char andyEnglish[]		= "andy johnson's";
static    char versionEnglish[]		= "1.4b";

    /************************/
    /* print battalion logo */
    /************************/

    if (detail > 0)
	glCallList(titleObj);
    else if (detail == 0)
	glCallList(titleObjOO);
    else if (detail == -1)
	{
	glPushAttrib(GL_POLYGON_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); 
	glCallList(titleObjOOO);
	glPopAttrib();
	}
	
    glColor3fv(colorwhite); 

    /**************************/
    /* print "andy johnsons'" */
    /**************************/

    glPushMatrix();
	glTranslatef(1.6 ,  +3.85,  0);
	glScalef(0.57, 0.83, 1);
	tkDrawStr(strokeBase, andyEnglish);
    glPopMatrix();

    /************************/
    /* print version number */
    /************************/

    glPushMatrix();
	glTranslatef(4.3,  -5.1,  0);
	glScalef(0.75, 0.75, 1);
	tkDrawStr(strokeBase, versionEnglish);
    glPopMatrix();
   }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw 'press spacebar to begin'                                */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void showText4()
    {
static    char spaceEnglish[]		= "Press the spacebar to begin";

    glColor3fv(colorwhite); 

    glPushMatrix();
	glTranslatef(-3.85,  2.3,  0);
	tkDrawStr(strokeBase, spaceEnglish);
    glPopMatrix();
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw option menu for choosing monster                         */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void doOptions(struct monsterInfo monster, long xWin, int counter,
		int itsChristmas, float offsetX, int detail)
    {
    const struct EntityArchetype *archetype;
    GLfloat lineWidthNow;
    float optionsBox[4][3];
    size_t optionCount;
    char optionLine[ENTITY_NAME_SIZE + 48];

    (void)monster;
    (void)xWin;
    (void)itsChristmas;
    (void)offsetX;

    uiRectangle("options-box", optionsBox);
    archetype = selectedPlayerArchetype();
    optionCount = playerEntityOptionCount();

    glPushAttrib(GL_DEPTH_BUFFER_BIT);
        	
    /*************************************/
    /* draw transparent brown background */
    /*************************************/

    glDisable(GL_DEPTH_TEST);

    if (detail <= 0)
	{
	dynamicDrawSolidQuad3(optionsBox, colorbrown);
	}
    else
	{
	glEnable(GL_BLEND);
	dynamicDrawSolidQuad3(optionsBox, colorbrown);
    
	glDisable(GL_BLEND);
	}


    /*************/
    /* draw text */
    /*************/

    glColor3fv(colorwhite); 

    glPushMatrix();
	glTranslatef(-3.15,  1.75,  0);
	tkDrawStr(strokeBase, "Choose player entity:");
    glPopMatrix();

    glPushMatrix();
	glTranslatef(-3.4,  0.85,  0);
	if (archetype != NULL)
	    {
	    if (archetype->shortcut != 0)
		sprintf(optionLine, "%s [%c]  %lu/%lu", archetype->id,
		        archetype->shortcut,
		        (unsigned long)(selectedPlayerArchetypeNumber() + 1),
		        (unsigned long)optionCount);
	    else
		sprintf(optionLine, "%s  %lu/%lu", archetype->id,
		        (unsigned long)(selectedPlayerArchetypeNumber() + 1),
		        (unsigned long)optionCount);
	    tkDrawStr(strokeBase, optionLine);
	    }
    glPopMatrix();

    glPushMatrix();
	glTranslatef(-3.4,  -2.8,  0);
	tkDrawStr(strokeBase, "arrows - select");
    glPopMatrix();

    glPushMatrix();
	glTranslatef(-3.4,  -3.45,  0);
	tkDrawStr(strokeBase, "enter - start");
    glPopMatrix();


    /*****************/
    /* draw monsters */
    /*****************/

    if (detail != -1)
	glEnable(GL_DEPTH_TEST);

    glGetFloatv(GL_LINE_WIDTH, &lineWidthNow);
    glLineWidth(uiScalar("overlay-model-line-width"));
	    
    if (archetype != NULL)
	{
	glPushMatrix();
	    glTranslatef(0, -0.35f, 7.2f);
	    glRotatef((float)(counter % 360), 0, 1, 0);
	    glScalef(0.7f, 0.7f, 0.7f);
	    drawConfiguredPlayerEntity(archetype, 1, 0, counter, 0,
	                               0.0f, 0.0f);
	glPopMatrix();
	}

    glPopAttrib();

    glLineWidth(lineWidthNow);
   }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw summary of kills                                         */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void doSummary(int monster,  int deadTime, long winX, int killtanks,
		int killmtanks, int killhelos,
		int killCHHs, int killmechags,
		int killheros, int killplanes, int killlaunchers,
		int killfighters,
		int counter, int detail, int moreThanOne)
    {    
    int angle;
    float pixtoSpace;
    char resultString[15];
    char monsterString[40];
    GLfloat lineWidthNow;
    float summaryBox[4][3];

    uiRectangle("summary-box", summaryBox);

    glPushAttrib(GL_DEPTH_BUFFER_BIT);

    pixtoSpace = 2.0 / winX;

    angle = (deadTime * 10) % 3600;

    glDisable(GL_DEPTH_TEST);

    if (detail <= 0)
	{
	dynamicDrawSolidQuad3(summaryBox, colorbrown);
	}
    else
	{
	glEnable(GL_BLEND);
	dynamicDrawSolidQuad3(summaryBox, colorbrown);
    
	glDisable(GL_BLEND);
	}
    

    glColor3fv(colorwhite); 

    if ((killtanks > 0) || (killlaunchers > 0) || (killmtanks > 0) || (killhelos > 0) ||
	(killmechags > 0) || (killfighters > 0) || (killCHHs > 0) || (killheros > 0) || (killplanes > 0))
	    strcpy(resultString, ":");
    else
	    strcpy(resultString, " nothing.");

    if (moreThanOne)    
	{
	 sprintf(monsterString, "The monsters destroyed %s", resultString);
	}
    else
	{
	switch(monster){
	    case GOOGELON:  sprintf(monsterString, "Googelon destroyed %s", resultString);
			    break;
	    case VAPOUR:    sprintf(monsterString, "The Vapour destroyed %s", resultString);
			    break;
	    case TECHS:     sprintf(monsterString, "Techs destroyed %s", resultString);
			    break;
	    case FLUTTER:   sprintf(monsterString, "Flutter destroyed %s", resultString);
			    break;
	    case DATAENTITY: if (selectedPlayerArchetype() != NULL)
				sprintf(monsterString, "%s destroyed %s",
				        selectedPlayerArchetype()->id,
				        resultString);
			    else
				strcpy(monsterString, "Entity destroyed nothing.");
			    break;
	    default:	    showError("Bogus Monster! (doSummary)");
			    break;
	    }
	}

    glPushMatrix();
	glTranslatef(-4.25, .5,  0);
	tkDrawStr(strokeBase, monsterString);
    glPopMatrix();


    if (killfighters > 0)
	{
    glPushMatrix();
	glTranslatef(-2.9-1.3,  -2.3,  0);
	if (killfighters > 1)
		sprintf(textline,  "%1u Fighters", killfighters);
	else
		sprintf(textline,  "%1u Fighter", killfighters);
	tkDrawStr(strokeBase, textline);
    glPopMatrix();
	}

    if (killtanks > 0)
	{
    glPushMatrix();
	glTranslatef(-3-1.1,  -1,  0);
	if (killtanks > 1)
		sprintf(textline,  "%1u Tanks", killtanks);
	else
		sprintf(textline,  "%1u Tank", killtanks);
	tkDrawStr(strokeBase, textline);
    glPopMatrix();
	}

    if (killlaunchers > 0)
	{
    glPushMatrix();
	glTranslatef(2.6-1.6,  -2.3,  0);
	if (killlaunchers > 1)
		sprintf(textline,  "%1u Launchers", killlaunchers);
	else
		sprintf(textline,  "%1u Launcher", killlaunchers);
	tkDrawStr(strokeBase, textline);
    glPopMatrix();
	}
	
    if (killhelos > 0)
	{
    glPushMatrix();
	glTranslatef(3.1-1.0,  -1,  0);
	if (killhelos > 1)
		sprintf(textline,  "%1u Helos", killhelos);
	else
		sprintf(textline,  "%1u Helo", killhelos);
	tkDrawStr(strokeBase, textline);
    glPopMatrix();
	}

    if (killmtanks > 0)
	{
    glPushMatrix();
    	glTranslatef(-0.2-1.2, -2.8,  0);
	if (killmtanks > 1)
		sprintf(textline,  "%1u Masers", killmtanks);
	else
		sprintf(textline,  "%1u Maser", killmtanks);
	tkDrawStr(strokeBase, textline);
    glPopMatrix();
	}


    if (killplanes > 0)
	{
    glPushMatrix();
	glTranslatef(0-1.4,  -1,  0);
	if (killplanes > 1)
		sprintf(textline,  "%1u Bombers", killplanes);
	else
		sprintf(textline,  "%1u Bomber", killplanes);
	tkDrawStr(strokeBase, textline);
    glPopMatrix();
	}
	
	
    if (killmechags > 0)
	{
    glPushMatrix();
	glTranslatef(2.8-1.4,  -4.7,  0);
	if (killmechags > 1)
		sprintf(textline,  "%1u MechaGs", killmechags);
	else
		sprintf(textline,  "%1u MechaG", killmechags);
	tkDrawStr(strokeBase, textline);
    glPopMatrix();
	}
	
    if (killCHHs > 0)
	{
    glPushMatrix();
	glTranslatef(0-1.3,  -4.7,  0);
	if (killCHHs > 1)
		sprintf(textline,  "%1u Hunters", killCHHs);
	else
		sprintf(textline,  "%1u Hunter", killCHHs);
	tkDrawStr(strokeBase, textline);
    glPopMatrix();
	}

    if (killheros > 0)
	{
    glPushMatrix();
	glTranslatef(-3-1.0,  -4.7,  0);
	if (killheros > 1)
		sprintf(textline,  "%1u Heros", killheros);
	else
		sprintf(textline,  "%1u Hero", killheros);
	tkDrawStr(strokeBase, textline);
    glPopMatrix();
	}

    if (detail != -1)
	glEnable(GL_DEPTH_TEST);

    glGetFloatv(GL_LINE_WIDTH, &lineWidthNow);
    glLineWidth(uiScalar("overlay-model-line-width"));

    if (killfighters > 0)
	{
	glPushMatrix();
	    glTranslatef(-0.65,  -.3,  7.1);
	    glRotatef(.1*angle + 180, 0, 1, 0);
	    glScalef(.6,  .6,  .6);
	    drawFighter();
	glPopMatrix();
	}

    if (killtanks > 0)
	{
	glPushMatrix();
	    glTranslatef(-.65,  -.1,  7.1);
	    glRotatef(.1*angle + 270, 0, 1, 0);
	    glScalef(0.33,  0.33,  0.33);
	    glCallList(plaintank);
	glPopMatrix();
	}

    if (killlaunchers > 0)
	{
	glPushMatrix();
	    glTranslatef(.5,  -.35,  7.1);
	    glRotatef(.1*angle + 270, 0, 1, 0);
	    glScalef(0.33,  0.33,  0.33);
	    glCallList(launchertank);
	glPopMatrix();
	}
 	
    if (killmtanks > 0)
	{
	glPushMatrix();
	    glTranslatef(-0.1,  -.42,  7.1);
	    glRotatef(.1*angle + 45, 0, 1, 0);
	    glScalef(0.05,  0.05,  0.05);
	    glCallList(masertank);
	glPopMatrix();
	}
 
    if (killhelos > 0)
	{
	glPushMatrix();
	    glTranslatef(.65,  0,  7.1);
	    glRotatef(.1*angle + 180, 0, 1, 0);
	    glScalef(.3,  .3,  .3);
	    drawHelo((float)counter,  0);
	glPopMatrix();
	}

    if (killplanes > 0)
	{
	glPushMatrix();
	    glTranslatef(0,  0,  7.1);
	    glRotatef(.1*angle, 0, 1, 0);
	    glScalef(.75,  .75,  .75);
	    drawAirplane(counter);
	glPopMatrix();
	}

    if (killmechags > 0)
	{
	glPushMatrix();
	    glTranslatef(0.6,  -.6,  7.1);
	    glRotatef(.1*angle + 90, 0, 1, 0);
	    glScalef(.25,  .25,  .25);
	    drawMechaMonster(50, counter, detail);
	glPopMatrix();
	}

    if (killCHHs > 0)
	{
	glPushMatrix();
	    glTranslatef(-0.1,  -.8,  7.1);
	    glRotatef(.1*angle + 180, 0, 1, 0);
	    glScalef(.3,  .3,  .3);
	    makeCHH(20);
	glPopMatrix();
	}

    if (killheros > 0)
	{
	glPushMatrix();
	    glTranslatef(-.6,  -.61,  7.1);
	    glRotatef(.1*angle + 180, 0, 1, 0);
	    glScalef(.25,  .25,  .25);
	    drawHeroDude2(100, counter, 0, 0, detail);
	glPopMatrix();
	}

    glPopAttrib();

    glLineWidth(lineWidthNow);
    }
