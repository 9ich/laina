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

typedef struct
{
	ent_t	*ent;
	vec3_t		origin;
	vec3_t		angles;
	float		deltayaw;
} pushed_t;

pushed_t pushed[MAX_GENTITIES], *pushed_p;

ent_t*
testentityposition(ent_t *ent)
{
	trace_t tr;
	int mask;

	if(ent->clipmask)
		mask = ent->clipmask;
	else
		mask = MASK_SOLID;
	if(ent->client)
		trap_Trace(&tr, ent->client->ps.origin, ent->r.mins, ent->r.maxs,
		   ent->client->ps.origin, ent->s.number, mask);
	else
		trap_Trace(&tr, ent->s.pos.trBase, ent->r.mins, ent->r.maxs,
		   ent->s.pos.trBase, ent->s.number, mask);

	if(tr.startsolid)
		return &g_entities[tr.entityNum];

	return nil;
}

void
createrotationmatrix(vec3_t angles, vec3_t matrix[3])
{
	anglevecs(angles, matrix[0], matrix[1], matrix[2]);
	vecinv(matrix[1]);
}

void
transposematrix(vec3_t matrix[3], vec3_t transpose[3])
{
	int i, j;
	for(i = 0; i < 3; i++)
		for(j = 0; j < 3; j++)
			transpose[i][j] = matrix[j][i];
}

void
rotatepoint(vec3_t point, vec3_t matrix[3])
{
	vec3_t tvec;

	veccpy(point, tvec);
	point[0] = vecdot(matrix[0], tvec);
	point[1] = vecdot(matrix[1], tvec);
	point[2] = vecdot(matrix[2], tvec);
}

/*
Returns qfalse if the move is blocked
*/
qboolean
trypushingentity(ent_t *check, ent_t *pusher, vec3_t move, vec3_t amove)
{
	vec3_t matrix[3], transpose[3];
	vec3_t org, org2, move2;
	ent_t *block;

	// EF_MOVER_STOP will just stop when contacting another entity
	// instead of pushing it, but entities can still ride on top of it
	if((pusher->s.eFlags & EF_MOVER_STOP) &&
	   check->s.groundEntityNum != pusher->s.number)
		return qfalse;

	// save off the old position
	if(pushed_p > &pushed[MAX_GENTITIES])
		errorf("pushed_p > &pushed[MAX_GENTITIES]");
	pushed_p->ent = check;
	veccpy(check->s.pos.trBase, pushed_p->origin);
	veccpy(check->s.apos.trBase, pushed_p->angles);
	if(check->client){
		pushed_p->deltayaw = check->client->ps.delta_angles[YAW];
		veccpy(check->client->ps.origin, pushed_p->origin);
	}
	pushed_p++;

	// try moving the contacted entity
	// figure movement due to the pusher's amove
	createrotationmatrix(amove, transpose);
	transposematrix(transpose, matrix);
	if(check->client)
		vecsub(check->client->ps.origin, pusher->r.currentOrigin, org);
	else
		vecsub(check->s.pos.trBase, pusher->r.currentOrigin, org);
	veccpy(org, org2);
	rotatepoint(org2, matrix);
	vecsub(org2, org, move2);
	// add movement
	vecadd(check->s.pos.trBase, move, check->s.pos.trBase);
	vecadd(check->s.pos.trBase, move2, check->s.pos.trBase);
	if(check->client){
		vecadd(check->client->ps.origin, move, check->client->ps.origin);
		vecadd(check->client->ps.origin, move2, check->client->ps.origin);
		// make sure the client's view rotates when on a rotating mover
		check->client->ps.delta_angles[YAW] += ANGLE2SHORT(amove[YAW]);
	}

	// may have pushed them off an edge
	if(check->s.groundEntityNum != pusher->s.number)
		check->s.groundEntityNum = ENTITYNUM_NONE;

	block = testentityposition(check);
	if(!block){
		// pushed ok
		if(check->client)
			veccpy(check->client->ps.origin, check->r.currentOrigin);
		else
			veccpy(check->s.pos.trBase, check->r.currentOrigin);
		trap_LinkEntity(check);
		return qtrue;
	}

	// if it is ok to leave in the old position, do it
	// this is only relevent for riding entities, not pushed
	// Sliding trapdoors can cause this.
	veccpy((pushed_p-1)->origin, check->s.pos.trBase);
	if(check->client)
		veccpy((pushed_p-1)->origin, check->client->ps.origin);
	veccpy((pushed_p-1)->angles, check->s.apos.trBase);
	block = testentityposition(check);
	if(!block){
		check->s.groundEntityNum = ENTITYNUM_NONE;
		pushed_p--;
		return qtrue;
	}

	// blocked
	return qfalse;
}

qboolean
checkproxmineposition(ent_t *check)
{
	vec3_t start, end;
	trace_t tr;

	vecmad(check->s.pos.trBase, 0.125, check->movedir, start);
	vecmad(check->s.pos.trBase, 2, check->movedir, end);
	trap_Trace(&tr, start, nil, nil, end, check->s.number, MASK_SOLID);

	if(tr.startsolid || tr.fraction < 1)
		return qfalse;

	return qtrue;
}

qboolean
trypushingproxmine(ent_t *check, ent_t *pusher, vec3_t move, vec3_t amove)
{
	vec3_t forward, right, up;
	vec3_t org, org2, move2;
	int ret;

	// we need this for pushing things later
	vecsub(vec3_origin, amove, org);
	anglevecs(org, forward, right, up);

	// try moving the contacted entity
	vecadd(check->s.pos.trBase, move, check->s.pos.trBase);

	// figure movement due to the pusher's amove
	vecsub(check->s.pos.trBase, pusher->r.currentOrigin, org);
	org2[0] = vecdot(org, forward);
	org2[1] = -vecdot(org, right);
	org2[2] = vecdot(org, up);
	vecsub(org2, org, move2);
	vecadd(check->s.pos.trBase, move2, check->s.pos.trBase);

	ret = checkproxmineposition(check);
	if(ret){
		veccpy(check->s.pos.trBase, check->r.currentOrigin);
		trap_LinkEntity(check);
	}
	return ret;
}

