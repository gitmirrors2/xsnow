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
// contains main_birds(), the C main function to be called from
// the CXX main program
//
#include <gtk/gtk.h>
#include <math.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include "birds.h"
#include "clocks.h"
#include "debug.h"
#include "doitb.h"
#include "flags.h"
#include "globals.h"
#include "hashtable.h"
#include "kdtree.h"
#include "mainstub.h"
#include "pixmaps.h"
#include "ui.h"

static int counter = 0;

//#define SHOW_ATTRACTION_POINT

#define NWINGS 8
#define NBIRDPIXBUFS (3*NWINGS)

#define add_to_mainloop(prio,time,func,datap) g_timeout_add_full(prio,(guint)1000*(time),(GSourceFunc)func,datap,0)

static gboolean draw_cb (GtkWidget *widget, cairo_t *cr, gpointer userdata);
#if 0
static void screen_changed(GtkWidget *widget, GdkScreen *old_screen, gpointer userdata);
#endif

/* Surface to store current scribbles */
#if 0
static cairo_surface_t *globsurface = NULL;
#endif
static GtkWidget       *drawing_area = 0;
static GdkPixbuf       *bird_pixbufs[NBIRDPIXBUFS];

static int Nbirds;  // is copied from Flags.Nbirds in init_birds. We cannot have that
//                  // Nbirds is changed outside init_birds

// https://stackoverflow.com/questions/3908565/how-to-make-gtk-window-background-transparent
// https://stackoverflow.com/questions/16832581/how-do-i-make-a-gtkwindow-background-transparent-on-linux]
//static int supports_alpha = 1;
static gboolean supports_alpha = TRUE;

#if 0
static void screen_changed(GtkWidget *widget, GdkScreen *old_screen, gpointer userdata)
{
   /* To check if the display supports alpha channels, get the visual */
   GdkScreen *screen = gtk_widget_get_screen(widget);
   GdkVisual *visual = gdk_screen_get_rgba_visual(screen);

   if (!visual)
   {
      printf("YOUR SCREEN DOES NOT SUPPORT ALPHA CHANNELS!\n");
      visual = gdk_screen_get_system_visual(screen);
      supports_alpha = FALSE;
   }
   else
   {
      printf("YOUR SCREEN SUPPORTS ALPHA CHANNELS!\n");
      supports_alpha = TRUE;
   }

   gtk_widget_set_visual(widget, visual);
}
#endif

typedef struct _Birdtype
{
   float x,y,z;        // position, meters
   // x: horizontal
   // y: in/out screen
   // z: vertical
   float sx,sy,sz;     // velocity, m/sec
   int ix,iy,iz,iw,ih; // pixels
   // ix .. in pixels, used to store previous screen parameters
   int wingstate, orient;      
   int drawable;       // is in drawable range
} BirdType;

struct _globals globals;

/*
   static int sign(float x)
   {
   return x>=0?1:-1;
   }
   */
static float sq3(float x, float y, float z)
{
   return x*x + y*y + z*z;
}

static float sq2(float x, float y)
{
   return x*x + y*y;
}

static float fsignf(float x)
{
   if (x>0)
      return 1.0f;
   if (x<0)
      return -1.0f;
   return 0.0f;
}

#ifdef FORREAL
static int do_check_flags(void);
static int do_test(void);
#endif
static void init_bird_pixbufs(const char *color);
static int do_update_pos_birds(void); 
static int do_wings(void);
static int do_draw_birds(void);
static int do_update_speed_birds(void);

static void background(cairo_t *cr);
static void normalize_speed(BirdType *bird, float speed);
static void r2i(BirdType *bird);
static void clear_flags(void);
static void prefxyz(BirdType *bird, float d, float e, float x, float y, float z, float *prefx, float *prefy, float *prefz);


