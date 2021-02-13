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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include "fallensnow.h"
#include "utils.h"
#include "windows.h"
#include "flags.h"
#include "snow.h"
#include "Santa.h"
#include "blowoff.h"
#include "wind.h"
#include "debug.h"

#define NOTACTIVE \
   (Flags.BirdsOnly || !WorkspaceActive() || Flags.NoSnowFlakes)

int        MaxScrSnowDepth = 0;
FallenSnow *FsnowFirst = NULL;

static GC EFallenGC;
static GC FallenGC;

static void   drawquartcircle(int n, short int *y);  // nb: dimension of y > n+1
static void   CreateSurfaceFromFallen(FallenSnow *f);
static Pixmap CreatePixmapFromFallen(FallenSnow *f);
static void   EraseFallenPixel(FallenSnow *fsnow,int x);

void fallensnow_init()
{
   InitFallenSnow();
   P(" ");
   FallenGC      = XCreateGC(display, SnowWin,    0, NULL);
   EFallenGC     = XCreateGC(display, SnowWin,    0, NULL);  // used to erase fallen snow
}

void fallensnow_set_gc()
{
   XSetLineAttributes(display, FallenGC, 1, LineSolid,CapRound,JoinMiter);
   XSetFillStyle( display, EFallenGC, FillSolid);
   XSetFunction(  display, EFallenGC, GXcopy);
   XSetForeground(display, EFallenGC, ErasePixel);
}

void fallensnow_draw(cairo_t *cr)
{
   if (NOTACTIVE)
      return;
   FallenSnow *fsnow = FsnowFirst;
   while(fsnow)
   {
      if (HandleFallenSnow(fsnow)) 
      {
	 P("fallensnow_draw %d\n",
	       cairo_image_surface_get_width(fsnow->surface));
	 cairo_set_source_surface (cr, fsnow->surface, fsnow->x, fsnow->y-fsnow->h+1);
	 my_cairo_paint_with_alpha(cr,ALPHA);
      }
      fsnow = fsnow->next;
   }
}

void fallensnow_ui()
{
   UIDO(MaxWinSnowDepth   , InitFallenSnow(); ClearScreen(); );
   UIDO(MaxScrSnowDepth   , 
      SetMaxScreenSnowDepth();
      InitFallenSnow();
      ClearScreen();
      );
   UIDO(NoKeepSBot        , InitFallenSnow(); ClearScreen(); );
   UIDO(NoKeepSWin        , InitFallenSnow(); ClearScreen(); );
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

void drawquartcircle(int n, short int *y)  // nb: dimension of y > n+1
{
   int i;
   float n2 = n*n;
   for(i=0; i<=n; i++)
      y[i] = lrintf(sqrtf(n2 - i*i));
}

// insert a node at the start of the list 
void PushFallenSnow(FallenSnow **first, WinInfo *win, int x, int y, int w, int h) 
{
   FallenSnow *p = (FallenSnow *)malloc(sizeof(FallenSnow));
   p->win        = *win;
   p->x          = x;
   p->y          = y;
   p->w          = w;
   p->h          = h;
   p->w8         = ((w-1)/8+1)*8;
   p->acth       = (short int *)malloc(sizeof(*(p->acth))*w);
   p->desh       = (short int *)malloc(sizeof(*(p->desh))*w);
   p->clean      = 0;  
   p->surface    = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,w,h);

   cairo_t *cr = cairo_create(p->surface);
   cairo_set_source_rgba(cr,1,0,0,0.0);
   cairo_rectangle(cr,0,0,w,h);
   cairo_fill(cr);
   cairo_destroy(cr);

   int l = 0,i;
   for (i=0; i<w; i++)
   {
      p->acth[i] = 0; // specify l to get sawtooth effect
      p->desh[i] = h;
      l++;
      if (l > h)
	 l = 0;
   }


   if (w > h && win->id != 0)
   {
      drawquartcircle(h,&(p->desh[w-h-1]));
      for(i=0; i<=h; i++)
	 p->desh[i] = p->desh[w-1-i];
   }

   p->next  = *first;
   *first   = p;
}

// pop from list
int PopFallenSnow(FallenSnow **list)
{
   FallenSnow *next_node = NULL;

   if (*list == NULL) 
      return 0;

   next_node = (*list)->next;
   FreeFallenSnow(*list);
   *list = next_node;
   return 1;
}

// remove by id
int RemoveFallenSnow(FallenSnow **list, Window id)
{
   if (*list == NULL)
      return 0;

   FallenSnow *fallen = *list;
   if (fallen->win.id == id)
   {
      fallen = fallen->next;
      FreeFallenSnow(*list);
      *list = fallen;
      return 1;
   }

   FallenSnow *scratch = NULL;

   while (1)
   {
      if (fallen->next == NULL)
	 return 0;
      scratch = fallen->next;
      if (scratch->win.id == id)
	 break;
      fallen = fallen->next;
   }

   fallen->next = scratch->next;
   FreeFallenSnow(scratch);

   return 1;
}

