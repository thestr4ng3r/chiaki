#include "chiaki/x11.h"


static Window GetCurrWindow(d)
Display *d;
{

	Window foo;
	Window win;
	int bar;

	do
	{
		(void) XQueryPointer(d, DefaultRootWindow(d), &foo, &win, &bar, &bar, &bar, &bar, &bar);
	} while(win <= 0);


#ifdef VROOT
	{
		int n;
		Window *wins;
		XWindowAttributes xwa;

		(void) fputs("=xwa=", stdout);

		XQueryTree(d, win, &foo, &foo, &wins, &n);

		bar=0;
		while(--n >= 0) {
			
			XGetWindowAttributes(d, wins[n], &xwa);
			if( (xwa.width * xwa.height) > bar) {
				win = wins[n];
				bar = xwa.width * xwa.height;
			}
			n--;
		}
		XFree(wins);
	}
#endif
	return(win);
}

Window ChXGetFocusWindow()
{
	
	Display* display = NULL;
	display = XOpenDisplay( NULL );
	if ( !display ) {
		printf("X11: Failed to open default display.\n");
		return;
	}
	
	//Window focused;
	return GetCurrWindow(display);
}


ChXMouse ChXGetMouse()
{
	int b = -1;
	ChXMouse mouseOut;
	
	Display* display = NULL;
	Window screen = 0;
	XWindowAttributes xwAttr;
	XEvent xevent;

	display = XOpenDisplay( NULL );
	if ( !display ) {
		printf("Failed to open default display.\n");
		return;
	}
	
	screen = DefaultRootWindow( display ); // Gives me the actual screen. (not a window)
	if ( 0 > screen ) {
		printf("Failed to obtain the root windows Id of the default screen of given display.\n");
		return;
	}
	
	// This also works with KB modifiers
	XQueryPointer (display, screen, &xevent.xbutton.root,
	&xevent.xbutton.window, &xevent.xbutton.x_root,
	&xevent.xbutton.y_root, &xevent.xbutton.x, &xevent.xbutton.y,
	&xevent.xbutton.state); // last one is button mask!
	//printf("Button state: %d\n", xevent.xbutton.state); // 0=Nobuttons down, 256=LMB down, 1024=RMB down
	mouseOut.x = xevent.xbutton.x_root;
	mouseOut.y = xevent.xbutton.y_root;
	
	if(xevent.xbutton.state == 256)
		mouseOut.lmb=1;
	else if(xevent.xbutton.state == 1032) //sneaky, this is actually Alt+RMB which also scales windows
		mouseOut.lmb=1;
	else if(xevent.xbutton.state == 0)
		mouseOut.lmb=0;
	
	//cleanup
	XCloseDisplay( display );
	return mouseOut;
}

void ChXWindowFocusOff()
{
	Display* display = NULL;
    Window screen = 0;
    XWindowAttributes xwAttr;
    XEvent xevent;

    display = XOpenDisplay( NULL );
    if ( !display ) {
		printf("Failed to open default display.\n");
		return;
    }
    
    screen = DefaultRootWindow( display ); // Gives me the actual screen. (not a window)
    if ( 0 > screen ) {
	    printf("Failed to obtain the root windows Id of the default screen of given display.\n");
		return;
    }
	
	XSetInputFocus(display, screen, RevertToNone, CurrentTime);
	
	
	//cleanup
	XCloseDisplay( display );
}
