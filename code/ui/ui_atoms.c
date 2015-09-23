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

/*
User interface building blocks and support functions.
Coordinates are 640*480 virtual values
*/

#include "ui_local.h"

vec4_t CBlack		= {0.000f, 0.000f, 0.000f, 1.000f};
vec4_t CWhite		= {1.000f, 1.000f, 1.000f, 1.000f};
vec4_t CAmethyst	= {0.600f, 0.400f, 0.800f, 1.000f};
vec4_t CApple		= {0.553f, 0.714f, 0.000f, 1.000f};
vec4_t CAquamarine	= {0.498f, 1.000f, 0.831f, 1.000f};
vec4_t CBlue		= {0.153f, 0.231f, 0.886f, 1.000f};
vec4_t CBrown		= {0.596f, 0.463f, 0.329f, 1.000f};
vec4_t CCream		= {1.000f, 0.992f, 0.816f, 1.000f};
vec4_t CCyan		= {0.000f, 1.000f, 1.000f, 1.000f};
vec4_t CDkBlue		= {0.063f, 0.204f, 0.651f, 1.000f};
vec4_t CDkGreen		= {0.000f, 0.420f, 0.235f, 1.000f};
vec4_t CDkGrey		= {0.600f, 0.600f, 0.600f, 1.000f};
vec4_t CDkLavender	= {0.451f, 0.310f, 0.588f, 1.000f};
vec4_t CGreen		= {0.000f, 1.000f, 0.498f, 1.000f};
vec4_t CIndigo		= {0.294f, 0.000f, 0.510f, 1.000f};
vec4_t CLtBlue		= {0.000f, 0.749f, 1.000f, 1.000f};
vec4_t CLtGreen		= {0.247f, 1.000f, 0.000f, 1.000f};
vec4_t CLtGrey		= {0.827f, 0.827f, 0.827f, 1.000f};
vec4_t CLtMagenta	= {0.957f, 0.604f, 0.761f, 1.000f};
vec4_t CLtOrange	= {1.000f, 0.624f, 0.000f, 1.000f};
vec4_t CMagenta		= {1.000f, 0.000f, 1.000f, 1.000f};
vec4_t COrange		= {1.000f, 0.400f, 0.000f, 1.000f};
vec4_t CPink		= {0.925f, 0.231f, 0.514f, 1.000f};
vec4_t CPurple		= {0.400f, 0.000f, 0.600f, 1.000f};
vec4_t CRed		= {0.890f, 0.149f, 0.212f, 1.000f};
vec4_t CTeal		= {0.000f, 0.502f, 0.502f, 1.000f};
vec4_t CViolet		= {0.624f, 0.000f, 1.000f, 1.000f};
vec4_t CYellow		= {1.000f, 0.749f, 0.000f, 1.000f};

float *CText		= CWhite;
float *CHot		= CLtGreen;
float *CActive		= CLtBlue;

uiStatic_t uis;

char *
Argv(int arg)
{
	static char buffer[MAX_STRING_CHARS];

	trap_Argv(arg, buffer, sizeof(buffer));
	return buffer;
}

void QDECL
Com_Error(int level, const char *error, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	trap_Error(text);
}

void QDECL
Com_Printf(const char *msg, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	trap_Print(text);
}

char *
UI_Cvar_VariableString(const char *var_name)
{
	static char buffer[MAX_STRING_CHARS];

	trap_Cvar_VariableStringBuffer(var_name, buffer, sizeof(buffer));
	return buffer;
}

float
UI_ClampCvar(float min, float max, float value)
{
	if(value < min)
		return min;
	if(value > max)
		return max;
	return value;
}

void
startdemoloop(void)
{
	trap_Cmd_ExecuteText(EXEC_APPEND, "d1\n");
}

void
lerpcolour(vec4_t a, vec4_t b, vec4_t c, float t)
{
	int i;

	// lerp and clamp each component
	for(i = 0; i<4; i++){
		c[i] = a[i] + t*(b[i]-a[i]);
		if(c[i] < 0)
			c[i] = 0;
		else if(c[i] > 1.0)
			c[i] = 1.0;
	}
}

