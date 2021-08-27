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
#include <X11/extensions/Xdbe.h>
#include <ctype.h>
#include <gtk/gtk.h>
#include <cairo-xlib.h>
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
#include "transwindow.h"
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

#include "vroot.h"

#ifdef DEBUG
#undef DEBUG
#endif
//#define DEBUG

#define BMETHOD XdbeBackground
//#define BMETHOD XdbeUndefined
//#define BMETHOD XdbeUntouched
//#define BMETHOD XdbeCopied       // to see if double buffering is actually used

struct _global global;

int              SnowWinChanged = 1;
cairo_t         *CairoDC = NULL;
cairo_surface_t *CairoSurface = NULL;



// miscellaneous
char       Copyright[] = "\nXsnow\nCopyright 1984,1988,1990,1993-1995,2000-2001 by Rick Jansen, all rights reserved, 2019,2020 also by Willem Vermin\n";

static char **Argv;
static int Argc;

// timing stuff
//static double       TotSleepTime = 0;

// windows stuff
static int          DoRestart      = 0;
static guint        draw_all_id    = 0;
static guint        drawit_id      = 0;
static int          Xorig;
static int          Yorig;
static GtkWidget   *TransA         = NULL;
static char        *SnowWinName    = NULL;

/* Colo(u)rs */
static const char *BlackColor  = "black";
static Pixel       BlackPix;

/* Forward decls */
static void   HandleCpuFactor(void);
static void   RestartDisplay(void);
static void   SigHandler(int signum);
static int    XsnowErrors(Display *dpy, XErrorEvent *err);
static void   drawit(cairo_t *cr);
static void   restart_do_draw_all(void);
static void   set_below_above(void);
static void   DoAllWorkspaces(void);
static void   X11WindowById(Window *xwin, char **xwinname);
static int    HandleX11Cairo(void);
static int    StartWindow(void);
static void   SetWindowScale(void);
static void   movewindow(void);


// callbacks
static int do_displaychanged(void *);
static int do_draw_all(gpointer widget);
static int do_event(void *);
static int do_show_range_etc(void *);
static int do_testing(void *);
static int do_ui_check(void *);
static int do_stopafter(void *);
static int do_show_desktop_type(void *);
static int do_display_dimensions(void *);
static int do_drawit(void*);
static gboolean     on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data);

/**********************************************************************************************/

/* About some technicalities of this program

 * The following animations exist:
 * ===============================
 *   - snow
 *   - treesnow
 *   - blowoff
 *   - Santa
 *   - scenery (trees etc)
 *   - stars
 *   - moon + halo
 *   - meteorites
 *   - birds
 *
 * Selection of window to draw in
 * ==============================
 *
 * All drawing is done using the gtk-cairo library, using the window SnowWin.
 *
 * For the definition of SnowWin, the following possibilities exist:
 * 
 * A: SnowWin is a transparent, click-through window covering the
 *    whole screen.
 * B: SnowWin is set to the root window, or a window with the name pcmanfm (LXDE desktop).
 * C: SnowWin is set to the desired window (flags -id or -xwininfo).
 *
 *
 * If the user specifies the flag -id or -xwininfo, case C is chosen 
 *    (make_trans_window() in transwindow.c).
 * else if a transparent, click-through window can be created, case A is chosen.
 * else case B is chosen.
 *
 * In cases B and C, usage is made of cairo_xlib_surface_create() to tell cairo
 * in which window to paint.
 *
 * Double buffering
 * ================
 *
 * In case A, double buffering is provided by the cairo library.
 * In the other cases, double buffering is accomplished by XdbeSwapBuffers().
 *
 * If cases B and C, if double buffering is not possible, or not wanted
 *   (flag -doublebuffer), an attempt is made to erase and draw with minimal flicker.
 *
 * Double buffer switches
 * ======================
 *
 * Trans:     1: case A
 * UseDouble: 1: case B or C using XdbeSwapBuffers() and friends. 
 * IsDouble:  1: using double buffering
 *
 * The following scenario's exist:
 *
 * Scenario Trans   UseDouble   IsDouble
 *   I        1         0          1            case A
 *   II       0         1          1            case B or C, using XdbeSwapBuffers and friends
 *   III      0         0          0            case B or C, not using double buffering
 *
 * Note: 
 * In scenario III it is necessary to explicitly erase moving objects before 
 * drawing.
 * One could use XlearWindow() to erase everything, and then draw everything again,
 * but this results in severe flicker.
 *
 * In scenario's I and II, it is necessary to repaint everything, also not moving objects.
 *
 */
