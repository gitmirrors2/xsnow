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
#include <ctype.h>
#include <gtk/gtk.h>
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
int     SnowWinBorderWidth;
int     SnowWinDepth;
int     SnowWinHeight;
int     SnowWinWidth; 
int     SnowWinX; 
int     SnowWinY; 
char   *DesktopSession = 0;
int     IsCompiz;

// locals
// snow flakes stuff
static float BlowOffFactor;
static int   DoNotMakeSnow = 0;
static Snow  *FirstFlake = 0;
static int   FlakeCount = 0;
static float FlakesPerSecond;
static int   MaxSnowFlakeHeight = 0;  /* Biggest flake */
static int   MaxSnowFlakeWidth = 0;   /* Biggest flake */
static float SnowSpeedFactor;
static int   SnowRunning = 0;

// fallen snow stuff
static FallenSnow *FsnowFirst = 0;
static int        MaxScrSnowDepth = 0;
static int        OnTrees = 0;

// miscellaneous
char       Copyright[] = "\nXsnow\nCopyright 1984,1988,1990,1993-1995,2000-2001 by Rick Jansen, all rights reserved, 2019 also by Willem Vermin\n";
static int      ActivateClean = 0;  // trigger for do_clean
static int      Argc;
static char**   Argv;

// tree stuff
static int      NTrees;                       // actual number of trees
static int      NtreeTypes = 0;
static Treeinfo *Tree = 0;
static Pixmap   TreeMaskPixmap[MAXTREETYPE+1][2];
static Pixmap   TreePixmap[MAXTREETYPE+1][2];
static int      *TreeType;
static int      TreeRead = 0;
static int      TreeWidth[MAXTREETYPE+1], TreeHeight[MAXTREETYPE+1];
static char     **TreeXpm = 0;

// Santa stuff
static float  ActualSantaSpeed;
static int    CurrentSanta;
static int    OldSantaX=0;  // the x value of Santa when he was last drawn
static int    OldSantaY=0;  // the y value of Santa when he was last drawn
static int    SantaHeight;   
static Pixmap SantaMaskPixmap[PIXINANIMATION] = {0,0,0,0};
static Pixmap SantaPixmap[PIXINANIMATION] = {0,0,0,0};
static float  SantaSpeed;  
static int    SantaWidth;
static float  SantaXr;
static int    SantaX;   // should always be lrintf(SantaXr)
static int    SantaY;
static int    SantaYStep;

/* Speed for each Santa  in pixels/second*/
static float Speed[] = {SANTASPEED0,  /* Santa 0 */
   SANTASPEED1,  /* Santa 1 */
   SANTASPEED2,  /* Santa 2 */
   SANTASPEED3,  /* Santa 3 */
   SANTASPEED4,  /* Santa 4 */
};

// star stuff
static Skoordinaten *star = 0;
static int NStars;

// meteorites stuff
static MeteoMap meteorite;

// timing stuff
static unsigned int RunCounter = 0;
static double       *Prevtime = 0;
static double       TotSleepTime = 0;
static double       TNow;
static double       TStart;

// define unique numbers for alarms:
#define ALARM(x,y) alarm_ ## x,
enum{
   ALARMALL
      LastAlarm
};
#undef ALARM

// windows stuff
static int          NWindows;
static long         CWorkSpace = 0;
static Window       RootWindow;
static char         *SnowWinName = 0;
static WinInfo      *Windows = 0;
static int          Exposures;
static long         TransWorkSpace = -1;  // workspace on which transparent window is placed
static int          UsingTrans     = 0;   // using transparent window or not
static int          Xroot;
static int          Yroot;
static unsigned int Wroot;
static unsigned int Hroot;
static int          DoRestart = 0;

/* Wind stuff */
// Wind = 0: no wind
// Wind = 1: wind only affecting snow
// Wind = 2: wind affecting snow and santa
// Direction =  0: no wind direction I guess
// Direction =  1: wind from left to right
// Direction = -1: wind from right to left
static int    Direction = 0;
static double FlakesDT;
static float  NewWind = 0;
static float  Whirl;
static int    Wind = 0;
static double WindTimer;
static double WindTimerStart;

// desktop stuff
static int       Isdesktop;
static int       UseAlpha;
static XPoint    *SnowOnTrees;
static GtkWidget *GtkWin = NULL;

/* Colo(u)rs */
static char *BlackColor = "black";
static char *MeteoColor = "orange";
static char *StarColor[]  = { "gold", "gold1", "gold4", "orange" };
static Pixel BlackPix;
static Pixel ErasePixel;
static Pixel MeteoPix;
static Pixel SnowcPix;
static Pixel StarcPix[STARANIMATIONS];
static Pixel TreePix;
static Pixel Black, White;

/* GC's */
static GC CleanGC;
static GC EFallenGC;
static GC ESantaGC;
static GC ESnowGC[SNOWFLAKEMAXTYPE+1];  // There are SNOWFLAKEMAXTYPE+1 flakes
static GC FallenGC;
static GC SantaGC;
static GC SnowGC[SNOWFLAKEMAXTYPE+1];  // There are SNOWFLAKEMAXTYPE+1 flakes
static GC SnowOnTreesGC;
static GC StarGC[STARANIMATIONS];
static GC TestingGC;
static GC TreeGC;
//static GC TreesGC[2];

// region stuff
static Region NoSnowArea_dynamic;
static Region NoSnowArea_static;
static Region TreeRegion;
static Region SantaRegion;
static Region SantaPlowRegion;
static Region SnowOnTreesRegion;

/* Forward decls */
// declare actions for alarms:
#define ALARM(x,y) static void do_ ## x(void);
ALARMALL
#undef ALARM
double Delay[LastAlarm];
static Pixel  AllocNamedColor(char *colorName, Pixel dfltPix);
static int    BlowOff(void);
static void   CleanFallenArea(FallenSnow *fsnow, int x, int w);
static void   CleanFallen(Window id);
static void   ClearScreen(void);
static void   ConvertOnTreeToFlakes(void);
static void   CreateAlarmDelays(void);
static Pixmap CreatePixmapFromFallen(struct FallenSnow *f);
static void   DeleteFlake(Snow *flake);
static int    DetermineWindow();
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
static void   HandleExposures(void);
static FILE   *HomeOpen(char *file,char *mode,char **path);
static Pixel  IAllocNamedColor(char *colorName, Pixel dfltPix);
static void   InitBaumKoordinaten(void);
static void   InitBlowOffFactor(void);
static void   InitDisplayDimensions(void);
static void   InitFallenSnow(void);
static void   InitFlakesPerSecond(void);
static void   ReInitTree0(void);
static void   RestartDisplay(void);
static void   InitFlake(Snow *flake);
static void   InitSantaPixmaps(void);
static void   InitSnowOnTrees(void);
static void   InitSnowSpeedFactor(void);
static void   InitSnowColor(void);
static void   InitStars(void);
static void   InitTreePixmaps(void);
static void   KDESetBG1(const char *color);
static void   MicroSleep(long usec);
static int    RandInt(int maxVal);
static void   RedrawTrees(void);
static Region RegionCreateRectangle(int x, int y, int w, int h);
static void   ResetSanta(void);
static void   SetGCFunctions(void);
static void   SetMaxScreenSnowDepth(void);
static void   SetSantaSpeed(void);
static void   SetWhirl(void);
static void   SetWindTimer(void);
static void   SigHandler(int signum);
static void   UpdateFallenSnowPartial(FallenSnow *fsnow, int x, int w);
static void   UpdateFallenSnowWithWind(FallenSnow *fsnow,int w, int h);
static void   UpdateSanta(void);
static void   UpdateSnowFlake(Snow *flake);
static void   UpdateWindows(void);
static int    XsnowErrors(Display *dpy, XErrorEvent *err);
static Window XWinInfo(char **name);


static void Thanks(void)
{
   printf("\nThank you for using xsnow\n");
}

int main(int argc, char *argv[])
{
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
   }
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

   TreeType = malloc(sizeof(*TreeType)); // to make realloc() possible in InitBaumKoordinaten

   InitSnowSpeedFactor();
   SetWhirl();
   SetWindTimer();

   SnowOnTrees = malloc(sizeof(*SnowOnTrees));  // will be remallloced in InitSnowOnTrees
   InitSnowOnTrees();
   InitBlowOffFactor();

   SnowOnTrees = malloc(sizeof(*SnowOnTrees)*Flags.MaxOnTrees);
   star = malloc(sizeof(*star)); // will be re-allocated in InitStars
   Tree = malloc(sizeof(*Tree)); // will be re-allocated in InitBaumKoordinaten
   srand48((unsigned int)wallclock());
   SnowMap *rp;
   //signal(SIGKILL, SigHandler);  // wwvv
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

   NoSnowArea_dynamic   = XCreateRegion();
   TreeRegion           = XCreateRegion();
   SantaRegion          = XCreateRegion();
   SantaPlowRegion      = XCreateRegion();
   SnowOnTreesRegion = XCreateRegion();
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
	 (char*)starPix.starBits, starPix.width, starPix.height);
   FirstFlake = createFlake(); FlakeCount++;
   InitFlake(FirstFlake);
   InitFlakesPerSecond();
   InitSantaPixmaps();
   InitFallenSnow();
   InitStars();
   InitTreePixmaps();  // can change value of Flags.NoMenu

