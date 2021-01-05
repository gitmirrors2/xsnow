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

#define DEFAULTTREETYPE 2

#define NOTACTIVE \
   (Flags.BirdsOnly || !WorkspaceActive())

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <X11/Intrinsic.h>
#include <X11/xpm.h>
#include "debug.h"
#include "scenery.h"
#include "windows.h"
#include "utils.h"
#include "ixpm.h"
#include "flags.h"
#include "pixmaps.h"
#include "fallensnow.h"
#include "csvpos.h"
#include "treesnow.h"
#include "varia.h"

static int      do_drawtree(Treeinfo *tree);
static int      do_initbaum(gpointer data);
static void     ReInitTree0(void);
static void     InitTreePixmaps(void);
static void     RedrawTrees(void);
static void     create_tree_surface(int tt,int flip, const char **xpm);
static int      NtreeTypes = 0;
static int      TreeRead = 0;
static char     **TreeXpm = NULL;
static Pixmap   TreePixmap[MAXTREETYPE+1][2];
static Pixmap   TreeMaskPixmap[MAXTREETYPE+1][2];
static int      TreeWidth[MAXTREETYPE+1], TreeHeight[MAXTREETYPE+1];
static int     *TreeType = NULL;
static int      NTrees     = 0;  // actual number of trees
static GC       TreeGC;
static Treeinfo **Trees = NULL;

Region TreeRegion;

static cairo_surface_t *tree_surfaces[MAXTREETYPE+1][2];

void scenery_init()
{
   TreeGC        = XCreateGC(display, SnowWin, 0, NULL);
   TreeRegion    = XCreateRegion();
   InitTreePixmaps();
   add_to_mainloop(PRIORITY_DEFAULT, time_initbaum,       (GSourceFunc)do_initbaum           ,NULL);
}

void scenery_set_gc()
{
   XSetFunction(display,   TreeGC, GXcopy);
   XSetForeground(display, TreeGC, BlackPix);
   XSetFillStyle(display,  TreeGC, FillStippled);
}

int scenery_draw(cairo_t *cr)
{
   int i;


   for (i=0; i<NTrees; i++)
   {
      Treeinfo *tree = Trees[i];
      cairo_surface_t *surface = tree_surfaces[tree->type][tree->rev];
      cairo_set_source_surface (cr, surface, tree->x, tree->y);
      my_cairo_paint_with_alpha(cr,ALPHA);
   }
   return TRUE;
}

void scenery_ui()
{
   UIDOS(TreeType               , RedrawTrees(););
   UIDO (DesiredNumberOfTrees   , RedrawTrees(););
   UIDO (TreeFill               , RedrawTrees(););
   UIDO (NoTrees                , RedrawTrees(););
   UIDOS(TreeColor              , ReInitTree0(););
}

void RedrawTrees()
{
   // remove trees from timeout callbacks:
   int i;
   for (i=0; i<NTrees; i++)
   {
      Treeinfo *tree = Trees[i];
      int rc = g_source_remove_by_user_data(tree);
      P("removed %d %d %p\n",i,rc,(void *)tree);
      if (rc)
	 free(tree);
      else
	I("This should not happen i=%d rc=%d tree=%p\n",i,rc,(void *)tree);
   }
   NTrees = 0;     // this signals initbaum to recreate the trees
   reinit_treesnow_region();
   ClearScreen();
}

void EraseTrees()
{
   RedrawTrees();
}

void create_tree_surface(int tt,int flip, const char **xpm)
{
   GdkPixbuf *pixbuf, *pixbuf1;
   pixbuf1 = gdk_pixbuf_new_from_xpm_data((const char **)xpm);
   if (flip)
   {
      pixbuf = gdk_pixbuf_flip(pixbuf1,1);
      g_clear_object(&pixbuf1);
   }
   else
      pixbuf = pixbuf1;
   tree_surfaces[tt][flip] = gdk_cairo_surface_create_from_pixbuf (pixbuf, 0, NULL);
   g_clear_object(&pixbuf);
   sscanf(xpmtrees[tt][0],"%d %d",&TreeWidth[tt],&TreeHeight[tt]);

}

