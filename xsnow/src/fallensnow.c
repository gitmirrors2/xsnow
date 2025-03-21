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
 *
 */
#include <pthread.h>
#include <semaphore.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_spline.h>
#include <gsl/gsl_sort.h>
#include <assert.h>

#include "xsnow-constants.h"

#include "fallensnow.h"
#include "utils.h"
#include "windows.h"
#include "flags.h"
#include "snow.h"
#include "Santa.h"
#include "blowoff.h"
#include "wind.h"
#include "debug.h"
#include "spline_interpol.h"

#define NOTACTIVE \
   (Flags.BirdsOnly || !WorkspaceActive() || Flags.NoSnowFlakes || (Flags.NoKeepSWin && Flags.NoKeepSBot))


static void   drawquartcircle(int n, short int *y);  // nb: dimension of y > n+1
static void   CreateSurfaceFromFallen(FallenSnow *f);
static void   EraseFallenPixel(FallenSnow *fsnow,int x);
static void   CreateDesh(FallenSnow *p);
static int    do_change_deshes(void *dummy);
static int    do_adjust_deshes(void *dummy);
static void   swapsurfaces(void);
static void  *do_fallen(void *d);
static int    lock_swap(void);
static int    unlock_swap(void);
static void   check_fallen(void);
static void   compute_mixed_color(GdkRGBA *color);

// pop first element
static int    PopFallenSnow(FallenSnow **list);

static sem_t swap_sem;
static sem_t fallen_sem;

void fallen_sem_init()
{
   sem_init(&swap_sem,0,1);
   sem_init(&fallen_sem,0,1);
}

void fallensnow_init()
{
   P("fallensnow_init\n");
   InitFallenSnow();
   add_to_mainloop(PRIORITY_DEFAULT, time_change_bottom, do_change_deshes);
   add_to_mainloop(PRIORITY_DEFAULT, time_adjust_bottom, do_adjust_deshes);
   static pthread_t thread; 
   pthread_create(&thread,NULL,do_fallen,NULL);
   P(" \n");
}

void UpdateFallenSnowAtBottom()
{
   // threads: locking by caller
   FallenSnow *fsnow = FindFallen(global.FsnowFirst, 0);
   if (fsnow)
      fsnow->y = global.SnowWinHeight;
}

void fallensnow_draw(cairo_t *cr)
{
   if (NOTACTIVE)
      return;
   lock_swap();

   // To avoid vertical line between blob and fallensnow when using transparency:
   if (Flags.Transparency)
      cairo_set_antialias(cr,CAIRO_ANTIALIAS_NONE);
   else
      cairo_set_antialias(cr,CAIRO_ANTIALIAS_NONE);

   FallenSnow *fsnow = global.FsnowFirst;
   while(fsnow)
   {
      if (HandleFallenSnow(fsnow)) 
      {
	 P("fallensnow_draw %d %d\n",counter++,
	       cairo_image_surface_get_width(fsnow->surface));
	 P("fallensnow_draw: x:%d y:%d\n",fsnow->x,fsnow->y);
	 cairo_set_source_surface (cr, fsnow->surface, fsnow->x, fsnow->y-fsnow->h);
	 my_cairo_paint_with_alpha(cr,ALPHA);
	 if (fsnow->win.id)
	 {
	    // draw half circle left and right (blobs)
	    GdkRGBA color;
	    int w = cairo_image_surface_get_width(fsnow->surface);
	    P("acth; %d %d\n",fsnow->acth[0],fsnow->acth[1]);
	    compute_mixed_color(&color);
	    cairo_set_source_rgba(cr, color.red, color.green, color.blue, ALPHA);
	    cairo_arc(cr,fsnow->x,fsnow->y,fsnow->firsth,M_PI/2,3*M_PI/2);
	    cairo_close_path(cr);
	    cairo_fill(cr);
	    cairo_arc(cr,fsnow->x+w-1,fsnow->y,fsnow->lasth,-M_PI/2,M_PI/2);
	    cairo_close_path(cr);
	    cairo_fill(cr);
	 }

	 fsnow->prevx = fsnow->x;
	 fsnow->prevy = fsnow->y-fsnow->h+1;
	 fsnow->prevw = cairo_image_surface_get_width(fsnow->surface);
	 fsnow->prevh = fsnow->h;
      }
      fsnow = fsnow->next;
   }
   unlock_swap();
}


