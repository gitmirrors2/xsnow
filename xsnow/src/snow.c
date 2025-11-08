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
*/

#include <pthread.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "xsnow-constants.h"

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
#include "xpm2cairo-gdk.h"

#define NOTACTIVE \
   (Flags.BirdsOnly || !WorkspaceActive() || Flags.NoSnowFlakes)

#define EXTRA_FLAKES 300

#define add_flake_to_mainloop(f) add_to_mainloop1(PRIORITY_HIGH,time_snowflakes,(GSourceFunc)do_UpdateSnowFlake,f)

static float      FlakesPerSecond;
static int        KillFlakes = 0;  // 1: signal to flakes to kill themselves, and do not generate flakes
static float      SnowSpeedFactor;

static SnowMap   *snowPix;
static char    ***xsnow_xpm = NULL;
static int        NFlakeTypesVintage;
static int        MaxFlakeTypes;

static int        do_genflakes(void *);
static void       InitFlake(Snow *flake);
static void       InitFlakesPerSecond(void);
static void       InitSnowColor(void);
static void       InitSnowSpeedFactor(void);
static int        do_show_flakecount(void *);
static void       init_snow_pix(void);
static void       EraseSnowFlake1(Snow *flake);
static void       DelFlake(Snow *flake);
static void       genxpmflake(char ***xpm, int w, int h);
static void       add_random_flakes(int n);
static void       SetSnowSize(void);
static int        do_UpdateSnowFlake(Snow *flake);
static int        do_SwitchFlakes(void *);

static const float  LocalScale = 0.8;


void snow_init()
{
   MaxFlakeTypes = 0;
   while(snow_xpm[MaxFlakeTypes])
      MaxFlakeTypes++;
   NFlakeTypesVintage = MaxFlakeTypes;

   add_random_flakes(EXTRA_FLAKES);   // will change MaxFlakeTypes
				      //                            and create xsnow_xpm, containing
				      //                            vintage and new flakes


   snowPix       = (SnowMap          *)malloc(MaxFlakeTypes*sizeof(SnowMap));
   assert(snowPix);

   P("MaxFlakeTypes: %d\n",MaxFlakeTypes);

   for (int i=0; i<MaxFlakeTypes; i++)
   {
      snowPix[i].surface = NULL;
   }

   //init_snow_surfaces();
   init_snow_pix();
   InitSnowSpeedFactor();
   InitFlakesPerSecond();
   InitSnowColor();
   InitSnowSpeedFactor();
   add_to_mainloop(PRIORITY_DEFAULT, time_genflakes,      do_genflakes       );
   add_to_mainloop(PRIORITY_DEFAULT, time_flakecount,     do_show_flakecount );
   add_to_mainloop(PRIORITY_DEFAULT, time_switchflakes,   do_SwitchFlakes    );

   // now we would like to be able to get rid of the snow xpms:
   /*
      for (int i=0; i<MaxFlakeTypes; i++)
      xpm_destroy(xsnow_xpm[i]);
      free(xsnow_xpm);
      */
   // but we cannot: they are needed if user changes color
}

void SetSnowSize()
{
   add_random_flakes(EXTRA_FLAKES);
   //init_snow_surfaces();
   init_snow_pix();
   if (!global.IsDouble)
      ClearScreen();
}

void snow_ui()
{
   UIDO (NoSnowFlakes, 
	 if(Flags.NoSnowFlakes)
	 ClearScreen();                                            );
   UIDO (UseColor2, 
	 InitSnowColor();
	 ClearScreen();
	);
   UIDO (SnowFlakesFactor               , InitFlakesPerSecond();   );
   UIDOS(SnowColor                      ,
	 InitSnowColor();
	 //InitFallenSnow(); 
	 ClearScreen();                                            );
   UIDOS(SnowColor2                     ,
	 InitSnowColor();
	 //InitFallenSnow(); 
	 ClearScreen();                                            );
   UIDO (SnowSpeedFactor                , InitSnowSpeedFactor();   );
   UIDO (FlakeCountMax                  ,                          );
   UIDO (SnowSize                       , 
	 SetSnowSize(); 
	 Flags.VintageFlakes = 0;                                  );
   P("%d snow_ui\n",global.counter++);

   static int prev=100;
   if(ScaleChanged(&prev))
      init_snow_pix();
}

