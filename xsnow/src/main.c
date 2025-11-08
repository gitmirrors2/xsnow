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
 *
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
#include <pthread.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <X11/Intrinsic.h>
#include <X11/extensions/Xdbe.h>
#include <X11/extensions/Xinerama.h>
#include <ctype.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <cairo-xlib.h>
#include <gsl/gsl_version.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "mygettext.h"

#include "xsnow-constants.h"

#include "aurora.h"
#include "birds.h"
#include "clocks.h"
#include "docs.h"
#include "dsimple.h"
#include "fallensnow.h"
#include "flags.h"
#include "mainstub.h"
#include "meteor.h"
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
#include "selfrep.h"
#include "xdo.h"

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

int lxdefound = False;
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
static GtkWidget   *TransA         = NULL;
static char        *SnowWinName    = NULL;
static int          wantx          = 0;
static int          wanty          = 0;
static int          IsSticky       = 0;
static int          X11cairo       = 0;
static int          PrevW          = 0;
static int          PrevH          = 0;

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
static void   X11WindowById(Window *xwin);
static int    HandleX11Cairo(void);
static int    StartWindow(void);
static void   SetWindowScale(void);
static void   GetDesktopSession(void);
static void   rectangle_draw(cairo_t *cr);
static void   mybindtestdomain(void);
static void   PrintAbsoluteCoordinates(void);

// callbacks
static int do_displaychanged(void *);
static int do_write_flags(void *);
static int do_draw_all(gpointer widget);
static int do_event(void *);
static int do_show_range_etc(void *);
static int do_testing(void *);
static int do_ui_check(void *);
static int do_stopafter(void *);
static int do_show_desktop_type(void *);
static int do_display_dimensions(void *);
static int do_drawit(void *);
static int do_check_stop(void *);
static gboolean     on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data);

static volatile int signal_caught = 0;
static char *StopFile;
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
 *   - meteors
 *   - birds
 *   - aurora
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
   signal(SIGUSR1, SigHandler);
   P("srand %d\n",(int)(fmod(wallcl()*1.0e6,1.0e8)));
   srand48((int)(fmod(wallcl()*1.0e6,1.0e8)));

   const char *STOPFILE = ".xsnowstop";

   memset(&global,0,sizeof(global));

   XInitThreads();

   fallen_sem_init();
   aurora_sem_init();
   birds_sem_init();

   global.counter             = 0;
   global.cpufactor           = 1.0;
   global.WindowScale         = 1.0;

   global.MaxSnowFlakeHeight  = 0;
   global.MaxSnowFlakeWidth   = 0;
   global.FlakeCount          = 0;  /* # active flakes */
   global.FluffCount          = 0;

   global.SnowWin             = 0;
   global.WindowOffsetX       = 0;
   global.WindowOffsetY       = 0;
   global.DesktopSession      = NULL;
   global.CWorkSpace          = 0;
   global.ChosenWorkSpace     = 0;
   global.NVisWorkSpaces      = 1;
   global.VisWorkSpaces[0]    = 0;
   global.WindowsChanged      = 0;
   global.ForceRestart        = 0;

   global.FsnowFirst          = NULL;
   global.MaxScrSnowDepth     = 0;
   global.RemoveFluff         = 0;

   global.SnowOnTrees         = NULL;
   global.OnTrees             = 0;

   global.moonX               = 1000;
   global.moonY               = 80;

   global.Wind                = 0;
   global.Direction           = 0;
   global.WindMax             = 500.0;
   global.NewWind             = 100.0;

   global.HaltedByInterrupt   = 0;
   global.Message[0]          = 0;

   global.SantaPlowRegion     = 0;

   global.DoCapella           = DOCAPELLA;
   global.time_to_write_flags = TRUE;

   StopFile = (char*) malloc(sizeof(char)*(strlen(getenv("HOME"))+1+strlen(STOPFILE)+1));
   assert(StopFile);
   strcpy(StopFile,getenv("HOME"));
   strcat(StopFile,"/");
   strcat(StopFile,STOPFILE);

   InitFlags();
   // we search for flags that only produce output to stdout,
   // to enable to run in a non-X environment, in which case 
   // gtk_init() would fail.
   for (int i=0; i<argc; i++)
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
      else if (!strcmp(arg, "-changelog"))
      {
	 docs_changelog();
	 return 0;
      }
#ifdef SELFREP
      else if (!strcmp(arg, "-selfrep"))
      {
	 selfrep();
	 return 0;
      }