void fallensnow_ui()
{
   UIDO(MaxWinSnowDepth   , InitFallenSnow(); ClearScreen(); );
   UIDO(MaxScrSnowDepth   , 
	 SetMaxScreenSnowDepthWithLock();
	 InitFallenSnow();
	 ClearScreen();
       );
   UIDO(NoKeepSBot        , InitFallenSnow(); ClearScreen(); );
   UIDO(NoKeepSWin        , InitFallenSnow(); ClearScreen(); );
   UIDO(IgnoreTop         ,                                  );
   UIDO(IgnoreBottom      ,                                  );
}

int lock_fallen() //Note: there is a macro Lock_fallen see fallensnow.h
{
   P("lock_fallen\n");
   return sem_wait(&fallen_sem);
}

int unlock_fallen() //Note: there is a macro Unlock_fallen, see fallensnow.h
{
   P("unlock_fallen\n");
   return sem_post(&fallen_sem);
}

// tries to get a lock on fallen_sem
// if (*c)++ <= n, the function returns immediately,
// the return value tells if the lock succeeded
// 0: succes, else no success
// if (*c)++ >n, sem_wait is used, the function returns 0 after getting
// the lock. 
// in both cases, *c is set to zero if the lock is obtained
int lock_fallen_n(int n, int *c)  // see fallensnow.h for the macro Lock_fallen_n
{
   int rc;
   if (*c < 0) 
      *c = 0;
   (*c)++;
   if (*c > n)
      rc = sem_wait(&fallen_sem);
   else
      rc = sem_trywait(&fallen_sem);
   P("lock_fallen_n %d %d %d\n",n,*c,rc);
   if(rc == 0)
      *c = 0;
   return rc;
}

void check_fallen()
{
   int i;
   int rc = sem_getvalue(&fallen_sem,&i);
   if(rc)
   {
      printf("error in get_semvalue()\n");
      traceback();
      exit(1);
   }
   if (i != 0)
   {
      printf("fallen_sem: %d\n",i);
      traceback();
      exit(1);
   }
}

int lock_swap()
{
   return sem_wait(&swap_sem);
}

int unlock_swap()
{
   return sem_post(&swap_sem);
}


void *do_fallen(void *d)
{

   (void)d;
   while(1)
   {
      if (Flags.Done)
	 pthread_exit(NULL);
      if (!NOTACTIVE)
      {
	 P("%d do_fallen\n",global.counter++);
	 Lock_fallen();

	 FallenSnow *fsnow = global.FsnowFirst;
	 while(fsnow)
	 {
	    if (HandleFallenSnow(fsnow)) 
	       DrawFallen(fsnow);
	    fsnow = fsnow->next;
	 }
	 XFlush(global.display);

	 swapsurfaces();

	 Unlock_fallen();
      }
      usleep((useconds_t)(time_fallen*1000000));
   }
   return NULL;
}

void swapsurfaces()
{
   lock_swap();
   FallenSnow *fsnow = global.FsnowFirst;
   while(fsnow)
   {
      cairo_surface_t *s = fsnow->surface1;
      fsnow->surface1    = fsnow->surface;
      fsnow->surface     = s;
      fsnow              = fsnow->next;
   }
   unlock_swap();
}

void drawquartcircle(int n, short int *y)  // nb: dimension of y > n+1
{
   float n2 = n*n;
   for (int i=0; i<=n; i++)
      y[i] = lrintf(sqrtf(n2 - i*i));
}

