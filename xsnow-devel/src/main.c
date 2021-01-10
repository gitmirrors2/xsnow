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
/*
   And in a vocoded voice it sounds:
   Xsnow zwei-tausend
   Xsnow two-thousand 
   Xsnow deux mille
   Xsnow dos mil
   etc.
   */

/*
 * contains main_c(), the actual main program, written in C.
 * main_c() is to be called from main(), written in C++, 
 * see mainstub.cpp and mainstub.h
 */

#define dosync 0  /* synchronise X-server. Change to 1 will detoriate the performance
		     but allow for better analysis
		     */

/*
 * Reals dealing with time are declared as double. 
 * Other reals as float
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <X11/Intrinsic.h>
#include <ctype.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "birds.h"
#include "clocks.h"
#include "docs.h"
#include "dsimple.h"
#include "fallensnow.h"
#include "flags.h"
#include "mainstub.h"
#include "meteo.h"
#include "moon.h"
#include "Santa.h"
#include "scenery.h"
#include "snow.h"
#include "transparent.h"
#include "ui.h"
#include "utils.h"
#include "version.h"
#include "wind.h"
#include "windows.h"
#include "wmctrl.h"
#include "xsnow.h"
#include "stars.h"
#include "blowoff.h"
#include "debug.h"
#include "treesnow.h"
#include "loadmeasure.h"
#include "varia.h"

#include "vroot.h"

#ifdef DEBUG
#undef DEBUG
#endif
//#define DEBUG


// from windows.h
Display *display;
int     screen;
int     SnowWinBorderWidth;
int     SnowWinDepth;
int     SnowWinHeight;
int     SnowWinWidth; 
char   *DesktopSession = NULL;
int     IsCompiz;
int     IsWayland;
Pixel   ErasePixel;
Pixel   BlackPix;
int     counter = 0;

double  cpufactor = 1.0;
float   NewWind = 0;

GtkWidget       *drawing_area = NULL;

// miscellaneous
char       Copyright[] = "\nXsnow\nCopyright 1984,1988,1990,1993-1995,2000-2001 by Rick Jansen, all rights reserved, 2019,2020 also by Willem Vermin\n";
static int HaltedByInterrupt = 0;

static char **Argv;
static int Argc;

// timing stuff
//static double       TotSleepTime = 0;

// windows stuff
int                 CWorkSpace = 0;
long                TransWorkSpace = -SOMENUMBER;  // workspace on which transparent window is placed
static int          DoRestart = 0;
static guint        draw_all_id = 0;
static gulong       drawconnect = 0;


/* Colo(u)rs */
static const char *BlackColor  = "black";


/* Forward decls */
static void   HandleCpuFactor(void);
static void   HandleExposures(void);
static void   RestartDisplay(void);
static void   SetGCFunctions(void);
static void   SigHandler(int signum);
static int    XsnowErrors(Display *dpy, XErrorEvent *err);
static void   drawit(cairo_t *cr);
static void   restart_do_draw_all(void);
static int    myDetermineWindow(void);
static void   set_below_above(void);
static void   change_ww(void);
static void   DoAllWorkspaces(void);


static void Thanks(void)
{
   if (HaltedByInterrupt)
      printf("\nXsnow: Caught signal %d\n",HaltedByInterrupt);
   printf("\nThank you for using xsnow\n");
}

// callbacks
static int do_displaychanged(gpointer data);
static int do_draw_all(gpointer widget);
static int do_event(gpointer data);
static int do_show_range_etc(gpointer data);
static int do_testing(gpointer data);
static int do_ui_check(gpointer data);
static int do_stopafter(gpointer data);
static int do_show_desktop_type(gpointer data);
static int do_display_dimensions(UNUSED gpointer data);
static gboolean     on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data);

/**********************************************************************************************/

