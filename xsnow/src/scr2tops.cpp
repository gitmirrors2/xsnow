/* 
   -copyright-
# xsnow: let it snow on your desktop
# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#              2019,2020,2021,2022,2023,2024,2025,2026 Willem Vermin
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
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/param.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "wv_matalloc.h"
#include "scr2tops.h"
#include "debug.h"

#define NDEBUG
#include <assert.h>

// if defined, you will see the stages the program is going through.
//#define SRC2TOPS_TESTING

#define scaleinv(x) (int)(x/scale + 0.5)

// see also: https://github.com/isela31/convolution_2D

static void ImageFromDisplay(std::vector<unsigned char>& Pixels, int x, int y, int Width, int Height, int& BitsPerPixel);
static void find_top_edges(unsigned char **in, int m, int n, int mk, int nk, int max0, unsigned char **left, unsigned char **right);

#ifdef SRC2TOPS_TESTING
static double wallclock(void);
double wallclock() 

{ 
   struct timeval toot;
   double r;

   gettimeofday(&toot,0);
   r=toot.tv_sec+0.000001*(double)toot.tv_usec;
   return(r);
}
#endif


void ImageFromDisplay(std::vector<unsigned char>& Pixels, int x, int y, int Width, int Height, int& BitsPerPixel)
{
   Display* display = XOpenDisplay(nullptr);
   Window root = DefaultRootWindow(display);

   XWindowAttributes attributes;
   XGetWindowAttributes(display, root, &attributes);

   XImage* img = XGetImage(display, root, x, y , Width, Height, AllPlanes, ZPixmap);
   if (img == NULL)
   {
      I("XGetImage failure, quitting... x: %d y: %d Width: %d Height: %d\n",
	    x,y,Width,Height);
      exit(1);
   }
   BitsPerPixel = img->bits_per_pixel;

   for (int i=0; i< (Width * Height * 4); i++)
      Pixels.push_back(img->data[i]);

   XDestroyImage(img);
   XCloseDisplay(display);
}

// find top edges
// in: input: pointers to input image. 
//     pointers must be set such that in[i][j] i=0..m-1, j=0..n-1 points to 
//     image[i][j]. After processing some elements will be set to zero.
// m: input: number of rows of image
// n: input: number of colums of image
// mk: input: number of kernel rows
// nk: input number of kernel colums
// max0: allow at most max0 gaps in the left and right edge detections
// left: input: see in. The array left is pointing to will be the convolution 
// of 'in' and the left kernel.
// right: see left
// the maximum dimensions of left and right:
//    left: 0..m-mk+1
//    right: 0..n-nk+1
//
void find_top_edges(unsigned char **in, int m, int n, int mk, int nk, int max0, 
      unsigned char **left, unsigned char **right)
{
   int mout = m - mk + 1;
   int nout = n - nk + 1;
   //std::cout << "find_top_edges: " << __LINE__ << ": "<<m<< " "<<n << " " << mk << " " << nk << " " << mout << " " <<nout << std::endl;

   for (int i=0; i < mout; i++)
      for (int j=0; j < nout; j++)
      {
	 int leftsum  = 0;  // number of zeros
	 int rightsum = 0;  // number of zeros
	 for (int l = j; l < j + nk; l++)   // count horizontal zero's
	    if(in[i][l] == 0)
	    {
	       leftsum++;
	       rightsum++;
	       if (leftsum + rightsum > max0 + max0)
		  break;
	    }
	 if (leftsum <= max0)
	    for(int k = i + 1; k < i + nk; k++)  // count left vertical zero's
	    {
	       if(in[k][j] == 0)
	       {
		  leftsum++;
		  if (leftsum > max0)
		     break;
	       }
	    }
	 if (rightsum <= max0)
	    for(int k = i + 1; k < i + nk; k++)  // count right vertical zero's
	    {
	       if(in[k][j + nk] == 0)
	       {
		  rightsum++;
		  if (rightsum > max0)
		     break;
	       }
	    }
	 if (leftsum <= max0)
	    left[i][j] = mk + nk - 1 - leftsum;
	 else
	    left[i][j] = 0;
	 if (rightsum <= max0)
	    right[i][j] = mk + nk - 1 - rightsum;
	 else
	    right[i][j] = 0;
      }
}

// src2tops makes a screenshot and tries to identify the top of the windows
// in the screenshot
// It does this by converting the screenshot to black and white using
// cv::Canny() and subsequent searching for two 'kernels':
// left-top:
// xxxxxxxxxx
// x
// x
// x
// x
// x
// right-top:
// xxxxxxxxxx
//          x
//          x
//          x
//          x
//          x
//          x
//
// The left-kernels are then matched against the right-kernels.

void scr2tops(
      int usescrot,    // whether to use scrot for screenshot or not
      int kernelrows,  // number of rows to be matched
      int kernelcols,  // number of cols to be matched
      int max0,        // number of pixels that can be missed
      int X,           // x-coordinate of sreenshot to consider
      int Y,           // y-coordinate of sreenshot to consider
      int Width,       // width of sreenshot to consider
      int Height,      // height of sreenshot to consider
      int min_y,       // tops with y < min_y are neglected (to ignore a panel, for instance)
      int min_width,   // minimal accepted width of top of window
      int max_width,   // maximal accepted width of top of window
      int removearea,  // remove shadowed tops if closer than removearea pixels from top above
		       // if < 0: do not remove shadowed tops
      tops_t **tops,   // output: x,y,w of top of windows
		       // can be destroyed by free().
      int *ntops       // output: number of tops
      )
{
   //std::cout << "entering scr2tops: " << std::endl;

   const int dilation_size    = 3;   // after Canny, the lines are broadened 
   const int corner           = 2;   // the top left and right corners of the screenshot
				     // are ignored.
   const float scale          = 1;   // scale factor applied to screenshot before searching
				     // for top edges.
   const std::string filename = "screenshot.png"; // scrot will use this filename for screenshot

   cv::Mat fullscreen;

   std::vector<unsigned char> Pixels;
   int Bpp = 0;

#ifdef SRC2TOPS_TESTING
   double t0;
   t0 = wallclock();
#endif
   if(usescrot)
   {
      std::string command = "scrot -o -F " + filename + " -a " + 
	 std::to_string(X) + "," + std::to_string(Y) + "," +
	 std::to_string(Width) + "," + std::to_string(Height);
      //std::cout << "command:" << command << std::endl;
      int rc = system(command.c_str());
      if (rc != 0)
      {
	 I("Cannot run then screenshot program scrot\n");
	 exit(1);
      }
      fullscreen = cv::imread(filename,cv::IMREAD_COLOR);
   }
   else
   {
      P("calling ImageFromDisplay: %d %d %d %d\n",X,Y,Width,Height);
      ImageFromDisplay(Pixels, X, Y, Width, Height, Bpp);

      fullscreen = cv::Mat(Height, Width, Bpp > 24 ? CV_8UC4 : CV_8UC3, &Pixels[0]);
   }
#ifdef SRC2TOPS_TESTING
   double tshot = wallclock() - t0;
   cv::Mat edges;
#endif

   std::vector <tops_t>lin;
   cv::Mat canny;
   cv::Mat img1;
#ifdef SRC2TOPS_TESTING
   std::cout << "fullscreen:" << std::endl;
   cv::imshow("fullscreen",fullscreen);
   cv::waitKey(0);
#endif

   if (scale == 1)
      img1 = fullscreen.clone();
   else
      cv::resize(fullscreen,img1,cv::Size(),scale,scale,cv::INTER_LINEAR);


#ifdef SRC2TOPS_TESTING
   std::cout << "img1:" << std::endl;
   cv::imshow("img1",img1);
   cv::waitKey(0);

   edges = fullscreen.clone();

   t0 = wallclock();
#endif

   cv::Canny(img1,canny,100,200);

#ifdef SRC2TOPS_TESTING
   double tcanny = wallclock() - t0;
#endif

   int dilation_type = 0;
   dilation_type = cv::MORPH_RECT;

#ifdef SRC2TOPS_TESTING
   t0 = wallclock();
#endif
   if(dilation_size > 0)
   {
      cv::Mat element = cv::getStructuringElement( dilation_type,
	    cv::Size( dilation_size , dilation_size ),
	    cv::Point( dilation_size/2, dilation_size/2 ) );
      cv::dilate( canny, canny, element );
      element.release();
   }
#ifdef SRC2TOPS_TESTING
   double tdilation = wallclock() - t0;
   std::cout << "canny:" << std::endl;
   cv::imshow("canny",canny);
   cv::waitKey(0);
   std::cout << __LINE__ << ": rows: "<< canny.rows << " cols: "<<canny.cols << std::endl;

   std::cout << std::endl;
#endif

   int scaled_cols = (int)(kernelcols*scale +0.5);
   int scaled_rows = (int)(kernelrows*scale +0.5);
   int out_width  = canny.cols - scaled_cols + 1;
   int out_height = canny.rows - scaled_rows + 1;

#ifdef SRC2TOPS_TESTING
   std::cout << "outw: " << out_width << " outh: " << out_height << std:: endl;
#endif

   unsigned char **blackandwhite = (unsigned char**)wv_matalloc(sizeof(unsigned char),canny.data,2,canny.rows,canny.cols);
   for (int i = 0; i<canny.rows; i++)
   {
      blackandwhite[i][0]            = 255;
      blackandwhite[i][canny.cols-1] = 255;
   }
   unsigned char **myout_left  = (unsigned char**)wv_matalloc(sizeof(unsigned char),0,2,out_height,out_width);
   unsigned char **myout_right = (unsigned char**)wv_matalloc(sizeof(unsigned char),0,2,out_height,out_width);

#ifdef SRC2TOPS_TESTING
   t0 = wallclock();
#endif
   find_top_edges(blackandwhite, canny.rows, canny.cols, scaled_rows, scaled_cols, 
	 max0, myout_left, myout_right);
#ifdef SRC2TOPS_TESTING
   double tmyconv2d = wallclock() - t0;
#endif

   int minpixels = (scaled_rows + scaled_cols -1) - max0;

#ifdef SRC2TOPS_TESTING
   for (int i=0; i<out_height; i++)
   {
      int ii = scaleinv(i);
      for (int j=0; j<out_width; j++)
      {
	 int jj = scaleinv(j);
	 unsigned char item1 = myout_left[i][j];

	 if (item1 > minpixels && !(i < corner && j < corner))
	 {
	    //std::cout << " leftfound: " << __LINE__ << ": "<<ii << " " << jj << " " << (int)item1 << std::endl;
	    cv::line(edges,cv::Point(jj,ii),cv::Point(jj+kernelcols,ii),cv::Scalar(0,0,255),1,cv::LINE_AA);
	    cv::line(edges,cv::Point(jj,ii),cv::Point(jj,ii+kernelrows),cv::Scalar(0,0,255),1,cv::LINE_AA);
	 }

      }
   }
   std::cout<< "----"<<std::endl;

   for (int i=0; i<out_height; i++)
   {
      int ii = scaleinv(i);
      for (int j=0; j<out_width; j++)
      {
	 int jj = scaleinv(j);
	 unsigned char item = myout_right[i][j];

	 if (item > minpixels && !(i < corner && j > canny.cols-kernelcols - corner))
	 {
	    //std::cout << "rightfound: " << __LINE__ << ": " <<ii << " " << jj+kernelcols << " " << (int)item << std::endl;
	    cv::line(edges,cv::Point(jj,ii),cv::Point(jj+kernelcols-1,ii),cv::Scalar(0,255,0),1,cv::LINE_AA);
	    cv::line(edges,cv::Point(jj+kernelcols-1,ii),cv::Point(jj+kernelcols-1,ii+kernelrows),cv::Scalar(0,255,0),1,cv::LINE_AA);
	 }

      }
   }

   std::cout << "edges:" << std::endl;
   cv::imshow("edges",edges);
   cv::waitKey(0);
#endif

   cv::Mat finale = fullscreen.clone();

   const int search_size = dilation_size;
   // scan left for hits > minpixels
#ifdef SRC2TOPS_TESTING
   std::cout << "scanning left and finding right" << std::endl;
   t0 = wallclock();
#endif
   for (int i=min_y; i<out_height; i++)
   {
      for (int j=0; j<out_width; j++)
      {
	 float item = myout_left[i][j];
	 if (item > minpixels && !(i < corner && j < corner))
	 {
	    int imax = i;
	    int jmax = j;
	    float max_item_left = 0;
	    // see if there are higher scores in the neighbourhood
	    for (int k=i; k<i+search_size; k++)
	    {
	       if (k > out_height-1)
		  break;
	       for (int l=j; l<j+search_size; l++)
	       {
		  if (l > out_width -1)
		     break;
		  assert(k>= 0); assert(k<out_height); assert(l >=0); assert(l <out_width);
		  if (myout_left[k][l] > max_item_left)
		  {
		     imax = k;
		     jmax = l;
		     max_item_left = myout_left[k][l];
		     //std::cout << "i: " << i << " j: " << j <<" imax: " << imax << " jmax: " << jmax << " value: " << max_item_left << std::endl;
		  }

	       }
	    }

	    // search to the right if there is an hit at the same row
	    int j_right = 0;
	    // search for end of line where the left top is situated
	    // search from right to the left if there is a suitable right top corner
	    for (int k=out_width-1; k>jmax; k--)
	       if (myout_right[imax][k] > minpixels)
	       {
		  j_right = k;
#ifdef SRC2TOPS_TESTING
		  // we have a hit, paint this in finale
		  // left:
		  int ii = scaleinv(imax);
		  int jj = scaleinv(jmax);
		  //std::cout << "left  " << ii << " " << jj << " " << item << std::endl;
		  cv::line(finale,cv::Point(jj,ii),cv::Point(jj+kernelcols,ii),cv::Scalar(0,0,255),1,cv::LINE_AA);
		  cv::line(finale,cv::Point(jj,ii),cv::Point(jj,ii+kernelrows),cv::Scalar(0,0,255),1,cv::LINE_AA);
		  // right:
		  jj = scaleinv(k);
		  //std::cout << "right " <<ii << " " << jj+kernelcols << " " << item << std::endl;
		  cv::line(finale,cv::Point(jj,ii),cv::Point(jj+kernelcols-1,ii),cv::Scalar(0,255,0),1,cv::LINE_AA);
		  cv::line(finale,cv::Point(jj+kernelcols-1,ii),cv::Point(jj+kernelcols-1,ii+kernelrows),cv::Scalar(0,255,0),1,cv::LINE_AA);
#endif
		  int y = scaleinv(imax);
		  // Now scan this line for contiguous one's in black and white
		  // Every block of ones is denoted as a top
		  int prev = blackandwhite[imax][jmax];
		  int xstart = jmax;
		  int lmax = j_right+kernelcols;
		  for (int l = jmax+1; l < lmax; l++)
		  {
		     if(prev)
		     {
			if (blackandwhite[imax][l] && l != lmax-1)
			   continue;
			else
			{
			   int x = scaleinv(xstart);
			   int w = scaleinv(l-1) - x;
			   if(w >= min_width && w<=max_width)
			      lin.push_back({x,y,w}); 
			   prev = 0; 
			   continue;
			}
		     }
		     else
		     {
			if (blackandwhite[imax][l])
			{
			   xstart = l;
			   prev = 1;
			   continue;
			}
			else
			{
			   continue;
			}
		     }
		  }

		  for (int k=i; k<i+search_size; k++)
		  {
		     if (k > out_height-1)
			break;
		     int lmin = j - search_size;
		     if (lmin < 0)
			lmin = 0;
		     lmax = j_right + kernelcols;
		     if (lmax > out_width)
			lmax = out_width;
		     for (int l = lmin; l<lmax; l++)
		     {
			assert(k>= 0); assert(k<out_height); assert(l >=0); assert(l <out_width);
			myout_left[k][l] = 0;
		     }
		  }
		  break;
	       }

	 }
      }
   }
   std::vector <tops_t>finalpos;
   std::vector<int>valid;
   for(unsigned int i=0; i<lin.size(); i++)
      valid.push_back(1);
   if(removearea > 0)
      for (unsigned int i=0; i<lin.size(); i++)
      {
	 for(unsigned int j = 0; j<lin.size(); j++)
	 {
	    // see if j shadows i
	    if (  i != j &&
		  valid[j] &&
		  lin[i].y >  lin[j].y  &&                        // i below j
		  lin[i].y -  lin[j].y < removearea &&            // i within range
		  lin[i].x >= lin[j].x - 1 &&                     // start[i] >  start[j]
		  lin[i].x <= lin[j].x + lin[j].w &&              // start[i] <= end[j]
		  lin[i].x + lin[i].w <= lin[j].x + lin[j].w + 1  // end[i]   <= end[j]
	       )
	    {
	       valid[i] = 0;
	       break;
	    }
	 }
      }
   for (unsigned int i =0; i < lin.size(); i++)
      if(valid[i])
	 finalpos.push_back(lin[i]);

#ifdef SRC2TOPS_TESTING
   double tscan = wallclock() - t0;

   std::cout << "finale:" << std::endl;
   cv::imshow("finale",finale);
   cv::waitKey(0);
#endif

   *ntops = finalpos.size();
   *tops = (tops_t *)malloc(sizeof(tops_t)*(*ntops));
   for (unsigned int i = 0; i<finalpos.size(); i++)
      (*tops)[i] = finalpos[i];

#ifdef SRC2TOPS_TESTING
   cv::Mat ultimate = fullscreen.clone();
   for (unsigned int i=0; i<finalpos.size(); i++)
   {
      int x = finalpos[i].x;
      int y = finalpos[i].y;
      int w = finalpos[i].w;
      std::cout << i << " x: " << x << " y: " << y << " w: " << w << std::endl;
      cv::line(ultimate,cv::Point(x,y),cv::Point(x+w,y),cv::Scalar(0,0,255),3,cv::LINE_AA);
   }
   std::cout << "ultimate:" << std::endl;
   cv::imshow("ultimate",ultimate);
   cv::waitKey(0);

   std::cout << "shot time:         " << tshot     << std::endl;
   std::cout << "findtopedges time: " << tmyconv2d << std::endl;
   std::cout << "canny time:        " << tcanny    << std::endl;
   std::cout << "dilation time:     " << tdilation << std::endl;
   std::cout << "scan time:         " << tscan     << std::endl;
   std::cout << "total time:        " << tshot+tmyconv2d+tcanny+tdilation+tscan << std::endl;
#endif
   wv_matfree(myout_left);
   wv_matfree(myout_right);
   wv_matfree(blackandwhite);
   canny.release();
   img1.release();
   finale.release();
   fullscreen.release();
#ifdef SRC2TOPS_TESTING
   edges.release();
   ultimate.release();
#endif
}

