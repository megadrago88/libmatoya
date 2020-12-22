// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "GL/glcorearb30.h"


// X interface

// Reference: https://code.woboq.org/qt5/include/X11/

#define _Xconst                   const

#define None                      0L
#define Bool                      int
#define True                      1
#define False                     0
#define Status                    int
#define AllocNone                 0

#define RevertToNone              ((int) None)

#define InputOutput               1
#define InputOnly                 2

#define NoEventMask               0L
#define KeyPressMask              (1L<<0)
#define KeyReleaseMask            (1L<<1)
#define ButtonPressMask           (1L<<2)
#define ButtonReleaseMask         (1L<<3)
#define EnterWindowMask           (1L<<4)
#define LeaveWindowMask           (1L<<5)
#define PointerMotionMask         (1L<<6)
#define PointerMotionHintMask     (1L<<7)
#define Button1MotionMask         (1L<<8)
#define Button2MotionMask         (1L<<9)
#define Button3MotionMask         (1L<<10)
#define Button4MotionMask         (1L<<11)
#define Button5MotionMask         (1L<<12)
#define ButtonMotionMask          (1L<<13)
#define KeymapStateMask           (1L<<14)
#define ExposureMask              (1L<<15)
#define VisibilityChangeMask      (1L<<16)
#define StructureNotifyMask       (1L<<17)
#define ResizeRedirectMask        (1L<<18)
#define SubstructureNotifyMask    (1L<<19)
#define SubstructureRedirectMask  (1L<<20)
#define FocusChangeMask           (1L<<21)
#define PropertyChangeMask        (1L<<22)
#define ColormapChangeMask        (1L<<23)
#define OwnerGrabButtonMask       (1L<<24)

#define PropModeReplace           0
#define PropModePrepend           1
#define PropModeAppend            2

#define XK_space                  0x0020
#define XK_apostrophe             0x0027
#define XK_comma                  0x002c
#define XK_minus                  0x002d
#define XK_period                 0x002e
#define XK_slash                  0x002f
#define XK_0                      0x0030
#define XK_1                      0x0031
#define XK_2                      0x0032
#define XK_3                      0x0033
#define XK_4                      0x0034
#define XK_5                      0x0035
#define XK_6                      0x0036
#define XK_7                      0x0037
#define XK_8                      0x0038
#define XK_9                      0x0039
#define XK_semicolon              0x003b
#define XK_equal                  0x003d
#define XK_bracketleft            0x005b
#define XK_backslash              0x005c
#define XK_bracketright           0x005d
#define XK_grave                  0x0060
#define XK_a                      0x0061
#define XK_b                      0x0062
#define XK_c                      0x0063
#define XK_d                      0x0064
#define XK_e                      0x0065
#define XK_f                      0x0066
#define XK_g                      0x0067
#define XK_h                      0x0068
#define XK_i                      0x0069
#define XK_j                      0x006a
#define XK_k                      0x006b
#define XK_l                      0x006c
#define XK_m                      0x006d
#define XK_n                      0x006e
#define XK_o                      0x006f
#define XK_p                      0x0070
#define XK_q                      0x0071
#define XK_r                      0x0072
#define XK_s                      0x0073
#define XK_t                      0x0074
#define XK_u                      0x0075
#define XK_v                      0x0076
#define XK_w                      0x0077
#define XK_x                      0x0078
#define XK_y                      0x0079
#define XK_z                      0x007a
#define XK_yen                    0x00a5

