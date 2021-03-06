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

/*
This file acts on changes in a new playerState_t.  With normal play,
this will be done after local prediction, but when following another
player or playing back a demo, it will be checked when the snapshot
transitions like all the other entities.
*/

#include "cg_local.h"

/*
A respawn happened this snapshot
*/
void
respawn(void)
{
	// no error decay on player movement
	cg.teleportthisframe = qtrue;

	// display weapons available
	cg.weapseltime = cg.time;

	// select the weapon the server says we are using
	cg.weapsel = cg.snap->ps.weapon;

	// set the display stats
	cg.disptokens = cg.snap->ps.stats[STAT_TOKENS];
	cg.displives = cg.snap->ps.persistant[PERS_LIVES];
}

void
chkpsevents(playerState_t *ps, playerState_t *ops)
{
	int i;
	int event;
	cent_t *cent;

	if(ps->externalEvent && ps->externalEvent != ops->externalEvent){
		cent = &cg_entities[ps->clientNum];
		cent->currstate.event = ps->externalEvent;
		cent->currstate.eventParm = ps->externalEventParm;
		entevent(cent, cent->lerporigin);
	}

	cent = &cg.pplayerent;	// cg_entities[ ps->clientNum ];
	// go through the predictable events buffer
	for(i = ps->eventSequence - MAX_PS_EVENTS; i < ps->eventSequence; i++)
		// if we have a new predictable event
		if(i >= ops->eventSequence
			// or the server told us to play another event instead of a predicted event we already issued
			// or something the server told us changed our prediction causing a different event
		   || (i > ops->eventSequence - MAX_PS_EVENTS && ps->events[i & (MAX_PS_EVENTS-1)] != ops->events[i & (MAX_PS_EVENTS-1)])){
			event = ps->events[i & (MAX_PS_EVENTS-1)];
			cent->currstate.event = event;
			cent->currstate.eventParm = ps->eventParms[i & (MAX_PS_EVENTS-1)];
			entevent(cent, cent->lerporigin);

			cg.pevents[i & (MAX_PREDICTED_EVENTS-1)] = event;

			cg.eventSequence++;
		}
}

void
chkpredictableevents(playerState_t *ps)
{
	int i;
	int event;
	cent_t *cent;

	cent = &cg.pplayerent;
	for(i = ps->eventSequence - MAX_PS_EVENTS; i < ps->eventSequence; i++){
		if(i >= cg.eventSequence)
			continue;
		// if this event is not further back in than the maximum
		// predictable events we remember
		if(i > cg.eventSequence - MAX_PREDICTED_EVENTS)
			// if the new playerstate event is different from a previously predicted one
			if(ps->events[i & (MAX_PS_EVENTS-1)] !=
			   cg.pevents[i & (MAX_PREDICTED_EVENTS-1)]){
				event = ps->events[i & (MAX_PS_EVENTS-1)];
				cent->currstate.event = event;
				cent->currstate.eventParm = ps->eventParms[i & (MAX_PS_EVENTS-1)];
				entevent(cent, cent->lerporigin);

				cg.pevents[i & (MAX_PREDICTED_EVENTS-1)] = event;

				if(cg_showmiss.integer)
					cgprintf("WARNING: changed predicted event\n");
			}
	}
}

static void
pushreward(sfxHandle_t sfx, qhandle_t shader, int nrewards)
{
	if(cg.rewardstack < (MAX_REWARDSTACK-1)){
		cg.rewardstack++;
		cg.rewardsounds[cg.rewardstack] = sfx;
		cg.rewardshaders[cg.rewardstack] = shader;
		cg.nrewards[cg.rewardstack] = nrewards;
	}
}

