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

#include <stdio.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <math.h>
#include "debug.h"
#include "xsnow.h"
#include "pixmaps.h"
#include "windows.h"
#include "hashtable.h"
#include "flags.h"
#include "ixpm.h"
#include "snow.h"
#include "utils.h"
#include "clocks.h"
#include "wind.h"
#include "fallensnow.h"
#include "scenery.h"
#include "ui.h"
#include "blowoff.h"
#include "treesnow.h"
#include "varia.h"

#define NOTACTIVE \
   (Flags.BirdsOnly || !WorkspaceActive() || Flags.NoSnowFlakes)

//static cairo_surface_t *snow_surfaces[SNOWFLAKEMAXTYPE+1];
//static cairo_surface_t *snow_surfaces[1000];
static cairo_surface_t **snow_surfaces;
static float             FlakesPerSecond;
static int               KillFlakes = 0;  // 1: signal to flakes to kill themselves, and do not generate flakes
static float             SnowSpeedFactor;
//static GC       ESnowGC[SNOWFLAKEMAXTYPE+1];  // There are SNOWFLAKEMAXTYPE+1 flakes
//static GC       ESnowGC[1000];  // There are SNOWFLAKEMAXTYPE+1 flakes
static GC                *ESnowGC;
//static GC       SnowGC[SNOWFLAKEMAXTYPE+1];  // There are SNOWFLAKEMAXTYPE+1 flakes
//static GC       SnowGC[1000];  // There are SNOWFLAKEMAXTYPE+1 flakes
static GC                *SnowGC;

static int    do_genflakes(gpointer data);
static void   InitFlake(Snow *flake);
static void   InitFlakesPerSecond(void);
static void   InitSnowColor(void);
static void   InitSnowSpeedFactor(void);
static int    do_show_flakecount(gpointer data);
static void   init_snow_surfaces(void);
static void   EraseSnowFlake(Snow *flake);
static void   DelFlake(Snow *flake);
static void   DrawSnowFlake(Snow *flake);


Region     NoSnowArea_dynamic;
Pixel      SnowcPix;
int        MaxSnowFlakeHeight = 0;  /* Biggest flake */
int        MaxSnowFlakeWidth = 0;   /* Biggest flake */
int        FlakeCount = 0;
int        MaxFlakeTypes;
int        NFlakeTypes;

void snow_init()
{
   int i;
   MaxFlakeTypes = 0;
   while(snowPix[MaxFlakeTypes].snowBits)
      MaxFlakeTypes++;

   NFlakeTypes = MaxFlakeTypes;

   snow_surfaces = (cairo_surface_t **)malloc(MaxFlakeTypes*sizeof(cairo_surface_t*));
   ESnowGC       = (GC               *)malloc(MaxFlakeTypes*sizeof(GC));
   SnowGC        = (GC               *)malloc(MaxFlakeTypes*sizeof(GC));


   P("MaxFlakeTypes: %d\n",MaxFlakeTypes);

   //for (i=0; i<SNOWFLAKEMAXTYPE +1; i++)
   for (i=0; i<MaxFlakeTypes; i++)
      snow_surfaces[i] = NULL;

   //for (i=0; i<=SNOWFLAKEMAXTYPE; i++) 
   for (i=0; i<MaxFlakeTypes; i++) 
   {
      SnowGC[i]  = XCreateGC(display, SnowWin, 0, NULL);
      ESnowGC[i] = XCreateGC(display, SnowWin, 0, NULL);
   }
   init_snow_surfaces();
   InitSnowSpeedFactor();
   InitFlakesPerSecond();
   InitSnowColor();
   InitSnowSpeedFactor();
   InitBlowOffFactor();
   add_to_mainloop(PRIORITY_DEFAULT, time_genflakes,      do_genflakes          ,NULL);
   add_to_mainloop(PRIORITY_DEFAULT, time_flakecount,     do_show_flakecount    ,NULL);
   int flake;
   //for (flake=0; flake<=SNOWFLAKEMAXTYPE; flake++) 
   for (flake=0; flake<MaxFlakeTypes; flake++) 
   {
      SnowMap *rp = &snowPix[flake];
      rp->pixmap = XCreateBitmapFromData(display, SnowWin,
	    (char *)rp->snowBits, rp->width, rp->height);
      if (rp->height > MaxSnowFlakeHeight) MaxSnowFlakeHeight = rp->height;
      if (rp->width  > MaxSnowFlakeWidth ) MaxSnowFlakeWidth  = rp->width;
   }
}


