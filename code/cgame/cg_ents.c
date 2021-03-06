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
// cg_ents.c -- present snapshot entities, happens every single frame

#include "cg_local.h"

/*
Modifies the entities position and axis by the given
tag location.
*/
void
entontag(refEntity_t *entity, const refEntity_t *parent,
   qhandle_t parentModel, char *tagName)
{
	int i;
	orientation_t lerped;

	// lerp the tag
	trap_R_LerpTag(&lerped, parentModel, parent->oldframe, parent->frame,
		       1.0 - parent->backlerp, tagName);

	// FIXME: allow origin offsets along tag?
	veccpy(parent->origin, entity->origin);
	for(i = 0; i < 3; i++)
		vecmad(entity->origin, lerped.origin[i], parent->axis[i], entity->origin);

	// had to cast away the const to avoid compiler problems...
	MatrixMultiply(lerped.axis, ((refEntity_t*)parent)->axis, entity->axis);
	entity->backlerp = parent->backlerp;
}

/*
Modifies the entity's position and axis by the given
tag location.
*/
void
rotentontag(refEntity_t *entity, const refEntity_t *parent,
   qhandle_t parentModel, char *tagName)
{
	int i;
	orientation_t lerped;
	vec3_t tempAxis[3];

	//AxisClear(entity->axis);
	// lerp the tag
	trap_R_LerpTag(&lerped, parentModel, parent->oldframe, parent->frame,
		       1.0 - parent->backlerp, tagName);

	// FIXME: allow origin offsets along tag?
	veccpy(parent->origin, entity->origin);
	for(i = 0; i < 3; i++)
		vecmad(entity->origin, lerped.origin[i], parent->axis[i], entity->origin);

	// had to cast away the const to avoid compiler problems...
	MatrixMultiply(entity->axis, lerped.axis, tempAxis);
	MatrixMultiply(tempAxis, ((refEntity_t*)parent)->axis, entity->axis);
}

/*
======================
Returns the Z component of the surface being shadowed.

  should it return a full plane instead of a Z?
======================
*/
#define SHADOW_DISTANCE 300
qboolean
drawentshadow(cent_t *cent, float *shadowPlane)
{
	vec3_t end, mins = {-8, -8, 0}, maxs = {8, 8, 2};
	trace_t trace;
	float alpha, size;

	*shadowPlane = 0;

	if(cg_shadows.integer == 0)
		return qfalse;
	// no shadows when invisible
	if(cent->currstate.powerups & (1 << PW_INVIS))
		return qfalse;

	// send a trace down from the entity to the ground
	veccpy(cent->lerporigin, end);
	end[2] -= SHADOW_DISTANCE;
	trap_CM_BoxTrace(&trace, cent->lerporigin, end, mins, maxs, 0, MASK_PLAYERSOLID);
	// no shadow if too high
	if(trace.fraction == 1.0f || trace.startsolid || trace.allsolid)
		return qfalse;
	*shadowPlane = trace.endpos[2] + 1.0f;

	// no mark for stencil or projection shadows
	if(cg_shadows.integer != 1)
		return qtrue;

	// fade out and expand the shadow blob with height
	alpha = 0.5f - 0.5f*trace.fraction;
	size = 24.0f + 15.0f*trace.fraction;

	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
	impactmark(cgs.media.shadowMarkShader, trace.endpos, trace.plane.normal,
		      cent->pe.legs.yaw, alpha, alpha, alpha, 1, qfalse, size, qtrue);
	return qtrue;
}

/*
==========================================================================

FUNCTIONS CALLED EACH FRAME

==========================================================================
*/

/*
======================
setentsoundpos

Also called by event processing code
======================
*/
void
setentsoundpos(cent_t *cent)
{
	if(cent->currstate.solid == SOLID_BMODEL){
		vec3_t origin;
		float *v;

		v = cgs.inlinemodelmidpoints[cent->currstate.modelindex];
		vecadd(cent->lerporigin, v, origin);
		trap_S_UpdateEntityPosition(cent->currstate.number, origin);
	}else
		trap_S_UpdateEntityPosition(cent->currstate.number, cent->lerporigin);
}

