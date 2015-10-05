#include "ui_local.h"

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

	if(mouseover(x, y, w, h)){
		Q_strncpyz(uis.hot, id, sizeof uis.hot);
		hot = qtrue;
		if(*uis.active == '\0' && uis.keys[K_MOUSE1]){
			Q_strncpyz(uis.active, id, sizeof uis.active);
			setfocus(id);
		}
	}

	clr = CLtBlue;
	if(hot){
		clr = CWHot;
		if(strcmp(uis.active, id) == 0)
			clr = CWActive;
	}
	drawpropstr(x+w/2, y, label, UI_CENTER|UI_DROPSHADOW, clr);

	if(strcmp(id, uis.focus) == 0)
		drawrect(x-2, y-2, w+4, h+4, CWFocus);
	return !uis.keys[K_MOUSE1] && hot && strcmp(uis.active, id) == 0;
}

qboolean
slider(const char *id, int x, int y, int just, float min, float max, float *val, const char *displayfmt)
{
	float w = 120, pad = 2, h = 12, knobw = 6, knobh = 18;
	float knobx, knoby, ix, iy, iw;
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

	if(mouseover(ix, y + h/2 - knobh/2, iw, knobh)){
		Q_strncpyz(uis.hot, id, sizeof uis.hot);
		hot = qtrue;
		if(*uis.active == '\0' && uis.keys[K_MOUSE1]){
			Q_strncpyz(uis.active, id, sizeof uis.active);
			setfocus(id);
		}
	}
	*val = Com_Scale(*val, min, max, 0, max);
	if(strcmp(uis.active, id) == 0){
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

	fillrect(x, y, w, h, CWBody);
	drawrect(x, y, w, h, CWBorder);

	knobpos = (iw * *val) / max;
	*val = Com_Scale(*val, 0, max, min, max);
	knobx = ix + knobpos - knobw/2;
	knoby = y + h/2 - knobh/2;

	if(hot || strcmp(uis.active, id) == 0)
		clr = CWHot;
	else
		clr = CWText;
	fillrect(knobx+1, knoby+1, knobw, knobh, CWShadow);
	fillrect(knobx, knoby, knobw, knobh, clr);
	drawrect(knobx, knoby, knobw, knobh, CWBorder);
	if(*displayfmt != '\0'){
		Com_sprintf(s, sizeof s, displayfmt, *val);
		drawstr(x+w+6, iy, s, UI_LEFT|UI_SMALLFONT|UI_DROPSHADOW, CWText);
		*val = atof(s);
	}

	if(strcmp(id, uis.focus) == 0)
		drawrect(x-2, y-2, w+4, h+4, CWFocus);
	return updated;
}

qboolean
checkbox(const char *id, int x, int y, int just, qboolean *state)
{
	const float w = 16, h = 16;
	qboolean hot;

	hot = qfalse;
	justify(just, &x, w);

	if(mouseover(x, y, w, h)){
		Q_strncpyz(uis.hot, id, sizeof uis.hot);
		hot = qtrue;
		if(*uis.active == '\0' && uis.keys[K_MOUSE1]){
			*state = !*state;
			Q_strncpyz(uis.active, id, sizeof uis.active);
			setfocus(id);
		}
	}

	fillrect(x, y, w, h, CWBody);
	drawrect(x, y, w, h, CWBorder);
	if(*state){
		setcolour(CWShadow);
		drawnamedpic(x+2, y+2, w, h, "menu/art/tick");
		if(hot)
			setcolour(CWHot);
		else
			setcolour(CWText);
		drawnamedpic(x, y, w, h, "menu/art/tick");
		setcolour(nil);
	}

	if(strcmp(id, uis.focus) == 0)
		drawrect(x-2, y-2, w+4, h+4, CWFocus);
	return !uis.keys[K_MOUSE1] && hot && strcmp(uis.active, id) == 0;
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

	if(mouseover(x-pad, y-pad, w+2*pad, h+2*pad)){
		Q_strncpyz(uis.hot, id, sizeof uis.hot);
		hot = qtrue;
		if(*uis.active == '\0' && uis.keys[K_MOUSE1]){
			Q_strncpyz(uis.active, id, sizeof uis.active);
			setfocus(id);
		}
	}

	if(strcmp(uis.focus, id) == 0)
		updated = updatefield(buf, caret, sz);

	if(strcmp(uis.active, id) == 0 || hot)
		fillrect(x-pad, y-pad, w+2*pad, h+2*pad, CWActive);
	else
		fillrect(x-pad, y-pad, w+2*pad, h+2*pad, CWHot);
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

	if(strcmp(uis.focus, id) == 0)
		drawrect(x-2, y-2, w+4, h+4, CWFocus);
	return updated;
}

static qboolean
spinnerbutton(const char *id, int x, int y, const char *shader)
{
	const float sz = 18;
	qboolean hot;

	hot = qfalse;

	if(mouseover(x, y, sz, sz)){
		Q_strncpyz(uis.hot, id, sizeof uis.hot);
		hot = qtrue;
		if(*uis.active == '\0' && uis.keys[K_MOUSE1])
			Q_strncpyz(uis.active, id, sizeof uis.active);
	}

	setcolour(CWShadow);
	drawnamedpic(x+2, y+2, sz, sz, shader);
	if(hot)
		setcolour(CWHot);
	else
		setcolour(CWText);
	drawnamedpic(x, y, sz, sz, shader);
	setcolour(nil);
	return !uis.keys[K_MOUSE1] && hot && strcmp(uis.active, id) == 0;
}

qboolean
textspinner(const char *id, int x, int y, int just, char **opts, int *i, int nopts)
{
	const float w = 13*SMALLCHAR_WIDTH, h = 18, bsz = 18;
	qboolean updated;
	char bid[IDLEN];

	updated = qfalse;
	justify(just, &x, w);

	if(nopts > 1){
		Com_sprintf(bid, sizeof bid, "%s.prev", id);
		if(spinnerbutton(bid, x, y, "menu/art/left")){
			*i = (*i <= 0)? nopts-1 : *i-1;
			updated = qtrue;
		}
		Com_sprintf(bid, sizeof bid, "%s.next", id);
		if(spinnerbutton(bid, x+bsz+w, y, "menu/art/right")){
			*i = (*i + 1) % nopts;
			updated = qtrue;
		}
	}

	if(mouseover(x, y, w, h)){
		Q_strncpyz(uis.hot, id, sizeof uis.hot);
		if(*uis.active == '\0' && uis.keys[K_MOUSE1]){
			Q_strncpyz(uis.active, id, sizeof uis.active);
			setfocus(id);
		}
	}

	fillrect(x+bsz, y, w, h, CWBody);
	drawrect(x+bsz, y, w, h, CWBorder);
	drawstr(x+bsz+w/2, y+2, opts[*i], UI_SMALLFONT|UI_CENTER|UI_DROPSHADOW,
	   CWText);

	if(strcmp(uis.focus, id) == 0)
		drawrect(x+bsz-2, y-2, w+4, h+4, CWFocus);
	return updated;
}

/*
This only displays the key; binding the key is the caller's responsibility.
*/
qboolean
keybinder(const char *id, int x, int y, int just, int key)
{
	const int width = 7;	// in chars
	const float w = width*SMALLCHAR_WIDTH;
	const float h = 16, pad = 4;
	char buf[32], *p;
	float *clr;

	justify(just, &x, w);

	if(mouseover(x-pad, y-pad, w+2*pad, h+2*pad)){
		Q_strncpyz(uis.hot, id, sizeof uis.hot);
		if(strcmp(uis.active, "") == 0 && uis.keys[K_MOUSE1]){
			Q_strncpyz(uis.active, id, sizeof uis.active);
			setfocus(id);
		}
	}

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

	fillrect(x-pad, y-pad, w+2*pad, h+2*pad, CWBody);
	drawrect(x-pad, y-pad, w+2*pad, h+2*pad, CWBorder);
	clr = CWText;
	if(mouseover(x, y, w, h))
		clr = CWHot;
	drawstr(x, y+2, buf, UI_LEFT|UI_SMALLFONT|UI_DROPSHADOW, clr);
	
	if(strcmp(uis.focus, id) == 0)
		drawrect(x-2, y-2, w+4, h+4, CWFocus);
	return !uis.keys[K_MOUSE1] && strcmp(uis.hot, id) == 0 && strcmp(uis.active, id) == 0;	
}
