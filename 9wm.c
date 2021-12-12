#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <X11/X.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include "dat.h"
#include "fns.h"
#include "patchlevel.h"
#include <X11/keysym.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>

char    *version[] = {
    "9wm version 1.1, Copyright (c) 1994 David Hogan",
    "w9wm version 0.4.2, Copyright (c) 2000-2003 Benjamin Drieu", 0,
};

Display         *dpy;
int             screen;
Window          root;
Window          menuwin;
Colormap        def_cmap;
int             initting;
GC              gc;
unsigned long   black;
unsigned long   white;
XFontStruct     *font;
int             nostalgia;
char            **myargv;
char            *termprog;
char            *display = NULL;
char            *shell;
Bool            shape;
int             shape_event;
int             _border = 4;
int             min_cmaps;
int             curtime;
int             debug;
int             signalled;
Bool            click_passes = 0;
Bool            use_keys = 1;
int             numvirtuals = 4;
char            *progsnames[16];
FILE            *barhandle;

Atom        exit_w9wm;
Atom        restart_w9wm;
Atom        wm_state;
Atom        wm_change_state;
Atom        wm_protocols;
Atom        wm_delete;
Atom        wm_take_focus;
Atom        wm_colormaps;
Atom        _w9wm_running;
Atom        _w9wm_hold_mode;

void    usage(), sighandler(), getevent();

char    *fontlist[] = {
    "lucm.latin1.9",
    "blit",
    "9x15bold",
    "lucidasanstypewriter-12",
    "fixed",
    0,
};

unsigned long int getKeysym(XEvent *ev) {
    return XLookupKeysym(&(ev->xkey), 0);
}

int isdigitkey(unsigned int x) {
    switch(x) {
    case XK_1:
    case XK_2:
    case XK_3:
    case XK_4:
    case XK_5:
    case XK_6:
    case XK_7:
    case XK_8:
    case XK_9:
        return (int)(x - XK_0);
    default:
        return 0;
    }
}

void xmobar() {
    barhandle = popen("xmobar", "w");
}

