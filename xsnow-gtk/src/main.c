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
/*
   And in a vocoded voice it sounds:
   Xsnow zwei-tausend
   Xsnow two-thousand 
   Xsnow deux mille
   Xsnow dos mil
   etc.
   */

#define debug 0
#define dosync 0  /* synchronise X-server. Change to 1 will detoriate the performance
		     but allow for better analysis
		     */

/*
 * Reals dealing with time are declared as double. 
 * Other reals as float
 */
#include <X11/Intrinsic.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>
#include <X11/xpm.h>
#include <assert.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <math.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alarm.h"
#include "clocks.h"
#include "transparent.h"
#include "csvpos.h"
#include "docs.h"
#include "doit.h"
#include "dsimple.h"
#include "fallensnow.h"
#include "flags.h"
#include "gaussian.h"
#include "ixpm.h"
#include "kdesetbg.h"
#include "pixmaps.h"
#include "snowflakes.h"
#include "ui.h"
#include "version.h"
#include "windows.h"
#include "wmctrl.h"
#include "xsnow.h"
#include "x11cairo.h"

#ifdef DEBUG
#undef DEBUG
#endif
//#define DEBUG
#ifdef DEBUG
#define P(...) printf ("%s: %d: ",__FILE__,__LINE__);printf(__VA_ARGS__)
#else
#define P(...)
#endif
#define R(...) printf ("%s: %d: ",__FILE__,__LINE__);printf(__VA_ARGS__)

// gtk - cairo stuff
static GtkWidget       *gtkwin  = NULL;
static GdkWindow       *gdkwin  = NULL;

#ifndef USEX11
static cairo_surface_t *surface = NULL;
static cairo_t         *cr      = NULL;
static GtkWidget       *darea   = NULL;
#endif

#ifndef USEX11
static gboolean draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
   static int counter =0; if(counter==0){}
   P("draw_cb: %d\n",counter++);
   cairo_set_source_surface (cr, surface, 0, 0);
   cairo_paint (cr);

   return FALSE;
}
#endif

#ifndef USEX11
static gboolean configure_event_cb(GtkWidget *widget,
      GdkEventConfigure *event,
      gpointer          data)
{
   static int counter = 0; if(counter == 0){}
   R("configure event %d\n",counter++);
   if(surface)
      cairo_surface_destroy(surface);
   SnowWinWidth = gtk_widget_get_allocated_width(widget);
   SnowWinHeight = gtk_widget_get_allocated_height(widget);
   surface = gdk_window_create_similar_surface(gtk_widget_get_window (widget),
	 CAIRO_CONTENT_COLOR_ALPHA, SnowWinWidth,SnowWinHeight);
   cr = cairo_create(surface);
   return TRUE;
}
#endif

// from flags.h
FLAGS flags;
FLAGS oldflags;

// from windows.h
Display *MyDisplay;
int     MyScreen;
Window  SnowWin;
int     SnowWinBorderWidth;
int     SnowWinDepth;
int     SnowWinHeight;
int     SnowWinWidth; 
int     SnowWinX; 
int     SnowWinY; 

// locals
// snow flakes stuff
static float    blowofffactor;
static int      DoNotMakeSnow = 0;
static Snow     *firstflake = 0;
static int      flakecount = 0;
static float    flakes_per_sec;
static int      MaxSnowFlakeHeight = 0;  /* Biggest flake */
static int      MaxSnowFlakeWidth = 0;   /* Biggest flake */
static float    SnowSpeedFactor;
static GdkRGBA  snow_rgba;         // GdkRGBA representation
static char     *snow_color = 0;   // textual representation: "green", "#123456"
static int      snow_intcolor = 0; // rgba integer, bytes: alpha, red, green, blue
static int      snow_running = 0;

// fallen snow stuff
static FallenSnow *fsnow_first = 0;
static int        MaxScrSnowDepth = 0;
static int        ontrees = 0;

// miscellaneous
char       Copyright[] = "\nXsnow\nCopyright 1984,1988,1990,1993-1995,2000-2001 by Rick Jansen, all rights reserved, 2019 also by Willem Vermin\n";
static int      activate_clean = 0;  // trigger for do_clean

// tree stuff
static int             ntrees;                       // actual number of trees
static int             ntreetypes = 0;
static Treeinfo        *tree = 0;
static Pixmap          TreeMaskPixmap[MAXTREETYPE+1][2];
static Pixmap          TreePixmap[MAXTREETYPE+1][2];
static int             *TreeType;
static int             treeread = 0;
static int             TreeWidth[MAXTREETYPE+1], TreeHeight[MAXTREETYPE+1];
static char            **treexpm = 0;
#ifndef USEX11
static cairo_surface_t *TreeSurface[MAXTREETYPE+1][2];
#endif

// Santa stuff
static float           ActualSantaSpeed;
static int             CurrentSanta;
static int             oldSantaX=0;  // the x value of Santa when he was last drawn
static int             oldSantaY=0;  // the y value of Santa when he was last drawn
static int             SantaHeight;   
static Pixmap          SantaMaskPixmap[PIXINANIMATION] = {0,0,0,0};
static Pixmap          SantaPixmap[PIXINANIMATION] = {0,0,0,0};
static float           SantaSpeed;  
static int             SantaWidth;
static float           SantaXr;
static int             SantaX;   // should always be lrintf(SantaXr)
static int             SantaY;
static int             SantaYStep;
#ifndef USEX11
static cairo_surface_t *SantaSurface[PIXINANIMATION] = {NULL,NULL,NULL,NULL};
#endif

/* Speed for each Santa  in pixels/second*/
static float Speed[] = {SANTASPEED0,  /* Santa 0 */
   SANTASPEED1,  /* Santa 1 */
   SANTASPEED2,  /* Santa 2 */
   SANTASPEED3,  /* Santa 3 */
   SANTASPEED4,  /* Santa 4 */
};

// star stuff
static Skoordinaten *star = 0;
static int nstars;

// meteorites stuff
static MeteoMap meteorite;

// timing stuff
unsigned int RunCounter = 0;  // from xsnow.h
static double       *Prevtime = 0;
static double       totsleeptime = 0;
static double       tnow;
static double       tstart;

// define unique numbers for alarms:
#define ALARM(x,y) alarm_ ## x,
enum{
   ALARMALL
      lastalarm
};
#undef ALARM

// windows stuff
static int     nwindows, cworkspace = 0;
static Window  rootwindow;
static char    *SnowWinName = 0;
static WinInfo *windows = 0;
static int     exposures;
static int     transworkspace = -1;  // workspace on which transparent window is placed
static int     usingtrans     = 0;   // using transparent window or not

/* Wind stuff */
// wind = 0: no wind
// wind = 1: wind only affecting snow
// wind = 2: wind affecting snow and santa
// direction =  0: no wind direction I guess
// direction =  1: wind from left to right
// direction = -1: wind from right to left
static int    direction = 0;
static double flakesdt;
static float  NewWind = 0;
static float  Whirl;
static int    wind = 0;
static double wind_timer;
static double WindTimer;

// desktop stuff
static int       Isdesktop;
static int       Usealpha;
static XPoint    *snow_on_trees;


/* Colo(u)rs */
static char *meteoColor = "orange";
#ifdef USEX11
static char *blackColor = "black";
static Pixel blackPix;
static Pixel erasePixel;
static Pixel meteoPix;
static Pixel snowcPix;
static Pixel starcPix[STARANIMATIONS];
static Pixel trPix;
static Pixel black, white;
#else
static GdkRGBA meteo_rgba;
static cairo_surface_t *StarSurface[STARANIMATIONS];
#endif
static char *starColor[STARANIMATIONS]  = { "gold", "gold1", "gold4", "orange" };

#ifdef USEX11
/* GC's */
static GC CleanGC;
static GC eFallenGC;
static GC eSantaGC;
static GC eSnowGC[SNOWFLAKEMAXTYPE+1];  // There are SNOWFLAKEMAXTYPE+1 flakes
static GC FallenGC;
static GC SantaGC;
static GC SnowGC[SNOWFLAKEMAXTYPE+1];  // There are SNOWFLAKEMAXTYPE+1 flakes
static GC SnowOnTreesGC;
static GC starGC[STARANIMATIONS];
static GC testingGC;
static GC TreeGC;
#endif

// region stuff
static REGION NoSnowArea_dynamic;
static REGION NoSnowArea_static;
static REGION TreeRegion;
static REGION SantaRegion;
static REGION SantaPlowRegion;
static REGION snow_on_trees_region;

/* Forward decls */
// declare actions for alarms:
#define ALARM(x,y) static void do_ ## x(void);
ALARMALL
#undef ALARM
double Delay[lastalarm];
#ifdef USEX11
static Pixel  AllocNamedColor(char *colorName, Pixel dfltPix);
#endif
static int    BlowOff(void);
static void   CleanFallenArea(FallenSnow *fsnow, int x, int w);
static void   CleanFallen(Window id);
static void   ClearScreen(void);
static void   ConvertOnTreeToFlakes(void);
static void   CreateAlarmDelays(void);
#ifdef USEX11
static Pixmap CreatePixmapFromFallen(struct FallenSnow *f);
#endif
static void   DeleteFlake(Snow *flake);
static int    DetermineWindow(void);
static void   DrawFallen(FallenSnow *fsnow);
static void   DrawSanta1(void);
static void   DrawSanta(void);
static void   DrawSnowFlake(Snow *flake);
static void   DrawTree(int i);
static void   EraseFallenPixel(FallenSnow *fsnow,int x);
static void   EraseStars(void);
static void   EraseTrees(void);
static void   EraseSanta(int x, int y);
static void   EraseSnowFlake(Snow *flake);
static void   GenerateFlakesFromFallen(FallenSnow *fsnow, int x, int w, float vy);
static int    HandleFallenSnow(FallenSnow *fsnow);
static FILE   *HomeOpen(char *file,char *mode,char **path);
#ifdef USEX11
static Pixel  iAllocNamedColor(char *colorName, Pixel dfltPix);
#endif
static void   InitBaumKoordinaten(void);
static void   InitBlowOffFactor(void);
static void   InitDisplayDimensions(void);
static void   InitFallenSnow(void);
static void   InitFlakesPerSecond(void);
static void   ReinitTree0(void);
static void   InitFlake(Snow *flake);
static void   InitSantaPixmaps(void);
static void   InitSnowOnTrees(void);
static void   InitSnowSpeedFactor(void);
static void   InitSnowColor(void);
static void   InitStars(void);
static void   InitTreePixmaps(void);
static void   KdeSetBG1(const char *color);
static int    RandInt(int maxVal);
static void   RedrawTrees(void);
static void   ResetSanta(void);
static void   SetGCFunctions(void);
static void   SetMaxSCRSnowDepth(void);
static void   SetSantaSpeed(void);
static void   SetWhirl(void);
static void   SetWindTimer(void);
static void   SigHandler(int signum);
static void   UpdateFallenSnowPartial(FallenSnow *fsnow, int x, int w);
static void   UpdateFallenSnowWithWind(FallenSnow *fsnow,int w, int h);
static void   UpdateSanta(void);
static void   UpdateSnowFlake(Snow *flake);
static void   UpdateWindows(void);
static void   USleep(long usec);
static int    XsnowErrors(Display *dpy, XErrorEvent *err);
static Window XWinInfo(char **name);

#ifndef USEX11
static void CairoClearRectangle(int x, int y, int w, int h);
static void CairoDrawFlake(Snow *flake, int erase);
static void CairoDrawFallen(FallenSnow *fsnow);
static void CairoDrawTree(int i, int erase);
static void InitStarSurfaces(void);
#endif

static void thanks(void)
{
   printf("\nThank you for using xsnow\n");
}

