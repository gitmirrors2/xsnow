/* -copyright-
#-# 
#-# xsnow: let it snow on your desktop
#-# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#-# 	      2019,2020,2021 Willem Vermin
#-# 
#-# This program is free software: you can redistribute it and/or modify
#-# it under the terms of the GNU General Public License as published by
#-# the Free Software Foundation, either version 3 of the License, or
#-# (at your option) any later version.
#-# 
#-# This program is distributed in the hope that it will be useful,
#-# but WITHOUT ANY WARRANTY; without even the implied warranty of
#-# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#-# GNU General Public License for more details.
#-# 
#-# You should have received a copy of the GNU General Public License
#-# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#-# 
*/

#include <stdio.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Intrinsic.h>
#include <ctype.h>
#include "debug.h"
#include "windows.h"
#include "flags.h"
#include "utils.h"
#include "xsnow.h"
#include "wmctrl.h"
#include "fallensnow.h"
#include "transparent.h"
#include "dsimple.h"
#include "varia.h"

#include "vroot.h"

static int    do_wupdate(gpointer data);
static void   UpdateFallenSnowRegions(void);
static Window XWinInfo(char **name);

static WinInfo      *Windows = NULL;
static int          NWindows;

char        *SnowWinName = NULL;
int          SnowWinX; 
int          SnowWinY; 
Window       Rootwindow;
unsigned int Wroot;
unsigned int Hroot;
int          Xroot;
int          Yroot;
GtkWidget   *TransA   = NULL;
GtkWidget   *TransB   = NULL;
Window       SnowWin  = 0;
Window       SnowWina = 0;
Window       SnowWinb = 0;

struct _switches switches;

void windows_ui()
{
}

void windows_draw(UNUSED cairo_t *cr)
{
   // nothing to draw
}

void DestroyWindow(Window w)
{
   return;
   if (w && w != Rootwindow)
      XDestroyWindow(display,w);
}

void windows_init()
{
   if (switches.Desktop)
      add_to_mainloop(PRIORITY_DEFAULT, time_wupdate, do_wupdate, NULL);
}

int WorkspaceActive()
{
   P("switches.UseGtk etc %d %d %d %d\n",Flags.AllWorkspaces,switches.UseGtk,CWorkSpace == TransWorkSpace,
	 Flags.AllWorkspaces || !switches.UseGtk || CWorkSpace == TransWorkSpace);
   // ah, so difficult ...
   return Flags.AllWorkspaces || !switches.UseGtk || CWorkSpace == TransWorkSpace;
}

int do_wupdate(UNUSED gpointer data)
{
   P("do_wupdate %d %d\n",counter++,WindowsChanged);
   if (Flags.Done)
      return FALSE;

   if(Flags.NoKeepSWin) return TRUE;
   long r;
   r = GetCurrentWorkspace();
   if(r>=0) 
      CWorkSpace = r;
   else
   {
      I("Cannot get current workspace\n");
      Flags.Done = 1;
      return TRUE;
   }

   if (!WindowsChanged)
      return TRUE;

   WindowsChanged = 0;

   R("Update windows\n");

   if(Windows) free(Windows);

   // special hack too keep SnowWin below (needed for example in FVWM/xcompmgr, 
   // where SnowWin is not click-through)
   {
      P("keep below %#lx\n",SnowWin);
      if(Flags.BelowAll)
      {
	 XWindowChanges changes;
	 changes.stack_mode = Below;
	 XConfigureWindow(display,SnowWin,CWStackMode,&changes);
      }
   }

   if (GetWindows(&Windows, &NWindows)<0)
   {
      I("Cannot get windows\n");
      Flags.Done = 1;
      return TRUE;
   }

   //P("%d:\n",counter++);printwindows(display,Windows,NWindows);
   //P("%d:\n",counter++);PrintFallenSnow(FsnowFirst);
   // Take care of the situation that the transparent window changes from workspace, 
   // which can happen if in a dynamic number of workspaces environment
   // a workspace is emptied.
   WinInfo *winfo;
   winfo = FindWindow(Windows,NWindows,SnowWin);

   // check also on valid winfo: after toggling 'below'
   // winfo is nil sometimes

   if(switches.UseGtk && winfo)
   {
      // in xfce and maybe others, workspace info is not to be found
      // in our transparent window. winfo->ws will be 0, and we keep
      // the same value for TransWorkSpace.

      if (winfo->ws > 0)
      {
	 TransWorkSpace = winfo->ws;
      }
      P("TransWorkSpace %#lx %#lx %#lx %#lx\n",TransWorkSpace,winfo->ws,SnowWin,GetCurrentWorkspace());
   }

   P("do_wupdate: %p %p\n",(void *)TransA,(void *)winfo);
   if (SnowWin != Rootwindow)
      if (!TransA && !winfo)
      {
	 I("No transparent window & no SnowWin %#lx found\n",SnowWin); 
	 Flags.Done = 1;
      }

   UpdateFallenSnowRegions();
   return TRUE;
}

