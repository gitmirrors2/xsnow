/* -copyright-
#-# 
#-# xsnow: let it snow on your desktop
#-# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#-# 	      2019,2020,2021 Willem Vermin
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
#include "selfrep.h"
#include "utils.h"
static unsigned char tarfile[] = {
#include "tarfile.inc"
};
void selfrep()
{
#ifdef SELFREP
   if(sizeof(tarfile) > 1000 && isatty(fileno(stdout)))
   {
      printf("Not sending tar file to terminal.\n");
      printf("Try redirecting to a file (e.g: xsnow -selfrep > xsnow.tar.gz),\n");
      printf("or use a pipe (e.g: xsnow -selfrep | tar zxf -).\n"); 
   }
   else
   {
      ssize_t rc = mywrite(fileno(stdout),tarfile,sizeof(tarfile));
      if (rc < 0)
	 fprintf(stderr,"Xsnow: Problems encountered during production of the tar ball.\n");
   }
#else
   // Since the -selfrep flag is not recognized in this case,
   // the following is somewhat superfluous:
   printf("Self replication is not compiled in.\n");
#endif
}
