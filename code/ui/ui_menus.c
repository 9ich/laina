#include "ui_local.h"

enum {NRES = 32};

static char *builtinres[] = {
	"320x240",
	"400x300",
	"512x384",
	"640x480",
	"800x600",
	"960x720",
	"1024x768",
	"1152x864",
	"1280x1024",
	"1600x1200",
	"2048x1536",
	"856x480",
	nil
};

static char *builtinhz[] = {"60"};

static struct {
	float r;
	char *s;
} knownratios[] = {
	{ 4./3,		"4:3"   },
	{ 16./10,	"16:10" },
	{ 16./9,		"16:9"  },
	{ 5./4,		"5:4"   },
	{ 3./2,		"3:2"   },
	{ 14./9,		"14:9"  },
	{ 5./3,		"5:3"   }
};

static char *qualitylist[] = {"N64", "PS1"};
static char *gqualitylist[] = {"Low", "High"};
static char *sndqualitylist[] = {"Default", "Low", "Normal", "Silly"};

static char *ratios[NRES];
static char ratiobuf[NRES][16];
static char resbuf[512];
static char *detectedres[NRES];
static char *detectedhz[NRES];
static char **resolutions = builtinres;
static char **refreshrates = builtinhz;

// video options
static struct {
	qboolean initialized, dirty, needrestart;
	char *reslist[NRES];
	int nres, resi;
	char *ratlist[ARRAY_LEN(knownratios) + 1];
	int nrat, rati;
	char *hzlist[NRES];
	int nhz, hzi;
	qboolean fullscr;
	qboolean vsync;
	float fov;
	qboolean drawfps;
	int texquality;
	int gquality;
	float gamma;
} vo;

// sound options
static struct {
	qboolean initialized, dirty, needrestart;
	int qual;
	float vol;
	float muvol;
	qboolean doppler;
} so;

static void getmodes(void)
{
	uint i;
	char *p;

	Q_strncpyz(resbuf, UI_Cvar_VariableString("r_availablemodes"), sizeof resbuf);
	if(*resbuf == '\0')
		return;
	for(p = resbuf, i = 0; p != nil && i < ARRAY_LEN(detectedres)-1;){
		// XxY
		detectedres[i++] = p;

		// @Hz
		p = strchr(p, '@');
		if(p != nil)
			*p++ = '\0';
		detectedhz[i] = p;

		// next
		p = strchr(p, ' ');
		if(p != nil)
			*p++ = '\0';
	}
	detectedres[i] = nil;
	detectedhz[i] = nil;
	if(i > 0){
		resolutions = detectedres;
		refreshrates = detectedhz;
	}
}

static void calcratios(void)
{
	int i, r;

	for(r = 0; resolutions[r]; r++){
		float w, h;
		char *x, str[sizeof ratiobuf[0]];

		// calculate resolution's aspect ratio
		x = strchr(resolutions[r], 'x') + 1;
		Q_strncpyz(str, resolutions[r], x-resolutions[r]);
		w = (float)atoi(str);
		h = (float)atoi(x);
		str[0] = '\0';
		for(i = 0; i < ARRAY_LEN(knownratios); i++)
			if(fabs(knownratios[i].r - w/h) < 0.02f){	// close enough?
				Q_strncpyz(str, knownratios[i].s, sizeof str);
				break;
			}
		if(*str == '\0')	// unrecognized ratio?
			Com_sprintf(str, sizeof str, "%.2f:1", w/h);
		Q_strncpyz(ratiobuf[r], str, sizeof ratiobuf[r]);
		ratios[r] = ratiobuf[r];
	}
	ratios[r] = nil;
}

void placeholder(void)
{
	uis.fullscreen = qtrue;
	drawpic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.menuBackShader);
	if(button(".p.b", SCREEN_WIDTH/2, 210, UI_CENTER, "Go away"))
		pop();
}

