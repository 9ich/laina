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
// cg_effects.c -- these functions generate localentities, usually as a result
// of event processing

#include "cg_local.h"

/*
==================
launchgib
==================
*/
static void
launchgib(vec3_t origin, vec3_t velocity, qhandle_t hModel)
{
	localent_t *le;
	refEntity_t *re;

	le = alloclocalent();
	re = &le->refEntity;

	le->type = LE_FRAGMENT;
	le->starttime = cg.time;
	le->endtime = le->starttime + 5000 + random() * 3000;

	veccpy(origin, re->origin);
	AxisCopy(axisDefault, re->axis);
	re->hModel = hModel;

	le->pos.trType = TR_GRAVITY;
	veccpy(origin, le->pos.trBase);
	veccpy(velocity, le->pos.trDelta);
	le->pos.trTime = cg.time;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[0] = rand();
	le->angles.trBase[1] = rand();
	le->angles.trBase[2] = rand();
	le->angles.trDelta[0] = -400 + random()*800;
	le->angles.trDelta[1] = -400 + random()*800;
	le->angles.trDelta[2] = -400 + random()*800;

	le->bouncefactor = 0.6f;

	le->bouncesoundtype = LEBS_BLOOD;
	le->flags = LEF_TUMBLE;
	le->marktype = LEMT_BLOOD;
}

static void
launchsplinter(vec3_t origin, vec3_t velocity, qhandle_t hModel)
{
	static int seed = 17;
	localent_t *le;
	refEntity_t *re;

	le = alloclocalent();
	re = &le->refEntity;

	le->type = LE_FRAGMENT;
	le->starttime = cg.time;
	le->endtime = le->starttime + 2000 + Q_random(&seed) * 3000;

	veccpy(origin, re->origin);
	AxisCopy(axisDefault, re->axis);
	re->hModel = hModel;

	le->pos.trType = TR_GRAVITY;
	veccpy(origin, le->pos.trBase);
	veccpy(velocity, le->pos.trDelta);
	le->pos.trTime = cg.time;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[0] = Q_random(&seed);
	le->angles.trBase[1] = Q_random(&seed);
	le->angles.trBase[2] = Q_random(&seed);
	le->angles.trDelta[0] = -1000 + Q_random(&seed)*2000;
	le->angles.trDelta[1] = -1000 + Q_random(&seed)*2000;
	le->angles.trDelta[2] = -1000 + Q_random(&seed)*2000;

	le->bouncefactor = 0.4f;

	le->bouncesoundtype = LEBS_WOOD;
	le->flags = LEF_TUMBLE;
}

/*
==================
bubbletrail

Bullets shot underwater
==================
*/
void
bubbletrail(vec3_t start, vec3_t end, float spacing)
{
	vec3_t move;
	vec3_t vec;
	float len;
	int i;

	if(cg_noProjectileTrail.integer)
		return;

	veccpy(start, move);
	vecsub(end, start, vec);
	len = vecnorm(vec);

	// advance a random amount first
	i = rand() % (int)spacing;
	vecmad(move, i, vec, move);

	vecmul(vec, spacing, vec);

	for(; i < len; i += spacing){
		localent_t *le;
		refEntity_t *re;

		le = alloclocalent();
		le->flags = LEF_PUFF_DONT_SCALE;
		le->type = LE_MOVE_SCALE_FADE;
		le->starttime = cg.time;
		le->endtime = cg.time + 1000 + random() * 250;
		le->liferate = 1.0 / (le->endtime - le->starttime);

		re = &le->refEntity;
		re->shaderTime = cg.time / 1000.0f;

		re->reType = RT_SPRITE;
		re->rotation = 0;
		re->radius = 3;
		re->customShader = cgs.media.waterBubbleShader;
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0xff;
		re->shaderRGBA[2] = 0xff;
		re->shaderRGBA[3] = 0xff;

		le->color[3] = 1.0;

		le->pos.trType = TR_LINEAR;
		le->pos.trTime = cg.time;
		veccpy(move, le->pos.trBase);
		le->pos.trDelta[0] = crandom()*5;
		le->pos.trDelta[1] = crandom()*5;
		le->pos.trDelta[2] = crandom()*5 + 6;

		vecadd(move, vec, move);
	}
}

