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

#include <stdio.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <math.h>
#include <X11/Intrinsic.h>
#include <X11/xpm.h>
#include "Santa.h"
#include "pixmaps.h"
#include "debug.h"
#include "windows.h"
#include "flags.h"
#include "utils.h"
#include "wind.h"
#include "ixpm.h"
#include "moon.h"
#include "varia.h"

#define NOTACTIVE \
   (Flags.BirdsOnly || !WorkspaceActive())
static int    do_santa(gpointer data);
static int    do_santa1(gpointer data);
static int    do_usanta(gpointer data);
static void   EraseSanta(int x, int y);
static void   DrawSanta(void);
static void   DrawSanta1(void);
static void   InitSantaPixmaps(void);
static void   init_Santa_surfaces(void);
static Region RegionCreateRectangle(int x, int y, int w, int h);
static void   ResetSanta(void);
static void   SetSantaSpeed(void);
static void   SetSantaType(void);

static int    CurrentSanta;
static GC     ESantaGC = NULL;
static int    OldSantaX = 0;  // the x value of Santa when he was last drawn
static int    OldSantaY = 0;  // the y value of Santa when he was last drawn
static GC     SantaGC = NULL;
static Pixmap SantaMaskPixmap[PIXINANIMATION];
static Pixmap SantaPixmap[PIXINANIMATION];
static Region SantaRegion;
static float  SantaSpeed;  
static float  SantaXr;
static float  SantaYr;
static int    SantaYStep;

float  ActualSantaSpeed;
Region SantaPlowRegion;
int    SantaHeight;   
int    SantaWidth;
int    SantaX;   // should always be lrintf(SantaYr)
int    SantaY;   // should always be lrintf(SantaYr)

static cairo_surface_t *Santa_surfaces[MAXSANTA+1][2][PIXINANIMATION];

/* Speed for each Santa  in pixels/second*/
static float Speed[] = {SANTASPEED0,  /* Santa 0 */
   SANTASPEED1,  /* Santa 1 */
   SANTASPEED2,  /* Santa 2 */
   SANTASPEED3,  /* Santa 3 */
   SANTASPEED4,  /* Santa 4 */
};

void SetSantaType()
{
   EraseSanta(OldSantaX,OldSantaY); 
   InitSantaPixmaps();
}

void Santa_ui()
{
   UIDO(SantaSize, SetSantaType(););
   UIDO(Rudolf,    SetSantaType(););
   UIDO(NoSanta,if(Flags.NoSanta)
	 EraseSanta(OldSantaX, OldSantaY););
   UIDO(SantaSpeedFactor, SetSantaSpeed(););
}

int Santa_draw(cairo_t *cr)
{
   P("Santa_draw %d\n",counter++);
   if (Flags.NoSanta)
      return TRUE;
   cairo_surface_t *surface;
   surface = Santa_surfaces[Flags.SantaSize][Flags.Rudolf][CurrentSanta];
   cairo_set_source_surface (cr, surface, SantaX, SantaY);
   my_cairo_paint_with_alpha(cr,ALPHA);
   return TRUE;
}

void Santa_init()
{
   P("Santa_init\n");
   int i;
   for (i=0; i<PIXINANIMATION; i++)
   {
      SantaPixmap[i]     = 0;
      SantaMaskPixmap[i] = 0;
   }
   InitSantaPixmaps();

   ESantaGC             = XCreateGC(display, SnowWin, 0, NULL);
   SantaGC              = XCreateGC(display, SnowWin, 0, NULL);
   SantaRegion          = XCreateRegion();
   SantaPlowRegion      = XCreateRegion();
   init_Santa_surfaces();
   ResetSanta();   
   add_to_mainloop(PRIORITY_HIGH,    time_usanta,         do_usanta             ,NULL);
}


