#include "ui_local.h"

qboolean
idcmp(const char *a, const char *b)
{
	return strcmp(a, b) == 0;
}

void
idcpy(char *dst, const char *src)
{
	Q_strncpyz(dst, src, MAXIDLEN);
}

static void
justify(int just, int *x, int w)
{
	switch(just){
	case UI_CENTER:
		*x = *x - w/2;
		break;
	case UI_RIGHT:
		*x = *x - w;
		break;
	}
}

// Focus occurs by draw order.
static qboolean
yieldfocus(const char *id)
{
	if(!uis.keys[K_TAB])
		return qfalse;
	if(uis.keys[K_SHIFT])
		idcpy(uis.focus, uis.lastfocus);
	else
		idcpy(uis.focus, "");
	idcpy(uis.lastfocus, id);
	uis.keys[K_TAB] = qfalse;
	return qtrue;
}

qboolean
button(const char *id, int x, int y, int just, const char *label)
{
	float w = 48, h = 28;
	float propw;
	qboolean hot;
	float *clr;

	hot = qfalse;
	propw = propstrwidth(label, 0, -1);
	if(w < propw)
		w = propw;
	justify(just, &x, w);

	if(idcmp(uis.focus, ""))
		idcpy(uis.focus, id);
	if(mouseover(x, y, w, h)){
		idcpy(uis.hot, id);
		hot = qtrue;
		if(idcmp(uis.active, "") && uis.keys[K_MOUSE1])
			idcpy(uis.active, id);
	}

	yieldfocus(id);

	clr = CLtBlue;
	if(hot)
		if(idcmp(uis.active, id))
			clr = CLtOrange;
		else
			clr = CLtGreen;
	drawpropstr(x+w/2, y, label, UI_CENTER|UI_DROPSHADOW, clr);
	return !uis.keys[K_MOUSE1] && idcmp(uis.hot, id) && idcmp(uis.active, id);
}

qboolean
slider(const char *id, int x, int y, int just, float min, float max, float *val, const char *displayfmt)
{
	float w = 120, pad = 2, h = 12, knobw = 6, knobh = 18, ix, iy, iw;
	qboolean hot, updated;
	float knobpos, mousepos;
	char s[32];
	float *clr;

	hot = qfalse;
	updated = qfalse;
	justify(just, &x, w);
	ix = x+pad;
	iy = y+pad;
	iw = w - 2*pad;

	if(idcmp(uis.focus, ""))
		idcpy(uis.focus, id);
	if(mouseover(ix, y + h/2 - knobh/2, iw, knobh)){
		idcpy(uis.hot, id);
		hot = qtrue;
		if(idcmp(uis.active, "") && uis.keys[K_MOUSE1])
			idcpy(uis.active, id);
	}
	*val = Com_Scale(*val, min, max, 0, max);
	if(idcmp(uis.active, id)){
		float v;

		mousepos = uis.cursorx - ix;
		if(mousepos < 0)
			mousepos = 0;
		if(mousepos > iw)
			mousepos = iw;
		v = (mousepos * max) / iw;
		updated = (*val != v);
		*val = v;
	}

	yieldfocus(id);

	fillrect(x, y, w, h, CLtBlue);
	drawrect(x, y, w, h, CBlack);
	knobpos = (iw * *val) / max;
	*val = Com_Scale(*val, 0, max, min, max);
	if(hot || idcmp(uis.active, id))
		clr = CLtOrange;
	else
		clr = CPurple;
	fillrect(ix + knobpos - knobw/2, y + h/2 - knobh/2, knobw, knobh, clr);
	drawrect(ix + knobpos - knobw/2, y + h/2 - knobh/2, knobw, knobh, CBlack);
	if(*displayfmt != '\0'){
		Com_sprintf(s, sizeof s, displayfmt, *val);
		drawstr(x+w+6, iy, s, UI_LEFT|UI_SMALLFONT|UI_DROPSHADOW, CLtGreen);
		*val = atof(s);
	}
	return updated;
}