void FreeFallenSnow(FallenSnow *fallen)
{
   free(fallen->acth);
   free(fallen->desh);
   cairo_surface_destroy(fallen->surface);
   free(fallen);
}

FallenSnow *FindFallen(FallenSnow *first, Window id)
{
   FallenSnow *fsnow = first;
   while(fsnow)
   {
      if(fsnow->win.id == id)
	 return fsnow;
      fsnow = fsnow->next;
   }
   return NULL;
}
// print list
void PrintFallenSnow(FallenSnow *list)
{
   FallenSnow *fallen = list;

   while (fallen != NULL) {
      int sumact = 0;
      int i;
      for(i=0; i<fallen->w; i++)
	 sumact += fallen->acth[i];
      printf("id:%#10lx ws:%4d x:%6d y:%6d w:%6d cln:%2d sty:%2d hid:%2d sum:%8d\n", fallen->win.id, fallen->win.ws,
	    fallen->x, fallen->y, fallen->w, fallen->clean, fallen->win.sticky, fallen->win.hidden, sumact);
      fallen = fallen->next;
   }
}

void CleanFallenArea(FallenSnow *fsnow,int xstart,int w)
{
   if(switches.UseGtk)
      return;
   if(fsnow->clean) 
      return;
   int x = fsnow->x;
   int y = fsnow->y - fsnow->h;
   if(switches.Trans|Flags.UseBG)
      XFillRectangle(display, SnowWin,  EFallenGC, x+xstart,y,
	    w, fsnow->h+MaxSnowFlakeHeight);
   else
      XClearArea(display, SnowWin, x+xstart, y, w, fsnow->h+MaxSnowFlakeHeight, switches.Exposures);
   if(xstart <= 0 && w >= fsnow->w)
      fsnow->clean = 1;
}

// clean area for fallensnow with id
void CleanFallen(Window id)
{
   FallenSnow *fsnow = FsnowFirst;
   // search the id
   while(fsnow)
   {
      if(fsnow->win.id == id)
      {
	 CleanFallenArea(fsnow,0,fsnow->w);
	 break;
      }
      fsnow = fsnow->next;
   }
}

Pixmap CreatePixmapFromFallen(FallenSnow *f)
{
   // todo: takes too much cpu
   int j;
   int p = 0;
   // malloc((1+ ...) to be sure we are allocating at least 1 byte
   unsigned char *bitmap = (unsigned char *) malloc((1+f->w8*f->h/8)*sizeof(unsigned char));

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
   free(bitmap);
   return pixmap;
}

void CreateSurfaceFromFallen(FallenSnow *f)
{
   P("createsurface %#10lx %d %d %d %d %d %d\n",f->id,f->x,f->y,f->w,f->h,
	 cairo_image_surface_get_width(f->surface),
	 cairo_image_surface_get_height(f->surface));
   int i;
   GdkRGBA color;

   // clear the surface (NB: CAIRO_OPERATOR_SOURCE)
   cairo_t *cr = cairo_create(f->surface);
   cairo_set_source_rgba(cr,0,0,0,0);
   cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
   cairo_rectangle(cr,0,0,f->w,f->h);
   cairo_fill(cr);
   cairo_set_line_width(cr,1);
   cairo_set_antialias(cr,CAIRO_ANTIALIAS_NONE);
   gdk_rgba_parse(&color,Flags.SnowColor);
   cairo_set_source_rgb(cr,color.red, color.green, color.blue);
   int h = f->h;
   short int *acth = f->acth;

   for (i=0; i<f->w; i++)
   {
      cairo_move_to(cr,i,h-1);
      cairo_line_to(cr,i,h-1-acth[i]);
   }
   cairo_stroke(cr);
   cairo_destroy(cr);
}

void DrawFallen(FallenSnow *fsnow)
{
   if(!fsnow->clean)
      if(fsnow->win.id == 0 || (!fsnow->win.hidden &&
	       (fsnow->win.ws == CWorkSpace || fsnow->win.sticky)))
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
	       const int clearing = 1;
	       float vy = -1.5*ActualSantaSpeed; 
	       if(vy > 0) vy = -vy;
	       if (vy < -100.0)
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
	 if(switches.UseGtk)
	 {
	    CreateSurfaceFromFallen(fsnow);
	    // drawing is handled in fallensnow-draw
	 }
	 else
	 {
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
	    P("Fallensnow %d %d %d %d\n",x,y,fsnow->w,fsnow->h);
	 }
      }
}

