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

// g_client.c -- client functions that don't happen every frame

static vec3_t playerMins = {MINS_X, MINS_Y, MINS_Z};
static vec3_t playerMaxs = {MAXS_X, MAXS_Y, MAXS_Z};

/*QUAKED playerspawn (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position.
The first time a player enters the game, they will be at an 'initial' spot.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void
SP_playerspawn(ent_t *ent)
{
	int i;

	ent->classname = "playerspawn";
	spawnint("nobots", "0", &i);
	if(i)
		ent->flags |= FL_NO_BOTS;
	spawnint("nohumans", "0", &i);
	if(i)
		ent->flags |= FL_NO_HUMANS;
}

/*QUAKED info_player_respawnpoint (1 1 0) (-16 -16 -24) (16 16 32)

Respawn spot to be targeted by a crate_checkpoint.
Targets will be fired when someone spawns on them.
*/
void
SP_info_player_respawnpoint(ent_t *ent)
{
	int i;

	ent->classname = "info_player_respawnpoint";
	spawnint("nobots", "0", &i);
	if(i)
		ent->flags |= FL_NO_BOTS;
	spawnint("nohumans", "0", &i);
	if(i)
		ent->flags |= FL_NO_HUMANS;
}

/*
=======================================================================

  selectspawnpoint

=======================================================================
*/

/*
================
possibletelefrag

================
*/
qboolean
possibletelefrag(ent_t *spot)
{
	int i, num;
	int touch[MAX_GENTITIES];
	ent_t *hit;
	vec3_t mins, maxs;

	vecadd(spot->s.origin, playerMins, mins);
	vecadd(spot->s.origin, playerMaxs, maxs);
	num = trap_EntitiesInBox(mins, maxs, touch, MAX_GENTITIES);

	for(i = 0; i<num; i++){
		hit = &g_entities[touch[i]];
		//if( hit->client && hit->client->ps.stats[STAT_HEALTH] > 0 ){
		if(hit->client)
			return qtrue;

	}

	return qfalse;
}

/*
================
SelectNearestDeathmatchSpawnPoint

Find the spot that we DON'T want to use
================
*/
#define MAX_SPAWN_POINTS 128
ent_t *
SelectNearestDeathmatchSpawnPoint(vec3_t from)
{
	ent_t *spot;
	vec3_t delta;
	float dist, nearestDist;
	ent_t *nearestSpot;

	nearestDist = 999999;
	nearestSpot = nil;
	spot = nil;

	while((spot = findent(spot, FOFS(classname), "playerspawn")) != nil){
		vecsub(spot->s.origin, from, delta);
		dist = veclen(delta);
		if(dist < nearestDist){
			nearestDist = dist;
			nearestSpot = spot;
		}
	}

	return nearestSpot;
}

/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point that doesn't telefrag
================
*/
#define MAX_SPAWN_POINTS 128
ent_t *
SelectRandomDeathmatchSpawnPoint(qboolean isbot)
{
	ent_t *spot;
	int count;
	int selection;
	ent_t *spots[MAX_SPAWN_POINTS];

	count = 0;
	spot = nil;

	while((spot = findent(spot, FOFS(classname), "playerspawn")) != nil && count < MAX_SPAWN_POINTS){
		if(possibletelefrag(spot))
			continue;

		if(((spot->flags & FL_NO_BOTS) && isbot) ||
		   ((spot->flags & FL_NO_HUMANS) && !isbot))
			// spot is not for this human/bot player
			continue;

		spots[count] = spot;
		count++;
	}

	if(!count)	// no spots that won't telefrag
		return findent(nil, FOFS(classname), "playerspawn");

	selection = rand() % count;
	return spots[selection];
}

