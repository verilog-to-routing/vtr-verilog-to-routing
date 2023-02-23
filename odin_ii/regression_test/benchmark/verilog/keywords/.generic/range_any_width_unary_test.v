/*
 * Generates a gate for each output (ie. out[0],out[1])
 * Tests the width of unary gates
*/

`define RANGE [`WIDTH-1:0]

module simple_op(a,out);
    input 	`RANGE a;
    output 	`RANGE out;

    `operator(out`RANGE,a`RANGE);

endmodule 