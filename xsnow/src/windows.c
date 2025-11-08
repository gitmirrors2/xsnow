/* 
   -copyright-
# xsnow: let it snow on your desktop
# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#              2019,2020,2021,2022,2023,2024 Willem Vermin
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# 
#-endcopyright-
*/

#include <pthread.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Intrinsic.h>
#include <X11/extensions/Xinerama.h>
#include <ctype.h>
#include <assert.h>

#include "xsnow-constants.h"

#include "mygettext.h"
#include "debug.h"
#include "windows.h"
#include "flags.h"
#include "utils.h"
#include "xsnow.h"
#include "wmctrl.h"
#include "fallensnow.h"
#include "transwindow.h"
#include "dsimple.h"
#include "xdo.h"
#include "scenery.h"

static int    do_sendevent(void *);
static long   TransWorkSpace = -SOMENUMBER;  // workspace on which transparent window is placed

static WinInfo      *Windows = NULL;
static int          NWindows;
static int          do_wupdate(void *);
static void         DetermineVisualWorkspaces(void);


void windows_ui()
{
}

void windows_draw()
{
   // nothing to draw
}

void windows_init()
{
   if (global.Desktop)
   {
      DetermineVisualWorkspaces();
      add_to_mainloop(PRIORITY_DEFAULT, time_wupdate, do_wupdate);
   }
   if (!global.IsDouble)
      add_to_mainloop(PRIORITY_DEFAULT, time_sendevent, do_sendevent);
}

int WorkspaceActive()
{
   P("global.Trans etc %d %d %d %d\n",Flags.AllWorkspaces,global.Trans,global.CWorkSpace == TransWorkSpace,
	 Flags.AllWorkspaces || !global.Trans || global.CWorkSpace == TransWorkSpace);
   // ah, so difficult ...
   if (Flags.AllWorkspaces)
      return 1;
   for (int i=0; i<global.NVisWorkSpaces; i++)
   {
      if (global.VisWorkSpaces[i] == global.ChosenWorkSpace)
	 return 1;
   }
   return 0;
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
   static long PrevWorkSpace = -123;
   P("do_wupdate #%d %d\n",global.counter++,global.WindowsChanged);
   if (Flags.Done)
      return FALSE;

   if(Flags.NoKeepSWin) return TRUE;

   static int lockcounter = 0;
   if (Lock_fallen_n(3,&lockcounter))
   {
      P("lock counter: %d\n",lockcounter);
      return TRUE;
   }
   P("do_wupdate running %d\n",lockcounter);

   // once in a while, we force updating windows
   static int wcounter = 0;
   wcounter++;
   if (wcounter > 9)
   {
      global.WindowsChanged = 1;
      wcounter = 0;
   }
   if (!global.WindowsChanged)
      goto end;

   global.WindowsChanged = 0;

   long r;
   r = GetCurrentWorkspace();
   if(r>=0) 
   {
      global.CWorkSpace = r;
      if (r != PrevWorkSpace)
      {
	 P("workspace changed from %ld to %ld\n",PrevWorkSpace,r);
	 PrevWorkSpace = r;
	 DetermineVisualWorkspaces();
      }
   }
   else
   {
      I("Cannot get current workspace\n");
      Flags.Done = 1;
      goto end;
   }

   P("Update windows\n");

   if(Windows) 
      free(Windows);

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
      goto end;
   }

   for (int i=0; i<NWindows; i++)
   {
      WinInfo *w = &Windows[i];
      P("SnowWinX SnowWinY: %d %d\n",global.SnowWinX,global.SnowWinY);
      w->x += global.WindowOffsetX-global.SnowWinX;
      w->y += global.WindowOffsetY-global.SnowWinY;
   }


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
      P("TransWorkSpace %ld %#lx %#lx %ld\n",TransWorkSpace,winfo->ws,global.SnowWin,GetCurrentWorkspace());
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

end:
   Unlock_fallen();
   return TRUE;
   (void)dummy;
}