/*
===========
SelectRandomFurthestSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
ent_t *
SelectRandomFurthestSpawnPoint(vec3_t avoidPoint, vec3_t origin, vec3_t angles, qboolean isbot)
{
	ent_t *spot;
	vec3_t delta;
	float dist;
	float list_dist[MAX_SPAWN_POINTS];
	ent_t *list_spot[MAX_SPAWN_POINTS];
	int numSpots, rnd, i, j;

	numSpots = 0;
	spot = nil;

	while((spot = findent(spot, FOFS(classname), "playerspawn")) != nil){
		if(possibletelefrag(spot))
			continue;

		if(((spot->flags & FL_NO_BOTS) && isbot) ||
		   ((spot->flags & FL_NO_HUMANS) && !isbot))
			// spot is not for this human/bot player
			continue;

		vecsub(spot->s.origin, avoidPoint, delta);
		dist = veclen(delta);

		for(i = 0; i < numSpots; i++)
			if(dist > list_dist[i]){
				if(numSpots >= MAX_SPAWN_POINTS)
					numSpots = MAX_SPAWN_POINTS - 1;

				for(j = numSpots; j > i; j--){
					list_dist[j] = list_dist[j-1];
					list_spot[j] = list_spot[j-1];
				}

				list_dist[i] = dist;
				list_spot[i] = spot;

				numSpots++;
				break;
			}

		if(i >= numSpots && numSpots < MAX_SPAWN_POINTS){
			list_dist[numSpots] = dist;
			list_spot[numSpots] = spot;
			numSpots++;
		}
	}

	if(!numSpots){
		spot = findent(nil, FOFS(classname), "playerspawn");

		if(!spot)
			errorf("Couldn't find a spawn point");

		veccpy(spot->s.origin, origin);
		origin[2] += 9;
		veccpy(spot->s.angles, angles);
		return spot;
	}

	// select a random spot from the spawn points furthest away
	rnd = random() * (numSpots / 2);

	veccpy(list_spot[rnd]->s.origin, origin);
	origin[2] += 9;
	veccpy(list_spot[rnd]->s.angles, angles);

	return list_spot[rnd];
}

static ent_t*
selectrespawnpoint(vec3_t origin, vec3_t angles)
{
	ent_t *ckpoint, *spot, *best;

	ckpoint = &g_entities[level.checkpoint];
	veccpy(ckpoint->s.origin, origin);
	origin[2] += 9;
	veccpy(ckpoint->s.angles, angles);
	if(!possibletelefrag(ckpoint))
		return ckpoint;

	best = ckpoint;
	spot = nil;
	while((spot = findent(spot, FOFS(targetname), ckpoint->target)) != nil){
		if(Q_stricmp(spot->classname, "info_player_respawnpoint") != 0)
			continue;
		if(possibletelefrag(spot))
			continue;
		veccpy(spot->s.origin, origin);
		origin[2] += 9;
		veccpy(spot->s.angles, angles);
		best = spot;
	}
	return best;
}

/*
===========
selectspawnpoint

Chooses a player start, deathmatch start, etc
============
*/
ent_t *
selectspawnpoint(vec3_t avoidPoint, vec3_t origin, vec3_t angles, qboolean isbot)
{
	ent_t *spot;

	if(level.checkpoint != ENTITYNUM_NONE){
		return selectrespawnpoint(origin, angles);
	}
	return SelectRandomFurthestSpawnPoint(avoidPoint, origin, angles, isbot);
}

/*
===========
SelectInitialSpawnPoint

Try to find a spawn point marked 'initial', otherwise
use normal spawn selection.
============
*/
ent_t *
SelectInitialSpawnPoint(vec3_t origin, vec3_t angles, qboolean isbot)
{
	ent_t *spot;

	spot = nil;

	if(level.checkpoint != ENTITYNUM_NONE){
		spot = &g_entities[level.checkpoint];
		veccpy(spot->s.origin, origin);
		origin[2] += 9;
		veccpy(spot->s.angles, angles);
		return spot;
	}

	while((spot = findent(spot, FOFS(classname), "playerspawn")) != nil){
		if(((spot->flags & FL_NO_BOTS) && isbot) ||
		   ((spot->flags & FL_NO_HUMANS) && !isbot))
			continue;

		if((spot->spawnflags & 0x01))
			break;
	}

	if(!spot || possibletelefrag(spot))
		return selectspawnpoint(vec3_origin, origin, angles, isbot);

	veccpy(spot->s.origin, origin);
	origin[2] += 9;
	veccpy(spot->s.angles, angles);

	return spot;
}

/*
===========
SelectSpectatorSpawnPoint

============
*/
ent_t *
SelectSpectatorSpawnPoint(vec3_t origin, vec3_t angles)
{
	findintermissionpoint();

	veccpy(level.intermissionpos, origin);
	veccpy(level.intermissionangle, angles);

	return nil;
}

/*
=======================================================================

BODYQUE

=======================================================================
*/

/*
===============
initbodyqueue
===============
*/
void
initbodyqueue(void)
{
	int i;
	ent_t *ent;

	level.bodyqueueindex = 0;
	for(i = 0; i<BODY_QUEUE_SIZE; i++){
		ent = entspawn();
		ent->classname = "bodyque";
		ent->neverfree = qtrue;
		level.bodyqueue[i] = ent;
	}
}

void
clearbodyqueue(void)
{
	ent_t *e;

	level.bodyqueueindex = 0;
	e = nil;
	while((e = findent(e, FOFS(classname), "bodyque")) != nil){
		memset(e, 0, sizeof *e);
		e->classname = "bodyque";
		e->neverfree = qtrue;
	}
}

