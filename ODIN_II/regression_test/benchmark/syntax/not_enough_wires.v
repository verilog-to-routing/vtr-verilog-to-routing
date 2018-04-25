
/* Odin crashes when instantiating a module and not sending enough enough wires */

module bar(a,b,c,d,clk);
input a,b,c;
input clk;
output d;

wire x;

register new_reg (a,clk,b, c,x); 
endmodule



module register(d,clk,resetn,en,q);
//parameter WIDTH=32;






input clk;
input resetn;
input en;
input [31:0] d;
output [31:0] q;
reg [31:0] q;

always @(posedge clk )		
begin
	if (resetn==0)
		q<=0;
	else if (en==1)
		q<=d;
end

endmodule