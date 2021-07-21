 
module simple_op(in,en,out);
    
    input in;
    input en;
    output reg out;
	 
always @(*) begin

    if(en)
        out = in;
    else
        out = 1'b0;
end 
endmodule 