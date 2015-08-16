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

typedef struct teamgame_s
{
	float		last_flag_capture;
	int		last_capture_team;
	flagStatus_t	redStatus;	// CTF
	flagStatus_t	blueStatus;	// CTF
	flagStatus_t	flagStatus;	// One Flag CTF
	int		redTakenTime;
	int		blueTakenTime;
	int		redObeliskAttackedTime;
	int		blueObeliskAttackedTime;
} teamgame_t;

teamgame_t teamgame;

ent_t *neutralObelisk;

void Team_SetFlagStatus(int team, flagStatus_t status);

void
teaminitgame(void)
{
	memset(&teamgame, 0, sizeof teamgame);

	switch(g_gametype.integer){
	case GT_CTF:
		teamgame.redStatus = -1;// Invalid to force update
		Team_SetFlagStatus(TEAM_RED, FLAG_ATBASE);
		teamgame.blueStatus = -1;	// Invalid to force update
		Team_SetFlagStatus(TEAM_BLUE, FLAG_ATBASE);
		break;
	default:
		break;
	}
}

int
otherteam(int team)
{
	if(team==TEAM_RED)
		return TEAM_BLUE;
	else if(team==TEAM_BLUE)
		return TEAM_RED;
	return team;
}

const char *
teamname(int team)
{
	if(team==TEAM_RED)
		return "RED";
	else if(team==TEAM_BLUE)
		return "BLUE";
	else if(team==TEAM_SPECTATOR)
		return "SPECTATOR";
	return "FREE";
}

const char *
teamcolorstr(int team)
{
	if(team==TEAM_RED)
		return S_COLOR_RED;
	else if(team==TEAM_BLUE)
		return S_COLOR_BLUE;
	else if(team==TEAM_SPECTATOR)
		return S_COLOR_YELLOW;
	return S_COLOR_WHITE;
}

// nil for everyone
static __attribute__((format(printf, 2, 3))) void QDECL
PrintMsg(ent_t *ent, const char *fmt, ...)
{
	char msg[1024];
	va_list argptr;
	char *p;

	va_start(argptr, fmt);
	if(Q_vsnprintf(msg, sizeof(msg), fmt, argptr) >= sizeof(msg))
		errorf("PrintMsg overrun");
	va_end(argptr);

	// double quotes are bad
	while((p = strchr(msg, '"')) != nil)
		*p = '\'';

	trap_SendServerCommand(((ent == nil) ? -1 : ent-g_entities), va("print \"%s\"", msg));
}

/*
==============
addteamscore

 used for gametype > GT_TEAM
 for gametype GT_TEAM the level.teamscores is updated in addscore in g_combat.c
==============
*/
void
addteamscore(vec3_t origin, int team, int score)
{
	ent_t *te;

	te = enttemp(origin, EV_GLOBAL_TEAM_SOUND);
	te->r.svFlags |= SVF_BROADCAST;

	if(team == TEAM_RED){
		if(level.teamscores[TEAM_RED] + score == level.teamscores[TEAM_BLUE])
			//teams are tied sound
			te->s.eventParm = GTS_TEAMS_ARE_TIED;
		else if(level.teamscores[TEAM_RED] <= level.teamscores[TEAM_BLUE] &&
			level.teamscores[TEAM_RED] + score > level.teamscores[TEAM_BLUE])
			// red took the lead sound
			te->s.eventParm = GTS_REDTEAM_TOOK_LEAD;
		else
			// red scored sound
			te->s.eventParm = GTS_REDTEAM_SCORED;
	}else{
		if(level.teamscores[TEAM_BLUE] + score == level.teamscores[TEAM_RED])
			//teams are tied sound
			te->s.eventParm = GTS_TEAMS_ARE_TIED;
		else if(level.teamscores[TEAM_BLUE] <= level.teamscores[TEAM_RED] &&
			level.teamscores[TEAM_BLUE] + score > level.teamscores[TEAM_RED])
			// blue took the lead sound
			te->s.eventParm = GTS_BLUETEAM_TOOK_LEAD;
		else
			// blue scored sound
			te->s.eventParm = GTS_BLUETEAM_SCORED;
	}
	level.teamscores[team] += score;
}