// proportional font
static int propMap[128][3] = {
	{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},
	{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},

	{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},
	{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},

	{0, 0, PROP_SPACE_WIDTH},	// SPACE
	{11, 122, 7},			// !
	{154, 181, 14},			// "
	{55, 122, 17},			// #
	{79, 122, 18},			// $
	{101, 122, 23},			// %
	{153, 122, 18},			// &
	{9, 93, 7},			// '
	{207, 122, 8},			// (
	{230, 122, 9},			// )
	{177, 122, 18},			// *
	{30, 152, 18},			// +
	{85, 181, 7},			// ,
	{34, 93, 11},			// -
	{110, 181, 6},			// .
	{130, 152, 14},			// /

	{22, 64, 17},			// 0
	{41, 64, 12},			// 1
	{58, 64, 17},			// 2
	{78, 64, 18},			// 3
	{98, 64, 19},			// 4
	{120, 64, 18},			// 5
	{141, 64, 18},			// 6
	{204, 64, 16},			// 7
	{162, 64, 17},			// 8
	{182, 64, 18},			// 9
	{59, 181, 7},			// :
	{35, 181, 7},			// ;
	{203, 152, 14},			// <
	{56, 93, 14},			// =
	{228, 152, 14},			// >
	{177, 181, 18},			// ?

	{28, 122, 22},			// @
	{5, 4, 18},			// A
	{27, 4, 18},			// B
	{48, 4, 18},			// C
	{69, 4, 17},			// D
	{90, 4, 13},			// E
	{106, 4, 13},			// F
	{121, 4, 18},			// G
	{143, 4, 17},			// H
	{164, 4, 8},			// I
	{175, 4, 16},			// J
	{195, 4, 18},			// K
	{216, 4, 12},			// L
	{230, 4, 23},			// M
	{6, 34, 18},			// N
	{27, 34, 18},			// O

	{48, 34, 18},			// P
	{68, 34, 18},			// Q
	{90, 34, 17},			// R
	{110, 34, 18},			// S
	{130, 34, 14},			// T
	{146, 34, 18},			// U
	{166, 34, 19},			// V
	{185, 34, 29},			// W
	{215, 34, 18},			// X
	{234, 34, 18},			// Y
	{5, 64, 14},			// Z
	{60, 152, 7},			// [
	{106, 151, 13},			// '\'
	{83, 152, 7},			// ]
	{128, 122, 17},			// ^
	{4, 152, 21},			// _

	{134, 181, 5},			// '
	{5, 4, 18},			// A
	{27, 4, 18},			// B
	{48, 4, 18},			// C
	{69, 4, 17},			// D
	{90, 4, 13},			// E
	{106, 4, 13},			// F
	{121, 4, 18},			// G
	{143, 4, 17},			// H
	{164, 4, 8},			// I
	{175, 4, 16},			// J
	{195, 4, 18},			// K
	{216, 4, 12},			// L
	{230, 4, 23},			// M
	{6, 34, 18},			// N
	{27, 34, 18},			// O

	{48, 34, 18},			// P
	{68, 34, 18},			// Q
	{90, 34, 17},			// R
	{110, 34, 18},			// S
	{130, 34, 14},			// T
	{146, 34, 18},			// U
	{166, 34, 19},			// V
	{185, 34, 29},			// W
	{215, 34, 18},			// X
	{234, 34, 18},			// Y
	{5, 64, 14},			// Z
	{153, 152, 13},			// {
	{11, 181, 5},			// |
	{180, 152, 13},			// }
	{79, 93, 17},			// ~
	{0, 0, -1}			// DEL
};

