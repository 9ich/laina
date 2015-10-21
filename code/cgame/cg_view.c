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
// Setup all the parameters (position, angle, etc) for a 3D rendering.

#include "cg_local.h"

#define WAVE_AMPLITUDE	1
#define WAVE_FREQUENCY	0.4
#define FOCUS_DISTANCE	150

static void addtestmodel(void);

/*
Sets the coordinates of the rendered window.
*/
static void
calcvrect(void)
{
	int size;

	// the intermission should allways be full screen
	if(cg.snap->ps.pm_type == PM_INTERMISSION)
		size = 100;
	else{
		// bound normal viewsize
		if(cg_viewsize.integer < 30){
			trap_Cvar_Set("cg_viewsize", "30");
			size = 30;
		}else if(cg_viewsize.integer > 100){
			trap_Cvar_Set("cg_viewsize", "100");
			size = 100;
		}else
			size = cg_viewsize.integer;

	}
	cg.refdef.width = cgs.glconfig.vidWidth*size/100;
	cg.refdef.width &= ~1;

	cg.refdef.height = cgs.glconfig.vidHeight*size/100;
	cg.refdef.height &= ~1;

	cg.refdef.x = (cgs.glconfig.vidWidth - cg.refdef.width)/2;
	cg.refdef.y = (cgs.glconfig.vidHeight - cg.refdef.height)/2;
}

/*
Smooths stair climbing.
*/
static void
stepoffset(void)
{
	int timeDelta;

	timeDelta = cg.time - cg.steptime;
	if(timeDelta < STEP_TIME)
		cg.refdef.vieworg[2] -= cg.stepchange *
		   (STEP_TIME - timeDelta) / STEP_TIME;
}

/*
Offsets the third-person view.
*/
static void
calc3rdperson(void)
{
	playerState_t *pps;
	vec3_t forward, right, up, view, focusAngles, focusPoint;
	trace_t trace;
	static vec3_t mins = {-4, -4, -4};
	static vec3_t maxs = {4, 4, 4};
	float focusDist, forwardScale, sideScale;

	pps = &cg.pps;
	cg.refdef.vieworg[2] += pps->viewheight;

	veccpy(cg.refdefviewangles, focusAngles);

	focusAngles[PITCH] = MIN(89.999f, focusAngles[PITCH]);
	anglevecs(focusAngles, forward, nil, nil);

	vecmad(cg.refdef.vieworg, FOCUS_DISTANCE, forward, focusPoint);
	veccpy(cg.refdef.vieworg, view);
	view[2] += 8;
	anglevecs(cg.refdefviewangles, forward, right, up);

	if(cg_thirdPerson.integer == 1){
		forwardScale = cos(cg_thirdPersonAngle.value / 180 * M_PI);
		sideScale = sin(cg_thirdPersonAngle.value / 180 * M_PI);
	}else{
		forwardScale = 1.0f;
		sideScale = 0.0f;
	}
	vecmad(view, -cg_thirdPersonRange.value * forwardScale, forward, view);
	vecmad(view, -cg_thirdPersonRange.value * sideScale, right, view);

	// trace a ray from the origin to the viewpoint to make sure
	// the view isn't in a solid block.  Use an 8 by 8 block to
	// prevent the view from near clipping anything
	if(!cg_cameraMode.integer){
		cgtrace(&trace, cg.refdef.vieworg, mins, maxs, view,
		   pps->clientNum, MASK_SOLID);
		if(trace.fraction != 1.0f){
			veccpy(trace.endpos, view);
			view[2] += (1.0f - trace.fraction) * 32;
			// try another trace to this position, because
			// a tunnel may have the ceiling close enough
			// that this is poking out
			cgtrace(&trace, cg.refdef.vieworg, mins, maxs, view,
			   pps->clientNum, MASK_SOLID);
			veccpy(trace.endpos, view);
		}
	}

	veccpy(view, cg.refdef.vieworg);

	// select pitch to look at focus point from vieword
	vecsub(focusPoint, cg.refdef.vieworg, focusPoint);
	focusDist = sqrt(focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1]);
	if(focusDist < 1)
		focusDist = 1;	// should never happen
	cg.refdefviewangles[PITCH] = -180 / M_PI * atan2(focusPoint[2], focusDist);
	if(cg_thirdPerson.integer == 1){
		cg.refdefviewangles[PITCH] = -180 / M_PI*atan2(focusPoint[2], focusDist);
		cg.refdefviewangles[YAW] -= cg_thirdPersonAngle.value;
	}
}

