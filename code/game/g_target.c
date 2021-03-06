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

//==========================================================

static void
target_levelrespawn(ent_t *e)
{
	restoreinitialstate(e);
}

/*QUAKED target_give (1 0 0) (-8 -8 -8) (8 8 8)
Gives the activator all the items pointed to.
*/
void
Use_Target_Give(ent_t *ent, ent_t *other, ent_t *activator)
{
	ent_t *t;
	trace_t trace;

	if(!activator->client)
		return;

	if(!ent->target)
		return;

	ent->ckpoint = level.checkpoint;

	memset(&trace, 0, sizeof(trace));
	t = nil;
	while((t = findent(t, FOFS(targetname), ent->target)) != nil){
		if(!t->item)
			continue;
		item_touch(t, activator, &trace);

		// make sure it isn't going to respawn or show any events
		t->nextthink = 0;
		trap_UnlinkEntity(t);
	}
}

void
SP_target_give(ent_t *ent)
{
	ent->use = Use_Target_Give;
	ent->levelrespawn = target_levelrespawn;
	ent->ckpoint = level.checkpoint;
}

//==========================================================

/*QUAKED target_remove_powerups (1 0 0) (-8 -8 -8) (8 8 8)
takes away all the activators powerups.
Used to drop flight powerups into death puts.
*/
void
Use_target_remove_powerups(ent_t *ent, ent_t *other, ent_t *activator)
{
	if(!activator->client)
		return;

	ent->ckpoint = level.checkpoint;

	if(activator->client->ps.powerups[PW_REDFLAG])
		teamreturnflag(TEAM_RED);
	else if(activator->client->ps.powerups[PW_BLUEFLAG])
		teamreturnflag(TEAM_BLUE);
	else if(activator->client->ps.powerups[PW_NEUTRALFLAG])
		teamreturnflag(TEAM_FREE);

	memset(activator->client->ps.powerups, 0, sizeof(activator->client->ps.powerups));
}

void
SP_target_remove_powerups(ent_t *ent)
{
	ent->use = Use_target_remove_powerups;
	ent->levelrespawn = target_levelrespawn;
	ent->ckpoint = level.checkpoint;
}

//==========================================================

/*QUAKED target_delay (1 0 0) (-8 -8 -8) (8 8 8)
"wait" seconds to pause before firing targets.
"random" delay variance, total delay = delay +/- random seconds
*/
void
Think_Target_Delay(ent_t *ent)
{
	usetargets(ent, ent->activator);
}

void
Use_Target_Delay(ent_t *ent, ent_t *other, ent_t *activator)
{
	ent->ckpoint = level.checkpoint;
	ent->nextthink = level.time + (ent->wait + ent->random*crandom()) * 1000;
	ent->think = Think_Target_Delay;
	ent->activator = activator;
}

void
SP_target_delay(ent_t *ent)
{
	// check delay for backwards compatability
	if(!spawnfloat("delay", "0", &ent->wait))
		spawnfloat("wait", "0", &ent->wait);

	ent->use = Use_Target_Delay;
	ent->levelrespawn = target_levelrespawn;
	ent->ckpoint = level.checkpoint;
}

//==========================================================

/*QUAKED target_score (1 0 0) (-8 -8 -8) (8 8 8)
"count" number of points to add, default 1

The activator is given this many points.
*/
void
Use_Target_Score(ent_t *ent, ent_t *other, ent_t *activator)
{
	ent->ckpoint = level.checkpoint;
	addscore(activator, ent->r.currentOrigin, ent->count);
}

void
SP_target_score(ent_t *ent)
{
	if(!ent->count)
		ent->count = 1;
	ent->use = Use_Target_Score;
	ent->levelrespawn = target_levelrespawn;
	ent->ckpoint = level.checkpoint;
}

//==========================================================

