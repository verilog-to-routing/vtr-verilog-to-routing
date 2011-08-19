// vqm_dll.cpp : Defines the entry point for the DLL application.
//
// This file contains a function that calls the VQM parser, parses a specified
// VQM file and returns a structure that represents a logic circuit described
// in the VQM file.
//
// Author:	Tomasz Czajkowski
// Date:	February 28, 2005
//
// NOTES/REVISIONS:
/////////////////////////////////////////////////////////////////


#include <assert.h>
#include <string.h>


extern "C" 
{
	#include "vqm_parser.tab.h"
}
#include "vqm_dll.h"
#include "vqm_common.h"

/*Bool APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}*/


VQM_DLL_API t_module *vqm_parse_file(char *filename)
/* Parse a VQM file and return the module contained within it. Return NULL on failure. */
{
	t_module	*my_module = NULL;

	assert(filename != NULL);

	yyin = fopen(filename,"r");
	if (yyin != NULL)
	{
		yyparse();
		if (module_list != NULL)
		{
			my_module = (t_module *) module_list->pointer[0];
		}
		yyrestart(yyin);
		fclose(yyin);
	} else {
		printf("ERROR: Could not open %s.\n", filename);
		exit(1);
	}

	return my_module;
}

VQM_DLL_API int vqm_get_error_message(char *message_buffer, int length)
{
	int result = -1;
	int temp = strlen(most_recent_error);

	if (temp <= length)
	{
		strcpy(message_buffer, message_buffer);
		result = temp;
	}
	return result;
}