void explodemissile(ent_t *ent);

/*
Objects need to be moved back on a failed push,
otherwise riders would continue to slide.
If qfalse is returned, *obstacle will be the blocking entity
*/
qboolean
moverpush(ent_t *pusher, vec3_t move, vec3_t amove, ent_t **obstacle)
{
	int i, e;
	ent_t *check;
	vec3_t mins, maxs;
	pushed_t *p;
	int entityList[MAX_GENTITIES];
	int listedEntities;
	vec3_t totalMins, totalMaxs;

	*obstacle = nil;

	// mins/maxs are the bounds at the destination
	// totalMins / totalMaxs are the bounds for the entire move
	if(pusher->r.currentAngles[0] || pusher->r.currentAngles[1] || pusher->r.currentAngles[2]
	   || amove[0] || amove[1] || amove[2]){
		float radius;

		radius = RadiusFromBounds(pusher->r.mins, pusher->r.maxs);
		for(i = 0; i < 3; i++){
			mins[i] = pusher->r.currentOrigin[i] + move[i] - radius;
			maxs[i] = pusher->r.currentOrigin[i] + move[i] + radius;
			totalMins[i] = mins[i] - move[i];
			totalMaxs[i] = maxs[i] - move[i];
		}
	}else{
		for(i = 0; i<3; i++){
			mins[i] = pusher->r.absmin[i] + move[i];
			maxs[i] = pusher->r.absmax[i] + move[i];
		}

		veccpy(pusher->r.absmin, totalMins);
		veccpy(pusher->r.absmax, totalMaxs);
		for(i = 0; i<3; i++){
			if(move[i] > 0)
				totalMaxs[i] += move[i];
			else
				totalMins[i] += move[i];
		}
	}

	// unlink the pusher so we don't get it in the entityList
	trap_UnlinkEntity(pusher);

	listedEntities = trap_EntitiesInBox(totalMins, totalMaxs, entityList, MAX_GENTITIES);

	// move the pusher to its final position
	vecadd(pusher->r.currentOrigin, move, pusher->r.currentOrigin);
	vecadd(pusher->r.currentAngles, amove, pusher->r.currentAngles);
	trap_LinkEntity(pusher);

	// see if any solid entities are inside the final position
	for(e = 0; e < listedEntities; e++){
		check = &g_entities[entityList[e]];

		// only push items and players
		if(check->s.eType != ET_ITEM && check->s.eType != ET_PLAYER && !check->physobj)
			continue;

		// if the entity is standing on the pusher, it will definitely be moved
		if(check->s.groundEntityNum != pusher->s.number){
			// see if the ent needs to be tested
			if(check->r.absmin[0] >= maxs[0]
			   || check->r.absmin[1] >= maxs[1]
			   || check->r.absmin[2] >= maxs[2]
			   || check->r.absmax[0] <= mins[0]
			   || check->r.absmax[1] <= mins[1]
			   || check->r.absmax[2] <= mins[2])
				continue;
			// see if the ent's bbox is inside the pusher's final position
			// this does allow a fast moving object to pass through a thin entity...
			if(!testentityposition(check))
				continue;
		}

		// the entity needs to be pushed
		if(trypushingentity(check, pusher, move, amove))
			continue;

		// the move was blocked an entity

		// bobbing entities are instant-kill and never get blocked
		if(pusher->s.pos.trType == TR_SINE || pusher->s.apos.trType == TR_SINE){
			entdamage(check, pusher, pusher, nil, nil, 99999, 0, MOD_CRUSH);
			continue;
		}

		// save off the obstacle so we can call the block function (crush, etc)
		*obstacle = check;

		// move back any entities we already moved
		// go backwards, so if the same entity was pushed
		// twice, it goes back to the original position
		for(p = pushed_p-1; p>=pushed; p--){
			veccpy(p->origin, p->ent->s.pos.trBase);
			veccpy(p->angles, p->ent->s.apos.trBase);
			if(p->ent->client){
				p->ent->client->ps.delta_angles[YAW] = p->deltayaw;
				veccpy(p->origin, p->ent->client->ps.origin);
			}
			trap_LinkEntity(p->ent);
		}
		return qfalse;
	}

	return qtrue;
}

void
moverteam(ent_t *ent)
{
	vec3_t move, amove;
	ent_t *part, *obstacle;
	vec3_t origin, angles;

	obstacle = nil;

	// make sure all team slaves can move before commiting
	// any moves or calling any think functions
	// if the move is blocked, all moved objects will be backed out
	pushed_p = pushed;
	for(part = ent; part; part = part->teamchain){
		// get current position
		evaltrajectory(&part->s.pos, level.time, origin);
		evaltrajectory(&part->s.apos, level.time, angles);
		vecsub(origin, part->r.currentOrigin, move);
		vecsub(angles, part->r.currentAngles, amove);
		if(!moverpush(part, move, amove, &obstacle))
			break;	// move was blocked
	}

	if(part){
		// go back to the previous position
		for(part = ent; part; part = part->teamchain){
			part->s.pos.trTime += level.time - level.prevtime;
			part->s.apos.trTime += level.time - level.prevtime;
			evaltrajectory(&part->s.pos, level.time, part->r.currentOrigin);
			evaltrajectory(&part->s.apos, level.time, part->r.currentAngles);
			trap_LinkEntity(part);
		}

		// if the pusher has a "blocked" function, call it
		if(ent->blocked)
			ent->blocked(ent, obstacle);
		return;
	}

	// the move succeeded
	for(part = ent; part; part = part->teamchain)
		// call the reached function if time is at or past end point
		if(part->s.pos.trType == TR_LINEAR_STOP)
			if(level.time >= part->s.pos.trTime + part->s.pos.trDuration)
				if(part->reached)
					part->reached(part);
}