/*
Offsets the first-person view for view bob, etc.
*/
static void
calc1stperson(void)
{
	float *origin, *angles;
	float bob, ratio, delta, speed, f;
	vec3_t predictedvel;
	int timeDelta;

	if(cg.snap->ps.pm_type == PM_INTERMISSION)
		return;

	origin = cg.refdef.vieworg;
	angles = cg.refdefviewangles;

	// if dead, fix the angle and don't add any kick
	if(cg.snap->ps.stats[STAT_HEALTH] <= 0){
		origin[2] += cg.pps.viewheight;
	}

	// add angles based on damage kick
	if(cg.dmgtime){
		ratio = cg.time - cg.dmgtime;
		if(ratio < DAMAGE_DEFLECT_TIME){
			ratio /= DAMAGE_DEFLECT_TIME;
			angles[PITCH] += ratio * cg.vdmgpitch;
			angles[ROLL] += ratio * cg.vdmgroll;
		}else{
			ratio = 1.0 - (ratio - DAMAGE_DEFLECT_TIME) / DAMAGE_RETURN_TIME;
			if(ratio > 0){
				angles[PITCH] += ratio * cg.vdmgpitch;
				angles[ROLL] += ratio * cg.vdmgroll;
			}
		}
	}

	// add angles based on velocity
	veccpy(cg.pps.velocity, predictedvel);

	delta = vecdot(predictedvel, cg.refdef.viewaxis[0]);
	angles[PITCH] += delta * cg_runpitch.value;

	delta = vecdot(predictedvel, cg.refdef.viewaxis[1]);
	angles[ROLL] -= delta * cg_runroll.value;

	// add angles based on bob

	// make sure the bob is visible even at low speeds
	speed = cg.xyspeed > 200 ? cg.xyspeed : 200;

	delta = cg.bobfracsin * cg_bobpitch.value * speed;
	if(cg.pps.pm_flags & PMF_DUCKED)
		delta *= 3;	// crouching
	angles[PITCH] += delta;
	delta = cg.bobfracsin * cg_bobroll.value * speed;
	if(cg.pps.pm_flags & PMF_DUCKED)
		delta *= 3;	// crouching accentuates roll
	if(cg.bobcycle & 1)
		delta = -delta;
	angles[ROLL] += delta;

	// add view height
	origin[2] += cg.pps.viewheight;

	// smooth out duck height changes
	timeDelta = cg.time - cg.ducktime;
	if(timeDelta < DUCK_TIME)
		cg.refdef.vieworg[2] -= cg.duckchange
					* (DUCK_TIME - timeDelta) / DUCK_TIME;

	// add bob height
	bob = cg.bobfracsin * cg.xyspeed * cg_bobup.value;
	if(bob > 6)
		bob = 6;

	origin[2] += bob;

	// add fall height
	delta = cg.time - cg.landtime;
	if(delta < LAND_DEFLECT_TIME){
		f = delta / LAND_DEFLECT_TIME;
		cg.refdef.vieworg[2] += cg.landchange * f;
	}else if(delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME){
		delta -= LAND_DEFLECT_TIME;
		f = 1.0 - (delta / LAND_RETURN_TIME);
		cg.refdef.vieworg[2] += cg.landchange * f;
	}

	stepoffset();
}

