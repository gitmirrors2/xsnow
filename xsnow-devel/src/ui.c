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
#include "buttons.h"
// undef NEWLINE if one wants to examine the by cpp generated code:
// cpp  ui.c | sed 's/NEWLINE/\n/g'
#define NEWLINE
//#undef NEWLINE
#ifdef NEWLINE
#include <gtk/gtk.h>
#include <stdlib.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "birds.h"
#include "clocks.h"
#include "csvpos.h"
#include "flags.h"
#include "pixmaps.h"
#include "snow.h"
#include "ui.h"
#include "ui_xml.h"
#include "utils.h"
#include "varia.h"
#include "version.h"
#include "windows.h"
#include "xsnow.h"

#ifndef DEBUG
#define DEBUG
#endif
#undef DEBUG

#include "debug.h"
#endif   /* NEWLINE */

#ifdef __cplusplus
#define MODULE_EXPORT extern "C" G_MODULE_EXPORT
#else
#define MODULE_EXPORT G_MODULE_EXPORT
#endif


#define PREFIX_SANTA   "santa-"
#define PREFIX_TREE    "tree-"
#define PREFIX_WW      "ww-"

#define SANTA2(x) SANTA(x) SANTA(x ## r)
#define SANTA_ALL SANTA2(0) SANTA2(1) SANTA2(2) SANTA2(3) SANTA2(4)

#define TREE_ALL TREE(0) TREE(1) TREE(2) TREE(3) TREE(4) TREE(5) TREE(6) TREE(7)

static GtkBuilder    *builder;
static GtkWidget     *mean_distance;
static GtkWidget     *range;
static GtkWidget     *desktop_type;
static GtkContainer  *birdsgrid;
static GtkContainer  *moonbox;
#define nsbuffer 512
static char sbuffer[nsbuffer];

static void set_buttons(void);
static void set_santa_buttons(void);
static void set_tree_buttons(void);
static void apply_standard_css(void);
static void birdscb(GtkWidget *w, void *m);
static int  below_confirm_ticker(UNUSED gpointer data);
static void show_bct_countdown(void);
static void yesyes(GtkWidget *w, gpointer data);
static void nono(GtkWidget *w, gpointer data);
static void activate (GtkApplication *app, gpointer user_data);

static int human_interaction = 1;
GtkWidget *nflakeslabel;

static int bct_id;
static int bct_countdown;

// Set the style provider for the widgets
static void apply_css_provider (GtkWidget *widget, GtkCssProvider *cssstyleProvider)
{
   P("apply_css_provider %s\n",gtk_widget_get_name(GTK_WIDGET(widget)));

   gtk_style_context_add_provider ( gtk_widget_get_style_context(widget), 
	 GTK_STYLE_PROVIDER(cssstyleProvider) , 
	 GTK_STYLE_PROVIDER_PRIORITY_USER );

   // For container widgets, apply to every child widget on the container
   if (GTK_IS_CONTAINER (widget))
   {
      gtk_container_forall( GTK_CONTAINER (widget),
	    (GtkCallback)apply_css_provider ,
	    cssstyleProvider);
   }
}

static GtkWidget *hauptfenster;

   MODULE_EXPORT
void button_iconify(UNUSED GtkWidget *w, UNUSED gpointer p)
{
   P("button_iconify\n");
   gtk_window_iconify(GTK_WINDOW(hauptfenster));
}

typedef struct _santa_button
{
   char *imid;
   GtkWidget *button;
   gdouble value;
} santa_button;

#define NBUTTONS (2*(MAXSANTA+1)) 
// NBUTTONS is number of Santa's too choose from
#define SANTA(x) NEWLINE santa_button santa_ ## x;
static struct _santa_buttons
{
   SANTA_ALL
} santa_buttons;
#undef SANTA


#define SANTA(x) NEWLINE &santa_buttons.santa_ ## x,
static santa_button *santa_barray[NBUTTONS]=
{
   SANTA_ALL
};
#undef SANTA