#define XK_BackSpace              0xff08
#define XK_Tab                    0xff09
#define XK_Return                 0xff0d
#define XK_Escape                 0xff1b
#define XK_Home                   0xff50
#define XK_Left                   0xff51
#define XK_Up                     0xff52
#define XK_Right                  0xff53
#define XK_Down                   0xff54
#define XK_Page_Up                0xff55
#define XK_Page_Down              0xff56
#define XK_End                    0xff57
#define XK_Insert                 0xff63
#define XK_Menu                   0xff67
#define XK_Num_Lock               0xff7f
#define XK_KP_Enter               0xff8d
#define XK_KP_Insert              0xff9e // 0
#define XK_KP_End                 0xff9c // 1
#define XK_KP_Down                0xff99 // 2
#define XK_KP_Page_Down           0xff9b // 3
#define XK_KP_Left                0xff96 // 4
#define XK_KP_Begin               0xff9d // 5
#define XK_KP_Right               0xff98 // 6
#define XK_KP_Home                0xff95 // 7
#define XK_KP_Up                  0xff97 // 8
#define XK_KP_Page_Up             0xff9a // 9
#define XK_KP_Delete              0xff9f // Decimal
#define XK_KP_Multiply            0xffaa
#define XK_KP_Add                 0xffab
#define XK_KP_Subtract            0xffad
#define XK_KP_Divide              0xffaf
#define XK_F1                     0xffbe
#define XK_F2                     0xffbf
#define XK_F3                     0xffc0
#define XK_F4                     0xffc1
#define XK_F5                     0xffc2
#define XK_F6                     0xffc3
#define XK_F7                     0xffc4
#define XK_F8                     0xffc5
#define XK_F9                     0xffc6
#define XK_F10                    0xffc7
#define XK_F11                    0xffc8
#define XK_F12                    0xffc9
#define XK_F13                    0xffca
#define XK_F14                    0xffcb
#define XK_F15                    0xffcc
#define XK_F16                    0xffcd
#define XK_F17                    0xffce
#define XK_F18                    0xffcf
#define XK_F19                    0xffd0
#define XK_Shift_L                0xffe1
#define XK_Shift_R                0xffe2
#define XK_Control_L              0xffe3
#define XK_Control_R              0xffe4
#define XK_Caps_Lock              0xffe5
#define XK_Alt_L                  0xffe9
#define XK_Super_L                0xffeb // Left Windows
#define XK_Super_R                0xffec // Right Windows
#define XK_Alt_R                  0xffea
#define XK_Delete                 0xffff

#define XK_Pause                  0xff13
#define XK_Scroll_Lock            0xff14
#define XK_Print                  0xff61

#define XK_Muhenkan               0xff22
#define XK_Henkan                 0xff23
#define XK_Romaji                 0xff24

#define KeyPress                  2
#define KeyRelease                3
#define ButtonPress               4
#define ButtonRelease             5
#define MotionNotify              6
#define EnterNotify               7
#define LeaveNotify               8
#define FocusIn                   9
#define FocusOut                  10
#define KeymapNotify              11
#define Expose                    12
#define GraphicsExpose            13
#define NoExpose                  14
#define VisibilityNotify          15
#define CreateNotify              16
#define DestroyNotify             17
#define UnmapNotify               18
#define MapNotify                 19
#define MapRequest                20
#define ReparentNotify            21
#define ConfigureNotify           22
#define ConfigureRequest          23
#define GravityNotify             24
#define ResizeRequest             25
#define CirculateNotify           26
#define CirculateRequest          27
#define PropertyNotify            28
#define SelectionClear            29
#define SelectionRequest          30
#define SelectionNotify           31
#define ColormapNotify            32
#define ClientMessage             33
#define MappingNotify             34
#define GenericEvent              35
#define LASTEvent                 36

#define CWBackPixmap              (1L<<0)
#define CWBackPixel               (1L<<1)
#define CWBorderPixmap            (1L<<2)
#define CWBorderPixel             (1L<<3)
#define CWBitGravity              (1L<<4)
#define CWWinGravity              (1L<<5)
#define CWBackingStore            (1L<<6)
#define CWBackingPlanes           (1L<<7)
#define CWBackingPixel            (1L<<8)
#define CWOverrideRedirect        (1L<<9)
#define CWSaveUnder               (1L<<10)
#define CWEventMask               (1L<<11)
#define CWDontPropagate           (1L<<12)
#define CWColormap                (1L<<13)
#define CWCursor                  (1L<<14)

#define IsUnmapped                0
#define IsUnviewable              1
#define IsViewable                2

#define QueuedAlready             0
#define QueuedAfterReading        1
#define QueuedAfterFlush          2

#define XNInputStyle              "inputStyle"
#define XNClientWindow            "clientWindow"

#define XIMPreeditNothing         0x0008L
#define XIMStatusNothing          0x0400L

#define CurrentTime               0L

#define GrabModeSync              0
#define GrabModeAsync             1