#endif
   }
   int rc = HandleFlags(argc, argv);

   handle_language(0); // the langues used is from flags or .xsnowrc
		       //                     this changes env LANGUAGE accordingly
		       //                     so that the desired translation is in effect

   mybindtestdomain();
   switch(rc)
   {
      case -1:   // wrong flag
		 //PrintVersion();
	 Thanks();
	 return 1;
	 break;
      case 1:    // manpage or help
		 //         Ok, this cannot happen, is already caught above
	 return 0;
	 break;
      default:  // ditto
		//PrintVersion();
	 break;
   }
   PrintVersion();

   printf(_("Available languages are: %s\n"),LANGUAGES);
   /* Eskimo warning */
   if (strstr(Flags.SnowColor,"yellow") != NULL)
      printf("\n%s\n\n",_("Warning: don't eat yellow snow!"));

   // make a copy of all flags, before gtk_init() maybe removes some.
   // we need this at a restart of the program.
   // we need to remove 
   //   -screen n
   //   -lang c

   Argv = (char**) malloc((argc+1)*sizeof(char**));
   assert(Argv);
   Argc = 0;
   for (int i=0; i<argc; i++)
   {
      if ((strcmp("-screen",argv[i])==0) || (strcmp("-lang",argv[i])==0))
	 i++; // found -screen or -lang
      else 
	 Argv[Argc++] = strdup(argv[i]);
   }
   Argv[Argc] = NULL;
   GetDesktopSession();
   assert(global.DesktopSession);
   if(!strncasecmp(global.DesktopSession,"bspwm",5))
   {
      printf(_("For optimal resuts, add to your %s:\n"),"bspwmrc");
      printf("   bspc rule -a Xsnow state=floating border=off\n");
   }
   printf("GTK version: %s\n",ui_gtk_version());
   printf("GSL version: ");
#ifdef GSL_VERSION
   printf("%s\n",GSL_VERSION);
#else
   printf(_("unknown\n"));
#endif

   // Circumvent wayland problems:before starting gtk: make sure that the 
   // gdk-x11 backend is used.

   setenv("GDK_BACKEND","x11",1);
   if (getenv("WAYLAND_DISPLAY")&&getenv("WAYLAND_DISPLAY")[0])
   {
      printf(_("Detected Wayland desktop\n"));
      global.IsWayland = 1;
   }
   else
      global.IsWayland = 0;

   gtk_init(&argc, &argv);

   if (Flags.BelowAllForce)
      Flags.BelowAll = 0;

   if(Flags.CheckGtk && !ui_checkgtk() && !Flags.NoMenu)
   {
      printf(_("Xsnow needs gtk version >= %s, found version %s \n"),ui_gtk_required(),ui_gtk_version());

      if(!ui_run_nomenu())
      {
	 Thanks();
	 return 0;
      }
      else
      {
	 Flags.NoMenu = 1;
	 printf("%s %s",_("Continuing with flag"),"'-nomenu'\n");
      }
   }

   global.display = XOpenDisplay(Flags.DisplayName);
   global.xdo = xdo_new_with_opened_display(global.display,NULL,0);
   if(global.xdo == NULL)
   {
      I("xdo problems\n");
      exit(1);
   }
   global.xdo->debug = 0;

   XSynchronize(global.display,dosync);
   XSetErrorHandler(XsnowErrors);
   int screen = DefaultScreen(global.display);
   global.Screen = screen;
   global.Black = BlackPixel(global.display, screen);
   global.White = WhitePixel(global.display, screen);


   { // if somebody messed up colors in .xsnowrc:
      if (!ValidColor(Flags.TreeColor))
	 Flags.TreeColor = strdup(DefaultFlags.TreeColor); 
      if (!ValidColor(Flags.SnowColor))
	 Flags.SnowColor = strdup(DefaultFlags.SnowColor); 
      if (!ValidColor(Flags.SnowColor2))
	 Flags.SnowColor2 = strdup(DefaultFlags.SnowColor2); 
      if (!ValidColor(Flags.BirdsColor))
	 Flags.BirdsColor = strdup(DefaultFlags.BirdsColor); 
   }


   InitSnowOnTrees();

   if (global.display == NULL) {
      if (Flags.DisplayName == NULL) Flags.DisplayName = getenv("DISPLAY");
      fprintf(stderr, _("%s: cannot connect to X server %s\n"), argv[0],
	    Flags.DisplayName ? Flags.DisplayName : _("(default)"));
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


#define DOIT_I(x,d,v) OldFlags.x = Flags.x;
#define DOIT_L(x,d,v) DOIT_I(x,d,v);
#define DOIT_S(x,d,v) OldFlags.x = strdup(Flags.x);
   DOITALL;
#include "undefall.inc"


   OldFlags.FullScreen = !Flags.FullScreen;

   BlackPix = AllocNamedColor(BlackColor, global.Black);

   if(global.Desktop)
   {
      XSelectInput(global.display, global.Rootwindow, StructureNotifyMask| SubstructureNotifyMask);
   }
   else
      XSelectInput(global.display, global.SnowWin, StructureNotifyMask|SubstructureNotifyMask);

   ClearScreen();   // without this, no snow, scenery etc. in KDE; this seems to be a vintage comment


   if(!Flags.NoMenu && !global.XscreensaverMode)
   {
      ui();
      ui_gray_erase(global.Trans);
      ui_set_sticky(Flags.AllWorkspaces);
      add_to_mainloop(PRIORITY_DEFAULT, time_desktop_type, do_show_desktop_type);
   }
   Flags.Done = 0;
   windows_init();
   moon_init();
   aurora_init();
   Santa_init();
   birds_init();
   scenery_init();
   snow_init();
   meteor_init();
   wind_init();
   stars_init();
   blowoff_init();
   treesnow_init();
   loadmeasure_init();
   fallensnow_init();

   add_to_mainloop(PRIORITY_DEFAULT, time_displaychanged,     do_displaychanged           );
   add_to_mainloop(PRIORITY_DEFAULT, time_event,              do_event                    );
   add_to_mainloop(PRIORITY_DEFAULT, time_testing,            do_testing                  );
   add_to_mainloop(PRIORITY_DEFAULT, time_writeflags,         do_write_flags              );
   add_to_mainloop(PRIORITY_DEFAULT, time_display_dimensions, do_display_dimensions       );
   add_to_mainloop(PRIORITY_HIGH,    time_ui_check,           do_ui_check                 );
   add_to_mainloop(PRIORITY_DEFAULT, time_show_range_etc,     do_show_range_etc           );
   add_to_mainloop(PRIORITY_DEFAULT, time_check_stop,         do_check_stop               );

   if (Flags.StopAfter > 0)
      add_to_mainloop(PRIORITY_DEFAULT, Flags.StopAfter, do_stopafter);

   HandleCpuFactor();

   fflush(stdout);

   DoAllWorkspaces(); // to set global.ChosenWorkSpace

   if (!Flags.NoConfig)
      global.time_to_write_flags = TRUE;

   PrintAbsoluteCoordinates();

   P("Entering main loop\n");
   // main loop
   gtk_main();

   if (SnowWinName) free(SnowWinName);

   XClearWindow(global.display, global.SnowWin);
   XFlush(global.display);
   XCloseDisplay(global.display); //also frees the GC's, pixmaps and other resources on display

   if (DoRestart)
   {
      sleep(0);
      printf("Xsnow %s: ",_("restarting"));
      int k = 0;
      while (Argv[k])
	 printf("%s ",Argv[k++]);
      printf("\n");
      fflush(NULL);
      setenv("XSNOW_RESTART","yes",1);
      execvp(Argv[0],Argv);
   }
   else
      Thanks();
   return 0;
}		/* End of snowing */