// Have a look at the windows we are snowing on
// Also update of fallensnow area's
void UpdateFallenSnowRegions()
{
   typeof(Windows) w;
   typeof(FsnowFirst) f;
   int i;
   // add fallensnow regions:
   w = Windows;
   for (i=0; i<NWindows; i++)
   {
      //P("%d %#lx\n",i,w->id);
      {
	 f = FindFallen(FsnowFirst,w->id);
	 P("%#lx %d\n",w->id,w->dock);
	 if(f)
	 {
	    f->win = *w;   // update window properties
	    if ((!f->win.sticky) && f->win.ws != CWorkSpace)
	       CleanFallenArea(f,0,f->w);
	 }
	 if (!f)
	 {
	    // window found in Windows, nut not in list of fallensnow,
	    // add it, but not if we are snowing or birding in this window (Desktop for example)
	    // and also not if this window has y <= 0
	    // and also not if this window is a "dock"
	    P("               %#lx %d\n",w->id,w->dock);
	    if (w->id != SnowWina && w->id != SnowWinb && w->y > 0 && !(w->dock))
	       PushFallenSnow(&FsnowFirst, w,
		     w->x+Flags.OffsetX, w->y+Flags.OffsetY, w->w+Flags.OffsetW, 
		     Flags.MaxWinSnowDepth); 
	    //P("UpdateFallenSnowRegions:\n");PrintFallenSnow(FsnowFirst);
	 }
      }
      w++;
   }
   // remove fallensnow regions
   f = FsnowFirst; int nf = 0; while(f) { nf++; f = f->next; }
   // nf+1: prevent allocation of zero bytes
   long int *toremove = (long int *)malloc(sizeof(*toremove)*(nf+1));
   int ntoremove = 0;
   f = FsnowFirst;
   //Atom wmState  = XInternAtom(display, "_NET_WM_STATE", True);
   while(f)
   {
      if (f->win.id != 0)  // f->id=0: this is the snow at the bottom
      {
	 w = FindWindow(Windows,NWindows,f->win.id);
	 if(!w)   // this window is gone
	 {
	    GenerateFlakesFromFallen(f,0,f->w,-10.0);
	    toremove[ntoremove++] = f->win.id;
	 }

	 // test if f->win.id is hidden. If so: clear the area and notify in f
	 // we have to test that here, because the hidden status of the window
	 // can change
	 P("%#lx hidden:%d\n",f->win.id,f->win.hidden);
	 if (f->win.hidden)
	 {
	    P("%#lx is hidden %d\n",f->win.id, counter++);
	    CleanFallenArea(f,0,f->w);
	 }
      }
      f = f->next;
   }

   // test if window has been moved or resized
   // moved: move fallen area accordingly
   // resized: remove fallen area: add it to toremove
   w = Windows;
   for(i=0; i<NWindows; i++)
   {
      f = FindFallen(FsnowFirst,w->id);
      if (f)
      {
	 if ((unsigned int)f->w == w->w+Flags.OffsetW) // width has not changed
	 {
	    if (f->x != w->x + Flags.OffsetX || f->y != w->y + Flags.OffsetY)
	    {
	       CleanFallenArea(f,0,f->w);
	       f->x = w->x + Flags.OffsetX;
	       f->y = w->y + Flags.OffsetY;
	       DrawFallen(f);
	       XFlush(display);
	    }
	 }
	 else
	 {
	    toremove[ntoremove++] = f->win.id;
	 }
      }
      w++;
   }

   for (i=0; i<ntoremove; i++)
   {
      CleanFallen(toremove[i]);
      RemoveFallenSnow(&FsnowFirst,toremove[i]);
   }
   free(toremove);
}

