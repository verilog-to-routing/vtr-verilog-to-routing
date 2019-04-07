#include "odin_error.h"
#include "config_t.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h> 
#include <stdlib.h>

const char *odin_error_STR[] =
{
	"NO_ERROR",
	"ARG_ERROR",
	"PARSE_ERROR",
	"NETLIST_ERROR",
	"BLIF_ERROR",
	"NETLIST_FILE_ERROR",
	"ACTIVATION_ERROR",
	"SIMULATION_ERROR",
	"ACE",
};

static void print_file_name(long file)
{
	if (file >= 0 && file < configuration.list_of_file_names.size())
		fprintf(stderr," (File: %s)", configuration.list_of_file_names[file].c_str());
}

static void print_line_number(long line_number)
{
	if (line_number >= 0)
		fprintf(stderr," (Line number: %ld)", line_number+1);
}

static void print_culprit_line(long line_number, long file)
{
	if (file >= 0 && file < configuration.list_of_file_names.size()
	&& line_number >= 0)
	{
		FILE *input_file = fopen(configuration.list_of_file_names[file].c_str(), "r");
		if(input_file)
		{
			fprintf(stderr,"\t");
		
			bool copy_characters = false;
			int current_line_number = 0;

			for (;;) {
				int c = fgetc(input_file);
				if (EOF == c) {
					break;
				} else if ('\n' == c) {
					++current_line_number;
					if (line_number == current_line_number) {
						copy_characters = true;
					} else if (copy_characters) {
						break;
					}
				} else if (copy_characters) {
					fprintf(stderr,"%c",c);
				}
			}
			fprintf(stderr,"\n");
			fclose(input_file);
		}
	}
}

void _log_message(odin_error error_type, long line_number, long file, bool soft_error, const char *function_file_name, long function_line, const char *function_name, const char *message, ...)
{
	va_list ap;

	static long warning_count = 0;

	if (!warning_count) 
	{
		fprintf(stderr,"--------------\nOdin has decided you ");
		if(soft_error)
			fprintf(stderr,"MAY fail ... :\n\n");
		else
			fprintf(stderr,"have failed ;)\n\n");
	}
	warning_count++;


	if(soft_error)
		fprintf(stderr,"WARNING ");		
	else
		fprintf(stderr,"ERROR ");

	fprintf(stderr,"(%ld):%s",warning_count, odin_error_STR[error_type]);

	print_file_name(file);
	print_line_number(line_number);
		
	if (message != NULL)
	{
		fprintf(stderr," ");
		va_start(ap, message);
		vfprintf(stderr,message, ap);
		va_end(ap);
	}

	if (message[strlen(message)-1] != '\n') 
		fprintf(stderr,"\n");

	print_culprit_line(line_number, file);

	fflush(stderr);
	_verbose_assert(soft_error,"", function_file_name, function_line, function_name);
}