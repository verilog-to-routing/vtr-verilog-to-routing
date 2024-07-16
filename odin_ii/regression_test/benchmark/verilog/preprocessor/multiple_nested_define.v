`define START 2
`define END 0
`define RANGE [`START:`END]

module simple_op(in,out);
    input  `RANGE in;
    output `RANGE out;

    assign out = in;
endmodule