#include "g_local.h"

#define BOX_CONTENTS_SPEED 200.0f
#define BOX_CONTENTS_JUMP 100.f

// g_trigger.c jump pad recycling
extern void trigger_push_touch(gentity_t *self, gentity_t *other, trace_t *trace);
extern void AimAtTarget(gentity_t *self);

static void crate_use(gentity_t*, gentity_t*, gentity_t*);
static void crate_touch(gentity_t*, gentity_t*, trace_t*);
static void crate_checkpoint_use(gentity_t*, gentity_t*, gentity_t*);
static void crate_checkpoint_touch(gentity_t*, gentity_t*, trace_t*);
static void crate_bouncy_touch(gentity_t*, gentity_t*, trace_t*);

/*
A breakable crate which usually contains items.
Breaks open when damaged or jumped on.

SUSPENDED	no drop to floor

"contents"	what's in the box? (item_token default)
"count"		number of items in box (5 default)
"target"	target ents to trigger when the box is smashed
"wait"		time before respawning (-1 default, -1 = never respawn)
*/
void SP_crate(gentity_t *ent)
{
	char *contents;
	gitem_t *item;

	// prepare the contents as a gitem_t*
	G_SpawnString("contents", "item_token", &contents);
	for(item=bg_itemlist+1 ; item->classname ; item++){
		if(strcmp(item->classname, contents) == 0){
			ent->boxcontents = item;
			break;
		}
	}
	if(ent->boxcontents == NULL)
		G_Printf(S_COLOR_YELLOW "WARNING: bad item %s in breakable_box\n", contents);
	G_SpawnInt("count", "5", &ent->count);

	ent->model = "models/crates/crate.md3";
	ent->physicsBounce = 0.2;
	ent->use = crate_use;
	ent->touch = crate_touch;
	ent->nextthink = -1;
	ent->takedamage = qtrue;
	ent->s.eType = ET_CRATE;
	ent->s.modelindex = G_ModelIndex(ent->model);
	G_SetOrigin(ent, ent->s.origin);
	VectorCopy(ent->s.angles, ent->s.apos.trBase);
	ent->r.contents = CONTENTS_SOLID | CONTENTS_TRIGGER;
	VectorSet(ent->r.mins, -16, -16, -16);
	VectorSet(ent->r.maxs, 16, 16, 16);
	trap_LinkEntity(ent);
}

/*
A breakable crate that acts as a checkpoint for all players after
being broken.

SUSPENDED	no drop to floor

"target"	target ents to trigger when the box is smashed
*/
void SP_crate_checkpoint(gentity_t *ent)
{
	ent->model = "models/crates/checkpoint.md3";
	ent->physicsBounce = 0.2;
	ent->use = crate_checkpoint_use;
	ent->touch = crate_checkpoint_touch;
	ent->nextthink = -1;
	ent->takedamage = qtrue;
	ent->s.eType = ET_CRATE;
	ent->s.modelindex = G_ModelIndex(ent->model);
	G_SetOrigin(ent, ent->s.origin);
	VectorCopy(ent->s.angles, ent->s.apos.trBase);
	ent->r.contents = CONTENTS_SOLID | CONTENTS_TRIGGER;
	VectorSet(ent->r.mins, -16, -16, -16);
	VectorSet(ent->r.maxs, 16, 16, 16);
	trap_LinkEntity(ent);
}

/*
An indestructible box which acts just like a jump pad.

SUSPENDED	no drop to floor
"target"	a target_position, which will be the apex of the leap
*/
void SP_crate_bouncy(gentity_t *ent)
{
	ent->r.svFlags &= ~SVF_NOCLIENT;
	// make sure the client precaches this sound
	G_SoundIndex("sound/world/jumppad.wav");
	G_SetOrigin(ent, ent->s.origin);
	VectorSet(ent->r.mins, -16, -16, -16);
	VectorSet(ent->r.maxs, 16, 16, 16);
	ent->model = "models/crates/bouncy.md3";
	ent->s.modelindex = G_ModelIndex(ent->model);
	ent->physicsBounce = 0.2;
	ent->touch = crate_bouncy_touch;
	ent->think = AimAtTarget;
	ent->nextthink = level.time + FRAMETIME;
	ent->takedamage = qfalse;
	ent->s.eType = ET_CRATE_BOUNCY;
	VectorCopy(ent->s.angles, ent->s.apos.trBase);
	ent->r.contents = CONTENTS_SOLID | CONTENTS_TRIGGER;
	trap_LinkEntity(ent);
}

static void crate_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	gentity_t *tent;
	vec3_t vel;
	int i;
	
	if(self->boxcontents == NULL)
		return;
	for(i = 0; i < self->count; i++){
		vel[0] = crandom()*BOX_CONTENTS_SPEED;
		vel[1] = crandom()*BOX_CONTENTS_SPEED;
		vel[2] = BOX_CONTENTS_JUMP + crandom()*BOX_CONTENTS_SPEED;
		LaunchItem(self->boxcontents, self->s.pos.trBase, vel);
	}
	tent = G_TempEntity(self->s.pos.trBase, EV_SMASH_CRATE);
	tent->s.otherEntityNum = activator->s.number;
	G_UseTargets(self, activator);
	trap_UnlinkEntity(self);
	G_FreeEntity(self);
}

static void crate_touch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	if(other->client == NULL)
		return;
	if(other->s.groundEntityNum != self->s.number)
		return;
	BG_Squish(&other->client->ps, &self->s);
	self->use(self, NULL, other);
}

static void crate_checkpoint_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	gentity_t *tent;

	level.checkpoint = self->s.number;
	tent = G_TempEntity(self->s.pos.trBase, EV_SMASH_CRATE);
	tent->s.otherEntityNum = activator->s.number;
	G_UseTargets(self, activator);
	trap_UnlinkEntity(self);
}

static void crate_checkpoint_touch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	if(other->client == NULL)
		return;
	if(other->s.groundEntityNum != self->s.number)
		return;
	BG_Squish(&other->client->ps, &self->s);
	self->use(self, NULL, other);
}

static void crate_bouncy_touch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	if(other->client == NULL)
		return;
	if(other->s.groundEntityNum != self->s.number)
		return;
	trigger_push_touch(self, other, trace);
}