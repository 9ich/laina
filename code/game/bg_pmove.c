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
// bg_pmove.c -- both games player movement code
// takes a playerstate and a usercmd as input and returns a modifed playerstate

#include "../qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

pmove_t *pm;
pml_t pml;

// movement parameters
float pm_stopspeed = 100.0f;
float pm_duckScale = 0.25f;
float pm_swimScale = 0.50f;

float pm_accelerate = 15.0f;
float pm_airaccelerate = 1.0f;
float pm_wateraccelerate = 4.0f;
float pm_flyaccelerate = 8.0f;

float pm_friction = 8.0f;
float pm_waterfriction = 1.0f;
float pm_flightfriction = 3.0f;
float pm_spectatorfriction = 5.0f;

float cpm_pm_airstopaccelerate = 2.5;
float cpm_pm_aircontrol = 150;
float cpm_pm_strafeaccelerate = 70;
float cpm_pm_wishspeed = 30;

int c_pmove = 0;

/*
===============
pmaddevent

===============
*/
void
pmaddevent(int newEvent)
{
	bgaddpredictableevent(newEvent, 0, pm->ps);
}

/*
===============
pmaddtouchent
===============
*/
void
pmaddtouchent(int entityNum)
{
	int i;

	if(entityNum == ENTITYNUM_WORLD)
		return;
	if(pm->numtouch == MAXTOUCH)
		return;

	// see if it is already added
	for(i = 0; i < pm->numtouch; i++)
		if(pm->touchents[i] == entityNum)
			return;

	// add it
	pm->touchents[pm->numtouch] = entityNum;
	pm->numtouch++;
}

