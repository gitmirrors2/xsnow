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
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Intrinsic.h>
#include <X11/extensions/Xinerama.h>
#include <assert.h>
#include "transwindow.h"
#include "windows.h"
#include "wmctrl.h"
#include "debug.h"
#include "xdo.h"


static int setvaria(GtkWidget *widget);
static int GetYOffset(Display *mydisplay, int x, int y, int w, int h);

/*
 * creates transparent window using gtk3/cairo. 
 * transwindow: (input)  GtkWidget to create transparent window in
 * xscreen:     (input)  <0: full-screen  else xinerama screen number
 * sticky:      (input)  visible on all workspaces or not
 * below:       (input)  1: below all other windows 2: above all other windows 0: no action
 * dock:        (input)  make it a 'dock' window: no decoration and not interfering with xsnow, xpenguins
 *                       NOTE: with dock=1, gtk ignores the value of below: window is above all other windows
 *                       NOTE: with decorations set to TRUE (see gtk_window_set_decorated()),
 *                             the window is not click-through in Gnome
 *                             So: dock = 1 is good for Gnome, or call gtk_window_set_decorated(w,FALSE)
 *                             before this function
 * gdk_window:  (output) GdkWindow created
 * x11_window:  (output) Window X11 window created
 */
int make_trans_window(Display *display, GtkWidget *transwindow, int xscreen, int sticky, int below, int dock,  
      GdkWindow **gdk_window, Window *x11_window, int *wantx, int *wanty)
{
   P("Entering make_trans_window... wantx: %d wanty: %d\n",*wantx, *wanty);
   if(gdk_window)
      *gdk_window = NULL;
   if(x11_window)
      *x11_window = 0;

   // We take full responsibility for drawing background etc.
   // Also, this is essential to obtain the desired effect. 
   gtk_widget_set_app_paintable(transwindow, TRUE);

   // essential in Gnome:
   gtk_window_set_decorated(GTK_WINDOW(transwindow),FALSE); 
   // essential everywhere:
   gtk_window_set_accept_focus(GTK_WINDOW(transwindow), FALSE);

   // take care that 'below' and 'sticky' are taken care of in gtk_main loop:
   g_signal_connect(transwindow, "draw", G_CALLBACK(setvaria), NULL);

   // remove our things from transwindow:
   g_object_steal_data(G_OBJECT(transwindow),"trans_sticky");
   g_object_steal_data(G_OBJECT(transwindow),"trans_below");
   g_object_steal_data(G_OBJECT(transwindow),"trans_nobelow");
   g_object_steal_data(G_OBJECT(transwindow),"trans_done");


   static char somechar;
   if (sticky)
      g_object_set_data(G_OBJECT(transwindow),"trans_sticky",&somechar);

   switch(below)
   {
      case 0: 
	 g_object_set_data(G_OBJECT(transwindow),"trans_nobelow",&somechar);
	 break;
      case 1:
	 g_object_set_data(G_OBJECT(transwindow),"trans_below",&somechar);
	 break;
   }

   /* To check if the display supports alpha channels, get the visual */
   GdkScreen *screen = gtk_widget_get_screen(transwindow);

   if (!gdk_screen_is_composited(screen)) 
   {
      P("No alpha\n");
      gtk_window_close(GTK_WINDOW(transwindow));
      return FALSE;
   }

   // Ensure the widget (the window, actually) can take RGBA
   gtk_widget_set_visual(transwindow, gdk_screen_get_rgba_visual(screen));

   int winx, winy; // desired position of window
   int winw, winh; // desired size of window
   int wantxin = (xscreen >=0);
   // set full screen if so desired:
   P("xscreen: %d\n",xscreen);

   if(xscreen < 0)
   {
      P("fullscreen\n");
      XWindowAttributes attr;
      XGetWindowAttributes(display,DefaultRootWindow(display),&attr);
      P("width, height %d %d\n",attr.width,attr.height);
      winx = 0;
      winy = 0;
      winw = attr.width;
      int x,y,w,h,yoffset;
      int nscreens = xinerama(display, -1, &x, &y, &w, &h);
      if (nscreens == 0)
      {
	 yoffset = GetYOffset(display,0,0,w,10);
	 P("nscreens = 0, yoffset: %d\n",yoffset);
      }
      else
      {
	 yoffset = 0;
	 for (int screen=0; screen<nscreens; screen++)
	 {
	    xinerama(display,screen, &x, &y, &w, &h);
	    int q = GetYOffset(display,x,y,w,10);
	    if (q > yoffset)
	    {
	       yoffset = q;
	       P("yoffset %d, x %d, y %d,w %d,h %d\n",yoffset,x,y,w,h);
	    }
	 }
      }
      winh = attr.height-yoffset;
      P("GetYOffset: %d\n",yoffset);
      gtk_widget_set_size_request(GTK_WIDGET(transwindow),winw,winh);

   }
   else  // xscreen >= 0
   {
      P("NOT fullscreen, but xineramascreen %d\n",xscreen);
      wantxin = xinerama(display,xscreen,&winx,&winy,&winw,&winh);
      int yoffset = GetYOffset(display,winx,winy,winw,10) - winy;
      P("yoffset: %d %d %d wantxin: %d\n",yoffset,winx,winy,wantxin);
      winh -= yoffset;
      P("winh: %d\n",winh);
      if(wantxin)
      {
	 P("winx ... %d %d %d %d\n",winx,winy,winw,winh);
	 gtk_widget_set_size_request(GTK_WIDGET(transwindow),winw,winh);
      }
   }

   gtk_widget_show_all(transwindow);
   GdkWindow *gdkwin = gtk_widget_get_window(GTK_WIDGET(transwindow));


   // so that apps like xsnow will ignore this window:
   if(dock)
      gdk_window_set_type_hint(gdkwin,GDK_WINDOW_TYPE_HINT_DOCK);
   else
      gdk_window_set_type_hint(gdkwin,GDK_WINDOW_TYPE_HINT_NORMAL);

   gdk_window_hide(gdkwin);

   gdk_window_show(gdkwin);

   if (x11_window)
   {
      *x11_window = gdk_x11_window_get_xid(gdkwin);

      P("resize %p: %d %d\n",(void*)*x11_window,winw,winh);
      XResizeWindow(display,*x11_window,winw,winh);  // necessary in xmonad, don't know why, 
      XFlush(display);                               // in combination with this one

      if (gdk_window)
	 *gdk_window = gdkwin;

      *wantx = winx;
      *wanty = winy;
      usleep(200000);  // seems sometimes to be necessary with nvidia

      // just to be sure all settings are communicated with the server
      gtk_widget_hide(transwindow);
      gtk_widget_show_all(transwindow);
      if(xscreen < 0)
      {
	 P("Calling gtk_window_move %d %d\n",winx,winy);
	 gtk_window_move(GTK_WINDOW(transwindow),0,0);
      }
      else if (wantxin)
      {
	 P("Calling gtk_window_move %d %d\n",winx,winy);
	 gtk_window_move(GTK_WINDOW(transwindow),winx,winy);
      }
      gtk_widget_show_all(transwindow);

      // set some things, but note that this has to be repeated in the gkt_main loop.

      P("explicitly call setvaria\n");
      setvaria(transwindow);
      P("end explicit call\n");
      g_object_steal_data(G_OBJECT(transwindow),"trans_done");

   }
   return TRUE;
}

