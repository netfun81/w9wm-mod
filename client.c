/* Copyright (c) 1994 David Hogan, 2000 Benjamin Drieu see README for licence details */
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include "dat.h"
#include "fns.h"

Client      *clients;
Client      *current = NULL;	/* paranoid */

void
setactive(c, on)
Client *c;
int on;
{
    if (on) {
        XUngrabButton(dpy, AnyButton, AnyModifier, c->parent);
	XSetInputFocus(dpy, c->window, RevertToPointerRoot, timestamp()); 

	if (use_keys)
	  {
	    XGrabKey(dpy, 
		     XKeysymToKeycode(dpy, XK_Tab),
		     Mod4Mask, 
		     root, False, GrabModeAsync, GrabModeAsync);
	    XGrabKey(dpy, 
		     XKeysymToKeycode(dpy, XK_Tab),
		     Mod4Mask|ShiftMask, 
		     root, False, GrabModeAsync, GrabModeAsync);
	  }

        if (c->proto & Ptakefocus)
            sendcmessage(c->window, wm_protocols, wm_take_focus);
        cmapfocus(c);
    }
    else
        XGrabButton(dpy, AnyButton, AnyModifier, c->parent, False,
            ButtonMask, GrabModeAsync, GrabModeSync, None, None);
    draw_border(c, on);
}

void
draw_border(c, active)
Client *c;
int active;
{
    XSetWindowBackground(dpy, c->parent, active ? black : white);
    XClearWindow(dpy, c->parent);
    if (c->hold && active)
        XDrawRectangle(dpy, c->parent, gc, 1, 1, c->dx+BORDER-1, c->dy+BORDER-1);
}

#ifdef  DEBUG
void
dump_revert()
{
    Client *c;
    int i;

    i = 0;
    for (c = current; c; c = c->revert) {
        fprintf(stderr, "%s(%x:%d)", c->label ? c->label : "?", c->window, c->state);
        if (i++ > 100)
            break;
        if (c->revert)
            fprintf(stderr, " -> ");
    }
    if (current == 0)
        fprintf(stderr, "empty");
    fprintf(stderr, "\n");
}

void
dump_clients()
{
    Client *c;

    for (c = clients; c; c = c->next)
        fprintf(stderr, "w 0x%x parent 0x%x @ (%d, %d)\n", c->window, c->parent, c->x, c->y);
}
#endif

void
active(c)
Client *c;
{
    Client *cc;

    if (c == 0) {
        fprintf(stderr, "9wm: active(c==0)\n");
        return;
    }
    if (c == current)
        return;
    if (current)
        setactive(current, 0);
    setactive(c, 1);
    for (cc = clients; cc; cc = cc->next)
        if (cc->revert == c)
            cc->revert = c->revert;
    c->revert = current;
    while (c->revert && !normal(c->revert))
        c->revert = c->revert->revert;
    current = c;
#ifdef  DEBUG
    if (debug)
        dump_revert();
#endif
}

void
nofocus()
{
    static Window w = 0;
    int mask;
    XSetWindowAttributes attr;
    Client *c;

    if (current) {
        setactive(current, 0);
        for (c = current->revert; c; c = c->revert)
            if (normal(c)) {
                active(c);
                return;
            }
        /* if no candidates to revert to, fall through */
    }
    current = 0;
    if (w == 0) {
        mask = CWOverrideRedirect;
        attr.override_redirect = 1;
        w = XCreateWindow(dpy, root, 0, 0, 1, 1, 0, CopyFromParent,
            InputOnly, CopyFromParent, mask, &attr);
        XMapWindow(dpy, w);
    }
    XSetInputFocus(dpy, w, RevertToPointerRoot, timestamp());
    cmapfocus(0);
}

Client *
getclient(w, create)
Window w;
int create;
{
    Client *c;

    if (w == 0 || w == root)
        return 0;

    for (c = clients; c; c = c->next)
        if (c->window == w || c->parent == w)
            return c;

    if (!create)
        return 0;

    c = (Client *)malloc(sizeof(Client));
    memset(c, 0, sizeof(Client));
    c->window = w;
    c->parent = root;
    c->reparenting = 0;
    c->state = WithdrawnState;
    c->init = 0;
    c->cmap = None;
    c->label = c->class = 0;
    c->revert = 0;
    c->is9term = 0;
    c->hold = 0;
    c->ncmapwins = 0;
    c->cmapwins = 0;
    c->wmcmaps = 0;
    c->next = clients;
    c->virtual = virtual;
    clients = c;
    return c;
}

void
rmclient(c)
Client *c;
{
    Client *cc;

    for (cc = current; cc && cc->revert; cc = cc->revert)
        if (cc->revert == c)
            cc->revert = cc->revert->revert;

    if (c == clients)
        clients = c->next;
    for (cc = clients; cc && cc->next; cc = cc->next)
        if (cc->next == c)
            cc->next = cc->next->next;

    if (hidden(c))
        unhidec(c, 0);

    if (c->parent != root)
        XDestroyWindow(dpy, c->parent);

    c->parent = c->window = None;       /* paranoia */
    if (current == c) {
        current = c->revert;
        if (current == 0)
            nofocus();
        else
            setactive(current, 1);
    }
    if (c->ncmapwins != 0) {
        XFree((char *)c->cmapwins);
        free((char *)c->wmcmaps);
    }
    if (c->iconname != 0)
        XFree((char*) c->iconname);
    if (c->name != 0)
        XFree((char*) c->name);
    if (c->instance != 0)
        XFree((char*) c->instance);
    if (c->class != 0)
        XFree((char*) c->class);
    memset(c, 0, sizeof(Client));       /* paranoia */
    free(c);
}
