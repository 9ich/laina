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

void npcnoprint(char *fmt, ...) { (void)fmt; }
#define dprint npcnoprint

static void	invalid(ent_t*);

static void
beginmove(ent_t *e)
{
	evaltrajectory(&e->s.pos, level.time, e->npc.pos);
	evaltrajectory(&e->s.apos, level.time, e->npc.angles);
	veccpy(e->npc.pos, e->s.origin);
	veccpy(e->npc.pos, e->r.currentOrigin);
	vecclear(e->npc.vel);
	vecclear(e->npc.anglesvel);
}

static void
finishmove(ent_t *e)
{
	veccpy(e->npc.pos, e->s.pos.trBase);
	veccpy(e->npc.vel, e->s.pos.trDelta);
	e->s.pos.trTime = level.time;
	if(e->npc.vel[0] == 0 && e->npc.vel[1] == 0 && e->npc.vel[2] == 0)
		e->s.pos.trTime = 0;
	evaltrajectory(&e->s.pos, level.time, e->npc.pos);
	evaltrajectory(&e->s.apos, level.time, e->npc.angles);
	veccpy(e->npc.pos, e->s.origin);
	veccpy(e->npc.pos, e->r.currentOrigin);

	veccpy(e->npc.angles, e->s.apos.trBase);
	veccpy(e->npc.anglesvel, e->s.apos.trDelta);
	e->s.apos.trTime = level.time;
	if(e->npc.anglesvel[0] == 0 && e->npc.anglesvel[1] == 0 && e->npc.anglesvel[2] == 0)
		e->s.apos.trTime = 0;
}

void
npccheckground(ent_t *e)
{
	vec3_t pt;
	trace_t tr;

	dprint("npccheckground\n");

	if(e->s.pos.trDelta[2] > 100.0f){
		e->s.groundEntityNum = ENTITYNUM_NONE;
		return;
	}

	// if the hull point 1/4 u down is solid, the entity
	// is on ground
	pt[0] = e->s.pos.trBase[0];
	pt[1] = e->s.pos.trBase[1];
	pt[2] = e->s.pos.trBase[2] - 0.25f;

	trap_Trace(&tr, e->s.pos.trBase, e->r.mins, e->r.maxs, pt, e->s.number, MASK_NPCSOLID);
	// check steepness
	if(tr.plane.normal[2] < 0.7f && !tr.startsolid){
		e->s.groundEntityNum = ENTITYNUM_NONE;
		return;
	}

	if(!tr.startsolid && !tr.allsolid){
		veccpy(tr.endpos, e->s.pos.trBase);
		e->s.groundEntityNum = tr.entityNum;
		e->s.pos.trDelta[2] = 0.0f;
	}
}

void
npcworldeffects(ent_t *e)
{
	int dmg;

	if(e->health > 0){
		if(e->waterlevel > 0)
			e->airouttime = level.time + 9;
		else if(e->airouttime < level.time){
			// suffocate
			if(e->paindebouncetime < level.time){
				dmg = 2 + 2*floor(level.time - e->airouttime);
				dmg = MIN(dmg, 15);
				entdamage(e, nil, nil, vec3_origin,
				   e->s.pos.trBase, dmg,
				   DAMAGE_NO_ARMOR, MOD_WATER);
			}
		}
	}

	if((e->watertype & CONTENTS_LAVA)){
		if(e->paindebouncetime < level.time){
			e->paindebouncetime = level.time + 0.2f;
			entdamage(e, nil, nil, vec3_origin,
			   e->s.pos.trBase, 10*e->waterlevel,
			   0, MOD_LAVA);
		}
	}

	if((e->watertype & CONTENTS_SLIME)){
		if(e->paindebouncetime < level.time){
			e->paindebouncetime = level.time + 1.0f;
			entdamage(e, nil, nil, vec3_origin,
			   e->s.pos.trBase, 10*e->waterlevel,
			   0, MOD_SLIME);
		}
	}
}

void
npcdroptofloor(ent_t *e)
{
	vec3_t end;
	trace_t tr;

	e->s.pos.trBase[2] += 1.0f;
	veccpy(e->s.pos.trBase, end);
	end[2] -= 99999;
	trap_Trace(&tr, e->s.pos.trBase, e->r.mins, e->r.maxs, end, e->s.number,
	   MASK_NPCSOLID);
	if(tr.fraction == 1.0f || tr.allsolid)
		return;
	veccpy(tr.endpos, e->s.pos.trBase);
	trap_LinkEntity(e);
	npccheckground(e);
	//npccategorizepos(e);
}

void
npcmoveframe(ent_t *e)
{
	npcmove_t *m;
	npcframe_t *f;
	int i;

	dprint("npcmoveframe\n");

	m = e->npc.mv;
	e->nextthink = level.time + FRAMETIME;

	if(e->npc.nextframe && e->npc.nextframe >= m->firstframe &&
	   e->npc.nextframe <= m->lastframe){
		e->s.frame = e->npc.nextframe;
		e->npc.nextframe = 0;
	}else{
		if(e->s.frame == m->lastframe && m->endfn != nil){
			m->endfn(e);
			// regrab move; endfn is likely to change it
			m = e->npc.mv;
		}

		if(e->s.frame < m->firstframe || e->s.frame > m->lastframe){
			e->npc.aiflags &= ~AI_HOLDFRAME;
			e->s.frame = m->firstframe;
		}else if(!(e->npc.aiflags & AI_HOLDFRAME)){
			e->s.frame++;
			if(e->s.frame > m->lastframe)
				e->s.frame = m->firstframe;
		}
	}

	i = e->s.frame - m->firstframe;
	if(m == nil){
		gprintf("npcmoveframe: nil e->npc.mv for %s\n",
		   e->classname);
		return;
	}
	f = &m->frame[i];
	if(f->aifn != nil){
		if(!(e->npc.aiflags & AI_HOLDFRAME))
			f->aifn(e, f->dist*e->npc.scale);
		else
			f->aifn(e, 0);
	}
	if(f->think != nil){
		f->think(e);
	}
}