/*
=============
BodySink

After sitting around for five seconds, fall into the ground and dissapear
=============
*/
void
BodySink(ent_t *ent)
{
	if(level.time - ent->timestamp > 6500){
		// the body ques are never actually freed, they are just unlinked
		trap_UnlinkEntity(ent);
		ent->physobj = qfalse;
		return;
	}
	ent->nextthink = level.time + 100;
	ent->s.pos.trBase[2] -= 1;
}

/*
=============
copytobodyqueue

A player is respawning, so make an entity that looks
just like the existing corpse to leave behind.
=============
*/
void
copytobodyqueue(ent_t *ent)
{
	ent_t *body;
	int contents;

	trap_UnlinkEntity(ent);

	// if client is in a nodrop area, don't leave the body
	contents = trap_PointContents(ent->s.origin, -1);
	if(contents & CONTENTS_NODROP)
		return;

	// grab a body que and cycle to the next one
	body = level.bodyqueue[level.bodyqueueindex];
	level.bodyqueueindex = (level.bodyqueueindex + 1) % BODY_QUEUE_SIZE;

	body->s = ent->s;
	body->s.eFlags = EF_DEAD;	// clear EF_TALK, etc
	body->s.powerups = 0;	// clear powerups
	body->s.loopSound = 0;	// clear lava burning
	body->s.number = body - g_entities;
	body->timestamp = level.time;
	body->physobj = qtrue;
	body->physbounce = 0;	// don't bounce
	if(body->s.groundEntityNum == ENTITYNUM_NONE){
		body->s.pos.trType = TR_GRAVITY;
		body->s.pos.trTime = level.time;
		veccpy(ent->client->ps.velocity, body->s.pos.trDelta);
	}else
		body->s.pos.trType = TR_STATIONARY;
	body->s.event = 0;

	// change the animation to the last-frame only, so the sequence
	// doesn't repeat anew for the body
	switch(body->s.legsAnim & ~ANIM_TOGGLEBIT){
	case BOTH_DEATH1:
	case BOTH_DEAD1:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD1;
		break;
	case BOTH_DEATH2:
	case BOTH_DEAD2:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD2;
		break;
	case BOTH_DEATH3:
	case BOTH_DEAD3:
	default:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD3;
		break;
	}

	body->r.svFlags = ent->r.svFlags;
	veccpy(ent->r.mins, body->r.mins);
	veccpy(ent->r.maxs, body->r.maxs);
	veccpy(ent->r.absmin, body->r.absmin);
	veccpy(ent->r.absmax, body->r.absmax);

	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	body->r.contents = CONTENTS_CORPSE;
	body->r.ownerNum = ent->s.number;

	body->nextthink = level.time + 5000;
	body->think = BodySink;

	body->die = body_die;

	// don't take more damage if already gibbed
	if(ent->health <= GIB_HEALTH)
		body->takedmg = qfalse;
	else
		body->takedmg = qtrue;


	veccpy(body->s.pos.trBase, body->r.currentOrigin);
	trap_LinkEntity(body);
}

//======================================================================

/*
==================
setviewangles

==================
*/
void
setviewangles(ent_t *ent, vec3_t angle)
{
	int i;

	// set the delta angle
	for(i = 0; i<3; i++){
		int cmdAngle;

		cmdAngle = ANGLE2SHORT(angle[i]);
		ent->client->ps.delta_angles[i] = cmdAngle - ent->client->pers.cmd.angles[i];
	}
	veccpy(angle, ent->s.angles);
	veccpy(ent->s.angles, ent->client->ps.viewangles);
}

/*
================
clientrespawn
================
*/
void
clientrespawn(ent_t *ent)
{
	levelrespawn();
	copytobodyqueue(ent);
	clientspawn(ent);
}

/*
================
teamcount

Returns number of players on a team
================
*/
int
teamcount(int ignoreClientNum, teamnum_t team)
{
	int i;
	int count = 0;

	for(i = 0; i < level.maxclients; i++){
		if(i == ignoreClientNum)
			continue;
		if(level.clients[i].pers.connected == CON_DISCONNECTED)
			continue;
		if(level.clients[i].sess.team == team)
			count++;
	}

	return count;
}

/*
================
teamleader

Returns the client number of the team leader
================
*/
int
teamleader(int team)
{
	int i;

	for(i = 0; i < level.maxclients; i++){
		if(level.clients[i].pers.connected == CON_DISCONNECTED)
			continue;
		if(level.clients[i].sess.team == team)
			if(level.clients[i].sess.teamleader)
				return i;
	}

	return -1;
}