void DetermineVisualWorkspaces()
{
   P("%d Entering DetermineVisualWorkspaces\n",global.counter++);
   static Window ProbeWindow = 0;
   static XClassHint class_hints;
   static XSetWindowAttributes attr;
   static long valuemask;
   static long hints[5] = {2 , 0, 0, 0, 0};
   static Atom motif_hints;
   static XSizeHints wmsize;

   if (!global.Desktop)
   {
      global.NVisWorkSpaces   = 1;
      global.VisWorkSpaces[0] = global.CWorkSpace;
      return;
   }

   if (ProbeWindow)
   {
      XDestroyWindow(global.display, ProbeWindow);
   }
   else
   {
      P("Creating attrs for ProbeWindow\n");
      attr.background_pixel = WhitePixel(global.display, global.Screen);
      attr.border_pixel     = WhitePixel(global.display, global.Screen);
      attr.event_mask       = ButtonPressMask;
      valuemask             = CWBackPixel | CWBorderPixel | CWEventMask;
      class_hints.res_name  = (char *)"xsnow";
      class_hints.res_class = (char *)"Xsnow";
      motif_hints = XInternAtom(global.display, "_MOTIF_WM_HINTS", False);
      wmsize.flags = USPosition | USSize;
   }

   int number;
   XineramaScreenInfo *info = XineramaQueryScreens(global.display,&number);
   if (number == 1 || info == NULL)
   {
      global.NVisWorkSpaces = 1;
      global.VisWorkSpaces[0] = global.CWorkSpace;
      return;
   }


   // This is for bspwm and possibly other tiling window magagers.
   //
   // Determine which workspaces are visible: place a window (ProbeWindow)
   // in each xinerama screen, and ask in which workspace the window
   // is located.

   //int prevsticky = set_sticky(1);

   ProbeWindow = XCreateWindow (global.display, global.Rootwindow,
	 1,1,1,1,10,
	 DefaultDepth(global.display, global.Screen), InputOutput,
	 DefaultVisual(global.display, global.Screen),valuemask,&attr);
   XSetClassHint(global.display,ProbeWindow,&class_hints);

   // to prevent the user to determine the intial position (in twm for example)
   XSetWMNormalHints(global.display, ProbeWindow, &wmsize);

   XChangeProperty(global.display, ProbeWindow, motif_hints, motif_hints, 
	 32, PropModeReplace, (unsigned char *)&hints, 5);
   xdo_map_window(global.xdo,ProbeWindow);

   global.NVisWorkSpaces = number;
   int prev = -SOMENUMBER;
   for (int i=0; i<number; i++)
   {
      int x = info[i].x_org;
      int y = info[i].y_org;
      int w = info[i].width;
      int h = info[i].height;

      // place ProbeWindow in the center of xinerama screen[i]

      int xm = x+w/2;
      int ym = y+h/2;
      P("movewindow: %d %d\n",xm,ym);
      xdo_move_window(global.xdo,ProbeWindow,xm,ym);
      xdo_wait_for_window_map_state(global.xdo,ProbeWindow,IsViewable);
      long desktop;
      int rc = xdo_get_desktop_for_window(global.xdo,ProbeWindow,&desktop);
      if (rc == XDO_ERROR)
	 desktop = global.CWorkSpace;
      P("desktop: %ld rc: %d\n",desktop,rc);
      global.VisWorkSpaces[i] = desktop;

      if (desktop != prev)
      {
	 // this is for the case that the xinerama screens belong to different workspaces,
	 // as seems to be the case in e.g. bspwm
	 if (prev >= 0)
	 {
	    global.WindowOffsetX = 0;
	    global.WindowOffsetY = 0;
	 }
	 prev = desktop;
      }
   }
   xdo_unmap_window(global.xdo,ProbeWindow);
}


void UpdateFallenSnowRegionsWithLock()
{
   Lock_fallen();
   UpdateFallenSnowRegions();
   Unlock_fallen();
}

