/*
 * Replicates the indexing process for each bit. For more information on this method page 78, 
 * section 7.1.6 of the IEEE Standard for Verilog® Hardware Description Language.
*/

`define RANGE [`WIDTH-1:0]

module simple_op(a,out);
    input 	`RANGE a;
    output 	`RANGE out;

`operator ar`RANGE(out,a);
endmodule