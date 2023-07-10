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
#include <X11/X.h>
#include <errno.h>
#include <limits.h>
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
#include <time.h>
#include <dirent.h>
#include <regex.h>
#include <stdint.h>

#include "http.c"
#include "drw.h"
#include "util.h"
#include "list.h"

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
static FILE *actionlogfile;

/* enums */
enum { CurNormal, CurResize, CurMove, CurLast }; /* cursor */
enum { SchemeNorm, SchemeSel , SchemeScr,SchemeInvalidNormal, SchemeInvalidSel,SchemeDoublePageMarked, SchemeTiled}; /* color schemes */
enum { NetSupported, NetWMName, NetWMIcon, NetWMState, NetWMCheck,
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

typedef struct MXY MXY ;
struct MXY
{
	int row;
	int col;
};

typedef struct XY XY ;
struct XY
{
	int x;
	int y;
};

typedef struct {
	unsigned int click;
	unsigned int mask;
	unsigned int button;
	void (*func)(const Arg *arg);
	const Arg arg;
} Button;

typedef struct Monitor Monitor;
typedef struct Client Client;
typedef struct Container Container;
struct Container {
	int id;
	MXY matcoor;
	int launchindex;
	Container *launchparent;
	int cn;
	Client *cs[2];
	int placed;
	int x, y, w, h;
	float masterfactor;
};
struct Client {
	int id;
	char name[256];
	char class[64];
	float mina, maxa;
	int x, y, w, h;
	int oldx, oldy, oldw, oldh;
	int basew, baseh, incw, inch, maxw, maxh, minw, minh;
	int hintsvalid;
	int bw, oldbw;
	unsigned int tags;
	int isfixed, isfloating, isurgent, neverfocus, oldstate, isfullscreen, isfocused, istemp, isdoublepagemarked, isdoubled,placed;
	Client *next;
	Client *snext;
	Client *lastfocus;
	Client *launchparent;
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
	float factx, facty;
	unsigned int icw, ich; Picture icon;
	unsigned int icws[3], ichs[3]; Picture icons[3];
	MXY matcoor;
	int launchindex;

	Client *containerrefc;
	//tmp
	Container *container;
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

typedef struct SwitcherAction SwitcherAction ;
struct SwitcherAction{
	void (*drawfunc)(Window win, int ww, int wh);
	void (*drawfuncx)(Window win, int ww, int wh);
	void (*movefunc)(const Arg *);
	Client *(*sxy2client)(int rx, int ry);
	void *(*pointerfunc)(int rx, int ry);
	void (*xy2switcherxy)(XY cxy[], int n, XY sxy[], int tagindexin[]);
	void (*switcherxy2xy)(XY sxy[], int n, XY cxy[], int tagindexout[]);
};

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
	Window barwin,switcher,switcherbarwin, switcherstickywin;
	int switcherww, switcherwh, switcherwx, switcherwy;
	int switcherbarww, switcherbarwh, switcherbarwx, switcherbarwy;
	int switcherstickyww, switcherstickywh, switcherstickywx, switcherstickywy;
	SwitcherAction switcheraction, switcherbaraction, switcherstickyaction;
	const Layout *lt[2];
	Pertag *pertag;
	int systrayrx, systrayy;
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
	int placed; // 是否已经调整过大小和位置, 如果是, 则下次显示用原来的大小和位置
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
	Client *lastfocused;
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

typedef struct TaskGroupItem TaskGroupItem;
struct TaskGroupItem
{
	char classpattern[256];
	char titlepattern[256];
	char **cmd;
	unsigned int tag;

