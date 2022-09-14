/* See LICENSE file for copyright and license details.
 *
 * dynamic window manager is designed like any other X client as well. It is
 * driven through handling X events. In contrast to other X clients, a window
 * manager selects for SubstructureRedirectMask on the root window, to receive
 * events about window (dis-)appearance. Only one X connection at a time is
 * allowed to select for this event mask.
 *
 * The event handlers of dwm are organized in an array which is accessed
 * whenever a new event has been fetched. This allows event dispatching
 * in O(1) time.
 *
 * Each child of the root window is called a client, except windows which have
 * set the override_redirect flag. Clients are organized in a linked client
 * list on each monitor, the focus history is remembered through a stack list
 * on each monitor. Each client contains a bit array to indicate the tags of a
 * client.
 *
 * Keys and tagging rules are organized as arrays and defined in config.h.
 *
 * To understand everything else, start reading main().
 */
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* XINERAMA */
#include <X11/Xft/Xft.h>
#include <math.h>

#include "drw.h"
#include "util.h"

/* macros */
#define BUTTONMASK              (ButtonPressMask|ButtonReleaseMask)
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define INTERSECT(x,y,w,h,m)    (MAX(0, MIN((x)+(w),(m)->wx+(m)->ww) - MAX((x),(m)->wx)) \
                               * MAX(0, MIN((y)+(h),(m)->wy+(m)->wh) - MAX((y),(m)->wy)))
#define ISVISIBLE(C)            ((C->tags & C->mon->tagset[C->mon->seltags]))
#define LENGTH(X)               (sizeof X / sizeof X[0])
#define MOUSEMASK               (BUTTONMASK|PointerMotionMask)
#define WIDTH(X)                ((X)->w + 2 * (X)->bw)
#define HEIGHT(X)               ((X)->h + 2 * (X)->bw)
#define TAGMASK                 ((1 << LENGTH(tags)) - 1)
#define TAGSLENGTH              (LENGTH(tags))
#define TEXTW(X)                (drw_fontset_getwidth(drw, (X)) + lrpad)

#define GAP_TOGGLE 100
#define GAP_RESET  0

#define SYSTEM_TRAY_REQUEST_DOCK    0
/* XEMBED messages */
#define XEMBED_EMBEDDED_NOTIFY      0
#define XEMBED_WINDOW_ACTIVATE      1
#define XEMBED_FOCUS_IN             4
#define XEMBED_MODALITY_ON         10
#define XEMBED_MAPPED              (1 << 0)
#define XEMBED_WINDOW_ACTIVATE      1
#define XEMBED_WINDOW_DEACTIVATE    2
#define VERSION_MAJOR               0
#define VERSION_MINOR               0
#define XEMBED_EMBEDDED_VERSION (VERSION_MAJOR << 16) | VERSION_MINOR

#define FOCUS_LEFT -1
#define FOCUS_RIGHT 1
#define FOCUS_UP 2
#define FOCUS_DOWN -2


static FILE *logfile;

/* enums */
enum { CurNormal, CurResize, CurMove, CurLast }; /* cursor */
enum { SchemeNorm, SchemeSel , SchemeScr}; /* color schemes */
enum { NetSupported, NetWMName, NetWMState, NetWMCheck,
       NetSystemTray, NetSystemTrayOP, NetSystemTrayOrientation, NetSystemTrayOrientationHorz,
       NetWMFullscreen, NetActiveWindow, NetWMWindowType,
	   NetWmStateSkipTaskbar,
	   NetWmPid,
       NetWMWindowTypeDialog, NetClientList, NetDesktopNames, NetDesktopViewport, NetNumberOfDesktops, NetCurrentDesktop, NetLast }; /* EWMH atoms */
enum { Manager, Xembed, XembedInfo, XLast }; /* Xembed atoms */
enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast }; /* default atoms */
enum { ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle,
       ClkClientWin, ClkRootWin, ClkLast }; /* clicks */

typedef union {
	int i;
	unsigned int ui;
	float f;
	const void *v;
} Arg;

typedef struct {
	unsigned int click;
	unsigned int mask;
	unsigned int button;
	void (*func)(const Arg *arg);
	const Arg arg;
} Button;

typedef struct Monitor Monitor;
typedef struct Client Client;
struct Client {
	char name[256];
	float mina, maxa;
	int x, y, w, h;
	int oldx, oldy, oldw, oldh;
	int basew, baseh, incw, inch, maxw, maxh, minw, minh;
	int bw, oldbw;
	unsigned int tags;
	int isfixed, isfloating, isurgent, neverfocus, oldstate, isfullscreen,isfocused;
	Client *next;
	Client *snext;
	Client *lastfocus;
	int focusfreq;
	int fullscreenfreq;
	int focusindex;
	Monitor *mon;
	Window win;
	int titlex, titlew;
	int priority;
	int nstub;
	unsigned long pid;
	int isscratched;
};

typedef struct {
	unsigned int mod;
	KeySym keysym;
	void (*func)(const Arg *);
	const Arg arg;
} Key;

typedef struct {
	const char *symbol;
	void (*arrange)(Monitor *);
} Layout;

typedef struct {
	int isgap;
	int realgap;
	int gappx;
} Gap;

typedef struct Pertag Pertag;
struct Monitor {
	char ltsymbol[16];
	float mfact;
	int nmaster;
	int num;
	int by;               /* bar geometry */
	int mx, my, mw, mh;   /* screen size */
	int wx, wy, ww, wh;   /* window area  */
	Gap *gap;
	unsigned int seltags;
	unsigned int sellt;
	unsigned int tagset[2];
	int showbar;
	int topbar;
	Client *clients;
	Client *sel;
	Client *stack;
	Monitor *next;
	Window barwin;
	const Layout *lt[2];
	Pertag *pertag;
};

typedef struct {
	const char *class;
	const char *instance;
	const char *title;
	unsigned int tags;
	int isfloating;
	int monitor;
	int priority;
	int nstub;
} Rule;

typedef struct {
	const char** command;
	const char* name;
} Launcher;

typedef struct Systray   Systray;
struct Systray {
	Window win;
	Client *icons;
};

typedef struct 
{
	Client * c;
	int x;
	int y;
	int w;
	int h;
	int interact;
} geo_t;

typedef struct 
{
	int x;
	int y;
	int w;
	int h;
} rect_t;


typedef struct ScratchItem ScratchItem;
struct ScratchItem
{
	Client *c;
	int tags;
	int pretags;
	ScratchItem *next;
	ScratchItem *prev;
	int placed;
	int x,y,w,h;
	const char **cmd;
};


typedef struct ScratchGroup ScratchGroup;
struct ScratchGroup
{
	ScratchItem *head;
	ScratchItem *tail;
	int tags;
	int pretags;
	int isfloating;
};

typedef struct Tag Tag;
struct Tag
{
	int tags;
	int tagindex;
	Tag *prev;
	Tag *next;
	time_t lastviewtime;
	long lastviewtimeusec;
	int skip;
};

typedef struct TagStat TagStat;
struct TagStat
{
	time_t laststaytime;
	time_t totalstaytime;
	time_t lastviewtime;
	int curviewdist; // distance in linked list 
	int curvalidviewdist; // skip distance in linked list 
	int jumpcnt;
	int validjumpcnt;
};


/* function declarations */
static void applyrules(Client *c);
static int applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact);
static void arrange(Monitor *m);
static void arrangemon(Monitor *m);
static void attach(Client *c);
static void attachstack(Client *c);
static void addtoscratchgroup(const Arg *arg);
static ScratchItem* addtoscratchgroupc(Client *c);
static ScratchItem * alloc_si(void);
static void buttonpress(XEvent *e);
static void checkotherwm(void);
static void cleanup(void);
static void cleanupmon(Monitor *mon);
static void clientmessage(XEvent *e);
static void configure(Client *c);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static Monitor *createmon(void);
static void destroynotify(XEvent *e);
static void detach(Client *c);
static void detachstack(Client *c);
static Monitor *dirtomon(int dir);
static void drawbar(Monitor *m);
static void drawbars(void);
static void enqueue(Client *c);
static void enqueuestack(Client *c);
static void enternotify(XEvent *e);
static void expose(XEvent *e);
static void focus(Client *c);
static void focusin(XEvent *e);
static void focusmon(const Arg *arg);
static void focusstack(const Arg *arg);
static void focusgrid(const Arg *arg);
static void free_si(ScratchItem *si);
static void gap_copy(Gap *to, const Gap *from);
static Atom getatomprop(Client *c, Atom prop);
static unsigned int getmaxtags();
static int getrootptr(int *x, int *y);
static long getstate(Window w);
static unsigned int getsystraywidth();
static pid_t getstatusbarpid();
static int gettextprop(Window w, Atom atom, char *text, unsigned int size);
static void grabbuttons(Client *c, int focused);
static void grabkeys(void);
static void hidescratchgroup(ScratchGroup *sg);
static void hidescratchgroupv(ScratchGroup *sg, int isarrange);
static void incnmaster(const Arg *arg);
static void keypress(XEvent *e);
static void killclient(const Arg *arg);
static void manage(Window w, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static void monocle(Monitor *m);
static void motionnotify(XEvent *e);
static void movemouse(const Arg *arg);
static Client *nexttiled(Client *c);
static void pop(Client *);
static void propertynotify(XEvent *e);
static void quit(const Arg *arg);
static Monitor *recttomon(int x, int y, int w, int h);
static void removesystrayicon(Client *i);
static void resize(Client *c, int x, int y, int w, int h, int interact);
static void resizebarwin(Monitor *m);
static void resizeclient(Client *c, int x, int y, int w, int h);
static void resizemouse(const Arg *arg);
static void resizerequest(XEvent *e);
static void restack(Monitor *m);
static void rerule(const Arg *arg);
static void rotatestack(const Arg *arg);
static void run(void);
static void runAutostart(void);
static void scan(void);
static int sendevent(Window w, Atom proto, int m, long d0, long d1, long d2, long d3, long d4);
static void sendmon(Client *c, Monitor *m);
static void setclientstate(Client *c, long state);
static void setcurrentdesktop(void);
static void setdesktopnames(void);
static void setfocus(Client *c);
static void setfullscreen(Client *c, int fullscreen);
static void setgaps(const Arg *arg);
static void setlayoutv(const Arg *arg, int isarrange);
static void setlayout(const Arg *arg);
static void setmfact(const Arg *arg);
static void setnumdesktops(void);
static void setup(void);
static void setviewport(void);
static void seturgent(Client *c, int urg);
static void showhide(Client *c);
static void sigchld(int unused);
static void sigstatusbar(const Arg *arg);
static void sighup(int unused);
static void sigterm(int unused);
static void spawn(const Arg *arg);
static void sspawn(const Arg *arg);
static Monitor *systraytomon(Monitor *m);
static void smartview(const Arg *arg);
static void showscratchgroup(ScratchGroup *sg);
static void tag(const Arg *arg);
static void tagmon(const Arg *arg);
static void tile(Monitor *);
static void tile2(Monitor *);
static void togglebar(const Arg *arg);
static void togglefloating(const Arg *arg);
static void togglescratch(const Arg *arg);
static void togglescratchgroup(const Arg *arg);
static void toggletag(const Arg *arg);
static void toggleview(const Arg *arg);
static void unfocus(Client *c, int setfocus);
static void unmanage(Client *c, int destroyed);
static void unmapnotify(XEvent *e);
static void updatecurrentdesktop(void);
static void updatebarpos(Monitor *m);
static void updatebars(void);
static void updateclientlist(void);
static int updategeom(void);
static void updatenumlockmask(void);
static void updatesizehints(Client *c);
static void updatestatus(void);
static void updatesystray(void);
static void updatesystrayicongeom(Client *i, int w, int h);
static void updatesystrayiconstate(Client *i, XPropertyEvent *ev);
static void updatetitle(Client *c);
static void updatewindowtype(Client *c);
static void updatewmhints(Client *c);
static void view(const Arg *arg);
static void relview(const Arg *arg);
static void removefromscratchgroup(const Arg *arg);
static Client *wintoclient(Window w);
static Monitor *wintomon(Window w);
static Client *wintosystrayicon(Window w);
static int xerror(Display *dpy, XErrorEvent *ee);
static int xerrordummy(Display *dpy, XErrorEvent *ee);
static int xerrorstart(Display *dpy, XErrorEvent *ee);
static void zoom(const Arg *arg);
static void zoomi(const Arg *arg);
static void smartzoom(const Arg *arg);
static void LOG(char *content,char *content2);

/* variables */
static Systray *systray = NULL;
static const char broken[] = "broken";
static char stext[256];
static int statusw;
static int statussig;
static pid_t statuspid = -1;
static int screen;
static int sw, sh;           /* X display screen geometry width, height */
static int bh, blw = 0;      /* bar geometry */
static int lrpad;            /* sum of left and right padding for text */
static int (*xerrorxlib)(Display *, XErrorEvent *);
static unsigned int numlockmask = 0;
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress] = buttonpress,
	[ClientMessage] = clientmessage,
	[ConfigureRequest] = configurerequest,
	[ConfigureNotify] = configurenotify,
	[DestroyNotify] = destroynotify,
	[EnterNotify] = enternotify,
	[Expose] = expose,
	[FocusIn] = focusin,
	[KeyPress] = keypress,
	[MappingNotify] = mappingnotify,
	[MapRequest] = maprequest,
	[MotionNotify] = motionnotify,
	[PropertyNotify] = propertynotify,
	[ResizeRequest] = resizerequest,
	[UnmapNotify] = unmapnotify
};
static Atom wmatom[WMLast], netatom[NetLast], xatom[XLast];
static int restart = 0;
static int running = 1;
static Cur *cursor[CurLast];
static Clr **scheme;
static Display *dpy;
static Drw *drw;
static Monitor *mons, *selmon;
static Window root, wmcheckwin;
static Client *focuschain, *FC_HEAD;
static ScratchItem *scratchitemptr;
static ScratchGroup *scratchgroupptr;
static Tag *HEADTAG, *TAILTAG;
static int isnextscratch = 0;
static const char **nextscratchcmd;

/* configuration, allows nested code to access above variables */
#include "config.h"

	static Tag *tagarray[LENGTH(tags) + 1];
static TagStat *taggraph[LENGTH(tags)+1][LENGTH(tags)+1];

struct Pertag {
	unsigned int curtag, prevtag; /* current and previous tag */
	int nmasters[LENGTH(tags) + 1]; /* number of windows in master area */
	float mfacts[LENGTH(tags) + 1]; /* mfacts per tag */
	unsigned int sellts[LENGTH(tags) + 1]; /* selected layouts */
	const Layout *ltidxs[LENGTH(tags) + 1][2]; /* matrix of tags and layouts indexes  */
	int showbars[LENGTH(tags) + 1]; /* display bar for the current tag */
};

static unsigned int scratchtag = 1 << LENGTH(tags);

/* compile-time check if all tags fit into an unsigned int bit array. */
struct NumTags { char limitexceeded[LENGTH(tags) > 31 ? -1 : 1]; };

void
LOG(char *content, char * content2){
	fprintf(logfile,"%s%s\n", content, content2);
}

void
LOG_FORMAT(char *format, ...){
	va_list ap;
	va_start(ap,format);
	vfprintf(logfile,format, ap);
	va_end(ap);
	fflush(logfile);
}

/* function implementations */
void
applyrules(Client *c)
{
	const char *class, *instance;
	unsigned int i;
	const Rule *r;
	Monitor *m;
	XClassHint ch = { NULL, NULL };

	/* rule matching */
	c->isfloating = 0;
	c->tags = 0;
	c->nstub = 0;
	XGetClassHint(dpy, c->win, &ch);
	class    = ch.res_class ? ch.res_class : broken;
	instance = ch.res_name  ? ch.res_name  : broken;

	for (i = 0; i < LENGTH(rules); i++) {
		r = &rules[i];
		if ((!r->title || strstr(c->name, r->title))
		&& (!r->class || strstr(class, r->class))
		&& (!r->instance || strstr(instance, r->instance)))
		{
			c->isfloating = r->isfloating;
			c->tags |= r->tags;
			c->priority = r->priority;
			c->nstub = r->nstub;
			for (m = mons; m && m->num != r->monitor; m = m->next);
			if (m)
				c->mon = m;
		}
	}
	if (ch.res_class)
		XFree(ch.res_class);
	if (ch.res_name)
		XFree(ch.res_name);
	c->tags = c->tags & TAGMASK ? c->tags & TAGMASK : c->mon->tagset[c->mon->seltags];
}


unsigned int
getmaxtags(){
	// 将没有rule的client放到最后一个空闲tag
	Client *c;
	unsigned int maxtags = 0;
	for(c = selmon->clients; c; c = c->next)
		if (c->tags > maxtags)
			maxtags = c->tags;
	return maxtags;
}

