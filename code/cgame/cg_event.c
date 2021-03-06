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
// Handle entity events at snapshot or playerstate transitions.

#include "cg_local.h"

/*
Also called by scoreboard drawing
*/
const char      *
placestr(int rank)
{
	static char str[64];
	char *s, *t;

	if(rank & RANK_TIED_FLAG){
		rank &= ~RANK_TIED_FLAG;
		t = "Tied for ";
	}else
		t = "";

	if(rank == 1)
		s = "1st";
	else if(rank == 2)
		s = "2nd";
	else if(rank == 3)
		s = "3rd";
	else if(rank == 11)
		s = "11th";
	else if(rank == 12)
		s = "12th";
	else if(rank == 13)
		s = "13th";
	else if(rank % 10 == 1)
		s = va("%ist", rank);
	else if(rank % 10 == 2)
		s = va("%ind", rank);
	else if(rank % 10 == 3)
		s = va("%ird", rank);
	else
		s = va("%ith", rank);

	Com_sprintf(str, sizeof(str), "%s%s", t, s);
	return str;
}

static void
obituary(entityState_t *ent)
{
	int mod;
	int target, attacker;
	char *message;
	char *message2;
	const char *targetInfo;
	const char *attackerInfo;
	char targetName[32];
	char attackerName[32];
	gender_t gender;
	clientinfo_t *ci;

	target = ent->otherEntityNum;
	attacker = ent->otherEntityNum2;
	mod = ent->eventParm;

	if(target < 0 || target >= MAX_CLIENTS)
		cgerrorf("obituary: target out of range");
	ci = &cgs.clientinfo[target];

	if(attacker < 0 || attacker >= MAX_CLIENTS){
		attacker = ENTITYNUM_WORLD;
		attackerInfo = nil;
	}else
		attackerInfo = getconfigstr(CS_PLAYERS + attacker);

	targetInfo = getconfigstr(CS_PLAYERS + target);
	if(!targetInfo)
		return;
	Q_strncpyz(targetName, Info_ValueForKey(targetInfo, "n"), sizeof(targetName) - 2);
	strcat(targetName, S_COLOR_WHITE);

	message2 = "";

	// check for single client messages

	switch(mod){
	case MOD_SUICIDE:
		message = "suicides";
		break;
	case MOD_FALLING:
		message = "cratered";
		break;
	case MOD_CRUSH:
		message = "was squished";
		break;
	case MOD_WATER:
		message = "sank like a rock";
		break;
	case MOD_SLIME:
		message = "melted";
		break;
	case MOD_LAVA:
		message = "does a back flip into the lava";
		break;
	case MOD_TARGET_LASER:
		message = "saw the light";
		break;
	case MOD_TRIGGER_HURT:
		message = "was in the wrong place";
		break;
	case MOD_BOLT:
		message = "took a bolt";
		break;
	case MOD_TNT:
		message = "was blown up with TNT";
		break;
	default:
		message = nil;
		break;
	}

	if(attacker == target){
		gender = ci->gender;
		switch(mod){
		case MOD_GRENADE_SPLASH:
			if(gender == GENDER_FEMALE)
				message = "tripped on her own grenade";
			else if(gender == GENDER_NEUTER)
				message = "tripped on its own grenade";
			else
				message = "tripped on his own grenade";
			break;
		case MOD_ROCKET_SPLASH:
			if(gender == GENDER_FEMALE)
				message = "blew herself up";
			else if(gender == GENDER_NEUTER)
				message = "blew itself up";
			else
				message = "blew himself up";
			break;
		case MOD_PLASMA_SPLASH:
			if(gender == GENDER_FEMALE)
				message = "melted herself";
			else if(gender == GENDER_NEUTER)
				message = "melted itself";
			else
				message = "melted himself";
			break;
		case MOD_BFG_SPLASH:
			message = "should have used a smaller gun";
			break;
		default:
			if(gender == GENDER_FEMALE)
				message = "killed herself";
			else if(gender == GENDER_NEUTER)
				message = "killed itself";
			else
				message = "killed himself";
			break;
		}
	}

	if(message){
		cgprintf("%s %s.\n", targetName, message);
		return;
	}

	// check for kill messages from the current clientNum
	if(attacker == cg.snap->ps.clientNum){
		char *s;

		if(cgs.gametype < GT_TEAM)
			s = va("You fragged %s\n%s place with %i", targetName,
			       placestr(cg.snap->ps.persistant[PERS_RANK] + 1),
			       cg.snap->ps.persistant[PERS_SCORE]);
		else
			s = va("You fragged %s", targetName);

		centerprint(s, SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);

		// print the text message as well
	}

	// check for double client messages
	if(!attackerInfo){
		attacker = ENTITYNUM_WORLD;
		strcpy(attackerName, "noname");
	}else{
		Q_strncpyz(attackerName, Info_ValueForKey(attackerInfo, "n"), sizeof(attackerName) - 2);
		strcat(attackerName, S_COLOR_WHITE);
		// check for kill messages about the current clientNum
		if(target == cg.snap->ps.clientNum)
			Q_strncpyz(cg.killername, attackerName, sizeof(cg.killername));
	}

	if(attacker != ENTITYNUM_WORLD){
		switch(mod){
		case MOD_GRAPPLE:
			message = "was caught by";
			break;
		case MOD_GAUNTLET:
			message = "was pummeled by";
			break;
		case MOD_MACHINEGUN:
			message = "was machinegunned by";
			break;
		case MOD_SHOTGUN:
			message = "was gunned down by";
			break;
		case MOD_GRENADE:
			message = "ate";
			message2 = "'s grenade";
			break;
		case MOD_GRENADE_SPLASH:
			message = "was shredded by";
			message2 = "'s shrapnel";
			break;
		case MOD_ROCKET:
			message = "ate";
			message2 = "'s rocket";
			break;
		case MOD_ROCKET_SPLASH:
			message = "almost dodged";
			message2 = "'s rocket";
			break;
		case MOD_PLASMA:
			message = "was melted by";
			message2 = "'s plasmagun";
			break;
		case MOD_PLASMA_SPLASH:
			message = "was melted by";
			message2 = "'s plasmagun";
			break;
		case MOD_RAILGUN:
			message = "was railed by";
			break;
		case MOD_LIGHTNING:
			message = "was electrocuted by";
			break;
		case MOD_BFG:
		case MOD_BFG_SPLASH:
			message = "was blasted by";
			message2 = "'s BFG";
			break;
		case MOD_TELEFRAG:
			message = "tried to invade";
			message2 = "'s personal space";
			break;
		default:
			message = "was killed by";
			break;
		}

		if(message){
			cgprintf("%s %s %s%s\n",
				  targetName, message, attackerName, message2);
			return;
		}
	}

	// we don't know what it was
	cgprintf("%s died.\n", targetName);
}