#define DOIT_I(x) OldFlags.x = Flags.x;
#define DOIT_L(x) DOIT_I(x);
#define DOIT_S(x) OldFlags.x = strdup(Flags.x);
   DOITALL;
#undef DOIT_I
#undef DOIT_L
#undef DOIT_S


   if(!Flags.NoMenu)
   {
      ui(&argc, &argv);
   }

   NoSnowArea_static = TreeRegion;
   BlackPix = AllocNamedColor(BlackColor, Black);
   //SnowcPix = IAllocNamedColor(Flags.SnowColor, White);   
   MeteoPix = IAllocNamedColor(MeteoColor, White);
   TreePix    = IAllocNamedColor(Flags.TreeColor,   Black);
   for(i=0; i<STARANIMATIONS; i++)
      StarcPix[i] = IAllocNamedColor(StarColor[i], Black);

   SantaGC       = XCreateGC(display, SnowWin, 0, 0);
   TestingGC     = XCreateGC(display, RootWindow, 0,0);
   ESantaGC      = XCreateGC(display, SnowWin, 0, 0);
   TreeGC        = XCreateGC(display, SnowWin, 0, 0);
   SnowOnTreesGC = XCreateGC(display, SnowWin, 0, 0);
   CleanGC       = XCreateGC(display,SnowWin,0,0);
   FallenGC      = XCreateGC(display, SnowWin, 0, 0);
   EFallenGC     = XCreateGC(display, SnowWin, 0, 0);  // used to erase fallen snow
   meteorite.gc  = XCreateGC(display, SnowWin, 0, 0);
   meteorite.egc = XCreateGC(display, SnowWin, 0, 0);
   for (i=0; i<=SNOWFLAKEMAXTYPE; i++) 
   {
      SnowGC[i]  = XCreateGC(display, SnowWin, 0, 0);
      ESnowGC[i] = XCreateGC(display, SnowWin, 0, 0);
   }
   for (i=0; i<STARANIMATIONS; i++)
      StarGC[i]  = XCreateGC(display,SnowWin,0,0);

   SetGCFunctions();

   InitBaumKoordinaten();
   InitSnowColor();

   ResetSanta();   
   // events
   if(Isdesktop)
      XSelectInput(display, SnowWin, 0);
   else
      XSelectInput(display, SnowWin, 
	    StructureNotifyMask);
   //	    ExposureMask|SubstructureNotifyMask|StructureNotifyMask);

   double Alarm[LastAlarm];
   Prevtime = malloc(sizeof(*Prevtime)*LastAlarm);
   unsigned int counter[LastAlarm];
   for(i=0; i<LastAlarm; i++) counter[i] = 0;
   char *names[LastAlarm];

   // define names for alarms:
#define ALARM(x,y) names[alarm_ ## x] = #x;
   ALARMALL;
#undef ALARM

   CreateAlarmDelays();
   //
   // about alarm_santa1: if Exposures == True, Santa has to
   // be redrawn in a high frequency because there seems
   // to be no way to determine when XClearArea(...,True)
   // is really finished. If alarm_santa1 is set to
   // for example 0.05, and Exposures = True, changes
   // are that Santa will not be visible.

   { int i; for(i=0; i<LastAlarm; i++) Alarm[i] = wallclock();}

   TStart = wallclock();
   TNow = wallclock();
   for(i=0; i<LastAlarm; i++) Prevtime[i] = wallclock();
   Flags.Done = 0;
   ClearScreen();   // without this, no snow, scenery etc. in KDE
   // main loop
   while (!Flags.Done)
   {
      if(RunCounter%10 == 0)
      {
	 // check if snow window still exists:
	 XWindowAttributes wattr;
	 if (!XGetWindowAttributes(display,SnowWin,&wattr))
	    break;
      }
      int i,action;
      RunCounter++;
      action = 0;
      for (i=1; i<LastAlarm; i++)
	 if (Alarm[i] < Alarm[action])
	    action = i;
      double waittime = Alarm[action] - TNow;
      while (SnowRunning)
      {
	 do_snowflakes();
	 waittime = Alarm[action] - wallclock();
	 if (waittime <=0)
	 {
	    waittime = 0;
	    break;
	 }
      }
      MicroSleep((long)(1e6*(Alarm[action] - TNow)));
      TNow = wallclock();
      // define actions for alarms:
#define ALARM(x,y) case alarm_ ## x: do_ ## x(); break;
      switch(action)
      {
	 ALARMALL;
      }
#undef ALARM
      Alarm[action]    = TNow + Delay[action]; // set alarm for this action
      Prevtime[action] = TNow;                 // remember time of last action  
      counter[action] ++;
      if (Flags.StopAfter > 0 && TNow - TStart > Flags.StopAfter) Flags.Done = 1;
   }


   if(TreeXpm) XpmFree(TreeXpm);
   while (FirstFlake->next)
   {
      FirstFlake = delFlake(FirstFlake); FlakeCount--;
   }

   free(FirstFlake);

   if (SnowWinName) free(SnowWinName);
   if (Prevtime) free(Prevtime);

   XClearArea(display, SnowWin, 0,0,0,0,True);
   XFlush(display);
   XCloseDisplay(display); //also frees the GC's, pixmaps and other resources on display
   while(FsnowFirst)
      PopFallenSnow(&FsnowFirst);
   double telapsed = wallclock() - TStart;

   if(Flags.ShowStats)
   {
      printf("\nElapsed: %8.2f seconds\n",telapsed);
      printf("Sleep:   %8.2f seconds = %6.2f%%\n",
	    TotSleepTime,100.0*TotSleepTime/telapsed);
      printf("Active:  %8.2f seconds = %6.2f%%\n",
	    telapsed - TotSleepTime,100.0*(telapsed-TotSleepTime)/telapsed);
      printf("                   wakeups   freq    delay   target\n");
      for (i=0; i<LastAlarm; i++)
      {
	 double delaytime,frequency;
	 if (telapsed  == 0.0) frequency = 0.0; else frequency = (double)counter[i]/telapsed;
	 if (frequency == 0.0) delaytime = 0.0; else delaytime = 1.0/frequency; 
	 printf("%-15s %10d %6.2f %8.4f %8.4f",names[i],counter[i], frequency, delaytime,Delay[i]);
	 if (delaytime > 1.1*Delay[i]) printf("  *\n"); else printf("\n");
      }
   }

   if(star) free(star);
   if(Tree) free(Tree);
   if (DoRestart)
   {
      sleep(2);
      extern char** environ;
      execve(Argv[0],Argv,environ);
   }
   Thanks();
   return 0;
}		/* End of snowing */
/* ------------------------------------------------------------------ */ 
/*
 * do nothing if current workspace is not to be updated
 */
#define NOTACTIVE \
   (!Flags.AllWorkspaces && (UsingTrans && CWorkSpace != TransWorkSpace))

void do_santa()
{
   if (NOTACTIVE)
      return;
   if (!Flags.NoSanta)
      DrawSanta();
}
void do_santa1()
{
   if (NOTACTIVE)
      return;
   if (!Flags.NoSanta)
      DrawSanta1();
}

void do_ui_loop()
{
   ui_loop();
}

