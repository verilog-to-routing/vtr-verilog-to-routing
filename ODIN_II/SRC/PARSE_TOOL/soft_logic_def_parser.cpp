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
		printf("Reading soft_logic definition file @ %s ... ", input_file_name);

		soft_def_map[std::string("+_0")] = NULL;
		soft_def_map[std::string("/_0")] = NULL;
		soft_def_map[std::string("%_0")] = NULL;
		soft_def_map[std::string("<<_0")] = NULL;

		int error = 0;
		int line_number = 0;
		int line_buffer_sz = 5*64+1; /* 5 token ~64 char each should be enough */
		char *line_buf = (char*)vtr::malloc(sizeof(char)*line_buffer_sz);
		while (fgets(line_buf, line_buffer_sz, input_file) != NULL && !error)
		{
			line_number += 1;
			int len = strnlen(line_buf,line_buffer_sz);
			/* remove the newline */
			if(len > 0 && line_buf[len-1] == '\n')
				line_buf[len--] = '\0';

			if(len < 1)
			{
				break;
			}
			else
			{

				char *temp_ptr = line_buf;
				std::vector<std::string> tokens;
				while(1)
				{
					char *temp = strtok(temp_ptr,",");
					temp_ptr = NULL;
					if(temp && strnlen(temp,65) > 0)
					{
						tokens.push_back(std::string(temp));
					}
					else if(!temp && tokens.size() == 5)
					{
						break;
					}
					else
					{
						error =1;
						break;
					}
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
						def->type = strdup(soft_hard.c_str());
						def->name = strdup(sub_structure_name.c_str());
						def->bitsize = sub_structure_bitsize;

						soft_def_map[key_map] = def;
					}
				}
			}
		}
		vtr::free(line_buf);
		fclose(input_file);

		if(error)
		{
			/* something went wrong! empty everything and throw a warning */
			soft_def_map = std::map<std::string,soft_sub_structure*>();
			printf("ERROR line::%d\n",line_number);
		}
		else
			printf("DONE read %d lines\n",line_number);
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
