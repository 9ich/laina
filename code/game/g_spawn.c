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

#include "g_local.h"

qboolean
spawnstr(const char *key, const char *defaultString, char **out)
{
	int i;

	if(!level.spawning)
		*out = (char*)defaultString;
//		errorf( "spawnstr() called while not spawning" );

	for(i = 0; i < level.nspawnvars; i++)
		if(!Q_stricmp(key, level.spawnvars[i][0])){
			*out = level.spawnvars[i][1];
			return qtrue;
		}

	*out = (char*)defaultString;
	return qfalse;
}

qboolean
spawnfloat(const char *key, const char *defaultString, float *out)
{
	char *s;
	qboolean present;

	present = spawnstr(key, defaultString, &s);
	*out = atof(s);
	return present;
}

qboolean
spawnint(const char *key, const char *defaultString, int *out)
{
	char *s;
	qboolean present;

	present = spawnstr(key, defaultString, &s);
	*out = atoi(s);
	return present;
}

qboolean
spawnvec(const char *key, const char *defaultString, float *out)
{
	char *s;
	qboolean present;

	present = spawnstr(key, defaultString, &s);
	sscanf(s, "%f %f %f", &out[0], &out[1], &out[2]);
	return present;
}

// fields are needed for spawning from the entity string
typedef enum
{
	F_INT,
	F_FLOAT,
	F_STRING,
	F_VECTOR,
	F_ANGLEHACK
} fieldtype_t;

typedef struct
{
	char		*name;
	size_t		ofs;
	fieldtype_t	type;
} field_t;

field_t fields[] = {
	{"classname", FOFS(classname), F_STRING},
	{"origin", FOFS(s.origin), F_VECTOR},
	{"model", FOFS(model), F_STRING},
	{"model2", FOFS(model2), F_STRING},
	{"spawnflags", FOFS(spawnflags), F_INT},
	{"speed", FOFS(speed), F_FLOAT},
	{"target", FOFS(target), F_STRING},
	{"targetname", FOFS(targetname), F_STRING},
	{"message", FOFS(message), F_STRING},
	{"team", FOFS(team), F_STRING},
	{"wait", FOFS(wait), F_FLOAT},
	{"random", FOFS(random), F_FLOAT},
	{"count", FOFS(count), F_INT},
	{"health", FOFS(health), F_INT},
	{"dmg", FOFS(damage), F_INT},
	{"angles", FOFS(s.angles), F_VECTOR},
	{"angle", FOFS(s.angles), F_ANGLEHACK},
	{"targetshadername", FOFS(targetshadername), F_STRING},
	{"newtargetshadername", FOFS(newtargetshadername), F_STRING},

	{nil}
};

typedef struct
{
	char *name;
	void (*spawn)(ent_t *ent);
} spawn_t;

void	SP_playerspawn(ent_t *ent);

void	SP_func_plat(ent_t *ent);
void	SP_func_static(ent_t *ent);
void	SP_func_rotating(ent_t *ent);
void	SP_func_bobbing(ent_t *ent);
void	SP_func_pendulum(ent_t *ent);
void	SP_func_button(ent_t *ent);
void	SP_func_door(ent_t *ent);
void	SP_func_piston(ent_t *ent);
void	SP_func_train(ent_t *ent);

void	SP_trigger_always(ent_t *ent);
void	SP_trigger_timer(ent_t *self);
void	SP_trigger_multiple(ent_t *ent);
void	SP_trigger_push(ent_t *ent);
void	SP_trigger_teleport(ent_t *ent);
void	SP_trigger_hurt(ent_t *ent);

void	SP_target_remove_powerups(ent_t *ent);
void	SP_target_give(ent_t *ent);
void	SP_target_delay(ent_t *ent);
void	SP_target_speaker(ent_t *ent);
void	SP_target_print(ent_t *ent);
void	SP_target_laser(ent_t *self);
void	SP_target_score(ent_t *ent);
void	SP_target_teleporter(ent_t *ent);
void	SP_target_relay(ent_t *ent);
void	SP_target_kill(ent_t *ent);
void	SP_target_position(ent_t *ent);
void	SP_target_location(ent_t *ent);
void	SP_target_push(ent_t *ent);
void	SP_target_secret(ent_t *ent);
void	SP_target_changemap(ent_t *ent);

void	SP_light(ent_t *self);
void	SP_info_null(ent_t *self);
void	SP_info_notnull(ent_t *self);
void	SP_info_player_intermission(ent_t *self);
void	SP_info_camp(ent_t *self);
void	SP_path_corner(ent_t *self);

