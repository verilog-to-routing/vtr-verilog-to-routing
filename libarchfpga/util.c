#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <memory>

#include "vtr_util.h"
#include "vtr_log.h"

/* This file contains utility functions widely used in *
 * my programs.  Many are simply versions of file and  *
 * memory grabbing routines that take the same         *
 * arguments as the standard library ones, but exit    *
 * the program if they find an error condition.        */



/* Returns the min of cur and max. If cur > max, a warning
 * is emitted. */
int limit_value(int cur, int max, const char *name) {
	if (cur > max) {
        vtr::printf_warning(__FILE__, __LINE__,
				"%s is being limited from [%d] to [%d]\n", name, cur, max);
		return max;
	}
	return cur;
}