/*
================
pickteam

================
*/
teamnum_t
pickteam(int ignoreClientNum)
{
	int counts[TEAM_NUM_TEAMS];

	counts[TEAM_BLUE] = teamcount(ignoreClientNum, TEAM_BLUE);
	counts[TEAM_RED] = teamcount(ignoreClientNum, TEAM_RED);

	if(counts[TEAM_BLUE] > counts[TEAM_RED])
		return TEAM_RED;
	if(counts[TEAM_RED] > counts[TEAM_BLUE])
		return TEAM_BLUE;
	// equal team count, so join the team with the lowest score
	if(level.teamscores[TEAM_BLUE] > level.teamscores[TEAM_RED])
		return TEAM_RED;
	return TEAM_BLUE;
}

/*
===========
ForceClientSkin

Forces a client's skin (for teamplay)
===========
*/
/*
static void ForceClientSkin( gclient_t *client, char *model, const char *skin ){
        char *p;

        if((p = strrchr(model, '/')) != 0){
                *p = 0;
        }

        Q_strcat(model, MAX_QPATH, "/");
        Q_strcat(model, MAX_QPATH, skin);
}
*/

/*
===========
ClientCleanName
============
*/
static void
ClientCleanName(const char *in, char *out, int outSize)
{
	int outpos = 0, colorlessLen = 0, spaces = 0;

	// discard leading spaces
	for(; *in == ' '; in++)
		;

	for(; *in && outpos < outSize - 1; in++){
		out[outpos] = *in;

		if(*in == ' '){
			// don't allow too many consecutive spaces
			if(spaces > 2)
				continue;

			spaces++;
		}else if(outpos > 0 && out[outpos - 1] == Q_COLOR_ESCAPE){
			if(Q_IsColorString(&out[outpos - 1])){
				colorlessLen--;

				if(ColorIndex(*in) == 0){
					// Disallow color black in names to prevent players
					// from getting advantage playing in front of black backgrounds
					outpos--;
					continue;
				}
			}else{
				spaces = 0;
				colorlessLen++;
			}
		}else{
			spaces = 0;
			colorlessLen++;
		}

		outpos++;
	}

	out[outpos] = '\0';

	// don't allow empty names
	if(*out == '\0' || colorlessLen == 0)
		Q_strncpyz(out, "UnnamedPlayer", outSize);
}