static int propMapB[26][3] = {
	{11, 12, 33},
	{49, 12, 31},
	{85, 12, 31},
	{120, 12, 30},
	{156, 12, 21},
	{183, 12, 21},
	{207, 12, 32},

	{13, 55, 30},
	{49, 55, 13},
	{66, 55, 29},
	{101, 55, 31},
	{135, 55, 21},
	{158, 55, 40},
	{204, 55, 32},

	{12, 97, 31},
	{48, 97, 31},
	{82, 97, 30},
	{118, 97, 30},
	{153, 97, 30},
	{185, 97, 25},
	{213, 97, 30},

	{11, 139, 32},
	{42, 139, 51},
	{93, 139, 32},
	{126, 139, 31},
	{158, 139, 25},
};

#define PROPB_GAP_WIDTH		4
#define PROPB_SPACE_WIDTH	12
#define PROPB_HEIGHT		36

static void
drawbannerstring2(int x, int y, const char *str, vec4_t color)
{
	const char *s;
	uchar ch;
	float ax;
	float ay;
	float aw;
	float ah;
	float frow;
	float fcol;
	float fwidth;
	float fheight;

	// draw the colored text
	trap_R_SetColor(color);

	ax = x * uis.xscale + uis.bias;
	ay = y * uis.yscale;

	s = str;
	while(*s){
		ch = *s & 127;
		if(ch == ' ')
			ax += ((float)PROPB_SPACE_WIDTH + (float)PROPB_GAP_WIDTH)* uis.xscale;
		else if(ch >= 'A' && ch <= 'Z'){
			ch -= 'A';
			fcol = (float)propMapB[ch][0] / 256.0f;
			frow = (float)propMapB[ch][1] / 256.0f;
			fwidth = (float)propMapB[ch][2] / 256.0f;
			fheight = (float)PROPB_HEIGHT / 256.0f;
			aw = (float)propMapB[ch][2] * uis.xscale;
			ah = (float)PROPB_HEIGHT * uis.yscale;
			trap_R_DrawStretchPic(ax, ay, aw, ah, fcol, frow, fcol+fwidth, frow+fheight, uis.charsetPropB);
			ax += (aw + (float)PROPB_GAP_WIDTH * uis.xscale);
		}
		s++;
	}

	trap_R_SetColor(nil);
}

/*
Find the width of the drawn text.
*/
void
drawbannerstring(int x, int y, const char *str, int style, vec4_t color)
{
	const char *s;
	int ch;
	int width;
	vec4_t drawcolor;

	s = str;
	width = 0;
	while(*s){
		ch = *s;
		if(ch == ' ')
			width += PROPB_SPACE_WIDTH;
		else if(ch >= 'A' && ch <= 'Z')
			width += propMapB[ch - 'A'][2] + PROPB_GAP_WIDTH;
		s++;
	}
	width -= PROPB_GAP_WIDTH;

	switch(style & UI_FORMATMASK){
	case UI_CENTER:
		x -= width / 2;
		break;

	case UI_RIGHT:
		x -= width;
		break;

	case UI_LEFT:
	default:
		break;
	}

	if(style & UI_DROPSHADOW){
		drawcolor[0] = drawcolor[1] = drawcolor[2] = 0;
		drawcolor[3] = color[3] * Shadowalpha;
		drawbannerstring2(x+2, y+2, str, drawcolor);
	}

	drawbannerstring2(x, y, str, color);
}

/*
Sliceend=-1 means up to the terminating \0.
*/
int
propstrwidth(const char *str, int slicebegin, int sliceend)
{
	int ch, charWidth, width, i;

	width = 0;
	for(i = slicebegin; i < sliceend || (sliceend == -1 && str[i] != '\0'); i++){
		ch = str[i] & 127;
		charWidth = propMap[ch][2];
		if(charWidth != -1){
			width += charWidth;
			width += PROP_GAP_WIDTH;
		}
	}
	width -= PROP_GAP_WIDTH;
	return width;
}

