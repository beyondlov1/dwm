/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <Imlib2.h>
#include <errno.h>
#include <time.h>

#include "drw.h"
#include "util.h"

#define UTF_INVALID 0xFFFD
#define UTF_SIZ     4

static const unsigned char utfbyte[UTF_SIZ + 1] = {0x80,    0, 0xC0, 0xE0, 0xF0};
static const unsigned char utfmask[UTF_SIZ + 1] = {0xC0, 0x80, 0xE0, 0xF0, 0xF8};
static const long utfmin[UTF_SIZ + 1] = {       0,    0,  0x80,  0x800,  0x10000};
static const long utfmax[UTF_SIZ + 1] = {0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF};

static long
utf8decodebyte(const char c, size_t *i)
{
	for (*i = 0; *i < (UTF_SIZ + 1); ++(*i))
		if (((unsigned char)c & utfmask[*i]) == utfbyte[*i])
			return (unsigned char)c & ~utfmask[*i];
	return 0;
}

static size_t
utf8validate(long *u, size_t i)
{
	if (!BETWEEN(*u, utfmin[i], utfmax[i]) || BETWEEN(*u, 0xD800, 0xDFFF))
		*u = UTF_INVALID;
	for (i = 1; *u > utfmax[i]; ++i)
		;
	return i;
}

static size_t
utf8decode(const char *c, long *u, size_t clen)
{
	size_t i, j, len, type;
	long udecoded;

	*u = UTF_INVALID;
	if (!clen)
		return 0;
	udecoded = utf8decodebyte(c[0], &len);
	if (!BETWEEN(len, 1, UTF_SIZ))
		return 1;
	for (i = 1, j = 1; i < clen && j < len; ++i, ++j) {
		udecoded = (udecoded << 6) | utf8decodebyte(c[i], &type);
		if (type)
			return j;
	}
	if (j < len)
		return 0;
	*u = udecoded;
	utf8validate(u, len);

	return len;
}

Drw *
drw_create(Display *dpy, int screen, Window root, unsigned int w, unsigned int h)
{
	Drw *drw = ecalloc(1, sizeof(Drw));

	drw->dpy = dpy;
	drw->screen = screen;
	drw->root = root;
	drw->w = w;
	drw->h = h;
	drw->drawable = XCreatePixmap(dpy, root, w, h, DefaultDepth(dpy, screen));
	drw->picture = XRenderCreatePicture(dpy, drw->drawable, XRenderFindVisualFormat(dpy, DefaultVisual(dpy, screen)), 0, NULL);
	drw->gc = XCreateGC(dpy, root, 0, NULL);
	XSetLineAttributes(dpy, drw->gc, 1, LineSolid, CapButt, JoinMiter);

	return drw;
}

void
drw_resize(Drw *drw, unsigned int w, unsigned int h)
{
	if (!drw)
		return;

	drw->w = w;
	drw->h = h;
	if (drw->picture)
		XRenderFreePicture(drw->dpy, drw->picture);
	if (drw->drawable)
		XFreePixmap(drw->dpy, drw->drawable);
	drw->drawable = XCreatePixmap(drw->dpy, drw->root, w, h, DefaultDepth(drw->dpy, drw->screen));
}

void
drw_free(Drw *drw)
{
	XRenderFreePicture(drw->dpy, drw->picture);
	XFreePixmap(drw->dpy, drw->drawable);
	XFreeGC(drw->dpy, drw->gc);
	drw_fontset_free(drw->fonts);
	free(drw);
}