/*
==================
entfx

Add continuous entity effects, like local entity emission and lighting
==================
*/
static void
entfx(cent_t *cent)
{
	// update sound origins
	setentsoundpos(cent);

	// add loop sound
	if(cent->currstate.loopSound){
		if(cent->currstate.eType != ET_SPEAKER)
			trap_S_AddLoopingSound(cent->currstate.number, cent->lerporigin, vec3_origin,
					       cgs.gamesounds[cent->currstate.loopSound]);
		else
			trap_S_AddRealLoopingSound(cent->currstate.number, cent->lerporigin, vec3_origin,
						   cgs.gamesounds[cent->currstate.loopSound]);
	}

	// constant light glow
	if(cent->currstate.constantLight){
		int cl;
		float i, r, g, b;

		cl = cent->currstate.constantLight;
		r = (float)(cl & 0xFF) / 255.0;
		g = (float)((cl >> 8) & 0xFF) / 255.0;
		b = (float)((cl >> 16) & 0xFF) / 255.0;
		i = (float)((cl >> 24) & 0xFF) * 4.0;
		trap_R_AddLightToScene(cent->lerporigin, i, r, g, b);
	}
}

/*
==================
dogeneral
==================
*/
static void
dogeneral(cent_t *cent)
{
	refEntity_t ent;
	entityState_t *s1;
	int anim;

	s1 = &cent->currstate;

	// if set to invisible, skip
	if(!s1->modelindex)
		return;

	// create the render entity
	memset(&ent, 0, sizeof(ent));
	veccpy(cent->lerporigin, ent.origin);
	veccpy(cent->lerporigin, ent.oldorigin);
	// convert angles to axis
	AnglesToAxis(cent->lerpangles, ent.axis);

	// set frame
	anim = s1->anim;
	if(s1->nextanimtime != 0 && cg.time > s1->nextanimtime)
		anim = s1->nextanim;
	runlerpframe(cgs.anims[s1->modelindex], &cent->lerpframe, anim, 1.0f);
	ent.frame = cent->lerpframe.frame;
	ent.oldframe = cent->lerpframe.oldframe;
	ent.backlerp = cent->lerpframe.backlerp;

	// get the model, either as a bmodel or a modelindex
	if(s1->solid == SOLID_BMODEL)
		ent.hModel = cgs.inlinedrawmodel[s1->modelindex];
	else
		ent.hModel = cgs.gamemodels[s1->modelindex];

	ent.renderfx |= RF_MINLIGHT;

	// player model
	if(s1->number == cg.snap->ps.clientNum)
		ent.renderfx |= RF_THIRD_PERSON;	// only draw from mirrors

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);
}

/*
dogeneral with a blob shadow
*/
static void
dogeneralshadowed(cent_t *cent)
{
	refEntity_t ent;
	entityState_t *s1;
	float shadowplane;
	int anim;

	s1 = &cent->currstate;

	// if set to invisible, skip
	if(!s1->modelindex)
		return;

	// create the render entity
	memset(&ent, 0, sizeof(ent));
	veccpy(cent->lerporigin, ent.origin);
	veccpy(cent->lerporigin, ent.oldorigin);
	// convert angles to axis
	AnglesToAxis(cent->lerpangles, ent.axis);

	// set frame
	anim = s1->anim;
	if(s1->nextanimtime != 0 && cg.time > s1->nextanimtime)
		anim = s1->nextanim;
	runlerpframe(cgs.anims[s1->modelindex], &cent->lerpframe, anim, 1.0f);
	ent.frame = cent->lerpframe.frame;
	ent.oldframe = cent->lerpframe.oldframe;
	ent.backlerp = cent->lerpframe.backlerp;

	// get the model, either as a bmodel or a modelindex
	if(s1->solid == SOLID_BMODEL)
		ent.hModel = cgs.inlinedrawmodel[s1->modelindex];
	else
		ent.hModel = cgs.gamemodels[s1->modelindex];

	drawentshadow(cent, &shadowplane);
	ent.shadowPlane = shadowplane;

	ent.renderfx |= RF_MINLIGHT;

	// player model
	if(s1->number == cg.snap->ps.clientNum)
		ent.renderfx |= RF_THIRD_PERSON;	// only draw from mirrors

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);
}