/* About windows, user whishes and scenarios

 * The following animations exist:

 *   - Snow: snow, Santa and scenery.
 *   - Birds: birds.
 *
 * * The following windows are used:
 *
 *   - TransA: transparent GtkWidget with drawing area
 *   - SnowWin: X11 window
 *   - depending on window manager and user whishes, this can be
 *   - the root window
 *   - an existing non-root window (see flags -xwindow and -id)
 *   - a new transparent window
 *
 * * The following user whishes are possible:
 *   ( enum{UW_NONE,UW_ROOT,UW_TRANSPARENT}; )
 *
 *   - WantWindow = none: default, no special whish
 *   - Flags.WantWindow = UW_NONE
 *   - -wantwindow default
 *   - WantWindow = root: user whishes to use X11 window for snow
 *   - Flags.WantWindow = UW_ROOT
 *   - -wantwindow root
 *   - WantWindow = trans: user whishes to use X11 transparent window
 *   - Flags.WantWindow = UW_TRANSPARENT
 *   - -wantwindow transparent
 *
 *   If -id or -xwininfo are specified, WantWindow = 1 is implied, and SnowWin
 *   is an existing non-root window.
 *   If WantWindow = 1 and -id and -xwininfo are not specified, 
 *   SnowWin is the root window.
 *
 * * The following switches are determined, dependend on window 
 *   manager and user whishes. These switches should be sufficient
 *   to determine how to paint snow and birds.
 *
 *   - UseGtk: 
 *     - 1: Snow is painted on TransA, using cairo
 *     - 0: Snow is painted on SnowWin, using X11
 *
 *   - Trans:
 *     - 1: SnowWin is     transparent
 *     - 0: SnowWin is not transparent
 *
 *   - Root:  (maybe this is superfluous, we will see)
 *     - 1: SnowWin is     root window
 *     - 0: SnowWin is not root window
 *
 *   - DrawBirds:
 *     - 1: Birds are painted on TransA, using cairo
 *     - 0: Birds cannot be painted
 *
 */
/*
 * * Depending on the possibility to create transparent windows and user wishes,
 *   the following scenarios exist:
 *
 *   - Transparent window is possible: TransA exists
 *
 *      - SCENARIO 1:        ** Not implemented, starting with scenario1, switch to
 *                           ** scenario 2 gives no Santa, snow etc. Something wrong probably 
 *                           ** with GC's, but also when re-creating GC's, got no visuals of Santa etc.
 *                           ** Snowing in root window is useless in a compositing window manager
 *                           ** so: no big loss.
 *        - WantWindow = root
 *        - Snow in     SnowWin
 *        - Birds in    TransA
 *        - UseGtk    = 0
 *        - Trans     = 0
 *        - Root      = 0 or 1
 *        - DrawBirds = 1
 *
 *      - SCENARIO 2:
 *        - WantWindow = trans
 *        - Snow in     SnowWin
 *        - Birds in    TransA
 *        - UseGtk    = 0
 *        - Trans     = 1
 *        - Root      = 1
 *        - DrawBirds = 1
 *
 *      - SCENARIO 3:
 *        - WantWindow = none
 *        - Snow in     TransA
 *        - Birds in    TransA
 *        - UseGtk    = 1
 *        - Trans     = n/a
 *        - Root      = n/a
 *        - DrawBirds = 1
 *
 *   - Transparent window is NOT possible, TransA does not exist
 *
 *      - SCENARIO 4:
 *        - WantWindow = none|trans|root, user cannot force anything
 *        - Snow in     SnowWin
 *        - Birds in    nothing, birds cannot fly
 *        - UseGtk    = 0
 *        - Trans     = 0
 *        - Root      = 1
 *        - DrawBirds = 0
 */

/**********************************************************************************************/

