/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "g_local.h"

levelstatic_t level;

typedef struct
{
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int		cvarFlags;
	int		modificationCount;	// for tracking changes
	qboolean	trackChange;		// track this variable, and announce if changed
	qboolean	teamShader;		// track and if changed, update shader state
} cvarTable_t;

ent_t g_entities[MAX_GENTITIES];
// all ents in g_entities saved in their state just after spawn, for levelrespawn
ent_t g_initialents[MAX_GENTITIES];
gclient_t g_clients[MAX_CLIENTS];

vmCvar_t g_gametype;
vmCvar_t g_dmflags;
vmCvar_t g_fraglimit;
vmCvar_t g_timelimit;
vmCvar_t g_capturelimit;
vmCvar_t g_friendlyFire;
vmCvar_t g_password;
vmCvar_t g_needpass;
vmCvar_t g_maxclients;
vmCvar_t g_maxGameClients;
vmCvar_t g_dedicated;
vmCvar_t g_speed;
vmCvar_t g_gravity;
vmCvar_t g_cheats;
vmCvar_t g_knockback;
vmCvar_t g_quadfactor;
vmCvar_t g_forcerespawn;
vmCvar_t g_inactivity;
vmCvar_t g_debugMove;
vmCvar_t g_debugDamage;
vmCvar_t g_debugAlloc;
vmCvar_t g_debugNPC;
vmCvar_t g_debugsave;
vmCvar_t g_weaponRespawn;
vmCvar_t g_weaponTeamRespawn;
vmCvar_t g_motd;
vmCvar_t g_synchronousClients;
vmCvar_t g_warmup;
vmCvar_t g_doWarmup;
vmCvar_t g_restarted;
vmCvar_t g_logfile;
vmCvar_t g_logfileSync;
vmCvar_t g_blood;
vmCvar_t g_podiumDist;
vmCvar_t g_podiumDrop;
vmCvar_t g_allowVote;
vmCvar_t g_teamAutoJoin;
vmCvar_t g_teamForceBalance;
vmCvar_t g_banIPs;
vmCvar_t g_filterBan;
vmCvar_t g_smoothClients;
vmCvar_t pmove_fixed;
vmCvar_t pmove_msec;
vmCvar_t g_rankings;
vmCvar_t g_listEntity;

