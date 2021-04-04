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
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Intrinsic.h>
#include "transwindow.h"
#include "debug.h"


static int setvaria(GtkWidget *widget);

/*
 * creates transparent window using gtk3/cairo. 
 * transwindow: (input)  GtkWidget to create transparent window in
 * fullscreen:  (input)  full-screen or not full screen
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
int make_trans_window(GtkWidget *transwindow, int fullscreen, int sticky, int below, int dock,  
      GdkWindow **gdk_window, Window *x11_window)
{
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

   // set full screen if so desired:
   if(fullscreen)
   {
      P("fullscreen\n");
      XWindowAttributes attr;
      Display *display = XOpenDisplay(NULL);
      XGetWindowAttributes(display,DefaultRootWindow(display),&attr);
      gtk_widget_set_size_request(GTK_WIDGET(transwindow),attr.width,attr.height);
      XCloseDisplay(display);
   }
   else
   {
      P("NOT fullsscreen\n");
   }

   gtk_widget_show_all(transwindow);
   GdkWindow *gdkwin = gtk_widget_get_window(GTK_WIDGET(transwindow));

   // so that apps like xsnow will ignore this window:
   if(dock)
      gdk_window_set_type_hint(gdkwin,GDK_WINDOW_TYPE_HINT_DOCK);

   gdk_window_hide(gdkwin);
   if(fullscreen)
      gdk_window_move(gdkwin,0,0);
   gdk_window_show(gdkwin);

   if (x11_window)
      *x11_window = gdk_x11_window_get_xid(gdkwin);
   if (gdk_window)
      *gdk_window = gdkwin;

   usleep(200000);  // seems sometimes to be necessary with nvidia

   // just to be sure all settings are communicated with the server
   gtk_widget_hide(transwindow);
   gtk_widget_show_all(transwindow);

   // set some things, but note that this has to be repeated in the gkt_main loop.

   P("explicitly call setvaria\n");
   setvaria(transwindow);
   P("end explicit call\n");
   g_object_steal_data(G_OBJECT(transwindow),"trans_done");

   return TRUE;
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
