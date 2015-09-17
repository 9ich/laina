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
===============
damagefeedback

Called just before a snapshot is sent to the given player.
Totals up all damage and generates both the player_state_t
damage values to that client for pain blends and kicks, and
global pain sound events for all clients.
===============
*/
void
P_DamageFeedback(ent_t *player)
{
	gclient_t *client;
	float count;
	vec3_t angles;

	client = player->client;
	if(client->ps.pm_type == PM_DEAD)
		return;

	// total points of damage shot at the player this frame
	count = client->dmgblood + client->dmgarmor;
	if(count == 0)
		return;	// didn't take any damage

	if(count > 255)
		count = 255;

	// send the information to the client

	// world damage (falling, slime, etc) uses a special code
	// to make the blend blob centered instead of positional
	if(client->dmgfromworld){
		client->ps.damagePitch = 255;
		client->ps.damageYaw = 255;

		client->dmgfromworld = qfalse;
	}else{
		vectoangles(client->dmgfrom, angles);
		client->ps.damagePitch = angles[PITCH]/360.0 * 256;
		client->ps.damageYaw = angles[YAW]/360.0 * 256;
	}

	// play an apropriate pain sound
	if((level.time > player->paindebouncetime) && !(player->flags & FL_GODMODE)){
		player->paindebouncetime = level.time + 700;
		addevent(player, EV_PAIN, player->health);
		client->ps.damageEvent++;
	}

	client->ps.damageCount = count;

	// clear totals
	client->dmgblood = 0;
	client->dmgarmor = 0;
	client->dmgknockback = 0;
}

/*
=============
P_WorldEffects

Check for lava / slime contents and drowning
=============
*/
void
P_WorldEffects(ent_t *ent)
{
	qboolean envirosuit;
	int waterlevel;

	if(ent->client->noclip){
		ent->airouttime = level.time + 12000;	// don't need air
		return;
	}

	waterlevel = ent->waterlevel;

	envirosuit = ent->client->ps.powerups[PW_BATTLESUIT] > level.time;

	// check for drowning
	if(waterlevel == 3){
		// envirosuit give air
		if(envirosuit)
			ent->airouttime = level.time + 10000;

		// if out of air, start drowning
		if(ent->airouttime < level.time){
			// drown!
			ent->airouttime += 1000;
			if(ent->health > 0){
				// take more damage the longer underwater
				ent->damage += 2;
				if(ent->damage > 15)
					ent->damage = 15;

				// don't play a normal pain sound
				ent->paindebouncetime = level.time + 200;

				entdamage(ent, nil, nil, nil, nil,
					 ent->damage, DAMAGE_NO_ARMOR, MOD_WATER);
			}
		}
	}else{
		ent->airouttime = level.time + 12000;
		ent->damage = 2;
	}

	// check for sizzle damage (move to pmove?)
	if(waterlevel &&
	   (ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)))
		if(ent->health > 0
		   && ent->paindebouncetime <= level.time){
			if(envirosuit)
				addevent(ent, EV_POWERUP_BATTLESUIT, 0);
			else{
				if(ent->watertype & CONTENTS_LAVA)
					entdamage(ent, nil, nil, nil, nil,
						 30*waterlevel, 0, MOD_LAVA);

				if(ent->watertype & CONTENTS_SLIME)
					entdamage(ent, nil, nil, nil, nil,
						 10*waterlevel, 0, MOD_SLIME);
			}
		}
}

/*
===============
setclientsound
===============
*/
void
setclientsound(ent_t *ent)
{
	if(ent->waterlevel && (ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)))
		ent->client->ps.loopSound = level.snd_fry;
	else
		ent->client->ps.loopSound = 0;
}

//==============================================================

/*
==============
ClientImpacts
==============
*/
void
ClientImpacts(ent_t *ent, pmove_t *pm)
{
	int i, j;
	trace_t trace;
	ent_t *other;

	memset(&trace, 0, sizeof(trace));
	for(i = 0; i<pm->numtouch; i++){
		for(j = 0; j<i; j++)
			if(pm->touchents[j] == pm->touchents[i])
				break;
		if(j != i)
			continue;	// duplicated
		other = &g_entities[pm->touchents[i]];

		if((ent->r.svFlags & SVF_BOT) && (ent->touch))
			ent->touch(ent, other, &trace);

		if(!other->touch)
			continue;

		other->touch(other, ent, &trace);
	}
}

