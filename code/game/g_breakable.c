#include "g_local.h"

static void breakable_box_use(gentity_t *self, gentity_t *other, gentity_t *activator);
static void breakable_box_touch(gentity_t *self, gentity_t *other, trace_t *trace);

/*
A breakable box which usually contains an item.
Breaks open when damaged or jumped on.

SUSPENDED	no drop to floor

"contents"	what's in the box? (item_token default)
"count"		number of items in box (5 default)
"target"		smashing this box will trigger the entity this points to
"wait"		time before respawning (-1 default, -1 = never respawn)
*/
void SP_breakable_box(gentity_t *ent)
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

	ent->model = "models/breakables/box.md3";
	ent->physicsBounce = 0.2;
	ent->use = breakable_box_use;
	ent->touch = breakable_box_touch;
	ent->nextthink = -1;
	ent->takedamage = qtrue;
	ent->s.eType = ET_BREAKABLE;
	ent->s.modelindex = G_ModelIndex(ent->model);
	G_SetOrigin(ent, ent->s.origin);
	VectorCopy(ent->s.angles, ent->s.apos.trBase);
	ent->r.contents = CONTENTS_SOLID | CONTENTS_TRIGGER;
	VectorSet(ent->r.mins, -16, -16, -16);
	VectorSet(ent->r.maxs, 16, 16, 16);
	trap_LinkEntity(ent);
}

#define BOX_CONTENTS_SPEED 200.0f
#define BOX_CONTENTS_JUMP 100.f

static void breakable_box_use(gentity_t *self, gentity_t *other, gentity_t *activator)
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
	tent = G_TempEntity(self->s.pos.trBase, EV_SMASH_BOX);
	tent->s.otherEntityNum = activator->s.number;
	trap_UnlinkEntity(self);
	G_FreeEntity(self);
}

static void breakable_box_touch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	if(other->client == NULL)
		return;
	if(other->s.groundEntityNum != self->s.number)
		return;
	BG_Squish(&other->client->ps, &self->s);
	self->use(self, NULL, other);
}
