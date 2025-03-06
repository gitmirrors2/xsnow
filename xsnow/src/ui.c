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

/* How to implement a new button
 *
 * The generation of code to add a button and/or a flag is dependent
 * on definitions in 'doit.h' and 'buttons.h'.
 *
 * doit.h
 *   definition of names of flags, together with default values and vintage values
 *   example:
 *     DOIT_I(HaloBright           ,25         ,25         )
 *
 *   DOIT_I: for flags with an integer value
 *   DOIT_L: for flags with a large value (for example a window-id)
 *   DOIT_S: for flags with a char* value (colors, mostly)
 *
 *   Macro DOIT will call macro's that are not meant for read/write from .xsnowrc
 *   Macro DOIT_ALL calls all DOIT_* macro's
 *   This will result in:
 *     see flags.h:
 *       creation of member HaloBright in type FLAGS  (see flags.h)
 *     see flags.c:
 *       definition of default value in DefaultFlags.HaloBright  (25)
 *       definition of vintage value in VintageFlags.Halobright  (0)
 *       definition of WriteFlags() to write the flags to .xsnowrc
 *       definition of ReadFlags() to read flags from .xsnowrc
 *
 *
 * buttons.h
 *   definition of button-related entities.
 *   example:
 *     BUTTON(scalecode      ,xsnow_celestials  ,HaloBright           ,1  )
 *     this takes care that flag 'HaloBright' is associated with a button
 *     in the 'celestials' tab with the glade-id 'id-HaloBright' and that a value
 *     of 1 is used in the expansion of scalecode.
 *     In this case, the button should be a GtkScale button.
 *
 *   The macro ALL_BUTTONS takes care that scalecode is called as
 *     scalecode(xsnow_celestials,HaloBright,1)
 *   and that all other BUTTON macro's are called
 *
 *   The following types of buttons are implemented:
 *     GtkScale (macro scalecode)
 *     GtkToggle(macro togglecode)
 *     GtkColor (macro colorcode)
 *
 *   In this way, the following items are generated:
 *
 *     ui.c:
 *       define type Buttons, containing all flags in buttons.h
 *       associate the elements of Buttons with the corresponding 
 *         glade-id's
 *       define call-backs
 *         these call backs have names like 'button_xsnow_celestials_HaloBright'
 *         the code ensures that for example Flags.HaloBright gets the value
 *         of the corresponding button.
 *       create a function set_buttons1(), that sets all buttons in the state
 *         defined by the corresponding Flags. For example, if 
 *         Flags.HaloBright = 40, the corresponding GtkScale button will be set
 *         to this value.
 *       connects signals of buttons to the corresponding call-backs, for example,
 *         button with glade-id 'id-HaloBright', when changed, will result in
 *         a call of button_xsnow_celestials_HaloBright().
 *       create function set_default_tab(int tab, int vintage) that gives the
 *         buttons in the given tab (for example 'xsnow_celestials') and the
 *         corresponding flags their default (vintage = 0) or vintage (vintage=1) 
 *         value. One will notice, that some buttons need extra care, for example
 *         flag TreeType in xsnow_scenery.
 *
 *   glade, ui.glade
 *
 *     Glade is used to maintain 'ui.glade', where the creation of the tabs and the
 *     placement of the buttons is arranged.
*     For the buttons in 'buttons.h' a callback is arranged in 'ui.c', so in general
*     there is no need to do something with the 'signals' properties of these buttons.
*     Things that are needed:
*       - the id, for example: id-HaloBright
*       - button text, maybe using a GtkLabel
*       - tooltip
*       - for scale buttons: a GtkScale, defining for example min and max values
*       - placement
*       - for few buttons: a css class. Example: BelowConfirm
*     In Makefile.am, ui.glade is converted to an include file: ui_xml.h
*     So, when compiled, the program does not need an external file for it's GtkBuilder.
*
*
*   Handling of changed flags.
*
*     In 'flags.h' the macros UIDO and UIDOS are defined. They take care of the
*     standard action to be used when a flag has been changed: simply copy
*     the new value to OldFlags and increment Flags.Changes. OldFlags is initialized 
*     at the start of the program, and is used to check if a flag has been changed.
*
*     UIDO (for integer valued flags) and UIDOS (for char* valued flags) take
*     two parameters:
*     - the name of the flag to check
*     - C-code to execute if the value of the flag has been changed.
*
*     In main.c the flags in the 'settings' tab are handled, and calls are
*     made to for example scenery_ui() which is supposed to handle flags related
*     with the 'scenery' tab.
*     If Flags.Changes > 0, the flags are written to .xsnowrc.
*
*   Documentation of flags
*
*     This is take care of in 'docs.c'.
*      
*/

#include <pthread.h>
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
#include <X11/extensions/Xinerama.h>


#include "xsnow-constants.h"

#include "Santa.h"
#include "birds.h"
#include "clocks.h"
#include "csvpos.h"
#include "flags.h"
#include "mygettext.h"
#include "pixmaps.h"
#include "snow.h"
#include "ui.h"
#include "ui_xml.h"
#include "utils.h"
#include "version.h"
#include "windows.h"
#include "xsnow.h"
#include "safe_malloc.h"
#include "xpm2cairo-gdk.h"

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

#define SANTA2(x) SANTA(x) SANTA(x ## r)
#define SANTA_ALL SANTA2(0) SANTA2(1) SANTA2(2) SANTA2(3) SANTA2(4)

#define TREE_ALL TREE(0) TREE(1) TREE(2) TREE(3) TREE(4) TREE(5) TREE(6) TREE(7)

