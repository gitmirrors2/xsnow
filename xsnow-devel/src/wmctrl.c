/* -copyright-
#-#
#-# xsnow: let it snow on your desktop
#-# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#-#               2019 Willem Vermin
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
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <string.h>
#include <unistd.h>
#include "wmctrl.h"
#include "windows.h"
#include "dsimple.h"
int GetCurrentWorkspace()
{
   Atom atom, type;
   int format;
   unsigned long nitems,b;
   unsigned char *properties = 0;
   long r;
   atom = XInternAtom(display,"_NET_CURRENT_DESKTOP",False);
   XGetWindowProperty(display, DefaultRootWindow(display), atom, 0, 1, False, 
	 AnyPropertyType, &type, &format, &nitems, &b, &properties);
   if(type != XA_CARDINAL)
   {
      //printf("%d: nog eens ...\n",__LINE__);
      if(properties) XFree(properties);
      atom = XInternAtom(display,"_WIN_WORKSPACE",False);
      XGetWindowProperty(display, DefaultRootWindow(display), atom, 0, 1, False, 
	    AnyPropertyType, &type, &format, &nitems, &b, &properties);
   }
   if(type != XA_CARDINAL)
      r = -1;
   else
      r = *(long *)properties;
   //printf("wmctrl: %d: %ld %ld\n",__LINE__,nitems,r);
   if(properties) XFree(properties);
   return (int)r;
}

int GetWindows(WinInfo **windows, int *nwin)
{
   Atom atom, type;
   int format;
   unsigned long nitems,b;
   unsigned char *properties = 0;
   long *r;
   (*windows) = 0;
   atom = XInternAtom(display,"_NET_CLIENT_LIST",False);
   XGetWindowProperty(display, DefaultRootWindow(display), atom, 0, 1000000, False, 
	 AnyPropertyType, &type, &format, &nitems, &b, &properties);
   if(type != XA_WINDOW)
   {
      printf("%d: nog eens ...\n",__LINE__);
      if(properties) XFree(properties);
      atom = XInternAtom(display,"_WIN_CLIENT_LIST",False);
      XGetWindowProperty(display, DefaultRootWindow(display), atom, 0, 1000000, False, 
	    AnyPropertyType, &type, &format, &nitems, &b, &properties);
   }
   //printf("wmctrl: %d: %ld\n",__LINE__,nitems);
   (*nwin) = nitems;
   r = (long*)properties;
   (*windows) = malloc(nitems*sizeof(WinInfo));
   int i;
   WinInfo *w = (*windows);
   static Atom net_atom = 0, gtk_atom = 0;
   if(net_atom == 0) net_atom = XInternAtom(display, "_NET_FRAME_EXTENTS", True);
   if(gtk_atom == 0) gtk_atom = XInternAtom(display, "_GTK_FRAME_EXTENTS", True);
   for (i=0; i<nitems; i++,w++)
   {
      Window root,child_return;
      int x0,y0;
      unsigned int bw,depth;
      w->id = r[i];
      XGetGeometry (display, w->id, &root, &x0, &y0,
	    &(w->w), &(w->h), &bw, &depth);
      XTranslateCoordinates (display, w->id, SnowWin, 0, 0,
	    &(w->x), &(w->y), &child_return);

      enum{NET,GTK};
      Atom type; int format; unsigned long nitems,b; unsigned char *properties = 0;
      Atom atom;
      atom = XInternAtom(display,"_NET_WM_DESKTOP",False);
      XGetWindowProperty(display, w->id, atom, 0, 1, False, 
	    AnyPropertyType, &type, &format, &nitems, &b, &properties);
      if(type != XA_CARDINAL)
      {
	 if(properties) XFree(properties);
	 properties = 0;
	 atom = XInternAtom(display,"_WIN_WORKSPACE",False);
	 XGetWindowProperty(display, w->id, atom, 0, 1, False, 
	       AnyPropertyType, &type, &format, &nitems, &b, &properties);
      }
      if(properties)
      {
	 w->ws = *(long*) properties;
	 if(properties) XFree(properties);
      }
      else
	 w->ws = 0;
      // maybe this window is sticky:
      w->sticky = 0;
      properties = 0;
      nitems = 0;
      atom = XInternAtom(display,"_NET_WM_STATE",True);
      XGetWindowProperty(display, w->id, atom, 0, (~0L), False,
	    AnyPropertyType, &type, &format, &nitems, &b, &properties);
      if (type == XA_ATOM)
      {
	 int i;
	 for(i=0; i<nitems; i++)
	 {
	    char *s = 0;
	    s = XGetAtomName(display,((Atom*)properties)[i]);
	    if (!strcmp(s,"_NET_WM_STATE_STICKY"))
	    { 
	       //printf("%d: %#lx is sticky\n",__LINE__,w->id);
	       w->sticky = 1;
	       if(s) XFree(s);
	       break;
	    }
	    if(s) XFree(s);
	 }
      }
      if(properties) XFree(properties);
      properties = 0;
      nitems = 0;

      XGetWindowProperty(display, w->id, net_atom, 0, 4, False, 
	    AnyPropertyType, &type, &format, &nitems, &b, &properties);
      int wintype = NET;
      if (nitems != 4)
      {
	 if(properties) XFree(properties);
	 properties = 0;
	 //printf("%d: trying gtk...\n",__LINE__);
	 XGetWindowProperty(display, w->id, gtk_atom, 0, 4, False, 
	       AnyPropertyType, &type, &format, &nitems, &b, &properties);
	 wintype = GTK;
      }
      //printf("%d: nitems: %ld %ld %d\n",__LINE__,type,nitems,format);
      if(nitems == 4 && format == 32 && type) // adjust x,y,w,h of window
      {
	 long *r; // borderleft, borderright, top decoration, bottomdecoration
	 r = (long*)properties;
	 //printf("%d: RRRR: %ld %ld %ld %ld\n",__LINE__,r[0],r[1],r[2],r[3]);
	 switch(wintype)
	 {
	    case NET:
	       //printf("%d: NET\n",__LINE__);
	       w->x -= r[0];
	       w->y -= r[2];
	       w->w += r[0]+r[1];
	       w->h += r[2]+r[3];
	       break;
	    case GTK:
	       //printf("%d: GTK\n",__LINE__);
	       w->x += r[0];
	       w->y += r[2];
	       w->w -= (r[0]+r[1]);
	       w->h -= (r[2]+r[3]);
	       break;
	    default:
	       //printf("%s:%d: dit kan niet\n",__FILE__,__LINE__);
	       exit(1);
	       break;
	 }
	 //printf("%d: NET/GTK: %#lx %d %d %d %d %d\n",__LINE__,
	 //      w->id,w->ws,w->x,w->y,w->w,w->h);
      }
      if(properties)XFree(properties);
   }
   if(properties) XFree(properties);
   return 1;
}

int FindWindowWithName(char *needle, Window *win, char **name)
{
   *win = Window_With_Name(display,DefaultRootWindow(display),needle);
   (*name) = strdup(needle);
   if (*win == 0)
      return 0;
   else
      return 1;

}

WinInfo *FindWindow(WinInfo *windows, int nwin, Window id)
{
   WinInfo *w = windows;
   int i;
   for (i=0; i<nwin; i++)
   {
      if (w->id == id)
	 return w;
      w++;
   }
   return 0;
}

void printwindows(WinInfo *windows, int nwin)
{
   WinInfo *w = windows;
   int i;
   for (i=0; i<nwin; i++)
   {
      printf("id:%#lx ws:%d x:%d y:%d w:%d h:%d sticky:%d\n",w->id,w->ws,w->x,w->y,w->w,w->h,w->sticky);
      w++;
   }
   return;
}

