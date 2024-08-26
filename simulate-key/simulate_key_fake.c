// sudo pacman -S libxtst


#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <unistd.h>
#include <stdio.h>

int main() {
    Display *display = XOpenDisplay(NULL);
    if (display == NULL) {
        return 1; // 无法打开显示
    }

    // 获取组合键的键码
    KeyCode ctrlKey = XKeysymToKeycode(display, XK_Control_L); // 左 Ctrl
    KeyCode altKey = XKeysymToKeycode(display, XK_Alt_L);     // 左 Alt
    KeyCode tKey = XKeysymToKeycode(display, XK_T);           // 'T' 键
    printf('%d', tKey);

    // 模拟按下 Ctrl + Alt + T
    // XTestFakeKeyEvent(display, ctrlKey, True, 0); // 按下 Ctrl
    // XTestFakeKeyEvent(display, altKey, True, 0);   // 按下 Alt
    XTestFakeKeyEvent(display, tKey, True, 0);     // 按下 T
    XFlush(display);
    usleep(100000); // 等待 100 毫秒

    // 模拟释放 T 键
    XTestFakeKeyEvent(display, tKey, False, 0);
    // 模拟释放 Alt 键
    // XTestFakeKeyEvent(display, altKey, False, 0);
    // 模拟释放 Ctrl 键
    // XTestFakeKeyEvent(display, ctrlKey, False, 0);
    XFlush(display);

    XCloseDisplay(display);
    return 0;
}