/*
==================
dospeaker

Speaker entities can automatically play sounds
==================
*/
static void
dospeaker(cent_t *cent)
{
	if(!cent->currstate.clientNum)	// FIXME: use something other than clientNum...
		return;				// not auto triggering

	if(cg.time < cent->misctime)
		return;

	trap_S_StartSound(nil, cent->currstate.number, CHAN_ITEM, cgs.gamesounds[cent->currstate.eventParm]);

	//	ent->s.frame = ent->wait * 10;
	//	ent->s.clientNum = ent->random * 10;
	cent->misctime = cg.time + cent->currstate.frame * 100 + cent->currstate.clientNum * 100 * crandom();
}

/*
==================
doitem
==================
*/
static void
doitem(cent_t *cent)
{
	refEntity_t ent;
	entityState_t *es;
	item_t *item;
	int msec;
	float frac;
	float scale;
	weapinfo_t *wi;

	es = &cent->currstate;
	if(es->modelindex >= bg_nitems)
		cgerrorf("Bad item index %i on entity", es->modelindex);

	// if set to invisible, skip
	if(!es->modelindex || (es->eFlags & EF_NODRAW))
		return;

	item = &bg_itemlist[es->modelindex];
	if(cg_simpleItems.integer && item->type != IT_TEAM){
		memset(&ent, 0, sizeof(ent));
		ent.reType = RT_SPRITE;
		veccpy(cent->lerporigin, ent.origin);
		ent.radius = 14;
		ent.customShader = cg_items[es->modelindex].icon;
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 255;
		ent.shaderRGBA[3] = 255;
		trap_R_AddRefEntityToScene(&ent);
		return;
	}

	// items bob up and down continuously
	scale = 0.005 + cent->currstate.number * 0.00001;
	cent->lerporigin[2] += 4 + cos((cg.time + 1000) *  scale) * 4;

	memset(&ent, 0, sizeof(ent));

	// autorotate at one of two speeds
	if(item->type == IT_LIFE){
		veccpy(cg.autoanglesfast, cent->lerpangles);
		AxisCopy(cg.autoaxisfast, ent.axis);
	}else{
		veccpy(cg.autoangles, cent->lerpangles);
		AxisCopy(cg.autoaxis, ent.axis);
	}

	wi = nil;
	// the weapons have their origin where they attatch to player
	// models, so we need to offset them or they will rotate
	// eccentricly
	if(item->type == IT_WEAPON){
		wi = &cg_weapons[item->tag];
		cent->lerporigin[0] -=
			wi->midpoint[0] * ent.axis[0][0] +
			wi->midpoint[1] * ent.axis[1][0] +
			wi->midpoint[2] * ent.axis[2][0];
		cent->lerporigin[1] -=
			wi->midpoint[0] * ent.axis[0][1] +
			wi->midpoint[1] * ent.axis[1][1] +
			wi->midpoint[2] * ent.axis[2][1];
		cent->lerporigin[2] -=
			wi->midpoint[0] * ent.axis[0][2] +
			wi->midpoint[1] * ent.axis[1][2] +
			wi->midpoint[2] * ent.axis[2][2];

		cent->lerporigin[2] += 8;	// an extra height boost
	}

	if(item->type == IT_WEAPON && item->tag == WP_RAILGUN){
		clientinfo_t *ci = &cgs.clientinfo[cg.snap->ps.clientNum];
		Byte4Copy(ci->c1rgba, ent.shaderRGBA);
	}

	ent.hModel = cg_items[es->modelindex].models[0];

	veccpy(cent->lerporigin, ent.origin);
	veccpy(cent->lerporigin, ent.oldorigin);

	ent.nonNormalizedAxes = qfalse;

	// if just respawned, slowly scale up
	msec = cg.time - cent->misctime;
	if(msec >= 0 && msec < ITEM_SCALEUP_TIME){
		frac = (float)msec / ITEM_SCALEUP_TIME;
		vecmul(ent.axis[0], frac, ent.axis[0]);
		vecmul(ent.axis[1], frac, ent.axis[1]);
		vecmul(ent.axis[2], frac, ent.axis[2]);
		ent.nonNormalizedAxes = qtrue;
	}else
		frac = 1.0;

	// items need to keep a minimum light value so they are always
	// visible
	ent.renderfx |= RF_MINLIGHT;

	// increase the size of the weapons when they are presented as items
	if(item->type == IT_WEAPON){
		vecmul(ent.axis[0], 1.5, ent.axis[0]);
		vecmul(ent.axis[1], 1.5, ent.axis[1]);
		vecmul(ent.axis[2], 1.5, ent.axis[2]);
		ent.nonNormalizedAxes = qtrue;
	}


	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);

	if(!cg_simpleItems.integer){
		vec3_t spinAngles;

		vecclear(spinAngles);

		if(item->type == IT_TOKEN ||
		   item->type == IT_LIFE ||
		   item->type == IT_POWERUP)
			if((ent.hModel = cg_items[es->modelindex].models[1]) != 0){
				if(item->type == IT_POWERUP){
					ent.origin[2] += 12;
					spinAngles[1] = (cg.time & 1023) * 360 / -1024.0f;
				}
				AnglesToAxis(spinAngles, ent.axis);

				// scale up if respawning
				if(frac != 1.0){
					vecmul(ent.axis[0], frac, ent.axis[0]);
					vecmul(ent.axis[1], frac, ent.axis[1]);
					vecmul(ent.axis[2], frac, ent.axis[2]);
					ent.nonNormalizedAxes = qtrue;
				}
				trap_R_AddRefEntityToScene(&ent);
			}
	}
}

