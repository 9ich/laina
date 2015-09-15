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

// cg_localents.c -- every frame, generate renderer commands for locally
// processed entities, like smoke puffs, gibs, shells, etc.

#include "cg_local.h"

#define MAX_LOCAL_ENTITIES 512
localent_t cg_localEntities[MAX_LOCAL_ENTITIES];
localent_t cg_activeLocalEntities;	// double linked list
localent_t *cg_freeLocalEntities;	// single linked list

/*
===================
initlocalents

This is called at startup and for tournement restarts
===================
*/
void
initlocalents(void)
{
	int i;

	memset(cg_localEntities, 0, sizeof(cg_localEntities));
	cg_activeLocalEntities.next = &cg_activeLocalEntities;
	cg_activeLocalEntities.prev = &cg_activeLocalEntities;
	cg_freeLocalEntities = cg_localEntities;
	for(i = 0; i < MAX_LOCAL_ENTITIES - 1; i++)
		cg_localEntities[i].next = &cg_localEntities[i+1];
}

/*
==================
freelocalentity
==================
*/
void
freelocalentity(localent_t *le)
{
	if(!le->prev)
		cgerrorf("freelocalentity: not active");

	// remove from the doubly linked active list
	le->prev->next = le->next;
	le->next->prev = le->prev;

	// the free list is only singly linked
	le->next = cg_freeLocalEntities;
	cg_freeLocalEntities = le;
}

/*
===================
alloclocalent

Will always succeed, even if it requires freeing an old active entity
===================
*/
localent_t   *
alloclocalent(void)
{
	localent_t *le;

	if(!cg_freeLocalEntities)
		// no free entities, so free the one at the end of the chain
		// remove the oldest active entity
		freelocalentity(cg_activeLocalEntities.prev);

	le = cg_freeLocalEntities;
	cg_freeLocalEntities = cg_freeLocalEntities->next;

	memset(le, 0, sizeof(*le));

	// link into the active list
	le->next = cg_activeLocalEntities.next;
	le->prev = &cg_activeLocalEntities;
	cg_activeLocalEntities.next->prev = le;
	cg_activeLocalEntities.next = le;
	return le;
}

/*
====================================================================================

FRAGMENT PROCESSING

A fragment localentity interacts with the environment in some way (hitting walls),
or generates more localentities along a trail.

====================================================================================
*/

/*
================
bloodtrail

Leave expanding blood puffs behind gibs
================
*/
void
bloodtrail(localent_t *le)
{
	int t;
	int t2;
	int step;
	vec3_t newOrigin;
	localent_t *blood;

	step = 150;
	t = step * ((cg.time - cg.frametime + step) / step);
	t2 = step * (cg.time / step);

	for(; t <= t2; t += step){
		evaltrajectory(&le->pos, t, newOrigin);

		blood = smokepuff(newOrigin, vec3_origin,
				     20,		// radius
				     1, 1, 1, 1,	// color
				     2000,		// trailtime
				     t,			// starttime
				     0,			// fadeintime
				     0,			// flags
				     cgs.media.bloodTrailShader);
		// use the optimized version
		blood->type = LE_FALL_SCALE_FADE;
		// drop a total of 40 units over its lifetime
		blood->pos.trDelta[2] = 40;
	}
}

/*
================
fragmentbouncemark
================
*/
void
fragmentbouncemark(localent_t *le, trace_t *trace)
{
	int radius;

	if(le->marktype == LEMT_BLOOD){
		radius = 16 + (rand()&31);
		impactmark(cgs.media.bloodMarkShader, trace->endpos, trace->plane.normal, random()*360,
			      1, 1, 1, 1, qtrue, radius, qfalse);
	}else if(le->marktype == LEMT_BURN){
		radius = 8 + (rand()&15);
		impactmark(cgs.media.burnMarkShader, trace->endpos, trace->plane.normal, random()*360,
			      1, 1, 1, 1, qtrue, radius, qfalse);
	}

	// don't allow a fragment to make multiple marks, or they
	// pile up while settling
	le->marktype = LEMT_NONE;
}

