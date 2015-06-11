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
//
// bg_misc.c -- both games misc functions, all completely stateless

#include "../qcommon/q_shared.h"
#include "bg_public.h"

/*QUAKED item_***** ( 0 0 0 ) (-16 -16 -16) (16 16 16) suspended
DO NOT USE THIS CLASS, IT JUST HOLDS GENERAL INFORMATION.
The suspended flag will allow items to hang in the air, otherwise they are dropped to the next surface.

If an item is the target of another entity, it will not spawn in until fired.

An item fires all of its targets when it is picked up.  If the toucher can't carry it, the targets won't be fired.

"notfree" if set to 1, don't spawn in free for all games
"notteam" if set to 1, don't spawn in team games
"notsingle" if set to 1, don't spawn in single player games
"wait"	override the default wait before respawning.  -1 = never respawn automatically, which can be used with targeted spawning.
"random" random number of plus or minus seconds varied from the respawn time
"count" override quantity or duration on most items.
*/

gitem_t	bg_itemlist[] = {
	{
		NULL,
		NULL,
		{
			NULL,
			NULL,
			NULL, NULL
		} ,
		/* icon */		NULL,
		/* pickup */	NULL,
		0,
		0,
		0,
		/* precache */ "",
		/* sounds */ ""
	},	// leave index 0 alone

	//
	// collectables
	//

	// QUAKED item_token (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	{
		"item_token",
		"sound/items/token",
		{
			"models/collectables/token",
			NULL, NULL, NULL
		},
		/* icon */		"icons/token",
		/* pickup */	"Token",
		1,
		IT_HEALTH,
		0,
		/* precache */ "",
		/* sounds */ ""
	},

	// QUAKED item_token_large (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	{
		"item_token_large",
		"sound/items/token",
		{
			"models/collectables/token",
			NULL, NULL, NULL
		},
		/* icon */		"icons/token_large",
		/* pickup */	"Big Token",
		25,
		IT_HEALTH,
		0,
		/* precache */ "",
		/* sounds */ ""
	},

	// QUAKED item_life (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	{
		"item_life",
		"sound/items/life",
		{
			"models/collectables/life",
			NULL, NULL, NULL
		},
		/* icon */		"icons/life",
		/* pickup */	"Extra Life",
		LIFE2TOK(1),
		IT_HEALTH,
		0,
		/* precache */ "",
		/* sounds */ ""
	},

	// QUAKED item_key_jade (1 1 1) (-6 -6 -6) (6 6 6) suspended
	{
		"item_key_jade",
		"sound/items/key_jade",
		{
			"models/items/key_jade",
			NULL, NULL, NULL
		},
		"icons/key_jade",	// icon	
		"Jade Key",		// pickup
		1,
		IT_KEY,
		KEY_JADE,
		"",				// precache
		""				// sounds
	},

	// QUAKED item_key_ruby (1 1 1) (-6 -6 -6) (6 6 6) suspended
	{
		"item_key_ruby",
		"sound/items/key_ruby",
		{
			"models/items/key_ruby",
			NULL, NULL, NULL
		},
		"icons/key_ruby",	// icon	
		"Ruby Key",		// pickup
		1,
		IT_KEY,
		KEY_RUBY,
		"",				// precache
		""				// sounds
	},

	// QUAKED item_key_sapphire (1 1 1) (-6 -6 -6) (6 6 6) suspended
	{
		"item_key_sapphire",
		"sound/items/key_sapphire",
		{
			"models/items/key_sapphire",
			NULL, NULL, NULL
		},
		"icons/key_sapphire",	// icon	
		"Sapphire Key",		// pickup
		1,
		IT_KEY,
		KEY_SAPPHIRE,
		"",				// precache
		""				// sounds
	},

	//
	// WEAPONS
	//

	/*QUAKED weapon_gauntlet (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	*/
	{
		"weapon_gauntlet",
		"sound/misc/w_pkup.wav",
		{
			"models/weapons2/gauntlet/gauntlet.md3",
			NULL, NULL, NULL
		},
		/* icon */		"icons/iconw_gauntlet",
		/* pickup */	"Gauntlet",
		0,
		IT_WEAPON,
		WP_GAUNTLET,
		/* precache */ "",
		/* sounds */ ""
	},

	// end of list marker
	{NULL}
};