Picture
drw_picture_create_resized(Drw *drw, char *src, unsigned int srcw, unsigned int srch, unsigned int dstw, unsigned int dsth) {
	Pixmap pm;
	Picture pic;
	GC gc;

	if (srcw <= (dstw << 1u) && srch <= (dsth << 1u)) {
		XImage img = {
			srcw, srch, 0, ZPixmap, src,
			ImageByteOrder(drw->dpy), BitmapUnit(drw->dpy), BitmapBitOrder(drw->dpy), 32,
			32, 0, 32,
			0, 0, 0
		};
		XInitImage(&img);

		pm = XCreatePixmap(drw->dpy, drw->root, srcw, srch, 32);
		gc = XCreateGC(drw->dpy, pm, 0, NULL);
		XPutImage(drw->dpy, pm, gc, &img, 0, 0, 0, 0, srcw, srch);
		XFreeGC(drw->dpy, gc);

		pic = XRenderCreatePicture(drw->dpy, pm, XRenderFindStandardFormat(drw->dpy, PictStandardARGB32), 0, NULL);
		XFreePixmap(drw->dpy, pm);

		XRenderSetPictureFilter(drw->dpy, pic, FilterBilinear, NULL, 0);
		XTransform xf;
		xf.matrix[0][0] = (srcw << 16u) / dstw; xf.matrix[0][1] = 0; xf.matrix[0][2] = 0;
		xf.matrix[1][0] = 0; xf.matrix[1][1] = (srch << 16u) / dsth; xf.matrix[1][2] = 0;
		xf.matrix[2][0] = 0; xf.matrix[2][1] = 0; xf.matrix[2][2] = 65536;
		XRenderSetPictureTransform(drw->dpy, pic, &xf);
	} else {
		Imlib_Image origin = imlib_create_image_using_data(srcw, srch, (DATA32 *)src);
		if (!origin) return None;
		imlib_context_set_image(origin);
		imlib_image_set_has_alpha(1);
		Imlib_Image scaled = imlib_create_cropped_scaled_image(0, 0, srcw, srch, dstw, dsth);
		imlib_free_image_and_decache();
		if (!scaled) return None;
		imlib_context_set_image(scaled);
		imlib_image_set_has_alpha(1);

		XImage img = {
		    dstw, dsth, 0, ZPixmap, (char *)imlib_image_get_data_for_reading_only(),
		    ImageByteOrder(drw->dpy), BitmapUnit(drw->dpy), BitmapBitOrder(drw->dpy), 32,
		    32, 0, 32,
		    0, 0, 0
		};
		XInitImage(&img);

		pm = XCreatePixmap(drw->dpy, drw->root, dstw, dsth, 32);
		gc = XCreateGC(drw->dpy, pm, 0, NULL);
		XPutImage(drw->dpy, pm, gc, &img, 0, 0, 0, 0, dstw, dsth);
		imlib_free_image_and_decache();
		XFreeGC(drw->dpy, gc);

		pic = XRenderCreatePicture(drw->dpy, pm, XRenderFindStandardFormat(drw->dpy, PictStandardARGB32), 0, NULL);
		XFreePixmap(drw->dpy, pm);
	}

	return pic;
}



Picture
drw_picture_load_resized(Drw *drw, char *file, unsigned int srcw, unsigned int srch, unsigned int dstw, unsigned int dsth) {
	Pixmap pm;
	Picture pic;
	GC gc;

	Imlib_Image origin = imlib_load_image(file);
	if (!origin) return None;
	imlib_context_set_image(origin);
	imlib_image_set_has_alpha(1);
	Imlib_Image scaled = imlib_create_cropped_scaled_image(0, 0, srcw, srch, dstw, dsth);
	imlib_free_image_and_decache();
	if (!scaled) return None;
	imlib_context_set_image(scaled);
	imlib_image_set_has_alpha(1);

	XImage img = {
		dstw, dsth, 0, ZPixmap, (char *)imlib_image_get_data_for_reading_only(),
		ImageByteOrder(drw->dpy), BitmapUnit(drw->dpy), BitmapBitOrder(drw->dpy), 32,
		32, 0, 32,
		0, 0, 0
	};
	XInitImage(&img);

	pm = XCreatePixmap(drw->dpy, drw->root, dstw, dsth, 32);
	gc = XCreateGC(drw->dpy, pm, 0, NULL);
	XPutImage(drw->dpy, pm, gc, &img, 0, 0, 0, 0, dstw, dsth);
	imlib_free_image_and_decache();
	XFreeGC(drw->dpy, gc);

	pic = XRenderCreatePicture(drw->dpy, pm, XRenderFindStandardFormat(drw->dpy, PictStandardARGB32), 0, NULL);
	XFreePixmap(drw->dpy, pm);
	
	return pic;
}

void
drw_pic(Drw *drw, int x, int y, unsigned int w, unsigned int h, Picture pic)
{
	if (!drw)
		return;
	XRenderComposite(drw->dpy, PictOpOver, pic, None, drw->picture, 0, 0, 0, 0, x, y, w, h);
}

/* This function is an implementation detail. Library users should use
 * drw_fontset_create instead.
 */
