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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ixpm.h"
// from the xpm package:
static void xpmCreatePixmapFromImage(
      Display	*display,
      Drawable	 d,
      XImage	*ximage,
      Pixmap	*pixmap_return)
{
   GC gc;
   XGCValues values;

   *pixmap_return = XCreatePixmap(display, d, ximage->width,
	 ximage->height, ximage->depth);
   /* set fg and bg in case we have an XYBitmap */
   values.foreground = 1;
   values.background = 0;
   gc = XCreateGC(display, *pixmap_return,
	 GCForeground | GCBackground, &values);

   XPutImage(display, *pixmap_return, gc, ximage, 0, 0, 0, 0,
	 ximage->width, ximage->height);

   XFreeGC(display, gc);
}

void paintit(XImage *img, long int color)
{
   int x,y;
   for (y=0; y<img->height; y++)
      for (x=0; x<img->width; x++)
      {
	 XPutPixel(img, x,y,color);
      }
}


// reverse characters in string, characters taken in chunks of l
// if you know what I mean
static void strrevert(char*s, size_t l)
{
   size_t n = strlen(s)/l;
   size_t i;
   char *c = (char *)malloc(l*sizeof(*c));
   char *a = s;
   char *b = s+strlen(s)-l;
   for (i=0; i<n/2; i++)
   {
      strncpy(c,a,l);
      strncpy(a,b,l);
      strncpy(b,c,l);
      a+=l;
      b-=l;
   }
   free(c);
}

//
//  equal to XpmCreatePixmapFromData, with extra flags:
//  flop: if 1, reverse the data horizontally
//  Extra: 0xff000000 is added to the pixmap data
//
int iXpmCreatePixmapFromData(Display *display, Drawable d, 
      char *data[], Pixmap *p, Pixmap *s, XpmAttributes *attr, int flop)
{
   int rc, lines, i, ncolors, height, w;
   char **idata;

   sscanf(data[0],"%*s %d %d %d", &height, &ncolors, &w);
   lines = height+ncolors+1;
   idata = (char **)malloc(lines*sizeof(*idata));
   for (i=0; i<lines; i++)
      idata[i] = strdup(data[i]);
   if(flop)
      // flop the image data
      for (i=1+ncolors; i<lines; i++)
	 strrevert(idata[i],w);

   XImage *ximage,*shapeimage;
   rc = XpmCreateImageFromData(display,idata,&ximage,&shapeimage,attr);
   XAddPixel(ximage,0xff000000);
   if(p)
      xpmCreatePixmapFromImage(display, d, ximage, p);
   if(s)
      xpmCreatePixmapFromImage(display, d, shapeimage, s);
   XDestroyImage(ximage);
   XDestroyImage(shapeimage);
   for(i=0; i<lines; i++) free(idata[i]);
   free(idata);
   return rc;
}

// given xpmdata **data, add the non-transparent pixels to Region r
Region regionfromxpm(char **data, int flop)
{
   int w,h,nc,n;
   Region r = XCreateRegion();
   // width, height, #colors, $chars to code color
   sscanf(*data,"%d %d %d %d",&w,&h,&nc,&n);
   // find color "None":
   int i;
   char *code = (char *)"";
   int offset = nc + 1;
   for(i=1; i<=nc; i++)
   {
      char s[100];
      //printf("%d: %s\n",__LINE__,data[i]);
      sscanf(data[i]+n,"%*s %100s",s);
      //printf("%d: %s\n",__LINE__,s);
      if(!strcmp(s,"None"))
      {
	 code = strndup(data[i],n);
	 break;
      }
   }
   XRectangle rect;
   rect.width = 1;
   rect.height = 1;
   int y;
   for (y=0; y<h; y++)
   {
      int x;
      char*s = strdup(data[y+offset]);
      if(flop)
	 strrevert(s,n);
      for(x=0; x<w; x++)
      {
	 if (strncmp(s+n*x,code,n))
	 {
	    rect.x = x;
	    rect.y = y;
	    XUnionRectWithRegion(&rect,r,r);
	 }
      }
      free(s);
   }
   free(code);
   return r;
}
