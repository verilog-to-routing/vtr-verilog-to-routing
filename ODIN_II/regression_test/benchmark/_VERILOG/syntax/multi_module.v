module testmodule (a,b,c);

input [31:0] a;
input [31:0] b;
output [31:0] c;

assign c = a & b;

endmodule

module define_tester (a,b, c);

input [31:0] a;

input [31:0] b;
output [31:0] c;
testmodule mymod (a,b,c);

endmodule