// fallen snow and trees must have been initialized 
// tree coordinates and so are recalculated here, in anticipation
// of a changed window size
// The function returns immediately if NTrees!=0, otherwize an attempt
// is done to place the DesiredNumberOfTrees
int do_initbaum(UNUSED gpointer data)
{
   if (Flags.Done)
      return FALSE;
   P("initbaum %d %d\n",NTrees, counter++);
   if (Flags.NoTrees || NTrees != 0)
      return TRUE;
   int i,h,w;

   XDestroyRegion(SnowOnTreesRegion);
   cairo_region_destroy(gSnowOnTreesRegion);
   XDestroyRegion(TreeRegion);

   SnowOnTreesRegion = XCreateRegion();
   gSnowOnTreesRegion = cairo_region_create();
   TreeRegion        = XCreateRegion();

   // determine which trees are to be used
   //
   int *tmptreetype, ntemp;
   if(TreeRead)
   {
      TreeType = (int *)realloc(TreeType,1*sizeof(*TreeType));
      TreeType[0] = 0;
   }
   else
   {
      if (!strcmp("all",Flags.TreeType))
	 // user wants all treetypes
      {
	 ntemp = 1+MAXTREETYPE;
	 tmptreetype = (int *)malloc(sizeof(*tmptreetype)*ntemp);
	 int i;
	 for (i=0; i<ntemp; i++)
	    tmptreetype[i] = i;
      }
      else if (strlen(Flags.TreeType) == 0) 
	 // default: use 1..MAXTREETYPE 
      {
	 ntemp = MAXTREETYPE;
	 tmptreetype = (int *)malloc(sizeof(*tmptreetype)*ntemp);
	 int i;
	 for (i=0; i<ntemp; i++)
	    tmptreetype[i] = i+1;
      }
      else
      {
	 // decode string like "1,1,3,2,4"
	 csvpos(Flags.TreeType,&tmptreetype,&ntemp);
      }

      NtreeTypes = 0;
      for (i=0; i<ntemp; i++)
      {
	 if (tmptreetype[i] >=0 && tmptreetype[i]<=MAXTREETYPE)
	 {
	    int j;
	    int unique = 1;
	    // investigate if this is already contained in TreeType.
	    // if so, do not use it. Note that this algorithm is not
	    // good scalable, when ntemp is large (100 ...) one should
	    // consider an algorithm involving qsort()
	    //
	    for (j=0; j<NtreeTypes; j++)
	       if (tmptreetype[i] == TreeType[j])
	       {
		  unique = 0;
		  break;
	       }
	    if (unique) 
	    {
	       TreeType = (int *)realloc(TreeType,(NtreeTypes+1)*sizeof(*TreeType));
	       TreeType[NtreeTypes] = tmptreetype[i];
	       NtreeTypes++;
	    }
	 }
      }
      if(NtreeTypes == 0)
      {
	 TreeType = (int *)realloc(TreeType,sizeof(*TreeType));
	 TreeType[0] = DEFAULTTREETYPE;
	 NtreeTypes++;
      }
      free(tmptreetype);
   }

   // determine placement of trees and NTrees:

   NTrees    = 0;
   for (i=0; i< 4*Flags.DesiredNumberOfTrees; i++) // no overlap permitted
   {
      if (NTrees >= Flags.DesiredNumberOfTrees)
	 break;

      int tt = TreeType[randint(NtreeTypes)];
      h = TreeHeight[tt];
      w = TreeWidth[tt];

      int y1 = SnowWinHeight - MaxScrSnowDepth - h;
      int y2 = SnowWinHeight*(1.0 - 0.01*Flags.TreeFill);
      if (y2>y1) y1=y2+1;

      int x = randint(SnowWinWidth-w);
      int y = y1 - randint(y1-y2);

      int in = XRectInRegion(TreeRegion,x,y,w,h);
      if (in == RectangleIn || in == RectanglePart)
	 continue;

      int flop = (drand48()>0.5);

      Treeinfo *tree = (Treeinfo *)malloc(sizeof(Treeinfo));
      tree->x    = x;
      tree->y    = y;
      tree->type = tt;
      tree->rev  = flop;
      P("tree: %d %d %d %d %d %p\n",tree->x, tree->y, tree->type, tree->rev, NTrees,(void *)tree);

      add_to_mainloop(PRIORITY_DEFAULT, time_tree, (GSourceFunc)do_drawtree, tree);

      Region r;

      switch(tt)
      {
	 case -SOMENUMBER:
	    r = regionfromxpm((const char **)TreeXpm,tree->rev);
	    break;
	 default:
	    r = regionfromxpm(xpmtrees[tt],tree->rev);
	    break;
      }
      XOffsetRegion(r,x,y);
      XUnionRegion(r,TreeRegion,TreeRegion);
      XDestroyRegion(r);

      NTrees++;
      Trees = (Treeinfo **)realloc(Trees,NTrees*sizeof(Treeinfo*));
      Trees[NTrees-1] = tree;
   }
   OnTrees = 0;
   return TRUE;
}

