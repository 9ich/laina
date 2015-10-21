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
// cg_main.c -- initialization and primary entry point for cgame
#include "cg_local.h"


int forceModelModificationCount = -1;

void	cginit(int serverMessageNum, int serverCommandSequence, int clientNum);
void	cgshutdown(void);

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .qvm file
================
*/
Q_EXPORT intptr_t
vmMain(int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11)
{
	switch(command){
	case CG_INIT:
		cginit(arg0, arg1, arg2);
		return 0;
	case CG_SHUTDOWN:
		cgshutdown();
		return 0;
	case CG_CONSOLE_COMMAND:
		return consolecmd();
	case CG_DRAW_ACTIVE_FRAME:
		drawframe(arg0, arg1, arg2);
		return 0;
	case CG_CROSSHAIR_PLAYER:
		return xhairplayer();
	case CG_LAST_ATTACKER:
		return getlastattacker();
	case CG_KEY_EVENT:
		keyevent(arg0, arg1);
		return 0;
	case CG_MOUSE_EVENT:
		mouseevent(arg0, arg1);
		return 0;
	case CG_EVENT_HANDLING:
		eventhandling(arg0);
		return 0;
	default:
		cgerrorf("vmMain: unknown command %i", command);
		break;
	}
	return -1;
}

cg_t cg;
cgs_t cgs;
cent_t cg_entities[MAX_GENTITIES];
weapinfo_t cg_weapons[MAX_WEAPONS];
iteminfo_t cg_items[MAX_ITEMS];

vmCvar_t cg_railTrailTime;
vmCvar_t cg_centertime;
vmCvar_t cg_runpitch;
vmCvar_t cg_runroll;
vmCvar_t cg_bobup;
vmCvar_t cg_bobpitch;
vmCvar_t cg_bobroll;
vmCvar_t cg_swingSpeed;
vmCvar_t cg_shadows;
vmCvar_t cg_gibs;
vmCvar_t cg_drawTimer;
vmCvar_t cg_drawFPS;
vmCvar_t cg_drawSnapshot;
vmCvar_t cg_draw3dIcons;
vmCvar_t cg_drawIcons;
vmCvar_t cg_drawAmmoWarning;
vmCvar_t cg_drawCrosshair;
vmCvar_t cg_drawCrosshairNames;
vmCvar_t cg_drawRewards;
vmCvar_t cg_crosshairSize;
vmCvar_t cg_crosshairX;
vmCvar_t cg_crosshairY;
vmCvar_t cg_crosshairHealth;
vmCvar_t cg_draw2D;
vmCvar_t cg_drawStatus;
vmCvar_t cg_animSpeed;
vmCvar_t cg_debugAnim;
vmCvar_t cg_debugPosition;
vmCvar_t cg_debugEvents;
vmCvar_t cg_errorDecay;
vmCvar_t cg_nopredict;
vmCvar_t cg_noPlayerAnims;
vmCvar_t cg_showmiss;
vmCvar_t cg_footsteps;
vmCvar_t cg_addMarks;
vmCvar_t cg_brassTime;
vmCvar_t cg_viewsize;
vmCvar_t cg_drawGun;
vmCvar_t cg_gun_frame;
vmCvar_t cg_gun_x;
vmCvar_t cg_gun_y;
vmCvar_t cg_gun_z;
vmCvar_t cg_tracerChance;
vmCvar_t cg_tracerWidth;
vmCvar_t cg_tracerLength;
vmCvar_t cg_autoswitch;
vmCvar_t cg_ignore;
vmCvar_t cg_simpleItems;
vmCvar_t cg_fov;
vmCvar_t cg_zoomFov;
vmCvar_t cg_thirdPerson;
vmCvar_t cg_thirdPersonRange;
vmCvar_t cg_thirdPersonAngle;
vmCvar_t cg_lagometer;
vmCvar_t cg_drawAttacker;
vmCvar_t cg_synchronousClients;
vmCvar_t cg_teamChatTime;
vmCvar_t cg_teamChatHeight;
vmCvar_t cg_stats;
vmCvar_t cg_buildScript;
vmCvar_t cg_forceModel;
vmCvar_t cg_paused;
vmCvar_t cg_blood;
vmCvar_t cg_predictItems;
vmCvar_t cg_deferPlayers;
vmCvar_t cg_drawTeamOverlay;
vmCvar_t cg_teamOverlayUserinfo;
vmCvar_t cg_drawFriend;
vmCvar_t cg_teamChatsOnly;
vmCvar_t cg_hudFiles;
vmCvar_t cg_scorePlum;
vmCvar_t cg_smoothClients;
vmCvar_t pmove_fixed;
//vmCvar_t	cg_pmove_fixed;
vmCvar_t pmove_msec;
vmCvar_t cg_pmove_msec;
vmCvar_t cg_cameraMode;
vmCvar_t cg_cameraOrbit;
vmCvar_t cg_cameraOrbitDelay;
vmCvar_t cg_timescaleFadeEnd;
vmCvar_t cg_timescaleFadeSpeed;
vmCvar_t cg_timescale;
vmCvar_t cg_smallFont;
vmCvar_t cg_bigFont;
vmCvar_t cg_noTaunt;
vmCvar_t cg_noProjectileTrail;
vmCvar_t cg_oldRail;
vmCvar_t cg_oldRocket;
vmCvar_t cg_oldPlasma;
vmCvar_t cg_trueLightning;


