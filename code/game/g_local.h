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
// g_local.h -- local definitions for game module

#include "../qcommon/q_shared.h"
#include "bg_public.h"
#include "g_public.h"

//==================================================================

// the "gameversion" client command will print this plus compile date
#define GAMEVERSION			BASEGAME

#define BODY_QUEUE_SIZE			8

#define INFINITE			1000000

#define FRAMETIME			100	// msec
#define CARNAGE_REWARD_TIME		3000
#define REWARD_SPRITE_TIME		2000

#define INTERMISSION_DELAY_TIME		1000
#define SP_INTERMISSION_DELAY_TIME	5000

// ent_t->flags
#define FL_GODMODE			0x00000010
#define FL_NOTARGET			0x00000020
#define FL_TEAMSLAVE			0x00000400	// not the first on the team
#define FL_NO_KNOCKBACK			0x00000800
#define FL_DROPPED_ITEM			0x00001000
#define FL_NO_BOTS			0x00002000	// spawn point not for bot use
#define FL_NO_HUMANS			0x00004000	// spawn point just for bots
#define FL_FORCE_GESTURE		0x00008000	// force gesture on client

// movers are things like doors, plats, buttons, etc
typedef enum
{
	MOVER_POS1,
	MOVER_POS2,
	MOVER_1TO2,
	MOVER_2TO1
} moverstate_t;

// NPC AI flags
enum
{
	AI_STANDGROUND		= (1<<0),
	AI_SOUNDTARGET		= (1<<1),
	AI_LOSTSIGHT		= (1<<2),
	AI_PURSUITLASTSEEN	= (1<<3),
	AI_PURSUENEXT		= (1<<4),
	AI_PURSUETEMP		= (1<<5),
	AI_HOLDFRAME		= (1<<6),
	AI_GOODGUY		= (1<<7),
	AI_NOSTEP		= (1<<8),
	AI_COMBATPOINT		= (1<<9),
	AI_MEDIC		= (1<<10),
	AI_SWIM			= (1<<11),
	AI_FLY			= (1<<12),
	AI_PARTIALGROUND	= (1<<13),
	AI_TEMPSTANDGROUND	= (1<<14)	// why
};

// NPC attack state
enum
{
	AS_STRAIGHT,
	AS_SLIDING,
	AS_MELEE,
	AS_MISSILE
};

// NPC range
enum
{
	RANGE_MELEE,
	RANGE_NEAR,
	RANGE_MID,
	RANGE_FAR
};

#define SP_PODIUM_MODEL "models/mapobjects/podium/podium4.md3"

typedef struct npcframe_t	npcframe_t;
typedef struct npcmove_t	npcmove_t;
typedef struct npc_t		npc_t;
typedef struct ent_s	ent_t;
typedef struct gclient_s	gclient_t;

struct npcframe_t
{
	void	(*aifn)(ent_t*, float dist);
	float	dist;
	void	(*think)(ent_t*);
};

struct npcmove_t
{
	int		firstframe, lastframe;
	npcframe_t	*frame;
	void		(*endfn)(ent_t*);
};

// NPCs are stupid bots.
struct npc_t
{
	pmove_t		pm;
	playerState_t	ps;
	usercmd_t	ucmd;
	npcmove_t	*mv;
	float		idealyaw;
	float		yawspeed;
	float		enemyyaw;
	qboolean	enemyvis;
	qboolean	enemyinfront;
	int		pausetime;
	int		nextframe;
	float		scale;
	vec3_t		goal;
	vec3_t		savedgoal;
	vec3_t		lastsighting;
	float		searchtime;
	float		trailtime;
	int		idletime;
	int		attackstate;
	int		aiflags;
	ent_t		*goalent;
	ent_t		*enemy;
	ent_t		*oldenemy;
	int		enemyrange;
	ent_t		*movetarg;
	char		*combattarg;
	float		viewheight;
	int		showhostile;
	// derived from ent->s.pos and ent->s.apos
	vec3_t		pos;
	vec3_t		vel;
	vec3_t		angles;
	vec3_t		anglesvel;
	qboolean	ongroundplane;
	trace_t		groundtrace;

	void		(*stand)(ent_t*);
	void		(*idle)(ent_t*);
	void		(*search)(ent_t*);
	void		(*walk)(ent_t*);
	void		(*run)(ent_t*);
	//void		(*attack)(ent_t*, ent_t *targ, float eta);
	void		(*attack)(ent_t*);
	void		(*melee)(ent_t*);
	void		(*sight)(ent_t*, ent_t*);
	qboolean	(*checkattack)(ent_t *);
};

struct ent_s
{
	entityState_t	s;	// communicated by server to clients
	entityShared_t	r;	// shared by both the server system and game

	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!
	//================================

	gclient_t		*client;	// nil if not a client
	npc_t			npc;		// just for NPCs

	qboolean		inuse;

	char			*classname;	// set in radiant
	int			spawnflags;	// set in radiant

	qboolean		neverfree;	// if true, FreeEntity will only unlink
						// bodyque uses this

	int			flags;	// FL_* variables

	char			*model;
	char			*model2;
	int			freetime;	// level.time when the object was freed