static void
drawpropstr2(int x, int y, const char *str, vec4_t color, float sizeScale, qhandle_t charset)
{
	const char *s;
	uchar ch;
	float ax, ay, aw, ah;
	float frow, fcol, fwidth, fheight;

	// draw the colored text
	trap_R_SetColor(color);

	ax = x * uis.xscale + uis.bias;
	ay = y * uis.yscale;
	aw = 0;

	s = str;
	while(*s){
		ch = *s & 127;
		if(ch == ' ')
			aw = (float)PROP_SPACE_WIDTH * uis.xscale * sizeScale;
		else if(propMap[ch][2] != -1){
			fcol = (float)propMap[ch][0] / 256.0f;
			frow = (float)propMap[ch][1] / 256.0f;
			fwidth = (float)propMap[ch][2] / 256.0f;
			fheight = (float)PROP_HEIGHT / 256.0f;
			aw = (float)propMap[ch][2] * uis.xscale * sizeScale;
			ah = (float)PROP_HEIGHT * uis.yscale * sizeScale;
			trap_R_DrawStretchPic(ax, ay, aw, ah, fcol, frow, fcol+fwidth, frow+fheight, charset);
		}
		ax += (aw + (float)PROP_GAP_WIDTH * uis.xscale * sizeScale);
		s++;
	}
	trap_R_SetColor(nil);
}

float
propstrsizescale(int style)
{
	if(style & UI_SMALLFONT)
		return PROP_SMALL_SIZE_SCALE;
	return 1.00;
}

void
drawpropstr(int x, int y, const char *str, int style, vec4_t color)
{
	vec4_t drawcolor;
	int width;
	float sizeScale;

	sizeScale = propstrsizescale(style);

	switch(style & UI_FORMATMASK){
	case UI_CENTER:
		width = propstrwidth(str, 0, -1) * sizeScale;
		x -= width / 2;
		break;
	case UI_RIGHT:
		width = propstrwidth(str, 0, -1) * sizeScale;
		x -= width;
		break;
	case UI_LEFT:
	default:
		break;
	}
	if(style & UI_DROPSHADOW){
		drawcolor[0] = drawcolor[1] = drawcolor[2] = 0;
		drawcolor[3] = color[3] * Shadowalpha;
		drawpropstr2(x+2, y+2, str, drawcolor, sizeScale, uis.charsetProp);
	}
	if(style & UI_INVERSE){
		drawcolor[0] = color[0] * 0.7;
		drawcolor[1] = color[1] * 0.7;
		drawcolor[2] = color[2] * 0.7;
		drawcolor[3] = color[3];
		drawpropstr2(x, y, str, drawcolor, sizeScale, uis.charsetProp);
		return;
	}
	if(style & UI_PULSE){
		drawcolor[0] = color[0] * 0.7;
		drawcolor[1] = color[1] * 0.7;
		drawcolor[2] = color[2] * 0.7;
		drawcolor[3] = color[3];
		drawpropstr2(x, y, str, color, sizeScale, uis.charsetProp);

		drawcolor[0] = color[0];
		drawcolor[1] = color[1];
		drawcolor[2] = color[2];
		drawcolor[3] = 0.5 + 0.5 * sin(uis.realtime / PULSE_DIVISOR);
		drawpropstr2(x, y, str, drawcolor, sizeScale, uis.charsetPropGlow);
		return;
	}
	drawpropstr2(x, y, str, color, sizeScale, uis.charsetProp);
}

void
drawpropstrwrapped(int x, int y, int xmax, int ystep, const char *str, int style, vec4_t color)
{
	int width;
	char *s1, *s2, *s3;
	char c_bcp;
	char buf[1024];
	float sizeScale;

	if(!str || str[0]=='\0')
		return;

	sizeScale = propstrsizescale(style);

	Q_strncpyz(buf, str, sizeof(buf));
	s1 = s2 = s3 = buf;

	while(1){
		do
			s3++;
		while(*s3!=' ' && *s3!='\0')
		;
		c_bcp = *s3;
		*s3 = '\0';
		width = propstrwidth(s1, 0, -1) * sizeScale;
		*s3 = c_bcp;
		if(width > xmax){
			if(s1==s2)
				// fuck, don't have a clean cut, we'll overflow
				s2 = s3;
			*s2 = '\0';
			drawpropstr(x, y, s1, style, color);
			y += ystep;
			if(c_bcp == '\0'){
				// that was the last word
				// we could start a new loop, but that wouldn't be much use
				// even if the word is too long, we would overflow it (see above)
				// so just print it now if needed
				s2++;
				if(*s2 != '\0')	// if we are printing an overflowing line we have s2 == s3
					drawpropstr(x, y, s2, style, color);
				break;
			}
			s2++;
			s1 = s2;
			s3 = s2;
		}else{
			s2 = s3;
			if(c_bcp == '\0'){	// we reached the end
				drawpropstr(x, y, s1, style, color);
				break;
			}
		}
	}
}