int bg_numItems = ARRAY_LEN(bg_itemlist) - 1;


/*
==============
BG_FindItemForPowerup
==============
*/
gitem_t	*BG_FindItemForPowerup(powerup_t pw)
{
	int		i;

	for(i = 0 ; i < bg_numItems ; i++){
		if((bg_itemlist[i].giType == IT_POWERUP ||
		        bg_itemlist[i].giType == IT_TEAM ||
		        bg_itemlist[i].giType == IT_PERSISTANT_POWERUP) &&
		        bg_itemlist[i].giTag == pw){
			return &bg_itemlist[i];
		}
	}

	return NULL;
}


/*
==============
BG_FindItemForHoldable
==============
*/
gitem_t	*BG_FindItemForHoldable(holdable_t pw)
{
	int		i;

	for(i = 0 ; i < bg_numItems ; i++){
		if(bg_itemlist[i].giType == IT_HOLDABLE && bg_itemlist[i].giTag == pw){
			return &bg_itemlist[i];
		}
	}

	Com_Error(ERR_DROP, "HoldableItem not found");

	return NULL;
}


/*
===============
BG_FindItemForWeapon

===============
*/
gitem_t	*BG_FindItemForWeapon(weapon_t weapon)
{
	gitem_t	*it;

	for(it = bg_itemlist + 1 ; it->classname ; it++){
		if(it->giType == IT_WEAPON && it->giTag == weapon){
			return it;
		}
	}

	Com_Error(ERR_DROP, "Couldn't find item for weapon %i", weapon);
	return NULL;
}

/*
===============
BG_FindItem

===============
*/
gitem_t	*BG_FindItem(const char *pickupName)
{
	gitem_t	*it;

	for(it = bg_itemlist + 1 ; it->classname ; it++){
		if(!Q_stricmp(it->pickup_name, pickupName))
			return it;
	}

	return NULL;
}

/*
============
BG_PlayerTouchesItem

Items can be picked up without actually touching their physical bounds to make
grabbing them easier
============
*/
qboolean	BG_PlayerTouchesItem(playerState_t *ps, entityState_t *item, int atTime)
{
	vec3_t		origin;

	BG_EvaluateTrajectory(&item->pos, atTime, origin);

	// we are ignoring ducked differences here
	if(ps->origin[0] - origin[0] > 44
	        || ps->origin[0] - origin[0] < -50
	        || ps->origin[1] - origin[1] > 36
	        || ps->origin[1] - origin[1] < -36
	        || ps->origin[2] - origin[2] > 36
	        || ps->origin[2] - origin[2] < -36){
		return qfalse;
	}

	return qtrue;
}