/*
==============
onsameteam
==============
*/
qboolean
onsameteam(ent_t *ent1, ent_t *ent2)
{
	if(!ent1->client || !ent2->client)
		return qfalse;

	if(g_gametype.integer < GT_TEAM)
		return qfalse;

	if(ent1->client->sess.team == ent2->client->sess.team)
		return qtrue;

	return qfalse;
}

static char ctfFlagStatusRemap[] = {'0', '1', '*', '*', '2'};
static char oneFlagStatusRemap[] = {'0', '1', '2', '3', '4'};

void
Team_SetFlagStatus(int team, flagStatus_t status)
{
	qboolean modified = qfalse;

	switch(team){
	case TEAM_RED:	// CTF
		if(teamgame.redStatus != status){
			teamgame.redStatus = status;
			modified = qtrue;
		}
		break;

	case TEAM_BLUE:	// CTF
		if(teamgame.blueStatus != status){
			teamgame.blueStatus = status;
			modified = qtrue;
		}
		break;

	case TEAM_FREE:	// One Flag CTF
		if(teamgame.flagStatus != status){
			teamgame.flagStatus = status;
			modified = qtrue;
		}
		break;
	}

	if(modified){
		char st[4];

		if(g_gametype.integer == GT_CTF){
			st[0] = ctfFlagStatusRemap[teamgame.redStatus];
			st[1] = ctfFlagStatusRemap[teamgame.blueStatus];
			st[2] = 0;
		}else{	// GT_1FCTF
			st[0] = oneFlagStatusRemap[teamgame.flagStatus];
			st[1] = 0;
		}

		trap_SetConfigstring(CS_FLAGSTATUS, st);
	}
}

void
ckhdroppedteamitem(ent_t *dropped)
{
	if(dropped->item->tag == PW_REDFLAG)
		Team_SetFlagStatus(TEAM_RED, FLAG_DROPPED);
	else if(dropped->item->tag == PW_BLUEFLAG)
		Team_SetFlagStatus(TEAM_BLUE, FLAG_DROPPED);
	else if(dropped->item->tag == PW_NEUTRALFLAG)
		Team_SetFlagStatus(TEAM_FREE, FLAG_DROPPED);
}

/*
================
Team_ForceGesture
================
*/
void
Team_ForceGesture(int team)
{
	int i;
	ent_t *ent;

	for(i = 0; i < MAX_CLIENTS; i++){
		ent = &g_entities[i];
		if(!ent->inuse)
			continue;
		if(!ent->client)
			continue;
		if(ent->client->sess.team != team)
			continue;
		ent->flags |= FL_FORCE_GESTURE;
	}
}