void
rerule(const Arg *arg)
{
	Client *c;
	for(c = selmon->clients; c; c = c->next){
			const char *class, *instance;
		unsigned int i;
		const Rule *r;
		Monitor *m;
		XClassHint ch = { NULL, NULL };

		/* rule matching */
		c->isfloating = 0;
		c->tags = 0;
		XGetClassHint(dpy, c->win, &ch);
		class    = ch.res_class ? ch.res_class : broken;
		instance = ch.res_name  ? ch.res_name  : broken;

		for (i = 0; i < LENGTH(subjrules); i++) {
			r = &subjrules[i];
			if ((!r->title || strstr(c->name, r->title))
			&& (!r->class || strstr(class, r->class))
			&& (!r->instance || strstr(instance, r->instance)))
			{
				c->isfloating = r->isfloating;
				c->tags |= r->tags;
				c->priority = r->priority;
				c->nstub = r->nstub;
				for (m = mons; m && m->num != r->monitor; m = m->next);
				if (m)
					c->mon = m;
			}
		}
		if (ch.res_class)
			XFree(ch.res_class);
		if (ch.res_name)
			XFree(ch.res_name);	
	}
	unsigned int maxtags = getmaxtags();
	for(c = selmon->clients; c; c = c->next)
	{
		if (c->tags == 0)
			c->tags = maxtags << 1;
		c->tags = c->tags & TAGMASK ? c->tags & TAGMASK : c->mon->tagset[c->mon->seltags];
	}
	arrange(selmon);
	Arg arg2 = {.ui=selmon->sel->tags};
	view(&arg2);
}

int
applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact)
{
	int baseismin;
	Monitor *m = c->mon;

	/* set minimum possible */
	*w = MAX(1, *w);
	*h = MAX(1, *h);
	if (interact) {
		if (*x > sw)
			*x = sw - WIDTH(c);
		if (*y > sh)
			*y = sh - HEIGHT(c);
		if (*x + *w + 2 * c->bw < 0)
			*x = 0;
		if (*y + *h + 2 * c->bw < 0)
			*y = 0;
	} else {
		if (*x >= m->wx + m->ww)
			*x = m->wx + m->ww - WIDTH(c);
		if (*y >= m->wy + m->wh)
			*y = m->wy + m->wh - HEIGHT(c);
		if (*x + *w + 2 * c->bw <= m->wx)
			*x = m->wx;
		if (*y + *h + 2 * c->bw <= m->wy)
			*y = m->wy;
	}
	if (*h < bh)
		*h = bh;
	if (*w < bh)
		*w = bh;
	if (resizehints || c->isfloating || !c->mon->lt[c->mon->sellt]->arrange) {
		/* see last two sentences in ICCCM 4.1.2.3 */
		baseismin = c->basew == c->minw && c->baseh == c->minh;
		if (!baseismin) { /* temporarily remove base dimensions */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for aspect limits */
		if (c->mina > 0 && c->maxa > 0) {
			if (c->maxa < (float)*w / *h)
				*w = *h * c->maxa + 0.5;
			else if (c->mina < (float)*h / *w)
				*h = *w * c->mina + 0.5;
		}
		if (baseismin) { /* increment calculation requires this */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for increment value */
		if (c->incw)
			*w -= *w % c->incw;
		if (c->inch)
			*h -= *h % c->inch;
		/* restore base dimensions */
		*w = MAX(*w + c->basew, c->minw);
		*h = MAX(*h + c->baseh, c->minh);
		if (c->maxw)
			*w = MIN(*w, c->maxw);
		if (c->maxh)
			*h = MIN(*h, c->maxh);
	}
	return *x != c->x || *y != c->y || *w != c->w || *h != c->h;
}

void
arrange(Monitor *m)
{
	if (m)
		showhide(m->stack);
	else for (m = mons; m; m = m->next)
		showhide(m->stack);
	if (m) {
		arrangemon(m);
		restack(m);
	} else for (m = mons; m; m = m->next)
		arrangemon(m);
}

void
arrangemon(Monitor *m)
{
	strncpy(m->ltsymbol, m->lt[m->sellt]->symbol, sizeof m->ltsymbol);
	if (m->lt[m->sellt]->arrange)
		m->lt[m->sellt]->arrange(m);
}

void
attach(Client *c)
{
	c->next = c->mon->clients;
	c->mon->clients = c;
}

void
attachstack(Client *c)
{
	c->snext = c->mon->stack;
	c->mon->stack = c;
}

void
buttonpress(XEvent *e)
{
	unsigned int i, x, click;
	Arg arg = {0};
	Client *c;
	Monitor *m;
	XButtonPressedEvent *ev = &e->xbutton;
	char *text, *s, ch;

	click = ClkRootWin;
	/* focus monitor if necessary */
	if ((m = wintomon(ev->window)) && m != selmon) {
		unfocus(selmon->sel, 1);
		selmon = m;
		focus(NULL);
	}
	if (ev->window == selmon->barwin) {
		i = x = 0;
		unsigned int occ = 0;
		for(c = m->clients; c; c=c->next)
			occ |= c->tags;
		do {
			/* Do not reserve space for vacant tags */
			if (!(occ & 1 << i || m->tagset[m->seltags] & 1 << i))
				continue;
			x += TEXTW(tags[i]);
		} while (ev->x >= x && ++i < LENGTH(tags));
		if (i < LENGTH(tags)) {
			click = ClkTagBar;
			arg.ui = 1 << i;
			goto execute_handler;
		} else if (ev->x < x + blw) {
			click = ClkLtSymbol;
			goto execute_handler;
		}

		x += blw;

		for(i = 0; i < LENGTH(launchers); i++) {
			x += TEXTW(launchers[i].name);
			if (ev->x < x) {
				Arg a;
				a.v = launchers[i].command;
				spawn(&a);
				return;
			}
		}

		if (ev->x > selmon->ww - getsystraywidth() - statusw) {
			x = selmon->ww - getsystraywidth() - statusw;
			click = ClkStatusText;
			statussig = 0;
			for (text = s = stext; *s && x <= ev->x; s++) {
				if ((unsigned char)(*s) < ' ') {
					ch = *s;
					*s = '\0';
					x += TEXTW(text) - lrpad;
					*s = ch;
					text = s + 1;
					if (x >= ev->x)
						break;
					statussig = ch;
				}
			}
		} else
		{
			click = ClkWinTitle;
			Client *c;
			for(c = selmon->clients; c; c = c->next){
				if (ev->x > c->titlex && ev->x < (c->titlex+c->titlew))
				{
					focus(c);
					break;
				}
			}
		}
	} else if ((c = wintoclient(ev->window))) {
		focus(c);
		LOG("buttonpress.elseif", c->name);
		restack(selmon);
		XAllowEvents(dpy, ReplayPointer, CurrentTime);
		click = ClkClientWin;
	}

execute_handler:

	for (i = 0; i < LENGTH(buttons); i++)
		if (click == buttons[i].click && buttons[i].func && buttons[i].button == ev->button
		&& CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state))
			buttons[i].func(click == ClkTagBar && buttons[i].arg.i == 0 ? &arg : &buttons[i].arg);
}

void
checkotherwm(void)
{
	xerrorxlib = XSetErrorHandler(xerrorstart);
	/* this causes an error if some other window manager is running */
	XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask);
	XSync(dpy, False);
	XSetErrorHandler(xerror);
	XSync(dpy, False);
}

void
cleanup(void)
{
	Arg a = {.ui = ~0};
	Layout foo = { "", NULL };
	Monitor *m;
	size_t i;

	view(&a);
	selmon->lt[selmon->sellt] = &foo;
	for (m = mons; m; m = m->next)
		while (m->stack)
			unmanage(m->stack, 0);
	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	while (mons)
		cleanupmon(mons);

	if (showsystray) {
		XUnmapWindow(dpy, systray->win);
		XDestroyWindow(dpy, systray->win);
		free(systray);
	}

    for (i = 0; i < CurLast; i++)
		drw_cur_free(drw, cursor[i]);
	for (i = 0; i < LENGTH(colors); i++)
		free(scheme[i]);
	XDestroyWindow(dpy, wmcheckwin);
	drw_free(drw);
	XSync(dpy, False);
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
}

void
cleanupmon(Monitor *mon)
{
	Monitor *m;

	if (mon == mons)
		mons = mons->next;
	else {
		for (m = mons; m && m->next != mon; m = m->next);
		m->next = mon->next;
	}
	XUnmapWindow(dpy, mon->barwin);
	XDestroyWindow(dpy, mon->barwin);
	free(mon);
}

void
clientmessage(XEvent *e)
{
	XWindowAttributes wa;
	XSetWindowAttributes swa;
	XClientMessageEvent *cme = &e->xclient;
	Client *c = wintoclient(cme->window);
	unsigned int i;

	if (showsystray && cme->window == systray->win && cme->message_type == netatom[NetSystemTrayOP]) {
		/* add systray icons */
		if (cme->data.l[1] == SYSTEM_TRAY_REQUEST_DOCK) {
			if (!(c = (Client *)calloc(1, sizeof(Client))))
				die("fatal: could not malloc() %u bytes\n", sizeof(Client));
			if (!(c->win = cme->data.l[2])) {
				free(c);
				return;
			}
			c->mon = selmon;
			c->next = systray->icons;
			systray->icons = c;
			if (!XGetWindowAttributes(dpy, c->win, &wa)) {
				/* use sane defaults */
				wa.width = bh;
				wa.height = bh;
				wa.border_width = 0;
			}
			c->x = c->oldx = c->y = c->oldy = 0;
			c->w = c->oldw = wa.width;
			c->h = c->oldh = wa.height;
			c->oldbw = wa.border_width;
			c->bw = 0;
			c->isfloating = True;
			/* reuse tags field as mapped status */
			c->tags = 1;
			updatesizehints(c);
			updatesystrayicongeom(c, wa.width, wa.height);
			XAddToSaveSet(dpy, c->win);
			XSelectInput(dpy, c->win, StructureNotifyMask | PropertyChangeMask | ResizeRedirectMask);
			XReparentWindow(dpy, c->win, systray->win, 0, 0);
			/* use parents background color */
			swa.background_pixel  = scheme[SchemeNorm][ColBg].pixel;
			XChangeWindowAttributes(dpy, c->win, CWBackPixel, &swa);
			sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_EMBEDDED_NOTIFY, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
			/* FIXME not sure if I have to send these events, too */
			sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_FOCUS_IN, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
			sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_WINDOW_ACTIVATE, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
			sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_MODALITY_ON, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
			XSync(dpy, False);
			resizebarwin(selmon);
			updatesystray();
			setclientstate(c, NormalState);
		}
		return;
	}

	if (!c)
		return;
	if (cme->message_type == netatom[NetWMState]) {
		if (cme->data.l[1] == netatom[NetWMFullscreen]
		|| cme->data.l[2] == netatom[NetWMFullscreen])
			setfullscreen(c, (cme->data.l[0] == 1 /* _NET_WM_STATE_ADD    */
				|| (cme->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */ && !c->isfullscreen)));
	} else if (cme->message_type == netatom[NetActiveWindow]) {
		// if (c != selmon->sel && !c->isurgent)
		// 	seturgent(c, 1);
		for (i = 0; i < LENGTH(tags) && !((1 << i) & c->tags); i++);
		if (i < LENGTH(tags)) {
			const Arg a = {.ui = 1 << i};
			selmon = c->mon;
			view(&a);
			focus(c);
			LOG("clientmessage", c->name);
			restack(selmon);
		}
	}
}

