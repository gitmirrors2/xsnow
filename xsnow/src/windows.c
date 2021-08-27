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
#include "transwindow.h"
#include "dsimple.h"

#include "vroot.h"
static int    do_wupdate(void *);
static int    do_sendevent(void *);
static long   TransWorkSpace = -SOMENUMBER;  // workspace on which transparent window is placed

static WinInfo      *Windows = NULL;
static int          NWindows;

void windows_ui()
{
}

void windows_draw()
{
   // nothing to draw
}

void DestroyWindow(Window w)
{
   return;
   if (w && w != global.Rootwindow)
      XDestroyWindow(global.display,w);
}

void windows_init()
{
   if (global.Desktop)
      add_to_mainloop(PRIORITY_DEFAULT, time_wupdate, do_wupdate);
   if (!global.IsDouble)
      add_to_mainloop(PRIORITY_DEFAULT, 0.5, do_sendevent);
}

int WorkspaceActive()
{
   P("global.Trans etc %d %d %d %d\n",Flags.AllWorkspaces,global.Trans,global.CWorkSpace == TransWorkSpace,
	 Flags.AllWorkspaces || !global.Trans || global.CWorkSpace == TransWorkSpace);
   // ah, so difficult ...
   return Flags.AllWorkspaces || !global.Trans || global.CWorkSpace == TransWorkSpace;
}

int do_sendevent(void *dummy)
{
   P("do_sendevent %d\n",counter++);
   XExposeEvent event;

   event.type        = Expose;
   event.send_event  = True;
   event.display     = global.display;
   event.window      = global.SnowWin;
   event.x           = 0;
   event.y           = 0;
   event.width       = global.SnowWinWidth;
   event.height      = global.SnowWinHeight;

   XSendEvent(global.display, global.SnowWin, True, Expose, (XEvent *) &event);
   return TRUE;
   (void)dummy;
}

int do_wupdate(void *dummy)
{
   P("do_wupdate %d %d\n",counter++,global.WindowsChanged);
   if (Flags.Done)
      return FALSE;

   if(Flags.NoKeepSWin) return TRUE;

   if (!global.WindowsChanged)
      return TRUE;

   global.WindowsChanged = 0;

   long r;
   r = GetCurrentWorkspace();
   if(r>=0) 
      global.CWorkSpace = r;
   else
   {
      I("Cannot get current workspace\n");
      Flags.Done = 1;
      return TRUE;
   }


   P("Update windows\n");

   if(Windows) free(Windows);

   // special hack too keep global.SnowWin below (needed for example in FVWM/xcompmgr, 
   // where global.SnowWin is not click-through)
   {
      P("keep below %#lx\n",global.SnowWin);
      if(Flags.BelowAll)
      {
	 XWindowChanges changes;
	 changes.stack_mode = Below;
	 XConfigureWindow(global.display,global.SnowWin,CWStackMode,&changes);
      }
   }

   if (GetWindows(&Windows, &NWindows)<0)
   {
      I("Cannot get windows\n");
      Flags.Done = 1;
      return TRUE;
   }

   //P("%d:\n",counter++);printwindows(display,Windows,NWindows);
   //P("%d:\n",counter++);PrintFallenSnow(global.FsnowFirst);
   // Take care of the situation that the transparent window changes from workspace, 
   // which can happen if in a dynamic number of workspaces environment
   // a workspace is emptied.
   WinInfo *winfo;
   winfo = FindWindow(Windows,NWindows,global.SnowWin);

   // check also on valid winfo: after toggling 'below'
   // winfo is nil sometimes

   if(global.Trans && winfo)
   {
      // in xfce and maybe others, workspace info is not to be found
      // in our transparent window. winfo->ws will be 0, and we keep
      // the same value for TransWorkSpace.

      if (winfo->ws > 0)
      {
	 TransWorkSpace = winfo->ws;
      }
      P("TransWorkSpace %#lx %#lx %#lx %#lx\n",TransWorkSpace,winfo->ws,global.SnowWin,GetCurrentWorkspace());
   }

   P("do_wupdate: %d %p\n",global.Trans,(void *)winfo);
   if (global.SnowWin != global.Rootwindow)
      //if (!TransA && !winfo)  // let op
      if (!global.Trans && !winfo)
      {
	 I("No transparent window & no SnowWin %#lx found\n",global.SnowWin); 
	 Flags.Done = 1;
      }

   UpdateFallenSnowRegions();
   return TRUE;
   (void)dummy;
}

