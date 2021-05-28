`define WIDTH 4

module top_module (rst, x, y, q);
	input 	rst;
    input  	[`WIDTH-1:0] x; 
    input   [`WIDTH-1:0] y;
    output	[2*`WIDTH-1:0] q;

	second_module my_module (.a({{2{x[1]}}, y[3]}), 
							 .b({rst, x[2]|~y[2], (rst ? 2'b00 : {x[1], y[1]})}), 
							 .so(q)
							);

endmodule

module second_module (a, b, so);

    input	[`WIDTH-1:0] a;
    input	[`WIDTH-1:0] b;
    output	[2*`WIDTH-1:0] so;

    third_module my_p_module (.in1({ {2{a[1]}}, {2{b[0]}} }), .in2(a<<1), .out(so));
	
endmodule

module third_module (in1, in2, out);

	input	[`WIDTH-1:0] in1;
    input	[`WIDTH-1:0] in2;
    output	[2*`WIDTH-1:0] out;

	assign out = in1 + in2;

endmodule

/*
	-V ~/Desktop/exin/module/mix_expression_in_module_port_nested.v -o ~/Desktop/exin/module/output.blif -G -A -t ~/Desktop/exin/module/mix_expression_in_module_port_nested_input -T ~/Desktop/exin/module/mix_expression_in_module_port_nested_output
{2{a[1}}, {2{b[0]}}

1bd7a518e25b4c01fa587c919eb1439082ccdcd1
*/
