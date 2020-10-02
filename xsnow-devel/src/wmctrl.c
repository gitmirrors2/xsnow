/* -copyright-
#-# 
#-# xsnow: let it snow on your desktop
#-# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#-# 	      2019,2020 Willem Vermin
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
#include "debug.h"

int GetCurrentWorkspace()
{
   Atom atom, type;
   int format;
   unsigned long nitems,b;
   unsigned char *properties;
   int r;

   P("GetCurrentWorkspace %p %d\n",(void *)display,counter++);
   if (IsCompiz)
   {
      P("compiz\n");
      properties = NULL;
      atom = XInternAtom(display,"_NET_DESKTOP_VIEWPORT",False);
      XGetWindowProperty(display, DefaultRootWindow(display), atom, 0, 2, False, 
	    AnyPropertyType, &type, &format, &nitems, &b, &properties);
      if (type != XA_CARDINAL || nitems != 2)
      {
	 r = -1;
      }
      else
      {
	 // we have the x-y coordinates of the workspace, we hussle this
	 // into one long number:
	 r = ((long *)properties)[0]+(((long *)properties)[1]<<16);
      }
      if(properties) XFree(properties);
   }
   else
   {
      properties = NULL;
      atom = XInternAtom(display,"_NET_CURRENT_DESKTOP",False);
      XGetWindowProperty(display, DefaultRootWindow(display), atom, 0, 1, False, 
	    AnyPropertyType, &type, &format, &nitems, &b, &properties);
      P("type: %ld %ld\n",type,XA_CARDINAL);
      P("properties: %d %d %d %ld\n",properties[0],properties[1],format,nitems);
      if(type != XA_CARDINAL)
      {
	 P("nog eens %ld ...\n",type);
	 if(properties) XFree(properties);
	 atom = XInternAtom(display,"_WIN_WORKSPACE",False);
	 XGetWindowProperty(display, DefaultRootWindow(display), atom, 0, 1, False, 
	       AnyPropertyType, &type, &format, &nitems, &b, &properties);
      }
      if(type != XA_CARDINAL)
      {
	 if (IsWayland)
	    // in Wayland, the actual number of current workspace can only
	    // be obtained if user has done some workspace-switching
	    // we return zero if the workspace number cannot be determined

	    r = 0;
	 else
	    r = -1;
	 r = 0; // second thought: always return 0 here
	 //        so things will run in enlightenment also
      }
      else
	 r = *(long *)properties;        // see man XGetWindowProperty
      if(properties) XFree(properties);
   }
   P("wmctrl: nitems: %ld ws: %d\n",nitems,r);

   return r;
}


int GetWindows(WinInfo **windows, int *nwin)
{
   Atom atom, type;
   int format;
   unsigned long nitems,b;
   unsigned char *properties = NULL;
   long *r;
   (*windows) = NULL;
   atom = XInternAtom(display,"_NET_CLIENT_LIST",False);
   XGetWindowProperty(display, DefaultRootWindow(display), atom, 0, 1000000, False, 
	 AnyPropertyType, &type, &format, &nitems, &b, &properties);
   if(type != XA_WINDOW)
   {
      //printf("%d: nog eens ...\n",__LINE__);
      if(properties) XFree(properties);
      atom = XInternAtom(display,"_WIN_CLIENT_LIST",False);
      XGetWindowProperty(display, DefaultRootWindow(display), atom, 0, 1000000, False, 
	    AnyPropertyType, &type, &format, &nitems, &b, &properties);
   }
   //printf("wmctrl: %d: %ld\n",__LINE__,nitems);
   (*nwin) = nitems;
   r = (long*)properties;
   (*windows) = (WinInfo *)malloc(nitems*sizeof(WinInfo));
   int i;
   WinInfo *w = (*windows);
   static Atom net_atom = 0, gtk_atom = 0;
   if(gtk_atom == 0) gtk_atom = XInternAtom(display, "_GTK_FRAME_EXTENTS", True);
   if(net_atom == 0) net_atom = XInternAtom(display, "_NET_FRAME_EXTENTS", True);
   for (i=0; (unsigned long)i<nitems; i++,w++)
   {
      Window root,child_return;
      int x0,y0,xr,yr;
      unsigned int bw,depth;
      w->id = r[i];
      XGetGeometry (display, w->id, &root, &x0, &y0,
	    &(w->w), &(w->h), &bw, &depth);
      XTranslateCoordinates(display, w->id, RootWindow, 0, 0, &xr,     &yr,     &child_return);
      w->xa = xr - x0;
      w->ya = yr - y0;

      XTranslateCoordinates(display, w->id, SnowWin,    0, 0, &(w->x), &(w->y), &child_return);

      enum{NET,GTK};
      Atom type; int format; unsigned long nitems,b; unsigned char *properties = NULL;
      Atom atom;
      atom = XInternAtom(display,"_NET_WM_DESKTOP",False);
      XGetWindowProperty(display, w->id, atom, 0, 1, False, 
	    AnyPropertyType, &type, &format, &nitems, &b, &properties);
      if(type != XA_CARDINAL)
      {
	 if(properties) XFree(properties);
	 properties = NULL;
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
      properties = NULL;
      nitems = 0;
      atom = XInternAtom(display,"_NET_WM_STATE",True);
      XGetWindowProperty(display, w->id, atom, 0, (~0L), False,
	    AnyPropertyType, &type, &format, &nitems, &b, &properties);
      if (type == XA_ATOM)
      {
	 int i;
	 for(i=0; (unsigned long)i<nitems; i++)
	 {
	    char *s = NULL;
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
      // another sticky test, needed in KDE en LXDE:
      if (w->ws == -1)
	 w->sticky = 1;
      if(properties) XFree(properties);

      // check if window is a "dock". 
      w->dock = 0;
      properties = NULL;
      nitems = 0;
      atom = XInternAtom(display,"_NET_WM_WINDOW_TYPE", True);
      XGetWindowProperty(display, w->id, atom, 0, (~0L), False, 
	    AnyPropertyType, &type, &format, &nitems, &b, &properties);
      if(format == 32)
      {
	 int i;
	 for(i=0; (unsigned long)i<nitems; i++)
	 {
	    char *s = NULL;
	    s = XGetAtomName(display,((Atom*)properties)[i]);
	    if (!strcmp(s,"_NET_WM_WINDOW_TYPE_DOCK"))
	    { 
	       P("%#lx is dock %d\n",w->id, counter++);
	       w->dock = 1;
	       if(s) XFree(s);
	       break;
	    }
	    if(s) XFree(s);
	 }
      }
      if(properties) XFree(properties);

      // check if window is hidden
      w->hidden = 0;
      properties = NULL;
      nitems = 0;
      atom  = XInternAtom(display, "_NET_WM_STATE", True);
      XGetWindowProperty(display, w->id, atom, 0, (~0L), False, 
	    AnyPropertyType, &type, &format, &nitems, &b, &properties);
      if(format == 32)
      {
	 unsigned long i;
	 for (i=0; i<nitems; i++)
	 {
	    char *s = NULL;
	    s = XGetAtomName(display,((Atom*)properties)[i]);
	    if (!strcmp(s,"_NET_WM_STATE_HIDDEN"))
	    { 
	       P("%#lx is hidden %d\n",f->id, counter++);
	       w->hidden = 1;
	       if(s) XFree(s);
	       break;
	    }
	    if(s) XFree(s);
	 }
      }
      if(properties) XFree(properties);


      properties = NULL;
      nitems = 0;

      // first try to get adjustments for _GTK_FRAME_EXTENTS
      if (gtk_atom)
	 XGetWindowProperty(display, w->id, gtk_atom, 0, 4, False, 
	       AnyPropertyType, &type, &format, &nitems, &b, &properties);
      int wintype = GTK;
      // if not succesfull, try _NET_FRAME_EXTENTS
      if (nitems != 4)
      {
	 if(properties) XFree(properties);
	 properties = NULL;
	 //printf("%d: trying net...\n",__LINE__);
	 XGetWindowProperty(display, w->id, net_atom, 0, 4, False, 
	       AnyPropertyType, &type, &format, &nitems, &b, &properties);
	 wintype = NET;
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
   //P("%d\n",counter++);printwindows(display,*windows,*nwin);
   return 1;
}

// an heroic effort to write a wrapper around XGetWindowProperty()
//     not used (yet)
// if needle != NULL:
//     returns 1 if Atom atomname contains char *needle
//     else returns 0
// if needle == NULL && props != NULL
//     props must be allocated by caller to contain nprops elements
//     on return, props will contain at most nprops integers as obtained by 
//     XGetWindowProperty().
//     return value is number of properties returned from XGetWindowProperty()
// works only with format==32, if another format is found, returns -format found
// see man XGetWindowProperty
int GetProperty32(Display *display, Window window, const char *atomname, 
      const char *needle, int *props, const int nprops)
{
   unsigned char *properties = NULL;
   Atom           type;
   int            format;
   unsigned long  nitems = 0;
   unsigned long  b;
   int            rc = 0;

   Atom atom = XInternAtom(display, atomname, True);
   XGetWindowProperty(display, window, atom, 0, (~0L), False, 
	 AnyPropertyType, &type, &format, &nitems, &b, &properties);
   if(format == 32)
   {
      if (needle)
      {
	 int i;
	 for(i=0; i<(int)nitems; i++)
	 {
	    char *s = NULL;
	    s = XGetAtomName(display,((Atom*)properties)[i]);
	    if (!strcmp(s,needle))
	    { 
	       if(s) XFree(s);
	       rc = 1;
	       break;
	    }
	    if(s) XFree(s);
	 }
      }
      else
      {
	 if(props)
	 {
	    int i;
	    for (i=0; i<(int)nitems && i<nprops; i++)
	       props[i] = ((long *)properties)[i];
	 }
	 rc = nitems;
      }
   }
   else
      rc = -format;

   if(properties) XFree(properties);
   return rc;
}

int FindWindowWithName(const char *needle, Window *win, char **name)
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
   return NULL;
}

void printwindows(Display *dpy,WinInfo *windows, int nwin)
{
   WinInfo *w = windows;
   int i;
   for (i=0; i<nwin; i++)
   {
      char *name;
      XFetchName(dpy, w->id, &name);
      if (strlen(name)>20)
	 name[20] = '\0';
      printf("id:%#10lx ws:%3d x:%6d y:%6d xa:%6d ya:%6d w:%6d h:%6d sticky:%d dock:%d hidden:%d name:%s\n",
	    w->id,w->ws,w->x,w->y,w->xa,w->ya,w->w,w->h,w->sticky,w->dock,w->hidden,name);
      XFree(name);
      w++;
   }
   return;
}

