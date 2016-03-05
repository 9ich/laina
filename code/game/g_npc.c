#include "g_local.h"
#include "bg_local.h"

enum
{
	SUSPEND		= (1<<0),	// don't drop path_corners to floor
};

static void
npclevelrespawn(ent_t *ent)
{
	restoreinitialstate(ent);
}

void
runnpc(ent_t *ent)
{
	if(ent->s.pos.trType == TR_GRAVITY)
		runitem(ent);	// flops on ground like an item
	else
		runmover(ent);
}

/*
Think function. The wait time at a corner has completed, so start moving again.
*/
static void
startmoving(ent_t *ent)
{
	ent->s.pos.trTime = level.time;
	ent->s.pos.trType = TR_LINEAR_STOP;
	ent->s.anim = ANIM_NPCWALK;
}

extern void	SetMoverState(ent_t *ent, moverstate_t moverstate, int time);

static void
npcreached(ent_t *ent)
{
	ent_t *next;
	float speed;
	vec3_t move;
	float length;
	int i;

	// copy the apropriate values
	next = ent->nexttrain;
	if(!next || !next->nexttrain)
		return;	// just stop

	// fire all other targets
	usetargets(next, nil);

	// set the new trajectory
	ent->nexttrain = next->nexttrain;
	veccpy(next->s.origin, ent->pos1);
	veccpy(next->nexttrain->s.origin, ent->pos2);

	// transition the angles
	for(i = 0; i < 3; i++)
		ent->s.apos.trBase[i] = anglemod(ent->r.currentAngles[i]);
	for(i = 0; i < 3; i++){
		ent->s.apos.trDelta[i] = anglemod(next->s.angles[i] - ent->r.currentAngles[i]);
		if(next->s.angles[i] - ent->r.currentAngles[i] <= 0)
			ent->s.apos.trDelta[i] = -anglemod(-ent->s.apos.trDelta[i]);
		if(next->wait != 0.0f)
			ent->s.apos.trDelta[i] /= next->wait;
	}
	if(next->wait != 0.0f)
		ent->s.apos.trDuration = next->wait*1000;
	else
		ent->s.apos.trDuration = 1000;
	ent->s.apos.trTime = level.time;
	ent->s.apos.trType = TR_LINEAR_STOP;
	ent->s.anim = ANIM_NPCTURN;

	// if the path_corner has a speed, use that
	if(next->speed)
		speed = next->speed;
	else
		// otherwise use the NPC's speed
		speed = ent->speed;
	if(speed < 1)
		speed = 1;

	// calculate duration
	vecsub(ent->pos2, ent->pos1, move);
	length = veclen(move);

	ent->s.pos.trDuration = length * 1000 / speed;

	ent->r.svFlags &= ~SVF_NOCLIENT;

	if(ent->s.pos.trDuration < 1){
		ent->s.pos.trDuration = 1;

		ent->r.svFlags |= SVF_NOCLIENT;
	}

	// looping sound
	ent->s.loopSound = next->soundloop;

	// start it going
	SetMoverState(ent, MOVER_1TO2, level.time);

	ent->think = startmoving;

	// if there is a "wait" value on the target, don't start moving yet
	if(next->wait){
		ent->nextthink = level.time + next->wait * 1000;
		ent->s.pos.trType = TR_STATIONARY;
	}
}

static void
npcuse(ent_t *ent, ent_t *other, ent_t *activator)
{
	// let NPC do startmoving
	ent->nextthink = level.time;
	ent->use = nil;
}

