/***************************************************************************************[System.cc]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include "sat/glucose2/System.h"

#if defined(__linux__)

#include <stdio.h>
#include <stdlib.h>

ABC_NAMESPACE_IMPL_START

using namespace Gluco2;

// TODO: split the memory reading functions into two: one for reading high-watermark of RSS, and
// one for reading the current virtual memory size.

static inline int memReadStat(int field)
{
    char  name[256];
    pid_t pid = getpid();
    int   value;

    sprintf(name, "/proc/%d/statm", pid);
    FILE* in = fopen(name, "rb");
    if (in == NULL) return 0;

    for (; field >= 0; field--)
        if (fscanf(in, "%d", &value) != 1)
            printf("ERROR! Failed to parse memory statistics from \"/proc\".\n"), exit(1);
    fclose(in);
    return value;
}


static inline int memReadPeak(void)
{
    char  name[256];
    pid_t pid = getpid();

    sprintf(name, "/proc/%d/status", pid);
    FILE* in = fopen(name, "rb");
    if (in == NULL) return 0;

    // Find the correct line, beginning with "VmPeak:":
    int peak_kb = 0;
    while (!feof(in) && fscanf(in, "VmPeak: %d kB", &peak_kb) != 1)
        while (!feof(in) && fgetc(in) != '\n')
            ;
    fclose(in);

    return peak_kb;
}

double Gluco2::memUsed() { return (double)memReadStat(0) * (double)getpagesize() / (1024*1024); }
double Gluco2::memUsedPeak() { 
    double peak = memReadPeak() / 1024;
    return peak == 0 ? memUsed() : peak; }

ABC_NAMESPACE_IMPL_END

#elif defined(__FreeBSD__)

ABC_NAMESPACE_IMPL_START

using namespace Gluco2;

double Gluco2::memUsed(void) {
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return (double)ru.ru_maxrss / 1024; }
double Gluco2::memUsedPeak(void) { return memUsed(); }

ABC_NAMESPACE_IMPL_END

#elif defined(__APPLE__)

#include <malloc/malloc.h>

ABC_NAMESPACE_IMPL_START

double Gluco2::memUsed(void) {
    malloc_statistics_t t;
    malloc_zone_statistics(NULL, &t);
    return (double)t.max_size_in_use / (1024*1024); }

ABC_NAMESPACE_IMPL_END

#else

ABC_NAMESPACE_IMPL_START

double Gluco2::memUsed() { 
    return 0; }

ABC_NAMESPACE_IMPL_END

#endif

