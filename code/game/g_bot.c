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
// g_bot.c

#include "g_local.h"

static int g_numBots;
static char *g_botInfos[MAX_BOTS];

int g_numArenas;
static char *g_arenaInfos[MAX_ARENAS];

#define BOT_BEGIN_DELAY_BASE		2000
#define BOT_BEGIN_DELAY_INCREMENT	1500

#define BOT_SPAWN_QUEUE_DEPTH		16

typedef struct
{
	int	clientNum;
	int	spawntime;
} botSpawnQueue_t;

static botSpawnQueue_t botSpawnQueue[BOT_SPAWN_QUEUE_DEPTH];

vmCvar_t bot_minplayers;

extern ent_t *podium1;
extern ent_t *podium2;
extern ent_t *podium3;

float
trap_Cvar_VariableValue(const char *var_name)
{
	char buf[128];

	trap_Cvar_VariableStringBuffer(var_name, buf, sizeof(buf));
	return atof(buf);
}

/*
===============
parseinfos
===============
*/
int
parseinfos(char *buf, int max, char *infos[])
{
	char *token;
	int count;
	char key[MAX_TOKEN_CHARS];
	char info[MAX_INFO_STRING];

	count = 0;

	while(1){
		token = COM_Parse(&buf);
		if(!token[0])
			break;
		if(strcmp(token, "{")){
			Com_Printf("Missing { in info file\n");
			break;
		}

		if(count == max){
			Com_Printf("Max infos exceeded\n");
			break;
		}

		info[0] = '\0';
		while(1){
			token = COM_ParseExt(&buf, qtrue);
			if(!token[0]){
				Com_Printf("Unexpected end of info file\n");
				break;
			}
			if(!strcmp(token, "}"))
				break;
			Q_strncpyz(key, token, sizeof(key));

			token = COM_ParseExt(&buf, qfalse);
			if(!token[0])
				strcpy(token, "<nil>");
			Info_SetValueForKey(info, key, token);
		}
		//NOTE: extra space for arena number
		infos[count] = alloc(strlen(info) + strlen("\\num\\") + strlen(va("%d", MAX_ARENAS)) + 1);
		if(infos[count]){
			strcpy(infos[count], info);
			count++;
		}
	}
	return count;
}

/*
===============
loadarenasfromfile
===============
*/
static void
loadarenasfromfile(char *filename)
{
	int len;
	fileHandle_t f;
	char buf[MAX_ARENAS_TEXT];

	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if(!f){
		trap_Print(va(S_COLOR_RED "file not found: %s\n", filename));
		return;
	}
	if(len >= MAX_ARENAS_TEXT){
		trap_FS_FCloseFile(f);
		trap_Print(va(S_COLOR_RED "file too large: %s is %i, max allowed is %i\n", filename, len, MAX_ARENAS_TEXT));
		return;
	}

	trap_FS_Read(buf, len, f);
	buf[len] = 0;
	trap_FS_FCloseFile(f);

	g_numArenas += parseinfos(buf, MAX_ARENAS - g_numArenas, &g_arenaInfos[g_numArenas]);
}

/*
===============
loadarenas
===============
*/
static void
loadarenas(void)
{
	int numdirs;
	vmCvar_t arenasFile;
	char filename[128];
	char dirlist[1024];
	char *dirptr;
	int i, n;
	int dirlen;

	g_numArenas = 0;

	trap_Cvar_Register(&arenasFile, "g_arenasFile", "", CVAR_INIT|CVAR_ROM);
	if(*arenasFile.string)
		loadarenasfromfile(arenasFile.string);
	else
		loadarenasfromfile("scripts/arenas.txt");

	// get all arenas from .arena files
	numdirs = trap_FS_GetFileList("scripts", ".arena", dirlist, 1024);
	dirptr = dirlist;
	for(i = 0; i < numdirs; i++, dirptr += dirlen+1){
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/");
		strcat(filename, dirptr);
		loadarenasfromfile(filename);
	}
	trap_Print(va("%i arenas parsed\n", g_numArenas));

	for(n = 0; n < g_numArenas; n++)
		Info_SetValueForKey(g_arenaInfos[n], "num", va("%i", n));
}