void init_snow_pix()
{
   (void)LocalScale;
   P("%d init_snow_pix\n",global.counter++);
   for (int flake=0; flake<MaxFlakeTypes; flake++) 
   {
      SnowMap *rp = &snowPix[flake];
      int w,h;
      sscanf(xsnow_xpm[flake][0],"%d %d",&w,&h);
      w *= 0.01*Flags.Scale*LocalScale*global.WindowScale;
      h *= 0.01*Flags.Scale*LocalScale*global.WindowScale;
      rp->width  = w;
      rp->height = h;
      char **x;
      int lines;

      if (Flags.UseColor2)
      {
	 char color[8];
	 mixcolors(Flags.SnowColor, Flags.SnowColor2, drand48(), &color[0]);
	 xpm_set_color(xsnow_xpm[flake], &x, &lines, color);
      }
      else
	 xpm_set_color(xsnow_xpm[flake], &x, &lines, Flags.SnowColor);
      GdkPixbuf *pixbuf = xpm2gdk(global.display, (char **)x, NULL);
      if (w<1) w=1;
      if (h<1) h=1;
      if (w == 1 && h == 1) h = 2;
      GdkPixbuf *pixbufscaled  = gdk_pixbuf_scale_simple(pixbuf,w,h,GDK_INTERP_HYPER);
      xpm_destroy(x);
      if (rp->surface)
	 cairo_surface_destroy(rp->surface);
      rp->surface = gdk_cairo_surface_create_from_pixbuf (pixbufscaled, 0, NULL);
      g_clear_object(&pixbuf);
      g_clear_object(&pixbufscaled);
   }
   global.fluffpix = &snowPix[MaxFlakeTypes-1];
}

int snow_draw(cairo_t *cr)
{
   if (Flags.NoSnowFlakes)
      return TRUE;
   P("snow_draw %d\n",global.counter++);

   set_begin();
   Snow *flake;
   while( (flake = (Snow *)set_next()) )
   {
      P("snow_draw %d %f\n",global.counter++,ALPHA);
      cairo_set_source_surface (cr, snowPix[flake->whatFlake].surface, flake->rx, flake->ry);
      double alpha = ALPHA;
      if (flake->fluff)
	 alpha *= (1-flake->flufftimer/flake->flufftime);
      if (alpha < 0)
	 alpha = 0;
      if (global.IsDouble || !(flake->freeze || flake->fluff))
	 my_cairo_paint_with_alpha(cr,alpha);
      flake->ix = lrint(flake->rx);
      flake->iy = lrint(flake->ry);
   }
   return TRUE;
}

int snow_erase(int force)
{
   if (!force && Flags.NoSnowFlakes)
      return TRUE;
   set_begin();
   Snow *flake;
   int n = 0;
   while( (flake = (Snow *)set_next()) )
   {
      EraseSnowFlake1(flake);
      n++;
   }
   P("snow_erase %d %d\n",global.counter++,n);
   return TRUE;
}

int do_genflakes(void *d)
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
   P("desflakes: %lf %lf %d %lf %d\n",dt,sumdt,desflakes,FlakesPerSecond,global.FlakeCount);
   if(desflakes == 0)  // save dt for use next time: happens with low snowfall rate
      sumdt += dt; 
   else
      sumdt = 0;

   for (int i=0; i<desflakes; i++)
   {
      MakeFlake(-1);
   }
   RETURN;
   (void)d;
#undef RETURN
}