/*
================
fragmentbouncesound
================
*/
void
fragmentbouncesound(localent_t *le, trace_t *trace)
{
	if(le->bouncesoundtype == LEBS_BLOOD){
		// half the gibs will make splat sounds
		if(rand() & 1){
			int r = rand()&3;
			sfxHandle_t s;

			if(r == 0)
				s = cgs.media.gibBounce1Sound;
			else if(r == 1)
				s = cgs.media.gibBounce2Sound;
			else
				s = cgs.media.gibBounce3Sound;
			trap_S_StartSound(trace->endpos, ENTITYNUM_WORLD, CHAN_AUTO, s);
		}
	}else if(le->bouncesoundtype == LEBS_WOOD){
		sfxHandle_t s;
		if(rand() & 1){
			s = cgs.media.splinterBounce;
			trap_S_StartSound(trace->endpos, ENTITYNUM_WORLD, CHAN_AUTO, s);	}
	}

	// don't allow a fragment to make multiple bounce sounds,
	// or it gets too noisy as they settle
	le->bouncesoundtype = LEBS_NONE;
}

/*
================
reflectvelocity
================
*/
void
reflectvelocity(localent_t *le, trace_t *trace)
{
	vec3_t velocity;
	float dot;
	int hitTime;

	// reflect the velocity on the trace plane
	hitTime = cg.time - cg.frametime + cg.frametime * trace->fraction;
	evaltrajectorydelta(&le->pos, hitTime, velocity);
	dot = vecdot(velocity, trace->plane.normal);
	vecmad(velocity, -2*dot, trace->plane.normal, le->pos.trDelta);

	vecmul(le->pos.trDelta, le->bouncefactor, le->pos.trDelta);

	veccpy(trace->endpos, le->pos.trBase);
	le->pos.trTime = cg.time;

	// check for stop, making sure that even on low FPS systems it doesn't bobble
	if(trace->allsolid ||
	   (trace->plane.normal[2] > 0 &&
	    (le->pos.trDelta[2] < 40 || le->pos.trDelta[2] < -cg.frametime * le->pos.trDelta[2])))
		le->pos.trType = TR_STATIONARY;
	else{
	}
}

/*
================
addfragment
================
*/
void
addfragment(localent_t *le)
{
	vec3_t newOrigin;
	trace_t trace;

	if(le->pos.trType == TR_STATIONARY){
		// sink into the ground if near the removal time
		int t;
		float oldZ;

		t = le->endtime - cg.time;
		if(t < SINK_TIME){
			// we must use an explicit lighting origin, otherwise the
			// lighting would be lost as soon as the origin went
			// into the ground
			veccpy(le->refEntity.origin, le->refEntity.lightingOrigin);
			le->refEntity.renderfx |= RF_LIGHTING_ORIGIN;
			oldZ = le->refEntity.origin[2];
			le->refEntity.origin[2] -= 16 * (1.0 - (float)t / SINK_TIME);
			trap_R_AddRefEntityToScene(&le->refEntity);
			le->refEntity.origin[2] = oldZ;
		}else
			trap_R_AddRefEntityToScene(&le->refEntity);

		return;
	}

	// calculate new position
	evaltrajectory(&le->pos, cg.time, newOrigin);

	// trace a line from previous position to new position
	cgtrace(&trace, le->refEntity.origin, nil, nil, newOrigin, -1, CONTENTS_SOLID);
	if(trace.fraction == 1.0){
		// still in free fall
		veccpy(newOrigin, le->refEntity.origin);

		if(le->flags & LEF_TUMBLE){
			vec3_t angles;

			evaltrajectory(&le->angles, cg.time, angles);
			AnglesToAxis(angles, le->refEntity.axis);
		}

		trap_R_AddRefEntityToScene(&le->refEntity);

		// add a blood trail
		if(le->bouncesoundtype == LEBS_BLOOD)
			bloodtrail(le);

		return;
	}

	// if it is in a nodrop zone, remove it
	// this keeps gibs from waiting at the bottom of pits of death
	// and floating levels
	if(pointcontents(trace.endpos, 0) & CONTENTS_NODROP){
		freelocalentity(le);
		return;
	}

	// leave a mark
	fragmentbouncemark(le, &trace);

	// do a bouncy sound
	fragmentbouncesound(le, &trace);

	// reflect the velocity on the trace plane
	reflectvelocity(le, &trace);

	trap_R_AddRefEntityToScene(&le->refEntity);
}

/*
=====================================================================

TRIVIAL LOCAL ENTITIES

These only do simple scaling or modulation before passing to the renderer
=====================================================================
*/

