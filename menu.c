/* Copyright (c) 1994 David Hogan, 2000 Benjamin Drieu, see README for licence details */
#include <stdio.h>
#include <unistd.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "dat.h"
#include "fns.h"

Client  *hiddenc[MAXHIDDEN];

int numhidden;
int virtual = 0;

Client * currents[NUMVIRTUALS] =
{
  NULL, NULL, NULL, NULL, 
};

char    *b2items[NUMVIRTUALS+1] = 
{
  "  One  ",			/* to produce a nicer menu */
  "Two",
  "Three",
  "Four",
  "Five",
  "Six",
  "Seven",
  "Eight",
  "Nine",
  "Ten",
  "Eleven",
  "Twelve",
  0,
};


Menu    b2menu =
{
    b2items,
};


char    *b3items[B3FIXED+MAXHIDDEN+1] = 
{
    "New",
    "Reshape",
    "Move",
    "Delete",
    "Hide",
    0,
};

Menu    b3menu =
{
    b3items,
};

Menu    egg =
{
    version,
};

Menu    progs =
{
  progsnames,
};

void
button(e)
XButtonEvent *e;
{
    Client *c;
    Window dw;

    curtime = e->time;
    c = getclient(e->window, 0);
    if (c) {
        e->x += c->x - BORDER + 1;
        e->y += c->y - BORDER + 1;
    }
    else if (e->window != root)
        XTranslateCoordinates(dpy, e->window, root, e->x, e->y,
                &e->x, &e->y, &dw);
    switch (e->button) {
    case Button1:
        if (c) {
            XMapRaised(dpy, c->parent);
            active(c);
	    if (click_passes)
	      XAllowEvents (dpy, ReplayPointer, curtime);
        }
	    else if ((e->state&(Mod4Mask))==(Mod4Mask)
		 && progsnames[0] != NULL)
	  {
	    int n;
	    if ((n = menuhit(e, &progs)) != -1)
	      {
		if (fork() == 0) {
		  if (fork() == 0) {
		    close(ConnectionNumber(dpy));
		    execlp(progsnames[n], progsnames[n], 0);
		    exit(1);
		  }
		  exit(0);
		}
		wait((int *) 0);
	      }
	  }
        return;
    case Button2:
        if (c && click_passes) {
            XMapRaised(dpy, c->parent);
            active(c);
	    XAllowEvents (dpy, ReplayPointer, curtime);
        }
	else 
        spawn();
        return;
    default:
        return;
    case Button3:
        if (c && click_passes) {
            XMapRaised(dpy, c->parent);
            active(c);
	    XAllowEvents (dpy, ReplayPointer, curtime);
        }
     else {
        if ((e->state&(Mod4Mask))==(Mod4Mask))
            button2(e);
	     else
            button3(e);
        }
        break;
    }
}


void 
button2(e)
XButtonEvent *e;
{
    int n;
    cmapfocus(0);
    if ((n = menuhit(e, &b2menu)) == -1)
      return;
    switch_to(n);
    if (current)
        cmapfocus(current);
}

void
initb2menu(n)
int n;
{
  b2items[n] = 0;
}

void 
button3(e)
XButtonEvent *e;
{
    int n, shift;
    Client *c;
    cmapfocus(0);
    switch (n = menuhit(e, &b3menu)) {
    case 0:     /* New */
        spawn();
        break;
    case 1:     /* Reshape */
        reshape(selectwin(1, 0));
        break;
    case 2:     /* Move */
        move(selectwin(0, 0));
        break;
    case 3:     /* Delete */
        shift = 0;
        c = selectwin(1, &shift);
        delete(c, shift);
        break;
    case 4:     /* Hide */
        hide(selectwin(1, 0));
        break;
    default:    /* unhide window */
        unhide(n - B3FIXED, 1);
        break;
    case -1:    /* nothing */
        break;
    }
    if (current)
        cmapfocus(current);
}




void
switch_to(n)
int n;
{
  if (n == virtual)
    return;
  currents[virtual] = current;
  virtual = n;
  switch_to_c(n,clients);
  current = currents[virtual];
}