void
runmover(ent_t *ent)
{
	// if not a team captain, don't do anything, because
	// the captain will handle everything
	if(ent->flags & FL_TEAMSLAVE)
		return;

	// if stationary at one of the positions, don't move anything
	if(ent->s.pos.trType != TR_STATIONARY || ent->s.apos.trType != TR_STATIONARY)
		moverteam(ent);

	// check think function
	runthink(ent);
}

/*

General movers

Doors, plats, and buttons are all binary (two position) movers
Pos1 is "at rest", pos2 is "activated"

*/

void
SetMoverState(ent_t *ent, moverstate_t moverstate, int time)
{
	vec3_t delta;
	float f;

	ent->moverstate = moverstate;

	ent->s.pos.trTime = time;
	switch(moverstate){
	case MOVER_POS1:
		veccpy(ent->pos1, ent->s.pos.trBase);
		ent->s.pos.trType = TR_STATIONARY;
		break;
	case MOVER_POS2:
		veccpy(ent->pos2, ent->s.pos.trBase);
		ent->s.pos.trType = TR_STATIONARY;
		break;
	case MOVER_1TO2:
		veccpy(ent->pos1, ent->s.pos.trBase);
		vecsub(ent->pos2, ent->pos1, delta);
		f = 1000.0 / ent->s.pos.trDuration;
		vecmul(delta, f, ent->s.pos.trDelta);
		ent->s.pos.trType = TR_LINEAR_STOP;
		break;
	case MOVER_2TO1:
		veccpy(ent->pos2, ent->s.pos.trBase);
		vecsub(ent->pos1, ent->pos2, delta);
		f = 1000.0 / ent->s.pos.trDuration;
		vecmul(delta, f, ent->s.pos.trDelta);
		ent->s.pos.trType = TR_LINEAR_STOP;
		break;
	}
	evaltrajectory(&ent->s.pos, level.time, ent->r.currentOrigin);
	trap_LinkEntity(ent);
}

/*
All entities in a mover team will move from pos1 to pos2
in the same amount of time
*/
void
matchteam(ent_t *teamleader, int moverstate, int time)
{
	ent_t *slave;

	for(slave = teamleader; slave; slave = slave->teamchain)
		SetMoverState(slave, moverstate, time);
}

void
ReturnToPos1(ent_t *ent)
{
	matchteam(ent, MOVER_2TO1, level.time);

	// looping sound
	ent->s.loopSound = ent->soundloop;

	// starting sound
	if(ent->sound2to1)
		addevent(ent, EV_GENERAL_SOUND, ent->sound2to1);
}

void
Reached_BinaryMover(ent_t *ent)
{
	// stop the looping sound
	ent->s.loopSound = ent->soundloop;

	if(ent->moverstate == MOVER_1TO2){
		// reached pos2
		SetMoverState(ent, MOVER_POS2, level.time);

		// play sound
		if(ent->soundpos2)
			addevent(ent, EV_GENERAL_SOUND, ent->soundpos2);

		// return to pos1 after a delay, -50ms to account for +50ms delay on trigger
		ent->think = ReturnToPos1;
		ent->nextthink = level.time + ent->wait - ent->s.pos.trDuration - 50;

		// fire targets
		if(!ent->activator)
			ent->activator = ent;
		usetargets(ent, ent->activator);
	}else if(ent->moverstate == MOVER_2TO1){
		// reached pos1
		SetMoverState(ent, MOVER_POS1, level.time);

		// play sound
		if(ent->soundpos1)
			addevent(ent, EV_GENERAL_SOUND, ent->soundpos1);

		// close areaportals
		if(ent->teammaster == ent || !ent->teammaster)
			trap_AdjustAreaPortalState(ent, qfalse);
	}else
		errorf("Reached_BinaryMover: bad moverstate");
}

void
Use_BinaryMover(ent_t *ent, ent_t *other, ent_t *activator)
{
	int total;
	int partial;

	// only the master should be used
	if(ent->flags & FL_TEAMSLAVE){
		Use_BinaryMover(ent->teammaster, other, activator);
		return;
	}

	ent->activator = activator;
	ent->ckpoint = level.checkpoint;

	if(ent->moverstate == MOVER_POS1){
		// start moving 50 msec later, becase if this was player
		// triggered, level.time hasn't been advanced yet
		matchteam(ent, MOVER_1TO2, level.time + 50);

		// starting sound
		if(ent->sound1to2)
			addevent(ent, EV_GENERAL_SOUND, ent->sound1to2);

		// looping sound
		ent->s.loopSound = ent->soundloop;

		// open areaportal
		if(ent->teammaster == ent || !ent->teammaster)
			trap_AdjustAreaPortalState(ent, qtrue);
		return;
	}

	// if all the way up, just delay before coming down
	if(ent->moverstate == MOVER_POS2){
		ent->nextthink = level.time + ent->wait - ent->s.pos.trDuration - 50;
		return;
	}

	// only partway down before reversing
	if(ent->moverstate == MOVER_2TO1){
		total = ent->s.pos.trDuration;
		partial = level.time - ent->s.pos.trTime;
		if(partial > total)
			partial = total;

		matchteam(ent, MOVER_1TO2, level.time - (total - partial));

		if(ent->sound1to2)
			addevent(ent, EV_GENERAL_SOUND, ent->sound1to2);
		return;
	}

	// only partway up before reversing
	if(ent->moverstate == MOVER_1TO2){
		total = ent->s.pos.trDuration;
		partial = level.time - ent->s.pos.trTime;
		if(partial > total)
			partial = total;

		matchteam(ent, MOVER_2TO1, level.time - (total - partial));

		if(ent->sound2to1)
			addevent(ent, EV_GENERAL_SOUND, ent->sound2to1);
		return;
	}
}

void
Levelrespawn_Mover(ent_t *ent)
{
	restoreinitialstate(ent);
	ent->s.pos.trTime = level.time;
	ent->s.apos.trTime = level.time;
}