static void init_santa_buttons()
{
#define SANTA(x) \
   NEWLINE santa_buttons.santa_ ## x.button = GTK_WIDGET(gtk_builder_get_object(builder,PREFIX_SANTA #x)); 
   SANTA_ALL;
#undef SANTA
#define SANTA(x) \
   NEWLINE gtk_widget_set_name(santa_buttons.santa_ ## x.button,PREFIX_SANTA #x);
   SANTA_ALL;
#undef SANTA

}

static void set_santa_buttons()
{
   int n = 2*Flags.SantaSize;
   if (Flags.Rudolf)
      n++;
   if (n<NBUTTONS)
      //HANDLE_SET_TOGGLE_(santa_barray[n]->button,TRUE);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(santa_barray[n]->button),TRUE);
}

   MODULE_EXPORT 
void button_santa(GtkWidget *w, UNUSED gpointer d)
{
   if(!human_interaction) return;
   if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
   const gchar *s  = gtk_widget_get_name(w)+strlen(PREFIX_SANTA);
   int santa_type  = atoi(s);
   int have_rudolf = ('r' == s[strlen(s)-1]);
   P("button_santa: Santa %d Rudolf %d s: %s name: %s\n",santa_type,have_rudolf,s,gtk_widget_get_name(w));
   Flags.SantaSize = santa_type;
   Flags.Rudolf  = have_rudolf;
}

void santa_default(int vintage)
{
   int h = human_interaction;
   human_interaction      = 0;
   Flags.SantaSize        = DEFAULT_SantaSize;
   Flags.Rudolf           = DEFAULT_Rudolf; 
   Flags.SantaSpeedFactor = DEFAULT_SantaSpeedFactor;
   Flags.NoSanta          = DEFAULT_NoSanta;
   if(vintage)
   {
      Flags.SantaSize = VINTAGE_SantaSize;
      Flags.Rudolf    = VINTAGE_Rudolf; 
   }
   set_buttons();
   human_interaction      = h;
}

   MODULE_EXPORT 
void button_defaults_santa(UNUSED GtkWidget *w, UNUSED gpointer d)
{
   P("button_defaults_santa defaults\n");
   santa_default(0);
}

   MODULE_EXPORT 
void button_vintage_santa(UNUSED GtkWidget *w, UNUSED gpointer d)
{
   P("button_defaults_santa vintage\n");
   santa_default(1);
}

typedef struct _tree_button
{
   GtkWidget *button;
}tree_button;

#define TREE(x) NEWLINE tree_button tree_ ## x;
static struct _tree_buttons
{
   TREE_ALL
} tree_buttons;
#undef TREE


// creating Button.NStars etc.

#define togglecode(type,name,m) NEWLINE GtkWidget *name;
#define rangecode togglecode
#define colorcode togglecode
static struct _Button 
{
   ALL_BUTTONS
} Button;
#undef togglecode
#undef rangecode
#undef colorcode

// create init_buttons: connect with glade-id

#define togglecode(type,name,m) \
   NEWLINE P("%s %s\n",#name,#type "-" #name); \
   NEWLINE Button.name = GTK_WIDGET(gtk_builder_get_object(builder,#type "-" #name));
#define rangecode togglecode
#define colorcode togglecode

static void init_buttons1()
{
   ALL_BUTTONS
}
#undef togglecode
#undef rangecode
#undef colorcode

// define call backs

#define buttoncb(type,name) button_##type##_##name
#define togglecode(type,name,m) \
   NEWLINE MODULE_EXPORT void button_##type##_##name(GtkWidget *w, UNUSED gpointer d) \
NEWLINE   { \
   NEWLINE    if(!human_interaction) return; \
   NEWLINE    gint active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)); \
   NEWLINE    if(active)  Flags.name = TRUE;  else  Flags.name = FALSE; \
   NEWLINE    if(m<0) Flags.name = !Flags.name;  \
NEWLINE   }

#define rangecode(type,name,m) \
   NEWLINE MODULE_EXPORT void buttoncb(type,name)(GtkWidget *w, UNUSED gpointer d)\
NEWLINE {\
   NEWLINE    if(!human_interaction) return; \
   NEWLINE    gdouble value; \
   NEWLINE    value = gtk_range_get_value(GTK_RANGE(w)); \
   NEWLINE    Flags.name = m*lrint(value); \
NEWLINE }

#define colorcode(type,name,m) \
   NEWLINE MODULE_EXPORT void buttoncb(type,name)(GtkWidget *w, UNUSED gpointer d) \
NEWLINE { \
   NEWLINE    if(!human_interaction) return; \
   NEWLINE    GdkRGBA color; \
   NEWLINE    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(w),&color); \
   NEWLINE    free(Flags.name); \
   NEWLINE    rgba2color(&color,&Flags.name); \
NEWLINE }

ALL_BUTTONS
#undef togglecode
#undef rangecode
#undef colorcode

// define set_buttons
//
#define togglecode(type,name,m)\
   NEWLINE if(m>0)gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Button.name),Flags.name);\
   NEWLINE else gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Button.name),!Flags.name);
#define rangecode(type,name,m) \
   NEWLINE gtk_range_set_value(GTK_RANGE(Button.name), m*((gdouble)Flags.name));
#define colorcode(type,name,m) \
   NEWLINE gdk_rgba_parse(&color,Flags.name); \
NEWLINE gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(Button.name),&color); 

static void set_buttons1()
{
   GdkRGBA color; 
   ALL_BUTTONS
}
#undef togglecode
#undef rangecode
#undef colorcode

// define signal_connect