void
feedback(playerState_t *ps, playerState_t *ops)
{
	int i, highScore, reward;
	sfxHandle_t sfx;

	// don't play the sounds if the player just changed teams
	if(ps->persistant[PERS_TEAM] != ops->persistant[PERS_TEAM])
		return;

	// hit changes
	if(ps->persistant[PERS_HITS] > ops->persistant[PERS_HITS]){
		trap_S_StartLocalSound(cgs.media.hitSound, CHAN_LOCAL_SOUND);
	}else if(ps->persistant[PERS_HITS] < ops->persistant[PERS_HITS])
		trap_S_StartLocalSound(cgs.media.hitTeamSound, CHAN_LOCAL_SOUND);

	// health changes downward cause pain fx
	if(ps->stats[STAT_HEALTH] < ops->stats[STAT_HEALTH])
		if(ps->stats[STAT_HEALTH] > 0)
			painevent(&cg.pplayerent, ps->stats[STAT_HEALTH]);
	
	//
	// hud pickup animations
	//

	for(i = 0; i < ps->stats[STAT_TOKENS] - ops->stats[STAT_TOKENS]; i++)
		queuepickupanim("item_token");

	// If carrots pushed us over the extra-life threshold, also add anims for
	// the number of carrots that were required to reach that threshold.
	if(ps->persistant[PERS_LIVES] > ops->persistant[PERS_LIVES] &&
	   ps->stats[STAT_TOKENS] < ops->stats[STAT_TOKENS]){
		for(i = 0; i <= 100 - ops->stats[STAT_TOKENS]; i++)
			queuepickupanim("item_token");
	}

	for(i = 0; i < ps->persistant[PERS_LIVES] - ops->persistant[PERS_LIVES]; i++)
		queuepickupanim("item_life");

	for(i = 0; i < ps->inv[INV_KEY_JADE] - ops->inv[INV_KEY_JADE]; i++)
		queuepickupanim("item_key_jade");
	for(i = 0; i < ps->inv[INV_KEY_RUBY] - ops->inv[INV_KEY_RUBY]; i++)
		queuepickupanim("item_key_ruby");
	for(i = 0; i < ps->inv[INV_KEY_SAPPHIRE] - ops->inv[INV_KEY_SAPPHIRE]; i++)
		queuepickupanim("item_key_sapphire");

	//
	// reward sounds
	//

	// if we are going into the intermission, don't start any voices
	if(cg.intermissionstarted)
		return;

	reward = qfalse;
	if(ps->persistant[PERS_CAPTURES] != ops->persistant[PERS_CAPTURES]){
		pushreward(cgs.media.captureAwardSound, cgs.media.medalCapture, ps->persistant[PERS_CAPTURES]);
		reward = qtrue;
		//Com_Printf("capture\n");
	}
	if(ps->persistant[PERS_IMPRESSIVE_COUNT] != ops->persistant[PERS_IMPRESSIVE_COUNT]){
		sfx = cgs.media.impressiveSound;
		pushreward(sfx, cgs.media.medalImpressive, ps->persistant[PERS_IMPRESSIVE_COUNT]);
		reward = qtrue;
		//Com_Printf("impressive\n");
	}
	if(ps->persistant[PERS_EXCELLENT_COUNT] != ops->persistant[PERS_EXCELLENT_COUNT]){
		sfx = cgs.media.excellentSound;
		pushreward(sfx, cgs.media.medalExcellent, ps->persistant[PERS_EXCELLENT_COUNT]);
		reward = qtrue;
		//Com_Printf("excellent\n");
	}
	if(ps->persistant[PERS_GAUNTLET_FRAG_COUNT] != ops->persistant[PERS_GAUNTLET_FRAG_COUNT]){
		sfx = cgs.media.humiliationSound;
		pushreward(sfx, cgs.media.medalGauntlet, ps->persistant[PERS_GAUNTLET_FRAG_COUNT]);
		reward = qtrue;
		//Com_Printf("gauntlet frag\n");
	}
	if(ps->persistant[PERS_DEFEND_COUNT] != ops->persistant[PERS_DEFEND_COUNT]){
		pushreward(cgs.media.defendSound, cgs.media.medalDefend, ps->persistant[PERS_DEFEND_COUNT]);
		reward = qtrue;
		//Com_Printf("defend\n");
	}
	if(ps->persistant[PERS_ASSIST_COUNT] != ops->persistant[PERS_ASSIST_COUNT]){
		pushreward(cgs.media.assistSound, cgs.media.medalAssist, ps->persistant[PERS_ASSIST_COUNT]);
		reward = qtrue;
		//Com_Printf("assist\n");
	}
	// if any of the player event bits changed
	if(ps->persistant[PERS_PLAYEREVENTS] != ops->persistant[PERS_PLAYEREVENTS]){
		if((ps->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_DENIEDREWARD) !=
		   (ops->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_DENIEDREWARD))
			trap_S_StartLocalSound(cgs.media.deniedSound, CHAN_ANNOUNCER);
		else if((ps->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_GAUNTLETREWARD) !=
			(ops->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_GAUNTLETREWARD))
			trap_S_StartLocalSound(cgs.media.humiliationSound, CHAN_ANNOUNCER);
		else if((ps->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_HOLYSHIT) !=
			(ops->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_HOLYSHIT))
			trap_S_StartLocalSound(cgs.media.holyShitSound, CHAN_ANNOUNCER);
		reward = qtrue;
	}

	// check for flag pickup
	if(cgs.gametype > GT_TEAM)
		if((ps->powerups[PW_REDFLAG] != ops->powerups[PW_REDFLAG] && ps->powerups[PW_REDFLAG]) ||
		   (ps->powerups[PW_BLUEFLAG] != ops->powerups[PW_BLUEFLAG] && ps->powerups[PW_BLUEFLAG]) ||
		   (ps->powerups[PW_NEUTRALFLAG] != ops->powerups[PW_NEUTRALFLAG] && ps->powerups[PW_NEUTRALFLAG]))
			trap_S_StartLocalSound(cgs.media.youHaveFlagSound, CHAN_ANNOUNCER);

	// lead changes
	if(!reward)
		if(!cg.warmup){
			// never play lead changes during warmup
			if(ps->persistant[PERS_RANK] != ops->persistant[PERS_RANK])
				if(cgs.gametype < GT_TEAM){
					if(ps->persistant[PERS_RANK] == 0)
						addbufferedsound(cgs.media.takenLeadSound);
					else if(ps->persistant[PERS_RANK] == RANK_TIED_FLAG)
						addbufferedsound(cgs.media.tiedLeadSound);
					else if((ops->persistant[PERS_RANK] & ~RANK_TIED_FLAG) == 0)
						addbufferedsound(cgs.media.lostLeadSound);
				}
		}

	// timelimit warnings
	if(cgs.timelimit > 0){
		int msec;

		msec = cg.time - cgs.levelStartTime;
		if(!(cg.timelimitwarnings & 4) && msec > (cgs.timelimit * 60 + 2) * 1000){
			cg.timelimitwarnings |= 1 | 2 | 4;
			trap_S_StartLocalSound(cgs.media.suddenDeathSound, CHAN_ANNOUNCER);
		}else if(!(cg.timelimitwarnings & 2) && msec > (cgs.timelimit - 1) * 60 * 1000){
			cg.timelimitwarnings |= 1 | 2;
			trap_S_StartLocalSound(cgs.media.oneMinuteSound, CHAN_ANNOUNCER);
		}else if(cgs.timelimit > 5 && !(cg.timelimitwarnings & 1) && msec > (cgs.timelimit - 5) * 60 * 1000){
			cg.timelimitwarnings |= 1;
			trap_S_StartLocalSound(cgs.media.fiveMinuteSound, CHAN_ANNOUNCER);
		}
	}

	// fraglimit warnings
	if(cgs.fraglimit > 0 && cgs.gametype < GT_CTF){
		highScore = cgs.scores1;

		if(cgs.gametype == GT_TEAM && cgs.scores2 > highScore)
			highScore = cgs.scores2;

		if(!(cg.fraglimitwarnings & 4) && highScore == (cgs.fraglimit - 1)){
			cg.fraglimitwarnings |= 1 | 2 | 4;
			addbufferedsound(cgs.media.oneFragSound);
		}else if(cgs.fraglimit > 2 && !(cg.fraglimitwarnings & 2) && highScore == (cgs.fraglimit - 2)){
			cg.fraglimitwarnings |= 1 | 2;
			addbufferedsound(cgs.media.twoFragSound);
		}else if(cgs.fraglimit > 3 && !(cg.fraglimitwarnings & 1) && highScore == (cgs.fraglimit - 3)){
			cg.fraglimitwarnings |= 1;
			addbufferedsound(cgs.media.threeFragSound);
		}
	}
}