void	SP_misc_teleporter_dest(ent_t *self);
void	SP_misc_model(ent_t *ent);
void	SP_misc_portal_camera(ent_t *ent);
void	SP_misc_portal_surface(ent_t *ent);

void	SP_shooter_rocket(ent_t *ent);
void	SP_shooter_plasma(ent_t *ent);
void	SP_shooter_grenade(ent_t *ent);

void	SP_crate(ent_t *ent);
void	SP_crate_strong(ent_t *ent);
void	SP_crate_checkpoint(ent_t *ent);
void	SP_crate_bouncy(ent_t *ent);

void	SP_team_CTF_redplayer(ent_t *ent);
void	SP_team_CTF_blueplayer(ent_t *ent);

void	SP_team_CTF_redspawn(ent_t *ent);
void	SP_team_CTF_bluespawn(ent_t *ent);

void
SP_item_botroam(ent_t *ent){ }

spawn_t spawns[] = {
	{"playerspawn", SP_playerspawn},

	{"crate_", SP_crate},	// underscore for radiant menu
	{"crate_checkpoint", SP_crate_checkpoint},
	{"crate_bouncy", SP_crate_bouncy},

	// info entities don't do anything at all, but provide positional
	// information for things controlled by other processes
	{"info_null", SP_info_null},
	{"info_notnull", SP_info_notnull},	// use target_position instead
	{"info_player_intermission", SP_info_player_intermission},
	{"info_camp", SP_info_camp},

	{"func_plat", SP_func_plat},
	{"func_button", SP_func_button},
	{"func_door", SP_func_door},
	{"func_piston", SP_func_piston},
	{"func_static", SP_func_static},
	{"func_rotating", SP_func_rotating},
	{"func_bobbing", SP_func_bobbing},
	{"func_pendulum", SP_func_pendulum},
	{"func_train", SP_func_train},
	{"func_group", SP_info_null},

	// Triggers, with the exception of trigger_timer, are brush
	// objects that cause an effect when contacted by a living
	// player, usually involving firing targets.
	//
	// While almost everything could be done with
	// a single trigger class and different targets, triggered effects
	// could not be client side predicted (push and teleport).
	{"trigger_always", SP_trigger_always},
	{"trigger_hurt", SP_trigger_hurt},
	{"trigger_multiple", SP_trigger_multiple},
	{"trigger_push", SP_trigger_push},
	{"trigger_teleport", SP_trigger_teleport},
	{"trigger_timer", SP_trigger_timer},

	// targets perform no action by themselves, but must be triggered
	// by another entity
	{"target_give", SP_target_give},
	{"target_remove_powerups", SP_target_remove_powerups},
	{"target_delay", SP_target_delay},
	{"target_speaker", SP_target_speaker},
	{"target_print", SP_target_print},
	{"target_laser", SP_target_laser},
	{"target_score", SP_target_score},
	{"target_teleporter", SP_target_teleporter},
	{"target_relay", SP_target_relay},
	{"target_kill", SP_target_kill},
	{"target_position", SP_target_position},
	{"target_location", SP_target_location},
	{"target_push", SP_target_push},
	{"target_secret", SP_target_secret},
	{"target_changemap", SP_target_changemap},

	{"light", SP_light},
	{"path_corner", SP_path_corner},

	{"misc_teleporter_dest", SP_misc_teleporter_dest},
	{"misc_model", SP_misc_model},
	{"misc_portal_surface", SP_misc_portal_surface},
	{"misc_portal_camera", SP_misc_portal_camera},

	{"shooter_rocket", SP_shooter_rocket},
	{"shooter_grenade", SP_shooter_grenade},
	{"shooter_plasma", SP_shooter_plasma},

	{"team_CTF_redplayer", SP_team_CTF_redplayer},
	{"team_CTF_blueplayer", SP_team_CTF_blueplayer},

	{"team_CTF_redspawn", SP_team_CTF_redspawn},
	{"team_CTF_bluespawn", SP_team_CTF_bluespawn},

	{"item_botroam", SP_item_botroam},

	{nil, 0}
};

/*
===============
callspawn

Finds the spawn function for the entity and calls it,
returning qfalse if not found
===============
*/
qboolean
callspawn(ent_t *ent)
{
	spawn_t *s;
	item_t *item;

	if(!ent->classname){
		gprintf("callspawn: nil classname\n");
		return qfalse;
	}

	// check item spawn functions
	for(item = bg_itemlist+1; item->classname; item++)
		if(!strcmp(item->classname, ent->classname)){
			itemspawn(ent, item);
			return qtrue;
		}

	// check normal spawn functions
	for(s = spawns; s->name; s++)
		if(!strcmp(s->name, ent->classname)){
			// found it
			s->spawn(ent);
			return qtrue;
		}
	gprintf("%s doesn't have a spawn function\n", ent->classname);
	return qfalse;
}

