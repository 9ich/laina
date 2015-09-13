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

#define MISSILE_PRESTEP_TIME 5

/*
================
bouncemissile

================
*/
void
bouncemissile(ent_t *ent, trace_t *trace)
{
	vec3_t velocity;
	float dot;
	int hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.prevtime + (level.time - level.prevtime) * trace->fraction;
	evaltrajectorydelta(&ent->s.pos, hitTime, velocity);
	dot = vecdot(velocity, trace->plane.normal);
	vecsadd(velocity, -2*dot, trace->plane.normal, ent->s.pos.trDelta);

	if(ent->s.eFlags & EF_BOUNCE_HALF){
		vecscale(ent->s.pos.trDelta, 0.65, ent->s.pos.trDelta);
		// check for stop
		if(trace->plane.normal[2] > 0.2 && veclen(ent->s.pos.trDelta) < 40){
			setorigin(ent, trace->endpos);
			ent->s.time = level.time / 4;
			return;
		}
	}

	vecadd(ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	veccopy(ent->r.currentOrigin, ent->s.pos.trBase);
	ent->s.pos.trTime = level.time;
}

/*
================
explodemissile

Explode a missile without an impact
================
*/
void
explodemissile(ent_t *ent)
{
	vec3_t dir;
	vec3_t origin;

	evaltrajectory(&ent->s.pos, level.time, origin);
	SnapVector(origin);
	setorigin(ent, origin);

	// we don't have a valid direction, so just point straight up
	dir[0] = dir[1] = 0;
	dir[2] = 1;

	ent->s.eType = ET_GENERAL;
	addevent(ent, EV_MISSILE_MISS, DirToByte(dir));

	ent->freeafterevent = qtrue;

	// splash damage
	if(ent->splashdmg)
		if(radiusdamage(ent->r.currentOrigin, ent->parent, ent->splashdmg, ent->splashradius, ent
				  , ent->splashmeansofdeath))
			g_entities[ent->r.ownerNum].client->accuracyhits++;

	trap_LinkEntity(ent);
}


/*
================
missileimpact
================
*/
void
missileimpact(ent_t *ent, trace_t *trace)
{
	ent_t *other;
	qboolean hitClient = qfalse;
	other = &g_entities[trace->entityNum];

	// check for bounce
	if(!other->takedmg &&
	   (ent->s.eFlags & (EF_BOUNCE | EF_BOUNCE_HALF))){
		bouncemissile(ent, trace);
		addevent(ent, EV_GRENADE_BOUNCE, 0);
		return;
	}

	// impact damage
	if(other->takedmg)
		// FIXME: wrong damage direction?
		if(ent->damage){
			vec3_t velocity;

			if(logaccuracyhit(other, &g_entities[ent->r.ownerNum])){
				g_entities[ent->r.ownerNum].client->accuracyhits++;
				hitClient = qtrue;
			}
			evaltrajectorydelta(&ent->s.pos, level.time, velocity);
			if(veclen(velocity) == 0)
				velocity[2] = 1;	// stepped on a grenade
			entdamage(other, ent, &g_entities[ent->r.ownerNum], velocity,
				 ent->s.origin, ent->damage,
				 0, ent->meansofdeath);
		}


	if(!strcmp(ent->classname, "hook")){
		ent_t *nent;
		vec3_t v;

		nent = entspawn();
		if(other->takedmg && other->client){
			addevent(nent, EV_MISSILE_HIT, DirToByte(trace->plane.normal));
			nent->s.otherEntityNum = other->s.number;

			ent->enemy = other;

			v[0] = other->r.currentOrigin[0] + (other->r.mins[0] + other->r.maxs[0]) * 0.5;
			v[1] = other->r.currentOrigin[1] + (other->r.mins[1] + other->r.maxs[1]) * 0.5;
			v[2] = other->r.currentOrigin[2] + (other->r.mins[2] + other->r.maxs[2]) * 0.5;

			snapvectortowards(v, ent->s.pos.trBase);	// save net bandwidth
		}else{
			veccopy(trace->endpos, v);
			addevent(nent, EV_MISSILE_MISS, DirToByte(trace->plane.normal));
			ent->enemy = nil;
		}

		snapvectortowards(v, ent->s.pos.trBase);	// save net bandwidth

		nent->freeafterevent = qtrue;
		// change over to a normal entity right at the point of impact
		nent->s.eType = ET_GENERAL;
		ent->s.eType = ET_GRAPPLE;

		setorigin(ent, v);
		setorigin(nent, v);

		ent->think = weapon_hook_think;
		ent->nextthink = level.time + FRAMETIME;

		ent->parent->client->ps.pm_flags |= PMF_GRAPPLE_PULL;
		veccopy(ent->r.currentOrigin, ent->parent->client->ps.grapplePoint);

		trap_LinkEntity(ent);
		trap_LinkEntity(nent);

		return;
	}

	// is it cheaper in bandwidth to just remove this ent and create a new
	// one, rather than changing the missile into the explosion?

	if(other->takedmg && other->client){
		addevent(ent, EV_MISSILE_HIT, DirToByte(trace->plane.normal));
		ent->s.otherEntityNum = other->s.number;
	}else if(trace->surfaceFlags & SURF_METALSTEPS)
		addevent(ent, EV_MISSILE_MISS_METAL, DirToByte(trace->plane.normal));
	else
		addevent(ent, EV_MISSILE_MISS, DirToByte(trace->plane.normal));

	ent->freeafterevent = qtrue;

	// change over to a normal entity right at the point of impact
	ent->s.eType = ET_GENERAL;

	snapvectortowards(trace->endpos, ent->s.pos.trBase);	// save net bandwidth

	setorigin(ent, trace->endpos);

	// splash damage (doesn't apply to person directly hit)
	if(ent->splashdmg)
		if(radiusdamage(trace->endpos, ent->parent, ent->splashdmg, ent->splashradius,
				  other, ent->splashmeansofdeath))
			if(!hitClient)
				g_entities[ent->r.ownerNum].client->accuracyhits++;

	trap_LinkEntity(ent);
}

/*
================
runmissile
================
*/
void
runmissile(ent_t *ent)
{
	vec3_t origin;
	trace_t tr;
	int passent;

	// get current position
	evaltrajectory(&ent->s.pos, level.time, origin);

	// if this missile bounced off an invulnerability sphere
	if(ent->target_ent)
		passent = ent->target_ent->s.number;

	else
		// ignore interactions with the missile owner
		passent = ent->r.ownerNum;
	// trace a line from the previous position to the current position
	trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask);

	if(tr.startsolid || tr.allsolid){
		// make sure the tr.entityNum is set to the entity we're stuck in
		trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, passent, ent->clipmask);
		tr.fraction = 0;
	}else
		veccopy(tr.endpos, ent->r.currentOrigin);

	trap_LinkEntity(ent);

	if(tr.fraction != 1){
		// never explode or bounce on sky
		if(tr.surfaceFlags & SURF_NOIMPACT){
			// If grapple, reset owner
			if(ent->parent && ent->parent->client && ent->parent->client->hook == ent)
				ent->parent->client->hook = nil;
			entfree(ent);
			return;
		}
		missileimpact(ent, &tr);
		if(ent->s.eType != ET_MISSILE)
			return;	// exploded
	}
	// check think function after bouncing
	runthink(ent);
}

