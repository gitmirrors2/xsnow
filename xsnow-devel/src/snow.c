/* -copyright-
#-# 
#-# xsnow: let it snow on your desktop
#-# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#-# 	      2019,2020 Willem Vermin
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

#define EXTRA_FLAKES 300

static cairo_surface_t **snow_surfaces;
static float             FlakesPerSecond;
static int               KillFlakes = 0;  // 1: signal to flakes to kill themselves, and do not generate flakes
static float             SnowSpeedFactor;
static GC                *ESnowGC;
static GC                *SnowGC;

static SnowMap           *snowPix;
static char            ***xsnow_xpm = NULL;
static int                NFlakeTypesVintage;
static int                MaxFlakeTypes;

static int    do_genflakes(gpointer data);
static void   InitFlake(Snow *flake);
static void   InitFlakesPerSecond(void);
static void   InitSnowColor(void);
static void   InitSnowSpeedFactor(void);
static int    do_show_flakecount(gpointer data);
static void   init_snow_surfaces(void);
static void   init_snow_pix(void);
static void   EraseSnowFlake(Snow *flake);
static void   DelFlake(Snow *flake);
static void   DrawSnowFlake(Snow *flake);
static void   genxpmflake(char ***xpm, int w, int h);
static void   add_random_flakes(int n);


Region       NoSnowArea_dynamic;
Pixel        SnowcPix;
unsigned int MaxSnowFlakeHeight = 0;  /* Highest flake */
unsigned int MaxSnowFlakeWidth  = 0;  /* Widest  flake */
int          FlakeCount         = 0;  /* # active flakes */
int          UseVintageFlakes   = 0;  /* whether to use only vintage flakes */
int          FluffCount         = 0;

void snow_init()
{
   int i;


   MaxFlakeTypes = 0;
   while(snow_xpm[MaxFlakeTypes])
      MaxFlakeTypes++;
   NFlakeTypesVintage = MaxFlakeTypes;

   add_random_flakes(EXTRA_FLAKES);   // will change MaxFlakeTypes
   //                            and create xsnow_xpm, containing
   //                            vintage and new flakes


   snowPix       = (SnowMap          *)malloc(MaxFlakeTypes*sizeof(SnowMap));
   snow_surfaces = (cairo_surface_t **)malloc(MaxFlakeTypes*sizeof(cairo_surface_t*));
   ESnowGC       = (GC               *)malloc(MaxFlakeTypes*sizeof(GC));
   SnowGC        = (GC               *)malloc(MaxFlakeTypes*sizeof(GC));

   P("MaxFlakeTypes: %d\n",MaxFlakeTypes);

   for (i=0; i<MaxFlakeTypes; i++)
      snow_surfaces[i] = NULL;

   for (i=0; i<MaxFlakeTypes; i++) 
   {
      SnowGC[i]         = XCreateGC(display, SnowWin, 0, NULL);
      ESnowGC[i]        = XCreateGC(display, SnowWin, 0, NULL);
      snowPix[i].pixmap = 0;
   }
   init_snow_surfaces();
   init_snow_pix();
   InitSnowSpeedFactor();
   InitFlakesPerSecond();
   InitSnowColor();
   InitSnowSpeedFactor();
   InitBlowOffFactor();
   add_to_mainloop(PRIORITY_DEFAULT, time_genflakes,      do_genflakes          ,NULL);
   add_to_mainloop(PRIORITY_DEFAULT, time_flakecount,     do_show_flakecount    ,NULL);

   // now we would like to be able to get rid of the snow xpms:
   /*
      for (i=0; i<MaxFlakeTypes; i++)
      xpm_destroy(xsnow_xpm[i]);
      free(xsnow_xpm);
      */
   // but we cannot: they are needed if user changes color
}


void snow_set_gc()
{
   P("%d snow_set_gc\n",counter++);
   int i;
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
   if(Flags.SnowSize != OldFlags.SnowSize)
   {
      OldFlags.SnowSize = Flags.SnowSize;
      add_random_flakes(EXTRA_FLAKES);
      init_snow_surfaces();
      init_snow_pix();
      snow_set_gc();
      ClearScreen();
      // the following, otherwize we see often double flakes
      // why? race condition in x server?
      if (!switches.UseGtk)
	 add_to_mainloop(PRIORITY_DEFAULT, 0.1, do_initsnow ,NULL);
      changes++;
      P("changes: %d %d\n",changes,Flags.SnowSize);
   }

   return changes;
}

