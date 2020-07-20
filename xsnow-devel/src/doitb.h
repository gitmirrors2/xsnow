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
#ifndef DOITB_H
#define DOITB_H

#define DOITALLB()  
   //DOITB(prefdistance ,float) /* [meter]          preferred distance */                    

   //DOITB(nbirds       ,int)   /* number of birds */ 
   //DOITB(neighbours   ,int)   /* number of neighbours to consider */ 
   //DOITB(followers    ,float) /* fraction of birds that follow the computations 0=<x<=1 */
   
#define BUTTONALL() \
   DOITB(restart) \
   DOITB(freeze)

#endif  /* DOIT_H */