	int			eventtime;	// events will be cleared EVENT_VALID_MSEC after set
	qboolean		freeafterevent;
	qboolean		unlinkAfterEvent;

	qboolean		physobj;	// if true, it can be pushed by movers and fall off edges
	// all game items are physicsObjects,
	float			physbounce;	// 1.0 = continuous bounce, 0.0 = no bounce
	int			clipmask;	// brushes with this content value will be collided against
	// when moving.  items and corpses do not collide against
	// players, for instance

	// movers
	moverstate_t	moverstate;
	int		soundpos1;
	int		sound1to2;
	int		sound2to1;
	int		soundpos2;
	int		soundloop;
	ent_t	*parent;
	ent_t	*nexttrain;
	ent_t	*prevtrain;
	vec3_t		pos1, pos2;

	char		*message;

	int		timestamp;	// body queue sinking, etc

	char		*target;
	char		*targetname;
	char		*team;
	char		*targetshadername;
	char		*newtargetshadername;
	ent_t	*target_ent;

	float		speed;
	vec3_t		movedir;

	int		nextthink;
	void		(*think)(ent_t *self);
	void		(*reached)(ent_t *self);	// movers call this when hitting endpoint
	void		(*blocked)(ent_t *self, ent_t *other);
	void		(*touch)(ent_t *self, ent_t *other, trace_t *trace);
	void		(*use)(ent_t *self, ent_t *other, ent_t *activator);
	void		(*pain)(ent_t *self, ent_t *attacker, int damage);
	void		(*die)(ent_t *self, ent_t *inflictor, ent_t *attacker, int damage, int mod);

	int		paindebouncetime;
	int		flysounddebouncetime;	// wind tunnel
	int		keymsgdebouncetime;		// "you need the key"
	int		lastmovetime;

	int		health;

	qboolean	takedmg;

	int		damage;
	int		splashdmg;	// quad will increase this without increasing radius
	int		splashradius;
	int		meansofdeath;
	int		splashmeansofdeath;

	int		boxcontents;	// bg_itemlist index for crate_*s
	int		count;

	ent_t	*chain;
	ent_t	*enemy;
	ent_t	*activator;
	ent_t	*teamchain;	// next entity in team
	ent_t	*teammaster;	// master of the team

	int		doorkey;	// bg_itemlist index


	int		watertype;
	int		waterlevel;

	int		noiseindex;

	// timing variables
	float		wait;
	float		random;

	item_t		*item;	// for bonus items

	int		airouttime;
};

typedef enum
{
	CON_DISCONNECTED,
	CON_CONNECTING,
	CON_CONNECTED
} clientconnected_t;

typedef enum
{
	SPECTATOR_NOT,
	SPECTATOR_FREE,
	SPECTATOR_FOLLOW,
	SPECTATOR_SCOREBOARD
} specstate_t;

typedef enum
{
	TEAM_BEGIN,	// Beginning a team game, spawn at base
	TEAM_ACTIVE	// Now actively playing
} pteamstatenum;

typedef struct
{
	pteamstatenum	state;

	int			location;

	int			captures;
	int			basedefense;
	int			carrierdefense;
	int			flagrecovery;
	int			fragcarrier;
	int			assists;

	float			lasthurtcarrier;
	float			lastreturnedflag;
	float			flagsince;
	float			lastfraggedcarrier;
} pteamstate_t;

// client data that stays across multiple levels or tournament restarts
// this is achieved by writing all the data to cvar strings at game shutdown
// time and reading them back at connection time.  Anything added here
// MUST be dealt with in sessinit() / sessread() / sesswrite()
typedef struct
{
	teamnum_t			team;
	int			specnum;		// for determining next-in-line to play
	specstate_t	specstate;
	int			specclient;	// for chasecam and follow mode
	int			wins, losses;		// tournament stats
	qboolean		teamleader;		// true when this client is a team leader
} clientsess_t;

#define MAX_NETNAME	36
#define MAX_VOTE_COUNT	3

// client data that stays across multiple respawns, but is cleared
// on each level change or team change at clientbegin()
typedef struct
{
	clientconnected_t	connected;
	usercmd_t		cmd;			// we would lose angles if not persistant
	qboolean		localclient;		// true if "ip" info key is "localhost"
	qboolean		initialspawn;		// the first spawn should be at a cool location
	qboolean		predictitempickup;	// based on cg_predictItems userinfo
	qboolean		pmovefixed;		//
	char			netname[MAX_NETNAME];
	int			maxhealth;		// for handicapping
	int			entertime;		// level.time the client entered the game
	pteamstate_t	teamstate;		// status in teamplay games
	int			votecount;		// to prevent people from constantly calling votes
	int			teamvotecount;		// to prevent people from constantly calling votes
	qboolean		teaminfo;		// send team overlay updates?
} clientpersist_t;

// this structure is cleared on each clientspawn(),
// except for 'client->pers' and 'client->sess'
struct gclient_s
{
	// ps MUST be the first element, because the server expects it
	playerState_t		ps;	// communicated by server to clients

	// the rest of the structure is private to game
	clientpersist_t	pers;
	clientsess_t		sess;

	qboolean		readytoexit;	// wishes to leave the intermission

	qboolean		noclip;

