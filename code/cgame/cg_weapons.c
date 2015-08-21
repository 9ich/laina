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
// cg_weapons.c -- events and effects dealing with weapons
#include "cg_local.h"

/*
==========================
machinegunejectbrass
==========================
*/
static void
machinegunejectbrass(cent_t *cent)
{
	localent_t *le;
	refEntity_t *re;
	vec3_t velocity, xvelocity;
	vec3_t offset, xoffset;
	float waterScale = 1.0f;
	vec3_t v[3];

	if(cg_brassTime.integer <= 0)
		return;

	le = alloclocalent();
	re = &le->refEntity;

	velocity[0] = 0;
	velocity[1] = -50 + 40 * crandom();
	velocity[2] = 100 + 50 * crandom();

	le->type = LE_FRAGMENT;
	le->starttime = cg.time;
	le->endtime = le->starttime + cg_brassTime.integer + (cg_brassTime.integer / 4) * random();

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - (rand()&15);

	AnglesToAxis(cent->lerpangles, v);

	offset[0] = 8;
	offset[1] = -4;
	offset[2] = 24;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
	vecadd(cent->lerporigin, xoffset, re->origin);

	veccopy(re->origin, le->pos.trBase);

	if(pointcontents(re->origin, -1) & CONTENTS_WATER)
		waterScale = 0.10f;

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	vecscale(xvelocity, waterScale, le->pos.trDelta);

	AxisCopy(axisDefault, re->axis);
	re->hModel = cgs.media.machinegunBrassModel;

	le->bouncefactor = 0.4 * waterScale;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[0] = rand()&31;
	le->angles.trBase[1] = rand()&31;
	le->angles.trBase[2] = rand()&31;
	le->angles.trDelta[0] = 2;
	le->angles.trDelta[1] = 1;
	le->angles.trDelta[2] = 0;

	le->flags = LEF_TUMBLE;
	le->bouncesoundtype = LEBS_BRASS;
	le->marktype = LEMT_NONE;
}

/*
==========================
shotgunejectbrass
==========================
*/
static void
shotgunejectbrass(cent_t *cent)
{
	localent_t *le;
	refEntity_t *re;
	vec3_t velocity, xvelocity;
	vec3_t offset, xoffset;
	vec3_t v[3];
	int i;

	if(cg_brassTime.integer <= 0)
		return;

	for(i = 0; i < 2; i++){
		float waterScale = 1.0f;

		le = alloclocalent();
		re = &le->refEntity;

		velocity[0] = 60 + 60 * crandom();
		if(i == 0)
			velocity[1] = 40 + 10 * crandom();
		else
			velocity[1] = -40 + 10 * crandom();
		velocity[2] = 100 + 50 * crandom();

		le->type = LE_FRAGMENT;
		le->starttime = cg.time;
		le->endtime = le->starttime + cg_brassTime.integer*3 + cg_brassTime.integer * random();

		le->pos.trType = TR_GRAVITY;
		le->pos.trTime = cg.time;

		AnglesToAxis(cent->lerpangles, v);

		offset[0] = 8;
		offset[1] = 0;
		offset[2] = 24;

		xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
		xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
		xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
		vecadd(cent->lerporigin, xoffset, re->origin);
		veccopy(re->origin, le->pos.trBase);
		if(pointcontents(re->origin, -1) & CONTENTS_WATER)
			waterScale = 0.10f;

		xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
		xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
		xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
		vecscale(xvelocity, waterScale, le->pos.trDelta);

		AxisCopy(axisDefault, re->axis);
		re->hModel = cgs.media.shotgunBrassModel;
		le->bouncefactor = 0.3f;

		le->angles.trType = TR_LINEAR;
		le->angles.trTime = cg.time;
		le->angles.trBase[0] = rand()&31;
		le->angles.trBase[1] = rand()&31;
		le->angles.trBase[2] = rand()&31;
		le->angles.trDelta[0] = 1;
		le->angles.trDelta[1] = 0.5;
		le->angles.trDelta[2] = 0;

		le->flags = LEF_TUMBLE;
		le->bouncesoundtype = LEBS_BRASS;
		le->marktype = LEMT_NONE;
	}
}


/*
==========================
dorailtrail
==========================
*/
void
dorailtrail(clientinfo_t *ci, vec3_t start, vec3_t end)
{
	vec3_t axis[36], move, move2, vec, temp;
	float len;
	int i, j, skip;

	localent_t *le;
	refEntity_t *re;

#define RADIUS		4
#define ROTATION	1
#define SPACING		5

	start[2] -= 4;

	le = alloclocalent();
	re = &le->refEntity;

	le->type = LE_FADE_RGB;
	le->starttime = cg.time;
	le->endtime = cg.time + cg_railTrailTime.value;
	le->liferate = 1.0 / (le->endtime - le->starttime);

	re->shaderTime = cg.time / 1000.0f;
	re->reType = RT_RAIL_CORE;
	re->customShader = cgs.media.railCoreShader;

	veccopy(start, re->origin);
	veccopy(end, re->oldorigin);

	re->shaderRGBA[0] = ci->color1[0] * 255;
	re->shaderRGBA[1] = ci->color1[1] * 255;
	re->shaderRGBA[2] = ci->color1[2] * 255;
	re->shaderRGBA[3] = 255;

	le->color[0] = ci->color1[0] * 0.75;
	le->color[1] = ci->color1[1] * 0.75;
	le->color[2] = ci->color1[2] * 0.75;
	le->color[3] = 1.0f;

	AxisClear(re->axis);

	if(cg_oldRail.integer){
		// nudge down a bit so it isn't exactly in center
		re->origin[2] -= 8;
		re->oldorigin[2] -= 8;
		return;
	}

	veccopy(start, move);
	vecsub(end, start, vec);
	len = vecnorm(vec);
	vecperp(temp, vec);
	for(i = 0; i < 36; i++)
		RotatePointAroundVector(axis[i], vec, temp, i * 10);	//banshee 2.4 was 10

	vecsadd(move, 20, vec, move);
	vecscale(vec, SPACING, vec);

	skip = -1;

	j = 18;
	for(i = 0; i < len; i += SPACING){
		if(i != skip){
			skip = i + SPACING;
			le = alloclocalent();
			re = &le->refEntity;
			le->flags = LEF_PUFF_DONT_SCALE;
			le->type = LE_MOVE_SCALE_FADE;
			le->starttime = cg.time;
			le->endtime = cg.time + (i>>1) + 600;
			le->liferate = 1.0 / (le->endtime - le->starttime);

			re->shaderTime = cg.time / 1000.0f;
			re->reType = RT_SPRITE;
			re->radius = 1.1f;
			re->customShader = cgs.media.railRingsShader;

			re->shaderRGBA[0] = ci->color2[0] * 255;
			re->shaderRGBA[1] = ci->color2[1] * 255;
			re->shaderRGBA[2] = ci->color2[2] * 255;
			re->shaderRGBA[3] = 255;

			le->color[0] = ci->color2[0] * 0.75;
			le->color[1] = ci->color2[1] * 0.75;
			le->color[2] = ci->color2[2] * 0.75;
			le->color[3] = 1.0f;

			le->pos.trType = TR_LINEAR;
			le->pos.trTime = cg.time;

			veccopy(move, move2);
			vecsadd(move2, RADIUS, axis[j], move2);
			veccopy(move2, le->pos.trBase);

			le->pos.trDelta[0] = axis[j][0]*6;
			le->pos.trDelta[1] = axis[j][1]*6;
			le->pos.trDelta[2] = axis[j][2]*6;
		}

		vecadd(move, vec, move);

		j = (j + ROTATION) % 36;
	}
}