#define togglecode(type,name,m) \
   NEWLINE g_signal_connect(G_OBJECT(Button.name),"toggled", G_CALLBACK(buttoncb(type,name)),NULL);
#define rangecode(type,name,m) \
   NEWLINE g_signal_connect(G_OBJECT(Button.name),"value-changed", G_CALLBACK(buttoncb(type,name)),NULL);
#define colorcode(type,name,m)  \
  NEWLINE g_signal_connect(G_OBJECT(Button.name),"color-set", G_CALLBACK(buttoncb(type,name)),NULL);

static void connect_signals()
{
   ALL_BUTTONS
}
#undef togglecode
#undef rangecode
#undef colorcode

static void report_tree_type(int p, gint active)
{
   P("Tree: %d %d %s\n",p,active,Flags.TreeType);
   int *a;
   int n;
   csvpos(Flags.TreeType,&a,&n);
   if(active)
   {
      a = (int *)realloc(a,sizeof(*a)*(n+1));
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
   int *b = (int *)malloc(sizeof(*b)*n);
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
   free(Flags.TreeType);
   vsc(&Flags.TreeType,b,m);
   free(a);
   free(b);
   P("Tree_Type set to %s\n",Flags.TreeType);
}

MODULE_EXPORT void button_tree(GtkWidget *w, UNUSED gpointer d)
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
   Flags.DesiredNumberOfTrees = DEFAULT_DesiredNumberOfTrees; 
   free(Flags.TreeType);
   Flags.TreeType                = strdup(DEFAULT_TreeType);
   Flags.NoTrees                 = DEFAULT_NoTrees;
   Flags.TreeFill                = DEFAULT_TreeFill;
   free(Flags.TreeColor);
   Flags.TreeColor                 = strdup(DEFAULT_TreeColor);
   if (vintage)
   {
      Flags.DesiredNumberOfTrees = VINTAGE_DesiredNumberOfTrees; 
      free(Flags.TreeType);
      Flags.TreeType             = strdup(VINTAGE_TreeType);
   }
   set_buttons();
   human_interaction = h;
}

   MODULE_EXPORT
void button_defaults_scenery(UNUSED GtkWidget *w, UNUSED gpointer d)
{
   P("button_defaults_scenery\n");
   scenery_default(0);
}

   MODULE_EXPORT
void button_vintage_scenery(UNUSED GtkWidget *w, UNUSED gpointer d)
{
   P("button_vintage_scenery\n");
   scenery_default(1);
}


static void init_tree_buttons()
{

#define TREE(x) \
   NEWLINE tree_buttons.tree_##x.button = GTK_WIDGET(gtk_builder_get_object(builder,PREFIX_TREE #x));
   TREE_ALL;
#undef TREE
#define TREE(x) \
   NEWLINE gtk_widget_set_name(tree_buttons.tree_##x.button,PREFIX_TREE #x);
   TREE_ALL;
#undef TREE
}

static void init_santa_pixmaps()
{
#define SANTA(x) NEWLINE santa_buttons.santa_ ## x.imid  = (char *)PREFIX_SANTA # x "-imid";
   SANTA_ALL;
#undef SANTA

   int i;
   GtkImage *image; 
   GdkPixbuf *pixbuf;
   for (i=0; i<NBUTTONS; i++)
   {
      pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **)Santas[i/2][i%2][0]);
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
   NEWLINE pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **)xpmtrees[x]);\
   NEWLINE image = GTK_IMAGE(gtk_builder_get_object(builder,"treeimage" # x));\
   NEWLINE gtk_image_set_from_pixbuf(image,pixbuf); \
   NEWLINE g_object_unref(pixbuf);

   TREE_ALL;
#undef TREE
}

static void init_hello_pixmaps()
{
   GtkImage *image; 
   GdkPixbuf *pixbuf, *pixbuf1;
   pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **)xsnow_logo);
   pixbuf1 = gdk_pixbuf_scale_simple(pixbuf,64,64,GDK_INTERP_BILINEAR);
   image = GTK_IMAGE(gtk_builder_get_object(builder,"hello-image1"));
   gtk_image_set_from_pixbuf(image,pixbuf1);
   image = GTK_IMAGE(gtk_builder_get_object(builder,"hello-image2"));
   gtk_image_set_from_pixbuf(image,pixbuf1);
   g_object_unref(pixbuf);
   g_object_unref(pixbuf1);
   pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **)xpmtrees[0]);
   image = GTK_IMAGE(gtk_builder_get_object(builder,"hello-image3"));
   gtk_image_set_from_pixbuf(image,pixbuf);
   image = GTK_IMAGE(gtk_builder_get_object(builder,"hello-image4"));
   gtk_image_set_from_pixbuf(image,pixbuf);
   g_object_unref(pixbuf);
}

