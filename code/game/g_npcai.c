/*
===========================================================================
Copyright (C) 1997-2001 Id Software, Inc.

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

void ainoprint(char *fmt, ...) { (void)fmt; }
#define dprint ainoprint

static int	range(ent_t *e, ent_t *other);
static void	checkcourse(ent_t *e, vec3_t v);
static void	printaiflags(int f);
static qboolean	findtarget(ent_t *e);
static qboolean	aicheckattack(ent_t *e, float dist);

/*
Move the specified distance at current facing.
*/
void
aimove(ent_t *e, float dist)
{
	npcwalkmove(e, e->npc.angles[YAW], dist);
}

/*
Used for standing around and looking for players.
vecdist is for slight position adjustments needed by the animations.
*/
void
aistand(ent_t *e, float dist)
{
	vec3_t v;

	dprint("aistand\n");
	printaiflags(e->npc.aiflags);

	if(dist){
		npcwalkmove(e, e->npc.angles[YAW], dist);
	}

	if(e->npc.aiflags & AI_STANDGROUND){
		if(e->enemy == nil){
			findtarget(e);
			return;
		}
		vecsub(e->npc.enemy->s.pos.trBase, e->s.pos.trBase, v);
		e->npc.idealyaw = vectoyaw(v);
		if(e->s.apos.trBase[YAW] != e->npc.idealyaw &&
		   (e->npc.aiflags & AI_TEMPSTANDGROUND)){
			e->npc.aiflags &= ~(AI_STANDGROUND|AI_TEMPSTANDGROUND);
			e->npc.run(e);
		}
		npcchangeyaw(e);
		aicheckattack(e, 0);
		return;
	}

	if(findtarget(e)){
		return;
	}
	if(level.time > e->npc.pausetime){
		e->npc.walk(e);
		return;
	}
	if(!(e->spawnflags & 1) && e->npc.idle != nil &&
	   level.time > e->npc.idletime){
		if(!e->npc.idletime){
			e->npc.idletime = level.time + random()*1500;
			return;
		}
		e->npc.idle(e);
		e->npc.idletime = level.time + 1500 + random()*1500;
	}
}

/*
The NPC is walking its beat.
*/
void
aiwalk(ent_t *e, float dist)
{
	dprint("aiwalk\n");
	printaiflags(e->npc.aiflags);

	npcmovetogoal(e, dist);
	printaiflags(e->npc.aiflags);
	// did it notice a player?
	if(findtarget(e)){
		return;
	}
	if(e->npc.search != nil && level.time > e->npc.idletime){
		if(!e->npc.idletime){
			e->npc.idletime = level.time + random()*1500;
			return;
		}
		e->npc.search(e);
		e->npc.idletime = level.time + 1500 + random()*1500;
	}
}

/*
Turns towards target and advances.
*/
void
aicharge(ent_t *e, float dist)
{
	vec3_t v;

	dprint("aicharge\n");

	vecsub(e->npc.enemy->s.pos.trBase, e->s.pos.trBase, v);
	gprintf("aicharge %s ---> %s = %s\n", vtos(e->npc.enemy->s.pos.trBase),
	   vtos(e->s.pos.trBase), vtos(v));
	e->npc.idealyaw = vectoyaw(v);
	npcchangeyaw(e);
	if(dist > 0){
		npcwalkmove(e, e->npc.angles[YAW], dist);
	}
}

/*
Don't move, but turn towards idealyaw.
vecdist is for slight position adjustments needed by the animations.
*/
void
aiturn(ent_t *e, float dist)
{
	dprint("aiturn\n");

	if(dist > 0){
		npcwalkmove(e, e->npc.angles[YAW], dist);
	}
	if(findtarget(e)){
		return;
	}
	npcchangeyaw(e);
}
		
/*
This is called once per game frame to set level.sightclient for NPCs to hunt.

In coop games, level.sightclient will cycle between the clients.
*/
void
setsightclient(void)
{
	ent_t *ent;
	int start, i;

	if(level.sightclient != nil)
		start = level.sightclient - g_entities;
	else
		start = 0;

	for(i = start;;){
		i++;
		if(i >= level.maxclients)
			i = 0;
		ent = &g_entities[i];
		if(ent->inuse && !(ent->flags & FL_NOTARGET)){
			level.sightclient = ent;
			return;
		}
		if(i == start){
			level.sightclient = nil;
			return;
		}
	}
}