/*
==========================
rockettrail
==========================
*/
static void
rockettrail(cent_t *ent, const weapinfo_t *wi)
{
	int step;
	vec3_t origin, lastPos;
	int t;
	int starttime, contents;
	int lastContents;
	entityState_t *es;
	vec3_t up;
	localent_t *smoke;

	if(cg_noProjectileTrail.integer)
		return;

	up[0] = 0;
	up[1] = 0;
	up[2] = 0;

	step = 50;

	es = &ent->currstate;
	starttime = ent->trailtime;
	t = step * ((starttime + step) / step);

	evaltrajectory(&es->pos, cg.time, origin);
	contents = pointcontents(origin, -1);

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if(es->pos.trType == TR_STATIONARY){
		ent->trailtime = cg.time;
		return;
	}

	evaltrajectory(&es->pos, ent->trailtime, lastPos);
	lastContents = pointcontents(lastPos, -1);

	ent->trailtime = cg.time;

	if(contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)){
		if(contents & lastContents & CONTENTS_WATER)
			bubbletrail(lastPos, origin, 8);
		return;
	}

	for(; t <= ent->trailtime; t += step){
		evaltrajectory(&es->pos, t, lastPos);

		smoke = smokepuff(lastPos, up,
				     wi->trailRadius,
				     1, 1, 1, 0.33f,
				     wi->wiTrailTime,
				     t,
				     0,
				     0,
				     cgs.media.smokePuffShader);
		// use the optimized local entity add
		smoke->type = LE_SCALE_FADE;
	}
}


/*
==========================
plasmatrail
==========================
*/
static void
plasmatrail(cent_t *cent, const weapinfo_t *wi)
{
	localent_t *le;
	refEntity_t *re;
	entityState_t *es;
	vec3_t velocity, xvelocity, origin;
	vec3_t offset, xoffset;
	vec3_t v[3];

	float waterScale = 1.0f;

	if(cg_noProjectileTrail.integer || cg_oldPlasma.integer)
		return;

	es = &cent->currstate;

	evaltrajectory(&es->pos, cg.time, origin);

	le = alloclocalent();
	re = &le->refEntity;

	velocity[0] = 60 - 120 * crandom();
	velocity[1] = 40 - 80 * crandom();
	velocity[2] = 100 - 200 * crandom();

	le->type = LE_MOVE_SCALE_FADE;
	le->flags = LEF_TUMBLE;
	le->bouncesoundtype = LEBS_NONE;
	le->marktype = LEMT_NONE;

	le->starttime = cg.time;
	le->endtime = le->starttime + 600;

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time;

	AnglesToAxis(cent->lerpangles, v);

	offset[0] = 2;
	offset[1] = 2;
	offset[2] = 2;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];

	vecadd(origin, xoffset, re->origin);
	veccopy(re->origin, le->pos.trBase);

	if(pointcontents(re->origin, -1) & CONTENTS_WATER)
		waterScale = 0.10f;

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	vecscale(xvelocity, waterScale, le->pos.trDelta);

	AxisCopy(axisDefault, re->axis);
	re->shaderTime = cg.time / 1000.0f;
	re->reType = RT_SPRITE;
	re->radius = 0.25f;
	re->customShader = cgs.media.railRingsShader;
	le->bouncefactor = 0.3f;

	re->shaderRGBA[0] = wi->flashDlightColor[0] * 63;
	re->shaderRGBA[1] = wi->flashDlightColor[1] * 63;
	re->shaderRGBA[2] = wi->flashDlightColor[2] * 63;
	re->shaderRGBA[3] = 63;

	le->color[0] = wi->flashDlightColor[0] * 0.2;
	le->color[1] = wi->flashDlightColor[1] * 0.2;
	le->color[2] = wi->flashDlightColor[2] * 0.2;
	le->color[3] = 0.25f;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[0] = rand()&31;
	le->angles.trBase[1] = rand()&31;
	le->angles.trBase[2] = rand()&31;
	le->angles.trDelta[0] = 1;
	le->angles.trDelta[1] = 0.5;
	le->angles.trDelta[2] = 0;
}

/*
==========================
grappletrail
==========================
*/
void
grappletrail(cent_t *ent, const weapinfo_t *wi)
{
	vec3_t origin;
	entityState_t *es;
	vec3_t forward, up;
	refEntity_t beam;

	es = &ent->currstate;

	evaltrajectory(&es->pos, cg.time, origin);
	ent->trailtime = cg.time;

	memset(&beam, 0, sizeof(beam));
	//FIXME adjust for muzzle position
	veccopy(cg_entities[ent->currstate.otherEntityNum].lerporigin, beam.origin);
	beam.origin[2] += 26;
	anglevecs(cg_entities[ent->currstate.otherEntityNum].lerpangles, forward, nil, up);
	vecsadd(beam.origin, -6, up, beam.origin);
	veccopy(origin, beam.oldorigin);

	if(vecdist(beam.origin, beam.oldorigin) < 64)
		return;		// Don't draw if close

	beam.reType = RT_LIGHTNING;
	beam.customShader = cgs.media.lightningShader;

	AxisClear(beam.axis);
	beam.shaderRGBA[0] = 0xff;
	beam.shaderRGBA[1] = 0xff;
	beam.shaderRGBA[2] = 0xff;
	beam.shaderRGBA[3] = 0xff;
	trap_R_AddRefEntityToScene(&beam);
}

/*
==========================
grenadetrail
==========================
*/
static void
grenadetrail(cent_t *ent, const weapinfo_t *wi)
{
	rockettrail(ent, wi);
}

