/* include X11 stuff */
#include <X11/Xlib.h>
/* include Imlib2 stuff */
#include <Imlib2.h>
/* sprintf include */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
// #include <X11/Xft/Xft.h>
#include <Imlib2.h>
#include <errno.h>
#include <time.h>

/* some globals for our window & X display */
Display *disp;
Window   win;
Visual  *vis;
Colormap cm;
int      depth;


// gcc testscrot.c -o testscrot -lXrender -lImlib2 -lX11 -lm -lXft -lfontconfig -DXINERAMA -I/usr/include/X11 -L/usr/lib/X11 -I/usr/include/freetype2  && ./testscrot


/* Clip rectangle nicely */
void scrotNiceClip(Display *dpy,  Window root, Screen *scr,  int *rx, int *ry, int *rw, int *rh)
{
    if (*rx < 0) {
        *rw += *rx;
        *rx = 0;
    }
    if (*ry < 0) {
        *rh += *ry;
        *ry = 0;
    }
    if ((*rx + *rw) > scr->width)
        *rw = scr->width - *rx;
    if ((*ry + *rh) > scr->height)
        *rh = scr->height - *ry;
}

static int findWindowManagerFrame(Display *dpy, Window root, Window *const target, int *const frames)
{
    int x, status;
    unsigned int d;
    Window rt, *children, parent;

    status = XGetGeometry(dpy, *target, &root, &x, &x, &d, &d, &d, &d);

    if (!status)
        return 0;

    for (;;) {
        status = XQueryTree(dpy, *target, &rt, &parent, &children, &d);
        if (status && (children != None))
            XFree(children);
        if (!status || (parent == None) || (parent == rt))
            break;
        *target = parent;
        ++*frames;
    }
    return 1;
}


static Window scrotFindWindowByProperty(Display *display, const Window window,
    const Atom property)
{
    Atom type = None;
    int format, status;
    unsigned char *data = NULL;
    unsigned int i, numberChildren;
    unsigned long after, numberItems;
    Window child = None, *children, parent, rootReturn;

    status = XQueryTree(display, window, &rootReturn, &parent, &children,
        &numberChildren);
    if (!status)
        return None;
    for (i = 0; (i < numberChildren) && (child == None); i++) {
        status = XGetWindowProperty(display, children[i], property, 0L, 0L, False,
            AnyPropertyType, &type, &format,
            &numberItems, &after, &data);
        XFree(data);
        if ((status == Success) && type)
            child = children[i];
    }
    for (i = 0; (i < numberChildren) && (child == None); i++)
        child = scrotFindWindowByProperty(display, children[i], property);
    if (children != None)
        XFree(children);
    return (child);
}


static Window scrotGetClientWindow(Display *display, Window target)
{
    Atom state;
    Atom type = None;
    int format, status;
    unsigned char *data = NULL;
    unsigned long after, items;
    Window client;

    state = XInternAtom(display, "WM_STATE", True);
    if (state == None)
        return target;
    status = XGetWindowProperty(display, target, state, 0L, 0L, False,
        AnyPropertyType, &type, &format, &items, &after,
        &data);
    XFree(data);
    if ((status == Success) && (type != None))
        return target;
    client = scrotFindWindowByProperty(display, target, state);
    if (!client)
        return target;
    return client;
}


/* clockNow() has the exact same semantics as CLOCK_MONOTONIC. Except that on
 * Linux, CLOCK_MONOTONIC does not progress while the system is suspended, so
 * the non-standard CLOCK_BOOTTIME is used instead to avoid this bug.
 */
struct timespec clockNow(void)
{
    struct timespec ret;
#if defined(__linux__)
    clock_gettime(CLOCK_BOOTTIME, &ret);
#else
    clock_gettime(CLOCK_MONOTONIC, &ret);
#endif
    return ret;
}

static long miliToNanoSec(int ms)
{
    return ms * 1000L * 1000L;
}

/* OpenBSD and OS X lack clock_nanosleep(), so we call nanosleep() and use a
 * trivial algorithm to correct for drift. The end timespec is returned for
 * callers that want it. EINTR is also dealt with.
 */
struct timespec scrotSleepFor(struct timespec start, int ms)
{
    struct timespec end = {
        .tv_sec  = start.tv_sec  + (ms / 1000),
        .tv_nsec = start.tv_nsec + miliToNanoSec(ms % 1000),
    };
    if (end.tv_nsec >= miliToNanoSec(1000)) {
        ++end.tv_sec;
        end.tv_nsec -= miliToNanoSec(1000);
    }