static Fnt *
xfont_create(Drw *drw, const char *fontname, FcPattern *fontpattern)
{
	Fnt *font;
	XftFont *xfont = NULL;
	FcPattern *pattern = NULL;

	if (fontname) {
		/* Using the pattern found at font->xfont->pattern does not yield the
		 * same substitution results as using the pattern returned by
		 * FcNameParse; using the latter results in the desired fallback
		 * behaviour whereas the former just results in missing-character
		 * rectangles being drawn, at least with some fonts. */
		if (!(xfont = XftFontOpenName(drw->dpy, drw->screen, fontname))) {
			fprintf(stderr, "error, cannot load font from name: '%s'\n", fontname);
			return NULL;
		}
		if (!(pattern = FcNameParse((FcChar8 *) fontname))) {
			fprintf(stderr, "error, cannot parse font name to pattern: '%s'\n", fontname);
			XftFontClose(drw->dpy, xfont);
			return NULL;
		}
	} else if (fontpattern) {
		if (!(xfont = XftFontOpenPattern(drw->dpy, fontpattern))) {
			fprintf(stderr, "error, cannot load font from pattern.\n");
			return NULL;
		}
	} else {
		die("no font specified.");
	}

	font = ecalloc(1, sizeof(Fnt));
	font->xfont = xfont;
	font->pattern = pattern;
	font->h = xfont->ascent + xfont->descent;
	font->dpy = drw->dpy;

	return font;
}

static void
xfont_free(Fnt *font)
{
	if (!font)
		return;
	if (font->pattern)
		FcPatternDestroy(font->pattern);
	XftFontClose(font->dpy, font->xfont);
	free(font);
}

Fnt*
drw_fontset_create(Drw* drw, const char *fonts[], size_t fontcount)
{
	Fnt *cur, *ret = NULL;
	size_t i;

	if (!drw || !fonts)
		return NULL;

	for (i = 1; i <= fontcount; i++) {
		if ((cur = xfont_create(drw, fonts[fontcount - i], NULL))) {
			cur->next = ret;
			ret = cur;
		}
	}
	return (drw->fonts = ret);
}

void
drw_fontset_free(Fnt *font)
{
	if (font) {
		drw_fontset_free(font->next);
		xfont_free(font);
	}
}

void
drw_clr_create(Drw *drw, Clr *dest, const char *clrname)
{
	if (!drw || !dest || !clrname)
		return;

	if (!XftColorAllocName(drw->dpy, DefaultVisual(drw->dpy, drw->screen),
	                       DefaultColormap(drw->dpy, drw->screen),
	                       clrname, dest))
		die("error, cannot allocate color '%s'", clrname);
}

/* Wrapper to create color schemes. The caller has to call free(3) on the
 * returned color scheme when done using it. */
Clr *
drw_scm_create(Drw *drw, const char *clrnames[], size_t clrcount)
{
	size_t i;
	Clr *ret;

	/* need at least two colors for a scheme */
	if (!drw || !clrnames || clrcount < 2 || !(ret = ecalloc(clrcount, sizeof(XftColor))))
		return NULL;

	for (i = 0; i < clrcount; i++)
		drw_clr_create(drw, &ret[i], clrnames[i]);
	return ret;
}

void
drw_setfontset(Drw *drw, Fnt *set)
{
	if (drw)
		drw->fonts = set;
}

void
drw_setscheme(Drw *drw, Clr *scm)
{
	if (drw)
		drw->scheme = scm;
}

void
drw_rect(Drw *drw, int x, int y, unsigned int w, unsigned int h, int filled, int invert)
{
	if (!drw || !drw->scheme)
		return;
	XSetForeground(drw->dpy, drw->gc, invert ? drw->scheme[ColBg].pixel : drw->scheme[ColFg].pixel);
	if (filled)
		XFillRectangle(drw->dpy, drw->drawable, drw->gc, x, y, w, h);
	else
		XDrawRectangle(drw->dpy, drw->drawable, drw->gc, x, y, w - 1, h - 1);
}

void
drw_line(Drw *drw, int x1, int y1, int x2, int y2, int invert)
{
	if (!drw || !drw->scheme)
		return;
	XSetForeground(drw->dpy, drw->gc, invert ? drw->scheme[ColBg].pixel : drw->scheme[ColFg].pixel);
	XDrawLine(drw->dpy, drw->drawable,drw->gc, x1, y1, x2, y2);
}