void set_below_above()
{
   P("%d set_below_above %d\n",global.counter++,Flags.BelowAll);
   XWindowChanges changes;

   // to be sure: we do it in gtk mode and window mode
   if (Flags.BelowAll)
   {
      if(TransA)
	 setbelow(GTK_WINDOW(TransA));
      changes.stack_mode = Below;
      if(global.SnowWin)
	 XConfigureWindow(global.display,global.SnowWin,CWStackMode,&changes);
   }
   else
   {
      if(TransA)
	 setabove(GTK_WINDOW(TransA));
      changes.stack_mode = Above;
      if(global.SnowWin)
	 XConfigureWindow(global.display,global.SnowWin,CWStackMode,&changes);
   }
}

void X11WindowById(Window *xwin)
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
      printf(_("Click on a window ...\n"));
      fflush(stdout);
      int rc;
      rc = xdo_select_window_with_click(global.xdo,xwin);
      if (rc == XDO_ERROR)
      {
	 fprintf(stderr,"XWinInfo failed\n");
	 exit(1);
      }
   }
   return;
}

void GetDesktopSession()
{
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

      for (int i=0; desktops[i]; i++)
      {
	 desktopsession = getenv(desktops[i]);
	 if (desktopsession)
	    break;
      }
      if (desktopsession)
	 printf(_("Detected desktop session: %s\n"),desktopsession);
      else
      {
	 printf(_("Could not determine desktop session\n"));
	 desktopsession = (char *)"unknown_desktop_session";
      }

      global.DesktopSession = strdup(desktopsession);
   }
}