/*
===========
ClientUserInfoChanged

Called from clientconnect when the player first connects and
directly by the server system when the player updates a userinfo variable.

The game can override any of the settings and call trap_SetUserinfo
if desired.
============
*/
void
clientuserinfochanged(int clientNum)
{
	ent_t *ent;
	int teamtask, teamleader, team;
	char *s;
	char model[MAX_QPATH];
	char headmodel[MAX_QPATH];
	char oldname[MAX_STRING_CHARS];
	gclient_t *client;
	char c1[MAX_INFO_STRING];
	char c2[MAX_INFO_STRING];
	char redteam[MAX_INFO_STRING];
	char blueteam[MAX_INFO_STRING];
	char userinfo[MAX_INFO_STRING];

	ent = g_entities + clientNum;
	client = ent->client;

	trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));

	// check for malformed or illegal info strings
	if(!Info_Validate(userinfo)){
		strcpy(userinfo, "\\name\\badinfo");
		// don't keep those clients and userinfo
		trap_DropClient(clientNum, "Invalid userinfo");
	}

	// check for local client
	s = Info_ValueForKey(userinfo, "ip");
	if(!strcmp(s, "localhost"))
		client->pers.localclient = qtrue;

	// check the item prediction
	s = Info_ValueForKey(userinfo, "cg_predictItems");
	if(!atoi(s))
		client->pers.predictitempickup = qfalse;
	else
		client->pers.predictitempickup = qtrue;

	// set name
	Q_strncpyz(oldname, client->pers.netname, sizeof(oldname));
	s = Info_ValueForKey(userinfo, "name");
	ClientCleanName(s, client->pers.netname, sizeof(client->pers.netname));

	if(client->sess.team == TEAM_SPECTATOR)
		if(client->sess.specstate == SPECTATOR_SCOREBOARD)
			Q_strncpyz(client->pers.netname, "scoreboard", sizeof(client->pers.netname));

	if(client->pers.connected == CON_CONNECTED)
		if(strcmp(oldname, client->pers.netname))
			trap_SendServerCommand(-1, va("print \"%s" S_COLOR_WHITE " renamed to %s\n\"", oldname,
						      client->pers.netname));

	client->ps.stats[STAT_MAX_ARMOR] = 100;

	// set model
	if(g_gametype.integer >= GT_TEAM){
		Q_strncpyz(model, Info_ValueForKey(userinfo, "team_model"), sizeof(model));
		Q_strncpyz(headmodel, Info_ValueForKey(userinfo, "team_headmodel"), sizeof(headmodel));
	}else{
		Q_strncpyz(model, Info_ValueForKey(userinfo, "model"), sizeof(model));
		Q_strncpyz(headmodel, Info_ValueForKey(userinfo, "headmodel"), sizeof(headmodel));
	}

	// bots set their team a few frames later
	if(g_gametype.integer >= GT_TEAM && g_entities[clientNum].r.svFlags & SVF_BOT){
		s = Info_ValueForKey(userinfo, "team");
		if(!Q_stricmp(s, "red") || !Q_stricmp(s, "r"))
			team = TEAM_RED;
		else if(!Q_stricmp(s, "blue") || !Q_stricmp(s, "b"))
			team = TEAM_BLUE;
		else
			// pick the team with the least number of players
			team = pickteam(clientNum);
	}else
		team = client->sess.team;

	/*	NOTE: all client side now

	    // team
	    switch( team ){
	    case TEAM_RED:
	            ForceClientSkin(client, model, "red");
	//		ForceClientSkin(client, headmodel, "red");
	            break;
	    case TEAM_BLUE:
	            ForceClientSkin(client, model, "blue");
	//		ForceClientSkin(client, headmodel, "blue");
	            break;
	    }
	    // don't ever use a default skin in teamplay, it would just waste memory
	    // however bots will always join a team but they spawn in as spectator
	    if( g_gametype.integer >= GT_TEAM && team == TEAM_SPECTATOR){
	            ForceClientSkin(client, model, "red");
	//		ForceClientSkin(client, headmodel, "red");
	    }
	*/

	// teaminfo
	s = Info_ValueForKey(userinfo, "teamoverlay");
	if(!*s || atoi(s) != 0)
		client->pers.teaminfo = qtrue;
	else
		client->pers.teaminfo = qfalse;

	/*
	s = Info_ValueForKey( userinfo, "cg_pmove_fixed" );
	if( !*s || atoi( s ) == 0 ){
	    client->pers.pmovefixed = qfalse;
	}
	else{
	    client->pers.pmovefixed = qtrue;
	}
	*/

	// team task (0 = none, 1 = offence, 2 = defence)
	teamtask = atoi(Info_ValueForKey(userinfo, "teamtask"));
	// team Leader (1 = leader, 0 is normal player)
	teamleader = client->sess.teamleader;

	// colors
	strcpy(c1, Info_ValueForKey(userinfo, "color1"));
	strcpy(c2, Info_ValueForKey(userinfo, "color2"));

	strcpy(redteam, Info_ValueForKey(userinfo, "g_redteam"));
	strcpy(blueteam, Info_ValueForKey(userinfo, "g_blueteam"));

	// send over a subset of the userinfo keys so other clients can
	// print scoreboards, display models, and play custom sounds
	if(ent->r.svFlags & SVF_BOT)
		s = va("n\\%s\\t\\%i\\model\\%s\\hmodel\\%s\\c1\\%s\\c2\\%s\\hc\\%i\\w\\%i\\l\\%i\\skill\\%s\\tt\\%d\\tl\\%d",
		       client->pers.netname, team, model, headmodel, c1, c2,
		       client->pers.maxhealth, client->sess.wins, client->sess.losses,
		       Info_ValueForKey(userinfo, "skill"), teamtask, teamleader);
	else
		s = va("n\\%s\\t\\%i\\model\\%s\\hmodel\\%s\\g_redteam\\%s\\g_blueteam\\%s\\c1\\%s\\c2\\%s\\hc\\%i\\w\\%i\\l\\%i\\tt\\%d\\tl\\%d",
		       client->pers.netname, client->sess.team, model, headmodel, redteam, blueteam, c1, c2,
		       client->pers.maxhealth, client->sess.wins, client->sess.losses, teamtask, teamleader);

	trap_SetConfigstring(CS_PLAYERS+clientNum, s);

	// this is not the userinfo, more like the configstring actually
	logprintf("clientuserinfochanged: %i %s\n", clientNum, s);
}

