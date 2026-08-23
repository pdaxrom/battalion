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
/* text.c v 1.3                                                        */
/* routines to display all the textual overlays for battalion          */
/***********************************************************************/

#include "battalion.h"
#include "asset_values.h"
#include "dynamic_primitives.h"

extern GLuint titleObj;
extern GLuint titleObjOO;
extern GLuint titleObjOOO;

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
	    float weaponReadiness, float viewerX, float viewerZ)
    {
    float pixtoSpace;
    struct targetInfo * temptarget;
    float drop;
    float energyBar[4][3];
    const float *energyColor;
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
	energyColor = assetPaletteColor("health-green");
    else if (energy > (MAXLIFE / 10))
	energyColor = assetPaletteColor("brown");
    else
	energyColor = assetPaletteColor("red");

    energyBar[2][0] = energyBar[3][0] = energyLeft +
	energy / MAXLIFE * (energyRight - energyLeft);

    glPushAttrib(GL_POLYGON_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    dynamicDrawSolidQuad3(energyBar, energyColor);


    energyBar[2][0] = energyBar[3][0] = energyRight;


    dynamicDrawLineLoop3(energyBar, assetPaletteColor("white"));

    if (weaponUsesReload)
	{
	uiRectangle("weapon-bar", weaponBar);
	weaponLeft = weaponBar[0][0];
	weaponRight = weaponBar[2][0];
	weaponBar[2][0] = weaponBar[3][0] = weaponLeft +
	    weaponReadiness * (weaponRight - weaponLeft);
	dynamicDrawSolidQuad3(weaponBar,
	    weaponReadiness >= 1.0f ? assetPaletteColor("health-green") : assetPaletteColor("brown"));
	weaponBar[2][0] = weaponBar[3][0] = weaponRight;
	dynamicDrawLineLoop3(weaponBar, assetPaletteColor("white"));
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
	    if ((fabs(temptarget->x - viewerX) <= PLANESIZE) &&
	        (fabs(temptarget->z - viewerZ) <= PLANESIZE))
	    {
	    drop -=0.6;

	    glPushMatrix();
		glTranslatef(0, drop, 0);

		energy = temptarget->player.energyRemaining;

		if (energy > (MAXLIFE / 3))
		    energyColor = assetPaletteColor("health-green");
		else if (energy > (MAXLIFE / 10))
		    energyColor = assetPaletteColor("brown");
		else
		    energyColor = assetPaletteColor("red");

		energyBar[2][0] = energyBar[3][0] = energyLeft +
		    energy / MAXLIFE * (energyRight - energyLeft);

		dynamicDrawSolidQuad3(energyBar, energyColor);

		energyBar[2][0] = energyBar[3][0] = energyRight;

		dynamicDrawLineLoop3(energyBar, assetPaletteColor("white"));

	    glPopMatrix();

	    glPushMatrix();
		glTranslatef(-3.8,  4.4+drop,  0);

		{
		const struct EntityArchetype *targetArchetype;
		targetArchetype = runtimePlayerArchetype(&temptarget->player);
		if (targetArchetype != NULL)
		    tkDrawStr(strokeBase,
		              targetArchetype->displayName[0] ?
		              (char *)targetArchetype->displayName :
		              (char *)targetArchetype->id);
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

void showScores(int itsChristmas, struct score *scores,
                struct runtimePlayer player, int counter,
                float offsetX, int detail)
    {
    const struct EntityArchetype *previewArchetype;
    char textString[256];
    float scoreBox[4][3];
    float rowY;
    int row;

    (void)itsChristmas;
    (void)player;
    (void)offsetX;
    uiRectangle("score-box", scoreBox);
    glPushAttrib(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    if (detail > 0)
        glEnable(GL_BLEND);
    dynamicDrawSolidQuad3(scoreBox, assetPaletteColor("brown"));
    glDisable(GL_BLEND);
    glColor3fv(assetPaletteColor("white"));
    glPushMatrix();
    glTranslatef(-2.0f, 1.4f, 0.0f);
    tkDrawStr(strokeBase, "High Scores");
    glPopMatrix();
    for (row = 0; row < 3; row++)
        {
        if (scores[row].number >= 0)
            sprintf(textString, "%5d %s", scores[row].number,
                    scores[row].name);
        else
            strcpy(textString, "  none");
        rowY = 0.5f - row * 0.55f;
        glPushMatrix();
        glTranslatef(-4.0f, rowY, 0.0f);
        tkDrawStr(strokeBase, textString);
        glPopMatrix();
        }
    previewArchetype = selectedPlayerArchetype();
    if (detail != -1 && previewArchetype != NULL)
        {
        glEnable(GL_DEPTH_TEST);
        glPushMatrix();
        glTranslatef(1.6f, -0.65f, 7.2f);
        glRotatef((float)(counter % 360), 0.0f, 1.0f, 0.0f);
        glScalef(0.35f, 0.35f, 0.35f);
        drawConfiguredPlayerEntity(previewArchetype, 1, 0, 0, counter,
                                   0.0f, 0.0f);
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
static    char views1English[]	= "-Player";
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
static    char shadowsEnglish[]		= "-Shadows";

    glPushAttrib(GL_DEPTH_BUFFER_BIT);

    pixtoSpace = 3.0/ winX;

    glDisable(GL_DEPTH_TEST);


    if (itsChristmas)
	glColor3fv(assetPaletteColor("plane-green"));
    else
	glColor3fv(assetPaletteColor("yellow"));

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
	drawHelpString("F5");
    glPopMatrix();
    glPushMatrix();
	glTranslatef(-3.6,  -5,  0);
	drawHelpString(shadowsEnglish);
    glPopMatrix();
    glPushMatrix();
	glTranslatef(-1.9,  -5,  0);
	shadowsEnabled ? drawHelpString(onstring) : drawHelpString(offstring);
    glPopMatrix();

    glPushMatrix();
	glTranslatef(-4.8,  -5.5,  0);
	drawHelpString("F1");
	glPopMatrix();
    glPushMatrix();
	glTranslatef(-3.6,  -5.5,  0);
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

    glColor3fv(assetPaletteColor("white"));

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

    glColor3fv(assetPaletteColor("white"));

    glPushMatrix();
	glTranslatef(-3.85,  2.3,  0);
	tkDrawStr(strokeBase, spaceEnglish);
    glPopMatrix();
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* draw option menu for choosing player                         */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void doOptions(struct runtimePlayer player, long xWin, int counter,
		int itsChristmas, float offsetX, int detail)
    {
    const struct EntityArchetype *archetype;
    const struct AssetValue *preview;
    GLfloat lineWidthNow;
    float previewMinimum[3];
    float previewMaximum[3];
    float previewCenter[3];
    float previewExtent;
    float previewScale;
    float optionsBox[4][3];
    int component;
    size_t optionCount;
    char optionLine[ENTITY_NAME_SIZE + 48];

    (void)player;
    (void)xWin;
    (void)itsChristmas;
    (void)offsetX;

    uiRectangle("options-box", optionsBox);
    preview = requiredUiValue("options-preview", 4);
    archetype = selectedPlayerArchetype();
    optionCount = playerEntityOptionCount();

    glPushAttrib(GL_DEPTH_BUFFER_BIT);

    /*************************************/
    /* draw transparent brown background */
    /*************************************/

    glDisable(GL_DEPTH_TEST);

    if (detail <= 0)
	{
	dynamicDrawSolidQuad3(optionsBox, assetPaletteColor("brown"));
	}
    else
	{
	glEnable(GL_BLEND);
	dynamicDrawSolidQuad3(optionsBox, assetPaletteColor("brown"));

	glDisable(GL_BLEND);
	}


    /*************/
    /* draw text */
    /*************/

    glColor3fv(assetPaletteColor("white"));

    glPushMatrix();
	glTranslatef(-3.15,  1.75,  0);
	tkDrawStr(strokeBase, "Choose player entity:");
    glPopMatrix();

    glPushMatrix();
	glTranslatef(-3.4,  0.85,  0);
	if (archetype != NULL)
	    {
	    if (archetype->shortcut != 0)
		sprintf(optionLine, "%s [%c]  %lu/%lu",
		        archetype->displayName[0] != '\0' ?
		            archetype->displayName : archetype->id,
		        archetype->shortcut,
		        (unsigned long)(selectedPlayerArchetypeNumber() + 1),
		        (unsigned long)optionCount);
	    else
		sprintf(optionLine, "%s  %lu/%lu",
		        archetype->displayName[0] != '\0' ?
		            archetype->displayName : archetype->id,
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
	    if (configuredPlayerEntityBounds(
		            archetype, 1, 0, 0, counter, 0.0f, 0.0f,
	            previewMinimum, previewMaximum))
		{
		previewExtent = 0.0f;
		for (component = 0; component < 3; component++)
		    {
		    previewCenter[component] =
		        (previewMinimum[component] +
		         previewMaximum[component]) * 0.5f;
		    if (previewMaximum[component] - previewMinimum[component] >
		        previewExtent)
		        previewExtent = previewMaximum[component] -
		                        previewMinimum[component];
		    }
		previewScale = previewExtent > 0.0f ?
		    preview->components[3] / previewExtent : 1.0f;
		glTranslatef(preview->components[0], preview->components[1],
		             preview->components[2]);
		glRotatef((float)(counter % 360), 0, 1, 0);
		glScalef(previewScale, previewScale, previewScale);
		glTranslatef(-previewCenter[0], -previewCenter[1],
		             -previewCenter[2]);
		}
	    else
		{
		glTranslatef(preview->components[0], preview->components[1],
		             preview->components[2]);
		glScalef(0.7f, 0.7f, 0.7f);
		}
	    drawConfiguredPlayerEntity(archetype, 1, 0, 0, counter,
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

void doSummary(int deadTime, long winX, int score,
               int counter, int detail, int moreThanOne)
    {
    const struct EntityArchetype *archetype;
    char summary[160];
    float summaryBox[4][3];

    (void)winX;
    archetype = selectedPlayerArchetype();
    if (moreThanOne)
        sprintf(summary, "Players score: %d", score);
    else if (archetype != NULL)
        sprintf(summary, "%s score: %d",
                archetype->displayName[0] ? archetype->displayName :
                archetype->id, score);
    else
        sprintf(summary, "Score: %d", score);
    uiRectangle("summary-box", summaryBox);
    glPushAttrib(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    if (detail > 0) glEnable(GL_BLEND);
    dynamicDrawSolidQuad3(summaryBox, assetPaletteColor("brown"));
    glDisable(GL_BLEND);
    glColor3fv(assetPaletteColor("white"));
    glPushMatrix();
    glTranslatef(-4.25f, 0.5f, 0.0f);
    tkDrawStr(strokeBase, summary);
    glPopMatrix();
    if (detail != -1 && archetype != NULL)
        {
        glEnable(GL_DEPTH_TEST);
        glPushMatrix();
        glTranslatef(0.0f, -0.45f, 7.1f);
        glRotatef((float)((deadTime * 10) % 3600) * 0.1f,
                  0.0f, 1.0f, 0.0f);
        glScalef(0.35f, 0.35f, 0.35f);
        drawConfiguredPlayerEntity(archetype, 1, 0, 0, counter,
                                   0.0f, 0.0f);
        glPopMatrix();
        }
    glPopAttrib();
    }