qboolean
checkbox(const char *id, int x, int y, int just, qboolean *state)
{
	const float w = 16, h = 16;
	qboolean hot;

	hot = qfalse;
	justify(just, &x, w);

	if(idcmp(uis.focus, ""))
		idcpy(uis.focus, id);
	if(mouseover(x, y, w, h)){
		idcpy(uis.hot, id);
		hot = qtrue;
		if(idcmp(uis.active, "") && uis.keys[K_MOUSE1]){
			idcpy(uis.active, id);
			*state = !*state;
		}
	}

	yieldfocus(id);

	fillrect(x, y, w, h, CLtBlue);
	drawrect(x, y, w, h, CBlack);
	if(*state){
		setcolour(CBlack);
		drawnamedpic(x+2, y+2, w, h, "menu/art/tick");
		if(hot)
			setcolour(COrange);
		else
			setcolour(CPink);
		drawnamedpic(x, y, w, h, "menu/art/tick");
		setcolour(nil);
	}
	return !uis.keys[K_MOUSE1] && idcmp(uis.hot, id) && idcmp(uis.active, id);
}

static qboolean
updatefield(char *buf, int *caret, int sz)
{
	char *p;
	int i;
	qboolean updated;

	updated = qfalse;

	for(p = uis.text, i = *caret; *p != '\0'; p++){
		switch(*p){
		case 1:	// ^A
			i = 0;
			break;
		case 3:	// ^C
			buf[0] = '\0';
			i = 0;
			break;
		case 5:	// ^E
			i = (int)(strchr(buf, '\0') - buf);
			break;
		case 8:	// backspace
			if(i > 0){
				if(i < sz)
					memcpy(buf+i-1, buf+i, sz-i);
				else
					buf[i-1] = '\0';
				i--;
			}
			break;
		default:
			if(i < sz-1 && strlen(buf) < sz-1){
				if(*p >= ' '){
					memcpy(buf+i+1, buf+i, sz-i-1);
					buf[i] = *p;
				}
				i++;
			}
		}
		updated = qtrue;
	}

	if(uis.keys[K_LEFTARROW]){
		if(i > 0)
			i = i - 1;
		uis.keys[K_LEFTARROW] = qfalse;
	}
	if(uis.keys[K_RIGHTARROW]){
		if(i < sz-1 && buf[i] != '\0')
			i = i + 1;
		uis.keys[K_RIGHTARROW] = qfalse;
	}
	if(uis.keys[K_HOME]){
		i = 0;
		uis.keys[K_HOME] = qfalse;
	}
	if(uis.keys[K_END]){
		i = (int)(strchr(buf, '\0') - buf);
		uis.keys[K_END] = qfalse;
	}
	if(uis.keys[K_DEL]){
		buf[i] = '\0';
		if(i < sz-1)
			memcpy(buf+i, buf+i+1, sz-i-1);
		uis.keys[K_DEL] = qfalse;
		updated = qtrue;
	}
	*caret = i;
	return updated;
}

qboolean
textfield(const char *id, int x, int y, int just, int width, char *buf, int *caret, int sz)
{
	const float w = width*SMALLCHAR_WIDTH;
	const float h = 16, pad = 4;
	qboolean hot, updated;
	int i, caretpos;

	hot = qfalse;
	updated = qfalse;
	justify(just, &x, w);

	if(idcmp(uis.focus, ""))
		idcpy(uis.focus, id);
	if(mouseover(x-pad, y-pad, w+2*pad, h+2*pad)){
		idcpy(uis.hot, id);
		hot = qtrue;
		if(idcmp(uis.active, "") && uis.keys[K_MOUSE1]){
			idcpy(uis.active, id);
			idcpy(uis.focus, id);
		}
	}

	if(idcmp(uis.focus, id) && !yieldfocus(id))
		updated = updatefield(buf, caret, sz);

	if(idcmp(uis.active, id) || hot)
		fillrect(x-pad, y-pad, w+2*pad, h+2*pad, CActive);
	else
		fillrect(x-pad, y-pad, w+2*pad, h+2*pad, CHot);
	drawstr(x, y+2, buf, UI_LEFT|UI_SMALLFONT|UI_DROPSHADOW, COrange);
	for(i = 0, caretpos = 0; i < *caret && buf[i] != '\0'; i++){
		if(Q_IsColorString(&buf[i]))
			i++;
		else
			caretpos += SMALLCHAR_WIDTH;
	}
	fillrect(x+caretpos, y, 2, h, CRed);
	if(updated)
		trap_S_StartLocalSound(uis.fieldUpdateSound, CHAN_LOCAL_SOUND);
	return updated;
}

