/***
 * this defines the structures for the adders if no file but optimization are requested
 * these are computed individually using adder_tree of each specific bit lenght 
 * 
 * TODO there is no way to modify this structure at the moment or allow odin to overwrite these at runtime
 * one interesting thing to do would be to wrap vtr within odin and parse the result to optimize the given circuit 
 * and run through all the possible branches of the adder size since each bits can have different dependency down the line 
 * and having Odin optimize these would allow for even faster circuit specif adder.
 * 
 * P.S it is extremely slow! running every branch for each adder_tree up to 128 bit took 4 days on 4 cores.
 * 
 * P.S.2 we can currently achieve this manualy using the bash script contained in vtr_flow/aritmetic_tasks/soft_adder
 */
 
#include "adders.h"
#include "types.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <vector>

std::vector<adder_def_t*> list_of_adder_def;

char adder_def_predefined[][32] = 
{
	"ripple,0",
	"fixed,1",
	"ripple,0",
	"fixed,2",
	"fixed,2",
	"fixed,3",
	"fixed,5",
	"fixed,6",
	"ripple,0",
	"fixed,8",
	"fixed,8",
	"fixed,11",
	"ripple,0",
	"ripple,0",
	"ripple,0",
	"fixed,15",
	"fixed,15",
	"fixed,17",
	"fixed,18",
	"fixed,18",
	"ripple,0",
	"fixed,19",
	"fixed,21",
	"fixed,16",
	"fixed,24",
	"fixed,25",
	"fixed,26",
	"ripple,0",
	"fixed,27",
	"fixed,20",
	"fixed,30",
	"fixed,31",
	"fixed,12",
	"fixed,29",
	"fixed,11",
	"fixed,35",
	"fixed,35",
	"fixed,12",
	"ripple,0",
	"fixed,37",
	"fixed,30",
	"fixed,13",
	"fixed,25",
	"ripple,0",
	"fixed,9",
	"fixed,23",
	"fixed,31",
	"fixed,44",
	"fixed,18",
	"fixed,20",
	"fixed,50",
	"ripple,0",
	"fixed,45",
	"ripple,0",
	"fixed,47",
	"fixed,51",
	"fixed,52",
	"ripple,0",
	"fixed,51",
	"fixed,59",
	"fixed,59",
	"ripple,0",
	"fixed,59",
	"fixed,56",
	"fixed,57",
	"fixed,58",
	"fixed,62",
	"fixed,60",
	"fixed,67",
	"fixed,62",
	"ripple,0",
	"fixed,64",
	"fixed,71",
	"fixed,66",
	"ripple,0",
	"fixed,71",
	"fixed,76",
	"fixed,76",
	"ripple,0",
	"fixed,76",
	"fixed,73",
	"fixed,74",
	"fixed,75",
	"fixed,76",
	"fixed,77",
	"fixed,78",
	"fixed,82",
	"fixed,80",
	"fixed,81",
	"fixed,82",
	"fixed,83",
	"fixed,84",
	"fixed,85",
	"fixed,86",
	"fixed,87",
	"fixed,95",
	"fixed,89",
	"fixed,90",
	"fixed,91",
	"fixed,92",
	"fixed,93",
	"fixed,94",
	"fixed,95",
	"fixed,96",
	"fixed,97",
	"fixed,98",
	"fixed,99",
	"fixed,100",
	"fixed,101",
	"fixed,102",
	"fixed,103",
	"fixed,104",
	"fixed,105",
	"fixed,106",
	"fixed,107",
	"fixed,108",
	"fixed,109",
	"fixed,110",
	"fixed,111",
	"fixed,112",
	"fixed,113",
	"fixed,114",
	"fixed,115",
	"fixed,116",
	"fixed,117",
	"fixed,118",
	"fixed,119",
	"fixed,120"
};

void populate_adder_def()
{
	adder_def_t *out = (adder_def_t*)malloc(sizeof(adder_def_t));
	out->next_step_size =0;
	out->type_of_adder = ripple;

	for(int lined=0; lined<128; lined++)
	{
		std::string type(strtok(adder_def_predefined[lined],","));
		if(type == "ripple")			out->type_of_adder = ripple;
		else if (type == "fixed")		out->type_of_adder = fixed_step;
		
		out->next_step_size = strtol(strtok(NULL,","),NULL,10);
		list_of_adder_def.push_back(out);
	}
	return;
}

void read_adder_def_file(const char *input_file_name)
{
	adder_def_t *out = (adder_def_t*)malloc(sizeof(adder_def_t));
	out->next_step_size =0;
	out->type_of_adder = ripple;

	FILE *input_file = fopen(input_file_name,"r");
	
	const size_t line_size = 128;
	char line_buf[line_size];
	char *line = line_buf;

	while (fgets(line, line_size, input_file) != NULL)
	{
		std::string type(strtok(line,","));
		if(type == "ripple")			out->type_of_adder = ripple;
		else if (type == "fixed")		out->type_of_adder = fixed_step;

		out->next_step_size = strtol(strtok(NULL,","),NULL,10);
		list_of_adder_def.push_back(out);
	}
	fclose(input_file);
}
