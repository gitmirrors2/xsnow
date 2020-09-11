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
/**
 * Original code by: Mike - http://plan99.net/~mike/blog (now a dead link--unable to find it).
 * Modified by karlphillip for StackExchange:
 *     (https://stackoverflow.com/questions/3908565/how-to-make-gtk-window-background-transparent)
 * Re-worked for Gtk 3 by Louis Melahn, L.C., January 30, 2014.
 */
// Thanks to:
// https://stackoverflow.com/questions/16832581/how-do-i-make-a-gtkwindow-background-transparent-on-linux]
// and
// https://github.com/anko/hudkit

#include <gtk/gtk.h>
#include <stdlib.h>
#include <gdk/gdkx.h>
#include <string.h>
#include "varia.h"
#include "windows.h"

#include "transparent.h"

#ifdef DEBUG
#undef DEBUG
#endif
//#define DEBUG
#include "debug.h"

// USEDRAW should NOT be set, it is here for historical reasons
//#define USEDRAW
//USEDRAW1 should certainly be set, see comment below at draw1()
#define USEDRAW1

static void screen_changed(GtkWidget *widget, GdkScreen *old_screen, gpointer user_data);
#ifdef USEDRAW
static gboolean draw(GtkWidget *widget, cairo_t *cr, gpointer user_data);
#endif
#ifdef USEDRAW1
static gboolean draw1(GtkWidget *widget, cairo_t *cr, gpointer user_data);
#endif

static gboolean supports_alpha = FALSE;
static int below1;

static GdkRectangle workarea;

//
// create transparent click-through window without any decorations
// input:
//   fullscreen:    1: set window fullscreen
//   below:         1: place transparent window below all windows
//                  0: place transparent window above all windows
//                  NOTE: this is applied in draw1()
//   allworkspaces: 1: make window visible on all workspaces
//   name:             use this as name for the window
//   gtkwin            GtkWindow to transform into transparent
//   width:            width of desired window
//   height:           height of desired window
// output:
//   xwin              will receive id of created X11-window, 0 if transparency is not possible
//                       0 if no transparent window is possible
//
// NOTE: in FVWM, combined with xcompmgr or compton, it seems not be possible to put a window below:
// reason (I guess): _NET_WM_ALLOWED_ACTIONS(ATOM) (from xprop) does not include _NET_WM_ACTION_BELOW
//
int create_transparent_window(int allworkspaces, int below, int fullscreen,  
      Window *xwin, const char *name, GtkWidget *gtkwin, unsigned int width, unsigned int height)
{
   below1 = below;
   workarea.width  = width;
   workarea.height = height;

   gtk_window_set_decorated        (GTK_WINDOW(gtkwin),FALSE);

   // prevent window from showing up in taskbar: 
   // Alas, it does show in Gnome's top bar standard window menu
   gtk_window_set_skip_taskbar_hint(GTK_WINDOW(gtkwin),TRUE);
   gtk_window_set_skip_pager_hint  (GTK_WINDOW(gtkwin),TRUE);

   gtk_window_set_position         (GTK_WINDOW(gtkwin), GTK_WIN_POS_CENTER);

   P("create_transparent_window %p\n",(void *)gtkwin);
   gtk_window_set_title(GTK_WINDOW(gtkwin), name);
   gtk_widget_set_app_paintable(gtkwin, TRUE);
   // this callback we do not need:
#ifdef USEDRAW
   g_signal_connect(G_OBJECT(gtkwin), "draw", G_CALLBACK(draw), NULL);
#endif
#ifdef USEDRAW1
   g_signal_connect(G_OBJECT(gtkwin), "draw", G_CALLBACK(draw1), NULL);
#endif

   g_signal_connect(G_OBJECT(gtkwin), "screen-changed", G_CALLBACK(screen_changed), NULL);
   gtk_widget_add_events(gtkwin, GDK_BUTTON_PRESS_MASK);
   screen_changed(gtkwin, NULL, NULL);
   if (!supports_alpha)
   {
      P("No alpha\n");
      *xwin = 0;
      return FALSE;
   }
#if 0
   if(fullscreen)
      gtk_window_fullscreen        (GTK_WINDOW(gtkwin));
   else
      gtk_window_unfullscreen      (GTK_WINDOW(gtkwin));
#endif

   gtk_widget_show_all(gtkwin);

   GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET(gtkwin));
   P("gdk_window %p\n",(void *)gdk_window);
   // keep xsnow visible after 'show desktop', and as a bonus, keep
   // xsnow visible on all workspaces in some desktops:

   gdk_window_hide                 (GDK_WINDOW(gdk_window));

   //gtk_window_set_skip_taskbar_hint(GTK_WINDOW(gtkwin), TRUE);
   //gtk_window_set_accept_focus     (GTK_WINDOW(gtkwin), FALSE);
   //gtk_window_set_decorated        (GTK_WINDOW(gtkwin), FALSE);
   gtk_window_set_resizable        (GTK_WINDOW(gtkwin), FALSE);

   // see comment at draw1()
   cairo_region_t *cairo_region = cairo_region_create();
   gdk_window_input_shape_combine_region(GDK_WINDOW(gdk_window), cairo_region, 0,0);
   P("shape stuff: %p %p\n",(void *)gtkwin,(void*)gdk_window);
   cairo_region_destroy(cairo_region);
   //gdk_window_set_pass_through(gdk_window,TRUE); // does not work as expected

   gdk_window_show                 (GDK_WINDOW(gdk_window));
   // it seems that some window managers want the following below-setting also here,
   // after showing the window:
