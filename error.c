/* Copyright (c) 1994 David Hogan, see README for licence details */
#include <stdio.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include "dat.h"
#include "fns.h"

int     ignore_badwindow;

void
fatal(s)
char *s;
{
    fprintf(stderr, "9wm: ");
    perror(s);
    fprintf(stderr, "\n");
    exit(1);
}

int
handler(d, e)
Display *d;
XErrorEvent *e;
{
    char msg[80], req[80], number[80];

    if (initting && (e->request_code == X_ChangeWindowAttributes) && (e->error_code == BadAccess)) {
        fprintf(stderr, "9wm: it looks like there's already a window manager running;  9wm not started\n");
        exit(1);
    }

    if (ignore_badwindow && (e->error_code == BadWindow || e->error_code == BadColor))
        return 0;

    XGetErrorText(d, e->error_code, msg, sizeof(msg));
    sprintf(number, "%d", e->request_code);
    XGetErrorDatabaseText(d, "XRequest", number, "<unknown>", req, sizeof(req));

    fprintf(stderr, "9wm: %s(0x%x): %s\n", req, (unsigned int)e->resourceid, msg);

    if (initting) {
        fprintf(stderr, "9wm: failure during initialisation; aborting\n");
        exit(1);
    }
    return 0;
}

void
graberror(f, err)
char *f;
int err;
{
    char *s;

    switch (err) {
    case GrabNotViewable:
        s = "not viewable";
        break;
    case AlreadyGrabbed:
        s = "already grabbed";
        break;
    case GrabFrozen:
        s = "grab frozen";
        break;
    case GrabInvalidTime:
        s = "invalid time";
        break;
    case GrabSuccess:
        return;
    default:
        fprintf(stderr, "9wm: %s: grab error: %d\n", f, err);
        return;
    }
    fprintf(stderr, "9wm: %s: grab error: %s\n", f, s);
}

void
showhints(c, size)
Client *c;
XSizeHints *size;
{
#ifdef  DEBUG
    fprintf(stderr, "\nNew window: %s(%s) ", c->label ? c->label : "???", c->class ? c->class : "???");
    fprintf(stderr, "posn (%d,%d) size (%d,%d)\n", c->x, c->y, c->dx, c->dy);

    if (size == 0) {
        fprintf(stderr, "no size hints\n");
        return;
    }

    fprintf(stderr, "size hints: ");
    if (size->flags&USPosition)
        fprintf(stderr, "USPosition(%d,%d) ", size->x, size->y);
    if (size->flags&USSize)
        fprintf(stderr, "USSize(%d,%d) ", size->width, size->height);
    if (size->flags&PPosition)
        fprintf(stderr, "PPosition(%d,%d) ", size->x, size->y);
    if (size->flags&PSize)
        fprintf(stderr, "PSize(%d,%d) ", size->width, size->height);
    if (size->flags&PMinSize)
        fprintf(stderr, "PMinSize(%d,%d) ", size->min_width, size->min_height);
    if (size->flags&PMaxSize)
        fprintf(stderr, "PMaxSize(%d,%d) ", size->max_width, size->max_height);
    if (size->flags&PResizeInc)
        fprintf(stderr, "PResizeInc ");
    if (size->flags&PAspect)
        fprintf(stderr, "PAspect ");
    if (size->flags&PBaseSize)
        fprintf(stderr, "PBaseSize ");
    if (size->flags&PWinGravity)
        fprintf(stderr, "PWinGravity ");
    fprintf(stderr, "\n");
#endif
}

#ifdef  DEBUG_EV
#include "showevent/ShowEvent.c"
#endif