/*
===============
domissile
===============
*/
static void
domissile(cent_t *cent)
{
	refEntity_t ent;
	entityState_t *s1;
	const weapinfo_t *weapon;
//	int	col;

	s1 = &cent->currstate;
	if(s1->weapon >= WP_NUM_WEAPONS)
		s1->weapon = 0;
	weapon = &cg_weapons[s1->weapon];

	// calculate the axis
	veccpy(s1->angles, cent->lerpangles);

	// add trails
	if(weapon->missileTrailFunc)
		weapon->missileTrailFunc(cent, weapon);
	/*
	if( cent->currstate.modelindex == TEAM_RED ){
	        col = 1;
	}
	else if( cent->currstate.modelindex == TEAM_BLUE ){
	        col = 2;
	}
	else{
	        col = 0;
	}

	// add dynamic light
	if( weapon->missilelight ){
	        trap_R_AddLightToScene(cent->lerporigin, weapon->missilelight,
	                weapon->missilelightcolor[col][0], weapon->missilelightcolor[col][1], weapon->missilelightcolor[col][2] );
	}
	*/
	// add dynamic light
	if(weapon->missilelight)
		trap_R_AddLightToScene(cent->lerporigin, weapon->missilelight,
				       weapon->missilelightcolor[0], weapon->missilelightcolor[1], weapon->missilelightcolor[2]);

	// add missile sound
	if(weapon->missilesound){
		vec3_t velocity;

		evaltrajectorydelta(&cent->currstate.pos, cg.time, velocity);

		trap_S_AddLoopingSound(cent->currstate.number, cent->lerporigin, velocity, weapon->missilesound);
	}

	// create the render entity
	memset(&ent, 0, sizeof(ent));
	veccpy(cent->lerporigin, ent.origin);
	veccpy(cent->lerporigin, ent.oldorigin);

	if(cent->currstate.weapon == WP_PLASMAGUN){
		ent.reType = RT_SPRITE;
		ent.radius = 16;
		ent.rotation = 0;
		ent.customShader = cgs.media.plasmaBallShader;
		trap_R_AddRefEntityToScene(&ent);
		return;
	}

	// flicker between two skins
	ent.skinNum = cg.clframe & 1;
	ent.hModel = weapon->missilemodel;
	ent.renderfx = weapon->missilerenderfx | RF_NOSHADOW;


	// convert direction of travel into axis
	if(VectorNormalize2(s1->pos.trDelta, ent.axis[0]) == 0)
		ent.axis[0][2] = 1;

	// spin as it moves
	if(s1->pos.trType != TR_STATIONARY)
		RotateAroundDirection(ent.axis, cg.time / 4);
	else{
		{
			RotateAroundDirection(ent.axis, s1->time);
		}
	}

	// add to refresh list, possibly with quad glow
	addrefentitywithpowerups(&ent, s1, TEAM_FREE);
}

