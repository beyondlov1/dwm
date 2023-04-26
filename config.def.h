/* Include */
#include <X11/XF86keysym.h>
/* See LICENSE file for copyright and license details. */

/* appearance */
static const unsigned int borderpx  = 2;        /* border pixel of windows */
static const Gap default_gap        = {.isgap = 1, .realgap = 10, .gappx = 15};
static const unsigned int snap      = 32;       /* snap pixel */
static const unsigned int systraypinning = 0;   /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
static const unsigned int systrayonleft = 0;    /* 0: systray in the right corner, >0: systray on left of status text */
static const unsigned int systrayspacing = 2;   /* systray spacing */
static const int systraypinningfailfirst = 1;   /* 1: if pinning fails, display systray on the first monitor, False: display systray on the last monitor*/
static const int showsystray        = 1;        /* 0 means no systray */
static const int showbar            = 1;        /* 0 means no bar */
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
	[SchemeSel] = {col_gray4, col_cyan, "#00BFFF"},
	[SchemeScr] = {col_gray4, col_cyan, "#cf110b"},
	[SchemeInvalidNormal] = {col_gray2, col_gray1, col_gray2},
	[SchemeInvalidSel] = {col_gray2, col_cyan, "#00BFFF"},
	[SchemeDoublePageMarked] = {col_gray2, col_cyan, "#ff9300"},
	[SchemeTiled] = {"#ff9300",col_gray1, col_gray2},
};

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

static const Rule defaultrule = { "",              NULL,       NULL,       NULL,       0,           -1 , 0,              0};
static const Rule rules[] = {
	/* xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 */
	/* class                 instance    title       tags mask     isfloating   monitor  priority   nstub */
	{"firefox", NULL, NULL, NULL, 0, -1, 5, 2},
	{"Google-chrome", NULL, NULL, NULL, 0, -1, 5, 2},
	{"Sidekick-browser", NULL, NULL, NULL, 0, -1, 5, 2},
	{"Brave-browser-nightly", NULL, NULL, NULL, 0, -1, 5, 2},
	{"X-terminal-emulator", NULL, NULL, NULL, 0, -1, 1, -1},
	{"Terminator", NULL, NULL, NULL, 0, -1, 1, -1},
	{"St", NULL, NULL, NULL, 0, -1, 1, -1},
	{"Code", NULL, NULL, NULL, 0, -1, 5, 0},
	{"jetbrains-idea", NULL, NULL, NULL, 0, -1, 5, 0},
	{"jetbrains-datagrip", NULL, NULL, NULL, 0, -1, 5, 2},
	{"Evince", NULL, NULL, NULL, 0, -1, 5, 2},
	{"flameshot", NULL, NULL, 0xFF, 1, -1, 5, 0},
	{"Fragcode", NULL, NULL, NULL, 1, -1, 5, 0},
	{"Thunar", NULL, NULL, NULL, 1, -1, 5, 0},
	{"ToDesk", NULL, NULL, NULL, 0, -1, 5, 2},
};

static const Rule subjrules[] = {
	/* xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 */
	/* class                  instance    title       tags mask     isfloating   monitor  priority  nstub*/
	{ "firefox",              NULL,       NULL,       1 << 1,       0,           -1 , 5,              2},
	{ "Google-chrome",        NULL,       NULL,       1 << 1,       0,           -1 , 5,              2},
	{ "Sidekick-browser",     NULL,       NULL,       1 << 1,       0,           -1 , 5,              2},
	{ "Brave-browser-nightly",     NULL,       NULL,       1 << 1,       0,           -1 , 5,              2},
	{ "X-terminal-emulator",  NULL,       NULL,       1 << 0,       0,           -1 , 1,              -1},
	{ "Terminator",  NULL,       NULL,       1 << 0,       0,           -1 , 1,              -1},
	{ "St",  NULL,       NULL,       1 << 0,       0,           -1 , 1,              -1},
	{ "Code",                 NULL,       NULL,       1 << 3,       0,           -1 , 5,              0},
	{ "jetbrains-idea",       NULL,       NULL,       1 << 2,       0,           -1 , 5,              0},
	{ "jetbrains-datagrip",   NULL,       NULL,       1 << 4,       0,           -1 , 5,              2},
	{ "Evince",               NULL,       NULL,       1 << 5,       0,           -1 , 5,              2},
	{ "flameshot",            NULL,       NULL,       0xFF,       1,           -1 , 5,              0},
};

/* layout(s) */
static const float mfact     = 0.60; /* factor of master area size [0.05..0.95] */
static const float factx     = 0.05; /* factor of master area size [0.05..0.95] */
static const float facty     = 0.05; /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;    /* number of clients in master area */
static const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen = 1; /* 1 will force focus on the fullscreen window */