static float time_update_pos_birds     = 0.01;
static float time_update_speed_birds   = 0.20;
static float time_draw_birds           = 0.04;
static float time_wings                = 0.1;
#ifdef FORREAL
static float time_check_flags          = 0.1;
static float time_test                 = 0.5;
static float time_update_range         = 0.5;
static float time_update_mean_distance = 0.5;
#endif

static struct kdtree *kd = 0;

static BirdType *birds = 0;
#ifdef SHOW_ATTRACTION_POINT
static BirdType testbird;
#endif

static void normalize_speed(BirdType *bird, float speed)
{
   float v2 = sq3(bird->sx, bird->sy, bird->sz);
   if (fabsf(v2) < 1.0e-10)
      v2 = globals.meanspeed;
   float a = speed/sqrtf(v2);
   bird->sx *= a;
   bird->sy *= a;
   bird->sz *= a;
   //if (fabsf(bird->sy) > 0.5*globals.meanspeed)
   //  bird->sy *=0.9;
}

static void background(cairo_t *cr)
{
   draw_cb(0,cr,0);
}

float MaxViewingDistance()
{
   return 2*globals.maxy;
}

//float PreferredViewingDistance()
//{
//   return 0.3*globals.maxy;
//}

static float scale(float y)
{
   if (y != 0)
      return 0.005*(100-Flags.ViewingDistance)*globals.maxy/y;
   else
      return 1.0e6;
}

static void r2i(BirdType *bird)
{
   if(bird->y > Flags.ViewingDistance/8)
   {
      bird->drawable = 1;
      //float f  =  Flags.ViewingDistance/bird->y;
      //float f = 0.5;
      //float f = 0.01*20.0*globals.maxy/bird->y;
      float f = scale(bird->y);
      P("%f %d %f\n",globals.maxy,Flags.ViewingDistance,f);
      /*
	 bird->ix = f*globals.ax*(bird->x - globals.maxx/2) + globals.ox;
	 bird->iy =   globals.ay*bird->y + globals.oy;
	 bird->iz = f*globals.az*(bird->z - globals.maxz/2) + globals.oz;
	 */
      //bird->ix = globals.ax*globals.attrx;
      //bird->iz = globals.ax*globals.attrz;
      P("%f %f %f %f %f %d %d %d\n",f,globals.attrx,globals.attrz,globals.ox,globals.oz,bird->ix,bird->iy,bird->iz);
      //#define CO_REAL
#ifdef CO_REAL
      // classical camera obscura, inverted image:
      float x = f*(globals.xc-bird->x) + globals.xc;
      float z = f*(globals.zc-bird->z) + globals.zc;
#else
      // alternative camera obscura, not inverted image:
      float x = f*(bird->x-globals.xc) + globals.xc;
      float z = f*(bird->z-globals.zc) + globals.zc;
#endif
      bird->ix = globals.ax*x;
      bird->iy = globals.ay*bird->y;
      bird->iz = globals.az*z;
   }
   else
   {
      P("%d %f %f\n",counter++,bird->y, globals.vd);
      bird->drawable = 0;
   }
}


/* Redraw the screen from the surface. Note that the ::draw
 * signal receives a ready-to-be-used cairo_t that is already
 * clipped to only draw the exposed areas of the widget
 */
static gboolean draw_cb (GtkWidget *widget, cairo_t *cr, gpointer userdata)
{
   cairo_save (cr);
   P("supports_alpha: %d\n",supports_alpha);
   if (supports_alpha)
   {
      cairo_set_source_rgba (cr, 0.0, 1.0, 1.0, 0.0); /* transparent */
   }
   else
   {
      cairo_set_source_rgb (cr, 0.0, 1.0, 1.0); /* opaque blueish */
   }

   /* draw the background */
   cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
   cairo_paint (cr);

   cairo_restore (cr);

   return FALSE;
}

