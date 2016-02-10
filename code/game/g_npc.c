#include "g_local.h"

/*
Think function. The wait time at a corner has completed, so start moving again.
*/
static void
startmoving(ent_t *ent)
{
	ent->s.pos.trTime = level.time;
	ent->s.pos.trType = TR_LINEAR_STOP;
}

extern void	SetMoverState(ent_t *ent, moverstate_t moverstate, int time);

static void
npcreached(ent_t *ent)
{
	ent_t *next;
	float speed;
	vec3_t move;
	float length;

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

	// if the path_corner has a speed, use that
	if(next->speed)
		speed = next->speed;
	else
		// otherwise use the train's speed
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

	// if there is a "wait" value on the target, don't start moving yet
	if(next->wait){
		ent->nextthink = level.time + next->wait * 1000;
		ent->think = startmoving;
		ent->s.pos.trType = TR_STATIONARY;
	}
}

static void
npcuse(ent_t *ent, ent_t *other, ent_t *activator)
{
	ent->nextthink = level.time + 1;
	ent->think = startmoving;
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
			// drop target path_corners to floor, subject to npc bounds
			veccpy(next->s.origin, end);
			end[2] -= 99999;
			trap_Trace(&tr, next->s.origin, ent->r.mins, ent->r.maxs, end,
			   next->s.number, MASK_NPCSOLID);
			if(tr.fraction == 1.0f || tr.allsolid)
				continue;
			veccpy(tr.endpos, next->s.origin);
		}while(strcmp(next->classname, "path_corner"));

		path->nexttrain = next;
	}

	// start the train from the first corner
	npcreached(ent);

	// and make it wait for activation
	//ent->s.pos.trType = TR_STATIONARY;
	//ent->use = npcuse;
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
	trap_LinkEntity(self);

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
}

static void
npc_pain(ent_t *e, ent_t *attacker, int dmg)
{
	gprintf("pain\n");
}

static void
npc_die(ent_t *e, ent_t *inflictor, ent_t *attacker, int dmg, int mod)
{
	gprintf("die\n");
	trap_UnlinkEntity(e);
}

void
SP_npc_test(ent_t *e)
{
	npcsetup(e);

	e->model = "models/npc/test";
	e->s.modelindex = modelindex(e->model);
	vecset(e->r.mins, -32, -32, -64);
	vecset(e->r.maxs, 32, 32, 64);
	setorigin(e, e->s.origin);
	e->health = 1;
	e->takedmg = qtrue;

	e->die = npc_die;
	e->pain = npc_pain;

	e->r.contents = CONTENTS_SOLID | CONTENTS_NPCCLIP;

	trap_LinkEntity(e);
}
