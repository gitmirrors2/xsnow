/* 
 -copyright-
# xsnow: let it snow on your desktop
# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#              2019,2020,2021,2022,2023,2024 Willem Vermin
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

// A stripped-down version of xsnow: only capable to output the man page
// To be compiled like: cc -DMAKEMAN makeman.c docs.c flags.c
// macro MAKEMAN is used in     flags.c flags.h
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "docs.h"
#include "flags.h"

int main()
{
   InitFlags();
   SetDefaultFlags();
   docs_usage(1);
}