/*
 * Globals
 * =======
 *
 * Many gobal variables are used. If a global is only used in one file only, it is declared
 * there as static.
 * Globals that are used in more than one file, are members of the struct 'global', defined in xsnow.h
 *
 * Types
 * =====
 *
 * All types are defined in xsnow.h
 *
 */

/**********************************************************************************************/

int main_c(int argc, char *argv[])
{
   P("This is xsnow\n");
   signal(SIGINT,  SigHandler);
   signal(SIGTERM, SigHandler);
   signal(SIGHUP,  SigHandler);
   srand48((long int)(wallcl()*1.0e6));


   memset((void *)&global,0,sizeof(global));

   global.counter            = 0;
   global.cpufactor          = 1.0;
   global.WindowScale        = 1.0;

   global.MaxSnowFlakeHeight = 0;
   global.MaxSnowFlakeWidth  = 0;
   global.FlakeCount         = 0;  /* # active flakes */
   global.FluffCount         = 0;

   global.SnowWin            = 0;
   global.DesktopSession     = NULL;
   global.CWorkSpace         = 0;
   global.WindowsChanged     = 0;

   global.FsnowFirst         = NULL;
   global.MaxScrSnowDepth    = 0;
   global.RemoveFluff        = 0;

   global.SnowOnTrees        = NULL;
   global.OnTrees            = 0;

   global.moonX              = 1000;
   global.moonY              = 80;

   global.Wind               = 0;
   global.Direction          = 0;
   global.WindMax            = 100.0;
   global.NewWind = 0;

   global.HaltedByInterrupt  = 0;
   global.Message[0]         = 0;

   global.SantaPlowRegion    = 0;

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

   setenv("GDK_BACKEND","x11",1);
   if (getenv("WAYLAND_DISPLAY")&&getenv("WAYLAND_DISPLAY")[0])
   {
      printf("Detected Wayland desktop\n");
      global.IsWayland = 1;
   }
   else
      global.IsWayland = 0;

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

   global.display = XOpenDisplay(Flags.DisplayName);
   XSynchronize(global.display,dosync);
   XSetErrorHandler(XsnowErrors);
   int screen = DefaultScreen(global.display);
   global.Black = BlackPixel(global.display, screen);
   global.White = WhitePixel(global.display, screen);


   InitSnowOnTrees();

   if (global.display == NULL) {
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



   if (!StartWindow())
   {
      return 1;
   }

   Flags.Done = 0;
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

   add_to_mainloop(PRIORITY_DEFAULT, time_displaychanged, do_displaychanged     );
   add_to_mainloop(PRIORITY_DEFAULT, time_event,          do_event              );
   add_to_mainloop(PRIORITY_DEFAULT, time_testing,        do_testing            );
   add_to_mainloop(PRIORITY_DEFAULT, 0.5,                 do_display_dimensions );
   add_to_mainloop(PRIORITY_HIGH,    time_ui_check,       do_ui_check           );
   add_to_mainloop(PRIORITY_DEFAULT, time_show_range_etc, do_show_range_etc     );

   if (Flags.StopAfter > 0)
      add_to_mainloop(PRIORITY_DEFAULT, Flags.StopAfter, do_stopafter);

   HandleCpuFactor();

#define DOIT_I(x,d,v) OldFlags.x = Flags.x;
#define DOIT_L(x,d,v) DOIT_I(x,d,v);
#define DOIT_S(x,d,v) OldFlags.x = strdup(Flags.x);
   DOITALL;
#include "undefall.inc"


   OldFlags.FullScreen = !Flags.FullScreen;

   BlackPix = AllocNamedColor(BlackColor, global.Black);

   // events
   if(global.Desktop)
   {
      XSelectInput(global.display, global.Rootwindow, StructureNotifyMask| SubstructureNotifyMask);
   }
   else
      XSelectInput(global.display, global.SnowWin, StructureNotifyMask|SubstructureNotifyMask);

   ClearScreen();   // without this, no snow, scenery etc. in KDE; this seems to be a vintage comment

   if(!Flags.NoMenu)
   {
      ui();
      ui_gray_erase(global.Trans);
      ui_set_sticky(Flags.AllWorkspaces);
      add_to_mainloop(PRIORITY_DEFAULT, 2.0, do_show_desktop_type);
   }

   // main loop
   gtk_main();


   if (SnowWinName) free(SnowWinName);

   XClearWindow(global.display, global.SnowWin);
   XFlush(global.display);
   XCloseDisplay(global.display); //also frees the GC's, pixmaps and other resources on display

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
   P("%d set_below_above\n",global.counter++);
   XWindowChanges changes;
   // to be sure: we do it in gtk mode and window mode
   if (Flags.BelowAll)
   {
      if(TransA)setbelow(GTK_WINDOW(TransA));
      changes.stack_mode = Below;
      if(global.SnowWin)XConfigureWindow(global.display,global.SnowWin,CWStackMode,&changes);
   }
   else
   {
      if(TransA)setabove(GTK_WINDOW(TransA));
      changes.stack_mode = Above;
      if(global.SnowWin)XConfigureWindow(global.display,global.SnowWin,CWStackMode,&changes);
   }
}

void X11WindowById(Window *xwin, char **xwinname)
{
   *xwin = 0;
   // user supplied window id:
   if (Flags.WindowId)
   {
      *xwin = Flags.WindowId;
      return;
   }
   if (Flags.XWinInfoHandling)
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
   return;
}

int StartWindow()
{
   int X11cairo     = 0;

   global.Trans     = 0;
   global.xxposures = 0;
   global.Desktop   = 0;
   global.UseDouble = 0;
   global.IsDouble  = 0;

   global.Rootwindow = DefaultRootWindow(global.display);
   Window xwin;
   // see if user chooses window
   X11WindowById(&xwin, NULL);
   if (xwin)
   {
      P("StartWindow xwin%#lx\n",xwin);
      global.SnowWin           = xwin;
      X11cairo = 1;
   }
   else if(Flags.ForceRoot)
   {
      // user specified to run on root window
      printf("Trying to snow in root window\n");
      global.SnowWin = global.Rootwindow;
      if (getenv("XSCREENSAVER_WINDOW"))
      {
	 global.SnowWin    = strtol(getenv("XSCREENSAVER_WINDOW"),NULL,0);
	 global.Rootwindow = global.SnowWin;
      }
      X11cairo = 1;
   }
   else
   {
      // default behaviour
      // try to create a transparent clickthrough window
      GtkWidget *gtkwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
      int rc = make_trans_window(gtkwin,
	    1,                   // full screen 
	    Flags.AllWorkspaces, // sticky 
	    Flags.BelowAll,      // below
	    1,                   // dock
	    NULL,                // gdk_window
	    &xwin                // x11_window
	    );
      if (rc)
      {
	 global.Trans            = 1;
	 global.IsDouble         = 1;
	 global.Desktop          = 1;
	 TransA                  = gtkwin;
	 GtkWidget *drawing_area = gtk_drawing_area_new();
	 gtk_container_add(GTK_CONTAINER(TransA), drawing_area);
	 g_signal_connect(TransA, "draw", G_CALLBACK(on_draw_event), NULL);
	 set_below_above();
	 global.SnowWin = xwin;
	 printf("Using transparent window\n");
      }
      else
      {
	 global.Desktop  = 1;
	 X11cairo = 1;
	 // use rootwindow, pcmanfm or Desktop:
	 printf("Cannot create transparent window\n");
	 if (global.DesktopSession == NULL)
	 {
	    char *desktopsession = NULL;
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
	 // if envvar DESKTOP_SESSION == LXDE, search for window with name pcmanfm
	 if (global.DesktopSession != NULL && 
	       !strncmp(global.DesktopSession,"LXDE",4) && 
	       (xwin = FindWindowWithName(global.display,"pcmanfm")))
	 {
	    printf("LXDE session found, using window 'pcmanfm'.\n");
	    P("lxdefound: %d %#lx\n",lxdefound,*xwin);
	 }
	 else if ((xwin = FindWindowWithName(global.display,"Desktop")))
	 {
	    printf("Using window 'Desktop'.\n");
	 }
	 else
	 {
	    printf("Using root window\n");
	    xwin = global.Rootwindow;
	 }
	 global.SnowWin = xwin;
      }
   }

   if(X11cairo)
   {
      HandleX11Cairo();
      drawit_id = add_to_mainloop1(PRIORITY_HIGH, time_draw_all, do_drawit, CairoDC);
   }

   global.IsDouble = global.Trans || global.UseDouble; // just to be sure ...

   XTextProperty x;
   if (XGetWMName(global.display,xwin,&x))
      SnowWinName = strdup((char *)x.value);
   else
      SnowWinName = strdup("no name");
   XFree(x.value);
   InitDisplayDimensions();
   I("Snowing in %#lx: %s %d+%d %dx%d\n",global.SnowWin,SnowWinName,global.SnowWinX,global.SnowWinY,global.SnowWinWidth,global.SnowWinHeight);

   Xorig = global.SnowWinX;
   Yorig = global.SnowWinY;

   movewindow();

   SetWindowScale();

   return TRUE;
}

int HandleX11Cairo()
{
   XWindowAttributes attr;
   XGetWindowAttributes(global.display,global.SnowWin,&attr);
   int w = attr.width;
   int h = attr.height;
   Visual *visual = DefaultVisual(global.display,DefaultScreen(global.display));
   int rcv;
   P("double: %d\n",Flags.UseDouble);
#ifdef XDBE_AVAILABLE
   int dodouble = Flags.UseDouble;
#else
   int dodouble = 0;
#endif
#ifdef XDBE_AVAILABLE
   if(dodouble)
   {
      static Drawable backBuf = 0;
      if(backBuf)
	 XdbeDeallocateBackBufferName(global.display,backBuf);
      backBuf = XdbeAllocateBackBufferName(global.display,global.SnowWin,BMETHOD);
      if (CairoSurface)
	 cairo_surface_destroy(CairoSurface);
      CairoSurface = cairo_xlib_surface_create(global.display, backBuf, visual, w, h);
      global.UseDouble = 1;
      global.IsDouble  = 1;
      printf("Using double buffer: %#lx.\n",backBuf);
      rcv = TRUE;
   }
#endif
   if(!dodouble)
   {
      CairoSurface = cairo_xlib_surface_create(global.display, global.SnowWin, visual, w, h);
      printf("NOT using double buffering:");
      if (Flags.UseDouble)
	 printf(" because double buffering is not available on this system\n");
      else
	 printf(" on your request.\n");
      rcv = FALSE;
   }

   if (CairoDC)
      cairo_destroy(CairoDC);

   CairoDC = cairo_create(CairoSurface);
   cairo_xlib_surface_set_size(CairoSurface,w,h);
   return rcv;
}

void DoAllWorkspaces()
{
   if(Flags.AllWorkspaces)
   {
      P("stick\n");
      if (global.Trans)
      {
	 gtk_window_stick(GTK_WINDOW(TransA));
      }
   }
   else
   {
      P("unstick\n");
      if (global.Trans)
      {
	 gtk_window_unstick(GTK_WINDOW(TransA));
      }
   }
   ui_set_sticky(Flags.AllWorkspaces);
}

// here we are handling the buttons in ui
// Ok, this is a long list, and could be implemented more efficient.
// But, do_ui_check is called not too frequently, so....
// Note: if changes != 0, the settings will be written to .xsnowrc
//
int do_ui_check(void *d)
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

   UIDO (CpuLoad             , HandleCpuFactor();               );
   UIDO (Transparency        ,                                  );
   UIDO (Scale               ,                                  );
   UIDO (OffsetS             , DisplayDimensions();             );
   UIDO (OffsetY             , UpdateFallenSnowRegions();       );
   UIDO (NoFluffy            , ClearScreen();                   );
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
   (void)d;
}


void movewindow()
{
   if(!global.Desktop)
      return;
   XMoveWindow(global.display,global.SnowWin,0,0);
   //InitDisplayDimensions();
   P("movewindow: Orig: %d %d %d %d\n",Xorig,Yorig,global.SnowWinWidth,global.SnowWinHeight);
}

int do_displaychanged(void *d)
{
   // if we are snowing in the desktop, we check if the size has changed,
   // this can happen after changing of the displays settings
   // If the size has been changed, we restart the program
   (void)d;
   if (Flags.Done)
      return FALSE;
   P("Trans: %d xxposures: %d Desktop: %d\n",
	 global.Trans,   
	 global.xxposures, global.Desktop);

   if (!global.Desktop)
      return TRUE;
   {
      unsigned int w,h;
      Display* display = XOpenDisplay(Flags.DisplayName);
      Screen* screen   = DefaultScreenOfDisplay(display);
      w = WidthOfScreen(screen);
      h = HeightOfScreen(screen);
      P("width height: %d %d\n",w,h);
      if(global.Wroot != w || global.Hroot != h)
      {
	 DoRestart = 1;
	 Flags.Done = 1;
	 printf("Restart due to change of display settings...\n");
      }
      XCloseDisplay(display);
      return TRUE;
   }
}

int do_event(void *d)
{
   (void)d;
   P("do_event %d\n",counter++);
   if (Flags.Done)
      return FALSE;
   XEvent ev;
   XFlush(global.display);
   while (XPending(global.display)) 
   {
      XNextEvent(global.display, &ev);
      {
	 if (ev.type == ConfigureNotify || ev.type == MapNotify
	       || ev.type == UnmapNotify) 
	 {
	    global.WindowsChanged++;
	    P("WindowsChanged %d %d\n",counter++,global.WindowsChanged);
	 }
	 switch (ev.type) 
	 {
	    case ConfigureNotify:
	       P("ConfigureNotify: ev=%ld win=%#lx geo=(%d,%d,%d,%d) bw=%d root: %d\n", 
		     ev.xconfigure.event,
		     ev.xconfigure.window,
		     ev.xconfigure.x,
		     ev.xconfigure.y,
		     ev.xconfigure.width,
		     ev.xconfigure.height,
		     ev.xconfigure.border_width,
		     (global.SnowWin == ev.xconfigure.event)  
		);
	       if (ev.xconfigure.window == global.SnowWin)
	       {
		  SnowWinChanged = 1;
	       }
	       break;
	 } 
      }
   }  
   return TRUE;
}

void RestartDisplay()
{
   P("Restartdisplay: %d W: %d H: %d\n",counter++,global.SnowWinWidth,global.SnowWinHeight);
   InitFallenSnow();
   init_stars();
   EraseTrees();
   if(!Flags.NoKeepSnowOnTrees && !Flags.NoTrees)
   {
      reinit_treesnow_region();
   }
   if(!Flags.NoTrees)
   {
      cairo_region_destroy(global.TreeRegion);
      global.TreeRegion = cairo_region_create();
   }
   if(!global.IsDouble)
      ClearScreen();
}



int do_show_range_etc(void *d)
{
   if (Flags.Done)
      return FALSE;

   if (Flags.NoMenu)
      return TRUE;
   ui_show_range_etc();
   return TRUE;
   (void)d;
}

int do_show_desktop_type(void *d)
{
   P("do_show_desktop_type %d\n",counter++);
   if (Flags.NoMenu)
      return TRUE;
   char *s;
   if (global.IsWayland)
      s = (char *)"Wayland (Expect some slugginess)";
   else if (global.IsCompiz)
      s = (char *)"Compiz";
   else
      s = (char *)"Probably X11";
   char t[128];
   snprintf(t,64,"%s. Snow window: %#lx",s,global.SnowWin);
   ui_show_desktop_type(t);
   return TRUE;
   (void)d;
}


int do_testing(void *d)
{
   (void)d;
   if (Flags.Done)
      return FALSE;

   return TRUE;
   global.counter++;
   XWindowAttributes attr;
   XGetWindowAttributes(global.display,global.SnowWin,&attr);

   P("%d wxh %d %d %d %d    %d %d %d %d \n",counter,SnowWinX,SnowWinY,global.SnowWinWidth,global.SnowWinHeight,attr.x,attr.y,attr.width,attr.height);

   return TRUE;
}


void SigHandler(int signum)
{
   global.HaltedByInterrupt = signum;
   Flags.Done        = 1;
}

int XsnowErrors(Display *dpy, XErrorEvent *err)
{
   static int count   = 0;
   const int countmax = 1000;
   char msg[1024];
   XGetErrorText(dpy, err->error_code, msg,sizeof(msg));
   if(Flags.Noisy)
      I("%d %s\n",global.counter++,msg);

   if (++count > countmax)
   {
      snprintf(global.Message,sizeof(global.Message),"More than %d errors, I quit!",countmax);
      Flags.Done = 1;
   }
   return 0;
}



// the draw callback
gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data) 
{
   P("Just to check who this is: %p %p\n",(void *)widget,(void *)TransA);
   drawit(cr);
   return FALSE;
   (void)widget;
   (void)user_data;
}