	int			lastcmdtime;	// level.time of last usercmd_t, for EF_CONNECTION
	// we can't just use pers.lastCommand.time, because
	// of the g_sycronousclients case
	int			buttons;
	int			oldbuttons;
	int			latchedbuttons;

	vec3_t			oldorigin;

	// sum up damage over an entire frame, so
	// shotgun blasts give a single big kick
	int		dmgarmor;		// damage absorbed by armor
	int		dmgblood;		// damage taken out of health
	int		dmgknockback;	// impact damage
	vec3_t		dmgfrom;		// origin for vector calculation
	qboolean	dmgfromworld;	// if true, don't use the dmgfrom vector

	int		accuratecount;		// for "impressive" reward sound

	int		accuracyshots;		// total number of shots
	int		accuracyhits;		// total number of hits

	int		lastkilledclient;	// last client that this client killed
	int		lasthurtclient;	// last client that damaged this client
	int		lasthurt_mod;		// type of damage the client did

	// timers
	int		respawntime;		// can respawn when time > this, force after g_forcerespwan
	int		inactivitytime;		// kick players when time > this
	qboolean	inactivitywarning;	// qtrue if the five seoond warning has been given
	int		rewardtime;		// clear the EF_AWARD_IMPRESSIVE, etc when time > this

	int		lastkilltime;	// for multiple kill rewards

	qboolean	fireheld;	// used for hook
	ent_t	*hook;		// grapple hook if out

	int		switchteamtime;	// time the player switched teams

	// residualtime is used to handle events that happen every second
	// like health / armor countdowns and regeneration
	int		residualtime;


	char		*areabits;
};

// this structure is cleared as each map is entered
#define MAX_SPAWN_VARS		64
#define MAX_SPAWN_VARS_CHARS	4096

typedef struct
{
	struct gclient_s	*clients;	// [maxclients]

	struct ent_s	*entities;
	int			nentities;	// MAX_CLIENTS <= nentities <= ENTITYNUM_MAX_NORMAL

	int			warmuptime;	// restart match at this time

	fileHandle_t		logfile;

	// store latched cvars here that we want to get at often
	int			maxclients;

	int			framenum;
	int			time;		// in msec
	int			prevtime;	// so movers can back up when blocked

	int			starttime;	// level.time the map was started

	int			teamscores[TEAM_NUM_TEAMS];
	int			lastteamlocationtime;	// last time of client team location update

	qboolean		newsess;		// don't use any old session data, because
	// we changed gametype

	qboolean		restarted;	// waiting for a map_restart to fire

	int			nconnectedclients;
	int			nnonspecclients;	// includes connecting clients
	int			nplayingclients;		// connected, non-spectators
	int			sortedclients[MAX_CLIENTS];	// sorted by score
	int			follow1, follow2;		// clientNums for auto-follow spectators

	int			snd_fry;			// sound index for standing in lava

	int			warmupmodificationcount;	// for detecting if g_warmup is changed

	// voting state
	char			votestr[MAX_STRING_CHARS];
	char			votedisplaystr[MAX_STRING_CHARS];
	int			votetime;		// level.time vote was called
	int			voteexectime;	// time the vote is executed
	int			voteyes;
	int			voteno;
	int			nvoters;	// set by calcranks

	// team voting state
	char			teamvotestr[2][MAX_STRING_CHARS];
	int			teamvotetime[2];	// level.time vote was called
	int			teamvoteyes[2];
	int			teamvoteno[2];
	int			nteamvoters[2];// set by calcranks

	// spawn variables
	qboolean		spawning;			// the entspawn*() functions are valid
	int			nspawnvars;
	char			*spawnvars[MAX_SPAWN_VARS][2];	// key / value pairs
	int			nspawnvarchars;
	char			spawnvarchars[MAX_SPAWN_VARS_CHARS];

	// last checkpoint unlocked
	int			checkpoint;

	// intermission state
	int			intermissionqueued;	// intermission was qualified, but
	// wait INTERMISSION_DELAY_TIME before
	// actually going there so the last
	// frag can be watched.  Disable future
	// kills during this delay
	int		intermissiontime;	// time the intermission was started
	char		*changemap;
	qboolean	readytoexit;		// at least one client wants to exit
	int		exittime;
	vec3_t		intermissionpos;	// also used for spectator spawns
	vec3_t		intermissionangle;

	qboolean	loclinked;	// target_locations get linked
	ent_t		*lochead;	// head of the location list
	int		bodyqueueindex;	// dead bodies
	ent_t		*bodyqueue[BODY_QUEUE_SIZE];

	int		gameovertime;

	int		nsecrets;
	int		secretsfound;
	int		ncrates;
	int		ncratesbroken;
	int		ncarrots;
	int		ncarrotspickedup;

	int		numnpcs;
	ent_t		*sightclient;
	ent_t		*sightent;	// FIXME: what's the difference here?
	int		sightentframenum;
	ent_t		*soundent;
	int		soundentframe;
} levelstatic_t;

// g_spawn.c
qboolean	spawnstr(const char *key, const char *defaultString, char **out);
// spawn string returns a temporary reference, you must CopyString() if you want to keep it
qboolean	spawnfloat(const char *key, const char *defaultString, float *out);
qboolean	spawnint(const char *key, const char *defaultString, int *out);
qboolean	spawnvec(const char *key, const char *defaultString, float *out);
void		spawnall(void);
char		*newstr(const char *string);