    struct timespec tmp;
    do {
        tmp = clockNow();

        /* XXX: Use timespecsub(). OS X doesn't have that BSD macro, and libbsd
         * doesn't support OS X save for an unmaintained fork. libobsd supports
         * OS X but doesn't have the macro yet.
         */
        tmp.tv_sec  = end.tv_sec  - tmp.tv_sec;
        tmp.tv_nsec = end.tv_nsec - tmp.tv_nsec;
        if (tmp.tv_nsec < 0) {
            --tmp.tv_sec;
            tmp.tv_nsec += miliToNanoSec(1000);
        }
    } while (nanosleep(&tmp, NULL) < 0 && errno == EINTR);

    return end;
}


/* Get geometry of window and use that */
int scrotGetGeometry(Display *dpy, Window root, Window target, int *rx, int *ry, int *rw, int *rh)
{
    Window child;
    XWindowAttributes attr;
    int stat, frames = 0;
    

    /* Get windowmanager frame of window */
    if (target != root) {
        if (findWindowManagerFrame(dpy, root, &target, &frames)) {
            /* Get client window. */
            target = scrotGetClientWindow(dpy, target);
            XRaiseWindow(dpy, target);
            XSync(dpy, False);

			/* HACK: there doesn't seem to be any way to figure out whether the
             * raise request was accepted or rejected. so just sleep a bit to
             * give the WM some time to update. */
            scrotSleepFor(clockNow(), 160);
        }
    }
    stat = XGetWindowAttributes(dpy, target, &attr);
    if (!stat || (attr.map_state != IsViewable))
        return 0;

   printf("%d", attr.width);
   printf("%d",attr.height);
      *rw = attr.width;
      *rh = attr.height;
    
    XTranslateCoordinates(dpy, target, root, 0, 0, rx, ry, &child);
    imlib_context_set_drawable(win);
    printf("%d",2222);
    return 1;
}

Imlib_Image scrotGrabRect(int x, int y, int w, int h)
{
   printf("%d,%d,%d,%d,", x, y, w, h);
   Imlib_Image im = imlib_create_image_from_drawable(0, x, y, w, h, 1);
   printf("%d",2222);
   if (!im)
      errx(EXIT_FAILURE, "failed to grab image");
    return im;
}

Imlib_Image scrotGrabWindowById(Display *dpy, Window root, Screen *scr, Window window)
{
    Imlib_Image im = NULL;
    int rx = 0, ry = 0, rw = 0, rh = 0;

//   printf("b");
//    fflush(stdout);


    if (!scrotGetGeometry(dpy, root, window, &rx, &ry, &rw, &rh))
        return NULL;
    
    scrotNiceClip(dpy, root, scr, &rx, &ry, &rw, &rh);
    im = scrotGrabRect(rx, ry, rw, rh);
    return im;
}


/* the program... */
int main(int argc, char **argv)
{
   setvbuf(stdout, NULL, _IONBF, 0);

   /* events we get from X */
   XEvent ev;
   /* areas to update */
   Imlib_Updates updates, current_update;
   /* our virtual framebuffer image we draw into */
   Imlib_Image buffer;
   /* a font */
   Imlib_Font font;
   /* our color range */
   Imlib_Color_Range range;
   /* our mouse x, y coordinates */
   int mouse_x = 0, mouse_y = 0;
   
   /* connect to X */
   disp  = XOpenDisplay(NULL);
      int screen = DefaultScreen(disp);
	Window root = RootWindow(disp, screen);
   Screen *scr = ScreenOfDisplay(disp, screen);
   /* get default visual , colormap etc. you could ask imlib2 for what it */
   /* thinks is the best, but this example is intended to be simple */
   vis   = DefaultVisual(disp, DefaultScreen(disp));
   depth = DefaultDepth(disp, DefaultScreen(disp));
   cm    = DefaultColormap(disp, DefaultScreen(disp));
   /* create a window 640x480 */
   win = XCreateSimpleWindow(disp, root, 
                             0, 0, 640, 480, 0, 0, 0);

 
  printf("a");
   fflush(stdout);

   XMapWindow(disp, win);

   Visual *vis = DefaultVisual(disp, XScreenNumberOfScreen(scr));
   Colormap cm = DefaultColormap(disp, XScreenNumberOfScreen(scr));

   imlib_context_set_drawable(root);
   imlib_context_set_display(disp);
   imlib_context_set_visual(vis);
   imlib_context_set_colormap(cm);
   imlib_context_set_color_modifier(NULL);
   imlib_context_set_operation(IMLIB_OP_COPY);
   imlib_set_cache_size(0);
   

   Imlib_Image image = scrotGrabWindowById(disp, root, scr, win);
	// drw_picture_image_resized(image, 640,480, 32,32);

   return 0;
}