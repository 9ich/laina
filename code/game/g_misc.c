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
// g_misc.c

#include "g_local.h"

/*QUAKED func_group (0 0 0) ?
Used to group brushes together just for editor convenience.  They are turned into normal brushes by the utilities.
*/

/*QUAKED info_camp (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for calculations in the utilities (spotlights, etc), but removed during gameplay.
*/
void
SP_info_camp(ent_t *self)
{
	setorigin(self, self->s.origin);
}

/*QUAKED info_null (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for calculations in the utilities (spotlights, etc), but removed during gameplay.
*/
void
SP_info_null(ent_t *self)
{
	entfree(self);
}

/*QUAKED info_notnull (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for in-game calculation, like jumppad targets.
target_position does the same thing
*/
void
SP_info_notnull(ent_t *self)
{
	setorigin(self, self->s.origin);
}

/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) linear
Non-displayed light.
"light" overrides the default 300 intensity.
Linear checbox gives linear falloff instead of inverse square
Lights pointed at a target will be spotlights.
"radius" overrides the default 64 unit radius of a spotlight at the target point.
*/
void
SP_light(ent_t *self)
{
	entfree(self);
}

/*
=================================================================================

TELEPORTERS

=================================================================================
*/

void
teleportentity(ent_t *player, vec3_t origin, vec3_t angles)
{
	ent_t *tent;
	qboolean noAngles;

	noAngles = (angles[0] > 999999.0);
	// use temp events at source and destination to prevent the effect
	// from getting dropped by a second player event
	if(player->client->sess.team != TEAM_SPECTATOR){
		tent = enttemp(player->client->ps.origin, EV_PLAYER_TELEPORT_OUT);
		tent->s.clientNum = player->s.clientNum;

		tent = enttemp(origin, EV_PLAYER_TELEPORT_IN);
		tent->s.clientNum = player->s.clientNum;
	}

	// unlink to make sure it can't possibly interfere with killbox
	trap_UnlinkEntity(player);

	veccopy(origin, player->client->ps.origin);
	player->client->ps.origin[2] += 1;
	if(!noAngles){
		// spit the player out
		anglevecs(angles, player->client->ps.velocity, nil, nil);
		vecscale(player->client->ps.velocity, 400, player->client->ps.velocity);
		player->client->ps.pm_time = 160;	// hold time
		player->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		// set angles
		setviewangles(player, angles);
	}
	// toggle the teleport bit so the client knows to not lerp
	player->client->ps.eFlags ^= EF_TELEPORT_BIT;
	// kill anything at the destination
	if(player->client->sess.team != TEAM_SPECTATOR)
		killbox(player);

	// save results of pmove
	playerstate2entstate(&player->client->ps, &player->s, qtrue);

	// use the precise origin for linking
	veccopy(player->client->ps.origin, player->r.currentOrigin);

	if(player->client->sess.team != TEAM_SPECTATOR)
		trap_LinkEntity(player);
}

/*QUAKED misc_teleporter_dest (1 0 0) (-32 -32 -24) (32 32 -16)
Point teleporters at these.
Now that we don't have teleport destination pads, this is just
an info_notnull
*/
void
SP_misc_teleporter_dest(ent_t *ent)
{
}

//===========================================================

/*QUAKED misc_model (1 0 0) (-16 -16 -16) (16 16 16)
"model"		arbitrary .md3 file to display
*/
void
SP_misc_model(ent_t *ent)
{
#if 0
	ent->s.modelindex = modelindex(ent->model);
	vecset(ent->mins, -16, -16, -16);
	vecset(ent->maxs, 16, 16, 16);
	trap_LinkEntity(ent);

	setorigin(ent, ent->s.origin);
	veccopy(ent->s.angles, ent->s.apos.trBase);
#else
	entfree(ent);
#endif
}

//===========================================================

void
locateCamera(ent_t *ent)
{
	vec3_t dir;
	ent_t *target;
	ent_t *owner;

	owner = picktarget(ent->target);
	if(!owner){
		gprintf("Couldn't find target for misc_partal_surface\n");
		entfree(ent);
		return;
	}
	ent->r.ownerNum = owner->s.number;

	// frame holds the rotate speed
	if(owner->spawnflags & 1)
		ent->s.frame = 25;
	else if(owner->spawnflags & 2)
		ent->s.frame = 75;

	// swing camera ?
	if(owner->spawnflags & 4)
		// set to 0 for no rotation at all
		ent->s.powerups = 0;
	else
		ent->s.powerups = 1;

	// clientNum holds the rotate offset
	ent->s.clientNum = owner->s.clientNum;

	veccopy(owner->s.origin, ent->s.origin2);

	// see if the portal_camera has a target
	target = picktarget(owner->target);
	if(target){
		vecsub(target->s.origin, owner->s.origin, dir);
		vecnorm(dir);
	}else
		setmovedir(owner->s.angles, dir);

	ent->s.eventParm = DirToByte(dir);
}