ent_t*
fire_bolt(ent_t *self, vec3_t start, vec3_t dir)
{
	ent_t *bolt;

	vecnorm(dir);

	bolt = entspawn();
	bolt->classname = "bolt";
	bolt->nextthink = level.time + 10000;
	bolt->think = explodemissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_CROSSBOW;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 20;
	bolt->splashdmg = 15;
	bolt->splashradius = 20;
	bolt->meansofdeath = MOD_BOLT;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = nil;

	bolt->s.pos.trType = TR_LINEAR;
	// move a bit on the very first frame
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;
	veccopy(start, bolt->s.pos.trBase);
	vecscale(dir, 1000, bolt->s.pos.trDelta);
	// save net bandwidth
	SnapVector(bolt->s.pos.trDelta);

	veccopy(start, bolt->r.currentOrigin);

	return bolt;
}

/*
=================
fire_plasma

=================
*/
ent_t *
fire_plasma(ent_t *self, vec3_t start, vec3_t dir)
{
	ent_t *bolt;

	vecnorm(dir);

	bolt = entspawn();
	bolt->classname = "plasma";
	bolt->nextthink = level.time + 10000;
	bolt->think = explodemissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_PLASMAGUN;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 20;
	bolt->splashdmg = 15;
	bolt->splashradius = 20;
	bolt->meansofdeath = MOD_PLASMA;
	bolt->splashmeansofdeath = MOD_PLASMA_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = nil;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;	// move a bit on the very first frame
	veccopy(start, bolt->s.pos.trBase);
	vecscale(dir, 2000, bolt->s.pos.trDelta);
	SnapVector(bolt->s.pos.trDelta);	// save net bandwidth

	veccopy(start, bolt->r.currentOrigin);

	return bolt;
}