// g_cmds.c
void		Cmd_Score_f(ent_t *ent);
void		stopfollowing(ent_t *ent);
void		broadcastteamchange(gclient_t *client, int oldTeam);
void		setteam(ent_t *ent, char *s);
void		Cmd_FollowCycle_f(ent_t *ent, int dir);

// g_items.c
void		checkteamitems(void);
void		runitem(ent_t *ent);
void		itemrespawn(ent_t *ent);

void		UseHoldableItem(ent_t *ent);
void		PrecacheItem(item_t *it);
ent_t	*itemdrop(ent_t *ent, item_t *item, float angle);
ent_t	*itemlaunch(item_t *item, vec3_t origin, vec3_t velocity);
void		setrespawn(ent_t *ent, float delay);
void		itemspawn(ent_t *ent, item_t *item);
void		itemspawnfinish(ent_t *ent);
void		weap_think(ent_t *ent);
void		addammo(ent_t *ent, int weapon, int count);
void		item_touch(ent_t *ent, ent_t *other, trace_t *trace);

void		clearitems(void);
void		registeritem(item_t *item);
void		mkitemsconfigstr(void);

// g_utils.c
void		G_ProjectSource(vec3_t pt, vec3_t dist, vec3_t fwd, vec3_t right, vec3_t out);
int		modelindex(char *name);
int		soundindex(char *name);
void		teamcmd(teamnum_t team, char *cmd);
void		killbox(ent_t *ent);
ent_t	*findent(ent_t *from, int fieldofs, const char *match);
ent_t	*picktarget(char *targetname);
void		usetargets(ent_t *ent, ent_t *activator);
void		setmovedir(vec3_t angles, vec3_t movedir);
int		inventory(playerState_t *ps, int item);

void		entinit(ent_t *e);
ent_t	*entspawn(void);
ent_t	*enttemp(vec3_t origin, int event);
void		mksound(ent_t *ent, int channel, int soundIndex);
void		entfree(ent_t *e);
qboolean	entsfree(void);

void		touchtriggers(ent_t *ent);

float		*tv(float x, float y, float z);
char		*vtos(const vec3_t v);

float		vectoyaw(const vec3_t vec);
float		anglemod(float a);

void		addpredictable(ent_t *ent, int event, int eventParm);
void		addevent(ent_t *ent, int event, int eventParm);
void		setorigin(ent_t *ent, vec3_t origin);
void		addshaderremap(const char *oldShader, const char *newShader, float timeOffset);
const char	*mkshaderstateconfigstr(void);

// g_combat.c
qboolean	candamage(ent_t *targ, vec3_t origin);
void		entdamage(ent_t *targ, ent_t *inflictor, ent_t *attacker, vec3_t dir, vec3_t point, int damage, int dflags, int mod);
qboolean	radiusdamage(vec3_t origin, ent_t *attacker, float damage, float radius, ent_t *ignore, int mod);
qboolean	boxdamage(vec3_t pos, vec3_t mins, vec3_t maxs, ent_t *attacker,  int ignore, float dmg, int mod);
void		body_die(ent_t *self, ent_t *inflictor, ent_t *attacker, int damage, int meansOfDeath);
void		tossclientitems(ent_t *self);

// damage flags
#define DAMAGE_RADIUS			0x00000001	// damage was indirect
#define DAMAGE_NO_ARMOR			0x00000002	// armour does not protect from this damage
#define DAMAGE_NO_KNOCKBACK		0x00000004	// do not affect velocity, just view angles
#define DAMAGE_NO_PROTECTION		0x00000008	// armor, shields, invulnerability, and godmode have no effect

// g_missile.c
void		runmissile(ent_t *ent);

ent_t	*fire_bolt(ent_t *self, vec3_t start, vec3_t aimdir);
ent_t	*fire_plasma(ent_t *self, vec3_t start, vec3_t aimdir);
ent_t	*fire_grenade(ent_t *self, vec3_t start, vec3_t aimdir);
ent_t	*fire_rocket(ent_t *self, vec3_t start, vec3_t dir);
ent_t	*fire_bfg(ent_t *self, vec3_t start, vec3_t dir);
ent_t	*fire_grapple(ent_t *self, vec3_t start, vec3_t dir);

// g_mover.c
void	runmover(ent_t *ent);
void	doortrigger_touch(ent_t *ent, ent_t *other, trace_t *trace);

// g_npc.c
void	npcstart(ent_t *e);
void	npcstartgo(ent_t *e);
void	npcdroptofloor(ent_t *ent);
void	npcthink(ent_t *e);
void	walknpcstart(ent_t *e);
void	swimnpcstart(ent_t *e);
void	flynpcstart(ent_t *e);
void	npcattackfinished(ent_t *e, float time);
void	npcdeathuse(ent_t *e);
void	npccategorizepos(ent_t *ent);
qboolean	npccheckattack(ent_t *e);
void	npccheckfly(ent_t *e);
void	npccheckground(ent_t *ent);