static void
drawstr2(int x, int y, const char *str, vec4_t color, int charw, int charh)
{
	const char *s;
	char ch;
	int forceColor = qfalse;//APSFIXME;
	vec4_t tempcolor;
	float ax, ay, aw, ah, frow, fcol;

	if(y < -charh)
		// offscreen
		return;

	// draw the colored text
	trap_R_SetColor(color);

	ax = x * uis.xscale + uis.bias;
	ay = y * uis.yscale;
	aw = charw * uis.xscale;
	ah = charh * uis.yscale;

	s = str;
	while(*s){
		if(Q_IsColorString(s)){
			if(!forceColor){
				memcpy(tempcolor, g_color_table[ColorIndex(s[1])],
				   sizeof(tempcolor));
				tempcolor[3] = color[3];
				trap_R_SetColor(tempcolor);
			}
			s += 2;
			continue;
		}
		ch = *s & 255;
		if(ch != ' '){
			frow = (ch>>4)*0.0625f;
			fcol = (ch&15)*0.0625f;
			trap_R_DrawStretchPic(ax, ay, aw, ah, fcol, frow, fcol + 0.0625f,
			   frow + 0.0625f, uis.charset);
		}
		ax += aw;
		s++;
	}
	trap_R_SetColor(nil);
}

void
drawstr(int x, int y, const char *str, int style, vec4_t color)
{
	int len;
	int charw;
	int charh;
	vec4_t newcolor;
	vec4_t lowlight;
	float *drawcolor;
	vec4_t dropcolor;

	if(!str)
		return;

	if((style & UI_BLINK) && ((uis.realtime/BLINK_DIVISOR) & 1))
		return;

	if(style & UI_SMALLFONT){
		charw = SMALLCHAR_WIDTH;
		charh = SMALLCHAR_HEIGHT;
	}else if(style & UI_GIANTFONT){
		charw = GIANTCHAR_WIDTH;
		charh = GIANTCHAR_HEIGHT;
	}else{
		charw = BIGCHAR_WIDTH;
		charh = BIGCHAR_HEIGHT;
	}

	if(style & UI_PULSE){
		lowlight[0] = 0.8*color[0];
		lowlight[1] = 0.8*color[1];
		lowlight[2] = 0.8*color[2];
		lowlight[3] = 0.8*color[3];
		lerpcolour(color, lowlight, newcolor, 0.5+0.5*sin(uis.realtime/PULSE_DIVISOR));
		drawcolor = newcolor;
	}else
		drawcolor = color;

	switch(style & UI_FORMATMASK){
	case UI_CENTER:
		len = strlen(str);
		x = x - len*charw/2;
		break;
	case UI_RIGHT:
		len = strlen(str);
		x = x - len*charw;
		break;
	default:
		break;
	}
	if(style & UI_DROPSHADOW){
		dropcolor[0] = dropcolor[1] = dropcolor[2] = 0;
		dropcolor[3] = drawcolor[3] * Shadowalpha;
		drawstr2(x+2, y+2, str, dropcolor, charw, charh);
	}
	drawstr2(x, y, str, drawcolor, charw, charh);
}