// given:
// bird
// distance d
// optimal distance e
// coordinates of other bird x,y,z
// compute optimal coordinates for bird: prefx, prefy, prefz
void prefxyz(BirdType *bird, float d, float e, float x, float y, float z, float *prefx, float *prefy, float *prefz)
{
   *prefx = e*(bird->x-x)/d + x;
   *prefy = e*(bird->y-y)/d + y;
   *prefz = e*(bird->z-z)/d + z;
   P("%f %f - %f %f %f, %f %f %f, %f %f %f\n",
	 d,e,
	 bird->x, bird->y, bird->z,
	 x,y,z,
	 *prefx, *prefy, *prefz
    );
}

int do_update_speed_birds()
{
   if (Flags.Done)
      return FALSE;
   if (!Flags.ShowBirds)
      return TRUE;
   if (globals.freeze)
      return TRUE;
   P("do_update_speed_birds %d\n",counter);

   kd_free(kd);
   kd = kd_create(3);

   int i;

   for (i=0; i<Nbirds; i++)
   {
      BirdType *bird = &birds[i];
      kd_insert3f(kd, bird->x, bird->y, bird->z, bird);
   }

   int sumnum         = 0;
   float summeandist  = 0;
   for (i=0; i<Nbirds; i++)
   {
      if (drand48() < Flags.Anarchy*0.01)
	 continue;
      BirdType *bird = &birds[i];

      struct kdres *result = kd_nearest_range3f(kd,bird->x,bird->y,bird->z,globals.range);
      float sumsx    = 0;
      float sumsy    = 0;
      float sumsz    = 0;
      float sumprefx = 0;
      float sumprefy = 0;
      float sumprefz = 0;
      float sumdist  = 0;
      int   num      = 0;
      while (!kd_res_end(result))
      {
	 float x,y,z;
	 BirdType *b = (BirdType *)kd_res_item3f(result, &x, &y, &z);
	 kd_res_next(result);
	 if (bird == b)
	    continue;
	 num++;

	 // sum the speeds of neighbour birds:

	 sumsx += b->sx;
	 sumsy += b->sy;
	 sumsz += b->sz;

	 float dist = sqrtf((bird->x-x)*(bird->x-x)+
	       (bird->y-y)*(bird->y-y)+(bird->z-z)*(bird->z-z));

	 float prefx=0, prefy=0, prefz=0;
	 P("prefxyz %f\n",dist);
	 if (dist > 1e-6)
	    prefxyz(bird, dist, Flags.PrefDistance, x, y, z, &prefx, &prefy, &prefz);
	 sumprefx += prefx;
	 sumprefy += prefy;
	 sumprefz += prefz;
	 sumdist  += dist;

      }
      kd_res_free(result);

      // meanprefx,y,z: mean optimal coordinates with respect to other birds
      float meanprefx, meanprefy, meanprefz, meandist;
      P("num: %d\n",num);
      if (num > 0)
      {
	 meanprefx     = sumprefx / num;
	 meanprefy     = sumprefy / num;
	 meanprefz     = sumprefz / num;
	 meandist      = sumdist  / num;
	 summeandist  += meandist;
	 P("prefx - x ... %f %f %f %f\n",meanprefx - bird->x, meanprefy - bird->y, meanprefz - bird->z, meandist);
      }
      sumnum +=num;
      // adjust speed to other birds, p is weight for own speed
      if (num > 0)
      {
	 //int p = 8;
	 int p = (100-Flags.FollowWeight)*0.1;
	 bird->sx = (sumsx + p*bird->sx)/(p+1+num);
	 bird->sy = (sumsy + p*bird->sy)/(p+1+num);
	 bird->sz = (sumsz + p*bird->sz)/(p+1+num);
	 //R("means %d %f %f %f\n",i,bird->sx,bird->sy,bird->sz);
      }
      // adjust speed to obtain desired distance to other birds
      if (num > 0)
      {
	 float q = Flags.DisWeight*0.4;
	 bird->sx += q*(meanprefx - bird->x);
	 bird->sy += q*(meanprefy - bird->y);
	 bird->sz += q*(meanprefz - bird->z);
	 P("%d %f %f %f, %f %f %f\n",i,meanprefx,meanprefy,meanprefz,bird->x,bird->y,bird->z);
      }


      // attraction of center:

      float dx = globals.attrx - bird->x;
      float dy = globals.attry - bird->y;
      float dz = globals.attrz - bird->z;

      //float f = 0.05f;
      float f = Flags.AttrFactor*0.01f*0.05f;

      bird->sx += f*dx;
      bird->sy += f*dy;
      bird->sz += f*dz;

      // limit vertical speed

      const float phs = 0.8;
      float hs = sqrtf(sq2(bird->sx, bird->sy));
      if (fabs(bird->sz) > phs*hs)
	 bird->sz = fsignf(bird->sz)*phs*hs;

      // randomize:
      {
	 const float p  = 0.4;  //  0<=p<=1 the higher the more random
	 const float p1 = 1.0f-0.5*p;
	 bird->sx *= (p1+p*drand48());
	 bird->sy *= (p1+p*drand48());
	 bird->sz *= (p1+p*drand48());
      }

      normalize_speed(bird,globals.meanspeed*(0.9+drand48()*0.2));
   }
   float meannum = (float)sumnum/(float)Nbirds;
   globals.mean_distance = summeandist/Nbirds;
   P("meannum %f %f\n",meannum,globals.range);

   if (meannum < Flags.Neighbours)
   {
      if (globals.range < 0.1)
	 globals.range = 0.1;
      if (meannum < Nbirds-1)
	 globals.range *=1.1;
      if (globals.range > globals.maxrange)
	 globals.range /=1.1;
   }
   else
      globals.range /=1.1;

   return TRUE;
}

