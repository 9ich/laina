#include "ui_local.h"

enum {NRES = 32};

static char *builtinres[ ] = {
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
	NULL
};

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
static char *squalitylist[] = {"Low", "Normal", "Silly"};

static char *ratios[NRES];
static char ratiobuf[NRES][16];
static char resbuf[512];
static char *detectedres[NRES];
static char **resolutions = builtinres;

// video options
static struct {
	qboolean initialized, dirty, needrestart;
	char *reslist[NRES];
	int nres, resi;
	char *ratlist[ARRAY_LEN(knownratios) + 1];
	int nrat, rati;
	char hz[5];
	int hzcaret;
	qboolean fullscr;
	qboolean vsync;
	float fov;
	qboolean drawfps;
	int texquality;
	int gquality;
	float gamma;
} vo;

static void getresolutions(void)
{
	unsigned int i;
	char *p;

	Q_strncpyz(resbuf, UI_Cvar_VariableString("r_availableModes"), sizeof resbuf);
	if(*resbuf == '\0')
		return;
	for(p = resbuf, i = 0; p != NULL && i < ARRAY_LEN(detectedres)-1;){
		detectedres[i++] = p;
		p = strchr(p, ' ');
		if(p != NULL)
			*p++ = '\0';
	}
	detectedres[i] = NULL;
	if(i > 0)
		resolutions = detectedres;
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
	ratios[r] = NULL;
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
	if(button(".o.s", x, y, UI_RIGHT, "Sound"))
		push(placeholder);
	y += spc;
	if(button(".o.c", x, y, UI_RIGHT, "Controls"))
		push(placeholder);
	y += spc;
	if(button(".o.d", x, y, UI_RIGHT, "Defaults"))
		push(placeholder);
	y += spc;
	if(button(".o.bk", 10, SCREEN_HEIGHT-30, UI_LEFT, "Back")){
		vo.initialized = qfalse;
		pop();
	}
}

static void initvideomenu(void)
{
	int w, h;
	int i, j;
	char resstr[16];
	char *ratio;
	qboolean present;

	memset(&vo, 0, sizeof vo);
	vo.initialized = qtrue;
	getresolutions();
	calcratios();
	// build list of supported aspect ratios
	for(i = 0; ratios[i] != NULL; i++){
		present = qfalse;
		for(j = 0; vo.ratlist[j] != NULL; j++)
			if(Q_stricmp(vo.ratlist[j], ratios[i]) == 0){
				present = qtrue;
				break;
			}
		if(present)
			continue;
		for(j = 0; j < ARRAY_LEN(vo.ratlist)-1; j++)
			if(vo.ratlist[j] == NULL){
				vo.ratlist[j] = ratios[i];
				vo.nrat++;
				break;
			}
	}

	// grab the current res
	w = trap_Cvar_VariableValue("r_customwidth");
	h = trap_Cvar_VariableValue("r_customheight");
	Com_sprintf(resstr, sizeof resstr, "%dx%d", w, h);
	for(i = 0; resolutions[i] != NULL; i++)
		if(Q_stricmp(resolutions[i], resstr) == 0)
			ratio = ratios[i];
	
	// build list of supported resolutions for the current aspect ratio
	for(i = 0, j = 0; resolutions[i] != NULL; i++)
		if(Q_stricmp(ratios[i], ratio) == 0){
			vo.reslist[j++] = resolutions[i];
			vo.nres++;
		}

	// init menu values
	vo.resi = 0;
	vo.rati = 0;
	for(i = 0; vo.reslist[i] != NULL; i++)
		if(Q_stricmp(vo.reslist[i], resstr) == 0)
			vo.resi = i;
	for(i = 0; vo.ratlist[i] != NULL; i++)
		if(Q_stricmp(vo.ratlist[i], ratio) == 0)
			vo.rati = i;
	vo.texquality = 0;
	if(trap_Cvar_VariableValue("r_texturebits") == 16)
		vo.texquality = 1;
	vo.vsync = (qboolean)trap_Cvar_VariableValue("r_swapinterval");
	vo.fov = trap_Cvar_VariableValue("cg_fov");
	vo.drawfps = trap_Cvar_VariableValue("cg_drawfps");
	vo.fullscr = (qboolean)trap_Cvar_VariableValue("r_fullscreen");
	vo.gamma = trap_Cvar_VariableValue("r_gamma");
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
		int w, h;
		char resstr[16];

		vo.nres = 0;
		vo.resi = 0;
		w = trap_Cvar_VariableValue("r_customwidth");
		h = trap_Cvar_VariableValue("r_customheight");
		Com_sprintf(resstr, sizeof resstr, "%dx%d", w, h);
		memset(vo.reslist, 0, sizeof vo.reslist);
		for(i = 0, j = 0; resolutions[i] != NULL; i++)
			if(Q_stricmp(ratios[i], vo.ratlist[vo.rati]) == 0){
				vo.reslist[j++] = resolutions[i];
				vo.nres++;
			}
		for(i = 0; vo.reslist[i] != NULL; i++)
			if(Q_stricmp(vo.reslist[i], resstr) == 0)
				vo.resi = i;
		vo.needrestart = qtrue;
	}
	y += spc;

	drawstr(x, y, "Refresh rate", UI_RIGHT|UI_DROPSHADOW, color_white);
	if(textfield(".v.hz", xx, y, 0, 4, vo.hz, &vo.hzcaret, sizeof vo.hz))
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
