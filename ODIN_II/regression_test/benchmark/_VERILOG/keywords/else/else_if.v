module simple_op(comp0,comp1,out);
    
    input comp0;
    input comp1;
    output reg [1:0] out;
	 
always @(*) begin

    if(comp0 > comp1)
        out = 2'b01;
    else if (comp1 == comp0)
        out = 2'b00;
    else
        out = 2'b10;
end 
endmodule 