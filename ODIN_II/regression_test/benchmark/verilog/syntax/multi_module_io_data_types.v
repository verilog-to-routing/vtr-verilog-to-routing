module testmodule (a,b,c,d);

input [31:0] a;
input [31:0] b;
output reg [31:0] c;
output wire [31:0] d;

assign c = a & b;
assign d = a | b;

endmodule

module define_tester (a,b, c);

input [31:0] a;
input wire [31:0] b;
output reg [31:0] c;
output wire [31:0] d;

testmodule mymod (a,b,c,d);

endmodule