int main(int argc, char *argv[])
{
   int i;
   PrintVersion();
   // Circumvent wayland problems:before starting gtk: make sure that the 
   // gdk-x11 backend is used.
   // I would prefer if this could be arranged in argc-argv, but 
   // it seems that it cannot be done there.
   setenv("GDK_BACKEND","x11",1);
   InitFlags();
   int rc = HandleFlags(argc, argv);
   switch(rc)
   {
      case -1:
	 thanks();
	 return 1;
	 break;
      case 1:
	 thanks();
	 return 0;
	 break;
      default:
	 break;
   }
#ifndef USEX11
   gdk_rgba_parse(&meteo_rgba,meteoColor);
#endif
   if (!flags.noconfig)
      WriteFlags();
   MyDisplay = XOpenDisplay(flags.display_name);
   XSynchronize(MyDisplay,dosync);
   XSetErrorHandler(XsnowErrors);
   MyScreen = DefaultScreen(MyDisplay);
#ifdef USEX11
   black = BlackPixel(MyDisplay, MyScreen);
   white = WhitePixel(MyDisplay, MyScreen);
#endif


   if (flags.exposures == -SOMENUMBER) // no -exposures or -noexposures given
      if (flags.xwininfohandling)
	 exposures = True;
      else
	 exposures = False;
   else
      exposures = flags.exposures;

   flags.exposures = exposures;   // trouble ?


   TreeType = malloc(sizeof(*TreeType)); // to make realloc() possible in InitBaumKoordinaten

   InitSnowSpeedFactor();
   SetWhirl();
   SetWindTimer();

   snow_on_trees = malloc(sizeof(*snow_on_trees));  // will be remallloced in InitSnowOnTrees
   InitSnowOnTrees();
   InitBlowOffFactor();

   snow_on_trees = malloc(sizeof(*snow_on_trees)*flags.maxontrees);
   star = malloc(sizeof(*star)); // will be re-allocated in InitStars
   tree = malloc(sizeof(*tree)); // will be re-allocated in InitBaumKoordinaten
   srand48((unsigned int)wallclock());
   //signal(SIGKILL, SigHandler);  // wwvv
   signal(SIGINT, SigHandler);
   signal(SIGTERM, SigHandler);
#if debug
   signal(SIGHUP, SigHupHandler);
#else
   signal(SIGHUP, SigHandler);
#endif
   if (MyDisplay == NULL) {
      if (flags.display_name == NULL) flags.display_name = getenv("DISPLAY");
      (void) fprintf(stderr, "%s: cannot connect to X server %s\n", argv[0],
	    flags.display_name ? flags.display_name : "(default)");
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
   // if(Usealpha): drawing to the desktop is as follows:
   //   - all colours are made opaque by or-ing the colors with 0xff000000
   //   - clearing is done writing the same image, but with color black (0x00000000) 
   //   else
   //   - we will use XClearArea to erase flakes and the like. This works well
   //     on fvwm-like desktops (desktop == rootwindow) with exposures set to 0
   //     It works more or less in for example KDE, but exposures must be set to 1
   //     which severely stresses plasma shell (or nautilus-desktop in Gnome, 
   //     but we do not use XClearArea in Gnome).
   //

   rootwindow = DefaultRootWindow(MyDisplay);

   gtk_init(&argc, &argv);
   if (!DetermineWindow())
   {
      printf("xsnow: cannot determine window, exiting...\n");
      return 1;
   }
   printf("Snowing in window: %#lx - \"%s\" - depth: %d - geom: %d %d %dx%d - alpha: %d - exposures: %d\n",
	 SnowWin,SnowWinName,SnowWinDepth,
	 SnowWinX,SnowWinY,SnowWinWidth,SnowWinHeight, Usealpha,exposures);

   NoSnowArea_dynamic   = REGION_CREATE();
   TreeRegion           = REGION_CREATE();
   SantaRegion          = REGION_CREATE();
   SantaPlowRegion      = REGION_CREATE();
   snow_on_trees_region = REGION_CREATE();

   int flake;
   for (flake=0; flake<=SNOWFLAKEMAXTYPE; flake++) 
   {
      SnowMap *rp = &snowPix[flake];
      rp->pixmap = XCreateBitmapFromData(MyDisplay, SnowWin,
	    rp->snowBits, rp->width, rp->height);
      if (rp->height > MaxSnowFlakeHeight) MaxSnowFlakeHeight = rp->height;
      if (rp->width  > MaxSnowFlakeWidth ) MaxSnowFlakeWidth  = rp->width;

      rp->r = regionfromxbm((unsigned char*)rp->snowBits, rp->width, rp->height);
#ifndef USEX11
      rp->pixbuf = gdk_pixbuf_new_from_xpm_data((const char**)Snows[flake]);
      rp->s      = gdk_cairo_surface_create_from_pixbuf(rp->pixbuf,0,0);
      rp->width  = cairo_image_surface_get_width(rp->s);
      rp->height = cairo_image_surface_get_height(rp->s);
#endif
   }
#ifndef USEX11
   InitStarSurfaces();
#endif
   starPix.pixmap = XCreateBitmapFromData(MyDisplay, SnowWin,
	 (char*)starPix.starBits, starPix.width, starPix.height);
   firstflake = createFlake();
   InitFlake(firstflake);
   InitFlakesPerSecond();
   InitSantaPixmaps();
   InitFallenSnow();
   InitStars();
   InitTreePixmaps();  // can change value of flags.nomenu

#define DOIT_I(x) oldflags.x = flags.x;
#define DOIT_L(x) DOIT_I(x);
#define DOIT_S(x) oldflags.x = strdup(flags.x);
   DOITALL;
#undef DOIT_I
#undef DOIT_L
#undef DOIT_S
   if(!flags.nomenu)
      ui(&argc, &argv);

   NoSnowArea_static = TreeRegion;
#ifdef USEX11
   blackPix = AllocNamedColor(blackColor, black);
   //snowcPix = iAllocNamedColor(flags.snowColor, white);   
   meteoPix = iAllocNamedColor(meteoColor, white);
   trPix    = iAllocNamedColor(flags.trColor,   black);
   for(i=0; i<STARANIMATIONS; i++)
      starcPix[i] = iAllocNamedColor(starColor[i], black);

   SantaGC       = XCreateGC(MyDisplay, SnowWin, 0, 0);
   testingGC     = XCreateGC(MyDisplay, rootwindow, 0,0);
   eSantaGC      = XCreateGC(MyDisplay, SnowWin, 0, 0);
   TreeGC        = XCreateGC(MyDisplay, SnowWin, 0, 0);
   SnowOnTreesGC = XCreateGC(MyDisplay, SnowWin, 0, 0);
   CleanGC       = XCreateGC(MyDisplay,SnowWin,0,0);
   FallenGC      = XCreateGC(MyDisplay, SnowWin, 0, 0);
   eFallenGC     = XCreateGC(MyDisplay, SnowWin, 0, 0);  // used to erase fallen snow
   meteorite.gc  = XCreateGC(MyDisplay, SnowWin, 0, 0);
   meteorite.egc = XCreateGC(MyDisplay, SnowWin, 0, 0);
   for (i=0; i<=SNOWFLAKEMAXTYPE; i++) 
   {
      SnowGC[i]  = XCreateGC(MyDisplay, SnowWin, 0, 0);
      eSnowGC[i] = XCreateGC(MyDisplay, SnowWin, 0, 0);
   }
   for (i=0; i<STARANIMATIONS; i++)
      starGC[i]  = XCreateGC(MyDisplay,SnowWin,0,0);

   SetGCFunctions();
#endif

   InitBaumKoordinaten();
   InitSnowColor();

   ResetSanta();   
   // events
   if(Isdesktop)
      XSelectInput(MyDisplay, SnowWin, 0);
   else
      XSelectInput(MyDisplay, SnowWin, 
	    StructureNotifyMask);
   //	    ExposureMask|SubstructureNotifyMask|StructureNotifyMask);

   double Alarm[lastalarm];
   Prevtime = malloc(sizeof(*Prevtime)*lastalarm);
   unsigned int counter[lastalarm];
   for(i=0; i<lastalarm; i++) counter[i] = 0;
   char *names[lastalarm];

   // define names for alarms:
#define ALARM(x,y) names[alarm_ ## x] = #x;
   ALARMALL;
#undef ALARM

   CreateAlarmDelays();
   //
   // about alarm_santa1: if exposures == True, Santa has to
   // be redrawn in a high frequency because there seems
   // to be no way to determine when XClearArea(...,True)
   // is really finished. If alarm_santa1 is set to
   // for example 0.05, and exposures = True, changes
   // are that Santa will not be visible.

   { int i; for(i=0; i<lastalarm; i++) Alarm[i] = wallclock();}

   tstart = wallclock();
   tnow = wallclock();
   for(i=0; i<lastalarm; i++) Prevtime[i] = wallclock();
   flags.done = 0;
   ClearScreen();   // without this, no snow, scenery etc. in KDE
   // main loop
   while (!flags.done)
   {
      if(RunCounter%10 == 0)
      {
	 // check if snow window still exists:
	 XWindowAttributes wattr;
	 if (!XGetWindowAttributes(MyDisplay,SnowWin,&wattr))
	    break;
      }
      int i,action;
      RunCounter++;
      action = 0;
      for (i=1; i<lastalarm; i++)
	 if (Alarm[i] < Alarm[action])
	    action = i;
      double waittime = Alarm[action] - tnow;
      while (snow_running)
      {
	 do_snowflakes();
	 waittime = Alarm[action] - wallclock();
	 if (waittime <=0)
	 {
	    waittime = 0;
	    break;
	 }
      }
      USleep((long)(1e6*(waittime)));
      tnow = wallclock();
      // define actions for alarms:
#define ALARM(x,y) case alarm_ ## x: do_ ## x(); break;
      switch(action)
      {
	 ALARMALL;
      }
#undef ALARM
      Alarm[action]    = tnow + Delay[action]; // set alarm for this action
      Prevtime[action] = tnow;                 // remember time of last action  
      counter[action] ++;
      if (flags.stopafter > 0 && tnow - tstart > flags.stopafter) flags.done = 1;
   }

   if(treexpm) XpmFree(treexpm);
   while (firstflake->next)
      firstflake = delFlake(firstflake);

   free(firstflake);

   if (SnowWinName) free(SnowWinName);
   if (Prevtime) free(Prevtime);

   XClearArea(MyDisplay, SnowWin, 0,0,0,0,True);
   XFlush(MyDisplay);
   XCloseDisplay(MyDisplay); //also frees the GC's, pixmaps and other resources on MyDisplay
   while(fsnow_first)
      PopFallenSnow(&fsnow_first);
   double telapsed = wallclock() - tstart;

   if(flags.showstats)
   {
      printf("\nElapsed: %8.2f seconds\n",telapsed);
      printf("Sleep:   %8.2f seconds = %6.2f%%\n",
	    totsleeptime,100.0*totsleeptime/telapsed);
      printf("Active:  %8.2f seconds = %6.2f%%\n",
	    telapsed - totsleeptime,100.0*(telapsed-totsleeptime)/telapsed);
      printf("                   wakeups   freq    delay   target\n");
      for (i=0; i<lastalarm; i++)
      {
	 double delaytime,frequency;
	 if (telapsed  == 0.0) frequency = 0.0; else frequency = (double)counter[i]/telapsed;
	 if (frequency == 0.0) delaytime = 0.0; else delaytime = 1.0/frequency; 
	 printf("%-15s %10d %6.2f %8.4f %8.4f",names[i],counter[i], frequency, delaytime,Delay[i]);
	 if (delaytime > 1.1*Delay[i]) printf("  *\n"); else printf("\n");
      }
   }

   thanks();
   if(star) free(star);
   if(tree) free(tree);
   return 0;
}		/* End of the snow */
/* ------------------------------------------------------------------ */ 
#define TRANSSKIP \
   if (usingtrans && cworkspace != transworkspace) return

void do_santa()
{
   TRANSSKIP;
   if (!flags.NoSanta)
      DrawSanta();
}
void do_santa1()
{
   TRANSSKIP;
   if (!flags.NoSanta)
      DrawSanta1();
}

void do_ui_loop()
{
   ui_loop();
}

void do_ui_check()
{
   if (flags.nomenu)
      return;
   int changes = 0;
   if (flags.SantaSize != oldflags.SantaSize || 
	 flags.NoRudolf != oldflags.NoRudolf)
   {
      EraseSanta(oldSantaX,oldSantaY);
      InitSantaPixmaps();
      oldflags.SantaSize = flags.SantaSize;
      oldflags.NoRudolf = flags.NoRudolf;
      changes++;
   }
   if (flags.NoSanta != oldflags.NoSanta)
   {
      //P("do_ui_check\n");
      if (flags.NoSanta)
	 EraseSanta(oldSantaX, oldSantaY);
      oldflags.NoSanta = flags.NoSanta;
      changes++;
   }
   if(flags.SantaSpeedFactor != oldflags.SantaSpeedFactor)
   {
      SetSantaSpeed();
      oldflags.SantaSpeedFactor = flags.SantaSpeedFactor;
      changes++;
   }
   if(strcmp(flags.TreeType, oldflags.TreeType))
   {
      //P(%s %s\n",flags.TreeType,oldflags.TreeType);
      RedrawTrees();
      free(oldflags.TreeType);
      oldflags.TreeType = strdup(flags.TreeType);
      changes++;
   }
   if(flags.desired_number_of_trees != oldflags.desired_number_of_trees)
   {
      RedrawTrees();
      oldflags.desired_number_of_trees = flags.desired_number_of_trees;
      changes++;
   }
   if(flags.treefill != oldflags.treefill)
   {
      RedrawTrees();
      oldflags.treefill = flags.treefill;
      changes++;
   }
   if(flags.NoTrees != oldflags.NoTrees)
   {
      RedrawTrees();
      oldflags.NoTrees = flags.NoTrees;
      changes++;
   }
   if(strcmp(flags.trColor, oldflags.trColor))
   {
      //P("%s %s\n",flags.trColor,oldflags.trColor);
      ReinitTree0();
      //RedrawTrees();
      free(oldflags.trColor);
      oldflags.trColor = strdup(flags.trColor);
      changes++;
   }
   if(flags.nstars != oldflags.nstars)
   {
      EraseStars();
      InitStars();
      oldflags.nstars = flags.nstars;
      changes++;
   }
   if(flags.NoMeteorites != oldflags.NoMeteorites)
   {
      oldflags.NoMeteorites = flags.NoMeteorites;
      changes++;
   }
   if(flags.NoSnowFlakes != oldflags.NoSnowFlakes)
   {
      oldflags.NoSnowFlakes = flags.NoSnowFlakes;
      if(flags.NoSnowFlakes)
	 ClearScreen();
      changes++;
   }
   if(flags.snowflakesfactor != oldflags.snowflakesfactor)
   {
      oldflags.snowflakesfactor = flags.snowflakesfactor;
      InitFlakesPerSecond();
      changes++;
   }
   if(strcmp(flags.snowColor, oldflags.snowColor))
   {
      //P("%s %s\n",flags.snowColor,oldflags.snowColor);
      InitSnowColor();
      ClearScreen();
      free(oldflags.snowColor);
      oldflags.snowColor = strdup(flags.snowColor);
      changes++;
   }
   if(flags.SnowSpeedFactor != oldflags.SnowSpeedFactor)
   {
      oldflags.SnowSpeedFactor = flags.SnowSpeedFactor;
      InitSnowSpeedFactor();
      changes++;
   }
   if(flags.blowofffactor != oldflags.blowofffactor)
   {
      oldflags.blowofffactor = flags.blowofffactor;
      InitBlowOffFactor();
      changes++;
   }
   if(flags.NoBlowSnow != oldflags.NoBlowSnow)
   {
      oldflags.NoBlowSnow = flags.NoBlowSnow;
      changes++;
   }
   if(flags.cpuload != oldflags.cpuload)
   {
      oldflags.cpuload = flags.cpuload;
      CreateAlarmDelays();
      changes++;
   }
   if(flags.usebg != oldflags.usebg)
   {
      oldflags.usebg = flags.usebg;
      SetGCFunctions();
      ClearScreen();
      changes++;
   }
   if(flags.KDEbg != oldflags.KDEbg)
   {
      oldflags.KDEbg = flags.KDEbg;
      if (flags.KDEbg)
	 KdeSetBG1(flags.bgcolor);
      else
	 KdeSetBG1(0);
      ClearScreen();
   }
   if(strcmp(flags.bgcolor,oldflags.bgcolor))
   {
      free(oldflags.bgcolor);
      oldflags.bgcolor = strdup(flags.bgcolor);
      SetGCFunctions();
      if(flags.KDEbg)
	 KdeSetBG1(flags.bgcolor);
      ClearScreen();
      changes++;
   }
   if(flags.usealpha != oldflags.usealpha)
   {
      oldflags.usealpha = flags.usealpha;
      Usealpha          = flags.usealpha;
      SetGCFunctions();
      ClearScreen();
      changes++;
   }
   if(flags.exposures != oldflags.exposures)
   {
      oldflags.exposures = flags.exposures;
      exposures          = flags.exposures;
      CreateAlarmDelays();
      ClearScreen();
      changes++;
   }
   if(flags.offset_s != oldflags.offset_s)
   {
      oldflags.offset_s = flags.offset_s;
      InitDisplayDimensions();
      InitFallenSnow();
      InitStars();
      RedrawTrees();
      ClearScreen();
      changes++;
   }
   if(flags.MaxWinSnowDepth != oldflags.MaxWinSnowDepth)
   {
      oldflags.MaxWinSnowDepth = flags.MaxWinSnowDepth;
      InitFallenSnow();
      ClearScreen();
      changes++;
   }
   if(flags.MaxScrSnowDepth != oldflags.MaxScrSnowDepth)
   {
      oldflags.MaxScrSnowDepth = flags.MaxScrSnowDepth;
      SetMaxSCRSnowDepth();
      InitFallenSnow();
      ClearScreen();
      changes++;
   }
   if(flags.maxontrees != oldflags.maxontrees)
   {
      oldflags.maxontrees = flags.maxontrees;
      ClearScreen();
      changes++;
   }
   if(flags.NoFluffy != oldflags.NoFluffy)
   {
      oldflags.NoFluffy = flags.NoFluffy;
      ClearScreen();
      changes++;
   }
   if(flags.NoKeepSnowOnTrees != oldflags.NoKeepSnowOnTrees)
   {
      oldflags.NoKeepSnowOnTrees = flags.NoKeepSnowOnTrees;
      ClearScreen();
      changes++;
   }
   if(flags.NoKeepSBot != oldflags.NoKeepSBot)
   {
      oldflags.NoKeepSBot = flags.NoKeepSBot;
      InitFallenSnow();
      ClearScreen();
      changes++;
   }
   if(flags.NoKeepSWin != oldflags.NoKeepSWin)
   {
      oldflags.NoKeepSWin = flags.NoKeepSWin;
      InitFallenSnow();
      ClearScreen();
      changes++;
   }
   if(flags.NoWind != oldflags.NoWind)
   {
      oldflags.NoWind = flags.NoWind;
      changes++;
   }
   if(flags.WhirlFactor != oldflags.WhirlFactor)
   {
      oldflags.WhirlFactor = flags.WhirlFactor;
      SetWhirl();
      changes++;
   }
   if(flags.WindTimer != oldflags.WindTimer)
   {
      oldflags.WindTimer = flags.WindTimer;
      SetWindTimer();
      changes++;
   }
   if(flags.fullscreen != oldflags.fullscreen)
   {
      oldflags.fullscreen = flags.fullscreen;
      DetermineWindow();
      InitFallenSnow();
      InitStars();
      RedrawTrees();
      ClearScreen();
      changes++;
   }
   if(flags.below != oldflags.below)
   {
      oldflags.below = flags.below;
      DetermineWindow();
      changes++;
   }
   if(flags.windnow)
   {
      flags.windnow = 0;
      wind = 2;
   }

   if (changes > 0)
   {
      WriteFlags();
   }
}

void ClearScreen()
{
   XClearArea(MyDisplay, SnowWin, 0,0,0,0,True);
}
void RedrawTrees()
{
   EraseTrees();
   InitBaumKoordinaten();
   NoSnowArea_static = TreeRegion;
}

void do_tree()
{
   TRANSSKIP;
   if(!flags.NoTrees)
   {
      int i;
      for (i=0; i<ntrees; i++)
	 DrawTree(i);
   }
}

void do_snow_on_trees()
{
   TRANSSKIP;
   if(flags.NoKeepSnowOnTrees || flags.NoTrees)
      return;
   if (wind == 2)
      ConvertOnTreeToFlakes();

#ifdef USEX11
   static int second = 0;

   if (second)
   {
      second = 1;
      XSetForeground(MyDisplay, SnowOnTreesGC, ~blackPix); 
      XFillRectangle(MyDisplay, SnowWin, SnowOnTreesGC, 0,0,SnowWinWidth,SnowWinHeight);
   }
   XSetRegion(MyDisplay, SnowOnTreesGC, snow_on_trees_region);
   XSetForeground(MyDisplay, SnowOnTreesGC, snowcPix); 
   XFillRectangle(MyDisplay, SnowWin, SnowOnTreesGC, 0,0,SnowWinWidth,SnowWinHeight);
#else

   GdkDrawingContext *c = gdk_window_begin_draw_frame(gdkwin,snow_on_trees_region);
   cairo_t *cc =
      gdk_drawing_context_get_cairo_context (c);
   gdk_cairo_set_source_rgba(cc,&snow_rgba);
   cairo_set_operator(cc, CAIRO_OPERATOR_SOURCE);
   cairo_paint(cc);
   gdk_window_end_draw_frame(gdkwin,c);
   cairo_set_operator(cc, CAIRO_OPERATOR_OVER);
   gtk_main_iteration_do(0);
#endif
}


void do_snowflakes()
{
   TRANSSKIP;
   static Snow *flake;
   //int flakecount_orig = flakecount;
   static double prevtime = 0;
   if (!snow_running)
   {
      flake = firstflake;
      prevtime = Prevtime[alarm_snowflakes];
   }
   flakesdt = wallclock() - prevtime;
   P("do_snow_flakes %f\n",flakesdt);
   int count = 0;

   snow_running = 1;
   while(flake && count++ < SNOWCHUNK)
   {
      Snow *next = flake->next;  // flake can disappear, so we have to save the 
      //                            pointer to the next flake
      UpdateSnowFlake(flake);
      flake = next;
   }
   if(!flake)
   {
      snow_running = 0;

      if(!flags.NoKeepSnowOnTrees && !flags.NoTrees)
      {
	 REGION_SUBTRACT(snow_on_trees_region,TreeRegion);
      }
   }
   P("%d %d %d %d\n",flakecount_orig,flakecount, flakecount_orig - flakecount,snow_running);
}

int HandleFallenSnow(FallenSnow *fsnow)
{
   // soo complicated to determine if a fallensnow should be handled, therefore
   // we isolate this question in this function for further use
   return !((fsnow->id == 0 && flags.NoKeepSBot)||(fsnow->id != 0 && flags.NoKeepSWin)); 
}

void do_fallen()
{
   TRANSSKIP;

   FallenSnow *fsnow = fsnow_first;
   //P("%d\n",RunCounter);
   //PrintFallenSnow(fsnow_first);
   while(fsnow)
   {
      if (HandleFallenSnow(fsnow)) 
	 DrawFallen(fsnow);
      fsnow = fsnow->next;
   }
   XFlush(MyDisplay);
}

void do_blowoff()
{
   TRANSSKIP;
   FallenSnow *fsnow = fsnow_first;
   while(fsnow)
   {
      if (HandleFallenSnow(fsnow)) 
	 if(fsnow->id == 0 || (!fsnow->hidden &&
		  (fsnow->ws == cworkspace || fsnow->sticky)))
	    UpdateFallenSnowWithWind(fsnow,fsnow->w/4,fsnow->h/4); 
      fsnow = fsnow->next;
   }
}

void DrawFallen(FallenSnow *fsnow)
{
   if(!fsnow->clean)
      if(fsnow->id == 0 || (!fsnow->hidden &&
	       (fsnow->ws == cworkspace || fsnow->sticky)))
      {
	 // do not interfere with Santa
	 if(!flags.NoSanta)
	 {
	    REGION_OVERLAP_T in = REGION_OVERLAP_RECT(SantaPlowRegion, 
		  fsnow->x, fsnow->y - fsnow->h, fsnow->w, fsnow->h);

	    if (in == REGION_OVERLAP_RECTANGLE_IN || in == REGION_OVERLAP_RECTANGLE_PART)
	    {
	       // determine front of Santa in fsnow
	       int xfront = SantaX+SantaWidth - fsnow->x;
	       // determine back of Santa in fsnow, Santa can move backwards in srong wind
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
	       XFlush(MyDisplay);
	    }
	 }

#ifdef USEX11
	 int x = fsnow->x;
	 int y = fsnow->y - fsnow->h;
	 Pixmap pixmap = CreatePixmapFromFallen(fsnow);
	 XSetStipple(MyDisplay, FallenGC, pixmap);
	 XFreePixmap(MyDisplay,pixmap);
	 XSetFillStyle( MyDisplay, FallenGC, FillStippled);
	 XSetFunction(  MyDisplay, FallenGC, GXcopy);
	 XSetForeground(MyDisplay, FallenGC, snowcPix);
	 XSetTSOrigin(  MyDisplay, FallenGC, x+fsnow->w, y+fsnow->h);
	 XFillRectangle(MyDisplay, SnowWin,  FallenGC, x,y, fsnow->w, fsnow->h);
#else
	 CairoDrawFallen(fsnow);
#endif
      }
}

#ifndef USEX11
void CairoDrawFallen(FallenSnow *fsnow)
{
   int x = fsnow->x;
   int y = fsnow->y - fsnow->h;
   int w = fsnow->w;
   int h = fsnow->h;
   cairo_surface_t *s = CreateSurfaceFromFallen(fsnow, snow_intcolor);
#ifdef DRAWFRAME
   REGION r = REGION_CREATE_RECTANGLE(x,y,w,h);
   GdkDrawingContext *c = gdk_window_begin_draw_frame(gdkwin, r);
   cairo_t *cc =
      gdk_drawing_context_get_cairo_context (c);
   cairo_set_source_surface(cc,s,x,y);
   cairo_paint(cc);
   gdk_window_end_draw_frame (gdkwin, c);
   REGION_DESTROY(r);
#else
   cairo_set_source_surface(cr,s,x,y);
   cairo_paint(cr);
   gtk_widget_queue_draw_area(GTK_WIDGET(darea),x,y,w,h);
   P("%d %d %d %d %d\n",x,y,w,h,RunCounter);
   gtk_main_iteration_do(0);
#endif
   cairo_surface_destroy(s);
}
#endif

// clean area for fallensnow with id
void CleanFallen(Window id)
{
   FallenSnow *fsnow = fsnow_first;
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
   if(fsnow->clean) return;
   int x = fsnow->x;
   int y = fsnow->y - fsnow->h;
#ifdef USEX11
   if(Usealpha|flags.usebg)
      XFillRectangle(MyDisplay, SnowWin,  eFallenGC, x+xstart,y,
	    w, fsnow->h+MaxSnowFlakeHeight);
   else
      XClearArea(MyDisplay, SnowWin, x+xstart, y, w, fsnow->h, exposures);
#else
   CairoClearRectangle(x,y,w,fsnow->h);
#endif
   fsnow->clean = 1;
}

#ifndef USEX11
void CairoClearRectangle(int x, int y, int w, int h)
{
   P("%d %d %d %d\n",x,y,w,h);
#ifdef DRAWFRAME
   cairo_region_t *r = REGION_CREATE_RECTANGLE(x,y,w,h);
   GdkDrawingContext *c = gdk_window_begin_draw_frame(gdkwin,r);
   cairo_t *cc = gdk_drawing_context_get_cairo_context(c);
   gdk_cairo_set_source_rgba(cc,&(GdkRGBA){0,0,0,0});
   cairo_set_operator(cc,CAIRO_OPERATOR_SOURCE);
   cairo_paint(cc);
   gdk_window_end_draw_frame(gdkwin,c);
   cairo_set_operator(cc,CAIRO_OPERATOR_OVER);
   cairo_region_destroy(r);
#else
   cairo_set_operator(cr,CAIRO_OPERATOR_CLEAR);
   cairo_rectangle(cr,x,y,w,h);
   cairo_fill(cr);
   cairo_set_operator(cr,CAIRO_OPERATOR_OVER);
   gtk_widget_queue_draw_area(GTK_WIDGET(darea),x,y,w,h);
   gtk_main_iteration_do(0);
#endif
}
#endif

void GenerateFlakesFromFallen(FallenSnow *fsnow, int x, int w, float vy)
{
   if (flags.NoBlowSnow)
      return;
   if (flags.NoWind)
      return;
   if (flakecount > flags.flakecountmax || DoNotMakeSnow)
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
	    firstflake = addFlake(firstflake);
	    InitFlake(firstflake);
	    firstflake->rx     = fsnow->x + i;
	    firstflake->ry     = fsnow->y - j;
	    if (flags.NoWind)
	       firstflake->vx     = 0;
	    else
	       firstflake->vx     = NewWind/8;
	    firstflake->vy     = vy;
	    firstflake->cyclic = 0;
	 }
	 //P("%f %f\n",firstflake->rx, firstflake->ry);
      }
   }
}