/*
"pos1", "pos2", and "speed" should be set before calling,
so the movement delta can be calculated
*/
void
InitMover(ent_t *ent)
{
	vec3_t move;
	float distance;
	float light;
	vec3_t color;
	qboolean lightSet, colorSet;
	char *sound;

	// if the "model2" key is set, use a seperate model
	// for drawing, but clip against the brushes
	if(ent->model2)
		ent->s.modelindex2 = modelindex(ent->model2);

	// if the "loopsound" key is set, use a constant looping sound when moving
	if(spawnstr("noise", "100", &sound))
		ent->s.loopSound = soundindex(sound);

	// if the "color" or "light" keys are set, setup constantLight
	lightSet = spawnfloat("light", "100", &light);
	colorSet = spawnvec("color", "1 1 1", color);
	if(lightSet || colorSet){
		int r, g, b, i;

		r = color[0] * 255;
		if(r > 255)
			r = 255;
		g = color[1] * 255;
		if(g > 255)
			g = 255;
		b = color[2] * 255;
		if(b > 255)
			b = 255;
		i = light / 4;
		if(i > 255)
			i = 255;
		ent->s.constantLight = r | (g << 8) | (b << 16) | (i << 24);
	}

	ent->use = Use_BinaryMover;
	ent->reached = Reached_BinaryMover;
	ent->levelrespawn = Levelrespawn_Mover;
	ent->ckpoint = level.checkpoint;

	ent->moverstate = MOVER_POS1;
	ent->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	ent->s.eType = ET_MOVER;
	veccpy(ent->pos1, ent->r.currentOrigin);
	trap_LinkEntity(ent);

	ent->s.pos.trType = TR_STATIONARY;
	veccpy(ent->pos1, ent->s.pos.trBase);

	// calculate time to reach second position from speed
	vecsub(ent->pos2, ent->pos1, move);
	distance = veclen(move);
	if(!ent->speed)
		ent->speed = 100;
	vecmul(move, ent->speed, ent->s.pos.trDelta);
	ent->s.pos.trDuration = distance * 1000 / ent->speed;
	if(ent->s.pos.trDuration <= 0)
		ent->s.pos.trDuration = 1;
}

/*

Door

A use can be triggered either by a touch function, by being shot, or by being
targeted by another entity.

*/

void
Blocked_Door(ent_t *ent, ent_t *other)
{
	// remove anything other than a client
	if(!other->client){
		// except CTF flags!!!!
		if(other->s.eType == ET_ITEM && other->item->type == IT_TEAM){
			teamdroppedflag_think(other);
			return;
		}
		enttemp(other->s.origin, EV_ITEM_POP);
		entfree(other);
		return;
	}

	if(ent->damage)
		entdamage(other, ent, ent, nil, nil, ent->damage, 0, MOD_CRUSH);
	if(ent->spawnflags & 4)
		return;	// crushers don't reverse

	// reverse direction
	Use_BinaryMover(ent, ent, other);
}

static void
Touch_DoorTriggerSpectator(ent_t *ent, ent_t *other, trace_t *trace)
{
	int axis;
	float doorMin, doorMax;
	vec3_t origin;

	axis = ent->count;
	// the constants below relate to constants in Think_SpawnNewDoorTrigger()
	doorMin = ent->r.absmin[axis] + 100;
	doorMax = ent->r.absmax[axis] - 100;

	veccpy(other->client->ps.origin, origin);

	if(origin[axis] < doorMin || origin[axis] > doorMax)
		return;

	if(fabs(origin[axis] - doorMax) < fabs(origin[axis] - doorMin))
		origin[axis] = doorMin - 10;
	else
		origin[axis] = doorMax + 10;

	teleportentity(other, origin, tv(10000000.0, 0, 0));
}

void
doortrigger_touch(ent_t *ent, ent_t *other, trace_t *trace)
{
	item_t *key;
	int keytype;

	if(other->client != nil && ent->doorkey > 0 && ent->doorkey < bg_nitems){
		key = &bg_itemlist[ent->doorkey];
		keytype = key->tag;
		if(inventory(&other->client->ps, keytype) <= 0){
			if(level.time > other->keymsgdebouncetime){
				char s[MAX_STRING_CHARS];

				Com_sprintf(s, sizeof s, "You need the %s", key->pickupname);
				trap_SendServerCommand(other-g_entities, va("cp \"%s\"", s));
				other->keymsgdebouncetime = level.time + 3000;
			}
			return;
		}
		other->client->ps.inv[ent->doorkey]--;
		ent->doorkey = -1;	// permanently unlock door
	}
	if(other->client != nil && other->client->sess.team == TEAM_SPECTATOR){
		// if the door is not open and not opening
		if(ent->parent->moverstate != MOVER_1TO2 &&
		   ent->parent->moverstate != MOVER_POS2)
			Touch_DoorTriggerSpectator(ent, other, trace);
	}else if(ent->parent->moverstate != MOVER_1TO2)
		Use_BinaryMover(ent->parent, ent, other);
}

/*
All of the parts of a door have been spawned, so create
a trigger that encloses all of them
*/
void
Think_SpawnNewDoorTrigger(ent_t *ent)
{
	ent_t *other;
	vec3_t mins, maxs;
	int i, best;

	if(!ent)
		return;

	// find the bounds of everything on the team
	veccpy(ent->r.absmin, mins);
	veccpy(ent->r.absmax, maxs);

	for(other = ent->teamchain; other; other = other->teamchain){
		AddPointToBounds(other->r.absmin, mins, maxs);
		AddPointToBounds(other->r.absmax, mins, maxs);
	}

	// find the thinnest axis, which will be the one we expand
	best = 0;
	for(i = 1; i < 3; i++)
		if(maxs[i] - mins[i] < maxs[best] - mins[best])
			best = i;
	maxs[best] += 120;
	mins[best] -= 120;

	// create a trigger with this size
	other = entspawn();
	other->classname = "door_trigger";
	other->doorkey = ent->doorkey;
	veccpy(mins, other->r.mins);
	veccpy(maxs, other->r.maxs);
	other->parent = ent;
	other->r.contents = CONTENTS_TRIGGER;
	other->touch = doortrigger_touch;
	// remember the thinnest axis
	other->count = best;
	trap_LinkEntity(other);

	matchteam(ent, ent->moverstate, level.time);
}