// g_npcai.c
void	aistand(ent_t *e, float dist);
void	aiwalk(ent_t *e, float dist);
void	aimove(ent_t *e, float dist);
void	aiturn(ent_t *e, float dist);
void	airun(ent_t *e, float dist);
void	aicharge(ent_t *e, float dist);
qboolean	visible(ent_t *e, ent_t *other);
void	setsightclient(void);

// g_npcmove.c
void	npcchangeyaw(ent_t*);
void	npcmovetogoal(ent_t *e, float dist);
qboolean	npcwalkmove(ent_t *e, float yaw, float dist);

// g_trigger.c
void	trigger_teleporter_touch(ent_t *self, ent_t *other, trace_t *trace);

// g_misc.c
void	teleportentity(ent_t *player, vec3_t origin, vec3_t angles);

// g_weapon.c
qboolean	logaccuracyhit(ent_t *target, ent_t *attacker);
void		calcmuzzlepoint(ent_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint);
void		snapvectortowards(vec3_t v, vec3_t to);
qboolean	chkgauntletattack(ent_t *ent);
qboolean	chkmelee2attack(ent_t *ent);
void		weapon_hook_free(ent_t *ent);
void		weapon_hook_think(ent_t *ent);

// g_client.c
int		teamcount(int ignoreClientNum, teamnum_t team);
int		teamleader(int team);
teamnum_t		pickteam(int ignoreClientNum);
void		setviewangles(ent_t *ent, vec3_t angle);
ent_t	*selectspawnpoint(vec3_t avoidPoint, vec3_t origin, vec3_t angles, qboolean isbot);
void		copytobodyqueue(ent_t *ent);
void		clientrespawn(ent_t *ent);
void		intermission(void);
void		initbodyqueue(void);
void		clientspawn(ent_t *ent);
void		player_die(ent_t *self, ent_t *inflictor, ent_t *attacker, int damage, int mod);
void		addscore(ent_t *ent, vec3_t origin, int score);
void		calcranks(void);
qboolean	possibletelefrag(ent_t *spot);
void		clientgameover(ent_t *e);

// g_svcmds.c
qboolean	consolecmd(void);
void		processipbans(void);
qboolean	filterpacket(char *from);

// g_weapon.c
void		fireweapon(ent_t *ent);

// g_cmds.c
void		deathmatchscoreboardmsg(ent_t *ent);

// g_main.c
void		clientintermission(ent_t *ent);
void		findintermissionpoint(void);
void		setleader(int team, int client);
void		chkteamleader(int team);
void		runthink(ent_t *ent);
void		addtourneyqueue(gclient_t *client);
void		gameover(void);
void QDECL	logprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void		sendscoreboardmsgall(void);
void QDECL	gprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void QDECL	errorf(const char *fmt, ...) __attribute__((noreturn, format(printf, 1, 2)));

// g_client.c
char		*clientconnect(int clientNum, qboolean firstTime, qboolean isBot);
void		clientuserinfochanged(int clientNum);
void		clientdisconnect(int clientNum);
void		clientbegin(int clientNum);
void		clientcmd(int clientNum);

// g_active.c
void		clientthink(int clientNum);
void		clientendframe(ent_t *ent);
void		runclient(ent_t *ent);

// g_team.c
qboolean	onsameteam(ent_t *ent1, ent_t *ent2);
void		ckhdroppedteamitem(ent_t *dropped);
qboolean	CheckObeliskAttack(ent_t *obelisk, ent_t *attacker);

// g_mem.c
void		*alloc(int size);
void		initmem(void);
void		Svcmd_GameMem_f(void);

// g_npcai.c
void		foundtarget(ent_t *e);

// g_save.c
void		savegame(const char *fname);
void		loadgame(const char *fname);
void		clientfromsave(gclient_t *client, const char *guid);

// g_session.c
void		sessread(gclient_t *client);
void		sesswrite(void);
void		sessinit(gclient_t *client, char *userinfo);
void		worldsessinit(void);

// g_trail.c
void		trailinit(void);
void		trailadd(vec3_t spot);
ent_t*		trailfirst(ent_t *e);
ent_t*		trailnext(ent_t *e);
ent_t*		traillastspot(void);

// g_arenas.c
void		updatetourney(void);
void		spawnonvictorypads(void);
void		Svcmd_AbortPodium_f(void);

// g_bot.c
void		initbots(qboolean restart);
char		*botinfo(int num);
char		*botinfobyname(const char *name);
void		chkbotspawn(void);
void		dequeuebotbegin(int clientNum);
qboolean	botconnect(int clientNum, qboolean restart);
void		Svcmd_AddBot_f(void);
void		Svcmd_BotList_f(void);
void		botinterbreed(void);

// ai_main.c
#define MAX_FILEPATH 144

//bot settings
typedef struct bot_settings_s
{
	char	characterfile[MAX_FILEPATH];
	float	skill;
	char	team[MAX_FILEPATH];
} bot_settings_t;

int	BotAISetup(int restart);
int	BotAIShutdown(int restart);
int	BotAILoadMap(int restart);
int	BotAISetupClient(int client, struct bot_settings_s *settings, qboolean restart);
int	BotAIShutdownClient(int client, qboolean restart);
int	BotAIStartFrame(int time);
void	BotTestAAS(vec3_t origin);

#include "g_team.h"	// teamplay specific stuff

