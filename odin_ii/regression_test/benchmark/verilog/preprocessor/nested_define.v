`define WIDTH 2
`define RANGE [`WIDTH:0]

module simple_op(in,out);
    input  `RANGE in;
    output `RANGE out;

    assign out = in;
endmodule