/*
================
BG_CanItemBeGrabbed

Returns false if the item should not be picked up.
This needs to be the same for client side prediction and server use.
================
*/
qboolean BG_CanItemBeGrabbed(int gametype, const entityState_t *ent, const playerState_t *ps)
{
	gitem_t	*item;
#ifdef MISSIONPACK
	int		upperBound;
#endif

	if(ent->modelindex < 1 || ent->modelindex >= bg_numItems){
		Com_Error(ERR_DROP, "BG_CanItemBeGrabbed: index out of range");
	}

	item = &bg_itemlist[ent->modelindex];

	switch(item->giType){
	case IT_WEAPON:
		return qtrue;	// weapons are always picked up

	case IT_AMMO:
		if(ps->ammo[ item->giTag ] >= 200){
			return qfalse;		// can't hold any more
		}
		return qtrue;

	case IT_ARMOR:
		if(ps->stats[STAT_ARMOR] >= ps->stats[STAT_MAX_ARMOR]){
			return qfalse;
		}
		return qtrue;

	case IT_HEALTH:
		return qtrue;

	case IT_LIFE:
		return qtrue;

	case IT_POWERUP:
		return qtrue;

	case IT_KEY:
		return qtrue;

#ifdef MISSIONPACK
	case IT_PERSISTANT_POWERUP:
		// can only hold one item at a time
		if(ps->stats[STAT_PERSISTANT_POWERUP]){
			return qfalse;
		}

		// check team only
		if((ent->generic1 & 2) && (ps->persistant[PERS_TEAM] != TEAM_RED)){
			return qfalse;
		}
		if((ent->generic1 & 4) && (ps->persistant[PERS_TEAM] != TEAM_BLUE)){
			return qfalse;
		}

		return qtrue;
#endif

	case IT_TEAM: // team items, such as flags
#ifdef MISSIONPACK
		if(gametype == GT_1FCTF){
			// neutral flag can always be picked up
			if(item->giTag == PW_NEUTRALFLAG){
				return qtrue;
			}
			if(ps->persistant[PERS_TEAM] == TEAM_RED){
				if(item->giTag == PW_BLUEFLAG  && ps->powerups[PW_NEUTRALFLAG]){
					return qtrue;
				}
			}else if(ps->persistant[PERS_TEAM] == TEAM_BLUE){
				if(item->giTag == PW_REDFLAG  && ps->powerups[PW_NEUTRALFLAG]){
					return qtrue;
				}
			}
		}
#endif
		if(gametype == GT_CTF){
			// ent->modelindex2 is non-zero on items if they are dropped
			// we need to know this because we can pick up our dropped flag (and return it)
			// but we can't pick up our flag at base
			if(ps->persistant[PERS_TEAM] == TEAM_RED){
				if(item->giTag == PW_BLUEFLAG ||
				        (item->giTag == PW_REDFLAG && ent->modelindex2) ||
				        (item->giTag == PW_REDFLAG && ps->powerups[PW_BLUEFLAG]))
					return qtrue;
			}else if(ps->persistant[PERS_TEAM] == TEAM_BLUE){
				if(item->giTag == PW_REDFLAG ||
				        (item->giTag == PW_BLUEFLAG && ent->modelindex2) ||
				        (item->giTag == PW_BLUEFLAG && ps->powerups[PW_REDFLAG]))
					return qtrue;
			}
		}

#ifdef MISSIONPACK
		if(gametype == GT_HARVESTER){
			return qtrue;
		}
#endif
		return qfalse;

	case IT_HOLDABLE:
		// can only hold one item at a time
		if(ps->stats[STAT_HOLDABLE_ITEM]){
			return qfalse;
		}
		return qtrue;

	case IT_BAD:
		Com_Error(ERR_DROP, "BG_CanItemBeGrabbed: IT_BAD");
	default:
#ifndef Q3_VM
#ifndef NDEBUG
		Com_Printf("BG_CanItemBeGrabbed: unknown enum %d\n", item->giType);
#endif
#endif
		break;
	}
	return qfalse;
}

//======================================================================

