/*
===========================================================================
Copyright (C) 1996-1997 Id Software, Inc.

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

void movenoprint(char *fmt, ...) { (void)fmt; }
#define dprint movenoprint

enum
{
	STEPSIZE	= 18,
	DI_NODIR	= -1
};

static qboolean	checkbottom(ent_t *e);
static qboolean	movestep(ent_t *e, vec3_t mv, qboolean relink);
static qboolean	stepdir(ent_t *e, float yaw, float dist);
static void	newchasedir(ent_t *e, ent_t *other, float dist);
static qboolean	ecloseenough(ent_t *e, ent_t *other, float thresh);

void
npcchangeyaw(ent_t *e)
{
	float ideal, curr, mv, speed;

	dprint("npcchangeyaw\n");

	curr = anglemod(e->npc.angles[YAW]);
	ideal = anglemod(e->npc.idealyaw);
	if(curr == ideal){
		vecclear(e->npc.anglesvel);
		return;
	}
	mv = ideal-curr;
	e->npc.yawspeed = 200;	// FIXME
	speed = e->npc.yawspeed;
	if(ideal > curr){
		if(mv >= 180)
			mv -= 360;
	}else{
		if(mv <= -180)
			mv += 360;
	}
	speed = CLAMP(speed, -mv, mv);
	speed *= 4;
	if(mv < 0)
		speed = -speed;
	e->npc.anglesvel[YAW] = speed;
}

void
npcmovetogoal(ent_t *e, float dist)
{
	ent_t *goal;

	gprintf("npcmovetogoal\n");
	goal = e->npc.goalent;

	if(e->enemy != nil && ecloseenough(e, e->enemy, dist)){
		return;
	}
	if(/*(rand()&3) == 1 || */ !stepdir(e, e->npc.idealyaw, dist)){
		if(e->inuse)
			newchasedir(e, goal, dist);
	}
}

qboolean
npcwalkmove(ent_t *e, float yaw, float dist)
{
	qboolean r;
	vec3_t mv;

	gprintf("npcwalkmove\n");

	if(e->s.groundEntityNum == ENTITYNUM_NONE || (e->npc.aiflags & (AI_FLY|AI_SWIM))){
		return qfalse;
	}
	yaw = yaw*M_PI*2 / 360.0f;
	mv[0] = cos(yaw)*dist;
	mv[1] = sin(yaw)*dist;
	mv[2] = 0;
	r = movestep(e, mv, qtrue);
	return r;
}

static qboolean
ecloseenough(ent_t *e, ent_t *other, float thresh)
{
	int i;

	for(i = 0; i < 3; i++){
		if(other->r.absmin[i] > e->r.absmax[i]+thresh)
			return qfalse;
		if(other->r.absmax[i] < e->r.absmin[i]-thresh)
			return qfalse;
	}
	return qtrue;
}

static int
npccorrectallsolid(ent_t *e)
{
	int i, j, k;
	vec3_t point;

	Com_Printf("npc allsolid\n");

	// jitter around
	for(i = -1; i <= 1; i++){
		for(j = -1; j <= 1; j++)
			for(k = -1; k <= 1; k++){
				veccpy(e->npc.pos, point);
				point[0] += (float)i;
				point[1] += (float)j;
				point[2] += (float)k;
				trap_Trace(&e->npc.groundtrace, point, e->r.mins, e->r.maxs, point, e->s.number, MASK_NPCSOLID);
				if(!e->npc.groundtrace.allsolid){
					point[0] = e->npc.pos[0];
					point[1] = e->npc.pos[1];
					point[2] = e->npc.pos[2] - 0.25;

					trap_Trace(&e->npc.groundtrace, e->npc.pos, e->r.mins, e->r.maxs, point, e->s.number, MASK_NPCSOLID);
					return qtrue;
				}
			}
	}

	e->s.groundEntityNum = ENTITYNUM_NONE;
	e->npc.ongroundplane = qfalse;

	return qfalse;
}

/*
The ground trace didn't hit a surface, so we are in freefall
*/
static void
npcgroundtracemissed(ent_t *e)
{
	if(e->s.groundEntityNum != ENTITYNUM_NONE){
		// we just transitioned into freefall
		Com_Printf("npc lift\n");
	}

	e->s.groundEntityNum = ENTITYNUM_NONE;
	e->npc.ongroundplane = qfalse;
}