void GenerateFlakesFromFallen(FallenSnow *fsnow, int x, int w, float vy)
{
   P("GenerateFlakes %d %d %d %f\n",counter++,x,w,vy);
   if (!Flags.BlowSnow || Flags.NoSnowFlakes)
      return;
   // animation of fallen fallen snow
   // x-values x..x+w are transformed in flakes, vertical speed vy
   int i;
   int ifirst = x; if (ifirst < 0) ifirst = 0;
   if (ifirst > fsnow->w) ifirst = fsnow->w;
   int ilast  = x+w; if(ilast < 0) ilast = 0;
   if (ilast > fsnow->w) ilast = fsnow->w;
   P("ifirst ilast: %d %d %d %d\n",ifirst,ilast,w,w<MaxSnowFlakeWidth?w:MaxSnowFlakeWidth);
   P("maxheight: %d maxw: %d\n",MaxSnowFlakeHeight,MaxSnowFlakeWidth);
   //for (i=ifirst; i<ilast; i+=w<MaxSnowFlakeHeight?w:MaxSnowFlakeWidth)
   for (i=ifirst; i<ilast; i+=1)
   {
      int j;
      for(j=0; j<fsnow->acth[i]; j++)
      {
	 int k, kmax = BlowOff();
	 for(k=0; k<kmax; k++)
	 {
	    float p = 0;
	    //if (!switches.UseGtk)
	    p = drand48();
	    // In X11, (switches.UseGtk!=1) we want not too much
	    // generated flakes
	    // Otherwize, we go for more dramatic effects
	    // But, it appeared that, if switches.UseGtk==1, too much snow
	    // is generated, choking the x server. 
	    if (p < 0.15)
	    {
	       Snow *flake   = MakeFlake(-1);
	       //flake->rx     = fsnow->x + i + 2*MaxSnowFlakeWidth*(drand48()-0.5);
	       //flake->ry     = fsnow->y - j - MaxSnowFlakeHeight;
	       flake->rx     = fsnow->x + i + 16*(drand48()-0.5);
	       flake->ry     = fsnow->y - j - 8;
	       if (Flags.NoWind)
		  flake->vx     = 0;
	       else
		  flake->vx      = NewWind/8;
	       flake->vy         = vy;
	       flake->cyclic     = 0;
	       //if (switches.UseGtk && drand48() > 0.25)
	       //if(drand48() > 0.25)
	       if(0)  // next {} is not needed, it seems now
	       {
		  fluffify(flake,0.7);
		  //flake->ry += 2*MaxSnowFlakeHeight*drand48();
	       }
	    }
	 }
      }
   }
}

void EraseFallenPixel(FallenSnow *fsnow, int x)
{
   if(fsnow->acth[x] > 0)
   {
      if(!switches.UseGtk)
      {
	 int x1 = fsnow->x + x;
	 int y1 = fsnow->y - fsnow->acth[x];
	 if(switches.Trans|Flags.UseBG)
	    XDrawPoint(display, SnowWin, EFallenGC, x1, y1);
	 else
	    XClearArea(display, SnowWin, x1 , y1, 1, 1, switches.Exposures);     
      }
      fsnow->acth[x]--;
   }
}

void InitFallenSnow()
{
   while (FsnowFirst)
      PopFallenSnow(&FsnowFirst);
   // create fallensnow on bottom of screen:
   WinInfo *NullWindow = (WinInfo *)malloc(sizeof(WinInfo));
   memset(NullWindow,0,sizeof(WinInfo));

   PushFallenSnow(&FsnowFirst, NullWindow, 0, SnowWinHeight, SnowWinWidth, MaxScrSnowDepth);
}

// removes some fallen snow from fsnow, w pixels. If fallensnowheight < h: no removal
// also add snowflakes
void UpdateFallenSnowWithWind(FallenSnow *fsnow, int w, int h)
{
   int i;
   int x = randint(fsnow->w - w);
   for(i=x; i<x+w; i++)
      if(fsnow->acth[i] > h)
      {
	 // animation of blown off snow
	 if (!Flags.NoWind && Wind != 0 && drand48() > 0.5)
	 {
	    int j, jmax = BlowOff();
	    //P("%d\n",jmax);
	    for (j=0; j< jmax; j++)
	    {
	       Snow *flake       = MakeFlake(0);
	       flake->rx         = fsnow->x + i;
	       flake->ry         = fsnow->y - fsnow->acth[i] - drand48()*8;// randint(MaxSnowFlakeWidth);
	       flake->vx         = fsignf(NewWind)*WindMax;
	       flake->vy         = -5;
	       flake->cyclic     = (fsnow->win.id == 0); // not cyclic for Windows, cyclic for bottom
	       P("%d:\n",counter++);
	    }
	    EraseFallenPixel(fsnow,i);
	 }
      }
}

void SetMaxScreenSnowDepth()
{
   MaxScrSnowDepth = Flags.MaxScrSnowDepth;
   if (MaxScrSnowDepth> (SnowWinHeight-SNOWFREE)) {
      printf("** Maximum snow depth set to %d\n", SnowWinHeight-SNOWFREE);
      MaxScrSnowDepth = SnowWinHeight-SNOWFREE;
   }
}


void UpdateFallenSnowPartial(FallenSnow *fsnow, int x, int w)
{
   if (NOTACTIVE)
      return;
   P("update ...\n");
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

int HandleFallenSnow(FallenSnow *fsnow)
{
   if (fsnow->win.id == 0)
      return !Flags.NoKeepSBot;
   if (fsnow->win.hidden)
      return 0;
   if (!fsnow->win.sticky)
   {
      if (fsnow->win.ws != CWorkSpace)
	 return 0;
   }
   return !Flags.NoKeepSWin;
}


