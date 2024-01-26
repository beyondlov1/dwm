// source file : win.c
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xatom.h>
#include <linux/input.h>  
#include <linux/uinput.h>  
#include <unistd.h>
#include <fcntl.h>  

typedef struct XY {
  int x;
  int y;
} XY; 

static char g[64][64][4];
static char cmd[4] = { '\0'};
static int cmdcursor = 0;
static int keep_running = 1;
static int success = 0;

void
draw(Display *display, GC gc, Window win, int width, int height, char candidates[3][17], XY *result) {

  XFontStruct* font;
  font = XLoadQueryFont(display,"*-helvetica-*-24-*");
  XSetFont(display, gc, font->fid);

  int unit_row = 3;
  int unit_col = 3;
  int xunit = unit_row * unit_row * unit_row;
  int yunit = unit_col * unit_col * unit_col;
  int selectcnt = 0;
  int lastx = 0;
  int lasty = 0;
  for (int i = 0; i < xunit; i++) {
    for (int j = 0; j < yunit; j++) {
      int row1 = i / (xunit / unit_row);
      int col1 = j / (yunit / unit_col);
      int level1 = row1 * unit_col + col1;
      int row2 = (i - row1 * (xunit/unit_row)) / unit_row;
      int col2 = (j - col1 * (yunit/unit_col)) / unit_col;
      int level2 = row2 * unit_col + col2;
      int row3 = (i - row1 * (xunit/unit_row) - row2 * (xunit/unit_row/unit_row));
      int col3 = (j - col1 * (yunit/unit_col) - col2 * (yunit/unit_col/unit_col));
      int level3 = row3 * unit_col + col3;
      // int awidth = width / 63;
      // int aheight = height / 63;
      int awidth = width / yunit;
      int aheight = height / xunit;
      char msg[4] = {candidates[0][level1], candidates[1][level2], candidates[2][level3], '\0'};
      if (strlen(cmd) > 0 && strncmp(cmd, msg, strlen(cmd)) == 0) {
        XSetForeground(display, gc, 0x80300000);
        selectcnt ++;
        lastx = awidth * j + awidth/2;
        lasty = aheight * i + aheight/2;
      } else {
        XSetForeground(display, gc, 0x80000000);
      }
      XFillRectangle(display, win, gc, awidth * j, aheight * i, awidth,  aheight);
      if (strlen(cmd)&& strncmp(cmd, msg, strlen(cmd)) == 0) {
        XSetForeground(display, gc, 0x403fffff);
      // }else if (cmdcursor == 0|| cmdcursor ==  3){
      //   XSetForeground(display, gc, 0x80ffffff);
      // }
      }else{
        XSetForeground(display, gc, 0x80ffffff);
        // XSetForeground(display, gc, 0x80F5CA40);
      }
      // 文字以左下角为基准
      int textw = XTextWidth(font, msg, strlen(msg));
      XDrawString(display, win, gc, awidth * j + awidth/2 - textw/2, aheight * i + aheight/2 + 12, msg, strlen(msg));
      strcpy(g[i][j], msg);
    }
  }
  if(selectcnt == 1){
    result->x = lastx;
    result->y = lasty;
  }
}

void simulate_key(int fd,int kval)  
{  
    struct input_event event;  
    event.type = EV_KEY;  
    event.value = 1;  
    event.code = kval;  

    gettimeofday(&event.time,0);  
    write(fd,&event,sizeof(event)) ;  

    event.type = EV_SYN;  
    event.code = SYN_REPORT;  
    event.value = 0;  
    write(fd, &event, sizeof(event));  

    memset(&event, 0, sizeof(event));  
    gettimeofday(&event.time, NULL);  
    event.type = EV_KEY;  
    event.code = kval;  
    event.value = 0;  
    write(fd, &event, sizeof(event));  
    event.type = EV_SYN;  
    event.code = SYN_REPORT;  
    event.value = 0;  
    write(fd, &event, sizeof(event));  
}  

void simulate_mouse_key(int fd, int keycode)  
{  
  struct input_event event;  
  memset(&event, 0, sizeof(event));  

  gettimeofday(&event.time, NULL);  
  event.type = EV_MSC;  
  event.code = 4;  
  event.value = 90001;  
  write(fd, &event, sizeof(event));  

  gettimeofday(&event.time, NULL);  
  event.type = EV_KEY;  
  event.code = keycode;  
  event.value = 1;  
  write(fd, &event, sizeof(event));  

  gettimeofday(&event.time, NULL);  
  event.type = EV_SYN;  
  event.code = SYN_REPORT;  
  event.value = 0;  
  write(fd, &event, sizeof(event));  

  gettimeofday(&event.time, NULL);  
  event.type = EV_MSC;  
  event.code = 4;  
  event.value = 90001;  
  write(fd, &event, sizeof(event));  

  gettimeofday(&event.time, NULL);  
  event.type = EV_KEY;  
  event.code = keycode;  
  event.value = 0;  
  write(fd, &event, sizeof(event));  

  gettimeofday(&event.time, NULL);  
  event.type = EV_SYN;  
  event.code = SYN_REPORT;  
  event.value = 0;  
  write(fd, &event, sizeof(event));  

}