int do_drawit(void *cr)
{
   P("do_drawit %d %p\n",counter++,cr);
   drawit((cairo_t*)cr);
   return TRUE;
}

void drawit(cairo_t *cr)
{
   P("drawit %d %p\n",counter++,(void *)cr);

   if (Flags.Done)
      return;

   if(global.UseDouble)
   {
      XdbeSwapInfo swapInfo;
      swapInfo.swap_window = global.SnowWin;
      swapInfo.swap_action = BMETHOD;
      XdbeSwapBuffers(global.display,&swapInfo,1);
   }
   else if(!global.IsDouble)
   {
      XFlush(global.display);
      moon_erase (0);
      Santa_erase(cr);
      stars_erase(); // not really necessary
      birds_erase(0);
      snow_erase(1);
      XFlush(global.display);
   }

   int skipit = Flags.BirdsOnly || !WorkspaceActive();

   if (!skipit)
   {
      stars_draw(cr);
      meteo_draw(cr);

      P("moon\n");
      moon_draw(cr);
      scenery_draw(cr);
   }

   P("birds %d\n",counter++);
   birds_draw(cr);

   if (skipit)
      return;

   fallensnow_draw(cr);

   if(!Flags.FollowSanta || !Flags.ShowBirds) // if Flags.FollowSanta: drawing of Santa is done in birds_draw()
      Santa_draw(cr);

   treesnow_draw(cr);

   snow_draw(cr);

   XFlush(global.display);

}

