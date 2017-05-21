/* 
 * Copyright (C) 2002-2007 XimpleWare, info@ximpleware.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "../vtdGen.h"
//#include "vtdNav.h"
#include "../autoPilot.h"
// since 2.10 declare exception context as thread local storage
_thread struct exception_context the_exception_context[1];

/* this is a simple benchmark file */
/* after make, type benchmark_gcc  */
/* <filename> to benchmark its performance */

int main(int argc, char *argv[])
{ 
	int j=0,k=0,j1=0;
	//int t;
	clock_t c1,c2;
	//printf("size of clock_t: %d \n",sizeof(clock_t));
	exception e;
	//UByte a = -1;
	//int i = a;
	//struct _stat fs;
	FILE *f = NULL;
	UByte *xml = NULL;
	VTDGen *vg = NULL;
	VTDNav *vn = NULL;
	struct stat s;
		
	f = fopen(argv[1],"r");
	stat(argv[1],&s);
	//i=(int)_filelength(f->_file);
	printf("size of the file is %d \n",(int)s.st_size);
	xml = (UByte *)malloc(sizeof(UByte)*(int)s.st_size); 
	k=(int)fread(xml,sizeof(UByte),s.st_size,f);
	j1 = (int) (800000000/k);
	printf("j1 : %d \n",j1);
	printf(" read in %d bytes \n",k);
	c1 = clock();
	vg = createVTDGen();
	
	Try{
		for (j=0;j<j1;j++){
		setDoc_BR2(vg,xml,s.st_size,0,k);
		parse(vg,TRUE);
		//vn = getNav(vg);
		//freeVTDGen(vg);
		//freeVTDNav(vn);
		}
	}
	Catch (e){
		if (e.et == parse_exception)
		printf("exception e ==> %s \n %s\n", e.msg,e.sub_msg);
	}
	
	c2 = clock();
	printf("mili-sec elapsed : %d \n",(int)(((int)c2-(int)c1)/CLOCKS_PER_SEC*1000));
	printf("average latency : %f ms \n",(double)(c2-c1)/CLOCKS_PER_SEC*1000/j1);
	printf("performance : %f MB/sec \n", 
		((double)k*j*CLOCKS_PER_SEC/((double)(1<<20)*(c2-c1)))
	);
	return 1;	
	
}