int do_update_pos_birds()
{
   if (Flags.Done)
      return FALSE;
   if (!Flags.ShowBirds)
      return TRUE;
   if (globals.freeze)
      return TRUE;
   P("do_update_pos_birds %d\n",Nbirds);
   counter ++;

   int i;
   for (i=0; i<Nbirds; i++)
   {
      BirdType *bird = &birds[i];

      bird->x += time_update_pos_birds*bird->sx;
      bird->y += time_update_pos_birds*bird->sy;
      bird->z += time_update_pos_birds*bird->sz;
      P("update: %f %f %f\n",bird->x,bird->z,bird->sx);

   }
   return TRUE;
}

int do_draw_birds()
{
   if (Flags.Done)
      return FALSE;

   P("do_draw_birds %d\n",counter);
   counter++;

   GdkWindow *window           = gtk_widget_get_window(drawing_area);  
   cairo_region_t *cairoRegion = cairo_region_create();

   GdkDrawingContext *drawingContext =
      gdk_window_begin_draw_frame(window,cairoRegion);

   cairo_t *cr = gdk_drawing_context_get_cairo_context(drawingContext);
   //cairo_set_source_rgba(cr,1.0,1.0,1.0,1.0);
   //cairo_rectangle(cr,0,0,globals.maxx,maxz);
   //cairo_fill(cr);
   //cairo_set_source_rgba (cr, 1, 1, 1, 1);
   //#define USE_RECTANGLE
   if (!Flags.ShowBirds)
   {
      background(cr);
      gdk_window_end_draw_frame(window,drawingContext);
      cairo_region_destroy(cairoRegion);
      return TRUE;
   }
#define CLEARALL
#ifdef CLEARALL
   background(cr);
#endif
#ifdef SHOW_ATTRACTION_POINT
   r2i(&testbird);
   cairo_set_source_rgb(cr,1.0,0.0,0.0);
   cairo_arc(cr,testbird.ix,testbird.iz,10,0,2*M_PI);
   cairo_fill(cr);
   cairo_set_source_rgb(cr,0.0,0.0,0.0);
#endif

   //GdkPixbuf *birdie = gdk_pixbuf_scale_simple(bird_pixbuf,16,8,GDK_INTERP_BILINEAR);

   int i;
   for (i=0; i<Nbirds; i++)
   {
      BirdType *bird = &birds[i];

#ifndef CLEARALL
      // erase:
      cairo_rectangle(cr,bird->ix,bird->iz,bird->iw,bird->ih);
      background(cr);
#endif

      // draw:

      r2i(bird);
      P("%d %f %f %f %d %d %d %d\n",i,bird->x,bird->y,bird->z,bird->ix,bird->iy,bird->iz,bird->drawable);
      if (bird->drawable)
      {
	 //float p = Flags.ViewingDistance/bird->y;
	 float p = scale(bird->y);

#ifdef USE_RECTANGLE 
	 bird->iw = 16*p;
	 bird->ih = 8*p;
	 cairo_rectangle(cr,bird->ix,bird->iz,bird->iw,bird->ih);
#else

	 cairo_surface_t *surface;
	 GdkPixbuf       *pixbuf = 0;
	 int iw,ih,nw;
	 nw = bird->wingstate;

	 int orient = 0*NWINGS;
	 //if (fabsf(bird->sx) > fabsf(bird->sy))
	 //	    orient = NWINGS;
	 //float sxz = sq2(bird->sx,bird->sz);
	 float sxz = fabsf(bird->sx);
	 float sy = fabs(bird->sy);
	 if (sxz > 1.73*sy)
	    orient = 2*NWINGS; // 1*NWINGS
	 else if (sy > 1.73*sxz)
	    orient = 0*NWINGS;
	 else
	    orient = 0*NWINGS;
	 // canonical:
	 // if (sxz > 1.73*sy)
	 //    orient = 1*NWINGS;    
	 //                          aside
	 //                            ***
	 //                         *********
	 //                         *********
	 //                         *********
	 //                            ***
	 //
	 // else if(sy > 1.73*sxz)    
	 //    orient = 0*NWINGS
	 //                          front
	 //                                 **
	 //                        ********************
	 //                                 **
	 // else
	 //    orient = 2*NWINGS
	 //                          oblique 
	 //                             ***
	 //                        *************
	 //                             ***
	 //

	 P("%f %f %d\n",sxz,bird->sy,orient);
	 GdkPixbuf *bird_pixbuf = bird_pixbufs[nw+orient];
	 iw = p*globals.bird_scale;
	 ih = p*globals.bird_scale*gdk_pixbuf_get_height(bird_pixbuf)/
	    (float)gdk_pixbuf_get_width(bird_pixbuf);
	 // do not draw very large birds (would be bad for cache use)
	 // and do not draw vanishing small birds
	 if (ih > globals.maxiz*0.2 || ih <=0) // iw is always larger than ih, we don't have to check iw
	 {
	    P("ih: %d %d\n",ih,globals.maxiz);
	    continue;
	 }

	 //int interpolation = GDK_INTERP_NEAREST;
	 GdkInterpType interpolation = GDK_INTERP_BILINEAR;
#if CACHE_LINEAR
	 // linear caching
	 const int k = 4;   // the higher, the less surfaces are cached and the more jerk
	 // in general 1<=k<=16
	 unsigned int key = ((iw/k)<<8)+nw+orient;
#else
	 // logarithmic caching
	 const double k   = log(1.2); // should be log(1.05) ... log(1.5). The higher, the less cache will be used
	 unsigned int key = ((unsigned int)(log(iw)/k)<<8) + nw + orient;
#endif
	 if (!table_get(key))
	 {
	    static int table_counter = 0;
	    static double cache = 0;
	    table_counter++;
	    cache += iw*ih;
	    P("Entries: %d Cache: %.0f MB width: %d Wing: %d orient: %d\n",table_counter,cache*4.0e-6,iw,nw,orient/8);
	    pixbuf = gdk_pixbuf_scale_simple(bird_pixbuf,iw,ih,interpolation); 
	    table_put(key,gdk_cairo_surface_create_from_pixbuf (pixbuf, 0, window));
	 }
	 surface = (cairo_surface_t*) table_get(key);

	 cairo_set_source_surface (cr, surface, bird->ix, bird->iz);
	 cairo_paint(cr);
	 g_clear_object(&pixbuf);
#endif
	 P("draw: %d %d\n",bird->ix,bird->iz);
      }
      else
      {
	 static int skipped = 0;
	 skipped++;
	 P("skipped: %d %d\n",skipped,bird->drawable);
      }
   }
   /*
      cairo_surface_t *
      gdk_cairo_surface_create_from_pixbuf (const GdkPixbuf *pixbuf,
      int scale,
      GdkWindow *for_window);
      void
      cairo_set_source_surface (cairo_t *cr,
      cairo_surface_t *surface,
      double x,
      double y);
      */
   //cairo_fill(cr);
   gdk_window_end_draw_frame(window,drawingContext);
   cairo_region_destroy(cairoRegion);
   return TRUE;
}