/*
================
teamfragbonuses

Calculate the bonuses for flag defense, flag carrier defense, etc.
Note that bonuses are not cumulative.  You get one, they are in importance
order.
================
*/
void
teamfragbonuses(ent_t *targ, ent_t *inflictor, ent_t *attacker)
{
	int i;
	ent_t *ent;
	int flag_pw, enemy_flag_pw;
	int other;
	int tokens;
	ent_t *flag, *carrier = nil;
	char *c;
	vec3_t v1, v2;
	int team;

	// no bonus for fragging yourself or team mates
	if(!targ->client || !attacker->client || targ == attacker || onsameteam(targ, attacker))
		return;

	team = targ->client->sess.team;
	other = otherteam(targ->client->sess.team);
	if(other < 0)
		return;		// whoever died isn't on a team

	// same team, if the flag at base, check to he has the enemy flag
	if(team == TEAM_RED){
		flag_pw = PW_REDFLAG;
		enemy_flag_pw = PW_BLUEFLAG;
	}else{
		flag_pw = PW_BLUEFLAG;
		enemy_flag_pw = PW_REDFLAG;
	}


	// did the attacker frag the flag carrier?
	tokens = 0;
	if(targ->client->ps.powerups[enemy_flag_pw]){
		attacker->client->pers.teamstate.lastfraggedcarrier = level.time;
		addscore(attacker, targ->r.currentOrigin, CTF_FRAG_CARRIER_BONUS);
		attacker->client->pers.teamstate.fragcarrier++;
		PrintMsg(nil, "%s" S_COLOR_WHITE " fragged %s's flag carrier!\n",
			 attacker->client->pers.netname, teamname(team));

		// the target had the flag, clear the hurt carrier
		// field on the other team
		for(i = 0; i < g_maxclients.integer; i++){
			ent = g_entities + i;
			if(ent->inuse && ent->client->sess.team == other)
				ent->client->pers.teamstate.lasthurtcarrier = 0;
		}
		return;
	}

	// did the attacker frag a head carrier? other->client->ps.generic1
	if(tokens){
		attacker->client->pers.teamstate.lastfraggedcarrier = level.time;
		addscore(attacker, targ->r.currentOrigin, CTF_FRAG_CARRIER_BONUS * tokens * tokens);
		attacker->client->pers.teamstate.fragcarrier++;
		PrintMsg(nil, "%s" S_COLOR_WHITE " fragged %s's skull carrier!\n",
			 attacker->client->pers.netname, teamname(team));

		// the target had the flag, clear the hurt carrier
		// field on the other team
		for(i = 0; i < g_maxclients.integer; i++){
			ent = g_entities + i;
			if(ent->inuse && ent->client->sess.team == other)
				ent->client->pers.teamstate.lasthurtcarrier = 0;
		}
		return;
	}

	if(targ->client->pers.teamstate.lasthurtcarrier &&
	   level.time - targ->client->pers.teamstate.lasthurtcarrier < CTF_CARRIER_DANGER_PROTECT_TIMEOUT &&
	   !attacker->client->ps.powerups[flag_pw]){
		// attacker is on the same team as the flag carrier and
		// fragged a guy who hurt our flag carrier
		addscore(attacker, targ->r.currentOrigin, CTF_CARRIER_DANGER_PROTECT_BONUS);

		attacker->client->pers.teamstate.carrierdefense++;
		targ->client->pers.teamstate.lasthurtcarrier = 0;

		attacker->client->ps.persistant[PERS_DEFEND_COUNT]++;
		// add the sprite over the player's head
		attacker->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP);
		attacker->client->ps.eFlags |= EF_AWARD_DEFEND;
		attacker->client->rewardtime = level.time + REWARD_SPRITE_TIME;

		return;
	}

	if(targ->client->pers.teamstate.lasthurtcarrier &&
	   level.time - targ->client->pers.teamstate.lasthurtcarrier < CTF_CARRIER_DANGER_PROTECT_TIMEOUT){
		// attacker is on the same team as the skull carrier and
		addscore(attacker, targ->r.currentOrigin, CTF_CARRIER_DANGER_PROTECT_BONUS);

		attacker->client->pers.teamstate.carrierdefense++;
		targ->client->pers.teamstate.lasthurtcarrier = 0;

		attacker->client->ps.persistant[PERS_DEFEND_COUNT]++;
		// add the sprite over the player's head
		attacker->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP);
		attacker->client->ps.eFlags |= EF_AWARD_DEFEND;
		attacker->client->rewardtime = level.time + REWARD_SPRITE_TIME;

		return;
	}

	// flag and flag carrier area defense bonuses

	// we have to find the flag and carrier entities

	// find the flag
	switch(attacker->client->sess.team){
	case TEAM_RED:
		c = "team_CTF_redflag";
		break;
	case TEAM_BLUE:
		c = "team_CTF_blueflag";
		break;
	default:
		return;
	}
	// find attacker's team's flag carrier
	for(i = 0; i < g_maxclients.integer; i++){
		carrier = g_entities + i;
		if(carrier->inuse && carrier->client->ps.powerups[flag_pw])
			break;
		carrier = nil;
	}
	flag = nil;
	while((flag = findent(flag, FOFS(classname), c)) != nil)
		if(!(flag->flags & FL_DROPPED_ITEM))
			break;

	if(!flag)
		return;		// can't find attacker's flag

	// ok we have the attackers flag and a pointer to the carrier

	// check to see if we are defending the base's flag
	vecsub(targ->r.currentOrigin, flag->r.currentOrigin, v1);
	vecsub(attacker->r.currentOrigin, flag->r.currentOrigin, v2);

	if(((veclen(v1) < CTF_TARGET_PROTECT_RADIUS &&
	     trap_InPVS(flag->r.currentOrigin, targ->r.currentOrigin)) ||
	    (veclen(v2) < CTF_TARGET_PROTECT_RADIUS &&
	     trap_InPVS(flag->r.currentOrigin, attacker->r.currentOrigin))) &&
	   attacker->client->sess.team != targ->client->sess.team){
		// we defended the base flag
		addscore(attacker, targ->r.currentOrigin, CTF_FLAG_DEFENSE_BONUS);
		attacker->client->pers.teamstate.basedefense++;

		attacker->client->ps.persistant[PERS_DEFEND_COUNT]++;
		// add the sprite over the player's head
		attacker->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP);
		attacker->client->ps.eFlags |= EF_AWARD_DEFEND;
		attacker->client->rewardtime = level.time + REWARD_SPRITE_TIME;

		return;
	}

	if(carrier && carrier != attacker){
		vecsub(targ->r.currentOrigin, carrier->r.currentOrigin, v1);
		vecsub(attacker->r.currentOrigin, carrier->r.currentOrigin, v1);

		if(((veclen(v1) < CTF_ATTACKER_PROTECT_RADIUS &&
		     trap_InPVS(carrier->r.currentOrigin, targ->r.currentOrigin)) ||
		    (veclen(v2) < CTF_ATTACKER_PROTECT_RADIUS &&
		     trap_InPVS(carrier->r.currentOrigin, attacker->r.currentOrigin))) &&
		   attacker->client->sess.team != targ->client->sess.team){
			addscore(attacker, targ->r.currentOrigin, CTF_CARRIER_PROTECT_BONUS);
			attacker->client->pers.teamstate.carrierdefense++;

			attacker->client->ps.persistant[PERS_DEFEND_COUNT]++;
			// add the sprite over the player's head
			attacker->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP);
			attacker->client->ps.eFlags |= EF_AWARD_DEFEND;
			attacker->client->rewardtime = level.time + REWARD_SPRITE_TIME;

			return;
		}
	}
}