/*
Fixed fov at intermissions, otherwise account for fov variable and zooms.
*/
static int
calcfov(void)
{
	float x, phase, v;
	int contents;
	float fov_x, fov_y, zoomfov;
	float f;
	int inwater;

	if(cg.pps.pm_type == PM_INTERMISSION)
		// if in intermission, use a fixed value
		fov_x = 90;
	else{
		// user selectable
		fov_x = MAX(1, MIN(170, cg_fov.value));
		zoomfov = MAX(1, MIN(170, cg_zoomFov.value));

		if(cg.zoomed){
			f = (cg.time - cg.zoomtime) / (float)ZOOM_TIME;
			if(f > 1.0)
				fov_x = zoomfov;
			else
				fov_x = fov_x + f * (zoomfov - fov_x);
		}else{
			f = (cg.time - cg.zoomtime) / (float)ZOOM_TIME;
			if(f <= 1.0)
				fov_x = zoomfov + f * (fov_x - zoomfov);
		}
	}

	x = cg.refdef.width / tan(fov_x / 360 * M_PI);
	fov_y = atan2(cg.refdef.height, x);
	fov_y = fov_y * 360 / M_PI;

	// warp if underwater
	contents = pointcontents(cg.refdef.vieworg, -1);
	if(contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)){
		phase = cg.time / 1000.0 * WAVE_FREQUENCY * M_PI * 2;
		v = WAVE_AMPLITUDE * sin(phase);
		fov_x += v;
		fov_y -= v;
		inwater = qtrue;
	}else
		inwater = qfalse;


	// set it
	cg.refdef.fov_x = fov_x;
	cg.refdef.fov_y = fov_y;

	if(!cg.zoomed)
		cg.zoomsens = 1;
	else
		cg.zoomsens = cg.refdef.fov_y / 75.0;

	return inwater;
}

static void
damageblendblob(void)
{
	int t;
	int maxTime;
	refEntity_t ent;

	if(!cg_blood.integer)
		return;

	if(!cg.dmgval)
		return;

	//if(cg.cameramode){
	//	return;
	//}

	// ragePro systems can't fade blends, so don't obscure the screen
	if(cgs.glconfig.hardwareType == GLHW_RAGEPRO)
		return;

	maxTime = DAMAGE_TIME;
	t = cg.time - cg.dmgtime;
	if(t <= 0 || t >= maxTime)
		return;


	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_FIRST_PERSON;

	vecmad(cg.refdef.vieworg, 8, cg.refdef.viewaxis[0], ent.origin);
	vecmad(ent.origin, cg.dmgx * -8, cg.refdef.viewaxis[1], ent.origin);
	vecmad(ent.origin, cg.dmgy * 8, cg.refdef.viewaxis[2], ent.origin);

	ent.radius = cg.dmgval * 3;
	ent.customShader = cgs.media.viewBloodShader;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 200 * (1.0 - ((float)t / maxTime));
	trap_R_AddRefEntityToScene(&ent);
}