extern levelstatic_t level;
extern ent_t g_entities[MAX_GENTITIES];

#define FOFS(x)		((size_t)&(((ent_t*)0)->x))

extern vmCvar_t g_gametype;
extern vmCvar_t g_dedicated;
extern vmCvar_t g_cheats;
extern vmCvar_t g_maxclients;		// allow this many total, including spectators
extern vmCvar_t g_maxGameClients;	// allow this many active
extern vmCvar_t g_restarted;

extern vmCvar_t g_dmflags;
extern vmCvar_t g_fraglimit;
extern vmCvar_t g_timelimit;
extern vmCvar_t g_capturelimit;
extern vmCvar_t g_friendlyFire;
extern vmCvar_t g_password;
extern vmCvar_t g_needpass;
extern vmCvar_t g_gravity;
extern vmCvar_t g_speed;
extern vmCvar_t g_knockback;
extern vmCvar_t g_quadfactor;
extern vmCvar_t g_forcerespawn;
extern vmCvar_t g_inactivity;
extern vmCvar_t g_debugMove;
extern vmCvar_t g_debugAlloc;
extern vmCvar_t g_debugDamage;
extern vmCvar_t	g_debugNPC;
extern vmCvar_t g_weaponRespawn;
extern vmCvar_t g_weaponTeamRespawn;
extern vmCvar_t g_synchronousClients;
extern vmCvar_t g_motd;
extern vmCvar_t g_warmup;
extern vmCvar_t g_doWarmup;
extern vmCvar_t g_blood;
extern vmCvar_t g_allowVote;
extern vmCvar_t g_teamAutoJoin;
extern vmCvar_t g_teamForceBalance;
extern vmCvar_t g_banIPs;
extern vmCvar_t g_filterBan;
extern vmCvar_t g_obeliskHealth;
extern vmCvar_t g_obeliskRegenPeriod;
extern vmCvar_t g_obeliskRegenAmount;
extern vmCvar_t g_obeliskRespawnDelay;
extern vmCvar_t g_cubeTimeout;
extern vmCvar_t g_redteam;
extern vmCvar_t g_blueteam;
extern vmCvar_t g_smoothClients;
extern vmCvar_t pmove_fixed;
extern vmCvar_t pmove_msec;
extern vmCvar_t g_rankings;
extern vmCvar_t g_enableDust;
extern vmCvar_t g_enableBreath;
extern vmCvar_t g_singlePlayer;
extern vmCvar_t g_proxMineTimeout;

void		trap_Print(const char *text);
void		trap_Error(const char *text) __attribute__((noreturn));
int		trap_Milliseconds(void);
int		trap_RealTime(qtime_t *qtime);
int		trap_Argc(void);
void		trap_Argv(int n, char *buffer, int bufferLength);
void		trap_Args(char *buffer, int bufferLength);
int		trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode);
void		trap_FS_Read(void *buffer, int len, fileHandle_t f);
void		trap_FS_Write(const void *buffer, int len, fileHandle_t f);
void		trap_FS_FCloseFile(fileHandle_t f);
int		trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize);
int		trap_FS_Seek(fileHandle_t f, long offset, int origin);	// fsOrigin_t
void		trap_SendConsoleCommand(int exec_when, const char *text);
void		trap_Cvar_Register(vmCvar_t *cvar, const char *var_name, const char *value, int flags);
void		trap_Cvar_Update(vmCvar_t *cvar);
void		trap_Cvar_Set(const char *var_name, const char *value);
int		trap_Cvar_VariableIntegerValue(const char *var_name);
float		trap_Cvar_VariableValue(const char *var_name);
void		trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize);
void		trap_LocateGameData(ent_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *gameClients, int sizeofGameClient);
void		trap_DropClient(int clientNum, const char *reason);
void		trap_SendServerCommand(int clientNum, const char *text);
void		trap_SetConfigstring(int num, const char *string);
void		trap_GetConfigstring(int num, char *buffer, int bufferSize);
void		trap_GetUserinfo(int num, char *buffer, int bufferSize);
void		trap_SetUserinfo(int num, const char *buffer);
void		trap_GetServerinfo(char *buffer, int bufferSize);
void		trap_SetBrushModel(ent_t *ent, const char *name);
void		trap_Trace(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask);
int		trap_PointContents(const vec3_t point, int passEntityNum);
qboolean	trap_InPVS(const vec3_t p1, const vec3_t p2);
qboolean	trap_InPVSIgnorePortals(const vec3_t p1, const vec3_t p2);
void		trap_AdjustAreaPortalState(ent_t *ent, qboolean open);
qboolean	trap_AreasConnected(int area1, int area2);
void		trap_LinkEntity(ent_t *ent);
void		trap_UnlinkEntity(ent_t *ent);
int		trap_EntitiesInBox(const vec3_t mins, const vec3_t maxs, int *entityList, int maxcount);
qboolean	trap_EntityContact(const vec3_t mins, const vec3_t maxs, const ent_t *ent);
int		trap_BotAllocateClient(void);
void		trap_BotFreeClient(int clientNum);
void		trap_GetUsercmd(int clientNum, usercmd_t *cmd);
qboolean	trap_GetEntityToken(char *buffer, int bufferSize);

