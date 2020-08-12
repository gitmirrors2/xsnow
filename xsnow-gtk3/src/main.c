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
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>
#include <X11/xpm.h>
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
#include <assert.h>
#include <ctype.h>
#include <gtk/gtk.h>
#include <math.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Santa.h"
#include "birds.h"
#include "clocks.h"
#include "csvpos.h"
#include "docs.h"
#include "doit.h"
#include "dsimple.h"
#include "fallensnow.h"
#include "flags.h"
#include "gaussian.h"
#include "ixpm.h"
#include "kdesetbg.h"
#include "mainstub.h"
#include "meteo.h"
#include "pixmaps.h"
#include "scenery.h"
#include "snow.h"
#include "transparent.h"
#include "ui.h"
#include "utils.h"
#include "version.h"
#include "windows.h"
#include "wmctrl.h"
#include "xsnow.h"
#include "wind.h"

#include "hashtable.h"

#ifdef DEBUG
#undef DEBUG
#endif
//#define DEBUG
#include "debug.h"

// from flags.h
FLAGS Flags;
FLAGS OldFlags;

// from windows.h
Display *display;
int     screen;
Window  SnowWin;
Window  BirdsWin;
int     SnowWinBorderWidth;
int     SnowWinDepth;
int     SnowWinHeight;
int     SnowWinWidth; 
int     SnowWinX; 
int     SnowWinY; 
char   *DesktopSession = 0;
int     IsCompiz;
int     IsWayland;
int     UseAlpha;
Pixel   ErasePixel;
int     Exposures;
Pixel   BlackPix;
int        OnTrees = 0;

double  factor = 1.0;
float   NewWind = 0;

GtkWidget       *drawing_area = 0;
GdkWindow       *gdkwindow = 0;

// locals
static int counter = 0;
// snow flakes stuff
float BlowOffFactor;
static int   MaxSnowFlakeHeight = 0;  /* Biggest flake */
static int   MaxSnowFlakeWidth = 0;   /* Biggest flake */

// fallen snow stuff
FallenSnow *FsnowFirst = 0;

// miscellaneous
char       Copyright[] = "\nXsnow\nCopyright 1984,1988,1990,1993-1995,2000-2001 by Rick Jansen, all rights reserved, 2019,2020 also by Willem Vermin\n";
static int      ActivateClean = 0;  // trigger for do_clean
static int      Argc;
static char     **Argv;

// tree stuff
static int      KillStars  = 0;  // 1: signal to trees to kill themselves



// star stuff
static int NStars;


// timing stuff
//static double       TotSleepTime = 0;
static double       TStart;

// windows stuff
static int          NWindows;
long         CWorkSpace = 0;
static Window       RootWindow;
static char         *SnowWinName = 0;
static WinInfo      *Windows = 0;
static long         TransWorkSpace = -1;  // workspace on which transparent window is placed
static int          UsingTrans     = 0;   // using transparent window or not
static int          Xroot;
static int          Yroot;
static unsigned int Wroot;
static unsigned int Hroot;
static int          DoRestart = 0;
static cairo_region_t  *cairoRegion = 0;

/* Wind stuff */
int Wind = 0;
// Wind = 0: no wind
// Wind = 1: wind only affecting snow
// Wind = 2: wind affecting snow and santa
// Direction =  0: no wind direction I guess
// Direction =  1: wind from left to right
// Direction = -1: wind from right to left
static int    Direction = 0;
static float  Whirl;
static double WindTimer;
static double WindTimerStart;

// desktop stuff
static int       Isdesktop;
static GtkWidget *GtkWin  = NULL;  // for snow etc
GtkWidget *GtkWinb = NULL;  // for birds

/* Colo(u)rs */
static const char *BlackColor  = "black";
static const char *StarColor[] = { "gold", "gold1", "gold4", "orange" };

static Pixel StarcPix[STARANIMATIONS];

/* GC's */
static GC CleanGC;
static GC EFallenGC;
static GC FallenGC;
static GC SnowOnTreesGC;
static GC StarGC[STARANIMATIONS];
static GC TestingGC;
//static GC TreesGC[2];

// region stuff
//static Region NoSnowArea_static;

/* Forward decls */
static int    BlowOff(void);
static void   CleanFallenArea(FallenSnow *fsnow, int x, int w);
static void   CleanFallen(Window id);
static void   ConvertOnTreeToFlakes(void);
static void   HandleFactor(void);
static Pixmap CreatePixmapFromFallen(struct FallenSnow *f);
static int    DetermineWindow(void);
static void   DrawFallen(FallenSnow *fsnow);
static void   EraseFallenPixel(FallenSnow *fsnow,int x);
static void   EraseStars(void);
static void   GenerateFlakesFromFallen(FallenSnow *fsnow, int x, int w, float vy);
static void   HandleExposures(void);
static void   InitDisplayDimensions(void);
static void   InitFallenSnow(void);
static void   RestartDisplay(void);
static void   InitSnowOnTrees(void);
static void   KDESetBG1(const char *color);
static int    RandInt(int maxVal);
static void   SetGCFunctions(void);
static void   SetMaxScreenSnowDepth(void);
static void   SetWhirl(void);
static void   SetWindTimer(void);
static void   SigHandler(int signum);
static void   UpdateFallenSnowWithWind(FallenSnow *fsnow,int w, int h);
static void   UpdateWindows(void);
static int    XsnowErrors(Display *dpy, XErrorEvent *err);
static Window XWinInfo(char **name);
static gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static void drawit(cairo_t *cr);


static void Thanks(void)
{
   printf("\nThank you for using xsnow\n");
}

// callbacks
static int do_blowoff(void);
static int do_clean(void);
static int do_displaychanged(void);
static int do_draw_all(gpointer widget);
static int do_event(void);
static int do_fallen(void);
static int do_initstars(void);
static int do_newwind(void);
static int do_show_desktop_type(void);
static int do_show_range_etc(void);
static int do_snow_on_trees(void);
static int do_star(Skoordinaten *star);
static int do_testing(void);
static int do_ui_check(void);
static int do_ustar(Skoordinaten *star);
static int do_wind(void);
static int do_wupdate(void);




#if 0
static void testje()
{
   const int n=10;
   int a[n];
   int i;
   for (i=0; i<n; i++)
      a[i]=i*i;

   for (i=0; i<n; i++)
   {
      printf("%d %p %d\n",i,(void *)&a[i],a[i]);
      set_insert(&a[i]);
   }
   printf("---\n");

   for (i=0; i<n; i++)
   {
      printf("%d %d %d\n",i,set_count(&a[i]),set_count(&a[i]+4));
   }
   printf("--- erase test\n");

   set_erase(&a[1]);
   for (i=0; i<n; i++)
   {
      printf("%d %d\n",i,set_count(&a[i]));
   }
   printf("--- begin, next\n");
   set_begin();
   int *p;
   while ( (p = set_next()) )
   {
      printf("%p %i\n",(void *)p,*p);
   }

   printf("--- clear\n");

   set_clear();
   for (i=0; i<n; i++)
   {
      printf("%d %d %d\n",i,set_count(&a[i]),set_count(&a[i]+1));
   }

   exit(0);
}
#endif