void CreateDesh(FallenSnow *p)
{
   // threads: locking by caller
   P("CreateDesh %d %p\n",global.counter++,(void*)p);
   int w           = p->w;
   int h           = p->h;
   short int *desh = p->desh;
#define N 6
   double splinex[N];
   double spliney[N];

   randomuniqarray(splinex,N,0.0000001,NULL);
   for (int i=0; i<N; i++)
   {
      float min = 0.3;
      float max = 1.0;
      splinex[i] *= (w-1);
      spliney[i] = min + (max - min)*drand48();
   }

   splinex[0] = 0;
   splinex[N-1] = w-1;

   double *x = (double *)malloc(w*sizeof(double));
   double *y = (double *)malloc(w*sizeof(double));
   assert(x);
   assert(y);
   for (int i=0; i<w; i++)
      x[i] = i;
   spline_interpol(splinex, N, spliney, x, w, y);
   for (int i=0; i<w; i++)
   {
      desh[i] = h*y[i];
      if (desh[i] < 2)
	 desh[i] = 2;
   }

   free(x);
   free(y);

#if 0
   int id          = p->win.id;
   FILE *ff = fopen("/tmp/desh","w");
   fprintf(ff,"id: %d\n",id);
   for (int i=0; i<w; i++)
      fprintf(ff,"%d %d\n",i,desh[i]);
   fclose(ff);
#endif
}

// insert a node at the start of the list 
void PushFallenSnow(FallenSnow **first, WinInfo *win, int x, int y, int w, int h) 
{
   // threads: locking by caller
   if(w<3) return;  // too narrow windows results in complications with regard to 
		    //                  computing splines etc.
   FallenSnow *p = (FallenSnow *)malloc(sizeof(FallenSnow));
   assert(p);
   p->win        = *win;
   p->x          = x;
   p->y          = y;
   p->w          = w;
   p->h          = h;
   p->prevx      = 0;
   p->prevy      = 0;
   p->prevw      = 10;
   p->prevh      = 10;
   p->firsth     = 0;
   p->lasth      = 0;
   p->acth       = (short int *)malloc(sizeof(*(p->acth))*w);
   assert(p->acth);
   p->desh       = (short int *)malloc(sizeof(*(p->desh))*w);
   assert(p->desh);
   p->surface    = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,w,h);
   //p->surface1   = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,w,h);
   p->surface1   = cairo_surface_create_similar (p->surface, CAIRO_CONTENT_COLOR_ALPHA, w, h);

   int l = 0;
   for (int i=0; i<w; i++)
   {
      p->acth[i] = 0;
      p->desh[i] = h;
      l++;
      if (l > h)
	 l = 0;
   }

   P("PushFallenSnow\n");
   CreateDesh(p);

   (void)drawquartcircle;
#if 0
   if (w > h && win->id != 0)
   {
      drawquartcircle(h,&(p->desh[w-h-1]));
      for (int i=0; i<=h; i++)
	 p->desh[i] = p->desh[w-1-i];
   }
#endif

   p->next  = *first;
   *first   = p;
}


// change to desired heights
int do_change_deshes(void *dummy)
{
   (void)dummy;
   static int lockcounter;
   if(Lock_fallen_n(3,&lockcounter))
      return TRUE;
   FallenSnow *fsnow = global.FsnowFirst;
   while(fsnow)
   {
      if (drand48() < 0.7)
	 continue;
      CreateDesh(fsnow);
      fsnow = fsnow->next;
   }
   Unlock_fallen();
   return TRUE;
}

int do_adjust_deshes(void *dummy)
{
   // threads: probably no need for lock, but to be sure:
   Lock_fallen();
   FallenSnow *fsnow = global.FsnowFirst;
   while(fsnow)
   {
      int adjustments = 0;
      for (int i=0; i<fsnow->w; i++)
      {
	 int d = fsnow->acth[i] - fsnow->desh[i];
	 if (d > 0)
	 {
	    int c = 1;
	    adjustments++;
	    fsnow->acth[i] -= c;
	 }
      }
      P("adjustments: %d\n",adjustments);
      fsnow = fsnow->next;
   }
   Unlock_fallen();
   return TRUE;
   (void)dummy;
}