int		trap_DebugPolygonCreate(int color, int numPoints, vec3_t *points);
void		trap_DebugPolygonDelete(int id);

int		trap_BotLibSetup(void);
int		trap_BotLibShutdown(void);
int		trap_BotLibVarSet(char *var_name, char *value);
int		trap_BotLibVarGet(char *var_name, char *value, int size);
int		trap_BotLibDefine(char *string);
int		trap_BotLibStartFrame(float time);
int		trap_BotLibLoadMap(const char *mapname);
int		trap_BotLibUpdateEntity(int ent, void /* struct bot_updateentity_s */ *bue);
int		trap_BotLibTest(int parm0, char *parm1, vec3_t parm2, vec3_t parm3);

int		trap_BotGetSnapshotEntity(int clientNum, int sequence);
int		trap_BotGetServerCommand(int clientNum, char *message, int size);
void		trap_BotUserCommand(int client, usercmd_t *ucmd);

int		trap_AAS_BBoxAreas(vec3_t absmins, vec3_t absmaxs, int *areas, int maxareas);
int		trap_AAS_AreaInfo(int areanum, void /* struct aas_areainfo_s */ *info);
void		trap_AAS_EntityInfo(int entnum, void /* struct aas_entityinfo_s */ *info);

int		trap_AAS_Initialized(void);
void		trap_AAS_PresenceTypeBoundingBox(int presencetype, vec3_t mins, vec3_t maxs);
float		trap_AAS_Time(void);

int		trap_AAS_PointAreaNum(vec3_t point);
int		trap_AAS_PointReachabilityAreaIndex(vec3_t point);
int		trap_AAS_TraceAreas(vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas);

int		trap_AAS_PointContents(vec3_t point);
int		trap_AAS_NextBSPEntity(int ent);
int		trap_AAS_ValueForBSPEpairKey(int ent, char *key, char *value, int size);
int		trap_AAS_VectorForBSPEpairKey(int ent, char *key, vec3_t v);
int		trap_AAS_FloatForBSPEpairKey(int ent, char *key, float *value);
int		trap_AAS_IntForBSPEpairKey(int ent, char *key, int *value);

int		trap_AAS_AreaReachability(int areanum);

int		trap_AAS_AreaTravelTimeToGoalArea(int areanum, vec3_t origin, int goalareanum, int travelflags);
int		trap_AAS_EnableRoutingArea(int areanum, int enable);
int		trap_AAS_PredictRoute(void /*struct aas_predictroute_s*/ *route, int areanum, vec3_t origin,
				      int goalareanum, int travelflags, int maxareas, int maxtime,
				      int stopevent, int stopcontents, int stoptfl, int stopareanum);

int	trap_AAS_AlternativeRouteGoals(vec3_t start, int startareanum, vec3_t goal, int goalareanum, int travelflags,
				       void /*struct aas_altroutegoal_s*/ *altroutegoals, int maxaltroutegoals,
				       int type);
int	trap_AAS_Swimming(vec3_t origin);
int	trap_AAS_PredictClientMovement(void /* aas_clientmove_s */ *move, int entnum, vec3_t origin, int presencetype, int onground, vec3_t velocity, vec3_t cmdmove, int cmdframes, int maxframes, float frametime, int stopevent, int stopareanum, int visualize);

void	trap_EA_Say(int client, char *str);
void	trap_EA_SayTeam(int client, char *str);
void	trap_EA_Command(int client, char *command);

void	trap_EA_Action(int client, int action);
void	trap_EA_Gesture(int client);
void	trap_EA_Talk(int client);
void	trap_EA_Attack(int client);
void	trap_EA_Use(int client);
void	trap_EA_Respawn(int client);
void	trap_EA_Crouch(int client);
void	trap_EA_MoveUp(int client);
void	trap_EA_MoveDown(int client);
void	trap_EA_MoveForward(int client);
void	trap_EA_MoveBack(int client);
void	trap_EA_MoveLeft(int client);
void	trap_EA_MoveRight(int client);
void	trap_EA_SelectWeapon(int client, int weapon);
void	trap_EA_Jump(int client);
void	trap_EA_DelayedJump(int client);
void	trap_EA_Move(int client, vec3_t dir, float speed);
void	trap_EA_View(int client, vec3_t viewangles);

void	trap_EA_EndRegular(int client, float thinktime);
void	trap_EA_GetInput(int client, float thinktime, void /* struct bot_input_s */ *input);
void	trap_EA_ResetInput(int client);

int	trap_BotLoadCharacter(char *charfile, float skill);
void	trap_BotFreeCharacter(int character);
float	trap_Characteristic_Float(int character, int index);
float	trap_Characteristic_BFloat(int character, int index, float min, float max);
int	trap_Characteristic_Integer(int character, int index);
int	trap_Characteristic_BInteger(int character, int index, int min, int max);
void	trap_Characteristic_String(int character, int index, char *buf, int size);