void snow_set_gc()
{
   int i;
   //for (i=0; i<=SNOWFLAKEMAXTYPE; i++) 
   for (i=0; i<MaxFlakeTypes; i++) 
   {
      XSetFunction(   display, SnowGC[i], GXcopy);
      XSetStipple(    display, SnowGC[i], snowPix[i].pixmap);
      XSetFillStyle(  display, SnowGC[i], FillStippled);

      XSetFunction(   display, ESnowGC[i], GXcopy);
      XSetStipple(    display, ESnowGC[i], snowPix[i].pixmap);
      XSetForeground( display, ESnowGC[i], ErasePixel);
      XSetFillStyle(  display, ESnowGC[i], FillStippled);
   }
}

int snow_ui()
{
   int changes = 0;

   if(Flags.NoSnowFlakes != OldFlags.NoSnowFlakes)
   {
      OldFlags.NoSnowFlakes = Flags.NoSnowFlakes;
      if(Flags.NoSnowFlakes)
	 ClearScreen();
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.SnowFlakesFactor != OldFlags.SnowFlakesFactor)
   {
      OldFlags.SnowFlakesFactor = Flags.SnowFlakesFactor;
      InitFlakesPerSecond();
      changes++;
      P("changes: %d\n",changes);
   }
   if(strcmp(Flags.SnowColor, OldFlags.SnowColor))
   {
      InitSnowColor();
      ClearScreen();
      free(OldFlags.SnowColor);
      OldFlags.SnowColor = strdup(Flags.SnowColor);
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.SnowSpeedFactor != OldFlags.SnowSpeedFactor)
   {
      OldFlags.SnowSpeedFactor = Flags.SnowSpeedFactor;
      InitSnowSpeedFactor();
      changes++;
      P("changes: %d\n",changes);
   }
   if(Flags.FlakeCountMax != OldFlags.FlakeCountMax)
   {
      OldFlags.FlakeCountMax = Flags.FlakeCountMax;
      changes++;
      P("changes: %d\n",changes);
   }

   return changes;
}

void init_snow_surfaces()
{
   GdkPixbuf *pixbuf;
   int i;
   //for(i=0; i<SNOWFLAKEMAXTYPE+1; i++)
   for(i=0; i<MaxFlakeTypes; i++)
   {
      P("%d\n",i);
      char **x;
      int lines;
      xpm_set_color(snow_xpm[i], &x, &lines, Flags.SnowColor);
      pixbuf = gdk_pixbuf_new_from_xpm_data((const char **)x);
      xpm_destroy(x,lines);
      if (snow_surfaces[i])
	 cairo_surface_destroy(snow_surfaces[i]);
      snow_surfaces[i] = gdk_cairo_surface_create_from_pixbuf (pixbuf, 0, gdkwindow);
      g_clear_object(&pixbuf);
   }
}

int snow_draw(cairo_t *cr)
{
   if (Flags.NoSnowFlakes)
      return TRUE;

   set_begin();
   Snow *flake;
   while( (flake = (Snow *)set_next()) )
   {
      P("snow_draw %d %f\n",counter++,ALPHA);
      cairo_set_source_surface (cr, snow_surfaces[flake->whatFlake], flake->rx, flake->ry);
      double alpha = ALPHA;
      if (flake->fluff)
	 alpha *= flake->flufftimer>0?flake->flufftimer/FLUFFTIME:0;
      cairo_paint_with_alpha(cr,alpha);
      //if (flake->testing) P("testing flake: alpha: %f\n",alpha);
   }
   return TRUE;
}

int do_genflakes(UNUSED gpointer data)
{
   if (Flags.Done)
      return FALSE;

#define RETURN do {Prevtime = TNow; return TRUE;} while (0)

   static double Prevtime;
   static double sumdt;
   static int    first_run = 1;
   double TNow = wallclock();

   if (KillFlakes)
      RETURN;

   if (NOTACTIVE)
      RETURN;

   if (first_run)
   {
      first_run = 0;
      Prevtime = wallclock();
      sumdt    = 0;
   }

   double dt = TNow - Prevtime;

   // after suspend or sleep dt could have a strange value
   if (dt < 0 || dt > 10*time_genflakes)
      RETURN;
   int desflakes = lrint((dt+sumdt)*FlakesPerSecond);
   P("desflakes: %lf %lf %d %lf %d\n",dt,sumdt,desflakes,FlakesPerSecond,FlakeCount);
   if(desflakes == 0)  // save dt for use next time: happens with low snowfall rate
      sumdt += dt; 
   else
      sumdt = 0;

   int i;
   for(i=0; i<desflakes; i++)
   {
      Snow *flake = MakeFlake(-1);
      add_flake_to_mainloop(flake);
   }
   RETURN;
#undef RETURN
}

