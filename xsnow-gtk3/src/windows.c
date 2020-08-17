/* -copyright-
#-# 
#-# xsnow: let it snow on your desktop
#-# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#-#               2019,2020 Willem Vermin
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
#include "birds.h"
#include "dsimple.h"

static int    do_wupdate(gpointer data);
static void   UpdateWindows(void);
static Window XWinInfo(char **name);

static WinInfo      *Windows = 0;
static int          NWindows;

int          UsingTrans = 0;
int          Isdesktop;
char        *SnowWinName = 0;
int     SnowWinX; 
int     SnowWinY; 
Window       RootWindow;
unsigned int Wroot;
unsigned int Hroot;
int          Xroot;
int          Yroot;

int windows_ui()
{
   int changes = 0;
   return changes;
}

void windows_draw(cairo_t *cr)
{
   // nothing to draw
}

void windows_init()
{
   add_to_mainloop(PRIORITY_DEFAULT, time_wupdate, do_wupdate, 0);
}

int WorkspaceActive()
{
   P("UsingTrans etc %d %d %d %d\n",Flags.AllWorkspaces,UsingTrans,CWorkSpace == TransWorkSpace,
	 Flags.AllWorkspaces || !UsingTrans || CWorkSpace == TransWorkSpace);
   // ah, so difficult ...
   return Flags.AllWorkspaces || !UsingTrans || CWorkSpace == TransWorkSpace;
}

int do_wupdate(gpointer data)
{
   if (Flags.Done)
      return FALSE;
   if(!Isdesktop) return TRUE;
   if(Flags.NoKeepSWin) return TRUE;
   long r;
   r = GetCurrentWorkspace();
   if(r>=0) 
      CWorkSpace = r;
   else
   {
      Flags.Done = 1;
      return TRUE;
   }

   if(Windows) free(Windows);

   if (GetWindows(&Windows, &NWindows)<0)
   {
      Flags.Done = 1;
      return TRUE;
   };
   //printwindows(Windows,NWindows);
   // Take care of the situation that the transparent window changes from workspace, 
   // which can happen if in a dynamic number of workspaces environment
   // a workspace is emptied.
   WinInfo *winfo;
   winfo = FindWindow(Windows,NWindows,SnowWin);

   // check also on valid winfo: after toggling 'below'
   // winfo is nil sometimes

   if(UsingTrans && winfo)
   {
      // in xfce and maybe others, workspace info is not to be found
      // in our transparent window. winfo->ws will be 0, and we keep
      // the same value for TransWorkSpace.

      if ((int)winfo->ws > 0)
      {
	 TransWorkSpace = winfo->ws;
      }
      P("TransWorkSpace %#lx %#lx %#lx %#lx\n",TransWorkSpace,winfo->ws,SnowWin,GetCurrentWorkspace());
   }


   UpdateWindows();
   return TRUE;
}
// Have a look at the windows we are snowing on

void UpdateWindows()
{
   typeof(Windows) w;
   typeof(FsnowFirst) f;
   int i;
   // put correct workspace in fallensnow areas
   w = Windows;
   for(i=0; i<NWindows; i++)
   {
      f = FsnowFirst;
      while(f)
      {
	 if(w->id == f->id)
	 {
	    f->ws     = w->ws;
	    f->sticky = w->sticky;
	 }
	 f = f->next;
      }
      w++;
   }
   // add fallensnow regions:
   w = Windows;
   for (i=0; i<NWindows; i++)
   {
      //P("%d %#lx\n",i,w->id);
      {
	 f = FindFallen(FsnowFirst,w->id);
	 if(f)
	 {
	    if ((!f->sticky) && f->ws != CWorkSpace)
	       CleanFallenArea(f,0,f->w);
	 }
	 if (!f)
	 {
	    // window found in Windows, nut not in list of fallensnow,
	    // add it, but not if we are snowing or birding in this window (Desktop for example)
	    // and also not if this window has y <= 0
	    // and also not if it is a very wide window, probably this is a panel then
	    //P("add %#lx %d\n",w->id, RunCounter);
	    //PrintFallenSnow(FsnowFirst);
	    if (w->id != SnowWin && w->y > 0 && w->w < SnowWinWidth -100)
	       PushFallenSnow(&FsnowFirst, w->id, w->ws, w->sticky,
		     w->x+Flags.OffsetX, w->y+Flags.OffsetY, w->w+Flags.OffsetW, 
		     Flags.MaxWinSnowDepth); 
	 }
      }
      w++;
   }
   // remove fallensnow regions
   f = FsnowFirst; int nf = 0; while(f) { nf++; f = f->next; }
   long int *toremove = (long int *)malloc(sizeof(*toremove)*nf);
   int ntoremove = 0;
   f = FsnowFirst;
   Atom wmState = XInternAtom(display, "_NET_WM_STATE", True);
   while(f)
   {
      if (f->id != 0)  // f->id=0: this is the snow at the bottom
      {
	 w = FindWindow(Windows,NWindows,f->id);
	 if(!w)   // this window is gone
	 {
	    GenerateFlakesFromFallen(f,0,f->w,-10.0);
	    toremove[ntoremove++] = f->id;
	 }

	 // test if f->id is hidden. If so: clear the area and notify in f
	 Atom type; int format; unsigned long n, b; unsigned char *properties = 0;
	 XGetWindowProperty(display, f->id, wmState, 0, (~0L), False, AnyPropertyType, 
	       &type, &format, &n, &b, &properties);
	 f->hidden = 0;
	 if(format == 32)
	 {
	    unsigned long i;
	    for (i=0; i<n; i++)
	    {
	       char *s = 0;
	       s = XGetAtomName(display,((Atom*)properties)[i]);
	       if (!strcmp(s,"_NET_WM_STATE_HIDDEN"))
	       { 
		  //P("%#lx is hidden %d\n",f->id, RunCounter);
		  f->hidden = 1;
		  CleanFallenArea(f,0,f->w);
		  if(s) XFree(s);
		  break;
	       }
	       if(s) XFree(s);
	    }
	 }
	 if(properties) XFree(properties);
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
	    toremove[ntoremove++] = f->id;
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

int DetermineWindow()
{
   P("DetermineWindow\n");
   if (Flags.WindowId)
   {
      SnowWin = Flags.WindowId;
      Isdesktop = 0;
      UseAlpha  = 0;
   }
   else
   {
      if (Flags.XWinInfoHandling)
      {
	 SnowWin = XWinInfo(&SnowWinName);
	 if (SnowWin == 0)
	 {
	    fprintf(stderr,"XWinInfo failed\n");
	    exit(1);
	 }
	 Isdesktop = 0;
	 UseAlpha  = 0;
      }
      else
      {
	 char *desktopsession = 0;
	 const char *desktops[] = {
	    "DESKTOP_SESSION",
	    "XDG_SESSION_DESKTOP",
	    "XDG_CURRENT_DESKTOP",
	    "GDMSESSION",
	    0
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

	 if (DesktopSession)
	    free(DesktopSession);
	 DesktopSession = strdup(desktopsession);

	 // convert DesktopSession to upper case
	 char *a = DesktopSession;
	 while (*a)
	 {
	    *a = toupper(*a);
	    a++;
	 }
	 IsCompiz = (strstr(DesktopSession,"COMPIZ") != 0);
	 int lxdefound = 0;
	 // if envvar DESKTOP_SESSION == LXDE, search for window with name pcmanfm
	 if (DesktopSession != NULL && !strncmp(DesktopSession,"LXDE",4))
	 {
	    lxdefound = FindWindowWithName("pcmanfm",&SnowWin,&SnowWinName);
	    printf("LXDE session found, searching for window 'pcmanfm'\n");
	 }
	 if(lxdefound)
	 {
	    UseAlpha  = 0;
	    Isdesktop = 1;
	    Exposures = 0;
	 }
	 else
	 {

	    P("DetermineWindow\n");
	    int x,y;
	    unsigned int w,h,b,depth;
	    Window root;
	    XGetGeometry(display,RootWindow,&root,
		  &x, &y, &w, &h, &b, &depth);
	    if(SnowWinName) free(SnowWinName);

	    if (GtkWinb)
	    {
	       gtk_window_close(GTK_WINDOW(GtkWinb));
	       gtk_widget_destroy(GTK_WIDGET(GtkWinb));
	    }
	    create_transparent_window(Flags.FullScreen, Flags.BelowAll, Flags.AllWorkspaces, 
		  &SnowWin, "Birds-Window", &SnowWinName, &GtkWinb,w,h);
	    R("birds window %ld %p\n",SnowWin,(void *)GtkWinb);
	    if (GtkWinb)
	    {
	       UseGtk = 1;
	       restart_do_draw_all();  // to (re-)establish the timeout for do_draw_all
	    }
	    else
	    {
	       UseGtk = 0;
	       printf("Your screen does not support alpha channel, no birds will fly.\n");
	    }
	    //create_transparent_window(Flags.FullScreen, Flags.BelowAll, Flags.AllWorkspaces, 
	    //	  &SnowWin, "Xsnow-Window", &SnowWinName, &GtkWin,w,h);
	    //   P("snow window %ld %p\n",SnowWin,(void *)GtkWin);

	    Isdesktop = 1;
	    UseAlpha  = 1;
	    XGetGeometry(display,SnowWin,&root,
		  &x, &y, &w, &h, &b, &depth);
	    P("depth: %d snowwin: 0x%lx %s\n",depth,SnowWin,SnowWinName);
	    if(SnowWin)
	    {
	       TransWorkSpace = GetCurrentWorkspace();
	       UsingTrans     = 1;

	       drawing_area = gtk_drawing_area_new();
	       gtk_container_add(GTK_CONTAINER(GtkWinb), drawing_area);

	       g_signal_connect(GtkWinb, "draw", G_CALLBACK(on_draw_event), NULL);

	       birds_init();
	    }
	    else
	    {
	       // snow in root window:
	       P("snow in root window\n");
	       Isdesktop = 1;
	       UseAlpha  = 0;
	       if(SnowWinName) free(SnowWinName);
	       SnowWin     = DefaultRootWindow(display);
	       SnowWinName = strdup("No Name");
	    }
	 }
      }
   }
   // override Isdesktop if user desires so:
   if (Flags.Desktop)
      Isdesktop = 1;
   P("Isdesktop: %d\n",Isdesktop);
   if(Isdesktop) 
      CWorkSpace = GetCurrentWorkspace();
   P("CWorkSpace: %ld\n",CWorkSpace);
   if (CWorkSpace < 0)
      return 0;
   InitDisplayDimensions();
   // if depth != 32, we assume that the desktop is not gnome-like TODO
   if (Isdesktop && SnowWinDepth != 32)
      UseAlpha = 0;
   // override UseAlpha if user desires so:
   if (Flags.UseAlpha != SOMENUMBER)
      UseAlpha = Flags.UseAlpha;

   return 1;
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
   unsigned int w,h,b,d;
   int x,y;
   unsigned int wroot,hroot,broot,droot;
   int xroot,yroot;
   Window root;
   XGetGeometry(display,RootWindow,&root,
	 &xroot, &yroot, &wroot, &hroot, &broot, &droot);
   Xroot = xroot;
   Yroot = yroot;
   Wroot = wroot;
   Hroot = hroot;
   P("InitDisplayDimensions: %d %d %d %d %d %d\n",xroot,yroot,wroot,hroot,broot,droot);
   XGetGeometry(display,SnowWin,&root,
	 &x, &y, &w, &h, &b, &d);
   P("InitDisplayDimensions: %d %d %d %d %d %d\n",x,y,w,h,b,d);
   SnowWinX           = x;
   SnowWinY           = y;
   SnowWinWidth       = w;
   if(UsingTrans)
      SnowWinHeight      = hroot + Flags.OffsetS;
   else
      SnowWinHeight      = h + Flags.OffsetS;
   if(!Flags.FullScreen && UsingTrans)
      SnowWinHeight -= y;
   SnowWinBorderWidth = b;
   SnowWinDepth       = d;

   SetMaxScreenSnowDepth();
}