/*
Sets cg.refdef view values.
*/
static int
calcviewvalues(void)
{
	playerState_t *ps;

	memset(&cg.refdef, 0, sizeof(cg.refdef));

	// calculate size of 3D view
	calcvrect();

	ps = &cg.pps;
	/*
	    if(cg.cameramode){
	            vec3_t origin, angles;
	            if(trap_getCameraInfo(cg.time, &origin, &angles)){
	                    veccpy(origin, cg.refdef.vieworg);
	                    angles[ROLL] = 0;
	                    veccpy(angles, cg.refdefviewangles);
	                    AnglesToAxis( cg.refdefviewangles, cg.refdef.viewaxis );
	                    return calcfov();
	            }else{
	                    cg.cameramode = qfalse;
	            }
	    }
	*/
	// intermission view
	if(ps->pm_type == PM_INTERMISSION){
		veccpy(ps->origin, cg.refdef.vieworg);
		veccpy(ps->viewangles, cg.refdefviewangles);
		AnglesToAxis(cg.refdefviewangles, cg.refdef.viewaxis);
		return calcfov();
	}

	cg.bobcycle = (ps->bobCycle & 128) >> 7;
	cg.bobfracsin = fabs(sin((ps->bobCycle & 127) / 127.0 * M_PI));
	cg.xyspeed = sqrt(ps->velocity[0] * ps->velocity[0] +
			  ps->velocity[1] * ps->velocity[1]);

	veccpy(ps->origin, cg.refdef.vieworg);
	if(cg_thirdPerson.integer != 3)
		veccpy(ps->viewangles, cg.refdefviewangles);
	if(cg_thirdPerson.integer == 2)
		vecset(ps->viewangles, 0, 0, 0);

	if(cg_cameraOrbit.value != 0.0f)
		if(cg.time > cg.nextorbittime){
			cg.nextorbittime = cg.time + cg_cameraOrbitDelay.integer;
			cg_thirdPersonAngle.value += cg_cameraOrbit.value;
		}
	// add error decay
	if(cg_errorDecay.value > 0){
		int t;
		float f;

		t = cg.time - cg.predictederrtime;
		f = (cg_errorDecay.value - t) / cg_errorDecay.value;
		if(f > 0 && f < 1)
			vecmad(cg.refdef.vieworg, f, cg.predictederr, cg.refdef.vieworg);
		else
			cg.predictederrtime = 0;
	}

	if(cg.thirdperson)
		calc3rdperson();
	else
		calc1stperson();

	// position eye relative to origin
	AnglesToAxis(cg.refdefviewangles, cg.refdef.viewaxis);

	if(cg.hyperspace)
		cg.refdef.rdflags |= RDF_NOWORLDMODEL | RDF_HYPERSPACE;

	return calcfov();
}

static void
poweruptimersounds(void)
{
	int i, t;

	// powerup timers going away
	for(i = 0; i < MAX_POWERUPS; i++){
		t = cg.snap->ps.powerups[i];
		if(t <= cg.time)
			continue;
		if(t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME)
			continue;
		if((t - cg.time) / POWERUP_BLINK_TIME != (t - cg.oldtime) / POWERUP_BLINK_TIME)
			trap_S_StartSound(nil, cg.snap->ps.clientNum, CHAN_ITEM, cgs.media.wearOffSound);
	}
}

void
addbufferedsound(sfxHandle_t sfx)
{
	if(!sfx)
		return;
	cg.sndbuf[cg.sndbufin] = sfx;
	cg.sndbufin = (cg.sndbufin + 1) % MAX_SOUNDBUFFER;
	if(cg.sndbufin == cg.sndbufout)
		cg.sndbufout++;
}

static void
playbufferedsounds(void)
{
	if(cg.sndtime < cg.time)
		if(cg.sndbufout != cg.sndbufin && cg.sndbuf[cg.sndbufout]){
			trap_S_StartLocalSound(cg.sndbuf[cg.sndbufout], CHAN_ANNOUNCER);
			cg.sndbuf[cg.sndbufout] = 0;
			cg.sndbufout = (cg.sndbufout + 1) % MAX_SOUNDBUFFER;
			cg.sndtime = cg.time + 750;
		}
}