/*
=============
newstr

Builds a copy of the string, translating \n to real linefeeds
so message texts can be multi-line
=============
*/
char *
newstr(const char *string)
{
	char *newb, *new_p;
	int i, l;

	l = strlen(string) + 1;

	newb = alloc(l);

	new_p = newb;

	// turn \n into a real linefeed
	for(i = 0; i< l; i++){
		if(string[i] == '\\' && i < l-1){
			i++;
			if(string[i] == 'n')
				*new_p++ = '\n';
			else
				*new_p++ = '\\';
		}else
			*new_p++ = string[i];
	}

	return newb;
}

/*
===============
parsefield

Takes a key/value pair and sets the binary values
in a gentity
===============
*/
void
parsefield(const char *key, const char *value, ent_t *ent)
{
	field_t *f;
	byte *b;
	float v;
	vec3_t vec;

	for(f = fields; f->name; f++)
		if(!Q_stricmp(f->name, key)){
			// found it
			b = (byte*)ent;

			switch(f->type){
			case F_STRING:
				*(char**)(b+f->ofs) = newstr(value);
				break;
			case F_VECTOR:
				sscanf(value, "%f %f %f", &vec[0], &vec[1], &vec[2]);
				((float*)(b+f->ofs))[0] = vec[0];
				((float*)(b+f->ofs))[1] = vec[1];
				((float*)(b+f->ofs))[2] = vec[2];
				break;
			case F_INT:
				*(int*)(b+f->ofs) = atoi(value);
				break;
			case F_FLOAT:
				*(float*)(b+f->ofs) = atof(value);
				break;
			case F_ANGLEHACK:
				v = atof(value);
				((float*)(b+f->ofs))[0] = 0;
				((float*)(b+f->ofs))[1] = v;
				((float*)(b+f->ofs))[2] = 0;
				break;
			}
			return;
		}
}

#define ADJUST_AREAPORTAL() \
	if(ent->s.eType == ET_MOVER) \
	{ \
		trap_LinkEntity(ent); \
		trap_AdjustAreaPortalState(ent, qtrue); \
	}

/*
===================
spawngentityfromspawnvars

Spawn an entity and fill in all of the level fields from
level.spawnvars[], then call the class specfic spawn function
===================
*/
void
spawngentityfromspawnvars(void)
{
	int i;
	ent_t *ent;
	char *s, *value, *gametypeName;
	static char *gametypeNames[] = {"ffa", "tournament", "single", "team", "ctf", "oneflag", "obelisk", "harvester"};

	// get the next free entity
	ent = entspawn();

	for(i = 0; i < level.nspawnvars; i++)
		parsefield(level.spawnvars[i][0], level.spawnvars[i][1], ent);

	// check for "notsingle" flag
	if(g_gametype.integer == GT_SINGLE_PLAYER){
		spawnint("notsingle", "0", &i);
		if(i){
			ADJUST_AREAPORTAL();
			entfree(ent);
			return;
		}
	}
	// check for "notteam" flag (GT_FFA, GT_TOURNAMENT, GT_SINGLE_PLAYER)
	if(g_gametype.integer >= GT_TEAM){
		spawnint("notteam", "0", &i);
		if(i){
			ADJUST_AREAPORTAL();
			entfree(ent);
			return;
		}
	}else{
		spawnint("notfree", "0", &i);
		if(i){
			ADJUST_AREAPORTAL();
			entfree(ent);
			return;
		}
	}

	spawnint("notq3a", "0", &i);
	if(i){
		ADJUST_AREAPORTAL();
		entfree(ent);
		return;
	}

	if(spawnstr("gametype", nil, &value))
		if(g_gametype.integer >= GT_FFA && g_gametype.integer < GT_MAX_GAME_TYPE){
			gametypeName = gametypeNames[g_gametype.integer];

			s = strstr(value, gametypeName);
			if(!s){
				ADJUST_AREAPORTAL();
				entfree(ent);
				return;
			}
		}

	// move editor origin to pos
	veccopy(ent->s.origin, ent->s.pos.trBase);
	veccopy(ent->s.origin, ent->r.currentOrigin);

	// if we didn't get a classname, don't bother spawning anything
	if(!callspawn(ent))
		entfree(ent);
}

/*
====================
addspawnvartoken
====================
*/
char *
addspawnvartoken(const char *string)
{
	int l;
	char *dest;

	l = strlen(string);
	if(level.nspawnvarchars + l + 1 > MAX_SPAWN_VARS_CHARS)
		errorf("addspawnvartoken: max_spawn_vars_chars");

	dest = level.spawnvarchars + level.nspawnvarchars;
	memcpy(dest, string, l+1);

	level.nspawnvarchars += l + 1;

	return dest;
}

