module simple_op(in,out);
    localparam msb = 2;
    localparam lsb = 0;

    input  [msb:lsb] in;
    output [msb:lsb] out;

    assign out = in;
endmodule 