static void
npcgroundtrace(ent_t *e)
{
	vec3_t point;
	trace_t trace;

	point[0] = e->npc.pos[0];
	point[1] = e->npc.pos[1];
	point[2] = e->npc.pos[2] - 0.25;

	trap_Trace(&e->npc.groundtrace, e->npc.pos, e->r.mins, e->r.maxs, point,
	   e->s.number, MASK_NPCSOLID);

	// do something corrective if the trace starts in a solid...
	if(e->npc.groundtrace.allsolid)
		if(!npccorrectallsolid(e))
			return;

	// if the trace didn't hit anything, we are in free fall
	if(e->npc.groundtrace.fraction == 1.0){
		npcgroundtracemissed(e);
		e->npc.ongroundplane = qfalse;
		return;
	}

	// check if getting thrown off the ground
	if(e->s.pos.trDelta[2] > 0 && vecdot(e->s.pos.trDelta, trace.plane.normal) > 10){
		/*
		Com_Printf("npc kickoff\n");
		// go into jump animation
		if(pm->cmd.forwardmove >= 0){
			forcelegsanim(LEGS_JUMP);
			pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
		}else{
			forcelegsanim(LEGS_JUMPB);
			pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
		}
		*/
		e->s.groundEntityNum = ENTITYNUM_NONE;
		e->npc.ongroundplane = qfalse;
		return;
	}

	// slopes that are too steep will not be considered onground
	if(0 /*e->npc.groundtrace.plane.normal[2] < MIN_WALK_NORMAL*/){
		Com_Printf("npc steep\n");
		// FIXME: if they can't slide down the slope, let them
		// walk (sharp crevices)
		e->s.groundEntityNum = ENTITYNUM_NONE;
		e->npc.ongroundplane = qtrue;
		return;
	}

	e->npc.ongroundplane = qtrue;

	if(e->s.groundEntityNum == ENTITYNUM_NONE){
		Com_Printf("npc land\n");

		/*
		crashland();
		*/
	}

	e->s.groundEntityNum = e->npc.groundtrace.entityNum;

	//pmaddtouchent(e->npc.groundtrace.entityNum);
}

static qboolean
checkbottom(ent_t *e)
{
	vec3_t mins, maxs, start, stop;
	trace_t tr;
	int x, y, contents;
	float mid, bot;

	vecadd(e->npc.pos, e->r.mins, mins);
	vecadd(e->npc.pos, e->r.maxs, maxs);

	npcgroundtrace(e);

	// If all corner points under the corners are solid world,
	// don't bother with the tougher checks.
	// The corners must be within 16u of the midpoint.
	start[2] = mins[2] - 1;
	for(x = 0; x <= 1; x++){
		for(y = 0; y <= 1; y++){
			start[0] = x? maxs[0] : mins[0];
			start[1] = y? maxs[1] : mins[1];
			contents = trap_PointContents(start, e->s.number);
			if(!(contents & CONTENTS_SOLID))
				goto Realcheck;
		}
	}
	return qtrue;

Realcheck:
	start[2] = mins[2];

	// The midpointmust be within 16 u of the bottom.
	start[0] = stop[0] = (mins[0] + maxs[0])*0.5f;
	start[1] = stop[1] = (mins[1] + maxs[1])*0.5f;
	stop[2] = start[2] - 2*STEPSIZE;
	trap_Trace(&tr, start, vec3_origin, vec3_origin, stop, e->s.number, 
	   MASK_NPCSOLID);
	if(tr.fraction == 1.0f)
		return qfalse;
	mid = bot = tr.endpos[2];

	for(x = 0; x <= 1; x++){
		for(y = 0; y <= 1; y++){
			start[0] = x? maxs[0] : mins[0];
			start[1] = y? maxs[1] : mins[1];
			trap_Trace(&tr, start, vec3_origin, vec3_origin, stop, 
			   e->s.number, MASK_NPCSOLID);
			if(tr.fraction != 1.0f && tr.endpos[2] > bot)
				bot = tr.endpos[2];
			if(tr.fraction == 1.0f || mid - tr.endpos[2] > STEPSIZE)
				return qfalse;
		}
	}
	return qtrue;
}

