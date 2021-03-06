#include "g_local.h"
#include "bg_local.h"

static vec3_t cratemins = {-16, -16, 0};
static vec3_t cratemaxs = {16, 16, 32};
#define BOX_CONTENTS_SPEED	150.0f
#define BOX_CONTENTS_JUMP	100.f

// g_trigger.c jump pad recycling
extern void	trigger_push_touch(ent_t*, ent_t*, trace_t*);
extern void	AimAtTarget(ent_t*);

static void	crate_use(ent_t*, ent_t*, ent_t*);
static void	crate_touch(ent_t*, ent_t*, trace_t*);
static void	crate_levelrespawn(ent_t*);
static void	crate_checkpoint_use(ent_t*, ent_t*, ent_t*);
static void	crate_bouncy_touch(ent_t*, ent_t*, trace_t*);
static void	crate_tnt_touch(ent_t*, ent_t*, trace_t*);
static void	crate_tnt_pain(ent_t*, ent_t*, int);
static void	crate_tnt_die(ent_t*, ent_t*, ent_t*, int, int);
static void	crate_tnt_use(ent_t*, ent_t*, ent_t*);
static void	crate_tnt_levelrespawn(ent_t*);
static void	crate_tnt_explode(ent_t*);
static void	SP_checkpoint_halo(ent_t*);

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
	spawnint("count", "1", &ent->count);

	ent->model = "models/crates/crate.md3";
	ent->physbounce = 0.2;
	ent->touch = crate_touch;
	ent->use = crate_use;
	ent->levelrespawn = crate_levelrespawn;
	ent->nextthink = -1;
	ent->takedmg = qtrue;
	ent->s.eType = ET_CRATE;
	ent->s.modelindex = modelindex(ent->model);
	setorigin(ent, ent->s.origin);
	veccpy(ent->s.angles, ent->s.apos.trBase);
	ent->r.contents = CONTENTS_SOLID | CONTENTS_TRIGGER;
	veccpy(cratemins, ent->r.mins);
	veccpy(cratemaxs, ent->r.maxs);
	ent->ckpoint = level.checkpoint;
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
	veccpy(cratemins, ent->r.mins);
	veccpy(cratemaxs, ent->r.maxs);
	trap_LinkEntity(ent);
}

/*
Recycles ent and transforms it into a checkpoint halo.
*/
static void
SP_checkpoint_halo(ent_t *ent)
{
	ent->classname = "ckpointhalo";
	ent->model = "models/mapobjects/ckpoint/ckpoint";
	ent->s.modelindex = modelindex(ent->model);
	ent->s.eType = ET_CHECKPOINTHALO;
	ent->use = nil;
	ent->touch = nil;
	ent->takedmg = qfalse;
	ent->r.contents = 0;
	// float off ground a little
	ent->s.origin[2] += 2.0f;
	setorigin(ent, ent->s.origin);
	ent->s.pos.trType = TR_SINE;
	ent->s.pos.trDelta[2] = 1.0f;
	ent->s.pos.trDuration = 4000;
	ent->s.pos.trTime = level.time;
	trap_LinkEntity(ent);
}

/*
An indestructible crate which acts just like a jump pad.

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
	veccpy(cratemins, ent->r.mins);
	veccpy(cratemaxs, ent->r.maxs);
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

/*
A breakable crate that explodes after a short delay if jumped on, or
immediately if hit.

SUSPENDED	no drop to floor
*/
void
SP_crate_tnt(ent_t *ent)
{
	setorigin(ent, ent->s.origin);
	veccpy(cratemins, ent->r.mins);
	veccpy(cratemaxs, ent->r.maxs);
	ent->model = "models/crates/tnt/tnt.md3";
	ent->s.modelindex = modelindex(ent->model);
	ent->physbounce = 0.2f;
	ent->touch = crate_tnt_touch;
	ent->use = crate_tnt_use;
	ent->pain = crate_tnt_pain;
	ent->die = crate_tnt_die;
	ent->ckpoint = level.checkpoint;
	ent->levelrespawn = crate_tnt_levelrespawn;
	ent->health = 1;
	ent->takedmg = qtrue;
	ent->s.eType = ET_CRATE;
	veccpy(ent->s.angles, ent->s.apos.trBase);
	ent->r.contents = CONTENTS_SOLID | CONTENTS_TRIGGER;

	ent->r.ownerNum = ent->s.number;
	ent->parent = ent;
	ent->damage = 1;
	ent->splashdmg = 1;
	ent->splashradius = 120;
	ent->meansofdeath = MOD_TNT;
	ent->splashmeansofdeath = MOD_TNT;
	ent->clipmask = MASK_SHOT;

	trap_LinkEntity(ent);
}