static void init_pixmaps()
{
   init_santa_pixmaps();
   init_tree_pixmaps();
   init_hello_pixmaps();
}

static void set_tree_buttons()
{

#define TREE(x)\
   NEWLINE gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tree_buttons.tree_##x.button),FALSE);
   TREE_ALL;
#undef TREE
   int i;
   int *a,n;
   csvpos(Flags.TreeType,&a,&n);

   for (i=0; i<n; i++)
   {
      // P("set_tree_buttons::::::::::::::::::::: %s %d %d\n",Flags.TreeType,n,a[i]);
      switch (a[i])
      {
#define TREE(x) \
	 NEWLINE case x: gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tree_buttons.tree_##x.button),TRUE);\
		NEWLINE  break;
	 TREE_ALL;
#undef TREE
      }
   }
   free(a);
}

typedef struct _general_button
{
   GtkWidget *button;
}general_button;

MODULE_EXPORT void button_ww(GtkWidget *w, UNUSED gpointer d)
{
   if(!human_interaction) return;
   if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
   const gchar *s  = gtk_widget_get_name(w)+strlen(PREFIX_WW);
   int ww  = atoi(s);
   P("button_ww: ww: %d s:%s name:%s\n",ww,s,gtk_widget_get_name(w));
   Flags.WantWindow = ww;
}

static struct _general_buttons
{
   general_button ww_0;
   general_button ww_2;
} general_buttons;

MODULE_EXPORT void button_below(GtkWidget *w, UNUSED gpointer d)
{
   /*
    * In some desktop environments putting our transparent click-through window
    * above all other windows results in a un-clickable desktop.
    * Therefore, we ask for confirmation by clicking on a button.
    * If this succeeds, then there is no problem.
    * If the user cannot click, the timer runs out and the BelowAll
    * setting is switched to TRUE.
    */
   if(!human_interaction) return;
   gint active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
   P("button_below: %d\n",Flags.BelowAll);
   if(active)
      Flags.BelowAll = 1;
   else
   {
      Flags.BelowAll = 0;
      bct_countdown  = 9;
      show_bct_countdown();
      gtk_widget_hide(Button.BelowAll);
      gtk_widget_show(Button.BelowConfirm);
      bct_id = add_to_mainloop(PRIORITY_DEFAULT,1.0,below_confirm_ticker,NULL);
   }
}
MODULE_EXPORT void button_below_confirm(UNUSED GtkWidget *w, UNUSED gpointer d)
{
   gtk_widget_hide(Button.BelowConfirm);
   gtk_widget_show(Button.BelowAll);
   remove_from_mainloop(bct_id);
}

static void init_general_buttons()
{
   general_buttons.ww_0.button = GTK_WIDGET(gtk_builder_get_object(builder,"general-ww-0"));
   general_buttons.ww_2.button = GTK_WIDGET(gtk_builder_get_object(builder,"general-ww-2"));

   g_signal_connect(Button.BelowAll, "toggled", G_CALLBACK (button_below), NULL);
   g_signal_connect(Button.BelowConfirm, "toggled", G_CALLBACK(button_below_confirm), NULL);

   gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(builder,"general-version")),"xsnow version " VERSION);

   gtk_widget_set_name(general_buttons.ww_0.button,"ww-0"); 
   gtk_widget_set_name(general_buttons.ww_2.button,"ww-2"); 

   gtk_widget_hide(Button.BelowConfirm);
}

static void set_general_buttons()
{
   if (Flags.WantWindow == UW_DEFAULT)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(general_buttons.ww_0.button),TRUE);
   else
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(general_buttons.ww_2.button),TRUE);
}



void show_bct_countdown()
{
   sprintf(sbuffer,"Click to\nconfirm %d",bct_countdown);
   gtk_button_set_label(GTK_BUTTON(Button.BelowConfirm),sbuffer);

}

int below_confirm_ticker(UNUSED gpointer data)
{
   bct_countdown--;
   show_bct_countdown();
   if (bct_countdown>0)
      return TRUE;
   else
   {
      Flags.BelowAll = 1;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Button.BelowAll),Flags.BelowAll);
      gtk_widget_hide(Button.BelowConfirm);
      gtk_widget_show(Button.BelowAll);
      return FALSE;
   }
}



   MODULE_EXPORT
void button_quit(UNUSED GtkWidget *w, UNUSED gpointer d)
{
   Flags.Done = 1;
   P("button_quit: %d\n",Flags.Done);
}