void init_snow_surfaces()
{
   GdkPixbuf *pixbuf;
   int i;
   for(i=0; i<MaxFlakeTypes; i++)
   {
      P("%d %d\n",counter++,i);
      char **x;
      int lines;
      xpm_set_color(xsnow_xpm[i], &x, &lines, Flags.SnowColor);
      pixbuf = gdk_pixbuf_new_from_xpm_data((const char **)x);
      xpm_destroy(x);
      if (snow_surfaces[i])
	 cairo_surface_destroy(snow_surfaces[i]);
      snow_surfaces[i] = gdk_cairo_surface_create_from_pixbuf (pixbuf, 0, gdkwindow);
      //snow_surfaces[i] = gdk_cairo_surface_create_from_pixbuf (pixbuf, 1, NULL);
      g_clear_object(&pixbuf);
      /*
       * another method:
       surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
       gdk_pixbuf_get_width (pixbuf),
       gdk_pixbuf_get_height (pixbuf));
       cr = cairo_create (surface);
       gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
       cairo_paint (cr);
       cairo_destroy (cr);
       */
   }
}

void init_snow_pix()
{
   int flake;
   P("%d init_snow_pix\n",counter++);
   for (flake=0; flake<MaxFlakeTypes; flake++) 
   {
      SnowMap *rp = &snowPix[flake];
      // get snowbits from xsnow_xpm[flake]
      unsigned char *bits;
      int w,h,l;
      xpmtobits(xsnow_xpm[flake],&bits,&w,&h,&l);
      rp->width  = w;
      rp->height = h;
      if(rp->pixmap)
	 XFreePixmap(display,rp->pixmap);
      /*
       * running under valgrind: XCreateBitmapFromData() often gives 
       * Syscall param writev(vector[...]) points to uninitialised byte(s)
       * and
       * Address 0xc60f5b1 is 1 bytes inside a block of size 36 alloc'd
       * at 0x483B7F3: malloc (in /usr/lib/x86_64-linux-gnu/valgrind/vgpreload_memcheck-amd64-linux.so)
       * by 0x581C3FC: _XAllocScratch (in /usr/lib/x86_64-linux-gnu/libX11.so.6.3.0)
       * by 0x580E30A: ??? (in /usr/lib/x86_64-linux-gnu/libX11.so.6.3.0)
       * by 0x580EAAD: XPutImage (in /usr/lib/x86_64-linux-gnu/libX11.so.6.3.0)
       * by 0x57F715B: XCreateBitmapFromData (in /usr/lib/x86_64-linux-gnu/libX11.so.6.3.0)
       * 
       * bits is initialised, I tested that. So, I guess there is 
       * something not cosher in XCreateBitmapFromData() maybe....
       */
      rp->pixmap = XCreateBitmapFromData(display, SnowWin,
	    (const char*)bits, rp->width, rp->height);
      if (rp->height > MaxSnowFlakeHeight) MaxSnowFlakeHeight = rp->height;
      if (rp->width  > MaxSnowFlakeWidth ) MaxSnowFlakeWidth  = rp->width;
      free(bits);
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
	 //alpha *= flake->flufftimer>0?flake->flufftimer/FLUFFTIME:0;
	 alpha *= (1-flake->flufftimer/flake->flufftime);
      if (alpha < 0)
	 alpha = 0;
      my_cairo_paint_with_alpha(cr,alpha);
      //if (flake->testing) P("testing flake: alpha: %f\n",alpha);
      //if (flake->whatFlake == 2) P("%d %d %d %f %f %f %f %f %f\n",counter++,flake->fluff,flake->freeze,flake->rx, flake->ry, flake->flufftimer, flake->flufftime,flake->vx, flake->vy);
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
      MakeFlake(-1);
   }
   RETURN;
#undef RETURN
}

