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

#define NOTACTIVE \
   (Flags.BirdsOnly || !WorkspaceActive())
#include <stdio.h>
#include <X11/xpm.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "scenery.h"
#include "windows.h"
#include "utils.h"
#include "ixpm.h"
#include "flags.h"
#include "pixmaps.h"

static void   InitTreePixmaps(void);
void create_tree_surface(int tt,int flip, const char **xpm);
static cairo_surface_t *tree_surfaces[MAXTREETYPE+1][2];

void scenery_init()
{
   InitTreePixmaps();
}

int scenery_draw(cairo_t *cr)
{
   if (Flags.Done)
      return FALSE;
   if (NOTACTIVE)
      return TRUE;
   int i;
   for (i=0; i<NTrees; i++)
   {
      Treeinfo *tree = &Trees[i];
      cairo_surface_t *surface = tree_surfaces[tree->type][tree->rev];
      cairo_set_source_surface (cr, surface, tree->x+100, tree->y);
      cairo_paint(cr);
   }
   return TRUE;
}
/*
   void drawtree(Treeinfo *tree) 
   {
   if (Flags.Done)
   return FALSE;
   if (NOTACTIVE)
   return TRUE;
   if (KillTrees)
   {
   free(tree);
   NTrees--;
   return FALSE;
   }
   int x = tree->x; int y = tree->y; int t = tree->type; int r = tree->rev;
   P("t = %d %d\n",t,(int)wallclock());
   if (t<0) t=0;
   XSetClipMask(display, TreeGC, TreeMaskPixmap[t][r]);
   XSetClipOrigin(display, TreeGC, x, y);
   XCopyArea(display, TreePixmap[t][r], SnowWin, TreeGC, 
   0,0,TreeWidth[t],TreeHeight[t], x, y);
   return TRUE;
   }
   */

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
   tree_surfaces[tt][flip] = gdk_cairo_surface_create_from_pixbuf (pixbuf, 0, gdkwindow);
   g_clear_object(&pixbuf);
   sscanf(xpmtrees[tt][0],"%d %d",&TreeWidth[tt],&TreeHeight[tt]);

}

void InitTreePixmaps()
{
   XpmAttributes attributes;
   attributes.valuemask = XpmDepth;
   attributes.depth     = SnowWinDepth;
   char *path;
   FILE *f = HomeOpen("xsnow/pixmaps/tree.xpm","r",&path);
   if (f)
   {
      // there seems to be a local definition of tree
      // set TreeType to some number, so we can respond accordingly
      free(TreeType);
      TreeType = (int *)malloc(sizeof(*TreeType));
      NtreeTypes = 1;
      TreeRead = 1;
      int rc = XpmReadFileToData(path,&TreeXpm);
      if(rc == XpmSuccess)
      {
	 int i;
	 for(i=0; i<2; i++)
	    iXpmCreatePixmapFromData(display, SnowWin, TreeXpm, 
		  &TreePixmap[0][i], &TreeMaskPixmap[0][i], &attributes,i);
	 create_tree_surface(0,i,(const char **)TreeXpm);
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
      free(path);
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
   char **xpmtmp = (char **)alloca(n*sizeof(char *));
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
      iXpmCreatePixmapFromData(display, SnowWin, xpmtmp,
	    &TreePixmap[0][i],&TreeMaskPixmap[0][i],&attributes,i);
      cairo_surface_destroy(tree_surfaces[0][i]);
      create_tree_surface(0,i,(const char **)xpmtmp);
   }
   for (j=0; j<n; j++)
      free(xpmtmp[j]);
}


int do_drawtree(Treeinfo *tree) 
{
   if (Flags.Done)
      return FALSE;
   if (NOTACTIVE)
      return TRUE;
   if (KillTrees)
   {
      free(tree);
      NTrees--;
      return FALSE;
   }
   int x = tree->x; int y = tree->y; int t = tree->type; int r = tree->rev;
   P("t = %d %d\n",t,(int)wallclock());
   if (t<0) t=0;
   XSetClipMask(display, TreeGC, TreeMaskPixmap[t][r]);
   XSetClipOrigin(display, TreeGC, x, y);
   XCopyArea(display, TreePixmap[t][r], SnowWin, TreeGC, 
	 0,0,TreeWidth[t],TreeHeight[t], x, y);
   return TRUE;
}