int StartWindow()
{
   P("Entering StartWindow...\n");

   global.Trans     = 0;
   global.xxposures = 0;
   global.Desktop   = 0;  // well, it seems that it always becomes 1, so canditate for deletion
   global.UseDouble = 0;
   global.IsDouble  = 0;
   global.XscreensaverMode = 0;

   global.Rootwindow = DefaultRootWindow(global.display);
   Window xwin;
   // see if user chooses window
   X11WindowById(&xwin);
   if (xwin)
   {
      P("StartWindow xwin%#lx\n",xwin);
      global.SnowWin           = xwin;
      X11cairo = 1;
   }
   else if(Flags.ForceRoot)
   {
      // user specified to run on root window
      printf(_("Trying to snow in root window\n"));
      global.SnowWin = global.Rootwindow;
      if (getenv("XSCREENSAVER_WINDOW"))
      {
	 global.XscreensaverMode  = 1;
	 global.SnowWin    = strtol(getenv("XSCREENSAVER_WINDOW"),NULL,0);
	 global.Rootwindow = global.SnowWin;
      }
      X11cairo = 1;
   }
   else
   {
      P("Flags.Screen:%d\n",Flags.Screen);
      // default behaviour
      // try to create a transparent clickthrough window
      GtkWidget *gtkwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
      gtk_widget_set_can_focus(gtkwin, TRUE);
      gtk_window_set_decorated(GTK_WINDOW(gtkwin), FALSE);
      gtk_window_set_type_hint(GTK_WINDOW(gtkwin), GDK_WINDOW_TYPE_HINT_POPUP_MENU);


      GdkWindow *gdkwin; (void)gdkwin;
      int rc = make_trans_window(global.display, gtkwin,
	    Flags.Screen,        // full screen or xinerama
	    Flags.AllWorkspaces, // sticky 
	    Flags.BelowAll,      // below
				 //1,                   // dock
	    0,                   // dock  see "IGNORE_ME" for xpenguins and others
	    NULL,                // gdk_window
	    &xwin,               // x11_window
	    &wantx,              // make_trans_window tries to place the window here,
				 //                      but, depending on window manager that does not always succeed
	    &wanty
	    );
      if (rc) // we have a transparent snow window
      {
	 global.Trans            = 1;
	 global.IsDouble         = 1;
	 global.Desktop          = 1;
	 TransA                  = gtkwin;
	 GtkWidget *drawing_area = gtk_drawing_area_new();
	 gtk_container_add(GTK_CONTAINER(TransA), drawing_area);
	 g_signal_connect(TransA, "draw", G_CALLBACK(on_draw_event), NULL);
	 P("calling set_below_above\n");
	 set_below_above();
	 global.SnowWin = xwin;
	 printf(_("Using transparent window\n"));
	 P("wantx, wanty: %d %d\n",wantx,wanty);
	 if(!strncasecmp(global.DesktopSession,"fvwm",4) ||
	       !strncasecmp(global.DesktopSession,"lxqt",4))
	 {
	    printf(_("The transparent snow-window is probably not click-through, alas..\n"));
	 }
      }
      else  // we don't have a transparent snow window
      {
	 global.Desktop  = 1;
	 X11cairo = 1;
	 // use rootwindow, pcmanfm or Desktop:
	 printf(_("Cannot create transparent window\n"));
	 if (!strcasecmp(global.DesktopSession,"enlightenment"))
	    printf(_("NOTE: xsnow will probably run, but some glitches are to be expected.\n"));
	 if(!strcasecmp(global.DesktopSession,"twm"))
	    printf("NOTE: you probably need to tweak 'Lift snow on windows' in the 'settings' panel.\n");
	 // if envvar DESKTOP_SESSION == LXDE, search for window with name pcmanfm
	 // largest_window_with_name uses name argument as regex, hence the '^' and '$'.
	 // Note that the name is still case-insensitive

	 int x,y;
	 xinerama(global.display, Flags.Screen, &x, &y, NULL, NULL);
	 P("Screen: %d x: %d y: %d\n",Flags.Screen, x, y);

	 // In LXDE, there are many windows named pcmanfm
	 if (!strncmp(global.DesktopSession,"LXDE",4) && 
	       (xwin = largest_window_with_name(global.xdo,"^pcmanfm$",x,y))
	    )
	 {
	    lxdefound = True;
	    printf(_("LXDE session found, using window 'pcmanfm'.\n"));
	    P("lxdefound: %d %#lx\n",lxdefound,*xwin);
	 }
	 else if (
	       (xwin = largest_window_with_name(global.xdo,"^Desktop$",x,y))
	       )
	 {
	    printf(_("Using window 'Desktop'.\n"));
	 }
	 else
	 {
	    printf(_("Using root window\n"));
	    xwin = global.Rootwindow;
	 }
	 global.SnowWin = xwin;
	 int winw,winh;
	 P("global.Desktop: %d\n",global.Desktop);
	 if (Flags.Screen >=0 && global.Desktop)
	 {
	    xinerama(global.display,Flags.Screen,&wantx,&wanty,&winw,&winh);
	    P("wantx: %d, wanty: %d, winw: %d, winh: %d\n",wantx, wanty, winw, winh);
	 }
      }
   }

   if(X11cairo)
   {
      P("X11cairo lxdefound: %d\n",lxdefound);
      HandleX11Cairo();
      drawit_id = add_to_mainloop1(PRIORITY_HIGH, time_draw_all, do_drawit, CairoDC);
      global.WindowOffsetX = 0;
      global.WindowOffsetY = 0;
   }
   else
   {
      // see also windows.c
      global.WindowOffsetX = wantx;
      global.WindowOffsetY = wanty;
   }

   global.IsDouble = global.Trans || global.UseDouble; // just to be sure ...

   XTextProperty x;
   if (XGetWMName(global.display,xwin,&x))
      SnowWinName = strdup((char *)x.value);
   else
      SnowWinName = strdup("no name");
   XFree(x.value);

   if(!X11cairo)
   {
      xdo_move_window(global.xdo,global.SnowWin,wantx,wanty);
      P("wantx wanty: %d %d\n",wantx,wanty);
   }

   xdo_wait_for_window_map_state(global.xdo,global.SnowWin,IsViewable);

   if (global.Trans)
      SetProperty(global.display,global.SnowWin,ignore_atom,"xsnow");

   P("SnowWinWidth: %d SnowWinHeight: %d\n", global.SnowWinWidth, global.SnowWinHeight);
   InitDisplayDimensions();

   global.SnowWinX = wantx;
   global.SnowWinY = wanty;

   PrevW = global.SnowWinWidth;
   PrevH = global.SnowWinHeight;
   P("PrevW: %d PrevH: %d\n",PrevW,PrevH);

   P("woffx: %d %d\n",global.WindowOffsetX,global.WindowOffsetY);

   fflush(stdout);

   SetWindowScale();
   if (global.XscreensaverMode && !Flags.BlackBackground)
      SetBackground();

   /*
    * to remove titlebar in mwm. Alas, this results in no visuals at all in xfce.
    XSetWindowAttributes attr;
    attr.override_redirect = True;
    XChangeWindowAttributes(global.display,global.SnowWin,CWOverrideRedirect,&attr);
    */
   return TRUE;
}