void
drawstrwrapped(int x, int y, int xmax, int ystep, const char *str, int style, vec4_t color)
{
	int len, width, charw, charh;
	vec4_t newcolor;
	vec4_t lowlight;
	float *drawcolor;
	vec4_t dropcolor;
	char *s1, *s2, *s3;
	char buf[1024];
	int c;

	if(str == nil)
		return;

	Q_strncpyz(buf, str, sizeof buf);
	s1 = s2 = s3 = buf;

	if(style & UI_SMALLFONT){
		charw = SMALLCHAR_WIDTH;
		charh = SMALLCHAR_HEIGHT;
	}else if(style & UI_GIANTFONT){
		charw = GIANTCHAR_WIDTH;
		charh = GIANTCHAR_HEIGHT;
	}else{
		charw = BIGCHAR_WIDTH;
		charh = BIGCHAR_HEIGHT;
	}

	if(style & UI_PULSE){
		lowlight[0] = 0.8f*color[0];
		lowlight[1] = 0.8f*color[1];
		lowlight[2] = 0.8f*color[2];
		lowlight[3] = 0.8f*color[3];
		lerpcolour(color, lowlight, newcolor, 0.5+0.5*sin(uis.realtime/PULSE_DIVISOR));
		drawcolor = newcolor;
	}else
		drawcolor = color;

	switch(style & UI_FORMATMASK){
	case UI_CENTER:
		len = strlen(str);
		x = x - len*charw/2;
		break;
	case UI_RIGHT:
		len = strlen(str);
		x = x - len*charw;
		break;
	default:
		break;
	}

	dropcolor[0] = dropcolor[1] = dropcolor[2] = 0;
	dropcolor[3] = color[3] * Shadowalpha;

	for(;;){
		do{
			s3++;
		}while(*s3 != ' ' && *s3 != '\0');
		c = *s3;
		*s3 = '\0';
		width = charw*strlen(s1);
		*s3 = c;
		if(width > xmax){
			if(s1 == s2)	// overflow
				s2 = s3;
			*s2 = '\0';
			if(style & UI_DROPSHADOW)
				drawstr2(x+2, y+2, s1, dropcolor, charw, charh);
			drawstr2(x, y, s1, drawcolor, charw, charh);
			y += ystep;
			if(c == '\0'){
				s2++;
				if(*s2 != '\0'){
					if(style & UI_DROPSHADOW)
						drawstr2(x+2, y+2, s2, dropcolor, charw, charh);
					drawstr2(x, y, s2, drawcolor, charw, charh);
				}
				break;
			}
			s2++;
			s1 = s2;
			s3 = s2;
		}else{
			s2 = s3;
			if(c == '\0'){
				if(style & UI_DROPSHADOW)
					drawstr2(x+2, y+2, s1, dropcolor, charw, charh);
				drawstr2(x, y, s1, drawcolor, charw, charh);
				break;
			}
		}
	}
}

void
drawchar(int x, int y, int ch, int style, vec4_t color)
{
	char buff[2];

	buff[0] = ch;
	buff[1] = '\0';
	drawstr(x, y, buff, style, color);
}

/*
This should be the ONLY way the menu system is brought up.
*/
void
setactivemenu(uiMenuCommand_t menu)
{
	cacheui();

	switch(menu){
	case UIMENU_NONE:
		dismissui();
		return;
	case UIMENU_MAIN: {
		char buf[MAX_STRING_CHARS];

		uis.sp = -1;
		memset(uis.keys, 0, sizeof uis.keys);
		trap_Cvar_VariableStringBuffer("com_errormessage", buf, sizeof buf);
		if(strlen(buf) > 0)
			push(errormenu);
		else
			push(mainmenu);
		return;
	}
	case UIMENU_INGAME:
		trap_Cvar_Set("cl_paused", "1");
		uis.sp = -1;
		push(ingamemenu);
		return;
	default:
#ifndef NDEBUG
		Com_Printf("setactivemenu: bad enum %d\n", menu);
#endif
		break;
	}
}

void
keyevent(int key, int down)
{
	if(key < 0 || key >= MAX_KEYS)
		return;
	uis.keys[key] = down;
}

void
charevent(int ch)
{
	if(uis.texti < TEXTLEN-2)
		uis.text[uis.texti++] = ch;
	uis.text[TEXTLEN-1] = '\0';
	uis.keys[tolower(ch)] = qtrue;
}

