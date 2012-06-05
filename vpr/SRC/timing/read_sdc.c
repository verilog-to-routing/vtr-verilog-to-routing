#include <string.h>
#include <stdio.h>
#include "assert.h"
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "read_sdc.h"
#include "read_blif.h"
#include "path_delay.h"

/* read_sdc.c is work in progress */

static FILE *sdc;

void read_sdc(char * sdc_file, int num_clocks) {
/*This function reads the constraints from the SDC file *
* specified on the command line into float ** timing_   *
* constraints.  If no file is specified, it leaves      *
* timing_constraints pointing to NULL.                  */

	char buf[BUFSIZE];
	int num_lines = 0;
	
	if (sdc == NULL) {
		printf("SDC file %s not found.\n", sdc_file);
		printf("All clocks will be analysed together during timing analysis.\n\n");
		return;
	}
	
	/* Now we know there's an SDC file, so open and parse it */
	sdc = fopen(sdc_file, "r");
	timing_constraints = (float **) my_malloc(num_clocks * num_clocks * sizeof(float));
	/*[0..num_clocks-1][0..num_clocks.1]*/

	while (my_fgets(buf, BUFSIZE, sdc) != NULL) {
		get_sdc_tok(buf, num_lines);
		num_lines++;
	}

	fclose(sdc);

}

void get_sdc_tok(char * buf, int num_lines) {
	/* Figures out which, if any token is at the start of this line and *
	 * takes the appropriate action.                                    */

#define SDC_TOKENS " \t\n{}" 
	/*We're using so little of the SDC syntax that we can ignore braces*/

	char * ptr;
	/*char clockname[BUFSIZE];*/

	ptr = my_strtok(buf, TOKENS, sdc, buf);
	if (ptr == NULL)
		return;

	if (strcmp(ptr, "create_clock") == 0) {
		/*make sure clock has a period specified*/
		ptr = my_strtok(NULL, TOKENS, sdc, buf);
		if(strcmp(buf, "-period")!=0) {
			fprintf(stderr, "Clock requires a '-period' on line %d of SDC file", num_lines);
			exit(1);
		}
		
	}
	else {
		fprintf(stderr, "Incorrect or unsupported syntax in line %d of SDC file", num_lines);
		exit(1);
	}
}