/*
================
teamcheckhurtcarrier

Check to see if attacker hurt the flag carrier.  Needed when handing out bonuses for assistance to flag
carrier defense.
================
*/
void
teamcheckhurtcarrier(ent_t *targ, ent_t *attacker)
{
	int flag_pw;

	if(!targ->client || !attacker->client)
		return;

	if(targ->client->sess.team == TEAM_RED)
		flag_pw = PW_BLUEFLAG;
	else
		flag_pw = PW_REDFLAG;

	// flags
	if(targ->client->ps.powerups[flag_pw] &&
	   targ->client->sess.team != attacker->client->sess.team)
		attacker->client->pers.teamstate.lasthurtcarrier = level.time;

	// skulls
	if(targ->client->ps.generic1 &&
	   targ->client->sess.team != attacker->client->sess.team)
		attacker->client->pers.teamstate.lasthurtcarrier = level.time;
}

ent_t *
Team_ResetFlag(int team)
{
	char *c;
	ent_t *ent, *rent = nil;

	switch(team){
	case TEAM_RED:
		c = "team_CTF_redflag";
		break;
	case TEAM_BLUE:
		c = "team_CTF_blueflag";
		break;
	case TEAM_FREE:
		c = "team_CTF_neutralflag";
		break;
	default:
		return nil;
	}

	ent = nil;
	while((ent = findent(ent, FOFS(classname), c)) != nil){
		if(ent->flags & FL_DROPPED_ITEM)
			entfree(ent);
		else{
			rent = ent;
			itemrespawn(ent);
		}
	}

	Team_SetFlagStatus(team, FLAG_ATBASE);

	return rent;
}

void
Team_ResetFlags(void)
{
	if(g_gametype.integer == GT_CTF){
		Team_ResetFlag(TEAM_RED);
		Team_ResetFlag(TEAM_BLUE);
	}
}

void
Team_ReturnFlagSound(ent_t *ent, int team)
{
	ent_t *te;

	if(ent == nil){
		gprintf("Warning:  nil passed to Team_ReturnFlagSound\n");
		return;
	}

	te = enttemp(ent->s.pos.trBase, EV_GLOBAL_TEAM_SOUND);
	if(team == TEAM_BLUE)
		te->s.eventParm = GTS_RED_RETURN;
	else
		te->s.eventParm = GTS_BLUE_RETURN;
	te->r.svFlags |= SVF_BROADCAST;
}