/*QUAKED misc_portal_surface (0 0 1) (-8 -8 -8) (8 8 8)
The portal surface nearest this entity will show a view from the targeted misc_portal_camera, or a mirror view if untargeted.
This must be within 64 world units of the surface!
*/
void
SP_misc_portal_surface(ent_t *ent)
{
	vecclear(ent->r.mins);
	vecclear(ent->r.maxs);
	trap_LinkEntity(ent);

	ent->r.svFlags = SVF_PORTAL;
	ent->s.eType = ET_PORTAL;

	if(!ent->target)
		veccopy(ent->s.origin, ent->s.origin2);
	else{
		ent->think = locateCamera;
		ent->nextthink = level.time + 100;
	}
}

/*QUAKED misc_portal_camera (0 0 1) (-8 -8 -8) (8 8 8) slowrotate fastrotate noswing
The target for a misc_portal_director.  You can set either angles or target another entity to determine the direction of view.
"roll" an angle modifier to orient the camera around the target vector;
*/
void
SP_misc_portal_camera(ent_t *ent)
{
	float roll;

	vecclear(ent->r.mins);
	vecclear(ent->r.maxs);
	trap_LinkEntity(ent);

	spawnfloat("roll", "0", &roll);

	ent->s.clientNum = roll/360.0 * 256;
}

/*
======================================================================

  SHOOTERS

======================================================================
*/

void
Use_Shooter(ent_t *ent, ent_t *other, ent_t *activator)
{
	vec3_t dir;
	float deg;
	vec3_t up, right;

	// see if we have a target
	if(ent->enemy){
		vecsub(ent->enemy->r.currentOrigin, ent->s.origin, dir);
		vecnorm(dir);
	}else
		veccopy(ent->movedir, dir);

	// randomize a bit
	vecperp(up, dir);
	veccross(up, dir, right);

	deg = crandom() * ent->random;
	vecsadd(dir, deg, up, dir);

	deg = crandom() * ent->random;
	vecsadd(dir, deg, right, dir);

	vecnorm(dir);

	switch(ent->s.weapon){
	case WP_GRENADE_LAUNCHER:
		fire_grenade(ent, ent->s.origin, dir);
		break;
	case WP_ROCKET_LAUNCHER:
		fire_rocket(ent, ent->s.origin, dir);
		break;
	case WP_PLASMAGUN:
		fire_plasma(ent, ent->s.origin, dir);
		break;
	}

	addevent(ent, EV_FIRE_WEAPON, 0);
}

static void
InitShooter_Finish(ent_t *ent)
{
	ent->enemy = picktarget(ent->target);
	ent->think = 0;
	ent->nextthink = 0;
}

void
InitShooter(ent_t *ent, int weapon)
{
	ent->use = Use_Shooter;
	ent->s.weapon = weapon;

	registeritem(finditemforweapon(weapon));

	setmovedir(ent->s.angles, ent->movedir);

	if(!ent->random)
		ent->random = 1.0;
	ent->random = sin(M_PI * ent->random / 180);
	// target might be a moving object, so we can't set movedir for it
	if(ent->target){
		ent->think = InitShooter_Finish;
		ent->nextthink = level.time + 500;
	}
	trap_LinkEntity(ent);
}

/*QUAKED shooter_rocket (1 0 0) (-16 -16 -16) (16 16 16)
Fires at either the target or the current direction.
"random" the number of degrees of deviance from the taget. (1.0 default)
*/
void
SP_shooter_rocket(ent_t *ent)
{
	InitShooter(ent, WP_ROCKET_LAUNCHER);
}

/*QUAKED shooter_plasma (1 0 0) (-16 -16 -16) (16 16 16)
Fires at either the target or the current direction.
"random" is the number of degrees of deviance from the taget. (1.0 default)
*/
void
SP_shooter_plasma(ent_t *ent)
{
	InitShooter(ent, WP_PLASMAGUN);
}

/*QUAKED shooter_grenade (1 0 0) (-16 -16 -16) (16 16 16)
Fires at either the target or the current direction.
"random" is the number of degrees of deviance from the taget. (1.0 default)
*/
void
SP_shooter_grenade(ent_t *ent)
{
	InitShooter(ent, WP_GRENADE_LAUNCHER);
}