void quitmenu(void)
{
	uis.fullscreen = qtrue;

	drawpic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.menuBackShader);
	if(button(".qm.yes", SCREEN_WIDTH/2 - 20, 210, UI_RIGHT, "Quit"))
		trap_Cmd_ExecuteText(EXEC_APPEND, "quit\n");
	if(button(".qm.no", SCREEN_WIDTH/2 + 20, 210, UI_LEFT, "Cancel"))
		pop();
}

static void optionsbuttons(void)
{
	const float spc = 35;
	float x, y;

	x = 160;
	y = 160;
	if(button(".o.v", 160, y, UI_RIGHT, "Video")){
		pop();
		vo.initialized = qfalse;
		push(videomenu);
	}
	y += spc;
	if(button(".o.s", x, y, UI_RIGHT, "Sound")){
		pop();
		so.initialized = qfalse;
		push(soundmenu);
	}
	y += spc;
	if(button(".o.c", x, y, UI_RIGHT, "Controls"))
		push(placeholder);
	y += spc;
	if(button(".o.d", x, y, UI_RIGHT, "Defaults"))
		push(placeholder);
	y += spc;
	if(button(".o.bk", 10, SCREEN_HEIGHT-30, UI_LEFT, "Back")){
		vo.initialized = qfalse;
		so.initialized = qfalse;
		pop();
	}
}

/*
Builds list of supported resolutions & refresh rates for the current aspect ratio.
*/
static void mkmodelists(const char *ratio)
{
	int i, j;

	for(i = 0; resolutions[i] != nil; i++){
		if(Q_stricmp(ratios[i], ratio) != 0)
			continue;
		for(j = 0; j < vo.nres; j++)
			if(Q_stricmp(vo.reslist[j], resolutions[i]) == 0)
				break;	// res already in list
		if(j == vo.nres)
			vo.reslist[vo.nres++] = resolutions[i];

		for(j = 0; j < vo.nhz; i++)
			if(Q_stricmp(vo.hzlist[j], refreshrates[i]) == 0)
				break;	// refresh rate already in list
		if(j == vo.nhz)
			vo.hzlist[vo.nhz++] = refreshrates[i];
	}
}

/*
Builds list of aspect ratios
*/
static void mkratlist(void)
{
	int i, j;
	qboolean present;

	for(i = 0; ratios[i] != nil; i++){
		present = qfalse;
		for(j = 0; vo.ratlist[j] != nil; j++)
			if(Q_stricmp(vo.ratlist[j], ratios[i]) == 0){
				present = qtrue;
				break;
			}
		if(present)
			continue;
		for(j = 0; j < ARRAY_LEN(vo.ratlist)-1; j++)
			if(vo.ratlist[j] == nil){
				vo.ratlist[j] = ratios[i];
				vo.nrat++;
				break;
			}
	}
}

static void initvideomenu(void)
{
	int i, w, h, hz;
	char resstr[16], hzstr[16];
	char *ratio;

	memset(&vo, 0, sizeof vo);
	ratio = nil;
	getmodes();
	calcratios();
	mkratlist();

	// grab the current mode
	w = trap_Cvar_VariableValue("r_customwidth");
	h = trap_Cvar_VariableValue("r_customheight");
	hz = trap_Cvar_VariableValue("r_displayrefresh");
	Com_sprintf(resstr, sizeof resstr, "%dx%d", w, h);
	Com_sprintf(hzstr, sizeof hzstr, "%d", hz);
	for(i = 0; resolutions[i] != nil; i++)
		if(Q_stricmp(resolutions[i], resstr) == 0)
			ratio = ratios[i];

	mkmodelists(ratio);

	// init menu values
	vo.resi = 0;
	vo.rati = 0;
	vo.hzi = 0;
	for(i = 0; vo.reslist[i] != nil; i++)
		if(Q_stricmp(vo.reslist[i], resstr) == 0)
			vo.resi = i;
	for(i = 0; vo.ratlist[i] != nil; i++)
		if(Q_stricmp(vo.ratlist[i], ratio) == 0)
			vo.rati = i;
	for(i = 0; vo.hzlist[i] != nil; i++)
		if(Q_stricmp(vo.hzlist[i], hzstr) == 0)
			vo.hzi = i;
	vo.texquality = 0;
	if(trap_Cvar_VariableValue("r_texturebits") == 16)
		vo.texquality = 1;
	vo.vsync = (qboolean)trap_Cvar_VariableValue("r_swapinterval");
	vo.fov = trap_Cvar_VariableValue("cg_fov");
	vo.drawfps = trap_Cvar_VariableValue("cg_drawfps");
	vo.fullscr = (qboolean)trap_Cvar_VariableValue("r_fullscreen");
	vo.gamma = trap_Cvar_VariableValue("r_gamma");
	vo.initialized = qtrue;
}

