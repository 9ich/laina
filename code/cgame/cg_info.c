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
// cg_info.c -- display information while data is being loading

#include "cg_local.h"

/*
======================
loadingstr

======================
*/
void
loadingstr(const char *s)
{
	Q_strncpyz(cg.infoscreentext, s, sizeof(cg.infoscreentext));

	trap_UpdateScreen();
}

/*
===================
loadingitem
===================
*/
void
loadingitem(int itemNum)
{
	item_t *item;

	item = &bg_itemlist[itemNum];
	loadingstr(item->pickupname);
}

/*
===================
loadingclient
===================
*/
void
loadingclient(int clientNum)
{
	const char *info;
	char *skin;
	char personality[MAX_QPATH];

	info = getconfigstr(CS_PLAYERS + clientNum);
	Q_strncpyz(personality, Info_ValueForKey(info, "n"), sizeof(personality));
	Q_CleanStr(personality);
	loadingstr(personality);
}

/*
====================
drawinfo

Draw all the status / pacifier stuff during level loading
====================
*/
void
drawinfo(void)
{
	const char *s;
	const char *info;
	const char *sysInfo;
	int y;
	int value;
	qhandle_t background;
	char buf[1024];

	info = getconfigstr(CS_SERVERINFO);
	sysInfo = getconfigstr(CS_SYSTEMINFO);

	background = trap_R_RegisterShaderNoMip("menuback");
	drawpic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, background);

	y = 200;

	// the first 150 rows are reserved for the client connection
	// screen to write into
	if(cg.infoscreentext[0])
		drawpropstr(320, y, va("Loading %s", cg.infoscreentext),
		   UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite);
	else
		drawpropstr(320, y, "Awaiting snapshot",
		   UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite);

	// draw info string information

	y = 240;

	// don't print server lines if playing a local game
	trap_Cvar_VariableStringBuffer("sv_running", buf, sizeof(buf));
	if(!atoi(buf)){
		// server hostname
		Q_strncpyz(buf, Info_ValueForKey(info, "sv_hostname"), 1024);
		Q_CleanStr(buf);
		drawpropstr(320, y, buf,
		   UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite);
		y += PROP_HEIGHT;

		// pure server
		s = Info_ValueForKey(sysInfo, "sv_pure");
		if(s[0] == '1'){
			drawpropstr(320, y, "Pure Server",
			   UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite);
			y += PROP_HEIGHT;
		}

		// server-specific message of the day
		s = getconfigstr(CS_MOTD);
		if(s[0]){
			drawpropstr(320, y, s,
			   UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite);
			y += PROP_HEIGHT;
		}

		// some extra space after hostname and motd
		y += 10;
	}

	// map-specific message (long map name)
	s = getconfigstr(CS_MESSAGE);
	if(s[0]){
		drawpropstr(320, y, s,
		   UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite);
		y += PROP_HEIGHT;
	}

	// game type
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
	case GT_TOURNAMENT:
		s = "Tournament";
		break;
	case GT_TEAM:
		s = "Team Deathmatch";
		break;
	case GT_CTF:
		s = "Capture The Flag";
		break;
	default:
		s = "Unknown Gametype";
		break;
	}
	drawpropstr(320, y, s, UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW,
	   colorWhite);
	y += PROP_HEIGHT;

	value = atoi(Info_ValueForKey(info, "timelimit"));
	if(value && cgs.gametype != GT_COOP && cgs.gametype != GT_SINGLE_PLAYER){
		drawpropstr(320, y, va("timelimit %i", value),
		   UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite);
		y += PROP_HEIGHT;
	}

	if(cgs.gametype < GT_CTF && cgs.gametype != GT_COOP &&
	   cgs.gametype != GT_SINGLE_PLAYER){
		value = atoi(Info_ValueForKey(info, "fraglimit"));
		if(value){
			drawpropstr(320, y, va("fraglimit %i", value),
			   UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite);
			y += PROP_HEIGHT;
		}
	}

	if(cgs.gametype >= GT_CTF){
		value = atoi(Info_ValueForKey(info, "capturelimit"));
		if(value)
			drawpropstr(320, y, va("capturelimit %i", value),
			   UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite);
	}
}