int main_c(int argc, char *argv[])
{
   //testje();
   Argc = argc;
   Argv = argv;
   int i;
   for (i=0; i<Argc; i++)
   {
      P("flag%d: %s\n",i,Argv[i]);
   }
   InitFlags();
   int rc = HandleFlags(argc, argv);
   switch(rc)
   {
      case -1:   // wrong flag
	 PrintVersion();
	 Thanks();
	 return 1;
	 break;
      case 1:    // manpage or help
	 return 0;
	 break;
      default:
	 PrintVersion();
	 break;
   }
   // Circumvent wayland problems:before starting gtk: make sure that the 
   // gdk-x11 backend is used.
   // I would prefer if this could be arranged in argc-argv, but 
   // it seems that it cannot be done there.

   if (getenv("WAYLAND_DISPLAY")&&getenv("WAYLAND_DISPLAY")[0])
   {
      printf("Detected Wayland desktop\n");
      setenv("GDK_BACKEND","x11",1);
      IsWayland = 1;
   }
   else
      IsWayland = 0;
   gtk_init(&argc, &argv);
   if (!Flags.NoConfig)
      WriteFlags();

   display = XOpenDisplay(Flags.DisplayName);
   XSynchronize(display,dosync);
   XSetErrorHandler(XsnowErrors);
   screen = DefaultScreen(display);
   Black = BlackPixel(display, screen);
   White = WhitePixel(display, screen);

   HandleExposures();

   //InitSnowSpeedFactor();
   SetWhirl();
   SetWindTimer();

   SnowOnTrees = (XPoint *)malloc(sizeof(*SnowOnTrees));  // will be remallloced in InitSnowOnTrees
   InitSnowOnTrees();


   SnowOnTrees = (XPoint *)malloc(sizeof(*SnowOnTrees)*Flags.MaxOnTrees);

   srand48((long int)(wallcl()*1.0e6));
   SnowMap *rp;
   signal(SIGINT, SigHandler);
   signal(SIGTERM, SigHandler);
   signal(SIGHUP, SigHandler);
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
   // if(Isdesktop): we have a desktop and we will snow on the windows in it
   //   else we have a normal window and will not snow on other windows
   // if(UseAlpha): drawing to the desktop is as follows:
   //   - all colours are made opaque by or-ing the colors with 0xff000000
   //   - clearing is done writing the same image, but with color black (0x00000000) 
   //   else
   //   - we will use XClearArea to erase flakes and the like. This works well
   //     on fvwm-like desktops (desktop == RootWindow) with exposures set to 0
   //     It works more or less in for example KDE, but exposures must be set to 1
   //     which severely stresses plasma shell (or nautilus-desktop in Gnome, 
   //     but we do not use XClearArea in Gnome).
   //


   RootWindow = DefaultRootWindow(display);

   if (!DetermineWindow())
   {
      printf("xsnow: cannot determine window, exiting...\n");
      return 1;
   }
   printf("Snowing in window: %#lx - \"%s\" - depth: %d - geom: %d %d %dx%d - alpha: %d - exposures: %d\n",
	 SnowWin,SnowWinName,SnowWinDepth,
	 SnowWinX,SnowWinY,SnowWinWidth,SnowWinHeight, UseAlpha,Exposures);

   Santa_init();

   NoSnowArea_dynamic   = XCreateRegion();  // needed when drawing on background with xor.
   //                                          unpleasant things happen when a snowflake
   //                                          is in the trajectory of a meteorite
   TreeRegion           = XCreateRegion();
   SnowOnTreesRegion    = XCreateRegion();
   int flake;
   for (flake=0; flake<=SNOWFLAKEMAXTYPE; flake++) 
   {
      rp = &snowPix[flake];
      rp->pixmap = XCreateBitmapFromData(display, SnowWin,
	    rp->snowBits, rp->width, rp->height);
      if (rp->height > MaxSnowFlakeHeight) MaxSnowFlakeHeight = rp->height;
      if (rp->width  > MaxSnowFlakeWidth ) MaxSnowFlakeWidth  = rp->width;
   }
   starPix.pixmap = XCreateBitmapFromData(display, SnowWin,
	 (char *)starPix.starBits, starPix.width, starPix.height);
   InitFallenSnow();
   scenery_init();
   snow_init();
   meteo_init();

#define DOIT_I(x) OldFlags.x = Flags.x;
#define DOIT_L(x) DOIT_I(x);
#define DOIT_S(x) OldFlags.x = strdup(Flags.x);
   DOITALL;
#undef DOIT_I
#undef DOIT_L
#undef DOIT_S


   BlackPix = AllocNamedColor(BlackColor, Black);
   for(i=0; i<STARANIMATIONS; i++)
      StarcPix[i] = IAllocNamedColor(StarColor[i], Black);

   TestingGC     = XCreateGC(display, RootWindow, 0,0);
   SnowOnTreesGC = XCreateGC(display, SnowWin, 0, 0);
   CleanGC       = XCreateGC(display, SnowWin,0,0);
   FallenGC      = XCreateGC(display, SnowWin, 0, 0);
   EFallenGC     = XCreateGC(display, SnowWin, 0, 0);  // used to erase fallen snow
   for (i=0; i<STARANIMATIONS; i++)
      StarGC[i]  = XCreateGC(display,SnowWin,0,0);

   SetGCFunctions();

   //InitSnowColor();

   // events
   if(Isdesktop)
      XSelectInput(display, SnowWin, 0);
   else
      XSelectInput(display, SnowWin, 
	    StructureNotifyMask);

   TStart = wallclock();
   Flags.Done = 0;
   ClearScreen();   // without this, no snow, scenery etc. in KDE

   add_to_mainloop(PRIORITY_DEFAULT, time_blowoff,        do_blowoff            ,0);
   add_to_mainloop(PRIORITY_DEFAULT, time_clean,          do_clean              ,0);
   add_to_mainloop(PRIORITY_DEFAULT, time_displaychanged, do_displaychanged     ,0);
   //add_to_mainloop(PRIORITY_DEFAULT, time_emeteorite,     do_emeteorite         ,0);
   add_to_mainloop(PRIORITY_DEFAULT, time_event,          do_event              ,0);
   //add_to_mainloop(PRIORITY_DEFAULT, time_flakecount,     do_show_flakecount    ,0);
   //add_to_mainloop(PRIORITY_DEFAULT, time_genflakes,      do_genflakes          ,0);
   add_to_mainloop(PRIORITY_DEFAULT, time_initstars,      do_initstars          ,0);
   //add_to_mainloop(PRIORITY_DEFAULT, time_meteorite,      do_meteorite          ,0);
   add_to_mainloop(PRIORITY_DEFAULT, time_newwind,        do_newwind            ,0);
   add_to_mainloop(PRIORITY_DEFAULT, time_snow_on_trees,  do_snow_on_trees      ,0);
   add_to_mainloop(PRIORITY_DEFAULT, time_testing,        do_testing            ,0);
   add_to_mainloop(PRIORITY_DEFAULT, time_ui_check,       do_ui_check           ,0);
   add_to_mainloop(PRIORITY_DEFAULT, time_wind,           do_wind               ,0);
   add_to_mainloop(PRIORITY_DEFAULT, time_wupdate,        do_wupdate            ,0);
   add_to_mainloop(PRIORITY_DEFAULT, time_show_range_etc, do_show_range_etc     ,0);
   add_to_mainloop(PRIORITY_DEFAULT, 1.0,                 do_show_desktop_type  ,0);
   if (GtkWinb)
      add_to_mainloop(PRIORITY_HIGH, time_draw_all,       do_draw_all           ,GtkWinb);

   HandleFactor();

   if(!Flags.NoMenu)
   {
      ui(&argc, &argv);
      if (GtkWinb)
      {
	 gtk_widget_show_all(GtkWinb);
      }
      else
      {
	 ui_set_birds_header("Your screen does not support alpha channel, no birds will fly.");
      }
      ui_set_sticky(Flags.AllWorkspaces);
   }

   // main loop
   gtk_main();


   if (SnowWinName) free(SnowWinName);

   XClearArea(display, SnowWin, 0,0,0,0,True);
   XFlush(display);
   XCloseDisplay(display); //also frees the GC's, pixmaps and other resources on display
   while(FsnowFirst)
      PopFallenSnow(&FsnowFirst);

   if (DoRestart)
   {
      sleep(2);
      extern char **environ;
      execve(Argv[0],Argv,environ);
   }
   Thanks();
   return 0;
}		/* End of snowing */