int do_UpdateSnowFlake(Snow *flake)
{
   if(NOTACTIVE)
      return TRUE;
   //P(" ");printflake(flake);
   
   double FlakesDT = time_snowflakes;
   float NewX = flake->rx + (flake->vx*FlakesDT)*SnowSpeedFactor;
   float NewY = flake->ry + (flake->vy*FlakesDT)*SnowSpeedFactor;

   // handle fluff and KillFlakes
   //if (flake->testing == 2)P("%d hoppa %d %f\n",counter++,flake->fluff,flake->flufftimer);
   if (KillFlakes || (flake->fluff && flake->flufftimer > flake->flufftime))
   {
      EraseSnowFlake(flake);
      DelFlake(flake);
      return FALSE;
   }
   if (flake->fluff)
   {
      if (switches.UseGtk && !flake->freeze)
      {
	 flake->rx = NewX;
	 flake->ry = NewY;
      }
      flake->flufftimer += FlakesDT;
      return TRUE;
   }

   int fckill = FlakeCount - FluffCount >= Flags.FlakeCountMax;
   if (
	 (fckill && !flake->cyclic && drand48() > 0.3) ||  // high probability to remove blown-off flake
	 (fckill && drand48() > 0.9)                       // low probability to remove other flakes
      )
   {
      //if (flake->fluff)P("%d hoppa %d %d %f\n",counter++,flake->fluff,flake->whatFlake,flake->flufftimer);
      /*
	 EraseSnowFlake(flake);
	 DelFlake(flake);
	 return FALSE;
	 */
      fluffify(flake,0.51);
      return TRUE;
   }

   //
   // update speed in x Direction
   //
   if (!Flags.NoWind)
   {
      flake->vx += FlakesDT*flake->wsens*(NewWind - flake->vx)/flake->m;
      static float speedxmaxes[] = {100.0, 300.0, 600.0,};
      float speedxmax = speedxmaxes[Wind];
      if(flake->vx > speedxmax) flake->vx = speedxmax;
      if(flake->vx < -speedxmax) flake->vx = -speedxmax;
   }

   flake->vy += INITIALYSPEED * (drand48()-0.4)*0.1 ;
   if (flake->vy > flake->ivy*1.5) flake->vy = flake->ivy*1.5;



   if (flake->freeze)
   {
      DrawSnowFlake(flake);
      return TRUE;
   }

   int flakew = snowPix[flake->whatFlake].width;
   int flakeh = snowPix[flake->whatFlake].height;

   if(flake->cyclic)
   {
      if (NewX < -flakew)       NewX += SnowWinWidth-1;
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

   if (!flake->fluff)
   {
      // determine if non-fluffy-flake touches the fallen snow,
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
		  int imax   = istart + flakew;
		  if (istart < 0) istart = 0;
		  if (imax > fsnow->w) imax = fsnow->w;
		  for (i = istart; i < imax; i++)
		     if (ny > fsnow->y - fsnow->acth[i] - 1)
		     {
			if(fsnow->acth[i] < fsnow->h/2)
			   UpdateFallenSnowPartial(fsnow,nx - fsnow->x, flakew);
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
			      flake->ry = fsnow->y - fsnow->acth[i] - 0.8*drand48()*flakeh;
			      DrawSnowFlake(flake);
			      fluffify(flake,0.1);
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
   }

   int x  = lrintf(flake->rx);
   int y  = lrintf(flake->ry);

   // check if flake is in nowsnowarea
   if (!switches.UseGtk)  // we can skip this when using gtk
   {
      int in = XRectInRegion(NoSnowArea_dynamic,x, y, flakew, flakeh);
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
      int in = XRectInRegion(SnowOnTreesRegion,x,y,flakew,flakeh);
      if (in == RectanglePart || in == RectangleIn)
      {
	 if (Flags.NoFluffy)
	 {
	    DelFlake(flake);
	    return FALSE;
	 }
	 else
	 {
	    fluffify(flake,0.4);
	    flake->freeze=1;
	    DrawSnowFlake(flake);
	    return TRUE;
	 }
      }

      // check if flake is touching TreeRegion. If so: add snow to 
      // SnowOnTreesRegion.
      in = XRectInRegion(TreeRegion,x,y,flakew,flakeh);
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
	 int xfound,yfound;
	 for(i=0; i<flakew; i++)
	 {
	    if(found) break;
	    int ybot = y+flakeh;
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
		  int p = 1+drand48()*3;
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
		  xfound = rec.x;
		  yfound = rec.y;
		  break;
	       }
	    }
	 }
	 // do not erase: this gives bad effects in fvwm-like desktops
	 if(found)
	 {
	    flake->freeze = 1;
	    fluffify(flake,0.6);
	    DrawSnowFlake(flake);

	    Snow *newflake = MakeFlake(1);
	    newflake->freeze = 1;
	    newflake->rx = xfound;
	    newflake->ry = yfound-snowPix[1].height*0.3f;
	    fluffify(newflake,8);
	    DrawSnowFlake(newflake);
	    return TRUE;
	 }
      }
   }

   // prevent snow erase on a tree
   // we can skip this when using gtk
   if (!switches.UseGtk)
   {
      int in = XRectInRegion(TreeRegion,x, y, flakew, flakeh);
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
   if (!switches.UseGtk)
   {
      int in = XRectInRegion(TreeRegion,nx, ny, flakew, flakeh);
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
   {
      if (UseVintageFlakes)
	 type = drand48()*NFlakeTypesVintage;
      else
	 type = NFlakeTypesVintage + drand48()*(MaxFlakeTypes - NFlakeTypesVintage);
   }
   //if(type > 0 && type <7)P("type: %d\n",type);
   flake -> whatFlake = type; 
   InitFlake(flake);
   add_flake_to_mainloop(flake);
   return flake;
}

void EraseSnowFlake(Snow *flake)
{
   if(switches.UseGtk || Flags.NoSnowFlakes)
      return;
   int x = lrintf(flake->rx);
   int y = lrintf(flake->ry);
   int flakew = snowPix[flake->whatFlake].width;
   int flakeh = snowPix[flake->whatFlake].height;
   if(switches.Trans|Flags.UseBG)
   {
      XSetTSOrigin(display, ESnowGC[flake->whatFlake], 
	    x + flakew, y + flakeh);
      XFillRectangle(display, SnowWin, ESnowGC[flake->whatFlake],
	    x, y, flakew, flakeh);
   }
   else
      XClearArea(display, SnowWin, 
	    x, y,
	    flakew, flakeh,
	    switches.Exposures);
}

// a call to this function must be followed by 'return FALSE' to remove this
// flake from the g_timeout callback
void DelFlake(Snow *flake)
{
   if (flake->fluff)
      FluffCount--;
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
   int flakew = snowPix[flake->whatFlake].width;
   int flakeh = snowPix[flake->whatFlake].height;
   XSetTSOrigin(display, SnowGC[flake->whatFlake], 
	 x + flakew, y + flakeh);
   XFillRectangle(display, SnowWin, SnowGC[flake->whatFlake],
	 x, y, flakew, flakeh);
}

void InitFlake(Snow *flake)
{
   int flakew        = snowPix[flake->whatFlake].width;
   int flakeh        = snowPix[flake->whatFlake].height;
   flake->rx         = randint(SnowWinWidth - flakew);
   flake->ry         = -randint(SnowWinHeight/10)-flakeh;
   flake->cyclic     = 1;
   flake->fluff      = 0;
   flake->flufftimer = 0;
   flake->flufftime  = 0;
   flake->m          = drand48()+0.1;
   if(Flags.NoWind)
      flake->vx      = 0; 
   else
      flake->vx      = randint(NewWind)/2; 
   flake->ivy        = INITIALYSPEED * sqrt(flake->m);
   flake->vy         = flake->ivy;
   flake->wsens      = drand48()*MAXWSENS;
   flake->testing    = 0;
   flake->freeze     = 0;
   set_insert(flake); // will be picked up by snow_draw()
   P("wsens: %f\n",flake->wsens);
   //P("%f %f\n",flake->rx, flake->ry);
}

void InitFlakesPerSecond()
{
   FlakesPerSecond = SnowWinWidth*0.0015*Flags.SnowFlakesFactor*
      0.001*FLAKES_PER_SEC_PER_PIXEL*SnowSpeedFactor;
   P("snowflakesfactor: %d %f %f\n",Flags.SnowFlakesFactor,FlakesPerSecond,SnowSpeedFactor);
}

void InitSnowColor()
{
   int i;
   SnowcPix = IAllocNamedColor(Flags.SnowColor, White);   
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

// generate random xpm for flake with dimensions wxh
// the flake will be rotated, so the w and h of the resulting xpm will
// be different from the input w and h.
void genxpmflake(char ***xpm, int w, int h)
{
   P("%d genxpmflake %d %d\n",counter++,w,h);
   const char c='.'; // imposed by xpm_set_color
   int nmax = w*h;
   float *x, *y;

   x = (float *)malloc(nmax*sizeof(float));
   y = (float *)malloc(nmax*sizeof(float));

   int i,j;
   float w2 = 0.5*w;
   float h2 = 0.5*h;

   y[0] = -w2;
   x[0] = -h2;    // to have at least one pixel in the centre
   int n = 1;
   for (i=0; i<h; i++)
   {
      float yy = i;
      if (yy > h2)
	 yy = h - yy;
      float py = 2*yy/h;
      for (j=0; j<w; j++)
      {
	 float xx = j;
	 if (xx > w2)
	    xx = w - xx;
	 float px = 2*xx/w;
	 float p = 1.1-(px*py);
	 //printf("%d %d %f %f %f %f %f\n",j,i,y,x,px,py,p);
	 if (drand48() > p )
	 {
	    if (n<nmax)
	    {
	       y[n] = i - w2;
	       x[n] = j - h2;
	       n++;
	    }
	 }
      }
   }
   // rotate points with a random angle 0 .. pi
   float a = drand48()*355.0/113.0;
   float *xa, *ya;
   xa = (float *)malloc(n*sizeof(float));
   ya = (float *)malloc(n*sizeof(float));


   for (i=0; i<n; i++)
   {
      xa[i] = x[i]*cosf(a)-y[i]*sinf(a);
      ya[i] = x[i]*sinf(a)+y[i]*cosf(a);
   }

   float xmin = xa[0];
   float xmax = xa[0];
   float ymin = ya[0];
   float ymax = ya[0];

   for (i=0; i<n; i++)
   {
      // smallest xa:
      if (xa[i] < xmin)
	 xmin = xa[i];
      // etc ..
      if (xa[i] > xmax)
	 xmax = xa[i];
      if (ya[i] < ymin)
	 ymin = ya[i];
      if (ya[i] > ymax)
	 ymax = ya[i];
   }

   int nw = ceilf(xmax - xmin + 1);
   int nh = ceilf(ymax - ymin + 1);

   // for some reason, drawing of cairo_surfaces derived from 1x1 xpm slow down
   // the x server terribly. So, to be sure, I demand that none of
   // the dimensions is 1, and that the width is a multiple of 8.
   // Btw: genxpmflake rotates and compresses the original wxh xpm, 
   // and sometimes that results in an xpm with both dimensions one.

   // Now, suddenly, nw doesn't seem to matter any more?
   //nw = ((nw-1)/8+1)*8;
   //nw = ((nw-1)/32+1)*32;
   P("%d nw nh: %d %d\n",counter++,nw,nh);
   // Ah! nh should be 2 at least ...
   // nw is not important
   //
   // Ok, I tried some things, and it seems that if both nw and nh are 1,
   // then we have trouble.
   // Some pages on the www point in the same direction.

   if (nw == 1 && nh == 1)
      nh = 2;


   P("allocating %d\n",(nh+3)*sizeof(char*));
   *xpm = (char **)malloc((nh+3)*sizeof(char*));
   char **X = *xpm;

   X[0] = (char *)malloc(80*sizeof(char));
   snprintf(X[0],80,"%d %d 2 1",nw,nh);

   X[1] = strdup("  c None");
   X[2] = (char *)malloc(80*sizeof(char));
   snprintf(X[2],80,"%c c black",c);

   int offset = 3;
   P("allocating %d\n",nw+1);
   for (i=0; i<nh; i++)
      X[i+offset] = (char *) malloc((nw+1)*sizeof(char));

   for (i=0; i<nh; i++)
   {
      for(j=0; j<nw; j++)
	 X[i+offset][j] = ' ';
      X[i+offset][nw] = 0;
   }

   P("max: %d %f %f %f %f\n",n,ymin,ymax,xmin,xmax);
   for (i=0; i<n; i++)
   {
      X[offset + (int)(ya[i]-ymin)] [(int)(xa[i]-xmin)] = c;
      P("%f %f\n",ya[i]-ymin,xa[i]-xmin);
   }
}

void add_random_flakes(int n)
{
   int i;
   // create a new array with snow-xpm's:
   if (xsnow_xpm)
   {
      for (i=0; i<MaxFlakeTypes; i++)
	 xpm_destroy(xsnow_xpm[i]);
      free(xsnow_xpm);
   }
   if(n < 1)
      n = 1;
   char ***x;
   x = (char ***)malloc((n+NFlakeTypesVintage+1)*sizeof(char **));
   int lines;
   // copy Rick's vintage flakes:
   for (i=0; i<NFlakeTypesVintage; i++)
   {
      xpm_set_color((char **)snow_xpm[i],&x[i],&lines,"snow");
      //xpm_print((char**)snow_xpm[i]);
   }
   // add n flakes:
   for (i=0; i<n; i++)
   {
      int w,h;
      int m = Flags.SnowSize;
      w = m+m*drand48();
      h = m+m*drand48();
      genxpmflake(&x[i+NFlakeTypesVintage],w,h);
      //xpm_print(x[i+NFlakeTypesVintage]);
   }
   MaxFlakeTypes   = n + NFlakeTypesVintage;
   x[MaxFlakeTypes] = NULL;
   xsnow_xpm = x;
}

void fluffify(Snow *flake,float t)
{
   if (flake->fluff)
      return;
   flake->fluff      = 1;
   flake->flufftimer = 0;
   if (t > 0.01)
      flake->flufftime  = t;
   else
      flake->flufftime = 0.01;
   FluffCount ++;
}

void printflake(Snow *flake)
{
   printf("flake: %p rx: %6.0f ry: %6.0f fluff: %d freeze: %d ftr: %8.3f ft: %8.3f\n",
	 (void *)flake,flake->rx,flake->ry,flake->fluff,flake->freeze,flake->flufftimer,flake->flufftime);
}
