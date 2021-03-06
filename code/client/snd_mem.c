/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

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

/*#
sound caching
*/

#include "snd_local.h"
#include "snd_codec.h"

#define DEF_COMSOUNDMEGS "8"

static sndBuffer* sndbuffers = NULL;
static sndBuffer* freelist = NULL;
static int sndmem_avail = 0;
static int sndmem_inuse = 0;

short *sfxScratchBuffer = NULL;
sfx_t *sfxScratchPointer = NULL;
int	   sfxScratchIndex = 0;

void	SND_free(sndBuffer *v) {
	*(sndBuffer **)v = freelist;
	freelist = (sndBuffer*)v;
	sndmem_avail += sizeof(sndBuffer);
}

sndBuffer*	SND_malloc(void) {
	while (!freelist) {
		S_FreeOldestSound();
	}

	sndmem_avail -= sizeof(sndBuffer);
	sndmem_inuse += sizeof(sndBuffer);

	sndBuffer* v = freelist;
	freelist = *(sndBuffer **)freelist;
	v->next = NULL;
	return v;
}

void SND_setup(void) {
	sndBuffer *p, *q;

	// a sndBuffer is actually 2K+, so this isn't even REMOTELY close to actual megs
	const cvar_t* cv = Cvar_Get( "com_soundMegs", DEF_COMSOUNDMEGS, CVAR_LATCH | CVAR_ARCHIVE );
	int scs = cv->integer * 1024;

	sndbuffers = (sndBuffer*)Hunk_Alloc( scs * sizeof(sndBuffer), h_high );
	sndmem_avail = scs * sizeof(sndBuffer);

	p = sndbuffers;
	q = p + scs;
	while (--q > p)
		*(sndBuffer **)q = q-1;

	*(sndBuffer **)q = NULL;
	freelist = p + scs - 1;
}

void SND_shutdown(void)
{
		free(sfxScratchBuffer);
}

/*
================
ResampleSfx

resample / decimate to the current source rate
================
*/
static int ResampleSfx( sfx_t *sfx, int channels, int inrate, int inwidth, int samples, byte *data, qboolean compressed ) {
	int		outcount;
	int		srcsample;
	float	stepscale;
	int		i, j;
	int		sample, samplefrac, fracstep;
	int			part;
	sndBuffer	*chunk;
	
	stepscale = (float)inrate / dma.speed;	// this is usually 0.5, 1, or 2

	outcount = samples / stepscale;

	samplefrac = 0;
	fracstep = stepscale * 256 * channels;
	chunk = sfx->soundData;

	for (i=0 ; i<outcount ; i++)
	{
		srcsample = samplefrac >> 8;
		samplefrac += fracstep;
		for (j=0 ; j<channels ; j++)
		{
			if( inwidth == 2 ) {
				sample = ( ((short *)data)[srcsample+j] );
			} else {
				sample = (int)( (unsigned char)(data[srcsample+j]) - 128) << 8;
			}
			part = (i*channels+j)&(SND_CHUNK_SIZE-1);
			if (part == 0) {
				sndBuffer	*newchunk;
				newchunk = SND_malloc();
				if (chunk == NULL) {
					sfx->soundData = newchunk;
				} else {
					chunk->next = newchunk;
				}
				chunk = newchunk;
			}

			chunk->sndChunk[part] = sample;
		}
	}

	return outcount;
}

//=============================================================================

/*
==============
S_LoadSound

The filename may be different than sfx->name in the case
of a forced fallback of a player specific sound
==============
*/
qboolean S_LoadSound( sfx_t *sfx )
{
	byte	*data;
	snd_info_t	info;

	// player specific sounds are never directly loaded
	if (sfx->soundName[0] == '*') {
		return qfalse;
	}

	// load it in
	data = S_CodecLoad(sfx->soundName, &info);
	if(!data)
		return qfalse;

	if ( info.width == 1 ) {
		Com_DPrintf(S_COLOR_YELLOW "WARNING: %s is a 8 bit audio file\n", sfx->soundName);
	}

	if ( info.rate != 22050 ) {
		Com_DPrintf(S_COLOR_YELLOW "WARNING: %s is not a 22kHz audio file\n", sfx->soundName);
	}

	sfx->lastTimeUsed = Com_Milliseconds()+1;

	sfx->soundData = NULL;
	sfx->soundLength = ResampleSfx( sfx, info.channels, info.rate,
		info.width, info.samples, data + info.dataofs, qfalse );

	sfx->soundChannels = info.channels;
	Z_Free(data);
	return qtrue;
}

void S_DisplayFreeMemory(void) {
	Com_Printf("%d bytes free sound buffer memory, %d total used\n", sndmem_inuse, sndmem_avail);
}