/* Include */
#include "sort.c"
#include "gaplessgrid.c"
#include "gapgrid.c"
#include "gapgridsorted.c"
#include "gapgridsortedneat.c"
static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "[]=",      tile6 },
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
static const char *dmenucmd[] = { "dmenu_run", "-m", dmenumon, "-fn", dmenufont, "-nb", col_gray1, "-nf", col_gray3, "-sb", col_cyan, "-sf", col_gray4, NULL };
//static const char *termcmd[]  = { "st", NULL };
static const char scratchpadname[] = "scratchpad";
// static const char *scratchpadcmd[] = { "st", "-t", scratchpadname, "-g", "120x34", NULL };
static const char *scratchpadcmd[] = {"jumpapp", "chrome", NULL };
//static const char *roficmd[] = {"rofi","-auto-select","-show","window",NULL};
static const char *roficmd[] = {"rofi","-auto-select","-show","fb","-modes ","fb:~/software/bin/rofi-script/switchwin.sh" ,"-matching","prefix_s", "-hover-select", "-me-select-entry", "", "-me-accept-entry","MousePrimary",NULL};
static const char *firefoxcmd[] = {"/usr/bin/firefox",NULL};
static const char *flameshotcmd[] = {"flameshot","gui",NULL};
static const char *browser[] = {"sidk.sh",NULL};
static const char *notecmd[] = {"qt-note.sh",NULL};
static const char *querycmd[] = {"/home/beyond/software/queryclip.sh", "http://cn.bing.com/search?q=%s", NULL};
static const char *querybrowsercmd[] = {"/home/beyond/software/browserclip.sh", "https://cn.bing.com/search?q=%s", NULL};
static const char *dictcmd[] = {"/home/beyond/software/queryclip.sh", "http://youdao.com/result?word=%s&lang=en", NULL};
//static const char *todotxtcmd[] = {"st","-e","/home/beyond/software/todotxtcmd.sh",NULL};
static const char *todotxtcmd[] = {"st","-e","python3", "/home/beyond/software/bin/textual-demo/app.py",NULL};
static const char *enotecmd[] = {"st","-e","python3", "/home/beyond/software/bin/enotepy/app.py",NULL};
static const char *copyqcmd[] = {"copyq", "toggle", NULL };

static const TaskGroup taskgroup1 = {
	2,
	{
		{
			"^St$","^dwm$",&((char *[]){"st",NULL}),1<<5,NULL,NULL,NULL,
		},
		{
			"^St$","^.*rofi.*$",&((char *[]){"st",NULL}),1<<5,NULL,NULL,NULL
		},
	}
};

static const char *taskgrouppath = "/home/beyond/software/bin/dwm-taskgroup/1.csv";