/*
===============
getarenainfobynumber
===============
*/
const char *
getarenainfobymap(const char *map)
{
	int n;

	for(n = 0; n < g_numArenas; n++)
		if(Q_stricmp(Info_ValueForKey(g_arenaInfos[n], "map"), map) == 0)
			return g_arenaInfos[n];

	return nil;
}

/*
===============
addrandombot
===============
*/
void
addrandombot(int team)
{
	int i, n, num;
	float skill;
	char *value, netname[36], *teamstr;
	gclient_t *cl;

	num = 0;
	for(n = 0; n < g_numBots; n++){
		value = Info_ValueForKey(g_botInfos[n], "name");
		for(i = 0; i< g_maxclients.integer; i++){
			cl = level.clients + i;
			if(cl->pers.connected != CON_CONNECTED)
				continue;
			if(!(g_entities[i].r.svFlags & SVF_BOT))
				continue;
			if(team >= 0 && cl->sess.team != team)
				continue;
			if(!Q_stricmp(value, cl->pers.netname))
				break;
		}
		if(i >= g_maxclients.integer)
			num++;
	}
	num = random() * num;
	for(n = 0; n < g_numBots; n++){
		value = Info_ValueForKey(g_botInfos[n], "name");
		for(i = 0; i< g_maxclients.integer; i++){
			cl = level.clients + i;
			if(cl->pers.connected != CON_CONNECTED)
				continue;
			if(!(g_entities[i].r.svFlags & SVF_BOT))
				continue;
			if(team >= 0 && cl->sess.team != team)
				continue;
			if(!Q_stricmp(value, cl->pers.netname))
				break;
		}
		if(i >= g_maxclients.integer){
			num--;
			if(num <= 0){
				skill = trap_Cvar_VariableValue("g_spSkill");
				if(team == TEAM_RED)
					teamstr = "red";
				else if(team == TEAM_BLUE)
					teamstr = "blue";
				else teamstr = "";
				Q_strncpyz(netname, value, sizeof(netname));
				Q_CleanStr(netname);
				trap_SendConsoleCommand(EXEC_INSERT, va("addbot %s %f %s %i\n", netname, skill, teamstr, 0));
				return;
			}
		}
	}
}

/*
===============
removerandombot
===============
*/
int
removerandombot(int team)
{
	int i;
	gclient_t *cl;

	for(i = 0; i< g_maxclients.integer; i++){
		cl = level.clients + i;
		if(cl->pers.connected != CON_CONNECTED)
			continue;
		if(!(g_entities[i].r.svFlags & SVF_BOT))
			continue;
		if(team >= 0 && cl->sess.team != team)
			continue;
		trap_SendConsoleCommand(EXEC_INSERT, va("clientkick %d\n", i));
		return qtrue;
	}
	return qfalse;
}

/*
===============
counthumanplayers
===============
*/
int
counthumanplayers(int team)
{
	int i, num;
	gclient_t *cl;

	num = 0;
	for(i = 0; i< g_maxclients.integer; i++){
		cl = level.clients + i;
		if(cl->pers.connected != CON_CONNECTED)
			continue;
		if(g_entities[i].r.svFlags & SVF_BOT)
			continue;
		if(team >= 0 && cl->sess.team != team)
			continue;
		num++;
	}
	return num;
}

/*
===============
countbotplayers
===============
*/
int
countbotplayers(int team)
{
	int i, n, num;
	gclient_t *cl;

	num = 0;
	for(i = 0; i< g_maxclients.integer; i++){
		cl = level.clients + i;
		if(cl->pers.connected != CON_CONNECTED)
			continue;
		if(!(g_entities[i].r.svFlags & SVF_BOT))
			continue;
		if(team >= 0 && cl->sess.team != team)
			continue;
		num++;
	}
	for(n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++){
		if(!botSpawnQueue[n].spawntime)
			continue;
		if(botSpawnQueue[n].spawntime > level.time)
			continue;
		num++;
	}
	return num;
}