int do_UpdateSnowFlake(Snow *flake)
{
   if(NOTACTIVE)
      return TRUE;
   int fckill = FlakeCount >= Flags.FlakeCountMax;
   if (
	 KillFlakes                                    ||  // merciless remove if KillFlakes
	 (fckill && !flake->cyclic && drand48() > 0.5) ||  // high probability to remove blown-off flake
	 (fckill && drand48() > 0.9)                   ||  // low probability to remove other flakes
	 (flake->fluff && flake->flufftimer < 0)           // fluff has expired
      )
   {
      EraseSnowFlake(flake);
      DelFlake(flake);
      return FALSE;
   }

   double FlakesDT = time_snowflakes;
   // handle fluff
   if (flake->fluff)
   {
      flake->flufftimer -= FlakesDT;
      return TRUE;
   }
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

   flake->vy += INITIALYSPEED * (drand48()-0.4)*0.1 ;
   if (flake->vy > flake->ivy*1.5) flake->vy = flake->ivy*1.5;

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
      DelFlake(flake);
      return FALSE;
   }

   // remove flake if it falls below bottom of screen:
   if (NewY >= SnowWinHeight)
   {
      EraseSnowFlake(flake);
      DelFlake(flake);
      return FALSE;
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
      if(!fsnow->win.hidden)
	 if(fsnow->win.id == 0 ||(fsnow->win.ws == CWorkSpace || fsnow->win.sticky))
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
			   flake->fluff      = 1;
			   flake->flufftimer = FLUFFTIME;
			}
			if (flake->fluff)
			   return TRUE;
			else
			{
			   DelFlake(flake);
			   return FALSE;
			}
		     }
		     found = 1;
		     break;
		  }
	    }
	 }
      fsnow = fsnow->next;
   }

   int x  = lrintf(flake->rx);
   int y  = lrintf(flake->ry);

   // check if flake is in nowsnowarea
   if (!switches.UseGtk)  // we can skip this when using gtk
   {
      int in = XRectInRegion(NoSnowArea_dynamic,x, y, flake ->w, flake->h);
      int b  = (in == RectangleIn || in == RectanglePart); // true if in nosnowarea_dynamic
      //
      // if (b): no erase, no draw, no move
      if(b) 
	 return TRUE;
   }

   if(Wind !=2  && !Flags.NoKeepSnowOnTrees && !Flags.NoTrees)
   {
      // check if flake is touching or in SnowOnTreesRegion
      // if so: remove it
      int in = XRectInRegion(SnowOnTreesRegion,x,y,flake->w,flake->h);
      if (in == RectanglePart || in == RectangleIn)
      {
	 if (Flags.NoFluffy)
	    EraseSnowFlake(flake);
	 else
	 {
	    flake->fluff      = 1;
	    flake->flufftimer = FLUFFTIME;
	 }
	 if (flake->fluff)
	    return TRUE;
	 else
	 {
	    DelFlake(flake);
	    return FALSE;
	 }
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
		  int p = randint(4);
		  rec.y = j-p+1;
		  rec.width = p;
		  rec.height = p;
		  XUnionRectWithRegion(&rec, SnowOnTreesRegion, SnowOnTreesRegion);
		  cairo_rectangle_int_t grec;
		  grec.x = rec.x;
		  grec.y = rec.y;
		  grec.width = rec.width;
		  grec.height = rec.height;
		  cairo_region_union_rectangle(gSnowOnTreesRegion,&grec);

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
	    // do not erase: this gives bad effects in fvwm-like desktops
	    //EraseSnowFlake(flake);
	    DelFlake(flake);
	    return FALSE;
	 }
      }
   }

   // prevent snow erase on a tree
   // we can skip this when using gtk
   if (!switches.UseGtk)
   {
      int in = XRectInRegion(TreeRegion,x, y, flake ->w, flake->h);
      int b  = (in == RectangleIn || in == RectanglePart); // true if in TreeRegion
      // if(b): erase: no, move: yes
      // erase this flake 
      if(!b) 
      {
	 EraseSnowFlake(flake);
      }
   }
   flake->rx = NewX;
   flake->ry = NewY;
   // prevent drawing a flake on a tree
   // we can skip this when using gtk
   if (switches.UseGtk)
      DrawSnowFlake(flake);
   else
   {
      int in = XRectInRegion(TreeRegion,nx, ny, flake ->w, flake->h);
      int b  = (in == RectangleIn || in == RectanglePart); // true if in TreeRegion
      // if b: draw: no
      if (!b)
      {
	 DrawSnowFlake(flake);
      }
   }
   return TRUE;
}