void do_ui_check()
{
   if (Flags.NoMenu)
      return;
   int changes = 0;
   if (Flags.SantaSize != OldFlags.SantaSize || 
	 Flags.NoRudolf != OldFlags.NoRudolf)
   {
      EraseSanta(OldSantaX,OldSantaY);
      InitSantaPixmaps();
      OldFlags.SantaSize = Flags.SantaSize;
      OldFlags.NoRudolf = Flags.NoRudolf;
      changes++;
   }
   if (Flags.NoSanta != OldFlags.NoSanta)
   {
      //P("do_ui_check\n");
      if (Flags.NoSanta)
	 EraseSanta(OldSantaX, OldSantaY);
      OldFlags.NoSanta = Flags.NoSanta;
      changes++;
   }
   if(Flags.SantaSpeedFactor != OldFlags.SantaSpeedFactor)
   {
      SetSantaSpeed();
      OldFlags.SantaSpeedFactor = Flags.SantaSpeedFactor;
      changes++;
   }
   if(strcmp(Flags.TreeType, OldFlags.TreeType))
   {
      //P(%s %s\n",Flags.TreeType,OldFlags.TreeType);
      RedrawTrees();
      free(OldFlags.TreeType);
      OldFlags.TreeType = strdup(Flags.TreeType);
      changes++;
   }
   if(Flags.DesiredNumberOfTrees != OldFlags.DesiredNumberOfTrees)
   {
      RedrawTrees();
      OldFlags.DesiredNumberOfTrees = Flags.DesiredNumberOfTrees;
      changes++;
   }
   if(Flags.TreeFill != OldFlags.TreeFill)
   {
      RedrawTrees();
      OldFlags.TreeFill = Flags.TreeFill;
      changes++;
   }
   if(Flags.NoTrees != OldFlags.NoTrees)
   {
      RedrawTrees();
      OldFlags.NoTrees = Flags.NoTrees;
      changes++;
   }
   if(strcmp(Flags.TreeColor, OldFlags.TreeColor))
   {
      //P("%s %s\n",Flags.TreeColor,OldFlags.TreeColor);
      ReInitTree0();
      //RedrawTrees();
      free(OldFlags.TreeColor);
      OldFlags.TreeColor = strdup(Flags.TreeColor);
      changes++;
   }
   if(Flags.NStars != OldFlags.NStars)
   {
      EraseStars();
      InitStars();
      OldFlags.NStars = Flags.NStars;
      changes++;
   }
   if(Flags.NoMeteorites != OldFlags.NoMeteorites)
   {
      OldFlags.NoMeteorites = Flags.NoMeteorites;
      changes++;
   }
   if(Flags.NoSnowFlakes != OldFlags.NoSnowFlakes)
   {
      OldFlags.NoSnowFlakes = Flags.NoSnowFlakes;
      if(Flags.NoSnowFlakes)
	 ClearScreen();
      changes++;
   }
   if(Flags.SnowFlakesFactor != OldFlags.SnowFlakesFactor)
   {
      OldFlags.SnowFlakesFactor = Flags.SnowFlakesFactor;
      InitFlakesPerSecond();
      changes++;
   }
   if(strcmp(Flags.SnowColor, OldFlags.SnowColor))
   {
      //P("%s %s\n",Flags.SnowColor,OldFlags.SnowColor);
      InitSnowColor();
      ClearScreen();
      free(OldFlags.SnowColor);
      OldFlags.SnowColor = strdup(Flags.SnowColor);
      changes++;
   }
   if(Flags.SnowSpeedFactor != OldFlags.SnowSpeedFactor)
   {
      OldFlags.SnowSpeedFactor = Flags.SnowSpeedFactor;
      InitSnowSpeedFactor();
      changes++;
   }
   if(Flags.BlowOffFactor != OldFlags.BlowOffFactor)
   {
      OldFlags.BlowOffFactor = Flags.BlowOffFactor;
      InitBlowOffFactor();
      changes++;
   }
   if(Flags.NoBlowSnow != OldFlags.NoBlowSnow)
   {
      OldFlags.NoBlowSnow = Flags.NoBlowSnow;
      changes++;
   }
   if(Flags.CpuLoad != OldFlags.CpuLoad)
   {
      OldFlags.CpuLoad = Flags.CpuLoad;
      CreateAlarmDelays();
      changes++;
   }
   if(Flags.UseBG != OldFlags.UseBG)
   {
      OldFlags.UseBG = Flags.UseBG;
      SetGCFunctions();
      ClearScreen();
      changes++;
   }
   if(Flags.KDEbg != OldFlags.KDEbg)
   {
      OldFlags.KDEbg = Flags.KDEbg;
      if (Flags.KDEbg)
	 KDESetBG1(Flags.BGColor);
      else
	 KDESetBG1(0);
      ClearScreen();
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
   }
   if(Flags.UseAlpha != OldFlags.UseAlpha)
   {
      OldFlags.UseAlpha = Flags.UseAlpha;
      UseAlpha          = Flags.UseAlpha;
      SetGCFunctions();
      ClearScreen();
      changes++;
   }
   if(Flags.Exposures != OldFlags.Exposures)
   {
      OldFlags.Exposures = Flags.Exposures;
      HandleExposures();
      CreateAlarmDelays();
      ClearScreen();
      changes++;
   }
   if(Flags.OffsetS != OldFlags.OffsetS)
   {
      OldFlags.OffsetS = Flags.OffsetS;
      InitDisplayDimensions();
      InitFallenSnow();
      InitStars();
      RedrawTrees();
      ClearScreen();
      changes++;
   }
   if(Flags.MaxWinSnowDepth != OldFlags.MaxWinSnowDepth)
   {
      OldFlags.MaxWinSnowDepth = Flags.MaxWinSnowDepth;
      InitFallenSnow();
      ClearScreen();
      changes++;
   }
   if(Flags.MaxScrSnowDepth != OldFlags.MaxScrSnowDepth)
   {
      OldFlags.MaxScrSnowDepth = Flags.MaxScrSnowDepth;
      SetMaxScreenSnowDepth();
      InitFallenSnow();
      ClearScreen();
      changes++;
   }
   if(Flags.MaxOnTrees != OldFlags.MaxOnTrees)
   {
      OldFlags.MaxOnTrees = Flags.MaxOnTrees;
      ClearScreen();
      changes++;
   }
   if(Flags.NoFluffy != OldFlags.NoFluffy)
   {
      OldFlags.NoFluffy = Flags.NoFluffy;
      ClearScreen();
      changes++;
   }
   if(Flags.NoKeepSnowOnTrees != OldFlags.NoKeepSnowOnTrees)
   {
      OldFlags.NoKeepSnowOnTrees = Flags.NoKeepSnowOnTrees;
      ClearScreen();
      changes++;
   }
   if(Flags.NoKeepSBot != OldFlags.NoKeepSBot)
   {
      OldFlags.NoKeepSBot = Flags.NoKeepSBot;
      InitFallenSnow();
      ClearScreen();
      changes++;
   }
   if(Flags.NoKeepSWin != OldFlags.NoKeepSWin)
   {
      OldFlags.NoKeepSWin = Flags.NoKeepSWin;
      InitFallenSnow();
      ClearScreen();
      changes++;
   }
   if(Flags.NoWind != OldFlags.NoWind)
   {
      OldFlags.NoWind = Flags.NoWind;
      changes++;
   }
   if(Flags.WhirlFactor != OldFlags.WhirlFactor)
   {
      OldFlags.WhirlFactor = Flags.WhirlFactor;
      SetWhirl();
      changes++;
   }
   if(Flags.WindTimer != OldFlags.WindTimer)
   {
      OldFlags.WindTimer = Flags.WindTimer;
      SetWindTimer();
      changes++;
   }
   if(Flags.FullScreen != OldFlags.FullScreen)
   {
      OldFlags.FullScreen = Flags.FullScreen;
      DetermineWindow();
      InitFallenSnow();
      InitStars();
      RedrawTrees();
      ClearScreen();
      changes++;
   }
   if(Flags.AllWorkspaces != OldFlags.AllWorkspaces)
   {
      OldFlags.AllWorkspaces = Flags.AllWorkspaces;
      DetermineWindow();
      changes++;
   }
   if(Flags.BelowAll != OldFlags.BelowAll)
   {
      OldFlags.BelowAll = Flags.BelowAll;
      DetermineWindow();
      changes++;
   }
   if(Flags.WindNow)
   {
      Flags.WindNow = 0;
      Wind = 2;
   }

   if (changes > 0)
   {
      WriteFlags();
   }
}

void ClearScreen()
{
   XClearArea(display, SnowWin, 0,0,0,0,True);
}
void RedrawTrees()
{
   EraseTrees();
   InitBaumKoordinaten();
   NoSnowArea_static = TreeRegion;
}

void do_tree()
{
   if (NOTACTIVE)
      return;
   if(!Flags.NoTrees)
   {
      int i;
      for (i=0; i<NTrees; i++)
	 DrawTree(i);
   }
}

void do_snow_on_trees()
{
   if (NOTACTIVE)
      return;
   if(Flags.NoKeepSnowOnTrees || Flags.NoTrees)
      return;
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
}

void do_snowflakes()
{
   if (NOTACTIVE)
   {
      SnowRunning = 0;
      return;
   }
   static Snow *flake;
   static double prevtime = 0;
   if (!SnowRunning)
   {
      flake = FirstFlake;
      prevtime = Prevtime[alarm_snowflakes];
   }
   FlakesDT = wallclock() - prevtime;
   // after suspend or sleep FlakesDT could have a strange
   // value. return if so.
   if (FlakesDT < 0 || FlakesDT > 10*Delay[alarm_snowflakes])
   {
      SnowRunning = 0;
      return;
   }
   P("do_snow_flakes %f\n",FlakesDT);
   int count = 0;

   SnowRunning = 1;
   while(flake && count++ < SNOWCHUNK)
   {
      Snow *next = flake->next;  // flake can disappear, so we have to save the 
      //                            pointer to the next flake
      UpdateSnowFlake(flake);
      flake = next;
   }
   if(!flake)
   {
      SnowRunning = 0;

      if(!Flags.NoKeepSnowOnTrees && !Flags.NoTrees)
      {
	 XSubtractRegion(SnowOnTreesRegion,TreeRegion,SnowOnTreesRegion);
      }
   }

#if 0
   {
      int test = countSnow(FirstFlake,0);
      P("%d %d %d %d %p %p\n",FlakeCount,test,SnowRunning,RunCounter,(void*)FirstFlake->prev,(void*)FirstFlake->next);
      assert(FlakeCount == test);
   }
#endif

}