/*
Link all the corners together
*/
static void
npclinktargets(ent_t *ent)
{
	ent_t *path, *next, *start;
	vec3_t end;
	trace_t tr;

	ent->nexttrain = findent(nil, FOFS(targetname), ent->target);
	if(!ent->nexttrain){
		gprintf("npc at %s with an unfound target\n",
		   vtos(ent->r.absmin));
		return;
	}

	start = nil;
	for(path = ent->nexttrain; path != start; path = next){
		if(!start)
			start = path;

		// if the corner has no target, the train will stop there
		if(!path->target)
			break;

		// find a path_corner among the targets
		// there may also be other targets that get fired when the corner
		// is reached
		next = nil;
		do{
			next = findent(next, FOFS(targetname), path->target);
			if(!next){
				gprintf("npc corner at %s without a target path_corner\n",
					 vtos(path->s.origin));
				return;
			}
		}while(strcmp(next->classname, "path_corner") != 0);

		if(!(ent->spawnflags & SUSPEND)){
			vec3_t mins, maxs;

			vecset(mins, -1, -1, -2);
			vecset(maxs, 1, 1, 1);
			// drop target path_corners to floor
			veccpy(next->s.origin, end);
			end[2] -= 99999;
			trap_Trace(&tr, next->s.origin, mins, maxs, end,
			   next->s.number, MASK_NPCSOLID);
			if(tr.fraction != 1.0f && !tr.allsolid)
				veccpy(tr.endpos, next->s.origin);
		}

		path->nexttrain = next;
	}

	// place the NPC on the first corner, but make it wait for activation
	npcreached(ent);
	ent->nextthink = 0;
	ent->s.pos.trType = TR_STATIONARY;
	ent->use = npcuse;
}

static void
npcsetup(ent_t *self)
{
	vec3_t move;
	float distance;
	float light;
	vec3_t color;
	qboolean lightSet, colorSet;
	char *sound;

	vecclear(self->s.angles);

	if(!self->speed)
		self->speed = 100;

	if(!self->target){
		gprintf("npc without a target at %s\n", vtos(self->r.absmin));
		entfree(self);
		return;
	}

	// init mover

	// if the "model2" key is set, use a seperate model
	// for drawing, but clip against the brushes
	if(self->model2)
		self->s.modelindex2 = modelindex(self->model2);

	// if the "loopsound" key is set, use a constant looping sound when moving
	if(spawnstr("noise", "100", &sound))
		self->s.loopSound = soundindex(sound);

	// if the "color" or "light" keys are set, setup constantLight
	lightSet = spawnfloat("light", "100", &light);
	colorSet = spawnvec("color", "1 1 1", color);
	if(lightSet || colorSet){
		int r, g, b, i;

		r = color[0] * 255;
		if(r > 255)
			r = 255;
		g = color[1] * 255;
		if(g > 255)
			g = 255;
		b = color[2] * 255;
		if(b > 255)
			b = 255;
		i = light / 4;
		if(i > 255)
			i = 255;
		self->s.constantLight = r | (g << 8) | (b << 16) | (i << 24);
	}

	self->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	self->s.eType = ET_NPC;
	veccpy(self->pos1, self->r.currentOrigin);

	self->s.pos.trType = TR_STATIONARY;
	veccpy(self->pos1, self->s.pos.trBase);

	// calculate time to reach second position from speed
	vecsub(self->pos2, self->pos1, move);
	distance = veclen(move);
	if(!self->speed)
		self->speed = 100;
	vecmul(move, self->speed, self->s.pos.trDelta);
	self->s.pos.trDuration = distance * 1000 / self->speed;
	if(self->s.pos.trDuration <= 0)
		self->s.pos.trDuration = 1;

	self->reached = npcreached;

	// start trains on the second frame, to make sure their targets have had
	// a chance to spawn
	self->nextthink = level.time + FRAMETIME;
	self->think = npclinktargets;
	self->levelrespawn = npclevelrespawn;
	self->ckpoint = level.checkpoint;
}

static void
deathend(ent_t *e)
{
	e->r.svFlags |= SVF_NOCLIENT;
}

static void
liedead(ent_t *e)
{
	e->s.anim = ANIM_NPCDEAD;
	e->think = deathend;
	e->nextthink = level.time + 10000;
}

