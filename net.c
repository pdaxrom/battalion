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
/* net.c  v 1.3                                                        */
/* network routines for battalion                                      */
/***********************************************************************/

/* the network coding is pretty much alpha-stage stuff. It seems
 * to generally work though the player positions are slightly off
 * in the remote versions making it hard to hit the other person.
 *
 * the phased archetype also has a problem getting kills on the other monsters
 *
 * ocassionally wehn switching from demo to game mode the screen goes
 * black and everything stops
 */
#include "battalion.h"

/*****************/
/* network stuff */
/*****************/

struct sockaddr_in serv_addr;
struct sockaddr_in clients[MAXCLIENTS];
struct sockaddr_in cli_addr;

long clientsFrom[MAXCLIENTS];
int lastClientPlayerStateFrame[MAXCLIENTS];

time_t clientsAlive[MAXCLIENTS];
time_t lastRecvTime;
time_t lastNotDeadYetTime;
int lastPlayerStateFrame;

int numClients;
int sockfd;
int portNumber;
int connected;

int waitCounter	    = 0;
int beamOnSinceLast = 0;
int actorModulator   = 0;

char hostAddr[40];

extern struct targetInfo * targets;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void turnBeamOnSinceLast()
    {
    beamOnSinceLast = 1;
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void setUnconnected()
    {
    connected = 0;
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void setPortNumber(int val)
    {
    portNumber = val;
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void setHostAddr(char * val)
    {
    strcpy(hostAddr, val);
    }
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int HostnameToIP(char *hostname, char *hostIP)
    {
    struct hostent* host;

    host = gethostbyname(hostname);

    if (host)
	{
	sprintf(hostIP,"%d.%d.%d.%d",(int) host->h_addr_list[0][0],
	(int) host->h_addr_list[0][1],
	(int) host->h_addr_list[0][2],
	(int) host->h_addr_list[0][3]);
	}
    else
	return(0);

    return (1);
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int setUpClient()
    {
    char hostIP[40];

    lastPlayerStateFrame = -1;
    lastRecvTime = lastNotDeadYetTime = 0;

    /************************/
    /* Open datagram socket */
    /************************/

    if ((sockfd = socket(AF_INET, SOCK_DGRAM,0)) < 0)
	{
	printf("battalion client cannot open datagram socket\n");
	return(0);
        }

    /*****************************/
    /* Prepare client to connect */
    /*****************************/

    bzero((char *) &cli_addr, sizeof(cli_addr));
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    cli_addr.sin_port = htons(0);

    if (bind(sockfd, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0)
	{
	printf("battalion client cannot bind local address\n");
	return(0);
	}

    /****************************************************************/
    /* Set up data structure to indicate which server to connect to */
    /****************************************************************/

    if ((hostAddr[0] >= '0') && (hostAddr[0] <= '9'))
	{
	strcpy (hostIP, hostAddr);
	}
    else
	{
	if(!HostnameToIP(hostAddr, hostIP))
	    {
	    strcpy (hostIP, hostAddr);
	    }
	}

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(hostIP);
    serv_addr.sin_port = htons(portNumber);

    printf("battalion client running - looking for server on %s (%s) on port %d\n\n", hostAddr, hostIP, portNumber);

    /*******************/
    /* set nonblocking */
    /*******************/

    fcntl(sockfd,F_SETFL,FNDELAY);


    return(1);
    }


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int setUpNetwork()
    {
    int i;
    char host[MAXHOSTNAMELEN];
    char hostIP[MAXHOSTNAMELEN];


    /************************/
    /* Open datagram socket */
    /************************/

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
	printf("Server cannot open datagram socket\n");
	return(0);
	}

    /******************************************************/
    /* Set up server to allow connection from any address */
    /******************************************************/

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);


    serv_addr.sin_port = htons(portNumber);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
	printf("battalion server cannot bind to local address\n");
	return(0);
	}

    gethostname(host,MAXHOSTNAMELEN);
    HostnameToIP(host, hostIP);

    printf("battalion server running on %s (%s) on port %d\n\n", host, hostIP, portNumber);

    /*******************/
    /* set nonblocking */
    /*******************/

    fcntl(sockfd,F_SETFL,FNDELAY);

    /******************/
    /* no clients yet */
    /******************/

    numClients = 0;

    for (i=0; i<MAXCLIENTS;i++)
	{
	clientsAlive[i] = 0;
	lastClientPlayerStateFrame[i] = -1;
	}

    return(1);
    }

 /*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void updateNetworkMapObjects(char * recvline, struct runtimeMapObject * mapObjectList)
    {
    char command[MAXLINE];
    char mesg[MAXLINE];
    int mapObjectNumber;
    int i;
    struct runtimeMapObject * mapObjectNode;

    /*****************************************************/
    /* find out which building was destroyed by a client */
    /*****************************************************/

    sscanf(recvline, "%s %d", command, &mapObjectNumber);

    for (mapObjectNode = mapObjectList->next; mapObjectNode != NULL; mapObjectNode = mapObjectNode->next)
	if (mapObjectNode->number == mapObjectNumber)
	    mapObjectNode->intact = 0;

    /***********************************************************************/
    /* tell all the clients about the building destroyed by another client */
    /***********************************************************************/

    sprintf(mesg, "%s %d #", MESG_OBJECT_DESTROYED, mapObjectNumber);

    for(i=0; i<numClients; i++)
	sendMessage(mesg, &clients[i], sizeof(clients[i]));

    }

 /*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void addNewNetworkActor(char * recvline, struct runtimeActor * actorlist,
    void * arena, int sizeActor)
    {
    char command[MAXLINE];
    char archetypeId[ENTITY_NAME_SIZE];
    float x, z;
    int damage, number, weaponActiveFrames;
    float heading;
    const struct EntityArchetype *archetype;
    struct runtimeActor * tempactor;
    int foundIt;

    weaponActiveFrames = 0;
    archetypeId[0] = 0;
    sscanf(recvline, "%255s %d %f %f %f %d %d %63s",
	command, &number, &x, &z, &heading, &damage,
	&weaponActiveFrames, archetypeId);


       foundIt = 0;
    for (tempactor = actorlist->next; tempactor != NULL; tempactor = tempactor->next)
	{
	if (number == tempactor->number)
	    foundIt = 1;
	}

    if (!foundIt)
	{
	    archetype = gameEntityArchetype(archetypeId);
	    if (!entityArchetypeIsActor(archetype))
		return;
	    tempactor = (struct runtimeActor *) amalloc(sizeActor, arena);
	    if (tempactor == NULL)
		return;
	    memset(tempactor, 0, sizeof(*tempactor));

		tempactor->number	= number;
		tempactor->x		= x;
		tempactor->y		= PLANEY + archetype->verticalOffset;
		tempactor->z		= z;
		tempactor->range		= archetype->weaponRange;
		tempactor->minrange	= archetype->weaponMinRange;
		tempactor->finalR	= archetype->weaponRange;
		tempactor->theta		= heading;
		tempactor->damage	= damage;
		tempactor->speed		= archetype->speed;
		tempactor->weaponActiveFrames	= weaponActiveFrames;
		tempactor->dataArchetype = archetype;
		sprintf(tempactor->dataId, "network-%d", number);
		entityAgentInitialize(&tempactor->dataAgent, archetype,
		                      x, z, heading, NULL, 0,
		                      ENTITY_PATROL_LOOP,
		                      runtimeFactionCatalog(), archetype->faction);
		tempactor->dataAgent.active = 0;

	tempactor->next	= actorlist->next;
	actorlist->next	= tempactor;
	}
    }

 /*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void updateNetworkActor(char * recvline, struct runtimeActor * actorlist)
    {
    char command[MAXLINE];
    float x, z;
    int number, weaponActiveFrames;
    struct runtimeActor * tempactor, * foundactor;
    int foundIt;

    sscanf(recvline, "%s %d %f %f %d",
	command, &number, &x, &z, &weaponActiveFrames);

/* this should have somethin that uses the monsters current location to modify the
 * location of the actor
 */

    foundIt = 0;
    for (tempactor = actorlist->next; (tempactor != NULL) && (!foundIt); tempactor = tempactor->next)
	{
	if (number == tempactor->number)
	    {
	    foundIt = 1;
	    foundactor = tempactor;
	    }
	}

    if (foundIt)
	{
/*
	fprintf(stderr, "%s \n", recvline);
	fprintf(stderr, "UPDATING ACTOR %d %f %f %f %d \n", number, x, y, z, weaponActiveFrames);
*/
	foundactor->x	= x;
	foundactor->z	= z;

	if (weaponActiveFrames > 1)
	    foundactor->weaponActiveFrames = 10;
	}
    }


  /*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void tellAboutAllActors(struct sockaddr_in * cli, int length
    , struct runtimeActor * actorlist)
    {
    char mesg[MAXLINE];

    struct runtimeActor * tempActor;

    for (tempActor = actorlist->next; tempActor != NULL; tempActor = tempActor->next)
	{
	if (tempActor->dataArchetype == NULL)
	    continue;
	sprintf(mesg, "%s %d %.3f %.3f %.6f %d %d %s #", MESG_ACTOR_SPAWN,
							 tempActor->number,
							 tempActor->x,
							 tempActor->z,
							 tempActor->theta,
							 tempActor->damage,
							 tempActor->weaponActiveFrames,
							 tempActor->dataArchetype->id
							 );

	sendMessage(mesg, cli, length);
	}
    }

 /*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void updateNetworkPlayers(char * recvline, long clientFrom, void * arena)
    {
    struct targetInfo * tempTarget,  *thaOne;

    float n_energy, n_x, n_z;
    int n_frame;
    int n_dead, n_beamOn;
    int n_horzrot, n_vertrot, n_timeDead;
    int exists;
    char command[MAXLINE];
    char archetypeId[ENTITY_NAME_SIZE];
    const struct EntityArchetype *archetype;

    if (sscanf(recvline, "%255s %d %63s %f %d %d %d %f %f %d %d",
	    command,
	    &n_frame,
	    archetypeId,
	    &n_energy,
	    &n_horzrot,
	    &n_vertrot,
	    &n_dead,
	    &n_x,
	    &n_z,
	    &n_beamOn,
	    &n_timeDead) != 11)
	return;
    archetype = gameEntityArchetype(archetypeId);
    if (archetype == NULL || !archetype->controllable)
	return;

    exists = 0;
    for(tempTarget = targets; tempTarget != NULL; tempTarget=tempTarget->next)
	{
	if (tempTarget->net_clientFrom == clientFrom)
	    {
	    exists = 1;
	    thaOne = tempTarget;
	    }
	}

    if (exists && thaOne->player.dataArchetype == archetype)
	{
	/*****************************************************************/
	/* update the values in the currently existing networked player */
	/*****************************************************************/

	thaOne->net_headHorzRotate	= n_horzrot;
	thaOne->net_headVertRotate	= n_vertrot;
	thaOne->net_energyRemaining	= n_energy;
	thaOne->net_x			= n_x;
	thaOne->net_z			= n_z;
	thaOne->net_beamOn		= n_beamOn;
	}
    else if (n_timeDead < 40)
	{
	/*******************************/
	/* add a new networked player */
	/*******************************/

	addNetworkPlayer(clientFrom, archetype, n_horzrot, n_vertrot,
	    n_energy, n_x, n_z, n_beamOn, arena);
	}
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void addNetworkPlayer(long fromNum, const struct EntityArchetype *archetype,
                        float horz, float vert,
			float energy, float x, float z, int beamOn, void * arena)
    {
    struct runtimePlayer newPlayerState;
    struct targetInfo * nother, *temp;
    int alreadyExists;
    int i;

    if (archetype == NULL || !archetype->controllable)
	return;
    alreadyExists = 0;
    for (i=0, temp=targets->next; temp != NULL; i++,temp=temp->next)
	{
	if (temp->net_clientFrom == fromNum)
	    {
	    alreadyExists = 1;

	    /******************************/
	    /* player type has changed ! */
	    /******************************/

	    memset(&newPlayerState, 0, sizeof(newPlayerState));
	    newPlayerState.dataArchetype = archetype;
	    newPlayerState		= resetPlayerState(newPlayerState);

	    temp->player		= newPlayerState;

	    temp->net_clientFrom	= fromNum;
	    temp->net_headHorzRotate	= horz;
	    temp->net_headVertRotate	= vert;
	    temp->net_energyRemaining	= energy;
	    temp->net_x			= x;
	    temp->net_z			= z;
	    temp->net_beamOn		= beamOn;

	    temp->player.headHorzRotate = horz;
	    temp->player.headVertRotate = vert;
	    temp->x = x;
	    temp->z = z;

	    printf("updating a remote player that has changed its type\n");
	    }
	}

    /************************************/
    /* add a new player into this game */
    /************************************/

    if (!alreadyExists)
	{
	memset(&newPlayerState, 0, sizeof(newPlayerState));
	newPlayerState.dataArchetype = archetype;
	newPlayerState	    = resetPlayerState(newPlayerState);

	nother = (struct targetInfo *) amalloc(sizeof(struct targetInfo), arena);

	if (nother != NULL)
	    {
	    nother->player		= newPlayerState;
	    nother->net_clientFrom	= fromNum;
	    nother->net_headHorzRotate	= horz;
	    nother->net_headVertRotate	= vert;
	    nother->net_energyRemaining = energy;
	    nother->net_x		= x;
	    nother->net_z		= z;
	    nother->net_beamOn		= beamOn;

	    nother->player.headHorzRotate = horz;
	    nother->player.headVertRotate = vert;
	    nother->x = x;
	    nother->z = z;

	    nother->next    = targets->next;
	    targets->next   = nother;
	    }
	}
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void updateRelayedPlayer(char * recvline, void * arena)
    {
    struct targetInfo * tempTarget,  *thaOne;

    float n_energy, n_x, n_z;
    int n_frame;
    int n_dead, n_beamOn;
    int n_horzrot, n_vertrot, n_timeDead;
    int exists;
    long recvfrom;
    char command[MAXLINE];
    char orig_command[MAXLINE];
    char archetypeId[ENTITY_NAME_SIZE];
    const struct EntityArchetype *archetype;

    if (sscanf(recvline,
            "%255s %ld %255s %d %63s %f %d %d %d %f %f %d %d",
	    command,
	    &recvfrom,
	    orig_command,
	    &n_frame,
	    archetypeId,
	    &n_energy,
	    &n_horzrot,
	    &n_vertrot,
	    &n_dead,
	    &n_x,
	    &n_z,
	    &n_beamOn,
	    &n_timeDead) != 13)
	return;
    archetype = gameEntityArchetype(archetypeId);
    if (archetype == NULL || !archetype->controllable)
	return;

    exists = 0;
    for(tempTarget = targets; tempTarget != NULL; tempTarget=tempTarget->next)
	{                                 /* note difference here ! */
	if (tempTarget->net_clientFrom == recvfrom)
	    {
	    exists = 1;
	    thaOne = tempTarget;
	    }
	}

    if (exists && thaOne->player.dataArchetype == archetype)
	{
	/*****************************************************************/
	/* update the values in the currently existing networked player */
	/*****************************************************************/

	thaOne->net_headHorzRotate	= n_horzrot;
	thaOne->net_headVertRotate	= n_vertrot;
	thaOne->net_energyRemaining	= n_energy;
	thaOne->net_x			= n_x;
	thaOne->net_z			= n_z;
	thaOne->net_beamOn		= n_beamOn;
	}
    else if (n_timeDead < 40)
	{
	/*******************************/
	/* add a new networked player */
	/*******************************/

	addNetworkPlayer(recvfrom, archetype, n_horzrot, n_vertrot,
	    n_energy, n_x, n_z, n_beamOn, arena);
	}
    }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int sendMessage(char * mesg,  struct sockaddr_in * cli, int length)
    {
    int n;

    n = strlen(mesg)+1;

    if (sendto(sockfd, mesg, n, 0, (struct sockaddr *) cli, length) != n)
	{
	printf("Sendto error\n");
	return(1);
	}
    else
	return(0);
    }

 /*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void processClient(time_t now, int mainCounter, int totalCounter,
    struct runtimeMapObject * mapObjectList, struct runtimeActor * actorlist, void * arena,
    int sizeActor, struct runtimePlayer playerEntityState,
    int destroyedMapObjectFrameCount, int *destroyedMapObjectFrame)
    {
    char recvline[MAXLINE];
    char mesg[MAXLINE];
    char command[MAXLINE];
    int n;
    socklen_t servlen;
    long clientFrom;
    int j;
    int frameNum;

    extern int errno;


    /* send a request every once in a while until connection established */
    /* if I haven't seen any data from the server in a while try to reconnect */
    /* this may be a bit harsh to say connected = 0 */

    if (now > (lastRecvTime + RECONNECT_TIME))
	connected = 0;

    if (!connected)
	{
	sprintf(mesg, "%s %d", MESG_JOIN, BATTALION_NETWORK_PROTOCOL);
	sendMessage(mesg, &serv_addr, sizeof(serv_addr));
	}

    if (now > (lastNotDeadYetTime + IM_NOT_DEAD_SEND_TIME))
	{
	lastNotDeadYetTime = now;

	sprintf(mesg,MESG_HEARTBEAT);
	sendMessage(mesg, &serv_addr, sizeof(serv_addr));
	}

	/*******************************************************/
	/* tell all the clients when a tree has been destroyed */
	/*******************************************************/


	for(j=0;j<destroyedMapObjectFrameCount;j++)
	    {
	    sprintf(mesg, "%s %d #", MESG_OBJECT_DESTROYED, destroyedMapObjectFrame[j]);

	    sendMessage(mesg, &serv_addr, sizeof(serv_addr));
	    }

	/***********************************/
	/* ocassionally send to the server */
	/***********************************/

	if (!(mainCounter % PLAYER_STATE_RATE))
	    {
	    sprintf(mesg, "%s %d %s %.1f %d %d %d %.3f %.3f %d %d #",
			    MESG_PLAYER_STATE,
			    totalCounter,
			    playerEntityState.dataArchetype->id,
			    playerEntityState.energyRemaining,
			    playerEntityState.headHorzRotate,
			    playerEntityState.headVertRotate,
			    playerEntityState.dead,
			    playerEntityState.x,
			    playerEntityState.z,
			    beamOnSinceLast,
			    playerEntityState.timeDead
			    );

	    beamOnSinceLast = 0;
	    sendMessage(mesg, &serv_addr, sizeof(serv_addr));
	    }


	/*************************************/
	/* wait for messages from the server */
	/*************************************/

	 servlen = sizeof(serv_addr);
	 n = recvfrom(sockfd, recvline, MAXLINE, 0,
	              (struct sockaddr *)&serv_addr, &servlen);
	 if ((n < 0)  && (errno != EWOULDBLOCK))
	    {
	    printf("Client receive error\n");
	    }
	else if (n >= 0)
	    {
	    clientFrom = ntohl(serv_addr.sin_addr.s_addr);
/*
	    printf("RECEIVED: %s",recvline);
	    printf("FROM: %ld\n\n",clientFrom);
*/
	    connected = 1;
	    lastRecvTime = now;

	    sscanf(recvline, "%s %d", command, &frameNum);
	    if (strcmp(command, MESG_PLAYER_STATE) == 0)
		{
		/*if (frameNum > lastPlayerStateFrame) */
		/* took this out since there can be > 1 player being sent
		 * to the client
		 */
		    {
		    updateNetworkPlayers(recvline, clientFrom, arena);
		    lastPlayerStateFrame = frameNum;
		    }
		}

	    if (strcmp(command, MESG_ACTOR_SPAWN) == 0)
		{
		addNewNetworkActor(recvline, actorlist, arena, sizeActor);
		}

	    if (strcmp(command, MESG_ACTOR_STATE) == 0)
		{
		updateNetworkActor(recvline, actorlist);
		}
	    if (strcmp(command, MESG_OBJECT_DESTROYED) == 0)
		{
		updateNetworkMapObjects(recvline, mapObjectList);
		}
	    if (strcmp(command, MESG_RELAY) == 0)
		{
		updateRelayedPlayer(recvline, arena);
		}
	    }

/*	close(sockfd); */

    }

 /*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void tellAboutDestroyedMapObjects(struct sockaddr_in * cli, int length,
			       int destroyedMapObjectHistoryCount,
			       int *destroyedMapObjectHistory)
    {
    int j;
    char mesg[MAXLINE];

    for(j=0; j<destroyedMapObjectHistoryCount; j++)
	{
	sprintf(mesg, "%s %d #", MESG_OBJECT_DESTROYED,  destroyedMapObjectHistory[j]);

	sendMessage(mesg, cli, length);
	}
    }

 /*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

  /* there is now a connected = 0 in setPlayConditions!!! */