static qboolean
movestep(ent_t *e, vec3_t mv, qboolean relink)
{
	vec3_t oldorg, neworg, end, test;
	trace_t tr;
	int contents;
	float stepsz;

	gprintf("movestep mv=%s\n", vtos(mv));

	// try the move
	veccpy(e->npc.pos, oldorg);
	vecadd(e->npc.pos, mv, neworg);

	// (this is where q2 handles fly and swim)

	// push down from a step height above the wished-for position
	stepsz = 1.0f;
	if(!(e->npc.aiflags & AI_NOSTEP))
		stepsz = STEPSIZE;
	neworg[2] += stepsz;
	veccpy(neworg, end);
	end[2] -= stepsz*2.0f;
	trap_Trace(&tr, neworg, e->r.mins, e->r.maxs, end, e->s.number,
	   MASK_NPCSOLID);
	dprint("%s %s\n", vtos(neworg), vtos(end));
	if(tr.allsolid){
		vecclear(e->npc.vel);
		return qfalse;
	}
	if(tr.startsolid){
		neworg[2] -= stepsz;
		trap_Trace(&tr, neworg, e->r.mins, e->r.maxs, end, e->s.number,
		   MASK_NPCSOLID);
		dprint("%s %s\n", vtos(neworg), vtos(end));
		if(tr.allsolid || tr.startsolid){
			vecclear(e->npc.vel);
			return qfalse;
		}
	}

	// (q2 swim here)

	// don't go in water
	veccpy(tr.endpos, test);
	test[2] = tr.endpos[2] + e->r.mins[2]+1;
	contents = trap_PointContents(test, e->s.number);
	if(contents & MASK_WATER)
		return qfalse;

	if(tr.fraction == 1.0f){
		if(!(e->npc.aiflags & AI_PARTIALGROUND)){
			vecclear(e->npc.vel);
			return qfalse;
		}
		veccpy(mv, e->npc.vel);
		dprint("trBase=%s trDelta=%s\n", vtos(e->s.pos.trBase),
		   vtos(e->s.pos.trDelta));
		if(relink){
			trap_LinkEntity(e);
		}
		e->s.groundEntityNum = ENTITYNUM_NONE;
		return qtrue;
	}

	// check point traces down for dangling corners
	vecsub(tr.endpos, e->npc.pos, e->npc.vel);
	vecmul(e->npc.vel, 10.0f, e->npc.vel);
	if(!checkbottom(e)){
		if(e->npc.aiflags & AI_PARTIALGROUND){
			// entity had floor mostly pulled out from under it
			// and is trying to correct
			if(relink){
				trap_LinkEntity(e);
			}
			vecclear(e->npc.vel);
			return qtrue;
		}
		dprint("oldorg=%s\n", oldorg);
		vecclear(e->npc.vel);
		return qfalse;
	}

	e->npc.aiflags &= ~AI_PARTIALGROUND;
	e->s.groundEntityNum = tr.entityNum;

	if(relink){
		trap_LinkEntity(e);
	}

	return qtrue;
}

static qboolean
stepdir(ent_t *e, float yaw, float dist)
{
	vec3_t mv;
	float d;

	gprintf("stepdir\n");

	e->npc.idealyaw = yaw;
	npcchangeyaw(e);
	yaw = yaw*M_PI*2 / 360;
	mv[0] = cos(yaw)*dist;
	mv[1] = sin(yaw)*dist;
	mv[2] = 0;
	if(movestep(e, mv, qfalse)){
		d = AngleMod(e->npc.angles[YAW]) - e->npc.idealyaw;
		if(d > 45 && d < 315){
			// not turned far enough, don't take the step
			vecclear(e->npc.vel);
		}
		trap_LinkEntity(e);
		return qtrue;
	}
	trap_LinkEntity(e);
	return qfalse;
}

static void
newchasedir(ent_t *e, ent_t *other, float dist)
{
	float dx, dy, d[3], tdir, olddir, turnaround;

	dprint("newchasedir\n");

	olddir = anglemod(((int)e->npc.idealyaw/45)*45);
	turnaround = anglemod(olddir - 180);
	dx = other->s.pos.trBase[0] - e->npc.pos[0];
	dy = other->s.pos.trBase[1] - e->npc.pos[1];
	d[1] = d[2] = DI_NODIR;
	if(dx > 10)
		d[1] = 0;
	else if(dx < -10)
		d[1] = 180;
	if(dy < -10)
		d[2] = 270;
	else if(dy > 10)
		d[2] = 90;

	// try direct route
	if(d[1] != DI_NODIR && d[2] != DI_NODIR){
		if(d[1] == 0)
			tdir = d[2] == 90? 45 : 315;
		else
			tdir = d[2] == 90? 135 : 215;
		if(tdir != turnaround && stepdir(e, tdir, dist))
			return;
	}

	// try other dirs
	if(((rand()&3) & 1) || fabs(dy) > fabs(dx)){
		tdir = d[1];
		d[1] = d[2];
		d[2] = tdir;
	}

	if(d[1] != DI_NODIR && d[1] != turnaround &&
	   stepdir(e, d[1], dist))
		return;
	if(d[2] != DI_NODIR && d[2] != turnaround &&
	   stepdir(e, d[2], dist))
		return;

	// there is no direct path to the enemy, pick another dir
	if(olddir != DI_NODIR && stepdir(e, olddir, dist))
		return;
	if(rand()&1){
		for(tdir = 0; tdir <= 315; tdir += 45)
			if(tdir != turnaround && stepdir(e, tdir, dist))
				return;
	}else{
		for(tdir = 315; tdir >= 0; tdir -= 45)
			if(tdir != turnaround && stepdir(e, tdir, dist))
				return;
	}

	if(turnaround != DI_NODIR && stepdir(e, turnaround, dist))
		return;
	e->npc.idealyaw = olddir;
	// if a bridge was pulled out from under an NPC, it may
	// not have a valid standing pos
	if(!checkbottom(e))
		e->npc.aiflags |= AI_PARTIALGROUND;
}