	char cmdbuf[256];
	regex_t *classregx;
	regex_t *titleregx;
	Client *c;
};

typedef struct TaskGroup TaskGroup ;
struct TaskGroup
{
	int n;
	TaskGroupItem items[20];
};


/* function declarations */
static void applyrules(Client *c);
static int applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact);
static void arrange(Monitor *m);
static void arrangemon(Monitor *m);
static void attach(Client *c);
static void attachstack(Client *c);
static void addtoscratchgroup(const Arg *arg);
static void assemble(const Arg *arg);
static void assemblecsv(const Arg *arg);
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
static Container *createcontainer(void);
static Container *createcontainerc(Client *c);
static void destroynotify(XEvent *e);
static void detach(Client *c);
static void detachstack(Client *c);
static Monitor *dirtomon(int dir);
static void drawbar(Monitor *m);
static void drawbars(void);
static void drawswitcher(Monitor *m);
static void destroyswitcher(Monitor *m);
static void drawswitchersticky(Monitor *m);
static void destroyswitchersticky(Monitor *m);
static void doublepage(Monitor *m);
static void doublepagemarkclient(Client *c);
static void doublepagemark(const Arg *arg);
static void dismiss(const Arg *arg);
static void cleardoublepage(int view);
static void toggleswitchers(const Arg *arg);
static void toggleswitchersticky(const Arg *arg);
static void enqueue(Client *c);
static void enqueuestack(Client *c);
static void enternotify(XEvent *e);
static void expose(XEvent *e);
static void empty(const Arg *arg);
static void expand(const Arg *arg);
static void focus(Client *c);
static void focusin(XEvent *e);
static void focusmon(const Arg *arg);
static void focusstack(const Arg *arg);
static void focusstackarrange(const Arg *arg);
static void focuslast(const Arg *arg);
static void focusgrid(const Arg *arg);
static void focusgrid5(const Arg *arg);
static void free_si(ScratchItem *si);
static ScratchItem *findscratchitem(Client *c, ScratchGroup *sg);
static void freeicon(Client *c);
static void removeclientfromcontainer(Container *container, Client *c);
static void freecontainerc(Container *container, Client *c);
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
static long getcurrusec();
static Picture geticonprop(Window w, unsigned int *icw, unsigned int *ich);
static unsigned long getwindowpid(Window w);
static void hidescratchgroup(ScratchGroup *sg);
static void hidescratchgroupv(ScratchGroup *sg, int isarrange);
static void incnmaster(const Arg *arg);
static void keypress(XEvent *e);
static void keyrelease(XEvent *e);
static void killclient(const Arg *arg);
static void killclientforce(const Arg *arg);
static void killclientc(Client *c);
static void manage(Window w, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static void mapnotify(XEvent *e);
static void monocle(Monitor *m);
static void monoclespace(Monitor *m);
static void motionnotify(XEvent *e);
static void movemouse(const Arg *arg);
static void movemouseswitcher(const Arg *arg);
static void movex(const Arg *arg);
static void movey(const Arg *arg);
static int mmax(int num,...);
static int mmin(int num,...);
static Client *nexttiled(Client *c);
static Client *nextclosestc(const Arg *arg);
static void pop(Client *);
static void propertynotify(XEvent *e);
static void pysmoveclient(Client *target, int sx, int sy);
static void quit(const Arg *arg);
static Monitor *recttomon(int x, int y, int w, int h);
static void removesystrayicon(Client *i);
static void resize(Client *c, int x, int y, int w, int h, int interact);
static void resizebarwin(Monitor *m);
static void resizeclient(Client *c, int x, int y, int w, int h);
static void resizemouse(const Arg *arg);
static void resizex(const Arg *arg);
static void resizey(const Arg *arg);
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
static void setfacty(const Arg *arg);
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
static void ispawn(const Arg *arg);
static void itspawn(const Arg *arg);
static void sspawn(const Arg *arg);
static void stspawn(const Arg *arg);
static void stsspawn(const Arg *arg);
static void stispawn(const Arg *arg);
static void swap(const Arg *arg);
static Monitor *systraytomon(Monitor *m);
static void smartview(const Arg *arg);
static void showscratchgroup(ScratchGroup *sg);
static void switchermove(const Arg *arg);
static void switcherview(const Arg *arg);
static void tag(const Arg *arg);
static void tagmon(const Arg *arg);
static void tile(Monitor *);
static void tile2(Monitor *);
static void tile3(Monitor *);
static void tile4(Monitor *);
static void tile5(Monitor *);
static void tile6(Monitor *);
static void tile7(Monitor *);
static void tile6zoom(const Arg *arg);
static void tile6maximize(const Arg *arg);
static void tiledual(const Arg *arg);
static void togglebar(const Arg *arg);
static void togglefloating(const Arg *arg);
static void togglescratch(const Arg *arg);
static void togglescratchgroup(const Arg *arg);
static void toggletag(const Arg *arg);
static void toggleview(const Arg *arg);
static void tsspawn(const Arg *arg);
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
static void updateclass(Client *c);
static void updatewindowtype(Client *c);
static void updatewmhints(Client *c);
static void updateicon(Client *c);
static void updateicons(Client *c);
static void updateswitchersticky(Monitor *m);
static void view(const Arg *arg);
static void viewi(int tagindex);
static void viewui(unsigned int tagui);
static void relview(const Arg *arg);
static void reltag(const Arg *arg);
static void reltagd(const Arg *arg);
static void removefromscratchgroup(const Arg *arg);
static void replacechar(char *buf, char old, char new);
static Client *wintoclient(Window w);
static Monitor *wintomon(Window w);
static Client *wintosystrayicon(Window w);
static int xerror(Display *dpy, XErrorEvent *ee);
static int xerrordummy(Display *dpy, XErrorEvent *ee);
static int xerrorstart(Display *dpy, XErrorEvent *ee);
static void zoom(const Arg *arg);
static void zoomi(const Arg *arg);
static void smartzoom(const Arg *arg);
static void left(int **arr, int row ,int col, int x, int y, int result[2]);
static void right(int **arr, int row ,int col, int x, int y, int result[2]);
static void up(int **arr, int row ,int col, int x, int y, int result[2]);
static void down(int **arr, int row ,int col, int x, int y, int result[2]);
static void switcherxy2clientxy(XY sxys[], int n, XY cxys[]);
static void clientxy2switcherxy(XY cxys[], int n, XY sxys[]);
static XY clientxy2centered(XY xy);

static void separatefromcontainer(Client *oldc);
static void mergetocontainerof(Client *oldc, Client *chosenc);
static void replacercincontainer(Client *oldc, Client *chosenc);
static void rispawn(const Arg *arg);

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
static int bh = 0, blw = 0;      /* bar geometry */
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
	[KeyRelease] = keyrelease,
	[MappingNotify] = mappingnotify,
	[MapRequest] = maprequest,
	[MapNotify] = mapnotify,
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

static volatile int isnexttemp = 0;
static const char **nexttempcmd;
static long lastnexttemptime;
static long lastmanagetime;
static long lastspawntime;
static volatile int isnextinner = 0;
static long lastnextinnertime;
static pid_t ispawnpids[] = {0};
static long ispawntimes[] = {0};
static volatile int isnextreplace = 0;
static long lastnextreplacetime;

static int switchercurtagindex;

/*static float tile6initwinfactor = 0.9;*/
static float tile6initwinfactor = 1;
static float lasttile6initwinfactor = 0.9;
static int global_client_id = 1;

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
static MXY spiral_index[] = {{0,0},{0,1},{-1,1},{-1,0},{-1,-1},{0,-1},{1,-1},{1,0},{1,1},{1,2},{0,2},{-1,2},{-2,2},{-2,1},{-2,0},{-2,-1},{-2,-2},{-1,-2},{0,-2},{1,-2},{2,-2},{2,-1},{2,0},{2,1},{2,2},{2,3},{1,3},{0,3},{-1,3},{-2,3},{-3,3},{-3,2},{-3,1},{-3,0},{-3,-1},{-3,-2},{-3,-3},{-2,-3},{-1,-3},{0,-3},{1,-3},{2,-3},{3,-3},{3,-2},{3,-1},{3,0},{3,1},{3,2},{3,3},{3,4},{2,4},{1,4},{0,4},{-1,4},{-2,4},{-3,4},{-4,4},{-4,3},{-4,2},{-4,1},{-4,0},{-4,-1},{-4,-2},{-4,-3},{-4,-4},{-3,-4},{-2,-4},{-1,-4},{0,-4},{1,-4},{2,-4},{3,-4},{4,-4},{4,-3},{4,-2},{4,-1},{4,0},{4,1},{4,2},{4,3},{4,4},{4,5},{3,5},{2,5},{1,5},{0,5},{-1,5},{-2,5},{-3,5},{-4,5},{-5,5},{-5,4},{-5,3},{-5,2},{-5,1},{-5,0},{-5,-1},{-5,-2},{-5,-3},{-5,-4},{-5,-5},{-4,-5},{-3,-5},{-2,-5},{-1,-5},{0,-5},{1,-5},{2,-5},{3,-5},{4,-5},{5,-5},{5,-4},{5,-3},{5,-2},{5,-1},{5,0},{5,1},{5,2},{5,3},{5,4},{5,5}};

static int islog = 1;

void
LOG(char *content, char * content2){
	if(!islog) return;
	fprintf(logfile,"%s%s\n", content, content2);
}

void
LOG_FORMAT(char *format, ...){

	if(!islog) return;

	struct timeval us;
	gettimeofday(&us, NULL);

	time_t tnow;
	tnow=time(0); 
	struct tm *sttm;  
	sttm=localtime(&tnow);
	fprintf(logfile, "%04u-%02u-%02u %02u:%02u:%02u.%03u: ", sttm->tm_year + 1900, sttm->tm_mon + 1,
			sttm->tm_mday, sttm->tm_hour, sttm->tm_min, sttm->tm_sec, us.tv_usec/1000);
	va_list ap;
	va_start(ap,format);
	vfprintf(logfile,format, ap);
	fprintf(logfile,"\n");
	va_end(ap);
	fflush(logfile);
}

void 
getclass(Window w, char c[])
{

	XClassHint ch = { NULL, NULL };
	XGetClassHint(dpy, w, &ch);
	char *class = ch.res_class ? ch.res_class : broken;

	strcpy(c, class);
	
	if (ch.res_class)
		XFree(ch.res_class);
	if (ch.res_name)
		XFree(ch.res_name);
}

/* function implementations */
void actionlog(char *action, Client *c)
{
	if (!islog) return;
	if (strstr(c->name,"Private")) return;

	char *class = "NULL";
	/*char class[64];*/
	/*getclass(c->win, class);*/
	struct timeval us;
	gettimeofday(&us,NULL);
	char name[256];
	strcpy(name, c->name);
	replacechar(name, ',',' ');
	fprintf(actionlogfile,"%ld\t%s\t%s\t%s\n", us.tv_sec*1000 + us.tv_usec/1000, action, class, name);
	fflush(actionlogfile);
}

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

unsigned int
getmaxtagstiled(){
	// 将没有rule的client放到最后一个空闲tag
	Client *c;
	unsigned int maxtags = 0;
	for(c = selmon->clients; c; c = c->next)
		if (c->tags > maxtags && !c->isfloating)
			maxtags = c->tags;
	return maxtags;
}

unsigned int
getmintagstiled(){
	// 将没有rule的client放到最后一个空闲tag
	Client *c;
	unsigned int mintags = 1 << LENGTH(tags);
	for(c = selmon->clients; c; c = c->next)
		if (c->tags < mintags && !c->isfloating)
			mintags = c->tags;
	return mintags;
}

void
rerule(const Arg *arg)
{
	Client *c;
	
	for(c = selmon->clients; c; c = c->next){
		if (c->istemp)
		{
			killclientc(c);
			continue;
		}
	}
	
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
	selmon->sellt = 0;
	arrange(selmon);
	if(selmon->sel){
		Arg arg2 = {.ui=selmon->sel->tags};
		view(&arg2);
	}
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
		if (!c->hintsvalid)
			updatesizehints(c);		
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
	LOG_FORMAT("arrange 1");
	if (m)
		showhide(m->stack);
	else for (m = mons; m; m = m->next)
		showhide(m->stack);
	LOG_FORMAT("arrange 2");
	if (m) {
		LOG_FORMAT("arrange 3");
		arrangemon(m);
		LOG_FORMAT("arrange 4");
		restack(m);
		LOG_FORMAT("arrange 5");
	} else for (m = mons; m; m = m->next)
		arrangemon(m);
	
	if (m && m->switcherstickywin) {
		m->switcherstickyaction.drawfunc(m->switcherstickywin, m->switcherstickyww, m->switcherstickywh);
	}
}

void
updateborder(Client *c){
	if (ISVISIBLE(c) && c->win)
	{
		if (c->isdoublepagemarked && c->isscratched && c == selmon->sel)
			XSetWindowBorder(dpy, c->win, scheme[SchemeScr][ColBorder].pixel);

		if (!c->isdoublepagemarked && c->isscratched && c == selmon->sel)
			XSetWindowBorder(dpy, c->win, scheme[SchemeScr][ColBorder].pixel);

		if (c->isdoublepagemarked && !c->isscratched && c == selmon->sel)
			XSetWindowBorder(dpy, c->win, scheme[SchemeDoublePageMarked][ColBorder].pixel);

		if (c->isdoublepagemarked && c->isscratched && c != selmon->sel)
			XSetWindowBorder(dpy, c->win, scheme[SchemeDoublePageMarked][ColBorder].pixel);

		if (!c->isdoublepagemarked && !c->isscratched && c == selmon->sel)
			XSetWindowBorder(dpy, c->win, scheme[SchemeSel][ColBorder].pixel);

		if (!c->isdoublepagemarked && c->isscratched && c != selmon->sel)
			XSetWindowBorder(dpy, c->win, scheme[SchemeNorm][ColBorder].pixel);

		if (c->isdoublepagemarked && !c->isscratched && c != selmon->sel)
			XSetWindowBorder(dpy, c->win, scheme[SchemeDoublePageMarked][ColBorder].pixel);

		if (!c->isdoublepagemarked && !c->isscratched && c != selmon->sel)
			XSetWindowBorder(dpy, c->win, scheme[SchemeNorm][ColBorder].pixel);
	
	}
}

void
arrangemon(Monitor *m)
{
	Client *c;
	for(c = selmon->clients;c;c=c->next){
		updateborder(c);
	}
	
	strncpy(m->ltsymbol, m->lt[m->sellt]->symbol, sizeof m->ltsymbol);
	if (m->lt[m->sellt]->arrange)
		m->lt[m->sellt]->arrange(m);
}

// unused
void arrangemonlayout(Monitor *m, Layout *layout)
{
	if(!layout) return;
	// strncpy(layout->symbol, m->lt[m->sellt]->symbol, sizeof layout->symbol);
	if (layout->arrange)
		layout->arrange(m);
}

// unused
void arrangelayout(Monitor *m, Layout *layout)
{
	if (m)
		showhide(m->stack);
	else
		for (m = mons; m; m = m->next)
			showhide(m->stack);
	if (m)
	{
		arrangemonlayout(m, layout);
		restack(m);
	}
	else
		for (m = mons; m; m = m->next)
			arrangemonlayout(m, layout);
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
switcherbuttonpress(XButtonPressedEvent *ev)
{
	int rx = ev->x;
	int ry = ev->y;
	int iw = selmon->ww / 6;
	int ih = selmon->wh / 6;
	int col = rx/iw;
	int row = ry/ih;
	int tagindex = col+row*3;
	unsigned int tags = 1 << tagindex;
	const Arg arg = {.ui = tags};
	view(&arg);
	destroyswitcher(selmon);
}

void
buttonpress(XEvent *e)
{
	LOG_FORMAT("buttonpress 1");
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
			int i;
			for(c = selmon->clients,i=0; c; c = c->next){
				if ( ISVISIBLE(c) && ev->x > c->titlex && ev->x < (c->titlex+c->titlew))
				{
					if(ev->button == Button1)
					{
						/*focus(c);*/
						Arg zoomiarg = {.i = i};
						zoomi(&zoomiarg);
					}
					if (ev->button == Button3)
					{
						killclientc(c);
					}
					break;
				}
				if (ISVISIBLE(c)) {
					i ++;
				}
			}
		}
	}else if(ev->window == selmon->switcher){
		/*selmon->switcheraction.pointerfunc(ev->x, ev->y);*/
		/*destroyswitcher(selmon);*/
		if(CLEANMASK(MODKEY) == CLEANMASK(ev->state)) movemouseswitcher(&arg);
		else destroyswitcher(selmon);
		return;
	}else if(ev->window == selmon->switcherstickywin && ev->button == Button3){
		Client *sc = selmon->switcherstickyaction.sxy2client(ev->x, ev->y);
		if (sc) {
			killclientc(sc);
			updateswitchersticky(selmon);
		}
		return;
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
	free(scheme);
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
				removefromscratchgroupc(c);
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
	} else if (cme->send_event && cme->message_type == netatom[NetActiveWindow]) {
		// if (c != selmon->sel && !c->isurgent)
		// 	seturgent(c, 1);
		
		if((c->tags & TAGMASK) == TAGMASK || (selmon->tagset[selmon->seltags] & TAGMASK) == TAGMASK){
			return;
		}
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
		LOG_FORMAT("configurenotify");
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
	unsigned int i, occ = 0, urg = 0, n = 0, occt = 0;
	Client *c;

	if (!m->showbar)
		return;

	if(showsystray && m == systraytomon(m) && !systrayonleft)
		stw = getsystraywidth();

	LOG_FORMAT("drawbar 1");

	/* draw status first so it can be overdrawn by tags later */
	if (m == selmon) { // status is only drawn on selected monitor
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

	LOG_FORMAT("drawbar 2");

	resizebarwin(m);
	for (c = m->clients; c; c = c->next) {
		if (ISVISIBLE(c))
			n++;
		occ |= c->tags;
		if(!c->isfloating) occt |= c->tags;
		if (c->isurgent)
			urg |= c->tags;
	}
	LOG_FORMAT("drawbar 3");
	x = 0;
	for (i = 0; i < LENGTH(tags); i++) {
		/* Do not draw vacant tags */
		if(!(occ & 1 << i || m->tagset[m->seltags] & 1 << i))
			continue;
		w = TEXTW(tags[i]);
		int colorindex = SchemeNorm;
		if (occt & 1 << i) {
			colorindex = SchemeTiled;
		}
		drw_setscheme(drw, scheme[m->tagset[m->seltags] & 1 << i ? SchemeSel :colorindex]);

		// update tag title
		Client *c;
		for(c = m->stack;c;c = c->snext){
			if (!c->isfloating && (c->tags & (1 << i))) {
				break;
			}
		}
		if (c) {
			char tagname[20];
			int tagnamelen = strlen(tags[i]);
			do {
				tagnamelen ++;
				snprintf(tagname, tagnamelen,"%d-%s",i + 1, c->name);
			}while (TEXTW(tagname) < TEXTW(tags[i]) && tagnamelen < 20);
			drw_text(drw, x, 0, w, bh, lrpad / 2,tagname, urg & 1 << i);
		}else {
			drw_text(drw, x, 0, w, bh, lrpad / 2, tags[i], urg & 1 << i);
		}

		x += w;
	}
	w = blw = TEXTW(m->ltsymbol);
	drw_setscheme(drw, scheme[SchemeNorm]);
	x = drw_text(drw, x, 0, w, bh, lrpad / 2, m->ltsymbol, 0);
	
	LOG_FORMAT("drawbar 4");
	for (i = 0; i < LENGTH(launchers); i++)
	{
		w = TEXTW(launchers[i].name);
		drw_text(drw, x, 0, w, bh, lrpad / 2, launchers[i].name, urg & 1 << i);
		x += w;
	}

	LOG_FORMAT("drawbar 7 %ld %d %d %d %d", m->barwin, m->ww, tw, stw, x);

	if ((w = m->ww - tw - stw - x) > bh) {
		if (n > 0) {
			/*tw = TEXTW(m->sel->name) + lrpad;*/
			tw = TEXTW(m->sel?m->sel->name:"HHHHHHHHHHHH") + lrpad;
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
					if (c->icon) {
						drw_text(drw, x, 0, tw, bh, lrpad / 2 + c->icw + ICONSPACING, c->name, 0);
						drw_pic(drw, x + lrpad / 2, (bh - c->ich) / 2, c->icw, c->ich, c->icon);
					}else{
						drw_text(drw, x, 0, tw, bh, lrpad / 2, c->name, 0);
					}
					c->titlex = x;
					c->titlew = tw;
				}
				if (c->isfloating)
					drw_rect(drw, x + boxs, boxs, boxw, boxw, c->isfixed, 0);
				x += tw;
				w -= tw;
			}
		}
		LOG_FORMAT("drawbar 6 %ld", m->barwin);
		drw_setscheme(drw, scheme[SchemeNorm]);
		drw_rect(drw, x, 0, w, bh, 1, 1);
	}
	drw_map(drw, m->barwin, 0, 0, m->ww - stw, bh);
	LOG_FORMAT("drawbar 5");
}


void
drawbars(void)
{
	Monitor *m;

	for (m = mons; m; m = m->next)
		drawbar(m);
}

int
gettagindex(unsigned int tag)
{
	int len = LENGTH(tags);
	int curtagindex = -1;
	while (tag)
	{
		if(curtagindex >= len) return len - 1;
		tag = tag >> 1;
		curtagindex++;
	}
	return curtagindex;
}

int
getcurtagindex(Monitor *m)
{
	unsigned int tag = m->tagset[m->seltags];
	return gettagindex(tag);
}

struct TagClient
{
	Client *client;
	struct list_head head;
};

void 
free_list(struct TagClient *tc)
{
	struct list_head *freehead;
	struct list_head *head = &tc->head;
	struct list_head *last = tc->head.prev;
	while (NULL != head)
	{
		head->prev->next = NULL;
		freehead = head;
		head = head->next;
		struct TagClient *freetc = list_entry(freehead, struct TagClient, head);
		free(freetc);
	}
}

void
drawswitcherwin(Window win, int ww, int wh, int curtagindex)
{
	drw_setscheme(drw, scheme[SchemeNorm]);
	drw_rect(drw, 0, 0, ww, wh, 1, 1);	

	int i;

	struct TagClient *tagclientsmap[LENGTH(tags)];
	for(i = 0; i < LENGTH(tags); i++){
		struct TagClient *tagclient = (struct TagClient *)malloc(sizeof(struct TagClient));
		tagclient->head.next = &tagclient->head;
		tagclient->head.prev = &tagclient->head;
		tagclient->client = NULL;
		tagclientsmap[i] = tagclient;
	}

	Client *c;
	for(c=selmon->clients; c; c = c->next)
	{
		int tagindex = gettagindex(c->tags);
		struct TagClient *tagclient = (struct TagClient *)malloc(sizeof(struct TagClient));
		tagclient->client = c;
		list_add(&tagclient->head, &tagclientsmap[tagindex]->head);
	}

	
	for(i = 0; i < LENGTH(tags); i++){
		int row = i/3;
		int col = i%3;
		if(curtagindex == i){
			if (!list_empty(&tagclientsmap[i]->head))
				drw_setscheme(drw, scheme[SchemeSel]);
			else
				drw_setscheme(drw, scheme[SchemeInvalidSel]);
		}else{
			if (!list_empty(&tagclientsmap[i]->head))
				drw_setscheme(drw, scheme[SchemeNorm]);
			else
				drw_setscheme(drw, scheme[SchemeInvalidNormal]);
		}
		drw_rect(drw, col * ww / 3, row * wh / 3, ww / 3, wh / 3, 1, 1);
		int y = row * wh / 3;
		drw_text(drw, col * ww / 3, row * wh / 3, ww / 3, bh, 10, tags[i], 0);
		y = y + bh;
		struct list_head *head;
		list_for_each(head, &tagclientsmap[i]->head)
		{
			struct TagClient *tc = list_entry(head, struct TagClient,head);
			drw_text(drw, col * ww / 3, y, ww / 3, bh, 30, tc->client->name, 0);
			y = y + bh;
		}
	}

	drw_map(drw,win, 0, 0, ww, wh);
	

	switchercurtagindex = curtagindex;
	

	for (i = 0; i < LENGTH(tags); i++)
	{
		free_list(tagclientsmap[i]);
	}
}

void
drawswitcherbar(Window switcherbarwin, int ww, int wh)
{
	int boxs = drw->fonts->h / 9;
	int boxw = drw->fonts->h / 6 + 2;
	unsigned int i, occ = 0, urg = 0, occt = 0;
	Client *c;
	int n;
	Monitor *m = selmon;

	/*XMoveResizeWindow(dpy,switcherbarwin, wx,wy, ww, wh);*/

	for (c = m->clients; c; c = c->next) {
		if (ISVISIBLE(c))
			n++;
		occ |= c->tags;
		if(!c->isfloating) occt |= c->tags;
		if (c->isurgent)
			urg |= c->tags;
	}

	int x = 0,w = 0;
	drw_setscheme(drw, scheme[SchemeNorm]);
	drw_rect(drw, x, 0, ww, bh, 1, 1);
	for (i = 0; i < LENGTH(tags); i++) {
		/* Do not draw vacant tags */
		if(!(occ & 1 << i || m->tagset[m->seltags] & 1 << i))
			continue;
		w = TEXTW(tags[i]);
		int colorindex = SchemeNorm;
		if (occt & 1 << i) {
			colorindex = SchemeTiled;
		}
		drw_setscheme(drw, scheme[m->tagset[m->seltags] & 1 << i ? SchemeSel :colorindex]);

		// update tag title
		Client *c;
		for(c = m->stack;c;c = c->snext){
			if (!c->isfloating && (c->tags & (1 << i))) {
				break;
			}
		}
		if (c) {
			char tagname[20];
			int tagnamelen = strlen(tags[i]);
			do {
				tagnamelen ++;
				snprintf(tagname, tagnamelen,"%d-%s",i + 1, c->name);
			}while (TEXTW(tagname) < TEXTW(tags[i]) && tagnamelen < 20);
			drw_text(drw, x, 0, w, bh, lrpad / 2,tagname, urg & 1 << i);
		}else {
			drw_text(drw, x, 0, w, bh, lrpad / 2, tags[i], urg & 1 << i);
		}

		x += w;
	}

	drw_map(drw, switcherbarwin, 0, 0, ww, wh);
}

Client * 
switcherbaraction(int rx, int ry)
{
	Monitor *m = selmon;
	if(ry >0 && ry < selmon->switcherbarwh){
		int x = 0, i = 0;
		Client *c;
		unsigned int occ = 0;
		for(c = m->clients; c; c=c->next)
			occ |= c->tags;
		do {
			/* Do not reserve space for vacant tags */
			if (!(occ & 1 << i || m->tagset[m->seltags] & 1 << i))
				continue;
			x += TEXTW(tags[i]);
		} while (rx >= x && ++i < LENGTH(tags));
		if (i < LENGTH(tags) && 1 << i != selmon->tagset[selmon->seltags]) {
			const Arg arg = {.ui = 1 << i};
			view(&arg);
			m->switcherbaraction.drawfunc(m->switcherbarwin, m->switcherbarww, m->switcherbarwh);
			m->switcheraction.drawfunc(m->switcher, m->switcherww, m->switcherwh);
		}else{
			x += blw;

			for(i = 0; i < LENGTH(launchers); i++) {
				x += TEXTW(launchers[i].name);
			}

			Client *vc;
			for (vc = selmon->clients; vc; vc = vc->next) {
				if (ISVISIBLE(vc)) {
					x += vc->titlew;
				}
			}
		}
	}
	return NULL;
}

// ------------------ switcher --------------------
// 实际坐标转化为switcher 中的相对坐标
void
clientxy2switcherxy(XY cxys[], int n, XY sxys[])
{

	int ww = selmon->switcherww;
	int wh = selmon->switcherwh;
	int i;
	Client *c;
	int maxw = 0;
	int maxh = 0;
	int maxx = INT_MIN;
	int minx = INT_MAX;
	int maxy = INT_MIN;
	int miny = INT_MAX;
	for (c = nexttiled(selmon->clients); c; c = nexttiled(c->next))
	{
		maxx = MAX(c->x, maxx);
		if (maxx == c->x) maxw = c->w;
		maxy = MAX(c->y, maxy);
		if (maxy == c->y) maxh = c->h;
		minx = MIN(c->x, minx);
		miny = MIN(c->y, miny);
	}
	float factor = MIN(1.0*ww/(maxx-minx+maxw),1.0*wh/(maxy-miny+maxh));
	int offsetx = - factor * minx;
	int offsety = - factor * miny;

	for(i = 0; i<n; i++)
	{
		sxys[i].x = (cxys[i].x - minx) * factor;
		sxys[i].y = (cxys[i].y - miny) * factor;
	}
}

// switcher 中的相对坐标转化为实际坐标
void
switcherxy2clientxy(XY sxys[], int n, XY cxys[])
{

	int ww = selmon->switcherww;
	int wh = selmon->switcherwh;
	int i;
	Client *c;
	int maxw = 0;
	int maxh = 0;
	int maxx = INT_MIN;
	int minx = INT_MAX;
	int maxy = INT_MIN;
	int miny = INT_MAX;
	for (c = nexttiled(selmon->clients); c; c = nexttiled(c->next))
	{
		maxx = MAX(c->x, maxx);
		if (maxx == c->x) maxw = c->w;
		maxy = MAX(c->y, maxy);
		if (maxy == c->y) maxh = c->h;
		minx = MIN(c->x, minx);
		miny = MIN(c->y, miny);
	}
	float factor = MIN(1.0*ww/(maxx-minx+maxw),1.0*wh/(maxy-miny+maxh));
	int offsetx = - factor * minx;
	int offsety = - factor * miny;

	for(i = 0; i<n; i++)
	{
		cxys[i].x = sxys[i].x/factor+minx;
		cxys[i].y = sxys[i].y/factor+miny;
	}
}
void
drawclientswitcherwinx(Window win, int ww, int wh)
{
	drw_setscheme(drw, scheme[SchemeNorm]);
	drw_rect(drw, 0, 0, ww, wh, 1, 1);	

	Client *c;
	int n = 0;
	for (c = nexttiled(selmon->clients); c; c = nexttiled(c->next)) n++;
	XY cxys[n];
	XY cxyse[n];
	int i;
	for (c = nexttiled(selmon->clients), i=0; c; c = nexttiled(c->next), i++)
	{
		cxys[i].x = c->x;
		cxys[i].y = c->y;
		cxyse[i].x = c->x + c->w;
		cxyse[i].y = c->y + c->h;
	}

	XY sxys[n];
	XY sxyse[n];
	clientxy2switcherxy(cxys, n, sxys);
	clientxy2switcherxy(cxyse, n, sxyse);
	for (c = nexttiled(selmon->clients), i=0; c; c = nexttiled(c->next),i++)
	{
		int x, y, w, h;
		x = sxys[i].x;
		y = sxys[i].y;
		w = sxyse[i].x - sxys[i].x;
		h = sxyse[i].y - sxys[i].y;
		if (c->isdoublepagemarked) {
			drw_setscheme(drw, scheme[SchemeSel]);
			drw_rect(drw, x, y, w, h, 1, 1);
			x = x+1;
			y = y+1;
			w = w-2;
			h = h-2;
		}
		if (c == selmon->sel) {
			drw_setscheme(drw, scheme[SchemeSel]);
		}else {
			drw_setscheme(drw, scheme[SchemeNorm]);
		}
		drw_rect(drw, x, y, w, h, 1, 1);
		int size_level = 1;
		if(c->icons[size_level]){
			drw_pic(drw, x+w/2-c->icws[size_level]/2, y+h/2-c->ichs[size_level], c->icws[size_level], c->ichs[size_level], c->icons[size_level]);
		}else{
			drw_text(drw, x+w/2-TEXTW(c->class)/2, y+h/2-bh, TEXTW(c->class), bh, 0, c->class, 0);
		}
		drw_text(drw, x, y+h/2, w, bh, 30, c->name, 0);
	}
}

Client *
sxy2client(int rx, int ry)
{
	XY sxys[] = {{rx, ry}};
	XY cxys[1];
	int tagindexout[1];
	selmon->switcheraction.switcherxy2xy(sxys, 1, cxys, tagindexout);

	Client *c;
	for (c = nexttiled(selmon->clients); c; c = nexttiled(c->next))
	{
		if (cxys[0].x > c->x && cxys[0].x < c->x + c->w && cxys[0].y > c->y && cxys[0].y < c->y + c->h && c != selmon->sel)
		{
			break;
		}
	}
	return c;
}

void 
clientswitchermove(const Arg *arg)
{
	focusgrid5(arg);
	int ww = selmon->switcherww;
	int wh = selmon->switcherwh;
	drawclientswitcherwin(selmon->switcher, ww, wh);
	XMapWindow(dpy, selmon->switcher);
	XSetInputFocus(dpy, selmon->switcher, RevertToPointerRoot, 0);
}

// ------------------ switcher end  --------------------

// ------------------ switcher vertical  --------------------
void
drawclientswitcherwinvertical(Window win, int ww, int wh)
{
	drw_setscheme(drw, scheme[SchemeNorm]);
	drw_rect(drw, 0, 0, ww, wh, 1, 1);	

	int n = 0;
	Client *c;
	for (c = nexttiled(selmon->clients); c; c = nexttiled(c->next))
		n++;

	int itemh = wh/n;
	int itemw = ww;
	int i = 0;
	for (c = nexttiled(selmon->clients); c; c = nexttiled(c->next))
	{
		int x, y, w, h;
		x = 0;
		y = itemh * i;
		w = itemw;
		h = itemh;
		if (c == selmon->sel) {
			drw_setscheme(drw, scheme[SchemeSel]);
		}else {
			drw_setscheme(drw, scheme[SchemeNorm]);
		}
		drw_rect(drw, x, y, w, h, 1, 1);
		int size_level = 0;
		if(c->icons[size_level]){
			drw_pic(drw, x+w/2-c->icws[size_level]/2, y+h/2-c->ichs[size_level], c->icws[size_level], c->ichs[size_level], c->icons[size_level]);
		}else{
			drw_text(drw, x, y+h/2-bh, w, bh, 30, c->class, 0);
		}
		drw_text(drw, x, y+h/2, w, bh, 30, c->name, 0);
		i++;
	}

	drw_map(drw,win, 0, 0, ww, wh);
}


void 
clientswitcheractionvertical(int rx, int ry)
{
	int n = 0;
	Client *c;
	for (c = nexttiled(selmon->clients); c; c = nexttiled(c->next))
		n++;

	int ww = selmon->switcherww;
	int wh = selmon->switcherwh;
	int itemh = wh/n;
	int itemw = ww;
	int i = 0;
	for (c = nexttiled(selmon->clients); c; c = nexttiled(c->next))
	{
		int x, y, w, h;
		x = 0;
		y = itemh * i;
		w = itemw;
		h = itemh;
		if (rx > x && rx < (x+w) && ry > y && ry < (y+h) && c != selmon->sel) {
			focus(c);
			arrange(selmon);
			selmon->switcheraction.drawfunc(selmon->switcher, ww, wh);
			XMapWindow(dpy, selmon->switcher);
			/*XSetInputFocus(dpy, selmon->switcher, RevertToPointerRoot, 0);*/
			break;
		}
		i++;
	}
}

int
matrixmove(const Arg *arg,int selindex, int row, int col, int result[])
{
	int arr[row][col];
	int i;
	int j;
	for(i=0;i<row;i++)
	{
		for(j=0;j<col;j++){
			arr[i][j] = 1;
		}
	}
	int x = selindex/col;
	int y = selindex%col;
	result[0] = x;
	result[1] = y;
	if (arg->i == 1) {
		right(arr, row, col, x, y, result);
		selindex = result[0] * col + result[1];
	}
	if (arg->i == -1) {
		left(arr, row, col, x, y, result);
		selindex = result[0] * col + result[1];
	}
	if (arg->i == 2) {
		up(arr, row, col, x, y, result);
		selindex = result[0] * col + result[1];
	}
	if (arg->i == -2) {
		down(arr, row, col, x, y, result);
		selindex = result[0] * col + result[1];
	}
	LOG_FORMAT("maxtrix move %d %d %d", arg->i, result[0], result[1]);
	return selindex;
}

void 
clientswitchermovevertical(const Arg *arg)
{
	LOG_FORMAT("clientswitchermovevertical 1");
	int selindex = 0, n = 0;
	Client *c;
	for (c = nexttiled(selmon->clients); c; c = nexttiled(c->next)) n++;
	for (c = nexttiled(selmon->clients); c; c = nexttiled(c->next))
	{
		if (selmon->sel == c) {
			break;
		}
		selindex++;
	}

	int row = n;
	int col = 1;
	int result[2];
	selindex = matrixmove(arg, selindex, row, col, result);
	LOG_FORMAT("clientswitchermovevertical 2 %d", selindex);
	int i = 0;
	for (c = nexttiled(selmon->clients); c; c = nexttiled(c->next))
	{
		if (i == selindex) {
			focus(c);
			arrange(selmon);
			break;
		}
		i++;
	}
	int ww = selmon->switcherww;
	int wh = selmon->switcherwh;
	selmon->switcheraction.drawfunc(selmon->switcher, ww, wh);
	XMapWindow(dpy, selmon->switcher);
	/*XSetInputFocus(dpy, selmon->switcher, RevertToPointerRoot, 0);*/
}

// ------------------ switcher vertical end  --------------------


// ------------------ switcher vertical sticky --------------------
void
drawclientswitcherwinverticalsticky(Window win, int ww, int wh)
{
	drw_setscheme(drw, scheme[SchemeNorm]);
	drw_rect(drw, 0, 0, ww, wh, 1, 1);	

	int n = 0;
	Client *c;
	for (c = selmon->clients; c; c = c->next)
		if (!c->isfloating)
			n++;
	if (n == 0)
	{
		drw_map(drw,win, 0, 0, ww, wh);
		return;
	}
	int itemh = wh/n;
	int itemw = ww;
	int i = 0;
	for (c = selmon->clients; c; c = c->next)
	{
		if (c->isfloating) continue;
		int x, y, w, h;
		x = 0;
		y = itemh * i;
		w = itemw;
		h = itemh;
		if (c == selmon->sel) {
			drw_setscheme(drw, scheme[SchemeSel]);
		}else {
			drw_setscheme(drw, scheme[SchemeNorm]);
		}
		drw_rect(drw, x, y, w, h, 1, 1);
		drw_line(drw, x, y, x+w,y, 0);
		drw_line(drw, x, y+h, x+w,y+h, 0);
		int size_level = 0;
		if(c->icons[size_level]){
			drw_pic(drw, x+w/2-c->icws[size_level]/2, y+h/2-c->ichs[size_level], c->icws[size_level], c->ichs[size_level], c->icons[size_level]);
		}else{
			drw_text(drw, x, y+h/2-bh, w, bh, 30, c->class, 0);
		}
		drw_text(drw, x, y+h/2, w, bh, 30, c->name, 0);
		i++;
	}

	drw_map(drw,win, 0, 0, ww, wh);
}


Client *
sxy2clientxysticky(int rx, int ry)
{
	int n = 0;
	Client *c;
	for (c = selmon->clients; c; c = c->next)
		if (!c->isfloating)
			n++;

	int ww = selmon->switcherstickyww;
	int wh = selmon->switcherstickywh;
	int itemh = wh/n;
	int itemw = ww;
	int i = 0;
	for (c = selmon->clients; c; c = c->next)
	{
		if (c->isfloating) continue;
		int x, y, w, h;
		x = 0;
		y = itemh * i;
		w = itemw;
		h = itemh;
		if (rx > x && rx < (x+w) && ry > y && ry < (y+h)) {
			return c;
		}
		i++;
	}
	return NULL;
}

void 
clientswitcheractionverticalsticky(int rx, int ry)
{
	int ww = selmon->switcherstickyww;
	int wh = selmon->switcherstickywh;
	Client *c = sxy2clientxysticky(rx, ry);
	if (!c) return;
	if (selmon->sel && c == selmon->sel) return;
	if(c->tags != selmon->tagset[selmon->seltags]) 
		viewui(c->tags);
	focus(c);
	arrange(selmon);
	selmon->switcherstickyaction.drawfunc(selmon->switcherstickywin, ww, wh);
	XMapWindow(dpy, selmon->switcher);
}


void 
clientswitchermoveverticalsticky(const Arg *arg)
{
	LOG_FORMAT("clientswitchermovevertical 1");
	int selindex = 0, n = 0;
	Client *c;
	for (c = selmon->clients; c; c = c->next)
		if (!c->isfloating)
			n++;
	for (c = selmon->clients; c; c = c->next)
	{
		if (c->isfloating)
			continue;
		if (selmon->sel == c) {
			break;
		}
		selindex++;
	}

	int row = n;
	int col = 1;
	int result[2];
	selindex = matrixmove(arg, selindex, row, col, result);
	LOG_FORMAT("clientswitchermovevertical 2 %d", selindex);
	int i = 0;
	for (c = selmon->clients; c; c = c->next)
	{
		if (c->isfloating)
			continue;
		if (i == selindex) {
			if(c->tags != selmon->tagset[selmon->seltags]) 
				viewui(c->tags);
			focus(c);
			arrange(selmon);
			break;
		}
		i++;
	}
	int ww = selmon->switcherstickyww;
	int wh = selmon->switcherstickywh;
	selmon->switcherstickyaction.drawfunc(selmon->switcherstickywin, ww, wh);
	XMapWindow(dpy, selmon->switcher);
	/*XSetInputFocus(dpy, selmon->switcher, RevertToPointerRoot, 0);*/
}

// ------------------ switcher vertical sticky end  --------------------

// ------------------ switcher common  --------------------

void
clientswitcheraction(int rx, int ry)
{
	if (!selmon->switcher) return;
	int ww = selmon->switcherww;
	int wh = selmon->switcherwh;
	Client *c = selmon->switcheraction.sxy2client(rx, ry);
	if (c && c != selmon->sel) {
		if(c->tags != selmon->tagset[selmon->seltags]) 
			viewui(c->tags);
		focus(c);
		arrange(selmon);
		selmon->switcheraction.drawfunc(selmon->switcher, ww, wh);
		XMapWindow(dpy, selmon->switcher);
		XSetInputFocus(dpy, selmon->switcher, RevertToPointerRoot, 0);
	}
}

void
drawclientswitcherwin(Window win, int ww, int wh)
{
	selmon->switcheraction.drawfuncx(win, ww, wh);
	drw_map(drw,win, 0, 0, ww, wh);
}


// ------------------ switcher common end --------------------


// ------------------ switcher tag client --------------------

void clientxy2switcherxy_pertag_one(XY cxys[], int n, XY sxys[], int s2t[], int tagindexin, int tagsx, int tagsy, int tagsww, int tagswh){
	int ww = tagsww;
	int wh = tagswh;
	int tmaxw = 0;
	int tmaxh = 0;
	int tmaxx = INT_MIN;
	int tminx = INT_MAX;
	int tmaxy = INT_MIN;
	int tminy = INT_MAX;
	unsigned int tags = 1 << tagindexin;
	Client *c;
	for (c = selmon->clients; c; c = c->next)
	{
		if ((c->tags & tags) == 0)
		{
			continue;
		}
		tmaxx = MAX(c->x, tmaxx);
		if (tmaxx == c->x)
			tmaxw = c->w;
		tmaxy = MAX(c->y, tmaxy);
		if (tmaxy == c->y)
			tmaxh = c->h;
		tminx = MIN(c->x, tminx);
		tminy = MIN(c->y, tminy);
	}
	float tfactor = MIN(1.0 * ww / (tmaxx - tminx + tmaxw), 1.0 * wh / (tmaxy - tminy + tmaxh));
	int toffsetx = -tfactor * tminx;
	int toffsety = -tfactor * tminy;
	int i;
	for (i = 0; i < n; i++)
	{
		int tagindex = s2t[i];
		if(tagindex != tagindexin) continue;
		sxys[i].x = (cxys[i].x - tminx) * tfactor + tagsx;
		sxys[i].y = (cxys[i].y - tminy) * tfactor + tagsy;
	}
}


// 实际坐标转化为switcher 中的相对坐标
void clientxy2switcherxy_pertag2(XY cxys[], int n, XY sxys[], int tagindexin[], int tagn, int tagsx[], int tagsy[], int tagsww[], int tagswh[])
{
	int k;
	for (k = 0; k < tagn; k++)
	{
		clientxy2switcherxy_pertag_one(cxys, n, sxys, tagindexin, k, tagsx[k], tagsy[k], tagsww[k], tagswh[k]);
	}
}

// 实际坐标转化为switcher 中的相对坐标
/**
 * @brief 
 * 
 * @param cxys 
 * @param n 
 * @param sxys 
 * @param s2t 长度是LENGTH(tags) xys -> tagindex
 * @param tagn LENGTH(tags)
 * @param tagsx LENGTH(tags)
 * @param tagsy LENGTH(tags)
 * @param tagsww LENGTH(tags)
 * @param tagswh LENGTH(tags)
 */
void clientxy2switcherxy_pertag(XY cxys[], int n, XY sxys[], int s2t[], int tagn, int tagsx[], int tagsy[], int tagsww[], int tagswh[])
{
	
	float factor[tagn];
	int minx[tagn];
	int miny[tagn];

	int k;
	for (k = 0; k < tagn; k++)
	{
		int tagindex = k;
		int ww = tagsww[k];
		int wh = tagswh[k];
		int tmaxw = 0;
		int tmaxh = 0;
		int tmaxx = INT_MIN;
		int tminx = INT_MAX;
		int tmaxy = INT_MIN;
		int tminy = INT_MAX;
		unsigned int tags = 1 << tagindex;
		Client *c;
		for (c = selmon->clients; c; c = c->next)
		{
			if ((c->tags & tags) == 0)
			{
				continue;
			}
			tmaxx = MAX(c->x, tmaxx);
			if (tmaxx == c->x)
				tmaxw = c->w;
			tmaxy = MAX(c->y, tmaxy);
			if (tmaxy == c->y)
				tmaxh = c->h;
			tminx = MIN(c->x, tminx);
			tminy = MIN(c->y, tminy);
		}
		float tfactor = MIN(1.0 * ww / (tmaxx - tminx + tmaxw), 1.0 * wh / (tmaxy - tminy + tmaxh));
		int toffsetx = -tfactor * tminx;
		int toffsety = -tfactor * tminy;

		factor[k] = tfactor;
		minx[k] = tminx;
		miny[k] = tminy;
	}
	
	int i;
	for (i = 0; i < n; i++)
	{
		int tagindex = s2t[i];
		sxys[i].x = (cxys[i].x - minx[tagindex]) * factor[tagindex] + tagsx[tagindex];
		sxys[i].y = (cxys[i].y - miny[tagindex]) * factor[tagindex] + tagsy[tagindex];
	}
}

int getvalidtagn(int *validtagn, int tagi2t[])
{
	unsigned int occ = 0;
	Client *c;
	for (c = selmon->clients; c; c = c->next) {
		if(c->isfloating) continue;
		occ |= c->tags;
	}
	int n = 0; // n个有client的tag
	int i;
	for(i=0;i<LENGTH(tags);i++)
	{
		if (((occ >> i) & 1) > 0)
			n++;
	}
	*validtagn = n;

	int j = 0;
	for(i=0;i<LENGTH(tags);i++)
	{
		if (((occ >> i) & 1) > 0)
		{
			tagi2t[j] = i;
			j++;
		}
	}
}

// 确定tag块的位置
void switchertagarrange_tag(int ww, int wh, int tagn, int tagsx[], int tagsy[], int tagsww[], int tagswh[], int *validtagn, int tagi2t[])
{
	
	// int i, j;
	// for(i=0;i<3;i++)
	// {
	// 	for(j=0;j<3;j++)
	// 	{
	// 		tagsx[i*3+j] = j*ww/3;
	// 		tagsy[i*3+j] = i*wh/3;
	// 		tagsww[i*3+j] = ww/3;
	// 		tagswh[i*3+j] = wh/3;
	// 	}
	// }
	
	int i;

	getvalidtagn(validtagn, tagi2t);
	int n = *validtagn;

	double cold = sqrt(n);
	int bias = 1;
	if(cold == (int)cold)
		bias = 0;
	int coln = ((int)cold)+bias;
	// int rown = coln;
	double rowd = 1.0 * n / coln;
	int rown = n / coln + (rowd == (int)rowd ? 0 : 1);
	for (i = 0; i < n; i++)
	{
		int t = tagi2t[i];
		tagsww[t] = ww / coln;
		// 按比例
		tagswh[t] = tagsww[t] * selmon->wh / selmon->ww;
		tagsx[t] = i % coln * tagsww[t];
		tagsy[t] = (i / coln) * tagswh[t];
	}
}


void clientxy2switcherxy_tag(XY cxys[], int n, XY sxys[], int tagindexin[])
{
	int ww = selmon->switcherww;
	int wh = selmon->switcherwh;
	int tagn = LENGTH(tags);
	int tagsx[tagn];
	int tagsy[tagn];
	int tagsww[tagn];
	int tagswh[tagn];
	int validtagn = tagn;
	int tagi2t[validtagn];
	switchertagarrange_tag(ww, wh, tagn, tagsx, tagsy, tagsww, tagswh, &validtagn, tagi2t);
	clientxy2switcherxy_pertag(cxys, n, sxys, tagindexin, tagn, tagsx, tagsy, tagsww, tagswh);
}

// switcher 中的相对坐标转化为实际坐标
void switcherxy2clientxy_pertag(XY sxys[], int n, XY cxys[], int tagindexout[], int tagn, int tagsx[], int tagsy[], int tagsww[], int tagswh[])
{
	int i,j;
	Client *c;

    	XY tsxys[n];
	int s2t[n]; // sxy->tagindex
	memset(s2t, 0, sizeof(s2t));
	for (j = 0; j < n; j++)
	{
		for (i = 0; i < tagn; i++)
		{
			if(sxys[j].x > tagsx[i] && sxys[j].x <= tagsx[i]+tagsww[i] && sxys[j].y > tagsy[i] && sxys[j].y <= tagsy[i]+tagswh[i])
			{
				tsxys[j].x = sxys[j].x - tagsx[i];
				tsxys[j].y = sxys[j].y - tagsy[i];
				s2t[j] = i;
				break;
			}
		}
	}

	float tfactor[tagn];
	int tminx[tagn];
	int tminy[tagn];
	for (i = 0; i < tagn; i++)
	{
		int ww = tagsww[i];
		int wh = tagswh[i];

		int maxw = 0;
		int maxh = 0;
		int maxx = INT_MIN;
		int minx = INT_MAX;
		int maxy = INT_MIN;
		int miny = INT_MAX;
		for (c = selmon->clients; c; c = c->next)
		{
			if ((c->tags & (1 << i)) == 0)
			{
				continue;
			}
			maxx = MAX(c->x, maxx);
			if (maxx == c->x)
				maxw = c->w;
			maxy = MAX(c->y, maxy);
			if (maxy == c->y)
				maxh = c->h;
			minx = MIN(c->x, minx);
			miny = MIN(c->y, miny);
		}
		float factor = MIN(1.0 * ww / (maxx - minx + maxw), 1.0 * wh / (maxy - miny + maxh));
		tfactor[i] = factor;
		tminx[i] = minx;
		tminy[i] = miny;
	}

	for (i = 0; i < n; i++)
	{
		float factor = tfactor[s2t[i]];
		int minx = tminx[s2t[i]];
		int miny = tminy[s2t[i]];
		if(minx == INT_MIN || miny == INT_MIN)
		{
			LOG_FORMAT("switcherxy2clientxy_pertag 1");
		}
		cxys[i].x = tsxys[i].x / factor + minx;
		cxys[i].y = tsxys[i].y / factor + miny;
		tagindexout[i] = s2t[i];
	}
}
// switcher 中的相对坐标转化为实际坐标
void switcherxy2clientxy_tag(XY sxys[], int n, XY cxys[], int tagindexout[])
{
	int ww = selmon->switcherww;
	int wh = selmon->switcherwh;
	int tagn = LENGTH(tags);
	int tagsx[tagn];
	int tagsy[tagn];
	int tagsww[tagn];
	int tagswh[tagn];
	int validtagn = tagn;
	int tagi2t[validtagn];
	switchertagarrange_tag(ww, wh, tagn, tagsx, tagsy, tagsww, tagswh, &validtagn, tagi2t);
	switcherxy2clientxy_pertag(sxys, n, cxys,tagindexout,tagn, tagsx, tagsy, tagsww, tagswh);
}

void drawclientswitcherwinx_pretag(Window win, int tagindex, int tagsx, int tagsy, int tagsww, int tagswh)
{
	unsigned int tags = 1<<tagindex;
	drw_setscheme(drw, scheme[SchemeNorm]);
	drw_rect(drw, tagsx, tagsy, tagsww, tagswh, 1, 1);

	Client *c;
	int n = 0;
	for (c = selmon->clients; c; c = c->next)
	{
		if ((c->tags & tags) == 0)
			continue;
		n++;
	}
	XY cxys[n];
	XY cxyse[n];
	int s2t[n];
	int i;
	for (c = selmon->clients, i = 0; c; c = c->next)
	{
		if ((c->tags & tags) == 0)
			continue;
		cxys[i].x = c->x;
		cxys[i].y = c->y;
		cxyse[i].x = c->x + c->w;
		cxyse[i].y = c->y + c->h;
		s2t[i] = tagindex;
		i++;
	}

	XY sxys[n];
	XY sxyse[n];
	clientxy2switcherxy_pertag_one(cxys, n, sxys, s2t, tagindex, tagsx, tagsy, tagsww, tagswh);
	clientxy2switcherxy_pertag_one(cxyse, n, sxyse,s2t, tagindex, tagsx, tagsy, tagsww, tagswh);
	for (c = selmon->clients, i = 0; c; c = c->next)
	{
		if ((c->tags & tags) == 0)
			continue;
		int x, y, w, h;
		x = sxys[i].x;
		y = sxys[i].y;
		w = sxyse[i].x - sxys[i].x;
		h = sxyse[i].y - sxys[i].y;
		if (c->isdoublepagemarked)
		{
			drw_setscheme(drw, scheme[SchemeSel]);
			drw_rect(drw, x, y, w, h, 1, 1);
			x = x + 1;
			y = y + 1;
			w = w - 2;
			h = h - 2;
		}
		if (c == selmon->sel)
		{
			drw_setscheme(drw, scheme[SchemeSel]);
		}
		else
		{
			drw_setscheme(drw, scheme[SchemeNorm]);
		}
		drw_rect(drw, x, y, w, h, 1, 1);
		int size_level = 1;
		if (c->ichs[size_level] > h/2) {
			size_level = 0;
		}
		if (c->icons[size_level])
		{
			drw_pic(drw, x + w / 2 - c->icws[size_level] / 2, y + h / 2 - c->ichs[size_level], c->icws[size_level], c->ichs[size_level], c->icons[size_level]);
		}
		else
		{
			int tw = MIN(TEXTW(c->class), w);
			drw_text(drw, x + w / 2 - tw / 2, y + h / 2 - bh, tw, bh, 0, c->class, 0);
		}
		drw_text(drw, x, y + h / 2, w, bh, 30, c->name, 0);
		drw_line(drw, x, y, x+w,y, 0);
		drw_line(drw, x, y+h, x+w, y+h, 0);
		drw_line(drw, x, y, x,y+h, 0);
		drw_line(drw, x+w, y, x+w,y+h, 0);
		i++;
	}
}

void drawclientswitcherwinx_tag(Window win, int ww, int wh)
{
	int tagn = LENGTH(tags);
	int tagsx[tagn];
	int tagsy[tagn];
	int tagsww[tagn];
	int tagswh[tagn];
	int validtagn = tagn;
	int tagi2t[validtagn];
	switchertagarrange_tag(ww, wh, tagn, tagsx, tagsy, tagsww, tagswh, &validtagn, tagi2t);
	drw_setscheme(drw, scheme[SchemeNorm]);
	drw_rect(drw, 0, 0, ww, wh, 1, 1);
	int i;
	for (i = 0; i < validtagn; i++)
	{
		int tagindex = tagi2t[i];
		drawclientswitcherwinx_pretag(win, tagindex, tagsx[tagindex], tagsy[tagindex], tagsww[tagindex], tagswh[tagindex]);
	}
	
}

Client *
sxy2client_tag(int rx, int ry)
{
	LOG_FORMAT("sxy2client 1, rx:%d,ry:%d", rx, ry);
	XY sxys[] = {{rx, ry}};
	XY cxys[1];
	int tagindexout[1];
	selmon->switcheraction.switcherxy2xy(sxys, 1, cxys, tagindexout);

	Client *found = NULL;
	Client *c;
	for (c = selmon->clients; c; c = c->next)
	{
		if ((c->tags & (1<<tagindexout[0])) == 0) continue;
		if (cxys[0].x > c->x && cxys[0].x < c->x + c->w && cxys[0].y > c->y && cxys[0].y < c->y + c->h)
		{
			found = c;
			break;
		}
	}
	return found;
}

int 
distancexy(XY xy1, XY xy2)
{
	return pow(pow(xy1.x - xy2.x,2) + pow(xy1.y-xy2.y,2),0.5);
}

int
nextclosestxy(const Arg *arg, int n, XY xys[], int curi)
{
	int i;
	int closest = 0;
	if (arg->i == FOCUS_LEFT) {
		int min = INT_MAX;
		for (i=0;i<n;i++)
		{
			XY xy = xys[i];
			XY curxy = xys[curi];
			if (i!=curi && xy.x < curxy.x)
			{
				int dist = distancexy(xy, curxy);
				if (min > distancexy(xy,curxy)) {
					closest = i;
					min = dist;
				}
			}
		}
	}
	else if (arg->i == FOCUS_RIGHT) {
		int min = INT_MAX;
		for (i = 0; i < n; i++)
		{
			XY xy = xys[i];
			XY curxy = xys[curi];
			if (i != curi && xy.x > curxy.x)
			{
				int dist = distancexy(xy, curxy);
				if (min > distancexy(xy, curxy))
				{
					closest = i;
					min = dist;
				}
			}
		}
	}
	else if (arg->i == FOCUS_UP)
	{
		int min = INT_MAX;
		for (i = 0; i < n; i++)
		{
			XY xy = xys[i];
			XY curxy = xys[curi];
			if (i != curi && xy.y < curxy.y)
			{
				int dist = distancexy(xy, curxy);
				if (min > distancexy(xy, curxy))
				{
					closest = i;
					min = dist;
				}
			}
		}
	}
	else if (arg->i == FOCUS_DOWN)
	{
		int min = INT_MAX;
		for (i = 0; i < n; i++)
		{
			XY xy = xys[i];
			XY curxy = xys[curi];
			if (i != curi && xy.y > curxy.y)
			{
				int dist = distancexy(xy, curxy);
				if (min > distancexy(xy, curxy))
				{
					closest = i;
					min = dist;
				}
			}
		}
	}
	return closest;
}

void 
clientswitchermove_tag(const Arg *arg)
{
	int n = 0;
	Client *c;
	for(c = selmon->clients;c;c=c->next)
	{
		if(!c->isfloating) n++;
	}

	int i=0;

	XY cxys2[4*n+1];
	XY sxys2[4*n+1];
	int tagindexin2[4*n+1];
	int curi2 = 0;
	int MY_INT_MAX = 50000;
	int bias = 4;
	for(i=0,c = selmon->clients;c;c=c->next)
	{
		if(!c->isfloating){
			if(selmon->sel == c){
				cxys2[i].x = MY_INT_MAX;
				cxys2[i].y = MY_INT_MAX;
				cxys2[i+1].x = MY_INT_MAX;
				cxys2[i+1].y = MY_INT_MAX;
				cxys2[i+2].x = MY_INT_MAX;
				cxys2[i+2].y = MY_INT_MAX;
				cxys2[i+3].x = MY_INT_MAX;
				cxys2[i+3].y = MY_INT_MAX;
				tagindexin2[i] = gettagindex(c->tags);
				tagindexin2[i+1] = gettagindex(c->tags);
				tagindexin2[i+2] = gettagindex(c->tags);
				tagindexin2[i+3] = gettagindex(c->tags);
				i+=4;
				continue;
			}
			cxys2[i].x = c->x + bias;
			cxys2[i].y = c->y + bias;
			cxys2[i+1].x = c->x + c->w - bias;
			cxys2[i+1].y = c->y + bias;
			cxys2[i+2].x = c->x + c->w - bias;
			cxys2[i+2].y = c->y + c->h - bias;
			cxys2[i+3].x = c->x + bias;
			cxys2[i+3].y = c->y + c->h - bias;
			tagindexin2[i] = gettagindex(c->tags);
			tagindexin2[i+1] = gettagindex(c->tags);
			tagindexin2[i+2] = gettagindex(c->tags);
			tagindexin2[i+3] = gettagindex(c->tags);
			i+=4;
		}
	}
	curi2 = 4*n;
	if (arg->i == FOCUS_LEFT) 
	{
		cxys2[curi2].x = selmon->sel->x + bias;
		cxys2[curi2].y = selmon->sel->y + selmon->sel->h/2;
	}
	else if (arg->i == FOCUS_RIGHT) 
	{
		cxys2[curi2].x = selmon->sel->x + selmon->sel->w - bias;
		cxys2[curi2].y = selmon->sel->y + selmon->sel->h/2;
	}
	else if (arg->i == FOCUS_UP)
	{
		cxys2[curi2].x = selmon->sel->x + selmon->sel->w/2;
		cxys2[curi2].y = selmon->sel->y + bias;
	}
	else if (arg->i == FOCUS_DOWN)
	{
		cxys2[curi2].x = selmon->sel->x + selmon->sel->w/2;
		cxys2[curi2].y = selmon->sel->y + selmon->sel->h - bias;
	}
	tagindexin2[curi2] = gettagindex(selmon->sel->tags);
	clientxy2switcherxy_tag(cxys2,4*n+1,sxys2,tagindexin2);
	int closest2 = nextclosestxy(arg, 4*n+1, sxys2, curi2);

	XY cxys[n];
	XY sxys[n];
	int tagindexin[n];
	int curi = 0;
	for(i = 0, c = selmon->clients;c;c=c->next)
	{
		if(!c->isfloating){
			if(selmon->sel == c) curi = i;
			cxys[i].x = c->x + c->w/2;
			cxys[i].y = c->y + c->h/2;
			tagindexin[i] = gettagindex(c->tags);
			i++;
		}
	}

	clientxy2switcherxy_tag(cxys,n,sxys,tagindexin);
	int closest = closest2/4;
	/*int closest = nextclosestxy(arg, n, sxys, curi);*/
	if (closest < 0) return;
	Client *closestc = sxy2client_tag(sxys[closest].x, sxys[closest].y);
	if (closestc) {
		if((closestc->tags & selmon->sel->tags) == 0)
			viewui(closestc->tags);
		focus(closestc);
		arrange(selmon);
	}
	int ww = selmon->switcherww;
	int wh = selmon->switcherwh;
	selmon->switcheraction.drawfunc(selmon->switcher, ww, wh);
	XMapWindow(dpy, selmon->switcher);
	XSetInputFocus(dpy, selmon->switcher, RevertToPointerRoot, 0);
}

int
angley(XY xy1, XY xy2)
{
	// 上下
	return distancexy(xy1,xy2) * abs(xy1.x - xy2.x) / abs(xy1.y - xy2.y) + distancexy(xy1, xy2);
}

int
anglex(XY xy1, XY xy2)
{
	// 左右
	return distancexy(xy1, xy2) * abs(xy1.y - xy2.y) / abs(xy1.x - xy2.x) + distancexy(xy1, xy2);
}

int
nextclosestanglexy(const Arg *arg, int n, XY xys[], int curi)
{
	int i;
	int closest = -1;
	if (arg->i == FOCUS_LEFT) {
		int min = INT_MAX;
		for (i=0;i<n;i++)
		{
			XY xy = xys[i];
			XY curxy = xys[curi];
			if (i!=curi && xy.x < curxy.x)
			{
				int dist = anglex(xy, curxy);
				if (min > anglex(xy,curxy)) {
					closest = i;
					min = dist;
				}
			}
		}
	}
	else if (arg->i == FOCUS_RIGHT) {
		int min = INT_MAX;
		for (i = 0; i < n; i++)
		{
			XY xy = xys[i];
			XY curxy = xys[curi];
			if (i != curi && xy.x > curxy.x)
			{
				int dist = anglex(xy, curxy);
				if (min > anglex(xy, curxy))
				{
					closest = i;
					min = dist;
				}
			}
		}
	}
	else if (arg->i == FOCUS_UP)
	{
		int min = INT_MAX;
		for (i = 0; i < n; i++)
		{
			XY xy = xys[i];
			XY curxy = xys[curi];
			if (i != curi && xy.y < curxy.y)
			{
				int dist = angley(xy, curxy);
				if (min > angley(xy, curxy))
				{
					closest = i;
					min = dist;
				}
			}
		}
	}
	else if (arg->i == FOCUS_DOWN)
	{
		int min = INT_MAX;
		for (i = 0; i < n; i++)
		{
			XY xy = xys[i];
			XY curxy = xys[curi];
			if (i != curi && xy.y > curxy.y)
			{
				int dist = angley(xy, curxy);
				if (min > angley(xy, curxy))
				{
					closest = i;
					min = dist;
				}
			}
		}
	}
	return closest;
}

void 
clientswitchermove_tag2(const Arg *arg)
{
	int n = 0;
	Client *c;
	for(c = selmon->clients;c;c=c->next)
	{
		if(!c->isfloating) n++;
	}

	int i=0;

	XY cxys[n];
	XY sxys[n];
	int tagindexin[n];
	int curi = 0;
	for(i = 0, c = selmon->clients;c;c=c->next)
	{
		if(!c->isfloating){
			if(selmon->sel == c) curi = i;
			cxys[i].x = c->x + c->w/2;
			cxys[i].y = c->y + c->h/2;
			tagindexin[i] = gettagindex(c->tags);
			i++;
		}
	}

	clientxy2switcherxy_tag(cxys,n,sxys,tagindexin);
	int closest = nextclosestanglexy(arg, n, sxys, curi);
	if (closest < 0) return;
	Client *closestc = sxy2client_tag(sxys[closest].x, sxys[closest].y);
	if (closestc) {
		if((closestc->tags & selmon->sel->tags) == 0)
			viewui(closestc->tags);
		focus(closestc);
		arrange(selmon);
	}
	int ww = selmon->switcherww;
	int wh = selmon->switcherwh;
	selmon->switcheraction.drawfunc(selmon->switcher, ww, wh);
	XMapWindow(dpy, selmon->switcher);
	XSetInputFocus(dpy, selmon->switcher, RevertToPointerRoot, 0);
}
// ------------------ switcher tag client end --------------------

void
drawswitcher(Monitor *m)
{
	if(m->switcher) return;
	if(!m->sel) return;
	if(m->sel->isfloating) return;


	XSetWindowAttributes wa = {
		.override_redirect = True,
		.background_pixmap = ParentRelative,
		.event_mask = ButtonPressMask|ExposureMask|KeyPressMask|PointerMotionMask
	};
	
	XClassHint ch = {"dwm", "dwm"};

	m->switcheraction.pointerfunc = clientswitcheraction;
	m->switcheraction.drawfunc = drawclientswitcherwin;

	// m->switcheraction.xy2switcherxy = clientxy2switcherxy;
	// m->switcheraction.switcherxy2xy = switcherxy2clientxy;
	// m->switcheraction.drawfuncx = drawclientswitcherwinx;
	// m->switcheraction.sxy2client = sxy2client;
	// m->switcheraction.movefunc = clientswitchermove;

	m->switcheraction.xy2switcherxy = clientxy2switcherxy_tag;
	m->switcheraction.switcherxy2xy = switcherxy2clientxy_tag;
	m->switcheraction.drawfuncx = drawclientswitcherwinx_tag;
	m->switcheraction.sxy2client = sxy2client_tag;
	m->switcheraction.movefunc = clientswitchermove_tag2;

	int ww = m->ww/2;
	int wh = m->wh/2;

	// -------------- 剪裁空白tag ---------------
	int tagn = LENGTH(tags);
	int validtagn = tagn;
	int tagi2t[validtagn];
	getvalidtagn(&validtagn, tagi2t);
	double zoomf = sqrt(validtagn);
	zoomf = MIN(zoomf,1.6);
	ww = zoomf * ww;
	wh = zoomf * wh;

	int tagsx[tagn];
	int tagsy[tagn];
	int tagsww[tagn];
	int tagswh[tagn];
	switchertagarrange_tag(ww, wh, tagn, tagsx, tagsy, tagsww, tagswh, &validtagn, tagi2t);
	int minx = INT_MAX, miny= INT_MAX, maxx= INT_MIN, maxy= INT_MIN;
	int i;
	for (i = 0; i < validtagn; i++)
	{
		int t = tagi2t[i];
		maxx = MAX(tagsx[t] + tagsww[t], maxx);
		maxy = MAX(tagsy[t] + tagswh[t], maxy);
		minx = MIN(tagsx[t], minx);
		miny = MIN(tagsy[t], miny);
	}
	ww = maxx - minx;
	wh = maxy - miny;
	if(ww == 0 || wh == 0) return;
	// -------------- 剪裁空白tag end ---------------

	m->switcherww = ww;
	m->switcherwh = wh;

	int wx = m->ww/2-ww/2;
	int wy = m->wh/2-wh/2;

	int prx, pry;
	getrootptr(&prx, &pry);
	LOG_FORMAT("prx,pry: %d %d", prx, pry);
	XY cxys[] = {{selmon->sel->x + selmon->sel->w/2, selmon->sel->y + selmon->sel->h/2}};
	XY sxys[1];
	int curtagindex = getcurtagindex(selmon);
	if(curtagindex < 0) return;
	int tagindexin[] = {curtagindex};
	m->switcheraction.xy2switcherxy(cxys, 1, sxys, tagindexin);
	// 多屏校正
	wx = prx - m->wx - sxys[0].x;
	wy = pry - m->wy - sxys[0].y;
	if (wx < 0) wx = 0;
	if (wx > m->ww - m->switcherww) wx = m->ww - m->switcherww;
	if (wy < 0) wy = 0;
	if (wy > m->wh - m->switcherwh) wy = m->wh - m->switcherwh;
	// 多屏校正
	m->switcherwx = wx + m->wx;
	m->switcherwy = wy + m->wy;
	m->switcher = XCreateWindow(dpy, root, m->switcherwx, m->switcherwy, m->switcherww, m->switcherwh, 0, DefaultDepth(dpy, screen),
				CopyFromParent, DefaultVisual(dpy, screen),
				CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
	XDefineCursor(dpy, m->switcher, cursor[CurNormal]->cursor);
	XMapWindow(dpy, m->switcher);
	m->switcheraction.drawfunc(m->switcher, m->switcherww, m->switcherwh);
	XMapRaised(dpy, m->switcher);
	XSetClassHint(dpy, m->switcher, &ch);
	XSetInputFocus(dpy, m->switcher, RevertToPointerRoot, 0);

	/*m->switcherbarww = ww;*/
	/*m->switcherbarwh = bh;*/
	/*m->switcherbarwx = wx;*/
	/*m->switcherbarwy = wy-bh;*/
	/*m->switcherbaraction.drawfunc = drawswitcherbar;*/
	/*m->switcherbaraction.movefunc = NULL;*/
	/*m->switcherbaraction.pointerfunc = switcherbaraction;*/
	/*m->switcherbarwin = XCreateWindow(dpy, root, m->switcherbarwx, m->switcherbarwy, m->switcherbarww, m->switcherbarwh, 0, DefaultDepth(dpy, screen),*/
				/*CopyFromParent, DefaultVisual(dpy, screen),*/
				/*CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);*/
	/*XDefineCursor(dpy, m->switcherbarwin, cursor[CurNormal]->cursor);*/
	/*XMapWindow(dpy, m->switcherbarwin);*/
	/*m->switcherbaraction.drawfunc(m->switcherbarwin, m->switcherbarww, m->switcherbarwh);*/
	/*XMapRaised(dpy, m->switcherbarwin);*/
	/*XSetClassHint(dpy, m->switcherbarwin, &ch);*/
}

void 
destroyswitcher(Monitor *m)
{
	if(selmon->sel){
		XSetInputFocus(dpy, selmon->sel->win, RevertToPointerRoot, CurrentTime);
	}else{
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
	}
	XUnmapWindow(dpy, m->switcher);
	XDestroyWindow(dpy, m->switcher);
	selmon->switcher = 0L;	

	XUnmapWindow(dpy, m->switcherbarwin);
	XDestroyWindow(dpy, m->switcherbarwin);
	selmon->switcherbarwin = 0L;	
	/*updatesystray();*/
}

void
drawswitchersticky(Monitor *m)
{
	if(m->switcherstickywin) return;
	if(!m->sel) return;

	XSetWindowAttributes wa = {
		.override_redirect = True,
		.background_pixmap = ParentRelative,
		.event_mask = ButtonPressMask|ExposureMask|KeyPressMask|PointerMotionMask
	};
	
	XClassHint ch = {"dwm", "dwm"};

	m->switcherstickyaction.drawfunc = drawclientswitcherwinverticalsticky;
	m->switcherstickyaction.sxy2client = sxy2clientxysticky;
	m->switcherstickyaction.pointerfunc = clientswitcheractionverticalsticky;
	m->switcherstickyaction.movefunc = clientswitchermoveverticalsticky;

	int ww = m->ww - m->ww * tile6initwinfactor + 2;
	int wh = m->wh;

	if (tile6initwinfactor == 1) {
		return;
	}

	m->switcherstickyww = ww;
	m->switcherstickywh = wh;

	int wx = m->ww * tile6initwinfactor;
	int wy = 0;

	m->switcherstickywx = wx;
	m->switcherstickywy = wy;
	m->switcherstickywin = XCreateWindow(dpy, root, m->switcherstickywx, m->switcherstickywy, m->switcherstickyww, m->switcherstickywh, 0, DefaultDepth(dpy, screen),
				CopyFromParent, DefaultVisual(dpy, screen),
				CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
	XDefineCursor(dpy, m->switcherstickywin, cursor[CurNormal]->cursor);
	XMapWindow(dpy, m->switcherstickywin);
	m->switcherstickyaction.drawfunc(m->switcherstickywin, m->switcherstickyww, m->switcherstickywh);
	XMapRaised(dpy, m->switcherstickywin);
	XSetClassHint(dpy, m->switcherstickywin, &ch);
	XSetInputFocus(dpy, m->switcherstickywin, RevertToPointerRoot, 0);
}

void
updateswitchersticky(Monitor *m)
{
	if(m->switcherstickywin)
		m->switcherstickyaction.drawfunc(m->switcherstickywin, m->switcherstickyww, m->switcherstickywh);
}

void 
destroyswitchersticky(Monitor *m)
{
	if (!m->switcherstickywin) {
		return;
	}
	if(selmon->sel){
		XSetInputFocus(dpy, selmon->sel->win, RevertToPointerRoot, CurrentTime);
	}else{
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
	}
	XUnmapWindow(dpy, m->switcherstickywin);
	XDestroyWindow(dpy, m->switcherstickywin);
	selmon->switcherstickywin = 0L;	
}

void
toggleswitchers(const Arg *arg)
{
	if (selmon->switcherstickywin) {
		return;
	}
	ScratchGroup *sg = scratchgroupptr;
	if(sg->isfloating){
		hidescratchgroup(sg);
	}
	if(selmon->switcher){
		destroyswitcher(selmon);
	}else{
		drawswitcher(selmon);
	}
}

void
toggleswitchersticky(const Arg *arg)
{
	if(selmon->switcherstickywin){
		destroyswitchersticky(selmon);
	}else{
		drawswitchersticky(selmon);
	}
}

void 
switchermove(const Arg *arg)
{
	if (selmon->switcherstickywin) {
		if (selmon->switcherstickyaction.movefunc) {
			selmon->switcherstickyaction.movefunc(arg);
			return;
		}
	}
	if (selmon->switcher) {
		if (selmon->switcheraction.movefunc) {
	 		selmon->switcheraction.movefunc(arg);
			return;
		}
	}
	focusgrid5(arg);
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

	/*if (selmon->sel && ev->window != selmon->sel->win) */
	/*{*/
		/*int cx = ev->x_root;*/
		/*int cy = ev->y_root;*/
		/*int offset = 10;*/
		/*if(ev->x_root < selmon->sel->x + selmon->sel->bw) cx = selmon->sel->x + offset;*/
		/*if(ev->x_root > selmon->sel->x + selmon->sel->w - selmon->sel->bw) cx = selmon->sel->x + selmon->sel->w - offset;*/
		/*if(ev->y_root < selmon->sel->y + selmon->sel->bw) cy = selmon->sel->y + offset;*/
		/*if(ev->y_root > selmon->sel->y + selmon->sel->h - selmon->sel->bw) cy = selmon->sel->y + selmon->sel->h - offset;*/
		/*XWarpPointer(dpy, None, root, 0, 0, 0, 0, cx, cy);*/
		/*return;*/
	/*}*/

	LOG_FORMAT("enternotify 1");

	if ((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
		return;
	if (selmon->switcher) {
		return;
	}
	if (selmon->sel && selmon->sel->istemp) {
		return;
	}
	c = wintoclient(ev->window);
	m = c ? c->mon : wintomon(ev->window);
	if (m != selmon) {
		unfocus(selmon->sel, 1);
		selmon = m;
	} else if (!c || c == selmon->sel)
		return;
	
	Client *oldc = selmon->sel;
	if (oldc && oldc->isfloating) {
		focus(c);
		return;
	}
	if (c && c->isfloating) {
		focus(c);
		return;
	}
	if (oldc && oldc->isdoubled && c->isdoubled) {
		return;
	}

	int oldx = c->x;
	int oldy = c->y;
	focus(c);
	/*LOG("enternotify", c?c->name:"");*/
	LOG_FORMAT("enternotify 2");
	/*LOG_FORMAT("enternotify 2 %d %d", ev->x_root, ev->y_root);*/
	arrange(m);

	/*int cursorx = ev->x_root + c->x - oldx;*/
	/*int cursory = ev->y_root + c->y - oldy;*/
	if ((oldc && oldc->isfloating) || (c && c->isfloating)) {
		return;
	}
	int cursorx = c->x + c->w/2;
	int cursory = c->y + c->h/2;
	/*XWarpPointer(dpy, None, root, 0, 0, 0, 0, cursorx, cursory);*/

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
empty(const Arg *arg)
{

}

void
focus(Client *c)
{
	if (!c || !ISVISIBLE(c)){
		for (c = selmon->stack; c; c = c->snext)
			if (c->isfloating && ISVISIBLE(c)) break;
		if (!c) for (c = selmon->stack; c && !ISVISIBLE(c); c = c->snext);
		// NULL then focus master
		/*if (!c) for (c = selmon->clients; c && !ISVISIBLE(c); c = c->next);*/
	}
	if (selmon->sel && selmon->sel != c)
		unfocus(selmon->sel, 0);
	if (c && c->win) {
		if (c->mon != selmon)
			selmon = c->mon;
		if (c->isurgent)
			seturgent(c, 0);
		detachstack(c);
		attachstack(c);
		grabbuttons(c, 1);
		LOG_FORMAT("focus: before setfocus, c->name:%s", c->name);
		setfocus(c);
		selmon->sel = c;
		updateborder(c);
		LOG_FORMAT("focus: after setfocus");

		actionlog("focus", c);
	} else {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
		// Client *t;
		// if (c == c->mon->sel)
		// {
		// 	for (t = c->mon->stack; t && !ISVISIBLE(t); t = t->snext);
		// 	c->mon->sel = t;
		// }
		LOG_FORMAT("focus: c or c->win is NULL");
		// return;
	}
	selmon->sel = c;
	drawbars();
	LOG_FORMAT("focus: over");
}

/* there are some broken focus acquiring clients needing extra handling */
void
focusin(XEvent *e)
{
	XFocusChangeEvent *ev = &e->xfocus;
	if(ev->window == selmon->switcher) {
		XSetInputFocus(dpy, selmon->switcher, RevertToPointerRoot,0);
		return;
	}
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
focusstackarrange(const Arg *arg)
{
	focusstack(arg);
	arrange(selmon);
}

void
focuslast(const Arg *arg)
{
	Client *lastfocused = selmon->sel->lastfocus;
	if (lastfocused && lastfocused->win) {
		focus(lastfocused);
		arrange(selmon);
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

int
avgy(Client *c)
{
	return c->y + c->h / 2;
}

int 
distance(Client *t1, Client *t2)
{
	return pow(pow(avgx(t1) - avgx(t2),2) + pow((avgy(t1) - avgy(t2)),2),0.5);
}

Client *
closestclient(Client *t, Client *c1, Client *c2)
{
	if(t == c1) return c2;
	if(t == c2) return c1;
	if (distance(t, c1) > distance(t, c2))
		return c2;
	else
		return c1;
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
	if(t == c1) return c2;
	if(t == c2) return c1;
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
				}
				else if (abs(cc->x - c->x) == min){
					if((selmon->tagset[selmon->seltags] & TAGMASK) == TAGMASK){
						closest = closestyclient(cc,c, closest);
					}else{
						closest = smartchoose(cc, c, closest);
					}
				}
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
				else if (abs(cc->x - c->x) == min){
					if((selmon->tagset[selmon->seltags] & TAGMASK) == TAGMASK){
						closest = closestyclient(cc,c, closest);
					}else{
						closest = smartchoose(cc, c, closest);
					}
				}
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
				int dist = distance(cc, c);
				if (min > distance(cc, c)) {
					closest = c;
					min = dist;
				}
				/*if (abs(cc->y - c->y) < min && (cc->x - c->x <= cc->bw + c->bw || cc->x + cc->w - (c->x + c->w) <= cc->bw + c->bw))*/
				/*{*/
					/*min = abs(cc->y - c->y);*/
					/*closest = c;*/
				/*}*/
				/*else if (abs(cc->y - c->y) == min)*/
					/*closest = closestxgravityclient(cc, c, closest);*/
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
				int dist = distance(cc, c);
				if (min > distance(cc, c)) {
					closest = c;
					min = dist;
				}
				/*if (abs(cc->y - c->y) < min && (cc->x - c->x <= cc->bw + c->bw || cc->x + cc->w - (c->x + c->w) <= cc->bw + c->bw))*/
				/*{*/
					/*min = abs(cc->y - c->y);*/
					/*closest = c;*/
				/*}*/
				/*else if (abs(cc->y - c->y) == min)*/
					/*closest = closestxgravityclient(cc, c, closest);*/
			}
		}
		c = closest;
	}
	if(cc && selmon->lt[selmon->sellt] == &layouts[1] && !scratchgroupptr->isfloating){
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

Client *
nextclosestc(const Arg *arg)
{
	Client *c = NULL;
	Client *cc = selmon->sel;

	if (!selmon->sel || (selmon->sel->isfullscreen && lockfullscreen)) return NULL;
	if (arg->i == FOCUS_LEFT) {
		Client *closest = NULL;
		int min = INT_MAX;
		for (c = selmon->clients; c; c = c->next)
		{
			if (cc && c != cc && c->x < cc->x && ISVISIBLE(c))
			{
				int dist = distance(cc, c);
				if (min > distance(cc, c)) {
					closest = c;
					min = dist;
				}
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
				int dist = distance(cc, c);
				if (min > distance(cc, c)) {
					closest = c;
					min = dist;
				}
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
				int dist = distance(cc, c);
				if (min > distance(cc, c)) {
					closest = c;
					min = dist;
				}
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
				int dist = distance(cc, c);
				if (min > distance(cc, c)) {
					closest = c;
					min = dist;
				}
			}
		}
		c = closest;
	}
	if(cc && selmon->lt[selmon->sellt] == &layouts[1] && !scratchgroupptr->isfloating){
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
	return c;
}

void
focusgrid5(const Arg *arg)
{
	Client *c = nextclosestc(arg);
	if (c) {
		focus(c);
		// try remove this for performance
		/*restack(selmon);*/
		arrange(selmon);
	}
}

void 
swapclient(Client *c1, Client *c2, Monitor *m)
{
	if(!c1 || !c2) return;

	Client *c;
	Client head;
	head.next = m->clients;

	// 排序, fc为第一个, sc为第二个
	Client *fc;
	Client *sc;
	for(c = m->clients; c; c = c->next){
		if(c == c1){
			fc = c1;
			sc = c2;
			break;
		}
		if(c == c2){
			fc = c2;
			sc = c1;
			break;
		}
	}

	Client *fcp = &head;
	Client *scp = &head;
	for(c = m->clients; c; c = c->next){
		if(c->next == fc){
			fcp = c;
		}
		if(c->next == sc){
			scp = c;
		}
	}

	Client *tmp;
	tmp = fcp->next;
	fcp->next = scp->next;
	scp->next = tmp;

	tmp = fc->next;
	fc->next = sc->next;
	sc->next = tmp;

	m->clients = head.next;
}

void
pyswap(const Arg *arg)
{
	if (!selmon->sel) return;
	int cx = selmon->sel->x + selmon->sel->w/2 + 10;
	int cy = selmon->sel->y + selmon->sel->h/2;
	/*int times = selmon->sel->container->cn > 1?selmon->sel->container->cn:1;*/
	int times = 1;
	if (arg->i == FOCUS_RIGHT) {
		cx = selmon->sel->x + selmon->sel->w /2 + selmon->sel->w * times + 10;
		if(selmon->sel->container->cn > 1 && selmon->sel->containerrefc && selmon->sel->x < selmon->sel->containerrefc->x) 
			cx = selmon->sel->containerrefc->x + selmon->sel->containerrefc->w /2;
	}
	if (arg->i == FOCUS_LEFT) {
		cx = selmon->sel->x + selmon->sel->w /2 - selmon->sel->w * times - 10;
		if(selmon->sel->container->cn > 1 && selmon->sel->containerrefc && selmon->sel->x > selmon->sel->containerrefc->x) 
			cx = selmon->sel->containerrefc->x + selmon->sel->containerrefc->w /2;
	}
	if (arg->i == FOCUS_UP) {
		cy = selmon->sel->y + selmon->sel->h/2 - selmon->sel->h;
	}
	if (arg->i == FOCUS_DOWN) {
		cy = selmon->sel->y + selmon->sel->h/2 + selmon->sel->h;
	}
	XY cxys[] = {{cx, cy}};
	XY sxys[1];
	int curtagindex = getcurtagindex(selmon);
	if(curtagindex < 0) return;
	int tagindexin[] = {curtagindex};
	selmon->switcheraction.xy2switcherxy(cxys, 1, sxys, tagindexin);
	pysmoveclient(selmon->sel, sxys[0].x, sxys[0].y);
	arrange(selmon);
	if (selmon->switcher) {
		selmon->switcheraction.drawfunc(selmon->switcher, selmon->switcherww, selmon->switcherwh);
	}
}

// 在pysort情况下不生效
void
swap(const Arg *arg)
{
	LOG_FORMAT("tracecontainer swap 0 sel:%s %d %d,%d %d,%d", selmon->sel->name, selmon->sel->container->id, selmon->sel->x, selmon->sel->y, selmon->sel->container->x, selmon->sel->container->y);
	pyswap(arg);
	/*Client *cnext;*/
	/*do {*/
		/*cnext = nextclosestc(arg);*/
	/*}while (cnext && cnext->isfloating);*/
	/*swapclient(selmon->sel, cnext, selmon);*/
	/*arrange(selmon);*/
	/*if (selmon->switcher) {*/
		/*selmon->switcheraction.drawfunc(selmon->switcher, selmon->switcherww, selmon->switcherwh);*/
	/*}*/
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
	LOG_FORMAT("keypress");
	unsigned int i;
	KeySym keysym;
	XKeyEvent *ev;

	ev = &e->xkey;
	keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0);
	if(selmon->switcher)
	{
		XSetInputFocus(dpy, selmon->switcher, RevertToPointerRoot, CurrentTime);
		XChangeProperty(dpy, root, netatom[NetActiveWindow],
			XA_WINDOW, 32, PropModeReplace,
			(unsigned char *) &(selmon->switcher), 1);
		XSync(dpy, False);

		for (i = 0; i < LENGTH(switcherkeys); i++)
			if (keysym == switcherkeys[i].keysym
			&& CLEANMASK(switcherkeys[i].mod) == CLEANMASK(ev->state)
			&& switcherkeys[i].func)
				switcherkeys[i].func(&(switcherkeys[i].arg));
	}
	// for tsspawn
	else if(selmon->sel && selmon->sel->istemp && keysym == XK_Escape){
		killclientc(selmon->sel);
	}
	else
	{
		for (i = 0; i < LENGTH(keys); i++)
			if (keysym == keys[i].keysym
			&& CLEANMASK(keys[i].mod) == CLEANMASK(ev->state)
			&& keys[i].func)
				keys[i].func(&(keys[i].arg));
	}

}

void
keyrelease(XEvent *e)
{
	/*LOG_FORMAT("keyrelease");*/
	Client *c = selmon->sel;
	unsigned int i;
	KeySym keysym;
	XKeyEvent *ev;

	ev = &e->xkey;
	keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0);
	if(selmon->switcher && keysym == XK_Super_L)
	{
		if (selmon->sel && selmon->sel->container->cn > 1) {
			XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w /2, c->h /2);
		}
		destroyswitcher(selmon);
	}
}

void 
killclientc(Client* c)
{
	if (!c)
		return;

	if (!sendevent(c->win, wmatom[WMDelete], NoEventMask, wmatom[WMDelete], CurrentTime, 0, 0, 0))
	{
		XGrabServer(dpy);
		XSetErrorHandler(xerrordummy);
		XSetCloseDownMode(dpy, DestroyAll);
		XKillClient(dpy, c->win);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
}


void
killclient(const Arg *arg)
{
	killclientc(selmon->sel);
}

void
killclientforce(const Arg *arg)
{
	if (selmon->sel) {
		char pidstr[20];
		sprintf(pidstr, "%d", selmon->sel->pid);
		const char *killcmd[] = {"/usr/bin/kill",pidstr,NULL};
		Arg killarg = {.v=killcmd};
		spawn(&killarg);
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

int counttagtiled(Client *clients, int tags){
	int i = 0;
	Client *tmp;
	for (tmp = clients; tmp; tmp = tmp->next)
		if ((tmp->tags & tags) > 0 && !tmp->isfloating)
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
	LOG_FORMAT("manageppidstick 1");
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
	LOG_FORMAT("manageppidstick 2");
	if (found && tmpparent && (tmpparent->tags & TAGMASK) != TAGMASK)
	{
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
	LOG_FORMAT("manageppidstick 3");
	return 0;
}

void 
managestub(Client *c){
	if(!managestubon) return;
	LOG_FORMAT("managestub 1");
	// 每个tag不能超过 n 个client, 超过则移动到下一个tag
	int curisfloating = 0;
	Atom wtype = getwinatomprop(c->win, netatom[NetWMWindowType]);
	if (wtype == netatom[NetWMWindowTypeDialog]) curisfloating = 1;
	if (!curisfloating)
	{
		LOG_FORMAT("managestub 2");
		Rule * rule = getwinrule(c->win);
		LOG_FORMAT("managestub 3");
		if (!rule->isfloating)
		{
			int tmptags = selmon->tagset[selmon->seltags];
			LOG_FORMAT("managestub 4");
			while (counttagnstub(selmon->clients, tmptags) >= (3 - rule->nstub))
				tmptags = tmptags << 1;
			LOG_FORMAT("managestub 5");
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
	LOG_FORMAT("managestub 6");
}


int 
scratchsingle(char *cmd[],ScratchItem **siptr){
	ScratchItem *si = NULL;
	for (si = scratchgroupptr->head->next; si && si !=  scratchgroupptr->tail; si = si->next)
	{
		if (si->cmd == cmd){
			*siptr = si;
			return 1;
		}
	}
	return 0;
}

int 
ischildof(unsigned long pid, unsigned long ppid)
{
	int found = 0;
	if(pid){
		if (pid == ppid) {
			return 1;
		}
		int PPIDCHAIN_N = 10;
		unsigned long ppidchain[PPIDCHAIN_N];
		getppidchain(pid, ppidchain, PPIDCHAIN_N, 0);
		for (int i = 0; i < PPIDCHAIN_N; i++)
		{
			if (ppidchain[i] <= 1)
				break;
			if (ppidchain[i] == ppid)
			{
				found = 1;
				break;
			}
		}
	}
	return found;
}

void
manage(Window w, XWindowAttributes *wa)
{
	lastmanagetime = getcurrusec(); 
	isnexttemp = isnexttemp && (lastmanagetime - lastnexttemptime <= 1000000*5) && lastnexttemptime >= lastspawntime;

	isnextinner = isnextinner && (lastmanagetime - lastnextinnertime <= 1000000*5);
	int isispawn = isnextinner || (ischildof(getwindowpid(w), ispawnpids[0]) && lastmanagetime - ispawntimes[0] <= 1000000*5 ? 1:0);
	isnextinner = 0;

	isnextreplace = isnextreplace && (lastmanagetime - lastnextreplacetime <= 1000000*5);
	int isrispawn = isnextreplace;
	isnextreplace = 0;


	LOG_FORMAT("manage isispawn:%d", isispawn);

	// hidescratchgroup if needed (example: open app from terminal)
	if(scratchgroupptr->isfloating && !isnexttemp)
		hidescratchgroupv(scratchgroupptr, 0);

	Client *c, *t = NULL;
	Client *oldc = selmon->sel;
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
	c->factx = factx;
	c->facty = facty;
	c->launchparent = selmon->sel;
	global_client_id ++;
	c->id = global_client_id;
	c->containerrefc = NULL;
	
	LOG_FORMAT("isnexttemp:%d, c->istemp: %d  %d", isnexttemp, c->istemp, getpid());
	if(isnexttemp) {
		c->istemp = 1;
		// only one tmp window
		Client *tmpc;
		for(tmpc = selmon->clients;tmpc;tmpc=tmpc->next)
			if(tmpc->istemp) {
				killclientc(tmpc);
				if (selmon->sel->container->cn > 1 && selmon->sel->containerrefc == tmpc) {
					freecontainerc(selmon->sel->container, tmpc);
				}
			}
	 } else c->istemp = 0;

	if (isispawn && selmon->sel && selmon->sel->container->cn <= 1) {
		c->container = selmon->sel->container;
		c->container->cs[c->container->cn] = c;
		c->container->cn ++;
		c->containerrefc = selmon->sel;
		selmon->sel->containerrefc = c;
		ispawnpids[0] = 0;
		ispawntimes[0] = 0;
	}else{
		c->container = createcontainerc(c);
		if(selmon->sel)
			c->container->launchparent = selmon->sel->container;
		if (isrispawn) {
			replacercincontainer(c, oldc->containerrefc);
		/*}else{*/
			/*if (oldc){*/
				/*int isoldcreplaceable = strcmp(oldc->class, "St");*/
				/*if (isoldcreplaceable == 0) {*/
					/*replacercincontainer(c, oldc);*/
				/*}*/
			/*}*/
		}
	}

	updateicon(c);
	updateicons(c);
	updatetitle(c);
	updateclass(c);

	LOG_FORMAT("manage 1");
	if (XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans))) {
		c->mon = t->mon;
		c->tags = t->tags;
		c->nstub = 0;
	} else {
		c->mon = selmon;
		applyrules(c);
	}
	LOG_FORMAT("manage 2");

	LOG_FORMAT("manage 3");
	if(!manageppidstick(c) && !isnextscratch && !isnexttemp) managestub(c);
	if((selmon->tagset[selmon->seltags] & TAGMASK == TAGMASK) && (c->tags & TAGMASK) == TAGMASK) c->tags = 1; 
	LOG_FORMAT("manage 4");

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

	if (c->istemp)
	{
		if (isispawn) {
			c->isfloating = False;
		}else {
			c->isfloating = True;
			c->tags = TAGMASK;
		}
		c->w = c->mon->ww / 2.5;
		c->h = c->mon->wh / 2;
		c->x = c->mon->wx + (c->mon->ww - WIDTH(c));
		c->y = c->mon->wy + (c->mon->wh - HEIGHT(c))/2;
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
	/*if (!c->isfloating)*/
		/*c->isfloating = c->oldstate = trans != None || c->isfixed;*/
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
	LOG_FORMAT("manage 5");
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
	LOG_FORMAT("manage 6");



	arrange(c->mon);
	XMapWindow(dpy, c->win);
	XRaiseWindow(dpy,c->win);
	focus(c);


	isnexttemp = 0;
	// 这个要放到最后, 否则 isnexttemp 将不能被正确设置, see keypress
	if (c->istemp)
	{
		XSetWindowAttributes wa = {.event_mask = EnterWindowMask | FocusChangeMask | PropertyChangeMask | StructureNotifyMask | KeyPressMask};
		XChangeWindowAttributes(dpy, c->win, CWEventMask, &wa);
	}

	LOG_FORMAT("isnexttemp:%d, c->istemp: %d  %d", isnexttemp, c->istemp, getpid());
	actionlog("manage", c);
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
mapnotify(XEvent *e){	

}

void
monoclespace(Monitor *m)
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
		if (m->switcher) {
			resize(c, m->wx+m->switcherww, m->wy, m->ww - 2 * c->bw - m->switcherww, m->wh - 2 * c->bw, 0);
		}else {
			resize(c, m->wx, m->wy, m->ww - 2 * c->bw, m->wh - 2 * c->bw, 0);
		}
		c->bw = tmpbw;
	}
		
	if(m->sel) m->sel->fullscreenfreq++;
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

Client *topcs[2];
unsigned int topcpretags[2];
void doublepage(Monitor *m)
{
	unsigned int n = 0;
	Client *c;
	for (c = m->stack; c; c = c->snext)
		if (ISVISIBLE(c))
			n++;
	if (n > 0) /* override layout symbol */
		snprintf(m->ltsymbol, sizeof m->ltsymbol, "[%d]", n);

	int i = 0;
	for (c = nexttiled(m->clients); c; c = nexttiled(c->next))
	{
		if (topcs[0] == c)
		{
			resize(c, m->wx, m->wy, m->ww / 2 - 2 * c->bw, m->wh - 2 * c->bw, 0);
		}
		else if(topcs[1] == c){
			resize(c, m->ww / 2 + m->wx, m->wy, m->ww / 2 - 2 * c->bw, m->wh - 2 * c->bw, 0);
		}
		else
		{
			unsigned int tmpbw = c->bw;
			c->bw = 0;
			resize(c, m->wx, m->wy, m->ww - 2 * c->bw, m->wh - 2 * c->bw, 0);
			c->bw = tmpbw;
		}
		i++;
	}
}

int doubled = 0;
void doublepagemarkclient(Client *c){
	if (!topcs[0])
	{
		// overview
		/*if((selmon->tagset[selmon->seltags] & TAGMASK) != TAGMASK){*/
			/*Arg viewarg1 = {.ui = ~0};*/
			/*view(&viewarg1);*/
		/*}*/
		topcs[0] = c;
		c->bw = borderpx;
		XWindowChanges wc;
		wc.border_width = c->bw;
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc);
		XSetWindowBorder(dpy, c->win, scheme[SchemeDoublePageMarked][ColBorder].pixel);
		c->isdoublepagemarked = 1;
	}else if(!topcs[1])
	{
		topcs[1] = c;
		c->bw = borderpx;
		XWindowChanges wc;
		wc.border_width = c->bw;
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc);
		XSetWindowBorder(dpy, c->win, scheme[SchemeDoublePageMarked][ColBorder].pixel);
		c->isdoublepagemarked = 1;
	}
	if(topcs[0] && topcs[1]){
		unsigned int targettag = 1 << 7;
		topcpretags[0] = topcs[0]->tags;
		topcpretags[1] = topcs[1]->tags;
		topcs[0]->tags = targettag;
		topcs[1]->tags = targettag;
		topcs[0]->isdoublepagemarked = 0;
		topcs[1]->isdoublepagemarked = 0;
		Arg viewarg = {.ui = targettag};
		view(&viewarg);
		Arg layoutarg = {.v = &layouts[4]};
		setlayout(&layoutarg);
		doubled = 1;
		topcs[0]->isdoubled = 1;
		topcs[1]->isdoubled = 1;
	}
}

void doublepagemark(const Arg *arg){
	if(doubled) {
		cleardoublepage(3);
		return;
	}
	doublepagemarkclient(selmon->sel);
}

void cleardoublepage(int v){
	if(!doubled) return;
	if (topcs[0] && topcpretags[0] && topcs[1] && topcpretags[1])
	{
		topcs[0]->tags = topcpretags[0];
		topcs[1]->tags = topcpretags[1];
		topcs[0]->isdoublepagemarked = 0;
		topcs[1]->isdoublepagemarked = 0;
		topcs[0]->isdoubled = 0;
		topcs[1]->isdoubled = 0;
		// overview
		/*if (v == 1){*/
			/*Arg viewarg = {.ui = ~0};*/
			/*view(&viewarg);*/
		/*}*/
		if (v == 2) {
			Arg viewarg = {.ui =selmon->pertag->prevtag};
			view(&viewarg);
		}
		if (v == 3) {
			Arg viewarg = {.ui = 1<<0};
			view(&viewarg);
		}
	}
	doubled = 0;
	topcs[0] = NULL;
	topcs[1] = NULL;
}

void 
switchermotionnotify(XMotionEvent *ev)
{
	//todo
	int rx = ev->x;
	int ry = ev->y;
	int iw = selmon->ww / 6;
	int ih = selmon->wh / 6;
	int col = rx/iw;
	int row = ry/ih;
	int tagindex = col+row*3;
	unsigned int tags = 1 << tagindex;
	if (selmon->tagset[selmon->seltags] != tags) {
		const Arg arg = {.ui = tags};
		view(&arg);
		/*destroyswitcher(selmon);*/
		const Arg layoutarg = {.v = &layouts[0]};
		setlayout(&layoutarg);
		drawswitcherwin(selmon->switcher, selmon->ww/2, selmon->wh/2, tagindex);
		XMapWindow(dpy, selmon->switcher);
		XSetInputFocus(dpy, selmon->switcher, RevertToPointerRoot, 0);
	}
}


static Time lastmotiontime = 0;

void
motionnotify(XEvent *e)
{
	static Monitor *mon = NULL;
	Monitor *m;
	XMotionEvent *ev = &e->xmotion;

	if ((e->xmotion.time - lastmotiontime) <= (1000 / 60))
		return;
	lastmotiontime = e->xmotion.time;
	
	if (ev->window == selmon->switcher) {
		selmon->switcheraction.pointerfunc(ev->x, ev->y);
		return;
	}
	if (ev->window == selmon->switcherbarwin) {
		selmon->switcherbaraction.pointerfunc(ev->x, ev->y);
		return;
	}
	if (ev->window == selmon->switcherstickywin) {
		selmon->switcherstickyaction.pointerfunc(ev->x, ev->y);
		return;
	}

	if (ev->window != root)
		return;

	if ((m = recttomon(ev->x_root, ev->y_root, 1, 1)) != mon && mon) {
		unfocus(selmon->sel, 1);
		selmon = m;
		focus(NULL);
	}
	mon = m;

	if(ev->y >selmon->by && ev->y < selmon->by + bh){
		int x = 0, i = 0;
		Client *c;
		unsigned int occ = 0;
		for(c = m->clients; c; c=c->next)
			occ |= c->tags;
		do {
			/* Do not reserve space for vacant tags */
			if (!(occ & 1 << i || m->tagset[m->seltags] & 1 << i))
				continue;
			x += TEXTW(tags[i]);
		} while (ev->x >= x && ++i < LENGTH(tags));
		if (i < LENGTH(tags) && 1 << i != selmon->tagset[selmon->seltags]) {
			const Arg arg = {.ui = 1 << i};
			view(&arg);
		}else{
			// 鼠标移动到标题栏 出现switcher
			/*if(ev->x > x && ev->x < selmon->ww - statusw - getsystraywidth()){*/
			// todo
			/*}*/

			x += blw;

			for(i = 0; i < LENGTH(launchers); i++) {
				x += TEXTW(launchers[i].name);
			}

			Client *vc;
			for (vc = selmon->clients; vc; vc = vc->next) {
				if (ISVISIBLE(vc)) {
					x += vc->titlew;
				}
			}

			// 鼠标移动到标题栏空白处, 出现rofi
			/*if(ev->x > x && ev->x < selmon->ww - statusw - getsystraywidth()){*/
				/*char offsetx[5];*/
				/*char offsety[5];*/
				/*if (ev->x - x > 300) {*/
					/*sprintf(offsetx, "%d", ev->x - 300);*/
				/*}else{*/
					/*sprintf(offsetx, "%d", x);*/
				/*}*/
				/*sprintf(offsety, "%d", ev->y - 250);*/
				/*char *rofi[] = {"rofi","-theme-str","window {width:600px;height:250px;}","-location","1","-xoffset",offsetx,"-yoffset",offsety,"-me-select-entry","","-me-accept-entry","MousePrimary", "-show","window",NULL};*/
				/*Arg arg = {.v = rofi};*/
				/*spawn(&arg);*/
			/*}*/
		}
	}
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

int
pyplace(int targetindex[], int n, MXY targetpos[], int clientids[])
{
	if(n==0) return 0;
	int i;
	char *url = "http://localhost:8666/place";
	char params[3000];
	memset(params, 0, sizeof(params));

	strcat(params, "tag=");
	char tag[2];
	sprintf(tag, "%d", getcurtagindex(selmon));
	strcat(params, tag);
	strcat(params, "&");
	strcat(params, "targets=");
	for(i=0;i<n;i++)
	{
		char str[20];
		sprintf(str, "%d|%d|%d|%d", targetindex[i], targetpos[i].row, targetpos[i].col, clientids[i]);
		strcat(params, str);
		if(i!=n-1)
			strcat(params, ",");
	}

	struct HttpResponse resp;
	resp.content = malloc(1);
	resp.size = 0;
	resp.code = CURLE_OK;
	int ok = httppost(url,params, &resp);
	if (!ok) return 0;

	/*int j = 0;*/
	/*char *temp = strtok(resp.content,",");*/
	/*while(temp)*/
	/*{*/
		/*[>LOG_FORMAT("pyresort2 %s", temp);<]*/
		/*sscanf(temp,"%d",&resorted[j]);*/
		/*j++;*/
		/*temp = strtok(NULL,",");*/
	/*}*/
	free(resp.content);
	return 1;
}


// 实际坐标转化为spiral 布局中的以中间为中心的坐标
/**
 * 假設所有塊的大小都相同
 * cxy: 待轉換的cxy
 * currcentercxy: 當前中心的塊(client or container)cxy座標
 * currcenterblockw, currcenterblockh: 當前中心塊的大小
 * currcentermatcoor: 當前中心所在塊兒的矩陣位置
*/
XY
clientxy2centeredx(XY cxy, XY currcentercxy, int currcenterblockw, int currcenterblockh, MXY curcentermatcoor)
{
	XY xy;
	rect_t sc;
	sc.x = 0;
	sc.y = 0;
	sc.w = selmon->ww;
	sc.h = selmon->wh;
	int offsetx = sc.w / 2 - currcentercxy.x;
	int offsety = sc.h / 2 - currcentercxy.y;

	
	int w = currcenterblockw;
	int h = currcenterblockh;
	int centerx = curcentermatcoor.col * w - w / 2;
	int centery = curcentermatcoor.row * h - h / 2;

	xy.x = cxy.x - offsetx + centerx;
	xy.y = cxy.y - offsety + centery;
	return xy;
}

XY
clientxy2centered(XY cxy)
{
	XY currcentercxy = {selmon->sel->x + selmon->sel->w / 2, selmon->sel->y + selmon->sel->h / 2};
	int currcenterblockw = selmon->ww * tile6initwinfactor;
	int currcenterblockh = selmon->wh * tile6initwinfactor;
	MXY curcentermatcoor = selmon->sel->matcoor;
	LOG_FORMAT("clientxy2centered 0 currcentercxy:%d %d ", currcentercxy.x, currcentercxy.y);
	return clientxy2centeredx(cxy, currcentercxy, currcenterblockw, currcenterblockh, curcentermatcoor);
}

XY
clientxy2centered_container(XY cxy)
{
	if(!selmon->sel)
	{
		XY sxy = {0,0};
		return sxy;
	}

	LOG_FORMAT("tracecontainer clientxy2centered_container 0 sel:%s %d %d,%d %d,%d", selmon->sel->name, selmon->sel->container->id, selmon->sel->x, selmon->sel->y, selmon->sel->container->x, selmon->sel->container->y);
	XY currcentercxy = {selmon->sel->container->x + selmon->sel->container->w / 2, selmon->sel->container->y + selmon->sel->container->h / 2};
	int currcenterblockw = selmon->sel->container->w;
	int currcenterblockh = selmon->sel->container->h;
	MXY curcentermatcoor = selmon->sel->matcoor;
	LOG_FORMAT("clientxy2centered_container 0 currcentercxy:%d %d ", currcentercxy.x, currcentercxy.y);
	LOG_FORMAT("clientxy2centered_container 0 curcentermatcoor:%d %d ", curcentermatcoor.row, curcentermatcoor.col);
	return clientxy2centeredx(cxy, currcentercxy, currcenterblockw, currcenterblockh, curcentermatcoor);
}

// 将转化后的spiral 坐标进行搜索, 查找对应的位置
int 
spiralsearch(XY centeredxy){
	int w = selmon->ww * tile6initwinfactor;
	int h = selmon->wh * tile6initwinfactor;

	int n = 121;
	int i;
	for(i=0;i<n;i++)
	{
		int col = spiral_index[i].col;
		int row = spiral_index[i].row;
		int blockcenterx = col * w;
		int blockcentery = row * h;
		if (centeredxy.x > blockcenterx - w/2 && centeredxy.x < blockcenterx + w/2 && centeredxy.y > blockcentery - h/2 && centeredxy.y < blockcentery + h/2) {
			return i;
		}
	}
	return -1;
}

void
separatefromcontainer(Client *oldc){
	if (!oldc->containerrefc) {
		return;
	}
	if (oldc->container->id == oldc->id) {
		createcontainerc(oldc->containerrefc);
		removeclientfromcontainer(oldc->container, oldc->containerrefc);
		oldc->containerrefc = NULL;
	}else{
		Client *refc = oldc->containerrefc;
		createcontainerc(oldc);
		if (refc) {
			LOG_FORMAT("pysmoveclient 1, oldc %s, containerid:%d, cid:%d", oldc->name, oldc->container->id, oldc->id);
			LOG_FORMAT("pysmoveclient 1, refc %s, containerid:%d, cid:%d", refc->name, refc->container->id, refc->id);
			removeclientfromcontainer(refc->container, refc->containerrefc);
			refc->containerrefc = NULL;
			LOG_FORMAT("pysmoveclient 2, oldc %s, containerid:%d, cid:%d, cn:%d", oldc->name, oldc->container->id, oldc->id,oldc->container->cn);
		}
	}

}

void 
mergetocontainerof(Client *oldc, Client *chosenc){
	if(chosenc->container->cn >= 2) return;
	if(chosenc == oldc) return;
	freecontainerc(oldc->container, oldc);
	oldc->container = chosenc->container;
	oldc->container->cs[oldc->container->cn] = oldc;
	if(oldc->x < chosenc->x){
		oldc->container->cs[oldc->container->cn] = chosenc;
		oldc->container->cs[0] = oldc;
	}
	oldc->container->cn ++;
	oldc->containerrefc = chosenc;
	chosenc->containerrefc = oldc;
}

void 
replacercincontainer(Client *oldc, Client *chosenc){
	if (chosenc->container->cn > 1) {
		Client *oric = chosenc->containerrefc;
		separatefromcontainer(chosenc);
		mergetocontainerof(oldc, oric);
	}
}


void
pysmoveclient(Client *target, int sx, int sy)
{
	LOG_FORMAT("movemouseswitcher sx:%d, sy:%d", sx, sy);
	Client *oldc = target;
	Container *container;
	if (sx < 0 || sx > selmon->switcherww || sy < 0 || sy > selmon->switcherwh) 
	{
		separatefromcontainer(oldc);
		arrange(selmon);
		selmon->switcheraction.drawfunc(selmon->switcher, selmon->switcherww, selmon->switcherwh);
		return;
	}

	XY sxys[] = {{sx, sy}};
	XY cxys[1];
	int tagindexout[1];
	selmon->switcheraction.switcherxy2xy(sxys,1,cxys, tagindexout);
	XY centerxy = clientxy2centered_container(cxys[0]);
	LOG_FORMAT("movemouseswitcher 0 centerxy:%d %d ", centerxy.x, centerxy.y);

	Client *chosenc = selmon->switcheraction.sxy2client(sx, sy);
	if (chosenc) {
		LOG_FORMAT("movemouseswitcher c:%s", chosenc->name);
		if (oldc) {
			if (chosenc->container == oldc->container) {
				// 交换位置
				Container *ct = chosenc->container;
				Client *tmp = ct->cs[0];
				ct->cs[0] = ct->cs[1];
				ct->cs[1] = tmp;
				return;
			}
			mergetocontainerof(oldc, chosenc);
			arrange(selmon);
			selmon->switcheraction.drawfunc(selmon->switcher, selmon->switcherww, selmon->switcherwh);
		}
	}else{
		LOG_FORMAT("movemouseswitcher centerxy:%d %d ", centerxy.x, centerxy.y);
		if (oldc) {
			LOG_FORMAT("movemouseswitcher sending");
			int n = 2;
			int targetindex[n];
			MXY targetpos[n];
			int clientids[n];
			int foundspiralindex = spiralsearch(centerxy);
			if (foundspiralindex >= 0) {
				/*clientids[0] = oldc->id;*/
				if(oldc->container->cn > 1){
					Client *containerrefc = oldc->containerrefc;

					MXY oldmxy = oldc->matcoor;
					MXY newmxy = spiral_index[foundspiralindex];

					// 拆
					if (oldc->container->id == oldc->id && oldc->containerrefc) {
						container = createcontainerc(oldc->containerrefc);
						removeclientfromcontainer(oldc->container, oldc->containerrefc);
						oldc->containerrefc = NULL;
					}else{
						Client *refc = oldc->containerrefc;
						container = createcontainerc(oldc);
						if (refc) {
							removeclientfromcontainer(refc->container, refc->containerrefc);
							refc->containerrefc = NULL;
						}
					}
					arrange(selmon);

					// 移
					LOG_FORMAT("movemouseswitcher 3");
					targetindex[0] = oldc->container->launchindex;
					targetpos[0] = newmxy;
					clientids[0] = oldc->container->id;

					targetindex[1] = containerrefc->container->launchindex;
					targetpos[1] = oldmxy;
					clientids[1] = containerrefc->container->id;
					LOG_FORMAT("movemouseswitcher 3.5 newmxy:%d,%d", newmxy.row, newmxy.col);
					pyplace(targetindex, 2, targetpos, clientids);
					LOG_FORMAT("movemouseswitcher 4");
				}else{
					LOG_FORMAT("movemouseswitcher 7");
					targetindex[0] = oldc->container->launchindex;
					targetpos[0] = spiral_index[foundspiralindex];
					clientids[0] = oldc->container->id;
					pyplace(targetindex, 1, targetpos, clientids);
					LOG_FORMAT("movemouseswitcher 8");
				}

				LOG_FORMAT("movemouseswitcher 2");
				arrange(selmon);
				selmon->switcheraction.drawfunc(selmon->switcher, selmon->switcherww, selmon->switcherwh);
			}
		}
	}
}

void
movemouseswitcher(const Arg *arg)
{
	int x, y, ocx, ocy, px, py;
	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel))
		return;
	if (c->isfullscreen) /* no support moving fullscreen windows by mouse */
		return;
	/*restack(selmon);*/
	ocx = c->x;
	ocy = c->y;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurMove]->cursor, CurrentTime) != GrabSuccess)
		return;
	if (!getrootptr(&x, &y))
		return;

	/*int oldx = ev.xmotion.x - selmon->switcherwx;*/
	/*int oldy = ev.xmotion.y - selmon->switcherwy;*/
	/*Client *oldc = selmon->switcheraction.pointerfuncx(oldx, oldy);*/
	Client *oldc = selmon->sel;
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
			px = ev.xmotion.x - selmon->switcherwx;
			py = ev.xmotion.y - selmon->switcherwy;
			selmon->switcheraction.drawfuncx(selmon->switcher, selmon->switcherww, selmon->switcherwh);
			drw_setscheme(drw, scheme[SchemeSel]);
			drw_rect(drw, px, py, selmon->switcherww/7, selmon->switcherwh/7, 1, 1);
			drw_map(drw,selmon->switcher, 0, 0,selmon->switcherww, selmon->switcherwh);
			break;
		}
	} while (ev.type != ButtonRelease);
	XUngrabPointer(dpy, CurrentTime);

	pysmoveclient(oldc, px, py);

}

Client *
snexttiled(Client *c)
{
	for (; c && (c->isfloating || !ISVISIBLE(c)); c = c->snext);
	return c;
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
			c->hintsvalid = 0;
			break;
		case XA_WM_HINTS:
			updatewmhints(c);
			drawbars();
			updateswitchersticky(selmon);
			break;
		}
		if (ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
			updatetitle(c);
			if (c == c->mon->sel)
				drawbar(c->mon);
		}
		if (ev->atom == XA_WM_CLASS) {
			updateclass(c);
			if (c == c->mon->sel)
				drawbar(c->mon);
		}
		else if (ev->atom == netatom[NetWMIcon]) {
			updateicon(c);
			updateicons(c);
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
	for (m = mons; m; m = m->next){
		LOG_FORMAT("recttomon %p", m);
	}

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
	if (c->x == x && c->y == y && c->w == w && c->h == h)
		return;
	
	// LOG_FORMAT("resize, c->name:%s, %d %d %d %d", c->name, x, y, w, h);
	if (applysizehints(c, &x, &y, &w, &h, interact))
		resizeclient(c, x, y, w, h);
}

void
movex(const Arg *arg)
{
	int delta = arg->i;
	if (selmon->sel)
	{
		/*if (!selmon->sel->isfloating)*/
		/*{*/
			/*Client *c;*/
			/*for (c = selmon->clients; c; c = c->next)*/
				/*if (ISVISIBLE(c) && c->isfloating)*/
					/*break;*/
			/*if (c)*/
				/*focus(c);*/
		/*}*/
		if (selmon->sel->isfloating)
		{
			resize(selmon->sel, selmon->sel->x + delta, selmon->sel->y, selmon->sel->w, selmon->sel->h, 0);
		}
	}
}

void 
movey(const Arg *arg)
{
	int delta = arg->i;
	if (selmon->sel)
	{
		/*if (!selmon->sel->isfloating)*/
		/*{*/
			/*Client *c;*/
			/*for(c = selmon->clients; c; c=c->next)*/
				/*if (ISVISIBLE(c) && c->isfloating)*/
					/*break;*/
			/*if(c) focus(c);*/
		/*}*/
		if (selmon->sel->isfloating)
		{
			resize(selmon->sel, selmon->sel->x, selmon->sel->y + delta, selmon->sel->w, selmon->sel->h, 0);
		}
	}
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
resizex(const Arg *arg)
{
	Client *c = selmon->sel;
	resizeclient(c, c->x, c->y, c->w + arg->f * c->mon->ww, c->h);
	c->placed = 1;
	arrange(selmon);
}

void
resizey(const Arg *arg)
{
	Client *c = selmon->sel;
	resizeclient(c,c->x,c->y, c->w, c->h + arg->f * c->mon->wh);
	c->placed = 1;
	arrange(selmon);
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
		if (handler[ev.type]){
			handler[ev.type](&ev); /* call handler */
		}
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
		LOG_FORMAT("setfocus: 1 ");
		XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
		XChangeProperty(dpy, root, netatom[NetActiveWindow],
			XA_WINDOW, 32, PropModeReplace,
			(unsigned char *) &(c->win), 1);
		LOG_FORMAT("setfocus: 2 ");
	}
	sendevent(c->win, wmatom[WMTakeFocus], NoEventMask, wmatom[WMTakeFocus], CurrentTime, 0, 0, 0);
	c->isfocused = True;
	LOG_FORMAT("setfocus: 3 ");
	lru(c);
	LOG_FORMAT("setfocus: 4 ");
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
	Client *curc = selmon->sel;
	if (curc && curc->isscratched && curc->isfloating && ISVISIBLE(selmon->sel))
	{
		const Arg scratcharg = {0};
		togglescratchgroup(&scratcharg);
		const Arg tagarg = {.ui = curc->tags};
		view(&tagarg);
		focus(curc);
	}
	if ((selmon->tagset[selmon->seltags] & TAGMASK) == TAGMASK)
	{
		const Arg tagarg = {.ui = curc->tags};
		view(&tagarg);
		focus(curc);
	}
	if (selmon->sel && !selmon->sel->isdoublepagemarked && selmon->sel->isdoubled)
	{
		if(topcs[0] == selmon->sel){
			const Arg tagarg = {.ui = topcpretags[0]};
			view(&tagarg);
		}
		if (topcs[1] == selmon->sel)
		{
			const Arg tagarg = {.ui = topcpretags[1]};
			view(&tagarg);
		}
		cleardoublepage(0);
	}
	unsigned int i;
	if (!arg || !arg->v || arg->v != selmon->lt[selmon->sellt])
		selmon->sellt ^= 1;
	if (arg && arg->v)
		selmon->lt[selmon->sellt] = (Layout *)arg->v;
	strncpy(selmon->ltsymbol, selmon->lt[selmon->sellt]->symbol, sizeof selmon->ltsymbol);

	for(i=0; i<LENGTH(tags); ++i)
		if(selmon->tagset[selmon->seltags] & 1<<i)
		{
			// 当前选中的layout在 selmon->lt 中的索引, selmon->lt 保存了2个
			selmon->pertag->sellts[i+1] = selmon->sellt;
			// 当前选中的layout
			selmon->pertag->ltidxs[i+1][selmon->pertag->sellts[i+1]] = selmon->lt[selmon->sellt]; 
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

	if (selmon->sel && selmon->sel->isfloating) {
		return;
	}

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
setfacty(const Arg *arg)
{
	float f;
	unsigned int i;

	if (selmon->sel && selmon->sel->isfloating) {
		return;
	}
	
	if (selmon->sel->facty != facty) {
		f = facty;
	}else{
		f = facty * 3;
	}

	Client *c;
	for (c = nexttiled(selmon->clients); c; c = nexttiled(c->next)) c->facty = facty;
	
	selmon->sel->facty = f;
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
	netatom[NetWMIcon] = XInternAtom(dpy, "_NET_WM_ICON", False);
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
	// LOG_FORMAT("showhide 1: c->name: %s ,p:%p| ", c->name, c);
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

// not used
void showhidealltag(Client *c)
{
	if (!c) return;
	Client *oc = c;
	for(;c;c = c->snext){
		if (ISVISIBLE(c)) 
		{
			// XMoveWindow(dpy, c->win, c->x, c->y);
			if ((!c->mon->lt[c->mon->sellt]->arrange || c->isfloating) && !c->isfullscreen){
				// resize(c, c->x, c->y, c->w, c->h, 0);
				// XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
			}
		}
	}
	unsigned int seltags = selmon->tagset[selmon->seltags];
	int i;
	for (i = 0; i < LENGTH(tags); i++)
	{
		selmon->tagset[selmon->seltags] = 1 << i;
		if (selmon->lt[selmon->pertag->sellts[i+1]]->arrange && !(seltags & (1<<i)))
		{
			selmon->lt[selmon->pertag->sellts[i+1]]->arrange(selmon);
			for (c = oc; c; c = c->snext)
				if (!(c->tags & seltags))
				{
					// resize(c, WIDTH(c) * -1, c->y, c->w, c->h, 0);
					// XMoveWindow(dpy, c->win, WIDTH(c) * -1, c->y);
					XMoveResizeWindow(dpy, c->win, WIDTH(c) * -1, c->y,  c->w, c->h);
				}
			
		}
	}
	selmon->tagset[selmon->seltags] = seltags;
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

pid_t
forkrun(const Arg *arg)
{
	lastspawntime = getcurrusec();
	pid_t pid = fork();
	if (pid == 0) {
		// child process
		if (dpy)
			close(ConnectionNumber(dpy));
		setsid();
		execvp(((char **)arg->v)[0], (char **)arg->v);
		fprintf(stderr, "dwm: execvp %s", ((char **)arg->v)[0]);
		perror(" failed");
		exit(EXIT_SUCCESS);
	}else{
		// parent process
		return pid;
	}
}

void
spawn(const Arg *arg)
{
	if (arg->v == dmenucmd)
		dmenumon[0] = '0' + selmon->num;
	selmon->tagset[selmon->seltags] &= ~scratchtag;
	forkrun((arg));
}

void
sspawn(const Arg *arg)
{
	ScratchItem *si;
	if (!scratchgroupptr->isfloating && scratchsingle(arg->v, &si))
	{
		LOG_FORMAT("sspawn: before showscratchgroup");
		showscratchgroup(scratchgroupptr);
		LOG_FORMAT("sspawn: after showscratchgroup");
		focus(si->c);
		LOG_FORMAT("sspawn: after focus");
		arrange(selmon);
		LOG_FORMAT("sspawn: after arrange");
		return;
	}
	isnextscratch = 1;
	nextscratchcmd = arg->v;
	spawn(arg);
}

void 
tsspawn(const Arg *arg)
{
	isnexttemp = 1;
	nexttempcmd = arg->v;
	lastnexttemptime = getcurrusec();
	spawn(arg);
	lastspawntime = lastnexttemptime;
}


void 
ispawn(const Arg *arg)
{
	isnextinner = 1;
	lastnextinnertime = getcurrusec();
	pid_t pid = forkrun(arg);
	ispawnpids[0] = pid;
	ispawntimes[0] = getcurrusec();
}

void 
itspawn(const Arg *arg)
{
	isnexttemp = 1;
	nexttempcmd = arg->v;
	lastnexttemptime = getcurrusec();
	pid_t pid = forkrun(arg);
	lastspawntime = lastnexttemptime;
	ispawnpids[0] = pid;
	ispawntimes[0] = getcurrusec();
}

void
rispawn(const Arg *arg){
	isnextreplace = 1;
	lastnextreplacetime = getcurrusec();
	ispawn(arg);
}

char *
getcwd_by_pid(pid_t pid) {
	char buf[32];
	snprintf(buf, sizeof buf, "/proc/%d/cwd", pid);
	return realpath(buf, NULL);
}

void getstworkingdir(char *workingdir, pid_t currpid){
	DIR *dirp;
	struct dirent *direntp;
	unsigned long childpid = 0L;
	char buf[512];
	char appname[512];
 
 	dirp=opendir("/proc");
	while((direntp=readdir(dirp))!=NULL) 
	{
		if (direntp->d_type == DT_DIR) {
			char dirbuf[512]; 
			memset(dirbuf,0,sizeof(dirbuf)); 
			strcpy(dirbuf,"/proc"); 
			strcat(dirbuf,"/"); 
			strcat(dirbuf,direntp->d_name); 
			pid_t pid;
			int r = sscanf(direntp->d_name, "%d",&pid);
			if (r) {
				if(currpid == getppidof(pid)){
					childpid = pid;
					snprintf(buf, sizeof buf, "/proc/%d/stat", pid);
					sscanf(buf, "%*d (%s) %*s", appname);
					break;
				}
			}
		}
	}
	if (childpid && strcmp(appname, "bash")) {
		char *cwd = getcwd_by_pid(childpid);
		if (cwd) {
			strcpy(workingdir, cwd);
			free(cwd);
		}
	}else{
		char *cwd = getcwd_by_pid(currpid);
		if (cwd) {
			strcpy(workingdir, cwd);
			free(cwd);
		}
	}
	closedir(dirp);
}

void stspawn(const Arg *arg){
	char workingdir[128] = "";
	if (selmon->sel) {
		pid_t currpid = selmon->sel->pid;
		if (currpid) {
			getstworkingdir(workingdir, currpid);
		}
	}
	if (workingdir) {
		char *cmd[] = {"st","-d",workingdir,NULL};
		const Arg a = {.v = cmd};
		spawn(&a);
	}else{
		char *cmd[] = {"st",NULL};
		const Arg a = {.v = cmd};
		spawn(&a);
	}
}
void stsspawn(const Arg *arg){
	char workingdir[128] = "";
	if (selmon->sel) {
		pid_t currpid = selmon->sel->pid;
		if (currpid) {
			getstworkingdir(workingdir, currpid);
		}
	}
	char *cmd[] = {"st","-d",workingdir,NULL};
	const Arg a = {.v = cmd};
	sspawn(&a);
}

void 
stispawn(const Arg *arg){
	char workingdir[128] = "";
	if (selmon->sel) {
		pid_t currpid = selmon->sel->pid;
		if (currpid) {
			getstworkingdir(workingdir, currpid);
		}
	}
	char *cmd[] = {"st","-d",workingdir,NULL};
	const Arg a = {.v = cmd};
	ispawn(&a);
}

void reltag(const Arg *arg)
{
	unsigned int maxtags = 1 << (LENGTH(tags)-1);
	unsigned int nexttags = 0;
	if (arg->i > 0)
	{
		if ((selmon->tagset[selmon->seltags] & TAGMASK) == TAGMASK)
			nexttags = 1;
		else
			nexttags = selmon->tagset[selmon->seltags] << arg->i;
		if(nexttags > maxtags) nexttags = 1;
	}
	if (arg->i < 0)
	{
		if ((selmon->tagset[selmon->seltags] & TAGMASK) == TAGMASK)
			nexttags = maxtags;
		else
			nexttags = selmon->tagset[selmon->seltags] >> -arg->i;
		if(nexttags == 0) nexttags = maxtags;
	}

	if (selmon->sel && nexttags & TAGMASK)
	{
		selmon->sel->tags = nexttags & TAGMASK;
		focus(NULL);
		arrange(selmon);
		Arg viewarg = {.ui = nexttags};
		view(&viewarg);
	}
}

void reltagd(const Arg *arg)
{
	unsigned int maxtags = 1 << (LENGTH(tags) - 1);
	unsigned int nexttags = 0;
	if (arg->i > 0)
	{
		if ((selmon->tagset[selmon->seltags] & TAGMASK) == TAGMASK)
			nexttags = 1;
		else
			nexttags = selmon->tagset[selmon->seltags] << arg->i;
		if (nexttags > maxtags)
			nexttags = 1;
	}
	if (arg->i < 0)
	{
		if ((selmon->tagset[selmon->seltags] & TAGMASK) == TAGMASK)
			nexttags = maxtags;
		else
			nexttags = selmon->tagset[selmon->seltags] >> -arg->i;
		if (nexttags == 0)
			nexttags = maxtags;
	}

	if (selmon->sel && nexttags & TAGMASK)
	{
		selmon->sel->tags = nexttags & TAGMASK;
		focus(NULL);
		arrange(selmon);
	}
}

void
tag(const Arg *arg)
{
	if (selmon->sel && arg->ui & TAGMASK) {
		Client *oldc = selmon->sel;
		if (oldc->container->cn > 1) {
				// 拆
				Container *container;
				if (oldc->container->id == oldc->id && oldc->containerrefc) {
					container = createcontainer();
					container->id = oldc->containerrefc->id;
					oldc->containerrefc->container = container;
				}else{
					container = createcontainer();
					container->id = oldc->id;
					oldc->container = container;
				}
				if (oldc->containerrefc) {
					oldc->containerrefc->containerrefc = NULL;
					oldc->containerrefc = NULL;
				}
		}
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
		/*mw = m->nmaster ? m->ww * m->pertag->mfacts[gettagindex(m->tagset[m->seltags]) + 1] : 0;*/
		// 每次arrange之前都会把m->mfact 设置成当前tag的mfact, 所以这里这样写也没问题. see view(const Arg *arg)
		mw = m->nmaster ? m->ww * m->mfact : 0;
	else
		mw = m->ww - m->gap->gappx;
	
	float stotalfacty = 0.0;
	i = 0;
	for (c = nexttiled(m->clients); c; c = nexttiled(c->next)){
		if (c->facty == 0.0) {
			c->facty = facty;
		}
		if (i >= m->nmaster) {
			stotalfacty += c->facty;
		}
		i ++;
	}

	float tfacty = 0.0;
	for (i = 0, my = ty = m->gap->gappx, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
		if (i < m->nmaster) {
			h = (m->wh - my) / (MIN(n, m->nmaster) - i) - m->gap->gappx;
			resize(c, m->wx + m->gap->gappx, m->wy + my, mw - (2*c->bw) - m->gap->gappx, h - (2*c->bw), 0);
			if (my + HEIGHT(c) + m->gap->gappx < m->wh)
				my += HEIGHT(c) + m->gap->gappx;
		} else {
			/*h = (m->wh - ty) / (n - i) - m->gap->gappx;*/
			h = (int)((m->wh - ty - m->gap->gappx) * c->facty / (stotalfacty-tfacty));
			resize(c, m->wx + mw + m->gap->gappx, m->wy + ty, m->ww - mw - (2*c->bw) - 2*m->gap->gappx, h - (2*c->bw), 0);
			if (ty + HEIGHT(c) + m->gap->gappx < m->wh)
				ty += HEIGHT(c) + m->gap->gappx;
			tfacty += c->facty;
		}

	for (c = nexttiled(m->clients); c; c = nexttiled(c->next)){
		c->bw = borderpx;
		XWindowChanges wc;
		wc.border_width = c->bw;
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc);
		updateborder(c);
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
	LOG_FORMAT("tile2 1");
	unsigned int i, n, h, mw, my, ty, masterend = 0;
	Client *c;

	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
	if (n == 0)
		return;

	if (n > m->nmaster)
		mw = m->nmaster ? (m->ww - 3*m->gap->gappx)*mfact : 0;
	else
		mw = m->ww - 2 * m->gap->gappx;

	LOG_FORMAT("tile2 2");

	int focused_slave_index = -1;
	geo_t gmap[n];
	int masterh = (m->wh - m->gap->gappx) / (MIN(n,m->nmaster)) - m->gap->gappx;
	int slaveh = n-MIN(n, m->nmaster) == 0 ? 0 : (m->wh - m->gap->gappx) / (n - MIN(n,m->nmaster)) - m->gap->gappx;
	for (i = 0, my = ty = m->gap->gappx, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
	{	
		memset(gmap+i, 0, sizeof(geo_t));
		if (i < m->nmaster) {
			h = masterh;
			geo(gmap+i,c, m->wx + m->gap->gappx, m->wy + my, mw, h, 0);
			geo_t g = gmap[i];
			if (my + g.h + m->gap->gappx < m->wh)
				my += g.h + m->gap->gappx;
			masterend = i;
		} else {
			if(c->isfocused){
				focused_slave_index = i;
			}
			h = slaveh;
			geo(gmap+i,c, m->wx + mw + 2 * m->gap->gappx, m->wy + ty, m->ww - mw - 3*m->gap->gappx, h, 0);
			geo_t g = gmap[i];
			if (ty + g.h + m->gap->gappx < m->wh)
				ty += g.h + m->gap->gappx;
		}
	}

	LOG_FORMAT("tile2 3");

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
		LOG_FORMAT("tile2 6");
		resize(onlyg.c, onlyg.c->mon->wx, onlyg.c->mon->wy, onlyg.c->mon->ww, onlyg.c->mon->wh, onlyg.interact);
		LOG_FORMAT("tile2 7");
		onlyg.c->bw = onlyg.c->oldbw;
		return;
	}

	LOG_FORMAT("tile2 4");

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

	LOG_FORMAT("tile2 5");
}

void
tile3(Monitor *m)
{
	unsigned int i, n, h, mw,mx, my, ty;
	Client *c;

	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
	if (n == 0)
		return;

	if (n > m->nmaster)
		/*mw = m->nmaster ? m->ww * m->pertag->mfacts[gettagindex(m->tagset[m->seltags]) + 1] : 0;*/
		// 每次arrange之前都会把m->mfact 设置成当前tag的mfact, 所以这里这样写也没问题. see view(const Arg *arg)
		mw = m->nmaster ? (m->ww - m->gap->gappx) * m->mfact : 0;
	else
		mw = m->ww - m->gap->gappx;
	
	if (n > m->nmaster + 1)
	{
		mx = (m->ww - m->gap->gappx) * ((1-m->mfact)/2) + m->gap->gappx;
	}
	else if (n > m->nmaster)
	{
		mx = m->wx + m->gap->gappx;
		/*mx = (m->ww - m->gap->gappx) * (1-m->mfact) + m->gap->gappx;*/
	}
	else
	{
		mx = m->wx + m->gap->gappx;
	}

	
	for (i = 0, my = m->gap->gappx, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
		if (i < m->nmaster) {
			h = (m->wh - my) / (MIN(n, m->nmaster) - i) - m->gap->gappx;
			resize(c, mx, m->wy + my, mw - (2*c->bw) - m->gap->gappx, h - (2*c->bw), 0);
			if (my + HEIGHT(c) + m->gap->gappx < m->wh)
				my += HEIGHT(c) + m->gap->gappx;
		}

	float soverflowfact = 0.2;
	int soverflow = soverflowfact * m->ww;
	int sn0 = (n-m->nmaster)/2 + (1-m->nmaster%2);
	int sn1 = n-m->nmaster - sn0;
	int ti;
	for (ti = 0,i = 0, ty = m->gap->gappx, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
	{
		if (i < m->nmaster) continue;
		if( i%2 != 0) continue;
		/*h = (m->wh - ty) / (n - i) - m->gap->gappx;*/
		int sx = m->wx + m->gap->gappx;
		int sw = mx - 2*m->gap->gappx;
		sw += soverflow;
		sx -= soverflow;
		if (c->isfocused) {
			sx += soverflow;
		}
		h = (int)((m->wh - ty - m->gap->gappx) / (sn0-ti));
		resize(c,sx , m->wy + ty, sw, h - (2*c->bw), 0);
		if (ty + HEIGHT(c) + m->gap->gappx < m->wh)
			ty += HEIGHT(c) + m->gap->gappx;
		ti ++;
	}

	for (ti = 0, i = 0, ty = m->gap->gappx, c = nexttiled(m->clients); c ; c = nexttiled(c->next), i++)
	{
		if (i < m->nmaster) continue;
		if( i%2 != 1) continue;
		/*h = (m->wh - ty) / (n - i) - m->gap->gappx;*/
		int sx = mx + mw; 
		int sw = m->ww - mx - mw - m->gap->gappx;
		sw += soverflow;
		if (c->isfocused) {
			sx -= soverflow;
		}
		h = (int)((m->wh - ty - m->gap->gappx) / (sn1-ti));
		resize(c, sx, m->wy + ty, sw, h - (2*c->bw), 0);
		if (ty + HEIGHT(c) + m->gap->gappx < m->wh)
			ty += HEIGHT(c) + m->gap->gappx;
		ti ++;
	}

	for (c = nexttiled(m->clients); c; c = nexttiled(c->next)){
		c->bw = borderpx;
		XWindowChanges wc;
		wc.border_width = c->bw;
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc);
		updateborder(c);
	}
}


void
tile4(Monitor *m)
{
	unsigned int i, n, h, mw,mx, my, ty;
	Client *c;

	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
	if (n == 0)
		return;

	float soverflowfact = 0.2;
	int soverflow = soverflowfact * m->ww;
	int woffsetx = 0;
	int focuspos = 0;
	
	for (i = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
	{
		if (i < m->nmaster) continue;
		if (i%2 == 0 && c->isfocused) {
			focuspos = -1;
		}
		if (i%2 == 1 && c->isfocused) {
			focuspos = 1;
		}
	}
	if (focuspos == -1) {
		woffsetx = soverflow;
	}
	if (focuspos == 1) {
		woffsetx = -soverflow;
	}

	if (n > m->nmaster)
		/*mw = m->nmaster ? m->ww * m->pertag->mfacts[gettagindex(m->tagset[m->seltags]) + 1] : 0;*/
		// 每次arrange之前都会把m->mfact 设置成当前tag的mfact, 所以这里这样写也没问题. see view(const Arg *arg)
		mw = m->nmaster ? (m->ww - m->gap->gappx) * m->mfact : 0;
	else
		mw = m->ww - m->gap->gappx;
	
	if (n > m->nmaster + 1)
	{
		mx = (m->ww - m->gap->gappx) * ((1-m->mfact)/2) + m->gap->gappx;
	}
	else if (n > m->nmaster)
	{
		mx = m->wx + m->gap->gappx;
		/*mx = (m->ww - m->gap->gappx) * (1-m->mfact) + m->gap->gappx;*/
	}
	else
	{
		mx = m->wx + m->gap->gappx;
	}

	
	for (i = 0, my = m->gap->gappx, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
		if (i < m->nmaster) {
			h = (m->wh - my) / (MIN(n, m->nmaster) - i) - m->gap->gappx;
			resize(c, mx + woffsetx, m->wy + my, mw - (2*c->bw) - m->gap->gappx, h - (2*c->bw), 0);
			if (my + HEIGHT(c) + m->gap->gappx < m->wh)
				my += HEIGHT(c) + m->gap->gappx;
		}

	int sn0 = (n-m->nmaster)/2 + (1-m->nmaster%2);
	int sn1 = n-m->nmaster - sn0;
	int ti;
	for (ti = 0,i = 0, ty = m->gap->gappx, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
	{
		if (i < m->nmaster) continue;
		if( i%2 != 0) continue;
		/*h = (m->wh - ty) / (n - i) - m->gap->gappx;*/
		int sx = m->wx + m->gap->gappx;
		int sw = mx - 2*m->gap->gappx;
		sw += soverflow;
		sx -= soverflow;
		sx += woffsetx;
		h = (int)((m->wh - ty - m->gap->gappx) / (sn0-ti));
		resizeclient(c,sx , m->wy + ty, sw, h - (2*c->bw));
		if (ty + HEIGHT(c) + m->gap->gappx < m->wh)
			ty += HEIGHT(c) + m->gap->gappx;
		ti ++;
	}

	for (ti = 0, i = 0, ty = m->gap->gappx, c = nexttiled(m->clients); c ; c = nexttiled(c->next), i++)
	{
		if (i < m->nmaster) continue;
		if( i%2 != 1) continue;
		/*h = (m->wh - ty) / (n - i) - m->gap->gappx;*/
		int sx = mx + mw; 
		int sw = m->ww - mx - mw - m->gap->gappx;
		sw += soverflow;
		sx += woffsetx;
		h = (int)((m->wh - ty - m->gap->gappx) / (sn1-ti));
		resizeclient(c, sx, m->wy + ty, sw, h - (2*c->bw));
		if (ty + HEIGHT(c) + m->gap->gappx < m->wh)
			ty += HEIGHT(c) + m->gap->gappx;
		ti ++;
	}

	for (c = nexttiled(m->clients); c; c = nexttiled(c->next)){
		c->bw = borderpx;
		XWindowChanges wc;
		wc.border_width = c->bw;
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc);
		updateborder(c);
	}
}

void
tile5(Monitor *m)
{

	unsigned int i, n, h, mw,mx, my, ty;
	Client *c;
	int gapx = 7;
	int gapy = 7;

	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
	if (n == 0)
		return;

	// reverse
	Client *tiledcs[n];
	for (i = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
		tiledcs[n-i-1] = c;
	int tsn = n;
	rect_t ts[tsn];
	memset(ts, 0, sizeof(ts));
	rect_t sc;
	sc.x = 1;
	sc.y = 1;
	sc.w = selmon->ww - 1;
	sc.h = selmon->wh - 1;
	/*for (i = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)*/
	for(i = 0;i<n;i++)
	{
		c = tiledcs[i];
		int fillblockn = 5;
		float radio = 0.8;
		// 下面三个式子联合先求出initblock, 然后算出其他
		/*neww = stepw * initblock;*/
		/*stepw = (sc.w - neww) / (n-1)*/
		/*radio = initblock * stepw / sc.w*/
	        
		int initblock = (radio * (fillblockn-1)) / (1-radio);
		int stepw = sc.w  / (fillblockn-1 + initblock);
		int steph = sc.h  / (fillblockn-1 + initblock);
		int neww = initblock*stepw;
		int newh = initblock*steph;

		if(c->placed) 
		{
			neww = c->w + 2*gapx;
			newh = c->h + 2*gapy;
		}

		rect_t r;
		int radiostepn = 1;
		double maxintersectradiostep[] = {0.0};
		int ok = 0;
		int radioi;
		for(radioi = 0;radioi<radiostepn;radioi++){
			ok = fill2x(sc, neww, newh, stepw, steph, fillblockn, ts, i, &r, maxintersectradiostep[radioi]);
			if(ok) break;
		}
		if(!ok)
		{
			r.x = (selmon->ww - neww) / 2;
			r.y = (selmon->wh - newh) / 2;
			r.w = neww;
			r.h = newh;
		}

		c->x = r.x;
		c->y = r.y;
		c->w = r.w;
		c->h = r.h;

		ts[i].x = c->x;
		ts[i].y = c->y;
		ts[i].w = c->w;
		ts[i].h = c->h;

		/*LOG_FORMAT("tile5 c->x,y,name %d %d %d %d, %s",c->x, c->y,c->w,c->h, c->name);*/

	}

	// move the axis
	if (!selmon->sel->isfloating) {
		int offsetx = sc.w / 2 - (selmon->sel->w / 2 + selmon->sel->x);
		int offsety = sc.h / 2 - (selmon->sel->h / 2 + selmon->sel->y);

		for (i = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
		{
			resizeclient(c,c->x+offsetx + gapx,c->y+offsety+gapy, c->w - 2*gapx, c->h-2*gapy);
			/*c->placed = 1;*/
		}
	}

	for (c = nexttiled(m->clients); c; c = nexttiled(c->next)){
		c->bw = borderpx;
		XWindowChanges wc;
		wc.border_width = c->bw;
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc);
		updateborder(c);
	}
}



int 
fillspiral(rect_t sc, int w, int h, int i, rect_t ts[], int tsn, rect_t *r)
{
	if(!w || !h) return 0;
	MXY matcoor = spiral_index[i];
	r->w = w;
	r->h = h;
	r->x = matcoor.col * w - w / 2;
	r->y = matcoor.row * h - h / 2;
	return 1;
}

void
replacechar(char *buf, char old, char new)
{
	int i = 0;
 	while(buf[i] != '\0')
	{
		if(buf[i] == old)
		{
		    buf[i] = new;
		}
		i++;
	}
}

int
pyresort(Client *cs[], int n, int resorted[])
{
	if(n==0) return 0;
	// diable for now
	return 0;
	int i;
	char *url = "http://localhost:8666";
	char params[3000];
	memset(params, 0, sizeof(params));
	strcat(params, "names=");
	for(i=0;i<n;i++)
	{
		char name[256];
		strcpy(name, cs[i]->name);
		replacechar(name, ',',' ');
		strcat(params, name);
		if(i!=n-1)
			strcat(params, ",");
	}
	
	strcat(params, "&");
	strcat(params, "classes=");
	for(i=0;i<n;i++)
	{
		char *class = cs[i]->class;
		replacechar(class, ',',' ');
		strcat(params, class);
		if(i!=n-1)
			strcat(params, ",");
	}

	struct HttpResponse resp;
	resp.content = malloc(1);
	resp.size = 0;
	resp.code = CURLE_OK;
	int ok = httppost(url,params, &resp);
	if (!ok) return 0;

	int j = 0;
	char *temp = strtok(resp.content,",");
	while(temp)
	{
		LOG_FORMAT("pyresort %s", temp);
		sscanf(temp,"%d",&resorted[j]);
		j++;
		temp = strtok(NULL,",");
	}
	free(resp.content);
	return 1;
}


int
pyresort2(Client *cs[], int n, int resorted[])
{
	if(n==0) return 0;
	int i;
	char *url = "http://localhost:8666/resort";
	char params[3000];
	memset(params, 0, sizeof(params));

	strcat(params, "tag=");
	char tag[2];
	sprintf(tag, "%d", getcurtagindex(selmon));
	strcat(params, tag);
	strcat(params, "&");
	strcat(params, "launchparents=");
	for(i=0;i<n;i++)
	{
		Client *lp = cs[i]->launchparent;
		char str[3];
		int foundindex = -1;
		int j;
		for(j=0;j<n;j++)
		{
			if (lp == cs[j]) {
				foundindex = j;
				break;
			}
		}
		sprintf(str, "%d", foundindex);
		strcat(params, str);
		if(i!=n-1)
			strcat(params, ",");
	}

	strcat(params, "&");
	strcat(params, "ids=");
	for(i=0;i<n;i++)
	{
		Client *c = cs[i];
		char str[3];
		sprintf(str, "%d", c->id);
		strcat(params, str);
		if(i!=n-1)
			strcat(params, ",");
	}

	struct HttpResponse resp;
	resp.content = malloc(1);
	resp.size = 0;
	resp.code = CURLE_OK;
	int ok = httppost(url,params, &resp);
	if (!ok) return 0;

	int j = 0;
	char *temp = strtok(resp.content,",");
	while(temp)
	{
		/*LOG_FORMAT("pyresort2 %s", temp);*/
		sscanf(temp,"%d",&resorted[j]);
		j++;
		temp = strtok(NULL,",");
	}
	free(resp.content);
	return 1;
}

void
tile6(Monitor *m)
{

	unsigned int i, n, h, mw,mx, my, ty;
	Client *c;
	int gapx = 7;
	int gapy = 7;
	if (tile6initwinfactor == 1) {
		gapx = 0;
		gapy = 0;
	}

	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
	if (n == 0)
		return;

	// reverse
	Client *tiledcs[n];
	for (i = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
	{
		c->launchindex = n-i-1;
		tiledcs[n-i-1] = c;
	}
	int tsn = n;
	rect_t ts[tsn];
	memset(ts, 0, sizeof(ts));
	rect_t sc;
	sc.x = 0;
	sc.y = 0;
	sc.w = selmon->ww;
	sc.h = selmon->wh;
	int initn = 121;
	int resorted[initn];
	memset(resorted, -1, sizeof(resorted));
	int resortok = pyresort2(tiledcs, n, resorted);
	/*LOG_FORMAT("%d %d", resorted[0], resorted[1]);*/
	/*for(i = 0;i<n;i++)*/
	if(resortok)
	{
		for(i = 0;i<initn;i++)
		{
			int resorteindex = resorted[i];
			if(resorteindex < 0 || resorteindex >= initn) continue;
			c = tiledcs[resorteindex];
			/*c = tiledcs[i];*/
			int neww = sc.w * tile6initwinfactor;
			/*int newh = sc.h * tile6initwinfactor;*/
			int newh = sc.h;

			if(c->placed) 
			{
				neww = c->w + 2*gapx;
				newh = c->h + 2*gapy;
			}

			rect_t r;
			int ok = 0;
			ok = fillspiral(sc, neww, newh, i, ts, i, &r);
			if(!ok)
			{
				r.x = (selmon->ww - neww) / 2;
				r.y = (selmon->wh - newh) / 2;
				r.w = neww;
				r.h = newh;
			}

			c->x = r.x;
			c->y = r.y;
			c->w = r.w;
			c->h = r.h;
			c->matcoor = spiral_index[i];

			ts[resorteindex].x = c->x;
			ts[resorteindex].y = c->y;
			ts[resorteindex].w = c->w;
			ts[resorteindex].h = c->h;

		}
	}
	else 
	{
		for(i = 0;i<n;i++)
		{
			c = tiledcs[i];
			int neww = sc.w * tile6initwinfactor;
			/*int newh = sc.h * tile6initwinfactor;*/
			int newh = sc.h;

			if(c->placed) 
			{
				neww = c->w + 2*gapx;
				newh = c->h + 2*gapy;
			}

			rect_t r;
			int ok = 0;
			ok = fillspiral(sc, neww, newh, i, ts, i, &r);
			if(!ok)
			{
				r.x = (selmon->ww - neww) / 2;
				r.y = (selmon->wh - newh) / 2;
				r.w = neww;
				r.h = newh;
			}

			c->x = r.x;
			c->y = r.y;
			c->w = r.w;
			c->h = r.h;
			c->matcoor = spiral_index[i];

			ts[i].x = c->x;
			ts[i].y = c->y;
			ts[i].w = c->w;
			ts[i].h = c->h;
		}
	}

	// move the axis
	if (!selmon->sel->isfloating) {
		int offsetx = sc.w / 2 - (selmon->sel->w / 2 + selmon->sel->x);
		int offsety = sc.h / 2 - (selmon->sel->h / 2 + selmon->sel->y);
		offsetx = offsetx - (sc.w - selmon->sel->w)/2;
		offsety = offsety - (sc.h - selmon->sel->h)/2;

		for (i = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
		{
			c->bw = 0;
			resizeclient(c,c->x+offsetx + gapx,c->y+offsety+gapy, c->w - 2*gapx, c->h-2*gapy);
			/*c->placed = 1;*/
		}
	}

	/*for (c = nexttiled(m->clients); c; c = nexttiled(c->next)){*/
		/*[>c->bw = borderpx;<]*/
		/*XWindowChanges wc;*/
		/*wc.border_width = c->bw;*/
		/*XConfigureWindow(dpy, c->win, CWBorderWidth, &wc);*/
		/*updateborder(c);*/
	/*}*/
}

void
tile6zoom(const Arg *arg)
{
	tile6initwinfactor += arg->f;
	arrange(selmon);
}

void
tile6maximize(const Arg *arg)
{
	if (tile6initwinfactor == 1) {
		tile6initwinfactor = lasttile6initwinfactor;
		drawswitchersticky(selmon);
	}else{
		lasttile6initwinfactor = tile6initwinfactor;
		tile6initwinfactor = 1.0;
		destroyswitchersticky(selmon);
	}
	arrange(selmon);
}

int
pyresort3(Container *cs[], int n, int resorted[])
{
	if(n==0) return 0;
	int i;
	char *url = "http://localhost:8666/resort";
	char params[3000];
	memset(params, 0, sizeof(params));

	strcat(params, "tag=");
	char tag[2];
	sprintf(tag, "%d", getcurtagindex(selmon));
	strcat(params, tag);
	strcat(params, "&");
	strcat(params, "launchparents=");
	for(i=0;i<n;i++)
	{
		Container *lp = cs[i]->launchparent;
		char str[5];
		int foundindex = -1;
		int j;
		for(j=0;j<n;j++)
		{
			if (lp == cs[j]) {
				foundindex = j;
				break;
			}
		}
		sprintf(str, "%d", foundindex);
		strcat(params, str);
		if(i!=n-1)
			strcat(params, ",");
	}

	strcat(params, "&");
	strcat(params, "ids=");
	for(i=0;i<n;i++)
	{
		Container *c = cs[i];
		char str[5];
		sprintf(str, "%d", c->id);
		strcat(params, str);
		if(i!=n-1)
			strcat(params, ",");
	}

	LOG_FORMAT("pysort3 %s", params);
	struct HttpResponse resp;
	resp.content = malloc(1);
	resp.size = 0;
	resp.code = CURLE_OK;
	int ok = httppost(url,params, &resp);
	if (!ok) return 0;

	int j = 0;
	char *temp = strtok(resp.content,",");
	while(temp)
	{
		LOG_FORMAT("pyresort3 %s", temp);
		sscanf(temp,"%d",&resorted[j]);
		j++;
		temp = strtok(NULL,",");
	}
	free(resp.content);
	return 1;
}

Container *
createcontainer()
{
	Container *container = (Container *)malloc(sizeof(Container));
	memset(container, 0, sizeof(Container));
	return container;
}

Container *
createcontainerc(Client *c)
{
	Container *container = (Container *)malloc(sizeof(Container));
	memset(container, 0, sizeof(Container));
	container->id = c->id;
	container->cs[container->cn] = c;
	container->cn ++;
	container->masterfactor = 1.3;
	c->container = container;
	c->containerrefc = NULL;
	return container;
}

void 
removeclientfromcontainer(Container *container, Client *c)
{
	if (!container) return;
	int found = 0;
	int i;
	for(i=0;i<container->cn;i++)
	{
		if(c == container->cs[i])
		{
			found = 1;
			break;
		}
	}
	if (!found) return;

	LOG_FORMAT("removeclientfromcontainer 1");
	if (c->containerrefc) {
		c->containerrefc->containerrefc = NULL;
		c->containerrefc = NULL;
	}
	
	if (i == container->cn-1) {
		container->cn --;
		return;
	}
	LOG_FORMAT("removeclientfromcontainer 2");
	int j;
	for(j=i;j<(container->cn-1);j++)
	{
		container->cs[j] = container->cs[j+1];
	}
	container->cn --;
	LOG_FORMAT("removeclientfromcontainer 3");
}

void
freecontainerc(Container *container, Client *c)
{
	if (!container) return;
	removeclientfromcontainer(container, c);
	if (container->cn == 0)
		free(container);
}

void
tile7(Monitor *m)
{
	if (m->sel && m->sel->isfloating) return;
	LOG_FORMAT("tile7 1");
	unsigned int i,j, n, h, mw,mx, my, ty;
	Client *c;
	Container *container;
	/*int gapx = 7;*/
	/*int gapy = 0;*/
	int gapx = 0;
	int gapy = 0;
	if (tile6initwinfactor == 1) {
		gapx = 0;
		gapy = 0;
	}

	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
	if (n == 0)
		return;

	// reverse
	Client *clients[n];
	for (i = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
	{
		c->launchindex = n-i-1;
		clients[n-i-1] = c;
	}

	Container *tiledcs[n];
	memset(tiledcs, 0, sizeof(tiledcs));
	int ctn = 0;
	for(i=0;i<n;i++)
	{
		Client *c = clients[i];
		int found = 0;
		for(j=0;j<ctn;j++)
		{
			if(tiledcs[j]->id == c->container->id){
				found = 1;
				break;
			}
		}
		if(!found)
		{
			c->container->launchindex = ctn;
			tiledcs[ctn] = c->container;
			ctn++;
		}
	}

	LOG_FORMAT("tile7 2");

	int tsn = n;
	rect_t ts[tsn];
	memset(ts, 0, sizeof(ts));
	rect_t sc;
	sc.x = m->wx;
	sc.y = m->wy;
	sc.w = m->ww;
	sc.h = m->wh;
	LOG_FORMAT("sc.x %d sc.y %d", sc.x, sc.y);
	int initn = 121;
	int resorted[initn];
	memset(resorted, -1, sizeof(resorted));
	int resortok = pyresort3(tiledcs, ctn, resorted);
	
	/*LOG_FORMAT("%d %d", resorted[0], resorted[1]);*/
	if(resortok)
	{
		for(i = 0;i<initn;i++)
		{
			int resorteindex = resorted[i];
			/*LOG_FORMAT("tile7 11 %d", resorteindex);*/
			if(resorteindex < 0 || resorteindex >= initn) continue;
			container = tiledcs[resorteindex];
			/*c = tiledcs[i];*/
			int neww = sc.w * tile6initwinfactor;
			/*int newh = sc.h * tile6initwinfactor;*/
			int newh = sc.h;

			if(container->placed) 
			{
				neww = container->w + 2*gapx;
				newh = container->h + 2*gapy;
			}

			rect_t r;
			int ok = 0;
			ok = fillspiral(sc, neww, newh, i, ts, i, &r);
			if(!ok)
			{
				r.x = (m->ww - neww) / 2;
				r.y = (m->wh - newh) / 2;
				r.w = neww;
				r.h = newh;
			}

			container->x = r.x;
			container->y = r.y;
			container->w = r.w;
			container->h = r.h;
			container->matcoor = spiral_index[i];

			ts[resorteindex].x = container->x;
			ts[resorteindex].y = container->y;
			ts[resorteindex].w = container->w;
			ts[resorteindex].h = container->h;

		}
	}
	else 
	{
		for(i = 0;i<n;i++)
		{
			container = tiledcs[i];
			int neww = sc.w * tile6initwinfactor;
			/*int newh = sc.h * tile6initwinfactor;*/
			int newh = sc.h;

			if(container->placed) 
			{
				neww = container->w + 2*gapx;
				newh = container->h + 2*gapy;
			}

			rect_t r;
			int ok = 0;
			ok = fillspiral(sc, neww, newh, i, ts, i, &r);
			if(!ok)
			{
				r.x = (m->ww - neww) / 2;
				r.y = (m->wh - newh) / 2;
				r.w = neww;
				r.h = newh;
			}

			container->x = r.x;
			container->y = r.y;
			container->w = r.w;
			container->h = r.h;
			container->matcoor = spiral_index[i];

			ts[i].x = container->x;
			ts[i].y = container->y;
			ts[i].w = container->w;
			ts[i].h = container->h;
		}
	}

	LOG_FORMAT("tile7 3");

	for(i=0;i<ctn;i++)
	{
		if (tiledcs[i]->cn > 0) {
			int perw = tiledcs[i]->w / tiledcs[i]->cn;
			int j;
			int nextx = 0;
			int splited = tiledcs[i]->cn > 1;
			for(j=0;j<tiledcs[i]->cn;j++){
				c = tiledcs[i]->cs[j];
				float cfactor = splited ? (j==0?tiledcs[i]->masterfactor:(2-tiledcs[i]->masterfactor)): 1;
				c->w = perw * cfactor;
				c->x = tiledcs[i]->x + nextx;
				c->y = tiledcs[i]->y;
				c->h = tiledcs[i]->h;
				nextx += c->w;
				c->matcoor = tiledcs[i]->matcoor;
				LOG_FORMAT("tile7 10 %d,%d,%d,%d %s containerid:%d", c->x, c->y, c->w, c->h, c->name, tiledcs[i]->id);
			}
		}
	}

	LOG_FORMAT("tile7 4");

	// move the axis
	if (m->sel && !m->sel->isfloating) {
		if(!m->sel->container) return;
		int selctx = m->sel->container->x;
		int selcty = m->sel->container->y;
		int selctw = m->sel->container->w;
		int selcth = m->sel->container->h;
		LOG_FORMAT("tile7 7 %d,%d,%d,%d %d", selctx, selcty, selctw, selcth, m->sel->container->id);
		// cxy的座标系: 0,0处的窗口中心点 在座标 0,0处, 将所选的窗口移动到0,0处需要 -selctx, -selcty, (selctx,selcty为原座标系座标)
		// 移动到左上角
		int offsetx = - selctx + sc.x;
		int offsety = - selcty + sc.y;
		LOG_FORMAT("tile7 8 %d,%d", offsetx, offsety);

		for(i=0;i<ctn;i++)
		{
			container = tiledcs[i];
			container->x = container->x + offsetx + gapx;
			container->y = container->y + offsety + gapy;
		}

		for (i = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
		{
			c->bw = 0;
			resizeclient(c,c->x+offsetx + gapx,c->y+offsety+gapy, c->w - 2*gapx, c->h-2*gapy);
			LOG_FORMAT("tile7 9 %d,%d,%d,%d %s", c->x, c->y, c->w, c->h, c->name);
			LOG_FORMAT("tile7 9 %d,%d,%d,%d %s containerid:%d", c->container->x, c->container->y, c->container->w, c->container->h, c->name, c->container->id);
			/*c->placed = 1;*/
		}
	}

	/*for (c = nexttiled(m->clients); c; c = nexttiled(c->next)){*/
		/*[>c->bw = borderpx;<]*/
		/*XWindowChanges wc;*/
		/*wc.border_width = c->bw;*/
		/*XConfigureWindow(dpy, c->win, CWBorderWidth, &wc);*/
		/*updateborder(c);*/
	/*}*/

	LOG_FORMAT("tile7 5");
}

void
expand(const Arg *arg)
{
	float incr = arg->f;
	selmon->sel->container->masterfactor += incr;
	if(selmon->sel->container->masterfactor > 1.7) selmon->sel->container->masterfactor = 1.7;
	if(selmon->sel->container->masterfactor < 0.5) selmon->sel->container->masterfactor = 0.5;
	arrange(selmon);
	selmon->switcheraction.drawfunc(selmon->switcher, selmon->switcherww, selmon->switcherwh);
}

void
tiledualclient(Client *c1,Client *c2)
{
	if(doubled) {
		cleardoublepage(2);
		return;
	}
	if (!c1 || !c2) {
		return;
	}
	doublepagemarkclient(c1);
	doublepagemarkclient(c2);
}

void
tiledual(const Arg *arg)
{
	Client *cnext;
	do {
		cnext = nextclosestc(arg);
	}while (cnext && cnext->isfloating);
	tiledualclient(selmon->sel, cnext);
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

	// 这里故意偏差1px, 为了不让scratchgroup中的窗口与全屏的窗口x,y重合, 导致键盘focus选不中
	sc.x = 1;
	sc.y = 1;
	sc.w = selmon->ww - 1;
	sc.h = selmon->wh - 1;
	int i = 0;
	LOG_FORMAT("showscratchgroup: before focus and arrange -1");
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

		LOG_FORMAT("showscratchgroup: isplaced: %d, si->x: %d, si->y: %d", si->placed, si->x, si->y);
		c->x = si->x;
		c->y = si->y;
		
		ts[i].x = si->x;
		ts[i].y = si->y;
		ts[i].w = si->w;
		ts[i].h = si->h;

		i++;
	}
	LOG_FORMAT("showscratchgroup: before focus and arrange");
	for(si = sg->head->next; si && si != sg->tail; si = si->next)
	{
		if(si->c) si->c->isfloating = 1;
		// focus(si->c);
		// arrange(selmon);
		XRaiseWindow(dpy, si->c->win);
		resize(si->c,si->x,si->y, si->w,si->h,0);
	}
	LOG_FORMAT("showscratchgroup: before focus and arrange 2");
	if (sg->lastfocused)
		focus(sg->lastfocused);
	else if (sg->head->next && sg->head->next->c)
		focus(sg->head->next->c);
	LOG_FORMAT("showscratchgroup: before focus and arrange 3");
	arrange(selmon);
	sg->isfloating = 1;
}


ScratchItem 
*findscratchitem(Client *c, ScratchGroup *sg){
	ScratchItem *sitmp = NULL;
	for (sitmp = sg->tail->prev; sitmp && sitmp != sg->head; sitmp = sitmp->prev)
	{
		if (sitmp->c == c)
			return sitmp;
	}
	return NULL;
}

/**
 * @brief 隐藏单个scratchitem
 * 
 * @param si 
 * @param returntags 0:c设置为之前的tags
 */
void
hidescratchitem(ScratchItem *si, int returntags)
{
	// sg lastfocus
	ScratchGroup *sg= scratchgroupptr;
	sg->lastfocused = NULL;
	ScratchItem *sitmp = findscratchitem(selmon->sel, sg);
	if(sitmp) sg->lastfocused = selmon->sel;


	Client * c = si->c;
	if(!c) return;
	c->isfloating = 0;
	if (returntags)
		c-> tags = returntags;
	else
		if (si->pretags)
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
	if(!sg || !sg->isfloating) return;

	// sg lastfocus
	sg->lastfocused = NULL;
	ScratchItem *si;
	for (si = sg->tail->prev; si && si != sg->head; si = si->prev)
	{
		if (si->c == selmon->sel){
			sg->lastfocused =selmon->sel;
			break;
		}
	}


	for (si = sg->tail->prev; si && si != sg->head; si = si->prev)
	{
		Client *c = si->c;
		if (!c)
			continue;
		if (si->pretags)
			c->tags = si->pretags;
		else
			c->tags = selmon->tagset[selmon->seltags];

		if (c->tags == selmon->tagset[selmon->seltags])
		{
			c->tags = 1 << (LENGTH(tags) - 1);
		}

		if (c->isfloating){
			si->x = c->x;
			si->y = c->y;
			si->w = c->w;
			si->h = c->h;
			si->placed = 1;
		}
	}
	for (si = sg->tail->prev; si && si != sg->head; si = si->prev)
		if(si->c) si->c->isfloating = 0;
	if(isarrange){
		int curtags = selmon->tagset[selmon->seltags];
		Client *c;
		for(c = selmon->stack;c;c=c->snext){
			int found = 0;
			for (si = sg->tail->prev; si && si != sg->head; si = si->prev){
				Client *itemc = si->c;
				if (itemc == c){
					found = 1;
					break;
				}
			}
			if (ISVISIBLE(c) && !found)
				break;
		}
		if(c) focus(c); else focus(NULL);
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
		hidescratchitem(found, 0);
		if (c == scratchgroupptr->lastfocused)
			scratchgroupptr->lastfocused = NULL;
		free_si(found);
	}else{
		if(c == scratchgroupptr->lastfocused) 
			scratchgroupptr->lastfocused = NULL;
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

#define INIT_XY -1061109568

#define SETINPOINT(x,y,t, tx,ty)  if(ispointin(x,y,t)){tx=x;ty=y;}

#define RETURNINTERSECT(i,j,x1,y1,x2,y2, g,t) if(x1[i]>INIT_XY && y1[i]>INIT_XY && x2[j]>INIT_XY && y2[j]>INIT_XY) return 1.0*(abs(x2[j]-x1[i])*abs(y2[j]-y1[i]))/MIN((abs(g.w)*abs(g.h)),(abs(t.w)*abs(t.h)));

#define MAXX(g,t) mmax(4,g.x, g.x+g.w, t.x, t.x+t.w);
#define MAXY(g,t) mmax(4,g.y, g.y+g.h, t.y, t.y+t.h);
#define MINX(g,t) mmin(4,g.x, g.x+g.w, t.x, t.x+t.w);
#define MINY(g,t) mmin(4,g.y, g.y+g.h, t.y, t.y+t.h);

int 
mmin(int num, ...)
{
	va_list ap;
	va_start(ap,num);
	int result = INT_MAX;
	int temp = 0;
	int i;
	for (i = 0; i < num; i++) {
		temp = va_arg(ap, int); 
		result = MIN(result, temp);
	}
	va_end(ap);
	return result;
}
int 
mmax(int num, ...)
{
	va_list ap;
	va_start(ap,num);
	int result = INT_MIN;
	int temp = 0;
	int i;
	for (i = 0; i < num; i++) {
		temp = va_arg(ap, int); 
		result = MAX(result, temp);
	}
	va_end(ap);
	return result;
}

double 
interlinepercent(int x1, int w1, int x2, int w2)
{
	return (w1 + w2 - (MAX(x1, x2) - MIN(x1, x2)))/MIN(w1, w2);
}

double
intersectpercent(rect_t g, rect_t t)
{
	// rect 顺时针坐标
	/*int x1[4] = {INIT_XY,INIT_XY,INIT_XY,INIT_XY};*/
	/*int y1[4] = {INIT_XY,INIT_XY,INIT_XY,INIT_XY};*/
	/*int x2[4] = {INIT_XY,INIT_XY,INIT_XY,INIT_XY};*/
	/*int y2[4] = {INIT_XY,INIT_XY,INIT_XY,INIT_XY};*/

	/*SETINPOINT(g.x, g.y, t, x1[0], y1[0])*/
	/*SETINPOINT(g.x+g.w,g.y, t, x1[1], y1[1])*/
	/*SETINPOINT(g.x+g.w,g.y+g.h, t ,x1[2], y1[2])*/
	/*SETINPOINT(g.x, g.y+g.h, t ,x1[3], y1[3])*/
	
	/*SETINPOINT(t.x,t.y,g,x2[0],y2[0]) */
	/*SETINPOINT(t.x+t.w,t.y, g,x2[1],y2[1]) */
	/*SETINPOINT(t.x+t.w,t.y+t.h, g,x2[2],y2[2]) */
	/*SETINPOINT(t.x, t.y+t.h, g,x2[3],y2[3]) */
	
	/*RETURNINTERSECT(0,2,x1, y1, x2,y2,g,t)*/
	/*RETURNINTERSECT(2,0,x1, y1, x2,y2,g,t)*/
	/*RETURNINTERSECT(1,3,x1, y1, x2,y2,g,t)*/
	/*RETURNINTERSECT(3,1,x1, y1, x2,y2,g,t)*/
	/*if(g.x <= t.x && (g.x + g.w) >= (t.x+t.w) && g.y >= t.y &&(g.y + g.h) <= (t.y + t.h)) return 1.0;*/
	/*if(t.x <= g.x && (t.x + t.w) >= (g.x+g.w) && t.y >= g.y &&(t.y + t.h) <= (g.y + g.h)) return 1.0;*/

	int maxx = MAXX(g, t);
	int minx = MINX(g, t);
	int maxy = MAXY(g, t);
	int miny = MINY(g, t);
	int interw = (g.w + t.w - (maxx - minx));
	int interh = (g.h + t.h - (maxy - miny));
	if (interw > 0 && interh > 0) {
		return (1.0*interw*interh) / MIN((abs(g.w)*abs(g.h)),(abs(t.w)*abs(t.h)));
	}
	return 0;
}

int 
isintersect(rect_t g, rect_t ts[], int tsn)
{
	int i;
	for(i = 0; i<tsn; i++)
	{
		rect_t t = ts[i];
		if(intersectpercent(g,t) > 0.0) return 1;
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

/**
 * @brief 把sc-(w*h)分为(n-1)*(n-1)块, 从中心开始尝试放入, 向外扩散转圈, 直到有一个满足与其他窗口(ts)交叉比例(maxintersectradio), 得到的结果放入r
 * 
 * @param sc 
 * @param w 
 * @param h 
 * @param n 
 * @param ts 
 * @param tsn 
 * @param r 
 * @param maxintersectradio 
 * @return int 
 */
int
fill2x(rect_t sc, int w, int h, int stepw, int steph, int n, rect_t ts[], int tsn, rect_t *r, double maxintersectradio)
{
	// todo: center iterator
	// LOG_FORMAT("tsn:%d, w:%d, h:%d", tsn, w, h);
	int radioi;
	/*int stepw = (sc.w - w) /(n-1);*/
	/*int steph = (sc.h - h) /(n-1);*/
	int centeri = n/2;
	int centerj = n/2;
	int k;
	for(k = 0;;k++)
	{
		int i;
		int j;
		if(k == 0)
			if(tryfillone(calcx(sc,centeri,stepw, w), calcy(sc, centerj, steph, h), w, h, ts, tsn, r, maxintersectradio)) return 1;

		/*float interactradiox = {0.7,0.3,0.0};*/
		/*float interactradioy = {0.7,0.3,0.0};*/

		i = centeri - k;
		for(j=centerj;j<centerj+k;j++)
		{
			/*if(i>=n || i <0 || j>=n || j <0 ) continue;*/
			if(tryfillone(calcx(sc,i,stepw, w), calcy(sc, j, steph, h), w, h, ts, tsn, r, maxintersectradio)) return 1;
		}

		j = centerj + k;
		for(i=centeri;i<centeri+k;i++)
		{
			/*if(i>=n || i <0 || j>=n || j <0 ) continue;*/
			if(tryfillone(calcx(sc,i,stepw, w), calcy(sc, j, steph, h), w, h, ts, tsn, r, maxintersectradio)) return 1;
		}


		j = centerj - k;
		for(i=centeri;i>centeri-k;i--)
		{
			/*if(i>=n || i <0 || j>=n || j <0 ) continue;*/
			if(tryfillone(calcx(sc,i,stepw, w), calcy(sc, j, steph, h), w, h, ts, tsn, r, maxintersectradio)) return 1;
		}

		i = centeri + k;
		for(j=centerj;j>centerj-k;j--)
		{
			/*if(i>=n || i <0 || j>=n || j <0 ) continue;*/
			if(tryfillone(calcx(sc,i,stepw, w), calcy(sc, j, steph, h), w, h, ts, tsn, r, maxintersectradio)) return 1;
		}

		
		if(tryfillone(calcx(sc,centeri-k,stepw, w), calcy(sc, centerj-k, steph, h), w, h, ts, tsn, r, maxintersectradio)) return 1;
		if(tryfillone(calcx(sc,centeri-k,stepw, w), calcy(sc, centerj+k, steph, h), w, h, ts, tsn, r, maxintersectradio)) return 1;
		if(tryfillone(calcx(sc,centeri+k,stepw, w), calcy(sc, centerj+k, steph, h), w, h, ts, tsn, r, maxintersectradio)) return 1;
		if(tryfillone(calcx(sc,centeri+k,stepw, w), calcy(sc, centerj-k, steph, h), w, h, ts, tsn, r, maxintersectradio)) return 1;
	}
	
	return 0;
}

/**
 * @brief 把sc-(w*h)分为(n-1)*(n-1)块, 从中心开始尝试放入, 向外扩散转圈, 直到有一个满足与其他窗口(ts)交叉比例(maxintersectradio), 得到的结果放入r
 * 
 * @param sc 
 * @param w 
 * @param h 
 * @param n 
 * @param ts 
 * @param tsn 
 * @param r 
 * @param maxintersectradio 
 * @return int 
 */
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
	if (setfocus) {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
	c->isfocused = False;
	selmon->sel = NULL;
	updateborder(c);
}

void
unmanage(Client *c, int destroyed)
{
	Monitor *m = c->mon;
	XWindowChanges wc;

	Client *tmpc;
	for(tmpc = m->clients;tmpc;tmpc = tmpc->next)
		if (tmpc->launchparent) 
			tmpc->launchparent = NULL;

	removefromscratchgroupc(c);
	removefromfocuschain(c);
	detach(c);
	detachstack(c);
	freeicon(c);
	freeicons(c);
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
	freecontainerc(c->container, c);
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

	m->switcheraction.drawfunc(m->switcher, m->switcherww, m->switcherwh);
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
	c->hintsvalid = 1;
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
	
	if (m->switcher) {
		/*m->systrayrx = m->switcherbarwx + m->switcherbarww;*/
		/*m->systrayy = m->switcherbarwy;*/
		m->systrayrx = m->ww + m->wx;
		m->systrayy = m->by + m->wy;
	}else{
		m->systrayrx = m->ww + m->wx;
		m->systrayy = m->by + m->wy;
	}

	unsigned int x = m->systrayrx;
	unsigned int y = m->systrayy;
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
	XMoveResizeWindow(dpy, systray->win, x, y, w, bh);
	wc.x = x; wc.y = y; wc.width = w; wc.height = bh;
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
	if (c->name[0] == '\0') /* hack to mark broken clients */
		strcpy(c->name, broken);
}

void
updateclass(Client *c)
{
	getclass(c->win, c->class);
}

void
updatewindowtype(Client *c)
{
	Atom state = getatomprop(c, netatom[NetWMState]);
	Atom wtype = getatomprop(c, netatom[NetWMWindowType]);

	if (state == netatom[NetWMFullscreen])
		setfullscreen(c, 1);
	if (wtype == netatom[NetWMWindowTypeDialog]){
		c->isfloating = 1;
		c->nstub = 0;
	}
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


static uint32_t prealpha(uint32_t p) {
	uint8_t a = p >> 24u;
	uint32_t rb = (a * (p & 0xFF00FFu)) >> 8u;
	uint32_t g = (a * (p & 0x00FF00u)) >> 8u;
	return (rb & 0xFF00FFu) | (g & 0x00FF00u) | (a << 24u);
}

Picture
geticonpropv(Window win, unsigned int *picw, unsigned int *pich, int icon_size)
{
	int format;
	unsigned long n, extra, *p = NULL;
	Atom real;

	if (XGetWindowProperty(dpy, win, netatom[NetWMIcon], 0L, LONG_MAX, False, AnyPropertyType, 
						   &real, &format, &n, &extra, (unsigned char **)&p) != Success)
		return None; 
	if (n == 0 || format != 32) { XFree(p); return None; }

	unsigned long *bstp = NULL;
	uint32_t w, h, sz;
	{
		unsigned long *i; const unsigned long *end = p + n;
		uint32_t bstd = UINT32_MAX, d, m;
		for (i = p; i < end - 1; i += sz) {
			if ((w = *i++) >= 16384 || (h = *i++) >= 16384) { XFree(p); return None; }
			if ((sz = w * h) > end - i) break;
			if ((m = w > h ? w : h) >= icon_size && (d = m - icon_size) < bstd) { bstd = d; bstp = i; }
		}
		if (!bstp) {
			for (i = p; i < end - 1; i += sz) {
				if ((w = *i++) >= 16384 || (h = *i++) >= 16384) { XFree(p); return None; }
				if ((sz = w * h) > end - i) break;
				if ((d = icon_size - (w > h ? w : h)) < bstd) { bstd = d; bstp = i; }
			}
		}
		if (!bstp) { XFree(p); return None; }
	}

	if ((w = *(bstp - 2)) == 0 || (h = *(bstp - 1)) == 0) { XFree(p); return None; }

	uint32_t icw, ich;
	if (w <= h) {
		ich = icon_size; icw = w * icon_size / h;
		if (icw == 0) icw = 1;
	}
	else {
		icw = icon_size; ich = h * icon_size / w;
		if (ich == 0) ich = 1;
	}
	*picw = icw; *pich = ich;

	uint32_t i, *bstp32 = (uint32_t *)bstp;
	for (sz = w * h, i = 0; i < sz; ++i) bstp32[i] = prealpha(bstp[i]);

	Picture ret = drw_picture_create_resized(drw, (char *)bstp, w, h, icw, ich);
	XFree(p);

	return ret;
}

Picture
geticonprop(Window win, unsigned int *picw, unsigned int *pich)
{
	return geticonpropv(win, picw, pich, ICONSIZE);
}

void
freeicon(Client *c)
{
	if (c->icon) {
		XRenderFreePicture(dpy, c->icon);
		c->icon = None;
	}
}

void
freeicons(Client *c)
{
	int i;
	for(i = 0;i<3;i++)
	{
		if (c->icons[i]) {
			XRenderFreePicture(dpy, c->icons[i]);
			c->icons[i] = None;
		}
	}
}

void
updateicon(Client *c)
{
	freeicon(c);
	c->icon = geticonprop(c->win, &c->icw, &c->ich);
}

void
updateicons(Client *c)
{
	freeicons(c);
	int i;
	for(i = 0;i<3;i++)
	{
		int j = pow(2,i);
		c->icons[i] = geticonpropv(c->win, &(c->icws[i]), &(c->ichs[i]), j * 32);
	}
}

void
view(const Arg *arg)
{
	viewui(arg->ui);
	focus(NULL);
	arrange(selmon);
	updatecurrentdesktop();

	if (selmon->switcher) {
		selmon->switcheraction.drawfunc(selmon->switcher, selmon->switcherww, selmon->switcherwh);
	}
}

void
viewi(int tagindex)
{
	Arg arg = {.ui = 1<<tagindex};
	view(&arg);
}

void
viewui(unsigned int tagui)
{
	cleardoublepage(0);

	int i;
	unsigned int tmptag;

	unsigned oldseltags = selmon->seltags;
	int isoverview = (selmon->tagset[oldseltags] & TAGMASK) == TAGMASK;
	if ((tagui & TAGMASK) == selmon->tagset[oldseltags] // 与原来的tag相同
		&& !isoverview)									  // 原来不是tag全选
		return;
	selmon->seltags ^= 1; /* toggle sel tagset */
	if (tagui & TAGMASK) {
		if (tagui == ~0 && isoverview){
			// 如果所有tag都显示了,就回到原来的tag. 只要这里不赋值, 默认就是原来的. see: selmon->seltags ^= 1
			selmon->tagset[selmon->seltags] = selmon->sel->tags;
		}else{
			selmon->tagset[selmon->seltags] = tagui & TAGMASK;
		}
		selmon->pertag->prevtag = selmon->pertag->curtag;

		if (tagui == ~0 && scratchgroupptr && scratchgroupptr->isfloating)
			hidescratchgroupv(scratchgroupptr, 0);
		if (tagui == ~0){
			Client *tmpc;
			for(tmpc = selmon->clients; tmpc; tmpc = tmpc->next)
			{
				if(tmpc->isfloating && !tmpc->istemp) tmpc->isfloating = 0;
			}
		}

		if (tagui == ~0)
			selmon->pertag->curtag = 0;
		else {
			for (i = 0; !(tagui & 1 << i); i++) ;
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

	if((selmon->tagset[selmon->seltags] & TAGMASK) == TAGMASK){
		selmon->sellt = 0;
		selmon->lt[selmon->sellt] = &layouts[3];
		selmon->lt[selmon->sellt^1] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt^1];
	}

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

	if (tagui == ~0 && isoverview)
	{
		detach(selmon->sel);
		attach(selmon->sel);
	}
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
	unsigned int maxtags =getmaxtagstiled();
	unsigned int mintags =getmintagstiled();
	unsigned int nexttags = 0;
	if (arg->i > 0)
	{
		if ((selmon->tagset[selmon->seltags] & TAGMASK) == TAGMASK ) 
			nexttags = 1;
		else 
			nexttags = selmon->tagset[selmon->seltags] << arg->i;
		while (counttagtiled(selmon->clients, nexttags) == 0)
		{
			if (nexttags > maxtags)
			{
				nexttags = mintags ;
				break;
			}else {
				nexttags = nexttags << 1;
			}
		}
	}
	if (arg->i < 0)
	{
		if ((selmon->tagset[selmon->seltags] & TAGMASK) == TAGMASK ) 
			nexttags = maxtags;
		else 
			nexttags = selmon->tagset[selmon->seltags] >> -arg->i;
		while (counttagtiled(selmon->clients, nexttags) == 0)
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

#define ARRAY(arr,i,j) *((int*)arr + col*i + j); 

void 
left(int **arr, int row ,int col, int x, int y, int result[2])
{
	if (y == 0) {
		return;
	}
	int j;
	for(j = y - 1;j >= 0;j--)
	{
		int step;
		int i;
		for(step = 0; step < row; step ++)
		{
			i = x + step;
			int m = ARRAY(arr, i, j);
			if (i >= 0 && i < row) {
				if(m == 1)
				{
					result[0] = i;
					result[1] = j;
					return;
				}
			}
			i = x - step;
			m = ARRAY(arr, i, j);
			if (i >= 0 && i < row) {
				if(m == 1)
				{
					result[0] = i;
					result[1] = j;
					return;
				}
			}
		}
	}
}

void 
right(int **arr, int row ,int col, int x, int y, int result[2])
{
	if (y == col - 1) {
		return;
	}
	int j;
	for(j = y + 1;j <col;j++)
	{
		int step;
		int i;
		for(step = 0; step < row; step ++)
		{
			i = x + step;
			int m = ARRAY(arr, i, j);
			if (i >= 0 && i < row) {
				if(m == 1)
				{
					result[0] = i;
					result[1] = j;
					return;
				}
			}
			i = x - step;
			m = ARRAY(arr, i, j);
			if (i >= 0 && i < row) {
				if(m == 1)
				{
					result[0] = i;
					result[1] = j;
					return;
				}
			}
		}
	}
}
void 
down(int **arr, int row ,int col, int x, int y, int result[2])
{
	if (x == row - 1) {
		return;
	}
	int i;
	for(i = x + 1;i <row;i++)
	{
		int step;
		int j;
		for(step = 0; step < col; step ++)
		{
			j = y + step;
			int m = ARRAY(arr, i, j);
			if (j >= 0 && j < col) {
				if(m == 1)
				{
					result[0] = i;
					result[1] = j;
					return;
				}
			}
			j = y - step;
			m = ARRAY(arr, i, j);
			if (j >= 0 && j < col) {
				if(m == 1)
				{
					result[0] = i;
					result[1] = j;
					return;
				}
			}
		}
	}
}
void 
up(int **arr, int row ,int col, int x, int y, int result[2])
{
	if (x == 0) {
		return;
	}
	int i;
	for(i = x - 1;i >= 0;i--)
	{
		int step;
		int j;
		for(step = 0; step < col; step ++)
		{
			j = y + step;
			int m = ARRAY(arr, i, j);
			if (j >= 0 && j < col) {
				if(m == 1)
				{
					result[0] = i;
					result[1] = j;
					return;
				}
			}
			j = y - step;
			m = ARRAY(arr, i, j);
			if (j >= 0 && j < col) {
				if(m == 1)
				{
					result[0] = i;
					result[1] = j;
					return;
				}
			}
		}
	}
}


void
tagswitchermove2(const Arg *arg)
{
	int occ = 0;
	Client *c;
	for(c = selmon->clients;c;c = c->next) occ |= c->tags;
	int selcurtagindex = switchercurtagindex;
	int row = 3;
	int col = 3;
	int arr[row][col];
	int i;
	int j;
	for(i=0;i<row;i++)
	{
		for(j=0;j<col;j++){
			arr[i][j] = (occ & (1<<(i*col+j)))?1:0;
			LOG_FORMAT("switchermove2 %d", arr[i][j]);
		}
	}
	int x = selcurtagindex/col;
	int y = selcurtagindex%col;
	int result[2];
	result[0] = x;
	result[1] = y;
	if (arg->i == 1) {
		right(arr, row, col, x, y, result);
		selcurtagindex = result[0] * col + result[1];
	}
	if (arg->i == -1) {
		left(arr, row, col, x, y, result);
		selcurtagindex = result[0] * col + result[1];
	}
	if (arg->i == 2) {
		up(arr, row, col, x, y, result);
		selcurtagindex = result[0] * col + result[1];
	}
	if (arg->i == -2) {
		down(arr, row, col, x, y, result);
		selcurtagindex = result[0] * col + result[1];
	}
	/*if(arg->i == -1) selcurtagindex -= 1;*/
	/*if(arg->i == 1) selcurtagindex += 1;*/
	/*if(arg->i == -2) selcurtagindex += 3;*/
	/*if(arg->i == 2) selcurtagindex -= 3;*/
	if(selcurtagindex < 0) selcurtagindex = 0;
	if(selcurtagindex >= LENGTH(tags)) selcurtagindex = LENGTH(tags) - 1;
	unsigned int tags = 1 << selcurtagindex;
	const Arg varg = {.ui = tags};
	view(&varg);
	const Arg layoutarg = {.v = &layouts[0]};
    	setlayout(&layoutarg);
	drawswitcherwin(selmon->switcher, selmon->ww/2, selmon->wh/2, selcurtagindex);
	XMapWindow(dpy, selmon->switcher);
	XSetInputFocus(dpy, selmon->switcher, RevertToPointerRoot, 0);
}

void
tagswitchermove(const Arg *arg)
{
	int selcurtagindex = switchercurtagindex;
	if(arg->i == -1) selcurtagindex -= 1;
	if(arg->i == 1) selcurtagindex += 1;
	if(arg->i == -2) selcurtagindex += 3;
	if(arg->i == 2) selcurtagindex -= 3;
	if(selcurtagindex < 0) selcurtagindex = 0;
	if(selcurtagindex >= LENGTH(tags)) selcurtagindex = LENGTH(tags) - 1;
	unsigned int tags = 1 << selcurtagindex;
	const Arg varg = {.ui = tags};
	view(&varg);
	const Arg layoutarg = {.v = &layouts[0]};
    	setlayout(&layoutarg);
	drawswitcherwin(selmon->switcher, selmon->ww/2, selmon->wh/2, selcurtagindex);
	XMapWindow(dpy, selmon->switcher);
	XSetInputFocus(dpy, selmon->switcher, RevertToPointerRoot, 0);
}

void 
tagswitchermove2cycle(const Arg *arg)
{
	int selcurtagindex = switchercurtagindex;
	int occ = 0;
	Client *c;
	for(c=selmon->clients;c;c=c->next) occ|=c->tags;
	do{
		selcurtagindex ++;
	}while (!(occ & (1<<selcurtagindex)) && selcurtagindex < LENGTH(tags));
	
	unsigned int maxtags =getmaxtagstiled();
	unsigned int mintags =getmintagstiled();

	if ((1<<selcurtagindex) > maxtags) {
		selcurtagindex = gettagindex(mintags);
	}

	unsigned int tags = 1 << selcurtagindex;
	const Arg varg = {.ui = tags};
	view(&varg);
	const Arg layoutarg = {.v = &layouts[0]};
    	setlayout(&layoutarg);
	drawswitcherwin(selmon->switcher, selmon->ww/2, selmon->wh/2, selcurtagindex);
	XMapWindow(dpy, selmon->switcher);
	XSetInputFocus(dpy, selmon->switcher, RevertToPointerRoot, 0);
}


void
switcherview(const Arg *arg)
{
	unsigned int tags = 1 << switchercurtagindex;
	const Arg varg = {.ui = tags};
	view(&varg);
	const Arg tsarg = {0};
	toggleswitchers(&tsarg);
}

int 
matchstr(char *pattern, char *target, regex_t **regptr)
{
	LOG_FORMAT("matchstr 1");
	int status ,i;
	int cflags = REG_EXTENDED;
	regmatch_t pmatch[1];
	
	regex_t *reg;
	if (*regptr) {
		// 本来想做缓存, 不过貌似没什么必要
		reg = *regptr;
	}else{
		regex_t regt;
		memset(&regt, 0, sizeof(regex_t));
		regcomp(&regt,pattern,cflags);//编译正则模式
		reg = &regt;
	}
	status = regexec(reg, target,1,pmatch,0);
	if (status == 0) {
		return 1;
	}
	return 0;
}

int
match(Client *c,TaskGroupItem *item)
{
	XClassHint ch = { NULL, NULL };

	XGetClassHint(dpy, c->win, &ch);
	char *class    = ch.res_class ? ch.res_class : broken;
	char *instance = ch.res_name  ? ch.res_name  : broken;
	
	if (matchstr(item->classpattern, class, &item->classregx)) {
		if (matchstr(item->titlepattern, c->name, &item->titleregx)) {
			return 1;
		}
	}

	if (ch.res_class)
		XFree(ch.res_class);
	if (ch.res_name)
		XFree(ch.res_name);

	return 0;
}

int readtaskgroup(char *path,TaskGroup *taskgroup)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        LOG_FORMAT("fopen() failed.\n");
	return 1;
    }

    char row[256];
    char *token;

    int i=0;
    while (fgets(row, 256, fp) != NULL) {
        LOG_FORMAT("Row: %s", row);
        token = strtok(row, "	"); 
	int j = 0;
        while (token != NULL) {
		if(j == 0) strcpy(taskgroup->items[i].classpattern,token);
		if(j == 1) strcpy(taskgroup->items[i].titlepattern,token);
		if(j == 2) strcpy(taskgroup->items[i].cmdbuf,token);
		if(j == 3) taskgroup->items[i].tag = 1 << atoi(token);
	    LOG_FORMAT("Token: %s", token);
	    token = strtok(NULL, "	");
	    j++;
        }
	i++;
    }
	LOG_FORMAT("Token: %p %s ", taskgroup, taskgroup->items[0].titlepattern);

    taskgroup->n = i;
    fclose(fp);
    return 0;
}

void
assemble(const Arg *arg)
{
	if (!arg->v) {
		return;
	}
	TaskGroup *taskgrouparg = arg->v;
	int n = taskgrouparg->n;

	TaskGroup taskgroupv;
	memset(&taskgroupv, 0, sizeof(TaskGroup));
	int i;
	for(i = 0;i<n;i++){
		strcpy(taskgroupv.items[i].classpattern , taskgrouparg->items[i].classpattern);
		strcpy(taskgroupv.items[i].titlepattern , taskgrouparg->items[i].titlepattern);
		taskgroupv.items[i].cmd = taskgrouparg->items[i].cmd;
		taskgroupv.items[i].tag = taskgrouparg->items[i].tag;
		memcpy(taskgroupv.items[i].cmdbuf , taskgrouparg->items[i].cmdbuf, sizeof(taskgrouparg->items[i].cmdbuf));
	}
	taskgroupv.n = n;
	

	TaskGroupItem *item;
	Client *c;
	for(c = selmon->clients; c; c = c->next)
	{
		for(i = 0;i<taskgroupv.n;i++)
		{
			item = &(taskgroupv.items[i]);
			if (match(c,item)) {
				item->c = c;
				break;
			}
		}
	}
	for(i = 0;i<taskgroupv.n;i++)
	{
		item = &(taskgroupv.items[i]);
		if (item->c) {
			item->c->tags = item->tag;
			if (item->tag) {
				Arg argview = {.i = item->tag};
				view(&argview);
			}
		}else{
			if (item->tag) {
				Arg argview = {.i = item->tag};
				view(&argview);
			}
			if (item->cmdbuf) {
				Arg argspawn = SHCMD(item->cmdbuf);
				spawn(&argspawn);
			}
			if (item->cmd) {
				Arg argspawn ={.v=item->cmd};
				spawn(&argspawn);
			}
		}
	}
	focus(NULL);
	arrange(selmon);
}

void 
assemblecsv(const Arg *arg){

	if (!arg->v) {
		return;
	}

	TaskGroup taskgroupv;
	memset(&taskgroupv, 0, sizeof(TaskGroup));
	readtaskgroup(*(char ***)arg->v, &taskgroupv);
	/*readtaskgroup("/home/beyond/software/bin/dwm-taskgroup/1.csv", &taskgroupv);*/
	LOG_FORMAT("taskgroup from csv %s %s", taskgroupv.items[0].classpattern, taskgroupv.items[0].titlepattern);
	Arg a = {.v=&taskgroupv};
	assemble(&a);
}

void
dismiss(const Arg *arg)
{
	
}
 
int
main(int argc, char *argv[])
{
	close(2);
	logfile = fopen("/home/beyond/m.log", "a");
	actionlogfile = fopen("/home/beyond/action.log", "a");
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