int
drw_text(Drw *drw, int x, int y, unsigned int w, unsigned int h, unsigned int lpad, const char *text, int invert)
{
	char buf[1024];
	int ty;
	unsigned int ew;
	XftDraw *d = NULL;
	Fnt *usedfont, *curfont, *nextfont;
	size_t i, len;
	int utf8strlen, utf8charlen, render = x || y || w || h;
	long utf8codepoint = 0;
	const char *utf8str;
	FcCharSet *fccharset;
	FcPattern *fcpattern;
	FcPattern *match;
	XftResult result;
	int charexists = 0;

	if (!drw || (render && !drw->scheme) || !text || !drw->fonts)
		return 0;

	if (!render) {
		w = ~w;
	} else {
		XSetForeground(drw->dpy, drw->gc, drw->scheme[invert ? ColFg : ColBg].pixel);
		XFillRectangle(drw->dpy, drw->drawable, drw->gc, x, y, w, h);
		d = XftDrawCreate(drw->dpy, drw->drawable,
		                  DefaultVisual(drw->dpy, drw->screen),
		                  DefaultColormap(drw->dpy, drw->screen));
		x += lpad;
		w -= lpad;
	}

	usedfont = drw->fonts;
	while (1) {
		utf8strlen = 0;
		utf8str = text;
		nextfont = NULL;
		while (*text) {
			utf8charlen = utf8decode(text, &utf8codepoint, UTF_SIZ);
			for (curfont = drw->fonts; curfont; curfont = curfont->next) {
				charexists = charexists || XftCharExists(drw->dpy, curfont->xfont, utf8codepoint);
				if (charexists) {
					if (curfont == usedfont) {
						utf8strlen += utf8charlen;
						text += utf8charlen;
					} else {
						nextfont = curfont;
					}
					break;
				}
			}

			if (!charexists || nextfont)
				break;
			else
				charexists = 0;
		}

		if (utf8strlen) {
			drw_font_getexts(usedfont, utf8str, utf8strlen, &ew, NULL);
			/* shorten text if necessary */
			for (len = MIN(utf8strlen, sizeof(buf) - 1); len && ew > w; len--)
				drw_font_getexts(usedfont, utf8str, len, &ew, NULL);

			if (len) {
				memcpy(buf, utf8str, len);
				buf[len] = '\0';
				if (len < utf8strlen)
					for (i = len; i && i > len - 3; buf[--i] = '.')
						; /* NOP */

				if (render) {
					ty = y + (h - usedfont->h) / 2 + usedfont->xfont->ascent;
					XftDrawStringUtf8(d, &drw->scheme[invert ? ColBg : ColFg],
					                  usedfont->xfont, x, ty, (XftChar8 *)buf, len);
				}
				x += ew;
				w -= ew;
			}
		}

		if (!*text) {
			break;
		} else if (nextfont) {
			charexists = 0;
			usedfont = nextfont;
		} else {
			/* Regardless of whether or not a fallback font is found, the
			 * character must be drawn. */
			charexists = 1;

			fccharset = FcCharSetCreate();
			FcCharSetAddChar(fccharset, utf8codepoint);

			if (!drw->fonts->pattern) {
				/* Refer to the comment in xfont_create for more information. */
				die("the first font in the cache must be loaded from a font string.");
			}

			fcpattern = FcPatternDuplicate(drw->fonts->pattern);
			FcPatternAddCharSet(fcpattern, FC_CHARSET, fccharset);
			FcPatternAddBool(fcpattern, FC_SCALABLE, FcTrue);
			FcPatternAddBool(fcpattern, FC_COLOR, FcFalse);

			FcConfigSubstitute(NULL, fcpattern, FcMatchPattern);
			FcDefaultSubstitute(fcpattern);
			match = XftFontMatch(drw->dpy, drw->screen, fcpattern, &result);

			FcCharSetDestroy(fccharset);
			FcPatternDestroy(fcpattern);

			if (match) {
				usedfont = xfont_create(drw, NULL, match);
				if (usedfont && XftCharExists(drw->dpy, usedfont->xfont, utf8codepoint)) {
					for (curfont = drw->fonts; curfont->next; curfont = curfont->next)
						; /* NOP */
					curfont->next = usedfont;
				} else {
					xfont_free(usedfont);
					usedfont = drw->fonts;
				}
			}
		}
	}
	if (d)
		XftDrawDestroy(d);

	return x + (render ? w : 0);
}