void processNetwork(time_t now, int mainCounter, int totalCounter,
    struct runtimeMapObject * mapObjectList, struct runtimeActor * actorlist, void * arena,
    struct runtimePlayer playerEntityState, int destroyedMapObjectHistoryCount,
    int destroyedMapObjectFrameCount, int *destroyedMapObjectHistory,
    int *destroyedMapObjectFrame)
    {
    long recvFrom;
    struct sockaddr_in cli_addr;
    char recvline[MAXLINE];
    char mesg[MAXLINE];
    char command[MAXLINE];

    struct targetInfo * tempTarget;
    extern int errno;

    socklen_t clilen;
    int oldClient;
    int n;
    int i, j;
    int gotOne;
    int noMore;
    int thisClient;
    int frameNumber;

    clilen = sizeof(cli_addr);

    noMore = 0;

    actorModulator = (actorModulator + 1) % PLAYER_STATE_RATE;

    while(!noMore)
	{
	/**********************/
	/* Read incoming data */
	/**********************/

	n = recvfrom(sockfd, recvline, MAXLINE, 0,
	             (struct sockaddr *)&cli_addr, &clilen);

	if (n < 0)
	    if(errno != EWOULDBLOCK)
		{
		printf("Server recvfrom error\n");
		noMore = 1;
		}
	    else
		{
		noMore = 1;
		}
	else
	    {
	    recvFrom = ntohl(cli_addr.sin_addr.s_addr);
/*
	    printf("RECEIVED: %s",recvline);
	    printf(" FROM: %ld\n\n",recvFrom);
*/

	    thisClient = -1;

	    for(i=0; i<numClients; i++)
		if (recvFrom == clientsFrom[i])
		    {
		    thisClient = i;
		    }

	    /* its possible we are getting an update message from a
	    client we didn't know we had (if server crashes) so we
	    ignore all of these messages until it establishes a
	    'proper' connection */

	    if (thisClient != -1)
		{
		if (strcmp(recvline,MESG_HEARTBEAT) == 0)
		    {
		    clientsAlive[thisClient] = now;
		    }

		sscanf(recvline, "%s %d", command, &frameNumber);
		if (strcmp(command, MESG_PLAYER_STATE) == 0)
		    {
		    if (frameNumber > lastClientPlayerStateFrame[thisClient])
			{
			updateNetworkPlayers(recvline, recvFrom, arena);
			lastClientPlayerStateFrame[thisClient] = frameNumber;

			/*******************************************/
			/* send this info to all the other clients */
			/*******************************************/

			sprintf(mesg, "%s %ld %s", MESG_RELAY, recvFrom, recvline);

			for(i=0; i<numClients; i++)
			    if (i != thisClient)
				{
				sendMessage(mesg, &clients[i], sizeof(clients[i]));
				/* fprintf(stderr, "%s\n", mesg); */
				}
			}
		    }
		}

	    if (strcmp(command, MESG_OBJECT_DESTROYED) == 0)
		{
		updateNetworkMapObjects(recvline, mapObjectList);
		}


	    if (strcmp(command, MESG_JOIN) == 0 &&
	        frameNumber == BATTALION_NETWORK_PROTOCOL)
		{
		/*********************************************/
		/* need to see if this client already exists */
		/*********************************************/

		oldClient = 0;

		for(i=0; i<numClients; i++)
		    {
		    if (recvFrom == clientsFrom[i])
			{
			clients[i] 	= cli_addr;
			clientsAlive[i] = now;
			oldClient 	= 1;
			}
		    }

		if (oldClient)
		    {
		    sprintf(mesg, "%s %d", MESG_REACQUIRE,
		            BATTALION_NETWORK_PROTOCOL);
		    sendMessage(mesg, &cli_addr, (int)clilen);

		    tellAboutDestroyedMapObjects(&cli_addr, (int)clilen, destroyedMapObjectHistoryCount, destroyedMapObjectHistory);
		    tellAboutAllActors(&cli_addr, (int)clilen, actorlist);

		    }

		if (!oldClient)
		    {
		    clients[numClients] 	= cli_addr;
		    clientsFrom[numClients] 	= recvFrom;
		    clientsAlive[numClients]	= now;

		    numClients++;

		    sprintf(mesg, "%s %d", MESG_ACKNOWLEDGE,
		            BATTALION_NETWORK_PROTOCOL);
		    sendMessage(mesg, &cli_addr, (int)clilen);

		    tellAboutDestroyedMapObjects(&cli_addr, (int)clilen, destroyedMapObjectHistoryCount, destroyedMapObjectHistory);
		    tellAboutAllActors(&cli_addr, (int)clilen, actorlist);
		    }
		}

	    }

	}

	/****************************************************************/
	/* kill any client connection that has not responded in a while */
	/****************************************************************/

	i = 0;
	gotOne = 0;

	while ((i < numClients) && (!gotOne))
	    {
	    if ((clientsAlive[i]+IM_NOT_DEAD_YET_TIME) < now)
		{
		printf("killing client %d\n", i);

		/* remove the appropriate target */
		for(tempTarget = targets; tempTarget != NULL; tempTarget=tempTarget->next)
		    {
		    if (tempTarget->net_clientFrom == clientsFrom[i])
			tempTarget->net_energyRemaining = 0;
		    }

		for (j=i+1; j < numClients; j++)
		    {
		    clients[j-1]	= clients[j];
		    clientsFrom[j-1]	= clientsFrom[j];
		    clientsAlive[j-1]	= clientsAlive[j];
		    gotOne = 1;
		    }
		numClients -= 1;

		}
	    else
		i++;
	    }

	/*******************************************************/
	/* tell all the clients when a tree has been destroyed */
	/*******************************************************/

	for(j=0;j<destroyedMapObjectFrameCount;j++)
	    {
	    sprintf(mesg, "%s %d #", MESG_OBJECT_DESTROYED, destroyedMapObjectFrame[j]);

	    for(i=0; i<numClients; i++)
		sendMessage(mesg, &clients[i], sizeof(clients[i]));
	    }



	/*************************************************************/
	/* ocassionally broadcast to all currently connected clients */
	/*************************************************************/

	if (!(mainCounter % PLAYER_STATE_RATE))
	    {
	    sprintf(mesg, "%s %d %s %.1f %d %d %d %.3f %.3f %d %d #",
			    MESG_PLAYER_STATE,
			    totalCounter,
			    playerEntityState.dataArchetype->id,
			    playerEntityState.energyRemaining,
			    playerEntityState.headHorzRotate,
			    playerEntityState.headVertRotate,
			    playerEntityState.dead,
			    playerEntityState.x,
			    playerEntityState.z,
			    beamOnSinceLast,
			    playerEntityState.timeDead
			    );

	    beamOnSinceLast = 0;

	    for(i=0;i<numClients;i++)
		sendMessage(mesg, &clients[i], sizeof(clients[i]));
	    }
}
