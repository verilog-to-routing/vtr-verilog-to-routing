
`include "bench_define.v"


module include_test (a,b,c);


	input a;
	input b;
	output c;

	`ifndef include_test_def
		assign c = a & b;
	`else
		assign c = a | b;
	`endif

endmodule