/*
===========
clientconnect

Called when a player begins connecting to the server.
Called again for every map change or tournement restart.

The session information will be valid after exit.

Return nil if the client should be allowed, otherwise return
a string with the reason for denial.

Otherwise, the client will be sent the current gamestate
and will eventually get to clientbegin.

firstTime will be qtrue the very first time a client connects
to the server machine, but qfalse on map changes and tournement
restarts.
============
*/
char *
clientconnect(int clientNum, qboolean firstTime, qboolean isBot)
{
	char *value;
//	char		*areabits;
	gclient_t *client;
	char userinfo[MAX_INFO_STRING];
	char *guid;
	ent_t *ent;

	ent = &g_entities[clientNum];

	trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));

	// IP filtering
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=500
	// recommanding PB based IP / GUID banning, the builtin system is pretty limited
	// check to see if they are on the banned IP list
	value = Info_ValueForKey(userinfo, "ip");
	if(filterpacket(value))
		return "You are banned from this server.";

	// we don't check password for bots and local client
	// NOTE: local client <-> "ip" "localhost"
	//   this means this client is not running in our current process
	if(!isBot && (strcmp(value, "localhost") != 0)){
		// check for a password
		value = Info_ValueForKey(userinfo, "password");
		if(g_password.string[0] && Q_stricmp(g_password.string, "none") &&
		   strcmp(g_password.string, value) != 0)
			return "Invalid password";
	}
	// if a player reconnects quickly after a disconnect, the client disconnect may never be called, thus flag can get lost in the ether
	if(ent->inuse){
		logprintf("Forcing disconnect on active client: %i\n", clientNum);
		// so lets just fix up anything that should happen on a disconnect
		clientdisconnect(clientNum);
	}
	// they can connect
	ent->client = level.clients + clientNum;
	client = ent->client;

//	areabits = client->areabits;

	memset(client, 0, sizeof(*client));

	client->pers.connected = CON_CONNECTING;

	// read or initialize the session data
	if(firstTime || level.newsess)
		sessinit(client, userinfo);
	sessread(client);

	if(isBot){
		ent->r.svFlags |= SVF_BOT;
		ent->inuse = qtrue;
		if(!botconnect(clientNum, !firstTime))
			return "BotConnectfailed";
	}

	// get and distribute relevent paramters
	logprintf("clientconnect: %i\n", clientNum);
	clientuserinfochanged(clientNum);

	// don't do the "xxx connected" messages if they were caried over from previous level
	if(firstTime)
		trap_SendServerCommand(-1, va("print \"%s" S_COLOR_WHITE " connected\n\"", client->pers.netname));

	if(g_gametype.integer >= GT_TEAM &&
	   client->sess.team != TEAM_SPECTATOR)
		broadcastteamchange(client, -1);

	guid = Info_ValueForKey(userinfo, "cl_guid");
	clientfromsave(client, guid);

	// count current clients and rank for scoreboard
	calcranks();

	// for statistics
//	client->areabits = areabits;
//	if( !client->areabits )
//		client->areabits = alloc( (trap_AAS_PointReachabilityAreaIndex( nil ) + 7) / 8 );

	return nil;
}

/*
===========
clientbegin

called when a client has finished connecting, and is ready
to be placed into the level.  This will happen every level load,
and on transition between teams, but doesn't happen on respawns
============
*/
void
clientbegin(int clientNum)
{
	ent_t *ent;
	gclient_t *client;
	int flags, nspawns;

	ent = g_entities + clientNum;

	client = level.clients + clientNum;

	if(ent->r.linked)
		trap_UnlinkEntity(ent);
	entinit(ent);
	ent->touch = 0;
	ent->pain = 0;
	ent->client = client;

	client->pers.connected = CON_CONNECTED;
	client->pers.entertime = level.time;
	client->pers.teamstate.state = TEAM_BEGIN;

	// save eflags around this, because changing teams will
	// cause this to happen with a valid entity, and we
	// want to make sure the teleport bit is set right
	// so the viewpoint doesn't interpolate through the
	// world to the new position
	//
	// also save number of spawns so we can tell when a client
	// is coming out of deathspec
	flags = client->ps.eFlags;
	nspawns = client->ps.persistant[PERS_SPAWNS];
	memset(&client->ps, 0, sizeof(client->ps));
	client->ps.eFlags = flags;
	client->ps.persistant[PERS_SPAWNS] = nspawns;

	// locate ent at a spawn point
	clientspawn(ent);

	if(client->sess.team != TEAM_SPECTATOR)
		if(g_gametype.integer != GT_TOURNAMENT)
			trap_SendServerCommand(-1, va("print \"%s" S_COLOR_WHITE " entered the game\n\"", client->pers.netname));
	logprintf("clientbegin: %i\n", clientNum);

	// count current clients and rank for scoreboard
	calcranks();
}

