#define BORDER      _border
#define MAXHIDDEN   32
#define B3FIXED     5
#define NUMVIRTUALS 12

#define AllButtonMask   (Button1Mask|Button2Mask|Button3Mask \
            |Button4Mask|Button5Mask)
#define ButtonMask  (ButtonPressMask|ButtonReleaseMask)
#define MenuMask    (ButtonMask|ButtonMotionMask|ExposureMask)
#define MenuGrabMask    (ButtonMask|ButtonMotionMask|StructureNotifyMask)

#ifdef  Plan9
#define DEFSHELL    "/bin/rc"
#else
#define DEFSHELL    "/bin/sh"
#endif

typedef struct Client   Client;
typedef struct Menu Menu;

struct Client {
    Window      window;
    Window      parent;
    Window      trans;
    Client      *next;
    Client      *revert;

    int         x;
    int         y;
    int         dx;
    int         dy;
    int         border;

    XSizeHints  size;
    int         min_dx;
    int         min_dy;

    int         state;
    int         init;
    int         reparenting;
    int         is9term;
    int         hold;
    int         proto;

    char        *label;
    char        *instance;
    char        *class;
    char        *name;
    char        *iconname;

    Colormap    cmap;
    int         ncmapwins;
    Window      *cmapwins;
    Colormap    *wmcmaps;

    /* benj: virtual screen */
    int		virtual;
};

#define hidden(c)       ((c)->state == IconicState)
#define withdrawn(c)    ((c)->state == WithdrawnState)
#define normal(c)       ((c)->state == NormalState)

/* c->proto */
#define Pdelete     1
#define Ptakefocus  2

struct Menu {
    char    **item;
    char    *(*gen)();
    int lasthit;
};

/* w9wm.c */
extern Display      *dpy;
extern int          screen;
extern Window       root;
extern Window       menuwin;
extern Colormap     def_cmap;
extern int          initting;
extern GC           gc;
extern XFontStruct  *font;
extern int          nostalgia;
extern Atom	    exit_w9wm;
extern Atom	    restart_w9wm;
extern Atom         wm_state;
extern Atom         _w9wm_hold_mode;
extern Atom         wm_protocols;
extern Atom         wm_delete;
extern Atom         wm_take_focus;
extern Atom         wm_colormaps;
extern unsigned long    black;
extern unsigned long    white;
extern Bool         shape;
extern char         *termprog;
extern char         *shell;
extern char         *version[];
extern int          _border;
extern int          min_cmaps;
extern int          curtime;
extern int          debug;
extern int	    click_passes;
extern int	    use_keys;
/* client.c */
extern Client       *clients;
extern Client       *current;

/* menu.c */
extern Client       *hiddenc[];
extern int          numhidden;
extern char         *b3items[];
extern Menu         b3menu;
extern int	    virtual;
extern Client *	    currents[];
extern Menu	    progs;
extern char *	    progsnames[];

/* cursor.c */
extern Cursor       target;
extern Cursor       sweep0;
extern Cursor       boxcurs;
extern Cursor       arrow;
extern Pixmap       root_pixmap;

/* error.c */
extern int          ignore_badwindow;

