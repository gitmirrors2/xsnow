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
#include <stdarg.h>
#include <stdlib.h>
#include "wv_matalloc.h"
/*
   void *wv_matalloc (size_t size, void *data, unsigned int ndim, ...)
   void *wv_matallocv (size_t size, void *data, unsigned int ndim, unsigned int*dims)

   allocates memory and builds pointerstructure for
   ndim dimensional matrix. 

Note: wv_matalloc is thread safe I think. 

size: size in bytes of matrix elements to be allocated
data: if (void *)0: wv_matalloc allocates memory
else it is assumed data is already allocated
ndim: number of dimensions of matrix to allocate
... : dimensions of the matrix to be allocated of type unsigned int

dims: dimensions of matrix to be allocated

return value: pointer to pointerstructure to access arrayelements
(void *)0 if something wrong with parameters or
if malloc doesn't work

example:

allocate 2-dimensional matrix, elements of type double,
dimensions M and N:

double **a;
a=wv_matalloc(sizeof(double),(void *)0,2,M,N);

usage of a is then like: a[i][j] = 3;

allocate 4-dimensional matrix, elements of type int,
dimensions m,n,p,q:

int ****a;
m=8; n=123; p=9; q=11;
a=wv_matalloc(sizeof(int),(void *)0,4,m,n,p,q);

a[i][j][k][l] = 12;

3-dimensional matrix, elements of type double,
dimensions l,m,n. Space for data is already allocated,
so in fact, only the pointer structure is set up:

double *a;
double ***x;

a=(double *) malloc(sizeof(double)*l*m*n);  

a allocated outside wv_matalloc 

x=wv_matalloc(sizeof(double),a,3,l,m,n);

x[0][0][0] is the same element as a[0] 

void wv_matfree(void *p)

Frees memory occupied by pointers and perhaps data allocated by
wv_matalloc. Only the space allocated by wv_matalloc is freed.

p:  returnvalue of wv_matalloc

example

char ***a;
a=wv_matalloc(sizeof(char),(void *)0,3,10,20,15);

wv_matfree(a);
*/

static void * wv_matalloc_1(unsigned int i,unsigned int size,unsigned int nd,
      unsigned int *d,void ***pp,char **q);

void *wv_matallocv(size_t size, void *data, unsigned int ndim, unsigned int *d)
{
   int i;
   int npad=64;
   size_t s;
   void *r;
   unsigned int pspace;
   void **p;         /* points to p-array: array of pointers */
   unsigned int np;      /* number of elements in p-array */
   unsigned int nq;      /* number of bytes in q-array */
   void **pp;        /* points to first not allocated element of p-array */
   char *q;         /* points to q-array: array of characters */

   if (ndim <=0)
      return (void *)0;

   if (ndim >=2)
   {
      np = d[ndim-2];
      for (i=ndim-3; i>=0; i--)
	 np = d[i]*(1+np);
   }
   else
      np=0;

   nq=1;
   for (i=0; i<(int)ndim; i++)
      nq *= d[i];

   /* calculate length of p-array
      round it up to multiple of npad bytes if no data is given*/

   if (data)
      s=np*sizeof(void*);
   else
      s=((np * sizeof(void*) - 1)/npad + 1)*npad;

   /* allocate in one step the space for p-array and q-array */
   /* don't allocate space for q-array if data is given */

   pspace = s;
   if (!data)
      pspace = s+nq*size;
   p = pp = (void**)malloc(pspace);

   if ( p == 0)
   {
      free(d);
      return (void *)0;
   }

   /* start of q-array after p-array if data not given */

   if (data)
      q = (char*)data;
   else
      q = (char *)p + s;

   r= wv_matalloc_1(0,size,ndim,d,&pp,&q);
   return r;
}

void *wv_matalloc(size_t size, void *data, unsigned int ndim, ...)
{
   va_list argp;
   int i;
   void *r;
   unsigned int *d;   /* array to hold dimensions */

   if (ndim <=0)
      return (void *)0;


   d=(unsigned int*)malloc(ndim*sizeof(int));
   if (d == 0)
      return (void *)0;

   va_start(argp, ndim);
   for (i=0; i<(int)ndim; i++)
   {
      d[i]  = va_arg(argp, unsigned int);
   }
   va_end(argp);

   r = wv_matallocv(size, data, ndim, d);
   free(d);
   return r;
}

void *wv_matalloc_1(unsigned int ld,unsigned int size,unsigned int nd,
      unsigned int *d,void ***p_pp, char**p_q)
{
   unsigned int i;
   void **pr, **pl;
   char *ql;

   if (ld == nd - 1 )
      /* simple: at this point we return a pointer to
       * a space of d[ld]*size in the q-array
       */
   {
      ql =  *p_q;
      *p_q += size * d[ld];
      return (void *)ql;
   }
   /* This is more interesting: 
    * get d[ld] pointers from the p-array
    * and assign the values of wv_matalloc-1(ld+1) to them */

   pr = pl = *p_pp;
   *p_pp  += d[ld];

   for (i=0; i < d[ld] ; i++)
   {
      *pl = wv_matalloc_1(ld+1,size,nd,d,p_pp,p_q);
      pl++;
   }

   return pr;
}    

void wv_matfree(void *p)
{
   free(p);
}