void init_birds(int start)
{
   int i;
   P("nbirds: %d %d\n",start,Flags.Nbirds);
   birds = (BirdType *)realloc(birds,sizeof(BirdType)*Flags.Nbirds);
   if (kd)
      kd_free(kd);
   kd = kd_create(3);
   Nbirds = Flags.Nbirds;
   for (i=start; i<Nbirds; i++)
   {
      BirdType *bird = &birds[i];
      bird->x = drand48()*globals.maxx;
      bird->y = drand48()*globals.maxy;
      bird->z = drand48()*globals.maxz;

      if (drand48() > 0.5)
	 bird->x += globals.maxx;
      else
	 bird->x -= globals.maxx;

      //bird->picture = surface; 
      bird->iw = 1;
      bird->ih = 1;
      r2i(bird);
      //bird->ix = globals.maxx*bird->x;
      //bird->iy = globals.maxy*bird->y;
      //bird->iz = globals.maxz*bird->z;
      bird->sx = (0.5-drand48());
      P("init %f\n",bird->sx);
      bird->sy = (0.5-drand48());
      bird->sz = (0.5-drand48());
      normalize_speed(bird,globals.meanspeed);
      P("speed: %d %f\n",i,sqrtf(sq3(bird->sx,bird->sy,bird->sz)));
      bird->drawable = 1;
      bird->wingstate = drand48()*NWINGS;

      kd_insert3f(kd, bird->x, bird->y, bird->z, 0);
   }
}

