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



static void   drawquartcircle(int n, short int *y);  // nb: dimension of y > n+1
static void   CreateSurfaceFromFallen(FallenSnow *f);
static void   EraseFallenPixel(FallenSnow *fsnow,int x);

void fallensnow_init()
{
   InitFallenSnow();
   P(" \n");
}

void UpdateFallenSnowAtBottom()
{
   FallenSnow *fsnow = FindFallen(global.FsnowFirst, 0);
   if (fsnow)
      fsnow->y = global.SnowWinHeight;
}

void fallensnow_draw(cairo_t *cr)
{
   if (NOTACTIVE)
      return;
   FallenSnow *fsnow = global.FsnowFirst;
   while(fsnow)
   {
      if (HandleFallenSnow(fsnow)) 
      {
	 P("fallensnow_draw %d %d\n",counter++,
	       cairo_image_surface_get_width(fsnow->surface));
	 P("fallensnow_draw: x:%d y:%d\n",fsnow->x,fsnow->y);
	 cairo_set_source_surface (cr, fsnow->surface, fsnow->x, fsnow->y-fsnow->h+1);
	 my_cairo_paint_with_alpha(cr,ALPHA);
	 fsnow->prevx = fsnow->x;
	 fsnow->prevy = fsnow->y-fsnow->h+1;
	 fsnow->prevw = cairo_image_surface_get_width(fsnow->surface);
	 fsnow->prevh = fsnow->h;
      }
      fsnow = fsnow->next;
   }
}