/*
=====================
smokepuff

Adds a smoke puff or blood trail localEntity.
=====================
*/
localent_t *
smokepuff(const vec3_t p, const vec3_t vel,
	     float radius,
	     float r, float g, float b, float a,
	     float duration,
	     int starttime,
	     int fadeintime,
	     int flags,
	     qhandle_t hShader)
{
	static int seed = 0x92;
	localent_t *le;
	refEntity_t *re;
//	int fadeintime = starttime + duration / 2;

	le = alloclocalent();
	le->flags = flags;
	le->radius = radius;

	re = &le->refEntity;
	re->rotation = Q_random(&seed) * 360;
	re->radius = radius;
	re->shaderTime = starttime / 1000.0f;

	le->type = LE_MOVE_SCALE_FADE;
	le->starttime = starttime;
	le->fadeintime = fadeintime;
	le->endtime = starttime + duration;
	if(fadeintime > starttime)
		le->liferate = 1.0 / (le->endtime - le->fadeintime);
	else
		le->liferate = 1.0 / (le->endtime - le->starttime);
	le->color[0] = r;
	le->color[1] = g;
	le->color[2] = b;
	le->color[3] = a;

	le->pos.trType = TR_LINEAR;
	le->pos.trTime = starttime;
	veccpy(vel, le->pos.trDelta);
	veccpy(p, le->pos.trBase);

	veccpy(p, re->origin);
	re->customShader = hShader;

	// rage pro can't alpha fade, so use a different shader
	if(cgs.glconfig.hardwareType == GLHW_RAGEPRO){
		re->customShader = cgs.media.smokePuffRageProShader;
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0xff;
		re->shaderRGBA[2] = 0xff;
		re->shaderRGBA[3] = 0xff;
	}else{
		re->shaderRGBA[0] = le->color[0] * 0xff;
		re->shaderRGBA[1] = le->color[1] * 0xff;
		re->shaderRGBA[2] = le->color[2] * 0xff;
		re->shaderRGBA[3] = 0xff;
	}

	re->reType = RT_SPRITE;
	re->radius = le->radius;

	return le;
}

enum {
	NSPLINTERS = 10,
	SPLINTERTIME = 1000
 };

void
cratesmash(vec3_t pos)
{
	static int seed = 0xc47;
	vec3_t pt, vel = {0, 0, -10};
	int i;

	veccpy(pos, pt);
	pt[2] += 16;	// reposition to crate's centre
	smokepuff(pt, vel, 64, 0.85f, 0.58f, 0.44f, 1.0f, 300, cg.time,
	   0, LEF_PUFF_DONT_SCALE, cgs.media.smokePuffShader);
	pt[0] += -4 + 8*Q_random(&seed);
	pt[1] += -4 + 8*Q_random(&seed);
	pt[2] += -4 + 8*Q_random(&seed);
	smokepuff(pt, vel, 100, 0.85f, 0.58f, 0.44f, 0.7f, 700, cg.time,
	   0, 0, cgs.media.smokePuffShader);
	for(i = 0; i < NSPLINTERS; i++){
		vel[0] = Q_crandom(&seed) * 400;
		vel[1] = Q_crandom(&seed) * 400;
		vel[2] = Q_crandom(&seed) * 400;
		launchsplinter(pt, vel, cgs.media.splinter);
	}
	trap_S_StartSound(pos, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.crateSmash);
}

void
tntexplode(vec3_t pos)
{
	vec3_t pt, up, vel;
	localent_t *xplo;

	veccpy(pos, pt);
	vecset(up, 0, 0, 1);
	vecset(vel, 0, 0, 16);

	trap_S_StartSound(pt, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.tntExpSound);

	xplo = explosion(pt, up, cgs.media.tntExpModel, cgs.media.tntExpShader,
	   1000, qtrue);
	xplo->light = 100;
	vecset(xplo->lightcolor, 1.0f, 0.8f, 0.4f);

	impactmark(cgs.media.burnMarkShader, pt, up, random()*360, 1, 1, 1,
	   1, qfalse, 100, qfalse);

	pt[2] += 24.0f;
	smokepuff(pt, vel, 64, 0.0f, 0.0f, 0.0, 1.0f, 3000, cg.time,
	   0, LEF_PUFF_DONT_SCALE, cgs.media.smokePuffShader);
}

