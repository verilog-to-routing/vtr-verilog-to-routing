/*
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
#include "soft_logic_def_parser.h"
#include "types.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#include "vtr_memory.h"


/*
file.txt =
{
<operation>,<target_bit_width>,<enum_name>,<blk_len_of_structure>
<operation>,<target_bit_width>,<enum_name>,<blk_len_of_structure>
<operation>,<target_bit_width>,<enum_name>,<blk_len_of_structure>
...
}

operation
	{ + , /, %, <<}

target_bit_width
	integer

enum_name
	user_defined in the switch case of the op!

blk_len_of_structure
	the size of the blks to construct this way

*/
std::map<std::string,soft_sub_structure*> soft_def_map;

void read_soft_def_file(const char *input_file_name)
{
	FILE *input_file = fopen(input_file_name,"r");
  if(input_file)
  {
		soft_def_map[std::string("+_0")] = NULL;
		soft_def_map[std::string("/_0")] = NULL;
		soft_def_map[std::string("%_0")] = NULL;
		soft_def_map[std::string("<<_0")] = NULL;

		char line_buf[640];
		while (fgets(line_buf, 640, input_file) != NULL)
		{
			char *point_to = line_buf;
			char *tokens[5];
			int i=0;
			while(i < 5 && (tokens[i++] = strtok(point_to,",")) != NULL)
				point_to = NULL;

			if(i != 5)
				continue;

			std::string operation_name(tokens[0]);
			int operation_bitsize				=	strtol(tokens[1],NULL,10);
			std::string soft_hard(tokens[2]);
			std::string sub_structure_name(tokens[3]);
			int sub_structure_bitsize		=	strtol(tokens[4],NULL,10);

			std::string lookup = operation_name + "_0";

			auto candidate = soft_def_map.find(lookup);
			if(candidate == soft_def_map.end() \
			|| operation_bitsize < 1 \
		 	|| operation_bitsize < sub_structure_bitsize \
			|| (soft_hard != "hard" && soft_hard != "soft")
			){
				continue;
			}

			std::string key_map = operation_name + "_" + tokens[1];
			soft_sub_structure* def = (soft_sub_structure*)vtr::malloc(sizeof(soft_sub_structure));
			def->type = strdup(tokens[2]);
			def->name = strdup(tokens[3]);
			def->bitsize = sub_structure_bitsize;

			soft_def_map[key_map] = def;
		}
		fclose(input_file);
  }
}

/*---------------------------------------------------------------------------------------------
 * gets the soft blk size at a certain point in the adder using a define file input
 *-------------------------------------------------------------------------------------------*/
soft_sub_structure *fetch_blk(std::string op, int width)
{
	std::string lookup = op + "_" + std::to_string(width);

	auto candidate = soft_def_map.find(lookup);
	if(candidate == soft_def_map.end())
		return NULL;

	return soft_def_map[lookup];
}