void general_default(int vintage)
{
   int h = human_interaction;
   human_interaction      = 0;

   Flags.CpuLoad       = DEFAULT_CpuLoad;
   Flags.Transparency  = DEFAULT_Transparency;
   Flags.UseBG         = DEFAULT_UseBG;
   free(Flags.BGColor);
   Flags.BGColor       = strdup(DEFAULT_BGColor);
   Flags.Exposures     = DEFAULT_Exposures;
   Flags.OffsetS       = DEFAULT_OffsetS;
   Flags.OffsetY       = DEFAULT_OffsetY;
   Flags.FullScreen    = DEFAULT_FullScreen;
   Flags.BelowAll      = DEFAULT_BelowAll;
   Flags.AllWorkspaces = DEFAULT_AllWorkspaces;
   Flags.WantWindow    = DEFAULT_WantWindow;
   if (vintage)
   {
   }
   set_buttons();
   human_interaction      = h;
}

   MODULE_EXPORT 
void button_defaults_general(UNUSED GtkWidget *w, UNUSED gpointer d)
{
   P("button_defaults_general\n");
   general_default(0);
}

   MODULE_EXPORT 
void button_vintage_general(UNUSED GtkWidget *w, UNUSED gpointer d)
{
   P("button_defaults_general vintage\n");
   general_default(1);
}

typedef struct _snow_button
{
   GtkWidget *button;
}snow_button;


void snow_default(int vintage)
{
   int h = human_interaction;
   human_interaction = 0;
   Flags.BlowSnow          = DEFAULT_BlowSnow;
   Flags.SnowFlakesFactor  = DEFAULT_SnowFlakesFactor;
   Flags.SnowSize          = DEFAULT_SnowSize;
   Flags.NoSnowFlakes      = DEFAULT_NoSnowFlakes;
   free(Flags.SnowColor);
   Flags.SnowColor         = strdup(DEFAULT_SnowColor);
   Flags.SnowSpeedFactor   = DEFAULT_SnowSpeedFactor;
   Flags.FlakeCountMax     = DEFAULT_FlakeCountMax;
   Flags.BlowOffFactor     = DEFAULT_BlowOffFactor;
   Flags.MaxWinSnowDepth   = DEFAULT_MaxWinSnowDepth;
   Flags.MaxScrSnowDepth   = DEFAULT_MaxScrSnowDepth;
   Flags.MaxOnTrees        = DEFAULT_MaxOnTrees;
   Flags.NoKeepSWin        = DEFAULT_NoKeepSWin;
   Flags.NoKeepSBot        = DEFAULT_NoKeepSBot;
   Flags.NoKeepSnowOnTrees = DEFAULT_NoKeepSnowOnTrees;
   Flags.NoFluffy          = DEFAULT_NoFluffy;

   UseVintageFlakes        = 0;

   if(vintage)
   {
      Flags.BlowSnow          = VINTAGE_BlowSnow;
      Flags.SnowFlakesFactor  = VINTAGE_SnowFlakesFactor;
      Flags.NoKeepSnowOnTrees = VINTAGE_NoKeepSnowOnTrees;

      UseVintageFlakes        = 1;

   }
   set_buttons();
   human_interaction = h;
}

   MODULE_EXPORT
void button_defaults_snow(UNUSED GtkWidget *w, UNUSED gpointer d)
{
   P("button_defaults_snow\n");
   snow_default(0);
}

   MODULE_EXPORT
void button_vintage_snow(UNUSED GtkWidget *w, UNUSED gpointer d)
{
   P("button_vintage_snow\n");
   snow_default(1);
}

void ui_set_birds_header(const char *text)
{
   GtkWidget *birds_header = GTK_WIDGET(gtk_builder_get_object(builder,"birds-header")); 
   gtk_label_set_text(GTK_LABEL(birds_header),text);
}

void ui_set_celestials_header(const char *text)
{
   GtkWidget *celestials_header = GTK_WIDGET(gtk_builder_get_object(builder,"celestials-header")); 
   char *a = strdup(gtk_label_get_text(GTK_LABEL(celestials_header)));
   a = (char *) realloc(a,strlen(a)+2+strlen(text));
   strcat(a,"\n");
   strcat(a,text);
   gtk_label_set_text(GTK_LABEL(celestials_header),a);
   free(a);
}


void birds_default(int vintage)
{
   int h = human_interaction;
   human_interaction = 0;
   if(vintage)
   {
      Flags.ShowBirds   = VINTAGE_ShowBirds;
      Flags.BirdsOnly   = VINTAGE_BirdsOnly;
   }
   else
   {
      Flags.ShowBirds        = DEFAULT_ShowBirds;
      Flags.BirdsOnly        = DEFAULT_BirdsOnly;
      Flags.Nbirds           = DEFAULT_Nbirds;
      Flags.Neighbours       = DEFAULT_Neighbours;
      Flags.Anarchy          = DEFAULT_Anarchy;
      Flags.PrefDistance     = DEFAULT_PrefDistance;
      Flags.ViewingDistance  = DEFAULT_ViewingDistance;
      Flags.BirdsSpeed       = DEFAULT_BirdsSpeed;
      Flags.AttrFactor       = DEFAULT_AttrFactor;
      Flags.DisWeight        = DEFAULT_DisWeight;
      Flags.FollowWeight     = DEFAULT_FollowWeight;
      Flags.ShowAttrPoint    = DEFAULT_ShowAttrPoint;
      Flags.FollowSanta      = DEFAULT_FollowSanta;
      Flags.BirdsScale       = DEFAULT_BirdsScale;
      free(Flags.BirdsColor);
      Flags.BirdsColor       = strdup(DEFAULT_BirdsColor);
   }
   set_buttons();
   human_interaction = h;
}