#define DEFAULT(name) DefaultFlags.name
#define VINTAGE(name) VintageFlags.name

GtkBuilder    *builder;
static GtkWidget     *mean_distance;
static GtkWidget     *range;
static GtkWidget     *desktop_type;
static GtkContainer  *birdsgrid;
static GtkContainer  *moonbox;
static GtkImage      *preview;
static int Nscreens;
static int HaveXinerama;
static int iconified = 0;

#define nsbuffer 1024
static char sbuffer[nsbuffer];

static void set_buttons1(void);
static void set_santa_buttons(void);
static void set_tree_buttons(void);
static void handle_css(void);
static void birdscb(GtkWidget *w, void *m);
static int  below_confirm_ticker(void *);
static int  ui_running = False;
static void show_bct_countdown(void);
static void yesyes(GtkWidget *w, gpointer data);
static void nono(GtkWidget *w, gpointer data);
static void activate (GtkApplication *app);
static void set_default_tab(int tab, int vintage);
static void set_belowall_default();
static void handle_theme(void);
static void handle_screen(void);
static void update_preview_cb (GtkFileChooser *file_chooser, gpointer data);
static void my_gtk_label_set_text(GtkLabel *label, const gchar *str); 
static int do_test_iconified(void *dummy);
static gboolean callback_func(GtkWidget *widget,
      GdkEventWindowState *event,
      gpointer user_data);
static void headerfunc(GtkWidget *w, gpointer data);


static int human_interaction = 1;
GtkWidget *nflakeslabel;

static guint bct_id = 0;
static int bct_countdown;

static GtkWidget       *hauptfenster;
static GtkStyleContext *hauptfenstersc;

static char* lang[100];
static int   nlang;

void ui_ui()
{
   UIDO (ThemeXsnow, handle_theme(););
   UIDO (Screen, handle_screen(););
   UIDO (Outline, ClearScreen(););
   UIDOS (Language, handle_language(1););
}

void handle_theme()
{
   if(!ui_running)
      return;
   if (Flags.ThemeXsnow)
   {
      gtk_style_context_add_class(hauptfenstersc,"xsnow");
   }
   else
   {
      gtk_style_context_remove_class(hauptfenstersc,"xsnow");
   }
}

void handle_screen()
{
   P("handle_screen:%d\n",Flags.Screen);
   if (HaveXinerama && Nscreens > 1)
      global.ForceRestart = 1;
}

void handle_language(int restart)
{
   P("handle_language: %s\n",Flags.Language);
   if (!strcmp(Flags.Language,"sys"))
      unsetenv("LANGUAGE");
   else
      setenv("LANGUAGE",Flags.Language,1);
   if(restart)
      global.ForceRestart = 1;
}

   MODULE_EXPORT
void button_iconify()
{
   P("button_iconify\n");
   gtk_window_iconify(GTK_WINDOW(hauptfenster));
   g_timeout_add_full(G_PRIORITY_DEFAULT,40, do_test_iconified, NULL, NULL);
}

int do_test_iconified(void *dummy)
{
   (void)dummy;
   static int counter = 0;
   P("do_test_iconified %d\n",counter);
   if(iconified)
   {
      P("is iconified\n");
      return FALSE;
   }
   counter++;
   if (!iconified && counter > 10)
   {
      P("hiding hauptfenster %d\n",counter);
      gtk_widget_hide(GTK_WIDGET(hauptfenster));
      iconified = 1;
      counter = 0;
      return FALSE;
   }
   return TRUE;
}