#ifdef FORREAL
static int do_test()
{
   if (Flags.Done)
   {
      R("stop\n");
      return FALSE;
   }
   counter++;
   P("attrxyz: %d %f %f %f\n",counter,globals.attrx,globals.attry,globals.attrz);
   P("range: %f %f\n",globals.range,globals.maxrange);
   return TRUE;
}
#endif

static int do_wings()
{
   if (Flags.Done)
      return FALSE;
   if (!Flags.ShowBirds)
      return TRUE;
   int i;
   for (i=0; i<Nbirds; i++)
   {
      BirdType *bird = &birds[i];
      bird->wingstate++;
      if (bird->wingstate >= NWINGS)
	 bird->wingstate = 0;
   }
   return TRUE;
}

#ifdef FORREAL
static char sbuffer[1024];
static int do_check_flags()
{
   if (globals.restart_requested)
   {
      P("TD: init_birds\n");
      init_birds(0);
      globals.restart_requested = 0;
   }
   if (globals.nbirds_changed)
   {
      R("nbirds changed %d %d\n",Nbirds,globals.nbirds_new);
      if (globals.nbirds_new <= 0)
	 globals.nbirds_new = 1;
      if (globals.nbirds_new > globals.nbirds_max)
	 globals.nbirds_new = globals.nbirds_max;

      R("TD: init_birds\n");
      int start = Nbirds;
      Nbirds = globals.nbirds_new;
      sprintf(sbuffer,"%d",Nbirds);
      nbirds_set_input_value(sbuffer);
      init_birds(start);
      globals.nbirds_changed = 0;
   }
   if (globals.neighbours_changed)
   {
      R("neighbours changed: %d %d\n",globals.neighbours,globals.neighbours_new);
      globals.neighbours = globals.neighbours_new;
      sprintf(sbuffer,"%d",globals.neighbours);
      neighbours_set_input_value(sbuffer);
      globals.neighbours_changed = 0;
   }
   if (globals.prefdistance_changed)
   {
      R("prefdistance changed: %.2f %.2f\n",globals.prefdistance,globals.prefdistance_new);
      globals.prefdistance = globals.prefdistance_new;
      sprintf(sbuffer,"%.2f",globals.prefdistance);
      prefdistance_set_input_value(sbuffer);
      globals.prefdistance_changed = 0;
   }
   if (globals.followers_changed)
   {
      R("followers changed: %.2f %.2f\n",globals.followers,globals.followers_new);
      globals.followers = globals.followers_new;
      sprintf(sbuffer,"%.2f",globals.followers);
      followers_set_input_value(sbuffer);
      globals.followers_changed = 0;
   }
   if (globals.freeze_requested)
   {
      R("freeze\n");
      globals.freeze = ~globals.freeze;
      globals.freeze_requested = 0;
   }
   return TRUE;
}
#endif

