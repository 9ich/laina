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
// g_weapon.c
// perform the server side effects of a weapon firing

#include "g_local.h"

static float s_quadFactor;
static vec3_t forward, right, up;
static vec3_t muzzle;

#define NUM_NAILSHOTS 15

/*
================
bounceprojectile
================
*/
void
bounceprojectile(vec3_t start, vec3_t impact, vec3_t dir, vec3_t endout)
{
	vec3_t v, newv;
	float dot;

	vecsub(impact, start, v);
	dot = vecdot(v, dir);
	vecmad(v, -2*dot, dir, newv);

	vecnorm(newv);
	vecmad(impact, 8192, newv, endout);
}

/*
======================================================================

GAUNTLET

======================================================================
*/

void
Weapon_Gauntlet(ent_t *ent)
{
}

/*
===============
chkgauntletattack
===============
*/
qboolean
chkgauntletattack(ent_t *ent)
{
	trace_t tr;
	vec3_t mins, maxs, end, muzzle;
	ent_t *tent, *traceEnt;
	int damage;

	if(ent->client->noclip)
		return qfalse;

	// set aiming directions
	anglevecs(ent->client->ps.viewangles, forward, right, up);

	veccpy(ent->client->ps.origin, muzzle);
	muzzle[2] += ent->client->ps.viewheight;
	vecmad(muzzle, MELEE_RANGE, forward, end);
	vecset(mins, -6, -6, -6);
	vecset(maxs, 6, 6, 6);

	trap_Trace(&tr, muzzle, mins, maxs, end, ent->s.number, MASK_SHOT);

	if(tr.surfaceFlags & SURF_NOIMPACT)
		return qfalse;

	traceEnt = &g_entities[tr.entityNum];

	// send blood impact
	if(traceEnt->takedmg && traceEnt->client){
		tent = enttemp(tr.endpos, EV_MISSILE_HIT);
		tent->s.otherEntityNum = traceEnt->s.number;
		tent->s.eventParm = DirToByte(tr.plane.normal);
		tent->s.weapon = ent->s.weapon;
	}

	if(!traceEnt->takedmg)
		return qfalse;

	if(ent->client->ps.powerups[PW_QUAD]){
		addevent(ent, EV_POWERUP_QUAD, 0);
		s_quadFactor = g_quadfactor.value;
	}else
		s_quadFactor = 1;
	damage = 50 * s_quadFactor;

	entdamage(traceEnt, ent, ent, forward, tr.endpos,
	   damage, 0, MOD_GAUNTLET);

	return qtrue;
}

qboolean
chkmelee2attack(ent_t *ent)
{
	vec3_t mins, maxs, pos;
	int damage;

	if(ent->client->noclip)
		return qfalse;
	
	if(ent->client->ps.powerups[PW_QUAD]){
		addevent(ent, EV_POWERUP_QUAD, 0);
		s_quadFactor = g_quadfactor.value;
	}else
		s_quadFactor = 1;
	damage = 50 * s_quadFactor;

	veccpy(ent->client->ps.origin, pos);
	pos[2] += ent->client->ps.viewheight;
	vecset(mins, -MELEE2_RANGE, -MELEE2_RANGE, -21);
	vecset(maxs, MELEE2_RANGE, MELEE2_RANGE, -20);
	boxdamage(pos, mins, maxs, ent, ent->s.number,
	   damage, MOD_GAUNTLET);

	// crates directly beneath the player get smashed too
	vecset(mins, MINS_X, MINS_Y, MELEE2_DOWNRANGE);
	vecset(maxs, MAXS_X, MAXS_Y, 1);
	boxdamage(ent->client->ps.origin, mins, maxs, ent, ent->s.number,
	   damage, MOD_GAUNTLET);

	return qtrue;
}

/*
======================================================================

MACHINEGUN

======================================================================
*/