/*
Generates and draws a game scene and status information at the given time.
*/
void
drawframe(int serverTime, stereoFrame_t stereoview, qboolean demoplayback)
{
	int inwater;

	cg.time = serverTime;
	cg.demoplayback = demoplayback;

	// update cvars
	updatecvars();

	// if we are only updating the screen as a loading
	// pacifier, don't even try to read snapshots
	if(cg.infoscreentext[0] != 0){
		drawinfo();
		return;
	}

	// any looped sounds will be respecified as entities
	// are added to the render list
	trap_S_ClearLoopingSounds(qfalse);

	// clear all the render lists
	trap_R_ClearScene();

	// set up cg.snap and possibly cg.nextsnap
	processsnaps();

	// if we haven't received any snapshots yet, all
	// we can draw is the information screen
	if(!cg.snap || (cg.snap->snapFlags & SNAPFLAG_NOT_ACTIVE)){
		drawinfo();
		return;
	}

	// let the client system know what our weapon and zoom settings are
	trap_SetUserCmdValue(cg.weapsel, cg.zoomsens);

	// this counter will be bumped for every valid scene we generate
	cg.clframe++;

	// update cg.pps
	predictplayerstate();

	// decide on third person view
	cg.thirdperson = cg_thirdPerson.integer || (cg.snap->ps.stats[STAT_HEALTH] <= 0);

	// build cg.refdef
	inwater = calcviewvalues();

	// first person blend blobs, done after AnglesToAxis
	if(!cg.thirdperson)
		damageblendblob();

	// build the render lists
	if(!cg.hyperspace){
		addpacketents();	// adter calcViewValues, so predicted player state is correct
		addmarks();
		addlocalents();
	}
	addviewweap(&cg.pps);

	// add buffered sounds
	playbufferedsounds();


	// finish up the rest of the refdef
	if(cg.testmodelent.hModel)
		addtestmodel();
	cg.refdef.time = cg.time;
	memcpy(cg.refdef.areamask, cg.snap->areamask, sizeof(cg.refdef.areamask));

	// warning sounds when powerup is wearing off
	poweruptimersounds();

	// update audio positions
	trap_S_Respatialize(cg.snap->ps.clientNum, cg.refdef.vieworg, cg.refdef.viewaxis, inwater);

	// make sure the lagometerSample and frame timing isn't done twice when in stereo
	if(stereoview != STEREO_RIGHT){
		cg.frametime = cg.time - cg.oldtime;
		if(cg.frametime < 0)
			cg.frametime = 0;
		cg.oldtime = cg.time;
		lagometerframeinfo();
	}
	if(cg_timescale.value != cg_timescaleFadeEnd.value){
		if(cg_timescale.value < cg_timescaleFadeEnd.value){
			cg_timescale.value += cg_timescaleFadeSpeed.value * ((float)cg.frametime) / 1000;
			if(cg_timescale.value > cg_timescaleFadeEnd.value)
				cg_timescale.value = cg_timescaleFadeEnd.value;
		}else{
			cg_timescale.value -= cg_timescaleFadeSpeed.value * ((float)cg.frametime) / 1000;
			if(cg_timescale.value < cg_timescaleFadeEnd.value)
				cg_timescale.value = cg_timescaleFadeEnd.value;
		}
		if(cg_timescaleFadeSpeed.value)
			trap_Cvar_Set("timescale", va("%f", cg_timescale.value));
	}

	// actually issue the rendering calls
	drawactive(stereoview);

	if(cg_stats.integer)
		cgprintf("cg.clframe:%i\n", cg.clframe);

	if(cg_debugPosition.integer)
		cgprintf("yaw=%06.2f pitch=%06.2f roll=%06.2f\n", cg.pps.viewangles[YAW],
		   cg.pps.viewangles[PITCH], cg.pps.viewangles[ROLL]);

}

void
zoomdown_f(void)
{
	if(cg.zoomed)
		return;
	cg.zoomed = qtrue;
	cg.zoomtime = cg.time;
}

void
zoomup_f(void)
{
	if(!cg.zoomed)
		return;
	cg.zoomed = qfalse;
	cg.zoomtime = cg.time;
}

/*
Model viewing can begin with either "testmodel <modelname>" or "testgun
<modelname>".

The names must be the full pathname after the basedir, like
"models/weapons/v_launch/tris.md3" or "players/male/tris.md3"

Testmodel will create a fake entity 100 units in front of the current
view position, directly facing the viewer.  It will remain immobile,
so you can move around it to view it from different angles.

Testgun will cause the model to follow the player around and supress the
real view weapon model.  The default frame 0 of most guns is completely
off screen, so you will probably have to cycle a couple frames to see it.

"nextframe", "prevframe", "nextskin", and "prevskin" commands will
change the frame or skin of the testmodel.  These are bound to F5, F6,
F7, and F8 in default.cfg.

If a gun is being tested, the "gun_x", "gun_y", and "gun_z" variables
will let you adjust the positioning.

Note that none of the model testing features update while the game is
paused, so it may be convenient to test with deathmatch set to 1 so that
bringing down the console doesn't pause the game.
*/

