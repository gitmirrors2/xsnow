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
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
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

#ifdef DEBUG
#undef DEBUG
#endif
//#define DEBUG

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
char   *DesktopSession = 0;
int     IsCompiz;
int     IsWayland;
int     UseAlpha;
Pixel   ErasePixel;
int     Exposures;
Pixel   BlackPix;
int     counter = 0;
int     UseGtk;
GtkWidget *GtkWinb = NULL; 

double  factor = 1.0;
float   NewWind = 0;

GtkWidget       *drawing_area = 0;
GdkWindow       *gdkwindow = 0;



// miscellaneous
char       Copyright[] = "\nXsnow\nCopyright 1984,1988,1990,1993-1995,2000-2001 by Rick Jansen, all rights reserved, 2019,2020 also by Willem Vermin\n";


// timing stuff
//static double       TotSleepTime = 0;
static double       TStart;

// windows stuff
long                CWorkSpace = 0;
long                TransWorkSpace = -SOMENUMBER;  // workspace on which transparent window is placed
static int          DoRestart = 0;



/* Colo(u)rs */
static const char *BlackColor  = "black";


/* GC's */
static GC CleanGC;
static GC TestingGC;
//static GC TreesGC[2];

// region stuff
//static Region NoSnowArea_static;

/* Forward decls */
static void   HandleFactor(void);
static void   HandleExposures(void);
static void   RestartDisplay(void);
static void   SetGCFunctions(void);
static void   SigHandler(int signum);
static int    XsnowErrors(Display *dpy, XErrorEvent *err);
static void   drawit(cairo_t *cr);
static void   show_desktop_type(void);


static void Thanks(void)
{
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
   signal(SIGINT, SigHandler);
   signal(SIGTERM, SigHandler);
   signal(SIGHUP, SigHandler);
   srand48((long int)(wallcl()*1.0e6));
   //testje();
   // we search for flags that only produce output to stdout,
   // to enable to run in a non-X environment, in which case 
   // gtk_init() would fail.
   int i;
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
	 //         Ok, this cannot happen, is already caught above
	 return 0;
	 break;
      default:  // ditto
	 PrintVersion();
	 break;
   }
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
   //SetWhirl();
   //SetWindTimer();

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
   scenery_init();
   snow_init();
   meteo_init();
   wind_init();
   stars_init();
   fallensnow_init();
   blowoff_init();
   treesnow_init();
   windows_init();
   loadmeasure_init();

#define DOIT_I(x) OldFlags.x = Flags.x;
#define DOIT_L(x) DOIT_I(x);
#define DOIT_S(x) OldFlags.x = strdup(Flags.x);
   DOITALL;
#undef DOIT_I
#undef DOIT_L
#undef DOIT_S


   BlackPix = AllocNamedColor(BlackColor, Black);

   TestingGC     = XCreateGC(display, RootWindow, 0, 0);
   CleanGC       = XCreateGC(display, SnowWin,    0, 0);
   /*
      FallenGC      = XCreateGC(display, SnowWin,    0, 0);
      EFallenGC     = XCreateGC(display, SnowWin,    0, 0);  // used to erase fallen snow
      */

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

   add_to_mainloop(PRIORITY_DEFAULT, time_displaychanged, do_displaychanged     ,0);
   add_to_mainloop(PRIORITY_DEFAULT, time_event,          do_event              ,0);
   add_to_mainloop(PRIORITY_DEFAULT, time_testing,        do_testing            ,0);
   add_to_mainloop(PRIORITY_HIGH, time_ui_check,       do_ui_check           ,0);
   //   add_to_mainloop(PRIORITY_DEFAULT, time_wupdate,        do_wupdate            ,0);
   add_to_mainloop(PRIORITY_DEFAULT, time_show_range_etc, do_show_range_etc     ,0);
   //add_to_mainloop(PRIORITY_DEFAULT, 1.0,                 do_show_desktop_type  ,0);

   if (Flags.StopAfter > 0)
      add_to_mainloop(PRIORITY_DEFAULT, Flags.StopAfter, do_stopafter, 0);

   HandleFactor();

   if(!Flags.NoMenu)
   {
      ui(&argc, &argv);
      if (UseGtk)
      {
	 gtk_widget_show_all(GtkWinb);
      }
      else
      {
	 ui_set_birds_header("Your screen does not support alpha channel, no birds will fly.");
      }
      ui_set_sticky(Flags.AllWorkspaces);
      show_desktop_type();
   }

   // main loop
   gtk_main();


   if (SnowWinName) free(SnowWinName);

   XClearArea(display, SnowWin, 0,0,0,0,True);
   XFlush(display);
   XCloseDisplay(display); //also frees the GC's, pixmaps and other resources on display

   if (DoRestart)
   {
      sleep(2);
      extern char **environ;
      execve(argv[0],argv,environ);
   }
   Thanks();
   return 0;
}		/* End of snowing */


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
int do_ui_check(gpointer data)
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
   changes += wind_ui();
   changes += stars_ui();
   changes += fallensnow_ui();
   changes += blowoff_ui();
   changes += treesnow_ui();

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
      init_stars();
      EraseTrees();
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
   if(Flags.FullScreen != OldFlags.FullScreen)
   {
      OldFlags.FullScreen = Flags.FullScreen;
      DetermineWindow();
      InitFallenSnow();
      init_stars();
      EraseTrees();
      ClearScreen();
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.AllWorkspaces != OldFlags.AllWorkspaces)
   {
      if(0)
      {
	 DetermineWindow();
      }
      else
      {
	 if(Flags.AllWorkspaces)
	 {
	    R("stick\n");
	    if (UseGtk)
	       gtk_window_stick(GTK_WINDOW(GtkWinb));
	 }
	 else
	 {
	    R("unstick\n");
	    if (UseGtk)
	       gtk_window_unstick(GTK_WINDOW(GtkWinb));
	 }
      }
      OldFlags.AllWorkspaces = Flags.AllWorkspaces;
      ui_set_sticky(Flags.AllWorkspaces);
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.BelowAll != OldFlags.BelowAll)
   {
      OldFlags.BelowAll = Flags.BelowAll;
      if(1)
	 DetermineWindow();
      else   // this does not work:
      {
	 if(UseGtk)
	 {
	    if(Flags.BelowAll)
	    {
	       R("below\n");
	       gtk_window_set_keep_above(GTK_WINDOW(GtkWinb), FALSE);
	       gtk_window_set_keep_below(GTK_WINDOW(GtkWinb), TRUE);
	    }
	    else
	    {
	       R("above\n");
	       gtk_window_set_keep_below(GTK_WINDOW(GtkWinb), FALSE);
	       gtk_window_set_keep_above(GTK_WINDOW(GtkWinb), TRUE);
	    }
	 }
      }
      changes++;
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




int do_displaychanged(gpointer data)
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

int do_event(gpointer data)
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
   XClearArea(display, SnowWin, 0,0,0,0,Exposures);
}