/*
======================
snapvectortowards

Round a vector to integers for more efficient network
transmission, but make sure that it rounds towards a given point
rather than blindly truncating.  This prevents it from truncating
into a wall.
======================
*/
void
snapvectortowards(vec3_t v, vec3_t to)
{
	int i;

	for(i = 0; i < 3; i++){
		if(to[i] <= v[i])
			v[i] = floor(v[i]);
		else
			v[i] = ceil(v[i]);
	}
}

#define MACHINEGUN_SPREAD	200
#define MACHINEGUN_DAMAGE	7
#define MACHINEGUN_TEAM_DAMAGE	5	// wimpier MG in teamplay

void
Bullet_Fire(ent_t *ent, float spread, int damage, int mod)
{
	trace_t tr;
	vec3_t end;
	float r;
	float u;
	ent_t *tent;
	ent_t *traceEnt;
	int i, passent;

	damage *= s_quadFactor;

	r = random() * M_PI * 2.0f;
	u = sin(r) * crandom() * spread * 16;
	r = cos(r) * crandom() * spread * 16;
	vecmad(muzzle, 8192*16, forward, end);
	vecmad(end, r, right, end);
	vecmad(end, u, up, end);

	passent = ent->s.number;
	for(i = 0; i < 10; i++){
		trap_Trace(&tr, muzzle, nil, nil, end, passent, MASK_SHOT);
		if(tr.surfaceFlags & SURF_NOIMPACT)
			return;

		traceEnt = &g_entities[tr.entityNum];

		// snap the endpos to integers, but nudged towards the line
		snapvectortowards(tr.endpos, muzzle);

		// send bullet impact
		if(traceEnt->takedmg && traceEnt->client){
			tent = enttemp(tr.endpos, EV_BULLET_HIT_FLESH);
			tent->s.eventParm = traceEnt->s.number;
			if(logaccuracyhit(traceEnt, ent))
				ent->client->accuracyhits++;
		}else{
			tent = enttemp(tr.endpos, EV_BULLET_HIT_WALL);
			tent->s.eventParm = DirToByte(tr.plane.normal);
		}
		tent->s.otherEntityNum = ent->s.number;

		if(traceEnt->takedmg){
			entdamage(traceEnt, ent, ent, forward, tr.endpos,
				 damage, 0, mod);
		}
		break;
	}
}

/*
======================================================================

BFG

======================================================================
*/