int WorkspaceActive()
{
   P("UsingTrans etc %d %d %d %d\n",Flags.AllWorkspaces,UsingTrans,CWorkSpace == TransWorkSpace,
	 Flags.AllWorkspaces || !UsingTrans || CWorkSpace == TransWorkSpace);
   // ah, so difficult ...
   return Flags.AllWorkspaces || !UsingTrans || CWorkSpace == TransWorkSpace;
}

/* ------------------------------------------------------------------ */ 
/*
 * do nothing if current workspace is not to be updated
 */
/*
#define NOTACTIVE \
(Flags.BirdsOnly || (!Flags.AllWorkspaces && (UsingTrans && CWorkSpace != TransWorkSpace)))
*/

#define NOTACTIVE \
   (Flags.BirdsOnly || !WorkspaceActive())


// here we are handling the buttons in ui
// Ok, this is a long list, and could be implemented more efficient.
// But, do_ui_check is called not too frequently, so....
// Note: if changes != 0, the settings will be written to .xsnowrc
//
int do_ui_check()
{
   if (Flags.Done)
      gtk_main_quit();

   if (Flags.NoMenu)
      return TRUE;

   int changes = 0;
   changes += Santa_ui();
   changes += scenery_ui();
   changes += birds_ui();
   changes += snow_ui();
   changes += meteo_ui();

   if(Flags.NStars != OldFlags.NStars)
   {
      EraseStars();
      OldFlags.NStars = Flags.NStars;
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.CpuLoad != OldFlags.CpuLoad)
   {
      OldFlags.CpuLoad = Flags.CpuLoad;
      P("cpuload: %d %d\n",OldFlags.CpuLoad,Flags.CpuLoad);
      HandleFactor();
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.UseBG != OldFlags.UseBG)
   {
      OldFlags.UseBG = Flags.UseBG;
      SetGCFunctions();
      ClearScreen();
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.KDEbg != OldFlags.KDEbg)
   {
      OldFlags.KDEbg = Flags.KDEbg;
      if (Flags.KDEbg)
	 KDESetBG1(Flags.BGColor);
      else
	 KDESetBG1(0);
      ClearScreen();
      P("changes: %d\n",changes);
   }
   if(strcmp(Flags.BGColor,OldFlags.BGColor))
   {
      free(OldFlags.BGColor);
      OldFlags.BGColor = strdup(Flags.BGColor);
      SetGCFunctions();
      if(Flags.KDEbg)
	 KDESetBG1(Flags.BGColor);
      ClearScreen();
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.UseAlpha != OldFlags.UseAlpha)
   {
      P("changes: %d %d %d\n",changes,OldFlags.UseAlpha,Flags.UseAlpha);
      OldFlags.UseAlpha = Flags.UseAlpha;
      UseAlpha          = Flags.UseAlpha;
      SetGCFunctions();
      ClearScreen();
      changes++;
      P("changes: %d %d %d\n",changes,OldFlags.UseAlpha,Flags.UseAlpha);
   }
   if(Flags.Exposures != OldFlags.Exposures)
   {
      P("changes: %d %d %d\n",changes,OldFlags.Exposures,Flags.Exposures);
      OldFlags.Exposures = Flags.Exposures;
      HandleExposures();
      HandleFactor();
      ClearScreen();
      changes++;
      P("changes: %d %d %d\n",changes,OldFlags.Exposures,Flags.Exposures);
      P("changes: %d\n",changes);
   }
   if(Flags.OffsetS != OldFlags.OffsetS)
   {
      OldFlags.OffsetS = Flags.OffsetS;
      InitDisplayDimensions();
      InitFallenSnow();
      EraseStars();
      EraseTrees();
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.MaxWinSnowDepth != OldFlags.MaxWinSnowDepth)
   {
      OldFlags.MaxWinSnowDepth = Flags.MaxWinSnowDepth;
      InitFallenSnow();
      ClearScreen();
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.MaxScrSnowDepth != OldFlags.MaxScrSnowDepth)
   {
      OldFlags.MaxScrSnowDepth = Flags.MaxScrSnowDepth;
      SetMaxScreenSnowDepth();
      InitFallenSnow();
      ClearScreen();
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.MaxOnTrees != OldFlags.MaxOnTrees)
   {
      OldFlags.MaxOnTrees = Flags.MaxOnTrees;
      ClearScreen();
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.NoFluffy != OldFlags.NoFluffy)
   {
      OldFlags.NoFluffy = Flags.NoFluffy;
      ClearScreen();
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.NoKeepSnowOnTrees != OldFlags.NoKeepSnowOnTrees)
   {
      OldFlags.NoKeepSnowOnTrees = Flags.NoKeepSnowOnTrees;
      ClearScreen();
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.NoKeepSBot != OldFlags.NoKeepSBot)
   {
      OldFlags.NoKeepSBot = Flags.NoKeepSBot;
      InitFallenSnow();
      ClearScreen();
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.NoKeepSWin != OldFlags.NoKeepSWin)
   {
      OldFlags.NoKeepSWin = Flags.NoKeepSWin;
      InitFallenSnow();
      ClearScreen();
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.NoWind != OldFlags.NoWind)
   {
      OldFlags.NoWind = Flags.NoWind;
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.WhirlFactor != OldFlags.WhirlFactor)
   {
      OldFlags.WhirlFactor = Flags.WhirlFactor;
      SetWhirl();
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.WindTimer != OldFlags.WindTimer)
   {
      OldFlags.WindTimer = Flags.WindTimer;
      SetWindTimer();
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.FullScreen != OldFlags.FullScreen)
   {
      OldFlags.FullScreen = Flags.FullScreen;
      DetermineWindow();
      InitFallenSnow();
      EraseStars();
      EraseTrees();
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.AllWorkspaces != OldFlags.AllWorkspaces)
   {
      OldFlags.AllWorkspaces = Flags.AllWorkspaces;
      DetermineWindow();
      ui_set_sticky(Flags.AllWorkspaces);
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.BelowAll != OldFlags.BelowAll)
   {
      OldFlags.BelowAll = Flags.BelowAll;
      DetermineWindow();
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.WindNow)
   {
      Flags.WindNow = 0;
      Wind = 2;
      P("changes: %d\n",changes);
   }

   if (changes > 0)
   {
      P("WriteFlags\n");
      WriteFlags();
      P("-----------changes: %d\n",changes);
   }
   return TRUE;
}