/*
==================
spawneffect

Player teleporting in or out
==================
*/
void
spawneffect(vec3_t org)
{
	localent_t *le;
	refEntity_t *re;

	le = alloclocalent();
	le->flags = 0;
	le->type = LE_FADE_RGB;
	le->starttime = cg.time;
	le->endtime = cg.time + 500;
	le->liferate = 1.0 / (le->endtime - le->starttime);

	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

	re = &le->refEntity;

	re->reType = RT_MODEL;
	re->shaderTime = cg.time / 1000.0f;

	re->customShader = cgs.media.teleportEffectShader;
	re->hModel = cgs.media.teleportEffectModel;
	AxisClear(re->axis);

	veccpy(org, re->origin);
	re->origin[2] -= 24;
}


/*
==================
scoreplum
==================
*/
void
scoreplum(int client, vec3_t org, int score)
{
	localent_t *le;
	refEntity_t *re;
	vec3_t angles;
	static vec3_t lastPos;

	// only visualize for the client that scored
	if(client != cg.pps.clientNum || cg_scorePlum.integer == 0)
		return;

	le = alloclocalent();
	le->flags = 0;
	le->type = LE_SCOREPLUM;
	le->starttime = cg.time;
	le->endtime = cg.time + 4000;
	le->liferate = 1.0 / (le->endtime - le->starttime);

	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;
	le->radius = score;

	veccpy(org, le->pos.trBase);
	if(org[2] >= lastPos[2] - 20 && org[2] <= lastPos[2] + 20)
		le->pos.trBase[2] -= 20;

	//cgprintf( "Plum origin %i %i %i -- %i\n", (int)org[0], (int)org[1], (int)org[2], (int)vecdist(org, lastPos));
	veccpy(org, lastPos);

	re = &le->refEntity;

	re->reType = RT_SPRITE;
	re->radius = 16;

	vecclear(angles);
	AnglesToAxis(angles, re->axis);
}

/*
====================
explosion
====================
*/
localent_t *
explosion(vec3_t origin, vec3_t dir,
		 qhandle_t hModel, qhandle_t shader,
		 int msec, qboolean isSprite)
{
	float ang;
	localent_t *ex;
	int offset;
	vec3_t tmpVec, newOrigin;

	if(msec <= 0)
		cgerrorf("explosion: msec = %i", msec);

	// skew the time a bit so they aren't all in sync
	offset = rand() & 63;

	ex = alloclocalent();
	if(isSprite){
		ex->type = LE_SPRITE_EXPLOSION;

		// randomly rotate sprite orientation
		ex->refEntity.rotation = rand() % 360;
		vecmul(dir, 16, tmpVec);
		vecadd(tmpVec, origin, newOrigin);
	}else{
		ex->type = LE_EXPLOSION;
		veccpy(origin, newOrigin);

		// set axis with random rotate
		if(!dir)
			AxisClear(ex->refEntity.axis);
		else{
			ang = rand() % 360;
			veccpy(dir, ex->refEntity.axis[0]);
			RotateAroundDirection(ex->refEntity.axis, ang);
		}
	}

	ex->starttime = cg.time - offset;
	ex->endtime = ex->starttime + msec;

	// bias the time so all shader effects start correctly
	ex->refEntity.shaderTime = ex->starttime / 1000.0f;

	ex->refEntity.hModel = hModel;
	ex->refEntity.customShader = shader;

	// set origin
	veccpy(newOrigin, ex->refEntity.origin);
	veccpy(newOrigin, ex->refEntity.oldorigin);

	ex->color[0] = ex->color[1] = ex->color[2] = 1.0;

	return ex;
}

