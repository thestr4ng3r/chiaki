#ifndef CHIAKI_X11_H
#define CHIAKI_X11_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>

typedef struct chx_rect_t{
	int x;
	int y;
	int width;
	int height;
} ChXRect;

typedef struct chx_mouse_t{
	int x;
	int y;
	int lmb; // left mouse button state
} ChXMouse;


ChXMouse ChXGetMouse();	// screen coordinates + LMB state

Window ChXGetFocusWindow();

void ChXWindowFocusOff();


#ifdef __cplusplus
}
#endif 

#endif // CHIAKI_X11_H