/*QUAKED target_print (1 0 0) (-8 -8 -8) (8 8 8) redteam blueteam private
"message"	text to print
If "private", only the activator gets the message.  If no checks, all clients get the message.
*/
void
Use_Target_Print(ent_t *ent, ent_t *other, ent_t *activator)
{
	if(activator->client && (ent->spawnflags & 4)){
		trap_SendServerCommand(activator-g_entities, va("cp \"%s\"", ent->message));
		return;
	}

	ent->ckpoint = level.checkpoint;


	if(ent->spawnflags & 3){
		if(ent->spawnflags & 1)
			teamcmd(TEAM_RED, va("cp \"%s\"", ent->message));
		if(ent->spawnflags & 2)
			teamcmd(TEAM_BLUE, va("cp \"%s\"", ent->message));
		return;
	}

	trap_SendServerCommand(-1, va("cp \"%s\"", ent->message));
}

void
SP_target_print(ent_t *ent)
{
	ent->use = Use_Target_Print;
	ent->levelrespawn = target_levelrespawn;
	ent->ckpoint = level.checkpoint;
}

//==========================================================

/*QUAKED target_speaker (1 0 0) (-8 -8 -8) (8 8 8) looped-on looped-off global activator
"noise"		wav file to play

A global sound will play full volume throughout the level.
Activator sounds will play on the player that activated the target.
Global and activator sounds can't be combined with looping.
Normal sounds play each time the target is used.
Looped sounds will be toggled by use functions.
Multiple identical looping sounds will just increase volume without any speed cost.
"wait" : Seconds between auto triggerings, 0 = don't auto trigger
"random"	wait variance, default is 0
*/
void
Use_Target_Speaker(ent_t *ent, ent_t *other, ent_t *activator)
{
	ent->ckpoint = level.checkpoint;

	if(ent->spawnflags & 3){				// looping sound toggles
		if(ent->s.loopSound)
			ent->s.loopSound = 0;			// turn it off
		else
			ent->s.loopSound = ent->noiseindex;	// start it
	}else{						// normal sound
		if(ent->spawnflags & 8)
			addevent(activator, EV_GENERAL_SOUND, ent->noiseindex);
		else if(ent->spawnflags & 4)
			addevent(ent, EV_GLOBAL_SOUND, ent->noiseindex);
		else
			addevent(ent, EV_GENERAL_SOUND, ent->noiseindex);
	}
}

void
SP_target_speaker(ent_t *ent)
{
	char buffer[MAX_QPATH];
	char *s;

	spawnfloat("wait", "0", &ent->wait);
	spawnfloat("random", "0", &ent->random);

	if(!spawnstr("noise", "NOSOUND", &s))
		errorf("target_speaker without a noise key at %s", vtos(ent->s.origin));

	// force all client relative sounds to be "activator" speakers that
	// play on the entity that activates it
	if(s[0] == '*')
		ent->spawnflags |= 8;

	if(!strstr(s, ".wav"))
		Com_sprintf(buffer, sizeof(buffer), "%s.wav", s);
	else
		Q_strncpyz(buffer, s, sizeof(buffer));
	ent->noiseindex = soundindex(buffer);

	// a repeating speaker can be done completely client side
	ent->s.eType = ET_SPEAKER;
	ent->s.eventParm = ent->noiseindex;
	ent->s.frame = ent->wait * 10;
	ent->s.clientNum = ent->random * 10;

	// check for prestarted looping sound
	if(ent->spawnflags & 1)
		ent->s.loopSound = ent->noiseindex;

	ent->use = Use_Target_Speaker;
	ent->levelrespawn = target_levelrespawn;
	ent->ckpoint = level.checkpoint;

	if(ent->spawnflags & 4)
		ent->r.svFlags |= SVF_BROADCAST;

	veccpy(ent->s.origin, ent->s.pos.trBase);

	// must link the entity so we get areas and clusters so
	// the server can determine who to send updates to
	trap_LinkEntity(ent);
}

//==========================================================

/*QUAKED target_laser (0 .5 .8) (-8 -8 -8) (8 8 8) START_ON
When triggered, fires a laser.  You can either set a target or a direction.
*/
void
target_laser_think(ent_t *self)
{
	vec3_t end;
	trace_t tr;
	vec3_t point;

	// if pointed at another entity, set movedir to point at it
	if(self->enemy){
		vecmad(self->enemy->s.origin, 0.5, self->enemy->r.mins, point);
		vecmad(point, 0.5, self->enemy->r.maxs, point);
		vecsub(point, self->s.origin, self->movedir);
		vecnorm(self->movedir);
	}

	// fire forward and see what we hit
	vecmad(self->s.origin, 2048, self->movedir, end);

	trap_Trace(&tr, self->s.origin, nil, nil, end, self->s.number, CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE);

	if(tr.entityNum)
		// hurt it if we can
		entdamage(&g_entities[tr.entityNum], self, self->activator, self->movedir,
			 tr.endpos, self->damage, DAMAGE_NO_KNOCKBACK, MOD_TARGET_LASER);

	veccpy(tr.endpos, self->s.origin2);

	trap_LinkEntity(self);
	self->nextthink = level.time + FRAMETIME;
	self->levelrespawn = target_levelrespawn;
	self->ckpoint = level.checkpoint;
}