// An hairy function, this is. It tries to determine a window to snow in, and possible to
//    let birds fly.
//
// IsDesktop (a return thing): true if the window is a desktop window, or a transparent click-through
//   window, created in this function.
//
// If user supplies window_id (Flags.WindowId), or points at window (Flags.XWinInfoHandling):
//    xwin:      the window_id
//    xwinname:  the name of the window
//    IsDesktop: 0
//    gtkwin:    0
//
// But normally, this function tries to create a transparent window:
// returning
//    xwin:      the window_id
//    xwinname:  the name of above window, equal to suplied transname
//    gtkwin:    a GtkWindow
//    Isdesktop: 1
//    
// If creating a transparent window fails, then:
//    the function desparately tries to find a window to snow in, using the following
//    heuristics:
//
//    if desktopsession is LXDE, then snowing will take place in window with name 'pcmanfm'
//       xwin:      the 'pcmanfm' window
//       xwinname:  "pcmanfm"
//       gtkwin:    0
//       IsDesktop: 1
//
//    if desktopdession is not LXDE, a search is made for a window with name "Desktop".
//      If such a window is found, and the dimensions are about
//      the same as that of the root window:
//       xwin:      the "Desktop" window
//       xwinname:  "Desktop"
//       gtkwin:    0
//       IsDesktop: 1
//
//       If that fails, the root window is chosen:
//          xwin:      the root window
//          xwinname:  something
//          gtkwin:    0
//          IsDesktop: 1
//
//  Spin-offs:
//     sets IsCompiz:   is this a compiz system?
//     sets CWorkSpace: current workspace
//     Rootwindow
//
//  Spin-ins:
//     Flags
//     display
//     and maybe more ...

