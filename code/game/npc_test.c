#include "g_local.h"

static int stest;

static npcframe_t standframes[] = {
	aistand, 0, nil
};

static npcframe_t walkframes[] = {
	aiwalk, 14, nil,
	aiwalk, 13, nil
};

static npcmove_t movestand = {0, 1, standframes, nil};
static npcmove_t movewalk = {0, 1, walkframes, nil};

static void
test_idle(ent_t *e)
{
	//mksound(e, CHAN_AUTO, stest);
}

static void
test_sight(ent_t *e, ent_t *other)
{
	gprintf("test_sight\n");
}

static void
test_search(ent_t *e)
{
	gprintf("test_search\n");
	mksound(e, CHAN_AUTO, stest);
}

static void
test_stand(ent_t *e)
{
	gprintf("test_stand\n");
	mksound(e, CHAN_AUTO, stest);
	e->npc.mv = &movestand;
}

static void
test_walk(ent_t *e)
{
	gprintf("test_walk\n");
	mksound(e, CHAN_AUTO, stest);
	e->npc.mv = &movewalk;
}
static void
test_run(ent_t *e)
{
}

static void
test_melee(ent_t *e)
{
}

static void
test_attack(ent_t *e)
{
	gprintf("test_attack\n");
}

static void
test_pain(ent_t *e, ent_t *attacker, int dmg)
{
	mksound(e, CHAN_AUTO, stest);
	e->paindebouncetime = level.time + 3;
}

static void
test_dead(ent_t *e)
{
}

static void
test_die(ent_t *e, ent_t *inflictor, ent_t *attacker, int dmg, int mod)
{
	mksound(e, CHAN_AUTO, stest);
	entfree(e);
}

void
SP_npc_test(ent_t *e)
{
	stest = soundindex("sound/npc/test");

	//e->solid = SOLID_BBOX;
	e->npc.scale = 1;
	e->takedmg = qtrue;
	e->model = "models/npc/test";
	e->s.modelindex = modelindex(e->model);
	vecset(e->r.mins, -32, -32, -64);
	vecset(e->r.maxs, 32, 32, 64);
	setorigin(e, e->s.origin);
	e->health = 1;

	e->pain = test_pain;
	e->die = test_die;

	e->npc.stand = test_stand;
	e->npc.walk = test_walk;
	e->npc.run = test_run;
	e->npc.attack = test_attack;
	e->npc.melee = test_melee;
	e->npc.sight = test_sight;
	e->npc.idle = test_idle;
	e->npc.search = test_search;

	e->r.contents = CONTENTS_SOLID | CONTENTS_NPCCLIP;
	e->npc.mv = &movestand;

	trap_LinkEntity(e);

	npcstart(e);
}
