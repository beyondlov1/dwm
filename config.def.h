/* Include */
#include <X11/X.h>
#include <X11/XF86keysym.h>
/* See LICENSE file for copyright and license details. */

/* appearance */
#define ICONSIZE 16
#define ICONSPACING 5
static const unsigned int isscratchmask  = 0;        /* hide other clients when scratch shown */
static const unsigned int isswitcherpreview  = 1;        /* switch preview */
static const unsigned int borderpx  = 2;        /* border pixel of windows */
static const Gap default_gap        = {.isgap = 1, .realgap = 10, .gappx = 15};
static const unsigned int snap      = 32;       /* snap pixel */
static const unsigned int systraypinning = 0;   /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
static const unsigned int systrayonleft = 0;    /* 0: systray in the right corner, >0: systray on left of status text */
static const unsigned int systrayspacing = 2;   /* systray spacing */
static const int systraypinningfailfirst = 1;   /* 1: if pinning fails, display systray on the first monitor, False: display systray on the last monitor*/
static const int showsystray        = 1;        /* 0 means no systray */
static const int showbar            = 0;        /* 0 means no bar */
static const int showborderwin      = 0;
static const int showcornerwin      = 1;
static const int topbar             = 0;        /* 0 means bottom bar */
static const Bool viewontag         = True;     /* Switch view on tag switch */
static const Bool managestubon         = False;     /* if total stub > screen stub(default 3); move new window to new tag */
static const char *fonts[]          = { 
    "JetBrainsMono Nerd Font Mono:size=16",
    "JoyPixels:size=14:antialias=true:autohint=true" };
static const char dmenufont[]       = "JetBrainsMono Nerd Font Mono:size=16";
static const char col_gray1[]       = "#222222";
static const char col_gray2[]       = "#444444";
static const char col_gray3[]       = "#bbbbbb";
static const char col_gray4[]       = "#eeeeee";
static const char col_cyan[]        = "#43A5F5";
static const char col_white[]        = "#FFFFFF";
static const char *colors[][3] = {
	/*               fg         bg         border   */
	[SchemeNorm] = {col_gray3, col_gray1, col_gray2},
	[SchemeSel] = {col_gray4, col_cyan, "#ffcd05"},
	[SchemeScr] = {col_gray4, col_cyan, "#cf110b"},
	[SchemeInvalidNormal] = {col_gray2, col_gray1, col_gray2},
	[SchemeInvalidSel] = {col_gray2, col_cyan, "#00BFFF"},
	[SchemeDoublePageMarked] = {col_gray2, col_cyan, "#ff9300"},
	[SchemeSwitchPrepareMove] = {"#ff9300", "#ff9300", "#ff9300"},
	[SchemeTiled] = {"#ff9300", col_gray1, col_gray2},
	[SchemeFulled] = {"#33FF89", col_gray1, col_gray2},
	[SchemeWarn] = {"#ff9300", "#000000", col_gray2},
};

static const int gradual_colors_count = 6;
static const char *gradual_colors[][3] = {
	 {col_white, "#222222", col_gray2},
	 {col_white, "#252c32", col_gray2}, 
	 {col_white, "#273642", col_gray2}, 
	 {col_white, "#2a4053", col_gray2}, 
	 {col_white, "#2c4a63", col_gray2}, 
	 {col_white, "#2f5473", col_gray2}
};

// 显示不出来
// static const XRenderColor gradual_colors[][3] = {
// 	 {{(unsigned short)187,(unsigned short)187,(unsigned short)187,(unsigned short)255}, {(unsigned short)67,(unsigned short)165,(unsigned short)245,(unsigned short)30}, {(unsigned short)68,(unsigned short)68,(unsigned short)68,(unsigned short)255}},
// 	 {{(unsigned short)187,(unsigned short)187,(unsigned short)187,(unsigned short)255}, {(unsigned short)67,(unsigned short)165,(unsigned short)245,(unsigned short)70}, {(unsigned short)68,(unsigned short)68,(unsigned short)68,(unsigned short)255}},
// 	 {{(unsigned short)187,(unsigned short)187,(unsigned short)187,(unsigned short)255}, {(unsigned short)67,(unsigned short)165,(unsigned short)245,(unsigned short)120}, {(unsigned short)68,(unsigned short)68,(unsigned short)68,(unsigned short)255}},
// 	 {{(unsigned short)187,(unsigned short)187,(unsigned short)187,(unsigned short)255}, {(unsigned short)67,(unsigned short)165,(unsigned short)245,(unsigned short)170}, {(unsigned short)68,(unsigned short)68,(unsigned short)68,(unsigned short)255}},
// 	 {{(unsigned short)187,(unsigned short)187,(unsigned short)187,(unsigned short)255}, {(unsigned short)67,(unsigned short)165,(unsigned short)245,(unsigned short)220}, {(unsigned short)68,(unsigned short)68,(unsigned short)68,(unsigned short)255}},
// 	 {{(unsigned short)187,(unsigned short)187,(unsigned short)187,(unsigned short)255}, {(unsigned short)67,(unsigned short)165,(unsigned short)245,(unsigned short)255}, {(unsigned short)68,(unsigned short)68,(unsigned short)68,(unsigned short)255}},
// };


// st打开时, 切换 nmaster = 1, 保持宽屏
static const char *widescreen_classes[] = {"Code", "Google-chrome", "firefox", "Chromium", "chromium"};

/* tagging */
static const char *tags[] = { "  1-term  ", "2-browser ", "  3-dev1  ", "  4-dev2  ", "5-datasource", " 6-learn  ", "    7     ", "    8     ", "    9     " };

/* launcher commands (They must be NULL terminated) */
static const char* chrome[]      = { "google-chrome-stable", NULL, NULL };
static const char *terminal[] = {"/home/beyond/software/terminator", NULL };

static const Launcher launchers[] = {
       /* command       name to display */
	{ chrome,         "Google" },
	{ terminal,         "Terminal" },
};

/* Lockfile */
static char lockfile[] = "/tmp/dwm.lock";