int
main(argc, argv)
int argc;
char    *argv[];
{
    int i, background, do_exit, do_restart, dummy;
    unsigned long mask;
    XEvent ev;
    XGCValues gv;
    XSetWindowAttributes attr;
    char *fname;

    myargv = argv;          /* for restart */
    
    background = do_exit = do_restart = 0;
    font = 0;
    fname = 0;
    for (i = 1; i < argc; i++)
        if (strcmp(argv[i], "-nostalgia") == 0)
            nostalgia++;
        else if (strcmp(argv[i], "-debug") == 0)
            debug++;
        else if ((strcmp(argv[i], "-display") == 0 || strcmp(argv[i], "-dpy") == 0 )
                 && i+1<argc) {
            display = argv[++i];
            setenv("DISPLAY", display, 1);
        } else if (strcmp(argv[i], "-pass") == 0) {
            click_passes = 1;
        } else if (strcmp(argv[i], "-nokeys") == 0) {
            use_keys = 0;
        } else if (strcmp(argv[i], "-font") == 0 && i+1<argc) {
            i++;
            fname = argv[i];
        } else if (strcmp(argv[i], "-grey") == 0)
            background = 1;
        else if (strcmp(argv[i], "-term") == 0 && i+1<argc)
            termprog = argv[++i];
        else if (strcmp(argv[i], "-version") == 0) {
            fprintf(stderr, "%s", version[1]);
            if (PATCHLEVEL > 0)
                fprintf(stderr, "; patch level %d", PATCHLEVEL);
            putc('\n', stderr);
            exit(0);
        } else if (strcmp(argv[i], "-virtuals") == 0 && i+1<argc) {
            int n = atoi(argv[++i]);
            if (n > 0 && n <= 12)
                numvirtuals = n;
            else
                fprintf(stderr, "w9wm: wrong number of virtual screens, must be between 1 and 12\n");
        } else if (argv[i][0] == '-')
            usage();
        else
            break;
    for (; i < argc; i++)
        if (strcmp(argv[i], "exit") == 0)
            do_exit++;
        else if (strcmp(argv[i], "restart") == 0)
            do_restart++;
        else
            usage();

    if (do_exit && do_restart)
        usage();

    shell = (char *)getenv("SHELL");
    if (shell == NULL)
        shell = DEFSHELL;

    parseprogsfile();

    dpy = XOpenDisplay(display);
    if (dpy == 0)
        fatal("can't open display");
    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);
    def_cmap = DefaultColormap(dpy, screen);
    min_cmaps = MinCmapsOfScreen(ScreenOfDisplay(dpy, screen));

    initting = 1;
    XSetErrorHandler(handler);
    if (signal(SIGTERM, sighandler) == SIG_IGN)
        signal(SIGTERM, SIG_IGN);
    if (signal(SIGINT, sighandler) == SIG_IGN)
        signal(SIGINT, SIG_IGN);
    if (signal(SIGHUP, sighandler) == SIG_IGN)
        signal(SIGHUP, SIG_IGN);

    exit_w9wm = XInternAtom(dpy, "w9wm_EXIT", False);
    restart_w9wm = XInternAtom(dpy, "w9wm_RESTART", False);

    curtime = -1;       /* don't care */
    if (do_exit) {
        sendcmessage(root, exit_w9wm, 0L);
        XSync(dpy, False);
        exit(0);
    }
    if (do_restart) {
        sendcmessage(root, restart_w9wm, 0L);
        XSync(dpy, False);
        exit(0);
    }

    wm_state = XInternAtom(dpy, "WM_STATE", False);
    wm_change_state = XInternAtom(dpy, "WM_CHANGE_STATE", False);
    wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
    wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    wm_take_focus = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
    wm_colormaps = XInternAtom(dpy, "WM_COLORMAP_WINDOWS", False);
    _w9wm_running = XInternAtom(dpy, "_w9wm_RUNNING", False);
    _w9wm_hold_mode = XInternAtom(dpy, "_w9wm_HOLD_MODE", False);

    black = BlackPixel(dpy, screen);
    white = WhitePixel(dpy, screen);

    if (fname != 0)
        if ((font = XLoadQueryFont(dpy, fname)) == 0)
            fprintf(stderr, "w9wm: warning: can't load font %s\n", fname);

    if (font == 0) {
        i = 0;
        for (;;) {
            fname = fontlist[i++];
            if (fname == 0) {
                fprintf(stderr, "w9wm: can't find a font\n");
                exit(1);
            }
            font = XLoadQueryFont(dpy, fname);
            if (font != 0)
                break;
        }
    }
    if (nostalgia)
        _border--;

    gv.foreground = black^white;
    gv.background = white;
    gv.font = font->fid;
    gv.function = GXxor;
    gv.line_width = 0;
    gv.subwindow_mode = IncludeInferiors;
    mask = GCForeground | GCBackground | GCFunction | GCFont | GCLineWidth
           | GCSubwindowMode;
    gc = XCreateGC(dpy, root, mask, &gv);

    initcurs();

#ifdef  SHAPE
    shape = XShapeQueryExtension(dpy, &shape_event, &dummy);
