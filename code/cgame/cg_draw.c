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
// draws all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"

enum
{
	FPS_FRAMES		= 4,
	LAG_SAMPLES		= 128,
	MAX_LAGOMETER_PING	= 900,
	MAX_LAGOMETER_RANGE	= 300
};

struct
{
	int	framesamples[LAG_SAMPLES];
	int	nframes;
	int	snapflags[LAG_SAMPLES];
	int	snapsamples[LAG_SAMPLES];
	int	nsnaps;
} lagometer;

int drawTeamOverlayModificationCount = -1;
int sortedteamplayers[TEAM_MAXOVERLAY];
int nsortedteamplayers;
char syschat[256];
char teamchat1[256];
char teamchat2[256];

/*
Draws large numbers for status bar and powerups
*/
static void
drawfield(int x, int y, int width, int value)
{
	char num[16], *ptr;
	int l;
	int frame;
	int spacing;

	if(width < 1)
		return;

	// draw number string
	if(width > 5)
		width = 5;

	switch(width){
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	}

	Com_sprintf(num, sizeof(num), "%i", value);
	l = strlen(num);
	if(l > width)
		l = width;
	spacing = 0;
	if(width > 1)
		spacing = FIELD_CHAR_SPACING;
	x += 2 + (CHAR_WIDTH+spacing)*(width - l);

	ptr = num;
	while(*ptr && l){
		if(*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		drawpic(x, y, CHAR_WIDTH, CHAR_HEIGHT, cgs.media.numberShaders[frame]);
		x += CHAR_WIDTH;
		if(l > 1)
			x += spacing;
		ptr++;
		l--;
	}
}

/*
Same as drawfield, but pads with a zero if necessary.
 */
static void
draw0field(int x, int y, int width, int value)
{
	char num[16], *ptr;
	int l;
	int frame;
	int spacing;

	if(width < 1)
		return;

	// draw number string
	if(width > 5)
		width = 5;

	switch(width){
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	}

	Com_sprintf(num, sizeof(num), "%02i", value);
	l = strlen(num);
	if(l > width)
		l = width;
	spacing = FIELD_CHAR_SPACING;
	x += 2 + (CHAR_WIDTH+spacing)*(width - l);

	ptr = num;
	while(*ptr && l){
		if(*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		drawpic(x, y, CHAR_WIDTH, CHAR_HEIGHT, cgs.media.numberShaders[frame]);
		x += CHAR_WIDTH;
		if(l > 1)
			x += spacing;
		ptr++;
		l--;
	}
}

void
drawmodel(float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles)
{
	refdef_t refdef;
	refEntity_t ent;

	if(!cg_draw3dIcons.integer || !cg_drawIcons.integer)
		return;

	adjustcoords(&x, &y, &w, &h);

	memset(&refdef, 0, sizeof(refdef));

	memset(&ent, 0, sizeof(ent));
	AnglesToAxis(angles, ent.axis);
	veccpy(origin, ent.origin);
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;	// no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear(refdef.viewaxis);

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene(&ent);
	trap_R_RenderScene(&refdef);
}

/*
Used for both the status bar and the scoreboard
*/
void
drawflag(float x, float y, float w, float h, int team, qboolean force2D)
{
	qhandle_t cm;
	float len;
	vec3_t origin, angles;
	vec3_t mins, maxs;
	qhandle_t handle;

	if(!force2D && cg_draw3dIcons.integer){
		vecclear(angles);

		cm = cgs.media.redFlagModel;

		// offset the origin y and z to center the flag
		trap_R_ModelBounds(cm, mins, maxs);

		origin[2] = -0.5 * (mins[2] + maxs[2]);
		origin[1] = 0.5 * (mins[1] + maxs[1]);

		// calculate distance so the flag nearly fills the box
		// assume heads are taller than wide
		len = 0.5 * (maxs[2] - mins[2]);
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		angles[YAW] = 60 * sin(cg.time / 2000.0);;

		if(team == TEAM_RED)
			handle = cgs.media.redFlagModel;
		else if(team == TEAM_BLUE)
			handle = cgs.media.blueFlagModel;
		else if(team == TEAM_FREE)
			handle = cgs.media.neutralFlagModel;
		else
			return;
		drawmodel(x, y, w, h, handle, 0, origin, angles);
	}else if(cg_drawIcons.integer){
		item_t *item;

		if(team == TEAM_RED)
			item = finditemforpowerup(PW_REDFLAG);
		else if(team == TEAM_BLUE)
			item = finditemforpowerup(PW_BLUEFLAG);
		else if(team == TEAM_FREE)
			item = finditemforpowerup(PW_NEUTRALFLAG);
		else
			return;
		if(item)
			drawpic(x, y, w, h, cg_items[ITEM_INDEX(item)].icon);
	}
}

void
drawteambg(int x, int y, int w, int h, float alpha, int team)
{
	vec4_t hcolor;

	hcolor[3] = alpha;
	if(team == TEAM_RED){
		hcolor[0] = 1;
		hcolor[1] = 0;
		hcolor[2] = 0;
	}else if(team == TEAM_BLUE){
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 1;
	}else
		return;
	trap_R_SetColor(hcolor);
	drawpic(x, y, w, h, cgs.media.teamStatusBar);
	trap_R_SetColor(nil);
}

static void
drawstatusbar(void)
{
	const int margin = 20;
	const vec4_t colour = {1.0f, 0.4f, 0.0f, 1.0f};	// orange
	int x, y;
	vec3_t angles;
	vec3_t origin;

	if(cg_drawStatus.integer == 0)
		return;

	vecclear(angles);

	// draw 3D icons first, so the changes back to 2D are minimized
	// token icon
	origin[0] = 120;
	origin[1] = 0;
	origin[2] = 0;
	angles[YAW] = (cg.time & 2047) * 360 / 2048.0f;
	x = margin;
	y = margin;
	drawmodel(x-20, y-15, ICON_SIZE+50, ICON_SIZE+50, cgs.media.tokenModel, 0, origin, angles);

	// lives icon
	origin[0] = 40;
	origin[1] = 0;
	origin[2] = 0;
	x = SCREEN_WIDTH - ICON_SIZE - margin;
	drawmodel(x-8, y-6, ICON_SIZE+8, ICON_SIZE+8, cgs.media.lifeModel, 0, origin, angles);

	// tokens
	trap_R_SetColor(colour);
	x = margin + ICON_SIZE + TEXT_ICON_SPACE;
	draw0field(x, y, 2, cg.disptokens);

	// lives
	trap_R_SetColor(colour);
	x = SCREEN_WIDTH - CHAR_WIDTH*3 - ICON_SIZE - margin + 10;
	drawfield(x, y, 3, cg.displives);
}

/*
upper right corner
*/

static float
drawsnap(float y)
{
	char *s;
	int w;

	s = va("time:%i snap:%i cmd:%i", cg.snap->serverTime,
	       cg.latestsnapnum, cgs.serverCommandSequence);
	w = drawstrlen(s) * BIGCHAR_WIDTH;

	drawbigstr(635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

static float
drawfps(float y)
{
	char *s;
	int w;
	static int previousTimes[FPS_FRAMES];
	static int index;
	int i, total;
	int fps;
	static int previous;
	int t, frametime;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap_Milliseconds();
	frametime = t - previous;
	previous = t;

	previousTimes[index % FPS_FRAMES] = frametime;
	index++;
	if(index > FPS_FRAMES){
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for(i = 0; i < FPS_FRAMES; i++)
			total += previousTimes[i];
		if(!total)
			total = 1;
		fps = 1000 * FPS_FRAMES / total;

		s = va("%ifps", fps);
		w = drawstrlen(s) * BIGCHAR_WIDTH;

		drawbigstr(635 - w, y + 2, s, 1.0F);
	}

	return y + BIGCHAR_HEIGHT + 4;
}

static float
drawtimer(float y)
{
	char *s;
	int w;
	int mins, seconds, tens;
	int msec;

	msec = cg.time - cgs.levelStartTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va("%i:%i%i", mins, tens, seconds);
	w = drawstrlen(s) * BIGCHAR_WIDTH;

	drawbigstr(635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

static void
drawupperright(stereoFrame_t stereoFrame)
{
	float y;

	y = 0;

	if(cg_drawSnapshot.integer)
		y = drawsnap(y);
	if(cg_drawFPS.integer && (stereoFrame == STEREO_CENTER || stereoFrame == STEREO_RIGHT))
		y = drawfps(y);
	if(cg_drawTimer.integer)
		y = drawtimer(y);
}

/*
lower right corner
*/

static float
drawpowerups(float y)
{
	int sorted[MAX_POWERUPS];
	int sortedTime[MAX_POWERUPS];
	int i, j, k;
	int active;
	playerState_t *ps;
	int t;
	item_t *item;
	int x;
	int color;
	float size;
	float f;
	static float colors[2][4] = {
		{0.2f, 1.0f, 0.2f, 1.0f},
		{1.0f, 0.4f, 0.001f, 1.0f}
	};

	ps = &cg.snap->ps;

	if(ps->stats[STAT_HEALTH] <= 0)
		return y;

	// sort the list by time remaining
	active = 0;
	for(i = 0; i < MAX_POWERUPS; i++){
		if(!ps->powerups[i])
			continue;
		t = ps->powerups[i] - cg.time;
		// ZOID--don't draw if the power up has unlimited time (999 seconds)
		// This is true of the CTF flags
		if(t < 0 || t > 999000)
			continue;

		// insert into the list
		for(j = 0; j < active; j++)
			if(sortedTime[j] >= t){
				for(k = active - 1; k >= j; k--){
					sorted[k+1] = sorted[k];
					sortedTime[k+1] = sortedTime[k];
				}
				break;
			}
		sorted[j] = i;
		sortedTime[j] = t;
		active++;
	}

	// draw the icons and timers
	x = 640 - ICON_SIZE - CHAR_WIDTH * 2;
	for(i = 0; i < active; i++){
		item = finditemforpowerup(sorted[i]);

		if(item){
			color = 1;

			y -= ICON_SIZE;

			trap_R_SetColor(colors[color]);
			drawfield(x, y, 2, sortedTime[i] / 1000);

			t = ps->powerups[sorted[i]];
			if(t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME)
				trap_R_SetColor(nil);
			else{
				vec4_t modulate;

				f = (float)(t - cg.time) / POWERUP_BLINK_TIME;
				f -= (int)f;
				modulate[0] = modulate[1] = modulate[2] = modulate[3] = f;
				trap_R_SetColor(modulate);
			}

			if(cg.powerupactive == sorted[i] &&
			   cg.time - cg.poweruptime < PULSE_TIME){
				f = 1.0 - (((float)cg.time - cg.poweruptime) / PULSE_TIME);
				size = ICON_SIZE * (1.0 + (PULSE_SCALE - 1.0) * f);
			}else
				size = ICON_SIZE;

			drawpic(640 - size, y + ICON_SIZE / 2 - size / 2,
				   size, size, trap_R_RegisterShader(item->icon));
		}
	}
	trap_R_SetColor(nil);

	return y;
}

static void
drawlowerright(void)
{
	float y;

	y = 480 - ICON_SIZE;
	drawpowerups(y);
}

void
queuepickupanim(const char *classname)
{
	item_t *it;
	int i;

	i = cg.npkupanims;

	if(i >= MAXPKUPANIMS)
		return;
	for(it = bg_itemlist + 1; it->classname != nil; it++)
		if(Q_stricmp(it->classname, classname) == 0)
			break;
	if(it->classname == nil)
		return;

	switch(it->type){
	case IT_TOKEN:
		vecset(cg.pkupanimstk[i].beg, 0, 310, 260);
		vecset(cg.pkupanimstk[i].end, 0, 60, 60);
		break;
	case IT_LIFE:
		vecset(cg.pkupanimstk[i].beg, 0, 330, 260);
		vecset(cg.pkupanimstk[i].end, 0, 580, 60);
		break;
	case IT_KEY:
		vecset(cg.pkupanimstk[i].beg, 0, 310, 260);
		vecset(cg.pkupanimstk[i].end, 0, 60, 180);
		break;
	default:
		return;
	}
	cg.pkupanimstk[i].item = it - bg_itemlist;
	cg.npkupanims++;
}

static void
drawpickupanim(void)
{
	pickupanim_t *pa;
	qhandle_t model;
	vec3_t pos, viewportpos, angles;
	float t;

	if(cg.npkupanims == 0){
		// set display stats just in case the stack was ever full,
		// causing us to miss increments
		cg.disptokens = cg.snap->ps.stats[STAT_TOKENS];
		cg.displives = cg.snap->ps.persistant[PERS_LIVES];
		return;
	}
	if(cg.time > cg.pkupanimtime){
		if(cg.pkupanimtime != 0){
			// end of anim
			cg.npkupanims--;
			pa = &cg.pkupanimstk[cg.npkupanims];

			// increment the stat displays
			switch(bg_itemlist[pa->item].type){
			case IT_TOKEN:
				cg.disptokens = (cg.disptokens + 1) % 100;
				break;
			case IT_LIFE:
				cg.displives++;
				break;
			default:
				break;
			}

			// play a sound as the pickupanim reaches the stat counter
			if(bg_itemlist[pa->item].pickupsound[1] != nil){
				trap_S_StartLocalSound(
				   trap_S_RegisterSound(bg_itemlist[pa->item].pickupsound[1], qfalse),
			   CHAN_ANNOUNCER);
			}
		}	
		cg.pkupanimstarttime = cg.time;
		cg.pkupanimtime = cg.time + PKUPANIMTIME;
		if(cg.npkupanims == 0){
			cg.pkupanimtime = 0;
			return;
		}
	}

	pa = &cg.pkupanimstk[cg.npkupanims-1];

	// some items may not have had their data registered yet
	if(cg_items[pa->item].models[0] == 0)
		registeritemgfx(pa->item);

	model = cg_items[pa->item].models[0];
	t = 1 - (cg.pkupanimtime - cg.time) / (float)(cg.pkupanimtime - cg.pkupanimstarttime);
	t = MIN(1.0f, t);
	pos[0] = 0;
	pos[1] = pa->beg[1] + t * (pa->end[1] - pa->beg[1]);
	pos[2] = pa->beg[2] + t * (pa->end[2] - pa->beg[2]);
	vecset(viewportpos, 200, 0, 0);
	vecclear(angles);
	// quick rotation
	angles[YAW] = (cg.pkupanimtime - (cg.time & 255)) * 360 / 256.0f;
	drawmodel(pos[1] - ICON_SIZE*1.5f, pos[2] - ICON_SIZE*1.5f,
	   ICON_SIZE*3, ICON_SIZE*3, model, 0, viewportpos, angles);
}

static void
drawteaminfo(void)
{
	const int CHATLOC_Y = 420;	// bottom end
	const int CHATLOC_X = 0;
	int h;
	int i;
	vec4_t hcolor;
	int chatHeight;

	if(cg_teamChatHeight.integer < TEAMCHAT_HEIGHT)
		chatHeight = cg_teamChatHeight.integer;
	else
		chatHeight = TEAMCHAT_HEIGHT;
	if(chatHeight <= 0)
		return;		// disabled

	if(cgs.teamlastchatpos != cgs.teamchatpos){
		if(cg.time - cgs.teamchatmsgtimes[cgs.teamlastchatpos % chatHeight] > cg_teamChatTime.integer)
			cgs.teamlastchatpos++;

		h = (cgs.teamchatpos - cgs.teamlastchatpos) * TINYCHAR_HEIGHT;

		if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED){
			hcolor[0] = 1.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		}else if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE){
			hcolor[0] = 0.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 1.0f;
			hcolor[3] = 0.33f;
		}else{
			hcolor[0] = 0.0f;
			hcolor[1] = 1.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		}

		trap_R_SetColor(hcolor);
		drawpic(CHATLOC_X, CHATLOC_Y - h, 640, h, cgs.media.teamStatusBar);
		trap_R_SetColor(nil);

		hcolor[0] = hcolor[1] = hcolor[2] = 1.0f;
		hcolor[3] = 1.0f;

		for(i = cgs.teamchatpos - 1; i >= cgs.teamlastchatpos; i--)
			drawstr2(CHATLOC_X + TINYCHAR_WIDTH,
			   CHATLOC_Y - (cgs.teamchatpos - i)*TINYCHAR_HEIGHT,
			   cgs.teamchatmsgs[i % chatHeight], hcolor, qfalse, qfalse,
			   TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
	}
}

static void
drawholdable(void)
{
	int value;

	value = cg.snap->ps.stats[STAT_HOLDABLE_ITEM];
	if(value){
		registeritemgfx(value);
		drawpic(640-ICON_SIZE, (SCREEN_HEIGHT-ICON_SIZE)/2, ICON_SIZE, ICON_SIZE,
		   cg_items[value].icon);
	}
}

static void
drawreward(void)
{
	float *color;
	int i, count;
	float x, y;
	char buf[32];

	if(!cg_drawRewards.integer)
		return;

	color = fadecolor(cg.rewardtime, REWARD_TIME);
	if(!color){
		if(cg.rewardstack <= 0)
			return;
		for(i = 0; i < cg.rewardstack; i++){
			cg.rewardsounds[i] = cg.rewardsounds[i+1];
			cg.rewardshaders[i] = cg.rewardshaders[i+1];
			cg.nrewards[i] = cg.nrewards[i+1];
		}
		cg.rewardtime = cg.time;
		cg.rewardstack--;
		color = fadecolor(cg.rewardtime, REWARD_TIME);
		trap_S_StartLocalSound(cg.rewardsounds[0], CHAN_ANNOUNCER);
	}

	trap_R_SetColor(color);

	if(cg.nrewards[0] >= 10){
		y = 56;
		x = 320 - ICON_SIZE/2;
		drawpic(x, y, ICON_SIZE-4, ICON_SIZE-4, cg.rewardshaders[0]);
		Com_sprintf(buf, sizeof(buf), "%d", cg.nrewards[0]);
		x = (SCREEN_WIDTH - SMALLCHAR_WIDTH * drawstrlen(buf)) / 2;
		drawstr2(x, y+ICON_SIZE, buf, color, qfalse, qtrue,
				 SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0);
	}else{
		count = cg.nrewards[0];

		y = 56;
		x = 320 - count * ICON_SIZE/2;
		for(i = 0; i < count; i++){
			drawpic(x, y, ICON_SIZE-4, ICON_SIZE-4, cg.rewardshaders[0]);
			x += ICON_SIZE;
		}
	}
	trap_R_SetColor(nil);
}

/*
lagometer
*/

/*
Adds the current interpolate / extrapolate bar for this frame
*/
void
lagometerframeinfo(void)
{
	int offset;

	offset = cg.time - cg.latestsnapttime;
	lagometer.framesamples[lagometer.nframes & (LAG_SAMPLES - 1)] = offset;
	lagometer.nframes++;
}

/*
Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass nil for a dropped packet.
*/
void
lagometersnapinfo(snapshot_t *snap)
{
	// dropped packet
	if(!snap){
		lagometer.snapsamples[lagometer.nsnaps & (LAG_SAMPLES - 1)] = -1;
		lagometer.nsnaps++;
		return;
	}

	// add this snapshot's info
	lagometer.snapsamples[lagometer.nsnaps & (LAG_SAMPLES - 1)] = snap->ping;
	lagometer.snapflags[lagometer.nsnaps & (LAG_SAMPLES - 1)] = snap->snapFlags;
	lagometer.nsnaps++;
}

/*
Should we draw something differnet for long lag vs no packets?
*/
static void
drawdisconnect(void)
{
	float x, y;
	int cmdNum;
	usercmd_t cmd;
	const char *s;
	int w;

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd(cmdNum, &cmd);
	if(cmd.serverTime <= cg.snap->ps.commandTime
	   || cmd.serverTime > cg.time)	// special check for map_restart
		return;

	// also add text in center of screen
	s = "Connection Interrupted";
	w = drawstrlen(s) * BIGCHAR_WIDTH;
	drawbigstr(320 - w/2, 100, s, 1.0F);

	// blink the icon
	if((cg.time >> 9) & 1)
		return;

	x = 640 - 48;
	y = 480 - 48;

	drawpic(x, y, 48, 48, trap_R_RegisterShader("gfx/2d/net.tga"));
}

static void
drawlagometer(void)
{
	int a, x, y, i;
	float v;
	float ax, ay, aw, ah, mid, range;
	int color;
	float vscale;

	if(!cg_lagometer.integer || cgs.srvislocal){
		drawdisconnect();
		return;
	}

	// draw the graph
	x = 640 - 48;
	y = 480 - 48;

	trap_R_SetColor(nil);
	drawpic(x, y, 48, 48, cgs.media.lagometerShader);

	ax = x;
	ay = y;
	aw = 48;
	ah = 48;
	adjustcoords(&ax, &ay, &aw, &ah);

	color = -1;
	range = ah / 3;
	mid = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for(a = 0; a < aw; a++){
		i = (lagometer.nframes - 1 - a) & (LAG_SAMPLES - 1);
		v = lagometer.framesamples[i];
		v *= vscale;
		if(v > 0){
			if(color != 1){
				color = 1;
				trap_R_SetColor(g_color_table[ColorIndex(COLOR_YELLOW)]);
			}
			if(v > range)
				v = range;
			trap_R_DrawStretchPic(ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}else if(v < 0){
			if(color != 2){
				color = 2;
				trap_R_SetColor(g_color_table[ColorIndex(COLOR_BLUE)]);
			}
			v = -v;
			if(v > range)
				v = range;
			trap_R_DrawStretchPic(ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for(a = 0; a < aw; a++){
		i = (lagometer.nsnaps - 1 - a) & (LAG_SAMPLES - 1);
		v = lagometer.snapsamples[i];
		if(v > 0){
			if(lagometer.snapflags[i] & SNAPFLAG_RATE_DELAYED){
				if(color != 5){
					color = 5;	// YELLOW for rate delay
					trap_R_SetColor(g_color_table[ColorIndex(COLOR_YELLOW)]);
				}
			}else if(color != 3){
				color = 3;
				trap_R_SetColor(g_color_table[ColorIndex(COLOR_GREEN)]);
			}
			v = v * vscale;
			if(v > range)
				v = range;
			trap_R_DrawStretchPic(ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}else if(v < 0){
			if(color != 4){
				color = 4;	// RED for dropped snapshots
				trap_R_SetColor(g_color_table[ColorIndex(COLOR_RED)]);
			}
			trap_R_DrawStretchPic(ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader);
		}
	}

	trap_R_SetColor(nil);

	if(cg_nopredict.integer || cg_synchronousClients.integer)
		drawbigstr(x, y, "snc", 1.0);

	drawdisconnect();
}

/*
speedometer
*/

void
drawspeedometer(void)
{
	int spd;
	char s[32];
	float w;

	if(cg.snap == nil)
		spd = 0;
	else
		spd = round(sqrt(Square(cg.snap->ps.velocity[0]) +
		   Square(cg.snap->ps.velocity[1])));
	Com_sprintf(s, sizeof s, "%iups", spd);
	w = drawstrlen(s) * BIGCHAR_WIDTH;
	drawbigstr(0.5f*SCREEN_WIDTH - 0.5f*w, 340, s, 1.0f);
}

/*
center printing
*/

/*
Called for important messages that should stay in the center of the screen
for a few moments
*/
void
centerprint(const char *str, int y, int charWidth)
{
	char *s;

	Q_strncpyz(cg.centerprint, str, sizeof(cg.centerprint));

	cg.centerprinttime = cg.time;
	cg.centerprinty = y;
	cg.centerprintcharwidth = charWidth;

	// count the number of lines for centering
	cg.centerprintlines = 1;
	s = cg.centerprint;
	while(*s){
		if(*s == '\n')
			cg.centerprintlines++;
		s++;
	}
}

static void
drawcenterstr(void)
{
	char *start;
	int l;
	int x, y, w;
	float *color;

	if(!cg.centerprinttime)
		return;

	color = fadecolor(cg.centerprinttime, 1000 * cg_centertime.value);
	if(!color)
		return;

	trap_R_SetColor(color);

	start = cg.centerprint;

	y = cg.centerprinty - cg.centerprintlines * BIGCHAR_HEIGHT / 2;

	while(1){
		char linebuffer[1024];

		for(l = 0; l < 50; l++){
			if(!start[l] || start[l] == '\n')
				break;
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		w = cg.centerprintcharwidth * drawstrlen(linebuffer);

		x = (SCREEN_WIDTH - w) / 2;

		drawstr2(x, y, linebuffer, color, qfalse, qtrue,
				 cg.centerprintcharwidth, (int)(cg.centerprintcharwidth * 1.5), 0);

		y += cg.centerprintcharwidth * 1.5;
		while(*start && (*start != '\n'))
			start++;
		if(!*start)
			break;
		start++;
	}

	trap_R_SetColor(nil);
}

/*
crosshair
*/

static void
drawxhair(void)
{
	float w, h;
	qhandle_t hShader;
	float f;
	float x, y;
	int ca;

	if(!cg_drawCrosshair.integer)
		return;

	if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
		return;

	if(cg.thirdperson)
		return;

	// set color based on health
	if(cg_crosshairHealth.integer){
		vec4_t hcolor;

		colorforhealth(hcolor);
		trap_R_SetColor(hcolor);
	}else
		trap_R_SetColor(nil);

	w = h = cg_crosshairSize.value;

	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.itempkupblendtime;
	if(f > 0 && f < ITEM_BLOB_TIME){
		f /= ITEM_BLOB_TIME;
		w *= (1 + f);
		h *= (1 + f);
	}

	x = cg_crosshairX.integer;
	y = cg_crosshairY.integer;
	adjustcoords(&x, &y, &w, &h);

	ca = cg_drawCrosshair.integer;
	if(ca < 0)
		ca = 0;
	hShader = cgs.media.crosshairShader[ca % NUM_CROSSHAIRS];

	trap_R_DrawStretchPic(x + cg.refdef.x + 0.5 * (cg.refdef.width - w),
	   y + cg.refdef.y + 0.5 * (cg.refdef.height - h),
	   w, h, 0, 0, 1, 1, hShader);
}

static void
drawxhair3d(void)
{
	float w;
	qhandle_t hShader;
	float f;
	int ca;

	trace_t trace;
	vec3_t endpos;
	float stereoSep, zProj, maxdist, xmax;
	char rendererinfos[128];
	refEntity_t ent;

	if(!cg_drawCrosshair.integer)
		return;

	if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
		return;

	if(cg.thirdperson)
		return;

	w = cg_crosshairSize.value;

	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.itempkupblendtime;
	if(f > 0 && f < ITEM_BLOB_TIME){
		f /= ITEM_BLOB_TIME;
		w *= (1 + f);
	}

	ca = cg_drawCrosshair.integer;
	if(ca < 0)
		ca = 0;
	hShader = cgs.media.crosshairShader[ca % NUM_CROSSHAIRS];

	// Use a different method rendering the crosshair so players don't see two of them when
	// focusing their eyes at distant objects with high stereo separation
	// We are going to trace to the next shootable object and place the crosshair in front of it.

	// first get all the important renderer information
	trap_Cvar_VariableStringBuffer("r_zProj", rendererinfos, sizeof(rendererinfos));
	zProj = atof(rendererinfos);
	trap_Cvar_VariableStringBuffer("r_stereoSeparation", rendererinfos, sizeof(rendererinfos));
	stereoSep = zProj / atof(rendererinfos);

	xmax = zProj * tan(cg.refdef.fov_x * M_PI / 360.0f);

	// let the trace run through until a change in stereo separation of the 
	// crosshair becomes less than one pixel.
	maxdist = cgs.glconfig.vidWidth * stereoSep * zProj / (2 * xmax);
	vecmad(cg.refdef.vieworg, maxdist, cg.refdef.viewaxis[0], endpos);
	cgtrace(&trace, cg.refdef.vieworg, nil, nil, endpos, 0, MASK_SHOT);

	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_DEPTHHACK | RF_CROSSHAIR;

	veccpy(trace.endpos, ent.origin);

	// scale the crosshair so it appears the same size for all distances
	ent.radius = w / 640 * xmax * trace.fraction * maxdist / zProj;
	ent.customShader = hShader;

	trap_R_AddRefEntityToScene(&ent);
}

static void
xhairscan(void)
{
	trace_t trace;
	vec3_t start, end;
	int content;

	veccpy(cg.refdef.vieworg, start);
	vecmad(start, 131072, cg.refdef.viewaxis[0], end);

	cgtrace(&trace, start, vec3_origin, vec3_origin, end,
		 cg.snap->ps.clientNum, CONTENTS_SOLID|CONTENTS_BODY);
	if(trace.entityNum >= MAX_CLIENTS)
		return;

	// if the player is in fog, don't show it
	content = pointcontents(trace.endpos, 0);
	if(content & CONTENTS_FOG)
		return;

	// if the player is invisible, don't show it
	if(cg_entities[trace.entityNum].currstate.powerups & (1 << PW_INVIS))
		return;

	// update the fade timer
	cg.xhairclnum = trace.entityNum;
	cg.xhaircltime = cg.time;
}

static void
drawxhairnames(void)
{
	float *color;
	char *name;
	float w;

	if(!cg_drawCrosshair.integer)
		return;
	if(!cg_drawCrosshairNames.integer)
		return;
	if(cg.thirdperson)
		return;

	// scan the known entities to see if the crosshair is sighted on one
	xhairscan();

	// draw the name of the player being looked at
	color = fadecolor(cg.xhaircltime, 1000);
	if(!color){
		trap_R_SetColor(nil);
		return;
	}

	name = cgs.clientinfo[cg.xhairclnum].name;
	w = drawstrlen(name) * BIGCHAR_WIDTH;
	drawbigstr(320 - w / 2, 170, name, color[3] * 0.5f);
	trap_R_SetColor(nil);
}

static void
drawspec(void)
{
	drawbigstr(320 - 9 * 8, 440, "SPECTATOR", 1.0F);
	if(cgs.gametype == GT_TOURNAMENT)
		drawbigstr(320 - 15 * 8, 460, "waiting to play", 1.0F);
	else if(cgs.gametype >= GT_TEAM)
		drawbigstr(320 - 39 * 8, 460, "press ESC and use the JOIN menu to play", 1.0F);
}

static void
drawvote(void)
{
	char *s;
	int sec;

	if(!cgs.votetime)
		return;

	// play a talk beep whenever it is modified
	if(cgs.votemodified){
		cgs.votemodified = qfalse;
		trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
	}

	sec = (VOTE_TIME - (cg.time - cgs.votetime)) / 1000;
	if(sec < 0)
		sec = 0;

	s = va("VOTE(%i):%s yes:%i no:%i", sec, cgs.votestr, cgs.voteyes, cgs.voteno);
	drawsmallstr(0, 58, s, 1.0F);
}

static void
drawteamvote(void)
{
	char *s;
	int sec, cs_offset;

	if(cgs.clientinfo[cg.clientNum].team == TEAM_RED)
		cs_offset = 0;
	else if(cgs.clientinfo[cg.clientNum].team == TEAM_BLUE)
		cs_offset = 1;
	else
		return;

	if(!cgs.teamvotetime[cs_offset])
		return;

	// play a talk beep whenever it is modified
	if(cgs.teamVoteModified[cs_offset]){
		cgs.teamVoteModified[cs_offset] = qfalse;
		trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
	}

	sec = (VOTE_TIME - (cg.time - cgs.teamvotetime[cs_offset])) / 1000;
	if(sec < 0)
		sec = 0;
	s = va("TEAMVOTE(%i):%s yes:%i no:%i", sec, cgs.teamvotestr[cs_offset],
	       cgs.teamvoteyes[cs_offset], cgs.teamvoteno[cs_offset]);
	drawsmallstr(0, 90, s, 1.0F);
}

static void
drawintermission(void)
{
	if(cgs.gametype == GT_SINGLE_PLAYER){
		drawcenterstr();
		return;
	}
	drawintermissionscores();
}

static qboolean
drawfollow(void)
{
	float x;
	vec4_t color;
	const char *name;

	if(!(cg.snap->ps.pm_flags & PMF_FOLLOW))
		return qfalse;
	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	drawbigstr(320 - 9 * 8, 24, "following", 1.0F);

	name = cgs.clientinfo[cg.snap->ps.clientNum].name;

	x = 0.5 * (640 - GIANT_WIDTH * drawstrlen(name));

	drawstr2(x, 40, name, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0);

	return qtrue;
}

static void
drawwarmup(void)
{
	int w;
	int sec;
	int i;
	int cw;
	clientinfo_t *ci1, *ci2;
	const char *s;

	sec = cg.warmup;
	if(!sec)
		return;

	if(sec < 0){
		s = "Waiting for players";
		w = drawstrlen(s) * BIGCHAR_WIDTH;
		drawbigstr(320 - w / 2, 24, s, 1.0F);
		cg.warmupcount = 0;
		return;
	}

	if(cgs.gametype == GT_TOURNAMENT){
		// find the two active players
		ci1 = nil;
		ci2 = nil;
		for(i = 0; i < cgs.maxclients; i++)
			if(cgs.clientinfo[i].infovalid && cgs.clientinfo[i].team == TEAM_FREE){
				if(!ci1)
					ci1 = &cgs.clientinfo[i];
				else
					ci2 = &cgs.clientinfo[i];
			}

		if(ci1 && ci2){
			s = va("%s vs %s", ci1->name, ci2->name);
			w = drawstrlen(s);
			if(w > 640 / GIANT_WIDTH)
				cw = 640 / w;
			else
				cw = GIANT_WIDTH;
			drawstr2(320 - w * cw/2, 20, s, colorWhite,
					 qfalse, qtrue, cw, (int)(cw * 1.5f), 0);
		}
	}else{
		switch(cgs.gametype){
		case GT_COOP:
			s = "Co-op";
			break;
		case GT_SINGLE_PLAYER:
			s = "Single Player";
			break;
		case GT_FFA:
			s = "Free For All";
			break;
		case GT_TEAM:
			s = "Team Deathmatch";
			break;
		case GT_CTF:
			s = "Capture the Flag";
			break;
		default:
			s = "";
		}
		w = drawstrlen(s);
		if(w > 640 / GIANT_WIDTH)
			cw = 640 / w;
		else
			cw = GIANT_WIDTH;
		drawstr2(320 - w * cw/2, 25, s, colorWhite,
				 qfalse, qtrue, cw, (int)(cw * 1.1f), 0);
	}

	sec = (sec - cg.time) / 1000;
	if(sec < 0){
		cg.warmup = 0;
		sec = 0;
	}
	s = va("Starts in: %i", sec + 1);
	if(sec != cg.warmupcount){
		cg.warmupcount = sec;
		switch(sec){
		case 0:
			trap_S_StartLocalSound(cgs.media.count1Sound, CHAN_ANNOUNCER);
			break;
		case 1:
			trap_S_StartLocalSound(cgs.media.count2Sound, CHAN_ANNOUNCER);
			break;
		case 2:
			trap_S_StartLocalSound(cgs.media.count3Sound, CHAN_ANNOUNCER);
			break;
		default:
			break;
		}
	}

	switch(cg.warmupcount){
	case 0:
		cw = 28;
		break;
	case 1:
		cw = 24;
		break;
	case 2:
		cw = 20;
		break;
	default:
		cw = 16;
		break;
	}

	w = drawstrlen(s);
	drawstr2(320 - w * cw/2, 70, s, colorWhite,
			 qfalse, qtrue, cw, (int)(cw * 1.5), 0);
}

static void
drawgameover(void)
{
	int i;

	if(cg.snap->ps.persistant[PERS_LIVES] > 0)
		return;

	for(i = 0; i < cgs.maxclients; i++){
		if(cg_entities[i].currstate.eType == ET_PLAYER &&
		   !(cg_entities[i].currstate.eFlags & EF_CONNECTION) &&
		   i != cg.snap->ps.clientNum)
			break;
	}
	// if we're not alone in the server, draw a different 
	// gameover message
	if(i == cgs.maxclients)
		centerprint("Game over!", 0.4f*SCREEN_HEIGHT, 18);
	else
		centerprint("Out of lives!", 0.4f*SCREEN_HEIGHT, 18);
}

static void
draw2d(stereoFrame_t stereoFrame)
{
	// if we are taking a levelshot for the menu, don't draw anything
	if(cg.levelshot)
		return;

	if(cg_draw2D.integer == 0)
		return;

	if(cg.snap->ps.pm_type == PM_INTERMISSION){
		drawintermission();
		return;
	}

	/*
	    if(cg.cameramode){
	            return;
	    }
	*/
	if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR){
		drawspec();

		if(stereoFrame == STEREO_CENTER)
			drawxhair();

		drawxhairnames();
	}else{
		// don't draw any status if dead or the scoreboard is being explicitly shown
		if(!cg.showscores && cg.snap->ps.stats[STAT_HEALTH] > 0){
			drawstatusbar();

			if(stereoFrame == STEREO_CENTER)
				drawxhair();
			drawxhairnames();

			drawholdable();
			drawreward();
		}

		if(cgs.gametype >= GT_TEAM){
			drawteaminfo();
		}
	}

	drawvote();
	drawteamvote();

	drawlagometer();

	if(cg_drawspeed.integer)
		drawspeedometer();

	drawupperright(stereoFrame);

	drawlowerright();

	drawpickupanim();

	if(!drawfollow())
		drawwarmup();

	drawgameover();

	// don't draw center string if scoreboard is up
	cg.scoreboardshown = drawscoreboard();
	if(!cg.scoreboardshown)
		drawcenterstr();
}

/*
Perform all drawing needed to completely fill the screen
*/
void
drawactive(stereoFrame_t stereoview)
{
	// optionally draw the info screen instead
	if(!cg.snap){
		drawinfo();
		return;
	}

	// clear around the rendered view if sized down
	tileclear();

	if(stereoview != STEREO_CENTER)
		drawxhair3d();

	// draw 3D view
	trap_R_RenderScene(&cg.refdef);

	// draw status bar and other floating elements
	draw2d(stereoview);
}