// GetYOffset() returns the absolute y coordinate of a window placed at x,y.
// Method: create a window at x,y and use XTranslateCoordinates() to get the 
// desired value.
int GetYOffset(Display *mydisplay, int x, int y, int w, int h)
{
   Window mywindow = XCreateSimpleWindow(mydisplay,DefaultRootWindow(mydisplay),
	 x,y,w,h,
	 0,              // border width
	 0UL,            // valuemask
			 // 0xffff00UL      // background color yellow
	 0UL             // background black
	 );

   // make sure that window is indeed placed at x,y and indeed has size w,h:
   XSizeHints wmsize;
   wmsize.flags = USPosition | USSize;
   XSetWMNormalHints ( mydisplay , mywindow , &wmsize ) ;

   // remove decorations:
   long hints[5] = {2 , 0, 0, 0, 0};
   Atom motif_hints = XInternAtom(mydisplay, "_MOTIF_WM_HINTS", False);
   XChangeProperty(mydisplay, mywindow, motif_hints, motif_hints, 32, PropModeReplace, (unsigned char *)&hints, 5);

   XMapWindow(mydisplay, mywindow);
   xdo_t *xdo = xdo_new_with_opened_display(mydisplay,NULL,0);
   xdo_wait_for_window_map_state(xdo,mywindow,IsViewable);

   int returnvalue;
   GetAbsoluteCoordinates(mydisplay, mywindow, NULL, &returnvalue, NULL, NULL);
   //  sleep(5);

   xdo_free(xdo);
   XDestroyWindow(mydisplay, mywindow);
   return returnvalue;
}