qboolean
visible(ent_t *e, ent_t *other)
{
	vec3_t a, b;
	trace_t tr;

	veccopy(e->s.pos.trBase, a);
	a[2] += e->npc.viewheight;
	veccopy(other->s.pos.trBase, b);
	b[2] += CROUCH_VIEWHEIGHT;
	trap_Trace(&tr, a, vec3_origin, vec3_origin, b, e->s.number, MASK_SOLID);
	return tr.fraction == 1.0f;
}

qboolean
infront(ent_t *e, ent_t *other)
{
	vec3_t v, forward;

	dprint("infront\n");

	anglevecs(e->s.apos.trBase, forward, nil, nil);
	vecsub(other->s.pos.trBase, e->s.pos.trBase, v);
	vecnorm(v);
	return vecdot(v, forward) > 0.3f;
}

void
hunttarget(ent_t *e)
{
	vec3_t v;

	dprint("hunttarget\n");

	if(e->npc.enemy == nil)
		return;

	e->npc.goalent = e->npc.enemy;
	if(e->npc.aiflags & AI_STANDGROUND)
		e->npc.stand(e);
	else
		e->npc.run(e);
	vecsub(e->npc.enemy->s.pos.trBase, e->s.pos.trBase, v);
	dprint("hunttarget %s ---> %s = %s\n", vtos(e->npc.enemy->s.pos.trBase),
			vtos(e->s.pos.trBase), vtos(v));
	e->npc.idealyaw = vectoyaw(v);
	/*
	if(!(e->npc.aiflags & AI_STANDGROUND))
		attackfinished(e, 1);
	*/
}

void
foundtarget(ent_t *e)
{
	dprint("foundtarget\n");

	if(e->npc.enemy->client != nil){
		level.sightent = e;
		level.sightentframenum = level.framenum;
	}

	e->npc.showhostile = level.time + 10;

	veccopy(e->npc.enemy->s.pos.trBase, e->npc.lastsighting);
	e->npc.trailtime = level.time;

	if(e->npc.combattarg == nil){
		hunttarget(e);
		return;
	}

	e->npc.goalent = e->npc.movetarg = picktarget(e->npc.combattarg);
	if(e->npc.movetarg == nil){
		e->npc.goalent = e->npc.movetarg = e->npc.enemy;
		hunttarget(e);
		gprintf("%s at %s, combattarg %s not found\n",
		   e->classname, vtos(e->s.pos.trBase), e->npc.combattarg);
		return;
	}

	e->npc.combattarg = nil;
	e->npc.aiflags |= AI_COMBATPOINT;

	e->npc.movetarg->targetname = nil;
	e->npc.pausetime = 0;

	e->npc.run(e);
}