static void
useitem(cent_t *cent)
{
	// there are no holdables at the moment
}

/*
A new item was picked up this frame.
*/
static void
itempickup(int itemNum)
{
	cg.itempkup = itemNum;
	cg.itempkuptime = cg.time;
	cg.itempkupblendtime = cg.time;
	// see if it should be the grabbed weapon
	if(bg_itemlist[itemNum].type == IT_WEAPON)
		// select it immediately
		if(cg_autoswitch.integer && bg_itemlist[itemNum].tag != WP_MACHINEGUN){
			cg.weapseltime = cg.time;
			cg.weapsel = bg_itemlist[itemNum].tag;
		}
}

/*
Returns waterlevel for entity origin.
*/
int
waterlevel(cent_t *cent)
{
	vec3_t point;
	int contents, sample1, sample2, anim, waterlevel;

	// get waterlevel, accounting for ducking
	waterlevel = 0;
	veccpy(cent->lerporigin, point);
	point[2] += MINS_Z + 1;
	anim = cent->currstate.legsAnim & ~ANIM_TOGGLEBIT;

	if(anim == LEGS_WALKCR || anim == LEGS_IDLECR)
		point[2] += CROUCH_VIEWHEIGHT;
	else
		point[2] += DEFAULT_VIEWHEIGHT;

	contents = pointcontents(point, -1);

	if(contents & MASK_WATER){
		sample2 = point[2] - MINS_Z;
		sample1 = sample2 / 2;
		waterlevel = 1;
		point[2] = cent->lerporigin[2] + MINS_Z + sample1;
		contents = pointcontents(point, -1);

		if(contents & MASK_WATER){
			waterlevel = 2;
			point[2] = cent->lerporigin[2] + MINS_Z + sample2;
			contents = pointcontents(point, -1);

			if(contents & MASK_WATER)
				waterlevel = 3;
		}
	}

	return waterlevel;
}