int HandleFallenSnow(FallenSnow *fsnow)
{
   // soo complicated to determine if a fallensnow should be handled, therefore
   // we isolate this question in this function for further use
   return !((fsnow->id == 0 && Flags.NoKeepSBot)||(fsnow->id != 0 && Flags.NoKeepSWin)); 
}

void do_fallen()
{
   if (NOTACTIVE)
      return;

   FallenSnow *fsnow = FsnowFirst;
   //P("%d\n",RunCounter);
   //PrintFallenSnow(FsnowFirst);
   while(fsnow)
   {
      if (HandleFallenSnow(fsnow)) 
	 DrawFallen(fsnow);
      fsnow = fsnow->next;
   }
   XFlush(display);
}

void do_blowoff()
{
   if (NOTACTIVE)
      return;
   FallenSnow *fsnow = FsnowFirst;
   while(fsnow)
   {
      if (HandleFallenSnow(fsnow)) 
	 if(fsnow->id == 0 || (!fsnow->hidden &&
		  (fsnow->ws == CWorkSpace || fsnow->sticky)))
	    UpdateFallenSnowWithWind(fsnow,fsnow->w/4,fsnow->h/4); 
      fsnow = fsnow->next;
   }
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
   //R("fsnow->clean: %d\n",fsnow->clean);
}

void GenerateFlakesFromFallen(FallenSnow *fsnow, int x, int w, float vy)
{
   if (SnowRunning || Flags.NoBlowSnow || Flags.NoWind || 
	 FlakeCount > Flags.FlakeCountMax || DoNotMakeSnow)
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
	    FirstFlake = addFlake(FirstFlake); FlakeCount++;
	    InitFlake(FirstFlake);
	    FirstFlake->rx     = fsnow->x + i;
	    FirstFlake->ry     = fsnow->y - j;
	    if (Flags.NoWind)
	       FirstFlake->vx     = 0;
	    else
	       FirstFlake->vx     = NewWind/8;
	    FirstFlake->vy     = vy;
	    FirstFlake->cyclic = 0;
	 }
	 //P("%f %f\n",FirstFlake->rx, FirstFlake->ry);
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

// smooth fallen snow
void do_sfallen()
{
   if (NOTACTIVE)
      return;
   return; // taken care of in UpdateFallenSnowPartial()
   FallenSnow *fsnow = FsnowFirst;
   while(fsnow)
   {
      if(!fsnow->clean)
	 if(fsnow->id == 0 || ((!fsnow->hidden) &&
		  (fsnow->ws == CWorkSpace || fsnow->sticky)))
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
   if (NOTACTIVE)
      return;
   UpdateSanta(); 
}

void do_displaychanged()
{
   // if we are snowing in the desktop, we check if the size has changed,
   // this can happen after changing of the displays settings
   // If the size has been changed, we restart the program
   if (!Isdesktop)
      return;
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
   }
#if 0
   Window       root;
   XGetGeometry(display,RootWindow,&root,
	 &x, &y, &w, &h, &b, &d);
   if(Xroot != x || Yroot != y || Wroot != w || Hroot != h)
   {
      // the rootwindow has changed, adapt SnowWin
      P("Calling RestartDisplay\n");
      sleep(1); // sleep is needed to let the displays settle
      //           without it, snowing is done in wrong places, especially 
      //           when switching to 'mirror'
      R("Calling execve:\n");
      DetermineWindow();
      RestartDisplay();
      P("new geometry: %d %d %d %d\n",SnowWinX,SnowWinY,SnowWinWidth,SnowWinHeight);
   }
#endif
}

void do_event()
{
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
		  //P("init %d %d\n",ev.xconfigure.width, ev.xconfigure.height);
		  /*
		     InitDisplayDimensions();
		     InitFallenSnow();
		     InitStars();
		     if(!Flags.NoKeepSnowOnTrees && !Flags.NoTrees)
		     {
		     XDestroyRegion(SnowOnTreesRegion);
		     SnowOnTreesRegion = XCreateRegion();
		     }
		     if(!Flags.NoTrees)
		     {
		     XDestroyRegion(TreeRegion);
		     TreeRegion = XCreateRegion();
		     InitBaumKoordinaten();
		     }
		     NoSnowArea_static = TreeRegion;
		     XClearArea(display, SnowWin, 0,0,0,0,Exposures);
		     */
		  P("Calling RestartDisplay\n");
		  RestartDisplay();
	       }
	       break;
	 } 
      }
   }  
}

void RestartDisplay()
{
   P("Calling InitDisplayDimensions\n");
   InitDisplayDimensions();
   InitFallenSnow();
   InitStars();
   if(!Flags.NoKeepSnowOnTrees && !Flags.NoTrees)
   {
      XDestroyRegion(SnowOnTreesRegion);
      SnowOnTreesRegion = XCreateRegion();
   }
   if(!Flags.NoTrees)
   {
      XDestroyRegion(TreeRegion);
      TreeRegion = XCreateRegion();
      InitBaumKoordinaten();
   }
   NoSnowArea_static = TreeRegion;
   XClearArea(display, SnowWin, 0,0,0,0,Exposures);

}

void do_genflakes()
{
   if (NOTACTIVE)
      return;
   if (DoNotMakeSnow)
      return;
   static double prevtime;
   static double sumdt = 0;
   static int    halted_by_snowrunning = 0;

   if(SnowRunning)
   {
      if (!halted_by_snowrunning)
      {
	 prevtime               = Prevtime[alarm_genflakes];
	 halted_by_snowrunning  = 1;
      }
      return;
   }

   double dt;
   if(halted_by_snowrunning)
      dt = TNow - prevtime;
   else
      dt = TNow - Prevtime[alarm_genflakes];
   halted_by_snowrunning = 0;

   // after suspend or sleep dt could have a strange value
   if (dt < 0 || dt > 10*Delay[alarm_genflakes])
      return;
   int desflakes = lrint((dt+sumdt)*FlakesPerSecond);
   P("desflakes: %d\n",desflakes);
   if(desflakes == 0)  // save dt for use next time: happens with low snowfall rate
      sumdt += dt; 
   else
      sumdt = 0;
   if (FlakeCount + desflakes > Flags.FlakeCountMax)
      return;
   int i;
   for(i=0; i<desflakes; i++)
   {
      FirstFlake = addFlake(FirstFlake); FlakeCount++;
      InitFlake(FirstFlake);
   }
}

void do_newwind()
{
   if (NOTACTIVE)
      return;
   //
   // the speed of newwind is pixels/second
   // at steady Wind, eventually all flakes get this speed.
   //
   if(Flags.NoWind) return;
   static double t0 = -1;
   if (t0<0)
   {
      t0 = wallclock();
      return;
   }

   //P("oldwind: %f %f %d\n",NewWind,Whirl,Wind);
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
	 //NewWind = Direction*300;
	 NewWind = Direction*0.6*Whirl;
	 break;
      case(2):
	 //NewWind = Direction*600;
	 NewWind = Direction*1.2*Whirl;
	 break;
   }
   //P(" newwind: %f %f\n",NewWind,r);
}


void do_wind()
{
   if (NOTACTIVE)
      return;
   if(Flags.NoWind) return;
   static int first = 1;
   static double prevtime;
   if (first)
   {
      prevtime = TNow;;
      first    = 0;
   }

   // on the average, this function will do something
   // after WindTimer secs

   if ((TNow - prevtime) < 2*WindTimer*drand48()) return;

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
}

// blow snow off trees
void ConvertOnTreeToFlakes()
{
   if(SnowRunning || Flags.NoKeepSnowOnTrees || Flags.NoBlowSnow || Flags.NoTrees ||
	 FlakeCount > Flags.FlakeCountMax || DoNotMakeSnow)
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
	    FirstFlake = addFlake(FirstFlake); FlakeCount++;
	    InitFlake(FirstFlake);
	    FirstFlake->rx = SnowOnTrees[i].x;
	    FirstFlake->ry = SnowOnTrees[i].y-5*j;
	    FirstFlake->vy = -10;
	    FirstFlake->cyclic = 0;
	 }
	 //P("%d %d %d\n",FlakeCount, (int)FirstFlake->rx,(int)FirstFlake->ry);
      }
      if(FlakeCount > Flags.FlakeCountMax)
	 break;
   }
   OnTrees = 0;
   XDestroyRegion(SnowOnTreesRegion);
   SnowOnTreesRegion = XCreateRegion();
}

void do_stars()
{
   if (NOTACTIVE)
      return;
   int i;
   for (i=0; i<NStars; i++)
   {
      int k = star[i].color;
      int x = star[i].x;
      int y = star[i].y;
      int w = starPix.width;
      int h = starPix.height;
      XSetTSOrigin(display, StarGC[k],x+w, y+h);
      XFillRectangle(display,SnowWin,StarGC[k],x,y,w,h);
   }
   XFlush(display);
}