typedef unsigned long VisualID;
typedef unsigned long XID;
typedef unsigned long Time;
typedef unsigned long Atom;
typedef char *XPointer;
typedef XID Window;
typedef XID Drawable;
typedef XID Colormap;
typedef XID Pixmap;
typedef XID Cursor;
typedef XID KeySym;
typedef struct _XDisplay Display;
typedef struct _XVisual Visual;
typedef struct _XScreen Screen;

typedef struct {
	unsigned long pixel;
	unsigned short red, green, blue;
	char flags;
	char pad;
} XColor;

typedef struct {
	Visual *visual;
	VisualID visualid;
	int screen;
	unsigned int depth;
	int class;
	unsigned long red_mask;
	unsigned long green_mask;
	unsigned long blue_mask;
	int colormap_size;
	int bits_per_rgb;
} XVisualInfo;

typedef struct {
	Pixmap background_pixmap;
	unsigned long background_pixel;
	Pixmap border_pixmap;
	unsigned long border_pixel;
	int bit_gravity;
	int win_gravity;
	int backing_store;
	unsigned long backing_planes;
	unsigned long backing_pixel;
	Bool save_under;
	long event_mask;
	long do_not_propagate_mask;
	Bool override_redirect;
	Colormap colormap;
	Cursor cursor;
} XSetWindowAttributes;

typedef struct {
	int x;
	int y;
	int width;
	int height;
	int border_width;
	int depth;
	Visual *visual;
	Window root;
	int class;
	int bit_gravity;
	int win_gravity;
	int backing_store;
	unsigned long backing_planes;
	unsigned long backing_pixel;
	Bool save_under;
	Colormap colormap;
	Bool map_installed;
	int map_state;
	long all_event_masks;
	long your_event_mask;
	long do_not_propagate_mask;
	Bool override_redirect;
	Screen *screen;
} XWindowAttributes;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	int extension;
	int evtype;
	unsigned int cookie;
	void *data;
} XGenericEventCookie;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	Window window;
	Window root;
	Window subwindow;
	Time time;
	int x;
	int y;
	int x_root;
	int y_root;
	unsigned int state;
	unsigned int keycode;
	Bool same_screen;
} XKeyEvent;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	Window window;
	Atom message_type;
	int format;
	union {
		char b[20];
		short s[10];
		long l[5];
	} data;
} XClientMessageEvent;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	Window window;
	Window root;
	Window subwindow;
	Time time;
	int x;
	int y;
	int x_root;
	int y_root;
	unsigned int state;
	unsigned int button;
	Bool same_screen;
} XButtonEvent;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	Window window;
	Window root;
	Window subwindow;
	Time time;
	int x;
	int y;
	int x_root;
	int y_root;
	unsigned int state;
	char is_hint;
	Bool same_screen;
} XMotionEvent;

typedef union _XEvent {
	int type;
	XKeyEvent xkey;
	XButtonEvent xbutton;
	XMotionEvent xmotion;
	XClientMessageEvent xclient;
	XGenericEventCookie xcookie;
	long pad[24];
} XEvent;

typedef struct Hints {
	unsigned long flags;
	unsigned long functions;
	unsigned long decorations;
	long inputMode;
	unsigned long status;
} Hints;

typedef XKeyEvent XKeyPressedEvent;
typedef XKeyEvent XKeyReleasedEvent;

struct _XrmHashBucketRec;
typedef struct _XIM *XIM;
typedef struct _XIC *XIC;

static Display *(*XOpenDisplay)(const char *display_name);
static int (*XCloseDisplay)(Display *display);
static Window (*XDefaultRootWindow)(Display *display);
static Colormap (*XCreateColormap)(Display *display, Window w, Visual *visual, int alloc);
static Window (*XCreateWindow)(Display *display, Window parent, int x, int y, unsigned int width,
	unsigned int height, unsigned int border_width, int depth, unsigned int class, Visual *visual,
	unsigned long valuemask, XSetWindowAttributes *attributes);