float birds_get_range()
{
   return globals.range;
}

float birds_get_mean_dist()
{
   return globals.mean_distance;
}

void birds_set_attraction_point_relative(float x, float y, float z)
{
   globals.attrx = globals.maxx*x;
   globals.attry = globals.maxy*y;
   globals.attrz = globals.maxz*z;
#ifdef SHOW_ATTRACTION_POINT
   testbird.x = globals.attrx;
   testbird.y = globals.attry;
   testbird.z = globals.attrz;
#endif
}

void clear_flags()
{
#define DOITB(what,type) \
   globals.what ## _changed = 0;
   DOITALLB();
#undef DOITB
#define DOITB(what) \
   globals.what ## _requested   = 0;
   BUTTONALL();
#undef DOITB
}

void birds_set_speed(int x)
{
   globals.meanspeed = x*0.01*globals.maxx*0.05;
   P("%f\n",globals.meanspeed);
}

static void main_window(GtkWidget *window)
{
   globals.maxix = gtk_widget_get_allocated_width(window);
   globals.maxiz = gtk_widget_get_allocated_height(window);
   globals.maxiy = (globals.maxix+globals.maxiz)/2;

   R("%d %d %d\n",globals.maxix,globals.maxiy,globals.maxiz);

   drawing_area = gtk_drawing_area_new();
   gtk_container_add(GTK_CONTAINER(window), drawing_area);

   gtk_widget_show_all(window);

   globals.maxz = globals.maxx*(float)globals.maxiz/(float)globals.maxix;
   globals.maxy = globals.maxx*(float)globals.maxiy/(float)globals.maxix;
   //globals.vd   = 0.2*globals.maxy;
   globals.xc   = (globals.maxx-globals.ox)/2;
   globals.zc   = (globals.maxz-globals.oz)/2;

   R("drawing window: %d %d %d %f %f %f\n",
	 globals.maxix, globals.maxiy, globals.maxiz, globals.maxx, globals.maxy,globals.maxz);
   gtk_window_set_title(GTK_WINDOW(window),"xflock_birds");
}

void birds_init_color(const char *color)
{
   int i;
   for (i=0; i<NBIRDPIXBUFS; i++)
   {
      g_object_unref(bird_pixbufs[i]);
   }
   init_bird_pixbufs(color);
   table_clear((void(*)(void *))cairo_surface_destroy);
}