void SetWindowScale()
{
   // assuming a standard screen of 1024x576, we suggest to use the scalefactor
   // WindowScale
   float x = global.SnowWinWidth / 1000.0;
   float y = global.SnowWinHeight / 576.0;
   if (x < y)
      global.WindowScale = x;
   else
      global.WindowScale = y;
   P("WindowScale: %f\n",global.WindowScale);
}

int do_display_dimensions(void *d)
{
   (void)d;
   if (Flags.Done)
      return FALSE;
   if (!SnowWinChanged)
      return TRUE;
   SnowWinChanged = 0;
   static int prevw = 0, prevh = 0;
   P("%d do_display_dimensions %d %d\n",counter++,global.SnowWinWidth,global.SnowWinHeight);
   DisplayDimensions();
   if (prevw != global.SnowWinWidth || prevh != global.SnowWinHeight)
   {
      // if(global.X11cairo) // let op
      if (!global.Trans)
      {
	 HandleX11Cairo();
	 RestartDisplay();
      }
      prevw = global.SnowWinWidth;
      prevh = global.SnowWinHeight;
      SetWindowScale();
   }
   return TRUE;
}

int do_draw_all(gpointer widget)
{
   if (Flags.Done)
      return FALSE;
   P("do_draw_all %d %p\n",counter++,(void *)widget);

   // this will result in a call off on_draw_event():
   gtk_widget_queue_draw(GTK_WIDGET(widget));
   return TRUE;
}