MODULE_EXPORT void button_defaults_birds(UNUSED GtkWidget *w, UNUSED gpointer d)
{
   P("button_defaults_birds\n");
   birds_default(0);
}

MODULE_EXPORT void button_vintage_birds(UNUSED GtkWidget *w, UNUSED gpointer d)
{
   P("button_vintage_birds\n");
   birds_default(1);
}

MODULE_EXPORT void button_birds_restart(UNUSED GtkWidget *w, UNUSED gpointer p)
{
   P("button_birds_restart\n");
   Flags.BirdsRestart = 1;
}

MODULE_EXPORT void button_wind_activate(UNUSED GtkWidget *w, UNUSED gpointer p)
{
   P("button_wind_activate\n");
   Flags.WindNow = 1;
}

void celestials_default(int vintage)
{
   int h = human_interaction;
   human_interaction = 0;
   Flags.NoWind        = DEFAULT_NoWind;
   Flags.WhirlFactor   = DEFAULT_WhirlFactor;
   Flags.WindTimer     = DEFAULT_WindTimer;
   Flags.NStars        = DEFAULT_NStars;
   Flags.Stars         = DEFAULT_Stars;
   Flags.NoMeteorites  = DEFAULT_NoMeteorites;
   Flags.Moon          = DEFAULT_Moon;
   Flags.MoonSpeed     = DEFAULT_MoonSpeed;
   Flags.MoonSize      = DEFAULT_MoonSize;
   Flags.Halo          = DEFAULT_Halo;
   Flags.HaloBright    = DEFAULT_HaloBright;
   if(vintage)
   {
      Flags.Stars          = VINTAGE_Stars;
      Flags.Stars          = VINTAGE_Stars;
      Flags.NoMeteorites   = VINTAGE_NoMeteorites;
      Flags.Moon           = VINTAGE_Moon;
   }
   set_buttons();
   human_interaction = h;
}

   MODULE_EXPORT
void button_defaults_celestials(UNUSED GtkWidget *w, UNUSED gpointer d)
{
   P("button_defaults_wind\n");
   celestials_default(0);
}

   MODULE_EXPORT
void button_vintage_celestials(UNUSED GtkWidget *w, UNUSED gpointer d)
{
   P("button_vintage_wind\n");
   celestials_default(1);
}

static void init_buttons()
{
   init_buttons1();
   init_santa_buttons();
   init_tree_buttons();
   init_general_buttons();
   nflakeslabel = GTK_WIDGET(gtk_builder_get_object(builder,"nflakes"));
}

static void set_buttons()
{
   human_interaction = 0;
   set_buttons1();
   set_santa_buttons();
   set_tree_buttons();
   set_general_buttons();
   human_interaction = 1;
}

void all_default(int vintage)
{
   santa_default(vintage);
   scenery_default(vintage);
   snow_default(vintage);
   celestials_default(vintage);
   birds_default(vintage);
}

MODULE_EXPORT void button_all_defaults()
{
   P("button_all_defaults\n");
   all_default(0);
}

MODULE_EXPORT void button_all_vintage()
{
   P("button_all_vintage\n");
   all_default(1);
}

void ui_show_nflakes(int n)
{
   snprintf(sbuffer,nsbuffer,"%6d",n);
   gtk_label_set_text(GTK_LABEL(nflakeslabel),sbuffer);
}

void ui_show_range_etc()
{
   snprintf(sbuffer,nsbuffer,"Range: %d\n",(int)birds_get_range());
   gtk_label_set_text(GTK_LABEL(range),sbuffer);
   snprintf(sbuffer,nsbuffer,"Mean dist: %d\n",(int)birds_get_mean_dist());
   gtk_label_set_text(GTK_LABEL(mean_distance),sbuffer);
}

void ui_show_desktop_type(const char *s)
{
   snprintf(sbuffer,nsbuffer,"Desktop type: %s",s);
   gtk_label_set_text(GTK_LABEL(desktop_type),sbuffer);
}

void ui_set_sticky(int x)
{
   if (x)
      gtk_window_stick(GTK_WINDOW(hauptfenster));
   else
      gtk_window_unstick(GTK_WINDOW(hauptfenster));
}