void EraseFallenPixel(FallenSnow *fsnow, int x)
{
   if(fsnow->acth[x] > 0)
   {
      int x1 = fsnow->x + x;
      int y1 = fsnow->y - fsnow->acth[x];
#ifdef USEX11
      if(Usealpha|flags.usebg)
	 XDrawPoint(MyDisplay, SnowWin, eFallenGC, x1, y1);
      else
	 XClearArea(MyDisplay, SnowWin, x1 , y1, 1, 1, exposures);     
#else
#ifdef DRAWFRAME
      REGION r = REGION_CREATE_RECTANGLE(x1,y1,1,1);
      GdkDrawingContext *c = 
	 gdk_window_begin_draw_frame(gdkwin,r);
      cairo_t *cc = 
	 gdk_drawing_context_get_cairo_context(c);
      gdk_cairo_set_source_rgba(cc,&(GdkRGBA){0,0,0,0});
      cairo_set_operator(cc,CAIRO_OPERATOR_SOURCE);
      cairo_paint(cc);
      gdk_window_end_draw_frame(gdkwin,c);
      cairo_set_operator(cc,CAIRO_OPERATOR_OVER);
#else
      cairo_set_operator(cr,CAIRO_OPERATOR_CLEAR);
      cairo_rectangle(cr,x1,y1,1,1);
      cairo_fill(cr);
      cairo_set_operator(cr,CAIRO_OPERATOR_OVER);
      gtk_widget_queue_draw_area(GTK_WIDGET(darea),x1,y1,1,1);
#endif
      gtk_main_iteration_do(0);
#endif
      fsnow->acth[x]--;
   }
}