void
BFG_Fire(ent_t *ent)
{
	ent_t   *m;

	m = fire_bfg(ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashdmg *= s_quadFactor;

//	vecadd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}

/*
======================================================================

SHOTGUN

======================================================================
*/

// DEFAULT_SHOTGUN_SPREAD and DEFAULT_SHOTGUN_COUNT	are in bg_public.h, because
// client predicts same spreads
#define DEFAULT_SHOTGUN_DAMAGE 10

qboolean
ShotgunPellet(vec3_t start, vec3_t end, ent_t *ent)
{
	trace_t tr;
	int damage, i, passent;
	ent_t   *traceEnt;
	vec3_t tr_start, tr_end;

	passent = ent->s.number;
	veccpy(start, tr_start);
	veccpy(end, tr_end);
	for(i = 0; i < 10; i++){
		trap_Trace(&tr, tr_start, nil, nil, tr_end, passent, MASK_SHOT);
		traceEnt = &g_entities[tr.entityNum];

		// send bullet impact
		if(tr.surfaceFlags & SURF_NOIMPACT)
			return qfalse;

		if(traceEnt->takedmg){
			damage = DEFAULT_SHOTGUN_DAMAGE * s_quadFactor;
			entdamage(traceEnt, ent, ent, forward, tr.endpos, damage, 0, MOD_SHOTGUN);
			if(logaccuracyhit(traceEnt, ent))
				return qtrue;

		}
		return qfalse;
	}
	return qfalse;
}

// this should match shotgunpattern
void
ShotgunPattern(vec3_t origin, vec3_t origin2, int seed, ent_t *ent)
{
	int i;
	float r, u;
	vec3_t end;
	vec3_t forward, right, up;
	qboolean hitClient = qfalse;

	// derive the right and up vectors from the forward vector, because
	// the client won't have any other information
	VectorNormalize2(origin2, forward);
	vecperp(right, forward);
	veccross(forward, right, up);

	// generate the "random" spread pattern
	for(i = 0; i < DEFAULT_SHOTGUN_COUNT; i++){
		r = Q_crandom(&seed) * DEFAULT_SHOTGUN_SPREAD * 16;
		u = Q_crandom(&seed) * DEFAULT_SHOTGUN_SPREAD * 16;
		vecmad(origin, 8192 * 16, forward, end);
		vecmad(end, r, right, end);
		vecmad(end, u, up, end);
		if(ShotgunPellet(origin, end, ent) && !hitClient){
			hitClient = qtrue;
			ent->client->accuracyhits++;
		}
	}
}

void
weapon_supershotgun_fire(ent_t *ent)
{
	ent_t           *tent;

	// send shotgun blast
	tent = enttemp(muzzle, EV_SHOTGUN);
	vecmul(forward, 4096, tent->s.origin2);
	SnapVector(tent->s.origin2);
	tent->s.eventParm = rand() & 255;	// seed for spread pattern
	tent->s.otherEntityNum = ent->s.number;

	ShotgunPattern(tent->s.pos.trBase, tent->s.origin2, tent->s.eventParm, ent);
}

/*
======================================================================

GRENADE LAUNCHER

======================================================================
*/

void
weapon_grenadelauncher_fire(ent_t *ent)
{
	ent_t   *m;

	// extra vertical velocity
	forward[2] += 0.2f;
	vecnorm(forward);

	m = fire_grenade(ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashdmg *= s_quadFactor;

//	vecadd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}

/*
======================================================================

ROCKET

======================================================================
*/

void
Weapon_RocketLauncher_Fire(ent_t *ent)
{
	ent_t   *m;

	m = fire_rocket(ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashdmg *= s_quadFactor;

//	vecadd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}

void
Weapon_Crossbow_Fire(ent_t *ent)
{
	ent_t   *m;

	m = fire_bolt(ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashdmg *= s_quadFactor;

//	vecadd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}

/*
======================================================================

PLASMA GUN

======================================================================
*/

void
Weapon_Plasmagun_Fire(ent_t *ent)
{
	ent_t   *m;

	m = fire_plasma(ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashdmg *= s_quadFactor;

//	vecadd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}

/*
======================================================================

RAILGUN

======================================================================
*/

/*
=================
weapon_railgun_fire
=================
*/
#define MAX_RAIL_HITS 4
void
weapon_railgun_fire(ent_t *ent)
{
	vec3_t end;
	trace_t trace;
	ent_t   *tent;
	ent_t   *traceEnt;
	int damage;
	int i;
	int hits;
	int unlinked;
	int passent;
	ent_t   *unlinkedEntities[MAX_RAIL_HITS];

	damage = 100 * s_quadFactor;

	vecmad(muzzle, 8192, forward, end);

	// trace only against the solids, so the railgun will go through people
	unlinked = 0;
	hits = 0;
	passent = ent->s.number;
	do{
		trap_Trace(&trace, muzzle, nil, nil, end, passent, MASK_SHOT);
		if(trace.entityNum >= ENTITYNUM_MAX_NORMAL)
			break;
		traceEnt = &g_entities[trace.entityNum];
		if(traceEnt->takedmg){
			if(logaccuracyhit(traceEnt, ent))
				hits++;
			entdamage(traceEnt, ent, ent, forward, trace.endpos, damage, 0, MOD_RAILGUN);
		}
		if(trace.contents & CONTENTS_SOLID)
			break;	// we hit something solid enough to stop the beam
		// unlink this entity, so the next trace will go past it
		trap_UnlinkEntity(traceEnt);
		unlinkedEntities[unlinked] = traceEnt;
		unlinked++;
	}while(unlinked < MAX_RAIL_HITS)
	;

	// link back in any entities we unlinked
	for(i = 0; i < unlinked; i++)
		trap_LinkEntity(unlinkedEntities[i]);

	// the final trace endpos will be the terminal point of the rail trail

	// snap the endpos to integers to save net bandwidth, but nudged towards the line
	snapvectortowards(trace.endpos, muzzle);

	// send railgun beam effect
	tent = enttemp(trace.endpos, EV_RAILTRAIL);

	// set player number for custom colors on the railtrail
	tent->s.clientNum = ent->s.clientNum;

	veccpy(muzzle, tent->s.origin2);
	// move origin a bit to come closer to the drawn gun muzzle
	vecmad(tent->s.origin2, 4, right, tent->s.origin2);
	vecmad(tent->s.origin2, -1, up, tent->s.origin2);

	// no explosion at end if SURF_NOIMPACT, but still make the trail
	if(trace.surfaceFlags & SURF_NOIMPACT)
		tent->s.eventParm = 255;	// don't make the explosion at the end
	else
		tent->s.eventParm = DirToByte(trace.plane.normal);
	tent->s.clientNum = ent->s.clientNum;

	// give the shooter a reward sound if they have made two railgun hits in a row
	if(hits == 0)
		// complete miss
		ent->client->accuratecount = 0;
	else{
		// check for "impressive" reward sound
		ent->client->accuratecount += hits;
		if(ent->client->accuratecount >= 2){
			ent->client->accuratecount -= 2;
			ent->client->ps.persistant[PERS_IMPRESSIVE_COUNT]++;
			// add the sprite over the player's head
			ent->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP);
			ent->client->ps.eFlags |= EF_AWARD_IMPRESSIVE;
			ent->client->rewardtime = level.time + REWARD_SPRITE_TIME;
		}
		ent->client->accuracyhits++;
	}
}

/*
======================================================================

GRAPPLING HOOK

======================================================================
*/

void
Weapon_GrapplingHook_Fire(ent_t *ent)
{
	if(!ent->client->fireheld && !ent->client->hook)
		fire_grapple(ent, muzzle, forward);

	ent->client->fireheld = qtrue;
}

void
weapon_hook_free(ent_t *ent)
{
	ent->parent->client->hook = nil;
	ent->parent->client->ps.pm_flags &= ~PMF_GRAPPLE_PULL;
	entfree(ent);
}

void
weapon_hook_think(ent_t *ent)
{
	if(ent->enemy){
		vec3_t v, oldorigin;

		veccpy(ent->r.currentOrigin, oldorigin);
		v[0] = ent->enemy->r.currentOrigin[0] + (ent->enemy->r.mins[0] + ent->enemy->r.maxs[0]) * 0.5;
		v[1] = ent->enemy->r.currentOrigin[1] + (ent->enemy->r.mins[1] + ent->enemy->r.maxs[1]) * 0.5;
		v[2] = ent->enemy->r.currentOrigin[2] + (ent->enemy->r.mins[2] + ent->enemy->r.maxs[2]) * 0.5;
		snapvectortowards(v, oldorigin);	// save net bandwidth

		setorigin(ent, v);
	}

	veccpy(ent->r.currentOrigin, ent->parent->client->ps.grapplePoint);
}

/*
======================================================================

LIGHTNING GUN

======================================================================
*/

void
Weapon_LightningFire(ent_t *ent)
{
	trace_t tr;
	vec3_t end;
	ent_t   *traceEnt, *tent;
	int damage, i, passent;

	damage = 8 * s_quadFactor;

	passent = ent->s.number;
	for(i = 0; i < 10; i++){
		vecmad(muzzle, LIGHTNING_RANGE, forward, end);

		trap_Trace(&tr, muzzle, nil, nil, end, passent, MASK_SHOT);

		if(tr.entityNum == ENTITYNUM_NONE)
			return;

		traceEnt = &g_entities[tr.entityNum];

		if(traceEnt->takedmg){
			entdamage(traceEnt, ent, ent, forward, tr.endpos,
				 damage, 0, MOD_LIGHTNING);
		}

		if(traceEnt->takedmg && traceEnt->client){
			tent = enttemp(tr.endpos, EV_MISSILE_HIT);
			tent->s.otherEntityNum = traceEnt->s.number;
			tent->s.eventParm = DirToByte(tr.plane.normal);
			tent->s.weapon = ent->s.weapon;
			if(logaccuracyhit(traceEnt, ent))
				ent->client->accuracyhits++;
		}else if(!(tr.surfaceFlags & SURF_NOIMPACT)){
			tent = enttemp(tr.endpos, EV_MISSILE_MISS);
			tent->s.eventParm = DirToByte(tr.plane.normal);
		}

		break;
	}
}


//======================================================================

/*
===============
logaccuracyhit
===============
*/
qboolean
logaccuracyhit(ent_t *target, ent_t *attacker)
{
	if(!target->takedmg)
		return qfalse;

	if(target == attacker)
		return qfalse;

	if(!target->client)
		return qfalse;

	if(!attacker->client)
		return qfalse;

	if(target->client->ps.stats[STAT_HEALTH] <= 0)
		return qfalse;

	if(onsameteam(target, attacker))
		return qfalse;

	return qtrue;
}

/*
===============
calcmuzzlepoint

set muzzle location relative to pivoting eye
===============
*/
void
calcmuzzlepoint(ent_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint)
{
	veccpy(ent->s.pos.trBase, muzzlePoint);
	muzzlePoint[2] += ent->client->ps.viewheight;
	vecmad(muzzlePoint, 14, forward, muzzlePoint);
	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector(muzzlePoint);
}

/*
===============
calcmuzzlepointorigin

set muzzle location relative to pivoting eye
===============
*/
void
calcmuzzlepointorigin(ent_t *ent, vec3_t origin, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint)
{
	veccpy(ent->s.pos.trBase, muzzlePoint);
	muzzlePoint[2] += ent->client->ps.viewheight;
	vecmad(muzzlePoint, 14, forward, muzzlePoint);
	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector(muzzlePoint);
}

/*
===============
fireweapon
===============
*/
void
fireweapon(ent_t *ent)
{
	if(ent->client->ps.powerups[PW_QUAD])
		s_quadFactor = g_quadfactor.value;
	else
		s_quadFactor = 1;


	// track shots taken for accuracy tracking.  Grapple is not a weapon and gauntet is just not tracked
	if(ent->s.weapon != WP_GRAPPLING_HOOK && ent->s.weapon != WP_GAUNTLET){
		ent->client->accuracyshots++;
	}

	// set aiming directions
	anglevecs(ent->client->ps.viewangles, forward, right, up);

	calcmuzzlepointorigin(ent, ent->client->oldorigin, forward, right, up, muzzle);

	// fire the specific weapon
	switch(ent->s.weapon){
	case WP_GAUNTLET:
		Weapon_Gauntlet(ent);
		break;
	case WP_LIGHTNING:
		Weapon_LightningFire(ent);
		break;
	case WP_SHOTGUN:
		weapon_supershotgun_fire(ent);
		break;
	case WP_MACHINEGUN:
		if(g_gametype.integer != GT_TEAM)
			Bullet_Fire(ent, MACHINEGUN_SPREAD, MACHINEGUN_DAMAGE, MOD_MACHINEGUN);
		else
			Bullet_Fire(ent, MACHINEGUN_SPREAD, MACHINEGUN_TEAM_DAMAGE, MOD_MACHINEGUN);
		break;
	case WP_GRENADE_LAUNCHER:
		weapon_grenadelauncher_fire(ent);
		break;
	case WP_CROSSBOW:
		Weapon_Crossbow_Fire(ent);
		break;
	case WP_ROCKET_LAUNCHER:
		Weapon_RocketLauncher_Fire(ent);
		break;
	case WP_PLASMAGUN:
		Weapon_Plasmagun_Fire(ent);
		break;
	case WP_RAILGUN:
		weapon_railgun_fire(ent);
		break;
	case WP_BFG:
		BFG_Fire(ent);
		break;
	case WP_GRAPPLING_HOOK:
		Weapon_GrapplingHook_Fire(ent);
		break;
	default:
// FIXME		errorf( "Bad ent->s.weapon" );
		break;
	}
}