/*
============
touchtriggers

Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.
============
*/
void
touchtriggers(ent_t *ent)
{
	int i, num;
	int touch[MAX_GENTITIES];
	ent_t *hit;
	trace_t trace;
	vec3_t mins, maxs;
	static vec3_t range = {40, 40, 52};

	if(!ent->client)
		return;

	// dead clients don't activate triggers!
	if(ent->client->ps.stats[STAT_HEALTH] <= 0)
		return;

	vecsub(ent->client->ps.origin, range, mins);
	vecadd(ent->client->ps.origin, range, maxs);

	num = trap_EntitiesInBox(mins, maxs, touch, MAX_GENTITIES);

	// can't use ent->absmin, because that has a one unit pad
	vecadd(ent->client->ps.origin, ent->r.mins, mins);
	vecadd(ent->client->ps.origin, ent->r.maxs, maxs);

	for(i = 0; i<num; i++){
		hit = &g_entities[touch[i]];

		if(!hit->touch && !ent->touch)
			continue;
		if(!(hit->r.contents & CONTENTS_TRIGGER))
			continue;

		// ignore most entities if a spectator
		if(ent->client->sess.team == TEAM_SPECTATOR)
			if(hit->s.eType != ET_TELEPORT_TRIGGER &&
				// this is ugly but adding a new ET_? type will
				// most likely cause network incompatibilities
			   hit->touch != doortrigger_touch)
				continue;

		// use seperate code for determining if an item is picked up
		// so you don't have to actually contact its bounding box
		if(hit->s.eType == ET_ITEM){
			if(!playertouchingitem(&ent->client->ps, &hit->s, level.time))
				continue;
		}else if(!trap_EntityContact(mins, maxs, hit))
			continue;


		memset(&trace, 0, sizeof(trace));

		if(hit->touch)
			hit->touch(hit, ent, &trace);

		if((ent->r.svFlags & SVF_BOT) && (ent->touch))
			ent->touch(ent, hit, &trace);
	}

	// if we didn't touch a jump pad this pmove frame
	if(ent->client->ps.jumppad_frame != ent->client->ps.pmove_framecount){
		ent->client->ps.jumppad_frame = 0;
		ent->client->ps.jumppad_ent = 0;
	}
}

/*
=================
SpectatorThink
=================
*/
void
SpectatorThink(ent_t *ent, usercmd_t *ucmd)
{
	pmove_t pm;
	gclient_t *client;

	client = ent->client;

	if(client->sess.specstate != SPECTATOR_FOLLOW){
		client->ps.pm_type = PM_SPECTATOR;
		client->ps.speed = 400;	// faster than normal

		// set up for pmove
		memset(&pm, 0, sizeof(pm));
		pm.ps = &client->ps;
		pm.cmd = *ucmd;
		pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;	// spectators can fly through bodies
		pm.trace = trap_Trace;
		pm.pointcontents = trap_PointContents;

		// perform a pmove
		Pmove(&pm);
		// save results of pmove
		veccpy(client->ps.origin, ent->s.origin);

		touchtriggers(ent);
		trap_UnlinkEntity(ent);
	}

	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;

	// attack button cycles through spectators
	if((client->buttons & BUTTON_ATTACK) && !(client->oldbuttons & BUTTON_ATTACK))
		Cmd_FollowCycle_f(ent, 1);
}

/*
=================
ClientInactivityTimer

Returns qfalse if the client is dropped
=================
*/
qboolean
ClientInactivityTimer(gclient_t *client)
{
	if(!g_inactivity.integer){
		// give everyone some time, so if the operator sets g_inactivity during
		// gameplay, everyone isn't kicked
		client->inactivitytime = level.time + 60 * 1000;
		client->inactivitywarning = qfalse;
	}else if(client->pers.cmd.forwardmove ||
		  client->pers.cmd.rightmove ||
		  client->pers.cmd.upmove ||
		  (client->pers.cmd.buttons & BUTTON_ATTACK)){
		client->inactivitytime = level.time + g_inactivity.integer * 1000;
		client->inactivitywarning = qfalse;
	}else if(!client->pers.localclient){
		if(level.time > client->inactivitytime){
			trap_DropClient(client - level.clients, "Dropped due to inactivity");
			return qfalse;
		}
		if(level.time > client->inactivitytime - 10000 && !client->inactivitywarning){
			client->inactivitywarning = qtrue;
			trap_SendServerCommand(client - level.clients, "cp \"Ten seconds until inactivity drop!\n\"");
		}
	}
	return qtrue;
}