/*
=================
registerweap

The server says this item is used on this level
=================
*/
void
registerweap(int weaponNum)
{
	weapinfo_t *weaponInfo;
	item_t *item, *ammo;
	char path[MAX_QPATH];
	vec3_t mins, maxs;
	int i;

	weaponInfo = &cg_weapons[weaponNum];

	if(weaponNum == 0)
		return;

	if(weaponInfo->registered)
		return;

	memset(weaponInfo, 0, sizeof(*weaponInfo));
	weaponInfo->registered = qtrue;

	for(item = bg_itemlist + 1; item->classname; item++)
		if(item->type == IT_WEAPON && item->tag == weaponNum){
			weaponInfo->item = item;
			break;
		}
	if(!item->classname)
		cgerrorf("Couldn't find weapon %i", weaponNum);
	registeritemgfx(item - bg_itemlist);

	// load cmodel before model so filecache works
	weaponInfo->weaponModel = trap_R_RegisterModel(item->model[0]);

	// calc midpoint for rotation
	trap_R_ModelBounds(weaponInfo->weaponModel, mins, maxs);
	for(i = 0; i < 3; i++)
		weaponInfo->weaponMidpoint[i] = mins[i] + 0.5 * (maxs[i] - mins[i]);

	weaponInfo->weaponIcon = trap_R_RegisterShader(item->icon);
	weaponInfo->ammoIcon = trap_R_RegisterShader(item->icon);

	for(ammo = bg_itemlist + 1; ammo->classname; ammo++)
		if(ammo->type == IT_AMMO && ammo->tag == weaponNum)
			break;
	if(ammo->classname && ammo->model[0])
		weaponInfo->ammoModel = trap_R_RegisterModel(ammo->model[0]);

	COM_StripExtension(item->model[0], path, sizeof(path));
	Q_strcat(path, sizeof(path), "_flash.md3");
	weaponInfo->flashModel = trap_R_RegisterModel(path);

	COM_StripExtension(item->model[0], path, sizeof(path));
	Q_strcat(path, sizeof(path), "_barrel.md3");
	weaponInfo->barrelModel = trap_R_RegisterModel(path);

	COM_StripExtension(item->model[0], path, sizeof(path));
	Q_strcat(path, sizeof(path), "_hand.md3");
	weaponInfo->handsModel = trap_R_RegisterModel(path);

	if(!weaponInfo->handsModel)
		weaponInfo->handsModel = trap_R_RegisterModel("models/weapons2/shotgun/shotgun_hand.md3");

	switch(weaponNum){
	case WP_GAUNTLET:
		MAKERGB(weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f);
		weaponInfo->firingsound = trap_S_RegisterSound("sound/weapons/melee/fstrun.wav", qfalse);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/melee/fstatck.wav", qfalse);
		break;

	case WP_LIGHTNING:
		MAKERGB(weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f);
		weaponInfo->readysound = trap_S_RegisterSound("sound/weapons/melee/fsthum.wav", qfalse);
		weaponInfo->firingsound = trap_S_RegisterSound("sound/weapons/lightning/lg_hum.wav", qfalse);

		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/lightning/lg_fire.wav", qfalse);
		cgs.media.lightningShader = trap_R_RegisterShader("lightningBoltNew");
		cgs.media.lightningExplosionModel = trap_R_RegisterModel("models/weaphits/crackle.md3");
		cgs.media.sfx_lghit1 = trap_S_RegisterSound("sound/weapons/lightning/lg_hit.wav", qfalse);
		cgs.media.sfx_lghit2 = trap_S_RegisterSound("sound/weapons/lightning/lg_hit2.wav", qfalse);
		cgs.media.sfx_lghit3 = trap_S_RegisterSound("sound/weapons/lightning/lg_hit3.wav", qfalse);

		break;

	case WP_GRAPPLING_HOOK:
		MAKERGB(weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f);
		weaponInfo->missilemodel = trap_R_RegisterModel("models/ammo/rocket/rocket.md3");
		weaponInfo->missileTrailFunc = grappletrail;
		weaponInfo->missilelight = 200;
		MAKERGB(weaponInfo->missilelightcolor, 1, 0.75f, 0);
		weaponInfo->readysound = trap_S_RegisterSound("sound/weapons/melee/fsthum.wav", qfalse);
		weaponInfo->firingsound = trap_S_RegisterSound("sound/weapons/melee/fstrun.wav", qfalse);
		cgs.media.lightningShader = trap_R_RegisterShader("lightningBoltNew");
		break;


	case WP_MACHINEGUN:
		MAKERGB(weaponInfo->flashDlightColor, 1, 1, 0);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/machinegun/machgf1b.wav", qfalse);
		weaponInfo->flashSound[1] = trap_S_RegisterSound("sound/weapons/machinegun/machgf2b.wav", qfalse);
		weaponInfo->flashSound[2] = trap_S_RegisterSound("sound/weapons/machinegun/machgf3b.wav", qfalse);
		weaponInfo->flashSound[3] = trap_S_RegisterSound("sound/weapons/machinegun/machgf4b.wav", qfalse);
		weaponInfo->ejectBrassFunc = machinegunejectbrass;
		cgs.media.bulletExplosionShader = trap_R_RegisterShader("bulletExplosion");
		break;

	case WP_SHOTGUN:
		MAKERGB(weaponInfo->flashDlightColor, 1, 1, 0);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/shotgun/sshotf1b.wav", qfalse);
		weaponInfo->ejectBrassFunc = shotgunejectbrass;
		break;

	case WP_ROCKET_LAUNCHER:
		weaponInfo->missilemodel = trap_R_RegisterModel("models/ammo/rocket/rocket.md3");
		weaponInfo->missilesound = trap_S_RegisterSound("sound/weapons/rocket/rockfly.wav", qfalse);
		weaponInfo->missileTrailFunc = rockettrail;
		weaponInfo->missilelight = 200;
		weaponInfo->wiTrailTime = 2000;
		weaponInfo->trailRadius = 64;

		MAKERGB(weaponInfo->missilelightcolor, 1, 0.75f, 0);
		MAKERGB(weaponInfo->flashDlightColor, 1, 0.75f, 0);

		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/rocket/rocklf1a.wav", qfalse);
		cgs.media.rocketExplosionShader = trap_R_RegisterShader("rocketExplosion");
		break;


	case WP_GRENADE_LAUNCHER:
		weaponInfo->missilemodel = trap_R_RegisterModel("models/ammo/grenade1.md3");
		weaponInfo->missileTrailFunc = grenadetrail;
		weaponInfo->wiTrailTime = 700;
		weaponInfo->trailRadius = 32;
		MAKERGB(weaponInfo->flashDlightColor, 1, 0.70f, 0);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/grenade/grenlf1a.wav", qfalse);
		cgs.media.grenadeExplosionShader = trap_R_RegisterShader("grenadeExplosion");
		break;


	case WP_PLASMAGUN:
//		weaponInfo->missilemodel = cgs.media.invulnerabilityPowerupModel;
		weaponInfo->missileTrailFunc = plasmatrail;
		weaponInfo->missilesound = trap_S_RegisterSound("sound/weapons/plasma/lasfly.wav", qfalse);
		MAKERGB(weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/plasma/hyprbf1a.wav", qfalse);
		cgs.media.plasmaExplosionShader = trap_R_RegisterShader("plasmaExplosion");
		cgs.media.railRingsShader = trap_R_RegisterShader("railDisc");
		break;

	case WP_RAILGUN:
		weaponInfo->readysound = trap_S_RegisterSound("sound/weapons/railgun/rg_hum.wav", qfalse);
		MAKERGB(weaponInfo->flashDlightColor, 1, 0.5f, 0);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/railgun/railgf1a.wav", qfalse);
		cgs.media.railExplosionShader = trap_R_RegisterShader("railExplosion");
		cgs.media.railRingsShader = trap_R_RegisterShader("railDisc");
		cgs.media.railCoreShader = trap_R_RegisterShader("railCore");
		break;

	case WP_BFG:
		weaponInfo->readysound = trap_S_RegisterSound("sound/weapons/bfg/bfg_hum.wav", qfalse);
		MAKERGB(weaponInfo->flashDlightColor, 1, 0.7f, 1);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/bfg/bfg_fire.wav", qfalse);
		cgs.media.bfgExplosionShader = trap_R_RegisterShader("bfgExplosion");
		weaponInfo->missilemodel = trap_R_RegisterModel("models/weaphits/bfg.md3");
		weaponInfo->missilesound = trap_S_RegisterSound("sound/weapons/rocket/rockfly.wav", qfalse);
		break;

	default:
		MAKERGB(weaponInfo->flashDlightColor, 1, 1, 1);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/rocket/rocklf1a.wav", qfalse);
		break;
	}
}

/*
=================
registeritemgfx

The server says this item is used on this level
=================
*/
void
registeritemgfx(int itemNum)
{
	iteminfo_t *itemInfo;
	item_t *item;

	if(itemNum < 0 || itemNum >= bg_nitems)
		cgerrorf("registeritemgfx: itemNum %d out of range [0-%d]", itemNum, bg_nitems-1);

	itemInfo = &cg_items[itemNum];
	if(itemInfo->registered)
		return;

	item = &bg_itemlist[itemNum];

	memset(itemInfo, 0, sizeof(*itemInfo));
	itemInfo->registered = qtrue;

	itemInfo->models[0] = trap_R_RegisterModel(item->model[0]);
	if(item->model[1])
		itemInfo->models[1] = trap_R_RegisterModel(item->model[1]);

	itemInfo->icon = trap_R_RegisterShader(item->icon);

	if(item->type == IT_WEAPON)
		registerweap(item->tag);
}

/*
========================================================================================

VIEW WEAPON

========================================================================================
*/

/*
=================
maptorsotoweaponframe

=================
*/
static int
maptorsotoweaponframe(clientinfo_t *ci, int frame)
{
	// change weapon
	if(frame >= ci->animations[TORSO_DROP].firstframe
	   && frame < ci->animations[TORSO_DROP].firstframe + 9)
		return frame - ci->animations[TORSO_DROP].firstframe + 6;

	// stand attack
	if(frame >= ci->animations[TORSO_ATTACK].firstframe
	   && frame < ci->animations[TORSO_ATTACK].firstframe + 6)
		return 1 + frame - ci->animations[TORSO_ATTACK].firstframe;

	// stand attack 2
	if(frame >= ci->animations[TORSO_ATTACK2].firstframe
	   && frame < ci->animations[TORSO_ATTACK2].firstframe + 6)
		return 1 + frame - ci->animations[TORSO_ATTACK2].firstframe;

	return 0;
}

/*
==============
calculateweaponposition
==============
*/
static void
calculateweaponposition(vec3_t origin, vec3_t angles)
{
	float scale;
	int delta;
	float fracsin;

	veccopy(cg.refdef.vieworg, origin);
	veccopy(cg.refdefviewangles, angles);

	// on odd legs, invert some angles
	if(cg.bobcycle & 1)
		scale = -cg.xyspeed;
	else
		scale = cg.xyspeed;

	// gun angles from bobbing
	angles[ROLL] += scale * cg.bobfracsin * 0.005;
	angles[YAW] += scale * cg.bobfracsin * 0.01;
	angles[PITCH] += cg.xyspeed * cg.bobfracsin * 0.005;

	// drop the weapon when landing
	delta = cg.time - cg.landtime;
	if(delta < LAND_DEFLECT_TIME)
		origin[2] += cg.landchange*0.25 * delta / LAND_DEFLECT_TIME;
	else if(delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME)
		origin[2] += cg.landchange*0.25 *
			     (LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;

#if 0
	// drop the weapon when stair climbing
	delta = cg.time - cg.steptime;
	if(delta < STEP_TIME/2)
		origin[2] -= cg.stepchange*0.25 * delta / (STEP_TIME/2);
	else if(delta < STEP_TIME)
		origin[2] -= cg.stepchange*0.25 * (STEP_TIME - delta) / (STEP_TIME/2);

#endif

	// idle drift
	scale = cg.xyspeed + 40;
	fracsin = sin(cg.time * 0.001);
	angles[ROLL] += scale * fracsin * 0.01;
	angles[YAW] += scale * fracsin * 0.01;
	angles[PITCH] += scale * fracsin * 0.01;
}

/*
===============
lightningbolt

Origin will be the exact tag point, which is slightly
different than the muzzle point used for determining hits.
The cent should be the non-predicted cent if it is from the player,
so the endpoint will reflect the simulated strike (lagging the predicted
angle)
===============
*/
static void
lightningbolt(cent_t *cent, vec3_t origin)
{
	trace_t trace;
	refEntity_t beam;
	vec3_t forward;
	vec3_t muzzlePoint, endPoint;
	int anim;

	if(cent->currstate.weapon != WP_LIGHTNING)
		return;

	memset(&beam, 0, sizeof(beam));

	// CPMA  "true" lightning
	if((cent->currstate.number == cg.pps.clientNum) && (cg_trueLightning.value != 0)){
		vec3_t angle;
		int i;

		for(i = 0; i < 3; i++){
			float a = cent->lerpangles[i] - cg.refdefviewangles[i];
			if(a > 180)
				a -= 360;
			if(a < -180)
				a += 360;

			angle[i] = cg.refdefviewangles[i] + a * (1.0 - cg_trueLightning.value);
			if(angle[i] < 0)
				angle[i] += 360;
			if(angle[i] > 360)
				angle[i] -= 360;
		}

		anglevecs(angle, forward, nil, nil);
		veccopy(cent->lerporigin, muzzlePoint);
//		veccopy(cg.refdef.vieworg, muzzlePoint );
	}else{
		// !CPMA
		anglevecs(cent->lerpangles, forward, nil, nil);
		veccopy(cent->lerporigin, muzzlePoint);
	}

	anim = cent->currstate.legsAnim & ~ANIM_TOGGLEBIT;
	if(anim == LEGS_WALKCR || anim == LEGS_IDLECR)
		muzzlePoint[2] += CROUCH_VIEWHEIGHT;
	else
		muzzlePoint[2] += DEFAULT_VIEWHEIGHT;

	vecsadd(muzzlePoint, 14, forward, muzzlePoint);

	// project forward by the lightning range
	vecsadd(muzzlePoint, LIGHTNING_RANGE, forward, endPoint);

	// see if it hit a wall
	cgtrace(&trace, muzzlePoint, vec3_origin, vec3_origin, endPoint,
		 cent->currstate.number, MASK_SHOT);

	// this is the endpoint
	veccopy(trace.endpos, beam.oldorigin);

	// use the provided origin, even though it may be slightly
	// different than the muzzle origin
	veccopy(origin, beam.origin);

	beam.reType = RT_LIGHTNING;
	beam.customShader = cgs.media.lightningShader;
	trap_R_AddRefEntityToScene(&beam);

	// add the impact flare if it hit something
	if(trace.fraction < 1.0){
		vec3_t angles;
		vec3_t dir;

		vecsub(beam.oldorigin, beam.origin, dir);
		vecnorm(dir);

		memset(&beam, 0, sizeof(beam));
		beam.hModel = cgs.media.lightningExplosionModel;

		vecsadd(trace.endpos, -16, dir, beam.origin);

		// make a random orientation
		angles[0] = rand() % 360;
		angles[1] = rand() % 360;
		angles[2] = rand() % 360;
		AnglesToAxis(angles, beam.axis);
		trap_R_AddRefEntityToScene(&beam);
	}
}

/*

static void lightningbolt( cent_t *cent, vec3_t origin ){
        trace_t		trace;
        refEntity_t		beam;
        vec3_t			forward;
        vec3_t			muzzlePoint, endPoint;

        if( cent->currstate.weapon != WP_LIGHTNING ){
                return;
        }

        memset( &beam, 0, sizeof( beam ) );

        // find muzzle point for this frame
        veccopy( cent->lerporigin, muzzlePoint );
        anglevecs( cent->lerpangles, forward, nil, nil );

        // FIXME: crouch
        muzzlePoint[2] += DEFAULT_VIEWHEIGHT;

        vecsadd( muzzlePoint, 14, forward, muzzlePoint );

        // project forward by the lightning range
        vecsadd( muzzlePoint, LIGHTNING_RANGE, forward, endPoint );

        // see if it hit a wall
        cgtrace( &trace, muzzlePoint, vec3_origin, vec3_origin, endPoint,
                cent->currstate.number, MASK_SHOT );

        // this is the endpoint
        veccopy( trace.endpos, beam.oldorigin );

        // use the provided origin, even though it may be slightly
        // different than the muzzle origin
        veccopy( origin, beam.origin );

        beam.reType = RT_LIGHTNING;
        beam.customShader = cgs.media.lightningShader;
        trap_R_AddRefEntityToScene( &beam );

        // add the impact flare if it hit something
        if( trace.fraction < 1.0 ){
                vec3_t	angles;
                vec3_t	dir;

                vecsub( beam.oldorigin, beam.origin, dir );
                vecnorm( dir );

                memset( &beam, 0, sizeof( beam ) );
                beam.hModel = cgs.media.lightningExplosionModel;

                vecsadd( trace.endpos, -16, dir, beam.origin );

                // make a random orientation
                angles[0] = rand() % 360;
                angles[1] = rand() % 360;
                angles[2] = rand() % 360;
                AnglesToAxis( angles, beam.axis );
                trap_R_AddRefEntityToScene( &beam );
        }
}
*/

/*
======================
machinegunspinangle
======================
*/
#define         SPIN_SPEED	0.9
#define         COAST_TIME	1000
static float
machinegunspinangle(cent_t *cent)
{
	int delta;
	float angle;
	float speed;

	delta = cg.time - cent->pe.barreltime;
	if(cent->pe.barrelspin)
		angle = cent->pe.barrelangle + delta * SPIN_SPEED;
	else{
		if(delta > COAST_TIME)
			delta = COAST_TIME;

		speed = 0.5 * (SPIN_SPEED + (float)(COAST_TIME - delta) / COAST_TIME);
		angle = cent->pe.barrelangle + delta * speed;
	}

	if(cent->pe.barrelspin == !(cent->currstate.eFlags & EF_FIRING)){
		cent->pe.barreltime = cg.time;
		cent->pe.barrelangle = AngleMod(angle);
		cent->pe.barrelspin = !!(cent->currstate.eFlags & EF_FIRING);
	}

	return angle;
}

/*
========================
addweaponwithpowerups
========================
*/
static void
addweaponwithpowerups(refEntity_t *gun, int powerups)
{
	// add powerup effects
	if(powerups & (1 << PW_INVIS)){
		gun->customShader = cgs.media.invisShader;
		trap_R_AddRefEntityToScene(gun);
	}else{
		trap_R_AddRefEntityToScene(gun);

		if(powerups & (1 << PW_BATTLESUIT)){
			gun->customShader = cgs.media.battleWeaponShader;
			trap_R_AddRefEntityToScene(gun);
		}
		if(powerups & (1 << PW_QUAD)){
			gun->customShader = cgs.media.quadWeaponShader;
			trap_R_AddRefEntityToScene(gun);
		}
	}
}

/*
=============
addplayerweap

Used for both the view weapon (ps is valid) and the world modelother character models (ps is nil)
The main player will have this called for BOTH cases, so effects like light and
sound should only be done on the world model case.
=============
*/
void
addplayerweap(refEntity_t *parent, playerState_t *ps, cent_t *cent, int team)
{
	refEntity_t gun;
	refEntity_t barrel;
	refEntity_t flash;
	vec3_t angles;
	weap_t weaponNum;
	weapinfo_t *weapon;
	cent_t *nonPredictedCent;
	orientation_t lerped;

	weaponNum = cent->currstate.weapon;

	registerweap(weaponNum);
	weapon = &cg_weapons[weaponNum];

	// add the weapon
	memset(&gun, 0, sizeof(gun));
	veccopy(parent->lightingOrigin, gun.lightingOrigin);
	gun.shadowPlane = parent->shadowPlane;
	gun.renderfx = parent->renderfx;

	// set custom shading for railgun refire rate
	if(weaponNum == WP_RAILGUN){
		clientinfo_t *ci = &cgs.clientinfo[cent->currstate.clientNum];
		if(cent->pe.railfiretime + 1500 > cg.time){
			int scale = 255 * (cg.time - cent->pe.railfiretime) / 1500;
			gun.shaderRGBA[0] = (ci->c1RGBA[0] * scale) >> 8;
			gun.shaderRGBA[1] = (ci->c1RGBA[1] * scale) >> 8;
			gun.shaderRGBA[2] = (ci->c1RGBA[2] * scale) >> 8;
			gun.shaderRGBA[3] = 255;
		}else
			Byte4Copy(ci->c1RGBA, gun.shaderRGBA);
	}

	gun.hModel = weapon->weaponModel;
	if(!gun.hModel)
		return;

	if(!ps){
		// add weapon ready sound
		cent->pe.lightningfiring = qfalse;
		if((cent->currstate.eFlags & EF_FIRING) && weapon->firingsound){
			// lightning gun and guantlet make a different sound when fire is held down
			trap_S_AddLoopingSound(cent->currstate.number, cent->lerporigin, vec3_origin, weapon->firingsound);
			cent->pe.lightningfiring = qtrue;
		}else if(weapon->readysound)
			trap_S_AddLoopingSound(cent->currstate.number, cent->lerporigin, vec3_origin, weapon->readysound);
	}

	trap_R_LerpTag(&lerped, parent->hModel, parent->oldframe, parent->frame,
		       1.0 - parent->backlerp, "tag_weapon");
	veccopy(parent->origin, gun.origin);

	vecsadd(gun.origin, lerped.origin[0], parent->axis[0], gun.origin);

	// Make weapon appear left-handed for 2 and centered for 3
	if(ps && cg_drawGun.integer == 2)
		vecsadd(gun.origin, -lerped.origin[1], parent->axis[1], gun.origin);
	else if(!ps || cg_drawGun.integer != 3)
		vecsadd(gun.origin, lerped.origin[1], parent->axis[1], gun.origin);

	vecsadd(gun.origin, lerped.origin[2], parent->axis[2], gun.origin);

	MatrixMultiply(lerped.axis, ((refEntity_t*)parent)->axis, gun.axis);
	gun.backlerp = parent->backlerp;

	addweaponwithpowerups(&gun, cent->currstate.powerups);

	// add the spinning barrel
	if(weapon->barrelModel){
		memset(&barrel, 0, sizeof(barrel));
		veccopy(parent->lightingOrigin, barrel.lightingOrigin);
		barrel.shadowPlane = parent->shadowPlane;
		barrel.renderfx = parent->renderfx;

		barrel.hModel = weapon->barrelModel;
		angles[YAW] = 0;
		angles[PITCH] = 0;
		angles[ROLL] = machinegunspinangle(cent);
		AnglesToAxis(angles, barrel.axis);

		rotentontag(&barrel, &gun, weapon->weaponModel, "tag_barrel");

		addweaponwithpowerups(&barrel, cent->currstate.powerups);
	}

	// make sure we aren't looking at cg.pplayerent for LG
	nonPredictedCent = &cg_entities[cent->currstate.clientNum];

	// if the index of the nonPredictedCent is not the same as the clientNum
	// then this is a fake player (like on teh single player podiums), so
	// go ahead and use the cent
	if((nonPredictedCent - cg_entities) != cent->currstate.clientNum)
		nonPredictedCent = cent;

	// add the flash
	if((weaponNum == WP_LIGHTNING || weaponNum == WP_GAUNTLET || weaponNum == WP_GRAPPLING_HOOK)
	   && (nonPredictedCent->currstate.eFlags & EF_FIRING)){
		// continuous flash
	}else
	// impulse flash
	if(cg.time - cent->muzzleflashtime > MUZZLE_FLASH_TIME)
		return;


	memset(&flash, 0, sizeof(flash));
	veccopy(parent->lightingOrigin, flash.lightingOrigin);
	flash.shadowPlane = parent->shadowPlane;
	flash.renderfx = parent->renderfx;

	flash.hModel = weapon->flashModel;
	if(!flash.hModel)
		return;
	angles[YAW] = 0;
	angles[PITCH] = 0;
	angles[ROLL] = crandom() * 10;
	AnglesToAxis(angles, flash.axis);

	// colorize the railgun blast
	if(weaponNum == WP_RAILGUN){
		clientinfo_t *ci;

		ci = &cgs.clientinfo[cent->currstate.clientNum];
		flash.shaderRGBA[0] = 255 * ci->color1[0];
		flash.shaderRGBA[1] = 255 * ci->color1[1];
		flash.shaderRGBA[2] = 255 * ci->color1[2];
	}

	rotentontag(&flash, &gun, weapon->weaponModel, "tag_flash");
	trap_R_AddRefEntityToScene(&flash);

	if(ps || cg.thirdperson ||
	   cent->currstate.number != cg.pps.clientNum){
		// add lightning bolt
		lightningbolt(nonPredictedCent, flash.origin);

		if(weapon->flashDlightColor[0] || weapon->flashDlightColor[1] || weapon->flashDlightColor[2])
			trap_R_AddLightToScene(flash.origin, 300 + (rand()&31), weapon->flashDlightColor[0],
					       weapon->flashDlightColor[1], weapon->flashDlightColor[2]);
	}
}

/*
==============
addviewweap

Add the weapon, and flash for the player's view
==============
*/
void
addviewweap(playerState_t *ps)
{
	refEntity_t hand;
	cent_t *cent;
	clientinfo_t *ci;
	float fovOffset;
	vec3_t angles;
	weapinfo_t *weapon;

	if(ps->persistant[PERS_TEAM] == TEAM_SPECTATOR)
		return;

	if(ps->pm_type == PM_INTERMISSION)
		return;

	// no gun if in third person view or a camera is active
	//if( cg.thirdperson || cg.cameramode){
	if(cg.thirdperson)
		return;


	// allow the gun to be completely removed
	if(!cg_drawGun.integer){
		vec3_t origin;

		if(cg.pps.eFlags & EF_FIRING){
			// special hack for lightning gun...
			veccopy(cg.refdef.vieworg, origin);
			vecsadd(origin, -8, cg.refdef.viewaxis[2], origin);
			lightningbolt(&cg_entities[ps->clientNum], origin);
		}
		return;
	}

	// don't draw if testing a gun model
	if(cg.testgun)
		return;

	// drop gun lower at higher fov
	if(cg_fov.integer > 90)
		fovOffset = -0.2 * (cg_fov.integer - 90);
	else
		fovOffset = 0;

	cent = &cg.pplayerent;	// &cg_entities[cg.snap->ps.clientNum];
	registerweap(ps->weapon);
	weapon = &cg_weapons[ps->weapon];

	memset(&hand, 0, sizeof(hand));

	// set up gun position
	calculateweaponposition(hand.origin, angles);

	vecsadd(hand.origin, cg_gun_x.value, cg.refdef.viewaxis[0], hand.origin);
	vecsadd(hand.origin, cg_gun_y.value, cg.refdef.viewaxis[1], hand.origin);
	vecsadd(hand.origin, (cg_gun_z.value+fovOffset), cg.refdef.viewaxis[2], hand.origin);

	AnglesToAxis(angles, hand.axis);

	// map torso animations to weapon animations
	if(cg_gun_frame.integer){
		// development tool
		hand.frame = hand.oldframe = cg_gun_frame.integer;
		hand.backlerp = 0;
	}else{
		// get clientinfo for animation map
		ci = &cgs.clientinfo[cent->currstate.clientNum];
		hand.frame = maptorsotoweaponframe(ci, cent->pe.torso.frame);
		hand.oldframe = maptorsotoweaponframe(ci, cent->pe.torso.oldframe);
		hand.backlerp = cent->pe.torso.backlerp;
	}

	hand.hModel = weapon->handsModel;
	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON | RF_MINLIGHT;

	// add everything onto the hand
	addplayerweap(&hand, ps, &cg.pplayerent, ps->persistant[PERS_TEAM]);
}

/*
==============================================================================

WEAPON SELECTION

==============================================================================
*/

/*
===================
drawweapsel
===================
*/
void
drawweapsel(void)
{
	int i;
	int bits;
	int count;
	int x, y, w;
	char *name;
	float *color;

	// don't display if dead
	if(cg.pps.stats[STAT_HEALTH] <= 0)
		return;

	color = fadecolor(cg.weapseltime, WEAPON_SELECT_TIME);
	if(!color)
		return;
	trap_R_SetColor(color);

	// showing weapon select clears pickup item display, but not the blend blob
	cg.itemPickupTime = 0;

	// count the number of weapons owned
	bits = cg.snap->ps.stats[STAT_WEAPONS];
	count = 0;
	for(i = 1; i < MAX_WEAPONS; i++)
		if(bits & (1 << i))
			count++;

	x = 320 - count * 20;
	y = 380;

	for(i = 1; i < MAX_WEAPONS; i++){
		if(!(bits & (1 << i)))
			continue;

		registerweap(i);

		// draw weapon icon
		drawpic(x, y, 32, 32, cg_weapons[i].weaponIcon);

		// draw selection marker
		if(i == cg.weapsel)
			drawpic(x-4, y-4, 40, 40, cgs.media.selectShader);

		// no ammo cross on top
		if(!cg.snap->ps.ammo[i])
			drawpic(x, y, 32, 32, cgs.media.noammoShader);

		x += 40;
	}

	// draw the selected name
	if(cg_weapons[cg.weapsel].item){
		name = cg_weapons[cg.weapsel].item->pickupname;
		if(name){
			w = drawstrlen(name) * BIGCHAR_WIDTH;
			x = (SCREEN_WIDTH - w) / 2;
			drawbigstrcolor(x, y - 22, name, color);
		}
	}

	trap_R_SetColor(nil);
}

/*
===============
weapselectable
===============
*/
static qboolean
weapselectable(int i)
{
	if(!cg.snap->ps.ammo[i])
		return qfalse;
	if(!(cg.snap->ps.stats[STAT_WEAPONS] & (1 << i)))
		return qfalse;

	return qtrue;
}

/*
===============
nextweapon_f
===============
*/
void
nextweapon_f(void)
{
	int i;
	int original;

	if(!cg.snap)
		return;
	if(cg.snap->ps.pm_flags & PMF_FOLLOW)
		return;

	cg.weapseltime = cg.time;
	original = cg.weapsel;

	for(i = 0; i < MAX_WEAPONS; i++){
		cg.weapsel++;
		if(cg.weapsel == MAX_WEAPONS)
			cg.weapsel = 0;
		if(cg.weapsel == WP_GAUNTLET)
			continue;	// never cycle to gauntlet
		if(weapselectable(cg.weapsel))
			break;
	}
	if(i == MAX_WEAPONS)
		cg.weapsel = original;
}

/*
===============
prevweapon_f
===============
*/
void
prevweapon_f(void)
{
	int i;
	int original;

	if(!cg.snap)
		return;
	if(cg.snap->ps.pm_flags & PMF_FOLLOW)
		return;

	cg.weapseltime = cg.time;
	original = cg.weapsel;

	for(i = 0; i < MAX_WEAPONS; i++){
		cg.weapsel--;
		if(cg.weapsel == -1)
			cg.weapsel = MAX_WEAPONS - 1;
		if(cg.weapsel == WP_GAUNTLET)
			continue;	// never cycle to gauntlet
		if(weapselectable(cg.weapsel))
			break;
	}
	if(i == MAX_WEAPONS)
		cg.weapsel = original;
}

/*
===============
weapon_f
===============
*/
void
weapon_f(void)
{
	int num;

	if(!cg.snap)
		return;
	if(cg.snap->ps.pm_flags & PMF_FOLLOW)
		return;

	num = atoi(cgargv(1));

	if(num < 1 || num > MAX_WEAPONS-1)
		return;

	cg.weapseltime = cg.time;

	if(!(cg.snap->ps.stats[STAT_WEAPONS] & (1 << num)))
		return;	// don't have the weapon

	cg.weapsel = num;
}

/*
===================
outofammochange

The current weapon has just run out of ammo
===================
*/
void
outofammochange(void)
{
	int i;

	cg.weapseltime = cg.time;

	for(i = MAX_WEAPONS-1; i > 0; i--)
		if(weapselectable(i)){
			cg.weapsel = i;
			break;
		}
}

/*
===================================================================================================

WEAPON EVENTS

===================================================================================================
*/

/*
================
fireweap

Caused by an EV_FIRE_WEAPON event
================
*/
void
fireweap(cent_t *cent)
{
	entityState_t *ent;
	int c;
	weapinfo_t *weap;

	ent = &cent->currstate;
	if(ent->weapon == WP_NONE)
		return;
	if(ent->weapon >= WP_NUM_WEAPONS){
		cgerrorf("fireweap: ent->weapon >= WP_NUM_WEAPONS");
		return;
	}
	weap = &cg_weapons[ent->weapon];

	// mark the entity as muzzle flashing, so when it is added it will
	// append the flash to the weapon model
	cent->muzzleflashtime = cg.time;

	// lightning gun only does this this on initial press
	if(ent->weapon == WP_LIGHTNING)
		if(cent->pe.lightningfiring)
			return;

	if(ent->weapon == WP_RAILGUN)
		cent->pe.railfiretime = cg.time;

	// play quad sound if needed
	if(cent->currstate.powerups & (1 << PW_QUAD))
		trap_S_StartSound(nil, cent->currstate.number, CHAN_ITEM, cgs.media.quadSound);

	// play a sound
	for(c = 0; c < 4; c++)
		if(!weap->flashSound[c])
			break;
	if(c > 0){
		c = rand() % c;
		if(weap->flashSound[c])
			trap_S_StartSound(nil, ent->number, CHAN_WEAPON, weap->flashSound[c]);
	}

	// do brass ejection
	if(weap->ejectBrassFunc && cg_brassTime.integer > 0)
		weap->ejectBrassFunc(cent);
}

/*
=================
missilehitwall

Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing
=================
*/
void
missilehitwall(int weapon, int clientNum, vec3_t origin, vec3_t dir, impactsound_t soundType)
{
	qhandle_t mod;
	qhandle_t mark;
	qhandle_t shader;
	sfxHandle_t sfx;
	float radius;
	float light;
	vec3_t lightcolor;
	localent_t *le;
	int r;
	qboolean alphafade;
	qboolean isSprite;
	int duration;
	vec3_t sprOrg;
	vec3_t sprVel;

	mod = 0;
	shader = 0;
	light = 0;
	lightcolor[0] = 1;
	lightcolor[1] = 1;
	lightcolor[2] = 0;

	// set defaults
	isSprite = qfalse;
	duration = 600;

	switch(weapon){
	default:
	case WP_LIGHTNING:
		// no explosion at LG impact, it is added with the beam
		r = rand() & 3;
		if(r < 2)
			sfx = cgs.media.sfx_lghit2;
		else if(r == 2)
			sfx = cgs.media.sfx_lghit1;
		else
			sfx = cgs.media.sfx_lghit3;
		mark = cgs.media.holeMarkShader;
		radius = 12;
		break;
	case WP_GRENADE_LAUNCHER:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.grenadeExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		break;
	case WP_ROCKET_LAUNCHER:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.rocketExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		duration = 1000;
		lightcolor[0] = 1;
		lightcolor[1] = 0.75;
		lightcolor[2] = 0.0;
		break;
	case WP_RAILGUN:
		mod = cgs.media.ringFlashModel;
		shader = cgs.media.railExplosionShader;
		//sfx = cgs.media.sfx_railg;
		sfx = cgs.media.sfx_plasmaexp;
		mark = cgs.media.energyMarkShader;
		radius = 24;
		break;
	case WP_PLASMAGUN:
		mod = cgs.media.ringFlashModel;
		shader = cgs.media.plasmaExplosionShader;
		sfx = cgs.media.sfx_plasmaexp;
		mark = cgs.media.energyMarkShader;
		radius = 16;
		break;
	case WP_BFG:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.bfgExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 32;
		isSprite = qtrue;
		break;
	case WP_SHOTGUN:
		mod = cgs.media.bulletFlashModel;
		shader = cgs.media.bulletExplosionShader;
		mark = cgs.media.bulletMarkShader;
		sfx = 0;
		radius = 4;
		break;


	case WP_MACHINEGUN:
		mod = cgs.media.bulletFlashModel;
		shader = cgs.media.bulletExplosionShader;
		mark = cgs.media.bulletMarkShader;

		r = rand() & 3;
		if(r == 0)
			sfx = cgs.media.sfx_ric1;
		else if(r == 1)
			sfx = cgs.media.sfx_ric2;
		else
			sfx = cgs.media.sfx_ric3;

		radius = 8;
		break;
	}

	if(sfx)
		trap_S_StartSound(origin, ENTITYNUM_WORLD, CHAN_AUTO, sfx);

	// create the explosion
	if(mod){
		le = explosion(origin, dir,
				      mod, shader,
				      duration, isSprite);
		le->light = light;
		veccopy(lightcolor, le->lightcolor);
		if(weapon == WP_RAILGUN){
			// colorize with client color
			veccopy(cgs.clientinfo[clientNum].color1, le->color);
			le->refEntity.shaderRGBA[0] = le->color[0] * 0xff;
			le->refEntity.shaderRGBA[1] = le->color[1] * 0xff;
			le->refEntity.shaderRGBA[2] = le->color[2] * 0xff;
			le->refEntity.shaderRGBA[3] = 0xff;
		}
	}

	// impact mark
	alphafade = (mark == cgs.media.energyMarkShader);	// plasma fades alpha, all others fade color
	if(weapon == WP_RAILGUN){
		float *color;

		// colorize with client color
		color = cgs.clientinfo[clientNum].color1;
		impactmark(mark, origin, dir, random()*360, color[0], color[1], color[2], 1, alphafade, radius, qfalse);
	}else
		impactmark(mark, origin, dir, random()*360, 1, 1, 1, 1, alphafade, radius, qfalse);
}

/*
=================
missilehitplayer
=================
*/
void
missilehitplayer(int weapon, vec3_t origin, vec3_t dir, int entityNum)
{
	bleed(origin, entityNum);

	// some weapons will make an explosion with the blood, while
	// others will just make the blood
	switch(weapon){
	case WP_GRENADE_LAUNCHER:
	case WP_ROCKET_LAUNCHER:
	case WP_PLASMAGUN:
	case WP_BFG:
		missilehitwall(weapon, 0, origin, dir, IMPACTSOUND_FLESH);
		break;
	default:
		break;
	}
}

/*
============================================================================

SHOTGUN TRACING

============================================================================
*/

/*
================
shotgunpellet
================
*/
static void
shotgunpellet(vec3_t start, vec3_t end, int skipNum)
{
	trace_t tr;
	int sourceContentType, destContentType;

	cgtrace(&tr, start, nil, nil, end, skipNum, MASK_SHOT);

	sourceContentType = pointcontents(start, 0);
	destContentType = pointcontents(tr.endpos, 0);

	// FIXME: should probably move this cruft into bubbletrail
	if(sourceContentType == destContentType){
		if(sourceContentType & CONTENTS_WATER)
			bubbletrail(start, tr.endpos, 32);
	}else if(sourceContentType & CONTENTS_WATER){
		trace_t trace;

		trap_CM_BoxTrace(&trace, end, start, nil, nil, 0, CONTENTS_WATER);
		bubbletrail(start, trace.endpos, 32);
	}else if(destContentType & CONTENTS_WATER){
		trace_t trace;

		trap_CM_BoxTrace(&trace, start, end, nil, nil, 0, CONTENTS_WATER);
		bubbletrail(tr.endpos, trace.endpos, 32);
	}

	if(tr.surfaceFlags & SURF_NOIMPACT)
		return;

	if(cg_entities[tr.entityNum].currstate.eType == ET_PLAYER)
		missilehitplayer(WP_SHOTGUN, tr.endpos, tr.plane.normal, tr.entityNum);
	else{
		if(tr.surfaceFlags & SURF_NOIMPACT)
			// SURF_NOIMPACT will not make a flame puff or a mark
			return;
		if(tr.surfaceFlags & SURF_METALSTEPS)
			missilehitwall(WP_SHOTGUN, 0, tr.endpos, tr.plane.normal, IMPACTSOUND_METAL);
		else
			missilehitwall(WP_SHOTGUN, 0, tr.endpos, tr.plane.normal, IMPACTSOUND_DEFAULT);
	}
}

/*
================
shotgunpattern

Perform the same traces the server did to locate the
hit splashes
================
*/
static void
shotgunpattern(vec3_t origin, vec3_t origin2, int seed, int otherEntNum)
{
	int i;
	float r, u;
	vec3_t end;
	vec3_t forward, right, up;

	// derive the right and up vectors from the forward vector, because
	// the client won't have any other information
	VectorNormalize2(origin2, forward);
	vecperp(right, forward);
	veccross(forward, right, up);

	// generate the "random" spread pattern
	for(i = 0; i < DEFAULT_SHOTGUN_COUNT; i++){
		r = Q_crandom(&seed) * DEFAULT_SHOTGUN_SPREAD * 16;
		u = Q_crandom(&seed) * DEFAULT_SHOTGUN_SPREAD * 16;
		vecsadd(origin, 8192 * 16, forward, end);
		vecsadd(end, r, right, end);
		vecsadd(end, u, up, end);

		shotgunpellet(origin, end, otherEntNum);
	}
}

/*
==============
shotgunfire
==============
*/
void
shotgunfire(entityState_t *es)
{
	vec3_t v;
	int contents;

	vecsub(es->origin2, es->pos.trBase, v);
	vecnorm(v);
	vecscale(v, 32, v);
	vecadd(es->pos.trBase, v, v);
	if(cgs.glconfig.hardwareType != GLHW_RAGEPRO){
		// ragepro can't alpha fade, so don't even bother with smoke
		vec3_t up;

		contents = pointcontents(es->pos.trBase, 0);
		if(!(contents & CONTENTS_WATER)){
			vecset(up, 0, 0, 8);
			smokepuff(v, up, 32, 1, 1, 1, 0.33f, 900, cg.time, 0, LEF_PUFF_DONT_SCALE, cgs.media.shotgunSmokePuffShader);
		}
	}
	shotgunpattern(es->pos.trBase, es->origin2, es->eventParm, es->otherEntityNum);
}

/*
============================================================================

BULLETS

============================================================================
*/

/*
===============
tracer
===============
*/
void
tracer(vec3_t source, vec3_t dest)
{
	vec3_t forward, right;
	polyVert_t verts[4];
	vec3_t line;
	float len, begin, end;
	vec3_t start, finish;
	vec3_t midpoint;

	// tracer
	vecsub(dest, source, forward);
	len = vecnorm(forward);

	// start at least a little ways from the muzzle
	if(len < 100)
		return;
	begin = 50 + random() * (len - 60);
	end = begin + cg_tracerLength.value;
	if(end > len)
		end = len;
	vecsadd(source, begin, forward, start);
	vecsadd(source, end, forward, finish);

	line[0] = vecdot(forward, cg.refdef.viewaxis[1]);
	line[1] = vecdot(forward, cg.refdef.viewaxis[2]);

	vecscale(cg.refdef.viewaxis[1], line[1], right);
	vecsadd(right, -line[0], cg.refdef.viewaxis[2], right);
	vecnorm(right);

	vecsadd(finish, cg_tracerWidth.value, right, verts[0].xyz);
	verts[0].st[0] = 0;
	verts[0].st[1] = 1;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	vecsadd(finish, -cg_tracerWidth.value, right, verts[1].xyz);
	verts[1].st[0] = 1;
	verts[1].st[1] = 0;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	vecsadd(start, -cg_tracerWidth.value, right, verts[2].xyz);
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	vecsadd(start, cg_tracerWidth.value, right, verts[3].xyz);
	verts[3].st[0] = 0;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene(cgs.media.tracerShader, 4, verts);

	midpoint[0] = (start[0] + finish[0]) * 0.5;
	midpoint[1] = (start[1] + finish[1]) * 0.5;
	midpoint[2] = (start[2] + finish[2]) * 0.5;

	// add the tracer sound
	trap_S_StartSound(midpoint, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.tracerSound);
}

/*
======================
calcmuzzlepoint
======================
*/
static qboolean
calcmuzzlepoint(int entityNum, vec3_t muzzle)
{
	vec3_t forward;
	cent_t *cent;
	int anim;

	if(entityNum == cg.snap->ps.clientNum){
		veccopy(cg.snap->ps.origin, muzzle);
		muzzle[2] += cg.snap->ps.viewheight;
		anglevecs(cg.snap->ps.viewangles, forward, nil, nil);
		vecsadd(muzzle, 14, forward, muzzle);
		return qtrue;
	}

	cent = &cg_entities[entityNum];
	if(!cent->currvalid)
		return qfalse;

	veccopy(cent->currstate.pos.trBase, muzzle);

	anglevecs(cent->currstate.apos.trBase, forward, nil, nil);
	anim = cent->currstate.legsAnim & ~ANIM_TOGGLEBIT;
	if(anim == LEGS_WALKCR || anim == LEGS_IDLECR)
		muzzle[2] += CROUCH_VIEWHEIGHT;
	else
		muzzle[2] += DEFAULT_VIEWHEIGHT;

	vecsadd(muzzle, 14, forward, muzzle);

	return qtrue;
}

/*
======================
dobullet

Renders bullet effects.
======================
*/
void
dobullet(vec3_t end, int sourceEntityNum, vec3_t normal, qboolean flesh, int fleshEntityNum)
{
	trace_t trace;
	int sourceContentType, destContentType;
	vec3_t start;

	// if the shooter is currently valid, calc a source point and possibly
	// do trail effects
	if(sourceEntityNum >= 0 && cg_tracerChance.value > 0)
		if(calcmuzzlepoint(sourceEntityNum, start)){
			sourceContentType = pointcontents(start, 0);
			destContentType = pointcontents(end, 0);

			// do a complete bubble trail if necessary
			if((sourceContentType == destContentType) && (sourceContentType & CONTENTS_WATER))
				bubbletrail(start, end, 32);
			// bubble trail from water into air
			else if((sourceContentType & CONTENTS_WATER)){
				trap_CM_BoxTrace(&trace, end, start, nil, nil, 0, CONTENTS_WATER);
				bubbletrail(start, trace.endpos, 32);
			}
			// bubble trail from air into water
			else if((destContentType & CONTENTS_WATER)){
				trap_CM_BoxTrace(&trace, start, end, nil, nil, 0, CONTENTS_WATER);
				bubbletrail(trace.endpos, end, 32);
			}

			// draw a tracer
			if(random() < cg_tracerChance.value)
				tracer(start, end);
		}

	// impact splash and mark
	if(flesh)
		bleed(end, fleshEntityNum);
	else
		missilehitwall(WP_MACHINEGUN, 0, end, normal, IMPACTSOUND_DEFAULT);

}