// Have a look at the windows we are snowing on
// Also update of fallensnow area's
void UpdateFallenSnowRegions()
{
   WinInfo *w;
   FallenSnow *f;
   int i;
   // add fallensnow regions:
   w = Windows;
   for (i=0; i<NWindows; i++)
   {
      //P("%d %#lx\n",i,w->id);
      {
	 f = FindFallen(global.FsnowFirst,w->id);
	 P("%#lx %d\n",w->id,w->dock);
	 if(f)
	 {
	    f->win = *w;   // update window properties
	    if ((!f->win.sticky) && f->win.ws != global.CWorkSpace)
	    {
	       P("CleanFallenArea\n");
	       CleanFallenArea(f,0,f->w);
	    }
	 }
	 if (!f)
	 {
	    // window found in Windows, nut not in list of fallensnow,
	    // add it, but not if we are snowing or birding in this window (Desktop for example)
	    // and also not if this window has y <= 0
	    // and also not if this window is a "dock"
	    P("               %#lx %d\n",w->id,w->dock);
	    // if (w->id != SnowWin_a && w->id != SnowWinb && w->y > 0 && !(w->dock)) // let op
	    if (w->id != global.SnowWin && w->y > 0 && !(w->dock)) 
	    {
	       if((int)(w->w) == global.SnowWinWidth && w->x == 0 && w->y <100) //maybe a transparent xpenguins window?
	       {
		  P("skipping: %d %#lx %d %d %d\n",global.counter++, w->id, w->w, w->x, w->y);
	       }
	       else
	       {
		  PushFallenSnow(&global.FsnowFirst, w,
			w->x+Flags.OffsetX, w->y+Flags.OffsetY, w->w+Flags.OffsetW, 
			Flags.MaxWinSnowDepth); 
	       }
	    }
	    //P("UpdateFallenSnowRegions:\n");PrintFallenSnow(global.FsnowFirst);
	 }
      }
      w++;
   }
   // remove fallensnow regions
   f = global.FsnowFirst; 
   int nf = 0; 
   while(f) 
   { 
      nf++; 
      f = f->next; 
   }
   // nf+1: prevent allocation of zero bytes
   long int *toremove = (long int *)malloc(sizeof(*toremove)*(nf+1));
   int ntoremove = 0;
   f = global.FsnowFirst;
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
	    P("CleanFallenArea\n");
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
      f = FindFallen(global.FsnowFirst,w->id);
      if (f)
      {
	 if ((unsigned int)f->w == w->w+Flags.OffsetW) // width has not changed
	 {
	    if (f->x != w->x + Flags.OffsetX || f->y != w->y + Flags.OffsetY)
	    {
	       CleanFallenArea(f,0,f->w);
	       P("CleanFallenArea\n");
	       f->x = w->x + Flags.OffsetX;
	       f->y = w->y + Flags.OffsetY;
	       DrawFallen(f);
	       XFlush(global.display);
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
      RemoveFallenSnow(&global.FsnowFirst,toremove[i]);
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
// and now it is not used any more... but we keep it here for future reference.
//
int DetermineWindow(Window *xwin, char **xwinname, GtkWidget **gtkwin, const char *transname, int *IsDesktop)
{

   global.Rootwindow = DefaultRootWindow(global.display);
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
   else if(Flags.ForceRoot)
   {
      //*xwin      = Window_With_Name(global.display,global.Rootwindow,"screensaver");
      //*xwin = strtol(getenv("XSCREENSAVER_WINDOW"),NULL,0);
      *xwin = DefaultRootWindow(global.display);
      if (getenv("XSCREENSAVER_WINDOW"))
      {
	 *xwin = strtol(getenv("XSCREENSAVER_WINDOW"),NULL,0);
	 global.Rootwindow = *xwin;
      }
      *IsDesktop = 0;
      *gtkwin    = NULL;
      int x,y; unsigned int w,h,b,depth;
      Window root;
      XGetGeometry(global.display,*xwin,&root,
	    &x, &y, &w, &h, &b, &depth);
      P("geom: %d %d %d %d\n",x,y,w,h);
      printf("Force snow on root: window: %#lx, depth: %d\n",*xwin,depth);
      if(0) // Trying to couple the virtual root window to gtk/cairo. No success ...
      {
	 GdkWindow *gdkwin;
	 GdkDisplay *gdkdisplay;
	 gdkdisplay = gdk_display_get_default();
	 P("display: %p gdkdisplay: %p\n",(void*)global.display,(void*)gdkdisplay);
	 gdkwin = gdk_x11_window_foreign_new_for_display(gdkdisplay,*xwin);
	 *gtkwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	 //gtk_window_set_screen(GTK_WINDOW(*gtkwin),gdk_window_get_screen(gdkwin));
	 gtk_widget_set_window(*gtkwin,gdkwin);
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
      XGetGeometry(global.display,global.Rootwindow,&root,
	    &x, &y, &w, &h, &b, &depth);

      if (*gtkwin)
      {
	 gtk_window_close(GTK_WINDOW(*gtkwin));
	 gtk_widget_destroy(GTK_WIDGET(*gtkwin));
      }

      *gtkwin = gtk_window_new        (GTK_WINDOW_TOPLEVEL); 

      int rc = make_trans_window(*gtkwin,
	    1,                   // full screen 
	    Flags.AllWorkspaces, // sticky 
	    Flags.BelowAll,      // below
	    1,                   // dock
	    NULL,                // gdk_window
	    xwin                 // x11_window
	    );
      gtk_window_set_title(GTK_WINDOW(*gtkwin), transname);
      //int rc = create_transparent_window(Flags.AllWorkspaces, Flags.BelowAll, 
      //	    xwin, transname, *gtkwin, w, h);

      if (!rc)
      {
	 gtk_window_close(GTK_WINDOW(*gtkwin));
	 *gtkwin = NULL;
      }

      *xwinname = strdup(transname);


      P("DetermineWindow gtkwin: %p xwin: %#lx xwinname: %s\n",(void *)gtkwin,*xwin,*xwinname);
      char *desktopsession = NULL;
      if (global.DesktopSession == NULL)
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

	 global.DesktopSession = strdup(desktopsession);

	 if (!strcasecmp(global.DesktopSession,"enlightenment"))
	    printf("NOTE: xsnow will probably run, but some glitches are to be expected.\n");
	 else if(!strcasecmp(global.DesktopSession,"twm"))
	    printf("NOTE: you probably need to tweak 'Lift snow on windows' in the 'settings' panel.\n");
      }

      // if not possible to create transparent window:
      if (*xwin == 0)
      {
	 // convert DesktopSession to upper case
	 if(global.DesktopSession)
	 {
	    char *a = global.DesktopSession;
	    while (*a)
	    {
	       *a = toupper(*a);
	       a++;
	    }
	 }
	 global.IsCompiz = (strstr(global.DesktopSession,"COMPIZ") != NULL);
	 P("IsCompiz %s %d\n",global.DesktopSession,global.IsCompiz);
	 // if envvar DESKTOP_SESSION == LXDE, search for window with name pcmanfm
	 if (global.DesktopSession != NULL && 
	       !strncmp(global.DesktopSession,"LXDE",4) && 
	       (*xwin = FindWindowWithName(global.display,"pcmanfm")))
	 {
	    printf("LXDE session found, using window 'pcmanfm'.\n");
	    P("lxdefound: %d %#lx\n",lxdefound,*xwin);
	 }
	 else if ((*xwin = FindWindowWithName(global.display,"Desktop")))
	 {
	    printf("Using window 'Desktop'.\n");
	 }
	 else
	 {
	    printf("Using root window\n");
	    *xwin = global.Rootwindow;
	 }
	 printf("You may have to tweak 'Advanced snow settings' in the 'settings' panel.\n");
      }
   }
   if(*IsDesktop)                                  
   {
      global.CWorkSpace = GetCurrentWorkspace();
      P("CWorkSpace: %d\n",global.CWorkSpace);
      if (global.CWorkSpace < 0)
	 return FALSE;
   }
   XTextProperty x;
   int rc = XGetWMName(global.display,*xwin,&x);
   if (rc)
      *xwinname = strdup((const char *)x.value);
   else
      *xwinname = strdup("no name");

   return TRUE;
}


Window XWinInfo(char **name)
{
   Window win = Select_Window(global.display,1);
   if(name)
   {
      XTextProperty text_prop;
      int rc = XGetWMName(global.display,win,&text_prop);
      if (!rc)
	 (*name) = strdup("No Name");
      else
	 (*name) = strndup((char *)text_prop.value,text_prop.nitems);
      XFree(text_prop.value);
   }
   return win;
}

void InitDisplayDimensions()
{
   unsigned int wroot,hroot,broot,droot;
   int xroot,yroot;
   Window root;
   XGetGeometry(global.display,global.Rootwindow,&root,
	 &xroot, &yroot, &wroot, &hroot, &broot, &droot);
   global.Xroot = xroot;
   global.Yroot = yroot;
   global.Wroot = wroot;
   global.Hroot = hroot;
   P("InitDisplayDimensions: %p %d %d %d %d %d %d\n",(void*)global.Rootwindow,xroot,yroot,wroot,hroot,broot,droot);
   DisplayDimensions();
}

void DisplayDimensions()
{
   unsigned int w,h,b,d;
   int x,y,xr,yr;
   Window root,child_return;

   int rc = XGetGeometry(global.display,global.SnowWin,&root, &x, &y, &w, &h, &b, &d);
   if (rc == 0)
   {
      P("Oeps\n");
      I("\nSnow window %#lx has disappeared, it seems. I quit.\n",global.SnowWin);
      Thanks();
      exit(1);
      return;
   }

   XTranslateCoordinates(global.display, global.SnowWin, global.Rootwindow, 0, 0, &xr, &yr, &child_return);
   P("DisplayDimensions: %#lx %#lx x:%d y:%d xr:%d yr:%d w:%d h:%d b:%d d:%d tr:%d\n",global.SnowWin,global.Rootwindow,x,y,xr,yr,w,h,b,d,global.Trans);
   global.SnowWinX           = xr;// - x;
   global.SnowWinY           = yr;// - y;
   if(global.Trans)
   {
      global.SnowWinWidth       = global.Wroot - global.SnowWinX;
      global.SnowWinHeight      = global.Hroot - global.SnowWinY + Flags.OffsetS;
   }
   else
   {
      global.SnowWinHeight      = h + Flags.OffsetS;
      global.SnowWinWidth       = w;
   }
   P("DisplayDimensions: SnowWinX: %d Y:%d W:%d H:%d\n",global.SnowWinX,global.SnowWinY,global.SnowWinWidth,global.SnowWinHeight);

   global.SnowWinBorderWidth = b;
   global.SnowWinDepth       = d;

   UpdateFallenSnowAtBottom();

   SetMaxScreenSnowDepth();
   if(!global.IsDouble)
      ClearScreen();
}