int DetermineWindow(Window *xwin, char **xwinname, GtkWidget **gtkwin, const char *transname, int *IsDesktop)
{

   Rootwindow = DefaultRootWindow(display);
   P("DetermineWindow\n");
   *IsDesktop = 1;
   // User supplies window id:
   if (Flags.WindowId || Flags.XWinInfoHandling)
   {
      if(Flags.WindowId)
      {
	 *xwin = Flags.WindowId;
      }
      else
      {
	 // user ask to point to a window
	 printf("Click on a window ...\n");
	 *xwin = XWinInfo(xwinname);
	 if (*xwin == 0)
	 {
	    fprintf(stderr,"XWinInfo failed\n");
	    exit(1);
	 }
      }
      *IsDesktop = 0;
      *gtkwin = NULL;
   }
   // maybe we are started by xscreensaver: the window is in $XSCREENSAVER_WINDOW:
   //else if (getenv("XSCREENSAVER_WINDOW"))
   else if(Flags.ForceRoot)
   {
      //*xwin      = Window_With_Name(display,Rootwindow,"screensaver");
      //*xwin = strtol(getenv("XSCREENSAVER_WINDOW"),NULL,0);
      *xwin = DefaultRootWindow(display);
      if (getenv("XSCREENSAVER_WINDOW"))
      {
	 *xwin = strtol(getenv("XSCREENSAVER_WINDOW"),NULL,0);
	 Rootwindow = *xwin;
      }
      *IsDesktop = 0;
      *gtkwin    = NULL;
      int x,y; unsigned int w,h,b,depth;
      Window root;
      XGetGeometry(display,*xwin,&root,
	    &x, &y, &w, &h, &b, &depth);
      P("geom: %d %d %d %d\n",x,y,w,h);
      printf("Force snow on root: window: %#lx, depth: %d\n",*xwin,depth);
      if(0) // Trying to couple the virtual root window to gtk/cairo. No success ...
      {
	 GdkWindow *gdkwin;
	 GdkDisplay *gdkdisplay;
	 gdkdisplay = gdk_display_get_default();
	 P("display: %p gdkdisplay: %p\n",(void*)display,(void*)gdkdisplay);
	 gdkwin = gdk_x11_window_foreign_new_for_display(gdkdisplay,*xwin);
	 *gtkwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	 //gtk_window_set_screen(GTK_WINDOW(*gtkwin),gdk_window_get_screen(gdkwin));
	 gtk_widget_set_window(*gtkwin,gdkwin);
	 P("ww: %p gdkwin: %p\n",(void*)ww,(void*)gdkwin);
	 gtk_widget_show_all(*gtkwin);
	 gdk_window_show(gdkwin);
	 *IsDesktop = 1;
      }
   }
   else
   {
      // default behaviour
      // try first to create a transparent window
      P("DetermineWindow %p\n",(void *)gtkwin);

      int x,y;
      unsigned int w,h,b,depth;
      Window root;
      XGetGeometry(display,Rootwindow,&root,
	    &x, &y, &w, &h, &b, &depth);

      if (*gtkwin)
      {
	 gtk_window_close(GTK_WINDOW(*gtkwin));
	 gtk_widget_destroy(GTK_WIDGET(*gtkwin));
      }

      *gtkwin = gtk_window_new        (GTK_WINDOW_TOPLEVEL); 

      int rc = create_transparent_window(Flags.AllWorkspaces, Flags.BelowAll, 
	    xwin, transname, *gtkwin, w, h);
      if(0)if(!strcmp("Xsnow-B",transname))
      {
	 R("sleeping ...\n");
	 sleep(1000);
	 R("awake\n");
      }

      // todo: use rc for testing on transparency later on, not TransA

      if (!rc)
      {
	 gtk_window_close(GTK_WINDOW(*gtkwin));
	 *gtkwin = NULL;
      }

      *xwinname = strdup(transname);


      R("DetermineWindow gtkwin: %p xwin: %#lx xwinname: %s\n",(void *)gtkwin,*xwin,*xwinname);
      char *desktopsession = NULL;
      if (DesktopSession == NULL)
      {
	 const char *desktops[] = {
	    "DESKTOP_SESSION",
	    "XDG_SESSION_DESKTOP",
	    "XDG_CURRENT_DESKTOP",
	    "GDMSESSION",
	    NULL
	 };

	 int i;
	 for (i=0; desktops[i]; i++)
	 {
	    desktopsession = getenv(desktops[i]);
	    if (desktopsession)
	       break;
	 }
	 if (desktopsession)
	    printf("Detected desktop session: %s\n",desktopsession);
	 else
	 {
	    printf("Could not determine desktop session\n");
	    desktopsession = (char *)"unknown_desktop_session";
	 }

	 DesktopSession = strdup(desktopsession);

	 if (!strcasecmp(DesktopSession,"enlightenment"))
	    printf("NOTE: xsnow will probably run, but some glitches are to be expected.\n");
	 else if(!strcasecmp(DesktopSession,"twm"))
	    printf("NOTE: you probably need to tweak 'Lift snow on windows' in the 'settings' panel.\n");
      }

      // if not possible to create transparent window:
      if (*xwin == 0)
      {
	 // convert DesktopSession to upper case
	 char *a = DesktopSession;
	 while (*a)
	 {
	    *a = toupper(*a);
	    a++;
	 }
	 IsCompiz = (strstr(DesktopSession,"COMPIZ") != NULL);
	 P("IsCompiz %s %d\n",DesktopSession,IsCompiz);
	 // if envvar DESKTOP_SESSION == LXDE, search for window with name pcmanfm
	 if (DesktopSession != NULL && 
	       !strncmp(DesktopSession,"LXDE",4) && 
	       FindWindowWithName("pcmanfm",xwin,xwinname))
	 {
	    printf("LXDE session found, using window 'pcmanfm'.\n");
	    P("lxdefound: %d %#lx\n",lxdefound,*xwin);
	 }
	 else if (FindWindowWithName("Desktop",xwin,xwinname))
	 {
	    printf("Using window 'Desktop'.\n");
	 }
	 else
	 {
	    printf("Using root window\n");
	    *xwin = Rootwindow;
	 }
	 printf("You may have to tweak 'Advanced snow settings' in the 'settings' panel.\n");
      }
   }
   if(*IsDesktop)                                  
   {
      CWorkSpace = GetCurrentWorkspace();
      P("CWorkSpace: %d\n",CWorkSpace);
      if (CWorkSpace < 0)
	 return FALSE;
   }
   XTextProperty x;
   int rc = XGetWMName(display,*xwin,&x);
   if (rc)
      *xwinname = strdup((const char *)x.value);
   else
      *xwinname = strdup("no name");

   return TRUE;
}