#endif

    attr.cursor = arrow;
    attr.event_mask = SubstructureRedirectMask
                      | SubstructureNotifyMask | ColormapChangeMask
                      | ButtonPressMask | ButtonReleaseMask
                      | PropertyChangeMask | StructureNotifyMask
                      | (use_keys ? KeyPressMask : 0) ;
    mask = CWCursor|CWEventMask;
    XChangeWindowAttributes(dpy, root, mask, &attr);
    XSync(dpy, False);

    if (background) {
        XSetWindowBackgroundPixmap(dpy, root, root_pixmap);
        XClearWindow(dpy, root);
    }

    menuwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 1, black, white);
    initb2menu(numvirtuals);

    /* set selection so that 9term knows we're running */
    curtime = CurrentTime;
    XSetSelectionOwner(dpy, _w9wm_running, menuwin, timestamp());

    XSync(dpy, False);
    initting = 0;

    nofocus();
    scanwins();

    xmobar();
    fprintf(barhandle, "[%d]\n", virtual + 1);

    for (;;) {
        getevent(&ev);

#ifdef  DEBUG_EV
        if (debug) {
            ShowEvent(&ev);
            printf("\n");
        }
#endif
        switch (ev.type) {
        default:
#ifdef  SHAPE
            if (shape && ev.type == shape_event)
                shapenotify((XShapeEvent *)&ev);
            else
#endif
                fprintf(stderr, "w9wm: unknown ev.type %d\n", ev.type);
            break;
        case KeyRelease:
            if (use_keys && current) {
                ev.xkey.window = current->window;
                XSendEvent(dpy, current->window, False, NoEventMask, &ev);
            }
            break;
        case KeyPress:
            if (use_keys) {
                if (ev.xkey.state & Mod1Mask) {
                    if (getKeysym(&ev) == XK_Tab) {
                        if (ev.xkey.state & ShiftMask) {
                            activateprevious();
                        } else {
                            activatenext();
                        }
                    } else if (isdigitkey(getKeysym(&ev))) {
                        switch_to(isdigitkey(XLookupKeysym(&(ev.xkey),0)) - 1);
                        if (current) {
                            cmapfocus(current);
                        }
                    } else if (getKeysym(&ev) == XK_r) {
                        if (fork() == 0) {
                            if (fork() == 0) {
                                close(ConnectionNumber(dpy));
                                execlp("dmenu_run", "dmenu_run", 0);
                                exit(1);
                            }
                            exit(0);
                        }
                        wait((int*)0);
                    } else if (getKeysym(&ev) == XK_t) {
                        spawn();
                    } else if (getKeysym(&ev) == XK_m) {
                        move(current);
                    } else if (getKeysym(&ev) == XK_n) {
                        reshape(current);
                    } else if (getKeysym(&ev) == XK_d) {
							sendcmessage(current->window, wm_protocols, wm_delete);
                    } else if (getKeysym(&ev) == XK_q) {
                        if (ev.xkey.state & ShiftMask) {
                            sendcmessage(root, exit_w9wm, 0L);
                            XSync(dpy, False);
                            exit(0);
						}
                    } else if (getKeysym(&ev) == XK_w) {
                        if (ev.xkey.state & ShiftMask) {
                            sendcmessage(root, restart_w9wm, 0L);
                            XSync(dpy, False);
                        }  
                    } else {
                        ev.xkey.window = current->window;
                        XSendEvent(dpy, current->window, False, NoEventMask, &ev);
                    }
                } else if (current) {
                    ev.xkey.window = current->window;
                    XSendEvent(dpy, current->window, False, NoEventMask, &ev);
                }
            }
            XAllowEvents(dpy, SyncBoth, ev.xkey.time);
            break;
        case ButtonPress:
            button(&ev.xbutton);
            /*  option */
            {
                XAllowEvents (dpy, SyncPointer, ev.xbutton.time);
            }
            break;
        case ButtonRelease:
            break;
        case MapRequest:
            mapreq(&ev.xmaprequest);
            break;
        case ConfigureRequest:
            configurereq(&ev.xconfigurerequest);
            break;
        case CirculateRequest:
            circulatereq(&ev.xcirculaterequest);
            break;
        case UnmapNotify:
            unmap(&ev.xunmap);
            break;
        case CreateNotify:
            newwindow(&ev.xcreatewindow);
            break;
        case DestroyNotify:
            destroy(ev.xdestroywindow.window);
            break;
        case ClientMessage:
            clientmesg(&ev.xclient);
            break;
        case ColormapNotify:
            cmap(&ev.xcolormap);
            break;
        case PropertyNotify:
            property(&ev.xproperty);
            break;
        case SelectionClear:
            fprintf(stderr, "w9wm: SelectionClear (this should not happen)\n");
            break;
        case SelectionNotify:
            fprintf(stderr, "w9wm: SelectionNotify (this should not happen)\n");
            break;
        case SelectionRequest:
            fprintf(stderr, "w9wm: SelectionRequest (this should not happen)\n");
            break;
        case EnterNotify:
            enter(&ev.xcrossing);
            break;
        case ReparentNotify:
            reparent(&ev.xreparent);
            break;
        case MotionNotify:
        case Expose:
        case FocusIn:
        case FocusOut:
        case ConfigureNotify:
        case MappingNotify:
        case MapNotify:
            /* not interested */
            break;
        }
    }
}

void
activateprevious() {
    Client * c, * tmp = NULL;

    if (!current)
        current = clients;

    if (!clients || !current)
        return;

    for (c = clients; c->next; c=c->next) {
        if (c->virtual == virtual &&
                c->state == NormalState)
            tmp = c;
        if (tmp &&
                c->next == current)
            break;
    }

    if (tmp &&
            tmp->state == NormalState &&
            tmp->parent != root) {	/* paranoid */
        active(tmp);
        XMapRaised(dpy, tmp->parent);
        return;
    }
}

