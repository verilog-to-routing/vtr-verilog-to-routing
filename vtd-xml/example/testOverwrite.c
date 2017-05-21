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
#include <wchar.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../xpath.h"
#include "../helper.h"
#include "../vtdGen.h"
// This example shows you how to overwrite the content of
// a token using VTDGen's overWrite function
// since 2.10 declare exception context as thread local storage
_thread struct exception_context the_exception_context[1];

int main(){

	exception e;
	int i = 0,t,result,len,count=0;
	UCSChar *tmpStr = NULL;
	UByte* xml = "<root>good</root>";
	VTDGen *vg = NULL;
	VTDNav *vn = NULL;
	Try{
		vg = createVTDGen();
		setDoc(vg,xml,strlen(xml));
		parse(vg,TRUE);
		vn = getNav(vg);
		i = getText(vn);
		tmpStr = toString(vn,i);
		wprintf(L" text value is %s\n",tmpStr);
		free(tmpStr);
		len=strlen("bad");
		overWrite(vn,i,"bad",0,len);
		tmpStr = toString(vg,i);
		wprintf(L" text value is %s\n",tmpStr);
		free(tmpStr);
		// remember C has no automatic garbage collector
		// needs to deallocate manually.
		freeVTDNav(vn);
		freeVTDGen(vg);
	}
	Catch (e) {
		// manual garbage collection here
		freeVTDGen(vg);
	}
  return 0;
}	