/*
===============
checkminimumplayers
===============
*/
void
checkminimumplayers(void)
{
	int minplayers;
	int humanplayers, botplayers;
	static int checkminimumplayers_time;

	if(level.intermissiontime)
		return;
	//only check once each 10 seconds
	if(checkminimumplayers_time > level.time - 10000)
		return;
	checkminimumplayers_time = level.time;
	trap_Cvar_Update(&bot_minplayers);
	minplayers = bot_minplayers.integer;
	if(minplayers <= 0)
		return;

	if(g_gametype.integer >= GT_TEAM){
		if(minplayers >= g_maxclients.integer / 2)
			minplayers = (g_maxclients.integer / 2) -1;

		humanplayers = counthumanplayers(TEAM_RED);
		botplayers = countbotplayers(TEAM_RED);
		if(humanplayers + botplayers < minplayers)
			addrandombot(TEAM_RED);
		else if(humanplayers + botplayers > minplayers && botplayers)
			removerandombot(TEAM_RED);
		humanplayers = counthumanplayers(TEAM_BLUE);
		botplayers = countbotplayers(TEAM_BLUE);
		if(humanplayers + botplayers < minplayers)
			addrandombot(TEAM_BLUE);
		else if(humanplayers + botplayers > minplayers && botplayers)
			removerandombot(TEAM_BLUE);
	}else if(g_gametype.integer == GT_TOURNAMENT){
		if(minplayers >= g_maxclients.integer)
			minplayers = g_maxclients.integer-1;
		humanplayers = counthumanplayers(-1);
		botplayers = countbotplayers(-1);
		if(humanplayers + botplayers < minplayers)
			addrandombot(TEAM_FREE);
		else if(humanplayers + botplayers > minplayers && botplayers)
			// try to remove spectators first
			if(!removerandombot(TEAM_SPECTATOR))
				// just remove the bot that is playing
				removerandombot(-1);
	}else if(g_gametype.integer == GT_FFA){
		if(minplayers >= g_maxclients.integer)
			minplayers = g_maxclients.integer-1;
		humanplayers = counthumanplayers(TEAM_FREE);
		botplayers = countbotplayers(TEAM_FREE);
		if(humanplayers + botplayers < minplayers)
			addrandombot(TEAM_FREE);
		else if(humanplayers + botplayers > minplayers && botplayers)
			removerandombot(TEAM_FREE);
	}
}

/*
===============
chkbotspawn
===============
*/
void
chkbotspawn(void)
{
	int n;

	checkminimumplayers();

	for(n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++){
		if(!botSpawnQueue[n].spawntime)
			continue;
		if(botSpawnQueue[n].spawntime > level.time)
			continue;
		clientbegin(botSpawnQueue[n].clientNum);
		botSpawnQueue[n].spawntime = 0;
	}
}

/*
===============
AddBotToSpawnQueue
===============
*/
static void
AddBotToSpawnQueue(int clientNum, int delay)
{
	int n;

	for(n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++)
		if(!botSpawnQueue[n].spawntime){
			botSpawnQueue[n].spawntime = level.time + delay;
			botSpawnQueue[n].clientNum = clientNum;
			return;
		}

	gprintf(S_COLOR_YELLOW "Unable to delay spawn\n");
	clientbegin(clientNum);
}

/*
===============
dequeuebotbegin

Called on client disconnect to make sure the delayed spawn
doesn't happen on a freed index
===============
*/
void
dequeuebotbegin(int clientNum)
{
	int n;

	for(n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++)
		if(botSpawnQueue[n].clientNum == clientNum){
			botSpawnQueue[n].spawntime = 0;
			return;
		}
}

/*
===============
botconnect
===============
*/
qboolean
botconnect(int clientNum, qboolean restart)
{
	bot_settings_t settings;
	char userinfo[MAX_INFO_STRING];

	trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));

	Q_strncpyz(settings.characterfile, Info_ValueForKey(userinfo, "characterfile"), sizeof(settings.characterfile));
	settings.skill = atof(Info_ValueForKey(userinfo, "skill"));
	Q_strncpyz(settings.team, Info_ValueForKey(userinfo, "team"), sizeof(settings.team));

	if(!BotAISetupClient(clientNum, &settings, restart)){
		trap_DropClient(clientNum, "BotAISetupClient failed");
		return qfalse;
	}

	return qtrue;
}

