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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_UNORDERED_MAP
#include <unordered_map>
#else
#include <map>
#endif
#include "hashtable.h"
#include "debug.h"

#ifdef HAVE_UNORDERED_MAP
static std::unordered_map<unsigned int,void*> table(500); 
#else
static std::map<unsigned int,void*> table; // ok, this is a binary tree
#endif

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
extern "C" void table_clear(void(*destroy)(void *p))
{
   for ( auto it = table.begin(); it != table.end(); ++it )
   {
      P("%d %p\n",it->first,it->second);
      destroy(it->second);
      it->second = 0;
   }
}