void
configure(Client *c)
{
	XConfigureEvent ce;

	ce.type = ConfigureNotify;
	ce.display = dpy;
	ce.event = c->win;
	ce.window = c->win;
	ce.x = c->x;
	ce.y = c->y;
	ce.width = c->w;
	ce.height = c->h;
	ce.border_width = c->bw;
	ce.above = None;
	ce.override_redirect = False;
	XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

void
configurenotify(XEvent *e)
{
	Monitor *m;
	Client *c;
	XConfigureEvent *ev = &e->xconfigure;
	int dirty;

	/* TODO: updategeom handling sucks, needs to be simplified */
	if (ev->window == root) {
		dirty = (sw != ev->width || sh != ev->height);
		sw = ev->width;
		sh = ev->height;
		if (updategeom() || dirty) {
			drw_resize(drw, sw, bh);
			updatebars();
			for (m = mons; m; m = m->next) {
				for (c = m->clients; c; c = c->next)
					if (c->isfullscreen)
						resizeclient(c, m->mx, m->my, m->mw, m->mh);
				resizebarwin(m);
			}
			focus(NULL);
			arrange(NULL);
		}
	}
}

void
configurerequest(XEvent *e)
{
	Client *c;
	Monitor *m;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XWindowChanges wc;

	if ((c = wintoclient(ev->window))) {
		if (ev->value_mask & CWBorderWidth)
			c->bw = ev->border_width;
		else if (c->isfloating || !selmon->lt[selmon->sellt]->arrange) {
			m = c->mon;
			if (ev->value_mask & CWX) {
				c->oldx = c->x;
				c->x = m->mx + ev->x;
			}
			if (ev->value_mask & CWY) {
				c->oldy = c->y;
				c->y = m->my + ev->y;
			}
			if (ev->value_mask & CWWidth) {
				c->oldw = c->w;
				c->w = ev->width;
			}
			if (ev->value_mask & CWHeight) {
				c->oldh = c->h;
				c->h = ev->height;
			}
			if ((c->x + c->w) > m->mx + m->mw && c->isfloating)
				c->x = m->mx + (m->mw / 2 - WIDTH(c) / 2); /* center in x direction */
			if ((c->y + c->h) > m->my + m->mh && c->isfloating)
				c->y = m->my + (m->mh / 2 - HEIGHT(c) / 2); /* center in y direction */
			if ((ev->value_mask & (CWX|CWY)) && !(ev->value_mask & (CWWidth|CWHeight)))
				configure(c);
			if (ISVISIBLE(c))
				XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
		} else
			configure(c);
	} else {
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
	}
	XSync(dpy, False);
}

Monitor *
createmon(void)
{
	Monitor *m;
	unsigned int i;

	m = ecalloc(1, sizeof(Monitor));
	m->tagset[0] = m->tagset[1] = 1;
	m->mfact = mfact;
	m->nmaster = nmaster;
	m->showbar = showbar;
	m->topbar = topbar;
	m->gap = malloc(sizeof(Gap));
	gap_copy(m->gap, &default_gap);
	m->lt[0] = &layouts[0];
	m->lt[1] = &layouts[1 % LENGTH(layouts)];
	strncpy(m->ltsymbol, layouts[0].symbol, sizeof m->ltsymbol);
	m->pertag = ecalloc(1, sizeof(Pertag));
	m->pertag->curtag = m->pertag->prevtag = 1;

	for (i = 0; i <= LENGTH(tags); i++) {
		m->pertag->nmasters[i] = m->nmaster;
		m->pertag->mfacts[i] = m->mfact;

		m->pertag->ltidxs[i][0] = m->lt[0];
		m->pertag->ltidxs[i][1] = m->lt[1];
		m->pertag->sellts[i] = m->sellt;

		m->pertag->showbars[i] = m->showbar;
	}

	return m;
}

void
destroynotify(XEvent *e)
{
	Client *c;
	XDestroyWindowEvent *ev = &e->xdestroywindow;

	if ((c = wintoclient(ev->window)))
		unmanage(c, 1);
	else if ((c = wintosystrayicon(ev->window))) {
		removesystrayicon(c);
		resizebarwin(selmon);
		updatesystray();
	}
}

void
detach(Client *c)
{
	Client **tc;

	for (tc = &c->mon->clients; *tc && *tc != c; tc = &(*tc)->next);
	*tc = c->next;
}

void
detachstack(Client *c)
{
	Client **tc, *t;

	for (tc = &c->mon->stack; *tc && *tc != c; tc = &(*tc)->snext);
	*tc = c->snext;

	if (c == c->mon->sel) {
		for (t = c->mon->stack; t && !ISVISIBLE(t); t = t->snext);
		c->mon->sel = t;
	}
}

Monitor *
dirtomon(int dir)
{
	Monitor *m = NULL;

	if (dir > 0) {
		if (!(m = selmon->next))
			m = mons;
	} else if (selmon == mons)
		for (m = mons; m->next; m = m->next);
	else
		for (m = mons; m->next != selmon; m = m->next);
	return m;
}

void
drawbar(Monitor *m)
{
	int x, w, tw = 0, stw = 0, mw, ew = 0;
	int boxs = drw->fonts->h / 9;
	int boxw = drw->fonts->h / 6 + 2;
	unsigned int i, occ = 0, urg = 0, n = 0;
	Client *c;

	if (!m->showbar)
		return;

	if(showsystray && m == systraytomon(m) && !systrayonleft)
		stw = getsystraywidth();

	/* draw status first so it can be overdrawn by tags later */
	if (m == selmon) { /* status is only drawn on selected monitor */
		char *text, *s, ch;
		drw_setscheme(drw, scheme[SchemeNorm]);

		x = 0;
		for (text = s = stext; *s; s++) {
			if ((unsigned char)(*s) < ' ') {
				ch = *s;
				*s = '\0';
				tw = TEXTW(text) - lrpad;
				drw_text(drw, m->ww - stw - statusw + x, 0, tw, bh, 0, text, 0);
				x += tw;
				*s = ch;
				text = s + 1;
			}
		}
		tw = TEXTW(text) - lrpad + 2;
		drw_text(drw, m->ww - stw - statusw + x, 0, tw, bh, 0, text, 0);
		tw = statusw;
	}

	resizebarwin(m);
	for (c = m->clients; c; c = c->next) {
		if (ISVISIBLE(c))
			n++;
		occ |= c->tags;
		if (c->isurgent)
			urg |= c->tags;
	}
	x = 0;
	for (i = 0; i < LENGTH(tags); i++) {
		/* Do not draw vacant tags */
		if(!(occ & 1 << i || m->tagset[m->seltags] & 1 << i))
			continue;
		w = TEXTW(tags[i]);
		drw_setscheme(drw, scheme[m->tagset[m->seltags] & 1 << i ? SchemeSel : SchemeNorm]);
		drw_text(drw, x, 0, w, bh, lrpad / 2, tags[i], urg & 1 << i);
		x += w;
	}
	w = blw = TEXTW(m->ltsymbol);
	drw_setscheme(drw, scheme[SchemeNorm]);
	x = drw_text(drw, x, 0, w, bh, lrpad / 2, m->ltsymbol, 0);
	
	for (i = 0; i < LENGTH(launchers); i++)
	{
		w = TEXTW(launchers[i].name);
		drw_text(drw, x, 0, w, bh, lrpad / 2, launchers[i].name, urg & 1 << i);
		x += w;
	}

	if ((w = m->ww - tw - stw - x) > bh) {
		if (n > 0) {
			tw = TEXTW(m->sel->name) + lrpad;
			mw = (tw >= w || n == 1) ? 0 : (w - tw) / (n - 1);

			i = 0;
			for (c = m->clients; c; c = c->next) {
				if (!ISVISIBLE(c) || c == m->sel)
					continue;
				tw = TEXTW(c->name);
				if(tw < mw)
					ew += (mw - tw);
				else
					i++;
			}
			if (i > 0)
				mw += ew / i;

			for (c = m->clients; c; c = c->next) {
				if (!ISVISIBLE(c))
					continue;
				tw = MIN(m->sel == c ? w : mw, TEXTW(c->name));

				drw_setscheme(drw, scheme[m->sel == c ? SchemeSel : SchemeNorm]);
				if (tw > 0) /* trap special handling of 0 in drw_text */
				{
					drw_text(drw, x, 0, tw, bh, lrpad / 2, c->name, 0);
					c->titlex = x;
					c->titlew = tw;
				}
				if (c->isfloating)
					drw_rect(drw, x + boxs, boxs, boxw, boxw, c->isfixed, 0);
				x += tw;
				w -= tw;
			}
		}
		drw_setscheme(drw, scheme[SchemeNorm]);
		drw_rect(drw, x, 0, w, bh, 1, 1);
	}
	drw_map(drw, m->barwin, 0, 0, m->ww - stw, bh);
}

void
drawbars(void)
{
	Monitor *m;

	for (m = mons; m; m = m->next)
		drawbar(m);
}

void
enqueue(Client *c)
{
	Client *l;
	for (l = c->mon->clients; l && l->next; l = l->next);
	if (l) {
		l->next = c;
		c->next = NULL;
	}
}

void
enqueuestack(Client *c)
{
	Client *l;
	for (l = c->mon->stack; l && l->snext; l = l->snext);
	if (l) {
		l->snext = c;
		c->snext = NULL;
	}
}

void
enternotify(XEvent *e)
{
	Client *c;
	Monitor *m;
	XCrossingEvent *ev = &e->xcrossing;

	if ((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
		return;
	c = wintoclient(ev->window);
	m = c ? c->mon : wintomon(ev->window);
	if (m != selmon) {
		unfocus(selmon->sel, 1);
		selmon = m;
	} else if (!c || c == selmon->sel)
		return;
	focus(c);
	LOG("enternotify", c->name);
	arrange(m);
}

void
expose(XEvent *e)
{
	Monitor *m;
	XExposeEvent *ev = &e->xexpose;

	if (ev->count == 0 && (m = wintomon(ev->window))) {
		drawbar(m);
		if (m == selmon)
			updatesystray();
	}
}

void
focus(Client *c)
{
	if (!c || !ISVISIBLE(c))
		for (c = selmon->stack; c && !ISVISIBLE(c); c = c->snext);
	if (selmon->sel && selmon->sel != c)
		unfocus(selmon->sel, 0);
	if (c) {
		if (c->mon != selmon)
			selmon = c->mon;
		if (c->isurgent)
			seturgent(c, 0);
		detachstack(c);
		attachstack(c);
		grabbuttons(c, 1);
		if(c->isscratched)
			XSetWindowBorder(dpy, c->win, scheme[SchemeScr][ColBorder].pixel);
		else
			XSetWindowBorder(dpy, c->win, scheme[SchemeSel][ColBorder].pixel);
		setfocus(c);
	} else {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
	selmon->sel = c;
	drawbars();
}

/* there are some broken focus acquiring clients needing extra handling */
void
focusin(XEvent *e)
{
	XFocusChangeEvent *ev = &e->xfocus;

	if (selmon->sel && ev->window != selmon->sel->win)
		setfocus(selmon->sel);
}

void
focusmon(const Arg *arg)
{
	Monitor *m;

	if (!mons->next)
		return;
	if ((m = dirtomon(arg->i)) == selmon)
		return;
	unfocus(selmon->sel, 0);
	selmon = m;
	focus(NULL);
}

void
focusstack(const Arg *arg)
{
	Client *c = NULL, *i;

	if (!selmon->sel || (selmon->sel->isfullscreen && lockfullscreen))
		return;
	if (arg->i > 0) {
		for (c = selmon->sel->next; c && !ISVISIBLE(c); c = c->next);
		if (!c)
			for (c = selmon->clients; c && !ISVISIBLE(c); c = c->next);
	} else {
		for (i = selmon->clients; i != selmon->sel; i = i->next)
			if (ISVISIBLE(i))
				c = i;
		if (!c)
			for (; i; i = i->next)
				if (ISVISIBLE(i))
					c = i;
	}
	if (c) {
		focus(c);
		restack(selmon);
	}
}


void 
lru(Client *c)
{
	if(!c) return;
	if(c == focuschain->lastfocus) return;
	// LOG_FORMAT("lru2 start \n");
	c->focusfreq++;
	Client *tmp = NULL, *prev = NULL;
	for(tmp = focuschain; tmp; tmp = tmp->lastfocus)
	{
		if(c == tmp) break;
		prev = tmp;
	}
	if(tmp)
	{
		prev->lastfocus = tmp->lastfocus;
		tmp->lastfocus = focuschain->lastfocus;
		focuschain->lastfocus = tmp;
	}
	else
	{
		c->lastfocus = focuschain->lastfocus;
		focuschain->lastfocus = c;
	}

	// LOG_FORMAT("lru end \n");
}

void
removefromfocuschain(Client *c)
{
	if(!c) return;
	Client *tmp = NULL, *prev = NULL;
	for(tmp = focuschain; tmp; tmp = tmp->lastfocus)
	{
		if(c == tmp) break;
		prev = tmp;
	}
	if(tmp)
		prev->lastfocus = tmp->lastfocus;
}

Client *
closestxclient(Client *t, Client *c1, Client *c2){
	if (abs(c1->x - t->x) > abs(c2->x - t->x))
		return c2;
	else
		return c1;
}

int
avgx(Client *c)
{
	return c->x + c->w / 2;
}

Client *
closestxgravityclient(Client *t, Client *c1, Client *c2)
{
	if(t == c1) return c2;
	if(t == c2) return c1;
	if (abs(avgx(c1) - avgx(t)) > abs(avgx(c2) - avgx(t)))
		return c2;
	else
		return c1;
}

Client *
closestyclient(Client *t, Client *c1, Client *c2)
{
	if (abs(c1->y - t->y) > abs(c2->y - t->y))
		return c2;
	else
		return c1;
}

double 
score(Client *c)
{
	int i = 0;
	Client *tmp;
	for(tmp = focuschain; tmp; tmp = tmp->lastfocus)
	{
		if(tmp == c) break;
		i++;
	}
	return log(c->fullscreenfreq+1) + log(1.0/(i+1));
}

Client *
smartchoose(Client *t, Client *c1, Client *c2)
{
	if (score(c1) < score(c2))
		return c2;
	else
		return c1;
}


void
focusgrid(const Arg *arg)
{
	Client *c = NULL;
	Client *cc = selmon->sel;

	if (!selmon->sel || (selmon->sel->isfullscreen && lockfullscreen)) return;
	if (arg->i == FOCUS_LEFT) {
		Client *closest = NULL;
		int min = INT_MAX;
		for (c = selmon->clients; c; c = c->next)
		{
			if (cc && c != cc && c->x < cc->x && ISVISIBLE(c))
			{
				if (abs(cc->x - c->x) < min)
				{
					min = abs(cc->x - c->x);
					closest = c;
				}else if (abs(cc->x - c->x) == min)
					closest = smartchoose(cc, c, closest);
			}
		}
		c = closest;
	}
	else if (arg->i == FOCUS_RIGHT) {
		Client *closest = NULL;
		int min = INT_MAX;
		for (c = selmon->clients; c; c = c->next)
		{
			if (cc && c != cc&& c->x > cc->x && ISVISIBLE(c))
			{
				if (abs(cc->x - c->x) < min)
				{
					min = abs(cc->x - c->x);
					closest = c;
				}
				else if (abs(cc->x - c->x) == min)
					closest = smartchoose(cc, c, closest);
			}
		}
		c = closest;
	}
	else if (arg->i == FOCUS_UP)
	{
		Client *closest = NULL;
		int min = INT_MAX;
		for (c = selmon->clients; c; c = c->next)
		{
			if (cc&& c != cc && c->y < cc->y && ISVISIBLE(c))
			{
				if (abs(cc->y - c->y) < min && (cc->x - c->x <= cc->bw + c->bw || cc->x + cc->w - (c->x + c->w) <= cc->bw + c->bw))
				{
					min = abs(cc->y - c->y);
					closest = c;
				}
				else if (abs(cc->y - c->y) == min)
					closest = closestxgravityclient(cc, c, closest);
			}
		}
		c = closest;
	}
	else if (arg->i == FOCUS_DOWN)
	{
		Client *closest = NULL;
		int min = INT_MAX;
		for (c = selmon->clients; c; c = c->next)
		{
			if (cc && c != cc&& c->y > cc->y && ISVISIBLE(c))
			{
				if (abs(cc->y - c->y) < min && (cc->x - c->x <= cc->bw + c->bw || cc->x + cc->w - (c->x + c->w) <= cc->bw + c->bw))
				{
					min = abs(cc->y - c->y);
					closest = c;
				}
				else if (abs(cc->y - c->y) == min)
					closest = closestxgravityclient(cc, c, closest);
			}
		}
		c = closest;
	}
	if(cc && selmon->sellt == 1){
		if(arg->i == FOCUS_LEFT) {
			Client *i;
			for(i = selmon->clients;i;i=i->next){
				if (!ISVISIBLE(i)) continue;
				if(i->next == cc) break;
			}
			if (!i)
			{
				for(i = selmon->clients;i && ISVISIBLE(i) && i->next;i=i->next);
			}
			c = i;
		}
		if(arg->i == FOCUS_RIGHT) {
			c = cc->next;
			if (!c)
			{
				Client *i;
				for(i = selmon->clients;i && !ISVISIBLE(i);i=i->next);
				c = i;
			}
		}
	}
	if (c) {
		focus(c);
		restack(selmon);
		arrange(selmon);
	}

}

Atom
getwinatomprop(Window win, Atom prop)
{
	int di;
	unsigned long dl;
	unsigned char *p = NULL;
	Atom da, atom = None;

	/* FIXME getatomprop should return the number of items and a pointer to
	 * the stored data instead of this workaround */
	Atom req = XA_ATOM;
	if (prop == xatom[XembedInfo])
		req = xatom[XembedInfo];

	if (XGetWindowProperty(dpy, win, prop, 0L, sizeof atom, False, req,
		&da, &di, &dl, &dl, &p) == Success && p) {
		atom = *(Atom *)p;
		if (da == xatom[XembedInfo] && dl == 2)
			atom = ((Atom *)p)[1];
		XFree(p);
	}
	return atom;
}

Atom
getatomprop(Client *c, Atom prop)
{
	return getwinatomprop(c->win, prop);
}


pid_t
getstatusbarpid()
{
	char buf[32], *str = buf, *c;
	FILE *fp;

	if (statuspid > 0) {
		snprintf(buf, sizeof(buf), "/proc/%u/cmdline", statuspid);
		if ((fp = fopen(buf, "r"))) {
			fgets(buf, sizeof(buf), fp);
			while ((c = strchr(str, '/')))
				str = c + 1;
			fclose(fp);
			if (!strcmp(str, STATUSBAR))
				return statuspid;
		}
	}
	if (!(fp = popen("pidof -s "STATUSBAR, "r")))
		return -1;
	fgets(buf, sizeof(buf), fp);
	pclose(fp);
	return strtol(buf, NULL, 10);
}

int
getrootptr(int *x, int *y)
{
	int di;
	unsigned int dui;
	Window dummy;

	return XQueryPointer(dpy, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

long
getstate(Window w)
{
	int format;
	long result = -1;
	unsigned char *p = NULL;
	unsigned long n, extra;
	Atom real;

	if (XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False, wmatom[WMState],
		&real, &format, &n, &extra, (unsigned char **)&p) != Success)
		return -1;
	if (n != 0)
		result = *p;
	XFree(p);
	return result;
}

unsigned int
getsystraywidth()
{
	unsigned int w = 0;
	Client *i;
	if(showsystray)
		for(i = systray->icons; i; w += i->w + systrayspacing, i = i->next) ;
	return w ? w + systrayspacing : 1;
}



int
gettextprop(Window w, Atom atom, char *text, unsigned int size)
{
	char **list = NULL;
	int n;
	XTextProperty name;

	if (!text || size == 0)
		return 0;
	text[0] = '\0';
	if (!XGetTextProperty(dpy, w, &name, atom) || !name.nitems)
		return 0;
	if (name.encoding == XA_STRING)
		strncpy(text, (char *)name.value, size - 1);
	else {
		if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success && n > 0 && *list) {
			strncpy(text, *list, size - 1);
			XFreeStringList(list);
		}
	}
	text[size - 1] = '\0';
	XFree(name.value);
	return 1;
}

void
grabbuttons(Client *c, int focused)
{
	updatenumlockmask();
	{
		unsigned int i, j;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		if (!focused)
			XGrabButton(dpy, AnyButton, AnyModifier, c->win, False,
				BUTTONMASK, GrabModeSync, GrabModeSync, None, None);
		for (i = 0; i < LENGTH(buttons); i++)
			if (buttons[i].click == ClkClientWin)
				for (j = 0; j < LENGTH(modifiers); j++)
					XGrabButton(dpy, buttons[i].button,
						buttons[i].mask | modifiers[j],
						c->win, False, BUTTONMASK,
						GrabModeAsync, GrabModeSync, None, None);
	}
}

void
grabkeys(void)
{
	updatenumlockmask();
	{
		unsigned int i, j;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		KeyCode code;

		XUngrabKey(dpy, AnyKey, AnyModifier, root);
		for (i = 0; i < LENGTH(keys); i++)
			if ((code = XKeysymToKeycode(dpy, keys[i].keysym)))
				for (j = 0; j < LENGTH(modifiers); j++)
					XGrabKey(dpy, code, keys[i].mod | modifiers[j], root,
						True, GrabModeAsync, GrabModeAsync);
	}
}

void
incnmaster(const Arg *arg)
{
	unsigned int i;
	selmon->nmaster = MAX(selmon->nmaster + arg->i, 0);
	for(i=0; i<LENGTH(tags); ++i)
		if(selmon->tagset[selmon->seltags] & 1<<i)
			selmon->pertag->nmasters[i+1] = selmon->nmaster;
	
	if(selmon->pertag->curtag == 0)
	{
		selmon->pertag->nmasters[0] = selmon->nmaster;
	}
	arrange(selmon);
}

#ifdef XINERAMA
static int
isuniquegeom(XineramaScreenInfo *unique, size_t n, XineramaScreenInfo *info)
{
	while (n--)
		if (unique[n].x_org == info->x_org && unique[n].y_org == info->y_org
		&& unique[n].width == info->width && unique[n].height == info->height)
			return 0;
	return 1;
}
#endif /* XINERAMA */

void
keypress(XEvent *e)
{
	unsigned int i;
	KeySym keysym;
	XKeyEvent *ev;

	ev = &e->xkey;
	keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0);
	for (i = 0; i < LENGTH(keys); i++)
		if (keysym == keys[i].keysym
		&& CLEANMASK(keys[i].mod) == CLEANMASK(ev->state)
		&& keys[i].func)
			keys[i].func(&(keys[i].arg));
}




void
killclient(const Arg *arg)
{
	if (!selmon->sel)
		return;

	if (!sendevent(selmon->sel->win, wmatom[WMDelete], NoEventMask, wmatom[WMDelete], CurrentTime, 0 , 0, 0)) {
		XGrabServer(dpy);
		XSetErrorHandler(xerrordummy);
		XSetCloseDownMode(dpy, DestroyAll);
		XKillClient(dpy, selmon->sel->win);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}

}

int 
counttagnstub(Client *clients, int tags){
	if (!clients)
	{
		return 0;
	}
	
	Client *tmp;
	int i = 0;
	for(tmp = clients;tmp ;tmp = tmp->next)
	{
		if ((tmp->tags & tags) && !tmp->isfloating)
			i+=(tmp->nstub+1);
	}
	return i;
}

int 
countcurtagnstub(Client *clients){
	counttagnstub(clients, selmon->tagset[selmon->seltags]);
}

int counttag(Client *clients, int tags){
	int i = 0;
	Client *tmp;
	for (tmp = clients; tmp; tmp = tmp->next)
		if ((tmp->tags & tags) > 0)
			i ++;
	return i;
}

int countcurtag(Client *clients)
{
	counttag(clients, selmon->tagset[selmon->seltags]);
}

Rule * 
getwinrule(Window win)
{
	const Rule *target = &defaultrule;
	const char *class, *instance;
	
	XClassHint ch = { NULL, NULL };
	XGetClassHint(dpy, win, &ch);
	class    = ch.res_class ? ch.res_class : broken;
	instance = ch.res_name  ? ch.res_name  : broken;

	const Rule *r;
	unsigned int i;
	for (i = 0; i < LENGTH(rules); i++) {
		r = &rules[i];
		if ((r->class && strstr(class, r->class))
		    || (r->instance && strstr(instance, r->instance)))
		{
			// LOG_FORMAT(" %s:%s ", r->class, class);
			target = r;
		}
	}
	if (ch.res_class)
		XFree(ch.res_class);
	if (ch.res_name)
		XFree(ch.res_name);
	return target;
}


unsigned long
getwindowpid(Window w)
{
	Atom           type;
	int            format;
	unsigned long  nItems;
	unsigned long  bytesAfter;
	unsigned char *propPID = 0;
	unsigned long pid;
	if(Success == XGetWindowProperty(dpy, w, netatom[NetWmPid], 0, 1, False, XA_CARDINAL,
										&type, &format, &nItems, &bytesAfter, &propPID))
	{
		if(propPID){
			pid = *((unsigned long *)propPID);
			XFree(propPID);
		}
		else 
			return 0;
	}
	return pid;
}


unsigned long getppidof(unsigned long pid)
{
    char dir[1024]={0};
    char path[1028] = {0};
    char buf[1024] = {0};
    int rpid = 0;
    unsigned long fpid=0;
    char fpth[1024]={0};
    ssize_t ret =0;
    sprintf(dir,"/proc/%ld/",pid);
    sprintf(path,"%sstat",dir);
	struct stat st;
	if(stat(path,&st)!=0)
    {
        return 0; 
	}
    memset(buf,0,strlen(buf));
    FILE * fp = fopen(path,"r");
	if(!fp) return 0;
    ret += fread(buf + ret,1,300-ret,fp);
    fclose(fp);
    sscanf(buf,"%*d %*c%s %*c %d %*s",fpth,&fpid);
    fpth[strlen(fpth)-1]='\0';
    return fpid;
}

void
getppidchain(unsigned long pid, unsigned long *ppidchain, int size, int index){
	if (index < size && pid > 1)
	{
		ppidchain[index] = getppidof(pid);
		getppidchain(ppidchain[index], ppidchain, size, index +1);
	}
}

int 
manageppidstick(Client *c){
	Client *tmpparent;
	int found = 0;
	unsigned long pid = getwindowpid(c->win);
	c->pid = pid;
	if(pid){
		int PPIDCHAIN_N = 10;
		unsigned long ppidchain[PPIDCHAIN_N];
		getppidchain(pid, ppidchain, PPIDCHAIN_N, 0);
		for (tmpparent = selmon->stack; tmpparent; tmpparent = tmpparent->snext)
		{
			for (int i = 0; i < PPIDCHAIN_N; i++)
			{
				if (ppidchain[i] <= 1)
					break;
				if (ppidchain[i] == tmpparent->pid)
				{
					found = 1;
					break;
				}
			}
			if (found)
				break;
		}
	}
	if(found && tmpparent && tmpparent->tags != 0xFFFFFFFF){
		Rule * rule = getwinrule(c->win);
		if (!rule->isfloating)
		{
			c->tags = tmpparent->tags;
			Arg arg = {.ui= tmpparent->tags };
			view(&arg);
			return 1;
		}else{
			return 1;
		}
	}
	return 0;
}

void 
managestub(Client *c){
	// 每个tag不能超过 n 个client, 超过则移动到下一个tag
	int curisfloating = 0;
	Atom wtype = getwinatomprop(c->win, netatom[NetWMWindowType]);
	if (wtype == netatom[NetWMWindowTypeDialog]) curisfloating = 1;
	if (!curisfloating)
	{
		Rule * rule = getwinrule(c->win);
		if (!rule->isfloating)
		{
			int tmptags = selmon->tagset[selmon->seltags];
			while (counttagnstub(selmon->clients, tmptags) >= (3 - rule->nstub))
				tmptags = tmptags << 1;
			c->tags = tmptags;
			Arg arg = {.ui= tmptags };
			view(&arg);
			if (rule->nstub == 2 && selmon->sellt == 0)
			{
				const Arg arg = {.v = &layouts[1]};
				setlayout(&arg);
			}else{
				const Arg arg = {.v = &layouts[selmon->sellt]};
				setlayout(&arg);
			}	
		}
	}
}


int 
scratchsingle(char *cmd[]){
	const ScratchItem *si = NULL;
	for (si = scratchgroupptr->head->next; si && si !=  scratchgroupptr->tail; si = si->next)
	{
		if (si->cmd == cmd){
			return 1;
		}
	}
	return 0;
}


void
manage(Window w, XWindowAttributes *wa)
{
	// hidescratchgroup if needed (example: open app from terminal)
	if(scratchgroupptr->isfloating)
		hidescratchgroupv(scratchgroupptr, 0);

	Client *c, *t = NULL;
	Window trans = None;
	XWindowChanges wc;

	c = ecalloc(1, sizeof(Client));
	c->win = w;
	/* geometry */
	c->x = c->oldx = wa->x;
	c->y = c->oldy = wa->y;
	c->w = c->oldw = wa->width;
	c->h = c->oldh = wa->height;
	c->oldbw = wa->border_width;

	updatetitle(c);

	if (XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans))) {
		c->mon = t->mon;
		c->tags = t->tags;
		c->nstub = 0;
	} else {
		c->mon = selmon;
		applyrules(c);
	}

	if(!manageppidstick(c) && !isnextscratch) managestub(c);

	if (c->x + WIDTH(c) > c->mon->mx + c->mon->mw)
		c->x = c->mon->mx + c->mon->mw - WIDTH(c);
	if (c->y + HEIGHT(c) > c->mon->my + c->mon->mh)
		c->y = c->mon->my + c->mon->mh - HEIGHT(c);
	c->x = MAX(c->x, c->mon->mx);
	/* only fix client y-offset, if the client center might cover the bar */
	c->y = MAX(c->y, ((c->mon->by == c->mon->my) && (c->x + (c->w / 2) >= c->mon->wx)
		&& (c->x + (c->w / 2) < c->mon->wx + c->mon->ww)) ? bh : c->mon->my);
	if (selmon->sellt == 1)
	{
		c->bw = 0;
	}else{
		c->bw = borderpx;
	}

	selmon->tagset[selmon->seltags] &= ~scratchtag;
	if (!strcmp(c->name, scratchpadname)) {
		c->mon->tagset[c->mon->seltags] |= c->tags = scratchtag;
		c->isfloating = True;
		c->x = c->mon->wx + (c->mon->ww / 2 - WIDTH(c) / 2);
		c->y = c->mon->wy + (c->mon->wh / 2 - HEIGHT(c) / 2);
	}

	wc.border_width = c->bw;
	XConfigureWindow(dpy, w, CWBorderWidth, &wc);
	XSetWindowBorder(dpy, w, scheme[SchemeNorm][ColBorder].pixel);
	configure(c); /* propagates border_width, if size doesn't change */
	updatewindowtype(c);
	updatesizehints(c);
	updatewmhints(c);
	XSelectInput(dpy, w, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
	grabbuttons(c, 0);
	if (!c->isfloating)
		c->isfloating = c->oldstate = trans != None || c->isfixed;
	if (c->isfloating)
		XRaiseWindow(dpy, c->win);
	attach(c);
	attachstack(c);
	XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
		(unsigned char *) &(c->win), 1);
	XMoveResizeWindow(dpy, c->win, c->x + 2 * sw, c->y, c->w, c->h); /* some windows require this */
	setclientstate(c, NormalState);
	if (c->mon == selmon)
		unfocus(selmon->sel, 0);
	c->mon->sel = c;
	// for sspawn
	if(isnextscratch){
		unsigned int curtags = c->tags;
		ScratchItem* si = addtoscratchgroupc(c);
		c->tags = curtags;
		isnextscratch = 0;
		si->pretags = 1 << (LENGTH(tags) - 1);
		si->cmd = nextscratchcmd;
		nextscratchcmd = NULL;
	}

	arrange(c->mon);
	XMapWindow(dpy, c->win);
	focus(NULL);

	Client *maxpc = NULL;
	Client *tmp = NULL;
	int maxp = 0;
	for (tmp = selmon->clients; tmp; tmp = tmp->next)
	{
		if(ISVISIBLE(tmp)){
			if(tmp->priority > maxp){
				maxpc = tmp;
				maxp = tmp->priority;
			}	
		}
	}
	if (c->priority > 0 && maxpc != c)
	{
		Arg arg = {0};
		zoom(&arg);
	}
	
}

void
mappingnotify(XEvent *e)
{
	XMappingEvent *ev = &e->xmapping;

	XRefreshKeyboardMapping(ev);
	if (ev->request == MappingKeyboard)
		grabkeys();
}

void
maprequest(XEvent *e)
{
	static XWindowAttributes wa;
	XMapRequestEvent *ev = &e->xmaprequest;

	Client *i;
	if ((i = wintosystrayicon(ev->window))) {
		sendevent(i->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_WINDOW_ACTIVATE, 0, systray->win, XEMBED_EMBEDDED_VERSION);
		resizebarwin(selmon);
		updatesystray();
	}

	if (!XGetWindowAttributes(dpy, ev->window, &wa))
		return;
	if (wa.override_redirect)
		return;
	if (!wintoclient(ev->window))
		manage(ev->window, &wa);
}

void
monocle(Monitor *m)
{
	unsigned int n = 0;
	Client *c;

	for (c = m->clients; c; c = c->next)
		if (ISVISIBLE(c))
			n++;
	if (n > 0) /* override layout symbol */
		snprintf(m->ltsymbol, sizeof m->ltsymbol, "[%d]", n);
	for (c = nexttiled(m->clients); c; c = nexttiled(c->next)){
		unsigned int tmpbw = c->bw;
		c->bw = 0;
		resize(c, m->wx, m->wy, m->ww - 2 * c->bw, m->wh - 2 * c->bw, 0);
		c->bw = tmpbw;
	}
		
	if(m->sel) m->sel->fullscreenfreq++;
}

void
motionnotify(XEvent *e)
{
	static Monitor *mon = NULL;
	Monitor *m;
	XMotionEvent *ev = &e->xmotion;

	if (ev->window != root)
		return;
	if ((m = recttomon(ev->x_root, ev->y_root, 1, 1)) != mon && mon) {
		unfocus(selmon->sel, 1);
		selmon = m;
		focus(NULL);
	}
	mon = m;
}

void
movemouse(const Arg *arg)
{
	int x, y, ocx, ocy, nx, ny;
	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel))
		return;
	if (c->isfullscreen) /* no support moving fullscreen windows by mouse */
		return;
	restack(selmon);
	ocx = c->x;
	ocy = c->y;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurMove]->cursor, CurrentTime) != GrabSuccess)
		return;
	if (!getrootptr(&x, &y))
		return;
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / 60))
				continue;
			lasttime = ev.xmotion.time;

			nx = ocx + (ev.xmotion.x - x);
			ny = ocy + (ev.xmotion.y - y);
			if (abs(selmon->wx - nx) < snap)
				nx = selmon->wx;
			else if (abs((selmon->wx + selmon->ww) - (nx + WIDTH(c))) < snap)
				nx = selmon->wx + selmon->ww - WIDTH(c);
			if (abs(selmon->wy - ny) < snap)
				ny = selmon->wy;
			else if (abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c))) < snap)
				ny = selmon->wy + selmon->wh - HEIGHT(c);
			if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
			&& (abs(nx - c->x) > snap || abs(ny - c->y) > snap))
				togglefloating(NULL);
			if (!selmon->lt[selmon->sellt]->arrange || c->isfloating)
				resize(c, nx, ny, c->w, c->h, 1);
			LOG("movemouse.motionNotify", c->name);
			break;
		}
	} while (ev.type != ButtonRelease);
	XUngrabPointer(dpy, CurrentTime);
	if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selmon = m;
		focus(NULL);
		LOG("movemouse.motionNotify.if", c->name);
	}
}

