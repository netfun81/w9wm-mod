/* Copyright (c) 1994 David Hogan, 2000 Benjamin Drieu, see README for licence details */
/* 9wm.c */
void    sendcmessage();
Time    timestamp();
void    sendconfig();
void    scanwins();
void    configurereq();
void    mapreq();
void    circulatereq();
void    unmap();
void    newwindow();
void    destroy();
int     ismapped();
void    clientmesg();
void    cmap();
void    property();
void    shapenotify();
void    enter();
void    reparent();
void	activatenext();
void	activateprevious();
void    parseprogsfile ();

/* manage.c */
int     manage();
void    setshape();
void    withdraw();
void    gravitate();
void    cleanup();
void    cmapfocus();
void    getcmaps();
int     _getprop();
char    *getprop();
Window  getwprop();
int     getiprop();
int     getstate();
void    setstate9();
void    setlabel();
void    getproto();
void    gettrans();

/* menu.c */
void    button();
void    button2();
void    button3();
void    spawn();
void    reshape();
void    move();
void    delete();
void    hide();
void    unhide();
void    unhidec();
void    renamec();
void	switch_to();
void	switch_to_c();
void	initb2menu();

/* client.c */
void    setactive();
void    draw_border();
void    active();
void    nofocus();
Client  *getclient();
void    rmclient();
void    dump_revert();
void    dump_clients();

/* grab.c */
int     menuhit();
void    draw_text();
Client  *selectwin();
int     sweep();
int     drag();
void    getmouse();
void    setmouse();

/* error.c */
int     handler();
void    fatal();
void    graberror();
void    showhints();

/* cursor.c */
void    initcurs();