// handle callbacks for things whose timings depend on cpufactor
void HandleCpuFactor()
{
   static guint fallen_id=0;

   // re-add things whose timing is dependent on cpufactor
   if (Flags.CpuLoad <= 0)
      global.cpufactor = 1;
   else
      global.cpufactor = 100.0/Flags.CpuLoad;

   if (fallen_id)
      g_source_remove(fallen_id);

   fallen_id = add_to_mainloop(PRIORITY_DEFAULT, time_fallen, do_fallen);
   P("handlecpufactor %f %f %d\n",oldcpufactor,cpufactor,counter++);
   add_to_mainloop(PRIORITY_HIGH, 0.2 , do_initsnow);  // remove flakes

   restart_do_draw_all();
}

void restart_do_draw_all()
{
   if (global.Trans)
   {
      if (draw_all_id)
	 g_source_remove(draw_all_id);
      draw_all_id = add_to_mainloop1(PRIORITY_HIGH, time_draw_all, do_draw_all, TransA);
      P("started do_draw_all %d %p %f\n",draw_all_id, (void *)TransA, time_draw_all);
   }
   else
   {
      if (drawit_id)
	 g_source_remove(drawit_id);
      drawit_id = add_to_mainloop1(PRIORITY_HIGH, time_draw_all, do_drawit, CairoDC);
      P("started do_drawit %d %p %f\n",drawit_id, (void *)CairoDC, time_draw_all);
   }
}


int do_stopafter(void *d)
{
   Flags.Done = 1;
   printf("Halting because of flag -stopafter\n");
   return FALSE;
   (void)d;
}