typedef struct
{
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int		cvarFlags;
} cvarTable_t;

static cvarTable_t cvarTable[] = {
	{&cg_ignore, "cg_ignore", "0", 0},	// used for debugging
	{&cg_autoswitch, "cg_autoswitch", "1", CVAR_ARCHIVE},
	{&cg_drawGun, "cg_drawGun", "1", CVAR_ARCHIVE},
	{&cg_zoomFov, "cg_zoomfov", "22.5", CVAR_ARCHIVE},
	{&cg_fov, "cg_fov", "90", CVAR_ARCHIVE},
	{&cg_viewsize, "cg_viewsize", "100", CVAR_ARCHIVE},
	{&cg_shadows, "cg_shadows", "1", CVAR_ARCHIVE},
	{&cg_gibs, "cg_gibs", "1", CVAR_ARCHIVE},
	{&cg_draw2D, "cg_draw2D", "1", CVAR_ARCHIVE},
	{&cg_drawStatus, "cg_drawStatus", "1", CVAR_ARCHIVE},
	{&cg_drawTimer, "cg_drawTimer", "0", CVAR_ARCHIVE},
	{&cg_drawFPS, "cg_drawFPS", "0", CVAR_ARCHIVE},
	{&cg_drawSnapshot, "cg_drawSnapshot", "0", CVAR_ARCHIVE},
	{&cg_draw3dIcons, "cg_draw3dIcons", "1", CVAR_ARCHIVE},
	{&cg_drawIcons, "cg_drawIcons", "1", CVAR_ARCHIVE},
	{&cg_drawAmmoWarning, "cg_drawAmmoWarning", "1", CVAR_ARCHIVE},
	{&cg_drawAttacker, "cg_drawAttacker", "1", CVAR_ARCHIVE},
	{&cg_drawCrosshair, "cg_drawCrosshair", "4", CVAR_ARCHIVE},
	{&cg_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE},
	{&cg_drawRewards, "cg_drawRewards", "1", CVAR_ARCHIVE},
	{&cg_crosshairSize, "cg_crosshairSize", "24", CVAR_ARCHIVE},
	{&cg_crosshairHealth, "cg_crosshairHealth", "1", CVAR_ARCHIVE},
	{&cg_crosshairX, "cg_crosshairX", "0", CVAR_ARCHIVE},
	{&cg_crosshairY, "cg_crosshairY", "0", CVAR_ARCHIVE},
	{&cg_brassTime, "cg_brassTime", "2500", CVAR_ARCHIVE},
	{&cg_simpleItems, "cg_simpleItems", "0", CVAR_ARCHIVE},
	{&cg_addMarks, "cg_marks", "1", CVAR_ARCHIVE},
	{&cg_lagometer, "cg_lagometer", "1", CVAR_ARCHIVE},
	{&cg_railTrailTime, "cg_railTrailTime", "400", CVAR_ARCHIVE},
	{&cg_gun_x, "cg_gunX", "0", CVAR_CHEAT},
	{&cg_gun_y, "cg_gunY", "0", CVAR_CHEAT},
	{&cg_gun_z, "cg_gunZ", "0", CVAR_CHEAT},
	{&cg_centertime, "cg_centertime", "3", CVAR_CHEAT},
	{&cg_runpitch, "cg_runpitch", "0.002", CVAR_ARCHIVE},
	{&cg_runroll, "cg_runroll", "0.005", CVAR_ARCHIVE},
	{&cg_bobup, "cg_bobup", "0.005", CVAR_CHEAT},
	{&cg_bobpitch, "cg_bobpitch", "0.002", CVAR_ARCHIVE},
	{&cg_bobroll, "cg_bobroll", "0.002", CVAR_ARCHIVE},
	{&cg_swingSpeed, "cg_swingSpeed", "0.3", CVAR_CHEAT},
	{&cg_animSpeed, "cg_animspeed", "1", CVAR_CHEAT},
	{&cg_debugAnim, "cg_debuganim", "0", CVAR_CHEAT},
	{&cg_debugPosition, "cg_debugposition", "0", CVAR_CHEAT},
	{&cg_debugEvents, "cg_debugevents", "0", CVAR_CHEAT},
	{&cg_errorDecay, "cg_errordecay", "100", 0},
	{&cg_nopredict, "cg_nopredict", "0", 0},
	{&cg_noPlayerAnims, "cg_noplayeranims", "0", CVAR_CHEAT},
	{&cg_showmiss, "cg_showmiss", "0", 0},
	{&cg_footsteps, "cg_footsteps", "1", CVAR_CHEAT},
	{&cg_tracerChance, "cg_tracerchance", "0.4", CVAR_CHEAT},
	{&cg_tracerWidth, "cg_tracerwidth", "1", CVAR_CHEAT},
	{&cg_tracerLength, "cg_tracerlength", "100", CVAR_CHEAT},
	{&cg_thirdPersonRange, "cg_thirdPersonRange", "80", CVAR_ARCHIVE},
	{&cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", CVAR_CHEAT},
	{&cg_thirdPerson, "cg_thirdPerson", "1", CVAR_ARCHIVE},
	{&cg_teamChatTime, "cg_teamChatTime", "3000", CVAR_ARCHIVE},
	{&cg_teamChatHeight, "cg_teamChatHeight", "0", CVAR_ARCHIVE},
	{&cg_forceModel, "cg_forceModel", "0", CVAR_ARCHIVE},
	{&cg_predictItems, "cg_predictItems", "1", CVAR_ARCHIVE},
	{&cg_deferPlayers, "cg_deferPlayers", "1", CVAR_ARCHIVE},
	{&cg_drawTeamOverlay, "cg_drawTeamOverlay", "0", CVAR_ARCHIVE},
	{&cg_teamOverlayUserinfo, "teamoverlay", "0", CVAR_ROM | CVAR_USERINFO},
	{&cg_stats, "cg_stats", "0", 0},
	{&cg_drawFriend, "cg_drawFriend", "1", CVAR_ARCHIVE},
	{&cg_teamChatsOnly, "cg_teamChatsOnly", "0", CVAR_ARCHIVE},
	// the following variables are created in other parts of the system,
	// but we also reference them here
	{&cg_buildScript, "com_buildScript", "0", 0},	// force loading of all possible data amd error on failures
	{&cg_paused, "cl_paused", "0", CVAR_ROM},
	{&cg_blood, "com_blood", "1", CVAR_ARCHIVE},
	{&cg_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO},
	{&cg_cameraOrbit, "cg_cameraOrbit", "0", CVAR_CHEAT},
	{&cg_cameraOrbitDelay, "cg_cameraOrbitDelay", "50", CVAR_ARCHIVE},
	{&cg_timescaleFadeEnd, "cg_timescaleFadeEnd", "1", 0},
	{&cg_timescaleFadeSpeed, "cg_timescaleFadeSpeed", "0", 0},
	{&cg_timescale, "timescale", "1", 0},
	{&cg_scorePlum, "cg_scorePlums", "1", CVAR_ARCHIVE},
	{&cg_smoothClients, "cg_smoothClients", "0", CVAR_ARCHIVE},
	{&cg_cameraMode, "com_cameraMode", "0", CVAR_CHEAT},

	{&pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO},
	{&pmove_msec, "pmove_msec", "8", CVAR_SYSTEMINFO},
	{&cg_noTaunt, "cg_noTaunt", "0", CVAR_ARCHIVE},
	{&cg_noProjectileTrail, "cg_noProjectileTrail", "0", CVAR_ARCHIVE},
	{&cg_smallFont, "ui_smallFont", "0.25", CVAR_ARCHIVE},
	{&cg_bigFont, "ui_bigFont", "0.4", CVAR_ARCHIVE},
	{&cg_oldRail, "cg_oldRail", "1", CVAR_ARCHIVE},
	{&cg_oldRocket, "cg_oldRocket", "1", CVAR_ARCHIVE},
	{&cg_oldPlasma, "cg_oldPlasma", "1", CVAR_ARCHIVE},
	{&cg_trueLightning, "cg_trueLightning", "0.0", CVAR_ARCHIVE}
//	{ &cg_pmove_fixed, "cg_pmove_fixed", "0", CVAR_USERINFO | CVAR_ARCHIVE }
};

static int cvarTableSize = ARRAY_LEN(cvarTable);

void
registercvars(void)
{
	int i;
	cvarTable_t *cv;
	char var[MAX_TOKEN_CHARS];

	for(i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++)
		trap_Cvar_Register(cv->vmCvar, cv->cvarName,
				   cv->defaultString, cv->cvarFlags);

	// see if we are also running the server on this machine
	trap_Cvar_VariableStringBuffer("sv_running", var, sizeof(var));
	cgs.srvislocal = atoi(var);

	forceModelModificationCount = cg_forceModel.modificationCount;

	trap_Cvar_Register(nil, "model", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE);
	trap_Cvar_Register(nil, "headmodel", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE);
	trap_Cvar_Register(nil, "team_model", DEFAULT_TEAM_MODEL, CVAR_ARCHIVE);
	trap_Cvar_Register(nil, "team_headmodel", DEFAULT_TEAM_HEAD, CVAR_ARCHIVE);
}

static void
forcemodelchange(void)
{
	int i;

	for(i = 0; i<MAX_CLIENTS; i++){
		const char *clientInfo;

		clientInfo = getconfigstr(CS_PLAYERS+i);
		if(!clientInfo[0])
			continue;
		newclientinfo(i);
	}
}

void
updatecvars(void)
{
	int i;
	cvarTable_t *cv;

	for(i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++)
		trap_Cvar_Update(cv->vmCvar);

	// check for modications here

	// If team overlay is on, ask for updates from the server.  If it's off,
	// let the server know so we don't receive it
	if(drawTeamOverlayModificationCount != cg_drawTeamOverlay.modificationCount){
		drawTeamOverlayModificationCount = cg_drawTeamOverlay.modificationCount;

		if(cg_drawTeamOverlay.integer > 0)
			trap_Cvar_Set("teamoverlay", "1");
		else
			trap_Cvar_Set("teamoverlay", "0");
	}

	// if force model changed
	if(forceModelModificationCount != cg_forceModel.modificationCount){
		forceModelModificationCount = cg_forceModel.modificationCount;
		forcemodelchange();
	}
}

int
xhairplayer(void)
{
	if(cg.time > (cg.xhaircltime + 1000))
		return -1;
	return cg.xhairclnum;
}

int
getlastattacker(void)
{
	if(!cg.attackertime)
		return -1;
	return cg.snap->ps.persistant[PERS_ATTACKER];
}

void QDECL
cgprintf(const char *msg, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	trap_Print(text);
}

void QDECL
cgerrorf(const char *msg, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	trap_Error(text);
}

void QDECL
Com_Error(int level, const char *error, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	trap_Error(text);
}

void QDECL
Com_Printf(const char *msg, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	trap_Print(text);
}

const char *
cgargv(int arg)
{
	static char buffer[MAX_STRING_CHARS];

	trap_Argv(arg, buffer, sizeof(buffer));

	return buffer;
}

//========================================================================

/*
The server says this item is used on this level
*/
static void
regitemsounds(int itemNum)
{
	item_t *item;
	char data[MAX_QPATH];
	char *s, *start;
	int i, len;

	item = &bg_itemlist[itemNum];

	for(i = 0; i < MAX_PICKUP_SOUNDS && item->pickupsound[i] != nil; i++)
		trap_S_RegisterSound(item->pickupsound[i], qfalse);

	// parse the space separated precache string for other media
	s = item->sounds;
	if(!s || !s[0])
		return;

	while(*s){
		start = s;
		while(*s && *s != ' ')
			s++;

		len = s-start;
		if(len >= MAX_QPATH || len < 5){
			cgerrorf("PrecacheItem: %s has bad precache string",
				 item->classname);
			return;
		}
		memcpy(data, start, len);
		data[len] = 0;
		if(*s)
			s++;

		if(!strcmp(data+len-3, "wav"))
			trap_S_RegisterSound(data, qfalse);
	}
}

/*
called during a precache command
*/
static void
regsounds(void)
{
	int i;
	char items[MAX_ITEMS+1];
	char name[MAX_QPATH];
	const char *soundName;

	cgs.media.airjumpSound = trap_S_RegisterSound("sound/player/boing", qfalse);
	cgs.media.crateSmash = trap_S_RegisterSound("sound/crates/smash", qfalse);
	cgs.media.flightSound = trap_S_RegisterSound("sound/items/flight.wav", qfalse);
	cgs.media.gibBounce1Sound = trap_S_RegisterSound("sound/player/gibimp1.wav", qfalse);
	cgs.media.gibBounce2Sound = trap_S_RegisterSound("sound/player/gibimp2.wav", qfalse);
	cgs.media.gibBounce3Sound = trap_S_RegisterSound("sound/player/gibimp3.wav", qfalse);
	cgs.media.gibSound = trap_S_RegisterSound("sound/player/gibsplt1.wav", qfalse);
	cgs.media.hitSound = trap_S_RegisterSound("sound/feedback/hit.wav", qfalse);
	cgs.media.jumpPadSound = trap_S_RegisterSound("sound/world/jumppad.wav", qfalse);
	cgs.media.landSound = trap_S_RegisterSound("sound/player/land1.wav", qfalse);
	cgs.media.respawnSound = trap_S_RegisterSound("sound/items/respawn1.wav", qfalse);
	cgs.media.selectSound = trap_S_RegisterSound("sound/weapons/change.wav", qfalse);
	cgs.media.splinterBounce = trap_S_RegisterSound("sound/crates/splinterbounce1", qfalse);
	cgs.media.talkSound = trap_S_RegisterSound("sound/player/talk.wav", qfalse);
	cgs.media.teleInSound = trap_S_RegisterSound("sound/world/telein.wav", qfalse);
	cgs.media.teleOutSound = trap_S_RegisterSound("sound/world/teleout.wav", qfalse);
	cgs.media.watrInSound = trap_S_RegisterSound("sound/player/watr_in.wav", qfalse);
	cgs.media.watrOutSound = trap_S_RegisterSound("sound/player/watr_out.wav", qfalse);
	cgs.media.watrUnSound = trap_S_RegisterSound("sound/player/watr_un.wav", qfalse);
	cgs.media.sfx_boltexp = trap_S_RegisterSound("sound/weapons/crossbow/bolthitwall", qfalse);

	for(i = 0; i<4; i++){
		Com_sprintf(name, sizeof(name), "sound/player/footsteps/step%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_NORMAL][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/boot%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_BOOT][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/flesh%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_FLESH][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/mech%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_MECH][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/energy%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_ENERGY][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/splash%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_SPLASH][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/clank%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_METAL][i] = trap_S_RegisterSound(name, qfalse);
	}

	// only register the items that the server says we need
	Q_strncpyz(items, getconfigstr(CS_ITEMS), sizeof(items));

	for(i = 1; i < bg_nitems; i++)
//		if( items[i] == '1' || cg_buildScript.integer ){
		regitemsounds(i);
//		}

	for(i = 1; i < MAX_SOUNDS; i++){
		soundName = getconfigstr(CS_SOUNDS+i);
		if(!soundName[0])
			break;
		if(soundName[0] == '*')
			continue;	// custom sound
		cgs.gamesounds[i] = trap_S_RegisterSound(soundName, qfalse);
	}
}

/*
This function may execute for a couple of minutes with a slow disk.
*/
static void
reggraphics(void)
{
	int i;
	char items[MAX_ITEMS+1];
	static char *sb_nums[11] = {
		"gfx/2d/numbers/zero_32b",
		"gfx/2d/numbers/one_32b",
		"gfx/2d/numbers/two_32b",
		"gfx/2d/numbers/three_32b",
		"gfx/2d/numbers/four_32b",
		"gfx/2d/numbers/five_32b",
		"gfx/2d/numbers/six_32b",
		"gfx/2d/numbers/seven_32b",
		"gfx/2d/numbers/eight_32b",
		"gfx/2d/numbers/nine_32b",
		"gfx/2d/numbers/minus_32b",
	};

	// clear any references to old media
	memset(&cg.refdef, 0, sizeof(cg.refdef));
	trap_R_ClearScene();

	loadingstr(cgs.mapname);

	trap_R_LoadWorldMap(cgs.mapname);

	// precache status bar pics
	loadingstr("game media");

	for(i = 0; i<11; i++)
		cgs.media.numberShaders[i] = trap_R_RegisterShader(sb_nums[i]);

	cgs.media.viewBloodShader = trap_R_RegisterShader("viewBloodBlend");

	cgs.media.deferShader = trap_R_RegisterShaderNoMip("gfx/2d/defer.tga");

	cgs.media.smokePuffShader = trap_R_RegisterShader("smokePuff");

	cgs.media.lagometerShader = trap_R_RegisterShader("lagometer");
	cgs.media.connectionShader = trap_R_RegisterShader("disconnected");

	cgs.media.waterBubbleShader = trap_R_RegisterShader("waterBubble");

	cgs.media.selectShader = trap_R_RegisterShader("gfx/2d/select");

	cgs.media.backTileShader = trap_R_RegisterShader("gfx/2d/backtile");

	// powerup shaders
	cgs.media.hastePuffShader = trap_R_RegisterShader("hasteSmokePuff");

	cgs.media.lifeModel = trap_R_RegisterModel("models/items/life");
	cgs.media.tokenModel = trap_R_RegisterModel("models/items/token");

	cgs.media.smoke2 = trap_R_RegisterModel("models/weapons2/shells/s_shell.md3");
	cgs.media.splinter = trap_R_RegisterModel("models/crates/splinter");

	cgs.media.balloonShader = trap_R_RegisterShader("sprites/balloon3");

	cgs.media.teleportEffectModel = trap_R_RegisterModel("models/misc/telep.md3");
	cgs.media.teleportEffectShader = trap_R_RegisterShader("teleportEffect");

	cgs.media.gib = trap_R_RegisterModel("models/players/gib");

	// weapon explosions/flashes/bolts
	cgs.media.boltModel = trap_R_RegisterModel("models/weapons2/crossbow/bolt");

	// precache checkpoint halo for crate smashes
	trap_R_RegisterModel("models/mapobjects/ckpoint/ckpoint");

	memset(cg_items, 0, sizeof(cg_items));
	memset(cg_weapons, 0, sizeof(cg_weapons));

	// only register the items that the server says we need
	Q_strncpyz(items, getconfigstr(CS_ITEMS), sizeof(items));

	for(i = 1; i < bg_nitems; i++)
		if(items[i] == '1' || cg_buildScript.integer){
			loadingitem(i);
			registeritemgfx(i);
		}

	// wall marks
	cgs.media.shadowMarkShader = trap_R_RegisterShader("markShadow");
	cgs.media.wakeMarkShader = trap_R_RegisterShader("wake");

	// register the inline models
	cgs.ninlinemodels = trap_CM_NumInlineModels();
	for(i = 1; i < cgs.ninlinemodels; i++){
		char name[10];
		vec3_t mins, maxs;
		int j;

		Com_sprintf(name, sizeof(name), "*%i", i);
		cgs.inlinedrawmodel[i] = trap_R_RegisterModel(name);
		trap_R_ModelBounds(cgs.inlinedrawmodel[i], mins, maxs);
		for(j = 0; j < 3; j++)
			cgs.inlinemodelmidpoints[i][j] = mins[j] + 0.5 * (maxs[j] - mins[j]);
	}

	// register all the server specified models
	for(i = 1; i<MAX_MODELS; i++){
		const char *modelname;
		char buf[MAX_QPATH], buf2[MAX_QPATH], *p;

		modelname = getconfigstr(CS_MODELS+i);
		if(!modelname[0])
			break;
		cgs.gamemodels[i] = trap_R_RegisterModel(modelname);

		// load the animation.cfg if it exists

		Q_strncpyz(buf2, modelname, sizeof buf2);	// discard const
		p = strrchr(buf2, '/');
		*p = '\0';
		Com_sprintf(buf, sizeof buf, "%s/animation.cfg", buf2);
		parseanimfile(buf, cgs.anims[i]);
	}
}

void
mkspecstr(void)
{
	int i;
	cg.speclist[0] = 0;
	for(i = 0; i < MAX_CLIENTS; i++)
		if(cgs.clientinfo[i].infovalid && cgs.clientinfo[i].team == TEAM_SPECTATOR)
			Q_strcat(cg.speclist, sizeof(cg.speclist), va("%s     ", cgs.clientinfo[i].name));
	i = strlen(cg.speclist);
	if(i != cg.speclen){
		cg.speclen = i;
		cg.specwidth = -1;
	}
}

static void
regclients(void)
{
	int i;

	loadingclient(cg.clientNum);
	newclientinfo(cg.clientNum);

	for(i = 0; i<MAX_CLIENTS; i++){
		const char *clientInfo;

		if(cg.clientNum == i)
			continue;

		clientInfo = getconfigstr(CS_PLAYERS+i);
		if(!clientInfo[0])
			continue;
		loadingclient(i);
		newclientinfo(i);
	}
	mkspecstr();
}

const char *
getconfigstr(int index)
{
	if(index < 0 || index >= MAX_CONFIGSTRINGS)
		cgerrorf("getconfigstr: bad index: %i", index);
	return cgs.gameState.stringData + cgs.gameState.stringOffsets[index];
}

void
startmusic(void)
{
	char *s;
	char parm1[MAX_QPATH], parm2[MAX_QPATH];

	// start the background music
	s = (char*)getconfigstr(CS_MUSIC);
	Q_strncpyz(parm1, COM_Parse(&s), sizeof(parm1));
	Q_strncpyz(parm2, COM_Parse(&s), sizeof(parm2));

	trap_S_StartBackgroundTrack(parm1, parm2);
}

/*
Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
*/
void
cginit(int serverMessageNum, int serverCommandSequence, int clientNum)
{
	const char *s;

	// clear everything
	memset(&cgs, 0, sizeof(cgs));
	memset(&cg, 0, sizeof(cg));
	memset(cg_entities, 0, sizeof(cg_entities));
	memset(cg_weapons, 0, sizeof(cg_weapons));
	memset(cg_items, 0, sizeof(cg_items));

	cg.clientNum = clientNum;

	cgs.nprocessedsnaps = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	// load a few needed things before we do any screen updates
	cgs.media.charsetShader = trap_R_RegisterShader("gfx/2d/bigchars");
	cgs.media.whiteShader = trap_R_RegisterShader("white");
	cgs.media.charsetProp = trap_R_RegisterShaderNoMip("menu/art/font1_prop.tga");
	cgs.media.charsetPropGlow = trap_R_RegisterShaderNoMip("menu/art/font1_prop_glo.tga");
	cgs.media.charsetPropB = trap_R_RegisterShaderNoMip("menu/art/font2_prop.tga");

	registercvars();

	initconsolesmds();

	cg.weapsel = WP_MACHINEGUN;

	cgs.redflag = cgs.blueflag = -1;// For compatibily, default to unset for
	cgs.flagStatus = -1;
	// old servers

	// get the rendering configuration from the client system
	trap_GetGlconfig(&cgs.glconfig);
	cgs.scrnxscale = cgs.glconfig.vidWidth / 640.0;
	cgs.scrnyscale = cgs.glconfig.vidHeight / 480.0;

	// get the gamestate from the client system
	trap_GetGameState(&cgs.gameState);

	// check version
	s = getconfigstr(CS_GAME_VERSION);
	if(strcmp(s, GAME_VERSION))
		cgerrorf("Client/Server game mismatch: %s/%s", GAME_VERSION, s);

	s = getconfigstr(CS_LEVEL_START_TIME);
	cgs.levelStartTime = atoi(s);

	parsesrvinfo();

	// load the new map
	loadingstr("collision map");

	trap_CM_LoadMap(cgs.mapname);


	cg.loading = qtrue;	// force players to load instead of defer

	loadingstr("sounds");

	regsounds();

	loadingstr("graphics");

	reggraphics();

	loadingstr("clients");

	regclients();	// if low on memory, some clients will be deferred


	cg.loading = qfalse;	// future players will be deferred

	initlocalents();

	initmarkpolys();

	// remove the last loading update
	cg.infoscreentext[0] = 0;

	// Make sure we have update values (scores)
	setconfigvals();

	startmusic();

	loadingstr("");


	shaderstatechanged();

	trap_S_ClearLoopingSounds(qtrue);
}

/*
Called before every level change or subsystem restart
*/
void
cgshutdown(void)
{
	// some mods may need to do cleanup work here,
	// like closing files or archiving session data
}

/*
 type 0 - no event handling
      1 - team menu
      2 - hud editor
*/
void
eventhandling(int type)
{
}

void
keyevent(int key, qboolean down)
{
}

void
mouseevent(int x, int y)
{
}
