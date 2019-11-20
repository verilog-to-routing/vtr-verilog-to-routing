//////////////////////////////////////////////////////////////////////////
// This file is used to detect memory leaks using Visual Studio 6.0
// The idea comes from this page: http://www.michaelmoser.org/memory.htm
// In addition to this file, it required the presence of "stdlib_hack.h"
//////////////////////////////////////////////////////////////////////////

#ifndef __LEAKS_H__
#define __LEAKS_H__

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC // include Microsoft memory leak detection procedures
//#define _INC_MALLOC	     // exclude standard memory alloc procedures

#define   malloc(s)         _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   calloc(c, s)      _calloc_dbg(c, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   realloc(p, s)     _realloc_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
//#define   _expand(p, s)     _expand_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
//#define   free(p)           _free_dbg(p, _NORMAL_BLOCK)
//#define   _msize(p)         _msize_dbg(p, _NORMAL_BLOCK)

//#include <stdlib.h>
#include <stdlib_hack.h>
#include <crtdbg.h>
#endif

#endif

//////////////////////////////////////