/*
Also called by playerstate transition.
*/
void
painevent(cent_t *cent, int health)
{
	char *snd;

	// don't do more than two pain sounds a second
	if(cg.time - cent->pe.paintime < 500)
		return;

	if(health < 25)
		snd = "*pain25_1.wav";
	else if(health < 50)
		snd = "*pain50_1.wav";
	else if(health < 75)
		snd = "*pain75_1.wav";
	else
		snd = "*pain100_1.wav";
	// play a gurp sound instead of a normal pain sound
	if(waterlevel(cent) >= 1){
		if(rand()&1)
			trap_S_StartSound(nil, cent->currstate.number, CHAN_VOICE, customsound(cent->currstate.number, "sound/player/gurp1.wav"));
		else
			trap_S_StartSound(nil, cent->currstate.number, CHAN_VOICE, customsound(cent->currstate.number, "sound/player/gurp2.wav"));
	}else
		trap_S_StartSound(nil, cent->currstate.number, CHAN_VOICE, customsound(cent->currstate.number, snd));
	// save pain time for programitic twitch animation
	cent->pe.paintime = cg.time;
	cent->pe.paindir ^= 1;
}

/*
An entity has an event value
also called by chkpsevents
*/
#define DEBUGNAME(x) if(cg_debugEvents.integer){cgprintf("%22s", x); }
void
entevent(cent_t *cent, vec3_t position)
{
	entityState_t *es;
	int event;
	vec3_t dir;
	const char *s;
	int clientNum;
	clientinfo_t *ci;

	es = &cent->currstate;
	event = es->event & ~EV_EVENT_BITS;

	if(!event){
		DEBUGNAME("ZEROEVENT\n");
		return;
	}

	clientNum = es->clientNum;
	if(clientNum < 0 || clientNum >= MAX_CLIENTS)
		clientNum = 0;
	ci = &cgs.clientinfo[clientNum];

	switch(event){
	// movement generated events
	case EV_FOOTSTEP:
		DEBUGNAME("EV_FOOTSTEP");
		if(cg_footsteps.integer)
			trap_S_StartSound(nil, es->number, CHAN_BODY,
					  cgs.media.footsteps[ci->footsteps][rand()&3]);
		break;
	case EV_FOOTSTEP_METAL:
		DEBUGNAME("EV_FOOTSTEP_METAL");
		if(cg_footsteps.integer)
			trap_S_StartSound(nil, es->number, CHAN_BODY,
					  cgs.media.footsteps[FOOTSTEP_METAL][rand()&3]);
		break;
	case EV_FOOTSPLASH:
		DEBUGNAME("EV_FOOTSPLASH");
		if(cg_footsteps.integer)
			trap_S_StartSound(nil, es->number, CHAN_BODY,
					  cgs.media.footsteps[FOOTSTEP_SPLASH][rand()&3]);
		break;
	case EV_FOOTWADE:
		DEBUGNAME("EV_FOOTWADE");
		if(cg_footsteps.integer)
			trap_S_StartSound(nil, es->number, CHAN_BODY,
					  cgs.media.footsteps[FOOTSTEP_SPLASH][rand()&3]);
		break;
	case EV_SWIM:
		DEBUGNAME("EV_SWIM");
		if(cg_footsteps.integer)
			trap_S_StartSound(nil, es->number, CHAN_BODY,
					  cgs.media.footsteps[FOOTSTEP_SPLASH][rand()&3]);
		break;

	case EV_FALL_SHORT:
		DEBUGNAME("EV_FALL_SHORT");
		trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.landSound);
		if(clientNum == cg.pps.clientNum){
			// smooth landing z changes
			cg.landchange = -8;
			cg.landtime = cg.time;
		}
		break;
	case EV_FALL_MEDIUM:
		DEBUGNAME("EV_FALL_MEDIUM");
		// use normal pain sound
		trap_S_StartSound(nil, es->number, CHAN_VOICE, customsound(es->number, "*pain100_1.wav"));
		if(clientNum == cg.pps.clientNum){
			// smooth landing z changes
			cg.landchange = -16;
			cg.landtime = cg.time;
		}
		break;
	case EV_FALL_FAR:
		DEBUGNAME("EV_FALL_FAR");
		trap_S_StartSound(nil, es->number, CHAN_AUTO, customsound(es->number, "*fall1.wav"));
		cent->pe.paintime = cg.time;	// don't play a pain sound right after this
		if(clientNum == cg.pps.clientNum){
			// smooth landing z changes
			cg.landchange = -24;
			cg.landtime = cg.time;
		}
		break;

	case EV_STEP_4:
	case EV_STEP_8:
	case EV_STEP_12:
	case EV_STEP_16:	// smooth out step up transitions
		DEBUGNAME("EV_STEP");
		{
			float oldStep;
			int delta;
			int step;

			if(clientNum != cg.pps.clientNum)
				break;
			// if we are interpolating, we don't need to smooth steps
			if(cg.demoplayback || (cg.snap->ps.pm_flags & PMF_FOLLOW) ||
			   cg_nopredict.integer || cg_synchronousClients.integer)
				break;
			// check for stepping up before a previous step is completed
			delta = cg.time - cg.steptime;
			if(delta < STEP_TIME)
				oldStep = cg.stepchange * (STEP_TIME - delta) / STEP_TIME;
			else
				oldStep = 0;

			// add this amount
			step = 4 * (event - EV_STEP_4 + 1);
			cg.stepchange = oldStep + step;
			if(cg.stepchange > MAX_STEP_CHANGE)
				cg.stepchange = MAX_STEP_CHANGE;
			cg.steptime = cg.time;
			break;
		}

	case EV_JUMP_PAD:
		DEBUGNAME("EV_JUMP_PAD");
//		cgprintf( "EV_JUMP_PAD w/effect #%i\n", es->eventParm );
		{
			vec3_t up = {0, 0, 1};

			smokepuff(cent->lerporigin, up,
				     32,
				     1, 1, 1, 0.33f,
				     1000,
				     cg.time, 0,
				     LEF_PUFF_DONT_SCALE,
				     cgs.media.smokePuffShader);
		}

		// boing sound at origin, jump sound on player
		trap_S_StartSound(cent->lerporigin, -1, CHAN_VOICE, cgs.media.jumpPadSound);
		trap_S_StartSound(nil, es->number, CHAN_VOICE, customsound(es->number, "*jump1.wav"));
		break;
	case EV_SQUISH:
		DEBUGNAME("EV_SQUISH");
		cgprintf("EV_SQUISH w/effect #%i\n", es->eventParm);
		break;

	case EV_JUMP:
		DEBUGNAME("EV_JUMP");
		trap_S_StartSound(nil, es->number, CHAN_VOICE, customsound(es->number, "*jump1.wav"));
		break;
	case EV_AIRJUMP:
		DEBUGNAME("EV_AIRJUMP");
		{
			vec3_t up = {0, 0, 1};
			vec3_t feet;

			veccpy(cent->lerporigin, feet);
			feet[2] -= 10.0f;

			smokepuff(feet, up, 24, 1.0f, 1.0f, 1.0f, 0.5f, 800,
			   cg.time, 0, LEF_PUFF_DONT_SCALE,
			   cgs.media.smokePuffShader);
		}
		trap_S_StartSound(cent->lerporigin, es->number, CHAN_VOICE, cgs.media.airjumpSound);
		break;
	case EV_TAUNT:
		DEBUGNAME("EV_TAUNT");
		trap_S_StartSound(nil, es->number, CHAN_VOICE, customsound(es->number, "*taunt.wav"));
		break;
	case EV_WATER_TOUCH:
		DEBUGNAME("EV_WATER_TOUCH");
		trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.watrInSound);
		break;
	case EV_WATER_LEAVE:
		DEBUGNAME("EV_WATER_LEAVE");
		trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.watrOutSound);
		break;
	case EV_WATER_UNDER:
		DEBUGNAME("EV_WATER_UNDER");
		trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.watrUnSound);
		break;
	case EV_WATER_CLEAR:
		DEBUGNAME("EV_WATER_CLEAR");
		trap_S_StartSound(nil, es->number, CHAN_AUTO, customsound(es->number, "*gasp.wav"));
		break;

	case EV_ITEM_PICKUP:
		DEBUGNAME("EV_ITEM_PICKUP");
		{
			item_t *item;
			int index;

			index = es->eventParm;	// player predicted

			if(index < 1 || index >= bg_nitems)
				break;
			item = &bg_itemlist[index];

			// powerups and team items will have a separate global sound, this one
			// will be played at prediction time
			if(item->type == IT_POWERUP || item->type == IT_TEAM)
				trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.n_healthSound);
			else if(item->type == IT_PERSISTANT_POWERUP){
			}else
				trap_S_StartSound(nil, es->number, CHAN_AUTO, trap_S_RegisterSound(item->pickupsound[0], qfalse));

			// show icon and name on status bar
			if(es->number == cg.snap->ps.clientNum)
				itempickup(index);
		}
		break;

	case EV_GLOBAL_ITEM_PICKUP:
		DEBUGNAME("EV_GLOBAL_ITEM_PICKUP");
		{
			item_t *item;
			int index;

			index = es->eventParm;	// player predicted

			if(index < 1 || index >= bg_nitems)
				break;
			item = &bg_itemlist[index];
			// powerup pickups are global
			if(item->pickupsound[0] != nil)
				trap_S_StartSound(nil, cg.snap->ps.clientNum, CHAN_AUTO, trap_S_RegisterSound(item->pickupsound[0], qfalse));

			// show icon and name on status bar
			if(es->number == cg.snap->ps.clientNum)
				itempickup(index);
		}
		break;

	// breakables
	case EV_SMASH_CRATE:
	case EV_SMASH_STRONG_CRATE:
	case EV_SMASH_CHECKPOINT_CRATE:
		DEBUGNAME("EV_SMASH_CRATE");
		cratesmash(cent->lerporigin);
		break;
	case EV_TNT_EXPLODE:
		DEBUGNAME("EV_TNT_EXPLODE");
		tntexplode(cent->lerporigin);
		break;

	// weapon events
	case EV_NOAMMO:
		DEBUGNAME("EV_NOAMMO");