void
Think_MatchTeam(ent_t *ent)
{
	matchteam(ent, ent->moverstate, level.time);
}

/*QUAKED func_door (0 .5 .8) ? START_OPEN x CRUSHER
TOGGLE		wait in both the start and end states for a trigger event.
START_OPEN	the door to moves to its destination when spawned, and operate in reverse.  It is used to temporarily or permanently close off an area when triggered (not useful for touch or takedmg doors).
NOMONSTER	monsters will not trigger this door

"key"	the item classname that must be in the activating player's inventory to unlock this door, such as as key
"model2"	.md3 model to also draw
"angle"		determines the opening direction
"targetname" if set, no touch field will be spawned and a remote button or trigger field activates the door.
"speed"		movement speed (100 default)
"wait"		interval between opening and closing (2 default, -1 = never return)
"lip"		lip remaining at end of move (8 default)
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
"health"	if set, the door must be shot open
*/
void
SP_func_door(ent_t *ent)
{
	vec3_t abs_movedir;
	float distance;
	vec3_t size;
	float lip;
	char *keyclass;
	int i;

	ent->sound1to2 = ent->sound2to1 = soundindex("sound/movers/doors/dr1_strt.wav");
	ent->soundpos1 = ent->soundpos2 = soundindex("sound/movers/doors/dr1_end.wav");

	ent->blocked = Blocked_Door;

	// default speed of 400
	if(!ent->speed)
		ent->speed = 400;

	// default wait of 2 seconds
	if(!ent->wait)
		ent->wait = 2;
	ent->wait *= 1000;

	// default lip of 8 units
	spawnfloat("lip", "8", &lip);

	// default damage of 2 points
	spawnint("dmg", "2", &ent->damage);

	// pick the key itemnum
	ent->doorkey = -1;
	spawnstr("key", "", &keyclass);
	for(i = 0; i < bg_nitems; i++)
		if(Q_stricmp(bg_itemlist[i].classname, keyclass) == 0){
			ent->doorkey = i;
			break;
		}
	if(ent->doorkey == -1 && Q_stricmp(keyclass, "") == 0)
		gprintf("%s: bad item classname for door key\n", keyclass);

	// first position at start
	veccpy(ent->s.origin, ent->pos1);

	// calculate second position
	trap_SetBrushModel(ent, ent->model);
	setmovedir(ent->s.angles, ent->movedir);
	abs_movedir[0] = fabs(ent->movedir[0]);
	abs_movedir[1] = fabs(ent->movedir[1]);
	abs_movedir[2] = fabs(ent->movedir[2]);
	vecsub(ent->r.maxs, ent->r.mins, size);
	distance = vecdot(abs_movedir, size) - lip;
	vecmad(ent->pos1, distance, ent->movedir, ent->pos2);

	// if "start_open", reverse position 1 and 2
	if(ent->spawnflags & 1){
		vec3_t temp;

		veccpy(ent->pos2, temp);
		veccpy(ent->s.origin, ent->pos2);
		veccpy(temp, ent->pos1);
	}

	InitMover(ent);

	ent->nextthink = level.time + FRAMETIME;

	if(!(ent->flags & FL_TEAMSLAVE)){
		int health;

		spawnint("health", "0", &health);
		if(health)
			ent->takedmg = qtrue;
		if(ent->targetname || health)
			// non touch/shoot doors
			ent->think = Think_MatchTeam;
		else
			ent->think = Think_SpawnNewDoorTrigger;
	}
}

/*

Piston

*/

static void
Blocked_Piston(ent_t *ent, ent_t *other)
{
	// crush clients
	if(other->client != nil){
		entdamage(other, ent, ent, nil, nil, 99999, 0, MOD_CRUSH);
		return;
	}
	
	// remove anything other than a client, except CTF flags
	if(other->s.eType == ET_ITEM && other->item->type == IT_TEAM){
		teamdroppedflag_think(other);
		return;
	}
	enttemp(other->s.origin, EV_ITEM_POP);
	entfree(other);
}

static void
Touch_Piston(ent_t *ent, ent_t *other, trace_t *tr)
{
	if(!(ent->spawnflags & (1<<1)) ||
	   ent->moverstate == MOVER_POS1 ||
	   other->health <= 0)
		return;
	entdamage(other, ent, ent, nil, nil, other->health, 0, MOD_CRUSH);
}