/*
Creates an entity in front of the current position, which
can then be moved around
*/
void
testmodel_f(void)
{
	vec3_t angles;

	memset(&cg.testmodelent, 0, sizeof(cg.testmodelent));
	if(trap_Argc() < 2)
		return;

	Q_strncpyz(cg.testmodelname, cgargv(1), MAX_QPATH);
	cg.testmodelent.hModel = trap_R_RegisterModel(cg.testmodelname);

	if(trap_Argc() == 3){
		cg.testmodelent.backlerp = atof(cgargv(2));
		cg.testmodelent.frame = 1;
		cg.testmodelent.oldframe = 0;
	}
	if(!cg.testmodelent.hModel){
		cgprintf("Can't register model\n");
		return;
	}

	vecmad(cg.refdef.vieworg, 100, cg.refdef.viewaxis[0], cg.testmodelent.origin);

	angles[PITCH] = 0;
	angles[YAW] = 180 + cg.refdefviewangles[1];
	angles[ROLL] = 0;

	AnglesToAxis(angles, cg.testmodelent.axis);
	cg.testgun = qfalse;
}

/*
Replaces the current view weapon with the given model
*/
void
testgun_f(void)
{
	testmodel_f();
	cg.testgun = qtrue;
	cg.testmodelent.renderfx = RF_MINLIGHT | RF_DEPTHHACK | RF_FIRST_PERSON;
}

void
testmodelnextframe_f(void)
{
	cg.testmodelent.frame++;
	cgprintf("frame %i\n", cg.testmodelent.frame);
}

void
testmodelprevframe_f(void)
{
	cg.testmodelent.frame--;
	if(cg.testmodelent.frame < 0)
		cg.testmodelent.frame = 0;
	cgprintf("frame %i\n", cg.testmodelent.frame);
}

void
testmodelnextskin_f(void)
{
	cg.testmodelent.skinNum++;
	cgprintf("skin %i\n", cg.testmodelent.skinNum);
}

void
testmodelprevskin_f(void)
{
	cg.testmodelent.skinNum--;
	if(cg.testmodelent.skinNum < 0)
		cg.testmodelent.skinNum = 0;
	cgprintf("skin %i\n", cg.testmodelent.skinNum);
}

static void
addtestmodel(void)
{
	int i;

	// re-register the model, because the level may have changed
	cg.testmodelent.hModel = trap_R_RegisterModel(cg.testmodelname);
	if(!cg.testmodelent.hModel){
		cgprintf("Can't register model\n");
		return;
	}

	// if testing a gun, set the origin relative to the view origin
	if(cg.testgun){
		veccpy(cg.refdef.vieworg, cg.testmodelent.origin);
		veccpy(cg.refdef.viewaxis[0], cg.testmodelent.axis[0]);
		veccpy(cg.refdef.viewaxis[1], cg.testmodelent.axis[1]);
		veccpy(cg.refdef.viewaxis[2], cg.testmodelent.axis[2]);

		// allow the position to be adjusted
		for(i = 0; i<3; i++){
			cg.testmodelent.origin[i] += cg.refdef.viewaxis[0][i] * cg_gun_x.value;
			cg.testmodelent.origin[i] += cg.refdef.viewaxis[1][i] * cg_gun_y.value;
			cg.testmodelent.origin[i] += cg.refdef.viewaxis[2][i] * cg_gun_z.value;
		}
	}

	trap_R_AddRefEntityToScene(&cg.testmodelent);
}