void
damagefeedback(int yawbyte, int pitchbyte, int damage)
{
	float left, front, up, kick;
	int health;
	float scale;
	vec3_t dir, angles;
	float dist, yaw, pitch;

	// show the attacking player's head and name in corner
	cg.attackertime = cg.time;

	// the lower on health you are, the greater the view kick will be
	health = cg.snap->ps.stats[STAT_HEALTH];
	if(health < 40)
		scale = 1;
	else
		scale = 40.0 / health;
	kick = damage * scale;

	if(kick < 5)
		kick = 5;
	if(kick > 10)
		kick = 10;

	// if yaw and pitch are both 255, make the damage always centered (falling, etc)
	if(yawbyte == 255 && pitchbyte == 255){
		cg.dmgx = 0;
		cg.dmgy = 0;
		cg.vdmgroll = 0;
		cg.vdmgpitch = -kick;
	}else{
		// positional
		pitch = pitchbyte / 255.0 * 360;
		yaw = yawbyte / 255.0 * 360;

		angles[PITCH] = pitch;
		angles[YAW] = yaw;
		angles[ROLL] = 0;

		anglevecs(angles, dir, nil, nil);
		vecsub(vec3_origin, dir, dir);

		front = vecdot(dir, cg.refdef.viewaxis[0]);
		left = vecdot(dir, cg.refdef.viewaxis[1]);
		up = vecdot(dir, cg.refdef.viewaxis[2]);

		dir[0] = front;
		dir[1] = left;
		dir[2] = 0;
		dist = veclen(dir);
		if(dist < 0.1)
			dist = 0.1f;

		cg.vdmgroll = kick * left;

		cg.vdmgpitch = -kick * front;

		if(front <= 0.1)
			front = 0.1f;
		cg.dmgx = -left / front;
		cg.dmgy = up / dist;
	}

	// clamp the position
	if(cg.dmgx > 1.0)
		cg.dmgx = 1.0;
	if(cg.dmgx < -1.0)
		cg.dmgx = -1.0;

	if(cg.dmgy > 1.0)
		cg.dmgy = 1.0;
	if(cg.dmgy < -1.0)
		cg.dmgy = -1.0;

	// don't let the screen flashes vary as much
	if(kick > 10)
		kick = 10;
	cg.dmgval = kick;
	cg.vdmgtime = cg.time + DAMAGE_TIME;
	cg.dmgtime = cg.snap->serverTime;
}

void
pstransition(playerState_t *ps, playerState_t *ops)
{
	// check for changing follow mode
	if(ps->clientNum != ops->clientNum){
		cg.teleportthisframe = qtrue;
		// make sure we don't get any unwanted transition effects
		*ops = *ps;
	}

	// damage events (player is getting wounded)
	if(ps->damageEvent != ops->damageEvent && ps->damageCount)
		damagefeedback(ps->damageYaw, ps->damagePitch, ps->damageCount);

	// respawning
	if(ps->persistant[PERS_SPAWN_COUNT] != ops->persistant[PERS_SPAWN_COUNT])
		respawn();

	if(cg.maprestart){
		respawn();
		cg.maprestart = qfalse;
	}

	if(cg.snap->ps.pm_type != PM_INTERMISSION
	   && ps->persistant[PERS_TEAM] != TEAM_SPECTATOR)
		feedback(ps, ops);

	// run events
	chkpsevents(ps, ops);

	// smooth the ducking viewheight change
	if(ps->viewheight != ops->viewheight){
		cg.duckchange = ps->viewheight - ops->viewheight;
		cg.ducktime = cg.time;
	}
}
