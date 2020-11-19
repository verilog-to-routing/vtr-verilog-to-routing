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
#include <stdint.h> // For uintptr_t in vqm_parser.gen.h


#include "vqm_dll.h"
#include "vqm_parser.gen.h"
#include "vqm_common.h"

//Some versions of bison (e.g. v2.5) to do not prototype yyparse()
//in the parser.gen.h, while others do (e.g. v3.0.2)
//extern int yyparse (t_parse_info* parse_info);


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

    //Initialize parse counting structure
    t_parse_info* parse_info = (t_parse_info*) malloc(sizeof(t_parse_info)); 
    parse_info->pass_type = COUNT_PASS;
    parse_info->number_of_pins = 0;
    parse_info->number_of_assignments = 0;
    parse_info->number_of_nodes = 0;
    parse_info->number_of_modules = 0;

	yyin = fopen(filename,"r");
	if (yyin != NULL)
	{

        //Initial pass to count items
        printf("\tCounting Pass\n");
		yyparse(parse_info);
		if (module_list != NULL)
		{
			my_module = (t_module *) module_list->pointer[0];
		}


		fclose(yyin);
	} else {
		printf("ERROR: Could not open %s for counting pass.\n", filename);
		exit(1);
	}

    //Next pass type
    parse_info->pass_type = ALLOCATE_PASS;

    //Counting statistics
    printf("\tVQM Contains:\n");
    printf("\t\t%d pin/net(s)\n", parse_info->number_of_pins);
    printf("\t\t%d assignment(s)\n", parse_info->number_of_assignments);
    printf("\t\t%d node(s)\n", parse_info->number_of_nodes);
    printf("\t\t%d module(s)\n", parse_info->number_of_modules);
	
    yyin = fopen(filename,"r");
	if (yyin != NULL)
	{

        //Initial pass to count items
        printf("\tAllocating Pass\n");
		yyparse(parse_info);
		if (module_list != NULL)
		{
			my_module = (t_module *) module_list->pointer[0];
		}


		fclose(yyin);
	} else {
		printf("ERROR: Could not open %s for allocating pass.\n", filename);
		exit(1);
	}

    //Cleanup
    free(parse_info);
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
