/*
 * Circular buffer of points where the player has been recently.  It
 * is used by NPCs for pursuit.
 */

#include "g_local.h"

enum { Ntrail	= 8 };

static ent_t *trail[Ntrail];
static int head;

#define next(n)	(((n)+1) % Ntrail)
#define prev(n)	(((n)-1) % Ntrail)

void
trailinit(void)
{
	int i;

	for(i = 0; i < Ntrail; i++){
		trail[i] = entspawn();
		trail[i]->classname = "player_trail";
	}
	head = 0;
}

void
trailadd(vec3_t spot)
{
	vec3_t tmp;

	veccopy(spot, trail[head]->s.pos.trBase);
	trail[head]->timestamp = level.time;

	vecsub(spot, trail[prev(head)]->s.pos.trBase, tmp);
	trail[head]->s.angles[YAW] = vectoyaw(tmp);

	head = next(head);
}

ent_t*
trailfirst(ent_t *e)
{
	int i, n;

	for(i = head, n = 0; n < Ntrail; n++, i = next(i)){
		if(trail[i]->timestamp > e->npc.trailtime)
			break;
	}

	if(visible(e, trail[i]))
		return trail[i];

	if(visible(e, trail[prev(i)]))
		return trail[prev(i)];

	return trail[i];
}

ent_t*
trailnext(ent_t *e)
{
	int i, n;

	for(i = head, n = 0; n < Ntrail; n++, i = next(i)){
		if(trail[i]->timestamp > e->npc.trailtime)
			break;
	}
	return trail[i];
}

ent_t*
traillastspot(void)
{
	return trail[prev(head)];
}
