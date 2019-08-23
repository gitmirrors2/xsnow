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

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include "transparent.h"

#ifdef DEBUG
#undef DEBUG
#endif

#ifdef DEBUG
#define P(...) printf ("%s: %d: ",__FILE__,__LINE__);printf(__VA_ARGS__)
#else
#define P(...)
#endif

static void screen_changed(GtkWidget *widget, GdkScreen *old_screen, gpointer user_data);
static gboolean draw(GtkWidget *widget, cairo_t *new_cr, gpointer user_data);

int supports_alpha(GtkWidget *widget)
{
   GdkScreen *screen = gtk_widget_get_screen(widget);
   return gdk_screen_is_composited(screen); 
}

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

//static gboolean supports_alpha = FALSE;
static int is_below = 1;

void create_transparent_window(int fullscreen, int below, 
      Window *win, char **name, GtkWidget **gtkwin)
{
   is_below = below;
   P("create_transparent_window\n");
   *gtkwin = gtk_window_new(GTK_WINDOW_TOPLEVEL); 
   gtk_window_set_position(GTK_WINDOW(*gtkwin), GTK_WIN_POS_CENTER);
   *name = strdup("Xsnow-Window");
   gtk_window_set_title(GTK_WINDOW(*gtkwin), *name);
   gtk_widget_set_app_paintable(*gtkwin, TRUE);
   g_signal_connect(G_OBJECT(*gtkwin), "draw", G_CALLBACK(draw), NULL);
   g_signal_connect(G_OBJECT(*gtkwin), "screen-changed", G_CALLBACK(screen_changed), NULL);
   gtk_widget_add_events(*gtkwin, GDK_BUTTON_PRESS_MASK);
   screen_changed(*gtkwin, NULL, NULL);
   if (!supports_alpha(*gtkwin))
   {
      P("No alpha\n");
      gtk_window_close(GTK_WINDOW(*gtkwin));
      *gtkwin = NULL;
      *win = 0;
      return;
   }

   if (fullscreen)
      gtk_window_fullscreen(GTK_WINDOW(*gtkwin));  // problems with dock
   gtk_widget_show_all(*gtkwin);

   GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET(*gtkwin));
   gdk_window_hide                 (GDK_WINDOW(gdk_window));
   if(below)
      gtk_window_set_keep_below       (GTK_WINDOW(*gtkwin), TRUE);
   else
      gtk_window_set_keep_above       (GTK_WINDOW(*gtkwin), TRUE);
   gtk_window_set_skip_taskbar_hint(GTK_WINDOW(*gtkwin), TRUE);
   gtk_window_set_accept_focus     (GTK_WINDOW(*gtkwin), FALSE);
   gtk_window_set_decorated        (GTK_WINDOW(*gtkwin), FALSE);
   gtk_window_set_resizable        (GTK_WINDOW(*gtkwin), FALSE);
   cairo_region_t *cairo_region = cairo_region_create();
   gdk_window_input_shape_combine_region(GDK_WINDOW(gdk_window),
	 cairo_region, 0,0);
   cairo_region_destroy(cairo_region);
   gdk_window_show                 (GDK_WINDOW(gdk_window));
   if (fullscreen)
      gtk_window_fullscreen(GTK_WINDOW(*gtkwin));
   usleep(200000);  // seems to be necessary with nvidia
   if(below)
      gtk_window_set_keep_below       (GTK_WINDOW(*gtkwin), TRUE);
   else
      gtk_window_set_keep_above       (GTK_WINDOW(*gtkwin), TRUE);
   *win = gdk_x11_window_get_xid(gdk_window);
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

static void screen_changed(GtkWidget *widget, GdkScreen *old_screen, gpointer userdata)
{
   static int msg1 = 0, msg2 = 0;
   /* To check if the display supports alpha channels, get the visual */
   GdkScreen *screen = gtk_widget_get_screen(widget);

   if (gdk_screen_is_composited(screen)) 
   {
      if(!msg1)
	 printf("Your screen supports alpha channel, good.\n");
      msg1 = 1;
   }
   else
   {
      if(!msg2)
	 printf("Your screen does not support transparency.\n");
      msg2 = 1;
   }

   // Ensure the widget (the window, actually) can take RGBA
   gtk_widget_set_visual(widget, gdk_screen_get_rgba_visual(screen));
   size_to_screen(GTK_WINDOW(widget));
}

static gboolean draw(GtkWidget *widget, cairo_t *cr, gpointer userdata)
{
#ifdef DEBUG
   static int count = 0;
#endif
   if(is_below)
      gtk_window_set_keep_below(GTK_WINDOW(widget),TRUE);
   else
      gtk_window_set_keep_above(GTK_WINDOW(widget),TRUE);
   gtk_window_set_accept_focus(GTK_WINDOW(widget),FALSE);
   cairo_save (cr);

   if (supports_alpha(widget))
   {
      cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.0); /* transparent */
      P("Draw: transparent %d\n",++count);
   }
   else
   {
      cairo_set_source_rgb (cr, 1.0, 1.0, 1.0); /* opaque white */
      P("Draw: not transparent %d\n",++count);
   }

   /* draw the background */
   cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
   cairo_paint (cr);

   cairo_restore (cr);

   return FALSE;
}