gboolean callback_func(GtkWidget *widget,
      GdkEventWindowState *event,
      gpointer user_data)
{
   (void)widget;
   (void)user_data;
   if(event->new_window_state & GDK_WINDOW_STATE_ICONIFIED)
   {
      iconified = 1;
      P("iconified\n");
   }
   return TRUE;
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
#include "undefall.inc"


#define SANTA(x) NEWLINE &santa_buttons.santa_ ## x,
static santa_button *santa_barray[NBUTTONS]=
{
   SANTA_ALL
};
#include "undefall.inc"

static void init_santa_buttons()
{
#define SANTA(x) \
   NEWLINE santa_buttons.santa_ ## x.button = GTK_WIDGET(gtk_builder_get_object(builder,PREFIX_SANTA #x)); 
   SANTA_ALL;
#include "undefall.inc"
#define SANTA(x) \
   NEWLINE gtk_widget_set_name(santa_buttons.santa_ ## x.button,PREFIX_SANTA #x);
   SANTA_ALL;
#include "undefall.inc"

}

static void set_santa_buttons()
{
   int n = 2*Flags.SantaSize;
   if (Flags.Rudolf)
      n++;
   if (n<NBUTTONS)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(santa_barray[n]->button),TRUE);
}

   MODULE_EXPORT 
void button_santa(GtkWidget *w)
{
   if(!human_interaction) return;
   if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
   const gchar *s  = gtk_widget_get_name(w)+strlen(PREFIX_SANTA);
   int santa_type  = atoi(s);
   int have_rudolf = ('r' == s[strlen(s)-1]);
   P("button_santa: Santa %d Rudolf %d s: %s name: %s\n",santa_type,have_rudolf,s,gtk_widget_get_name(w));
   Flags.SantaSize = santa_type;
   Flags.Rudolf  = have_rudolf;
   SantaVisible();
}

   MODULE_EXPORT 
void button_defaults_santa()
{
   P("button_defaults_santa defaults\n");
   set_default_tab(xsnow_santa,0);
}

   MODULE_EXPORT 
void button_vintage_santa()
{
   P("button_defaults_santa vintage\n");
   set_default_tab(xsnow_santa,1);
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
#include "undefall.inc"


// creating type Buttons: Button.NStars etc.

#define togglecode(type,name,m) NEWLINE GtkWidget *name;
#define scalecode togglecode
#define colorcode togglecode
#define filecode  togglecode
static struct _Button 
{
   ALL_BUTTONS
} Button;
#include "undefall.inc"

// create init_buttons: connect with glade-id
// glade-id will be: "id-name", eg: "id-SnowFlakesFactor"
#define ID "id"

#define togglecode(type,name,m) \
   NEWLINE P("%s %s\n",#name,#type); \
   NEWLINE Button.name = GTK_WIDGET(gtk_builder_get_object(builder,ID "-" #name));
#define scalecode togglecode
#define colorcode togglecode
#define filecode  togglecode 

static void init_buttons1()
{
   P("\nstart init_buttons1\n\n");
   ALL_BUTTONS
      P("\nend init_buttons1\n\n");
}

#include "undefall.inc"

// define call backs

#define buttoncb(type,name) button_##type##_##name
#define togglecode(type,name,m) \
   NEWLINE MODULE_EXPORT void buttoncb(type,name)(GtkWidget *w) \
   NEWLINE   { \
      NEWLINE    if(!human_interaction) return; \
      NEWLINE    gint active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)); \
      NEWLINE    if(active)  Flags.name = TRUE;  else  Flags.name = FALSE; \
      NEWLINE    if(m<0) Flags.name = !Flags.name;  \
      NEWLINE   }

#define scalecode(type,name,m) \
   NEWLINE MODULE_EXPORT void buttoncb(type,name)(GtkWidget *w)\
   NEWLINE {\
      NEWLINE    if(!human_interaction) return; \
      NEWLINE    gdouble value; \
      NEWLINE    value = gtk_range_get_value(GTK_RANGE(w)); \
      NEWLINE    Flags.name = m*lrint(value); \
      NEWLINE }

#define colorcode(type,name,m) \
   NEWLINE MODULE_EXPORT void buttoncb(type,name)(GtkWidget *w) \
   NEWLINE { \
      NEWLINE    if(!human_interaction) return; \
      NEWLINE    GdkRGBA color; \
      NEWLINE    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(w),&color); \
      NEWLINE    free(Flags.name); \
      NEWLINE    rgba2color(&color,&Flags.name); \
      NEWLINE }

#define filecode(type,name,m) \
   NEWLINE MODULE_EXPORT void buttoncb(type,name)(GtkWidget *w) \
   NEWLINE { \
      NEWLINE    if(!human_interaction) return; \
      NEWLINE    gchar *filename; \
      NEWLINE    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(w)); \
      NEWLINE    free(Flags.name); \
      NEWLINE    Flags.name = strdup(filename); \
      NEWLINE    g_free(filename); \
      NEWLINE }

ALL_BUTTONS
#include "undefall.inc"

// define set_buttons
//
#define togglecode(type,name,m)\
   NEWLINE P("toggle %s %s %d %d\n",#name,#type,m,Flags.name); \
NEWLINE     if (m) { \
   NEWLINE     if(m>0)  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Button.name),Flags.name);\
   NEWLINE     else     gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Button.name),!Flags.name);\
   NEWLINE   }