static const Rule defaultrule = { "",              NULL,       NULL,       0,       0,           -1 , 0,              0};
static const Rule rules[] = {
	/* xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 */
	/* class                 instance    title       tags mask     isfloating   monitor  priority   nstub isscratch*/
	{"firefox", NULL, NULL, 0, 0, -1, 5, 2, 0},
	{"Google-chrome", NULL, NULL, 0, 0, -1, 5, 2,0},
	{"chromium", NULL, NULL, 0, 0, -1, 5, 2,0},
	{"Sidekick-browser", NULL, NULL, 0, 0, -1, 5, 2,0},
	{"Brave-browser-nightly", NULL, NULL, 0, 0, -1, 5, 2,0},
	{"X-terminal-emulator", NULL, NULL, 0, 0, -1, 1, -1,0},
	{"Terminator", NULL, NULL, 0, 0, -1, 1, -1,0},
	{"St", NULL, NULL, 0, 0, -1, 1, -1,0},
	{"Code", NULL, NULL, 0, 0, -1, 5, 0,0},
	{"jetbrains-idea", NULL, NULL, 0, 0, -1, 5, 0,0},
	{"jetbrains-datagrip", NULL, NULL, 0, 0, -1, 5, 2,0},
	{"Evince", NULL, NULL, 0, 0, -1, 5, 2,0},
	{"flameshot", NULL, NULL, 0xFF, 1, -1, 5, 0,0},
	{"Fragcode", NULL, NULL, 0, 1, -1, 5, 0,0},
	{"Thunar", NULL, NULL, 0, 1, -1, 5, 0,0},
	{"ToDesk", NULL, NULL, 0, 0, -1, 5, 2,0},
	{"qclip", NULL, NULL, 0, 1, -1, 1, 2,1},
	{"obsidian", NULL, NULL, 0, 1, -1, 1, 2,1},
	// {"Rofi", NULL, NULL, 0, 1, -1, 1, 2,0},
};

static const Rule subjrules[] = {
	/* xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 */
	/* class                  instance    title       tags mask     isfloating   monitor  priority  nstub isscratch*/
	{ "firefox",              NULL,       NULL,       1 << 1,       0,           -1 , 5,              2, 0},
	{ "Google-chrome",        NULL,       NULL,       1 << 1,       0,           -1 , 5,              2,0},
	{ "chromium",        NULL,       NULL,       1 << 1,       0,           -1 , 5,              2,0},
	{ "Sidekick-browser",     NULL,       NULL,       1 << 1,       0,           -1 , 5,              2,0},
	{ "Brave-browser-nightly",     NULL,       NULL,       1 << 1,       0,           -1 , 5,              2,0},
	{ "X-terminal-emulator",  NULL,       NULL,       1 << 0,       0,           -1 , 1,              -1,0},
	{ "Terminator",  NULL,       NULL,       1 << 0,       0,           -1 , 1,              -1,0},
	{ "St",  NULL,       NULL,       1 << 0,       0,           -1 , 1,              -1,0},
	{ "Code",                 NULL,       NULL,       1 << 3,       0,           -1 , 5,              0,0},
	{ "jetbrains-idea",       NULL,       NULL,       1 << 2,       0,           -1 , 5,              0,0},
	{ "jetbrains-datagrip",   NULL,       NULL,       1 << 4,       0,           -1 , 5,              2,0},
	{ "Evince",               NULL,       NULL,       1 << 5,       0,           -1 , 5,              2,0},
	{ "flameshot",            NULL,       NULL,       0xFF,       1,           -1 , 5,              0,0},
	{ "qclip",  NULL,       NULL,       1 << 0,       0,           -1 , 1,              2,1},
	{ "obsidian",  NULL,       NULL,       1 << 0,       0,           -1 , 1,              2,1},
};

/* layout(s) */
static const float mfact     = 0.60; /* factor of master area size [0.05..0.95] */
static const float factx     = 0.05; /* factor of master area size [0.05..0.95] */
static const float facty     = 0.05; /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;    /* number of clients in master area (没什么用, 但是不能删, 有旧功能占用着) */
static const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen = 1; /* 1 will force focus on the fullscreen window */
static const int expandslavewhenfocus = 0; // focus slave窗口时, 是否扩展当前窗口

/* Include */
#include "sort.c"
#include "gaplessgrid.c"
#include "gapgrid.c"
#include "gapgridsorted.c"
#include "gapgridsortedneat.c"
static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "[]=",      tile7 },
	//{ "[]=",      tile5 },
	{ "[M]",      monocle },
	{ "><>",      NULL },    /* no layout function means floating behavior */
	{ "HHH",      gapgridsortedneat },
	{ "[|]",      doublepage },
	{ "HHH",      gapgridsorted },
	{ "HHH",      gapgrid },
	{ "HHH",      gaplessgrid },
	{ "[]=",      tile3 },    /* first entry is default */
	{ "[]=",      tile5 }, 
};

/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

#define STATUSBAR "dwmblocks"

/* commands */
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *dmenucmd[] = { "dmenu_run", "-m", dmenumon, "-fn", dmenufont, "-nb", col_gray1, "-nf", col_gray3, "-sb", col_cyan, "-sf", col_gray4, "-l", "10", NULL };
static const char *rofiruncmd[] = { "/home/beyond/software/rofi.sh",  NULL};
//static const char *termcmd[]  = { "st", NULL };
static const char scratchpadname[] = "scratchpad";
// static const char *scratchpadcmd[] = { "st", "-t", scratchpadname, "-g", "120x34", NULL };
static const char *scratchpadcmd[] = {"jumpapp", "chrome", NULL };
static const char *firefoxcmd[] = {"/usr/bin/firefox",NULL};
// static const char *flameshotcmd[] = {"flameshot","gui", NULL};
static const char *flameshotcmd[] = {"/home/beyond/software/flameshottocloud.sh", NULL};
static const char *browser[] = {"sidk.sh",NULL};
static const char *notecmd[] = {"qt-note.sh",NULL};
static const char *querycmd[] = {"/home/beyond/software/queryclip.sh", "http://cn.bing.com/search?q=%s", NULL};
static const char *querybrowsercmd[] = {"/home/beyond/software/browserclip.sh", "https://cn.bing.com/search?q=%s", NULL};
//static const char *dictcmd[] = {"/home/beyond/software/queryclip.sh", "http://youdao.com/result?word=%s&lang=en", NULL};
static const char *dictcmd[] = {"/home/beyond/software/browserclip.sh", "http://youdao.com/result?word=%s&lang=en", NULL};
//static const char *todotxtcmd[] = {"st","-e","/home/beyond/software/todotxtcmd.sh",NULL};
static const char *todotxtcmd[] = {"st","-e","python3", "/home/beyond/software/bin/textual-demo/app.py",NULL};
static const char *enotecmd[] = {"st","-e","python3", "/home/beyond/software/bin/enotepy/app.py",NULL};
// static const char *copyqcmd[] = {"copyq", "show", NULL };
// static const char *copyqcmd[] = {"/usr/bin/clipcat-menu", NULL };
static const char *copyqcmd[] = {"/home/beyond/software/rofi.sh", "/clipboard",  NULL };
static const char *contextcmd[] = {"/home/beyond/software/rofi.sh", "/context",  NULL };
//static const char *browsercmd[] = {"firefox", NULL};
static const char *browsercmd[] = {"/home/beyond/software/ba", NULL};
//static const char *browsercmd[] = {"/home/beyond/software/bin/firefox/firefox/firefox", NULL};
static const char *fmcmd[] = {"/home/beyond/software/fm", NULL};
static const char *fmclipcmd[] = {"/home/beyond/software/fmclip", NULL};