/*
====================
parsespawnvars

Parses a brace bounded set of key / value pairs out of the
level's entity strings into level.spawnvars[]

This does not actually spawn an entity.
====================
*/
qboolean
parsespawnvars(void)
{
	char keyname[MAX_TOKEN_CHARS];
	char com_token[MAX_TOKEN_CHARS];

	level.nspawnvars = 0;
	level.nspawnvarchars = 0;

	// parse the opening brace
	if(!trap_GetEntityToken(com_token, sizeof(com_token)))
		// end of spawn string
		return qfalse;
	if(com_token[0] != '{')
		errorf("parsespawnvars: found %s when expecting {", com_token);

	// go through all the key / value pairs
	while(1){
		// parse key
		if(!trap_GetEntityToken(keyname, sizeof(keyname)))
			errorf("parsespawnvars: eof without closing brace");

		if(keyname[0] == '}')
			break;

		// parse value
		if(!trap_GetEntityToken(com_token, sizeof(com_token)))
			errorf("parsespawnvars: eof without closing brace");

		if(com_token[0] == '}')
			errorf("parsespawnvars: closing brace without data");
		if(level.nspawnvars == MAX_SPAWN_VARS)
			errorf("parsespawnvars: max_spawn_vars");
		level.spawnvars[level.nspawnvars][0] = addspawnvartoken(keyname);
		level.spawnvars[level.nspawnvars][1] = addspawnvartoken(com_token);
		level.nspawnvars++;
	}

	return qtrue;
}

/*QUAKED worldspawn (0 0 0) ?

Every map should have exactly one worldspawn.
"music"		music wav file
"gravity"	800 is default gravity
"message"	Text to print during connection process
*/
void
SP_worldspawn(void)
{
	char *s;

	spawnstr("classname", "", &s);
	if(Q_stricmp(s, "worldspawn"))
		errorf("SP_worldspawn: The first entity isn't 'worldspawn'");

	// make some data visible to connecting client
	trap_SetConfigstring(CS_GAME_VERSION, GAME_VERSION);

	trap_SetConfigstring(CS_LEVEL_START_TIME, va("%i", level.starttime));

	spawnstr("music", "", &s);
	trap_SetConfigstring(CS_MUSIC, s);

	spawnstr("message", "", &s);
	trap_SetConfigstring(CS_MESSAGE, s);		// map specific message

	trap_SetConfigstring(CS_MOTD, g_motd.string);	// message of the day

	spawnstr("gravity", "800", &s);
	trap_Cvar_Set("g_gravity", s);

	spawnstr("enableDust", "0", &s);
	trap_Cvar_Set("g_enableDust", s);

	spawnstr("enableBreath", "0", &s);
	trap_Cvar_Set("g_enableBreath", s);

	g_entities[ENTITYNUM_WORLD].s.number = ENTITYNUM_WORLD;
	g_entities[ENTITYNUM_WORLD].r.ownerNum = ENTITYNUM_NONE;
	g_entities[ENTITYNUM_WORLD].classname = "worldspawn";

	g_entities[ENTITYNUM_NONE].s.number = ENTITYNUM_NONE;
	g_entities[ENTITYNUM_NONE].r.ownerNum = ENTITYNUM_NONE;
	g_entities[ENTITYNUM_NONE].classname = "nothing";

	// see if we want a warmup time
	trap_SetConfigstring(CS_WARMUP, "");
	if(g_restarted.integer){
		trap_Cvar_Set("g_restarted", "0");
		level.warmuptime = 0;
	}else if(g_doWarmup.integer){	// Turn it on
		level.warmuptime = -1;
		trap_SetConfigstring(CS_WARMUP, va("%i", level.warmuptime));
		logprintf("Warmup:\n");
	}
}

/*
==============
spawnall

Parses textual entity definitions out of an entstring and spawns entities.
==============
*/
void
spawnall(void)
{
	// allow calls to spawn*()
	level.spawning = qtrue;
	level.nspawnvars = 0;

	// the worldspawn is not an actual entity, but it still
	// has a "spawn" function to perform any global setup
	// needed by a level (setting configstrings or cvars, etc)
	if(!parsespawnvars())
		errorf("SpawnEntities: no entities");
	SP_worldspawn();

	// parse ents
	while(parsespawnvars())
		spawngentityfromspawnvars();

	level.spawning = qfalse;	// any future calls to entspawn*() will be errors
}