static qboolean
findtarget(ent_t *e)
{
	ent_t *cl;
	qboolean heard;
	int r;

	dprint("findtarget\n");

	if(e->npc.aiflags & AI_GOODGUY)
		return qfalse;
	if(e->npc.aiflags & AI_COMBATPOINT)
		return qfalse;
	// if the first spawnflag bit is set, the monster will only
	// wake up on really seeing the player, not another monster
	// getting angry or hearing something
	heard = qfalse;
	if(level.sightentframenum >= level.framenum-1 &&
	   !(e->spawnflags & 1)){
		cl = level.sightent;
		if(cl->enemy == e->npc.enemy){
			gprintf("cl->enemy == e->npc.enemy\n");
			return qfalse;
		}
	}else if(level.soundentframe >= level.framenum-1){
		cl = level.soundent;
		heard = qtrue;
	}else{
		cl = level.sightclient;
		if(cl == nil)
			return qfalse;	// no clients to get mad at
	}

	if(!cl->inuse)
		return qfalse;

	if(cl == e->npc.enemy)
		return qtrue;

	if(cl->client != nil){
		if((cl->flags & FL_NOTARGET))
			return qfalse;
	}else if(cl->r.svFlags & SVF_NPC){
		if(cl->enemy == nil || (cl->enemy->flags & FL_NOTARGET))
			return qfalse;
	}else if(heard){
		if(g_entities[cl->r.ownerNum].flags & FL_NOTARGET)
			return qfalse;
	}else{
		return qfalse;
	}

	printaiflags(e->npc.aiflags);
	if(heard){
		vec3_t tmp;
	
		if(!visible(e, cl)){
			gprintf("heard but no line of sight\n");
			return qfalse;
		}

		vecsub(cl->s.pos.trBase, e->s.pos.trBase, tmp);
		dprint("%s ---> %s = %s\n", vtos(cl->s.pos.trBase),
		   vtos(e->s.pos.trBase), vtos(tmp));
		if(veclen(tmp) > 1000)
			return qfalse;

		e->npc.idealyaw = vectoyaw(tmp);
		npcchangeyaw(e);

		e->npc.aiflags |= AI_SOUNDTARGET;
		e->npc.enemy = cl;
	}else{
		vec3_t tmp;

		vecsub(cl->s.pos.trBase, e->s.pos.trBase, tmp);
		dprint("%s ---> %s = %s\n", vtos(cl->s.pos.trBase),
		   vtos(e->s.pos.trBase), vtos(tmp));
		e->npc.idealyaw = vectoyaw(tmp);
		npcchangeyaw(e);

		// !heard
		r = range(e, cl);
		if(r == RANGE_FAR)
			return qfalse;
		if(!visible(e, cl))
			return qfalse;
		if(r == RANGE_NEAR){
			if(cl->npc.showhostile < level.time && !infront(e, cl))
				return qfalse;
		}else if(r == RANGE_MID){
			if(!infront(e, cl))
				return qfalse;
		}

		e->enemy = cl;

		// FIXME: players should drop player_noise entities as in q2
		/*
		if(e->enemy->s.eFlags != EF_PLAYER_EVENT && e->enemy->s.eType < ET_EVENTS){
			e->npc.aiflags &= ~AI_SOUNDTARGET;
			if(e->enemy->client == nil){
				e->enemy = e->enemy->enemy;
				if(e->enemy->client == nil){
					e->enemy = nil;
					return qfalse;
				}
			}
		}
		*/
	}
	
	foundtarget(e);
	if(!(e->npc.aiflags & AI_SOUNDTARGET) && (e->npc.sight != nil))
		e->npc.sight(e, e->npc.enemy);
	return qtrue;
}

qboolean
facingideal(ent_t *e)
{
	float delta;

	delta = anglemod(e->npc.angles[YAW] - e->npc.idealyaw);
	return !(delta > 45 && delta < 315);
}

qboolean
npccheckattack(ent_t *e)
{
	vec3_t a, b;
	float chance;
	trace_t tr;

	dprint("checkattack\n");

	if(e->npc.enemy != nil && e->npc.enemy->health > 0){
		const int mask = CONTENTS_SOLID|CONTENTS_NPCCLIP|CONTENTS_SLIME|
		   CONTENTS_LAVA;
		veccopy(e->s.pos.trBase, a);
		a[2] += e->npc.viewheight;
		veccopy(e->npc.enemy->s.pos.trBase, b);
		b[2] += CROUCH_VIEWHEIGHT;
	
		
		trap_Trace(&tr, a, nil, nil, b, e->s.number, mask);
		// do we have a clear shot?
		if(tr.entityNum != e->npc.enemy->s.number)
			return qfalse;
	}

	if(e->npc.enemyrange == RANGE_MELEE){
		if(e->npc.melee != nil)
			e->npc.attackstate = AS_MELEE;
		else
			e->npc.attackstate = AS_MISSILE;
		return qtrue;
	}

	if(e->npc.attack == nil)
		return qfalse;

	if(e->npc.enemyrange == RANGE_FAR)
		return qfalse;
	if(e->npc.aiflags & AI_STANDGROUND)
		chance = 0.4f;
	else if(e->npc.enemyrange == RANGE_MELEE)
		chance = 0.2f;
	else if(e->npc.enemyrange == RANGE_NEAR)
		chance = 0.1f;
	else if(e->npc.enemyrange == RANGE_MID)
		chance = 0.02f;
	else
		return qfalse;

	if(random() < chance)
		e->npc.attackstate = AS_SLIDING;
	else
		e->npc.attackstate = AS_STRAIGHT;
	return qfalse;
}

