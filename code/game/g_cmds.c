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


/*
==================
deathmatchscoreboardmsg

==================
*/
void
deathmatchscoreboardmsg(ent_t *ent)
{
	char entry[1024];
	char string[1400];
	int stringlength;
	int i, j;
	gclient_t *cl;
	int numSorted, scoreflags, accuracy, perfect;

	// send the latest information on all clients
	string[0] = 0;
	stringlength = 0;
	scoreflags = 0;

	numSorted = level.nconnectedclients;

	for(i = 0; i < numSorted; i++){
		int ping;

		cl = &level.clients[level.sortedclients[i]];

		if(cl->pers.connected == CON_CONNECTING)
			ping = -1;
		else
			ping = cl->ps.ping < 999 ? cl->ps.ping : 999;

		if(cl->accuracyshots)
			accuracy = cl->accuracyhits * 100 / cl->accuracyshots;
		else
			accuracy = 0;
		perfect = (cl->ps.persistant[PERS_RANK] == 0 && cl->ps.persistant[PERS_KILLED] == 0) ? 1 : 0;

		Com_sprintf(entry, sizeof(entry),
			    " %i %i %i %i %i %i %i %i %i %i %i %i %i %i", level.sortedclients[i],
			    cl->ps.persistant[PERS_SCORE], ping, (level.time - cl->pers.entertime)/60000,
			    scoreflags, g_entities[level.sortedclients[i]].s.powerups, accuracy,
			    cl->ps.persistant[PERS_IMPRESSIVE_COUNT],
			    cl->ps.persistant[PERS_EXCELLENT_COUNT],
			    cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT],
			    cl->ps.persistant[PERS_DEFEND_COUNT],
			    cl->ps.persistant[PERS_ASSIST_COUNT],
			    perfect,
			    cl->ps.persistant[PERS_CAPTURES]);
		j = strlen(entry);
		if(stringlength + j >= sizeof(string))
			break;
		strcpy(string + stringlength, entry);
		stringlength += j;
	}

	trap_SendServerCommand(ent-g_entities, va("scores %i %i %i%s", i,
						  level.teamscores[TEAM_RED], level.teamscores[TEAM_BLUE],
						  string));
}

/*
==================
Cmd_Score_f

Request current scoreboard information
==================
*/
void
Cmd_Score_f(ent_t *ent)
{
	deathmatchscoreboardmsg(ent);
}

/*
==================
CheatsOk
==================
*/
qboolean
CheatsOk(ent_t *ent)
{
	if(!g_cheats.integer){
		trap_SendServerCommand(ent-g_entities, "print \"Cheats are not enabled on this server.\n\"");
		return qfalse;
	}
	if(ent->health <= 0){
		trap_SendServerCommand(ent-g_entities, "print \"You must be alive to use this command.\n\"");
		return qfalse;
	}
	return qtrue;
}