int do_UpdateSnowFlake(Snow *flake)
{
   if(NOTACTIVE)
      return TRUE;
   //P(" ");printflake(flake);

   if ((flake->freeze || flake->fluff)  && global.RemoveFluff)
   {
      P("removefluff\n");
      EraseSnowFlake1(flake);
      DelFlake(flake);
      return FALSE;
   }
   double FlakesDT = time_snowflakes;
   float NewX = flake->rx + (flake->vx*FlakesDT)*SnowSpeedFactor;
   float NewY = flake->ry + (flake->vy*FlakesDT)*SnowSpeedFactor;

   // restore accumulation on fallensnow after some calls
   if (flake->accum == 0)
   {
      const int countmax = 200;
      flake->counter++;
      if (flake->counter > countmax)
      {
	 flake->counter = 0;
	 flake->accum   = 1;
      }
   }
   // handle fluff and KillFlakes
   if (KillFlakes || (flake->fluff && flake->flufftimer > flake->flufftime))
   {
      EraseSnowFlake1(flake);
      DelFlake(flake);
      return FALSE;
   }
   if (flake->fluff)
   {
      if (!flake->freeze)
      {
	 flake->rx = NewX;
	 flake->ry = NewY;
      }
      flake->flufftimer += FlakesDT;
      return TRUE;
   }

   int fckill = global.FlakeCount - global.FluffCount >= Flags.FlakeCountMax;
   if (
	 (fckill && !flake->cyclic && drand48() > 0.3) ||  // high probability to remove blown-off flake
	 (fckill && drand48() > 0.9)                       // low probability to remove other flakes
      )
   {
      fluffify(flake,0.51);
      return TRUE;
   }

   //
   // update speed in x Direction
   //
   if (!Flags.NoWind)
   {
      P("flakevx1: %p %f\n",(void*)flake,flake->vx);
      float f = FlakesDT*flake->wsens/flake->m;
      if (f > 0.9)  f =  0.9;
      if (f < -0.9) f = -0.9;
      flake->vx += f*(global.NewWind - flake->vx);
      static float speedxmaxes[] = {100.0, 300.0, 600.0,};
      float speedxmax = 2*speedxmaxes[global.Wind];
      if(fabs(flake->vx)>speedxmax)
	 P("flakevx:  %p %f %f %f %f %f %f\n",(void*)flake,flake->vx,FlakesDT,flake->wsens,global.NewWind,flake->vx,flake->m);
      if(flake->vx > speedxmax)  flake->vx =  speedxmax;
      if(flake->vx < -speedxmax) flake->vx = -speedxmax;
      P("vxa: %f\n",flake->vx);
   }

   flake->vy += INITIALYSPEED * (drand48()-0.4)*0.1 ;
   if (flake->vy > flake->ivy*1.5) flake->vy = flake->ivy*1.5;

   if (flake->freeze)
   {
      return TRUE;
   }

   int flakew = snowPix[flake->whatFlake].width;
   int flakeh = snowPix[flake->whatFlake].height;

   if(flake->cyclic)
   {
      if (NewX < -flakew)       NewX += global.SnowWinWidth-1;
      if (NewX >= global.SnowWinWidth) NewX -= global.SnowWinWidth;
   }
   else if (NewX < 0 || NewX >= global.SnowWinWidth)
   {
      // not-cyclic flakes die when going left or right out of the window
      DelFlake(flake);
      return FALSE;
   }

   // remove flake if it falls below bottom of screen:
   if (NewY >= global.SnowWinHeight)
   {
      DelFlake(flake);
      return FALSE;
   }

   int nx = lrintf(NewX);
   int ny = lrintf(NewY);

   if (!flake->fluff && flake->accum)
   {
      Lock_fallen();
      // determine if non-fluffy-flake touches the fallen snow,
      // if so: make the flake inactive.
      // the bottom pixels of the snowflake are at y = NewY + (height of flake)
      // the bottompixels are at x values NewX .. NewX+(width of flake)-1

      FallenSnow *fsnow = global.FsnowFirst;
      int found = 0;
      // investigate if flake is in a not-hidden fallensnowarea on current workspace
      while(fsnow && !found)
      {
	 if(!fsnow->win.hidden)
	    //if(fsnow->win.id == 0 ||(fsnow->win.ws == global.CWorkSpace || fsnow->win.sticky))
	    if(fsnow->win.id == 0 ||(IsVisibleFallen(fsnow) || fsnow->win.sticky))
	    {
	       //if (nx >= fsnow->x && nx <= fsnow->x + fsnow->w &&
	       if (nx >= fsnow->x - Flags.SnowSize && nx <= fsnow->x + fsnow->w && // debug
		     ny < fsnow->y+2)
	       {
		  int istart = nx     - fsnow->x;
		  int imax   = istart + flakew;
		  if (istart < 0) istart = 0;
		  if (imax > fsnow->w) imax = fsnow->w;
		  for (int i = istart; i < imax; i++)
		     if (ny > fsnow->y - fsnow->acth[i] - 1)
		     {
			if(fsnow->acth[i] < fsnow->desh[i])
			   UpdateFallenSnowPartial(fsnow,nx - fsnow->x, flakew);
			if(HandleFallenSnow(fsnow))
			{
			   // always erase flake, but repaint it on top of
			   // the correct position on fsnow (if !NoFluffy))
			   if (Flags.NoFluffy)
			   {
			   }
			   else
			   {
			      // x-value: NewX;
			      // y-value of top of fallen snow: fsnow->y - fsnow->acth[i]
			      //flake->rx = NewX;
			      //flake->ry = fsnow->y - 0.5*fsnow->acth[i] - 0.8*drand48()*flakeh ;
			      fluffify(flake,0.1);
			   }
			   if (flake->fluff)
			   {
			      Unlock_fallen();
			      return TRUE;
			   }
			   else
			   {
			      DelFlake(flake);
			      Unlock_fallen();
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
      Unlock_fallen();
   }

   int x  = lrintf(flake->rx);
   int y  = lrintf(flake->ry);

   if(global.Wind !=2  && !Flags.NoKeepSnowOnTrees && !Flags.NoTrees)
   {
      // check if flake is touching or in gSnowOnTreesRegion
      // if so: remove it

      cairo_rectangle_int_t grec = {x,y,flakew,flakeh};
      cairo_region_overlap_t in = cairo_region_contains_rectangle(global.gSnowOnTreesRegion,&grec);

      if (in == CAIRO_REGION_OVERLAP_PART || in ==  CAIRO_REGION_OVERLAP_IN)
      {
	 P("part or in\n");
	 if (Flags.NoFluffy)
	 {
	    DelFlake(flake);
	    return FALSE;
	 }
	 else
	 {
	    fluffify(flake,0.4);
	    flake->freeze=1;
	    return TRUE;
	 }
      }

      // check if flake is touching TreeRegion. If so: add snow to 
      // gSnowOnTreesRegion.
      cairo_rectangle_int_t grect = {x,y,flakew,flakeh};
      in = cairo_region_contains_rectangle(global.TreeRegion,&grect);
      if (in == CAIRO_REGION_OVERLAP_PART)
      {
	 // so, part of the flake is in TreeRegion.
	 // For each bottom pixel of the flake:
	 //   find out if bottompixel is in TreeRegion
	 //   if so:
	 //     move upwards until pixel is not in TreeRegion
	 //     That pixel will be designated as snow-on-tree
	 // Only one snow-on-tree pixel has to be found.
	 int found = 0;
	 int xfound,yfound;
	 for (int i=0; i<flakew; i++)
	 {
	    if(found) break;
	    int ybot = y+flakeh;
	    int xbot = x+i;
	    cairo_rectangle_int_t grect = {xbot,ybot,1,1};
	    cairo_region_overlap_t in = cairo_region_contains_rectangle(global.TreeRegion,&grect);
	    if (in != CAIRO_REGION_OVERLAP_IN) // if bottom pixel not in TreeRegion, skip
	       continue;
	    // move upwards, until pixel is not in TreeRegion
	    for (int j=ybot-1; j >= y; j--)
	    {
	       cairo_rectangle_int_t grect = {xbot,j,1,1};
	       cairo_region_overlap_t in = cairo_region_contains_rectangle(global.TreeRegion,&grect); 
	       if (in != CAIRO_REGION_OVERLAP_IN)
	       {
		  // pixel (xbot,j) is snow-on-tree
		  found = 1;
		  cairo_rectangle_int_t grec;
		  grec.x = xbot;
		  int p = 1+drand48()*3;
		  grec.y = j-p+1;
		  grec.width = p;
		  grec.height = p;
		  cairo_region_union_rectangle(global.gSnowOnTreesRegion,&grec);

		  if(Flags.BlowSnow && global.OnTrees < Flags.MaxOnTrees)
		  {
		     global.SnowOnTrees[global.OnTrees].x = grec.x;
		     global.SnowOnTrees[global.OnTrees].y = grec.y;
		     global.OnTrees++;
		     //P("%d %d %d\n",global.OnTrees,rec.x,rec.y);
		  }
		  xfound = grec.x;
		  yfound = grec.y;
		  break;
	       }
	    }
	 }
	 // do not erase: this gives bad effects in fvwm-like desktops
	 if(found)
	 {
	    flake->freeze = 1;
	    fluffify(flake,0.6);

	    Snow *newflake;
	    if(Flags.VintageFlakes)
	       newflake = MakeFlake(0);
	    else
	       newflake = MakeFlake(-1);
	    newflake->freeze = 1;
	    newflake->rx = xfound;
	    newflake->ry = yfound-snowPix[1].height*0.3f;
	    fluffify(newflake,8);
	    return TRUE;
	 }
      }
   }

   flake->rx = NewX;
   flake->ry = NewY;
   return TRUE;
}

// creates snowflake from type (0<type<=SNOWFLAKEMAXTYPE)
// if <0, create random type
Snow *MakeFlake(int type)
{
   Snow *flake = (Snow *)malloc(sizeof(Snow)); 
   assert(flake);
   global.FlakeCount++; 
   if (type < 0)
   {
      if (Flags.VintageFlakes)
	 type = drand48()*NFlakeTypesVintage;
      else
	 type = NFlakeTypesVintage + drand48()*(MaxFlakeTypes - NFlakeTypesVintage);
   }
   flake -> whatFlake = type; 
   InitFlake(flake);
   add_flake_to_mainloop(flake);
   return flake;
}


void EraseSnowFlake1(Snow *flake)
{
   P("Erasesnowflake1\n");
   if(global.IsDouble)
      return;
   P("Erasesnowflake++++++++\n");
   int x = flake->ix-1;
   int y = flake->iy-1;
   int flakew = snowPix[flake->whatFlake].width+2;
   int flakeh = snowPix[flake->whatFlake].height+2;
   myXClearArea(global.display, global.SnowWin, 
	 x, y,
	 flakew, flakeh,
	 global.xxposures);
}

// a call to this function must be followed by 'return FALSE' to remove this
// flake from the g_timeout callback
void DelFlake(Snow *flake)
{
   if (flake->fluff)
      global.FluffCount--;
   set_erase(flake);
   free(flake);
   global.FlakeCount--;
}

void InitFlake(Snow *flake)
{
   int flakew        = snowPix[flake->whatFlake].width;
   int flakeh        = snowPix[flake->whatFlake].height;
   flake->rx         = randint(global.SnowWinWidth - flakew);
   flake->ry         = -randint(global.SnowWinHeight/10)-flakeh;
   flake->cyclic     = 1;
   flake->fluff      = 0;
   flake->accum      = 1;
   flake->counter    = 0;
   flake->flufftimer = 0;
   flake->flufftime  = 0;
   flake->m          = drand48()+0.1;
   if(Flags.NoWind)
      flake->vx      = 0; 
   else
      flake->vx      = randint(global.NewWind)/2; 
   flake->ivy        = INITIALYSPEED * sqrt(flake->m);
   flake->vy         = flake->ivy;
   flake->wsens      = drand48()*MAXWSENS;
   flake->testing    = 0;
   flake->freeze     = 0;
   set_insert(flake); // will be picked up by snow_draw()
   P("wsens: %f\n",flake->wsens);
}

void InitFlakesPerSecond()
{
   FlakesPerSecond = global.SnowWinWidth*0.0015*Flags.SnowFlakesFactor*
      0.001*FLAKES_PER_SEC_PER_PIXEL*SnowSpeedFactor;
   P("snowflakesfactor: %d %f %f\n",Flags.SnowFlakesFactor,FlakesPerSecond,SnowSpeedFactor);
}

void InitSnowColor()
{
   init_snow_pix();
}

void InitSnowSpeedFactor()
{
   if (Flags.SnowSpeedFactor < 10)
      SnowSpeedFactor = 0.01*10;
   else
      SnowSpeedFactor = 0.01*Flags.SnowSpeedFactor;
   SnowSpeedFactor *= SNOWSPEED;
}


int do_initsnow(void *d)
{
   P("initsnow %d %d\n",global.FlakeCount,counter++);
   if (Flags.Done)
      return FALSE;
   // first, kill all snowflakes
   KillFlakes = 1;

   // if FlakeCount != 0, there are still some flakes
   if (global.FlakeCount > 0)
      return TRUE;

   // signal that flakes may be generated
   KillFlakes = 0;

   return FALSE;  // stop callback
   (void)d;
}

int do_show_flakecount(void *d)
{
   if (Flags.Done)
      return FALSE;
   ui_show_nflakes(global.FlakeCount);
   return TRUE;
   (void)d;
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
   assert(x);
   assert(y);

   float w2 = 0.5*w;
   float h2 = 0.5*h;

   y[0] = 0;
   x[0] = 0;    // to have at least one pixel in the centre
   int n = 1;
   for (int i=0; i<h; i++)
   {
      float yy = i;
      if (yy > h2)
	 yy = h - yy;
      float py = 2*yy/h;
      for (int j=0; j<w; j++)
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
   assert(xa);
   assert(ya);


   for (int i=0; i<n; i++)
   {
      xa[i] = x[i]*cosf(a)-y[i]*sinf(a);
      ya[i] = x[i]*sinf(a)+y[i]*cosf(a);
   }

   float xmin = xa[0];
   float xmax = xa[0];
   float ymin = ya[0];
   float ymax = ya[0];

   for (int i=0; i<n; i++)
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

   assert(nh>0);

   P("allocating %d\n",(nh+3)*sizeof(char*));
   *xpm = (char **)malloc((nh+3)*sizeof(char*));
   assert(*xpm);
   char **X = *xpm;

   X[0] = (char *)malloc(80*sizeof(char));
   snprintf(X[0],80,"%d %d 2 1",nw,nh);

   X[1] = strdup("  c None");
   X[2] = (char *)malloc(80*sizeof(char));
   snprintf(X[2],80,"%c c black",c);

   int offset = 3;
   P("allocating %d\n",nw+1);
   assert(nw>=0);
   for (int i=0; i<nh; i++)
   {
      X[i+offset] = (char *) malloc((nw+1)*sizeof(char));
      assert(X[i+offset]);
   }

   for (int i=0; i<nh; i++)
   {
      for (int j=0; j<nw; j++)
	 X[i+offset][j] = ' ';
      X[i+offset][nw] = 0;
   }

   P("max: %d %f %f %f %f\n",n,ymin,ymax,xmin,xmax);
   for (int i=0; i<n; i++)
   {
      X[offset + (int)(ya[i]-ymin)] [(int)(xa[i]-xmin)] = c;
      P("%f %f\n",ya[i]-ymin,xa[i]-xmin);
   }
   free(x);
   free(y);
   free(xa);
   free(ya);
}

void add_random_flakes(int n)
{
   // create a new array with snow-xpm's:
   if (xsnow_xpm)
   {
      for (int i=0; i<MaxFlakeTypes; i++)
	 xpm_destroy(xsnow_xpm[i]);
      free(xsnow_xpm);
   }
   if(n < 1)
      n = 1;
   char ***x;
   x = (char ***)malloc((n+NFlakeTypesVintage+1)*sizeof(char **));
   int lines;
   // copy Rick's vintage flakes:
   for (int i=0; i<NFlakeTypesVintage; i++)
   {
      xpm_set_color((char **)snow_xpm[i],&x[i],&lines,"snow");
      //xpm_print((char**)snow_xpm[i]);
   }
   // add n flakes:
   for (int i=0; i<n; i++)
   {
      int w,h;
      int m = Flags.SnowSize;
      //w = m+m*drand48();
      //h = m+m*drand48();

      w = m+m*drand48();
      h = m+m*drand48();

      genxpmflake(&x[i+NFlakeTypesVintage],w,h);
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
   global.FluffCount ++;
}

int do_SwitchFlakes(void *d)
{
   (void)d;
   static int prev = 0;
   if (Flags.VintageFlakes != prev)
   {
      P("SwitchFlakes\n");
      set_begin();
      Snow *flake;
      while( (flake = (Snow *)set_next()) )
      {
	 if (Flags.VintageFlakes)
	 {
	    flake->whatFlake = drand48()*NFlakeTypesVintage;
	 }
	 else
	 {
	    flake->whatFlake = NFlakeTypesVintage + drand48()*(MaxFlakeTypes - NFlakeTypesVintage);
	 }
      }
      prev = Flags.VintageFlakes;
   }
   return TRUE;
}

void printflake(Snow *flake)
{
   printf("flake: %p rx: %6.0f ry: %6.0f vx: %6.0f vy: %6.0f ws: %6.0f fluff: %d freeze: %d ftr: %8.3f ft: %8.3f\n",
	 (void *)flake,flake->rx,flake->ry,flake->vx,flake->vy,flake->wsens,flake->fluff,flake->freeze,flake->flufftimer,flake->flufftime);
}
