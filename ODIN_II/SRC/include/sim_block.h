#include "odin_types.h"

/*
	This method is what you need to implement in order to get generic black block
	simulation working. This method is called once per cycle. The inputs and ouputs are
	provided in the order specified in the verilog file. The cycle parameter starts
	at zero and increments with each successive invocation of the method.

	The method encompasses the rising edge and falling edge of one clock tick.
*/
void simulate_block_cycle(int cycle, int num_input_pins, int *inputs, int num_output_pins, int *outputs);