/*
=================
bleed

This is the spurt of blood when a character gets hit
=================
*/
void
bleed(vec3_t origin, int entityNum)
{
	localent_t *ex;

	if(!cg_blood.integer)
		return;

	ex = alloclocalent();
	ex->type = LE_EXPLOSION;

	ex->starttime = cg.time;
	ex->endtime = ex->starttime + 500;

	veccpy(origin, ex->refEntity.origin);
	ex->refEntity.reType = RT_SPRITE;
	ex->refEntity.rotation = rand() % 360;
	ex->refEntity.radius = 24;

	ex->refEntity.customShader = cgs.media.bloodExplosionShader;

	// don't show player's own blood in view
	if(entityNum == cg.snap->ps.clientNum)
		ex->refEntity.renderfx |= RF_THIRD_PERSON;
}

/*
===================
gibplayer

Generated a bunch of gibs launching out from the bodies location
===================
*/
#define GIB_VELOCITY	250
#define GIB_JUMP	250
void
gibplayer(vec3_t playerpos)
{
	vec3_t pos, vel;
	int i;

	if(!cg_blood.integer)
		return;
	if(!cg_gibs.integer)
		return;

	for(i = 0; i < 9; i++){
		veccpy(playerpos, pos);
		vel[0] = crandom()*GIB_VELOCITY;
		vel[1] = crandom()*GIB_VELOCITY;
		vel[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
		launchgib(pos, vel, cgs.media.gib);
	}
}

/*
==================
launchexplode
==================
*/
void
launchexplode(vec3_t origin, vec3_t velocity, qhandle_t hModel)
{
	localent_t *le;
	refEntity_t *re;

	le = alloclocalent();
	re = &le->refEntity;

	le->type = LE_FRAGMENT;
	le->starttime = cg.time;
	le->endtime = le->starttime + 10000 + random() * 6000;

	veccpy(origin, re->origin);
	AxisCopy(axisDefault, re->axis);
	re->hModel = hModel;

	le->pos.trType = TR_GRAVITY;
	veccpy(origin, le->pos.trBase);
	veccpy(velocity, le->pos.trDelta);
	le->pos.trTime = cg.time;

	le->bouncefactor = 0.1f;

	le->bouncesoundtype = LEBS_BRASS;
	le->marktype = LEMT_NONE;
}

#define EXP_VELOCITY	100
#define EXP_JUMP	150
/*
===================
bigexplode

Generated a bunch of gibs launching out from the bodies location
===================
*/
void
bigexplode(vec3_t playerOrigin)
{
	vec3_t origin, velocity;

	if(!cg_blood.integer)
		return;

	veccpy(playerOrigin, origin);
	velocity[0] = crandom()*EXP_VELOCITY;
	velocity[1] = crandom()*EXP_VELOCITY;
	velocity[2] = EXP_JUMP + crandom()*EXP_VELOCITY;
	launchexplode(origin, velocity, cgs.media.smoke2);

	veccpy(playerOrigin, origin);
	velocity[0] = crandom()*EXP_VELOCITY;
	velocity[1] = crandom()*EXP_VELOCITY;
	velocity[2] = EXP_JUMP + crandom()*EXP_VELOCITY;
	launchexplode(origin, velocity, cgs.media.smoke2);

	veccpy(playerOrigin, origin);
	velocity[0] = crandom()*EXP_VELOCITY*1.5;
	velocity[1] = crandom()*EXP_VELOCITY*1.5;
	velocity[2] = EXP_JUMP + crandom()*EXP_VELOCITY;
	launchexplode(origin, velocity, cgs.media.smoke2);

	veccpy(playerOrigin, origin);
	velocity[0] = crandom()*EXP_VELOCITY*2.0;
	velocity[1] = crandom()*EXP_VELOCITY*2.0;
	velocity[2] = EXP_JUMP + crandom()*EXP_VELOCITY;
	launchexplode(origin, velocity, cgs.media.smoke2);

	veccpy(playerOrigin, origin);
	velocity[0] = crandom()*EXP_VELOCITY*2.5;
	velocity[1] = crandom()*EXP_VELOCITY*2.5;
	velocity[2] = EXP_JUMP + crandom()*EXP_VELOCITY;
	launchexplode(origin, velocity, cgs.media.smoke2);
}