Client *
nexttiled(Client *c)
{
	for (; c && (c->isfloating || !ISVISIBLE(c)); c = c->next);
	return c;
}

void
pop(Client *c)
{
	detach(c);
	attach(c);
	focus(c);
	arrange(c->mon);
}

void
propertynotify(XEvent *e)
{
	Client *c;
	Window trans;
	XPropertyEvent *ev = &e->xproperty;

	if ((c = wintosystrayicon(ev->window))) {
		if (ev->atom == XA_WM_NORMAL_HINTS) {
			updatesizehints(c);
			updatesystrayicongeom(c, c->w, c->h);
		}
		else
			updatesystrayiconstate(c, ev);
		resizebarwin(selmon);
		updatesystray();
	}

    if ((ev->window == root) && (ev->atom == XA_WM_NAME))
		updatestatus();
	else if (ev->state == PropertyDelete)
		return; /* ignore */
	else if ((c = wintoclient(ev->window))) {
		switch(ev->atom) {
		default: break;
		case XA_WM_TRANSIENT_FOR:
			if (!c->isfloating && (XGetTransientForHint(dpy, c->win, &trans)) 
			// idea 弹窗后其他窗口会变成floating, 这里先注释掉
			// && (c->isfloating = (wintoclient(trans)) != NULL)
			)
				arrange(c->mon);
			break;
		case XA_WM_NORMAL_HINTS:
			updatesizehints(c);
			break;
		case XA_WM_HINTS:
			updatewmhints(c);
			drawbars();
			break;
		}
		if (ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
			updatetitle(c);
			if (c == c->mon->sel)
				drawbar(c->mon);
		}
		if (ev->atom == netatom[NetWMWindowType])
			updatewindowtype(c);
	}
}

void
quit(const Arg *arg)
{
	if(arg->i) {
		restart = 1;
		running = 0;
    }
	FILE *fd = NULL;
	struct stat filestat;

	if ((fd = fopen(lockfile, "r")) && stat(lockfile, &filestat) == 0) {
		fclose(fd);

		if (filestat.st_ctime <= time(NULL)-2)
			remove(lockfile);
	}

	if ((fd = fopen(lockfile, "r")) != NULL) {
		fclose(fd);
		remove(lockfile);
		running = 0;
	} else {
		if ((fd = fopen(lockfile, "a")) != NULL)
			fclose(fd);
	}
}

Monitor *
recttomon(int x, int y, int w, int h)
{
	Monitor *m, *r = selmon;
	int a, area = 0;

	for (m = mons; m; m = m->next)
		if ((a = INTERSECT(x, y, w, h, m)) > area) {
			area = a;
			r = m;
		}
	return r;
}

void
removesystrayicon(Client *i)
{
	Client **ii;

	if (!showsystray || !i)
		return;
	for (ii = &systray->icons; *ii && *ii != i; ii = &(*ii)->next);
	if (ii)
		*ii = i->next;
	free(i);
}

void
resize(Client *c, int x, int y, int w, int h, int interact)
{
	if (applysizehints(c, &x, &y, &w, &h, interact))
		resizeclient(c, x, y, w, h);
}

void
resizebarwin(Monitor *m) {
	unsigned int w = m->ww;
	if (showsystray && m == systraytomon(m) && !systrayonleft)
		w -= getsystraywidth();
	XMoveResizeWindow(dpy, m->barwin, m->wx, m->by, w, bh);
}

void
resizeclient(Client *c, int x, int y, int w, int h)
{
	XWindowChanges wc;

	c->oldx = c->x; c->x = wc.x = x;
	c->oldy = c->y; c->y = wc.y = y;
	c->oldw = c->w; c->w = wc.width = w;
	c->oldh = c->h; c->h = wc.height = h;
	wc.border_width = c->bw;
	XConfigureWindow(dpy, c->win, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
	configure(c);
	XSync(dpy, False);
}

void
resizemouse(const Arg *arg)
{
	int ocx, ocy, nw, nh;
	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel))
		return;
	if (c->isfullscreen) /* no support resizing fullscreen windows by mouse */
		return;
	restack(selmon);
	ocx = c->x;
	ocy = c->y;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurResize]->cursor, CurrentTime) != GrabSuccess)
		return;
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / 60))
				continue;
			lasttime = ev.xmotion.time;

			nw = MAX(ev.xmotion.x - ocx - 2 * c->bw + 1, 1);
			nh = MAX(ev.xmotion.y - ocy - 2 * c->bw + 1, 1);
			if (c->mon->wx + nw >= selmon->wx && c->mon->wx + nw <= selmon->wx + selmon->ww
			&& c->mon->wy + nh >= selmon->wy && c->mon->wy + nh <= selmon->wy + selmon->wh)
			{
				if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
				&& (abs(nw - c->w) > snap || abs(nh - c->h) > snap))
					togglefloating(NULL);
			}
			if (!selmon->lt[selmon->sellt]->arrange || c->isfloating)
				resize(c, c->x, c->y, nw, nh, 1);
			break;
		}
	} while (ev.type != ButtonRelease);
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
	XUngrabPointer(dpy, CurrentTime);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
	if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selmon = m;
		focus(NULL);
	}
}