void
activatenext() {
    Client * c;

    if (!current)
        current = clients;

    if (!clients || !current)
        return;

    for (c=current->next; c != current; c = ((c && c->next) ? c->next : clients))
        if (c &&
                c->state == NormalState &&
                c->virtual == virtual &&
                c->parent != root) {	/* paranoid */
            active(c);
            XMapRaised(dpy, c->parent);
            return;
        }
}

void
usage() {
    fprintf(stderr, "usage: w9wm [[-display|-dpy] dpy] [-grey] [-version] [-font fname] [-pass]\n"
            "       [-nokeys] [-debug] [-nostalgia] [-term prog] [-pass] [-virtuals n]\n"
            "       [exit|restart]\n");
    exit(1);
}

void
sendcmessage(w, a, x)
Window w;
Atom a;
long x;
{
    XEvent ev;
    int status;
    long mask;

    memset(&ev, 0, sizeof(ev));
    ev.xclient.type = ClientMessage;
    ev.xclient.window = w;
    ev.xclient.message_type = a;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = x;
    ev.xclient.data.l[1] = timestamp();
    mask = 0L;
    if (w == root)
        mask = SubstructureRedirectMask;        /* magic! */
    status = XSendEvent(dpy, w, False, mask, &ev);
    if (status == 0)
        fprintf(stderr, "w9wm: sendcmessage failed\n");
}

Time
timestamp() {
    XEvent ev;

    if (curtime == CurrentTime) {
        XChangeProperty(dpy, root, _w9wm_running, _w9wm_running, 8,
                        PropModeAppend, (unsigned char *)"", 0);
        XMaskEvent(dpy, PropertyChangeMask, &ev);
        curtime = ev.xproperty.time;
    }
    return curtime;
}

void
sendconfig(c)
Client *c;
{
    XConfigureEvent ce;

    ce.type = ConfigureNotify;
    ce.event = c->window;
    ce.window = c->window;
    ce.x = c->x;
    ce.y = c->y;
    ce.width = c->dx;
    ce.height = c->dy;
    ce.border_width = c->border;
    ce.above = None;
    ce.override_redirect = 0;
    XSendEvent(dpy, c->window, False, StructureNotifyMask, (XEvent*)&ce);
}

void
scanwins() {
    unsigned int i, nwins;
    Client *c;
    Window dw1, dw2, *wins;
    XWindowAttributes attr;

    XQueryTree(dpy, root, &dw1, &dw2, &wins, &nwins);
    for (i = 0; i < nwins; i++) {
        XGetWindowAttributes(dpy, wins[i], &attr);
        if (attr.override_redirect || wins[i] == menuwin)
            continue;
        c = getclient(wins[i], 1);
        if (c != 0 && c->window == wins[i]) {
            c->x = attr.x;
            c->y = attr.y;
            c->dx = attr.width;
            c->dy = attr.height;
            c->border = attr.border_width;
            if (attr.map_state == IsViewable)
                manage(c, 1);
        }
    }
    XFree((void *) wins);   /* cast is to shut stoopid compiler up */
}

void
configurereq(e)
XConfigureRequestEvent *e;
{
    XWindowChanges wc;
    /*      XConfigureEvent ce; */
    Client *c;

    /* we don't set curtime as nothing here uses it */
    c = getclient(e->window, 0);
    if (c) {
        gravitate(c, 1);

        if (e->value_mask & CWX)
            c->x = e->x;
        if (e->value_mask & CWY)
            c->y = e->y;
        if (e->value_mask & CWWidth) {
            c->dx = e->width;
            XResizeWindow(dpy, c->parent, c->dx+2*(BORDER-1), c->dy+2*(BORDER-1));
        }
        if (e->value_mask & CWHeight) {
            c->dy = e->height;
            XResizeWindow(dpy, c->parent, c->dx+2*(BORDER-1), c->dy+2*(BORDER-1));
        }
        if (e->value_mask & CWBorderWidth)
            c->border = e->border_width;
        gravitate(c, 0);
        if (c->parent != root && c->window == e->window) {
            wc.x = c->x-BORDER;
            wc.y = c->y-BORDER;
            wc.width = c->dx+2*(BORDER-1);
            wc.height = c->dy+2*(BORDER-1);
            wc.border_width = 1;
            wc.sibling = e->above;
            wc.stack_mode = e->detail;
            XConfigureWindow(dpy, e->parent, e->value_mask, &wc);
            sendconfig(c);
        }
    }

    if (c && c->init) {
        wc.x = BORDER-1;
        wc.y = BORDER-1;
    } else {
        wc.x = e->x;
        wc.y = e->y;
    }
    wc.width = e->width;
    wc.height = e->height;
    wc.border_width = 0;
    wc.sibling = e->above;
    wc.stack_mode = e->detail;
    e->value_mask |= CWBorderWidth;

    XConfigureWindow(dpy, e->window, e->value_mask, &wc);
}