static void savevideochanges(void)
{
	char w[32], *h;

	if(!vo.dirty && !vo.needrestart)
		return;
	
	h = strchr(vo.reslist[vo.resi], 'x') + 1;
	Q_strncpyz(w, vo.reslist[vo.resi], h-vo.reslist[vo.resi]);
	trap_Cvar_Set("r_customwidth", w);
	trap_Cvar_Set("r_customheight", h);
	trap_Cvar_SetValue("r_mode -1", -1);
	trap_Cvar_SetValue("cg_fov", (int)vo.fov);
	trap_Cvar_SetValue("cg_drawfps", (int)vo.drawfps);
	trap_Cvar_SetValue("r_gamma", vo.gamma);

	if(Q_stricmp(qualitylist[vo.texquality], "PS1") == 0){
		trap_Cvar_SetValue("r_texturebits", 16);
		trap_Cvar_Set("r_texturemode", "GL_NEAREST");
	}else{
		trap_Cvar_SetValue("r_texturebits", 32);
		trap_Cvar_Set("r_texturemode", "GL_LINEAR_MIPMAP_LINEAR");
	}

	if(vo.fullscr)
		trap_Cvar_SetValue("r_fullscreen", 1);
	else
		trap_Cvar_SetValue("r_fullscreen", 0);

	if(vo.vsync)
		trap_Cvar_SetValue("r_swapinterval", 1);
	else
		trap_Cvar_SetValue("r_swapinterval", 0);

	if(vo.needrestart)
		trap_Cmd_ExecuteText(EXEC_APPEND, "vid_restart\n");
	vo.initialized = qfalse;
}