void
Team_TakeFlagSound(ent_t *ent, int team)
{
	ent_t *te;

	if(ent == nil){
		gprintf("Warning:  nil passed to Team_TakeFlagSound\n");
		return;
	}

	// only play sound when the flag was at the base
	// or not picked up the last 10 seconds
	switch(team){
	case TEAM_RED:
		if(teamgame.blueStatus != FLAG_ATBASE)
			if(teamgame.blueTakenTime > level.time - 10000)
				return;
		teamgame.blueTakenTime = level.time;
		break;

	case TEAM_BLUE:	// CTF
		if(teamgame.redStatus != FLAG_ATBASE)
			if(teamgame.redTakenTime > level.time - 10000)
				return;
		teamgame.redTakenTime = level.time;
		break;
	}

	te = enttemp(ent->s.pos.trBase, EV_GLOBAL_TEAM_SOUND);
	if(team == TEAM_BLUE)
		te->s.eventParm = GTS_RED_TAKEN;
	else
		te->s.eventParm = GTS_BLUE_TAKEN;
	te->r.svFlags |= SVF_BROADCAST;
}

void
Team_CaptureFlagSound(ent_t *ent, int team)
{
	ent_t *te;

	if(ent == nil){
		gprintf("Warning:  nil passed to Team_CaptureFlagSound\n");
		return;
	}

	te = enttemp(ent->s.pos.trBase, EV_GLOBAL_TEAM_SOUND);
	if(team == TEAM_BLUE)
		te->s.eventParm = GTS_BLUE_CAPTURE;
	else
		te->s.eventParm = GTS_RED_CAPTURE;
	te->r.svFlags |= SVF_BROADCAST;
}

void
teamreturnflag(int team)
{
	Team_ReturnFlagSound(Team_ResetFlag(team), team);
	if(team == TEAM_FREE)
		PrintMsg(nil, "The flag has returned!\n");
	else
		PrintMsg(nil, "The %s flag has returned!\n", teamname(team));
}

void
teamfreeent(ent_t *ent)
{
	if(ent->item->tag == PW_REDFLAG)
		teamreturnflag(TEAM_RED);
	else if(ent->item->tag == PW_BLUEFLAG)
		teamreturnflag(TEAM_BLUE);
	else if(ent->item->tag == PW_NEUTRALFLAG)
		teamreturnflag(TEAM_FREE);
}

/*
==============
teamdroppedflag_think

Automatically set in Launch_Item if the item is one of the flags

Flags are unique in that if they are dropped, the base flag must be respawned when they time out
==============
*/
void
teamdroppedflag_think(ent_t *ent)
{
	int team = TEAM_FREE;

	if(ent->item->tag == PW_REDFLAG)
		team = TEAM_RED;
	else if(ent->item->tag == PW_BLUEFLAG)
		team = TEAM_BLUE;
	else if(ent->item->tag == PW_NEUTRALFLAG)
		team = TEAM_FREE;

	Team_ReturnFlagSound(Team_ResetFlag(team), team);
	// Reset Flag will delete this entity
}

