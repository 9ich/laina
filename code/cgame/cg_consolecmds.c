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
// cg_consolecmds.c -- text commands typed in at the local console, or
// executed by a key binding

#include "cg_local.h"

void
targetcommand_f(void)
{
	int targetNum;
	char test[4];

	targetNum = xhairplayer();
	if(targetNum == -1)
		return;

	trap_Argv(1, test, 4);
	trap_SendClientCommand(va("gc %i %i", targetNum, atoi(test)));
}

/*
=================
sizeup_f

Keybinding command
=================
*/
static void
sizeup_f(void)
{
	trap_Cvar_Set("cg_viewsize", va("%i", (int)(cg_viewsize.integer+10)));
}

/*
=================
sizedown_f

Keybinding command
=================
*/
static void
sizedown_f(void)
{
	trap_Cvar_Set("cg_viewsize", va("%i", (int)(cg_viewsize.integer-10)));
}

/*
=============
viewpos_f

Debugging command to print the current position
=============
*/
static void
viewpos_f(void)
{
	cgprintf("(%i %i %i) : %i\n", (int)cg.refdef.vieworg[0],
		  (int)cg.refdef.vieworg[1], (int)cg.refdef.vieworg[2],
		  (int)cg.refdefviewangles[YAW]);
}

static void
scoresdown_f(void)
{
	if(cg.scoresreqtime + 2000 < cg.time){
		// the scores are more than two seconds out of data,
		// so request new ones
		cg.scoresreqtime = cg.time;
		trap_SendClientCommand("score");

		// leave the current scores up if they were already
		// displayed, but if this is the first hit, clear them out
		if(!cg.showscores){
			cg.showscores = qtrue;
			cg.nscores = 0;
		}
	}else
		// show the cached contents even if they just pressed if it
		// is within two seconds
		cg.showscores = qtrue;
}

static void
scoresup_f(void)
{
	if(cg.showscores){
		cg.showscores = qfalse;
		cg.scorefadetime = cg.time;
	}
}


static void
telltarget_f(void)
{
	int clientNum;
	char command[128];
	char message[128];

	clientNum = xhairplayer();
	if(clientNum == -1)
		return;

	trap_Args(message, 128);
	Com_sprintf(command, 128, "tell %i %s", clientNum, message);
	trap_SendClientCommand(command);
}

static void
tellattacker_f(void)
{
	int clientNum;
	char command[128];
	char message[128];

	clientNum = getlastattacker();
	if(clientNum == -1)
		return;

	trap_Args(message, 128);
	Com_sprintf(command, 128, "tell %i %s", clientNum, message);
	trap_SendClientCommand(command);
}


/*
==================
startorbit_f
==================
*/

static void
startorbit_f(void)
{
	char var[MAX_TOKEN_CHARS];

	trap_Cvar_VariableStringBuffer("developer", var, sizeof(var));
	if(!atoi(var))
		return;
	if(cg_cameraOrbit.value != 0){
		trap_Cvar_Set("cg_cameraOrbit", "0");
		trap_Cvar_Set("cg_thirdPerson", "0");
	}else{
		trap_Cvar_Set("cg_cameraOrbit", "5");
		trap_Cvar_Set("cg_thirdPerson", "1");
		trap_Cvar_Set("cg_thirdPersonAngle", "0");
		trap_Cvar_Set("cg_thirdPersonRange", "100");
	}
}

/*
static void camera_f( void ){
        char name[1024];
        trap_Argv( 1, name, sizeof(name));
        if(trap_loadCamera(name)){
                cg.cameramode = qtrue;
                trap_startCamera(cg.time);
        }else{
                cgprintf ("Unable to load camera %s\n",name);
        }
}
*/

typedef struct
{
	char *cmd;
	void (*function)(void);
} consoleCommand_t;

static consoleCommand_t commands[] = {
	{"testgun", testgun_f},
	{"testmodel", testmodel_f},
	{"nextframe", testmodelnextframe_f},
	{"prevframe", testmodelprevframe_f},
	{"nextskin", testmodelnextskin_f},
	{"prevskin", testmodelprevskin_f},
	{"viewpos", viewpos_f},
	{"+scores", scoresdown_f},
	{"-scores", scoresup_f},
	{"+zoom", zoomdown_f},
	{"-zoom", zoomup_f},
	{"sizeup", sizeup_f},
	{"sizedown", sizedown_f},
	{"weapnext", nextweapon_f},
	{"weapprev", prevweapon_f},
	{"weapon", weapon_f},
	{"tcmd", targetcommand_f},
	{"tell_target", telltarget_f},
	{"tell_attacker", tellattacker_f},
	{"startOrbit", startorbit_f},
	//{ "camera", camera_f },
	{"loaddeferred", loaddeferred}
};

/*
=================
consolecmd

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
qboolean
consolecmd(void)
{
	const char *cmd;
	int i;

	cmd = cgargv(0);

	for(i = 0; i < ARRAY_LEN(commands); i++)
		if(!Q_stricmp(cmd, commands[i].cmd)){
			commands[i].function();
			return qtrue;
		}

	return qfalse;
}

/*
=================
initconsolesmds

Let the client system know about all of our commands
so it can perform tab completion
=================
*/
void
initconsolesmds(void)
{
	int i;

	for(i = 0; i < ARRAY_LEN(commands); i++)
		trap_AddCommand(commands[i].cmd);

	// the game server will interpret these commands, which will be automatically
	// forwarded to the server after they are not recognized locally
	trap_AddCommand("kill");
	trap_AddCommand("say");
	trap_AddCommand("say_team");
	trap_AddCommand("tell");
	trap_AddCommand("give");
	trap_AddCommand("god");
	trap_AddCommand("notarget");
	trap_AddCommand("noclip");
	trap_AddCommand("where");
	trap_AddCommand("team");
	trap_AddCommand("follow");
	trap_AddCommand("follownext");
	trap_AddCommand("followprev");
	trap_AddCommand("levelshot");
	trap_AddCommand("addbot");
	trap_AddCommand("setviewpos");
	trap_AddCommand("callvote");
	trap_AddCommand("vote");
	trap_AddCommand("callteamvote");
	trap_AddCommand("teamvote");
	trap_AddCommand("stats");
	trap_AddCommand("teamtask");
	trap_AddCommand("loaddefered");	// spelled wrong, but not changing for demo
}