/*
===============
dograpple

This is called when the grapple is sitting up against the wall
===============
*/
static void
dograpple(cent_t *cent)
{
	refEntity_t ent;
	entityState_t *s1;
	const weapinfo_t *weapon;

	s1 = &cent->currstate;
	if(s1->weapon >= WP_NUM_WEAPONS)
		s1->weapon = 0;
	weapon = &cg_weapons[s1->weapon];

	// calculate the axis
	veccpy(s1->angles, cent->lerpangles);

#if 0	// FIXME add grapple pull sound here..?
	// add missile sound
	if(weapon->missilesound)
		trap_S_AddLoopingSound(cent->currstate.number, cent->lerporigin, vec3_origin, weapon->missilesound);

#endif

	// Will draw cable if needed
	grappletrail(cent, weapon);

	// create the render entity
	memset(&ent, 0, sizeof(ent));
	veccpy(cent->lerporigin, ent.origin);
	veccpy(cent->lerporigin, ent.oldorigin);

	// flicker between two skins
	ent.skinNum = cg.clframe & 1;
	ent.hModel = weapon->missilemodel;
	ent.renderfx = weapon->missilerenderfx | RF_NOSHADOW;

	// convert direction of travel into axis
	if(VectorNormalize2(s1->pos.trDelta, ent.axis[0]) == 0)
		ent.axis[0][2] = 1;

	trap_R_AddRefEntityToScene(&ent);
}

/*
===============
domover
===============
*/
static void
domover(cent_t *cent)
{
	refEntity_t ent;
	entityState_t *s1;
	int anim;

	s1 = &cent->currstate;

	// create the render entity
	memset(&ent, 0, sizeof(ent));
	veccpy(cent->lerporigin, ent.origin);
	veccpy(cent->lerporigin, ent.oldorigin);
	AnglesToAxis(cent->lerpangles, ent.axis);

	ent.renderfx = RF_NOSHADOW;

	// flicker between two skins (FIXME?)
	ent.skinNum = (cg.time >> 6) & 1;

	// get the model, either as a bmodel or a modelindex
	if(s1->solid == SOLID_BMODEL)
		ent.hModel = cgs.inlinedrawmodel[s1->modelindex];
	else
		ent.hModel = cgs.gamemodels[s1->modelindex];

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);

	// add the secondary model
	if(s1->modelindex2){
		ent.skinNum = 0;
		ent.hModel = cgs.gamemodels[s1->modelindex2];
		// set frame
		anim = s1->anim;
		if(s1->nextanimtime != 0 && cg.time > s1->nextanimtime)
			anim = s1->nextanim;
		runlerpframe(cgs.anims[s1->modelindex], &cent->lerpframe, anim, 1.0f);
		ent.frame = cent->lerpframe.frame;
		ent.oldframe = cent->lerpframe.oldframe;
		ent.backlerp = cent->lerpframe.backlerp;
		trap_R_AddRefEntityToScene(&ent);
	}
}

/*
===============
drawbeam

Also called as an event
===============
*/
void
drawbeam(cent_t *cent)
{
	refEntity_t ent;
	entityState_t *s1;

	s1 = &cent->currstate;

	// create the render entity
	memset(&ent, 0, sizeof(ent));
	veccpy(s1->pos.trBase, ent.origin);
	veccpy(s1->origin2, ent.oldorigin);
	AxisClear(ent.axis);
	ent.reType = RT_BEAM;

	ent.renderfx = RF_NOSHADOW;

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);
}

/*
===============
doportal
===============
*/
static void
doportal(cent_t *cent)
{
	refEntity_t ent;
	entityState_t *s1;

	s1 = &cent->currstate;

	// create the render entity
	memset(&ent, 0, sizeof(ent));
	veccpy(cent->lerporigin, ent.origin);
	veccpy(s1->origin2, ent.oldorigin);
	ByteToDir(s1->eventParm, ent.axis[0]);
	vecperp(ent.axis[1], ent.axis[0]);

	// negating this tends to get the directions like they want
	// we really should have a camera roll value
	vecsub(vec3_origin, ent.axis[1], ent.axis[1]);

	veccross(ent.axis[0], ent.axis[1], ent.axis[2]);
	ent.reType = RT_PORTALSURFACE;
	ent.oldframe = s1->powerups;
	ent.frame = s1->frame;				// rotation speed
	ent.skinNum = s1->clientNum/256.0 * 360;	// roll offset

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);
}