// pop from list
int PopFallenSnow(FallenSnow **list)
{
   // threads: locking by caller
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
   // threads: locking by caller
   P("RemoveFallenSnow\n");
   if (*list == NULL)
   {
      return 0;
   }

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
      {
	 return 0;
      }
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
   // threads: locking by caller
   free(fallen->acth);
   free(fallen->desh);
   cairo_surface_destroy(fallen->surface);
   cairo_surface_destroy(fallen->surface1);
   free(fallen);
}

FallenSnow *FindFallen(FallenSnow *first, Window id)
{
   // threads: locking by caller
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
      for (int i=0; i<fallen->w; i++)
	 sumact += fallen->acth[i];
      printf("id:%#10lx ws:%4ld x:%6d y:%6d w:%6d sty:%2d hid:%2d sum:%8d\n", fallen->win.id, fallen->win.ws,
	    fallen->x, fallen->y, fallen->w, fallen->win.sticky, fallen->win.hidden, sumact);
      fallen = fallen->next;
   }
}

void CleanFallenArea(FallenSnow *fsnow,int xstart,int w)
{
   // threads: locking by caller
   if(global.IsDouble)  // so this one is only used in vintage desktops
      return;
   P("CleanFallenArea %d %d %d %d\n",global.counter++,global.IsDouble,xstart,w);
   int x = fsnow->prevx;
   int y = fsnow->prevy;
   if (!global.IsDouble) // TODO: make sure that not too much is erased
      myXClearArea(global.display, global.SnowWin, x+xstart, y, 
	    w, fsnow->h+global.MaxSnowFlakeHeight, global.xxposures);
}