int main_c(int argc, char *argv[])
{
   P("This is xsnow\n");
   signal(SIGINT,  SigHandler);
   signal(SIGTERM, SigHandler);
   signal(SIGHUP,  SigHandler);
   srand48((long int)(wallcl()*1.0e6));

   int i;
   // make a copy of all flags, before gtk_init() maybe removes some.
   // we need this at a restart of the program.

   Argv = (char**) malloc((argc+1)*sizeof(char**));
   for (i=0; i<argc; i++)
      Argv[i] = strdup(argv[i]);
   Argv[argc] = NULL;
   Argc = argc;

   InitFlags();
   // we search for flags that only produce output to stdout,
   // to enable to run in a non-X environment, in which case 
   // gtk_init() would fail.
   for (i=0; i<argc; i++)
   {
      char *arg = argv[i];
      if(!strcmp(arg, "-h") || !strcmp(arg, "-help")) 
      {
	 docs_usage(0);
	 return 0;
      }
      if(!strcmp(arg, "-H") || !strcmp(arg, "-manpage")) 
      {
	 docs_usage(1);
	 return 0;
      }
      if (!strcmp(arg, "-v") || !strcmp(arg, "-version")) 
      {
	 PrintVersion();
	 return 0;
      }
   }
   printf("Xsnow running in GTK version: %s\n",ui_gtk_version());

   // Circumvent wayland problems:before starting gtk: make sure that the 
   // gdk-x11 backend is used.

   if (getenv("WAYLAND_DISPLAY")&&getenv("WAYLAND_DISPLAY")[0])
   {
      printf("Detected Wayland desktop\n");
      setenv("GDK_BACKEND","x11",1);
      IsWayland = 1;
   }
   else
      IsWayland = 0;

   gtk_init(&argc, &argv);
   int rc = HandleFlags(argc, argv);
   switch(rc)
   {
      case -1:   // wrong flag
	 PrintVersion();
	 Thanks();
	 return 1;
	 break;
      case 1:    // manpage or help
	 //         Ok, this cannot happen, is already caught above
	 return 0;
	 break;
      default:  // ditto
	 PrintVersion();
	 break;
   }
   if (!Flags.NoConfig)
      WriteFlags();

   if(Flags.CheckGtk && !ui_checkgtk() && !Flags.NoMenu)
   {
      printf("Xsnow needs gtk version >= %s, found version %s\n",ui_gtk_required(),ui_gtk_version());

      if(!ui_run_nomenu())
      {
	 Thanks();
	 return 0;
      }
      else
      {
	 Flags.NoMenu = 1;
	 printf("Continuing with flag '-nomenu'\n");
      }
   }

   display = XOpenDisplay(Flags.DisplayName);
   if(0)  // to prevent warning about unused function in vroot.h
      Rootwindow = DefaultRootWindow(display);  // Rootwindow will be set in DetermineWindow() in windows.c 
   XSynchronize(display,dosync);
   XSetErrorHandler(XsnowErrors);
   screen = DefaultScreen(display);
   Black = BlackPixel(display, screen);
   White = WhitePixel(display, screen);

   HandleExposures();

   InitSnowOnTrees();

   if (display == NULL) {
      if (Flags.DisplayName == NULL) Flags.DisplayName = getenv("DISPLAY");
      (void) fprintf(stderr, "%s: cannot connect to X server %s\n", argv[0],
	    Flags.DisplayName ? Flags.DisplayName : "(default)");
      exit(1);
   }
   //
   // define:
   // - snow in: 
   //           ------------------------
   //           |  * *           *     |
   //           |     *  *             |
   //           |               *      |
   //           ------------------------
   // - snow on:
   //               *****  *     ****
   //           *  ****** **** *******
   //           ------------------------
   //           |                      |
   //           |                      |
   //           |                      |
   //           ------------------------
   //
   // Find window to snow in:
   //
   // if(switches.Desktop): we have a desktop and we will snow on the windows in it
   //   else we have a normal window and will not snow on other windows
   // if(switches.Trans): drawing to the desktop is as follows:
   //   - all colours are made opaque by or-ing the colors with 0xff000000
   //   - clearing is done writing the same image, but with color black (0x00000000) 
   //   else
   //   - we will use XClearArea to erase flakes and the like. This works well
   //     on fvwm-like desktops (desktop == Rootwindow) with exposures set to 0
   //     It works more or less in for example KDE, but exposures must be set to 1
   //     which severely stresses plasma shell (or nautilus-desktop in Gnome, 
   //     but we do not use XClearArea in Gnome).
   //
   // And then, we have 'snowing below' and 'snowing above', which respectively
   // means: snow behind all windows and snow before all windows.



   if (!myDetermineWindow())
   {
      return 1;
   }


   Flags.Done = 0;
   NoSnowArea_dynamic   = XCreateRegion();  // needed when drawing on background with xor.
   //                                          unpleasant things happen when a snowflake
   //                                          is in the trajectory of a meteorite
   windows_init();
   moon_init();
   Santa_init();
   birds_init();
   scenery_init();
   snow_init();
   meteo_init();
   wind_init();
   stars_init();
   fallensnow_init();
   blowoff_init();
   treesnow_init();
   loadmeasure_init();

   add_to_mainloop(PRIORITY_DEFAULT, time_displaychanged, do_displaychanged          ,NULL);
   add_to_mainloop(PRIORITY_DEFAULT, time_event,          do_event                   ,NULL);
   add_to_mainloop(PRIORITY_DEFAULT, time_testing,        do_testing                 ,NULL);
   add_to_mainloop(PRIORITY_DEFAULT, 1.0,                 do_display_dimensions      ,NULL);
   add_to_mainloop(PRIORITY_HIGH,    time_ui_check,       do_ui_check                ,NULL);
   add_to_mainloop(PRIORITY_DEFAULT, time_show_range_etc, do_show_range_etc          ,NULL);

   if (Flags.StopAfter > 0)
      add_to_mainloop(PRIORITY_DEFAULT, Flags.StopAfter, do_stopafter, NULL);

   HandleCpuFactor();

#define DOIT_I(x,d,v) OldFlags.x = Flags.x;
#define DOIT_L(x,d,v) DOIT_I(x,d,v);
#define DOIT_S(x,d,v) OldFlags.x = strdup(Flags.x);
   DOITALL;
#include "undefall.inc"


   OldFlags.FullScreen = !Flags.FullScreen;

   BlackPix = AllocNamedColor(BlackColor, Black);

   SetGCFunctions();

   // events
   if(switches.Desktop)
      XSelectInput(display, SnowWin, 0);
   else
      XSelectInput(display, SnowWin, 
	    StructureNotifyMask);

   ClearScreen();   // without this, no snow, scenery etc. in KDE; this seems to be a vintage comment

   if(!Flags.NoMenu)
   {
      ui(&argc, &argv);

      ui_gray_erase(switches.UseGtk);

      if (!TransA)
      {
	 ui_gray_ww(1);
	 ui_gray_below(1);
	 ui_gray_birds(1);
	 ui_set_birds_header("No alpha channel: no birds will fly.");
	 ui_set_celestials_header("No alpha channel: no moon.");
      }
      ui_set_sticky(Flags.AllWorkspaces);
      add_to_mainloop(PRIORITY_DEFAULT, 2.0, do_show_desktop_type, NULL);
   }

   // main loop
   gtk_main();


   if (SnowWinName) free(SnowWinName);

   XClearArea(display, SnowWin, 0,0,0,0,True);
   XFlush(display);
   XCloseDisplay(display); //also frees the GC's, pixmaps and other resources on display

   if (DoRestart)
   {
      sleep(0);
      printf("Xsnow restarting: %s\n",Argv[0]);
      fflush(NULL);
      execvp(Argv[0],Argv);
   }
   else
      Thanks();
   return 0;
}		/* End of snowing */



