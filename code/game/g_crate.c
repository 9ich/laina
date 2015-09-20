#include "g_local.h"

#define BOX_CONTENTS_SPEED	200.0f
#define BOX_CONTENTS_JUMP	100.f

// g_trigger.c jump pad recycling
extern void	trigger_push_touch(ent_t *self, ent_t *other, trace_t *trace);
extern void	AimAtTarget(ent_t *self);

static void	crate_use(ent_t*, ent_t*, ent_t*);
static void	crate_touch(ent_t*, ent_t*, trace_t*);
static void	crate_checkpoint_use(ent_t*, ent_t*, ent_t*);
static void	crate_bouncy_touch(ent_t*, ent_t*, trace_t*);
static void	SP_checkpoint_halo(ent_t *ent);

/*
A breakable crate which usually contains items.
Breaks open when damaged or jumped on.

SUSPENDED	no drop to floor

"contents"	what's in the box? (item_token default)
"count"		number of items in box (5 default)
"target"	target ents to trigger when the box is smashed
"wait"		time before respawning (-1 default, -1 = never respawn)
*/
void
SP_crate(ent_t *ent)
{
	char *contents;
	int i;

	// prepare the crate contents
	spawnstr("contents", "item_token", &contents);
	for(i = 0; i < bg_nitems; i++)
		if(strcmp(bg_itemlist[i].classname, contents) == 0){
			ent->boxcontents = i;
			break;
		}
	if(ent->boxcontents < 1)
		gprintf(S_COLOR_YELLOW "WARNING: bad item %s in crate\n", contents);
	else
		registeritem(&bg_itemlist[ent->boxcontents]);
	spawnint("count", "5", &ent->count);

	ent->model = "models/crates/crate.md3";
	ent->physbounce = 0.2;
	ent->touch = crate_touch;
	ent->use = crate_use;
	ent->nextthink = -1;
	ent->takedmg = qtrue;
	ent->s.eType = ET_CRATE;
	ent->s.modelindex = modelindex(ent->model);
	setorigin(ent, ent->s.origin);
	veccpy(ent->s.angles, ent->s.apos.trBase);
	ent->r.contents = CONTENTS_SOLID | CONTENTS_TRIGGER;
	vecset(ent->r.mins, -16, -16, -16);
	vecset(ent->r.maxs, 16, 16, 16);
	level.ncrates++;
	trap_LinkEntity(ent);
}

/*
A breakable crate that acts as a checkpoint for all players after
being broken.

SUSPENDED	no drop to floor

"target"	target ents to trigger when the box is smashed
*/
void
SP_crate_checkpoint(ent_t *ent)
{
	ent->model = "models/crates/checkpoint.md3";
	ent->physbounce = 0.2;
	ent->use = crate_checkpoint_use;
	ent->touch = crate_touch;
	ent->nextthink = -1;
	ent->takedmg = qtrue;
	ent->s.eType = ET_CRATE;
	ent->s.modelindex = modelindex(ent->model);
	setorigin(ent, ent->s.origin);
	veccpy(ent->s.angles, ent->s.apos.trBase);
	ent->r.contents = CONTENTS_SOLID | CONTENTS_TRIGGER;
	vecset(ent->r.mins, -16, -16, -16);
	vecset(ent->r.maxs, 16, 16, 16);
	trap_LinkEntity(ent);
}

/*
Recycles ent and transforms it into a checkpoint halo.
*/
static void
SP_checkpoint_halo(ent_t *ent)
{
	ent->model = "models/mapobjects/ckpoint/ckpoint";
	ent->s.modelindex = modelindex(ent->model);
	ent->s.eType = ET_CHECKPOINTHALO;
	ent->use = nil;
	ent->touch = nil;
	ent->takedmg = qfalse;
	ent->r.contents = 0;
	// reposition on ground
	ent->s.origin[2] += ent->r.mins[2];
	setorigin(ent, ent->s.origin);
	trap_LinkEntity(ent);
}

