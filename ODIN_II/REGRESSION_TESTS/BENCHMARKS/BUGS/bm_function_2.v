// DEFINES
`define BITS 2         // Bit width of the operands

module 	bm_function_2(clock, 
		reset_n, 
		a, 
		b,
		c, 
		d, 
		out0,
		out1);

// SIGNAL DECLARATIONS
input	clock;
input 	reset_n;

input [`BITS-1:0] a;
input [`BITS-1:0] b;
input c;
input d;

output [`BITS-1:0] out0;
output  out1;

reg [`BITS-1:0]    out0;
wire     out1;

reg temp1;
reg [`BITS-1:0]temp2;


function simple_func;
	input [`BITS-1:0] in1;
	input [`BITS-1:0] in2;
	input in3;
	input in4;

	simple_func <= in1[0] & in2[0] | in3 ^ in4;
endfunction

function [`BITS-1:0] simple_func_2;
	input in3;
	input in4;
begin
	simple_func_2[1] <= in3 & in4;
	simple_func_2[0] <= in3 | in4;
end
endfunction

assign out1 = simple_func(a, b, c, d) & c;

always @(posedge clock or negedge reset_n)
begin
	temp1 <= simple_func(a, b, c, d);
	temp2 <= simple_func_2(c, d);
	out0 <= temp1 & temp2;
end

endmodule
