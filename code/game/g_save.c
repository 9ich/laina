/*
Permanent savegames.

Savegame files begin with a list of world-related infostrings, followed by
brace-enclosed blocks of player infostring lists, for example:

	"complete"	"e1m1 e1m2"
	...

	{
		"guid"	"1A2A3A"
		"name"	"fou"
		"bluerunes"	"0"
		...
	}
*/

#include "g_local.h"

static char saveinfo[1024];
static char playerinfos[MAX_CLIENTS][1024];
static int nplayerinfos;

static void
parseplayersaves(char *buf)
{
	int n;
	char *tok;
	char key[MAX_TOKEN_CHARS], info[MAX_INFO_STRING];

	for(;;){
		if(nplayerinfos >= MAX_CLIENTS){
			gprintf("too many players in savegame\n");
			break;
		}
		tok = COM_Parse(&buf);
		if(*tok == '\0')
			break;
		if(strcmp(tok, "{") != 0){
			gprintf("syntax error: expected \"{\", found \"%s\"\n", tok);
			break;
		}

		info[0] = '\0';
		for(;;){
			// key
			tok = COM_ParseExt(&buf, qtrue);
			if(*tok == '\0'){
				gprintf("syntax error: unexpected end of file\n");
				break;
			}
			if(strcmp(tok, "}") == 0)
				break;
			Q_strncpyz(key, tok, sizeof key);

			// val
			tok = COM_ParseExt(&buf, qfalse);
			if(*tok == '\0')
				Q_strncpyz(tok, "<nil>", strlen(tok)+1);
			Info_SetValueForKey(info, key, tok);
		}
		n = nplayerinfos;
		Q_strncpyz(playerinfos[n], info, sizeof playerinfos[n]);
		nplayerinfos++;
	}
}

static char*
parsesave(char *buf)
{
	char *tok;
	char key[MAX_TOKEN_CHARS], info[MAX_INFO_STRING];

	tok = nil;
	for(;;){
		// key
		tok = COM_Parse(&buf);
		if(*tok == '\0')
			break;
		if(strcmp(tok, "{") == 0)
			break;	// hit start of the players section
		if(strcmp(tok, "}") == 0){
			gprintf("syntax error: unexpected \"}\"\n");
			break;
		}
		Q_strncpyz(key, tok, sizeof key);

		// val
		tok = COM_ParseExt(&buf, qfalse);
		if(*tok == '\0')
			Q_strncpyz(tok, "<nil>", strlen(tok)+1);
		Info_SetValueForKey(info, key, tok);
	}
	Q_strncpyz(saveinfo, info, sizeof saveinfo);
	return tok;
}

/*
Converts active game values to infostrings.  Updates existing infostring pairs.
*/
static void
infoconv(void)
{
	char uinfo[MAX_INFO_STRING], *p, *s, *t;
	int i, j;

	for(i = 0; i < MAX_CLIENTS; i++){
		if(!g_entities[i].inuse)
			continue;
		trap_GetUserinfo(g_entities[i].s.number, uinfo, sizeof uinfo);
		s = Info_ValueForKey(uinfo, "cl_guid");
		p = nil;
		// look for this guid in existing infostrings
		for(j = 0; j < MAX_CLIENTS; j++){
			if(*playerinfos[j] == '\0')
				continue;
			t = Info_ValueForKey(playerinfos[j], "guid");
			if(Q_stricmp(s, t) == 0){
				p = playerinfos[j];
				break;
			}
		}
		// if it doesn't exist, make a new infostring for it
		if(p == nil){
			if(nplayerinfos >= ARRAY_LEN(playerinfos)-1){
				gprintf("error: trying to save too many players\n");
				continue;
			}
			p = playerinfos[nplayerinfos];
			nplayerinfos++;
		}

		// populate/update infostrings
		Info_SetValueForKey(p, "guid", s);
		s = Info_ValueForKey(uinfo, "name");
		Info_SetValueForKey(p, "(name)", s);
	}
}

void
loadgame(const char *fname)
{
	int len;
	fileHandle_t f;
	char buf[8192], *p;

	len = trap_FS_FOpenFile(fname, &f, FS_READ);
	if(f == 0){
		gprintf("%s: file not found\n", fname);
		return;
	}
	if(len >= sizeof buf){
		gprintf("%s: savegame file too large\n", fname);
		return;
	}
	trap_FS_Read(buf, len, f);
	buf[len] = '\0';
	trap_FS_FCloseFile(f);

	p = parsesave(buf);
	parseplayersaves(p);
}

void
savegame(const char *fname)
{
	char buf[MAX_STRING_CHARS], fbuf[8192];
	char key[MAX_INFO_KEY], val[MAX_INFO_VALUE];
	const char *p;
	int i;
	fileHandle_t f;

	buf[0] = '\0';
	fbuf[0] = '\0';

	infoconv();

	p = saveinfo;
	Info_NextPair(&p, key, val);
	while(*key != '\0'){
		Com_sprintf(buf, sizeof buf, "\"%s\"\t\"%s\"\n", key, val);
		Q_strlcat(fbuf, buf, sizeof fbuf);
		Info_NextPair(&p, key, val);
	}
	Q_strlcat(fbuf, "\n", sizeof fbuf);
	gprintf("\n");

	for(i = 0; i < MAX_CLIENTS; i++){
		if(*playerinfos[i] == '\0')
			continue;

		Q_strlcat(fbuf, "{\n", sizeof fbuf);
		p = playerinfos[i];
		Info_NextPair(&p, key, val);
		while(*key != '\0'){
			Com_sprintf(buf, sizeof buf, "\t\"%s\"\t\"%s\"\n", key, val);
			Q_strlcat(fbuf, buf, sizeof fbuf);
			Info_NextPair(&p, key, val);
		}
		Q_strlcat(fbuf, "}\n", sizeof fbuf);
	}
	
	trap_FS_FOpenFile(fname, &f, FS_WRITE);
	if(f == 0){
		gprintf("%s: could not open file for writing\n", fname);
		return;
	}
	trap_FS_Write(fbuf, strlen(fbuf), f);
	trap_FS_FCloseFile(f);
}

void
clientfromsave(gclient_t *client, const char *guid)
{
	int i;
	const char *s;

	for(i = 0; i < nplayerinfos; i++){
		s = Info_ValueForKey(playerinfos[i], "guid");
		if(Q_stricmp(s, guid) == 0)
			break;
	}
	if(i == nplayerinfos)
		return;	// player not in save data
}

