module edge_testing (d0,clk, resetn, en,q0);

input clk;
input resetn;
input en;
input d0;
output q0;

posedge_reg_sync_reset zzxx_posedge_reg_sync_reset_zzxx (d2,clk,resetn,en,q2);

endmodule


module posedge_reg_sync_reset(d,clk,resetn,en,q);


input clk;
input resetn;
input en;
input d;
output  q;
reg q;

always @(posedge clk )		
begin : sync_reset
	if (resetn==0)
		q<=0;
	else if (en==1)
		q<=d;
end

endmodule