void
target_laser_on(ent_t *self)
{
	if(!self->activator)
		self->activator = self;
	target_laser_think(self);
}

void
target_laser_off(ent_t *self)
{
	trap_UnlinkEntity(self);
	self->nextthink = 0;
}

void
target_laser_use(ent_t *self, ent_t *other, ent_t *activator)
{
	self->activator = activator;
	if(self->nextthink > 0)
		target_laser_off(self);
	else
		target_laser_on(self);
}

void
target_laser_start(ent_t *self)
{
	ent_t *ent;

	self->s.eType = ET_BEAM;

	if(self->target){
		ent = findent(nil, FOFS(targetname), self->target);
		if(!ent)
			gprintf("%s at %s: %s is a bad target\n", self->classname, vtos(self->s.origin), self->target);
		self->enemy = ent;
	}else
		setmovedir(self->s.angles, self->movedir);

	self->use = target_laser_use;
	self->think = target_laser_think;

	if(!self->damage)
		self->damage = 1;

	if(self->spawnflags & 1)
		target_laser_on(self);
	else
		target_laser_off(self);
}

void
SP_target_laser(ent_t *self)
{
	// let everything else get spawned before we start firing
	self->think = target_laser_start;
	self->nextthink = level.time + FRAMETIME;
	self->levelrespawn = target_levelrespawn;
	self->ckpoint = level.checkpoint;
}

//==========================================================

void
target_teleporter_use(ent_t *self, ent_t *other, ent_t *activator)
{
	ent_t *dest;

	if(!activator->client)
		return;
	dest = picktarget(self->target);
	if(!dest){
		gprintf("Couldn't find teleporter destination\n");
		return;
	}

	teleportentity(activator, dest->s.origin, dest->s.angles);
}

/*QUAKED target_teleporter (1 0 0) (-8 -8 -8) (8 8 8)
The activator will be teleported away.
*/
void
SP_target_teleporter(ent_t *self)
{
	if(!self->targetname)
		gprintf("untargeted %s at %s\n", self->classname, vtos(self->s.origin));

	self->use = target_teleporter_use;
	self->levelrespawn = target_levelrespawn;
	self->ckpoint = level.checkpoint;
}

//==========================================================

/*QUAKED target_relay (.5 .5 .5) (-8 -8 -8) (8 8 8) RED_ONLY BLUE_ONLY RANDOM
This doesn't perform any actions except fire its targets.
The activator can be forced to be from a certain team.
if RANDOM is checked, only one of the targets will be fired, not all of them
*/
void
target_relay_use(ent_t *self, ent_t *other, ent_t *activator)
{
	if((self->spawnflags & 1) && activator->client
	   && activator->client->sess.team != TEAM_RED)
		return;
	if((self->spawnflags & 2) && activator->client
	   && activator->client->sess.team != TEAM_BLUE)
		return;
	if(self->spawnflags & 4){
		ent_t *ent;

		ent = picktarget(self->target);
		if(ent && ent->use)
			ent->use(ent, self, activator);
		return;
	}
	usetargets(self, activator);
}

void
SP_target_relay(ent_t *self)
{
	self->use = target_relay_use;
	self->levelrespawn = target_levelrespawn;
	self->ckpoint = level.checkpoint;
}

//==========================================================

/*QUAKED target_kill (.5 .5 .5) (-8 -8 -8) (8 8 8)
Kills the activator.
*/
void
target_kill_use(ent_t *self, ent_t *other, ent_t *activator)
{
	entdamage(activator, nil, nil, nil, nil, 100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);
}

