module simple_op(in,out);
    parameter msb = 2;
    localparam lsb = msb-msb;

    input  [msb:lsb] in;
    output [msb:lsb] out;

    assign out = in;
endmodule 