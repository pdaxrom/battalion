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
/* update.c  v 1.2                                                     */
/* update routines for battalion                                       */
/***********************************************************************/

#include "battalion.h"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* output an error message                                       */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void showError(char * theString)
    {
    fprintf(stderr, "BATTALION ERROR: %s\n", theString);
    fflush(stderr);
    }

int same(char * first,  char * second)
    {
    if (strcmp(first, second) == 0)
	return(1);
    else
	return(0);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int different(char * first,  char * second)
    {
    if (strcmp(first, second) != 0)
	return(1);
    else
	return(0);
    }
    

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

struct runtimePlayer resetPlayerState(struct runtimePlayer playerEntityState)
    {
    const struct EntityArchetype *playerEntity;

    /************************************/
    /* reset general player parmeters  */
    /************************************/

    playerEntityState.score	= 0;
    playerEntityState.dead	= 0;
    playerEntityState.deadCount		= 0;
    playerEntityState.timeDead		= 0;

    playerEntityState.energyRemaining	= MAXLIFE;

    playerEntityState.moveCount		= 0;

    playerEntityState.headHorzRotate	= 0;
    playerEntityState.headVertRotate	= 150;
    playerEntityState.beamOn		= 0;

    playerEntityState.playerId		= rand();

    /*************************************/
    /* reset specific player parameters */
    /*************************************/

    playerEntity = playerEntityState.dataArchetype;
    if (playerEntity == NULL)
	playerEntity = selectedPlayerArchetype();
    if (playerEntity == NULL)
	{
	showError("No entity archetype for player type");
	return playerEntityState;
	}
    playerEntityState.dataArchetype = playerEntity;
    playerEntityState.headVertRotate =
	(int)(playerEntity->aimPitchDefault * 10.0f);
    playerEntityState.x = 0.0f;
    playerEntityState.z = 0.0f;
    playerEntityState.y = PLANEY + playerEntity->verticalOffset;
    playerEntityState.energyRemaining = playerEntity->playerEnergy;
    playerEntityState.speed = entityArchetypePlayerSpeed(playerEntity);
    playerEntityState.xspeed = playerEntity->playerTurnRate;
    playerEntityState.moveCost = playerEntity->playerMoveCost;
    playerEntityState.height = PLANEY + playerEntity->playerHeight;
    playerEntityState.width = playerEntity->playerWidth;
    playerEntityState.bottom = PLANEY + playerEntity->playerBottom;
    playerEntityState.regenRate = playerEntity->playerRegen;
    
    return(playerEntityState);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* active entity is firing its energy beam                  */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void beamDamageSource(struct EntityContact *contact,
	struct runtimePlayer *player, struct targetInfo *targetList,
	struct runtimePlayer *owner, float x, float y, float z)
    {
    struct targetInfo *target;
    int number;

    memset(contact, 0, sizeof(*contact));
    strcpy(contact->faction,
           owner != NULL ? runtimePlayerFaction() : "neutral");
    contact->x = x;
    contact->y = y;
    contact->z = z;
    contact->active = 1;
    contact->visible = 1;
    contact->audible = 1;
    if (owner == player)
	{
	strcpy(contact->id, "player");
	return;
	}
    number = 0;
    for (target = targetList->next; target != NULL; target = target->next)
	{
	if (owner == &target->player)
	    {
	    sprintf(contact->id, "target-%d", number);
	    return;
	    }
	number++;
	}
    strcpy(contact->id, "beam-source");
    }

void updateBeam(struct runtimePlayer * g, struct targetInfo * t,
		struct runtimeActor * allActors,
		struct runtimeMapObject * allMapObjects, struct runtimePlayer * m,
		const struct EntityArchetype *archetype,
		const float muzzleOrigin[3], const float muzzleDirection[3])
    {
    struct targetInfo * temptarget;
    struct runtimeActor * tactor, * tempactor;
    struct runtimeMapObject * object;
    float loc1[3],  loc3[3];
    float ractorhorz,  ractorvert;
    float aactorhorz,  aactorvert;
    float abeamhorz,  abeamvert;
    float rbeamhorz,  rbeamvert;
    float hdiff,  vdiff;
    float actorx,  actory,  actorz;
    float beamx,  beamy,  beamz;
    float height;
    float beamRange;
    float R;
    const struct EntityArchetype *impactProjectile;
    struct EntityContact damageSource;
    int itsDead;
    if (archetype == NULL || muzzleOrigin == NULL ||
		muzzleDirection == NULL || allMapObjects == NULL)
	return;
    beamRange = archetype->weaponRange;

    doSoundKeyAt(archetype->fireSound, muzzleOrigin[0], muzzleOrigin[1],
	         muzzleOrigin[2]);

    R = beamRange;
    loc1[0] = muzzleOrigin[0];
	loc1[1] = muzzleOrigin[1];
	loc1[2] = muzzleOrigin[2];
    worldClippedRayEndpoint(loc1, muzzleDirection, R, PLANEY, loc3);

    /**********************************/
    /* does beam touch ground         */
    /**********************************/

    if (loc3[1] <= PLANEY + 0.1)
	{
	impactProjectile = gameEntityArchetype(archetype->impactProjectile);
	if (impactProjectile != NULL)
	    spawnProjectile(loc3[0], PLANEY + impactProjectile->verticalOffset,
	                    loc3[2], impactProjectile,
	                    0.0f, -impactProjectile->projectileSpeed,
	                    0.0f, 0.0f, NULL);
	}
	
    /******************************/
    /* what does beam hit         */
    /******************************/
    
    if ((loc3[2] - loc1[2]) == 0)
	loc1[2] += NEARZERO;
	    
    rbeamhorz = fabs(sqrt(pow(loc3[0] - loc1[0], 2) + pow(loc3[2] - loc1[2], 2)));
    abeamhorz = atan((loc3[0] - loc1[0]) / (loc3[2] - loc1[2]));

    if ((loc3[2] - loc1[2]) < 0)
	abeamhorz += PI;

    hdiff = - abeamhorz;

    beamx = 0;		/* rbeamhorz * sin(abeamhorz + hdiff); */
    beamz = rbeamhorz;	/* rbeamhorz * cos(abeamhorz + hdiff); */
    
    if (beamz == 0)
	beamz = NEARZERO;

    rbeamvert = fabs(sqrt(pow(loc3[1] - loc1[1], 2) + beamz*beamz));
    abeamvert = atan( (loc3[1] - loc1[1]) / beamz);

    vdiff = -abeamvert;
    beamy = 0;		/* rbeamvert * sin(abeamvert + vdiff); */
    beamz = rbeamvert;	/* rbeamvert * cos(abeamvert + vdiff); */
    
    if (beamz < 0)
	abeamvert += PI;

    /**********************************/
    /* does beam touch a actor         */
    /**********************************/
	    
    tempactor	= allActors;

    while(tempactor->next != NULL)
	{
	tactor = tempactor->next;

	if ((tactor->z - loc1[2]) == 0)
	    loc1[2] += NEARZERO; 

	ractorhorz = fabs(sqrt((pow(tactor->x - loc1[0], 2) + pow(tactor->z - loc1[2], 2))));
	aactorhorz = atan( (tactor->x - loc1[0]) / (tactor->z - loc1[2]));

	if ((tactor->z - loc1[2]) < 0)
	    aactorhorz += PI;
    
	actorx = ractorhorz * sin(aactorhorz + hdiff);
	actorz = ractorhorz * cos(aactorhorz + hdiff);

	if (actorz == 0)
	    actorz = NEARZERO;

	if (tactor->dataArchetype == NULL)
	    {
	    tempactor = tempactor->next;
	    continue;
	    }
	ractorvert = fabs(sqrt(pow(tactor->y +
	    tactor->dataArchetype->collisionHeight * 0.5f - loc1[1], 2) +
	    actorz * actorz));
	aactorvert = atan((tactor->y +
	    tactor->dataArchetype->collisionHeight * 0.5f - loc1[1]) / actorz);

	if (actorz < 0)
	    aactorvert += PI;

	actory = ractorvert * sin(aactorvert + vdiff);
	actorz = ractorvert * cos(aactorvert + vdiff);

	itsDead = 0;

	if (tactor->dataArchetype != NULL &&
	    worldSegmentIntersectsCylinder(
		loc1[0], loc1[1], loc1[2], loc3[0], loc3[1], loc3[2],
		tactor->x, tactor->z,
		tactor->dataArchetype->collisionRadius,
		tactor->y,
		tactor->y + tactor->dataArchetype->collisionHeight))
	    itsDead = 1;

	if (itsDead && archetype->weaponDamagesActors)
	    {
	    tactor->damage -= archetype->weaponDamage;
	    if (tactor->dataArchetype != NULL)
		{
		beamDamageSource(&damageSource, g, t, m,
			muzzleOrigin[0], muzzleOrigin[1], muzzleOrigin[2]);
		entityAgentNotifyDamage(&tactor->dataAgent, &damageSource);
		}

	    if (tactor->damage <= 0)
		{
		if (m != NULL && tactor->dataArchetype != NULL)
		    m->score += tactor->dataArchetype->score;
		tactor->damage = -99;
		}		
	    }
	    
	tempactor = tempactor->next;
	}

/***********************/
/* check other targets */
/***********************/

    temptarget = t->next;

    while(temptarget != NULL)
	{
	if (temptarget->player.playerId != m->playerId)
	    {
	    if ((temptarget->z - loc1[2]) == 0)
		loc1[2] += NEARZERO; 
    
	    ractorhorz = fabs(sqrt((pow(temptarget->x - loc1[0], 2) + pow(temptarget->z - loc1[2], 2))));
	    aactorhorz = atan( (temptarget->x - loc1[0]) / (temptarget->z - loc1[2]));
    
	    if ((temptarget->z - loc1[2]) < 0)
		aactorhorz += PI;
	
	    actorx = ractorhorz * sin(aactorhorz + hdiff);
	    actorz = ractorhorz * cos(aactorhorz + hdiff);
    
	    if (actorz == 0)
		actorz = NEARZERO;
    
	    ractorvert = fabs(sqrt(pow(((temptarget->player.height * 0.5 + temptarget->player.bottom * 0.5) - loc1[1]), 2) + actorz*actorz));
	    aactorvert = atan( ((temptarget->player.height * 0.5 + temptarget->player.bottom * 0.5) - loc1[1]) / actorz);


    
	    if (actorz < 0)
		aactorvert += PI;
    
	    actory = ractorvert * sin(aactorvert + vdiff);
	    actorz = ractorvert * cos(aactorvert + vdiff);
    
	    itsDead = 0;
    
	    if ((actorz >= 0) && (actorz <= (beamz + 0.2)) &&
		(fabs(actorx) < temptarget->player.width) &&
		(fabs(actory) < (temptarget->player.height -
		                    temptarget->player.bottom) * 0.5f))
		itsDead = 1;
    
	    if (itsDead && archetype->weaponDamagesActors)
		{
		temptarget->player.energyRemaining -= archetype->weaponDamage;
		}
		
	    }
	    temptarget = temptarget->next;
	}


/**************************/
/* check player's player */
/**************************/

	if (g != m)
	    {
	    if ((g->z - loc1[2]) == 0)
		loc1[2] += NEARZERO; 
    
	    ractorhorz = fabs(sqrt(pow(g->x - loc1[0], 2) +
	                           pow(g->z - loc1[2], 2)));
	    aactorhorz = atan((g->x - loc1[0]) / (g->z - loc1[2]));
    
	    if ((g->z - loc1[2]) < 0)
		aactorhorz += PI;
	
	    actorx = ractorhorz * sin(aactorhorz + hdiff);
	    actorz = ractorhorz * cos(aactorhorz + hdiff);
    
	    if (actorz == 0)
		actorz = NEARZERO;
    
	    ractorvert = fabs(sqrt(pow(((g->height * 0.5 + g->bottom * 0.5) - loc1[1]), 2) + actorz*actorz));
	    aactorvert = atan( ((g->height * 0.5 + g->bottom * 0.5) - loc1[1]) / actorz);


    
	    if (actorz < 0)
		aactorvert += PI;
    
	    actory = ractorvert * sin(aactorvert + vdiff);
	    actorz = ractorvert * cos(aactorvert + vdiff);
    
	    itsDead = 0;
    
	    if ((actorz >= 0) && (actorz <= (beamz + 0.2)) &&
		(fabs(actorx) < g->width) &&
		(fabs(actory) < (g->height - g->bottom) * 0.5f))
		itsDead = 1;
    
	    if (itsDead && archetype->weaponDamagesActors)
		{
		g->energyRemaining -= archetype->weaponDamage;
		}
		
	    }





	
        /**********************************/
        /* does beam touch a map object   */
        /**********************************/

    for (object = allMapObjects->next; object != NULL; object = object->next)
	{
	if (!object->intact || object->dataArchetype == NULL ||
	    object->dataArchetype->collisionMode == ENTITY_COLLISION_NONE)
	    continue;
	if ((object->z - loc1[2]) == 0)
	    loc1[2] += NEARZERO; 

	ractorhorz = fabs(sqrt(pow(object->x - loc1[0], 2) + pow(object->z - loc1[2], 2)));
	aactorhorz = atan( (object->x - loc1[0]) / (object->z - loc1[2]));

	if ((object->z - loc1[2]) < 0)
	    aactorhorz += PI;
    
	actorx = ractorhorz * sin(aactorhorz + hdiff);
	actorz = ractorhorz * cos(aactorhorz + hdiff);
	
	height = object->height;
	
	if (actorz == 0)
	    actorz = NEARZERO;

	ractorvert = fabs(sqrt(pow(object->y + height - loc1[1], 2) + actorz * actorz));
	aactorvert = atan( (object->y + height - loc1[1]) / actorz);

	if (actorz < 0)
	    aactorvert += PI;

	actory = ractorvert * sin(aactorvert + vdiff);
	actorz = ractorvert * cos(aactorvert + vdiff);

	if ((actorz >= 0) && (actorz <= (beamz + 0.2)) &&
	    object->dataArchetype != NULL &&
	    (fabs(actorx) < object->dataArchetype->collisionRadius *
	                     object->instanceScale) &&
	    (fabs(actory) < height))
		{
		if (archetype->weaponDestroysMapObjects && object->intact &&
		    object->dataArchetype->destructible)
		    {
		    if (m != NULL)
			m->score += object->dataArchetype->score;
		    object->intact = 0;
		    }
		}
	}
    }


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* decide whether a vehicle has been hit                         */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int actorHit(const struct runtimeActor *tactor,
             float xloc, float yloc, float zloc)
    {
    float blastx, blasty, blastz;

    if (tactor->dataArchetype == NULL ||
        tactor->dataArchetype->collisionMode == ENTITY_COLLISION_NONE)
        return 0;
    blastx = blastz = entityArchetypeCollisionRadius(
        tactor->dataArchetype);
    blasty = tactor->dataArchetype->collisionHeight;
		
    if ((fabs(tactor->x - xloc) < blastx) &&
	(fabs(tactor->z - zloc) < blastz) &&
	(fabs(tactor->y - yloc) < blasty))
	    return(1);
    else
	    return(0);
    }