static const char *notecurrwincmd[] = {"/home/beyond/software/notecurrwin.sh", NULL};
static const char *ocrcmd[] = {"/home/beyond/software/ocr.sh", NULL};
static const char *smartleftcmd[] = {"/home/beyond/software/smartleft.sh", NULL};
static const char *smartrightcmd[] = {"/home/beyond/software/smartright.sh", NULL};
static const char *smartclosecmd[] = {"/home/beyond/software/smartclose.sh", NULL};
static const char *smartrefreshcmd[] = {"/home/beyond/software/smartrefresh.sh", NULL};
static const char *highlightcmd[] = {"/home/beyond/software/smartdispatch.sh", "highlight", NULL};
static const char *searchwordcmd[] = {"/home/beyond/software/smartdispatch.sh", "searchword", NULL};
static const char *vimmaskcmd[] = {"/home/beyond/software/vimmask.sh",  NULL};

//static const char *smartleftcmd[] = {"/home/beyond/software/smartdispatch.sh", "smartleft", NULL};
//static const char *smartrightcmd[] = {"/home/beyond/software/smartdispatch.sh", "smartright", NULL};
//static const char *smartupcmd[] = {"/home/beyond/software/smartdispatch.sh", "smartup", NULL};
//static const char *smartdowncmd[] = {"/home/beyond/software/smartdispatch.sh", "smartdown", NULL};


static const TaskGroup taskgroup1 = {
	2,
	{
		{
			"^St$","^dwm$",(char **)((char *[]){"st",NULL}),1<<5,0,NULL,NULL,NULL,NULL
		},
		{
			"^St$","^.*rofi.*$",(char **)((char *[]){"st",NULL}),1<<5,0,NULL,NULL,NULL,NULL
		},
	}
};

static const char *taskgrouppath = "/home/beyond/software/bin/dwm-taskgroup/1.csv";