void
mapreq(e)
XMapRequestEvent *e;
{
    Client *c;

    curtime = CurrentTime;
    c = getclient(e->window, 0);
    if (c == 0 || c->window != e->window) {
        fprintf(stderr, "w9wm: bogus mapreq %p %p\n", c, (void*)e->window);
        return;
    }

    switch (c->state) {
    case WithdrawnState:
        if (c->parent == root) {
            if (!manage(c, 0))
                return;
            break;
        }
        XReparentWindow(dpy, c->window, c->parent, BORDER-1, BORDER-1);
        XAddToSaveSet(dpy, c->window);
    /* fall through... */
    case NormalState:
        XMapRaised(dpy, c->parent);
        XMapWindow(dpy, c->window);
        setstate9(c, NormalState);
        //        if (c->trans != None && current && c->trans == current->window)
        active(c);
        break;
    case IconicState:
        unhidec(c, 1);
        break;
    }
}

void
unmap(e)
XUnmapEvent *e;
{
    Client *c;

    curtime = CurrentTime;
    c = getclient(e->window, 0);
    if (c) {
        switch (c->state) {
        case IconicState:
            if (e->send_event) {
                unhidec(c, 0);
                withdraw(c);
            }
            break;
        case NormalState:
            if (c == current)
                nofocus();
            if (!c->reparenting) {
                withdraw(c);
            }
            break;
        }
        c->reparenting = 0;
    }
}

void
circulatereq(e)
XCirculateRequestEvent *e;
{
    fprintf(stderr, "It must be the warlock Krill!\n");  /* :-) */
}

void
newwindow(e)
XCreateWindowEvent *e;
{
    Client *c;

    /* we don't set curtime as nothing here uses it */
    if (e->override_redirect)
        return;
    c = getclient(e->window, 1);
    if (c && c->parent == root) {
        c->x = e->x;
        c->y = e->y;
        c->dx = e->width;
        c->dy = e->height;
        c->border = e->border_width;
    }
}

void
destroy(w)
Window w;
{
    Client *c;

    curtime = CurrentTime;
    c = getclient(w, 0);
    if (c == 0)
        return;

    rmclient(c);

    /* flush any errors generated by the window's sudden demise */
    ignore_badwindow = 1;
    XSync(dpy, False);
    ignore_badwindow = 0;
}

void
clientmesg(e)
XClientMessageEvent *e;
{
    Client *c;

    curtime = CurrentTime;
    if (e->window == root && e->message_type == exit_w9wm) {
        cleanup();
        exit(0);
    }
    if (e->window == root && e->message_type == restart_w9wm) {
        fprintf(stderr, "*** w9wm restarting ***\n");
        cleanup();
        execvp(myargv[0], myargv);
        perror("w9wm: exec failed");
        exit(1);
    }
    if (e->message_type == wm_change_state) {
        c = getclient(e->window, 0);
        if (e->format == 32 && e->data.l[0] == IconicState && c != 0) {
            if (normal(c))
                hide(c);
        } else
            fprintf(stderr, "w9wm: WM_CHANGE_STATE: format %d data %d w 0p%p\n",
                    e->format, (int)e->data.l[0], (void*)e->window);
        return;
    }
    fprintf(stderr, "w9wm: strange ClientMessage, type 0x%x window 0x%x\n",
            (int)e->message_type, (int)e->window);
}

void
cmap(e)
XColormapEvent *e;
{
    Client *c;
    int i;

    /* we don't set curtime as nothing here uses it */
    if (e->new) {
        c = getclient(e->window, 0);
        if (c) {
            c->cmap = e->colormap;
            if (c == current)
                cmapfocus(c);
        } else
            for (c = clients; c; c = c->next)
                for (i = 0; i < c->ncmapwins; i++)
                    if (c->cmapwins[i] == e->window) {
                        c->wmcmaps[i] = e->colormap;
                        if (c == current)
                            cmapfocus(c);
                        return;
                    }
    }
}