/*
==============
teamdroppedflag_think
==============
*/
int
Team_TouchOurFlag(ent_t *ent, ent_t *other, int team)
{
	int i;
	ent_t *player;
	gclient_t *cl = other->client;
	int enemy_flag;

	if(cl->sess.team == TEAM_RED)
		enemy_flag = PW_BLUEFLAG;
	else
		enemy_flag = PW_REDFLAG;

	if(ent->flags & FL_DROPPED_ITEM){
		// hey, it's not home.  return it by teleporting it back
		PrintMsg(nil, "%s" S_COLOR_WHITE " returned the %s flag!\n",
			 cl->pers.netname, teamname(team));
		addscore(other, ent->r.currentOrigin, CTF_RECOVERY_BONUS);
		other->client->pers.teamstate.flagrecovery++;
		other->client->pers.teamstate.lastreturnedflag = level.time;
		//ResetFlag will remove this entity!  We must return zero
		Team_ReturnFlagSound(Team_ResetFlag(team), team);
		return 0;
	}

	// the flag is at home base.  if the player has the enemy
	// flag, he's just won!
	if(!cl->ps.powerups[enemy_flag])
		return 0;	// We don't have the flag
	PrintMsg(nil, "%s" S_COLOR_WHITE " captured the %s flag!\n", cl->pers.netname, teamname(otherteam(team)));

	cl->ps.powerups[enemy_flag] = 0;

	teamgame.last_flag_capture = level.time;
	teamgame.last_capture_team = team;

	// Increase the team's score
	addteamscore(ent->s.pos.trBase, other->client->sess.team, 1);
	Team_ForceGesture(other->client->sess.team);

	other->client->pers.teamstate.captures++;
	// add the sprite over the player's head
	other->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP);
	other->client->ps.eFlags |= EF_AWARD_CAP;
	other->client->rewardtime = level.time + REWARD_SPRITE_TIME;
	other->client->ps.persistant[PERS_CAPTURES]++;

	// other gets another 10 frag bonus
	addscore(other, ent->r.currentOrigin, CTF_CAPTURE_BONUS);

	Team_CaptureFlagSound(ent, team);

	// Ok, let's do the player loop, hand out the bonuses
	for(i = 0; i < g_maxclients.integer; i++){
		player = &g_entities[i];

		// also make sure we don't award assist bonuses to the flag carrier himself.
		if(!player->inuse || player == other)
			continue;

		if(player->client->sess.team !=
		   cl->sess.team)
			player->client->pers.teamstate.lasthurtcarrier = -5;
		else if(player->client->sess.team ==
			cl->sess.team){
			// award extra points for capture assists
			if(player->client->pers.teamstate.lastreturnedflag +
			   CTF_RETURN_FLAG_ASSIST_TIMEOUT > level.time){
				addscore(player, ent->r.currentOrigin, CTF_RETURN_FLAG_ASSIST_BONUS);
				other->client->pers.teamstate.assists++;

				player->client->ps.persistant[PERS_ASSIST_COUNT]++;
				// add the sprite over the player's head
				player->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP);
				player->client->ps.eFlags |= EF_AWARD_ASSIST;
				player->client->rewardtime = level.time + REWARD_SPRITE_TIME;
			}
			if(player->client->pers.teamstate.lastfraggedcarrier +
			   CTF_FRAG_CARRIER_ASSIST_TIMEOUT > level.time){
				addscore(player, ent->r.currentOrigin, CTF_FRAG_CARRIER_ASSIST_BONUS);
				other->client->pers.teamstate.assists++;
				player->client->ps.persistant[PERS_ASSIST_COUNT]++;
				// add the sprite over the player's head
				player->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP);
				player->client->ps.eFlags |= EF_AWARD_ASSIST;
				player->client->rewardtime = level.time + REWARD_SPRITE_TIME;
			}
		}
	}
	Team_ResetFlags();

	calcranks();

	return 0;	// Do not respawn this automatically
}

int
Team_TouchEnemyFlag(ent_t *ent, ent_t *other, int team)
{
	gclient_t *cl = other->client;

	PrintMsg(nil, "%s" S_COLOR_WHITE " got the %s flag!\n",
		 other->client->pers.netname, teamname(team));

	if(team == TEAM_RED)
		cl->ps.powerups[PW_REDFLAG] = INT_MAX;		// flags never expire
	else
		cl->ps.powerups[PW_BLUEFLAG] = INT_MAX;		// flags never expire

	Team_SetFlagStatus(team, FLAG_TAKEN);
	cl->pers.teamstate.flagsince = level.time;
	Team_TakeFlagSound(ent, team);

	return -1;	// Do not respawn this automatically, but do delete it if it was FL_DROPPED
}

int
pickupteam(ent_t *ent, ent_t *other)
{
	int team;
	gclient_t *cl = other->client;

	// figure out what team this flag is
	if(strcmp(ent->classname, "team_CTF_redflag") == 0)
		team = TEAM_RED;
	else if(strcmp(ent->classname, "team_CTF_blueflag") == 0)
		team = TEAM_BLUE;

	else{
		PrintMsg(other, "Don't know what team the flag is on.\n");
		return 0;
	}
	// GT_CTF
	if(team == cl->sess.team)
		return Team_TouchOurFlag(ent, other, team);
	return Team_TouchEnemyFlag(ent, other, team);
}