void do_ustars()
{
   if (NOTACTIVE)
      return;
   int i;
   for (i=0; i<NStars; i++)
      if (drand48() > 0.7)
	 star[i].color = RandInt(STARANIMATIONS);
}

void do_meteorite()
{
   if (NOTACTIVE)
      return;
   if(Flags.NoMeteorites) return;
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
   XUnionRegion(meteorite.r,NoSnowArea_dynamic,NoSnowArea_dynamic);
   meteorite.starttime = wallclock();
   XDrawLine(display, SnowWin, meteorite.gc, 
	 meteorite.x1,meteorite.y1,meteorite.x2,meteorite.y2);
   XFlush(display);
}

void do_emeteorite()
{
   if (NOTACTIVE)
      return;
   if(Flags.NoMeteorites) return;
   if (meteorite.active)
      if (wallclock() - meteorite.starttime > 0.3)
      {
	 XDrawLine(display, SnowWin, meteorite.egc,  
	       meteorite.x1,meteorite.y1,meteorite.x2,meteorite.y2);
	 XSubtractRegion(NoSnowArea_dynamic ,meteorite.r,NoSnowArea_dynamic);
	 XDestroyRegion(meteorite.r);
	 meteorite.active = 0;
      }
   XFlush(display);
}

// used after kdesetbg: it appears that after kdesetbg 
// we have to wait a second or so and then clear the screen.
void do_clean()
{
   static int active = 0;
   static double TStart = 0.0;
   if (active)
   {
      if (wallclock() - TStart > 2.0)
      {
	 //P("do_clean ClearScreen\n");
	 ClearScreen();
	 active         = 0;
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
}

void do_wupdate()
{
   if(!Isdesktop) return;
   if(Flags.NoKeepSWin) return;
   long r;
   r = GetCurrentWorkspace();
   if(r>=0) 
      CWorkSpace = r;
   else
   {
      Flags.Done = 1;
      return;
   }

   if(Windows) free(Windows);

   if (GetWindows(&Windows, &NWindows)<0)
   {
      Flags.Done = 1;
      return;
   };
   // Take care of the situation that the transparent window changes from workspace, 
   // which can happen if in a dynamic number of workspaces environment
   // a workspace is emptied.
   WinInfo *winfo;
   winfo = FindWindow(Windows,NWindows,SnowWin);

   //P("SnowWin: 0x%lx\n",SnowWin);
   //P("current workspace: %d %d\n",CWorkSpace,RunCounter);
   //printwindows(Windows, NWindows);
   //P("ws:%d\n",winfo->ws);

   // check also on valid winfo: after toggling 'below'
   // winfo is nil sometimes

   if(UsingTrans && winfo)
   {
      //P("winfo: %p\n",(void*)winfo);
      // in xfce and maybe others, workspace info is not to be found
      // in our transparent window. winfo->ws will be 0, and we keep
      // the same value for TransWorkSpace.
      if (winfo->ws)
      {
	 TransWorkSpace = winfo->ws;
      }
   }

   UpdateWindows();
}

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
	    // add it, but not if we are snowing in this window (Desktop for example)
	    // and also not if this window has y == 0
	    //P("add %#lx %d\n",w->id, RunCounter);
	    //PrintFallenSnow(FsnowFirst);
	    if (w->id != SnowWin && w->y != 0)
	       PushFallenSnow(&FsnowFirst, w->id, w->ws, w->sticky,
		     w->x+Flags.OffsetX, w->y+Flags.OffsetY, w->w+Flags.OffsetW, 
		     Flags.MaxWinSnowDepth); 
	 }
      }
      w++;
   }
   // remove fallensnow regions
   f = FsnowFirst; int nf = 0; while(f) { nf++; f = f->next; }
   long int *toremove = malloc(sizeof(*toremove)*nf);
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
	    int i;
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
	 if (f->w == w->w+Flags.OffsetW) // width has not changed
	 {
	    //P("%#lx no change width %d %d %d %d %d %d\n",f->id,f->x,f->y,w->x,w->y,f->w,RunCounter);
	    if (f->x != w->x + Flags.OffsetX || f->y != w->y + Flags.OffsetY)
	    {
	       //R("window moved %d\n",RunCounter);
	       CleanFallenArea(f,0,f->w);
	       f->x = w->x + Flags.OffsetX;
	       f->y = w->y + Flags.OffsetY;
	       DrawFallen(f);
	       XFlush(display);
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
      RemoveFallenSnow(&FsnowFirst,toremove[i]);
   }
   free(toremove);
}

void do_testing()
{
   return;
   static int first = 1;
   if(first)
   {
      first = 0;
      return;
   }
   EraseTrees();
   InitBaumKoordinaten();
   NoSnowArea_static = TreeRegion;
   //P("%d:\n",NTrees);
   return;
   Region region;
   //region = SantaRegion;
   //region = NoSnowArea_static;
   region = SnowOnTreesRegion;
   //region = NoSnowArea_dynamic;
   //region = SantaPlowRegion;
   //region = TreeRegion;

   XSetFunction(display,   TestingGC, GXcopy);
   XSetForeground(display, TestingGC, BlackPix); 
   XFillRectangle(display, SnowWin, TestingGC, 0,0,SnowWinWidth,SnowWinHeight);
   XSetRegion(display,     TestingGC, region);
   XSetForeground(display, TestingGC, BlackPix); 
   XFillRectangle(display, SnowWin, TestingGC, 0,0,SnowWinWidth,SnowWinHeight);
}

void do_fuse()
{
   if (SnowRunning) // UpdateSnowFlake would be very confused when
      //               flakes suddenly disappear.
      return;
   if (FlakeCount >= Flags.FlakeCountMax)
   {
      if (!Flags.Quiet)
	 fprintf(stderr,"fuse blown: remove snowflakes\n");

      while(FirstFlake->next != 0)
      {
	 EraseSnowFlake(FirstFlake);
	 DeleteFlake(FirstFlake);
      }
      DoNotMakeSnow = 1;
   }
   else
      DoNotMakeSnow = 0;
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
   // return rand() % maxVal;   // Poor
   //return rand() / (RAND_MAX / maxVal + 1);
   // see http://c-faq.com/lib/randrange.html
   return drand48()*maxVal;
}
/* ------------------------------------------------------------------ */ 
void MicroSleep(long usec) 
{
   struct timespec t;
   if (usec <= 0) return;
   TotSleepTime += 1e-6*usec;
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
   if(Flags.NoWind)
      flake->vx     = 0; 
   else
      flake->vx     = drand48()*NewWind/2; 
   flake->ivy    = INITIALYSPEED * sqrt(flake->m);
   flake->vy     = flake->ivy;
   flake->wsens  = MAXWSENS*drand48();
   //P("%f %f\n",flake->rx, flake->ry);
}