static Key keys[] = {
	/* modifier                     key        function        argument */
	{MODKEY, XK_grave, i_maxwindow, {0} },
	{ Mod1Mask|ShiftMask,                       XK_Return,      stsubspawn,      { 0 } },
	// { Mod1Mask,                       XK_a,      spawn,       {.v=dmenucmd} },
	{ Mod1Mask,                       XK_a,      spawn,       {.v=rofiruncmd} },
	//{ MODKEY,                       XK_Return, sspawn,          {.v = terminal } },
	//{ MODKEY,                       XK_Return, stsspawn,          { 0 } },

	//{ Mod1Mask,                       XK_m,      tile7makemaster,      { 0 } },

	//{ 0,                       XK_Caps_Lock,      toggleswitchers,      {0} },
	{ 0,                       XF86XK_Tools,      toggleswitchers,      {0} },
	

	//{ 0,                       XK_F1,      spawn,      { .v = smartclosecmd } },
	//{ 0,                       XK_F2,      spawn,      { .v = smartrefreshcmd } },
	//{ 0,                       XK_F3,      spawn,      { .v = searchwordcmd } },
	//{ 0,                       XK_F4,      spawn,      { .v = highlightcmd } },
	// { Mod1Mask,                       XK_z,      spawn,      { .v = ocrcmd } },
	{ Mod1Mask,                       XK_s,      spawn,      { .v = flameshotcmd } },
	{ Mod1Mask,                       XK_F4,      killclient,       {0} },
	{ Mod1Mask,                       XK_n,      spawn,      { .v = notecurrwincmd } },
	{ Mod1Mask,                       XK_semicolon,      spawn,      { .v = vimmaskcmd } },

	// for tile5
	// { MODKEY|ShiftMask,                       XK_j,      tile5move,       {.i = -2} },
	// { MODKEY|ShiftMask,                       XK_k,      tile5move,       {.i = +2} },
	// { MODKEY|ShiftMask,                       XK_l,      tile5move,       {.i = +1} },
	// { MODKEY|ShiftMask,                       XK_h,      tile5move,       {.i = -1} },
	// { MODKEY|ShiftMask,                       XK_Down,      tile5move,       {.i = -2} },
	// { MODKEY|ShiftMask,                       XK_Up,      tile5move,       {.i = +2} },
	// { MODKEY|ShiftMask,                       XK_Right,      tile5move,       {.i = +1} },
	// { MODKEY|ShiftMask,                       XK_Left,      tile5move,       {.i = -1} },
	{ MODKEY|ControlMask,                       XK_l,      tile5expandx,       {.f = 0.1} },
	{ MODKEY|ControlMask,                       XK_h,      tile5expandx,       {.f = -0.1} },
	{ MODKEY|ControlMask,                       XK_Right,      tile5expandx,       {.f = 0.1} },
	{ MODKEY|ControlMask,                       XK_Left,      tile5expandx,       {.f = -0.1} },
	{ MODKEY|ControlMask,                       XK_j,      tile5expandy,       {.f = -0.1} },
	{ MODKEY|ControlMask,                       XK_k,      tile5expandy,       {.f = 0.1} },
	{ MODKEY|ControlMask,                       XK_Down,      tile5expandy,       {.f = -0.1} },
	{ MODKEY|ControlMask,                       XK_Up,      tile5expandy,       {.f = 0.1} },
	{ Mod1Mask,                       XK_Tab,      scratchmove,       {.i = -2} },
	{ Mod1Mask,                       XK_Page_Up,      scratchmove,       {.i = +2} },
	{ Mod1Mask,                       XK_Page_Down,      scratchmove,       {.i = -2} },

	{ Mod1Mask,                       XK_equal,      tile5expand,       {.f=0.1} },
	{ Mod1Mask,                       XK_minus,      tile5expand,       {.f=-0.1} },
	{ MODKEY,                       XK_equal,      tile5expand,       {.f=0.1} },
	{ MODKEY,                       XK_minus,      tile5expand,       {.f=-0.1} },
	// { Mod1Mask,                       XK_grave,      tile5viewcomplete,       {0} },
	
	{MODKEY,			XK_F2, killclientforce, {0}},
	{ControlMask|Mod1Mask,			XK_g, tile6maximizewithsticky, {0}},
	// { MODKEY,                       XK_q, ispawn,          {.v = browsercmd} },
	{ MODKEY,                       XK_e, rispawn,          {.v = fmcmd} },
	{ MODKEY,                       XK_equal, tile6zoom,          { .f = 0.1 } },
	{ MODKEY,                       XK_minus, tile6zoom,          { .f = -0.1} },
	// { Mod1Mask,                       XK_Tab, focuslast,          { 0 } },
	{ MODKEY,                       XK_Return, stispawn,          { 0 } },
	{ MODKEY,                       XK_KP_Enter, stispawn,          { 0 } },
	{ MODKEY|ShiftMask,                       XK_Return, stspawn,          { 0 } },
	{ MODKEY|ShiftMask,                       XK_KP_Enter, stspawn,          { 0 } },
	{ MODKEY,                       XK_b, assemblecsv,          { .v = &taskgrouppath } },
	{ MODKEY,                       XK_q, tsspawn,          {.v = querybrowsercmd } },

	{ ControlMask,                  XK_space, tsspawn,          {.v = copyqcmd } },
	{ Mod1Mask,                  XK_z, spawn,          {.v = contextcmd } },

	{ MODKEY,                       XK_a, tsspawn,          {.v = dictcmd } },
	{ MODKEY,                       XK_z, tsspawn,          {.v = todotxtcmd} },
	{ MODKEY,                       XK_x, tsspawn,          {.v = enotecmd} },
	// { MODKEY,                       XK_i, sspawn,          {.v = notecmd } },
	{ MODKEY,                       XK_s,      spawn,          {.v = flameshotcmd } },
	// { MODKEY,                       XK_grave,  togglescratch,  {.v = scratchpadcmd } },
	{ Mod1Mask,                       XK_p,      addtoscratchgroup,  {0 } },
	{ Mod1Mask,                       XK_y,      removefromscratchgroup,  {0 } },
	{ Mod1Mask,                       XK_m,      removefromscratchgroup,  {0 } },
	//{ MODKEY,                       XK_n,      togglescratchgroup,  {0} },
	//{ Mod1Mask,                       XK_n,      togglescratchgroup,  {0} },
	 { Mod1Mask,                       XK_grave,      togglescratchgroup,  {0} },
	// { MODKEY,                       XK_b,      togglebar,      {0} },
	{ MODKEY,                       XK_r,      rerule,      {0} },
	// { MODKEY|ShiftMask,             XK_j,      rotatestack,    {.i = +1 } },
	// { MODKEY|ShiftMask,             XK_k,      rotatestack,    {.i = -1 } },
	//{ MODKEY|ShiftMask,                       XK_j,      movey,     {.i = 100 } },
	//{ MODKEY|ShiftMask,                       XK_k,      movey,     {.i = -100 } },
	//{ MODKEY|ShiftMask,                       XK_l,      movex,     {.i = 200 } },
	//{ MODKEY|ShiftMask,                       XK_h,      movex,     {.i = -200 } },
	
	//{ MODKEY,                       XK_j,      focusgrid5,     {.i = -2 } },
	//{ MODKEY,                       XK_k,      focusgrid5,     {.i = +2 } },
	//{ MODKEY,                       XK_l,      focusgrid5,     {.i = +1 } },
	//{ MODKEY,                       XK_h,      focusgrid5,     {.i = -1 } },
	//{ MODKEY,                       XK_Down,      focusgrid5,     {.i = -2 } },
	//{ MODKEY,                       XK_Up,      focusgrid5,     {.i = +2 } },
	//{ MODKEY,                       XK_Right,      focusgrid5,     {.i = +1 } },
	//{ MODKEY,                       XK_Left,      focusgrid5,     {.i = -1 } },

	{MODKEY, XK_Left, switchermove, {.i = -1}},
	{MODKEY, XK_Right, switchermove, {.i = 1}},
	{MODKEY, XK_Up, switchermove, {.i = 2}},
	{MODKEY, XK_Down, switchermove, {.i = -2}},
	{MODKEY, XK_h, switchermove, {.i = -1}},
	{MODKEY, XK_l, switchermove, {.i = 1}},
	{MODKEY, XK_k, switchermove, {.i = 2}},
	{MODKEY, XK_j, switchermove, {.i = -2}},

	// {Mod1Mask|ShiftMask, XK_Left, switchermove, {.i = -1}},
	// {Mod1Mask|ShiftMask, XK_Right, switchermove, {.i = 1}},
	// {Mod1Mask|ShiftMask, XK_Up, switchermove, {.i = 2}},
	// {Mod1Mask|ShiftMask, XK_Down, switchermove, {.i = -2}},
	// {Mod1Mask|ShiftMask, XK_h, switchermove, {.i = -1}},
	// {Mod1Mask|ShiftMask, XK_l, switchermove, {.i = 1}},
	// {Mod1Mask|ShiftMask, XK_k, switchermove, {.i = 2}},
	// {Mod1Mask|ShiftMask, XK_j, switchermove, {.i = -2}},

	{Mod1Mask|ShiftMask, XK_Up, container_layout_tile_v_movesplit, {.i = 2}},
	{Mod1Mask|ShiftMask, XK_Down, container_layout_tile_v_movesplit, {.i = -2}},
	{Mod1Mask|ShiftMask, XK_k, container_layout_tile_v_movesplit, {.i = 2}},
	{Mod1Mask|ShiftMask, XK_j, container_layout_tile_v_movesplit, {.i = -2}},

	{Mod1Mask|ShiftMask, XK_u, container_layout_tile_v_movesplit_toggle, {.i = 2}},

	{Mod1Mask|ShiftMask, XK_Left, container_layout_tile_v_movesplit, {.i = -1}},
	{Mod1Mask|ShiftMask, XK_Right, container_layout_tile_v_movesplit, {.i = 1}},
	{Mod1Mask|ShiftMask, XK_h, container_layout_tile_v_movesplit, {.i = -1}},
	{Mod1Mask|ShiftMask, XK_l, container_layout_tile_v_movesplit, {.i = 1}},

	{ControlMask|Mod1Mask, XK_Left, tile7switchermovecontainer, {.i = -1}},
	{ControlMask|Mod1Mask, XK_Right, tile7switchermovecontainer, {.i = 1}},
	{ControlMask|Mod1Mask, XK_Up, tile7switchermovecontainer, {.i = 2}},
	{ControlMask|Mod1Mask, XK_Down, tile7switchermovecontainer, {.i = -2}},

	// { MODKEY,                       XK_i,      incnmaster,     {.i = +1 } },
	// { MODKEY,                       XK_d,      incnmaster,     {.i = -1 } },
	//{ MODKEY|ControlMask,                       XK_h,      setmfact,       {.f = -0.05} },
	//{ MODKEY|ControlMask,                       XK_l,      setmfact,       {.f = +0.05} },
	//{ MODKEY|ShiftMask,                       XK_h,      setmfact,       {.f = -0.05} },
	//{ MODKEY|ShiftMask,                       XK_l,      setmfact,       {.f = +0.05} },
	
	//{ MODKEY|ShiftMask,                       XK_h,      resizex,       {.f = -0.05} },
	//{ MODKEY|ShiftMask,                       XK_l,      resizex,       {.f = 0.05} },
	//{ MODKEY|ShiftMask,                       XK_j,      resizey,       {.f = +0.05} },
	//{ MODKEY|ShiftMask,                       XK_k,      resizey,       {.f = -0.05} },
	
	// for tile7
	// { MODKEY|ShiftMask,                       XK_j,      swap,       {.i = -2} },
	// { MODKEY|ShiftMask,                       XK_k,      swap,       {.i = +2} },
	// { MODKEY|ShiftMask,                       XK_l,      swap,       {.i = +1} },
	// { MODKEY|ShiftMask,                       XK_h,      swap,       {.i = -1} },
	// { MODKEY|ShiftMask,                       XK_Down,      swap,       {.i = -2} },
	// { MODKEY|ShiftMask,                       XK_Up,      swap,       {.i = +2} },
	// { MODKEY|ShiftMask,                       XK_Right,      swap,       {.i = +1} },
	// { MODKEY|ShiftMask,                       XK_Left,      swap,       {.i = -1} },

    { MODKEY|ControlMask,                       XK_l,      i_expandx,       {.f = 0.2} },
	{ MODKEY|ControlMask,                       XK_h,      i_expandx,       {.f = -0.2} },
	{ MODKEY|ControlMask,                       XK_Right,      i_expandx,       {.f = 0.2} },
	{ MODKEY|ControlMask,                       XK_Left,      i_expandx,       {.f = -0.2} },
	 { MODKEY|ControlMask,                       XK_j,      i_expandy,       {.f = 0.2} },
	{ MODKEY|ControlMask,                       XK_k,      i_expandy,       {.f = -0.2} },
	 { MODKEY|ControlMask,                       XK_Down,      i_expandy,       {.f = 0.2} },
	{ MODKEY|ControlMask,                       XK_Up,      i_expandy,       {.f = -0.2} },

	// { MODKEY|ControlMask,                       XK_j,      tiledual,       {.i = -2} },
	// { MODKEY|ControlMask,                       XK_k,      tiledual,       {.i = +2} },
	// { MODKEY|ControlMask,                       XK_l,      tiledual,       {.i = +1} },
	// { MODKEY|ControlMask,                       XK_h,      tiledual,       {.i = -1} },
	// { MODKEY|ControlMask,                       XK_Down,      tiledual,       {.i = -2} },
	// { MODKEY|ControlMask,                       XK_Up,      tiledual,       {.i = +2} },
	// { MODKEY|ControlMask,                       XK_Right,      tiledual,       {.i = +1} },
	// { MODKEY|ControlMask,                       XK_Left,      tiledual,       {.i = -1} },

	{ MODKEY|ShiftMask,                       XK_n,      setfacty,       {0} },
	// { MODKEY,             XK_Tab, smartzoom,           {0} },
	// { MODKEY,             XK_Tab,   toggleswitchers,           {0} },
	//{ MODKEY,                       XK_Tab,    view,           {.ui = ~0 } },
	{ MODKEY,                       XK_Tab,    focuslast,           { 0 } },
	// { MODKEY,                       XK_Tab,    tile7shiftnext,           { 0 } },
	// { MODKEY,                       XK_Tab,    toggleswitchers,           {0} },
	{ MODKEY,             XK_KP_Page_Up, zoomi,           {.i=1} },
	{ MODKEY,             XK_KP_Right, zoomi,           {.i=2} },
	{ MODKEY,             XK_KP_Page_Down, zoomi,           {.i=3} },
	{ MODKEY,             XK_KP_Delete, zoomi,           {.i=1000} },
	{ MODKEY,             XK_comma, zoomi,           {.i=1} },
	{ MODKEY,             XK_period, zoomi,           {.i=1000} },
	{ MODKEY|ShiftMask,             XK_Return, zoom,           {0} },
	// { Mod1Mask,                       XK_Tab,    smartview,           {0} },
	{ MODKEY,                       XK_u,    relview,           {.i=-1} },
	{ MODKEY,                       XK_o,    relview,           {.i=1} },
	{ MODKEY|ShiftMask,                       XK_u,    reltag,           {.i=-1} },
	{ MODKEY|ShiftMask,                       XK_o,    reltag,           {.i=1} },
	{ MODKEY|ShiftMask,                       XK_minus,    reltagd,           {.i=-1} },
	{ MODKEY|ShiftMask,                       XK_equal,    reltagd,           {.i=1} },
	// { MODKEY,                       XK_a,    relview,           {.i=-1} },
	// { MODKEY,                       XK_d,    relview,           {.i=1} },
	// { MODKEY,                       XK_q,      relview,          {.i = -1 } },
	// { MODKEY,                       XK_w,      relview,          {.i = 1 } },
	//{ 0,                       XK_Super_L,      empty,      {0} },
	{ 0,                       XK_Super_L,      toggleswitchers,      {0} },
	{ 0,                       XK_Super_R,      toggleswitchers,      {0} },
	// { MODKEY,                       XK_grave,      tile6maximizewithsticky,      {0} },
	{ MODKEY,                       XK_F1,      killclient,     {0} },
	{ MODKEY,                       XK_t,      setlayout,      {.v = &layouts[0]} },
	//{ MODKEY,                       XK_f,      setlayout,      {.v = &layouts[1]} },
	// { MODKEY,                       XK_d,      setlayoutt,      {.v = &layouts[4]} },
	{ MODKEY,                       XK_d,      doublepagemark,      {.v = &layouts[4]} },
	// { MODKEY,                       XK_m,      setlayout,      {.v = &layouts[2]} },
	//{ MODKEY,                       XK_m,      setlayout,      {0} },
	{ MODKEY,                       XK_g,      setcontainerlayout,      {0} },
	{ MODKEY,                       XK_v,      setlayout,      {.v=&layouts[8]} },
	// { MODKEY,                       XK_g,      toggleswitchers,      {0} },
	//{ MODKEY,                       XK_g,      toggleswitchers,      {0} },
	// { MODKEY,                       XK_b,      destroyswitchers,      {0} },
	{ MODKEY,                       XK_space,  tile5toggleshift,      {0} },
	{ MODKEY|ShiftMask,             XK_space,  tile5togglefloating, {0} },
	{ Mod1Mask|ShiftMask,             XK_space,  tile5togglefloating, {0} },
	{ MODKEY,                       XK_0,      view,           {.ui = ~0 } },
	{ MODKEY|ShiftMask,             XK_0,      tag,            {.ui = ~0 } },
	// { MODKEY,                       XK_comma,  focusmon,       {.i = -1 } },
	// { MODKEY,                       XK_period, focusmon,       {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_comma,  tagmon,         {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_period, tagmon,         {.i = +1 } },
	{ MODKEY,                       XK_minus,  setgaps,        {.i = -5 } },
	{ MODKEY,                       XK_equal,  setgaps,        {.i = +5 } },
	{ MODKEY|ShiftMask,             XK_minus,  setgaps,        {.i = GAP_RESET } },
	{ MODKEY|ShiftMask,             XK_equal,  setgaps,        {.i = GAP_TOGGLE} },
	TAGKEYS(                        XK_1,                      0)
	TAGKEYS(                        XK_2,                      1)
	TAGKEYS(                        XK_3,                      2)
	TAGKEYS(                        XK_4,                      3)
	TAGKEYS(                        XK_5,                      4)
	TAGKEYS(                        XK_6,                      5)
	TAGKEYS(                        XK_7,                      6)
	TAGKEYS(                        XK_8,                      7)
	TAGKEYS(                        XK_9,                      8)
	// TAGKEYS(                        XK_KP_End,                      0)
	// TAGKEYS(                        XK_KP_Down,                      1)
	// TAGKEYS(                        XK_KP_Page_Down,                      2)
	// TAGKEYS(                        XK_KP_Left,                      3)
	// TAGKEYS(                        XK_KP_Begin,                      4)
	// TAGKEYS(                        XK_KP_Right,                      5)
	// TAGKEYS(                        XK_KP_Home,                      6)
	// TAGKEYS(                        XK_KP_Up,                      7)
	// TAGKEYS(                        XK_KP_Page_Up,                      8)
	{ MODKEY|ShiftMask,             XK_q,      quit,           {0} },
	{ MODKEY|ControlMask|ShiftMask, XK_q,      quit,           {1} }, 
	// { 0, XF86XK_AudioMute,        spawn, SHCMD("pamixer -t; pkill -RTMIN+7 dwmblocks") },
	// { 0, XF86XK_AudioRaiseVolume, spawn, SHCMD("pamixer --allow-boost -i 5; pkill -RTMIN+7 dwmblocks") },
	// { 0, XF86XK_AudioLowerVolume, spawn, SHCMD("pamixer --allow-boost -d 5; pkill -RTMIN+7 dwmblocks") },
	// { 0, XF86XK_AudioPause,       spawn, SHCMD("mpc pause; pkill -RTMIN+1 dwmblocks") },
	// { 0, XF86XK_Search,        spawn, SHCMD("mpc clear; mpc update; mpc listall | mpc add; mpc shuffle; mpc random on; mpc play; pkill -RTMIN+1 dwmblocks") },
	{ MODKEY, XK_F9,        spawn, SHCMD("mpc clear; mpc update; mpc listall | mpc add; mpc shuffle; mpc random on; mpc play; pkill -RTMIN+1 dwmblocks") },
	// { 0, XF86XK_AudioPrev,        spawn, SHCMD("mpc prev; pkill -RTMIN+1 dwmblocks") },
	{ MODKEY, XK_F10,        spawn, SHCMD("mpc prev; pkill -RTMIN+1 dwmblocks") },
	// { 0, XF86XK_AudioNext,        spawn, SHCMD("mpc next; pkill -RTMIN+1 dwmblocks") },
	{ MODKEY, XK_F12,        spawn, SHCMD("echo \"$(mpc -f %file% | sed -n \"1,1p\") $(mpc | sed -n \"2,2p\" | awk '{print $4}' | grep -Eo \"[0-9]*\") \" >> $HOME/.config/netease/play_skip.log.csv;mpc next; ") },
	// { 0, XF86XK_AudioPlay,        spawn, SHCMD("mpc toggle; pkill -RTMIN+1 dwmblocks") },
	{ MODKEY, XK_F11,        spawn, SHCMD("mpc toggle; pkill -RTMIN+1 dwmblocks") },
	{ MODKEY, XK_F8,        spawn, SHCMD("mpc sticker $(mpc current) set like yes") },
	{ 0, XF86XK_AudioStop,        spawn, SHCMD("mpc stop; pkill -RTMIN+1 dwmblocks") },
	{ 0, XF86XK_AudioRaiseVolume, spawn, SHCMD("pactl set-sink-volume @DEFAULT_SINK@ +10%") },
	{ 0, XF86XK_AudioLowerVolume, spawn, SHCMD("pactl set-sink-volume @DEFAULT_SINK@ -10% ") },
	{ 0, XF86XK_AudioMute,       spawn, SHCMD("pactl set-sink-mute @DEFAULT_SINK@ toggle") },
	{ 0, XF86XK_AudioMicMute,        spawn, SHCMD("pactl set-source-mute @DEFAULT_SOURCE@ toggle") },
};