// Have a look at the windows we are snowing on
// Also update of fallensnow area's
void UpdateFallenSnowRegions()
{
   // threads: locking by caller
   WinInfo *w;
   FallenSnow *fsnow;
   // add fallensnow regions:
   w = Windows;
   for (int i=0; i<NWindows; i++)
   {
      //P("%d %#lx\n",i,w->id);
      {
	 fsnow = FindFallen(global.FsnowFirst,w->id);
	 P("%#lx %d\n",w->id,w->dock);
	 if(fsnow)
	 {
	    fsnow->win = *w;   // update window properties
	    if ((!fsnow->win.sticky) && fsnow->win.ws != global.CWorkSpace)
	    {
	       P("CleanFallenArea\n");
	       CleanFallenArea(fsnow,0,fsnow->w);
	    }
	 }
	 if (!fsnow)
	 {
	    // window found in Windows, nut not in list of fallensnow,
	    // add it, but not if we are snowing or birding in this window (Desktop for example)
	    // and also not if this window has y <= 0
	    // and also not if this window is a "dock"
	    // and also not if this window is "hidden"
	    // and also not if this window is to be ignored
	    P("               %#lx %d %d %d\n",w->id,w->dock,w->w,w->ignore);
	    if (w->id != global.SnowWin && w->y > 0 && !(w->dock) && !(w->hidden) && !(w->ignore)) 
	    {
	       P("dock? %#lx %d %d %d\n",w->id,w->dock,w->hidden,w->w);
	       if(((int)(w->w) == global.SnowWinWidth && w->x == 0 && w->y <100)) //maybe a transparent xpenguins window?
	       {
		  P("skipping: #%d %#lx %d %d %d\n",global.counter++, w->id, w->w, w->x, w->y);
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
   fsnow = global.FsnowFirst; 
   int nf = 0; 
   while(fsnow) 
   { 
      nf++; 
      fsnow = fsnow->next; 
   }
   // nf+1: prevent allocation of zero bytes
   long int *toremove = (long int *)malloc(sizeof(*toremove)*(nf+1));
   assert(toremove);
   int ntoremove = 0;
   fsnow = global.FsnowFirst;
   while(fsnow)
   {
      if (fsnow->win.id != 0)  // fsnow->id=0: this is the snow at the bottom
      {
	 w = FindWindow(Windows,NWindows,fsnow->win.id);
	 if(
	       !w                                                       // this window is gone
	       || ( w->w > 0.8*global.SnowWinWidth 
		  && w->ya < Flags.IgnoreTop)                           // too wide&too close to top   
	       || ( w->w > 0.8*global.SnowWinWidth 
		  && (int)global.SnowWinHeight - w->ya < Flags.IgnoreBottom) // too wide&too close to bottom
	   )
	 {
	    P("Gone...\n");
	    GenerateFlakesFromFallen(fsnow,0,fsnow->w,-10.0,0.15,1);
	    toremove[ntoremove++] = fsnow->win.id;
	 }

	 // test if fsnow->win.id is hidden. If so: clear the area and notify in fsnow
	 // we have to test that here, because the hidden status of the window
	 // can change
	 P("%#lx hidden:%d\n",fsnow->win.id,fsnow->win.hidden);
	 if (fsnow->win.hidden)
	 {
	    P("%#lx is hidden %d\n",fsnow->win.id, global.counter++);
	    if(global.DoCapella)
	    {
	       GenerateFlakesFromFallen(fsnow,0,fsnow->w,-10.0,0.15,1);
	       toremove[ntoremove++] = fsnow->win.id;
	    }
	    else
	    {
	       CleanFallenArea(fsnow,0,fsnow->w);
	    }
	    P("CleanFallenArea\n");
	 }
      }
      fsnow = fsnow->next;
   }

   // test if window has been moved or resized
   // moved: move fallen area accordingly
   // resized: remove fallen area: add it to toremove
   w = Windows;
   for (int i=0; i<NWindows; i++)
   {
      fsnow = FindFallen(global.FsnowFirst,w->id);
      if (fsnow)
      {
	 if ((unsigned int)fsnow->w == w->w+Flags.OffsetW) // width has not changed
	 {
	    if (fsnow->x != w->x + Flags.OffsetX || fsnow->y != w->y + Flags.OffsetY)
	    {
	       if (global.DoCapella)
	       {
		  GenerateFlakesFromFallen(fsnow,0,fsnow->w,-10.0,0.15,1);
		  toremove[ntoremove++] = fsnow->win.id;
	       }
	       else
	       {
		  CleanFallenArea(fsnow,0,fsnow->w);
		  P("CleanFallenArea\n");
		  fsnow->x = w->x + Flags.OffsetX;
		  fsnow->y = w->y + Flags.OffsetY;
		  XFlush(global.display);
	       }
	    }
	 }
	 else
	 {
	    if(global.DoCapella)
	       GenerateFlakesFromFallen(fsnow,0,fsnow->w,-10.0,0.15,1);
	    toremove[ntoremove++] = fsnow->win.id;
	 }
      }
      w++;
   }

   for (int i=0; i<ntoremove; i++)
   {
      CleanFallen(toremove[i]);
      RemoveFallenSnow(&global.FsnowFirst,toremove[i]);
   }
   free(toremove);
}



Window XWinInfo(char **name)
   // not used
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

// gets location and size of xinerama screen xscreen, -1: full screen
// returns the number of xinerama screens
int xinerama(Display *display, int xscreen, int *x, int *y, int *w, int *h)
{
   int number;
   XineramaScreenInfo *info = XineramaQueryScreens(display,&number);
   if (info == NULL)
   {
      I("No xinerama...\n");
      return 0;
   }
   else
   {
      int scr = xscreen;
      if(scr > number-1)
	 scr = number-1;

      for (int i=0; i<number; i++)
      {
	 P("number: %d\n",info[i].screen_number);
	 P("   x_org:  %d\n",info[i].x_org);
	 P("   y_org:  %d\n",info[i].y_org);
	 P("   width:  %d\n",info[i].width);
	 P("   height: %d\n",info[i].height);
      }

      if (scr < 0)
      {
	 // set x,y to 0,0
	 // set width and height to maximum values found
	 if(x)
	    *x = 0;
	 if(y)
	    *y = 0;
	 if(w)
	    *w = 0;
	 if(h)
	    *h = 0;
	 for (int i=0; i<number; i++)
	 {
	    if (w && info[i].width > *w)
	       *w = info[i].width;
	    if (h && info[i].height > *h)
	       *h = info[i].height;
	 }
      }
      else
      {
	 if(x)
	    *x = info[scr].x_org;
	 if(y)
	    *y = info[scr].y_org;
	 if(w)
	    *w = info[scr].width;
	 if(h)
	    *h = info[scr].height;
      }
      P("Xinerama window: %d+%d %dx%d\n",*x,*y,*w,*h);

      XFree(info);
   }
   return number;
}

void InitDisplayDimensions()
{
   unsigned int w,h;
   int x,y;
   xdo_get_window_location(global.xdo, global.Rootwindow, &x, &y,NULL);
   xdo_get_window_size    (global.xdo, global.Rootwindow, &w, &h);

   P("InitDisplayDimensions root: %p %d %d %d %d\n",(void*)global.Rootwindow,x,y,w,h);

   global.Xroot = x;
   global.Yroot = y;
   global.Wroot = w;
   global.Hroot = h;

   DisplayDimensions();
}

void DisplayDimensions()
{
   P("Displaydimensions\n");
   Lock_fallen();
   unsigned int w,h,b,d;
   int x,y;
   Window root;

   int rc = XGetGeometry(global.display,global.SnowWin,&root, &x, &y, &w, &h, &b, &d); 

   if (rc == 0)
   {
      P("Oeps\n");
      I("\nSnow window %#lx has disappeared, it seems. I quit.\n",global.SnowWin);
      Thanks();
      exit(1);
      return;
   }

   global.SnowWinWidth  = w;
   global.SnowWinHeight = h + Flags.OffsetS;

   P("DisplayDimensions: SnowWinX: %d Y:%d W:%d H:%d\n",global.SnowWinX,global.SnowWinY,global.SnowWinWidth,global.SnowWinHeight);

   global.SnowWinBorderWidth = b;
   global.SnowWinDepth       = d;

   UpdateFallenSnowAtBottom();

   RedrawTrees();

   SetMaxScreenSnowDepth();
   if(!global.IsDouble)
      ClearScreen();
   Unlock_fallen();
}

void SetBackground()
{
   char *f = Flags.BackgroundFile;
   if (!IsReadableFile(f))
      return;

   printf(_("Setting background from %s\n"),f);

   int w = global.SnowWinWidth;
   int h = global.SnowWinHeight;
   Display *display = global.display;
   Window window = global.SnowWin;
   int screen_num = DefaultScreen(display);
   int depth = DefaultDepth(display, screen_num);

   GdkPixbuf *pixbuf;
   pixbuf = gdk_pixbuf_new_from_file_at_scale(f,w,h,FALSE,NULL);
   if (!pixbuf)
      return;
   int n_channels = gdk_pixbuf_get_n_channels(pixbuf);

   guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);
   P("pad: %d %d\n",XBitmapPad(display),depth);

   unsigned char *pixels1 = (unsigned char*)malloc(w*h*4*sizeof(unsigned char));
   assert(pixels1);
   // https://gnome.pages.gitlab.gnome.org/gdk-pixbuf/gdk-pixbuf/class.Pixbuf.html
   //

   int rowstride = gdk_pixbuf_get_rowstride (pixbuf);
   P("rowstride: %d\n",rowstride);
   int k = 0;
   if(is_little_endian())
      for (int i=0; i<h; i++)
	 for (int j=0; j<w; j++)
	 {
	    guchar *p = &pixels[i*rowstride +j*n_channels];
	    pixels1[k++] = p[2];
	    pixels1[k++] = p[1];
	    pixels1[k++] = p[0];
	    pixels1[k++] = 0xff;
	 }
   else
   {
      I("Big endian system, swapping bytes in background.\n");
      I("Let me know if this is not OK.\n");
      for (int i=0; i<h; i++)
	 for (int j=0; j<w; j++)
	 {
	    guchar *p = &pixels[i*rowstride +j*n_channels];
	    pixels1[k++] = 0xff;
	    pixels1[k++] = p[0];
	    pixels1[k++] = p[1];
	    pixels1[k++] = p[2];
	 }
   }

   XImage *ximage;
   ximage = XCreateImage(display, 
	 DefaultVisual(display, screen_num),
	 depth,
	 ZPixmap,
	 0,
	 (char*)pixels1,
	 w,
	 h,
	 XBitmapPad(display),
	 0
	 );
   XInitImage(ximage);
   Pixmap pixmap;
   pixmap = XCreatePixmap(display,window,w,h,DefaultDepth(display,screen_num));

   GC gc;
   gc = XCreateGC(display,pixmap,0,0);
   XPutImage(display,pixmap,gc,ximage,0,0,0,0,w,h);

   P("setwindowbackground\n");
   XSetWindowBackgroundPixmap(display,window,pixmap);
   g_object_unref(pixbuf);
   XFreePixmap(display, pixmap);
   XDestroyImage(ximage);
   //free(pixels1);  //This is already freed by XDestroyImage
   return;
}

// add property with name and value
int SetProperty(Display *display, Window window, const char *name, const char *value)
{
   return XChangeProperty(
	 display,
	 window,
	 XInternAtom(display, name, False),      // property
	 XInternAtom(display, "STRING", False),  // type
	 8,                                      // format
	 PropModeReplace,                        // mode
	 (unsigned char*)value,                  // data
	 strlen(value)                           // nelements
	 );
}
