
module simple_op(a,b,c,sel,out);
    input [2:0] a,b,c;
    input [1:0] sel;
    output reg [2:0] out;
	
	
always @(*)
begin
    case(sel)
        2'b00   :   out = a;
        2'b01   :   out = b;
        2'b10   :   out = c;
        2'b11   :   out = 0;
    endcase
end 
endmodule 