/*QUAKED func_piston (.8 .2 .2) ? START_OPEN
TOGGLE		wait in both the start and end states for a trigger event.
START_OPEN	the door to moves to its destination when spawned, and operate
		in reverse.  It is used to temporarily or permanently close
		off an area when triggered (not useful for touch or takedmg
		doors).
KILLER		kill on contact when piston is moving or up

"model2"	.md3 model to also draw
"angle"		determines the opening direction
"targetname"	if set, no touch field will be spawned and a remote button or
		trigger field activates the door.
"speed"		movement speed (100 default)
"wait"		interval between opening and closing (2 default, -1 = never return)
"lip"		lip remaining at end of move (8 default)
"color"		constantLight color
"light"		constantLight radius
*/
void
SP_func_piston(ent_t *ent)
{
	vec3_t abs_movedir;
	float distance;
	vec3_t size;
	float lip;

	ent->sound1to2 = ent->sound2to1 = soundindex("sound/movers/piston/start");
	ent->soundpos1 = ent->soundpos2 = soundindex("sound/movers/piston/end");

	ent->blocked = Blocked_Piston;
	if(ent->spawnflags & (1<<1))
		ent->touch = Touch_Piston;

	// default speed of 400
	if(!ent->speed)
		ent->speed = 400;

	// default wait of 2 seconds
	if(!ent->wait)
		ent->wait = 2;
	ent->wait *= 1000;

	// default lip of 8 units
	spawnfloat("lip", "8", &lip);

	// first position at start
	veccpy(ent->s.origin, ent->pos1);

	// calculate second position
	trap_SetBrushModel(ent, ent->model);
	setmovedir(ent->s.angles, ent->movedir);
	abs_movedir[0] = fabs(ent->movedir[0]);
	abs_movedir[1] = fabs(ent->movedir[1]);
	abs_movedir[2] = fabs(ent->movedir[2]);
	vecsub(ent->r.maxs, ent->r.mins, size);
	distance = vecdot(abs_movedir, size) - lip;
	vecmad(ent->pos1, distance, ent->movedir, ent->pos2);

	// if "start_open", reverse position 1 and 2
	if(ent->spawnflags & 1){
		vec3_t temp;

		veccpy(ent->pos2, temp);
		veccpy(ent->s.origin, ent->pos2);
		veccpy(temp, ent->pos1);
	}

	InitMover(ent);

	ent->nextthink = level.time + FRAMETIME;

	if(!(ent->flags & FL_TEAMSLAVE)){
		if(ent->targetname)
			// non touch/shoot doors
			ent->think = Think_MatchTeam;
	}
}

/*

Platform

*/

/*
Don't allow decent if a living player is on it
*/
void
Touch_Plat(ent_t *ent, ent_t *other, trace_t *trace)
{
	if(!other->client || other->client->ps.stats[STAT_HEALTH] <= 0)
		return;

	// delay return-to-pos1 by one second
	if(ent->moverstate == MOVER_POS2)
		ent->nextthink = level.time + 1000;
}

/*
If the plat is at the bottom position, start it going up
*/
void
Touch_PlatCenterTrigger(ent_t *ent, ent_t *other, trace_t *trace)
{
	if(!other->client)
		return;

	if(ent->parent->moverstate == MOVER_POS1)
		Use_BinaryMover(ent->parent, ent, other);
}

/*
Spawn a trigger in the middle of the plat's low position
Elevator cars require that the trigger extend through the entire low position,
not just sit on top of it.
*/
void
SpawnPlatTrigger(ent_t *ent)
{
	ent_t *trigger;
	vec3_t tmin, tmax;

	// the middle trigger will be a thin trigger just
	// above the starting position
	trigger = entspawn();
	trigger->classname = "plat_trigger";
	trigger->touch = Touch_PlatCenterTrigger;
	trigger->r.contents = CONTENTS_TRIGGER;
	trigger->parent = ent;

	tmin[0] = ent->pos1[0] + ent->r.mins[0] + 33;
	tmin[1] = ent->pos1[1] + ent->r.mins[1] + 33;
	tmin[2] = ent->pos1[2] + ent->r.mins[2];

	tmax[0] = ent->pos1[0] + ent->r.maxs[0] - 33;
	tmax[1] = ent->pos1[1] + ent->r.maxs[1] - 33;
	tmax[2] = ent->pos1[2] + ent->r.maxs[2] + 8;

	if(tmax[0] <= tmin[0]){
		tmin[0] = ent->pos1[0] + (ent->r.mins[0] + ent->r.maxs[0]) *0.5;
		tmax[0] = tmin[0] + 1;
	}
	if(tmax[1] <= tmin[1]){
		tmin[1] = ent->pos1[1] + (ent->r.mins[1] + ent->r.maxs[1]) *0.5;
		tmax[1] = tmin[1] + 1;
	}

	veccpy(tmin, trigger->r.mins);
	veccpy(tmax, trigger->r.maxs);

	trap_LinkEntity(trigger);
}

/*QUAKED func_plat (0 .5 .8) ?
Plats are always drawn in the extended position so they will light correctly.

"lip"		default 8, protrusion above rest position
"height"	total height of movement, defaults to model height
"speed"		overrides default 200.
"dmg"		overrides default 2
"model2"	.md3 model to also draw
"color"		constantLight color
"light"		constantLight radius
*/
void
SP_func_plat(ent_t *ent)
{
	float lip, height;

	ent->sound1to2 = ent->sound2to1 = soundindex("sound/movers/plats/pt1_strt.wav");
	ent->soundpos1 = ent->soundpos2 = soundindex("sound/movers/plats/pt1_end.wav");

	vecclear(ent->s.angles);

	spawnfloat("speed", "200", &ent->speed);
	spawnint("dmg", "2", &ent->damage);
	spawnfloat("wait", "1", &ent->wait);
	spawnfloat("lip", "8", &lip);

	ent->wait *= 1000;

	// create second position
	trap_SetBrushModel(ent, ent->model);

	if(!spawnfloat("height", "0", &height))
		height = (ent->r.maxs[2] - ent->r.mins[2]) - lip;

	// pos1 is the rest (bottom) position, pos2 is the top
	veccpy(ent->s.origin, ent->pos2);
	veccpy(ent->pos2, ent->pos1);
	ent->pos1[2] -= height;

	InitMover(ent);

	// touch function keeps the plat from returning while
	// a live player is standing on it
	ent->touch = Touch_Plat;

	ent->blocked = Blocked_Door;

	ent->parent = ent;	// so it can be treated as a door

	// spawn the trigger if one hasn't been custom made
	if(!ent->targetname)
		SpawnPlatTrigger(ent);
}

/*

Button

*/

void
Touch_Button(ent_t *ent, ent_t *other, trace_t *trace)
{
	if(!other->client)
		return;

	if(ent->moverstate == MOVER_POS1)
		Use_BinaryMover(ent, other, other);
}