void ui(UNUSED int *argc, UNUSED char **argv[])
{

   builder = gtk_builder_new_from_string (xsnow_xml, -1);
   gtk_builder_connect_signals (builder, builder);
   hauptfenster  = GTK_WIDGET   (gtk_builder_get_object(builder, "hauptfenster"));
   mean_distance = GTK_WIDGET   (gtk_builder_get_object(builder, "birds-mean-distance"));
   range         = GTK_WIDGET   (gtk_builder_get_object(builder, "birds-range"));
   desktop_type  = GTK_WIDGET   (gtk_builder_get_object(builder, "settings-show-desktop-type"));
   birdsgrid     = GTK_CONTAINER(gtk_builder_get_object(builder, "grid_birds"));
   moonbox       = GTK_CONTAINER(gtk_builder_get_object(builder, "moon-box"));

   apply_standard_css();
   gtk_window_set_title(GTK_WINDOW(hauptfenster),"XsnoW");
   gtk_widget_show_all (hauptfenster);

   init_buttons();
   connect_signals();
   init_pixmaps();
   set_buttons();
}

void apply_standard_css()
{
   const char *css     = 
      "scale                               { padding:          1em;     }"   // padding in slider buttons
      "button.radio                        { min-width:        10px;    }"   // make window as small as possible
      "button                              { background:       #CCF0D8; }"   // color of normal buttons
      "button.radio,        button.toggle  { background:       #E2FDEC; }"   // color of radio and toggle buttons
      "radiobutton:active,  button:active  { background:       #0DAB44; }"   // color of buttons while being activated
      "radiobutton:checked, button:checked { background:       #6AF69B; }"   // color of checked buttons
      "headerbar                           { background:       #B3F4CA; }"   // color of headerbar
      "scale slider                        { background:       #D4EDDD; }"   // color of sliders
      "scale trough                        { background:       #0DAB44; }"   // color of trough of sliders
      "stack                               { background-color: #EAFBF0; }"   // color of main area
      "*                                   { color:            #065522; }"   // foreground color (text)
      "*:disabled *                        { color:            #8FB39B; }"   // foreground color for disabled items
      ".pink    { background-color: #FFC0CB; border-radius: 4px; min-height: 3.5em }"
      "button.confirm { background-color: #FFFF00; }"
      ;

   static GtkCssProvider *cssProvider = NULL;
   if (!cssProvider)
   {
      cssProvider  = gtk_css_provider_new();
      gtk_css_provider_load_from_data (cssProvider, css,-1,NULL);
   }

   apply_css_provider(hauptfenster, cssProvider);

}

// if m==1: change some colors of the ui
// if m==0: change back to default colors

void ui_background(int m)
{
   const char *colorbg =   // load alert colors
      "stack                { background-color: #FFC0CB; }"   // color of main area
      "scale.cpuload slider { background:       #FF0000; }"   // color of sliders with class cpuload
      ;
   static GtkCssProvider *cssProvidercolor = NULL;
   if (!cssProvidercolor)
   {
      cssProvidercolor  = gtk_css_provider_new();
      gtk_css_provider_load_from_data (cssProvidercolor, colorbg,-1,NULL);
   }

   apply_standard_css();
   if(m)
      apply_css_provider(hauptfenster,cssProvidercolor);
}

// m=0: make active
// m=1: make inactive
void ui_gray_ww(int m)
{
   gtk_widget_set_sensitive(general_buttons.ww_0.button,!m);
   gtk_widget_set_sensitive(general_buttons.ww_2.button,!m);
}

// m=0: make active
// m=1: make inactive
// however, see transparency and below 
void ui_gray_erase(int m)
{
   gtk_widget_set_sensitive(Button.Exposures,                    !m);
   gtk_widget_set_sensitive(Button.UseBG,                        !m);
   gtk_widget_set_sensitive(Button.BGColor,                      !m);
   gtk_widget_set_sensitive(Button.Transparency,                  m);
   gtk_widget_set_sensitive(Button.BelowAll,                      m);
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Button.BelowAll),1);
}


// m=0: make active
// m=1: make inactive
void ui_gray_below(int m)
{
   gtk_widget_set_sensitive(Button.BelowAll, !m);
}

void birdscb(GtkWidget *w, void *m)
{
   gtk_widget_set_sensitive(w,!(int *)m);
}

void ui_gray_birds(int m)
{
   gtk_container_foreach(birdsgrid, birdscb, &m);
   gtk_container_foreach(moonbox, birdscb, &m);
}

char * ui_gtk_version()
{
   static char s[20];
   snprintf(s,20,"%d.%d.%d",gtk_get_major_version(),gtk_get_minor_version(),gtk_get_micro_version());
   return s;
}

