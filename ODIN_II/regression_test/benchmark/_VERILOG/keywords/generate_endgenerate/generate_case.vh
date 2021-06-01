module simple_op(a,b,c,out);
	input [2:0] a,b,c;
    output [2:0] out;
	
	

generate
    case(`sel)
        2'b00   :   assign out = a;
        2'b01   :   assign out = b;
        2'b10   :   assign out = c;
        2'b11   :   assign out = 0;
    endcase
endgenerate 
endmodule 