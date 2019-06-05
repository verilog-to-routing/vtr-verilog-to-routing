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
#include "odin_types.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#include "vtr_util.h"
#include "vtr_memory.h"
#include "vtr_path.h"

#include <algorithm>
#include <string>


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

void read_soft_def_file(t_model *hard_adder_models)
{
	std::string soft_distribution(global_args.adder_def);
	if(hard_adder_models || !global_args.adder_def || soft_distribution == "default")
	{
		// given any of these cases do not optimize soft logic for adders
		return;
	}

	// use the default optimized file input
	if(soft_distribution == "optimized")
		soft_distribution = vtr::dirname(global_args.program_name) + "odin.soft_config";
	//else keep as is and try to open it

	FILE *input_file = fopen(soft_distribution.c_str(),"r");
	if(input_file)
	{
		printf("Reading soft_logic definition file @ %s ... ", soft_distribution.c_str());

		soft_def_map[std::string("+_0")] = NULL;
		soft_def_map[std::string("/_0")] = NULL;
		soft_def_map[std::string("%_0")] = NULL;
		soft_def_map[std::string("<<_0")] = NULL;

		int error = 0;
		int line_number = 0;
		char line_buf[1024] = { 0 };
		while (fgets(line_buf, 1024, input_file) != NULL && !error)
		{
			std::string line = line_buf;
			line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
			line_number += 1;

			if(line == "")
				break;
			
			std::vector<std::string> tokens;
			char *temp_str = strtok(vtr::strdup(line.c_str()),",");
			while(1)
			{				
				if(!temp_str)
				{
					error = (tokens.size() == 5)? 0: 1;
					break;
				}
				tokens.push_back(std::string(temp_str));
				temp_str = strtok(NULL,",");
			}

			if(!error)
			{
				std::string operation_name 			= tokens[0];
				int operation_bitsize			 			=	std::stoi(tokens[1]);
				std::string soft_hard						= tokens[2];
				std::string sub_structure_name	= tokens[3];
				int sub_structure_bitsize				=	std::stoi(tokens[4]);

				std::string lookup = operation_name + "_0";

				auto candidate = soft_def_map.find(lookup);
				if(candidate == soft_def_map.end() \
				|| operation_bitsize < 1 \
				|| operation_bitsize < sub_structure_bitsize \
				|| (soft_hard != "hard" && soft_hard != "soft"))
				{
					error =1;
					break;
				}
				else
				{
					std::string key_map = operation_name + "_" + tokens[1];
					soft_sub_structure* def = (soft_sub_structure*)vtr::malloc(sizeof(soft_sub_structure));
					def->type= vtr::strdup(soft_hard.c_str());
					def->name= vtr::strdup(sub_structure_name.c_str());
					def->bitsize = sub_structure_bitsize;

					soft_def_map[key_map] = def;
				}
			}
		}
		fclose(input_file);

		if(error)
		{
			/* something went wrong! empty everything and throw a warning */
			soft_def_map = std::map<std::string,soft_sub_structure*>();
			warning_message(PARSE_ERROR, -1, -1, "%s", "ERROR parsing adder soft logic definition file @ line::%ld\n",line_number);
		}

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
