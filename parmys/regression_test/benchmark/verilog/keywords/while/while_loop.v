`define WIDTH 3

module simple_op(out);
    
  output reg [`WIDTH-1:0] out;
	integer i;

  always @(*) begin
		i = 0;
    while (i <`WIDTH) begin
      out[i] = 1;
      i = i+1;
    end
  end

endmodule 