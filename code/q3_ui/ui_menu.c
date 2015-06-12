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

/*
Main menu
*/

#include "ui_local.h"

enum {
	ID_SINGLEPLAYER		= 10,
	ID_MULTIPLAYER			= 11,
	ID_SETUP				= 12,
	ID_TEAMARENA		= 13,
	ID_MODS					= 14,
	ID_EXIT					= 15
};

#define MAIN_BACKGROUND_SHADER			"menu/art/back"
#define MAIN_BANNER_SHADER				"menu/art/banner"
#define MAIN_MENU_VERTICAL_SPACING		34

typedef struct {
	menuframework_s	menu;
	menutext_s		singleplayer;
	menutext_s		multiplayer;
	menutext_s		setup;
	menutext_s		teamArena;
	menutext_s		mods;
	menutext_s		exit;
	qhandle_t		bannerShader;
} mainmenu_t;

typedef struct {
	menuframework_s menu;
	char errorMessage[4096];
} errorMessage_t;

static mainmenu_t s_main;
static errorMessage_t s_errorMessage;

static void MainMenu_ExitAction(qboolean result)
{
	if(!result)
		return;
	UI_PopMenu();
	trap_Cmd_ExecuteText(EXEC_APPEND, "quit\n");
}

void Main_MenuEvent(void *ptr, int event)
{
	if(event != QM_ACTIVATED)
		return;
	switch(((menucommon_s *)ptr)->id){
	case ID_SINGLEPLAYER:
		UI_SPLevelMenu();
		break;
	case ID_MULTIPLAYER:
		UI_ArenaServersMenu();
		break;
	case ID_SETUP:
		UI_SetupMenu();
		break;
	case ID_MODS:
		UI_ModsMenu();
		break;
	case ID_TEAMARENA:
		trap_Cvar_Set("fs_game", BASETA);
		trap_Cmd_ExecuteText(EXEC_APPEND, "vid_restart;");
		break;
	case ID_EXIT:
		UI_ConfirmMenu("EXIT GAME?", 0, MainMenu_ExitAction);
		break;
	}
}

void MainMenu_Cache(void)
{
}

sfxHandle_t ErrorMessage_Key(int key)
{
	trap_Cvar_Set("com_errorMessage", "");
	UI_MainMenu();
	return (menu_null_sound);
}

/*
TTimo: this function is common to the main menu and errorMessage menu
*/
static void Main_MenuDraw(void)
{
	trap_R_ClearScene();
	UI_DrawNamedPic(0, 0, 640, 480, MAIN_BACKGROUND_SHADER);
	UI_DrawNamedPic(180, 0, 280, 140, MAIN_BANNER_SHADER);

	if(strlen(s_errorMessage.errorMessage)){
		UI_DrawProportionalString_AutoWrapped(320, 192, 600, 20, s_errorMessage.errorMessage, UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, menu_text_color);
	}else{
		// standard menu drawing
		Menu_Draw(&s_main.menu);
	}
}

static qboolean UI_TeamArenaExists(void)
{
	int		numdirs;
	char	dirlist[2048];
	char	*dirptr;
	char  *descptr;
	int		i;
	int		dirlen;

	numdirs = trap_FS_GetFileList("$modlist", "", dirlist, sizeof(dirlist));
	dirptr  = dirlist;
	for(i = 0; i < numdirs; i++){
		dirlen = strlen(dirptr) + 1;
		descptr = dirptr + dirlen;
		if(Q_stricmp(dirptr, BASETA) == 0){
			return qtrue;
		}
		dirptr += dirlen + strlen(descptr) + 1;
	}
	return qfalse;
}

#define MENU_ITEMS_Y 134
static int menuY = MENU_ITEMS_Y;

static menutext_s DupMenutext(menutext_s m)
{
	menutext_s n;

	memcpy(&n, &m, sizeof m);
	menuY += MAIN_MENU_VERTICAL_SPACING;
	n.generic.y = menuY;
	return n;
}

