// source file : win.c
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <string.h>

int keypress(XEvent event) {
  unsigned int keycode = event.xkey.keycode;
  /*printf("%d", keycode);*/
  /*return keycode;*/
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
      /*DefaultDepth(display, screen),*/
      vinfo.depth,
      /*InputOutput,*/
      CopyFromParent,
      /*DefaultVisual(display, screen),*/
      vinfo.visual,
      /*CWOverrideRedirect|CWBackPixmap|CWEventMask, */
      CWColormap | CWBorderPixel | CWBackPixel | CWEventMask, &attr);
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
  int keep_running = 1;
  XEvent event;
  /*XSetForeground(display,gc, 0x80ffffff);*/
  /*XFillRectangle(display, win, gc, 20, 20, 10, 10);*/

  char tmp;
  char cmd[4] = {0};

  char candidates[17] = "abcdefghijklmnop";
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
      for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 64; j++) {
          int row1 = i / 16;
          int col1 = j / 16;
          int level1 = row1 * 4 + col1;
          int row2 = (i - row1 * 16) / 4;
          int col2 = (j - col1 * 16) / 4;
          int level2 = row2 * 4 + col2;
          int row3 = (i - row1 * 16 - row2 * 4);
          int col3 = (j - col1 * 16 - col2 * 4);
          int level3 = row3 * 4 + col3;
          int awidth = width / 63;
          int aheight = height / 63;
          XSetForeground(display, gc, 0x80000000);
          XFillRectangle(display, win, gc, awidth * i, aheight * j, awidth,
                         aheight);
          XSetForeground(display, gc, 0x80ffffff);
          char msg[4] = {candidates[level1], candidates[level2],
                         candidates[level3], '\0'};
          /*char msg[4] = {candidates[level1], '\0'};*/
          XDrawString(display, win, gc, awidth * i, aheight * j, msg,
                      strlen(msg));
        }
      }
      break;
    case KeyPress:
      keypress(event);
      break;
    default:
      break;
    }
  }

  // 结束销毁
  XDestroyWindow(display, win);
  XCloseDisplay(display);
  return 0;
}