/*
================
BG_EvaluateTrajectory

================
*/
void BG_EvaluateTrajectory(const trajectory_t *tr, int atTime, vec3_t result)
{
	float		deltaTime;
	float		phase;

	switch(tr->trType){
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorCopy(tr->trBase, result);
		break;
	case TR_LINEAR:
		deltaTime = (atTime - tr->trTime) * 0.001;	// milliseconds to seconds
		VectorMA(tr->trBase, deltaTime, tr->trDelta, result);
		break;
	case TR_SINE:
		deltaTime = (atTime - tr->trTime) / (float) tr->trDuration;
		phase = sin(deltaTime * M_PI * 2);
		VectorMA(tr->trBase, phase, tr->trDelta, result);
		break;
	case TR_LINEAR_STOP:
		if(atTime > tr->trTime + tr->trDuration){
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = (atTime - tr->trTime) * 0.001;	// milliseconds to seconds
		if(deltaTime < 0){
			deltaTime = 0;
		}
		VectorMA(tr->trBase, deltaTime, tr->trDelta, result);
		break;
	case TR_GRAVITY:
		deltaTime = (atTime - tr->trTime) * 0.001;	// milliseconds to seconds
		VectorMA(tr->trBase, deltaTime, tr->trDelta, result);
		result[2] -= 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime;		// FIXME: local gravity...
		break;
	default:
		Com_Error(ERR_DROP, "BG_EvaluateTrajectory: unknown trType: %i", tr->trType);
		break;
	}
}

/*
================
BG_EvaluateTrajectoryDelta

For determining velocity at a given time
================
*/
void BG_EvaluateTrajectoryDelta(const trajectory_t *tr, int atTime, vec3_t result)
{
	float	deltaTime;
	float	phase;

	switch(tr->trType){
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorClear(result);
		break;
	case TR_LINEAR:
		VectorCopy(tr->trDelta, result);
		break;
	case TR_SINE:
		deltaTime = (atTime - tr->trTime) / (float) tr->trDuration;
		phase = cos(deltaTime * M_PI * 2);	// derivative of sin = cos
		phase *= 0.5;
		VectorScale(tr->trDelta, phase, result);
		break;
	case TR_LINEAR_STOP:
		if(atTime > tr->trTime + tr->trDuration){
			VectorClear(result);
			return;
		}
		VectorCopy(tr->trDelta, result);
		break;
	case TR_GRAVITY:
		deltaTime = (atTime - tr->trTime) * 0.001;	// milliseconds to seconds
		VectorCopy(tr->trDelta, result);
		result[2] -= DEFAULT_GRAVITY * deltaTime;		// FIXME: local gravity...
		break;
	default:
		Com_Error(ERR_DROP, "BG_EvaluateTrajectoryDelta: unknown trType: %i", tr->trType);
		break;
	}
}

char *eventnames[] = {
	"EV_NONE",

	"EV_FOOTSTEP",
	"EV_FOOTSTEP_METAL",
	"EV_FOOTSPLASH",
	"EV_FOOTWADE",
	"EV_SWIM",

	"EV_STEP_4",
	"EV_STEP_8",
	"EV_STEP_12",
	"EV_STEP_16",

	"EV_FALL_SHORT",
	"EV_FALL_MEDIUM",
	"EV_FALL_FAR",

	"EV_JUMP_PAD",			// boing sound at origin", jump sound on player
	"EV_SQUISH",			// player landed on something breakable

	"EV_JUMP",
	"EV_WATER_TOUCH",	// foot touches
	"EV_WATER_LEAVE",	// foot leaves
	"EV_WATER_UNDER",	// head touches
	"EV_WATER_CLEAR",	// head leaves

	"EV_ITEM_PICKUP",			// normal item pickups are predictable
	"EV_GLOBAL_ITEM_PICKUP",	// powerup / team sounds are broadcast to everyone

	"EV_SMASH_CRATE",
	"EV_SMASH_STRONG_CRATE",
	"EV_SMASH_CHECKPOINT_CRATE",

	"EV_NOAMMO",
	"EV_CHANGE_WEAPON",
	"EV_FIRE_WEAPON",

	"EV_USE_ITEM0",
	"EV_USE_ITEM1",
	"EV_USE_ITEM2",
	"EV_USE_ITEM3",
	"EV_USE_ITEM4",
	"EV_USE_ITEM5",
	"EV_USE_ITEM6",
	"EV_USE_ITEM7",
	"EV_USE_ITEM8",
	"EV_USE_ITEM9",
	"EV_USE_ITEM10",
	"EV_USE_ITEM11",
	"EV_USE_ITEM12",
	"EV_USE_ITEM13",
	"EV_USE_ITEM14",
	"EV_USE_ITEM15",

	"EV_ITEM_RESPAWN",
	"EV_ITEM_POP",
	"EV_PLAYER_TELEPORT_IN",
	"EV_PLAYER_TELEPORT_OUT",

	"EV_GRENADE_BOUNCE",		// eventParm will be the soundindex

	"EV_GENERAL_SOUND",
	"EV_GLOBAL_SOUND",		// no attenuation
	"EV_GLOBAL_TEAM_SOUND",

	"EV_BULLET_HIT_FLESH",
	"EV_BULLET_HIT_WALL",

	"EV_MISSILE_HIT",
	"EV_MISSILE_MISS",
	"EV_MISSILE_MISS_METAL",
	"EV_RAILTRAIL",
	"EV_SHOTGUN",
	"EV_BULLET",				// otherEntity is the shooter

	"EV_PAIN",
	"EV_DEATH1",
	"EV_DEATH2",
	"EV_DEATH3",
	"EV_OBITUARY",

	"EV_POWERUP_QUAD",
	"EV_POWERUP_BATTLESUIT",
	"EV_POWERUP_REGEN",

	"EV_GIB_PLAYER",			// gib a previously living player
	"EV_SCOREPLUM",			// score plum

//#ifdef MISSIONPACK
	"EV_PROXIMITY_MINE_STICK",
	"EV_PROXIMITY_MINE_TRIGGER",
	"EV_KAMIKAZE",			// kamikaze explodes
	"EV_OBELISKEXPLODE",		// obelisk explodes
	"EV_OBELISKPAIN",		// obelisk pain
	"EV_INVUL_IMPACT",		// invulnerability sphere impact
	"EV_JUICED",				// invulnerability juiced effect
	"EV_LIGHTNINGBOLT",		// lightning bolt bounced of invulnerability sphere
//#endif

	"EV_DEBUG_LINE",
	"EV_STOPLOOPINGSOUND",
	"EV_TAUNT",
	"EV_TAUNT_YES",
	"EV_TAUNT_NO",
	"EV_TAUNT_FOLLOWME",
	"EV_TAUNT_GETFLAG",
	"EV_TAUNT_GUARDBASE",
	"EV_TAUNT_PATROL"

};

/*
===============
BG_AddPredictableEventToPlayerstate

Handles the sequence numbers
===============
*/

void	trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize);

