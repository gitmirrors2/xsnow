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
#include "debug.h"
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
      const char *data[], Pixmap *p, Pixmap *s, XpmAttributes *attr, int flop)
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
   if (rc != 0)
   {
      I("rc from XpmCreateImageFromData: ");
      switch (rc)
      {
	 case 1:
	    printf("XpmColorError\n");
	    break;
	 case -1:
	    printf("XpmOpenFailed\n");
	    break;
	 case -2:
	    printf("XpmFileInvalid\n");
	    break;
	 case -3:
	    printf("XpmNoMemory\n");
	    break;
	 case -4:
	    printf("XpmColorFaild\n");
	    break;
	 default:
	    printf("%d\n",rc);
	    break;
      }
      printf("exiting\n");
      abort();
   }
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
Region regionfromxpm(const char **data, int flop)
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

// given color and xmpdata **data of a monocolored picture like:
// 
//XPM_TYPE *snow06_xpm[] = {
///* columns rows colors chars-per-pixel */
//"3 3 2 1 ",
//"  c none",
//". c black",
///* pixels */
//". .",
//" . ",
//". ."
//};
// change the second color to color and put the result in out.
// lines will become the number of lines in out, comes in handy
// when wanteing to free out.
void xpm_set_color(const char **data, char ***out, int *lines, const char *color)
{
   int n;  
   sscanf(data[0],"%*d %d",&n);
   *out = (char**)malloc(sizeof(char *)*(n+3));
   char **x = *out;
   int j;
   for (j=0; j<2; j++)
      x[j] = strdup(data[j]);
   x[2] = (char *)malloc(5+sizeof(color));
   x[2][0] = 0;
   strcat(x[2],". c ");
   strcat(x[2],color);
   P("c: [%s]\n",x[2]);

   for (j=3; j<n+3; j++)
   {
      x[j] = strdup(data[j]);
      P("%d %s\n",j,x[j]);
   }
   *lines = n+3;
}

void xpm_destroy(char **data, int lines)
{
   int i;
   for (i=0; i<lines; i++)
      free(data[i]);
   free(data);
}