/*
=========================
adjustposformover

Also called by client movement prediction code
=========================
*/
void
adjustposformover(const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out, vec3_t angles_in, vec3_t angles_out)
{
	cent_t *cent;
	vec3_t oldorigin, origin, deltaOrigin;
	vec3_t oldAngles, angles, deltaAngles;

	if(moverNum <= 0 || moverNum >= ENTITYNUM_MAX_NORMAL){
		veccpy(in, out);
		veccpy(angles_in, angles_out);
		return;
	}

	cent = &cg_entities[moverNum];
	if(cent->currstate.eType != ET_MOVER && cent->currstate.eType != ET_NPC){
		veccpy(in, out);
		veccpy(angles_in, angles_out);
		return;
	}

	evaltrajectory(&cent->currstate.pos, fromTime, oldorigin);
	evaltrajectory(&cent->currstate.apos, fromTime, oldAngles);

	evaltrajectory(&cent->currstate.pos, toTime, origin);
	evaltrajectory(&cent->currstate.apos, toTime, angles);

	vecsub(origin, oldorigin, deltaOrigin);
	vecsub(angles, oldAngles, deltaAngles);

	vecadd(in, deltaOrigin, out);
	vecadd(angles_in, deltaAngles, angles_out);
	// FIXME: origin change when on a rotating object
}

/*
=============================
interpolateentityposition
=============================
*/
static void
interpolateentityposition(cent_t *cent)
{
	vec3_t current, next;
	float f;

	// it would be an internal error to find an entity that interpolates without
	// a snapshot ahead of the current one
	if(cg.nextsnap == nil)
		cgerrorf("interpoateentityposition: cg.nextsnap == nil");

	f = cg.frameinterpolation;

	// this will linearize a sine or parabolic curve, but it is important
	// to not extrapolate player positions if more recent data is available
	evaltrajectory(&cent->currstate.pos, cg.snap->serverTime, current);
	evaltrajectory(&cent->nextstate.pos, cg.nextsnap->serverTime, next);

	cent->lerporigin[0] = current[0] + f * (next[0] - current[0]);
	cent->lerporigin[1] = current[1] + f * (next[1] - current[1]);
	cent->lerporigin[2] = current[2] + f * (next[2] - current[2]);

	evaltrajectory(&cent->currstate.apos, cg.snap->serverTime, current);
	evaltrajectory(&cent->nextstate.apos, cg.nextsnap->serverTime, next);

	cent->lerpangles[0] = LerpAngle(current[0], next[0], f);
	cent->lerpangles[1] = LerpAngle(current[1], next[1], f);
	cent->lerpangles[2] = LerpAngle(current[2], next[2], f);
}

/*
===============
calcentitylerppositions

===============
*/
static void
calcentitylerppositions(cent_t *cent)
{
	// if this player does not want to see extrapolated players
	if(!cg_smoothClients.integer)
		// make sure the clients use TR_INTERPOLATE
		if(cent->currstate.number < MAX_CLIENTS){
			cent->currstate.pos.trType = TR_INTERPOLATE;
			cent->nextstate.pos.trType = TR_INTERPOLATE;
		}

	if(cent->interpolate && cent->currstate.pos.trType == TR_INTERPOLATE){
		interpolateentityposition(cent);
		return;
	}

	// first see if we can interpolate between two snaps for
	// linear extrapolated clients
	if(cent->interpolate && cent->currstate.pos.trType == TR_LINEAR_STOP &&
	   cent->currstate.number < MAX_CLIENTS){
		interpolateentityposition(cent);
		return;
	}

	// just use the current frame and evaluate as best we can
	evaltrajectory(&cent->currstate.pos, cg.time, cent->lerporigin);
	evaltrajectory(&cent->currstate.apos, cg.time, cent->lerpangles);

	// adjust for riding a mover if it wasn't rolled into the predicted
	// player state
	if(cent != &cg.pplayerent)
		adjustposformover(cent->lerporigin, cent->currstate.groundEntityNum,
					  cg.snap->serverTime, cg.time, cent->lerporigin, cent->lerpangles, cent->lerpangles);
}