void PrintAbsoluteCoordinates()
{

   // we did our best to place the snow window, but you never know what
   // the window manager is cooking for us. So we ask politely the
   // whereabouts of the snowwindow:

   int x,y,w,h;
   GetAbsoluteCoordinates(global.display, global.SnowWin, &x, &y, &w, &h);
   printf(_("Snowing in %#lx: %s %d+%d %dx%d\n"),global.SnowWin,SnowWinName,x,y,w,h);
   P("Snowing in %#lx: %s %d+%d %dx%d\n",global.SnowWin,SnowWinName,x,y,w,h);
   fflush(stdout);
}

int HandleX11Cairo()
{
   P("Eentering HandleX11Cairo: SnowWin: 0x%lx\n",global.SnowWin);
   unsigned int w,h;
   xdo_get_window_size(global.xdo, global.SnowWin, &w, &h);
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
      printf(_("Using double buffer: %#lx %dx%d\n"),backBuf,w,h);
      rcv = TRUE;
   }
#endif
   if(!dodouble)
   {
      if (CairoSurface)
	 cairo_surface_destroy(CairoSurface);
      CairoSurface = cairo_xlib_surface_create(global.display, global.SnowWin, visual, w, h);
      printf(_("NOT using double buffering:"));
      if (Flags.UseDouble)
	 printf(_(" because double buffering is not available on this system\n"));
      else
	 printf(_(" on your request.\n"));
      printf(_("NOTE: expect some flicker.\n"));
      fflush(stdout);
      rcv = FALSE;
   }

   if (CairoDC)
      cairo_destroy(CairoDC);

   CairoDC = cairo_create(CairoSurface);
   cairo_xlib_surface_set_size(CairoSurface,w,h);
   global.SnowWinWidth  = w;
   global.SnowWinHeight = h;

   // We are not able to create a transparent, click-through window so we have to paint
   // in an existing window.
   //
   // Placement of the snow window:
   // In multi-screen environments, it depends on your desktop manager how we
   // should proceed.
   // Simple X (also fvwm ant twm): There is one big (root) window, covering all screens.
   //    In that case, we use cairo_rectangle and cairo_clip to abtain a drawing area
   //    that starts at global.SnowWinX, global.SnowWinY, 
   //       width: global.SnowWinWidth, height: global.SnowWinHeight
   //
   // Others, like LXDE, have already created windows, precisely covering
   // the screens. We can check this by searching for a window that
   // precisely covers the requested screen. The drawing coordinates of the
   // must be 0,0 in that case.
   //
   // So, we find, using xinerama(), the coordinates and dimensions of the snow window.
   //
   // If there is a window with the same coordinates and sizes, we use that to draw in,
   // and set the drawing coordinates to 0,0.
   //
   // Otherwize, we use cairo_rectangle and cairo_clip to define our drawing area.
   //

   int x1,y1,w1,h1;
   xinerama(global.display, Flags.Screen, &x1, &y1, &w1, &h1);

   WinInfo *windows;
   int      nwin;

   GetWindows(&windows,&nwin);
   int ownwindow = False;
   for (int i=0; i<nwin; i++)
   {
      WinInfo *w = &windows[i];
      if (w->xa == x1 && w->ya == y1 && (int)w->w == w1 && (int)w->h == h1)
      {
	 P("match found: x1: %d y1: %d w1: %d h1: %d screen: %d id: %p\n",x1,y1,w1,h1,Flags.Screen,(void*)w->id);
	 ownwindow = True;
	 break;
      }
   }
   if(windows)
      free(windows);


   if (Flags.Screen >= 0 && global.Desktop)
   {
      int winx=0, winy=0, winw, winh;
      //int rc = xinerama(global.display,Flags.Screen,NULL, NULL, &winw, &winh);
      int rc = xinerama(global.display, Flags.Screen, &winx, &winy, &winw, &winh);
      P("rc: %d winx: %d winy: %d winw: %d winh: %d\n",rc,winx,winy,winw,winh);
      //if (rc && winx==0 && winy==0)
      if(rc)
      {
	 if(ownwindow)
	 {
	    global.SnowWinX = 0;
	    global.SnowWinY = 0;
	 }
	 else
	 {
	    global.SnowWinX = winx;
	    global.SnowWinY = winy;
	 }
	 global.SnowWinWidth  = winw;
	 global.SnowWinHeight = winh;
	 PrevW                = winw;
	 PrevH                = winh;
      }
      //cairo_rectangle(CairoDC,global.SnowWinX,global.SnowWinY,global.SnowWinWidth,global.SnowWinHeight);
      if(!ownwindow)
      {
	 cairo_rectangle(CairoDC,winx,winy,global.SnowWinWidth,global.SnowWinHeight);
	 P("clipsnow winx: %d winy: %d snowwinx: %d snowwiny:%d snowwinwidth: %d snowwimheight: %d\n",winx, winy, global.SnowWinX,global.SnowWinY,global.SnowWinWidth,global.SnowWinHeight);
	 cairo_clip(CairoDC);
      }
   }
   return rcv;
}

