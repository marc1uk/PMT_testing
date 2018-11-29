/* vim:set noexpandtab tabstop=2 wrap */
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h> // /usr/include/X11/extensions/XTest.h
#include <iostream>

// convenience struct for passing related parameters
struct KeyPressVars{
	Display* display=nullptr;
	Window winRoot;
	Window winFocus;
	int    revert;
};

XKeyEvent createKeyEvent(Display *display, Window &win, Window &winRoot, bool press, int keycode, int modifiers);
void KeyPressInitialise(KeyPressVars &thevars);
void DoKeyPress(int KEYCODE, KeyPressVars thevars, bool caps=false, int state=0);
void KeyPressFinalise(KeyPressVars thevars);

int main(int argc, char* argv[]){
	
	// Get the X11 display for sending keypresses to other programs
	// ============================================================
	//std::cout<<"Initialising X11 for sending keystrokes"<<std::endl;
	KeyPressVars thekeypressvars;
	KeyPressInitialise(thekeypressvars);
	if(thekeypressvars.display==nullptr){ std::cerr<<"Keypress Initialization Failed!!"<<std::endl; return 0; }
	
	// Cleaup X11 stuff for sending keypresses
	// =======================================
	//std::cout<<"Cleaning up X11"<<std::endl;
	KeyPressFinalise(thekeypressvars);
	
	std::cout<<"Complete. Goodbye."<<std::endl;
}

// ***************************************************************************
// CREATE X KEYPRESS EVENTS
// ***************************************************************************
XKeyEvent createKeyEvent(Display *display, Window &win,
                           Window &winRoot, bool press,
                           int keycode, int modifiers)
{
   XKeyEvent event;
   
   event.display     = display;
   event.window      = win;
   event.root        = winRoot;
   event.subwindow   = None;
   event.time        = CurrentTime;
   event.x           = 1;
   event.y           = 1;
   event.x_root      = 1;
   event.y_root      = 1;
   event.same_screen = True;
   event.keycode     = XKeysymToKeycode(display, keycode);
   event.state       = modifiers;
   
   if(press)
      event.type = KeyPress;
   else
      event.type = KeyRelease;
   
   return event;
}

// ***************************************************************************
// SETUP SENDING KEYS TO OTHER WINDOWS WITH X
// ***************************************************************************
void KeyPressInitialise(KeyPressVars &thevars){
		// Obtain the X11 display.
	thevars.display = XOpenDisplay(0);
	if(thevars.display == nullptr){
		std::cerr<<"Could not open X Display!"<<std::endl;
		return;
	}
	// Note multiple calls to XOpenDisplay() with the same parameter return different handles;
	// sending the press event with one handle and the release event with another will not work.
	
	// Get the root window for the current display.
	thevars.winRoot = XDefaultRootWindow(thevars.display);
	
	// Prevent keydown-keyup generating multiple keypresses due to autorepeat
	XAutoRepeatOff(thevars.display);
	// XXX THIS AFFECTS THE GLOBAL X SETTINGS AFTER THE PROGRAM EXITS!!! XXX
	// RE-ENABLE BEFORE CLOSING THE PROGRAM TO REVERT TO NORMAL BEHAVIOUR! XXX
	
	// Find the window which has the current keyboard focus.
	XGetInputFocus(thevars.display, &thevars.winFocus, &thevars.revert);
	
	//tell X11 this client wants to receive key-press and key-release events associated with winFocus.
	XSelectInput(thevars.display, thevars.winFocus, KeyPressMask|KeyReleaseMask); 
}

// ***************************************************************************
// SEND A KEYPRESS EVENT
// ***************************************************************************
void DoKeyPress(int KEYCODE, KeyPressVars thevars, bool caps, int state){  // state=0: press+release, 1=press, 2=release
	
	if(caps){
	// KEYCODE for w and W are the same!!
	// we need to manually send the modifier keys!
		DoKeyPress(XK_Shift_L, thevars, false, 1);
	}
	
	int modifiers = 0; //(caps) ? ShiftMask : 0;  // did not seem to work for sending caps
	
	if(state!=2){
		// generate the key-down event
		XGetInputFocus(thevars.display, &thevars.winFocus, &thevars.revert);
		XKeyEvent event = createKeyEvent(thevars.display, thevars.winFocus, thevars.winRoot, true, KEYCODE, modifiers);
		int ret = XTestFakeKeyEvent(event.display, event.keycode, True, CurrentTime);
		if(ret==0) std::cerr<<"XTestFakeKeyEvent returned 0! Could not send keypress?!"<<std::endl;
	}
	
	// ensure the thing has gone through? Did not help with some keys not getting seen?
	//XSync(thevars.display,False);
	//usleep(100);
	
	if(state!=1){
		// repeat with a key-release event
		XGetInputFocus(thevars.display, &thevars.winFocus, &thevars.revert);
		XKeyEvent event = createKeyEvent(thevars.display, thevars.winFocus, thevars.winRoot, false, KEYCODE, modifiers);
		int ret = XTestFakeKeyEvent(event.display, event.keycode, False, CurrentTime);
		if(ret==0) std::cerr<<"XTestFakeKeyEvent returned 0! Could not send keypress?!"<<std::endl;
	}
	
	if(caps){
		DoKeyPress(XK_Shift_L, thevars, false, 2);
	}
}

// ***************************************************************************
// CLEANUP SENDING KEYS TO OTHER WINDOWS
// ***************************************************************************
void KeyPressFinalise(KeyPressVars thevars){
	// Re-enable auto-repeat
	XAutoRepeatOn(thevars.display);
	
	// Cleanup X11 display
	XCloseDisplay(thevars.display);
}