/*
==================
ConcatArgs
==================
*/
char    *
ConcatArgs(int start)
{
	int i, c, tlen;
	static char line[MAX_STRING_CHARS];
	int len;
	char arg[MAX_STRING_CHARS];

	len = 0;
	c = trap_Argc();
	for(i = start; i < c; i++){
		trap_Argv(i, arg, sizeof(arg));
		tlen = strlen(arg);
		if(len + tlen >= MAX_STRING_CHARS - 1)
			break;
		memcpy(line + len, arg, tlen);
		len += tlen;
		if(i != c - 1){
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

/*
==================
StringIsInteger
==================
*/
qboolean
StringIsInteger(const char *s)
{
	int i;
	int len;
	qboolean foundDigit;

	len = strlen(s);
	foundDigit = qfalse;

	for(i = 0; i < len; i++){
		if(!isdigit(s[i]))
			return qfalse;

		foundDigit = qtrue;
	}

	return foundDigit;
}

/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
int
ClientNumberFromString(ent_t *to, char *s)
{
	gclient_t *cl;
	int idnum;
	char cleanName[MAX_STRING_CHARS];

	// numeric values could be slot numbers
	if(StringIsInteger(s)){
		idnum = atoi(s);
		if(idnum >= 0 && idnum < level.maxclients){
			cl = &level.clients[idnum];
			if(cl->pers.connected == CON_CONNECTED)
				return idnum;
		}
	}

	// check for a name match
	for(idnum = 0, cl = level.clients; idnum < level.maxclients; idnum++, cl++){
		if(cl->pers.connected != CON_CONNECTED)
			continue;
		Q_strncpyz(cleanName, cl->pers.netname, sizeof(cleanName));
		Q_CleanStr(cleanName);
		if(!Q_stricmp(cleanName, s))
			return idnum;
	}

	trap_SendServerCommand(to-g_entities, va("print \"User %s is not on the server\n\"", s));
	return -1;
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void
Cmd_Give_f(ent_t *ent)
{
	char *name;
	item_t *it;
	int i;
	qboolean give_all;
	ent_t *it_ent;
	trace_t trace;

	if(!CheatsOk(ent))
		return;

	name = ConcatArgs(1);

	if(Q_stricmp(name, "all") == 0)
		give_all = qtrue;
	else
		give_all = qfalse;

	if(give_all || Q_stricmp(name, "tokens") == 0){
		ent->client->ps.stats[STAT_TOKENS] = 999;
		if(!give_all)
			return;
	}

	if(give_all || Q_stricmp(name, "lives") == 0){
		ent->client->ps.persistant[PERS_LIVES] = 999;
		if(!give_all)
			return;
	}

	if(give_all || Q_stricmp(name, "weapons") == 0){
		ent->client->ps.stats[STAT_WEAPONS] = (1 << WP_NUM_WEAPONS) - 1 -
						      (1 << WP_GRAPPLING_HOOK) - (1 << WP_NONE);
		if(!give_all)
			return;
	}

	if(give_all || Q_stricmp(name, "ammo") == 0){
		for(i = 0; i < MAX_WEAPONS; i++)
			ent->client->ps.ammo[i] = 999;
		if(!give_all)
			return;
	}

	if(give_all || Q_stricmp(name, "armor") == 0){
		ent->client->ps.stats[STAT_ARMOR] = 200;

		if(!give_all)
			return;
	}

	if(Q_stricmp(name, "excellent") == 0){
		ent->client->ps.persistant[PERS_EXCELLENT_COUNT]++;
		return;
	}
	if(Q_stricmp(name, "impressive") == 0){
		ent->client->ps.persistant[PERS_IMPRESSIVE_COUNT]++;
		return;
	}
	if(Q_stricmp(name, "gauntletaward") == 0){
		ent->client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT]++;
		return;
	}
	if(Q_stricmp(name, "defend") == 0){
		ent->client->ps.persistant[PERS_DEFEND_COUNT]++;
		return;
	}
	if(Q_stricmp(name, "assist") == 0){
		ent->client->ps.persistant[PERS_ASSIST_COUNT]++;
		return;
	}

	// spawn a specific item right on the player
	if(!give_all){
		it = finditem(name);
		if(!it)
			return;

		it_ent = entspawn();
		veccpy(ent->r.currentOrigin, it_ent->s.origin);
		it_ent->classname = it->classname;
		itemspawn(it_ent, it);
		itemspawnfinish(it_ent);
		memset(&trace, 0, sizeof(trace));
		item_touch(it_ent, ent, &trace);
		if(it_ent->inuse)
			entfree(it_ent);
	}
}

/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void
Cmd_God_f(ent_t *ent)
{
	char *msg;

	if(!CheatsOk(ent))
		return;

	ent->flags ^= FL_GODMODE;
	if(!(ent->flags & FL_GODMODE))
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	trap_SendServerCommand(ent-g_entities, va("print \"%s\"", msg));
}

/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void
Cmd_Notarget_f(ent_t *ent)
{
	char *msg;

	if(!CheatsOk(ent))
		return;

	ent->flags ^= FL_NOTARGET;
	if(!(ent->flags & FL_NOTARGET))
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	trap_SendServerCommand(ent-g_entities, va("print \"%s\"", msg));
}

/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void
Cmd_Noclip_f(ent_t *ent)
{
	char *msg;

	if(!CheatsOk(ent))
		return;

	if(ent->client->noclip)
		msg = "noclip OFF\n";
	else
		msg = "noclip ON\n";
	ent->client->noclip = !ent->client->noclip;

	trap_SendServerCommand(ent-g_entities, va("print \"%s\"", msg));
}

/*
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
void
Cmd_LevelShot_f(ent_t *ent)
{
	if(!ent->client->pers.localclient){
		trap_SendServerCommand(ent-g_entities,
				       "print \"The levelshot command must be executed by a local client\n\"");
		return;
	}

	if(!CheatsOk(ent))
		return;

	// doesn't work in single player
	if(g_gametype.integer == GT_SINGLE_PLAYER){
		trap_SendServerCommand(ent-g_entities,
				       "print \"Must not be in singleplayer mode for levelshot\n\"");
		return;
	}

	intermission();
	trap_SendServerCommand(ent-g_entities, "clientLevelShot");
}

/*
==================
Cmd_TeamTask_f
==================
*/
void
Cmd_TeamTask_f(ent_t *ent)
{
	char userinfo[MAX_INFO_STRING];
	char arg[MAX_TOKEN_CHARS];
	int task;
	int client = ent->client - level.clients;

	if(trap_Argc() != 2)
		return;
	trap_Argv(1, arg, sizeof(arg));
	task = atoi(arg);

	trap_GetUserinfo(client, userinfo, sizeof(userinfo));
	Info_SetValueForKey(userinfo, "teamtask", va("%d", task));
	trap_SetUserinfo(client, userinfo);
	clientuserinfochanged(client);
}

/*
=================
Cmd_Kill_f
=================
*/
void
Cmd_Kill_f(ent_t *ent)
{
	if(ent->client->sess.team == TEAM_SPECTATOR)
		return;
	if(ent->health <= 0)
		return;
	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = -999;
	player_die(ent, ent, ent, 100000, MOD_SUICIDE);
}

/*
=================
broadcastteamchange

Let everyone know about a team change
=================
*/
void
broadcastteamchange(gclient_t *client, int oldTeam)
{
	if(client->sess.team == TEAM_RED)
		trap_SendServerCommand(-1, va("cp \"%s" S_COLOR_WHITE " joined the red team.\n\"",
					      client->pers.netname));
	else if(client->sess.team == TEAM_BLUE)
		trap_SendServerCommand(-1, va("cp \"%s" S_COLOR_WHITE " joined the blue team.\n\"",
					      client->pers.netname));
	else if(client->sess.team == TEAM_SPECTATOR && oldTeam != TEAM_SPECTATOR)
		trap_SendServerCommand(-1, va("cp \"%s" S_COLOR_WHITE " joined the spectators.\n\"",
					      client->pers.netname));
	else if(client->sess.team == TEAM_FREE)
		trap_SendServerCommand(-1, va("cp \"%s" S_COLOR_WHITE " joined the battle.\n\"",
					      client->pers.netname));
}

/*
=================
setteam
=================
*/
void
setteam(ent_t *ent, char *s)
{
	int team, oldTeam;
	gclient_t *client;
	int clientNum;
	specstate_t specState;
	int specClient;
	int leader;

	// see what change is requested
	client = ent->client;

	clientNum = client - level.clients;
	specClient = 0;
	specState = SPECTATOR_NOT;
	if(!Q_stricmp(s, "scoreboard") || !Q_stricmp(s, "score")){
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_SCOREBOARD;
	}else if(!Q_stricmp(s, "follow1")){
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -1;
	}else if(!Q_stricmp(s, "follow2")){
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -2;
	}else if(!Q_stricmp(s, "spectator") || !Q_stricmp(s, "s")){
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FREE;
	}else if(g_gametype.integer >= GT_TEAM){
		// if running a team game, assign player to one of the teams
		specState = SPECTATOR_NOT;
		if(!Q_stricmp(s, "red") || !Q_stricmp(s, "r"))
			team = TEAM_RED;
		else if(!Q_stricmp(s, "blue") || !Q_stricmp(s, "b"))
			team = TEAM_BLUE;
		else
			// pick the team with the least number of players
			team = pickteam(clientNum);

		if(g_teamForceBalance.integer){
			int counts[TEAM_NUM_TEAMS];

			counts[TEAM_BLUE] = teamcount(clientNum, TEAM_BLUE);
			counts[TEAM_RED] = teamcount(clientNum, TEAM_RED);

			// We allow a spread of two
			if(team == TEAM_RED && counts[TEAM_RED] - counts[TEAM_BLUE] > 1){
				trap_SendServerCommand(clientNum,
						       "cp \"Red team has too many players.\n\"");
				return;	// ignore the request
			}
			if(team == TEAM_BLUE && counts[TEAM_BLUE] - counts[TEAM_RED] > 1){
				trap_SendServerCommand(clientNum,
						       "cp \"Blue team has too many players.\n\"");
				return;	// ignore the request
			}

			// It's ok, the team we are switching to has less or same number of players
		}
	}else
		// force them to spectators if there aren't any spots free
		team = TEAM_FREE;

	// override decision if limiting the players
	if((g_gametype.integer == GT_TOURNAMENT)
	   && level.nnonspecclients >= 2)
		team = TEAM_SPECTATOR;
	else if(g_maxGameClients.integer > 0 &&
		level.nnonspecclients >= g_maxGameClients.integer)
		team = TEAM_SPECTATOR;

	// decide if we will allow the change
	oldTeam = client->sess.team;
	if(team == oldTeam && team != TEAM_SPECTATOR)
		return;

	// execute the team change

	// if the player was dead leave the body
	if(client->ps.stats[STAT_HEALTH] <= 0)
		copytobodyqueue(ent);

	// he starts at 'base'
	client->pers.teamstate.state = TEAM_BEGIN;
	if(oldTeam != TEAM_SPECTATOR){
		// Kill him (makes sure he loses flags, etc)
		ent->flags &= ~FL_GODMODE;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
		player_die(ent, ent, ent, 100000, MOD_SUICIDE);
	}

	// they go to the end of the line for tournements
	if(team == TEAM_SPECTATOR && oldTeam != team)
		addtourneyqueue(client);

	client->sess.team = team;
	client->sess.specstate = specState;
	client->sess.specclient = specClient;

	client->sess.teamleader = qfalse;
	if(team == TEAM_RED || team == TEAM_BLUE){
		leader = teamleader(team);
		// if there is no team leader or the team leader is a bot and this client is not a bot
		if(leader == -1 || (!(g_entities[clientNum].r.svFlags & SVF_BOT) && (g_entities[leader].r.svFlags & SVF_BOT)))
			setleader(team, clientNum);
	}
	// make sure there is a team leader on the team the player came from
	if(oldTeam == TEAM_RED || oldTeam == TEAM_BLUE)
		chkteamleader(oldTeam);

	broadcastteamchange(client, oldTeam);

	// get and distribute relevent paramters
	clientuserinfochanged(clientNum);

	clientbegin(clientNum);
}

/*
=================
stopfollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
void
stopfollowing(ent_t *ent)
{
	ent->client->ps.persistant[PERS_TEAM] = TEAM_SPECTATOR;
	ent->client->sess.team = TEAM_SPECTATOR;
	ent->client->sess.specstate = SPECTATOR_FREE;
	ent->client->ps.pm_flags &= ~PMF_FOLLOW;
	ent->r.svFlags &= ~SVF_BOT;
	ent->client->ps.clientNum = ent - g_entities;

	setviewangles(ent, ent->client->ps.viewangles);

	// don't use dead view angles
	if(ent->client->ps.stats[STAT_HEALTH] <= 0)
		ent->client->ps.stats[STAT_HEALTH] = 1;
}

/*
=================
Cmd_Team_f
=================
*/
void
Cmd_Team_f(ent_t *ent)
{
	int oldTeam;
	char s[MAX_TOKEN_CHARS];

	if(trap_Argc() != 2){
		oldTeam = ent->client->sess.team;
		switch(oldTeam){
		case TEAM_BLUE:
			trap_SendServerCommand(ent-g_entities, "print \"Blue team\n\"");
			break;
		case TEAM_RED:
			trap_SendServerCommand(ent-g_entities, "print \"Red team\n\"");
			break;
		case TEAM_FREE:
			trap_SendServerCommand(ent-g_entities, "print \"Free team\n\"");
			break;
		case TEAM_SPECTATOR:
			trap_SendServerCommand(ent-g_entities, "print \"Spectator team\n\"");
			break;
		}
		return;
	}

	if(ent->client->switchteamtime > level.time){
		trap_SendServerCommand(ent-g_entities, "print \"May not switch teams more than once per 5 seconds.\n\"");
		return;
	}

	trap_Argv(1, s, sizeof(s));
	
	if(Q_stricmp(s, "spectator") != 0 && Q_stricmp(s, "s") != 0 &&
	   !CheatsOk(ent) && ent->client->deathspec){
		return;
	}

	// if they are playing a tournament game, count as a loss
	if((g_gametype.integer == GT_TOURNAMENT)
	   && ent->client->sess.team == TEAM_FREE)
		ent->client->sess.losses++;

	setteam(ent, s);

	ent->client->switchteamtime = level.time + 5000;
}

/*
=================
Cmd_Follow_f
=================
*/
void
Cmd_Follow_f(ent_t *ent)
{
	int i;
	char arg[MAX_TOKEN_CHARS];

	if(trap_Argc() != 2){
		if(ent->client->sess.specstate == SPECTATOR_FOLLOW)
			stopfollowing(ent);
		return;
	}

	trap_Argv(1, arg, sizeof(arg));
	i = ClientNumberFromString(ent, arg);
	if(i == -1)
		return;

	// can't follow self
	if(&level.clients[i] == ent->client)
		return;

	// can't follow another spectator
	if(level.clients[i].sess.team == TEAM_SPECTATOR)
		return;

	// if they are playing a tournement game, count as a loss
	if((g_gametype.integer == GT_TOURNAMENT)
	   && ent->client->sess.team == TEAM_FREE)
		ent->client->sess.losses++;

	// first set them to spectator
	if(ent->client->sess.team != TEAM_SPECTATOR)
		setteam(ent, "spectator");

	ent->client->sess.specstate = SPECTATOR_FOLLOW;
	ent->client->sess.specclient = i;
}

/*
=================
Cmd_FollowCycle_f
=================
*/
void
Cmd_FollowCycle_f(ent_t *ent, int dir)
{
	int clientnum;
	int original;

	// if they are playing a tournement game, count as a loss
	if((g_gametype.integer == GT_TOURNAMENT)
	   && ent->client->sess.team == TEAM_FREE)
		ent->client->sess.losses++;
	// first set them to spectator
	if(ent->client->sess.specstate == SPECTATOR_NOT)
		setteam(ent, "spectator");

	if(dir != 1 && dir != -1)
		errorf("Cmd_FollowCycle_f: bad dir %i", dir);

	// if dedicated follow client, just switch between the two auto clients
	if(ent->client->sess.specclient < 0){
		if(ent->client->sess.specclient == -1)
			ent->client->sess.specclient = -2;
		else if(ent->client->sess.specclient == -2)
			ent->client->sess.specclient = -1;
		return;
	}

	clientnum = ent->client->sess.specclient;
	original = clientnum;
	do{
		clientnum += dir;
		if(clientnum >= level.maxclients)
			clientnum = 0;
		if(clientnum < 0)
			clientnum = level.maxclients - 1;

		// can only follow connected clients
		if(level.clients[clientnum].pers.connected != CON_CONNECTED)
			continue;

		// can't follow another spectator
		if(level.clients[clientnum].sess.team == TEAM_SPECTATOR)
			continue;

		// this is good, we can use it
		ent->client->sess.specclient = clientnum;
		ent->client->sess.specstate = SPECTATOR_FOLLOW;
		return;
	}while(clientnum != original)
	;

	// leave it where it was
}

/*
==================
say
==================
*/

static void
sayto(ent_t *ent, ent_t *other, int mode, int color, const char *name, const char *message)
{
	if(!other)
		return;
	if(!other->inuse)
		return;
	if(!other->client)
		return;
	if(other->client->pers.connected != CON_CONNECTED)
		return;
	if(mode == SAY_TEAM  && !onsameteam(ent, other))
		return;
	// no chatting to players in tournements
	if((g_gametype.integer == GT_TOURNAMENT)
	   && other->client->sess.team == TEAM_FREE
	   && ent->client->sess.team != TEAM_FREE)
		return;

	trap_SendServerCommand(other-g_entities, va("%s \"%s%c%c%s\"",
						    mode == SAY_TEAM ? "tchat" : "chat",
						    name, Q_COLOR_ESCAPE, color, message));
}

#define EC "\x19"

void
say(ent_t *ent, ent_t *target, int mode, const char *chatText)
{
	int j;
	ent_t *other;
	int color;
	char name[64];
	// don't let text be too long for malicious reasons
	char text[MAX_SAY_TEXT];
	char location[64];

	if(g_gametype.integer < GT_TEAM && mode == SAY_TEAM)
		mode = SAY_ALL;

	switch(mode){
	default:
	case SAY_ALL:
		logprintf("say: %s: %s\n", ent->client->pers.netname, chatText);
		Com_sprintf(name, sizeof(name), "%s%c%c"EC ": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
		color = COLOR_GREEN;
		break;
	case SAY_TEAM:
		logprintf("sayteam: %s: %s\n", ent->client->pers.netname, chatText);
		if(teamgetlocationmsg(ent, location, sizeof(location)))
			Com_sprintf(name, sizeof(name), EC "(%s%c%c"EC ") (%s)"EC ": ",
				    ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location);
		else
			Com_sprintf(name, sizeof(name), EC "(%s%c%c"EC ")"EC ": ",
				    ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
		color = COLOR_CYAN;
		break;
	case SAY_TELL:
		if(target && target->inuse && target->client && g_gametype.integer >= GT_TEAM &&
		   target->client->sess.team == ent->client->sess.team &&
		   teamgetlocationmsg(ent, location, sizeof(location)))
			Com_sprintf(name, sizeof(name), EC "[%s%c%c"EC "] (%s)"EC ": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location);
		else
			Com_sprintf(name, sizeof(name), EC "[%s%c%c"EC "]"EC ": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
		color = COLOR_MAGENTA;
		break;
	}

	Q_strncpyz(text, chatText, sizeof(text));

	if(target){
		sayto(ent, target, mode, color, name, text);
		return;
	}

	// echo the text to the console
	if(g_dedicated.integer)
		gprintf("%s%s\n", name, text);

	// send it to all the apropriate clients
	for(j = 0; j < level.maxclients; j++){
		other = &g_entities[j];
		sayto(ent, other, mode, color, name, text);
	}
}

/*
==================
Cmd_Say_f
==================
*/
static void
Cmd_Say_f(ent_t *ent, int mode, qboolean arg0)
{
	char *p;

	if(trap_Argc() < 2 && !arg0)
		return;

	if(arg0)
		p = ConcatArgs(0);
	else
		p = ConcatArgs(1);

	say(ent, nil, mode, p);
}

/*
==================
Cmd_Tell_f
==================
*/
static void
Cmd_Tell_f(ent_t *ent)
{
	int targetNum;
	ent_t *target;
	char *p;
	char arg[MAX_TOKEN_CHARS];

	if(trap_Argc() < 3){
		trap_SendServerCommand(ent-g_entities, "print \"Usage: tell <player id> <message>\n\"");
		return;
	}

	trap_Argv(1, arg, sizeof(arg));
	targetNum = ClientNumberFromString(ent, arg);
	if(targetNum == -1)
		return;

	target = &g_entities[targetNum];
	if(!target->inuse || !target->client)
		return;

	p = ConcatArgs(2);

	logprintf("tell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, p);
	say(ent, target, SAY_TELL, p);
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if(ent != target && !(ent->r.svFlags & SVF_BOT))
		say(ent, ent, SAY_TELL, p);
}


static char *gc_orders[] = {
	"hold your position",
	"hold this position",
	"come here",
	"cover me",
	"guard location",
	"search and destroy",
	"report"
};

static const int numgc_orders = ARRAY_LEN(gc_orders);

void
Cmd_GameCommand_f(ent_t *ent)
{
	int targetNum;
	ent_t *target;
	int order;
	char arg[MAX_TOKEN_CHARS];

	if(trap_Argc() != 3){
		trap_SendServerCommand(ent-g_entities, va("print \"Usage: gc <player id> <order 0-%d>\n\"", numgc_orders - 1));
		return;
	}

	trap_Argv(2, arg, sizeof(arg));
	order = atoi(arg);

	if(order < 0 || order >= numgc_orders){
		trap_SendServerCommand(ent-g_entities, va("print \"Bad order: %i\n\"", order));
		return;
	}

	trap_Argv(1, arg, sizeof(arg));
	targetNum = ClientNumberFromString(ent, arg);
	if(targetNum == -1)
		return;

	target = &g_entities[targetNum];
	if(!target->inuse || !target->client)
		return;

	logprintf("tell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, gc_orders[order]);
	say(ent, target, SAY_TELL, gc_orders[order]);
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if(ent != target && !(ent->r.svFlags & SVF_BOT))
		say(ent, ent, SAY_TELL, gc_orders[order]);
}

/*
==================
Cmd_Where_f
==================
*/
void
Cmd_Where_f(ent_t *ent)
{
	trap_SendServerCommand(ent-g_entities, va("print \"%s\n\"", vtos(ent->r.currentOrigin)));
}

static const char *gameNames[] = {
	"Free For All",
	"Tournament",
	"Single Player",
	"Team Deathmatch",
	"Capture the Flag",
	"One Flag CTF",
	"Overload",
	"Harvester"
};

/*
==================
Cmd_CallVote_f
==================
*/
void
Cmd_CallVote_f(ent_t *ent)
{
	char *c;
	int i;
	char arg1[MAX_STRING_TOKENS];
	char arg2[MAX_STRING_TOKENS];

	if(!g_allowVote.integer){
		trap_SendServerCommand(ent-g_entities, "print \"Voting not allowed here.\n\"");
		return;
	}

	if(level.votetime){
		trap_SendServerCommand(ent-g_entities, "print \"A vote is already in progress.\n\"");
		return;
	}
	if(ent->client->pers.votecount >= MAX_VOTE_COUNT){
		trap_SendServerCommand(ent-g_entities, "print \"You have called the maximum number of votes.\n\"");
		return;
	}
	if(ent->client->sess.team == TEAM_SPECTATOR){
		trap_SendServerCommand(ent-g_entities, "print \"Not allowed to call a vote as spectator.\n\"");
		return;
	}

	// make sure it is a valid command to vote on
	trap_Argv(1, arg1, sizeof(arg1));
	trap_Argv(2, arg2, sizeof(arg2));

	// check for command separators in arg2
	for(c = arg2; *c; ++c)
		switch(*c){
		case '\n':
		case '\r':
		case ';':
			trap_SendServerCommand(ent-g_entities, "print \"Invalid vote string.\n\"");
			return;
			break;
		}

	if(!Q_stricmp(arg1, "map_restart")){
	}else if(!Q_stricmp(arg1, "nextmap")){
	}else if(!Q_stricmp(arg1, "map")){
	}else if(!Q_stricmp(arg1, "g_gametype")){
	}else if(!Q_stricmp(arg1, "kick")){
	}else if(!Q_stricmp(arg1, "clientkick")){
	}else if(!Q_stricmp(arg1, "g_doWarmup")){
	}else if(!Q_stricmp(arg1, "timelimit")){
	}else if(!Q_stricmp(arg1, "fraglimit")){
	}else{
		trap_SendServerCommand(ent-g_entities, "print \"Invalid vote string.\n\"");
		trap_SendServerCommand(ent-g_entities, "print \"Vote commands are: map_restart, nextmap, map <mapname>, g_gametype <n>, kick <player>, clientkick <clientnum>, g_doWarmup, timelimit <time>, fraglimit <frags>.\n\"");
		return;
	}

	// if there is still a vote to be executed
	if(level.voteexectime){
		level.voteexectime = 0;
		trap_SendConsoleCommand(EXEC_APPEND, va("%s\n", level.votestr));
	}

	// special case for g_gametype, check for bad values
	if(!Q_stricmp(arg1, "g_gametype")){
		i = atoi(arg2);
		if(i == GT_SINGLE_PLAYER || i < 0 || i >= GT_MAX_GAME_TYPE){
			trap_SendServerCommand(ent-g_entities, "print \"Invalid gametype.\n\"");
			return;
		}

		Com_sprintf(level.votestr, sizeof(level.votestr), "%s %d", arg1, i);
		Com_sprintf(level.votedisplaystr, sizeof(level.votedisplaystr), "%s %s", arg1, gameNames[i]);
	}else if(!Q_stricmp(arg1, "map")){
		// special case for map changes, we want to reset the nextmap setting
		// this allows a player to change maps, but not upset the map rotation
		char s[MAX_STRING_CHARS];

		trap_Cvar_VariableStringBuffer("nextmap", s, sizeof(s));
		if(*s)
			Com_sprintf(level.votestr, sizeof(level.votestr), "%s %s; set nextmap \"%s\"", arg1, arg2, s);
		else
			Com_sprintf(level.votestr, sizeof(level.votestr), "%s %s", arg1, arg2);
		Com_sprintf(level.votedisplaystr, sizeof(level.votedisplaystr), "%s", level.votestr);
	}else if(!Q_stricmp(arg1, "nextmap")){
		char s[MAX_STRING_CHARS];

		trap_Cvar_VariableStringBuffer("nextmap", s, sizeof(s));
		if(!*s){
			trap_SendServerCommand(ent-g_entities, "print \"nextmap not set.\n\"");
			return;
		}
		Com_sprintf(level.votestr, sizeof(level.votestr), "vstr nextmap");
		Com_sprintf(level.votedisplaystr, sizeof(level.votedisplaystr), "%s", level.votestr);
	}else{
		Com_sprintf(level.votestr, sizeof(level.votestr), "%s \"%s\"", arg1, arg2);
		Com_sprintf(level.votedisplaystr, sizeof(level.votedisplaystr), "%s", level.votestr);
	}

	trap_SendServerCommand(-1, va("print \"%s called a vote.\n\"", ent->client->pers.netname));

	// start the voting, the caller automatically votes yes
	level.votetime = level.time;
	level.voteyes = 1;
	level.voteno = 0;

	for(i = 0; i < level.maxclients; i++)
		level.clients[i].ps.eFlags &= ~EF_VOTED;
	ent->client->ps.eFlags |= EF_VOTED;

	trap_SetConfigstring(CS_VOTE_TIME, va("%i", level.votetime));
	trap_SetConfigstring(CS_VOTE_STRING, level.votedisplaystr);
	trap_SetConfigstring(CS_VOTE_YES, va("%i", level.voteyes));
	trap_SetConfigstring(CS_VOTE_NO, va("%i", level.voteno));
}

/*
==================
Cmd_Vote_f
==================
*/
void
Cmd_Vote_f(ent_t *ent)
{
	char msg[64];

	if(!level.votetime){
		trap_SendServerCommand(ent-g_entities, "print \"No vote in progress.\n\"");
		return;
	}
	if(ent->client->ps.eFlags & EF_VOTED){
		trap_SendServerCommand(ent-g_entities, "print \"Vote already cast.\n\"");
		return;
	}
	if(ent->client->sess.team == TEAM_SPECTATOR){
		trap_SendServerCommand(ent-g_entities, "print \"Not allowed to vote as spectator.\n\"");
		return;
	}

	trap_SendServerCommand(ent-g_entities, "print \"Vote cast.\n\"");

	ent->client->ps.eFlags |= EF_VOTED;

	trap_Argv(1, msg, sizeof(msg));

	if(tolower(msg[0]) == 'y' || msg[0] == '1'){
		level.voteyes++;
		trap_SetConfigstring(CS_VOTE_YES, va("%i", level.voteyes));
	}else{
		level.voteno++;
		trap_SetConfigstring(CS_VOTE_NO, va("%i", level.voteno));
	}

	// a majority will be determined in CheckVote, which will also account
	// for players entering or leaving
}

/*
==================
Cmd_CallTeamVote_f
==================
*/
void
Cmd_CallTeamVote_f(ent_t *ent)
{
	int i, team, cs_offset;
	char arg1[MAX_STRING_TOKENS];
	char arg2[MAX_STRING_TOKENS];

	team = ent->client->sess.team;
	if(team == TEAM_RED)
		cs_offset = 0;
	else if(team == TEAM_BLUE)
		cs_offset = 1;
	else
		return;

	if(!g_allowVote.integer){
		trap_SendServerCommand(ent-g_entities, "print \"Voting not allowed here.\n\"");
		return;
	}

	if(level.teamvotetime[cs_offset]){
		trap_SendServerCommand(ent-g_entities, "print \"A team vote is already in progress.\n\"");
		return;
	}
	if(ent->client->pers.teamvotecount >= MAX_VOTE_COUNT){
		trap_SendServerCommand(ent-g_entities, "print \"You have called the maximum number of team votes.\n\"");
		return;
	}
	if(ent->client->sess.team == TEAM_SPECTATOR){
		trap_SendServerCommand(ent-g_entities, "print \"Not allowed to call a vote as spectator.\n\"");
		return;
	}

	// make sure it is a valid command to vote on
	trap_Argv(1, arg1, sizeof(arg1));
	arg2[0] = '\0';
	for(i = 2; i < trap_Argc(); i++){
		if(i > 2)
			strcat(arg2, " ");
		trap_Argv(i, &arg2[strlen(arg2)], sizeof(arg2) - strlen(arg2));
	}

	if(strchr(arg1, ';') || strchr(arg2, ';')){
		trap_SendServerCommand(ent-g_entities, "print \"Invalid vote string.\n\"");
		return;
	}

	if(!Q_stricmp(arg1, "leader")){
		char netname[MAX_NETNAME], leader[MAX_NETNAME];

		if(!arg2[0])
			i = ent->client->ps.clientNum;
		else{
			// numeric values are just slot numbers
			for(i = 0; i < 3; i++)
				if(!arg2[i] || arg2[i] < '0' || arg2[i] > '9')
					break;
			if(i >= 3 || !arg2[i]){
				i = atoi(arg2);
				if(i < 0 || i >= level.maxclients){
					trap_SendServerCommand(ent-g_entities, va("print \"Bad client slot: %i\n\"", i));
					return;
				}

				if(!g_entities[i].inuse){
					trap_SendServerCommand(ent-g_entities, va("print \"Client %i is not active\n\"", i));
					return;
				}
			}else{
				Q_strncpyz(leader, arg2, sizeof(leader));
				Q_CleanStr(leader);
				for(i = 0; i < level.maxclients; i++){
					if(level.clients[i].pers.connected == CON_DISCONNECTED)
						continue;
					if(level.clients[i].sess.team != team)
						continue;
					Q_strncpyz(netname, level.clients[i].pers.netname, sizeof(netname));
					Q_CleanStr(netname);
					if(!Q_stricmp(netname, leader))
						break;
				}
				if(i >= level.maxclients){
					trap_SendServerCommand(ent-g_entities, va("print \"%s is not a valid player on your team.\n\"", arg2));
					return;
				}
			}
		}
		Com_sprintf(arg2, sizeof(arg2), "%d", i);
	}else{
		trap_SendServerCommand(ent-g_entities, "print \"Invalid vote string.\n\"");
		trap_SendServerCommand(ent-g_entities, "print \"Team vote commands are: leader <player>.\n\"");
		return;
	}

	Com_sprintf(level.teamvotestr[cs_offset], sizeof(level.teamvotestr[cs_offset]), "%s %s", arg1, arg2);

	for(i = 0; i < level.maxclients; i++){
		if(level.clients[i].pers.connected == CON_DISCONNECTED)
			continue;
		if(level.clients[i].sess.team == team)
			trap_SendServerCommand(i, va("print \"%s called a team vote.\n\"", ent->client->pers.netname));
	}

	// start the voting, the caller automatically votes yes
	level.teamvotetime[cs_offset] = level.time;
	level.teamvoteyes[cs_offset] = 1;
	level.teamvoteno[cs_offset] = 0;

	for(i = 0; i < level.maxclients; i++)
		if(level.clients[i].sess.team == team)
			level.clients[i].ps.eFlags &= ~EF_TEAMVOTED;
	ent->client->ps.eFlags |= EF_TEAMVOTED;

	trap_SetConfigstring(CS_TEAMVOTE_TIME + cs_offset, va("%i", level.teamvotetime[cs_offset]));
	trap_SetConfigstring(CS_TEAMVOTE_STRING + cs_offset, level.teamvotestr[cs_offset]);
	trap_SetConfigstring(CS_TEAMVOTE_YES + cs_offset, va("%i", level.teamvoteyes[cs_offset]));
	trap_SetConfigstring(CS_TEAMVOTE_NO + cs_offset, va("%i", level.teamvoteno[cs_offset]));
}

/*
==================
Cmd_TeamVote_f
==================
*/
void
Cmd_TeamVote_f(ent_t *ent)
{
	int team, cs_offset;
	char msg[64];

	team = ent->client->sess.team;
	if(team == TEAM_RED)
		cs_offset = 0;
	else if(team == TEAM_BLUE)
		cs_offset = 1;
	else
		return;

	if(!level.teamvotetime[cs_offset]){
		trap_SendServerCommand(ent-g_entities, "print \"No team vote in progress.\n\"");
		return;
	}
	if(ent->client->ps.eFlags & EF_TEAMVOTED){
		trap_SendServerCommand(ent-g_entities, "print \"Team vote already cast.\n\"");
		return;
	}
	if(ent->client->sess.team == TEAM_SPECTATOR){
		trap_SendServerCommand(ent-g_entities, "print \"Not allowed to vote as spectator.\n\"");
		return;
	}

	trap_SendServerCommand(ent-g_entities, "print \"Team vote cast.\n\"");

	ent->client->ps.eFlags |= EF_TEAMVOTED;

	trap_Argv(1, msg, sizeof(msg));

	if(tolower(msg[0]) == 'y' || msg[0] == '1'){
		level.teamvoteyes[cs_offset]++;
		trap_SetConfigstring(CS_TEAMVOTE_YES + cs_offset, va("%i", level.teamvoteyes[cs_offset]));
	}else{
		level.teamvoteno[cs_offset]++;
		trap_SetConfigstring(CS_TEAMVOTE_NO + cs_offset, va("%i", level.teamvoteno[cs_offset]));
	}

	// a majority will be determined in TeamCheckVote, which will also account
	// for players entering or leaving
}

/*
=================
Cmd_SetViewpos_f
=================
*/
void
Cmd_SetViewpos_f(ent_t *ent)
{
	vec3_t origin, angles;
	char buffer[MAX_TOKEN_CHARS];
	int i;

	if(!g_cheats.integer){
		trap_SendServerCommand(ent-g_entities, "print \"Cheats are not enabled on this server.\n\"");
		return;
	}
	if(trap_Argc() != 5){
		trap_SendServerCommand(ent-g_entities, "print \"usage: setviewpos x y z yaw\n\"");
		return;
	}

	vecclear(angles);
	for(i = 0; i < 3; i++){
		trap_Argv(i + 1, buffer, sizeof(buffer));
		origin[i] = atof(buffer);
	}

	trap_Argv(4, buffer, sizeof(buffer));
	angles[YAW] = atof(buffer);

	teleportentity(ent, origin, angles);
}

/*
=================
Cmd_Stats_f
=================
*/
void
Cmd_Stats_f(ent_t *ent)
{
	/*
	    int max, n, i;

	    max = trap_AAS_PointReachabilityAreaIndex( nil );

	    n = 0;
	    for( i = 0; i < max; i++ ){
	            if( ent->client->areabits[i >> 3] & (1 << (i & 7)) )
	                    n++;
	    }

	    //trap_SendServerCommand( ent-g_entities, va("print \"visited %d of %d areas\n\"", n, max));
	    trap_SendServerCommand( ent-g_entities, va("print \"%d%% level coverage\n\"", n * 100 / max));
	*/
}
	

/*
=================
clientcmd
=================
*/
void
clientcmd(int clientNum)
{
	ent_t *ent;
	char cmd[MAX_TOKEN_CHARS];

	ent = g_entities + clientNum;
	if(!ent->client || ent->client->pers.connected != CON_CONNECTED)
		return;	// not fully in game yet


	trap_Argv(0, cmd, sizeof(cmd));

	if(Q_stricmp(cmd, "say") == 0){
		Cmd_Say_f(ent, SAY_ALL, qfalse);
		return;
	}
	if(Q_stricmp(cmd, "say_team") == 0){
		Cmd_Say_f(ent, SAY_TEAM, qfalse);
		return;
	}
	if(Q_stricmp(cmd, "tell") == 0){
		Cmd_Tell_f(ent);
		return;
	}
	if(Q_stricmp(cmd, "score") == 0){
		Cmd_Score_f(ent);
		return;
	}

	// ignore all other commands when at intermission
	if(level.intermissiontime){
		Cmd_Say_f(ent, qfalse, qtrue);
		return;
	}

	if(Q_stricmp(cmd, "give") == 0)
		Cmd_Give_f(ent);
	else if(Q_stricmp(cmd, "god") == 0)
		Cmd_God_f(ent);
	else if(Q_stricmp(cmd, "notarget") == 0)
		Cmd_Notarget_f(ent);
	else if(Q_stricmp(cmd, "noclip") == 0)
		Cmd_Noclip_f(ent);
	else if(Q_stricmp(cmd, "kill") == 0)
		Cmd_Kill_f(ent);
	else if(Q_stricmp(cmd, "teamtask") == 0)
		Cmd_TeamTask_f(ent);
	else if(Q_stricmp(cmd, "levelshot") == 0)
		Cmd_LevelShot_f(ent);
	else if(Q_stricmp(cmd, "follow") == 0)
		Cmd_Follow_f(ent);
	else if(Q_stricmp(cmd, "follownext") == 0)
		Cmd_FollowCycle_f(ent, 1);
	else if(Q_stricmp(cmd, "followprev") == 0)
		Cmd_FollowCycle_f(ent, -1);
	else if(Q_stricmp(cmd, "team") == 0)
		Cmd_Team_f(ent);
	else if(Q_stricmp(cmd, "where") == 0)
		Cmd_Where_f(ent);
	else if(Q_stricmp(cmd, "callvote") == 0)
		Cmd_CallVote_f(ent);
	else if(Q_stricmp(cmd, "vote") == 0)
		Cmd_Vote_f(ent);
	else if(Q_stricmp(cmd, "callteamvote") == 0)
		Cmd_CallTeamVote_f(ent);
	else if(Q_stricmp(cmd, "teamvote") == 0)
		Cmd_TeamVote_f(ent);
	else if(Q_stricmp(cmd, "gc") == 0)
		Cmd_GameCommand_f(ent);
	else if(Q_stricmp(cmd, "setviewpos") == 0)
		Cmd_SetViewpos_f(ent);
	else if(Q_stricmp(cmd, "stats") == 0)
		Cmd_Stats_f(ent);
	else if(Q_stricmp(cmd, "levelrespawn") == 0)
		levelrespawn();	// force a levelrespawn for debugging
	else
		trap_SendServerCommand(clientNum, va("print \"unknown cmd %s\n\"", cmd));
}