/*
===========
teamgetlocation

Report a location for the player. Uses placed nearby target_location entities
============
*/
ent_t *
teamgetlocation(ent_t *ent)
{
	ent_t *eloc, *best;
	float bestlen, len;
	vec3_t origin;

	best = nil;
	bestlen = 3*8192.0*8192.0;

	veccopy(ent->r.currentOrigin, origin);

	for(eloc = level.lochead; eloc; eloc = eloc->nexttrain){
		len = (origin[0] - eloc->r.currentOrigin[0]) * (origin[0] - eloc->r.currentOrigin[0])
		      + (origin[1] - eloc->r.currentOrigin[1]) * (origin[1] - eloc->r.currentOrigin[1])
		      + (origin[2] - eloc->r.currentOrigin[2]) * (origin[2] - eloc->r.currentOrigin[2]);

		if(len > bestlen)
			continue;

		if(!trap_InPVS(origin, eloc->r.currentOrigin))
			continue;

		bestlen = len;
		best = eloc;
	}

	return best;
}

/*
===========
teamgetlocation

Report a location for the player. Uses placed nearby target_location entities
============
*/
qboolean
teamgetlocationmsg(ent_t *ent, char *loc, int loclen)
{
	ent_t *best;

	best = teamgetlocation(ent);

	if(!best)
		return qfalse;

	if(best->count){
		if(best->count < 0)
			best->count = 0;
		if(best->count > 7)
			best->count = 7;
		Com_sprintf(loc, loclen, "%c%c%s" S_COLOR_WHITE, Q_COLOR_ESCAPE, best->count + '0', best->message);
	}else
		Com_sprintf(loc, loclen, "%s", best->message);

	return qtrue;
}

/*---------------------------------------------------------------------------*/

/*
================
SelectRandomTeamSpawnPoint

go to a random point that doesn't telefrag
================
*/
#define MAX_TEAM_SPAWN_POINTS 32
ent_t *
SelectRandomTeamSpawnPoint(int teamstate, teamnum_t team)
{
	ent_t *spot;
	int count;
	int selection;
	ent_t *spots[MAX_TEAM_SPAWN_POINTS];
	char *classname;

	if(teamstate == TEAM_BEGIN){
		if(team == TEAM_RED)
			classname = "team_CTF_redplayer";
		else if(team == TEAM_BLUE)
			classname = "team_CTF_blueplayer";
		else
			return nil;
	}else{
		if(team == TEAM_RED)
			classname = "team_CTF_redspawn";
		else if(team == TEAM_BLUE)
			classname = "team_CTF_bluespawn";
		else
			return nil;
	}
	count = 0;

	spot = nil;

	while((spot = findent(spot, FOFS(classname), classname)) != nil){
		if(possibletelefrag(spot))
			continue;
		spots[count] = spot;
		if(++count == MAX_TEAM_SPAWN_POINTS)
			break;
	}

	if(!count)	// no spots that won't telefrag
		return findent(nil, FOFS(classname), classname);

	selection = rand() % count;
	return spots[selection];
}

/*
===========
selctfspawnpoint

============
*/
ent_t *
selctfspawnpoint(teamnum_t team, int teamstate, vec3_t origin, vec3_t angles, qboolean isbot)
{
	ent_t *spot;

	spot = SelectRandomTeamSpawnPoint(teamstate, team);

	if(!spot)
		return selectspawnpoint(vec3_origin, origin, angles, isbot);

	veccopy(spot->s.origin, origin);
	origin[2] += 9;
	veccopy(spot->s.angles, angles);

	return spot;
}

/*---------------------------------------------------------------------------*/

static int QDECL
SortClients(const void *a, const void *b)
{
	return *(int*)a - *(int*)b;
}

