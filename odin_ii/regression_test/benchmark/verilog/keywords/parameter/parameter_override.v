module simple_op(in1,in2,in3,out1,out2);
    parameter msb1 = 3;
    parameter msb2 = 2;
    parameter lsb  = 0;
    
    input  [msb1:lsb] in1;
    input  [msb2:lsb] in2;
    input  [msb2:lsb] in3;
    output [msb1:lsb] out1;
    output [msb2:lsb] out2;

    assg #(.msb(msb1),.lsb(lsb)) assg(in1,out1);

    addi #(.msb(msb2),.lsb(lsb)) addi(in2,in3,out2);
    
endmodule 


module assg(in,out);
    parameter msb = 8;
    parameter lsb = 1;

    input  [msb:lsb] in;
    output [msb:lsb] out;

    assign out = in;
endmodule

module addi(in1,in2,out);
    parameter msb = 4;
    parameter lsb = 2;
    
    input  [msb:lsb] in1;
    input  [msb:lsb] in2;
    output [msb:lsb] out;

    assign out = in1 + in2;
endmodule