void
aimelee(ent_t *e)
{
	dprint("aimelee\n");

	e->npc.idealyaw = e->npc.enemyyaw;
	npcchangeyaw(e);
	if(facingideal(e)){
		e->npc.melee(e);
		e->npc.attackstate = AS_STRAIGHT;
	}
}

void
aimissile(ent_t *e)
{
	dprint("aimissile\n");
	e->npc.idealyaw = e->npc.enemyyaw;
	npcchangeyaw(e);
	if(facingideal(e)){
		e->npc.attack(e);
		e->npc.attackstate = AS_STRAIGHT;
	}
}

void
aislide(ent_t *e, float dist)
{
	float ofs;

	e->npc.idealyaw = e->npc.enemyyaw;
	npcchangeyaw(e);
	ofs = -90;
	if(npcwalkmove(e, e->npc.idealyaw+ofs, dist))
		return;
	npcwalkmove(e, e->npc.idealyaw-ofs, dist);
}

static qboolean
aicheckattack(ent_t *e, float dist)
{
	vec3_t v;
	qboolean dead;

	dprint("aicheckattack\n");

	// this makes npcs run blindly to the combat point
	// without firing
	if(e->npc.goalent != nil){
		if(e->npc.aiflags & AI_COMBATPOINT)
			return qfalse;
		
		if(e->npc.aiflags & AI_SOUNDTARGET){
			//if(level.time - e->npc.enemy->teleporttime > 5){
			if(1){
				if(e->npc.goalent == e->npc.enemy)
					e->npc.goalent = e->npc.movetarg;
				e->npc.aiflags &= ~AI_SOUNDTARGET;
				if(e->npc.aiflags & AI_TEMPSTANDGROUND)
					e->npc.aiflags &= ~(AI_STANDGROUND|AI_TEMPSTANDGROUND);
			}else{
				e->npc.showhostile = level.time+1;
				return qfalse;
			}
		}
	}

	// see if enemy's dead (or nonexistent) and decide what to do
	// next

	e->npc.enemyvis = qfalse;
	dead = qfalse;
	if(e->npc.enemy == nil || !e->npc.enemy->inuse /* || e->npc.enemy->health <= 0 */){
		dead = qtrue;
	}else if(e->npc.aiflags & AI_MEDIC){
		if(e->npc.enemy->health > 0){
			dead = qtrue;
			e->npc.aiflags &= ~AI_MEDIC;
		}
	}
	if(dead){
		e->npc.enemy = nil;
		if(e->npc.oldenemy != nil /* && e->npc.oldenemy->health > 0 */){
			e->npc.enemy = e->npc.oldenemy;
			e->npc.oldenemy = nil;
			hunttarget(e);
		}else{
			if(e->npc.movetarg != nil){
				e->npc.goalent = e->npc.movetarg;
				e->npc.walk(e);
			}else{
				// pausetime is needed to stop NPCs
				// aimlessly wandering and hunting
				// the world entity
				e->npc.pausetime = level.time+100000000;
				e->npc.stand(e);
			}
			return qtrue;
		}
	}

	e->npc.showhostile = level.time+1;	// wake up others

	e->npc.enemyvis = visible(e, e->npc.enemy);
	if(e->npc.enemyvis){
		e->npc.searchtime = level.time + 5000;
		veccopy(e->npc.enemy->s.pos.trBase, e->npc.lastsighting);
	}

	e->npc.enemyinfront = infront(e, e->npc.enemy);
	e->npc.enemyrange = range(e, e->npc.enemy);
	vecsub(e->npc.enemy->s.pos.trBase, e->s.pos.trBase, v);
	dprint("aicheckattack %s ---> %s = %s\n", vtos(e->npc.enemy->s.pos.trBase),
			vtos(e->s.pos.trBase), vtos(v));
	e->npc.enemyyaw = vectoyaw(v);

	if(e->npc.attackstate == AS_MISSILE){
		aimissile(e);
		return qtrue;
	}else if(e->npc.attackstate == AS_MELEE){
		aimelee(e);
		return qtrue;
	}
	if(!e->npc.enemyvis)
		return qtrue;
	return e->npc.checkattack(e);
}

/*
 * NPC has an enemy it is trying to kill.
 */