void
switch_to_c(n,c)
int n;
Client * c;
{
  if (c && c->next)
    switch_to_c(n,c->next);

  if (c->parent == DefaultRootWindow(dpy))
    return;

  if (c->virtual != virtual && c->state == NormalState)
    {
      XUnmapWindow(dpy, c->parent);
      XUnmapWindow(dpy, c->window);
      setstate9(c, IconicState);
      if (c == current)
	nofocus();
    }
  else if (c->virtual == virtual &&  c->state == IconicState)
    {
      int i;

      for (i = 0; i < numhidden; i++)
	if (c == hiddenc[i]) 
	  break;

      if (i == numhidden)
	{
	  XMapWindow(dpy, c->window);
	  XMapWindow(dpy, c->parent);
	  setstate9(c, NormalState);
	  if (currents[virtual] == c)
	    active(c); 
	}
    }
}



void
spawn()
{
    /*
     * ugly dance to avoid leaving zombies.  Could use SIGCHLD,
     * but it's not very portable, and I'm in a hurry...
     */
    if (fork() == 0) {
        if (fork() == 0) {
            close(ConnectionNumber(dpy));
            if (termprog != NULL) {
                execl(shell, shell, "-c", termprog, 0);
                fprintf(stderr, "9wm: exec %s", shell);
                perror(" failed");
            }
            execlp("xterm", "xterm", "-ut", 0);
            execlp("9term", "9term", "-9wm", 0);
            perror("9wm: exec 9term/xterm failed");
            exit(1);
        }
        exit(0);
    }
    wait((int *) 0);
}

void
reshape(c)
Client *c;
{
    int odx, ody;

    if (c == 0)
        return;
    odx = c->dx;
    ody = c->dy;
    if (sweep(c) == 0)
        return;
    active(c);
    XRaiseWindow(dpy, c->parent);
    XMoveResizeWindow(dpy, c->parent, c->x-BORDER, c->y-BORDER,
                    c->dx+2*(BORDER-1), c->dy+2*(BORDER-1));
    if (c->dx == odx && c->dy == ody)
        sendconfig(c);
    else
        XMoveResizeWindow(dpy, c->window, BORDER-1, BORDER-1, c->dx, c->dy);
}

void
move(c)
Client *c;
{
    if (c == 0)
        return;
    if (drag(c) == 0)
        return;
    active(c);
    XRaiseWindow(dpy, c->parent);
    XMoveWindow(dpy, c->parent, c->x-BORDER, c->y-BORDER);
    sendconfig(c);
}

void
delete(c, shift)
Client *c;
int shift;
{
    if (c == 0)
        return;
    if ((c->proto & Pdelete) && !shift)
        sendcmessage(c->window, wm_protocols, wm_delete);
    else
        XKillClient(dpy, c->window);        /* let event clean up */
}

void
hide(c)
Client *c;
{
    if (c == 0 || numhidden == MAXHIDDEN)
        return;
    if (hidden(c)) {
        fprintf(stderr, "9wm: already hidden: %s\n", c->label);
        return;
    }
    XUnmapWindow(dpy, c->parent);
    XUnmapWindow(dpy, c->window);
    setstate9(c, IconicState);
    if (c == current)
        nofocus();
    hiddenc[numhidden] = c;
    b3items[B3FIXED+numhidden] = c->label;
    numhidden++;
    b3items[B3FIXED+numhidden] = 0;
}

void
unhide(n, map)
int n;
int map;
{
    Client *c;
    int i;

    if (n >= numhidden) {
        fprintf(stderr, "9wm: unhide: n %d numhidden %d\n", n, numhidden);
        return;
    }
    c = hiddenc[n];
    if (!hidden(c)) {
        fprintf(stderr, "9wm: unhide: not hidden: %s(0x%lx)\n",
            c->label, c->window);
        return;
    }

    if (map) {
        XMapWindow(dpy, c->window);
        XMapRaised(dpy, c->parent);
        setstate9(c, NormalState);
        active(c);
    }

    c->virtual = virtual;
    numhidden--;
    for (i = n; i < numhidden; i ++) {
        hiddenc[i] = hiddenc[i+1];
        b3items[B3FIXED+i] = b3items[B3FIXED+i+1];
    }
    b3items[B3FIXED+numhidden] = 0;
}

void
unhidec(c, map)
Client *c;
int map;
{
    int i;
    
    for (i = 0; i < numhidden; i++)
        if (c == hiddenc[i]) {
            unhide(i, map);
            return;
        }
    fprintf(stderr, "9wm: unhidec: not hidden: %s(0x%lx)\n",
        c->label, c->window);
}

void
renamec(c, name)
Client *c;
char *name;
{
    int i;

    if (name == 0)
        name = "???";
    c->label = name;
    if (!hidden(c))
        return;
    for (i = 0; i < numhidden; i++)
        if (c == hiddenc[i]) {
            b3items[B3FIXED+i] = name;
            return;
        }
}