/*
The main menu only comes up when not in a game,
so make sure that the attract loop server is down
and that local cinematics are killed
*/
void UI_MainMenu(void)
{
	qboolean teamArena = qfalse;
	int style = UI_CENTER | UI_DROPSHADOW;

	trap_Cvar_Set("sv_killserver", "1");

	if(!uis.demoversion && !ui_cdkeychecked.integer){
		char	key[17];

		trap_GetCDKey(key, sizeof(key));
		if(trap_VerifyCDKey(key, NULL) == qfalse){
			UI_CDKeyMenu();
			return;
		}
	}

	memset(&s_main, 0 ,sizeof(mainmenu_t));
	memset(&s_errorMessage, 0 ,sizeof(errorMessage_t));
	MainMenu_Cache();	// com_errorMessage would need that too

	trap_Cvar_VariableStringBuffer("com_errorMessage", s_errorMessage.errorMessage, sizeof(s_errorMessage.errorMessage));
	if(strlen(s_errorMessage.errorMessage)){
		s_errorMessage.menu.draw = Main_MenuDraw;
		s_errorMessage.menu.key = ErrorMessage_Key;
		s_errorMessage.menu.fullscreen = qtrue;
		s_errorMessage.menu.wrapAround = qtrue;
		s_errorMessage.menu.showlogo = qtrue;

		trap_Key_SetCatcher(KEYCATCH_UI);
		uis.menusp = 0;
		UI_PushMenu(&s_errorMessage.menu);
		return;
	}

	s_main.menu.draw = Main_MenuDraw;
	s_main.menu.fullscreen = qtrue;
	s_main.menu.wrapAround = qtrue;
	s_main.menu.showlogo = qtrue;

	s_main.singleplayer.generic.type		= MTYPE_PTEXT;
	s_main.singleplayer.generic.flags		= QMF_CENTER_JUSTIFY|QMF_HIGHLIGHT_IF_FOCUS;
	s_main.singleplayer.generic.x			= 320;
	s_main.singleplayer.generic.y			= MENU_ITEMS_Y;
	s_main.singleplayer.generic.id			= ID_SINGLEPLAYER;
	s_main.singleplayer.generic.callback	= Main_MenuEvent;
	s_main.singleplayer.string				= "SINGLE PLAYER";
	s_main.singleplayer.color				= menu_text_color;
	s_main.singleplayer.focuscolor			= menu_highlight_color;
	s_main.singleplayer.style				= style;

	s_main.multiplayer = DupMenutext(s_main.singleplayer);
	s_main.multiplayer.generic.id			= ID_MULTIPLAYER;
	s_main.multiplayer.generic.callback		= Main_MenuEvent;
	s_main.multiplayer.string				= "MULTIPLAYER";

	s_main.setup = DupMenutext(s_main.singleplayer);
	s_main.setup.generic.id					= ID_SETUP;
	s_main.setup.generic.callback			= Main_MenuEvent;
	s_main.setup.string						= "SETUP";

	if(!uis.demoversion && UI_TeamArenaExists()){
		teamArena = qtrue;
		s_main.teamArena = DupMenutext(s_main.singleplayer);
		s_main.teamArena.generic.id				= ID_TEAMARENA;
		s_main.teamArena.generic.callback		= Main_MenuEvent;
		s_main.teamArena.string					= "TEAM ARENA";
	}

	if(!uis.demoversion){
		s_main.mods = DupMenutext(s_main.singleplayer);
		s_main.mods.generic.id				= ID_MODS;
		s_main.mods.generic.callback		= Main_MenuEvent;
		s_main.mods.string					= "MODS";
	}

	s_main.exit = DupMenutext(s_main.singleplayer);
	s_main.exit.generic.id					= ID_EXIT;
	s_main.exit.generic.callback			= Main_MenuEvent;
	s_main.exit.string						= "EXIT";

	Menu_AddItem(&s_main.menu,	&s_main.singleplayer);
	Menu_AddItem(&s_main.menu,	&s_main.multiplayer);
	Menu_AddItem(&s_main.menu,	&s_main.setup);
	if(teamArena){
		Menu_AddItem(&s_main.menu,	&s_main.teamArena);
	}
	if(!uis.demoversion){
		Menu_AddItem(&s_main.menu,	&s_main.mods);
	}
	Menu_AddItem(&s_main.menu,	&s_main.exit);

	trap_Key_SetCatcher(KEYCATCH_UI);
	uis.menusp = 0;
	UI_PushMenu(&s_main.menu);
	menuY = MENU_ITEMS_Y;
}