/*
An indestructible box which acts just like a jump pad.

SUSPENDED	no drop to floor
"target"	a target_position, which will be the apex of the leap
*/
void
SP_crate_bouncy(ent_t *ent)
{
	ent->r.svFlags &= ~SVF_NOCLIENT;
	// make sure the client precaches this sound
	soundindex("sound/world/jumppad.wav");
	setorigin(ent, ent->s.origin);
	vecset(ent->r.mins, -16, -16, -16);
	vecset(ent->r.maxs, 16, 16, 16);
	ent->model = "models/crates/bouncy/bouncy.md3";
	ent->s.modelindex = modelindex(ent->model);
	ent->physbounce = 0.2;
	ent->touch = crate_bouncy_touch;
	ent->think = AimAtTarget;
	ent->nextthink = level.time + FRAMETIME;
	ent->takedmg = qfalse;
	ent->s.eType = ET_CRATE_BOUNCY;
	veccpy(ent->s.angles, ent->s.apos.trBase);
	ent->r.contents = CONTENTS_SOLID | CONTENTS_TRIGGER;
	trap_LinkEntity(ent);
}

static void
crate_use(ent_t *self, ent_t *other, ent_t *activator)
{
	ent_t *tent;
	vec3_t vel;
	int i, it;

	if(self->boxcontents != 0){
		it = self->boxcontents;
		for(i = 0; i < self->count; i++){
			vel[0] = crandom()*BOX_CONTENTS_SPEED;
			vel[1] = crandom()*BOX_CONTENTS_SPEED;
			vel[2] = BOX_CONTENTS_JUMP + crandom()*BOX_CONTENTS_SPEED;
			itemlaunch(&bg_itemlist[it], self->s.pos.trBase, vel);
		}
	}
	tent = enttemp(self->s.pos.trBase, EV_SMASH_CRATE);
	tent->s.otherEntityNum = activator->s.number;

	usetargets(self, activator);

	// disable collision, free in a moment
	self->r.contents = 0;
	self->think = entfree;
	self->nextthink = 50;

	level.ncratesbroken++;
}

/*
Unlike crate_use, this function doesn't make an EV_SMASH_CRATE event
because the call to touchcrate already predicts one.
*/
static void
crate_touch(ent_t *self, ent_t *other, trace_t *trace)
{
	vec3_t vel;
	int i, it;

	if(other->client == nil)
		return;
	if(!touchcrate(&other->client->ps, &self->s))
		return;

	if(self->boxcontents != 0){
		it = self->boxcontents;
		for(i = 0; i < self->count; i++){
			vel[0] = crandom()*BOX_CONTENTS_SPEED;
			vel[1] = crandom()*BOX_CONTENTS_SPEED;
			vel[2] = BOX_CONTENTS_JUMP + crandom()*BOX_CONTENTS_SPEED;
			itemlaunch(&bg_itemlist[it], self->s.pos.trBase, vel);
		}
	}

	usetargets(self, other);

	// disable collision, free in a moment
	self->r.contents = 0;
	self->think = entfree;
	self->nextthink = 50;

	level.ncratesbroken++;
}

static void
crate_checkpoint_use(ent_t *self, ent_t *other, ent_t *activator)
{
	ent_t *tent;

	level.checkpoint = self->s.number;
	tent = enttemp(self->s.pos.trBase, EV_SMASH_CRATE);
	tent->s.otherEntityNum = activator->s.number;
	usetargets(self, activator);
	trap_UnlinkEntity(self);
	SP_checkpoint_halo(self);
}

static void
crate_bouncy_touch(ent_t *self, ent_t *other, trace_t *trace)
{
	if(other->client == nil)
		return;
	if(other->s.groundEntityNum != self->s.number)
		return;
	trigger_push_touch(self, other, trace);
	self->s.nextanim = ANIM_CRATEIDLE;
	self->s.nextanimtime = level.time + 300;
	self->s.anim = ANIM_CRATESMASH;
}