// smooth fallen snow
void do_sfallen()
{
   TRANSSKIP;
   return; // taken care of in UpdateFallenSnowPartial()
   FallenSnow *fsnow = fsnow_first;
   while(fsnow)
   {
      if(!fsnow->clean)
	 if(fsnow->id == 0 || ((!fsnow->hidden) &&
		  (fsnow->ws == cworkspace || fsnow->sticky)))
	 {
	    int i;
	    typeof(fsnow->acth) old;
	    old = malloc(fsnow->w*sizeof(*old));
	    for(i=0; i<fsnow->w; i++)
	       old[i] = fsnow->acth[i];
	    // make fsnow->acth the running average of nav+nav+1
	    int nav = 3;
	    if(0)
	       for(i=0; i< fsnow->w; i++)
	       {
		  int j;
		  int sum = 0;
		  int jmin = i-nav;
		  if(jmin<0) jmin = 0;
		  int jmax = i+nav;
		  if(jmax > fsnow->w) jmax = fsnow->w;
		  for (j=jmin; j<jmax; j++)
		     if(old[j]>=2)
			sum += old[j];
		     else
			sum++;
		  int h = sum/(jmax-jmin);
		  if (h > fsnow->desh[i]) h = fsnow->desh[i];
		  while(fsnow->acth[i] > h)
		     EraseFallenPixel(fsnow,i);
		  fsnow->acth[i] = h;
	       }
	    free(old);
	 }
      fsnow = fsnow->next;
   }
}

void do_usanta() { 
   TRANSSKIP;
   UpdateSanta(); 
}

void do_event()
{
   //if(Usealpha) return; we are tempted, but if the event loop is escaped,
   // a memory leak appears
   XEvent ev;
   XFlush(MyDisplay);

   while (XPending(MyDisplay)) 
   {
      XNextEvent(MyDisplay, &ev);
      if(!Usealpha) 
      {
	 switch (ev.type) 
	 {
	    case ConfigureNotify:
	       R("ConfigureNotify: r=%ld w=%ld geo=(%d,%d,%d,%d) bw=%d root: %d\n", 
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
		  //P("init %d %d\n",ev.xconfigure.width, ev.xconfigure.height);
		  InitDisplayDimensions();
		  InitFallenSnow();
		  InitStars();
		  if(!flags.NoKeepSnowOnTrees && !flags.NoTrees)
		  {
		     REGION_DESTROY(snow_on_trees_region);
		     snow_on_trees_region = REGION_CREATE();
		  }
		  if(!flags.NoTrees)
		  {
		     REGION_DESTROY(TreeRegion);
		     TreeRegion = REGION_CREATE();
		     InitBaumKoordinaten();
		  }
		  NoSnowArea_static = TreeRegion;
		  // gtk_todo
		  XClearArea(MyDisplay, SnowWin, 0,0,0,0,exposures);
	       }
	       break;
	 } 
      }
   }  
}

void do_genflakes()
{
   TRANSSKIP;
   if (DoNotMakeSnow)
      return;
   int desflakes = 1 + lrint((tnow - Prevtime[alarm_genflakes])*flakes_per_sec);
   if (flakecount + desflakes > flags.flakecountmax)
      return;
   int i;
   for(i=0; i<desflakes; i++)
   {
      firstflake = addFlake(firstflake);
      InitFlake(firstflake);
   }
}

void do_newwind()
{
   TRANSSKIP;
   //
   // the speed of newwind is pixels/second
   // at steady wind, eventually all flakes get this speed.
   //
   if(flags.NoWind) return;
   static double t0 = -1;
   if (t0<0)
   {
      t0 = wallclock();
      return;
   }

   //P("oldwind: %f %f %d\n",NewWind,Whirl,wind);
   float windmax = 100.0;
   float r;
   switch (wind)
   {
      case(0): 
      default:
	 r = drand48()*Whirl;
	 NewWind += r - Whirl/2;
	 if(NewWind > windmax) NewWind = windmax;
	 if(NewWind < -windmax) NewWind = -windmax;
	 break;
      case(1): 
	 //NewWind = direction*300;
	 NewWind = direction*0.6*Whirl;
	 break;
      case(2):
	 //NewWind = direction*600;
	 NewWind = direction*1.2*Whirl;
	 break;
   }
   //P(" newwind: %f %f\n",NewWind,r);
}


void do_wind()
{
   TRANSSKIP;
   if(flags.NoWind) return;
   static int first = 1;
   static double prevtime;
   if (first)
   {
      prevtime = tnow;;
      first    = 0;
   }

   // on the average, this function will do something
   // after wind_timer secs

   if ((tnow - prevtime) < 2*wind_timer*drand48()) return;

   prevtime = tnow;

   if(RandInt(100) > 65)  // Now for some of Rick's magic:
   {
      if(RandInt(10) > 4)
	 direction = 1;
      else
	 direction = -1;
      wind = 2;
      wind_timer = 5;
      //               next time, this function will be active 
      //               after on average 5 secs
   }
   else
   {
      if(wind == 2)
      {
	 wind = 1;
	 wind_timer = 3;
	 //                   next time, this function will be active 
	 //                   after on average 3 secs
      }
      else
      {
	 wind = 0;
	 wind_timer = WindTimer;
	 //                   next time, this function will be active 
	 //                   after on average WindTimer secs
      }
   }
}

// blow snow off trees
void ConvertOnTreeToFlakes()
{
   if(flags.NoKeepSnowOnTrees || flags.NoBlowSnow || flags.NoTrees)
      return;
   if (flakecount > flags.flakecountmax || DoNotMakeSnow)
      return;
   int i;
   for (i=0; i<ontrees; i++)
   {
      int j;
      for (j=0; j<3; j++)
      {
	 int k, kmax = BlowOff();
	 for (k=0; k<kmax; k++)
	 {
	    firstflake = addFlake(firstflake);
	    InitFlake(firstflake);
	    firstflake->rx = snow_on_trees[i].x;
	    firstflake->ry = snow_on_trees[i].y-5*j;
	    firstflake->vy = -10;
	    firstflake->cyclic = 0;
	 }
	 //P("%d %d %d\n",flakecount, (int)firstflake->rx,(int)firstflake->ry);
      }
      if(flakecount > flags.flakecountmax)
	 break;
   }
   ontrees = 0;
   REGION_DESTROY(snow_on_trees_region);
   snow_on_trees_region = REGION_CREATE();
}

void do_stars()
{
   TRANSSKIP;
   int i;
   for (i=0; i<nstars; i++)
   {
      int k = star[i].color;
      int x = star[i].x;
      int y = star[i].y;
      int w = starPix.width;
      int h = starPix.height;
#ifdef USEX11
      XSetTSOrigin(MyDisplay, starGC[k],x+w, y+h);
      XFillRectangle(MyDisplay,SnowWin,starGC[k],x,y,w,h);
#else
#ifdef DRAWFRAME
      REGION region;
      region = REGION_CREATE_RECTANGLE(x,y,w,h);
      GdkDrawingContext *c = gdk_window_begin_draw_frame(gdkwin, region);
      cairo_t *cc =
	 gdk_drawing_context_get_cairo_context (c);
      cairo_set_source_surface(cc,StarSurface[k],x,y);
      cairo_paint(cc);
      gdk_window_end_draw_frame (gdkwin, c);
      REGION_DESTROY(region);
#else
      cairo_set_source_surface(cr,StarSurface[k],x,y);
      cairo_paint(cr);
      gtk_widget_queue_draw_area(GTK_WIDGET(darea),x,y,w,h);
#endif
      gtk_main_iteration_do(0);
#endif
   }
#ifdef USEX11
   XFlush(MyDisplay);
#endif
}

void do_ustars()
{
   TRANSSKIP;
   int i;
   for (i=0; i<nstars; i++)
      if (drand48() > 0.7)
	 star[i].color = RandInt(STARANIMATIONS);
}

#ifndef USEX11
void cairoDrawMeteorite(MeteoMap *meteorite, int erase)
{
   P("meteo %d %d %d %d %d\n",erase,
	 meteorite->x1,meteorite->y1,meteorite->x2,meteorite->y2);
   int x = meteorite->x1 < meteorite->x2 ? meteorite->x1 : meteorite->x2;
   int y = meteorite->y1 < meteorite->y2 ? meteorite->y1 : meteorite->y2;
   int w = abs(meteorite->x1-meteorite->x2);
   int h = abs(meteorite->y1-meteorite->y2);
   P("meteo1 %d %d %d %d\n",x,y,w,h);

   if (!erase)
      meteorite->r = REGION_CREATE_RECTANGLE(x,y,w,h);

   GdkDrawingContext *c = 
      gdk_window_begin_draw_frame(gdkwin,meteorite->r);
   cairo_t *cc = 
      gdk_drawing_context_get_cairo_context(c);

   if(erase)
   {
      gdk_cairo_set_source_rgba(cc,&(GdkRGBA){0,0,0,0});
      cairo_set_line_width(cc, 3);
   }
   else
   {
      gdk_cairo_set_source_rgba(cc,&meteo_rgba);
      cairo_set_line_width (cc,2);
   }
   cairo_move_to(cc,meteorite->x1,meteorite->y1);
   cairo_line_to(cc,meteorite->x2,meteorite->y2);
   cairo_stroke(cc);
   gdk_window_end_draw_frame(gdkwin,c);
   //gtk_widget_queue_draw(gtkwin);
}
#endif

void do_meteorite()
{
   TRANSSKIP;
   if(flags.NoMeteorites) return;
   if (meteorite.active) return;
   if (RandInt(1000) > 200) return;
   meteorite.x1 = RandInt(SnowWinWidth);
   meteorite.y1 = RandInt(SnowWinHeight/4);
   meteorite.x2 = meteorite.x1 + SnowWinWidth/10 - RandInt(SnowWinWidth/5);
   if (meteorite.x2 == meteorite.x1)
      meteorite.x2 +=5;
   meteorite.y2 = meteorite.y1 + SnowWinHeight/5 - RandInt(SnowWinHeight/5);
   if (meteorite.y2 == meteorite.y1)
      meteorite.y2 +=5;
   meteorite.active  = 1;
#if USEX11
   const int npoints = 5;
   XPoint points[npoints];
   points[0].x = meteorite.x1+1;
   points[0].y = meteorite.y1-1;
   points[1].x = meteorite.x2+1;
   points[1].y = meteorite.y2-1;
   points[2].x = meteorite.x2-1;
   points[2].y = meteorite.y2+1;
   points[3].x = meteorite.x1-1;
   points[3].y = meteorite.y1+1;
   points[4].x = meteorite.x1+1;
   points[4].y = meteorite.y1-1;
   // here sometimes: realloc(): invalid next size
   meteorite.r = XPolygonRegion(points,npoints,EvenOddRule);
   REGION_UNION(NoSnowArea_dynamic,meteorite.r);
   XDrawLine(MyDisplay, SnowWin, meteorite.gc, 
	 meteorite.x1,meteorite.y1,meteorite.x2,meteorite.y2);
   XFlush(MyDisplay);
#else
   // gtk_todo
   cairoDrawMeteorite(&meteorite,0);
#endif

   meteorite.starttime = wallclock();
}

void do_emeteorite()
{
   TRANSSKIP;
   if(flags.NoMeteorites) return;
#if USEX11
   if (meteorite.active)
      if (wallclock() - meteorite.starttime > 0.3)
      {
	 XDrawLine(MyDisplay, SnowWin, meteorite.egc,  
	       meteorite.x1,meteorite.y1,meteorite.x2,meteorite.y2);
	 REGION_SUBTRACT(NoSnowArea_dynamic ,meteorite.r);
	 REGION_DESTROY(meteorite.r);

	 meteorite.active = 0;
      }
   XFlush(MyDisplay);
#else
   if (meteorite.active)
   {
      cairoDrawMeteorite(&meteorite,1);
      meteorite.active = 0;
   }
#endif
}

// used after kdesetbg: it appears that after kdesetbg 
// we have to wait a second or so and then clear the screen.
void do_clean()
{
   static int active = 0;
   static double tstart = 0.0;
   if (active)
   {
      if (wallclock() - tstart > 2.0)
      {
	 //P("do_clean ClearScreen\n");
	 ClearScreen();
	 active         = 0;
	 activate_clean = 0;
      }
   }
   else
   {
      if (activate_clean)
      {
	 active = 1;
	 tstart = wallclock();
      }
   }
}

void do_wupdate()
{
   if(!Isdesktop) return;
   if(flags.NoKeepSWin) return;
   int i;
   i = GetCurrentWorkspace();
   if(i>=0) 
      cworkspace = i;
   else
   {
      flags.done = 1;
      return;
   }

   if(windows) free(windows);

   if (GetWindows(&windows, &nwindows)<0)
   {
      flags.done = 1;
      return;
   };
   // Take care of the situation that the transparent window changes from workspace, 
   // which can happen if in a dynamic number of workspaces environment
   // a workspace is emptied.
   WinInfo *winfo;
   winfo = FindWindow(windows,nwindows,SnowWin);

   //P("SnowWin: 0x%lx\n",SnowWin);
   //P("current workspace: %d %d\n",cworkspace,RunCounter);
   //printwindows(windows, nwindows);
   //P("ws:%d\n",winfo->ws);

   // check also on valid winfo: after toggling 'below'
   // winfo is nil sometimes

   if(usingtrans && winfo)
   {
      //P("winfo: %p\n",(void*)winfo);
      // in xfce and maybe others, workspace info is not to be found
      // in our transparent window. winfo->ws will be 0, and we keep
      // the same value for transworkspace.
      if (winfo->ws)
	 transworkspace = winfo->ws;
   }

   UpdateWindows();
}