// clean area for fallensnow with id
void CleanFallen(Window id)
{
   // threads: locking by caller
   P("CleanFallen %#lx\n",id);
   FallenSnow *fsnow = global.FsnowFirst;
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

void compute_mixed_color(GdkRGBA *color)
{
   if (Flags.UseColor2)
   {
      char newcolor[8];
      mixcolors(Flags.SnowColor, Flags.SnowColor2, 0.5, &newcolor[0]);
      gdk_rgba_parse(color,newcolor);
   }
   else
      gdk_rgba_parse(color,Flags.SnowColor);
}

void CreateSurfaceFromFallen(FallenSnow *f)
{
   // threads: locking by caller
   P("createsurface %#10lx %d %d %d %d %d %d\n",f->id,f->x,f->y,f->w,f->h,
	 cairo_image_surface_get_width(f->surface1),
	 cairo_image_surface_get_height(f->surface1));
   GdkRGBA color;

   cairo_t *cr      = cairo_create(f->surface1);
   int h            = f->h;
   int w            = f->w;
   short int *acth  = f->acth;

   // To avoid vertical line between blob and fallensnow when using transparency:
   if (Flags.Transparency)
      cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
   else
      cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);

   cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

   compute_mixed_color(&color);

   cairo_set_source_rgb(cr,color.red, color.green, color.blue);

   {
      // clear surface1
      cairo_save(cr);
      cairo_set_source_rgba(cr, 0, 0, 0, 0);
      cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
      cairo_paint (cr);
      cairo_restore(cr);
   }

   {
      // compute averages for 10 points, draw spline through them
      // and use that to draw fallensnow

      const int m = 10;
      int nav = 3+(w-2)/m;

      double *av = (double *)malloc(nav*sizeof(double));
      double *x  = (double *)malloc(nav*sizeof(double));
      assert(av);
      assert(x);

      for (int i=0; i<nav-3; i++)
      {
	 double s = 0;
	 for (int j=0; j<m; j++)
	    s += acth[m*i+j];
	 av[i+1] = s/m;
	 x[i+1]  = m*i + 0.5*m;
      }
      x[0]  = 0;
      av[0] = av[1];

      int k    = nav - 3;
      int mk   = m*k;
      double s = 0;
      for (int i=mk; i<w; i++)
	 s += acth[i];

      av[k+1] = s/(w-mk);
      x[k+1]  = mk + 0.5*(w-mk-1);

      x[nav-1]  = w-1;
      av[nav-1] = av[nav-2];

      gsl_interp_accel *acc = gsl_interp_accel_alloc();
      gsl_spline *spline    = gsl_spline_alloc(SPLINE_INTERP, nav);
      gsl_spline_init(spline,x,av,nav);

      // fill in first and last heights, needed for drwaing blobs
      f->firsth = gsl_spline_eval(spline,0  ,acc);
      f->lasth  = gsl_spline_eval(spline,w-1,acc);

      cairo_set_line_width(cr,1);

      {
	 // draw fallensnow: a move_to, followed by line_to's, followed by close_path and fill.
	 // to prevent a permanent bottom snow-line, even if there has no snow fallen on
	 // certain parts, only handle regions where snow has been fallen.

	 enum{searching, drawing};
	 int state = searching; // searching for acth[] > 0
	 int startpos;
	 for (int i=0; i<w; ++i)
	 {
	    int val = gsl_spline_eval(spline,i,acc);

	    switch(state)
	    {
	       case searching:
		  if (val != 0)
		  {
		     startpos = i;
		     cairo_move_to(cr, i, h);
		     cairo_line_to(cr, i, h);
		     cairo_line_to(cr, i, h - val);
		     state = drawing;
		  }
		  break;
	       case drawing:
		  cairo_line_to(cr, i, h - val);
		  if (val == 0 || i == w-1)
		  {
		     cairo_line_to(cr, i, h);
		     cairo_line_to(cr, startpos, h);
		     cairo_close_path(cr);
		     cairo_stroke_preserve(cr);
		     cairo_fill(cr);
		     state = searching;
		  }
		  break;
	    }
	 }
      }
      if(0) // draw averages
      {
	 cairo_save(cr);
	 cairo_set_source_rgba(cr,1,0,0,1);
	 for (int i=0; i<nav; i++)
	    cairo_rectangle(cr,x[i],h-av[i]-4,4,4);
	 cairo_fill(cr);
	 cairo_restore(cr);
      }
      gsl_spline_free (spline);
      gsl_interp_accel_free (acc);

      free(x);
      free(av);
   }

   if(0) // for debugging
   {
      // draw max height of fallensnow (using f->desh)
      cairo_save(cr);

      cairo_set_operator(cr,CAIRO_OPERATOR_OVER);
      cairo_set_source_rgba(cr,1,0,0,1);
      for (int j=0; j<w; j++)
      {
	 cairo_rectangle(cr,j,h-f->desh[j],1,1);
      }
      cairo_fill(cr);
      cairo_fill(cr);
      cairo_restore(cr);
   }

   cairo_destroy(cr);
}

