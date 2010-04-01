#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/cursorfont.h>
#include <X11/Xaw/XawInit.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/SmeLine.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Viewport.h>

#include "config.h"
#include "grab.h"
#include "toolbox.h"

extern Display *dpy;

Cursor left_ptr;
Cursor menu_ptr;
Cursor qu_ptr;
Cursor no_ptr;

Pixmap bm_yes;
Pixmap bm_no;

static unsigned char bm_yes_data[] = {
    /* -------- -------- */  0x00,
    /* -------- -------- */  0x00,
    /* ------xx xx------ */  0x18,			   
    /* ----xxxx xxxx---- */  0x3c,
    /* ----xxxx xxxx---- */  0x3c,
    /* ------xx xx------ */  0x18,
    /* -------- -------- */  0x00,
    /* -------- -------- */  0x00
};

static unsigned char bm_no_data[] = { 0,0,0,0, 0,0,0,0 };

/* ---------------------------------------------------------------------- */
/* simple and handy error rotine                                          */

void
oops(char *msg)
{
    fprintf(stderr,"Oops: %s\n",msg);
    exit(1);
}

/* ---------------------------------------------------------------------- */
/* some menu stuff                                                        */

Widget
add_pulldown_menu(Widget menubar,
		  char *name)
{
    Widget menu,button;

    button = XtVaCreateManagedWidget(name,menuButtonWidgetClass,menubar,NULL);
    menu = XtVaCreatePopupShell("menu",simpleMenuWidgetClass,button, NULL);

    return menu;
}

Widget
add_menu_entry(Widget menu, char *name,
	       XtCallbackProc callback, XtPointer data)
{
    Widget entry;
    
    entry = XtVaCreateManagedWidget(name,smeBSBObjectClass,menu,NULL);
    if (callback)
        XtAddCallback(entry,XtNcallback,callback,data);
    return entry;
}

Widget
add_menu_sep(Widget menu,char *name)
{
    Widget entry;
    
    entry = XtVaCreateManagedWidget(name,smeLineObjectClass,menu,NULL);
    return entry;
}

/* ---------------------------------------------------------------------- */
/* right-mouse popupmenu                                                  */

static int sel=-1;

void
popdown_menu_CB(Widget widget, XtPointer client_data, XtPointer calldata)
{
    if (-1 == sel)
	sel = -2;
    XUngrabPointer(dpy,CurrentTime);
    XtDestroyWidget(widget);
}

void
select_menu_CB(Widget widget, XtPointer client_data, XtPointer calldata)
{
    sel = (int)client_data;
}

int
popup_menu(Widget parent,char *title, struct STRTAB *entries)
{
    Widget        menu,line;
    int           x,y,rx,ry,mask;
    Window        root,child;
    XtAppContext  context;
    int           i;

    sel = -1;
    if (!title)
	menu = XtVaCreatePopupShell("menu",simpleMenuWidgetClass,parent,NULL);
    else {
	menu = XtVaCreatePopupShell("menu",simpleMenuWidgetClass,parent,
				    XtNlabel,title,NULL);
	add_menu_sep(menu,"sep");
    }

    for (i = 0; entries[i].str != NULL; i++) {
	if (strlen(entries[i].str) == 0) {
	    add_menu_sep(menu,"sep");
	} else {
	    line = add_menu_entry(menu, entries[i].str,
				  select_menu_CB,
				  (XtPointer)(entries[i].nr));
	    if (entries[i].nr == -1)
		XtVaSetValues(line,XtNsensitive,False,NULL);
	}
    }
    
    XQueryPointer(dpy,
		  RootWindowOfScreen(XtScreen(menu)),
		  &root, &child,
		  &rx, &ry, &x, &y, &mask);
    XtVaSetValues(menu,
		  XtNx,x-10,
		  XtNy,y-10,
		  NULL);

    XtAddCallback(menu,XtNpopdownCallback,
		  (XtCallbackProc)popdown_menu_CB,(XtPointer)NULL);
    XtPopupSpringLoaded(menu);
    XGrabPointer(dpy, XtWindow(menu), False,
		 ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
		 GrabModeAsync, GrabModeAsync, None, None, CurrentTime);

    XDefineCursor(dpy,XtWindow(menu),menu_ptr);

    context = XtWidgetToApplicationContext (menu);
    while (sel == -1 || XtAppPending(context)) {
	XtAppProcessEvent (context, XtIMAll);
    }
    if (sel == -2)
	sel = -1;    
    return sel;
}


/* ---------------------------------------------------------------------- */
/* resource handling                                                      */