void
SP_target_kill(ent_t *self)
{
	self->use = target_kill_use;
	self->levelrespawn = target_levelrespawn;
	self->ckpoint = level.checkpoint;
}

/*QUAKED target_position (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for in-game calculation, like jumppad targets.
*/
void
SP_target_position(ent_t *self)
{
	setorigin(self, self->s.origin);
	self->levelrespawn = target_levelrespawn;
	self->ckpoint = level.checkpoint;
}

static void
target_location_linkup(ent_t *ent)
{
	int i;
	int n;

	if(level.loclinked)
		return;

	level.loclinked = qtrue;

	level.lochead = nil;

	trap_SetConfigstring(CS_LOCATIONS, "unknown");

	for(i = 0, ent = g_entities, n = 1;
	    i < level.nentities;
	    i++, ent++)
		if(ent->classname && !Q_stricmp(ent->classname, "target_location")){
			// lets overload some variables!
			ent->health = n;// use for location marking
			trap_SetConfigstring(CS_LOCATIONS + n, ent->message);
			n++;
			ent->nexttrain = level.lochead;
			level.lochead = ent;
		}

	// All linked together now
}

/*QUAKED target_location (0 0.5 0) (-8 -8 -8) (8 8 8)
Set "message" to the name of this location.
Set "count" to 0-7 for color.
0:white 1:red 2:green 3:yellow 4:blue 5:cyan 6:magenta 7:white

Closest target_location in sight used for the location, if none
in site, closest in distance
*/
void
SP_target_location(ent_t *self)
{
	self->think = target_location_linkup;
	self->nextthink = level.time + 200;	// Let them all spawn first
	self->levelrespawn = target_levelrespawn;
	self->ckpoint = level.checkpoint;

	setorigin(self, self->s.origin);
}

/*
target_secret
*/

void
target_secret_use(ent_t *self, ent_t *other, ent_t *activator)
{
	if(activator->client == nil)
		return;

	level.secretsfound++;
	mksound(other, CHAN_AUTO, self->noiseindex);
	trap_SendServerCommand(activator->client - level.clients,
	   va("cp \"%s\"", self->message));
	usetargets(self, self->activator);
	entfree(self);
}

/*
QUAKED target_secret (0.3 0.1 0.6) (-8 -8 -8) (8 8 8)
Marks out a secret area.
When touched, increments level.secretsfound, triggers its target if any,
then frees itself.
*/
void
SP_target_secret(ent_t *self)
{
	char *s;

	spawnstr("noise", "sound/feedback/secret.wav", &s);
	self->noiseindex = soundindex(s);

	self->use = target_secret_use;
	self->message = "You discovered a secret area!";
	self->levelrespawn = target_levelrespawn;
	self->ckpoint = level.checkpoint;
	self->r.svFlags = SVF_NOCLIENT;
	level.nsecrets++;
	trap_LinkEntity(self);
}

/*
target_changemap
*/

void
target_changemap_use(ent_t *self, ent_t *other, ent_t *activator)
{
	char *cmd, str[MAX_STRING_CHARS];

	// start intermission straight away
	level.intermissionqueued = level.time - INTERMISSION_DELAY_TIME;

	if(trap_Cvar_VariableValue("sv_cheats"))
		cmd = "devmap";
	else
		cmd = "map";
	if(self->message == nil || self->message[0] == '\0'){
		gprintf("target_changemap: no map set\n");
		Com_sprintf(str, sizeof str, "%s limbo", cmd);
		trap_Cvar_Set("nextmap", str);
		return;
	}
	Com_sprintf(str, sizeof str, "%s %s", cmd, self->message);
	trap_Cvar_Set("nextmap", str);
	trap_UnlinkEntity(self);
}

/*
QUAKED target_changemap (0.2 0.9 0.4) (-8 -8 -8) (8 8 8) nointermission
When activated, starts an intermission (unless nointermission is set),
then changes the map.

map		filename of map to change to
*/
void
SP_target_changemap(ent_t *self)
{
	spawnstr("map", "", &self->message);
	self->use = target_changemap_use;
	self->levelrespawn = target_levelrespawn;
	self->ckpoint = level.checkpoint;
	self->r.svFlags = SVF_NOCLIENT;
	trap_LinkEntity(self);
}