/*
====================
addfadergb
====================
*/
void
addfadergb(localent_t *le)
{
	refEntity_t *re;
	float c;

	re = &le->refEntity;

	c = (le->endtime - cg.time) * le->liferate;
	c *= 0xff;

	re->shaderRGBA[0] = le->color[0] * c;
	re->shaderRGBA[1] = le->color[1] * c;
	re->shaderRGBA[2] = le->color[2] * c;
	re->shaderRGBA[3] = le->color[3] * c;

	trap_R_AddRefEntityToScene(re);
}

/*
==================
addmovescalefade
==================
*/
static void
addmovescalefade(localent_t *le)
{
	refEntity_t *re;
	float c;
	vec3_t delta;
	float len;

	re = &le->refEntity;

	if(le->fadeintime > le->starttime && cg.time < le->fadeintime)
		// fade / grow time
		c = 1.0 - (float)(le->fadeintime - cg.time) / (le->fadeintime - le->starttime);
	else
		// fade / grow time
		c = (le->endtime - cg.time) * le->liferate;

	re->shaderRGBA[3] = 0xff * c * le->color[3];

	if(!(le->flags & LEF_PUFF_DONT_SCALE))
		re->radius = le->radius * (1.0 - c) + 8;

	evaltrajectory(&le->pos, cg.time, re->origin);

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	vecsub(re->origin, cg.refdef.vieworg, delta);
	len = veclen(delta);
	if(len < le->radius){
		freelocalentity(le);
		return;
	}

	trap_R_AddRefEntityToScene(re);
}

/*
===================
addscalefade

For rocket smokes that hang in place, fade out, and are
removed if the view passes through them.
There are often many of these, so it needs to be simple.
===================
*/
static void
addscalefade(localent_t *le)
{
	refEntity_t *re;
	float c;
	vec3_t delta;
	float len;

	re = &le->refEntity;

	// fade / grow time
	c = (le->endtime - cg.time) * le->liferate;

	re->shaderRGBA[3] = 0xff * c * le->color[3];
	re->radius = le->radius * (1.0 - c) + 8;

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	vecsub(re->origin, cg.refdef.vieworg, delta);
	len = veclen(delta);
	if(len < le->radius){
		freelocalentity(le);
		return;
	}

	trap_R_AddRefEntityToScene(re);
}

/*
=================
addfallscalefade

This is just an optimized addmovescalefade
For blood mists that drift down, fade out, and are
removed if the view passes through them.
There are often 100+ of these, so it needs to be simple.
=================
*/
static void
addfallscalefade(localent_t *le)
{
	refEntity_t *re;
	float c;
	vec3_t delta;
	float len;

	re = &le->refEntity;

	// fade time
	c = (le->endtime - cg.time) * le->liferate;

	re->shaderRGBA[3] = 0xff * c * le->color[3];

	re->origin[2] = le->pos.trBase[2] - (1.0 - c) * le->pos.trDelta[2];

	re->radius = le->radius * (1.0 - c) + 16;

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	vecsub(re->origin, cg.refdef.vieworg, delta);
	len = veclen(delta);
	if(len < le->radius){
		freelocalentity(le);
		return;
	}

	trap_R_AddRefEntityToScene(re);
}

/*
================
addexplosion
================
*/
static void
addexplosion(localent_t *ex)
{
	refEntity_t *ent;

	ent = &ex->refEntity;

	// add the entity
	trap_R_AddRefEntityToScene(ent);

	// add the dlight
	if(ex->light){
		float light;

		light = (float)(cg.time - ex->starttime) / (ex->endtime - ex->starttime);
		if(light < 0.5)
			light = 1.0;
		else
			light = 1.0 - (light - 0.5) * 2;
		light = ex->light * light;
		trap_R_AddLightToScene(ent->origin, light, ex->lightcolor[0], ex->lightcolor[1], ex->lightcolor[2]);
	}
}