/*
==================
ClientTimerActions

Actions that happen once a second
==================
*/
void
ClientTimerActions(ent_t *ent, int msec)
{
	gclient_t *client;

	client = ent->client;
	client->residualtime += msec;

	while(client->residualtime >= 1000)
		client->residualtime -= 1000;

}

/*
====================
ClientIntermissionThink
====================
*/
void
ClientIntermissionThink(gclient_t *client)
{
	client->ps.eFlags &= ~EF_TALK;
	client->ps.eFlags &= ~EF_FIRING;

	// the level will exit when everyone wants to or after timeouts

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = client->pers.cmd.buttons;
	if(client->buttons & (BUTTON_ATTACK | BUTTON_USE_HOLDABLE) & (client->oldbuttons ^ client->buttons))
		// this used to be an ^1 but once a player says ready, it should stick
		client->readytoexit = 1;
}

/*
================
ClientEvents

Events will be passed on to the clients for presentation,
but any server game effects are handled here
================
*/
void
ClientEvents(ent_t *ent, int oldEventSequence)
{
	int i, j;
	int event;
	gclient_t *client;
	vec3_t origin, angles;
//	qboolean	fired;
	item_t *item;
	ent_t *drop;

	client = ent->client;

	if(oldEventSequence < client->ps.eventSequence - MAX_PS_EVENTS)
		oldEventSequence = client->ps.eventSequence - MAX_PS_EVENTS;
	for(i = oldEventSequence; i < client->ps.eventSequence; i++){
		event = client->ps.events[i & (MAX_PS_EVENTS-1)];

		switch(event){
		case EV_FALL_MEDIUM:
		case EV_FALL_FAR:
			break;

		case EV_FIRE_WEAPON:
			fireweapon(ent);
			break;

		case EV_USE_ITEM1:	// teleporter
			// drop flags in CTF
			item = nil;
			j = 0;

			if(ent->client->ps.powerups[PW_REDFLAG]){
				item = finditemforpowerup(PW_REDFLAG);
				j = PW_REDFLAG;
			}else if(ent->client->ps.powerups[PW_BLUEFLAG]){
				item = finditemforpowerup(PW_BLUEFLAG);
				j = PW_BLUEFLAG;
			}else if(ent->client->ps.powerups[PW_NEUTRALFLAG]){
				item = finditemforpowerup(PW_NEUTRALFLAG);
				j = PW_NEUTRALFLAG;
			}

			if(item){
				drop = itemdrop(ent, item, 0);
				// decide how many seconds it has left
				drop->count = (ent->client->ps.powerups[j] - level.time) / 1000;
				if(drop->count < 1)
					drop->count = 1;

				ent->client->ps.powerups[j] = 0;
			}

			selectspawnpoint(ent->client->ps.origin, origin, angles, qfalse);
			teleportentity(ent, origin, angles);
			break;

		case EV_USE_ITEM2:	// medkit
			ent->health += 125;

			break;


		default:
			break;
		}
	}
}


void BotTestSolid(vec3_t origin);

/*
==============
SendPendingPredictableEvents
==============
*/
void
SendPendingPredictableEvents(playerState_t *ps)
{
	ent_t *t;
	int event, seq;
	int extEvent, number;

	// if there are still events pending
	if(ps->entityEventSequence < ps->eventSequence){
		// create a temporary entity for this event which is sent to everyone
		// except the client who generated the event
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		event = ps->events[seq] | ((ps->entityEventSequence & 3) << 8);
		// set external event to zero before calling playerstate2entstate
		extEvent = ps->externalEvent;
		ps->externalEvent = 0;
		// create temporary entity for event
		t = enttemp(ps->origin, event);
		number = t->s.number;
		playerstate2entstate(ps, &t->s, qtrue);
		t->s.number = number;
		t->s.eType = ET_EVENTS + event;
		t->s.eFlags |= EF_PLAYER_EVENT;
		t->s.otherEntityNum = ps->clientNum;
		// send to everyone except the client who generated the event
		t->r.svFlags |= SVF_NOTSINGLECLIENT;
		t->r.singleClient = ps->clientNum;
		// set back external event
		ps->externalEvent = extEvent;
	}
}