void DrawFallen(FallenSnow *fsnow)
{
   // threads: locking done by caller
   if(fsnow->win.id == 0 || (!fsnow->win.hidden &&
	    //(fsnow->win.ws == global.CWorkSpace || fsnow->win.sticky)))
      (IsVisibleFallen(fsnow) || fsnow->win.sticky)))
      {
	 // do not interfere with Santa
	 if(!Flags.NoSanta)
	 {
	    {
	       // generate snow if Santa touches top of window
	       int in = XRectInRegion(global.SantaRegion, fsnow->x, fsnow->y - fsnow->h,
		     fsnow->w, fsnow->h);
	       if (in == RectangleIn || in == RectanglePart)
	       {
		  int x = global.SantaX - fsnow->x;
		  int w = global.SantaWidth;
		  if (x < 0)
		  {
		     w += x;
		     x = 0;
		  }

		  P("generate flakes %d %d %d\n",global.counter++,x,w);
		  GenerateFlakesFromFallen(fsnow, x, w, -100, 0.05);
		  CleanFallenArea(fsnow,x,w);
		  for (int i=0; i<fsnow->w; i++)
		     if (i < x+w && i>=x)
			fsnow->acth[i] = 0;
	       }
	    }
	    int in = XRectInRegion(global.SantaPlowRegion, fsnow->x, fsnow->y - fsnow->h,
		  fsnow->w, fsnow->h);
	    if (in == RectangleIn || in == RectanglePart)
	    {
	       // generate snow for plouging Santa
	       // determine front of Santa in fsnow
	       int xfront;
	       if (global.SantaDirection == 0)  // left to right
		  xfront = global.SantaX+global.SantaWidth - fsnow->x;
	       else
		  xfront = global.SantaX                   - fsnow->x;

	       // determine back of Santa in fsnow, Santa can move backwards in strong wind
	       int xback;
	       if (global.SantaDirection == 0)
		  xback = xfront - global.SantaWidth;
	       else
		  xback = xfront + global.SantaWidth;

	       // clearing determines the amount of generated ploughing snow
	       const int clearing = 10;
	       float vy = -1.5*global.ActualSantaSpeed; 
	       if(vy > 0) vy = -vy;
	       if (vy < -100.0)
		  vy = -100;

	       if (global.ActualSantaSpeed > 0)
	       {
		  if (global.SantaDirection == 0)  // left to right
		  {
		     GenerateFlakesFromFallen(fsnow,xfront,         clearing,vy,0.15);
		     CleanFallenArea(fsnow,xback-clearing,global.SantaWidth+2*clearing);
		  }
		  else
		  {
		     GenerateFlakesFromFallen(fsnow,xfront-clearing,clearing,vy,0.15);
		     CleanFallenArea(fsnow,xback+clearing,global.SantaWidth+2*clearing);
		  }
	       }
	       if(global.SantaDirection == 0)
	       {
		  for (int i=0; i<fsnow->w; i++)
		     if (i < xfront+clearing && i>=xback-clearing)
			fsnow->acth[i] = 0;
	       }
	       else
	       {
		  for (int i=0; i<fsnow->w; i++)
		     if (i > xfront-clearing && i<=xback+clearing)
			fsnow->acth[i] = 0;
	       }
	       XFlush(global.display);
	    }
	 }
	 CreateSurfaceFromFallen(fsnow);
	 // drawing is handled in fallensnow_draw
      }
}

