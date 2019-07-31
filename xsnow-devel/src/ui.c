/* -copyright-
#-#
#-# xsnow: let it snow on your desktop
#-# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#-#               2019 Willem Vermin
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
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "clocks.h"
#include "ui.h"
#include "ui_xml.h"
#include "xsnow.h"
#include "flags.h"
#include "csvpos.h"
#include "pixmaps.h"
#include "version.h"

#ifndef DEBUG
#define DEBUG
#endif
#undef DEBUG

#ifdef DEBUG
#define P(...) g_printf ("%s: %d: ",__FILE__,__LINE__);g_printf(__VA_ARGS__)
#else
#define P(...) 
#endif

#define PREFIX_SANTA   "santa-"
#define PREFIX_TREE    "tree-"

#define SANTA2(x) SANTA(x) SANTA(x ## r)
#define SANTA_ALL SANTA2(0) SANTA2(1) SANTA2(2) SANTA2(3) SANTA2(4)

#define TREE_ALL TREE(0) TREE(1) TREE(2) TREE(3) TREE(4) TREE(5) TREE(6)


#define HANDLE_RANGE(_name,_flag,_value) \
   G_MODULE_EXPORT void _name(GtkWidget *w, gpointer d)\
{\
   if(!human_interaction) return;\
   gdouble value;\
   value = gtk_range_get_value(GTK_RANGE(w));\
   flags._flag = lrint(_value);\
   P(#_name ": %d\n",flags._flag);\
}

#define HANDLE_TOGGLE(_name,_flag,_t,_f) \
   G_MODULE_EXPORT \
   void _name(GtkWidget *w, gpointer d) \
{ \
   if(!human_interaction) return; \
   gint active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)); \
   if(active) \
   flags._flag = _t; \
   else \
   flags._flag = _f; \
   P(#_name ": %d\n",flags._flag); \
}

#define HANDLE_COLOR(_name,_flag) \
   G_MODULE_EXPORT \
   void _name(GtkWidget *w, gpointer d) \
{ \
   if(!human_interaction) return; \
   GdkRGBA color; \
   gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(w),&color); \
   free(flags._flag); \
   rgba2color(&color,&flags._flag); \
   P(#_name ": %s\n",flags._flag); \
}

#define HANDLE_SET_COLOR(_button,_flag) \
   do { \
      GdkRGBA color; \
      gdk_rgba_parse(&color,flags._flag); \
      gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(_button),&color); \
   } while(0)

#define HANDLE_INIT(_button,_id) \
   _button = GTK_WIDGET(gtk_builder_get_object(builder,#_id))

#define HANDLE_SET_RANGE(_button,_flag,_fun) \
   gtk_range_set_value(GTK_RANGE(_button), _fun((gdouble)flags._flag))

#define HANDLE_SET_TOGGLE_(_button,_x) \
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_button),_x)

#define HANDLE_SET_TOGGLE(_button,_flag)\
   HANDLE_SET_TOGGLE_(_button,flags._flag)

#define HANDLE_SET_TOGGLE_I(_button,_flag) \
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_button),!flags._flag)

#define self(x) (x)

long int counter = 0;
GtkBuilder *builder;

static void set_buttons(void);
static void set_santa_buttons(void);
static void set_tree_buttons(void);
static void set_star_buttons(void);
static void set_meteo_buttons(void);
static int human_interaction = 1;

static GtkWidget *hauptfenster;

   G_MODULE_EXPORT
void button_iconify(GtkWidget *w, gpointer p)
{
   P("button_iconify\n");
   gtk_window_iconify(GTK_WINDOW(hauptfenster));
}

typedef struct santa_button
{
   char *imid;
   GtkWidget *button;
   gdouble value;
} santa_button;

#define NBUTTONS (2*(MAXSANTA+1)) 
// NBUTTONS is number of Santa's too choose from
#define SANTA(x) santa_button santa_ ## x;
static struct santa_buttons
{
   SANTA_ALL

      santa_button santa_show;
   santa_button santa_speed;
} santa_buttons;
#undef SANTA


#define SANTA(x) &santa_buttons.santa_ ## x,
static santa_button *santa_barray[NBUTTONS]=
{
   SANTA_ALL
};
#undef SANTA

static void init_santa_buttons()
{
#define SANTA(x) \
   santa_buttons.santa_ ## x.button = GTK_WIDGET(gtk_builder_get_object(builder,PREFIX_SANTA #x)); 
   SANTA_ALL;
#undef SANTA
#define SANTA(x) \
   gtk_widget_set_name(santa_buttons.santa_ ## x.button,PREFIX_SANTA #x);
   SANTA_ALL;
#undef SANTA

   HANDLE_INIT(santa_buttons.santa_show.button  ,santa-show);
   HANDLE_INIT(santa_buttons.santa_speed.button ,santa-speed);
}

static void set_santa_buttons()
{
   int n = 2*flags.SantaSize;
   if (!flags.NoRudolf)
      n++;
   if (n<NBUTTONS)
      HANDLE_SET_TOGGLE_(santa_barray[n]->button,TRUE);

   HANDLE_SET_TOGGLE(santa_buttons.santa_show.button,NoSanta);
   HANDLE_SET_RANGE(santa_buttons.santa_speed.button,SantaSpeedFactor,log10);
}

   G_MODULE_EXPORT 
void button_santa(GtkWidget *w, gpointer d)
{
   if(!human_interaction) return;
   if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
   const gchar *s  = gtk_widget_get_name(w)+strlen(PREFIX_SANTA);
   int santa_type  = atoi(s);
   int have_rudolf = ('r' == s[strlen(s)-1]);
   P("button_santa: Santa %d Rudolf %d\n",santa_type,have_rudolf);
   flags.SantaSize = santa_type;
   flags.NoRudolf  = !have_rudolf;
}

HANDLE_TOGGLE(button_santa_show, NoSanta, 1, 0)

HANDLE_RANGE(button_santa_speed, SantaSpeedFactor, pow(10.0,value))

void santa_default(int vintage)
{
   int h = human_interaction;
   human_interaction      = 0;
   flags.SantaSize        = DEFAULT_SantaSize;
   flags.NoRudolf         = DEFAULT_NoRudolf; 
   flags.SantaSpeedFactor = DEFAULT_SantaSpeedFactor;
   flags.NoSanta          = DEFAULT_NoSanta;
   if(vintage)
   {
      flags.SantaSize = VINTAGE_SantaSize;
      flags.NoRudolf  = VINTAGE_NoRudolf; 
   }
   set_santa_buttons();
   human_interaction      = h;
}

   G_MODULE_EXPORT 
void button_defaults_santa(GtkWidget *w, gpointer d)
{
   P("button_defaults_santa defaults\n");
   santa_default(0);
}

   G_MODULE_EXPORT 
void button_vintage_santa(GtkWidget *w, gpointer d)
{
   P("button_defaults_santa vintage\n");
   santa_default(1);
}

typedef struct tree_button
{
   GtkWidget *button;
}tree_button;

#define TREE(x) tree_button tree_ ## x;
static struct tree_buttons
{
   TREE_ALL

      tree_button desired_trees;
   tree_button tree_fill;
   tree_button show;
   tree_button color;
} tree_buttons;
#undef TREE

typedef struct star_button
{
   GtkWidget *button;
} star_button;

static struct star_buttons
{
   star_button nstars;
} star_buttons;

typedef struct meteo_button
{
   GtkWidget *button;
} meteo_button;

static struct meteo_buttons
{
   meteo_button show;
} meteo_buttons;

static void report_tree_type(int p, gint active)
{
   P("Tree: %d %d %s\n",p,active,flags.TreeType);
   int *a;
   int n;
   csvpos(flags.TreeType,&a,&n);
   if(active)
   {
      a = realloc(a,sizeof(*a)*(n+1));
      a[n] = p;
      n++;
   }
   else
   {
      int i;
      for (i=0; i<n; i++)
	 if(a[i] == p)
	    a[i] = -1;
   }
   int *b = malloc(sizeof(*b)*n);
   int i,m=0;
   for(i=0; i<n; i++)
   {
      int j;
      int unique = (a[i] >= 0);
      if(unique)
	 for (j=0; j<m; j++)
	    if(a[i] == b[j])
	    {
	       unique = 0;
	       break;
	    }
      if(unique)
      {
	 b[m] = a[i];
	 m++;
      }
   }
   free(flags.TreeType);
   vsc(&flags.TreeType,b,m);
   free(a);
   free(b);
   P("Tree_Type set to %s\n",flags.TreeType);
}

   G_MODULE_EXPORT
void button_tree(GtkWidget *w, gpointer d)
{
   if(!human_interaction) return;
   gint active;
   active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
   const gchar *s  = gtk_widget_get_name(w)+strlen(PREFIX_TREE);
   int p = atoi(s);
   report_tree_type(p,active);
   P("button_tree: tree: %d active: %d\n",p,active);
}

void scenery_default(int vintage)
{
   int h = human_interaction;
   human_interaction = 0;
   flags.desired_number_of_trees = DEFAULT_desired_number_of_trees; 
   free(flags.TreeType);
   flags.TreeType                = strdup(DEFAULT_TreeType);
   flags.nstars                  = DEFAULT_nstars;
   flags.NoMeteorites            = DEFAULT_NoMeteorites;
   flags.NoTrees                 = DEFAULT_NoTrees;
   flags.treefill                = DEFAULT_treefill;
   free(flags.trColor);
   flags.trColor                 = strdup(DEFAULT_trColor);
   if (vintage)
   {
      flags.desired_number_of_trees = VINTAGE_desired_number_of_trees; 
      free(flags.TreeType);
      flags.TreeType                = strdup(VINTAGE_TreeType);
      flags.nstars                  = VINTAGE_nstars;
      flags.NoMeteorites            = VINTAGE_NoMeteorites;
   }
   set_tree_buttons();
   set_star_buttons();
   set_meteo_buttons();
   human_interaction = h;
}

   G_MODULE_EXPORT
void button_defaults_scenery(GtkWidget *w, gpointer d)
{
   P("button_defaults_scenery\n");
   scenery_default(0);
}

   G_MODULE_EXPORT
void button_vintage_scenery(GtkWidget *w, gpointer d)
{
   P("button_vintage_scenery\n");
   scenery_default(1);
}


static void init_tree_buttons()
{

#define TREE(x) \
   tree_buttons.tree_##x.button = GTK_WIDGET(gtk_builder_get_object(builder,PREFIX_TREE #x));
   TREE_ALL;
#undef TREE
#define TREE(x) \
   gtk_widget_set_name(tree_buttons.tree_##x.button,PREFIX_TREE #x);
   TREE_ALL;
#undef TREE
   HANDLE_INIT(tree_buttons.desired_trees.button,tree-ntrees);
   HANDLE_INIT(tree_buttons.tree_fill.button,tree-fill);
   HANDLE_INIT(tree_buttons.show.button,tree-show);
   HANDLE_INIT(tree_buttons.color.button,tree-treecolor0);
}

static void init_santa_pixmaps()
{
#define SANTA(x) santa_buttons.santa_ ## x.imid  = PREFIX_SANTA # x "-imid";
   SANTA_ALL;
#undef SANTA

   int i;
   GtkImage *image; 
   GdkPixbuf *pixbuf;
   for (i=0; i<NBUTTONS; i++)
   {
      pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)Santas[i/2][i%2][0]);
      image = GTK_IMAGE(gtk_builder_get_object(builder,santa_barray[i]->imid));
      gtk_image_set_from_pixbuf(image,pixbuf);
      g_object_unref(pixbuf);
   }
}

static void init_tree_pixmaps()
{
   GtkImage *image; 
   GdkPixbuf *pixbuf;
#define TREE(x) \
   pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)xpmtrees[x]);\
   image = GTK_IMAGE(gtk_builder_get_object(builder,"treeimage" # x));\
   gtk_image_set_from_pixbuf(image,pixbuf); \
   g_object_unref(pixbuf);

   TREE_ALL;
#undef TREE
}

static void init_hello_pixmaps()
{
   GtkImage *image; 
   GdkPixbuf *pixbuf;
   pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)xsnow_logo);
   image = GTK_IMAGE(gtk_builder_get_object(builder,"hello-image1"));
   gtk_image_set_from_pixbuf(image,pixbuf);
   image = GTK_IMAGE(gtk_builder_get_object(builder,"hello-image2"));
   gtk_image_set_from_pixbuf(image,pixbuf);
   g_object_unref(pixbuf);
   pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)xpmtrees[0]);
   image = GTK_IMAGE(gtk_builder_get_object(builder,"hello-image3"));
   gtk_image_set_from_pixbuf(image,pixbuf);
   g_object_unref(pixbuf);
}

static void init_pixmaps()
{
   init_santa_pixmaps();
   init_tree_pixmaps();
   init_hello_pixmaps();
}

HANDLE_RANGE(button_ntrees,desired_number_of_trees,value)

HANDLE_RANGE(button_tree_fill, treefill, value)

HANDLE_TOGGLE(button_show_trees,NoTrees,0,1)

static void rgba2color(GdkRGBA *c, char **s)
{
   *s = malloc(8);
   sprintf(*s,"#%02lx%02lx%02lx",lrint(c->red*255),lrint(c->green*255),lrint(c->blue*255));
}


HANDLE_COLOR(button_tree_color,trColor)

static void set_tree_buttons()
{

#define TREE(x)\
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tree_buttons.tree_##x.button),FALSE);
   TREE_ALL;
#undef TREE
   int i;
   int *a,n;
   csvpos(flags.TreeType,&a,&n);

   for (i=0; i<n; i++)
   {
      // P("set_tree_buttons::::::::::::::::::::: %s %d %d\n",flags.TreeType,n,a[i]);
      switch (a[i])
      {
#define TREE(x) \
	 case x: gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tree_buttons.tree_##x.button),TRUE);\
		 break;
	 TREE_ALL;
#undef TREE
      }
   }
   free(a);
   HANDLE_SET_RANGE(tree_buttons.desired_trees.button ,desired_number_of_trees ,self);
   HANDLE_SET_RANGE(tree_buttons.tree_fill.button     ,treefill                ,self);
   HANDLE_SET_TOGGLE_I(tree_buttons.show.button,NoTrees);

   HANDLE_SET_COLOR(tree_buttons.color.button,trColor);
}


HANDLE_RANGE(button_star_nstars, nstars, value)

static void init_star_buttons()
{
   HANDLE_INIT(star_buttons.nstars.button,stars-nstars);
}

static void set_star_buttons()
{
   HANDLE_SET_RANGE(star_buttons.nstars.button,nstars,self);
}

HANDLE_TOGGLE(button_meteo_show, NoMeteorites, 0,1)

static void init_meteo_buttons()
{
   HANDLE_INIT(meteo_buttons.show.button,meteo-show);
}

static void set_meteo_buttons()
{
   HANDLE_SET_TOGGLE_I(meteo_buttons.show.button,NoMeteorites);
}

typedef struct general_button
{
   GtkWidget *button;
}general_button;

static struct general_buttons
{
   general_button cpuload;
   general_button usebg;
   general_button bgcolor;
   general_button alpha;
   general_button exposures;
   general_button kdebg;
   general_button lift;
   general_button fullscreen;
   general_button below;
} general_buttons;

static void init_general_buttons()
{
   HANDLE_INIT(general_buttons.cpuload.button,general-cpuload);
   HANDLE_INIT(general_buttons.usebg.button,general-usebg);
   HANDLE_INIT(general_buttons.bgcolor.button,general-bgcolor);
   HANDLE_INIT(general_buttons.alpha.button,general-alpha);
   HANDLE_INIT(general_buttons.exposures.button,general-exposures);
   HANDLE_INIT(general_buttons.kdebg.button,general-kde-background);
   HANDLE_INIT(general_buttons.lift.button,general-lift);
   HANDLE_INIT(general_buttons.fullscreen.button,general-fullscreen);
   HANDLE_INIT(general_buttons.below.button,general-below);
   gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(builder,"general-version")),"xsnow version " VERSION);
}

static void set_general_buttons()
{
   HANDLE_SET_RANGE(general_buttons.cpuload.button,cpuload,self);
   HANDLE_SET_TOGGLE(general_buttons.usebg.button,usebg);
   HANDLE_SET_COLOR(general_buttons.bgcolor.button,bgcolor);
   HANDLE_SET_TOGGLE(general_buttons.alpha.button,usealpha);
   HANDLE_SET_TOGGLE(general_buttons.kdebg.button,KDEbg);
   HANDLE_SET_RANGE(general_buttons.lift.button,offset_s,-self);
   HANDLE_SET_TOGGLE(general_buttons.fullscreen.button,fullscreen);
   HANDLE_SET_TOGGLE(general_buttons.below.button,below);
   if (flags.exposures != -SOMENUMBER)
      HANDLE_SET_TOGGLE(general_buttons.exposures.button,exposures);
   else
      HANDLE_SET_TOGGLE_(general_buttons.exposures.button,0);
}

   G_MODULE_EXPORT
void button_cpuload(GtkWidget *w, gpointer d)
{
   if(!human_interaction) return;
   gdouble value;
   value = gtk_range_get_value(GTK_RANGE(w));
   flags.cpuload = lrint(value);
   P("button_cpuload: %d\n",flags.cpuload);
}

   HANDLE_TOGGLE(button_use_bgcolor, usebg, 1,0)
HANDLE_TOGGLE(button_kde_background, KDEbg, 1, 0)

   G_MODULE_EXPORT
void button_bgcolor(GtkWidget *w, gpointer d)
{
   if(!human_interaction) return;
   GdkRGBA color;
   gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(w),&color);
   free(flags.bgcolor);
   rgba2color(&color,&flags.bgcolor);
   P("button_bgcolor: %s\n",flags.bgcolor);
}

   HANDLE_TOGGLE(button_alpha, usealpha, 1,0)
   HANDLE_TOGGLE(button_exposures, exposures, 1,0)
   HANDLE_TOGGLE(button_fullscreen, fullscreen, 1,0)
   HANDLE_TOGGLE(button_below, below, 1,0)
HANDLE_RANGE(button_lift, offset_s, -value)

   G_MODULE_EXPORT
void button_quit(GtkWidget *w, gpointer d)
{
   flags.done = 1;
   P("button_quit: %d\n",flags.done);
}

void general_default(int vintage)
{
   int h = human_interaction;
   human_interaction      = 0;

   flags.cpuload    = DEFAULT_cpuload;
   flags.usebg      = DEFAULT_usebg;
   free(flags.bgcolor);
   flags.bgcolor    = strdup(DEFAULT_bgcolor);
   flags.usealpha   = DEFAULT_usealpha;
   flags.exposures  = DEFAULT_exposures;
   flags.offset_s   = DEFAULT_offset_s;
   flags.KDEbg      = DEFAULT_KDEbg;
   flags.fullscreen = DEFAULT_fullscreen;
   flags.below      = DEFAULT_below;
   if (vintage)
   {
   }
   set_general_buttons();
   human_interaction      = h;
}


   G_MODULE_EXPORT 
void button_defaults_general(GtkWidget *w, gpointer d)
{
   P("button_defaults_general\n");
   general_default(0);
}

   G_MODULE_EXPORT 
void button_vintage_general(GtkWidget *w, gpointer d)
{
   P("button_defaults_general vintage\n");
   general_default(1);
}

typedef struct snow_button
{
   GtkWidget *button;
}snow_button;

static struct snow_buttons
{
   snow_button show_snow;
   snow_button show_snow_blowoff;
   snow_button intensity;
   snow_button blowoff_intensity;
   snow_button speed;
   snow_button color;
   snow_button windows;
   snow_button windows_show;
   snow_button bottom;
   snow_button bottom_show;
   snow_button trees;
   snow_button trees_show;
   snow_button fluff_show;
   snow_button fallen_show;
} snow_buttons;

static void init_snow_buttons()
{
   HANDLE_INIT(snow_buttons.show_snow.button             ,snow-show);
   HANDLE_INIT(snow_buttons.show_snow_blowoff.button     ,snow-show-blowoff);
   HANDLE_INIT(snow_buttons.intensity.button             ,snow-intensity);
   HANDLE_INIT(snow_buttons.blowoff_intensity.button     ,snow-blowoff-intensity);
   HANDLE_INIT(snow_buttons.speed.button                 ,snow-speed);
   HANDLE_INIT(snow_buttons.color.button                 ,snow-color);
   HANDLE_INIT(snow_buttons.windows.button               ,snow-windows);
   HANDLE_INIT(snow_buttons.windows_show.button          ,snow-windows-show);
   HANDLE_INIT(snow_buttons.bottom.button                ,snow-bottom);
   HANDLE_INIT(snow_buttons.bottom_show.button           ,snow-bottom-show);
   HANDLE_INIT(snow_buttons.trees.button                 ,snow-trees);
   HANDLE_INIT(snow_buttons.trees_show.button            ,snow-trees-show);
   HANDLE_INIT(snow_buttons.fluff_show.button            ,snow-fluff-show);
}


static void set_snow_buttons()
{
   HANDLE_SET_TOGGLE_I(snow_buttons.show_snow.button          ,NoSnowFlakes);
   HANDLE_SET_TOGGLE_I(snow_buttons.show_snow_blowoff.button  ,NoBlowSnow);
   HANDLE_SET_TOGGLE_I(snow_buttons.windows_show.button       ,NoKeepSWin);
   HANDLE_SET_TOGGLE_I(snow_buttons.bottom_show.button        ,NoKeepSBot);
   HANDLE_SET_TOGGLE_I(snow_buttons.trees_show.button         ,NoKeepSnowOnTrees);
   HANDLE_SET_TOGGLE_I(snow_buttons.fluff_show.button         ,NoFluffy);

   HANDLE_SET_RANGE(snow_buttons.intensity.button             ,snowflakesfactor ,self);
   HANDLE_SET_RANGE(snow_buttons.blowoff_intensity.button     ,blowofffactor    ,self);
   HANDLE_SET_RANGE(snow_buttons.speed.button                 ,SnowSpeedFactor  ,self);
   HANDLE_SET_RANGE(snow_buttons.windows.button               ,MaxWinSnowDepth  ,self);
   HANDLE_SET_RANGE(snow_buttons.bottom.button                ,MaxScrSnowDepth  ,self);
   HANDLE_SET_RANGE(snow_buttons.trees.button                 ,maxontrees       ,self);

   HANDLE_SET_COLOR(snow_buttons.color.button,snowColor);
}


   HANDLE_TOGGLE(button_snow_show_snow     ,NoSnowFlakes       ,0,1)
   HANDLE_TOGGLE(button_snow_show_blowoff  ,NoBlowSnow         ,0,1)
   HANDLE_TOGGLE(button_snow_fluff_show    ,NoFluffy           ,0,1)
   HANDLE_TOGGLE(button_snow_trees_show    ,NoKeepSnowOnTrees  ,0,1)
   HANDLE_TOGGLE(button_snow_bottom_show   ,NoKeepSBot         ,0,1)
HANDLE_TOGGLE(button_snow_windows_show  ,NoKeepSWin         ,0,1)

HANDLE_COLOR(button_snow_color,snowColor)

   HANDLE_RANGE(button_snow_blowoff_intensity   , blowofffactor    ,value)
   HANDLE_RANGE(button_snow_intensity           , snowflakesfactor ,value)
   HANDLE_RANGE(button_snow_speed               , SnowSpeedFactor  ,value)
   HANDLE_RANGE(button_snow_windows             , MaxWinSnowDepth  ,value)
   HANDLE_RANGE(button_snow_bottom              , MaxScrSnowDepth  ,value)
HANDLE_RANGE(button_snow_trees               , maxontrees       ,value)

void snow_default(int vintage)
{
   int h = human_interaction;
   human_interaction = 0;
   flags.NoBlowSnow        = DEFAULT_NoBlowSnow;
   flags.snowflakesfactor  = DEFAULT_snowflakesfactor;
   flags.NoSnowFlakes      = DEFAULT_NoSnowFlakes;
   free(flags.snowColor);
   flags.snowColor         = strdup(DEFAULT_snowColor);
   flags.SnowSpeedFactor   = DEFAULT_SnowSpeedFactor;
   flags.blowofffactor     = DEFAULT_blowofffactor;
   flags.MaxWinSnowDepth   = DEFAULT_MaxWinSnowDepth;
   flags.MaxScrSnowDepth   = DEFAULT_MaxScrSnowDepth;
   flags.maxontrees        = DEFAULT_maxontrees;
   flags.NoKeepSWin        = DEFAULT_NoKeepSWin;
   flags.NoKeepSBot        = DEFAULT_NoKeepSBot;
   flags.NoKeepSnowOnTrees = DEFAULT_NoKeepSnowOnTrees;
   flags.NoFluffy          = DEFAULT_NoFluffy;
   if(vintage)
   {
      flags.NoBlowSnow        = VINTAGE_NoBlowSnow;
      flags.snowflakesfactor  = VINTAGE_snowflakesfactor;
      flags.NoKeepSnowOnTrees = VINTAGE_NoKeepSnowOnTrees;
   }
   set_snow_buttons();
   human_interaction = h;
}

   G_MODULE_EXPORT
void button_defaults_snow(GtkWidget *w, gpointer d)
{
   P("button_defaults_snow\n");
   snow_default(0);
}

   G_MODULE_EXPORT
void button_vintage_snow(GtkWidget *w, gpointer d)
{
   P("button_vintage_snow\n");
   snow_default(1);
}


typedef struct wind_button
{
   GtkWidget *button;
}wind_button;

static struct wind_buttons
{
   wind_button windy;
   wind_button whirl;
   wind_button timer;
} wind_buttons;

static void init_wind_buttons()
{
   HANDLE_INIT(wind_buttons.windy.button             ,wind-windy);
   HANDLE_INIT(wind_buttons.whirl.button             ,wind-whirl);
   HANDLE_INIT(wind_buttons.timer.button             ,wind-timer);
}

static void set_wind_buttons()
{
   HANDLE_SET_TOGGLE_I(wind_buttons.windy.button     ,NoWind);

   HANDLE_SET_RANGE(wind_buttons.whirl.button        ,WhirlFactor ,self);
   HANDLE_SET_RANGE(wind_buttons.timer.button        ,WindTimer   ,self);
}

   HANDLE_TOGGLE(button_wind_windy,NoWind   ,0,1)
   HANDLE_RANGE(button_wind_whirl           ,WhirlFactor ,value)
HANDLE_RANGE(button_wind_timer           ,WindTimer   ,value)

   G_MODULE_EXPORT
void button_wind_activate(GtkWidget *w, gpointer p)
{
   P("button_wind_activate\n");
   flags.windnow = 1;
}

void wind_default(int vintage)
{
   int h = human_interaction;
   human_interaction = 0;
   flags.NoWind        = DEFAULT_NoWind;
   flags.WhirlFactor   = DEFAULT_WhirlFactor;
   flags.WindTimer     = DEFAULT_WindTimer;
   if(vintage)
   {
   }
   set_wind_buttons();
   human_interaction = h;
}

   G_MODULE_EXPORT
void button_defaults_wind(GtkWidget *w, gpointer d)
{
   P("button_defaults_wind\n");
   wind_default(0);
}

   G_MODULE_EXPORT
void button_vintage_wind(GtkWidget *w, gpointer d)
{
   P("button_vintage_wind\n");
   wind_default(1);
}

static void init_buttons()
{
   init_santa_buttons();
   init_tree_buttons();
   init_star_buttons();
   init_meteo_buttons();
   init_snow_buttons();
   init_general_buttons();
   init_wind_buttons();
}

static void set_buttons()
{
   human_interaction = 0;
   set_santa_buttons();
   set_tree_buttons();
   set_star_buttons();
   set_meteo_buttons();
   set_snow_buttons();
   set_general_buttons();
   set_wind_buttons();
   human_interaction = 1;
}

void all_default(int vintage)
{
   santa_default(vintage);
   // general_default(vintage);
   scenery_default(vintage);
   snow_default(vintage);
   wind_default(vintage);
}
   G_MODULE_EXPORT
void button_all_defaults()
{
   P("button_all_defaults\n");
   all_default(0);
}
   G_MODULE_EXPORT
void button_all_vintage()
{
   P("button_all_vintage\n");
   all_default(1);
}

   G_MODULE_EXPORT
void button_kde()
{
   int h = human_interaction;
   human_interaction      = 0;
   P("button_kde\n");
   KDEFLAGS;
   set_general_buttons();
   human_interaction = h;
}

   G_MODULE_EXPORT
void button_fvwm()
{
   int h = human_interaction;
   human_interaction      = 0;
   P("button_fvwm\n");
   FVWMFLAGS;
   set_general_buttons();
   human_interaction = h;
}

   G_MODULE_EXPORT
void button_gnome()
{
   int h = human_interaction;
   human_interaction      = 0;
   P("button_gnome\n");
   GNOMEFLAGS;
   set_general_buttons();
   human_interaction = h;
}

void ui (int *argc, char **argv[])
{

   // gtk_init(argc, argv);
   builder = gtk_builder_new_from_string (xsnow_xml, -1);
   gtk_builder_connect_signals (builder, builder);
   hauptfenster = GTK_WIDGET(gtk_builder_get_object (builder, "hauptfenster"));
   gtk_widget_show_all (hauptfenster);

   init_buttons();
   init_pixmaps();
   set_buttons();

   double t0 = wallclock();
   while (wallclock() - t0 < 1.0)
      gtk_main_iteration_do(0);
}

void ui_loop()
{
   gtk_main_iteration_do(0);
}

void ui_error_x11(int *argc, char **argv[])
{
   GtkWidget *errorfenster;
   GObject *button;
   builder = gtk_builder_new_from_string (xsnow_xml, -1);
   gtk_builder_connect_signals (builder, builder);
   errorfenster = GTK_WIDGET(gtk_builder_get_object (builder, "error_x11_fenster"));
   button = gtk_builder_get_object(builder,"error_x11_ok_button");
   g_signal_connect (button,"clicked",G_CALLBACK(gtk_main_quit),NULL);

   GtkImage *image; 
   GdkPixbuf *pixbuf;
   pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)xsnow_logo);
   image = GTK_IMAGE(gtk_builder_get_object(builder,"error-x11-image1"));
   gtk_image_set_from_pixbuf(image,pixbuf);
   image = GTK_IMAGE(gtk_builder_get_object(builder,"error-x11-image2"));
   gtk_image_set_from_pixbuf(image,pixbuf);
   g_object_unref(pixbuf);

   gtk_widget_show_all (errorfenster);
   gtk_main();
}