static void init_bird_pixbufs(const char *color)
{
   int i;
   for (i=0; i<NBIRDPIXBUFS; i++)
   {
      int n;
      sscanf(birds_xpm[i][0],"%*d %d",&n);
      P("n= %d\n",n);
      char **x = (char**)malloc(sizeof(char *)*(n+3));
      int j;
      for (j=0; j<2; j++)
	 x[j] = strdup(birds_xpm[i][j]);
      x[2] = (char *)malloc(4+sizeof(color));
      x[2][0] = 0;
      strcat(x[2],". c ");
      strcat(x[2],color);
      P("c: [%s]\n",x[2]);

      for (j=3; j<n+3; j++)
      {
	 x[j] = strdup(birds_xpm[i][j]);
	 P("%d %s\n",j,x[j]);
      }

      //bird_pixbufs[i] = gdk_pixbuf_new_from_xpm_data(birds_xpm[i]);
      bird_pixbufs[i] = gdk_pixbuf_new_from_xpm_data((const char **)x);
      for (j=0; j<n+3; j++)
	 free(x[j]);
      free(x);
   }
}

void main_birds (GtkWidget *window)
{
   if (!window)
      return;
   static int running = 0;

   if (running)
   {
      main_window(window);
      return;
   }
   else
   {
      running = 1;
      P("%d\n",counter++);
      //globals.neighbours     = 7;
      globals.neighbours_max = 100;
      globals.range          = 20;
      globals.freeze         = 0;
      //globals.followers      = 0.5;
      globals.maxx           = 1000;    // meters
      globals.bird_scale     = 32;

      globals.prefdweight    = 1;

      clear_flags();
      add_to_mainloop(G_PRIORITY_DEFAULT,time_update_pos_birds,     do_update_pos_birds,     0);
      add_to_mainloop(G_PRIORITY_DEFAULT,time_update_speed_birds,   do_update_speed_birds,   0);
      add_to_mainloop(G_PRIORITY_DEFAULT,time_draw_birds,           do_draw_birds,           0);
      add_to_mainloop(G_PRIORITY_DEFAULT,time_wings,                do_wings,                0);
#ifdef FORREAL
      add_to_mainloop(G_PRIORITY_DEFAULT,time_check_flags,          do_check_flags,          0);
      add_to_mainloop(G_PRIORITY_DEFAULT,time_test,                 do_test,                 0);
      add_to_mainloop(G_PRIORITY_DEFAULT,time_update_range,         do_update_range,         0); // ui.c
      add_to_mainloop(G_PRIORITY_DEFAULT,time_update_mean_distance, do_update_mean_distance, 0); // ui.c
#endif
      main_window(window);
   }


   globals.attrx = globals.maxx/2;
   globals.attry = globals.maxy/2;
   globals.attrz = globals.maxz/2;

#ifdef SHOW_ATTRACTION_POINT
   testbird.x = globals.attrx;
   testbird.y = globals.attry;
   testbird.z = globals.attrz;
#endif

   //globals.meanspeed = globals.maxx/10;
   //globals.meanspeed = globals.maxx/20;
   globals.meanspeed = 0;

   globals.ax = globals.maxix/globals.maxx;
   globals.ay = globals.maxiy/globals.maxy;
   globals.az = globals.maxiz/globals.maxz;

   globals.ox = 0;
   globals.oy = 0;
   globals.oz = 0;

   globals.maxrange = globals.maxx-globals.ox+ globals.maxy-globals.oy+ globals.maxz-globals.oz;


   init_bird_pixbufs("black");


#ifdef FORREAL
   ui();

   snprintf(sbuffer,sizeof(sbuffer),"%d",Nbirds);
   R("%s\n",sbuffer);
   nbirds_set_input_value(sbuffer);
   snprintf(sbuffer,sizeof(sbuffer),"%d",globals.neighbours);
   neighbours_set_input_value(sbuffer);
   snprintf(sbuffer,sizeof(sbuffer),"%.2f",globals.prefdistance);
   prefdistance_set_input_value(sbuffer);
   snprintf(sbuffer,sizeof(sbuffer),"%.2f",globals.followers);
   followers_set_input_value(sbuffer);

   init_birds(0);
   gtk_main();
#endif

   init_birds(0);

#if 0
   printf("\nThank you for using xflock\n");
   return 0;
#endif

}