void fallensnow_erase()
{
   if (NOTACTIVE)
      return;
   FallenSnow *fsnow = global.FsnowFirst;
   while(fsnow)
   {
      if (HandleFallenSnow(fsnow)) 
      {
	 P("fallensnow_erase %d %d %d %d %d\n",counter++,
	       fsnow->prevx, fsnow->prevy, fsnow->prevw, fsnow->prevh);

	 myXClearArea(global.display,global.SnowWin,
	       fsnow->prevx, fsnow->prevy, fsnow->prevw, fsnow->prevh, global.xxposures);
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

int do_fallen(void *d)
{

   if (Flags.Done)
      return FALSE;
   if (NOTACTIVE)
      return TRUE;

   FallenSnow *fsnow = global.FsnowFirst;
   while(fsnow)
   {
      if (HandleFallenSnow(fsnow)) 
	 DrawFallen(fsnow);
      fsnow = fsnow->next;
   }
   XFlush(global.display);
   return TRUE;
   (void)d;
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
   p->prevx      = 0;
   p->prevy      = 0;
   p->prevw      = 10;
   p->prevh      = 10;
   p->w8         = ((w-1)/8+1)*8;
   p->acth       = (short int *)malloc(sizeof(*(p->acth))*w);
   p->desh       = (short int *)malloc(sizeof(*(p->desh))*w);
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
   P("RemoveFallenSnow\n");
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
      printf("id:%#10lx ws:%4d x:%6d y:%6d w:%6d sty:%2d hid:%2d sum:%8d\n", fallen->win.id, fallen->win.ws,
	    fallen->x, fallen->y, fallen->w, fallen->win.sticky, fallen->win.hidden, sumact);
      fallen = fallen->next;
   }
}

void CleanFallenArea(FallenSnow *fsnow,int xstart,int w)
{
   if(global.IsDouble)
      return;
   P("CleanFallenArea %d %d\n",counter++,global.IsDouble);
   int x = fsnow->prevx;
   int y = fsnow->prevy;
   myXClearArea(global.display, global.SnowWin, x+xstart, y, w, fsnow->h+global.MaxSnowFlakeHeight, global.xxposures);
}

// clean area for fallensnow with id
void CleanFallen(Window id)
{
   P("CleanFallen %#lx\n",id);
   FallenSnow *fsnow = global.FsnowFirst;
   // search the id
   while(fsnow)
   {
      if(fsnow->win.id == id)
      {
	 CleanFallenArea(fsnow,0,fsnow->w);
	 //myXClearArea(global.display,global.SnowWin,
	 //     fsnow->prevx, fsnow->prevy, fsnow->prevw, fsnow->prevh, global.xxposures);
	 break;
      }
      fsnow = fsnow->next;
   }
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
   if(fsnow->win.id == 0 || (!fsnow->win.hidden &&
	    (fsnow->win.ws == global.CWorkSpace || fsnow->win.sticky)))
   {
      // do not interfere with Santa
      if(!Flags.NoSanta)
      {
	 int in = XRectInRegion(global.SantaPlowRegion, fsnow->x, fsnow->y - fsnow->h,
	       fsnow->w, fsnow->h);
	 if (in == RectangleIn || in == RectanglePart)
	 {
	    // determine front of Santa in fsnow
	    int xfront = global.SantaX+global.SantaWidth - fsnow->x;
	    // determine back of Santa in fsnow, Santa can move backwards in strong wind
	    int xback = xfront - global.SantaWidth;
	    const int clearing = 1;
	    float vy = -1.5*global.ActualSantaSpeed; 
	    if(vy > 0) vy = -vy;
	    if (vy < -100.0)
	       vy = -100;
	    if (global.ActualSantaSpeed > 0)
	       GenerateFlakesFromFallen(fsnow,xfront,clearing,vy);
	    CleanFallenArea(fsnow,xback-clearing,global.SantaWidth+2*clearing);
	    int i;
	    for (i=0; i<fsnow->w; i++)
	       if (i < xfront+clearing && i>=xback-clearing)
		  fsnow->acth[i] = 0;
	    XFlush(global.display);
	 }
      }
      CreateSurfaceFromFallen(fsnow);
      // drawing is handled in fallensnow-draw
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
   P("ifirst ilast: %d %d %d %d\n",ifirst,ilast,w,w<global.MaxSnowFlakeWidth?w:global.MaxSnowFlakeWidth);
   P("maxheight: %d maxw: %d\n",global.MaxSnowFlakeHeight,global.MaxSnowFlakeWidth);
   //for (i=ifirst; i<ilast; i+=w<global.MaxSnowFlakeHeight?w:global.MaxSnowFlakeWidth)
   for (i=ifirst; i<ilast; i+=1)
   {
      int j;
      for(j=0; j<fsnow->acth[i]; j++)
      {
	 int k, kmax = BlowOff();
	 for(k=0; k<kmax; k++)
	 {
	    float p = 0;
	    p = drand48();
	    // In X11, (global.Trans!=1) we want not too much
	    // generated flakes
	    // Otherwize, we go for more dramatic effects
	    // But, it appeared that, if global.Trans==1, too much snow
	    // is generated, choking the x server. 
	    if (p < 0.15)
	    {
	       Snow *flake   = MakeFlake(-1);
	       flake->rx     = fsnow->x + i + 16*(drand48()-0.5);
	       flake->ry     = fsnow->y - j - 8;
	       if (Flags.NoWind)
		  flake->vx     = 0;
	       else
		  flake->vx      = global.NewWind/8;
	       flake->vy         = vy;
	       flake->cyclic     = 0;
	    }
	 }
      }
   }
}

void EraseFallenPixel(FallenSnow *fsnow, int x)
{
   if(fsnow->acth[x] > 0)
   {
      if(!global.IsDouble)
      {
	 int x1 = fsnow->x + x;
	 int y1 = fsnow->y - fsnow->acth[x];
	 myXClearArea(global.display, global.SnowWin, x1 , y1, 1, 1, global.xxposures);     
      }
      fsnow->acth[x]--;
   }
}

void InitFallenSnow()
{
   while (global.FsnowFirst)
      PopFallenSnow(&global.FsnowFirst);
   // create fallensnow on bottom of screen:
   WinInfo *NullWindow = (WinInfo *)malloc(sizeof(WinInfo));
   memset(NullWindow,0,sizeof(WinInfo));

   PushFallenSnow(&global.FsnowFirst, NullWindow, 0, global.SnowWinHeight, global.SnowWinWidth, global.MaxScrSnowDepth);
   free(NullWindow);
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
	 if (!Flags.NoWind && global.Wind != 0 && drand48() > 0.5)
	 {
	    int j, jmax = BlowOff();
	    for (j=0; j< jmax; j++)
	    {
	       Snow *flake       = MakeFlake(0);
	       flake->rx         = fsnow->x + i;
	       flake->ry         = fsnow->y - fsnow->acth[i] - drand48()*8;
	       flake->vx         = fsignf(global.NewWind)*global.WindMax;
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
   global.MaxScrSnowDepth = Flags.MaxScrSnowDepth;
   if (global.MaxScrSnowDepth> (global.SnowWinHeight-SNOWFREE)) {
      printf("** Maximum snow depth set to %d\n", global.SnowWinHeight-SNOWFREE);
      global.MaxScrSnowDepth = global.SnowWinHeight-SNOWFREE;
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
   short int *old;
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
}

int HandleFallenSnow(FallenSnow *fsnow)
{
   if (fsnow->win.id == 0)
      return !Flags.NoKeepSBot;
   if (fsnow->win.hidden)
      return 0;
   if (!fsnow->win.sticky)
   {
      if (fsnow->win.ws != global.CWorkSpace)
	 return 0;
   }
   return !Flags.NoKeepSWin;
}