void DoAllWorkspaces()
{
   if(Flags.AllWorkspaces)
   {
      P("stick\n");
      set_sticky(1);
   }
   else
   {
      P("unstick\n");
      set_sticky(0);
      if (Flags.Screen >=0)
	 global.ChosenWorkSpace = global.VisWorkSpaces[Flags.Screen];
      else
	 global.ChosenWorkSpace = global.VisWorkSpaces[0];
   }
   ui_set_sticky(Flags.AllWorkspaces);
}

int set_sticky(int s)
{
   int r = IsSticky;
   if (global.Trans)
   {
      if(s)
      {
	 IsSticky = 1;
	 gtk_window_stick(GTK_WINDOW(TransA));
      }
      else
      {
	 IsSticky = 0;
	 gtk_window_unstick(GTK_WINDOW(TransA));
      }
   }
   return r;
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
   meteor_ui();
   wind_ui();
   stars_ui();
   fallensnow_ui();
   blowoff_ui();
   treesnow_ui();
   moon_ui();
   aurora_ui();
   ui_ui();

   UIDO (CpuLoad             , HandleCpuFactor();                 );
   UIDO (Transparency        ,                                    );
   UIDO (Scale               ,                                    );
   UIDO (OffsetS             , DisplayDimensions();               );
   UIDO (OffsetY             , 
	 int x = global.DoCapella;
	 global.DoCapella=0; 
	 UpdateFallenSnowRegionsWithLock(); 
	 global.DoCapella = x;                                    );
   UIDO (NoFluffy            , ClearScreen();                     );
   UIDO (AllWorkspaces       , DoAllWorkspaces();                 );
   UIDO (BelowAll            , set_below_above();                 );
   UIDOS(BackgroundFile      ,                                    );
   UIDO (BlackBackground     ,                                    );

   if (Flags.Changes > 0)
   {
      //WriteFlags(1);
      global.time_to_write_flags = TRUE;
      set_buttons();
      P("-----------Changes: %d\n",Flags.Changes);
   }
   Flags.Changes = 0;
   return TRUE;
   (void)d;
}