void set_below_above()
{
   P("%d set_below_above\n",counter++);
   XWindowChanges changes;
   // to be sure: we do it in gtk mode and window mode
   if (Flags.BelowAll)
   {
      setbelow(GTK_WINDOW(TransA));
      setbelow(GTK_WINDOW(TransB));
      changes.stack_mode = Below;
      if(SnowWina)XConfigureWindow(display,SnowWina,CWStackMode,&changes);
      if(SnowWinb)XConfigureWindow(display,SnowWinb,CWStackMode,&changes);
   }
   else
   {
      setabove(GTK_WINDOW(TransA));
      setabove(GTK_WINDOW(TransB));
      changes.stack_mode = Above;
      if(SnowWina)XConfigureWindow(display,SnowWina,CWStackMode,&changes);
      if(SnowWinb)XConfigureWindow(display,SnowWinb,CWStackMode,&changes);
   }
}

int myDetermineWindow()
{
   P("myDetermineWindow root: %#lx\n",Rootwindow);

   if (drawconnect)
   {
      P("disconnecting %ld\n",drawconnect);
      g_signal_handler_disconnect(TransA,drawconnect);
      P("disconnected\n");
   }

   static char * SnowWinaName = NULL;
   static char * SnowWinbName = NULL;
   static int IsDesktop = 0;

   if(IsDesktop)
      if(1)
      {
	 if (SnowWina)
	 {
	    XDestroyWindow(display,SnowWina);
	    SnowWina = 0;
	 }
	 if (SnowWinb)
	 {
	    XDestroyWindow(display,SnowWinb);
	    SnowWinb = 0;
	 }
	 if (TransA)
	 {
	    gtk_widget_destroy(TransA);
	    TransA = NULL;
	 }
	 if (TransB)
	 {
	    gtk_widget_destroy(TransB);
	    TransB = NULL;
	 }
      }
   if (!SnowWina)
   {
      if (SnowWinaName)
      {
	 free(SnowWinaName);
	 SnowWinaName = NULL;
      }
      if (!DetermineWindow(&SnowWina,&SnowWinaName,&TransA,"Xsnow-A", &IsDesktop))
      {
	 printf("xsnow: Cannot run, probably missing a window manager. Exiting...\n");
	 return 0;
      }

      P("SnowWina: %#lx %s TransA: %p\n",SnowWina,SnowWinaName,(void *)TransA);

      // if user specified window, TransA will be 0
      if (TransA)
      {
	 if (SnowWinbName)
	 {
	    free(SnowWinbName);
	    SnowWinbName = NULL;
	 }
	 drawing_area = gtk_drawing_area_new();
	 gtk_container_add(GTK_CONTAINER(TransA), drawing_area);

	 DetermineWindow(&SnowWinb, &SnowWinbName, &TransB, "Xsnow-B", &IsDesktop);

	 P("SnowWinb: %#lx %s TransB: %p\n",SnowWinb,SnowWinbName,(void *)TransB);
      }
   }

   switches.UseGtk    = 1;
   switches.DrawBirds = 1;
   switches.Trans     = 0;
   switches.Root      = 0;
   switches.Desktop   = IsDesktop;


   if (TransA)  // we have a transparent window
   {
      if (Flags.WantWindow == UW_TRANSPARENT) // Scenario 2
      {
	 P("Scenario 2\n");

	 printf("Scenario: Use X11 for drawing snow in transparent window, birds can fly.\n");
	 SnowWin            = SnowWinb;
	 SnowWinName        = SnowWinbName;
	 gtk_widget_show_all(TransB);

	 switches.UseGtk    = 0;
	 switches.DrawBirds = 1;
	 switches.Trans     = 1;
	 switches.Root      = 0;
	 switches.Desktop   = IsDesktop;
      }
      else if(Flags.WantWindow == UW_DEFAULT)  // Scenario 3
      {
	 P("Scenario 3\n");
	 printf("Scenario: Use Gtk for drawing snow in transparent window, birds can fly.\n");
	 SnowWin            = SnowWina;
	 SnowWinName        = SnowWinaName;
	 gtk_widget_hide(TransB);

	 switches.UseGtk    = 1;
	 switches.DrawBirds = 1;
	 switches.Trans     = 1;  // should be 1? todo
	 switches.Root      = 0;
	 switches.Desktop   = 1;
      }
   }
   else                          //  No transparent window: Scenario 4
   {
      P("Scenario 4 IsDesktop: %d\n",IsDesktop);
      printf("Scenario: Use X11 for drawing snow, no birds will fly, no moon will shine.\n");
      // in LXDE, SnowWin will be overwritten by id of window pcmanfm
      SnowWin            = SnowWina;
      SnowWinName        = SnowWinaName;
      switches.UseGtk    = 0;
      switches.DrawBirds = 0;
      switches.Trans     = 0;
      switches.Root      = 1;
      switches.Desktop   = IsDesktop;
   }

   if (TransA)
      printf("Birds in window: %#lx - \"%s\"\n",SnowWina,SnowWinaName);
   else
      printf("No birds, no moon (you need a compositing display manager to let birds fly and monn shine).\n");

   if(TransA)
   {
      TransWorkSpace = GetCurrentWorkspace();

      drawconnect = g_signal_connect(TransA, "draw", G_CALLBACK(on_draw_event), NULL);
      P("connecting %ld\n",drawconnect);
      restart_do_draw_all();  // to (re-)establish the timeout for do_draw_all
      set_below_above();
   }
   else
   {
      drawconnect = 0;
   }

   InitDisplayDimensions();

   static int first_call = 1;
   static int xa = 0;
   static int ya = 0;

   if (first_call)
   {
      first_call = 0;
      xa = SnowWinX;
      ya = SnowWinY;
   }


   if(switches.Desktop)
   {
      if(Flags.MoveWindow)
      {
	 P("XM: %d %d\n",0,0);
	 if(SnowWina)XMoveWindow(display,SnowWina,0,0);
	 if(SnowWinb)XMoveWindow(display,SnowWinb,0,0);
      }
      else
      {
	 P("XM: %d %d\n",xa,ya);
	 if(SnowWina)XMoveWindow(display,SnowWina,xa,ya);
	 if(SnowWinb)XMoveWindow(display,SnowWinb,xa,ya);
      }
   }

   P("windows: SnowWin:%s SnowWina:%s SnowWinb:%s\n",SnowWinName,SnowWinaName,SnowWinbName);
   printf("Snowing in window: %#lx - \"%s\" - depth: %d - geom: %d %d %dx%d - alpha: %s - exposures: %d\n",
	 SnowWin,SnowWinName,SnowWinDepth,
	 SnowWinX,SnowWinY,SnowWinWidth,SnowWinHeight, TransA?"yes":"no",switches.Exposures);
   fflush(NULL);
   return 1;
}


