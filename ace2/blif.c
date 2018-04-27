#include <stdio.h>
#include <string.h>

#include "ace.h"
#include "blif.h"
bool blif_clock_from_latch(char * latch_line, char * clk_name);

bool blif_clock_from_latch(char * latch_line, char * clk_name) {
	char * pos;

	pos = strtok(latch_line, " ");
	pos = strtok(NULL, " ");
	pos = strtok(NULL, " ");
	pos = strtok(NULL, " ");
	pos = strtok(NULL, " ");

	if (pos) {
		strncpy(clk_name, pos, ACE_CHAR_BUFFER_SIZE);
		clk_name[ACE_CHAR_BUFFER_SIZE - 1] = '\0';
		return TRUE;
	} else {
		return FALSE;
	}
}

#if 0
void blif_clock_info(char * blif_file_name, int * num_clks, char * clk_name)
{
	bool multiple_clocks = FALSE;
	FILE * file_desc;
	char line[ACE_CHAR_BUFFER_SIZE];
	char clk_name_saved[ACE_CHAR_BUFFER_SIZE] = "";
	char clk_name_temp[ACE_CHAR_BUFFER_SIZE];

	char str_latch[] = ".latch";

	file_desc = fopen(blif_file_name, "r");
	while(fgets(line, ACE_CHAR_BUFFER_SIZE, file_desc))
	{
		char * start_pos = line;
		while (start_pos[0] == ' ' || start_pos[0] == '\t')
		{
			start_pos++;
		}

		// Check for .latch
		if ((start_pos == NULL) || (strncmp(start_pos, str_latch, 6) != 0))
		{
			continue;
		}

		// Extract and check clock name
		if (blif_clock_from_latch(line, clk_name_temp))
		{
			if (strcmp(clk_name_saved, "") == 0)
			{
				strcpy(clk_name_saved, clk_name_temp);
			}
			else if (strcmp(clk_name_saved, clk_name_temp))
			{
				multiple_clocks = TRUE;
				break;
			}
		}
	}

	if (multiple_clocks)
	{
		*num_clks = 2;
	}
	else if (strcmp(clk_name_saved, "") != 0)
	{
		*num_clks = 1;
		strcpy(clk_name, clk_name_saved);
	}
	else
	{
		*num_clks = 0;
	}
}
#endif