//		trap_S_StartSound (nil, es->number, CHAN_AUTO, cgs.media.noAmmoSound );
		if(es->number == cg.snap->ps.clientNum)
			outofammochange();
		break;
	case EV_CHANGE_WEAPON:
		DEBUGNAME("EV_CHANGE_WEAPON");
		trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.selectSound);
		break;
	case EV_FIRE_WEAPON:
		DEBUGNAME("EV_FIRE_WEAPON");
		fireweap(cent);
		break;

	case EV_USE_ITEM0:
		DEBUGNAME("EV_USE_ITEM0");
		useitem(cent);
		break;
	case EV_USE_ITEM1:
		DEBUGNAME("EV_USE_ITEM1");
		useitem(cent);
		break;
	case EV_USE_ITEM2:
		DEBUGNAME("EV_USE_ITEM2");
		useitem(cent);
		break;
	case EV_USE_ITEM3:
		DEBUGNAME("EV_USE_ITEM3");
		useitem(cent);
		break;
	case EV_USE_ITEM4:
		DEBUGNAME("EV_USE_ITEM4");
		useitem(cent);
		break;
	case EV_USE_ITEM5:
		DEBUGNAME("EV_USE_ITEM5");
		useitem(cent);
		break;
	case EV_USE_ITEM6:
		DEBUGNAME("EV_USE_ITEM6");
		useitem(cent);
		break;
	case EV_USE_ITEM7:
		DEBUGNAME("EV_USE_ITEM7");
		useitem(cent);
		break;
	case EV_USE_ITEM8:
		DEBUGNAME("EV_USE_ITEM8");
		useitem(cent);
		break;
	case EV_USE_ITEM9:
		DEBUGNAME("EV_USE_ITEM9");
		useitem(cent);
		break;
	case EV_USE_ITEM10:
		DEBUGNAME("EV_USE_ITEM10");
		useitem(cent);
		break;
	case EV_USE_ITEM11:
		DEBUGNAME("EV_USE_ITEM11");
		useitem(cent);
		break;
	case EV_USE_ITEM12:
		DEBUGNAME("EV_USE_ITEM12");
		useitem(cent);
		break;
	case EV_USE_ITEM13:
		DEBUGNAME("EV_USE_ITEM13");
		useitem(cent);
		break;
	case EV_USE_ITEM14:
		DEBUGNAME("EV_USE_ITEM14");
		useitem(cent);
		break;
	case EV_USE_ITEM15:
		DEBUGNAME("EV_USE_ITEM15");
		useitem(cent);
		break;

	//=================================================================

	// other events
	case EV_PLAYER_TELEPORT_IN:
		DEBUGNAME("EV_PLAYER_TELEPORT_IN");
		trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.teleInSound);
		spawneffect(position);
		break;

	case EV_PLAYER_TELEPORT_OUT:
		DEBUGNAME("EV_PLAYER_TELEPORT_OUT");
		trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.teleOutSound);
		spawneffect(position);
		break;

	case EV_ITEM_POP:
		DEBUGNAME("EV_ITEM_POP");
		trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.respawnSound);
		break;
	case EV_ITEM_RESPAWN:
		DEBUGNAME("EV_ITEM_RESPAWN");
		cent->misctime = cg.time;	// scale up from this
		trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.respawnSound);
		break;

	case EV_GRENADE_BOUNCE:
		DEBUGNAME("EV_GRENADE_BOUNCE");
		if(rand() & 1)
			trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.hgrenb1aSound);
		else
			trap_S_StartSound(nil, es->number, CHAN_AUTO, cgs.media.hgrenb2aSound);
		break;

	case EV_SCOREPLUM:
		DEBUGNAME("EV_SCOREPLUM");
		scoreplum(cent->currstate.otherEntityNum, cent->lerporigin, cent->currstate.time);
		break;

	// missile impacts
	case EV_MISSILE_HIT:
		DEBUGNAME("EV_MISSILE_HIT");
		ByteToDir(es->eventParm, dir);
		missilehitplayer(es->weapon, position, dir, es->otherEntityNum);
		break;

	case EV_MISSILE_MISS:
		DEBUGNAME("EV_MISSILE_MISS");
		ByteToDir(es->eventParm, dir);
		missilehitwall(es->weapon, 0, position, dir, IMPACTSOUND_DEFAULT);
		break;

	case EV_MISSILE_MISS_METAL:
		DEBUGNAME("EV_MISSILE_MISS_METAL");
		ByteToDir(es->eventParm, dir);
		missilehitwall(es->weapon, 0, position, dir, IMPACTSOUND_METAL);
		break;

	case EV_RAILTRAIL:
		DEBUGNAME("EV_RAILTRAIL");
		cent->currstate.weapon = WP_RAILGUN;

		if(es->clientNum == cg.snap->ps.clientNum && !cg.thirdperson){
			if(cg_drawGun.integer == 2)
				vecmad(es->origin2, 8, cg.refdef.viewaxis[1], es->origin2);
			else if(cg_drawGun.integer == 3)
				vecmad(es->origin2, 4, cg.refdef.viewaxis[1], es->origin2);
		}

		dorailtrail(ci, es->origin2, es->pos.trBase);

		// if the end was on a nomark surface, don't make an explosion
		if(es->eventParm != 255){
			ByteToDir(es->eventParm, dir);
			missilehitwall(es->weapon, es->clientNum, position, dir, IMPACTSOUND_DEFAULT);
		}
		break;

	case EV_BULLET_HIT_WALL:
		DEBUGNAME("EV_BULLET_HIT_WALL");
		ByteToDir(es->eventParm, dir);
		dobullet(es->pos.trBase, es->otherEntityNum, dir, qfalse, ENTITYNUM_WORLD);
		break;

	case EV_BULLET_HIT_FLESH:
		DEBUGNAME("EV_BULLET_HIT_FLESH");
		dobullet(es->pos.trBase, es->otherEntityNum, dir, qtrue, es->eventParm);
		break;

	case EV_SHOTGUN:
		DEBUGNAME("EV_SHOTGUN");
		shotgunfire(es);
		break;

	case EV_GENERAL_SOUND:
		DEBUGNAME("EV_GENERAL_SOUND");
		if(cgs.gamesounds[es->eventParm])
			trap_S_StartSound(nil, es->number, CHAN_VOICE, cgs.gamesounds[es->eventParm]);
		else{
			s = getconfigstr(CS_SOUNDS + es->eventParm);
			trap_S_StartSound(nil, es->number, CHAN_VOICE, customsound(es->number, s));
		}
		break;

	case EV_GLOBAL_SOUND:	// play from the player's head so it never diminishes
		DEBUGNAME("EV_GLOBAL_SOUND");
		if(cgs.gamesounds[es->eventParm])
			trap_S_StartSound(nil, cg.snap->ps.clientNum, CHAN_AUTO, cgs.gamesounds[es->eventParm]);
		else{
			s = getconfigstr(CS_SOUNDS + es->eventParm);
			trap_S_StartSound(nil, cg.snap->ps.clientNum, CHAN_AUTO, customsound(es->number, s));
		}
		break;

	case EV_GLOBAL_TEAM_SOUND: {	// play from the player's head so it never diminishes
		DEBUGNAME("EV_GLOBAL_TEAM_SOUND");
		switch(es->eventParm){
		case GTS_RED_CAPTURE:	// CTF: red team captured the blue flag, 1FCTF: red team captured the neutral flag
			if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED)
				addbufferedsound(cgs.media.captureYourTeamSound);
			else
				addbufferedsound(cgs.media.captureOpponentSound);
			break;
		case GTS_BLUE_CAPTURE:	// CTF: blue team captured the red flag, 1FCTF: blue team captured the neutral flag
			if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE)
				addbufferedsound(cgs.media.captureYourTeamSound);
			else
				addbufferedsound(cgs.media.captureOpponentSound);
			break;
		case GTS_RED_RETURN:	// CTF: blue flag returned, 1FCTF: never used
			if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED)
				addbufferedsound(cgs.media.returnYourTeamSound);
			else
				addbufferedsound(cgs.media.returnOpponentSound);
			addbufferedsound(cgs.media.blueFlagReturnedSound);
			break;
		case GTS_BLUE_RETURN:	// CTF red flag returned, 1FCTF: neutral flag returned
			if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE)
				addbufferedsound(cgs.media.returnYourTeamSound);
			else
				addbufferedsound(cgs.media.returnOpponentSound);
			addbufferedsound(cgs.media.redFlagReturnedSound);
			break;

		case GTS_RED_TAKEN:	// CTF: red team took blue flag, 1FCTF: blue team took the neutral flag
			// if this player picked up the flag then a sound is played in feedback
			if(cg.snap->ps.powerups[PW_BLUEFLAG] || cg.snap->ps.powerups[PW_NEUTRALFLAG]){
			}else{
				if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE){
					addbufferedsound(cgs.media.enemyTookYourFlagSound);
				}else if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED){
					addbufferedsound(cgs.media.yourTeamTookEnemyFlagSound);
				}
			}
			break;
		case GTS_BLUE_TAKEN:	// CTF: blue team took the red flag, 1FCTF red team took the neutral flag
			// if this player picked up the flag then a sound is played in feedback
			if(cg.snap->ps.powerups[PW_REDFLAG] || cg.snap->ps.powerups[PW_NEUTRALFLAG]){
			}else{
				if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED){
					addbufferedsound(cgs.media.enemyTookYourFlagSound);
				}else if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE){
					addbufferedsound(cgs.media.yourTeamTookEnemyFlagSound);
				}
			}
			break;

		case GTS_REDTEAM_SCORED:
			addbufferedsound(cgs.media.redScoredSound);
			break;
		case GTS_BLUETEAM_SCORED:
			addbufferedsound(cgs.media.blueScoredSound);
			break;
		case GTS_REDTEAM_TOOK_LEAD:
			addbufferedsound(cgs.media.redLeadsSound);
			break;
		case GTS_BLUETEAM_TOOK_LEAD:
			addbufferedsound(cgs.media.blueLeadsSound);
			break;
		case GTS_TEAMS_ARE_TIED:
			addbufferedsound(cgs.media.teamsTiedSound);
			break;
		default:
			break;
		}
		break;
	}

	case EV_PAIN:
		// local player sounds are triggered in feedback,
		// so ignore events on the player
		DEBUGNAME("EV_PAIN");
		if(cent->currstate.number != cg.snap->ps.clientNum)
			painevent(cent, es->eventParm);
		break;

	case EV_DEATH1:
	case EV_DEATH2:
	case EV_DEATH3:
		DEBUGNAME("EV_DEATHx");

		if(waterlevel(cent) >= 1)
			trap_S_StartSound(nil, es->number, CHAN_VOICE, customsound(es->number, "*drown.wav"));
		else
			trap_S_StartSound(nil, es->number, CHAN_VOICE, customsound(es->number, va("*death%i.wav", event - EV_DEATH1 + 1)));

		break;

	case EV_OBITUARY:
		DEBUGNAME("EV_OBITUARY");
		obituary(es);
		break;

	case EV_LEVELRESPAWN:
		DEBUGNAME("EV_LEVELRESPAWN");
		initlocalents();
		initmarkpolys();
		shaderstatechanged();
		trap_S_ClearLoopingSounds(qtrue);
		break;
		

	// powerup events
	case EV_POWERUP_QUAD:
		DEBUGNAME("EV_POWERUP_QUAD");
		if(es->number == cg.snap->ps.clientNum){
			cg.powerupactive = PW_QUAD;
			cg.poweruptime = cg.time;
		}
		trap_S_StartSound(nil, es->number, CHAN_ITEM, cgs.media.quadSound);
		break;
	case EV_POWERUP_BATTLESUIT:
		DEBUGNAME("EV_POWERUP_BATTLESUIT");
		if(es->number == cg.snap->ps.clientNum){
			cg.powerupactive = PW_BATTLESUIT;
			cg.poweruptime = cg.time;
		}
		trap_S_StartSound(nil, es->number, CHAN_ITEM, cgs.media.protectSound);
		break;
	case EV_POWERUP_REGEN:
		DEBUGNAME("EV_POWERUP_REGEN");
		if(es->number == cg.snap->ps.clientNum){
			cg.powerupactive = PW_REGEN;
			cg.poweruptime = cg.time;
		}
		trap_S_StartSound(nil, es->number, CHAN_ITEM, cgs.media.regenSound);
		break;

	case EV_GIB_PLAYER:
		DEBUGNAME("EV_GIB_PLAYER");
		trap_S_StartSound(nil, es->number, CHAN_BODY, cgs.media.gibSound);
		gibplayer(cent->lerporigin);
		break;

	case EV_STOPLOOPINGSOUND:
		DEBUGNAME("EV_STOPLOOPINGSOUND");
		trap_S_StopLoopingSound(es->number);
		es->loopSound = 0;
		break;

	case EV_DEBUG_LINE:
		DEBUGNAME("EV_DEBUG_LINE");
		drawbeam(cent);
		break;

	default:
		DEBUGNAME("UNKNOWN");
		cgerrorf("Unknown event: %i", event);
		break;
	}

	if(cg_debugEvents.integer)
		cgprintf(" (%02i) ent=%03i parm=%i\n", event, es->number, es->eventParm);
}

void
chkevents(cent_t *cent)
{
	// check for event-only entities
	if(cent->currstate.eType > ET_EVENTS){
		if(cent->prevevent)
			return;	// already fired
		// if this is a player event set the entity number of the client entity number
		if(cent->currstate.eFlags & EF_PLAYER_EVENT)
			cent->currstate.number = cent->currstate.otherEntityNum;

		cent->prevevent = 1;

		cent->currstate.event = cent->currstate.eType - ET_EVENTS;
	}else{
		// check for events riding with another entity
		if(cent->currstate.event == cent->prevevent)
			return;
		cent->prevevent = cent->currstate.event;
		if((cent->currstate.event & ~EV_EVENT_BITS) == 0)
			return;
	}

	// calculate the position at exactly the frame time
	evaltrajectory(&cent->currstate.pos, cg.snap->serverTime, cent->lerporigin);
	setentsoundpos(cent);

	entevent(cent, cent->lerporigin);
}