/*QUAKED func_button (0 .5 .8) ?
When a button is touched, it moves some distance in the direction of its angle, triggers all of its targets, waits some time, then returns to its original position where it can be triggered again.

"model2"	.md3 model to also draw
"angle"		determines the opening direction
"target"	all entities with a matching targetname will be used
"speed"		override the default 40 speed
"wait"		override the default 1 second wait (-1 = never return)
"lip"		override the default 4 pixel lip remaining at end of move
"health"	if set, the button must be killed instead of touched
"color"		constantLight color
"light"		constantLight radius
*/
void
SP_func_button(ent_t *ent)
{
	vec3_t abs_movedir;
	float distance;
	vec3_t size;
	float lip;

	ent->sound1to2 = soundindex("sound/movers/switches/butn2.wav");

	if(!ent->speed)
		ent->speed = 40;

	if(!ent->wait)
		ent->wait = 1;
	ent->wait *= 1000;

	// first position
	veccpy(ent->s.origin, ent->pos1);

	// calculate second position
	trap_SetBrushModel(ent, ent->model);

	spawnfloat("lip", "4", &lip);

	setmovedir(ent->s.angles, ent->movedir);
	abs_movedir[0] = fabs(ent->movedir[0]);
	abs_movedir[1] = fabs(ent->movedir[1]);
	abs_movedir[2] = fabs(ent->movedir[2]);
	vecsub(ent->r.maxs, ent->r.mins, size);
	distance = abs_movedir[0] * size[0] + abs_movedir[1] * size[1] + 
	   abs_movedir[2] * size[2] - lip;
	vecmad(ent->pos1, distance, ent->movedir, ent->pos2);

	if(ent->health)
		// shootable button
		ent->takedmg = qtrue;
	else
		// touchable button
		ent->touch = Touch_Button;

	InitMover(ent);
}

/*

Train

*/

#define TRAIN_START_ON		1
#define TRAIN_TOGGLE		2
#define TRAIN_BLOCK_STOPS	4

/*
The wait time at a corner has completed, so start moving again
*/
void
Think_BeginMoving(ent_t *ent)
{
	ent->s.pos.trTime = level.time;
	ent->s.pos.trType = TR_LINEAR_STOP;
}

void
Reached_Train(ent_t *ent)
{
	ent_t *next;
	float speed;
	vec3_t move;
	float length;

	// copy the apropriate values
	next = ent->nexttrain;
	if(!next || !next->nexttrain)
		return;	// just stop

	// fire all other targets
	usetargets(next, nil);

	// set the new trajectory
	ent->nexttrain = next->nexttrain;
	veccpy(next->s.origin, ent->pos1);
	veccpy(next->nexttrain->s.origin, ent->pos2);

	// if the path_corner has a speed, use that
	if(next->speed)
		speed = next->speed;
	else
		// otherwise use the train's speed
		speed = ent->speed;
	if(speed < 1)
		speed = 1;

	// calculate duration
	vecsub(ent->pos2, ent->pos1, move);
	length = veclen(move);

	ent->s.pos.trDuration = length * 1000 / speed;

	// Tequila comment: Be sure to send to clients after any fast move case
	ent->r.svFlags &= ~SVF_NOCLIENT;

	// Tequila comment: Fast move case
	if(ent->s.pos.trDuration<1){
		// Tequila comment: As trDuration is used later in a division, we need to avoid that case now
		// With null trDuration,
		// the calculated rocks bounding box becomes infinite and the engine think for a short time
		// any entity is riding that mover but not the world entity... In rare case, I found it
		// can also stuck every map entities after func_door are used.
		// The desired effect with very very big speed is to have instant move, so any not null duration
		// lower than a frame duration should be sufficient.
		// Afaik, the negative case don't have to be supported.
		ent->s.pos.trDuration = 1;

		// Tequila comment: Don't send entity to clients so it becomes really invisible
		ent->r.svFlags |= SVF_NOCLIENT;
	}

	// looping sound
	ent->s.loopSound = next->soundloop;

	// start it going
	SetMoverState(ent, MOVER_1TO2, level.time);

	// if there is a "wait" value on the target, don't start moving yet
	if(next->wait){
		ent->nextthink = level.time + next->wait * 1000;
		ent->think = Think_BeginMoving;
		ent->s.pos.trType = TR_STATIONARY;
	}
}

static void
Use_Train(ent_t *ent, ent_t *other, ent_t *activator)
{
	ent->ckpoint = level.checkpoint;
	ent->nextthink = level.time + 1;
	ent->think = Think_BeginMoving;
}

/*
Link all the corners together
*/
void
Think_SetupTrainTargets(ent_t *ent)
{
	ent_t *path, *next, *start;

	ent->nexttrain = findent(nil, FOFS(targetname), ent->target);
	if(!ent->nexttrain){
		gprintf("func_train at %s with an unfound target\n",
			 vtos(ent->r.absmin));
		return;
	}

	start = nil;
	for(path = ent->nexttrain; path != start; path = next){
		if(!start)
			start = path;

		// if the corner has no target, the train will stop there
		if(!path->target)
			break;

		// find a path_corner among the targets
		// there may also be other targets that get fired when the corner
		// is reached
		next = nil;
		do{
			next = findent(next, FOFS(targetname), path->target);
			if(!next){
				gprintf("Train corner at %s without a target path_corner\n",
					 vtos(path->s.origin));
				return;
			}
		}while(strcmp(next->classname, "path_corner"));

		path->nexttrain = next;
	}

	// start the train from the first corner
	Reached_Train(ent);

	// and make it wait for activation
	ent->s.pos.trType = TR_STATIONARY;
	ent->use = Use_Train;
}

/*QUAKED path_corner (.5 .3 0) (-8 -8 -8) (8 8 8)
Train path corners.
Target: next path corner and other targets to fire
"speed" speed to move to the next corner
"wait" seconds to wait before behining move to next corner
*/
void
SP_path_corner(ent_t *self)
{
	if(!self->targetname){
		gprintf("path_corner with no targetname at %s\n", vtos(self->s.origin));
		entfree(self);
		return;
	}
	// path corners don't need to be linked in
}

