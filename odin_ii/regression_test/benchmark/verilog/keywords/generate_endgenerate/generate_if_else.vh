module simple_op(a,b,out);
	input a,b;
	output out;
	
generate 
	if(`en)begin
		assign out = a;
	end
	else begin
		assign out = b;
		end
	endgenerate
endmodule