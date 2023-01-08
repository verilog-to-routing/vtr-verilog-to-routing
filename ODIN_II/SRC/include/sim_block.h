/*
 * Copyright 2023 CAS—Atlantic (University of New Brunswick, CASA)
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef SIM_BLOCK_H
#define SIM_BLOCK_H

#include "odin_types.h"

/*
 * This method is what you need to implement in order to get generic black block
 * simulation working. This method is called once per cycle. The inputs and ouputs are
 * provided in the order specified in the verilog file. The cycle parameter starts
 * at zero and increments with each successive invocation of the method.
 *
 * The method encompasses the rising edge and falling edge of one clock tick.
 */
void simulate_block_cycle(int cycle, int num_input_pins, int* inputs, int num_output_pins, int* outputs);

#endif