void BG_AddPredictableEventToPlayerstate(int newEvent, int eventParm, playerState_t *ps)
{

#ifdef _DEBUG
	{
		char buf[256];
		trap_Cvar_VariableStringBuffer("showevents", buf, sizeof(buf));
		if(atof(buf) != 0)
		{
#ifdef QAGAME
			Com_Printf(" game event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount/*ps->commandTime*/, ps->eventSequence, eventnames[newEvent], eventParm);
#else
			Com_Printf("Cgame event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount/*ps->commandTime*/, ps->eventSequence, eventnames[newEvent], eventParm);
#endif
		}
	}
#endif
	ps->events[ps->eventSequence & (MAX_PS_EVENTS-1)] = newEvent;
	ps->eventParms[ps->eventSequence & (MAX_PS_EVENTS-1)] = eventParm;
	ps->eventSequence++;
}

/*
========================
BG_TouchJumpPad
========================
*/
void BG_TouchJumpPad(playerState_t *ps, entityState_t *jumppad)
{
	vec3_t	angles;
	float p;
	int effectNum;

	// spectators don't use jump pads
	if(ps->pm_type != PM_NORMAL){
		return;
	}

	// flying characters don't hit bounce pads
	if(ps->powerups[PW_FLIGHT]){
		return;
	}

	// if we didn't hit this same jumppad the previous frame
	// then don't play the event sound again if we are in a fat trigger
	if(ps->jumppad_ent != jumppad->number){

		vectoangles(jumppad->origin2, angles);
		p = fabs(AngleNormalize180(angles[PITCH]));
		if(p < 45){
			effectNum = 0;
		}else{
			effectNum = 1;
		}
		BG_AddPredictableEventToPlayerstate(EV_JUMP_PAD, effectNum, ps);
	}
	// remember hitting this jumppad this frame
	ps->jumppad_ent = jumppad->number;
	ps->jumppad_frame = ps->pmove_framecount;
	// give the player the velocity from the jumppad
	VectorCopy(jumppad->origin2, ps->velocity);
}

// Player landed on something after jumping.
// A squishable entity is a breakable or a regular enemy.
void BG_Squish(playerState_t *ps, entityState_t *squishable)
{
	if(ps->pm_type != PM_NORMAL)
		return;	// spectators don't break things
	if(ps->powerups[PW_FLIGHT])
		return;	// flying characters don't squish

	if(ps->jumppad_ent != squishable->number)
		BG_AddPredictableEventToPlayerstate(EV_SQUISH, 0, ps);
	// remember hitting this squishable this frame (recycle jumppad fields)
	ps->jumppad_ent = squishable->number;
	ps->jumppad_frame = ps->pmove_framecount;
	// bounce
	ps->velocity[2] = 360;
}

/*
========================
BG_PlayerStateToEntityState

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityState(playerState_t *ps, entityState_t *s, qboolean snap)
{
	int		i;

	if(ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR){
		s->eType = ET_INVISIBLE;
	}else if(ps->stats[STAT_HEALTH] <= GIB_HEALTH){
		s->eType = ET_INVISIBLE;
	}else{
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_INTERPOLATE;
	VectorCopy(ps->origin, s->pos.trBase);
	if(snap){
		SnapVector(s->pos.trBase);
	}
	// set the trDelta for flag direction
	VectorCopy(ps->velocity, s->pos.trDelta);

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy(ps->viewangles, s->apos.trBase);
	if(snap){
		SnapVector(s->apos.trBase);
	}

	s->angles2[YAW] = ps->movementDir;
	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->clientNum = ps->clientNum;		// ET_PLAYER looks here instead of at number
	// so corpses can also reference the proper config
	s->eFlags = ps->eFlags;
	if(ps->stats[STAT_HEALTH] <= 0){
		s->eFlags |= EF_DEAD;
	}else{
		s->eFlags &= ~EF_DEAD;
	}

	if(ps->externalEvent){
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	}else if(ps->entityEventSequence < ps->eventSequence){
		int		seq;

		if(ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS){
			ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
		}
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		s->event = ps->events[ seq ] | ((ps->entityEventSequence & 3) << 8);
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}

	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;
	s->headEntityNum = ps->headEntityNum;

	s->powerups = 0;
	for(i = 0 ; i < MAX_POWERUPS ; i++){
		if(ps->powerups[ i ]){
			s->powerups |= 1 << i;
		}
	}

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;
}

/*
========================
BG_PlayerStateToEntityStateExtraPolate

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityStateExtraPolate(playerState_t *ps, entityState_t *s, int time, qboolean snap)
{
	int		i;

	if(ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR){
		s->eType = ET_INVISIBLE;
	}else if(ps->stats[STAT_HEALTH] <= GIB_HEALTH){
		s->eType = ET_INVISIBLE;
	}else{
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_LINEAR_STOP;
	VectorCopy(ps->origin, s->pos.trBase);
	if(snap){
		SnapVector(s->pos.trBase);
	}
	// set the trDelta for flag direction and linear prediction
	VectorCopy(ps->velocity, s->pos.trDelta);
	// set the time for linear prediction
	s->pos.trTime = time;
	// set maximum extra polation time
	s->pos.trDuration = 50; // 1000 / sv_fps (default = 20)

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy(ps->viewangles, s->apos.trBase);
	if(snap){
		SnapVector(s->apos.trBase);
	}

	s->angles2[YAW] = ps->movementDir;
	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->clientNum = ps->clientNum;		// ET_PLAYER looks here instead of at number
	// so corpses can also reference the proper config
	s->eFlags = ps->eFlags;
	if(ps->stats[STAT_HEALTH] <= 0){
		s->eFlags |= EF_DEAD;
	}else{
		s->eFlags &= ~EF_DEAD;
	}

	if(ps->externalEvent){
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	}else if(ps->entityEventSequence < ps->eventSequence){
		int		seq;

		if(ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS){
			ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
		}
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		s->event = ps->events[ seq ] | ((ps->entityEventSequence & 3) << 8);
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}

	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;
	s->headEntityNum = ps->headEntityNum;

	s->powerups = 0;
	for(i = 0 ; i < MAX_POWERUPS ; i++){
		if(ps->powerups[ i ]){
			s->powerups |= 1 << i;
		}
	}

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;
}