int do_snow_on_trees()
{
   if (Flags.Done)
      return FALSE;
   if (NOTACTIVE || KillTrees)
      return TRUE;
   if(Flags.NoKeepSnowOnTrees || Flags.NoTrees)
      return TRUE;
   if (Wind == 2)
      ConvertOnTreeToFlakes();
   static int second = 0;

   if (second)
   {
      second = 1;
      XSetForeground(display, SnowOnTreesGC, ~BlackPix); 
      XFillRectangle(display, SnowWin, SnowOnTreesGC, 0,0,SnowWinWidth,SnowWinHeight);
   }
   XSetRegion(display, SnowOnTreesGC, SnowOnTreesRegion);
   XSetForeground(display, SnowOnTreesGC, SnowcPix); 
   XFillRectangle(display, SnowWin, SnowOnTreesGC, 0,0,SnowWinWidth,SnowWinHeight);
   return TRUE;
}


int HandleFallenSnow(FallenSnow *fsnow)
{
   // soo complicated to determine if a fallensnow should be handled, therefore
   // we isolate this question in this function for further use
   return !((fsnow->id == 0 && Flags.NoKeepSBot)||(fsnow->id != 0 && Flags.NoKeepSWin)); 
}

int do_fallen()
{

   if (Flags.Done)
      return FALSE;
   if (NOTACTIVE)
      return TRUE;

   FallenSnow *fsnow = FsnowFirst;
   while(fsnow)
   {
      if (HandleFallenSnow(fsnow)) 
	 DrawFallen(fsnow);
      fsnow = fsnow->next;
   }
   XFlush(display);
   return TRUE;
}

int do_blowoff()
{
   if (Flags.Done)
      return FALSE;
   if (NOTACTIVE)
      return TRUE;
   FallenSnow *fsnow = FsnowFirst;
   while(fsnow)
   {
      if (HandleFallenSnow(fsnow)) 
	 if(fsnow->id == 0 || (!fsnow->hidden &&
		  (fsnow->ws == CWorkSpace || fsnow->sticky)))
	    UpdateFallenSnowWithWind(fsnow,fsnow->w/4,fsnow->h/4); 
      fsnow = fsnow->next;
   }
   return TRUE;
}

void DrawFallen(FallenSnow *fsnow)
{
   if(!fsnow->clean)
      if(fsnow->id == 0 || (!fsnow->hidden &&
	       (fsnow->ws == CWorkSpace || fsnow->sticky)))
      {
	 // do not interfere with Santa
	 if(!Flags.NoSanta)
	 {
	    int in = XRectInRegion(SantaPlowRegion, fsnow->x, fsnow->y - fsnow->h,
		  fsnow->w, fsnow->h);
	    if (in == RectangleIn || in == RectanglePart)
	    {
	       // determine front of Santa in fsnow
	       int xfront = SantaX+SantaWidth - fsnow->x;
	       // determine back of Santa in fsnow, Santa can move backwards in strong wind
	       int xback = xfront - SantaWidth;
	       const int clearing = 10;
	       float vy = -1.5*ActualSantaSpeed; 
	       if(vy > 0) vy = -vy;
	       if (vy > -100.0)
		  vy = -100;
	       if (ActualSantaSpeed > 0)
		  GenerateFlakesFromFallen(fsnow,xfront,clearing,vy);
	       CleanFallenArea(fsnow,xback-clearing,SantaWidth+2*clearing);
	       int i;
	       for (i=0; i<fsnow->w; i++)
		  if (i < xfront+clearing && i>=xback-clearing)
		     fsnow->acth[i] = 0;
	       XFlush(display);
	    }
	 }

	 Pixmap pixmap = CreatePixmapFromFallen(fsnow);
	 XSetStipple(display, FallenGC, pixmap);
	 XFreePixmap(display,pixmap);
	 int x = fsnow->x;
	 int y = fsnow->y - fsnow->h;
	 XSetFillStyle( display, FallenGC, FillStippled);
	 XSetFunction(  display, FallenGC, GXcopy);
	 XSetForeground(display, FallenGC, SnowcPix);
	 XSetTSOrigin(  display, FallenGC, x+fsnow->w, y+fsnow->h);
	 XFillRectangle(display, SnowWin,  FallenGC, x,y, fsnow->w, fsnow->h);
      }
}

// clean area for fallensnow with id
void CleanFallen(Window id)
{
   FallenSnow *fsnow = FsnowFirst;
   // search the id
   while(fsnow)
   {
      if(fsnow->id == id)
      {
	 CleanFallenArea(fsnow,0,fsnow->w);
	 break;
      }
      fsnow = fsnow->next;
   }
}

void CleanFallenArea(FallenSnow *fsnow,int xstart,int w)
{
   if(fsnow->clean) 
      return;
   int x = fsnow->x;
   int y = fsnow->y - fsnow->h;
   if(UseAlpha|Flags.UseBG)
      XFillRectangle(display, SnowWin,  EFallenGC, x+xstart,y,
	    w, fsnow->h+MaxSnowFlakeHeight);
   else
      XClearArea(display, SnowWin, x+xstart, y, w, fsnow->h+MaxSnowFlakeHeight, Exposures);
   if(xstart <= 0 && w >= fsnow->w)
      fsnow->clean = 1;
}

void GenerateFlakesFromFallen(FallenSnow *fsnow, int x, int w, float vy)
{
   if (Flags.NoBlowSnow || Flags.NoWind)
      return;
   // animation of fallen fallen snow
   // x-values x..x+w are transformed in flakes, vertical speed vy
   int i;
   int ifirst = x; if (ifirst < 0) ifirst = 0;
   if (ifirst > fsnow->w) ifirst = fsnow->w;
   int ilast  = x+w; if(ilast < 0) ilast = 0;
   if (ilast > fsnow->w) ilast = fsnow->w;
   for (i=ifirst; i<ilast; i+=MaxSnowFlakeWidth)
   {
      int j;
      for(j=0; j<fsnow->acth[i]; j++)
      {
	 int k, kmax = BlowOff();
	 for(k=0; k<kmax; k++)
	 {
	    Snow *flake   = MakeFlake();
	    flake->rx     = fsnow->x + i;
	    flake->ry     = fsnow->y - j;
	    if (Flags.NoWind)
	       flake->vx     = 0;
	    else
	       flake->vx     = NewWind/8;
	    flake->vy     = vy;
	    flake->cyclic = 0;
	    add_flake_to_mainloop(flake);
	 }
      }
   }
}

void EraseFallenPixel(FallenSnow *fsnow, int x)
{
   if(fsnow->acth[x] > 0)
   {
      int x1 = fsnow->x + x;
      int y1 = fsnow->y - fsnow->acth[x];
      if(UseAlpha|Flags.UseBG)
	 XDrawPoint(display, SnowWin, EFallenGC, x1, y1);
      else
	 XClearArea(display, SnowWin, x1 , y1, 1, 1, Exposures);     
      fsnow->acth[x]--;
   }
}