static Key keys[] = {
	/* modifier                     key        function        argument */
	//{ MODKEY,                       XK_Return, sspawn,          {.v = terminal } },
	//{ MODKEY,                       XK_Return, stsspawn,          { 0 } },
	{ MODKEY,                       XK_Return, stspawn,          { 0 } },
	{ MODKEY,                       XK_b, assemblecsv,          { .v = &taskgrouppath } },
	{ MODKEY,                       XK_q, tsspawn,          {.v = querybrowsercmd } },
	{ ControlMask,                  XK_space, spawn,          {.v = copyqcmd } },
	{ MODKEY,                       XK_a, tsspawn,          {.v = dictcmd } },
	{ MODKEY,                       XK_z, tsspawn,          {.v = todotxtcmd} },
	{ MODKEY,                       XK_x, tsspawn,          {.v = enotecmd} },
	// { MODKEY,                       XK_i, sspawn,          {.v = notecmd } },
	{ MODKEY,                       XK_s,      spawn,          {.v = flameshotcmd } },
	// { MODKEY,                       XK_grave,  togglescratch,  {.v = scratchpadcmd } },
	{ MODKEY,                       XK_p,      addtoscratchgroup,  {0 } },
	{ MODKEY,                       XK_y,      removefromscratchgroup,  {0 } },
	{ MODKEY,                       XK_n,      togglescratchgroup,  {0} },
	{ Mod1Mask,                       XK_grave,      togglescratchgroup,  {0} },
	// { MODKEY,                       XK_b,      togglebar,      {0} },
	{ MODKEY,                       XK_r,      rerule,      {0} },
	// { MODKEY|ShiftMask,             XK_j,      rotatestack,    {.i = +1 } },
	// { MODKEY|ShiftMask,             XK_k,      rotatestack,    {.i = -1 } },
	//{ MODKEY|ShiftMask,                       XK_j,      movey,     {.i = 100 } },
	//{ MODKEY|ShiftMask,                       XK_k,      movey,     {.i = -100 } },
	//{ MODKEY|ShiftMask,                       XK_l,      movex,     {.i = 200 } },
	//{ MODKEY|ShiftMask,                       XK_h,      movex,     {.i = -200 } },
	{ MODKEY,                       XK_j,      focusgrid5,     {.i = -2 } },
	{ MODKEY,                       XK_k,      focusgrid5,     {.i = +2 } },
	{ MODKEY,                       XK_l,      focusgrid5,     {.i = +1 } },
	{ MODKEY,                       XK_h,      focusgrid5,     {.i = -1 } },
	{ MODKEY,                       XK_Down,      focusgrid5,     {.i = -2 } },
	{ MODKEY,                       XK_Up,      focusgrid5,     {.i = +2 } },
	{ MODKEY,                       XK_Right,      focusgrid5,     {.i = +1 } },
	{ MODKEY,                       XK_Left,      focusgrid5,     {.i = -1 } },
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
	
	{ MODKEY|ShiftMask,                       XK_j,      swap,       {.i = -2} },
	{ MODKEY|ShiftMask,                       XK_k,      swap,       {.i = +2} },
	{ MODKEY|ShiftMask,                       XK_l,      swap,       {.i = +1} },
	{ MODKEY|ShiftMask,                       XK_h,      swap,       {.i = -1} },

	{ MODKEY|ShiftMask,                       XK_n,      setfacty,       {0} },
	// { MODKEY,             XK_Tab, smartzoom,           {0} },
	// { MODKEY,             XK_Tab,   toggleswitchers,           {0} },
	{ MODKEY,                       XK_Tab,    view,           {.ui = ~0 } },
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
	//{ MODKEY,                       XK_grave,    spawn,           {.v = roficmd} },
	{ MODKEY,                       XK_grave,      toggleswitchers,      {0} },
	{ MODKEY,                       XK_F1,      killclient,     {0} },
	{ MODKEY,                       XK_t,      setlayout,      {.v = &layouts[0]} },
	//{ MODKEY,                       XK_f,      setlayout,      {.v = &layouts[1]} },
	// { MODKEY,                       XK_d,      setlayoutt,      {.v = &layouts[4]} },
	{ MODKEY,                       XK_d,      doublepagemark,      {.v = &layouts[4]} },
	{ MODKEY,                       XK_e,      setlayout,      {.v = &layouts[9]} },
	// { MODKEY,                       XK_m,      setlayout,      {.v = &layouts[2]} },
	{ MODKEY,                       XK_m,      setlayout,      {0} },
	{ MODKEY,                       XK_v,      setlayout,      {.v=&layouts[8]} },
	// { MODKEY,                       XK_g,      toggleswitchers,      {0} },
	{ MODKEY,                       XK_g,      toggleswitchers,      {0} },
	// { MODKEY,                       XK_b,      destroyswitchers,      {0} },
	{ MODKEY,                       XK_semicolon,      spawn,           {.v = roficmd} },
	{ MODKEY,                       XK_space,  setlayout,      {0} },
	{ MODKEY|ShiftMask,             XK_space,  togglefloating, {0} },
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
	{0, XK_grave, switchermove2cycle, {0}},
	{0, XK_Left, switchermove2, {.i = -1}},
	{0, XK_Right, switchermove2, {.i = 1}},
	{0, XK_Up, switchermove2, {.i = 2}},
	{0, XK_Down, switchermove2, {.i = -2}},
	{0, XK_h, switchermove2, {.i = -1}},
	{0, XK_l, switchermove2, {.i = 1}},
	{0, XK_k, switchermove2, {.i = 2}},
	{0, XK_j, switchermove2, {.i = -2}},
	{0, XK_Return, switcherview, {0}},
	{MODKEY, XK_Left, switchermove, {.i = -1}},
	{MODKEY, XK_Right, switchermove, {.i = 1}},
	{MODKEY, XK_Up, switchermove, {.i = 2}},
	{MODKEY, XK_Down, switchermove, {.i = -2}},
	{MODKEY, XK_h, switchermove, {.i = -1}},
	{MODKEY, XK_l, switchermove, {.i = 1}},
	{MODKEY, XK_k, switchermove, {.i = 2}},
	{MODKEY, XK_j, switchermove, {.i = -2}},
	{MODKEY, XK_Return, switcherview, {0}},
	{MODKEY, XK_Tab, toggleswitchers, {0}},
	{0, XK_Escape, toggleswitchers, {0}},

	{MODKEY, XK_grave, toggleswitchers, {0}},
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
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};

