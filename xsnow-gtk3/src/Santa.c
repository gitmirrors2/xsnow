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
#include <math.h>
#include <X11/Intrinsic.h>
#include "Santa.h"
#include "pixmaps.h"
#include "debug.h"
#include "windows.h"
#include "flags.h"
#include "wind.h"

#define add_to_mainloop(prio,time,func,datap) g_timeout_add_full(prio,(int)1000*(time),(GSourceFunc)func,datap,0)
#define NOTACTIVE \
   (Flags.BirdsOnly || !WorkspaceActive())
static void init_Santa_surfaces(void);
static int do_usanta(void);
static Region RegionCreateRectangle(int x, int y, int w, int h);
static int    SantaYStep;
static Region SantaRegion;
static int    do_santa(void);
static int    do_santa1(void);
static void   DrawSanta1(void);
static void   DrawSanta(void);
static GC     ESantaGC;
static GC     SantaGC;

cairo_surface_t *Santa_surfaces[MAXSANTA+1][2][PIXINANIMATION];

int    SantaX;   // should always be lrintf(SantaYr)
int    SantaY;   // should always be lrintf(SantaYr)
float  ActualSantaSpeed;
float  SantaSpeed;  
float  SantaXr;
float  SantaYr;
int    SantaHeight;   
int    SantaWidth;
int    CurrentSanta;
Region SantaPlowRegion;
int    OldSantaX = 0;  // the x value of Santa when he was last drawn
int    OldSantaY = 0;  // the y value of Santa when he was last drawn


int Santa_draw(cairo_t *cr)
{
   cairo_surface_t *surface;
   surface = Santa_surfaces[Flags.SantaSize][!Flags.NoRudolf][CurrentSanta];
   cairo_set_source_surface (cr, surface, SantaX, SantaY);
   cairo_paint(cr);
   return TRUE;
}

void Santa_init()
{
   ESantaGC             = XCreateGC(display, SnowWin, 0, 0);
   SantaGC              = XCreateGC(display, SnowWin, 0, 0);

   SantaRegion          = XCreateRegion();
   SantaPlowRegion      = XCreateRegion();
   init_Santa_surfaces();
   add_to_mainloop(PRIORITY_HIGH,    time_usanta,         do_usanta             ,0);
}

void Santa_set_gc()
{
   XSetFunction(display,   SantaGC, GXcopy);
   XSetForeground(display, SantaGC, BlackPix);
   XSetFillStyle(display,  SantaGC, FillStippled);

   XSetFunction(display,   ESantaGC, GXcopy);
   XSetFillStyle(display,  ESantaGC, FillSolid);
   XSetForeground(display, ESantaGC, ErasePixel);
}

void init_Santa_surfaces()
{
   GdkPixbuf *pixbuf;
   int i,j,k;
   for(i=0; i<MAXSANTA+1; i++)
      for (j=0; j<2; j++)
	 for (k=0; k<PIXINANIMATION; k++)
	 {
	    R("%d %d %d\n",i,j,k);
	    pixbuf = gdk_pixbuf_new_from_xpm_data((const char **)Santas[i][j][k]);
	    Santa_surfaces[i][j][k] = gdk_cairo_surface_create_from_pixbuf (pixbuf, 0, gdkwindow);
	    g_clear_object(&pixbuf);
	 }
}

void Santa_HandleFactor()
{
   static guint santa_id=0, santa1_id=0;
   // re-add things whose timing is dependent on factor
   if (Flags.CpuLoad <= 0)
      factor = 1;
   else
      factor = 100.0/Flags.CpuLoad;

   if (santa_id)
      g_source_remove(santa_id);
   if (santa1_id)
      g_source_remove(santa1_id);

   santa_id  = add_to_mainloop(PRIORITY_DEFAULT, time_santa,  do_santa,  0);
   santa1_id = add_to_mainloop(PRIORITY_HIGH,    time_santa1, do_santa1, 0);
}

int do_santa()
{
   if (Flags.Done)
      return FALSE;
   if (NOTACTIVE)
      return TRUE;
   if (!Flags.NoSanta)
      DrawSanta();
   return TRUE;
}
int do_santa1()
{
   if (Flags.Done)
      return FALSE;
   if (NOTACTIVE)
      return TRUE;
   if (!Flags.NoSanta)
      DrawSanta1();
   return TRUE;
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
// update santa's coordinates and speed
int do_usanta()
{
   if (Flags.Done)
      return FALSE;
#define RETURN do { return TRUE ; } while(0)
   if (NOTACTIVE)
      RETURN;
   if(Flags.NoSanta)
      RETURN;
   double         yspeed;
   static int yspeeddir  = 0;
   static double sdt     = 0;
   static double dtt     = 0;

   int oldx = SantaX;
   int oldy = SantaY;

   double dt = time_usanta;
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
   dtt += dt;
   if (dtt > 0.1 && fabs(ActualSantaSpeed) > 3)
   {
      dtt = 0;
      CurrentSanta++;
      if (CurrentSanta >= PIXINANIMATION) CurrentSanta = 0;
   }

   yspeed = ActualSantaSpeed/4;
   sdt += dt;
   if (sdt > 2.0)
   {
      // time to change yspeed
      sdt = 0;
      yspeeddir = drand48()*3-1;  //  -1, 0, 1
   }

   SantaYr += dt*yspeed*yspeeddir;
   if (SantaYr < 0)
      SantaYr = 0;

   if (SantaYr > SnowWinHeight*0.33)
      SantaYr = SnowWinHeight*0.33;

   SantaY = lrintf(SantaYr);
   XOffsetRegion(SantaRegion, SantaX - oldx, SantaY - oldy);
   XOffsetRegion(SantaPlowRegion, SantaX - oldx, SantaY - oldy);

   RETURN;
}

void ResetSanta()      
{
   SantaX  = -SantaWidth - ActualSantaSpeed;
   SantaXr = SantaX;
   SantaY  = drand48()*(SnowWinHeight / 3)+40;
   SantaYr = SantaY;
   SantaYStep = 1;
   CurrentSanta = 0;
   XDestroyRegion(SantaRegion);
   SantaRegion = RegionCreateRectangle(
	 SantaX,SantaY,SantaHeight,SantaWidth);

   XDestroyRegion(SantaPlowRegion);
   SantaPlowRegion = RegionCreateRectangle(
	 SantaX + SantaWidth, SantaY, 1, SantaHeight);
}

Region RegionCreateRectangle(int x, int y, int w, int h)
{
   XPoint p[5];
   //p[0] = (XPoint){x  ,        y  };
   p[0].x =          x; p[0].y = y;

   //p[1] = (XPoint){x+w,          y  };
   p[1].x =          x+w; p[1].y = y;

   //p[2] = (XPoint){x+w,          y+h};
   p[2].x =          x+w; p[2].y = y+h;

   //p[3] = (XPoint){x  ,        y+h}; 
   p[3].x =          x; p[3].y = y+h;

   //p[4] = (XPoint){x  ,        y  };
   p[4].x =          x; p[4].y = y;
   return XPolygonRegion(p, 5, EvenOddRule);
}
