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
#ifndef X11CAIRO_H
#define X11CAIRO_H
#ifdef USEX11
typedef Region     REGION;
typedef int        REGION_OVERLAP_T;
typedef XRectangle RECTANGLE;
static Region region_create_rectangle(int x, int y, int w, int h)
{
   XPoint p[5];
   p[0] = (XPoint){x  , y  };
   p[1] = (XPoint){x+w, y  };
   p[2] = (XPoint){x+w, y+h};
   p[3] = (XPoint){x  , y+h}; 
   p[4] = (XPoint){x  , y  };
   return XPolygonRegion(p, 5, EvenOddRule);
}
#define REGION_CREATE() XCreateRegion()
#define REGION_DESTROY(r) XDestroyRegion(r)
#define REGION_SUBTRACT(x,y) XSubtractRegion(x,y,x)
#define REGION_UNION(x,y) XUnionRegion(x,y,x)
#define REGION_TRANSLATE(r,dx,dy) XOffsetRegion(r,dx,dy)
#define REGION_OVERLAP_RECT(r,x,y,w,h) XRectInRegion(r,x,y,w,h)
#define REGION_OVERLAP_RECTANGLE_IN RectangleIn
#define REGION_OVERLAP_RECTANGLE_PART RectanglePart
#define REGION_CREATE_RECTANGLE(x,y,w,h) region_create_rectangle(x,y,w,h)
#define REGION_UNION_RECTANGLE(r,x,y,w,h) XUnionRectWithRegion( \
      &(RECTANGLE){x,y,w,h}, r,r)

#else

typedef cairo_region_t         *REGION;
typedef cairo_region_overlap_t REGION_OVERLAP_T;
typedef cairo_rectangle_int_t  RECTANGLE;
#define REGION_CREATE() cairo_region_create()
#define REGION_DESTROY(r) cairo_region_destroy(r)
#define REGION_SUBTRACT(x,y) cairo_region_subtract(x,y)
#define REGION_UNION(x,y) cairo_region_union(x,y)
#define REGION_TRANSLATE(r,dx,dy) cairo_region_translate(r,dx,dy)
#define REGION_OVERLAP_RECT(r,x,y,w,h) cairo_region_contains_rectangle( \
      r,&(cairo_rectangle_int_t){x,y,w,h})
#define REGION_OVERLAP_RECTANGLE_IN CAIRO_REGION_OVERLAP_IN
#define REGION_OVERLAP_RECTANGLE_PART CAIRO_REGION_OVERLAP_PART
#define REGION_CREATE_RECTANGLE(x,y,w,h) cairo_region_create_rectangle( \
      &(cairo_rectangle_int_t){x,y,w,h})
#define REGION_UNION_RECTANGLE(r,x,y,w,h) cairo_region_union_rectangle( \
      r,&(RECTANGLE){x,y,w,h})

#endif
#endif