void UpdateWindows()
{
   typeof(windows) w;
   typeof(fsnow_first) f;
   int i;
   // put correct workspace in fallensnow areas
   w = windows;
   for(i=0; i<nwindows; i++)
   {
      f = fsnow_first;
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
   w = windows;
   for (i=0; i<nwindows; i++)
   {
      //P("%d %#lx\n",i,w->id);
      {
	 f = FindFallen(fsnow_first,w->id);
	 if(f)
	 {
	    if ((!f->sticky) && f->ws != cworkspace)
	       CleanFallenArea(f,0,f->w);
	 }
	 if (!f)
	 {
	    // window found in windows, nut not in list of fallensnow,
	    // add it, but not if we are snowing in this window (Desktop for example)
	    // and also not if this window has y == 0
	    //P("add %#lx %d\n",w->id, RunCounter);
	    //PrintFallenSnow(fsnow_first);
	    if (w->id != SnowWin && w->y != 0)
	       PushFallenSnow(&fsnow_first, w->id, w->ws, w->sticky,
		     w->x+flags.offset_x, w->y+flags.offset_y, w->w+flags.offset_w, 
		     flags.MaxWinSnowDepth); 
	 }
      }
      w++;
   }
   // remove fallensnow regions
   f = fsnow_first; int nf = 0; while(f) { nf++; f = f->next; }
   long int *toremove = malloc(sizeof(*toremove)*nf);
   int ntoremove = 0;
   f = fsnow_first;
   Atom wmState = XInternAtom(MyDisplay, "_NET_WM_STATE", True);
   while(f)
   {
      if (f->id != 0)  // f->id=0: this is the snow at the bottom
      {
	 w = FindWindow(windows,nwindows,f->id);
	 if(!w)   // this window is gone
	 {
	    GenerateFlakesFromFallen(f,0,f->w,-10.0);
	    toremove[ntoremove++] = f->id;
	 }

	 // test if f->id is hidden. If so: clear the area and notify in f
	 Atom type; int format; unsigned long n, b; unsigned char *properties = 0;
	 XGetWindowProperty(MyDisplay, f->id, wmState, 0, (~0L), False, AnyPropertyType, 
	       &type, &format, &n, &b, &properties);
	 f->hidden = 0;
	 if(format == 32)
	 {
	    int i;
	    for (i=0; i<n; i++)
	    {
	       char *s = 0;
	       s = XGetAtomName(MyDisplay,((Atom*)properties)[i]);
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
   w = windows;
   for(i=0; i<nwindows; i++)
   {
      f = FindFallen(fsnow_first,w->id);
      if (f)
      {
	 if (f->w == w->w+flags.offset_w) // width has not changed
	 {
	    //P("%#lx no change width %d %d %d %d %d %d\n",f->id,f->x,f->y,w->x,w->y,f->w,RunCounter);
	    if (f->x != w->x + flags.offset_x || f->y != w->y + flags.offset_y)
	    {
	       //P("window moved\n");
	       CleanFallenArea(f,0,f->w);
	       f->x = w->x + flags.offset_x;
	       f->y = w->y + flags.offset_y;
	       DrawFallen(f);
	       XFlush(MyDisplay);
	    }
	 }
	 else
	 {
	    //P("%#lx change width\n",f->id);
	    toremove[ntoremove++] = f->id;
	 }
      }
      w++;
   }

   for (i=0; i<ntoremove; i++)
   {
      //P("remove window %#lx\n",toremove[i]);
      CleanFallen(toremove[i]);
      RemoveFallenSnow(&fsnow_first,toremove[i]);
   }
   free(toremove);
}

void do_testing()
{
   return;
   REGION region;
   region = SantaRegion;
   //region = NoSnowArea_static;
   //region = snow_on_trees_region;
   //region = NoSnowArea_dynamic;
   //region = SantaPlowRegion;
   //region = TreeRegion;

#ifdef USEX11
   XSetFunction(MyDisplay,   testingGC, GXcopy);
   XSetForeground(MyDisplay, testingGC, blackPix); 
   XFillRectangle(MyDisplay, SnowWin, testingGC, 0,0,SnowWinWidth,SnowWinHeight);
   XSetRegion(MyDisplay,     testingGC, region);
   XSetForeground(MyDisplay, testingGC, blackPix); 
   XFillRectangle(MyDisplay, SnowWin, testingGC, 0,0,SnowWinWidth,SnowWinHeight);
#else
   // gtk_todo
   GdkDrawingContext *gdkcontext = gdk_window_begin_draw_frame(gdkwin,region);
   cairo_t *cairocontext =
      gdk_drawing_context_get_cairo_context (gdkcontext);
   cairo_set_source_rgb(cairocontext,0,0,0);
   cairo_set_operator(cairocontext, CAIRO_OPERATOR_SOURCE);
   cairo_paint(cairocontext);
   gdk_window_end_draw_frame(gdkwin,gdkcontext);
   cairo_set_operator(cairocontext, CAIRO_OPERATOR_OVER);
#endif
}

void do_fuse()
{
   if (!snow_running) // UpdateSnowFlake would be very confused when
      // flakes suddenly disappear.
      return;
   if (flakecount >= flags.flakecountmax)
   {
      if (!flags.quiet)
	 fprintf(stderr,"fuse blown: remove snowflakes\n");

      while(firstflake->next != 0)
      {
	 EraseSnowFlake(firstflake);
	 DeleteFlake(firstflake);
      }
      DoNotMakeSnow = 1;
   }
   else
      DoNotMakeSnow = 0;
}

/* ------------------------------------------------------------------ */ 
void SigHandler(int signum)
{
   flags.done = 1;
}
/* ------------------------------------------------------------------ */ 

int RandInt(int maxVal)
{
   if (maxVal <= 0) maxVal = 1;
   // return rand() % maxVal;   // Poor
   //return rand() / (RAND_MAX / maxVal + 1);
   // see http://c-faq.com/lib/randrange.html
   return drand48()*maxVal;
}
/* ------------------------------------------------------------------ */ 
void USleep(long usec) 
{
   struct timespec t;
   if (usec <= 0) return;
   totsleeptime += 1e-6*usec;
   t.tv_sec  = usec/1000000;
   t.tv_nsec = 1000*(usec - 1000000*(t.tv_sec));
   nanosleep(&t,0);
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

void InitFlake(Snow *flake)
{
   flake->whatFlake = RandInt(SNOWFLAKEMAXTYPE+1);
   flake->w         = snowPix[flake->whatFlake].width;
   flake->h         = snowPix[flake->whatFlake].height;
   flake->rx = drand48()*(SnowWinWidth - flake->w);
   flake->ry = drand48()*(SnowWinHeight/10);
   flake->cyclic = 1;
   flake->m      = drand48()+0.1;
   if(flags.NoWind)
      flake->vx     = 0; 
   else
      flake->vx     = drand48()*NewWind/2; 
   flake->ivy    = INITIALYSPEED * sqrt(flake->m);
   flake->vy     = flake->ivy;
   flake->wsens  = MAXWSENS*drand48();
   flakecount++;
   //P("%f %f\n",flake->rx, flake->ry);
}


void UpdateSnowFlake(Snow *flake)
{
   //
   // update speed in x direction
   //

   if (!flags.NoWind)
   {
      flake->vx += flakesdt*flake->wsens*(NewWind - flake->vx)/flake->m;
      float speedxmaxes[] = {100.0, 300.0, 600.0,};
      float speedxmax = speedxmaxes[wind];
      if(flake->vx > speedxmax) flake->vx = speedxmax;
      if(flake->vx < -speedxmax) flake->vx = -speedxmax;
   }

   //P("vy: %f\n",flake->vy);
   flake->vy += INITIALYSPEED * (drand48()-0.4)*0.1 ;
   if (flake->vy > flake->ivy*1.5) flake->vy = flake->ivy*1.5;

   //P("%f %f %f\n",flake->vx, NewWind, flakesdt);
   float NewX = flake->rx + (flake->vx*flakesdt)*SnowSpeedFactor;
   float NewY = flake->ry + (flake->vy*flakesdt)*SnowSpeedFactor;
   if(flake->cyclic)
   {
      if (NewX < 0)            NewX = SnowWinWidth-1;
      if (NewX > SnowWinWidth) NewX = 0;
   }
   else if (NewX < 0 || NewX > SnowWinWidth)
   {
      // not-cyclic flakes die when going left or right out of the window
      EraseSnowFlake(flake);
      DeleteFlake(flake);
      return;
   }

   // keep flakes y>0: 
   if (NewY < 0) { NewY = 1; flake->vy = 0;}

   // remove flake if it falls below bottom of screen:
   if (NewY >= SnowWinHeight)
   {
      EraseSnowFlake(flake);
      DeleteFlake(flake);
      return;
   }

   int nx = lrintf(NewX);
   int ny = lrintf(NewY);

   // determine if flake touches the fallen snow,
   // if so: make the flake inactive.
   // the bottom pixels of the snowflake are at y = NewY + (height of flake)
   // the bottompixels are at x values NewX .. NewX+(width of flake)-1

   FallenSnow *fsnow = fsnow_first;
   int found = 0;
   // investigate if flake is in a not-hidden fallensnowarea on current workspace
   while(fsnow && !found)
   {
      if(!fsnow->hidden)
	 if(fsnow->id == 0 ||(fsnow->ws == cworkspace || fsnow->sticky))
	 {
	    if (nx >= fsnow->x && nx <= fsnow->x + fsnow->w &&
		  ny < fsnow->y+2)
	    {
	       int i;
	       int istart = nx     - fsnow->x;
	       int imax   = istart + flake->w;
	       if (istart < 0) istart = 0;
	       if (imax > fsnow->w) imax = fsnow->w;
	       for (i = istart; i < imax; i++)
		  if (ny > fsnow->y - fsnow->acth[i] - 1)
		  {
		     if(fsnow->acth[i] < fsnow->h/2)
			UpdateFallenSnowPartial(fsnow,nx - fsnow->x, flake->w);
		     if(HandleFallenSnow(fsnow))
		     {
			// always erase flake, but repaint it on top of
			// the correct position on fsnow (if !NoFluffy))
			if (flags.NoFluffy)
			   EraseSnowFlake(flake); // flake is removed from screen, but still available
			else
			{
			   // x-value: NewX;
			   // y-value of top of fallen snow: fsnow->y - fsnow->acth[i]
			   flake->rx = NewX;
			   flake->ry = fsnow->y - fsnow->acth[i] - 0.8*drand48()*flake->h;
			   DrawSnowFlake(flake);
			}
			DeleteFlake(flake);
			return;
		     }
		     found = 1;
		     break;
		  }
	    }
	 }
      fsnow = fsnow->next;
   }

   int x = lrintf(flake->rx);
   int y = lrintf(flake->ry);
   REGION_OVERLAP_T in = REGION_OVERLAP_RECT(NoSnowArea_dynamic,x, y, flake ->w, flake->h);
   int b  = (in == REGION_OVERLAP_RECTANGLE_IN || in == REGION_OVERLAP_RECTANGLE_PART); // true if in nosnowarea_dynamic
   //
   // if (b): no erase, no draw, no move
   if(b) return;

   if(wind !=2  && !flags.NoKeepSnowOnTrees && !flags.NoTrees)
   {
      // check if flake is touching or in snow_on_trees_region
      // if so: remove it
      in = REGION_OVERLAP_RECT(snow_on_trees_region,x,y,flake->w,flake->h);
      if (in == REGION_OVERLAP_RECTANGLE_IN || in == REGION_OVERLAP_RECTANGLE_PART)

      {
	 EraseSnowFlake(flake);
	 DeleteFlake(flake);
	 return;
      }

      // check if flake is touching TreeRegion. If so: add snow to 
      // snow_on_trees_region.
      in = REGION_OVERLAP_RECT(TreeRegion,x,y,flake->w,flake->h);
      if (in == REGION_OVERLAP_RECTANGLE_PART)
      {
	 // so, part of the flake is in TreeRegion.
	 // For each bottom pixel of the flake:
	 //   find out if bottompixel is in TreeRegion
	 //   if so:
	 //     move upwards until pixel is not in TreeRegion
	 //     That pixel will be designated as snow-on-tree
	 // Only one snow-on-tree pixel has to be found.
	 int i;
	 int found = 0;
	 for(i=0; i<flake->w; i++)
	 {
	    if(found) break;
	    int ybot = y+flake->h;
	    int xbot = x+i;
	    REGION_OVERLAP_T in = REGION_OVERLAP_RECT(TreeRegion,xbot,ybot,1,1);
	    if (in != REGION_OVERLAP_RECTANGLE_IN) // if bottom pixel not in TreeRegion, skip
	       continue;
	    // move upwards, until pixel is not in TreeRegion
	    int j;
	    for (j=ybot-1; j >= y; j--)
	    {
	       REGION_OVERLAP_T in = REGION_OVERLAP_RECT(TreeRegion,xbot,j,1,1); 
	       if (in != REGION_OVERLAP_RECTANGLE_IN)
	       {
		  // pixel (xbot,j) is snow-on-tree
		  found = 1;
		  int p = RandInt(4)+1;
		  REGION_UNION_RECTANGLE(snow_on_trees_region,xbot,j-p+1,p,p);
		  //P("add to snow_on_trees: %d %d %d %d\n",rec.x,rec.y,rec.width,rec.height);
		  if(!flags.NoBlowSnow && ontrees < flags.maxontrees)
		  {
		     snow_on_trees[ontrees].x = xbot;
		     snow_on_trees[ontrees].y = j-p+1;
		     ontrees++;
		     //P("%d %d %d\n",ontrees,xbot,j-p+1);
		  }
		  break;
	       }
	    }
	    EraseSnowFlake(flake);
	    DeleteFlake(flake);
	    return;
	 }
      }
   }

   in = REGION_OVERLAP_RECT(NoSnowArea_static,x, y, flake ->w, flake->h);
   b  = (in == REGION_OVERLAP_RECTANGLE_IN || in == REGION_OVERLAP_RECTANGLE_PART); // true if in nosnowarea_static

   // if(b): erase: no, move: yes
   // erase this flake 
   if(!b) EraseSnowFlake(flake);
   flake->rx = NewX;
   flake->ry = NewY;
   in = REGION_OVERLAP_RECT(NoSnowArea_static,nx, ny, flake ->w, flake->h);
   b  = (in == REGION_OVERLAP_RECTANGLE_IN || in == REGION_OVERLAP_RECTANGLE_PART); // true if in nosnowarea_static
   // if b: draw: no
   if(!b) DrawSnowFlake(flake);
}

// Note: this function is only to be called in UpdateSnowFlake()
// or after a check if snow_running == 0
void DeleteFlake(Snow *flake)
{
   if (flake->prev)
   {
      delFlake(flake);
      flakecount--;
   }
   else                 //  deleting the first flake, but not if is the only one left
      if(flake->next) 
      {
	 firstflake = delFlake(flake);
	 flakecount--;
      }
}

#ifndef USEX11
void CairoDrawFlake(Snow *flake, int erase)
{
   gint x = lrintf(flake->rx);
   gint y = lrintf(flake->ry);

#ifdef DRAWFRAME
   cairo_region_t *r = snowPix[flake->whatFlake].r; 
   cairo_region_translate(r,x,y);

   P("%d %d %d %d\n",x,y,flakecount,erase);
   GdkDrawingContext *c = 
      gdk_window_begin_draw_frame(gdkwin,r);
   cairo_region_translate(r,-x,-y);
   cairo_t *cc = 
      gdk_drawing_context_get_cairo_context(c);

   if(erase)
      gdk_cairo_set_source_rgba(cc,&(GdkRGBA){0,0,0,0});
   else
      gdk_cairo_set_source_rgba(cc,&snow_rgba);
   cairo_set_operator(cc,CAIRO_OPERATOR_SOURCE);
   cairo_paint(cc);
   gdk_window_end_draw_frame(gdkwin,c);
   cairo_set_operator(cc,CAIRO_OPERATOR_OVER);
#else
   gint w = snowPix[flake->whatFlake].width; 
   gint h = snowPix[flake->whatFlake].height; 
   P("%d %d %d %d %d %d\n",x,y,w,h,flakecount,erase);
   if(erase)
   {
      cairo_set_operator(cr,CAIRO_OPERATOR_CLEAR);
      cairo_rectangle(cr,x,y,w,h);
      cairo_fill(cr);
      cairo_set_operator(cr,CAIRO_OPERATOR_OVER);
      gtk_widget_queue_draw_area(GTK_WIDGET(darea),x,y,w,h);
   }
   else
   {
      cairo_set_source_surface(cr,snowPix[flake->whatFlake].s,x,y);
      cairo_paint(cr);
      gtk_widget_queue_draw_area(GTK_WIDGET(darea),x,y,w,h);
   }
   gtk_main_iteration_do(0);
#endif
}
#endif

void DrawSnowFlake(Snow *flake) // draw snowflake using flake->rx and flake->ry
{
   if(flags.NoSnowFlakes) return;
   P("%d %ld %ld\n",RunCounter,lrint(flake->rx),lrint(flake->ry));
#ifdef USEX11
   int x = lrintf(flake->rx);
   int y = lrintf(flake->ry);
   XSetTSOrigin(MyDisplay, SnowGC[flake->whatFlake], 
	 x + flake->w, y + flake->h);
   XFillRectangle(MyDisplay, SnowWin, SnowGC[flake->whatFlake],
	 x, y, flake->w, flake->h);
#else
   CairoDrawFlake(flake,0);
#endif
}

void EraseSnowFlake(Snow *flake)
{
   if(flags.NoSnowFlakes) return;
#ifdef USEX11
   int x = lrintf(flake->rx);
   int y = lrintf(flake->ry);
   if(Usealpha|flags.usebg)
   {
      XSetTSOrigin(MyDisplay, eSnowGC[flake->whatFlake], 
	    x + flake->w, y + flake->h);
      XFillRectangle(MyDisplay, SnowWin, eSnowGC[flake->whatFlake],
	    x, y, flake->w, flake->h);
   }
   else
      XClearArea(MyDisplay, SnowWin, 
	    x, y,
	    flake->w, flake->h,
	    exposures);
#else
   CairoDrawFlake(flake,1);
#endif
}

// fallen snow and trees must have been initialized 
void InitBaumKoordinaten()
{
   if (flags.NoTrees)
      return;
   int i,h,w;
   free(tree);
   tree = malloc(sizeof(*tree)*flags.desired_number_of_trees);

   // determine which trees are to be used
   //
   int *tmptreetype, ntemp;
   if(treeread)
   {
      TreeType = realloc(TreeType,1*sizeof(*TreeType));
      TreeType[0] = 0;
   }
   else
   {
      if (!strcmp("all",flags.TreeType))
	 // user wants all treetypes
      {
	 ntemp = 1+MAXTREETYPE;
	 tmptreetype = malloc(sizeof(*tmptreetype)*ntemp);
	 int i;
	 for (i=0; i<ntemp; i++)
	    tmptreetype[i] = i;
      }
      else if (strlen(flags.TreeType) == 0) 
	 // default: use 1..MAXTREETYPE 
      {
	 ntemp = MAXTREETYPE;
	 tmptreetype = malloc(sizeof(*tmptreetype)*ntemp);
	 int i;
	 for (i=0; i<ntemp; i++)
	    tmptreetype[i] = i+1;
      }
      else
      {
	 // decode string like "1,1,3,2,4"
	 csvpos(flags.TreeType,&tmptreetype,&ntemp);
      }

      ntreetypes = 0;
      for (i=0; i<ntemp; i++)
      {
	 if (tmptreetype[i] >=0 && tmptreetype[i]<=MAXTREETYPE)
	 {
	    int j;
	    int unique = 1;
	    // investigate if this is already contained in TreeType.
	    // if so, do not use it. Note that this algorithm is not
	    // good scalable, when ntemp is large (100 ...) one should
	    // consider an algorithm involving qsort()
	    //
	    for (j=0; j<ntreetypes; j++)
	       if (tmptreetype[i] == TreeType[j])
	       {
		  unique = 0;
		  break;
	       }
	    if (unique) 
	    {
	       TreeType = realloc(TreeType,(ntreetypes+1)*sizeof(*TreeType));
	       TreeType[ntreetypes] = tmptreetype[i];
	       ntreetypes++;
	    }
	 }
      }
      if(ntreetypes == 0)
      {
	 TreeType = realloc(TreeType,sizeof(*TreeType));
	 TreeType[0] = DEFAULTTREETYPE;
	 ntreetypes++;
      }
      free(tmptreetype);
   }

   //P("ntreetypes: %d\n",ntreetypes);
   //for (i=0; i<ntreetypes; i++)
   //  P("%d\n",TreeType[i]);

   // determine placement of trees and ntrees:

   ntrees = 0;
   for (i=0; i< 4*flags.desired_number_of_trees; i++) // no overlap permitted
   {
      if (ntrees >= flags.desired_number_of_trees)
	 break;

      int tt = TreeType[RandInt(ntreetypes)];
      //P("%d %d\n",tt,ntreetypes);
      h = TreeHeight[tt];
      w = TreeWidth[tt];

      int y1 = SnowWinHeight - MaxScrSnowDepth - h;
      int y2 = SnowWinHeight*(1.0 - 0.01*flags.treefill);
      if (y2>y1) y1=y2+1;

      int x = RandInt(SnowWinWidth);
      int y = y1 - RandInt(y1-y2);

      //P("treetry %4d %4d %4d %4d\n",x,y,y1,y2);
      REGION_OVERLAP_T in = REGION_OVERLAP_RECT(TreeRegion,x,y,w,h);
      if (in == REGION_OVERLAP_RECTANGLE_IN || in == REGION_OVERLAP_RECTANGLE_PART)
	 continue;

      //P("treesuc %4d %4d\n",x,y);
      tree[ntrees].x    = x;
      tree[ntrees].y    = y;
      tree[ntrees].type = tt;
      int flop = (drand48()>0.5);
      tree[ntrees].rev  = flop;

      REGION r;

      switch(tt)
      {
	 case -SOMENUMBER:
	    r = regionfromxpm(treexpm,tree[ntrees].rev);
	    break;
	 default:
	    r = regionfromxpm(xpmtrees[tt],tree[ntrees].rev);
	    break;
      }
      REGION_TRANSLATE(r,x,y);
      REGION_UNION(TreeRegion,r);
      REGION_DESTROY(r);
      ntrees++;
   }
   //for(i=0; i<ntrees; i++)
   //  P("%d\n",tree[i].type);
   ontrees = 0;
   return;
}

#ifndef USEX11
void InitStarSurfaces()
{
   int i;
   for(i=0; i<STARANIMATIONS; i++)
   {
      StarSurface[i] = igdk_cairo_surface_create_from_xpm(star_xpm,0);
      GdkRGBA rgba;
      if(!gdk_rgba_parse(&rgba,starColor[i]))
	 gdk_rgba_parse(&rgba,"white");
      surface_change_color(StarSurface[i],&rgba);
   }
}
#endif

void InitStars()
{
   int i;
   free(star);
   nstars = flags.nstars;
   star = malloc(nstars*sizeof(*star));
   for (i=0; i<nstars; i++)
   {
      star[i].x = RandInt(SnowWinWidth);
      star[i].y = RandInt(SnowWinHeight/4);
   }
   for (i=0; i<nstars; i++)
      star[i].color = RandInt(STARANIMATIONS);
}

void EraseStars()
{
   int i;
   for (i=0; i<nstars; i++)
   {
      int x = star[i].x; 
      int y = star[i].y; 
      int w = starPix.width;
      int h = starPix.height;
#ifdef USEX11
      if(Usealpha|flags.usebg)
	 XFillRectangle(MyDisplay, SnowWin, eSantaGC, 
	       x, y, w, h);
      else
	 XClearArea(MyDisplay, SnowWin,
	       x, y, w, h, exposures);
#else
#ifdef DRAWFRAME
      REGION region;
      region = REGION_CREATE_RECTANGLE(x,y,w,h);
      GdkDrawingContext *c = gdk_window_begin_draw_frame(gdkwin, region);
      cairo_t *cc =
	 gdk_drawing_context_get_cairo_context (c);
      cairo_set_source_rgba(cc,0,0,0,0);
      cairo_set_operator(cc, CAIRO_OPERATOR_SOURCE);
      cairo_paint(cc);
      gdk_window_end_draw_frame(gdkwin, c);
      cairo_set_operator(cc, CAIRO_OPERATOR_OVER);
#else
      cairo_set_operator(cr,CAIRO_OPERATOR_CLEAR);
      cairo_rectangle(cr,x,y,w,h);
      cairo_fill(cr);
      cairo_set_operator(cr,CAIRO_OPERATOR_OVER);
      gtk_widget_queue_draw_area(GTK_WIDGET(darea),x,y,w,h);
#endif
#endif
   }
}

void InitFallenSnow()
{
   //P("%d\n",SnowWinHeight);
   while (fsnow_first)
      PopFallenSnow(&fsnow_first);
   PushFallenSnow(&fsnow_first, 0, cworkspace, 0, 0, 
	 SnowWinHeight, SnowWinWidth, MaxScrSnowDepth);
}

void UpdateFallenSnowPartial(FallenSnow *fsnow, int x, int w)
{
   //P("%#lx %d %d %d\n",fsnow ->id, x,w,fsnow->ws);
   if(!HandleFallenSnow(fsnow)) return;
   int imin = x;
   if(imin < 0) imin = 0;
   int imax = x + w;
   if (imax > fsnow->w) imax = fsnow->w;
   int i, k;
   k = 0;
   typeof(fsnow->acth[0]) *old;
   // old will contain the acth values, corresponding with x-1..x+w (including)
   old = malloc(sizeof(*old)*(w+2));
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
   if(flags.NoBlowSnow)
      return;
   if (flakecount > flags.flakecountmax || DoNotMakeSnow)
      return;
   //P("%#lx\n",fsnow->id);
   int i;
   int x = RandInt(fsnow->w - w);
   for(i=x; i<x+w; i++)
      if(fsnow->acth[i] > h)
      {
	 // animation of blown off snow
	 //if (!flags.NoBlowSnow && abs(NewWind) > 100 && drand48() > 0.5)
	 if (!flags.NoWind && !flags.NoBlowSnow && wind != 0 && drand48() > 0.5)
	 {
	    int j, jmax = BlowOff();
	    //P("%d\n",jmax);
	    for (j=0; j< jmax; j++)
	    {
	       firstflake = addFlake(firstflake);
	       InitFlake(firstflake);
	       firstflake->rx     = fsnow->x + i;
	       firstflake->ry     = fsnow->y - fsnow->acth[i] - drand48()*2*MaxSnowFlakeWidth;
	       firstflake->vx     = NewWind/8;
	       firstflake->vy     = -10;
	       firstflake->cyclic = (fsnow->id == 0); // not cyclic for windows, cyclic for bottom
	    }
	    EraseFallenPixel(fsnow,i);
	 }
	 if(flakecount > flags.flakecountmax)
	    return;
      }
}

#ifdef USEX11
Pixmap CreatePixmapFromFallen(FallenSnow *f)
{
   bitmap_from_fallen(f,f->w8/8);
   Pixmap pixmap = XCreateBitmapFromData(MyDisplay, SnowWin, (char*)f->map, f->w, f->h);
   return pixmap;
}
#endif

void ResetSanta()      
{
   SantaX = -SantaWidth - ActualSantaSpeed;
   SantaXr = SantaX;
   SantaY = RandInt(SnowWinHeight / 3)+40;
   SantaYStep = 1;
   CurrentSanta = 0;
   REGION_DESTROY(SantaRegion);
   SantaRegion = REGION_CREATE_RECTANGLE(SantaX, SantaY, SantaWidth, SantaHeight);
   REGION_DESTROY(SantaPlowRegion);
   SantaPlowRegion = REGION_CREATE_RECTANGLE(SantaX + SantaWidth, SantaY, 1, SantaHeight);
}

void UpdateSanta()
{
   if(flags.NoSanta)
      return;
   int oldx = SantaX;
   int oldy = SantaY;
   static double dtt = 0;
   double dt = tnow - Prevtime[alarm_usanta];
   ActualSantaSpeed += dt*(SANTASENS*NewWind+SantaSpeed - ActualSantaSpeed);
   if (ActualSantaSpeed>3*SantaSpeed)
      ActualSantaSpeed = 3*SantaSpeed;
   else if (ActualSantaSpeed < -2*SantaSpeed)
      ActualSantaSpeed = -2*SantaSpeed;

   SantaXr += dt*ActualSantaSpeed;
   if (SantaXr >= SnowWinWidth) 
   {
      ResetSanta(); 
      oldx = SantaX;
      oldy = SantaY;
   }
   if (SantaXr < -SantaWidth-ActualSantaSpeed) SantaXr = -SantaWidth - ActualSantaSpeed; 
   SantaX = lrintf(SantaXr);
   //P("%f %f %f %f\n",dt,ActualSantaSpeed,SantaXr,NewWind);
   dtt += dt;
   if (dtt > 0.1 && fabs(ActualSantaSpeed) > 3)
   {
      dtt = 0;
      CurrentSanta++;
      if (CurrentSanta >= PIXINANIMATION) CurrentSanta = 0;
   }
   if (RandInt(10) > 3) SantaY = SantaY + SantaYStep; 
   if (SantaY < 0) SantaY = 0;
   if (RandInt(100) > 80) SantaYStep = -SantaYStep;

   REGION_TRANSLATE(SantaRegion,     SantaX - oldx, SantaY - oldy);
   REGION_TRANSLATE(SantaPlowRegion, SantaX - oldx, SantaY - oldy);

   return;
}

#ifndef USEX11
void cairoDrawSanta(int x, int y, int erase)
{
#ifdef DRAWFRAME
   REGION r;
   // should use SantaRegion, but that one seems to have slightly wrong x and y.
   int w = SantaWidth;
   if(erase)
      w += 4;
   r = REGION_CREATE_RECTANGLE(x,y,w, SantaHeight);
   GdkDrawingContext *c = gdk_window_begin_draw_frame(gdkwin, r);
   cairo_t *cc =
      gdk_drawing_context_get_cairo_context (c);
   if(erase)
   {
      cairo_set_source_rgba(cc,0,0,0,0);
      cairo_set_operator(cc, CAIRO_OPERATOR_SOURCE);
      cairo_paint(cc);
      cairo_set_operator(cc, CAIRO_OPERATOR_OVER);
   }
   else
   {
      cairo_set_source_surface(
	    cc,SantaSurface[CurrentSanta],x,y);
      cairo_paint(cc);
   }
   gdk_window_end_draw_frame (gdkwin, c);
   REGION_DESTROY(r);
#else
   int w = SantaWidth;
   int h = SantaHeight;
   if(erase)
   {
      w += 4;
      cairo_set_operator(cr,CAIRO_OPERATOR_CLEAR);
      cairo_rectangle(cr,x,y,w,h);
      cairo_fill(cr);
      cairo_set_operator(cr,CAIRO_OPERATOR_OVER);
      gtk_widget_queue_draw_area(GTK_WIDGET(darea),x,y,w,h);
   }
   else
   {
      cairo_set_source_surface(cr,SantaSurface[CurrentSanta],x,y);
      cairo_paint(cr);
      gtk_widget_queue_draw_area(GTK_WIDGET(darea),x,y,w,h);
   }
   gtk_main_iteration_do(0);
#endif
}
#endif

void DrawSanta() 
{
   if(oldSantaX != SantaX || oldSantaY != SantaY)
      EraseSanta(oldSantaX,oldSantaY);
   DrawSanta1();
   oldSantaX = SantaX;
   oldSantaY = SantaY;
   /* Note: the fur in his hat is *imitation* white-seal fur, of course. */
   /* Santa is a big supporter of Greenpeace.                            */
}

void DrawSanta1()
{
#ifdef USEX11
   XSetClipMask(MyDisplay,
	 SantaGC,
	 SantaMaskPixmap[CurrentSanta]);
   XSetClipOrigin(MyDisplay,
	 SantaGC,
	 SantaX,SantaY);
   XCopyArea(MyDisplay,
	 SantaPixmap[CurrentSanta],
	 SnowWin,
	 SantaGC,
	 0,0,SantaWidth,SantaHeight,
	 SantaX,SantaY);
#else
   cairoDrawSanta(SantaX, SantaY, 0);
#endif
}

void EraseSanta(int x, int y)
{
#ifdef USEX11
   if(Usealpha|flags.usebg)
      XFillRectangle(MyDisplay, SnowWin, eSantaGC, x,y,SantaWidth+1,SantaHeight);
   // probably due to rounding errors in computing SantaX, one pixel in front 
   // is not erased when leaving out the +1
   else
      XClearArea(MyDisplay, SnowWin,
	    x , y,     
	    SantaWidth+1,SantaHeight,
	    exposures);
#else
   cairoDrawSanta(x,y,1);
#endif
}

void DrawTree(int i) 
{
#ifdef USEX11
   int x = tree[i].x; int y = tree[i].y; int t = tree[i].type; int r = tree[i].rev;
   //P("t = %d\n",t);
   if (t<0) t=0;
   XSetClipMask(MyDisplay, TreeGC, TreeMaskPixmap[t][r]);
   XSetClipOrigin(MyDisplay, TreeGC, x, y);
   XCopyArea(MyDisplay, TreePixmap[t][r], SnowWin, TreeGC, 
	 0,0,TreeWidth[t],TreeHeight[t], x, y);
#else
   CairoDrawTree(i,0);
   do_snow_on_trees();
#endif
}

#ifndef USEX11
void CairoDrawTree(int i, int erase)
{
   P("CairoDrawTree: %d %d %d\n",i,erase,RunCounter);
   int x = tree[i].x; int y = tree[i].y; int t = tree[i].type; int r = tree[i].rev;
   int w = TreeWidth[t]; int h = TreeHeight[t];
   const int d = 3;
   if (erase)
   {
      x -= d; y -= d; w += 2*d; h += 2*d;
   }
#ifdef DRAWFRAME
   REGION region;
   region = REGION_CREATE_RECTANGLE(x,y,w,h);
   GdkDrawingContext *c = gdk_window_begin_draw_frame(gdkwin, region);
   cairo_t *cc =
      gdk_drawing_context_get_cairo_context (c);
   if(erase)
   {
      cairo_set_source_rgba(cc,0,0,0,0);
      cairo_set_operator(cc, CAIRO_OPERATOR_SOURCE);
      cairo_paint(cc);
      cairo_set_operator(cc, CAIRO_OPERATOR_OVER);
   }
   else
   {
      cairo_set_source_surface(cc,TreeSurface[t][r],x,y);
      cairo_paint(cc);
   }
   gdk_window_end_draw_frame (gdkwin, c);
   REGION_DESTROY(region);
#else
   if(erase)
   {
      cairo_set_operator(cr,CAIRO_OPERATOR_CLEAR);
      cairo_rectangle(cr,x,y,w,h);
      cairo_fill(cr);
      cairo_set_operator(cr,CAIRO_OPERATOR_OVER);
      gtk_widget_queue_draw_area(GTK_WIDGET(darea),x,y,w,h);
   }
   else
   {
      cairo_set_source_surface(cr,TreeSurface[t][r],x,y);
      cairo_paint(cr);
      gtk_widget_queue_draw_area(GTK_WIDGET(darea),x,y,w,h);
   }
   gtk_main_iteration_do(0);
#endif
}
#endif

void EraseTrees()
{
   int i;
   for (i=0; i<ntrees; i++)
   {
#ifdef USEX11
      const int d = 3;
      int x = tree[i].x-d; 
      int y = tree[i].y-d; 
      int t = tree[i].type; 
      int w = TreeWidth[t]+d+d;
      int h = TreeHeight[t]+d+d;
      if(Usealpha|flags.usebg)
	 XFillRectangle(MyDisplay, SnowWin, eSantaGC, 
	       x, y, w, h);
      else
	 XClearArea(MyDisplay, SnowWin,
	       x, y, w, h, exposures);
#else
      CairoDrawTree(i,1);
#endif
   }

   REGION_DESTROY(TreeRegion);
   TreeRegion = REGION_CREATE();
   REGION_DESTROY(snow_on_trees_region);
   snow_on_trees_region = REGION_CREATE();
   ClearScreen();
}

/* ------------------------------------------------------------------ */ 
int XsnowErrors(Display *dpy, XErrorEvent *err)
{
   if (flags.quiet) return 0;
   char msg[1024];
   XGetErrorText(dpy, err->error_code, msg,sizeof(msg));
   P("%s\n",msg);
   return 0;
}
/* ------------------------------------------------------------------ */ 

#ifdef USEX11
Pixel AllocNamedColor(char *colorName, Pixel dfltPix)
{
   XColor scrncolor;
   XColor exactcolor;
   if (XAllocNamedColor(MyDisplay, DefaultColormap(MyDisplay, MyScreen),
	    colorName, &scrncolor, &exactcolor)) 
      return scrncolor.pixel;
   else
      return dfltPix;
}

Pixel iAllocNamedColor(char *colorName, Pixel dfltPix)
{
   return AllocNamedColor(colorName, dfltPix) | 0xff000000;
}
#endif
/* ------------------------------------------------------------------ */ 


Window XWinInfo(char **name)
{
   Window win = Select_Window(MyDisplay,1);
   XTextProperty text_prop;
   int rc = XGetWMName(MyDisplay,win,&text_prop);
   if (!rc)
      (*name) = strdup("No Name");
   else
      (*name) = strndup((char*)text_prop.value,text_prop.nitems);
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
   XGetGeometry(MyDisplay,rootwindow,&root,
	 &xroot, &yroot, &wroot, &hroot, &broot, &droot);
   // P("%d %d %d %d %d %d\n",xroot,yroot,wroot,hroot,broot,droot);
   XGetGeometry(MyDisplay,SnowWin,&root,
	 &x, &y, &w, &h, &b, &d);
   // P("%d %d %d %d %d %d\n",x,y,w,h,b,d);
   SnowWinX           = x;
   SnowWinY           = y;
   SnowWinWidth       = w;
   if(usingtrans)
      SnowWinHeight      = hroot + flags.offset_s;
   else
      SnowWinHeight      = h + flags.offset_s;
   if(!flags.fullscreen && usingtrans)
      SnowWinHeight -= y;
   SnowWinBorderWidth = b;
   SnowWinDepth       = d;
   //P("%d %d %d %d %d %d\n",SnowWinX,SnowWinY,SnowWinWidth,
   //	 SnowWinHeight,SnowWinDepth,flags.offset_s);

   SetMaxSCRSnowDepth();
}

void SetMaxSCRSnowDepth()
{
   MaxScrSnowDepth = flags.MaxScrSnowDepth;
   if (MaxScrSnowDepth> (SnowWinHeight-SNOWFREE)) {
      printf("** Maximum snow depth set to %d\n", SnowWinHeight-SNOWFREE);
      MaxScrSnowDepth = SnowWinHeight-SNOWFREE;
   }
}


void SetSantaSpeed()
{
   SantaSpeed = Speed[flags.SantaSize];
   if (flags.SantaSpeedFactor < 10)
      SantaSpeed = 0.1*SantaSpeed;
   else
      SantaSpeed = 0.01*flags.SantaSpeedFactor*SantaSpeed;
   ActualSantaSpeed               = SantaSpeed;
}

/* ------------------------------------------------------------------ */ 
void InitSantaPixmaps()
{
   XpmAttributes attributes;
   //P("InitSantaPixmaps: SantaSize=%d NoRudolf=%d\n",flags.SantaSize,flags.NoRudolf);
   //attributes.visual = DefaultVisual(MyDisplay,DefaultScreen(MyDisplay));
   attributes.valuemask = XpmDepth /*| XpmColorKey*/;
   attributes.depth = SnowWinDepth;
   //attributes.color_key = XPM_COLOR;

   SetSantaSpeed();

   char *path[PIXINANIMATION];
   char *filenames[] = 
   {
      "xsnow/pixmaps/santa1.xpm",
      "xsnow/pixmaps/santa2.xpm",
      "xsnow/pixmaps/santa3.xpm",
      "xsnow/pixmaps/santa4.xpm",
   };
   FILE *f;
   int i;
   int ok = 1;
   for (i=0; i<PIXINANIMATION; i++)
   {
      f = HomeOpen(filenames[i],"r",&path[i]);
      if(!f){ ok = 0; free(path[i]); break; }
      fclose(f);
   }
   if (ok)
   {
      printf("Using external Santa: %s.\n",path[0]);
      if (!flags.nomenu)
	 printf("Disabling menu.\n");
      flags.nomenu = 1;
      int rc,i;
      char **santaxpm;
      for (i=0; i<PIXINANIMATION; i++)
      {
	 if(SantaPixmap[i]) 
	    XFreePixmap(MyDisplay,SantaPixmap[i]);
	 if(SantaMaskPixmap[i]) 
	    XFreePixmap(MyDisplay,SantaMaskPixmap[i]);
	 rc = XpmReadFileToData(path[i],&santaxpm);
	 if(rc == XpmSuccess)
	 {
	    iXpmCreatePixmapFromData(MyDisplay, SnowWin, santaxpm, 
		  &SantaPixmap[i], &SantaMaskPixmap[i], &attributes,0);

	    sscanf(*santaxpm,"%d %d",&SantaWidth,&SantaHeight);
	    XpmFree(santaxpm);
	 }
	 else
	 {
	    printf("Invalid external xpm for Santa given: %s\n",path[i]);
	    exit(1);
	 }
	 free(path[i]);
      }
      //P("InitSantaPixmaps: SantaWidth: %d, SantaHeight: %d\n",SantaWidth,SantaHeight);
      return;
   }


   int rc[PIXINANIMATION];
   int withRudolf;
   withRudolf = !flags.NoRudolf;

   for(i=0; i<PIXINANIMATION; i++)
   {
#ifdef USEX11
      if(SantaPixmap[i]) 
	 XFreePixmap(MyDisplay,SantaPixmap[i]);
      if(SantaMaskPixmap[i]) 
	 XFreePixmap(MyDisplay,SantaMaskPixmap[i]);
      rc[i] = iXpmCreatePixmapFromData(MyDisplay, SnowWin, 
	    Santas[flags.SantaSize][withRudolf][i], 
	    &SantaPixmap[i], &SantaMaskPixmap[i], &attributes,0);
#else
      if(SantaSurface[i])
	 cairo_surface_destroy(SantaSurface[i]);
      SantaSurface[i] = igdk_cairo_surface_create_from_xpm(
	    Santas[flags.SantaSize][withRudolf][i],0);
      rc[i] = (SantaSurface[i] == NULL); // cannot happen?
#endif

      sscanf(Santas[flags.SantaSize][withRudolf][0][0],"%d %d", 
	    &SantaWidth,&SantaHeight);
   }

   int wrong = 0;
   for (i=0; i<PIXINANIMATION; i++)
   {
      if (rc[i])
      {
	 printf("Something wrong reading Santa's xpm nr %d: errorstring %s\n",i,XpmGetErrorString(rc[i]));
	 wrong = 1;
      }
   }
   if (wrong) exit(1);
   //P("InitSantaPixmaps: SantaWidth: %d, SantaHeight: %d\n",SantaWidth,SantaHeight);
}		

void InitTreePixmaps()
{
   XpmAttributes attributes;
   attributes.valuemask = XpmDepth;
   attributes.depth     = SnowWinDepth;
   char *path;
   FILE *f = HomeOpen("xsnow/pixmaps/tree.xpm","r",&path);
   if (f)
   {
      // there seems to be a local definition of tree
      // set TreeType to some number, so we can respond accordingly
      free(TreeType);
      TreeType = malloc(sizeof(*TreeType));
      ntreetypes = 1;
      treeread = 1;
      int rc = XpmReadFileToData(path,&treexpm);
      if(rc == XpmSuccess)
      {
	 int i;
	 for(i=0; i<2; i++)
	    iXpmCreatePixmapFromData(MyDisplay, SnowWin, treexpm, 
		  &TreePixmap[0][i], &TreeMaskPixmap[0][i], &attributes,i);
	 sscanf(*treexpm,"%d %d", &TreeWidth[0],&TreeHeight[0]);
	 //P("%d %d\n",TreeWidth[0],TreeHeight[0]);
	 printf("using external tree: %s\n",path);
	 if (!flags.nomenu)
	    printf("Disabling menu.\n");
	 flags.nomenu = 1;
      }
      else
      {
	 printf("Invalid external xpm for tree given: %s\n",path);
	 exit(1);
      }
      fclose(f);
   }
   else
   {
      int i;
      for(i=0; i<2; i++)
      {
	 int tt;
	 for (tt=0; tt<=MAXTREETYPE; tt++)
	 {
#ifdef USEX11
	    iXpmCreatePixmapFromData(MyDisplay, SnowWin, xpmtrees[tt],
		  &TreePixmap[tt][i],&TreeMaskPixmap[tt][i],&attributes,i);
#else
	    TreeSurface[tt][i] = igdk_cairo_surface_create_from_xpm(
		  xpmtrees[tt],i);
#endif
	    sscanf(xpmtrees[tt][0],"%d %d",&TreeWidth[tt],&TreeHeight[tt]);
	 }
      }
      ReinitTree0();
   }
   free(path);
   ontrees = 0;
}

// apply trColor to xpmtree[0] and xpmtree[1]
void ReinitTree0()
{
#ifdef USEX11
   XpmAttributes attributes;
   attributes.valuemask = XpmDepth;
   attributes.depth     = SnowWinDepth;
#endif
   int i;
   int n = TreeHeight[0]+3;
   char *xpmtmp[n];
   int j;
   for (j=0; j<2; j++)
      xpmtmp[j] = strdup(xpmtrees[0][j]);
   xpmtmp[2] = strdup(". c ");
   xpmtmp[2] = realloc(xpmtmp[2],strlen(xpmtmp[2])+strlen(flags.trColor)+1);
   strcat(xpmtmp[2],flags.trColor);
   for(j=3; j<n; j++)
      xpmtmp[j] = strdup(xpmtrees[0][j]);
   for(i=0; i<2; i++)
   {
#ifdef USEX11
      XFreePixmap(MyDisplay,TreePixmap[0][i]);
      iXpmCreatePixmapFromData(MyDisplay, SnowWin, xpmtmp,
	    &TreePixmap[0][i],&TreeMaskPixmap[0][i],&attributes,i);
#else
      cairo_surface_destroy(TreeSurface[0][i]);
      TreeSurface[0][i] = igdk_cairo_surface_create_from_xpm(
	    xpmtrees[0],i);
#endif
   }
   for (j=0; j<n; j++)
      free(xpmtmp[j]);
}


FILE *HomeOpen(char *file,char *mode, char**path)
{
   char *home = strdup(getenv("HOME"));
   (*path) = malloc(strlen(home)+strlen(file)+2);
   strcpy(*path,home);
   strcat(*path,"/");
   strcat(*path,file);
   FILE *f = fopen(*path,mode);
   free(home);
   return f;
}

int BlowOff()
{
   float g = gaussian(blowofffactor,0.5*blowofffactor,0.0,2.0*MAXBLOWOFFFACTOR);
   //P("%f %f %f %ld\n",blowofffactor,2.0*MAXBLOWOFFFACTOR,g,lrint(g));
   return lrint(g);
}

void InitFlakesPerSecond()
{
   flakes_per_sec = SnowWinWidth*0.01*flags.snowflakesfactor*
      0.001*FLAKES_PER_SEC_PER_PIXEL*SnowSpeedFactor;
}

void InitSnowColor()
{
   if (snow_color != NULL)
      free(snow_color);
   snow_color = strdup(flags.snowColor);
#ifdef USEX11
   int i;
   snowcPix = iAllocNamedColor(flags.snowColor, white);   
   for (i=0; i<=SNOWFLAKEMAXTYPE; i++) 
      XSetForeground(MyDisplay, SnowGC[i], snowcPix);
#endif
   if (!gdk_rgba_parse(&snow_rgba,snow_color))
      gdk_rgba_parse(&snow_rgba,"white");
   snow_intcolor = (int)(snow_rgba.alpha*255) << 24 | (int)(snow_rgba.red*255) << 16 |
      (unsigned int)(255*snow_rgba.green) << 8  | (unsigned int)(255*snow_rgba.blue);

#ifndef USEX11
#ifndef DRAWFRAME
   int i;
   for (i=0; i<=SNOWFLAKEMAXTYPE; i++)
   {
      surface_change_color(snowPix[i].s,&snow_rgba);
   }

#endif
#endif
}

void InitSnowSpeedFactor()
{
   if (flags.SnowSpeedFactor < 10)
      SnowSpeedFactor = 0.01*10;
   else
      SnowSpeedFactor = 0.01*flags.SnowSpeedFactor;
}

void InitBlowOffFactor()
{
   blowofffactor = 0.01*flags.blowofffactor;
   if (blowofffactor > MAXBLOWOFFFACTOR)
      blowofffactor = MAXBLOWOFFFACTOR;
}

void InitSnowOnTrees()
{
   free(snow_on_trees);
   snow_on_trees = malloc(sizeof(*snow_on_trees)*flags.maxontrees);
}

void CreateAlarmDelays()
{
   double factor;
   if (flags.cpuload <= 0)
      factor = 1;
   else
      factor = 100.0/flags.cpuload;

   //P("%d %f\n",flags.cpuload,factor);
   //
   // define delays for alarms:
#define ALARM(x,y) Delay[alarm_ ## x] = y;
   ALARMALL;
#undef ALARM
   if (!exposures) 
      Delay[alarm_santa1] = 10*factor;
}

void SetGCFunctions()
{
#ifdef USEX11
   int i;
   if (flags.usebg)
      erasePixel = AllocNamedColor(flags.bgcolor,black) | 0xff000000;
   else
      erasePixel = 0;
   XSetFunction(MyDisplay,   SantaGC, GXcopy);
   XSetForeground(MyDisplay, SantaGC, blackPix);
   XSetFillStyle(MyDisplay,  SantaGC, FillStippled);

   XSetFunction(MyDisplay,   eSantaGC, GXcopy);
   XSetFillStyle(MyDisplay,  eSantaGC, FillSolid);
   XSetForeground(MyDisplay, eSantaGC, erasePixel);

   XSetFunction(MyDisplay,   TreeGC, GXcopy);
   XSetForeground(MyDisplay, TreeGC, blackPix);
   XSetFillStyle(MyDisplay,  TreeGC, FillStippled);

   XSetFunction(MyDisplay, SnowOnTreesGC, GXcopy);


   for (i=0; i<=SNOWFLAKEMAXTYPE; i++) 
   {
      XSetFunction(   MyDisplay, SnowGC[i], GXcopy);
      XSetStipple(    MyDisplay, SnowGC[i], snowPix[i].pixmap);
      XSetFillStyle(  MyDisplay, SnowGC[i], FillStippled);

      XSetFunction(   MyDisplay, eSnowGC[i], GXcopy);
      XSetStipple(    MyDisplay, eSnowGC[i], snowPix[i].pixmap);
      XSetForeground( MyDisplay, eSnowGC[i], erasePixel);
      XSetFillStyle(  MyDisplay, eSnowGC[i], FillStippled);

      //FlakeSurface[i] = igdk_cairo_surface_create_from_xpm();

   }

   for (i=0; i<STARANIMATIONS; i++)
   {
      XSetFunction(   MyDisplay,starGC[i],GXcopy);
      XSetStipple(    MyDisplay,starGC[i],starPix.pixmap);
      XSetForeground( MyDisplay,starGC[i],starcPix[i]);
      XSetFillStyle(  MyDisplay,starGC[i],FillStippled);
   }

   XSetFunction(MyDisplay,CleanGC, GXcopy);
   XSetForeground(MyDisplay,CleanGC,blackPix);

   XSetLineAttributes(MyDisplay, FallenGC, 1, LineSolid,CapRound,JoinMiter);

   XSetFillStyle( MyDisplay, eFallenGC, FillSolid);
   XSetFunction(  MyDisplay, eFallenGC, GXcopy);
   XSetForeground(MyDisplay, eFallenGC, erasePixel);

   XSetLineAttributes(MyDisplay, meteorite.gc,  1,LineSolid,CapRound,JoinMiter);
   XSetLineAttributes(MyDisplay, meteorite.egc, 1,LineSolid,CapRound,JoinMiter);
   if(Usealpha)
   {
      XSetFunction(MyDisplay,   meteorite.gc,  GXcopy);
      XSetForeground(MyDisplay, meteorite.gc,  meteoPix);
      XSetFunction(MyDisplay,   meteorite.egc, GXcopy);
      XSetForeground(MyDisplay, meteorite.egc, erasePixel);
   }
   else
   {
      XSetFunction(MyDisplay,   meteorite.gc,  GXxor);
      XSetForeground(MyDisplay, meteorite.gc,  meteoPix);
      XSetFunction(MyDisplay,   meteorite.egc, GXxor);
      XSetForeground(MyDisplay, meteorite.egc, meteoPix);
   }
#endif
}

void SetWhirl()
{
   Whirl = 0.01*flags.WhirlFactor*WHIRL;
}
void SetWindTimer()
{
   WindTimer                      = flags.WindTimer;
   if (WindTimer < 3) WindTimer   = 3;
   wind_timer                     = WindTimer;
}
void KdeSetBG1(const char *color)
{
   kdesetbg(color);
   activate_clean = 1;
}

int DetermineWindow()
{
   P("DetermineWindow\n");
   if (flags.window_id)
   {
      SnowWin = flags.window_id;
      Isdesktop = 0;
      Usealpha  = 0;
   }
   else
   {
      if (flags.xwininfohandling)
      {
	 SnowWin = XWinInfo(&SnowWinName);
	 if (SnowWin == 0)
	 {
	    fprintf(stderr,"XWinInfo failed\n");
	    exit(1);
	 }
	 Isdesktop = 0;
	 Usealpha  = 0;
      }
      else
      {
	 // if envvar DESKTOP_SESSION == LXDE, search for window with name pcmanfm
	 char *desktopsession = getenv("DESKTOP_SESSION");
	 if (!desktopsession)
	    desktopsession = getenv("XDG_SESSION_DESKTOP");
	 if (!desktopsession)
	    desktopsession = getenv("XDG_CURRENT_DESKTOP");
	 if (!desktopsession)
	    desktopsession = getenv("GDMSESSION");
	 if (desktopsession)
	    printf("Detected desktop session: %s\n",desktopsession);
	 else
	    printf("Could not determine desktop session\n");
	 int lxdefound = 0;
	 if (desktopsession != NULL && !strncmp(desktopsession,"LXDE",4))
	 {
	    lxdefound = FindWindowWithName("pcmanfm",&SnowWin,&SnowWinName);
	    printf("LXDE session found, searching for window 'pcmanfm'\n");
	 }
	 if(lxdefound)
	 {
	    Usealpha  = 0;
	    Isdesktop = 1;
	    exposures = 0;
	 }
	 else
	 {

	    P("DetermineWindow\n");
	    int x,y;
	    unsigned int w,h,b,depth;
	    Window root;
	    XGetGeometry(MyDisplay,rootwindow,&root,
		  &x, &y, &w, &h, &b, &depth);
	    if(SnowWinName) free(SnowWinName);
	    gtkwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	    gtk_window_set_position(GTK_WINDOW(gtkwin),GTK_WIN_POS_CENTER);
	    SnowWinName = strdup("Xsnow-Window");
	    gtk_window_set_title(GTK_WINDOW(gtkwin), SnowWinName);

#ifndef USEX11
	    darea   = gtk_drawing_area_new();
	    g_signal_connect(G_OBJECT(darea),"draw",G_CALLBACK(draw_cb),NULL);

	    gtk_container_add(GTK_CONTAINER(gtkwin),darea);
	    g_signal_connect(darea,"configure-event",
		  G_CALLBACK(configure_event_cb),NULL);
#endif

	    if (flags.fullscreen)
	       gtk_window_fullscreen(GTK_WINDOW(gtkwin));
	    if(flags.below)
	       gtk_window_set_keep_below       (GTK_WINDOW(gtkwin), TRUE);
	    else
	       gtk_window_set_keep_above       (GTK_WINDOW(gtkwin), TRUE);
	    make_transparent(gtkwin);
	    gtk_widget_show_all(gtkwin);
	    gdkwin  = gtk_widget_get_window(gtkwin);
	    SnowWin = gdk_x11_window_get_xid(gdkwin);

	    Isdesktop = 1;
	    Usealpha  = 1;
	    XGetGeometry(MyDisplay,SnowWin,&root,
		  &x, &y, &w, &h, &b, &depth);
	    P("depth: %d snowwin: 0x%lx %s\n",depth,SnowWin,SnowWinName);
	    if(SnowWin)
	    {
	       transworkspace = GetCurrentWorkspace();
	       usingtrans     = 1;
	    }
	    else
	    {
	       // snow in root window:
	       P("snow in root window\n");
	       Isdesktop = 1;
	       Usealpha  = 0;
	       if(SnowWinName) free(SnowWinName);
	       SnowWin     = DefaultRootWindow(MyDisplay);
	       SnowWinName = strdup("No Name");
	    }
	 }
      }
   }
   // override Isdesktop if user desires so:
   if (flags.desktop)
      Isdesktop = 1;
   // P("Isdesktop: %d\n",Isdesktop);
   if(Isdesktop) cworkspace = GetCurrentWorkspace();
   if (cworkspace < 0)
      return 0;
   InitDisplayDimensions();
   // if depth != 32, we assume that the desktop is not gnome-like TODO
   if (Isdesktop && SnowWinDepth != 32)
      Usealpha = 0;
   // override Usealpha if user desires so:
   if (flags.usealpha != SOMENUMBER)
      Usealpha = flags.usealpha;

   flags.usealpha = Usealpha;   // we could run into trouble with this, let's see...
   if(flags.KDEbg)
      KdeSetBG1(flags.bgcolor);
   return 1;
}