/*
==================
TeamplayLocationsMessage

Format:
        clientNum location health armor weapon powerups

==================
*/
void
teamplayinfomsg(ent_t *ent)
{
	char entry[1024];
	char string[8192];
	int stringlength;
	int i, j;
	ent_t *player;
	int cnt;
	int h, a;
	int clients[TEAM_MAXOVERLAY];
	int team;

	if(!ent->client->pers.teaminfo)
		return;

	// send team info to spectator for team of followed client
	if(ent->client->sess.team == TEAM_SPECTATOR){
		if(ent->client->sess.specstate != SPECTATOR_FOLLOW
		   || ent->client->sess.specclient < 0)
			return;
		team = g_entities[ent->client->sess.specclient].client->sess.team;
	}else
		team = ent->client->sess.team;

	if(team != TEAM_RED && team != TEAM_BLUE)
		return;

	// figure out what client should be on the display
	// we are limited to 8, but we want to use the top eight players
	// but in client order (so they don't keep changing position on the overlay)
	for(i = 0, cnt = 0; i < g_maxclients.integer && cnt < TEAM_MAXOVERLAY; i++){
		player = g_entities + level.sortedclients[i];
		if(player->inuse && player->client->sess.team == team)
			clients[cnt++] = level.sortedclients[i];
	}

	// We have the top eight players, sort them by clientNum
	qsort(clients, cnt, sizeof(clients[0]), SortClients);

	// send the latest information on all clients
	string[0] = 0;
	stringlength = 0;

	for(i = 0, cnt = 0; i < g_maxclients.integer && cnt < TEAM_MAXOVERLAY; i++){
		player = g_entities + i;
		if(player->inuse && player->client->sess.team == team){
			h = player->client->ps.stats[STAT_HEALTH];
			a = player->client->ps.stats[STAT_ARMOR];
			if(h < 0)
				h = 0;
			if(a < 0)
				a = 0;

			Com_sprintf(entry, sizeof(entry),
				    " %i %i %i %i %i %i",
//				level.sortedclients[i], player->client->pers.teamstate.location, h, a,
				    i, player->client->pers.teamstate.location, h, a,
				    player->client->ps.weapon, player->s.powerups);
			j = strlen(entry);
			if(stringlength + j >= sizeof(string))
				break;
			strcpy(string + stringlength, entry);
			stringlength += j;
			cnt++;
		}
	}

	trap_SendServerCommand(ent-g_entities, va("tinfo %i %s", cnt, string));
}

void
checkteamstatus(void)
{
	int i;
	ent_t *loc, *ent;

	if(level.time - level.lastteamlocationtime > TEAM_LOCATION_UPDATE_TIME){
		level.lastteamlocationtime = level.time;

		for(i = 0; i < g_maxclients.integer; i++){
			ent = g_entities + i;

			if(ent->client->pers.connected != CON_CONNECTED)
				continue;

			if(ent->inuse && (ent->client->sess.team == TEAM_RED ||      ent->client->sess.team == TEAM_BLUE)){
				loc = teamgetlocation(ent);
				if(loc)
					ent->client->pers.teamstate.location = loc->health;
				else
					ent->client->pers.teamstate.location = 0;
			}
		}

		for(i = 0; i < g_maxclients.integer; i++){
			ent = g_entities + i;

			if(ent->client->pers.connected != CON_CONNECTED)
				continue;

			if(ent->inuse)
				teamplayinfomsg(ent);
		}
	}
}

/*-----------------------------------------------------------------*/

/*QUAKED team_CTF_redplayer (1 0 0) (-16 -16 -16) (16 16 32)
Only in CTF games.  Red players spawn here at game start.
*/
void
SP_team_CTF_redplayer(ent_t *ent)
{
}

/*QUAKED team_CTF_blueplayer (0 0 1) (-16 -16 -16) (16 16 32)
Only in CTF games.  Blue players spawn here at game start.
*/
void
SP_team_CTF_blueplayer(ent_t *ent)
{
}

/*QUAKED team_CTF_redspawn (1 0 0) (-16 -16 -24) (16 16 32)
potential spawning position for red team in CTF games.
Targets will be fired when someone spawns in on them.
*/
void
SP_team_CTF_redspawn(ent_t *ent)
{
}

/*QUAKED team_CTF_bluespawn (0 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for blue team in CTF games.
Targets will be fired when someone spawns in on them.
*/
void
SP_team_CTF_bluespawn(ent_t *ent)
{
}