void
drw_map(Drw *drw, Window win, int x, int y, unsigned int w, unsigned int h)
{
	if (!drw)
		return;

	XCopyArea(drw->dpy, drw->drawable, win, drw->gc, x, y, w, h, x, y);
	XSync(drw->dpy, False);
}

unsigned int
drw_fontset_getwidth(Drw *drw, const char *text)
{
	if (!drw || !drw->fonts || !text)
		return 0;
	return drw_text(drw, 0, 0, 0, 0, 0, text, 0);
}

void
drw_font_getexts(Fnt *font, const char *text, unsigned int len, unsigned int *w, unsigned int *h)
{
	XGlyphInfo ext;

	if (!font || !text)
		return;

	XftTextExtentsUtf8(font->dpy, font->xfont, (XftChar8 *)text, len, &ext);
	if (w)
		*w = ext.xOff;
	if (h)
		*h = font->h;
}

Cur *
drw_cur_create(Drw *drw, int shape)
{
	Cur *cur;

	if (!drw || !(cur = ecalloc(1, sizeof(Cur))))
		return NULL;

	cur->cursor = XCreateFontCursor(drw->dpy, shape);

	return cur;
}

void
drw_cur_free(Drw *drw, Cur *cursor)
{
	if (!cursor)
		return;

	XFreeCursor(drw->dpy, cursor->cursor);
	free(cursor);
}



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
    *rw = attr.width;
    *rh = attr.height;
    XTranslateCoordinates(dpy, target, root, 0, 0, rx, ry, &child);
    return 1;
}

Imlib_Image scrotGrabRect(int x, int y, int w, int h)
{
    Imlib_Image im = imlib_create_image_from_drawable(0, x, y, w, h, 1);
    if (!im)
        errx(EXIT_FAILURE, "failed to grab image");
    return im;
}

Imlib_Image scrotGrabWindowById(Display *dpy, Window root, Screen *scr, Window window)
{
    Imlib_Image im = NULL;
    int rx = 0, ry = 0, rw = 0, rh = 0;

    if (!scrotGetGeometry(dpy, root, window, &rx, &ry, &rw, &rh))
        return NULL;
    scrotNiceClip(dpy, root, scr, &rx, &ry, &rw, &rh);
    im = scrotGrabRect(rx, ry, rw, rh);
    return im;
}


Picture
drw_picture_image_resized(Drw *drw, Imlib_Image image, unsigned int srcw, unsigned int srch, unsigned int dstw, unsigned int dsth) {
	Pixmap pm;
	Picture pic;
	GC gc;

	Imlib_Image origin = image;
	if (!origin) return None;
	imlib_context_set_image(origin);
	imlib_image_set_has_alpha(1);
	Imlib_Image scaled = imlib_create_cropped_scaled_image(0, 0, srcw, srch, dstw, dsth);
	imlib_free_image_and_decache();
	if (!scaled) return None;
	imlib_context_set_image(scaled);
	imlib_image_set_has_alpha(1);

	XImage img = {
		dstw, dsth, 0, ZPixmap, (char *)imlib_image_get_data_for_reading_only(),
		ImageByteOrder(drw->dpy), BitmapUnit(drw->dpy), BitmapBitOrder(drw->dpy), 32,
		32, 0, 32,
		0, 0, 0
	};
	XInitImage(&img);

	pm = XCreatePixmap(drw->dpy, drw->root, dstw, dsth, 32);
	gc = XCreateGC(drw->dpy, pm, 0, NULL);
	XPutImage(drw->dpy, pm, gc, &img, 0, 0, 0, 0, dstw, dsth);
	imlib_free_image_and_decache();
	XFreeGC(drw->dpy, gc);

	pic = XRenderCreatePicture(drw->dpy, pm, XRenderFindStandardFormat(drw->dpy, PictStandardARGB32), 0, NULL);
	XFreePixmap(drw->dpy, pm);
	
	return pic;
}

void 
initimlib(Display *disp, Window root, Screen *scr){
	Visual *vis = DefaultVisual(disp, XScreenNumberOfScreen(scr));
	Colormap cm = DefaultColormap(disp, XScreenNumberOfScreen(scr));

	imlib_context_set_drawable(root);
	imlib_context_set_display(disp);
	imlib_context_set_visual(vis);
	imlib_context_set_colormap(cm);
	imlib_context_set_color_modifier(NULL);
	imlib_context_set_operation(IMLIB_OP_COPY);
	imlib_set_cache_size(0);
   
}