void
resizerequest(XEvent *e)
{
	XResizeRequestEvent *ev = &e->xresizerequest;
	Client *i;

	if ((i = wintosystrayicon(ev->window))) {
		updatesystrayicongeom(i, ev->width, ev->height);
		resizebarwin(selmon);
		updatesystray();
	}
}

void
restack(Monitor *m)
{
	Client *c;
	XEvent ev;
	XWindowChanges wc;

	drawbar(m);
	if (!m->sel)
		return;
	if (m->sel->isfloating || !m->lt[m->sellt]->arrange)
		XRaiseWindow(dpy, m->sel->win);
	if (m->lt[m->sellt]->arrange) {
		wc.stack_mode = Below;
		wc.sibling = m->barwin;
		for (c = m->stack; c; c = c->snext)
			if (!c->isfloating && ISVISIBLE(c)) {
				XConfigureWindow(dpy, c->win, CWSibling|CWStackMode, &wc);
				wc.sibling = c->win;
			}
	}
	XSync(dpy, False);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

void
rotatestack(const Arg *arg)
{
	Client *c = NULL, *f;

	if (!selmon->sel)
		return;
	f = selmon->sel;
	if (arg->i > 0) {
		for (c = nexttiled(selmon->clients); c && nexttiled(c->next); c = nexttiled(c->next));
		if (c){
			detach(c);
			attach(c);
			detachstack(c);
			attachstack(c);
		}
	} else {
		if ((c = nexttiled(selmon->clients))){
			detach(c);
			enqueue(c);
			detachstack(c);
			enqueuestack(c);
		}
	}
	if (c){
		arrange(selmon);
		//unfocus(f, 1);
		focus(f);
		restack(selmon);
	}
}

void
run(void)
{
	XEvent ev;
	/* main event loop */
	XSync(dpy, False);
	while (running && !XNextEvent(dpy, &ev))
		if (handler[ev.type])
			handler[ev.type](&ev); /* call handler */
}

void
runAutostart(void) {
	system("~/.config/dwm/script/init.sh");
}

void
scan(void)
{
	unsigned int i, num;
	Window d1, d2, *wins = NULL;
	XWindowAttributes wa;

	if (XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
		for (i = 0; i < num; i++) {
			if (!XGetWindowAttributes(dpy, wins[i], &wa)
			|| wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1))
				continue;
			if (wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
				manage(wins[i], &wa);
		}
		for (i = 0; i < num; i++) { /* now the transients */
			if (!XGetWindowAttributes(dpy, wins[i], &wa))
				continue;
			if (XGetTransientForHint(dpy, wins[i], &d1)
			&& (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
				manage(wins[i], &wa);
		}
		if (wins)
			XFree(wins);
	}
}

void
sendmon(Client *c, Monitor *m)
{
	if (c->mon == m)
		return;
	unfocus(c, 1);
	detach(c);
	detachstack(c);
	c->mon = m;
	c->tags = m->tagset[m->seltags]; /* assign tags of target monitor */
	attach(c);
	attachstack(c);
	focus(NULL);
	arrange(NULL);
}

void
setclientstate(Client *c, long state)
{
	long data[] = { state, None };

	XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32,
		PropModeReplace, (unsigned char *)data, 2);
}

int
sendevent(Window w, Atom proto, int mask, long d0, long d1, long d2, long d3, long d4)
{
	int n;
	Atom *protocols, mt;
	int exists = 0;
	XEvent ev;

	if (proto == wmatom[WMTakeFocus] || proto == wmatom[WMDelete]) {
		mt = wmatom[WMProtocols];
		if (XGetWMProtocols(dpy, w, &protocols, &n)) {
			while (!exists && n--)
				exists = protocols[n] == proto;
			XFree(protocols);
		}
	}
	else {
		exists = True;
		mt = proto;
    }

	if (exists) {
		ev.type = ClientMessage;
		ev.xclient.window = w;
		ev.xclient.message_type = mt;
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = d0;
		ev.xclient.data.l[1] = d1;
		ev.xclient.data.l[2] = d2;
		ev.xclient.data.l[3] = d3;
		ev.xclient.data.l[4] = d4;
		XSendEvent(dpy, w, False, mask, &ev);
	}
	return exists;
}

void
setfocus(Client *c)
{
	if (!c->neverfocus) {
		XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
		XChangeProperty(dpy, root, netatom[NetActiveWindow],
			XA_WINDOW, 32, PropModeReplace,
			(unsigned char *) &(c->win), 1);
	}
	sendevent(c->win, wmatom[WMTakeFocus], NoEventMask, wmatom[WMTakeFocus], CurrentTime, 0, 0, 0);
	c->isfocused = True;
	lru(c);
}

void
setfullscreen(Client *c, int fullscreen)
{
	if (fullscreen && !c->isfullscreen) {
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)&netatom[NetWMFullscreen], 1);
		c->isfullscreen = 1;
		c->oldstate = c->isfloating;
		c->oldbw = c->bw;
		c->bw = 0;
		c->isfloating = 1;
		resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
		XRaiseWindow(dpy, c->win);
	} else if (!fullscreen && c->isfullscreen){
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)0, 0);
		c->isfullscreen = 0;
		c->isfloating = c->oldstate;
		c->bw = c->oldbw;
		c->x = c->oldx;
		c->y = c->oldy;
		c->w = c->oldw;
		c->h = c->oldh;
		resizeclient(c, c->x, c->y, c->w, c->h);
		arrange(c->mon);
	}
}

void
gap_copy(Gap *to, const Gap *from)
{
	to->isgap   = from->isgap;
	to->realgap = from->realgap;
	to->gappx   = from->gappx;
}

void
setgaps(const Arg *arg)
{
	Gap *p = selmon->gap;
	switch(arg->i)
	{
		case GAP_TOGGLE:
			p->isgap = 1 - p->isgap;
			break;
		case GAP_RESET:
			gap_copy(p, &default_gap);
			break;
		default:
			p->realgap += arg->i;
			p->isgap = 1;
	}
	p->realgap = MAX(p->realgap, 0);
	p->gappx = p->realgap * p->isgap;
	arrange(selmon);
}


void
setlayoutv(const Arg *arg, int isarrange)
{
	unsigned int i;
	if (!arg || !arg->v || arg->v != selmon->lt[selmon->sellt])
		selmon->sellt ^= 1;
	if (arg && arg->v)
		selmon->lt[selmon->sellt] = (Layout *)arg->v;
	strncpy(selmon->ltsymbol, selmon->lt[selmon->sellt]->symbol, sizeof selmon->ltsymbol);

	for(i=0; i<LENGTH(tags); ++i)
		if(selmon->tagset[selmon->seltags] & 1<<i)
		{
			selmon->pertag->ltidxs[i+1][selmon->sellt] = selmon->lt[selmon->sellt]; 
			selmon->pertag->sellts[i+1] = selmon->sellt;
		}
	
	if(selmon->pertag->curtag == 0)
	{
		selmon->pertag->ltidxs[0][selmon->sellt] = selmon->lt[selmon->sellt]; 
		selmon->pertag->sellts[0] = selmon->sellt;
	}

	if(isarrange){
		if (selmon->sel)
			arrange(selmon);
		else
			drawbar(selmon);
	}
}

void
setlayout(const Arg *arg)
{
	unsigned int i;
	if (!arg || !arg->v || arg->v != selmon->lt[selmon->sellt])
		selmon->sellt ^= 1;
	if (arg && arg->v)
		selmon->lt[selmon->sellt] = (Layout *)arg->v;
	strncpy(selmon->ltsymbol, selmon->lt[selmon->sellt]->symbol, sizeof selmon->ltsymbol);

	for(i=0; i<LENGTH(tags); ++i)
		if(selmon->tagset[selmon->seltags] & 1<<i)
		{
			selmon->pertag->ltidxs[i+1][selmon->sellt] = selmon->lt[selmon->sellt]; 
			selmon->pertag->sellts[i+1] = selmon->sellt;
		}
	
	if(selmon->pertag->curtag == 0)
	{
		selmon->pertag->ltidxs[0][selmon->sellt] = selmon->lt[selmon->sellt]; 
		selmon->pertag->sellts[0] = selmon->sellt;
	}

	if (selmon->sel)
		arrange(selmon);
	else
		drawbar(selmon);
}

/* arg > 1.0 will set mfact absolutely */
void
setmfact(const Arg *arg)
{
	float f;
	unsigned int i;

	if (!arg || !selmon->lt[selmon->sellt]->arrange)
		return;
	f = arg->f < 1.0 ? arg->f + selmon->mfact : arg->f - 1.0;
	if (arg->f == 0.0)
		f = mfact;
	if (f < 0.05 || f > 0.95)
		return;
	selmon->mfact = f;
	for(i=0; i<LENGTH(tags); ++i)
		if(selmon->tagset[selmon->seltags] & 1<<i)
			selmon->pertag->mfacts[i+1] = f;

	if(selmon->pertag->curtag == 0)
	{
		selmon->pertag->mfacts[0] = f;
	}
	arrange(selmon);
}

void
setup(void)
{
	FC_HEAD = (Client*)malloc(sizeof(Client));
	memset(FC_HEAD, 0, sizeof(FC_HEAD));
	focuschain = FC_HEAD;

	int tagi;
	for(tagi = 0; tagi < LENGTH(tags)+1; tagi++){
		Tag *tag = (Tag*)malloc(sizeof(Tag));
		tagarray[tagi] = tag;
		memset(tag, 0, sizeof(Tag));
	}
	HEADTAG = (Tag*)malloc(sizeof(Tag));
	memset(HEADTAG, 0, sizeof(Tag));
	TAILTAG = (Tag*)malloc(sizeof(Tag));
	memset(TAILTAG, 0, sizeof(Tag));
	HEADTAG->prev = TAILTAG;
	TAILTAG->next = HEADTAG;

	int taggi;
	for(taggi = 0; taggi < LENGTH(tags)+1; taggi++){
		int taggj;
		for(taggj = 0; taggj < LENGTH(tags)+1; taggj++){
			TagStat *edge = (TagStat*)malloc(sizeof(TagStat));
			taggraph[taggi][taggj] = edge;
			memset(edge, 0, sizeof(TagStat));
		}
	}
	

	int i;
	XSetWindowAttributes wa;
	Atom utf8string;

	/* clean up any zombies immediately */
	sigchld(0);

	signal(SIGHUP, sighup);
	signal(SIGTERM, sigterm);

	/* init screen */
	screen = DefaultScreen(dpy);
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);
	root = RootWindow(dpy, screen);
	drw = drw_create(dpy, screen, root, sw, sh);
	if (!drw_fontset_create(drw, fonts, LENGTH(fonts)))
		die("no fonts could be loaded.");
	lrpad = drw->fonts->h;
	bh = drw->fonts->h + 2;
	updategeom();
	/* init atoms */
	utf8string = XInternAtom(dpy, "UTF8_STRING", False);
	wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
	wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
	netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
   netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
	netatom[NetSystemTray] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_S0", False);
	netatom[NetSystemTrayOP] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
	netatom[NetSystemTrayOrientation] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION", False);
	netatom[NetSystemTrayOrientationHorz] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION_HORZ", False);
    netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
	netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
	netatom[NetWMCheck] = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
	netatom[NetWMFullscreen] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	netatom[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
	netatom[NetWMWindowTypeDialog] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
	netatom[NetClientList] = XInternAtom(dpy, "_NET_CLIENT_LIST", False);

	netatom[NetWmStateSkipTaskbar] = XInternAtom(dpy, "_NET_WM_STATE_SKIP_TASKBAR", False);
	
	
	//EWMHTAGS
	netatom[NetDesktopViewport] = XInternAtom(dpy, "_NET_DESKTOP_VIEWPORT", False);
	netatom[NetNumberOfDesktops] = XInternAtom(dpy, "_NET_NUMBER_OF_DESKTOPS", False);
	netatom[NetCurrentDesktop] = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);
	netatom[NetDesktopNames] = XInternAtom(dpy, "_NET_DESKTOP_NAMES", False);

	netatom[NetWmPid] = XInternAtom(dpy, "_NET_WM_PID", False);

	xatom[Manager] = XInternAtom(dpy, "MANAGER", False);
	xatom[Xembed] = XInternAtom(dpy, "_XEMBED", False);
	xatom[XembedInfo] = XInternAtom(dpy, "_XEMBED_INFO", False);
    /* init cursors */
	cursor[CurNormal] = drw_cur_create(drw, XC_left_ptr);
	cursor[CurResize] = drw_cur_create(drw, XC_sizing);
	cursor[CurMove] = drw_cur_create(drw, XC_fleur);
	/* init appearance */
	scheme = ecalloc(LENGTH(colors), sizeof(Clr *));
	for (i = 0; i < LENGTH(colors); i++)
		scheme[i] = drw_scm_create(drw, colors[i], 3);
	/* init system tray */
	updatesystray();
	/* init bars */
	updatebars();
	updatestatus();
	/* supporting window for NetWMCheck */
	wmcheckwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], utf8string, 8,
		PropModeReplace, (unsigned char *) "dwm", 3);
	XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	/* EWMH support per view */
	XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
		PropModeReplace, (unsigned char *) netatom, NetLast);
	setnumdesktops();
	setcurrentdesktop();
	setdesktopnames();
	setviewport();
	XDeleteProperty(dpy, root, netatom[NetClientList]);
	/* select events */
	wa.cursor = cursor[CurNormal]->cursor;
	wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask
		|ButtonPressMask|PointerMotionMask|EnterWindowMask
		|LeaveWindowMask|StructureNotifyMask|PropertyChangeMask;
	XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);
	XSelectInput(dpy, root, wa.event_mask);
	grabkeys();
	focus(NULL);

	scratchgroupptr = (ScratchGroup *)malloc(sizeof(ScratchGroup));
	memset(scratchgroupptr, 0, sizeof(ScratchGroup));
	scratchgroupptr->head = alloc_si();
	scratchgroupptr->tail = alloc_si();
	scratchgroupptr->head->next = scratchgroupptr->tail;
	scratchgroupptr->tail->prev = scratchgroupptr->head;
}


void
seturgent(Client *c, int urg)
{
	XWMHints *wmh;

	c->isurgent = urg;
	if (!(wmh = XGetWMHints(dpy, c->win)))
		return;
	wmh->flags = urg ? (wmh->flags | XUrgencyHint) : (wmh->flags & ~XUrgencyHint);
	XSetWMHints(dpy, c->win, wmh);
	XFree(wmh);
}

void
showhide(Client *c)
{
	if (!c)
		return;
	if (ISVISIBLE(c)) {
		/* show clients top down */
		XMoveWindow(dpy, c->win, c->x, c->y);
		if ((!c->mon->lt[c->mon->sellt]->arrange || c->isfloating) && !c->isfullscreen)
			resize(c, c->x, c->y, c->w, c->h, 0);
		showhide(c->snext);
	} else {
		/* hide clients bottom up */
		showhide(c->snext);
		XMoveWindow(dpy, c->win, WIDTH(c) * -2, c->y);
	}
}

void
sigchld(int unused)
{
	if (signal(SIGCHLD, sigchld) == SIG_ERR)
		die("can't install SIGCHLD handler:");
	while (0 < waitpid(-1, NULL, WNOHANG));
}

void
sighup(int unused)
{
	Arg a = {.i = 1};
	quit(&a);
}

void
sigterm(int unused)
{
	Arg a = {.i = 0};
	quit(&a);
}

void
sigstatusbar(const Arg *arg)
{
	union sigval sv;

	if (!statussig)
		return;
	sv.sival_int = arg->i;
	if ((statuspid = getstatusbarpid()) <= 0)
		return;

	sigqueue(statuspid, SIGRTMIN+statussig, sv);
}

void
spawn(const Arg *arg)
{
	if (arg->v == dmenucmd)
		dmenumon[0] = '0' + selmon->num;
	selmon->tagset[selmon->seltags] &= ~scratchtag;
	if (fork() == 0) {
		if (dpy)
			close(ConnectionNumber(dpy));
		setsid();
		execvp(((char **)arg->v)[0], (char **)arg->v);
		fprintf(stderr, "dwm: execvp %s", ((char **)arg->v)[0]);
		perror(" failed");
		exit(EXIT_SUCCESS);
	}
}

void
sspawn(const Arg *arg)
{
	if (!scratchgroupptr->isfloating && scratchsingle(arg->v))
	{
		showscratchgroup(scratchgroupptr);
		return;
	}
	spawn(arg);
	isnextscratch = 1;
	nextscratchcmd = arg->v;
}

void
tag(const Arg *arg)
{
	if (selmon->sel && arg->ui & TAGMASK) {
		selmon->sel->tags = arg->ui & TAGMASK;
		focus(NULL);
		arrange(selmon);
		if(viewontag && ((arg->ui & TAGMASK) != TAGMASK))
			view(arg);
	}
}

void
tagmon(const Arg *arg)
{
	if (!selmon->sel || !mons->next)
		return;
	sendmon(selmon->sel, dirtomon(arg->i));
}