Window XWinInfo(char **name)
{
   Window win = Select_Window(display,1);
   XTextProperty text_prop;
   int rc = XGetWMName(display,win,&text_prop);
   if (!rc)
      (*name) = strdup("No Name");
   else
      (*name) = strndup((char *)text_prop.value,text_prop.nitems);
   XFree(text_prop.value);  // cannot find this in the docs, but otherwise memory leak
   return win;
}

void InitDisplayDimensions()
{
   unsigned int wroot,hroot,broot,droot;
   int xroot,yroot;
   Window root;
   XGetGeometry(display,Rootwindow,&root,
	 &xroot, &yroot, &wroot, &hroot, &broot, &droot);
   Xroot = xroot;
   Yroot = yroot;
   Wroot = wroot;
   Hroot = hroot;
   P("InitDisplayDimensions: %p %d %d %d %d %d %d\n",(void*)Rootwindow,xroot,yroot,wroot,hroot,broot,droot);
   DisplayDimensions();
}

void DisplayDimensions()
{
   unsigned int w,h,b,d;
   int x,y,xr,yr;
   Window root,child_return;

   int rc = XGetGeometry(display,SnowWin,&root, &x, &y, &w, &h, &b, &d);
   if (rc == 0)
   {
      P("Oeps\n");
      I("\nSnow window %#lx has disappeared, it seems. I quit.\n",SnowWin);
      Thanks();
      exit(1);
      return;
   }

   XTranslateCoordinates(display, SnowWin, Rootwindow, 0, 0, &xr, &yr, &child_return);
   P("DisplayDimensions: %#lx %d %d %d %d %d %d %d %d\n",SnowWin,x,y,xr,yr,w,h,b,d);
   SnowWinX           = xr;// - x;
   SnowWinY           = yr;// - y;
   if(switches.UseGtk || switches.Trans)
   {
      SnowWinWidth       = Wroot - SnowWinX;
      SnowWinHeight      = Hroot - SnowWinY + Flags.OffsetS;
      //SnowWinHeight      = hroot + Flags.OffsetS;
   }
   else
   {
      SnowWinHeight      = h + Flags.OffsetS;
      SnowWinWidth       = w;
   }
   P("DisplayDimensions: SnowWinX: %d Y:%d W:%d H:%d\n",SnowWinX,SnowWinY,SnowWinWidth,SnowWinHeight);

   SnowWinBorderWidth = b;
   SnowWinDepth       = d;

   SetMaxScreenSnowDepth();
}

// Force window below or above other windows.
// It appears that, to get a window below other windows, it often is necessary
// to do first the opposite, and vice-versa.
void setbelow(GtkWindow *w)
{
   gtk_window_set_keep_above(GTK_WINDOW(w), TRUE);
   gtk_window_set_keep_below(GTK_WINDOW(w), TRUE);
}

void setabove(GtkWindow *w)
{
   gtk_window_set_keep_below(GTK_WINDOW(w), TRUE);
   gtk_window_set_keep_above(GTK_WINDOW(w), TRUE);
}