/*
================
addspriteexplosion
================
*/
static void
addspriteexplosion(localent_t *le)
{
	refEntity_t re;
	float c;

	re = le->refEntity;

	c = (le->endtime - cg.time) / (float)(le->endtime - le->starttime);
	if(c > 1)
		c = 1.0;	// can happen during connection problems

	re.shaderRGBA[0] = 0xff;
	re.shaderRGBA[1] = 0xff;
	re.shaderRGBA[2] = 0xff;
	re.shaderRGBA[3] = 0xff * c * 0.33;

	re.reType = RT_SPRITE;
	re.radius = 42 * (1.0 - c) + 30;

	trap_R_AddRefEntityToScene(&re);

	// add the dlight
	if(le->light){
		float light;

		light = (float)(cg.time - le->starttime) / (le->endtime - le->starttime);
		if(light < 0.5)
			light = 1.0;
		else
			light = 1.0 - (light - 0.5) * 2;
		light = le->light * light;
		trap_R_AddLightToScene(re.origin, light, le->lightcolor[0], le->lightcolor[1], le->lightcolor[2]);
	}
}

/*
===================
addscoreplum
===================
*/
#define NUMBER_SIZE 8

void
addscoreplum(localent_t *le)
{
	refEntity_t *re;
	vec3_t origin, delta, dir, vec, up = {0, 0, 1};
	float c, len;
	int i, score, digits[10], numdigits, negative;

	re = &le->refEntity;

	c = (le->endtime - cg.time) * le->liferate;

	score = le->radius;
	if(score < 0){
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0x11;
		re->shaderRGBA[2] = 0x11;
	}else{
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0xff;
		re->shaderRGBA[2] = 0xff;
		if(score >= 50)
			re->shaderRGBA[1] = 0;
		else if(score >= 20)
			re->shaderRGBA[0] = re->shaderRGBA[1] = 0;
		else if(score >= 10)
			re->shaderRGBA[2] = 0;
		else if(score >= 2)
			re->shaderRGBA[0] = re->shaderRGBA[2] = 0;

	}
	if(c < 0.25)
		re->shaderRGBA[3] = 0xff * 4 * c;
	else
		re->shaderRGBA[3] = 0xff;

	re->radius = NUMBER_SIZE / 2;

	veccpy(le->pos.trBase, origin);
	origin[2] += 110 - c * 100;

	vecsub(cg.refdef.vieworg, origin, dir);
	veccross(dir, up, vec);
	vecnorm(vec);

	vecmad(origin, -10 + 20 * sin(c * 2 * M_PI), vec, origin);

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	vecsub(origin, cg.refdef.vieworg, delta);
	len = veclen(delta);
	if(len < 20){
		freelocalentity(le);
		return;
	}

	negative = qfalse;
	if(score < 0){
		negative = qtrue;
		score = -score;
	}

	for(numdigits = 0; !(numdigits && !score); numdigits++){
		digits[numdigits] = score % 10;
		score = score / 10;
	}

	if(negative){
		digits[numdigits] = 10;
		numdigits++;
	}

	for(i = 0; i < numdigits; i++){
		vecmad(origin, (float)(((float)numdigits / 2) - i) * NUMBER_SIZE, vec, re->origin);
		re->customShader = cgs.media.numberShaders[digits[numdigits-1-i]];
		trap_R_AddRefEntityToScene(re);
	}
}

/*
===================
addlocalents

===================
*/
void
addlocalents(void)
{
	localent_t *le, *next;

	// walk the list backwards, so any new local entities generated
	// (trails, marks, etc) will be present this frame
	le = cg_activeLocalEntities.prev;
	for(; le != &cg_activeLocalEntities; le = next){
		// grab next now, so if the local entity is freed we
		// still have it
		next = le->prev;

		if(cg.time >= le->endtime){
			freelocalentity(le);
			continue;
		}
		switch(le->type){
		default:
			cgerrorf("Bad type: %i", le->type);
			break;

		case LE_MARK:
			break;

		case LE_SPRITE_EXPLOSION:
			addspriteexplosion(le);
			break;

		case LE_EXPLOSION:
			addexplosion(le);
			break;

		case LE_FRAGMENT:	// gibs and brass
			addfragment(le);
			break;

		case LE_MOVE_SCALE_FADE:	// water bubbles
			addmovescalefade(le);
			break;

		case LE_FADE_RGB:	// teleporters, railtrails
			addfadergb(le);
			break;

		case LE_FALL_SCALE_FADE:// gib blood trails
			addfallscalefade(le);
			break;

		case LE_SCALE_FADE:	// rocket trails
			addscalefade(le);
			break;

		case LE_SCOREPLUM:
			addscoreplum(le);
			break;

		}
	}
}