void
tile(Monitor *m)
{
	unsigned int i, n, h, mw, my, ty;
	Client *c;

	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
	if (n == 0)
		return;

	if (n > m->nmaster)
		mw = m->nmaster ? m->ww * m->mfact : 0;
	else
		mw = m->ww - m->gap->gappx;
	for (i = 0, my = ty = m->gap->gappx, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
		if (i < m->nmaster) {
			h = (m->wh - my) / (MIN(n, m->nmaster) - i) - m->gap->gappx;
			resize(c, m->wx + m->gap->gappx, m->wy + my, mw - (2*c->bw) - m->gap->gappx, h - (2*c->bw), 0);
			if (my + HEIGHT(c) + m->gap->gappx < m->wh)
				my += HEIGHT(c) + m->gap->gappx;
		} else {
			h = (m->wh - ty) / (n - i) - m->gap->gappx;
			resize(c, m->wx + mw + m->gap->gappx, m->wy + ty, m->ww - mw - (2*c->bw) - 2*m->gap->gappx, h - (2*c->bw), 0);
			if (ty + HEIGHT(c) + m->gap->gappx < m->wh)
				ty += HEIGHT(c) + m->gap->gappx;
		}
}


void
geo( geo_t *g, Client *c, int x, int y, int w, int h, int interact)
{
	g->c = c;
	g->x = x;
	g->y = y;
	g->w = w;
	g->h = h;
	g->interact =interact;
}

// expand focused client
void
tile2(Monitor *m)
{
	unsigned int i, n, h, mw, my, ty, masterend = 0;
	Client *c;

	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
	if (n == 0)
		return;

	if (n > m->nmaster)
		mw = m->nmaster ? m->ww * m->mfact : 0;
	else
		mw = m->ww - m->gap->gappx;


	int focused_slave_index = -1;
	geo_t gmap[n];
	for (i = 0, my = ty = m->gap->gappx, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
	{	
		memset(gmap+i, 0, sizeof(geo_t));
		if (i < m->nmaster) {
			h = (m->wh - my) / (MIN(n, m->nmaster) - i) - m->gap->gappx;
			geo(gmap+i,c, m->wx + m->gap->gappx, m->wy + my, mw - (2*c->bw) - m->gap->gappx, h - (2*c->bw), 0);
			geo_t g = gmap[i];
			if (my + (g.h + 2*c->bw) + m->gap->gappx < m->wh)
				my += (g.h + 2*c->bw) + m->gap->gappx;
			masterend = i;
		} else {
			if(c->isfocused){
				focused_slave_index = i;
			}
			h = (m->wh - ty) / (n - i) - m->gap->gappx;
			geo(gmap+i,c, m->wx + mw + m->gap->gappx, m->wy + ty, m->ww - mw - (2*c->bw) - 2*m->gap->gappx, h - (2*c->bw), 0);
			geo_t g = gmap[i];
			if (ty + (g.h + 2*c->bw) + m->gap->gappx < m->wh)
				ty += (g.h + 2*c->bw) + m->gap->gappx;
		}
	}
	
	int slave_cnt = n - masterend - 1;
	int focused_slave_h = m->wh -  m->gap->gappx;
	if(slave_cnt > 0) focused_slave_h = (m->wh - m->gap->gappx) / slave_cnt ;
	if(focused_slave_index > 0 && slave_cnt > 1) focused_slave_h = m->wh - m->gap->gappx - 200;
	int unfocused_slave_h = m->wh -  m->gap->gappx;
	if(slave_cnt > 1) unfocused_slave_h = (m->wh- m->gap->gappx - focused_slave_h ) / (slave_cnt -1) ;
	if(slave_cnt == 1) unfocused_slave_h = m->wh - m->gap->gappx;

	if (i == 1)
	{
		geo_t onlyg = gmap[0];
		onlyg.c->bw = 0;
		resize(onlyg.c, onlyg.c->mon->wx, onlyg.c->mon->wy, onlyg.c->mon->ww, onlyg.c->mon->wh, onlyg.interact);
		onlyg.c->bw = onlyg.c->oldbw;
		return;
	}

	// int focused_slave_w = m->ww * m->mfact;
	int focused_slave_w = m->ww / 2; 
	int currenty = 0;
	for (i = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
	{
		geo_t g = gmap[i];
		if(i <= masterend){
			resize(g.c, g.x, g.y, g.w, g.h, g.interact);
			continue;
		}
		int x;
		int y;
		int w;
		int h;
		if(i == focused_slave_index){
			x = g.x + g.w - focused_slave_w;
			y = currenty + m->gap->gappx;
			w = focused_slave_w;
			h = focused_slave_h;
		}else{
			x = g.x;
			y = currenty + m->gap->gappx;
			w = g.w;
			h = unfocused_slave_h;
		}
		resize(g.c, x, y, w, h - m->gap->gappx, g.interact);
		currenty += h;
	}
	
}

void
togglebar(const Arg *arg)
{
	unsigned int i;
	selmon->showbar = !selmon->showbar;
	for(i=0; i<LENGTH(tags); ++i)
		if(selmon->tagset[selmon->seltags] & 1<<i)
			selmon->pertag->showbars[i+1] = selmon->showbar;

	if(selmon->pertag->curtag == 0)
	{
		selmon->pertag->showbars[0] = selmon->showbar;
	}
	updatebarpos(selmon);
	resizebarwin(selmon);
	if (showsystray) {
		XWindowChanges wc;
		if (!selmon->showbar)
			wc.y = -bh;
		else if (selmon->showbar) {
			wc.y = 0;
			if (!selmon->topbar)
				wc.y = selmon->mh - bh;
		}
		XConfigureWindow(dpy, systray->win, CWY, &wc);
	}
	arrange(selmon);
}

void
togglefloating(const Arg *arg)
{
	if (!selmon->sel)
		return;
	if (selmon->sel->isfullscreen) /* no support for fullscreen windows */
		return;
	selmon->sel->isfloating = !selmon->sel->isfloating || selmon->sel->isfixed;
	if (selmon->sel->isfloating)
		resize(selmon->sel, selmon->sel->x, selmon->sel->y,
			selmon->sel->w, selmon->sel->h, 0);
	arrange(selmon);
}

void
togglescratch(const Arg *arg)
{
	Client *c;
	unsigned int found = 0;

	for (c = selmon->clients; c && !(found = c->tags & scratchtag); c = c->next);
	if (found) {
		unsigned int newtagset = selmon->tagset[selmon->seltags] ^ scratchtag;
		if (newtagset) {
			selmon->tagset[selmon->seltags] = newtagset;
			focus(NULL);
			arrange(selmon);
		}
		if (ISVISIBLE(c)) {
			focus(c);
			restack(selmon);
		}
	} else
		spawn(arg);
}

ScratchItem *
alloc_si()
{
	ScratchItem * si = (ScratchItem *)malloc(sizeof(ScratchItem));
	memset(si,0,sizeof(ScratchItem));
	return si;
}

void 
free_si(ScratchItem *si)
{
	if(si) free(si);
}

ScratchItem*
findingroup(ScratchGroup *scratchgroupptr, Client *c)
{
	ScratchItem * tmp = NULL;
	for( tmp = scratchgroupptr->head->next;tmp && tmp != scratchgroupptr->tail;tmp = tmp->next)
		if(tmp->c == c) return tmp;
	return NULL;
}


void
showscratchgroup(ScratchGroup *sg)
{
	ScratchItem *si;
	int tsn;
	for(tsn = 0, si = sg->head->next; si && si != sg->tail; si = si->next, tsn++);
	if(tsn == 0) return;
	rect_t ts[tsn];
	memset(ts, 0, sizeof(ts));
	rect_t sc;
	// sc.x = selmon->gap->gappx + 10;
	// sc.y = selmon->gap->gappx + 10;
	sc.x = 0;
	sc.y = 0;
	sc.w = selmon->ww;
	sc.h = selmon->wh;
	int i = 0;
	for(si = sg->tail->prev; si && si != sg->head; si = si->prev)
	{
		Client * c = si->c;
		if(!c) continue;

		c->isfloating = 1;
		if(!sg->isfloating) si->pretags = c->tags;
		c->tags = 0xFFFFFFFF;
		int neww = selmon->ww * 0.45;
		int newh = selmon->wh * 0.6;
		// int neww = selmon->ww * 0.3;
		// int newh = selmon->wh * 0.3;
		
		rect_t r;
		int radiostepn = 4;
		double maxintersectradiostep[] = {0.0, 0.3, 0.6, 0.8};
		int ok = 0;
		int radioi;
		for(radioi = 0;radioi<radiostepn;radioi++){
			ok = fill2(sc, neww, newh, 9, ts, i, &r, maxintersectradiostep[radioi]);
			if(ok) break;
		}
		if(!ok)
		{
			r.x = selmon->ww / 2 - neww / 2;
			r.y = selmon->wh / 2 - newh / 2;
			r.w = neww;
			r.h = newh;
		}

		if(!si->placed) {
			si->x = r.x;
			si->y = r.y;
			si->w = r.w;
			si->h = r.h;
		}

		c->x = si->x;
		c->y = si->y;
		
		ts[i].x = si->x;
		ts[i].y = si->y;
		ts[i].w = si->w;
		ts[i].h = si->h;

		i++;
	} 
	
	for(si = sg->head->next; si && si != sg->tail; si = si->next)
	{
		focus(si->c);
		arrange(selmon);
		resize(si->c,si->x,si->y, si->w,si->h,1);
	}
	if(sg->head->next && sg->head->next->c) 
		focus(sg->head->next->c);
	arrange(selmon);
	sg->isfloating = 1;
}

void
hidescratchitem(ScratchItem *si)
{
	Client * c = si->c;
	if(!c) return;
	c->isfloating = 0;
	if(si->pretags)
		c->tags = si->pretags;
	else
		c->tags = selmon->tagset[selmon->seltags];
	si->x = c->x;
	si->y = c->y;
	si->w = c->w;
	si->h = c->h;
	si->placed = 1;
	focus(NULL);
	arrange(selmon);
}

void 
hidescratchgroupv(ScratchGroup *sg, int isarrange)
{
	ScratchItem *si;
	for (si = sg->tail->prev; si && si != sg->head; si = si->prev)
	{
		Client *c = si->c;
		if (!c)
			continue;
		c->isfloating = 0;
		if (si->pretags)
			c->tags = si->pretags;
		else
			c->tags = selmon->tagset[selmon->seltags];
		si->x = c->x;
		si->y = c->y;
		si->w = c->w;
		si->h = c->h;
		si->placed = 1;
	}
	if(isarrange){
		focus(NULL);
		arrange(selmon);
	}
	sg->isfloating = 0;
}

void 
hidescratchgroup(ScratchGroup *sg)
{
	hidescratchgroupv(sg, 1);
}

ScratchItem*
addtoscratchgroupc(Client *c)
{
	if (!c) return;
	ScratchItem* scratchitemptr;
	scratchitemptr = findingroup(scratchgroupptr, c);
	if(!scratchitemptr){
		scratchitemptr = alloc_si();
		scratchitemptr->c = c;
		scratchitemptr->prev = scratchgroupptr->head;
		scratchitemptr->next = scratchgroupptr->head->next;
		scratchgroupptr->head->next->prev = scratchitemptr;
		scratchgroupptr->head->next = scratchitemptr;
		c->isscratched = 1;
		c->bw = borderpx;
		XWindowChanges wc;
		wc.border_width = c->bw;
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc);
		XSetWindowBorder(dpy, c->win, scheme[SchemeScr][ColBorder].pixel);
	}
	showscratchgroup(scratchgroupptr);
	return scratchitemptr;
}

void 
addtoscratchgroup(const Arg *arg)
{
	Client * c = selmon->sel ;
	addtoscratchgroupc(c);
}

void 
removefromscratchgroupc(Client *c)
{
	ScratchItem *found = findingroup(scratchgroupptr, c);
	if (found){
		c->isscratched = 0;
		XSetWindowBorder(dpy, c->win, scheme[SchemeSel][ColBorder].pixel);
		found->prev->next = found->next;
		found->next->prev = found->prev;
		found->prev = NULL;
		found->next = NULL;
		hidescratchitem(found);
		free_si(found);
	}
}


void 
removefromscratchgroup(const Arg *arg)
{
	Client * c = selmon->sel;
	removefromscratchgroupc(c);
	if(scratchgroupptr->head->next == scratchgroupptr->tail)
		hidescratchgroup(scratchgroupptr);
}

int
ispointin(int x, int y, rect_t t)
{
	if(x >= t.x && x<=(t.x+t.w) && y >= t.y && y <= (t.y+t.h)) return 1;
	return 0;
}

int
isintersectone(rect_t g, rect_t t)
{
	if(ispointin(g.x,g.y,t) 
	|| ispointin(g.x+g.w,g.y, t) 
	|| ispointin(g.x+g.w,g.y+g.h, t)
	|| ispointin(g.x, g.y+g.h, t)
	|| ispointin(t.x,t.y,g) 
	|| ispointin(t.x+t.w,t.y, g) 
	|| ispointin(t.x+t.w,t.y+t.h, g)
	|| ispointin(t.x, t.y+t.h, g)
	) return 1;
	return 0;
}

#define SETINPOINT(x,y,t, tx,ty)  if(ispointin(x,y,t)){tx=x;ty=y;}

#define RETURNINTERSECT(i,j,x1,y1,x2,y2, g,t) if(x1[i]>-1 && y1[i]>-1 && x2[j]>-1 && y2[j]>-1) return 1.0*(abs(x2[j]-x1[i])*abs(y2[j]-y1[i]))/MIN((abs(g.w)*abs(g.h)),(abs(t.w)*abs(t.h)));


double
intersectpercent(rect_t g, rect_t t)
{
	int x1[4] = {-1,-1,-1,-1};
	int y1[4] = {-1,-1,-1,-1};
	int x2[4] = {-1,-1,-1,-1};
	int y2[4] = {-1,-1,-1,-1};

	SETINPOINT(g.x, g.y, t, x1[0], y1[0])
	SETINPOINT(g.x+g.w,g.y, t, x1[1], y1[1])
	SETINPOINT(g.x+g.w,g.y+g.h, t ,x1[2], y1[2])
	SETINPOINT(g.x, g.y+g.h, t ,x1[3], y1[3])
	
	SETINPOINT(t.x,t.y,g,x2[0],y2[0]) 
	SETINPOINT(t.x+t.w,t.y, g,x2[1],y2[1]) 
	SETINPOINT(t.x+t.w,t.y+t.h, g,x2[2],y2[2]) 
	SETINPOINT(t.x, t.y+t.h, g,x2[3],y2[3]) 
	
	RETURNINTERSECT(0,2,x1, y1, x2,y2,g,t)
	RETURNINTERSECT(2,0,x1, y1, x2,y2,g,t)
	RETURNINTERSECT(1,3,x1, y1, x2,y2,g,t)
	RETURNINTERSECT(3,1,x1, y1, x2,y2,g,t)
	return 0;
}

int 
isintersect(rect_t g, rect_t ts[], int tsn)
{
	int i;
	for(i = 0; i<tsn; i++)
	{
		rect_t t = ts[i];
		if(intersectpercent(g,t) > 0) return 1;
	}
	return 0;
}

int 
isintersectp(rect_t g, rect_t ts[], int tsn, double maxintersectradio)
{
	// LOG_FORMAT("\n\ntsn:%d\n", tsn);
	int i;
	for(i = 0; i<tsn; i++)
	{
		rect_t t = ts[i];
		// LOG_FORMAT("i:%d:intersectpercent:%f,maxintersectradio:%f\n",i,intersectpercent(g,t), maxintersectradio);
		// LOG_FORMAT("g(%d,%d,%d,%d), t(%d,%d,%d,%d)", g.x, g.y, g.w, g.h, t.x, t.y, t.w, t.h);
		if(intersectpercent(g,t) > maxintersectradio) return 1;
	}
	return 0;
}

int
fill(rect_t sc, int w, int h, int n, rect_t ts[], int tsn, rect_t *r)
{
	// todo: center iterator
	// LOG_FORMAT("tsn:%d, w:%d, h:%d", tsn, w, h);
	int stepw = (sc.w - w) /n;
	int steph = (sc.h - h) /n;
	int i;
	int j;
	for(i = 0; i<n;i++)
	{
		for(j=0;j<n;j++)
		{
			rect_t rect;
			rect.x = sc.x + stepw * i;
			rect.y = sc.y + steph * j;
			rect.w = w;
			rect.h = h;
			if(!isintersect(rect, ts, tsn)){
				r->x = rect.x;
				r->y = rect.y;
				r->w = rect.w;
				r->h = rect.h;
				// LOG_FORMAT(", fillr:%d", 1);
				// LOG_FORMAT(", fx:%d, fy:%d, fw:%d, fh:%d\n", r->x, r->y, r->w, r->h);
				return 1;
			}
		}
	}
	return 0;
}

int 
tryfillone(int x, int y, int w, int h, rect_t ts[], int tsn, rect_t *r,double maxintersectradio)
{
	rect_t rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
	if(!isintersectp(rect, ts, tsn, maxintersectradio)){
		r->x = rect.x;
		r->y = rect.y;
		r->w = rect.w;
		r->h = rect.h;
		return 1;
	}
	return 0;
}

int
calcx(rect_t sc, int i, int stepw, int w)
{
	return sc.x + i*stepw ;
}


int
calcy(rect_t sc, int j, int steph, int h)
{
	return sc.y + j*steph ;
}

int
fill2(rect_t sc, int w, int h, int n, rect_t ts[], int tsn, rect_t *r, double maxintersectradio)
{
	// todo: center iterator
	// LOG_FORMAT("tsn:%d, w:%d, h:%d", tsn, w, h);
	int stepw = (sc.w - w) /(n-1);
	int steph = (sc.h - h) /(n-1);
	int centeri = n/2;
	int centerj = n/2;
	int k;
	for(k = 0; k<(n/2+1);k++)
	{
		int i;
		int j;
		if(k == 0)
			if(tryfillone(calcx(sc,centeri,stepw, w), calcy(sc, centerj, steph, h), w, h, ts, tsn, r, maxintersectradio)) return 1;

		if(tryfillone(calcx(sc,centeri+k,stepw, w), calcy(sc, centerj+k, steph, h), w, h, ts, tsn, r, maxintersectradio)) return 1;
		if(tryfillone(calcx(sc,centeri-k,stepw, w), calcy(sc, centerj-k, steph, h), w, h, ts, tsn, r, maxintersectradio)) return 1;
		if(tryfillone(calcx(sc,centeri-k,stepw, w), calcy(sc, centerj+k, steph, h), w, h, ts, tsn, r, maxintersectradio)) return 1;
		if(tryfillone(calcx(sc,centeri+k,stepw, w), calcy(sc, centerj-k, steph, h), w, h, ts, tsn, r, maxintersectradio)) return 1;

		i = centeri - k;
		for(j=centerj - k;j<centerj+k;j++)
		{
			if(i>=n || i <0 || j>=n || j <0 ) continue;
			if(tryfillone(calcx(sc,i,stepw, w), calcy(sc, j, steph, h), w, h, ts, tsn, r, maxintersectradio)) return 1;
		}

		j = centerj + k;
		for(i=centeri - k;i<centeri+k;i++)
		{
			if(i>=n || i <0 || j>=n || j <0 ) continue;
			if(tryfillone(calcx(sc,i,stepw, w), calcy(sc, j, steph, h), w, h, ts, tsn, r, maxintersectradio)) return 1;
		}

		i = centeri + k;
		for(j=centerj + k;j>centerj-k;j--)
		{
			if(i>=n || i <0 || j>=n || j <0 ) continue;
			if(tryfillone(calcx(sc,i,stepw, w), calcy(sc, j, steph, h), w, h, ts, tsn, r, maxintersectradio)) return 1;
		}
		
		j = centerj - k;
		for(i=centeri + k;i>centeri-k;i--)
		{
			if(i>=n || i <0 || j>=n || j <0 ) continue;
			if(tryfillone(calcx(sc,i,stepw, w), calcy(sc, j, steph, h), w, h, ts, tsn, r, maxintersectradio)) return 1;
		}
	}
	
	return 0;
}


void
togglescratchgroup(const Arg *arg)
{
	ScratchGroup *sg = scratchgroupptr;
	if(!sg->isfloating)
	{
		showscratchgroup(sg);
	}else{
		hidescratchgroup(sg);
	}
}

void
toggletag(const Arg *arg)
{
	unsigned int newtags;

	if (!selmon->sel)
		return;
	newtags = selmon->sel->tags ^ (arg->ui & TAGMASK);
	if (newtags) {
		selmon->sel->tags = newtags;
		focus(NULL);
		arrange(selmon);
	}
	updatecurrentdesktop();
}

void
toggleview(const Arg *arg)
{
	unsigned int newtagset = selmon->tagset[selmon->seltags] ^ (arg->ui & TAGMASK);
	int i;

	if (newtagset) {
		selmon->tagset[selmon->seltags] = newtagset;

		if (newtagset == ~0) {
			selmon->pertag->prevtag = selmon->pertag->curtag;
			selmon->pertag->curtag = 0;
		}

		/* test if the user did not select the same tag */
		if (!(newtagset & 1 << (selmon->pertag->curtag - 1))) {
			selmon->pertag->prevtag = selmon->pertag->curtag;
			for (i = 0; !(newtagset & 1 << i); i++) ;
			selmon->pertag->curtag = i + 1;
		}

		/* apply settings for this view */
		selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag];
		selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag];
		selmon->sellt = selmon->pertag->sellts[selmon->pertag->curtag];
		selmon->lt[selmon->sellt] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt];
		selmon->lt[selmon->sellt^1] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt^1];

		if (selmon->showbar != selmon->pertag->showbars[selmon->pertag->curtag])
			togglebar(NULL);

		focus(NULL);
		arrange(selmon);
	}
	updatecurrentdesktop();
}