char * ui_gtk_required()
{
   static char s[20];
   snprintf(s,20,"%d.%d.%d",GTK_MAJOR,GTK_MINOR,GTK_MICRO);
   return s;
}

// returns:
// 0: gtk version in use too low
// 1: gtk version in use OK
int ui_checkgtk()
{
   if ((int)gtk_get_major_version() > GTK_MAJOR)
      return 1;
   if ((int)gtk_get_major_version() < GTK_MAJOR) 
      return 0;
   if ((int)gtk_get_minor_version() > GTK_MINOR)
      return 1;
   if ((int)gtk_get_minor_version() < GTK_MINOR)
      return 0;
   if ((int)gtk_get_micro_version() >= GTK_MICRO)
      return 1;
   return 0;
}

// to be used if gtk version is too low
// returns 1: user clicked 'Run with ...'
// returns 0: user clicked 'Quit'
static int RC;
int ui_run_nomenu()
{
   GtkApplication *app;
   app = gtk_application_new ("nl.ratrabbit.example", G_APPLICATION_FLAGS_NONE);
   g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
   g_application_run (G_APPLICATION (app), 0, NULL);
   g_object_unref (app);
   return RC;
}

static void activate (GtkApplication *app, UNUSED gpointer user_data)
{
   GtkWidget *window;
   GtkWidget *grid;
   GtkWidget *button;
   GtkWidget *label;


   /* create a new window, and set its title */
   window = gtk_application_window_new (app);
   gtk_window_set_position        (GTK_WINDOW(window), GTK_WIN_POS_CENTER);
   gtk_window_set_title           (GTK_WINDOW (window), "Xsnow");
   gtk_window_set_decorated       (GTK_WINDOW(window), FALSE);
   gtk_window_set_keep_above      (GTK_WINDOW(window), TRUE);
   gtk_container_set_border_width (GTK_CONTAINER (window), 10);

   /* Here we construct the container that is going pack our buttons */
   grid = gtk_grid_new ();

   /* Pack the container in the window */
   gtk_container_add (GTK_CONTAINER (window), grid);

   snprintf(sbuffer,nsbuffer,
	 "You are using GTK-%s, but you need at least GTK-%s to view\n"
	 "the user interface.\n"
	 "Use the option '-nomenu' to disable the user interface.\n"
	 "If you want to try the user interface anyway, use the flag '-checkgtk 0'.\n\n"
	 "See 'man xsnow' or 'xsnow -h' to see the command line options.\n"
	 "Alternatively, you could edit ~/.xsnowrc to set options.\n",
	 ui_gtk_version(),ui_gtk_required());
   label = gtk_label_new(sbuffer);

   /* Place the label in cell (0,0) and make it fill 2 cells horizontally */

   gtk_grid_attach(GTK_GRID(grid),label,0,0,2,1);
   button = gtk_button_new_with_label ("Run without user interface");
   g_signal_connect(button,"clicked",G_CALLBACK(yesyes),window);

   /* Place the first button in the grid cell (0, 1), and make it fill
    * just 1 cell horizontally and vertically (ie no spanning)
    */
   gtk_grid_attach (GTK_GRID (grid), button, 0, 1, 1, 1);

   button = gtk_button_new_with_label ("Quit");
   g_signal_connect(button, "clicked", G_CALLBACK (nono), window);

   /* Place the second button in the grid cell (1, 1), and make it fill
    * just 1 cell horizontally and vertically (ie no spanning)
    */
   gtk_grid_attach (GTK_GRID (grid), button, 1, 1, 1, 1);

   /* Now that we are done packing our widgets, we show them all
    * in one go, by calling gtk_widget_show_all() on the window.
    * This call recursively calls gtk_widget_show() on all widgets
    * that are contained in the window, directly or indirectly.
    */
   gtk_widget_show_all (window);
}

void yesyes(UNUSED GtkWidget *w, gpointer window)
{
   RC = 1;
   gtk_widget_destroy(GTK_WIDGET(window));
}

void nono(UNUSED GtkWidget *w, gpointer window)
{
   RC = 0;
   gtk_widget_destroy(GTK_WIDGET(window));
}

// next function is not used, I leave it here as a template, who knows...
// see also ui.xml
void ui_error_x11(UNUSED int *argc, UNUSED char **argv[])
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
   pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **)xsnow_logo);
   image = GTK_IMAGE(gtk_builder_get_object(builder,"error-x11-image1"));
   gtk_image_set_from_pixbuf(image,pixbuf);
   image = GTK_IMAGE(gtk_builder_get_object(builder,"error-x11-image2"));
   gtk_image_set_from_pixbuf(image,pixbuf);
   g_object_unref(pixbuf);

   gtk_widget_show_all (errorfenster);
   gtk_main();
}