void videomenu(void)
{
	const float spc = 24;
	float x, xx, y;
	int i, j;

	if(!vo.initialized)
		initvideomenu();

	uis.fullscreen = qtrue;
	drawpic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.menuBackShader);
	optionsbuttons();
	x = 420;
	xx = 440;
	y = 100;

	drawstr(x, y, "Resolution", UI_RIGHT|UI_DROPSHADOW, color_white);
	if(textspinner(".v.res", xx, y, 0, vo.reslist, &vo.resi, vo.nres))
		vo.needrestart = qtrue;
	y += spc;

	drawstr(x, y, "Aspect ratio", UI_RIGHT|UI_DROPSHADOW, color_white);
	if(textspinner(".v.rat", xx, y, 0, vo.ratlist, &vo.rati, vo.nrat)){
		// show resolutions for this aspect ratio
		int w, h;
		char resstr[16];

		vo.nres = 0;
		vo.resi = 0;
		w = trap_Cvar_VariableValue("r_customwidth");
		h = trap_Cvar_VariableValue("r_customheight");
		Com_sprintf(resstr, sizeof resstr, "%dx%d", w, h);
		memset(vo.reslist, 0, sizeof vo.reslist);
		for(i = 0, j = 0; resolutions[i] != nil; i++)
			if(Q_stricmp(ratios[i], vo.ratlist[vo.rati]) == 0){
				vo.reslist[j++] = resolutions[i];
				vo.nres++;
			}
		for(i = 0; vo.reslist[i] != nil; i++)
			if(Q_stricmp(vo.reslist[i], resstr) == 0)
				vo.resi = i;
		vo.needrestart = qtrue;
	}
	y += spc;

	drawstr(x, y, "Refresh rate", UI_RIGHT|UI_DROPSHADOW, color_white);
	if(textspinner(".v.hz", xx, y, 0, vo.hzlist, &vo.hzi, vo.nhz))
		vo.needrestart = qtrue;
	y += spc;

	drawstr(x, y, "Fullscreen", UI_RIGHT|UI_DROPSHADOW, color_white);
	if(checkbox(".v.fs", xx, y, 0, &vo.fullscr))
		vo.needrestart = qtrue;
	y += spc;

	drawstr(x, y, "Brightness", UI_RIGHT|UI_DROPSHADOW, color_white);
	if(slider("v.brightness", xx, y, 0, 0, 4, &vo.gamma, "%.2f"))
		vo.dirty = qtrue;
	y += spc;

	drawstr(x, y, "Field of view", UI_RIGHT|UI_DROPSHADOW, color_white);
	if(slider("v.fov", xx, y, 0, 85, 130, &vo.fov, "%.0f"))
		vo.dirty = qtrue;
	y += spc;

	drawstr(x, y, "Vertical sync", UI_RIGHT|UI_DROPSHADOW, color_white);
	if(checkbox(".v.vs", xx, y, 0, &vo.vsync))
		vo.needrestart = qtrue;
	y += spc;

	drawstr(x, y, "Show framerate", UI_RIGHT|UI_DROPSHADOW, color_white);
	if(checkbox(".v.fps", xx, y, 0, &vo.drawfps))
		vo.dirty = qtrue;
	y += spc;

	drawstr(x, y, "Texture quality", UI_RIGHT|UI_DROPSHADOW, color_white);
	if(textspinner(".v.tq", xx, y, 0, qualitylist, &vo.texquality, ARRAY_LEN(qualitylist)))
		vo.needrestart = qtrue;
	y += spc;

	drawstr(x, y, "Geometry quality", UI_RIGHT|UI_DROPSHADOW, color_white);
	if(textspinner(".v.gq", xx, y, 0, gqualitylist, &vo.gquality, ARRAY_LEN(gqualitylist)))
		vo.dirty = qtrue;

	if(vo.dirty || vo.needrestart){
		if(button(".v.accept", 640-20, 480-30, UI_RIGHT, "Accept")){
			savevideochanges();
			pop();
		}
	}
}

static void initsoundmenu(void)
{
	int freq;

	memset(&so, 0, sizeof so);
	freq = (int)trap_Cvar_VariableValue("s_sdlspeed");
	switch(freq){
	case 11025:
		so.qual = 1;
		break;
	case 22050:
		so.qual = 2;
		break;
	case 44100:
		so.qual = 3;
		break;
	default:
		so.qual = 0;
	}
	so.vol = trap_Cvar_VariableValue("s_volume");
	so.muvol = trap_Cvar_VariableValue("s_musicvolume");
	so.doppler = (qboolean)trap_Cvar_VariableValue("s_doppler");
	so.initialized = qtrue;
}

static void savesoundchanges(void)
{
	switch(so.qual){
	case 0:
		trap_Cvar_SetValue("s_sdlspeed", 0);
		break;
	case 1:
		trap_Cvar_SetValue("s_sdlspeed", 11025);
		break;
	case 2:
		trap_Cvar_SetValue("s_sdlspeed", 22050);
		break;
	default:
		trap_Cvar_SetValue("s_sdlspeed", 44100);
	}
	trap_Cvar_SetValue("s_doppler", (float)so.doppler);
	if(so.needrestart)
		trap_Cmd_ExecuteText(EXEC_APPEND, "snd_restart");
	so.initialized = qfalse;
}