static void
crate_use(ent_t *self, ent_t *other, ent_t *activator)
{
	ent_t *tent;
	vec3_t pt, vel;
	int i, it;

	self->classname = "smashedcrate";
	self->s.eType = ET_EVENTS + EV_SMASH_CRATE;
	self->eventtime = level.time;
	self->s.modelindex = 0;
	self->r.contents = 0;

	vecadd(self->r.absmin, self->r.absmax, pt);
	vecmul(pt, 0.5f, pt);

	if(self->boxcontents != 0){
		it = self->boxcontents;
		for(i = 0; i < self->count; i++){
			vel[0] = crandom()*BOX_CONTENTS_SPEED;
			vel[1] = crandom()*BOX_CONTENTS_SPEED;
			vel[2] = BOX_CONTENTS_JUMP + crandom()*BOX_CONTENTS_SPEED;
			itemlaunch(&bg_itemlist[it], pt, vel);
		}
	}
	usetargets(self, activator);
	trap_LinkEntity(self);
	level.ncratesbroken++;
	self->ckpoint = level.checkpoint;
}

/*
Unlike crate_use, this function doesn't make an EV_SMASH_CRATE event
because the call to touchcrate already predicts one.
*/
static void
crate_touch(ent_t *self, ent_t *other, trace_t *trace)
{
	if(other->client == nil)
		return;
	if(!touchcrate(&other->client->ps, &self->s))
		return;
	self->use(self, nil, other);
}

static void
crate_levelrespawn(ent_t *self)
{
	level.ncratesbroken--;
	restoreinitialstate(self);
}

static void
crate_checkpoint_use(ent_t *self, ent_t *other, ent_t *activator)
{
	ent_t *tent;

	tent = enttemp(self->s.pos.trBase, EV_SMASH_CRATE);
	tent->s.otherEntityNum = activator->s.number;

	level.checkpoint = self->s.number;
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
	if(other->client->ps.velocity[2] > 0)
		return;
	trigger_push_touch(self, other, trace);
	self->s.anim = ANIM_CRATESMASH;
	self->s.nextanim = ANIM_CRATEIDLE;
	self->s.nextanimtime = level.time + 300;
}

static void
crate_tnt_pain(ent_t *e, ent_t *attacker, int dmg)
{
	e->ckpoint = level.checkpoint;
	crate_tnt_explode(e);
}

static void
crate_tnt_use(ent_t *e, ent_t *other, ent_t *activator)
{
	e->ckpoint = level.checkpoint;
	crate_tnt_explode(e);
}

static void
crate_tnt_die(ent_t *e, ent_t *inflictor, ent_t *attacker, int dmg, int mod)
{
	e->ckpoint = level.checkpoint;
	crate_tnt_explode(e);
}

static void
crate_tnt_levelrespawn(ent_t *self)
{
	restoreinitialstate(self);
}

static void
crate_tnt_explode(ent_t *e)
{
	vec3_t pt;

	vecadd(e->r.absmin, e->r.absmax, pt);
	vecmul(pt, 0.5f, pt);
	trap_UnlinkEntity(e);
	radiusdamage(e->r.currentOrigin, e, e->splashdmg,
	   e->splashradius, e, e->splashmeansofdeath);
	setorigin(e, pt);
	e->classname = "explodedtnt";
	e->s.eType = ET_EVENTS + EV_TNT_EXPLODE;
	e->eventtime = level.time;
	e->r.contents = 0;
	trap_LinkEntity(e);
}

static void
crate_tnt_touch(ent_t *self, ent_t *other, trace_t *trace)
{
	if(other->client == nil)
		return;
	if(other->s.groundEntityNum != self->s.number)
		return;
	if(!touchcrate(&other->client->ps, &self->s))
		return;
	self->touch = nil;
	self->r.contents = CONTENTS_SOLID;
	self->ckpoint = level.checkpoint;
	other->client->ps.velocity[2] = JUMP_VELOCITY;
	self->nextthink = level.time + 632;
	self->think = crate_tnt_explode;
	self->s.anim = ANIM_CRATESMASH;
	self->s.nextanim = ANIM_CRATEIDLE;
	self->s.nextanimtime = level.time + 632;
	mksound(other, CHAN_AUTO, soundindex("sound/misc/tntfuse.wav"));
}