/*
===============
addbot
===============
*/
static void
addbot(const char *name, float skill, const char *team, int delay, char *altname)
{
	int clientNum;
	char *botinfo;
	char *key;
	char *s;
	char *botname;
	char *model;
	char *headmodel;
	char userinfo[MAX_INFO_STRING];

	// have the server allocate a client slot
	clientNum = trap_BotAllocateClient();
	if(clientNum == -1){
		gprintf(S_COLOR_RED "Unable to add bot. All player slots are in use.\n");
		gprintf(S_COLOR_RED "Start server with more 'open' slots (or check setting of sv_maxclients cvar).\n");
		return;
	}

	// get the botinfo from bots.txt
	botinfo = botinfobyname(name);
	if(!botinfo){
		gprintf(S_COLOR_RED "Error: Bot '%s' not defined\n", name);
		trap_BotFreeClient(clientNum);
		return;
	}

	// create the bot's userinfo
	userinfo[0] = '\0';

	botname = Info_ValueForKey(botinfo, "funname");
	if(!botname[0])
		botname = Info_ValueForKey(botinfo, "name");
	// check for an alternative name
	if(altname && altname[0])
		botname = altname;
	Info_SetValueForKey(userinfo, "name", botname);
	Info_SetValueForKey(userinfo, "rate", "25000");
	Info_SetValueForKey(userinfo, "snaps", "20");
	Info_SetValueForKey(userinfo, "skill", va("%.2f", skill));

	if(skill >= 1 && skill < 2)
		Info_SetValueForKey(userinfo, "handicap", "50");
	else if(skill >= 2 && skill < 3)
		Info_SetValueForKey(userinfo, "handicap", "70");
	else if(skill >= 3 && skill < 4)
		Info_SetValueForKey(userinfo, "handicap", "90");

	key = "model";
	model = Info_ValueForKey(botinfo, key);
	if(!*model)
		model = "visor/default";
	Info_SetValueForKey(userinfo, key, model);
	key = "team_model";
	Info_SetValueForKey(userinfo, key, model);

	key = "headmodel";
	headmodel = Info_ValueForKey(botinfo, key);
	if(!*headmodel)
		headmodel = model;
	Info_SetValueForKey(userinfo, key, headmodel);
	key = "team_headmodel";
	Info_SetValueForKey(userinfo, key, headmodel);

	key = "gender";
	s = Info_ValueForKey(botinfo, key);
	if(!*s)
		s = "male";
	Info_SetValueForKey(userinfo, "sex", s);

	key = "color1";
	s = Info_ValueForKey(botinfo, key);
	if(!*s)
		s = "4";
	Info_SetValueForKey(userinfo, key, s);

	key = "color2";
	s = Info_ValueForKey(botinfo, key);
	if(!*s)
		s = "5";
	Info_SetValueForKey(userinfo, key, s);

	s = Info_ValueForKey(botinfo, "aifile");
	if(!*s){
		trap_Print(S_COLOR_RED "Error: bot has no aifile specified\n");
		trap_BotFreeClient(clientNum);
		return;
	}
	Info_SetValueForKey(userinfo, "characterfile", s);

	if(!team || !*team){
		if(g_gametype.integer >= GT_TEAM){
			if(pickteam(clientNum) == TEAM_RED)
				team = "red";
			else
				team = "blue";
		}else
			team = "red";
	}
	Info_SetValueForKey(userinfo, "team", team);

	// register the userinfo
	trap_SetUserinfo(clientNum, userinfo);

	// have it connect to the game as a normal client
	if(clientconnect(clientNum, qtrue, qtrue))
		return;

	if(delay == 0){
		clientbegin(clientNum);
		return;
	}

	AddBotToSpawnQueue(clientNum, delay);
}