void soundmenu(void)
{
	const float spc = 24;
	float x, xx, y;

	if(!so.initialized)
		initsoundmenu();
	
	uis.fullscreen = qtrue;
	drawpic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.menuBackShader);
	optionsbuttons();
	x = 420;
	xx = 440;
	y = 100;

	drawstr(x, y, "Quality", UI_RIGHT|UI_DROPSHADOW, color_white);
	if(textspinner(".s.qual", xx, y, 0, sndqualitylist, &so.qual, ARRAY_LEN(sndqualitylist)))
		so.needrestart = qtrue;
	y += spc;

	drawstr(x, y, "Effects volume", UI_RIGHT|UI_DROPSHADOW, color_white);
	if(slider(".s.vol", xx, y, 0, 0.0f, 1.0f, &so.vol, "%.2f"))
		trap_Cvar_SetValue("s_volume", so.vol);
	y += spc;

	drawstr(x, y, "Music volume", UI_RIGHT|UI_DROPSHADOW, color_white);
	if(slider(".s.muvol", xx, y, 0, 0.0f, 1.0f, &so.muvol, "%.2f"))
		trap_Cvar_SetValue("s_musicvolume", so.muvol);
	y += spc;

	drawstr(x, y, "Doppler effect", UI_RIGHT|UI_DROPSHADOW, color_white);
	if(checkbox(".s.dop", xx, y, 0, &so.doppler))
		so.dirty = qtrue;
	y += spc;

	if(so.dirty || so.needrestart)
		if(button(".s.accept", 640-20, 480-30, UI_RIGHT, "Accept")){
			savesoundchanges();
			pop();
		}
}

void errormenu(void)
{
	int i;
	char buf[MAX_STRING_CHARS], msg[MAX_STRING_CHARS];

	for(i = 0; i < ARRAY_LEN(uis.keys); i++)
		if(uis.keys[i]){
			trap_Cvar_Set("com_errormessage", "");
			pop();
			push(mainmenu);
			return;
		}
	uis.fullscreen = qtrue;
	drawpic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.menuBackShader);
	trap_Cvar_VariableStringBuffer("com_errormessage", buf, sizeof buf);
	Com_sprintf(msg, sizeof msg, "error: %s", buf);
	drawstr(320, 220, msg, UI_SMALLFONT|UI_CENTER|UI_DROPSHADOW, color_red);
}

void mainmenu(void)
{
	const float spc = 35;
	float y;

	uis.fullscreen = qtrue;
	drawpic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.menuBackShader);
	y = 190;
	if(button(".mm.sp", SCREEN_WIDTH/2, y, UI_CENTER, "Single Player"))
		push(placeholder);
	y += spc;
	if(button(".mm.co", SCREEN_WIDTH/2, y, UI_CENTER, "Co-op"))
		push(placeholder);
	y += spc;
	if(button(".mm.o", SCREEN_WIDTH/2, y, UI_CENTER, "Options"))
		push(videomenu);
	y += spc;
	if(button(".mm.q", SCREEN_WIDTH/2, y, UI_CENTER, "Quit"))
		push(quitmenu);
}

void ingamemenu(void)
{
	const float spc = 35;
	float y;

	y = 180;
	if(uis.keys[K_ESCAPE] || button(".im.r", SCREEN_WIDTH/2, y, UI_CENTER, "Resume")){
		pop();
		trap_Cvar_Set("cl_paused", "0");
	}
	y += spc;
	if(button(".im.o", SCREEN_WIDTH/2, y, UI_CENTER, "Options"))
		push(videomenu);
	y += spc;
	if(button(".im.qm", SCREEN_WIDTH/2, y, UI_CENTER, "Quit to menu"))
		trap_Cmd_ExecuteText(EXEC_APPEND, "disconnect\n");
	y += spc;
	if(button(".im.q", SCREEN_WIDTH/2, y, UI_CENTER, "Quit"))
		push(quitmenu);
}