void
airun(ent_t *e, float dist)
{
	vec3_t v, forward, right, lefttarg, righttarg;
	ent_t *tmpgoal, *save, *marker;
	trace_t tr;
	float d1, d2;
	qboolean new;

	if(e == nil){
		gprintf("airun(e=nil)\n");
		return;
	}

	if(e->npc.aiflags & AI_COMBATPOINT){
		npcmovetogoal(e, dist);
		return;
	}

	if(e->npc.aiflags & AI_SOUNDTARGET){
		vecsub(e->s.pos.trBase, e->npc.enemy->s.pos.trBase, v);
		if(veclen(v) < 64){
			e->npc.aiflags |= AI_STANDGROUND;
			e->npc.stand(e);
			return;
		}

		npcmovetogoal(e, dist);
		if(!findtarget(e))
			return;
	}

	if(aicheckattack(e, dist))
		return;

	if(e->npc.attackstate == AS_SLIDING){
		aislide(e, dist);
		return;
	}

	if(e->npc.enemyvis){
		npcmovetogoal(e, dist);
		e->npc.aiflags &= AI_LOSTSIGHT;
		veccopy(e->npc.enemy->s.pos.trBase, e->npc.lastsighting);
		e->npc.trailtime = level.time;
		return;
	}

	if(e->npc.searchtime && level.time > e->npc.searchtime+2000){
		npcmovetogoal(e, dist);
		e->npc.searchtime = 0;
		return;
	}

	save = e->npc.goalent;
	tmpgoal = entspawn();
	e->npc.goalent = tmpgoal;
	new = qfalse;

	if(!(e->npc.aiflags & AI_LOSTSIGHT)){
		gprintf("lost sight, last seen at %s\n",
		   vtos(e->npc.lastsighting));
		e->npc.aiflags |= AI_LOSTSIGHT|AI_PURSUITLASTSEEN;
		e->npc.aiflags &= ~(AI_PURSUENEXT|AI_PURSUETEMP);
		new = qtrue;
	}

	if(e->npc.aiflags & AI_PURSUENEXT){
		e->npc.aiflags &= ~AI_PURSUENEXT;
		gprintf("reached goal %s / %s\n",
		   vtos(e->s.pos.trBase), vtos(e->npc.lastsighting));
		// give npc more time since it got this far
		e->npc.searchtime = level.time + 5000;

		if(e->npc.aiflags & AI_PURSUETEMP){
			gprintf("was temp goal; retrying original\n");
			e->npc.aiflags &= ~AI_PURSUETEMP;
			marker = nil;
			veccopy(e->npc.savedgoal, e->npc.lastsighting);
			new = qtrue;
		}else if(e->npc.aiflags & AI_PURSUITLASTSEEN){
			e->npc.aiflags &= ~AI_PURSUITLASTSEEN;
			marker = trailfirst(e);
		}else{
			marker = trailnext(e);
		}
	}

	if(marker != nil){
		veccopy(marker->s.pos.trBase, e->npc.lastsighting);
		e->npc.trailtime = marker->timestamp;
		e->npc.idealyaw = marker->s.apos.trBase[YAW];
		gprintf("heading is %f\n", e->npc.idealyaw);
		new = qtrue;
	}

	vecsub(e->s.pos.trBase, e->npc.lastsighting, v);
	d1 = veclen(v);
	if(d1 <= dist){
		e->npc.aiflags |= AI_PURSUENEXT;
		dist = d1;
	}

	veccopy(e->npc.lastsighting, e->npc.goalent->s.pos.trBase);

	if(new)
		checkcourse(e, v);

	npcmovetogoal(e, dist);
	entfree(tmpgoal);
	e->npc.goalent = save;
}