void
unfocus(Client *c, int setfocus)
{
	if (!c)
		return;
	grabbuttons(c, 0);
	XSetWindowBorder(dpy, c->win, scheme[SchemeNorm][ColBorder].pixel);
	if (setfocus) {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
	c->isfocused = False;
}

void
unmanage(Client *c, int destroyed)
{
	removefromscratchgroupc(c);
	removefromfocuschain(c);
	Monitor *m = c->mon;
	XWindowChanges wc;

	detach(c);
	detachstack(c);
	if (!destroyed) {
		wc.border_width = c->oldbw;
		XGrabServer(dpy); /* avoid race conditions */
		XSetErrorHandler(xerrordummy);
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc); /* restore border */
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		setclientstate(c, WithdrawnState);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
	free(c);
	focus(NULL);
	updateclientlist();
	arrange(m);

	if (countcurtag(m->clients) == 0)
	{
		int last = m->tagset[m->seltags] >> 1;
		if(last){
			Arg arg = {.ui= last};
			view(&arg);
		}
	}
	
}

void
unmapnotify(XEvent *e)
{
	Client *c;
	XUnmapEvent *ev = &e->xunmap;

	if ((c = wintoclient(ev->window))) {
		if (ev->send_event)
			setclientstate(c, WithdrawnState);
		else
			unmanage(c, 0);
	}
	else if ((c = wintosystrayicon(ev->window))) {
		/* KLUDGE! sometimes icons occasionally unmap their windows, but do
		 * _not_ destroy them. We map those windows back */
		XMapRaised(dpy, c->win);
		updatesystray();
	}
}

void
updatebars(void)
{
	unsigned int w;
	Monitor *m;
	XSetWindowAttributes wa = {
		.override_redirect = True,
		.background_pixmap = ParentRelative,
		.event_mask = ButtonPressMask|ExposureMask
	};
	XClassHint ch = {"dwm", "dwm"};
	for (m = mons; m; m = m->next) {
		if (m->barwin)
			continue;
		w = m->ww;
		if (showsystray && m == systraytomon(m))
			w -= getsystraywidth();
		m->barwin = XCreateWindow(dpy, root, m->wx, m->by, w, bh, 0, DefaultDepth(dpy, screen),
				CopyFromParent, DefaultVisual(dpy, screen),
				CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
		XDefineCursor(dpy, m->barwin, cursor[CurNormal]->cursor);
		if (showsystray && m == systraytomon(m))
			XMapRaised(dpy, systray->win);
		XMapRaised(dpy, m->barwin);
		XSetClassHint(dpy, m->barwin, &ch);
	}
}

void
updatebarpos(Monitor *m)
{
	m->wy = m->my;
	m->wh = m->mh;
	if (m->showbar) {
		m->wh -= bh;
		m->by = m->topbar ? m->wy : m->wy + m->wh;
		m->wy = m->topbar ? m->wy + bh : m->wy;
	} else
		m->by = -bh;
}

void
updateclientlist()
{
	Client *c;
	Monitor *m;

	XDeleteProperty(dpy, root, netatom[NetClientList]);
	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next)
			XChangeProperty(dpy, root, netatom[NetClientList],
				XA_WINDOW, 32, PropModeAppend,
				(unsigned char *) &(c->win), 1);
}

int
updategeom(void)
{
	int dirty = 0;

#ifdef XINERAMA
	if (XineramaIsActive(dpy)) {
		int i, j, n, nn;
		Client *c;
		Monitor *m;
		XineramaScreenInfo *info = XineramaQueryScreens(dpy, &nn);
		XineramaScreenInfo *unique = NULL;

		for (n = 0, m = mons; m; m = m->next, n++);
		/* only consider unique geometries as separate screens */
		unique = ecalloc(nn, sizeof(XineramaScreenInfo));
		for (i = 0, j = 0; i < nn; i++)
			if (isuniquegeom(unique, j, &info[i]))
				memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
		XFree(info);
		nn = j;
		if (n <= nn) { /* new monitors available */
			for (i = 0; i < (nn - n); i++) {
				for (m = mons; m && m->next; m = m->next);
				if (m)
					m->next = createmon();
				else
					mons = createmon();
			}
			for (i = 0, m = mons; i < nn && m; m = m->next, i++)
				if (i >= n
				|| unique[i].x_org != m->mx || unique[i].y_org != m->my
				|| unique[i].width != m->mw || unique[i].height != m->mh)
				{
					dirty = 1;
					m->num = i;
					m->mx = m->wx = unique[i].x_org;
					m->my = m->wy = unique[i].y_org;
					m->mw = m->ww = unique[i].width;
					m->mh = m->wh = unique[i].height;
					updatebarpos(m);
				}
		} else { /* less monitors available nn < n */
			for (i = nn; i < n; i++) {
				for (m = mons; m && m->next; m = m->next);
				while ((c = m->clients)) {
					dirty = 1;
					m->clients = c->next;
					detachstack(c);
					c->mon = mons;
					attach(c);
					attachstack(c);
				}
				if (m == selmon)
					selmon = mons;
				cleanupmon(m);
			}
		}
		free(unique);
	} else
#endif /* XINERAMA */
	{ /* default monitor setup */
		if (!mons)
			mons = createmon();
		if (mons->mw != sw || mons->mh != sh) {
			dirty = 1;
			mons->mw = mons->ww = sw;
			mons->mh = mons->wh = sh;
			updatebarpos(mons);
		}
	}
	if (dirty) {
		selmon = mons;
		selmon = wintomon(root);
	}
	return dirty;
}

void
updatenumlockmask(void)
{
	unsigned int i, j;
	XModifierKeymap *modmap;

	numlockmask = 0;
	modmap = XGetModifierMapping(dpy);
	for (i = 0; i < 8; i++)
		for (j = 0; j < modmap->max_keypermod; j++)
			if (modmap->modifiermap[i * modmap->max_keypermod + j]
				== XKeysymToKeycode(dpy, XK_Num_Lock))
				numlockmask = (1 << i);
	XFreeModifiermap(modmap);
}

void
updatesizehints(Client *c)
{
	long msize;
	XSizeHints size;

	if (!XGetWMNormalHints(dpy, c->win, &size, &msize))
		/* size is uninitialized, ensure that size.flags aren't used */
		size.flags = PSize;
	if (size.flags & PBaseSize) {
		c->basew = size.base_width;
		c->baseh = size.base_height;
	} else if (size.flags & PMinSize) {
		c->basew = size.min_width;
		c->baseh = size.min_height;
	} else
		c->basew = c->baseh = 0;
	if (size.flags & PResizeInc) {
		c->incw = size.width_inc;
		c->inch = size.height_inc;
	} else
		c->incw = c->inch = 0;
	if (size.flags & PMaxSize) {
		c->maxw = size.max_width;
		c->maxh = size.max_height;
	} else
		c->maxw = c->maxh = 0;
	if (size.flags & PMinSize) {
		c->minw = size.min_width;
		c->minh = size.min_height;
	} else if (size.flags & PBaseSize) {
		c->minw = size.base_width;
		c->minh = size.base_height;
	} else
		c->minw = c->minh = 0;
	if (size.flags & PAspect) {
		c->mina = (float)size.min_aspect.y / size.min_aspect.x;
		c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
	} else
		c->maxa = c->mina = 0.0;
	c->isfixed = (c->maxw && c->maxh && c->maxw == c->minw && c->maxh == c->minh);
}

void
updatestatus(void)
{
	if (!gettextprop(root, XA_WM_NAME, stext, sizeof(stext))) {
		strcpy(stext, "dwm-"VERSION);
		statusw = TEXTW(stext) - lrpad + 2;
	} else {
		char *text, *s, ch;

		statusw  = 0;
		for (text = s = stext; *s; s++) {
			if ((unsigned char)(*s) < ' ') {
				ch = *s;
				*s = '\0';
				statusw += TEXTW(text) - lrpad;
				*s = ch;
				text = s + 1;
			}
		}
		statusw += TEXTW(text) - lrpad + 2;

	}
	drawbar(selmon);
	updatesystray();
}


void
updatesystrayicongeom(Client *i, int w, int h)
{
	if (i) {
		i->h = bh;
		if (w == h)
			i->w = bh;
		else if (h == bh)
			i->w = w;
		else
			i->w = (int) ((float)bh * ((float)w / (float)h));
		applysizehints(i, &(i->x), &(i->y), &(i->w), &(i->h), False);
		/* force icons into the systray dimensions if they don't want to */
		if (i->h > bh) {
			if (i->w == i->h)
				i->w = bh;
			else
				i->w = (int) ((float)bh * ((float)i->w / (float)i->h));
			i->h = bh;
		}
	}
}

void
updatesystrayiconstate(Client *i, XPropertyEvent *ev)
{
	long flags;
	int code = 0;

	if (!showsystray || !i || ev->atom != xatom[XembedInfo] ||
			!(flags = getatomprop(i, xatom[XembedInfo])))
		return;

	if (flags & XEMBED_MAPPED && !i->tags) {
		i->tags = 1;
		code = XEMBED_WINDOW_ACTIVATE;
		XMapRaised(dpy, i->win);
		setclientstate(i, NormalState);
	}
	else if (!(flags & XEMBED_MAPPED) && i->tags) {
		i->tags = 0;
		code = XEMBED_WINDOW_DEACTIVATE;
		XUnmapWindow(dpy, i->win);
		setclientstate(i, WithdrawnState);
	}
	else
		return;
	sendevent(i->win, xatom[Xembed], StructureNotifyMask, CurrentTime, code, 0,
			systray->win, XEMBED_EMBEDDED_VERSION);
}

void
updatesystray(void)
{
	XSetWindowAttributes wa;
	XWindowChanges wc;
	Client *i;
	Monitor *m = systraytomon(NULL);
	unsigned int x = m->mx + m->mw;
	unsigned int sw = TEXTW(stext) - lrpad + systrayspacing;
	unsigned int w = 1;

	if (!showsystray)
		return;
	if (systrayonleft)
		x -= sw + lrpad / 2;
	if (!systray) {
		/* init systray */
		if (!(systray = (Systray *)calloc(1, sizeof(Systray))))
			die("fatal: could not malloc() %u bytes\n", sizeof(Systray));
		systray->win = XCreateSimpleWindow(dpy, root, x, m->by, w, bh, 0, 0, scheme[SchemeSel][ColBg].pixel);
		wa.event_mask        = ButtonPressMask | ExposureMask;
		wa.override_redirect = True;
		wa.background_pixel  = scheme[SchemeNorm][ColBg].pixel;
		XSelectInput(dpy, systray->win, SubstructureNotifyMask);
		XChangeProperty(dpy, systray->win, netatom[NetSystemTrayOrientation], XA_CARDINAL, 32,
				PropModeReplace, (unsigned char *)&netatom[NetSystemTrayOrientationHorz], 1);
		XChangeWindowAttributes(dpy, systray->win, CWEventMask|CWOverrideRedirect|CWBackPixel, &wa);
		XMapRaised(dpy, systray->win);
		XSetSelectionOwner(dpy, netatom[NetSystemTray], systray->win, CurrentTime);
		if (XGetSelectionOwner(dpy, netatom[NetSystemTray]) == systray->win) {
			sendevent(root, xatom[Manager], StructureNotifyMask, CurrentTime, netatom[NetSystemTray], systray->win, 0, 0);
			XSync(dpy, False);
		}
		else {
			fprintf(stderr, "dwm: unable to obtain system tray.\n");
			free(systray);
			systray = NULL;
			return;
		}
	}
	for (w = 0, i = systray->icons; i; i = i->next) {
		/* make sure the background color stays the same */
		wa.background_pixel  = scheme[SchemeNorm][ColBg].pixel;
		XChangeWindowAttributes(dpy, i->win, CWBackPixel, &wa);
		XMapRaised(dpy, i->win);
		w += systrayspacing;
		i->x = w;
		XMoveResizeWindow(dpy, i->win, i->x, 0, i->w, i->h);
		w += i->w;
		if (i->mon != m)
			i->mon = m;
	}
	w = w ? w + systrayspacing : 1;
	x -= w;
	XMoveResizeWindow(dpy, systray->win, x, m->by, w, bh);
	wc.x = x; wc.y = m->by; wc.width = w; wc.height = bh;
	wc.stack_mode = Above; wc.sibling = m->barwin;
	XConfigureWindow(dpy, systray->win, CWX|CWY|CWWidth|CWHeight|CWSibling|CWStackMode, &wc);
	XMapWindow(dpy, systray->win);
	XMapSubwindows(dpy, systray->win);
	/* redraw background */
	XSetForeground(dpy, drw->gc, scheme[SchemeNorm][ColBg].pixel);
	XFillRectangle(dpy, systray->win, drw->gc, 0, 0, w, bh);
	XSync(dpy, False);
}

void
updatetitle(Client *c)
{
	if (!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
		gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name);
	// char *subname;
	// for(subname = c->name;*subname != '\0'; subname++);
	// for(;subname != c->name && *subname != '-'; subname--);
	// if (subname == c->name ) return;
	// for (; *subname != '\0' && (*subname == ' ' || *subname == '-'); subname++);
	// if(subname[0] != '\0' && subname[1] != '\0') strcpy(c->name, subname);
	if (c->name[0] == '\0') /* hack to mark broken clients */
		strcpy(c->name, broken);
}

void
updatewindowtype(Client *c)
{
	Atom state = getatomprop(c, netatom[NetWMState]);
	Atom wtype = getatomprop(c, netatom[NetWMWindowType]);

	if (state == netatom[NetWMFullscreen])
		setfullscreen(c, 1);
	if (wtype == netatom[NetWMWindowTypeDialog])
		c->isfloating = 1;
}

void
updatewmhints(Client *c)
{
	XWMHints *wmh;

	if ((wmh = XGetWMHints(dpy, c->win))) {
		if (c == selmon->sel && wmh->flags & XUrgencyHint) {
			wmh->flags &= ~XUrgencyHint;
			XSetWMHints(dpy, c->win, wmh);
		} else
			c->isurgent = (wmh->flags & XUrgencyHint) ? 1 : 0;
		if (wmh->flags & InputHint)
			c->neverfocus = !wmh->input;
		else
			c->neverfocus = 0;
		XFree(wmh);
	}
}

long
getcurrusec(){
	struct timeval us;
	gettimeofday(&us,NULL);
	return us.tv_sec * 1000000 + us.tv_usec;
}

void
view(const Arg *arg)
{
	int i;
	unsigned int tmptag;

	if ((arg->ui & TAGMASK) == selmon->tagset[selmon->seltags])
		return;
	selmon->seltags ^= 1; /* toggle sel tagset */
	if (arg->ui & TAGMASK) {
		selmon->tagset[selmon->seltags] = arg->ui & TAGMASK;
		selmon->pertag->prevtag = selmon->pertag->curtag;

		if (arg->ui == ~0)
			selmon->pertag->curtag = 0;
		else {
			for (i = 0; !(arg->ui & 1 << i); i++) ;
			selmon->pertag->curtag = i + 1;
		}
	} else {
		tmptag = selmon->pertag->prevtag;
		selmon->pertag->prevtag = selmon->pertag->curtag;
		selmon->pertag->curtag = tmptag;
	}

	selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag];
	selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag];
	selmon->sellt = selmon->pertag->sellts[selmon->pertag->curtag];
	selmon->lt[selmon->sellt] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt];
	selmon->lt[selmon->sellt^1] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt^1];


    // ------------- test smartview ---------------//
	// record lastviewtime
	int tagindex = selmon->pertag->curtag;
	Tag *tag = tagarray[tagindex];
	tag->tags = selmon->tagset[selmon->seltags];
	tag->tagindex = tagindex;
	tag->lastviewtime = time(NULL);
	tag->lastviewtimeusec = getcurrusec();
	tag->skip = 0;
	Tag *head = HEADTAG;
	Tag *prev = tag->prev;
	Tag *next = tag->next;

	// record edge
	TagStat *thisedge = taggraph[HEADTAG->prev->tagindex][tagindex];
	thisedge->lastviewtime = tag->lastviewtime;
	thisedge->jumpcnt ++;
	if(HEADTAG->prev->prev){
		TagStat *prevedge = taggraph[HEADTAG->prev->prev->tagindex][HEADTAG->prev->tagindex];
		LOG_FORMAT("prev->head edge, %d->%d\n", HEADTAG->prev->prev->tagindex, HEADTAG->prev->tagindex);
		prevedge->laststaytime = tag->lastviewtime - HEADTAG->prev->lastviewtime;
		prevedge->totalstaytime += tag->lastviewtime - HEADTAG->prev->lastviewtime;
	}
   // ------------- test smartview end---------------//

	tag->prev = head->prev;
	tag->next = head;
	head->prev->next = tag;
	head->prev = tag;
	if(prev)
		prev->next = next;
	if(next)
		next->prev = prev;

	if (selmon->showbar != selmon->pertag->showbars[selmon->pertag->curtag])
		togglebar(NULL);

	focus(NULL);
	arrange(selmon);
	updatecurrentdesktop();
}