int do_write_flags(void *d)
{
   if (signal_caught)
   {
      printf("xsnow: caught signal %d\n",signal_caught);
      fflush(stdout);
      signal_caught = 0;
   }
   const int mc = 0;
   // if one should wait a little longer before calling WriteFlags()
   // one could set mc to 1, 2, ...
   static int c = mc;
   if (global.time_to_write_flags)
   {
      P("c: %d\n",c);
      if (c > 0)
      {
	 c--;
	 return TRUE;
      }
      WriteFlags(1);
      global.time_to_write_flags = FALSE;
      c = mc;
   }
   return TRUE;
   (void)d;
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

   if (global.ForceRestart)
   {
      DoRestart = 1;
      Flags.Done = 1;
      I(_("Restart due to change of screen or language settings ...\n"));
   }
   else
   {
      unsigned int w,h;
      Display* display = XOpenDisplay(Flags.DisplayName);
      Screen* screen   = DefaultScreenOfDisplay(display);
      w = WidthOfScreen(screen);
      h = HeightOfScreen(screen);
      P("width height: %d %d %d %d\n",w,h,global.Wroot,global.Hroot);
      if(global.Wroot != w || global.Hroot != h)
      {
	 DoRestart = 1;
	 Flags.Done = 1;
	 I(_("Restart due to change of display settings...\n"));
      }
      XCloseDisplay(display);
   }
   return TRUE;
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

// force stop if writeable file StopFile ($Home/.xsnowstop) exists
int do_check_stop(void*dummy)
{
   (void)dummy;
   if (access(StopFile,F_OK)) // 0: access, else no access
      return TRUE;
   if (access(StopFile,W_OK)) // 0: access, else no access
      return TRUE;
   if (unlink(StopFile) != 0)
      return TRUE;
   printf("Xsnow: halting because of existence of '%s'\n",StopFile);
   Flags.Done = 1;
   return FALSE;
}

void RestartDisplay()
{
   P("Restartdisplay: %d W: %d H: %d\n",global.counter++,global.SnowWinWidth,global.SnowWinHeight);
   fflush(stdout);
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
      s = (char *)_("Wayland (Expect some slugginess)");
   else if (global.IsCompiz)
      s = (char *)"Compiz";
   else
      s = (char *)_("Probably X11");
   char t[128];
   snprintf(t,64,_("%s. Snow window: %#lx"),s,global.SnowWin);
   ui_show_desktop_type(t);
   return TRUE;
   (void)d;
}


int do_testing(void *d)
{
   (void)d;
   if (Flags.Done)
      return FALSE;
   P("SnowWinX: %d SnowWinY: %d SnowWinWidth: %d SnowWinHeight: %d\n",global.SnowWinX,global.SnowWinY,global.SnowWinWidth, global.SnowWinHeight);

   //PrintAbsoluteCoordinates();
   return TRUE;
}

void SigHandler(int signum)
{
   signal_caught = signum;
   if (signum == SIGUSR1)
   {
      // Forbidden: printf in sighandler
      //I("Caught SIGUSR1\n");
      global.time_to_write_flags = TRUE;
      return;
   }
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
      snprintf(global.Message,sizeof(global.Message),_("More than %d errors, I quit!"),countmax);
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
   P("do_drawit %d %p\n",global.counter++,cr);
   drawit((cairo_t*)cr);
   return TRUE;
}


void drawit(cairo_t *cr)
{
   static int counter = 0;
   P("drawit %d %p\n",global.counter++,(void *)cr);
   // Due to instabilities at the start of xsnow: spurious detection of
   //   user intervention; resizing of screen; etc.
   //   placement of scenery, stars etc is repeated a few times at the
   //   start of xsnow.
   // This is not harmful, but a bit annoying, so we do not draw anything 
   //   the first few times this function is called.
   if (counter*time_draw_all < 1.5)
   {
      counter++;
      return;
   }

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
      aurora_erase();
      XFlush(global.display);
   }


   /* clear snowwindow test */
   /*
      cairo_set_source_rgba(cr, 0, 0, 0, 1);
      cairo_set_line_width(cr, 1);

      cairo_rectangle(cr, 0, 0, global.SnowWinWidth, global.SnowWinHeight);
      cairo_fill(cr);
      */

   cairo_save(cr);
   int tx = 0;
   int ty = 0;
   if (X11cairo)
   {
      tx = global.SnowWinX;
      ty = global.SnowWinY;
   }
   P("tx ty: %d %d\n",tx,ty);
   cairo_translate(cr,tx,ty);

   int skipit = Flags.BirdsOnly || !WorkspaceActive();

   if (!skipit)
   {
      stars_draw(cr);

      P("moon\n");
      moon_draw(cr);
      aurora_draw(cr);
      meteor_draw(cr);
      scenery_draw(cr);
   }

   P("birds %d\n",counter++);
   birds_draw(cr);

   if (skipit)
      goto end;;

   fallensnow_draw(cr);

   if(!Flags.FollowSanta || !Flags.ShowBirds) // if Flags.FollowSanta: drawing of Santa is done in birds_draw()
      Santa_draw(cr);

   treesnow_draw(cr);

   snow_draw(cr);

end:
   if (Flags.Outline)
      rectangle_draw(cr);

   cairo_restore(cr);

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

   DisplayDimensions();
   if (!global.Trans)
   {
      HandleX11Cairo();
   }
   P("%d do_display_dimensions %dx%d %dx%d %d\n",global.counter++,global.SnowWinWidth,global.SnowWinHeight,PrevW,PrevH,global.Trans);
   if (PrevW != global.SnowWinWidth || PrevH != global.SnowWinHeight)
   {
      {
	 // we did our best to place the snow window, but you never know what
	 // the window manager is cooking for us. So we ask politely the
	 // whereabouts of the snowwindow:

	 int x,y,w,h;
	 GetAbsoluteCoordinates(global.display, global.SnowWin, &x, &y, &w, &h);
	 printf(_("Window size changed, now snowing in in %#lx: %s %d+%d %dx%d\n"),global.SnowWin,SnowWinName,x,y,w,h);
      }
      P("Calling RestartDisplay\n");
      RestartDisplay();
      PrevW = global.SnowWinWidth;
      PrevH = global.SnowWinHeight;
      SetWindowScale();
   }
   fflush(stdout);
   return TRUE;
}

