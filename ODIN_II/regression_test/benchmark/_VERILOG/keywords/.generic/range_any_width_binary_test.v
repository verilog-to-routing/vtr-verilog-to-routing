/*
 * Generates a gate for each output (ie. out[0],out[1])
 * Tests the width of binary gates
*/

`define RANGE [`WIDTH-1:0]

module simple_op(a,b,out);
    input 	`RANGE a;
    input   `RANGE b;
    output 	`RANGE out;

    `operator(out`RANGE,a`RANGE,b`RANGE);

endmodule 