void
property(e)
XPropertyEvent *e;
{
    Atom a;
    int delete;
    Client *c;

    /* we don't set curtime as nothing here uses it */
    a = e->atom;
    delete = (e->state == PropertyDelete);
    c = getclient(e->window, 0);
    if (c == 0)
        return;

    switch (a) {
    case XA_WM_ICON_NAME:
        if (c->iconname != 0)
            XFree((char*) c->iconname);
        c->iconname = getprop(c->window, XA_WM_ICON_NAME);
        setlabel(c);
        renamec(c, c->label);
        return;
    case XA_WM_NAME:
        if (c->name != 0)
            XFree((char*) c->name);
        c->name = getprop(c->window, XA_WM_NAME);
        setlabel(c);
        renamec(c, c->label);
        return;
    case XA_WM_TRANSIENT_FOR:
        gettrans(c);
        return;
    }
    if (a == _w9wm_hold_mode) {
        c->hold = getiprop(c->window, _w9wm_hold_mode);
        if (c == current)
            draw_border(c, 1);
    } else if (a == wm_colormaps) {
        getcmaps(c);
        if (c == current)
            cmapfocus(c);
    }
}

void
reparent(e)
XReparentEvent *e;
{
    Client *c;
    XWindowAttributes attr;

    if (e->event != root || e->override_redirect)
        return;
    if (e->parent == root) {
        c = getclient(e->window, 1);
        if (c != 0 && (c->dx == 0 || c->dy == 0)) {
            XGetWindowAttributes(dpy, c->window, &attr);
            c->x = attr.x;
            c->y = attr.y;
            c->dx = attr.width;
            c->dy = attr.height;
            c->border = attr.border_width;
        }
    } else {
        c = getclient(e->window, 0);
        if (c != 0 && (c->parent == root || withdrawn(c)))
            rmclient(c);
    }
}

#ifdef  SHAPE
void
shapenotify(e)
XShapeEvent *e;
{
    Client *c;

    c = getclient(e->window, 0);
    if (c == 0)
        return;

    setshape(c);
}
#endif

void
enter(e)
XCrossingEvent *e;
{
    Client *c;

    curtime = e->time;
    if (e->mode != NotifyGrab || e->detail != NotifyNonlinearVirtual)
        return;
    c = getclient(e->window, 0);
    if (c != 0 && c != current) {
        XMapRaised(dpy, c->parent);
        active(c);
    }
}

void
sighandler() {
    signalled = 1;
}

void
getevent(e)
XEvent *e;
{
    int fd;
    fd_set rfds;
    struct timeval t;

    if (!signalled) {
        if (QLength(dpy) > 0) {
            XNextEvent(dpy, e);
            return;
        }
        fd = ConnectionNumber(dpy);
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        t.tv_sec = t.tv_usec = 0;
        if (select(fd+1, &rfds, NULL, NULL, &t) == 1) {
            XNextEvent(dpy, e);
            return;
        }
        XFlush(dpy);
        FD_SET(fd, &rfds);
        if (select(fd+1, &rfds, NULL, NULL, NULL) == 1) {
            XNextEvent(dpy, e);
            return;
        }
        if (errno != EINTR || !signalled) {
            perror("w9wm: select failed");
            exit(1);
        }
    }
    cleanup();
    fprintf(stderr, "w9wm: exiting on signal\n");
    exit(1);
}


void
parseprogsfile () {
    FILE *file;
    int i;
    struct passwd * p = getpwuid(getuid());
    char * buffer;

    buffer = (char *) malloc (1024);
    snprintf (buffer, 1024, "%s/.w9wmrc", p->pw_dir);

    file = fopen (buffer, "r");
    if (! file) {
        fprintf (stderr, "cannot open %s\n", buffer);
        progsnames[0] = NULL;
        return;
    }

    for (i = 0; i<16 && ! feof(file); i++) {
        buffer = (char *) malloc (1024);
        if (! fgets(buffer, 1024, file))
            break;
        if (buffer[strlen(buffer)-1] == '\n')
            buffer[strlen(buffer)-1] = 0;
        progsnames[i] = buffer;
    }

    progsnames[i] = NULL;

    fclose (file);
    return;
}