static void
checkcourse(ent_t *e, vec3_t v)
{
	trace_t tr;
	vec3_t vforward, vright, lefttarg, righttarg;
	float d1, d2, left, center, right;

	trap_Trace(&tr, e->s.pos.trBase, e->r.mins, e->r.maxs, 
	   e->npc.lastsighting, e->s.number, MASK_PLAYERSOLID);
	if(tr.fraction == 1.0f){
		gprintf("course was fine\n");
		return;
	}
	vecsub(e->npc.goalent->s.pos.trBase, e->s.pos.trBase, v);
	d1 = veclen(v);
	center = tr.fraction;
	d2 = d1*((center+1)/2);
	e->npc.idealyaw = vectoyaw(v);
	anglevecs(e->npc.angles, vforward, vright, nil);

	vecset(v, d2, -16.0f, 0.0f);
	G_ProjectSource(e->s.pos.trBase, v, vforward, vright, lefttarg);
	trap_Trace(&tr, e->s.pos.trBase, e->r.mins, e->r.maxs, lefttarg,
	   e->s.number, MASK_PLAYERSOLID);
	left = tr.fraction;

	vecset(v, d2, 16.0f, 0.0f);
	G_ProjectSource(e->s.pos.trBase, v, vforward, vright, righttarg);
	trap_Trace(&tr, e->s.pos.trBase, e->r.mins, e->r.maxs, righttarg,
	   e->s.number, MASK_PLAYERSOLID);
	right = tr.fraction;


	center = (d1*center)/d2;
	if(left >= center && left > right){
		if(left < 1.0f){
			vecset(v, d2*left*0.5f, -16.0f, 0.0f);
			G_ProjectSource(e->s.pos.trBase, v, vforward,
			   vright, lefttarg);
			gprintf("incomplete path (L), go part way\n");
		}
		veccopy(e->npc.lastsighting, e->npc.savedgoal);
		e->npc.aiflags |= AI_PURSUETEMP;
		veccopy(lefttarg, e->npc.goalent->s.pos.trBase);
		veccopy(lefttarg, e->npc.lastsighting);
		vecsub(e->npc.goalent->s.pos.trBase, e->s.pos.trBase, v);
		dprint("%s ---> %s = %s\n", vtos(e->npc.goalent->s.pos.trBase),
		   vtos(e->s.pos.trBase), vtos(v));
		e->npc.idealyaw = vectoyaw(v);
		gprintf("adjusted left\n");
	}else if(right >= center && right > left){
		if(right < 1.0f){
			vecset(v, d2*right*0.5f, 16.0f, 0.0f);
			G_ProjectSource(e->s.pos.trBase, v, vforward,
			   vright, righttarg);
			gprintf("incomplete path (R), go part way\n");
		}
		veccopy(e->npc.lastsighting, e->npc.savedgoal);
		e->npc.aiflags |= AI_PURSUETEMP;
		veccopy(righttarg, e->npc.goalent->s.pos.trBase);
		veccopy(righttarg, e->npc.lastsighting);
		vecsub(e->npc.goalent->s.pos.trBase, e->s.pos.trBase, v);
		dprint("%s ---> %s = %s\n", vtos(e->npc.goalent->s.pos.trBase),
		   vtos(e->s.pos.trBase), vtos(v));
		e->npc.idealyaw = vectoyaw(v);
		gprintf("adjusted right\n");
	}
}

static int
range(ent_t *e, ent_t *other)
{
	vec3_t v;
	float len;

	dprint("range\n");

	vecsub(e->s.pos.trBase, other->s.pos.trBase, v);
	len = veclen(v);
	if(len < 80)
		return RANGE_MELEE;
	if(len < 500)
		return RANGE_NEAR;
	if(len < 1000)
		return RANGE_MID;
	return RANGE_FAR;
}

struct
{
	int	f;
	char	*s;
} aifconv[] = {
	{AI_STANDGROUND,	"stand"},
	{AI_TEMPSTANDGROUND,	"tempstand"},
	{AI_SOUNDTARGET,	"sound"},
	{AI_LOSTSIGHT,		"lostsight"},
	{AI_PURSUITLASTSEEN,	"lastseen"},
	{AI_PURSUENEXT,		"pursuenext"},
	{AI_PURSUETEMP,		"pursuetemp"},
	{AI_HOLDFRAME,		"hold"},
	{AI_GOODGUY,		"good"},
	{AI_NOSTEP,		"nostep"},
	{AI_COMBATPOINT,	"combatpt"},
	{AI_MEDIC,		"medic"},
	{AI_SWIM,		"swim"},
	{AI_FLY,		"fly"},
	{AI_PARTIALGROUND,	"partial"}
};

static void
printaiflags(int flags)
{
	char s[256];
	int i;

	return;
	gprintf("%d: ", flags);
	*s = '\0';
	for(i = 0; i < ARRAY_LEN(aifconv); i++){
		if(flags & aifconv[i].f){
			Q_strlcat(s, aifconv[i].s, sizeof s);
			Q_strlcat(s, " ", sizeof s);
		}
	}
	gprintf("%s\n", s);
}