void GenerateFlakesFromFallen(FallenSnow *fsnow, int x, int w, float vy, float amount)
{
   // threads: locking by caller
   P("GenerateFlakes %d %d %d %f\n",global.counter++,x,w,vy);
   if (!Flags.BlowSnow || Flags.NoSnowFlakes)
      return;
   // animation of fallen fallen snow
   // x-values x..x+w are transformed in flakes, vertical speed vy
   int ifirst = x; 
   if (ifirst < 0) 
      ifirst = 0;
   if (ifirst > fsnow->w) 
      ifirst = fsnow->w;
   int ilast  = x+w; 
   if(ilast < 0) 
      ilast = 0;
   if (ilast > fsnow->w) 
      ilast = fsnow->w;
   P("ifirst ilast: %d %d %d %d %d\n",global.counter++,ifirst,ilast,w,w<(int)global.MaxSnowFlakeWidth?w:(int)global.MaxSnowFlakeWidth);
   P("maxheight: %d maxw: %d\n",global.MaxSnowFlakeHeight,global.MaxSnowFlakeWidth);
   for (int i=ifirst; i<ilast; i+=1)
   {
      for (int j=0; j<fsnow->acth[i]; j++)
      {
	 int kmax = BlowOff();
	 for (int k=0; k<kmax; k++)
	 {
	    float p = 0;
	    p = drand48();
	    // In X11, (global.Trans!=1) we want not too much
	    // generated flakes
	    // Otherwize, we go for more dramatic effects
	    // But, it appeared that, if global.Trans==1, too much snow
	    // is generated, choking the x server. 
	    if (p < amount)
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
   // threads: locking by caller
   if(fsnow->acth[x] > 0)
   {
      if(!global.IsDouble)
      {
	 int x1 = fsnow->x + x;
	 int y1 = fsnow->y - fsnow->acth[x];
	 if(!global.IsDouble)
	    myXClearArea(global.display, global.SnowWin, x1 , y1, 1, 1, global.xxposures);     
      }
      fsnow->acth[x]--;
   }
}

void InitFallenSnow()
{
   Lock_fallen();
   while (global.FsnowFirst)
      PopFallenSnow(&global.FsnowFirst);
   // create fallensnow on bottom of screen:
   WinInfo *NullWindow = (WinInfo *)malloc(sizeof(WinInfo));
   assert(NullWindow);
   memset(NullWindow,0,sizeof(WinInfo));

   PushFallenSnow(&global.FsnowFirst, NullWindow, 0, global.SnowWinHeight, global.SnowWinWidth, global.MaxScrSnowDepth);
   free(NullWindow);  // todo: is free correct?

   Unlock_fallen();
   (void)check_fallen;  // to prevent warning about unused check_fallen
}

// removes some fallen snow from fsnow, w pixels. If fallensnowheight < h: no removal
// also add snowflakes
void UpdateFallenSnowWithWind(FallenSnow *fsnow, int w, int h)
{
   // threads: locking by caller
   int x = randint(fsnow->w - w);
   for (int i=x; i<x+w; i++)
      if(fsnow->acth[i] > h)
      {
	 // animation of blown off snow
	 if (!Flags.NoWind && global.Wind != 0 && drand48() > 0.5)
	 {
	    int jmax = BlowOff();
	    for (int j=0; j< jmax; j++)
	    {
	       Snow *flake       = MakeFlake(-1);
	       flake->rx         = fsnow->x + i;
	       flake->ry         = fsnow->y - fsnow->acth[i] - drand48()*4;
	       flake->vx         = 0.25*fsignf(global.NewWind)*global.WindMax;
	       flake->vy         = -10;
	       flake->cyclic     = (fsnow->win.id == 0); // not cyclic for Windows, cyclic for bottom
	       P("%d:\n",counter++);
	    }
	    EraseFallenPixel(fsnow,i);
	 }
      }
}

void SetMaxScreenSnowDepthWithLock()
{
   Lock_fallen();
   SetMaxScreenSnowDepth();
   Unlock_fallen();
}

void SetMaxScreenSnowDepth()
{
   // threads: locking by caller
   global.MaxScrSnowDepth = Flags.MaxScrSnowDepth;
   if (global.MaxScrSnowDepth > (int)(global.SnowWinHeight-SNOWFREE)) {
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
   int k;
   k = 0;
   short int *old;
   // old will contain the acth values, corresponding with x-1..x+w (including)
   old = (short int *)malloc(sizeof(*old)*(w+2));
   assert(old);
   for (int i=imin-1; i<=imax; i++) 
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
   for (int i=imin; i<imax; i++)
   {
      if ((fsnow->desh[i] > old[k]) &&
	    (old[k-1] >= old[k] || old[k+1] >= old[k]))
	 fsnow->acth[i] = add + (old[k-1] + old[k+1])/2;
      k++;
   }
   // old will contain the new acth values, corresponding with x-1..x+w (including)
   k = 0;
   for (int i=imin-1; i<=imax; i++) 
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
   for (int i=imin; i<imax; i++)
   {
      int sum=0;
      for (int j=k-1; j<=k+1; j++)
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
      //if (fsnow->win.ws != global.CWorkSpace)
      if (!IsVisibleFallen(fsnow))
	 return 0;
   }
   return !Flags.NoKeepSWin;
}

int IsVisibleFallen(FallenSnow *fsnow)
{
   if (!fsnow)
   {
      P("fsnow: %p\n",(void*)fsnow);
      return 0;
   }
   long ws = fsnow->win.ws;

   for (int i=0; i<global.NVisWorkSpaces; i++)
   {
      P("%d examining %d %ld %ld\n",global.counter++,i,ws,global.VisWorkSpaces[i]);
      if (global.VisWorkSpaces[i] == ws)
      {
	 P("fallensnow    visible  %ld %ld\n",fsnow->win.id,ws);
	 return 1;
      }
   }
   P("desktop not visible %ld %ld\n",fsnow->win.id,ws);
   return 0;
}