static int (*XMapWindow)(Display *display, Window w);
static int (*XWithdrawWindow)(Display *display, Window w);
static int (*XMapRaised)(Display *display, Window w);
static int (*XSetInputFocus)(Display *display, Window focus, int revert_to, Time time);
static int (*XStoreName)(Display *display, Window w, const char *window_name);
static Status (*XGetWindowAttributes)(Display *display, Window w, XWindowAttributes *window_attributes_return);
static KeySym (*XLookupKeysym)(XKeyEvent *key_event, int index);
static Status (*XSetWMProtocols)(Display *display, Window w, Atom *protocols, int count);
static Atom (*XInternAtom)(Display *display, const char *atom_name, Bool only_if_exists);
static int (*XNextEvent)(Display *display, XEvent *event_return);
static int (*XEventsQueued)(Display *display, int mode);
static int (*XMoveWindow)(Display *display, Window w, int x, int y);
static int (*XChangeProperty)(Display *display, Window w, Atom property, Atom type, int format, int mode, const unsigned char *data, int nelements);
static int (*XGetInputFocus)(Display *display, Window *focus_return, int *revert_to_return);
static char *(*XGetDefault)(Display *display, const char *program, const char *option);
static int (*XWidthOfScreen)(Screen *screen);
static int (*XHeightOfScreen)(Screen *screen);
static int (*XDestroyWindow)(Display *display, Window w);
static int (*XFree)(void *data);
static Status (*XInitThreads)(void);
static int (*Xutf8LookupString)(XIC ic, XKeyPressedEvent *event, char *buffer_return, int bytes_buffer,
	KeySym *keysym_return, Status *status_return);
static XIM (*XOpenIM)(Display *dpy, struct _XrmHashBucketRec *rdb, char *res_name, char *res_class);
static Status (*XCloseIM)(XIM im);
static XIC (*XCreateIC)(XIM im, ...);
static void (*XDestroyIC)(XIC ic);
static Bool (*XGetEventData)(Display *dpy, XGenericEventCookie *cookie);
static int (*XGrabPointer)(Display *display, Window grab_window, Bool owner_events, unsigned int event_mask, int pointer_mode,
	int keyboard_mode, Window confine_to, Cursor cursor, Time time);
static int (*XUngrabPointer)(Display *display, Time time);
static int (*XWarpPointer)(Display *display, Window src_w, Window dest_w, int src_x, int src_y, unsigned int src_width,
	unsigned int src_height, int dest_x, int dest_y);
static int (*XSync)(Display *display, Bool discard);
static Pixmap (*XCreateBitmapFromData)(Display *display, Drawable d, _Xconst char *data, unsigned int width, unsigned int height);
static Cursor (*XCreatePixmapCursor)(Display *display, Pixmap source, Pixmap mask, XColor *foreground_color,
	XColor *background_color, unsigned int x, unsigned int y);
static int (*XFreePixmap)(Display *display, Pixmap pixmap);
static int (*XDefineCursor)(Display *display, Window w, Cursor cursor);
static int (*XFreeCursor)(Display *display, Cursor cursor);
static int (*XResizeWindow)(Display *display, Window w, unsigned int width, unsigned int height);


// XI2 interface

// Reference: https://code.woboq.org/kde/include/X11/extensions/

#define XIAllMasterDevices 1

#define XI_RawMotion       17

#define XISetMask(ptr, event) \
	(((unsigned char *) (ptr))[(event) >> 3] |= (1 << ((event) & 7)))

typedef struct {
	int deviceid;
	int mask_len;
	unsigned char *mask;
} XIEventMask;

typedef struct {
	int mask_len;
	unsigned char *mask;
	double *values;
} XIValuatorState;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	int extension;
	int evtype;
	Time time;
	int deviceid;
	int sourceid;
	int detail;
	int flags;
	XIValuatorState valuators;
	double *raw_values;
} XIRawEvent;

static int (*XISelectEvents)(Display *dpy, Window win, XIEventMask *masks, int num_masks);


// Xcursor interface

typedef int XcursorBool;
typedef unsigned int XcursorUInt;
typedef XcursorUInt XcursorDim;
typedef XcursorUInt XcursorPixel;

typedef struct _XcursorImage {
	XcursorUInt version;
	XcursorDim size;
	XcursorDim width;
	XcursorDim height;
	XcursorDim xhot;
	XcursorDim yhot;
	XcursorUInt delay;
	XcursorPixel *pixels;
} XcursorImage;

