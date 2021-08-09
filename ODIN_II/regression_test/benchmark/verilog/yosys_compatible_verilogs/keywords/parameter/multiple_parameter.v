module simple_op(in,out);
    parameter msb = 2,lsb = 0;

    input  [msb:lsb] in;
    output [msb:lsb] out;

    assign out = in;
endmodule 