void change_ww()
{
   ClearScreen();
   P("WantWindow: %d\n",Flags.WantWindow);
   if(draw_all_id)
   {
      g_source_remove(draw_all_id);
      P("removed %d\n",draw_all_id);
   }
   draw_all_id = 0;
   myDetermineWindow();
   ui_gray_erase(switches.UseGtk);
   P("WantWindow: %d\n",Flags.WantWindow);
}


void DoAllWorkspaces()
{
   if(Flags.AllWorkspaces)
   {
      P("stick\n");
      if (switches.UseGtk||switches.Trans)
      {
	 gtk_window_stick(GTK_WINDOW(TransA));
	 gtk_window_stick(GTK_WINDOW(TransB));
      }
   }
   else
   {
      P("unstick\n");
      if (switches.UseGtk||switches.Trans)
      {
	 gtk_window_unstick(GTK_WINDOW(TransA));
	 gtk_window_unstick(GTK_WINDOW(TransB));
      }
   }
   ui_set_sticky(Flags.AllWorkspaces);
}

// here we are handling the buttons in ui
// Ok, this is a long list, and could be implemented more efficient.
// But, do_ui_check is called not too frequently, so....
// Note: if changes != 0, the settings will be written to .xsnowrc
//
int do_ui_check(UNUSED gpointer data)
{
   if (Flags.Done)
      gtk_main_quit();

   if (Flags.NoMenu)
      return TRUE;

   Santa_ui();
   scenery_ui();
   birds_ui();
   snow_ui();
   meteo_ui();
   wind_ui();
   stars_ui();
   fallensnow_ui();
   blowoff_ui();
   treesnow_ui();
   moon_ui();
   ui_ui();

   UIDO (WantWindow          , change_ww();                     );
   UIDO (CpuLoad             , HandleCpuFactor();               );
   UIDO (Transparency        ,                                  );
   UIDO (UseBG               , SetGCFunctions(); ClearScreen(); );
   UIDOS(BGColor             , 
	 if(Flags.UseBG)
	 {
	 SetGCFunctions();
	 ClearScreen();
	 }
	);
   UIDO (Exposures           ,
	 HandleExposures();
	 HandleCpuFactor();
	 ClearScreen();
	);
   UIDO (OffsetS             ,                                  );
   UIDO (OffsetY             ,                                  );
   UIDO (NoFluffy            , ClearScreen();                   );
   UIDO (MoveWindow          , myDetermineWindow();             );
   UIDO (AllWorkspaces       , DoAllWorkspaces();               );
   UIDO (BelowAll            , set_below_above();               );

   if (Flags.Changes > 0)
   {
      P("WriteFlags\n");
      WriteFlags();
      P("-----------Changes: %d\n",Flags.Changes);
   }
   Flags.Changes = 0;
   return TRUE;
}