static Key switcherkeys[] = {

	//{ 0,                       XK_Caps_Lock,      toggleswitchers,      {0} },
	{0, XK_o, quickfocus, {.ui = 'o'}},
	{0, XK_b, quickfocus, {.ui = 'b'}},
	{0, XK_y, quickfocus, {.ui = 'y'}},
	{0, XK_i, quickfocus, {.ui = 'i'}},
	{0, XK_p, quickfocus, {.ui = 'p'}},
	{0, XK_n, quickfocus, {.ui = 'n'}},
	{0, XK_m, quickfocus, {.ui = 'm'}},
	{0, XK_u, quickfocus, {.ui = 'u'}},

	{0, XK_s, quickfocus, {.ui = 's'}},
	{0, XK_d, quickfocus, {.ui = 'd'}},
	{0, XK_f, quickfocus, {.ui = 'f'}},
	{0, XK_g, quickfocus, {.ui = 'g'}},

	{0, XK_w, quickfocus, {.ui = 'w'}},
	{0, XK_e, quickfocus, {.ui = 'e'}},
	{0, XK_r, quickfocus, {.ui = 'r'}},

	{MODKEY, XK_m, container_layout_tile_v_movesplit_toggle, {.i = 2}},

	{ MODKEY|ControlMask,                       XK_l,      tile5expandx,       {.f = 0.1} },
	{ MODKEY|ControlMask,                       XK_h,      tile5expandx,       {.f = -0.1} },
	{ MODKEY|ControlMask,                       XK_Right,      tile5expandx,       {.f = 0.1} },
	{ MODKEY|ControlMask,                       XK_Left,      tile5expandx,       {.f = -0.1} },
	{ MODKEY|ControlMask,                       XK_j,      tile5expandy,       {.f = -0.1} },
	{ MODKEY|ControlMask,                       XK_k,      tile5expandy,       {.f = 0.1} },
	{ MODKEY|ControlMask,                       XK_Down,      tile5expandy,       {.f = -0.1} },
	{ MODKEY|ControlMask,                       XK_Up,      tile5expandy,       {.f = 0.1} },


	{MODKEY,                       XK_equal,      tile5expand,       {.f=0.1} },
	{ MODKEY,                       XK_minus,      tile5expand,       {.f=-0.1} },

	{MODKEY|ShiftMask,XK_b,togglebar,{0} },
	//{MODKEY, XK_g, toggleswitchersticky, {0}},
	{0, XK_Left, switchermove, {.i = -1}},
	{0, XK_Right, switchermove, {.i = 1}},
	{0, XK_Up, switchermove, {.i = 2}},
	{0, XK_Down, switchermove, {.i = -2}},
	{0, XK_h, switchermove, {.i = -1}},
	{0, XK_l, switchermove, {.i = 1}},
	{0, XK_k, switchermove, {.i = 2}},
	{0, XK_j, switchermove, {.i = -2}},
	{MODKEY, XK_Left, switchermove, {.i = -1}},
	{MODKEY, XK_Right, switchermove, {.i = 1}},
	{MODKEY, XK_Up, switchermove, {.i = 2}},
	{MODKEY, XK_Down, switchermove, {.i = -2}},
	{MODKEY, XK_h, switchermove, {.i = -1}},
	{MODKEY, XK_l, switchermove, {.i = 1}},
	{MODKEY, XK_k, switchermove, {.i = 2}},
	{MODKEY, XK_j, switchermove, {.i = -2}},

	{ControlMask|Mod1Mask, XK_Left, tile7switchermovecontainer, {.i = -1}},
	{ControlMask|Mod1Mask, XK_Right, tile7switchermovecontainer, {.i = 1}},
	{ControlMask|Mod1Mask, XK_Up, tile7switchermovecontainer, {.i = 2}},
	{ControlMask|Mod1Mask, XK_Down, tile7switchermovecontainer, {.i = -2}},

	{0, XK_Return, toggleswitchers, {0}},
	{0, XK_Escape, toggleswitchers, {0}},
	{0, XK_Super_L, toggleswitchers, {0}},
	{0, XK_Super_R, toggleswitchers, {0}},
	{0, XF86XK_Tools, toggleswitchers, {0}},
	{MODKEY, XK_Return, toggleswitchers, {0}},
	// {MODKEY, XK_grave, tile6maximizewithsticky, {0}},
	{MODKEY, XK_grave, i_maxwindow, {0} },

	{ MODKEY,			XK_F2, killclientforce, {0}},
	{ MODKEY,                       XK_q, sspawn,          {.v = browsercmd} },
	{ MODKEY,                       XK_e, rispawn,          {.v = fmcmd} },
	{ MODKEY,                       XK_slash, spawn,          {.v = fmclipcmd} },
	{ MODKEY,                       XK_equal, tile6zoom,          { .f = 0.1 } },
	{ MODKEY,                       XK_minus, tile6zoom,          { .f = -0.1} },
	{ MODKEY,                       XK_Return, stispawn,          { 0 } },
	{ MODKEY,                       XK_KP_Enter, stispawn,          { 0 } },
	{ MODKEY|ShiftMask,                       XK_Return, stspawn,          { 0 } },
	{ MODKEY|ShiftMask,                       XK_KP_Enter, stspawn,          { 0 } },
	{ MODKEY,                       XK_b, assemblecsv,          { .v = &taskgrouppath } },
	//{ MODKEY,                       XK_q, tsspawn,          {.v = querybrowsercmd } },
	{ MODKEY,                       XK_a, tsspawn,          {.v = dictcmd } },
	// { Mod1Mask,                       XK_a, tsspawn,          {.v = dictcmd } },
	{ MODKEY,                       XK_z, tsspawn,          {.v = todotxtcmd} },
	{ MODKEY,                       XK_x, tsspawn,          {.v = enotecmd} },
	{ MODKEY,                       XK_s,      spawn,          {.v = flameshotcmd } },
	{ MODKEY,                       XK_p,      addtoscratchgroup,  {0 } },
	{ MODKEY,                       XK_y,      removefromscratchgroup,  {0 } },
	// { MODKEY,                       XK_m,      removefromscratchgroup,  {0 } },
	{ MODKEY,                       XK_n,      togglescratchgroup,  {0} },
	{ MODKEY,                       XK_r,      rerule,      {0} },

	{ MODKEY|ShiftMask,                       XK_j,      i_move,       {.i = -2} },
	{ MODKEY|ShiftMask,                       XK_k,      i_move,       {.i = +2} },
	{ MODKEY|ShiftMask,                       XK_l,      i_move,       {.i = +1} },
	{ MODKEY|ShiftMask,                       XK_h,      i_move,       {.i = -1} },
	{ MODKEY|ShiftMask,                       XK_Down,      i_move,       {.i = -2} },
	{ MODKEY|ShiftMask,                       XK_Up,     i_move,       {.i = +2} },
	{ MODKEY|ShiftMask,                       XK_Right,      i_move,       {.i = +1} },
	{ MODKEY|ShiftMask,                       XK_Left,      i_move,       {.i = -1} },

    { MODKEY|ControlMask,                       XK_l,      i_expandx,       {.f = 0.2} },
	{ MODKEY|ControlMask,                       XK_h,      i_expandx,       {.f = -0.2} },
	 { MODKEY|ControlMask,                       XK_Right,      i_expandx,       {.f = 0.2} },
	{ MODKEY|ControlMask,                       XK_Left,      i_expandx,       {.f = -0.2} },
	 { MODKEY|ControlMask,                       XK_j,      i_expandy,       {.f = 0.2} },
	{ MODKEY|ControlMask,                       XK_k,      i_expandy,       {.f = -0.2} },
	 { MODKEY|ControlMask,                       XK_Down,      i_expandy,       {.f = 0.2} },
	{ MODKEY|ControlMask,                       XK_Up,      i_expandy,       {.f = -0.2} },

	// { MODKEY|ControlMask,                       XK_j,      tiledual,       {.i = -2} },
	// { MODKEY|ControlMask,                       XK_k,      tiledual,       {.i = +2} },
	// { MODKEY|ControlMask,                       XK_l,      tiledual,       {.i = +1} },
	// { MODKEY|ControlMask,                       XK_h,      tiledual,       {.i = -1} },
	// { MODKEY|ControlMask,                       XK_Down,      tiledual,       {.i = -2} },
	// { MODKEY|ControlMask,                       XK_Up,      tiledual,       {.i = +2} },
	// { MODKEY|ControlMask,                       XK_Right,      tiledual,       {.i = +1} },
	// { MODKEY|ControlMask,                       XK_Left,      tiledual,       {.i = -1} },
	{ MODKEY|ShiftMask,                       XK_n,      setfacty,       {0} },
	{ MODKEY,                       XK_Tab,    focuslast,           { 0 } },
	// { MODKEY,                       XK_Tab,    tile7shiftnext,           { 0 } },
	{ MODKEY,             XK_KP_Page_Up, zoomi,           {.i=1} },
	{ MODKEY,             XK_KP_Right, zoomi,           {.i=2} },
	{ MODKEY,             XK_KP_Page_Down, zoomi,           {.i=3} },
	{ MODKEY,             XK_KP_Delete, zoomi,           {.i=1000} },
	{ MODKEY,             XK_comma, zoomi,           {.i=1} },
	{ MODKEY,             XK_period, zoomi,           {.i=1000} },
	{ MODKEY|ShiftMask,             XK_Return, zoom,           {0} },
	{ MODKEY,                       XK_u,    relview,           {.i=-1} },
	{ MODKEY,                       XK_o,    relview,           {.i=1} },
	{ MODKEY|ShiftMask,                       XK_u,    reltag,           {.i=-1} },
	{ MODKEY|ShiftMask,                       XK_o,    reltag,           {.i=1} },
	{ MODKEY|ShiftMask,                       XK_minus,    reltagd,           {.i=-1} },
	{ MODKEY|ShiftMask,                       XK_equal,    reltagd,           {.i=1} },
	{ MODKEY,                       XK_F1,      killclient,     {0} },
	{ MODKEY,                       XK_t,      setlayout,      {.v = &layouts[0]} },
	{ MODKEY,                       XK_d,      doublepagemark,      {.v = &layouts[4]} },
	{ MODKEY,                       XK_g,      setcontainerlayout,      {0} },
	{ MODKEY,                       XK_v,      setlayout,      {.v=&layouts[8]} },
	//{ MODKEY,                       XK_g,      toggleswitchers,      {0} },
	{ MODKEY,                       XK_space,  tile5toggleshift,      {0} },
	{ MODKEY|ShiftMask,             XK_space,  tile5togglefloating, {0} },
	{ MODKEY,                       XK_0,      view,           {.ui = ~0 } },
	{ MODKEY|ShiftMask,             XK_0,      tag,            {.ui = ~0 } },
	{ MODKEY|ShiftMask,             XK_comma,  tagmon,         {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_period, tagmon,         {.i = +1 } },
	{ MODKEY,                       XK_minus,  setgaps,        {.i = -5 } },
	{ MODKEY,                       XK_equal,  setgaps,        {.i = +5 } },
	{ MODKEY|ShiftMask,             XK_minus,  setgaps,        {.i = GAP_RESET } },
	{ MODKEY|ShiftMask,             XK_equal,  setgaps,        {.i = GAP_TOGGLE} },
	TAGKEYS(                        XK_1,                      0)
	TAGKEYS(                        XK_2,                      1)
	TAGKEYS(                        XK_3,                      2)
	TAGKEYS(                        XK_4,                      3)
	TAGKEYS(                        XK_5,                      4)
	TAGKEYS(                        XK_6,                      5)
	TAGKEYS(                        XK_7,                      6)
	TAGKEYS(                        XK_8,                      7)
	TAGKEYS(                        XK_9,                      8)
	{ MODKEY|ShiftMask,             XK_q,      quit,           {0} },
	{ MODKEY|ControlMask|ShiftMask, XK_q,      quit,           {1} }, 
	{ MODKEY, XK_F9,        spawn, SHCMD("mpc clear; mpc update; mpc listall | mpc add; mpc shuffle; mpc random on; mpc play; pkill -RTMIN+1 dwmblocks") },
	{ MODKEY, XK_F10,        spawn, SHCMD("mpc prev; pkill -RTMIN+1 dwmblocks") },
	{ MODKEY, XK_F12,        spawn, SHCMD("echo \"$(mpc -f %file% | sed -n \"1,1p\") $(mpc | sed -n \"2,2p\" | awk '{print $4}' | grep -Eo \"[0-9]*\") \" >> $HOME/.config/netease/play_skip.log.csv;mpc next; ") },
	{ MODKEY, XK_F11,        spawn, SHCMD("mpc toggle; pkill -RTMIN+1 dwmblocks") },
	{ MODKEY, XK_F8,        spawn, SHCMD("mpc sticker $(mpc current) set like yes") },
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
	//{ ClkWinTitle,          0,              Button1,        zoom,           {0} },
	{ ClkWinTitle,          0,              Button2,        zoom,           {0} },
	{ ClkStatusText,        0,              Button1,        sigstatusbar,   {.i = 1} },
	{ ClkStatusText,        0,              Button2,        sigstatusbar,   {.i = 2} },
	{ ClkStatusText,        0,              Button3,        sigstatusbar,   {.i = 3} },
	{ ClkClientWin,         Mod1Mask|ShiftMask,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         Mod1Mask|ShiftMask,         Button3,        resizemouse,    {0} },
	{ ClkClientWin,         0,              Button2,        spawn,    {.v = smartclosecmd} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};

static const char *ipcsockpath = "/tmp/dwm.sock";
static IPCCommand ipccommands[] = {
  IPCCOMMAND(  view,                1,      {ARG_TYPE_UINT}   ),
  IPCCOMMAND(  toggleview,          1,      {ARG_TYPE_UINT}   ),
  IPCCOMMAND(  tag,                 1,      {ARG_TYPE_UINT}   ),
  IPCCOMMAND(  toggletag,           1,      {ARG_TYPE_UINT}   ),
  IPCCOMMAND(  tagmon,              1,      {ARG_TYPE_UINT}   ),
  IPCCOMMAND(  focusmon,            1,      {ARG_TYPE_SINT}   ),
  IPCCOMMAND(  focusstack,          1,      {ARG_TYPE_SINT}   ),
  IPCCOMMAND(  zoom,                1,      {ARG_TYPE_NONE}   ),
  IPCCOMMAND(  incnmaster,          1,      {ARG_TYPE_SINT}   ),
  IPCCOMMAND(  killclient,          1,      {ARG_TYPE_SINT}   ),
  IPCCOMMAND(  togglefloating,      1,      {ARG_TYPE_NONE}   ),
  IPCCOMMAND(  setmfact,            1,      {ARG_TYPE_FLOAT}  ),
  IPCCOMMAND(  setlayoutsafe,       1,      {ARG_TYPE_PTR}    ),
  IPCCOMMAND(  nextsidecar,         1,      {ARG_TYPE_NONE}    ),
  IPCCOMMAND(  nextmanagetype,         1,      {ARG_TYPE_UINT}    ),
  IPCCOMMAND(  container_layout_tile_v_movesplit_toggle,         1,      {ARG_TYPE_SINT}    ),
//   IPCCOMMAND(  setcontainerlayout,  1,      {ARG_TYPE_PTR}    ),
  IPCCOMMAND(  quit,                1,      {ARG_TYPE_NONE}   )
};

