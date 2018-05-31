#include "soft_logic_def_parser.h"
#include "types.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <vector>

/*
file.txt =
{
<operation>,<target_bit_width>,<enum_name>,<blk_len_of_structure>
<operation>,<target_bit_width>,<enum_name>,<blk_len_of_structure>
<operation>,<target_bit_width>,<enum_name>,<blk_len_of_structure>
...
}

operation
 	"+"
	"x"
	"/"
	"%"
	"^"
	">>"
	"<<"
	"<<<"
	">>>"
	"++"
	"--"

target_bit_width
	integer

enum_name
	user_defined in the enum!

blk_len_of_structure
	the number of blks to construct this way

*/
std::vector<adder_def_t*> list_of_adder_def;

void read_soft_def_file(const char *input_file_name)
{
	adder_def_t *out = (adder_def_t*)malloc(sizeof(adder_def_t));
	out->next_step_size =0;
	out->type_of_adder = rca;

	FILE *input_file = fopen(input_file_name,"r");
	char line_buf[128];

	while (fgets(line_buf, 128, input_file) != NULL)
	{
		std::string type(strtok(line_buf,","));
		if(type == "rca")			out->type_of_adder = rca;
		else if (type == "csla")		out->type_of_adder = csla;
		else if (type == "bec_csla")		out->type_of_adder = bec_csla;

		out->next_step_size = strtol(strtok(NULL,","),NULL,10);
		list_of_adder_def.push_back(out);
	}
	fclose(input_file);
}