// for some reason, in some environments the 'below' and 'stick' properties
// disappear. It works again, if we express our wishes after starting gtk_main
// and the best place is in the draw event.
//
int setvaria(GtkWidget *widget)
{
   // We want to reset the settings at least once to be sure.
   // Things like sticky and below should be stored in the widget beforehand.
   // Use the value of p itself, not what it points to.
   // Following the C standard, we have to use an array to subtract pointers.
   enum {rep = 1,nrep}; // must be >= 0, and is equal to the number of times the settings
			//                      will be done when called more than once
   static char something[nrep];
   char *p = (char *)g_object_get_data(G_OBJECT(widget),"trans_done");
   if (!p)
      p = &something[0];
   P("setvaria %p %p %d\n",p,&something,(int)(p-&something[0]));
   if (p - &something[0] >=  rep)
      return FALSE;
   p++;
   g_object_set_data(G_OBJECT(widget),"trans_done",p);

   P("setvaria %p %p %d\n",p, (void *)widget,(int)(p - &something[0]));


   GdkWindow *gdk_window1 = gtk_widget_get_window(widget);
   const int Usepassthru = 0;
   if(Usepassthru)
      gdk_window_set_pass_through(gdk_window1,TRUE); // does not work as expected
   else
   {
      cairo_region_t *cairo_region1 = cairo_region_create();
      gdk_window_input_shape_combine_region(gdk_window1, cairo_region1, 0,0);
      cairo_region_destroy(cairo_region1);
   }
   P("setvaria %d widget: %p gdkwin: %p passthru: %d\n",counter++,(void *)widget,(void *)gdk_window1,gdk_window_get_pass_through(gdk_window1));


   if(!g_object_get_data(G_OBJECT(widget),"trans_nobelow"))
   {
      if(g_object_get_data(G_OBJECT(widget),"trans_below"))
	 setbelow(GTK_WINDOW(widget));
      else
	 setabove(GTK_WINDOW(widget));
   }

   if(1)
   {
      if(g_object_get_data(G_OBJECT(widget),"trans_sticky"))
	 gtk_window_stick(GTK_WINDOW(widget));
      else
	 gtk_window_unstick(GTK_WINDOW(widget));
   }

   return FALSE;
}


// Force window below or above other windows.
// It appears that, to get a window below other windows, it can be necessary
// to do first the opposite, and then vice-versa.
// These codes are probably somewhat too exuberant ....
void setbelow(GtkWindow *w)
{
   gtk_window_set_keep_above(GTK_WINDOW(w), TRUE);
   gtk_window_set_keep_below(GTK_WINDOW(w), TRUE);
   GdkWindow *gdkw = gtk_widget_get_window(GTK_WIDGET(w));
   Window xwin = gdk_x11_window_get_xid(gdkw);
   XWindowChanges changes;
   changes.stack_mode = Below;
   Display *display = XOpenDisplay(NULL);
   XConfigureWindow(display,xwin,CWStackMode,&changes);
   P("setbelow %#lx\n",xwin);
   XCloseDisplay(display);
}

void setabove(GtkWindow *w)
{
   gtk_window_set_keep_below(GTK_WINDOW(w), TRUE);
   gtk_window_set_keep_above(GTK_WINDOW(w), TRUE);
   GdkWindow *gdkw = gtk_widget_get_window(GTK_WIDGET(w));
   Window xwin = gdk_x11_window_get_xid(gdkw);
   XWindowChanges changes;
   changes.stack_mode = Above;
   Display *display = XOpenDisplay(NULL);
   XConfigureWindow(display,xwin,CWStackMode,&changes);
   P("setabove %#lx\n",xwin);
   XCloseDisplay(display);
}