/*
===========
clientspawn

Called every time a client is placed fresh in the world:
after the first clientbegin, and after each respawn
Initializes all non-persistant parts of playerState
============
*/
void
clientspawn(ent_t *ent)
{
	int index;
	vec3_t spawn_origin, spawn_angles;
	gclient_t *client;
	int i;
	clientpersist_t saved;
	clientsess_t savedSess;
	int persistant[MAX_PERSISTANT];
	ent_t *spawnPoint;
	ent_t *tent;
	int flags;
	int savedPing;
//	char	*savedAreaBits;
	int accuracyhits, accuracyshots;
	qboolean deathspec;
	int eventSequence;
	char userinfo[MAX_INFO_STRING];

	index = ent - g_entities;
	client = ent->client;

	vecclear(spawn_origin);

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	if(client->sess.team == TEAM_SPECTATOR)
		spawnPoint = SelectSpectatorSpawnPoint(spawn_origin, spawn_angles);
	else if(g_gametype.integer >= GT_CTF)
		// all base oriented team games use the CTF spawn points
		spawnPoint = selctfspawnpoint(
			client->sess.team,
			client->pers.teamstate.state,
			spawn_origin, spawn_angles,
			!!(ent->r.svFlags & SVF_BOT));
	else{
		// the first spawn should be at a good looking spot
		if(!client->pers.initialspawn && client->pers.localclient){
			client->pers.initialspawn = qtrue;
			spawnPoint = SelectInitialSpawnPoint(spawn_origin, spawn_angles,
							     !!(ent->r.svFlags & SVF_BOT));
		}else
			// don't spawn near existing origin if possible
			spawnPoint = selectspawnpoint(client->ps.origin, spawn_origin,
						      spawn_angles, !!(ent->r.svFlags & SVF_BOT));
	}
	client->pers.teamstate.state = TEAM_ACTIVE;

	// toggle the teleport bit so the client knows to not lerp
	// and never clear the voted flag
	flags = ent->client->ps.eFlags & (EF_TELEPORT_BIT | EF_VOTED | EF_TEAMVOTED);
	flags ^= EF_TELEPORT_BIT;

	// clear everything but the persistant data

	saved = client->pers;
	savedSess = client->sess;
	savedPing = client->ps.ping;
//	savedAreaBits = client->areabits;
	accuracyhits = client->accuracyhits;
	accuracyshots = client->accuracyshots;
	deathspec = client->deathspec;
	for(i = 0; i < MAX_PERSISTANT; i++)
		persistant[i] = client->ps.persistant[i];
	eventSequence = client->ps.eventSequence;

	memset(client, 0, sizeof(*client));

	client->pers = saved;
	client->sess = savedSess;
	client->ps.ping = savedPing;
//	client->areabits = savedAreaBits;
	client->accuracyhits = accuracyhits;
	client->accuracyshots = accuracyshots;
	client->deathspec = deathspec;
	client->lastkilledclient = -1;

	for(i = 0; i < MAX_PERSISTANT; i++)
		client->ps.persistant[i] = persistant[i];
	client->ps.eventSequence = eventSequence;
	// increment the spawncount so the client will detect the respawn
	client->ps.persistant[PERS_SPAWN_COUNT]++;
	client->ps.persistant[PERS_TEAM] = client->sess.team;

	ent->airouttime = level.time + 12000;

	trap_GetUserinfo(index, userinfo, sizeof(userinfo));
	// set max health
	client->ps.stats[STAT_MAX_ARMOR] = 100;
	client->ps.eFlags = flags;

	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->s.headEntityNum = ENTITYNUM_NONE;
	ent->client = &level.clients[index];
	ent->takedmg = qtrue;
	ent->inuse = qtrue;
	ent->classname = "player";
	ent->r.contents = CONTENTS_BODY;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags = 0;

	veccpy(playerMins, ent->r.mins);
	veccpy(playerMaxs, ent->r.maxs);

	client->ps.clientNum = index;

	client->ps.stats[STAT_WEAPONS] = 0;

	client->ps.stats[STAT_WEAPONS] |= (1 << WP_GAUNTLET);
	client->ps.ammo[WP_GAUNTLET] = -1;
	client->ps.ammo[WP_GRAPPLING_HOOK] = -1;

	if(client->ps.persistant[PERS_SPAWNS] == 0)
		client->ps.persistant[PERS_LIVES] = 3;

	ent->health = client->ps.stats[STAT_HEALTH] = 1;
	client->ps.persistant[PERS_SPAWNS]++;

	setorigin(ent, spawn_origin);
	veccpy(spawn_origin, client->ps.origin);

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;

	trap_GetUsercmd(client - level.clients, &ent->client->pers.cmd);
	setviewangles(ent, spawn_angles);
	// don't allow full run speed for a bit
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

	client->respawntime = level.time;
	client->inactivitytime = level.time + g_inactivity.integer * 1000;
	client->latchedbuttons = 0;

	// set default animations
	client->ps.torsoAnim = TORSO_STAND;
	client->ps.legsAnim = LEGS_IDLE;

	if(!level.intermissiontime){
		if(ent->client->sess.team != TEAM_SPECTATOR){
			killbox(ent);
			// force the base weapon up
			client->ps.weapon = WP_MACHINEGUN;
			client->ps.weaponstate = WEAPON_READY;
			// fire the targets of the spawn point
			usetargets(spawnPoint, ent);
			// select the highest weapon number available, after any spawn given items have fired
			client->ps.weapon = 1;

			for(i = WP_NUM_WEAPONS - 1; i > 0; i--)
				if(client->ps.stats[STAT_WEAPONS] & (1 << i)){
					client->ps.weapon = i;
					break;
				}
			// positively link the client, even if the command times are weird
			veccpy(ent->client->ps.origin, ent->r.currentOrigin);

			tent = enttemp(ent->client->ps.origin, EV_PLAYER_TELEPORT_IN);
			tent->s.clientNum = ent->s.clientNum;

			trap_LinkEntity(ent);
		}
	}else
		// move players to intermission
		clientintermission(ent);
	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	client->ps.commandTime = level.time - 100;
	ent->client->pers.cmd.serverTime = level.time;
	clientthink(ent-g_entities);
	// run the presend to set anything else, follow spectators wait
	// until all clients have been reconnected after map_restart
	if(ent->client->sess.specstate != SPECTATOR_FOLLOW)
		clientendframe(ent);

	// clear entity state values
	playerstate2entstate(&client->ps, &ent->s, qtrue);
}

