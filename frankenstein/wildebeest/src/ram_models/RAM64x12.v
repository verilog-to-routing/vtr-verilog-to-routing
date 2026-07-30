
(* blackbox *)
module RAM64x12 (
	input			R_CLK,
	input [5:0]		R_ADDR,
	input			R_ADDR_BYPASS,
	input			R_ADDR_EN,
	input 			R_ADDR_SL_N,
	input			R_ADDR_SD,
	input			R_ADDR_AL_N, 
	input			R_ADDR_AD_N,
	input			BLK_EN,
	output [11:0]	R_DATA,
	input			R_DATA_BYPASS,
	input	 		R_DATA_EN,
	input	 		R_DATA_SL_N,
	input	 		R_DATA_SD,
	input	 		R_DATA_AL_N,
	input	 		R_DATA_AD_N,

	input		W_CLK,
	input [5:0]	W_ADDR,
	input [11:0]W_DATA,
	input		W_EN,

	input		BUSY_FB,
	output		ACCESS_BUSY
);
parameter INIT0 = 64'h0;
parameter INIT1 = 64'h0;
parameter INIT2 = 64'h0;
parameter INIT3 = 64'h0;
parameter INIT4 = 64'h0;
parameter INIT5 = 64'h0;
parameter INIT6 = 64'h0;
parameter INIT7 = 64'h0;
parameter INIT8 = 64'h0;
parameter INIT9 = 64'h0;
parameter INIT10 = 64'h0;
parameter INIT11 = 64'h0;

endmodule