/*
=================
fire_grenade
=================
*/
ent_t *
fire_grenade(ent_t *self, vec3_t start, vec3_t dir)
{
	ent_t *bolt;

	vecnorm(dir);

	bolt = entspawn();
	bolt->classname = "grenade";
	bolt->nextthink = level.time + 2500;
	bolt->think = explodemissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_GRENADE_LAUNCHER;
	bolt->s.eFlags = EF_BOUNCE_HALF;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashdmg = 100;
	bolt->splashradius = 150;
	bolt->meansofdeath = MOD_GRENADE;
	bolt->splashmeansofdeath = MOD_GRENADE_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = nil;

	bolt->s.pos.trType = TR_GRAVITY;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;	// move a bit on the very first frame
	veccopy(start, bolt->s.pos.trBase);
	vecscale(dir, 700, bolt->s.pos.trDelta);
	SnapVector(bolt->s.pos.trDelta);	// save net bandwidth

	veccopy(start, bolt->r.currentOrigin);

	return bolt;
}

/*
=================
fire_bfg
=================
*/
ent_t *
fire_bfg(ent_t *self, vec3_t start, vec3_t dir)
{
	ent_t *bolt;

	vecnorm(dir);

	bolt = entspawn();
	bolt->classname = "bfg";
	bolt->nextthink = level.time + 10000;
	bolt->think = explodemissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_BFG;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashdmg = 100;
	bolt->splashradius = 120;
	bolt->meansofdeath = MOD_BFG;
	bolt->splashmeansofdeath = MOD_BFG_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = nil;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;	// move a bit on the very first frame
	veccopy(start, bolt->s.pos.trBase);
	vecscale(dir, 2000, bolt->s.pos.trDelta);
	SnapVector(bolt->s.pos.trDelta);	// save net bandwidth
	veccopy(start, bolt->r.currentOrigin);

	return bolt;
}

/*
=================
fire_rocket
=================
*/
ent_t *
fire_rocket(ent_t *self, vec3_t start, vec3_t dir)
{
	ent_t *bolt;

	vecnorm(dir);

	bolt = entspawn();
	bolt->classname = "rocket";
	bolt->nextthink = level.time + 15000;
	bolt->think = explodemissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_ROCKET_LAUNCHER;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashdmg = 100;
	bolt->splashradius = 120;
	bolt->meansofdeath = MOD_ROCKET;
	bolt->splashmeansofdeath = MOD_ROCKET_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = nil;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;	// move a bit on the very first frame
	veccopy(start, bolt->s.pos.trBase);
	vecscale(dir, 900, bolt->s.pos.trDelta);
	SnapVector(bolt->s.pos.trDelta);	// save net bandwidth
	veccopy(start, bolt->r.currentOrigin);

	return bolt;
}

/*
=================
fire_grapple
=================
*/
ent_t *
fire_grapple(ent_t *self, vec3_t start, vec3_t dir)
{
	ent_t *hook;

	vecnorm(dir);

	hook = entspawn();
	hook->classname = "hook";
	hook->nextthink = level.time + 10000;
	hook->think = weapon_hook_free;
	hook->s.eType = ET_MISSILE;
	hook->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	hook->s.weapon = WP_GRAPPLING_HOOK;
	hook->r.ownerNum = self->s.number;
	hook->meansofdeath = MOD_GRAPPLE;
	hook->clipmask = MASK_SHOT;
	hook->parent = self;
	hook->target_ent = nil;

	hook->s.pos.trType = TR_LINEAR;
	hook->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;	// move a bit on the very first frame
	hook->s.otherEntityNum = self->s.number;// use to match beam in client
	veccopy(start, hook->s.pos.trBase);
	vecscale(dir, 800, hook->s.pos.trDelta);
	SnapVector(hook->s.pos.trDelta);	// save net bandwidth
	veccopy(start, hook->r.currentOrigin);

	self->client->hook = hook;

	return hook;
}