int do_show_range_etc(gpointer data)
{
   if (Flags.Done)
      return FALSE;

   if (Flags.NoMenu)
      return TRUE;
   ui_show_range_etc();
   return TRUE;
}

void show_desktop_type()
{
   if (Flags.NoMenu)
      return;
   if (IsWayland)
      ui_show_desktop_type("Wayland (Expect some slugginess)");
   else if (IsCompiz)
      ui_show_desktop_type("Compiz");
   else
      ui_show_desktop_type("Probably X11");
}



int do_testing(gpointer data)
{
   counter++;
   if (Flags.Done)
      return FALSE;
   P("Wind: %d %f\n",Wind,NewWind);
   P("%d cw %#lx\n",counter, CWorkSpace);
   //PrintFallenSnow(FsnowFirst);
   return TRUE;
}


/* ------------------------------------------------------------------ */ 
void SigHandler(int signum)
{
   Flags.Done = 1;
}
/* ------------------------------------------------------------------ */ 

/*
   int SnowPtInRect(int snx, int sny, int recx, int recy, int width, int height)
   {
   if (snx < recx) return 0;
   if (snx > (recx + width)) return 0;
   if (sny < recy) return 0;
   if (sny > (recy + height)) return 0;
   return 1;
   }
   */




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




// the draw callback
gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data) 
{
   P("Just to check who this is: %p %p\n",(void *)widget,(void *)GtkWinb);
   drawit(cr);
   return FALSE;
}

void drawit(cairo_t *cr)
{
   P("drawit %d\n",counter++);
   stars_draw(cr);

   meteo_draw(cr);

   scenery_draw(cr);

   Santa_draw(cr);

   birds_draw(cr);

   snow_draw(cr);

   fallensnow_draw(cr);

   treesnow_draw(cr);

}

int do_draw_all(gpointer widget)
{
   if (Flags.Done)
      return FALSE;
   P("do_draw_all %d %p\n",counter++,(void *)widget);

   gtk_widget_queue_draw(GTK_WIDGET(widget));
   return TRUE;
}


// handle callbacks for things whose timings depend on factor
void HandleFactor()
{
   static guint fallen_id=0;

   float oldfactor = factor;
   // re-add things whose timing is dependent on factor
   if (Flags.CpuLoad <= 0)
      factor = 1;
   else
      factor = 100.0/Flags.CpuLoad;

   //EraseTrees();

   if (fallen_id)
      g_source_remove(fallen_id);

   if(!UseGtk)
      Santa_HandleFactor();

   fallen_id = add_to_mainloop(PRIORITY_DEFAULT, time_fallen, do_fallen, 0);
   P("handlefactor %f %f %d\n",oldfactor,factor,counter++);
   if (factor > oldfactor) // user wants a smaller cpu load
      add_to_mainloop(PRIORITY_HIGH, 0.2 , do_initsnow, 0);  // remove flakes

   restart_do_draw_all();
}

void restart_do_draw_all()
{
   static guint draw_all_id = 0;
   if (!UseGtk)
      return;
   if (draw_all_id)
      g_source_remove(draw_all_id);
   draw_all_id = add_to_mainloop(PRIORITY_HIGH, time_draw_all, do_draw_all, GtkWinb);
}


void SetGCFunctions()
{
   if(UseGtk)
      return;
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

   treesnow_set_gc();
   // XSetFunction(display, SnowOnTreesGC, GXcopy);


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

   stars_set_gc();
   /*
      for (i=0; i<STARANIMATIONS; i++)
      {
      XSetFunction(   display,StarGC[i],GXcopy);
      XSetStipple(    display,StarGC[i],starPix.pixmap);
      XSetForeground( display,StarGC[i],StarcPix[i]);
      XSetFillStyle(  display,StarGC[i],FillStippled);
      }
      */

   XSetFunction(display,CleanGC, GXcopy);
   XSetForeground(display,CleanGC,BlackPix);

   /*
      XSetLineAttributes(display, FallenGC, 1, LineSolid,CapRound,JoinMiter);
      */

   fallensnow_set_gc();
   /*
      XSetFillStyle( display, EFallenGC, FillSolid);
      XSetFunction(  display, EFallenGC, GXcopy);
      XSetForeground(display, EFallenGC, ErasePixel);
      */

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


int do_stopafter(gpointer data)
{
   Flags.Done = 1;
   return FALSE;
}
