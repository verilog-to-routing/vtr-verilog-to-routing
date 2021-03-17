`define WIDTH 4

module top_module (clk, x, y, q1, q2);
	input 	clk;
    input  	[`WIDTH-1:0] x; 
    input   [`WIDTH-1:0] y;
    output	[2*`WIDTH-1:0] q1;
    output	[2*`WIDTH-1:0] q2;

	wire [2*`WIDTH-1:0] f;

	second_module my_module (clk, {x[3], x[2:0]}, {y[3], y[2:0]}, f);

	second_module my_p_module (clk, {f[3], f[2:0]}, x, q1);

	second_module my_p2_module (clk, {f[3], f[2:0]}, y, q2);


endmodule

module second_module (clk, in1, in2, out);

	input 	clk;
	input	[`WIDTH-1:0] in1;
    input	[`WIDTH-1:0] in2;
    output reg	[2*`WIDTH-1:0] out;

	always@(posedge clk)
	begin
	out <= in1 + in2;
	end


endmodule

/*
	-V ~/desktop/exin/module/expression_in_chain_modules.v -o ~/desktop/exin/module/output.blif -G -A -t ~/desktop/exin/module/expression_in_chain_modules_input -T ~/desktop/exin/module/expression_in_chain_modules_output

*/