/*QUAKED func_train (0 .5 .8) ? START_ON TOGGLE BLOCK_STOPS
A train is a mover that moves between path_corner target points.
Trains MUST HAVE AN ORIGIN BRUSH.
The train spawns at the first target it is pointing at.
"model2"	.md3 model to also draw
"speed"		default 100
"dmg"		default	2
"noise"		looping sound to play when the train is in motion
"target"	next path corner
"color"		constantLight color
"light"		constantLight radius
*/
void
SP_func_train(ent_t *self)
{
	vecclear(self->s.angles);

	if(self->spawnflags & TRAIN_BLOCK_STOPS)
		self->damage = 0;
	else if(!self->damage)
		self->damage = 2;


	if(!self->speed)
		self->speed = 100;

	if(!self->target){
		gprintf("func_train without a target at %s\n", vtos(self->r.absmin));
		entfree(self);
		return;
	}

	trap_SetBrushModel(self, self->model);
	InitMover(self);

	self->reached = Reached_Train;

	// start trains on the second frame, to make sure their targets have had
	// a chance to spawn
	self->nextthink = level.time + FRAMETIME;
	self->think = Think_SetupTrainTargets;
}

/*

Static

*/

/*QUAKED func_static (0 .5 .8) ?
A bmodel that just sits there, doing nothing.  Can be used for conditional walls and models.
"model2"	.md3 model to also draw
"color"		constantLight color
"light"		constantLight radius
*/
void
SP_func_static(ent_t *ent)
{
	trap_SetBrushModel(ent, ent->model);
	InitMover(ent);
	veccpy(ent->s.origin, ent->s.pos.trBase);
	veccpy(ent->s.origin, ent->r.currentOrigin);
}

/*

Rotating

*/

/*QUAKED func_rotating (0 .5 .8) ? START_ON - X_AXIS Y_AXIS
You need to have an origin brush as part of this entity.  The center of that brush will be
the point around which it is rotated. It will rotate around the Z axis by default.  You can
check either the X_AXIS or Y_AXIS box to change that.

"model2"	.md3 model to also draw
"speed"		determines how fast it moves; default value is 100.
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
*/
void
SP_func_rotating(ent_t *ent)
{
	if(!ent->speed)
		ent->speed = 100;

	// set the axis of rotation
	ent->s.apos.trType = TR_LINEAR;
	if(ent->spawnflags & 4)
		ent->s.apos.trDelta[2] = ent->speed;
	else if(ent->spawnflags & 8)
		ent->s.apos.trDelta[0] = ent->speed;
	else
		ent->s.apos.trDelta[1] = ent->speed;

	if(!ent->damage)
		ent->damage = 2;

	trap_SetBrushModel(ent, ent->model);
	InitMover(ent);

	veccpy(ent->s.origin, ent->s.pos.trBase);
	veccpy(ent->s.pos.trBase, ent->r.currentOrigin);
	veccpy(ent->s.apos.trBase, ent->r.currentAngles);

	trap_LinkEntity(ent);
}

/*

Bobbing

*/

/*QUAKED func_bobbing (0 .5 .8) ? X_AXIS Y_AXIS
Normally bobs on the Z axis
"model2"	.md3 model to also draw
"height"	amplitude of bob (32 default)
"speed"		seconds to complete a bob cycle (4 default)
"phase"		the 0.0 to 1.0 offset in the cycle to start at
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
*/
void
SP_func_bobbing(ent_t *ent)
{
	float height;
	float phase;

	spawnfloat("speed", "4", &ent->speed);
	spawnfloat("height", "32", &height);
	spawnint("dmg", "2", &ent->damage);
	spawnfloat("phase", "0", &phase);

	trap_SetBrushModel(ent, ent->model);
	InitMover(ent);

	veccpy(ent->s.origin, ent->s.pos.trBase);
	veccpy(ent->s.origin, ent->r.currentOrigin);

	ent->s.pos.trDuration = ent->speed * 1000;
	ent->s.pos.trTime = ent->s.pos.trDuration * phase;
	ent->s.pos.trType = TR_SINE;

	// set the axis of bobbing
	if(ent->spawnflags & 1)
		ent->s.pos.trDelta[0] = height;
	else if(ent->spawnflags & 2)
		ent->s.pos.trDelta[1] = height;
	else
		ent->s.pos.trDelta[2] = height;
}

/*

Pendulum

*/

/*QUAKED func_pendulum (0 .5 .8) ?
You need to have an origin brush as part of this entity.  Pendulums
always swing north / south on unrotated models.  Add an angles field
to the model to allow rotation in other directions.  Pendulum
frequency is a physical constant based on the length of the beam and
gravity.  The frequency can be multiplied with the freq field.
"model2"	.md3 model to also draw
"speed"		the number of degrees each way the pendulum swings, (30 default)
"freq"		frequency multiplier
"phase"		the 0.0 to 1.0 offset in the cycle to start at
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
*/
void
SP_func_pendulum(ent_t *ent)
{
	float freq, freqmul, length, phase, speed;

	spawnfloat("speed", "30", &speed);
	spawnfloat("freq", "1", &freqmul);
	spawnint("dmg", "2", &ent->damage);
	spawnfloat("phase", "0", &phase);

	trap_SetBrushModel(ent, ent->model);

	// find pendulum length
	length = fabs(ent->r.mins[2]);
	if(length < 8)
		length = 8;

	if(freqmul == 0.0f)
		freqmul = 1.0f;

	freq = 1 / (M_PI * 2) * sqrt(g_gravity.value / (3 * length));

	ent->s.pos.trDuration = 1/freqmul * 1000/freq;

	InitMover(ent);

	veccpy(ent->s.origin, ent->s.pos.trBase);
	veccpy(ent->s.origin, ent->r.currentOrigin);
	veccpy(ent->s.angles, ent->s.apos.trBase);

	ent->s.apos.trDuration = 1/freqmul * 1000/freq;
	ent->s.apos.trTime = ent->s.apos.trDuration * phase;
	ent->s.apos.trType = TR_SINE;
	ent->s.apos.trDelta[2] = speed;
}