/*
===========
clientdisconnect

Called when a player drops from the server.
Will not be called between levels.

This should NOT be called directly by any game logic,
call trap_DropClient(), which will call this and do
server system housekeeping.
============
*/
void
clientdisconnect(int clientNum)
{
	ent_t *ent;
	ent_t *tent;
	int i;

	// cleanup if we are kicking a bot that
	// hasn't spawned yet
	dequeuebotbegin(clientNum);

	ent = g_entities + clientNum;
	if(!ent->client || ent->client->pers.connected == CON_DISCONNECTED)
		return;

	// stop any following clients
	for(i = 0; i < level.maxclients; i++)
		if(level.clients[i].sess.team == TEAM_SPECTATOR
		   && level.clients[i].sess.specstate == SPECTATOR_FOLLOW
		   && level.clients[i].sess.specclient == clientNum)
			stopfollowing(&g_entities[i]);

	// send effect if they were completely connected
	if(ent->client->pers.connected == CON_CONNECTED
	   && ent->client->sess.team != TEAM_SPECTATOR){
		tent = enttemp(ent->client->ps.origin, EV_PLAYER_TELEPORT_OUT);
		tent->s.clientNum = ent->s.clientNum;

		// They don't get to take powerups with them!
		// Especially important for stuff like CTF flags
		tossclientitems(ent);
	}

	logprintf("clientdisconnect: %i\n", clientNum);

	// if we are playing in tourney mode and losing, give a win to the other player
	if((g_gametype.integer == GT_TOURNAMENT)
	   && !level.intermissiontime
	   && !level.warmuptime && level.sortedclients[1] == clientNum){
		level.clients[level.sortedclients[0]].sess.wins++;
		clientuserinfochanged(level.sortedclients[0]);
	}

	if(g_gametype.integer == GT_TOURNAMENT &&
	   ent->client->sess.team == TEAM_FREE &&
	   level.intermissiontime){
		trap_SendConsoleCommand(EXEC_APPEND, "map_restart 0\n");
		level.restarted = qtrue;
		level.changemap = nil;
		level.intermissiontime = 0;
	}

	trap_UnlinkEntity(ent);
	ent->s.modelindex = 0;
	ent->inuse = qfalse;
	ent->classname = "disconnected";
	ent->client->pers.connected = CON_DISCONNECTED;
	ent->client->ps.persistant[PERS_TEAM] = TEAM_FREE;
	ent->client->sess.team = TEAM_FREE;

	trap_SetConfigstring(CS_PLAYERS + clientNum, "");

	calcranks();

	if(ent->r.svFlags & SVF_BOT)
		BotAIShutdownClient(clientNum, qfalse);
}

/*
Game over for this client.  If they were the only live client left,
the game ends.  If not, they're stuck in spectate.
*/
void
clientgameover(ent_t *e)
{
	ent_t *ent;
	int i;

	for(i = 0, ent = level.entities; i < MAX_CLIENTS; i++, ent++)
		if(ent->inuse && ent->client->sess.specstate == SPECTATOR_NOT &&
		   !ent->client->deathspec && ent->s.number != e->s.number)
			break;
	if(i == MAX_CLIENTS)
		gameover();
}