/*
==============
clientthink

This will be called once for each client frame, which will
usually be a couple times for each server frame on fast clients.

If "g_synchronousClients 1" is set, this will be called exactly
once for each server frame, which makes for smooth demo recording.
==============
*/
void
ClientThink_real(ent_t *ent)
{
	gclient_t *client;
	pmove_t pm;
	int oldEventSequence;
	int msec;
	usercmd_t *ucmd;

	client = ent->client;

	// don't think if the client is not yet connected (and thus not yet spawned in)
	if(client->pers.connected != CON_CONNECTED)
		return;
	// mark the time, so the connection sprite can be removed
	ucmd = &ent->client->pers.cmd;

	// sanity check the command time to prevent speedup cheating
	if(ucmd->serverTime > level.time + 200)
		ucmd->serverTime = level.time + 200;
//		gprintf("serverTime <<<<<\n" );
	if(ucmd->serverTime < level.time - 1000)
		ucmd->serverTime = level.time - 1000;
//		gprintf("serverTime >>>>>\n" );

	msec = ucmd->serverTime - client->ps.commandTime;
	// following others may result in bad times, but we still want
	// to check for follow toggles
	if(msec < 1 && client->sess.specstate != SPECTATOR_FOLLOW)
		return;
	if(msec > 200)
		msec = 200;

	if(pmove_msec.integer < 8)
		trap_Cvar_Set("pmove_msec", "8");
	else if(pmove_msec.integer > 33)
		trap_Cvar_Set("pmove_msec", "33");

	if(pmove_fixed.integer || client->pers.pmovefixed)
		ucmd->serverTime = ((ucmd->serverTime + pmove_msec.integer-1) / pmove_msec.integer) * pmove_msec.integer;
		//if(ucmd->serverTime - client->ps.commandTime <= 0)
		//	return;

	// check for exiting intermission
	if(level.intermissiontime){
		ClientIntermissionThink(client);
		return;
	}

	// spectators don't do much
	if(client->sess.team == TEAM_SPECTATOR){
		if(client->sess.specstate == SPECTATOR_SCOREBOARD)
			return;
		SpectatorThink(ent, ucmd);
		return;
	}

	// check for inactivity timer, but never drop the local client of a non-dedicated server
	if(!ClientInactivityTimer(client))
		return;

	// clear the rewards if time
	if(level.time > client->rewardtime)
		client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP);

	if(client->noclip)
		client->ps.pm_type = PM_NOCLIP;
	else if(client->ps.stats[STAT_HEALTH] <= 0)
		client->ps.pm_type = PM_DEAD;
	else
		client->ps.pm_type = PM_NORMAL;

	client->ps.gravity = g_gravity.value;

	// set speed
	client->ps.speed = g_speed.value;

	if(client->ps.powerups[PW_HASTE])
		client->ps.speed *= 1.3;

	// Let go of the hook if we aren't firing
	if(client->ps.weapon == WP_GRAPPLING_HOOK &&
	   client->hook && !(ucmd->buttons & BUTTON_ATTACK))
		weapon_hook_free(client->hook);

	// set up for pmove
	oldEventSequence = client->ps.eventSequence;

	memset(&pm, 0, sizeof(pm));

	// check for the hit-scan gauntlet, don't let the action
	// go through as an attack unless it actually hits something
	if(client->ps.weapon == WP_GAUNTLET && !(ucmd->buttons & BUTTON_TALK) &&
	   (ucmd->buttons & BUTTON_ATTACK) && client->ps.weaponTime <= 0)
		pm.gauntlethit = chkgauntletattack(ent);

	if(ent->flags & FL_FORCE_GESTURE){
		ent->flags &= ~FL_FORCE_GESTURE;
		ent->client->pers.cmd.buttons |= BUTTON_GESTURE;
	}


	pm.ps = &client->ps;
	pm.cmd = *ucmd;
	if(pm.ps->pm_type == PM_DEAD)
		pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
	else if(ent->r.svFlags & SVF_BOT)
		pm.tracemask = MASK_PLAYERSOLID | CONTENTS_BOTCLIP;
	else
		pm.tracemask = MASK_PLAYERSOLID;
	pm.trace = trap_Trace;
	pm.pointcontents = trap_PointContents;
	pm.debuglevel = g_debugMove.integer;
	pm.nofootsteps = (g_dmflags.integer & DF_NO_FOOTSTEPS) > 0;

	pm.pmove_fixed = pmove_fixed.integer | client->pers.pmovefixed;
	pm.pmove_msec = pmove_msec.integer;

	veccpy(client->ps.origin, client->oldorigin);

	Pmove(&pm);

	// save results of pmove
	if(ent->client->ps.eventSequence != oldEventSequence)
		ent->eventtime = level.time;
	if(g_smoothClients.integer)
		playerstate2entstatexerp(&ent->client->ps, &ent->s, ent->client->ps.commandTime, qtrue);
	else
		playerstate2entstate(&ent->client->ps, &ent->s, qtrue);
	SendPendingPredictableEvents(&ent->client->ps);

	if(!(ent->client->ps.eFlags & EF_FIRING))
		client->fireheld = qfalse;	// for grapple

	// use the snapped origin for linking so it matches client predicted versions
	veccpy(ent->s.pos.trBase, ent->r.currentOrigin);

	veccpy(pm.mins, ent->r.mins);
	veccpy(pm.maxs, ent->r.maxs);

	ent->waterlevel = pm.waterlevel;
	ent->watertype = pm.watertype;

	// execute client events
	ClientEvents(ent, oldEventSequence);

	// link entity now, after any personal teleporters have been used
	trap_LinkEntity(ent);
	if(!ent->client->noclip)
		touchtriggers(ent);

	// NOTE: now copy the exact origin over otherwise clients can be snapped into solid
	veccpy(ent->client->ps.origin, ent->r.currentOrigin);

	//test for solid areas in the AAS file
	BotTestAAS(ent->r.currentOrigin);

	// touch other objects
	ClientImpacts(ent, &pm);

	// save results of triggers and client events
	if(ent->client->ps.eventSequence != oldEventSequence)
		ent->eventtime = level.time;

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latchedbuttons |= client->buttons & ~client->oldbuttons;

	// check for respawning
	if(client->ps.stats[STAT_HEALTH] <= 0){
		// wait for the attack button to be pressed
		if(level.time > client->respawntime){
			// forcerespawn is to prevent users from waiting out powerups
			if(g_forcerespawn.integer > 0 &&
			   (level.time - client->respawntime) > g_forcerespawn.integer * 1000){
				clientrespawn(ent);
				return;
			}

			// pressing attack or use is the normal respawn method
			if(ucmd->buttons & (BUTTON_ATTACK | BUTTON_USE_HOLDABLE))
				clientrespawn(ent);
		}
		return;
	}

	// perform once-a-second actions
	ClientTimerActions(ent, msec);
}