/*
===================
starttorsoanim
===================
*/
static void
starttorsoanim(int anim)
{
	if(pm->ps->pm_type >= PM_DEAD)
		return;
	pm->ps->torsoAnim = ((pm->ps->torsoAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT)
			    | anim;
}

static void
startlegsanim(int anim)
{
	if(pm->ps->pm_type >= PM_DEAD)
		return;
	if(pm->ps->legsTimer > 0)
		return;	// a high priority animation is running
	pm->ps->legsAnim = ((pm->ps->legsAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT)
			   | anim;
}

static void
continuelegsanim(int anim)
{
	if((pm->ps->legsAnim & ~ANIM_TOGGLEBIT) == anim)
		return;
	if(pm->ps->legsTimer > 0)
		return;	// a high priority animation is running
	startlegsanim(anim);
}

static void
continuetorsoanim(int anim)
{
	if((pm->ps->torsoAnim & ~ANIM_TOGGLEBIT) == anim)
		return;
	if(pm->ps->torsoTimer > 0)
		return;	// a high priority animation is running
	starttorsoanim(anim);
}

static void
forcelegsanim(int anim)
{
	pm->ps->legsTimer = 0;
	startlegsanim(anim);
}

/*
==================
pmclipvel

Slide off of the impacting surface
==================
*/
void
pmclipvel(vec3_t in, vec3_t normal, vec3_t out, float overbounce)
{
	float backoff;
	float change;
	int i;

	backoff = vecdot(in, normal);

	if(backoff < 0)
		backoff *= overbounce;
	else
		backoff /= overbounce;

	for(i = 0; i<3; i++){
		change = normal[i]*backoff;
		out[i] = in[i] - change;
	}
}

/*
==================
friction

Handles both ground friction and water friction
==================
*/
static void
friction(void)
{
	vec3_t vec;
	float *vel;
	float speed, newspeed, control;
	float drop;

	vel = pm->ps->velocity;

	veccopy(vel, vec);
	if(pml.walking)
		vec[2] = 0;	// ignore slope movement

	speed = veclen(vec);
	if(speed < 1){
		vel[0] = 0;
		vel[1] = 0;	// allow sinking underwater
		// FIXME: still have z friction underwater?
		return;
	}

	drop = 0;

	// apply ground friction
	if(pm->waterlevel <= 1){
		if(pml.walking && !(pml.groundtrace.surfaceFlags & SURF_SLICK))
			// if getting knocked back, no friction
			if(!(pm->ps->pm_flags & PMF_TIME_KNOCKBACK)){
				control = speed < pm_stopspeed ? pm_stopspeed : speed;
				drop += control*pm_friction*pml.frametime;
			}
	}

	// apply water friction even if just wading
	if(pm->waterlevel)
		drop += speed*pm_waterfriction*pm->waterlevel*pml.frametime;

	// apply flying friction
	if(pm->ps->powerups[PW_FLIGHT])
		drop += speed*pm_flightfriction*pml.frametime;

	if(pm->ps->pm_type == PM_SPECTATOR)
		drop += speed*pm_spectatorfriction*pml.frametime;

	// scale the velocity
	newspeed = speed - drop;
	if(newspeed < 0)
		newspeed = 0;
	newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}

/*
==============
accelerate

Handles user intended acceleration
==============
*/
static void
accelerate(vec3_t wishdir, float wishspeed, float accel)
{
#if 1
	// q2 style
	int i;
	float addspeed, accelspeed, currentspeed;

	currentspeed = vecdot(pm->ps->velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if(addspeed <= 0)
		return;
	accelspeed = accel*pml.frametime*wishspeed;
	if(accelspeed > addspeed)
		accelspeed = addspeed;

	for(i = 0; i<3; i++)
		pm->ps->velocity[i] += accelspeed*wishdir[i];

#else
	// proper way (avoids strafe jump maxspeed bug), but feels bad
	vec3_t wishVelocity;
	vec3_t pushDir;
	float pushLen;
	float canPush;

	vecscale(wishdir, wishspeed, wishVelocity);
	vecsub(wishVelocity, pm->ps->velocity, pushDir);
	pushLen = vecnorm(pushDir);

	canPush = accel*pml.frametime*wishspeed;
	if(canPush > pushLen)
		canPush = pushLen;

	vecsadd(pm->ps->velocity, canPush, pushDir, pm->ps->velocity);
#endif
}

/*
============
cmdscale

Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrt(2) distortion in speed.
============
*/
static float
cmdscale(usercmd_t *cmd)
{
	int max;
	float total;
	float scale;

	max = abs(cmd->forwardmove);
	if(abs(cmd->rightmove) > max)
		max = abs(cmd->rightmove);
	if(abs(cmd->upmove) > max)
		max = abs(cmd->upmove);
	if(!max)
		return 0;

	total = sqrt(cmd->forwardmove * cmd->forwardmove
		     + cmd->rightmove * cmd->rightmove + cmd->upmove * cmd->upmove);
	scale = (float)pm->ps->speed * max / (127.0 * total);

	return scale;
}

/*
================
setmovementdir

Determine the rotation of the legs relative
to the facing dir
================
*/
static void
setmovementdir(void)
{
	if(pm->cmd.forwardmove || pm->cmd.rightmove){
		if(pm->cmd.rightmove == 0 && pm->cmd.forwardmove > 0)
			pm->ps->movementDir = 0;
		else if(pm->cmd.rightmove < 0 && pm->cmd.forwardmove > 0)
			pm->ps->movementDir = 1;
		else if(pm->cmd.rightmove < 0 && pm->cmd.forwardmove == 0)
			pm->ps->movementDir = 2;
		else if(pm->cmd.rightmove < 0 && pm->cmd.forwardmove < 0)
			pm->ps->movementDir = 3;
		else if(pm->cmd.rightmove == 0 && pm->cmd.forwardmove < 0)
			pm->ps->movementDir = 4;
		else if(pm->cmd.rightmove > 0 && pm->cmd.forwardmove < 0)
			pm->ps->movementDir = 5;
		else if(pm->cmd.rightmove > 0 && pm->cmd.forwardmove == 0)
			pm->ps->movementDir = 6;
		else if(pm->cmd.rightmove > 0 && pm->cmd.forwardmove > 0)
			pm->ps->movementDir = 7;
	}else{
		// if they aren't actively going directly sideways,
		// change the animation to the diagonal so they
		// don't stop too crooked
		if(pm->ps->movementDir == 2)
			pm->ps->movementDir = 1;
		else if(pm->ps->movementDir == 6)
			pm->ps->movementDir = 7;
	}
}

/*
=============
checkjump
=============
*/
static qboolean
checkjump(void)
{
	if(pm->ps->pm_flags & PMF_RESPAWNED)
		return qfalse;		// don't allow jump until all buttons are up
	if(pm->cmd.upmove < 10)
		return qfalse;		// not holding jump

	pml.groundplane = qfalse;	// jumping away
	pml.walking = qfalse;
	pm->ps->pm_flags |= PMF_JUMP_HELD;

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pm->ps->headEntityNum = ENTITYNUM_NONE;
	pm->ps->velocity[2] = JUMP_VELOCITY;
	pmaddevent(EV_JUMP);

	if(pm->cmd.forwardmove >= 0){
		forcelegsanim(LEGS_JUMP);
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	}else{
		forcelegsanim(LEGS_JUMPB);
		pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
	}

	return qtrue;
}

// double jumps
static qboolean
checkairjump(void)
{
	int i;
	float scale;

	if(pm->ps->nAirjumps >= 1 || pm->cmd.upmove < 10)
		return qfalse;
	// must wait for jump to be released
	if(pm->ps->pm_flags & PMF_JUMP_HELD){
		// clear upmove so cmdscale doesn't lower move speed
		pm->cmd.upmove = 0;
		return qfalse;
	}

	pm->ps->pm_flags |= PMF_JUMP_HELD;

	// project jump direction to ground plane
	pml.forward[2] = 0;
	pml.right[2] = 0;
	vecnorm(pml.forward);
	vecnorm(pml.right);

	scale = cmdscale(&pm->cmd);
	// kick the player in the dir they want
	for(i = 0; i<2; i++)
		pm->ps->velocity[i] = 2*scale*pml.forward[i]*pm->cmd.forwardmove + 2*scale*pml.right[i]*pm->cmd.rightmove;
	pm->ps->velocity[2] = AIRJUMP_VELOCITY;
	pm->ps->nAirjumps++;
	pmaddevent(EV_AIRJUMP);

	if(pm->cmd.forwardmove >= 0){
		forcelegsanim(LEGS_JUMP);
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	}else{
		forcelegsanim(LEGS_JUMPB);
		pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
	}
	return qtrue;
}

/*
=============
checkwaterjump
=============
*/
static qboolean
checkwaterjump(void)
{
	vec3_t spot;
	int cont;
	vec3_t flatforward;

	if(pm->ps->pm_time)
		return qfalse;

	// check for water jump
	if(pm->waterlevel != 2)
		return qfalse;

	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	vecnorm(flatforward);

	vecsadd(pm->ps->origin, 30, flatforward, spot);
	spot[2] += 4;
	cont = pm->pointcontents(spot, pm->ps->clientNum);
	if(!(cont & CONTENTS_SOLID))
		return qfalse;

	spot[2] += 16;
	cont = pm->pointcontents(spot, pm->ps->clientNum);
	if(cont & (CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY))
		return qfalse;

	// jump out of water
	vecscale(pml.forward, 200, pm->ps->velocity);
	pm->ps->velocity[2] = 350;

	pm->ps->pm_flags |= PMF_TIME_WATERJUMP;
	pm->ps->pm_time = 2000;

	return qtrue;
}

/*
===================
waterjumpmove

Flying out of the water
===================
*/
static void
waterjumpmove(void)
{
	// waterjump has no control, but falls

	pmstepslidemove(qtrue);

	pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	if(pm->ps->velocity[2] < 0){
		// cancel as soon as we are falling down again
		pm->ps->pm_flags &= ~PMF_ALL_TIMES;
		pm->ps->pm_time = 0;
	}
}

/*
===================
watermove

===================
*/
static void
watermove(void)
{
	int i;
	vec3_t wishvel;
	float wishspeed;
	vec3_t wishdir;
	float scale;
	float vel;

	if(checkwaterjump()){
		waterjumpmove();
		return;
	}
#if 0
	// jump = head for surface
	if(pm->cmd.upmove >= 10)
		if(pm->ps->velocity[2] > -300){
			if(pm->watertype == CONTENTS_WATER)
				pm->ps->velocity[2] = 100;
			else if(pm->watertype == CONTENTS_SLIME)
				pm->ps->velocity[2] = 80;
			else
				pm->ps->velocity[2] = 50;
		}

#endif
	friction();

	scale = cmdscale(&pm->cmd);
	// user intentions
	if(!scale){
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = -60;	// sink towards bottom
	}else{
		for(i = 0; i<3; i++)
			wishvel[i] = scale * pml.forward[i]*pm->cmd.forwardmove + scale * pml.right[i]*pm->cmd.rightmove;

		wishvel[2] += scale * pm->cmd.upmove;
	}

	veccopy(wishvel, wishdir);
	wishspeed = vecnorm(wishdir);

	if(wishspeed > pm->ps->speed * pm_swimScale)
		wishspeed = pm->ps->speed * pm_swimScale;

	accelerate(wishdir, wishspeed, pm_wateraccelerate);

	// make sure we can go up slopes easily under water
	if(pml.groundplane && vecdot(pm->ps->velocity, pml.groundtrace.plane.normal) < 0){
		vel = veclen(pm->ps->velocity);
		// slide along the ground plane
		pmclipvel(pm->ps->velocity, pml.groundtrace.plane.normal,
				pm->ps->velocity, OVERCLIP);

		vecnorm(pm->ps->velocity);
		vecscale(pm->ps->velocity, vel, pm->ps->velocity);
	}

	pmslidemode(qfalse);
}


/*
===================
flymove

Only with the flight powerup
===================
*/
static void
flymove(void)
{
	int i;
	vec3_t wishvel;
	float wishspeed;
	vec3_t wishdir;
	float scale;

	// normal slowdown
	friction();

	scale = cmdscale(&pm->cmd);
	// user intentions
	if(!scale){
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = 0;
	}else{
		for(i = 0; i<3; i++)
			wishvel[i] = scale * pml.forward[i]*pm->cmd.forwardmove + scale * pml.right[i]*pm->cmd.rightmove;

		wishvel[2] += scale * pm->cmd.upmove;
	}

	veccopy(wishvel, wishdir);
	wishspeed = vecnorm(wishdir);

	accelerate(wishdir, wishspeed, pm_flyaccelerate);

	pmstepslidemove(qfalse);
}

static void
aircontrol(pmove_t *pm, vec3_t wishdir, float wishspeed)
{
	float zspeed, speed, dot, k;
	int i;

	speed = veclen(pm->ps->velocity);
	// the player will sharply slow down in mid-air if all movement keys are released.
	// note that this also has an effect if the player swaps turn direction mid-airstrafe.
	if(wishspeed < 0.000001 && speed > 0.000001){
		vec_t newspeed;

		newspeed = speed - (MAX(speed, pm_stopspeed)*pm_friction*pml.frametime);
		newspeed = MAX(newspeed, 0.0f);
		newspeed /= speed;
		pm->ps->velocity[0] *= newspeed;
		pm->ps->velocity[1] *= newspeed;
		return;
	}
	if((pm->ps->movementDir && pm->ps->movementDir != 4 && pm->ps->movementDir != -4 && pm->ps->movementDir != 12) || wishspeed == 0.0f)
		return;		// can't control movement if not moving forward or backward
	zspeed = pm->ps->velocity[2];
	pm->ps->velocity[2] = 0;
	speed = vecnorm(pm->ps->velocity);
	dot = vecdot(pm->ps->velocity, wishdir);
	k = 32;
	k *= cpm_pm_aircontrol*dot*dot*pml.frametime;
	if(dot > 0){	// can't change direction while slowing down
		for(i = 0; i < 2; i++)
			pm->ps->velocity[i] = pm->ps->velocity[i]*speed + wishdir[i]*k;
		vecnorm(pm->ps->velocity);
	}
	for(i = 0; i < 2; i++)
		pm->ps->velocity[i] *= speed;
	pm->ps->velocity[2] = zspeed;
}

/*
===================
airmove

===================
*/
static void
airmove(void)
{
	int i;
	vec3_t wishvel;
	float fmove, smove;
	vec3_t wishdir;
	float wishspeed;
	float scale;
	float accel;
	float wishspeed2;
	usercmd_t cmd;

	// set the movementDir so clients can rotate the legs for strafing
	setmovementdir();

	checkairjump();

	friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = cmdscale(&cmd);

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;
	vecnorm(pml.forward);
	vecnorm(pml.right);

	for(i = 0; i < 2; i++)
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	wishvel[2] = 0;

	veccopy(wishvel, wishdir);
	wishspeed = vecnorm(wishdir);
	wishspeed *= scale;

	wishspeed2 = wishspeed;
	if(vecdot(pm->ps->velocity, wishdir) < 0)
		accel = cpm_pm_airstopaccelerate;
	else
		accel = pm_airaccelerate;
	if((pm->ps->movementDir == 2 || pm->ps->movementDir == -2 || pm->ps->movementDir == 10) ||
	   (pm->ps->movementDir == 6 || pm->ps->movementDir == -6 || pm->ps->movementDir == 14)){
		if(wishspeed > cpm_pm_wishspeed)
			wishspeed = cpm_pm_wishspeed;
		accel = cpm_pm_strafeaccelerate;
	}

	// not on ground, so little effect on velocity
	accelerate(wishdir, wishspeed, accel);
	aircontrol(pm, wishdir, wishspeed2);

	// we may have a ground plane that is very steep, even
	// though we don't have a groundentity
	// slide along the steep plane
	if(pml.groundplane)
		pmclipvel(pm->ps->velocity, pml.groundtrace.plane.normal,
				pm->ps->velocity, OVERCLIP);

#if 0
	//ZOID:  If we are on the grapple, try stair-stepping
	//this allows a player to use the grapple to pull himself
	//over a ledge
	if(pm->ps->pm_flags & PMF_GRAPPLE_PULL)
		pmstepslidemove(qtrue);
	else
		pmslidemode(qtrue);
#endif

	pmstepslidemove(qtrue);
}

/*
===================
grapplemove

===================
*/
static void
grapplemove(void)
{
	vec3_t vel, v;
	float vlen;

	vecscale(pml.forward, -16, v);
	vecadd(pm->ps->grapplePoint, v, v);
	vecsub(v, pm->ps->origin, vel);
	vlen = veclen(vel);
	vecnorm(vel);

	if(vlen <= 100)
		vecscale(vel, 10 * vlen, vel);
	else
		vecscale(vel, 800, vel);

	veccopy(vel, pm->ps->velocity);

	pml.groundplane = qfalse;
}

/*
===================
walkmove

===================
*/
static void
walkmove(void)
{
	int i;
	vec3_t wishvel;
	float fmove, smove;
	vec3_t wishdir;
	float wishspeed;
	float scale;
	usercmd_t cmd;
	float accel;
	float vel;

	if(pm->waterlevel > 2 && vecdot(pml.forward, pml.groundtrace.plane.normal) > 0){
		// begin swimming
		watermove();
		return;
	}

	if(checkjump()){
		// jumped away
		if(pm->waterlevel > 1)
			watermove();
		else
			airmove();
		return;
	}

	friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = cmdscale(&cmd);

	// set the movementDir so clients can rotate the legs for strafing
	setmovementdir();

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;

	// project the forward and right directions onto the ground plane
	pmclipvel(pml.forward, pml.groundtrace.plane.normal, pml.forward, OVERCLIP);
	pmclipvel(pml.right, pml.groundtrace.plane.normal, pml.right, OVERCLIP);
	vecnorm(pml.forward);
	vecnorm(pml.right);

	for(i = 0; i < 3; i++)
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	// when going up or down slopes the wish velocity should Not be zero
//	wishvel[2] = 0;

	veccopy(wishvel, wishdir);
	wishspeed = vecnorm(wishdir);
	wishspeed *= scale;

	// clamp the speed lower if ducking
	if(pm->ps->pm_flags & PMF_DUCKED)
		if(wishspeed > pm->ps->speed * pm_duckScale)
			wishspeed = pm->ps->speed * pm_duckScale;

	// clamp the speed lower if wading or walking on the bottom
	if(pm->waterlevel){
		float waterScale;

		waterScale = pm->waterlevel / 3.0;
		waterScale = 1.0 - (1.0 - pm_swimScale) * waterScale;
		if(wishspeed > pm->ps->speed * waterScale)
			wishspeed = pm->ps->speed * waterScale;
	}

	// when a player gets hit, they temporarily lose
	// full control, which allows them to be moved a bit
	if((pml.groundtrace.surfaceFlags & SURF_SLICK) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK)
		accel = pm_airaccelerate;
	else
		accel = pm_accelerate;

	accelerate(wishdir, wishspeed, accel);

	//Com_Printf("velocity = %1.1f %1.1f %1.1f\n", pm->ps->velocity[0], pm->ps->velocity[1], pm->ps->velocity[2]);
	//Com_Printf("velocity1 = %1.1f\n", veclen(pm->ps->velocity));

	if((pml.groundtrace.surfaceFlags & SURF_SLICK) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK)
		pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	else{
		// don't reset the z velocity for slopes
//		pm->ps->velocity[2] = 0;
	}

	vel = veclen(pm->ps->velocity);

	// slide along the ground plane
	pmclipvel(pm->ps->velocity, pml.groundtrace.plane.normal,
			pm->ps->velocity, OVERCLIP);

	// don't decrease velocity when going up or down a slope
	vecnorm(pm->ps->velocity);
	vecscale(pm->ps->velocity, vel, pm->ps->velocity);

	// don't do anything if standing still
	if(!pm->ps->velocity[0] && !pm->ps->velocity[1])
		return;

	pmstepslidemove(qfalse);

	//Com_Printf("velocity2 = %1.1f\n", veclen(pm->ps->velocity));
}

/*
==============
deadmove
==============
*/
static void
deadmove(void)
{
	float forward;

	if(!pml.walking)
		return;

	// extra friction

	forward = veclen(pm->ps->velocity);
	forward -= 20;
	if(forward <= 0)
		vecclear(pm->ps->velocity);
	else{
		vecnorm(pm->ps->velocity);
		vecscale(pm->ps->velocity, forward, pm->ps->velocity);
	}
}

/*
===============
noclipmove
===============
*/
static void
noclipmove(void)
{
	float speed, drop, friction, control, newspeed;
	int i;
	vec3_t wishvel;
	float fmove, smove;
	vec3_t wishdir;
	float wishspeed;
	float scale;

	pm->ps->viewheight = DEFAULT_VIEWHEIGHT;

	// friction

	speed = veclen(pm->ps->velocity);
	if(speed < 1)
		veccopy(vec3_origin, pm->ps->velocity);
	else{
		drop = 0;

		friction = pm_friction*1.5;	// extra friction
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control*friction*pml.frametime;

		// scale the velocity
		newspeed = speed - drop;
		if(newspeed < 0)
			newspeed = 0;
		newspeed /= speed;

		vecscale(pm->ps->velocity, newspeed, pm->ps->velocity);
	}

	// accelerate
	scale = cmdscale(&pm->cmd);

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	for(i = 0; i<3; i++)
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	wishvel[2] += pm->cmd.upmove;

	veccopy(wishvel, wishdir);
	wishspeed = vecnorm(wishdir);
	wishspeed *= scale;

	accelerate(wishdir, wishspeed, pm_accelerate);

	// move
	vecsadd(pm->ps->origin, pml.frametime, pm->ps->velocity, pm->ps->origin);
}

/*
================
footstepforsurface

Returns an event number apropriate for the groundsurface
================
*/
static int
footstepforsurface(void)
{
	if(pml.groundtrace.surfaceFlags & SURF_NOSTEPS)
		return 0;
	if(pml.groundtrace.surfaceFlags & SURF_METALSTEPS)
		return EV_FOOTSTEP_METAL;
	return EV_FOOTSTEP;
}

/*
=================
crashland

Check for hard landings that generate sound events
=================
*/
static void
crashland(void)
{
	float delta;
	float dist;
	float vel, acc;
	float t;
	float a, b, c, den;

	// decide which landing animation to use
	if(pm->ps->pm_flags & PMF_BACKWARDS_JUMP)
		forcelegsanim(LEGS_LANDB);
	else
		forcelegsanim(LEGS_LAND);

	pm->ps->legsTimer = TIMER_LAND;

	// calculate the exact velocity on landing
	dist = pm->ps->origin[2] - pml.prevorigin[2];
	vel = pml.prevvelocity[2];
	acc = -pm->ps->gravity;

	a = acc / 2;
	b = vel;
	c = -dist;

	den = b * b - 4 * a * c;
	if(den < 0)
		return;
	t = (-b - sqrt(den)) / (2 * a);

	delta = vel + t * acc;
	delta = delta*delta * 0.0001;

	// ducking while falling doubles damage
	if(pm->ps->pm_flags & PMF_DUCKED)
		delta *= 2;

	// never take falling damage if completely underwater
	if(pm->waterlevel == 3)
		return;

	// reduce falling damage if there is standing water
	if(pm->waterlevel == 2)
		delta *= 0.25;
	if(pm->waterlevel == 1)
		delta *= 0.5;

	if(delta < 1)
		return;

	// create a local entity event to play the sound

	// SURF_NODAMAGE is used for bounce pads where you don't ever
	// want to take damage or play a crunch sound
	if(!(pml.groundtrace.surfaceFlags & SURF_NODAMAGE)){
		if(delta > 60){
			pmaddevent(EV_FALL_FAR);
			pm->ps->crashland = qtrue;
		}else if(delta > 40){
			// this is a pain grunt, so don't play it if dead
			if(pm->ps->stats[STAT_HEALTH] > 0)
				pmaddevent(EV_FALL_MEDIUM);
			pm->ps->crashland = qtrue;
		}else if(delta > 7){
			pmaddevent(EV_FALL_SHORT);
			pm->ps->crashland = qtrue;
		}else
			pmaddevent(footstepforsurface());
	}

	// start footstep cycle over
	pm->ps->bobCycle = 0;
}

/*
=============
checkstuck
=============
*/
/*
void checkstuck(void){
        trace_t trace;

        pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, pm->ps->origin, pm->ps->clientNum, pm->tracemask);
        if(trace.allsolid){
                //int shit = qtrue;
        }
}
*/

/*
=============
correctallsolid
=============
*/
static int
correctallsolid(trace_t *trace)
{
	int i, j, k;
	vec3_t point;

	if(pm->debuglevel)
		Com_Printf("%i:allsolid\n", c_pmove);

	// jitter around
	for(i = -1; i <= 1; i++){
		for(j = -1; j <= 1; j++)
			for(k = -1; k <= 1; k++){
				veccopy(pm->ps->origin, point);
				point[0] += (float)i;
				point[1] += (float)j;
				point[2] += (float)k;
				pm->trace(trace, point, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
				if(!trace->allsolid){
					point[0] = pm->ps->origin[0];
					point[1] = pm->ps->origin[1];
					point[2] = pm->ps->origin[2] - 0.25;

					pm->trace(trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
					pml.groundtrace = *trace;
					return qtrue;
				}
			}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundplane = qfalse;
	pml.walking = qfalse;

	return qfalse;
}

static int
correctheadallsolid(trace_t *trace)
{
	int i, j, k;
	vec3_t point;

	if(pm->debuglevel)
		Com_Printf("%i:head allsolid\n", c_pmove);
	// jitter around
	for(i = 1; i >= -1; i--){
		for(j = 1; j >= -1; j--)
			for(k = 1; k >= -1; k--){
				veccopy(pm->ps->origin, point);
				point[0] -= (float)i;
				point[1] -= (float)j;
				point[2] -= (float)k;
				pm->trace(trace, point, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
				if(!trace->allsolid){
					point[0] = pm->ps->origin[0];
					point[1] = pm->ps->origin[1];
					point[2] = pm->ps->origin[2] + 0.25;
					pm->trace(trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
					return qtrue;
				}
			}
	}
	pm->ps->headEntityNum = ENTITYNUM_NONE;
	return qfalse;
}

/*
=============
groundtracemissed

The ground trace didn't hit a surface, so we are in freefall
=============
*/
static void
groundtracemissed(void)
{
	trace_t trace;
	vec3_t point;

	if(pm->ps->groundEntityNum != ENTITYNUM_NONE){
		// we just transitioned into freefall
		if(pm->debuglevel)
			Com_Printf("%i:lift\n", c_pmove);

		// if they aren't in a jumping animation and the ground is a ways away, force into it
		// if we didn't do the trace, the player would be backflipping down staircases
		veccopy(pm->ps->origin, point);
		point[2] -= 64;

		pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
		if(trace.fraction == 1.0){
			if(pm->cmd.forwardmove >= 0){
				forcelegsanim(LEGS_JUMP);
				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			}else{
				forcelegsanim(LEGS_JUMPB);
				pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundplane = qfalse;
	pml.walking = qfalse;
}

static void
headtracemissed(void)
{
	if(pm->ps->headEntityNum != ENTITYNUM_NONE)
		// falling from entity
		if(pm->debuglevel)
			Com_Printf("%i:head away\n", c_pmove);
	pm->ps->headEntityNum = ENTITYNUM_NONE;
}

/*
=============
groundtrace
=============
*/
static void
groundtrace(void)
{
	vec3_t point;
	trace_t trace;

	pm->ps->crashland = qfalse;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] - 0.25;

	pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
	pml.groundtrace = trace;

	// do something corrective if the trace starts in a solid...
	if(trace.allsolid)
		if(!correctallsolid(&trace))
			return;

	// if the trace didn't hit anything, we are in free fall
	if(trace.fraction == 1.0){
		groundtracemissed();
		pml.groundplane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// check if getting thrown off the ground
	if(pm->ps->velocity[2] > 0 && vecdot(pm->ps->velocity, trace.plane.normal) > 10){
		if(pm->debuglevel)
			Com_Printf("%i:kickoff\n", c_pmove);
		// go into jump animation
		if(pm->cmd.forwardmove >= 0){
			forcelegsanim(LEGS_JUMP);
			pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
		}else{
			forcelegsanim(LEGS_JUMPB);
			pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
		}

		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundplane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// slopes that are too steep will not be considered onground
	if(trace.plane.normal[2] < MIN_WALK_NORMAL){
		if(pm->debuglevel)
			Com_Printf("%i:steep\n", c_pmove);
		// FIXME: if they can't slide down the slope, let them
		// walk (sharp crevices)
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundplane = qtrue;
		pml.walking = qfalse;
		return;
	}

	pml.groundplane = qtrue;
	pml.walking = qtrue;

	// hitting solid ground will end a waterjump
	if(pm->ps->pm_flags & PMF_TIME_WATERJUMP){
		pm->ps->pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND);
		pm->ps->pm_time = 0;
	}

	if(pm->ps->groundEntityNum == ENTITYNUM_NONE){
		if(pm->debuglevel)
			Com_Printf("%i:land\n", c_pmove);

		crashland();
		pm->ps->nAirjumps = 0;

		// don't do landing time if we were just going down a slope
		if(pml.prevvelocity[2] < -200){
			// don't allow another jump for a little while
			pm->ps->pm_flags |= PMF_TIME_LAND;
			pm->ps->pm_time = 250;
		}
	}

	pm->ps->groundEntityNum = trace.entityNum;

	// don't reset the z velocity for slopes
//	pm->ps->velocity[2] = 0;

	pmaddtouchent(trace.entityNum);
}

// for headbutting things. stands to reason, dunnit?
static void
headtrace(void)
{
	vec3_t point;
	trace_t trace;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] + 0.25;

	pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);

	// do something corrective if the trace starts in a solid...
	if(trace.allsolid)
		if(!correctheadallsolid(&trace))
			return;
	if(trace.fraction == 1.0){
		headtracemissed();
		return;
	}

	Com_Printf("%i:head contact?\n", c_pmove);

	// check if falling away
	if(pm->ps->velocity[2] < 0 && vecdot(pm->ps->velocity, trace.plane.normal) > 10){
		if(pm->debuglevel)
			Com_Printf("%i:headoff\n", c_pmove);
		pm->ps->headEntityNum = ENTITYNUM_NONE;
		return;
	}
	// slopes that are too steep will not be considered headbuttable
	if(trace.plane.normal[2] < MIN_WALK_NORMAL){
		if(pm->debuglevel)
			Com_Printf("%i:head steep\n", c_pmove);
		pm->ps->headEntityNum = ENTITYNUM_NONE;
		return;
	}
	if(pm->ps->headEntityNum == ENTITYNUM_NONE)
		// just hit the thing
		if(pm->debuglevel)
			Com_Printf("%i:headbutt\n", c_pmove);
	pm->ps->headEntityNum = trace.entityNum;
	pmaddtouchent(trace.entityNum);
}

/*
=============
setwaterlevel	fixme: avoid this twice?  certainly if not moving
=============
*/
static void
setwaterlevel(void)
{
	vec3_t point;
	int cont;
	int sample1;
	int sample2;

	// get waterlevel, accounting for ducking
	pm->waterlevel = 0;
	pm->watertype = 0;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] + MINS_Z + 1;
	cont = pm->pointcontents(point, pm->ps->clientNum);

	if(cont & MASK_WATER){
		sample2 = pm->ps->viewheight - MINS_Z;
		sample1 = sample2 / 2;

		pm->watertype = cont;
		pm->waterlevel = 1;
		point[2] = pm->ps->origin[2] + MINS_Z + sample1;
		cont = pm->pointcontents(point, pm->ps->clientNum);
		if(cont & MASK_WATER){
			pm->waterlevel = 2;
			point[2] = pm->ps->origin[2] + MINS_Z + sample2;
			cont = pm->pointcontents(point, pm->ps->clientNum);
			if(cont & MASK_WATER)
				pm->waterlevel = 3;
		}
	}
}

/*
==============
checkduck

Sets mins, maxs, and pm->ps->viewheight
==============
*/
static void
checkduck(void)
{
	trace_t trace;

	pm->mins[0] = MINS_X;
	pm->mins[1] = MINS_Y;

	pm->maxs[0] = MAXS_X;
	pm->maxs[1] = MAXS_Y;

	pm->mins[2] = MINS_Z;

	if(pm->ps->pm_type == PM_DEAD){
		pm->maxs[2] = -8;
		pm->ps->viewheight = DEAD_VIEWHEIGHT;
		return;
	}

	if(pm->cmd.upmove < 0)
		// duck
		pm->ps->pm_flags |= PMF_DUCKED;
	else
	// stand up if possible
	if(pm->ps->pm_flags & PMF_DUCKED){
		// try to stand up
		pm->maxs[2] = MAXS_Z;
		pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, pm->ps->origin, pm->ps->clientNum, pm->tracemask);
		if(!trace.allsolid)
			pm->ps->pm_flags &= ~PMF_DUCKED;
	}

	if(pm->ps->pm_flags & PMF_DUCKED){
		pm->maxs[2] = CROUCH_MAXS_Z;
		pm->ps->viewheight = CROUCH_VIEWHEIGHT;
	}else{
		pm->maxs[2] = MAXS_Z;
		pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
	}
}

//===================================================================

/*
===============
footsteps
===============
*/
static void
footsteps(void)
{
	float bobmove;
	int old;
	qboolean footstep;

	// calculate speed and cycle to be used for
	// all cyclic walking effects
	pm->xyspeed = sqrt(pm->ps->velocity[0] * pm->ps->velocity[0]
			   +  pm->ps->velocity[1] * pm->ps->velocity[1]);

	if(pm->ps->groundEntityNum == ENTITYNUM_NONE){
		if(pm->ps->powerups[PW_INVULNERABILITY])
			continuelegsanim(LEGS_IDLECR);
		// airborne leaves position in cycle intact, but doesn't advance
		if(pm->waterlevel > 1)
			continuelegsanim(LEGS_SWIM);
		return;
	}

	// if not trying to move
	if(!pm->cmd.forwardmove && !pm->cmd.rightmove){
		if(pm->xyspeed < 5){
			pm->ps->bobCycle = 0;	// start at beginning of cycle again
			if(pm->ps->pm_flags & PMF_DUCKED)
				continuelegsanim(LEGS_IDLECR);
			else
				continuelegsanim(LEGS_IDLE);
		}
		return;
	}

	footstep = qfalse;

	if(pm->ps->pm_flags & PMF_DUCKED){
		bobmove = 0.5;	// ducked characters bob much faster
		if(pm->ps->pm_flags & PMF_BACKWARDS_RUN)
			continuelegsanim(LEGS_BACKCR);
		else
			continuelegsanim(LEGS_WALKCR);
		// ducked characters never play footsteps
		/*
		}else   if( pm->ps->pm_flags & PMF_BACKWARDS_RUN ){
		        if( !( pm->cmd.buttons & BUTTON_WALKING ) ){
		                bobmove = 0.4;	// faster speeds bob faster
		                footstep = qtrue;
		        }else{
		                bobmove = 0.3;
		        }
		        continuelegsanim( LEGS_BACK );
		*/
	}else{
		if(!(pm->cmd.buttons & BUTTON_WALKING)){
			bobmove = 0.4f;	// faster speeds bob faster
			if(pm->ps->pm_flags & PMF_BACKWARDS_RUN)
				continuelegsanim(LEGS_BACK);
			else
				continuelegsanim(LEGS_RUN);
			footstep = qtrue;
		}else{
			bobmove = 0.3f;	// walking bobs slow
			if(pm->ps->pm_flags & PMF_BACKWARDS_RUN)
				continuelegsanim(LEGS_BACKWALK);
			else
				continuelegsanim(LEGS_WALK);
		}
	}

	// check for footstep / splash sounds
	old = pm->ps->bobCycle;
	pm->ps->bobCycle = (int)(old + bobmove * pml.msec) & 255;

	// if we just crossed a cycle boundary, play an apropriate footstep event
	if(((old + 64) ^ (pm->ps->bobCycle + 64)) & 128){
		if(pm->waterlevel == 0){
			// on ground will only play sounds if running
			if(footstep && !pm->nofootsteps)
				pmaddevent(footstepforsurface());
		}else if(pm->waterlevel == 1)
			// splashing
			pmaddevent(EV_FOOTSPLASH);
		else if(pm->waterlevel == 2)
			// wading / swimming at surface
			pmaddevent(EV_SWIM);
		else if(pm->waterlevel == 3){
			// no sound when completely underwater
		}
	}
}

/*
==============
waterevents

Generate sound events for entering and leaving water
==============
*/
static void
waterevents(void)	// FIXME?
{
	// if just entered a water volume, play a sound
	if(!pml.prevwaterlevel && pm->waterlevel)
		pmaddevent(EV_WATER_TOUCH);

	// if just completely exited a water volume, play a sound
	if(pml.prevwaterlevel && !pm->waterlevel)
		pmaddevent(EV_WATER_LEAVE);

	// check for head just going under water
	if(pml.prevwaterlevel != 3 && pm->waterlevel == 3)
		pmaddevent(EV_WATER_UNDER);

	// check for head just coming out of water
	if(pml.prevwaterlevel == 3 && pm->waterlevel != 3)
		pmaddevent(EV_WATER_CLEAR);
}

/*
===============
beginweaponchange
===============
*/
static void
beginweaponchange(int weapon)
{
	if(weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS)
		return;

	if(!(pm->ps->stats[STAT_WEAPONS] & (1 << weapon)))
		return;

	if(pm->ps->weaponstate == WEAPON_DROPPING)
		return;

	pmaddevent(EV_CHANGE_WEAPON);
	pm->ps->weaponstate = WEAPON_DROPPING;
	pm->ps->weaponTime += 200;
	starttorsoanim(TORSO_DROP);
}

/*
===============
finishweaponchange
===============
*/
static void
finishweaponchange(void)
{
	int weapon;

	weapon = pm->cmd.weapon;
	if(weapon < WP_NONE || weapon >= WP_NUM_WEAPONS)
		weapon = WP_NONE;

	if(!(pm->ps->stats[STAT_WEAPONS] & (1 << weapon)))
		weapon = WP_NONE;

	pm->ps->weapon = weapon;
	pm->ps->weaponstate = WEAPON_RAISING;
	pm->ps->weaponTime += 250;
	starttorsoanim(TORSO_RAISE);
}

/*
==============
torsoanimation

==============
*/
static void
torsoanimation(void)
{
	if(pm->ps->weaponstate == WEAPON_READY){
		if(pm->ps->weapon == WP_GAUNTLET)
			continuetorsoanim(TORSO_STAND2);
		else
			continuetorsoanim(TORSO_STAND);
		return;
	}
}

/*
==============
weapon

Generates weapon events and modifes the weapon counter
==============
*/
static void
weapon(void)
{
	int addTime;

	// don't allow attack until all buttons are up
	if(pm->ps->pm_flags & PMF_RESPAWNED)
		return;

	// ignore if spectator
	if(pm->ps->persistant[PERS_TEAM] == TEAM_SPECTATOR)
		return;

	// check for dead player
	if(pm->ps->stats[STAT_HEALTH] <= 0){
		pm->ps->weapon = WP_NONE;
		return;
	}

	// check for item using
	if(pm->cmd.buttons & BUTTON_USE_HOLDABLE){
		if(!(pm->ps->pm_flags & PMF_USE_ITEM_HELD)){
			pm->ps->pm_flags |= PMF_USE_ITEM_HELD;
			pmaddevent(EV_USE_ITEM0 + bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].tag);
			pm->ps->stats[STAT_HOLDABLE_ITEM] = 0;
			return;
		}
	}else
		pm->ps->pm_flags &= ~PMF_USE_ITEM_HELD;


	// make weapon function
	if(pm->ps->weaponTime > 0)
		pm->ps->weaponTime -= pml.msec;

	// check for weapon change
	// can't change if weapon is firing, but can change
	// again if lowering or raising
	if(pm->ps->weaponTime <= 0 || pm->ps->weaponstate != WEAPON_FIRING)
		if(pm->ps->weapon != pm->cmd.weapon)
			beginweaponchange(pm->cmd.weapon);

	if(pm->ps->weaponTime > 0)
		return;

	// change weapon if time
	if(pm->ps->weaponstate == WEAPON_DROPPING){
		finishweaponchange();
		return;
	}

	if(pm->ps->weaponstate == WEAPON_RAISING){
		pm->ps->weaponstate = WEAPON_READY;
		if(pm->ps->weapon == WP_GAUNTLET)
			starttorsoanim(TORSO_STAND2);
		else
			starttorsoanim(TORSO_STAND);
		return;
	}

	// check for fire
	if(!(pm->cmd.buttons & BUTTON_ATTACK)){
		pm->ps->weaponTime = 0;
		pm->ps->weaponstate = WEAPON_READY;
		return;
	}

	// start the animation even if out of ammo
	if(pm->ps->weapon == WP_GAUNTLET){
		// the guantlet only "fires" when it actually hits something
		if(!pm->gauntlethit){
			pm->ps->weaponTime = 0;
			pm->ps->weaponstate = WEAPON_READY;
			return;
		}
		starttorsoanim(TORSO_ATTACK2);
	}else
		starttorsoanim(TORSO_ATTACK);

	pm->ps->weaponstate = WEAPON_FIRING;

	// check for out of ammo
	if(!pm->ps->ammo[pm->ps->weapon]){
		pmaddevent(EV_NOAMMO);
		pm->ps->weaponTime += 500;
		return;
	}

	// take an ammo away if not infinite
	if(pm->ps->ammo[pm->ps->weapon] != -1)
		pm->ps->ammo[pm->ps->weapon]--;

	// fire weapon
	pmaddevent(EV_FIRE_WEAPON);

	switch(pm->ps->weapon){
	default:
	case WP_GAUNTLET:
		addTime = 400;
		break;
	case WP_LIGHTNING:
		addTime = 50;
		break;
	case WP_SHOTGUN:
		addTime = 1000;
		break;
	case WP_MACHINEGUN:
		addTime = 100;
		break;
	case WP_GRENADE_LAUNCHER:
		addTime = 800;
		break;
	case WP_ROCKET_LAUNCHER:
		addTime = 800;
		break;
	case WP_PLASMAGUN:
		addTime = 100;
		break;
	case WP_RAILGUN:
		addTime = 1500;
		break;
	case WP_BFG:
		addTime = 200;
		break;
	case WP_GRAPPLING_HOOK:
		addTime = 400;
		break;
	}

	if(pm->ps->powerups[PW_HASTE])
		addTime /= 1.3;

	pm->ps->weaponTime += addTime;
}

/*
================
animate
================
*/

static void
animate(void)
{
	if(pm->cmd.buttons & BUTTON_GESTURE){
		if(pm->ps->torsoTimer == 0){
			starttorsoanim(TORSO_GESTURE);
			pm->ps->torsoTimer = TIMER_GESTURE;
			pmaddevent(EV_TAUNT);
		}
	}
}

/*
================
droptimers
================
*/
static void
droptimers(void)
{
	// drop misc timing counter
	if(pm->ps->pm_time){
		if(pml.msec >= pm->ps->pm_time){
			pm->ps->pm_flags &= ~PMF_ALL_TIMES;
			pm->ps->pm_time = 0;
		}else
			pm->ps->pm_time -= pml.msec;
	}

	// drop animation counter
	if(pm->ps->legsTimer > 0){
		pm->ps->legsTimer -= pml.msec;
		if(pm->ps->legsTimer < 0)
			pm->ps->legsTimer = 0;
	}

	if(pm->ps->torsoTimer > 0){
		pm->ps->torsoTimer -= pml.msec;
		if(pm->ps->torsoTimer < 0)
			pm->ps->torsoTimer = 0;
	}
}

/*
================
updateviewangles

This can be used as another entry point when only the viewangles
are being updated instead of a full move
================
*/
void
updateviewangles(playerState_t *ps, const usercmd_t *cmd)
{
	short temp;
	int i;

	if(ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPINTERMISSION)
		return;	// no view changes at all

	if(ps->pm_type != PM_SPECTATOR && ps->stats[STAT_HEALTH] <= 0)
		return;	// no view changes at all

	// circularly clamp the angles with deltas
	for(i = 0; i<3; i++){
		temp = cmd->angles[i] + ps->delta_angles[i];
		if(i == PITCH){
			// don't let the player look up or down more than 90 degrees
			if(temp > 16000){
				ps->delta_angles[i] = 16000 - cmd->angles[i];
				temp = 16000;
			}else if(temp < -16000){
				ps->delta_angles[i] = -16000 - cmd->angles[i];
				temp = -16000;
			}
		}
		ps->viewangles[i] = SHORT2ANGLE(temp);
	}
}

/*
================
PmoveSingle

================
*/
void trap_SnapVector(float *v);

void
PmoveSingle(pmove_t *pmove)
{
	pm = pmove;

	// this counter lets us debug movement problems with a journal
	// by setting a conditional breakpoint fot the previous frame
	c_pmove++;

	// clear results
	pm->numtouch = 0;
	pm->watertype = 0;
	pm->waterlevel = 0;

	if(pm->ps->stats[STAT_HEALTH] <= 0)
		pm->tracemask &= ~CONTENTS_BODY;	// corpses can fly through bodies

	// make sure walking button is clear if they are running, to avoid
	// proxy no-footsteps cheats
	if(abs(pm->cmd.forwardmove) > 64 || abs(pm->cmd.rightmove) > 64)
		pm->cmd.buttons &= ~BUTTON_WALKING;

	// set the talk balloon flag
	if(pm->cmd.buttons & BUTTON_TALK)
		pm->ps->eFlags |= EF_TALK;
	else
		pm->ps->eFlags &= ~EF_TALK;

	// set the firing flag for continuous beam weapons
	if(!(pm->ps->pm_flags & PMF_RESPAWNED) && pm->ps->pm_type != PM_INTERMISSION && pm->ps->pm_type != PM_NOCLIP
	   && (pm->cmd.buttons & BUTTON_ATTACK) && pm->ps->ammo[pm->ps->weapon])
		pm->ps->eFlags |= EF_FIRING;
	else
		pm->ps->eFlags &= ~EF_FIRING;

	// clear the respawned flag if attack and use are cleared
	if(pm->ps->stats[STAT_HEALTH] > 0 &&
	   !(pm->cmd.buttons & (BUTTON_ATTACK | BUTTON_USE_HOLDABLE)))
		pm->ps->pm_flags &= ~PMF_RESPAWNED;

	// if talk button is down, dissallow all other input
	// this is to prevent any possible intercept proxy from
	// adding fake talk balloons
	if(pmove->cmd.buttons & BUTTON_TALK){
		// keep the talk button set tho for when the cmd.serverTime > 66 msec
		// and the same cmd is used multiple times in Pmove
		pmove->cmd.buttons = BUTTON_TALK;
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;
		pmove->cmd.upmove = 0;
	}

	// clear all pmove local vars
	memset(&pml, 0, sizeof(pml));

	// determine the time
	pml.msec = pmove->cmd.serverTime - pm->ps->commandTime;
	if(pml.msec < 1)
		pml.msec = 1;
	else if(pml.msec > 200)
		pml.msec = 200;
	pm->ps->commandTime = pmove->cmd.serverTime;

	// save old org in case we get stuck
	veccopy(pm->ps->origin, pml.prevorigin);

	// save old velocity for crashlanding
	veccopy(pm->ps->velocity, pml.prevvelocity);

	pml.frametime = pml.msec * 0.001;

	// update the viewangles
	updateviewangles(pm->ps, &pm->cmd);

	anglevecs(pm->ps->viewangles, pml.forward, pml.right, pml.up);

	if(pm->cmd.upmove < 10)
		// not holding jump
		pm->ps->pm_flags &= ~PMF_JUMP_HELD;

	// decide if backpedaling animations should be used
	if(pm->cmd.forwardmove < 0)
		pm->ps->pm_flags |= PMF_BACKWARDS_RUN;
	else if(pm->cmd.forwardmove > 0 || (pm->cmd.forwardmove == 0 && pm->cmd.rightmove))
		pm->ps->pm_flags &= ~PMF_BACKWARDS_RUN;

	if(pm->ps->pm_type >= PM_DEAD){
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}

	if(pm->ps->pm_type == PM_SPECTATOR){
		checkduck();
		flymove();
		droptimers();
		return;
	}

	if(pm->ps->pm_type == PM_NOCLIP){
		noclipmove();
		droptimers();
		return;
	}

	if(pm->ps->pm_type == PM_FREEZE)
		return;	// no movement at all

	if(pm->ps->pm_type == PM_INTERMISSION || pm->ps->pm_type == PM_SPINTERMISSION)
		return;	// no movement at all

	// set watertype, and waterlevel
	setwaterlevel();
	pml.prevwaterlevel = pmove->waterlevel;

	// set mins, maxs, and viewheight
	checkduck();

	// set groundentity
	groundtrace();
	headtrace();

	if(pm->ps->pm_type == PM_DEAD)
		deadmove();

	droptimers();

	if(pm->ps->powerups[PW_FLIGHT])
		// flight powerup doesn't allow jump and has different friction
		flymove();
	else if(pm->ps->pm_flags & PMF_GRAPPLE_PULL){
		grapplemove();
		// We can wiggle a bit
		airmove();
	}else if(pm->ps->pm_flags & PMF_TIME_WATERJUMP)
		waterjumpmove();
	else if(pm->waterlevel > 1)
		// swimming
		watermove();
	else if(pml.walking)
		// walking on ground
		walkmove();
	else
		// airborne
		airmove();

	animate();

	// set groundentity, watertype, and waterlevel
	groundtrace();
	headtrace();
	setwaterlevel();

	// weapons
	weapon();

	// torso animation
	torsoanimation();

	// footstep events / legs animations
	footsteps();

	// entering / leaving water splashes
	waterevents();

	// snap some parts of playerstate to save network bandwidth
	trap_SnapVector(pm->ps->velocity);
}

/*
================
Pmove

Can be called by either the server or the client
================
*/
void
Pmove(pmove_t *pmove)
{
	int finalTime;

	finalTime = pmove->cmd.serverTime;

	if(finalTime < pmove->ps->commandTime)
		return;	// should not happen

	if(finalTime > pmove->ps->commandTime + 1000)
		pmove->ps->commandTime = finalTime - 1000;

	pmove->ps->pmove_framecount = (pmove->ps->pmove_framecount+1) & ((1<<PS_PMOVEFRAMECOUNTBITS)-1);

	// chop the move up if it is too long, to prevent framerate
	// dependent behavior
	while(pmove->ps->commandTime != finalTime){
		int msec;

		msec = finalTime - pmove->ps->commandTime;

		if(pmove->pmove_fixed){
			if(msec > pmove->pmove_msec)
				msec = pmove->pmove_msec;
		}else if(msec > 66)
			msec = 66;

		pmove->cmd.serverTime = pmove->ps->commandTime + msec;
		PmoveSingle(pmove);

		if(pmove->ps->pm_flags & PMF_JUMP_HELD)
			pmove->cmd.upmove = 20;
	}

	//checkstuck();
}