char*
get_string_resource(Widget widget, char *name)
{
    struct RESDATA { char *str; } resdata = { NULL };
    XtResource res_desc[] = {{
	NULL,  /* name goes here */
	XtCString,
	XtRString,
	sizeof(char*),
	XtOffset(struct RESDATA*,str),
	XtRString,
	""
    }};

    res_desc[0].resource_name = name;
    XtGetApplicationResources(widget,&resdata,
                              res_desc,XtNumber(res_desc),
                              NULL,0);
    return resdata.str;
}

/* ---------------------------------------------------------------------- */
/* some stuff for doing keyboard scroll with the viewport widget          */

void
kbd_scroll_viewport_AC(Widget widget,  XEvent *event,
		       String *params, Cardinal *num_params)
{
    XtOrientation  ori;
    long           dir, percent;
    Dimension      length = 0;

    if (2 != *num_params) {
	fprintf(stderr,"KbdScroll: wrong number of arguments\n");
	return;
    }

    if (XtClass(widget) != scrollbarWidgetClass) {
	fprintf(stderr,"KbdScroll: not a scrollbar\n");
	return;
    }

    XtVaGetValues(widget,XtNorientation,&ori,NULL);
    XtVaGetValues(widget,(ori == XtorientVertical) ? XtNheight : XtNwidth,
		  &length,NULL);


    if (0 == strcasecmp(params[0],"left") ||
	0 == strcasecmp(params[0],"up")) {
	dir = -1;
    } else if (0 == strcasecmp(params[0],"right") ||
	       0 == strcasecmp(params[0],"down")) {
	dir = 1;
    } else {
	fprintf(stderr,"KbdScroll: what is %s?\n",params[0]);
	return;
    }

    /* secound arg */
    percent = atoi(params[1]);
    if (percent <= 0 || percent > 100) {
	fprintf(stderr,"KbdScroll: invalid value: %s\n",params[1]);
	return;
    }
#if 0
    fprintf(stderr,"KbdScroll: %d %d %d => %d\n",
	    percent,length,dir,(dir*percent*length/100));
#endif
    XtCallCallbacks(widget, XtNscrollProc,
		    (XtPointer)(dir*percent*length/100));
}

void
report_viewport_CB(Widget widget, XtPointer client_data, XtPointer calldata)
{
    Widget w,s;

    w = (Widget)client_data;
    s = w;
    while (s && !XtIsShell(s))
	s = XtParent(s);
    if (s)
	XtInstallAllAccelerators(w,s);
}

/* ---------------------------------------------------------------------- */
/* ask/tell user                                                          */

void
popdown_CB(Widget widget, XtPointer client_data, XtPointer calldata)
{
    Widget cancel;

    cancel = (Widget)client_data;
    XtPopdown(cancel);
}

void
destroy_CB(Widget widget, XtPointer client_data, XtPointer calldata)
{
    Widget cancel;

    cancel = (Widget)client_data;
    XtDestroyWidget(cancel);
}

void
center_under_mouse(Widget shell, int w, int h)
{
    int     x,y,rx,ry,mask,width=0,height=0,nx,ny;
    Window  root,child;

    if (!XtIsShell(shell))
	oops("move_under_mouse: not a shell");

    XQueryPointer(dpy,
		  RootWindowOfScreen(XtScreen(shell)),
		  &root, &child,
		  &rx, &ry, &x, &y, &mask);
    XtVaGetValues(shell,
		  XtNwidth, &width,
		  XtNheight,&height,
		  NULL);
    if (width==0)  width=w;
    if (height==0) height=h;
    nx = rx-(width/2);
    ny = ry-(height/2);
    if (nx < 0) nx = 0;
    if (ny < 0) ny = 0;
    /* TODO: test right & bottom border too */
    XtVaSetValues(shell,
		  XtNx,nx,
		  XtNy,ny,
		  NULL);
}

void
get_user_string(Widget parent, char *title, char *label, char *value,
		XtCallbackProc ok, XtPointer data)
{
    Widget          shell,ask;
    char            *t,*l;

    t = get_string_resource(parent,title);
    l = get_string_resource(parent,label);
    if (strlen(t) == 0) t = title;
    if (strlen(l) == 0) l = label;

    shell = XtVaCreatePopupShell("popup_ask",transientShellWidgetClass,parent,
				 XtNtitle,t,
				 NULL);
    ask = XtVaCreateManagedWidget("ask",dialogWidgetClass,shell,
				  XtNlabel,l,
				  XtNvalue,value ? value : "",
				  NULL);
    XawDialogAddButton(ask,"ok",ok,data);
    XawDialogAddButton(ask,"cancel",
		       (XtCallbackProc)destroy_CB,(XtPointer)shell);
    XtInstallAllAccelerators
	(XtNameToWidget(ask,"value"),shell);

    center_under_mouse(XtParent(ask),200,80);
    XtPopup(XtParent(ask),XtGrabNonexclusive);
    XDefineCursor(dpy,XtWindow(shell),left_ptr);
}