// creates snowflake from type (0<type<=SNOWFLAKEMAXTYPE)
// if <0, create random type
Snow *MakeFlake(int type)
{
   Snow *flake = (Snow *)malloc(sizeof(Snow)); 
   FlakeCount++; 
   if (type < 0)
      //type = randint(SNOWFLAKEMAXTYPE+1);
      type = randint(NFlakeTypes);
   flake -> whatFlake = type; 
   InitFlake(flake);
   //DrawSnowFlake(flake);
   return flake;
}

void EraseSnowFlake(Snow *flake)
{
   if(switches.UseGtk || Flags.NoSnowFlakes)
      return;
   int x = lrintf(flake->rx);
   int y = lrintf(flake->ry);
   if(switches.Trans|Flags.UseBG)
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
	    switches.Exposures);
}

// a call to this function must be followed by 'return FALSE' to remove this
// flake from the g_timeout callback
void DelFlake(Snow *flake)
{
   set_erase(flake);
   free(flake);
   FlakeCount--;
}


void DrawSnowFlake(Snow *flake) // draw snowflake using flake->rx and flake->ry
{
   if(Flags.NoSnowFlakes) return;
   if (switches.UseGtk)
      return; // will be picked up by snow_draw()
   P("DrawSnowFlake X11 %d\n",counter++);
   int x = lrintf(flake->rx);
   int y = lrintf(flake->ry);
   XSetTSOrigin(display, SnowGC[flake->whatFlake], 
	 x + flake->w, y + flake->h);
   XFillRectangle(display, SnowWin, SnowGC[flake->whatFlake],
	 x, y, flake->w, flake->h);
}

void InitFlake(Snow *flake)
{
   flake->w          = snowPix[flake->whatFlake].width;
   flake->h          = snowPix[flake->whatFlake].height;
   flake->rx         = randint(SnowWinWidth - flake->w);
   flake->ry         = -randint(SnowWinHeight/10);
   flake->cyclic     = 1;
   flake->fluff      = 0;
   flake->flufftimer = 0;
   flake->m          = drand48()+0.1;
   if(Flags.NoWind)
      flake->vx      = 0; 
   else
      flake->vx      = randint(NewWind)/2; 
   flake->ivy        = INITIALYSPEED * sqrt(flake->m);
   flake->vy         = flake->ivy;
   flake->wsens      = drand48()*MAXWSENS;
   flake->testing    = 0;
   set_insert(flake); // will be picked up by snow_draw()
   P("wsens: %f\n",flake->wsens);
   //P("%f %f\n",flake->rx, flake->ry);
}

void InitFlakesPerSecond()
{
   FlakesPerSecond = SnowWinWidth*0.003*Flags.SnowFlakesFactor*
      0.001*FLAKES_PER_SEC_PER_PIXEL*SnowSpeedFactor;
   P("snowflakesfactor: %d %f %f\n",Flags.SnowFlakesFactor,FlakesPerSecond,SnowSpeedFactor);
}

void InitSnowColor()
{
   int i;
   SnowcPix = IAllocNamedColor(Flags.SnowColor, White);   
   //for (i=0; i<=SNOWFLAKEMAXTYPE; i++) 
   for (i=0; i<MaxFlakeTypes; i++) 
      XSetForeground(display, SnowGC[i], SnowcPix);
   init_snow_surfaces();
}

void InitSnowSpeedFactor()
{
   if (Flags.SnowSpeedFactor < 10)
      SnowSpeedFactor = 0.01*10;
   else
      SnowSpeedFactor = 0.01*Flags.SnowSpeedFactor;
   SnowSpeedFactor *= SNOWSPEED;
}


int do_initsnow(UNUSED gpointer data)
{
   P("initsnow %d %d\n",FlakeCount,counter++);
   if (Flags.Done)
      return FALSE;
   // first, kill all snowflakes
   KillFlakes = 1;

   // if FlakeCount != 0, there are still some flakes
   if (FlakeCount > 0)
      return TRUE;

   // signal that flakes may be generated
   KillFlakes = 0;

   return FALSE;  // stop callback
}

int do_show_flakecount(UNUSED gpointer data)
{
   if (Flags.Done)
      return FALSE;
   if (!Flags.NoMenu)
      ui_show_nflakes(FlakeCount);
   return TRUE;
}