#if 0
   if(fullscreen)
      gtk_window_fullscreen        (GTK_WINDOW(gtkwin));
   else
      gtk_window_unfullscreen      (GTK_WINDOW(gtkwin));
#endif

   usleep(200000);  // seems to be necessary with nvidia, not sure if this is indeed the case


   // xsnow visible on all workspaces:
   if (allworkspaces)
      gtk_window_stick(GTK_WINDOW(gtkwin));
   //

   *xwin = gdk_x11_window_get_xid(gdk_window);

   return TRUE;
}

#if 1
static void size_to_screen(GtkWindow *window)
{
   // see https://stackoverflow.com/questions/43225956/how-to-get-the-size-of-the-screen-with-gtk:
   // also gdk_monitor_get_workarea is not always available ...
   /* GdkRectangle workarea = {0};
      gdk_monitor_get_workarea(
      gdk_display_get_primary_monitor(gdk_display_get_default()),
      &workarea);
      */

   //printf ("W: %u x H:%u\n", workarea.width, workarea.height);
   gtk_window_set_default_size(window, workarea.width, workarea.height);
   gtk_window_resize          (window, workarea.width, workarea.height);
   gtk_window_set_resizable   (window, FALSE);
}
#else
// sometimes (e.g. in slackware) gdkmonitor.h is missing, so we use a simpler solution
// see above
static int get_monitor_rects(GdkDisplay *display, GdkRectangle **rectangles) {
   int n = gdk_display_get_n_monitors(display);
   GdkRectangle *new_rectangles = (GdkRectangle*)malloc(n * sizeof(GdkRectangle));
   for (int i = 0; i < n; ++i) {
      GdkMonitor *monitor = gdk_display_get_monitor(display, i);
      gdk_monitor_get_geometry(monitor, &new_rectangles[i]);
   }
   *rectangles = new_rectangles;
   return n;
}
static void size_to_screen(GtkWindow *window) {
   //GdkScreen *screen = gtk_widget_get_screen(GTK_WIDGET(window));

   // Get total screen size.  This involves finding all physical monitors
   // connected, and examining their positions and sizes.  This is as complex
   // as it is because monitors can be configured to have relative
   // positioning, causing overlapping areas and a non-rectangular total
   // desktop area.
   //
   // We want our window to cover the minimum axis-aligned bounding box of
   // that total desktop area.  This means it's too large (even large bits of
   // it may be outside the accessible desktop) but it's easier to manage than
   // multiple windows.

   // TODO Find the min x and y too, just in case someone's weird setup
   // has something other than 0,0 as top-left.

   GdkDisplay *display = gdk_display_get_default();
   GdkRectangle *rectangles = NULL;
   int nRectangles = get_monitor_rects(display, &rectangles);

   int width = 0, height = 0;
   for (int i = 0; i < nRectangles; ++i) {
      GdkRectangle rect = rectangles[i];
      int actualWidth = rect.x + rect.width;
      int actualHeight = rect.y + rect.height;
      if (width < actualWidth) width = actualWidth;
      if (height < actualHeight) height = actualHeight;
   }
   free(rectangles);

   gtk_window_set_default_size(window, width, height);
   gtk_window_resize(window, width, height);
   gtk_window_set_resizable(window, FALSE);
}
#endif