void
tell_user(Widget parent, char *title, char *label)
{
    Widget          shell,tell;
    char            *t,*l;

    t = get_string_resource(parent,title);
    l = get_string_resource(parent,label);
    if (strlen(t) == 0) t = title;
    if (strlen(l) == 0) l = label;

    shell = XtVaCreatePopupShell("popup_tell",transientShellWidgetClass,parent,
				 XtNtitle,t,
				 NULL);
    tell = XtVaCreateManagedWidget("tell",dialogWidgetClass,shell,
				   XtNlabel,l,
				   NULL);
    XawDialogAddButton(tell,"ok",(XtCallbackProc)destroy_CB,
		       (XtPointer)shell);
    XtInstallAllAccelerators(tell,shell);

    center_under_mouse(XtParent(tell),200,80);
    XtPopup(XtParent(tell),XtGrabNonexclusive);
    XDefineCursor(dpy,XtWindow(shell),left_ptr);
}

void
xperror(Widget parent, char *msg)
{
    char text[512];

    sprintf(text,"%s: %s",msg,strerror(errno));
    tell_user(parent,"str_perror_title",text);
}

/* ---------------------------------------------------------------------- */
/* cool 3D-Buttons :-)                                                    */

void
set_shadowWidth_AC(Widget widget,  XEvent *event,
		   String *params, Cardinal *num_params)
{
    int depth;
    
    if (1 != *num_params) {
	fprintf(stderr,"SetShadowWidth: wrong number of arguments\n");
	return;
    }
    depth = atoi(params[0]);
    XtVaSetValues(widget,"shadowWidth" /* XtNshadowWidth */, depth,NULL);
}


/* ---------------------------------------------------------------------- */
/* some stuff for help                                                    */

void
help_AC(Widget widget,  XEvent *event,
	String *params, Cardinal *num_params)
{
    Widget          shell,help;
    char            *l;

    shell =
	XtVaCreatePopupShell("popup_help",transientShellWidgetClass,widget,
			     NULL);
    help = XtVaCreateManagedWidget("help",dialogWidgetClass,shell,
				   NULL);
    XawDialogAddButton(help,"ok",(XtCallbackProc)destroy_CB,
		       (XtPointer)shell);
    XtInstallAllAccelerators(help,shell);

    l = get_string_resource(widget,"help");
    if (strlen(l) == 0) {
	l = get_string_resource(widget,"str_no_help");
	if (strlen(l) == 0)
	    l = "no help";
    }

    center_under_mouse(shell,200,100);

    XtVaSetValues(help,XtNlabel,l,NULL);
    XtPopup(shell,XtGrabNonexclusive);
    XDefineCursor(dpy,XtWindow(shell),left_ptr);
}

/* ---------------------------------------------------------------------- */

void
create_pointers(Widget app_shell)
{
    XColor white,red,dummy;
    Screen *scr;
    
    left_ptr = XCreateFontCursor(dpy,XC_left_ptr);
    menu_ptr = XCreateFontCursor(dpy,XC_right_ptr);
    qu_ptr   = XCreateFontCursor(dpy,XC_question_arrow);
    scr = DefaultScreenOfDisplay(dpy);
    if (DefaultDepthOfScreen(scr) > 1) {
	if (XAllocNamedColor(dpy,DefaultColormapOfScreen(scr),
			     "white",&white,&dummy) &&
	    XAllocNamedColor(dpy,DefaultColormapOfScreen(scr),
			     "red",&red,&dummy)) {
	    XRecolorCursor(dpy,left_ptr,&red,&white);
	    XRecolorCursor(dpy,menu_ptr,&red,&white);
	    XRecolorCursor(dpy,qu_ptr,&red,&white);
	} 
    }
}

void
create_bitmaps(Widget app_shell)
{
    XColor black, dummy;

    bm_yes = XCreateBitmapFromData(dpy, XtWindow(app_shell),
				   bm_yes_data, 8,8);
    bm_no = XCreateBitmapFromData(dpy, XtWindow(app_shell),
				  bm_no_data, 8,8);

    XAllocNamedColor(dpy,DefaultColormapOfScreen(DefaultScreenOfDisplay(dpy)),
		     "black",&black,&dummy);
    no_ptr = XCreatePixmapCursor(dpy, bm_no, bm_no,
				 &black, &black,
				 0, 0);
}