/*
===============
Svcmd_AddBot_f
===============
*/
void
Svcmd_AddBot_f(void)
{
	float skill;
	int delay;
	char name[MAX_TOKEN_CHARS];
	char altname[MAX_TOKEN_CHARS];
	char string[MAX_TOKEN_CHARS];
	char team[MAX_TOKEN_CHARS];

	// are bots enabled?
	if(!trap_Cvar_VariableIntegerValue("bot_enable"))
		return;

	// name
	trap_Argv(1, name, sizeof(name));
	if(!name[0]){
		trap_Print("Usage: Addbot <botname> [skill 1-5] [team] [msec delay] [altname]\n");
		return;
	}

	// skill
	trap_Argv(2, string, sizeof(string));
	if(!string[0])
		skill = 4;
	else
		skill = atof(string);

	// team
	trap_Argv(3, team, sizeof(team));

	// delay
	trap_Argv(4, string, sizeof(string));
	if(!string[0])
		delay = 0;
	else
		delay = atoi(string);

	// alternative name
	trap_Argv(5, altname, sizeof(altname));

	addbot(name, skill, team, delay, altname);

	// if this was issued during gameplay and we are playing locally,
	// go ahead and load the bot's media immediately
	if(level.time - level.starttime > 1000 &&
	   trap_Cvar_VariableIntegerValue("cl_running"))
		trap_SendServerCommand(-1, "loaddefered\n");	// FIXME: spelled wrong, but not changing for demo
}

/*
===============
Svcmd_BotList_f
===============
*/
void
Svcmd_BotList_f(void)
{
	int i;
	char name[MAX_TOKEN_CHARS];
	char funname[MAX_TOKEN_CHARS];
	char model[MAX_TOKEN_CHARS];
	char aifile[MAX_TOKEN_CHARS];

	trap_Print("^1name             model            aifile              funname\n");
	for(i = 0; i < g_numBots; i++){
		strcpy(name, Info_ValueForKey(g_botInfos[i], "name"));
		if(!*name)
			strcpy(name, "UnnamedPlayer");
		strcpy(funname, Info_ValueForKey(g_botInfos[i], "funname"));
		if(!*funname)
			strcpy(funname, "");
		strcpy(model, Info_ValueForKey(g_botInfos[i], "model"));
		if(!*model)
			strcpy(model, "visor/default");
		strcpy(aifile, Info_ValueForKey(g_botInfos[i], "aifile"));
		if(!*aifile)
			strcpy(aifile, "bots/default_c.c");
		trap_Print(va("%-16s %-16s %-20s %-20s\n", name, model, aifile, funname));
	}
}

/*
===============
spawnbots
===============
*/
static void
spawnbots(char *botList, int baseDelay)
{
	char *bot;
	char *p;
	float skill;
	int delay;
	char bots[MAX_INFO_VALUE];

	podium1 = nil;
	podium2 = nil;
	podium3 = nil;

	skill = trap_Cvar_VariableValue("g_spSkill");
	if(skill < 1){
		trap_Cvar_Set("g_spSkill", "1");
		skill = 1;
	}else if(skill > 5){
		trap_Cvar_Set("g_spSkill", "5");
		skill = 5;
	}

	Q_strncpyz(bots, botList, sizeof(bots));
	p = &bots[0];
	delay = baseDelay;
	while(*p){
		//skip spaces
		while(*p && *p == ' ')
			p++;
		if(!*p)
			break;

		// mark start of bot name
		bot = p;

		// skip until space of null
		while(*p && *p != ' ')
			p++;
		if(*p)
			*p++ = 0;

		// we must add the bot this way, calling addbot directly at this stage
		// does "Bad Things"
		trap_SendConsoleCommand(EXEC_INSERT, va("addbot %s %f free %i\n", bot, skill, delay));

		delay += BOT_BEGIN_DELAY_INCREMENT;
	}
}