static void screen_changed(GtkWidget *widget, UNUSED GdkScreen *old_screen, UNUSED gpointer userdata)
{
   static int msg1 = 0, msg2 = 0;
   /* To check if the display supports alpha channels, get the visual */
   GdkScreen *screen = gtk_widget_get_screen(widget);

   if (gdk_screen_is_composited(screen)) 
   {
      if(!msg1)
	 printf("Your screen supports alpha channel, good.\n");
      msg1 = 1;
      supports_alpha = TRUE;
   }
   else
   {
      if(!msg2)
	 printf("Your screen does not support transparency.\n");
      msg2 = 1;
      supports_alpha = FALSE; 
   }

   // Ensure the widget (the window, actually) can take RGBA
   gtk_widget_set_visual(widget, gdk_screen_get_rgba_visual(screen));
   size_to_screen(GTK_WINDOW(widget)); 
}

#ifdef USEDRAW
static gboolean draw(GtkWidget *widget, cairo_t *cr, UNUSED gpointer userdata)
{
   gtk_window_set_accept_focus(GTK_WINDOW(widget),FALSE);
   cairo_save (cr);

   if (supports_alpha)
   {
      //cairo_set_source_rgba (cr, 0.5, 1.0, 0.50, 0.5); /* transparent */
      cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.0); /* transparent */
      P("Draw: transparent %d\n",++counter);
   }
   else
   {
      cairo_set_source_rgb (cr, 1.0, 1.0, 1.0); /* opaque white */
      P("Draw: not transparent %d\n",++counter);
   }

   /* draw the background */
   cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
   cairo_paint (cr);

   cairo_restore (cr);

   return FALSE;
}
#endif

#ifdef USEDRAW1
// Two windows have been dreated: SnowWina and SnowWinb.
// Both windows must be made click-through.
// Fot this, gdk_window_input_shape_combine_region() is used.
// This works fine in e.g. xfce (compositing ON), Gnome, KDE.
//
// However: in e.g. LXDE with xcompmgr running, somebody loses the click-through
// properties: when running with settings "Below windows" off, snow and birds
// are flying over your windows, but you cannot click: the windows are not
// click-through.
// Maybe this has something to do with the mainloop of GTK running or not?
//
// Hence this callback: the gdk_window_input_shape_combine_region() trick is done
// if a widget appears that has not been seen yet. This results in three calls
// to gdk_window_input_shape_combine_region(): for SnowWina, then for SnowWinb
// and then for SnowWina again (probably superfluous). 
// This seems to fix the problem in some cases.
// Problem remains in fvwm and others in combination with xcompmgr or compton.
//
// Also, the 'below' paarmeter is applied here, this seems to be the only
// safe place to do it. Maybe because it is in the gtk_main loop?
//
// NOTE: this code assumes that no more than two windows are created. If there are 
//       more, some trivial changes in keeping track of these windows.
//
static gboolean draw1(GtkWidget *widget, UNUSED cairo_t *cr, UNUSED gpointer userdata)
{
   static GtkWidget *prev_widget[2] = {NULL,NULL};

   P("draw1 %d %p\n",counter++,(void *)widget);
   if (prev_widget[0] != widget && prev_widget[1]!= widget)
   {
      prev_widget[0] = prev_widget[1];
      prev_widget[1] = widget;
      GdkWindow *gdk_window1 = gtk_widget_get_window(widget);
      cairo_region_t *cairo_region1 = cairo_region_create();
      gdk_window_input_shape_combine_region(gdk_window1, cairo_region1, 0,0);
      cairo_region_destroy(cairo_region1);
      // gdk_window_set_pass_through(gdk_window1,TRUE); // does not work as expected
      P("draw1 %d widget: %p gdkwin: %p passthru: %d\n",counter++,(void *)widget,(void *)gdk_window1,gdk_window_get_pass_through(gdk_window1));

      if(below1)
	 setbelow(GTK_WINDOW(widget)); // see windows.c 
      else
	 setabove(GTK_WINDOW(widget)); // see windows.c 
   }
   return FALSE;
}
#endif

