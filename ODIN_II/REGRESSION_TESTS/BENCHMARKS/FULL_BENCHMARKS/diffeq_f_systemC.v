
/*--------------------------------------------------------------------------
--------------------------------------------------------------------------
--	File Name 	: diffeq.v
--	Author(s)	: P. Sridhar
--	Affiliation	: Laboratory for Digital Design Environments
--			  Department of Electrical & Computer Engineering
--			  University of Cincinnati
--	Date Created	: June 1991.
--	Introduction	: Behavioral description of a differential equation
--			  solver written in a synthesizable subset of VHDL.
--	Source		: Written in HardwareC by Rajesh Gupta, Stanford Univ. 
--			  Obtained from the Highlevel Synthesis Workshop
--                        Repository.
--
--	Modified For Synthesis by Jay(anta) Roy, University of Cincinnati.
--	Date Modified	: Sept, 91.
--
--	Disclaimer	: This comes with absolutely no guarantees of any
--			  kind (just stating the obvious ...)
--
--      Acknowledgement : The Distributed Synthesis Systems research at 
--                        the Laboratory for Digital Design Environments,
--                        University of Cincinnati, is sponsored in part 
--                        by the Defense Advanced Research Projects Agency 
--                        under order number 7056 monitored by the Federal 
--                        Bureau of Investigation under contract number 
--                        J-FBI-89-094.
--
--------------------------------------------------------------------------
-------------------------------------------------------------------------*/
module diffeq_f_systemC(aport, dxport, xport, yport, uport, clk, reset);

input clk;
input reset;
input [31:0]aport;
input [31:0]dxport;
output [31:0]xport;
output [31:0]yport;
output [31:0]uport;
reg [31:0]xport;
reg [31:0]yport;
reg [31:0]uport;
wire [31:0]temp;

assign temp = uport * dxport;
always @(posedge clk or posedge reset)
begin
	if (reset == 1'b1)
	begin
		xport <= 0;
		yport <= 0;
		uport <= 0;
	end
else
  	if (xport < aport) 
	begin
		xport <= xport + dxport;
		yport <= yport + temp;//(uport * dxport);
		uport <= (uport - (temp/*(uport * dxport)*/ * (5 * xport))) - (dxport * (3 * yport));
	end
end
endmodule