static void
npc_die(ent_t *e, ent_t *inflictor, ent_t *attacker, int dmg, int mod)
{
	vec3_t end;
	trace_t tr;

	e->ckpoint = level.checkpoint;

	veccpy(e->s.origin, end);
	end[2] -= 99999;
	trap_Trace(&tr, e->r.currentOrigin, e->r.mins, e->r.maxs, end, e->s.number, MASK_SOLID);
	if(tr.startsolid){
		trap_UnlinkEntity(e);
		return;
	}

	e->s.groundEntityNum = tr.entityNum;

	setorigin(e, e->r.currentOrigin);
	e->s.pos.trType = TR_GRAVITY;
	e->s.pos.trTime = level.time;
	e->physbounce = 0.5f;
	e->s.eFlags |= EF_BOUNCE_HALF;
	e->r.contents = 0;
	e->s.anim = ANIM_NPCDEATH;
	e->think = liedead;
	e->nextthink = level.time + 800;
	trap_LinkEntity(e);
}

static void
npc_touch(ent_t *ent, ent_t *other, trace_t *trace)
{
	vec3_t dir, org, corner, closest;
	float dist;

	if(other->client == nil)
		return;

	// if player entered the bounds from above, kill the npc
	vecadd(ent->r.absmin, ent->r.absmax, org);
	vecmul(org, 0.5f, org);

	vecset(closest, other->r.absmin[0], other->r.absmin[1], other->r.absmin[2]);
	dist = vecdistsq(closest, org);

	vecset(corner, other->r.absmax[0], other->r.absmin[1], other->r.absmin[2]);
	if(vecdistsq(corner, org) < dist){
		veccpy(corner, closest);
		dist = vecdistsq(closest, org);
	}
	vecset(corner, other->r.absmin[0], other->r.absmax[1], other->r.absmin[2]);
	if(vecdistsq(corner, org) < dist){
		veccpy(corner, closest);
		dist = vecdistsq(closest, org);
	}
	vecset(corner, other->r.absmax[0], other->r.absmax[1], other->r.absmin[2]);
	if(vecdistsq(corner, org) < dist){
		veccpy(corner, closest);
	}

	vecsub(closest, org, dir);
	vecnorm(dir);

	if(other->client->ps.velocity[2] < 0 &&
	   dir[2] > 0.5f*M_PI - DEG2RAD(60) &&
	   dir[2] <= 0.5f*M_PI + DEG2RAD(60)){
		npc_die(ent, other, other, 1, 0);
		other->client->ps.velocity[2] = JUMP_VELOCITY;
		return;
	}
	other->client->ps.velocity[2] = JUMP_VELOCITY;
	entdamage(other, nil, nil, nil, nil, ent->damage, DAMAGE_NO_PROTECTION, ent->meansofdeath);
}

static void
npc_blocked(ent_t *ent, ent_t *other)
{
	if(other->client == nil)
		return;

	other->client->ps.velocity[2] = JUMP_VELOCITY;
	entdamage(other, ent, ent, nil, nil, ent->damage, DAMAGE_NO_PROTECTION, ent->meansofdeath);
}

static void
npcfinishspawn(ent_t *e)
{
	npcsetup(e);
	e->r.contents = CONTENTS_TRIGGER;
	e->s.anim = ANIM_NPCIDLE;
	trap_LinkEntity(e);
}

/*
An overgrown rat.

SUSPENDED	no drop to floor

"angle"		angle to face when spawning
"target"	the first path_corner of a succession of linked path_corners to follow
*/
void
SP_npc_rat(ent_t *e)
{
	e->model = "models/npc/rat/rat";
	e->s.modelindex = modelindex(e->model);
	vecset(e->r.mins, -30, -30, 0);
	vecset(e->r.maxs, 30, 30, 26);
	setorigin(e, e->s.origin);
	e->health = 1;
	e->takedmg = qtrue;
	e->damage = 1;
	e->meansofdeath = 2;

	e->die = npc_die;
	e->blocked = npc_blocked;
	e->touch = npc_touch;

	npcfinishspawn(e);
}