/*
===============
doteambase
===============
*/
static void
doteambase(cent_t *cent)
{
	refEntity_t model;
	if(cgs.gametype == GT_CTF){
		// show the flag base
		memset(&model, 0, sizeof(model));
		model.reType = RT_MODEL;
		veccpy(cent->lerporigin, model.lightingOrigin);
		veccpy(cent->lerporigin, model.origin);
		AnglesToAxis(cent->currstate.angles, model.axis);
		if(cent->currstate.modelindex == TEAM_RED)
			model.hModel = cgs.media.redFlagBaseModel;
		else if(cent->currstate.modelindex == TEAM_BLUE)
			model.hModel = cgs.media.blueFlagBaseModel;
		else
			model.hModel = cgs.media.neutralFlagBaseModel;
		trap_R_AddRefEntityToScene(&model);
	}
}

/*
===============
addcentity

===============
*/
static void
addcentity(cent_t *cent)
{
	vec3_t v;

	// event-only entities will have been dealt with already
	if(cent->currstate.eType >= ET_EVENTS)
		return;

	// calculate the current origin
	calcentitylerppositions(cent);

	// add automatic effects
	entfx(cent);

	switch(cent->currstate.eType){
	default:
		cgerrorf("Bad entity type: %i", cent->currstate.eType);
		break;
	case ET_INVISIBLE:
	case ET_PUSH_TRIGGER:
	case ET_TELEPORT_TRIGGER:
		break;
	case ET_GENERAL:
		dogeneral(cent);
		break;
	case ET_PLAYER:
		doplayer(cent);
		break;
	case ET_ITEM:
		doitem(cent);
		break;
	case ET_MISSILE:
		domissile(cent);
		break;
	case ET_MOVER:
		domover(cent);
		break;
	case ET_NPC:
		dogeneralshadowed(cent);
		break;
	case ET_CRATE:
	case ET_CRATE_BOUNCY:
		dogeneralshadowed(cent);
		break;
	case ET_CHECKPOINTHALO:
		dogeneral(cent);
		veccpy(cent->lerporigin, v);
		v[2] += 3.0f;
		trap_R_AddAdditiveLightToScene(v,
		   200 + 25 * sin(1000 + cg.time/600.0f),
		   0.03f, 0.058f, 0.091f);
		v[2] += 20.0f;
		trap_R_AddAdditiveLightToScene(v,
		   190 + 10 * sin(cg.time/970.82f),
		   0.03f, 0.058f, 0.091f);
		break;
	case ET_BEAM:
		drawbeam(cent);
		break;
	case ET_PORTAL:
		doportal(cent);
		break;
	case ET_SPEAKER:
		dospeaker(cent);
		break;
	case ET_GRAPPLE:
		dograpple(cent);
		break;
	case ET_TEAM:
		doteambase(cent);
		break;
	}
}

/*
===============
addpacketents

===============
*/
void
addpacketents(void)
{
	int num;
	cent_t *cent;
	playerState_t *ps;

	// set cg.frameinterpolation
	if(cg.nextsnap){
		int delta;

		delta = (cg.nextsnap->serverTime - cg.snap->serverTime);
		if(delta == 0)
			cg.frameinterpolation = 0;
		else
			cg.frameinterpolation = (float)(cg.time - cg.snap->serverTime) / delta;
	}else
		cg.frameinterpolation = 0;	// actually, it should never be used, because
		// no entities should be marked as interpolating

	// the auto-rotating items will all have the same axis
	cg.autoangles[0] = 0;
	cg.autoangles[1] = (cg.time & 2047) * 360 / 2048.0;
	cg.autoangles[2] = 0;

	cg.autoanglesfast[0] = 0;
	cg.autoanglesfast[1] = (cg.time & 1023) * 360 / 1024.0f;
	cg.autoanglesfast[2] = 0;

	AnglesToAxis(cg.autoangles, cg.autoaxis);
	AnglesToAxis(cg.autoanglesfast, cg.autoaxisfast);

	// generate and add the entity from the playerstate
	ps = &cg.pps;
	playerstate2entstate(ps, &cg.pplayerent.currstate, qfalse);
	addcentity(&cg.pplayerent);

	// lerp the non-predicted value for lightning gun origins
	calcentitylerppositions(&cg_entities[cg.snap->ps.clientNum]);

	// add each entity sent over by the server
	for(num = 0; num < cg.snap->numEntities; num++){
		cent = &cg_entities[cg.snap->entities[num].number];
		addcentity(cent);
	}
}