#define scalecode(type,name,m) \
   NEWLINE P("range %s %s %d %d\n",#name,#type,m,Flags.name); \
NEWLINE     gtk_range_set_value(GTK_RANGE(Button.name), m*((gdouble)Flags.name));
//NEWLINE     if(gtk_orientable_get_orientation(GTK_ORIENTABLE(Button.name)) == GTK_ORIENTATION_HORIZONTAL) 
//NEWLINE         gtk_widget_set_size_request(GTK_WIDGET(Button.name),100,-1);
#define colorcode(type,name,m) \
   NEWLINE P("color %s %s %d %s\n",#name,#type,m,Flags.name); \
NEWLINE     gdk_rgba_parse(&color,Flags.name); \
NEWLINE        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(Button.name),&color); 
#define filecode(type,name,m) \
   NEWLINE P("file %s %s %d %s\n",#name,#type,m,Flags.name); \
NEWLINE  gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(Button.name),Flags.BackgroundFile);

static void set_buttons1()
{
   GdkRGBA color; 
   ALL_BUTTONS
}
#include "undefall.inc"

// define signal_connect

#define togglecode(type,name,m) \
   NEWLINE P("%s %s\n",#name,#type); \
   NEWLINE g_signal_connect(G_OBJECT(Button.name),"toggled", G_CALLBACK(buttoncb(type,name)),NULL);
#define scalecode(type,name,m) \
   NEWLINE P("%s %s\n",#name,#type); \
   NEWLINE g_signal_connect(G_OBJECT(Button.name),"value-changed", G_CALLBACK(buttoncb(type,name)),NULL);
#define colorcode(type,name,m)  \
   NEWLINE P("%s %s\n",#name,#type); \
   NEWLINE g_signal_connect(G_OBJECT(Button.name),"color-set", G_CALLBACK(buttoncb(type,name)),NULL);
#define filecode(type,name,m) \
   NEWLINE P("%s %s\n",#name,#type); \
   NEWLINE g_signal_connect(G_OBJECT(Button.name),"file-set", G_CALLBACK(buttoncb(type,name)),NULL);

static void connect_signals()
{
   P("\nstart connect_signals\n\n");
   ALL_BUTTONS
      P("\nend connect_signals\n\n");
}
#include "undefall.inc"


static void report_tree_type(int p, gint active)
{
   P("Tree: %d %d %s\n",p,active,Flags.TreeType);
   int *a;
   int n;
   csvpos(Flags.TreeType,&a,&n);
   if(active)
   {
      a = (int *)realloc(a,sizeof(*a)*(n+1));
      REALLOC_CHECK(a);
      a[n] = p;
      n++;
   }
   else
   {
      for (int i=0; i<n; i++)
	 if(a[i] == p)
	    a[i] = -1;
   }
   int *b = (int *)malloc(sizeof(*b)*n);
   assert(b);
   int m=0;
   for(int i=0; i<n; i++)
   {
      int unique = (a[i] >= 0);
      if(unique)
	 for (int j=0; j<m; j++)
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

MODULE_EXPORT void button_tree(GtkWidget *w)
{
   if(!human_interaction) return;
   gint active;
   active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
   const gchar *s  = gtk_widget_get_name(w)+strlen(PREFIX_TREE);
   int p = atoi(s);
   report_tree_type(p,active);
   P("button_tree: tree: %d active: %d\n",p,active);
}


   MODULE_EXPORT
void button_defaults_scenery()
{
   P("button_defaults_scenery\n");
   set_default_tab(xsnow_scenery,0);
}

   MODULE_EXPORT
void button_vintage_scenery()
{
   P("button_vintage_scenery\n");
   set_default_tab(xsnow_scenery,1);
}


static void init_tree_buttons()
{

#define TREE(x) \
   NEWLINE tree_buttons.tree_##x.button = GTK_WIDGET(gtk_builder_get_object(builder,PREFIX_TREE #x));
   TREE_ALL;
#include "undefall.inc"
#define TREE(x) \
   NEWLINE gtk_widget_set_name(tree_buttons.tree_##x.button,PREFIX_TREE #x);
   TREE_ALL;
#include "undefall.inc"
}

static void init_santa_pixmaps()
{
#define SANTA(x) NEWLINE santa_buttons.santa_ ## x.imid  = (char *)PREFIX_SANTA # x "-imid";
   SANTA_ALL;
#include "undefall.inc"

   GtkImage *image; 
   GdkPixbuf *pixbuf;
   for (int i=0; i<NBUTTONS; i++)
   {
      pixbuf = xpm2gdk(global.display, (char **)Santas[i/2][i%2][0],NULL);
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
   NEWLINE pixbuf = xpm2gdk(global.display, (char **)xpmtrees[x], NULL);\
   NEWLINE image = GTK_IMAGE(gtk_builder_get_object(builder,"treeimage" # x));\
   NEWLINE gtk_image_set_from_pixbuf(image,pixbuf); \
   NEWLINE g_object_unref(pixbuf);

   TREE_ALL;
#include "undefall.inc"
}

static void init_hello_pixmaps()
{
   GtkImage *image; 
   GdkPixbuf *pixbuf, *pixbuf1;
   pixbuf = xpm2gdk(global.display, (char **)xsnow_logo, NULL);
   pixbuf1 = gdk_pixbuf_scale_simple(pixbuf,64,64,GDK_INTERP_BILINEAR);
   image = GTK_IMAGE(gtk_builder_get_object(builder,"hello-image1"));
   gtk_image_set_from_pixbuf(image,pixbuf1);
   image = GTK_IMAGE(gtk_builder_get_object(builder,"hello-image2"));
   gtk_image_set_from_pixbuf(image,pixbuf1);
   g_object_unref(pixbuf);
   g_object_unref(pixbuf1);
   pixbuf = xpm2gdk(global.display, (char **)xpmtrees[0], NULL);
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
#include "undefall.inc"
   int *a,n;
   csvpos(Flags.TreeType,&a,&n);

   for (int i=0; i<n; i++)
   {
      P("set_tree_buttons::::::::::::::::::::: %s %d %d\n",Flags.TreeType,n,a[i]);
      switch (a[i])
      {
#define TREE(x) \
	 NEWLINE case x: gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tree_buttons.tree_##x.button),TRUE);\
	 NEWLINE  break;
	 TREE_ALL;
#include "undefall.inc"
      }
   }
   free(a);
}

typedef struct _general_button
{
   GtkWidget *button;
}general_button;



MODULE_EXPORT void button_below(GtkWidget *w)
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
   P("button_below: %d %d\n",Flags.BelowAllForce, Flags.BelowAll);
   if (!Flags.BelowAllForce)
   {
      if(active)
	 Flags.BelowAll = 1;
      else
      {
	 Flags.BelowAll = 0;
	 bct_countdown  = 9;
	 show_bct_countdown();
	 gtk_widget_hide(Button.BelowAll);
	 gtk_widget_show(Button.BelowConfirm);
	 bct_id = add_to_mainloop(PRIORITY_DEFAULT,time_below_confirm,below_confirm_ticker);
      }
   }
}
MODULE_EXPORT void button_below_confirm()
{
   gtk_widget_hide(Button.BelowConfirm);
   gtk_widget_show(Button.BelowAll);
   remove_from_mainloop(&bct_id);
}


MODULE_EXPORT void combo_screen(GtkComboBoxText *combo, gpointer data)
{
   (void)data;
   int num = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
   P("combo_screen:%d\n",num);
   Flags.Screen = num-1;
}

MODULE_EXPORT void combo_language(GtkComboBoxText *combo, gpointer data)
{
   (void)data;
   int num = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
   P("combo_language:%d\n",num);
   Flags.Language = strdup(lang[num]);
}


static void init_general_buttons()
{
   g_signal_connect(Button.BelowAll, "toggled", G_CALLBACK (button_below), NULL);
   g_signal_connect(Button.BelowConfirm, "toggled", G_CALLBACK(button_below_confirm), NULL);

   my_gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(builder,"id-version")),"xsnow-" VERSION);

   gtk_widget_hide(Button.BelowConfirm);
}


void show_bct_countdown()
{
   sprintf(sbuffer,"Click to\nconfirm %d",bct_countdown);
   gtk_button_set_label(GTK_BUTTON(Button.BelowConfirm),sbuffer);
}

int below_confirm_ticker(void *d)
{
   bct_countdown--;
   show_bct_countdown();
   if (bct_countdown>0)
      return TRUE;
   else
   {
      set_belowall_default();
      return FALSE;
   }
   (void)d;
}

void set_belowall_default()
{
   P("set_belowall_default: %d\n",bct_id);
   remove_from_mainloop(&bct_id);
   Flags.BelowAll = 1;
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Button.BelowAll),Flags.BelowAll);
   gtk_widget_hide(Button.BelowConfirm);
   gtk_widget_show(Button.BelowAll);
}


   MODULE_EXPORT
void button_quit()
{
   Flags.Done = 1;
   P("button_quit: %d\n",Flags.Done);
}


   MODULE_EXPORT 
void button_defaults_general()
{
   P("button_defaults_general\n");
   set_default_tab(xsnow_settings,0);
}

   MODULE_EXPORT 
void button_vintage_general()
{
   P("button_defaults_general vintage\n");
   set_default_tab(xsnow_settings,1);
}

typedef struct _snow_button
{
   GtkWidget *button;
}snow_button;


   MODULE_EXPORT
void button_defaults_snow()
{
   P("button_defaults_snow\n");
   set_default_tab(xsnow_snow,0);
}

   MODULE_EXPORT
void button_vintage_snow()
{
   P("button_vintage_snow\n");
   set_default_tab(xsnow_snow,1);
}

void ui_set_birds_header(const char *text)
{
   if(!ui_running)
      return;
   GtkWidget *birds_header = GTK_WIDGET(gtk_builder_get_object(builder,"birds-header")); 
   my_gtk_label_set_text(GTK_LABEL(birds_header),text);
}

void ui_set_celestials_header(const char *text)
{
   if(!ui_running)
      return;
   GtkWidget *celestials_header = GTK_WIDGET(gtk_builder_get_object(builder,"celestials-header")); 
   char *a = strdup(gtk_label_get_text(GTK_LABEL(celestials_header)));
   assert(a);
   a = (char *) realloc(a,strlen(a)+2+strlen(text));
   assert(a);
   strcat(a,"\n");
   strcat(a,text);
   my_gtk_label_set_text(GTK_LABEL(celestials_header),a);
   free(a);
}


MODULE_EXPORT void button_defaults_birds()
{
   P("button_defaults_birds\n");
   set_default_tab(xsnow_birds,0);
}

MODULE_EXPORT void button_vintage_birds()
{
   P("button_vintage_birds\n");
   set_default_tab(xsnow_birds,1);
}

MODULE_EXPORT void button_birds_restart()
{
   P("button_birds_restart\n");
   Flags.BirdsRestart = 1;
}

MODULE_EXPORT void button_wind_activate()
{
   P("button_wind_activate\n");
   Flags.WindNow = 1;
}

MODULE_EXPORT void button_xscreensaver_activate()
{
   int rc = system("xscreensaver-command -activate");
   (void) rc;
}

void set_default_tab(int tab, int vintage)
{
   int h = human_interaction;
   human_interaction = 0;
   char *background = strdup(Flags.BackgroundFile); // don't want to clear backgroundfile
   if(vintage)
   {
#define togglecode(type,name,m) \
      NEWLINE    if (type == tab) Flags.name = VINTAGE(name); 
#define scalecode togglecode
#define colorcode(type,name,m) \
      NEWLINE    if (type == tab) \
      NEWLINE       { free(Flags.name); Flags.name = strdup(VINTAGE(name)); } 
#define filecode colorcode

      ALL_BUTTONS;
#include "undefall.inc"
      switch(tab)
      {
	 case xsnow_scenery:
	    free(Flags.TreeType);
	    Flags.TreeType = strdup(VINTAGE(TreeType));
	    break;
	 case xsnow_snow:
	    Flags.VintageFlakes = 1;
	    break;
	 case xsnow_santa:
	    Flags.SantaSize = VINTAGE(SantaSize);
	    Flags.Rudolf    = VINTAGE(Rudolf);
	    break;
	 case xsnow_settings:
	    set_belowall_default();
	    free(Flags.BackgroundFile);
	    Flags.BackgroundFile = strdup(background);
	    Flags.Screen = VINTAGE(Screen);
	    break;
      }
   }
   else
#define togglecode(type,name,m) \
      NEWLINE    if (type == tab) Flags.name = DEFAULT(name); 
#define scalecode togglecode
#define colorcode(type,name,m) \
      NEWLINE    if (type == tab) \
      NEWLINE       { free(Flags.name); Flags.name = strdup(DEFAULT(name)); } 
#define filecode colorcode
   {
      ALL_BUTTONS;
#include "undefall.inc"
      switch(tab)
      {
	 case xsnow_scenery:
	    free(Flags.TreeType);
	    Flags.TreeType = strdup(DEFAULT(TreeType));
	    break;
	 case xsnow_snow:
	    Flags.VintageFlakes = 0;
	    break;
	 case xsnow_santa:
	    Flags.SantaSize = DEFAULT(SantaSize);
	    Flags.Rudolf    = DEFAULT(Rudolf);
	    break;
	 case xsnow_settings:
	    set_belowall_default();
	    free(Flags.BackgroundFile);
	    Flags.BackgroundFile = strdup(background);
	    Flags.Screen = DEFAULT(Screen);
	    break;
      }
   }
   set_buttons();
   human_interaction = h;
   free(background);
}

   MODULE_EXPORT
void button_defaults_celestials()
{
   P("button_defaults_wind\n");
   set_default_tab(xsnow_celestials,0);
}

   MODULE_EXPORT
void button_vintage_celestials()
{
   P("button_vintage_wind\n");
   set_default_tab(xsnow_celestials,1);
}

static void init_buttons()
{
   init_buttons1();
   init_santa_buttons();
   init_tree_buttons();
   init_general_buttons();
   nflakeslabel = GTK_WIDGET(gtk_builder_get_object(builder,"nflakes"));
}

void set_buttons()
{
   human_interaction = 0;
   set_buttons1();
   set_santa_buttons();
   set_tree_buttons();
   human_interaction = 1;
}

void all_default(int vintage)
{
   /* xsnow_settings is deliberately not included here */
   set_default_tab(xsnow_santa,vintage);
   set_default_tab(xsnow_scenery,vintage);
   set_default_tab(xsnow_snow,vintage);
   set_default_tab(xsnow_celestials,vintage);
   set_default_tab(xsnow_birds,vintage);
   set_belowall_default();
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
   if(!ui_running)
      return;
   snprintf(sbuffer,nsbuffer,"%6d",n);
   my_gtk_label_set_text(GTK_LABEL(nflakeslabel),sbuffer);
}

void ui_show_range_etc()
{
   if(!ui_running)
      return;
   snprintf(sbuffer,nsbuffer,_("Range: %d\n"),(int)birds_get_range());
   my_gtk_label_set_text(GTK_LABEL(range),sbuffer);
   snprintf(sbuffer,nsbuffer,_("Mean distance: %3d\n"),(int)birds_get_mean_dist());
   my_gtk_label_set_text(GTK_LABEL(mean_distance),sbuffer);
}

void ui_show_desktop_type(const char *s)
{
   if(!ui_running)
      return;
   snprintf(sbuffer,nsbuffer,_("Desktop type: %s"),s);
   my_gtk_label_set_text(GTK_LABEL(desktop_type),sbuffer);
}

void ui_set_sticky(int x)
{
   if(!ui_running)
      return;
   if (x)
      gtk_window_stick(GTK_WINDOW(hauptfenster));
   else
      gtk_window_unstick(GTK_WINDOW(hauptfenster));
}

// https://docs.gtk.org/gtk3/iface.FileChooser.html
void update_preview_cb (GtkFileChooser *file_chooser, gpointer data)
{
   GtkWidget *preview;
   char *filename;
   GdkPixbuf *pixbuf;
   gboolean have_preview;

   preview = GTK_WIDGET (data);
   filename = gtk_file_chooser_get_preview_filename (file_chooser);
   if(!IsReadableFile(filename))
   {
      g_free(filename);
      return;
   }

   int w = global.SnowWinWidth/10;

   //pixbuf = gdk_pixbuf_new_from_file_at_size (filename, 128, 128, NULL);
   pixbuf = gdk_pixbuf_new_from_file_at_size (filename, w, w, NULL);
   have_preview = (pixbuf != NULL);
   g_free (filename);

   gtk_image_set_from_pixbuf (GTK_IMAGE (preview), pixbuf);
   if (pixbuf)
      g_object_unref (pixbuf);

   gtk_file_chooser_set_use_preview_label(file_chooser, FALSE);
   gtk_file_chooser_set_preview_widget_active (file_chooser, have_preview);
}

void headerfunc(GtkWidget *w, gpointer data)
{
   static double last = 0;
   double now = wallclock();
   if (now < last + 0.01)
      return;
   P("headerfunc: now: %lf last: %lf\n",now,last);
   last = now;
   global.time_to_write_flags = TRUE;
   
   (void) w;
   (void) data;
}

void ui()
{
   ui_running = True;

   builder        = gtk_builder_new_from_string (xsnow_xml, -1);
#ifdef HAVE_GETTEXT
   gtk_builder_set_translation_domain(builder,TEXTDOMAIN);
#endif

   gtk_builder_connect_signals  (builder, NULL);
   hauptfenster  = GTK_WIDGET   (gtk_builder_get_object(builder, "hauptfenster"));
   mean_distance = GTK_WIDGET   (gtk_builder_get_object(builder, "birds-mean-distance"));
   range         = GTK_WIDGET   (gtk_builder_get_object(builder, "birds-range"));
   desktop_type  = GTK_WIDGET   (gtk_builder_get_object(builder, "settings-show-desktop-type"));
   birdsgrid     = GTK_CONTAINER(gtk_builder_get_object(builder, "grid_birds"));
   moonbox       = GTK_CONTAINER(gtk_builder_get_object(builder, "moon-box"));

   {
      g_signal_connect(G_OBJECT(hauptfenster),
	    "window-state-event",
	    G_CALLBACK(callback_func),
	    NULL);
   }

   hauptfenstersc  = gtk_widget_get_style_context(hauptfenster);

   handle_css();

   gtk_window_set_title(GTK_WINDOW(hauptfenster),"XsnoW");
   if (getenv("XSNOW_RESTART"))
      gtk_window_set_position(GTK_WINDOW(hauptfenster), GTK_WIN_POS_CENTER_ALWAYS);
   gtk_widget_show_all (hauptfenster);

   init_buttons();
   connect_signals();
   init_pixmaps();
   set_buttons();
   preview = GTK_IMAGE(gtk_image_new());
   gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER(Button.BackgroundFile), GTK_WIDGET(preview));
   g_signal_connect (GTK_FILE_CHOOSER(Button.BackgroundFile), "update-preview",
	 G_CALLBACK (update_preview_cb), preview);


   XineramaScreenInfo *xininfo = XineramaQueryScreens(global.display,&Nscreens);
   if (xininfo == NULL)
   {
      P("No xinerama...\n");
      HaveXinerama = 0;
   }
   else
   {
      HaveXinerama = 1;
   }

   // handle number of screens/monitors

   GtkComboBoxText *ScreenButton;
   ScreenButton = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder,"id-Screen"));

   if (Nscreens < 2)
   {
      gtk_widget_set_sensitive(GTK_WIDGET(ScreenButton),FALSE);
      Flags.Screen = -1;
   }
   if (Flags.Screen < -1)
      Flags.Screen = -1;
   if (Flags.Screen >= Nscreens)
      Flags.Screen = Nscreens - 1;

   gtk_combo_box_text_remove_all(ScreenButton);
   gtk_combo_box_text_append_text(ScreenButton,_("all monitors"));

   char *s = NULL;
   int p = 0;
   for (int i=0; i<Nscreens; i++)
   {
      snprintf(sbuffer,nsbuffer,_("monitor %d"),i);
      s = (char*) realloc(s,p+1+strlen(sbuffer));
      assert(s);
      strcpy(&s[p],sbuffer);
      gtk_combo_box_text_append_text(ScreenButton,&s[p]);
      p += 1+strlen(sbuffer);
   }

   XFree(xininfo);
   gtk_combo_box_set_active(GTK_COMBO_BOX(ScreenButton),Flags.Screen+1);
   g_signal_connect (ScreenButton,"changed",G_CALLBACK(combo_screen),NULL);

   // handle languages

   P("LANGUAGES %s\n",LANGUAGES);

   GtkComboBoxText *LangButton;
   LangButton =  GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder,"id-Lang"));

   strcpy(sbuffer,_("Available languages are: "));
   strcat(sbuffer,LANGUAGES);
   strcat(sbuffer,".\n");
   strcat(sbuffer,_("Use \"sys\" for your default language.\n"));
   strcat(sbuffer,_("See also the man page."));
   gtk_widget_set_tooltip_text(GTK_WIDGET(LangButton),sbuffer);

   gtk_combo_box_text_remove_all(LangButton);
   gtk_combo_box_text_append_text(LangButton,"sys");
   lang[0] = strdup("sys");

   char *languages = strdup(LANGUAGES);
   char *token = strtok(languages," ");
   nlang = 1;
   while(token != NULL)
   {
      P("token: %s\n",token);
      lang[nlang] = strdup(token);
      gtk_combo_box_text_append_text(LangButton, lang[nlang]);
      nlang++;
      token=strtok(NULL, " ");
   }

   gtk_combo_box_set_active(GTK_COMBO_BOX(LangButton),0);
   for (int i=0; i<nlang; i++)
   {
      if (!strcmp(lang[i],Flags.Language))
      {
	 gtk_combo_box_set_active(GTK_COMBO_BOX(LangButton),i);
	 break;
      }
   }

   g_signal_connect(LangButton,"changed",G_CALLBACK(combo_language),NULL);

   if (strlen(LANGUAGES) == 0)
      gtk_widget_destroy(GTK_WIDGET(LangButton));

   if (Flags.HideMenu)
      gtk_window_iconify(GTK_WINDOW(hauptfenster));

   // connect handler to radiobuttons in the headerbar

   {
      GtkStackSwitcher* switcher = GTK_STACK_SWITCHER(gtk_builder_get_object(builder,"id-tabs"));
      GList *list = gtk_container_get_children((GtkContainer*)switcher);  // this is a list with radiobuttons

      GList *p = list;
      while(p != NULL)
      {
	 g_signal_connect(G_OBJECT(p->data),"clicked",G_CALLBACK(headerfunc),NULL);
	 p = p->next;
      }
   }

   global.builder = builder;
   if(s)
      free(s);
}

// Set the style provider for the widgets
static void apply_css_provider (GtkWidget *widget, GtkCssProvider *cssstyleProvider)
{
   P("apply_css_provider %s\n",gtk_widget_get_name(GTK_WIDGET(widget)));

   gtk_style_context_add_provider ( gtk_widget_get_style_context(widget), 
	 GTK_STYLE_PROVIDER(cssstyleProvider) , 
	 GTK_STYLE_PROVIDER_PRIORITY_APPLICATION );

   // For container widgets, apply to every child widget on the container
   if (GTK_IS_CONTAINER (widget))
   {
      gtk_container_forall( GTK_CONTAINER (widget),
	    (GtkCallback)apply_css_provider ,
	    cssstyleProvider);
   }
}


void handle_css()
{
   const char *css     = 
      // I wish how I could copy the Adwaita settings ...
      //".xsnow button { padding-left: 16px; padding-right: 16px; padding-top: 4px; padding-bottom: 4px;}"
      //".xsnow headerbar button { padding-left: 10px; padding-right: 10px; padding-top: 4px; padding-bottom: 4px;}"
      //".xsnow headerbar.titlebar { border-color: rgb(213,208,204); border-style:solid; border-bottom-left-radius: 0px; border-bottom-right-radius: 0px; border-top-left-radius: 8px; border-top-right-radius: 8px;}"
      //".xsnow headerbar label.title { padding-left: 12px; padding-right:12px;}"
      //".xsnow button.color {padding: 4px; }"
      //".xsnow headerbar stackswitcher button.radio label       { color: #065522;  }"   
      //".xsnow headerbar stackswitcher button.radio        { box-shadow: 0px 0px; border-top-width: 0px;  }"   

      // These are not colors, but nevertheless I think we should do this always:
      "scale              { padding:       1em;                    }"   // padding in slider buttons
      "button.radio       { min-width:     10px;                   }"   // make window as narrow as possible
      "label.busymessage  { border-radius: 4px;  min-height: 3.5em }"   // info message in welcome tab

      // colors: (the buttons in the headerbar need some work)
      ".xsnow *                                          { border-color:     #B4EEB4; }"   // border colors
      ".xsnow button                                     { background:       #CCF0D8; }"   // color of normal buttons
      ".xsnow button.radio,        .xsnow button.toggle  { background:       #E2FDEC; }"   // color of radio and toggle buttons
      ".xsnow combobox > window.popup *                  { background:       #E2FDEC; }"   // does not work, somebody?
      ".xsnow radiobutton:active,  .xsnow button:active  { background:       #0DAB44; }"   // color of buttons while being activated
      ".xsnow radiobutton:checked, .xsnow button:checked { background:       #6AF69B; }"   // color of checked buttons
      ".xsnow headerbar                                  { background:       #B3F4CA; }"   // color of headerbar
      ".xsnow scale slider                               { background:       #D4EDDD; }"   // color of sliders
      ".xsnow scale trough                               { background:       #0DAB44; }"   // color of trough of sliders
      ".xsnow stack                                      { background:       #EAFBF0; }"   // color of main area
      ".xsnow *                                          { color:            #065522; }"   // foreground color (text)
      ".xsnow *:disabled *                               { color:            #8FB39B; }"   // foreground color for disabled items
      ".busy stack                                       { background:       #FFC0CB; }"   // background color when too busy
      ".busy .cpuload slider                             { background:       #FF0000; }"   // color of some sliders when too busy
      "button.confirm                                    { background:       #FFFF00; }"   // color for confirm above windows
      ".xsnow button.confirm                             { background-color: #FFFF00; }"   // yes we need both, but why?
      "label.busymessage                                 { background:       #FFC0CB; }"   // info message in welcome tab
      ;

   static GtkCssProvider *cssProvider = NULL;
   if (!cssProvider)
   {
      cssProvider  = gtk_css_provider_new();
      gtk_css_provider_load_from_data (cssProvider, css,-1,NULL);
      apply_css_provider(hauptfenster, cssProvider);
   }

   handle_theme();

}

// if m==1: change some colors of the ui
// if m==0: change back to default colors

void ui_background(int m)
{
   if(!ui_running)
      return;
   if(m)
      gtk_style_context_add_class(hauptfenstersc,"busy");
   else
      gtk_style_context_remove_class(hauptfenstersc,"busy");

}


// m=0: make active
// m=1: make inactive
// however, see transparency and below 
void ui_gray_erase(int m)
{
   gtk_widget_set_sensitive(Button.BelowAll,                      m);
   if(!Flags.BelowAllForce)
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
   if(!ui_running)
      return;
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
   // see https://fuchsia.googlesource.com/third_party/glib/+/refs/heads/upstream/main/NEWS
#define MY_GLIB_VERSION (100000000*GLIB_MAJOR_VERSION + 10000*GLIB_MINOR_VERSION + GLIB_MICRO_VERSION)
#if MY_GLIB_VERSION >= 200730003
#define XXFLAGS G_APPLICATION_DEFAULT_FLAGS
#else
#define XXFLAGS G_APPLICATION_FLAGS_NONE
#endif

   app = gtk_application_new ("nl.ratrabbit.example", XXFLAGS);
   g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
   g_application_run (G_APPLICATION (app), 0, NULL);
   g_object_unref (app);
   return RC;
}

static void activate (GtkApplication *app)
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

void yesyes(GtkWidget *w, gpointer window)
{
   RC = (w != NULL);
   gtk_widget_destroy(GTK_WIDGET(window));
}

void nono(GtkWidget *w, gpointer window)
{
   RC = (w == NULL);
   gtk_widget_destroy(GTK_WIDGET(window));
}

// next function is not used, I leave it here as a template, who knows...
// see also ui.xml
void ui_error_x11()
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
   pixbuf = xpm2gdk(global.display, (char **)xsnow_logo, NULL);
   image = GTK_IMAGE(gtk_builder_get_object(builder,"error-x11-image1"));
   gtk_image_set_from_pixbuf(image,pixbuf);
   image = GTK_IMAGE(gtk_builder_get_object(builder,"error-x11-image2"));
   gtk_image_set_from_pixbuf(image,pixbuf);
   g_object_unref(pixbuf);

   gtk_widget_show_all (errorfenster);
   gtk_main();
}

void my_gtk_label_set_text(GtkLabel *label, const gchar *str)
{
   if(ui_running)
      gtk_label_set_text(label, str);
}