void
mouseevent(int dx, int dy)
{
	uis.cursorx += dx;
	if(uis.cursorx < -uis.bias)
		uis.cursorx = -uis.bias;
	else if(uis.cursorx > SCREEN_WIDTH+uis.bias)
		uis.cursorx = SCREEN_WIDTH+uis.bias;
	uis.cursory += dy;
	if(uis.cursory < 0)
		uis.cursory = 0;
	else if(uis.cursory > SCREEN_HEIGHT)
		uis.cursory = SCREEN_HEIGHT;
}

void
cacheui(void)
{
	uis.charset = trap_R_RegisterShaderNoMip("gfx/2d/bigchars");
	uis.charsetProp = trap_R_RegisterShaderNoMip("menu/art/font1_prop");
	uis.charsetPropGlow = trap_R_RegisterShaderNoMip("menu/art/font1_prop_glo");
	uis.charsetPropB = trap_R_RegisterShaderNoMip("menu/art/font2_prop");
	uis.cursor = trap_R_RegisterShaderNoMip("menu/art/cursor");
	uis.whiteShader = trap_R_RegisterShaderNoMip("white");
	uis.menuBackShader = trap_R_RegisterShaderNoMip("menuback");
	uis.fieldUpdateSound = trap_S_RegisterSound("sound/misc/menu2", qfalse);
}

qboolean
consolecommand(int realTime)
{
	char *cmd;

	uis.frametime = realTime - uis.realtime;
	uis.realtime = realTime;

	cmd = Argv(0);

	if(Q_stricmp(cmd, "ui_cache") == 0){
		cacheui();
		return qtrue;
	}
	return qfalse;
}

void
shutdown(void)
{
}

void
init(void)
{
	registercvars();

	trap_GetGlconfig(&uis.glconfig);

	// for 640x480 virtualized screen
	uis.xscale = uis.glconfig.vidWidth * (1.0/640.0);
	uis.yscale = uis.glconfig.vidHeight * (1.0/480.0);
	if(uis.glconfig.vidWidth * 480 > uis.glconfig.vidHeight * 640){
		// widescreen
		uis.bias = 0.5 * (uis.glconfig.vidWidth - (uis.glconfig.vidHeight * (640.0/480.0)));
		uis.xscale = uis.yscale;
	}else
		uis.bias = 0;
	cacheui();
	uis.firstdraw = qtrue;
	uis.sp = -1;
	uis.fullscreen = qfalse;
}

/*
Adjust for resolution and screen aspect ratio
*/
void
adjustcoords(float *x, float *y, float *w, float *h)
{
	// expect valid pointers
	*x = *x * uis.xscale + uis.bias;
	*y *= uis.yscale;
	*w *= uis.xscale;
	*h *= uis.yscale;
}

void
drawnamedpic(float x, float y, float width, float height, const char *picname)
{
	qhandle_t hShader;

	hShader = trap_R_RegisterShaderNoMip(picname);
	adjustcoords(&x, &y, &width, &height);
	trap_R_DrawStretchPic(x, y, width, height, 0, 0, 1, 1, hShader);
}

void
drawpic(float x, float y, float w, float h, qhandle_t hShader)
{
	float s0;
	float s1;
	float t0;
	float t1;

	if(w < 0){	// flip about vertical
		w = -w;
		s0 = 1;
		s1 = 0;
	}else{
		s0 = 0;
		s1 = 1;
	}

	if(h < 0){	// flip about horizontal
		h = -h;
		t0 = 1;
		t1 = 0;
	}else{
		t0 = 0;
		t1 = 1;
	}

	adjustcoords(&x, &y, &w, &h);
	trap_R_DrawStretchPic(x, y, w, h, s0, t0, s1, t1, hShader);
}

void
fillrect(float x, float y, float width, float height, const float *color)
{
	trap_R_SetColor(color);

	adjustcoords(&x, &y, &width, &height);
	trap_R_DrawStretchPic(x, y, width, height, 0, 0, 0, 0, uis.whiteShader);

	trap_R_SetColor(nil);
}