static XcursorImage *(*XcursorImageCreate)(int width, int height);
static Cursor (*XcursorImageLoadCursor)(Display *dpy, const XcursorImage *image);
static void (*XcursorImageDestroy)(XcursorImage *image);


// GLX interface

// Reference: https://code.woboq.org/qt5/include/GL/

enum _GLX_CONFIGS {
	GLX_USE_GL           = 1,
	GLX_BUFFER_SIZE      = 2,
	GLX_LEVEL            = 3,
	GLX_RGBA             = 4,
	GLX_DOUBLEBUFFER     = 5,
	GLX_STEREO           = 6,
	GLX_AUX_BUFFERS      = 7,
	GLX_RED_SIZE         = 8,
	GLX_GREEN_SIZE       = 9,
	GLX_BLUE_SIZE        = 10,
	GLX_ALPHA_SIZE       = 11,
	GLX_DEPTH_SIZE       = 12,
	GLX_STENCIL_SIZE     = 13,
	GLX_ACCUM_RED_SIZE   = 14,
	GLX_ACCUM_GREEN_SIZE = 15,
	GLX_ACCUM_BLUE_SIZE  = 16,
	GLX_ACCUM_ALPHA_SIZE = 17,
};

typedef XID GLXDrawable;
typedef struct GLXContext * GLXContext;

static void *(*glXGetProcAddress)(const GLubyte *procName);
static void (*glXSwapBuffers)(Display *dpy, GLXDrawable drawable);
static XVisualInfo *(*glXChooseVisual)(Display *dpy, int screen, int *attribList);
static GLXContext (*glXCreateContext)(Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct);
static Bool (*glXMakeCurrent)(Display *dpy, GLXDrawable drawable, GLXContext ctx);
static void (*glXSwapIntervalEXT)(Display *dpy, GLXDrawable drawable, int interval);
static void (*glXDestroyContext)(Display *dpy, GLXContext ctx);
static GLXContext (*glXGetCurrentContext)(void);


// Helper window struct

struct xinfo {
	Display *display;
	XVisualInfo *vis;
	Window window;
};


// Runtime open

static MTY_Atomic32 X_DL_LOCK;
static MTY_SO *X_DL_GLX_SO;
static MTY_SO *X_DL_XI2_SO;
static MTY_SO *X_DL_XCURSOR_SO;
static MTY_SO *X_DL_SO;
static bool X_DL_INIT;

static void x_dl_global_destroy(void)
{
	MTY_SOUnload(&X_DL_GLX_SO);
	MTY_SOUnload(&X_DL_XI2_SO);
	MTY_SOUnload(&X_DL_XCURSOR_SO);
	MTY_SOUnload(&X_DL_SO);
	X_DL_INIT = false;
}