static cvarTable_t gameCvarTable[] = {
	// don't override the cheat state set by the system
	{&g_cheats, "sv_cheats", "", 0, 0, qfalse},

	// noset vars
	{nil, "gamename", GAMEVERSION, CVAR_SERVERINFO | CVAR_ROM, 0, qfalse},
	{nil, "gamedate", __DATE__, CVAR_ROM, 0, qfalse},
	{&g_restarted, "g_restarted", "0", CVAR_ROM, 0, qfalse},

	// latched vars
	{&g_gametype, "g_gametype", "0", CVAR_SERVERINFO | CVAR_USERINFO | CVAR_LATCH, 0, qfalse},

	{&g_maxclients, "sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse},
	{&g_maxGameClients, "g_maxGameClients", "0", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse},

	// change anytime vars
	{&g_dmflags, "dmflags", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue},
	{&g_fraglimit, "fraglimit", "20", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue},
	{&g_timelimit, "timelimit", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue},
	{&g_capturelimit, "capturelimit", "8", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue},

	{&g_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO, 0, qfalse},

	{&g_friendlyFire, "g_friendlyFire", "0", CVAR_ARCHIVE, 0, qtrue},

	{&g_teamAutoJoin, "g_teamAutoJoin", "0", CVAR_ARCHIVE},
	{&g_teamForceBalance, "g_teamForceBalance", "0", CVAR_ARCHIVE},

	{&g_warmup, "g_warmup", "20", CVAR_ARCHIVE, 0, qtrue},
	{&g_doWarmup, "g_doWarmup", "0", CVAR_ARCHIVE, 0, qtrue},
	{&g_logfile, "g_log", "", CVAR_ARCHIVE, 0, qfalse},
	{&g_logfileSync, "g_logsync", "0", CVAR_ARCHIVE, 0, qfalse},

	{&g_password, "g_password", "", CVAR_USERINFO, 0, qfalse},

	{&g_banIPs, "g_banIPs", "", CVAR_ARCHIVE, 0, qfalse},
	{&g_filterBan, "g_filterBan", "1", CVAR_ARCHIVE, 0, qfalse},

	{&g_needpass, "g_needpass", "0", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse},

	{&g_dedicated, "dedicated", "0", 0, 0, qfalse},

	{&g_speed, "g_speed", "320", 0, 0, qtrue},
	{&g_gravity, "g_gravity", "800", 0, 0, qtrue},
	{&g_knockback, "g_knockback", "1000", 0, 0, qtrue},
	{&g_quadfactor, "g_quadfactor", "3", 0, 0, qtrue},
	{&g_weaponRespawn, "g_weaponrespawn", "5", 0, 0, qtrue},
	{&g_weaponTeamRespawn, "g_weaponTeamRespawn", "30", 0, 0, qtrue},
	{&g_forcerespawn, "g_forcerespawn", "20", 0, 0, qtrue},
	{&g_inactivity, "g_inactivity", "0", 0, 0, qtrue},
	{&g_debugMove, "g_debugMove", "0", 0, 0, qfalse},
	{&g_debugDamage, "g_debugDamage", "0", 0, 0, qfalse},
	{&g_debugAlloc, "g_debugAlloc", "0", 0, 0, qfalse},
	{&g_debugNPC, "g_debugNPC", "1", 0, 0, qfalse},
	{&g_debugsave, "g_debugsave", "0", 0, 0, qfalse},
	{&g_motd, "g_motd", "", 0, 0, qfalse},
	{&g_blood, "com_blood", "1", 0, 0, qfalse},

	{&g_podiumDist, "g_podiumDist", "80", 0, 0, qfalse},
	{&g_podiumDrop, "g_podiumDrop", "70", 0, 0, qfalse},

	{&g_allowVote, "g_allowVote", "1", CVAR_ARCHIVE, 0, qfalse},
	{&g_listEntity, "g_listEntity", "0", 0, 0, qfalse},

	{&g_smoothClients, "g_smoothClients", "1", 0, 0, qfalse},
	{&pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO, 0, qfalse},
	{&pmove_msec, "pmove_msec", "8", CVAR_SYSTEMINFO, 0, qfalse},

	{&g_rankings, "g_rankings", "0", 0, 0, qfalse}
};

static int gameCvarTableSize = ARRAY_LEN(gameCvarTable);

void	initgame(int levelTime, int randomSeed, int restart);
void	runframe(int levelTime);
void	shutdowngame(int restart);
void	CheckExitRules(void);

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
Q_EXPORT intptr_t
vmMain(int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11)
{
	switch(command){
	case GAME_INIT:
		initgame(arg0, arg1, arg2);
		return 0;
	case GAME_SHUTDOWN:
		shutdowngame(arg0);
		return 0;
	case GAME_CLIENT_CONNECT:
		return (intptr_t)clientconnect(arg0, arg1, arg2);
	case GAME_CLIENT_THINK:
		clientthink(arg0);
		return 0;
	case GAME_CLIENT_USERINFO_CHANGED:
		clientuserinfochanged(arg0);
		return 0;
	case GAME_CLIENT_DISCONNECT:
		clientdisconnect(arg0);
		return 0;
	case GAME_CLIENT_BEGIN:
		clientbegin(arg0);
		return 0;
	case GAME_CLIENT_COMMAND:
		clientcmd(arg0);
		return 0;
	case GAME_RUN_FRAME:
		runframe(arg0);
		return 0;
	case GAME_CONSOLE_COMMAND:
		return consolecmd();
	case BOTAI_START_FRAME:
		return BotAIStartFrame(arg0);
	}

	return -1;
}

void QDECL
gprintf(const char *fmt, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	trap_Print(text);
}

void QDECL
errorf(const char *fmt, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	trap_Error(text);
}

/*
Chain together all entities with a matching team field.
Entity teams are used for item groups and multi-entity mover groups.

All but the first will have the FL_TEAMSLAVE flag set and teammaster field set
All but the last will have the teamchain field set to the next one
*/
void
findteams(void)
{
	ent_t *e, *e2;
	int i, j;
	int c, c2;

	c = 0;
	c2 = 0;
	for(i = 1, e = g_entities+i; i < level.nentities; i++, e++){
		if(!e->inuse)
			continue;
		if(!e->team)
			continue;
		if(e->flags & FL_TEAMSLAVE)
			continue;
		e->teammaster = e;
		c++;
		c2++;
		for(j = i+1, e2 = e+1; j < level.nentities; j++, e2++){
			if(!e2->inuse)
				continue;
			if(!e2->team)
				continue;
			if(e2->flags & FL_TEAMSLAVE)
				continue;
			if(!strcmp(e->team, e2->team)){
				c2++;
				e2->teamchain = e->teamchain;
				e->teamchain = e2;
				e2->teammaster = e;
				e2->flags |= FL_TEAMSLAVE;

				// make sure that targets only point at the master
				if(e2->targetname){
					e->targetname = e2->targetname;
					e2->targetname = nil;
				}
			}
		}
	}

	gprintf("%i teams with %i entities\n", c, c2);
}

void
remapteamshaders(void)
{
}

void
registercvars(void)
{
	int i;
	cvarTable_t *cv;
	qboolean remapped = qfalse;

	for(i = 0, cv = gameCvarTable; i < gameCvarTableSize; i++, cv++){
		trap_Cvar_Register(cv->vmCvar, cv->cvarName,
				   cv->defaultString, cv->cvarFlags);
		if(cv->vmCvar)
			cv->modificationCount = cv->vmCvar->modificationCount;

		if(cv->teamShader)
			remapped = qtrue;
	}

	if(remapped)
		remapteamshaders();

	// check some things
	if(g_gametype.integer < 0 || g_gametype.integer >= GT_MAX_GAME_TYPE){
		gprintf("g_gametype %i is out of range, defaulting to 0\n", g_gametype.integer);
		trap_Cvar_Set("g_gametype", "0");
		trap_Cvar_Update(&g_gametype);
	}

	level.warmupmodificationcount = g_warmup.modificationCount;
}

void
updatecvars(void)
{
	int i;
	cvarTable_t *cv;
	qboolean remapped = qfalse;

	for(i = 0, cv = gameCvarTable; i < gameCvarTableSize; i++, cv++)
		if(cv->vmCvar){
			trap_Cvar_Update(cv->vmCvar);

			if(cv->modificationCount != cv->vmCvar->modificationCount){
				cv->modificationCount = cv->vmCvar->modificationCount;

				if(cv->trackChange)
					trap_SendServerCommand(-1, va("print \"Server: %s changed to %s\n\"",
								      cv->cvarName, cv->vmCvar->string));

				if(cv->teamShader)
					remapped = qtrue;
			}
		}

	if(remapped)
		remapteamshaders();
}

void
initgame(int levelTime, int randomSeed, int restart)
{
	int i;

	gprintf("------- Game Initialization -------\n");
	gprintf("gamename: %s\n", GAMEVERSION);
	gprintf("gamedate: %s\n", __DATE__);

	srand(randomSeed);

	registercvars();

	processipbans();

	initmem();

	// set some level globals
	memset(&level, 0, sizeof(level));
	level.time = levelTime;
	level.starttime = levelTime;
	level.checkpoint = ENTITYNUM_NONE;

	level.snd_fry = soundindex("sound/player/fry.wav");	// FIXME standing in lava / slime

	if(g_gametype.integer != GT_SINGLE_PLAYER && g_logfile.string[0]){
		if(g_logfileSync.integer)
			trap_FS_FOpenFile(g_logfile.string, &level.logfile, FS_APPEND_SYNC);
		else
			trap_FS_FOpenFile(g_logfile.string, &level.logfile, FS_APPEND);
		if(!level.logfile)
			gprintf("WARNING: Couldn't open logfile: %s\n", g_logfile.string);
		else{
			char serverinfo[MAX_INFO_STRING];

			trap_GetServerinfo(serverinfo, sizeof(serverinfo));

			logprintf("------------------------------------------------------------\n");
			logprintf("InitGame: %s\n", serverinfo);
		}
	}else
		gprintf("Not logging to disk.\n");

	worldsessinit();

	loadgame("save/0.txt");

	// initialize all entities for this game
	memset(g_entities, 0, MAX_GENTITIES * sizeof(g_entities[0]));
	level.entities = g_entities;

	// initialize all clients for this game
	level.maxclients = g_maxclients.integer;
	memset(g_clients, 0, MAX_CLIENTS * sizeof(g_clients[0]));
	level.clients = g_clients;

	// set client fields on player ents
	for(i = 0; i<level.maxclients; i++)
		g_entities[i].client = level.clients + i;

	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	level.nentities = MAX_CLIENTS;

	for(i = 0; i<MAX_CLIENTS; i++)
		g_entities[i].classname = "clientslot";

	// let the server system know where the entites are
	trap_LocateGameData(level.entities, level.nentities, sizeof(ent_t),
			    &level.clients[0].ps, sizeof(level.clients[0]));

	// reserve some spots for dead player bodies
	initbodyqueue();

	clearitems();

	// parse the key/value pairs and spawn entities
	spawnall();

	// general initialization
	findteams();

	// make sure we have flags for CTF, etc
	if(g_gametype.integer >= GT_TEAM)
		checkteamitems();

	mkitemsconfigstr();

	gprintf("-----------------------------------\n");

	if(trap_Cvar_VariableIntegerValue("bot_enable")){
		BotAISetup(restart);
		BotAILoadMap(restart);
		initbots(restart);
	}

	remapteamshaders();

	trap_SetConfigstring(CS_INTERMISSION, "");
}

void
shutdowngame(int restart)
{
	gprintf("==== ShutdownGame ====\n");

	if(level.logfile){
		logprintf("ShutdownGame:\n");
		logprintf("------------------------------------------------------------\n");
		trap_FS_FCloseFile(level.logfile);
		level.logfile = 0;
	}

	// write all the client session data so we can get it back
	sesswrite();

	// permanent save
	savegame("save/0.txt");

	if(trap_Cvar_VariableIntegerValue("bot_enable"))
		BotAIShutdown(restart);
}

/*
Soft re-initialisation of the level.

Called on singleplayer respawn.
Calls ent->levelrespawn if != nil. Use ent->levelrespawn with restoreinitialstate.
*/
void
levelrespawn(void)
{
	ent_t *e;
	int i;

	gprintf("------- levelrespawn -------\n");

	clearbodyqueue();
	for(i = 0, e = level.entities; i < level.nentities; i++, e++){
		if(!e->inuse)
			continue;
		if(i < MAX_CLIENTS && e->client->pers.connected == CON_CONNECTED)
			addevent(e, EV_LEVELRESPAWN, 0);
		else if(e->flags & FL_DROPPED_ITEM)
			entfree(e);
		else if(e->levelrespawn != nil &&
		   e->ckpoint == level.checkpoint)
			e->levelrespawn(e);
	}
}

void QDECL
Com_Error(int level, const char *error, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	trap_Error(text);
}

void QDECL
Com_Printf(const char *msg, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	trap_Print(text);
}

/*

Player scoring & sorting

*/

/*
If there are less than two tournament players, put a
spectator in the game and restart
*/
void
AddTournamentPlayer(void)
{
	int i;
	gclient_t *client;
	gclient_t *nextInLine;

	if(level.nplayingclients >= 2)
		return;

	// never change during intermission
	if(level.intermissiontime)
		return;

	nextInLine = nil;

	for(i = 0; i < level.maxclients; i++){
		client = &level.clients[i];
		if(client->pers.connected != CON_CONNECTED)
			continue;
		if(client->sess.team != TEAM_SPECTATOR)
			continue;
		// never select the dedicated follow or scoreboard clients
		if(client->sess.specstate == SPECTATOR_SCOREBOARD ||
		   client->sess.specclient < 0)
			continue;

		if(!nextInLine || client->sess.specnum > nextInLine->sess.specnum)
			nextInLine = client;
	}

	if(!nextInLine)
		return;

	level.warmuptime = -1;

	// set them to free-for-all team
	setteam(&g_entities[nextInLine - level.clients], "f");
}

/*
Add client to end of tournament queue
*/
void
addtourneyqueue(gclient_t *client)
{
	int index;
	gclient_t *curclient;

	for(index = 0; index < level.maxclients; index++){
		curclient = &level.clients[index];

		if(curclient->pers.connected != CON_DISCONNECTED){
			if(curclient == client)
				curclient->sess.specnum = 0;
			else if(curclient->sess.team == TEAM_SPECTATOR)
				curclient->sess.specnum++;
		}
	}
}

/*
Make the loser a spectator at the back of the line
*/
void
RemoveTournamentLoser(void)
{
	int clientNum;

	if(level.nplayingclients != 2)
		return;

	clientNum = level.sortedclients[1];

	if(level.clients[clientNum].pers.connected != CON_CONNECTED)
		return;

	// make them a spectator
	setteam(&g_entities[clientNum], "s");
}

void
RemoveTournamentWinner(void)
{
	int clientNum;

	if(level.nplayingclients != 2)
		return;

	clientNum = level.sortedclients[0];

	if(level.clients[clientNum].pers.connected != CON_CONNECTED)
		return;

	// make them a spectator
	setteam(&g_entities[clientNum], "s");
}

void
AdjustTournamentScores(void)
{
	int clientNum;

	clientNum = level.sortedclients[0];
	if(level.clients[clientNum].pers.connected == CON_CONNECTED){
		level.clients[clientNum].sess.wins++;
		clientuserinfochanged(clientNum);
	}

	clientNum = level.sortedclients[1];
	if(level.clients[clientNum].pers.connected == CON_CONNECTED){
		level.clients[clientNum].sess.losses++;
		clientuserinfochanged(clientNum);
	}
}

int QDECL
SortRanks(const void *a, const void *b)
{
	gclient_t *ca, *cb;

	ca = &level.clients[*(int*)a];
	cb = &level.clients[*(int*)b];

	// sort special clients last
	if(ca->sess.specstate == SPECTATOR_SCOREBOARD || ca->sess.specclient < 0)
		return 1;
	if(cb->sess.specstate == SPECTATOR_SCOREBOARD || cb->sess.specclient < 0)
		return -1;

	// then connecting clients
	if(ca->pers.connected == CON_CONNECTING)
		return 1;
	if(cb->pers.connected == CON_CONNECTING)
		return -1;


	// then spectators
	if(ca->sess.team == TEAM_SPECTATOR && cb->sess.team == TEAM_SPECTATOR){
		if(ca->sess.specnum > cb->sess.specnum)
			return -1;
		if(ca->sess.specnum < cb->sess.specnum)
			return 1;
		return 0;
	}
	if(ca->sess.team == TEAM_SPECTATOR)
		return 1;
	if(cb->sess.team == TEAM_SPECTATOR)
		return -1;

	// then sort by score
	if(ca->ps.persistant[PERS_SCORE]
	   > cb->ps.persistant[PERS_SCORE])
		return -1;
	if(ca->ps.persistant[PERS_SCORE]
	   < cb->ps.persistant[PERS_SCORE])
		return 1;
	return 0;
}

/*
Recalculates the score ranks of all players
This will be called on every client connect, begin, disconnect, death,
and team change.
*/
void
calcranks(void)
{
	int i;
	int rank;
	int score;
	int newScore;
	gclient_t *cl;

	level.follow1 = -1;
	level.follow2 = -1;
	level.nconnectedclients = 0;
	level.nnonspecclients = 0;
	level.nplayingclients = 0;
	level.nvoters = 0;	// don't count bots

	for(i = 0; i < ARRAY_LEN(level.nteamvoters); i++)
		level.nteamvoters[i] = 0;

	for(i = 0; i < level.maxclients; i++)
		if(level.clients[i].pers.connected != CON_DISCONNECTED){
			level.sortedclients[level.nconnectedclients] = i;
			level.nconnectedclients++;

			if(level.clients[i].sess.team != TEAM_SPECTATOR){
				level.nnonspecclients++;

				// decide if this should be auto-followed
				if(level.clients[i].pers.connected == CON_CONNECTED){
					level.nplayingclients++;
					if(!(g_entities[i].r.svFlags & SVF_BOT)){
						level.nvoters++;
						if(level.clients[i].sess.team == TEAM_RED)
							level.nteamvoters[0]++;
						else if(level.clients[i].sess.team == TEAM_BLUE)
							level.nteamvoters[1]++;
					}
					if(level.follow1 == -1)
						level.follow1 = i;
					else if(level.follow2 == -1)
						level.follow2 = i;
				}
			}
		}

	qsort(level.sortedclients, level.nconnectedclients,
	      sizeof(level.sortedclients[0]), SortRanks);

	// set the rank value for all clients that are connected and not spectators
	if(g_gametype.integer >= GT_TEAM)
		// in team games, rank is just the order of the teams, 0=red, 1=blue, 2=tied
		for(i = 0; i < level.nconnectedclients; i++){
			cl = &level.clients[level.sortedclients[i]];
			if(level.teamscores[TEAM_RED] == level.teamscores[TEAM_BLUE])
				cl->ps.persistant[PERS_RANK] = 2;
			else if(level.teamscores[TEAM_RED] > level.teamscores[TEAM_BLUE])
				cl->ps.persistant[PERS_RANK] = 0;
			else
				cl->ps.persistant[PERS_RANK] = 1;
		}
	else{
		rank = -1;
		score = 0;
		for(i = 0; i < level.nplayingclients; i++){
			cl = &level.clients[level.sortedclients[i]];
			newScore = cl->ps.persistant[PERS_SCORE];
			if(i == 0 || newScore != score){
				rank = i;
				// assume we aren't tied until the next client is checked
				level.clients[level.sortedclients[i]].ps.persistant[PERS_RANK] = rank;
			}else{
				// we are tied with the previous client
				level.clients[level.sortedclients[i-1]].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
				level.clients[level.sortedclients[i]].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
			score = newScore;
			if(g_gametype.integer == GT_SINGLE_PLAYER && level.nplayingclients == 1)
				level.clients[level.sortedclients[i]].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
		}
	}

	// set the CS_SCORES1/2 configstrings, which will be visible to everyone
	if(g_gametype.integer >= GT_TEAM){
		trap_SetConfigstring(CS_SCORES1, va("%i", level.teamscores[TEAM_RED]));
		trap_SetConfigstring(CS_SCORES2, va("%i", level.teamscores[TEAM_BLUE]));
	}else{
		if(level.nconnectedclients == 0){
			trap_SetConfigstring(CS_SCORES1, va("%i", SCORE_NOT_PRESENT));
			trap_SetConfigstring(CS_SCORES2, va("%i", SCORE_NOT_PRESENT));
		}else if(level.nconnectedclients == 1){
			trap_SetConfigstring(CS_SCORES1, va("%i", level.clients[level.sortedclients[0]].ps.persistant[PERS_SCORE]));
			trap_SetConfigstring(CS_SCORES2, va("%i", SCORE_NOT_PRESENT));
		}else{
			trap_SetConfigstring(CS_SCORES1, va("%i", level.clients[level.sortedclients[0]].ps.persistant[PERS_SCORE]));
			trap_SetConfigstring(CS_SCORES2, va("%i", level.clients[level.sortedclients[1]].ps.persistant[PERS_SCORE]));
		}
	}

	// see if it is time to end the level
	CheckExitRules();

	// if we are at the intermission, send the new info to everyone
	if(level.intermissiontime)
		sendscoreboardmsgall();
}

/*

Map changing

*/

/*
Do this at intermission time and whenever ranks are recalculated
due to enters/exits/forced team changes
*/
void
sendscoreboardmsgall(void)
{
	int i;

	for(i = 0; i < level.maxclients; i++)
		if(level.clients[i].pers.connected == CON_CONNECTED)
			deathmatchscoreboardmsg(g_entities + i);
}

/*
When the intermission starts, this will be called for all players.
If a new client connects, this will be called after the spawn function.
*/
void
clientintermission(ent_t *ent)
{
	// take out of follow mode if needed
	if(ent->client->sess.specstate == SPECTATOR_FOLLOW)
		stopfollowing(ent);

	findintermissionpoint();
	// move to the spot
	veccpy(level.intermissionpos, ent->s.origin);
	veccpy(level.intermissionpos, ent->client->ps.origin);
	veccpy(level.intermissionangle, ent->client->ps.viewangles);
	ent->client->ps.pm_type = PM_INTERMISSION;

	// clean up powerup info
	memset(ent->client->ps.powerups, 0, sizeof(ent->client->ps.powerups));

	ent->client->ps.eFlags = 0;
	ent->s.eFlags = 0;
	ent->s.eType = ET_GENERAL;
	ent->s.modelindex = 0;
	ent->s.loopSound = 0;
	ent->s.event = 0;
	ent->r.contents = 0;
}

/*
This is also used for spectator spawns
*/
void
findintermissionpoint(void)
{
	ent_t *ent, *target;
	vec3_t dir;

	// find the intermission spot
	ent = findent(nil, FOFS(classname), "info_player_intermission");
	if(ent == nil){	// the map creator forgot to put in an intermission point...
		selectspawnpoint(vec3_origin, level.intermissionpos,
		   level.intermissionangle, qfalse);
		return;
	}
	veccpy(ent->s.origin, level.intermissionpos);
	veccpy(ent->s.angles, level.intermissionangle);
	// if it has a target, look towards it
	if(ent->target != nil){
		target = picktarget(ent->target);
		if(target != nil){
			vecsub(target->s.origin, level.intermissionpos, dir);
			vectoangles(dir, level.intermissionangle);
		}
		mksound(target, CHAN_AUTO, ent->noiseindex);
	}else{
		mksound(ent, CHAN_AUTO, ent->noiseindex);
	}
	if(ent->message != nil){
		// FIXME: centerprint won't accommodate long end-of-level messages
		trap_SendServerCommand(-1, va("cp \"%s\"", ent->message));
	}
}

void
intermission(void)
{
	int i;
	ent_t *client;

	if(level.intermissiontime)
		return;	// already active

	// if in tournement mode, change the wins / losses
	if(g_gametype.integer == GT_TOURNAMENT)
		AdjustTournamentScores();

	level.intermissiontime = level.time;
	// move all clients to the intermission point
	for(i = 0; i< level.maxclients; i++){
		client = g_entities + i;
		if(!client->inuse)
			continue;
		// respawn if dead
		if(client->health <= 0 && client->client->ps.persistant[PERS_LIVES] > 0)
			clientrespawn(client);
		clientintermission(client);
	}
	// if single player game
	// send the current scoring to all clients
	sendscoreboardmsgall();
}

/*
When the intermission has been exited, the server is either killed
or moved to a new level based on the "nextmap" cvar
*/
void
ExitLevel(void)
{
	int i;
	gclient_t *cl;
	char nextmap[MAX_STRING_CHARS];
	char d1[MAX_STRING_CHARS];

	//bot interbreeding
	botinterbreed();

	// if we are running a tournement map, kick the loser to spectator status,
	// which will automatically grab the next spectator and restart
	if(g_gametype.integer == GT_TOURNAMENT){
		if(!level.restarted){
			RemoveTournamentLoser();
			trap_SendConsoleCommand(EXEC_APPEND, "map_restart 0\n");
			level.restarted = qtrue;
			level.changemap = nil;
			level.intermissiontime = 0;
		}
		return;
	}

	trap_Cvar_VariableStringBuffer("nextmap", nextmap, sizeof(nextmap));
	trap_Cvar_VariableStringBuffer("d1", d1, sizeof(d1));

	if(!Q_stricmp(nextmap, "map_restart 0") && Q_stricmp(d1, "")){
		trap_Cvar_Set("nextmap", "vstr d2");
		trap_SendConsoleCommand(EXEC_APPEND, "vstr d1\n");
	}else
		trap_SendConsoleCommand(EXEC_APPEND, "vstr nextmap\n");

	level.changemap = nil;
	level.intermissiontime = 0;

	// reset all the scores so we don't enter the intermission again
	level.teamscores[TEAM_RED] = 0;
	level.teamscores[TEAM_BLUE] = 0;
	for(i = 0; i< g_maxclients.integer; i++){
		cl = level.clients + i;
		if(cl->pers.connected != CON_CONNECTED)
			continue;
		cl->ps.persistant[PERS_SCORE] = 0;
	}

	// we need to do this here before changing to CON_CONNECTING
	sesswrite();

	// permanent save
	savegame("save/0.txt");

	// change all client states to connecting, so the early players into the
	// next level will know the others aren't done reconnecting
	for(i = 0; i< g_maxclients.integer; i++)
		if(level.clients[i].pers.connected == CON_CONNECTED)
			level.clients[i].pers.connected = CON_CONNECTING;

}

/*
Print to the logfile with a time stamp if it is open
*/
void QDECL
logprintf(const char *fmt, ...)
{
	va_list argptr;
	char string[1024];
	int min, tens, sec;

	sec = (level.time - level.starttime) / 1000;

	min = sec / 60;
	sec -= min * 60;
	tens = sec / 10;
	sec -= tens * 10;

	Com_sprintf(string, sizeof(string), "%3i:%i%i ", min, tens, sec);

	va_start(argptr, fmt);
	Q_vsnprintf(string + 7, sizeof(string) - 7, fmt, argptr);
	va_end(argptr);

	if(g_dedicated.integer)
		gprintf("%s", string + 7);

	if(!level.logfile)
		return;

	trap_FS_Write(string, strlen(string), level.logfile);
}

/*
Append information about this game to the log file
*/
void
LogExit(const char *string)
{
	int i, numSorted;
	gclient_t *cl;
	logprintf("Exit: %s\n", string);

	level.intermissionqueued = level.time;

	// this will keep the clients from playing any voice sounds
	// that will get cut off when the queued intermission starts
	trap_SetConfigstring(CS_INTERMISSION, "1");

	// don't send more than 32 scores (FIXME?)
	numSorted = level.nconnectedclients;
	if(numSorted > 32)
		numSorted = 32;

	if(g_gametype.integer >= GT_TEAM)
		logprintf("red:%i  blue:%i\n",
			    level.teamscores[TEAM_RED], level.teamscores[TEAM_BLUE]);

	for(i = 0; i < numSorted; i++){
		int ping;

		cl = &level.clients[level.sortedclients[i]];

		if(cl->sess.team == TEAM_SPECTATOR)
			continue;
		if(cl->pers.connected == CON_CONNECTING)
			continue;

		ping = cl->ps.ping < 999 ? cl->ps.ping : 999;

		logprintf("score: %i  ping: %i  client: %i %s\n",
		   cl->ps.persistant[PERS_SCORE], ping, level.sortedclients[i],
		   cl->pers.netname);
	}

}

/*
The level will stay at the intermission for a minimum of 5 seconds
If all players wish to continue, the level will then exit.
If one or more players have not acknowledged the continue, the game will
wait 10 seconds before going on.
*/
void
CheckIntermissionExit(void)
{
	int ready, notReady, playerCount;
	int i;
	gclient_t *cl;
	int readyMask;

	if(g_gametype.integer == GT_SINGLE_PLAYER)
		return;

	// see which players are ready
	ready = 0;
	notReady = 0;
	readyMask = 0;
	playerCount = 0;
	for(i = 0; i< g_maxclients.integer; i++){
		cl = level.clients + i;
		if(cl->pers.connected != CON_CONNECTED)
			continue;
		if(g_entities[i].r.svFlags & SVF_BOT)
			continue;

		playerCount++;
		if(cl->readytoexit){
			ready++;
			if(i < 16)
				readyMask |= 1 << i;
		}else
			notReady++;
	}

	// copy the readyMask to each player's stats so
	// it can be displayed on the scoreboard
	for(i = 0; i< g_maxclients.integer; i++){
		cl = level.clients + i;
		if(cl->pers.connected != CON_CONNECTED)
			continue;
		cl->ps.stats[STAT_CLIENTS_READY] = readyMask;
	}

	// never exit in less than five seconds
	if(level.time < level.intermissiontime + 5000)
		return;

	// only test ready status when there are real players present
	if(playerCount > 0){
		// if nobody wants to go, clear timer
		if(!ready){
			level.readytoexit = qfalse;
			return;
		}

		// if everyone wants to go, go now
		if(!notReady){
			ExitLevel();
			return;
		}
	}

	// the first person to ready starts the ten second timeout
	if(!level.readytoexit){
		level.readytoexit = qtrue;
		level.exittime = level.time;
	}

	// if we have waited ten seconds since at least one player
	// wanted to exit, go ahead
	if(level.time < level.exittime + 10000)
		return;

	ExitLevel();
}

qboolean
ScoreIsTied(void)
{
	int a, b;

	if(level.nplayingclients < 2)
		return qfalse;

	if(g_gametype.integer >= GT_TEAM)
		return level.teamscores[TEAM_RED] == level.teamscores[TEAM_BLUE];

	a = level.clients[level.sortedclients[0]].ps.persistant[PERS_SCORE];
	b = level.clients[level.sortedclients[1]].ps.persistant[PERS_SCORE];

	return a == b;
}

/*
There will be a delay between the time the exit is qualified for
and the time everyone is moved to the intermission spot, so you
can see the last frag.
*/
void
CheckExitRules(void)
{
	int i;
	gclient_t *cl;
	// if at the intermission, wait for all non-bots to
	// signal ready, then go to next level
	if(level.intermissiontime){
		CheckIntermissionExit();
		return;
	}

	if(level.intermissionqueued){
		if(level.time - level.intermissionqueued >= INTERMISSION_DELAY_TIME){
			level.intermissionqueued = 0;
			intermission();
		}
		return;
	}

	// check for sudden death
	if(ScoreIsTied())
		// always wait for sudden death
		return;

	if(g_timelimit.integer && !level.warmuptime)
		if(level.time - level.starttime >= g_timelimit.integer*60000){
			trap_SendServerCommand(-1, "print \"Timelimit hit.\n\"");
			LogExit("Timelimit hit.");
			return;
		}

	if(g_gametype.integer < GT_CTF && g_fraglimit.integer){
		if(level.teamscores[TEAM_RED] >= g_fraglimit.integer){
			trap_SendServerCommand(-1, "print \"Red hit the fraglimit.\n\"");
			LogExit("Fraglimit hit.");
			return;
		}

		if(level.teamscores[TEAM_BLUE] >= g_fraglimit.integer){
			trap_SendServerCommand(-1, "print \"Blue hit the fraglimit.\n\"");
			LogExit("Fraglimit hit.");
			return;
		}

		for(i = 0; i< g_maxclients.integer; i++){
			cl = level.clients + i;
			if(cl->pers.connected != CON_CONNECTED)
				continue;
			if(cl->sess.team != TEAM_FREE)
				continue;

			if(cl->ps.persistant[PERS_SCORE] >= g_fraglimit.integer){
				LogExit("Fraglimit hit.");
				trap_SendServerCommand(-1, va("print \"%s" S_COLOR_WHITE " hit the fraglimit.\n\"",
							      cl->pers.netname));
				return;
			}
		}
	}

	if(g_gametype.integer >= GT_CTF && g_capturelimit.integer){
		if(level.teamscores[TEAM_RED] >= g_capturelimit.integer){
			trap_SendServerCommand(-1, "print \"Red hit the capturelimit.\n\"");
			LogExit("Capturelimit hit.");
			return;
		}

		if(level.teamscores[TEAM_BLUE] >= g_capturelimit.integer){
			trap_SendServerCommand(-1, "print \"Blue hit the capturelimit.\n\"");
			LogExit("Capturelimit hit.");
			return;
		}
	}
}

/*

Functions called every frame

*/

/*
Once a frame, check for changes in tournement player state
*/
void
CheckTournament(void)
{
	// check because we run 3 game frames before calling Connect and/or clientbegin
	// for clients on a map_restart
	if(level.nplayingclients == 0)
		return;

	if(g_gametype.integer == GT_TOURNAMENT){
		// pull in a spectator if needed
		if(level.nplayingclients < 2)
			AddTournamentPlayer();

		// if we don't have two players, go back to "waiting for players"
		if(level.nplayingclients != 2){
			if(level.warmuptime != -1){
				level.warmuptime = -1;
				trap_SetConfigstring(CS_WARMUP, va("%i", level.warmuptime));
				logprintf("Warmup:\n");
			}
			return;
		}

		if(level.warmuptime == 0)
			return;

		// if the warmup is changed at the console, restart it
		if(g_warmup.modificationCount != level.warmupmodificationcount){
			level.warmupmodificationcount = g_warmup.modificationCount;
			level.warmuptime = -1;
		}

		// if all players have arrived, start the countdown
		if(level.warmuptime < 0){
			if(level.nplayingclients == 2){
				// fudge by -1 to account for extra delays
				if(g_warmup.integer > 1)
					level.warmuptime = level.time + (g_warmup.integer - 1) * 1000;
				else
					level.warmuptime = 0;

				trap_SetConfigstring(CS_WARMUP, va("%i", level.warmuptime));
			}
			return;
		}

		// if the warmup time has counted down, restart
		if(level.time > level.warmuptime){
			level.warmuptime += 10000;
			trap_Cvar_Set("g_restarted", "1");
			trap_SendConsoleCommand(EXEC_APPEND, "map_restart 0\n");
			level.restarted = qtrue;
			return;
		}
	}else if(g_gametype.integer != GT_SINGLE_PLAYER && level.warmuptime != 0){
		int counts[TEAM_NUM_TEAMS];
		qboolean notEnough = qfalse;

		if(g_gametype.integer > GT_TEAM){
			counts[TEAM_BLUE] = teamcount(-1, TEAM_BLUE);
			counts[TEAM_RED] = teamcount(-1, TEAM_RED);

			if(counts[TEAM_RED] < 1 || counts[TEAM_BLUE] < 1)
				notEnough = qtrue;
		}else if(level.nplayingclients < 2)
			notEnough = qtrue;

		if(notEnough){
			if(level.warmuptime != -1){
				level.warmuptime = -1;
				trap_SetConfigstring(CS_WARMUP, va("%i", level.warmuptime));
				logprintf("Warmup:\n");
			}
			return;	// still waiting for team members
		}

		if(level.warmuptime == 0)
			return;

		// if the warmup is changed at the console, restart it
		if(g_warmup.modificationCount != level.warmupmodificationcount){
			level.warmupmodificationcount = g_warmup.modificationCount;
			level.warmuptime = -1;
		}

		// if all players have arrived, start the countdown
		if(level.warmuptime < 0){
			// fudge by -1 to account for extra delays
			if(g_warmup.integer > 1)
				level.warmuptime = level.time + (g_warmup.integer - 1) * 1000;
			else
				level.warmuptime = 0;

			trap_SetConfigstring(CS_WARMUP, va("%i", level.warmuptime));
			return;
		}

		// if the warmup time has counted down, restart
		if(level.time > level.warmuptime){
			level.warmuptime += 10000;
			trap_Cvar_Set("g_restarted", "1");
			trap_SendConsoleCommand(EXEC_APPEND, "map_restart 0\n");
			level.restarted = qtrue;
			return;
		}
	}
}

void
CheckVote(void)
{
	if(level.voteexectime && level.voteexectime < level.time){
		level.voteexectime = 0;
		trap_SendConsoleCommand(EXEC_APPEND, va("%s\n", level.votestr));
	}
	if(!level.votetime)
		return;
	if(level.time - level.votetime >= VOTE_TIME)
		trap_SendServerCommand(-1, "print \"Vote failed.\n\"");
	else{
		// ATVI Q3 1.32 Patch #9, WNF
		if(level.voteyes > level.nvoters/2){
			// execute the command, then remove the vote
			trap_SendServerCommand(-1, "print \"Vote passed.\n\"");
			level.voteexectime = level.time + 3000;
		}else if(level.voteno >= level.nvoters/2)
			// same behavior as a timeout
			trap_SendServerCommand(-1, "print \"Vote failed.\n\"");
		else
			// still waiting for a majority
			return;
	}
	level.votetime = 0;
	trap_SetConfigstring(CS_VOTE_TIME, "");
}

void
PrintTeam(int team, char *message)
{
	int i;

	for(i = 0; i < level.maxclients; i++){
		if(level.clients[i].sess.team != team)
			continue;
		trap_SendServerCommand(i, message);
	}
}

void
setleader(int team, int client)
{
	int i;

	if(level.clients[client].pers.connected == CON_DISCONNECTED){
		PrintTeam(team, va("print \"%s is not connected\n\"",
		   level.clients[client].pers.netname));
		return;
	}
	if(level.clients[client].sess.team != team){
		PrintTeam(team, va("print \"%s is not on the team anymore\n\"",
		   level.clients[client].pers.netname));
		return;
	}
	for(i = 0; i < level.maxclients; i++){
		if(level.clients[i].sess.team != team)
			continue;
		if(level.clients[i].sess.teamleader){
			level.clients[i].sess.teamleader = qfalse;
			clientuserinfochanged(i);
		}
	}
	level.clients[client].sess.teamleader = qtrue;
	clientuserinfochanged(client);
	PrintTeam(team, va("print \"%s is the new team leader\n\"",
	   level.clients[client].pers.netname));
}

void
chkteamleader(int team)
{
	int i;

	for(i = 0; i < level.maxclients; i++){
		if(level.clients[i].sess.team != team)
			continue;
		if(level.clients[i].sess.teamleader)
			break;
	}
	if(i >= level.maxclients){
		for(i = 0; i < level.maxclients; i++){
			if(level.clients[i].sess.team != team)
				continue;
			if(!(g_entities[i].r.svFlags & SVF_BOT)){
				level.clients[i].sess.teamleader = qtrue;
				break;
			}
		}

		if(i >= level.maxclients)
			for(i = 0; i < level.maxclients; i++){
				if(level.clients[i].sess.team != team)
					continue;
				level.clients[i].sess.teamleader = qtrue;
				break;
			}
	}
}

void
CheckTeamVote(int team)
{
	int cs_offset;

	if(team == TEAM_RED)
		cs_offset = 0;
	else if(team == TEAM_BLUE)
		cs_offset = 1;
	else
		return;

	if(!level.teamvotetime[cs_offset])
		return;
	if(level.time - level.teamvotetime[cs_offset] >= VOTE_TIME)
		trap_SendServerCommand(-1, "print \"Team vote failed.\n\"");
	else{
		if(level.teamvoteyes[cs_offset] > level.nteamvoters[cs_offset]/2){
			// execute the command, then remove the vote
			trap_SendServerCommand(-1, "print \"Team vote passed.\n\"");
			if(!Q_strncmp("leader", level.teamvotestr[cs_offset], 6))
				//set the team leader
				setleader(team, atoi(level.teamvotestr[cs_offset] + 7));
			else
				trap_SendConsoleCommand(EXEC_APPEND, va("%s\n", level.teamvotestr[cs_offset]));
		}else if(level.teamvoteno[cs_offset] >= level.nteamvoters[cs_offset]/2)
			// same behavior as a timeout
			trap_SendServerCommand(-1, "print \"Team vote failed.\n\"");
		else
			// still waiting for a majority
			return;
	}
	level.teamvotetime[cs_offset] = 0;
	trap_SetConfigstring(CS_TEAMVOTE_TIME + cs_offset, "");
}

void
CheckCvars(void)
{
	static int lastMod = -1;

	if(g_password.modificationCount != lastMod){
		lastMod = g_password.modificationCount;
		if(*g_password.string && Q_stricmp(g_password.string, "none"))
			trap_Cvar_Set("g_needpass", "1");
		else
			trap_Cvar_Set("g_needpass", "0");
	}
}

/*
Runs thinking code for this frame if necessary
*/
void
runthink(ent_t *ent)
{
	int thinktime;

	thinktime = ent->nextthink;
	if(thinktime <= 0)
		return;
	if(thinktime > level.time)
		return;

	ent->nextthink = 0;
	if(!ent->think)
		errorf("nil ent->think");
	ent->think(ent);
}

void
gameover(void)
{
	ent_t *ent;
	int i;

	gprintf("------- gameover -------\n");

	if(level.gameovertime)
		return;

	for(i = 0, ent = level.entities; i < MAX_CLIENTS; i++, ent++)
		if(ent->inuse && ent->client->sess.specstate == SPECTATOR_NOT){
			// force spec now
			ent->client->deathspectime = level.time;
		}
	level.gameovertime = level.time + 5000;
}

/*
Advances the non-player objects in the world
*/
void
runframe(int levelTime)
{
	int i;
	ent_t *ent;

	// if we are waiting for the level to restart, do nothing
	if(level.restarted)
		return;

	level.framenum++;
	level.prevtime = level.time;
	level.time = levelTime;

	// get any cvar changes
	updatecvars();

	if(level.gameovertime > 0 && level.time > level.gameovertime){
		trap_SendConsoleCommand(EXEC_APPEND, "map limbo\n");
		level.gameovertime = 0;
	}

	// go through all allocated objects
	ent = &g_entities[0];
	for(i = 0; i<level.nentities; i++, ent++){
		if(!ent->inuse)
			continue;

		// clear events that are too old
		if(level.time - ent->eventtime > EVENT_VALID_MSEC){
			if(ent->s.event){
				ent->s.event = 0;	// &= EV_EVENT_BITS;
				if(ent->client)
					ent->client->ps.externalEvent = 0;
					// predicted events should never be set to zero
					//ent->client->ps.events[0] = 0;
					//ent->client->ps.events[1] = 0;
			}
			if(ent->freeafterevent){
				// tempEntities or dropped items completely go away after their event
				entfree(ent);
				continue;
			}else if(ent->unlinkAfterEvent){
				// items that will respawn will hide themselves after their pickup event
				ent->unlinkAfterEvent = qfalse;
				trap_UnlinkEntity(ent);
			}
		}

		// temporary entities don't think
		if(ent->freeafterevent)
			continue;

		if(!ent->r.linked && ent->neverfree)
			continue;

		if(ent->s.eType == ET_MISSILE){
			runmissile(ent);
			continue;
		}

		if(ent->s.eType == ET_ITEM || ent->physobj){
			runitem(ent);
			continue;
		}

		if(ent->s.eType == ET_MOVER){
			runmover(ent);
			continue;
		}

		if(ent->s.eType == ET_NPC){
			runnpc(ent);
			continue;
		}

		if(i < MAX_CLIENTS){
			runclient(ent);
			continue;
		}

		runthink(ent);
	}

	// perform final fixups on the players
	ent = &g_entities[0];
	for(i = 0; i < level.maxclients; i++, ent++)
		if(ent->inuse)
			clientendframe(ent);

	// see if it is time to do a tournement restart
	CheckTournament();

	// see if it is time to end the level
	CheckExitRules();

	// update to team status?
	checkteamstatus();

	// cancel vote if timed out
	CheckVote();

	// check team votes
	CheckTeamVote(TEAM_RED);
	CheckTeamVote(TEAM_BLUE);

	// for tracking changes
	CheckCvars();

	if(g_listEntity.integer){
		for(i = 0; i < MAX_GENTITIES; i++)
			gprintf("%4i: %s\n", i, g_entities[i].classname);
		trap_Cvar_Set("g_listEntity", "0");
	}
}
