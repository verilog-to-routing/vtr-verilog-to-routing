

module function_syntax (clock, a, b, c );

input clock;
input a;
input b;
output c;


//range of function xor_them returns a wire
function [1:1] xor_them;
input x;
input y;
reg regz;

	begin
		xor_them = x ^ y;
	end
endfunction

assign c = xor_them(a,b);

endmodule