int do_displaychanged(UNUSED gpointer data)
{
   // if we are snowing in the desktop, we check if the size has changed,
   // this can happen after changing of the displays settings
   // If the size has been changed, we restart the program
   if (Flags.Done)
      return FALSE;
   P("UseGtk: %d Trans: %d Root: %d DrawBirds: %d Exposures: %d Desktop: %d\n",
	 switches.UseGtk, switches.Trans, switches.Root, switches.DrawBirds, 
	 switches.Exposures, switches.Desktop);

   if (!switches.Desktop)
      return TRUE;
   {
      unsigned int w,h;
      Display* display = XOpenDisplay(Flags.DisplayName);
      Screen* screen   = DefaultScreenOfDisplay(display);
      w = WidthOfScreen(screen);
      h = HeightOfScreen(screen);
      P("width height: %d %d\n",w,h);
      if(Wroot != w || Hroot != h)
      {
	 DoRestart = 1;
	 Flags.Done = 1;
	 printf("Restart due to change of display settings...\n");
      }
      XCloseDisplay(display);
      return TRUE;
   }
}

int do_event(UNUSED gpointer data)
{
   if (Flags.Done)
      return FALSE;
   //if(switches.Trans) return; we are tempted, but if the event loop is escaped,
   // a memory leak appears
   XEvent ev;
   XFlush(display);
   while (XPending(display)) 
   {
      XNextEvent(display, &ev);
      if(!switches.Trans) 
      {
	 switch (ev.type) 
	 {
	    case ConfigureNotify:
	       P("ConfigureNotify: r=%ld w=%ld geo=(%d,%d,%d,%d) bw=%d root: %d\n", 
		     ev.xconfigure.event,
		     ev.xconfigure.window,
		     ev.xconfigure.x,
		     ev.xconfigure.y,
		     ev.xconfigure.width,
		     ev.xconfigure.height,
		     ev.xconfigure.border_width,
		     (SnowWin == ev.xconfigure.event)  
		);
	       if (ev.xconfigure.window == SnowWin &&
		     (ev.xconfigure.width != SnowWinWidth ||
		      ev.xconfigure.height != SnowWinHeight))
	       {
		  P("init %d %d\n",ev.xconfigure.width, ev.xconfigure.height);
		  P("Calling RestartDisplay\n");
		  RestartDisplay();
	       }
	       break;
	 } 
      }
   }  
   return TRUE;
}