void
drawrect(float x, float y, float width, float height, const float *color)
{
	trap_R_SetColor(color);

	adjustcoords(&x, &y, &width, &height);

	trap_R_DrawStretchPic(x, y, width, 1, 0, 0, 0, 0, uis.whiteShader);
	trap_R_DrawStretchPic(x, y, 1, height, 0, 0, 0, 0, uis.whiteShader);
	trap_R_DrawStretchPic(x, y + height - 1, width, 1, 0, 0, 0, 0, uis.whiteShader);
	trap_R_DrawStretchPic(x + width - 1, y, 1, height, 0, 0, 0, 0, uis.whiteShader);

	trap_R_SetColor(nil);
}

void
setcolour(const float *rgba)
{
	trap_R_SetColor(rgba);
}

void
updatescreen(void)
{
	trap_UpdateScreen();
}

static void
drawfps(void)
{
	enum {FRAMES = 4};
	static int prevtimes[FRAMES], index, previous;
	int i, total, t, frametime, fps;
	char s[16];

	t = trap_Milliseconds();
	frametime = t - previous;
	previous = t;
	prevtimes[index % FRAMES] = frametime;
	index++;
	if(index > FRAMES){
		total = 0;
		for(i = 0; i < FRAMES; i++)
			total += prevtimes[i];
		if(total <= 0)
			total = 1;
		fps = 1000 * FRAMES / total;
		Com_sprintf(s, sizeof s, "%ifps", fps);
		drawstr(638, 2, s, UI_RIGHT|UI_SMALLFONT, CWhite);
	}
}

void
push(menuFn_t f)
{
	if(uis.sp >= NSTACK)
		Com_Error(ERR_FATAL, "ui stack overflow");
	uis.sp++;
	uis.stk[uis.sp] = f;
	idcpy(uis.focus, "");
	trap_Key_SetCatcher(KEYCATCH_UI);
}

void
pop(void)
{
	if(uis.sp < 0)
		Com_Error(ERR_FATAL, "ui stack underflow");
	uis.sp--;
	if(uis.sp < 0)
		dismissui();
}

void
dismissui(void)
{
	uis.sp = -1;
	trap_Key_SetCatcher(trap_Key_GetCatcher() & ~KEYCATCH_UI);
	trap_Key_ClearStates();
	trap_Cvar_Set("cl_paused", 0);
}

void
refresh(int realtime)
{
	uis.frametime = realtime - uis.realtime;
	uis.realtime = realtime;

	if(!(trap_Key_GetCatcher() & KEYCATCH_UI)){
		uis.fullscreen = qfalse;
		return;
	}
	if(uis.firstdraw){
		mouseevent(SCREEN_WIDTH/2, SCREEN_HEIGHT/2);
		uis.firstdraw = qfalse;
	}
	uis.fullscreen = qfalse;

	updatecvars();

	if(uis.sp >= 0)
		uis.stk[uis.sp]();

	if(ui_drawfps.integer)
		drawfps();

	// draw cursor
	setcolour(nil);
	drawpic(uis.cursorx-16, uis.cursory-16, 32, 32, uis.cursor);

#ifndef NDEBUG
	if(uis.debug)
		// cursor coordinates
		drawstr(0, 0, va("(%d,%d)", uis.cursorx, uis.cursory), UI_LEFT|UI_SMALLFONT, colorRed);

#endif

	// finish the frame
	if(!uis.keys[K_MOUSE1])
		idcpy(uis.active, "");
	else if(idcmp(uis.active, ""))
		idcpy(uis.active, "minusone");
	memset(uis.text, 0, TEXTLEN);
	memset(uis.keys+'0', 0, '0'+'9');
	memset(uis.keys+'A', 0, 'A'+'Z');
	uis.texti = 0;
}

qboolean
mouseover(int x, int y, int w, int h)
{
	if(uis.cursorx < x || uis.cursory < y ||
	   uis.cursorx > x+w || uis.cursory > y+h)
		return qfalse;
	return qtrue;
}
