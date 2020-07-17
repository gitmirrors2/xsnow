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
//http://www.cplusplus.com/reference/unordered_map/unordered_map/insert/
#include <unordered_map>
#include "hashtable.h"

static std::unordered_map<unsigned int,void*> table; 

extern "C" void table_put(unsigned int key,void *value)
{
   table[key] = value;
}
extern "C" void *table_get(unsigned int key)
{
   void *v;
   try
   {
      v = table[key];
   }
   catch(...)
   {
      v = 0;
   }
   return v;
}
#if 0
#include <iostream>
#include <string>
int main()
{
   std::cout << "max size " << table.max_size() << std::endl;
   int i,j;
   void *v;
   v = &i;
   table_put(1,v);
   std::cout << v << std::endl;
   std::cout << table_get(1) << std::endl;
   std::cout << table_get(2) << std::endl;
   table_put(13,&j);
   std::cout << &j << std::endl;
   std::cout << table_get(13) << std::endl;
}
#endif