void RestartDisplay()    // todo
{
   P("Calling InitDisplayDimensions\n");
   InitDisplayDimensions();
   InitFallenSnow();
   init_stars();
   EraseTrees();
   if(!Flags.NoKeepSnowOnTrees && !Flags.NoTrees)
   {
      reinit_treesnow_region();
   }
   if(!Flags.NoTrees)
   {
      XDestroyRegion(TreeRegion);
      TreeRegion = XCreateRegion();
   }
   XClearArea(display, SnowWin, 0,0,0,0,switches.Exposures);
}



int do_show_range_etc(UNUSED gpointer data)
{
   if (Flags.Done)
      return FALSE;

   if (Flags.NoMenu)
      return TRUE;
   ui_show_range_etc();
   return TRUE;
}

int do_show_desktop_type(UNUSED gpointer data)
{
   P("do_show_desktop_type %d\n",counter++);
   if (Flags.NoMenu)
      return TRUE;
   char *s;
   if (IsWayland)
      s = (char *)"Wayland (Expect some slugginess)";
   else if (IsCompiz)
      s = (char *)"Compiz";
   else
      s = (char *)"Probably X11";
   char t[128];
   snprintf(t,64,"%s. Snow window: %#lx",s,SnowWin);
   ui_show_desktop_type(t);
   return TRUE;
}


int do_testing(UNUSED gpointer data)
{
   counter++;
   //Flags.ThemeXsnow = 1-Flags.ThemeXsnow;
   P("testing ThemeXsnow:%d\n",Flags.ThemeXsnow);
   if (Flags.Done)
      return FALSE;
   return TRUE;
   P("Flakecount: %d FluffCount: %d\n",FlakeCount,FluffCount);
   if (FluffCount < 0 || FluffCount > FlakeCount)
      Flags.Done = 1;
   /*
#include "hashtable.h"
#include "snow.h"
P("flakes: %d %d\n",set_size(),FlakeCount);
*/
   return TRUE;
   Flags.BelowAll = !Flags.BelowAll;
}


/* ------------------------------------------------------------------ */ 
void SigHandler(int signum)
{
   HaltedByInterrupt = signum;
   Flags.Done        = 1;
}
/* ------------------------------------------------------------------ */ 



#if 0
// keep this in case I need the erasure code
void EraseStars()
{
   int i;
   for (i=0; i<NStars; i++)
   {
      int x = star[i].x; 
      int y = star[i].y; 
      int w = starPix.width;
      int h = starPix.height;
      if(UseAlpha|Flags.UseBG)
	 XFillRectangle(display, SnowWin, ESantaGC, 
	       x, y, w, h);
      else
	 XClearArea(display, SnowWin,
	       x, y, w, h, Exposures);
   }
}
#endif




int XsnowErrors(Display *dpy, XErrorEvent *err)
{
   char msg[1024];
   XGetErrorText(dpy, err->error_code, msg,sizeof(msg));
   if(Flags.Noisy)
      I("%d %s\n",counter++,msg);
   return 0;
}



// the draw callback
gboolean on_draw_event(UNUSED GtkWidget *widget, cairo_t *cr, UNUSED gpointer user_data) 
{
   P("Just to check who this is: %p %p\n",(void *)widget,(void *)TransA);
   drawit(cr);
   return FALSE;
}