void UpdateSnowFlake(Snow *flake)
{
   //
   // update speed in x Direction
   //

   if (!Flags.NoWind)
   {
      flake->vx += FlakesDT*flake->wsens*(NewWind - flake->vx)/flake->m;
      float speedxmaxes[] = {100.0, 300.0, 600.0,};
      float speedxmax = speedxmaxes[Wind];
      if(flake->vx > speedxmax) flake->vx = speedxmax;
      if(flake->vx < -speedxmax) flake->vx = -speedxmax;
   }

   //P("vy: %f\n",flake->vy);
   flake->vy += INITIALYSPEED * (drand48()-0.4)*0.1 ;
   if (flake->vy > flake->ivy*1.5) flake->vy = flake->ivy*1.5;

   //P("%f %f %f\n",flake->vx, NewWind, FlakesDT);
   float NewX = flake->rx + (flake->vx*FlakesDT)*SnowSpeedFactor;
   float NewY = flake->ry + (flake->vy*FlakesDT)*SnowSpeedFactor;
   if(flake->cyclic)
   {
      if (NewX < -flake->w)     NewX += SnowWinWidth-1;
      if (NewX >= SnowWinWidth) NewX -= SnowWinWidth;
   }
   else if (NewX < 0 || NewX >= SnowWinWidth)
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

   FallenSnow *fsnow = FsnowFirst;
   int found = 0;
   // investigate if flake is in a not-hidden fallensnowarea on current workspace
   while(fsnow && !found)
   {
      if(!fsnow->hidden)
	 if(fsnow->id == 0 ||(fsnow->ws == CWorkSpace || fsnow->sticky))
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
			if (Flags.NoFluffy)
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
   int in = XRectInRegion(NoSnowArea_dynamic,x, y, flake ->w, flake->h);
   int b  = (in == RectangleIn || in == RectanglePart); // true if in nosnowarea_dynamic
   //
   // if (b): no erase, no draw, no move
   if(b) return;

   if(Wind !=2  && !Flags.NoKeepSnowOnTrees && !Flags.NoTrees)
   {
      // check if flake is touching or in SnowOnTreesRegion
      // if so: remove it
      in = XRectInRegion(SnowOnTreesRegion,x,y,flake->w,flake->h);
      if (in == RectanglePart || in == RectangleIn)
      {
	 EraseSnowFlake(flake);
	 DeleteFlake(flake);
	 return;
      }

      // check if flake is touching TreeRegion. If so: add snow to 
      // SnowOnTreesRegion.
      in = XRectInRegion(TreeRegion,x,y,flake->w,flake->h);
      if (in == RectanglePart)
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
	    int in = XRectInRegion(TreeRegion,xbot,ybot,1,1);
	    if (in != RectangleIn) // if bottom pixel not in TreeRegion, skip
	       continue;
	    // move upwards, until pixel is not in TreeRegion
	    int j;
	    for (j=ybot-1; j >= y; j--)
	    {
	       int in = XRectInRegion(TreeRegion,xbot,j,1,1); 
	       if (in != RectangleIn)
	       {
		  // pixel (xbot,j) is snow-on-tree
		  found = 1;
		  XRectangle rec;
		  rec.x = xbot;
		  int p = RandInt(4);
		  rec.y = j-p+1;
		  rec.width = p;
		  rec.height = p;
		  XUnionRectWithRegion(&rec, SnowOnTreesRegion, SnowOnTreesRegion);
		  if(!Flags.NoBlowSnow && OnTrees < Flags.MaxOnTrees)
		  {
		     SnowOnTrees[OnTrees].x = rec.x;
		     SnowOnTrees[OnTrees].y = rec.y;
		     OnTrees++;
		     //P("%d %d %d\n",OnTrees,rec.x,rec.y);
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

   in = XRectInRegion(NoSnowArea_static,x, y, flake ->w, flake->h);
   b  = (in == RectangleIn || in == RectanglePart); // true if in nosnowarea_static
   // if(b): erase: no, move: yes
   // erase this flake 
   if(!b) EraseSnowFlake(flake);
   flake->rx = NewX;
   flake->ry = NewY;
   in = XRectInRegion(NoSnowArea_static,nx, ny, flake ->w, flake->h);
   b  = (in == RectangleIn || in == RectanglePart); // true if in nosnowarea_static
   // if b: draw: no
   if(!b) DrawSnowFlake(flake);
}

// Note: this function is only to be called in UpdateSnowFlake()
// or after a check if snow_running == 0
void DeleteFlake(Snow *flake)
{
   if (flake->prev)
   {
      delFlake(flake); FlakeCount--;
   }
   else                 //  deleting the first flake, but not it is the only one left
      if(flake->next) 
      {
	 FirstFlake = delFlake(flake); FlakeCount--;
      }
   P("deleteflake: flakecount:: %d\n",FlakeCount);
}


void DrawSnowFlake(Snow *flake) // draw snowflake using flake->rx and flake->ry
{
   if(Flags.NoSnowFlakes) return;
   int x = lrintf(flake->rx);
   int y = lrintf(flake->ry);
   XSetTSOrigin(display, SnowGC[flake->whatFlake], 
	 x + flake->w, y + flake->h);
   XFillRectangle(display, SnowWin, SnowGC[flake->whatFlake],
	 x, y, flake->w, flake->h);
}

void EraseSnowFlake(Snow *flake)
{
   if(Flags.NoSnowFlakes) return;
   int x = lrintf(flake->rx);
   int y = lrintf(flake->ry);
   if(UseAlpha|Flags.UseBG)
   {
      XSetTSOrigin(display, ESnowGC[flake->whatFlake], 
	    x + flake->w, y + flake->h);
      XFillRectangle(display, SnowWin, ESnowGC[flake->whatFlake],
	    x, y, flake->w, flake->h);
   }
   else
      XClearArea(display, SnowWin, 
	    x, y,
	    flake->w, flake->h,
	    Exposures);
}

// fallen snow and trees must have been initialized 
void InitBaumKoordinaten()
{
   if (Flags.NoTrees)
      return;
   int i,h,w;
   free(Tree);
   Tree = malloc(sizeof(*Tree)*Flags.DesiredNumberOfTrees);

   // determine which trees are to be used
   //
   int *tmptreetype, ntemp;
   if(TreeRead)
   {
      TreeType = realloc(TreeType,1*sizeof(*TreeType));
      TreeType[0] = 0;
   }
   else
   {
      if (!strcmp("all",Flags.TreeType))
	 // user wants all treetypes
      {
	 ntemp = 1+MAXTREETYPE;
	 tmptreetype = malloc(sizeof(*tmptreetype)*ntemp);
	 int i;
	 for (i=0; i<ntemp; i++)
	    tmptreetype[i] = i;
      }
      else if (strlen(Flags.TreeType) == 0) 
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
	 csvpos(Flags.TreeType,&tmptreetype,&ntemp);
      }

      NtreeTypes = 0;
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
	    for (j=0; j<NtreeTypes; j++)
	       if (tmptreetype[i] == TreeType[j])
	       {
		  unique = 0;
		  break;
	       }
	    if (unique) 
	    {
	       TreeType = realloc(TreeType,(NtreeTypes+1)*sizeof(*TreeType));
	       TreeType[NtreeTypes] = tmptreetype[i];
	       NtreeTypes++;
	    }
	 }
      }
      if(NtreeTypes == 0)
      {
	 TreeType = realloc(TreeType,sizeof(*TreeType));
	 TreeType[0] = DEFAULTTREETYPE;
	 NtreeTypes++;
      }
      free(tmptreetype);
   }

   //P("NtreeTypes: %d\n",NtreeTypes);
   //for (i=0; i<NtreeTypes; i++)
   //  P("%d\n",TreeType[i]);

   // determine placement of trees and NTrees:

   NTrees = 0;
   for (i=0; i< 4*Flags.DesiredNumberOfTrees; i++) // no overlap permitted
   {
      if (NTrees >= Flags.DesiredNumberOfTrees)
	 break;

      int tt = TreeType[RandInt(NtreeTypes)];
      //P("%d %d\n",tt,NtreeTypes);
      h = TreeHeight[tt];
      w = TreeWidth[tt];

      int y1 = SnowWinHeight - MaxScrSnowDepth - h;
      int y2 = SnowWinHeight*(1.0 - 0.01*Flags.TreeFill);
      if (y2>y1) y1=y2+1;

      int x = RandInt(SnowWinWidth);
      int y = y1 - RandInt(y1-y2);

      //P("treetry %4d %4d %4d %4d\n",x,y,y1,y2);
      int in = XRectInRegion(TreeRegion,x,y,w,h);
      if (in == RectangleIn || in == RectanglePart)
	 continue;

      //P("treesuc %4d %4d\n",x,y);
      Tree[NTrees].x    = x;
      Tree[NTrees].y    = y;
      Tree[NTrees].type = tt;
      int flop = (drand48()>0.5);
      Tree[NTrees].rev  = flop;

      Region r;

      switch(tt)
      {
	 case -SOMENUMBER:
	    r = regionfromxpm(TreeXpm,Tree[NTrees].rev);
	    break;
	 default:
	    r = regionfromxpm(xpmtrees[tt],Tree[NTrees].rev);
	    break;
      }
      XOffsetRegion(r,x,y);
      XUnionRegion(r,TreeRegion,TreeRegion);
      XDestroyRegion(r);
      NTrees++;
   }
   //for(i=0; i<NTrees; i++)
   //  P("%d\n",Tree[i].type);
   OnTrees = 0;
   return;
}

void InitStars()
{
   int i;
   free(star);
   NStars = Flags.NStars;
   star = malloc(NStars*sizeof(*star));
   for (i=0; i<NStars; i++)
   {
      star[i].x = RandInt(SnowWinWidth);
      star[i].y = RandInt(SnowWinHeight/4);
   }
   for (i=0; i<NStars; i++)
      star[i].color = RandInt(STARANIMATIONS);
}

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

void InitFallenSnow()
{
   //P("%d\n",SnowWinHeight);
   while (FsnowFirst)
      PopFallenSnow(&FsnowFirst);
   PushFallenSnow(&FsnowFirst, 0, CWorkSpace, 0, 0, 
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
   if(SnowRunning || Flags.NoBlowSnow || FlakeCount > Flags.FlakeCountMax || DoNotMakeSnow)
      return;
   //P("%#lx\n",fsnow->id);
   int i;
   int x = RandInt(fsnow->w - w);
   for(i=x; i<x+w; i++)
      if(fsnow->acth[i] > h)
      {
	 // animation of blown off snow
	 //if (!Flags.NoBlowSnow && abs(NewWind) > 100 && drand48() > 0.5)
	 if (!Flags.NoWind && !Flags.NoBlowSnow && Wind != 0 && drand48() > 0.5)
	 {
	    int j, jmax = BlowOff();
	    //P("%d\n",jmax);
	    for (j=0; j< jmax; j++)
	    {
	       FirstFlake = addFlake(FirstFlake); FlakeCount++;
	       InitFlake(FirstFlake);
	       FirstFlake->rx     = fsnow->x + i;
	       FirstFlake->ry     = fsnow->y - fsnow->acth[i] - drand48()*2*MaxSnowFlakeWidth;
	       FirstFlake->vx     = NewWind/8;
	       FirstFlake->vy     = -10;
	       FirstFlake->cyclic = (fsnow->id == 0); // not cyclic for Windows, cyclic for bottom
	    }
	    EraseFallenPixel(fsnow,i);
	 }
	 if(FlakeCount > Flags.FlakeCountMax)
	    return;
      }
}

Pixmap CreatePixmapFromFallen(FallenSnow *f)
{
   // todo: takes too much cpu
   int j;
   int p = 0;
   unsigned char bitmap[f->w8*f->h/8];

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
   return XCreateBitmapFromData(display, SnowWin, (char*)bitmap, f->w, f->h);
}

/*
   Region regionfrompixmap(char *bits, int w, int h)
   {
   Region r = XCreateRegion();
   int i,j,m=1,n=0;
   int bit;
   char *b = bits;
   XRectangle rec;
   rec.width  = 1;
   rec.height = 1;
   int w8 = ((w-1)/8+1)*8;

   for(j=0; j<h; j++)
   for (i=0; i<w8; i++)
   {
   if (n>7)
   {
   n=0;
   m=1;
   b++;
   }
   bit = (*b)&m;
   if (bit)
   {
   rec.x = i;
   rec.y = j;
   XUnionRectWithRegion(&rec,r,r);
   }
   m <<= 1;
   n++;
   }
   return r;
   }
   */

//void DrawTannenbaum(int i)
//{
//  int x = Tree[i].x; int y =Tree[i].y; int t = Tree[i].type;
// XSetTSOrigin(display, TreesGC[t], x+tannenbaumPix[t].width,y+tannenbaumPix[t].height);
//XFillRectangle(display, SnowWin, TreesGC[t],
//	 x,y,
//	 tannenbaumPix[t].width, tannenbaumPix[t].height);
//}

void ResetSanta()      
{
   SantaX = -SantaWidth - ActualSantaSpeed;
   SantaXr = SantaX;
   SantaY = RandInt(SnowWinHeight / 3)+40;
   SantaYStep = 1;
   CurrentSanta = 0;
   XDestroyRegion(SantaRegion);
   SantaRegion = RegionCreateRectangle(
	 SantaX,SantaY,SantaHeight,SantaWidth);

   XDestroyRegion(SantaPlowRegion);
   SantaPlowRegion = RegionCreateRectangle(
	 SantaX + SantaWidth, SantaY, 1, SantaHeight);
}

void UpdateSanta()
{
   if(Flags.NoSanta)
      return;
   int oldx = SantaX;
   int oldy = SantaY;
   static double dtt = 0;
   double dt = TNow - Prevtime[alarm_usanta];
   // after suspend or sleep dt could have a strange value
   if (dt < 0 || dt > 10*Delay[alarm_usanta])
      return;
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

   XOffsetRegion(SantaRegion, SantaX - oldx, SantaY - oldy);
   XOffsetRegion(SantaPlowRegion, SantaX - oldx, SantaY - oldy);

   return;
}

void DrawSanta() 
{
   if(OldSantaX != SantaX || OldSantaY != SantaY)
      EraseSanta(OldSantaX,OldSantaY);
   DrawSanta1();
   OldSantaX = SantaX;
   OldSantaY = SantaY;
   /* Note: the fur in his hat is *imitation* White-seal fur, of course. */
   /* Santa is a big supporter of Greenpeace.                            */
}

void DrawSanta1()
{
   XSetClipMask(display,
	 SantaGC,
	 SantaMaskPixmap[CurrentSanta]);
   XSetClipOrigin(display,
	 SantaGC,
	 SantaX,SantaY);
   XCopyArea(display,
	 SantaPixmap[CurrentSanta],
	 SnowWin,
	 SantaGC,
	 0,0,SantaWidth,SantaHeight,
	 SantaX,SantaY);
}

void EraseSanta(int x, int y)
{
   if(UseAlpha|Flags.UseBG)
      XFillRectangle(display, SnowWin, ESantaGC, x,y,SantaWidth+1,SantaHeight);
   // probably due to rounding errors in computing SantaX, one pixel in front 
   // is not erased when leaving out the +1
   else
      XClearArea(display, SnowWin,
	    x , y,     
	    SantaWidth+1,SantaHeight,
	    Exposures);
}

void DrawTree(int i) 
{
   int x = Tree[i].x; int y = Tree[i].y; int t = Tree[i].type; int r = Tree[i].rev;
   //P("t = %d\n",t);
   if (t<0) t=0;
   XSetClipMask(display, TreeGC, TreeMaskPixmap[t][r]);
   XSetClipOrigin(display, TreeGC, x, y);
   XCopyArea(display, TreePixmap[t][r], SnowWin, TreeGC, 
	 0,0,TreeWidth[t],TreeHeight[t], x, y);
}

void EraseTrees()
{
   int i;
   int d = 3;
   for (i=0; i<NTrees; i++)
   {
      int x = Tree[i].x-d; 
      int y = Tree[i].y-d; 
      int t = Tree[i].type; 
      int w = TreeWidth[t]+d+d;
      int h = TreeHeight[t]+d+d;
      if(UseAlpha|Flags.UseBG)
	 XFillRectangle(display, SnowWin, ESantaGC, 
	       x, y, w, h);
      else
	 XClearArea(display, SnowWin,
	       x, y, w, h, Exposures);
   }

   XDestroyRegion(TreeRegion);
   TreeRegion = XCreateRegion();
   XDestroyRegion(SnowOnTreesRegion);
   SnowOnTreesRegion = XCreateRegion();
   ClearScreen();
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

Pixel AllocNamedColor(char *colorName, Pixel dfltPix)
{
   XColor scrncolor;
   XColor exactcolor;
   if (XAllocNamedColor(display, DefaultColormap(display, screen),
	    colorName, &scrncolor, &exactcolor)) 
      return scrncolor.pixel;
   else
      return dfltPix;
}

Pixel IAllocNamedColor(char *colorName, Pixel dfltPix)
{
   return AllocNamedColor(colorName, dfltPix) | 0xff000000;
}
/* ------------------------------------------------------------------ */ 


Window XWinInfo(char **name)
{
   Window win = Select_Window(display,1);
   XTextProperty text_prop;
   int rc = XGetWMName(display,win,&text_prop);
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
   //P("%d %d %d %d %d %d\n",SnowWinX,SnowWinY,SnowWinWidth,
   //	 SnowWinHeight,SnowWinDepth,Flags.OffsetS);

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


void SetSantaSpeed()
{
   SantaSpeed = Speed[Flags.SantaSize];
   if (Flags.SantaSpeedFactor < 10)
      SantaSpeed = 0.1*SantaSpeed;
   else
      SantaSpeed = 0.01*Flags.SantaSpeedFactor*SantaSpeed;
   ActualSantaSpeed               = SantaSpeed;
}

/* ------------------------------------------------------------------ */ 
void InitSantaPixmaps()
{
   XpmAttributes attributes;
   //P("InitSantaPixmaps: SantaSize=%d NoRudolf=%d\n",Flags.SantaSize,Flags.NoRudolf);
   //attributes.visual = DefaultVisual(display,DefaultScreen(display));
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
      path[i] = 0;
      f = HomeOpen(filenames[i],"r",&path[i]);
      if(!f){ ok = 0; if (path[i]) free(path[i]); break; }
      fclose(f);
   }
   if (ok)
   {
      printf("Using external Santa: %s.\n",path[0]);
      if (!Flags.NoMenu)
	 printf("Disabling menu.\n");
      Flags.NoMenu = 1;
      int rc,i;
      char **santaxpm;
      for (i=0; i<PIXINANIMATION; i++)
      {
	 if(SantaPixmap[i]) 
	    XFreePixmap(display,SantaPixmap[i]);
	 if(SantaMaskPixmap[i]) 
	    XFreePixmap(display,SantaMaskPixmap[i]);
	 rc = XpmReadFileToData(path[i],&santaxpm);
	 if(rc == XpmSuccess)
	 {
	    iXpmCreatePixmapFromData(display, SnowWin, santaxpm, 
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
   withRudolf = !Flags.NoRudolf;

   for(i=0; i<PIXINANIMATION; i++)
   {
      if(SantaPixmap[i]) 
	 XFreePixmap(display,SantaPixmap[i]);
      if(SantaMaskPixmap[i]) 
	 XFreePixmap(display,SantaMaskPixmap[i]);
      rc[i] = iXpmCreatePixmapFromData(display, SnowWin, 
	    Santas[Flags.SantaSize][withRudolf][i], 
	    &SantaPixmap[i], &SantaMaskPixmap[i], &attributes,0);
      sscanf(Santas[Flags.SantaSize][withRudolf][0][0],"%d %d", 
	    &SantaWidth,&SantaHeight);
   }

   int wrong = 0;
   for (i=0; i<PIXINANIMATION; i++)
   {
      if (rc[i])
      {
	 printf("Something wrong reading Santa's xpm nr %d: errorstring %s\n",rc[i],XpmGetErrorString(rc[i]));
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
      NtreeTypes = 1;
      TreeRead = 1;
      int rc = XpmReadFileToData(path,&TreeXpm);
      if(rc == XpmSuccess)
      {
	 int i;
	 for(i=0; i<2; i++)
	    iXpmCreatePixmapFromData(display, SnowWin, TreeXpm, 
		  &TreePixmap[0][i], &TreeMaskPixmap[0][i], &attributes,i);
	 sscanf(*TreeXpm,"%d %d", &TreeWidth[0],&TreeHeight[0]);
	 //P("%d %d\n",TreeWidth[0],TreeHeight[0]);
	 printf("using external tree: %s\n",path);
	 if (!Flags.NoMenu)
	    printf("Disabling menu.\n");
	 Flags.NoMenu = 1;
      }
      else
      {
	 printf("Invalid external xpm for tree given: %s\n",path);
	 exit(1);
      }
      fclose(f);
      free(path);
   }
   else
   {
      int i;
      for(i=0; i<2; i++)
      {
	 int tt;
	 for (tt=0; tt<=MAXTREETYPE; tt++)
	 {
	    iXpmCreatePixmapFromData(display, SnowWin, xpmtrees[tt],
		  &TreePixmap[tt][i],&TreeMaskPixmap[tt][i],&attributes,i);
	    sscanf(xpmtrees[tt][0],"%d %d",&TreeWidth[tt],&TreeHeight[tt]);
	 }
      }
      ReInitTree0();
   }
   OnTrees = 0;
}

// apply TreeColor to xpmtree[0] and xpmtree[1]
void ReInitTree0()
{
   XpmAttributes attributes;
   attributes.valuemask = XpmDepth;
   attributes.depth     = SnowWinDepth;
   int i;
   int n = TreeHeight[0]+3;
   char *xpmtmp[n];
   int j;
   for (j=0; j<2; j++)
      xpmtmp[j] = strdup(xpmtrees[0][j]);
   xpmtmp[2] = strdup(". c ");
   xpmtmp[2] = realloc(xpmtmp[2],strlen(xpmtmp[2])+strlen(Flags.TreeColor)+1);
   strcat(xpmtmp[2],Flags.TreeColor);
   for(j=3; j<n; j++)
      xpmtmp[j] = strdup(xpmtrees[0][j]);
   for(i=0; i<2; i++)
   {
      XFreePixmap(display,TreePixmap[0][i]);
      iXpmCreatePixmapFromData(display, SnowWin, xpmtmp,
	    &TreePixmap[0][i],&TreeMaskPixmap[0][i],&attributes,i);
   }
   for (j=0; j<n; j++)
      free(xpmtmp[j]);
}


FILE *HomeOpen(char *file,char *mode, char**path)
{
   char *h = getenv("HOME");
   if (h == 0)
      return 0;
   char *home = strdup(h);
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
   float g = gaussian(BlowOffFactor,0.5*BlowOffFactor,0.0,2.0*MAXBLOWOFFFACTOR);
   //P("%f %f %f %ld\n",BlowOffFactor,2.0*MAXBLOWOFFFACTOR,g,lrint(g));
   return lrint(g);
}

void InitFlakesPerSecond()
{
   P("snowflakesfactor: %d\n",Flags.SnowFlakesFactor);
   FlakesPerSecond = SnowWinWidth*0.01*Flags.SnowFlakesFactor*
      0.001*FLAKES_PER_SEC_PER_PIXEL*SnowSpeedFactor;
}

void InitSnowColor()
{
   int i;
   SnowcPix = IAllocNamedColor(Flags.SnowColor, White);   
   for (i=0; i<=SNOWFLAKEMAXTYPE; i++) 
      XSetForeground(display, SnowGC[i], SnowcPix);
}

void InitSnowSpeedFactor()
{
   if (Flags.SnowSpeedFactor < 10)
      SnowSpeedFactor = 0.01*10;
   else
      SnowSpeedFactor = 0.01*Flags.SnowSpeedFactor;
   SnowSpeedFactor *= SNOWSPEED;
}

void InitBlowOffFactor()
{
   BlowOffFactor = 0.01*Flags.BlowOffFactor;
   if (BlowOffFactor > MAXBLOWOFFFACTOR)
      BlowOffFactor = MAXBLOWOFFFACTOR;
}

void InitSnowOnTrees()
{
   free(SnowOnTrees);
   SnowOnTrees = malloc(sizeof(*SnowOnTrees)*Flags.MaxOnTrees);
}

void CreateAlarmDelays()
{
   double factor;
   if (Flags.CpuLoad <= 0)
      factor = 1;
   else
      factor = 100.0/Flags.CpuLoad;

   //P("%d %f\n",Flags.CpuLoad,factor);
   //
   // define delays for alarms:
#define ALARM(x,y) Delay[alarm_ ## x] = y;
   ALARMALL;
#undef ALARM
   if (!Exposures) 
      Delay[alarm_santa1] = 10*factor;
   P("alarm_santa1: %d %f\n",Exposures,Delay[alarm_santa1]);
}

void SetGCFunctions()
{
   int i;
   if (Flags.UseBG)
      ErasePixel = AllocNamedColor(Flags.BGColor,Black) | 0xff000000;
   else
      ErasePixel = 0;
   XSetFunction(display,   SantaGC, GXcopy);
   XSetForeground(display, SantaGC, BlackPix);
   XSetFillStyle(display,  SantaGC, FillStippled);

   XSetFunction(display,   ESantaGC, GXcopy);
   XSetFillStyle(display,  ESantaGC, FillSolid);
   XSetForeground(display, ESantaGC, ErasePixel);

   XSetFunction(display,   TreeGC, GXcopy);
   XSetForeground(display, TreeGC, BlackPix);
   XSetFillStyle(display,  TreeGC, FillStippled);

   XSetFunction(display, SnowOnTreesGC, GXcopy);


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

   XSetLineAttributes(display, meteorite.gc,  1,LineSolid,CapRound,JoinMiter);
   XSetLineAttributes(display, meteorite.egc, 1,LineSolid,CapRound,JoinMiter);
   if(UseAlpha)
   {
      XSetFunction(display,   meteorite.gc,  GXcopy);
      XSetForeground(display, meteorite.gc,  MeteoPix);
      XSetFunction(display,   meteorite.egc, GXcopy);
      XSetForeground(display, meteorite.egc, ErasePixel);
   }
   else
   {
      XSetFunction(display,   meteorite.gc,  GXxor);
      XSetForeground(display, meteorite.gc,  MeteoPix);
      XSetFunction(display,   meteorite.egc, GXxor);
      XSetForeground(display, meteorite.egc, MeteoPix);
   }
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
	 // if envvar DESKTOP_SESSION == LXDE, search for window with name pcmanfm
	 char *desktopsession;
	 desktopsession = getenv("DESKTOP_SESSION");
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
	    create_transparent_window(Flags.FullScreen, Flags.BelowAll, Flags.AllWorkspaces, 
		  &SnowWin, &SnowWinName, &GtkWin,w,h);

	    Isdesktop = 1;
	    UseAlpha  = 1;
	    XGetGeometry(display,SnowWin,&root,
		  &x, &y, &w, &h, &b, &depth);
	    P("depth: %d snowwin: 0x%lx %s\n",depth,SnowWin,SnowWinName);
	    if(SnowWin)
	    {
	       TransWorkSpace = GetCurrentWorkspace();
	       UsingTrans     = 1;
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
   // P("Isdesktop: %d\n",Isdesktop);
   if(Isdesktop) 
      CWorkSpace = GetCurrentWorkspace();
   if (CWorkSpace < 0)
      return 0;
   InitDisplayDimensions();
   // if depth != 32, we assume that the desktop is not gnome-like TODO
   if (Isdesktop && SnowWinDepth != 32)
      UseAlpha = 0;
   // override UseAlpha if user desires so:
   if (Flags.UseAlpha != SOMENUMBER)
      UseAlpha = Flags.UseAlpha;

   Flags.UseAlpha = UseAlpha;   // we could run into trouble with this, let's see...
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

   Flags.Exposures = Exposures;   // trouble ?
}


Region RegionCreateRectangle(int x, int y, int w, int h)
{
   XPoint p[5];
   p[0] = (XPoint){x  , y  };
   p[1] = (XPoint){x+w, y  };
   p[2] = (XPoint){x+w, y+h};
   p[3] = (XPoint){x  , y+h}; 
   p[4] = (XPoint){x  , y  };
   return XPolygonRegion(p, 5, EvenOddRule);
}

