module simple_op(clk,res,q,clk_out);

  input clk;
	input res;
  output reg q;
	output clk_out;


always @(negedge clk) begin
	if(res)begin
		q<= 1'b1;
	end else begin
		q <= ~q;
	end
end
    
  assign clk_out = clk;
    
endmodule 