int do_draw_all(gpointer widget)
{
   if (Flags.Done)
      return FALSE;
   P("do_draw_all %d %p\n",global.counter++,(void *)widget);

   // this will result in a call off on_draw_event():
   gtk_widget_queue_draw(GTK_WIDGET(widget));
   return TRUE;
}


// handle callbacks for things whose timings depend on cpufactor
void HandleCpuFactor()
{
   // re-add things whose timing is dependent on cpufactor
   if (Flags.CpuLoad <= 0)
      global.cpufactor = 1;
   else
      global.cpufactor = 100.0/Flags.CpuLoad;

   P("handlecpufactor %f %f %d\n",oldcpufactor,cpufactor,counter++);
   add_to_mainloop(PRIORITY_HIGH, time_init_snow , do_initsnow);  // remove flakes

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
void rectangle_draw(cairo_t *cr)
{
   const int lw = 8;
   cairo_save(cr);
   cairo_set_source_rgba(cr,1,1,0,0.5);
   cairo_rectangle(cr,lw/2,lw/2,global.SnowWinWidth-lw,global.SnowWinHeight-lw); 
   cairo_set_line_width(cr,lw);
   cairo_stroke(cr);
   cairo_restore(cr);
}

int do_stopafter(void *d)
{
   Flags.Done = 1;
   printf(_("Halting because of flag %s\n"),"-stopafter");
   return FALSE;
   (void)d;
}

void mybindtestdomain()
{
   global.Language = guess_language();
#ifdef HAVE_GETTEXT
   // One would assume that gettext() uses the environment variable LOCPATH
   // to find locales, but no, it does not.
   // So, we scan LOCPATH for paths, use them and see if gettext gets a translation
   // for a text that is known to exist in the ui and should be translated to
   // something different.

   char *startlocale;
   (void) startlocale;
   char *r = setlocale(LC_ALL,"");
   if (r)
      startlocale = strdup(r); 
   else
      startlocale = NULL;
   P("startlc: %s\n",startlocale);
   //if (startlocale == NULL)
   //  startlocale = strdup("en_US.UTF-8");
   P("startlc: %s\n",startlocale);
   P("LC_ALL: %s\n",getenv("LC_ALL"));
   P("LANG: %s\n",getenv("LANG"));
   P("LANGUAGE: %s\n",getenv("LANGUAGE"));
   textdomain (TEXTDOMAIN);
   char *textd = bindtextdomain(TEXTDOMAIN,LOCALEDIR);
   (void) textd;
   P("textdomain:%s\n",textd);

   char *locpath = getenv("LOCPATH");
   P("LOCPATH: %s\n",locpath);
   P("LOCALEDIR: %s\n",LOCALEDIR);
   if (locpath)
   {
      char *initial_textdomain = strdup(bindtextdomain(TEXTDOMAIN,NULL));
      char *mylocpath          = strdup(locpath);
      int  translation_found   = False;
      P("Initial textdomain: %s\n",initial_textdomain);

      char *q = mylocpath;
      while(1)
      {
	 char *p = strsep(&q,":");
	 if (!p)
	    break;
	 bindtextdomain(TEXTDOMAIN, p);
	 //char *trans = _(TESTSTRING);
	 P("trying '%s' '%s' '%s'\n",p,TESTSTRING,trans);
	 if(strcmp(_(TESTSTRING),TESTSTRING))
	 {
	    // translation found, we are happy
	    P("Translation: %s\n",trans);
	    translation_found = True;
	    break;
	 }
      }
      if (!translation_found)
      {
	 P("No translation found in %s\n",locpath);
	 bindtextdomain(TEXTDOMAIN,initial_textdomain);
      }
      free(mylocpath);
      free(initial_textdomain);
   }


   // todo: naar restart of ui?


   P("textdomain: %s\n",textdomain(NULL));
   P("bindtextdomain: %s\n",bindtextdomain(TEXTDOMAIN,NULL));
   //bind_textdomain_codeset(TEXTDOMAIN,"UTF-8");

#endif
}
