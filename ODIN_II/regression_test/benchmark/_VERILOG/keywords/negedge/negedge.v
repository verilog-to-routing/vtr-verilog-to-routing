module simple_op(clk,a,q);

  input clk;
  input a;
  output reg q;


always @(negedge clk) begin
	q <= a;
end

endmodule