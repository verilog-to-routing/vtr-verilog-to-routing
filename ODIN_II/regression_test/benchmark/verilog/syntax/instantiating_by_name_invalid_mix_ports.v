module and2 (in1,in2,clk,out);
input in1,in2;
input clk;
output out;

assign out = in1 & in2;
endmodule

module top (a,b,c,out);
input a,b,c;
output out;

and2 a1 (.in2(a),.in1(b),out);

endmodule
