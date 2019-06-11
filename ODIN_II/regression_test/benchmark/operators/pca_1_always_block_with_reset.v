module simple_op(reset, 
	set, 
	a, 
	q
);

	input reset;
	input set;
	input a;

	output q;
	reg q;

always @(*)
begin

	if (reset)
		assign q = 1'b0;


	else
		deassign q;

end

endmodule
