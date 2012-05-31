/*
Copyright (c) 2009 Peter Andrew Jamieson (jamieson.peter@gmail.com)

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/ 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "types.h"
#include "globals.h"
#include "util.h"

/*---------------------------------------------------------------------------------------------
 * (function: error_message)
 *-------------------------------------------------------------------------------------------*/
void error_message(short error_type, int line_number, int file, char *message, ...)
{
	va_list ap;

	fprintf(stderr,"--------------\nOdin has decided you have failed ;)\n\n");

	fprintf(stderr,"ERROR:");
	if (file != -1)
		fprintf(stderr," (File: %s)", configuration.list_of_file_names[file]);
	if (line_number > 0)
		fprintf(stderr," (Line number: %d)", line_number);
	if (message != NULL)
	{
		fprintf(stderr," ");
		va_start(ap, message);
		vfprintf(stderr,message, ap);	
		va_end(ap); 
	}

	if (message[strlen(message)-1] != '\n') fprintf(stderr,"\n");

	exit(error_type);
}

/*---------------------------------------------------------------------------------------------
 * (function: warning_message)
 *-------------------------------------------------------------------------------------------*/
void warning_message(short error_type, int line_number, int file, char *message, ...)
{
	va_list ap;
	static short is_warned = FALSE;
	static long warning_count = 0;
	warning_count++;

	if (is_warned == FALSE) {
		fprintf(stderr,"-------------------------\nOdin has decided you may fail ... WARNINGS:\n\n");
		is_warned = TRUE;
	}

	fprintf(stderr,"WARNING (%ld):", warning_count);
	if (file != -1)
		fprintf(stderr," (File: %s)", configuration.list_of_file_names[file]);
	if (line_number > 0)
		fprintf(stderr," (Line number: %d)", line_number);
	if (message != NULL) {
		fprintf(stderr," ");

		va_start(ap, message);
		vfprintf(stderr,message, ap);	
		va_end(ap); 
	}

	if (message[strlen(message)-1] != '\n') fprintf(stderr,"\n");
}
