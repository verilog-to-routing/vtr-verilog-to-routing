`define WIDTH 3 

module simple_op(out);
    
    output [`WIDTH-1:0] out;
	 
	genvar i;
	generate
        for (i = 0;i<`WIDTH;i = i+1) begin: a
            assign out[i] = 1;
      end
	endgenerate 
endmodule 