void drawit(cairo_t *cr)
{
   P("drawit %d %p\n",counter++,(void *)cr);

   if (Flags.Done)
      return;


   int skipit = !switches.UseGtk || Flags.BirdsOnly || !WorkspaceActive();

   if (!skipit)
   {
      stars_draw(cr);
      meteo_draw(cr);
   }

   moon_draw(cr);

   birds_draw(cr);

   if (skipit)
      return;

   scenery_draw(cr);

   if(!Flags.FollowSanta || !Flags.ShowBirds) // if Flags.FollowSanta: drawing of Santa is done in birds_draw()
      Santa_draw(cr);

   treesnow_draw(cr);

   snow_draw(cr);

   fallensnow_draw(cr);

}

int do_display_dimensions(UNUSED gpointer data)
{
   if (Flags.Done)
      return FALSE;
   static int prevw = 0, prevh = 0;
   P("%d do_init_display_dimensions\n",counter);
   static int first = 1;
   if(first)
   { 
      // at the first call, nothing has to be done, except:
      prevw = SnowWinWidth;
      prevh = SnowWinHeight;
      first = 0;
      return TRUE;
   }
   DisplayDimensions();
   P("do_display_dim: prevw: %d SnowWinWidth: %d prevh: %d SnowWinHeight: %d\n",prevw,SnowWinWidth,prevh,SnowWinHeight);
   if (prevw != SnowWinWidth || prevh != SnowWinHeight)
   {
      if (prevw == SnowWinWidth)
      {
	 // search fallensnow at bottom
	 FallenSnow *f = FindFallen(FsnowFirst,0);
	 if (f)
	    f->y = SnowWinHeight;
      }
      else
	 InitFallenSnow();
      P("%d do_display_dimensions\n",counter++);
      init_stars();
      EraseTrees();
      ClearScreen();
      prevw = SnowWinWidth;
      prevh = SnowWinHeight;
   }
   return TRUE;
}

int do_draw_all(gpointer widget)
{
   // called via drawit()
   if (Flags.Done)
      return FALSE;
   P("do_draw_all %d %p\n",counter++,(void *)widget);

   gtk_widget_queue_draw(GTK_WIDGET(widget));
   return TRUE;
}


// handle callbacks for things whose timings depend on cpufactor
void HandleCpuFactor()
{
   static guint fallen_id=0;

   // re-add things whose timing is dependent on cpufactor
   if (Flags.CpuLoad <= 0)
      cpufactor = 1;
   else
      cpufactor = 100.0/Flags.CpuLoad;

   if (fallen_id)
      g_source_remove(fallen_id);

   Santa_HandleCpuFactor();

   fallen_id = add_to_mainloop(PRIORITY_DEFAULT, time_fallen, do_fallen, NULL);
   P("handlecpufactor %f %f %d\n",oldcpufactor,cpufactor,counter++);
   add_to_mainloop(PRIORITY_HIGH, 0.2 , do_initsnow, NULL);  // remove flakes

   restart_do_draw_all();
}

void restart_do_draw_all()
{
   if (!TransA)
      return;
   if (draw_all_id)
      g_source_remove(draw_all_id);
   draw_all_id = add_to_mainloop(PRIORITY_HIGH, time_draw_all, do_draw_all, TransA);
   P("started do_draw_all %d %p %f\n",draw_all_id, (void *)TransA, time_draw_all);
}


void SetGCFunctions()
{
   if (Flags.UseBG)
      ErasePixel = AllocNamedColor(Flags.BGColor,Black) | 0xff000000;
   else
      ErasePixel = 0;

   Santa_set_gc();
   P("Santa_set_gc\n");

   scenery_set_gc();

   treesnow_set_gc();

   snow_set_gc();

   stars_set_gc();

   fallensnow_set_gc();

}


void HandleExposures()
{
   if (Flags.Exposures == -SOMENUMBER) // no -exposures or -noexposures given
      if (Flags.XWinInfoHandling)
	 switches.Exposures = 1;
      else
	 switches.Exposures = 0;
   else
      switches.Exposures = Flags.Exposures;
}


int do_stopafter(UNUSED gpointer data)
{
   Flags.Done = 1;
   printf("Halting because of flag -stopafter\n");
   return FALSE;
}