/*
===============
loadbotsfromfile
===============
*/
static void
loadbotsfromfile(char *filename)
{
	int len;
	fileHandle_t f;
	char buf[MAX_BOTS_TEXT];

	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if(!f){
		trap_Print(va(S_COLOR_RED "file not found: %s\n", filename));
		return;
	}
	if(len >= MAX_BOTS_TEXT){
		trap_Print(va(S_COLOR_RED "file too large: %s is %i, max allowed is %i\n", filename, len, MAX_BOTS_TEXT));
		trap_FS_FCloseFile(f);
		return;
	}

	trap_FS_Read(buf, len, f);
	buf[len] = 0;
	trap_FS_FCloseFile(f);

	g_numBots += parseinfos(buf, MAX_BOTS - g_numBots, &g_botInfos[g_numBots]);
}

/*
===============
loadbots
===============
*/
static void
loadbots(void)
{
	vmCvar_t botsFile;
	int numdirs;
	char filename[128];
	char dirlist[1024];
	char *dirptr;
	int i;
	int dirlen;

	if(!trap_Cvar_VariableIntegerValue("bot_enable"))
		return;

	g_numBots = 0;

	trap_Cvar_Register(&botsFile, "g_botsFile", "", CVAR_INIT|CVAR_ROM);
	if(*botsFile.string)
		loadbotsfromfile(botsFile.string);
	else
		loadbotsfromfile("scripts/bots.txt");

	// get all bots from .bot files
	numdirs = trap_FS_GetFileList("scripts", ".bot", dirlist, 1024);
	dirptr = dirlist;
	for(i = 0; i < numdirs; i++, dirptr += dirlen+1){
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/");
		strcat(filename, dirptr);
		loadbotsfromfile(filename);
	}
	trap_Print(va("%i bots parsed\n", g_numBots));
}

/*
===============
botinfo
===============
*/
char *
botinfo(int num)
{
	if(num < 0 || num >= g_numBots){
		trap_Print(va(S_COLOR_RED "Invalid bot number: %i\n", num));
		return nil;
	}
	return g_botInfos[num];
}

/*
===============
botinfobyname
===============
*/
char *
botinfobyname(const char *name)
{
	int n;
	char *value;

	for(n = 0; n < g_numBots; n++){
		value = Info_ValueForKey(g_botInfos[n], "name");
		if(!Q_stricmp(value, name))
			return g_botInfos[n];
	}

	return nil;
}

/*
===============
initbots
===============
*/
void
initbots(qboolean restart)
{
	int fragLimit;
	int timeLimit;
	const char *arenainfo;
	char *strValue;
	int basedelay;
	char map[MAX_QPATH];
	char serverinfo[MAX_INFO_STRING];

	loadbots();
	loadarenas();

	trap_Cvar_Register(&bot_minplayers, "bot_minplayers", "0", CVAR_SERVERINFO);

	if(g_gametype.integer == GT_SINGLE_PLAYER){
		trap_GetServerinfo(serverinfo, sizeof(serverinfo));
		Q_strncpyz(map, Info_ValueForKey(serverinfo, "mapname"), sizeof(map));
		arenainfo = getarenainfobymap(map);
		if(!arenainfo)
			return;

		strValue = Info_ValueForKey(arenainfo, "fraglimit");
		fragLimit = atoi(strValue);
		if(fragLimit)
			trap_Cvar_Set("fraglimit", strValue);
		else
			trap_Cvar_Set("fraglimit", "0");

		strValue = Info_ValueForKey(arenainfo, "timelimit");
		timeLimit = atoi(strValue);
		if(timeLimit)
			trap_Cvar_Set("timelimit", strValue);
		else
			trap_Cvar_Set("timelimit", "0");

		if(!fragLimit && !timeLimit){
			trap_Cvar_Set("fraglimit", "10");
			trap_Cvar_Set("timelimit", "0");
		}

		basedelay = BOT_BEGIN_DELAY_BASE;
		strValue = Info_ValueForKey(arenainfo, "special");
		if(Q_stricmp(strValue, "training") == 0)
			basedelay += 10000;

		if(!restart)
			spawnbots(Info_ValueForKey(arenainfo, "bots"), basedelay);
	}
}