void Santa_set_gc()
{
   P("Santa_set_gc SantaGC: %p SnowWin: %#lx\n",(void*)SantaGC,SnowWin);
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
	    P("%d %d %d\n",i,j,k);
	    pixbuf = gdk_pixbuf_new_from_xpm_data((const char **)Santas[i][j][k]);
	    Santa_surfaces[i][j][k] = gdk_cairo_surface_create_from_pixbuf (pixbuf, 0, NULL);
	    g_clear_object(&pixbuf);
	 }
   int ok = 1;
   char *path[PIXINANIMATION];
   const char *filenames[] = 
   {
      "xsnow/pixmaps/santa1.xpm",
      "xsnow/pixmaps/santa2.xpm",
      "xsnow/pixmaps/santa3.xpm",
      "xsnow/pixmaps/santa4.xpm",
   };
   for (i=0; i<PIXINANIMATION; i++)
   {
      path[i] = NULL;
      FILE *f = HomeOpen(filenames[i],"r",&path[i]);
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
	 rc = XpmReadFileToData(path[i],&santaxpm);
	 if(rc == XpmSuccess)
	 {
	    pixbuf = gdk_pixbuf_new_from_xpm_data((const char **)santaxpm);
	    cairo_surface_destroy( Santa_surfaces[0][0][i]);
	    Santa_surfaces[0][0][i] = gdk_cairo_surface_create_from_pixbuf(pixbuf,0,NULL);
	    XpmFree(santaxpm);
	    g_clear_object(&pixbuf);
	 }
	 else
	 {
	    printf("Invalid external xpm for Santa given: %s\n",path[i]);
	    exit(1);
	 }
	 free(path[i]);
      }
      Flags.SantaSize = 0;
      Flags.Rudolf    = 0;
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
   attributes.valuemask = XpmDepth /*| XpmColorKey*/;
   attributes.depth = SnowWinDepth;

   SetSantaSpeed();

   char *path[PIXINANIMATION];
   const char *filenames[] = 
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
      path[i] = NULL;
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
	    iXpmCreatePixmapFromData(display, SnowWin, (const char **)santaxpm, 
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
      return;
   }


   int rc[PIXINANIMATION];
   int withRudolf;
   withRudolf = Flags.Rudolf;

   for(i=0; i<PIXINANIMATION; i++)
   {
      if(SantaPixmap[i]) 
	 XFreePixmap(display,SantaPixmap[i]);
      if(SantaMaskPixmap[i]) 
	 XFreePixmap(display,SantaMaskPixmap[i]);
      rc[i] = iXpmCreatePixmapFromData(display, SnowWin, 
	    (const char **)Santas[Flags.SantaSize][withRudolf][i], 
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
}		

void Santa_HandleCpuFactor()
{
   static guint santa_id=0, santa1_id=0;

   if (santa_id)
      g_source_remove(santa_id);
   if (santa1_id)
      g_source_remove(santa1_id);

   santa_id  = add_to_mainloop(PRIORITY_DEFAULT, time_santa,  do_santa,  NULL);
   santa1_id = add_to_mainloop(PRIORITY_HIGH,    time_santa1, do_santa1, NULL);
}

int do_santa(UNUSED gpointer data)
{
   if (Flags.Done)
      return FALSE;
   if (NOTACTIVE)
      return TRUE;
   if (switches.UseGtk)
      return TRUE;
   if (!Flags.NoSanta)
      DrawSanta();
   return TRUE;
}

int do_santa1(UNUSED gpointer data)
{
   if (Flags.Done)
      return FALSE;
   if (NOTACTIVE)
      return TRUE;
   if (switches.UseGtk)
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
   if (switches.UseGtk)
      return;
   if(switches.Trans|Flags.UseBG)
      XFillRectangle(display, SnowWin, ESantaGC, x,y,SantaWidth+1,SantaHeight);
   // probably due to rounding errors in computing SantaX, one pixel in front 
   // is not erased when leaving out the +1
   else
      XClearArea(display, SnowWin,
	    x , y,     
	    SantaWidth+1,SantaHeight,
	    switches.Exposures);
}

void DrawSanta1()
{
   P("DrawSanta1 %#lx %d\n",SnowWin,counter++);
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
int do_usanta(UNUSED gpointer data)
{
   P("do_usanta %d\n",counter++);
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

   double santayrmin = 0;
   double santayrmax = SnowWinHeight*0.33;

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
   if (sdt > 2.0*50.0/SantaSpeed || sdt > 2.0)
   {
      // time to change yspeed
      sdt = 0;
      yspeeddir = randint(3)-1;  //  -1, 0, 1
      if (SantaYr < santayrmin + 20)
	 yspeeddir = 2;

      if (SantaYr > santayrmax - 20)
	 yspeeddir = -2;
      //int mooncx = moonX+Flags.MoonSize/2;
      int mooncy = moonY+Flags.MoonSize/2;
      P("DrawBirds:%d\n",switches.DrawBirds);
      if (switches.DrawBirds && Flags.Moon && SantaX+SantaWidth < moonX+Flags.MoonSize && SantaX+SantaWidth > moonX-300) // Santa likes to hover the moon
      {
	 int dy = SantaY+SantaHeight/2 - mooncy;
	 if (dy < 0)
	    yspeeddir = 1;
	 else
	    yspeeddir = -1;
	 if (dy < -Flags.MoonSize/2)
	    yspeeddir = 3;
	 else if (dy > Flags.MoonSize/2)
	    yspeeddir = -3;
	 P("moon seeking %f %f %d %f\n",SantaYr, moonY, yspeeddir,SantaSpeed);
      }
   }

   SantaYr += dt*yspeed*yspeeddir;
   if (SantaYr < santayrmin)
      SantaYr = 0;

   if (SantaYr > santayrmax)
      SantaYr = santayrmax;

   SantaY = lrintf(SantaYr);
   XOffsetRegion(SantaRegion, SantaX - oldx, SantaY - oldy);
   XOffsetRegion(SantaPlowRegion, SantaX - oldx, SantaY - oldy);

   RETURN;
}

void ResetSanta()      
{
   SantaX  = -SantaWidth - ActualSantaSpeed;
   SantaXr = SantaX;
   SantaY  = randint(SnowWinHeight / 3)+40;
   if (Flags.Moon && switches.DrawBirds && moonX < 400)
   {
      P("moon seeking at start\n");
      SantaY = randint(Flags.MoonSize + 40)+moonY-20;
   }
   else
      SantaY  = randint(SnowWinHeight / 3)+40;
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
   p[0].x =          x; p[0].y = y;
   p[1].x =          x+w; p[1].y = y;
   p[2].x =          x+w; p[2].y = y+h;
   p[3].x =          x; p[3].y = y+h;
   p[4].x =          x; p[4].y = y;
   return XPolygonRegion(p, 5, EvenOddRule);
}
