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
#include <stdlib.h>
#include <math.h>
#include "gaussian.h"
// https://www.alanzucconi.com/2015/09/16/how-to-sample-from-a-gaussian-distribution/

double gaussian (double mean, double std, double min, double max) 
{
   double x;
   do {
      double v1, v2, s;
      do {
	 v1 = 2.0 * drand48() - 1.0;
	 v2 = 2.0 * drand48() - 1.0;
	 s = v1 * v1 + v2 * v2;
      } while (s >= 1.0 || s == 0);
      x = mean + v1 * sqrt((-2.0 * log(s)) / s) * std;
   } while (x < min || x > max);
   return x;
}

void sgaussian(long int seed)
{
   srand48(seed);
}