/*
==================
clientthink

A new command has arrived from the client
==================
*/
void
clientthink(int clientNum)
{
	ent_t *ent;

	ent = g_entities + clientNum;
	trap_GetUsercmd(clientNum, &ent->client->pers.cmd);

	// mark the time we got info, so we can display the
	// phone jack if they don't get any for a while
	ent->client->lastcmdtime = level.time;

	if(!(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer)
		ClientThink_real(ent);

	ent->client->ps.persistant[PERS_TOTALSECRETS] = level.nsecrets;
	ent->client->ps.persistant[PERS_SECRETSFOUND] = level.secretsfound;
	ent->client->ps.persistant[PERS_TOTALCRATES] = level.ncrates;
	ent->client->ps.persistant[PERS_CRATESBROKEN] = level.ncratesbroken;
	ent->client->ps.persistant[PERS_TOTALCARROTS] = level.ncarrots;
	ent->client->ps.persistant[PERS_CARROTSPICKEDUP] = level.ncarrotspickedup;
}

void
runclient(ent_t *ent)
{
	if(!(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer)
		return;
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink_real(ent);
	ent->client->ps.persistant[PERS_TOTALSECRETS] = level.nsecrets;
	ent->client->ps.persistant[PERS_SECRETSFOUND] = level.secretsfound;
	ent->client->ps.persistant[PERS_TOTALCRATES] = level.ncrates;
	ent->client->ps.persistant[PERS_CRATESBROKEN] = level.ncratesbroken;
	ent->client->ps.persistant[PERS_TOTALCARROTS] = level.ncarrots;
	ent->client->ps.persistant[PERS_CARROTSPICKEDUP] = level.ncarrotspickedup;
}

/*
==================
SpectatorClientEndFrame

==================
*/
void
SpectatorClientEndFrame(ent_t *ent)
{
	gclient_t *cl;

	// if we are doing a chase cam or a remote view, grab the latest info
	if(ent->client->sess.specstate == SPECTATOR_FOLLOW){
		int clientNum, flags;

		clientNum = ent->client->sess.specclient;

		// team follow1 and team follow2 go to whatever clients are playing
		if(clientNum == -1)
			clientNum = level.follow1;
		else if(clientNum == -2)
			clientNum = level.follow2;
		if(clientNum >= 0){
			cl = &level.clients[clientNum];
			if(cl->pers.connected == CON_CONNECTED && cl->sess.team != TEAM_SPECTATOR){
				flags = (cl->ps.eFlags & ~(EF_VOTED | EF_TEAMVOTED)) | (ent->client->ps.eFlags & (EF_VOTED | EF_TEAMVOTED));
				ent->client->ps = cl->ps;
				ent->client->ps.pm_flags |= PMF_FOLLOW;
				ent->client->ps.eFlags = flags;
				return;
			}else
			// drop them to free spectators unless they are dedicated camera followers
			if(ent->client->sess.specclient >= 0){
				ent->client->sess.specstate = SPECTATOR_FREE;
				clientbegin(ent->client - level.clients);
			}
		}
	}
}

/*
==============
clientendframe

Called at the end of each server frame for each connected client
A fast client will have multiple clientthink for each ClientEdFrame,
while a slow client may have multiple clientendframe between clientthink.
==============
*/
void
clientendframe(ent_t *ent)
{
	int i;

	if(ent->client->sess.team == TEAM_SPECTATOR){
		SpectatorClientEndFrame(ent);
		return;
	}

	// turn off any expired powerups
	for(i = 0; i < MAX_POWERUPS; i++)
		if(ent->client->ps.powerups[i] < level.time)
			ent->client->ps.powerups[i] = 0;


	// save network bandwidth
#if 0
	if(!g_synchronousClients->integer && ent->client->ps.pm_type == PM_NORMAL)
		// FIXME: this must change eventually for non-sync demo recording
		vecclear(ent->client->ps.viewangles);

#endif

	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	if(level.intermissiontime)
		return;

	// burn from lava, etc
	P_WorldEffects(ent);

	// apply all the damage taken this frame
	P_DamageFeedback(ent);

	// add the EF_CONNECTION flag if we haven't gotten commands recently
	if(level.time - ent->client->lastcmdtime > 1000)
		ent->client->ps.eFlags |= EF_CONNECTION;
	else
		ent->client->ps.eFlags &= ~EF_CONNECTION;

	ent->client->ps.stats[STAT_HEALTH] = ent->health;	// FIXME: get rid of ent->health...

	setclientsound(ent);

	// set the latest infor
	if(g_smoothClients.integer)
		playerstate2entstatexerp(&ent->client->ps, &ent->s, ent->client->ps.commandTime, qtrue);
	else
		playerstate2entstate(&ent->client->ps, &ent->s, qtrue);
	SendPendingPredictableEvents(&ent->client->ps);

	// set the bit for the reachability area the client is currently in
//	i = trap_AAS_PointReachabilityAreaIndex( ent->client->ps.origin );
//	ent->client->areabits[i >> 3] |= 1 << (i & 7);

	// add player trail so monsters can follow
	if(!visible(ent, traillastspot())){
		gprintf("adding a trail %s %s\n", vtos(ent->s.pos.trBase),
		   vtos(traillastspot()->s.pos.trBase));
		trailadd(ent->s.pos.trBase);
	}
}