int do_displaychanged()
{
   // if we are snowing in the desktop, we check if the size has changed,
   // this can happen after changing of the displays settings
   // If the size has been changed, we restart the program
   if (Flags.Done)
      return FALSE;
   if (!Isdesktop)
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

int do_event()
{
   if (Flags.Done)
      return FALSE;
   //if(UseAlpha) return; we are tempted, but if the event loop is escaped,
   // a memory leak appears
   XEvent ev;
   XFlush(display);
   while (XPending(display)) 
   {
      XNextEvent(display, &ev);
      if(!UseAlpha) 
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
   EraseStars();
   EraseTrees();
   if(!Flags.NoKeepSnowOnTrees && !Flags.NoTrees)
   {
      XDestroyRegion(SnowOnTreesRegion);
      SnowOnTreesRegion = XCreateRegion();
   }
   if(!Flags.NoTrees)
   {
      XDestroyRegion(TreeRegion);
      TreeRegion = XCreateRegion();
   }
   XClearArea(display, SnowWin, 0,0,0,0,Exposures);

}


int do_newwind()
{
   if (Flags.Done)
      return FALSE;
   if (NOTACTIVE)
      return TRUE;
   //
   // the speed of newwind is pixels/second
   // at steady Wind, eventually all flakes get this speed.
   //
   if(Flags.NoWind) return TRUE;
   static double t0 = -1;
   if (t0<0)
   {
      t0 = wallclock();
      return TRUE;
   }

   float windmax = 100.0;
   float r;
   switch (Wind)
   {
      case(0): 
      default:
	 r = drand48()*Whirl;
	 NewWind += r - Whirl/2;
	 if(NewWind > windmax) NewWind = windmax;
	 if(NewWind < -windmax) NewWind = -windmax;
	 break;
      case(1): 
	 NewWind = Direction*0.6*Whirl;
	 break;
      case(2):
	 NewWind = Direction*1.2*Whirl;
	 break;
   }
   return TRUE;
}


int do_wind()
{
   if (Flags.Done)
      return FALSE;
   if (NOTACTIVE)
      return TRUE;
   if(Flags.NoWind) return TRUE;
   static int first = 1;
   static double prevtime;

   double TNow = wallclock();
   if (first)
   {
      prevtime = TNow;;
      first    = 0;
   }

   // on the average, this function will do something
   // after WindTimer secs

   if ((TNow - prevtime) < 2*WindTimer*drand48()) return TRUE;

   prevtime = TNow;

   if(RandInt(100) > 65)  // Now for some of Rick's magic:
   {
      if(RandInt(10) > 4)
	 Direction = 1;
      else
	 Direction = -1;
      Wind = 2;
      WindTimer = 5;
      //               next time, this function will be active 
      //               after on average 5 secs
   }
   else
   {
      if(Wind == 2)
      {
	 Wind = 1;
	 WindTimer = 3;
	 //                   next time, this function will be active 
	 //                   after on average 3 secs
      }
      else
      {
	 Wind = 0;
	 WindTimer = WindTimerStart;
	 //                   next time, this function will be active 
	 //                   after on average WindTimerStart secs
      }
   }
   return TRUE;
}

// blow snow off trees
void ConvertOnTreeToFlakes()
{
   if(Flags.NoKeepSnowOnTrees || Flags.NoBlowSnow || Flags.NoTrees)
      return;
   int i;
   for (i=0; i<OnTrees; i++)
   {
      int j;
      for (j=0; j<3; j++)
      {
	 int k, kmax = BlowOff();
	 for (k=0; k<kmax; k++)
	 {
	    Snow *flake   = MakeFlake();
	    flake->rx     = SnowOnTrees[i].x;
	    flake->ry     = SnowOnTrees[i].y-5*j;
	    flake->vy     = -10;
	    flake->cyclic = 0;
	    add_flake_to_mainloop(flake);
	 }
      }
   }
   OnTrees = 0;
   XDestroyRegion(SnowOnTreesRegion);
   SnowOnTreesRegion = XCreateRegion();
}

int do_star(Skoordinaten *star)
{
   if (Flags.Done)
      return FALSE;
   if (KillStars)
   {
      NStars--;
      return FALSE;
   }
   if (NOTACTIVE)
      return TRUE;
   int k = star->color;
   int x = star->x;
   int y = star->y;
   int w = starPix.width;
   int h = starPix.height;
   XSetTSOrigin(display, StarGC[k],x+w, y+h);
   XFillRectangle(display,SnowWin,StarGC[k],x,y,w,h);
   XFlush(display);
   return TRUE;
}

int do_ustar(Skoordinaten *star)
{
   if (Flags.Done)
      return FALSE;
   if (KillStars)
      return TRUE;
   if (NOTACTIVE)
      return TRUE;

   star->color = RandInt(STARANIMATIONS);
   return TRUE;
}



// used after kdesetbg: it appears that after kdesetbg 
// we have to wait a second or so and then clear the screen.
int do_clean()
{
   if (Flags.Done)
      return FALSE;
   static int active    = 0;
   static double TStart = 0.0;
   if (active)
   {
      if (wallclock() - TStart > 2.0)
      {
	 ClearScreen();
	 active        = 0;
	 ActivateClean = 0;
      }
   }
   else
   {
      if (ActivateClean)
      {
	 active = 1;
	 TStart = wallclock();
      }
   }
   return TRUE;
}

int do_wupdate()
{
   if (Flags.Done)
      return FALSE;
   if(!Isdesktop) return TRUE;
   if(Flags.NoKeepSWin) return TRUE;
   long r;
   r = GetCurrentWorkspace();
   if(r>=0) 
      CWorkSpace = r;
   else
   {
      Flags.Done = 1;
      return TRUE;
   }

   if(Windows) free(Windows);

   if (GetWindows(&Windows, &NWindows)<0)
   {
      Flags.Done = 1;
      return TRUE;
   };
   // Take care of the situation that the transparent window changes from workspace, 
   // which can happen if in a dynamic number of workspaces environment
   // a workspace is emptied.
   WinInfo *winfo;
   winfo = FindWindow(Windows,NWindows,SnowWin);

   // check also on valid winfo: after toggling 'below'
   // winfo is nil sometimes

   if(UsingTrans && winfo)
   {
      // in xfce and maybe others, workspace info is not to be found
      // in our transparent window. winfo->ws will be 0, and we keep
      // the same value for TransWorkSpace.

      if (winfo->ws)
      {
	 TransWorkSpace = winfo->ws;
      }
   }

   UpdateWindows();
   return TRUE;
}

int do_show_range_etc()
{
   if (Flags.Done)
      return FALSE;

   if (Flags.NoMenu)
      return TRUE;
   ui_show_range_etc();
   return TRUE;
}

int do_show_desktop_type()
{
   if (Flags.NoMenu)
      return TRUE;
   if (IsWayland)
      ui_show_desktop_type("Wayland (Expect some slugginess)");
   else if (IsCompiz)
      ui_show_desktop_type("Compiz");
   else
      ui_show_desktop_type("Probably X11");

   return TRUE;
}


// Have a look at the windows we are snowing on

void UpdateWindows()
{
   typeof(Windows) w;
   typeof(FsnowFirst) f;
   int i;
   // put correct workspace in fallensnow areas
   w = Windows;
   for(i=0; i<NWindows; i++)
   {
      f = FsnowFirst;
      while(f)
      {
	 if(w->id == f->id)
	 {
	    f->ws     = w->ws;
	    f->sticky = w->sticky;
	 }
	 f = f->next;
      }
      w++;
   }
   // add fallensnow regions:
   w = Windows;
   for (i=0; i<NWindows; i++)
   {
      //P("%d %#lx\n",i,w->id);
      {
	 f = FindFallen(FsnowFirst,w->id);
	 if(f)
	 {
	    if ((!f->sticky) && f->ws != CWorkSpace)
	       CleanFallenArea(f,0,f->w);
	 }
	 if (!f)
	 {
	    // window found in Windows, nut not in list of fallensnow,
	    // add it, but not if we are snowing or birding in this window (Desktop for example)
	    // and also not if this window has y == 0
	    //P("add %#lx %d\n",w->id, RunCounter);
	    //PrintFallenSnow(FsnowFirst);
	    if (w->id != SnowWin && w->id != BirdsWin && w->y != 0)
	       PushFallenSnow(&FsnowFirst, w->id, w->ws, w->sticky,
		     w->x+Flags.OffsetX, w->y+Flags.OffsetY, w->w+Flags.OffsetW, 
		     Flags.MaxWinSnowDepth); 
	 }
      }
      w++;
   }
   // remove fallensnow regions
   f = FsnowFirst; int nf = 0; while(f) { nf++; f = f->next; }
   long int *toremove = (long int *)malloc(sizeof(*toremove)*nf);
   int ntoremove = 0;
   f = FsnowFirst;
   Atom wmState = XInternAtom(display, "_NET_WM_STATE", True);
   while(f)
   {
      if (f->id != 0)  // f->id=0: this is the snow at the bottom
      {
	 w = FindWindow(Windows,NWindows,f->id);
	 if(!w)   // this window is gone
	 {
	    GenerateFlakesFromFallen(f,0,f->w,-10.0);
	    toremove[ntoremove++] = f->id;
	 }

	 // test if f->id is hidden. If so: clear the area and notify in f
	 Atom type; int format; unsigned long n, b; unsigned char *properties = 0;
	 XGetWindowProperty(display, f->id, wmState, 0, (~0L), False, AnyPropertyType, 
	       &type, &format, &n, &b, &properties);
	 f->hidden = 0;
	 if(format == 32)
	 {
	    unsigned long i;
	    for (i=0; i<n; i++)
	    {
	       char *s = 0;
	       s = XGetAtomName(display,((Atom*)properties)[i]);
	       if (!strcmp(s,"_NET_WM_STATE_HIDDEN"))
	       { 
		  //P("%#lx is hidden %d\n",f->id, RunCounter);
		  f->hidden = 1;
		  CleanFallenArea(f,0,f->w);
		  if(s) XFree(s);
		  break;
	       }
	       if(s) XFree(s);
	    }
	 }
	 if(properties) XFree(properties);
      }
      f = f->next;
   }

   // test if window has been moved or resized
   // moved: move fallen area accordingly
   // resized: remove fallen area: add it to toremove
   w = Windows;
   for(i=0; i<NWindows; i++)
   {
      f = FindFallen(FsnowFirst,w->id);
      if (f)
      {
	 if ((unsigned int)f->w == w->w+Flags.OffsetW) // width has not changed
	 {
	    if (f->x != w->x + Flags.OffsetX || f->y != w->y + Flags.OffsetY)
	    {
	       CleanFallenArea(f,0,f->w);
	       f->x = w->x + Flags.OffsetX;
	       f->y = w->y + Flags.OffsetY;
	       DrawFallen(f);
	       XFlush(display);
	    }
	 }
	 else
	 {
	    toremove[ntoremove++] = f->id;
	 }
      }
      w++;
   }

   for (i=0; i<ntoremove; i++)
   {
      CleanFallen(toremove[i]);
      RemoveFallenSnow(&FsnowFirst,toremove[i]);
   }
   free(toremove);
}

int do_testing()
{
   counter++;
   if (Flags.Done)
      return FALSE;
   return TRUE;
   static int first = 1;
   if(first)
   {
      first = 0;
      return TRUE;
   }
   EraseTrees();
   return TRUE;
   Region region;
   region = SnowOnTreesRegion;

   XSetFunction(display,   TestingGC, GXcopy);
   XSetForeground(display, TestingGC, BlackPix); 
   XFillRectangle(display, SnowWin, TestingGC, 0,0,SnowWinWidth,SnowWinHeight);
   XSetRegion(display,     TestingGC, region);
   XSetForeground(display, TestingGC, BlackPix); 
   XFillRectangle(display, SnowWin, TestingGC, 0,0,SnowWinWidth,SnowWinHeight);
   return TRUE;
}


/* ------------------------------------------------------------------ */ 
void SigHandler(int signum)
{
   Flags.Done = 1;
}
/* ------------------------------------------------------------------ */ 

int RandInt(int maxVal)
{
   if (maxVal <= 0) maxVal = 1;
   // see http://c-faq.com/lib/randrange.html
   return drand48()*maxVal;
}
/* ------------------------------------------------------------------ */ 

int SnowPtInRect(int snx, int sny, int recx, int recy, int width, int height)
{
   if (snx < recx) return 0;
   if (snx > (recx + width)) return 0;
   if (sny < recy) return 0;
   if (sny > (recy + height)) return 0;
   return 1;
}




int do_initstars()
{
   if (Flags.Done)
      return FALSE;
   if (NStars != 0)
      return TRUE;
   NStars    = Flags.NStars;
   KillStars = 0;
   int i;
   for (i=0; i<NStars; i++)
   {
      Skoordinaten *star = (Skoordinaten *)malloc(sizeof(Skoordinaten));
      star->x = RandInt(SnowWinWidth);
      star->y = RandInt(SnowWinHeight/4);
      star->color = RandInt(STARANIMATIONS);
      add_to_mainloop(PRIORITY_DEFAULT, time_star,  do_star,  star);
      add_to_mainloop(PRIORITY_DEFAULT, drand48()*time_ustar+0.5*time_ustar, do_ustar, star);
   }
   return TRUE;
}

void EraseStars()
{
   KillStars = 1;
   ClearScreen();
}

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

void InitFallenSnow()
{
   while (FsnowFirst)
      PopFallenSnow(&FsnowFirst);
   PushFallenSnow(&FsnowFirst, 0, CWorkSpace, 0, 0, 
	 SnowWinHeight, SnowWinWidth, MaxScrSnowDepth);
}

void UpdateFallenSnowPartial(FallenSnow *fsnow, int x, int w)
{
   if(!HandleFallenSnow(fsnow)) return;
   int imin = x;
   if(imin < 0) imin = 0;
   int imax = x + w;
   if (imax > fsnow->w) imax = fsnow->w;
   int i, k;
   k = 0;
   typeof(fsnow->acth[0]) *old;
   // old will contain the acth values, corresponding with x-1..x+w (including)
   old = (short int *)malloc(sizeof(*old)*(w+2));
   for (i=imin-1; i<=imax; i++) 
   {
      if (i < 0) 
	 old[k++] = fsnow->acth[0];
      else if (i>=fsnow->w)
	 old[k++] = fsnow->acth[fsnow->w-1];
      else
	 old[k++] = fsnow->acth[i];
   }

   int add;
   if (fsnow->acth[imin] < fsnow->desh[imin]/4)
      add = 4;
   else if(fsnow->acth[imin] < fsnow->desh[imin]/2)
      add = 2;
   else
      add = 1;
   k = 1;  // old[1] corresponds with acth[0]
   for (i=imin; i<imax; i++)
   {
      if ((fsnow->desh[i] > old[k]) &&
	    (old[k-1] >= old[k] || old[k+1] >= old[k]))
	 fsnow->acth[i] = add + (old[k-1] + old[k+1])/2;
      k++;
   }
   // old will contain the new acth values, corresponding with x-1..x+w (including)
   k = 0;
   for (i=imin-1; i<=imax; i++) 
   {
      if (i < 0) 
	 old[k++] = fsnow->acth[0];
      else if (i>=fsnow->w)
	 old[k++] = fsnow->acth[fsnow->w-1];
      else
	 old[k++] = fsnow->acth[i];
   }
   // and now some smoothing
   k = 1;
   for (i=imin; i<imax; i++)
   {
      int j;
      int sum=0;
      for (j=k-1; j<=k+1; j++)
	 sum += old[j];
      fsnow->acth[i] = sum/3;
      k++;
   }
   free(old);
   fsnow->clean = 0;
}

// removes some fallen snow from fsnow, w pixels. If fallensnowheight < h: no removal
// also add snowflakes
void UpdateFallenSnowWithWind(FallenSnow *fsnow, int w, int h)
{
   if(Flags.NoBlowSnow)
      return;
   int i;
   int x = RandInt(fsnow->w - w);
   for(i=x; i<x+w; i++)
      if(fsnow->acth[i] > h)
      {
	 // animation of blown off snow
	 if (!Flags.NoWind && !Flags.NoBlowSnow && Wind != 0 && drand48() > 0.5)
	 {
	    int j, jmax = BlowOff();
	    //P("%d\n",jmax);
	    for (j=0; j< jmax; j++)
	    {
	       Snow *flake   = MakeFlake();
	       flake->rx     = fsnow->x + i;
	       flake->ry     = fsnow->y - fsnow->acth[i] - drand48()*2*MaxSnowFlakeWidth;
	       flake->vx     = NewWind/8;
	       flake->vy     = -10;
	       flake->cyclic = (fsnow->id == 0); // not cyclic for Windows, cyclic for bottom
	       add_flake_to_mainloop(flake);
	    }
	    EraseFallenPixel(fsnow,i);
	 }
      }
}

Pixmap CreatePixmapFromFallen(FallenSnow *f)
{
   // todo: takes too much cpu
   int j;
   int p = 0;
   unsigned char *bitmap = (unsigned char *) alloca((f->w8*f->h/8)*sizeof(unsigned char));

   for (j=0; j<f->h; j++)
   {
      int i;
      for (i=0; i<f->w8; i+=8)
      {
	 int b = 0;
	 int m = 1;
	 int k;
	 int kmax = i+8;
	 if (kmax > f->w) kmax = f->w;
	 for (k=i; k<kmax; k++)
	 {
	    if(f->acth[k] >= f->h-j)
	       b |= m;
	    m <<= 1;
	 }
	 bitmap[p++] = b;
      }
   }
   Pixmap pixmap = XCreateBitmapFromData(display, SnowWin, (char *)bitmap, f->w, f->h);
   return pixmap;
}




/* ------------------------------------------------------------------ */ 
int XsnowErrors(Display *dpy, XErrorEvent *err)
{
   if (Flags.Quiet) return 0;
   char msg[1024];
   XGetErrorText(dpy, err->error_code, msg,sizeof(msg));
   P("%s\n",msg);
   return 0;
}
/* ------------------------------------------------------------------ */ 

/* ------------------------------------------------------------------ */ 


Window XWinInfo(char **name)
{
   Window win = Select_Window(display,1);
   XTextProperty text_prop;
   int rc = XGetWMName(display,win,&text_prop);
   if (!rc)
      (*name) = strdup("No Name");
   else
      (*name) = strndup((char *)text_prop.value,text_prop.nitems);
   XFree(text_prop.value);  // cannot find this in the docs, but otherwise memory leak
   return win;
}

void InitDisplayDimensions()
{
   unsigned int w,h,b,d;
   int x,y;
   unsigned int wroot,hroot,broot,droot;
   int xroot,yroot;
   Window root;
   XGetGeometry(display,RootWindow,&root,
	 &xroot, &yroot, &wroot, &hroot, &broot, &droot);
   Xroot = xroot;
   Yroot = yroot;
   Wroot = wroot;
   Hroot = hroot;
   P("InitDisplayDimensions: %d %d %d %d %d %d\n",xroot,yroot,wroot,hroot,broot,droot);
   XGetGeometry(display,SnowWin,&root,
	 &x, &y, &w, &h, &b, &d);
   P("InitDisplayDimensions: %d %d %d %d %d %d\n",x,y,w,h,b,d);
   SnowWinX           = x;
   SnowWinY           = y;
   SnowWinWidth       = w;
   if(UsingTrans)
      SnowWinHeight      = hroot + Flags.OffsetS;
   else
      SnowWinHeight      = h + Flags.OffsetS;
   if(!Flags.FullScreen && UsingTrans)
      SnowWinHeight -= y;
   SnowWinBorderWidth = b;
   SnowWinDepth       = d;

   SetMaxScreenSnowDepth();
}

void SetMaxScreenSnowDepth()
{
   MaxScrSnowDepth = Flags.MaxScrSnowDepth;
   if (MaxScrSnowDepth> (SnowWinHeight-SNOWFREE)) {
      printf("** Maximum snow depth set to %d\n", SnowWinHeight-SNOWFREE);
      MaxScrSnowDepth = SnowWinHeight-SNOWFREE;
   }
}


int BlowOff()
{
   float g = gaussian(BlowOffFactor,0.5*BlowOffFactor,0.0,2.0*MAXBLOWOFFFACTOR);
   return lrint(g);
}


void InitSnowOnTrees()
{
   free(SnowOnTrees);
   SnowOnTrees = (XPoint *)malloc(sizeof(*SnowOnTrees)*Flags.MaxOnTrees);
}

// the draw callback
gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data) 
{
   P("Just to check if this is darea: %p\n",(void *)widget);
   drawit(cr);
   return FALSE;
}

void drawit(cairo_t *cr)
{
   meteo_draw(cr);

   scenery_draw(cr);

   Santa_draw(cr);

   birds_draw(cr);

   snow_draw(cr);
}

int do_draw_all(gpointer widget)
{
   if (Flags.Done)
      return FALSE;
   static double tprev = 0;
   static int count = 0;
   double tnow = wallcl();
   count++;
   if(tnow-tprev > 1.2*time_draw_all)
      R(" %d %f\n",count,tnow-tprev);
   tprev = tnow;

   gtk_widget_queue_draw(widget);
   return TRUE;
}


// handle callbacks for things whose timings depend on factor
void HandleFactor()
{
   static guint fallen_id=0;
   // re-add things whose timing is dependent on factor
   if (Flags.CpuLoad <= 0)
      factor = 1;
   else
      factor = 100.0/Flags.CpuLoad;

   EraseTrees();

   if (fallen_id)
      g_source_remove(fallen_id);

   if(!GtkWin)
      Santa_HandleFactor();

   fallen_id = add_to_mainloop(PRIORITY_DEFAULT, time_fallen, do_fallen, 0);

   add_to_mainloop(PRIORITY_DEFAULT, 0.2 , do_initsnow, 0);

}

void SetGCFunctions()
{
   int i;
   if (Flags.UseBG)
      ErasePixel = AllocNamedColor(Flags.BGColor,Black) | 0xff000000;
   else
      ErasePixel = 0;

   Santa_set_gc();
   /*
      XSetFunction(display,   ESantaGC, GXcopy);
      XSetFillStyle(display,  ESantaGC, FillSolid);
      XSetForeground(display, ESantaGC, ErasePixel);
      */

   scenery_set_gc();
   /*
      XSetFunction(display,   TreeGC, GXcopy);
      XSetForeground(display, TreeGC, BlackPix);
      XSetFillStyle(display,  TreeGC, FillStippled);
      */

   XSetFunction(display, SnowOnTreesGC, GXcopy);


   snow_set_gc();
   /*
   for (i=0; i<=SNOWFLAKEMAXTYPE; i++) 
   {
      XSetFunction(   display, SnowGC[i], GXcopy);
      XSetStipple(    display, SnowGC[i], snowPix[i].pixmap);
      XSetFillStyle(  display, SnowGC[i], FillStippled);

      XSetFunction(   display, ESnowGC[i], GXcopy);
      XSetStipple(    display, ESnowGC[i], snowPix[i].pixmap);
      XSetForeground( display, ESnowGC[i], ErasePixel);
      XSetFillStyle(  display, ESnowGC[i], FillStippled);
   }
   */

   for (i=0; i<STARANIMATIONS; i++)
   {
      XSetFunction(   display,StarGC[i],GXcopy);
      XSetStipple(    display,StarGC[i],starPix.pixmap);
      XSetForeground( display,StarGC[i],StarcPix[i]);
      XSetFillStyle(  display,StarGC[i],FillStippled);
   }

   XSetFunction(display,CleanGC, GXcopy);
   XSetForeground(display,CleanGC,BlackPix);

   XSetLineAttributes(display, FallenGC, 1, LineSolid,CapRound,JoinMiter);

   XSetFillStyle( display, EFallenGC, FillSolid);
   XSetFunction(  display, EFallenGC, GXcopy);
   XSetForeground(display, EFallenGC, ErasePixel);

}

void SetWhirl()
{
   Whirl = 0.01*Flags.WhirlFactor*WHIRL;
}
void SetWindTimer()
{
   WindTimerStart           = Flags.WindTimer;
   if (WindTimerStart < 3) 
      WindTimerStart = 3;
   WindTimer                = WindTimerStart;
}
void KDESetBG1(const char *color)
{
   kdesetbg(color);
   ActivateClean = 1;
}

int DetermineWindow()
{
   P("DetermineWindow\n");
   if (Flags.WindowId)
   {
      SnowWin = Flags.WindowId;
      Isdesktop = 0;
      UseAlpha  = 0;
   }
   else
   {
      if (Flags.XWinInfoHandling)
      {
	 SnowWin = XWinInfo(&SnowWinName);
	 if (SnowWin == 0)
	 {
	    fprintf(stderr,"XWinInfo failed\n");
	    exit(1);
	 }
	 Isdesktop = 0;
	 UseAlpha  = 0;
      }
      else
      {
	 char *desktopsession = 0;
	 const char *desktops[] = {
	    "DESKTOP_SESSION",
	    "XDG_SESSION_DESKTOP",
	    "XDG_CURRENT_DESKTOP",
	    "GDMSESSION",
	    0
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

	 if (DesktopSession)
	    free(DesktopSession);
	 DesktopSession = strdup(desktopsession);

	 // convert DesktopSession to upper case
	 char *a = DesktopSession;
	 while (*a)
	 {
	    *a = toupper(*a);
	    a++;
	 }
	 IsCompiz = (strstr(DesktopSession,"COMPIZ") != 0);
	 int lxdefound = 0;
	 // if envvar DESKTOP_SESSION == LXDE, search for window with name pcmanfm
	 if (DesktopSession != NULL && !strncmp(DesktopSession,"LXDE",4))
	 {
	    lxdefound = FindWindowWithName("pcmanfm",&SnowWin,&SnowWinName);
	    printf("LXDE session found, searching for window 'pcmanfm'\n");
	 }
	 if(lxdefound)
	 {
	    UseAlpha  = 0;
	    Isdesktop = 1;
	    Exposures = 0;
	 }
	 else
	 {

	    P("DetermineWindow\n");
	    int x,y;
	    unsigned int w,h,b,depth;
	    Window root;
	    XGetGeometry(display,RootWindow,&root,
		  &x, &y, &w, &h, &b, &depth);
	    if(SnowWinName) free(SnowWinName);

	    if (GtkWin)
	    {
	       gtk_window_close(GTK_WINDOW(GtkWin));
	       gtk_widget_destroy(GTK_WIDGET(GtkWin));
	    }
	    if (GtkWinb)
	    {
	       gtk_window_close(GTK_WINDOW(GtkWinb));
	       gtk_widget_destroy(GTK_WIDGET(GtkWinb));
	    }
	    create_transparent_window(Flags.FullScreen, Flags.BelowAll, Flags.AllWorkspaces, 
		  &BirdsWin, "Birds-Window", &SnowWinName, &GtkWinb,w,h);
	    P("birds window %ld %p\n",BirdsWin,(void *)GtkWinb);
	    if (GtkWinb)
	    {

	    }
	    else
	    {
	       printf("Your screen does not support alpha channel, no birds will fly.\n");
	    }
	    create_transparent_window(Flags.FullScreen, Flags.BelowAll, Flags.AllWorkspaces, 
		  &SnowWin, "Xsnow-Window", &SnowWinName, &GtkWin,w,h);
	    P("snow window %ld %p\n",SnowWin,(void *)GtkWin);

	    Isdesktop = 1;
	    UseAlpha  = 1;
	    XGetGeometry(display,SnowWin,&root,
		  &x, &y, &w, &h, &b, &depth);
	    P("depth: %d snowwin: 0x%lx %s\n",depth,SnowWin,SnowWinName);
	    if(SnowWin)
	    {

	       TransWorkSpace = GetCurrentWorkspace();
	       UsingTrans     = 1;

	       drawing_area = gtk_drawing_area_new();
	       gtk_container_add(GTK_CONTAINER(GtkWinb), drawing_area);
	       //gdkwindow   = gtk_widget_get_window(drawing_area);  
	       g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw_event), NULL);


	       if (cairoRegion)
		  cairo_region_destroy(cairoRegion);
	       cairoRegion = cairo_region_create();

	       birds_init();
	    }
	    else
	    {
	       // snow in root window:
	       P("snow in root window\n");
	       Isdesktop = 1;
	       UseAlpha  = 0;
	       if(SnowWinName) free(SnowWinName);
	       SnowWin     = DefaultRootWindow(display);
	       SnowWinName = strdup("No Name");
	    }
	 }
      }
   }
   // override Isdesktop if user desires so:
   if (Flags.Desktop)
      Isdesktop = 1;
   P("Isdesktop: %d\n",Isdesktop);
   if(Isdesktop) 
      CWorkSpace = GetCurrentWorkspace();
   P("CWorkSpace: %ld\n",CWorkSpace);
   if (CWorkSpace < 0)
      return 0;
   InitDisplayDimensions();
   // if depth != 32, we assume that the desktop is not gnome-like TODO
   if (Isdesktop && SnowWinDepth != 32)
      UseAlpha = 0;
   // override UseAlpha if user desires so:
   if (Flags.UseAlpha != SOMENUMBER)
      UseAlpha = Flags.UseAlpha;

   if(Flags.KDEbg)
      KDESetBG1(Flags.BGColor);
   return 1;
}

void HandleExposures()
{
   if (Flags.Exposures == -SOMENUMBER) // no -exposures or -noexposures given
      if (Flags.XWinInfoHandling)
	 Exposures = True;
      else
	 Exposures = False;
   else
      Exposures = Flags.Exposures;

}