void
npcthink(ent_t *e)
{
	dprint("npcthink\n");
	beginmove(e);
	npcmoveframe(e);
	finishmove(e);
	touchtriggers(e);
}

void
npcuse(ent_t *e, ent_t *other, ent_t *activator)
{
	if(e->npc.enemy != nil ||
	   e->health <= 0 ||
	   activator->flags & FL_NOTARGET ||
	   (activator->client == nil &&
	   !(activator->npc.aiflags & AI_GOODGUY))){
		return;
	}
	e->npc.enemy = activator;
	foundtarget(e);
}

void
npctriggeredspawn(ent_t *e)
{
	e->s.pos.trBase[2] += 1.0f;
	//killbox(e);
	//e->solid = SOLID_BBOX;
	//e->movetype = MOVETYPE_STEP;	// FIXME
	e->r.svFlags &= ~SVF_NOCLIENT;
	e->airouttime = level.time + 12;
	trap_LinkEntity(e);
	npcstart(e);
	if(e->npc.enemy != nil && !(e->spawnflags & 1) && 
	   !(e->npc.enemy->flags & FL_NOTARGET))
		foundtarget(e);
	else
		e->npc.enemy = nil;
}

void
npctriggeredspawnuse(ent_t *e, ent_t *other, ent_t *activator)
{
	e->think = npctriggeredspawn;
	e->nextthink = level.time + FRAMETIME;
	if(activator->client != nil)
		e->npc.enemy = activator;
	e->use = npcuse;
}

void
npctriggeredstart(ent_t *e)
{
	//e->solid = SOLID_NOT;
	//e->movetype = MOVETYPE_NONE;
	e->r.svFlags |= SVF_NOCLIENT;
	e->nextthink = 0;
	e->use = npctriggeredspawnuse;
}

/*
 * Dying NPCs trigger all their targets.
 */
void
npcdeathuse(ent_t *e)
{
	dprint("npcdeathuse\n");

	e->npc.aiflags &= AI_GOODGUY;
	if(e->item != nil){
		itemdrop(e, e->item, 0);
		e->item = nil;
	}
	/*
	if(e->deathtarget != nil)
		e->target = e->deathtarget;

	*/
	if(e->target == nil)
		return;
	usetargets(e, e->enemy);
}

void
npcstart(ent_t *e)
{
	dprint("npcstart\n");

	e->s.pos.trType = TR_LINEAR;
	e->s.apos.trType = TR_LINEAR;
	e->think = npcstartgo;
	e->nextthink = level.time + FRAMETIME;
	e->r.svFlags |= SVF_NPC;
	e->takedmg = qtrue;
	e->airouttime = level.time + 12;
	e->use = npcuse;
	e->clipmask = MASK_NPCSOLID;
	//e->s.skinnum = 0;
	//e->deadflag = DEAD_NO;	// FIXME
	//e->r.svFlags &= ~SVF_DEADBODY;
	//veccpy(e->s.pos.trBase, e->s.oldorigin);

	if(e->npc.checkattack == nil)
		e->npc.checkattack = npccheckattack;

	level.numnpcs++;
	//return qtrue;
}

void
npcstartgo(ent_t *e)
{
	vec3_t v;
	qboolean nocombat, fix;
	ent_t *targ;

	dprint("npcstartgo\n");

	npcdroptofloor(e);

	targ = nil;
	nocombat = qfalse;
	fix = qfalse;

	if(e->health <= 0)
		return;

	if(e->target != nil){
		while((targ = findent(targ, FOFS(targetname), e->target)) != nil){
			if(strcmp(targ->classname, "point_combat") == 0){
				e->npc.combattarg = e->target;
				fix = qtrue;
			}else{
				nocombat = qtrue;
			}
		}
		if(nocombat && e->npc.combattarg)
			gprintf("%s at %s has target with mixed types\n",
			   e->classname, vtos(e->s.pos.trBase));
		if(fix)
			e->target = nil;
	}

	// validate
	if(e->npc.combattarg != nil){
		targ = nil;
		while((targ = findent(targ, FOFS(targetname), e->npc.combattarg)) != nil){
			if(strcmp(targ->classname, "point_combat") != 0){
				gprintf("%s at %s has bad combattarg %s : %s at %s\n",
				   e->classname, vtos(e->s.pos.trBase),
				   e->npc.combattarg, targ->classname,
				   vtos(targ->s.pos.trBase));
			}
		}
	}

	if(e->target != nil){
		e->npc.goalent = e->npc.movetarg = picktarget(e->target);
		if(e->npc.movetarg == nil){
			gprintf("%s can't find target %s at %s\n", 
			   e->classname, e->target, vtos(e->s.pos.trBase));
			e->target = nil;
			invalid(e);
		}else if(strcmp(e->npc.movetarg->classname, "path_corner") == 0){
			vecsub(e->npc.goalent->s.pos.trBase, e->s.pos.trBase, v);
			e->npc.idealyaw = vectoyaw(v);
			e->npc.walk(e);
			e->target = nil;
		}else{
			e->npc.goalent = e->npc.movetarg = nil;
			invalid(e);
		}
	}else{
		invalid(e);
	}

	e->think = npcthink;
	e->nextthink = level.time + FRAMETIME;
}

static void
invalid(ent_t *e)
{
	gprintf("invalid\n");
	e->npc.pausetime = level.time + 100000000;
	e->npc.stand(e);
}