// ------------- test smartview ---------------//
void
smartviewtest(Tag *tag)
{
	double staypmatrix[LENGTH(tags)+1][LENGTH(tags)+1];
	double leavepmatrix[LENGTH(tags)+1][LENGTH(tags)+1];
	int i;
	for(i=1; i< LENGTH(tags) + 1 ; i++){
		TagStat **row = taggraph[i];
		int rowsumcnt = 0;
		int j;
		for(j=1;j< LENGTH(tags) + 1 ; j++){
			rowsumcnt += row[j]->jumpcnt + 1;
		}
		for(j=1; j< LENGTH(tags) + 1 ; j++){
			staypmatrix[i][j] = (1.0/exp(time(NULL) - tagarray[j]->lastviewtime + 1.1))*(row[j]->jumpcnt + 1)*1.0 / rowsumcnt * (1- (1/exp(row[j]->laststaytime)));
			leavepmatrix[i][j] = (1.0/exp(time(NULL) - tagarray[j]->lastviewtime + 1.1))*(row[j]->jumpcnt + 1)*1.0 / rowsumcnt * (1/exp(row[j]->laststaytime));
			// staypmatrix[i][j] = (row[j]->jumpcnt + 1)*1.0 / rowsumcnt * (1- (1/exp(row[j]->laststaytime)));
			// leavepmatrix[i][j] = (row[j]->jumpcnt + 1)*1.0 / rowsumcnt * (1/exp(row[j]->laststaytime));
		}
	}
	
	double rowp[LENGTH(tags)+1];

	int j;
	for(j=1; j< LENGTH(tags) + 1 ; j++){
		double sumjp = 0;
		double sumip = 0;
		int i;
		for(i=1; i< LENGTH(tags) + 1 ; i++){
			sumjp += staypmatrix[j][i];
			sumip += staypmatrix[i][j];
		}
		
		rowp[j] = staypmatrix[tag->tagindex][j] + leavepmatrix[tag->tagindex][j] * sumjp + staypmatrix[j][tag->tagindex] + leavepmatrix[j][tag->tagindex] * sumip;
	}

	LOG_FORMAT("taggraph result:\n");
	for(i=1; i< LENGTH(tags) + 1 ; i++){
		for(j=1; j< LENGTH(tags) + 1 ; j++){
			LOG_FORMAT(" (%d %d %d %d) ", taggraph[i][j]->jumpcnt, taggraph[i][j]->lastviewtime,taggraph[i][j]->laststaytime, taggraph[i][j]->totalstaytime);
		}
		LOG_FORMAT("\n");
	}
	LOG_FORMAT("\n");

	LOG_FORMAT("calc p result:");
	for(j=1; j< LENGTH(tags) + 1 ; j++){
		LOG_FORMAT("%d_%f,", j, rowp[j]);
	}
	LOG_FORMAT("\n");

	int maxindex = 0;
	double maxp = 0;
	for(j=1; j< LENGTH(tags) + 1 ; j++){
		if(maxp < rowp[j]){
			maxp = rowp[j];
			maxindex = j;
		}
	}

	const Arg arg2 = {.ui = tagarray[maxindex]->tags};
	view(&arg2);
}

void 
smartviewtest2(){
	Tag * tag;
	int i = 0;
	for(tag = HEADTAG->prev ; tag != TAILTAG; tag = tag->prev, i ++);
	int n = i;
	double pl[n];
	double fornextp = 0.0;
	pl[0] = 0.0;
	long laststaytimel[n];
	for(i = 1, tag = HEADTAG->prev->prev ; tag && tag != TAILTAG; tag = tag->prev,i++){
		laststaytimel[i] = tag->next->lastviewtimeusec - tag->lastviewtimeusec;
	}
	double rlaststaytimel[n];
	long minstaytime = LONG_MAX;
	for(i = 1, tag = HEADTAG->prev->prev ; tag && tag != TAILTAG; tag = tag->prev,i++){
		if(minstaytime > laststaytimel[i])
			minstaytime = laststaytimel[i];
	}
	for(i = 1, tag = HEADTAG->prev->prev ; tag && tag != TAILTAG; tag = tag->prev,i++){
		rlaststaytimel[i] = laststaytimel[i] * 1.0 / minstaytime ;
	}
	int supplied = 0;
	for(i = 1, tag = HEADTAG->prev->prev ; tag && tag != TAILTAG; tag = tag->prev,i ++){
		double p = -log(i*1.0/LENGTH(tags))/exp(1) + fornextp;
		if (rlaststaytimel[i] > 2 && !supplied){
			p += 0.5;
			supplied = 1;
		} 
		pl[i] = p;
	}
	LOG_FORMAT("smartviewtest2, pl:");
	for(i = 0,tag = HEADTAG->prev ; tag != TAILTAG; tag = tag->prev, i ++){
		LOG_FORMAT(" %d:%f ", tag->tagindex, pl[i]);
	}

	int maxindex = 0;
	double maxp = 0;
	for(i = 0,tag = HEADTAG->prev ; tag != TAILTAG; tag = tag->prev, i ++){
		if(maxp < pl[i]){
			maxindex = tag->tagindex;
			maxp = pl[i];
		}
	}

	const Arg arg2 = {.ui = tagarray[maxindex]->tags};
	view(&arg2);
}

void
smartview(const Arg *arg){
	
	ScratchGroup *sg = scratchgroupptr;
	if(sg->isfloating){
		hidescratchgroup(sg);
	}else{
		if(arg->ui & TAGMASK){
			view(arg);
		}else{
			// smart failed, fallback to origin. expect reborn.
			view(arg);
			return;

			Tag *tag = tagarray[selmon->pertag->curtag];
			// smartviewtest2(tag);
			// return;
			
			// LOG_FORMAT("tag: %d, time:%d", tag->tagindex, tag->lastviewtime);
			Tag *last = tag;
			Tag *tmp;
			for(tmp = tag->prev; tmp && tmp != TAILTAG ; last = tmp, tmp = tmp->prev){
				time_t stayperiod = last->lastviewtime - tmp->lastviewtime;
				if(!tmp->skip && stayperiod > 1){
					const Arg arg2 = {.ui = tmp->tags};
					view(&arg2);
					return;
				}
				// at least two
				if(tmp->prev != TAILTAG){
					tmp->skip = 1;
					LOG_FORMAT("tag: %d skipped , stay:%ds\n", tmp->tagindex, stayperiod);
				}
			}
			if(tag->prev != TAILTAG){
				Tag *valid;
				for(valid = tag->prev; valid->skip; valid = valid->prev);
				const Arg arg2 = {.ui = valid->tags};
				view(&arg2);
			}
		}
	}
}

// ------------- test smartview end---------------//

void
relview(const Arg *arg)
{
	unsigned int maxtags = getmaxtags();
	unsigned int nexttags = 0;
	if (arg->i > 0)
	{
		nexttags = selmon->tagset[selmon->seltags] << arg->i;
		while (counttag(selmon->clients, nexttags) == 0)
		{
			if (nexttags > maxtags)
			{
				nexttags = 1;
				break;
			}else {
				nexttags = nexttags << 1;
			}
		}
	}
	if (arg->i < 0)
	{
		nexttags = selmon->tagset[selmon->seltags] >> -arg->i;
		while (counttag(selmon->clients, nexttags) == 0)
		{
			if (!nexttags)
			{
		  		nexttags = maxtags ;
				break;
			}else {
				nexttags = nexttags >> 1;
			}
		}
	}

	if (arg -> i != 0 && nexttags)
	{
		Arg viewarg = {.ui = nexttags};
		view(&viewarg);
	}
	
}

Client *
wintoclient(Window w)
{
	Client *c;
	Monitor *m;

	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next)
			if (c->win == w)
				return c;
	return NULL;
}

Client *
wintosystrayicon(Window w) {
	Client *i = NULL;

	if (!showsystray || !w)
		return i;
	for (i = systray->icons; i && i->win != w; i = i->next) ;
	return i;
}

Monitor *
wintomon(Window w)
{
	int x, y;
	Client *c;
	Monitor *m;

	if (w == root && getrootptr(&x, &y))
		return recttomon(x, y, 1, 1);
	for (m = mons; m; m = m->next)
		if (w == m->barwin)
			return m;
	if ((c = wintoclient(w)))
		return c->mon;
	return selmon;
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's). Other types of errors call Xlibs
 * default error handler, which may call exit. */
int
xerror(Display *dpy, XErrorEvent *ee)
{
	if (ee->error_code == BadWindow
	|| (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
	|| (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
	|| (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
	|| (ee->request_code == X_GrabButton && ee->error_code == BadAccess)
	|| (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
	|| (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
		return 0;
	fprintf(stderr, "dwm: fatal error: request code=%d, error code=%d\n",
		ee->request_code, ee->error_code);
	return xerrorxlib(dpy, ee); /* may call exit */
}

int
xerrordummy(Display *dpy, XErrorEvent *ee)
{
	return 0;
}

/* Startup Error handler to check if another window manager
 * is already running. */
int
xerrorstart(Display *dpy, XErrorEvent *ee)
{
	die("dwm: another window manager is already running");
	return -1;
}

Monitor *
systraytomon(Monitor *m) {
	Monitor *t;
	int i, n;
	if(!systraypinning) {
		if(!m)
			return selmon;
		return m == selmon ? m : NULL;
	}
	for(n = 1, t = mons; t && t->next; n++, t = t->next) ;
	for(i = 1, t = mons; t && t->next && i < systraypinning; i++, t = t->next) ;
	if(systraypinningfailfirst && n < systraypinning)
		return mons;
	return t;
}

void
zoom(const Arg *arg)
{
	Client *c = selmon->sel;
	c->fullscreenfreq ++;
	if (!selmon->lt[selmon->sellt]->arrange
	|| (selmon->sel && selmon->sel->isfloating))
		return;
	if (c == nexttiled(selmon->clients))
		if (!c || !(c = nexttiled(c->next)))
			return;
	pop(c);
}


void
zoomi(const Arg *arg)
{
	int targeti = arg->i;
	Client *c;
	if(targeti == 0){
		c = selmon->sel;
	}else{
		Client *lastc;
		int i = 0;
		for(c = selmon->clients; c; c = c->next){
			if(ISVISIBLE(c)){
				lastc = c;
				if(targeti == i) break;
				i++;
			}
		}
		if(!c) c = lastc;
	}
	if(!c) return;
	c->fullscreenfreq ++;
	if (!selmon->lt[selmon->sellt]->arrange
	|| (selmon->sel && selmon->sel->isfloating))
		return;
	if (c == nexttiled(selmon->clients))
		if (!c || !(c = nexttiled(c->next)))
			return;
	pop(c);
}

void 
smartzoom(const Arg *arg)
{
	int isselcslave = 0;
	int i;
	Client *c;
	for (i = 0, c = nexttiled(selmon->clients); c; c = nexttiled(c->next), i++)
	{
		if (i >= selmon->nmaster && c == selmon->sel) isselcslave = 1;
	}
	if(isselcslave){
		const Arg arg = {0};
		zoom(&arg);
	}else{
		const Arg arg = {.i = 1};
		zoomi(&arg);
	}
}

void
setcurrentdesktop(void){
	long data[] = { 0 };
	XChangeProperty(dpy, root, netatom[NetCurrentDesktop], XA_CARDINAL, 32, PropModeReplace, (unsigned char *)data, 1);
}
void setdesktopnames(void){
	XTextProperty text;
	Xutf8TextListToTextProperty(dpy, tags, TAGSLENGTH, XUTF8StringStyle, &text);
	XSetTextProperty(dpy, root, &text, netatom[NetDesktopNames]);
}

void
setnumdesktops(void){
	long data[] = { TAGSLENGTH };
	XChangeProperty(dpy, root, netatom[NetNumberOfDesktops], XA_CARDINAL, 32, PropModeReplace, (unsigned char *)data, 1);
}

void
setviewport(void){
	long data[] = { 0, 0 };
	XChangeProperty(dpy, root, netatom[NetDesktopViewport], XA_CARDINAL, 32, PropModeReplace, (unsigned char *)data, 2);
}

void updatecurrentdesktop(void){
	long rawdata[] = { selmon->tagset[selmon->seltags] };
	int i=0;
	while(*rawdata >> i+1){
		i++;
	}
	long data[] = { i };
	XChangeProperty(dpy, root, netatom[NetCurrentDesktop], XA_CARDINAL, 32, PropModeReplace, (unsigned char *)data, 1);
}
 

int
main(int argc, char *argv[])
{
	close(2);
	logfile = fopen("/home/beyond/m.log", "a");
	if (argc == 2 && !strcmp("-v", argv[1]))
		die("dwm-"VERSION);
	else if (argc != 1)
		die("usage: dwm [-v]");
	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fputs("warning: no locale support\n", stderr);
	if (!(dpy = XOpenDisplay(NULL)))
		die("dwm: cannot open display");
	checkotherwm();
	setup();
#ifdef __OpenBSD__
	if (pledge("stdio rpath proc exec", NULL) == -1)
		die("pledge");
#endif /* __OpenBSD__ */
	scan();
	runAutostart();
	run();
	if(restart) execvp(argv[0], argv);
	cleanup();
	XCloseDisplay(dpy);
	fclose(logfile);
	return EXIT_SUCCESS;
}