static bool x_dl_global_init(void)
{
	MTY_GlobalLock(&X_DL_LOCK);

	if (!X_DL_INIT) {
		bool r = true;

		X_DL_SO = MTY_SOLoad("libX11.so.6");
		if (!X_DL_SO)
			X_DL_SO = MTY_SOLoad("libX11.so");

		X_DL_XI2_SO = MTY_SOLoad("libXi.so.6");
		X_DL_XCURSOR_SO = MTY_SOLoad("libXcursor.so.1");
		X_DL_GLX_SO = MTY_SOLoad("libGL.so.1");

		if (!X_DL_SO || !X_DL_GLX_SO || !X_DL_XI2_SO || !X_DL_XCURSOR_SO) {
			r = false;
			goto except;
		}

		#define X_DL_LOAD_SYM(so, name) \
			name = MTY_SOGetSymbol(so, #name); \
			if (!name) {r = false; goto except;}

		X_DL_LOAD_SYM(X_DL_SO, XOpenDisplay);
		X_DL_LOAD_SYM(X_DL_SO, XCloseDisplay);
		X_DL_LOAD_SYM(X_DL_SO, XDefaultRootWindow);
		X_DL_LOAD_SYM(X_DL_SO, XCreateColormap);
		X_DL_LOAD_SYM(X_DL_SO, XCreateWindow);
		X_DL_LOAD_SYM(X_DL_SO, XMapWindow);
		X_DL_LOAD_SYM(X_DL_SO, XWithdrawWindow);
		X_DL_LOAD_SYM(X_DL_SO, XMapRaised);
		X_DL_LOAD_SYM(X_DL_SO, XSetInputFocus);
		X_DL_LOAD_SYM(X_DL_SO, XStoreName);
		X_DL_LOAD_SYM(X_DL_SO, XGetWindowAttributes);
		X_DL_LOAD_SYM(X_DL_SO, XLookupKeysym);
		X_DL_LOAD_SYM(X_DL_SO, XSetWMProtocols);
		X_DL_LOAD_SYM(X_DL_SO, XInternAtom);
		X_DL_LOAD_SYM(X_DL_SO, XNextEvent);
		X_DL_LOAD_SYM(X_DL_SO, XEventsQueued);
		X_DL_LOAD_SYM(X_DL_SO, XMoveWindow);
		X_DL_LOAD_SYM(X_DL_SO, XChangeProperty);
		X_DL_LOAD_SYM(X_DL_SO, XGetInputFocus);
		X_DL_LOAD_SYM(X_DL_SO, XGetDefault);
		X_DL_LOAD_SYM(X_DL_SO, XWidthOfScreen);
		X_DL_LOAD_SYM(X_DL_SO, XHeightOfScreen);
		X_DL_LOAD_SYM(X_DL_SO, XDestroyWindow);
		X_DL_LOAD_SYM(X_DL_SO, XFree);
		X_DL_LOAD_SYM(X_DL_SO, XInitThreads);
		X_DL_LOAD_SYM(X_DL_SO, Xutf8LookupString);
		X_DL_LOAD_SYM(X_DL_SO, XOpenIM);
		X_DL_LOAD_SYM(X_DL_SO, XCloseIM);
		X_DL_LOAD_SYM(X_DL_SO, XCreateIC);
		X_DL_LOAD_SYM(X_DL_SO, XDestroyIC);
		X_DL_LOAD_SYM(X_DL_SO, XGetEventData);
		X_DL_LOAD_SYM(X_DL_SO, XGrabPointer);
		X_DL_LOAD_SYM(X_DL_SO, XUngrabPointer);
		X_DL_LOAD_SYM(X_DL_SO, XWarpPointer);
		X_DL_LOAD_SYM(X_DL_SO, XSync);
		X_DL_LOAD_SYM(X_DL_SO, XCreateBitmapFromData);
		X_DL_LOAD_SYM(X_DL_SO, XCreatePixmapCursor);
		X_DL_LOAD_SYM(X_DL_SO, XFreePixmap);
		X_DL_LOAD_SYM(X_DL_SO, XDefineCursor);
		X_DL_LOAD_SYM(X_DL_SO, XFreeCursor);
		X_DL_LOAD_SYM(X_DL_SO, XResizeWindow);

		X_DL_LOAD_SYM(X_DL_XI2_SO, XISelectEvents);

		X_DL_LOAD_SYM(X_DL_XCURSOR_SO, XcursorImageCreate);
		X_DL_LOAD_SYM(X_DL_XCURSOR_SO, XcursorImageLoadCursor);
		X_DL_LOAD_SYM(X_DL_XCURSOR_SO, XcursorImageDestroy);

		X_DL_LOAD_SYM(X_DL_GLX_SO, glXGetProcAddress);
		X_DL_LOAD_SYM(X_DL_GLX_SO, glXSwapBuffers);
		X_DL_LOAD_SYM(X_DL_GLX_SO, glXChooseVisual);
		X_DL_LOAD_SYM(X_DL_GLX_SO, glXCreateContext);
		X_DL_LOAD_SYM(X_DL_GLX_SO, glXMakeCurrent);
		X_DL_LOAD_SYM(X_DL_GLX_SO, glXSwapIntervalEXT);
		X_DL_LOAD_SYM(X_DL_GLX_SO, glXDestroyContext);
		X_DL_LOAD_SYM(X_DL_GLX_SO, glXGetCurrentContext);

		except:

		if (!r)
			x_dl_global_destroy();

		X_DL_INIT = r;
	}

	MTY_GlobalUnlock(&X_DL_LOCK);

	return X_DL_INIT;
}