void InitTreePixmaps()
{
   XpmAttributes attributes;
   attributes.valuemask = XpmDepth;
   attributes.depth     = SnowWinDepth;
   char *path = NULL;
   FILE *f = HomeOpen("xsnow/pixmaps/tree.xpm","r",&path);
   if (f)
   {
      // there seems to be a local definition of tree
      // set TreeType to some number, so we can respond accordingly
      TreeType = (int *)realloc(TreeType,sizeof(*TreeType));
      NtreeTypes = 1;
      TreeRead = 1;
      int rc = XpmReadFileToData(path,&TreeXpm);
      if(rc == XpmSuccess)
      {
	 int i;
	 for(i=0; i<2; i++)
	 {
	    iXpmCreatePixmapFromData(display, SnowWin, (const char **)TreeXpm, 
		  &TreePixmap[0][i], &TreeMaskPixmap[0][i], &attributes,i);
	    create_tree_surface(0,i,(const char **)TreeXpm);
	 }
	 sscanf(*TreeXpm,"%d %d", &TreeWidth[0],&TreeHeight[0]);
	 printf("using external tree: %s\n",path);
	 if (!Flags.NoMenu)
	    printf("Disabling menu.\n");
	 Flags.NoMenu = 1;
      }
      else
      {
	 printf("Invalid external xpm for tree given: %s\n",path);
	 exit(1);
      }
      fclose(f);
   }
   else
   {
      int i;
      for(i=0; i<2; i++)
      {
	 int tt;
	 for (tt=0; tt<=MAXTREETYPE; tt++)
	 {
	    iXpmCreatePixmapFromData(display, SnowWin, xpmtrees[tt],
		  &TreePixmap[tt][i],&TreeMaskPixmap[tt][i],&attributes,i);
	    sscanf(xpmtrees[tt][0],"%d %d",&TreeWidth[tt],&TreeHeight[tt]);
	    create_tree_surface(tt,i,(const char **)xpmtrees[tt]);
	 }
      }
      ReInitTree0();
   }
   if(path)
      free(path);
   OnTrees = 0;
}


//
// apply TreeColor to xpmtree[0] and xpmtree[1]
void ReInitTree0()
{
   XpmAttributes attributes;
   attributes.valuemask = XpmDepth;
   attributes.depth     = SnowWinDepth;
   int i;
   int n = TreeHeight[0]+3;
   //char *xpmtmp[n];
   char **xpmtmp = (char **)malloc(n*sizeof(char *));
   int j;
   for (j=0; j<2; j++)
      xpmtmp[j] = strdup(xpmtrees[0][j]);
   xpmtmp[2] = strdup(". c ");
   xpmtmp[2] = (char *)realloc(xpmtmp[2],strlen(xpmtmp[2])+strlen(Flags.TreeColor)+1);
   strcat(xpmtmp[2],Flags.TreeColor);
   for(j=3; j<n; j++)
      xpmtmp[j] = strdup(xpmtrees[0][j]);
   for(i=0; i<2; i++)
   {
      XFreePixmap(display,TreePixmap[0][i]);
      iXpmCreatePixmapFromData(display, SnowWin, (const char **)xpmtmp,
	    &TreePixmap[0][i],&TreeMaskPixmap[0][i],&attributes,i);
      cairo_surface_destroy(tree_surfaces[0][i]);
      create_tree_surface(0,i,(const char **)xpmtmp);
   }
   for (j=0; j<n; j++)
      free(xpmtmp[j]);
   free(xpmtmp);
}


int do_drawtree(Treeinfo *tree) 
{
   if (Flags.Done)
      return FALSE;
   if (NOTACTIVE)
      return TRUE;
   if (switches.UseGtk)
      return TRUE;
   int x = tree->x; int y = tree->y; int t = tree->type; int r = tree->rev;
   P("t = %d %d\n",t,(int)wallclock());
   if (t<0) t=0;
   XSetClipMask(display, TreeGC, TreeMaskPixmap[t][r]);
   XSetClipOrigin(display, TreeGC, x, y);
   XCopyArea(display, TreePixmap[t][r], SnowWin, TreeGC, 
	 0,0,TreeWidth[t],TreeHeight[t], x, y);
   return TRUE;
}
