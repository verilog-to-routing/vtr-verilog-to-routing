/*
 * Replicates the indexing process for each bit. For more information on this method page 78, 
 * section 7.1.6 of the IEEE Standard for Verilog® Hardware Description Language.
*/

`define RANGE [`WIDTH-1:0]

module simple_op(a,b,out);
    //input 	`RANGE a;
    //input   `RANGE b;
    //output 	`RANGE out;
    input 	[`WIDTH-1:0] a;
    input   [`WIDTH-1:0] b;
    output 	[`WIDTH-1:0] out;

/*genvar i;
generate
  for(i = 0; i < `WIDTH; i = i + 1) begin : bitop
    `operator u_op (out[i], a[i], b[i]);
  end
endgenerate*/
    assign out = a `operator b;
//`operator ar`RANGE(out,a,b);
//`operator ar[`WIDTH-1:0] (out,a,b);
endmodule