int keypress(XEvent event, Display *display,  GC gc,Window root,  Window win, int width,
             int height, char candidates[3][17]) {
  unsigned int keycode = event.xkey.keycode;
  // printf("%d", keycode);
  /*return keycode;*/
  // printf("%s", g[5][2]);
  // printf("%c", keycode);
  int keysyms_per_keycode;
  KeySym *keysym =
      XGetKeyboardMapping(display, keycode, 1, &keysyms_per_keycode);
  if (*keysym <= XK_z && *keysym >= XK_a) {
    char *result = XKeysymToString(keysym[0]);
    if (cmdcursor == 3) {
      cmdcursor = 0;
    }
    cmd[cmdcursor] = result[0];
    cmdcursor++;
    cmd[cmdcursor] = '\0';
    // printf("%s", result);
    // printf("%s", cmd);
    XY xy = {0};
    draw(display, gc, win, width, height, candidates,&xy);
    if(xy.x != 0 && xy.y != 0){
		  XWarpPointer(display, None, root, 0, 0, 0, 0, xy.x, xy.y);
		  keep_running = 0;
		  success = 1;
    }
  }
  if(*keysym == XK_Escape) {
    keep_running = 0;
    success = 0;
  }
  return 0;
}

int main(int argc, char *argv[]) {
  Display *display = XOpenDisplay(NULL);

  XVisualInfo vinfo;
  XMatchVisualInfo(display, DefaultScreen(display), 32, TrueColor, &vinfo);

  XSetWindowAttributes attr; // 窗口属性设置
  attr.colormap = XCreateColormap(display, DefaultRootWindow(display),
                                  vinfo.visual, AllocNone);
  attr.border_pixel = 0;
  attr.background_pixel = 0;
  attr.event_mask =
      ButtonPressMask | ExposureMask | KeyPressMask | PointerMotionMask;

  // 取输出设备的长宽像素
  int screen = DefaultScreen(display);
	Window root = RootWindow(display, screen);
  int height = DisplayHeight(display, screen);
  int width = DisplayWidth(display, screen);

  // 开始创建窗口
  /*Window win = XCreateWindow(display, DefaultRootWindow(display), 0, 0,width,
   * height , 0, vinfo.depth, InputOutput, vinfo.visual,  CWColormap |
   * CWBorderPixel | CWBackPixel, &attr);*/
  XSetWindowAttributes wa = {/*.override_redirect = True,*/
                             /*.background_pixmap = ParentRelative,*/
                             .background_pixmap = 0,
                             .event_mask = ButtonPressMask | ExposureMask |
                                           KeyPressMask | PointerMotionMask};
  Window win = XCreateWindow(
      display, DefaultRootWindow(display), 0, 0, width, height, 0,
      // DefaultDepth(display, screen),
      vinfo.depth,
      // InputOutput,
      CopyFromParent,
      /*DefaultVisual(display, screen),*/
      vinfo.visual,
      /*CWOverrideRedirect|CWBackPixmap|CWEventMask, */
      CWColormap | CWBorderPixel | CWBackPixel | CWEventMask, &attr);

  // XSetTransientForHint(display, win, root);
	Atom netWMWindowTypeDialog = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", False);
	Atom netWMWindowTypeFullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
	Atom netWMWindowType = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
	Atom netWMState = XInternAtom(display, "_NET_WM_STATE", False);
	// XChangeProperty(display, win, netWMWindowType, XA_ATOM, 32,
	// 		PropModeReplace, (unsigned char*)&netWMWindowTypeDialog, 1);
	XChangeProperty(display, win, netWMState, XA_ATOM, 32,
			PropModeReplace, (unsigned char*)&netWMWindowTypeFullscreen, 1);

	XClassHint ch = { "vimmask" , "vimmask"};
	XSetClassHint(display, win, &ch);

  XSelectInput(display, win, StructureNotifyMask | ExposureMask | KeyPressMask);
  GC gc = XCreateGC(display, win, 0, 0);

  /*
  Atom wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", 0);
  XSetWMProtocols(display, win, &wm_delete_window, 1);
  */

  /*char *msg = "hello ";*/
  /*XDrawString(display, win, DefaultGC(display, screen), 10, 50, msg,
   * strlen(msg));*/
  XMapWindow(display, win);
  XMoveWindow(display, win, 0, 0);

  XFlush(display); // 刷新输出设备
  XEvent event;
  /*XSetForeground(display,gc, 0x80ffffff);*/
  /*XFillRectangle(display, win, gc, 20, 20, 10, 10);*/

  char candidates[][17] = {"asdfwerqgtjkluiop","jlkuiohnmasdfgrew","asdfwerqgtjkluiop"};
  XY xy = {0};

  // 窗口事件监听尝试
  while (keep_running) {
    XNextEvent(display, &event);
    switch (event.type) {
    case ClientMessage:
      if (event.xclient.message_type ==
              XInternAtom(display, "WM_PROTOCOLS", 1) &&
          (Atom)event.xclient.data.l[0] ==
              XInternAtom(display, "WM_DELETE_WINDOW", 1))
        keep_running = 0;
      break;
    case Expose:
      draw(display, gc, win, width, height, candidates, &xy);
      break;
    case KeyPress:
      keypress(event, display, gc,root, win, width, height, candidates);
      break;
    default:
      break;
    }
  }

  // 结束销毁
  XDestroyWindow(display, win);
  XSync(display, 0);

  if (success) 
  {
    char outDevicePath[64];
    strcpy(outDevicePath, "/dev/input/");
    strcat(outDevicePath, argv[1]);
    int outDeviceFd = open(outDevicePath, O_RDWR);
     if (outDeviceFd == -1)
     {
        perror("Failed to open input device");
        return 1;
     }
    simulate_mouse_key(outDeviceFd, 272);
  }
  XCloseDisplay(display);

  return 0;
}