int	trap_BotAllocChatState(void);
void	trap_BotFreeChatState(int handle);
void	trap_BotQueueConsoleMessage(int chatstate, int type, char *message);
void	trap_BotRemoveConsoleMessage(int chatstate, int handle);
int	trap_BotNextConsoleMessage(int chatstate, void /* struct bot_consolemessage_s */ *cm);
int	trap_BotNumConsoleMessages(int chatstate);
void	trap_BotInitialChat(int chatstate, char *type, int mcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7);
int	trap_BotNumInitialChats(int chatstate, char *type);
int	trap_BotReplyChat(int chatstate, char *message, int mcontext, int vcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7);
int	trap_BotChatLength(int chatstate);
void	trap_BotEnterChat(int chatstate, int client, int sendto);
void	trap_BotGetChatMessage(int chatstate, char *buf, int size);
int	trap_StringContains(char *str1, char *str2, int casesensitive);
int	trap_BotFindMatch(char *str, void /* struct bot_match_s */ *match, ulong context);
void	trap_BotMatchVariable(void /* struct bot_match_s */ *match, int variable, char *buf, int size);
void	trap_UnifyWhiteSpaces(char *string);
void	trap_BotReplaceSynonyms(char *string, ulong context);
int	trap_BotLoadChatFile(int chatstate, char *chatfile, char *chatname);
void	trap_BotSetChatGender(int chatstate, int gender);
void	trap_BotSetChatName(int chatstate, char *name, int client);
void	trap_BotResetGoalState(int goalstate);
void	trap_BotRemoveFromAvoidGoals(int goalstate, int number);
void	trap_BotResetAvoidGoals(int goalstate);
void	trap_BotPushGoal(int goalstate, void /* struct bot_goal_s */ *goal);
void	trap_BotPopGoal(int goalstate);
void	trap_BotEmptyGoalStack(int goalstate);
void	trap_BotDumpAvoidGoals(int goalstate);
void	trap_BotDumpGoalStack(int goalstate);
void	trap_BotGoalName(int number, char *name, int size);
int	trap_BotGetTopGoal(int goalstate, void /* struct bot_goal_s */ *goal);
int	trap_BotGetSecondGoal(int goalstate, void /* struct bot_goal_s */ *goal);
int	trap_BotChooseLTGItem(int goalstate, vec3_t origin, int *inventory, int travelflags);
int	trap_BotChooseNBGItem(int goalstate, vec3_t origin, int *inventory, int travelflags, void /* struct bot_goal_s */ *ltg, float maxtime);
int	trap_BotTouchingGoal(vec3_t origin, void /* struct bot_goal_s */ *goal);
int	trap_BotItemGoalInVisButNotVisible(int viewer, vec3_t eye, vec3_t viewangles, void /* struct bot_goal_s */ *goal);
int	trap_BotGetNextCampSpotGoal(int num, void /* struct bot_goal_s */ *goal);
int	trap_BotGetMapLocationGoal(char *name, void /* struct bot_goal_s */ *goal);
int	trap_BotGetLevelItemGoal(int index, char *classname, void /* struct bot_goal_s */ *goal);
float	trap_BotAvoidGoalTime(int goalstate, int number);
void	trap_BotSetAvoidGoalTime(int goalstate, int number, float avoidtime);
void	trap_BotInitLevelItems(void);
void	trap_BotUpdateEntityItems(void);
int	trap_BotLoadItemWeights(int goalstate, char *filename);
void	trap_BotFreeItemWeights(int goalstate);
void	trap_BotInterbreedGoalFuzzyLogic(int parent1, int parent2, int child);
void	trap_BotSaveGoalFuzzyLogic(int goalstate, char *filename);
void	trap_BotMutateGoalFuzzyLogic(int goalstate, float range);
int	trap_BotAllocGoalState(int state);
void	trap_BotFreeGoalState(int handle);

void	trap_BotResetMoveState(int movestate);
void	trap_BotMoveToGoal(void	/* struct bot_moveresult_s */ *result, int movestate, void /* struct bot_goal_s */ *goal, int travelflags);
int	trap_BotMoveInDirection(int movestate, vec3_t dir, float speed, int type);
void	trap_BotResetAvoidReach(int movestate);
void	trap_BotResetLastAvoidReach(int movestate);
int	trap_BotReachabilityArea(vec3_t origin, int testground);
int	trap_BotMovementViewTarget(int movestate, void /* struct bot_goal_s */ *goal, int travelflags, float lookahead, vec3_t target);
int	trap_BotPredictVisiblePosition(vec3_t origin, int areanum, void	/* struct bot_goal_s */ *goal, int travelflags, vec3_t target);
int	trap_BotAllocMoveState(void);
void	trap_BotFreeMoveState(int handle);
void	trap_BotInitMoveState(int handle, void /* struct bot_initmove_s */ *initmove);
void	trap_BotAddAvoidSpot(int movestate, vec3_t origin, float radius, int type);

int	trap_BotChooseBestFightWeapon(int weaponstate, int *inventory);
void	trap_BotGetWeaponInfo(int weaponstate, int weapon, void	/* struct weaponinfo_s */ *weaponinfo);
int	trap_BotLoadWeaponWeights(int weaponstate, char *filename);
int	trap_BotAllocWeaponState(void);
void	trap_BotFreeWeaponState(int weaponstate);
void	trap_BotResetWeaponState(int weaponstate);

int	trap_GeneticParentsAndChildSelection(int numranks, float *ranks, int *parent1, int *parent2, int *child);

void	trap_SnapVector(float *v);