static qboolean
spinnerbutton(const char *id, int x, int y, const char *shader)
{
	const float sz = 18;
	qboolean hot;

	hot = qfalse;

	if(mouseover(x, y, sz, sz)){
		idcpy(uis.hot, id);
		hot = qtrue;
		if(idcmp(uis.active, "") && uis.keys[K_MOUSE1])
			idcpy(uis.active, id);
	}

	setcolour(CBlack);
	drawnamedpic(x+2, y+2, sz, sz, shader);
	if(hot)
		setcolour(COrange);
	else
		setcolour(CAmethyst);
	drawnamedpic(x, y, sz, sz, shader);
	setcolour(nil);
	return !uis.keys[K_MOUSE1] && idcmp(uis.hot, id) && idcmp(uis.active, id);
}

qboolean
textspinner(const char *id, int x, int y, int just, char **opts, int *i, int nopts)
{
	const float w = 13*SMALLCHAR_WIDTH, h = 18, bsz = 18;
	qboolean updated;
	char bid[MAXIDLEN];

	updated = qfalse;
	justify(just, &x, w);

	if(nopts > 1){
		Com_sprintf(bid, sizeof bid, "%s.prev", id);
		if(spinnerbutton(bid, x, y, "menu/art/left")){
			*i = (*i <= 0) ? nopts-1 : *i-1;
			updated = qtrue;
		}
		Com_sprintf(bid, sizeof bid, "%s.next", id);
		if(spinnerbutton(bid, x+bsz+w, y, "menu/art/right")){
			*i = (*i + 1) % nopts;
			updated = qtrue;
		}
	}

	if(idcmp(uis.focus, ""))
		idcpy(uis.focus, id);
	if(mouseover(x, y, w, h)){
		idcpy(uis.hot, id);
		if(idcmp(uis.active, "") && uis.keys[K_MOUSE1]){
			idcpy(uis.active, id);
			idcpy(uis.focus, id);
		}
	}

	yieldfocus(id);

	fillrect(x+bsz, y, w, h, CLtBlue);
	drawrect(x+bsz, y, w, h, CBlack);
	drawstr(x+bsz+w/2, y+2, opts[*i], UI_SMALLFONT|UI_CENTER|UI_DROPSHADOW,
	   CRed);
	return updated;
}

/*
This only displays the key; binding the key is the caller's responsibility.
*/
qboolean
keybinder(const char *id, int x, int y, int just, int key)
{
	const int width = 7;	// chars
	const float w = width*SMALLCHAR_WIDTH;
	const float h = 16, pad = 4;
	char buf[32], *p;

	justify(just, &x, w);

	if(idcmp(uis.focus, ""))
		idcpy(uis.focus, id);
	if(mouseover(x-pad, y-pad, w+2*pad, h+2*pad)){
		idcpy(uis.hot, id);
		if(idcmp(uis.active, "") && uis.keys[K_MOUSE1]){
			idcpy(uis.active, id);
			idcpy(uis.focus, id);
		}
	}
	
	yieldfocus(id);

	if(key == -1)
		*buf = '\0';
	else
		trap_Key_KeynumToStringBuf(key, buf, sizeof buf);

	Q_strlwr(buf);

	// strip "arrow" off e.g. "leftarrow"
	p = strstr(buf, "arrow");
	if(p != nil)
		*p = '\0';

	// truncate to field width
	buf[width-1] = '\0';

	fillrect(x-pad, y-pad, w+2*pad, h+2*pad, CActive);
	drawstr(x, y+2, buf, UI_LEFT|UI_SMALLFONT|UI_DROPSHADOW, COrange);

	return !uis.keys[K_MOUSE1] && idcmp(uis.hot, id) && idcmp(uis.active, id);	
}
