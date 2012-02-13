
// Skeleton
// Reads constants and instantiates all modules used by the hardware components

//parameter BIT_WIDTH=32;
//parameter LAYER_WIDTH=3;
`define TRIG_WIDTH 5'b01010 //10;
`define PIPELINE_DEPTH_UPPER_LIMIT 10'b0100000000 //256;
`define ABSORB_ADDR_WIDTH 6'b010000 //16;
`define ABSORB_WORD_WIDTH 7'b01000000 //64;
`define WSCALE 22'b0111010100101111111111 //1919999;

//From Roulette
//`define BIT_WIDTH 7'b0100000
//`define LAYER_WIDTH 6'b000011 
`define LEFTSHIFT 6'b000011         // 2^3=8=1/0.125 where 0.125 = CHANCE of roulette
`define INTCHANCE 32'b00100000000000000000000000000000 //Based on 32 bit rand num generator
`define MIN_WEIGHT 9'b011001000 

// From Boundary
`define BIT_WIDTH 7'b0100000
`define LAYER_WIDTH 6'b000011
`define INTMAX 32'b01111111111111111111111111111111
`define INTMIN 32'b10000000000000000000000000000000
`define DIVIDER_LATENCY 6'b011110
`define FINAL_LATENCY 6'b011100
`define MULT_LATENCY 1'b1
`define ASPECT_RATIO 6'b000111
`define TOTAL_LATENCY 7'b0111100

//From Move
//`define BIT_WIDTH 6'b100000
//`define LAYER_WIDTH 6'b000011
`define LOGSCALEFACTOR 6'b000101
`define MAXLOG 32'b10110001011100100001011111110111		//Based on 32 bit unsigned rand num generator
`define CONST_MOVE_AMOUNT 15'b110000110101000  //Used for testing purposes only
`define MUTMAX_BITS 6'b001111

//From Hop
//`define BIT_WIDTH 6'b100000
//`define LAYER_WIDTH 6'b000011
//`define INTMAX 32'b01111111111111111111111111111111
//`define INTMIN 32'b10000000000000000000000000000000

//From LogCalc
//`define BIT_WIDTH 7'b0100000
`define MANTISSA_PRECISION 6'b001010
`define LOG2_BIT_WIDTH 6'b000110
`define LOG2 28'b0101100010111001000010111111

//From DropSpinWrapper
`define NR 10'b0100000000             
`define NZ 10'b0100000000              

`define NR_EXP 5'b01000              //meaning `NR=2^`NR_exp or 2^8=256
`define RGRID_SCLAE_EXP 6'b010101    //2^21 = RGRID_SCALE
`define ZGRID_SCLAE_EXP 6'b010101    //2^21 = ZGRID_SCALE


//`define BIT_WIDTH 7'b0100000
`define BIT_WIDTH_2 8'b01000000
`define WORD_WIDTH 8'b01000000
`define ADDR_WIDTH 6'b010000          //256x256=2^8*2^8=2^16


//From scatterer:
`define DIV 6'b010100//20;
`define SQRT 5'b01010 //10;
`define LAT 7'b0100101 //DIV + SQRT + 7;
`define INTMAX_2 {32'h3FFFFFFF,32'h00000001}
//`define INTMAX 32'b01111111111111111111111111111111//2147483647;
//`define INTMIN 32'b10000000000000000000000000000001//-2147483647;
`define INTMAXMinus3 32'b01111111111111111111111111111100//2147483644;
`define negINTMAXPlus3 32'b10000000000000000000000000000100//-2147483644;

//From Reflector:
`define INTMAX_2_ref {32'h3FFFFFFF,32'hFFFFFFFF}


module mcml (
	reset,
	clk,	

	constants,
	read_constants,

	result, 
	inc_result,

	calc_in_progress
	);

// Total number of constants
//parameter LAST_CONSTANT = 104;
//parameter NUM_FRESNELS = 128;
//parameter NUM_TRIG_ELS = 1024;
//parameter ABSORB_ADDR_WIDTH=16;
//parameter ABSORB_WORD_WIDTH=64;
//parameter BIT_WIDTH = 32;

input				reset;
input				clk;
input [31:0]		constants;
input				read_constants;
input				inc_result;

output				calc_in_progress;
output [31:0]		result;
reg					calc_in_progress;
reg [31:0]			result;


//integer i;

wire [31:0] mem_fres_up, mem_fres_down, mem_sint, mem_cost;

// photon calculator

wire reset;

// Scatterer Reflector memory look-up
wire [12:0] tindex;
wire [9:0] fresIndex;

//   DeadOrAlive Module (nothing)

// Final results
wire [16-1:0] absorb_rdaddress, absorb_wraddress;
wire absorb_wren;
wire [64-1:0] absorb_data;
wire [64-1:0] absorb_q;

// Flag when final results ready
wire done;
reg enable;
reg reset_calculator;

// Combinational drivers
//reg [31:0]			c_const[104 - 1:0];
reg [31:0]			c_const__103;
reg [31:0]			c_const__102;
reg [31:0]			c_const__101;
reg [31:0]			c_const__100;
reg [31:0]			c_const__99;
reg [31:0]			c_const__98;
reg [31:0]			c_const__97;
reg [31:0]			c_const__96;
reg [31:0]			c_const__95;
reg [31:0]			c_const__94;
reg [31:0]			c_const__93;
reg [31:0]			c_const__92;
reg [31:0]			c_const__91;
reg [31:0]			c_const__90;
reg [31:0]			c_const__89;
reg [31:0]			c_const__88;
reg [31:0]			c_const__87;
reg [31:0]			c_const__86;
reg [31:0]			c_const__85;
reg [31:0]			c_const__84;
reg [31:0]			c_const__83;
reg [31:0]			c_const__82;
reg [31:0]			c_const__81;
reg [31:0]			c_const__80;
reg [31:0]			c_const__79;
reg [31:0]			c_const__78;
reg [31:0]			c_const__77;
reg [31:0]			c_const__76;
reg [31:0]			c_const__75;
reg [31:0]			c_const__74;
reg [31:0]			c_const__73;
reg [31:0]			c_const__72;
reg [31:0]			c_const__71;
reg [31:0]			c_const__70;
reg [31:0]			c_const__69;
reg [31:0]			c_const__68;
reg [31:0]			c_const__67;
reg [31:0]			c_const__66;
reg [31:0]			c_const__65;
reg [31:0]			c_const__64;
reg [31:0]			c_const__63;
reg [31:0]			c_const__62;
reg [31:0]			c_const__61;
reg [31:0]			c_const__60;
reg [31:0]			c_const__59;
reg [31:0]			c_const__58;
reg [31:0]			c_const__57;
reg [31:0]			c_const__56;
reg [31:0]			c_const__55;
reg [31:0]			c_const__54;
reg [31:0]			c_const__53;
reg [31:0]			c_const__52;
reg [31:0]			c_const__51;
reg [31:0]			c_const__50;
reg [31:0]			c_const__49;
reg [31:0]			c_const__48;
reg [31:0]			c_const__47;
reg [31:0]			c_const__46;
reg [31:0]			c_const__45;
reg [31:0]			c_const__44;
reg [31:0]			c_const__43;
reg [31:0]			c_const__42;
reg [31:0]			c_const__41;
reg [31:0]			c_const__40;
reg [31:0]			c_const__39;
reg [31:0]			c_const__38;
reg [31:0]			c_const__37;
reg [31:0]			c_const__36;
reg [31:0]			c_const__35;
reg [31:0]			c_const__34;
reg [31:0]			c_const__33;
reg [31:0]			c_const__32;
reg [31:0]			c_const__31;
reg [31:0]			c_const__30;
reg [31:0]			c_const__29;
reg [31:0]			c_const__28;
reg [31:0]			c_const__27;
reg [31:0]			c_const__26;
reg [31:0]			c_const__25;
reg [31:0]			c_const__24;
reg [31:0]			c_const__23;
reg [31:0]			c_const__22;
reg [31:0]			c_const__21;
reg [31:0]			c_const__20;
reg [31:0]			c_const__19;
reg [31:0]			c_const__18;
reg [31:0]			c_const__17;
reg [31:0]			c_const__16;
reg [31:0]			c_const__15;
reg [31:0]			c_const__14;
reg [31:0]			c_const__13;
reg [31:0]			c_const__12;
reg [31:0]			c_const__11;
reg [31:0]			c_const__10;
reg [31:0]			c_const__9;
reg [31:0]			c_const__8;
reg [31:0]			c_const__7;
reg [31:0]			c_const__6;
reg [31:0]			c_const__5;
reg [31:0]			c_const__4;
reg [31:0]			c_const__3;
reg [31:0]			c_const__2;
reg [31:0]			c_const__1;
reg [31:0]			c_const__0;


reg [12:0]			c_counter;
reg	c_toggle;

reg [16-1:0] c_absorb_read_counter, c_absorb_write_counter;
reg [16-1:0] absorb_rdaddress_mux, absorb_wraddress_mux;
reg [64-1:0] absorb_data_mux;
reg absorb_wren_mux;

reg [3:0]			c_state;

reg [31:0]			c_result;
reg					c_calc_in_progress;

reg wren_fres_up, wren_fres_down, wren_sinp, wren_cosp, wren_sint, wren_cost;
reg [2:0] mem_layer;

// Registered drivers
//reg [31:0]			r_const[104 - 1:0];
reg [31:0]			r_const__103;
reg [31:0]			r_const__102;
reg [31:0]			r_const__101;
reg [31:0]			r_const__100;
reg [31:0]			r_const__99;
reg [31:0]			r_const__98;
reg [31:0]			r_const__97;
reg [31:0]			r_const__96;
reg [31:0]			r_const__95;
reg [31:0]			r_const__94;
reg [31:0]			r_const__93;
reg [31:0]			r_const__92;
reg [31:0]			r_const__91;
reg [31:0]			r_const__90;
reg [31:0]			r_const__89;
reg [31:0]			r_const__88;
reg [31:0]			r_const__87;
reg [31:0]			r_const__86;
reg [31:0]			r_const__85;
reg [31:0]			r_const__84;
reg [31:0]			r_const__83;
reg [31:0]			r_const__82;
reg [31:0]			r_const__81;
reg [31:0]			r_const__80;
reg [31:0]			r_const__79;
reg [31:0]			r_const__78;
reg [31:0]			r_const__77;
reg [31:0]			r_const__76;
reg [31:0]			r_const__75;
reg [31:0]			r_const__74;
reg [31:0]			r_const__73;
reg [31:0]			r_const__72;
reg [31:0]			r_const__71;
reg [31:0]			r_const__70;
reg [31:0]			r_const__69;
reg [31:0]			r_const__68;
reg [31:0]			r_const__67;
reg [31:0]			r_const__66;
reg [31:0]			r_const__65;
reg [31:0]			r_const__64;
reg [31:0]			r_const__63;
reg [31:0]			r_const__62;
reg [31:0]			r_const__61;
reg [31:0]			r_const__60;
reg [31:0]			r_const__59;
reg [31:0]			r_const__58;
reg [31:0]			r_const__57;
reg [31:0]			r_const__56;
reg [31:0]			r_const__55;
reg [31:0]			r_const__54;
reg [31:0]			r_const__53;
reg [31:0]			r_const__52;
reg [31:0]			r_const__51;
reg [31:0]			r_const__50;
reg [31:0]			r_const__49;
reg [31:0]			r_const__48;
reg [31:0]			r_const__47;
reg [31:0]			r_const__46;
reg [31:0]			r_const__45;
reg [31:0]			r_const__44;
reg [31:0]			r_const__43;
reg [31:0]			r_const__42;
reg [31:0]			r_const__41;
reg [31:0]			r_const__40;
reg [31:0]			r_const__39;
reg [31:0]			r_const__38;
reg [31:0]			r_const__37;
reg [31:0]			r_const__36;
reg [31:0]			r_const__35;
reg [31:0]			r_const__34;
reg [31:0]			r_const__33;
reg [31:0]			r_const__32;
reg [31:0]			r_const__31;
reg [31:0]			r_const__30;
reg [31:0]			r_const__29;
reg [31:0]			r_const__28;
reg [31:0]			r_const__27;
reg [31:0]			r_const__26;
reg [31:0]			r_const__25;
reg [31:0]			r_const__24;
reg [31:0]			r_const__23;
reg [31:0]			r_const__22;
reg [31:0]			r_const__21;
reg [31:0]			r_const__20;
reg [31:0]			r_const__19;
reg [31:0]			r_const__18;
reg [31:0]			r_const__17;
reg [31:0]			r_const__16;
reg [31:0]			r_const__15;
reg [31:0]			r_const__14;
reg [31:0]			r_const__13;
reg [31:0]			r_const__12;
reg [31:0]			r_const__11;
reg [31:0]			r_const__10;
reg [31:0]			r_const__9;
reg [31:0]			r_const__8;
reg [31:0]			r_const__7;
reg [31:0]			r_const__6;
reg [31:0]			r_const__5;
reg [31:0]			r_const__4;
reg [31:0]			r_const__3;
reg [31:0]			r_const__2;
reg [31:0]			r_const__1;
reg [31:0]			r_const__0;



reg [12:0]			r_counter;
reg [16-1:0] r_absorb_read_counter;
reg [16-1:0] r_absorb_write_counter;
reg [3:0]			r_state;
reg	r_toggle;

// Skeleton program states
parameter [3:0] ERROR_ST = 4'b0000, 
				READ1_ST = 4'b0001, 
				READ2_ST = 4'b0010, 
				READ3_ST = 4'b0011, 
				READ4_ST = 4'b0100, 
				READ5_ST = 4'b0101, 
				RESET_MEM_ST = 4'b0110,
				CALC_ST = 4'b1000, 
				DONE1_ST = 4'b1001, 
				DONE2_ST = 4'b1010, 
				DONE3_ST = 4'b1011, 
				DONE4_ST = 4'b1100, 
				DONE5_ST = 4'b1101, 
				DONE6_ST = 4'b1110;

// Instantiate lookup memories
dual_port_mem_zz u_fres_up(clk, constants, {3'b0, fresIndex}, {3'b0, mem_layer, r_counter[6:0]}, wren_fres_up, mem_fres_up);
dual_port_mem_yy u_fres_down(clk, constants, {3'b0, fresIndex}, {3'b0, mem_layer, r_counter[6:0]}, wren_fres_down, mem_fres_down);
dual_port_mem_xx u_sint(clk, constants, tindex, {mem_layer, r_counter[9:0]}, wren_sint, mem_sint);
dual_port_mem_ww u_cost(clk, constants, tindex, {mem_layer, r_counter[9:0]}, wren_cost, mem_cost);

// Reduce size of absorption matrix
dual absorptionMatrix(   .clk (clk), .data(absorb_data_mux[35:0]), 
                         .rdaddress(absorb_rdaddress_mux), .wraddress(absorb_wraddress_mux), 
                         .wren(absorb_wren_mux), .q(absorb_q[35:0]));
dual2 absorptionMatrix2(   .clk (clk), .data(absorb_data_mux[53:36]), 
                         .rdaddress(absorb_rdaddress_mux), .wraddress(absorb_wraddress_mux), 
                         .wren(absorb_wren_mux), .q(absorb_q[53:36]));
dual3 absorptionMatrix3(   .clk (clk), .data(absorb_data_mux[61:54]), 
                         .rdaddress(absorb_rdaddress_mux), .wraddress(absorb_wraddress_mux), 
                         .wren(absorb_wren_mux), .q(absorb_q[61:54]));

						 
						 //
						 //peter m test since absorb_q not defined for 63:62
						 assign absorb_q[63:62] = 2'b00;
						 
						 
PhotonCalculator u_calc (
	.clock(clk), .reset(reset_calculator), .enable(enable),

	// CONSTANTS
	.total_photons(r_const__0),
	
	.randseed1(r_const__19), .randseed2(r_const__20), .randseed3(r_const__21), .randseed4(r_const__22), .randseed5(r_const__23),

	//Because it is in the module:
	.initialWeight(32'b00000000000111010100101111111111),
	
	//   Mover
	.OneOver_MutMaxrad_0(r_const__32), .OneOver_MutMaxrad_1(r_const__33), .OneOver_MutMaxrad_2(r_const__34), .OneOver_MutMaxrad_3(r_const__35), .OneOver_MutMaxrad_4(r_const__36), .OneOver_MutMaxrad_5(r_const__37),
	.OneOver_MutMaxdep_0(r_const__38), .OneOver_MutMaxdep_1(r_const__39), .OneOver_MutMaxdep_2(r_const__40), .OneOver_MutMaxdep_3(r_const__41), .OneOver_MutMaxdep_4(r_const__42), .OneOver_MutMaxdep_5(r_const__43),
	.OneOver_Mut_0(r_const__26), .OneOver_Mut_1(r_const__27), .OneOver_Mut_2(r_const__28), .OneOver_Mut_3(r_const__29), .OneOver_Mut_4(r_const__30), .OneOver_Mut_5(r_const__31),

	//   BoundaryChecker
	.z1_0(r_const__50), .z1_1(r_const__51), .z1_2(r_const__52), .z1_3(r_const__53), .z1_4(r_const__54), .z1_5(r_const__55),
	.z0_0(r_const__44), .z0_1(r_const__45), .z0_2(r_const__46), .z0_3(r_const__47), .z0_4(r_const__48), .z0_5(r_const__49),
	.mut_0(32'b00000000000000000000000000000000), .mut_1(r_const__2), .mut_2(r_const__3), .mut_3(r_const__4), .mut_4(r_const__5), .mut_5(r_const__6),
	.maxDepth_over_maxRadius(r_const__1),

	//   Hop (no constants)

	//   Scatterer Reflector Wrapper
	.down_niOverNt_1(r_const__69), .down_niOverNt_2(r_const__70), .down_niOverNt_3(r_const__71), .down_niOverNt_4(r_const__72), .down_niOverNt_5(r_const__73),
	.up_niOverNt_1(r_const__75), .up_niOverNt_2(r_const__76), .up_niOverNt_3(r_const__77), .up_niOverNt_4(r_const__78), .up_niOverNt_5(r_const__79),
	.down_niOverNt_2_1({r_const__81,r_const__87}), .down_niOverNt_2_2({r_const__82,r_const__88}), .down_niOverNt_2_3({r_const__83,r_const__89}), .down_niOverNt_2_4({r_const__84,r_const__90}), .down_niOverNt_2_5({r_const__85,r_const__91}),
	.up_niOverNt_2_1({r_const__93,r_const__99}), .up_niOverNt_2_2({r_const__94,r_const__100}), .up_niOverNt_2_3({r_const__95,r_const__101}), .up_niOverNt_2_4({r_const__96,r_const__102}), .up_niOverNt_2_5({r_const__97,r_const__103}),
	.downCritAngle_0(r_const__7), .downCritAngle_1(r_const__8), .downCritAngle_2(r_const__9), .downCritAngle_3(r_const__10), .downCritAngle_4(r_const__11),
	.upCritAngle_0(r_const__13), .upCritAngle_1(r_const__14), .upCritAngle_2(r_const__15), .upCritAngle_3(r_const__16), .upCritAngle_4(r_const__17),
	.muaFraction1(r_const__57), .muaFraction2(r_const__58), .muaFraction3(r_const__59), .muaFraction4(r_const__60), .muaFraction5(r_const__61),
	  // Interface to memory look-up
	    // From Memories
	.up_rFresnel(mem_fres_up), .down_rFresnel(mem_fres_down), .sint(mem_sint), .cost(mem_cost),
		// To Memories
	.tindex(tindex), .fresIndex(fresIndex),

	// DeadOrAlive (no Constants)

	// Absorber
	.absorb_data(absorb_data), .absorb_rdaddress(absorb_rdaddress), .absorb_wraddress(absorb_wraddress), 
	.absorb_wren(absorb_wren), .absorb_q(absorb_q),

	// Done signal
	.done(done)
	);

// Mux to read the absorbtion array
always @(r_state or done or r_absorb_read_counter or r_absorb_write_counter or absorb_wraddress or absorb_data or absorb_rdaddress or absorb_data or absorb_wren )
begin
	if(r_state == RESET_MEM_ST)
	begin
		absorb_wren_mux = 1'b1;
		absorb_data_mux = 64'b0;
		absorb_rdaddress_mux = r_absorb_read_counter;
		absorb_wraddress_mux = r_absorb_write_counter;
	end
	else if(done == 1'b1)
	begin
		absorb_rdaddress_mux = r_absorb_read_counter;
		absorb_wraddress_mux = absorb_wraddress;
		absorb_data_mux = absorb_data;
		absorb_wren_mux = 1'b0;
	end
	else
	begin
		absorb_rdaddress_mux = absorb_rdaddress;
		absorb_wraddress_mux = absorb_wraddress;
		absorb_data_mux = absorb_data;
		absorb_wren_mux = absorb_wren;
	end
end

// Skeleton SW/HW interface
//  1.  Read constants
//  2.  Wait for completion
//  3.  Write data back
always @(r_state or r_absorb_read_counter or r_absorb_write_counter or result or r_toggle or r_counter or read_constants or constants or done
			or inc_result or mem_cost or mem_sint or absorb_q
			or r_const__103
			or r_const__102
			or r_const__101
			or r_const__100
			or r_const__99
			or r_const__98
			or r_const__97
			or r_const__96
			or r_const__95
			or r_const__94
			or r_const__93
			or r_const__92
			or r_const__91
			or r_const__90
			or r_const__89
			or r_const__88
			or r_const__87
			or r_const__86
			or r_const__85
			or r_const__84
			or r_const__83
			or r_const__82
			or r_const__81
			or r_const__80
			or r_const__79
			or r_const__78
			or r_const__77
			or r_const__76
			or r_const__75
			or r_const__74
			or r_const__73
			or r_const__72
			or r_const__71
			or r_const__70
			or r_const__69
			or r_const__68
			or r_const__67
			or r_const__66
			or r_const__65
			or r_const__64
			or r_const__63
			or r_const__62
			or r_const__61
			or r_const__60
			or r_const__59
			or r_const__58
			or r_const__57
			or r_const__56
			or r_const__55
			or r_const__54
			or r_const__53
			or r_const__52
			or r_const__51
			or r_const__50
			or r_const__49
			or r_const__48
			or r_const__47
			or r_const__46
			or r_const__45
			or r_const__44
			or r_const__43
			or r_const__42
			or r_const__41
			or r_const__40
			or r_const__39
			or r_const__38
			or r_const__37
			or r_const__36
			or r_const__35
			or r_const__34
			or r_const__33
			or r_const__32
			or r_const__31
			or r_const__30
			or r_const__29
			or r_const__28
			or r_const__27
			or r_const__26
			or r_const__25
			or r_const__24
			or r_const__23
			or r_const__22
			or r_const__21
			or r_const__20
			or r_const__19
			or r_const__18
			or r_const__17
			or r_const__16
			or r_const__15
			or r_const__14
			or r_const__13
			or r_const__12
			or r_const__11
			or r_const__10
			or r_const__9
			or r_const__8
			or r_const__7
			or r_const__6
			or r_const__5
			or r_const__4
			or r_const__3
			or r_const__2
			or r_const__1
			or r_const__0) begin
		// Initialize data
		//for(i = 0; i < 104; i = i + 1) begin
		//	c_const[i] = r_const[i];
		//end
		begin
//c_const__103 = r_const__103;
c_const__102 = r_const__102;
c_const__101 = r_const__101;
c_const__100 = r_const__100;
c_const__99 = r_const__99;
c_const__98 = r_const__98;
c_const__97 = r_const__97;
c_const__96 = r_const__96;
c_const__95 = r_const__95;
c_const__94 = r_const__94;
c_const__93 = r_const__93;
c_const__92 = r_const__92;
c_const__91 = r_const__91;
c_const__90 = r_const__90;
c_const__89 = r_const__89;
c_const__88 = r_const__88;
c_const__87 = r_const__87;
c_const__86 = r_const__86;
c_const__85 = r_const__85;
c_const__84 = r_const__84;
c_const__83 = r_const__83;
c_const__82 = r_const__82;
c_const__81 = r_const__81;
c_const__80 = r_const__80;
c_const__79 = r_const__79;
c_const__78 = r_const__78;
c_const__77 = r_const__77;
c_const__76 = r_const__76;
c_const__75 = r_const__75;
c_const__74 = r_const__74;
c_const__73 = r_const__73;
c_const__72 = r_const__72;
c_const__71 = r_const__71;
c_const__70 = r_const__70;
c_const__69 = r_const__69;
c_const__68 = r_const__68;
c_const__67 = r_const__67;
c_const__66 = r_const__66;
c_const__65 = r_const__65;
c_const__64 = r_const__64;
c_const__63 = r_const__63;
c_const__62 = r_const__62;
c_const__61 = r_const__61;
c_const__60 = r_const__60;
c_const__59 = r_const__59;
c_const__58 = r_const__58;
c_const__57 = r_const__57;
c_const__56 = r_const__56;
c_const__55 = r_const__55;
c_const__54 = r_const__54;
c_const__53 = r_const__53;
c_const__52 = r_const__52;
c_const__51 = r_const__51;
c_const__50 = r_const__50;
c_const__49 = r_const__49;
c_const__48 = r_const__48;
c_const__47 = r_const__47;
c_const__46 = r_const__46;
c_const__45 = r_const__45;
c_const__44 = r_const__44;
c_const__43 = r_const__43;
c_const__42 = r_const__42;
c_const__41 = r_const__41;
c_const__40 = r_const__40;
c_const__39 = r_const__39;
c_const__38 = r_const__38;
c_const__37 = r_const__37;
c_const__36 = r_const__36;
c_const__35 = r_const__35;
c_const__34 = r_const__34;
c_const__33 = r_const__33;
c_const__32 = r_const__32;
c_const__31 = r_const__31;
c_const__30 = r_const__30;
c_const__29 = r_const__29;
c_const__28 = r_const__28;
c_const__27 = r_const__27;
c_const__26 = r_const__26;
c_const__25 = r_const__25;
c_const__24 = r_const__24;
c_const__23 = r_const__23;
c_const__22 = r_const__22;
c_const__21 = r_const__21;
c_const__20 = r_const__20;
c_const__19 = r_const__19;
c_const__18 = r_const__18;
c_const__17 = r_const__17;
c_const__16 = r_const__16;
c_const__15 = r_const__15;
c_const__14 = r_const__14;
c_const__13 = r_const__13;
c_const__12 = r_const__12;
c_const__11 = r_const__11;
c_const__10 = r_const__10;
c_const__9 = r_const__9;
c_const__8 = r_const__8;
c_const__7 = r_const__7;
c_const__6 = r_const__6;
c_const__5 = r_const__5;
c_const__4 = r_const__4;
c_const__3 = r_const__3;
c_const__2 = r_const__2;
c_const__1 = r_const__1;
c_const__0 = r_const__0;
		end
		/*
		//honourary c_const__103 = r_const__103
		c_counter = r_counter;
		c_absorb_read_counter = r_absorb_read_counter;
		c_result = result;
		c_calc_in_progress = 1'b0;
		c_state = r_state;
		wren_fres_up = 1'b0;
		wren_fres_down = 1'b0;
		wren_sint = 1'b0;
		wren_cost = 1'b0;
		c_absorb_write_counter = r_absorb_write_counter;
		c_toggle = r_toggle;
		
		mem_layer = r_counter[12:10];
*/
		// Determine next state and which data changes
		case(r_state)
			//ERROR_ST:
			READ1_ST:
				begin			
					if(read_constants)
						begin
						// peter m redoing this to a shift register r_const 104 will shift to r_const 103 etc etc
						// if its in the read_constants state
						//	c_const[r_counter] = constants;
						c_counter = r_counter + 13'b00000000000001;
						c_const__103 = constants;
						//pm preventing latches
						c_absorb_read_counter = r_absorb_read_counter;
						c_result = result;
						c_calc_in_progress = 1'b0;
						c_state = r_state;
						wren_fres_up = 1'b0;
						wren_fres_down = 1'b0;
						wren_sint = 1'b0;
						wren_cost = 1'b0;
						c_absorb_write_counter = r_absorb_write_counter;
						c_toggle = r_toggle;
						mem_layer = r_counter[12:10];
						end
					else
						begin
						c_const__103 = r_const__103;
							if(r_counter >= 104) 
								begin
									c_counter = 13'b0000000000000;
									c_state = READ2_ST;
									//preventing latches
									
									c_absorb_read_counter = r_absorb_read_counter;
									c_result = result;
									c_calc_in_progress = 1'b0;
									wren_fres_up = 1'b0;
									wren_fres_down = 1'b0;
									wren_sint = 1'b0;
									wren_cost = 1'b0;
									c_absorb_write_counter = r_absorb_write_counter;
									c_toggle = r_toggle;
									mem_layer = r_counter[12:10];
									
								end
								else
								begin
								c_counter = r_counter;
								c_state = r_state;
								
								//preventing latches
								c_absorb_read_counter = r_absorb_read_counter;
								c_result = result;
								c_calc_in_progress = 1'b0;
								wren_fres_up = 1'b0;
								wren_fres_down = 1'b0;
								wren_sint = 1'b0;
								wren_cost = 1'b0;
								c_absorb_write_counter = r_absorb_write_counter;
								c_toggle = r_toggle;
								mem_layer = r_counter[12:10];
								end
						end
				end
			READ2_ST:
				begin		
					mem_layer = r_counter[9:7];
					if(read_constants)
						begin
							wren_fres_up = 1'b1;
							c_counter = r_counter + 13'b00000000000001;
						//prevent latches
						
						c_const__103 = r_const__103;
						c_absorb_read_counter = r_absorb_read_counter;
						c_result = result;
						c_calc_in_progress = 1'b0;
						c_state = r_state;
						wren_fres_down = 1'b0;
						wren_sint = 1'b0;
						wren_cost = 1'b0;
						c_absorb_write_counter = r_absorb_write_counter;
						c_toggle = r_toggle;
		
		
						end
					else
						begin
							if(r_counter >= 5*128) 
								begin
									c_counter = 13'b0000000000000;
									c_state = READ3_ST;
									
					c_const__103 = r_const__103;
				
		c_absorb_read_counter = r_absorb_read_counter;
		c_result = result;
		c_calc_in_progress = 1'b0;
	
		wren_fres_up = 1'b0;
		wren_fres_down = 1'b0;
		wren_sint = 1'b0;
		wren_cost = 1'b0;
		c_absorb_write_counter = r_absorb_write_counter;
		c_toggle = r_toggle;
	
								end
								else
								begin
		c_counter = r_counter;
		c_const__103 = r_const__103;
		
		c_absorb_read_counter = r_absorb_read_counter;
		c_result = result;
		c_calc_in_progress = 1'b0;
		c_state = r_state;
		wren_fres_up = 1'b0;
		wren_fres_down = 1'b0;
		wren_sint = 1'b0;
		wren_cost = 1'b0;
		c_absorb_write_counter = r_absorb_write_counter;
		c_toggle = r_toggle;
		
	
								end
						end
				end
			READ3_ST:
				begin
			mem_layer = r_counter[9:7];
			 c_const__103 = r_const__103;
	
		c_absorb_read_counter = r_absorb_read_counter;
		c_result = result;
		c_calc_in_progress = 1'b0;
		
		wren_fres_up = 1'b0;
		
		wren_sint = 1'b0;
		wren_cost = 1'b0;
		c_absorb_write_counter = r_absorb_write_counter;
		c_toggle = r_toggle;
		
	//	mem_layer = r_counter[12:10];
					if(read_constants)
						begin
							wren_fres_down = 1'b1;
							c_counter = r_counter + 13'b00000000000001;
							c_state = r_state;
						end
					else
						begin
							if(r_counter >= 5*128) 
								begin
									c_counter = 13'b0000000000000;
									c_state = READ4_ST;
									wren_fres_down = 1'b0;
								end
								else
								begin
								c_counter = r_counter;
								c_state = r_state;
								wren_fres_down = 1'b0;
								end
						end
				end
			READ4_ST:
				begin
		c_const__103 = r_const__103;
		//c_counter = r_counter;
		c_absorb_read_counter = r_absorb_read_counter;
		c_result = result;
		c_calc_in_progress = 1'b0;
		//c_state = r_state;
		wren_fres_up = 1'b0;
		wren_fres_down = 1'b0;
		wren_sint = 1'b0;
		//wren_cost = 1'b0;
		c_absorb_write_counter = r_absorb_write_counter;
		c_toggle = r_toggle;
		
		mem_layer = r_counter[12:10];
		
		
					if(read_constants)
						begin
							wren_cost = 1'b1;
							c_counter = r_counter + 13'b00000000000001;
							c_state = r_state;
						end
					else
						begin
							if(r_counter >= 13'b1010000000000) //5*1024 = 5120
								begin
									c_counter = 13'b0000000000000000000000000;
									c_state = READ5_ST;
									wren_cost = 1'b0;
								end
								else
								begin
								c_counter = r_counter;
								c_state = r_state;
								wren_cost = 1'b0;
								end
						end
				end
			READ5_ST:
				begin			
		c_const__103 = r_const__103;
		//c_counter = r_counter;
		//c_absorb_read_counter = r_absorb_read_counter;
		c_result = result;
		c_calc_in_progress = 1'b0;
		//c_state = r_state;
		wren_fres_up = 1'b0;
		wren_fres_down = 1'b0;
		//wren_sint = 1'b0;
		wren_cost = 1'b0;
		c_absorb_write_counter = r_absorb_write_counter;
		c_toggle = r_toggle;
		
		mem_layer = r_counter[12:10];
		
		
					if(read_constants)
						begin
							wren_sint = 1'b1;
							c_counter = r_counter + 13'b00000000000000000000000001;
							c_state = r_state;
							c_absorb_read_counter = r_absorb_read_counter;
					end
					else
						begin
							if(r_counter >= 13'b1010000000000) //5*1024 = 5120
								begin
									c_counter = 13'b0000000000000000000000000;
									c_absorb_read_counter = 16'b0000000000000000000000000; //use to be 13 bit. Error in odin
									c_state = RESET_MEM_ST;
									wren_sint = 1'b0;
								end
								else
								begin
								c_counter = r_counter;
								c_absorb_read_counter = r_absorb_read_counter;
									c_state = r_state;
									wren_sint = 1'b0;
								end
						end
				end
			RESET_MEM_ST:
				begin
		 c_const__103 = r_const__103;
	//	c_counter = r_counter;
		c_absorb_read_counter = r_absorb_read_counter;
		c_result = result;
		//c_calc_in_progress = 1'b0;
		//c_state = r_state;
		wren_fres_up = 1'b0;
		wren_fres_down = 1'b0;
		wren_sint = 1'b0;
		wren_cost = 1'b0;
		//c_absorb_write_counter = r_absorb_write_counter;
		//c_toggle = r_toggle;
		
		mem_layer = r_counter[12:10];
				

				
				
				c_counter = r_counter;
				
				    c_toggle = 1'b0;
					c_calc_in_progress = 1'b1;
					c_absorb_write_counter = r_absorb_write_counter + 16'b0000000000000001;
					if(r_absorb_write_counter == 16'b1111111111111111)
					begin
						c_state = CALC_ST;
					end
					else
					begin
					c_state = r_state;
					
					end
				end
			CALC_ST:
				begin
		c_const__103 = r_const__103;
		//c_counter = r_counter;
		c_absorb_read_counter = r_absorb_read_counter;
		c_result = result;
		//c_calc_in_progress = 1'b0;
		//c_state = r_state;
		wren_fres_up = 1'b0;
		wren_fres_down = 1'b0;
		wren_sint = 1'b0;
		wren_cost = 1'b0;
		c_absorb_write_counter = r_absorb_write_counter;
		//c_toggle = r_toggle;
		
		mem_layer = r_counter[12:10];
				
				
				
					if(done == 1'b0)
						begin
							c_calc_in_progress = 1'b1;
							c_toggle = 1'b0;
							c_counter = r_counter;
							c_state = r_state;
								
						end
					else
						begin
							c_toggle = 1'b0;
							c_calc_in_progress = 1'b0;
							c_state = DONE6_ST;
							c_counter = 13'b0000000000000;
						end
				end
		// DEBUG STATES BEGIN
		
			DONE1_ST:
				begin
		c_const__103 = r_const__103;
		//c_counter = r_counter;
		c_absorb_read_counter = r_absorb_read_counter;
		//c_result = result;
		c_calc_in_progress = 1'b0;
		//c_state = r_state;
		wren_fres_up = 1'b0;
		wren_fres_down = 1'b0;
		wren_sint = 1'b0;
		wren_cost = 1'b0;
		c_absorb_write_counter = r_absorb_write_counter;
		c_toggle = r_toggle;
		
		mem_layer = r_counter[12:10];
				
				
					c_result = r_const__103;				
					//original -c_result = {32'b0,r_const[r_counter]};
					if(inc_result)
						begin
						if(r_counter >= 13'b0000010001100) //104
							begin
								c_counter = 13'b0000000000000;
								c_state = DONE2_ST;
							end
						else
							begin
								c_counter = r_counter + 13'b0000000000001;
								c_state = DONE1_ST;
							end
						end
							
						else
						begin
						if(r_counter >= 13'b0000010001100) //104
							begin
								c_counter = 13'b0;
								c_state = DONE2_ST;
							end
						else
							begin
								c_state = DONE1_ST;
								c_counter = r_counter;
							end
					end
				end
			DONE2_ST:
				begin
		c_const__103 = r_const__103;
		//c_counter = r_counter;
		c_absorb_read_counter = r_absorb_read_counter;
		//c_result = result;
		c_calc_in_progress = 1'b0;
		//c_state = r_state;
		wren_fres_up = 1'b0;
		wren_fres_down = 1'b0;
		wren_sint = 1'b0;
		wren_cost = 1'b0;
		c_absorb_write_counter = r_absorb_write_counter;
		c_toggle = r_toggle;
		
		//mem_layer = r_counter[12:10];
			
				
					mem_layer = r_counter[9:7];
					//c_result = {32'b00000000000000000000000000000000,mem_fres_up};
					c_result = 32'b0;
						if(inc_result)
						begin
								c_counter = r_counter + 13'b0000000000001;
								c_state = DONE1_ST;
						end
						else
						begin
						if(r_counter >= 13'b0000010001100) //104
							begin
								c_counter = 13'b0000000000000;
								c_state = DONE2_ST;
							end
						else
							begin
								c_counter = r_counter;
								c_state = r_state;
							end
						end
				end
			DONE3_ST:
				begin
		c_const__103 = r_const__103;
		//c_counter = r_counter;
		c_absorb_read_counter = r_absorb_read_counter;
		//c_result = result;
		c_calc_in_progress = 1'b0;
		//c_state = r_state;
		wren_fres_up = 1'b0;
		wren_fres_down = 1'b0;
		wren_sint = 1'b0;
		wren_cost = 1'b0;
		c_absorb_write_counter = r_absorb_write_counter;
		c_toggle = r_toggle;
		
		//mem_layer = r_counter[12:10];
			
				
				
				
					mem_layer = r_counter[9:7];
					//c_result = {32'b00000000000000000000000000000000,mem_fres_down};
					c_result = 32'b0;
					
					if(inc_result)
						begin
							// stub, write constants back to see if read in properly
							c_counter = r_counter + 13'b0000000000001;
							c_state = DONE3_ST;
						end
						
					else
						begin
						if(r_counter >= 13'b0001010000000) //5*128 = 640
							begin
								c_counter = 13'b0000000000000;
								c_state = DONE4_ST;
							end
						else
							begin
								c_counter = r_counter;
								c_state = DONE3_ST;
							end
						
						
						
						
						end
				end
			DONE4_ST:
				begin
				
				
				
						c_const__103 = r_const__103;
		//c_counter = r_counter;
		c_absorb_read_counter = r_absorb_read_counter;
		//c_result = result;
		c_calc_in_progress = 1'b0;
		//c_state = r_state;
		wren_fres_up = 1'b0;
		wren_fres_down = 1'b0;
		wren_sint = 1'b0;
		wren_cost = 1'b0;
		c_absorb_write_counter = r_absorb_write_counter;
		c_toggle = r_toggle;
		
		mem_layer = r_counter[12:10];
		
		
		
					c_result = mem_cost;
					
				if(inc_result)
						begin
							// stub, write constants back to see if read in properly
						c_counter = r_counter + 13'b0000000000001;
						c_state = DONE4_ST;
						end
				else
				begin
					if(r_counter >= 13'b1010000000000) //5*1024 = 5120
						begin
							c_counter = 13'b0000000000000;
							c_state = DONE5_ST;
						end
						
						else
						begin
							c_state = DONE4_ST;
							c_counter = r_counter;
						end
					end
				end
			DONE5_ST:
				begin
				
		c_const__103 = r_const__103;
		//c_counter = r_counter;
		c_absorb_read_counter = r_absorb_read_counter;
		//c_result = result;
		c_calc_in_progress = 1'b0;
		//c_state = r_state;
		wren_fres_up = 1'b0;
		wren_fres_down = 1'b0;
		wren_sint = 1'b0;
		wren_cost = 1'b0;
		c_absorb_write_counter = r_absorb_write_counter;
		c_toggle = r_toggle;
		
		mem_layer = r_counter[12:10];
		
			
					c_result = mem_sint;
				
					if(r_counter >= 13'b1010000000000) //5*1024 = 5120
						begin
							c_counter = 13'b0000000000000;
							c_state = DONE6_ST;
						end
						else
						begin
						c_state = DONE5_ST;
						if(inc_result)
						begin
							// stub, write constants back to see if read in properly
							c_counter = r_counter + 13'b00000000000001;
						end
						else
						begin
						c_counter = r_counter;
						end
				end
				end

			// DEBUG STATES END*/
			DONE6_ST:
				begin
		c_const__103 = r_const__103;
		c_counter = r_counter;
		//c_absorb_read_counter = r_absorb_read_counter;
		//c_result = result;
		c_calc_in_progress = 1'b0;
		//c_state = r_state;
		wren_fres_up = 1'b0;
		wren_fres_down = 1'b0;
		wren_sint = 1'b0;
		wren_cost = 1'b0;
		c_absorb_write_counter = r_absorb_write_counter;
		//c_toggle = r_toggle;
		
		mem_layer = r_counter[12:10];
				
				
									c_state = DONE6_ST;

				
					if(r_toggle == 1'b0)
					begin
						c_result = absorb_q[63:32];
					//	c_state = r_state;
						end
					else
					begin
						c_result = absorb_q[31:0];
					//	c_state = r_state;
					end

					if(inc_result)
						begin
							if(r_toggle == 1'b0)
							begin
								c_toggle = 1'b1;
								c_absorb_read_counter = r_absorb_read_counter ;
							end
							else
							begin
								c_toggle = 1'b0;
								c_absorb_read_counter = r_absorb_read_counter + 16'b01;
							end
						end
					else
					begin
					c_absorb_read_counter = r_absorb_read_counter;
					c_toggle= r_toggle;

					end
				//	c_state = DONE6_ST;
					end
				
			default:
				begin
			c_state = ERROR_ST;
		c_const__103 = r_const__103;
		c_counter = r_counter;
		c_absorb_read_counter = r_absorb_read_counter;
		c_result = result;
		c_calc_in_progress = 1'b0;
	//	c_state = r_state;
		wren_fres_up = 1'b0;
		wren_fres_down = 1'b0;
		wren_sint = 1'b0;
		wren_cost = 1'b0;
		c_absorb_write_counter = r_absorb_write_counter;
		c_toggle = r_toggle;
		
		mem_layer = r_counter[12:10];
				end
		endcase
	end // FSM always



// Latch Data
always @(posedge clk)
	begin
		if(reset)
			begin
	r_counter <= 13'b0000000000000000000000000;
r_const__103 <= 32'b00000000000000000000000000000000;
r_const__102 <= 32'b00000000000000000000000000000000;
r_const__101 <= 32'b00000000000000000000000000000000;
r_const__100 <= 32'b00000000000000000000000000000000;
r_const__99 <= 32'b00000000000000000000000000000000;
r_const__98 <= 32'b00000000000000000000000000000000;
r_const__97 <= 32'b00000000000000000000000000000000;
r_const__96 <= 32'b00000000000000000000000000000000;
r_const__95 <= 32'b00000000000000000000000000000000;
r_const__94 <= 32'b00000000000000000000000000000000;
r_const__93 <= 32'b00000000000000000000000000000000;
r_const__92 <= 32'b00000000000000000000000000000000;
r_const__91 <= 32'b00000000000000000000000000000000;
r_const__90 <= 32'b00000000000000000000000000000000;
r_const__89 <= 32'b00000000000000000000000000000000;
r_const__88 <= 32'b00000000000000000000000000000000;
r_const__87 <= 32'b00000000000000000000000000000000;
r_const__86 <= 32'b00000000000000000000000000000000;
r_const__85 <= 32'b00000000000000000000000000000000;
r_const__84 <= 32'b00000000000000000000000000000000;
r_const__83 <= 32'b00000000000000000000000000000000;
r_const__82 <= 32'b00000000000000000000000000000000;
r_const__81 <= 32'b00000000000000000000000000000000;
r_const__80 <= 32'b00000000000000000000000000000000;
r_const__79 <= 32'b00000000000000000000000000000000;
r_const__78 <= 32'b00000000000000000000000000000000;
r_const__77 <= 32'b00000000000000000000000000000000;
r_const__76 <= 32'b00000000000000000000000000000000;
r_const__75 <= 32'b00000000000000000000000000000000;
r_const__74 <= 32'b00000000000000000000000000000000;
r_const__73 <= 32'b00000000000000000000000000000000;
r_const__72 <= 32'b00000000000000000000000000000000;
r_const__71 <= 32'b00000000000000000000000000000000;
r_const__70 <= 32'b00000000000000000000000000000000;
r_const__69 <= 32'b00000000000000000000000000000000;
r_const__68 <= 32'b00000000000000000000000000000000;
r_const__67 <= 32'b00000000000000000000000000000000;
r_const__66 <= 32'b00000000000000000000000000000000;
r_const__65 <= 32'b00000000000000000000000000000000;
r_const__64 <= 32'b00000000000000000000000000000000;
r_const__63 <= 32'b00000000000000000000000000000000;
r_const__62 <= 32'b00000000000000000000000000000000;
r_const__61 <= 32'b00000000000000000000000000000000;
r_const__60 <= 32'b00000000000000000000000000000000;
r_const__59 <= 32'b00000000000000000000000000000000;
r_const__58 <= 32'b00000000000000000000000000000000;
r_const__57 <= 32'b00000000000000000000000000000000;
r_const__56 <= 32'b00000000000000000000000000000000;
r_const__55 <= 32'b00000000000000000000000000000000;
r_const__54 <= 32'b00000000000000000000000000000000;
r_const__53 <= 32'b00000000000000000000000000000000;
r_const__52 <= 32'b00000000000000000000000000000000;
r_const__51 <= 32'b00000000000000000000000000000000;
r_const__50 <= 32'b00000000000000000000000000000000;
r_const__49 <= 32'b00000000000000000000000000000000;
r_const__48 <= 32'b00000000000000000000000000000000;
r_const__47 <= 32'b00000000000000000000000000000000;
r_const__46 <= 32'b00000000000000000000000000000000;
r_const__45 <= 32'b00000000000000000000000000000000;
r_const__44 <= 32'b00000000000000000000000000000000;
r_const__43 <= 32'b00000000000000000000000000000000;
r_const__42 <= 32'b00000000000000000000000000000000;
r_const__41 <= 32'b00000000000000000000000000000000;
r_const__40 <= 32'b00000000000000000000000000000000;
r_const__39 <= 32'b00000000000000000000000000000000;
r_const__38 <= 32'b00000000000000000000000000000000;
r_const__37 <= 32'b00000000000000000000000000000000;
r_const__36 <= 32'b00000000000000000000000000000000;
r_const__35 <= 32'b00000000000000000000000000000000;
r_const__34 <= 32'b00000000000000000000000000000000;
r_const__33 <= 32'b00000000000000000000000000000000;
r_const__32 <= 32'b00000000000000000000000000000000;
r_const__31 <= 32'b00000000000000000000000000000000;
r_const__30 <= 32'b00000000000000000000000000000000;
r_const__29 <= 32'b00000000000000000000000000000000;
r_const__28 <= 32'b00000000000000000000000000000000;
r_const__27 <= 32'b00000000000000000000000000000000;
r_const__26 <= 32'b00000000000000000000000000000000;
r_const__25 <= 32'b00000000000000000000000000000000;
r_const__24 <= 32'b00000000000000000000000000000000;
r_const__23 <= 32'b00000000000000000000000000000000;
r_const__22 <= 32'b00000000000000000000000000000000;
r_const__21 <= 32'b00000000000000000000000000000000;
r_const__20 <= 32'b00000000000000000000000000000000;
r_const__19 <= 32'b00000000000000000000000000000000;
r_const__18 <= 32'b00000000000000000000000000000000;
r_const__17 <= 32'b00000000000000000000000000000000;
r_const__16 <= 32'b00000000000000000000000000000000;
r_const__15 <= 32'b00000000000000000000000000000000;
r_const__14 <= 32'b00000000000000000000000000000000;
r_const__13 <= 32'b00000000000000000000000000000000;
r_const__12 <= 32'b00000000000000000000000000000000;
r_const__11 <= 32'b00000000000000000000000000000000;
r_const__10 <= 32'b00000000000000000000000000000000;
r_const__9 <= 32'b00000000000000000000000000000000;
r_const__8 <= 32'b00000000000000000000000000000000;
r_const__7 <= 32'b00000000000000000000000000000000;
r_const__6 <= 32'b00000000000000000000000000000000;
r_const__5 <= 32'b00000000000000000000000000000000;
r_const__4 <= 32'b00000000000000000000000000000000;
r_const__3 <= 32'b00000000000000000000000000000000;
r_const__2 <= 32'b00000000000000000000000000000000;
r_const__1 <= 32'b00000000000000000000000000000000;
r_const__0 <= 32'b00000000000000000000000000000000;

				r_state <= READ1_ST;
				result <= 32'b00000000000000000000000000000000;
				calc_in_progress <= 1'b0;
				r_absorb_read_counter <= 16'b0000000000000000;
				enable <= 1'b0;
				r_absorb_write_counter <= 16'b0000000000000000;
				reset_calculator <= 1'b1;
				r_toggle <= 1'b0;
			end
		else
			begin
					r_counter <= c_counter;
			if (c_state == READ1_ST)
				
				//for(i = 0; i < 104; i = i + 1) begin
				//	r_const[i] <= c_const[i];
				//end
				begin
				
				//shift register implementation for read-in constant state

//first one is from counter				
r_const__103 <= c_const__103;
// all others shift
r_const__102 <= r_const__103;
r_const__101 <= r_const__102;
r_const__100 <= r_const__101;
r_const__99 <= r_const__100;
r_const__98 <= r_const__99;
r_const__97 <= r_const__98;
r_const__96 <= r_const__97;
r_const__95 <= r_const__96;
r_const__94 <= r_const__95;
r_const__93 <= r_const__94;
r_const__92 <= r_const__93;
r_const__91 <= r_const__92;
r_const__90 <= r_const__91;
r_const__89 <= r_const__90;
r_const__88 <= r_const__89;
r_const__87 <= r_const__88;
r_const__86 <= r_const__87;
r_const__85 <= r_const__86;
r_const__84 <= r_const__85;
r_const__83 <= r_const__84;
r_const__82 <= r_const__83;
r_const__81 <= r_const__82;
r_const__80 <= r_const__81;
r_const__79 <= r_const__80;
r_const__78 <= r_const__79;
r_const__77 <= r_const__78;
r_const__76 <= r_const__77;
r_const__75 <= r_const__76;
r_const__74 <= r_const__75;
r_const__73 <= r_const__74;
r_const__72 <= r_const__73;
r_const__71 <= r_const__72;
r_const__70 <= r_const__71;
r_const__69 <= r_const__70;
r_const__68 <= r_const__69;
r_const__67 <= r_const__68;
r_const__66 <= r_const__67;
r_const__65 <= r_const__66;
r_const__64 <= r_const__65;
r_const__63 <= r_const__64;
r_const__62 <= r_const__63;
r_const__61 <= r_const__62;
r_const__60 <= r_const__61;
r_const__59 <= r_const__60;
r_const__58 <= r_const__59;
r_const__57 <= r_const__58;
r_const__56 <= r_const__57;
r_const__55 <= r_const__56;
r_const__54 <= r_const__55;
r_const__53 <= r_const__54;
r_const__52 <= r_const__53;
r_const__51 <= r_const__52;
r_const__50 <= r_const__51;
r_const__49 <= r_const__50;
r_const__48 <= r_const__49;
r_const__47 <= r_const__48;
r_const__46 <= r_const__47;
r_const__45 <= r_const__46;
r_const__44 <= r_const__45;
r_const__43 <= r_const__44;
r_const__42 <= r_const__43;
r_const__41 <= r_const__42;
r_const__40 <= r_const__41;
r_const__39 <= r_const__40;
r_const__38 <= r_const__39;
r_const__37 <= r_const__38;
r_const__36 <= r_const__37;
r_const__35 <= r_const__36;
r_const__34 <= r_const__35;
r_const__33 <= r_const__34;
r_const__32 <= r_const__33;
r_const__31 <= r_const__32;
r_const__30 <= r_const__31;
r_const__29 <= r_const__30;
r_const__28 <= r_const__29;
r_const__27 <= r_const__28;
r_const__26 <= r_const__27;
r_const__25 <= r_const__26;
r_const__24 <= r_const__25;
r_const__23 <= r_const__24;
r_const__22 <= r_const__23;
r_const__21 <= r_const__22;
r_const__20 <= r_const__21;
r_const__19 <= r_const__20;
r_const__18 <= r_const__19;
r_const__17 <= r_const__18;
r_const__16 <= r_const__17;
r_const__15 <= r_const__16;
r_const__14 <= r_const__15;
r_const__13 <= r_const__14;
r_const__12 <= r_const__13;
r_const__11 <= r_const__12;
r_const__10 <= r_const__11;
r_const__9 <= r_const__10;
r_const__8 <= r_const__9;
r_const__7 <= r_const__8;
r_const__6 <= r_const__7;
r_const__5 <= r_const__6;
r_const__4 <= r_const__5;
r_const__3 <= r_const__4;
r_const__2 <= r_const__3;
r_const__1 <= r_const__2;
r_const__0 <= r_const__1;
end
else
begin
//original code
r_const__103 <= c_const__103;
r_const__102 <= c_const__102;
r_const__101 <= c_const__101;
r_const__100 <= c_const__100;
r_const__99 <= c_const__99;
r_const__98 <= c_const__98;
r_const__97 <= c_const__97;
r_const__96 <= c_const__96;
r_const__95 <= c_const__95;
r_const__94 <= c_const__94;
r_const__93 <= c_const__93;
r_const__92 <= c_const__92;
r_const__91 <= c_const__91;
r_const__90 <= c_const__90;
r_const__89 <= c_const__89;
r_const__88 <= c_const__88;
r_const__87 <= c_const__87;
r_const__86 <= c_const__86;
r_const__85 <= c_const__85;
r_const__84 <= c_const__84;
r_const__83 <= c_const__83;
r_const__82 <= c_const__82;
r_const__81 <= c_const__81;
r_const__80 <= c_const__80;
r_const__79 <= c_const__79;
r_const__78 <= c_const__78;
r_const__77 <= c_const__77;
r_const__76 <= c_const__76;
r_const__75 <= c_const__75;
r_const__74 <= c_const__74;
r_const__73 <= c_const__73;
r_const__72 <= c_const__72;
r_const__71 <= c_const__71;
r_const__70 <= c_const__70;
r_const__69 <= c_const__69;
r_const__68 <= c_const__68;
r_const__67 <= c_const__67;
r_const__66 <= c_const__66;
r_const__65 <= c_const__65;
r_const__64 <= c_const__64;
r_const__63 <= c_const__63;
r_const__62 <= c_const__62;
r_const__61 <= c_const__61;
r_const__60 <= c_const__60;
r_const__59 <= c_const__59;
r_const__58 <= c_const__58;
r_const__57 <= c_const__57;
r_const__56 <= c_const__56;
r_const__55 <= c_const__55;
r_const__54 <= c_const__54;
r_const__53 <= c_const__53;
r_const__52 <= c_const__52;
r_const__51 <= c_const__51;
r_const__50 <= c_const__50;
r_const__49 <= c_const__49;
r_const__48 <= c_const__48;
r_const__47 <= c_const__47;
r_const__46 <= c_const__46;
r_const__45 <= c_const__45;
r_const__44 <= c_const__44;
r_const__43 <= c_const__43;
r_const__42 <= c_const__42;
r_const__41 <= c_const__41;
r_const__40 <= c_const__40;
r_const__39 <= c_const__39;
r_const__38 <= c_const__38;
r_const__37 <= c_const__37;
r_const__36 <= c_const__36;
r_const__35 <= c_const__35;
r_const__34 <= c_const__34;
r_const__33 <= c_const__33;
r_const__32 <= c_const__32;
r_const__31 <= c_const__31;
r_const__30 <= c_const__30;
r_const__29 <= c_const__29;
r_const__28 <= c_const__28;
r_const__27 <= c_const__27;
r_const__26 <= c_const__26;
r_const__25 <= c_const__25;
r_const__24 <= c_const__24;
r_const__23 <= c_const__23;
r_const__22 <= c_const__22;
r_const__21 <= c_const__21;
r_const__20 <= c_const__20;
r_const__19 <= c_const__19;
r_const__18 <= c_const__18;
r_const__17 <= c_const__17;
r_const__16 <= c_const__16;
r_const__15 <= c_const__15;
r_const__14 <= c_const__14;
r_const__13 <= c_const__13;
r_const__12 <= c_const__12;
r_const__11 <= c_const__11;
r_const__10 <= c_const__10;
r_const__9 <= c_const__9;
r_const__8 <= c_const__8;
r_const__7 <= c_const__7;
r_const__6 <= c_const__6;
r_const__5 <= c_const__5;
r_const__4 <= c_const__4;
r_const__3 <= c_const__3;
r_const__2 <= c_const__2;
r_const__1 <= c_const__1;
r_const__0 <= c_const__0;
end



				
				r_state <= c_state;
				result <= c_result;
				calc_in_progress <= c_calc_in_progress;
				r_absorb_read_counter <= c_absorb_read_counter;
				r_absorb_write_counter <= c_absorb_write_counter;
				r_toggle <= c_toggle;
				//if(c_state == CALC_ST) 
				//begin
					enable <= 1'b1;
				//end
				//else
				//begin
				//	enable = 1'b0;
				//end
				if(c_state == RESET_MEM_ST) 
				begin
					reset_calculator <= 1'b1;
				end
				else
				begin
					reset_calculator <= 1'b0;
				end
			end
	end

endmodule
			
			
			


			
			
module dual_port_mem_zz (clk, data, rdaddress, wraddress , wren, q);

// 32bit wide
// 13bit address

input clk;
input[31:0] data;
input [12:0] rdaddress;
input [12:0] wraddress;
input wren;
output [31:0] q;


wire const_zero;
wire [31:0] const_zero_data;
wire [31:0] dont_care_out;

assign const_zero = 1'b0;
assign const_zero_data = 32'b00000000000000000000000000000000;
assign dont_care_out = 32'b00000000000000000000000000000000;
	
dual_port_ram dpram1(	
  .clk (clk),
  .we1(wren),
  .we2(const_zero),
  .data1(data),
  .data2(const_zero_data),
  .out1(dont_care_out),
  .out2 (q),
  .addr1(wraddress),
  .addr2(rdaddress));
  
  
  endmodule
  
module dual_port_mem_yy (clk, data, rdaddress, wraddress , wren, q);

// 32bit wide
// 13bit address

input clk;
input[31:0] data;
input [12:0] rdaddress;
input [12:0] wraddress;
input wren;
output [31:0] q;


wire const_zero;
wire [31:0] const_zero_data;
wire [31:0] dont_care_out;

assign const_zero = 1'b0;
assign const_zero_data = 32'b00000000000000000000000000000000;
assign dont_care_out = 32'b00000000000000000000000000000000;
	
dual_port_ram dpram1(	
  .clk (clk),
  .we1(wren),
  .we2(const_zero),
  .data1(data),
  .data2(const_zero_data),
  .out1(dont_care_out),
  .out2 (q),
  .addr1(wraddress),
  .addr2(rdaddress));
  
  
  endmodule
  
module dual_port_mem_xx (clk, data, rdaddress, wraddress , wren, q);

// 32bit wide
// 13bit address

input clk;
input[31:0] data;
input [12:0] rdaddress;
input [12:0] wraddress;
input wren;
output [31:0] q;


wire const_zero;
wire [31:0] const_zero_data;
wire [31:0] dont_care_out;

assign const_zero = 1'b0;
assign const_zero_data = 32'b00000000000000000000000000000000;
assign dont_care_out = 32'b00000000000000000000000000000000;
	
dual_port_ram dpram1(	
  .clk (clk),
  .we1(wren),
  .we2(const_zero),
  .data1(data),
  .data2(const_zero_data),
  .out1(dont_care_out),
  .out2 (q),
  .addr1(wraddress),
  .addr2(rdaddress));
  
  
  endmodule

module dual_port_mem_ww (clk, data, rdaddress, wraddress , wren, q);

// 32bit wide
// 13bit address

input clk;
input[31:0] data;
input [12:0] rdaddress;
input [12:0] wraddress;
input wren;
output [31:0] q;


wire const_zero;
wire [31:0] const_zero_data;
wire [31:0] dont_care_out;

assign const_zero = 1'b0;
assign const_zero_data = 32'b00000000000000000000000000000000;
assign dont_care_out = 32'b00000000000000000000000000000000;
	
dual_port_ram dpram1(	
  .clk (clk),
  .we1(wren),
  .we2(const_zero),
  .data1(data),
  .data2(const_zero_data),
  .out1(dont_care_out),
  .out2 (q),
  .addr1(wraddress),
  .addr2(rdaddress));
  
  
  endmodule
  module dual (clk, data, rdaddress, wraddress , wren, q);

// 36bit wide
// 16bit address

input clk;
input[35:0] data;
input [15:0] rdaddress;
input [15:0] wraddress;
input wren;
output [35:0] q;


wire const_zero;
wire [35:0] const_zero_data;
wire [35:0] dont_care_out;

assign const_zero = 1'b0;
assign const_zero_data = 36'b000000000000000000000000000000000000;
assign dont_care_out = 36'b000000000000000000000000000000000000;
	
dual_port_ram dpram1(	
  .clk (clk),
  .we1(wren),
  .we2(const_zero),
  .data1(data),
  .data2(const_zero_data),
  .out1(dont_care_out),
  .out2 (q),
  .addr1(wraddress),
  .addr2(rdaddress));
  
  
  endmodule
   module dual2 (clk, data, rdaddress, wraddress , wren, q);

// 18bit wide
// 16bit address

input clk;
input[17:0] data;
input [15:0] rdaddress;
input [15:0] wraddress;
input wren;
output [17:0] q;


wire const_zero;
wire [17:0] const_zero_data;
wire [17:0] dont_care_out;

assign const_zero = 1'b0;
assign const_zero_data = 18'b000000000000000000;
assign dont_care_out = 18'b000000000000000000;
	
dual_port_ram dpram1(	
  .clk (clk),
  .we1(wren),
  .we2(const_zero),
  .data1(data),
  .data2(const_zero_data),
  .out1(dont_care_out),
  .out2 (q),
  .addr1(wraddress),
  .addr2(rdaddress));
  
  
  endmodule
   module dual3 (clk, data, rdaddress, wraddress , wren, q);

// 8bit wide
// 16bit address

input clk;
input[7:0] data;
input [15:0] rdaddress;
input [15:0] wraddress;
input wren;
output [7:0] q;


wire const_zero;
wire [7:0] const_zero_data;
wire [7:0] dont_care_out;

assign const_zero = 1'b0;
assign const_zero_data = 8'b00000000;
assign dont_care_out = 8'b00000000;
	
dual_port_ram dpram1(	
  .clk (clk),
  .we1(wren),
  .we2(const_zero),
  .data1(data),
  .data2(const_zero_data),
  .out1(dont_care_out),
  .out2 (q),
  .addr1(wraddress),
  .addr2(rdaddress));
  
  
  endmodule

 
 
 //  Photon Calculator
// Note: Use the same random number for fresnel (reflect) as for scatterer because they are mutually exclusive blocks
//       Also scatterer needs two


module PhotonCalculator (
	clock, reset, enable,

	// CONSTANTS
	total_photons,

	randseed1, randseed2, randseed3, randseed4, randseed5,
	
	initialWeight,

	//   Mover
	OneOver_MutMaxrad_0, OneOver_MutMaxrad_1, OneOver_MutMaxrad_2, OneOver_MutMaxrad_3, OneOver_MutMaxrad_4, OneOver_MutMaxrad_5,
	OneOver_MutMaxdep_0, OneOver_MutMaxdep_1, OneOver_MutMaxdep_2, OneOver_MutMaxdep_3, OneOver_MutMaxdep_4, OneOver_MutMaxdep_5,
	OneOver_Mut_0, OneOver_Mut_1, OneOver_Mut_2, OneOver_Mut_3, OneOver_Mut_4, OneOver_Mut_5,

	//   BoundaryChecker
	z1_0, z1_1, z1_2, z1_3, z1_4, z1_5,
	z0_0, z0_1, z0_2, z0_3, z0_4, z0_5,
	mut_0, mut_1, mut_2, mut_3, mut_4, mut_5,
	maxDepth_over_maxRadius,

	//   Hop (no constants)

	//   Scatterer Reflector Wrapper
	down_niOverNt_1, down_niOverNt_2, down_niOverNt_3, down_niOverNt_4, down_niOverNt_5,
	up_niOverNt_1, up_niOverNt_2, up_niOverNt_3, up_niOverNt_4, up_niOverNt_5,
	down_niOverNt_2_1, down_niOverNt_2_2, down_niOverNt_2_3, down_niOverNt_2_4, down_niOverNt_2_5,
	up_niOverNt_2_1, up_niOverNt_2_2, up_niOverNt_2_3, up_niOverNt_2_4, up_niOverNt_2_5,
	downCritAngle_0, downCritAngle_1, downCritAngle_2, downCritAngle_3, downCritAngle_4,
	upCritAngle_0, upCritAngle_1, upCritAngle_2, upCritAngle_3, upCritAngle_4,
	muaFraction1, muaFraction2, muaFraction3, muaFraction4, muaFraction5,
	  // Interface to memory look-up
	    // From Memories
	up_rFresnel, down_rFresnel, sint, cost,
		// To Memories
	tindex, fresIndex,

	// Roulette (no Constants)

	// Absorber
	absorb_data, absorb_rdaddress, absorb_wraddress, 
	absorb_wren, absorb_q,

	// Done signal
	done
	);
//parameter BIT_WIDTH=32;
//parameter LAYER_WIDTH=3;
//parameter TRIG_WIDTH=10;
//parameter PIPELINE_DEPTH_UPPER_LIMIT = 256;
//parameter ABSORB_ADDR_WIDTH=16;
//parameter ABSORB_WORD_WIDTH=64;
//parameter WSCALE=1919999;


input clock, reset, enable;

// CONSTANTS
input [`BIT_WIDTH-1:0] total_photons;

input [`BIT_WIDTH-1:0] randseed1;
input [`BIT_WIDTH-1:0] randseed2;
input [`BIT_WIDTH-1:0] randseed3;
input [`BIT_WIDTH-1:0] randseed4;
input [`BIT_WIDTH-1:0] randseed5;

input [`BIT_WIDTH-1:0] initialWeight;

//   Mover
input [`BIT_WIDTH-1:0] OneOver_MutMaxrad_0, OneOver_MutMaxrad_1, OneOver_MutMaxrad_2, OneOver_MutMaxrad_3, OneOver_MutMaxrad_4, OneOver_MutMaxrad_5;
input [`BIT_WIDTH-1:0] OneOver_MutMaxdep_0, OneOver_MutMaxdep_1, OneOver_MutMaxdep_2, OneOver_MutMaxdep_3, OneOver_MutMaxdep_4, OneOver_MutMaxdep_5;
input [`BIT_WIDTH-1:0] OneOver_Mut_0, OneOver_Mut_1, OneOver_Mut_2, OneOver_Mut_3, OneOver_Mut_4, OneOver_Mut_5;

//   BoundaryChecker
input [`BIT_WIDTH-1:0] z1_0, z1_1, z1_2, z1_3, z1_4, z1_5;
input [`BIT_WIDTH-1:0] z0_0, z0_1, z0_2, z0_3, z0_4, z0_5;
input [`BIT_WIDTH-1:0] mut_0, mut_1, mut_2, mut_3, mut_4, mut_5;
input [`BIT_WIDTH-1:0] maxDepth_over_maxRadius;

//   Hop (no constants)

//   Scatterer Reflector Absorber Wrapper
input [`BIT_WIDTH-1:0] down_niOverNt_1, down_niOverNt_2, down_niOverNt_3, down_niOverNt_4, down_niOverNt_5;
input [`BIT_WIDTH-1:0] up_niOverNt_1, up_niOverNt_2, up_niOverNt_3, up_niOverNt_4, up_niOverNt_5;
input [2*`BIT_WIDTH-1:0] down_niOverNt_2_1, down_niOverNt_2_2, down_niOverNt_2_3, down_niOverNt_2_4, down_niOverNt_2_5;
input [2*`BIT_WIDTH-1:0] up_niOverNt_2_1, up_niOverNt_2_2, up_niOverNt_2_3, up_niOverNt_2_4, up_niOverNt_2_5;
input [`BIT_WIDTH-1:0] downCritAngle_0, downCritAngle_1, downCritAngle_2, downCritAngle_3, downCritAngle_4;
input [`BIT_WIDTH-1:0] upCritAngle_0, upCritAngle_1, upCritAngle_2, upCritAngle_3, upCritAngle_4;
input [`BIT_WIDTH-1:0] muaFraction1, muaFraction2, muaFraction3, muaFraction4, muaFraction5;

  // Memory look-up interface
input [`BIT_WIDTH-1:0] up_rFresnel;
input [`BIT_WIDTH-1:0] down_rFresnel;
input [`BIT_WIDTH-1:0] sint;
input [`BIT_WIDTH-1:0] cost;
	//To Memories
output [12:0] tindex;
output [9:0] fresIndex;

//   Roulette Module (nothing)

// Final results
output [`ABSORB_ADDR_WIDTH-1:0] absorb_rdaddress, absorb_wraddress;
output absorb_wren;
output [`ABSORB_WORD_WIDTH-1:0] absorb_data;
input [`ABSORB_WORD_WIDTH-1:0] absorb_q;

// Flag when final results ready
output done;


// Local variables
// Wired nets
/*mover inputs*/
reg [`BIT_WIDTH-1:0] x_moverMux;
reg [`BIT_WIDTH-1:0] y_moverMux;
reg [`BIT_WIDTH-1:0] z_moverMux;
reg [`BIT_WIDTH-1:0] ux_moverMux;
reg [`BIT_WIDTH-1:0] uy_moverMux;
reg [`BIT_WIDTH-1:0] uz_moverMux;
reg [`BIT_WIDTH-1:0] sz_moverMux;
reg [`BIT_WIDTH-1:0] sr_moverMux;
reg [`BIT_WIDTH-1:0] sleftz_moverMux;
reg [`BIT_WIDTH-1:0] sleftr_moverMux;
reg [`LAYER_WIDTH-1:0] layer_moverMux;
reg [`BIT_WIDTH-1:0] weight_moverMux;
reg dead_moverMux;

/*mover outputs*/
wire [`BIT_WIDTH-1:0] x_mover;
wire [`BIT_WIDTH-1:0] y_mover;
wire [`BIT_WIDTH-1:0] z_mover;
wire [`BIT_WIDTH-1:0] ux_mover;
wire [`BIT_WIDTH-1:0] uy_mover;
wire [`BIT_WIDTH-1:0] uz_mover;
wire [`BIT_WIDTH-1:0] sz_mover;
wire [`BIT_WIDTH-1:0] sr_mover;
wire [`BIT_WIDTH-1:0] sleftz_mover;
wire [`BIT_WIDTH-1:0] sleftr_mover;
wire [`LAYER_WIDTH-1:0] layer_mover;
wire [`BIT_WIDTH-1:0] weight_mover;
wire dead_mover;

/*boundary checker outputs*/
wire [`BIT_WIDTH-1:0] x_boundaryChecker;
wire [`BIT_WIDTH-1:0] y_boundaryChecker;
wire [`BIT_WIDTH-1:0] z_boundaryChecker;
wire [`BIT_WIDTH-1:0] ux_boundaryChecker;
wire [`BIT_WIDTH-1:0] uy_boundaryChecker;
wire [`BIT_WIDTH-1:0] uz_boundaryChecker;
wire [`BIT_WIDTH-1:0] sz_boundaryChecker;
wire [`BIT_WIDTH-1:0] sr_boundaryChecker;
wire [`BIT_WIDTH-1:0] sleftz_boundaryChecker;
wire [`BIT_WIDTH-1:0] sleftr_boundaryChecker;
wire [`LAYER_WIDTH-1:0] layer_boundaryChecker;
wire [`BIT_WIDTH-1:0] weight_boundaryChecker;
wire dead_boundaryChecker;
wire hit_boundaryChecker;

/*hop outputs*/
wire [`BIT_WIDTH-1:0] x_hop;
wire [`BIT_WIDTH-1:0] y_hop;
wire [`BIT_WIDTH-1:0] z_hop;
wire [`BIT_WIDTH-1:0] ux_hop;
wire [`BIT_WIDTH-1:0] uy_hop;
wire [`BIT_WIDTH-1:0] uz_hop;
wire [`BIT_WIDTH-1:0] sz_hop;
wire [`BIT_WIDTH-1:0] sr_hop;
wire [`BIT_WIDTH-1:0] sleftz_hop;
wire [`BIT_WIDTH-1:0] sleftr_hop;
wire [`LAYER_WIDTH-1:0] layer_hop;
wire [`BIT_WIDTH-1:0] weight_hop;
wire dead_hop;
wire hit_hop;

/*Drop spin outputs*/
wire [`BIT_WIDTH-1:0] x_dropSpin;
wire [`BIT_WIDTH-1:0] y_dropSpin;
wire [`BIT_WIDTH-1:0] z_dropSpin;
wire [`BIT_WIDTH-1:0] ux_dropSpin;
wire [`BIT_WIDTH-1:0] uy_dropSpin;
wire [`BIT_WIDTH-1:0] uz_dropSpin;
wire [`BIT_WIDTH-1:0] sz_dropSpin;
wire [`BIT_WIDTH-1:0] sr_dropSpin;
wire [`BIT_WIDTH-1:0] sleftz_dropSpin;
wire [`BIT_WIDTH-1:0] sleftr_dropSpin;
wire [`LAYER_WIDTH-1:0] layer_dropSpin;
wire [`BIT_WIDTH-1:0] weight_dropSpin;
wire dead_dropSpin;
//Had to add this one to avoid implicit net:
wire hit_dropSpin;

/*Dead or Alive outputs*/
wire [`BIT_WIDTH-1:0] x_Roulette;
wire [`BIT_WIDTH-1:0] y_Roulette;
wire [`BIT_WIDTH-1:0] z_Roulette;
wire [`BIT_WIDTH-1:0] ux_Roulette;
wire [`BIT_WIDTH-1:0] uy_Roulette;
wire [`BIT_WIDTH-1:0] uz_Roulette;
wire [`BIT_WIDTH-1:0] sz_Roulette;
wire [`BIT_WIDTH-1:0] sr_Roulette;
wire [`BIT_WIDTH-1:0] sleftz_Roulette;
wire [`BIT_WIDTH-1:0] sleftr_Roulette;
wire [`LAYER_WIDTH-1:0] layer_Roulette;
wire [`BIT_WIDTH-1:0] weight_Roulette;
wire dead_Roulette;

// internals
wire [`BIT_WIDTH-1:0] rand1, rand2, rand3, rand4, rand5;
wire [`BIT_WIDTH-1:0] logrand;

// Combinational Drivers
reg [`BIT_WIDTH-1:0] c_num_photons_left;
reg [`BIT_WIDTH-1:0] c_counter;
reg c_done;

// Registered Drivers
reg r_done;
reg loadseed;
reg delay_loadseed;


reg [`BIT_WIDTH-1:0] r_num_photons_left;
reg [`BIT_WIDTH-1:0] r_counter;

assign done = r_done;

//Cannot be logic in instantiatino:
wire not_reset;
assign not_reset = ~reset;

// Connect blocks
LogCalc log_u1(.clock(clock), .reset(reset), .enable(1'b1), .in_x(rand1), .log_x(logrand));
rng rand_u1(.clk(clock), .en(1'b1), .resetn(not_reset), .loadseed_i(loadseed), .seed_i(randseed1), .number_o(rand1));
rng rand_u2(.clk(clock), .en(1'b1), .resetn(not_reset), .loadseed_i(loadseed), .seed_i(randseed2), .number_o(rand2));
rng rand_u3(.clk(clock), .en(1'b1), .resetn(not_reset), .loadseed_i(loadseed), .seed_i(randseed3), .number_o(rand3));
rng rand_u4(.clk(clock), .en(1'b1), .resetn(not_reset), .loadseed_i(loadseed), .seed_i(randseed4), .number_o(rand4));
rng rand_u5(.clk(clock), .en(1'b1), .resetn(not_reset), .loadseed_i(loadseed), .seed_i(randseed5), .number_o(rand5));

Move mover(		 .clock(clock), .reset(reset), .enable(enable),
				 .x_moverMux(x_moverMux), .y_moverMux(y_moverMux), .z_moverMux(z_moverMux),
				 .ux_moverMux(ux_moverMux), .uy_moverMux(uy_moverMux), .uz_moverMux(uz_moverMux),
				 .sz_moverMux(sz_moverMux), .sr_moverMux(sr_moverMux),
				 .sleftz_moverMux(sleftz_moverMux), .sleftr_moverMux(sleftr_moverMux),
				 .layer_moverMux(layer_moverMux), .weight_moverMux(weight_moverMux), .dead_moverMux(dead_moverMux),

				 .log_rand_num(logrand),

				 //OUTPUTS
				 .x_mover(x_mover), .y_mover(y_mover), .z_mover(z_mover),
				 .ux_mover(ux_mover), .uy_mover(uy_mover), .uz_mover(uz_mover),
				 .sz_mover(sz_mover), .sr_mover(sr_mover),
				 .sleftz_mover(sleftz_mover), .sleftr_mover(sleftr_mover),
				 .layer_mover(layer_mover), .weight_mover(weight_mover), .dead_mover(dead_mover),

				 // CONSTANTS
				 .OneOver_MutMaxrad_0(OneOver_MutMaxrad_0), .OneOver_MutMaxrad_1(OneOver_MutMaxrad_1), .OneOver_MutMaxrad_2(OneOver_MutMaxrad_2), .OneOver_MutMaxrad_3(OneOver_MutMaxrad_3), .OneOver_MutMaxrad_4(OneOver_MutMaxrad_4), .OneOver_MutMaxrad_5(OneOver_MutMaxrad_5),
				 .OneOver_MutMaxdep_0(OneOver_MutMaxdep_0), .OneOver_MutMaxdep_1(OneOver_MutMaxdep_1), .OneOver_MutMaxdep_2(OneOver_MutMaxdep_2), .OneOver_MutMaxdep_3(OneOver_MutMaxdep_3), .OneOver_MutMaxdep_4(OneOver_MutMaxdep_4), .OneOver_MutMaxdep_5(OneOver_MutMaxdep_5),
				 .OneOver_Mut_0(OneOver_Mut_0), .OneOver_Mut_1(OneOver_Mut_1), .OneOver_Mut_2(OneOver_Mut_2), .OneOver_Mut_3(OneOver_Mut_3), .OneOver_Mut_4(OneOver_Mut_4), .OneOver_Mut_5(OneOver_Mut_5)
		);

Boundary boundaryChecker ( //INPUTS
				 .clock(clock), .reset(reset), .enable(enable),
				 .x_mover(x_mover), .y_mover(y_mover), .z_mover(z_mover),
				 .ux_mover(ux_mover), .uy_mover(uy_mover), .uz_mover(uz_mover),
				 .sz_mover(sz_mover), .sr_mover(sr_mover),
				 .sleftz_mover(sleftz_mover), .sleftr_mover(sleftr_mover),
				 .layer_mover(layer_mover), .weight_mover(weight_mover), .dead_mover(dead_mover),

				 //OUTPUTS
				 .x_boundaryChecker(x_boundaryChecker), .y_boundaryChecker(y_boundaryChecker), .z_boundaryChecker(z_boundaryChecker),
				 .ux_boundaryChecker(ux_boundaryChecker), .uy_boundaryChecker(uy_boundaryChecker), .uz_boundaryChecker(uz_boundaryChecker),
				 .sz_boundaryChecker(sz_boundaryChecker), .sr_boundaryChecker(sr_boundaryChecker),
				 .sleftz_boundaryChecker(sleftz_boundaryChecker), .sleftr_boundaryChecker(sleftr_boundaryChecker),
				 .layer_boundaryChecker(layer_boundaryChecker), .weight_boundaryChecker(weight_boundaryChecker), .dead_boundaryChecker(dead_boundaryChecker), .hit_boundaryChecker(hit_boundaryChecker),

				 //CONSTANTS
				 .z1_0(z1_0), .z1_1(z1_1), .z1_2(z1_2), .z1_3(z1_3), .z1_4(z1_4), .z1_5(z1_5),
				 .z0_0(z0_0), .z0_1(z0_1), .z0_2(z0_2), .z0_3(z0_3), .z0_4(z0_4), .z0_5(z0_5),
				 .mut_0(mut_0), .mut_1(mut_1), .mut_2(mut_2), .mut_3(mut_3), .mut_4(mut_4), .mut_5(mut_5),
				 .maxDepth_over_maxRadius(maxDepth_over_maxRadius)
				 );

Hop hopper (     //INPUTS
				 .clock(clock), .reset(reset), .enable(enable),
				 .x_boundaryChecker(x_boundaryChecker), .y_boundaryChecker(y_boundaryChecker), .z_boundaryChecker(z_boundaryChecker),
				 .ux_boundaryChecker(ux_boundaryChecker), .uy_boundaryChecker(uy_boundaryChecker), .uz_boundaryChecker(uz_boundaryChecker),
				 .sz_boundaryChecker(sz_boundaryChecker), .sr_boundaryChecker(sr_boundaryChecker),
				 .sleftz_boundaryChecker(sleftz_boundaryChecker), .sleftr_boundaryChecker(sleftr_boundaryChecker),
				 .layer_boundaryChecker(layer_boundaryChecker), .weight_boundaryChecker(weight_boundaryChecker), .dead_boundaryChecker(dead_boundaryChecker),
				 .hit_boundaryChecker(hit_boundaryChecker),

				 //OUTPUTS
				 .x_hop(x_hop), .y_hop(y_hop), .z_hop(z_hop),
				 .ux_hop(ux_hop), .uy_hop(uy_hop), .uz_hop(uz_hop),
				 .sz_hop(sz_hop), .sr_hop(sr_hop),
				 .sleftz_hop(sleftz_hop), .sleftr_hop(sleftr_hop),
				 .layer_hop(layer_hop), .weight_hop(weight_hop), .dead_hop(dead_hop), .hit_hop(hit_hop)
				 );

Roulette Roulette ( //INPUTS
                     .clock(clock), .reset(reset), .enable(enable),
                     .x_RouletteMux(x_dropSpin), .y_RouletteMux(y_dropSpin), .z_RouletteMux(z_dropSpin),
                     .ux_RouletteMux(ux_dropSpin), .uy_RouletteMux(uy_dropSpin), .uz_RouletteMux(uz_dropSpin),
                     .sz_RouletteMux(sz_dropSpin), .sr_RouletteMux(sr_dropSpin),
                     .sleftz_RouletteMux(sleftz_dropSpin), .sleftr_RouletteMux(sleftr_dropSpin),
                     .layer_RouletteMux(layer_dropSpin), .weight_absorber(weight_dropSpin), .dead_RouletteMux(dead_dropSpin),
					 .randnumber(rand4),

                     //OUTPUTS
                     .x_Roulette(x_Roulette), .y_Roulette(y_Roulette), .z_Roulette(z_Roulette),
                     .ux_Roulette(ux_Roulette), .uy_Roulette(uy_Roulette), .uz_Roulette(uz_Roulette),
                     .sz_Roulette(sz_Roulette), .sr_Roulette(sr_Roulette),
                     .sleftz_Roulette(sleftz_Roulette), .sleftr_Roulette(sleftr_Roulette),
                     .layer_Roulette(layer_Roulette), .weight_Roulette(weight_Roulette), .dead_Roulette(dead_Roulette)
					 );


DropSpinWrapper dropSpin (
	.clock(clock), .reset(reset), .enable(enable),

   //From Hopper Module
    .i_x(x_hop),
	.i_y(y_hop),
	.i_z(z_hop),
	.i_ux(ux_hop),
	.i_uy(uy_hop),
	.i_uz(uz_hop),
	.i_sz(sz_hop),
	.i_sr(sr_hop),
	.i_sleftz(sleftz_hop),
	.i_sleftr(sleftr_hop),
	.i_weight(weight_hop),
	.i_layer(layer_hop),
	.i_dead(dead_hop),
	.i_hit(hit_hop),	
	
	//From System Register File (5 layers)- Absorber
	.muaFraction1(muaFraction1), .muaFraction2(muaFraction2), .muaFraction3(muaFraction3), .muaFraction4(muaFraction4), .muaFraction5(muaFraction5),
 
 	//From System Register File - ScattererReflector 
	.down_niOverNt_1(down_niOverNt_1),
	.down_niOverNt_2(down_niOverNt_2),
	.down_niOverNt_3(down_niOverNt_3),
	.down_niOverNt_4(down_niOverNt_4),
	.down_niOverNt_5(down_niOverNt_5),
	.up_niOverNt_1(up_niOverNt_1),
	.up_niOverNt_2(up_niOverNt_2),
	.up_niOverNt_3(up_niOverNt_3),
	.up_niOverNt_4(up_niOverNt_4),
	.up_niOverNt_5(up_niOverNt_5),
	.down_niOverNt_2_1(down_niOverNt_2_1),
	.down_niOverNt_2_2(down_niOverNt_2_2),
	.down_niOverNt_2_3(down_niOverNt_2_3),
	.down_niOverNt_2_4(down_niOverNt_2_4),
	.down_niOverNt_2_5(down_niOverNt_2_5),
	.up_niOverNt_2_1(up_niOverNt_2_1),
	.up_niOverNt_2_2(up_niOverNt_2_2),
	.up_niOverNt_2_3(up_niOverNt_2_3),
	.up_niOverNt_2_4(up_niOverNt_2_4),
	.up_niOverNt_2_5(up_niOverNt_2_5),
	.downCritAngle_0(downCritAngle_0),
	.downCritAngle_1(downCritAngle_1),
	.downCritAngle_2(downCritAngle_2),
	.downCritAngle_3(downCritAngle_3),
	.downCritAngle_4(downCritAngle_4),
	.upCritAngle_0(upCritAngle_0),
	.upCritAngle_1(upCritAngle_1),
	.upCritAngle_2(upCritAngle_2),
	.upCritAngle_3(upCritAngle_3),
	.upCritAngle_4(upCritAngle_4),
 
	// port to memory
	 .data(absorb_data), .rdaddress(absorb_rdaddress), .wraddress(absorb_wraddress), 
	 .wren(absorb_wren), .q(absorb_q),

 //Generated by random number generators controlled by skeleton
	.up_rFresnel(up_rFresnel),
	.down_rFresnel(down_rFresnel),
	.sint(sint),
	.cost(cost),
	.rand2(rand2),
	.rand3(rand3),
	.rand5(rand5),
	//To Memories
	.tindex(tindex),
	.fresIndex(fresIndex),


	 
   //To Roulette Module
	.o_x(x_dropSpin),
	.o_y(y_dropSpin),
	.o_z(z_dropSpin),
	.o_ux(ux_dropSpin),
	.o_uy(uy_dropSpin),
	.o_uz(uz_dropSpin),
	.o_sz(sz_dropSpin),
	.o_sr(sr_dropSpin),
	.o_sleftz(sleftz_dropSpin),
	.o_sleftr(sleftr_dropSpin),
	.o_weight(weight_dropSpin),
	.o_layer(layer_dropSpin),
	.o_dead(dead_dropSpin),
	.o_hit(hit_dropSpin)
                    
	);
	
// Determine how many photons left
always @(r_num_photons_left or dead_Roulette or r_done or r_counter)
begin
	//c_num_photons_left = r_num_photons_left;
	//c_counter = 0;

	if(dead_Roulette == 1'b1 && r_done == 1'b0)
	begin
		if(r_num_photons_left > 0)
		begin
			c_num_photons_left = r_num_photons_left - 1;
			c_counter = 0;
		end
		else
		begin
			c_counter = r_counter + 1;
			c_num_photons_left = r_num_photons_left;
		end
	end 
	else
	begin
		c_num_photons_left = r_num_photons_left;
		c_counter = 0;
	end
end

// Only state info is done
always @(r_done or r_counter)
begin
	//c_done = r_done;
	if(r_counter > `PIPELINE_DEPTH_UPPER_LIMIT)
	begin
		c_done = 1'b1;
	end else begin
		c_done = r_done;
	end
end

// Create mux to mover
always @(dead_Roulette or initialWeight or r_num_photons_left or x_Roulette or y_Roulette or z_Roulette or 
			ux_Roulette or uy_Roulette or uz_Roulette or sz_Roulette or sr_Roulette or sleftz_Roulette or 
			sleftr_Roulette or layer_Roulette or weight_Roulette or dead_Roulette)
begin
	if(dead_Roulette)
	begin
		x_moverMux = 0;
		y_moverMux = 0;
		z_moverMux = 0;
		ux_moverMux = 0;
		uy_moverMux = 0;
		uz_moverMux = 32'h7fffffff;
		sz_moverMux = 0;
		sr_moverMux = 0;
		sleftz_moverMux = 0;
		sleftr_moverMux = 0;
		layer_moverMux = 3'b01;
		weight_moverMux = initialWeight;
		if(r_num_photons_left > 0)
		begin
			dead_moverMux = 1'b0;
		end
		else
		begin
			dead_moverMux = 1'b1;
		end
	end
	else
	begin
		x_moverMux = x_Roulette;
		y_moverMux = y_Roulette;
		z_moverMux = z_Roulette;
		ux_moverMux = ux_Roulette;
		uy_moverMux = uy_Roulette;
		uz_moverMux = uz_Roulette;
		sz_moverMux = sz_Roulette;
		sr_moverMux = sr_Roulette;
		sleftz_moverMux = sleftz_Roulette;
		sleftr_moverMux = sleftr_Roulette;
		layer_moverMux = layer_Roulette;
		weight_moverMux = weight_Roulette;
		dead_moverMux = dead_Roulette;
	end
end

// register state
always @(posedge clock)
begin
	if(reset)
	begin
		r_num_photons_left <= total_photons;
		r_counter <= 1'b0;
		r_done <= 1'b0;
		delay_loadseed <= 1'b1;
		loadseed <= 1'b1;
	end
	else
	begin
		if(enable)
		begin
			r_num_photons_left <= c_num_photons_left;
			r_counter <= c_counter;
			r_done <= c_done;
			delay_loadseed <= 1'b0;
			loadseed <= delay_loadseed;
		end
	end
end
endmodule


module Move(     //INPUTS
				 clock, reset, enable,
				 x_moverMux, y_moverMux, z_moverMux,
				 ux_moverMux, uy_moverMux, uz_moverMux,
				 sz_moverMux, sr_moverMux,
				 sleftz_moverMux, sleftr_moverMux,
				 layer_moverMux, weight_moverMux, dead_moverMux,

				 log_rand_num,

				 //OUTPUTS
				 x_mover, y_mover, z_mover,
				 ux_mover, uy_mover, uz_mover,
				 sz_mover, sr_mover,
				 sleftz_mover, sleftr_mover,
				 layer_mover, weight_mover, dead_mover,

				 // CONSTANTS
				 OneOver_MutMaxrad_0, OneOver_MutMaxrad_1, OneOver_MutMaxrad_2, OneOver_MutMaxrad_3, OneOver_MutMaxrad_4, OneOver_MutMaxrad_5,
				 OneOver_MutMaxdep_0, OneOver_MutMaxdep_1, OneOver_MutMaxdep_2, OneOver_MutMaxdep_3, OneOver_MutMaxdep_4, OneOver_MutMaxdep_5,
				 OneOver_Mut_0, OneOver_Mut_1, OneOver_Mut_2, OneOver_Mut_3, OneOver_Mut_4, OneOver_Mut_5
				 );


input clock;
input reset;
input enable;

input [`BIT_WIDTH-1:0] x_moverMux;
input [`BIT_WIDTH-1:0] y_moverMux;
input [`BIT_WIDTH-1:0] z_moverMux;
input [`BIT_WIDTH-1:0] ux_moverMux;
input [`BIT_WIDTH-1:0] uy_moverMux;
input [`BIT_WIDTH-1:0] uz_moverMux;
input [`BIT_WIDTH-1:0] sz_moverMux;
input [`BIT_WIDTH-1:0] sr_moverMux;
input [`BIT_WIDTH-1:0] sleftz_moverMux;
input [`BIT_WIDTH-1:0] sleftr_moverMux;
input [`LAYER_WIDTH-1:0] layer_moverMux;
input [`BIT_WIDTH-1:0] weight_moverMux;
input	dead_moverMux;

output [`BIT_WIDTH-1:0] x_mover;
output [`BIT_WIDTH-1:0] y_mover;
output [`BIT_WIDTH-1:0] z_mover;
output [`BIT_WIDTH-1:0] ux_mover;
output [`BIT_WIDTH-1:0] uy_mover;
output [`BIT_WIDTH-1:0] uz_mover;
output [`BIT_WIDTH-1:0] sz_mover;
output [`BIT_WIDTH-1:0] sr_mover;
output [`BIT_WIDTH-1:0] sleftz_mover;
output [`BIT_WIDTH-1:0] sleftr_mover;
output [`LAYER_WIDTH-1:0]layer_mover;
output [`BIT_WIDTH-1:0] weight_mover;
output	dead_mover;


input [`BIT_WIDTH-1:0] OneOver_MutMaxrad_0;
input [`BIT_WIDTH-1:0] OneOver_MutMaxrad_1;
input [`BIT_WIDTH-1:0] OneOver_MutMaxrad_2;
input [`BIT_WIDTH-1:0] OneOver_MutMaxrad_3;
input [`BIT_WIDTH-1:0] OneOver_MutMaxrad_4;
input [`BIT_WIDTH-1:0] OneOver_MutMaxrad_5;
input [`BIT_WIDTH-1:0] OneOver_MutMaxdep_0;
input [`BIT_WIDTH-1:0] OneOver_MutMaxdep_1;
input [`BIT_WIDTH-1:0] OneOver_MutMaxdep_2;
input [`BIT_WIDTH-1:0] OneOver_MutMaxdep_3;
input [`BIT_WIDTH-1:0] OneOver_MutMaxdep_4;
input [`BIT_WIDTH-1:0] OneOver_MutMaxdep_5;
input [`BIT_WIDTH-1:0] OneOver_Mut_0;
input [`BIT_WIDTH-1:0] OneOver_Mut_1;
input [`BIT_WIDTH-1:0] OneOver_Mut_2;
input [`BIT_WIDTH-1:0] OneOver_Mut_3;
input [`BIT_WIDTH-1:0] OneOver_Mut_4;
input [`BIT_WIDTH-1:0] OneOver_Mut_5;
input [`BIT_WIDTH-1:0] log_rand_num;

//------------Local Variables------------------------
reg [`BIT_WIDTH-1:0] c_sr;
reg [`BIT_WIDTH-1:0] c_sz;
reg [2*`BIT_WIDTH-1:0] c_sr_big;
reg [2*`BIT_WIDTH-1:0] c_sz_big;
reg [`BIT_WIDTH-1:0] c_sleftr;
reg [`BIT_WIDTH-1:0] c_sleftz;

//No signed regs, unsigned unecessary
//reg unsigned [`BIT_WIDTH-1:0] c_r_op0;
//reg unsigned [`BIT_WIDTH-1:0] c_r_op1;
//reg unsigned [`BIT_WIDTH-1:0] c_z_op0;
//reg unsigned [`BIT_WIDTH-1:0] c_z_op1;

reg [`BIT_WIDTH-1:0] c_r_op0;
reg [`BIT_WIDTH-1:0] c_r_op1;
reg [`BIT_WIDTH-1:0] c_z_op0;
reg [`BIT_WIDTH-1:0] c_z_op1;

// grab multiplexed constant
reg [`BIT_WIDTH-1:0] OneOver_MutMaxrad;
reg [`BIT_WIDTH-1:0] OneOver_MutMaxdep;
reg [`BIT_WIDTH-1:0] OneOver_Mut;

//------------REGISTERED Values------------------------
reg [`BIT_WIDTH-1:0] x_mover;
reg [`BIT_WIDTH-1:0] y_mover;
reg [`BIT_WIDTH-1:0] z_mover;
reg [`BIT_WIDTH-1:0] ux_mover;
reg [`BIT_WIDTH-1:0] uy_mover;
reg [`BIT_WIDTH-1:0] uz_mover;
reg [`BIT_WIDTH-1:0] sz_mover;
reg [`BIT_WIDTH-1:0] sr_mover;
reg [`BIT_WIDTH-1:0] sleftz_mover;
reg [`BIT_WIDTH-1:0] sleftr_mover;
reg [`LAYER_WIDTH-1:0]layer_mover;
reg [`BIT_WIDTH-1:0] weight_mover;
reg	dead_mover;


//Need this to deal with 'unused' inputs for ODIN II
wire bigOr;
assign bigOr = sr_moverMux[0] | sr_moverMux[1] | sr_moverMux[2] | sr_moverMux[3] | sr_moverMux[4] | sr_moverMux[5] | 
					sr_moverMux[6] | sr_moverMux[7] | sr_moverMux[8] | sr_moverMux[9] | sr_moverMux[10] | sr_moverMux[11] | 
					sr_moverMux[12] | sr_moverMux[13] | sr_moverMux[14] | sr_moverMux[15] | sr_moverMux[16] | sr_moverMux[17] | 
					sr_moverMux[18] | sr_moverMux[19] | sr_moverMux[20] | sr_moverMux[21] | sr_moverMux[22] | sr_moverMux[23] | 
					sr_moverMux[24] | sr_moverMux[25] | sr_moverMux[26] | sr_moverMux[27] | sr_moverMux[28] | sr_moverMux[29] | 
					sr_moverMux[30] | sr_moverMux[31] | 
					sz_moverMux[0] | sz_moverMux[1] | sz_moverMux[2] | sz_moverMux[3] | sz_moverMux[4] | sz_moverMux[5] | 
					sz_moverMux[6] | sz_moverMux[7] | sz_moverMux[8] | sz_moverMux[9] | sz_moverMux[10] | sz_moverMux[11] | 
					sz_moverMux[12] | sz_moverMux[13] | sz_moverMux[14] | sz_moverMux[15] | sz_moverMux[16] | sz_moverMux[17] | 
					sz_moverMux[18] | sz_moverMux[19] | sz_moverMux[20] | sz_moverMux[21] | sz_moverMux[22] | sz_moverMux[23] | 
					sz_moverMux[24] | sz_moverMux[25] | sz_moverMux[26] | sz_moverMux[27] | sz_moverMux[28] | sz_moverMux[29] | 
					sz_moverMux[30] | sz_moverMux[31] | 
					1'b1;
wire reset_new;
assign reset_new = reset & bigOr;

// multiplex constants
always @(layer_moverMux or OneOver_MutMaxrad_0 or OneOver_MutMaxdep_0 or OneOver_Mut_0 or
						OneOver_MutMaxrad_1 or OneOver_MutMaxdep_1 or OneOver_Mut_1 or
						OneOver_MutMaxrad_2 or OneOver_MutMaxdep_2 or OneOver_Mut_2 or
						OneOver_MutMaxrad_3 or OneOver_MutMaxdep_3 or OneOver_Mut_3 or
						OneOver_MutMaxrad_4 or OneOver_MutMaxdep_4 or OneOver_Mut_4 or
						OneOver_MutMaxrad_5 or OneOver_MutMaxdep_5 or OneOver_Mut_5)
begin
case(layer_moverMux)
	3'b000:
	begin
		OneOver_MutMaxrad = OneOver_MutMaxrad_0;
		OneOver_MutMaxdep = OneOver_MutMaxdep_0;
		OneOver_Mut = OneOver_Mut_0;
	end
	3'b001:
	begin
		OneOver_MutMaxrad = OneOver_MutMaxrad_1;
		OneOver_MutMaxdep = OneOver_MutMaxdep_1;
		OneOver_Mut = OneOver_Mut_1;
	end
	3'b010:
	begin
		OneOver_MutMaxrad = OneOver_MutMaxrad_2;
		OneOver_MutMaxdep = OneOver_MutMaxdep_2;
		OneOver_Mut = OneOver_Mut_2;
	end
	3'b011:
	begin
		OneOver_MutMaxrad = OneOver_MutMaxrad_3;
		OneOver_MutMaxdep = OneOver_MutMaxdep_3;
		OneOver_Mut = OneOver_Mut_3;
	end
	3'b100:
	begin
		OneOver_MutMaxrad = OneOver_MutMaxrad_4;
		OneOver_MutMaxdep = OneOver_MutMaxdep_4;
		OneOver_Mut = OneOver_Mut_4;
	end
	3'b101:
	begin
		OneOver_MutMaxrad = OneOver_MutMaxrad_5;
		OneOver_MutMaxdep = OneOver_MutMaxdep_5;
		OneOver_Mut = OneOver_Mut_5;
	end
	default:
	begin
		OneOver_MutMaxrad = 0;
		OneOver_MutMaxdep = 0;
		OneOver_Mut = 0;
	end
endcase
end

// Determine move value
always @(sleftz_moverMux or log_rand_num or OneOver_MutMaxrad or OneOver_MutMaxdep or sleftr_moverMux or
		OneOver_Mut)
begin
	// Resource sharing for multipliers
	if(sleftz_moverMux == 32'b0)
	begin
		c_r_op0 = `MAXLOG - log_rand_num;
		c_r_op1 = OneOver_MutMaxrad;
		c_z_op0 = `MAXLOG - log_rand_num;
		c_z_op1 = OneOver_MutMaxdep;
	end
	else
	begin
		c_r_op0 = sleftr_moverMux;
		c_r_op1 = OneOver_Mut;
		c_z_op0 = sleftz_moverMux;
		c_z_op1 = OneOver_Mut;
	end
end

// Determine move value
always @(sleftz_moverMux or c_r_op0 or c_r_op1 or c_z_op0 or c_z_op1 or sleftr_moverMux)
begin
	c_sr_big = c_r_op0 * c_r_op1;
	c_sz_big = c_z_op0 * c_z_op1;
	if(sleftz_moverMux == 32'b0)
	begin
		c_sr = c_sr_big[2*`BIT_WIDTH - `LOGSCALEFACTOR - 1:`BIT_WIDTH - `LOGSCALEFACTOR];
		c_sz = c_sz_big[2*`BIT_WIDTH - `LOGSCALEFACTOR - 1:`BIT_WIDTH - `LOGSCALEFACTOR];

		c_sleftr = sleftr_moverMux;
		c_sleftz = 0;

		//c_sr = `CONST_MOVE_AMOUNT;
		//c_sz = `CONST_MOVE_AMOUNT;
	end
	else
	begin
		c_sr = c_sr_big[2*`BIT_WIDTH - `MUTMAX_BITS - 1 - 1:`BIT_WIDTH - `MUTMAX_BITS - 1];
		c_sz = c_sz_big[2*`BIT_WIDTH - `MUTMAX_BITS - 1 - 1:`BIT_WIDTH - `MUTMAX_BITS - 1];

		c_sleftz = 0;
		c_sleftr = 0;
	end
end

// latch values
always @ (posedge clock)
begin
	if (reset_new)
	begin
		// Photon variables
		x_mover <= 0;
		y_mover <= 0;
		z_mover <= 0;
		ux_mover <= 0;
		uy_mover <= 0;
		uz_mover <= 0;
		sz_mover <= 0;
		sr_mover <= 0;
		sleftz_mover <= 0;
		sleftr_mover <= 0;
		layer_mover <= 0;
		weight_mover <= 0;
		dead_mover <= 1'b1;
	end
	else
	begin
		if(enable)
		begin
			// Photon variables
			x_mover <= x_moverMux;
			y_mover <= y_moverMux;
			z_mover <= z_moverMux;
			ux_mover <= ux_moverMux;
			uy_mover <= uy_moverMux;
			uz_mover <= uz_moverMux;
			layer_mover <= layer_moverMux;
			weight_mover <= weight_moverMux;
			dead_mover <= dead_moverMux;

			sz_mover <= c_sz;
			sr_mover <= c_sr;
			sleftz_mover <= c_sleftz;
			sleftr_mover <= c_sleftr;
		end
	end
end

endmodule


module Boundary ( //INPUTS
				 clock, reset, enable,
				 x_mover, y_mover, z_mover,
				 ux_mover, uy_mover, uz_mover,
				 sz_mover, sr_mover,
				 sleftz_mover, sleftr_mover,
				 layer_mover, weight_mover, dead_mover,

				 //OUTPUTS
				 x_boundaryChecker, y_boundaryChecker, z_boundaryChecker,
				 ux_boundaryChecker, uy_boundaryChecker, uz_boundaryChecker,
				 sz_boundaryChecker, sr_boundaryChecker,
				 sleftz_boundaryChecker, sleftr_boundaryChecker,
				 layer_boundaryChecker, weight_boundaryChecker, dead_boundaryChecker, hit_boundaryChecker,

				 //CONSTANTS
				 z1_0, z1_1, z1_2, z1_3, z1_4, z1_5, 
				 z0_0, z0_1, z0_2, z0_3, z0_4, z0_5,
				 mut_0, mut_1, mut_2, mut_3, mut_4, mut_5, 
				 maxDepth_over_maxRadius
				 );

//parameter BIT_WIDTH=32;
//parameter LAYER_WIDTH=3;
//parameter INTMAX=2147483647;
//parameter INTMIN=-2147483648;
//parameter DIVIDER_LATENCY=30;
//parameter FINAL_LATENCY=28;
//parameter MULT_LATENCY=1;
//parameter ASPECT_RATIO = 7;
//parameter TOTAL_LATENCY = `DIVIDER_LATENCY + `FINAL_LATENCY + `MULT_LATENCY + `MULT_LATENCY;

input clock;
input reset;
input enable;

input [`BIT_WIDTH-1:0] x_mover;
input [`BIT_WIDTH-1:0] y_mover;
input [`BIT_WIDTH-1:0] z_mover;
input [`BIT_WIDTH-1:0] ux_mover;
input [`BIT_WIDTH-1:0] uy_mover;
input [`BIT_WIDTH-1:0] uz_mover;
input [`BIT_WIDTH-1:0] sz_mover;
input [`BIT_WIDTH-1:0] sr_mover;
input [`BIT_WIDTH-1:0] sleftz_mover;
input [`BIT_WIDTH-1:0] sleftr_mover;
input [`LAYER_WIDTH-1:0] layer_mover;
input [`BIT_WIDTH-1:0] weight_mover;
input	dead_mover;

output [`BIT_WIDTH-1:0] x_boundaryChecker;
output [`BIT_WIDTH-1:0] y_boundaryChecker;
output [`BIT_WIDTH-1:0] z_boundaryChecker;
output [`BIT_WIDTH-1:0] ux_boundaryChecker;
output [`BIT_WIDTH-1:0] uy_boundaryChecker;
output [`BIT_WIDTH-1:0] uz_boundaryChecker;
output [`BIT_WIDTH-1:0] sz_boundaryChecker;
output [`BIT_WIDTH-1:0] sr_boundaryChecker;
output [`BIT_WIDTH-1:0] sleftz_boundaryChecker;
output [`BIT_WIDTH-1:0] sleftr_boundaryChecker;
output [`LAYER_WIDTH-1:0]layer_boundaryChecker;
output [`BIT_WIDTH-1:0] weight_boundaryChecker;
output dead_boundaryChecker;
output hit_boundaryChecker;

// Constants
input [`BIT_WIDTH-1:0] z1_0;
input [`BIT_WIDTH-1:0] z1_1;
input [`BIT_WIDTH-1:0] z1_2;
input [`BIT_WIDTH-1:0] z1_3;
input [`BIT_WIDTH-1:0] z1_4;
input [`BIT_WIDTH-1:0] z1_5;
input [`BIT_WIDTH-1:0] z0_0;
input [`BIT_WIDTH-1:0] z0_1;
input [`BIT_WIDTH-1:0] z0_2;
input [`BIT_WIDTH-1:0] z0_3;
input [`BIT_WIDTH-1:0] z0_4;
input [`BIT_WIDTH-1:0] z0_5;
input [`BIT_WIDTH-1:0] mut_0;
input [`BIT_WIDTH-1:0] mut_1;
input [`BIT_WIDTH-1:0] mut_2;
input [`BIT_WIDTH-1:0] mut_3;
input [`BIT_WIDTH-1:0] mut_4;
input [`BIT_WIDTH-1:0] mut_5;
input [`BIT_WIDTH-1:0] maxDepth_over_maxRadius;


//WIRES FOR CONNECTING REGISTERS
//reg	[BIT_WIDTH-1:0]				c_x	[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				c_y	[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				c_z	[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				c_ux	[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				c_uy	[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				c_uz	[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				c_sz	[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				c_sr	[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				c_sleftz	[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				c_sleftr	[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				c_weight	[TOTAL_LATENCY - 1:0];
//reg	[LAYER_WIDTH-1:0]			c_layer	[TOTAL_LATENCY - 1:0];
//reg								c_dead	[TOTAL_LATENCY - 1:0];
//reg								c_hit	[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				c_diff[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				c_dl_b[TOTAL_LATENCY - 1:0];
//reg	[2*BIT_WIDTH-1:0]			c_numer[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				c_z1[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				c_z0[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				c_mut[TOTAL_LATENCY - 1:0];

//reg	[BIT_WIDTH-1:0]				r_x	[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				r_y	[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				r_z	[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				r_ux	[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				r_uy	[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				r_uz	[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				r_sz	[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				r_sr	[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				r_sleftz	[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				r_sleftr	[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				r_weight	[TOTAL_LATENCY - 1:0];
//reg	[LAYER_WIDTH-1:0]			r_layer	[TOTAL_LATENCY - 1:0];
//reg								r_dead	[TOTAL_LATENCY - 1:0];
//reg								r_hit	[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				r_diff[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				r_dl_b[TOTAL_LATENCY - 1:0];
//reg	[2*BIT_WIDTH-1:0]			r_numer[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				r_z1[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				r_z0[TOTAL_LATENCY - 1:0];
//reg	[BIT_WIDTH-1:0]				r_mut[TOTAL_LATENCY - 1:0];


//EXPANDED FOR ODIN
//reg	[BIT_WIDTH-1:0]				c_x	[TOTAL_LATENCY - 1:0];
reg	[`BIT_WIDTH-1:0]				c_x__0;
reg	[`BIT_WIDTH-1:0]				c_x__1;
reg	[`BIT_WIDTH-1:0]				c_x__2;
reg	[`BIT_WIDTH-1:0]				c_x__3;
reg	[`BIT_WIDTH-1:0]				c_x__4;
reg	[`BIT_WIDTH-1:0]				c_x__5;
reg	[`BIT_WIDTH-1:0]				c_x__6;
reg	[`BIT_WIDTH-1:0]				c_x__7;
reg	[`BIT_WIDTH-1:0]				c_x__8;
reg	[`BIT_WIDTH-1:0]				c_x__9;
reg	[`BIT_WIDTH-1:0]				c_x__10;
reg	[`BIT_WIDTH-1:0]				c_x__11;
reg	[`BIT_WIDTH-1:0]				c_x__12;
reg	[`BIT_WIDTH-1:0]				c_x__13;
reg	[`BIT_WIDTH-1:0]				c_x__14;
reg	[`BIT_WIDTH-1:0]				c_x__15;
reg	[`BIT_WIDTH-1:0]				c_x__16;
reg	[`BIT_WIDTH-1:0]				c_x__17;
reg	[`BIT_WIDTH-1:0]				c_x__18;
reg	[`BIT_WIDTH-1:0]				c_x__19;
reg	[`BIT_WIDTH-1:0]				c_x__20;
reg	[`BIT_WIDTH-1:0]				c_x__21;
reg	[`BIT_WIDTH-1:0]				c_x__22;
reg	[`BIT_WIDTH-1:0]				c_x__23;
reg	[`BIT_WIDTH-1:0]				c_x__24;
reg	[`BIT_WIDTH-1:0]				c_x__25;
reg	[`BIT_WIDTH-1:0]				c_x__26;
reg	[`BIT_WIDTH-1:0]				c_x__27;
reg	[`BIT_WIDTH-1:0]				c_x__28;
reg	[`BIT_WIDTH-1:0]				c_x__29;
reg	[`BIT_WIDTH-1:0]				c_x__30;
reg	[`BIT_WIDTH-1:0]				c_x__31;
reg	[`BIT_WIDTH-1:0]				c_x__32;
reg	[`BIT_WIDTH-1:0]				c_x__33;
reg	[`BIT_WIDTH-1:0]				c_x__34;
reg	[`BIT_WIDTH-1:0]				c_x__35;
reg	[`BIT_WIDTH-1:0]				c_x__36;
reg	[`BIT_WIDTH-1:0]				c_x__37;
reg	[`BIT_WIDTH-1:0]				c_x__38;
reg	[`BIT_WIDTH-1:0]				c_x__39;
reg	[`BIT_WIDTH-1:0]				c_x__40;
reg	[`BIT_WIDTH-1:0]				c_x__41;
reg	[`BIT_WIDTH-1:0]				c_x__42;
reg	[`BIT_WIDTH-1:0]				c_x__43;
reg	[`BIT_WIDTH-1:0]				c_x__44;
reg	[`BIT_WIDTH-1:0]				c_x__45;
reg	[`BIT_WIDTH-1:0]				c_x__46;
reg	[`BIT_WIDTH-1:0]				c_x__47;
reg	[`BIT_WIDTH-1:0]				c_x__48;
reg	[`BIT_WIDTH-1:0]				c_x__49;
reg	[`BIT_WIDTH-1:0]				c_x__50;
reg	[`BIT_WIDTH-1:0]				c_x__51;
reg	[`BIT_WIDTH-1:0]				c_x__52;
reg	[`BIT_WIDTH-1:0]				c_x__53;
reg	[`BIT_WIDTH-1:0]				c_x__54;
reg	[`BIT_WIDTH-1:0]				c_x__55;
reg	[`BIT_WIDTH-1:0]				c_x__56;
reg	[`BIT_WIDTH-1:0]				c_x__57;
reg	[`BIT_WIDTH-1:0]				c_x__58;
reg	[`BIT_WIDTH-1:0]				c_x__59;

//reg	[BIT_WIDTH-1:0]				c_y	[TOTAL_LATENCY - 1:0];


reg	[`BIT_WIDTH-1:0]				c_y__0;
reg	[`BIT_WIDTH-1:0]				c_y__1;
reg	[`BIT_WIDTH-1:0]				c_y__2;
reg	[`BIT_WIDTH-1:0]				c_y__3;
reg	[`BIT_WIDTH-1:0]				c_y__4;
reg	[`BIT_WIDTH-1:0]				c_y__5;
reg	[`BIT_WIDTH-1:0]				c_y__6;
reg	[`BIT_WIDTH-1:0]				c_y__7;
reg	[`BIT_WIDTH-1:0]				c_y__8;
reg	[`BIT_WIDTH-1:0]				c_y__9;
reg	[`BIT_WIDTH-1:0]				c_y__10;
reg	[`BIT_WIDTH-1:0]				c_y__11;
reg	[`BIT_WIDTH-1:0]				c_y__12;
reg	[`BIT_WIDTH-1:0]				c_y__13;
reg	[`BIT_WIDTH-1:0]				c_y__14;
reg	[`BIT_WIDTH-1:0]				c_y__15;
reg	[`BIT_WIDTH-1:0]				c_y__16;
reg	[`BIT_WIDTH-1:0]				c_y__17;
reg	[`BIT_WIDTH-1:0]				c_y__18;
reg	[`BIT_WIDTH-1:0]				c_y__19;
reg	[`BIT_WIDTH-1:0]				c_y__20;
reg	[`BIT_WIDTH-1:0]				c_y__21;
reg	[`BIT_WIDTH-1:0]				c_y__22;
reg	[`BIT_WIDTH-1:0]				c_y__23;
reg	[`BIT_WIDTH-1:0]				c_y__24;
reg	[`BIT_WIDTH-1:0]				c_y__25;
reg	[`BIT_WIDTH-1:0]				c_y__26;
reg	[`BIT_WIDTH-1:0]				c_y__27;
reg	[`BIT_WIDTH-1:0]				c_y__28;
reg	[`BIT_WIDTH-1:0]				c_y__29;
reg	[`BIT_WIDTH-1:0]				c_y__30;
reg	[`BIT_WIDTH-1:0]				c_y__31;
reg	[`BIT_WIDTH-1:0]				c_y__32;
reg	[`BIT_WIDTH-1:0]				c_y__33;
reg	[`BIT_WIDTH-1:0]				c_y__34;
reg	[`BIT_WIDTH-1:0]				c_y__35;
reg	[`BIT_WIDTH-1:0]				c_y__36;
reg	[`BIT_WIDTH-1:0]				c_y__37;
reg	[`BIT_WIDTH-1:0]				c_y__38;
reg	[`BIT_WIDTH-1:0]				c_y__39;
reg	[`BIT_WIDTH-1:0]				c_y__40;
reg	[`BIT_WIDTH-1:0]				c_y__41;
reg	[`BIT_WIDTH-1:0]				c_y__42;
reg	[`BIT_WIDTH-1:0]				c_y__43;
reg	[`BIT_WIDTH-1:0]				c_y__44;
reg	[`BIT_WIDTH-1:0]				c_y__45;
reg	[`BIT_WIDTH-1:0]				c_y__46;
reg	[`BIT_WIDTH-1:0]				c_y__47;
reg	[`BIT_WIDTH-1:0]				c_y__48;
reg	[`BIT_WIDTH-1:0]				c_y__49;
reg	[`BIT_WIDTH-1:0]				c_y__50;
reg	[`BIT_WIDTH-1:0]				c_y__51;
reg	[`BIT_WIDTH-1:0]				c_y__52;
reg	[`BIT_WIDTH-1:0]				c_y__53;
reg	[`BIT_WIDTH-1:0]				c_y__54;
reg	[`BIT_WIDTH-1:0]				c_y__55;
reg	[`BIT_WIDTH-1:0]				c_y__56;
reg	[`BIT_WIDTH-1:0]				c_y__57;
reg	[`BIT_WIDTH-1:0]				c_y__58;
reg	[`BIT_WIDTH-1:0]				c_y__59;


//reg	[BIT_WIDTH-1:0]				c_z	[TOTAL_LATENCY - 1:0];


reg	[`BIT_WIDTH-1:0]				c_z__0;
reg	[`BIT_WIDTH-1:0]				c_z__1;
reg	[`BIT_WIDTH-1:0]				c_z__2;
reg	[`BIT_WIDTH-1:0]				c_z__3;
reg	[`BIT_WIDTH-1:0]				c_z__4;
reg	[`BIT_WIDTH-1:0]				c_z__5;
reg	[`BIT_WIDTH-1:0]				c_z__6;
reg	[`BIT_WIDTH-1:0]				c_z__7;
reg	[`BIT_WIDTH-1:0]				c_z__8;
reg	[`BIT_WIDTH-1:0]				c_z__9;
reg	[`BIT_WIDTH-1:0]				c_z__10;
reg	[`BIT_WIDTH-1:0]				c_z__11;
reg	[`BIT_WIDTH-1:0]				c_z__12;
reg	[`BIT_WIDTH-1:0]				c_z__13;
reg	[`BIT_WIDTH-1:0]				c_z__14;
reg	[`BIT_WIDTH-1:0]				c_z__15;
reg	[`BIT_WIDTH-1:0]				c_z__16;
reg	[`BIT_WIDTH-1:0]				c_z__17;
reg	[`BIT_WIDTH-1:0]				c_z__18;
reg	[`BIT_WIDTH-1:0]				c_z__19;
reg	[`BIT_WIDTH-1:0]				c_z__20;
reg	[`BIT_WIDTH-1:0]				c_z__21;
reg	[`BIT_WIDTH-1:0]				c_z__22;
reg	[`BIT_WIDTH-1:0]				c_z__23;
reg	[`BIT_WIDTH-1:0]				c_z__24;
reg	[`BIT_WIDTH-1:0]				c_z__25;
reg	[`BIT_WIDTH-1:0]				c_z__26;
reg	[`BIT_WIDTH-1:0]				c_z__27;
reg	[`BIT_WIDTH-1:0]				c_z__28;
reg	[`BIT_WIDTH-1:0]				c_z__29;
reg	[`BIT_WIDTH-1:0]				c_z__30;
reg	[`BIT_WIDTH-1:0]				c_z__31;
reg	[`BIT_WIDTH-1:0]				c_z__32;
reg	[`BIT_WIDTH-1:0]				c_z__33;
reg	[`BIT_WIDTH-1:0]				c_z__34;
reg	[`BIT_WIDTH-1:0]				c_z__35;
reg	[`BIT_WIDTH-1:0]				c_z__36;
reg	[`BIT_WIDTH-1:0]				c_z__37;
reg	[`BIT_WIDTH-1:0]				c_z__38;
reg	[`BIT_WIDTH-1:0]				c_z__39;
reg	[`BIT_WIDTH-1:0]				c_z__40;
reg	[`BIT_WIDTH-1:0]				c_z__41;
reg	[`BIT_WIDTH-1:0]				c_z__42;
reg	[`BIT_WIDTH-1:0]				c_z__43;
reg	[`BIT_WIDTH-1:0]				c_z__44;
reg	[`BIT_WIDTH-1:0]				c_z__45;
reg	[`BIT_WIDTH-1:0]				c_z__46;
reg	[`BIT_WIDTH-1:0]				c_z__47;
reg	[`BIT_WIDTH-1:0]				c_z__48;
reg	[`BIT_WIDTH-1:0]				c_z__49;
reg	[`BIT_WIDTH-1:0]				c_z__50;
reg	[`BIT_WIDTH-1:0]				c_z__51;
reg	[`BIT_WIDTH-1:0]				c_z__52;
reg	[`BIT_WIDTH-1:0]				c_z__53;
reg	[`BIT_WIDTH-1:0]				c_z__54;
reg	[`BIT_WIDTH-1:0]				c_z__55;
reg	[`BIT_WIDTH-1:0]				c_z__56;
reg	[`BIT_WIDTH-1:0]				c_z__57;
reg	[`BIT_WIDTH-1:0]				c_z__58;
reg	[`BIT_WIDTH-1:0]				c_z__59;



//reg	[`BIT_WIDTH-1:0]				c_ux	[TOTAL_LATENCY - 1:0];

reg	[`BIT_WIDTH-1:0]				c_ux__0;
reg	[`BIT_WIDTH-1:0]				c_ux__1;
reg	[`BIT_WIDTH-1:0]				c_ux__2;
reg	[`BIT_WIDTH-1:0]				c_ux__3;
reg	[`BIT_WIDTH-1:0]				c_ux__4;
reg	[`BIT_WIDTH-1:0]				c_ux__5;
reg	[`BIT_WIDTH-1:0]				c_ux__6;
reg	[`BIT_WIDTH-1:0]				c_ux__7;
reg	[`BIT_WIDTH-1:0]				c_ux__8;
reg	[`BIT_WIDTH-1:0]				c_ux__9;
reg	[`BIT_WIDTH-1:0]				c_ux__10;
reg	[`BIT_WIDTH-1:0]				c_ux__11;
reg	[`BIT_WIDTH-1:0]				c_ux__12;
reg	[`BIT_WIDTH-1:0]				c_ux__13;
reg	[`BIT_WIDTH-1:0]				c_ux__14;
reg	[`BIT_WIDTH-1:0]				c_ux__15;
reg	[`BIT_WIDTH-1:0]				c_ux__16;
reg	[`BIT_WIDTH-1:0]				c_ux__17;
reg	[`BIT_WIDTH-1:0]				c_ux__18;
reg	[`BIT_WIDTH-1:0]				c_ux__19;
reg	[`BIT_WIDTH-1:0]				c_ux__20;
reg	[`BIT_WIDTH-1:0]				c_ux__21;
reg	[`BIT_WIDTH-1:0]				c_ux__22;
reg	[`BIT_WIDTH-1:0]				c_ux__23;
reg	[`BIT_WIDTH-1:0]				c_ux__24;
reg	[`BIT_WIDTH-1:0]				c_ux__25;
reg	[`BIT_WIDTH-1:0]				c_ux__26;
reg	[`BIT_WIDTH-1:0]				c_ux__27;
reg	[`BIT_WIDTH-1:0]				c_ux__28;
reg	[`BIT_WIDTH-1:0]				c_ux__29;
reg	[`BIT_WIDTH-1:0]				c_ux__30;
reg	[`BIT_WIDTH-1:0]				c_ux__31;
reg	[`BIT_WIDTH-1:0]				c_ux__32;
reg	[`BIT_WIDTH-1:0]				c_ux__33;
reg	[`BIT_WIDTH-1:0]				c_ux__34;
reg	[`BIT_WIDTH-1:0]				c_ux__35;
reg	[`BIT_WIDTH-1:0]				c_ux__36;
reg	[`BIT_WIDTH-1:0]				c_ux__37;
reg	[`BIT_WIDTH-1:0]				c_ux__38;
reg	[`BIT_WIDTH-1:0]				c_ux__39;
reg	[`BIT_WIDTH-1:0]				c_ux__40;
reg	[`BIT_WIDTH-1:0]				c_ux__41;
reg	[`BIT_WIDTH-1:0]				c_ux__42;
reg	[`BIT_WIDTH-1:0]				c_ux__43;
reg	[`BIT_WIDTH-1:0]				c_ux__44;
reg	[`BIT_WIDTH-1:0]				c_ux__45;
reg	[`BIT_WIDTH-1:0]				c_ux__46;
reg	[`BIT_WIDTH-1:0]				c_ux__47;
reg	[`BIT_WIDTH-1:0]				c_ux__48;
reg	[`BIT_WIDTH-1:0]				c_ux__49;
reg	[`BIT_WIDTH-1:0]				c_ux__50;
reg	[`BIT_WIDTH-1:0]				c_ux__51;
reg	[`BIT_WIDTH-1:0]				c_ux__52;
reg	[`BIT_WIDTH-1:0]				c_ux__53;
reg	[`BIT_WIDTH-1:0]				c_ux__54;
reg	[`BIT_WIDTH-1:0]				c_ux__55;
reg	[`BIT_WIDTH-1:0]				c_ux__56;
reg	[`BIT_WIDTH-1:0]				c_ux__57;
reg	[`BIT_WIDTH-1:0]				c_ux__58;
reg	[`BIT_WIDTH-1:0]				c_ux__59;
//reg	[`BIT_WIDTH-1:0]				c_uy	[TOTAL_LATENCY - 1:0];

reg	[`BIT_WIDTH-1:0]				c_uy__0;
reg	[`BIT_WIDTH-1:0]				c_uy__1;
reg	[`BIT_WIDTH-1:0]				c_uy__2;
reg	[`BIT_WIDTH-1:0]				c_uy__3;
reg	[`BIT_WIDTH-1:0]				c_uy__4;
reg	[`BIT_WIDTH-1:0]				c_uy__5;
reg	[`BIT_WIDTH-1:0]				c_uy__6;
reg	[`BIT_WIDTH-1:0]				c_uy__7;
reg	[`BIT_WIDTH-1:0]				c_uy__8;
reg	[`BIT_WIDTH-1:0]				c_uy__9;
reg	[`BIT_WIDTH-1:0]				c_uy__10;
reg	[`BIT_WIDTH-1:0]				c_uy__11;
reg	[`BIT_WIDTH-1:0]				c_uy__12;
reg	[`BIT_WIDTH-1:0]				c_uy__13;
reg	[`BIT_WIDTH-1:0]				c_uy__14;
reg	[`BIT_WIDTH-1:0]				c_uy__15;
reg	[`BIT_WIDTH-1:0]				c_uy__16;
reg	[`BIT_WIDTH-1:0]				c_uy__17;
reg	[`BIT_WIDTH-1:0]				c_uy__18;
reg	[`BIT_WIDTH-1:0]				c_uy__19;
reg	[`BIT_WIDTH-1:0]				c_uy__20;
reg	[`BIT_WIDTH-1:0]				c_uy__21;
reg	[`BIT_WIDTH-1:0]				c_uy__22;
reg	[`BIT_WIDTH-1:0]				c_uy__23;
reg	[`BIT_WIDTH-1:0]				c_uy__24;
reg	[`BIT_WIDTH-1:0]				c_uy__25;
reg	[`BIT_WIDTH-1:0]				c_uy__26;
reg	[`BIT_WIDTH-1:0]				c_uy__27;
reg	[`BIT_WIDTH-1:0]				c_uy__28;
reg	[`BIT_WIDTH-1:0]				c_uy__29;
reg	[`BIT_WIDTH-1:0]				c_uy__30;
reg	[`BIT_WIDTH-1:0]				c_uy__31;
reg	[`BIT_WIDTH-1:0]				c_uy__32;
reg	[`BIT_WIDTH-1:0]				c_uy__33;
reg	[`BIT_WIDTH-1:0]				c_uy__34;
reg	[`BIT_WIDTH-1:0]				c_uy__35;
reg	[`BIT_WIDTH-1:0]				c_uy__36;
reg	[`BIT_WIDTH-1:0]				c_uy__37;
reg	[`BIT_WIDTH-1:0]				c_uy__38;
reg	[`BIT_WIDTH-1:0]				c_uy__39;
reg	[`BIT_WIDTH-1:0]				c_uy__40;
reg	[`BIT_WIDTH-1:0]				c_uy__41;
reg	[`BIT_WIDTH-1:0]				c_uy__42;
reg	[`BIT_WIDTH-1:0]				c_uy__43;
reg	[`BIT_WIDTH-1:0]				c_uy__44;
reg	[`BIT_WIDTH-1:0]				c_uy__45;
reg	[`BIT_WIDTH-1:0]				c_uy__46;
reg	[`BIT_WIDTH-1:0]				c_uy__47;
reg	[`BIT_WIDTH-1:0]				c_uy__48;
reg	[`BIT_WIDTH-1:0]				c_uy__49;
reg	[`BIT_WIDTH-1:0]				c_uy__50;
reg	[`BIT_WIDTH-1:0]				c_uy__51;
reg	[`BIT_WIDTH-1:0]				c_uy__52;
reg	[`BIT_WIDTH-1:0]				c_uy__53;
reg	[`BIT_WIDTH-1:0]				c_uy__54;
reg	[`BIT_WIDTH-1:0]				c_uy__55;
reg	[`BIT_WIDTH-1:0]				c_uy__56;
reg	[`BIT_WIDTH-1:0]				c_uy__57;
reg	[`BIT_WIDTH-1:0]				c_uy__58;
reg	[`BIT_WIDTH-1:0]				c_uy__59;
//reg	[`BIT_WIDTH-1:0]				c_uz	[TOTAL_LATENCY - 1:0];

reg	[`BIT_WIDTH-1:0]				c_uz__0;
reg	[`BIT_WIDTH-1:0]				c_uz__1;
reg	[`BIT_WIDTH-1:0]				c_uz__2;
reg	[`BIT_WIDTH-1:0]				c_uz__3;
reg	[`BIT_WIDTH-1:0]				c_uz__4;
reg	[`BIT_WIDTH-1:0]				c_uz__5;
reg	[`BIT_WIDTH-1:0]				c_uz__6;
reg	[`BIT_WIDTH-1:0]				c_uz__7;
reg	[`BIT_WIDTH-1:0]				c_uz__8;
reg	[`BIT_WIDTH-1:0]				c_uz__9;
reg	[`BIT_WIDTH-1:0]				c_uz__10;
reg	[`BIT_WIDTH-1:0]				c_uz__11;
reg	[`BIT_WIDTH-1:0]				c_uz__12;
reg	[`BIT_WIDTH-1:0]				c_uz__13;
reg	[`BIT_WIDTH-1:0]				c_uz__14;
reg	[`BIT_WIDTH-1:0]				c_uz__15;
reg	[`BIT_WIDTH-1:0]				c_uz__16;
reg	[`BIT_WIDTH-1:0]				c_uz__17;
reg	[`BIT_WIDTH-1:0]				c_uz__18;
reg	[`BIT_WIDTH-1:0]				c_uz__19;
reg	[`BIT_WIDTH-1:0]				c_uz__20;
reg	[`BIT_WIDTH-1:0]				c_uz__21;
reg	[`BIT_WIDTH-1:0]				c_uz__22;
reg	[`BIT_WIDTH-1:0]				c_uz__23;
reg	[`BIT_WIDTH-1:0]				c_uz__24;
reg	[`BIT_WIDTH-1:0]				c_uz__25;
reg	[`BIT_WIDTH-1:0]				c_uz__26;
reg	[`BIT_WIDTH-1:0]				c_uz__27;
reg	[`BIT_WIDTH-1:0]				c_uz__28;
reg	[`BIT_WIDTH-1:0]				c_uz__29;
reg	[`BIT_WIDTH-1:0]				c_uz__30;
reg	[`BIT_WIDTH-1:0]				c_uz__31;
reg	[`BIT_WIDTH-1:0]				c_uz__32;
reg	[`BIT_WIDTH-1:0]				c_uz__33;
reg	[`BIT_WIDTH-1:0]				c_uz__34;
reg	[`BIT_WIDTH-1:0]				c_uz__35;
reg	[`BIT_WIDTH-1:0]				c_uz__36;
reg	[`BIT_WIDTH-1:0]				c_uz__37;
reg	[`BIT_WIDTH-1:0]				c_uz__38;
reg	[`BIT_WIDTH-1:0]				c_uz__39;
reg	[`BIT_WIDTH-1:0]				c_uz__40;
reg	[`BIT_WIDTH-1:0]				c_uz__41;
reg	[`BIT_WIDTH-1:0]				c_uz__42;
reg	[`BIT_WIDTH-1:0]				c_uz__43;
reg	[`BIT_WIDTH-1:0]				c_uz__44;
reg	[`BIT_WIDTH-1:0]				c_uz__45;
reg	[`BIT_WIDTH-1:0]				c_uz__46;
reg	[`BIT_WIDTH-1:0]				c_uz__47;
reg	[`BIT_WIDTH-1:0]				c_uz__48;
reg	[`BIT_WIDTH-1:0]				c_uz__49;
reg	[`BIT_WIDTH-1:0]				c_uz__50;
reg	[`BIT_WIDTH-1:0]				c_uz__51;
reg	[`BIT_WIDTH-1:0]				c_uz__52;
reg	[`BIT_WIDTH-1:0]				c_uz__53;
reg	[`BIT_WIDTH-1:0]				c_uz__54;
reg	[`BIT_WIDTH-1:0]				c_uz__55;
reg	[`BIT_WIDTH-1:0]				c_uz__56;
reg	[`BIT_WIDTH-1:0]				c_uz__57;
reg	[`BIT_WIDTH-1:0]				c_uz__58;
reg	[`BIT_WIDTH-1:0]				c_uz__59;
//reg	[`BIT_WIDTH-1:0]				c_sz	[TOTAL_LATENCY - 1:0];

reg	[`BIT_WIDTH-1:0]				c_sz__0;
reg	[`BIT_WIDTH-1:0]				c_sz__1;
reg	[`BIT_WIDTH-1:0]				c_sz__2;
reg	[`BIT_WIDTH-1:0]				c_sz__3;
reg	[`BIT_WIDTH-1:0]				c_sz__4;
reg	[`BIT_WIDTH-1:0]				c_sz__5;
reg	[`BIT_WIDTH-1:0]				c_sz__6;
reg	[`BIT_WIDTH-1:0]				c_sz__7;
reg	[`BIT_WIDTH-1:0]				c_sz__8;
reg	[`BIT_WIDTH-1:0]				c_sz__9;
reg	[`BIT_WIDTH-1:0]				c_sz__10;
reg	[`BIT_WIDTH-1:0]				c_sz__11;
reg	[`BIT_WIDTH-1:0]				c_sz__12;
reg	[`BIT_WIDTH-1:0]				c_sz__13;
reg	[`BIT_WIDTH-1:0]				c_sz__14;
reg	[`BIT_WIDTH-1:0]				c_sz__15;
reg	[`BIT_WIDTH-1:0]				c_sz__16;
reg	[`BIT_WIDTH-1:0]				c_sz__17;
reg	[`BIT_WIDTH-1:0]				c_sz__18;
reg	[`BIT_WIDTH-1:0]				c_sz__19;
reg	[`BIT_WIDTH-1:0]				c_sz__20;
reg	[`BIT_WIDTH-1:0]				c_sz__21;
reg	[`BIT_WIDTH-1:0]				c_sz__22;
reg	[`BIT_WIDTH-1:0]				c_sz__23;
reg	[`BIT_WIDTH-1:0]				c_sz__24;
reg	[`BIT_WIDTH-1:0]				c_sz__25;
reg	[`BIT_WIDTH-1:0]				c_sz__26;
reg	[`BIT_WIDTH-1:0]				c_sz__27;
reg	[`BIT_WIDTH-1:0]				c_sz__28;
reg	[`BIT_WIDTH-1:0]				c_sz__29;
reg	[`BIT_WIDTH-1:0]				c_sz__30;
reg	[`BIT_WIDTH-1:0]				c_sz__31;
reg	[`BIT_WIDTH-1:0]				c_sz__32;
reg	[`BIT_WIDTH-1:0]				c_sz__33;
reg	[`BIT_WIDTH-1:0]				c_sz__34;
reg	[`BIT_WIDTH-1:0]				c_sz__35;
reg	[`BIT_WIDTH-1:0]				c_sz__36;
reg	[`BIT_WIDTH-1:0]				c_sz__37;
reg	[`BIT_WIDTH-1:0]				c_sz__38;
reg	[`BIT_WIDTH-1:0]				c_sz__39;
reg	[`BIT_WIDTH-1:0]				c_sz__40;
reg	[`BIT_WIDTH-1:0]				c_sz__41;
reg	[`BIT_WIDTH-1:0]				c_sz__42;
reg	[`BIT_WIDTH-1:0]				c_sz__43;
reg	[`BIT_WIDTH-1:0]				c_sz__44;
reg	[`BIT_WIDTH-1:0]				c_sz__45;
reg	[`BIT_WIDTH-1:0]				c_sz__46;
reg	[`BIT_WIDTH-1:0]				c_sz__47;
reg	[`BIT_WIDTH-1:0]				c_sz__48;
reg	[`BIT_WIDTH-1:0]				c_sz__49;
reg	[`BIT_WIDTH-1:0]				c_sz__50;
reg	[`BIT_WIDTH-1:0]				c_sz__51;
reg	[`BIT_WIDTH-1:0]				c_sz__52;
reg	[`BIT_WIDTH-1:0]				c_sz__53;
reg	[`BIT_WIDTH-1:0]				c_sz__54;
reg	[`BIT_WIDTH-1:0]				c_sz__55;
reg	[`BIT_WIDTH-1:0]				c_sz__56;
reg	[`BIT_WIDTH-1:0]				c_sz__57;
reg	[`BIT_WIDTH-1:0]				c_sz__58;
reg	[`BIT_WIDTH-1:0]				c_sz__59;
//reg	[`BIT_WIDTH-1:0]				c_sr	[TOTAL_LATENCY - 1:0];

reg	[`BIT_WIDTH-1:0]				c_sr__0;
reg	[`BIT_WIDTH-1:0]				c_sr__1;
reg	[`BIT_WIDTH-1:0]				c_sr__2;
reg	[`BIT_WIDTH-1:0]				c_sr__3;
reg	[`BIT_WIDTH-1:0]				c_sr__4;
reg	[`BIT_WIDTH-1:0]				c_sr__5;
reg	[`BIT_WIDTH-1:0]				c_sr__6;
reg	[`BIT_WIDTH-1:0]				c_sr__7;
reg	[`BIT_WIDTH-1:0]				c_sr__8;
reg	[`BIT_WIDTH-1:0]				c_sr__9;
reg	[`BIT_WIDTH-1:0]				c_sr__10;
reg	[`BIT_WIDTH-1:0]				c_sr__11;
reg	[`BIT_WIDTH-1:0]				c_sr__12;
reg	[`BIT_WIDTH-1:0]				c_sr__13;
reg	[`BIT_WIDTH-1:0]				c_sr__14;
reg	[`BIT_WIDTH-1:0]				c_sr__15;
reg	[`BIT_WIDTH-1:0]				c_sr__16;
reg	[`BIT_WIDTH-1:0]				c_sr__17;
reg	[`BIT_WIDTH-1:0]				c_sr__18;
reg	[`BIT_WIDTH-1:0]				c_sr__19;
reg	[`BIT_WIDTH-1:0]				c_sr__20;
reg	[`BIT_WIDTH-1:0]				c_sr__21;
reg	[`BIT_WIDTH-1:0]				c_sr__22;
reg	[`BIT_WIDTH-1:0]				c_sr__23;
reg	[`BIT_WIDTH-1:0]				c_sr__24;
reg	[`BIT_WIDTH-1:0]				c_sr__25;
reg	[`BIT_WIDTH-1:0]				c_sr__26;
reg	[`BIT_WIDTH-1:0]				c_sr__27;
reg	[`BIT_WIDTH-1:0]				c_sr__28;
reg	[`BIT_WIDTH-1:0]				c_sr__29;
reg	[`BIT_WIDTH-1:0]				c_sr__30;
reg	[`BIT_WIDTH-1:0]				c_sr__31;
reg	[`BIT_WIDTH-1:0]				c_sr__32;
reg	[`BIT_WIDTH-1:0]				c_sr__33;
reg	[`BIT_WIDTH-1:0]				c_sr__34;
reg	[`BIT_WIDTH-1:0]				c_sr__35;
reg	[`BIT_WIDTH-1:0]				c_sr__36;
reg	[`BIT_WIDTH-1:0]				c_sr__37;
reg	[`BIT_WIDTH-1:0]				c_sr__38;
reg	[`BIT_WIDTH-1:0]				c_sr__39;
reg	[`BIT_WIDTH-1:0]				c_sr__40;
reg	[`BIT_WIDTH-1:0]				c_sr__41;
reg	[`BIT_WIDTH-1:0]				c_sr__42;
reg	[`BIT_WIDTH-1:0]				c_sr__43;
reg	[`BIT_WIDTH-1:0]				c_sr__44;
reg	[`BIT_WIDTH-1:0]				c_sr__45;
reg	[`BIT_WIDTH-1:0]				c_sr__46;
reg	[`BIT_WIDTH-1:0]				c_sr__47;
reg	[`BIT_WIDTH-1:0]				c_sr__48;
reg	[`BIT_WIDTH-1:0]				c_sr__49;
reg	[`BIT_WIDTH-1:0]				c_sr__50;
reg	[`BIT_WIDTH-1:0]				c_sr__51;
reg	[`BIT_WIDTH-1:0]				c_sr__52;
reg	[`BIT_WIDTH-1:0]				c_sr__53;
reg	[`BIT_WIDTH-1:0]				c_sr__54;
reg	[`BIT_WIDTH-1:0]				c_sr__55;
reg	[`BIT_WIDTH-1:0]				c_sr__56;
reg	[`BIT_WIDTH-1:0]				c_sr__57;
reg	[`BIT_WIDTH-1:0]				c_sr__58;
reg	[`BIT_WIDTH-1:0]				c_sr__59;
//reg	[`BIT_WIDTH-1:0]				c_sleftz	[TOTAL_LATENCY - 1:0];

reg	[`BIT_WIDTH-1:0]				c_sleftz__0;
reg	[`BIT_WIDTH-1:0]				c_sleftz__1;
reg	[`BIT_WIDTH-1:0]				c_sleftz__2;
reg	[`BIT_WIDTH-1:0]				c_sleftz__3;
reg	[`BIT_WIDTH-1:0]				c_sleftz__4;
reg	[`BIT_WIDTH-1:0]				c_sleftz__5;
reg	[`BIT_WIDTH-1:0]				c_sleftz__6;
reg	[`BIT_WIDTH-1:0]				c_sleftz__7;
reg	[`BIT_WIDTH-1:0]				c_sleftz__8;
reg	[`BIT_WIDTH-1:0]				c_sleftz__9;
reg	[`BIT_WIDTH-1:0]				c_sleftz__10;
reg	[`BIT_WIDTH-1:0]				c_sleftz__11;
reg	[`BIT_WIDTH-1:0]				c_sleftz__12;
reg	[`BIT_WIDTH-1:0]				c_sleftz__13;
reg	[`BIT_WIDTH-1:0]				c_sleftz__14;
reg	[`BIT_WIDTH-1:0]				c_sleftz__15;
reg	[`BIT_WIDTH-1:0]				c_sleftz__16;
reg	[`BIT_WIDTH-1:0]				c_sleftz__17;
reg	[`BIT_WIDTH-1:0]				c_sleftz__18;
reg	[`BIT_WIDTH-1:0]				c_sleftz__19;
reg	[`BIT_WIDTH-1:0]				c_sleftz__20;
reg	[`BIT_WIDTH-1:0]				c_sleftz__21;
reg	[`BIT_WIDTH-1:0]				c_sleftz__22;
reg	[`BIT_WIDTH-1:0]				c_sleftz__23;
reg	[`BIT_WIDTH-1:0]				c_sleftz__24;
reg	[`BIT_WIDTH-1:0]				c_sleftz__25;
reg	[`BIT_WIDTH-1:0]				c_sleftz__26;
reg	[`BIT_WIDTH-1:0]				c_sleftz__27;
reg	[`BIT_WIDTH-1:0]				c_sleftz__28;
reg	[`BIT_WIDTH-1:0]				c_sleftz__29;
reg	[`BIT_WIDTH-1:0]				c_sleftz__30;
reg	[`BIT_WIDTH-1:0]				c_sleftz__31;
reg	[`BIT_WIDTH-1:0]				c_sleftz__32;
reg	[`BIT_WIDTH-1:0]				c_sleftz__33;
reg	[`BIT_WIDTH-1:0]				c_sleftz__34;
reg	[`BIT_WIDTH-1:0]				c_sleftz__35;
reg	[`BIT_WIDTH-1:0]				c_sleftz__36;
reg	[`BIT_WIDTH-1:0]				c_sleftz__37;
reg	[`BIT_WIDTH-1:0]				c_sleftz__38;
reg	[`BIT_WIDTH-1:0]				c_sleftz__39;
reg	[`BIT_WIDTH-1:0]				c_sleftz__40;
reg	[`BIT_WIDTH-1:0]				c_sleftz__41;
reg	[`BIT_WIDTH-1:0]				c_sleftz__42;
reg	[`BIT_WIDTH-1:0]				c_sleftz__43;
reg	[`BIT_WIDTH-1:0]				c_sleftz__44;
reg	[`BIT_WIDTH-1:0]				c_sleftz__45;
reg	[`BIT_WIDTH-1:0]				c_sleftz__46;
reg	[`BIT_WIDTH-1:0]				c_sleftz__47;
reg	[`BIT_WIDTH-1:0]				c_sleftz__48;
reg	[`BIT_WIDTH-1:0]				c_sleftz__49;
reg	[`BIT_WIDTH-1:0]				c_sleftz__50;
reg	[`BIT_WIDTH-1:0]				c_sleftz__51;
reg	[`BIT_WIDTH-1:0]				c_sleftz__52;
reg	[`BIT_WIDTH-1:0]				c_sleftz__53;
reg	[`BIT_WIDTH-1:0]				c_sleftz__54;
reg	[`BIT_WIDTH-1:0]				c_sleftz__55;
reg	[`BIT_WIDTH-1:0]				c_sleftz__56;
reg	[`BIT_WIDTH-1:0]				c_sleftz__57;
reg	[`BIT_WIDTH-1:0]				c_sleftz__58;
reg	[`BIT_WIDTH-1:0]				c_sleftz__59;
//reg	[`BIT_WIDTH-1:0]				c_sleftr	[TOTAL_LATENCY - 1:0];

reg	[`BIT_WIDTH-1:0]				c_sleftr__0;
reg	[`BIT_WIDTH-1:0]				c_sleftr__1;
reg	[`BIT_WIDTH-1:0]				c_sleftr__2;
reg	[`BIT_WIDTH-1:0]				c_sleftr__3;
reg	[`BIT_WIDTH-1:0]				c_sleftr__4;
reg	[`BIT_WIDTH-1:0]				c_sleftr__5;
reg	[`BIT_WIDTH-1:0]				c_sleftr__6;
reg	[`BIT_WIDTH-1:0]				c_sleftr__7;
reg	[`BIT_WIDTH-1:0]				c_sleftr__8;
reg	[`BIT_WIDTH-1:0]				c_sleftr__9;
reg	[`BIT_WIDTH-1:0]				c_sleftr__10;
reg	[`BIT_WIDTH-1:0]				c_sleftr__11;
reg	[`BIT_WIDTH-1:0]				c_sleftr__12;
reg	[`BIT_WIDTH-1:0]				c_sleftr__13;
reg	[`BIT_WIDTH-1:0]				c_sleftr__14;
reg	[`BIT_WIDTH-1:0]				c_sleftr__15;
reg	[`BIT_WIDTH-1:0]				c_sleftr__16;
reg	[`BIT_WIDTH-1:0]				c_sleftr__17;
reg	[`BIT_WIDTH-1:0]				c_sleftr__18;
reg	[`BIT_WIDTH-1:0]				c_sleftr__19;
reg	[`BIT_WIDTH-1:0]				c_sleftr__20;
reg	[`BIT_WIDTH-1:0]				c_sleftr__21;
reg	[`BIT_WIDTH-1:0]				c_sleftr__22;
reg	[`BIT_WIDTH-1:0]				c_sleftr__23;
reg	[`BIT_WIDTH-1:0]				c_sleftr__24;
reg	[`BIT_WIDTH-1:0]				c_sleftr__25;
reg	[`BIT_WIDTH-1:0]				c_sleftr__26;
reg	[`BIT_WIDTH-1:0]				c_sleftr__27;
reg	[`BIT_WIDTH-1:0]				c_sleftr__28;
reg	[`BIT_WIDTH-1:0]				c_sleftr__29;
reg	[`BIT_WIDTH-1:0]				c_sleftr__30;
reg	[`BIT_WIDTH-1:0]				c_sleftr__31;
reg	[`BIT_WIDTH-1:0]				c_sleftr__32;
reg	[`BIT_WIDTH-1:0]				c_sleftr__33;
reg	[`BIT_WIDTH-1:0]				c_sleftr__34;
reg	[`BIT_WIDTH-1:0]				c_sleftr__35;
reg	[`BIT_WIDTH-1:0]				c_sleftr__36;
reg	[`BIT_WIDTH-1:0]				c_sleftr__37;
reg	[`BIT_WIDTH-1:0]				c_sleftr__38;
reg	[`BIT_WIDTH-1:0]				c_sleftr__39;
reg	[`BIT_WIDTH-1:0]				c_sleftr__40;
reg	[`BIT_WIDTH-1:0]				c_sleftr__41;
reg	[`BIT_WIDTH-1:0]				c_sleftr__42;
reg	[`BIT_WIDTH-1:0]				c_sleftr__43;
reg	[`BIT_WIDTH-1:0]				c_sleftr__44;
reg	[`BIT_WIDTH-1:0]				c_sleftr__45;
reg	[`BIT_WIDTH-1:0]				c_sleftr__46;
reg	[`BIT_WIDTH-1:0]				c_sleftr__47;
reg	[`BIT_WIDTH-1:0]				c_sleftr__48;
reg	[`BIT_WIDTH-1:0]				c_sleftr__49;
reg	[`BIT_WIDTH-1:0]				c_sleftr__50;
reg	[`BIT_WIDTH-1:0]				c_sleftr__51;
reg	[`BIT_WIDTH-1:0]				c_sleftr__52;
reg	[`BIT_WIDTH-1:0]				c_sleftr__53;
reg	[`BIT_WIDTH-1:0]				c_sleftr__54;
reg	[`BIT_WIDTH-1:0]				c_sleftr__55;
reg	[`BIT_WIDTH-1:0]				c_sleftr__56;
reg	[`BIT_WIDTH-1:0]				c_sleftr__57;
reg	[`BIT_WIDTH-1:0]				c_sleftr__58;
reg	[`BIT_WIDTH-1:0]				c_sleftr__59;
//reg	[`BIT_WIDTH-1:0]				c_weight	[TOTAL_LATENCY - 1:0];

reg	[`BIT_WIDTH-1:0]				c_weight__0;
reg	[`BIT_WIDTH-1:0]				c_weight__1;
reg	[`BIT_WIDTH-1:0]				c_weight__2;
reg	[`BIT_WIDTH-1:0]				c_weight__3;
reg	[`BIT_WIDTH-1:0]				c_weight__4;
reg	[`BIT_WIDTH-1:0]				c_weight__5;
reg	[`BIT_WIDTH-1:0]				c_weight__6;
reg	[`BIT_WIDTH-1:0]				c_weight__7;
reg	[`BIT_WIDTH-1:0]				c_weight__8;
reg	[`BIT_WIDTH-1:0]				c_weight__9;
reg	[`BIT_WIDTH-1:0]				c_weight__10;
reg	[`BIT_WIDTH-1:0]				c_weight__11;
reg	[`BIT_WIDTH-1:0]				c_weight__12;
reg	[`BIT_WIDTH-1:0]				c_weight__13;
reg	[`BIT_WIDTH-1:0]				c_weight__14;
reg	[`BIT_WIDTH-1:0]				c_weight__15;
reg	[`BIT_WIDTH-1:0]				c_weight__16;
reg	[`BIT_WIDTH-1:0]				c_weight__17;
reg	[`BIT_WIDTH-1:0]				c_weight__18;
reg	[`BIT_WIDTH-1:0]				c_weight__19;
reg	[`BIT_WIDTH-1:0]				c_weight__20;
reg	[`BIT_WIDTH-1:0]				c_weight__21;
reg	[`BIT_WIDTH-1:0]				c_weight__22;
reg	[`BIT_WIDTH-1:0]				c_weight__23;
reg	[`BIT_WIDTH-1:0]				c_weight__24;
reg	[`BIT_WIDTH-1:0]				c_weight__25;
reg	[`BIT_WIDTH-1:0]				c_weight__26;
reg	[`BIT_WIDTH-1:0]				c_weight__27;
reg	[`BIT_WIDTH-1:0]				c_weight__28;
reg	[`BIT_WIDTH-1:0]				c_weight__29;
reg	[`BIT_WIDTH-1:0]				c_weight__30;
reg	[`BIT_WIDTH-1:0]				c_weight__31;
reg	[`BIT_WIDTH-1:0]				c_weight__32;
reg	[`BIT_WIDTH-1:0]				c_weight__33;
reg	[`BIT_WIDTH-1:0]				c_weight__34;
reg	[`BIT_WIDTH-1:0]				c_weight__35;
reg	[`BIT_WIDTH-1:0]				c_weight__36;
reg	[`BIT_WIDTH-1:0]				c_weight__37;
reg	[`BIT_WIDTH-1:0]				c_weight__38;
reg	[`BIT_WIDTH-1:0]				c_weight__39;
reg	[`BIT_WIDTH-1:0]				c_weight__40;
reg	[`BIT_WIDTH-1:0]				c_weight__41;
reg	[`BIT_WIDTH-1:0]				c_weight__42;
reg	[`BIT_WIDTH-1:0]				c_weight__43;
reg	[`BIT_WIDTH-1:0]				c_weight__44;
reg	[`BIT_WIDTH-1:0]				c_weight__45;
reg	[`BIT_WIDTH-1:0]				c_weight__46;
reg	[`BIT_WIDTH-1:0]				c_weight__47;
reg	[`BIT_WIDTH-1:0]				c_weight__48;
reg	[`BIT_WIDTH-1:0]				c_weight__49;
reg	[`BIT_WIDTH-1:0]				c_weight__50;
reg	[`BIT_WIDTH-1:0]				c_weight__51;
reg	[`BIT_WIDTH-1:0]				c_weight__52;
reg	[`BIT_WIDTH-1:0]				c_weight__53;
reg	[`BIT_WIDTH-1:0]				c_weight__54;
reg	[`BIT_WIDTH-1:0]				c_weight__55;
reg	[`BIT_WIDTH-1:0]				c_weight__56;
reg	[`BIT_WIDTH-1:0]				c_weight__57;
reg	[`BIT_WIDTH-1:0]				c_weight__58;
reg	[`BIT_WIDTH-1:0]				c_weight__59;

//reg	[`LAYER_WIDTH-1:0]			c_layer	[TOTAL_LATENCY - 1:0];

reg	[`LAYER_WIDTH-1:0]				c_layer__0;
reg	[`LAYER_WIDTH-1:0]				c_layer__1;
reg	[`LAYER_WIDTH-1:0]				c_layer__2;
reg	[`LAYER_WIDTH-1:0]				c_layer__3;
reg	[`LAYER_WIDTH-1:0]				c_layer__4;
reg	[`LAYER_WIDTH-1:0]				c_layer__5;
reg	[`LAYER_WIDTH-1:0]				c_layer__6;
reg	[`LAYER_WIDTH-1:0]				c_layer__7;
reg	[`LAYER_WIDTH-1:0]				c_layer__8;
reg	[`LAYER_WIDTH-1:0]				c_layer__9;
reg	[`LAYER_WIDTH-1:0]				c_layer__10;
reg	[`LAYER_WIDTH-1:0]				c_layer__11;
reg	[`LAYER_WIDTH-1:0]				c_layer__12;
reg	[`LAYER_WIDTH-1:0]				c_layer__13;
reg	[`LAYER_WIDTH-1:0]				c_layer__14;
reg	[`LAYER_WIDTH-1:0]				c_layer__15;
reg	[`LAYER_WIDTH-1:0]				c_layer__16;
reg	[`LAYER_WIDTH-1:0]				c_layer__17;
reg	[`LAYER_WIDTH-1:0]				c_layer__18;
reg	[`LAYER_WIDTH-1:0]				c_layer__19;
reg	[`LAYER_WIDTH-1:0]				c_layer__20;
reg	[`LAYER_WIDTH-1:0]				c_layer__21;
reg	[`LAYER_WIDTH-1:0]				c_layer__22;
reg	[`LAYER_WIDTH-1:0]				c_layer__23;
reg	[`LAYER_WIDTH-1:0]				c_layer__24;
reg	[`LAYER_WIDTH-1:0]				c_layer__25;
reg	[`LAYER_WIDTH-1:0]				c_layer__26;
reg	[`LAYER_WIDTH-1:0]				c_layer__27;
reg	[`LAYER_WIDTH-1:0]				c_layer__28;
reg	[`LAYER_WIDTH-1:0]				c_layer__29;
reg	[`LAYER_WIDTH-1:0]				c_layer__30;
reg	[`LAYER_WIDTH-1:0]				c_layer__31;
reg	[`LAYER_WIDTH-1:0]				c_layer__32;
reg	[`LAYER_WIDTH-1:0]				c_layer__33;
reg	[`LAYER_WIDTH-1:0]				c_layer__34;
reg	[`LAYER_WIDTH-1:0]				c_layer__35;
reg	[`LAYER_WIDTH-1:0]				c_layer__36;
reg	[`LAYER_WIDTH-1:0]				c_layer__37;
reg	[`LAYER_WIDTH-1:0]				c_layer__38;
reg	[`LAYER_WIDTH-1:0]				c_layer__39;
reg	[`LAYER_WIDTH-1:0]				c_layer__40;
reg	[`LAYER_WIDTH-1:0]				c_layer__41;
reg	[`LAYER_WIDTH-1:0]				c_layer__42;
reg	[`LAYER_WIDTH-1:0]				c_layer__43;
reg	[`LAYER_WIDTH-1:0]				c_layer__44;
reg	[`LAYER_WIDTH-1:0]				c_layer__45;
reg	[`LAYER_WIDTH-1:0]				c_layer__46;
reg	[`LAYER_WIDTH-1:0]				c_layer__47;
reg	[`LAYER_WIDTH-1:0]				c_layer__48;
reg	[`LAYER_WIDTH-1:0]				c_layer__49;
reg	[`LAYER_WIDTH-1:0]				c_layer__50;
reg	[`LAYER_WIDTH-1:0]				c_layer__51;
reg	[`LAYER_WIDTH-1:0]				c_layer__52;
reg	[`LAYER_WIDTH-1:0]				c_layer__53;
reg	[`LAYER_WIDTH-1:0]				c_layer__54;
reg	[`LAYER_WIDTH-1:0]				c_layer__55;
reg	[`LAYER_WIDTH-1:0]				c_layer__56;
reg	[`LAYER_WIDTH-1:0]				c_layer__57;
reg	[`LAYER_WIDTH-1:0]				c_layer__58;
reg	[`LAYER_WIDTH-1:0]				c_layer__59;



//reg								c_dead	[TOTAL_LATENCY - 1:0];

reg					c_dead__0;
reg					c_dead__1;
reg					c_dead__2;
reg					c_dead__3;
reg					c_dead__4;
reg					c_dead__5;
reg					c_dead__6;
reg					c_dead__7;
reg					c_dead__8;
reg					c_dead__9;
reg					c_dead__10;
reg					c_dead__11;
reg					c_dead__12;
reg					c_dead__13;
reg					c_dead__14;
reg					c_dead__15;
reg					c_dead__16;
reg					c_dead__17;
reg					c_dead__18;
reg					c_dead__19;
reg					c_dead__20;
reg					c_dead__21;
reg					c_dead__22;
reg					c_dead__23;
reg					c_dead__24;
reg					c_dead__25;
reg					c_dead__26;
reg					c_dead__27;
reg					c_dead__28;
reg					c_dead__29;
reg					c_dead__30;
reg					c_dead__31;
reg					c_dead__32;
reg					c_dead__33;
reg					c_dead__34;
reg					c_dead__35;
reg					c_dead__36;
reg					c_dead__37;
reg					c_dead__38;
reg					c_dead__39;
reg					c_dead__40;
reg					c_dead__41;
reg					c_dead__42;
reg					c_dead__43;
reg					c_dead__44;
reg					c_dead__45;
reg					c_dead__46;
reg					c_dead__47;
reg					c_dead__48;
reg					c_dead__49;
reg					c_dead__50;
reg					c_dead__51;
reg					c_dead__52;
reg					c_dead__53;
reg					c_dead__54;
reg					c_dead__55;
reg					c_dead__56;
reg					c_dead__57;
reg					c_dead__58;
reg					c_dead__59;


//reg								c_hit	[TOTAL_LATENCY - 1:0];

reg					c_hit__0;
reg					c_hit__1;
reg					c_hit__2;
reg					c_hit__3;
reg					c_hit__4;
reg					c_hit__5;
reg					c_hit__6;
reg					c_hit__7;
reg					c_hit__8;
reg					c_hit__9;
reg					c_hit__10;
reg					c_hit__11;
reg					c_hit__12;
reg					c_hit__13;
reg					c_hit__14;
reg					c_hit__15;
reg					c_hit__16;
reg					c_hit__17;
reg					c_hit__18;
reg					c_hit__19;
reg					c_hit__20;
reg					c_hit__21;
reg					c_hit__22;
reg					c_hit__23;
reg					c_hit__24;
reg					c_hit__25;
reg					c_hit__26;
reg					c_hit__27;
reg					c_hit__28;
reg					c_hit__29;
reg					c_hit__30;
reg					c_hit__31;
reg					c_hit__32;
reg					c_hit__33;
reg					c_hit__34;
reg					c_hit__35;
reg					c_hit__36;
reg					c_hit__37;
reg					c_hit__38;
reg					c_hit__39;
reg					c_hit__40;
reg					c_hit__41;
reg					c_hit__42;
reg					c_hit__43;
reg					c_hit__44;
reg					c_hit__45;
reg					c_hit__46;
reg					c_hit__47;
reg					c_hit__48;
reg					c_hit__49;
reg					c_hit__50;
reg					c_hit__51;
reg					c_hit__52;
reg					c_hit__53;
reg					c_hit__54;
reg					c_hit__55;
reg					c_hit__56;
reg					c_hit__57;
reg					c_hit__58;
reg					c_hit__59;

//reg	[`BIT_WIDTH-1:0]				c_diff[TOTAL_LATENCY - 1:0];


reg	[`BIT_WIDTH-1:0]				c_diff__0;
reg	[`BIT_WIDTH-1:0]				c_diff__1;
reg	[`BIT_WIDTH-1:0]				c_diff__2;
reg	[`BIT_WIDTH-1:0]				c_diff__3;
reg	[`BIT_WIDTH-1:0]				c_diff__4;
reg	[`BIT_WIDTH-1:0]				c_diff__5;
reg	[`BIT_WIDTH-1:0]				c_diff__6;
reg	[`BIT_WIDTH-1:0]				c_diff__7;
reg	[`BIT_WIDTH-1:0]				c_diff__8;
reg	[`BIT_WIDTH-1:0]				c_diff__9;
reg	[`BIT_WIDTH-1:0]				c_diff__10;
reg	[`BIT_WIDTH-1:0]				c_diff__11;
reg	[`BIT_WIDTH-1:0]				c_diff__12;
reg	[`BIT_WIDTH-1:0]				c_diff__13;
reg	[`BIT_WIDTH-1:0]				c_diff__14;
reg	[`BIT_WIDTH-1:0]				c_diff__15;
reg	[`BIT_WIDTH-1:0]				c_diff__16;
reg	[`BIT_WIDTH-1:0]				c_diff__17;
reg	[`BIT_WIDTH-1:0]				c_diff__18;
reg	[`BIT_WIDTH-1:0]				c_diff__19;
reg	[`BIT_WIDTH-1:0]				c_diff__20;
reg	[`BIT_WIDTH-1:0]				c_diff__21;
reg	[`BIT_WIDTH-1:0]				c_diff__22;
reg	[`BIT_WIDTH-1:0]				c_diff__23;
reg	[`BIT_WIDTH-1:0]				c_diff__24;
reg	[`BIT_WIDTH-1:0]				c_diff__25;
reg	[`BIT_WIDTH-1:0]				c_diff__26;
reg	[`BIT_WIDTH-1:0]				c_diff__27;
reg	[`BIT_WIDTH-1:0]				c_diff__28;
reg	[`BIT_WIDTH-1:0]				c_diff__29;
reg	[`BIT_WIDTH-1:0]				c_diff__30;
reg	[`BIT_WIDTH-1:0]				c_diff__31;
reg	[`BIT_WIDTH-1:0]				c_diff__32;
reg	[`BIT_WIDTH-1:0]				c_diff__33;
reg	[`BIT_WIDTH-1:0]				c_diff__34;
reg	[`BIT_WIDTH-1:0]				c_diff__35;
reg	[`BIT_WIDTH-1:0]				c_diff__36;
reg	[`BIT_WIDTH-1:0]				c_diff__37;
reg	[`BIT_WIDTH-1:0]				c_diff__38;
reg	[`BIT_WIDTH-1:0]				c_diff__39;
reg	[`BIT_WIDTH-1:0]				c_diff__40;
reg	[`BIT_WIDTH-1:0]				c_diff__41;
reg	[`BIT_WIDTH-1:0]				c_diff__42;
reg	[`BIT_WIDTH-1:0]				c_diff__43;
reg	[`BIT_WIDTH-1:0]				c_diff__44;
reg	[`BIT_WIDTH-1:0]				c_diff__45;
reg	[`BIT_WIDTH-1:0]				c_diff__46;
reg	[`BIT_WIDTH-1:0]				c_diff__47;
reg	[`BIT_WIDTH-1:0]				c_diff__48;
reg	[`BIT_WIDTH-1:0]				c_diff__49;
reg	[`BIT_WIDTH-1:0]				c_diff__50;
reg	[`BIT_WIDTH-1:0]				c_diff__51;
reg	[`BIT_WIDTH-1:0]				c_diff__52;
reg	[`BIT_WIDTH-1:0]				c_diff__53;
reg	[`BIT_WIDTH-1:0]				c_diff__54;
reg	[`BIT_WIDTH-1:0]				c_diff__55;
reg	[`BIT_WIDTH-1:0]				c_diff__56;
reg	[`BIT_WIDTH-1:0]				c_diff__57;
reg	[`BIT_WIDTH-1:0]				c_diff__58;
reg	[`BIT_WIDTH-1:0]				c_diff__59;


//reg	[`BIT_WIDTH-1:0]				c_dl_b[TOTAL_LATENCY - 1:0];


reg	[`BIT_WIDTH-1:0]				c_dl_b__0;
reg	[`BIT_WIDTH-1:0]				c_dl_b__1;
reg	[`BIT_WIDTH-1:0]				c_dl_b__2;
reg	[`BIT_WIDTH-1:0]				c_dl_b__3;
reg	[`BIT_WIDTH-1:0]				c_dl_b__4;
reg	[`BIT_WIDTH-1:0]				c_dl_b__5;
reg	[`BIT_WIDTH-1:0]				c_dl_b__6;
reg	[`BIT_WIDTH-1:0]				c_dl_b__7;
reg	[`BIT_WIDTH-1:0]				c_dl_b__8;
reg	[`BIT_WIDTH-1:0]				c_dl_b__9;
reg	[`BIT_WIDTH-1:0]				c_dl_b__10;
reg	[`BIT_WIDTH-1:0]				c_dl_b__11;
reg	[`BIT_WIDTH-1:0]				c_dl_b__12;
reg	[`BIT_WIDTH-1:0]				c_dl_b__13;
reg	[`BIT_WIDTH-1:0]				c_dl_b__14;
reg	[`BIT_WIDTH-1:0]				c_dl_b__15;
reg	[`BIT_WIDTH-1:0]				c_dl_b__16;
reg	[`BIT_WIDTH-1:0]				c_dl_b__17;
reg	[`BIT_WIDTH-1:0]				c_dl_b__18;
reg	[`BIT_WIDTH-1:0]				c_dl_b__19;
reg	[`BIT_WIDTH-1:0]				c_dl_b__20;
reg	[`BIT_WIDTH-1:0]				c_dl_b__21;
reg	[`BIT_WIDTH-1:0]				c_dl_b__22;
reg	[`BIT_WIDTH-1:0]				c_dl_b__23;
reg	[`BIT_WIDTH-1:0]				c_dl_b__24;
reg	[`BIT_WIDTH-1:0]				c_dl_b__25;
reg	[`BIT_WIDTH-1:0]				c_dl_b__26;
reg	[`BIT_WIDTH-1:0]				c_dl_b__27;
reg	[`BIT_WIDTH-1:0]				c_dl_b__28;
reg	[`BIT_WIDTH-1:0]				c_dl_b__29;
reg	[`BIT_WIDTH-1:0]				c_dl_b__30;
reg	[`BIT_WIDTH-1:0]				c_dl_b__31;
reg	[`BIT_WIDTH-1:0]				c_dl_b__32;
reg	[`BIT_WIDTH-1:0]				c_dl_b__33;
reg	[`BIT_WIDTH-1:0]				c_dl_b__34;
reg	[`BIT_WIDTH-1:0]				c_dl_b__35;
reg	[`BIT_WIDTH-1:0]				c_dl_b__36;
reg	[`BIT_WIDTH-1:0]				c_dl_b__37;
reg	[`BIT_WIDTH-1:0]				c_dl_b__38;
reg	[`BIT_WIDTH-1:0]				c_dl_b__39;
reg	[`BIT_WIDTH-1:0]				c_dl_b__40;
reg	[`BIT_WIDTH-1:0]				c_dl_b__41;
reg	[`BIT_WIDTH-1:0]				c_dl_b__42;
reg	[`BIT_WIDTH-1:0]				c_dl_b__43;
reg	[`BIT_WIDTH-1:0]				c_dl_b__44;
reg	[`BIT_WIDTH-1:0]				c_dl_b__45;
reg	[`BIT_WIDTH-1:0]				c_dl_b__46;
reg	[`BIT_WIDTH-1:0]				c_dl_b__47;
reg	[`BIT_WIDTH-1:0]				c_dl_b__48;
reg	[`BIT_WIDTH-1:0]				c_dl_b__49;
reg	[`BIT_WIDTH-1:0]				c_dl_b__50;
reg	[`BIT_WIDTH-1:0]				c_dl_b__51;
reg	[`BIT_WIDTH-1:0]				c_dl_b__52;
reg	[`BIT_WIDTH-1:0]				c_dl_b__53;
reg	[`BIT_WIDTH-1:0]				c_dl_b__54;
reg	[`BIT_WIDTH-1:0]				c_dl_b__55;
reg	[`BIT_WIDTH-1:0]				c_dl_b__56;
reg	[`BIT_WIDTH-1:0]				c_dl_b__57;
reg	[`BIT_WIDTH-1:0]				c_dl_b__58;
reg	[`BIT_WIDTH-1:0]				c_dl_b__59;


//reg	[2*`BIT_WIDTH-1:0]			c_numer[TOTAL_LATENCY - 1:0];


reg	[2*`BIT_WIDTH-1:0]				c_numer__0;
reg	[2*`BIT_WIDTH-1:0]				c_numer__1;
reg	[2*`BIT_WIDTH-1:0]				c_numer__2;
reg	[2*`BIT_WIDTH-1:0]				c_numer__3;
reg	[2*`BIT_WIDTH-1:0]				c_numer__4;
reg	[2*`BIT_WIDTH-1:0]				c_numer__5;
reg	[2*`BIT_WIDTH-1:0]				c_numer__6;
reg	[2*`BIT_WIDTH-1:0]				c_numer__7;
reg	[2*`BIT_WIDTH-1:0]				c_numer__8;
reg	[2*`BIT_WIDTH-1:0]				c_numer__9;
reg	[2*`BIT_WIDTH-1:0]				c_numer__10;
reg	[2*`BIT_WIDTH-1:0]				c_numer__11;
reg	[2*`BIT_WIDTH-1:0]				c_numer__12;
reg	[2*`BIT_WIDTH-1:0]				c_numer__13;
reg	[2*`BIT_WIDTH-1:0]				c_numer__14;
reg	[2*`BIT_WIDTH-1:0]				c_numer__15;
reg	[2*`BIT_WIDTH-1:0]				c_numer__16;
reg	[2*`BIT_WIDTH-1:0]				c_numer__17;
reg	[2*`BIT_WIDTH-1:0]				c_numer__18;
reg	[2*`BIT_WIDTH-1:0]				c_numer__19;
reg	[2*`BIT_WIDTH-1:0]				c_numer__20;
reg	[2*`BIT_WIDTH-1:0]				c_numer__21;
reg	[2*`BIT_WIDTH-1:0]				c_numer__22;
reg	[2*`BIT_WIDTH-1:0]				c_numer__23;
reg	[2*`BIT_WIDTH-1:0]				c_numer__24;
reg	[2*`BIT_WIDTH-1:0]				c_numer__25;
reg	[2*`BIT_WIDTH-1:0]				c_numer__26;
reg	[2*`BIT_WIDTH-1:0]				c_numer__27;
reg	[2*`BIT_WIDTH-1:0]				c_numer__28;
reg	[2*`BIT_WIDTH-1:0]				c_numer__29;
reg	[2*`BIT_WIDTH-1:0]				c_numer__30;
reg	[2*`BIT_WIDTH-1:0]				c_numer__31;
reg	[2*`BIT_WIDTH-1:0]				c_numer__32;
reg	[2*`BIT_WIDTH-1:0]				c_numer__33;
reg	[2*`BIT_WIDTH-1:0]				c_numer__34;
reg	[2*`BIT_WIDTH-1:0]				c_numer__35;
reg	[2*`BIT_WIDTH-1:0]				c_numer__36;
reg	[2*`BIT_WIDTH-1:0]				c_numer__37;
reg	[2*`BIT_WIDTH-1:0]				c_numer__38;
reg	[2*`BIT_WIDTH-1:0]				c_numer__39;
reg	[2*`BIT_WIDTH-1:0]				c_numer__40;
reg	[2*`BIT_WIDTH-1:0]				c_numer__41;
reg	[2*`BIT_WIDTH-1:0]				c_numer__42;
reg	[2*`BIT_WIDTH-1:0]				c_numer__43;
reg	[2*`BIT_WIDTH-1:0]				c_numer__44;
reg	[2*`BIT_WIDTH-1:0]				c_numer__45;
reg	[2*`BIT_WIDTH-1:0]				c_numer__46;
reg	[2*`BIT_WIDTH-1:0]				c_numer__47;
reg	[2*`BIT_WIDTH-1:0]				c_numer__48;
reg	[2*`BIT_WIDTH-1:0]				c_numer__49;
reg	[2*`BIT_WIDTH-1:0]				c_numer__50;
reg	[2*`BIT_WIDTH-1:0]				c_numer__51;
reg	[2*`BIT_WIDTH-1:0]				c_numer__52;
reg	[2*`BIT_WIDTH-1:0]				c_numer__53;
reg	[2*`BIT_WIDTH-1:0]				c_numer__54;
reg	[2*`BIT_WIDTH-1:0]				c_numer__55;
reg	[2*`BIT_WIDTH-1:0]				c_numer__56;
reg	[2*`BIT_WIDTH-1:0]				c_numer__57;
reg	[2*`BIT_WIDTH-1:0]				c_numer__58;
reg	[2*`BIT_WIDTH-1:0]				c_numer__59;

//reg	[`BIT_WIDTH-1:0]				c_z1[TOTAL_LATENCY - 1:0];


reg	[`BIT_WIDTH-1:0]				c_z1__0;
reg	[`BIT_WIDTH-1:0]				c_z1__1;
reg	[`BIT_WIDTH-1:0]				c_z1__2;
reg	[`BIT_WIDTH-1:0]				c_z1__3;
reg	[`BIT_WIDTH-1:0]				c_z1__4;
reg	[`BIT_WIDTH-1:0]				c_z1__5;
reg	[`BIT_WIDTH-1:0]				c_z1__6;
reg	[`BIT_WIDTH-1:0]				c_z1__7;
reg	[`BIT_WIDTH-1:0]				c_z1__8;
reg	[`BIT_WIDTH-1:0]				c_z1__9;
reg	[`BIT_WIDTH-1:0]				c_z1__10;
reg	[`BIT_WIDTH-1:0]				c_z1__11;
reg	[`BIT_WIDTH-1:0]				c_z1__12;
reg	[`BIT_WIDTH-1:0]				c_z1__13;
reg	[`BIT_WIDTH-1:0]				c_z1__14;
reg	[`BIT_WIDTH-1:0]				c_z1__15;
reg	[`BIT_WIDTH-1:0]				c_z1__16;
reg	[`BIT_WIDTH-1:0]				c_z1__17;
reg	[`BIT_WIDTH-1:0]				c_z1__18;
reg	[`BIT_WIDTH-1:0]				c_z1__19;
reg	[`BIT_WIDTH-1:0]				c_z1__20;
reg	[`BIT_WIDTH-1:0]				c_z1__21;
reg	[`BIT_WIDTH-1:0]				c_z1__22;
reg	[`BIT_WIDTH-1:0]				c_z1__23;
reg	[`BIT_WIDTH-1:0]				c_z1__24;
reg	[`BIT_WIDTH-1:0]				c_z1__25;
reg	[`BIT_WIDTH-1:0]				c_z1__26;
reg	[`BIT_WIDTH-1:0]				c_z1__27;
reg	[`BIT_WIDTH-1:0]				c_z1__28;
reg	[`BIT_WIDTH-1:0]				c_z1__29;
reg	[`BIT_WIDTH-1:0]				c_z1__30;
reg	[`BIT_WIDTH-1:0]				c_z1__31;
reg	[`BIT_WIDTH-1:0]				c_z1__32;
reg	[`BIT_WIDTH-1:0]				c_z1__33;
reg	[`BIT_WIDTH-1:0]				c_z1__34;
reg	[`BIT_WIDTH-1:0]				c_z1__35;
reg	[`BIT_WIDTH-1:0]				c_z1__36;
reg	[`BIT_WIDTH-1:0]				c_z1__37;
reg	[`BIT_WIDTH-1:0]				c_z1__38;
reg	[`BIT_WIDTH-1:0]				c_z1__39;
reg	[`BIT_WIDTH-1:0]				c_z1__40;
reg	[`BIT_WIDTH-1:0]				c_z1__41;
reg	[`BIT_WIDTH-1:0]				c_z1__42;
reg	[`BIT_WIDTH-1:0]				c_z1__43;
reg	[`BIT_WIDTH-1:0]				c_z1__44;
reg	[`BIT_WIDTH-1:0]				c_z1__45;
reg	[`BIT_WIDTH-1:0]				c_z1__46;
reg	[`BIT_WIDTH-1:0]				c_z1__47;
reg	[`BIT_WIDTH-1:0]				c_z1__48;
reg	[`BIT_WIDTH-1:0]				c_z1__49;
reg	[`BIT_WIDTH-1:0]				c_z1__50;
reg	[`BIT_WIDTH-1:0]				c_z1__51;
reg	[`BIT_WIDTH-1:0]				c_z1__52;
reg	[`BIT_WIDTH-1:0]				c_z1__53;
reg	[`BIT_WIDTH-1:0]				c_z1__54;
reg	[`BIT_WIDTH-1:0]				c_z1__55;
reg	[`BIT_WIDTH-1:0]				c_z1__56;
reg	[`BIT_WIDTH-1:0]				c_z1__57;
reg	[`BIT_WIDTH-1:0]				c_z1__58;
reg	[`BIT_WIDTH-1:0]				c_z1__59;

//reg	[`BIT_WIDTH-1:0]				c_z0[TOTAL_LATENCY - 1:0];


reg	[`BIT_WIDTH-1:0]				c_z0__0;
reg	[`BIT_WIDTH-1:0]				c_z0__1;
reg	[`BIT_WIDTH-1:0]				c_z0__2;
reg	[`BIT_WIDTH-1:0]				c_z0__3;
reg	[`BIT_WIDTH-1:0]				c_z0__4;
reg	[`BIT_WIDTH-1:0]				c_z0__5;
reg	[`BIT_WIDTH-1:0]				c_z0__6;
reg	[`BIT_WIDTH-1:0]				c_z0__7;
reg	[`BIT_WIDTH-1:0]				c_z0__8;
reg	[`BIT_WIDTH-1:0]				c_z0__9;
reg	[`BIT_WIDTH-1:0]				c_z0__10;
reg	[`BIT_WIDTH-1:0]				c_z0__11;
reg	[`BIT_WIDTH-1:0]				c_z0__12;
reg	[`BIT_WIDTH-1:0]				c_z0__13;
reg	[`BIT_WIDTH-1:0]				c_z0__14;
reg	[`BIT_WIDTH-1:0]				c_z0__15;
reg	[`BIT_WIDTH-1:0]				c_z0__16;
reg	[`BIT_WIDTH-1:0]				c_z0__17;
reg	[`BIT_WIDTH-1:0]				c_z0__18;
reg	[`BIT_WIDTH-1:0]				c_z0__19;
reg	[`BIT_WIDTH-1:0]				c_z0__20;
reg	[`BIT_WIDTH-1:0]				c_z0__21;
reg	[`BIT_WIDTH-1:0]				c_z0__22;
reg	[`BIT_WIDTH-1:0]				c_z0__23;
reg	[`BIT_WIDTH-1:0]				c_z0__24;
reg	[`BIT_WIDTH-1:0]				c_z0__25;
reg	[`BIT_WIDTH-1:0]				c_z0__26;
reg	[`BIT_WIDTH-1:0]				c_z0__27;
reg	[`BIT_WIDTH-1:0]				c_z0__28;
reg	[`BIT_WIDTH-1:0]				c_z0__29;
reg	[`BIT_WIDTH-1:0]				c_z0__30;
reg	[`BIT_WIDTH-1:0]				c_z0__31;
reg	[`BIT_WIDTH-1:0]				c_z0__32;
reg	[`BIT_WIDTH-1:0]				c_z0__33;
reg	[`BIT_WIDTH-1:0]				c_z0__34;
reg	[`BIT_WIDTH-1:0]				c_z0__35;
reg	[`BIT_WIDTH-1:0]				c_z0__36;
reg	[`BIT_WIDTH-1:0]				c_z0__37;
reg	[`BIT_WIDTH-1:0]				c_z0__38;
reg	[`BIT_WIDTH-1:0]				c_z0__39;
reg	[`BIT_WIDTH-1:0]				c_z0__40;
reg	[`BIT_WIDTH-1:0]				c_z0__41;
reg	[`BIT_WIDTH-1:0]				c_z0__42;
reg	[`BIT_WIDTH-1:0]				c_z0__43;
reg	[`BIT_WIDTH-1:0]				c_z0__44;
reg	[`BIT_WIDTH-1:0]				c_z0__45;
reg	[`BIT_WIDTH-1:0]				c_z0__46;
reg	[`BIT_WIDTH-1:0]				c_z0__47;
reg	[`BIT_WIDTH-1:0]				c_z0__48;
reg	[`BIT_WIDTH-1:0]				c_z0__49;
reg	[`BIT_WIDTH-1:0]				c_z0__50;
reg	[`BIT_WIDTH-1:0]				c_z0__51;
reg	[`BIT_WIDTH-1:0]				c_z0__52;
reg	[`BIT_WIDTH-1:0]				c_z0__53;
reg	[`BIT_WIDTH-1:0]				c_z0__54;
reg	[`BIT_WIDTH-1:0]				c_z0__55;
reg	[`BIT_WIDTH-1:0]				c_z0__56;
reg	[`BIT_WIDTH-1:0]				c_z0__57;
reg	[`BIT_WIDTH-1:0]				c_z0__58;
reg	[`BIT_WIDTH-1:0]				c_z0__59;



//reg	[`BIT_WIDTH-1:0]				c_mut[TOTAL_LATENCY - 1:0];

reg	[`BIT_WIDTH-1:0]				c_mut__0;
reg	[`BIT_WIDTH-1:0]				c_mut__1;
reg	[`BIT_WIDTH-1:0]				c_mut__2;
reg	[`BIT_WIDTH-1:0]				c_mut__3;
reg	[`BIT_WIDTH-1:0]				c_mut__4;
reg	[`BIT_WIDTH-1:0]				c_mut__5;
reg	[`BIT_WIDTH-1:0]				c_mut__6;
reg	[`BIT_WIDTH-1:0]				c_mut__7;
reg	[`BIT_WIDTH-1:0]				c_mut__8;
reg	[`BIT_WIDTH-1:0]				c_mut__9;
reg	[`BIT_WIDTH-1:0]				c_mut__10;
reg	[`BIT_WIDTH-1:0]				c_mut__11;
reg	[`BIT_WIDTH-1:0]				c_mut__12;
reg	[`BIT_WIDTH-1:0]				c_mut__13;
reg	[`BIT_WIDTH-1:0]				c_mut__14;
reg	[`BIT_WIDTH-1:0]				c_mut__15;
reg	[`BIT_WIDTH-1:0]				c_mut__16;
reg	[`BIT_WIDTH-1:0]				c_mut__17;
reg	[`BIT_WIDTH-1:0]				c_mut__18;
reg	[`BIT_WIDTH-1:0]				c_mut__19;
reg	[`BIT_WIDTH-1:0]				c_mut__20;
reg	[`BIT_WIDTH-1:0]				c_mut__21;
reg	[`BIT_WIDTH-1:0]				c_mut__22;
reg	[`BIT_WIDTH-1:0]				c_mut__23;
reg	[`BIT_WIDTH-1:0]				c_mut__24;
reg	[`BIT_WIDTH-1:0]				c_mut__25;
reg	[`BIT_WIDTH-1:0]				c_mut__26;
reg	[`BIT_WIDTH-1:0]				c_mut__27;
reg	[`BIT_WIDTH-1:0]				c_mut__28;
reg	[`BIT_WIDTH-1:0]				c_mut__29;
reg	[`BIT_WIDTH-1:0]				c_mut__30;
reg	[`BIT_WIDTH-1:0]				c_mut__31;
reg	[`BIT_WIDTH-1:0]				c_mut__32;
reg	[`BIT_WIDTH-1:0]				c_mut__33;
reg	[`BIT_WIDTH-1:0]				c_mut__34;
reg	[`BIT_WIDTH-1:0]				c_mut__35;
reg	[`BIT_WIDTH-1:0]				c_mut__36;
reg	[`BIT_WIDTH-1:0]				c_mut__37;
reg	[`BIT_WIDTH-1:0]				c_mut__38;
reg	[`BIT_WIDTH-1:0]				c_mut__39;
reg	[`BIT_WIDTH-1:0]				c_mut__40;
reg	[`BIT_WIDTH-1:0]				c_mut__41;
reg	[`BIT_WIDTH-1:0]				c_mut__42;
reg	[`BIT_WIDTH-1:0]				c_mut__43;
reg	[`BIT_WIDTH-1:0]				c_mut__44;
reg	[`BIT_WIDTH-1:0]				c_mut__45;
reg	[`BIT_WIDTH-1:0]				c_mut__46;
reg	[`BIT_WIDTH-1:0]				c_mut__47;
reg	[`BIT_WIDTH-1:0]				c_mut__48;
reg	[`BIT_WIDTH-1:0]				c_mut__49;
reg	[`BIT_WIDTH-1:0]				c_mut__50;
reg	[`BIT_WIDTH-1:0]				c_mut__51;
reg	[`BIT_WIDTH-1:0]				c_mut__52;
reg	[`BIT_WIDTH-1:0]				c_mut__53;
reg	[`BIT_WIDTH-1:0]				c_mut__54;
reg	[`BIT_WIDTH-1:0]				c_mut__55;
reg	[`BIT_WIDTH-1:0]				c_mut__56;
reg	[`BIT_WIDTH-1:0]				c_mut__57;
reg	[`BIT_WIDTH-1:0]				c_mut__58;
reg	[`BIT_WIDTH-1:0]				c_mut__59;


//reg	[`BIT_WIDTH-1:0]				r_x	[TOTAL_LATENCY - 1:0];

reg	[`BIT_WIDTH-1:0]				r_x__0;
reg	[`BIT_WIDTH-1:0]				r_x__1;
reg	[`BIT_WIDTH-1:0]				r_x__2;
reg	[`BIT_WIDTH-1:0]				r_x__3;
reg	[`BIT_WIDTH-1:0]				r_x__4;
reg	[`BIT_WIDTH-1:0]				r_x__5;
reg	[`BIT_WIDTH-1:0]				r_x__6;
reg	[`BIT_WIDTH-1:0]				r_x__7;
reg	[`BIT_WIDTH-1:0]				r_x__8;
reg	[`BIT_WIDTH-1:0]				r_x__9;
reg	[`BIT_WIDTH-1:0]				r_x__10;
reg	[`BIT_WIDTH-1:0]				r_x__11;
reg	[`BIT_WIDTH-1:0]				r_x__12;
reg	[`BIT_WIDTH-1:0]				r_x__13;
reg	[`BIT_WIDTH-1:0]				r_x__14;
reg	[`BIT_WIDTH-1:0]				r_x__15;
reg	[`BIT_WIDTH-1:0]				r_x__16;
reg	[`BIT_WIDTH-1:0]				r_x__17;
reg	[`BIT_WIDTH-1:0]				r_x__18;
reg	[`BIT_WIDTH-1:0]				r_x__19;
reg	[`BIT_WIDTH-1:0]				r_x__20;
reg	[`BIT_WIDTH-1:0]				r_x__21;
reg	[`BIT_WIDTH-1:0]				r_x__22;
reg	[`BIT_WIDTH-1:0]				r_x__23;
reg	[`BIT_WIDTH-1:0]				r_x__24;
reg	[`BIT_WIDTH-1:0]				r_x__25;
reg	[`BIT_WIDTH-1:0]				r_x__26;
reg	[`BIT_WIDTH-1:0]				r_x__27;
reg	[`BIT_WIDTH-1:0]				r_x__28;
reg	[`BIT_WIDTH-1:0]				r_x__29;
reg	[`BIT_WIDTH-1:0]				r_x__30;
reg	[`BIT_WIDTH-1:0]				r_x__31;
reg	[`BIT_WIDTH-1:0]				r_x__32;
reg	[`BIT_WIDTH-1:0]				r_x__33;
reg	[`BIT_WIDTH-1:0]				r_x__34;
reg	[`BIT_WIDTH-1:0]				r_x__35;
reg	[`BIT_WIDTH-1:0]				r_x__36;
reg	[`BIT_WIDTH-1:0]				r_x__37;
reg	[`BIT_WIDTH-1:0]				r_x__38;
reg	[`BIT_WIDTH-1:0]				r_x__39;
reg	[`BIT_WIDTH-1:0]				r_x__40;
reg	[`BIT_WIDTH-1:0]				r_x__41;
reg	[`BIT_WIDTH-1:0]				r_x__42;
reg	[`BIT_WIDTH-1:0]				r_x__43;
reg	[`BIT_WIDTH-1:0]				r_x__44;
reg	[`BIT_WIDTH-1:0]				r_x__45;
reg	[`BIT_WIDTH-1:0]				r_x__46;
reg	[`BIT_WIDTH-1:0]				r_x__47;
reg	[`BIT_WIDTH-1:0]				r_x__48;
reg	[`BIT_WIDTH-1:0]				r_x__49;
reg	[`BIT_WIDTH-1:0]				r_x__50;
reg	[`BIT_WIDTH-1:0]				r_x__51;
reg	[`BIT_WIDTH-1:0]				r_x__52;
reg	[`BIT_WIDTH-1:0]				r_x__53;
reg	[`BIT_WIDTH-1:0]				r_x__54;
reg	[`BIT_WIDTH-1:0]				r_x__55;
reg	[`BIT_WIDTH-1:0]				r_x__56;
reg	[`BIT_WIDTH-1:0]				r_x__57;
reg	[`BIT_WIDTH-1:0]				r_x__58;
reg	[`BIT_WIDTH-1:0]				r_x__59;

//reg	[`BIT_WIDTH-1:0]				r_y	[TOTAL_LATENCY - 1:0];

reg	[`BIT_WIDTH-1:0]				r_y__0;
reg	[`BIT_WIDTH-1:0]				r_y__1;
reg	[`BIT_WIDTH-1:0]				r_y__2;
reg	[`BIT_WIDTH-1:0]				r_y__3;
reg	[`BIT_WIDTH-1:0]				r_y__4;
reg	[`BIT_WIDTH-1:0]				r_y__5;
reg	[`BIT_WIDTH-1:0]				r_y__6;
reg	[`BIT_WIDTH-1:0]				r_y__7;
reg	[`BIT_WIDTH-1:0]				r_y__8;
reg	[`BIT_WIDTH-1:0]				r_y__9;
reg	[`BIT_WIDTH-1:0]				r_y__10;
reg	[`BIT_WIDTH-1:0]				r_y__11;
reg	[`BIT_WIDTH-1:0]				r_y__12;
reg	[`BIT_WIDTH-1:0]				r_y__13;
reg	[`BIT_WIDTH-1:0]				r_y__14;
reg	[`BIT_WIDTH-1:0]				r_y__15;
reg	[`BIT_WIDTH-1:0]				r_y__16;
reg	[`BIT_WIDTH-1:0]				r_y__17;
reg	[`BIT_WIDTH-1:0]				r_y__18;
reg	[`BIT_WIDTH-1:0]				r_y__19;
reg	[`BIT_WIDTH-1:0]				r_y__20;
reg	[`BIT_WIDTH-1:0]				r_y__21;
reg	[`BIT_WIDTH-1:0]				r_y__22;
reg	[`BIT_WIDTH-1:0]				r_y__23;
reg	[`BIT_WIDTH-1:0]				r_y__24;
reg	[`BIT_WIDTH-1:0]				r_y__25;
reg	[`BIT_WIDTH-1:0]				r_y__26;
reg	[`BIT_WIDTH-1:0]				r_y__27;
reg	[`BIT_WIDTH-1:0]				r_y__28;
reg	[`BIT_WIDTH-1:0]				r_y__29;
reg	[`BIT_WIDTH-1:0]				r_y__30;
reg	[`BIT_WIDTH-1:0]				r_y__31;
reg	[`BIT_WIDTH-1:0]				r_y__32;
reg	[`BIT_WIDTH-1:0]				r_y__33;
reg	[`BIT_WIDTH-1:0]				r_y__34;
reg	[`BIT_WIDTH-1:0]				r_y__35;
reg	[`BIT_WIDTH-1:0]				r_y__36;
reg	[`BIT_WIDTH-1:0]				r_y__37;
reg	[`BIT_WIDTH-1:0]				r_y__38;
reg	[`BIT_WIDTH-1:0]				r_y__39;
reg	[`BIT_WIDTH-1:0]				r_y__40;
reg	[`BIT_WIDTH-1:0]				r_y__41;
reg	[`BIT_WIDTH-1:0]				r_y__42;
reg	[`BIT_WIDTH-1:0]				r_y__43;
reg	[`BIT_WIDTH-1:0]				r_y__44;
reg	[`BIT_WIDTH-1:0]				r_y__45;
reg	[`BIT_WIDTH-1:0]				r_y__46;
reg	[`BIT_WIDTH-1:0]				r_y__47;
reg	[`BIT_WIDTH-1:0]				r_y__48;
reg	[`BIT_WIDTH-1:0]				r_y__49;
reg	[`BIT_WIDTH-1:0]				r_y__50;
reg	[`BIT_WIDTH-1:0]				r_y__51;
reg	[`BIT_WIDTH-1:0]				r_y__52;
reg	[`BIT_WIDTH-1:0]				r_y__53;
reg	[`BIT_WIDTH-1:0]				r_y__54;
reg	[`BIT_WIDTH-1:0]				r_y__55;
reg	[`BIT_WIDTH-1:0]				r_y__56;
reg	[`BIT_WIDTH-1:0]				r_y__57;
reg	[`BIT_WIDTH-1:0]				r_y__58;
reg	[`BIT_WIDTH-1:0]				r_y__59;



//reg	[`BIT_WIDTH-1:0]				r_z	[TOTAL_LATENCY - 1:0];

reg	[`BIT_WIDTH-1:0]				r_z__0;
reg	[`BIT_WIDTH-1:0]				r_z__1;
reg	[`BIT_WIDTH-1:0]				r_z__2;
reg	[`BIT_WIDTH-1:0]				r_z__3;
reg	[`BIT_WIDTH-1:0]				r_z__4;
reg	[`BIT_WIDTH-1:0]				r_z__5;
reg	[`BIT_WIDTH-1:0]				r_z__6;
reg	[`BIT_WIDTH-1:0]				r_z__7;
reg	[`BIT_WIDTH-1:0]				r_z__8;
reg	[`BIT_WIDTH-1:0]				r_z__9;
reg	[`BIT_WIDTH-1:0]				r_z__10;
reg	[`BIT_WIDTH-1:0]				r_z__11;
reg	[`BIT_WIDTH-1:0]				r_z__12;
reg	[`BIT_WIDTH-1:0]				r_z__13;
reg	[`BIT_WIDTH-1:0]				r_z__14;
reg	[`BIT_WIDTH-1:0]				r_z__15;
reg	[`BIT_WIDTH-1:0]				r_z__16;
reg	[`BIT_WIDTH-1:0]				r_z__17;
reg	[`BIT_WIDTH-1:0]				r_z__18;
reg	[`BIT_WIDTH-1:0]				r_z__19;
reg	[`BIT_WIDTH-1:0]				r_z__20;
reg	[`BIT_WIDTH-1:0]				r_z__21;
reg	[`BIT_WIDTH-1:0]				r_z__22;
reg	[`BIT_WIDTH-1:0]				r_z__23;
reg	[`BIT_WIDTH-1:0]				r_z__24;
reg	[`BIT_WIDTH-1:0]				r_z__25;
reg	[`BIT_WIDTH-1:0]				r_z__26;
reg	[`BIT_WIDTH-1:0]				r_z__27;
reg	[`BIT_WIDTH-1:0]				r_z__28;
reg	[`BIT_WIDTH-1:0]				r_z__29;
reg	[`BIT_WIDTH-1:0]				r_z__30;
reg	[`BIT_WIDTH-1:0]				r_z__31;
reg	[`BIT_WIDTH-1:0]				r_z__32;
reg	[`BIT_WIDTH-1:0]				r_z__33;
reg	[`BIT_WIDTH-1:0]				r_z__34;
reg	[`BIT_WIDTH-1:0]				r_z__35;
reg	[`BIT_WIDTH-1:0]				r_z__36;
reg	[`BIT_WIDTH-1:0]				r_z__37;
reg	[`BIT_WIDTH-1:0]				r_z__38;
reg	[`BIT_WIDTH-1:0]				r_z__39;
reg	[`BIT_WIDTH-1:0]				r_z__40;
reg	[`BIT_WIDTH-1:0]				r_z__41;
reg	[`BIT_WIDTH-1:0]				r_z__42;
reg	[`BIT_WIDTH-1:0]				r_z__43;
reg	[`BIT_WIDTH-1:0]				r_z__44;
reg	[`BIT_WIDTH-1:0]				r_z__45;
reg	[`BIT_WIDTH-1:0]				r_z__46;
reg	[`BIT_WIDTH-1:0]				r_z__47;
reg	[`BIT_WIDTH-1:0]				r_z__48;
reg	[`BIT_WIDTH-1:0]				r_z__49;
reg	[`BIT_WIDTH-1:0]				r_z__50;
reg	[`BIT_WIDTH-1:0]				r_z__51;
reg	[`BIT_WIDTH-1:0]				r_z__52;
reg	[`BIT_WIDTH-1:0]				r_z__53;
reg	[`BIT_WIDTH-1:0]				r_z__54;
reg	[`BIT_WIDTH-1:0]				r_z__55;
reg	[`BIT_WIDTH-1:0]				r_z__56;
reg	[`BIT_WIDTH-1:0]				r_z__57;
reg	[`BIT_WIDTH-1:0]				r_z__58;
reg	[`BIT_WIDTH-1:0]				r_z__59;

//reg	[`BIT_WIDTH-1:0]				r_ux	[TOTAL_LATENCY - 1:0];

reg	[`BIT_WIDTH-1:0]				r_ux__0;
reg	[`BIT_WIDTH-1:0]				r_ux__1;
reg	[`BIT_WIDTH-1:0]				r_ux__2;
reg	[`BIT_WIDTH-1:0]				r_ux__3;
reg	[`BIT_WIDTH-1:0]				r_ux__4;
reg	[`BIT_WIDTH-1:0]				r_ux__5;
reg	[`BIT_WIDTH-1:0]				r_ux__6;
reg	[`BIT_WIDTH-1:0]				r_ux__7;
reg	[`BIT_WIDTH-1:0]				r_ux__8;
reg	[`BIT_WIDTH-1:0]				r_ux__9;
reg	[`BIT_WIDTH-1:0]				r_ux__10;
reg	[`BIT_WIDTH-1:0]				r_ux__11;
reg	[`BIT_WIDTH-1:0]				r_ux__12;
reg	[`BIT_WIDTH-1:0]				r_ux__13;
reg	[`BIT_WIDTH-1:0]				r_ux__14;
reg	[`BIT_WIDTH-1:0]				r_ux__15;
reg	[`BIT_WIDTH-1:0]				r_ux__16;
reg	[`BIT_WIDTH-1:0]				r_ux__17;
reg	[`BIT_WIDTH-1:0]				r_ux__18;
reg	[`BIT_WIDTH-1:0]				r_ux__19;
reg	[`BIT_WIDTH-1:0]				r_ux__20;
reg	[`BIT_WIDTH-1:0]				r_ux__21;
reg	[`BIT_WIDTH-1:0]				r_ux__22;
reg	[`BIT_WIDTH-1:0]				r_ux__23;
reg	[`BIT_WIDTH-1:0]				r_ux__24;
reg	[`BIT_WIDTH-1:0]				r_ux__25;
reg	[`BIT_WIDTH-1:0]				r_ux__26;
reg	[`BIT_WIDTH-1:0]				r_ux__27;
reg	[`BIT_WIDTH-1:0]				r_ux__28;
reg	[`BIT_WIDTH-1:0]				r_ux__29;
reg	[`BIT_WIDTH-1:0]				r_ux__30;
reg	[`BIT_WIDTH-1:0]				r_ux__31;
reg	[`BIT_WIDTH-1:0]				r_ux__32;
reg	[`BIT_WIDTH-1:0]				r_ux__33;
reg	[`BIT_WIDTH-1:0]				r_ux__34;
reg	[`BIT_WIDTH-1:0]				r_ux__35;
reg	[`BIT_WIDTH-1:0]				r_ux__36;
reg	[`BIT_WIDTH-1:0]				r_ux__37;
reg	[`BIT_WIDTH-1:0]				r_ux__38;
reg	[`BIT_WIDTH-1:0]				r_ux__39;
reg	[`BIT_WIDTH-1:0]				r_ux__40;
reg	[`BIT_WIDTH-1:0]				r_ux__41;
reg	[`BIT_WIDTH-1:0]				r_ux__42;
reg	[`BIT_WIDTH-1:0]				r_ux__43;
reg	[`BIT_WIDTH-1:0]				r_ux__44;
reg	[`BIT_WIDTH-1:0]				r_ux__45;
reg	[`BIT_WIDTH-1:0]				r_ux__46;
reg	[`BIT_WIDTH-1:0]				r_ux__47;
reg	[`BIT_WIDTH-1:0]				r_ux__48;
reg	[`BIT_WIDTH-1:0]				r_ux__49;
reg	[`BIT_WIDTH-1:0]				r_ux__50;
reg	[`BIT_WIDTH-1:0]				r_ux__51;
reg	[`BIT_WIDTH-1:0]				r_ux__52;
reg	[`BIT_WIDTH-1:0]				r_ux__53;
reg	[`BIT_WIDTH-1:0]				r_ux__54;
reg	[`BIT_WIDTH-1:0]				r_ux__55;
reg	[`BIT_WIDTH-1:0]				r_ux__56;
reg	[`BIT_WIDTH-1:0]				r_ux__57;
reg	[`BIT_WIDTH-1:0]				r_ux__58;
reg	[`BIT_WIDTH-1:0]				r_ux__59;

//reg	[`BIT_WIDTH-1:0]				r_uy	[TOTAL_LATENCY - 1:0];

reg	[`BIT_WIDTH-1:0]				r_uy__0;
reg	[`BIT_WIDTH-1:0]				r_uy__1;
reg	[`BIT_WIDTH-1:0]				r_uy__2;
reg	[`BIT_WIDTH-1:0]				r_uy__3;
reg	[`BIT_WIDTH-1:0]				r_uy__4;
reg	[`BIT_WIDTH-1:0]				r_uy__5;
reg	[`BIT_WIDTH-1:0]				r_uy__6;
reg	[`BIT_WIDTH-1:0]				r_uy__7;
reg	[`BIT_WIDTH-1:0]				r_uy__8;
reg	[`BIT_WIDTH-1:0]				r_uy__9;
reg	[`BIT_WIDTH-1:0]				r_uy__10;
reg	[`BIT_WIDTH-1:0]				r_uy__11;
reg	[`BIT_WIDTH-1:0]				r_uy__12;
reg	[`BIT_WIDTH-1:0]				r_uy__13;
reg	[`BIT_WIDTH-1:0]				r_uy__14;
reg	[`BIT_WIDTH-1:0]				r_uy__15;
reg	[`BIT_WIDTH-1:0]				r_uy__16;
reg	[`BIT_WIDTH-1:0]				r_uy__17;
reg	[`BIT_WIDTH-1:0]				r_uy__18;
reg	[`BIT_WIDTH-1:0]				r_uy__19;
reg	[`BIT_WIDTH-1:0]				r_uy__20;
reg	[`BIT_WIDTH-1:0]				r_uy__21;
reg	[`BIT_WIDTH-1:0]				r_uy__22;
reg	[`BIT_WIDTH-1:0]				r_uy__23;
reg	[`BIT_WIDTH-1:0]				r_uy__24;
reg	[`BIT_WIDTH-1:0]				r_uy__25;
reg	[`BIT_WIDTH-1:0]				r_uy__26;
reg	[`BIT_WIDTH-1:0]				r_uy__27;
reg	[`BIT_WIDTH-1:0]				r_uy__28;
reg	[`BIT_WIDTH-1:0]				r_uy__29;
reg	[`BIT_WIDTH-1:0]				r_uy__30;
reg	[`BIT_WIDTH-1:0]				r_uy__31;
reg	[`BIT_WIDTH-1:0]				r_uy__32;
reg	[`BIT_WIDTH-1:0]				r_uy__33;
reg	[`BIT_WIDTH-1:0]				r_uy__34;
reg	[`BIT_WIDTH-1:0]				r_uy__35;
reg	[`BIT_WIDTH-1:0]				r_uy__36;
reg	[`BIT_WIDTH-1:0]				r_uy__37;
reg	[`BIT_WIDTH-1:0]				r_uy__38;
reg	[`BIT_WIDTH-1:0]				r_uy__39;
reg	[`BIT_WIDTH-1:0]				r_uy__40;
reg	[`BIT_WIDTH-1:0]				r_uy__41;
reg	[`BIT_WIDTH-1:0]				r_uy__42;
reg	[`BIT_WIDTH-1:0]				r_uy__43;
reg	[`BIT_WIDTH-1:0]				r_uy__44;
reg	[`BIT_WIDTH-1:0]				r_uy__45;
reg	[`BIT_WIDTH-1:0]				r_uy__46;
reg	[`BIT_WIDTH-1:0]				r_uy__47;
reg	[`BIT_WIDTH-1:0]				r_uy__48;
reg	[`BIT_WIDTH-1:0]				r_uy__49;
reg	[`BIT_WIDTH-1:0]				r_uy__50;
reg	[`BIT_WIDTH-1:0]				r_uy__51;
reg	[`BIT_WIDTH-1:0]				r_uy__52;
reg	[`BIT_WIDTH-1:0]				r_uy__53;
reg	[`BIT_WIDTH-1:0]				r_uy__54;
reg	[`BIT_WIDTH-1:0]				r_uy__55;
reg	[`BIT_WIDTH-1:0]				r_uy__56;
reg	[`BIT_WIDTH-1:0]				r_uy__57;
reg	[`BIT_WIDTH-1:0]				r_uy__58;
reg	[`BIT_WIDTH-1:0]				r_uy__59;

//reg	[`BIT_WIDTH-1:0]				r_uz	[TOTAL_LATENCY - 1:0];

reg	[`BIT_WIDTH-1:0]				r_uz__0;
reg	[`BIT_WIDTH-1:0]				r_uz__1;
reg	[`BIT_WIDTH-1:0]				r_uz__2;
reg	[`BIT_WIDTH-1:0]				r_uz__3;
reg	[`BIT_WIDTH-1:0]				r_uz__4;
reg	[`BIT_WIDTH-1:0]				r_uz__5;
reg	[`BIT_WIDTH-1:0]				r_uz__6;
reg	[`BIT_WIDTH-1:0]				r_uz__7;
reg	[`BIT_WIDTH-1:0]				r_uz__8;
reg	[`BIT_WIDTH-1:0]				r_uz__9;
reg	[`BIT_WIDTH-1:0]				r_uz__10;
reg	[`BIT_WIDTH-1:0]				r_uz__11;
reg	[`BIT_WIDTH-1:0]				r_uz__12;
reg	[`BIT_WIDTH-1:0]				r_uz__13;
reg	[`BIT_WIDTH-1:0]				r_uz__14;
reg	[`BIT_WIDTH-1:0]				r_uz__15;
reg	[`BIT_WIDTH-1:0]				r_uz__16;
reg	[`BIT_WIDTH-1:0]				r_uz__17;
reg	[`BIT_WIDTH-1:0]				r_uz__18;
reg	[`BIT_WIDTH-1:0]				r_uz__19;
reg	[`BIT_WIDTH-1:0]				r_uz__20;
reg	[`BIT_WIDTH-1:0]				r_uz__21;
reg	[`BIT_WIDTH-1:0]				r_uz__22;
reg	[`BIT_WIDTH-1:0]				r_uz__23;
reg	[`BIT_WIDTH-1:0]				r_uz__24;
reg	[`BIT_WIDTH-1:0]				r_uz__25;
reg	[`BIT_WIDTH-1:0]				r_uz__26;
reg	[`BIT_WIDTH-1:0]				r_uz__27;
reg	[`BIT_WIDTH-1:0]				r_uz__28;
reg	[`BIT_WIDTH-1:0]				r_uz__29;
reg	[`BIT_WIDTH-1:0]				r_uz__30;
reg	[`BIT_WIDTH-1:0]				r_uz__31;
reg	[`BIT_WIDTH-1:0]				r_uz__32;
reg	[`BIT_WIDTH-1:0]				r_uz__33;
reg	[`BIT_WIDTH-1:0]				r_uz__34;
reg	[`BIT_WIDTH-1:0]				r_uz__35;
reg	[`BIT_WIDTH-1:0]				r_uz__36;
reg	[`BIT_WIDTH-1:0]				r_uz__37;
reg	[`BIT_WIDTH-1:0]				r_uz__38;
reg	[`BIT_WIDTH-1:0]				r_uz__39;
reg	[`BIT_WIDTH-1:0]				r_uz__40;
reg	[`BIT_WIDTH-1:0]				r_uz__41;
reg	[`BIT_WIDTH-1:0]				r_uz__42;
reg	[`BIT_WIDTH-1:0]				r_uz__43;
reg	[`BIT_WIDTH-1:0]				r_uz__44;
reg	[`BIT_WIDTH-1:0]				r_uz__45;
reg	[`BIT_WIDTH-1:0]				r_uz__46;
reg	[`BIT_WIDTH-1:0]				r_uz__47;
reg	[`BIT_WIDTH-1:0]				r_uz__48;
reg	[`BIT_WIDTH-1:0]				r_uz__49;
reg	[`BIT_WIDTH-1:0]				r_uz__50;
reg	[`BIT_WIDTH-1:0]				r_uz__51;
reg	[`BIT_WIDTH-1:0]				r_uz__52;
reg	[`BIT_WIDTH-1:0]				r_uz__53;
reg	[`BIT_WIDTH-1:0]				r_uz__54;
reg	[`BIT_WIDTH-1:0]				r_uz__55;
reg	[`BIT_WIDTH-1:0]				r_uz__56;
reg	[`BIT_WIDTH-1:0]				r_uz__57;
reg	[`BIT_WIDTH-1:0]				r_uz__58;
reg	[`BIT_WIDTH-1:0]				r_uz__59;


//reg	[`BIT_WIDTH-1:0]				r_sz	[TOTAL_LATENCY - 1:0];

reg	[`BIT_WIDTH-1:0]				r_sz__0;
reg	[`BIT_WIDTH-1:0]				r_sz__1;
reg	[`BIT_WIDTH-1:0]				r_sz__2;
reg	[`BIT_WIDTH-1:0]				r_sz__3;
reg	[`BIT_WIDTH-1:0]				r_sz__4;
reg	[`BIT_WIDTH-1:0]				r_sz__5;
reg	[`BIT_WIDTH-1:0]				r_sz__6;
reg	[`BIT_WIDTH-1:0]				r_sz__7;
reg	[`BIT_WIDTH-1:0]				r_sz__8;
reg	[`BIT_WIDTH-1:0]				r_sz__9;
reg	[`BIT_WIDTH-1:0]				r_sz__10;
reg	[`BIT_WIDTH-1:0]				r_sz__11;
reg	[`BIT_WIDTH-1:0]				r_sz__12;
reg	[`BIT_WIDTH-1:0]				r_sz__13;
reg	[`BIT_WIDTH-1:0]				r_sz__14;
reg	[`BIT_WIDTH-1:0]				r_sz__15;
reg	[`BIT_WIDTH-1:0]				r_sz__16;
reg	[`BIT_WIDTH-1:0]				r_sz__17;
reg	[`BIT_WIDTH-1:0]				r_sz__18;
reg	[`BIT_WIDTH-1:0]				r_sz__19;
reg	[`BIT_WIDTH-1:0]				r_sz__20;
reg	[`BIT_WIDTH-1:0]				r_sz__21;
reg	[`BIT_WIDTH-1:0]				r_sz__22;
reg	[`BIT_WIDTH-1:0]				r_sz__23;
reg	[`BIT_WIDTH-1:0]				r_sz__24;
reg	[`BIT_WIDTH-1:0]				r_sz__25;
reg	[`BIT_WIDTH-1:0]				r_sz__26;
reg	[`BIT_WIDTH-1:0]				r_sz__27;
reg	[`BIT_WIDTH-1:0]				r_sz__28;
reg	[`BIT_WIDTH-1:0]				r_sz__29;
reg	[`BIT_WIDTH-1:0]				r_sz__30;
reg	[`BIT_WIDTH-1:0]				r_sz__31;
reg	[`BIT_WIDTH-1:0]				r_sz__32;
reg	[`BIT_WIDTH-1:0]				r_sz__33;
reg	[`BIT_WIDTH-1:0]				r_sz__34;
reg	[`BIT_WIDTH-1:0]				r_sz__35;
reg	[`BIT_WIDTH-1:0]				r_sz__36;
reg	[`BIT_WIDTH-1:0]				r_sz__37;
reg	[`BIT_WIDTH-1:0]				r_sz__38;
reg	[`BIT_WIDTH-1:0]				r_sz__39;
reg	[`BIT_WIDTH-1:0]				r_sz__40;
reg	[`BIT_WIDTH-1:0]				r_sz__41;
reg	[`BIT_WIDTH-1:0]				r_sz__42;
reg	[`BIT_WIDTH-1:0]				r_sz__43;
reg	[`BIT_WIDTH-1:0]				r_sz__44;
reg	[`BIT_WIDTH-1:0]				r_sz__45;
reg	[`BIT_WIDTH-1:0]				r_sz__46;
reg	[`BIT_WIDTH-1:0]				r_sz__47;
reg	[`BIT_WIDTH-1:0]				r_sz__48;
reg	[`BIT_WIDTH-1:0]				r_sz__49;
reg	[`BIT_WIDTH-1:0]				r_sz__50;
reg	[`BIT_WIDTH-1:0]				r_sz__51;
reg	[`BIT_WIDTH-1:0]				r_sz__52;
reg	[`BIT_WIDTH-1:0]				r_sz__53;
reg	[`BIT_WIDTH-1:0]				r_sz__54;
reg	[`BIT_WIDTH-1:0]				r_sz__55;
reg	[`BIT_WIDTH-1:0]				r_sz__56;
reg	[`BIT_WIDTH-1:0]				r_sz__57;
reg	[`BIT_WIDTH-1:0]				r_sz__58;
reg	[`BIT_WIDTH-1:0]				r_sz__59;

//reg	[`BIT_WIDTH-1:0]				r_sr	[TOTAL_LATENCY - 1:0];

reg	[`BIT_WIDTH-1:0]				r_sr__0;
reg	[`BIT_WIDTH-1:0]				r_sr__1;
reg	[`BIT_WIDTH-1:0]				r_sr__2;
reg	[`BIT_WIDTH-1:0]				r_sr__3;
reg	[`BIT_WIDTH-1:0]				r_sr__4;
reg	[`BIT_WIDTH-1:0]				r_sr__5;
reg	[`BIT_WIDTH-1:0]				r_sr__6;
reg	[`BIT_WIDTH-1:0]				r_sr__7;
reg	[`BIT_WIDTH-1:0]				r_sr__8;
reg	[`BIT_WIDTH-1:0]				r_sr__9;
reg	[`BIT_WIDTH-1:0]				r_sr__10;
reg	[`BIT_WIDTH-1:0]				r_sr__11;
reg	[`BIT_WIDTH-1:0]				r_sr__12;
reg	[`BIT_WIDTH-1:0]				r_sr__13;
reg	[`BIT_WIDTH-1:0]				r_sr__14;
reg	[`BIT_WIDTH-1:0]				r_sr__15;
reg	[`BIT_WIDTH-1:0]				r_sr__16;
reg	[`BIT_WIDTH-1:0]				r_sr__17;
reg	[`BIT_WIDTH-1:0]				r_sr__18;
reg	[`BIT_WIDTH-1:0]				r_sr__19;
reg	[`BIT_WIDTH-1:0]				r_sr__20;
reg	[`BIT_WIDTH-1:0]				r_sr__21;
reg	[`BIT_WIDTH-1:0]				r_sr__22;
reg	[`BIT_WIDTH-1:0]				r_sr__23;
reg	[`BIT_WIDTH-1:0]				r_sr__24;
reg	[`BIT_WIDTH-1:0]				r_sr__25;
reg	[`BIT_WIDTH-1:0]				r_sr__26;
reg	[`BIT_WIDTH-1:0]				r_sr__27;
reg	[`BIT_WIDTH-1:0]				r_sr__28;
reg	[`BIT_WIDTH-1:0]				r_sr__29;
reg	[`BIT_WIDTH-1:0]				r_sr__30;
reg	[`BIT_WIDTH-1:0]				r_sr__31;
reg	[`BIT_WIDTH-1:0]				r_sr__32;
reg	[`BIT_WIDTH-1:0]				r_sr__33;
reg	[`BIT_WIDTH-1:0]				r_sr__34;
reg	[`BIT_WIDTH-1:0]				r_sr__35;
reg	[`BIT_WIDTH-1:0]				r_sr__36;
reg	[`BIT_WIDTH-1:0]				r_sr__37;
reg	[`BIT_WIDTH-1:0]				r_sr__38;
reg	[`BIT_WIDTH-1:0]				r_sr__39;
reg	[`BIT_WIDTH-1:0]				r_sr__40;
reg	[`BIT_WIDTH-1:0]				r_sr__41;
reg	[`BIT_WIDTH-1:0]				r_sr__42;
reg	[`BIT_WIDTH-1:0]				r_sr__43;
reg	[`BIT_WIDTH-1:0]				r_sr__44;
reg	[`BIT_WIDTH-1:0]				r_sr__45;
reg	[`BIT_WIDTH-1:0]				r_sr__46;
reg	[`BIT_WIDTH-1:0]				r_sr__47;
reg	[`BIT_WIDTH-1:0]				r_sr__48;
reg	[`BIT_WIDTH-1:0]				r_sr__49;
reg	[`BIT_WIDTH-1:0]				r_sr__50;
reg	[`BIT_WIDTH-1:0]				r_sr__51;
reg	[`BIT_WIDTH-1:0]				r_sr__52;
reg	[`BIT_WIDTH-1:0]				r_sr__53;
reg	[`BIT_WIDTH-1:0]				r_sr__54;
reg	[`BIT_WIDTH-1:0]				r_sr__55;
reg	[`BIT_WIDTH-1:0]				r_sr__56;
reg	[`BIT_WIDTH-1:0]				r_sr__57;
reg	[`BIT_WIDTH-1:0]				r_sr__58;
reg	[`BIT_WIDTH-1:0]				r_sr__59;

//reg	[`BIT_WIDTH-1:0]				r_sleftz	[TOTAL_LATENCY - 1:0];

reg	[`BIT_WIDTH-1:0]				r_sleftz__0;
reg	[`BIT_WIDTH-1:0]				r_sleftz__1;
reg	[`BIT_WIDTH-1:0]				r_sleftz__2;
reg	[`BIT_WIDTH-1:0]				r_sleftz__3;
reg	[`BIT_WIDTH-1:0]				r_sleftz__4;
reg	[`BIT_WIDTH-1:0]				r_sleftz__5;
reg	[`BIT_WIDTH-1:0]				r_sleftz__6;
reg	[`BIT_WIDTH-1:0]				r_sleftz__7;
reg	[`BIT_WIDTH-1:0]				r_sleftz__8;
reg	[`BIT_WIDTH-1:0]				r_sleftz__9;
reg	[`BIT_WIDTH-1:0]				r_sleftz__10;
reg	[`BIT_WIDTH-1:0]				r_sleftz__11;
reg	[`BIT_WIDTH-1:0]				r_sleftz__12;
reg	[`BIT_WIDTH-1:0]				r_sleftz__13;
reg	[`BIT_WIDTH-1:0]				r_sleftz__14;
reg	[`BIT_WIDTH-1:0]				r_sleftz__15;
reg	[`BIT_WIDTH-1:0]				r_sleftz__16;
reg	[`BIT_WIDTH-1:0]				r_sleftz__17;
reg	[`BIT_WIDTH-1:0]				r_sleftz__18;
reg	[`BIT_WIDTH-1:0]				r_sleftz__19;
reg	[`BIT_WIDTH-1:0]				r_sleftz__20;
reg	[`BIT_WIDTH-1:0]				r_sleftz__21;
reg	[`BIT_WIDTH-1:0]				r_sleftz__22;
reg	[`BIT_WIDTH-1:0]				r_sleftz__23;
reg	[`BIT_WIDTH-1:0]				r_sleftz__24;
reg	[`BIT_WIDTH-1:0]				r_sleftz__25;
reg	[`BIT_WIDTH-1:0]				r_sleftz__26;
reg	[`BIT_WIDTH-1:0]				r_sleftz__27;
reg	[`BIT_WIDTH-1:0]				r_sleftz__28;
reg	[`BIT_WIDTH-1:0]				r_sleftz__29;
reg	[`BIT_WIDTH-1:0]				r_sleftz__30;
reg	[`BIT_WIDTH-1:0]				r_sleftz__31;
reg	[`BIT_WIDTH-1:0]				r_sleftz__32;
reg	[`BIT_WIDTH-1:0]				r_sleftz__33;
reg	[`BIT_WIDTH-1:0]				r_sleftz__34;
reg	[`BIT_WIDTH-1:0]				r_sleftz__35;
reg	[`BIT_WIDTH-1:0]				r_sleftz__36;
reg	[`BIT_WIDTH-1:0]				r_sleftz__37;
reg	[`BIT_WIDTH-1:0]				r_sleftz__38;
reg	[`BIT_WIDTH-1:0]				r_sleftz__39;
reg	[`BIT_WIDTH-1:0]				r_sleftz__40;
reg	[`BIT_WIDTH-1:0]				r_sleftz__41;
reg	[`BIT_WIDTH-1:0]				r_sleftz__42;
reg	[`BIT_WIDTH-1:0]				r_sleftz__43;
reg	[`BIT_WIDTH-1:0]				r_sleftz__44;
reg	[`BIT_WIDTH-1:0]				r_sleftz__45;
reg	[`BIT_WIDTH-1:0]				r_sleftz__46;
reg	[`BIT_WIDTH-1:0]				r_sleftz__47;
reg	[`BIT_WIDTH-1:0]				r_sleftz__48;
reg	[`BIT_WIDTH-1:0]				r_sleftz__49;
reg	[`BIT_WIDTH-1:0]				r_sleftz__50;
reg	[`BIT_WIDTH-1:0]				r_sleftz__51;
reg	[`BIT_WIDTH-1:0]				r_sleftz__52;
reg	[`BIT_WIDTH-1:0]				r_sleftz__53;
reg	[`BIT_WIDTH-1:0]				r_sleftz__54;
reg	[`BIT_WIDTH-1:0]				r_sleftz__55;
reg	[`BIT_WIDTH-1:0]				r_sleftz__56;
reg	[`BIT_WIDTH-1:0]				r_sleftz__57;
reg	[`BIT_WIDTH-1:0]				r_sleftz__58;
reg	[`BIT_WIDTH-1:0]				r_sleftz__59;



//reg	[`BIT_WIDTH-1:0]				r_sleftr	[TOTAL_LATENCY - 1:0];

reg	[`BIT_WIDTH-1:0]				r_sleftr__0;
reg	[`BIT_WIDTH-1:0]				r_sleftr__1;
reg	[`BIT_WIDTH-1:0]				r_sleftr__2;
reg	[`BIT_WIDTH-1:0]				r_sleftr__3;
reg	[`BIT_WIDTH-1:0]				r_sleftr__4;
reg	[`BIT_WIDTH-1:0]				r_sleftr__5;
reg	[`BIT_WIDTH-1:0]				r_sleftr__6;
reg	[`BIT_WIDTH-1:0]				r_sleftr__7;
reg	[`BIT_WIDTH-1:0]				r_sleftr__8;
reg	[`BIT_WIDTH-1:0]				r_sleftr__9;
reg	[`BIT_WIDTH-1:0]				r_sleftr__10;
reg	[`BIT_WIDTH-1:0]				r_sleftr__11;
reg	[`BIT_WIDTH-1:0]				r_sleftr__12;
reg	[`BIT_WIDTH-1:0]				r_sleftr__13;

reg	[`BIT_WIDTH-1:0]				r_sleftr__14;
reg	[`BIT_WIDTH-1:0]				r_sleftr__15;
reg	[`BIT_WIDTH-1:0]				r_sleftr__16;
reg	[`BIT_WIDTH-1:0]				r_sleftr__17;
reg	[`BIT_WIDTH-1:0]				r_sleftr__18;
reg	[`BIT_WIDTH-1:0]				r_sleftr__19;
reg	[`BIT_WIDTH-1:0]				r_sleftr__20;
reg	[`BIT_WIDTH-1:0]				r_sleftr__21;
reg	[`BIT_WIDTH-1:0]				r_sleftr__22;
reg	[`BIT_WIDTH-1:0]				r_sleftr__23;
reg	[`BIT_WIDTH-1:0]				r_sleftr__24;
reg	[`BIT_WIDTH-1:0]				r_sleftr__25;
reg	[`BIT_WIDTH-1:0]				r_sleftr__26;
reg	[`BIT_WIDTH-1:0]				r_sleftr__27;
reg	[`BIT_WIDTH-1:0]				r_sleftr__28;
reg	[`BIT_WIDTH-1:0]				r_sleftr__29;
reg	[`BIT_WIDTH-1:0]				r_sleftr__30;
reg	[`BIT_WIDTH-1:0]				r_sleftr__31;
reg	[`BIT_WIDTH-1:0]				r_sleftr__32;
reg	[`BIT_WIDTH-1:0]				r_sleftr__33;
reg	[`BIT_WIDTH-1:0]				r_sleftr__34;
reg	[`BIT_WIDTH-1:0]				r_sleftr__35;
reg	[`BIT_WIDTH-1:0]				r_sleftr__36;
reg	[`BIT_WIDTH-1:0]				r_sleftr__37;
reg	[`BIT_WIDTH-1:0]				r_sleftr__38;
reg	[`BIT_WIDTH-1:0]				r_sleftr__39;
reg	[`BIT_WIDTH-1:0]				r_sleftr__40;
reg	[`BIT_WIDTH-1:0]				r_sleftr__41;
reg	[`BIT_WIDTH-1:0]				r_sleftr__42;
reg	[`BIT_WIDTH-1:0]				r_sleftr__43;
reg	[`BIT_WIDTH-1:0]				r_sleftr__44;
reg	[`BIT_WIDTH-1:0]				r_sleftr__45;
reg	[`BIT_WIDTH-1:0]				r_sleftr__46;
reg	[`BIT_WIDTH-1:0]				r_sleftr__47;
reg	[`BIT_WIDTH-1:0]				r_sleftr__48;
reg	[`BIT_WIDTH-1:0]				r_sleftr__49;
reg	[`BIT_WIDTH-1:0]				r_sleftr__50;
reg	[`BIT_WIDTH-1:0]				r_sleftr__51;
reg	[`BIT_WIDTH-1:0]				r_sleftr__52;
reg	[`BIT_WIDTH-1:0]				r_sleftr__53;
reg	[`BIT_WIDTH-1:0]				r_sleftr__54;
reg	[`BIT_WIDTH-1:0]				r_sleftr__55;
reg	[`BIT_WIDTH-1:0]				r_sleftr__56;
reg	[`BIT_WIDTH-1:0]				r_sleftr__57;
reg	[`BIT_WIDTH-1:0]				r_sleftr__58;
reg	[`BIT_WIDTH-1:0]				r_sleftr__59;


//reg	[`BIT_WIDTH-1:0]				r_weight	[TOTAL_LATENCY - 1:0];

reg	[`BIT_WIDTH-1:0]				r_weight__0;
reg	[`BIT_WIDTH-1:0]				r_weight__1;
reg	[`BIT_WIDTH-1:0]				r_weight__2;
reg	[`BIT_WIDTH-1:0]				r_weight__3;
reg	[`BIT_WIDTH-1:0]				r_weight__4;
reg	[`BIT_WIDTH-1:0]				r_weight__5;
reg	[`BIT_WIDTH-1:0]				r_weight__6;
reg	[`BIT_WIDTH-1:0]				r_weight__7;
reg	[`BIT_WIDTH-1:0]				r_weight__8;
reg	[`BIT_WIDTH-1:0]				r_weight__9;
reg	[`BIT_WIDTH-1:0]				r_weight__10;
reg	[`BIT_WIDTH-1:0]				r_weight__11;
reg	[`BIT_WIDTH-1:0]				r_weight__12;
reg	[`BIT_WIDTH-1:0]				r_weight__13;
reg	[`BIT_WIDTH-1:0]				r_weight__14;
reg	[`BIT_WIDTH-1:0]				r_weight__15;
reg	[`BIT_WIDTH-1:0]				r_weight__16;
reg	[`BIT_WIDTH-1:0]				r_weight__17;
reg	[`BIT_WIDTH-1:0]				r_weight__18;
reg	[`BIT_WIDTH-1:0]				r_weight__19;
reg	[`BIT_WIDTH-1:0]				r_weight__20;
reg	[`BIT_WIDTH-1:0]				r_weight__21;
reg	[`BIT_WIDTH-1:0]				r_weight__22;
reg	[`BIT_WIDTH-1:0]				r_weight__23;
reg	[`BIT_WIDTH-1:0]				r_weight__24;
reg	[`BIT_WIDTH-1:0]				r_weight__25;
reg	[`BIT_WIDTH-1:0]				r_weight__26;
reg	[`BIT_WIDTH-1:0]				r_weight__27;
reg	[`BIT_WIDTH-1:0]				r_weight__28;
reg	[`BIT_WIDTH-1:0]				r_weight__29;
reg	[`BIT_WIDTH-1:0]				r_weight__30;
reg	[`BIT_WIDTH-1:0]				r_weight__31;
reg	[`BIT_WIDTH-1:0]				r_weight__32;
reg	[`BIT_WIDTH-1:0]				r_weight__33;
reg	[`BIT_WIDTH-1:0]				r_weight__34;
reg	[`BIT_WIDTH-1:0]				r_weight__35;
reg	[`BIT_WIDTH-1:0]				r_weight__36;
reg	[`BIT_WIDTH-1:0]				r_weight__37;
reg	[`BIT_WIDTH-1:0]				r_weight__38;
reg	[`BIT_WIDTH-1:0]				r_weight__39;
reg	[`BIT_WIDTH-1:0]				r_weight__40;
reg	[`BIT_WIDTH-1:0]				r_weight__41;
reg	[`BIT_WIDTH-1:0]				r_weight__42;
reg	[`BIT_WIDTH-1:0]				r_weight__43;
reg	[`BIT_WIDTH-1:0]				r_weight__44;
reg	[`BIT_WIDTH-1:0]				r_weight__45;
reg	[`BIT_WIDTH-1:0]				r_weight__46;
reg	[`BIT_WIDTH-1:0]				r_weight__47;
reg	[`BIT_WIDTH-1:0]				r_weight__48;
reg	[`BIT_WIDTH-1:0]				r_weight__49;
reg	[`BIT_WIDTH-1:0]				r_weight__50;
reg	[`BIT_WIDTH-1:0]				r_weight__51;
reg	[`BIT_WIDTH-1:0]				r_weight__52;
reg	[`BIT_WIDTH-1:0]				r_weight__53;
reg	[`BIT_WIDTH-1:0]				r_weight__54;
reg	[`BIT_WIDTH-1:0]				r_weight__55;
reg	[`BIT_WIDTH-1:0]				r_weight__56;
reg	[`BIT_WIDTH-1:0]				r_weight__57;
reg	[`BIT_WIDTH-1:0]				r_weight__58;
reg	[`BIT_WIDTH-1:0]				r_weight__59;

//reg	[`LAYER_WIDTH-1:0]			r_layer	[TOTAL_LATENCY - 1:0];

reg	[`LAYER_WIDTH-1:0]				r_layer__0;
reg	[`LAYER_WIDTH-1:0]				r_layer__1;
reg	[`LAYER_WIDTH-1:0]				r_layer__2;
reg	[`LAYER_WIDTH-1:0]				r_layer__3;
reg	[`LAYER_WIDTH-1:0]				r_layer__4;
reg	[`LAYER_WIDTH-1:0]				r_layer__5;
reg	[`LAYER_WIDTH-1:0]				r_layer__6;
reg	[`LAYER_WIDTH-1:0]				r_layer__7;
reg	[`LAYER_WIDTH-1:0]				r_layer__8;
reg	[`LAYER_WIDTH-1:0]				r_layer__9;
reg	[`LAYER_WIDTH-1:0]				r_layer__10;
reg	[`LAYER_WIDTH-1:0]				r_layer__11;
reg	[`LAYER_WIDTH-1:0]				r_layer__12;
reg	[`LAYER_WIDTH-1:0]				r_layer__13;
reg	[`LAYER_WIDTH-1:0]				r_layer__14;
reg	[`LAYER_WIDTH-1:0]				r_layer__15;
reg	[`LAYER_WIDTH-1:0]				r_layer__16;
reg	[`LAYER_WIDTH-1:0]				r_layer__17;
reg	[`LAYER_WIDTH-1:0]				r_layer__18;
reg	[`LAYER_WIDTH-1:0]				r_layer__19;
reg	[`LAYER_WIDTH-1:0]				r_layer__20;
reg	[`LAYER_WIDTH-1:0]				r_layer__21;
reg	[`LAYER_WIDTH-1:0]				r_layer__22;
reg	[`LAYER_WIDTH-1:0]				r_layer__23;
reg	[`LAYER_WIDTH-1:0]				r_layer__24;
reg	[`LAYER_WIDTH-1:0]				r_layer__25;
reg	[`LAYER_WIDTH-1:0]				r_layer__26;
reg	[`LAYER_WIDTH-1:0]				r_layer__27;
reg	[`LAYER_WIDTH-1:0]				r_layer__28;
reg	[`LAYER_WIDTH-1:0]				r_layer__29;
reg	[`LAYER_WIDTH-1:0]				r_layer__30;
reg	[`LAYER_WIDTH-1:0]				r_layer__31;
reg	[`LAYER_WIDTH-1:0]				r_layer__32;
reg	[`LAYER_WIDTH-1:0]				r_layer__33;
reg	[`LAYER_WIDTH-1:0]				r_layer__34;
reg	[`LAYER_WIDTH-1:0]				r_layer__35;
reg	[`LAYER_WIDTH-1:0]				r_layer__36;
reg	[`LAYER_WIDTH-1:0]				r_layer__37;
reg	[`LAYER_WIDTH-1:0]				r_layer__38;
reg	[`LAYER_WIDTH-1:0]				r_layer__39;
reg	[`LAYER_WIDTH-1:0]				r_layer__40;
reg	[`LAYER_WIDTH-1:0]				r_layer__41;
reg	[`LAYER_WIDTH-1:0]				r_layer__42;
reg	[`LAYER_WIDTH-1:0]				r_layer__43;
reg	[`LAYER_WIDTH-1:0]				r_layer__44;
reg	[`LAYER_WIDTH-1:0]				r_layer__45;
reg	[`LAYER_WIDTH-1:0]				r_layer__46;
reg	[`LAYER_WIDTH-1:0]				r_layer__47;
reg	[`LAYER_WIDTH-1:0]				r_layer__48;
reg	[`LAYER_WIDTH-1:0]				r_layer__49;
reg	[`LAYER_WIDTH-1:0]				r_layer__50;
reg	[`LAYER_WIDTH-1:0]				r_layer__51;
reg	[`LAYER_WIDTH-1:0]				r_layer__52;
reg	[`LAYER_WIDTH-1:0]				r_layer__53;
reg	[`LAYER_WIDTH-1:0]				r_layer__54;
reg	[`LAYER_WIDTH-1:0]				r_layer__55;
reg	[`LAYER_WIDTH-1:0]				r_layer__56;
reg	[`LAYER_WIDTH-1:0]				r_layer__57;
reg	[`LAYER_WIDTH-1:0]				r_layer__58;
reg	[`LAYER_WIDTH-1:0]				r_layer__59;

//reg								r_dead	[TOTAL_LATENCY - 1:0];

reg					r_dead__0;
reg					r_dead__1;
reg					r_dead__2;
reg					r_dead__3;
reg					r_dead__4;
reg					r_dead__5;
reg					r_dead__6;
reg					r_dead__7;
reg					r_dead__8;
reg					r_dead__9;
reg					r_dead__10;
reg					r_dead__11;
reg					r_dead__12;
reg					r_dead__13;
reg					r_dead__14;
reg					r_dead__15;
reg					r_dead__16;
reg					r_dead__17;
reg					r_dead__18;
reg					r_dead__19;
reg					r_dead__20;
reg					r_dead__21;
reg					r_dead__22;
reg					r_dead__23;
reg					r_dead__24;
reg					r_dead__25;
reg					r_dead__26;
reg					r_dead__27;
reg					r_dead__28;
reg					r_dead__29;
reg					r_dead__30;
reg					r_dead__31;
reg					r_dead__32;
reg					r_dead__33;
reg					r_dead__34;
reg					r_dead__35;
reg					r_dead__36;
reg					r_dead__37;
reg					r_dead__38;
reg					r_dead__39;
reg					r_dead__40;
reg					r_dead__41;
reg					r_dead__42;
reg					r_dead__43;
reg					r_dead__44;
reg					r_dead__45;
reg					r_dead__46;
reg					r_dead__47;
reg					r_dead__48;
reg					r_dead__49;
reg					r_dead__50;
reg					r_dead__51;
reg					r_dead__52;
reg					r_dead__53;
reg					r_dead__54;
reg					r_dead__55;
reg					r_dead__56;
reg					r_dead__57;
reg					r_dead__58;
reg					r_dead__59;

//reg								r_hit	[TOTAL_LATENCY - 1:0];

reg					r_hit__0;
reg					r_hit__1;
reg					r_hit__2;
reg					r_hit__3;
reg					r_hit__4;
reg					r_hit__5;
reg					r_hit__6;
reg					r_hit__7;
reg					r_hit__8;
reg					r_hit__9;
reg					r_hit__10;
reg					r_hit__11;
reg					r_hit__12;
reg					r_hit__13;
reg					r_hit__14;
reg					r_hit__15;
reg					r_hit__16;
reg					r_hit__17;
reg					r_hit__18;
reg					r_hit__19;
reg					r_hit__20;
reg					r_hit__21;
reg					r_hit__22;
reg					r_hit__23;
reg					r_hit__24;
reg					r_hit__25;
reg					r_hit__26;
reg					r_hit__27;
reg					r_hit__28;
reg					r_hit__29;
reg					r_hit__30;
reg					r_hit__31;
reg					r_hit__32;
reg					r_hit__33;
reg					r_hit__34;
reg					r_hit__35;
reg					r_hit__36;
reg					r_hit__37;
reg					r_hit__38;
reg					r_hit__39;
reg					r_hit__40;
reg					r_hit__41;
reg					r_hit__42;
reg					r_hit__43;
reg					r_hit__44;
reg					r_hit__45;
reg					r_hit__46;
reg					r_hit__47;
reg					r_hit__48;
reg					r_hit__49;
reg					r_hit__50;
reg					r_hit__51;
reg					r_hit__52;
reg					r_hit__53;
reg					r_hit__54;
reg					r_hit__55;
reg					r_hit__56;
reg					r_hit__57;
reg					r_hit__58;
reg					r_hit__59;

//reg	[`BIT_WIDTH-1:0]				r_diff[TOTAL_LATENCY - 1:0];

reg	[`BIT_WIDTH-1:0]				r_diff__0;
reg	[`BIT_WIDTH-1:0]				r_diff__1;
reg	[`BIT_WIDTH-1:0]				r_diff__2;
reg	[`BIT_WIDTH-1:0]				r_diff__3;
reg	[`BIT_WIDTH-1:0]				r_diff__4;
reg	[`BIT_WIDTH-1:0]				r_diff__5;
reg	[`BIT_WIDTH-1:0]				r_diff__6;
reg	[`BIT_WIDTH-1:0]				r_diff__7;
reg	[`BIT_WIDTH-1:0]				r_diff__8;
reg	[`BIT_WIDTH-1:0]				r_diff__9;
reg	[`BIT_WIDTH-1:0]				r_diff__10;
reg	[`BIT_WIDTH-1:0]				r_diff__11;
reg	[`BIT_WIDTH-1:0]				r_diff__12;
reg	[`BIT_WIDTH-1:0]				r_diff__13;
reg	[`BIT_WIDTH-1:0]				r_diff__14;
reg	[`BIT_WIDTH-1:0]				r_diff__15;
reg	[`BIT_WIDTH-1:0]				r_diff__16;
reg	[`BIT_WIDTH-1:0]				r_diff__17;
reg	[`BIT_WIDTH-1:0]				r_diff__18;
reg	[`BIT_WIDTH-1:0]				r_diff__19;
reg	[`BIT_WIDTH-1:0]				r_diff__20;
reg	[`BIT_WIDTH-1:0]				r_diff__21;
reg	[`BIT_WIDTH-1:0]				r_diff__22;
reg	[`BIT_WIDTH-1:0]				r_diff__23;
reg	[`BIT_WIDTH-1:0]				r_diff__24;
reg	[`BIT_WIDTH-1:0]				r_diff__25;
reg	[`BIT_WIDTH-1:0]				r_diff__26;
reg	[`BIT_WIDTH-1:0]				r_diff__27;
reg	[`BIT_WIDTH-1:0]				r_diff__28;
reg	[`BIT_WIDTH-1:0]				r_diff__29;
reg	[`BIT_WIDTH-1:0]				r_diff__30;
reg	[`BIT_WIDTH-1:0]				r_diff__31;
reg	[`BIT_WIDTH-1:0]				r_diff__32;
reg	[`BIT_WIDTH-1:0]				r_diff__33;
reg	[`BIT_WIDTH-1:0]				r_diff__34;
reg	[`BIT_WIDTH-1:0]				r_diff__35;
reg	[`BIT_WIDTH-1:0]				r_diff__36;
reg	[`BIT_WIDTH-1:0]				r_diff__37;
reg	[`BIT_WIDTH-1:0]				r_diff__38;
reg	[`BIT_WIDTH-1:0]				r_diff__39;
reg	[`BIT_WIDTH-1:0]				r_diff__40;
reg	[`BIT_WIDTH-1:0]				r_diff__41;
reg	[`BIT_WIDTH-1:0]				r_diff__42;
reg	[`BIT_WIDTH-1:0]				r_diff__43;
reg	[`BIT_WIDTH-1:0]				r_diff__44;
reg	[`BIT_WIDTH-1:0]				r_diff__45;
reg	[`BIT_WIDTH-1:0]				r_diff__46;
reg	[`BIT_WIDTH-1:0]				r_diff__47;
reg	[`BIT_WIDTH-1:0]				r_diff__48;
reg	[`BIT_WIDTH-1:0]				r_diff__49;
reg	[`BIT_WIDTH-1:0]				r_diff__50;
reg	[`BIT_WIDTH-1:0]				r_diff__51;
reg	[`BIT_WIDTH-1:0]				r_diff__52;
reg	[`BIT_WIDTH-1:0]				r_diff__53;
reg	[`BIT_WIDTH-1:0]				r_diff__54;
reg	[`BIT_WIDTH-1:0]				r_diff__55;
reg	[`BIT_WIDTH-1:0]				r_diff__56;
reg	[`BIT_WIDTH-1:0]				r_diff__57;
reg	[`BIT_WIDTH-1:0]				r_diff__58;
reg	[`BIT_WIDTH-1:0]				r_diff__59;

//reg	[`BIT_WIDTH-1:0]				r_dl_b[TOTAL_LATENCY - 1:0];

reg	[`BIT_WIDTH-1:0]				r_dl_b__0;
reg	[`BIT_WIDTH-1:0]				r_dl_b__1;
reg	[`BIT_WIDTH-1:0]				r_dl_b__2;
reg	[`BIT_WIDTH-1:0]				r_dl_b__3;
reg	[`BIT_WIDTH-1:0]				r_dl_b__4;
reg	[`BIT_WIDTH-1:0]				r_dl_b__5;
reg	[`BIT_WIDTH-1:0]				r_dl_b__6;
reg	[`BIT_WIDTH-1:0]				r_dl_b__7;
reg	[`BIT_WIDTH-1:0]				r_dl_b__8;
reg	[`BIT_WIDTH-1:0]				r_dl_b__9;
reg	[`BIT_WIDTH-1:0]				r_dl_b__10;
reg	[`BIT_WIDTH-1:0]				r_dl_b__11;
reg	[`BIT_WIDTH-1:0]				r_dl_b__12;
reg	[`BIT_WIDTH-1:0]				r_dl_b__13;
reg	[`BIT_WIDTH-1:0]				r_dl_b__14;
reg	[`BIT_WIDTH-1:0]				r_dl_b__15;
reg	[`BIT_WIDTH-1:0]				r_dl_b__16;
reg	[`BIT_WIDTH-1:0]				r_dl_b__17;
reg	[`BIT_WIDTH-1:0]				r_dl_b__18;
reg	[`BIT_WIDTH-1:0]				r_dl_b__19;
reg	[`BIT_WIDTH-1:0]				r_dl_b__20;
reg	[`BIT_WIDTH-1:0]				r_dl_b__21;
reg	[`BIT_WIDTH-1:0]				r_dl_b__22;
reg	[`BIT_WIDTH-1:0]				r_dl_b__23;
reg	[`BIT_WIDTH-1:0]				r_dl_b__24;
reg	[`BIT_WIDTH-1:0]				r_dl_b__25;
reg	[`BIT_WIDTH-1:0]				r_dl_b__26;
reg	[`BIT_WIDTH-1:0]				r_dl_b__27;
reg	[`BIT_WIDTH-1:0]				r_dl_b__28;
reg	[`BIT_WIDTH-1:0]				r_dl_b__29;
reg	[`BIT_WIDTH-1:0]				r_dl_b__30;
reg	[`BIT_WIDTH-1:0]				r_dl_b__31;
reg	[`BIT_WIDTH-1:0]				r_dl_b__32;
reg	[`BIT_WIDTH-1:0]				r_dl_b__33;
reg	[`BIT_WIDTH-1:0]				r_dl_b__34;
reg	[`BIT_WIDTH-1:0]				r_dl_b__35;
reg	[`BIT_WIDTH-1:0]				r_dl_b__36;
reg	[`BIT_WIDTH-1:0]				r_dl_b__37;
reg	[`BIT_WIDTH-1:0]				r_dl_b__38;
reg	[`BIT_WIDTH-1:0]				r_dl_b__39;
reg	[`BIT_WIDTH-1:0]				r_dl_b__40;
reg	[`BIT_WIDTH-1:0]				r_dl_b__41;
reg	[`BIT_WIDTH-1:0]				r_dl_b__42;
reg	[`BIT_WIDTH-1:0]				r_dl_b__43;
reg	[`BIT_WIDTH-1:0]				r_dl_b__44;
reg	[`BIT_WIDTH-1:0]				r_dl_b__45;
reg	[`BIT_WIDTH-1:0]				r_dl_b__46;
reg	[`BIT_WIDTH-1:0]				r_dl_b__47;
reg	[`BIT_WIDTH-1:0]				r_dl_b__48;
reg	[`BIT_WIDTH-1:0]				r_dl_b__49;
reg	[`BIT_WIDTH-1:0]				r_dl_b__50;
reg	[`BIT_WIDTH-1:0]				r_dl_b__51;
reg	[`BIT_WIDTH-1:0]				r_dl_b__52;
reg	[`BIT_WIDTH-1:0]				r_dl_b__53;
reg	[`BIT_WIDTH-1:0]				r_dl_b__54;
reg	[`BIT_WIDTH-1:0]				r_dl_b__55;
reg	[`BIT_WIDTH-1:0]				r_dl_b__56;
reg	[`BIT_WIDTH-1:0]				r_dl_b__57;
reg	[`BIT_WIDTH-1:0]				r_dl_b__58;
reg	[`BIT_WIDTH-1:0]				r_dl_b__59;

//reg	[2*`BIT_WIDTH-1:0]			r_numer[TOTAL_LATENCY - 1:0];

reg	[2*`BIT_WIDTH-1:0]				r_numer__0;
reg	[2*`BIT_WIDTH-1:0]				r_numer__1;
reg	[2*`BIT_WIDTH-1:0]				r_numer__2;
reg	[2*`BIT_WIDTH-1:0]				r_numer__3;
reg	[2*`BIT_WIDTH-1:0]				r_numer__4;
reg	[2*`BIT_WIDTH-1:0]				r_numer__5;
reg	[2*`BIT_WIDTH-1:0]				r_numer__6;
reg	[2*`BIT_WIDTH-1:0]				r_numer__7;
reg	[2*`BIT_WIDTH-1:0]				r_numer__8;
reg	[2*`BIT_WIDTH-1:0]				r_numer__9;
reg	[2*`BIT_WIDTH-1:0]				r_numer__10;
reg	[2*`BIT_WIDTH-1:0]				r_numer__11;
reg	[2*`BIT_WIDTH-1:0]				r_numer__12;
reg	[2*`BIT_WIDTH-1:0]				r_numer__13;
reg	[2*`BIT_WIDTH-1:0]				r_numer__14;
reg	[2*`BIT_WIDTH-1:0]				r_numer__15;
reg	[2*`BIT_WIDTH-1:0]				r_numer__16;
reg	[2*`BIT_WIDTH-1:0]				r_numer__17;
reg	[2*`BIT_WIDTH-1:0]				r_numer__18;
reg	[2*`BIT_WIDTH-1:0]				r_numer__19;
reg	[2*`BIT_WIDTH-1:0]				r_numer__20;
reg	[2*`BIT_WIDTH-1:0]				r_numer__21;
reg	[2*`BIT_WIDTH-1:0]				r_numer__22;
reg	[2*`BIT_WIDTH-1:0]				r_numer__23;
reg	[2*`BIT_WIDTH-1:0]				r_numer__24;
reg	[2*`BIT_WIDTH-1:0]				r_numer__25;
reg	[2*`BIT_WIDTH-1:0]				r_numer__26;
reg	[2*`BIT_WIDTH-1:0]				r_numer__27;
reg	[2*`BIT_WIDTH-1:0]				r_numer__28;
reg	[2*`BIT_WIDTH-1:0]				r_numer__29;
reg	[2*`BIT_WIDTH-1:0]				r_numer__30;
reg	[2*`BIT_WIDTH-1:0]				r_numer__31;
reg	[2*`BIT_WIDTH-1:0]				r_numer__32;
reg	[2*`BIT_WIDTH-1:0]				r_numer__33;
reg	[2*`BIT_WIDTH-1:0]				r_numer__34;
reg	[2*`BIT_WIDTH-1:0]				r_numer__35;
reg	[2*`BIT_WIDTH-1:0]				r_numer__36;
reg	[2*`BIT_WIDTH-1:0]				r_numer__37;
reg	[2*`BIT_WIDTH-1:0]				r_numer__38;
reg	[2*`BIT_WIDTH-1:0]				r_numer__39;
reg	[2*`BIT_WIDTH-1:0]				r_numer__40;
reg	[2*`BIT_WIDTH-1:0]				r_numer__41;
reg	[2*`BIT_WIDTH-1:0]				r_numer__42;
reg	[2*`BIT_WIDTH-1:0]				r_numer__43;
reg	[2*`BIT_WIDTH-1:0]				r_numer__44;
reg	[2*`BIT_WIDTH-1:0]				r_numer__45;
reg	[2*`BIT_WIDTH-1:0]				r_numer__46;
reg	[2*`BIT_WIDTH-1:0]				r_numer__47;
reg	[2*`BIT_WIDTH-1:0]				r_numer__48;
reg	[2*`BIT_WIDTH-1:0]				r_numer__49;
reg	[2*`BIT_WIDTH-1:0]				r_numer__50;
reg	[2*`BIT_WIDTH-1:0]				r_numer__51;
reg	[2*`BIT_WIDTH-1:0]				r_numer__52;
reg	[2*`BIT_WIDTH-1:0]				r_numer__53;
reg	[2*`BIT_WIDTH-1:0]				r_numer__54;
reg	[2*`BIT_WIDTH-1:0]				r_numer__55;
reg	[2*`BIT_WIDTH-1:0]				r_numer__56;
reg	[2*`BIT_WIDTH-1:0]				r_numer__57;
reg	[2*`BIT_WIDTH-1:0]				r_numer__58;
reg	[2*`BIT_WIDTH-1:0]				r_numer__59;

//reg	[`BIT_WIDTH-1:0]				r_z1[TOTAL_LATENCY - 1:0];

reg	[`BIT_WIDTH-1:0]				r_z1__0;
reg	[`BIT_WIDTH-1:0]				r_z1__1;
reg	[`BIT_WIDTH-1:0]				r_z1__2;
reg	[`BIT_WIDTH-1:0]				r_z1__3;
reg	[`BIT_WIDTH-1:0]				r_z1__4;
reg	[`BIT_WIDTH-1:0]				r_z1__5;
reg	[`BIT_WIDTH-1:0]				r_z1__6;
reg	[`BIT_WIDTH-1:0]				r_z1__7;
reg	[`BIT_WIDTH-1:0]				r_z1__8;
reg	[`BIT_WIDTH-1:0]				r_z1__9;
reg	[`BIT_WIDTH-1:0]				r_z1__10;
reg	[`BIT_WIDTH-1:0]				r_z1__11;
reg	[`BIT_WIDTH-1:0]				r_z1__12;
reg	[`BIT_WIDTH-1:0]				r_z1__13;
reg	[`BIT_WIDTH-1:0]				r_z1__14;
reg	[`BIT_WIDTH-1:0]				r_z1__15;
reg	[`BIT_WIDTH-1:0]				r_z1__16;
reg	[`BIT_WIDTH-1:0]				r_z1__17;
reg	[`BIT_WIDTH-1:0]				r_z1__18;
reg	[`BIT_WIDTH-1:0]				r_z1__19;
reg	[`BIT_WIDTH-1:0]				r_z1__20;
reg	[`BIT_WIDTH-1:0]				r_z1__21;
reg	[`BIT_WIDTH-1:0]				r_z1__22;
reg	[`BIT_WIDTH-1:0]				r_z1__23;
reg	[`BIT_WIDTH-1:0]				r_z1__24;
reg	[`BIT_WIDTH-1:0]				r_z1__25;
reg	[`BIT_WIDTH-1:0]				r_z1__26;
reg	[`BIT_WIDTH-1:0]				r_z1__27;
reg	[`BIT_WIDTH-1:0]				r_z1__28;
reg	[`BIT_WIDTH-1:0]				r_z1__29;
reg	[`BIT_WIDTH-1:0]				r_z1__30;
reg	[`BIT_WIDTH-1:0]				r_z1__31;
reg	[`BIT_WIDTH-1:0]				r_z1__32;
reg	[`BIT_WIDTH-1:0]				r_z1__33;
reg	[`BIT_WIDTH-1:0]				r_z1__34;
reg	[`BIT_WIDTH-1:0]				r_z1__35;
reg	[`BIT_WIDTH-1:0]				r_z1__36;
reg	[`BIT_WIDTH-1:0]				r_z1__37;
reg	[`BIT_WIDTH-1:0]				r_z1__38;
reg	[`BIT_WIDTH-1:0]				r_z1__39;
reg	[`BIT_WIDTH-1:0]				r_z1__40;
reg	[`BIT_WIDTH-1:0]				r_z1__41;
reg	[`BIT_WIDTH-1:0]				r_z1__42;
reg	[`BIT_WIDTH-1:0]				r_z1__43;
reg	[`BIT_WIDTH-1:0]				r_z1__44;
reg	[`BIT_WIDTH-1:0]				r_z1__45;
reg	[`BIT_WIDTH-1:0]				r_z1__46;
reg	[`BIT_WIDTH-1:0]				r_z1__47;
reg	[`BIT_WIDTH-1:0]				r_z1__48;
reg	[`BIT_WIDTH-1:0]				r_z1__49;
reg	[`BIT_WIDTH-1:0]				r_z1__50;
reg	[`BIT_WIDTH-1:0]				r_z1__51;
reg	[`BIT_WIDTH-1:0]				r_z1__52;
reg	[`BIT_WIDTH-1:0]				r_z1__53;
reg	[`BIT_WIDTH-1:0]				r_z1__54;
reg	[`BIT_WIDTH-1:0]				r_z1__55;
reg	[`BIT_WIDTH-1:0]				r_z1__56;
reg	[`BIT_WIDTH-1:0]				r_z1__57;
reg	[`BIT_WIDTH-1:0]				r_z1__58;
reg	[`BIT_WIDTH-1:0]				r_z1__59;

//reg	[`BIT_WIDTH-1:0]				r_z0[TOTAL_LATENCY - 1:0];

reg	[`BIT_WIDTH-1:0]				r_z0__0;
reg	[`BIT_WIDTH-1:0]				r_z0__1;
reg	[`BIT_WIDTH-1:0]				r_z0__2;
reg	[`BIT_WIDTH-1:0]				r_z0__3;
reg	[`BIT_WIDTH-1:0]				r_z0__4;
reg	[`BIT_WIDTH-1:0]				r_z0__5;
reg	[`BIT_WIDTH-1:0]				r_z0__6;
reg	[`BIT_WIDTH-1:0]				r_z0__7;
reg	[`BIT_WIDTH-1:0]				r_z0__8;
reg	[`BIT_WIDTH-1:0]				r_z0__9;
reg	[`BIT_WIDTH-1:0]				r_z0__10;
reg	[`BIT_WIDTH-1:0]				r_z0__11;
reg	[`BIT_WIDTH-1:0]				r_z0__12;
reg	[`BIT_WIDTH-1:0]				r_z0__13;
reg	[`BIT_WIDTH-1:0]				r_z0__14;
reg	[`BIT_WIDTH-1:0]				r_z0__15;
reg	[`BIT_WIDTH-1:0]				r_z0__16;
reg	[`BIT_WIDTH-1:0]				r_z0__17;
reg	[`BIT_WIDTH-1:0]				r_z0__18;
reg	[`BIT_WIDTH-1:0]				r_z0__19;
reg	[`BIT_WIDTH-1:0]				r_z0__20;
reg	[`BIT_WIDTH-1:0]				r_z0__21;
reg	[`BIT_WIDTH-1:0]				r_z0__22;
reg	[`BIT_WIDTH-1:0]				r_z0__23;
reg	[`BIT_WIDTH-1:0]				r_z0__24;
reg	[`BIT_WIDTH-1:0]				r_z0__25;
reg	[`BIT_WIDTH-1:0]				r_z0__26;
reg	[`BIT_WIDTH-1:0]				r_z0__27;
reg	[`BIT_WIDTH-1:0]				r_z0__28;
reg	[`BIT_WIDTH-1:0]				r_z0__29;
reg	[`BIT_WIDTH-1:0]				r_z0__30;
reg	[`BIT_WIDTH-1:0]				r_z0__31;
reg	[`BIT_WIDTH-1:0]				r_z0__32;
reg	[`BIT_WIDTH-1:0]				r_z0__33;
reg	[`BIT_WIDTH-1:0]				r_z0__34;
reg	[`BIT_WIDTH-1:0]				r_z0__35;
reg	[`BIT_WIDTH-1:0]				r_z0__36;
reg	[`BIT_WIDTH-1:0]				r_z0__37;
reg	[`BIT_WIDTH-1:0]				r_z0__38;
reg	[`BIT_WIDTH-1:0]				r_z0__39;
reg	[`BIT_WIDTH-1:0]				r_z0__40;
reg	[`BIT_WIDTH-1:0]				r_z0__41;
reg	[`BIT_WIDTH-1:0]				r_z0__42;
reg	[`BIT_WIDTH-1:0]				r_z0__43;
reg	[`BIT_WIDTH-1:0]				r_z0__44;
reg	[`BIT_WIDTH-1:0]				r_z0__45;
reg	[`BIT_WIDTH-1:0]				r_z0__46;
reg	[`BIT_WIDTH-1:0]				r_z0__47;
reg	[`BIT_WIDTH-1:0]				r_z0__48;
reg	[`BIT_WIDTH-1:0]				r_z0__49;
reg	[`BIT_WIDTH-1:0]				r_z0__50;
reg	[`BIT_WIDTH-1:0]				r_z0__51;
reg	[`BIT_WIDTH-1:0]				r_z0__52;
reg	[`BIT_WIDTH-1:0]				r_z0__53;
reg	[`BIT_WIDTH-1:0]				r_z0__54;
reg	[`BIT_WIDTH-1:0]				r_z0__55;
reg	[`BIT_WIDTH-1:0]				r_z0__56;
reg	[`BIT_WIDTH-1:0]				r_z0__57;
reg	[`BIT_WIDTH-1:0]				r_z0__58;
reg	[`BIT_WIDTH-1:0]				r_z0__59;
//reg	[`BIT_WIDTH-1:0]				r_mut[TOTAL_LATENCY - 1:0];

reg	[`BIT_WIDTH-1:0]				r_mut__0;
reg	[`BIT_WIDTH-1:0]				r_mut__1;
reg	[`BIT_WIDTH-1:0]				r_mut__2;
reg	[`BIT_WIDTH-1:0]				r_mut__3;
reg	[`BIT_WIDTH-1:0]				r_mut__4;
reg	[`BIT_WIDTH-1:0]				r_mut__5;
reg	[`BIT_WIDTH-1:0]				r_mut__6;
reg	[`BIT_WIDTH-1:0]				r_mut__7;
reg	[`BIT_WIDTH-1:0]				r_mut__8;
reg	[`BIT_WIDTH-1:0]				r_mut__9;
reg	[`BIT_WIDTH-1:0]				r_mut__10;
reg	[`BIT_WIDTH-1:0]				r_mut__11;
reg	[`BIT_WIDTH-1:0]				r_mut__12;
reg	[`BIT_WIDTH-1:0]				r_mut__13;
reg	[`BIT_WIDTH-1:0]				r_mut__14;
reg	[`BIT_WIDTH-1:0]				r_mut__15;
reg	[`BIT_WIDTH-1:0]				r_mut__16;
reg	[`BIT_WIDTH-1:0]				r_mut__17;
reg	[`BIT_WIDTH-1:0]				r_mut__18;
reg	[`BIT_WIDTH-1:0]				r_mut__19;
reg	[`BIT_WIDTH-1:0]				r_mut__20;
reg	[`BIT_WIDTH-1:0]				r_mut__21;
reg	[`BIT_WIDTH-1:0]				r_mut__22;
reg	[`BIT_WIDTH-1:0]				r_mut__23;
reg	[`BIT_WIDTH-1:0]				r_mut__24;
reg	[`BIT_WIDTH-1:0]				r_mut__25;
reg	[`BIT_WIDTH-1:0]				r_mut__26;
reg	[`BIT_WIDTH-1:0]				r_mut__27;
reg	[`BIT_WIDTH-1:0]				r_mut__28;
reg	[`BIT_WIDTH-1:0]				r_mut__29;
reg	[`BIT_WIDTH-1:0]				r_mut__30;
reg	[`BIT_WIDTH-1:0]				r_mut__31;
reg	[`BIT_WIDTH-1:0]				r_mut__32;
reg	[`BIT_WIDTH-1:0]				r_mut__33;
reg	[`BIT_WIDTH-1:0]				r_mut__34;
reg	[`BIT_WIDTH-1:0]				r_mut__35;
reg	[`BIT_WIDTH-1:0]				r_mut__36;
reg	[`BIT_WIDTH-1:0]				r_mut__37;
reg	[`BIT_WIDTH-1:0]				r_mut__38;
reg	[`BIT_WIDTH-1:0]				r_mut__39;
reg	[`BIT_WIDTH-1:0]				r_mut__40;
reg	[`BIT_WIDTH-1:0]				r_mut__41;
reg	[`BIT_WIDTH-1:0]				r_mut__42;
reg	[`BIT_WIDTH-1:0]				r_mut__43;
reg	[`BIT_WIDTH-1:0]				r_mut__44;
reg	[`BIT_WIDTH-1:0]				r_mut__45;
reg	[`BIT_WIDTH-1:0]				r_mut__46;
reg	[`BIT_WIDTH-1:0]				r_mut__47;
reg	[`BIT_WIDTH-1:0]				r_mut__48;
reg	[`BIT_WIDTH-1:0]				r_mut__49;
reg	[`BIT_WIDTH-1:0]				r_mut__50;
reg	[`BIT_WIDTH-1:0]				r_mut__51;
reg	[`BIT_WIDTH-1:0]				r_mut__52;
reg	[`BIT_WIDTH-1:0]				r_mut__53;
reg	[`BIT_WIDTH-1:0]				r_mut__54;
reg	[`BIT_WIDTH-1:0]				r_mut__55;
reg	[`BIT_WIDTH-1:0]				r_mut__56;
reg	[`BIT_WIDTH-1:0]				r_mut__57;
reg	[`BIT_WIDTH-1:0]				r_mut__58;
reg	[`BIT_WIDTH-1:0]				r_mut__59;

wire	[2*`BIT_WIDTH-1:0]			sleftz_big;
wire	[2*`BIT_WIDTH-1:0]			sleftr_big;
wire	[2*`BIT_WIDTH-1:0]			sr_big;
wire	[`BIT_WIDTH-1:0]			remainder_div1;
wire	[2*`BIT_WIDTH-1:0]			quotient_div1;

//ASSIGNMENTS FROM PIPE TO OUTPUT
assign x_boundaryChecker = r_x__59;
assign y_boundaryChecker = r_y__59;
assign z_boundaryChecker = r_z__59;
assign ux_boundaryChecker = r_ux__59;
assign uy_boundaryChecker = r_uy__59;
assign uz_boundaryChecker = r_uz__59;
assign sz_boundaryChecker = r_sz__59;
assign sr_boundaryChecker = r_sr__59;
assign sleftz_boundaryChecker = r_sleftz__59;
assign sleftr_boundaryChecker = r_sleftr__59;
assign weight_boundaryChecker = r_weight__59;
assign layer_boundaryChecker = r_layer__59;
assign dead_boundaryChecker = r_dead__59;
assign hit_boundaryChecker = r_hit__59;

// divider
signed_div_30 divide_u1 (
	.clock(clock),
	.denom(c_uz__0),
	.numer(c_numer__0),
	.quotient(quotient_div1),
	.remain(remainder_div1));

// multipliers
mult_signed_32_bc mult_u1(
	.clock(clock),
	.dataa(c_diff__30),
	.datab(c_mut__30),
	.result(sleftz_big));

mult_signed_32_bc mult_u2(
	.clock(clock),
	.dataa(maxDepth_over_maxRadius),
	.datab(c_sleftz__31),
	.result(sleftr_big));

mult_signed_32_bc mult_u3(
	.clock(clock),
	.dataa(maxDepth_over_maxRadius),
	.datab(c_dl_b__30),
	.result(sr_big));

// multiplexor to find z1 and z0
always @(c_layer__0 or z1_0 or z0_0 or mut_0 or
					z1_1 or z0_1 or mut_1 or
					z1_2 or z0_2 or mut_2 or
					z1_3 or z0_3 or mut_3 or
					z1_4 or z0_4 or mut_4 or
					z1_5 or z0_5 or mut_5)
begin
	case(c_layer__0)
		3'b000:
		begin
			c_z1__0 = z1_0;
			c_z0__0 = z0_0;
			c_mut__0 = mut_0;
		end
		3'b001:
		begin
			c_z1__0 = z1_1;
			c_z0__0 = z0_1;
			c_mut__0 = mut_1;
		end
		3'b010:
		begin
			c_z1__0 = z1_2;
			c_z0__0 = z0_2;
			c_mut__0 = mut_2;
		end
		3'b011:
		begin
			c_z1__0 = z1_3;
			c_z0__0 = z0_3;
			c_mut__0 = mut_3;
		end
		3'b100:
		begin
			c_z1__0 = z1_4;
			c_z0__0 = z0_4;
			c_mut__0 = mut_4;
		end
		3'b101:
		begin
			c_z1__0 = z1_5;
			c_z0__0 = z0_5;
			c_mut__0 = mut_5;
		end
		default:
		begin
			c_z1__0 = 0;
			c_z0__0 = 0;
			c_mut__0 = 0;
		end
	endcase
end

// May have to change block staments on this one for odin
// set numerator
always @(c_uz__0 or c_z1__0 or c_z__0 or c_z0__0)
begin
	//c_numer__0 = 63'b0;
	if(c_uz__0[31] == 1'b0)
	begin
		c_numer__0[63:32] = c_z1__0 - c_z__0;
		c_numer__0[31:0] = 32'b0;
	end
	else if(c_uz__0[31] == 1'b1)
	begin
		c_numer__0[63:32] = c_z0__0 - c_z__0;
		c_numer__0[31:0] = 32'b0;
	end
	else
	begin
		c_numer__0 = 63'b0;
	end
end

// initialize uninitialized data in pipeline
always @(x_mover or y_mover or z_mover or 
		ux_mover or uy_mover or uz_mover or
		sz_mover or sr_mover or sleftz_mover or sleftr_mover or
		weight_mover or layer_mover or dead_mover)
begin
	c_x__0 = x_mover;
	c_y__0 = y_mover;
	c_z__0 = z_mover;
	c_ux__0 = ux_mover;
	c_uy__0 = uy_mover;
	c_uz__0 = uz_mover;
	c_sz__0 = sz_mover;
	c_sr__0 = sr_mover;
	c_sleftz__0 = sleftz_mover;
	c_sleftr__0 = sleftr_mover;
	c_weight__0 = weight_mover;
	c_layer__0 = layer_mover;
	c_dead__0 = dead_mover;
	c_hit__0 = 1'b0;
	c_diff__0 = 32'b0;
	c_dl_b__0 = 32'b0;
end

// Determine new (x,y,z) coordinates
always @(r_x__0 or r_y__0 or r_z__0 or r_ux__0 or r_uy__0 or r_uz__0 or r_sz__0 or r_sr__0 or r_sleftz__0 or r_sleftr__0 or
		r_weight__0 or r_layer__0 or r_dead__0 or r_hit__0 or r_diff__0 or r_dl_b__0 or r_numer__0 or r_z1__0 or r_z0__0 or r_mut__0 or

		r_x__1 or r_y__1 or r_z__1 or r_ux__1 or r_uy__1 or r_uz__1 or r_sz__1 or r_sr__1 or r_sleftz__1 or r_sleftr__1 or
		r_weight__1 or r_layer__1 or r_dead__1 or r_hit__1 or r_diff__1 or r_dl_b__1 or r_numer__1 or r_z1__1 or r_z0__1 or r_mut__1 or

		r_x__2 or r_y__2 or r_z__2 or r_ux__2 or r_uy__2 or r_uz__2 or r_sz__2 or r_sr__2 or r_sleftz__2 or r_sleftr__2 or
		r_weight__2 or r_layer__2 or r_dead__2 or r_hit__2 or r_diff__2 or r_dl_b__2 or r_numer__2 or r_z1__2 or r_z0__2 or r_mut__2 or

		r_x__3 or r_y__3 or r_z__3 or r_ux__3 or r_uy__3 or r_uz__3 or r_sz__3 or r_sr__3 or r_sleftz__3 or r_sleftr__3 or
		r_weight__3 or r_layer__3 or r_dead__3 or r_hit__3 or r_diff__3 or r_dl_b__3 or r_numer__3 or r_z1__3 or r_z0__3 or r_mut__3 or

		r_x__4 or r_y__4 or r_z__4 or r_ux__4 or r_uy__4 or r_uz__4 or r_sz__4 or r_sr__4 or r_sleftz__4 or r_sleftr__4 or
		r_weight__4 or r_layer__4 or r_dead__4 or r_hit__4 or r_diff__4 or r_dl_b__4 or r_numer__4 or r_z1__4 or r_z0__4 or r_mut__4 or

		r_x__5 or r_y__5 or r_z__5 or r_ux__5 or r_uy__5 or r_uz__5 or r_sz__5 or r_sr__5 or r_sleftz__5 or r_sleftr__5 or
		r_weight__5 or r_layer__5 or r_dead__5 or r_hit__5 or r_diff__5 or r_dl_b__5 or r_numer__5 or r_z1__5 or r_z0__5 or r_mut__5 or

		r_x__6 or r_y__6 or r_z__6 or r_ux__6 or r_uy__6 or r_uz__6 or r_sz__6 or r_sr__6 or r_sleftz__6 or r_sleftr__6 or
		r_weight__6 or r_layer__6 or r_dead__6 or r_hit__6 or r_diff__6 or r_dl_b__6 or r_numer__6 or r_z1__6 or r_z0__6 or r_mut__6 or

		r_x__7 or r_y__7 or r_z__7 or r_ux__7 or r_uy__7 or r_uz__7 or r_sz__7 or r_sr__7 or r_sleftz__7 or r_sleftr__7 or
		r_weight__7 or r_layer__7 or r_dead__7 or r_hit__7 or r_diff__7 or r_dl_b__7 or r_numer__7 or r_z1__7 or r_z0__7 or r_mut__7 or

		r_x__8 or r_y__8 or r_z__8 or r_ux__8 or r_uy__8 or r_uz__8 or r_sz__8 or r_sr__8 or r_sleftz__8 or r_sleftr__8 or
		r_weight__8 or r_layer__8 or r_dead__8 or r_hit__8 or r_diff__8 or r_dl_b__8 or r_numer__8 or r_z1__8 or r_z0__8 or r_mut__8 or

		r_x__9 or r_y__9 or r_z__9 or r_ux__9 or r_uy__9 or r_uz__9 or r_sz__9 or r_sr__9 or r_sleftz__9 or r_sleftr__9 or
		r_weight__9 or r_layer__9 or r_dead__9 or r_hit__9 or r_diff__9 or r_dl_b__9 or r_numer__9 or r_z1__9 or r_z0__9 or r_mut__9 or

		r_x__10 or r_y__10 or r_z__10 or r_ux__10 or r_uy__10 or r_uz__10 or r_sz__10 or r_sr__10 or r_sleftz__10 or r_sleftr__10 or
		r_weight__10 or r_layer__10 or r_dead__10 or r_hit__10 or r_diff__10 or r_dl_b__10 or r_numer__10 or r_z1__10 or r_z0__10 or r_mut__10 or

		r_x__11 or r_y__11 or r_z__11 or r_ux__11 or r_uy__11 or r_uz__11 or r_sz__11 or r_sr__11 or r_sleftz__11 or r_sleftr__11 or
		r_weight__11 or r_layer__11 or r_dead__11 or r_hit__11 or r_diff__11 or r_dl_b__11 or r_numer__11 or r_z1__11 or r_z0__11 or r_mut__11 or

		r_x__12 or r_y__12 or r_z__12 or r_ux__12 or r_uy__12 or r_uz__12 or r_sz__12 or r_sr__12 or r_sleftz__12 or r_sleftr__12 or
		r_weight__12 or r_layer__12 or r_dead__12 or r_hit__12 or r_diff__12 or r_dl_b__12 or r_numer__12 or r_z1__12 or r_z0__12 or r_mut__12 or

		r_x__13 or r_y__13 or r_z__13 or r_ux__13 or r_uy__13 or r_uz__13 or r_sz__13 or r_sr__13 or r_sleftz__13 or r_sleftr__13 or
		r_weight__13 or r_layer__13 or r_dead__13 or r_hit__13 or r_diff__13 or r_dl_b__13 or r_numer__13 or r_z1__13 or r_z0__13 or r_mut__13 or

		r_x__14 or r_y__14 or r_z__14 or r_ux__14 or r_uy__14 or r_uz__14 or r_sz__14 or r_sr__14 or r_sleftz__14 or r_sleftr__14 or
		r_weight__14 or r_layer__14 or r_dead__14 or r_hit__14 or r_diff__14 or r_dl_b__14 or r_numer__14 or r_z1__14 or r_z0__14 or r_mut__14 or

		r_x__15 or r_y__15 or r_z__15 or r_ux__15 or r_uy__15 or r_uz__15 or r_sz__15 or r_sr__15 or r_sleftz__15 or r_sleftr__15 or
		r_weight__15 or r_layer__15 or r_dead__15 or r_hit__15 or r_diff__15 or r_dl_b__15 or r_numer__15 or r_z1__15 or r_z0__15 or r_mut__15 or

		r_x__16 or r_y__16 or r_z__16 or r_ux__16 or r_uy__16 or r_uz__16 or r_sz__16 or r_sr__16 or r_sleftz__16 or r_sleftr__16 or
		r_weight__16 or r_layer__16 or r_dead__16 or r_hit__16 or r_diff__16 or r_dl_b__16 or r_numer__16 or r_z1__16 or r_z0__16 or r_mut__16 or

		r_x__17 or r_y__17 or r_z__17 or r_ux__17 or r_uy__17 or r_uz__17 or r_sz__17 or r_sr__17 or r_sleftz__17 or r_sleftr__17 or
		r_weight__17 or r_layer__17 or r_dead__17 or r_hit__17 or r_diff__17 or r_dl_b__17 or r_numer__17 or r_z1__17 or r_z0__17 or r_mut__17 or

		r_x__18 or r_y__18 or r_z__18 or r_ux__18 or r_uy__18 or r_uz__18 or r_sz__18 or r_sr__18 or r_sleftz__18 or r_sleftr__18 or
		r_weight__18 or r_layer__18 or r_dead__18 or r_hit__18 or r_diff__18 or r_dl_b__18 or r_numer__18 or r_z1__18 or r_z0__18 or r_mut__18 or

		r_x__19 or r_y__19 or r_z__19 or r_ux__19 or r_uy__19 or r_uz__19 or r_sz__19 or r_sr__19 or r_sleftz__19 or r_sleftr__19 or
		r_weight__19 or r_layer__19 or r_dead__19 or r_hit__19 or r_diff__19 or r_dl_b__19 or r_numer__19 or r_z1__19 or r_z0__19 or r_mut__19 or

		r_x__20 or r_y__20 or r_z__20 or r_ux__20 or r_uy__20 or r_uz__20 or r_sz__20 or r_sr__20 or r_sleftz__20 or r_sleftr__20 or
		r_weight__20 or r_layer__20 or r_dead__20 or r_hit__20 or r_diff__20 or r_dl_b__20 or r_numer__20 or r_z1__20 or r_z0__20 or r_mut__20 or

		r_x__21 or r_y__21 or r_z__21 or r_ux__21 or r_uy__21 or r_uz__21 or r_sz__21 or r_sr__21 or r_sleftz__21 or r_sleftr__21 or
		r_weight__21 or r_layer__21 or r_dead__21 or r_hit__21 or r_diff__21 or r_dl_b__21 or r_numer__21 or r_z1__21 or r_z0__21 or r_mut__21 or

		r_x__22 or r_y__22 or r_z__22 or r_ux__22 or r_uy__22 or r_uz__22 or r_sz__22 or r_sr__22 or r_sleftz__22 or r_sleftr__22 or
		r_weight__22 or r_layer__22 or r_dead__22 or r_hit__22 or r_diff__22 or r_dl_b__22 or r_numer__22 or r_z1__22 or r_z0__22 or r_mut__22 or

		r_x__23 or r_y__23 or r_z__23 or r_ux__23 or r_uy__23 or r_uz__23 or r_sz__23 or r_sr__23 or r_sleftz__23 or r_sleftr__23 or
		r_weight__23 or r_layer__23 or r_dead__23 or r_hit__23 or r_diff__23 or r_dl_b__23 or r_numer__23 or r_z1__23 or r_z0__23 or r_mut__23 or

		r_x__24 or r_y__24 or r_z__24 or r_ux__24 or r_uy__24 or r_uz__24 or r_sz__24 or r_sr__24 or r_sleftz__24 or r_sleftr__24 or
		r_weight__24 or r_layer__24 or r_dead__24 or r_hit__24 or r_diff__24 or r_dl_b__24 or r_numer__24 or r_z1__24 or r_z0__24 or r_mut__24 or

		r_x__25 or r_y__25 or r_z__25 or r_ux__25 or r_uy__25 or r_uz__25 or r_sz__25 or r_sr__25 or r_sleftz__25 or r_sleftr__25 or
		r_weight__25 or r_layer__25 or r_dead__25 or r_hit__25 or r_diff__25 or r_dl_b__25 or r_numer__25 or r_z1__25 or r_z0__25 or r_mut__25 or

		r_x__26 or r_y__26 or r_z__26 or r_ux__26 or r_uy__26 or r_uz__26 or r_sz__26 or r_sr__26 or r_sleftz__26 or r_sleftr__26 or
		r_weight__26 or r_layer__26 or r_dead__26 or r_hit__26 or r_diff__26 or r_dl_b__26 or r_numer__26 or r_z1__26 or r_z0__26 or r_mut__26 or

		r_x__27 or r_y__27 or r_z__27 or r_ux__27 or r_uy__27 or r_uz__27 or r_sz__27 or r_sr__27 or r_sleftz__27 or r_sleftr__27 or
		r_weight__27 or r_layer__27 or r_dead__27 or r_hit__27 or r_diff__27 or r_dl_b__27 or r_numer__27 or r_z1__27 or r_z0__27 or r_mut__27 or

		r_x__28 or r_y__28 or r_z__28 or r_ux__28 or r_uy__28 or r_uz__28 or r_sz__28 or r_sr__28 or r_sleftz__28 or r_sleftr__28 or
		r_weight__28 or r_layer__28 or r_dead__28 or r_hit__28 or r_diff__28 or r_dl_b__28 or r_numer__28 or r_z1__28 or r_z0__28 or r_mut__28 or

		r_x__29 or r_y__29 or r_z__29 or r_ux__29 or r_uy__29 or r_uz__29 or r_sz__29 or r_sr__29 or r_sleftz__29 or r_sleftr__29 or
		r_weight__29 or r_layer__29 or r_dead__29 or r_hit__29 or r_diff__29 or r_dl_b__29 or r_numer__29 or r_z1__29 or r_z0__29 or r_mut__29 or

		r_x__30 or r_y__30 or r_z__30 or r_ux__30 or r_uy__30 or r_uz__30 or r_sz__30 or r_sr__30 or r_sleftz__30 or r_sleftr__30 or
		r_weight__30 or r_layer__30 or r_dead__30 or r_hit__30 or r_diff__30 or r_dl_b__30 or r_numer__30 or r_z1__30 or r_z0__30 or r_mut__30 or

		r_x__31 or r_y__31 or r_z__31 or r_ux__31 or r_uy__31 or r_uz__31 or r_sz__31 or r_sr__31 or r_sleftz__31 or r_sleftr__31 or
		r_weight__31 or r_layer__31 or r_dead__31 or r_hit__31 or r_diff__31 or r_dl_b__31 or r_numer__31 or r_z1__31 or r_z0__31 or r_mut__31 or

		r_x__32 or r_y__32 or r_z__32 or r_ux__32 or r_uy__32 or r_uz__32 or r_sz__32 or r_sr__32 or r_sleftz__32 or r_sleftr__32 or
		r_weight__32 or r_layer__32 or r_dead__32 or r_hit__32 or r_diff__32 or r_dl_b__32 or r_numer__32 or r_z1__32 or r_z0__32 or r_mut__32 or

		r_x__33 or r_y__33 or r_z__33 or r_ux__33 or r_uy__33 or r_uz__33 or r_sz__33 or r_sr__33 or r_sleftz__33 or r_sleftr__33 or
		r_weight__33 or r_layer__33 or r_dead__33 or r_hit__33 or r_diff__33 or r_dl_b__33 or r_numer__33 or r_z1__33 or r_z0__33 or r_mut__33 or

		r_x__34 or r_y__34 or r_z__34 or r_ux__34 or r_uy__34 or r_uz__34 or r_sz__34 or r_sr__34 or r_sleftz__34 or r_sleftr__34 or
		r_weight__34 or r_layer__34 or r_dead__34 or r_hit__34 or r_diff__34 or r_dl_b__34 or r_numer__34 or r_z1__34 or r_z0__34 or r_mut__34 or

		r_x__35 or r_y__35 or r_z__35 or r_ux__35 or r_uy__35 or r_uz__35 or r_sz__35 or r_sr__35 or r_sleftz__35 or r_sleftr__35 or
		r_weight__35 or r_layer__35 or r_dead__35 or r_hit__35 or r_diff__35 or r_dl_b__35 or r_numer__35 or r_z1__35 or r_z0__35 or r_mut__35 or

		r_x__36 or r_y__36 or r_z__36 or r_ux__36 or r_uy__36 or r_uz__36 or r_sz__36 or r_sr__36 or r_sleftz__36 or r_sleftr__36 or
		r_weight__36 or r_layer__36 or r_dead__36 or r_hit__36 or r_diff__36 or r_dl_b__36 or r_numer__36 or r_z1__36 or r_z0__36 or r_mut__36 or

		r_x__37 or r_y__37 or r_z__37 or r_ux__37 or r_uy__37 or r_uz__37 or r_sz__37 or r_sr__37 or r_sleftz__37 or r_sleftr__37 or
		r_weight__37 or r_layer__37 or r_dead__37 or r_hit__37 or r_diff__37 or r_dl_b__37 or r_numer__37 or r_z1__37 or r_z0__37 or r_mut__37 or

		r_x__38 or r_y__38 or r_z__38 or r_ux__38 or r_uy__38 or r_uz__38 or r_sz__38 or r_sr__38 or r_sleftz__38 or r_sleftr__38 or
		r_weight__38 or r_layer__38 or r_dead__38 or r_hit__38 or r_diff__38 or r_dl_b__38 or r_numer__38 or r_z1__38 or r_z0__38 or r_mut__38 or

		r_x__39 or r_y__39 or r_z__39 or r_ux__39 or r_uy__39 or r_uz__39 or r_sz__39 or r_sr__39 or r_sleftz__39 or r_sleftr__39 or
		r_weight__39 or r_layer__39 or r_dead__39 or r_hit__39 or r_diff__39 or r_dl_b__39 or r_numer__39 or r_z1__39 or r_z0__39 or r_mut__39 or

		r_x__40 or r_y__40 or r_z__40 or r_ux__40 or r_uy__40 or r_uz__40 or r_sz__40 or r_sr__40 or r_sleftz__40 or r_sleftr__40 or
		r_weight__40 or r_layer__40 or r_dead__40 or r_hit__40 or r_diff__40 or r_dl_b__40 or r_numer__40 or r_z1__40 or r_z0__40 or r_mut__40 or

		r_x__41 or r_y__41 or r_z__41 or r_ux__41 or r_uy__41 or r_uz__41 or r_sz__41 or r_sr__41 or r_sleftz__41 or r_sleftr__41 or
		r_weight__41 or r_layer__41 or r_dead__41 or r_hit__41 or r_diff__41 or r_dl_b__41 or r_numer__41 or r_z1__41 or r_z0__41 or r_mut__41 or

		r_x__42 or r_y__42 or r_z__42 or r_ux__42 or r_uy__42 or r_uz__42 or r_sz__42 or r_sr__42 or r_sleftz__42 or r_sleftr__42 or
		r_weight__42 or r_layer__42 or r_dead__42 or r_hit__42 or r_diff__42 or r_dl_b__42 or r_numer__42 or r_z1__42 or r_z0__42 or r_mut__42 or

		r_x__43 or r_y__43 or r_z__43 or r_ux__43 or r_uy__43 or r_uz__43 or r_sz__43 or r_sr__43 or r_sleftz__43 or r_sleftr__43 or
		r_weight__43 or r_layer__43 or r_dead__43 or r_hit__43 or r_diff__43 or r_dl_b__43 or r_numer__43 or r_z1__43 or r_z0__43 or r_mut__43 or

		r_x__44 or r_y__44 or r_z__44 or r_ux__44 or r_uy__44 or r_uz__44 or r_sz__44 or r_sr__44 or r_sleftz__44 or r_sleftr__44 or
		r_weight__44 or r_layer__44 or r_dead__44 or r_hit__44 or r_diff__44 or r_dl_b__44 or r_numer__44 or r_z1__44 or r_z0__44 or r_mut__44 or

		r_x__45 or r_y__45 or r_z__45 or r_ux__45 or r_uy__45 or r_uz__45 or r_sz__45 or r_sr__45 or r_sleftz__45 or r_sleftr__45 or
		r_weight__45 or r_layer__45 or r_dead__45 or r_hit__45 or r_diff__45 or r_dl_b__45 or r_numer__45 or r_z1__45 or r_z0__45 or r_mut__45 or

		r_x__46 or r_y__46 or r_z__46 or r_ux__46 or r_uy__46 or r_uz__46 or r_sz__46 or r_sr__46 or r_sleftz__46 or r_sleftr__46 or
		r_weight__46 or r_layer__46 or r_dead__46 or r_hit__46 or r_diff__46 or r_dl_b__46 or r_numer__46 or r_z1__46 or r_z0__46 or r_mut__46 or

		r_x__47 or r_y__47 or r_z__47 or r_ux__47 or r_uy__47 or r_uz__47 or r_sz__47 or r_sr__47 or r_sleftz__47 or r_sleftr__47 or
		r_weight__47 or r_layer__47 or r_dead__47 or r_hit__47 or r_diff__47 or r_dl_b__47 or r_numer__47 or r_z1__47 or r_z0__47 or r_mut__47 or

		r_x__48 or r_y__48 or r_z__48 or r_ux__48 or r_uy__48 or r_uz__48 or r_sz__48 or r_sr__48 or r_sleftz__48 or r_sleftr__48 or
		r_weight__48 or r_layer__48 or r_dead__48 or r_hit__48 or r_diff__48 or r_dl_b__48 or r_numer__48 or r_z1__48 or r_z0__48 or r_mut__48 or

		r_x__49 or r_y__49 or r_z__49 or r_ux__49 or r_uy__49 or r_uz__49 or r_sz__49 or r_sr__49 or r_sleftz__49 or r_sleftr__49 or
		r_weight__49 or r_layer__49 or r_dead__49 or r_hit__49 or r_diff__49 or r_dl_b__49 or r_numer__49 or r_z1__49 or r_z0__49 or r_mut__49 or

		r_x__50 or r_y__50 or r_z__50 or r_ux__50 or r_uy__50 or r_uz__50 or r_sz__50 or r_sr__50 or r_sleftz__50 or r_sleftr__50 or
		r_weight__50 or r_layer__50 or r_dead__50 or r_hit__50 or r_diff__50 or r_dl_b__50 or r_numer__50 or r_z1__50 or r_z0__50 or r_mut__50 or

		r_x__51 or r_y__51 or r_z__51 or r_ux__51 or r_uy__51 or r_uz__51 or r_sz__51 or r_sr__51 or r_sleftz__51 or r_sleftr__51 or
		r_weight__51 or r_layer__51 or r_dead__51 or r_hit__51 or r_diff__51 or r_dl_b__51 or r_numer__51 or r_z1__51 or r_z0__51 or r_mut__51 or

		r_x__52 or r_y__52 or r_z__52 or r_ux__52 or r_uy__52 or r_uz__52 or r_sz__52 or r_sr__52 or r_sleftz__52 or r_sleftr__52 or
		r_weight__52 or r_layer__52 or r_dead__52 or r_hit__52 or r_diff__52 or r_dl_b__52 or r_numer__52 or r_z1__52 or r_z0__52 or r_mut__52 or

		r_x__53 or r_y__53 or r_z__53 or r_ux__53 or r_uy__53 or r_uz__53 or r_sz__53 or r_sr__53 or r_sleftz__53 or r_sleftr__53 or
		r_weight__53 or r_layer__53 or r_dead__53 or r_hit__53 or r_diff__53 or r_dl_b__53 or r_numer__53 or r_z1__53 or r_z0__53 or r_mut__53 or

		r_x__54 or r_y__54 or r_z__54 or r_ux__54 or r_uy__54 or r_uz__54 or r_sz__54 or r_sr__54 or r_sleftz__54 or r_sleftr__54 or
		r_weight__54 or r_layer__54 or r_dead__54 or r_hit__54 or r_diff__54 or r_dl_b__54 or r_numer__54 or r_z1__54 or r_z0__54 or r_mut__54 or

		r_x__55 or r_y__55 or r_z__55 or r_ux__55 or r_uy__55 or r_uz__55 or r_sz__55 or r_sr__55 or r_sleftz__55 or r_sleftr__55 or
		r_weight__55 or r_layer__55 or r_dead__55 or r_hit__55 or r_diff__55 or r_dl_b__55 or r_numer__55 or r_z1__55 or r_z0__55 or r_mut__55 or

		r_x__56 or r_y__56 or r_z__56 or r_ux__56 or r_uy__56 or r_uz__56 or r_sz__56 or r_sr__56 or r_sleftz__56 or r_sleftr__56 or
		r_weight__56 or r_layer__56 or r_dead__56 or r_hit__56 or r_diff__56 or r_dl_b__56 or r_numer__56 or r_z1__56 or r_z0__56 or r_mut__56 or

		r_x__57 or r_y__57 or r_z__57 or r_ux__57 or r_uy__57 or r_uz__57 or r_sz__57 or r_sr__57 or r_sleftz__57 or r_sleftr__57 or
		r_weight__57 or r_layer__57 or r_dead__57 or r_hit__57 or r_diff__57 or r_dl_b__57 or r_numer__57 or r_z1__57 or r_z0__57 or r_mut__57 or

		r_x__58 or r_y__58 or r_z__58 or r_ux__58 or r_uy__58 or r_uz__58 or r_sz__58 or r_sr__58 or r_sleftz__58 or r_sleftr__58 or
		r_weight__58 or r_layer__58 or r_dead__58 or r_hit__58 or r_diff__58 or r_dl_b__58 or r_numer__58 or r_z1__58 or r_z0__58 or r_mut__58 or

		r_x__59 or r_y__59 or r_z__59 or r_ux__59 or r_uy__59 or r_uz__59 or r_sz__59 or r_sr__59 or r_sleftz__59 or r_sleftr__59 or
		r_weight__59 or r_layer__59 or r_dead__59 or r_hit__59 or r_diff__59 or r_dl_b__59 or r_numer__59 or r_z1__59 or r_z0__59 or r_mut__59 or 
		
		sr_big or sleftz_big or sleftr_big or quotient_div1)

	// default
	// setup standard pipeline
//	for(i = 1; i < `TOTAL_LATENCY; i = i + 1)
//	begin
//		c_x[i]	= r_x[i-1];
//		c_y[i]	= r_y[i-1];
//		c_z[i]	= r_z[i-1];
//		c_ux[i]	= r_ux[i-1];
//		c_uy[i]	= r_uy[i-1];
//		c_uz[i]	= r_uz[i-1];
//		c_sz[i]	= r_sz[i-1];
//		c_sr[i]	= r_sr[i-1];
//		c_sleftz[i]	= r_sleftz[i-1];
//		c_sleftr[i]	= r_sleftr[i-1];
//		c_weight[i]	= r_weight[i-1];
//		c_layer[i]	= r_layer[i-1];
//		c_dead[i]	= r_dead[i-1];
//		c_hit[i]	= r_hit[i-1];
//		c_diff[i] = r_diff[i-1];
//		c_dl_b[i] = r_dl_b[i-1];
//		c_numer[i] = r_numer[i-1];
//		c_z1[i] = r_z1[i-1];
//		c_z0[i] = r_z0[i-1];
//		c_mut[i] = r_mut[i-1];
//	end

begin
	//Instatiate all 60 instances of the above for-loop
	//for 1
		c_x__1	= r_x__0;
		c_y__1	= r_y__0;
		c_z__1	= r_z__0;
		c_ux__1	= r_ux__0;
		c_uy__1	= r_uy__0;
		c_uz__1	= r_uz__0;
		c_sz__1	= r_sz__0;
		c_sr__1	= r_sr__0;
		c_sleftz__1	= r_sleftz__0;
		c_sleftr__1	= r_sleftr__0;
		c_weight__1	= r_weight__0;
		c_layer__1	= r_layer__0;
		c_dead__1	= r_dead__0;
		c_hit__1	= r_hit__0;
		c_diff__1 = r_diff__0;
		c_dl_b__1 = r_dl_b__0;
		c_numer__1 = r_numer__0;
		c_z1__1 = r_z1__0;
		c_z0__1 = r_z0__0;
		c_mut__1 = r_mut__0;
		
		//for 2
		c_x__2	= r_x__1;
		c_y__2	= r_y__1;
		c_z__2	= r_z__1;
		c_ux__2	= r_ux__1;
		c_uy__2	= r_uy__1;
		c_uz__2	= r_uz__1;
		c_sz__2	= r_sz__1;
		c_sr__2	= r_sr__1;
		c_sleftz__2	= r_sleftz__1;
		c_sleftr__2	= r_sleftr__1;
		c_weight__2	= r_weight__1;
		c_layer__2	= r_layer__1;
		c_dead__2	= r_dead__1;
		c_hit__2	= r_hit__1;
		c_diff__2 = r_diff__1;
		c_dl_b__2 = r_dl_b__1;
		c_numer__2 = r_numer__1;
		c_z1__2 = r_z1__1;
		c_z0__2 = r_z0__1;
		c_mut__2 = r_mut__1;
	
		//for 3
		c_x__3	= r_x__2;
		c_y__3	= r_y__2;
		c_z__3	= r_z__2;
		c_ux__3	= r_ux__2;
		c_uy__3	= r_uy__2;
		c_uz__3	= r_uz__2;
		c_sz__3	= r_sz__2;
		c_sr__3	= r_sr__2;
		c_sleftz__3	= r_sleftz__2;
		c_sleftr__3	= r_sleftr__2;
		c_weight__3	= r_weight__2;
		c_layer__3	= r_layer__2;
		c_dead__3	= r_dead__2;
		c_hit__3	= r_hit__2;
		c_diff__3 = r_diff__2;
		c_dl_b__3 = r_dl_b__2;
		c_numer__3 = r_numer__2;
		c_z1__3 = r_z1__2;
		c_z0__3 = r_z0__2;
		c_mut__3 = r_mut__2;
		
		//for 4
		c_x__4	= r_x__3;
		c_y__4	= r_y__3;
		c_z__4	= r_z__3;
		c_ux__4	= r_ux__3;
		c_uy__4	= r_uy__3;
		c_uz__4	= r_uz__3;
		c_sz__4	= r_sz__3;
		c_sr__4	= r_sr__3;
		c_sleftz__4	= r_sleftz__3;
		c_sleftr__4	= r_sleftr__3;
		c_weight__4	= r_weight__3;
		c_layer__4	= r_layer__3;
		c_dead__4	= r_dead__3;
		c_hit__4	= r_hit__3;
		c_diff__4 = r_diff__3;
		c_dl_b__4 = r_dl_b__3;
		c_numer__4 = r_numer__3;
		c_z1__4 = r_z1__3;
		c_z0__4 = r_z0__3;
		c_mut__4 = r_mut__3;
		
		//for 5
		c_x__5	= r_x__4;
		c_y__5	= r_y__4;
		c_z__5	= r_z__4;
		c_ux__5	= r_ux__4;
		c_uy__5	= r_uy__4;
		c_uz__5	= r_uz__4;
		c_sz__5	= r_sz__4;
		c_sr__5	= r_sr__4;
		c_sleftz__5	= r_sleftz__4;
		c_sleftr__5	= r_sleftr__4;
		c_weight__5	= r_weight__4;
		c_layer__5	= r_layer__4;
		c_dead__5	= r_dead__4;
		c_hit__5	= r_hit__4;
		c_diff__5 = r_diff__4;
		c_dl_b__5 = r_dl_b__4;
		c_numer__5 = r_numer__4;
		c_z1__5 = r_z1__4;
		c_z0__5 = r_z0__4;
		c_mut__5 = r_mut__4;
		
		//for 6
		c_x__6	= r_x__5;
		c_y__6	= r_y__5;
		c_z__6	= r_z__5;
		c_ux__6	= r_ux__5;
		c_uy__6	= r_uy__5;
		c_uz__6	= r_uz__5;
		c_sz__6	= r_sz__5;
		c_sr__6	= r_sr__5;
		c_sleftz__6	= r_sleftz__5;
		c_sleftr__6	= r_sleftr__5;
		c_weight__6	= r_weight__5;
		c_layer__6	= r_layer__5;
		c_dead__6	= r_dead__5;
		c_hit__6	= r_hit__5;
		c_diff__6 = r_diff__5;
		c_dl_b__6 = r_dl_b__5;
		c_numer__6 = r_numer__5;
		c_z1__6 = r_z1__5;
		c_z0__6 = r_z0__5;
		c_mut__6 = r_mut__5;
		
		//for 7
		c_x__7	= r_x__6;
		c_y__7	= r_y__6;
		c_z__7	= r_z__6;
		c_ux__7	= r_ux__6;
		c_uy__7	= r_uy__6;
		c_uz__7	= r_uz__6;
		c_sz__7	= r_sz__6;
		c_sr__7	= r_sr__6;
		c_sleftz__7	= r_sleftz__6;
		c_sleftr__7	= r_sleftr__6;
		c_weight__7	= r_weight__6;
		c_layer__7	= r_layer__6;
		c_dead__7	= r_dead__6;
		c_hit__7	= r_hit__6;
		c_diff__7 = r_diff__6;
		c_dl_b__7 = r_dl_b__6;
		c_numer__7 = r_numer__6;
		c_z1__7 = r_z1__6;
		c_z0__7 = r_z0__6;
		c_mut__7 = r_mut__6;
		
		//for 8
		c_x__8	= r_x__7;
		c_y__8	= r_y__7;
		c_z__8	= r_z__7;
		c_ux__8	= r_ux__7;
		c_uy__8	= r_uy__7;
		c_uz__8	= r_uz__7;
		c_sz__8	= r_sz__7;
		c_sr__8	= r_sr__7;
		c_sleftz__8	= r_sleftz__7;
		c_sleftr__8	= r_sleftr__7;
		c_weight__8	= r_weight__7;
		c_layer__8	= r_layer__7;
		c_dead__8	= r_dead__7;
		c_hit__8	= r_hit__7;
		c_diff__8 = r_diff__7;
		c_dl_b__8 = r_dl_b__7;
		c_numer__8 = r_numer__7;
		c_z1__8 = r_z1__7;
		c_z0__8 = r_z0__7;
		c_mut__8 = r_mut__7;
		
		//for 9
		c_x__9	= r_x__8;
		c_y__9	= r_y__8;
		c_z__9	= r_z__8;
		c_ux__9	= r_ux__8;
		c_uy__9	= r_uy__8;
		c_uz__9	= r_uz__8;
		c_sz__9	= r_sz__8;
		c_sr__9	= r_sr__8;
		c_sleftz__9	= r_sleftz__8;
		c_sleftr__9	= r_sleftr__8;
		c_weight__9	= r_weight__8;
		c_layer__9	= r_layer__8;
		c_dead__9	= r_dead__8;
		c_hit__9	= r_hit__8;
		c_diff__9 = r_diff__8;
		c_dl_b__9 = r_dl_b__8;
		c_numer__9 = r_numer__8;
		c_z1__9 = r_z1__8;
		c_z0__9 = r_z0__8;
		c_mut__9 = r_mut__8;
		
		//for 10
		c_x__10	= r_x__9;
		c_y__10	= r_y__9;
		c_z__10	= r_z__9;
		c_ux__10	= r_ux__9;
		c_uy__10	= r_uy__9;
		c_uz__10	= r_uz__9;
		c_sz__10	= r_sz__9;
		c_sr__10	= r_sr__9;
		c_sleftz__10	= r_sleftz__9;
		c_sleftr__10	= r_sleftr__9;
		c_weight__10	= r_weight__9;
		c_layer__10	= r_layer__9;
		c_dead__10	= r_dead__9;
		c_hit__10	= r_hit__9;
		c_diff__10 = r_diff__9;
		c_dl_b__10 = r_dl_b__9;
		c_numer__10 = r_numer__9;
		c_z1__10 = r_z1__9;
		c_z0__10 = r_z0__9;
		c_mut__10 = r_mut__9;
		
		//for 11
		c_x__11	= r_x__10;
		c_y__11	= r_y__10;
		c_z__11	= r_z__10;
		c_ux__11	= r_ux__10;
		c_uy__11	= r_uy__10;
		c_uz__11	= r_uz__10;
		c_sz__11	= r_sz__10;
		c_sr__11	= r_sr__10;
		c_sleftz__11	= r_sleftz__10;
		c_sleftr__11	= r_sleftr__10;
		c_weight__11	= r_weight__10;
		c_layer__11	= r_layer__10;
		c_dead__11	= r_dead__10;
		c_hit__11	= r_hit__10;
		c_diff__11 = r_diff__10;
		c_dl_b__11 = r_dl_b__10;
		c_numer__11 = r_numer__10;
		c_z1__11 = r_z1__10;
		c_z0__11 = r_z0__10;
		c_mut__11 = r_mut__10;
		
		//for 12
		c_x__12	= r_x__11;
		c_y__12	= r_y__11;
		c_z__12	= r_z__11;
		c_ux__12	= r_ux__11;
		c_uy__12	= r_uy__11;
		c_uz__12	= r_uz__11;
		c_sz__12	= r_sz__11;
		c_sr__12	= r_sr__11;
		c_sleftz__12	= r_sleftz__11;
		c_sleftr__12	= r_sleftr__11;
		c_weight__12	= r_weight__11;
		c_layer__12	= r_layer__11;
		c_dead__12	= r_dead__11;
		c_hit__12	= r_hit__11;
		c_diff__12 = r_diff__11;
		c_dl_b__12 = r_dl_b__11;
		c_numer__12 = r_numer__11;
		c_z1__12 = r_z1__11;
		c_z0__12 = r_z0__11;
		c_mut__12 = r_mut__11;
		
		//for 13
		c_x__13	= r_x__12;
		c_y__13	= r_y__12;
		c_z__13	= r_z__12;
		c_ux__13	= r_ux__12;
		c_uy__13	= r_uy__12;
		c_uz__13	= r_uz__12;
		c_sz__13	= r_sz__12;
		c_sr__13	= r_sr__12;
		c_sleftz__13	= r_sleftz__12;
		c_sleftr__13	= r_sleftr__12;
		c_weight__13	= r_weight__12;
		c_layer__13	= r_layer__12;
		c_dead__13	= r_dead__12;
		c_hit__13	= r_hit__12;
		c_diff__13 = r_diff__12;
		c_dl_b__13 = r_dl_b__12;
		c_numer__13 = r_numer__12;
		c_z1__13 = r_z1__12;
		c_z0__13 = r_z0__12;
		c_mut__13 = r_mut__12;
		
		//for 14
		c_x__14	= r_x__13;
		c_y__14	= r_y__13;
		c_z__14	= r_z__13;
		c_ux__14	= r_ux__13;
		c_uy__14	= r_uy__13;
		c_uz__14	= r_uz__13;
		c_sz__14	= r_sz__13;
		c_sr__14	= r_sr__13;
		c_sleftz__14	= r_sleftz__13;
		c_sleftr__14	= r_sleftr__13;
		c_weight__14	= r_weight__13;
		c_layer__14	= r_layer__13;
		c_dead__14	= r_dead__13;
		c_hit__14	= r_hit__13;
		c_diff__14 = r_diff__13;
		c_dl_b__14 = r_dl_b__13;
		c_numer__14 = r_numer__13;
		c_z1__14 = r_z1__13;
		c_z0__14 = r_z0__13;
		c_mut__14 = r_mut__13;
		
		//for 15
		c_x__15	= r_x__14;
		c_y__15	= r_y__14;
		c_z__15	= r_z__14;
		c_ux__15	= r_ux__14;
		c_uy__15	= r_uy__14;
		c_uz__15	= r_uz__14;
		c_sz__15	= r_sz__14;
		c_sr__15	= r_sr__14;
		c_sleftz__15	= r_sleftz__14;
		c_sleftr__15	= r_sleftr__14;
		c_weight__15	= r_weight__14;
		c_layer__15	= r_layer__14;
		c_dead__15	= r_dead__14;
		c_hit__15	= r_hit__14;
		c_diff__15 = r_diff__14;
		c_dl_b__15 = r_dl_b__14;
		c_numer__15 = r_numer__14;
		c_z1__15 = r_z1__14;
		c_z0__15 = r_z0__14;
		c_mut__15 = r_mut__14;
		
		//for 16
		c_x__16	= r_x__15;
		c_y__16	= r_y__15;
		c_z__16	= r_z__15;
		c_ux__16	= r_ux__15;
		c_uy__16	= r_uy__15;
		c_uz__16	= r_uz__15;
		c_sz__16	= r_sz__15;
		c_sr__16	= r_sr__15;
		c_sleftz__16	= r_sleftz__15;
		c_sleftr__16	= r_sleftr__15;
		c_weight__16	= r_weight__15;
		c_layer__16	= r_layer__15;
		c_dead__16	= r_dead__15;
		c_hit__16	= r_hit__15;
		c_diff__16 = r_diff__15;
		c_dl_b__16 = r_dl_b__15;
		c_numer__16 = r_numer__15;
		c_z1__16 = r_z1__15;
		c_z0__16 = r_z0__15;
		c_mut__16 = r_mut__15;
		
		//for 17
		c_x__17	= r_x__16;
		c_y__17	= r_y__16;
		c_z__17	= r_z__16;
		c_ux__17	= r_ux__16;
		c_uy__17	= r_uy__16;
		c_uz__17	= r_uz__16;
		c_sz__17	= r_sz__16;
		c_sr__17	= r_sr__16;
		c_sleftz__17	= r_sleftz__16;
		c_sleftr__17	= r_sleftr__16;
		c_weight__17	= r_weight__16;
		c_layer__17	= r_layer__16;
		c_dead__17	= r_dead__16;
		c_hit__17	= r_hit__16;
		c_diff__17 = r_diff__16;
		c_dl_b__17 = r_dl_b__16;
		c_numer__17 = r_numer__16;
		c_z1__17 = r_z1__16;
		c_z0__17 = r_z0__16;
		c_mut__17 = r_mut__16;
		
		//for 18
		c_x__18	= r_x__17;
		c_y__18	= r_y__17;
		c_z__18	= r_z__17;
		c_ux__18	= r_ux__17;
		c_uy__18	= r_uy__17;
		c_uz__18	= r_uz__17;
		c_sz__18	= r_sz__17;
		c_sr__18	= r_sr__17;
		c_sleftz__18	= r_sleftz__17;
		c_sleftr__18	= r_sleftr__17;
		c_weight__18	= r_weight__17;
		c_layer__18	= r_layer__17;
		c_dead__18	= r_dead__17;
		c_hit__18	= r_hit__17;
		c_diff__18 = r_diff__17;
		c_dl_b__18 = r_dl_b__17;
		c_numer__18 = r_numer__17;
		c_z1__18 = r_z1__17;
		c_z0__18 = r_z0__17;
		c_mut__18 = r_mut__17;
		
		//for 19
		c_x__19	= r_x__18;
		c_y__19	= r_y__18;
		c_z__19	= r_z__18;
		c_ux__19	= r_ux__18;
		c_uy__19	= r_uy__18;
		c_uz__19	= r_uz__18;
		c_sz__19	= r_sz__18;
		c_sr__19	= r_sr__18;
		c_sleftz__19	= r_sleftz__18;
		c_sleftr__19	= r_sleftr__18;
		c_weight__19	= r_weight__18;
		c_layer__19	= r_layer__18;
		c_dead__19	= r_dead__18;
		c_hit__19	= r_hit__18;
		c_diff__19 = r_diff__18;
		c_dl_b__19 = r_dl_b__18;
		c_numer__19 = r_numer__18;
		c_z1__19 = r_z1__18;
		c_z0__19 = r_z0__18;
		c_mut__19 = r_mut__18;
		
		//for 20
		c_x__20	= r_x__19;
		c_y__20	= r_y__19;
		c_z__20	= r_z__19;
		c_ux__20	= r_ux__19;
		c_uy__20	= r_uy__19;
		c_uz__20	= r_uz__19;
		c_sz__20	= r_sz__19;
		c_sr__20	= r_sr__19;
		c_sleftz__20	= r_sleftz__19;
		c_sleftr__20	= r_sleftr__19;
		c_weight__20	= r_weight__19;
		c_layer__20	= r_layer__19;
		c_dead__20	= r_dead__19;
		c_hit__20	= r_hit__19;
		c_diff__20 = r_diff__19;
		c_dl_b__20 = r_dl_b__19;
		c_numer__20 = r_numer__19;
		c_z1__20 = r_z1__19;
		c_z0__20 = r_z0__19;
		c_mut__20 = r_mut__19;
		
		
		//for 21
		c_x__21	= r_x__20;
		c_y__21	= r_y__20;
		c_z__21	= r_z__20;
		c_ux__21	= r_ux__20;
		c_uy__21	= r_uy__20;
		c_uz__21	= r_uz__20;
		c_sz__21	= r_sz__20;
		c_sr__21	= r_sr__20;
		c_sleftz__21	= r_sleftz__20;
		c_sleftr__21	= r_sleftr__20;
		c_weight__21	= r_weight__20;
		c_layer__21	= r_layer__20;
		c_dead__21	= r_dead__20;
		c_hit__21	= r_hit__20;
		c_diff__21 = r_diff__20;
		c_dl_b__21 = r_dl_b__20;
		c_numer__21 = r_numer__20;
		c_z1__21 = r_z1__20;
		c_z0__21 = r_z0__20;
		c_mut__21 = r_mut__20;
		
		//for 22
		c_x__22	= r_x__21;
		c_y__22	= r_y__21;
		c_z__22	= r_z__21;
		c_ux__22	= r_ux__21;
		c_uy__22	= r_uy__21;
		c_uz__22	= r_uz__21;
		c_sz__22	= r_sz__21;
		c_sr__22	= r_sr__21;
		c_sleftz__22	= r_sleftz__21;
		c_sleftr__22	= r_sleftr__21;
		c_weight__22	= r_weight__21;
		c_layer__22	= r_layer__21;
		c_dead__22	= r_dead__21;
		c_hit__22	= r_hit__21;
		c_diff__22 = r_diff__21;
		c_dl_b__22 = r_dl_b__21;
		c_numer__22 = r_numer__21;
		c_z1__22 = r_z1__21;
		c_z0__22 = r_z0__21;
		c_mut__22 = r_mut__21;
		
		//for 23
		c_x__23	= r_x__22;
		c_y__23	= r_y__22;
		c_z__23	= r_z__22;
		c_ux__23	= r_ux__22;
		c_uy__23	= r_uy__22;
		c_uz__23	= r_uz__22;
		c_sz__23	= r_sz__22;
		c_sr__23	= r_sr__22;
		c_sleftz__23	= r_sleftz__22;
		c_sleftr__23	= r_sleftr__22;
		c_weight__23	= r_weight__22;
		c_layer__23	= r_layer__22;
		c_dead__23	= r_dead__22;
		c_hit__23	= r_hit__22;
		c_diff__23 = r_diff__22;
		c_dl_b__23 = r_dl_b__22;
		c_numer__23 = r_numer__22;
		c_z1__23 = r_z1__22;
		c_z0__23 = r_z0__22;
		c_mut__23 = r_mut__22;
		
		//for 24
		c_x__24	= r_x__23;
		c_y__24	= r_y__23;
		c_z__24	= r_z__23;
		c_ux__24	= r_ux__23;
		c_uy__24	= r_uy__23;
		c_uz__24	= r_uz__23;
		c_sz__24	= r_sz__23;
		c_sr__24	= r_sr__23;
		c_sleftz__24	= r_sleftz__23;
		c_sleftr__24	= r_sleftr__23;
		c_weight__24	= r_weight__23;
		c_layer__24	= r_layer__23;
		c_dead__24	= r_dead__23;
		c_hit__24	= r_hit__23;
		c_diff__24 = r_diff__23;
		c_dl_b__24 = r_dl_b__23;
		c_numer__24 = r_numer__23;
		c_z1__24 = r_z1__23;
		c_z0__24 = r_z0__23;
		c_mut__24 = r_mut__23;
		
		//for 25
		c_x__25	= r_x__24;
		c_y__25	= r_y__24;
		c_z__25	= r_z__24;
		c_ux__25	= r_ux__24;
		c_uy__25	= r_uy__24;
		c_uz__25	= r_uz__24;
		c_sz__25	= r_sz__24;
		c_sr__25	= r_sr__24;
		c_sleftz__25	= r_sleftz__24;
		c_sleftr__25	= r_sleftr__24;
		c_weight__25	= r_weight__24;
		c_layer__25	= r_layer__24;
		c_dead__25	= r_dead__24;
		c_hit__25	= r_hit__24;
		c_diff__25 = r_diff__24;
		c_dl_b__25 = r_dl_b__24;
		c_numer__25 = r_numer__24;
		c_z1__25 = r_z1__24;
		c_z0__25 = r_z0__24;
		c_mut__25 = r_mut__24;
		
		//for 26
		c_x__26	= r_x__25;
		c_y__26	= r_y__25;
		c_z__26	= r_z__25;
		c_ux__26	= r_ux__25;
		c_uy__26	= r_uy__25;
		c_uz__26	= r_uz__25;
		c_sz__26	= r_sz__25;
		c_sr__26	= r_sr__25;
		c_sleftz__26	= r_sleftz__25;
		c_sleftr__26	= r_sleftr__25;
		c_weight__26	= r_weight__25;
		c_layer__26	= r_layer__25;
		c_dead__26	= r_dead__25;
		c_hit__26	= r_hit__25;
		c_diff__26 = r_diff__25;
		c_dl_b__26 = r_dl_b__25;
		c_numer__26 = r_numer__25;
		c_z1__26 = r_z1__25;
		c_z0__26 = r_z0__25;
		c_mut__26 = r_mut__25;
		
		//for 27
		c_x__27	= r_x__26;
		c_y__27	= r_y__26;
		c_z__27	= r_z__26;
		c_ux__27	= r_ux__26;
		c_uy__27	= r_uy__26;
		c_uz__27	= r_uz__26;
		c_sz__27	= r_sz__26;
		c_sr__27	= r_sr__26;
		c_sleftz__27	= r_sleftz__26;
		c_sleftr__27	= r_sleftr__26;
		c_weight__27	= r_weight__26;
		c_layer__27	= r_layer__26;
		c_dead__27	= r_dead__26;
		c_hit__27	= r_hit__26;
		c_diff__27 = r_diff__26;
		c_dl_b__27 = r_dl_b__26;
		c_numer__27 = r_numer__26;
		c_z1__27 = r_z1__26;
		c_z0__27 = r_z0__26;
		c_mut__27 = r_mut__26;
		
		//for 28
		c_x__28	= r_x__27;
		c_y__28	= r_y__27;
		c_z__28	= r_z__27;
		c_ux__28	= r_ux__27;
		c_uy__28	= r_uy__27;
		c_uz__28	= r_uz__27;
		c_sz__28	= r_sz__27;
		c_sr__28	= r_sr__27;
		c_sleftz__28	= r_sleftz__27;
		c_sleftr__28	= r_sleftr__27;
		c_weight__28	= r_weight__27;
		c_layer__28	= r_layer__27;
		c_dead__28	= r_dead__27;
		c_hit__28	= r_hit__27;
		c_diff__28 = r_diff__27;
		c_dl_b__28 = r_dl_b__27;
		c_numer__28 = r_numer__27;
		c_z1__28 = r_z1__27;
		c_z0__28 = r_z0__27;
		c_mut__28 = r_mut__27;
		
		//for 29
		c_x__29	= r_x__28;
		c_y__29	= r_y__28;
		c_z__29	= r_z__28;
		c_ux__29	= r_ux__28;
		c_uy__29	= r_uy__28;
		c_uz__29	= r_uz__28;
		c_sz__29	= r_sz__28;
		c_sr__29	= r_sr__28;
		c_sleftz__29	= r_sleftz__28;
		c_sleftr__29	= r_sleftr__28;
		c_weight__29	= r_weight__28;
		c_layer__29	= r_layer__28;
		c_dead__29	= r_dead__28;
		c_hit__29	= r_hit__28;
		c_diff__29 = r_diff__28;
		c_dl_b__29 = r_dl_b__28;
		c_numer__29 = r_numer__28;
		c_z1__29 = r_z1__28;
		c_z0__29 = r_z0__28;
		c_mut__29 = r_mut__28;
		
		//for 30
		c_x__30	= r_x__29;
		c_y__30	= r_y__29;
		c_z__30	= r_z__29;
		c_ux__30	= r_ux__29;
		c_uy__30	= r_uy__29;
		c_uz__30	= r_uz__29;
		c_sz__30	= r_sz__29;
		c_sr__30	= r_sr__29;
		c_sleftz__30	= r_sleftz__29;
		c_sleftr__30	= r_sleftr__29;
		c_weight__30	= r_weight__29;
		c_layer__30	= r_layer__29;
		c_dead__30	= r_dead__29;
	//	c_hit__30	= r_hit__29;//
	//	c_diff__30 = r_diff__29;//
	// this value is set later, removing default - peter m	
	//	c_dl_b__30 = r_dl_b__29;//
	// this one too
		c_numer__30 = r_numer__29;
		c_z1__30 = r_z1__29;
		c_z0__30 = r_z0__29;
		c_mut__30 = r_mut__29;
		
		//for 31
		c_x__31	= r_x__30;
		c_y__31	= r_y__30;
	//	c_z__31	= r_z__30;//
		c_ux__31	= r_ux__30;
		c_uy__31	= r_uy__30;
		c_uz__31	= r_uz__30;
	//	c_sz__31	= r_sz__30;//
	//	c_sr__31	= r_sr__30;//
	//	c_sleftz__31	= r_sleftz__30;//
		c_sleftr__31	= r_sleftr__30;
		c_weight__31	= r_weight__30;
		c_layer__31	= r_layer__30;
		c_dead__31	= r_dead__30;
		c_hit__31	= r_hit__30;
		c_diff__31 = r_diff__30;
		c_dl_b__31 = r_dl_b__30;
		c_numer__31 = r_numer__30;
		c_z1__31 = r_z1__30;
		c_z0__31 = r_z0__30;
		c_mut__31 = r_mut__30;
		
		//for 32
		c_x__32	= r_x__31;
		c_y__32	= r_y__31;
		c_z__32	= r_z__31;
		c_ux__32	= r_ux__31;
		c_uy__32	= r_uy__31;
		c_uz__32	= r_uz__31;
		c_sz__32	= r_sz__31;
		c_sr__32	= r_sr__31;
		c_sleftz__32	= r_sleftz__31;
	//	c_sleftr__32	= r_sleftr__31;//
		c_weight__32	= r_weight__31;
		c_layer__32	= r_layer__31;
		c_dead__32	= r_dead__31;
		c_hit__32	= r_hit__31;
		c_diff__32 = r_diff__31;
		c_dl_b__32 = r_dl_b__31;
		c_numer__32 = r_numer__31;
		c_z1__32 = r_z1__31;
		c_z0__32 = r_z0__31;
		c_mut__32 = r_mut__31;
		
		//for 33
		c_x__33	= r_x__32;
		c_y__33	= r_y__32;
		c_z__33	= r_z__32;
		c_ux__33	= r_ux__32;
		c_uy__33	= r_uy__32;
		c_uz__33	= r_uz__32;
		c_sz__33	= r_sz__32;
		c_sr__33	= r_sr__32;
		c_sleftz__33	= r_sleftz__32;
		c_sleftr__33	= r_sleftr__32;
		c_weight__33	= r_weight__32;
		c_layer__33	= r_layer__32;
		c_dead__33	= r_dead__32;
		c_hit__33	= r_hit__32;
		c_diff__33 = r_diff__32;
		c_dl_b__33 = r_dl_b__32;
		c_numer__33 = r_numer__32;
		c_z1__33 = r_z1__32;
		c_z0__33 = r_z0__32;
		c_mut__33 = r_mut__32;
		
		//for 34
		c_x__34	= r_x__33;
		c_y__34	= r_y__33;
		c_z__34	= r_z__33;
		c_ux__34	= r_ux__33;
		c_uy__34	= r_uy__33;
		c_uz__34	= r_uz__33;
		c_sz__34	= r_sz__33;
		c_sr__34	= r_sr__33;
		c_sleftz__34	= r_sleftz__33;
		c_sleftr__34	= r_sleftr__33;
		c_weight__34	= r_weight__33;
		c_layer__34	= r_layer__33;
		c_dead__34	= r_dead__33;
		c_hit__34	= r_hit__33;
		c_diff__34 = r_diff__33;
		c_dl_b__34 = r_dl_b__33;
		c_numer__34 = r_numer__33;
		c_z1__34 = r_z1__33;
		c_z0__34 = r_z0__33;
		c_mut__34 = r_mut__33;
		
		//for 35
		c_x__35	= r_x__34;
		c_y__35	= r_y__34;
		c_z__35	= r_z__34;
		c_ux__35	= r_ux__34;
		c_uy__35	= r_uy__34;
		c_uz__35	= r_uz__34;
		c_sz__35	= r_sz__34;
		c_sr__35	= r_sr__34;
		c_sleftz__35	= r_sleftz__34;
		c_sleftr__35	= r_sleftr__34;
		c_weight__35	= r_weight__34;
		c_layer__35	= r_layer__34;
		c_dead__35	= r_dead__34;
		c_hit__35	= r_hit__34;
		c_diff__35 = r_diff__34;
		c_dl_b__35 = r_dl_b__34;
		c_numer__35 = r_numer__34;
		c_z1__35 = r_z1__34;
		c_z0__35 = r_z0__34;
		c_mut__35 = r_mut__34;
		
		//for 36
		c_x__36	= r_x__35;
		c_y__36	= r_y__35;
		c_z__36	= r_z__35;
		c_ux__36	= r_ux__35;
		c_uy__36	= r_uy__35;
		c_uz__36	= r_uz__35;
		c_sz__36	= r_sz__35;
		c_sr__36	= r_sr__35;
		c_sleftz__36	= r_sleftz__35;
		c_sleftr__36	= r_sleftr__35;
		c_weight__36	= r_weight__35;
		c_layer__36	= r_layer__35;
		c_dead__36	= r_dead__35;
		c_hit__36	= r_hit__35;
		c_diff__36 = r_diff__35;
		c_dl_b__36 = r_dl_b__35;
		c_numer__36 = r_numer__35;
		c_z1__36 = r_z1__35;
		c_z0__36 = r_z0__35;
		c_mut__36 = r_mut__35;
		
		//for 37
		c_x__37	= r_x__36;
		c_y__37	= r_y__36;
		c_z__37	= r_z__36;
		c_ux__37	= r_ux__36;
		c_uy__37	= r_uy__36;
		c_uz__37	= r_uz__36;
		c_sz__37	= r_sz__36;
		c_sr__37	= r_sr__36;
		c_sleftz__37	= r_sleftz__36;
		c_sleftr__37	= r_sleftr__36;
		c_weight__37	= r_weight__36;
		c_layer__37	= r_layer__36;
		c_dead__37	= r_dead__36;
		c_hit__37	= r_hit__36;
		c_diff__37 = r_diff__36;
		c_dl_b__37 = r_dl_b__36;
		c_numer__37 = r_numer__36;
		c_z1__37 = r_z1__36;
		c_z0__37 = r_z0__36;
		c_mut__37 = r_mut__36;
		
		//for 38
		c_x__38	= r_x__37;
		c_y__38	= r_y__37;
		c_z__38	= r_z__37;
		c_ux__38	= r_ux__37;
		c_uy__38	= r_uy__37;
		c_uz__38	= r_uz__37;
		c_sz__38	= r_sz__37;
		c_sr__38	= r_sr__37;
		c_sleftz__38	= r_sleftz__37;
		c_sleftr__38	= r_sleftr__37;
		c_weight__38	= r_weight__37;
		c_layer__38	= r_layer__37;
		c_dead__38	= r_dead__37;
		c_hit__38	= r_hit__37;
		c_diff__38 = r_diff__37;
		c_dl_b__38 = r_dl_b__37;
		c_numer__38 = r_numer__37;
		c_z1__38 = r_z1__37;
		c_z0__38 = r_z0__37;
		c_mut__38 = r_mut__37;
		
		//for 39
		c_x__39	= r_x__38;
		c_y__39	= r_y__38;
		c_z__39	= r_z__38;
		c_ux__39	= r_ux__38;
		c_uy__39	= r_uy__38;
		c_uz__39	= r_uz__38;
		c_sz__39	= r_sz__38;
		c_sr__39	= r_sr__38;
		c_sleftz__39	= r_sleftz__38;
		c_sleftr__39	= r_sleftr__38;
		c_weight__39	= r_weight__38;
		c_layer__39	= r_layer__38;
		c_dead__39	= r_dead__38;
		c_hit__39	= r_hit__38;
		c_diff__39 = r_diff__38;
		c_dl_b__39 = r_dl_b__38;
		c_numer__39 = r_numer__38;
		c_z1__39 = r_z1__38;
		c_z0__39 = r_z0__38;
		c_mut__39 = r_mut__38;
		
		//for 40
		c_x__40	= r_x__39;
		c_y__40	= r_y__39;
		c_z__40	= r_z__39;
		c_ux__40	= r_ux__39;
		c_uy__40	= r_uy__39;
		c_uz__40	= r_uz__39;
		c_sz__40	= r_sz__39;
		c_sr__40	= r_sr__39;
		c_sleftz__40	= r_sleftz__39;
		c_sleftr__40	= r_sleftr__39;
		c_weight__40	= r_weight__39;
		c_layer__40	= r_layer__39;
		c_dead__40	= r_dead__39;
		c_hit__40	= r_hit__39;
		c_diff__40 = r_diff__39;
		c_dl_b__40 = r_dl_b__39;
		c_numer__40 = r_numer__39;
		c_z1__40 = r_z1__39;
		c_z0__40 = r_z0__39;
		c_mut__40 = r_mut__39;
		
		//for 41
		c_x__41	= r_x__40;
		c_y__41	= r_y__40;
		c_z__41	= r_z__40;
		c_ux__41	= r_ux__40;
		c_uy__41	= r_uy__40;
		c_uz__41	= r_uz__40;
		c_sz__41	= r_sz__40;
		c_sr__41	= r_sr__40;
		c_sleftz__41	= r_sleftz__40;
		c_sleftr__41	= r_sleftr__40;
		c_weight__41	= r_weight__40;
		c_layer__41	= r_layer__40;
		c_dead__41	= r_dead__40;
		c_hit__41	= r_hit__40;
		c_diff__41 = r_diff__40;
		c_dl_b__41 = r_dl_b__40;
		c_numer__41 = r_numer__40;
		c_z1__41 = r_z1__40;
		c_z0__41 = r_z0__40;
		c_mut__41 = r_mut__40;
		
		//for 42
		c_x__42	= r_x__41;
		c_y__42	= r_y__41;
		c_z__42	= r_z__41;
		c_ux__42	= r_ux__41;
		c_uy__42	= r_uy__41;
		c_uz__42	= r_uz__41;
		c_sz__42	= r_sz__41;
		c_sr__42	= r_sr__41;
		c_sleftz__42	= r_sleftz__41;
		c_sleftr__42	= r_sleftr__41;
		c_weight__42	= r_weight__41;
		c_layer__42	= r_layer__41;
		c_dead__42	= r_dead__41;
		c_hit__42	= r_hit__41;
		c_diff__42 = r_diff__41;
		c_dl_b__42 = r_dl_b__41;
		c_numer__42 = r_numer__41;
		c_z1__42 = r_z1__41;
		c_z0__42 = r_z0__41;
		c_mut__42 = r_mut__41;
		
		//for 43
		c_x__43	= r_x__42;
		c_y__43	= r_y__42;
		c_z__43	= r_z__42;
		c_ux__43	= r_ux__42;
		c_uy__43	= r_uy__42;
		c_uz__43	= r_uz__42;
		c_sz__43	= r_sz__42;
		c_sr__43	= r_sr__42;
		c_sleftz__43	= r_sleftz__42;
		c_sleftr__43	= r_sleftr__42;
		c_weight__43	= r_weight__42;
		c_layer__43	= r_layer__42;
		c_dead__43	= r_dead__42;
		c_hit__43	= r_hit__42;
		c_diff__43 = r_diff__42;
		c_dl_b__43 = r_dl_b__42;
		c_numer__43 = r_numer__42;
		c_z1__43 = r_z1__42;
		c_z0__43 = r_z0__42;
		c_mut__43 = r_mut__42;
		
		//for 44
		c_x__44	= r_x__43;
		c_y__44	= r_y__43;
		c_z__44	= r_z__43;
		c_ux__44	= r_ux__43;
		c_uy__44	= r_uy__43;
		c_uz__44	= r_uz__43;
		c_sz__44	= r_sz__43;
		c_sr__44	= r_sr__43;
		c_sleftz__44	= r_sleftz__43;
		c_sleftr__44	= r_sleftr__43;
		c_weight__44	= r_weight__43;
		c_layer__44	= r_layer__43;
		c_dead__44	= r_dead__43;
		c_hit__44	= r_hit__43;
		c_diff__44 = r_diff__43;
		c_dl_b__44 = r_dl_b__43;
		c_numer__44 = r_numer__43;
		c_z1__44 = r_z1__43;
		c_z0__44 = r_z0__43;
		c_mut__44 = r_mut__43;
		
		//for 45
		c_x__45	= r_x__44;
		c_y__45	= r_y__44;
		c_z__45	= r_z__44;
		c_ux__45	= r_ux__44;
		c_uy__45	= r_uy__44;
		c_uz__45	= r_uz__44;
		c_sz__45	= r_sz__44;
		c_sr__45	= r_sr__44;
		c_sleftz__45	= r_sleftz__44;
		c_sleftr__45	= r_sleftr__44;
		c_weight__45	= r_weight__44;
		c_layer__45	= r_layer__44;
		c_dead__45	= r_dead__44;
		c_hit__45	= r_hit__44;
		c_diff__45 = r_diff__44;
		c_dl_b__45 = r_dl_b__44;
		c_numer__45 = r_numer__44;
		c_z1__45 = r_z1__44;
		c_z0__45 = r_z0__44;
		c_mut__45 = r_mut__44;
		
		//for 46
		c_x__46	= r_x__45;
		c_y__46	= r_y__45;
		c_z__46	= r_z__45;
		c_ux__46	= r_ux__45;
		c_uy__46	= r_uy__45;
		c_uz__46	= r_uz__45;
		c_sz__46	= r_sz__45;
		c_sr__46	= r_sr__45;
		c_sleftz__46	= r_sleftz__45;
		c_sleftr__46	= r_sleftr__45;
		c_weight__46	= r_weight__45;
		c_layer__46	= r_layer__45;
		c_dead__46	= r_dead__45;
		c_hit__46	= r_hit__45;
		c_diff__46 = r_diff__45;
		c_dl_b__46 = r_dl_b__45;
		c_numer__46 = r_numer__45;
		c_z1__46 = r_z1__45;
		c_z0__46 = r_z0__45;
		c_mut__46 = r_mut__45;
		
		//for 47
		c_x__47	= r_x__46;
		c_y__47	= r_y__46;
		c_z__47	= r_z__46;
		c_ux__47	= r_ux__46;
		c_uy__47	= r_uy__46;
		c_uz__47	= r_uz__46;
		c_sz__47	= r_sz__46;
		c_sr__47	= r_sr__46;
		c_sleftz__47	= r_sleftz__46;
		c_sleftr__47	= r_sleftr__46;
		c_weight__47	= r_weight__46;
		c_layer__47	= r_layer__46;
		c_dead__47	= r_dead__46;
		c_hit__47	= r_hit__46;
		c_diff__47 = r_diff__46;
		c_dl_b__47 = r_dl_b__46;
		c_numer__47 = r_numer__46;
		c_z1__47 = r_z1__46;
		c_z0__47 = r_z0__46;
		c_mut__47 = r_mut__46;
		
		//for 48
		c_x__48	= r_x__47;
		c_y__48	= r_y__47;
		c_z__48	= r_z__47;
		c_ux__48	= r_ux__47;
		c_uy__48	= r_uy__47;
		c_uz__48	= r_uz__47;
		c_sz__48	= r_sz__47;
		c_sr__48	= r_sr__47;
		c_sleftz__48	= r_sleftz__47;
		c_sleftr__48	= r_sleftr__47;
		c_weight__48	= r_weight__47;
		c_layer__48	= r_layer__47;
		c_dead__48	= r_dead__47;
		c_hit__48	= r_hit__47;
		c_diff__48 = r_diff__47;
		c_dl_b__48 = r_dl_b__47;
		c_numer__48 = r_numer__47;
		c_z1__48 = r_z1__47;
		c_z0__48 = r_z0__47;
		c_mut__48 = r_mut__47;
		
		//for 49
		c_x__49	= r_x__48;
		c_y__49	= r_y__48;
		c_z__49	= r_z__48;
		c_ux__49	= r_ux__48;
		c_uy__49	= r_uy__48;
		c_uz__49	= r_uz__48;
		c_sz__49	= r_sz__48;
		c_sr__49	= r_sr__48;
		c_sleftz__49	= r_sleftz__48;
		c_sleftr__49	= r_sleftr__48;
		c_weight__49	= r_weight__48;
		c_layer__49	= r_layer__48;
		c_dead__49	= r_dead__48;
		c_hit__49	= r_hit__48;
		c_diff__49 = r_diff__48;
		c_dl_b__49 = r_dl_b__48;
		c_numer__49 = r_numer__48;
		c_z1__49 = r_z1__48;
		c_z0__49 = r_z0__48;
		c_mut__49 = r_mut__48;
		
		//for 50
		c_x__50	= r_x__49;
		c_y__50	= r_y__49;
		c_z__50	= r_z__49;
		c_ux__50	= r_ux__49;
		c_uy__50	= r_uy__49;
		c_uz__50	= r_uz__49;
		c_sz__50	= r_sz__49;
		c_sr__50	= r_sr__49;
		c_sleftz__50	= r_sleftz__49;
		c_sleftr__50	= r_sleftr__49;
		c_weight__50	= r_weight__49;
		c_layer__50	= r_layer__49;
		c_dead__50	= r_dead__49;
		c_hit__50	= r_hit__49;
		c_diff__50 = r_diff__49;
		c_dl_b__50 = r_dl_b__49;
		c_numer__50 = r_numer__49;
		c_z1__50 = r_z1__49;
		c_z0__50 = r_z0__49;
		c_mut__50 = r_mut__49;
		
		//for 51
		c_x__51	= r_x__50;
		c_y__51	= r_y__50;
		c_z__51	= r_z__50;
		c_ux__51	= r_ux__50;
		c_uy__51	= r_uy__50;
		c_uz__51	= r_uz__50;
		c_sz__51	= r_sz__50;
		c_sr__51	= r_sr__50;
		c_sleftz__51	= r_sleftz__50;
		c_sleftr__51	= r_sleftr__50;
		c_weight__51	= r_weight__50;
		c_layer__51	= r_layer__50;
		c_dead__51	= r_dead__50;
		c_hit__51	= r_hit__50;
		c_diff__51 = r_diff__50;
		c_dl_b__51 = r_dl_b__50;
		c_numer__51 = r_numer__50;
		c_z1__51 = r_z1__50;
		c_z0__51 = r_z0__50;
		c_mut__51 = r_mut__50;
		
		//for 52
		c_x__52	= r_x__51;
		c_y__52	= r_y__51;
		c_z__52	= r_z__51;
		c_ux__52	= r_ux__51;
		c_uy__52	= r_uy__51;
		c_uz__52	= r_uz__51;
		c_sz__52	= r_sz__51;
		c_sr__52	= r_sr__51;
		c_sleftz__52	= r_sleftz__51;
		c_sleftr__52	= r_sleftr__51;
		c_weight__52	= r_weight__51;
		c_layer__52	= r_layer__51;
		c_dead__52	= r_dead__51;
		c_hit__52	= r_hit__51;
		c_diff__52 = r_diff__51;
		c_dl_b__52 = r_dl_b__51;
		c_numer__52 = r_numer__51;
		c_z1__52 = r_z1__51;
		c_z0__52 = r_z0__51;
		c_mut__52 = r_mut__51;
		
		//for 53
		c_x__53	= r_x__52;
		c_y__53	= r_y__52;
		c_z__53	= r_z__52;
		c_ux__53	= r_ux__52;
		c_uy__53	= r_uy__52;
		c_uz__53	= r_uz__52;
		c_sz__53	= r_sz__52;
		c_sr__53	= r_sr__52;
		c_sleftz__53	= r_sleftz__52;
		c_sleftr__53	= r_sleftr__52;
		c_weight__53	= r_weight__52;
		c_layer__53	= r_layer__52;
		c_dead__53	= r_dead__52;
		c_hit__53	= r_hit__52;
		c_diff__53 = r_diff__52;
		c_dl_b__53 = r_dl_b__52;
		c_numer__53 = r_numer__52;
		c_z1__53 = r_z1__52;
		c_z0__53 = r_z0__52;
		c_mut__53 = r_mut__52;
		
		//for 54
		c_x__54	= r_x__53;
		c_y__54	= r_y__53;
		c_z__54	= r_z__53;
		c_ux__54	= r_ux__53;
		c_uy__54	= r_uy__53;
		c_uz__54	= r_uz__53;
		c_sz__54	= r_sz__53;
		c_sr__54	= r_sr__53;
		c_sleftz__54	= r_sleftz__53;
		c_sleftr__54	= r_sleftr__53;
		c_weight__54	= r_weight__53;
		c_layer__54	= r_layer__53;
		c_dead__54	= r_dead__53;
		c_hit__54	= r_hit__53;
		c_diff__54 = r_diff__53;
		c_dl_b__54 = r_dl_b__53;
		c_numer__54 = r_numer__53;
		c_z1__54 = r_z1__53;
		c_z0__54 = r_z0__53;
		c_mut__54 = r_mut__53;
		
		//for 55
		c_x__55	= r_x__54;
		c_y__55	= r_y__54;
		c_z__55	= r_z__54;
		c_ux__55	= r_ux__54;
		c_uy__55	= r_uy__54;
		c_uz__55	= r_uz__54;
		c_sz__55	= r_sz__54;
		c_sr__55	= r_sr__54;
		c_sleftz__55	= r_sleftz__54;
		c_sleftr__55	= r_sleftr__54;
		c_weight__55	= r_weight__54;
		c_layer__55	= r_layer__54;
		c_dead__55	= r_dead__54;
		c_hit__55	= r_hit__54;
		c_diff__55 = r_diff__54;
		c_dl_b__55 = r_dl_b__54;
		c_numer__55 = r_numer__54;
		c_z1__55 = r_z1__54;
		c_z0__55 = r_z0__54;
		c_mut__55 = r_mut__54;
		
		//for 56
		c_x__56	= r_x__55;
		c_y__56	= r_y__55;
		c_z__56	= r_z__55;
		c_ux__56	= r_ux__55;
		c_uy__56	= r_uy__55;
		c_uz__56	= r_uz__55;
		c_sz__56	= r_sz__55;
		c_sr__56	= r_sr__55;
		c_sleftz__56	= r_sleftz__55;
		c_sleftr__56	= r_sleftr__55;
		c_weight__56	= r_weight__55;
		c_layer__56	= r_layer__55;
		c_dead__56	= r_dead__55;
		c_hit__56	= r_hit__55;
		c_diff__56 = r_diff__55;
		c_dl_b__56 = r_dl_b__55;
		c_numer__56 = r_numer__55;
		c_z1__56 = r_z1__55;
		c_z0__56 = r_z0__55;
		c_mut__56 = r_mut__55;
		
		//for 57
		c_x__57	= r_x__56;
		c_y__57	= r_y__56;
		c_z__57	= r_z__56;
		c_ux__57	= r_ux__56;
		c_uy__57	= r_uy__56;
		c_uz__57	= r_uz__56;
		c_sz__57	= r_sz__56;
		c_sr__57	= r_sr__56;
		c_sleftz__57	= r_sleftz__56;
		c_sleftr__57	= r_sleftr__56;
		c_weight__57	= r_weight__56;
		c_layer__57	= r_layer__56;
		c_dead__57	= r_dead__56;
		c_hit__57	= r_hit__56;
		c_diff__57 = r_diff__56;
		c_dl_b__57 = r_dl_b__56;
		c_numer__57 = r_numer__56;
		c_z1__57 = r_z1__56;
		c_z0__57 = r_z0__56;
		c_mut__57 = r_mut__56;
		
		//for 58
		c_x__58	= r_x__57;
		c_y__58	= r_y__57;
		c_z__58	= r_z__57;
		c_ux__58	= r_ux__57;
		c_uy__58	= r_uy__57;
		c_uz__58	= r_uz__57;
		c_sz__58	= r_sz__57;
		c_sr__58	= r_sr__57;
		c_sleftz__58	= r_sleftz__57;
		c_sleftr__58	= r_sleftr__57;
		c_weight__58	= r_weight__57;
		c_layer__58	= r_layer__57;
		c_dead__58	= r_dead__57;
		c_hit__58	= r_hit__57;
		c_diff__58 = r_diff__57;
		c_dl_b__58 = r_dl_b__57;
		c_numer__58 = r_numer__57;
		c_z1__58 = r_z1__57;
		c_z0__58 = r_z0__57;
		c_mut__58 = r_mut__57;
		
		//for 59
		c_x__59	= r_x__58;
		c_y__59	= r_y__58;
		c_z__59	= r_z__58;
		c_ux__59	= r_ux__58;
		c_uy__59	= r_uy__58;
		c_uz__59	= r_uz__58;
		c_sz__59	= r_sz__58;
		c_sr__59	= r_sr__58;
		c_sleftz__59	= r_sleftz__58;
		c_sleftr__59	= r_sleftr__58;
		c_weight__59	= r_weight__58;
		c_layer__59	= r_layer__58;
		c_dead__59	= r_dead__58;
		c_hit__59	= r_hit__58;
		c_diff__59 = r_diff__58;
		c_dl_b__59 = r_dl_b__58;
		c_numer__59 = r_numer__58;
		c_z1__59 = r_z1__58;
		c_z0__59 = r_z0__58;
		c_mut__59 = r_mut__58;
	

	// Pull out and replace signals in pipe
	/* STAGE 1: Division completed */
	c_dl_b__30 = quotient_div1[32:1];
	c_diff__30 = c_sz__30 - c_dl_b__30;

	if(c_uz__30 != 32'b0 && c_sz__30 > c_dl_b__30 && quotient_div1[63:32] == 32'b0)
	begin
		/* not horizontal & crossing. */
		c_hit__30 = 1'b1;
	end
	//Remove blocking on c_hit__30
	else
	begin
		c_hit__30	= r_hit__29;
	end

	/* STAGE 2: First multiply completed */
	if(c_hit__31 == 1'b1)
	begin
		/*step left = (original step - distance travelled) * scaling factor*/

		c_sleftz__31 = sleftz_big[2*`BIT_WIDTH-2:`BIT_WIDTH - 1];
		if(c_uz__31[`BIT_WIDTH-1] == 1'b0) 
		begin
			c_z__31 = c_z1__31;
		end
		else
		begin
			c_z__31 = c_z0__31;
		end

		c_sz__31 = c_dl_b__31;
		c_sr__31 = sr_big[2*`BIT_WIDTH-2 - `ASPECT_RATIO:`BIT_WIDTH - 1 - `ASPECT_RATIO];
	end
	//Remove blocking on c_sleftz_31, c_sr__31, c_sz__31, c_z__31
	else 
	begin
		c_sleftz__31 = r_sleftz__30;
		c_sr__31 = r_sr__30;
		c_sz__31 = r_sz__30;
		c_z__31	= r_z__30;
	end

	/* STAGE 3: Second multiply completed */
	if(c_hit__32 == 1'b1)
	begin
		/*additional scaling factor on dl_b to switch to r-dimension scale*/
		c_sleftr__32 = sleftr_big[2*`BIT_WIDTH-2 - `ASPECT_RATIO:`BIT_WIDTH - 1 - `ASPECT_RATIO];
	end
	//Remove blocking on c_sleftr__32
	else
	begin
		c_sleftr__32 = r_sleftr__31;
	
	end
end

// latch values
always @ (posedge clock)
begin
//	for(j = 0; j < `TOTAL_LATENCY; j = j + 1)
//	begin
//		if (reset)
//		begin
//			r_x[j]	<= 32'b0;
//			r_y[j]	<= 32'b0;
//			r_z[j]	<= 32'b0;
//			r_ux[j]	<= 32'b0;
//			r_uy[j]	<= 32'b0;
//			r_uz[j]	<= 32'b0;
//			r_sz[j]	<= 32'b0;
//			r_sr[j]	<= 32'b0;
//			r_sleftz[j]	<= 32'b0;
//			r_sleftr[j]	<= 32'b0;
//			r_weight[j]	<= 32'b0;
//			r_layer[j]	<= 3'b0;
//			r_dead[j]	<= 1'b1;
//			r_hit[j]	<= 1'b0;
//			r_diff[j] <= 32'b0;
//			r_dl_b[j] <= 32'b0;
//			r_numer[j] <= 64'b0;
//			r_z1[j] <= 32'b0;
//			r_z0[j] <= 32'b0;
//			r_mut[j] <= 32'b0;
//		end
//		else
//		begin
//			if(enable)
//			begin
//				r_x[j]	<= c_x[j];
//				r_y[j]	<= c_y[j];
//				r_z[j]	<= c_z[j];
//				r_ux[j]	<= c_ux[j];
//				r_uy[j]	<= c_uy[j];
//				r_uz[j]	<= c_uz[j];
//				r_sz[j]	<= c_sz[j];
//				r_sr[j]	<= c_sr[j];
//				r_sleftz[j]	<= c_sleftz[j];
//				r_sleftr[j]	<= c_sleftr[j];
//				r_weight[j]	<= c_weight[j];
//				r_layer[j]	<= c_layer[j];
//				r_dead[j]	<= c_dead[j];
//				r_hit[j]	<= c_hit[j];
//				r_diff[j] <= c_diff[j];
//				r_dl_b[j] <= c_dl_b[j];
//				r_numer[j] <= c_numer[j];
//				r_z1[j] <= c_z1[j];
//				r_z0[j] <= c_z0[j];
//				r_mut[j] <= c_mut[j];
//			end
//		end
//	end
	if(reset)
	begin
		//Instantiate all 60 aspects of loop
			r_x__59	<= 32'b00000000000000000000000000000000;
			r_y__59	<= 32'b00000000000000000000000000000000;
			r_z__59	<= 32'b00000000000000000000000000000000;
			r_ux__59	<= 32'b00000000000000000000000000000000;
			r_uy__59	<= 32'b00000000000000000000000000000000;
			r_uz__59	<= 32'b00000000000000000000000000000000;
			r_sz__59	<= 32'b00000000000000000000000000000000;
			r_sr__59	<= 32'b00000000000000000000000000000000;
			r_sleftz__59	<= 32'b00000000000000000000000000000000;
			r_sleftr__59	<= 32'b00000000000000000000000000000000;
			r_weight__59	<= 32'b00000000000000000000000000000000;
			r_layer__59	<= 3'b000;
			r_dead__59	<= 1'b1;
			r_hit__59	<= 1'b0;
			r_diff__59 <= 32'b00000000000000000000000000000000;
			r_dl_b__59 <= 32'b00000000000000000000000000000000;
			r_numer__59 <= 0;
			r_z1__59 <= 32'b00000000000000000000000000000000;
			r_z0__59 <= 32'b00000000000000000000000000000000;
			r_mut__59 <= 32'b00000000000000000000000000000000;

			r_x__58	<= 32'b00000000000000000000000000000000;
			r_y__58	<= 32'b00000000000000000000000000000000;
			r_z__58	<= 32'b00000000000000000000000000000000;
			r_ux__58	<= 32'b00000000000000000000000000000000;
			r_uy__58	<= 32'b00000000000000000000000000000000;
			r_uz__58	<= 32'b00000000000000000000000000000000;
			r_sz__58	<= 32'b00000000000000000000000000000000;
			r_sr__58	<= 32'b00000000000000000000000000000000;
			r_sleftz__58	<= 32'b00000000000000000000000000000000;
			r_sleftr__58	<= 32'b00000000000000000000000000000000;
			r_weight__58	<= 32'b00000000000000000000000000000000;
			r_layer__58	<= 3'b000;
			r_dead__58	<= 1'b1;
			r_hit__58	<= 1'b0;
			r_diff__58 <= 32'b00000000000000000000000000000000;
			r_dl_b__58 <= 32'b00000000000000000000000000000000;
			r_numer__58 <= 0;
			r_z1__58 <= 32'b00000000000000000000000000000000;
			r_z0__58 <= 32'b00000000000000000000000000000000;
			r_mut__58 <= 32'b00000000000000000000000000000000;

			r_x__57	<= 32'b00000000000000000000000000000000;
			r_y__57	<= 32'b00000000000000000000000000000000;
			r_z__57	<= 32'b00000000000000000000000000000000;
			r_ux__57	<= 32'b00000000000000000000000000000000;
			r_uy__57	<= 32'b00000000000000000000000000000000;
			r_uz__57	<= 32'b00000000000000000000000000000000;
			r_sz__57	<= 32'b00000000000000000000000000000000;
			r_sr__57	<= 32'b00000000000000000000000000000000;
			r_sleftz__57	<= 32'b00000000000000000000000000000000;
			r_sleftr__57	<= 32'b00000000000000000000000000000000;
			r_weight__57	<= 32'b00000000000000000000000000000000;
			r_layer__57	<= 3'b000;
			r_dead__57	<= 1'b1;
			r_hit__57	<= 1'b0;
			r_diff__57 <= 32'b00000000000000000000000000000000;
			r_dl_b__57 <= 32'b00000000000000000000000000000000;
			r_numer__57 <= 0;
			r_z1__57 <= 32'b00000000000000000000000000000000;
			r_z0__57 <= 32'b00000000000000000000000000000000;
			r_mut__57 <= 32'b00000000000000000000000000000000;

			r_x__56	<= 32'b00000000000000000000000000000000;
			r_y__56	<= 32'b00000000000000000000000000000000;
			r_z__56	<= 32'b00000000000000000000000000000000;
			r_ux__56	<= 32'b00000000000000000000000000000000;
			r_uy__56	<= 32'b00000000000000000000000000000000;
			r_uz__56	<= 32'b00000000000000000000000000000000;
			r_sz__56	<= 32'b00000000000000000000000000000000;
			r_sr__56	<= 32'b00000000000000000000000000000000;
			r_sleftz__56	<= 32'b00000000000000000000000000000000;
			r_sleftr__56	<= 32'b00000000000000000000000000000000;
			r_weight__56	<= 32'b00000000000000000000000000000000;
			r_layer__56	<= 3'b000;
			r_dead__56	<= 1'b1;
			r_hit__56	<= 1'b0;
			r_diff__56 <= 32'b00000000000000000000000000000000;
			r_dl_b__56 <= 32'b00000000000000000000000000000000;
			r_numer__56 <= 0;
			r_z1__56 <= 32'b00000000000000000000000000000000;
			r_z0__56 <= 32'b00000000000000000000000000000000;
			r_mut__56 <= 32'b00000000000000000000000000000000;

			r_x__55	<= 32'b00000000000000000000000000000000;
			r_y__55	<= 32'b00000000000000000000000000000000;
			r_z__55	<= 32'b00000000000000000000000000000000;
			r_ux__55	<= 32'b00000000000000000000000000000000;
			r_uy__55	<= 32'b00000000000000000000000000000000;
			r_uz__55	<= 32'b00000000000000000000000000000000;
			r_sz__55	<= 32'b00000000000000000000000000000000;
			r_sr__55	<= 32'b00000000000000000000000000000000;
			r_sleftz__55	<= 32'b00000000000000000000000000000000;
			r_sleftr__55	<= 32'b00000000000000000000000000000000;
			r_weight__55	<= 32'b00000000000000000000000000000000;
			r_layer__55	<= 3'b000;
			r_dead__55	<= 1'b1;
			r_hit__55	<= 1'b0;
			r_diff__55 <= 32'b00000000000000000000000000000000;
			r_dl_b__55 <= 32'b00000000000000000000000000000000;
			r_numer__55 <= 0;
			r_z1__55 <= 32'b00000000000000000000000000000000;
			r_z0__55 <= 32'b00000000000000000000000000000000;
			r_mut__55 <= 32'b00000000000000000000000000000000;

			r_x__54	<= 32'b00000000000000000000000000000000;
			r_y__54	<= 32'b00000000000000000000000000000000;
			r_z__54	<= 32'b00000000000000000000000000000000;
			r_ux__54	<= 32'b00000000000000000000000000000000;
			r_uy__54	<= 32'b00000000000000000000000000000000;
			r_uz__54	<= 32'b00000000000000000000000000000000;
			r_sz__54	<= 32'b00000000000000000000000000000000;
			r_sr__54	<= 32'b00000000000000000000000000000000;
			r_sleftz__54	<= 32'b00000000000000000000000000000000;
			r_sleftr__54	<= 32'b00000000000000000000000000000000;
			r_weight__54	<= 32'b00000000000000000000000000000000;
			r_layer__54	<= 3'b000;
			r_dead__54	<= 1'b1;
			r_hit__54	<= 1'b0;
			r_diff__54 <= 32'b00000000000000000000000000000000;
			r_dl_b__54 <= 32'b00000000000000000000000000000000;
			r_numer__54 <= 0;
			r_z1__54 <= 32'b00000000000000000000000000000000;
			r_z0__54 <= 32'b00000000000000000000000000000000;
			r_mut__54 <= 32'b00000000000000000000000000000000;

			r_x__53	<= 32'b00000000000000000000000000000000;
			r_y__53	<= 32'b00000000000000000000000000000000;
			r_z__53	<= 32'b00000000000000000000000000000000;
			r_ux__53	<= 32'b00000000000000000000000000000000;
			r_uy__53	<= 32'b00000000000000000000000000000000;
			r_uz__53	<= 32'b00000000000000000000000000000000;
			r_sz__53	<= 32'b00000000000000000000000000000000;
			r_sr__53	<= 32'b00000000000000000000000000000000;
			r_sleftz__53	<= 32'b00000000000000000000000000000000;
			r_sleftr__53	<= 32'b00000000000000000000000000000000;
			r_weight__53	<= 32'b00000000000000000000000000000000;
			r_layer__53	<= 3'b000;
			r_dead__53	<= 1'b1;
			r_hit__53	<= 1'b0;
			r_diff__53 <= 32'b00000000000000000000000000000000;
			r_dl_b__53 <= 32'b00000000000000000000000000000000;
			r_numer__53 <= 0;
			r_z1__53 <= 32'b00000000000000000000000000000000;
			r_z0__53 <= 32'b00000000000000000000000000000000;
			r_mut__53 <= 32'b00000000000000000000000000000000;

			r_x__52	<= 32'b00000000000000000000000000000000;
			r_y__52	<= 32'b00000000000000000000000000000000;
			r_z__52	<= 32'b00000000000000000000000000000000;
			r_ux__52	<= 32'b00000000000000000000000000000000;
			r_uy__52	<= 32'b00000000000000000000000000000000;
			r_uz__52	<= 32'b00000000000000000000000000000000;
			r_sz__52	<= 32'b00000000000000000000000000000000;
			r_sr__52	<= 32'b00000000000000000000000000000000;
			r_sleftz__52	<= 32'b00000000000000000000000000000000;
			r_sleftr__52	<= 32'b00000000000000000000000000000000;
			r_weight__52	<= 32'b00000000000000000000000000000000;
			r_layer__52	<= 3'b000;
			r_dead__52	<= 1'b1;
			r_hit__52	<= 1'b0;
			r_diff__52 <= 32'b00000000000000000000000000000000;
			r_dl_b__52 <= 32'b00000000000000000000000000000000;
			r_numer__52 <= 0;
			r_z1__52 <= 32'b00000000000000000000000000000000;
			r_z0__52 <= 32'b00000000000000000000000000000000;
			r_mut__52 <= 32'b00000000000000000000000000000000;

			r_x__51	<= 32'b00000000000000000000000000000000;
			r_y__51	<= 32'b00000000000000000000000000000000;
			r_z__51	<= 32'b00000000000000000000000000000000;
			r_ux__51	<= 32'b00000000000000000000000000000000;
			r_uy__51	<= 32'b00000000000000000000000000000000;
			r_uz__51	<= 32'b00000000000000000000000000000000;
			r_sz__51	<= 32'b00000000000000000000000000000000;
			r_sr__51	<= 32'b00000000000000000000000000000000;
			r_sleftz__51	<= 32'b00000000000000000000000000000000;
			r_sleftr__51	<= 32'b00000000000000000000000000000000;
			r_weight__51	<= 32'b00000000000000000000000000000000;
			r_layer__51	<= 3'b000;
			r_dead__51	<= 1'b1;
			r_hit__51	<= 1'b0;
			r_diff__51 <= 32'b00000000000000000000000000000000;
			r_dl_b__51 <= 32'b00000000000000000000000000000000;
			r_numer__51 <= 0;
			r_z1__51 <= 32'b00000000000000000000000000000000;
			r_z0__51 <= 32'b00000000000000000000000000000000;
			r_mut__51 <= 32'b00000000000000000000000000000000;

			r_x__50	<= 32'b00000000000000000000000000000000;
			r_y__50	<= 32'b00000000000000000000000000000000;
			r_z__50	<= 32'b00000000000000000000000000000000;
			r_ux__50	<= 32'b00000000000000000000000000000000;
			r_uy__50	<= 32'b00000000000000000000000000000000;
			r_uz__50	<= 32'b00000000000000000000000000000000;
			r_sz__50	<= 32'b00000000000000000000000000000000;
			r_sr__50	<= 32'b00000000000000000000000000000000;
			r_sleftz__50	<= 32'b00000000000000000000000000000000;
			r_sleftr__50	<= 32'b00000000000000000000000000000000;
			r_weight__50	<= 32'b00000000000000000000000000000000;
			r_layer__50	<= 3'b000;
			r_dead__50	<= 1'b1;
			r_hit__50	<= 1'b0;
			r_diff__50 <= 32'b00000000000000000000000000000000;
			r_dl_b__50 <= 32'b00000000000000000000000000000000;
			r_numer__50 <= 0;
			r_z1__50 <= 32'b00000000000000000000000000000000;
			r_z0__50 <= 32'b00000000000000000000000000000000;
			r_mut__50 <= 32'b00000000000000000000000000000000;

			r_x__49	<= 32'b00000000000000000000000000000000;
			r_y__49	<= 32'b00000000000000000000000000000000;
			r_z__49	<= 32'b00000000000000000000000000000000;
			r_ux__49	<= 32'b00000000000000000000000000000000;
			r_uy__49	<= 32'b00000000000000000000000000000000;
			r_uz__49	<= 32'b00000000000000000000000000000000;
			r_sz__49	<= 32'b00000000000000000000000000000000;
			r_sr__49	<= 32'b00000000000000000000000000000000;
			r_sleftz__49	<= 32'b00000000000000000000000000000000;
			r_sleftr__49	<= 32'b00000000000000000000000000000000;
			r_weight__49	<= 32'b00000000000000000000000000000000;
			r_layer__49	<= 3'b000;
			r_dead__49	<= 1'b1;
			r_hit__49	<= 1'b0;
			r_diff__49 <= 32'b00000000000000000000000000000000;
			r_dl_b__49 <= 32'b00000000000000000000000000000000;
			r_numer__49 <= 0;
			r_z1__49 <= 32'b00000000000000000000000000000000;
			r_z0__49 <= 32'b00000000000000000000000000000000;
			r_mut__49 <= 32'b00000000000000000000000000000000;

			r_x__48	<= 32'b00000000000000000000000000000000;
			r_y__48	<= 32'b00000000000000000000000000000000;
			r_z__48	<= 32'b00000000000000000000000000000000;
			r_ux__48	<= 32'b00000000000000000000000000000000;
			r_uy__48	<= 32'b00000000000000000000000000000000;
			r_uz__48	<= 32'b00000000000000000000000000000000;
			r_sz__48	<= 32'b00000000000000000000000000000000;
			r_sr__48	<= 32'b00000000000000000000000000000000;
			r_sleftz__48	<= 32'b00000000000000000000000000000000;
			r_sleftr__48	<= 32'b00000000000000000000000000000000;
			r_weight__48	<= 32'b00000000000000000000000000000000;
			r_layer__48	<= 3'b000;
			r_dead__48	<= 1'b1;
			r_hit__48	<= 1'b0;
			r_diff__48 <= 32'b00000000000000000000000000000000;
			r_dl_b__48 <= 32'b00000000000000000000000000000000;
			r_numer__48 <= 0;
			r_z1__48 <= 32'b00000000000000000000000000000000;
			r_z0__48 <= 32'b00000000000000000000000000000000;
			r_mut__48 <= 32'b00000000000000000000000000000000;

			r_x__47	<= 32'b00000000000000000000000000000000;
			r_y__47	<= 32'b00000000000000000000000000000000;
			r_z__47	<= 32'b00000000000000000000000000000000;
			r_ux__47	<= 32'b00000000000000000000000000000000;
			r_uy__47	<= 32'b00000000000000000000000000000000;
			r_uz__47	<= 32'b00000000000000000000000000000000;
			r_sz__47	<= 32'b00000000000000000000000000000000;
			r_sr__47	<= 32'b00000000000000000000000000000000;
			r_sleftz__47	<= 32'b00000000000000000000000000000000;
			r_sleftr__47	<= 32'b00000000000000000000000000000000;
			r_weight__47	<= 32'b00000000000000000000000000000000;
			r_layer__47	<= 3'b000;
			r_dead__47	<= 1'b1;
			r_hit__47	<= 1'b0;
			r_diff__47 <= 32'b00000000000000000000000000000000;
			r_dl_b__47 <= 32'b00000000000000000000000000000000;
			r_numer__47 <= 0;
			r_z1__47 <= 32'b00000000000000000000000000000000;
			r_z0__47 <= 32'b00000000000000000000000000000000;
			r_mut__47 <= 32'b00000000000000000000000000000000;

			r_x__46	<= 32'b00000000000000000000000000000000;
			r_y__46	<= 32'b00000000000000000000000000000000;
			r_z__46	<= 32'b00000000000000000000000000000000;
			r_ux__46	<= 32'b00000000000000000000000000000000;
			r_uy__46	<= 32'b00000000000000000000000000000000;
			r_uz__46	<= 32'b00000000000000000000000000000000;
			r_sz__46	<= 32'b00000000000000000000000000000000;
			r_sr__46	<= 32'b00000000000000000000000000000000;
			r_sleftz__46	<= 32'b00000000000000000000000000000000;
			r_sleftr__46	<= 32'b00000000000000000000000000000000;
			r_weight__46	<= 32'b00000000000000000000000000000000;
			r_layer__46	<= 3'b000;
			r_dead__46	<= 1'b1;
			r_hit__46	<= 1'b0;
			r_diff__46 <= 32'b00000000000000000000000000000000;
			r_dl_b__46 <= 32'b00000000000000000000000000000000;
			r_numer__46 <= 0;
			r_z1__46 <= 32'b00000000000000000000000000000000;
			r_z0__46 <= 32'b00000000000000000000000000000000;
			r_mut__46 <= 32'b00000000000000000000000000000000;

			r_x__45	<= 32'b00000000000000000000000000000000;
			r_y__45	<= 32'b00000000000000000000000000000000;
			r_z__45	<= 32'b00000000000000000000000000000000;
			r_ux__45	<= 32'b00000000000000000000000000000000;
			r_uy__45	<= 32'b00000000000000000000000000000000;
			r_uz__45	<= 32'b00000000000000000000000000000000;
			r_sz__45	<= 32'b00000000000000000000000000000000;
			r_sr__45	<= 32'b00000000000000000000000000000000;
			r_sleftz__45	<= 32'b00000000000000000000000000000000;
			r_sleftr__45	<= 32'b00000000000000000000000000000000;
			r_weight__45	<= 32'b00000000000000000000000000000000;
			r_layer__45	<= 3'b000;
			r_dead__45	<= 1'b1;
			r_hit__45	<= 1'b0;
			r_diff__45 <= 32'b00000000000000000000000000000000;
			r_dl_b__45 <= 32'b00000000000000000000000000000000;
			r_numer__45 <= 0;
			r_z1__45 <= 32'b00000000000000000000000000000000;
			r_z0__45 <= 32'b00000000000000000000000000000000;
			r_mut__45 <= 32'b00000000000000000000000000000000;

			r_x__44	<= 32'b00000000000000000000000000000000;
			r_y__44	<= 32'b00000000000000000000000000000000;
			r_z__44	<= 32'b00000000000000000000000000000000;
			r_ux__44	<= 32'b00000000000000000000000000000000;
			r_uy__44	<= 32'b00000000000000000000000000000000;
			r_uz__44	<= 32'b00000000000000000000000000000000;
			r_sz__44	<= 32'b00000000000000000000000000000000;
			r_sr__44	<= 32'b00000000000000000000000000000000;
			r_sleftz__44	<= 32'b00000000000000000000000000000000;
			r_sleftr__44	<= 32'b00000000000000000000000000000000;
			r_weight__44	<= 32'b00000000000000000000000000000000;
			r_layer__44	<= 3'b000;
			r_dead__44	<= 1'b1;
			r_hit__44	<= 1'b0;
			r_diff__44 <= 32'b00000000000000000000000000000000;
			r_dl_b__44 <= 32'b00000000000000000000000000000000;
			r_numer__44 <= 0;
			r_z1__44 <= 32'b00000000000000000000000000000000;
			r_z0__44 <= 32'b00000000000000000000000000000000;
			r_mut__44 <= 32'b00000000000000000000000000000000;

			r_x__43	<= 32'b00000000000000000000000000000000;
			r_y__43	<= 32'b00000000000000000000000000000000;
			r_z__43	<= 32'b00000000000000000000000000000000;
			r_ux__43	<= 32'b00000000000000000000000000000000;
			r_uy__43	<= 32'b00000000000000000000000000000000;
			r_uz__43	<= 32'b00000000000000000000000000000000;
			r_sz__43	<= 32'b00000000000000000000000000000000;
			r_sr__43	<= 32'b00000000000000000000000000000000;
			r_sleftz__43	<= 32'b00000000000000000000000000000000;
			r_sleftr__43	<= 32'b00000000000000000000000000000000;
			r_weight__43	<= 32'b00000000000000000000000000000000;
			r_layer__43	<= 3'b000;
			r_dead__43	<= 1'b1;
			r_hit__43	<= 1'b0;
			r_diff__43 <= 32'b00000000000000000000000000000000;
			r_dl_b__43 <= 32'b00000000000000000000000000000000;
			r_numer__43 <= 0;
			r_z1__43 <= 32'b00000000000000000000000000000000;
			r_z0__43 <= 32'b00000000000000000000000000000000;
			r_mut__43 <= 32'b00000000000000000000000000000000;

			r_x__42	<= 32'b00000000000000000000000000000000;
			r_y__42	<= 32'b00000000000000000000000000000000;
			r_z__42	<= 32'b00000000000000000000000000000000;
			r_ux__42	<= 32'b00000000000000000000000000000000;
			r_uy__42	<= 32'b00000000000000000000000000000000;
			r_uz__42	<= 32'b00000000000000000000000000000000;
			r_sz__42	<= 32'b00000000000000000000000000000000;
			r_sr__42	<= 32'b00000000000000000000000000000000;
			r_sleftz__42	<= 32'b00000000000000000000000000000000;
			r_sleftr__42	<= 32'b00000000000000000000000000000000;
			r_weight__42	<= 32'b00000000000000000000000000000000;
			r_layer__42	<= 3'b000;
			r_dead__42	<= 1'b1;
			r_hit__42	<= 1'b0;
			r_diff__42 <= 32'b00000000000000000000000000000000;
			r_dl_b__42 <= 32'b00000000000000000000000000000000;
			r_numer__42 <= 0;
			r_z1__42 <= 32'b00000000000000000000000000000000;
			r_z0__42 <= 32'b00000000000000000000000000000000;
			r_mut__42 <= 32'b00000000000000000000000000000000;

			r_x__41	<= 32'b00000000000000000000000000000000;
			r_y__41	<= 32'b00000000000000000000000000000000;
			r_z__41	<= 32'b00000000000000000000000000000000;
			r_ux__41	<= 32'b00000000000000000000000000000000;
			r_uy__41	<= 32'b00000000000000000000000000000000;
			r_uz__41	<= 32'b00000000000000000000000000000000;
			r_sz__41	<= 32'b00000000000000000000000000000000;
			r_sr__41	<= 32'b00000000000000000000000000000000;
			r_sleftz__41	<= 32'b00000000000000000000000000000000;
			r_sleftr__41	<= 32'b00000000000000000000000000000000;
			r_weight__41	<= 32'b00000000000000000000000000000000;
			r_layer__41	<= 3'b000;
			r_dead__41	<= 1'b1;
			r_hit__41	<= 1'b0;
			r_diff__41 <= 32'b00000000000000000000000000000000;
			r_dl_b__41 <= 32'b00000000000000000000000000000000;
			r_numer__41 <= 0;
			r_z1__41 <= 32'b00000000000000000000000000000000;
			r_z0__41 <= 32'b00000000000000000000000000000000;
			r_mut__41 <= 32'b00000000000000000000000000000000;

			r_x__40	<= 32'b00000000000000000000000000000000;
			r_y__40	<= 32'b00000000000000000000000000000000;
			r_z__40	<= 32'b00000000000000000000000000000000;
			r_ux__40	<= 32'b00000000000000000000000000000000;
			r_uy__40	<= 32'b00000000000000000000000000000000;
			r_uz__40	<= 32'b00000000000000000000000000000000;
			r_sz__40	<= 32'b00000000000000000000000000000000;
			r_sr__40	<= 32'b00000000000000000000000000000000;
			r_sleftz__40	<= 32'b00000000000000000000000000000000;
			r_sleftr__40	<= 32'b00000000000000000000000000000000;
			r_weight__40	<= 32'b00000000000000000000000000000000;
			r_layer__40	<= 3'b000;
			r_dead__40	<= 1'b1;
			r_hit__40	<= 1'b0;
			r_diff__40 <= 32'b00000000000000000000000000000000;
			r_dl_b__40 <= 32'b00000000000000000000000000000000;
			r_numer__40 <= 0;
			r_z1__40 <= 32'b00000000000000000000000000000000;
			r_z0__40 <= 32'b00000000000000000000000000000000;
			r_mut__40 <= 32'b00000000000000000000000000000000;

			r_x__39	<= 32'b00000000000000000000000000000000;
			r_y__39	<= 32'b00000000000000000000000000000000;
			r_z__39	<= 32'b00000000000000000000000000000000;
			r_ux__39	<= 32'b00000000000000000000000000000000;
			r_uy__39	<= 32'b00000000000000000000000000000000;
			r_uz__39	<= 32'b00000000000000000000000000000000;
			r_sz__39	<= 32'b00000000000000000000000000000000;
			r_sr__39	<= 32'b00000000000000000000000000000000;
			r_sleftz__39	<= 32'b00000000000000000000000000000000;
			r_sleftr__39	<= 32'b00000000000000000000000000000000;
			r_weight__39	<= 32'b00000000000000000000000000000000;
			r_layer__39	<= 3'b000;
			r_dead__39	<= 1'b1;
			r_hit__39	<= 1'b0;
			r_diff__39 <= 32'b00000000000000000000000000000000;
			r_dl_b__39 <= 32'b00000000000000000000000000000000;
			r_numer__39 <= 0;
			r_z1__39 <= 32'b00000000000000000000000000000000;
			r_z0__39 <= 32'b00000000000000000000000000000000;
			r_mut__39 <= 32'b00000000000000000000000000000000;

			r_x__38	<= 32'b00000000000000000000000000000000;
			r_y__38	<= 32'b00000000000000000000000000000000;
			r_z__38	<= 32'b00000000000000000000000000000000;
			r_ux__38	<= 32'b00000000000000000000000000000000;
			r_uy__38	<= 32'b00000000000000000000000000000000;
			r_uz__38	<= 32'b00000000000000000000000000000000;
			r_sz__38	<= 32'b00000000000000000000000000000000;
			r_sr__38	<= 32'b00000000000000000000000000000000;
			r_sleftz__38	<= 32'b00000000000000000000000000000000;
			r_sleftr__38	<= 32'b00000000000000000000000000000000;
			r_weight__38	<= 32'b00000000000000000000000000000000;
			r_layer__38	<= 3'b000;
			r_dead__38	<= 1'b1;
			r_hit__38	<= 1'b0;
			r_diff__38 <= 32'b00000000000000000000000000000000;
			r_dl_b__38 <= 32'b00000000000000000000000000000000;
			r_numer__38 <= 0;
			r_z1__38 <= 32'b00000000000000000000000000000000;
			r_z0__38 <= 32'b00000000000000000000000000000000;
			r_mut__38 <= 32'b00000000000000000000000000000000;

			r_x__37	<= 32'b00000000000000000000000000000000;
			r_y__37	<= 32'b00000000000000000000000000000000;
			r_z__37	<= 32'b00000000000000000000000000000000;
			r_ux__37	<= 32'b00000000000000000000000000000000;
			r_uy__37	<= 32'b00000000000000000000000000000000;
			r_uz__37	<= 32'b00000000000000000000000000000000;
			r_sz__37	<= 32'b00000000000000000000000000000000;
			r_sr__37	<= 32'b00000000000000000000000000000000;
			r_sleftz__37	<= 32'b00000000000000000000000000000000;
			r_sleftr__37	<= 32'b00000000000000000000000000000000;
			r_weight__37	<= 32'b00000000000000000000000000000000;
			r_layer__37	<= 3'b000;
			r_dead__37	<= 1'b1;
			r_hit__37	<= 1'b0;
			r_diff__37 <= 32'b00000000000000000000000000000000;
			r_dl_b__37 <= 32'b00000000000000000000000000000000;
			r_numer__37 <= 0;
			r_z1__37 <= 32'b00000000000000000000000000000000;
			r_z0__37 <= 32'b00000000000000000000000000000000;
			r_mut__37 <= 32'b00000000000000000000000000000000;

			r_x__36	<= 32'b00000000000000000000000000000000;
			r_y__36	<= 32'b00000000000000000000000000000000;
			r_z__36	<= 32'b00000000000000000000000000000000;
			r_ux__36	<= 32'b00000000000000000000000000000000;
			r_uy__36	<= 32'b00000000000000000000000000000000;
			r_uz__36	<= 32'b00000000000000000000000000000000;
			r_sz__36	<= 32'b00000000000000000000000000000000;
			r_sr__36	<= 32'b00000000000000000000000000000000;
			r_sleftz__36	<= 32'b00000000000000000000000000000000;
			r_sleftr__36	<= 32'b00000000000000000000000000000000;
			r_weight__36	<= 32'b00000000000000000000000000000000;
			r_layer__36	<= 3'b000;
			r_dead__36	<= 1'b1;
			r_hit__36	<= 1'b0;
			r_diff__36 <= 32'b00000000000000000000000000000000;
			r_dl_b__36 <= 32'b00000000000000000000000000000000;
			r_numer__36 <= 0;
			r_z1__36 <= 32'b00000000000000000000000000000000;
			r_z0__36 <= 32'b00000000000000000000000000000000;
			r_mut__36 <= 32'b00000000000000000000000000000000;

			r_x__35	<= 32'b00000000000000000000000000000000;
			r_y__35	<= 32'b00000000000000000000000000000000;
			r_z__35	<= 32'b00000000000000000000000000000000;
			r_ux__35	<= 32'b00000000000000000000000000000000;
			r_uy__35	<= 32'b00000000000000000000000000000000;
			r_uz__35	<= 32'b00000000000000000000000000000000;
			r_sz__35	<= 32'b00000000000000000000000000000000;
			r_sr__35	<= 32'b00000000000000000000000000000000;
			r_sleftz__35	<= 32'b00000000000000000000000000000000;
			r_sleftr__35	<= 32'b00000000000000000000000000000000;
			r_weight__35	<= 32'b00000000000000000000000000000000;
			r_layer__35	<= 3'b000;
			r_dead__35	<= 1'b1;
			r_hit__35	<= 1'b0;
			r_diff__35 <= 32'b00000000000000000000000000000000;
			r_dl_b__35 <= 32'b00000000000000000000000000000000;
			r_numer__35 <= 0;
			r_z1__35 <= 32'b00000000000000000000000000000000;
			r_z0__35 <= 32'b00000000000000000000000000000000;
			r_mut__35 <= 32'b00000000000000000000000000000000;

			r_x__34	<= 32'b00000000000000000000000000000000;
			r_y__34	<= 32'b00000000000000000000000000000000;
			r_z__34	<= 32'b00000000000000000000000000000000;
			r_ux__34	<= 32'b00000000000000000000000000000000;
			r_uy__34	<= 32'b00000000000000000000000000000000;
			r_uz__34	<= 32'b00000000000000000000000000000000;
			r_sz__34	<= 32'b00000000000000000000000000000000;
			r_sr__34	<= 32'b00000000000000000000000000000000;
			r_sleftz__34	<= 32'b00000000000000000000000000000000;
			r_sleftr__34	<= 32'b00000000000000000000000000000000;
			r_weight__34	<= 32'b00000000000000000000000000000000;
			r_layer__34	<= 3'b000;
			r_dead__34	<= 1'b1;
			r_hit__34	<= 1'b0;
			r_diff__34 <= 32'b00000000000000000000000000000000;
			r_dl_b__34 <= 32'b00000000000000000000000000000000;
			r_numer__34 <= 0;
			r_z1__34 <= 32'b00000000000000000000000000000000;
			r_z0__34 <= 32'b00000000000000000000000000000000;
			r_mut__34 <= 32'b00000000000000000000000000000000;

			r_x__33	<= 32'b00000000000000000000000000000000;
			r_y__33	<= 32'b00000000000000000000000000000000;
			r_z__33	<= 32'b00000000000000000000000000000000;
			r_ux__33	<= 32'b00000000000000000000000000000000;
			r_uy__33	<= 32'b00000000000000000000000000000000;
			r_uz__33	<= 32'b00000000000000000000000000000000;
			r_sz__33	<= 32'b00000000000000000000000000000000;
			r_sr__33	<= 32'b00000000000000000000000000000000;
			r_sleftz__33	<= 32'b00000000000000000000000000000000;
			r_sleftr__33	<= 32'b00000000000000000000000000000000;
			r_weight__33	<= 32'b00000000000000000000000000000000;
			r_layer__33	<= 3'b000;
			r_dead__33	<= 1'b1;
			r_hit__33	<= 1'b0;
			r_diff__33 <= 32'b00000000000000000000000000000000;
			r_dl_b__33 <= 32'b00000000000000000000000000000000;
			r_numer__33 <= 0;
			r_z1__33 <= 32'b00000000000000000000000000000000;
			r_z0__33 <= 32'b00000000000000000000000000000000;
			r_mut__33 <= 32'b00000000000000000000000000000000;

			r_x__32	<= 32'b00000000000000000000000000000000;
			r_y__32	<= 32'b00000000000000000000000000000000;
			r_z__32	<= 32'b00000000000000000000000000000000;
			r_ux__32	<= 32'b00000000000000000000000000000000;
			r_uy__32	<= 32'b00000000000000000000000000000000;
			r_uz__32	<= 32'b00000000000000000000000000000000;
			r_sz__32	<= 32'b00000000000000000000000000000000;
			r_sr__32	<= 32'b00000000000000000000000000000000;
			r_sleftz__32	<= 32'b00000000000000000000000000000000;
			r_sleftr__32	<= 32'b00000000000000000000000000000000;
			r_weight__32	<= 32'b00000000000000000000000000000000;
			r_layer__32	<= 3'b000;
			r_dead__32	<= 1'b1;
			r_hit__32	<= 1'b0;
			r_diff__32 <= 32'b00000000000000000000000000000000;
			r_dl_b__32 <= 32'b00000000000000000000000000000000;
			r_numer__32 <= 0;
			r_z1__32 <= 32'b00000000000000000000000000000000;
			r_z0__32 <= 32'b00000000000000000000000000000000;
			r_mut__32 <= 32'b00000000000000000000000000000000;

			r_x__31	<= 32'b00000000000000000000000000000000;
			r_y__31	<= 32'b00000000000000000000000000000000;
			r_z__31	<= 32'b00000000000000000000000000000000;
			r_ux__31	<= 32'b00000000000000000000000000000000;
			r_uy__31	<= 32'b00000000000000000000000000000000;
			r_uz__31	<= 32'b00000000000000000000000000000000;
			r_sz__31	<= 32'b00000000000000000000000000000000;
			r_sr__31	<= 32'b00000000000000000000000000000000;
			r_sleftz__31	<= 32'b00000000000000000000000000000000;
			r_sleftr__31	<= 32'b00000000000000000000000000000000;
			r_weight__31	<= 32'b00000000000000000000000000000000;
			r_layer__31	<= 3'b000;
			r_dead__31	<= 1'b1;
			r_hit__31	<= 1'b0;
			r_diff__31 <= 32'b00000000000000000000000000000000;
			r_dl_b__31 <= 32'b00000000000000000000000000000000;
			r_numer__31 <= 0;
			r_z1__31 <= 32'b00000000000000000000000000000000;
			r_z0__31 <= 32'b00000000000000000000000000000000;
			r_mut__31 <= 32'b00000000000000000000000000000000;

			r_x__30	<= 32'b00000000000000000000000000000000;
			r_y__30	<= 32'b00000000000000000000000000000000;
			r_z__30	<= 32'b00000000000000000000000000000000;
			r_ux__30	<= 32'b00000000000000000000000000000000;
			r_uy__30	<= 32'b00000000000000000000000000000000;
			r_uz__30	<= 32'b00000000000000000000000000000000;
			r_sz__30	<= 32'b00000000000000000000000000000000;
			r_sr__30	<= 32'b00000000000000000000000000000000;
			r_sleftz__30	<= 32'b00000000000000000000000000000000;
			r_sleftr__30	<= 32'b00000000000000000000000000000000;
			r_weight__30	<= 32'b00000000000000000000000000000000;
			r_layer__30	<= 3'b000;
			r_dead__30	<= 1'b1;
			r_hit__30	<= 1'b0;
			r_diff__30 <= 32'b00000000000000000000000000000000;
			r_dl_b__30 <= 32'b00000000000000000000000000000000;
			r_numer__30 <= 0;
			r_z1__30 <= 32'b00000000000000000000000000000000;
			r_z0__30 <= 32'b00000000000000000000000000000000;
			r_mut__30 <= 32'b00000000000000000000000000000000;

			r_x__29	<= 32'b00000000000000000000000000000000;
			r_y__29	<= 32'b00000000000000000000000000000000;
			r_z__29	<= 32'b00000000000000000000000000000000;
			r_ux__29	<= 32'b00000000000000000000000000000000;
			r_uy__29	<= 32'b00000000000000000000000000000000;
			r_uz__29	<= 32'b00000000000000000000000000000000;
			r_sz__29	<= 32'b00000000000000000000000000000000;
			r_sr__29	<= 32'b00000000000000000000000000000000;
			r_sleftz__29	<= 32'b00000000000000000000000000000000;
			r_sleftr__29	<= 32'b00000000000000000000000000000000;
			r_weight__29	<= 32'b00000000000000000000000000000000;
			r_layer__29	<= 3'b000;
			r_dead__29	<= 1'b1;
			r_hit__29	<= 1'b0;
			r_diff__29 <= 32'b00000000000000000000000000000000;
			r_dl_b__29 <= 32'b00000000000000000000000000000000;
			r_numer__29 <= 0;
			r_z1__29 <= 32'b00000000000000000000000000000000;
			r_z0__29 <= 32'b00000000000000000000000000000000;
			r_mut__29 <= 32'b00000000000000000000000000000000;

			r_x__28	<= 32'b00000000000000000000000000000000;
			r_y__28	<= 32'b00000000000000000000000000000000;
			r_z__28	<= 32'b00000000000000000000000000000000;
			r_ux__28	<= 32'b00000000000000000000000000000000;
			r_uy__28	<= 32'b00000000000000000000000000000000;
			r_uz__28	<= 32'b00000000000000000000000000000000;
			r_sz__28	<= 32'b00000000000000000000000000000000;
			r_sr__28	<= 32'b00000000000000000000000000000000;
			r_sleftz__28	<= 32'b00000000000000000000000000000000;
			r_sleftr__28	<= 32'b00000000000000000000000000000000;
			r_weight__28	<= 32'b00000000000000000000000000000000;
			r_layer__28	<= 3'b000;
			r_dead__28	<= 1'b1;
			r_hit__28	<= 1'b0;
			r_diff__28 <= 32'b00000000000000000000000000000000;
			r_dl_b__28 <= 32'b00000000000000000000000000000000;
			r_numer__28 <= 0;
			r_z1__28 <= 32'b00000000000000000000000000000000;
			r_z0__28 <= 32'b00000000000000000000000000000000;
			r_mut__28 <= 32'b00000000000000000000000000000000;

			r_x__27	<= 32'b00000000000000000000000000000000;
			r_y__27	<= 32'b00000000000000000000000000000000;
			r_z__27	<= 32'b00000000000000000000000000000000;
			r_ux__27	<= 32'b00000000000000000000000000000000;
			r_uy__27	<= 32'b00000000000000000000000000000000;
			r_uz__27	<= 32'b00000000000000000000000000000000;
			r_sz__27	<= 32'b00000000000000000000000000000000;
			r_sr__27	<= 32'b00000000000000000000000000000000;
			r_sleftz__27	<= 32'b00000000000000000000000000000000;
			r_sleftr__27	<= 32'b00000000000000000000000000000000;
			r_weight__27	<= 32'b00000000000000000000000000000000;
			r_layer__27	<= 3'b000;
			r_dead__27	<= 1'b1;
			r_hit__27	<= 1'b0;
			r_diff__27 <= 32'b00000000000000000000000000000000;
			r_dl_b__27 <= 32'b00000000000000000000000000000000;
			r_numer__27 <= 0;
			r_z1__27 <= 32'b00000000000000000000000000000000;
			r_z0__27 <= 32'b00000000000000000000000000000000;
			r_mut__27 <= 32'b00000000000000000000000000000000;

			r_x__26	<= 32'b00000000000000000000000000000000;
			r_y__26	<= 32'b00000000000000000000000000000000;
			r_z__26	<= 32'b00000000000000000000000000000000;
			r_ux__26	<= 32'b00000000000000000000000000000000;
			r_uy__26	<= 32'b00000000000000000000000000000000;
			r_uz__26	<= 32'b00000000000000000000000000000000;
			r_sz__26	<= 32'b00000000000000000000000000000000;
			r_sr__26	<= 32'b00000000000000000000000000000000;
			r_sleftz__26	<= 32'b00000000000000000000000000000000;
			r_sleftr__26	<= 32'b00000000000000000000000000000000;
			r_weight__26	<= 32'b00000000000000000000000000000000;
			r_layer__26	<= 3'b000;
			r_dead__26	<= 1'b1;
			r_hit__26	<= 1'b0;
			r_diff__26 <= 32'b00000000000000000000000000000000;
			r_dl_b__26 <= 32'b00000000000000000000000000000000;
			r_numer__26 <= 0;
			r_z1__26 <= 32'b00000000000000000000000000000000;
			r_z0__26 <= 32'b00000000000000000000000000000000;
			r_mut__26 <= 32'b00000000000000000000000000000000;

			r_x__25	<= 32'b00000000000000000000000000000000;
			r_y__25	<= 32'b00000000000000000000000000000000;
			r_z__25	<= 32'b00000000000000000000000000000000;
			r_ux__25	<= 32'b00000000000000000000000000000000;
			r_uy__25	<= 32'b00000000000000000000000000000000;
			r_uz__25	<= 32'b00000000000000000000000000000000;
			r_sz__25	<= 32'b00000000000000000000000000000000;
			r_sr__25	<= 32'b00000000000000000000000000000000;
			r_sleftz__25	<= 32'b00000000000000000000000000000000;
			r_sleftr__25	<= 32'b00000000000000000000000000000000;
			r_weight__25	<= 32'b00000000000000000000000000000000;
			r_layer__25	<= 3'b000;
			r_dead__25	<= 1'b1;
			r_hit__25	<= 1'b0;
			r_diff__25 <= 32'b00000000000000000000000000000000;
			r_dl_b__25 <= 32'b00000000000000000000000000000000;
			r_numer__25 <= 0;
			r_z1__25 <= 32'b00000000000000000000000000000000;
			r_z0__25 <= 32'b00000000000000000000000000000000;
			r_mut__25 <= 32'b00000000000000000000000000000000;

			r_x__24	<= 32'b00000000000000000000000000000000;
			r_y__24	<= 32'b00000000000000000000000000000000;
			r_z__24	<= 32'b00000000000000000000000000000000;
			r_ux__24	<= 32'b00000000000000000000000000000000;
			r_uy__24	<= 32'b00000000000000000000000000000000;
			r_uz__24	<= 32'b00000000000000000000000000000000;
			r_sz__24	<= 32'b00000000000000000000000000000000;
			r_sr__24	<= 32'b00000000000000000000000000000000;
			r_sleftz__24	<= 32'b00000000000000000000000000000000;
			r_sleftr__24	<= 32'b00000000000000000000000000000000;
			r_weight__24	<= 32'b00000000000000000000000000000000;
			r_layer__24	<= 3'b000;
			r_dead__24	<= 1'b1;
			r_hit__24	<= 1'b0;
			r_diff__24 <= 32'b00000000000000000000000000000000;
			r_dl_b__24 <= 32'b00000000000000000000000000000000;
			r_numer__24 <= 0;
			r_z1__24 <= 32'b00000000000000000000000000000000;
			r_z0__24 <= 32'b00000000000000000000000000000000;
			r_mut__24 <= 32'b00000000000000000000000000000000;

			r_x__23	<= 32'b00000000000000000000000000000000;
			r_y__23	<= 32'b00000000000000000000000000000000;
			r_z__23	<= 32'b00000000000000000000000000000000;
			r_ux__23	<= 32'b00000000000000000000000000000000;
			r_uy__23	<= 32'b00000000000000000000000000000000;
			r_uz__23	<= 32'b00000000000000000000000000000000;
			r_sz__23	<= 32'b00000000000000000000000000000000;
			r_sr__23	<= 32'b00000000000000000000000000000000;
			r_sleftz__23	<= 32'b00000000000000000000000000000000;
			r_sleftr__23	<= 32'b00000000000000000000000000000000;
			r_weight__23	<= 32'b00000000000000000000000000000000;
			r_layer__23	<= 3'b000;
			r_dead__23	<= 1'b1;
			r_hit__23	<= 1'b0;
			r_diff__23 <= 32'b00000000000000000000000000000000;
			r_dl_b__23 <= 32'b00000000000000000000000000000000;
			r_numer__23 <= 0;
			r_z1__23 <= 32'b00000000000000000000000000000000;
			r_z0__23 <= 32'b00000000000000000000000000000000;
			r_mut__23 <= 32'b00000000000000000000000000000000;

			r_x__22	<= 32'b00000000000000000000000000000000;
			r_y__22	<= 32'b00000000000000000000000000000000;
			r_z__22	<= 32'b00000000000000000000000000000000;
			r_ux__22	<= 32'b00000000000000000000000000000000;
			r_uy__22	<= 32'b00000000000000000000000000000000;
			r_uz__22	<= 32'b00000000000000000000000000000000;
			r_sz__22	<= 32'b00000000000000000000000000000000;
			r_sr__22	<= 32'b00000000000000000000000000000000;
			r_sleftz__22	<= 32'b00000000000000000000000000000000;
			r_sleftr__22	<= 32'b00000000000000000000000000000000;
			r_weight__22	<= 32'b00000000000000000000000000000000;
			r_layer__22	<= 3'b000;
			r_dead__22	<= 1'b1;
			r_hit__22	<= 1'b0;
			r_diff__22 <= 32'b00000000000000000000000000000000;
			r_dl_b__22 <= 32'b00000000000000000000000000000000;
			r_numer__22 <= 0;
			r_z1__22 <= 32'b00000000000000000000000000000000;
			r_z0__22 <= 32'b00000000000000000000000000000000;
			r_mut__22 <= 32'b00000000000000000000000000000000;

			r_x__21	<= 32'b00000000000000000000000000000000;
			r_y__21	<= 32'b00000000000000000000000000000000;
			r_z__21	<= 32'b00000000000000000000000000000000;
			r_ux__21	<= 32'b00000000000000000000000000000000;
			r_uy__21	<= 32'b00000000000000000000000000000000;
			r_uz__21	<= 32'b00000000000000000000000000000000;
			r_sz__21	<= 32'b00000000000000000000000000000000;
			r_sr__21	<= 32'b00000000000000000000000000000000;
			r_sleftz__21	<= 32'b00000000000000000000000000000000;
			r_sleftr__21	<= 32'b00000000000000000000000000000000;
			r_weight__21	<= 32'b00000000000000000000000000000000;
			r_layer__21	<= 3'b000;
			r_dead__21	<= 1'b1;
			r_hit__21	<= 1'b0;
			r_diff__21 <= 32'b00000000000000000000000000000000;
			r_dl_b__21 <= 32'b00000000000000000000000000000000;
			r_numer__21 <= 0;
			r_z1__21 <= 32'b00000000000000000000000000000000;
			r_z0__21 <= 32'b00000000000000000000000000000000;
			r_mut__21 <= 32'b00000000000000000000000000000000;

			r_x__20	<= 32'b00000000000000000000000000000000;
			r_y__20	<= 32'b00000000000000000000000000000000;
			r_z__20	<= 32'b00000000000000000000000000000000;
			r_ux__20	<= 32'b00000000000000000000000000000000;
			r_uy__20	<= 32'b00000000000000000000000000000000;
			r_uz__20	<= 32'b00000000000000000000000000000000;
			r_sz__20	<= 32'b00000000000000000000000000000000;
			r_sr__20	<= 32'b00000000000000000000000000000000;
			r_sleftz__20	<= 32'b00000000000000000000000000000000;
			r_sleftr__20	<= 32'b00000000000000000000000000000000;
			r_weight__20	<= 32'b00000000000000000000000000000000;
			r_layer__20	<= 3'b000;
			r_dead__20	<= 1'b1;
			r_hit__20	<= 1'b0;
			r_diff__20 <= 32'b00000000000000000000000000000000;
			r_dl_b__20 <= 32'b00000000000000000000000000000000;
			r_numer__20 <= 0;
			r_z1__20 <= 32'b00000000000000000000000000000000;
			r_z0__20 <= 32'b00000000000000000000000000000000;
			r_mut__20 <= 32'b00000000000000000000000000000000;

			r_x__19	<= 32'b00000000000000000000000000000000;
			r_y__19	<= 32'b00000000000000000000000000000000;
			r_z__19	<= 32'b00000000000000000000000000000000;
			r_ux__19	<= 32'b00000000000000000000000000000000;
			r_uy__19	<= 32'b00000000000000000000000000000000;
			r_uz__19	<= 32'b00000000000000000000000000000000;
			r_sz__19	<= 32'b00000000000000000000000000000000;
			r_sr__19	<= 32'b00000000000000000000000000000000;
			r_sleftz__19	<= 32'b00000000000000000000000000000000;
			r_sleftr__19	<= 32'b00000000000000000000000000000000;
			r_weight__19	<= 32'b00000000000000000000000000000000;
			r_layer__19	<= 3'b000;
			r_dead__19	<= 1'b1;
			r_hit__19	<= 1'b0;
			r_diff__19 <= 32'b00000000000000000000000000000000;
			r_dl_b__19 <= 32'b00000000000000000000000000000000;
			r_numer__19 <= 0;
			r_z1__19 <= 32'b00000000000000000000000000000000;
			r_z0__19 <= 32'b00000000000000000000000000000000;
			r_mut__19 <= 32'b00000000000000000000000000000000;

			r_x__18	<= 32'b00000000000000000000000000000000;
			r_y__18	<= 32'b00000000000000000000000000000000;
			r_z__18	<= 32'b00000000000000000000000000000000;
			r_ux__18	<= 32'b00000000000000000000000000000000;
			r_uy__18	<= 32'b00000000000000000000000000000000;
			r_uz__18	<= 32'b00000000000000000000000000000000;
			r_sz__18	<= 32'b00000000000000000000000000000000;
			r_sr__18	<= 32'b00000000000000000000000000000000;
			r_sleftz__18	<= 32'b00000000000000000000000000000000;
			r_sleftr__18	<= 32'b00000000000000000000000000000000;
			r_weight__18	<= 32'b00000000000000000000000000000000;
			r_layer__18	<= 3'b000;
			r_dead__18	<= 1'b1;
			r_hit__18	<= 1'b0;
			r_diff__18 <= 32'b00000000000000000000000000000000;
			r_dl_b__18 <= 32'b00000000000000000000000000000000;
			r_numer__18 <= 0;
			r_z1__18 <= 32'b00000000000000000000000000000000;
			r_z0__18 <= 32'b00000000000000000000000000000000;
			r_mut__18 <= 32'b00000000000000000000000000000000;

			r_x__17	<= 32'b00000000000000000000000000000000;
			r_y__17	<= 32'b00000000000000000000000000000000;
			r_z__17	<= 32'b00000000000000000000000000000000;
			r_ux__17	<= 32'b00000000000000000000000000000000;
			r_uy__17	<= 32'b00000000000000000000000000000000;
			r_uz__17	<= 32'b00000000000000000000000000000000;
			r_sz__17	<= 32'b00000000000000000000000000000000;
			r_sr__17	<= 32'b00000000000000000000000000000000;
			r_sleftz__17	<= 32'b00000000000000000000000000000000;
			r_sleftr__17	<= 32'b00000000000000000000000000000000;
			r_weight__17	<= 32'b00000000000000000000000000000000;
			r_layer__17	<= 3'b000;
			r_dead__17	<= 1'b1;
			r_hit__17	<= 1'b0;
			r_diff__17 <= 32'b00000000000000000000000000000000;
			r_dl_b__17 <= 32'b00000000000000000000000000000000;
			r_numer__17 <= 0;
			r_z1__17 <= 32'b00000000000000000000000000000000;
			r_z0__17 <= 32'b00000000000000000000000000000000;
			r_mut__17 <= 32'b00000000000000000000000000000000;

			r_x__16	<= 32'b00000000000000000000000000000000;
			r_y__16	<= 32'b00000000000000000000000000000000;
			r_z__16	<= 32'b00000000000000000000000000000000;
			r_ux__16	<= 32'b00000000000000000000000000000000;
			r_uy__16	<= 32'b00000000000000000000000000000000;
			r_uz__16	<= 32'b00000000000000000000000000000000;
			r_sz__16	<= 32'b00000000000000000000000000000000;
			r_sr__16	<= 32'b00000000000000000000000000000000;
			r_sleftz__16	<= 32'b00000000000000000000000000000000;
			r_sleftr__16	<= 32'b00000000000000000000000000000000;
			r_weight__16	<= 32'b00000000000000000000000000000000;
			r_layer__16	<= 3'b000;
			r_dead__16	<= 1'b1;
			r_hit__16	<= 1'b0;
			r_diff__16 <= 32'b00000000000000000000000000000000;
			r_dl_b__16 <= 32'b00000000000000000000000000000000;
			r_numer__16 <= 0;
			r_z1__16 <= 32'b00000000000000000000000000000000;
			r_z0__16 <= 32'b00000000000000000000000000000000;
			r_mut__16 <= 32'b00000000000000000000000000000000;

			r_x__15	<= 32'b00000000000000000000000000000000;
			r_y__15	<= 32'b00000000000000000000000000000000;
			r_z__15	<= 32'b00000000000000000000000000000000;
			r_ux__15	<= 32'b00000000000000000000000000000000;
			r_uy__15	<= 32'b00000000000000000000000000000000;
			r_uz__15	<= 32'b00000000000000000000000000000000;
			r_sz__15	<= 32'b00000000000000000000000000000000;
			r_sr__15	<= 32'b00000000000000000000000000000000;
			r_sleftz__15	<= 32'b00000000000000000000000000000000;
			r_sleftr__15	<= 32'b00000000000000000000000000000000;
			r_weight__15	<= 32'b00000000000000000000000000000000;
			r_layer__15	<= 3'b000;
			r_dead__15	<= 1'b1;
			r_hit__15	<= 1'b0;
			r_diff__15 <= 32'b00000000000000000000000000000000;
			r_dl_b__15 <= 32'b00000000000000000000000000000000;
			r_numer__15 <= 0;
			r_z1__15 <= 32'b00000000000000000000000000000000;
			r_z0__15 <= 32'b00000000000000000000000000000000;
			r_mut__15 <= 32'b00000000000000000000000000000000;

			r_x__14	<= 32'b00000000000000000000000000000000;
			r_y__14	<= 32'b00000000000000000000000000000000;
			r_z__14	<= 32'b00000000000000000000000000000000;
			r_ux__14	<= 32'b00000000000000000000000000000000;
			r_uy__14	<= 32'b00000000000000000000000000000000;
			r_uz__14	<= 32'b00000000000000000000000000000000;
			r_sz__14	<= 32'b00000000000000000000000000000000;
			r_sr__14	<= 32'b00000000000000000000000000000000;
			r_sleftz__14	<= 32'b00000000000000000000000000000000;
			r_sleftr__14	<= 32'b00000000000000000000000000000000;
			r_weight__14	<= 32'b00000000000000000000000000000000;
			r_layer__14	<= 3'b000;
			r_dead__14	<= 1'b1;
			r_hit__14	<= 1'b0;
			r_diff__14 <= 32'b00000000000000000000000000000000;
			r_dl_b__14 <= 32'b00000000000000000000000000000000;
			r_numer__14 <= 0;
			r_z1__14 <= 32'b00000000000000000000000000000000;
			r_z0__14 <= 32'b00000000000000000000000000000000;
			r_mut__14 <= 32'b00000000000000000000000000000000;

			r_x__13	<= 32'b00000000000000000000000000000000;
			r_y__13	<= 32'b00000000000000000000000000000000;
			r_z__13	<= 32'b00000000000000000000000000000000;
			r_ux__13	<= 32'b00000000000000000000000000000000;
			r_uy__13	<= 32'b00000000000000000000000000000000;
			r_uz__13	<= 32'b00000000000000000000000000000000;
			r_sz__13	<= 32'b00000000000000000000000000000000;
			r_sr__13	<= 32'b00000000000000000000000000000000;
			r_sleftz__13	<= 32'b00000000000000000000000000000000;
			r_sleftr__13	<= 32'b00000000000000000000000000000000;
			r_weight__13	<= 32'b00000000000000000000000000000000;
			r_layer__13	<= 3'b000;
			r_dead__13	<= 1'b1;
			r_hit__13	<= 1'b0;
			r_diff__13 <= 32'b00000000000000000000000000000000;
			r_dl_b__13 <= 32'b00000000000000000000000000000000;
			r_numer__13 <= 0;
			r_z1__13 <= 32'b00000000000000000000000000000000;
			r_z0__13 <= 32'b00000000000000000000000000000000;
			r_mut__13 <= 32'b00000000000000000000000000000000;

			r_x__12	<= 32'b00000000000000000000000000000000;
			r_y__12	<= 32'b00000000000000000000000000000000;
			r_z__12	<= 32'b00000000000000000000000000000000;
			r_ux__12	<= 32'b00000000000000000000000000000000;
			r_uy__12	<= 32'b00000000000000000000000000000000;
			r_uz__12	<= 32'b00000000000000000000000000000000;
			r_sz__12	<= 32'b00000000000000000000000000000000;
			r_sr__12	<= 32'b00000000000000000000000000000000;
			r_sleftz__12	<= 32'b00000000000000000000000000000000;
			r_sleftr__12	<= 32'b00000000000000000000000000000000;
			r_weight__12	<= 32'b00000000000000000000000000000000;
			r_layer__12	<= 3'b000;
			r_dead__12	<= 1'b1;
			r_hit__12	<= 1'b0;
			r_diff__12 <= 32'b00000000000000000000000000000000;
			r_dl_b__12 <= 32'b00000000000000000000000000000000;
			r_numer__12 <= 0;
			r_z1__12 <= 32'b00000000000000000000000000000000;
			r_z0__12 <= 32'b00000000000000000000000000000000;
			r_mut__12 <= 32'b00000000000000000000000000000000;

			r_x__11	<= 32'b00000000000000000000000000000000;
			r_y__11	<= 32'b00000000000000000000000000000000;
			r_z__11	<= 32'b00000000000000000000000000000000;
			r_ux__11	<= 32'b00000000000000000000000000000000;
			r_uy__11	<= 32'b00000000000000000000000000000000;
			r_uz__11	<= 32'b00000000000000000000000000000000;
			r_sz__11	<= 32'b00000000000000000000000000000000;
			r_sr__11	<= 32'b00000000000000000000000000000000;
			r_sleftz__11	<= 32'b00000000000000000000000000000000;
			r_sleftr__11	<= 32'b00000000000000000000000000000000;
			r_weight__11	<= 32'b00000000000000000000000000000000;
			r_layer__11	<= 3'b000;
			r_dead__11	<= 1'b1;
			r_hit__11	<= 1'b0;
			r_diff__11 <= 32'b00000000000000000000000000000000;
			r_dl_b__11 <= 32'b00000000000000000000000000000000;
			r_numer__11 <= 0;
			r_z1__11 <= 32'b00000000000000000000000000000000;
			r_z0__11 <= 32'b00000000000000000000000000000000;
			r_mut__11 <= 32'b00000000000000000000000000000000;

			r_x__10	<= 32'b00000000000000000000000000000000;
			r_y__10	<= 32'b00000000000000000000000000000000;
			r_z__10	<= 32'b00000000000000000000000000000000;
			r_ux__10	<= 32'b00000000000000000000000000000000;
			r_uy__10	<= 32'b00000000000000000000000000000000;
			r_uz__10	<= 32'b00000000000000000000000000000000;
			r_sz__10	<= 32'b00000000000000000000000000000000;
			r_sr__10	<= 32'b00000000000000000000000000000000;
			r_sleftz__10	<= 32'b00000000000000000000000000000000;
			r_sleftr__10	<= 32'b00000000000000000000000000000000;
			r_weight__10	<= 32'b00000000000000000000000000000000;
			r_layer__10	<= 3'b000;
			r_dead__10	<= 1'b1;
			r_hit__10	<= 1'b0;
			r_diff__10 <= 32'b00000000000000000000000000000000;
			r_dl_b__10 <= 32'b00000000000000000000000000000000;
			r_numer__10 <= 0;
			r_z1__10 <= 32'b00000000000000000000000000000000;
			r_z0__10 <= 32'b00000000000000000000000000000000;
			r_mut__10 <= 32'b00000000000000000000000000000000;

			r_x__9	<= 32'b00000000000000000000000000000000;
			r_y__9	<= 32'b00000000000000000000000000000000;
			r_z__9	<= 32'b00000000000000000000000000000000;
			r_ux__9	<= 32'b00000000000000000000000000000000;
			r_uy__9	<= 32'b00000000000000000000000000000000;
			r_uz__9	<= 32'b00000000000000000000000000000000;
			r_sz__9	<= 32'b00000000000000000000000000000000;
			r_sr__9	<= 32'b00000000000000000000000000000000;
			r_sleftz__9	<= 32'b00000000000000000000000000000000;
			r_sleftr__9	<= 32'b00000000000000000000000000000000;
			r_weight__9	<= 32'b00000000000000000000000000000000;
			r_layer__9	<= 3'b000;
			r_dead__9	<= 1'b1;
			r_hit__9	<= 1'b0;
			r_diff__9 <= 32'b00000000000000000000000000000000;
			r_dl_b__9 <= 32'b00000000000000000000000000000000;
			r_numer__9 <= 0;
			r_z1__9 <= 32'b00000000000000000000000000000000;
			r_z0__9 <= 32'b00000000000000000000000000000000;
			r_mut__9 <= 32'b00000000000000000000000000000000;

			r_x__8	<= 32'b00000000000000000000000000000000;
			r_y__8	<= 32'b00000000000000000000000000000000;
			r_z__8	<= 32'b00000000000000000000000000000000;
			r_ux__8	<= 32'b00000000000000000000000000000000;
			r_uy__8	<= 32'b00000000000000000000000000000000;
			r_uz__8	<= 32'b00000000000000000000000000000000;
			r_sz__8	<= 32'b00000000000000000000000000000000;
			r_sr__8	<= 32'b00000000000000000000000000000000;
			r_sleftz__8	<= 32'b00000000000000000000000000000000;
			r_sleftr__8	<= 32'b00000000000000000000000000000000;
			r_weight__8	<= 32'b00000000000000000000000000000000;
			r_layer__8	<= 3'b000;
			r_dead__8	<= 1'b1;
			r_hit__8	<= 1'b0;
			r_diff__8 <= 32'b00000000000000000000000000000000;
			r_dl_b__8 <= 32'b00000000000000000000000000000000;
			r_numer__8 <= 0;
			r_z1__8 <= 32'b00000000000000000000000000000000;
			r_z0__8 <= 32'b00000000000000000000000000000000;
			r_mut__8 <= 32'b00000000000000000000000000000000;

			r_x__7	<= 32'b00000000000000000000000000000000;
			r_y__7	<= 32'b00000000000000000000000000000000;
			r_z__7	<= 32'b00000000000000000000000000000000;
			r_ux__7	<= 32'b00000000000000000000000000000000;
			r_uy__7	<= 32'b00000000000000000000000000000000;
			r_uz__7	<= 32'b00000000000000000000000000000000;
			r_sz__7	<= 32'b00000000000000000000000000000000;
			r_sr__7	<= 32'b00000000000000000000000000000000;
			r_sleftz__7	<= 32'b00000000000000000000000000000000;
			r_sleftr__7	<= 32'b00000000000000000000000000000000;
			r_weight__7	<= 32'b00000000000000000000000000000000;
			r_layer__7	<= 3'b000;
			r_dead__7	<= 1'b1;
			r_hit__7	<= 1'b0;
			r_diff__7 <= 32'b00000000000000000000000000000000;
			r_dl_b__7 <= 32'b00000000000000000000000000000000;
			r_numer__7 <= 0;
			r_z1__7 <= 32'b00000000000000000000000000000000;
			r_z0__7 <= 32'b00000000000000000000000000000000;
			r_mut__7 <= 32'b00000000000000000000000000000000;

			r_x__6	<= 32'b00000000000000000000000000000000;
			r_y__6	<= 32'b00000000000000000000000000000000;
			r_z__6	<= 32'b00000000000000000000000000000000;
			r_ux__6	<= 32'b00000000000000000000000000000000;
			r_uy__6	<= 32'b00000000000000000000000000000000;
			r_uz__6	<= 32'b00000000000000000000000000000000;
			r_sz__6	<= 32'b00000000000000000000000000000000;
			r_sr__6	<= 32'b00000000000000000000000000000000;
			r_sleftz__6	<= 32'b00000000000000000000000000000000;
			r_sleftr__6	<= 32'b00000000000000000000000000000000;
			r_weight__6	<= 32'b00000000000000000000000000000000;
			r_layer__6	<= 3'b000;
			r_dead__6	<= 1'b1;
			r_hit__6	<= 1'b0;
			r_diff__6 <= 32'b00000000000000000000000000000000;
			r_dl_b__6 <= 32'b00000000000000000000000000000000;
			r_numer__6 <= 0;
			r_z1__6 <= 32'b00000000000000000000000000000000;
			r_z0__6 <= 32'b00000000000000000000000000000000;
			r_mut__6 <= 32'b00000000000000000000000000000000;

			r_x__5	<= 32'b00000000000000000000000000000000;
			r_y__5	<= 32'b00000000000000000000000000000000;
			r_z__5	<= 32'b00000000000000000000000000000000;
			r_ux__5	<= 32'b00000000000000000000000000000000;
			r_uy__5	<= 32'b00000000000000000000000000000000;
			r_uz__5	<= 32'b00000000000000000000000000000000;
			r_sz__5	<= 32'b00000000000000000000000000000000;
			r_sr__5	<= 32'b00000000000000000000000000000000;
			r_sleftz__5	<= 32'b00000000000000000000000000000000;
			r_sleftr__5	<= 32'b00000000000000000000000000000000;
			r_weight__5	<= 32'b00000000000000000000000000000000;
			r_layer__5	<= 3'b000;
			r_dead__5	<= 1'b1;
			r_hit__5	<= 1'b0;
			r_diff__5 <= 32'b00000000000000000000000000000000;
			r_dl_b__5 <= 32'b00000000000000000000000000000000;
			r_numer__5 <= 0;
			r_z1__5 <= 32'b00000000000000000000000000000000;
			r_z0__5 <= 32'b00000000000000000000000000000000;
			r_mut__5 <= 32'b00000000000000000000000000000000;

			r_x__4	<= 32'b00000000000000000000000000000000;
			r_y__4	<= 32'b00000000000000000000000000000000;
			r_z__4	<= 32'b00000000000000000000000000000000;
			r_ux__4	<= 32'b00000000000000000000000000000000;
			r_uy__4	<= 32'b00000000000000000000000000000000;
			r_uz__4	<= 32'b00000000000000000000000000000000;
			r_sz__4	<= 32'b00000000000000000000000000000000;
			r_sr__4	<= 32'b00000000000000000000000000000000;
			r_sleftz__4	<= 32'b00000000000000000000000000000000;
			r_sleftr__4	<= 32'b00000000000000000000000000000000;
			r_weight__4	<= 32'b00000000000000000000000000000000;
			r_layer__4	<= 3'b000;
			r_dead__4	<= 1'b1;
			r_hit__4	<= 1'b0;
			r_diff__4 <= 32'b00000000000000000000000000000000;
			r_dl_b__4 <= 32'b00000000000000000000000000000000;
			r_numer__4 <= 0;
			r_z1__4 <= 32'b00000000000000000000000000000000;
			r_z0__4 <= 32'b00000000000000000000000000000000;
			r_mut__4 <= 32'b00000000000000000000000000000000;

			r_x__3	<= 32'b00000000000000000000000000000000;
			r_y__3	<= 32'b00000000000000000000000000000000;
			r_z__3	<= 32'b00000000000000000000000000000000;
			r_ux__3	<= 32'b00000000000000000000000000000000;
			r_uy__3	<= 32'b00000000000000000000000000000000;
			r_uz__3	<= 32'b00000000000000000000000000000000;
			r_sz__3	<= 32'b00000000000000000000000000000000;
			r_sr__3	<= 32'b00000000000000000000000000000000;
			r_sleftz__3	<= 32'b00000000000000000000000000000000;
			r_sleftr__3	<= 32'b00000000000000000000000000000000;
			r_weight__3	<= 32'b00000000000000000000000000000000;
			r_layer__3	<= 3'b000;
			r_dead__3	<= 1'b1;
			r_hit__3	<= 1'b0;
			r_diff__3 <= 32'b00000000000000000000000000000000;
			r_dl_b__3 <= 32'b00000000000000000000000000000000;
			r_numer__3 <= 0;
			r_z1__3 <= 32'b00000000000000000000000000000000;
			r_z0__3 <= 32'b00000000000000000000000000000000;
			r_mut__3 <= 32'b00000000000000000000000000000000;

			r_x__2	<= 32'b00000000000000000000000000000000;
			r_y__2	<= 32'b00000000000000000000000000000000;
			r_z__2	<= 32'b00000000000000000000000000000000;
			r_ux__2	<= 32'b00000000000000000000000000000000;
			r_uy__2	<= 32'b00000000000000000000000000000000;
			r_uz__2	<= 32'b00000000000000000000000000000000;
			r_sz__2	<= 32'b00000000000000000000000000000000;
			r_sr__2	<= 32'b00000000000000000000000000000000;
			r_sleftz__2	<= 32'b00000000000000000000000000000000;
			r_sleftr__2	<= 32'b00000000000000000000000000000000;
			r_weight__2	<= 32'b00000000000000000000000000000000;
			r_layer__2	<= 3'b000;
			r_dead__2	<= 1'b1;
			r_hit__2	<= 1'b0;
			r_diff__2 <= 32'b00000000000000000000000000000000;
			r_dl_b__2 <= 32'b00000000000000000000000000000000;
			r_numer__2 <= 0;
			r_z1__2 <= 32'b00000000000000000000000000000000;
			r_z0__2 <= 32'b00000000000000000000000000000000;
			r_mut__2 <= 32'b00000000000000000000000000000000;

			r_x__1	<= 32'b00000000000000000000000000000000;
			r_y__1	<= 32'b00000000000000000000000000000000;
			r_z__1	<= 32'b00000000000000000000000000000000;
			r_ux__1	<= 32'b00000000000000000000000000000000;
			r_uy__1	<= 32'b00000000000000000000000000000000;
			r_uz__1	<= 32'b00000000000000000000000000000000;
			r_sz__1	<= 32'b00000000000000000000000000000000;
			r_sr__1	<= 32'b00000000000000000000000000000000;
			r_sleftz__1	<= 32'b00000000000000000000000000000000;
			r_sleftr__1	<= 32'b00000000000000000000000000000000;
			r_weight__1	<= 32'b00000000000000000000000000000000;
			r_layer__1	<= 3'b000;
			r_dead__1	<= 1'b1;
			r_hit__1	<= 1'b0;
			r_diff__1 <= 32'b00000000000000000000000000000000;
			r_dl_b__1 <= 32'b00000000000000000000000000000000;
			r_numer__1 <= 0;
			r_z1__1 <= 32'b00000000000000000000000000000000;
			r_z0__1 <= 32'b00000000000000000000000000000000;
			r_mut__1 <= 32'b00000000000000000000000000000000;
			
				r_x__0	<= 32'b00000000000000000000000000000000;
			r_y__0	<= 32'b00000000000000000000000000000000;
			r_z__0	<= 32'b00000000000000000000000000000000;
			r_ux__0	<= 32'b00000000000000000000000000000000;
			r_uy__0	<= 32'b00000000000000000000000000000000;
			r_uz__0	<= 32'b00000000000000000000000000000000;
			r_sz__0	<= 32'b00000000000000000000000000000000;
			r_sr__0	<= 32'b00000000000000000000000000000000;
			r_sleftz__0	<= 32'b00000000000000000000000000000000;
			r_sleftr__0	<= 32'b00000000000000000000000000000000;
			r_weight__0	<= 32'b00000000000000000000000000000000;
			r_layer__0	<= 3'b000;
			r_dead__0	<= 1'b0;
			r_hit__0	<= 1'b0;
			r_diff__0 <= 32'b00000000000000000000000000000000;
			r_dl_b__0 <= 32'b00000000000000000000000000000000;
			r_numer__0 <= 0;
			r_z1__0 <= 32'b00000000000000000000000000000000;
			r_z0__0 <= 32'b00000000000000000000000000000000;
			r_mut__0 <= 32'b00000000000000000000000000000000;
	end
	
	else 
	begin
		if(enable)
		begin
		
			//for 0
		r_x__0	<=c_x__0;
		r_y__0	<=c_y__0;
		r_z__0	<=c_z__0;
		r_ux__0	<=c_ux__0;
		r_uy__0	<=c_uy__0;
		r_uz__0	<=c_uz__0;
		r_sz__0	<=c_sz__0;
		r_sr__0	<=c_sr__0;
		r_sleftz__0	<=c_sleftz__0;
		r_sleftr__0	<=c_sleftr__0;
		r_weight__0	<=c_weight__0;
		r_layer__0	<=c_layer__0;
		r_dead__0	<=c_dead__0;
		r_hit__0	<=c_hit__0;
		r_diff__0 <=c_diff__0;
		r_dl_b__0 <=c_dl_b__0;
		r_numer__0 <=c_numer__0;
		r_z1__0 <=c_z1__0;
		r_z0__0 <=c_z0__0;
		r_mut__0 <=c_mut__0;
		
	//for 1
	
		r_x__1	<=c_x__1;
		r_y__1	<=c_y__1;
		r_z__1	<=c_z__1;
		r_ux__1	<=c_ux__1;
		r_uy__1	<=c_uy__1;
		r_uz__1	<=c_uz__1;
		r_sz__1	<=c_sz__1;
		r_sr__1	<=c_sr__1;
		r_sleftz__1	<=c_sleftz__1;
		r_sleftr__1	<=c_sleftr__1;
		r_weight__1	<=c_weight__1;
		r_layer__1	<=c_layer__1;
		r_dead__1	<=c_dead__1;
		r_hit__1	<=c_hit__1;
		r_diff__1 <=c_diff__1;
		r_dl_b__1 <=c_dl_b__1;
		r_numer__1 <=c_numer__1;
		r_z1__1 <=c_z1__1;
		r_z0__1 <=c_z0__1;
		r_mut__1 <=c_mut__1;
		
		//for 2
		r_x__2	<=c_x__2;
		r_y__2	<=c_y__2;
		r_z__2	<=c_z__2;
		r_ux__2	<=c_ux__2;
		r_uy__2	<=c_uy__2;
		r_uz__2	<=c_uz__2;
		r_sz__2	<=c_sz__2;
		r_sr__2	<=c_sr__2;
		r_sleftz__2	<=c_sleftz__2;
		r_sleftr__2	<=c_sleftr__2;
		r_weight__2	<=c_weight__2;
		r_layer__2	<=c_layer__2;
		r_dead__2	<=c_dead__2;
		r_hit__2	<=c_hit__2;
		r_diff__2 <=c_diff__2;
		r_dl_b__2 <=c_dl_b__2;
		r_numer__2 <=c_numer__2;
		r_z1__2 <=c_z1__2;
		r_z0__2 <=c_z0__2;
		r_mut__2 <=c_mut__2;
	
		//for 3
		r_x__3	<=c_x__3;
		r_y__3	<=c_y__3;
		r_z__3	<=c_z__3;
		r_ux__3	<=c_ux__3;
		r_uy__3	<=c_uy__3;
		r_uz__3	<=c_uz__3;
		r_sz__3	<=c_sz__3;
		r_sr__3	<=c_sr__3;
		r_sleftz__3	<=c_sleftz__3;
		r_sleftr__3	<=c_sleftr__3;
		r_weight__3	<=c_weight__3;
		r_layer__3	<=c_layer__3;
		r_dead__3	<=c_dead__3;
		r_hit__3	<=c_hit__3;
		r_diff__3 <=c_diff__3;
		r_dl_b__3 <=c_dl_b__3;
		r_numer__3 <=c_numer__3;
		r_z1__3 <=c_z1__3;
		r_z0__3 <=c_z0__3;
		r_mut__3 <=c_mut__3;
		
		//for 4
		r_x__4	<=c_x__4;
		r_y__4	<=c_y__4;
		r_z__4	<=c_z__4;
		r_ux__4	<=c_ux__4;
		r_uy__4	<=c_uy__4;
		r_uz__4	<=c_uz__4;
		r_sz__4	<=c_sz__4;
		r_sr__4	<=c_sr__4;
		r_sleftz__4	<=c_sleftz__4;
		r_sleftr__4	<=c_sleftr__4;
		r_weight__4	<=c_weight__4;
		r_layer__4	<=c_layer__4;
		r_dead__4	<=c_dead__4;
		r_hit__4	<=c_hit__4;
		r_diff__4 <=c_diff__4;
		r_dl_b__4 <=c_dl_b__4;
		r_numer__4 <=c_numer__4;
		r_z1__4 <=c_z1__4;
		r_z0__4 <=c_z0__4;
		r_mut__4 <=c_mut__4;
		
		//for 5
		r_x__5	<=c_x__5;
		r_y__5	<=c_y__5;
		r_z__5	<=c_z__5;
		r_ux__5	<=c_ux__5;
		r_uy__5	<=c_uy__5;
		r_uz__5	<=c_uz__5;
		r_sz__5	<=c_sz__5;
		r_sr__5	<=c_sr__5;
		r_sleftz__5	<=c_sleftz__5;
		r_sleftr__5	<=c_sleftr__5;
		r_weight__5	<=c_weight__5;
		r_layer__5	<=c_layer__5;
		r_dead__5	<=c_dead__5;
		r_hit__5	<=c_hit__5;
		r_diff__5 <=c_diff__5;
		r_dl_b__5 <=c_dl_b__5;
		r_numer__5 <=c_numer__5;
		r_z1__5 <=c_z1__5;
		r_z0__5 <=c_z0__5;
		r_mut__5 <=c_mut__5;
		
		//for 6
		r_x__6	<=c_x__6;
		r_y__6	<=c_y__6;
		r_z__6	<=c_z__6;
		r_ux__6	<=c_ux__6;
		r_uy__6	<=c_uy__6;
		r_uz__6	<=c_uz__6;
		r_sz__6	<=c_sz__6;
		r_sr__6	<=c_sr__6;
		r_sleftz__6	<=c_sleftz__6;
		r_sleftr__6	<=c_sleftr__6;
		r_weight__6	<=c_weight__6;
		r_layer__6	<=c_layer__6;
		r_dead__6	<=c_dead__6;
		r_hit__6	<=c_hit__6;
		r_diff__6 <=c_diff__6;
		r_dl_b__6 <=c_dl_b__6;
		r_numer__6 <=c_numer__6;
		r_z1__6 <=c_z1__6;
		r_z0__6 <=c_z0__6;
		r_mut__6 <=c_mut__6;
		
		//for 7
		r_x__7	<=c_x__7;
		r_y__7	<=c_y__7;
		r_z__7	<=c_z__7;
		r_ux__7	<=c_ux__7;
		r_uy__7	<=c_uy__7;
		r_uz__7	<=c_uz__7;
		r_sz__7	<=c_sz__7;
		r_sr__7	<=c_sr__7;
		r_sleftz__7	<=c_sleftz__7;
		r_sleftr__7	<=c_sleftr__7;
		r_weight__7	<=c_weight__7;
		r_layer__7	<=c_layer__7;
		r_dead__7	<=c_dead__7;
		r_hit__7	<=c_hit__7;
		r_diff__7 <=c_diff__7;
		r_dl_b__7 <=c_dl_b__7;
		r_numer__7 <=c_numer__7;
		r_z1__7 <=c_z1__7;
		r_z0__7 <=c_z0__7;
		r_mut__7 <=c_mut__7;
		
		//for 8
		r_x__8	<=c_x__8;
		r_y__8	<=c_y__8;
		r_z__8	<=c_z__8;
		r_ux__8	<=c_ux__8;
		r_uy__8	<=c_uy__8;
		r_uz__8	<=c_uz__8;
		r_sz__8	<=c_sz__8;
		r_sr__8	<=c_sr__8;
		r_sleftz__8	<=c_sleftz__8;
		r_sleftr__8	<=c_sleftr__8;
		r_weight__8	<=c_weight__8;
		r_layer__8	<=c_layer__8;
		r_dead__8	<=c_dead__8;
		r_hit__8	<=c_hit__8;
		r_diff__8 <=c_diff__8;
		r_dl_b__8 <=c_dl_b__8;
		r_numer__8 <=c_numer__8;
		r_z1__8 <=c_z1__8;
		r_z0__8 <=c_z0__8;
		r_mut__8 <=c_mut__8;
		
		//for 9
		r_x__9	<=c_x__9;
		r_y__9	<=c_y__9;
		r_z__9	<=c_z__9;
		r_ux__9	<=c_ux__9;
		r_uy__9	<=c_uy__9;
		r_uz__9	<=c_uz__9;
		r_sz__9	<=c_sz__9;
		r_sr__9	<=c_sr__9;
		r_sleftz__9	<=c_sleftz__9;
		r_sleftr__9	<=c_sleftr__9;
		r_weight__9	<=c_weight__9;
		r_layer__9	<=c_layer__9;
		r_dead__9	<=c_dead__9;
		r_hit__9	<=c_hit__9;
		r_diff__9 <=c_diff__9;
		r_dl_b__9 <=c_dl_b__9;
		r_numer__9 <=c_numer__9;
		r_z1__9 <=c_z1__9;
		r_z0__9 <=c_z0__9;
		r_mut__9 <=c_mut__9;
		
		//for 10
		r_x__10	<=c_x__10;
		r_y__10	<=c_y__10;
		r_z__10	<=c_z__10;
		r_ux__10	<=c_ux__10;
		r_uy__10	<=c_uy__10;
		r_uz__10	<=c_uz__10;
		r_sz__10	<=c_sz__10;
		r_sr__10	<=c_sr__10;
		r_sleftz__10	<=c_sleftz__10;
		r_sleftr__10	<=c_sleftr__10;
		r_weight__10	<=c_weight__10;
		r_layer__10	<=c_layer__10;
		r_dead__10	<=c_dead__10;
		r_hit__10	<=c_hit__10;
		r_diff__10 <=c_diff__10;
		r_dl_b__10 <=c_dl_b__10;
		r_numer__10 <=c_numer__10;
		r_z1__10 <=c_z1__10;
		r_z0__10 <=c_z0__10;
		r_mut__10 <=c_mut__10;
		
		//for 11
		r_x__11	<=c_x__11;
		r_y__11	<=c_y__11;
		r_z__11	<=c_z__11;
		r_ux__11	<=c_ux__11;
		r_uy__11	<=c_uy__11;
		r_uz__11	<=c_uz__11;
		r_sz__11	<=c_sz__11;
		r_sr__11	<=c_sr__11;
		r_sleftz__11	<=c_sleftz__11;
		r_sleftr__11	<=c_sleftr__11;
		r_weight__11	<=c_weight__11;
		r_layer__11	<=c_layer__11;
		r_dead__11	<=c_dead__11;
		r_hit__11	<=c_hit__11;
		r_diff__11 <=c_diff__11;
		r_dl_b__11 <=c_dl_b__11;
		r_numer__11 <=c_numer__11;
		r_z1__11 <=c_z1__11;
		r_z0__11 <=c_z0__11;
		r_mut__11 <=c_mut__11;
		
		//for 12
		r_x__12	<=c_x__12;
		r_y__12	<=c_y__12;
		r_z__12	<=c_z__12;
		r_ux__12	<=c_ux__12;
		r_uy__12	<=c_uy__12;
		r_uz__12	<=c_uz__12;
		r_sz__12	<=c_sz__12;
		r_sr__12	<=c_sr__12;
		r_sleftz__12	<=c_sleftz__12;
		r_sleftr__12	<=c_sleftr__12;
		r_weight__12	<=c_weight__12;
		r_layer__12	<=c_layer__12;
		r_dead__12	<=c_dead__12;
		r_hit__12	<=c_hit__12;
		r_diff__12 <=c_diff__12;
		r_dl_b__12 <=c_dl_b__12;
		r_numer__12 <=c_numer__12;
		r_z1__12 <=c_z1__12;
		r_z0__12 <=c_z0__12;
		r_mut__12 <=c_mut__12;
		
		//for 13
		r_x__13	<=c_x__13;
		r_y__13	<=c_y__13;
		r_z__13	<=c_z__13;
		r_ux__13	<=c_ux__13;
		r_uy__13	<=c_uy__13;
		r_uz__13	<=c_uz__13;
		r_sz__13	<=c_sz__13;
		r_sr__13	<=c_sr__13;
		r_sleftz__13	<=c_sleftz__13;
		r_sleftr__13	<=c_sleftr__13;
		r_weight__13	<=c_weight__13;
		r_layer__13	<=c_layer__13;
		r_dead__13	<=c_dead__13;
		r_hit__13	<=c_hit__13;
		r_diff__13 <=c_diff__13;
		r_dl_b__13 <=c_dl_b__13;
		r_numer__13 <=c_numer__13;
		r_z1__13 <=c_z1__13;
		r_z0__13 <=c_z0__13;
		r_mut__13 <=c_mut__13;
		
		//for 14
		r_x__14	<=c_x__14;
		r_y__14	<=c_y__14;
		r_z__14	<=c_z__14;
		r_ux__14	<=c_ux__14;
		r_uy__14	<=c_uy__14;
		r_uz__14	<=c_uz__14;
		r_sz__14	<=c_sz__14;
		r_sr__14	<=c_sr__14;
		r_sleftz__14	<=c_sleftz__14;
		r_sleftr__14	<=c_sleftr__14;
		r_weight__14	<=c_weight__14;
		r_layer__14	<=c_layer__14;
		r_dead__14	<=c_dead__14;
		r_hit__14	<=c_hit__14;
		r_diff__14 <=c_diff__14;
		r_dl_b__14 <=c_dl_b__14;
		r_numer__14 <=c_numer__14;
		r_z1__14 <=c_z1__14;
		r_z0__14 <=c_z0__14;
		r_mut__14 <=c_mut__14;
		
		//for 15
		r_x__15	<=c_x__15;
		r_y__15	<=c_y__15;
		r_z__15	<=c_z__15;
		r_ux__15	<=c_ux__15;
		r_uy__15	<=c_uy__15;
		r_uz__15	<=c_uz__15;
		r_sz__15	<=c_sz__15;
		r_sr__15	<=c_sr__15;
		r_sleftz__15	<=c_sleftz__15;
		r_sleftr__15	<=c_sleftr__15;
		r_weight__15	<=c_weight__15;
		r_layer__15	<=c_layer__15;
		r_dead__15	<=c_dead__15;
		r_hit__15	<=c_hit__15;
		r_diff__15 <=c_diff__15;
		r_dl_b__15 <=c_dl_b__15;
		r_numer__15 <=c_numer__15;
		r_z1__15 <=c_z1__15;
		r_z0__15 <=c_z0__15;
		r_mut__15 <=c_mut__15;
		
		//for 16
		r_x__16	<=c_x__16;
		r_y__16	<=c_y__16;
		r_z__16	<=c_z__16;
		r_ux__16	<=c_ux__16;
		r_uy__16	<=c_uy__16;
		r_uz__16	<=c_uz__16;
		r_sz__16	<=c_sz__16;
		r_sr__16	<=c_sr__16;
		r_sleftz__16	<=c_sleftz__16;
		r_sleftr__16	<=c_sleftr__16;
		r_weight__16	<=c_weight__16;
		r_layer__16	<=c_layer__16;
		r_dead__16	<=c_dead__16;
		r_hit__16	<=c_hit__16;
		r_diff__16 <=c_diff__16;
		r_dl_b__16 <=c_dl_b__16;
		r_numer__16 <=c_numer__16;
		r_z1__16 <=c_z1__16;
		r_z0__16 <=c_z0__16;
		r_mut__16 <=c_mut__16;
		
		//for 17
		r_x__17	<=c_x__17;
		r_y__17	<=c_y__17;
		r_z__17	<=c_z__17;
		r_ux__17	<=c_ux__17;
		r_uy__17	<=c_uy__17;
		r_uz__17	<=c_uz__17;
		r_sz__17	<=c_sz__17;
		r_sr__17	<=c_sr__17;
		r_sleftz__17	<=c_sleftz__17;
		r_sleftr__17	<=c_sleftr__17;
		r_weight__17	<=c_weight__17;
		r_layer__17	<=c_layer__17;
		r_dead__17	<=c_dead__17;
		r_hit__17	<=c_hit__17;
		r_diff__17 <=c_diff__17;
		r_dl_b__17 <=c_dl_b__17;
		r_numer__17 <=c_numer__17;
		r_z1__17 <=c_z1__17;
		r_z0__17 <=c_z0__17;
		r_mut__17 <=c_mut__17;
		
		//for 18
		r_x__18	<=c_x__18;
		r_y__18	<=c_y__18;
		r_z__18	<=c_z__18;
		r_ux__18	<=c_ux__18;
		r_uy__18	<=c_uy__18;
		r_uz__18	<=c_uz__18;
		r_sz__18	<=c_sz__18;
		r_sr__18	<=c_sr__18;
		r_sleftz__18	<=c_sleftz__18;
		r_sleftr__18	<=c_sleftr__18;
		r_weight__18	<=c_weight__18;
		r_layer__18	<=c_layer__18;
		r_dead__18	<=c_dead__18;
		r_hit__18	<=c_hit__18;
		r_diff__18 <=c_diff__18;
		r_dl_b__18 <=c_dl_b__18;
		r_numer__18 <=c_numer__18;
		r_z1__18 <=c_z1__18;
		r_z0__18 <=c_z0__18;
		r_mut__18 <=c_mut__18;
		
		//for 19
		r_x__19	<=c_x__19;
		r_y__19	<=c_y__19;
		r_z__19	<=c_z__19;
		r_ux__19	<=c_ux__19;
		r_uy__19	<=c_uy__19;
		r_uz__19	<=c_uz__19;
		r_sz__19	<=c_sz__19;
		r_sr__19	<=c_sr__19;
		r_sleftz__19	<=c_sleftz__19;
		r_sleftr__19	<=c_sleftr__19;
		r_weight__19	<=c_weight__19;
		r_layer__19	<=c_layer__19;
		r_dead__19	<=c_dead__19;
		r_hit__19	<=c_hit__19;
		r_diff__19 <=c_diff__19;
		r_dl_b__19 <=c_dl_b__19;
		r_numer__19 <=c_numer__19;
		r_z1__19 <=c_z1__19;
		r_z0__19 <=c_z0__19;
		r_mut__19 <=c_mut__19;
		
		//for 20
		r_x__20	<=c_x__20;
		r_y__20	<=c_y__20;
		r_z__20	<=c_z__20;
		r_ux__20	<=c_ux__20;
		r_uy__20	<=c_uy__20;
		r_uz__20	<=c_uz__20;
		r_sz__20	<=c_sz__20;
		r_sr__20	<=c_sr__20;
		r_sleftz__20	<=c_sleftz__20;
		r_sleftr__20	<=c_sleftr__20;
		r_weight__20	<=c_weight__20;
		r_layer__20	<=c_layer__20;
		r_dead__20	<=c_dead__20;
		r_hit__20	<=c_hit__20;
		r_diff__20 <=c_diff__20;
		r_dl_b__20 <=c_dl_b__20;
		r_numer__20 <=c_numer__20;
		r_z1__20 <=c_z1__20;
		r_z0__20 <=c_z0__20;
		r_mut__20 <=c_mut__20;
		
		
		//for 21
		r_x__21	<=c_x__21;
		r_y__21	<=c_y__21;
		r_z__21	<=c_z__21;
		r_ux__21	<=c_ux__21;
		r_uy__21	<=c_uy__21;
		r_uz__21	<=c_uz__21;
		r_sz__21	<=c_sz__21;
		r_sr__21	<=c_sr__21;
		r_sleftz__21	<=c_sleftz__21;
		r_sleftr__21	<=c_sleftr__21;
		r_weight__21	<=c_weight__21;
		r_layer__21	<=c_layer__21;
		r_dead__21	<=c_dead__21;
		r_hit__21	<=c_hit__21;
		r_diff__21 <=c_diff__21;
		r_dl_b__21 <=c_dl_b__21;
		r_numer__21 <=c_numer__21;
		r_z1__21 <=c_z1__21;
		r_z0__21 <=c_z0__21;
		r_mut__21 <=c_mut__21;
		
		//for 22
		r_x__22	<=c_x__22;
		r_y__22	<=c_y__22;
		r_z__22	<=c_z__22;
		r_ux__22	<=c_ux__22;
		r_uy__22	<=c_uy__22;
		r_uz__22	<=c_uz__22;
		r_sz__22	<=c_sz__22;
		r_sr__22	<=c_sr__22;
		r_sleftz__22	<=c_sleftz__22;
		r_sleftr__22	<=c_sleftr__22;
		r_weight__22	<=c_weight__22;
		r_layer__22	<=c_layer__22;
		r_dead__22	<=c_dead__22;
		r_hit__22	<=c_hit__22;
		r_diff__22 <=c_diff__22;
		r_dl_b__22 <=c_dl_b__22;
		r_numer__22 <=c_numer__22;
		r_z1__22 <=c_z1__22;
		r_z0__22 <=c_z0__22;
		r_mut__22 <=c_mut__22;
		
		//for 23
		r_x__23	<=c_x__23;
		r_y__23	<=c_y__23;
		r_z__23	<=c_z__23;
		r_ux__23	<=c_ux__23;
		r_uy__23	<=c_uy__23;
		r_uz__23	<=c_uz__23;
		r_sz__23	<=c_sz__23;
		r_sr__23	<=c_sr__23;
		r_sleftz__23	<=c_sleftz__23;
		r_sleftr__23	<=c_sleftr__23;
		r_weight__23	<=c_weight__23;
		r_layer__23	<=c_layer__23;
		r_dead__23	<=c_dead__23;
		r_hit__23	<=c_hit__23;
		r_diff__23 <=c_diff__23;
		r_dl_b__23 <=c_dl_b__23;
		r_numer__23 <=c_numer__23;
		r_z1__23 <=c_z1__23;
		r_z0__23 <=c_z0__23;
		r_mut__23 <=c_mut__23;
		
		//for 24
		r_x__24	<=c_x__24;
		r_y__24	<=c_y__24;
		r_z__24	<=c_z__24;
		r_ux__24	<=c_ux__24;
		r_uy__24	<=c_uy__24;
		r_uz__24	<=c_uz__24;
		r_sz__24	<=c_sz__24;
		r_sr__24	<=c_sr__24;
		r_sleftz__24	<=c_sleftz__24;
		r_sleftr__24	<=c_sleftr__24;
		r_weight__24	<=c_weight__24;
		r_layer__24	<=c_layer__24;
		r_dead__24	<=c_dead__24;
		r_hit__24	<=c_hit__24;
		r_diff__24 <=c_diff__24;
		r_dl_b__24 <=c_dl_b__24;
		r_numer__24 <=c_numer__24;
		r_z1__24 <=c_z1__24;
		r_z0__24 <=c_z0__24;
		r_mut__24 <=c_mut__24;
		
		//for 25
		r_x__25	<=c_x__25;
		r_y__25	<=c_y__25;
		r_z__25	<=c_z__25;
		r_ux__25	<=c_ux__25;
		r_uy__25	<=c_uy__25;
		r_uz__25	<=c_uz__25;
		r_sz__25	<=c_sz__25;
		r_sr__25	<=c_sr__25;
		r_sleftz__25	<=c_sleftz__25;
		r_sleftr__25	<=c_sleftr__25;
		r_weight__25	<=c_weight__25;
		r_layer__25	<=c_layer__25;
		r_dead__25	<=c_dead__25;
		r_hit__25	<=c_hit__25;
		r_diff__25 <=c_diff__25;
		r_dl_b__25 <=c_dl_b__25;
		r_numer__25 <=c_numer__25;
		r_z1__25 <=c_z1__25;
		r_z0__25 <=c_z0__25;
		r_mut__25 <=c_mut__25;
		
		//for 26
		r_x__26	<=c_x__26;
		r_y__26	<=c_y__26;
		r_z__26	<=c_z__26;
		r_ux__26	<=c_ux__26;
		r_uy__26	<=c_uy__26;
		r_uz__26	<=c_uz__26;
		r_sz__26	<=c_sz__26;
		r_sr__26	<=c_sr__26;
		r_sleftz__26	<=c_sleftz__26;
		r_sleftr__26	<=c_sleftr__26;
		r_weight__26	<=c_weight__26;
		r_layer__26	<=c_layer__26;
		r_dead__26	<=c_dead__26;
		r_hit__26	<=c_hit__26;
		r_diff__26 <=c_diff__26;
		r_dl_b__26 <=c_dl_b__26;
		r_numer__26 <=c_numer__26;
		r_z1__26 <=c_z1__26;
		r_z0__26 <=c_z0__26;
		r_mut__26 <=c_mut__26;
		
		//for 27
		r_x__27	<=c_x__27;
		r_y__27	<=c_y__27;
		r_z__27	<=c_z__27;
		r_ux__27	<=c_ux__27;
		r_uy__27	<=c_uy__27;
		r_uz__27	<=c_uz__27;
		r_sz__27	<=c_sz__27;
		r_sr__27	<=c_sr__27;
		r_sleftz__27	<=c_sleftz__27;
		r_sleftr__27	<=c_sleftr__27;
		r_weight__27	<=c_weight__27;
		r_layer__27	<=c_layer__27;
		r_dead__27	<=c_dead__27;
		r_hit__27	<=c_hit__27;
		r_diff__27 <=c_diff__27;
		r_dl_b__27 <=c_dl_b__27;
		r_numer__27 <=c_numer__27;
		r_z1__27 <=c_z1__27;
		r_z0__27 <=c_z0__27;
		r_mut__27 <=c_mut__27;
		
		//for 28
		r_x__28	<=c_x__28;
		r_y__28	<=c_y__28;
		r_z__28	<=c_z__28;
		r_ux__28	<=c_ux__28;
		r_uy__28	<=c_uy__28;
		r_uz__28	<=c_uz__28;
		r_sz__28	<=c_sz__28;
		r_sr__28	<=c_sr__28;
		r_sleftz__28	<=c_sleftz__28;
		r_sleftr__28	<=c_sleftr__28;
		r_weight__28	<=c_weight__28;
		r_layer__28	<=c_layer__28;
		r_dead__28	<=c_dead__28;
		r_hit__28	<=c_hit__28;
		r_diff__28 <=c_diff__28;
		r_dl_b__28 <=c_dl_b__28;
		r_numer__28 <=c_numer__28;
		r_z1__28 <=c_z1__28;
		r_z0__28 <=c_z0__28;
		r_mut__28 <=c_mut__28;
		
		//for 29
		r_x__29	<=c_x__29;
		r_y__29	<=c_y__29;
		r_z__29	<=c_z__29;
		r_ux__29	<=c_ux__29;
		r_uy__29	<=c_uy__29;
		r_uz__29	<=c_uz__29;
		r_sz__29	<=c_sz__29;
		r_sr__29	<=c_sr__29;
		r_sleftz__29	<=c_sleftz__29;
		r_sleftr__29	<=c_sleftr__29;
		r_weight__29	<=c_weight__29;
		r_layer__29	<=c_layer__29;
		r_dead__29	<=c_dead__29;
		r_hit__29	<=c_hit__29;
		r_diff__29 <=c_diff__29;
		r_dl_b__29 <=c_dl_b__29;
		r_numer__29 <=c_numer__29;
		r_z1__29 <=c_z1__29;
		r_z0__29 <=c_z0__29;
		r_mut__29 <=c_mut__29;
		
		//for 30
		r_x__30	<=c_x__30;
		r_y__30	<=c_y__30;
		r_z__30	<=c_z__30;
		r_ux__30	<=c_ux__30;
		r_uy__30	<=c_uy__30;
		r_uz__30	<=c_uz__30;
		r_sz__30	<=c_sz__30;
		r_sr__30	<=c_sr__30;
		r_sleftz__30	<=c_sleftz__30;
		r_sleftr__30	<=c_sleftr__30;
		r_weight__30	<=c_weight__30;
		r_layer__30	<=c_layer__30;
		r_dead__30	<=c_dead__30;
		r_hit__30	<=c_hit__30;
		r_diff__30 <=c_diff__30;
		r_dl_b__30 <=c_dl_b__30;
		r_numer__30 <=c_numer__30;
		r_z1__30 <=c_z1__30;
		r_z0__30 <=c_z0__30;
		r_mut__30 <=c_mut__30;
		
		//for 31
		r_x__31	<=c_x__31;
		r_y__31	<=c_y__31;
		r_z__31	<=c_z__31;
		r_ux__31	<=c_ux__31;
		r_uy__31	<=c_uy__31;
		r_uz__31	<=c_uz__31;
		r_sz__31	<=c_sz__31;
		r_sr__31	<=c_sr__31;
		r_sleftz__31	<=c_sleftz__31;
		r_sleftr__31	<=c_sleftr__31;
		r_weight__31	<=c_weight__31;
		r_layer__31	<=c_layer__31;
		r_dead__31	<=c_dead__31;
		r_hit__31	<=c_hit__31;
		r_diff__31 <=c_diff__31;
		r_dl_b__31 <=c_dl_b__31;
		r_numer__31 <=c_numer__31;
		r_z1__31 <=c_z1__31;
		r_z0__31 <=c_z0__31;
		r_mut__31 <=c_mut__31;
		
		//for 32
		r_x__32	<=c_x__32;
		r_y__32	<=c_y__32;
		r_z__32	<=c_z__32;
		r_ux__32	<=c_ux__32;
		r_uy__32	<=c_uy__32;
		r_uz__32	<=c_uz__32;
		r_sz__32	<=c_sz__32;
		r_sr__32	<=c_sr__32;
		r_sleftz__32	<=c_sleftz__32;
		r_sleftr__32	<=c_sleftr__32;
		r_weight__32	<=c_weight__32;
		r_layer__32	<=c_layer__32;
		r_dead__32	<=c_dead__32;
		r_hit__32	<=c_hit__32;
		r_diff__32 <=c_diff__32;
		r_dl_b__32 <=c_dl_b__32;
		r_numer__32 <=c_numer__32;
		r_z1__32 <=c_z1__32;
		r_z0__32 <=c_z0__32;
		r_mut__32 <=c_mut__32;
		
		//for 33
		r_x__33	<=c_x__33;
		r_y__33	<=c_y__33;
		r_z__33	<=c_z__33;
		r_ux__33	<=c_ux__33;
		r_uy__33	<=c_uy__33;
		r_uz__33	<=c_uz__33;
		r_sz__33	<=c_sz__33;
		r_sr__33	<=c_sr__33;
		r_sleftz__33	<=c_sleftz__33;
		r_sleftr__33	<=c_sleftr__33;
		r_weight__33	<=c_weight__33;
		r_layer__33	<=c_layer__33;
		r_dead__33	<=c_dead__33;
		r_hit__33	<=c_hit__33;
		r_diff__33 <=c_diff__33;
		r_dl_b__33 <=c_dl_b__33;
		r_numer__33 <=c_numer__33;
		r_z1__33 <=c_z1__33;
		r_z0__33 <=c_z0__33;
		r_mut__33 <=c_mut__33;
		
		//for 34
		r_x__34	<=c_x__34;
		r_y__34	<=c_y__34;
		r_z__34	<=c_z__34;
		r_ux__34	<=c_ux__34;
		r_uy__34	<=c_uy__34;
		r_uz__34	<=c_uz__34;
		r_sz__34	<=c_sz__34;
		r_sr__34	<=c_sr__34;
		r_sleftz__34	<=c_sleftz__34;
		r_sleftr__34	<=c_sleftr__34;
		r_weight__34	<=c_weight__34;
		r_layer__34	<=c_layer__34;
		r_dead__34	<=c_dead__34;
		r_hit__34	<=c_hit__34;
		r_diff__34 <=c_diff__34;
		r_dl_b__34 <=c_dl_b__34;
		r_numer__34 <=c_numer__34;
		r_z1__34 <=c_z1__34;
		r_z0__34 <=c_z0__34;
		r_mut__34 <=c_mut__34;
		
		//for 35
		r_x__35	<=c_x__35;
		r_y__35	<=c_y__35;
		r_z__35	<=c_z__35;
		r_ux__35	<=c_ux__35;
		r_uy__35	<=c_uy__35;
		r_uz__35	<=c_uz__35;
		r_sz__35	<=c_sz__35;
		r_sr__35	<=c_sr__35;
		r_sleftz__35	<=c_sleftz__35;
		r_sleftr__35	<=c_sleftr__35;
		r_weight__35	<=c_weight__35;
		r_layer__35	<=c_layer__35;
		r_dead__35	<=c_dead__35;
		r_hit__35	<=c_hit__35;
		r_diff__35 <=c_diff__35;
		r_dl_b__35 <=c_dl_b__35;
		r_numer__35 <=c_numer__35;
		r_z1__35 <=c_z1__35;
		r_z0__35 <=c_z0__35;
		r_mut__35 <=c_mut__35;
		
		//for 36
		r_x__36	<=c_x__36;
		r_y__36	<=c_y__36;
		r_z__36	<=c_z__36;
		r_ux__36	<=c_ux__36;
		r_uy__36	<=c_uy__36;
		r_uz__36	<=c_uz__36;
		r_sz__36	<=c_sz__36;
		r_sr__36	<=c_sr__36;
		r_sleftz__36	<=c_sleftz__36;
		r_sleftr__36	<=c_sleftr__36;
		r_weight__36	<=c_weight__36;
		r_layer__36	<=c_layer__36;
		r_dead__36	<=c_dead__36;
		r_hit__36	<=c_hit__36;
		r_diff__36 <=c_diff__36;
		r_dl_b__36 <=c_dl_b__36;
		r_numer__36 <=c_numer__36;
		r_z1__36 <=c_z1__36;
		r_z0__36 <=c_z0__36;
		r_mut__36 <=c_mut__36;
		
		//for 37
		r_x__37	<=c_x__37;
		r_y__37	<=c_y__37;
		r_z__37	<=c_z__37;
		r_ux__37	<=c_ux__37;
		r_uy__37	<=c_uy__37;
		r_uz__37	<=c_uz__37;
		r_sz__37	<=c_sz__37;
		r_sr__37	<=c_sr__37;
		r_sleftz__37	<=c_sleftz__37;
		r_sleftr__37	<=c_sleftr__37;
		r_weight__37	<=c_weight__37;
		r_layer__37	<=c_layer__37;
		r_dead__37	<=c_dead__37;
		r_hit__37	<=c_hit__37;
		r_diff__37 <=c_diff__37;
		r_dl_b__37 <=c_dl_b__37;
		r_numer__37 <=c_numer__37;
		r_z1__37 <=c_z1__37;
		r_z0__37 <=c_z0__37;
		r_mut__37 <=c_mut__37;
		
		//for 38
		r_x__38	<=c_x__38;
		r_y__38	<=c_y__38;
		r_z__38	<=c_z__38;
		r_ux__38	<=c_ux__38;
		r_uy__38	<=c_uy__38;
		r_uz__38	<=c_uz__38;
		r_sz__38	<=c_sz__38;
		r_sr__38	<=c_sr__38;
		r_sleftz__38	<=c_sleftz__38;
		r_sleftr__38	<=c_sleftr__38;
		r_weight__38	<=c_weight__38;
		r_layer__38	<=c_layer__38;
		r_dead__38	<=c_dead__38;
		r_hit__38	<=c_hit__38;
		r_diff__38 <=c_diff__38;
		r_dl_b__38 <=c_dl_b__38;
		r_numer__38 <=c_numer__38;
		r_z1__38 <=c_z1__38;
		r_z0__38 <=c_z0__38;
		r_mut__38 <=c_mut__38;
		
		//for 39
		r_x__39	<=c_x__39;
		r_y__39	<=c_y__39;
		r_z__39	<=c_z__39;
		r_ux__39	<=c_ux__39;
		r_uy__39	<=c_uy__39;
		r_uz__39	<=c_uz__39;
		r_sz__39	<=c_sz__39;
		r_sr__39	<=c_sr__39;
		r_sleftz__39	<=c_sleftz__39;
		r_sleftr__39	<=c_sleftr__39;
		r_weight__39	<=c_weight__39;
		r_layer__39	<=c_layer__39;
		r_dead__39	<=c_dead__39;
		r_hit__39	<=c_hit__39;
		r_diff__39 <=c_diff__39;
		r_dl_b__39 <=c_dl_b__39;
		r_numer__39 <=c_numer__39;
		r_z1__39 <=c_z1__39;
		r_z0__39 <=c_z0__39;
		r_mut__39 <=c_mut__39;
		
		//for 40
		r_x__40	<=c_x__40;
		r_y__40	<=c_y__40;
		r_z__40	<=c_z__40;
		r_ux__40	<=c_ux__40;
		r_uy__40	<=c_uy__40;
		r_uz__40	<=c_uz__40;
		r_sz__40	<=c_sz__40;
		r_sr__40	<=c_sr__40;
		r_sleftz__40	<=c_sleftz__40;
		r_sleftr__40	<=c_sleftr__40;
		r_weight__40	<=c_weight__40;
		r_layer__40	<=c_layer__40;
		r_dead__40	<=c_dead__40;
		r_hit__40	<=c_hit__40;
		r_diff__40 <=c_diff__40;
		r_dl_b__40 <=c_dl_b__40;
		r_numer__40 <=c_numer__40;
		r_z1__40 <=c_z1__40;
		r_z0__40 <=c_z0__40;
		r_mut__40 <=c_mut__40;
		
		//for 41
		r_x__41	<=c_x__41;
		r_y__41	<=c_y__41;
		r_z__41	<=c_z__41;
		r_ux__41	<=c_ux__41;
		r_uy__41	<=c_uy__41;
		r_uz__41	<=c_uz__41;
		r_sz__41	<=c_sz__41;
		r_sr__41	<=c_sr__41;
		r_sleftz__41	<=c_sleftz__41;
		r_sleftr__41	<=c_sleftr__41;
		r_weight__41	<=c_weight__41;
		r_layer__41	<=c_layer__41;
		r_dead__41	<=c_dead__41;
		r_hit__41	<=c_hit__41;
		r_diff__41 <=c_diff__41;
		r_dl_b__41 <=c_dl_b__41;
		r_numer__41 <=c_numer__41;
		r_z1__41 <=c_z1__41;
		r_z0__41 <=c_z0__41;
		r_mut__41 <=c_mut__41;
		
		//for 42
		r_x__42	<=c_x__42;
		r_y__42	<=c_y__42;
		r_z__42	<=c_z__42;
		r_ux__42	<=c_ux__42;
		r_uy__42	<=c_uy__42;
		r_uz__42	<=c_uz__42;
		r_sz__42	<=c_sz__42;
		r_sr__42	<=c_sr__42;
		r_sleftz__42	<=c_sleftz__42;
		r_sleftr__42	<=c_sleftr__42;
		r_weight__42	<=c_weight__42;
		r_layer__42	<=c_layer__42;
		r_dead__42	<=c_dead__42;
		r_hit__42	<=c_hit__42;
		r_diff__42 <=c_diff__42;
		r_dl_b__42 <=c_dl_b__42;
		r_numer__42 <=c_numer__42;
		r_z1__42 <=c_z1__42;
		r_z0__42 <=c_z0__42;
		r_mut__42 <=c_mut__42;
		
		//for 43
		r_x__43	<=c_x__43;
		r_y__43	<=c_y__43;
		r_z__43	<=c_z__43;
		r_ux__43	<=c_ux__43;
		r_uy__43	<=c_uy__43;
		r_uz__43	<=c_uz__43;
		r_sz__43	<=c_sz__43;
		r_sr__43	<=c_sr__43;
		r_sleftz__43	<=c_sleftz__43;
		r_sleftr__43	<=c_sleftr__43;
		r_weight__43	<=c_weight__43;
		r_layer__43	<=c_layer__43;
		r_dead__43	<=c_dead__43;
		r_hit__43	<=c_hit__43;
		r_diff__43 <=c_diff__43;
		r_dl_b__43 <=c_dl_b__43;
		r_numer__43 <=c_numer__43;
		r_z1__43 <=c_z1__43;
		r_z0__43 <=c_z0__43;
		r_mut__43 <=c_mut__43;
		
		//for 44
		r_x__44	<=c_x__44;
		r_y__44	<=c_y__44;
		r_z__44	<=c_z__44;
		r_ux__44	<=c_ux__44;
		r_uy__44	<=c_uy__44;
		r_uz__44	<=c_uz__44;
		r_sz__44	<=c_sz__44;
		r_sr__44	<=c_sr__44;
		r_sleftz__44	<=c_sleftz__44;
		r_sleftr__44	<=c_sleftr__44;
		r_weight__44	<=c_weight__44;
		r_layer__44	<=c_layer__44;
		r_dead__44	<=c_dead__44;
		r_hit__44	<=c_hit__44;
		r_diff__44 <=c_diff__44;
		r_dl_b__44 <=c_dl_b__44;
		r_numer__44 <=c_numer__44;
		r_z1__44 <=c_z1__44;
		r_z0__44 <=c_z0__44;
		r_mut__44 <=c_mut__44;
		
		//for 45
		r_x__45	<=c_x__45;
		r_y__45	<=c_y__45;
		r_z__45	<=c_z__45;
		r_ux__45	<=c_ux__45;
		r_uy__45	<=c_uy__45;
		r_uz__45	<=c_uz__45;
		r_sz__45	<=c_sz__45;
		r_sr__45	<=c_sr__45;
		r_sleftz__45	<=c_sleftz__45;
		r_sleftr__45	<=c_sleftr__45;
		r_weight__45	<=c_weight__45;
		r_layer__45	<=c_layer__45;
		r_dead__45	<=c_dead__45;
		r_hit__45	<=c_hit__45;
		r_diff__45 <=c_diff__45;
		r_dl_b__45 <=c_dl_b__45;
		r_numer__45 <=c_numer__45;
		r_z1__45 <=c_z1__45;
		r_z0__45 <=c_z0__45;
		r_mut__45 <=c_mut__45;
		
		//for 46
		r_x__46	<=c_x__46;
		r_y__46	<=c_y__46;
		r_z__46	<=c_z__46;
		r_ux__46	<=c_ux__46;
		r_uy__46	<=c_uy__46;
		r_uz__46	<=c_uz__46;
		r_sz__46	<=c_sz__46;
		r_sr__46	<=c_sr__46;
		r_sleftz__46	<=c_sleftz__46;
		r_sleftr__46	<=c_sleftr__46;
		r_weight__46	<=c_weight__46;
		r_layer__46	<=c_layer__46;
		r_dead__46	<=c_dead__46;
		r_hit__46	<=c_hit__46;
		r_diff__46 <=c_diff__46;
		r_dl_b__46 <=c_dl_b__46;
		r_numer__46 <=c_numer__46;
		r_z1__46 <=c_z1__46;
		r_z0__46 <=c_z0__46;
		r_mut__46 <=c_mut__46;
		
		//for 47
		r_x__47	<=c_x__47;
		r_y__47	<=c_y__47;
		r_z__47	<=c_z__47;
		r_ux__47	<=c_ux__47;
		r_uy__47	<=c_uy__47;
		r_uz__47	<=c_uz__47;
		r_sz__47	<=c_sz__47;
		r_sr__47	<=c_sr__47;
		r_sleftz__47	<=c_sleftz__47;
		r_sleftr__47	<=c_sleftr__47;
		r_weight__47	<=c_weight__47;
		r_layer__47	<=c_layer__47;
		r_dead__47	<=c_dead__47;
		r_hit__47	<=c_hit__47;
		r_diff__47 <=c_diff__47;
		r_dl_b__47 <=c_dl_b__47;
		r_numer__47 <=c_numer__47;
		r_z1__47 <=c_z1__47;
		r_z0__47 <=c_z0__47;
		r_mut__47 <=c_mut__47;
		
		//for 48
		r_x__48	<=c_x__48;
		r_y__48	<=c_y__48;
		r_z__48	<=c_z__48;
		r_ux__48	<=c_ux__48;
		r_uy__48	<=c_uy__48;
		r_uz__48	<=c_uz__48;
		r_sz__48	<=c_sz__48;
		r_sr__48	<=c_sr__48;
		r_sleftz__48	<=c_sleftz__48;
		r_sleftr__48	<=c_sleftr__48;
		r_weight__48	<=c_weight__48;
		r_layer__48	<=c_layer__48;
		r_dead__48	<=c_dead__48;
		r_hit__48	<=c_hit__48;
		r_diff__48 <=c_diff__48;
		r_dl_b__48 <=c_dl_b__48;
		r_numer__48 <=c_numer__48;
		r_z1__48 <=c_z1__48;
		r_z0__48 <=c_z0__48;
		r_mut__48 <=c_mut__48;
		
		//for 49
		r_x__49	<=c_x__49;
		r_y__49	<=c_y__49;
		r_z__49	<=c_z__49;
		r_ux__49	<=c_ux__49;
		r_uy__49	<=c_uy__49;
		r_uz__49	<=c_uz__49;
		r_sz__49	<=c_sz__49;
		r_sr__49	<=c_sr__49;
		r_sleftz__49	<=c_sleftz__49;
		r_sleftr__49	<=c_sleftr__49;
		r_weight__49	<=c_weight__49;
		r_layer__49	<=c_layer__49;
		r_dead__49	<=c_dead__49;
		r_hit__49	<=c_hit__49;
		r_diff__49 <=c_diff__49;
		r_dl_b__49 <=c_dl_b__49;
		r_numer__49 <=c_numer__49;
		r_z1__49 <=c_z1__49;
		r_z0__49 <=c_z0__49;
		r_mut__49 <=c_mut__49;
		
		//for 50
		r_x__50	<=c_x__50;
		r_y__50	<=c_y__50;
		r_z__50	<=c_z__50;
		r_ux__50	<=c_ux__50;
		r_uy__50	<=c_uy__50;
		r_uz__50	<=c_uz__50;
		r_sz__50	<=c_sz__50;
		r_sr__50	<=c_sr__50;
		r_sleftz__50	<=c_sleftz__50;
		r_sleftr__50	<=c_sleftr__50;
		r_weight__50	<=c_weight__50;
		r_layer__50	<=c_layer__50;
		r_dead__50	<=c_dead__50;
		r_hit__50	<=c_hit__50;
		r_diff__50 <=c_diff__50;
		r_dl_b__50 <=c_dl_b__50;
		r_numer__50 <=c_numer__50;
		r_z1__50 <=c_z1__50;
		r_z0__50 <=c_z0__50;
		r_mut__50 <=c_mut__50;
		
		//for 51
		r_x__51	<=c_x__51;
		r_y__51	<=c_y__51;
		r_z__51	<=c_z__51;
		r_ux__51	<=c_ux__51;
		r_uy__51	<=c_uy__51;
		r_uz__51	<=c_uz__51;
		r_sz__51	<=c_sz__51;
		r_sr__51	<=c_sr__51;
		r_sleftz__51	<=c_sleftz__51;
		r_sleftr__51	<=c_sleftr__51;
		r_weight__51	<=c_weight__51;
		r_layer__51	<=c_layer__51;
		r_dead__51	<=c_dead__51;
		r_hit__51	<=c_hit__51;
		r_diff__51 <=c_diff__51;
		r_dl_b__51 <=c_dl_b__51;
		r_numer__51 <=c_numer__51;
		r_z1__51 <=c_z1__51;
		r_z0__51 <=c_z0__51;
		r_mut__51 <=c_mut__51;
		
		//for 52
		r_x__52	<=c_x__52;
		r_y__52	<=c_y__52;
		r_z__52	<=c_z__52;
		r_ux__52	<=c_ux__52;
		r_uy__52	<=c_uy__52;
		r_uz__52	<=c_uz__52;
		r_sz__52	<=c_sz__52;
		r_sr__52	<=c_sr__52;
		r_sleftz__52	<=c_sleftz__52;
		r_sleftr__52	<=c_sleftr__52;
		r_weight__52	<=c_weight__52;
		r_layer__52	<=c_layer__52;
		r_dead__52	<=c_dead__52;
		r_hit__52	<=c_hit__52;
		r_diff__52 <=c_diff__52;
		r_dl_b__52 <=c_dl_b__52;
		r_numer__52 <=c_numer__52;
		r_z1__52 <=c_z1__52;
		r_z0__52 <=c_z0__52;
		r_mut__52 <=c_mut__52;
		
		//for 53
		r_x__53	<=c_x__53;
		r_y__53	<=c_y__53;
		r_z__53	<=c_z__53;
		r_ux__53	<=c_ux__53;
		r_uy__53	<=c_uy__53;
		r_uz__53	<=c_uz__53;
		r_sz__53	<=c_sz__53;
		r_sr__53	<=c_sr__53;
		r_sleftz__53	<=c_sleftz__53;
		r_sleftr__53	<=c_sleftr__53;
		r_weight__53	<=c_weight__53;
		r_layer__53	<=c_layer__53;
		r_dead__53	<=c_dead__53;
		r_hit__53	<=c_hit__53;
		r_diff__53 <=c_diff__53;
		r_dl_b__53 <=c_dl_b__53;
		r_numer__53 <=c_numer__53;
		r_z1__53 <=c_z1__53;
		r_z0__53 <=c_z0__53;
		r_mut__53 <=c_mut__53;
		
		//for 54
		r_x__54	<=c_x__54;
		r_y__54	<=c_y__54;
		r_z__54	<=c_z__54;
		r_ux__54	<=c_ux__54;
		r_uy__54	<=c_uy__54;
		r_uz__54	<=c_uz__54;
		r_sz__54	<=c_sz__54;
		r_sr__54	<=c_sr__54;
		r_sleftz__54	<=c_sleftz__54;
		r_sleftr__54	<=c_sleftr__54;
		r_weight__54	<=c_weight__54;
		r_layer__54	<=c_layer__54;
		r_dead__54	<=c_dead__54;
		r_hit__54	<=c_hit__54;
		r_diff__54 <=c_diff__54;
		r_dl_b__54 <=c_dl_b__54;
		r_numer__54 <=c_numer__54;
		r_z1__54 <=c_z1__54;
		r_z0__54 <=c_z0__54;
		r_mut__54 <=c_mut__54;
		
		//for 55
		r_x__55	<=c_x__55;
		r_y__55	<=c_y__55;
		r_z__55	<=c_z__55;
		r_ux__55	<=c_ux__55;
		r_uy__55	<=c_uy__55;
		r_uz__55	<=c_uz__55;
		r_sz__55	<=c_sz__55;
		r_sr__55	<=c_sr__55;
		r_sleftz__55	<=c_sleftz__55;
		r_sleftr__55	<=c_sleftr__55;
		r_weight__55	<=c_weight__55;
		r_layer__55	<=c_layer__55;
		r_dead__55	<=c_dead__55;
		r_hit__55	<=c_hit__55;
		r_diff__55 <=c_diff__55;
		r_dl_b__55 <=c_dl_b__55;
		r_numer__55 <=c_numer__55;
		r_z1__55 <=c_z1__55;
		r_z0__55 <=c_z0__55;
		r_mut__55 <=c_mut__55;
		
		//for 56
		r_x__56	<=c_x__56;
		r_y__56	<=c_y__56;
		r_z__56	<=c_z__56;
		r_ux__56	<=c_ux__56;
		r_uy__56	<=c_uy__56;
		r_uz__56	<=c_uz__56;
		r_sz__56	<=c_sz__56;
		r_sr__56	<=c_sr__56;
		r_sleftz__56	<=c_sleftz__56;
		r_sleftr__56	<=c_sleftr__56;
		r_weight__56	<=c_weight__56;
		r_layer__56	<=c_layer__56;
		r_dead__56	<=c_dead__56;
		r_hit__56	<=c_hit__56;
		r_diff__56 <=c_diff__56;
		r_dl_b__56 <=c_dl_b__56;
		r_numer__56 <=c_numer__56;
		r_z1__56 <=c_z1__56;
		r_z0__56 <=c_z0__56;
		r_mut__56 <=c_mut__56;
		
		//for 57
		r_x__57	<=c_x__57;
		r_y__57	<=c_y__57;
		r_z__57	<=c_z__57;
		r_ux__57	<=c_ux__57;
		r_uy__57	<=c_uy__57;
		r_uz__57	<=c_uz__57;
		r_sz__57	<=c_sz__57;
		r_sr__57	<=c_sr__57;
		r_sleftz__57	<=c_sleftz__57;
		r_sleftr__57	<=c_sleftr__57;
		r_weight__57	<=c_weight__57;
		r_layer__57	<=c_layer__57;
		r_dead__57	<=c_dead__57;
		r_hit__57	<=c_hit__57;
		r_diff__57 <=c_diff__57;
		r_dl_b__57 <=c_dl_b__57;
		r_numer__57 <=c_numer__57;
		r_z1__57 <=c_z1__57;
		r_z0__57 <=c_z0__57;
		r_mut__57 <=c_mut__57;
		
		//for 58
		r_x__58	<=c_x__58;
		r_y__58	<=c_y__58;
		r_z__58	<=c_z__58;
		r_ux__58	<=c_ux__58;
		r_uy__58	<=c_uy__58;
		r_uz__58	<=c_uz__58;
		r_sz__58	<=c_sz__58;
		r_sr__58	<=c_sr__58;
		r_sleftz__58	<=c_sleftz__58;
		r_sleftr__58	<=c_sleftr__58;
		r_weight__58	<=c_weight__58;
		r_layer__58	<=c_layer__58;
		r_dead__58	<=c_dead__58;
		r_hit__58	<=c_hit__58;
		r_diff__58 <=c_diff__58;
		r_dl_b__58 <=c_dl_b__58;
		r_numer__58 <=c_numer__58;
		r_z1__58 <=c_z1__58;
		r_z0__58 <=c_z0__58;
		r_mut__58 <=c_mut__58;
		
		//for 59
		r_x__59	<=c_x__59;
		r_y__59	<=c_y__59;
		r_z__59	<=c_z__59;
		r_ux__59	<=c_ux__59;
		r_uy__59	<=c_uy__59;
		r_uz__59	<=c_uz__59;
		r_sz__59	<=c_sz__59;
		r_sr__59	<=c_sr__59;
		r_sleftz__59	<=c_sleftz__59;
		r_sleftr__59	<=c_sleftr__59;
		r_weight__59	<=c_weight__59;
		r_layer__59	<=c_layer__59;
		r_dead__59	<=c_dead__59;
		r_hit__59	<=c_hit__59;
		r_diff__59 <=c_diff__59;
		r_dl_b__59 <=c_dl_b__59;
		r_numer__59 <=c_numer__59;
		r_z1__59 <=c_z1__59;
		r_z0__59 <=c_z0__59;
		r_mut__59 <=c_mut__59;
		
		end
	end
end

endmodule


/////////////////////////////////////////////////////////////
//mult_signed_32_bc 
/////////////////////////////////////////////////////////////
module mult_signed_32_bc ( clock, dataa, datab, result);


	input clock;
	input [31:0] dataa;
	input [31:0] datab;
	output [63:0] result;
	reg [63:0] result;
	
	wire [63:0] prelim_result;
	
	
	wire [31:0] opa;
	wire [31:0] opb;
	wire [31:0] opa_comp;
	wire [31:0] opb_comp;
	
	assign opa_comp =  ((~dataa) + 32'b00000000000000000000000000000001);

	assign opb_comp =  ((~datab) + 32'b00000000000000000000000000000001);

	
	wire opa_is_neg;
	wire opb_is_neg;
	assign opa_is_neg = dataa[31];
	assign opb_is_neg = datab [31];
	assign opa = (opa_is_neg== 1'b1) ? opa_comp:dataa;
	assign opb = (opb_is_neg == 1'b1) ? opb_comp:datab;
	
	
	assign prelim_result = opa * opb ;
	wire sign;
	assign sign = dataa[31] ^ datab[31];
	
	wire [63:0] prelim_result_comp;
	wire [63:0] prelim_result_changed;
	wire [63:0] result_changed;
	assign result_changed = (sign==1'b1)? prelim_result_comp :prelim_result;
	assign prelim_result_comp =  ((~prelim_result) + 1);
	
	always @ (posedge clock)
	begin
	result <= result_changed;
	end
	
	endmodule


/////////////////////////////////////////////////////////////
//signed_div_30 
/////////////////////////////////////////////////////////////
module signed_div_30 (clock , denom , numer, quotient, remain);

input clock;

input [31:0] denom;

input [63:0] numer;

output [63:0] quotient;

output [31:0] remain;

Div_64b div_replace (.clock(clock), .denom(denom), .numer(numer), .quotient(quotient), .remain(remain));

endmodule 
module Hop(     //INPUTS
				 clock, reset, enable,
				 x_boundaryChecker, y_boundaryChecker, z_boundaryChecker,
				 ux_boundaryChecker, uy_boundaryChecker, uz_boundaryChecker,
				 sz_boundaryChecker, sr_boundaryChecker,
				 sleftz_boundaryChecker, sleftr_boundaryChecker,
				 layer_boundaryChecker, weight_boundaryChecker, dead_boundaryChecker,
				 hit_boundaryChecker,

				 //OUTPUTS
				 x_hop, y_hop, z_hop,
				 ux_hop, uy_hop, uz_hop,
				 sz_hop, sr_hop,
				 sleftz_hop, sleftr_hop,
				 layer_hop, weight_hop, dead_hop, hit_hop
				 );

//parameter BIT_WIDTH=32;
//parameter LAYER_WIDTH=3;
//parameter INTMAX=2147483647;
//parameter INTMIN=-2147483648;

input clock;
input reset;
input enable;

input [`BIT_WIDTH-1:0] x_boundaryChecker;
input [`BIT_WIDTH-1:0] y_boundaryChecker;
input [`BIT_WIDTH-1:0] z_boundaryChecker;
input [`BIT_WIDTH-1:0] ux_boundaryChecker;
input [`BIT_WIDTH-1:0] uy_boundaryChecker;
input [`BIT_WIDTH-1:0] uz_boundaryChecker;
input [`BIT_WIDTH-1:0] sz_boundaryChecker;
input [`BIT_WIDTH-1:0] sr_boundaryChecker;
input [`BIT_WIDTH-1:0] sleftz_boundaryChecker;
input [`BIT_WIDTH-1:0] sleftr_boundaryChecker;
input [`LAYER_WIDTH-1:0] layer_boundaryChecker;
input [`BIT_WIDTH-1:0] weight_boundaryChecker;
input	dead_boundaryChecker;
input	hit_boundaryChecker;

output [`BIT_WIDTH-1:0] x_hop;
output [`BIT_WIDTH-1:0] y_hop;
output [`BIT_WIDTH-1:0] z_hop;
output [`BIT_WIDTH-1:0] ux_hop;
output [`BIT_WIDTH-1:0] uy_hop;
output [`BIT_WIDTH-1:0] uz_hop;
output [`BIT_WIDTH-1:0] sz_hop;
output [`BIT_WIDTH-1:0] sr_hop;
output [`BIT_WIDTH-1:0] sleftz_hop;
output [`BIT_WIDTH-1:0] sleftr_hop;
output [`LAYER_WIDTH-1:0]layer_hop;
output [`BIT_WIDTH-1:0] weight_hop;
output dead_hop;
output hit_hop;

//------------Local Variables------------------------
reg [`BIT_WIDTH-1:0] c_x;
reg [`BIT_WIDTH-1:0] c_y;
reg [`BIT_WIDTH-1:0] c_z;
reg c_dead;

reg [`BIT_WIDTH:0] c_x_big;
reg [`BIT_WIDTH:0] c_y_big;
reg [`BIT_WIDTH:0] c_z_big;

wire [2*`BIT_WIDTH-1:0] c_xmult_big;
wire [2*`BIT_WIDTH-1:0] c_ymult_big;
wire [2*`BIT_WIDTH-1:0] c_zmult_big;

//------------REGISTERED Values------------------------
reg [`BIT_WIDTH-1:0] x_hop;
reg [`BIT_WIDTH-1:0] y_hop;
reg [`BIT_WIDTH-1:0] z_hop;
reg [`BIT_WIDTH-1:0] ux_hop;
reg [`BIT_WIDTH-1:0] uy_hop;
reg [`BIT_WIDTH-1:0] uz_hop;
reg [`BIT_WIDTH-1:0] sz_hop;
reg [`BIT_WIDTH-1:0] sr_hop;
reg [`BIT_WIDTH-1:0] sleftz_hop;
reg [`BIT_WIDTH-1:0] sleftr_hop;
reg [`LAYER_WIDTH-1:0]layer_hop;
reg [`BIT_WIDTH-1:0] weight_hop;
reg	dead_hop;
reg	hit_hop;

mult_signed_32 u1(sr_boundaryChecker, ux_boundaryChecker, c_xmult_big);
mult_signed_32 u2(sr_boundaryChecker, uy_boundaryChecker, c_ymult_big);
mult_signed_32 u3(sz_boundaryChecker, uz_boundaryChecker, c_zmult_big);

// Determine new (x,y,z) coordinates
always @(c_dead or 
		c_x_big or c_y_big or c_z_big or 
		c_x or c_y or c_z or
		x_boundaryChecker or y_boundaryChecker or z_boundaryChecker or
		c_xmult_big or c_ymult_big or c_zmult_big 
		or hit_boundaryChecker or dead_boundaryChecker)
begin
		
	c_x_big = x_boundaryChecker + c_xmult_big[2*`BIT_WIDTH-2:31];
	c_y_big = y_boundaryChecker + c_ymult_big[2*`BIT_WIDTH-2:31];
	c_z_big = z_boundaryChecker + c_zmult_big[2*`BIT_WIDTH-2:31];


	// Calculate x position, photon dies if outside grid
	if(c_x_big[`BIT_WIDTH] != c_x_big[`BIT_WIDTH-1] && x_boundaryChecker[`BIT_WIDTH-1] == c_xmult_big[2*`BIT_WIDTH-2])
	begin
		if(c_x_big[`BIT_WIDTH] == 1'b0)
		begin
		//	c_dead = 1'b1;
			c_x = `INTMAX;
		end
		else
		begin
		//	c_dead = 1'b1;
			c_x = `INTMIN;
		end
	end 
	else
	begin
		c_x = c_x_big[`BIT_WIDTH-1:0];
	end

	
	// Calculate y position, photon dies if outside grid
	if(c_y_big[`BIT_WIDTH] != c_y_big[`BIT_WIDTH-1] && y_boundaryChecker[`BIT_WIDTH-1] == c_ymult_big[2*`BIT_WIDTH-2])
	begin
		if(c_y_big[`BIT_WIDTH] == 1'b0)
		begin
		//	c_dead = 1'b1;
			c_y = `INTMAX;
		end
		else
		begin
		//	c_dead = 1'b1;
			c_y = `INTMIN;
		end
	end
	else
	begin
		c_y = c_y_big[`BIT_WIDTH-1:0];
	end
	
	// Calculate z position, photon dies if outside grid
	if(hit_boundaryChecker) 
	begin
		c_z = z_boundaryChecker;
	end
	else if(c_z_big[`BIT_WIDTH] != c_z_big[`BIT_WIDTH-1] && z_boundaryChecker[`BIT_WIDTH-1] == c_zmult_big[2*`BIT_WIDTH-2])
	begin
	//	c_dead = 1'b1;
		c_z = `INTMAX;
	end
	else if (c_z_big[`BIT_WIDTH-1] == 1'b1)
	begin
	//	c_dead = 1'b1;
		c_z = 0;
	end 
	else
	begin
		c_z = c_z_big[`BIT_WIDTH-1:0];
	end
	
	// Calculate c_dead (necessary because odin does not support block statements).
	if( (c_x_big[`BIT_WIDTH] != c_x_big[`BIT_WIDTH-1] && x_boundaryChecker[`BIT_WIDTH-1] == c_xmult_big[2*`BIT_WIDTH-2])
	   |(c_y_big[`BIT_WIDTH] != c_y_big[`BIT_WIDTH-1] && y_boundaryChecker[`BIT_WIDTH-1] == c_ymult_big[2*`BIT_WIDTH-2])
	   |(c_z_big[`BIT_WIDTH] != c_z_big[`BIT_WIDTH-1] && z_boundaryChecker[`BIT_WIDTH-1] == c_zmult_big[2*`BIT_WIDTH-2]) )
	begin
		c_dead = 1'b1;
	end
	else
	begin
		c_dead = dead_boundaryChecker;
	end

end

// latch values
always @ (posedge clock)
begin
	if (reset)
	begin
		// Photon variables
		x_hop <= 0;
		y_hop <= 0;
		z_hop <= 0;
		ux_hop <= 0;
		uy_hop <= 0;
		uz_hop <= 0;
		sz_hop <= 0;
		sr_hop <= 0;
		sleftz_hop <= 0;
		sleftr_hop <= 0;
		layer_hop <= 0;
		weight_hop <= 0;
		dead_hop <= 1'b1;
		hit_hop <= 1'b0;
	end
	else
	begin
		if(enable)
		begin
			// Photon variables
			ux_hop <= ux_boundaryChecker;
			uy_hop <= uy_boundaryChecker;
			uz_hop <= uz_boundaryChecker;
			sz_hop <= sz_boundaryChecker;
			sr_hop <= sr_boundaryChecker;
			sleftz_hop <= sleftz_boundaryChecker;
			sleftr_hop <= sleftr_boundaryChecker;
			layer_hop <= layer_boundaryChecker;
			weight_hop <= weight_boundaryChecker;
			hit_hop <= hit_boundaryChecker;

			x_hop <= c_x;
			y_hop <= c_y;
			z_hop <= c_z;
			dead_hop <= c_dead;
		end			
	end
end

endmodule


/////////////////////////////////////////////////////////////
//mult_signed_32
/////////////////////////////////////////////////////////////
module mult_signed_32(a, b, c);
	input [31:0]a;
	input [31:0]b;
	output [63:0]c;
	reg [63:0]c;
	
	reg is_neg_a;
	reg is_neg_b;
	reg [31:0]a_tmp;
	reg [31:0]b_tmp;
	reg [63:0]c_tmp;


always@(a or b or is_neg_a or is_neg_b or a_tmp or b_tmp or c)
begin

	if(a[31] == 1) begin
		a_tmp = -a;
		is_neg_a = 1;
	end else
	begin
		a_tmp = a;
		is_neg_a = 0;
	end

	if(b[31] == 1) begin
		b_tmp = -b;
		is_neg_b = 1;
	end else
	begin
		b_tmp = b;
		is_neg_b = 0;
	end

	if( is_neg_a != is_neg_b) begin
		c_tmp = -(a_tmp * b_tmp);
	end else
	begin
		c_tmp = (a_tmp * b_tmp);
	end
end

always@(c_tmp)
begin
	c = c_tmp;
end

endmodule


module Roulette ( //INPUTS
                     clock, reset, enable, 
                     x_RouletteMux, y_RouletteMux, z_RouletteMux,  
                     ux_RouletteMux, uy_RouletteMux, uz_RouletteMux, 
                     sz_RouletteMux, sr_RouletteMux, 
                     sleftz_RouletteMux, sleftr_RouletteMux, 
                     layer_RouletteMux, weight_absorber, dead_RouletteMux, 
			
		     //From Random Number Generator in Skeleton.v	
		     randnumber,
                     
                     //OUTPUTS
                     x_Roulette, y_Roulette, z_Roulette,
                     ux_Roulette, uy_Roulette, uz_Roulette, 
                     sz_Roulette, sr_Roulette, 
                     sleftz_Roulette, sleftr_Roulette, 
                     layer_Roulette, weight_Roulette, dead_Roulette
                     ); 

//parameter BIT_WIDTH=32;
//parameter LAYER_WIDTH=3; 

//parameter LEFTSHIFT=3;         // 2^3=8=1/0.125 where 0.125 = CHANCE of roulette
//parameter INTCHANCE=536870912; //Based on 32 bit rand num generator
//parameter MIN_WEIGHT=200; 

input clock;        
input reset;
input enable;

input [`BIT_WIDTH-1:0] x_RouletteMux;
input [`BIT_WIDTH-1:0] y_RouletteMux;
input [`BIT_WIDTH-1:0] z_RouletteMux;
input [`BIT_WIDTH-1:0] ux_RouletteMux;
input [`BIT_WIDTH-1:0] uy_RouletteMux;
input [`BIT_WIDTH-1:0] uz_RouletteMux;
input [`BIT_WIDTH-1:0] sz_RouletteMux;
input [`BIT_WIDTH-1:0] sr_RouletteMux;
input [`BIT_WIDTH-1:0] sleftz_RouletteMux;
input [`BIT_WIDTH-1:0] sleftr_RouletteMux;
input [`LAYER_WIDTH-1:0] layer_RouletteMux;
input [`BIT_WIDTH-1:0] weight_absorber;
input [`BIT_WIDTH-1:0] randnumber;
input dead_RouletteMux;
              
output [`BIT_WIDTH-1:0] x_Roulette;
output [`BIT_WIDTH-1:0] y_Roulette;
output [`BIT_WIDTH-1:0] z_Roulette;
output [`BIT_WIDTH-1:0] ux_Roulette;
output [`BIT_WIDTH-1:0] uy_Roulette;
output [`BIT_WIDTH-1:0] uz_Roulette;
output [`BIT_WIDTH-1:0] sz_Roulette;
output [`BIT_WIDTH-1:0] sr_Roulette;
output [`BIT_WIDTH-1:0] sleftz_Roulette;
output [`BIT_WIDTH-1:0] sleftr_Roulette;
output [`LAYER_WIDTH-1:0]layer_Roulette;
output [`BIT_WIDTH-1:0] weight_Roulette;
output dead_Roulette;

//------------Local Variables------------------------
reg dead_roulette; 
reg [`BIT_WIDTH-1:0] weight_roulette; 
reg [31:0] randBits;             //Hard-coded bitwidth because rng is 32-bit

//------------REGISTERED Values------------------------
reg [`BIT_WIDTH-1:0] x_Roulette;
reg [`BIT_WIDTH-1:0] y_Roulette;
reg [`BIT_WIDTH-1:0] z_Roulette;
reg [`BIT_WIDTH-1:0] ux_Roulette;
reg [`BIT_WIDTH-1:0] uy_Roulette;
reg [`BIT_WIDTH-1:0] uz_Roulette;
reg [`BIT_WIDTH-1:0] sz_Roulette;
reg [`BIT_WIDTH-1:0] sr_Roulette;
reg [`BIT_WIDTH-1:0] sleftz_Roulette;
reg [`BIT_WIDTH-1:0] sleftr_Roulette;
reg [`LAYER_WIDTH-1:0]layer_Roulette;
reg [`BIT_WIDTH-1:0] weight_Roulette;
reg dead_Roulette;
   
always @ (reset or enable or weight_absorber or randBits or randnumber or dead_RouletteMux) begin 
  	//Default case moved inside else statements for odin
	//randBits = randnumber;   //Reading from external random num generator
	//weight_roulette=weight_absorber;	//Avoid inferring a latch
	//dead_roulette=dead_RouletteMux; 
	
	if (reset) begin 
		//Local variables
		weight_roulette=0; 
		dead_roulette=0; 
		randBits=0; 
	end

	else if (enable) begin
		//Set default case
		randBits = randnumber;
		//DO ROULETTE!!!
		if (weight_absorber < `MIN_WEIGHT && !dead_RouletteMux) begin
			//Replicate Operator (same as 32'b000000..., except more flexible)			
			if (weight_absorber== {`BIT_WIDTH{1'b0}}) begin
				dead_roulette = 1;
				weight_roulette = weight_absorber;
			end
				
			else if (randBits < `INTCHANCE) begin // survived the roulette
				dead_roulette=0;
				weight_roulette=weight_absorber << `LEFTSHIFT; //To avoid mult
			end
			
			else begin
				dead_roulette=1;
				weight_roulette = weight_absorber;
			end
		end
		
		//No Roulette
		else  begin
			weight_roulette = weight_absorber;
			dead_roulette = 0;
		end
	end
	
	else //for insurance that this is default case
	begin
		randBits = randnumber;
		weight_roulette = weight_absorber;
		dead_roulette = dead_RouletteMux;
	end
end 

always @ (posedge clock) begin
	if (reset) begin
		x_Roulette <= 0;
		y_Roulette <= 0;
		z_Roulette <= 0;
		ux_Roulette <= 0;
		uy_Roulette <= 0;
		uz_Roulette <= 0;
		sz_Roulette <= 0;
		sr_Roulette <= 0;
		sleftz_Roulette <= 0;
		sleftr_Roulette <= 0;
		layer_Roulette <= 0;
		weight_Roulette <= 0;
		dead_Roulette <= 1'b1;
	end
	
	else if (enable) begin
		//Write through values from Roulette block
		dead_Roulette <= (dead_RouletteMux | dead_roulette);   //OR operator ???
		weight_Roulette <= weight_roulette; //weight_absorber.read();

		//Write through unchanged values
		x_Roulette <= x_RouletteMux;
		y_Roulette <= y_RouletteMux;
		z_Roulette <= z_RouletteMux;

		ux_Roulette <= ux_RouletteMux;
		uy_Roulette <= uy_RouletteMux;
		uz_Roulette <= uz_RouletteMux;
		sz_Roulette <= sz_RouletteMux;
		sr_Roulette <= sr_RouletteMux;
		sleftz_Roulette <= sleftz_RouletteMux;
		sleftr_Roulette <= sleftr_RouletteMux;
		layer_Roulette <= layer_RouletteMux;
	end
end

endmodule


module rng(clk, en, resetn,loadseed_i,seed_i,number_o);
input clk;
input resetn;
input en; 
input loadseed_i;
input [31:0] seed_i;
output [31:0] number_o;

wire [31:0] number_o;

reg [31:0] c_b1, c_b2, c_b3;
reg [31:0] c_s1, c_s2, c_s3;

reg [31:0] r_s1, r_s2, r_s3;

assign number_o = r_s1 ^ r_s2 ^ r_s3;

always @(loadseed_i or seed_i or r_s1 or r_s2 or r_s3)
begin
	if(loadseed_i)
	begin
		c_b1 = 32'b0;
		c_s1 = seed_i;
		c_b2 = 32'b0;
		c_s2 = {seed_i[5:0], seed_i[17], seed_i[18], seed_i[19], seed_i[20], seed_i[25:21], seed_i[31:26], seed_i[16:6]} ^ 32'd1493609598;
		c_b3 = 32'b0;
		c_s3 = {seed_i[23:16], seed_i[5], seed_i[6], seed_i[7], seed_i[15:8], seed_i[4:0], seed_i[31:24]} ^ 32'd3447127471;
	end
	else
	begin
		c_b1 = (((r_s1 << 13) ^ r_s1) >> 19);
		c_s1 = (((r_s1 & 32'd4294967294) << 12) ^ c_b1);
		c_b2 = (((r_s2 << 2) ^ r_s2) >> 25);
		c_s2 = (((r_s2 & 32'd4294967288) << 4) ^ c_b2);
		c_b3 = (((r_s3 << 3) ^ r_s3) >> 11);
		c_s3 = (((r_s3 & 32'd4294967280) << 17) ^ c_b3);
	end
end


//combinate:
always @(posedge clk or negedge resetn)
   begin
   if (!resetn )
      begin
      r_s1 <= 32'b0;
	  r_s2 <= 32'b0;
	  r_s3 <= 32'b0;
      end
   else if (en)   //Originally else only
      begin
		  r_s1 <= c_s1;
		  r_s2 <= c_s2;
		  r_s3 <= c_s3;
	  end
   end

endmodule



module LogCalc(clock, reset, enable, in_x, log_x);

//parameter BIT_WIDTH=32;
//parameter MANTISSA_PRECISION=10;
//parameter LOG2_BIT_WIDTH = 6;
//parameter LOG2=93032639;

input clock;
input reset;
input enable;
input [`BIT_WIDTH - 1:0] in_x;
output [`BIT_WIDTH - 1:0] log_x;


wire [`BIT_WIDTH - 1:0] mantissa;

reg [`BIT_WIDTH - 1:0] c_mantissa_val;

// deleted unsigned in these
reg [`BIT_WIDTH - 1:0] c_log_x;
reg [`LOG2_BIT_WIDTH - 1:0] c_indexFirstOne;
reg [`BIT_WIDTH - 1:0] c_temp_shift_x;
reg [`MANTISSA_PRECISION - 1:0] c_shifted_x;

reg [`LOG2_BIT_WIDTH - 1:0] r_indexFirstOne;
reg [`BIT_WIDTH - 1:0] log_x;

//Log_mantissa u1(c_shifted_x, clock, mantissa);
wire [31:0]blank;
assign blank = 32'b000000000000000000000000000000;
single_port_ram sram_replace0 (.clk (clock), .addr (c_shifted_x), .data (blank), .we (1'b0), .out (mantissa));

// priority encoder
//integer i;
//always @(*)
//begin
//	c_indexFirstOne = 6'b0; 
//	for(i = 0; i < `BIT_WIDTH; i = i + 1)
//	begin
//		if(in_x[i])
//			c_indexFirstOne = i;
//	end
//end

// Priority encoder, loop expanded
always @(in_x)
begin
	if (in_x[31]) begin
	c_indexFirstOne = 6'b011111;
	end
	else if (in_x[30]) begin
	c_indexFirstOne = 6'b011110;
	end
	else if (in_x[29]) begin
	c_indexFirstOne = 6'b011101;
	end
	else if (in_x[28]) begin
	c_indexFirstOne = 6'b011100;
	end
	else if (in_x[27]) begin
	c_indexFirstOne = 6'b011011;
	end
	else if (in_x[26]) begin
	c_indexFirstOne = 6'b011010;
	end
	else if (in_x[25]) begin
	c_indexFirstOne = 6'b011001;
	end
	else if (in_x[24]) begin
	c_indexFirstOne = 6'b011000;
	end
	else if (in_x[23]) begin
	c_indexFirstOne = 6'b010111;
	end
	else if (in_x[22]) begin
	c_indexFirstOne = 6'b010110;
	end
	else if (in_x[21]) begin
	c_indexFirstOne = 6'b010101;
	end
	else if (in_x[20]) begin
	c_indexFirstOne = 6'b010100;
	end
	else if (in_x[19]) begin
	c_indexFirstOne = 6'b010011;
	end
	else if (in_x[18]) begin
	c_indexFirstOne = 6'b010010;
	end
	else if (in_x[17]) begin
	c_indexFirstOne = 6'b010001;
	end
	else if (in_x[16]) begin
	c_indexFirstOne = 6'b010000;
	end
	else if (in_x[15]) begin
	c_indexFirstOne = 6'b001111;
	end
	else if (in_x[14]) begin
	c_indexFirstOne = 6'b001110;
	end
	else if (in_x[13]) begin
	c_indexFirstOne = 6'b001101;
	end
	else if (in_x[12]) begin
	c_indexFirstOne = 6'b001100;
	end
	else if (in_x[11]) begin
	c_indexFirstOne = 6'b001011;
	end
	else if (in_x[10]) begin
	c_indexFirstOne = 6'b001010;
	end
	else if (in_x[9]) begin
	c_indexFirstOne = 6'b001001;
	end
	else if (in_x[8]) begin
	c_indexFirstOne = 6'b001000;
	end
	else if (in_x[7]) begin
	c_indexFirstOne = 6'b000111;
	end
	else if (in_x[6]) begin
	c_indexFirstOne = 6'b000110;
	end
	else if (in_x[5]) begin
	c_indexFirstOne = 6'b000101;
	end
	else if (in_x[4]) begin
	c_indexFirstOne = 6'b000100;
	end
	else if (in_x[3]) begin
	c_indexFirstOne = 6'b000011;
	end
	else if (in_x[2]) begin
	c_indexFirstOne = 6'b000010;
	end
	else if (in_x[1]) begin
	c_indexFirstOne = 6'b000001;
	end
	else if (in_x[0]) begin
	c_indexFirstOne = 6'b000000;
	end
	else begin
	c_indexFirstOne = 6'b000000;
	end
end
	
// shift operation based on priority encoder results

//Need constant shift
wire [5:0]shifted;
assign shifted = c_indexFirstOne - `MANTISSA_PRECISION + 1;

always@(c_indexFirstOne or in_x or shifted)
begin
//	c_temp_shift_x = in_x >> (c_indexFirstOne - `MANTISSA_PRECISION + 1);
	if(c_indexFirstOne >= `MANTISSA_PRECISION)
	begin
		if(shifted == 22) begin
			c_temp_shift_x = in_x >> 22;
		end
		else if(shifted == 21) begin
			c_temp_shift_x = in_x >> 21;
		end
		else if(shifted == 20) begin
			c_temp_shift_x = in_x >> 20;
		end
		else if(shifted == 19) begin
			c_temp_shift_x = in_x >> 19;
		end
		else if(shifted == 18) begin
			c_temp_shift_x = in_x >> 18;
		end
		else if(shifted == 17) begin
			c_temp_shift_x = in_x >> 17;
		end
		else if(shifted == 16) begin
			c_temp_shift_x = in_x >> 16;
		end
		else if(shifted == 15) begin
			c_temp_shift_x = in_x >> 15;
		end
		else if(shifted == 14) begin
			c_temp_shift_x = in_x >> 14;
		end
		else if(shifted == 13) begin
			c_temp_shift_x = in_x >> 13;
		end
		else if(shifted == 12) begin
			c_temp_shift_x = in_x >> 12;
		end
		else if(shifted == 11) begin
			c_temp_shift_x = in_x >> 11;
		end
		else if(shifted == 10) begin
			c_temp_shift_x = in_x >> 10;
		end
		else if(shifted == 9) begin
			c_temp_shift_x = in_x >> 9;
		end
		else if(shifted == 8) begin
			c_temp_shift_x = in_x >> 8;
		end
		else if(shifted == 7) begin
			c_temp_shift_x = in_x >> 7;
		end
		else if(shifted == 6) begin
			c_temp_shift_x = in_x >> 6;
		end
		else if(shifted == 5) begin
			c_temp_shift_x = in_x >> 5;
		end
		else if(shifted == 4) begin
			c_temp_shift_x = in_x >> 4;
		end
		else if(shifted == 3) begin
			c_temp_shift_x = in_x >> 3;
		end
		else if(shifted == 2) begin
			c_temp_shift_x = in_x >> 2;
		end
		else if(shifted == 1) begin
			c_temp_shift_x = in_x >> 1;
		end
		else begin
			c_temp_shift_x = in_x >> 0;
		end
		//Store needed bits of shifted value
		c_shifted_x = c_temp_shift_x[`MANTISSA_PRECISION - 1:0];
	end
	else begin
		c_shifted_x = in_x[`MANTISSA_PRECISION - 1:0];
		c_temp_shift_x = 32'b0;
	end
end

// calculate log
always@(r_indexFirstOne or mantissa)
begin
	if(r_indexFirstOne >= `MANTISSA_PRECISION)
	begin
		c_log_x =  mantissa - ((`MANTISSA_PRECISION - 1) * `LOG2) + (r_indexFirstOne * `LOG2);
	end
	else
	begin
		c_log_x = mantissa;
	end
end

// latch values
always @(posedge clock)
begin
	if(reset)
		begin
			log_x <= 0;
			r_indexFirstOne <= 0;
		end
	else
		begin
			if(enable)
			begin
				r_indexFirstOne <= c_indexFirstOne;
				log_x <= c_log_x;
			end
		end
end 

endmodule



module DropSpinWrapper (
	clock, reset, enable,

   //From Hopper Module
   i_x,
	i_y,
	i_z,
	i_ux,
	i_uy,
	i_uz,
	i_sz,
	i_sr,
	i_sleftz,
	i_sleftr,
	i_weight,
	i_layer,
	i_dead,
	i_hit,
	
	
	//From System Register File (5 layers)- Absorber
	muaFraction1, muaFraction2, muaFraction3, muaFraction4, muaFraction5, 
 
 	//From System Register File - ScattererReflector 
	down_niOverNt_1,
	down_niOverNt_2,
	down_niOverNt_3,
	down_niOverNt_4,
	down_niOverNt_5,
	up_niOverNt_1,
	up_niOverNt_2,
	up_niOverNt_3,
	up_niOverNt_4,
	up_niOverNt_5,
	down_niOverNt_2_1,
	down_niOverNt_2_2,
	down_niOverNt_2_3,
	down_niOverNt_2_4,
	down_niOverNt_2_5,
	up_niOverNt_2_1,
	up_niOverNt_2_2,
	up_niOverNt_2_3,
	up_niOverNt_2_4,
	up_niOverNt_2_5,
	downCritAngle_0,
	downCritAngle_1,
	downCritAngle_2,
	downCritAngle_3,
	downCritAngle_4,
	upCritAngle_0,
	upCritAngle_1,
	upCritAngle_2,
	upCritAngle_3,
	upCritAngle_4,
 
	
 	
 	//////////////////////////////////////////////////////////////////////////////
   //I/O to on-chip mem
   /////////////////////////////////////////////////////////////////////////////

   data, 
   rdaddress, wraddress,
   wren, q,

   //From Memories
   up_rFresnel,
   down_rFresnel,
   sint,
   cost,
   rand2,
   rand3,
   rand5,
   //To Memories
   tindex,
   fresIndex,

 	
   //To DeadOrAlive Module
	o_x,
	o_y,
	o_z,
	o_ux,
	o_uy,
	o_uz,
	o_sz,
	o_sr,
	o_sleftz,
	o_sleftr,
	o_weight,
	o_layer,
	o_dead,
	o_hit
                    
	);
	
//////////////////////////////////////////////////////////////////////////////
//PARAMETERS
//////////////////////////////////////////////////////////////////////////////	
//parameter BIT_WIDTH=32;
//parameter LAYER_WIDTH=3;
//parameter PIPE_DEPTH = 37;
//parameter ADDR_WIDTH=16;          //TODO: TBD
//parameter WORD_WIDTH=64;

//////////////////////////////////////////////////////////////////////////////
//INPUTS
//////////////////////////////////////////////////////////////////////////////
input clock, reset, enable;

//From Hopper Module
input	[`BIT_WIDTH-1:0]			i_x;
input	[`BIT_WIDTH-1:0]			i_y;
input	[`BIT_WIDTH-1:0]			i_z;
input	[`BIT_WIDTH-1:0]			i_ux;
input	[`BIT_WIDTH-1:0]			i_uy;
input	[`BIT_WIDTH-1:0]			i_uz;
input	[`BIT_WIDTH-1:0]			i_sz;
input	[`BIT_WIDTH-1:0]			i_sr;
input	[`BIT_WIDTH-1:0]			i_sleftz;
input	[`BIT_WIDTH-1:0]			i_sleftr;
input	[`BIT_WIDTH-1:0]			i_weight;
input	[`LAYER_WIDTH-1:0]		i_layer;
input					i_dead;
input					i_hit;


//From System Register File (5 layers)- Absorber
input	[`BIT_WIDTH-1:0] muaFraction1, muaFraction2, muaFraction3, muaFraction4, muaFraction5; 

//From System Register File - ScattererReflector 
input	[`BIT_WIDTH-1:0]	down_niOverNt_1;
input	[`BIT_WIDTH-1:0]	down_niOverNt_2;
input	[`BIT_WIDTH-1:0]	down_niOverNt_3;
input	[`BIT_WIDTH-1:0]	down_niOverNt_4;
input	[`BIT_WIDTH-1:0]	down_niOverNt_5;
input	[`BIT_WIDTH-1:0]	up_niOverNt_1;
input	[`BIT_WIDTH-1:0]	up_niOverNt_2;
input	[`BIT_WIDTH-1:0]	up_niOverNt_3;
input	[`BIT_WIDTH-1:0]	up_niOverNt_4;
input	[`BIT_WIDTH-1:0]	up_niOverNt_5;
input	[`WORD_WIDTH-1:0]	down_niOverNt_2_1;
input	[`WORD_WIDTH-1:0]	down_niOverNt_2_2;
input	[`WORD_WIDTH-1:0]	down_niOverNt_2_3;
input	[`WORD_WIDTH-1:0]	down_niOverNt_2_4;
input	[`WORD_WIDTH-1:0]	down_niOverNt_2_5;
input	[`WORD_WIDTH-1:0]	up_niOverNt_2_1;
input	[`WORD_WIDTH-1:0]	up_niOverNt_2_2;
input	[`WORD_WIDTH-1:0]	up_niOverNt_2_3;
input	[`WORD_WIDTH-1:0]	up_niOverNt_2_4;
input	[`WORD_WIDTH-1:0]	up_niOverNt_2_5;
input	[`BIT_WIDTH-1:0]	downCritAngle_0;
input	[`BIT_WIDTH-1:0]	downCritAngle_1;
input	[`BIT_WIDTH-1:0]	downCritAngle_2;
input	[`BIT_WIDTH-1:0]	downCritAngle_3;
input	[`BIT_WIDTH-1:0]	downCritAngle_4;
input	[`BIT_WIDTH-1:0]	upCritAngle_0;
input	[`BIT_WIDTH-1:0]	upCritAngle_1;
input	[`BIT_WIDTH-1:0]	upCritAngle_2;
input	[`BIT_WIDTH-1:0]	upCritAngle_3;
input	[`BIT_WIDTH-1:0]	upCritAngle_4;

//Generated by random number generators controlled by skeleton
output	[12:0]		tindex;
output	[9:0]		fresIndex;


input	[31:0]		rand2;
input	[31:0]		rand3;
input	[31:0]		rand5;
input	[31:0]		sint;
input	[31:0]		cost;
input	[31:0]		up_rFresnel;
input	[31:0]		down_rFresnel;

 

//////////////////////////////////////////////////////////////////////////////
//OUTPUTS
/////////////////////////////////////////////////////////////////////////////
//To DeadOrAlive Module
output	[`BIT_WIDTH-1:0]			o_x;
output	[`BIT_WIDTH-1:0]			o_y;
output	[`BIT_WIDTH-1:0]			o_z;
output	[`BIT_WIDTH-1:0]			o_ux;
output	[`BIT_WIDTH-1:0]			o_uy;
output	[`BIT_WIDTH-1:0]			o_uz;
output	[`BIT_WIDTH-1:0]			o_sz;
output	[`BIT_WIDTH-1:0]			o_sr;
output	[`BIT_WIDTH-1:0]			o_sleftz;
output	[`BIT_WIDTH-1:0]			o_sleftr;
output	[`BIT_WIDTH-1:0]			o_weight;
output	[`LAYER_WIDTH-1:0]		o_layer;
output					o_dead;
output					o_hit;

wire	[`BIT_WIDTH-1:0]			o_x;
wire	[`BIT_WIDTH-1:0]			o_y;
wire	[`BIT_WIDTH-1:0]			o_z;
reg	[`BIT_WIDTH-1:0]			o_ux;
reg	[`BIT_WIDTH-1:0]			o_uy;
reg	[`BIT_WIDTH-1:0]			o_uz;
wire	[`BIT_WIDTH-1:0]			o_sz;
wire	[`BIT_WIDTH-1:0]			o_sr;
wire	[`BIT_WIDTH-1:0]			o_sleftz;
wire	[`BIT_WIDTH-1:0]			o_sleftr;
wire	[`BIT_WIDTH-1:0]			o_weight;
reg	[`LAYER_WIDTH-1:0]		o_layer;
reg					o_dead;
wire					o_hit;


//////////////////////////////////////////////////////////////////////////////
//I/O to on-chip mem
/////////////////////////////////////////////////////////////////////////////

output [`WORD_WIDTH-1:0] data; 
output [`ADDR_WIDTH-1:0] rdaddress, wraddress; 
output wren; 
input [`WORD_WIDTH-1:0] q;


//////////////////////////////////////////////////////////////////////////////
//Generate SHARED REGISTER PIPELINE 
//////////////////////////////////////////////////////////////////////////////
//WIRES FOR CONNECTING REGISTERS
//wire	[`BIT_WIDTH-1:0]			x	[PIPE_DEPTH:0];
//wire	[`BIT_WIDTH-1:0]			y	[PIPE_DEPTH:0];
//wire	[`BIT_WIDTH-1:0]			z	[PIPE_DEPTH:0];
//wire	[`BIT_WIDTH-1:0]			ux	[PIPE_DEPTH:0];
//wire	[`BIT_WIDTH-1:0]			uy	[PIPE_DEPTH:0];
//wire	[`BIT_WIDTH-1:0]			uz	[PIPE_DEPTH:0];
//wire	[`BIT_WIDTH-1:0]			sz	[PIPE_DEPTH:0];
//wire	[`BIT_WIDTH-1:0]			sr	[PIPE_DEPTH:0];
//wire	[`BIT_WIDTH-1:0]			sleftz	[PIPE_DEPTH:0];
//wire	[`BIT_WIDTH-1:0]			sleftr	[PIPE_DEPTH:0];
//wire	[`BIT_WIDTH-1:0]			weight	[PIPE_DEPTH:0];
//wire	[LAYER_WIDTH-1:0]		layer	[PIPE_DEPTH:0];
//wire					dead	[PIPE_DEPTH:0];
//wire					hit	[PIPE_DEPTH:0];

//WIRES FOR CONNECTING REGISTERS
//wire	[32-1:0]			x	[37:0];
wire	[32-1:0]				x__0;
wire	[32-1:0]				x__1;
wire	[32-1:0]				x__2;
wire	[32-1:0]				x__3;
wire	[32-1:0]				x__4;
wire	[32-1:0]				x__5;
wire	[32-1:0]				x__6;
wire	[32-1:0]				x__7;
wire	[32-1:0]				x__8;
wire	[32-1:0]				x__9;
wire	[32-1:0]				x__10;
wire	[32-1:0]				x__11;
wire	[32-1:0]				x__12;
wire	[32-1:0]				x__13;
wire	[32-1:0]				x__14;
wire	[32-1:0]				x__15;
wire	[32-1:0]				x__16;
wire	[32-1:0]				x__17;
wire	[32-1:0]				x__18;
wire	[32-1:0]				x__19;
wire	[32-1:0]				x__20;
wire	[32-1:0]				x__21;
wire	[32-1:0]				x__22;
wire	[32-1:0]				x__23;
wire	[32-1:0]				x__24;
wire	[32-1:0]				x__25;
wire	[32-1:0]				x__26;
wire	[32-1:0]				x__27;
wire	[32-1:0]				x__28;
wire	[32-1:0]				x__29;
wire	[32-1:0]				x__30;
wire	[32-1:0]				x__31;
wire	[32-1:0]				x__32;
wire	[32-1:0]				x__33;
wire	[32-1:0]				x__34;
wire	[32-1:0]				x__35;
wire	[32-1:0]				x__36;
wire	[32-1:0]				x__37;




//wire	[32-1:0]			y	[37:0];
wire	[32-1:0]				y__0;
wire	[32-1:0]				y__1;
wire	[32-1:0]				y__2;
wire	[32-1:0]				y__3;
wire	[32-1:0]				y__4;
wire	[32-1:0]				y__5;
wire	[32-1:0]				y__6;
wire	[32-1:0]				y__7;
wire	[32-1:0]				y__8;
wire	[32-1:0]				y__9;
wire	[32-1:0]				y__10;
wire	[32-1:0]				y__11;
wire	[32-1:0]				y__12;
wire	[32-1:0]				y__13;
wire	[32-1:0]				y__14;
wire	[32-1:0]				y__15;
wire	[32-1:0]				y__16;
wire	[32-1:0]				y__17;
wire	[32-1:0]				y__18;
wire	[32-1:0]				y__19;
wire	[32-1:0]				y__20;
wire	[32-1:0]				y__21;
wire	[32-1:0]				y__22;
wire	[32-1:0]				y__23;
wire	[32-1:0]				y__24;
wire	[32-1:0]				y__25;
wire	[32-1:0]				y__26;
wire	[32-1:0]				y__27;
wire	[32-1:0]				y__28;
wire	[32-1:0]				y__29;
wire	[32-1:0]				y__30;
wire	[32-1:0]				y__31;
wire	[32-1:0]				y__32;
wire	[32-1:0]				y__33;
wire	[32-1:0]				y__34;
wire	[32-1:0]				y__35;
wire	[32-1:0]				y__36;
wire	[32-1:0]				y__37;



//wire	[32-1:0]			z	[37:0];
wire	[32-1:0]				z__0;
wire	[32-1:0]				z__1;
wire	[32-1:0]				z__2;
wire	[32-1:0]				z__3;
wire	[32-1:0]				z__4;
wire	[32-1:0]				z__5;
wire	[32-1:0]				z__6;
wire	[32-1:0]				z__7;
wire	[32-1:0]				z__8;
wire	[32-1:0]				z__9;
wire	[32-1:0]				z__10;
wire	[32-1:0]				z__11;
wire	[32-1:0]				z__12;
wire	[32-1:0]				z__13;
wire	[32-1:0]				z__14;
wire	[32-1:0]				z__15;
wire	[32-1:0]				z__16;
wire	[32-1:0]				z__17;
wire	[32-1:0]				z__18;
wire	[32-1:0]				z__19;
wire	[32-1:0]				z__20;
wire	[32-1:0]				z__21;
wire	[32-1:0]				z__22;
wire	[32-1:0]				z__23;
wire	[32-1:0]				z__24;
wire	[32-1:0]				z__25;
wire	[32-1:0]				z__26;
wire	[32-1:0]				z__27;
wire	[32-1:0]				z__28;
wire	[32-1:0]				z__29;
wire	[32-1:0]				z__30;
wire	[32-1:0]				z__31;
wire	[32-1:0]				z__32;
wire	[32-1:0]				z__33;
wire	[32-1:0]				z__34;
wire	[32-1:0]				z__35;
wire	[32-1:0]				z__36;
wire	[32-1:0]				z__37;


//wire	[32-1:0]			ux	[37:0];
wire	[32-1:0]				ux__0;
wire	[32-1:0]				ux__1;
wire	[32-1:0]				ux__2;
wire	[32-1:0]				ux__3;
wire	[32-1:0]				ux__4;
wire	[32-1:0]				ux__5;
wire	[32-1:0]				ux__6;
wire	[32-1:0]				ux__7;
wire	[32-1:0]				ux__8;
wire	[32-1:0]				ux__9;
wire	[32-1:0]				ux__10;
wire	[32-1:0]				ux__11;
wire	[32-1:0]				ux__12;
wire	[32-1:0]				ux__13;
wire	[32-1:0]				ux__14;
wire	[32-1:0]				ux__15;
wire	[32-1:0]				ux__16;
wire	[32-1:0]				ux__17;
wire	[32-1:0]				ux__18;
wire	[32-1:0]				ux__19;
wire	[32-1:0]				ux__20;
wire	[32-1:0]				ux__21;
wire	[32-1:0]				ux__22;
wire	[32-1:0]				ux__23;
wire	[32-1:0]				ux__24;
wire	[32-1:0]				ux__25;
wire	[32-1:0]				ux__26;
wire	[32-1:0]				ux__27;
wire	[32-1:0]				ux__28;
wire	[32-1:0]				ux__29;
wire	[32-1:0]				ux__30;
wire	[32-1:0]				ux__31;
wire	[32-1:0]				ux__32;
wire	[32-1:0]				ux__33;
wire	[32-1:0]				ux__34;
wire	[32-1:0]				ux__35;
wire	[32-1:0]				ux__36;
wire	[32-1:0]				ux__37;



//wire	[32-1:0]			uy	[37:0];
wire	[32-1:0]				uy__0;
wire	[32-1:0]				uy__1;
wire	[32-1:0]				uy__2;
wire	[32-1:0]				uy__3;
wire	[32-1:0]				uy__4;
wire	[32-1:0]				uy__5;
wire	[32-1:0]				uy__6;
wire	[32-1:0]				uy__7;
wire	[32-1:0]				uy__8;
wire	[32-1:0]				uy__9;
wire	[32-1:0]				uy__10;
wire	[32-1:0]				uy__11;
wire	[32-1:0]				uy__12;
wire	[32-1:0]				uy__13;
wire	[32-1:0]				uy__14;
wire	[32-1:0]				uy__15;
wire	[32-1:0]				uy__16;
wire	[32-1:0]				uy__17;
wire	[32-1:0]				uy__18;
wire	[32-1:0]				uy__19;
wire	[32-1:0]				uy__20;
wire	[32-1:0]				uy__21;
wire	[32-1:0]				uy__22;
wire	[32-1:0]				uy__23;
wire	[32-1:0]				uy__24;
wire	[32-1:0]				uy__25;
wire	[32-1:0]				uy__26;
wire	[32-1:0]				uy__27;
wire	[32-1:0]				uy__28;
wire	[32-1:0]				uy__29;
wire	[32-1:0]				uy__30;
wire	[32-1:0]				uy__31;
wire	[32-1:0]				uy__32;
wire	[32-1:0]				uy__33;
wire	[32-1:0]				uy__34;
wire	[32-1:0]				uy__35;
wire	[32-1:0]				uy__36;
wire	[32-1:0]				uy__37;


//wire	[32-1:0]			uz	[37:0];
wire	[32-1:0]				uz__0;
wire	[32-1:0]				uz__1;
wire	[32-1:0]				uz__2;
wire	[32-1:0]				uz__3;
wire	[32-1:0]				uz__4;
wire	[32-1:0]				uz__5;
wire	[32-1:0]				uz__6;
wire	[32-1:0]				uz__7;
wire	[32-1:0]				uz__8;
wire	[32-1:0]				uz__9;
wire	[32-1:0]				uz__10;
wire	[32-1:0]				uz__11;
wire	[32-1:0]				uz__12;
wire	[32-1:0]				uz__13;
wire	[32-1:0]				uz__14;
wire	[32-1:0]				uz__15;
wire	[32-1:0]				uz__16;
wire	[32-1:0]				uz__17;
wire	[32-1:0]				uz__18;
wire	[32-1:0]				uz__19;
wire	[32-1:0]				uz__20;
wire	[32-1:0]				uz__21;
wire	[32-1:0]				uz__22;
wire	[32-1:0]				uz__23;
wire	[32-1:0]				uz__24;
wire	[32-1:0]				uz__25;
wire	[32-1:0]				uz__26;
wire	[32-1:0]				uz__27;
wire	[32-1:0]				uz__28;
wire	[32-1:0]				uz__29;
wire	[32-1:0]				uz__30;
wire	[32-1:0]				uz__31;
wire	[32-1:0]				uz__32;
wire	[32-1:0]				uz__33;
wire	[32-1:0]				uz__34;
wire	[32-1:0]				uz__35;
wire	[32-1:0]				uz__36;
wire	[32-1:0]				uz__37;


//wire	[32-1:0]			sz	[37:0];
wire	[32-1:0]				sz__0;
wire	[32-1:0]				sz__1;
wire	[32-1:0]				sz__2;
wire	[32-1:0]				sz__3;
wire	[32-1:0]				sz__4;
wire	[32-1:0]				sz__5;
wire	[32-1:0]				sz__6;
wire	[32-1:0]				sz__7;
wire	[32-1:0]				sz__8;
wire	[32-1:0]				sz__9;
wire	[32-1:0]				sz__10;
wire	[32-1:0]				sz__11;
wire	[32-1:0]				sz__12;
wire	[32-1:0]				sz__13;
wire	[32-1:0]				sz__14;
wire	[32-1:0]				sz__15;
wire	[32-1:0]				sz__16;
wire	[32-1:0]				sz__17;
wire	[32-1:0]				sz__18;
wire	[32-1:0]				sz__19;
wire	[32-1:0]				sz__20;
wire	[32-1:0]				sz__21;
wire	[32-1:0]				sz__22;
wire	[32-1:0]				sz__23;
wire	[32-1:0]				sz__24;
wire	[32-1:0]				sz__25;
wire	[32-1:0]				sz__26;
wire	[32-1:0]				sz__27;
wire	[32-1:0]				sz__28;
wire	[32-1:0]				sz__29;
wire	[32-1:0]				sz__30;
wire	[32-1:0]				sz__31;
wire	[32-1:0]				sz__32;
wire	[32-1:0]				sz__33;
wire	[32-1:0]				sz__34;
wire	[32-1:0]				sz__35;
wire	[32-1:0]				sz__36;
wire	[32-1:0]				sz__37;


//wire	[32-1:0]			sr	[37:0];
wire	[32-1:0]				sr__0;
wire	[32-1:0]				sr__1;
wire	[32-1:0]				sr__2;
wire	[32-1:0]				sr__3;
wire	[32-1:0]				sr__4;
wire	[32-1:0]				sr__5;
wire	[32-1:0]				sr__6;
wire	[32-1:0]				sr__7;
wire	[32-1:0]				sr__8;
wire	[32-1:0]				sr__9;
wire	[32-1:0]				sr__10;
wire	[32-1:0]				sr__11;
wire	[32-1:0]				sr__12;
wire	[32-1:0]				sr__13;
wire	[32-1:0]				sr__14;
wire	[32-1:0]				sr__15;
wire	[32-1:0]				sr__16;
wire	[32-1:0]				sr__17;
wire	[32-1:0]				sr__18;
wire	[32-1:0]				sr__19;
wire	[32-1:0]				sr__20;
wire	[32-1:0]				sr__21;
wire	[32-1:0]				sr__22;
wire	[32-1:0]				sr__23;
wire	[32-1:0]				sr__24;
wire	[32-1:0]				sr__25;
wire	[32-1:0]				sr__26;
wire	[32-1:0]				sr__27;
wire	[32-1:0]				sr__28;
wire	[32-1:0]				sr__29;
wire	[32-1:0]				sr__30;
wire	[32-1:0]				sr__31;
wire	[32-1:0]				sr__32;
wire	[32-1:0]				sr__33;
wire	[32-1:0]				sr__34;
wire	[32-1:0]				sr__35;
wire	[32-1:0]				sr__36;
wire	[32-1:0]				sr__37;



//wire	[32-1:0]			sleftz	[37:0];
wire	[32-1:0]				sleftz__0;
wire	[32-1:0]				sleftz__1;
wire	[32-1:0]				sleftz__2;
wire	[32-1:0]				sleftz__3;
wire	[32-1:0]				sleftz__4;
wire	[32-1:0]				sleftz__5;
wire	[32-1:0]				sleftz__6;
wire	[32-1:0]				sleftz__7;
wire	[32-1:0]				sleftz__8;
wire	[32-1:0]				sleftz__9;
wire	[32-1:0]				sleftz__10;
wire	[32-1:0]				sleftz__11;
wire	[32-1:0]				sleftz__12;
wire	[32-1:0]				sleftz__13;
wire	[32-1:0]				sleftz__14;
wire	[32-1:0]				sleftz__15;
wire	[32-1:0]				sleftz__16;
wire	[32-1:0]				sleftz__17;
wire	[32-1:0]				sleftz__18;
wire	[32-1:0]				sleftz__19;
wire	[32-1:0]				sleftz__20;
wire	[32-1:0]				sleftz__21;
wire	[32-1:0]				sleftz__22;
wire	[32-1:0]				sleftz__23;
wire	[32-1:0]				sleftz__24;
wire	[32-1:0]				sleftz__25;
wire	[32-1:0]				sleftz__26;
wire	[32-1:0]				sleftz__27;
wire	[32-1:0]				sleftz__28;
wire	[32-1:0]				sleftz__29;
wire	[32-1:0]				sleftz__30;
wire	[32-1:0]				sleftz__31;
wire	[32-1:0]				sleftz__32;
wire	[32-1:0]				sleftz__33;
wire	[32-1:0]				sleftz__34;
wire	[32-1:0]				sleftz__35;
wire	[32-1:0]				sleftz__36;
wire	[32-1:0]				sleftz__37;


//wire	[32-1:0]			sleftr	[37:0];
wire	[32-1:0]				sleftr__0;
wire	[32-1:0]				sleftr__1;
wire	[32-1:0]				sleftr__2;
wire	[32-1:0]				sleftr__3;
wire	[32-1:0]				sleftr__4;
wire	[32-1:0]				sleftr__5;
wire	[32-1:0]				sleftr__6;
wire	[32-1:0]				sleftr__7;
wire	[32-1:0]				sleftr__8;
wire	[32-1:0]				sleftr__9;
wire	[32-1:0]				sleftr__10;
wire	[32-1:0]				sleftr__11;
wire	[32-1:0]				sleftr__12;
wire	[32-1:0]				sleftr__13;
wire	[32-1:0]				sleftr__14;
wire	[32-1:0]				sleftr__15;
wire	[32-1:0]				sleftr__16;
wire	[32-1:0]				sleftr__17;
wire	[32-1:0]				sleftr__18;
wire	[32-1:0]				sleftr__19;
wire	[32-1:0]				sleftr__20;
wire	[32-1:0]				sleftr__21;
wire	[32-1:0]				sleftr__22;
wire	[32-1:0]				sleftr__23;
wire	[32-1:0]				sleftr__24;
wire	[32-1:0]				sleftr__25;
wire	[32-1:0]				sleftr__26;
wire	[32-1:0]				sleftr__27;
wire	[32-1:0]				sleftr__28;
wire	[32-1:0]				sleftr__29;
wire	[32-1:0]				sleftr__30;
wire	[32-1:0]				sleftr__31;
wire	[32-1:0]				sleftr__32;
wire	[32-1:0]				sleftr__33;
wire	[32-1:0]				sleftr__34;
wire	[32-1:0]				sleftr__35;
wire	[32-1:0]				sleftr__36;
wire	[32-1:0]				sleftr__37;


//wire	[32-1:0]			weight	[37:0];
wire	[32-1:0]				weight__0;
wire	[32-1:0]				weight__1;
wire	[32-1:0]				weight__2;
wire	[32-1:0]				weight__3;
wire	[32-1:0]				weight__4;
wire	[32-1:0]				weight__5;
wire	[32-1:0]				weight__6;
wire	[32-1:0]				weight__7;
wire	[32-1:0]				weight__8;
wire	[32-1:0]				weight__9;
wire	[32-1:0]				weight__10;
wire	[32-1:0]				weight__11;
wire	[32-1:0]				weight__12;
wire	[32-1:0]				weight__13;
wire	[32-1:0]				weight__14;
wire	[32-1:0]				weight__15;
wire	[32-1:0]				weight__16;
wire	[32-1:0]				weight__17;
wire	[32-1:0]				weight__18;
wire	[32-1:0]				weight__19;
wire	[32-1:0]				weight__20;
wire	[32-1:0]				weight__21;
wire	[32-1:0]				weight__22;
wire	[32-1:0]				weight__23;
wire	[32-1:0]				weight__24;
wire	[32-1:0]				weight__25;
wire	[32-1:0]				weight__26;
wire	[32-1:0]				weight__27;
wire	[32-1:0]				weight__28;
wire	[32-1:0]				weight__29;
wire	[32-1:0]				weight__30;
wire	[32-1:0]				weight__31;
wire	[32-1:0]				weight__32;
wire	[32-1:0]				weight__33;
wire	[32-1:0]				weight__34;
wire	[32-1:0]				weight__35;
wire	[32-1:0]				weight__36;
wire	[32-1:0]				weight__37;


//wire	[3-1:0]		layer	[37:0];
wire	[3-1:0]				layer__0;
wire	[3-1:0]				layer__1;
wire	[3-1:0]				layer__2;
wire	[3-1:0]				layer__3;
wire	[3-1:0]				layer__4;
wire	[3-1:0]				layer__5;
wire	[3-1:0]				layer__6;
wire	[3-1:0]				layer__7;
wire	[3-1:0]				layer__8;
wire	[3-1:0]				layer__9;
wire	[3-1:0]				layer__10;
wire	[3-1:0]				layer__11;
wire	[3-1:0]				layer__12;
wire	[3-1:0]				layer__13;
wire	[3-1:0]				layer__14;
wire	[3-1:0]				layer__15;
wire	[3-1:0]				layer__16;
wire	[3-1:0]				layer__17;
wire	[3-1:0]				layer__18;
wire	[3-1:0]				layer__19;
wire	[3-1:0]				layer__20;
wire	[3-1:0]				layer__21;
wire	[3-1:0]				layer__22;
wire	[3-1:0]				layer__23;
wire	[3-1:0]				layer__24;
wire	[3-1:0]				layer__25;
wire	[3-1:0]				layer__26;
wire	[3-1:0]				layer__27;
wire	[3-1:0]				layer__28;
wire	[3-1:0]				layer__29;
wire	[3-1:0]				layer__30;
wire	[3-1:0]				layer__31;
wire	[3-1:0]				layer__32;
wire	[3-1:0]				layer__33;
wire	[3-1:0]				layer__34;
wire	[3-1:0]				layer__35;
wire	[3-1:0]				layer__36;
wire	[3-1:0]				layer__37;

//wire		[37:0]			dead;
wire					dead__0;
wire					dead__1;
wire					dead__2;
wire					dead__3;
wire					dead__4;
wire					dead__5;
wire					dead__6;
wire					dead__7;
wire					dead__8;
wire					dead__9;
wire					dead__10;
wire					dead__11;
wire					dead__12;
wire					dead__13;
wire					dead__14;
wire					dead__15;
wire					dead__16;
wire					dead__17;
wire					dead__18;
wire					dead__19;
wire					dead__20;
wire					dead__21;
wire					dead__22;
wire					dead__23;
wire					dead__24;
wire					dead__25;
wire					dead__26;
wire					dead__27;
wire					dead__28;
wire					dead__29;
wire					dead__30;
wire					dead__31;
wire					dead__32;
wire					dead__33;
wire					dead__34;
wire					dead__35;
wire					dead__36;
wire					dead__37;


//wire		[37:0]			hit	;

wire					hit__0;
wire					hit__1;
wire					hit__2;
wire					hit__3;
wire					hit__4;
wire					hit__5;
wire					hit__6;
wire					hit__7;
wire					hit__8;
wire					hit__9;
wire					hit__10;
wire					hit__11;
wire					hit__12;
wire					hit__13;
wire					hit__14;
wire					hit__15;
wire					hit__16;
wire					hit__17;
wire					hit__18;
wire					hit__19;
wire					hit__20;
wire					hit__21;
wire					hit__22;
wire					hit__23;
wire					hit__24;
wire					hit__25;
wire					hit__26;
wire					hit__27;
wire					hit__28;
wire					hit__29;
wire					hit__30;
wire					hit__31;
wire					hit__32;
wire					hit__33;
wire					hit__34;
wire					hit__35;
wire					hit__36;
wire					hit__37;


//ASSIGNMENTS FROM INPUTS TO PIPE
assign x__0 = i_x;
assign y__0 = i_y;
assign z__0 = i_z;
assign ux__0 = i_ux;
assign uy__0 = i_uy;
assign uz__0 = i_uz;
assign sz__0 = i_sz;
assign sr__0 = i_sr;
assign sleftz__0 = i_sleftz;
assign sleftr__0 = i_sleftr;
assign weight__0 = i_weight;
assign layer__0 = i_layer;
assign dead__0 = i_dead;
assign hit__0 = i_hit;

//ASSIGNMENTS FROM PIPE TO OUTPUT
//TODO: Assign outputs from the correct module 
assign o_x =x__37;
assign o_y =y__37;
assign o_z =z__37;
//assign o_ux =ux[PIPE_DEPTH]; Assigned by deadOrAliveMux
//assign o_uy =uy[PIPE_DEPTH]; Assigned by deadOrAliveMux
//assign o_uz =uz[PIPE_DEPTH]; Assigned by deadOrAliveMux
assign o_sz =sz__37;
assign o_sr =sr__37;
assign o_sleftz =sleftz__37;
assign o_sleftr =sleftr__37;
//assign o_weight =weight[PIPE_DEPTH]; Assigned by absorber module (below)
//assign o_layer =layer[PIPE_DEPTH]; Assigned by deadOrAliveMux
//assign o_dead =dead[PIPE_DEPTH]; Assigned by deadOrAliveMux
assign o_hit =hit__37;


//GENERATE PIPELINE
//genvar i;
//generate
//	for(i=PIPE_DEPTH; i>0; i=i-1) begin: regPipe
//		case(i)
//		
//		default:
//		PhotonBlock5 photon(
//			//Inputs
//			.clock(clock),
//			.reset(reset),
//			.enable(enable),
//			
//			.i_x(x[i-1]),
//			.i_y(y[i-1]),
//			.i_z(z[i-1]),
//			.i_ux(ux[i-1]),
//			.i_uy(uy[i-1]),
//			.i_uz(uz[i-1]),
//			.i_sz(sz[i-1]),
//			.i_sr(sr[i-1]),
//			.i_sleftz(sleftz[i-1]),
//			.i_sleftr(sleftr[i-1]),
//			.i_weight(weight[i-1]),
//			.i_layer(layer[i-1]),
//			.i_dead(dead[i-1]),
//			.i_hit(hit[i-1]),
//			
//			//Outputs			
//			.o_x(x[i]),
//			.o_y(y[i]),
//			.o_z(z[i]),
//			.o_ux(ux[i]),
//			.o_uy(uy[i]),
//			.o_uz(uz[i]),
//			.o_sz(sz[i]),
//			.o_sr(sr[i]),
//			.o_sleftz(sleftz[i]),
//			.o_sleftr(sleftr[i]),
//			.o_weight(weight[i]),
//			.o_layer(layer[i]),
//			.o_dead(dead[i]),
//			.o_hit(hit[i])
//		);
//		endcase
//	end
//endgenerate	

PhotonBlock5 photon37(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__36),
.i_y(y__36),
.i_z(z__36),
.i_ux(ux__36),
.i_uy(uy__36),
.i_uz(uz__36),
.i_sz(sz__36),
.i_sr(sr__36),
.i_sleftz(sleftz__36),
.i_sleftr(sleftr__36),
.i_weight(weight__36),
.i_layer(layer__36),
.i_dead(dead__36),
.i_hit(hit__36),
//Outputs			
.o_x(x__37),
.o_y(y__37),
.o_z(z__37),
.o_ux(ux__37),
.o_uy(uy__37),
.o_uz(uz__37),
.o_sz(sz__37),
.o_sr(sr__37),
.o_sleftz(sleftz__37),
.o_sleftr(sleftr__37),
.o_weight(weight__37),
.o_layer(layer__37),
.o_dead(dead__37),
.o_hit(hit__37)
); 
PhotonBlock5 photon36(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__35),
.i_y(y__35),
.i_z(z__35),
.i_ux(ux__35),
.i_uy(uy__35),
.i_uz(uz__35),
.i_sz(sz__35),
.i_sr(sr__35),
.i_sleftz(sleftz__35),
.i_sleftr(sleftr__35),
.i_weight(weight__35),
.i_layer(layer__35),
.i_dead(dead__35),
.i_hit(hit__35),
//Outputs			
.o_x(x__36),
.o_y(y__36),
.o_z(z__36),
.o_ux(ux__36),
.o_uy(uy__36),
.o_uz(uz__36),
.o_sz(sz__36),
.o_sr(sr__36),
.o_sleftz(sleftz__36),
.o_sleftr(sleftr__36),
.o_weight(weight__36),
.o_layer(layer__36),
.o_dead(dead__36),
.o_hit(hit__36)
); 
PhotonBlock5 photon35(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__34),
.i_y(y__34),
.i_z(z__34),
.i_ux(ux__34),
.i_uy(uy__34),
.i_uz(uz__34),
.i_sz(sz__34),
.i_sr(sr__34),
.i_sleftz(sleftz__34),
.i_sleftr(sleftr__34),
.i_weight(weight__34),
.i_layer(layer__34),
.i_dead(dead__34),
.i_hit(hit__34),
//Outputs			
.o_x(x__35),
.o_y(y__35),
.o_z(z__35),
.o_ux(ux__35),
.o_uy(uy__35),
.o_uz(uz__35),
.o_sz(sz__35),
.o_sr(sr__35),
.o_sleftz(sleftz__35),
.o_sleftr(sleftr__35),
.o_weight(weight__35),
.o_layer(layer__35),
.o_dead(dead__35),
.o_hit(hit__35)
); 
PhotonBlock5 photon34(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__33),
.i_y(y__33),
.i_z(z__33),
.i_ux(ux__33),
.i_uy(uy__33),
.i_uz(uz__33),
.i_sz(sz__33),
.i_sr(sr__33),
.i_sleftz(sleftz__33),
.i_sleftr(sleftr__33),
.i_weight(weight__33),
.i_layer(layer__33),
.i_dead(dead__33),
.i_hit(hit__33),
//Outputs			
.o_x(x__34),
.o_y(y__34),
.o_z(z__34),
.o_ux(ux__34),
.o_uy(uy__34),
.o_uz(uz__34),
.o_sz(sz__34),
.o_sr(sr__34),
.o_sleftz(sleftz__34),
.o_sleftr(sleftr__34),
.o_weight(weight__34),
.o_layer(layer__34),
.o_dead(dead__34),
.o_hit(hit__34)
); 
PhotonBlock5 photon33(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__32),
.i_y(y__32),
.i_z(z__32),
.i_ux(ux__32),
.i_uy(uy__32),
.i_uz(uz__32),
.i_sz(sz__32),
.i_sr(sr__32),
.i_sleftz(sleftz__32),
.i_sleftr(sleftr__32),
.i_weight(weight__32),
.i_layer(layer__32),
.i_dead(dead__32),
.i_hit(hit__32),
//Outputs			
.o_x(x__33),
.o_y(y__33),
.o_z(z__33),
.o_ux(ux__33),
.o_uy(uy__33),
.o_uz(uz__33),
.o_sz(sz__33),
.o_sr(sr__33),
.o_sleftz(sleftz__33),
.o_sleftr(sleftr__33),
.o_weight(weight__33),
.o_layer(layer__33),
.o_dead(dead__33),
.o_hit(hit__33)
); 
PhotonBlock5 photon32(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__31),
.i_y(y__31),
.i_z(z__31),
.i_ux(ux__31),
.i_uy(uy__31),
.i_uz(uz__31),
.i_sz(sz__31),
.i_sr(sr__31),
.i_sleftz(sleftz__31),
.i_sleftr(sleftr__31),
.i_weight(weight__31),
.i_layer(layer__31),
.i_dead(dead__31),
.i_hit(hit__31),
//Outputs			
.o_x(x__32),
.o_y(y__32),
.o_z(z__32),
.o_ux(ux__32),
.o_uy(uy__32),
.o_uz(uz__32),
.o_sz(sz__32),
.o_sr(sr__32),
.o_sleftz(sleftz__32),
.o_sleftr(sleftr__32),
.o_weight(weight__32),
.o_layer(layer__32),
.o_dead(dead__32),
.o_hit(hit__32)
); 
PhotonBlock5 photon31(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__30),
.i_y(y__30),
.i_z(z__30),
.i_ux(ux__30),
.i_uy(uy__30),
.i_uz(uz__30),
.i_sz(sz__30),
.i_sr(sr__30),
.i_sleftz(sleftz__30),
.i_sleftr(sleftr__30),
.i_weight(weight__30),
.i_layer(layer__30),
.i_dead(dead__30),
.i_hit(hit__30),
//Outputs			
.o_x(x__31),
.o_y(y__31),
.o_z(z__31),
.o_ux(ux__31),
.o_uy(uy__31),
.o_uz(uz__31),
.o_sz(sz__31),
.o_sr(sr__31),
.o_sleftz(sleftz__31),
.o_sleftr(sleftr__31),
.o_weight(weight__31),
.o_layer(layer__31),
.o_dead(dead__31),
.o_hit(hit__31)
); 
PhotonBlock5 photon30(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__29),
.i_y(y__29),
.i_z(z__29),
.i_ux(ux__29),
.i_uy(uy__29),
.i_uz(uz__29),
.i_sz(sz__29),
.i_sr(sr__29),
.i_sleftz(sleftz__29),
.i_sleftr(sleftr__29),
.i_weight(weight__29),
.i_layer(layer__29),
.i_dead(dead__29),
.i_hit(hit__29),
//Outputs			
.o_x(x__30),
.o_y(y__30),
.o_z(z__30),
.o_ux(ux__30),
.o_uy(uy__30),
.o_uz(uz__30),
.o_sz(sz__30),
.o_sr(sr__30),
.o_sleftz(sleftz__30),
.o_sleftr(sleftr__30),
.o_weight(weight__30),
.o_layer(layer__30),
.o_dead(dead__30),
.o_hit(hit__30)
); 
PhotonBlock5 photon29(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__28),
.i_y(y__28),
.i_z(z__28),
.i_ux(ux__28),
.i_uy(uy__28),
.i_uz(uz__28),
.i_sz(sz__28),
.i_sr(sr__28),
.i_sleftz(sleftz__28),
.i_sleftr(sleftr__28),
.i_weight(weight__28),
.i_layer(layer__28),
.i_dead(dead__28),
.i_hit(hit__28),
//Outputs			
.o_x(x__29),
.o_y(y__29),
.o_z(z__29),
.o_ux(ux__29),
.o_uy(uy__29),
.o_uz(uz__29),
.o_sz(sz__29),
.o_sr(sr__29),
.o_sleftz(sleftz__29),
.o_sleftr(sleftr__29),
.o_weight(weight__29),
.o_layer(layer__29),
.o_dead(dead__29),
.o_hit(hit__29)
); 
PhotonBlock5 photon28(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__27),
.i_y(y__27),
.i_z(z__27),
.i_ux(ux__27),
.i_uy(uy__27),
.i_uz(uz__27),
.i_sz(sz__27),
.i_sr(sr__27),
.i_sleftz(sleftz__27),
.i_sleftr(sleftr__27),
.i_weight(weight__27),
.i_layer(layer__27),
.i_dead(dead__27),
.i_hit(hit__27),
//Outputs			
.o_x(x__28),
.o_y(y__28),
.o_z(z__28),
.o_ux(ux__28),
.o_uy(uy__28),
.o_uz(uz__28),
.o_sz(sz__28),
.o_sr(sr__28),
.o_sleftz(sleftz__28),
.o_sleftr(sleftr__28),
.o_weight(weight__28),
.o_layer(layer__28),
.o_dead(dead__28),
.o_hit(hit__28)
); 
PhotonBlock5 photon27(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__26),
.i_y(y__26),
.i_z(z__26),
.i_ux(ux__26),
.i_uy(uy__26),
.i_uz(uz__26),
.i_sz(sz__26),
.i_sr(sr__26),
.i_sleftz(sleftz__26),
.i_sleftr(sleftr__26),
.i_weight(weight__26),
.i_layer(layer__26),
.i_dead(dead__26),
.i_hit(hit__26),
//Outputs			
.o_x(x__27),
.o_y(y__27),
.o_z(z__27),
.o_ux(ux__27),
.o_uy(uy__27),
.o_uz(uz__27),
.o_sz(sz__27),
.o_sr(sr__27),
.o_sleftz(sleftz__27),
.o_sleftr(sleftr__27),
.o_weight(weight__27),
.o_layer(layer__27),
.o_dead(dead__27),
.o_hit(hit__27)
); 
PhotonBlock5 photon26(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__25),
.i_y(y__25),
.i_z(z__25),
.i_ux(ux__25),
.i_uy(uy__25),
.i_uz(uz__25),
.i_sz(sz__25),
.i_sr(sr__25),
.i_sleftz(sleftz__25),
.i_sleftr(sleftr__25),
.i_weight(weight__25),
.i_layer(layer__25),
.i_dead(dead__25),
.i_hit(hit__25),
//Outputs			
.o_x(x__26),
.o_y(y__26),
.o_z(z__26),
.o_ux(ux__26),
.o_uy(uy__26),
.o_uz(uz__26),
.o_sz(sz__26),
.o_sr(sr__26),
.o_sleftz(sleftz__26),
.o_sleftr(sleftr__26),
.o_weight(weight__26),
.o_layer(layer__26),
.o_dead(dead__26),
.o_hit(hit__26)
); 
PhotonBlock5 photon25(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__24),
.i_y(y__24),
.i_z(z__24),
.i_ux(ux__24),
.i_uy(uy__24),
.i_uz(uz__24),
.i_sz(sz__24),
.i_sr(sr__24),
.i_sleftz(sleftz__24),
.i_sleftr(sleftr__24),
.i_weight(weight__24),
.i_layer(layer__24),
.i_dead(dead__24),
.i_hit(hit__24),
//Outputs			
.o_x(x__25),
.o_y(y__25),
.o_z(z__25),
.o_ux(ux__25),
.o_uy(uy__25),
.o_uz(uz__25),
.o_sz(sz__25),
.o_sr(sr__25),
.o_sleftz(sleftz__25),
.o_sleftr(sleftr__25),
.o_weight(weight__25),
.o_layer(layer__25),
.o_dead(dead__25),
.o_hit(hit__25)
); 
PhotonBlock5 photon24(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__23),
.i_y(y__23),
.i_z(z__23),
.i_ux(ux__23),
.i_uy(uy__23),
.i_uz(uz__23),
.i_sz(sz__23),
.i_sr(sr__23),
.i_sleftz(sleftz__23),
.i_sleftr(sleftr__23),
.i_weight(weight__23),
.i_layer(layer__23),
.i_dead(dead__23),
.i_hit(hit__23),
//Outputs			
.o_x(x__24),
.o_y(y__24),
.o_z(z__24),
.o_ux(ux__24),
.o_uy(uy__24),
.o_uz(uz__24),
.o_sz(sz__24),
.o_sr(sr__24),
.o_sleftz(sleftz__24),
.o_sleftr(sleftr__24),
.o_weight(weight__24),
.o_layer(layer__24),
.o_dead(dead__24),
.o_hit(hit__24)
); 
PhotonBlock5 photon23(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__22),
.i_y(y__22),
.i_z(z__22),
.i_ux(ux__22),
.i_uy(uy__22),
.i_uz(uz__22),
.i_sz(sz__22),
.i_sr(sr__22),
.i_sleftz(sleftz__22),
.i_sleftr(sleftr__22),
.i_weight(weight__22),
.i_layer(layer__22),
.i_dead(dead__22),
.i_hit(hit__22),
//Outputs			
.o_x(x__23),
.o_y(y__23),
.o_z(z__23),
.o_ux(ux__23),
.o_uy(uy__23),
.o_uz(uz__23),
.o_sz(sz__23),
.o_sr(sr__23),
.o_sleftz(sleftz__23),
.o_sleftr(sleftr__23),
.o_weight(weight__23),
.o_layer(layer__23),
.o_dead(dead__23),
.o_hit(hit__23)
); 
PhotonBlock5 photon22(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__21),
.i_y(y__21),
.i_z(z__21),
.i_ux(ux__21),
.i_uy(uy__21),
.i_uz(uz__21),
.i_sz(sz__21),
.i_sr(sr__21),
.i_sleftz(sleftz__21),
.i_sleftr(sleftr__21),
.i_weight(weight__21),
.i_layer(layer__21),
.i_dead(dead__21),
.i_hit(hit__21),
//Outputs			
.o_x(x__22),
.o_y(y__22),
.o_z(z__22),
.o_ux(ux__22),
.o_uy(uy__22),
.o_uz(uz__22),
.o_sz(sz__22),
.o_sr(sr__22),
.o_sleftz(sleftz__22),
.o_sleftr(sleftr__22),
.o_weight(weight__22),
.o_layer(layer__22),
.o_dead(dead__22),
.o_hit(hit__22)
); 
PhotonBlock5 photon21(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__20),
.i_y(y__20),
.i_z(z__20),
.i_ux(ux__20),
.i_uy(uy__20),
.i_uz(uz__20),
.i_sz(sz__20),
.i_sr(sr__20),
.i_sleftz(sleftz__20),
.i_sleftr(sleftr__20),
.i_weight(weight__20),
.i_layer(layer__20),
.i_dead(dead__20),
.i_hit(hit__20),
//Outputs			
.o_x(x__21),
.o_y(y__21),
.o_z(z__21),
.o_ux(ux__21),
.o_uy(uy__21),
.o_uz(uz__21),
.o_sz(sz__21),
.o_sr(sr__21),
.o_sleftz(sleftz__21),
.o_sleftr(sleftr__21),
.o_weight(weight__21),
.o_layer(layer__21),
.o_dead(dead__21),
.o_hit(hit__21)
); 
PhotonBlock5 photon20(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__19),
.i_y(y__19),
.i_z(z__19),
.i_ux(ux__19),
.i_uy(uy__19),
.i_uz(uz__19),
.i_sz(sz__19),
.i_sr(sr__19),
.i_sleftz(sleftz__19),
.i_sleftr(sleftr__19),
.i_weight(weight__19),
.i_layer(layer__19),
.i_dead(dead__19),
.i_hit(hit__19),
//Outputs			
.o_x(x__20),
.o_y(y__20),
.o_z(z__20),
.o_ux(ux__20),
.o_uy(uy__20),
.o_uz(uz__20),
.o_sz(sz__20),
.o_sr(sr__20),
.o_sleftz(sleftz__20),
.o_sleftr(sleftr__20),
.o_weight(weight__20),
.o_layer(layer__20),
.o_dead(dead__20),
.o_hit(hit__20)
); 
PhotonBlock5 photon19(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__18),
.i_y(y__18),
.i_z(z__18),
.i_ux(ux__18),
.i_uy(uy__18),
.i_uz(uz__18),
.i_sz(sz__18),
.i_sr(sr__18),
.i_sleftz(sleftz__18),
.i_sleftr(sleftr__18),
.i_weight(weight__18),
.i_layer(layer__18),
.i_dead(dead__18),
.i_hit(hit__18),
//Outputs			
.o_x(x__19),
.o_y(y__19),
.o_z(z__19),
.o_ux(ux__19),
.o_uy(uy__19),
.o_uz(uz__19),
.o_sz(sz__19),
.o_sr(sr__19),
.o_sleftz(sleftz__19),
.o_sleftr(sleftr__19),
.o_weight(weight__19),
.o_layer(layer__19),
.o_dead(dead__19),
.o_hit(hit__19)
); 
PhotonBlock5 photon18(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__17),
.i_y(y__17),
.i_z(z__17),
.i_ux(ux__17),
.i_uy(uy__17),
.i_uz(uz__17),
.i_sz(sz__17),
.i_sr(sr__17),
.i_sleftz(sleftz__17),
.i_sleftr(sleftr__17),
.i_weight(weight__17),
.i_layer(layer__17),
.i_dead(dead__17),
.i_hit(hit__17),
//Outputs			
.o_x(x__18),
.o_y(y__18),
.o_z(z__18),
.o_ux(ux__18),
.o_uy(uy__18),
.o_uz(uz__18),
.o_sz(sz__18),
.o_sr(sr__18),
.o_sleftz(sleftz__18),
.o_sleftr(sleftr__18),
.o_weight(weight__18),
.o_layer(layer__18),
.o_dead(dead__18),
.o_hit(hit__18)
); 
PhotonBlock5 photon17(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__16),
.i_y(y__16),
.i_z(z__16),
.i_ux(ux__16),
.i_uy(uy__16),
.i_uz(uz__16),
.i_sz(sz__16),
.i_sr(sr__16),
.i_sleftz(sleftz__16),
.i_sleftr(sleftr__16),
.i_weight(weight__16),
.i_layer(layer__16),
.i_dead(dead__16),
.i_hit(hit__16),
//Outputs			
.o_x(x__17),
.o_y(y__17),
.o_z(z__17),
.o_ux(ux__17),
.o_uy(uy__17),
.o_uz(uz__17),
.o_sz(sz__17),
.o_sr(sr__17),
.o_sleftz(sleftz__17),
.o_sleftr(sleftr__17),
.o_weight(weight__17),
.o_layer(layer__17),
.o_dead(dead__17),
.o_hit(hit__17)
); 
PhotonBlock5 photon16(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__15),
.i_y(y__15),
.i_z(z__15),
.i_ux(ux__15),
.i_uy(uy__15),
.i_uz(uz__15),
.i_sz(sz__15),
.i_sr(sr__15),
.i_sleftz(sleftz__15),
.i_sleftr(sleftr__15),
.i_weight(weight__15),
.i_layer(layer__15),
.i_dead(dead__15),
.i_hit(hit__15),
//Outputs			
.o_x(x__16),
.o_y(y__16),
.o_z(z__16),
.o_ux(ux__16),
.o_uy(uy__16),
.o_uz(uz__16),
.o_sz(sz__16),
.o_sr(sr__16),
.o_sleftz(sleftz__16),
.o_sleftr(sleftr__16),
.o_weight(weight__16),
.o_layer(layer__16),
.o_dead(dead__16),
.o_hit(hit__16)
); 
PhotonBlock5 photon15(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__14),
.i_y(y__14),
.i_z(z__14),
.i_ux(ux__14),
.i_uy(uy__14),
.i_uz(uz__14),
.i_sz(sz__14),
.i_sr(sr__14),
.i_sleftz(sleftz__14),
.i_sleftr(sleftr__14),
.i_weight(weight__14),
.i_layer(layer__14),
.i_dead(dead__14),
.i_hit(hit__14),
//Outputs			
.o_x(x__15),
.o_y(y__15),
.o_z(z__15),
.o_ux(ux__15),
.o_uy(uy__15),
.o_uz(uz__15),
.o_sz(sz__15),
.o_sr(sr__15),
.o_sleftz(sleftz__15),
.o_sleftr(sleftr__15),
.o_weight(weight__15),
.o_layer(layer__15),
.o_dead(dead__15),
.o_hit(hit__15)
); 
PhotonBlock5 photon14(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__13),
.i_y(y__13),
.i_z(z__13),
.i_ux(ux__13),
.i_uy(uy__13),
.i_uz(uz__13),
.i_sz(sz__13),
.i_sr(sr__13),
.i_sleftz(sleftz__13),
.i_sleftr(sleftr__13),
.i_weight(weight__13),
.i_layer(layer__13),
.i_dead(dead__13),
.i_hit(hit__13),
//Outputs			
.o_x(x__14),
.o_y(y__14),
.o_z(z__14),
.o_ux(ux__14),
.o_uy(uy__14),
.o_uz(uz__14),
.o_sz(sz__14),
.o_sr(sr__14),
.o_sleftz(sleftz__14),
.o_sleftr(sleftr__14),
.o_weight(weight__14),
.o_layer(layer__14),
.o_dead(dead__14),
.o_hit(hit__14)
); 
PhotonBlock5 photon13(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__12),
.i_y(y__12),
.i_z(z__12),
.i_ux(ux__12),
.i_uy(uy__12),
.i_uz(uz__12),
.i_sz(sz__12),
.i_sr(sr__12),
.i_sleftz(sleftz__12),
.i_sleftr(sleftr__12),
.i_weight(weight__12),
.i_layer(layer__12),
.i_dead(dead__12),
.i_hit(hit__12),
//Outputs			
.o_x(x__13),
.o_y(y__13),
.o_z(z__13),
.o_ux(ux__13),
.o_uy(uy__13),
.o_uz(uz__13),
.o_sz(sz__13),
.o_sr(sr__13),
.o_sleftz(sleftz__13),
.o_sleftr(sleftr__13),
.o_weight(weight__13),
.o_layer(layer__13),
.o_dead(dead__13),
.o_hit(hit__13)
); 
PhotonBlock5 photon12(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__11),
.i_y(y__11),
.i_z(z__11),
.i_ux(ux__11),
.i_uy(uy__11),
.i_uz(uz__11),
.i_sz(sz__11),
.i_sr(sr__11),
.i_sleftz(sleftz__11),
.i_sleftr(sleftr__11),
.i_weight(weight__11),
.i_layer(layer__11),
.i_dead(dead__11),
.i_hit(hit__11),
//Outputs			
.o_x(x__12),
.o_y(y__12),
.o_z(z__12),
.o_ux(ux__12),
.o_uy(uy__12),
.o_uz(uz__12),
.o_sz(sz__12),
.o_sr(sr__12),
.o_sleftz(sleftz__12),
.o_sleftr(sleftr__12),
.o_weight(weight__12),
.o_layer(layer__12),
.o_dead(dead__12),
.o_hit(hit__12)
); 
PhotonBlock5 photon11(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__10),
.i_y(y__10),
.i_z(z__10),
.i_ux(ux__10),
.i_uy(uy__10),
.i_uz(uz__10),
.i_sz(sz__10),
.i_sr(sr__10),
.i_sleftz(sleftz__10),
.i_sleftr(sleftr__10),
.i_weight(weight__10),
.i_layer(layer__10),
.i_dead(dead__10),
.i_hit(hit__10),
//Outputs			
.o_x(x__11),
.o_y(y__11),
.o_z(z__11),
.o_ux(ux__11),
.o_uy(uy__11),
.o_uz(uz__11),
.o_sz(sz__11),
.o_sr(sr__11),
.o_sleftz(sleftz__11),
.o_sleftr(sleftr__11),
.o_weight(weight__11),
.o_layer(layer__11),
.o_dead(dead__11),
.o_hit(hit__11)
); 
PhotonBlock5 photon10(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__9),
.i_y(y__9),
.i_z(z__9),
.i_ux(ux__9),
.i_uy(uy__9),
.i_uz(uz__9),
.i_sz(sz__9),
.i_sr(sr__9),
.i_sleftz(sleftz__9),
.i_sleftr(sleftr__9),
.i_weight(weight__9),
.i_layer(layer__9),
.i_dead(dead__9),
.i_hit(hit__9),
//Outputs			
.o_x(x__10),
.o_y(y__10),
.o_z(z__10),
.o_ux(ux__10),
.o_uy(uy__10),
.o_uz(uz__10),
.o_sz(sz__10),
.o_sr(sr__10),
.o_sleftz(sleftz__10),
.o_sleftr(sleftr__10),
.o_weight(weight__10),
.o_layer(layer__10),
.o_dead(dead__10),
.o_hit(hit__10)
); 
PhotonBlock5 photon9(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__8),
.i_y(y__8),
.i_z(z__8),
.i_ux(ux__8),
.i_uy(uy__8),
.i_uz(uz__8),
.i_sz(sz__8),
.i_sr(sr__8),
.i_sleftz(sleftz__8),
.i_sleftr(sleftr__8),
.i_weight(weight__8),
.i_layer(layer__8),
.i_dead(dead__8),
.i_hit(hit__8),
//Outputs			
.o_x(x__9),
.o_y(y__9),
.o_z(z__9),
.o_ux(ux__9),
.o_uy(uy__9),
.o_uz(uz__9),
.o_sz(sz__9),
.o_sr(sr__9),
.o_sleftz(sleftz__9),
.o_sleftr(sleftr__9),
.o_weight(weight__9),
.o_layer(layer__9),
.o_dead(dead__9),
.o_hit(hit__9)
); 
PhotonBlock5 photon8(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__7),
.i_y(y__7),
.i_z(z__7),
.i_ux(ux__7),
.i_uy(uy__7),
.i_uz(uz__7),
.i_sz(sz__7),
.i_sr(sr__7),
.i_sleftz(sleftz__7),
.i_sleftr(sleftr__7),
.i_weight(weight__7),
.i_layer(layer__7),
.i_dead(dead__7),
.i_hit(hit__7),
//Outputs			
.o_x(x__8),
.o_y(y__8),
.o_z(z__8),
.o_ux(ux__8),
.o_uy(uy__8),
.o_uz(uz__8),
.o_sz(sz__8),
.o_sr(sr__8),
.o_sleftz(sleftz__8),
.o_sleftr(sleftr__8),
.o_weight(weight__8),
.o_layer(layer__8),
.o_dead(dead__8),
.o_hit(hit__8)
); 
PhotonBlock5 photon7(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__6),
.i_y(y__6),
.i_z(z__6),
.i_ux(ux__6),
.i_uy(uy__6),
.i_uz(uz__6),
.i_sz(sz__6),
.i_sr(sr__6),
.i_sleftz(sleftz__6),
.i_sleftr(sleftr__6),
.i_weight(weight__6),
.i_layer(layer__6),
.i_dead(dead__6),
.i_hit(hit__6),
//Outputs			
.o_x(x__7),
.o_y(y__7),
.o_z(z__7),
.o_ux(ux__7),
.o_uy(uy__7),
.o_uz(uz__7),
.o_sz(sz__7),
.o_sr(sr__7),
.o_sleftz(sleftz__7),
.o_sleftr(sleftr__7),
.o_weight(weight__7),
.o_layer(layer__7),
.o_dead(dead__7),
.o_hit(hit__7)
); 
PhotonBlock5 photon6(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__5),
.i_y(y__5),
.i_z(z__5),
.i_ux(ux__5),
.i_uy(uy__5),
.i_uz(uz__5),
.i_sz(sz__5),
.i_sr(sr__5),
.i_sleftz(sleftz__5),
.i_sleftr(sleftr__5),
.i_weight(weight__5),
.i_layer(layer__5),
.i_dead(dead__5),
.i_hit(hit__5),
//Outputs			
.o_x(x__6),
.o_y(y__6),
.o_z(z__6),
.o_ux(ux__6),
.o_uy(uy__6),
.o_uz(uz__6),
.o_sz(sz__6),
.o_sr(sr__6),
.o_sleftz(sleftz__6),
.o_sleftr(sleftr__6),
.o_weight(weight__6),
.o_layer(layer__6),
.o_dead(dead__6),
.o_hit(hit__6)
); 
PhotonBlock5 photon5(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__4),
.i_y(y__4),
.i_z(z__4),
.i_ux(ux__4),
.i_uy(uy__4),
.i_uz(uz__4),
.i_sz(sz__4),
.i_sr(sr__4),
.i_sleftz(sleftz__4),
.i_sleftr(sleftr__4),
.i_weight(weight__4),
.i_layer(layer__4),
.i_dead(dead__4),
.i_hit(hit__4),
//Outputs			
.o_x(x__5),
.o_y(y__5),
.o_z(z__5),
.o_ux(ux__5),
.o_uy(uy__5),
.o_uz(uz__5),
.o_sz(sz__5),
.o_sr(sr__5),
.o_sleftz(sleftz__5),
.o_sleftr(sleftr__5),
.o_weight(weight__5),
.o_layer(layer__5),
.o_dead(dead__5),
.o_hit(hit__5)
); 
PhotonBlock5 photon4(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__3),
.i_y(y__3),
.i_z(z__3),
.i_ux(ux__3),
.i_uy(uy__3),
.i_uz(uz__3),
.i_sz(sz__3),
.i_sr(sr__3),
.i_sleftz(sleftz__3),
.i_sleftr(sleftr__3),
.i_weight(weight__3),
.i_layer(layer__3),
.i_dead(dead__3),
.i_hit(hit__3),
//Outputs			
.o_x(x__4),
.o_y(y__4),
.o_z(z__4),
.o_ux(ux__4),
.o_uy(uy__4),
.o_uz(uz__4),
.o_sz(sz__4),
.o_sr(sr__4),
.o_sleftz(sleftz__4),
.o_sleftr(sleftr__4),
.o_weight(weight__4),
.o_layer(layer__4),
.o_dead(dead__4),
.o_hit(hit__4)
); 
PhotonBlock5 photon3(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__2),
.i_y(y__2),
.i_z(z__2),
.i_ux(ux__2),
.i_uy(uy__2),
.i_uz(uz__2),
.i_sz(sz__2),
.i_sr(sr__2),
.i_sleftz(sleftz__2),
.i_sleftr(sleftr__2),
.i_weight(weight__2),
.i_layer(layer__2),
.i_dead(dead__2),
.i_hit(hit__2),
//Outputs			
.o_x(x__3),
.o_y(y__3),
.o_z(z__3),
.o_ux(ux__3),
.o_uy(uy__3),
.o_uz(uz__3),
.o_sz(sz__3),
.o_sr(sr__3),
.o_sleftz(sleftz__3),
.o_sleftr(sleftr__3),
.o_weight(weight__3),
.o_layer(layer__3),
.o_dead(dead__3),
.o_hit(hit__3)
); 
PhotonBlock5 photon2(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__1),
.i_y(y__1),
.i_z(z__1),
.i_ux(ux__1),
.i_uy(uy__1),
.i_uz(uz__1),
.i_sz(sz__1),
.i_sr(sr__1),
.i_sleftz(sleftz__1),
.i_sleftr(sleftr__1),
.i_weight(weight__1),
.i_layer(layer__1),
.i_dead(dead__1),
.i_hit(hit__1),
//Outputs			
.o_x(x__2),
.o_y(y__2),
.o_z(z__2),
.o_ux(ux__2),
.o_uy(uy__2),
.o_uz(uz__2),
.o_sz(sz__2),
.o_sr(sr__2),
.o_sleftz(sleftz__2),
.o_sleftr(sleftr__2),
.o_weight(weight__2),
.o_layer(layer__2),
.o_dead(dead__2),
.o_hit(hit__2)
); 
PhotonBlock5 photon1(
//Inputs

.clock(clock), 
.reset(reset),
.enable(enable), 
.i_x(x__0),
.i_y(y__0),
.i_z(z__0),
.i_ux(ux__0),
.i_uy(uy__0),
.i_uz(uz__0),
.i_sz(sz__0),
.i_sr(sr__0),
.i_sleftz(sleftz__0),
.i_sleftr(sleftr__0),
.i_weight(weight__0),
.i_layer(layer__0),
.i_dead(dead__0),
.i_hit(hit__0),
//Outputs			
.o_x(x__1),
.o_y(y__1),
.o_z(z__1),
.o_ux(ux__1),
.o_uy(uy__1),
.o_uz(uz__1),
.o_sz(sz__1),
.o_sr(sr__1),
.o_sleftz(sleftz__1),
.o_sleftr(sleftr__1),
.o_weight(weight__1),
.o_layer(layer__1),
.o_dead(dead__1),
.o_hit(hit__1)
); 



//////////////////////////////////////////////////////////////////////////////
//Tapping into the Registered Pipeline
//***NOTE: Index must be incremented by 1 compared to SystemC version 
//////////////////////////////////////////////////////////////////////////////

//>>>>>>>>>>>>> Absorber <<<<<<<<<<<<<<<<<<
wire	[`BIT_WIDTH-1:0]			x_pipe, y_pipe,	z_pipe;
wire	[`LAYER_WIDTH-1:0]			layer_pipe;
assign x_pipe=x__2; 
assign y_pipe=y__2; 
assign z_pipe=z__14;  //TODO: Check square-root latency and modify z[14] if needed!!!!
assign layer_pipe=layer__4;

//>>>>>>>>>>>>> ScattererReflectorWrapper <<<<<<<<<<<<<<<<<<
wire	[`BIT_WIDTH-1:0]			ux_scatterer;
wire	[`BIT_WIDTH-1:0]			uy_scatterer;
wire	[`BIT_WIDTH-1:0]			uz_scatterer;
wire	[`BIT_WIDTH-1:0]			ux_reflector;
wire	[`BIT_WIDTH-1:0]			uy_reflector;
wire	[`BIT_WIDTH-1:0]			uz_reflector;
wire	[`LAYER_WIDTH-1:0]			layer_reflector;
wire					dead_reflector;




//////////////////////////////////////////////////////////////////////////////
//Connect up different modules
//////////////////////////////////////////////////////////////////////////////

//>>>>>>>>>>>>> Absorber <<<<<<<<<<<<<<<<<<

Absorber absorb (    //INPUTS
                     .clock(clock) , .reset(reset), .enable(enable), 
                     
                     //From hopper
                     .weight_hop(i_weight), .hit_hop(i_hit), .dead_hop(i_dead),

                     //From Shared Registers
                     .x_pipe (x_pipe), .y_pipe (y_pipe), .z_pipe(z_pipe), .layer_pipe(layer_pipe),
                     
                     //From System Register File (5 layers)
                     .muaFraction1(muaFraction1), .muaFraction2(muaFraction2), .muaFraction3(muaFraction3), .muaFraction4(muaFraction4), .muaFraction5(muaFraction5),  
                     
                     //Dual-port Mem
                     .data(data), .rdaddress(rdaddress), .wraddress(wraddress), 
                     .wren(wren), .q(q),
                     
                     //OUTPUT
                     .weight_absorber(o_weight)

                     ); 

//>>>>>>>>>>>>> ScattererReflectorWrapper <<<<<<<<<<<<<<<<<<

ScattererReflectorWrapper scattererReflector(
	//Inputs
	.clock(clock),
	.reset(reset),
	.enable(enable),
		//Inputs
		
		//Photon values
		.i_uz1_pipeWrapper(uz__1),
		.i_hit2_pipeWrapper(hit__1),
		.i_ux3_pipeWrapper(ux__3),
		.i_uz3_pipeWrapper(uz__3),
		.i_layer3_pipeWrapper(layer__3),
		.i_hit4_pipeWrapper(hit__3),
		.i_hit6_pipeWrapper(hit__5),
		.i_hit16_pipeWrapper(hit__15), 
		.i_layer31_pipeWrapper(layer__31),
		.i_uy32_pipeWrapper(uy__32),
		.i_uz32_pipeWrapper(uz__32),
		.i_hit33_pipeWrapper(hit__32),
		.i_ux33_pipeWrapper(ux__33),
		.i_uy33_pipeWrapper(uy__33),
		.i_hit34_pipeWrapper(hit__33),
		.i_ux35_pipeWrapper(ux__35),
		.i_uy35_pipeWrapper(uy__35),
		.i_uz35_pipeWrapper(uz__35),
		.i_layer35_pipeWrapper(layer__35),
		.i_hit36_pipeWrapper(hit__35),
		.i_ux36_pipeWrapper(ux__36),
		.i_uy36_pipeWrapper(uy__36),
		.i_uz36_pipeWrapper(uz__36),
		.i_layer36_pipeWrapper(layer__36),
		.i_dead36_pipeWrapper(dead__36),
	
		//Memory Interface
			//Inputs
		.rand2(rand2),
		.rand3(rand3),
		.rand5(rand5),
		.sint(sint),
		.cost(cost),
		.up_rFresnel(up_rFresnel),
		.down_rFresnel(down_rFresnel),
			//Outputs
		.tindex(tindex),
		.fresIndex(fresIndex),
		
		//Constants
		.down_niOverNt_1(down_niOverNt_1),
		.down_niOverNt_2(down_niOverNt_2),
		.down_niOverNt_3(down_niOverNt_3),
		.down_niOverNt_4(down_niOverNt_4),
		.down_niOverNt_5(down_niOverNt_5),
		.up_niOverNt_1(up_niOverNt_1),
		.up_niOverNt_2(up_niOverNt_2),
		.up_niOverNt_3(up_niOverNt_3),
		.up_niOverNt_4(up_niOverNt_4),
		.up_niOverNt_5(up_niOverNt_5),
		.down_niOverNt_2_1(down_niOverNt_2_1),
		.down_niOverNt_2_2(down_niOverNt_2_2),
		.down_niOverNt_2_3(down_niOverNt_2_3),
		.down_niOverNt_2_4(down_niOverNt_2_4),
		.down_niOverNt_2_5(down_niOverNt_2_5),
		.up_niOverNt_2_1(up_niOverNt_2_1),
		.up_niOverNt_2_2(up_niOverNt_2_2),
		.up_niOverNt_2_3(up_niOverNt_2_3),
		.up_niOverNt_2_4(up_niOverNt_2_4),
		.up_niOverNt_2_5(up_niOverNt_2_5),
		.downCritAngle_0(downCritAngle_0),
		.downCritAngle_1(downCritAngle_1),
		.downCritAngle_2(downCritAngle_2),
		.downCritAngle_3(downCritAngle_3),
		.downCritAngle_4(downCritAngle_4),
		.upCritAngle_0(upCritAngle_0),
		.upCritAngle_1(upCritAngle_1),
		.upCritAngle_2(upCritAngle_2),
		.upCritAngle_3(upCritAngle_3),
		.upCritAngle_4(upCritAngle_4),
		
		//Outputs
		.ux_scatterer(ux_scatterer),
		.uy_scatterer(uy_scatterer),
		.uz_scatterer(uz_scatterer),
		
		.ux_reflector(ux_reflector),
		.uy_reflector(uy_reflector),
		.uz_reflector(uz_reflector),
		.layer_reflector(layer_reflector),
		.dead_reflector(dead_reflector)
	);
	
	
//////////////////////////////////////////////////////////////////////
////  dead or alive MUX                                           ////
////                                                              ////
////  Description:                                                ////
////    Used to determine whether the output from the scatterer   ////
////    or the reflector should be used in any clock cycle        ////
//////////////////////////////////////////////////////////////////////

always @ (hit__37 or ux_scatterer or uy_scatterer or uz_scatterer or layer__37 or dead__37 or
			ux_reflector or uy_reflector or uz_reflector or layer_reflector or dead_reflector) begin
   case (hit__37)   
   0: begin
       o_ux = ux_scatterer;
       o_uy = uy_scatterer;
       o_uz = uz_scatterer;
       o_layer = layer__37;
       o_dead = dead__37;          
   end
   1: begin
      o_ux = ux_reflector;
      o_uy = uy_reflector;
      o_uz = uz_reflector;
      o_layer = layer_reflector;
      o_dead = dead_reflector;
   end   
   endcase 
    
end

endmodule

//Photons that make up the register pipeline
module PhotonBlock5(
	//Inputs
	clock,
	reset,
	enable,
	
	i_x,
	i_y,
	i_z,
	i_ux,
	i_uy,
	i_uz,
	i_sz,
	i_sr,
	i_sleftz,
	i_sleftr,
	i_weight,
	i_layer,
	i_dead,
	i_hit,
	//Outputs
	o_x,
	o_y,
	o_z,
	o_ux,
	o_uy,
	o_uz,
	o_sz,
	o_sr,
	o_sleftz,
	o_sleftr,
	o_weight,
	o_layer,
	o_dead,
	o_hit
	);

//parameter BIT_WIDTH=32;
//parameter LAYER_WIDTH=3;

input				clock;
input				reset;
input				enable;

input	[`BIT_WIDTH-1:0]			i_x;
input	[`BIT_WIDTH-1:0]			i_y;
input	[`BIT_WIDTH-1:0]			i_z;
input	[`BIT_WIDTH-1:0]			i_ux;
input	[`BIT_WIDTH-1:0]			i_uy;
input	[`BIT_WIDTH-1:0]			i_uz;
input	[`BIT_WIDTH-1:0]			i_sz;
input	[`BIT_WIDTH-1:0]			i_sr;
input	[`BIT_WIDTH-1:0]			i_sleftz;
input	[`BIT_WIDTH-1:0]			i_sleftr;
input	[`BIT_WIDTH-1:0]			i_weight;
input	[`LAYER_WIDTH-1:0]			i_layer;
input				i_dead;
input				i_hit;


output	[`BIT_WIDTH-1:0]			o_x;
output	[`BIT_WIDTH-1:0]			o_y;
output	[`BIT_WIDTH-1:0]			o_z;
output	[`BIT_WIDTH-1:0]			o_ux;
output	[`BIT_WIDTH-1:0]			o_uy;
output	[`BIT_WIDTH-1:0]			o_uz;
output	[`BIT_WIDTH-1:0]			o_sz;
output	[`BIT_WIDTH-1:0]			o_sr;
output	[`BIT_WIDTH-1:0]			o_sleftz;
output	[`BIT_WIDTH-1:0]			o_sleftr;
output	[`BIT_WIDTH-1:0]			o_weight;
output	[`LAYER_WIDTH-1:0]			o_layer;
output				o_dead;
output				o_hit;


wire				clock;
wire				reset;
wire				enable;

wire	[`BIT_WIDTH-1:0]			i_x;
wire	[`BIT_WIDTH-1:0]			i_y;
wire	[`BIT_WIDTH-1:0]			i_z;
wire	[`BIT_WIDTH-1:0]			i_ux;
wire	[`BIT_WIDTH-1:0]			i_uy;
wire	[`BIT_WIDTH-1:0]			i_uz;
wire	[`BIT_WIDTH-1:0]			i_sz;
wire	[`BIT_WIDTH-1:0]			i_sr;
wire	[`BIT_WIDTH-1:0]			i_sleftz;
wire	[`BIT_WIDTH-1:0]			i_sleftr;
wire	[`BIT_WIDTH-1:0]			i_weight;
wire	[`LAYER_WIDTH-1:0]			i_layer;
wire				i_dead;
wire				i_hit;


reg	[`BIT_WIDTH-1:0]			o_x;
reg	[`BIT_WIDTH-1:0]			o_y;
reg	[`BIT_WIDTH-1:0]			o_z;
reg	[`BIT_WIDTH-1:0]			o_ux;
reg	[`BIT_WIDTH-1:0]			o_uy;
reg	[`BIT_WIDTH-1:0]			o_uz;
reg	[`BIT_WIDTH-1:0]			o_sz;
reg	[`BIT_WIDTH-1:0]			o_sr;
reg	[`BIT_WIDTH-1:0]			o_sleftz;
reg	[`BIT_WIDTH-1:0]			o_sleftr;
reg	[`BIT_WIDTH-1:0]			o_weight;
reg	[`LAYER_WIDTH-1:0]			o_layer;
reg				o_dead;
reg				o_hit;


always @ (posedge clock)
	if(reset) begin
		o_x		<=	{`BIT_WIDTH{1'b0}};
		o_y		<=	{`BIT_WIDTH{1'b0}};
		o_z		<=	{`BIT_WIDTH{1'b0}};
		o_ux		<=	{`BIT_WIDTH{1'b0}};
		o_uy		<=	{`BIT_WIDTH{1'b0}};
		o_uz		<=	{`BIT_WIDTH{1'b0}};
		o_sz		<=	{`BIT_WIDTH{1'b0}};
		o_sr		<=	{`BIT_WIDTH{1'b0}};
		o_sleftz	<=	{`BIT_WIDTH{1'b0}};
		o_sleftr	<=	{`BIT_WIDTH{1'b0}};
		o_weight	<=	{`BIT_WIDTH{1'b0}};
		o_layer		<=	{`LAYER_WIDTH{1'b0}};
		o_dead		<=	1'b1;
		o_hit		<=	1'b0;
	end else if(enable) begin
		o_x		<=	i_x;
		o_y		<=	i_y;
		o_z		<=	i_z;
		o_ux		<=	i_ux;
		o_uy		<=	i_uy;
		o_uz		<=	i_uz;
		o_sz		<=	i_sz;
		o_sr		<=	i_sr;
		o_sleftz	<=	i_sleftz;
		o_sleftr	<=	i_sleftr;
		o_weight	<=	i_weight;
		o_layer		<=	i_layer;
		o_dead		<=	i_dead;
		o_hit		<=	i_hit;
	end
endmodule


//module FluenceUpdate (    //INPUTS
module Absorber ( 	//INPUTS
                     clock, reset, enable, 
                     
                     //From hopper
                     weight_hop, hit_hop, dead_hop,

                     //From Shared Registers
                     x_pipe, y_pipe, z_pipe, layer_pipe,
                     
                     //From System Register File (5 layers)
                     muaFraction1, muaFraction2, muaFraction3, muaFraction4, muaFraction5,  
                     
                     //I/O to on-chip mem -- check interface
                     data, rdaddress, wraddress, wren, q,
                     
                     //OUTPUT
                     weight_absorber
                     
                     ); 


//////////////////////////////////////////////////////////////////////////////
//PARAMETERS
//////////////////////////////////////////////////////////////////////////////
//parameter `NR=256;              
//parameter `NZ=256;              
//
//parameter `NR_EXP=8;              //meaning `NR=2^`NR_exp or 2^8=256
//parameter `RGRID_SCLAE_EXP=21;    //2^21 = RGRID_SCALE
//parameter `ZGRID_SCLAE_EXP=21;    //2^21 = ZGRID_SCALE
//
//
//parameter `BIT_WIDTH=32;
//parameter `BIT_WIDTH_2=64;
//parameter `WORD_WIDTH=64;
//parameter `ADDR_WIDTH=16;          //256x256=2^8*2^8=2^16
//
//
//parameter `LAYER_WIDTH=3; 
//parameter `PIPE_DEPTH = 37;        


//////////////////////////////////////////////////////////////////////////////
//INPUTS
//////////////////////////////////////////////////////////////////////////////
input clock;        
input reset;
input enable;

//From hopper
input [`BIT_WIDTH-1:0] weight_hop; 
input hit_hop; 
input dead_hop; 

//From Shared Reg
//input signed [`BIT_WIDTH-1:0] x_pipe;
//input signed [`BIT_WIDTH-1:0] y_pipe;
input [`BIT_WIDTH-1:0] x_pipe;
input [`BIT_WIDTH-1:0] y_pipe;
input [`BIT_WIDTH-1:0] z_pipe;
input [`LAYER_WIDTH-1:0] layer_pipe;

//From System Reg File
input [`BIT_WIDTH-1:0] muaFraction1, muaFraction2, muaFraction3, muaFraction4, muaFraction5;  

//////////////////////////////////////////////////////////////////////////////
//OUTPUTS
//////////////////////////////////////////////////////////////////////////////
output [`BIT_WIDTH-1:0] weight_absorber; 

//////////////////////////////////////////////////////////////////////////////
//I/O to on-chip mem -- check interface
//////////////////////////////////////////////////////////////////////////////
output [`WORD_WIDTH-1:0] data; 
output [`ADDR_WIDTH-1:0] rdaddress, wraddress; 
output wren;     
reg wren; 
input [`WORD_WIDTH-1:0] q;

//////////////////////////////////////////////////////////////////////////////
//Local AND Registered Value Variables
//////////////////////////////////////////////////////////////////////////////
//STAGE 1 - Do nothing

//STAGE 2
reg [`BIT_WIDTH_2-1:0] x2_temp, y2_temp;   //From mult
reg [`BIT_WIDTH_2-1:0] x2_P, y2_P;         //Registered Value

//STAGE 3
reg [`BIT_WIDTH_2-1:0] r2_temp, r2_P;   
wire [`BIT_WIDTH_2-1:0] r2_P_wire;  

//STAGE 4
reg [`BIT_WIDTH-1:0]		fractionScaled; 
reg [`BIT_WIDTH-1:0]		weight_P4; 
reg [`BIT_WIDTH-1:0]		r_P; 
wire [`BIT_WIDTH-1:0]		r_P_wire; 

reg [`BIT_WIDTH_2-1:0] product64bit; 
reg [`BIT_WIDTH-1:0] dwa_temp; 

//STAGE 14
reg [`BIT_WIDTH-1:0]		ir_temp; 
reg [`BIT_WIDTH-1:0]		iz_temp; 

//STAGE 15
reg [`BIT_WIDTH-1:0]		ir_P; 
reg [`BIT_WIDTH-1:0]		iz_P; 
reg [`BIT_WIDTH-1:0]		ir_scaled; 
reg [`ADDR_WIDTH-1:0] rADDR_temp; 
reg [`ADDR_WIDTH-1:0] rADDR_16; 

//STAGE 16
reg [`WORD_WIDTH-1:0] oldAbs_MEM;
reg [`WORD_WIDTH-1:0] oldAbs_P; 
reg [`ADDR_WIDTH-1:0] rADDR_17;
 
//STAGE 17
reg [`BIT_WIDTH-1:0] weight_P; 
reg [`BIT_WIDTH-1:0] dwa_P; 
reg [`BIT_WIDTH-1:0] newWeight; 

reg [`WORD_WIDTH-1:0] newAbs_P; 
reg [`WORD_WIDTH-1:0] newAbs_temp; 

//reg [`ADDR_WIDTH-1:0] wADDR; 


//////////////////////////////////////////////////////////////////////////////
//PIPELINE weight, hit, dead
//////////////////////////////////////////////////////////////////////////////
//WIRES FOR CONNECTING REGISTERS

//peter m made this manual
//wire	[32-1:0]			weight	[37:0];

wire	[32-1:0]				weight__0;
wire	[32-1:0]				weight__1;
wire	[32-1:0]				weight__2;
wire	[32-1:0]				weight__3;
wire	[32-1:0]				weight__4;
wire	[32-1:0]				weight__5;
wire	[32-1:0]				weight__6;
wire	[32-1:0]				weight__7;
wire	[32-1:0]				weight__8;
wire	[32-1:0]				weight__9;
wire	[32-1:0]				weight__10;
wire	[32-1:0]				weight__11;
wire	[32-1:0]				weight__12;
wire	[32-1:0]				weight__13;
wire	[32-1:0]				weight__14;
wire	[32-1:0]				weight__15;
wire	[32-1:0]				weight__16;
wire	[32-1:0]				weight__17;
wire	[32-1:0]				weight__18;
wire	[32-1:0]				weight__19;
wire	[32-1:0]				weight__20;
wire	[32-1:0]				weight__21;
wire	[32-1:0]				weight__22;
wire	[32-1:0]				weight__23;
wire	[32-1:0]				weight__24;
wire	[32-1:0]				weight__25;
wire	[32-1:0]				weight__26;
wire	[32-1:0]				weight__27;
wire	[32-1:0]				weight__28;
wire	[32-1:0]				weight__29;
wire	[32-1:0]				weight__30;
wire	[32-1:0]				weight__31;
wire	[32-1:0]				weight__32;
wire	[32-1:0]				weight__33;
wire	[32-1:0]				weight__34;
wire	[32-1:0]				weight__35;
wire	[32-1:0]				weight__36;
wire	[32-1:0]				weight__37;


//wire [37:0]	hit	;
wire					hit__0;
wire					hit__1;
wire					hit__2;
wire					hit__3;
wire					hit__4;
wire					hit__5;
wire					hit__6;
wire					hit__7;
wire					hit__8;
wire					hit__9;
wire					hit__10;
wire					hit__11;
wire					hit__12;
wire					hit__13;
wire					hit__14;
wire					hit__15;
wire					hit__16;
wire					hit__17;
wire					hit__18;
wire					hit__19;
wire					hit__20;
wire					hit__21;
wire					hit__22;
wire					hit__23;
wire					hit__24;
wire					hit__25;
wire					hit__26;
wire					hit__27;
wire					hit__28;
wire					hit__29;
wire					hit__30;
wire					hit__31;
wire					hit__32;
wire					hit__33;
wire					hit__34;
wire					hit__35;
wire					hit__36;
wire					hit__37;



//wire	[37:0]  dead	;
wire					dead__0;
wire					dead__1;
wire					dead__2;
wire					dead__3;
wire					dead__4;
wire					dead__5;
wire					dead__6;
wire					dead__7;
wire					dead__8;
wire					dead__9;
wire					dead__10;
wire					dead__11;
wire					dead__12;
wire					dead__13;
wire					dead__14;
wire					dead__15;
wire					dead__16;
wire					dead__17;
wire					dead__18;
wire					dead__19;
wire					dead__20;
wire					dead__21;
wire					dead__22;
wire					dead__23;
wire					dead__24;
wire					dead__25;
wire					dead__26;
wire					dead__27;
wire					dead__28;
wire					dead__29;
wire					dead__30;
wire					dead__31;
wire					dead__32;
wire					dead__33;
wire					dead__34;
wire					dead__35;
wire					dead__36;
wire					dead__37;


//ASSIGNMENTS FROM INPUTS TO PIPE
assign weight__0 = weight_hop;
assign hit__0 = hit_hop;
assign dead__0 = dead_hop;

//ASSIGNMENTS FROM PIPE TO OUTPUT
assign weight_absorber = weight__37;

//GENERATE PIPELINE
//genvar i;
//generate
//	for(i=`PIPE_DEPTH; i>0; i=i-1) begin: weightHitDeadPipe
//		case(i)  
//		
//		//REGISTER 17 on diagram!!
//		18:   
//		begin
//		   
//		PhotonBlock2 photon(
//			//Inputs
//			.clock(clock),
//			.reset(reset),
//			.enable(enable),
//			
//			.i_x(newWeight),
//			.i_y(hit[17]),
//			.i_z(dead[17]),
//			
//			//Outputs			
//			.o_x(weight[18]),
//			.o_y(hit[18]),
//			.o_z(dead[18])
//		);
//		    
//		end
//		default:
//		begin
//		PhotonBlock2 photon(
//			//Inputs
//			.clock(clock),
//			.reset(reset),
//			.enable(enable),
//			
//			.i_x(weight[i-1]),
//			.i_y(hit[i-1]),
//			.i_z(dead[i-1]),
//			
//			//Outputs			
//			.o_x(weight[i]),
//			.o_y(hit[i]),
//			.o_z(dead[i])
//		);
//		end
//		endcase
//	end
//endgenerate	

//Expand pipeline generation
//special case i = 18 first
PhotonBlock2 photon18(
			//Inputs
			.clock(clock),
			.reset(reset),
			.enable(enable),
			
			.i_x(newWeight),
			.i_y(hit__17),
			.i_z(dead__17),
			
			//Outputs			
			.o_x(weight__18),
			.o_y(hit__18),
			.o_z(dead__18)
		);
		
PhotonBlock2 photon37(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__36),
.i_y(hit__36),
.i_z(dead__36),
//Outputs		
	.o_x(weight__37),
.o_y(hit__37),
.o_z(dead__37)
);

PhotonBlock2 photon36(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__35),
.i_y(hit__35),
.i_z(dead__35),
//Outputs		
	.o_x(weight__36),
.o_y(hit__36),
.o_z(dead__36)
);

PhotonBlock2 photon35(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__34),
.i_y(hit__34),
.i_z(dead__34),
//Outputs		
	.o_x(weight__35),
.o_y(hit__35),
.o_z(dead__35)
);

PhotonBlock2 photon34(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__33),
.i_y(hit__33),
.i_z(dead__33),
//Outputs		
	.o_x(weight__34),
.o_y(hit__34),
.o_z(dead__34)
);

PhotonBlock2 photon33(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__32),
.i_y(hit__32),
.i_z(dead__32),
//Outputs		
	.o_x(weight__33),
.o_y(hit__33),
.o_z(dead__33)
);

PhotonBlock2 photon32(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__31),
.i_y(hit__31),
.i_z(dead__31),
//Outputs		
	.o_x(weight__32),
.o_y(hit__32),
.o_z(dead__32)
);

PhotonBlock2 photon31(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__30),
.i_y(hit__30),
.i_z(dead__30),
//Outputs		
	.o_x(weight__31),
.o_y(hit__31),
.o_z(dead__31)
);

PhotonBlock2 photon30(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__29),
.i_y(hit__29),
.i_z(dead__29),
//Outputs		
	.o_x(weight__30),
.o_y(hit__30),
.o_z(dead__30)
);

PhotonBlock2 photon29(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__28),
.i_y(hit__28),
.i_z(dead__28),
//Outputs		
	.o_x(weight__29),
.o_y(hit__29),
.o_z(dead__29)
);

PhotonBlock2 photon28(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__27),
.i_y(hit__27),
.i_z(dead__27),
//Outputs		
	.o_x(weight__28),
.o_y(hit__28),
.o_z(dead__28)
);

PhotonBlock2 photon27(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__26),
.i_y(hit__26),
.i_z(dead__26),
//Outputs		
	.o_x(weight__27),
.o_y(hit__27),
.o_z(dead__27)
);

PhotonBlock2 photon26(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__25),
.i_y(hit__25),
.i_z(dead__25),
//Outputs		
	.o_x(weight__26),
.o_y(hit__26),
.o_z(dead__26)
);

PhotonBlock2 photon25(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__24),
.i_y(hit__24),
.i_z(dead__24),
//Outputs		
	.o_x(weight__25),
.o_y(hit__25),
.o_z(dead__25)
);

PhotonBlock2 photon24(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__23),
.i_y(hit__23),
.i_z(dead__23),
//Outputs		
	.o_x(weight__24),
.o_y(hit__24),
.o_z(dead__24)
);

PhotonBlock2 photon23(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__22),
.i_y(hit__22),
.i_z(dead__22),
//Outputs		
	.o_x(weight__23),
.o_y(hit__23),
.o_z(dead__23)
);

PhotonBlock2 photon22(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__21),
.i_y(hit__21),
.i_z(dead__21),
//Outputs		
	.o_x(weight__22),
.o_y(hit__22),
.o_z(dead__22)
);

PhotonBlock2 photon21(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__20),
.i_y(hit__20),
.i_z(dead__20),
//Outputs		
	.o_x(weight__21),
.o_y(hit__21),
.o_z(dead__21)
);

PhotonBlock2 photon20(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__19),
.i_y(hit__19),
.i_z(dead__19),
//Outputs		
	.o_x(weight__20),
.o_y(hit__20),
.o_z(dead__20)
);

PhotonBlock2 photon19(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__18),
.i_y(hit__18),
.i_z(dead__18),
//Outputs		
	.o_x(weight__19),
.o_y(hit__19),
.o_z(dead__19)
);


PhotonBlock2 photon17(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__16),
.i_y(hit__16),
.i_z(dead__16),
//Outputs		
	.o_x(weight__17),
.o_y(hit__17),
.o_z(dead__17)
);

PhotonBlock2 photon16(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__15),
.i_y(hit__15),
.i_z(dead__15),
//Outputs		
	.o_x(weight__16),
.o_y(hit__16),
.o_z(dead__16)
);

PhotonBlock2 photon15(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__14),
.i_y(hit__14),
.i_z(dead__14),
//Outputs		
	.o_x(weight__15),
.o_y(hit__15),
.o_z(dead__15)
);

PhotonBlock2 photon14(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__13),
.i_y(hit__13),
.i_z(dead__13),
//Outputs		
	.o_x(weight__14),
.o_y(hit__14),
.o_z(dead__14)
);

PhotonBlock2 photon13(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__12),
.i_y(hit__12),
.i_z(dead__12),
//Outputs		
	.o_x(weight__13),
.o_y(hit__13),
.o_z(dead__13)
);

PhotonBlock2 photon12(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__11),
.i_y(hit__11),
.i_z(dead__11),
//Outputs		
	.o_x(weight__12),
.o_y(hit__12),
.o_z(dead__12)
);

PhotonBlock2 photon11(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__10),
.i_y(hit__10),
.i_z(dead__10),
//Outputs		
	.o_x(weight__11),
.o_y(hit__11),
.o_z(dead__11)
);

PhotonBlock2 photon10(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__9),
.i_y(hit__9),
.i_z(dead__9),
//Outputs		
	.o_x(weight__10),
.o_y(hit__10),
.o_z(dead__10)
);

PhotonBlock2 photon9(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__8),
.i_y(hit__8),
.i_z(dead__8),
//Outputs		
	.o_x(weight__9),
.o_y(hit__9),
.o_z(dead__9)
);

PhotonBlock2 photon8(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__7),
.i_y(hit__7),
.i_z(dead__7),
//Outputs		
	.o_x(weight__8),
.o_y(hit__8),
.o_z(dead__8)
);

PhotonBlock2 photon7(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__6),
.i_y(hit__6),
.i_z(dead__6),
//Outputs		
	.o_x(weight__7),
.o_y(hit__7),
.o_z(dead__7)
);

PhotonBlock2 photon6(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__5),
.i_y(hit__5),
.i_z(dead__5),
//Outputs		
	.o_x(weight__6),
.o_y(hit__6),
.o_z(dead__6)
);

PhotonBlock2 photon5(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__4),
.i_y(hit__4),
.i_z(dead__4),
//Outputs		
	.o_x(weight__5),
.o_y(hit__5),
.o_z(dead__5)
);

PhotonBlock2 photon4(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__3),
.i_y(hit__3),
.i_z(dead__3),
//Outputs		
	.o_x(weight__4),
.o_y(hit__4),
.o_z(dead__4)
);

PhotonBlock2 photon3(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__2),
.i_y(hit__2),
.i_z(dead__2),
//Outputs		
	.o_x(weight__3),
.o_y(hit__3),
.o_z(dead__3)
);

PhotonBlock2 photon2(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__1),
.i_y(hit__1),
.i_z(dead__1),
//Outputs		
	.o_x(weight__2),
.o_y(hit__2),
.o_z(dead__2)
);

PhotonBlock2 photon1(
.clock(clock),
.reset(reset),
.enable(enable),
.i_x(weight__0),
.i_y(hit__0),
.i_z(dead__0),
//Outputs		
	.o_x(weight__1),
.o_y(hit__1),
.o_z(dead__1)
);


//////////////////////////////////////////////////////////////////////////////
//PIPELINE ir,iz,dwa
//////////////////////////////////////////////////////////////////////////////
//WIRES FOR CONNECTING REGISTERS
//wire	[32-1:0]			ir	[37:0];

wire	[32-1:0]				ir__0;
wire	[32-1:0]				ir__1;
wire	[32-1:0]				ir__2;
wire	[32-1:0]				ir__3;
wire	[32-1:0]				ir__4;
wire	[32-1:0]				ir__5;
wire	[32-1:0]				ir__6;
wire	[32-1:0]				ir__7;
wire	[32-1:0]				ir__8;
wire	[32-1:0]				ir__9;
wire	[32-1:0]				ir__10;
wire	[32-1:0]				ir__11;
wire	[32-1:0]				ir__12;
wire	[32-1:0]				ir__13;
wire	[32-1:0]				ir__14;
wire	[32-1:0]				ir__15;
wire	[32-1:0]				ir__16;
wire	[32-1:0]				ir__17;
wire	[32-1:0]				ir__18;
wire	[32-1:0]				ir__19;
wire	[32-1:0]				ir__20;
wire	[32-1:0]				ir__21;
wire	[32-1:0]				ir__22;
wire	[32-1:0]				ir__23;
wire	[32-1:0]				ir__24;
wire	[32-1:0]				ir__25;
wire	[32-1:0]				ir__26;
wire	[32-1:0]				ir__27;
wire	[32-1:0]				ir__28;
wire	[32-1:0]				ir__29;
wire	[32-1:0]				ir__30;
wire	[32-1:0]				ir__31;
wire	[32-1:0]				ir__32;
wire	[32-1:0]				ir__33;
wire	[32-1:0]				ir__34;
wire	[32-1:0]				ir__35;
wire	[32-1:0]				ir__36;
wire	[32-1:0]				ir__37;


//wire	[32-1:0]			iz	[37:0];


wire	[32-1:0]				iz__0;
wire	[32-1:0]				iz__1;
wire	[32-1:0]				iz__2;
wire	[32-1:0]				iz__3;
wire	[32-1:0]				iz__4;
wire	[32-1:0]				iz__5;
wire	[32-1:0]				iz__6;
wire	[32-1:0]				iz__7;
wire	[32-1:0]				iz__8;
wire	[32-1:0]				iz__9;
wire	[32-1:0]				iz__10;
wire	[32-1:0]				iz__11;
wire	[32-1:0]				iz__12;
wire	[32-1:0]				iz__13;
wire	[32-1:0]				iz__14;
wire	[32-1:0]				iz__15;
wire	[32-1:0]				iz__16;
wire	[32-1:0]				iz__17;
wire	[32-1:0]				iz__18;
wire	[32-1:0]				iz__19;
wire	[32-1:0]				iz__20;
wire	[32-1:0]				iz__21;
wire	[32-1:0]				iz__22;
wire	[32-1:0]				iz__23;
wire	[32-1:0]				iz__24;
wire	[32-1:0]				iz__25;
wire	[32-1:0]				iz__26;
wire	[32-1:0]				iz__27;
wire	[32-1:0]				iz__28;
wire	[32-1:0]				iz__29;
wire	[32-1:0]				iz__30;
wire	[32-1:0]				iz__31;
wire	[32-1:0]				iz__32;
wire	[32-1:0]				iz__33;
wire	[32-1:0]				iz__34;
wire	[32-1:0]				iz__35;
wire	[32-1:0]				iz__36;
wire	[32-1:0]				iz__37;


//wire	[32-1:0]			dwa	[37:0];


wire	[32-1:0]				dwa__0;
wire	[32-1:0]				dwa__1;
wire	[32-1:0]				dwa__2;
wire	[32-1:0]				dwa__3;
wire	[32-1:0]				dwa__4;
wire	[32-1:0]				dwa__5;
wire	[32-1:0]				dwa__6;
wire	[32-1:0]				dwa__7;
wire	[32-1:0]				dwa__8;
wire	[32-1:0]				dwa__9;
wire	[32-1:0]				dwa__10;
wire	[32-1:0]				dwa__11;
wire	[32-1:0]				dwa__12;
wire	[32-1:0]				dwa__13;
wire	[32-1:0]				dwa__14;
wire	[32-1:0]				dwa__15;
wire	[32-1:0]				dwa__16;
wire	[32-1:0]				dwa__17;
wire	[32-1:0]				dwa__18;
wire	[32-1:0]				dwa__19;
wire	[32-1:0]				dwa__20;
wire	[32-1:0]				dwa__21;
wire	[32-1:0]				dwa__22;
wire	[32-1:0]				dwa__23;
wire	[32-1:0]				dwa__24;
wire	[32-1:0]				dwa__25;
wire	[32-1:0]				dwa__26;
wire	[32-1:0]				dwa__27;
wire	[32-1:0]				dwa__28;
wire	[32-1:0]				dwa__29;
wire	[32-1:0]				dwa__30;
wire	[32-1:0]				dwa__31;
wire	[32-1:0]				dwa__32;
wire	[32-1:0]				dwa__33;
wire	[32-1:0]				dwa__34;
wire	[32-1:0]				dwa__35;
wire	[32-1:0]				dwa__36;
wire	[32-1:0]				dwa__37;


//ASSIGNMENTS FROM INPUTS TO PIPE
assign ir__0 = 32'b0;
assign iz__0 = 32'b0;
assign dwa__0 = 32'b0;

//GENERATE PIPELINE
//generate
//	for(i=`PIPE_DEPTH; i>0; i=i-1) begin: IrIzDwaPipe
//		case(i)
//		    
//		//NOTE: STAGE 14 --> REGISTER 14 on diagram !!   ir, iz 
//		15:   
//		begin
//
//		PhotonBlock1 photon(
//			//Inputs
//			.clock(clock),
//			.reset(reset),
//			.enable(enable),
//			
//			.i_x(ir_temp),
//			.i_y(iz_temp),
//			.i_z(dwa[14]),
//			
//			//Outputs			
//			.o_x(ir[15]),
//			.o_y(iz[15]),
//			.o_z(dwa[15])
//		);		
//		
//		end    
//		
//		//NOTE: STAGE 4 --> REGISTER 4 on diagram !!   dwa  
//		5:   
//		begin
//		    
//		PhotonBlock1 photon(
//			//Inputs
//			.clock(clock),
//			.reset(reset),
//			.enable(enable),
//			
//			.i_x(ir[4]),
//			.i_y(iz[4]),
//			.i_z(dwa_temp),
//			
//			//Outputs			
//			.o_x(ir[5]),
//			.o_y(iz[5]),
//			.o_z(dwa[5])
//		);		    
//		
//		end    
//				
//		default:
//		begin
//		    	    
//		PhotonBlock1 photon(
//			//Inputs
//			.clock(clock),
//			.reset(reset),
//			.enable(enable),
//			
//			.i_x(ir[i-1]),
//			.i_y(iz[i-1]),
//			.i_z(dwa[i-1]),
//			
//			//Outputs			
//			.o_x(ir[i]),
//			.o_y(iz[i]),
//			.o_z(dwa[i])
//		);
//		end
//		endcase
//	end
//endgenerate	

//Expanded generation


//special cases first peter m

	

		PhotonBlock1 photon15q(
			//Inputs
			.clock(clock),
			.reset(reset),
			.enable(enable),
			
			.i_x(ir_temp),
			.i_y(iz_temp),
			.i_z(dwa__14),
			
			//Outputs			
			.o_x(ir__15),
			.o_y(iz__15),
			.o_z(dwa__15)
		);		
		

		
		//NOTE: STAGE 4 --> REGISTER 4 on diagram !!   dwa  

		    
		PhotonBlock1 photon5q(
			//Inputs
			.clock(clock),
			.reset(reset),
			.enable(enable),
			
			.i_x(ir__4),
			.i_y(iz__4),
			.i_z(dwa_temp),
			
			//Outputs			
			.o_x(ir__5),
			.o_y(iz__5),
			.o_z(dwa__5)
		);		

	PhotonBlock1 photon37q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__36),
.i_y(iz__36),
.i_z(dwa__36),
//Outputs		
	.o_x(ir__37),
.o_y(iz__37),
.o_z(dwa__37)
);
PhotonBlock1 photon36q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__35),
.i_y(iz__35),
.i_z(dwa__35),
//Outputs		
	.o_x(ir__36),
.o_y(iz__36),
.o_z(dwa__36)
);
PhotonBlock1 photon35q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__34),
.i_y(iz__34),
.i_z(dwa__34),
//Outputs		
	.o_x(ir__35),
.o_y(iz__35),
.o_z(dwa__35)
);
PhotonBlock1 photon34q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__33),
.i_y(iz__33),
.i_z(dwa__33),
//Outputs		
	.o_x(ir__34),
.o_y(iz__34),
.o_z(dwa__34)
);
PhotonBlock1 photon33q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__32),
.i_y(iz__32),
.i_z(dwa__32),
//Outputs		
	.o_x(ir__33),
.o_y(iz__33),
.o_z(dwa__33)
);
PhotonBlock1 photon32q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__31),
.i_y(iz__31),
.i_z(dwa__31),
//Outputs		
	.o_x(ir__32),
.o_y(iz__32),
.o_z(dwa__32)
);
PhotonBlock1 photon31q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__30),
.i_y(iz__30),
.i_z(dwa__30),
//Outputs		
	.o_x(ir__31),
.o_y(iz__31),
.o_z(dwa__31)
);
PhotonBlock1 photon30q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__29),
.i_y(iz__29),
.i_z(dwa__29),
//Outputs		
	.o_x(ir__30),
.o_y(iz__30),
.o_z(dwa__30)
);
PhotonBlock1 photon29q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__28),
.i_y(iz__28),
.i_z(dwa__28),
//Outputs		
	.o_x(ir__29),
.o_y(iz__29),
.o_z(dwa__29)
);
PhotonBlock1 photon28q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__27),
.i_y(iz__27),
.i_z(dwa__27),
//Outputs		
	.o_x(ir__28),
.o_y(iz__28),
.o_z(dwa__28)
);
PhotonBlock1 photon27q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__26),
.i_y(iz__26),
.i_z(dwa__26),
//Outputs		
	.o_x(ir__27),
.o_y(iz__27),
.o_z(dwa__27)
);
PhotonBlock1 photon26q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__25),
.i_y(iz__25),
.i_z(dwa__25),
//Outputs		
	.o_x(ir__26),
.o_y(iz__26),
.o_z(dwa__26)
);
PhotonBlock1 photon25q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__24),
.i_y(iz__24),
.i_z(dwa__24),
//Outputs		
	.o_x(ir__25),
.o_y(iz__25),
.o_z(dwa__25)
);
PhotonBlock1 photon24q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__23),
.i_y(iz__23),
.i_z(dwa__23),
//Outputs		
	.o_x(ir__24),
.o_y(iz__24),
.o_z(dwa__24)
);
PhotonBlock1 photon23q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__22),
.i_y(iz__22),
.i_z(dwa__22),
//Outputs		
	.o_x(ir__23),
.o_y(iz__23),
.o_z(dwa__23)
);
PhotonBlock1 photon22q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__21),
.i_y(iz__21),
.i_z(dwa__21),
//Outputs		
	.o_x(ir__22),
.o_y(iz__22),
.o_z(dwa__22)
);
PhotonBlock1 photon21q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__20),
.i_y(iz__20),
.i_z(dwa__20),
//Outputs		
	.o_x(ir__21),
.o_y(iz__21),
.o_z(dwa__21)
);
PhotonBlock1 photon20q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__19),
.i_y(iz__19),
.i_z(dwa__19),
//Outputs		
	.o_x(ir__20),
.o_y(iz__20),
.o_z(dwa__20)
);
PhotonBlock1 photon19q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__18),
.i_y(iz__18),
.i_z(dwa__18),
//Outputs		
	.o_x(ir__19),
.o_y(iz__19),
.o_z(dwa__19)
);
PhotonBlock1 photon18q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__17),
.i_y(iz__17),
.i_z(dwa__17),
//Outputs		
	.o_x(ir__18),
.o_y(iz__18),
.o_z(dwa__18)
);
PhotonBlock1 photon17q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__16),
.i_y(iz__16),
.i_z(dwa__16),
//Outputs		
	.o_x(ir__17),
.o_y(iz__17),
.o_z(dwa__17)
);
PhotonBlock1 photon16q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__15),
.i_y(iz__15),
.i_z(dwa__15),
//Outputs		
	.o_x(ir__16),
.o_y(iz__16),
.o_z(dwa__16)
);




PhotonBlock1 photon14q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__13),
.i_y(iz__13),
.i_z(dwa__13),
//Outputs		
	.o_x(ir__14),
.o_y(iz__14),
.o_z(dwa__14)
);
PhotonBlock1 photon13q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__12),
.i_y(iz__12),
.i_z(dwa__12),
//Outputs		
	.o_x(ir__13),
.o_y(iz__13),
.o_z(dwa__13)
);
PhotonBlock1 photon12q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__11),
.i_y(iz__11),
.i_z(dwa__11),
//Outputs		
	.o_x(ir__12),
.o_y(iz__12),
.o_z(dwa__12)
);
PhotonBlock1 photon11q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__10),
.i_y(iz__10),
.i_z(dwa__10),
//Outputs		
	.o_x(ir__11),
.o_y(iz__11),
.o_z(dwa__11)
);
PhotonBlock1 photon10q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__9),
.i_y(iz__9),
.i_z(dwa__9),
//Outputs		
	.o_x(ir__10),
.o_y(iz__10),
.o_z(dwa__10)
);
PhotonBlock1 photon9q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__8),
.i_y(iz__8),
.i_z(dwa__8),
//Outputs		
	.o_x(ir__9),
.o_y(iz__9),
.o_z(dwa__9)
);
PhotonBlock1 photon8q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__7),
.i_y(iz__7),
.i_z(dwa__7),
//Outputs		
	.o_x(ir__8),
.o_y(iz__8),
.o_z(dwa__8)
);
PhotonBlock1 photon7q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__6),
.i_y(iz__6),
.i_z(dwa__6),
//Outputs		
	.o_x(ir__7),
.o_y(iz__7),
.o_z(dwa__7)
);
PhotonBlock1 photon6q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__5),
.i_y(iz__5),
.i_z(dwa__5),
//Outputs		
	.o_x(ir__6),
.o_y(iz__6),
.o_z(dwa__6)
);



PhotonBlock1 photon4q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__3),
.i_y(iz__3),
.i_z(dwa__3),
//Outputs		
	.o_x(ir__4),
.o_y(iz__4),
.o_z(dwa__4)
);
PhotonBlock1 photon3q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__2),
.i_y(iz__2),
.i_z(dwa__2),
//Outputs		
	.o_x(ir__3),
.o_y(iz__3),
.o_z(dwa__3)
);
PhotonBlock1 photon2q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__1),
.i_y(iz__1),
.i_z(dwa__1),
//Outputs		
	.o_x(ir__2),
.o_y(iz__2),
.o_z(dwa__2)
);
PhotonBlock1 photon1q(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_x(ir__0),
.i_y(iz__0),
.i_z(dwa__0),
//Outputs		
	.o_x(ir__1),
.o_y(iz__1),
.o_z(dwa__1)
);
	


//////////////////////////////////////////////////////////////////////////////
//STAGE BY STAGE PIPELINE DESIGN
//////////////////////////////////////////////////////////////////////////////

///////////////STAGE 2 - square of x and y/////////////////////////
always @(reset or x_pipe or y_pipe) begin
	if (reset)	begin      
		x2_temp=0;      
		y2_temp=0;
	end
	else	begin
	   x2_temp=x_pipe*x_pipe;     
	   y2_temp=y_pipe*y_pipe;
	end 
end

///////////////STAGE 3 - square of r/////////////////////////
always @(reset or x2_P or y2_P) begin
	if (reset)
		r2_temp=0; 
	else 
		r2_temp=x2_P+y2_P; 
end

///////////////STAGE 4 - Find r and dwa/////////////////////////
//Create MUX
always@(layer_pipe or muaFraction1 or muaFraction2 or muaFraction3 or muaFraction4 or muaFraction5)  
   case(layer_pipe) 
       1: fractionScaled=muaFraction1; 
       2: fractionScaled=muaFraction2; 
       3: fractionScaled=muaFraction3; 
       4: fractionScaled=muaFraction4; 
       5: fractionScaled=muaFraction5; 
       default: fractionScaled=0; //Sys Reset case
   endcase


always @(reset or weight__4 or r_P_wire or weight_P4 or fractionScaled or product64bit or dead__4 or hit__4) begin
	if (reset) begin
	   weight_P4=0; 
		r_P=0;  
      product64bit=0; 
      dwa_temp=0; 
   end
	else begin
	   weight_P4=weight__4;    
		r_P=r_P_wire;  //Connect to sqrt block
      product64bit=weight_P4*fractionScaled; 
  
      //Checking corner cases
      if (dead__4==1)       //Dead photon
         dwa_temp=weight_P4;//drop all its weight
      else if (hit__4==1)   //Hit Boundary 
         dwa_temp=0;        //Don't add to absorption array
      else
         dwa_temp=product64bit[63:32]; 	  
	end	
end

assign r2_P_wire=r2_P; 

Sqrt_64b	squareRoot (
				.clk(clock),
				.num_(r2_P_wire),
				.res(r_P_wire)
			);
			
///////////////STAGE 14 - Find ir and iz/////////////////////////
always @(reset or r_P or z_pipe or dead__14 or hit__14 or iz_temp or ir_temp) begin
	if (reset) begin
		ir_temp=0; 
		iz_temp=0;
	end	

	//Checking corner cases!!!
	else begin
		//ir_temp=r_P>>`RGRID_SCLAE_EXP; 
		//iz_temp=z_pipe>>`ZGRID_SCLAE_EXP;
		if (dead__14==1) begin 
			ir_temp=`NR-1;    
			iz_temp=`NZ-1; 
		end
		else if (hit__14==1) begin 
			ir_temp=0;
			iz_temp=0; 
		end 
		else begin
			if ((z_pipe>>`ZGRID_SCLAE_EXP) >=`NZ ) begin
				iz_temp=`NZ-1;
			end 
			else begin
				iz_temp=z_pipe>>`ZGRID_SCLAE_EXP;
			end
			
			if ((r_P>>`RGRID_SCLAE_EXP) >= `NR ) begin
				ir_temp=`NR-1;
			end
			else begin
				ir_temp=r_P>>`RGRID_SCLAE_EXP; 
			end
		end

//		if (iz_temp>=`NZ) begin
//			iz_temp=`NZ-1;   
//		end
//
//  
//		if (ir_temp>=`NR) begin
//			ir_temp=`NR-1; 
//		end

         
	end
end

///////////////STAGE 15 - Compute MEM address/////////////////////////
always @(reset or ir__15 or iz__15 or ir_P or iz_P or ir_scaled) begin
	if (reset) begin
	   ir_P=0; 
	   iz_P=0; 
	   ir_scaled=0; 
      rADDR_temp=0; 
   end
	else begin
	   ir_P=ir__15; 
	   iz_P=iz__15; 
	   ir_scaled=ir_P<<`NR_EXP;  
      rADDR_temp=ir_scaled[15:0] + iz_P[15:0]; 		
   end
end

///////////////STAGE 16 - MEM read/////////////////////////
always @(reset or ir__16 or ir__17 or iz__16 or iz__17 or ir__18 or iz__18 or newAbs_P or q or newAbs_temp) begin
	if (reset) begin
		oldAbs_MEM=0; 
	end else begin
	   //Check Corner cases (RAW hazards) 
      if ((ir__16==ir__17) && (iz__16==iz__17)) begin
         oldAbs_MEM=newAbs_temp; 
      end else if ((ir__16==ir__18) && (iz__16==iz__18)) begin   
         oldAbs_MEM=newAbs_P;       //RAW hazard
      end else begin
         oldAbs_MEM=q;   //Connect to REAL dual-port MEM 
		end
	end
	
end

///////////////STAGE 17 - Update Weight/////////////////////////
//TO BE TESTED!!!
always @(reset or dwa__17 or weight__17 or weight_P or dwa_P or oldAbs_P) begin
	if(reset) begin
	   dwa_P=0;   //How to specify Base 10??? 
		weight_P=0; 
		newWeight = 0;
		newAbs_temp =0; 
   end
	else begin
	   dwa_P=dwa__17;
	   weight_P=weight__17; 
		newWeight=weight_P-dwa_P; 
		newAbs_temp=oldAbs_P+dwa_P;   //Check bit width casting (64-bit<--64-bit+32-bit)
   end 
end    
		
//////////////////////////////////////////////////////////////////////////////
//STAGE BY STAGE - EXTRA REGISTERS
//////////////////////////////////////////////////////////////////////////////   
always @ (posedge clock) 
begin
	if (reset) begin	    
	  //Stage 2
	  x2_P<=0;         
	  y2_P<=0;
	  
	  //Stage 3
	  r2_P<=0;	  
	  
	  //Stage 15
     rADDR_16<=0; 

	  //Stage 16 
	  oldAbs_P<=0; 
	  rADDR_17<=0; 
	  
	  //Stage 17
	  newAbs_P<=0; 
	 // wADDR <=0; 
	end
	
	else if (enable) begin	    
	  //Stage 2
	  x2_P<=x2_temp;    //From comb logic above
	  y2_P<=y2_temp;    
      
 	  //Stage 3
 	  r2_P<=r2_temp;   

	  //Stage 15
     rADDR_16<=rADDR_temp; 
     
     //Stage 16 
	  oldAbs_P<=oldAbs_MEM; 
	  rADDR_17<=rADDR_16; 
	  	     
     //Stage 17
     newAbs_P<=newAbs_temp; 
   //  wADDR <=rADDR_17; 
	end
end

//////////////////////////////////////////////////////////////////////////////
//INTERFACE to on-chip MEM
//////////////////////////////////////////////////////////////////////////////   
always @ (posedge clock) 
begin
	if (reset) 
		  wren <=0; 
	else
		  wren<=1;          //Memory enabled every cycle after global enable 
end	
	    
assign rdaddress=rADDR_temp; 
assign wraddress=rADDR_17; 

assign data=newAbs_temp; 

endmodule


//Photons that make up the register pipeline
module PhotonBlock1(
	//Inputs
	clock,
	reset,
	enable,
	
   i_x, 
   i_y, 
   i_z, 

	//Outputs
	o_x,
	o_y,
	o_z
	);

//////////////////////////////////////////////////////////////////////////////
//PARAMETERS
//////////////////////////////////////////////////////////////////////////////
//parameter BIT_WIDTH=32;

input				clock;
input				reset;
input				enable;

input	[`BIT_WIDTH-1:0]			i_x;
input	[`BIT_WIDTH-1:0]			i_y;
input	[`BIT_WIDTH-1:0]			i_z;


output	[`BIT_WIDTH-1:0]			o_x;
output	[`BIT_WIDTH-1:0]			o_y;
output	[`BIT_WIDTH-1:0]			o_z;

wire				clock;
wire				reset;
wire				enable;

wire	[`BIT_WIDTH-1:0]			i_x;
wire	[`BIT_WIDTH-1:0]			i_y;
wire	[`BIT_WIDTH-1:0]			i_z;

reg	[`BIT_WIDTH-1:0]			o_x;
reg	[`BIT_WIDTH-1:0]			o_y;
reg	[`BIT_WIDTH-1:0]			o_z;

always @ (posedge clock)
	if(reset) begin
		o_x		<=	{`BIT_WIDTH{1'b0}} ;
		o_y		<=	{`BIT_WIDTH{1'b0}};
		o_z		<=	{`BIT_WIDTH{1'b0}};
	end else if(enable) begin
		o_x		<=	i_x;
		o_y		<=	i_y;
		o_z		<=	i_z;
	end
endmodule


//Photons that make up the register pipeline
module PhotonBlock2(
	//Inputs
	clock,
	reset,
	enable,
	
   i_x, 
   i_y, 
   i_z, 

	//Outputs
	o_x,
	o_y,
	o_z
	);

//////////////////////////////////////////////////////////////////////////////
//PARAMETERS
//////////////////////////////////////////////////////////////////////////////
//parameter BIT_WIDTH=32;

input				clock;
input				reset;
input				enable;

input	[`BIT_WIDTH-1:0]			i_x;
input	i_y;
input	i_z;


output	[`BIT_WIDTH-1:0]			o_x;
output	o_y;
output	o_z;

wire				clock;
wire				reset;
wire				enable;

wire	[`BIT_WIDTH-1:0]			i_x;
wire	i_y;
wire	i_z;

reg	[`BIT_WIDTH-1:0]			o_x;
reg	o_y;
reg	o_z;

always @ (posedge clock)
	if(reset) begin
		o_x		<=	{`BIT_WIDTH{1'b0}} ;
		o_y		<=	1'b0;
		o_z		<=	1'b0;
	end else if(enable) begin
		o_x		<=	i_x;
		o_y		<=	i_y;
		o_z		<=	i_z;
	end
endmodule








module ScattererReflectorWrapper (
	//Inputs
	clock,
	reset,
	enable,
	//MEMORY WRAPPER

		//Inputs
		
		//Photon values
		i_uz1_pipeWrapper,
		i_hit2_pipeWrapper,
		i_ux3_pipeWrapper,
		i_uz3_pipeWrapper,
		i_layer3_pipeWrapper,
		i_hit4_pipeWrapper,
		i_hit6_pipeWrapper,
		i_hit16_pipeWrapper,
		i_layer31_pipeWrapper,
		i_uy32_pipeWrapper,
		i_uz32_pipeWrapper,
		i_hit33_pipeWrapper,
		i_ux33_pipeWrapper,
		i_uy33_pipeWrapper,
		i_hit34_pipeWrapper,
		i_ux35_pipeWrapper,
		i_uy35_pipeWrapper,
		i_uz35_pipeWrapper,
		i_layer35_pipeWrapper,
		i_hit36_pipeWrapper,
		i_ux36_pipeWrapper,
		i_uy36_pipeWrapper,
		i_uz36_pipeWrapper,
		i_layer36_pipeWrapper,
		i_dead36_pipeWrapper,


		//Memory Interface
			//Inputs
		rand2,
		rand3,
		rand5,
		sint,
		cost,
		up_rFresnel,
		down_rFresnel,
			//Outputs
		tindex,
		fresIndex,
		
		//Constants
		down_niOverNt_1,
		down_niOverNt_2,
		down_niOverNt_3,
		down_niOverNt_4,
		down_niOverNt_5,
		up_niOverNt_1,
		up_niOverNt_2,
		up_niOverNt_3,
		up_niOverNt_4,
		up_niOverNt_5,
		down_niOverNt_2_1,
		down_niOverNt_2_2,
		down_niOverNt_2_3,
		down_niOverNt_2_4,
		down_niOverNt_2_5,
		up_niOverNt_2_1,
		up_niOverNt_2_2,
		up_niOverNt_2_3,
		up_niOverNt_2_4,
		up_niOverNt_2_5,
		downCritAngle_0,
		downCritAngle_1,
		downCritAngle_2,
		downCritAngle_3,
		downCritAngle_4,
		upCritAngle_0,
		upCritAngle_1,
		upCritAngle_2,
		upCritAngle_3,
		upCritAngle_4,
		
		//Outputs
		ux_scatterer,
		uy_scatterer,
		uz_scatterer,
		
		ux_reflector,
		uy_reflector,
		uz_reflector,
		layer_reflector,
		dead_reflector
	);
	
//-------------------PARAMETER DEFINITION----------------------
//
//
//
//
//
//
//Assign values to parameters used later in the program.
	
//parameter INTMAX_2 = 64'h3FFFFFFF00000001;
//The above parameter is never used in the ScattererReflectorWrapper module itself
	
	
//-----------------------------PIN DECLARATION----------------------
//
//
//
//
//
//
//
//
//Assign appropriate types to pins (input or output).


input				clock;
input				reset;
input				enable;
input	[2:0]			i_layer31_pipeWrapper;



input	[31:0]			i_uz1_pipeWrapper;
input				i_hit2_pipeWrapper;
input	[31:0]			i_ux3_pipeWrapper;
input	[31:0]			i_uz3_pipeWrapper;
input	[2:0]			i_layer3_pipeWrapper;
input				i_hit4_pipeWrapper;
input				i_hit6_pipeWrapper;
input				i_hit16_pipeWrapper;
input	[31:0]			i_uy32_pipeWrapper;
input	[31:0]			i_uz32_pipeWrapper;
input				i_hit33_pipeWrapper;
input	[31:0]			i_ux33_pipeWrapper;
input	[31:0]			i_uy33_pipeWrapper;
input				i_hit34_pipeWrapper;
input	[31:0]			i_ux35_pipeWrapper;
input	[31:0]			i_uy35_pipeWrapper;
input	[31:0]			i_uz35_pipeWrapper;
input	[2:0]			i_layer35_pipeWrapper;
input				i_hit36_pipeWrapper;
input	[31:0]			i_ux36_pipeWrapper;
input	[31:0]			i_uy36_pipeWrapper;
input	[31:0]			i_uz36_pipeWrapper;
input	[2:0]			i_layer36_pipeWrapper;
input				i_dead36_pipeWrapper;


//Memory Interface
input	[31:0]			rand2;
input	[31:0]			rand3;
input	[31:0]			rand5;
input	[31:0]			sint;
input	[31:0]			cost;
input	[31:0]			up_rFresnel;
input	[31:0]			down_rFresnel;

output	[12:0]			tindex;
output	[9:0]			fresIndex;


//Constants
input	[31:0]			down_niOverNt_1;
input	[31:0]			down_niOverNt_2;
input	[31:0]			down_niOverNt_3;
input	[31:0]			down_niOverNt_4;
input	[31:0]			down_niOverNt_5;
input	[31:0]			up_niOverNt_1;
input	[31:0]			up_niOverNt_2;
input	[31:0]			up_niOverNt_3;
input	[31:0]			up_niOverNt_4;
input	[31:0]			up_niOverNt_5;
input	[63:0]			down_niOverNt_2_1;
input	[63:0]			down_niOverNt_2_2;
input	[63:0]			down_niOverNt_2_3;
input	[63:0]			down_niOverNt_2_4;
input	[63:0]			down_niOverNt_2_5;
input	[63:0]			up_niOverNt_2_1;
input	[63:0]			up_niOverNt_2_2;
input	[63:0]			up_niOverNt_2_3;
input	[63:0]			up_niOverNt_2_4;
input	[63:0]			up_niOverNt_2_5;
input	[31:0]			downCritAngle_0;
input	[31:0]			downCritAngle_1;
input	[31:0]			downCritAngle_2;
input	[31:0]			downCritAngle_3;
input	[31:0]			downCritAngle_4;
input	[31:0]			upCritAngle_0;
input	[31:0]			upCritAngle_1;
input	[31:0]			upCritAngle_2;
input	[31:0]			upCritAngle_3;
input	[31:0]			upCritAngle_4;


output	[31:0]			ux_scatterer;
output	[31:0]			uy_scatterer;
output	[31:0]			uz_scatterer;
output	[31:0]			ux_reflector;
output	[31:0]			uy_reflector;
output	[31:0]			uz_reflector;
output	[2:0]			layer_reflector;
output				dead_reflector;




//-----------------------------PIN TYPES-----------------------------
//
//
//
//
//
//
//
//
//Assign pins to be wires or regs.
wire				clock;
wire				reset;
wire				enable;

wire	[2:0]			i_layer31_pipeWrapper;

wire	[31:0]			i_uz1_pipeWrapper;
wire				i_hit2_pipeWrapper;
wire	[31:0]			i_ux3_pipeWrapper;
wire	[31:0]			i_uz3_pipeWrapper;
wire	[2:0]			i_layer3_pipeWrapper;
wire				i_hit4_pipeWrapper;
wire				i_hit6_pipeWrapper;
wire				i_hit16_pipeWrapper;
wire	[31:0]			i_uy32_pipeWrapper;
wire	[31:0]			i_uz32_pipeWrapper;
wire				i_hit33_pipeWrapper;
wire	[31:0]			i_ux33_pipeWrapper;
wire	[31:0]			i_uy33_pipeWrapper;
wire				i_hit34_pipeWrapper;
wire	[31:0]			i_ux35_pipeWrapper;
wire	[31:0]			i_uy35_pipeWrapper;
wire	[31:0]			i_uz35_pipeWrapper;
wire	[2:0]			i_layer35_pipeWrapper;
wire				i_hit36_pipeWrapper;
wire	[31:0]			i_ux36_pipeWrapper;
wire	[31:0]			i_uy36_pipeWrapper;
wire	[31:0]			i_uz36_pipeWrapper;
wire	[2:0]			i_layer36_pipeWrapper;
wire				i_dead36_pipeWrapper;

wire	[9:0]			pindex;
wire	[12:0]			tindex;
wire	[31:0]			rand2;
wire	[31:0]			rand3;
wire	[31:0]			rand5;


//Constants
wire	[31:0]			down_niOverNt_1;
wire	[31:0]			down_niOverNt_2;
wire	[31:0]			down_niOverNt_3;
wire	[31:0]			down_niOverNt_4;
wire	[31:0]			down_niOverNt_5;
wire	[31:0]			up_niOverNt_1;
wire	[31:0]			up_niOverNt_2;
wire	[31:0]			up_niOverNt_3;
wire	[31:0]			up_niOverNt_4;
wire	[31:0]			up_niOverNt_5;
wire	[63:0]			down_niOverNt_2_1;
wire	[63:0]			down_niOverNt_2_2;
wire	[63:0]			down_niOverNt_2_3;
wire	[63:0]			down_niOverNt_2_4;
wire	[63:0]			down_niOverNt_2_5;
wire	[63:0]			up_niOverNt_2_1;
wire	[63:0]			up_niOverNt_2_2;
wire	[63:0]			up_niOverNt_2_3;
wire	[63:0]			up_niOverNt_2_4;
wire	[63:0]			up_niOverNt_2_5;
wire	[31:0]			downCritAngle_0;
wire	[31:0]			downCritAngle_1;
wire	[31:0]			downCritAngle_2;
wire	[31:0]			downCritAngle_3;
wire	[31:0]			downCritAngle_4;
wire	[31:0]			upCritAngle_0;
wire	[31:0]			upCritAngle_1;
wire	[31:0]			upCritAngle_2;
wire	[31:0]			upCritAngle_3;
wire	[31:0]			upCritAngle_4;

//Scatterer, final calculated values
wire	[31:0]			ux_scatterer;
wire	[31:0]			uy_scatterer;
wire	[31:0]			uz_scatterer;
wire	[31:0]			ux_reflector;
wire	[31:0]			uy_reflector;
wire	[31:0]			uz_reflector;
wire	[2:0]			layer_reflector;
wire				dead_reflector;


//Mathematics results signals
wire	[63:0]			prod1_2;
wire	[63:0]			prod1_4;
wire	[31:0]			sqrtResult1_6;
//wire	[32:0]			sqrtRemainder; //not necessary, not used except as dummy var in sqrt.
wire	[63:0]			prod1_33;
wire	[63:0]			prod2_33;
wire	[63:0]			prod3_33;
wire	[63:0]			prod1_34;
wire	[63:0]			prod2_34;
wire	[63:0]			prod3_34;
wire	[63:0]			prod4_34;
wire	[63:0]			quot1_16;
wire	[31:0]			divRemainder;
wire	[63:0]			prod1_36;
wire	[63:0]			prod2_36;
wire	[63:0]			prod3_36;
wire	[63:0]			prod4_36;
wire	[63:0]			prod5_36;
wire	[63:0]			prod6_36;

//Scatterer Operands
wire	[31:0]			op1_2_1_scatterer;
wire	[31:0]			op1_2_2_scatterer;
wire	[31:0]			op1_4_1_scatterer;
wire	[31:0]			op1_4_2_scatterer;
wire	[63:0]			sqrtOperand1_6_scatterer;
wire	[63:0]			divNumerator1_16_scatterer;
wire	[31:0]			divDenominator1_16_scatterer;
wire	[31:0]			op1_33_1_scatterer;
wire	[31:0]			op1_33_2_scatterer;
wire	[31:0]			op2_33_1_scatterer;
wire	[31:0]			op2_33_2_scatterer;
wire	[31:0]			op3_33_1_scatterer;
wire	[31:0]			op3_33_2_scatterer;
wire	[31:0]			op1_34_1_scatterer;
wire	[31:0]			op1_34_2_scatterer;
wire	[31:0]			op2_34_1_scatterer;
wire	[31:0]			op2_34_2_scatterer;
wire	[31:0]			op3_34_1_scatterer;
wire	[31:0]			op3_34_2_scatterer;
wire	[31:0]			op4_34_1_scatterer;
wire	[31:0]			op4_34_2_scatterer;
wire	[31:0]			op1_36_1_scatterer;
wire	[31:0]			op1_36_2_scatterer;
wire	[31:0]			op2_36_1_scatterer;
wire	[31:0]			op2_36_2_scatterer;
wire	[31:0]			op3_36_1_scatterer;
wire	[31:0]			op3_36_2_scatterer;
wire	[31:0]			op4_36_1_scatterer;
wire	[31:0]			op4_36_2_scatterer;
wire	[31:0]			op5_36_1_scatterer;
wire	[31:0]			op5_36_2_scatterer;
wire	[31:0]			op6_36_1_scatterer;
wire	[31:0]			op6_36_2_scatterer;


//Reflector Operands
wire	[31:0]			op1_2_1_reflector;
wire	[31:0]			op1_2_2_reflector;
wire	[31:0]			op1_4_1_reflector;
wire	[31:0]			op1_4_2_reflector;
wire	[63:0]			sqrtOperand1_6_reflector;
wire	[31:0]			op1_36_1_reflector;
wire	[31:0]			op1_36_2_reflector;
wire	[31:0]			op2_36_1_reflector;
wire	[31:0]			op2_36_2_reflector;




//Operands entering the multipliers, divider, and sqrt
wire	[31:0]			op1_2_1;
wire	[31:0]			op1_2_2;
wire	[31:0]			op1_4_1;
wire	[31:0]			op1_4_2;
wire	[63:0]			sqrtOperand1_6;
wire	[63:0]			divNumerator1_16;
wire	[31:0]			divDenominator1_16;
wire	[31:0]			op1_33_1;
wire	[31:0]			op1_33_2;
wire	[31:0]			op2_33_1;
wire	[31:0]			op2_33_2;
wire	[31:0]			op3_33_1;
wire	[31:0]			op3_33_2;
wire	[31:0]			op1_34_1;
wire	[31:0]			op1_34_2;
wire	[31:0]			op2_34_1;
wire	[31:0]			op2_34_2;
wire	[31:0]			op3_34_1;
wire	[31:0]			op3_34_2;
wire	[31:0]			op4_34_1;
wire	[31:0]			op4_34_2;
wire	[31:0]			op1_36_1;
wire	[31:0]			op1_36_2;
wire	[31:0]			op2_36_1;
wire	[31:0]			op2_36_2;
wire	[31:0]			op3_36_1;
wire	[31:0]			op3_36_2;
wire	[31:0]			op4_36_1;
wire	[31:0]			op4_36_2;
wire	[31:0]			op5_36_1;
wire	[31:0]			op5_36_2;
wire	[31:0]			op6_36_1;
wire	[31:0]			op6_36_2;


reg	[2:0]			layerMinusOne;

wire	[31:0]			sint;
wire	[31:0]			cost;
wire	[31:0]			sinp;
wire	[31:0]			cosp;

wire	[31:0]			up_rFresnel;
wire	[31:0]			down_rFresnel;
wire	[9:0]			fresIndex;



//Need this to deal with 'unused' inputs for ODIN II
wire bigOr;
assign bigOr = i_hit16_pipeWrapper|i_hit33_pipeWrapper|i_hit34_pipeWrapper|
					rand2[31]|rand2[30]|rand2[29]|rand2[28]|rand2[27]|rand2[26]|
					rand2[25]|rand2[24]|rand2[23]|rand2[22]|rand2[21]|rand2[20]|
					rand2[19]|rand2[18]|rand2[17]|rand2[16]|rand2[15]|rand2[14]|
					rand2[13]|rand2[12]|rand2[11]|rand2[10]|
					rand3[31]|rand3[30]|rand3[29]|rand3[28]|rand3[27]|rand3[26]|
					rand3[25]|rand3[24]|rand3[23]|rand3[22]|rand3[21]|rand3[20]|
					rand3[19]|rand3[18]|rand3[17]|rand3[16]|rand3[15]|rand3[14]|
					rand3[13]|rand3[12]|rand3[11]|rand3[10]|
					rand5[31]|(1'b1);
wire reset_new;
assign reset_new = reset & bigOr;


//MUX for sending in indices for memory.
always @ (i_layer31_pipeWrapper) begin
	case (i_layer31_pipeWrapper)
		3'b001: layerMinusOne = 0;
		3'b010: layerMinusOne = 1;
		3'b011: layerMinusOne = 2;
		3'b100: layerMinusOne = 3;
		3'b101: layerMinusOne = 4;
		default: layerMinusOne = 0;
	endcase
end

assign tindex = {layerMinusOne, rand2[9:0]};
assign pindex = rand3[9:0];


//Arbitrarily decide on values of sine and cosine for now, should be memory lookups
Memory_Wrapper	memories(
					//INPUTS
					.clock(clock),
					//.reset(reset), //Unused. ODIN II complained.
					.pindex(pindex),
					//OUTPUTS
					.sinp(sinp),
					.cosp(cosp)
				);


Scatterer scatterer_0 (
			.clock(clock),
			.reset(reset_new), //so pins are used
			.enable(enable),
			//Photon values
			.i_uz1(i_uz1_pipeWrapper),
			.i_ux3(i_ux3_pipeWrapper),
			.i_uz3(i_uz3_pipeWrapper),
			.i_uy32(i_uy32_pipeWrapper),
			.i_uz32(i_uz32_pipeWrapper),
			.i_ux33(i_ux33_pipeWrapper),
			.i_uy33(i_uy33_pipeWrapper),
			.i_ux35(i_ux35_pipeWrapper),
			.i_uy35(i_uy35_pipeWrapper),
			.i_uz35(i_uz35_pipeWrapper),
			.i_uz36(i_uz36_pipeWrapper),
			//Mathematics Results
			.prod1_2(prod1_2),
			.prod1_4({prod1_4[63:63], prod1_4[61:31]}),
			.sqrtResult1_6(sqrtResult1_6),
			.prod1_33({prod1_33[63:63], prod1_33[61:31]}),
			.prod2_33({prod2_33[63:63], prod2_33[61:31]}),
			.prod3_33({prod3_33[63:63], prod3_33[61:31]}),
			.prod1_34({prod1_34[63:63], prod1_34[61:31]}),
			.prod2_34({prod2_34[63:63], prod2_34[61:31]}),
			.prod3_34({prod3_34[63:63], prod3_34[61:31]}),
			.prod4_34({prod4_34[63:63], prod4_34[61:31]}),
			.quot1_16(quot1_16[63:0]),
			.prod1_36(prod1_36[63:0]),
			.prod2_36(prod2_36[63:0]),
			.prod3_36({prod3_36[63:63], prod3_36[61:31]}),
			.prod4_36({prod4_36[63:63], prod4_36[61:31]}),
			.prod5_36({prod5_36[63:63], prod5_36[61:31]}),
			.prod6_36({prod6_36[63:63], prod6_36[61:31]}),
			//Trig from Memory
			.sint_Mem(sint),
			.cost_Mem(cost),
			.sinp_Mem(sinp),
			.cosp_Mem(cosp),
			//Operands for mathematics
			.op1_2_1(op1_2_1_scatterer),
			.op1_2_2(op1_2_2_scatterer),
			.op1_4_1(op1_4_1_scatterer),
			.op1_4_2(op1_4_2_scatterer),
			.sqrtOperand1_6(sqrtOperand1_6_scatterer),
			.divNumerator1_16(divNumerator1_16_scatterer),
			.divDenominator1_16(divDenominator1_16_scatterer),
			.op1_33_1(op1_33_1_scatterer),
			.op1_33_2(op1_33_2_scatterer),
			.op2_33_1(op2_33_1_scatterer),
			.op2_33_2(op2_33_2_scatterer),
			.op3_33_1(op3_33_1_scatterer),
			.op3_33_2(op3_33_2_scatterer),
			.op1_34_1(op1_34_1_scatterer),
			.op1_34_2(op1_34_2_scatterer),
			.op2_34_1(op2_34_1_scatterer),
			.op2_34_2(op2_34_2_scatterer),
			.op3_34_1(op3_34_1_scatterer),
			.op3_34_2(op3_34_2_scatterer),
			.op4_34_1(op4_34_1_scatterer),
			.op4_34_2(op4_34_2_scatterer),
			.op1_36_1(op1_36_1_scatterer),
			.op1_36_2(op1_36_2_scatterer),
			.op2_36_1(op2_36_1_scatterer),
			.op2_36_2(op2_36_2_scatterer),
			.op3_36_1(op3_36_1_scatterer),
			.op3_36_2(op3_36_2_scatterer),
			.op4_36_1(op4_36_1_scatterer),
			.op4_36_2(op4_36_2_scatterer),
			.op5_36_1(op5_36_1_scatterer),
			.op5_36_2(op5_36_2_scatterer),
			.op6_36_1(op6_36_1_scatterer),
			.op6_36_2(op6_36_2_scatterer),
			
			//Final calculated values
			.ux_scatterer(ux_scatterer),
			.uy_scatterer(uy_scatterer),
			.uz_scatterer(uz_scatterer)

		);
		
Reflector reflector_0 (

			//INPUTS
			.clock(clock),
			.reset(reset),
			.enable(enable),
			//Photon values
			.i_uz1(i_uz1_pipeWrapper),
			.i_uz3(i_uz3_pipeWrapper),
			.i_layer3(i_layer3_pipeWrapper),
			.i_ux35(i_ux35_pipeWrapper),
			.i_uy35(i_uy35_pipeWrapper),
			.i_uz35(i_uz35_pipeWrapper),
			.i_layer35(i_layer35_pipeWrapper),
			.i_ux36(i_ux36_pipeWrapper),
			.i_uy36(i_uy36_pipeWrapper),
			.i_uz36(i_uz36_pipeWrapper),
			.i_layer36(i_layer36_pipeWrapper),
			.i_dead36(i_dead36_pipeWrapper),

			//Constants
			.down_niOverNt_1(down_niOverNt_1),
			.down_niOverNt_2(down_niOverNt_2),
			.down_niOverNt_3(down_niOverNt_3),
			.down_niOverNt_4(down_niOverNt_4),
			.down_niOverNt_5(down_niOverNt_5),
			.up_niOverNt_1(up_niOverNt_1),
			.up_niOverNt_2(up_niOverNt_2),
			.up_niOverNt_3(up_niOverNt_3),
			.up_niOverNt_4(up_niOverNt_4),
			.up_niOverNt_5(up_niOverNt_5),
			.down_niOverNt_2_1(down_niOverNt_2_1),
			.down_niOverNt_2_2(down_niOverNt_2_2),
			.down_niOverNt_2_3(down_niOverNt_2_3),
			.down_niOverNt_2_4(down_niOverNt_2_4),
			.down_niOverNt_2_5(down_niOverNt_2_5),
			.up_niOverNt_2_1(up_niOverNt_2_1),
			.up_niOverNt_2_2(up_niOverNt_2_2),
			.up_niOverNt_2_3(up_niOverNt_2_3),
			.up_niOverNt_2_4(up_niOverNt_2_4),
			.up_niOverNt_2_5(up_niOverNt_2_5),
			.downCritAngle_0(downCritAngle_0),
			.downCritAngle_1(downCritAngle_1),
			.downCritAngle_2(downCritAngle_2),
			.downCritAngle_3(downCritAngle_3),
			.downCritAngle_4(downCritAngle_4),
			.upCritAngle_0(upCritAngle_0),
			.upCritAngle_1(upCritAngle_1),
			.upCritAngle_2(upCritAngle_2),
			.upCritAngle_3(upCritAngle_3),
			.upCritAngle_4(upCritAngle_4),

			//Fresnels inputs
			.rnd({1'b0, rand5[30:0]}),
			.up_rFresnel(up_rFresnel),
			.down_rFresnel(down_rFresnel),

			//Mathematics Results
			.prod1_2(prod1_2),
			.prod1_4(prod1_4),
			.sqrtResult1_6(sqrtResult1_6),
			.prod1_36(prod1_36),
			.prod2_36(prod2_36),

			//OUTPUTS

			//Fresnels outputs
			.fresIndex(fresIndex),

			//Mathematics Operands
			.op1_2_1(op1_2_1_reflector),
			.op1_2_2(op1_2_2_reflector),
			.op1_4_1(op1_4_1_reflector),
			.op1_4_2(op1_4_2_reflector),
			.sqrtOperand1_6(sqrtOperand1_6_reflector),
			.op1_36_1(op1_36_1_reflector),
			.op1_36_2(op1_36_2_reflector),
			.op2_36_1(op2_36_1_reflector),
			.op2_36_2(op2_36_2_reflector),


			//Final Calculated Results
			.ux_reflector(ux_reflector),
			.uy_reflector(uy_reflector),
			.uz_reflector(uz_reflector),
			.layer_reflector(layer_reflector),
			.dead_reflector(dead_reflector)

);
		



	
//Multipliers, Dividers, and Sqrts for Scatterer & Reflector

assign op1_2_1 = (i_hit2_pipeWrapper == 1'b1) ? op1_2_1_reflector		:		op1_2_1_scatterer;
assign op1_2_2 = (i_hit2_pipeWrapper == 1'b1) ? op1_2_2_reflector		:		op1_2_2_scatterer;

Mult_32b	multiplier1_2 (
				.dataa(op1_2_1),
				.datab(op1_2_2),
				.result(prod1_2)
			);
			
assign op1_4_1 = (i_hit4_pipeWrapper == 1'b1) ? op1_4_1_reflector		:		op1_4_1_scatterer;
assign op1_4_2 = (i_hit4_pipeWrapper == 1'b1) ? op1_4_2_reflector		:		op1_4_2_scatterer;

Mult_32b	multiplier1_4 (
				.dataa(op1_4_1),
				.datab(op1_4_2),
				.result(prod1_4)
			);
			


Mult_32b	multiplier1_33 (
				.dataa(op1_33_1_scatterer),
				.datab(op1_33_2_scatterer),
				.result(prod1_33)
			);

Mult_32b	multiplier2_33 (
				.dataa(op2_33_1_scatterer),
				.datab(op2_33_2_scatterer),
				.result(prod2_33)
			);

Mult_32b	multiplier3_33 (
				.dataa(op3_33_1_scatterer),
				.datab(op3_33_2_scatterer),
				.result(prod3_33)
			);


Mult_32b	multiplier1_34 (
				.dataa(op1_34_1_scatterer),
				.datab(op1_34_2_scatterer),
				.result(prod1_34)
			);


Mult_32b	multiplier2_34 (
				.dataa(op2_34_1_scatterer),
				.datab(op2_34_2_scatterer),
				.result(prod2_34)
			);


Mult_32b	multiplier3_34 (
				.dataa(op3_34_1_scatterer),
				.datab(op3_34_2_scatterer),
				.result(prod3_34)
			);

Mult_32b	multiplier4_34 (
				.dataa(op4_34_1_scatterer),
				.datab(op4_34_2_scatterer),
				.result(prod4_34)
			);

assign op1_36_1 = (i_hit36_pipeWrapper == 1'b1) ? op1_36_1_reflector	:		op1_36_1_scatterer;
assign op1_36_2 = (i_hit36_pipeWrapper == 1'b1) ? op1_36_2_reflector	:		op1_36_2_scatterer;

Mult_32b	multiplier1_36 (
				.dataa(op1_36_1),
				.datab(op1_36_2),
				.result(prod1_36)
			);

assign op2_36_1 = (i_hit36_pipeWrapper == 1'b1) ? op2_36_1_reflector	:		op2_36_1_scatterer;
assign op2_36_2 = (i_hit36_pipeWrapper == 1'b1) ? op2_36_2_reflector	:		op2_36_2_scatterer;

Mult_32b	multiplier2_36 (
				.dataa(op2_36_1),
				.datab(op2_36_2),
				.result(prod2_36)
			);
			
Mult_32b	multiplier3_36 (
				.dataa(op3_36_1_scatterer),
				.datab(op3_36_2_scatterer),
				.result(prod3_36)
			);


Mult_32b	multiplier4_36 (
				.dataa(op4_36_1_scatterer),
				.datab(op4_36_2_scatterer),
				.result(prod4_36)
			);
			

Mult_32b	multiplier5_36 (
				.dataa(op5_36_1_scatterer),
				.datab(op5_36_2_scatterer),
				.result(prod5_36)
			);


Mult_32b	multiplier6_36 (
				.dataa(op6_36_1_scatterer),
				.datab(op6_36_2_scatterer),
				.result(prod6_36)
			);
			
assign sqrtOperand1_6 = (i_hit6_pipeWrapper == 1'b1) ? sqrtOperand1_6_reflector	:		sqrtOperand1_6_scatterer;

Sqrt_64b	squareRoot1_6 (
				.clk(clock),
				.num_(sqrtOperand1_6),
				.res(sqrtResult1_6)
			);



Div_64b		divide1_16 (
				.clock(clock),
				.denom(divDenominator1_16_scatterer),
				.numer(divNumerator1_16_scatterer),
				.quotient(quot1_16),
				.remain(divRemainder)
			);
				

endmodule
			



module InternalsBlock_Reflector(
	//Inputs
	clock,
	reset,
	enable,

	i_uz_2,			//uz^2
	i_uz2,			//new uz, should the photon transmit to new layer
	i_oneMinusUz_2, 	//(1-uz)^2
	i_sa2_2,		//(sine of angle 2)^2 (uz2 = cosine of angle 2).
	i_uz2_2,		//(uz2)^2, new uz squared.
	i_ux_transmitted,	//new value for ux, if the photon transmits to the next layer
	i_uy_transmitted,	//new value for uy, if the photon transmits to the next layer
	
	//Outputs
	o_uz_2,
	o_uz2,
	o_oneMinusUz_2,
	o_sa2_2,
	o_uz2_2,
	o_ux_transmitted,
	o_uy_transmitted
	);

input					clock;
input					reset;
input					enable;

input		[63:0]		i_uz_2;
input		[31:0]		i_uz2;
input		[63:0]		i_oneMinusUz_2;
input		[63:0]		i_sa2_2;
input		[63:0]		i_uz2_2;
input		[31:0]		i_ux_transmitted;
input		[31:0]		i_uy_transmitted;

output		[63:0]		o_uz_2;
output		[31:0]		o_uz2;
output		[63:0]		o_oneMinusUz_2;
output		[63:0]		o_sa2_2;
output		[63:0]		o_uz2_2;
output		[31:0]		o_ux_transmitted;
output		[31:0]		o_uy_transmitted;


wire					clock;
wire					reset;
wire					enable;

wire		[63:0]		i_uz_2;
wire		[31:0]		i_uz2;
wire		[63:0]		i_oneMinusUz_2;
wire		[63:0]		i_sa2_2;
wire		[63:0]		i_uz2_2;
wire		[31:0]		i_ux_transmitted;
wire		[31:0]		i_uy_transmitted;


reg		[63:0]		o_uz_2;
reg		[31:0]		o_uz2;
reg		[63:0]		o_oneMinusUz_2;
reg		[63:0]		o_sa2_2;
reg		[63:0]		o_uz2_2;
reg		[31:0]		o_ux_transmitted;
reg		[31:0]		o_uy_transmitted;



always @ (posedge clock)
	if(reset) begin
		o_uz_2					<= 64'h3FFFFFFFFFFFFFFF;
		o_uz2					<= 32'h7FFFFFFF;
		o_oneMinusUz_2				<= 64'h0000000000000000;
		o_sa2_2					<= 64'h0000000000000000;
		o_uz2_2					<= 64'h3FFFFFFFFFFFFFFF;
		o_ux_transmitted			<= 32'h00000000;
		o_uy_transmitted			<= 32'h00000000;
	end else if(enable) begin
		o_uz_2					<= i_uz_2;
		o_uz2					<= i_uz2;
		o_oneMinusUz_2				<= i_oneMinusUz_2;
		o_sa2_2					<= i_sa2_2;
		o_uz2_2					<= i_uz2_2;
		o_ux_transmitted			<= i_ux_transmitted;
		o_uy_transmitted			<= i_uy_transmitted;
	end
endmodule


module Reflector (
	
	//INPUTS
	clock,
	reset,
	enable,
	//Values from Photon Pipeline
	i_uz1,
	i_uz3,
	i_layer3,
	i_ux35,
	i_uy35,
	i_uz35,
	i_layer35,
	i_ux36,
	i_uy36,
	i_uz36,
	i_layer36,
	i_dead36,
	
	//Constants
	down_niOverNt_1,
	down_niOverNt_2,
	down_niOverNt_3,
	down_niOverNt_4,
	down_niOverNt_5,
	up_niOverNt_1,
	up_niOverNt_2,
	up_niOverNt_3,
	up_niOverNt_4,
	up_niOverNt_5,
	down_niOverNt_2_1,
	down_niOverNt_2_2,
	down_niOverNt_2_3,
	down_niOverNt_2_4,
	down_niOverNt_2_5,
	up_niOverNt_2_1,
	up_niOverNt_2_2,
	up_niOverNt_2_3,
	up_niOverNt_2_4,
	up_niOverNt_2_5,
	downCritAngle_0,
	downCritAngle_1,
	downCritAngle_2,
	downCritAngle_3,
	downCritAngle_4,
	upCritAngle_0,
	upCritAngle_1,
	upCritAngle_2,
	upCritAngle_3,
	upCritAngle_4,
	
	//Fresnels inputs
	rnd,
	up_rFresnel,
	down_rFresnel,
	
	//Mathematics Results
	prod1_2,
	prod1_4,
	sqrtResult1_6,
	prod1_36,
	prod2_36,
	
	
	//OUTPUTS
	
	//Fresnels outputs
	fresIndex,
	
	//Mathematics Operands
	op1_2_1,
	op1_2_2,
	op1_4_1,
	op1_4_2,
	sqrtOperand1_6,
	op1_36_1,
	op1_36_2,
	op2_36_1,
	op2_36_2,

	
	//Final Calcu`LATed Results
	ux_reflector,
	uy_reflector,
	uz_reflector,
	layer_reflector,
	dead_reflector
);

//-------------------PARAMETER DEFINITION----------------------
//
//
//
//
//
//
//Assign values to parameters used `LATer in the program.
	
//parameter `DIV = 20;
//parameter `SQRT = 10;
//parameter `LAT = `DIV + `SQRT + 7;
//parameter `INTMAX_2 = 64'h3FFFFFFFFFFFFFFF;
//parameter `INTMAX = 2147483647;
//parameter `INTMIN = -2147483647;


//-----------------------------PIN DECLARATION----------------------
//
//
//
//
//
//
//
//
//Assign appropriate types to pins (input or output).
input					clock;
input					reset;
input					enable;

//Values from Photon Pipeline
input		[31:0]			i_uz1;
input		[31:0]			i_uz3;
input		[2:0]			i_layer3;
input		[31:0]			i_ux35;
input		[31:0]			i_uy35;
input		[31:0]			i_uz35;
input		[2:0]			i_layer35;
input		[31:0]			i_ux36;
input		[31:0]			i_uy36;
input		[31:0]			i_uz36;
input		[2:0]			i_layer36;
input					i_dead36;

//Constants
input		[31:0]			down_niOverNt_1;
input		[31:0]			down_niOverNt_2;
input		[31:0]			down_niOverNt_3;
input		[31:0]			down_niOverNt_4;
input		[31:0]			down_niOverNt_5;
input		[31:0]			up_niOverNt_1;
input		[31:0]			up_niOverNt_2;
input		[31:0]			up_niOverNt_3;
input		[31:0]			up_niOverNt_4;
input		[31:0]			up_niOverNt_5;
input		[63:0]			down_niOverNt_2_1; 
input		[63:0]			down_niOverNt_2_2; 
input		[63:0]			down_niOverNt_2_3; 
input		[63:0]			down_niOverNt_2_4; 
input		[63:0]			down_niOverNt_2_5; 
input		[63:0]			up_niOverNt_2_1; 
input		[63:0]			up_niOverNt_2_2; 
input		[63:0]			up_niOverNt_2_3; 
input		[63:0]			up_niOverNt_2_4; 
input		[63:0]			up_niOverNt_2_5; 
input		[31:0]			downCritAngle_0;
input		[31:0]			downCritAngle_1;
input		[31:0]			downCritAngle_2;
input		[31:0]			downCritAngle_3;
input		[31:0]			downCritAngle_4;
input		[31:0]			upCritAngle_0;
input		[31:0]			upCritAngle_1;
input		[31:0]			upCritAngle_2;
input		[31:0]			upCritAngle_3;
input		[31:0]			upCritAngle_4;

//Fresnels inputs
input		[31:0]			rnd;
input		[31:0]			up_rFresnel;
input		[31:0]			down_rFresnel;

//Mathematics Results
input		[63:0]			prod1_2;
input		[63:0]			prod1_4;
input		[31:0]			sqrtResult1_6;
input		[63:0]			prod1_36;
input		[63:0]			prod2_36;

//OUTPUTS

//Fresnels outputs
output		[9:0]			fresIndex;

//Mathematics operands
output		[31:0]			op1_2_1;
output		[31:0]			op1_2_2;
output		[31:0]			op1_4_1;
output		[31:0]			op1_4_2;
output		[63:0]			sqrtOperand1_6;
output		[31:0]			op1_36_1;
output		[31:0]			op1_36_2;
output		[31:0]			op2_36_1;
output		[31:0]			op2_36_2;


//Final Calcu`LATed Results
output		[31:0]			ux_reflector;
output		[31:0]			uy_reflector;
output		[31:0]			uz_reflector;
output		[2:0]			layer_reflector;
output					dead_reflector;


//-----------------------------PIN TYPES-----------------------------
//
//
//
//
//
//
//
//
//Assign pins to be wires or regs.

wire					clock;
wire					reset;
wire					enable;
//Values from Photon Pipeline
wire		[31:0]			i_uz1;
wire		[31:0]			i_uz3; 
wire		[2:0]			i_layer3;
wire		[31:0]			i_ux35;
wire		[31:0]			i_uy35;
wire		[31:0]			i_uz35;
wire		[2:0]			i_layer35;
wire		[31:0]			i_ux36;
wire		[31:0]			i_uy36;
wire		[31:0]			i_uz36;
wire		[2:0]			i_layer36;
wire					i_dead36;

//Constants
wire		[31:0]			down_niOverNt_1;
wire		[31:0]			down_niOverNt_2;
wire		[31:0]			down_niOverNt_3;
wire		[31:0]			down_niOverNt_4;
wire		[31:0]			down_niOverNt_5;
wire		[31:0]			up_niOverNt_1;
wire		[31:0]			up_niOverNt_2;
wire		[31:0]			up_niOverNt_3;
wire		[31:0]			up_niOverNt_4;
wire		[31:0]			up_niOverNt_5;
wire		[63:0]			down_niOverNt_2_1;  
wire		[63:0]			down_niOverNt_2_2;  
wire		[63:0]			down_niOverNt_2_3;  
wire		[63:0]			down_niOverNt_2_4;  
wire		[63:0]			down_niOverNt_2_5;  
wire		[63:0]			up_niOverNt_2_1;  
wire		[63:0]			up_niOverNt_2_2;  
wire		[63:0]			up_niOverNt_2_3;  
wire		[63:0]			up_niOverNt_2_4;  
wire		[63:0]			up_niOverNt_2_5;  
wire		[31:0]			downCritAngle_0;
wire		[31:0]			downCritAngle_1;
wire		[31:0]			downCritAngle_2;
wire		[31:0]			downCritAngle_3;
wire		[31:0]			downCritAngle_4;
wire		[31:0]			upCritAngle_0;
wire		[31:0]			upCritAngle_1;
wire		[31:0]			upCritAngle_2;
wire		[31:0]			upCritAngle_3;
wire		[31:0]			upCritAngle_4;

//Fresnels inputs
wire		[31:0]			rnd;
wire		[31:0]			up_rFresnel;
wire		[31:0]			down_rFresnel;

//Mathematics Results
wire		[63:0]			prod1_2;
wire		[63:0]			prod1_4;
wire		[31:0]			sqrtResult1_6;
wire		[63:0]			prod1_36;
wire		[63:0]			prod2_36;

//OUTPUTS


//Fresnels outputs
reg		[9:0]			fresIndex;

//Operands for shared resources
wire		[31:0]			op1_2_1;
wire		[31:0]			op1_2_2;
reg		[31:0]			op1_4_1;
wire		[31:0]			op1_4_2;
wire		[63:0]			sqrtOperand1_6;
wire		[31:0]			op1_36_1;
reg		[31:0]			op1_36_2;
wire		[31:0]			op2_36_1;
reg		[31:0]			op2_36_2;

//Final Calcu`LATed Results
reg		[31:0]			ux_reflector;
reg		[31:0]			uy_reflector;
reg		[31:0]			uz_reflector;
reg		[2:0]			layer_reflector;
reg					dead_reflector;


//Need this to deal with 'unused' inputs for ODIN II
wire [63:0]bigOr;
assign bigOr = i_uz3|down_niOverNt_2_1|down_niOverNt_2_2|down_niOverNt_2_3|down_niOverNt_2_3|down_niOverNt_2_4|down_niOverNt_2_5|up_niOverNt_2_1|up_niOverNt_2_2|up_niOverNt_2_3|up_niOverNt_2_3|up_niOverNt_2_4|up_niOverNt_2_5|prod1_36|prod2_36|({32'hFFFFFFFF,32'hFFFFFFFF});
wire reset_new;
assign reset_new = reset & bigOr[63] & bigOr[62] & bigOr[61] & bigOr[60] & bigOr[59] & bigOr[58] & bigOr[57] & bigOr[56] & bigOr[55] & bigOr[54] & bigOr[53] & bigOr[52] & bigOr[51] & bigOr[50] & bigOr[49] & bigOr[48] & bigOr[47] & bigOr[46] & bigOr[45] & bigOr[44] & bigOr[43] & bigOr[42] & bigOr[41] & bigOr[40] & bigOr[39] & bigOr[38] & bigOr[37] & bigOr[36] & bigOr[35] & bigOr[34] & bigOr[33] & bigOr[32] & bigOr[31] & bigOr[30] & bigOr[29] & bigOr[28] & bigOr[27] & bigOr[26] & bigOr[25] & bigOr[24] & bigOr[23] & bigOr[22] & bigOr[21] & bigOr[20] & bigOr[19] & bigOr[18] & bigOr[17] & bigOr[16] & bigOr[15] & bigOr[14] & bigOr[13] & bigOr[12] & bigOr[11] & bigOr[10] & bigOr[9] & bigOr[8] & bigOr[7] & bigOr[6] & bigOr[5] & bigOr[4] & bigOr[3] & bigOr[2] & bigOr[1] & bigOr[0];



//-----------------------------END Pin Types-------------------------

//Overflow Wiring
wire					overflow1_4;
wire					toAnd1_36_1;
wire					toAnd1_36_2;
wire					overflow1_36;
wire					negOverflow1_36;
wire					toAnd2_36_1;
wire					toAnd2_36_2;
wire					overflow2_36;
wire					negOverflow2_36;
	
//Wiring for calcu`LATing final Results
reg		[31:0]			new_ux;
reg		[31:0]			new_uy;
reg		[31:0]			new_uz;
reg		[2:0]			new_layer;
reg					new_dead;
reg		[31:0]			downCritAngle;
reg		[31:0]			upCritAngle;
reg		[31:0]			negUz;



//Wires to Connect to Internal Registers
//wire		[63:0]			uz_2[`LAT:0];
//wire		[31:0]			uz2[`LAT:0];
//wire		[63:0]			oneMinusUz_2[`LAT:0];
//wire		[63:0]			sa2_2[`LAT:0];
//wire		[63:0]			uz2_2[`LAT:0];
//wire		[31:0]			ux_transmitted[`LAT:0];
//wire		[31:0]			uy_transmitted[`LAT:0];

wire	[63:0]				uz_2__0;
wire	[63:0]				uz_2__1;
wire	[63:0]				uz_2__2;
wire	[63:0]				uz_2__3;
wire	[63:0]				uz_2__4;
wire	[63:0]				uz_2__5;
wire	[63:0]				uz_2__6;
wire	[63:0]				uz_2__7;
wire	[63:0]				uz_2__8;
wire	[63:0]				uz_2__9;
wire	[63:0]				uz_2__10;
wire	[63:0]				uz_2__11;
wire	[63:0]				uz_2__12;
wire	[63:0]				uz_2__13;
wire	[63:0]				uz_2__14;
wire	[63:0]				uz_2__15;
wire	[63:0]				uz_2__16;
wire	[63:0]				uz_2__17;
wire	[63:0]				uz_2__18;
wire	[63:0]				uz_2__19;
wire	[63:0]				uz_2__20;
wire	[63:0]				uz_2__21;
wire	[63:0]				uz_2__22;
wire	[63:0]				uz_2__23;
wire	[63:0]				uz_2__24;
wire	[63:0]				uz_2__25;
wire	[63:0]				uz_2__26;
wire	[63:0]				uz_2__27;
wire	[63:0]				uz_2__28;
wire	[63:0]				uz_2__29;
wire	[63:0]				uz_2__30;
wire	[63:0]				uz_2__31;
wire	[63:0]				uz_2__32;
wire	[63:0]				uz_2__33;
wire	[63:0]				uz_2__34;
wire	[63:0]				uz_2__35;
wire	[63:0]				uz_2__36;
wire	[63:0]				uz_2__37;



//wire		[31:0]			uz2[37:0];
wire	[32-1:0]				uz2__0;
wire	[32-1:0]				uz2__1;
wire	[32-1:0]				uz2__2;
wire	[32-1:0]				uz2__3;
wire	[32-1:0]				uz2__4;
wire	[32-1:0]				uz2__5;
wire	[32-1:0]				uz2__6;
wire	[32-1:0]				uz2__7;
wire	[32-1:0]				uz2__8;
wire	[32-1:0]				uz2__9;
wire	[32-1:0]				uz2__10;
wire	[32-1:0]				uz2__11;
wire	[32-1:0]				uz2__12;
wire	[32-1:0]				uz2__13;
wire	[32-1:0]				uz2__14;
wire	[32-1:0]				uz2__15;
wire	[32-1:0]				uz2__16;
wire	[32-1:0]				uz2__17;
wire	[32-1:0]				uz2__18;
wire	[32-1:0]				uz2__19;
wire	[32-1:0]				uz2__20;
wire	[32-1:0]				uz2__21;
wire	[32-1:0]				uz2__22;
wire	[32-1:0]				uz2__23;
wire	[32-1:0]				uz2__24;
wire	[32-1:0]				uz2__25;
wire	[32-1:0]				uz2__26;
wire	[32-1:0]				uz2__27;
wire	[32-1:0]				uz2__28;
wire	[32-1:0]				uz2__29;
wire	[32-1:0]				uz2__30;
wire	[32-1:0]				uz2__31;
wire	[32-1:0]				uz2__32;
wire	[32-1:0]				uz2__33;
wire	[32-1:0]				uz2__34;
wire	[32-1:0]				uz2__35;
wire	[32-1:0]				uz2__36;
wire	[32-1:0]				uz2__37;


//wire		[63:0]			oneMinusUz_2[37:0];

wire	[63:0]				oneMinusUz_2__0;
wire	[63:0]				oneMinusUz_2__1;
wire	[63:0]				oneMinusUz_2__2;
wire	[63:0]				oneMinusUz_2__3;
wire	[63:0]				oneMinusUz_2__4;
wire	[63:0]				oneMinusUz_2__5;
wire	[63:0]				oneMinusUz_2__6;
wire	[63:0]				oneMinusUz_2__7;
wire	[63:0]				oneMinusUz_2__8;
wire	[63:0]				oneMinusUz_2__9;
wire	[63:0]				oneMinusUz_2__10;
wire	[63:0]				oneMinusUz_2__11;
wire	[63:0]				oneMinusUz_2__12;
wire	[63:0]				oneMinusUz_2__13;
wire	[63:0]				oneMinusUz_2__14;
wire	[63:0]				oneMinusUz_2__15;
wire	[63:0]				oneMinusUz_2__16;
wire	[63:0]				oneMinusUz_2__17;
wire	[63:0]				oneMinusUz_2__18;
wire	[63:0]				oneMinusUz_2__19;
wire	[63:0]				oneMinusUz_2__20;
wire	[63:0]				oneMinusUz_2__21;
wire	[63:0]				oneMinusUz_2__22;
wire	[63:0]				oneMinusUz_2__23;
wire	[63:0]				oneMinusUz_2__24;
wire	[63:0]				oneMinusUz_2__25;
wire	[63:0]				oneMinusUz_2__26;
wire	[63:0]				oneMinusUz_2__27;
wire	[63:0]				oneMinusUz_2__28;
wire	[63:0]				oneMinusUz_2__29;
wire	[63:0]				oneMinusUz_2__30;
wire	[63:0]				oneMinusUz_2__31;
wire	[63:0]				oneMinusUz_2__32;
wire	[63:0]				oneMinusUz_2__33;
wire	[63:0]				oneMinusUz_2__34;
wire	[63:0]				oneMinusUz_2__35;
wire	[63:0]				oneMinusUz_2__36;
wire	[63:0]				oneMinusUz_2__37;


//wire		[63:0]			sa2_2[37:0];
wire	[63:0]				sa2_2__0;
wire	[63:0]				sa2_2__1;
wire	[63:0]				sa2_2__2;
wire	[63:0]				sa2_2__3;
wire	[63:0]				sa2_2__4;
wire	[63:0]				sa2_2__5;
wire	[63:0]				sa2_2__6;
wire	[63:0]				sa2_2__7;
wire	[63:0]				sa2_2__8;
wire	[63:0]				sa2_2__9;
wire	[63:0]				sa2_2__10;
wire	[63:0]				sa2_2__11;
wire	[63:0]				sa2_2__12;
wire	[63:0]				sa2_2__13;
wire	[63:0]				sa2_2__14;
wire	[63:0]				sa2_2__15;
wire	[63:0]				sa2_2__16;
wire	[63:0]				sa2_2__17;
wire	[63:0]				sa2_2__18;
wire	[63:0]				sa2_2__19;
wire	[63:0]				sa2_2__20;
wire	[63:0]				sa2_2__21;
wire	[63:0]				sa2_2__22;
wire	[63:0]				sa2_2__23;
wire	[63:0]				sa2_2__24;
wire	[63:0]				sa2_2__25;
wire	[63:0]				sa2_2__26;
wire	[63:0]				sa2_2__27;
wire	[63:0]				sa2_2__28;
wire	[63:0]				sa2_2__29;
wire	[63:0]				sa2_2__30;
wire	[63:0]				sa2_2__31;
wire	[63:0]				sa2_2__32;
wire	[63:0]				sa2_2__33;
wire	[63:0]				sa2_2__34;
wire	[63:0]				sa2_2__35;
wire	[63:0]				sa2_2__36;
wire	[63:0]				sa2_2__37;


//wire		[63:0]			uz2_2[37:0];

wire	[63:0]				uz2_2__0;
wire	[63:0]				uz2_2__1;
wire	[63:0]				uz2_2__2;
wire	[63:0]				uz2_2__3;
wire	[63:0]				uz2_2__4;
wire	[63:0]				uz2_2__5;
wire	[63:0]				uz2_2__6;
wire	[63:0]				uz2_2__7;
wire	[63:0]				uz2_2__8;
wire	[63:0]				uz2_2__9;
wire	[63:0]				uz2_2__10;
wire	[63:0]				uz2_2__11;
wire	[63:0]				uz2_2__12;
wire	[63:0]				uz2_2__13;
wire	[63:0]				uz2_2__14;
wire	[63:0]				uz2_2__15;
wire	[63:0]				uz2_2__16;
wire	[63:0]				uz2_2__17;
wire	[63:0]				uz2_2__18;
wire	[63:0]				uz2_2__19;
wire	[63:0]				uz2_2__20;
wire	[63:0]				uz2_2__21;
wire	[63:0]				uz2_2__22;
wire	[63:0]				uz2_2__23;
wire	[63:0]				uz2_2__24;
wire	[63:0]				uz2_2__25;
wire	[63:0]				uz2_2__26;
wire	[63:0]				uz2_2__27;
wire	[63:0]				uz2_2__28;
wire	[63:0]				uz2_2__29;
wire	[63:0]				uz2_2__30;
wire	[63:0]				uz2_2__31;
wire	[63:0]				uz2_2__32;
wire	[63:0]				uz2_2__33;
wire	[63:0]				uz2_2__34;
wire	[63:0]				uz2_2__35;
wire	[63:0]				uz2_2__36;
wire	[63:0]				uz2_2__37;

//wire		[31:0]			ux_transmitted[37:0];

wire	[32-1:0]				ux_transmitted__0;
wire	[32-1:0]				ux_transmitted__1;
wire	[32-1:0]				ux_transmitted__2;
wire	[32-1:0]				ux_transmitted__3;
wire	[32-1:0]				ux_transmitted__4;
wire	[32-1:0]				ux_transmitted__5;
wire	[32-1:0]				ux_transmitted__6;
wire	[32-1:0]				ux_transmitted__7;
wire	[32-1:0]				ux_transmitted__8;
wire	[32-1:0]				ux_transmitted__9;
wire	[32-1:0]				ux_transmitted__10;
wire	[32-1:0]				ux_transmitted__11;
wire	[32-1:0]				ux_transmitted__12;
wire	[32-1:0]				ux_transmitted__13;
wire	[32-1:0]				ux_transmitted__14;
wire	[32-1:0]				ux_transmitted__15;
wire	[32-1:0]				ux_transmitted__16;
wire	[32-1:0]				ux_transmitted__17;
wire	[32-1:0]				ux_transmitted__18;
wire	[32-1:0]				ux_transmitted__19;
wire	[32-1:0]				ux_transmitted__20;
wire	[32-1:0]				ux_transmitted__21;
wire	[32-1:0]				ux_transmitted__22;
wire	[32-1:0]				ux_transmitted__23;
wire	[32-1:0]				ux_transmitted__24;
wire	[32-1:0]				ux_transmitted__25;
wire	[32-1:0]				ux_transmitted__26;
wire	[32-1:0]				ux_transmitted__27;
wire	[32-1:0]				ux_transmitted__28;
wire	[32-1:0]				ux_transmitted__29;
wire	[32-1:0]				ux_transmitted__30;
wire	[32-1:0]				ux_transmitted__31;
wire	[32-1:0]				ux_transmitted__32;
wire	[32-1:0]				ux_transmitted__33;
wire	[32-1:0]				ux_transmitted__34;
wire	[32-1:0]				ux_transmitted__35;
wire	[32-1:0]				ux_transmitted__36;
wire	[32-1:0]				ux_transmitted__37;

//wire		[31:0]			uy_transmitted[37:0];

wire	[32-1:0]				uy_transmitted__0;
wire	[32-1:0]				uy_transmitted__1;
wire	[32-1:0]				uy_transmitted__2;
wire	[32-1:0]				uy_transmitted__3;
wire	[32-1:0]				uy_transmitted__4;
wire	[32-1:0]				uy_transmitted__5;
wire	[32-1:0]				uy_transmitted__6;
wire	[32-1:0]				uy_transmitted__7;
wire	[32-1:0]				uy_transmitted__8;
wire	[32-1:0]				uy_transmitted__9;
wire	[32-1:0]				uy_transmitted__10;
wire	[32-1:0]				uy_transmitted__11;
wire	[32-1:0]				uy_transmitted__12;
wire	[32-1:0]				uy_transmitted__13;
wire	[32-1:0]				uy_transmitted__14;
wire	[32-1:0]				uy_transmitted__15;
wire	[32-1:0]				uy_transmitted__16;
wire	[32-1:0]				uy_transmitted__17;
wire	[32-1:0]				uy_transmitted__18;
wire	[32-1:0]				uy_transmitted__19;
wire	[32-1:0]				uy_transmitted__20;
wire	[32-1:0]				uy_transmitted__21;
wire	[32-1:0]				uy_transmitted__22;
wire	[32-1:0]				uy_transmitted__23;
wire	[32-1:0]				uy_transmitted__24;
wire	[32-1:0]				uy_transmitted__25;
wire	[32-1:0]				uy_transmitted__26;
wire	[32-1:0]				uy_transmitted__27;
wire	[32-1:0]				uy_transmitted__28;
wire	[32-1:0]				uy_transmitted__29;
wire	[32-1:0]				uy_transmitted__30;
wire	[32-1:0]				uy_transmitted__31;
wire	[32-1:0]				uy_transmitted__32;
wire	[32-1:0]				uy_transmitted__33;
wire	[32-1:0]				uy_transmitted__34;
wire	[32-1:0]				uy_transmitted__35;
wire	[32-1:0]				uy_transmitted__36;
wire	[32-1:0]				uy_transmitted__37;

wire		[63:0]			new_uz_2;
wire		[31:0]			new_uz2;
wire		[63:0]			new_oneMinusUz_2;
wire		[63:0]			new_sa2_2;
wire		[63:0]			new_uz2_2;
reg		[31:0]			new_ux_transmitted;
reg		[31:0]			new_uy_transmitted;



//------------------Register Pipeline-----------------
//Generation Methodology: Standard block, called InternalsBlock_Reflector,
//is repeated multiple times, based on the `LATency of the reflector and
//scatterer.  This block contains the list of all internal variables
//that need to be registered and passed along in the pipeline.
//
//Previous values in the pipeline are passed to the next register on each
//clock tick.  The exception comes when an internal variable gets
//calcu`LATed.  Each time a new internal variable is calcu`LATed, a new
//case is added to the case statement, and instead of hooking previous
//values of that variable to next, the new, calcu`LATed values are hooked up.
//
//This method will generate many more registers than what are required, but
//it is expected that the synthesis tool will synthesize these away.
//
//
//Commenting Convention: Whenever a new value is injected into the pipe, the
//comment //Changed Value is added directly above the variable in question.
//When multiple values are calcu`LATed in a single clock cycle, multiple such
//comments are placed.  Wires connected to "Changed Values" always start with
//the prefix new_.
//
//GENERATE PIPELINE

//genvar i;
//generate
//	for(i=`LAT; i>0; i=i-1) begin: internalPipe_Reflector
//		case(i)
//		
//		2:
//		InternalsBlock_Reflector pipeReg(
//			Inputs
//			.clock(clock),
//			.reset(reset),
//			.enable(enable),
//			
//			Changed Value
//			.i_uz_2(new_uz_2),			//uz^2
//			.i_uz2(uz2[i-1]),			//new uz, should the photon transmit to new layer
//			.i_oneMinusUz_2(oneMinusUz_2[i-1]), 	//(1-uz)^2
//			.i_sa2_2(sa2_2[i-1]),			//(sine of angle 2)^2 (uz2 = cosine of angle 2).
//			.i_uz2_2(uz2_2[i-1]),			//(uz2)^2, new uz squared.
//			.i_ux_transmitted(ux_transmitted[i-1]), //New value for ux, if photon moves to next layer
//			.i_uy_transmitted(uy_transmitted[i-1]),	//New value for uy, if photon moves to next layer
//
//			Outputs
//			.o_uz_2(uz_2[i]),
//			.o_uz2(uz2[i]),
//			.o_oneMinusUz_2(oneMinusUz_2[i]),
//			.o_sa2_2(sa2_2[i]),
//			.o_uz2_2(uz2_2[i]),
//			.o_ux_transmitted(ux_transmitted[i]),
//			.o_uy_transmitted(uy_transmitted[i])
//		);
//		
//		3:
//		InternalsBlock_Reflector pipeReg(
//			Inputs
//			.clock(clock),
//			.reset(reset),
//			.enable(enable),
//
//			.i_uz_2(uz_2[i-1]),			//uz^2
//			.i_uz2(uz2[i-1]),			//new uz, should the photon transmit to new layer
//		//	Changed Value
//			.i_oneMinusUz_2(new_oneMinusUz_2), 	//(1-uz)^2
//			.i_sa2_2(sa2_2[i-1]),			//(sine of angle 2)^2 (uz2 = cosine of angle 2).
//			.i_uz2_2(uz2_2[i-1]),			//(uz2)^2, new uz squared.
//			.i_ux_transmitted(ux_transmitted[i-1]), //New value for ux, if photon moves to next layer
//			.i_uy_transmitted(uy_transmitted[i-1]),	//New value for uy, if photon moves to next layer
//
//			Outputs
//			.o_uz_2(uz_2[i]),
//			.o_uz2(uz2[i]),
//			.o_oneMinusUz_2(oneMinusUz_2[i]),
//			.o_sa2_2(sa2_2[i]),
//			.o_uz2_2(uz2_2[i]),
//			.o_ux_transmitted(ux_transmitted[i]),
//			.o_uy_transmitted(uy_transmitted[i])
//		);
//		4:
//		InternalsBlock_Reflector pipeReg(
//			Inputs
//			.clock(clock),
//			.reset(reset),
//			.enable(enable),
//
//			.i_uz_2(uz_2[i-1]),			//uz^2
//			.i_uz2(uz2[i-1]),			//new uz, should the photon transmit to new layer
//			.i_oneMinusUz_2(oneMinusUz_2[i-1]), 	//(1-uz)^2
//			Changed Value
//			.i_sa2_2(new_sa2_2),			//(sine of angle 2)^2 (uz2 = cosine of angle 2).
//			.i_uz2_2(uz2_2[i-1]),			//(uz2)^2, new uz squared.
//			.i_ux_transmitted(ux_transmitted[i-1]), //New value for ux, if photon moves to next layer
//			.i_uy_transmitted(uy_transmitted[i-1]),	//New value for uy, if photon moves to next layer
//
//			Outputs
//			.o_uz_2(uz_2[i]),
//			.o_uz2(uz2[i]),
//			.o_oneMinusUz_2(oneMinusUz_2[i]),
//			.o_sa2_2(sa2_2[i]),
//			.o_uz2_2(uz2_2[i]),
//			.o_ux_transmitted(ux_transmitted[i]),
//			.o_uy_transmitted(uy_transmitted[i])
//		);
//		
//		5:
//		InternalsBlock_Reflector pipeReg(
//			Inputs
//			.clock(clock),
//			.reset(reset),
//			.enable(enable),
//
//			.i_uz_2(uz_2[i-1]),			//uz^2
//			.i_uz2(uz2[i-1]),			//new uz, should the photon transmit to new layer
//			.i_oneMinusUz_2(oneMinusUz_2[i-1]), 	//(1-uz)^2
//			.i_sa2_2(sa2_2[i-1]),			//(sine of angle 2)^2 (uz2 = cosine of angle 2).
//			Changed Value
//			.i_uz2_2(new_uz2_2),			//(uz2)^2, new uz squared.
//			.i_ux_transmitted(ux_transmitted[i-1]), //New value for ux, if photon moves to next layer
//			.i_uy_transmitted(uy_transmitted[i-1]),	//New value for uy, if photon moves to next layer
//
//			Outputs
//			.o_uz_2(uz_2[i]),
//			.o_uz2(uz2[i]),
//			.o_oneMinusUz_2(oneMinusUz_2[i]),
//			.o_sa2_2(sa2_2[i]),
//			.o_uz2_2(uz2_2[i]),
//			.o_ux_transmitted(ux_transmitted[i]),
//			.o_uy_transmitted(uy_transmitted[i])
//		);
//		
//		(`SQRT+6):
//		InternalsBlock_Reflector pipeReg(
//			Inputs
//			.clock(clock),
//			.reset(reset),
//			.enable(enable),
//
//			.i_uz_2(uz_2[i-1]),			//uz^2
//			Changed Value
//			.i_uz2(new_uz2),			//new uz, should the photon transmit to new layer
//			.i_oneMinusUz_2(oneMinusUz_2[i-1]), 	//(1-uz)^2
//			.i_sa2_2(sa2_2[i-1]),			//(sine of angle 2)^2 (uz2 = cosine of angle 2).
//			.i_uz2_2(uz2_2[i-1]),			//(uz2)^2, new uz squared.
//			.i_ux_transmitted(ux_transmitted[i-1]), //New value for ux, if photon moves to next layer
//			.i_uy_transmitted(uy_transmitted[i-1]),	//New value for uy, if photon moves to next layer
//
//			Outputs
//			.o_uz_2(uz_2[i]),
//			.o_uz2(uz2[i]),
//			.o_oneMinusUz_2(oneMinusUz_2[i]),
//			.o_sa2_2(sa2_2[i]),
//			.o_uz2_2(uz2_2[i]),
//			.o_ux_transmitted(ux_transmitted[i]),
//			.o_uy_transmitted(uy_transmitted[i])
//		);
//		
//		(`SQRT+`DIV+6):
//		InternalsBlock_Reflector pipeReg(
//			Inputs
//			.clock(clock),
//			.reset(reset),
//			.enable(enable),
//
//			.i_uz_2(uz_2[i-1]),			//uz^2
//			.i_uz2(uz2[i-1]),			//new uz, should the photon transmit to new layer
//			.i_oneMinusUz_2(oneMinusUz_2[i-1]), 	//(1-uz)^2
//			.i_sa2_2(sa2_2[i-1]),			//(sine of angle 2)^2 (uz2 = cosine of angle 2).
//			.i_uz2_2(uz2_2[i-1]),			//(uz2)^2, new uz squared.
//			Changed Value
//			.i_ux_transmitted(new_ux_transmitted),	//New value for ux, if photon moves to next layer
//			Changed Value
//			.i_uy_transmitted(new_uy_transmitted),	//New value for uy, if photon moves to next layer
//
//			Outputs
//			.o_uz_2(uz_2[i]),
//			.o_uz2(uz2[i]),
//			.o_oneMinusUz_2(oneMinusUz_2[i]),
//			.o_sa2_2(sa2_2[i]),
//			.o_uz2_2(uz2_2[i]),
//			.o_ux_transmitted(ux_transmitted[i]),
//			.o_uy_transmitted(uy_transmitted[i])
//		);
//		default:
//		InternalsBlock_Reflector pipeReg(
//			Inputs
//			.clock(clock),
//			.reset(reset),
//			.enable(enable),
//		
//			.i_uz_2(uz_2[i-1]),			//uz^2
//			.i_uz2(uz2[i-1]),			//new uz, should the photon transmit to new layer
//			.i_oneMinusUz_2(oneMinusUz_2[i-1]), 	//(1-uz)^2
//			.i_sa2_2(sa2_2[i-1]),			//(sine of angle 2)^2 (uz2 = cosine of angle 2).
//			.i_uz2_2(uz2_2[i-1]),			//(uz2)^2, new uz squared.
//			.i_ux_transmitted(ux_transmitted[i-1]), //New value for ux, if photon moves to next layer
//			.i_uy_transmitted(uy_transmitted[i-1]),	//New value for uy, if photon moves to next layer
//			
//			Outputs
//			.o_uz_2(uz_2[i]),
//			.o_uz2(uz2[i]),
//			.o_oneMinusUz_2(oneMinusUz_2[i]),
//			.o_sa2_2(sa2_2[i]),
//			.o_uz2_2(uz2_2[i]),
//			.o_ux_transmitted(ux_transmitted[i]),
//			.o_uy_transmitted(uy_transmitted[i])
//		);
//		endcase
//	end
//endgenerate	



// special cases first
	
	//	forloop2
		InternalsBlock_Reflector pipeReg2(
			//Inputs
			.clock(clock),
			.reset(reset),
			.enable(enable),
			
			//Changed Value
			.i_uz_2(new_uz_2),			//uz^2
			.i_uz2(uz2__1),			//new uz, should the photon transmit to new layer
			.i_oneMinusUz_2(oneMinusUz_2__1), 	//(1-uz)^2
			.i_sa2_2(sa2_2__1),			//(sine of angle 2)^2 (uz2 = cosine of angle 2).
			.i_uz2_2(uz2_2__1),			//(uz2)^2, new uz squared.
			.i_ux_transmitted(ux_transmitted__1), //New value for ux, if photon moves to next layer
			.i_uy_transmitted(uy_transmitted__1),	//New value for uy, if photon moves to next layer

			//Outputs
			.o_uz_2(uz_2__2),
			.o_uz2(uz2__2),
			.o_oneMinusUz_2(oneMinusUz_2__2),
			.o_sa2_2(sa2_2__2),
			.o_uz2_2(uz2_2__2),
			.o_ux_transmitted(ux_transmitted__2),
			.o_uy_transmitted(uy_transmitted__2)
		);
		
		// for loop3:
		InternalsBlock_Reflector pipeReg3(
			//Inputs
			.clock(clock),
			.reset(reset),
			.enable(enable),

			.i_uz_2(uz_2__2),			//uz^2
			.i_uz2(uz2__2),			//new uz, should the photon transmit to new layer
			//Changed Value
			.i_oneMinusUz_2(new_oneMinusUz_2), 	//(1-uz)^2
			.i_sa2_2(sa2_2__2),			//(sine of angle 2)^2 (uz2 = cosine of angle 2).
			.i_uz2_2(uz2_2__2),			//(uz2)^2, new uz squared.
			.i_ux_transmitted(ux_transmitted__2), //New value for ux, if photon moves to next layer
			.i_uy_transmitted(uy_transmitted__2),	//New value for uy, if photon moves to next layer

			//Outputs
			.o_uz_2(uz_2__3),
			.o_uz2(uz2__3),
			.o_oneMinusUz_2(oneMinusUz_2__3),
			.o_sa2_2(sa2_2__3),
			.o_uz2_2(uz2_2__3),
			.o_ux_transmitted(ux_transmitted__3),
			.o_uy_transmitted(uy_transmitted__3)
		);
		
		// for loop4
		InternalsBlock_Reflector pipeReg4(
			//Inputs
			.clock(clock),
			.reset(reset),
			.enable(enable),

			.i_uz_2(uz_2__3),			//uz^2
			.i_uz2(uz2__3),			//new uz, should the photon transmit to new layer
			.i_oneMinusUz_2(oneMinusUz_2__3), 	//(1-uz)^2
			//Changed Value
			.i_sa2_2(new_sa2_2),			//(sine of angle 2)^2 (uz2 = cosine of angle 2).
			.i_uz2_2(uz2_2__3),			//(uz2)^2, new uz squared.
			.i_ux_transmitted(ux_transmitted__3), //New value for ux, if photon moves to next layer
			.i_uy_transmitted(uy_transmitted__3),	//New value for uy, if photon moves to next layer

			//Outputs
			.o_uz_2(uz_2__4),
			.o_uz2(uz2__4),
			.o_oneMinusUz_2(oneMinusUz_2__4),
			.o_sa2_2(sa2_2__4),
			.o_uz2_2(uz2_2__4),
			.o_ux_transmitted(ux_transmitted__4),
			.o_uy_transmitted(uy_transmitted__4)
		);
		
		//for loop5
		InternalsBlock_Reflector pipeReg5(
			//Inputs
			.clock(clock),
			.reset(reset),
			.enable(enable),

			.i_uz_2(uz_2__4),			//uz^2
			.i_uz2(uz2__4),			//new uz, should the photon transmit to new layer
			.i_oneMinusUz_2(oneMinusUz_2__4), 	//(1-uz)^2
			.i_sa2_2(sa2_2__4),			//(sine of angle 2)^2 (uz2 = cosine of angle 2).
			//Changed Value
			.i_uz2_2(new_uz2_2),			//(uz2)^2, new uz squared.
			.i_ux_transmitted(ux_transmitted__4), //New value for ux, if photon moves to next layer
			.i_uy_transmitted(uy_transmitted__4),	//New value for uy, if photon moves to next layer

			//Outputs
			.o_uz_2(uz_2__5),
			.o_uz2(uz2__5),
			.o_oneMinusUz_2(oneMinusUz_2__5),
			.o_sa2_2(sa2_2__5),
			.o_uz2_2(uz2_2__5),
			.o_ux_transmitted(ux_transmitted__5),
			.o_uy_transmitted(uy_transmitted__5)
		);
		
		//for loop(10+6):
		InternalsBlock_Reflector pipeReg16(
			//Inputs
			.clock(clock),
			.reset(reset),
			.enable(enable),

			.i_uz_2(uz_2__15),			//uz^2
			//Changed Value
			.i_uz2(new_uz2),			//new uz, should the photon transmit to new layer
			.i_oneMinusUz_2(oneMinusUz_2__15), 	//(1-uz)^2
			.i_sa2_2(sa2_2__15),			//(sine of angle 2)^2 (uz2 = cosine of angle 2).
			.i_uz2_2(uz2_2__15),			//(uz2)^2, new uz squared.
			.i_ux_transmitted(ux_transmitted__15), //New value for ux, if photon moves to next layer
			.i_uy_transmitted(uy_transmitted__15),	//New value for uy, if photon moves to next layer

			//Outputs
			.o_uz_2(uz_2__16),
			.o_uz2(uz2__16),
			.o_oneMinusUz_2(oneMinusUz_2__16),
			.o_sa2_2(sa2_2__16),
			.o_uz2_2(uz2_2__16),
			.o_ux_transmitted(ux_transmitted__16),
			.o_uy_transmitted(uy_transmitted__16)
		);
		
		//for loop (10+20+6):
		InternalsBlock_Reflector pipeReg36(
			//Inputs
			.clock(clock),
			.reset(reset),
			.enable(enable),

			.i_uz_2(uz_2__35),			//uz^2
			.i_uz2(uz2__35),			//new uz, should the photon transmit to new layer
			.i_oneMinusUz_2(oneMinusUz_2__35), 	//(1-uz)^2
			.i_sa2_2(sa2_2__35),			//(sine of angle 2)^2 (uz2 = cosine of angle 2).
			.i_uz2_2(uz2_2__35),			//(uz2)^2, new uz squared.
			//Changed Value
			.i_ux_transmitted(new_ux_transmitted),	//New value for ux, if photon moves to next layer
			//Changed Value
			.i_uy_transmitted(new_uy_transmitted),	//New value for uy, if photon moves to next layer

			//Outputs
			.o_uz_2(uz_2__36),
			.o_uz2(uz2__36),
			.o_oneMinusUz_2(oneMinusUz_2__36),
			.o_sa2_2(sa2_2__36),
			.o_uz2_2(uz2_2__36),
			.o_ux_transmitted(ux_transmitted__36),
			.o_uy_transmitted(uy_transmitted__36)
		);

		
		//rest of loop
		
InternalsBlock_Reflector pipeReg37(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__36),
.i_uz2(uz2__36),
.i_oneMinusUz_2(oneMinusUz_2__36),
.i_sa2_2(sa2_2__36),
.i_uz2_2(uz2_2__36),
.i_ux_transmitted(ux_transmitted__36),
.i_uy_transmitted(uy_transmitted__36),

 //outputs

.o_uz_2(uz_2__37),
.o_uz2(uz2__37),
.o_oneMinusUz_2(oneMinusUz_2__37),
.o_sa2_2(sa2_2__37),
.o_uz2_2(uz2_2__37),
.o_ux_transmitted(ux_transmitted__37),
.o_uy_transmitted(uy_transmitted__37)
);

//removed 36
InternalsBlock_Reflector pipeReg35(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__34),
.i_uz2(uz2__34),
.i_oneMinusUz_2(oneMinusUz_2__34),
.i_sa2_2(sa2_2__34),
.i_uz2_2(uz2_2__34),
.i_ux_transmitted(ux_transmitted__34),
.i_uy_transmitted(uy_transmitted__34),

 //outputs

.o_uz_2(uz_2__35),
.o_uz2(uz2__35),
.o_oneMinusUz_2(oneMinusUz_2__35),
.o_sa2_2(sa2_2__35),
.o_uz2_2(uz2_2__35),
.o_ux_transmitted(ux_transmitted__35),
.o_uy_transmitted(uy_transmitted__35)
);

InternalsBlock_Reflector pipeReg34(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__33),
.i_uz2(uz2__33),
.i_oneMinusUz_2(oneMinusUz_2__33),
.i_sa2_2(sa2_2__33),
.i_uz2_2(uz2_2__33),
.i_ux_transmitted(ux_transmitted__33),
.i_uy_transmitted(uy_transmitted__33),

 //outputs

.o_uz_2(uz_2__34),
.o_uz2(uz2__34),
.o_oneMinusUz_2(oneMinusUz_2__34),
.o_sa2_2(sa2_2__34),
.o_uz2_2(uz2_2__34),
.o_ux_transmitted(ux_transmitted__34),
.o_uy_transmitted(uy_transmitted__34)
);

InternalsBlock_Reflector pipeReg33(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__32),
.i_uz2(uz2__32),
.i_oneMinusUz_2(oneMinusUz_2__32),
.i_sa2_2(sa2_2__32),
.i_uz2_2(uz2_2__32),
.i_ux_transmitted(ux_transmitted__32),
.i_uy_transmitted(uy_transmitted__32),

 //outputs

.o_uz_2(uz_2__33),
.o_uz2(uz2__33),
.o_oneMinusUz_2(oneMinusUz_2__33),
.o_sa2_2(sa2_2__33),
.o_uz2_2(uz2_2__33),
.o_ux_transmitted(ux_transmitted__33),
.o_uy_transmitted(uy_transmitted__33)
);

InternalsBlock_Reflector pipeReg32(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__31),
.i_uz2(uz2__31),
.i_oneMinusUz_2(oneMinusUz_2__31),
.i_sa2_2(sa2_2__31),
.i_uz2_2(uz2_2__31),
.i_ux_transmitted(ux_transmitted__31),
.i_uy_transmitted(uy_transmitted__31),

 //outputs

.o_uz_2(uz_2__32),
.o_uz2(uz2__32),
.o_oneMinusUz_2(oneMinusUz_2__32),
.o_sa2_2(sa2_2__32),
.o_uz2_2(uz2_2__32),
.o_ux_transmitted(ux_transmitted__32),
.o_uy_transmitted(uy_transmitted__32)
);

InternalsBlock_Reflector pipeReg31(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__30),
.i_uz2(uz2__30),
.i_oneMinusUz_2(oneMinusUz_2__30),
.i_sa2_2(sa2_2__30),
.i_uz2_2(uz2_2__30),
.i_ux_transmitted(ux_transmitted__30),
.i_uy_transmitted(uy_transmitted__30),

 //outputs

.o_uz_2(uz_2__31),
.o_uz2(uz2__31),
.o_oneMinusUz_2(oneMinusUz_2__31),
.o_sa2_2(sa2_2__31),
.o_uz2_2(uz2_2__31),
.o_ux_transmitted(ux_transmitted__31),
.o_uy_transmitted(uy_transmitted__31)
);

InternalsBlock_Reflector pipeReg30(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__29),
.i_uz2(uz2__29),
.i_oneMinusUz_2(oneMinusUz_2__29),
.i_sa2_2(sa2_2__29),
.i_uz2_2(uz2_2__29),
.i_ux_transmitted(ux_transmitted__29),
.i_uy_transmitted(uy_transmitted__29),

 //outputs

.o_uz_2(uz_2__30),
.o_uz2(uz2__30),
.o_oneMinusUz_2(oneMinusUz_2__30),
.o_sa2_2(sa2_2__30),
.o_uz2_2(uz2_2__30),
.o_ux_transmitted(ux_transmitted__30),
.o_uy_transmitted(uy_transmitted__30)
);

InternalsBlock_Reflector pipeReg29(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__28),
.i_uz2(uz2__28),
.i_oneMinusUz_2(oneMinusUz_2__28),
.i_sa2_2(sa2_2__28),
.i_uz2_2(uz2_2__28),
.i_ux_transmitted(ux_transmitted__28),
.i_uy_transmitted(uy_transmitted__28),

 //outputs

.o_uz_2(uz_2__29),
.o_uz2(uz2__29),
.o_oneMinusUz_2(oneMinusUz_2__29),
.o_sa2_2(sa2_2__29),
.o_uz2_2(uz2_2__29),
.o_ux_transmitted(ux_transmitted__29),
.o_uy_transmitted(uy_transmitted__29)
);

InternalsBlock_Reflector pipeReg28(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__27),
.i_uz2(uz2__27),
.i_oneMinusUz_2(oneMinusUz_2__27),
.i_sa2_2(sa2_2__27),
.i_uz2_2(uz2_2__27),
.i_ux_transmitted(ux_transmitted__27),
.i_uy_transmitted(uy_transmitted__27),

 //outputs

.o_uz_2(uz_2__28),
.o_uz2(uz2__28),
.o_oneMinusUz_2(oneMinusUz_2__28),
.o_sa2_2(sa2_2__28),
.o_uz2_2(uz2_2__28),
.o_ux_transmitted(ux_transmitted__28),
.o_uy_transmitted(uy_transmitted__28)
);

InternalsBlock_Reflector pipeReg27(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__26),
.i_uz2(uz2__26),
.i_oneMinusUz_2(oneMinusUz_2__26),
.i_sa2_2(sa2_2__26),
.i_uz2_2(uz2_2__26),
.i_ux_transmitted(ux_transmitted__26),
.i_uy_transmitted(uy_transmitted__26),

 //outputs

.o_uz_2(uz_2__27),
.o_uz2(uz2__27),
.o_oneMinusUz_2(oneMinusUz_2__27),
.o_sa2_2(sa2_2__27),
.o_uz2_2(uz2_2__27),
.o_ux_transmitted(ux_transmitted__27),
.o_uy_transmitted(uy_transmitted__27)
);

InternalsBlock_Reflector pipeReg26(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__25),
.i_uz2(uz2__25),
.i_oneMinusUz_2(oneMinusUz_2__25),
.i_sa2_2(sa2_2__25),
.i_uz2_2(uz2_2__25),
.i_ux_transmitted(ux_transmitted__25),
.i_uy_transmitted(uy_transmitted__25),

 //outputs

.o_uz_2(uz_2__26),
.o_uz2(uz2__26),
.o_oneMinusUz_2(oneMinusUz_2__26),
.o_sa2_2(sa2_2__26),
.o_uz2_2(uz2_2__26),
.o_ux_transmitted(ux_transmitted__26),
.o_uy_transmitted(uy_transmitted__26)
);

InternalsBlock_Reflector pipeReg25(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__24),
.i_uz2(uz2__24),
.i_oneMinusUz_2(oneMinusUz_2__24),
.i_sa2_2(sa2_2__24),
.i_uz2_2(uz2_2__24),
.i_ux_transmitted(ux_transmitted__24),
.i_uy_transmitted(uy_transmitted__24),

 //outputs

.o_uz_2(uz_2__25),
.o_uz2(uz2__25),
.o_oneMinusUz_2(oneMinusUz_2__25),
.o_sa2_2(sa2_2__25),
.o_uz2_2(uz2_2__25),
.o_ux_transmitted(ux_transmitted__25),
.o_uy_transmitted(uy_transmitted__25)
);

InternalsBlock_Reflector pipeReg24(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__23),
.i_uz2(uz2__23),
.i_oneMinusUz_2(oneMinusUz_2__23),
.i_sa2_2(sa2_2__23),
.i_uz2_2(uz2_2__23),
.i_ux_transmitted(ux_transmitted__23),
.i_uy_transmitted(uy_transmitted__23),

 //outputs

.o_uz_2(uz_2__24),
.o_uz2(uz2__24),
.o_oneMinusUz_2(oneMinusUz_2__24),
.o_sa2_2(sa2_2__24),
.o_uz2_2(uz2_2__24),
.o_ux_transmitted(ux_transmitted__24),
.o_uy_transmitted(uy_transmitted__24)
);

InternalsBlock_Reflector pipeReg23(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__22),
.i_uz2(uz2__22),
.i_oneMinusUz_2(oneMinusUz_2__22),
.i_sa2_2(sa2_2__22),
.i_uz2_2(uz2_2__22),
.i_ux_transmitted(ux_transmitted__22),
.i_uy_transmitted(uy_transmitted__22),

 //outputs

.o_uz_2(uz_2__23),
.o_uz2(uz2__23),
.o_oneMinusUz_2(oneMinusUz_2__23),
.o_sa2_2(sa2_2__23),
.o_uz2_2(uz2_2__23),
.o_ux_transmitted(ux_transmitted__23),
.o_uy_transmitted(uy_transmitted__23)
);

InternalsBlock_Reflector pipeReg22(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__21),
.i_uz2(uz2__21),
.i_oneMinusUz_2(oneMinusUz_2__21),
.i_sa2_2(sa2_2__21),
.i_uz2_2(uz2_2__21),
.i_ux_transmitted(ux_transmitted__21),
.i_uy_transmitted(uy_transmitted__21),

 //outputs

.o_uz_2(uz_2__22),
.o_uz2(uz2__22),
.o_oneMinusUz_2(oneMinusUz_2__22),
.o_sa2_2(sa2_2__22),
.o_uz2_2(uz2_2__22),
.o_ux_transmitted(ux_transmitted__22),
.o_uy_transmitted(uy_transmitted__22)
);

InternalsBlock_Reflector pipeReg21(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__20),
.i_uz2(uz2__20),
.i_oneMinusUz_2(oneMinusUz_2__20),
.i_sa2_2(sa2_2__20),
.i_uz2_2(uz2_2__20),
.i_ux_transmitted(ux_transmitted__20),
.i_uy_transmitted(uy_transmitted__20),

 //outputs

.o_uz_2(uz_2__21),
.o_uz2(uz2__21),
.o_oneMinusUz_2(oneMinusUz_2__21),
.o_sa2_2(sa2_2__21),
.o_uz2_2(uz2_2__21),
.o_ux_transmitted(ux_transmitted__21),
.o_uy_transmitted(uy_transmitted__21)
);

InternalsBlock_Reflector pipeReg20(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__19),
.i_uz2(uz2__19),
.i_oneMinusUz_2(oneMinusUz_2__19),
.i_sa2_2(sa2_2__19),
.i_uz2_2(uz2_2__19),
.i_ux_transmitted(ux_transmitted__19),
.i_uy_transmitted(uy_transmitted__19),

 //outputs

.o_uz_2(uz_2__20),
.o_uz2(uz2__20),
.o_oneMinusUz_2(oneMinusUz_2__20),
.o_sa2_2(sa2_2__20),
.o_uz2_2(uz2_2__20),
.o_ux_transmitted(ux_transmitted__20),
.o_uy_transmitted(uy_transmitted__20)
);

InternalsBlock_Reflector pipeReg19(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__18),
.i_uz2(uz2__18),
.i_oneMinusUz_2(oneMinusUz_2__18),
.i_sa2_2(sa2_2__18),
.i_uz2_2(uz2_2__18),
.i_ux_transmitted(ux_transmitted__18),
.i_uy_transmitted(uy_transmitted__18),

 //outputs

.o_uz_2(uz_2__19),
.o_uz2(uz2__19),
.o_oneMinusUz_2(oneMinusUz_2__19),
.o_sa2_2(sa2_2__19),
.o_uz2_2(uz2_2__19),
.o_ux_transmitted(ux_transmitted__19),
.o_uy_transmitted(uy_transmitted__19)
);

InternalsBlock_Reflector pipeReg18(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__17),
.i_uz2(uz2__17),
.i_oneMinusUz_2(oneMinusUz_2__17),
.i_sa2_2(sa2_2__17),
.i_uz2_2(uz2_2__17),
.i_ux_transmitted(ux_transmitted__17),
.i_uy_transmitted(uy_transmitted__17),

 //outputs

.o_uz_2(uz_2__18),
.o_uz2(uz2__18),
.o_oneMinusUz_2(oneMinusUz_2__18),
.o_sa2_2(sa2_2__18),
.o_uz2_2(uz2_2__18),
.o_ux_transmitted(ux_transmitted__18),
.o_uy_transmitted(uy_transmitted__18)
);

InternalsBlock_Reflector pipeReg17(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__16),
.i_uz2(uz2__16),
.i_oneMinusUz_2(oneMinusUz_2__16),
.i_sa2_2(sa2_2__16),
.i_uz2_2(uz2_2__16),
.i_ux_transmitted(ux_transmitted__16),
.i_uy_transmitted(uy_transmitted__16),

 //outputs

.o_uz_2(uz_2__17),
.o_uz2(uz2__17),
.o_oneMinusUz_2(oneMinusUz_2__17),
.o_sa2_2(sa2_2__17),
.o_uz2_2(uz2_2__17),
.o_ux_transmitted(ux_transmitted__17),
.o_uy_transmitted(uy_transmitted__17)
);
//removed 16

InternalsBlock_Reflector pipeReg15(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__14),
.i_uz2(uz2__14),
.i_oneMinusUz_2(oneMinusUz_2__14),
.i_sa2_2(sa2_2__14),
.i_uz2_2(uz2_2__14),
.i_ux_transmitted(ux_transmitted__14),
.i_uy_transmitted(uy_transmitted__14),

 //outputs

.o_uz_2(uz_2__15),
.o_uz2(uz2__15),
.o_oneMinusUz_2(oneMinusUz_2__15),
.o_sa2_2(sa2_2__15),
.o_uz2_2(uz2_2__15),
.o_ux_transmitted(ux_transmitted__15),
.o_uy_transmitted(uy_transmitted__15)
);

InternalsBlock_Reflector pipeReg14(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__13),
.i_uz2(uz2__13),
.i_oneMinusUz_2(oneMinusUz_2__13),
.i_sa2_2(sa2_2__13),
.i_uz2_2(uz2_2__13),
.i_ux_transmitted(ux_transmitted__13),
.i_uy_transmitted(uy_transmitted__13),

 //outputs

.o_uz_2(uz_2__14),
.o_uz2(uz2__14),
.o_oneMinusUz_2(oneMinusUz_2__14),
.o_sa2_2(sa2_2__14),
.o_uz2_2(uz2_2__14),
.o_ux_transmitted(ux_transmitted__14),
.o_uy_transmitted(uy_transmitted__14)
);

InternalsBlock_Reflector pipeReg13(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__12),
.i_uz2(uz2__12),
.i_oneMinusUz_2(oneMinusUz_2__12),
.i_sa2_2(sa2_2__12),
.i_uz2_2(uz2_2__12),
.i_ux_transmitted(ux_transmitted__12),
.i_uy_transmitted(uy_transmitted__12),

 //outputs

.o_uz_2(uz_2__13),
.o_uz2(uz2__13),
.o_oneMinusUz_2(oneMinusUz_2__13),
.o_sa2_2(sa2_2__13),
.o_uz2_2(uz2_2__13),
.o_ux_transmitted(ux_transmitted__13),
.o_uy_transmitted(uy_transmitted__13)
);

InternalsBlock_Reflector pipeReg12(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__11),
.i_uz2(uz2__11),
.i_oneMinusUz_2(oneMinusUz_2__11),
.i_sa2_2(sa2_2__11),
.i_uz2_2(uz2_2__11),
.i_ux_transmitted(ux_transmitted__11),
.i_uy_transmitted(uy_transmitted__11),

 //outputs

.o_uz_2(uz_2__12),
.o_uz2(uz2__12),
.o_oneMinusUz_2(oneMinusUz_2__12),
.o_sa2_2(sa2_2__12),
.o_uz2_2(uz2_2__12),
.o_ux_transmitted(ux_transmitted__12),
.o_uy_transmitted(uy_transmitted__12)
);

InternalsBlock_Reflector pipeReg11(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__10),
.i_uz2(uz2__10),
.i_oneMinusUz_2(oneMinusUz_2__10),
.i_sa2_2(sa2_2__10),
.i_uz2_2(uz2_2__10),
.i_ux_transmitted(ux_transmitted__10),
.i_uy_transmitted(uy_transmitted__10),

 //outputs

.o_uz_2(uz_2__11),
.o_uz2(uz2__11),
.o_oneMinusUz_2(oneMinusUz_2__11),
.o_sa2_2(sa2_2__11),
.o_uz2_2(uz2_2__11),
.o_ux_transmitted(ux_transmitted__11),
.o_uy_transmitted(uy_transmitted__11)
);

InternalsBlock_Reflector pipeReg10(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__9),
.i_uz2(uz2__9),
.i_oneMinusUz_2(oneMinusUz_2__9),
.i_sa2_2(sa2_2__9),
.i_uz2_2(uz2_2__9),
.i_ux_transmitted(ux_transmitted__9),
.i_uy_transmitted(uy_transmitted__9),

 //outputs

.o_uz_2(uz_2__10),
.o_uz2(uz2__10),
.o_oneMinusUz_2(oneMinusUz_2__10),
.o_sa2_2(sa2_2__10),
.o_uz2_2(uz2_2__10),
.o_ux_transmitted(ux_transmitted__10),
.o_uy_transmitted(uy_transmitted__10)
);

InternalsBlock_Reflector pipeReg9(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__8),
.i_uz2(uz2__8),
.i_oneMinusUz_2(oneMinusUz_2__8),
.i_sa2_2(sa2_2__8),
.i_uz2_2(uz2_2__8),
.i_ux_transmitted(ux_transmitted__8),
.i_uy_transmitted(uy_transmitted__8),

 //outputs

.o_uz_2(uz_2__9),
.o_uz2(uz2__9),
.o_oneMinusUz_2(oneMinusUz_2__9),
.o_sa2_2(sa2_2__9),
.o_uz2_2(uz2_2__9),
.o_ux_transmitted(ux_transmitted__9),
.o_uy_transmitted(uy_transmitted__9)
);

InternalsBlock_Reflector pipeReg8(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__7),
.i_uz2(uz2__7),
.i_oneMinusUz_2(oneMinusUz_2__7),
.i_sa2_2(sa2_2__7),
.i_uz2_2(uz2_2__7),
.i_ux_transmitted(ux_transmitted__7),
.i_uy_transmitted(uy_transmitted__7),

 //outputs

.o_uz_2(uz_2__8),
.o_uz2(uz2__8),
.o_oneMinusUz_2(oneMinusUz_2__8),
.o_sa2_2(sa2_2__8),
.o_uz2_2(uz2_2__8),
.o_ux_transmitted(ux_transmitted__8),
.o_uy_transmitted(uy_transmitted__8)
);

InternalsBlock_Reflector pipeReg7(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__6),
.i_uz2(uz2__6),
.i_oneMinusUz_2(oneMinusUz_2__6),
.i_sa2_2(sa2_2__6),
.i_uz2_2(uz2_2__6),
.i_ux_transmitted(ux_transmitted__6),
.i_uy_transmitted(uy_transmitted__6),

 //outputs

.o_uz_2(uz_2__7),
.o_uz2(uz2__7),
.o_oneMinusUz_2(oneMinusUz_2__7),
.o_sa2_2(sa2_2__7),
.o_uz2_2(uz2_2__7),
.o_ux_transmitted(ux_transmitted__7),
.o_uy_transmitted(uy_transmitted__7)
);

InternalsBlock_Reflector pipeReg6(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__5),
.i_uz2(uz2__5),
.i_oneMinusUz_2(oneMinusUz_2__5),
.i_sa2_2(sa2_2__5),
.i_uz2_2(uz2_2__5),
.i_ux_transmitted(ux_transmitted__5),
.i_uy_transmitted(uy_transmitted__5),

 //outputs

.o_uz_2(uz_2__6),
.o_uz2(uz2__6),
.o_oneMinusUz_2(oneMinusUz_2__6),
.o_sa2_2(sa2_2__6),
.o_uz2_2(uz2_2__6),
.o_ux_transmitted(ux_transmitted__6),
.o_uy_transmitted(uy_transmitted__6)
);


//removed 2,3,4,5


//peter m
//  no driver
assign uz_2__0 = 64'b0;
assign uz2__0 = 32'b0;
assign oneMinusUz_2__0 = 0;
assign sa2_2__0 = 0;
assign uz2_2__0 = 64'b0;
assign ux_transmitted__0 = 32'b00000000000000000000000000000000;
assign uy_transmitted__0 = 32'b00000000000000000000000000000000;


InternalsBlock_Reflector pipeReg1(
//Inputs

.clock(clock),
.reset(reset),
.enable(enable),
.i_uz_2(uz_2__0),
.i_uz2(uz2__0),
.i_oneMinusUz_2(oneMinusUz_2__0),
.i_sa2_2(sa2_2__0),
.i_uz2_2(uz2_2__0),
.i_ux_transmitted(ux_transmitted__0),
.i_uy_transmitted(uy_transmitted__0),

 //outputs

.o_uz_2(uz_2__1),
.o_uz2(uz2__1),
.o_oneMinusUz_2(oneMinusUz_2__1),
.o_sa2_2(sa2_2__1),
.o_uz2_2(uz2_2__1),
.o_ux_transmitted(ux_transmitted__1),
.o_uy_transmitted(uy_transmitted__1)
);




//-------------SYNCHRONOUS LOGIC----------------------
//
//
//
//
//
//
//
//
//
//
//
//
//This is the end of the generate statement, and the beginning of the
//synchronous logic.  On the clock event, the outputs calcu`LATed from
//this block are put on the output pins for reading (registered
//outputs, as per the convention).

//Assign outputs from block on positive clock edge.
always @ (posedge clock) begin
	if(reset_new) begin
		//Reset internal non-pipelined registers here.
		ux_reflector	<= 32'h00000000;
		uy_reflector	<= 32'h00000000;
		uz_reflector	<= 32'h7FFFFFFF;
		layer_reflector	<= 3'b001;
		dead_reflector	<= 1'b1;
	end else if (enable) begin
		ux_reflector	<= new_ux;
		uy_reflector	<= new_uy;
		uz_reflector	<= new_uz;
		layer_reflector <= new_layer;
		dead_reflector	<= new_dead;
	end	
end


//-------------ASYNCHRONOUS LOGIC----------------------
//
//
//
//
//
//
//
//
//
//
//
//
//This is where the asynchronous logic takes place.  Things that
//occur here include setting up wiring to send to the multipliers,
//and square root unit.  Also, products brought in from the wrapper 
//are placed on the appropriate wires for placement in the pipeline.

//-------------MUXES for SYNCHRONOUS LOGIC--------
always @ (i_layer36 or downCritAngle_0 or upCritAngle_0 or
						downCritAngle_1 or upCritAngle_1 or
						downCritAngle_2 or upCritAngle_2 or
						downCritAngle_3 or upCritAngle_3 or
						downCritAngle_4 or upCritAngle_4) begin
	case (i_layer36)
	1:begin
		downCritAngle		=	downCritAngle_0;
		upCritAngle		=	upCritAngle_0;
	end
	2:begin
		downCritAngle		=	downCritAngle_1;
		upCritAngle		=	upCritAngle_1;
	end
	3:begin
		downCritAngle		=	downCritAngle_2;
		upCritAngle		=	upCritAngle_2;
	end
	4:begin
		downCritAngle		=	downCritAngle_3;
		upCritAngle		=	upCritAngle_3;
	end
	5:begin
		downCritAngle		=	downCritAngle_4;
		upCritAngle		=	upCritAngle_4;
	end
	//Should never occur
	default:begin
		downCritAngle		=	downCritAngle_0;
		upCritAngle		=	upCritAngle_0;
	end
	endcase
end

always @ (i_uz35 or i_layer35) begin
	negUz = -i_uz35;
	case (i_uz35[31])
	0: begin
		case (i_layer35)
			1:	fresIndex		=	{3'b000, i_uz35[30:24]};
			2:	fresIndex		=	{3'b001, i_uz35[30:24]};
			3:	fresIndex		=	{3'b010, i_uz35[30:24]};
			4:	fresIndex		=	{3'b011, i_uz35[30:24]};
			5:	fresIndex		=	{3'b100, i_uz35[30:24]};
			//Should never occur
			default: fresIndex		=	{3'b000, i_uz35[30:24]};
		endcase
	end
	1: begin
		case (i_layer35)
			1:	fresIndex		=	{3'b000, negUz[30:24]};
			2:	fresIndex		=	{3'b001, negUz[30:24]};
			3:	fresIndex		=	{3'b010, negUz[30:24]};
			4:	fresIndex		=	{3'b011, negUz[30:24]};
			5:	fresIndex		=	{3'b100, negUz[30:24]};
			//Should never occur
			default: fresIndex		=	{3'b000, negUz[30:24]};
		endcase
	end
	endcase
		
end


//-------------OPERAND SETUP----------------------


//NAMING CONVENTION:
//opX_Y_Z, op stands for operand, X stands for the multiplication number for
//that clock cycle, Y stands for the clock cycle, Z is either 1 or 2 for the
//first or second operand for this multiply
//
//COMMENTING CONVENTIONS:
//CC X means that the values being calcu`LATed will be ready for the Xth register
//location, where 0 is the register prior to any calcu`LATions being done, 1 is
//after the 1st clock cycle of calcu`LATion, etc.

//CC 2
assign	op1_2_1						=	i_uz1;
assign	op1_2_2						=	i_uz1;

//CC 3
//SUBTRACTION, see math results

//CC 4
always @ (i_uz3 or i_layer3 or down_niOverNt_2_1 or up_niOverNt_2_1 or
								down_niOverNt_2_2 or up_niOverNt_2_2 or
								down_niOverNt_2_3 or up_niOverNt_2_3 or
								down_niOverNt_2_4 or up_niOverNt_2_4 or
								down_niOverNt_2_5 or up_niOverNt_2_5) begin
	case (i_uz3[31])
	//uz >= 0
	0:begin
		case (i_layer3)
			1: op1_4_1			=	{down_niOverNt_2_1[63], down_niOverNt_2_1[61:31]};
			2: op1_4_1			=	{down_niOverNt_2_2[63], down_niOverNt_2_2[61:31]};
			3: op1_4_1			=	{down_niOverNt_2_3[63], down_niOverNt_2_3[61:31]};
			4: op1_4_1			=	{down_niOverNt_2_4[63], down_niOverNt_2_4[61:31]};
			5: op1_4_1			=	{down_niOverNt_2_5[63], down_niOverNt_2_5[61:31]};
			default: op1_4_1		=	{down_niOverNt_2_1[63], down_niOverNt_2_1[61:31]};
		endcase
	end
	//uz < 0
	1:begin
		case (i_layer3)
			1: op1_4_1			=	{up_niOverNt_2_1[63], up_niOverNt_2_1[61:31]};
			2: op1_4_1			=	{up_niOverNt_2_2[63], up_niOverNt_2_2[61:31]};
			3: op1_4_1			=	{up_niOverNt_2_3[63], up_niOverNt_2_3[61:31]};
			4: op1_4_1			=	{up_niOverNt_2_4[63], up_niOverNt_2_4[61:31]};
			5: op1_4_1			=	{up_niOverNt_2_5[63], up_niOverNt_2_5[61:31]};
			default: op1_4_1		=	{up_niOverNt_2_1[63], up_niOverNt_2_1[61:31]};
		endcase
	end
	endcase
end

assign	op1_4_2						=	{oneMinusUz_2__3[63], oneMinusUz_2__3[61:31]};

//CC 5
//SUBTRACTION, see math results

//CC `SQRT+5 -- Started in CC 6
assign	sqrtOperand1_6					=	uz2_2__5;

//CC `SQRT+`DIV+6 -- Line up with Scatterer.
assign	op1_36_1					=	i_ux35;

always @ (i_uz35 or i_layer35 or down_niOverNt_1 or up_niOverNt_1 or
								down_niOverNt_2 or up_niOverNt_2 or
								down_niOverNt_3 or up_niOverNt_3 or
								down_niOverNt_4 or up_niOverNt_4 or
								down_niOverNt_5 or up_niOverNt_5) begin
	case (i_uz35[31])
	0: begin//uz >= 0
		case (i_layer35)
			1:begin	
				op1_36_2		=	down_niOverNt_1;
				op2_36_2		=	down_niOverNt_1;
			end
			2:begin	
				op1_36_2		=	down_niOverNt_2;
				op2_36_2		=	down_niOverNt_2;
			end
			3:begin	
				op1_36_2		=	down_niOverNt_3;
				op2_36_2		=	down_niOverNt_3;
			end
			4:begin	
				op1_36_2		=	down_niOverNt_4;
				op2_36_2		=	down_niOverNt_4;
			end
			5:begin	
				op1_36_2		=	down_niOverNt_5;
				op2_36_2		=	down_niOverNt_5;
			end
			default:begin
				op1_36_2		=	down_niOverNt_1;
				op2_36_2		=	down_niOverNt_1;
			end
		endcase
	end
	1: begin//uz < 0
		case (i_layer35)
			1:begin
				op1_36_2		=	up_niOverNt_1;
				op2_36_2		=	up_niOverNt_1;
			end
			2:begin
				op1_36_2		=	up_niOverNt_2;
				op2_36_2		=	up_niOverNt_2;
			end
			3:begin
				op1_36_2		=	up_niOverNt_3;
				op2_36_2		=	up_niOverNt_3;
			end
			4:begin
				op1_36_2		=	up_niOverNt_4;
				op2_36_2		=	up_niOverNt_4;
			end
			5:begin
				op1_36_2		=	up_niOverNt_5;
				op2_36_2		=	up_niOverNt_5;
			end
			default:begin
				op1_36_2		=	up_niOverNt_1;
				op2_36_2		=	up_niOverNt_1;
			end
		endcase
	end
	endcase
end

assign	op2_36_1					=	i_uy35;





//-------------MATH RESULTS----------------------


//NAMING CONVENTION:
//new_VAR means that the variable named VAR will be stored into the register
//pipeline at the clock cycle indicated by the comments above it.
//
//prod stands for product, quot stands for quotient, `SQRT stands for square root
//prodX_Y means the Xth product which started calcu`LATion at the Yth clock cycle
//Similarly for quot and `SQRTResult.
//
//
//COMMENTING CONVENTIONS:
//CC X means that the values being calcu`LATed will be ready for the Xth register
//location, where 0 is the register prior to any calcu`LATions being done, 1 is
//after the 1st clock cycle of calcu`LATion, etc.


//CC 2
assign new_uz_2						=	prod1_2;

//CC 3
sub_64b		oneMinusUz2_sub(
			.dataa(`INTMAX_2_ref),
			.datab(uz_2__2),
			.result(new_oneMinusUz_2)
		);

//CC 4
//Used to determine whether or not the multiply operation overflowed.
//or U1(overflow1_4, prod1_4[62], prod1_4[61], prod1_4[60], prod1_4[59], prod1_4[58]);
assign overflow1_4 = prod1_4[62]|prod1_4[61]|prod1_4[60]|prod1_4[59]|prod1_4[58];

//Cannot take `SQRT of negative number, that is why prod1_4[58] must be 0.

													//sign		//data		//padding
assign	new_sa2_2					=	(overflow1_4 == 1)? `INTMAX_2_ref	:	{prod1_4[63], prod1_4[58:0], 4'h0};

//5th CC
sub_64b		uz2_2_sub(
			.dataa(`INTMAX_2_ref),
			.datab(sa2_2__4),
			.result(new_uz2_2)
		);

//CC `SQRT+5
assign new_uz2						= sqrtResult1_6;

//CC `SQRT+`DIV+6 -- Line up with Scatterer.


//Used to determine whether or not the multiply operation overflowed.
//or U2(toAnd1_36_1, prod1_36[62], prod1_36[61], prod1_36[60]);
assign toAnd1_36_1 = prod1_36[62]|prod1_36[61]|prod1_36[60];
//Used to determine whether or not the multiply operation overflowed in the negative direction.
//or U3(toAnd1_36_2, ~prod1_36[62], ~prod1_36[61], ~prod1_36[60]);
assign toAnd1_36_2 = ~prod1_36[62]|~prod1_36[61]|~prod1_36[60];

//and U4(overflow1_36, ~prod1_36[63], toAnd1_36_1);
assign overflow1_36 = ~prod1_36[63] & toAnd1_36_1;
//and U5(negOverflow1_36, prod1_36[63], toAnd1_36_2);
assign negOverflow1_36 = prod1_36[63] & toAnd1_36_2;


//Used to determine whether or not the multiply operation overflowed.
//or U6(toAnd2_36_1, prod2_36[62], prod2_36[61], prod2_36[60]);
assign toAnd2_36_1 = prod2_36[62]|prod2_36[61]|prod2_36[60];
//Used to determine whether or not the multiply operation overflowed in the negative direction.
//or U7(toAnd2_36_2, ~prod2_36[62], ~prod2_36[61], ~prod2_36[60]);
assign toAnd2_36_2 = ~prod2_36[62]|~prod2_36[61]|~prod2_36[60];


//and U8(overflow2_36, ~prod2_36[63], toAnd2_36_1);
assign overflow2_36 = ~prod2_36[63] & toAnd2_36_1;
//and U9(negOverflow2_36, prod2_36[63], toAnd2_36_2);
assign negOverflow2_36 = prod2_36[63] & toAnd2_36_2;

always @ (overflow1_36 or negOverflow1_36 or prod1_36 or
		  overflow2_36 or negOverflow2_36 or prod2_36) begin
	case ({overflow1_36, negOverflow1_36})
	0:	new_ux_transmitted = {prod1_36[63:63], prod1_36[59:29]};
	1:	new_ux_transmitted = `INTMIN;
	2:	new_ux_transmitted = `INTMAX;
	//Should never occur
	3:	new_ux_transmitted = {prod1_36[63:63], prod1_36[59:29]};
	endcase
	
	case ({overflow2_36, negOverflow2_36})
	
	0:	new_uy_transmitted = {prod2_36[63:63], prod2_36[59:29]};
	1:	new_uy_transmitted = `INTMIN;
	2:	new_uy_transmitted = `INTMAX;
	//Should never occur
	3:	new_uy_transmitted = {prod2_36[63:63], prod2_36[59:29]};
	endcase
end


//-------------FINAL CALCU`LATED VALUES----------------------
//
//
//
//
//
//
//
//
//
//
//
//
//
//
always @ (i_uz36 or downCritAngle or upCritAngle or down_rFresnel or i_ux36 or
			i_uy36 or i_layer36 or i_dead36 or rnd or up_rFresnel or ux_transmitted__37 or 
			uy_transmitted__37 or uz2__37) begin
	//REFLECTED -- Due to total internal reflection while moving down
	if (~i_uz36[31] && i_uz36 <= downCritAngle) begin
		new_ux		= i_ux36;
		new_uy		= i_uy36;
		new_uz		= -i_uz36;
		new_layer	= i_layer36;
		new_dead	= i_dead36;
	//REFLECTED -- Due to total internal reflection while moving up
	end else if (i_uz36[31] && -i_uz36 <= upCritAngle) begin
		new_ux		= i_ux36;
		new_uy		= i_uy36;
		new_uz		= -i_uz36;
		new_layer	= i_layer36;
		new_dead	= i_dead36;
	//REFLECTED -- Due to random number being too small while moving down
	end else if (~i_uz36[31] && rnd <= down_rFresnel) begin
		new_ux		= i_ux36;
		new_uy		= i_uy36;
		new_uz		= -i_uz36;
		new_layer	= i_layer36;
		new_dead	= i_dead36;
	//REFLECTED -- Due to random number being too small while moving up
	end else if (i_uz36[31] && rnd <= up_rFresnel) begin
		new_ux		= i_ux36;
		new_uy		= i_uy36;
		new_uz		= -i_uz36;
		new_layer	= i_layer36;
		new_dead	= i_dead36;
	//TRANSMITTED
	end else begin
		new_ux		= ux_transmitted__37;
		new_uy		= uy_transmitted__37;
		case (i_uz36[31])
		0:begin//uz >= 0
			if (i_layer36 == 5) begin
				new_layer	= 3'h5;
				new_dead	= 1'b1;
			end else begin
				new_layer	= i_layer36+3'h1;
				new_dead	= i_dead36;
			end
			new_uz			= uz2__37;
		end
		1:begin//uz < 0
			if (i_layer36 == 1) begin
				new_layer	= 3'h1;
				new_dead	= 1'b1;
			end else begin
				new_layer	= i_layer36-3'h1;
				new_dead	= i_dead36;
			end
			new_uz			= -uz2__37;
		end
		endcase
	
	end
end

endmodule


module Memory_Wrapper (
	//Inputs
	clock,
//	reset, //This is unused in the module. ODIN II complains.
	pindex,
	//Outputs
	sinp,
	cosp
	);


input					clock;
//input					reset;
input	[9:0]			pindex;


output	[31:0]			sinp;
output	[31:0]			cosp;

//sinp_ROM sinp_MEM (.address(pindex), .clock(clock), .q(sinp));
//cosp_ROM cosp_MEM (.address(pindex), .clock(clock), .q(cosp));

//Instantiate a single port ram for odin
wire [31:0]blank;
assign blank = 32'b000000000000000000000000000000;
single_port_ram sinp_replace(.clk (clock), .addr (pindex), .data (blank), .we (1'b0), .out (sinp));
single_port_ram cosp_replace(.clk (clock), .addr (pindex), .data (blank), .we (1'b0), .out (cosp));

			
endmodule


module InternalsBlock(
	//Inputs
	clock,
	reset,
	enable,
	
	i_sint,
	i_cost,
	i_sinp,
	i_cosp,
	i_sintCosp,
	i_sintSinp,
	i_uz2,
	i_uxUz,
	i_uyUz,
	i_uySintSinp,
	i_oneMinusUz2,
	i_uyUzSintCosp,
	i_uxUzSintCosp,
	i_uxSintSinp,
	i_sqrtOneMinusUz2,
	i_sintCospSqrtOneMinusUz2,
	i_uxCost,
	i_uzCost,
	i_sqrtOneMinusUz2_inv,
	i_uxNumerator,
	i_uyNumerator,
	i_uyCost,
	i_uxQuotient,
	i_uyQuotient,
	//Outputs
	o_sint,
	o_cost,
	o_sinp,
	o_cosp,
	o_sintCosp,
	o_sintSinp,
	o_uz2,
	o_uxUz,
	o_uyUz,
	o_uySintSinp,
	o_oneMinusUz2,
	o_uyUzSintCosp,
	o_uxUzSintCosp,
	o_uxSintSinp,
	o_sqrtOneMinusUz2,
	o_sintCospSqrtOneMinusUz2,
	o_uxCost,
	o_uzCost,
	o_sqrtOneMinusUz2_inv,
	o_uxNumerator,
	o_uyNumerator,
	o_uyCost,
	o_uxQuotient,
	o_uyQuotient
	);

input					clock;
input					reset;
input					enable;

input		[31:0]		i_sint;
input		[31:0]		i_cost;
input		[31:0]		i_sinp;
input		[31:0]		i_cosp;
input		[31:0]		i_sintCosp;
input		[31:0]		i_sintSinp;
input		[63:0]		i_uz2;
input		[31:0]		i_uxUz;
input		[31:0]		i_uyUz;
input		[31:0]		i_uySintSinp;
input		[63:0]		i_oneMinusUz2;
input		[31:0]		i_uyUzSintCosp;
input		[31:0]		i_uxUzSintCosp;
input		[31:0]		i_uxSintSinp;
input		[31:0]		i_sqrtOneMinusUz2;
input		[31:0]		i_sintCospSqrtOneMinusUz2;
input		[31:0]		i_uxCost;
input		[31:0]		i_uzCost;
input		[31:0]		i_sqrtOneMinusUz2_inv;
input		[31:0]		i_uxNumerator;
input		[31:0]		i_uyNumerator;
input		[31:0]		i_uyCost;
input		[31:0]		i_uxQuotient;
input		[31:0]		i_uyQuotient;


output		[31:0]		o_sint;
output		[31:0]		o_cost;
output		[31:0]		o_sinp;
output		[31:0]		o_cosp;
output		[31:0]		o_sintCosp;
output		[31:0]		o_sintSinp;
output		[63:0]		o_uz2;
output		[31:0]		o_uxUz;
output		[31:0]		o_uyUz;
output		[31:0]		o_uySintSinp;
output		[63:0]		o_oneMinusUz2;
output		[31:0]		o_uyUzSintCosp;
output		[31:0]		o_uxUzSintCosp;
output		[31:0]		o_uxSintSinp;
output		[31:0]		o_sqrtOneMinusUz2;
output		[31:0]		o_sintCospSqrtOneMinusUz2;
output		[31:0]		o_uxCost;
output		[31:0]		o_uzCost;
output		[31:0]		o_sqrtOneMinusUz2_inv;
output		[31:0]		o_uxNumerator;
output		[31:0]		o_uyNumerator;
output		[31:0]		o_uyCost;
output		[31:0]		o_uxQuotient;
output		[31:0]		o_uyQuotient;


wire					clock;
wire					reset;
wire					enable;

wire		[31:0]		i_sint;
wire		[31:0]		i_cost;
wire		[31:0]		i_sinp;
wire		[31:0]		i_cosp;
wire		[31:0]		i_sintCosp;
wire		[31:0]		i_sintSinp;
wire		[63:0]		i_uz2;
wire		[31:0]		i_uxUz;
wire		[31:0]		i_uyUz;
wire		[31:0]		i_uySintSinp;
wire		[63:0]		i_oneMinusUz2;
wire		[31:0]		i_uyUzSintCosp;
wire		[31:0]		i_uxUzSintCosp;
wire		[31:0]		i_uxSintSinp;
wire		[31:0]		i_sqrtOneMinusUz2;
wire		[31:0]		i_sintCospSqrtOneMinusUz2;
wire		[31:0]		i_uxCost;
wire		[31:0]		i_uzCost;
wire		[31:0]		i_sqrtOneMinusUz2_inv;
wire		[31:0]		i_uxNumerator;
wire		[31:0]		i_uyNumerator;
wire		[31:0]		i_uyCost;
wire		[31:0]		i_uxQuotient;
wire		[31:0]		i_uyQuotient;


reg			[31:0]		o_sint;
reg			[31:0]		o_cost;
reg			[31:0]		o_sinp;
reg			[31:0]		o_cosp;
reg			[31:0]		o_sintCosp;
reg			[31:0]		o_sintSinp;
reg			[63:0]		o_uz2;
reg			[31:0]		o_uxUz;
reg			[31:0]		o_uyUz;
reg			[31:0]		o_uySintSinp;
reg			[63:0]		o_oneMinusUz2;
reg			[31:0]		o_uyUzSintCosp;
reg			[31:0]		o_uxUzSintCosp;
reg			[31:0]		o_uxSintSinp;
reg			[31:0]		o_sqrtOneMinusUz2;
reg			[31:0]		o_sintCospSqrtOneMinusUz2;
reg			[31:0]		o_uxCost;
reg			[31:0]		o_uzCost;
reg			[31:0]		o_sqrtOneMinusUz2_inv;
reg			[31:0]		o_uxNumerator;
reg			[31:0]		o_uyNumerator;
reg			[31:0]		o_uyCost;
reg			[31:0]		o_uxQuotient;
reg			[31:0]		o_uyQuotient;




always @ (posedge clock)
	if(reset) begin
		o_sint						<= 32'h00000000;
		o_cost						<= 32'h00000000;
		o_sinp						<= 32'h00000000;
		o_cosp						<= 32'h00000000;
		o_sintCosp					<= 32'h00000000;
		o_sintSinp					<= 32'h00000000;
		o_uz2						<= 64'h0000000000000000;
		o_uxUz						<= 32'h00000000;
		o_uyUz						<= 32'h00000000;
		o_uySintSinp				<= 32'h00000000;
		o_oneMinusUz2				<= 64'h0000000000000000;
		o_uyUzSintCosp				<= 32'h00000000;
		o_uxUzSintCosp				<= 32'h00000000;
		o_uxSintSinp				<= 32'h00000000;
		o_sqrtOneMinusUz2			<= 32'h00000000;
		o_sintCospSqrtOneMinusUz2	<= 32'h00000000;
		o_uxCost					<= 32'h00000000;
		o_uzCost					<= 32'h00000000;
		o_sqrtOneMinusUz2_inv		<= 32'h00000000;
		o_uxNumerator				<= 32'h00000000;
		o_uyNumerator				<= 32'h00000000;
		o_uyCost					<= 32'h00000000;
		o_uxQuotient				<= 32'h00000000;
		o_uyQuotient				<= 32'h00000000;
	end else if(enable) begin
		o_sint						<= i_sint;
		o_cost						<= i_cost;
		o_sinp						<= i_sinp;
		o_cosp						<= i_cosp;
		o_sintCosp					<= i_sintCosp;
		o_sintSinp					<= i_sintSinp;
		o_uz2						<= i_uz2;
		o_uxUz						<= i_uxUz;
		o_uyUz						<= i_uyUz;
		o_uySintSinp				<= i_uySintSinp;
		o_oneMinusUz2				<= i_oneMinusUz2;
		o_uyUzSintCosp				<= i_uyUzSintCosp;
		o_uxUzSintCosp				<= i_uxUzSintCosp;
		o_uxSintSinp				<= i_uxSintSinp;
		o_sqrtOneMinusUz2			<= i_sqrtOneMinusUz2;
		o_sintCospSqrtOneMinusUz2	<= i_sintCospSqrtOneMinusUz2;
		o_uxCost					<= i_uxCost;
		o_uzCost					<= i_uzCost;
		o_sqrtOneMinusUz2_inv		<= i_sqrtOneMinusUz2_inv;
		o_uxNumerator				<= i_uxNumerator;
		o_uyNumerator				<= i_uyNumerator;
		o_uyCost					<= i_uyCost;
		o_uxQuotient				<= i_uxQuotient;
		o_uyQuotient				<= i_uyQuotient;
	end
endmodule


module Scatterer (
	//INPUTS
	clock,
	reset,
	enable,
	//Values from Photon Pipeline
	i_uz1,
	i_ux3,
	i_uz3,
	i_uy32,
	i_uz32,
	i_ux33,
	i_uy33,
	i_ux35,
	i_uy35,
	i_uz35,
	i_uz36,

	//Mathematics Results
	prod1_2,
	prod1_4,
	sqrtResult1_6,
	prod1_33,
	prod2_33,
	prod3_33,
	prod1_34,
	prod2_34,
	prod3_34,
	prod4_34,
	quot1_16,
	prod1_36,
	prod2_36,
	prod3_36,
	prod4_36,
	prod5_36,
	prod6_36,

	//Trig from Memory
	sint_Mem,
	cost_Mem,
	sinp_Mem,
	cosp_Mem,
	
	//OUTPUTS
	op1_2_1,
	op1_2_2,
	op1_4_1,
	op1_4_2,
	sqrtOperand1_6,
	divNumerator1_16,
	divDenominator1_16,
	op1_33_1,
	op1_33_2,
	op2_33_1,
	op2_33_2,
	op3_33_1,
	op3_33_2,
	op1_34_1,
	op1_34_2,
	op2_34_1,
	op2_34_2,
	op3_34_1,
	op3_34_2,
	op4_34_1,
	op4_34_2,
	op1_36_1,
	op1_36_2,
	op2_36_1,
	op2_36_2,
	op3_36_1,
	op3_36_2,
	op4_36_1,
	op4_36_2,
	op5_36_1,
	op5_36_2,
	op6_36_1,
	op6_36_2,
	
	//Final calculated values
	ux_scatterer,
	uy_scatterer,
	uz_scatterer
	
	
	);
	
//-------------------PARAMETER DEFINITION----------------------
//
//
//
//
//
//
//Assign values to parameters used later in the program.
	
//parameter DIV = 20;
//parameter SQRT = 10;
//parameter LAT = DIV + SQRT + 7;
//parameter `INTMAX_2 = 64'h3FFFFFFF00000001;
//parameter `INTMAX = 2147483647;
//parameter `INTMIN = -2147483647;
//parameter `INTMAXMinus3 = 2147483644;
//parameter neg`INTMAXPlus3 = -2147483644;



//-----------------------------PIN DECLARATION----------------------
//
//
//
//
//
//
//
//
//Assign appropriate types to pins (input or output).

input				clock;
input				reset;
input				enable;
//Values from Photon Pipeline
input	[31:0]		i_uz1;
input	[31:0]		i_ux3;
input	[31:0]		i_uz3;
input	[31:0]		i_uy32;
input	[31:0]		i_uz32;
input	[31:0]		i_ux33;
input	[31:0]		i_uy33;
input	[31:0]		i_ux35;
input	[31:0]		i_uy35;
input	[31:0]		i_uz35;
input	[31:0]		i_uz36;

//Multiplication Results
input	[63:0]		prod1_2;
input	[31:0]		prod1_4;
input	[31:0]		sqrtResult1_6;
input	[31:0]		prod1_33;
input	[31:0]		prod2_33;
input	[31:0]		prod3_33;
input	[31:0]		prod1_34;
input	[31:0]		prod2_34;
input	[31:0]		prod3_34;
input	[31:0]		prod4_34;
input	[63:0]		quot1_16;
//Need all 64-bits for these two to detect overflows
input	[63:0]		prod1_36;
input	[63:0]		prod2_36;
input	[31:0]		prod3_36;
input	[31:0]		prod4_36;
input	[31:0]		prod5_36;
input	[31:0]		prod6_36;


//Trig Values from Memory
input	[31:0]		sint_Mem;
input	[31:0]		cost_Mem;
input	[31:0]		sinp_Mem;
input	[31:0]		cosp_Mem;

output	[31:0]		op1_2_1;
output	[31:0]		op1_2_2;
output	[31:0]		op1_4_1;
output	[31:0]		op1_4_2;
output	[63:0]		sqrtOperand1_6;
output	[63:0]		divNumerator1_16;
output	[31:0]		divDenominator1_16;
output	[31:0]		op1_33_1;
output	[31:0]		op1_33_2;
output	[31:0]		op2_33_1;
output	[31:0]		op2_33_2;
output	[31:0]		op3_33_1;
output	[31:0]		op3_33_2;
output	[31:0]		op1_34_1;
output	[31:0]		op1_34_2;
output	[31:0]		op2_34_1;
output	[31:0]		op2_34_2;
output	[31:0]		op3_34_1;
output	[31:0]		op3_34_2;
output	[31:0]		op4_34_1;
output	[31:0]		op4_34_2;
output	[31:0]		op1_36_1;
output	[31:0]		op1_36_2;
output	[31:0]		op2_36_1;
output	[31:0]		op2_36_2;
output	[31:0]		op3_36_1;
output	[31:0]		op3_36_2;
output	[31:0]		op4_36_1;
output	[31:0]		op4_36_2;
output	[31:0]		op5_36_1;
output	[31:0]		op5_36_2;
output	[31:0]		op6_36_1;
output	[31:0]		op6_36_2;

//Final Calculated Results
output	[31:0]		ux_scatterer;
output	[31:0]		uy_scatterer;
output	[31:0]		uz_scatterer;


//-----------------------------PIN TYPES-----------------------------
//
//
//
//
//
//
//
//
//Assign pins to be wires or regs.


wire				clock;
wire				reset;
wire				enable;
//Values from Photon Pipeline
wire	[31:0]		i_uz1;
wire	[31:0]		i_ux3;
wire	[31:0]		i_uz3;
wire	[31:0]		i_uy32;
wire	[31:0]		i_uz32;
wire	[31:0]		i_ux33;
wire	[31:0]		i_uy33;
wire	[31:0]		i_ux35;
wire	[31:0]		i_uy35;
wire	[31:0]		i_uz35;
wire	[31:0]		i_uz36;

//Multiplication Results
wire	[63:0]		prod1_2;
wire	[31:0]		prod1_4;
wire	[31:0]		sqrtResult1_6;
wire	[31:0]		prod1_33;
wire	[31:0]		prod2_33;
wire	[31:0]		prod3_33;
wire	[31:0]		prod1_34;
wire	[31:0]		prod2_34;
wire	[31:0]		prod3_34;
wire	[31:0]		prod4_34;
wire	[63:0]		quot1_16;
wire	[63:0]		prod1_36;
wire	[63:0]		prod2_36;
wire	[31:0]		prod3_36;
wire	[31:0]		prod4_36;
wire	[31:0]		prod5_36;
wire	[31:0]		prod6_36;


//Trig Values from Memory
wire	[31:0]		sint_Mem;
wire	[31:0]		cost_Mem;
wire	[31:0]		sinp_Mem;
wire	[31:0]		cosp_Mem;

//Operands for shared resources
wire	[31:0]		op1_2_1;
wire	[31:0]		op1_2_2;
wire	[31:0]		op1_4_1;
wire	[31:0]		op1_4_2;
wire	[63:0]		sqrtOperand1_6;
wire	[63:0]		divNumerator1_16;
wire	[31:0]		divDenominator1_16;
wire	[31:0]		op1_33_1;
wire	[31:0]		op1_33_2;
wire	[31:0]		op2_33_1;
wire	[31:0]		op2_33_2;
wire	[31:0]		op3_33_1;
wire	[31:0]		op3_33_2;
wire	[31:0]		op1_34_1;
wire	[31:0]		op1_34_2;
wire	[31:0]		op2_34_1;
wire	[31:0]		op2_34_2;
wire	[31:0]		op3_34_1;
wire	[31:0]		op3_34_2;
wire	[31:0]		op4_34_1;
wire	[31:0]		op4_34_2;
wire	[31:0]		op1_36_1;
wire	[31:0]		op1_36_2;
wire	[31:0]		op2_36_1;
wire	[31:0]		op2_36_2;
wire	[31:0]		op3_36_1;
wire	[31:0]		op3_36_2;
wire	[31:0]		op4_36_1;
wire	[31:0]		op4_36_2;
wire	[31:0]		op5_36_1;
wire	[31:0]		op5_36_2;
wire	[31:0]		op6_36_1;
wire	[31:0]		op6_36_2;

//Final outputs
reg		[31:0]		ux_scatterer;
reg		[31:0]		uy_scatterer;
reg		[31:0]		uz_scatterer;


//Need this to deal with 'unused' inputs for ODIN II
wire [63:0]bigOr;
assign bigOr = quot1_16|prod1_36|prod2_36|({32'hFFFFFFFF,32'hFFFFFFFF});
wire reset_new;
assign reset_new = reset & bigOr[63] & bigOr[62] & bigOr[61] & bigOr[60] & bigOr[59] & bigOr[58] & bigOr[57] & bigOr[56] & bigOr[55] & bigOr[54] & bigOr[53] & bigOr[52] & bigOr[51] & bigOr[50] & bigOr[49] & bigOr[48] & bigOr[47] & bigOr[46] & bigOr[45] & bigOr[44] & bigOr[43] & bigOr[42] & bigOr[41] & bigOr[40] & bigOr[39] & bigOr[38] & bigOr[37] & bigOr[36] & bigOr[35] & bigOr[34] & bigOr[33] & bigOr[32] & bigOr[31] & bigOr[30] & bigOr[29] & bigOr[28] & bigOr[27] & bigOr[26] & bigOr[25] & bigOr[24] & bigOr[23] & bigOr[22] & bigOr[21] & bigOr[20] & bigOr[19] & bigOr[18] & bigOr[17] & bigOr[16] & bigOr[15] & bigOr[14] & bigOr[13] & bigOr[12] & bigOr[11] & bigOr[10] & bigOr[9] & bigOr[8] & bigOr[7] & bigOr[6] & bigOr[5] & bigOr[4] & bigOr[3] & bigOr[2] & bigOr[1] & bigOr[0];
 

//-----------------------------END Pin Types-------------------------



//Wires to Connect to Internal Registers
//wire		[31:0]		sint[`LAT:0];
//wire		[31:0]		cost[`LAT:0];
//wire		[31:0]		sinp[`LAT:0];
//wire		[31:0]		cosp[`LAT:0];
//wire		[31:0]		sintCosp[`LAT:0];
//wire		[31:0]		sintSinp[`LAT:0];
//wire		[63:0]		uz2[`LAT:0];
//wire		[31:0]		uxUz[`LAT:0];
//wire		[31:0]		uyUz[`LAT:0];
//wire		[31:0]		uySintSinp[`LAT:0];
//wire		[63:0]		oneMinusUz2[`LAT:0];
//wire		[31:0]		uyUzSintCosp[`LAT:0];
//wire		[31:0]		uxUzSintCosp[`LAT:0];
//wire		[31:0]		uxSintSinp[`LAT:0];
//wire		[31:0]		sqrtOneMinusUz2[`LAT:0];
//wire		[31:0]		sintCospSqrtOneMinusUz2[`LAT:0];
//wire		[31:0]		uxCost[`LAT:0];
//wire		[31:0]		uzCost[`LAT:0];
//wire		[31:0]		sqrtOneMinusUz2_inv[`LAT:0];
//wire		[31:0]		uxNumerator[`LAT:0];
//wire		[31:0]		uyNumerator[`LAT:0];
//wire		[31:0]		uyCost[`LAT:0];
//wire		[31:0]		uxQuotient[`LAT:0];
//wire		[31:0]		uyQuotient[`LAT:0];
//wire		[31:0]		sint[37:0];
wire	[32-1:0]				sint__0;
wire	[32-1:0]				sint__1;
wire	[32-1:0]				sint__2;
wire	[32-1:0]				sint__3;
wire	[32-1:0]				sint__4;
wire	[32-1:0]				sint__5;
wire	[32-1:0]				sint__6;
wire	[32-1:0]				sint__7;
wire	[32-1:0]				sint__8;
wire	[32-1:0]				sint__9;
wire	[32-1:0]				sint__10;
wire	[32-1:0]				sint__11;
wire	[32-1:0]				sint__12;
wire	[32-1:0]				sint__13;
wire	[32-1:0]				sint__14;
wire	[32-1:0]				sint__15;
wire	[32-1:0]				sint__16;
wire	[32-1:0]				sint__17;
wire	[32-1:0]				sint__18;
wire	[32-1:0]				sint__19;
wire	[32-1:0]				sint__20;
wire	[32-1:0]				sint__21;
wire	[32-1:0]				sint__22;
wire	[32-1:0]				sint__23;
wire	[32-1:0]				sint__24;
wire	[32-1:0]				sint__25;
wire	[32-1:0]				sint__26;
wire	[32-1:0]				sint__27;
wire	[32-1:0]				sint__28;
wire	[32-1:0]				sint__29;
wire	[32-1:0]				sint__30;
wire	[32-1:0]				sint__31;
wire	[32-1:0]				sint__32;
wire	[32-1:0]				sint__33;
wire	[32-1:0]				sint__34;
wire	[32-1:0]				sint__35;
wire	[32-1:0]				sint__36;
wire	[32-1:0]				sint__37;





//wire		[31:0]		cost[37:0];


wire	[32-1:0]				cost__0;
wire	[32-1:0]				cost__1;
wire	[32-1:0]				cost__2;
wire	[32-1:0]				cost__3;
wire	[32-1:0]				cost__4;
wire	[32-1:0]				cost__5;
wire	[32-1:0]				cost__6;
wire	[32-1:0]				cost__7;
wire	[32-1:0]				cost__8;
wire	[32-1:0]				cost__9;
wire	[32-1:0]				cost__10;
wire	[32-1:0]				cost__11;
wire	[32-1:0]				cost__12;
wire	[32-1:0]				cost__13;
wire	[32-1:0]				cost__14;
wire	[32-1:0]				cost__15;
wire	[32-1:0]				cost__16;
wire	[32-1:0]				cost__17;
wire	[32-1:0]				cost__18;
wire	[32-1:0]				cost__19;
wire	[32-1:0]				cost__20;
wire	[32-1:0]				cost__21;
wire	[32-1:0]				cost__22;
wire	[32-1:0]				cost__23;
wire	[32-1:0]				cost__24;
wire	[32-1:0]				cost__25;
wire	[32-1:0]				cost__26;
wire	[32-1:0]				cost__27;
wire	[32-1:0]				cost__28;
wire	[32-1:0]				cost__29;
wire	[32-1:0]				cost__30;
wire	[32-1:0]				cost__31;
wire	[32-1:0]				cost__32;
wire	[32-1:0]				cost__33;
wire	[32-1:0]				cost__34;
wire	[32-1:0]				cost__35;
wire	[32-1:0]				cost__36;
wire	[32-1:0]				cost__37;


//wire		[31:0]		sinp[37:0];


wire	[32-1:0]				sinp__0;
wire	[32-1:0]				sinp__1;
wire	[32-1:0]				sinp__2;
wire	[32-1:0]				sinp__3;
wire	[32-1:0]				sinp__4;
wire	[32-1:0]				sinp__5;
wire	[32-1:0]				sinp__6;
wire	[32-1:0]				sinp__7;
wire	[32-1:0]				sinp__8;
wire	[32-1:0]				sinp__9;
wire	[32-1:0]				sinp__10;
wire	[32-1:0]				sinp__11;
wire	[32-1:0]				sinp__12;
wire	[32-1:0]				sinp__13;
wire	[32-1:0]				sinp__14;
wire	[32-1:0]				sinp__15;
wire	[32-1:0]				sinp__16;
wire	[32-1:0]				sinp__17;
wire	[32-1:0]				sinp__18;
wire	[32-1:0]				sinp__19;
wire	[32-1:0]				sinp__20;
wire	[32-1:0]				sinp__21;
wire	[32-1:0]				sinp__22;
wire	[32-1:0]				sinp__23;
wire	[32-1:0]				sinp__24;
wire	[32-1:0]				sinp__25;
wire	[32-1:0]				sinp__26;
wire	[32-1:0]				sinp__27;
wire	[32-1:0]				sinp__28;
wire	[32-1:0]				sinp__29;
wire	[32-1:0]				sinp__30;
wire	[32-1:0]				sinp__31;
wire	[32-1:0]				sinp__32;
wire	[32-1:0]				sinp__33;
wire	[32-1:0]				sinp__34;
wire	[32-1:0]				sinp__35;
wire	[32-1:0]				sinp__36;
wire	[32-1:0]				sinp__37;


//wire		[31:0]		cosp[37:0];


wire	[32-1:0]				cosp__0;
wire	[32-1:0]				cosp__1;
wire	[32-1:0]				cosp__2;
wire	[32-1:0]				cosp__3;
wire	[32-1:0]				cosp__4;
wire	[32-1:0]				cosp__5;
wire	[32-1:0]				cosp__6;
wire	[32-1:0]				cosp__7;
wire	[32-1:0]				cosp__8;
wire	[32-1:0]				cosp__9;
wire	[32-1:0]				cosp__10;
wire	[32-1:0]				cosp__11;
wire	[32-1:0]				cosp__12;
wire	[32-1:0]				cosp__13;
wire	[32-1:0]				cosp__14;
wire	[32-1:0]				cosp__15;
wire	[32-1:0]				cosp__16;
wire	[32-1:0]				cosp__17;
wire	[32-1:0]				cosp__18;
wire	[32-1:0]				cosp__19;
wire	[32-1:0]				cosp__20;
wire	[32-1:0]				cosp__21;
wire	[32-1:0]				cosp__22;
wire	[32-1:0]				cosp__23;
wire	[32-1:0]				cosp__24;
wire	[32-1:0]				cosp__25;
wire	[32-1:0]				cosp__26;
wire	[32-1:0]				cosp__27;
wire	[32-1:0]				cosp__28;
wire	[32-1:0]				cosp__29;
wire	[32-1:0]				cosp__30;
wire	[32-1:0]				cosp__31;
wire	[32-1:0]				cosp__32;
wire	[32-1:0]				cosp__33;
wire	[32-1:0]				cosp__34;
wire	[32-1:0]				cosp__35;
wire	[32-1:0]				cosp__36;
wire	[32-1:0]				cosp__37;


//wire		[31:0]		sintCosp[37:0];

wire	[32-1:0]				sintCosp__0;
wire	[32-1:0]				sintCosp__1;
wire	[32-1:0]				sintCosp__2;
wire	[32-1:0]				sintCosp__3;
wire	[32-1:0]				sintCosp__4;
wire	[32-1:0]				sintCosp__5;
wire	[32-1:0]				sintCosp__6;
wire	[32-1:0]				sintCosp__7;
wire	[32-1:0]				sintCosp__8;
wire	[32-1:0]				sintCosp__9;
wire	[32-1:0]				sintCosp__10;
wire	[32-1:0]				sintCosp__11;
wire	[32-1:0]				sintCosp__12;
wire	[32-1:0]				sintCosp__13;
wire	[32-1:0]				sintCosp__14;
wire	[32-1:0]				sintCosp__15;
wire	[32-1:0]				sintCosp__16;
wire	[32-1:0]				sintCosp__17;
wire	[32-1:0]				sintCosp__18;
wire	[32-1:0]				sintCosp__19;
wire	[32-1:0]				sintCosp__20;
wire	[32-1:0]				sintCosp__21;
wire	[32-1:0]				sintCosp__22;
wire	[32-1:0]				sintCosp__23;
wire	[32-1:0]				sintCosp__24;
wire	[32-1:0]				sintCosp__25;
wire	[32-1:0]				sintCosp__26;
wire	[32-1:0]				sintCosp__27;
wire	[32-1:0]				sintCosp__28;
wire	[32-1:0]				sintCosp__29;
wire	[32-1:0]				sintCosp__30;
wire	[32-1:0]				sintCosp__31;
wire	[32-1:0]				sintCosp__32;
wire	[32-1:0]				sintCosp__33;
wire	[32-1:0]				sintCosp__34;
wire	[32-1:0]				sintCosp__35;
wire	[32-1:0]				sintCosp__36;
wire	[32-1:0]				sintCosp__37;


//wire		[31:0]		sintSinp[37:0];


wire	[32-1:0]				sintSinp__0;
wire	[32-1:0]				sintSinp__1;
wire	[32-1:0]				sintSinp__2;
wire	[32-1:0]				sintSinp__3;
wire	[32-1:0]				sintSinp__4;
wire	[32-1:0]				sintSinp__5;
wire	[32-1:0]				sintSinp__6;
wire	[32-1:0]				sintSinp__7;
wire	[32-1:0]				sintSinp__8;
wire	[32-1:0]				sintSinp__9;
wire	[32-1:0]				sintSinp__10;
wire	[32-1:0]				sintSinp__11;
wire	[32-1:0]				sintSinp__12;
wire	[32-1:0]				sintSinp__13;
wire	[32-1:0]				sintSinp__14;
wire	[32-1:0]				sintSinp__15;
wire	[32-1:0]				sintSinp__16;
wire	[32-1:0]				sintSinp__17;
wire	[32-1:0]				sintSinp__18;
wire	[32-1:0]				sintSinp__19;
wire	[32-1:0]				sintSinp__20;
wire	[32-1:0]				sintSinp__21;
wire	[32-1:0]				sintSinp__22;
wire	[32-1:0]				sintSinp__23;
wire	[32-1:0]				sintSinp__24;
wire	[32-1:0]				sintSinp__25;
wire	[32-1:0]				sintSinp__26;
wire	[32-1:0]				sintSinp__27;
wire	[32-1:0]				sintSinp__28;
wire	[32-1:0]				sintSinp__29;
wire	[32-1:0]				sintSinp__30;
wire	[32-1:0]				sintSinp__31;
wire	[32-1:0]				sintSinp__32;
wire	[32-1:0]				sintSinp__33;
wire	[32-1:0]				sintSinp__34;
wire	[32-1:0]				sintSinp__35;
wire	[32-1:0]				sintSinp__36;
wire	[32-1:0]				sintSinp__37;


//wire		[63:0]		uz2[37:0];


wire	[63:0]				uz2__0;
wire	[63:0]				uz2__1;
wire	[63:0]				uz2__2;
wire	[63:0]				uz2__3;
wire	[63:0]				uz2__4;
wire	[63:0]				uz2__5;
wire	[63:0]				uz2__6;
wire	[63:0]				uz2__7;
wire	[63:0]				uz2__8;
wire	[63:0]				uz2__9;
wire	[63:0]				uz2__10;
wire	[63:0]				uz2__11;
wire	[63:0]				uz2__12;
wire	[63:0]				uz2__13;
wire	[63:0]				uz2__14;
wire	[63:0]				uz2__15;
wire	[63:0]				uz2__16;
wire	[63:0]				uz2__17;
wire	[63:0]				uz2__18;
wire	[63:0]				uz2__19;
wire	[63:0]				uz2__20;
wire	[63:0]				uz2__21;
wire	[63:0]				uz2__22;
wire	[63:0]				uz2__23;
wire	[63:0]				uz2__24;
wire	[63:0]				uz2__25;
wire	[63:0]				uz2__26;
wire	[63:0]				uz2__27;
wire	[63:0]				uz2__28;
wire	[63:0]				uz2__29;
wire	[63:0]				uz2__30;
wire	[63:0]				uz2__31;
wire	[63:0]				uz2__32;
wire	[63:0]				uz2__33;
wire	[63:0]				uz2__34;
wire	[63:0]				uz2__35;
wire	[63:0]				uz2__36;
wire	[63:0]				uz2__37;


//wire		[31:0]		uxUz[37:0];

wire	[32-1:0]				uxUz__0;
wire	[32-1:0]				uxUz__1;
wire	[32-1:0]				uxUz__2;
wire	[32-1:0]				uxUz__3;
wire	[32-1:0]				uxUz__4;
wire	[32-1:0]				uxUz__5;
wire	[32-1:0]				uxUz__6;
wire	[32-1:0]				uxUz__7;
wire	[32-1:0]				uxUz__8;
wire	[32-1:0]				uxUz__9;
wire	[32-1:0]				uxUz__10;
wire	[32-1:0]				uxUz__11;
wire	[32-1:0]				uxUz__12;
wire	[32-1:0]				uxUz__13;
wire	[32-1:0]				uxUz__14;
wire	[32-1:0]				uxUz__15;
wire	[32-1:0]				uxUz__16;
wire	[32-1:0]				uxUz__17;
wire	[32-1:0]				uxUz__18;
wire	[32-1:0]				uxUz__19;
wire	[32-1:0]				uxUz__20;
wire	[32-1:0]				uxUz__21;
wire	[32-1:0]				uxUz__22;
wire	[32-1:0]				uxUz__23;
wire	[32-1:0]				uxUz__24;
wire	[32-1:0]				uxUz__25;
wire	[32-1:0]				uxUz__26;
wire	[32-1:0]				uxUz__27;
wire	[32-1:0]				uxUz__28;
wire	[32-1:0]				uxUz__29;
wire	[32-1:0]				uxUz__30;
wire	[32-1:0]				uxUz__31;
wire	[32-1:0]				uxUz__32;
wire	[32-1:0]				uxUz__33;
wire	[32-1:0]				uxUz__34;
wire	[32-1:0]				uxUz__35;
wire	[32-1:0]				uxUz__36;
wire	[32-1:0]				uxUz__37;


//wire		[31:0]		uyUz[37:0];


wire	[32-1:0]				uyUz__0;
wire	[32-1:0]				uyUz__1;
wire	[32-1:0]				uyUz__2;
wire	[32-1:0]				uyUz__3;
wire	[32-1:0]				uyUz__4;
wire	[32-1:0]				uyUz__5;
wire	[32-1:0]				uyUz__6;
wire	[32-1:0]				uyUz__7;
wire	[32-1:0]				uyUz__8;
wire	[32-1:0]				uyUz__9;
wire	[32-1:0]				uyUz__10;
wire	[32-1:0]				uyUz__11;
wire	[32-1:0]				uyUz__12;
wire	[32-1:0]				uyUz__13;
wire	[32-1:0]				uyUz__14;
wire	[32-1:0]				uyUz__15;
wire	[32-1:0]				uyUz__16;
wire	[32-1:0]				uyUz__17;
wire	[32-1:0]				uyUz__18;
wire	[32-1:0]				uyUz__19;
wire	[32-1:0]				uyUz__20;
wire	[32-1:0]				uyUz__21;
wire	[32-1:0]				uyUz__22;
wire	[32-1:0]				uyUz__23;
wire	[32-1:0]				uyUz__24;
wire	[32-1:0]				uyUz__25;
wire	[32-1:0]				uyUz__26;
wire	[32-1:0]				uyUz__27;
wire	[32-1:0]				uyUz__28;
wire	[32-1:0]				uyUz__29;
wire	[32-1:0]				uyUz__30;
wire	[32-1:0]				uyUz__31;
wire	[32-1:0]				uyUz__32;
wire	[32-1:0]				uyUz__33;
wire	[32-1:0]				uyUz__34;
wire	[32-1:0]				uyUz__35;
wire	[32-1:0]				uyUz__36;
wire	[32-1:0]				uyUz__37;

//wire		[31:0]		uySintSinp[37:0];


wire	[32-1:0]				uySintSinp__0;
wire	[32-1:0]				uySintSinp__1;
wire	[32-1:0]				uySintSinp__2;
wire	[32-1:0]				uySintSinp__3;
wire	[32-1:0]				uySintSinp__4;
wire	[32-1:0]				uySintSinp__5;
wire	[32-1:0]				uySintSinp__6;
wire	[32-1:0]				uySintSinp__7;
wire	[32-1:0]				uySintSinp__8;
wire	[32-1:0]				uySintSinp__9;
wire	[32-1:0]				uySintSinp__10;
wire	[32-1:0]				uySintSinp__11;
wire	[32-1:0]				uySintSinp__12;
wire	[32-1:0]				uySintSinp__13;
wire	[32-1:0]				uySintSinp__14;
wire	[32-1:0]				uySintSinp__15;
wire	[32-1:0]				uySintSinp__16;
wire	[32-1:0]				uySintSinp__17;
wire	[32-1:0]				uySintSinp__18;
wire	[32-1:0]				uySintSinp__19;
wire	[32-1:0]				uySintSinp__20;
wire	[32-1:0]				uySintSinp__21;
wire	[32-1:0]				uySintSinp__22;
wire	[32-1:0]				uySintSinp__23;
wire	[32-1:0]				uySintSinp__24;
wire	[32-1:0]				uySintSinp__25;
wire	[32-1:0]				uySintSinp__26;
wire	[32-1:0]				uySintSinp__27;
wire	[32-1:0]				uySintSinp__28;
wire	[32-1:0]				uySintSinp__29;
wire	[32-1:0]				uySintSinp__30;
wire	[32-1:0]				uySintSinp__31;
wire	[32-1:0]				uySintSinp__32;
wire	[32-1:0]				uySintSinp__33;
wire	[32-1:0]				uySintSinp__34;
wire	[32-1:0]				uySintSinp__35;
wire	[32-1:0]				uySintSinp__36;
wire	[32-1:0]				uySintSinp__37;


//wire		[63:0]		oneMinusUz2[37:0];


wire	[63:0]				oneMinusUz2__0;
wire	[63:0]				oneMinusUz2__1;
wire	[63:0]				oneMinusUz2__2;
wire	[63:0]				oneMinusUz2__3;
wire	[63:0]				oneMinusUz2__4;
wire	[63:0]				oneMinusUz2__5;
wire	[63:0]				oneMinusUz2__6;
wire	[63:0]				oneMinusUz2__7;
wire	[63:0]				oneMinusUz2__8;
wire	[63:0]				oneMinusUz2__9;
wire	[63:0]				oneMinusUz2__10;
wire	[63:0]				oneMinusUz2__11;
wire	[63:0]				oneMinusUz2__12;
wire	[63:0]				oneMinusUz2__13;
wire	[63:0]				oneMinusUz2__14;
wire	[63:0]				oneMinusUz2__15;
wire	[63:0]				oneMinusUz2__16;
wire	[63:0]				oneMinusUz2__17;
wire	[63:0]				oneMinusUz2__18;
wire	[63:0]				oneMinusUz2__19;
wire	[63:0]				oneMinusUz2__20;
wire	[63:0]				oneMinusUz2__21;
wire	[63:0]				oneMinusUz2__22;
wire	[63:0]				oneMinusUz2__23;
wire	[63:0]				oneMinusUz2__24;
wire	[63:0]				oneMinusUz2__25;
wire	[63:0]				oneMinusUz2__26;
wire	[63:0]				oneMinusUz2__27;
wire	[63:0]				oneMinusUz2__28;
wire	[63:0]				oneMinusUz2__29;
wire	[63:0]				oneMinusUz2__30;
wire	[63:0]				oneMinusUz2__31;
wire	[63:0]				oneMinusUz2__32;
wire	[63:0]				oneMinusUz2__33;
wire	[63:0]				oneMinusUz2__34;
wire	[63:0]				oneMinusUz2__35;
wire	[63:0]				oneMinusUz2__36;
wire	[63:0]				oneMinusUz2__37;


//wire		[31:0]		uyUzSintCosp[37:0];


wire	[32-1:0]				uyUzSintCosp__0;
wire	[32-1:0]				uyUzSintCosp__1;
wire	[32-1:0]				uyUzSintCosp__2;
wire	[32-1:0]				uyUzSintCosp__3;
wire	[32-1:0]				uyUzSintCosp__4;
wire	[32-1:0]				uyUzSintCosp__5;
wire	[32-1:0]				uyUzSintCosp__6;
wire	[32-1:0]				uyUzSintCosp__7;
wire	[32-1:0]				uyUzSintCosp__8;
wire	[32-1:0]				uyUzSintCosp__9;
wire	[32-1:0]				uyUzSintCosp__10;
wire	[32-1:0]				uyUzSintCosp__11;
wire	[32-1:0]				uyUzSintCosp__12;
wire	[32-1:0]				uyUzSintCosp__13;
wire	[32-1:0]				uyUzSintCosp__14;
wire	[32-1:0]				uyUzSintCosp__15;
wire	[32-1:0]				uyUzSintCosp__16;
wire	[32-1:0]				uyUzSintCosp__17;
wire	[32-1:0]				uyUzSintCosp__18;
wire	[32-1:0]				uyUzSintCosp__19;
wire	[32-1:0]				uyUzSintCosp__20;
wire	[32-1:0]				uyUzSintCosp__21;
wire	[32-1:0]				uyUzSintCosp__22;
wire	[32-1:0]				uyUzSintCosp__23;
wire	[32-1:0]				uyUzSintCosp__24;
wire	[32-1:0]				uyUzSintCosp__25;
wire	[32-1:0]				uyUzSintCosp__26;
wire	[32-1:0]				uyUzSintCosp__27;
wire	[32-1:0]				uyUzSintCosp__28;
wire	[32-1:0]				uyUzSintCosp__29;
wire	[32-1:0]				uyUzSintCosp__30;
wire	[32-1:0]				uyUzSintCosp__31;
wire	[32-1:0]				uyUzSintCosp__32;
wire	[32-1:0]				uyUzSintCosp__33;
wire	[32-1:0]				uyUzSintCosp__34;
wire	[32-1:0]				uyUzSintCosp__35;
wire	[32-1:0]				uyUzSintCosp__36;
wire	[32-1:0]				uyUzSintCosp__37;


//wire		[31:0]		uxUzSintCosp[37:0];


wire	[32-1:0]				uxUzSintCosp__0;
wire	[32-1:0]				uxUzSintCosp__1;
wire	[32-1:0]				uxUzSintCosp__2;
wire	[32-1:0]				uxUzSintCosp__3;
wire	[32-1:0]				uxUzSintCosp__4;
wire	[32-1:0]				uxUzSintCosp__5;
wire	[32-1:0]				uxUzSintCosp__6;
wire	[32-1:0]				uxUzSintCosp__7;
wire	[32-1:0]				uxUzSintCosp__8;
wire	[32-1:0]				uxUzSintCosp__9;
wire	[32-1:0]				uxUzSintCosp__10;
wire	[32-1:0]				uxUzSintCosp__11;
wire	[32-1:0]				uxUzSintCosp__12;
wire	[32-1:0]				uxUzSintCosp__13;
wire	[32-1:0]				uxUzSintCosp__14;
wire	[32-1:0]				uxUzSintCosp__15;
wire	[32-1:0]				uxUzSintCosp__16;
wire	[32-1:0]				uxUzSintCosp__17;
wire	[32-1:0]				uxUzSintCosp__18;
wire	[32-1:0]				uxUzSintCosp__19;
wire	[32-1:0]				uxUzSintCosp__20;
wire	[32-1:0]				uxUzSintCosp__21;
wire	[32-1:0]				uxUzSintCosp__22;
wire	[32-1:0]				uxUzSintCosp__23;
wire	[32-1:0]				uxUzSintCosp__24;
wire	[32-1:0]				uxUzSintCosp__25;
wire	[32-1:0]				uxUzSintCosp__26;
wire	[32-1:0]				uxUzSintCosp__27;
wire	[32-1:0]				uxUzSintCosp__28;
wire	[32-1:0]				uxUzSintCosp__29;
wire	[32-1:0]				uxUzSintCosp__30;
wire	[32-1:0]				uxUzSintCosp__31;
wire	[32-1:0]				uxUzSintCosp__32;
wire	[32-1:0]				uxUzSintCosp__33;
wire	[32-1:0]				uxUzSintCosp__34;
wire	[32-1:0]				uxUzSintCosp__35;
wire	[32-1:0]				uxUzSintCosp__36;
wire	[32-1:0]				uxUzSintCosp__37;


//wire		[31:0]		uxSintSinp[37:0];

wire	[32-1:0]				uxSintSinp__0;
wire	[32-1:0]				uxSintSinp__1;
wire	[32-1:0]				uxSintSinp__2;
wire	[32-1:0]				uxSintSinp__3;
wire	[32-1:0]				uxSintSinp__4;
wire	[32-1:0]				uxSintSinp__5;
wire	[32-1:0]				uxSintSinp__6;
wire	[32-1:0]				uxSintSinp__7;
wire	[32-1:0]				uxSintSinp__8;
wire	[32-1:0]				uxSintSinp__9;
wire	[32-1:0]				uxSintSinp__10;
wire	[32-1:0]				uxSintSinp__11;
wire	[32-1:0]				uxSintSinp__12;
wire	[32-1:0]				uxSintSinp__13;
wire	[32-1:0]				uxSintSinp__14;
wire	[32-1:0]				uxSintSinp__15;
wire	[32-1:0]				uxSintSinp__16;
wire	[32-1:0]				uxSintSinp__17;
wire	[32-1:0]				uxSintSinp__18;
wire	[32-1:0]				uxSintSinp__19;
wire	[32-1:0]				uxSintSinp__20;
wire	[32-1:0]				uxSintSinp__21;
wire	[32-1:0]				uxSintSinp__22;
wire	[32-1:0]				uxSintSinp__23;
wire	[32-1:0]				uxSintSinp__24;
wire	[32-1:0]				uxSintSinp__25;
wire	[32-1:0]				uxSintSinp__26;
wire	[32-1:0]				uxSintSinp__27;
wire	[32-1:0]				uxSintSinp__28;
wire	[32-1:0]				uxSintSinp__29;
wire	[32-1:0]				uxSintSinp__30;
wire	[32-1:0]				uxSintSinp__31;
wire	[32-1:0]				uxSintSinp__32;
wire	[32-1:0]				uxSintSinp__33;
wire	[32-1:0]				uxSintSinp__34;
wire	[32-1:0]				uxSintSinp__35;
wire	[32-1:0]				uxSintSinp__36;
wire	[32-1:0]				uxSintSinp__37;


//wire		[31:0]		sqrtOneMinusUz2[37:0];

wire	[32-1:0]				sqrtOneMinusUz2__0;
wire	[32-1:0]				sqrtOneMinusUz2__1;
wire	[32-1:0]				sqrtOneMinusUz2__2;
wire	[32-1:0]				sqrtOneMinusUz2__3;
wire	[32-1:0]				sqrtOneMinusUz2__4;
wire	[32-1:0]				sqrtOneMinusUz2__5;
wire	[32-1:0]				sqrtOneMinusUz2__6;
wire	[32-1:0]				sqrtOneMinusUz2__7;
wire	[32-1:0]				sqrtOneMinusUz2__8;
wire	[32-1:0]				sqrtOneMinusUz2__9;
wire	[32-1:0]				sqrtOneMinusUz2__10;
wire	[32-1:0]				sqrtOneMinusUz2__11;
wire	[32-1:0]				sqrtOneMinusUz2__12;
wire	[32-1:0]				sqrtOneMinusUz2__13;
wire	[32-1:0]				sqrtOneMinusUz2__14;
wire	[32-1:0]				sqrtOneMinusUz2__15;
wire	[32-1:0]				sqrtOneMinusUz2__16;
wire	[32-1:0]				sqrtOneMinusUz2__17;
wire	[32-1:0]				sqrtOneMinusUz2__18;
wire	[32-1:0]				sqrtOneMinusUz2__19;
wire	[32-1:0]				sqrtOneMinusUz2__20;
wire	[32-1:0]				sqrtOneMinusUz2__21;
wire	[32-1:0]				sqrtOneMinusUz2__22;
wire	[32-1:0]				sqrtOneMinusUz2__23;
wire	[32-1:0]				sqrtOneMinusUz2__24;
wire	[32-1:0]				sqrtOneMinusUz2__25;
wire	[32-1:0]				sqrtOneMinusUz2__26;
wire	[32-1:0]				sqrtOneMinusUz2__27;
wire	[32-1:0]				sqrtOneMinusUz2__28;
wire	[32-1:0]				sqrtOneMinusUz2__29;
wire	[32-1:0]				sqrtOneMinusUz2__30;
wire	[32-1:0]				sqrtOneMinusUz2__31;
wire	[32-1:0]				sqrtOneMinusUz2__32;
wire	[32-1:0]				sqrtOneMinusUz2__33;
wire	[32-1:0]				sqrtOneMinusUz2__34;
wire	[32-1:0]				sqrtOneMinusUz2__35;
wire	[32-1:0]				sqrtOneMinusUz2__36;
wire	[32-1:0]				sqrtOneMinusUz2__37;

//wire		[31:0]		sintCospSqrtOneMinusUz2[37:0];


wire	[32-1:0]				sintCospSqrtOneMinusUz2__0;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__1;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__2;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__3;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__4;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__5;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__6;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__7;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__8;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__9;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__10;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__11;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__12;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__13;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__14;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__15;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__16;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__17;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__18;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__19;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__20;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__21;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__22;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__23;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__24;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__25;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__26;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__27;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__28;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__29;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__30;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__31;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__32;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__33;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__34;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__35;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__36;
wire	[32-1:0]				sintCospSqrtOneMinusUz2__37;

//wire		[31:0]		uxCost[37:0];


wire	[32-1:0]				uxCost__0;
wire	[32-1:0]				uxCost__1;
wire	[32-1:0]				uxCost__2;
wire	[32-1:0]				uxCost__3;
wire	[32-1:0]				uxCost__4;
wire	[32-1:0]				uxCost__5;
wire	[32-1:0]				uxCost__6;
wire	[32-1:0]				uxCost__7;
wire	[32-1:0]				uxCost__8;
wire	[32-1:0]				uxCost__9;
wire	[32-1:0]				uxCost__10;
wire	[32-1:0]				uxCost__11;
wire	[32-1:0]				uxCost__12;
wire	[32-1:0]				uxCost__13;
wire	[32-1:0]				uxCost__14;
wire	[32-1:0]				uxCost__15;
wire	[32-1:0]				uxCost__16;
wire	[32-1:0]				uxCost__17;
wire	[32-1:0]				uxCost__18;
wire	[32-1:0]				uxCost__19;
wire	[32-1:0]				uxCost__20;
wire	[32-1:0]				uxCost__21;
wire	[32-1:0]				uxCost__22;
wire	[32-1:0]				uxCost__23;
wire	[32-1:0]				uxCost__24;
wire	[32-1:0]				uxCost__25;
wire	[32-1:0]				uxCost__26;
wire	[32-1:0]				uxCost__27;
wire	[32-1:0]				uxCost__28;
wire	[32-1:0]				uxCost__29;
wire	[32-1:0]				uxCost__30;
wire	[32-1:0]				uxCost__31;
wire	[32-1:0]				uxCost__32;
wire	[32-1:0]				uxCost__33;
wire	[32-1:0]				uxCost__34;
wire	[32-1:0]				uxCost__35;
wire	[32-1:0]				uxCost__36;
wire	[32-1:0]				uxCost__37;

//wire		[31:0]		uzCost[37:0];


wire	[32-1:0]				uzCost__0;
wire	[32-1:0]				uzCost__1;
wire	[32-1:0]				uzCost__2;
wire	[32-1:0]				uzCost__3;
wire	[32-1:0]				uzCost__4;
wire	[32-1:0]				uzCost__5;
wire	[32-1:0]				uzCost__6;
wire	[32-1:0]				uzCost__7;
wire	[32-1:0]				uzCost__8;
wire	[32-1:0]				uzCost__9;
wire	[32-1:0]				uzCost__10;
wire	[32-1:0]				uzCost__11;
wire	[32-1:0]				uzCost__12;
wire	[32-1:0]				uzCost__13;
wire	[32-1:0]				uzCost__14;
wire	[32-1:0]				uzCost__15;
wire	[32-1:0]				uzCost__16;
wire	[32-1:0]				uzCost__17;
wire	[32-1:0]				uzCost__18;
wire	[32-1:0]				uzCost__19;
wire	[32-1:0]				uzCost__20;
wire	[32-1:0]				uzCost__21;
wire	[32-1:0]				uzCost__22;
wire	[32-1:0]				uzCost__23;
wire	[32-1:0]				uzCost__24;
wire	[32-1:0]				uzCost__25;
wire	[32-1:0]				uzCost__26;
wire	[32-1:0]				uzCost__27;
wire	[32-1:0]				uzCost__28;
wire	[32-1:0]				uzCost__29;
wire	[32-1:0]				uzCost__30;
wire	[32-1:0]				uzCost__31;
wire	[32-1:0]				uzCost__32;
wire	[32-1:0]				uzCost__33;
wire	[32-1:0]				uzCost__34;
wire	[32-1:0]				uzCost__35;
wire	[32-1:0]				uzCost__36;
wire	[32-1:0]				uzCost__37;


//wire		[31:0]		sqrtOneMinusUz2_inv[37:0];


wire	[32-1:0]				sqrtOneMinusUz2_inv__0;
wire	[32-1:0]				sqrtOneMinusUz2_inv__1;
wire	[32-1:0]				sqrtOneMinusUz2_inv__2;
wire	[32-1:0]				sqrtOneMinusUz2_inv__3;
wire	[32-1:0]				sqrtOneMinusUz2_inv__4;
wire	[32-1:0]				sqrtOneMinusUz2_inv__5;
wire	[32-1:0]				sqrtOneMinusUz2_inv__6;
wire	[32-1:0]				sqrtOneMinusUz2_inv__7;
wire	[32-1:0]				sqrtOneMinusUz2_inv__8;
wire	[32-1:0]				sqrtOneMinusUz2_inv__9;
wire	[32-1:0]				sqrtOneMinusUz2_inv__10;
wire	[32-1:0]				sqrtOneMinusUz2_inv__11;
wire	[32-1:0]				sqrtOneMinusUz2_inv__12;
wire	[32-1:0]				sqrtOneMinusUz2_inv__13;
wire	[32-1:0]				sqrtOneMinusUz2_inv__14;
wire	[32-1:0]				sqrtOneMinusUz2_inv__15;
wire	[32-1:0]				sqrtOneMinusUz2_inv__16;
wire	[32-1:0]				sqrtOneMinusUz2_inv__17;
wire	[32-1:0]				sqrtOneMinusUz2_inv__18;
wire	[32-1:0]				sqrtOneMinusUz2_inv__19;
wire	[32-1:0]				sqrtOneMinusUz2_inv__20;
wire	[32-1:0]				sqrtOneMinusUz2_inv__21;
wire	[32-1:0]				sqrtOneMinusUz2_inv__22;
wire	[32-1:0]				sqrtOneMinusUz2_inv__23;
wire	[32-1:0]				sqrtOneMinusUz2_inv__24;
wire	[32-1:0]				sqrtOneMinusUz2_inv__25;
wire	[32-1:0]				sqrtOneMinusUz2_inv__26;
wire	[32-1:0]				sqrtOneMinusUz2_inv__27;
wire	[32-1:0]				sqrtOneMinusUz2_inv__28;
wire	[32-1:0]				sqrtOneMinusUz2_inv__29;
wire	[32-1:0]				sqrtOneMinusUz2_inv__30;
wire	[32-1:0]				sqrtOneMinusUz2_inv__31;
wire	[32-1:0]				sqrtOneMinusUz2_inv__32;
wire	[32-1:0]				sqrtOneMinusUz2_inv__33;
wire	[32-1:0]				sqrtOneMinusUz2_inv__34;
wire	[32-1:0]				sqrtOneMinusUz2_inv__35;
wire	[32-1:0]				sqrtOneMinusUz2_inv__36;
wire	[32-1:0]				sqrtOneMinusUz2_inv__37;

//wire		[31:0]		uxNumerator[37:0];


wire	[32-1:0]				uxNumerator__0;
wire	[32-1:0]				uxNumerator__1;
wire	[32-1:0]				uxNumerator__2;
wire	[32-1:0]				uxNumerator__3;
wire	[32-1:0]				uxNumerator__4;
wire	[32-1:0]				uxNumerator__5;
wire	[32-1:0]				uxNumerator__6;
wire	[32-1:0]				uxNumerator__7;
wire	[32-1:0]				uxNumerator__8;
wire	[32-1:0]				uxNumerator__9;
wire	[32-1:0]				uxNumerator__10;
wire	[32-1:0]				uxNumerator__11;
wire	[32-1:0]				uxNumerator__12;
wire	[32-1:0]				uxNumerator__13;
wire	[32-1:0]				uxNumerator__14;
wire	[32-1:0]				uxNumerator__15;
wire	[32-1:0]				uxNumerator__16;
wire	[32-1:0]				uxNumerator__17;
wire	[32-1:0]				uxNumerator__18;
wire	[32-1:0]				uxNumerator__19;
wire	[32-1:0]				uxNumerator__20;
wire	[32-1:0]				uxNumerator__21;
wire	[32-1:0]				uxNumerator__22;
wire	[32-1:0]				uxNumerator__23;
wire	[32-1:0]				uxNumerator__24;
wire	[32-1:0]				uxNumerator__25;
wire	[32-1:0]				uxNumerator__26;
wire	[32-1:0]				uxNumerator__27;
wire	[32-1:0]				uxNumerator__28;
wire	[32-1:0]				uxNumerator__29;
wire	[32-1:0]				uxNumerator__30;
wire	[32-1:0]				uxNumerator__31;
wire	[32-1:0]				uxNumerator__32;
wire	[32-1:0]				uxNumerator__33;
wire	[32-1:0]				uxNumerator__34;
wire	[32-1:0]				uxNumerator__35;
wire	[32-1:0]				uxNumerator__36;
wire	[32-1:0]				uxNumerator__37;

//wire		[31:0]		uyNumerator[37:0];


wire	[32-1:0]				uyNumerator__0;
wire	[32-1:0]				uyNumerator__1;
wire	[32-1:0]				uyNumerator__2;
wire	[32-1:0]				uyNumerator__3;
wire	[32-1:0]				uyNumerator__4;
wire	[32-1:0]				uyNumerator__5;
wire	[32-1:0]				uyNumerator__6;
wire	[32-1:0]				uyNumerator__7;
wire	[32-1:0]				uyNumerator__8;
wire	[32-1:0]				uyNumerator__9;
wire	[32-1:0]				uyNumerator__10;
wire	[32-1:0]				uyNumerator__11;
wire	[32-1:0]				uyNumerator__12;
wire	[32-1:0]				uyNumerator__13;
wire	[32-1:0]				uyNumerator__14;
wire	[32-1:0]				uyNumerator__15;
wire	[32-1:0]				uyNumerator__16;
wire	[32-1:0]				uyNumerator__17;
wire	[32-1:0]				uyNumerator__18;
wire	[32-1:0]				uyNumerator__19;
wire	[32-1:0]				uyNumerator__20;
wire	[32-1:0]				uyNumerator__21;
wire	[32-1:0]				uyNumerator__22;
wire	[32-1:0]				uyNumerator__23;
wire	[32-1:0]				uyNumerator__24;
wire	[32-1:0]				uyNumerator__25;
wire	[32-1:0]				uyNumerator__26;
wire	[32-1:0]				uyNumerator__27;
wire	[32-1:0]				uyNumerator__28;
wire	[32-1:0]				uyNumerator__29;
wire	[32-1:0]				uyNumerator__30;
wire	[32-1:0]				uyNumerator__31;
wire	[32-1:0]				uyNumerator__32;
wire	[32-1:0]				uyNumerator__33;
wire	[32-1:0]				uyNumerator__34;
wire	[32-1:0]				uyNumerator__35;
wire	[32-1:0]				uyNumerator__36;
wire	[32-1:0]				uyNumerator__37;

//wire		[31:0]		uyCost[37:0];


wire	[32-1:0]				uyCost__0;
wire	[32-1:0]				uyCost__1;
wire	[32-1:0]				uyCost__2;
wire	[32-1:0]				uyCost__3;
wire	[32-1:0]				uyCost__4;
wire	[32-1:0]				uyCost__5;
wire	[32-1:0]				uyCost__6;
wire	[32-1:0]				uyCost__7;
wire	[32-1:0]				uyCost__8;
wire	[32-1:0]				uyCost__9;
wire	[32-1:0]				uyCost__10;
wire	[32-1:0]				uyCost__11;
wire	[32-1:0]				uyCost__12;
wire	[32-1:0]				uyCost__13;
wire	[32-1:0]				uyCost__14;
wire	[32-1:0]				uyCost__15;
wire	[32-1:0]				uyCost__16;
wire	[32-1:0]				uyCost__17;
wire	[32-1:0]				uyCost__18;
wire	[32-1:0]				uyCost__19;
wire	[32-1:0]				uyCost__20;
wire	[32-1:0]				uyCost__21;
wire	[32-1:0]				uyCost__22;
wire	[32-1:0]				uyCost__23;
wire	[32-1:0]				uyCost__24;
wire	[32-1:0]				uyCost__25;
wire	[32-1:0]				uyCost__26;
wire	[32-1:0]				uyCost__27;
wire	[32-1:0]				uyCost__28;
wire	[32-1:0]				uyCost__29;
wire	[32-1:0]				uyCost__30;
wire	[32-1:0]				uyCost__31;
wire	[32-1:0]				uyCost__32;
wire	[32-1:0]				uyCost__33;
wire	[32-1:0]				uyCost__34;
wire	[32-1:0]				uyCost__35;
wire	[32-1:0]				uyCost__36;
wire	[32-1:0]				uyCost__37;

//wire		[31:0]		uxQuotient[37:0];


wire	[32-1:0]				uxQuotient__0;
wire	[32-1:0]				uxQuotient__1;
wire	[32-1:0]				uxQuotient__2;
wire	[32-1:0]				uxQuotient__3;
wire	[32-1:0]				uxQuotient__4;
wire	[32-1:0]				uxQuotient__5;
wire	[32-1:0]				uxQuotient__6;
wire	[32-1:0]				uxQuotient__7;
wire	[32-1:0]				uxQuotient__8;
wire	[32-1:0]				uxQuotient__9;
wire	[32-1:0]				uxQuotient__10;
wire	[32-1:0]				uxQuotient__11;
wire	[32-1:0]				uxQuotient__12;
wire	[32-1:0]				uxQuotient__13;
wire	[32-1:0]				uxQuotient__14;
wire	[32-1:0]				uxQuotient__15;
wire	[32-1:0]				uxQuotient__16;
wire	[32-1:0]				uxQuotient__17;
wire	[32-1:0]				uxQuotient__18;
wire	[32-1:0]				uxQuotient__19;
wire	[32-1:0]				uxQuotient__20;
wire	[32-1:0]				uxQuotient__21;
wire	[32-1:0]				uxQuotient__22;
wire	[32-1:0]				uxQuotient__23;
wire	[32-1:0]				uxQuotient__24;
wire	[32-1:0]				uxQuotient__25;
wire	[32-1:0]				uxQuotient__26;
wire	[32-1:0]				uxQuotient__27;
wire	[32-1:0]				uxQuotient__28;
wire	[32-1:0]				uxQuotient__29;
wire	[32-1:0]				uxQuotient__30;
wire	[32-1:0]				uxQuotient__31;
wire	[32-1:0]				uxQuotient__32;
wire	[32-1:0]				uxQuotient__33;
wire	[32-1:0]				uxQuotient__34;
wire	[32-1:0]				uxQuotient__35;
wire	[32-1:0]				uxQuotient__36;
wire	[32-1:0]				uxQuotient__37;

//wire		[31:0]		uyQuotient[37:0];


wire	[32-1:0]				uyQuotient__0;
wire	[32-1:0]				uyQuotient__1;
wire	[32-1:0]				uyQuotient__2;
wire	[32-1:0]				uyQuotient__3;
wire	[32-1:0]				uyQuotient__4;
wire	[32-1:0]				uyQuotient__5;
wire	[32-1:0]				uyQuotient__6;
wire	[32-1:0]				uyQuotient__7;
wire	[32-1:0]				uyQuotient__8;
wire	[32-1:0]				uyQuotient__9;
wire	[32-1:0]				uyQuotient__10;
wire	[32-1:0]				uyQuotient__11;
wire	[32-1:0]				uyQuotient__12;
wire	[32-1:0]				uyQuotient__13;
wire	[32-1:0]				uyQuotient__14;
wire	[32-1:0]				uyQuotient__15;
wire	[32-1:0]				uyQuotient__16;
wire	[32-1:0]				uyQuotient__17;
wire	[32-1:0]				uyQuotient__18;
wire	[32-1:0]				uyQuotient__19;
wire	[32-1:0]				uyQuotient__20;
wire	[32-1:0]				uyQuotient__21;
wire	[32-1:0]				uyQuotient__22;
wire	[32-1:0]				uyQuotient__23;
wire	[32-1:0]				uyQuotient__24;
wire	[32-1:0]				uyQuotient__25;
wire	[32-1:0]				uyQuotient__26;
wire	[32-1:0]				uyQuotient__27;
wire	[32-1:0]				uyQuotient__28;
wire	[32-1:0]				uyQuotient__29;
wire	[32-1:0]				uyQuotient__30;
wire	[32-1:0]				uyQuotient__31;
wire	[32-1:0]				uyQuotient__32;
wire	[32-1:0]				uyQuotient__33;
wire	[32-1:0]				uyQuotient__34;
wire	[32-1:0]				uyQuotient__35;
wire	[32-1:0]				uyQuotient__36;
wire	[32-1:0]				uyQuotient__37;

wire		[31:0]		new_sint;
wire		[31:0]		new_cost;
wire		[31:0]		new_sinp;
wire		[31:0]		new_cosp;
wire		[31:0]		new_sintCosp;
wire		[31:0]		new_sintSinp;
wire		[63:0]		new_uz2;
wire		[31:0]		new_uxUz;
wire		[31:0]		new_uyUz;
wire		[31:0]		new_uySintSinp;
wire		[63:0]		new_oneMinusUz2;
wire		[31:0]		new_uyUzSintCosp;
wire		[31:0]		new_uxUzSintCosp;
wire		[31:0]		new_uxSintSinp;
wire		[31:0]		new_sqrtOneMinusUz2;
wire		[31:0]		new_sintCospSqrtOneMinusUz2;
wire		[31:0]		new_uxCost;
wire		[31:0]		new_uzCost;
wire		[31:0]		new_sqrtOneMinusUz2_inv;
wire		[31:0]		new_uxNumerator;
wire		[31:0]		new_uyNumerator;
wire		[31:0]		new_uyCost;
reg		[31:0]		new_uxQuotient;
reg		[31:0]		new_uyQuotient;


//Wiring for calculating final values
wire				uxNumerOverflow;
wire				uyNumerOverflow;
reg					normalIncident;
wire		[31:0]		ux_add_1;
wire		[31:0]		ux_add_2;
wire					uxOverflow;
wire		[31:0]		uy_add_1;
wire		[31:0]		uy_add_2;
wire					uyOverflow;
wire		[31:0]		normalUz;
wire		[31:0]		uz_sub_1;
wire		[31:0]		uz_sub_2;
wire					uzOverflow;
	
wire		[31:0]		new_ux;
wire		[31:0]		new_uy;
wire		[31:0]		new_uz;

wire				div_overflow;
wire				toAnd1_36_1;
wire				toAnd1_36_2;
wire				overflow1_36;
wire				negOverflow1_36;
wire				toAnd2_36_1;
wire				toAnd2_36_2;
wire				overflow2_36;
wire				negOverflow2_36;



//------------------Register Pipeline-----------------
//Generation Methodology: Standard block, called InternalsBlock, is
//repeated multiple times, based on the latency of the reflector and
//scatterer.  This block contains the list of all internal variables
//that need to be registered and passed along in the pipeline.
//
//Previous values in the pipeline are passed to the next register on each
//clock tick.  The exception comes when an internal variable gets
//calculated.  Each time a new internal variable is calculated, a new
//case is added to the case statement, and instead of hooking previous
//values of that variable to next, the new, calculated values are hooked up.
//
//This method will generate many more registers than what are required, but
//it is expected that the synthesis tool will synthesize these away.
//
//
//Commenting Convention: Whenever a new value is injected into the pipe, the
//comment //Changed Value is added directly above the variable in question.
//When multiple values are calculated in a single clock cycle, multiple such
//comments are placed.  Wires connected to "Changed Values" always start with
//the prefix new_.
//
//GENERATE PIPELINE
//genvar i;
//generate
//	for(i=`LAT; i>0; i=i-1) begin: internalPipe
//		case(i)
//		
//		2:
//		InternalsBlock pipeReg(
//			//Inputs
//			.clock(clock),
//			.reset(reset),
//			.enable(enable),
//			
//			.i_sint(sint[i-1]),
//			.i_cost(cost[i-1]),
//			.i_sinp(sinp[i-1]),
//			.i_cosp(cosp[i-1]),
//			.i_sintCosp(sintCosp[i-1]),
//			.i_sintSinp(sintSinp[i-1]),
//			//Changed Value
//			.i_uz2(new_uz2),
//			.i_uxUz(uxUz[i-1]),
//			.i_uyUz(uyUz[i-1]),
//			.i_uySintSinp(uySintSinp[i-1]),
//			.i_oneMinusUz2(oneMinusUz2[i-1]),
//			.i_uyUzSintCosp(uyUzSintCosp[i-1]),
//			.i_uxUzSintCosp(uxUzSintCosp[i-1]),
//			.i_uxSintSinp(uxSintSinp[i-1]),
//			.i_sqrtOneMinusUz2(sqrtOneMinusUz2[i-1]),
//			.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2[i-1]),
//			.i_uxCost(uxCost[i-1]),
//			.i_uzCost(uzCost[i-1]),
//			.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv[i-1]),
//			.i_uxNumerator(uxNumerator[i-1]),
//			.i_uyNumerator(uyNumerator[i-1]),
//			.i_uyCost(uyCost[i-1]),
//			.i_uxQuotient(uxQuotient[i-1]),
//			.i_uyQuotient(uyQuotient[i-1]),
//			
//			//Outputs			
//			.o_sint(sint[i]),
//			.o_cost(cost[i]),
//			.o_sinp(sinp[i]),
//			.o_cosp(cosp[i]),
//			.o_sintCosp(sintCosp[i]),
//			.o_sintSinp(sintSinp[i]),
//			.o_uz2(uz2[i]),
//			.o_uxUz(uxUz[i]),
//			.o_uyUz(uyUz[i]),
//			.o_uySintSinp(uySintSinp[i]),
//			.o_oneMinusUz2(oneMinusUz2[i]),
//			.o_uyUzSintCosp(uyUzSintCosp[i]),
//			.o_uxUzSintCosp(uxUzSintCosp[i]),
//			.o_uxSintSinp(uxSintSinp[i]),
//			.o_sqrtOneMinusUz2(sqrtOneMinusUz2[i]),
//			.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2[i]),
//			.o_uxCost(uxCost[i]),
//			.o_uzCost(uzCost[i]),
//			.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv[i]),
//			.o_uxNumerator(uxNumerator[i]),
//			.o_uyNumerator(uyNumerator[i]),
//			.o_uyCost(uyCost[i]),
//			.o_uxQuotient(uxQuotient[i]),
//			.o_uyQuotient(uyQuotient[i])
//		);
//		3:
//		InternalsBlock pipeReg(
//			//Inputs
//			.clock(clock),
//			.reset(reset),
//			.enable(enable),
//			
//			.i_sint(sint[i-1]),
//			.i_cost(cost[i-1]),
//			.i_sinp(sinp[i-1]),
//			.i_cosp(cosp[i-1]),
//			.i_sintCosp(sintCosp[i-1]),
//			.i_sintSinp(sintSinp[i-1]),
//			.i_uz2(uz2[i-1]),
//			.i_uxUz(uxUz[i-1]),
//			.i_uyUz(uyUz[i-1]),
//			.i_uySintSinp(uySintSinp[i-1]),
//			//Changed Value
//			.i_oneMinusUz2(new_oneMinusUz2),
//			.i_uyUzSintCosp(uyUzSintCosp[i-1]),
//			.i_uxUzSintCosp(uxUzSintCosp[i-1]),
//			.i_uxSintSinp(uxSintSinp[i-1]),
//			.i_sqrtOneMinusUz2(sqrtOneMinusUz2[i-1]),
//			.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2[i-1]),
//			.i_uxCost(uxCost[i-1]),
//			.i_uzCost(uzCost[i-1]),
//			.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv[i-1]),
//			.i_uxNumerator(uxNumerator[i-1]),
//			.i_uyNumerator(uyNumerator[i-1]),
//			.i_uyCost(uyCost[i-1]),
//			.i_uxQuotient(uxQuotient[i-1]),
//			.i_uyQuotient(uyQuotient[i-1]),
//			
//			//Outputs			
//			.o_sint(sint[i]),
//			.o_cost(cost[i]),
//			.o_sinp(sinp[i]),
//			.o_cosp(cosp[i]),
//			.o_sintCosp(sintCosp[i]),
//			.o_sintSinp(sintSinp[i]),
//			.o_uz2(uz2[i]),
//			.o_uxUz(uxUz[i]),
//			.o_uyUz(uyUz[i]),
//			.o_uySintSinp(uySintSinp[i]),
//			.o_oneMinusUz2(oneMinusUz2[i]),
//			.o_uyUzSintCosp(uyUzSintCosp[i]),
//			.o_uxUzSintCosp(uxUzSintCosp[i]),
//			.o_uxSintSinp(uxSintSinp[i]),
//			.o_sqrtOneMinusUz2(sqrtOneMinusUz2[i]),
//			.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2[i]),
//			.o_uxCost(uxCost[i]),
//			.o_uzCost(uzCost[i]),
//			.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv[i]),
//			.o_uxNumerator(uxNumerator[i]),
//			.o_uyNumerator(uyNumerator[i]),
//			.o_uyCost(uyCost[i]),
//			.o_uxQuotient(uxQuotient[i]),
//			.o_uyQuotient(uyQuotient[i])
//		);
//		4:
//		InternalsBlock pipeReg(
//			//Inputs
//			.clock(clock),
//			.reset(reset),
//			.enable(enable),
//			
//			.i_sint(sint[i-1]),
//			.i_cost(cost[i-1]),
//			.i_sinp(sinp[i-1]),
//			.i_cosp(cosp[i-1]),
//			.i_sintCosp(sintCosp[i-1]),
//			.i_sintSinp(sintSinp[i-1]),
//			.i_uz2(uz2[i-1]),
//			//Changed Value
//			.i_uxUz(new_uxUz),
//			.i_uyUz(uyUz[i-1]),
//			.i_uySintSinp(uySintSinp[i-1]),
//			.i_oneMinusUz2(oneMinusUz2[i-1]),
//			.i_uyUzSintCosp(uyUzSintCosp[i-1]),
//			.i_uxUzSintCosp(uxUzSintCosp[i-1]),
//			.i_uxSintSinp(uxSintSinp[i-1]),
//			.i_sqrtOneMinusUz2(sqrtOneMinusUz2[i-1]),
//			.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2[i-1]),
//			.i_uxCost(uxCost[i-1]),
//			.i_uzCost(uzCost[i-1]),
//			.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv[i-1]),
//			.i_uxNumerator(uxNumerator[i-1]),
//			.i_uyNumerator(uyNumerator[i-1]),
//			.i_uyCost(uyCost[i-1]),
//			.i_uxQuotient(uxQuotient[i-1]),
//			.i_uyQuotient(uyQuotient[i-1]),
//			
//			//Outputs			
//			.o_sint(sint[i]),
//			.o_cost(cost[i]),
//			.o_sinp(sinp[i]),
//			.o_cosp(cosp[i]),
//			.o_sintCosp(sintCosp[i]),
//			.o_sintSinp(sintSinp[i]),
//			.o_uz2(uz2[i]),
//			.o_uxUz(uxUz[i]),
//			.o_uyUz(uyUz[i]),
//			.o_uySintSinp(uySintSinp[i]),
//			.o_oneMinusUz2(oneMinusUz2[i]),
//			.o_uyUzSintCosp(uyUzSintCosp[i]),
//			.o_uxUzSintCosp(uxUzSintCosp[i]),
//			.o_uxSintSinp(uxSintSinp[i]),
//			.o_sqrtOneMinusUz2(sqrtOneMinusUz2[i]),
//			.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2[i]),
//			.o_uxCost(uxCost[i]),
//			.o_uzCost(uzCost[i]),
//			.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv[i]),
//			.o_uxNumerator(uxNumerator[i]),
//			.o_uyNumerator(uyNumerator[i]),
//			.o_uyCost(uyCost[i]),
//			.o_uxQuotient(uxQuotient[i]),
//			.o_uyQuotient(uyQuotient[i])
//		);
//		(`SQRT+6):
//		InternalsBlock pipeReg(
//			//Inputs
//			.clock(clock),
//			.reset(reset),
//			.enable(enable),
//			
//			.i_sint(sint[i-1]),
//			.i_cost(cost[i-1]),
//			.i_sinp(sinp[i-1]),
//			.i_cosp(cosp[i-1]),
//			.i_sintCosp(sintCosp[i-1]),
//			.i_sintSinp(sintSinp[i-1]),
//			.i_uz2(uz2[i-1]),
//			.i_uxUz(uxUz[i-1]),
//			.i_uyUz(uyUz[i-1]),
//			.i_uySintSinp(uySintSinp[i-1]),
//			.i_oneMinusUz2(oneMinusUz2[i-1]),
//			.i_uyUzSintCosp(uyUzSintCosp[i-1]),
//			.i_uxUzSintCosp(uxUzSintCosp[i-1]),
//			.i_uxSintSinp(uxSintSinp[i-1]),
//			//Changed Value
//			.i_sqrtOneMinusUz2(new_sqrtOneMinusUz2),
//			.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2[i-1]),
//			.i_uxCost(uxCost[i-1]),
//			.i_uzCost(uzCost[i-1]),
//			.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv[i-1]),
//			.i_uxNumerator(uxNumerator[i-1]),
//			.i_uyNumerator(uyNumerator[i-1]),
//			.i_uyCost(uyCost[i-1]),
//			.i_uxQuotient(uxQuotient[i-1]),
//			.i_uyQuotient(uyQuotient[i-1]),
//			
//			//Outputs			
//			.o_sint(sint[i]),
//			.o_cost(cost[i]),
//			.o_sinp(sinp[i]),
//			.o_cosp(cosp[i]),
//			.o_sintCosp(sintCosp[i]),
//			.o_sintSinp(sintSinp[i]),
//			.o_uz2(uz2[i]),
//			.o_uxUz(uxUz[i]),
//			.o_uyUz(uyUz[i]),
//			.o_uySintSinp(uySintSinp[i]),
//			.o_oneMinusUz2(oneMinusUz2[i]),
//			.o_uyUzSintCosp(uyUzSintCosp[i]),
//			.o_uxUzSintCosp(uxUzSintCosp[i]),
//			.o_uxSintSinp(uxSintSinp[i]),
//			.o_sqrtOneMinusUz2(sqrtOneMinusUz2[i]),
//			.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2[i]),
//			.o_uxCost(uxCost[i]),
//			.o_uzCost(uzCost[i]),
//			.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv[i]),
//			.o_uxNumerator(uxNumerator[i]),
//			.o_uyNumerator(uyNumerator[i]),
//			.o_uyCost(uyCost[i]),
//			.o_uxQuotient(uxQuotient[i]),
//			.o_uyQuotient(uyQuotient[i])
//		);
//		
//		(`SQRT+`DIV+3):
//		InternalsBlock pipeReg(
//			//Inputs
//			.clock(clock),
//			.reset(reset),
//			.enable(enable),
//			
//			//Changed Value
//			.i_sint(new_sint),
//			//Changed Value
//			.i_cost(new_cost),
//			//Changed Value
//			.i_sinp(new_sinp),
//			//Changed Value
//			.i_cosp(new_cosp),
//			//Changed Value
//			.i_sintCosp(new_sintCosp),
//			//Changed Value
//			.i_sintSinp(new_sintSinp),
//			.i_uz2(uz2[i-1]),
//			.i_uxUz(uxUz[i-1]),
//			//Changed Value
//			.i_uyUz(new_uyUz),
//			.i_uySintSinp(uySintSinp[i-1]),
//			.i_oneMinusUz2(oneMinusUz2[i-1]),
//			.i_uyUzSintCosp(uyUzSintCosp[i-1]),
//			.i_uxUzSintCosp(uxUzSintCosp[i-1]),
//			.i_uxSintSinp(uxSintSinp[i-1]),
//			.i_sqrtOneMinusUz2(sqrtOneMinusUz2[i-1]),
//			.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2[i-1]),
//			.i_uxCost(uxCost[i-1]),
//			.i_uzCost(uzCost[i-1]),
//			.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv[i-1]),
//			.i_uxNumerator(uxNumerator[i-1]),
//			.i_uyNumerator(uyNumerator[i-1]),
//			.i_uyCost(uyCost[i-1]),
//			.i_uxQuotient(uxQuotient[i-1]),
//			.i_uyQuotient(uyQuotient[i-1]),
//			
//			//Outputs			
//			.o_sint(sint[i]),
//			.o_cost(cost[i]),
//			.o_sinp(sinp[i]),
//			.o_cosp(cosp[i]),
//			.o_sintCosp(sintCosp[i]),
//			.o_sintSinp(sintSinp[i]),
//			.o_uz2(uz2[i]),
//			.o_uxUz(uxUz[i]),
//			.o_uyUz(uyUz[i]),
//			.o_uySintSinp(uySintSinp[i]),
//			.o_oneMinusUz2(oneMinusUz2[i]),
//			.o_uyUzSintCosp(uyUzSintCosp[i]),
//			.o_uxUzSintCosp(uxUzSintCosp[i]),
//			.o_uxSintSinp(uxSintSinp[i]),
//			.o_sqrtOneMinusUz2(sqrtOneMinusUz2[i]),
//			.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2[i]),
//			.o_uxCost(uxCost[i]),
//			.o_uzCost(uzCost[i]),
//			.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv[i]),
//			.o_uxNumerator(uxNumerator[i]),
//			.o_uyNumerator(uyNumerator[i]),
//			.o_uyCost(uyCost[i]),
//			.o_uxQuotient(uxQuotient[i]),
//			.o_uyQuotient(uyQuotient[i])
//		);
//		
//		(`SQRT+`DIV+4):
//		InternalsBlock pipeReg(
//			//Inputs
//			.clock(clock),
//			.reset(reset),
//			.enable(enable),
//			
//			.i_sint(sint[i-1]),
//			.i_cost(cost[i-1]),
//			.i_sinp(sinp[i-1]),
//			.i_cosp(cosp[i-1]),
//			.i_sintCosp(sintCosp[i-1]),
//			.i_sintSinp(sintSinp[i-1]),
//			.i_uz2(uz2[i-1]),
//			.i_uxUz(uxUz[i-1]),
//			.i_uyUz(uyUz[i-1]),
//			//Changed Value
//			.i_uySintSinp(new_uySintSinp),
//			.i_oneMinusUz2(oneMinusUz2[i-1]),
//			//Changed Value
//			.i_uyUzSintCosp(new_uyUzSintCosp),
//			//Changed Value
//			.i_uxUzSintCosp(new_uxUzSintCosp),
//			//Changed Value
//			.i_uxSintSinp(new_uxSintSinp),
//			.i_sqrtOneMinusUz2(sqrtOneMinusUz2[i-1]),
//			.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2[i-1]),
//			.i_uxCost(uxCost[i-1]),
//			.i_uzCost(uzCost[i-1]),
//			.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv[i-1]),
//			.i_uxNumerator(uxNumerator[i-1]),
//			.i_uyNumerator(uyNumerator[i-1]),
//			.i_uyCost(uyCost[i-1]),
//			.i_uxQuotient(uxQuotient[i-1]),
//			.i_uyQuotient(uyQuotient[i-1]),
//			
//			//Outputs			
//			.o_sint(sint[i]),
//			.o_cost(cost[i]),
//			.o_sinp(sinp[i]),
//			.o_cosp(cosp[i]),
//			.o_sintCosp(sintCosp[i]),
//			.o_sintSinp(sintSinp[i]),
//			.o_uz2(uz2[i]),
//			.o_uxUz(uxUz[i]),
//			.o_uyUz(uyUz[i]),
//			.o_uySintSinp(uySintSinp[i]),
//			.o_oneMinusUz2(oneMinusUz2[i]),
//			.o_uyUzSintCosp(uyUzSintCosp[i]),
//			.o_uxUzSintCosp(uxUzSintCosp[i]),
//			.o_uxSintSinp(uxSintSinp[i]),
//			.o_sqrtOneMinusUz2(sqrtOneMinusUz2[i]),
//			.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2[i]),
//			.o_uxCost(uxCost[i]),
//			.o_uzCost(uzCost[i]),
//			.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv[i]),
//			.o_uxNumerator(uxNumerator[i]),
//			.o_uyNumerator(uyNumerator[i]),
//			.o_uyCost(uyCost[i]),
//			.o_uxQuotient(uxQuotient[i]),
//			.o_uyQuotient(uyQuotient[i])
//		);
//		
//		(`SQRT+`DIV+5):
//		InternalsBlock pipeReg(
//			//Inputs
//			.clock(clock),
//			.reset(reset),
//			.enable(enable),
//			
//			.i_sint(sint[i-1]),
//			.i_cost(cost[i-1]),
//			.i_sinp(sinp[i-1]),
//			.i_cosp(cosp[i-1]),
//			.i_sintCosp(sintCosp[i-1]),
//			.i_sintSinp(sintSinp[i-1]),
//			.i_uz2(uz2[i-1]),
//			.i_uxUz(uxUz[i-1]),
//			.i_uyUz(uyUz[i-1]),
//			.i_uySintSinp(uySintSinp[i-1]),
//			.i_oneMinusUz2(oneMinusUz2[i-1]),
//			.i_uyUzSintCosp(uyUzSintCosp[i-1]),
//			.i_uxUzSintCosp(uxUzSintCosp[i-1]),
//			.i_uxSintSinp(uxSintSinp[i-1]),
//			.i_sqrtOneMinusUz2(sqrtOneMinusUz2[i-1]),
//			.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2[i-1]),
//			.i_uxCost(uxCost[i-1]),
//			.i_uzCost(uzCost[i-1]),
//			//Changed Value
//			.i_sqrtOneMinusUz2_inv(new_sqrtOneMinusUz2_inv),
//			//Changed Value
//			.i_uxNumerator(new_uxNumerator),
//			//Changed Value
//			.i_uyNumerator(new_uyNumerator),
//			.i_uyCost(uyCost[i-1]),
//			.i_uxQuotient(uxQuotient[i-1]),
//			.i_uyQuotient(uyQuotient[i-1]),
//			
//			//Outputs			
//			.o_sint(sint[i]),
//			.o_cost(cost[i]),
//			.o_sinp(sinp[i]),
//			.o_cosp(cosp[i]),
//			.o_sintCosp(sintCosp[i]),
//			.o_sintSinp(sintSinp[i]),
//			.o_uz2(uz2[i]),
//			.o_uxUz(uxUz[i]),
//			.o_uyUz(uyUz[i]),
//			.o_uySintSinp(uySintSinp[i]),
//			.o_oneMinusUz2(oneMinusUz2[i]),
//			.o_uyUzSintCosp(uyUzSintCosp[i]),
//			.o_uxUzSintCosp(uxUzSintCosp[i]),
//			.o_uxSintSinp(uxSintSinp[i]),
//			.o_sqrtOneMinusUz2(sqrtOneMinusUz2[i]),
//			.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2[i]),
//			.o_uxCost(uxCost[i]),
//			.o_uzCost(uzCost[i]),
//			.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv[i]),
//			.o_uxNumerator(uxNumerator[i]),
//			.o_uyNumerator(uyNumerator[i]),
//			.o_uyCost(uyCost[i]),
//			.o_uxQuotient(uxQuotient[i]),
//			.o_uyQuotient(uyQuotient[i])
//		);
//		
//		(`SQRT+`DIV+6):
//		InternalsBlock pipeReg(
//			//Inputs
//			.clock(clock),
//			.reset(reset),
//			.enable(enable),
//
//			.i_sint(sint[i-1]),
//			.i_cost(cost[i-1]),
//			.i_sinp(sinp[i-1]),
//			.i_cosp(cosp[i-1]),
//			.i_sintCosp(sintCosp[i-1]),
//			.i_sintSinp(sintSinp[i-1]),
//			.i_uz2(uz2[i-1]),
//			.i_uxUz(uxUz[i-1]),
//			.i_uyUz(uyUz[i-1]),
//			.i_uySintSinp(uySintSinp[i-1]),
//			.i_oneMinusUz2(oneMinusUz2[i-1]),
//			.i_uyUzSintCosp(uyUzSintCosp[i-1]),
//			.i_uxUzSintCosp(uxUzSintCosp[i-1]),
//			.i_uxSintSinp(uxSintSinp[i-1]),
//			.i_sqrtOneMinusUz2(sqrtOneMinusUz2[i-1]),
//			//Changed Value
//			.i_sintCospSqrtOneMinusUz2(new_sintCospSqrtOneMinusUz2),
//			//Changed Value
//			.i_uxCost(new_uxCost),
//			//Changed Value
//			.i_uzCost(new_uzCost),
//			.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv[i-1]),
//			.i_uxNumerator(uxNumerator[i-1]),
//			.i_uyNumerator(uyNumerator[i-1]),
//			//Changed Value
//			.i_uyCost(new_uyCost),
//			//Changed Value
//			.i_uxQuotient(new_uxQuotient),
//			//Changed Value
//			.i_uyQuotient(new_uyQuotient),
//			
//			//Outputs			
//			.o_sint(sint[i]),
//			.o_cost(cost[i]),
//			.o_sinp(sinp[i]),
//			.o_cosp(cosp[i]),
//			.o_sintCosp(sintCosp[i]),
//			.o_sintSinp(sintSinp[i]),
//			.o_uz2(uz2[i]),
//			.o_uxUz(uxUz[i]),
//			.o_uyUz(uyUz[i]),
//			.o_uySintSinp(uySintSinp[i]),
//			.o_oneMinusUz2(oneMinusUz2[i]),
//			.o_uyUzSintCosp(uyUzSintCosp[i]),
//			.o_uxUzSintCosp(uxUzSintCosp[i]),
//			.o_uxSintSinp(uxSintSinp[i]),
//			.o_sqrtOneMinusUz2(sqrtOneMinusUz2[i]),
//			.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2[i]),
//			.o_uxCost(uxCost[i]),
//			.o_uzCost(uzCost[i]),
//			.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv[i]),
//			.o_uxNumerator(uxNumerator[i]),
//			.o_uyNumerator(uyNumerator[i]),
//			.o_uyCost(uyCost[i]),
//			.o_uxQuotient(uxQuotient[i]),
//			.o_uyQuotient(uyQuotient[i])
//		);
//		
//		default:
//		InternalsBlock pipeReg(
//			//Inputs
//			.clock(clock),
//			.reset(reset),
//			.enable(enable),
//			
//			.i_sint(sint[i-1]),
//			.i_cost(cost[i-1]),
//			.i_sinp(sinp[i-1]),
//			.i_cosp(cosp[i-1]),
//			.i_sintCosp(sintCosp[i-1]),
//			.i_sintSinp(sintSinp[i-1]),
//			.i_uz2(uz2[i-1]),
//			.i_uxUz(uxUz[i-1]),
//			.i_uyUz(uyUz[i-1]),
//			.i_uySintSinp(uySintSinp[i-1]),
//			.i_oneMinusUz2(oneMinusUz2[i-1]),
//			.i_uyUzSintCosp(uyUzSintCosp[i-1]),
//			.i_uxUzSintCosp(uxUzSintCosp[i-1]),
//			.i_uxSintSinp(uxSintSinp[i-1]),
//			.i_sqrtOneMinusUz2(sqrtOneMinusUz2[i-1]),
//			.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2[i-1]),
//			.i_uxCost(uxCost[i-1]),
//			.i_uzCost(uzCost[i-1]),
//			.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv[i-1]),
//			.i_uxNumerator(uxNumerator[i-1]),
//			.i_uyNumerator(uyNumerator[i-1]),
//			.i_uyCost(uyCost[i-1]),
//			.i_uxQuotient(uxQuotient[i-1]),
//			.i_uyQuotient(uyQuotient[i-1]),
//			
//			//Outputs			
//			.o_sint(sint[i]),
//			.o_cost(cost[i]),
//			.o_sinp(sinp[i]),
//			.o_cosp(cosp[i]),
//			.o_sintCosp(sintCosp[i]),
//			.o_sintSinp(sintSinp[i]),
//			.o_uz2(uz2[i]),
//			.o_uxUz(uxUz[i]),
//			.o_uyUz(uyUz[i]),
//			.o_uySintSinp(uySintSinp[i]),
//			.o_oneMinusUz2(oneMinusUz2[i]),
//			.o_uyUzSintCosp(uyUzSintCosp[i]),
//			.o_uxUzSintCosp(uxUzSintCosp[i]),
//			.o_uxSintSinp(uxSintSinp[i]),
//			.o_sqrtOneMinusUz2(sqrtOneMinusUz2[i]),
//			.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2[i]),
//			.o_uxCost(uxCost[i]),
//			.o_uzCost(uzCost[i]),
//			.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv[i]),
//			.o_uxNumerator(uxNumerator[i]),
//			.o_uyNumerator(uyNumerator[i]),
//			.o_uyCost(uyCost[i]),
//			.o_uxQuotient(uxQuotient[i]),
//			.o_uyQuotient(uyQuotient[i])
//		);
//		endcase
//	end
//endgenerate	

//Expanded generate loop:
//special cases first
		//forloop2
		InternalsBlock pipeReg2(
			//Inputs
			.clock(clock),
			.reset(reset),
			.enable(enable),
			
			.i_sint(sint__1),
			.i_cost(cost__1),
			.i_sinp(sinp__1),
			.i_cosp(cosp__1),
			.i_sintCosp(sintCosp__1),
			.i_sintSinp(sintSinp__1),
			//Changed Value
			.i_uz2(new_uz2),
			.i_uxUz(uxUz__1),
			.i_uyUz(uyUz__1),
			.i_uySintSinp(uySintSinp__1),
			.i_oneMinusUz2(oneMinusUz2__1),
			.i_uyUzSintCosp(uyUzSintCosp__1),
			.i_uxUzSintCosp(uxUzSintCosp__1),
			.i_uxSintSinp(uxSintSinp__1),
			.i_sqrtOneMinusUz2(sqrtOneMinusUz2__1),
			.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__1),
			.i_uxCost(uxCost__1),
			.i_uzCost(uzCost__1),
			.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__1),
			.i_uxNumerator(uxNumerator__1),
			.i_uyNumerator(uyNumerator__1),
			.i_uyCost(uyCost__1),
			.i_uxQuotient(uxQuotient__1),
			.i_uyQuotient(uyQuotient__1),
			
			//Outputs			
			.o_sint(sint__2),
			.o_cost(cost__2),
			.o_sinp(sinp__2),
			.o_cosp(cosp__2),
			.o_sintCosp(sintCosp__2),
			.o_sintSinp(sintSinp__2),
			.o_uz2(uz2__2),
			.o_uxUz(uxUz__2),
			.o_uyUz(uyUz__2),
			.o_uySintSinp(uySintSinp__2),
			.o_oneMinusUz2(oneMinusUz2__2),
			.o_uyUzSintCosp(uyUzSintCosp__2),
			.o_uxUzSintCosp(uxUzSintCosp__2),
			.o_uxSintSinp(uxSintSinp__2),
			.o_sqrtOneMinusUz2(sqrtOneMinusUz2__2),
			.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__2),
			.o_uxCost(uxCost__2),
			.o_uzCost(uzCost__2),
			.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__2),
			.o_uxNumerator(uxNumerator__2),
			.o_uyNumerator(uyNumerator__2),
			.o_uyCost(uyCost__2),
			.o_uxQuotient(uxQuotient__2),
			.o_uyQuotient(uyQuotient__2)
		);
		
		
	//	forloop3
		InternalsBlock pipeReg3(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__2),
.i_cost(cost__2),
.i_sinp(sinp__2),
.i_cosp(cosp__2),
.i_sintCosp(sintCosp__2),
.i_sintSinp(sintSinp__2),
.i_uz2(uz2__2),
.i_uxUz(uxUz__2),
.i_uyUz(uyUz__2),
.i_uySintSinp(uySintSinp__2),
//changed
.i_oneMinusUz2(new_oneMinusUz2),
.i_uyUzSintCosp(uyUzSintCosp__2),
.i_uxUzSintCosp(uxUzSintCosp__2),
.i_uxSintSinp(uxSintSinp__2),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__2),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__2),
.i_uxCost(uxCost__2),
.i_uzCost(uzCost__2),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__2),
.i_uxNumerator(uxNumerator__2),
.i_uyNumerator(uyNumerator__2),
.i_uyCost(uyCost__2),
.i_uxQuotient(uxQuotient__2),
.i_uyQuotient(uyQuotient__2),
//Outputs			 
.o_sint(sint__3),
.o_cost(cost__3),
.o_sinp(sinp__3),
.o_cosp(cosp__3),
.o_sintCosp(sintCosp__3),
.o_sintSinp(sintSinp__3),
.o_uz2(uz2__3),
.o_uxUz(uxUz__3),
.o_uyUz(uyUz__3),
.o_uySintSinp(uySintSinp__3),
.o_oneMinusUz2(oneMinusUz2__3),
.o_uyUzSintCosp(uyUzSintCosp__3),
.o_uxUzSintCosp(uxUzSintCosp__3),
.o_uxSintSinp(uxSintSinp__3),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__3),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__3),
.o_uxCost(uxCost__3),
.o_uzCost(uzCost__3),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__3),
.o_uxNumerator(uxNumerator__3),
.o_uyNumerator(uyNumerator__3),
.o_uyCost(uyCost__3),
.o_uxQuotient(uxQuotient__3),
.o_uyQuotient(uyQuotient__3)
);  

		//forloop4:
		InternalsBlock pipeReg4(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__3),
.i_cost(cost__3),
.i_sinp(sinp__3),
.i_cosp(cosp__3),
.i_sintCosp(sintCosp__3),
.i_sintSinp(sintSinp__3),
.i_uz2(uz2__3),
//changed
.i_uxUz(new_uxUz),
.i_uyUz(uyUz__3),
.i_uySintSinp(uySintSinp__3),
.i_oneMinusUz2(oneMinusUz2__3),
.i_uyUzSintCosp(uyUzSintCosp__3),
.i_uxUzSintCosp(uxUzSintCosp__3),
.i_uxSintSinp(uxSintSinp__3),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__3),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__3),
.i_uxCost(uxCost__3),
.i_uzCost(uzCost__3),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__3),
.i_uxNumerator(uxNumerator__3),
.i_uyNumerator(uyNumerator__3),
.i_uyCost(uyCost__3),
.i_uxQuotient(uxQuotient__3),
.i_uyQuotient(uyQuotient__3),
//Outputs			 
.o_sint(sint__4),
.o_cost(cost__4),
.o_sinp(sinp__4),
.o_cosp(cosp__4),
.o_sintCosp(sintCosp__4),
.o_sintSinp(sintSinp__4),
.o_uz2(uz2__4),
.o_uxUz(uxUz__4),
.o_uyUz(uyUz__4),
.o_uySintSinp(uySintSinp__4),
.o_oneMinusUz2(oneMinusUz2__4),
.o_uyUzSintCosp(uyUzSintCosp__4),
.o_uxUzSintCosp(uxUzSintCosp__4),
.o_uxSintSinp(uxSintSinp__4),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__4),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__4),
.o_uxCost(uxCost__4),
.o_uzCost(uzCost__4),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__4),
.o_uxNumerator(uxNumerator__4),
.o_uyNumerator(uyNumerator__4),
.o_uyCost(uyCost__4),
.o_uxQuotient(uxQuotient__4),
.o_uyQuotient(uyQuotient__4)
);  
		
InternalsBlock pipeReg16(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__15),
.i_cost(cost__15),
.i_sinp(sinp__15),
.i_cosp(cosp__15),
.i_sintCosp(sintCosp__15),
.i_sintSinp(sintSinp__15),
.i_uz2(uz2__15),
.i_uxUz(uxUz__15),
.i_uyUz(uyUz__15),
.i_uySintSinp(uySintSinp__15),
.i_oneMinusUz2(oneMinusUz2__15),
.i_uyUzSintCosp(uyUzSintCosp__15),
.i_uxUzSintCosp(uxUzSintCosp__15),
.i_uxSintSinp(uxSintSinp__15),
//changed
.i_sqrtOneMinusUz2(new_sqrtOneMinusUz2),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__15),
.i_uxCost(uxCost__15),
.i_uzCost(uzCost__15),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__15),
.i_uxNumerator(uxNumerator__15),
.i_uyNumerator(uyNumerator__15),
.i_uyCost(uyCost__15),
.i_uxQuotient(uxQuotient__15),
.i_uyQuotient(uyQuotient__15),
//Outputs			 
.o_sint(sint__16),
.o_cost(cost__16),
.o_sinp(sinp__16),
.o_cosp(cosp__16),
.o_sintCosp(sintCosp__16),
.o_sintSinp(sintSinp__16),
.o_uz2(uz2__16),
.o_uxUz(uxUz__16),
.o_uyUz(uyUz__16),
.o_uySintSinp(uySintSinp__16),
.o_oneMinusUz2(oneMinusUz2__16),
.o_uyUzSintCosp(uyUzSintCosp__16),
.o_uxUzSintCosp(uxUzSintCosp__16),
.o_uxSintSinp(uxSintSinp__16),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__16),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__16),
.o_uxCost(uxCost__16),
.o_uzCost(uzCost__16),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__16),
.o_uxNumerator(uxNumerator__16),
.o_uyNumerator(uyNumerator__16),
.o_uyCost(uyCost__16),
.o_uxQuotient(uxQuotient__16),
.o_uyQuotient(uyQuotient__16)
);  
		
		//forloop 33 (10+20+3):
		
InternalsBlock pipeReg33(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
//changed
.i_sint(new_sint),
//changed
.i_cost(new_cost),
//changed
.i_sinp(new_sinp),
//changed
.i_cosp(new_cosp),
//changed
.i_sintCosp(new_sintCosp),
//changed
.i_sintSinp(new_sintSinp),
.i_uz2(uz2__32),
.i_uxUz(uxUz__32),
//changed
.i_uyUz(new_uyUz),
.i_uySintSinp(uySintSinp__32),
.i_oneMinusUz2(oneMinusUz2__32),
.i_uyUzSintCosp(uyUzSintCosp__32),
.i_uxUzSintCosp(uxUzSintCosp__32),
.i_uxSintSinp(uxSintSinp__32),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__32),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__32),
.i_uxCost(uxCost__32),
.i_uzCost(uzCost__32),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__32),
.i_uxNumerator(uxNumerator__32),
.i_uyNumerator(uyNumerator__32),
.i_uyCost(uyCost__32),
.i_uxQuotient(uxQuotient__32),
.i_uyQuotient(uyQuotient__32),
//Outputs			 
.o_sint(sint__33),
.o_cost(cost__33),
.o_sinp(sinp__33),
.o_cosp(cosp__33),
.o_sintCosp(sintCosp__33),
.o_sintSinp(sintSinp__33),
.o_uz2(uz2__33),
.o_uxUz(uxUz__33),
.o_uyUz(uyUz__33),
.o_uySintSinp(uySintSinp__33),
.o_oneMinusUz2(oneMinusUz2__33),
.o_uyUzSintCosp(uyUzSintCosp__33),
.o_uxUzSintCosp(uxUzSintCosp__33),
.o_uxSintSinp(uxSintSinp__33),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__33),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__33),
.o_uxCost(uxCost__33),
.o_uzCost(uzCost__33),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__33),
.o_uxNumerator(uxNumerator__33),
.o_uyNumerator(uyNumerator__33),
.o_uyCost(uyCost__33),
.o_uxQuotient(uxQuotient__33),
.o_uyQuotient(uyQuotient__33)
);  
		
		//forloop34 (10+20+4):
		
InternalsBlock pipeReg34(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__33),
.i_cost(cost__33),
.i_sinp(sinp__33),
.i_cosp(cosp__33),
.i_sintCosp(sintCosp__33),
.i_sintSinp(sintSinp__33),
.i_uz2(uz2__33),
.i_uxUz(uxUz__33),
.i_uyUz(uyUz__33),
//changed
.i_uySintSinp(new_uySintSinp),
.i_oneMinusUz2(oneMinusUz2__33),
//changed
.i_uyUzSintCosp(new_uyUzSintCosp),
//changed
.i_uxUzSintCosp(new_uxUzSintCosp),
//changed
.i_uxSintSinp(new_uxSintSinp),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__33),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__33),
.i_uxCost(uxCost__33),
.i_uzCost(uzCost__33),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__33),
.i_uxNumerator(uxNumerator__33),
.i_uyNumerator(uyNumerator__33),
.i_uyCost(uyCost__33),
.i_uxQuotient(uxQuotient__33),
.i_uyQuotient(uyQuotient__33),
//Outputs			 
.o_sint(sint__34),
.o_cost(cost__34),
.o_sinp(sinp__34),
.o_cosp(cosp__34),
.o_sintCosp(sintCosp__34),
.o_sintSinp(sintSinp__34),
.o_uz2(uz2__34),
.o_uxUz(uxUz__34),
.o_uyUz(uyUz__34),
.o_uySintSinp(uySintSinp__34),
.o_oneMinusUz2(oneMinusUz2__34),
.o_uyUzSintCosp(uyUzSintCosp__34),
.o_uxUzSintCosp(uxUzSintCosp__34),
.o_uxSintSinp(uxSintSinp__34),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__34),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__34),
.o_uxCost(uxCost__34),
.o_uzCost(uzCost__34),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__34),
.o_uxNumerator(uxNumerator__34),
.o_uyNumerator(uyNumerator__34),
.o_uyCost(uyCost__34),
.o_uxQuotient(uxQuotient__34),
.o_uyQuotient(uyQuotient__34)
);  
		
		//forloop35(10+20+5):
		InternalsBlock pipeReg35(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__34),
.i_cost(cost__34),
.i_sinp(sinp__34),
.i_cosp(cosp__34),
.i_sintCosp(sintCosp__34),
.i_sintSinp(sintSinp__34),
.i_uz2(uz2__34),
.i_uxUz(uxUz__34),
.i_uyUz(uyUz__34),
.i_uySintSinp(uySintSinp__34),
.i_oneMinusUz2(oneMinusUz2__34),
.i_uyUzSintCosp(uyUzSintCosp__34),
.i_uxUzSintCosp(uxUzSintCosp__34),
.i_uxSintSinp(uxSintSinp__34),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__34),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__34),
.i_uxCost(uxCost__34),
.i_uzCost(uzCost__34),
//changedval
.i_sqrtOneMinusUz2_inv(new_sqrtOneMinusUz2_inv),
//changed
.i_uxNumerator(new_uxNumerator),
//changed
.i_uyNumerator(new_uyNumerator),
.i_uyCost(uyCost__34),
.i_uxQuotient(uxQuotient__34),
.i_uyQuotient(uyQuotient__34),
//Outputs			 
.o_sint(sint__35),
.o_cost(cost__35),
.o_sinp(sinp__35),
.o_cosp(cosp__35),
.o_sintCosp(sintCosp__35),
.o_sintSinp(sintSinp__35),
.o_uz2(uz2__35),
.o_uxUz(uxUz__35),
.o_uyUz(uyUz__35),
.o_uySintSinp(uySintSinp__35),
.o_oneMinusUz2(oneMinusUz2__35),
.o_uyUzSintCosp(uyUzSintCosp__35),
.o_uxUzSintCosp(uxUzSintCosp__35),
.o_uxSintSinp(uxSintSinp__35),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__35),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__35),
.o_uxCost(uxCost__35),
.o_uzCost(uzCost__35),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__35),
.o_uxNumerator(uxNumerator__35),
.o_uyNumerator(uyNumerator__35),
.o_uyCost(uyCost__35),
.o_uxQuotient(uxQuotient__35),
.o_uyQuotient(uyQuotient__35)
);  

		
	//forloop36	(10+20+6):
		
InternalsBlock pipeReg36(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__35),
.i_cost(cost__35),
.i_sinp(sinp__35),
.i_cosp(cosp__35),
.i_sintCosp(sintCosp__35),
.i_sintSinp(sintSinp__35),
.i_uz2(uz2__35),
.i_uxUz(uxUz__35),
.i_uyUz(uyUz__35),
.i_uySintSinp(uySintSinp__35),
.i_oneMinusUz2(oneMinusUz2__35),
.i_uyUzSintCosp(uyUzSintCosp__35),
.i_uxUzSintCosp(uxUzSintCosp__35),
.i_uxSintSinp(uxSintSinp__35),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__35),
//changed
.i_sintCospSqrtOneMinusUz2(new_sintCospSqrtOneMinusUz2),
//changed
.i_uxCost(new_uxCost),
//changed
.i_uzCost(new_uzCost),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__35),
.i_uxNumerator(uxNumerator__35),
.i_uyNumerator(uyNumerator__35),
//changed
.i_uyCost(new_uyCost),
//changed),
.i_uxQuotient(new_uxQuotient),
//cahgned
.i_uyQuotient(new_uyQuotient),
//Outputs			 
.o_sint(sint__36),
.o_cost(cost__36),
.o_sinp(sinp__36),
.o_cosp(cosp__36),
.o_sintCosp(sintCosp__36),
.o_sintSinp(sintSinp__36),
.o_uz2(uz2__36),
.o_uxUz(uxUz__36),
.o_uyUz(uyUz__36),
.o_uySintSinp(uySintSinp__36),
.o_oneMinusUz2(oneMinusUz2__36),
.o_uyUzSintCosp(uyUzSintCosp__36),
.o_uxUzSintCosp(uxUzSintCosp__36),
.o_uxSintSinp(uxSintSinp__36),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__36),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__36),
.o_uxCost(uxCost__36),
.o_uzCost(uzCost__36),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__36),
.o_uxNumerator(uxNumerator__36),
.o_uyNumerator(uyNumerator__36),
.o_uyCost(uyCost__36),
.o_uxQuotient(uxQuotient__36),
.o_uyQuotient(uyQuotient__36)
);  

InternalsBlock pipeReg37(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__36),
.i_cost(cost__36),
.i_sinp(sinp__36),
.i_cosp(cosp__36),
.i_sintCosp(sintCosp__36),
.i_sintSinp(sintSinp__36),
.i_uz2(uz2__36),
.i_uxUz(uxUz__36),
.i_uyUz(uyUz__36),
.i_uySintSinp(uySintSinp__36),
.i_oneMinusUz2(oneMinusUz2__36),
.i_uyUzSintCosp(uyUzSintCosp__36),
.i_uxUzSintCosp(uxUzSintCosp__36),
.i_uxSintSinp(uxSintSinp__36),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__36),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__36),
.i_uxCost(uxCost__36),
.i_uzCost(uzCost__36),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__36),
.i_uxNumerator(uxNumerator__36),
.i_uyNumerator(uyNumerator__36),
.i_uyCost(uyCost__36),
.i_uxQuotient(uxQuotient__36),
.i_uyQuotient(uyQuotient__36),
//Outputs			 
.o_sint(sint__37),
.o_cost(cost__37),
.o_sinp(sinp__37),
.o_cosp(cosp__37),
.o_sintCosp(sintCosp__37),
.o_sintSinp(sintSinp__37),
.o_uz2(uz2__37),
.o_uxUz(uxUz__37),
.o_uyUz(uyUz__37),
.o_uySintSinp(uySintSinp__37),
.o_oneMinusUz2(oneMinusUz2__37),
.o_uyUzSintCosp(uyUzSintCosp__37),
.o_uxUzSintCosp(uxUzSintCosp__37),
.o_uxSintSinp(uxSintSinp__37),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__37),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__37),
.o_uxCost(uxCost__37),
.o_uzCost(uzCost__37),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__37),
.o_uxNumerator(uxNumerator__37),
.o_uyNumerator(uyNumerator__37),
.o_uyCost(uyCost__37),
.o_uxQuotient(uxQuotient__37),
.o_uyQuotient(uyQuotient__37)
);  



InternalsBlock pipeReg32(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__31),
.i_cost(cost__31),
.i_sinp(sinp__31),
.i_cosp(cosp__31),
.i_sintCosp(sintCosp__31),
.i_sintSinp(sintSinp__31),
.i_uz2(uz2__31),
.i_uxUz(uxUz__31),
.i_uyUz(uyUz__31),
.i_uySintSinp(uySintSinp__31),
.i_oneMinusUz2(oneMinusUz2__31),
.i_uyUzSintCosp(uyUzSintCosp__31),
.i_uxUzSintCosp(uxUzSintCosp__31),
.i_uxSintSinp(uxSintSinp__31),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__31),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__31),
.i_uxCost(uxCost__31),
.i_uzCost(uzCost__31),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__31),
.i_uxNumerator(uxNumerator__31),
.i_uyNumerator(uyNumerator__31),
.i_uyCost(uyCost__31),
.i_uxQuotient(uxQuotient__31),
.i_uyQuotient(uyQuotient__31),
//Outputs			 
.o_sint(sint__32),
.o_cost(cost__32),
.o_sinp(sinp__32),
.o_cosp(cosp__32),
.o_sintCosp(sintCosp__32),
.o_sintSinp(sintSinp__32),
.o_uz2(uz2__32),
.o_uxUz(uxUz__32),
.o_uyUz(uyUz__32),
.o_uySintSinp(uySintSinp__32),
.o_oneMinusUz2(oneMinusUz2__32),
.o_uyUzSintCosp(uyUzSintCosp__32),
.o_uxUzSintCosp(uxUzSintCosp__32),
.o_uxSintSinp(uxSintSinp__32),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__32),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__32),
.o_uxCost(uxCost__32),
.o_uzCost(uzCost__32),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__32),
.o_uxNumerator(uxNumerator__32),
.o_uyNumerator(uyNumerator__32),
.o_uyCost(uyCost__32),
.o_uxQuotient(uxQuotient__32),
.o_uyQuotient(uyQuotient__32)
);  

InternalsBlock pipeReg31(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__30),
.i_cost(cost__30),
.i_sinp(sinp__30),
.i_cosp(cosp__30),
.i_sintCosp(sintCosp__30),
.i_sintSinp(sintSinp__30),
.i_uz2(uz2__30),
.i_uxUz(uxUz__30),
.i_uyUz(uyUz__30),
.i_uySintSinp(uySintSinp__30),
.i_oneMinusUz2(oneMinusUz2__30),
.i_uyUzSintCosp(uyUzSintCosp__30),
.i_uxUzSintCosp(uxUzSintCosp__30),
.i_uxSintSinp(uxSintSinp__30),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__30),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__30),
.i_uxCost(uxCost__30),
.i_uzCost(uzCost__30),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__30),
.i_uxNumerator(uxNumerator__30),
.i_uyNumerator(uyNumerator__30),
.i_uyCost(uyCost__30),
.i_uxQuotient(uxQuotient__30),
.i_uyQuotient(uyQuotient__30),
//Outputs			 
.o_sint(sint__31),
.o_cost(cost__31),
.o_sinp(sinp__31),
.o_cosp(cosp__31),
.o_sintCosp(sintCosp__31),
.o_sintSinp(sintSinp__31),
.o_uz2(uz2__31),
.o_uxUz(uxUz__31),
.o_uyUz(uyUz__31),
.o_uySintSinp(uySintSinp__31),
.o_oneMinusUz2(oneMinusUz2__31),
.o_uyUzSintCosp(uyUzSintCosp__31),
.o_uxUzSintCosp(uxUzSintCosp__31),
.o_uxSintSinp(uxSintSinp__31),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__31),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__31),
.o_uxCost(uxCost__31),
.o_uzCost(uzCost__31),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__31),
.o_uxNumerator(uxNumerator__31),
.o_uyNumerator(uyNumerator__31),
.o_uyCost(uyCost__31),
.o_uxQuotient(uxQuotient__31),
.o_uyQuotient(uyQuotient__31)
);  

InternalsBlock pipeReg30(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__29),
.i_cost(cost__29),
.i_sinp(sinp__29),
.i_cosp(cosp__29),
.i_sintCosp(sintCosp__29),
.i_sintSinp(sintSinp__29),
.i_uz2(uz2__29),
.i_uxUz(uxUz__29),
.i_uyUz(uyUz__29),
.i_uySintSinp(uySintSinp__29),
.i_oneMinusUz2(oneMinusUz2__29),
.i_uyUzSintCosp(uyUzSintCosp__29),
.i_uxUzSintCosp(uxUzSintCosp__29),
.i_uxSintSinp(uxSintSinp__29),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__29),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__29),
.i_uxCost(uxCost__29),
.i_uzCost(uzCost__29),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__29),
.i_uxNumerator(uxNumerator__29),
.i_uyNumerator(uyNumerator__29),
.i_uyCost(uyCost__29),
.i_uxQuotient(uxQuotient__29),
.i_uyQuotient(uyQuotient__29),
//Outputs			 
.o_sint(sint__30),
.o_cost(cost__30),
.o_sinp(sinp__30),
.o_cosp(cosp__30),
.o_sintCosp(sintCosp__30),
.o_sintSinp(sintSinp__30),
.o_uz2(uz2__30),
.o_uxUz(uxUz__30),
.o_uyUz(uyUz__30),
.o_uySintSinp(uySintSinp__30),
.o_oneMinusUz2(oneMinusUz2__30),
.o_uyUzSintCosp(uyUzSintCosp__30),
.o_uxUzSintCosp(uxUzSintCosp__30),
.o_uxSintSinp(uxSintSinp__30),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__30),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__30),
.o_uxCost(uxCost__30),
.o_uzCost(uzCost__30),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__30),
.o_uxNumerator(uxNumerator__30),
.o_uyNumerator(uyNumerator__30),
.o_uyCost(uyCost__30),
.o_uxQuotient(uxQuotient__30),
.o_uyQuotient(uyQuotient__30)
);  

InternalsBlock pipeReg29(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__28),
.i_cost(cost__28),
.i_sinp(sinp__28),
.i_cosp(cosp__28),
.i_sintCosp(sintCosp__28),
.i_sintSinp(sintSinp__28),
.i_uz2(uz2__28),
.i_uxUz(uxUz__28),
.i_uyUz(uyUz__28),
.i_uySintSinp(uySintSinp__28),
.i_oneMinusUz2(oneMinusUz2__28),
.i_uyUzSintCosp(uyUzSintCosp__28),
.i_uxUzSintCosp(uxUzSintCosp__28),
.i_uxSintSinp(uxSintSinp__28),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__28),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__28),
.i_uxCost(uxCost__28),
.i_uzCost(uzCost__28),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__28),
.i_uxNumerator(uxNumerator__28),
.i_uyNumerator(uyNumerator__28),
.i_uyCost(uyCost__28),
.i_uxQuotient(uxQuotient__28),
.i_uyQuotient(uyQuotient__28),
//Outputs			 
.o_sint(sint__29),
.o_cost(cost__29),
.o_sinp(sinp__29),
.o_cosp(cosp__29),
.o_sintCosp(sintCosp__29),
.o_sintSinp(sintSinp__29),
.o_uz2(uz2__29),
.o_uxUz(uxUz__29),
.o_uyUz(uyUz__29),
.o_uySintSinp(uySintSinp__29),
.o_oneMinusUz2(oneMinusUz2__29),
.o_uyUzSintCosp(uyUzSintCosp__29),
.o_uxUzSintCosp(uxUzSintCosp__29),
.o_uxSintSinp(uxSintSinp__29),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__29),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__29),
.o_uxCost(uxCost__29),
.o_uzCost(uzCost__29),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__29),
.o_uxNumerator(uxNumerator__29),
.o_uyNumerator(uyNumerator__29),
.o_uyCost(uyCost__29),
.o_uxQuotient(uxQuotient__29),
.o_uyQuotient(uyQuotient__29)
);  

InternalsBlock pipeReg28(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__27),
.i_cost(cost__27),
.i_sinp(sinp__27),
.i_cosp(cosp__27),
.i_sintCosp(sintCosp__27),
.i_sintSinp(sintSinp__27),
.i_uz2(uz2__27),
.i_uxUz(uxUz__27),
.i_uyUz(uyUz__27),
.i_uySintSinp(uySintSinp__27),
.i_oneMinusUz2(oneMinusUz2__27),
.i_uyUzSintCosp(uyUzSintCosp__27),
.i_uxUzSintCosp(uxUzSintCosp__27),
.i_uxSintSinp(uxSintSinp__27),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__27),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__27),
.i_uxCost(uxCost__27),
.i_uzCost(uzCost__27),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__27),
.i_uxNumerator(uxNumerator__27),
.i_uyNumerator(uyNumerator__27),
.i_uyCost(uyCost__27),
.i_uxQuotient(uxQuotient__27),
.i_uyQuotient(uyQuotient__27),
//Outputs			 
.o_sint(sint__28),
.o_cost(cost__28),
.o_sinp(sinp__28),
.o_cosp(cosp__28),
.o_sintCosp(sintCosp__28),
.o_sintSinp(sintSinp__28),
.o_uz2(uz2__28),
.o_uxUz(uxUz__28),
.o_uyUz(uyUz__28),
.o_uySintSinp(uySintSinp__28),
.o_oneMinusUz2(oneMinusUz2__28),
.o_uyUzSintCosp(uyUzSintCosp__28),
.o_uxUzSintCosp(uxUzSintCosp__28),
.o_uxSintSinp(uxSintSinp__28),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__28),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__28),
.o_uxCost(uxCost__28),
.o_uzCost(uzCost__28),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__28),
.o_uxNumerator(uxNumerator__28),
.o_uyNumerator(uyNumerator__28),
.o_uyCost(uyCost__28),
.o_uxQuotient(uxQuotient__28),
.o_uyQuotient(uyQuotient__28)
);  

InternalsBlock pipeReg27(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__26),
.i_cost(cost__26),
.i_sinp(sinp__26),
.i_cosp(cosp__26),
.i_sintCosp(sintCosp__26),
.i_sintSinp(sintSinp__26),
.i_uz2(uz2__26),
.i_uxUz(uxUz__26),
.i_uyUz(uyUz__26),
.i_uySintSinp(uySintSinp__26),
.i_oneMinusUz2(oneMinusUz2__26),
.i_uyUzSintCosp(uyUzSintCosp__26),
.i_uxUzSintCosp(uxUzSintCosp__26),
.i_uxSintSinp(uxSintSinp__26),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__26),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__26),
.i_uxCost(uxCost__26),
.i_uzCost(uzCost__26),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__26),
.i_uxNumerator(uxNumerator__26),
.i_uyNumerator(uyNumerator__26),
.i_uyCost(uyCost__26),
.i_uxQuotient(uxQuotient__26),
.i_uyQuotient(uyQuotient__26),
//Outputs			 
.o_sint(sint__27),
.o_cost(cost__27),
.o_sinp(sinp__27),
.o_cosp(cosp__27),
.o_sintCosp(sintCosp__27),
.o_sintSinp(sintSinp__27),
.o_uz2(uz2__27),
.o_uxUz(uxUz__27),
.o_uyUz(uyUz__27),
.o_uySintSinp(uySintSinp__27),
.o_oneMinusUz2(oneMinusUz2__27),
.o_uyUzSintCosp(uyUzSintCosp__27),
.o_uxUzSintCosp(uxUzSintCosp__27),
.o_uxSintSinp(uxSintSinp__27),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__27),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__27),
.o_uxCost(uxCost__27),
.o_uzCost(uzCost__27),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__27),
.o_uxNumerator(uxNumerator__27),
.o_uyNumerator(uyNumerator__27),
.o_uyCost(uyCost__27),
.o_uxQuotient(uxQuotient__27),
.o_uyQuotient(uyQuotient__27)
);  

InternalsBlock pipeReg26(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__25),
.i_cost(cost__25),
.i_sinp(sinp__25),
.i_cosp(cosp__25),
.i_sintCosp(sintCosp__25),
.i_sintSinp(sintSinp__25),
.i_uz2(uz2__25),
.i_uxUz(uxUz__25),
.i_uyUz(uyUz__25),
.i_uySintSinp(uySintSinp__25),
.i_oneMinusUz2(oneMinusUz2__25),
.i_uyUzSintCosp(uyUzSintCosp__25),
.i_uxUzSintCosp(uxUzSintCosp__25),
.i_uxSintSinp(uxSintSinp__25),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__25),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__25),
.i_uxCost(uxCost__25),
.i_uzCost(uzCost__25),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__25),
.i_uxNumerator(uxNumerator__25),
.i_uyNumerator(uyNumerator__25),
.i_uyCost(uyCost__25),
.i_uxQuotient(uxQuotient__25),
.i_uyQuotient(uyQuotient__25),
//Outputs			 
.o_sint(sint__26),
.o_cost(cost__26),
.o_sinp(sinp__26),
.o_cosp(cosp__26),
.o_sintCosp(sintCosp__26),
.o_sintSinp(sintSinp__26),
.o_uz2(uz2__26),
.o_uxUz(uxUz__26),
.o_uyUz(uyUz__26),
.o_uySintSinp(uySintSinp__26),
.o_oneMinusUz2(oneMinusUz2__26),
.o_uyUzSintCosp(uyUzSintCosp__26),
.o_uxUzSintCosp(uxUzSintCosp__26),
.o_uxSintSinp(uxSintSinp__26),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__26),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__26),
.o_uxCost(uxCost__26),
.o_uzCost(uzCost__26),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__26),
.o_uxNumerator(uxNumerator__26),
.o_uyNumerator(uyNumerator__26),
.o_uyCost(uyCost__26),
.o_uxQuotient(uxQuotient__26),
.o_uyQuotient(uyQuotient__26)
);  

InternalsBlock pipeReg25(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__24),
.i_cost(cost__24),
.i_sinp(sinp__24),
.i_cosp(cosp__24),
.i_sintCosp(sintCosp__24),
.i_sintSinp(sintSinp__24),
.i_uz2(uz2__24),
.i_uxUz(uxUz__24),
.i_uyUz(uyUz__24),
.i_uySintSinp(uySintSinp__24),
.i_oneMinusUz2(oneMinusUz2__24),
.i_uyUzSintCosp(uyUzSintCosp__24),
.i_uxUzSintCosp(uxUzSintCosp__24),
.i_uxSintSinp(uxSintSinp__24),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__24),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__24),
.i_uxCost(uxCost__24),
.i_uzCost(uzCost__24),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__24),
.i_uxNumerator(uxNumerator__24),
.i_uyNumerator(uyNumerator__24),
.i_uyCost(uyCost__24),
.i_uxQuotient(uxQuotient__24),
.i_uyQuotient(uyQuotient__24),
//Outputs			 
.o_sint(sint__25),
.o_cost(cost__25),
.o_sinp(sinp__25),
.o_cosp(cosp__25),
.o_sintCosp(sintCosp__25),
.o_sintSinp(sintSinp__25),
.o_uz2(uz2__25),
.o_uxUz(uxUz__25),
.o_uyUz(uyUz__25),
.o_uySintSinp(uySintSinp__25),
.o_oneMinusUz2(oneMinusUz2__25),
.o_uyUzSintCosp(uyUzSintCosp__25),
.o_uxUzSintCosp(uxUzSintCosp__25),
.o_uxSintSinp(uxSintSinp__25),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__25),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__25),
.o_uxCost(uxCost__25),
.o_uzCost(uzCost__25),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__25),
.o_uxNumerator(uxNumerator__25),
.o_uyNumerator(uyNumerator__25),
.o_uyCost(uyCost__25),
.o_uxQuotient(uxQuotient__25),
.o_uyQuotient(uyQuotient__25)
);  

InternalsBlock pipeReg24(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__23),
.i_cost(cost__23),
.i_sinp(sinp__23),
.i_cosp(cosp__23),
.i_sintCosp(sintCosp__23),
.i_sintSinp(sintSinp__23),
.i_uz2(uz2__23),
.i_uxUz(uxUz__23),
.i_uyUz(uyUz__23),
.i_uySintSinp(uySintSinp__23),
.i_oneMinusUz2(oneMinusUz2__23),
.i_uyUzSintCosp(uyUzSintCosp__23),
.i_uxUzSintCosp(uxUzSintCosp__23),
.i_uxSintSinp(uxSintSinp__23),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__23),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__23),
.i_uxCost(uxCost__23),
.i_uzCost(uzCost__23),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__23),
.i_uxNumerator(uxNumerator__23),
.i_uyNumerator(uyNumerator__23),
.i_uyCost(uyCost__23),
.i_uxQuotient(uxQuotient__23),
.i_uyQuotient(uyQuotient__23),
//Outputs			 
.o_sint(sint__24),
.o_cost(cost__24),
.o_sinp(sinp__24),
.o_cosp(cosp__24),
.o_sintCosp(sintCosp__24),
.o_sintSinp(sintSinp__24),
.o_uz2(uz2__24),
.o_uxUz(uxUz__24),
.o_uyUz(uyUz__24),
.o_uySintSinp(uySintSinp__24),
.o_oneMinusUz2(oneMinusUz2__24),
.o_uyUzSintCosp(uyUzSintCosp__24),
.o_uxUzSintCosp(uxUzSintCosp__24),
.o_uxSintSinp(uxSintSinp__24),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__24),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__24),
.o_uxCost(uxCost__24),
.o_uzCost(uzCost__24),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__24),
.o_uxNumerator(uxNumerator__24),
.o_uyNumerator(uyNumerator__24),
.o_uyCost(uyCost__24),
.o_uxQuotient(uxQuotient__24),
.o_uyQuotient(uyQuotient__24)
);  

InternalsBlock pipeReg23(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__22),
.i_cost(cost__22),
.i_sinp(sinp__22),
.i_cosp(cosp__22),
.i_sintCosp(sintCosp__22),
.i_sintSinp(sintSinp__22),
.i_uz2(uz2__22),
.i_uxUz(uxUz__22),
.i_uyUz(uyUz__22),
.i_uySintSinp(uySintSinp__22),
.i_oneMinusUz2(oneMinusUz2__22),
.i_uyUzSintCosp(uyUzSintCosp__22),
.i_uxUzSintCosp(uxUzSintCosp__22),
.i_uxSintSinp(uxSintSinp__22),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__22),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__22),
.i_uxCost(uxCost__22),
.i_uzCost(uzCost__22),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__22),
.i_uxNumerator(uxNumerator__22),
.i_uyNumerator(uyNumerator__22),
.i_uyCost(uyCost__22),
.i_uxQuotient(uxQuotient__22),
.i_uyQuotient(uyQuotient__22),
//Outputs			 
.o_sint(sint__23),
.o_cost(cost__23),
.o_sinp(sinp__23),
.o_cosp(cosp__23),
.o_sintCosp(sintCosp__23),
.o_sintSinp(sintSinp__23),
.o_uz2(uz2__23),
.o_uxUz(uxUz__23),
.o_uyUz(uyUz__23),
.o_uySintSinp(uySintSinp__23),
.o_oneMinusUz2(oneMinusUz2__23),
.o_uyUzSintCosp(uyUzSintCosp__23),
.o_uxUzSintCosp(uxUzSintCosp__23),
.o_uxSintSinp(uxSintSinp__23),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__23),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__23),
.o_uxCost(uxCost__23),
.o_uzCost(uzCost__23),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__23),
.o_uxNumerator(uxNumerator__23),
.o_uyNumerator(uyNumerator__23),
.o_uyCost(uyCost__23),
.o_uxQuotient(uxQuotient__23),
.o_uyQuotient(uyQuotient__23)
);  

InternalsBlock pipeReg22(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__21),
.i_cost(cost__21),
.i_sinp(sinp__21),
.i_cosp(cosp__21),
.i_sintCosp(sintCosp__21),
.i_sintSinp(sintSinp__21),
.i_uz2(uz2__21),
.i_uxUz(uxUz__21),
.i_uyUz(uyUz__21),
.i_uySintSinp(uySintSinp__21),
.i_oneMinusUz2(oneMinusUz2__21),
.i_uyUzSintCosp(uyUzSintCosp__21),
.i_uxUzSintCosp(uxUzSintCosp__21),
.i_uxSintSinp(uxSintSinp__21),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__21),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__21),
.i_uxCost(uxCost__21),
.i_uzCost(uzCost__21),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__21),
.i_uxNumerator(uxNumerator__21),
.i_uyNumerator(uyNumerator__21),
.i_uyCost(uyCost__21),
.i_uxQuotient(uxQuotient__21),
.i_uyQuotient(uyQuotient__21),
//Outputs			 
.o_sint(sint__22),
.o_cost(cost__22),
.o_sinp(sinp__22),
.o_cosp(cosp__22),
.o_sintCosp(sintCosp__22),
.o_sintSinp(sintSinp__22),
.o_uz2(uz2__22),
.o_uxUz(uxUz__22),
.o_uyUz(uyUz__22),
.o_uySintSinp(uySintSinp__22),
.o_oneMinusUz2(oneMinusUz2__22),
.o_uyUzSintCosp(uyUzSintCosp__22),
.o_uxUzSintCosp(uxUzSintCosp__22),
.o_uxSintSinp(uxSintSinp__22),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__22),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__22),
.o_uxCost(uxCost__22),
.o_uzCost(uzCost__22),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__22),
.o_uxNumerator(uxNumerator__22),
.o_uyNumerator(uyNumerator__22),
.o_uyCost(uyCost__22),
.o_uxQuotient(uxQuotient__22),
.o_uyQuotient(uyQuotient__22)
);  

InternalsBlock pipeReg21(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__20),
.i_cost(cost__20),
.i_sinp(sinp__20),
.i_cosp(cosp__20),
.i_sintCosp(sintCosp__20),
.i_sintSinp(sintSinp__20),
.i_uz2(uz2__20),
.i_uxUz(uxUz__20),
.i_uyUz(uyUz__20),
.i_uySintSinp(uySintSinp__20),
.i_oneMinusUz2(oneMinusUz2__20),
.i_uyUzSintCosp(uyUzSintCosp__20),
.i_uxUzSintCosp(uxUzSintCosp__20),
.i_uxSintSinp(uxSintSinp__20),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__20),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__20),
.i_uxCost(uxCost__20),
.i_uzCost(uzCost__20),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__20),
.i_uxNumerator(uxNumerator__20),
.i_uyNumerator(uyNumerator__20),
.i_uyCost(uyCost__20),
.i_uxQuotient(uxQuotient__20),
.i_uyQuotient(uyQuotient__20),
//Outputs			 
.o_sint(sint__21),
.o_cost(cost__21),
.o_sinp(sinp__21),
.o_cosp(cosp__21),
.o_sintCosp(sintCosp__21),
.o_sintSinp(sintSinp__21),
.o_uz2(uz2__21),
.o_uxUz(uxUz__21),
.o_uyUz(uyUz__21),
.o_uySintSinp(uySintSinp__21),
.o_oneMinusUz2(oneMinusUz2__21),
.o_uyUzSintCosp(uyUzSintCosp__21),
.o_uxUzSintCosp(uxUzSintCosp__21),
.o_uxSintSinp(uxSintSinp__21),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__21),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__21),
.o_uxCost(uxCost__21),
.o_uzCost(uzCost__21),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__21),
.o_uxNumerator(uxNumerator__21),
.o_uyNumerator(uyNumerator__21),
.o_uyCost(uyCost__21),
.o_uxQuotient(uxQuotient__21),
.o_uyQuotient(uyQuotient__21)
);  

InternalsBlock pipeReg20(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__19),
.i_cost(cost__19),
.i_sinp(sinp__19),
.i_cosp(cosp__19),
.i_sintCosp(sintCosp__19),
.i_sintSinp(sintSinp__19),
.i_uz2(uz2__19),
.i_uxUz(uxUz__19),
.i_uyUz(uyUz__19),
.i_uySintSinp(uySintSinp__19),
.i_oneMinusUz2(oneMinusUz2__19),
.i_uyUzSintCosp(uyUzSintCosp__19),
.i_uxUzSintCosp(uxUzSintCosp__19),
.i_uxSintSinp(uxSintSinp__19),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__19),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__19),
.i_uxCost(uxCost__19),
.i_uzCost(uzCost__19),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__19),
.i_uxNumerator(uxNumerator__19),
.i_uyNumerator(uyNumerator__19),
.i_uyCost(uyCost__19),
.i_uxQuotient(uxQuotient__19),
.i_uyQuotient(uyQuotient__19),
//Outputs			 
.o_sint(sint__20),
.o_cost(cost__20),
.o_sinp(sinp__20),
.o_cosp(cosp__20),
.o_sintCosp(sintCosp__20),
.o_sintSinp(sintSinp__20),
.o_uz2(uz2__20),
.o_uxUz(uxUz__20),
.o_uyUz(uyUz__20),
.o_uySintSinp(uySintSinp__20),
.o_oneMinusUz2(oneMinusUz2__20),
.o_uyUzSintCosp(uyUzSintCosp__20),
.o_uxUzSintCosp(uxUzSintCosp__20),
.o_uxSintSinp(uxSintSinp__20),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__20),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__20),
.o_uxCost(uxCost__20),
.o_uzCost(uzCost__20),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__20),
.o_uxNumerator(uxNumerator__20),
.o_uyNumerator(uyNumerator__20),
.o_uyCost(uyCost__20),
.o_uxQuotient(uxQuotient__20),
.o_uyQuotient(uyQuotient__20)
);  

InternalsBlock pipeReg19(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__18),
.i_cost(cost__18),
.i_sinp(sinp__18),
.i_cosp(cosp__18),
.i_sintCosp(sintCosp__18),
.i_sintSinp(sintSinp__18),
.i_uz2(uz2__18),
.i_uxUz(uxUz__18),
.i_uyUz(uyUz__18),
.i_uySintSinp(uySintSinp__18),
.i_oneMinusUz2(oneMinusUz2__18),
.i_uyUzSintCosp(uyUzSintCosp__18),
.i_uxUzSintCosp(uxUzSintCosp__18),
.i_uxSintSinp(uxSintSinp__18),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__18),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__18),
.i_uxCost(uxCost__18),
.i_uzCost(uzCost__18),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__18),
.i_uxNumerator(uxNumerator__18),
.i_uyNumerator(uyNumerator__18),
.i_uyCost(uyCost__18),
.i_uxQuotient(uxQuotient__18),
.i_uyQuotient(uyQuotient__18),
//Outputs			 
.o_sint(sint__19),
.o_cost(cost__19),
.o_sinp(sinp__19),
.o_cosp(cosp__19),
.o_sintCosp(sintCosp__19),
.o_sintSinp(sintSinp__19),
.o_uz2(uz2__19),
.o_uxUz(uxUz__19),
.o_uyUz(uyUz__19),
.o_uySintSinp(uySintSinp__19),
.o_oneMinusUz2(oneMinusUz2__19),
.o_uyUzSintCosp(uyUzSintCosp__19),
.o_uxUzSintCosp(uxUzSintCosp__19),
.o_uxSintSinp(uxSintSinp__19),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__19),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__19),
.o_uxCost(uxCost__19),
.o_uzCost(uzCost__19),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__19),
.o_uxNumerator(uxNumerator__19),
.o_uyNumerator(uyNumerator__19),
.o_uyCost(uyCost__19),
.o_uxQuotient(uxQuotient__19),
.o_uyQuotient(uyQuotient__19)
);  

InternalsBlock pipeReg18(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__17),
.i_cost(cost__17),
.i_sinp(sinp__17),
.i_cosp(cosp__17),
.i_sintCosp(sintCosp__17),
.i_sintSinp(sintSinp__17),
.i_uz2(uz2__17),
.i_uxUz(uxUz__17),
.i_uyUz(uyUz__17),
.i_uySintSinp(uySintSinp__17),
.i_oneMinusUz2(oneMinusUz2__17),
.i_uyUzSintCosp(uyUzSintCosp__17),
.i_uxUzSintCosp(uxUzSintCosp__17),
.i_uxSintSinp(uxSintSinp__17),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__17),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__17),
.i_uxCost(uxCost__17),
.i_uzCost(uzCost__17),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__17),
.i_uxNumerator(uxNumerator__17),
.i_uyNumerator(uyNumerator__17),
.i_uyCost(uyCost__17),
.i_uxQuotient(uxQuotient__17),
.i_uyQuotient(uyQuotient__17),
//Outputs			 
.o_sint(sint__18),
.o_cost(cost__18),
.o_sinp(sinp__18),
.o_cosp(cosp__18),
.o_sintCosp(sintCosp__18),
.o_sintSinp(sintSinp__18),
.o_uz2(uz2__18),
.o_uxUz(uxUz__18),
.o_uyUz(uyUz__18),
.o_uySintSinp(uySintSinp__18),
.o_oneMinusUz2(oneMinusUz2__18),
.o_uyUzSintCosp(uyUzSintCosp__18),
.o_uxUzSintCosp(uxUzSintCosp__18),
.o_uxSintSinp(uxSintSinp__18),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__18),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__18),
.o_uxCost(uxCost__18),
.o_uzCost(uzCost__18),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__18),
.o_uxNumerator(uxNumerator__18),
.o_uyNumerator(uyNumerator__18),
.o_uyCost(uyCost__18),
.o_uxQuotient(uxQuotient__18),
.o_uyQuotient(uyQuotient__18)
);  

InternalsBlock pipeReg17(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__16),
.i_cost(cost__16),
.i_sinp(sinp__16),
.i_cosp(cosp__16),
.i_sintCosp(sintCosp__16),
.i_sintSinp(sintSinp__16),
.i_uz2(uz2__16),
.i_uxUz(uxUz__16),
.i_uyUz(uyUz__16),
.i_uySintSinp(uySintSinp__16),
.i_oneMinusUz2(oneMinusUz2__16),
.i_uyUzSintCosp(uyUzSintCosp__16),
.i_uxUzSintCosp(uxUzSintCosp__16),
.i_uxSintSinp(uxSintSinp__16),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__16),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__16),
.i_uxCost(uxCost__16),
.i_uzCost(uzCost__16),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__16),
.i_uxNumerator(uxNumerator__16),
.i_uyNumerator(uyNumerator__16),
.i_uyCost(uyCost__16),
.i_uxQuotient(uxQuotient__16),
.i_uyQuotient(uyQuotient__16),
//Outputs			 
.o_sint(sint__17),
.o_cost(cost__17),
.o_sinp(sinp__17),
.o_cosp(cosp__17),
.o_sintCosp(sintCosp__17),
.o_sintSinp(sintSinp__17),
.o_uz2(uz2__17),
.o_uxUz(uxUz__17),
.o_uyUz(uyUz__17),
.o_uySintSinp(uySintSinp__17),
.o_oneMinusUz2(oneMinusUz2__17),
.o_uyUzSintCosp(uyUzSintCosp__17),
.o_uxUzSintCosp(uxUzSintCosp__17),
.o_uxSintSinp(uxSintSinp__17),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__17),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__17),
.o_uxCost(uxCost__17),
.o_uzCost(uzCost__17),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__17),
.o_uxNumerator(uxNumerator__17),
.o_uyNumerator(uyNumerator__17),
.o_uyCost(uyCost__17),
.o_uxQuotient(uxQuotient__17),
.o_uyQuotient(uyQuotient__17)
);  


InternalsBlock pipeReg15(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__14),
.i_cost(cost__14),
.i_sinp(sinp__14),
.i_cosp(cosp__14),
.i_sintCosp(sintCosp__14),
.i_sintSinp(sintSinp__14),
.i_uz2(uz2__14),
.i_uxUz(uxUz__14),
.i_uyUz(uyUz__14),
.i_uySintSinp(uySintSinp__14),
.i_oneMinusUz2(oneMinusUz2__14),
.i_uyUzSintCosp(uyUzSintCosp__14),
.i_uxUzSintCosp(uxUzSintCosp__14),
.i_uxSintSinp(uxSintSinp__14),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__14),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__14),
.i_uxCost(uxCost__14),
.i_uzCost(uzCost__14),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__14),
.i_uxNumerator(uxNumerator__14),
.i_uyNumerator(uyNumerator__14),
.i_uyCost(uyCost__14),
.i_uxQuotient(uxQuotient__14),
.i_uyQuotient(uyQuotient__14),
//Outputs			 
.o_sint(sint__15),
.o_cost(cost__15),
.o_sinp(sinp__15),
.o_cosp(cosp__15),
.o_sintCosp(sintCosp__15),
.o_sintSinp(sintSinp__15),
.o_uz2(uz2__15),
.o_uxUz(uxUz__15),
.o_uyUz(uyUz__15),
.o_uySintSinp(uySintSinp__15),
.o_oneMinusUz2(oneMinusUz2__15),
.o_uyUzSintCosp(uyUzSintCosp__15),
.o_uxUzSintCosp(uxUzSintCosp__15),
.o_uxSintSinp(uxSintSinp__15),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__15),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__15),
.o_uxCost(uxCost__15),
.o_uzCost(uzCost__15),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__15),
.o_uxNumerator(uxNumerator__15),
.o_uyNumerator(uyNumerator__15),
.o_uyCost(uyCost__15),
.o_uxQuotient(uxQuotient__15),
.o_uyQuotient(uyQuotient__15)
);  

InternalsBlock pipeReg14(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__13),
.i_cost(cost__13),
.i_sinp(sinp__13),
.i_cosp(cosp__13),
.i_sintCosp(sintCosp__13),
.i_sintSinp(sintSinp__13),
.i_uz2(uz2__13),
.i_uxUz(uxUz__13),
.i_uyUz(uyUz__13),
.i_uySintSinp(uySintSinp__13),
.i_oneMinusUz2(oneMinusUz2__13),
.i_uyUzSintCosp(uyUzSintCosp__13),
.i_uxUzSintCosp(uxUzSintCosp__13),
.i_uxSintSinp(uxSintSinp__13),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__13),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__13),
.i_uxCost(uxCost__13),
.i_uzCost(uzCost__13),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__13),
.i_uxNumerator(uxNumerator__13),
.i_uyNumerator(uyNumerator__13),
.i_uyCost(uyCost__13),
.i_uxQuotient(uxQuotient__13),
.i_uyQuotient(uyQuotient__13),
//Outputs			 
.o_sint(sint__14),
.o_cost(cost__14),
.o_sinp(sinp__14),
.o_cosp(cosp__14),
.o_sintCosp(sintCosp__14),
.o_sintSinp(sintSinp__14),
.o_uz2(uz2__14),
.o_uxUz(uxUz__14),
.o_uyUz(uyUz__14),
.o_uySintSinp(uySintSinp__14),
.o_oneMinusUz2(oneMinusUz2__14),
.o_uyUzSintCosp(uyUzSintCosp__14),
.o_uxUzSintCosp(uxUzSintCosp__14),
.o_uxSintSinp(uxSintSinp__14),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__14),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__14),
.o_uxCost(uxCost__14),
.o_uzCost(uzCost__14),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__14),
.o_uxNumerator(uxNumerator__14),
.o_uyNumerator(uyNumerator__14),
.o_uyCost(uyCost__14),
.o_uxQuotient(uxQuotient__14),
.o_uyQuotient(uyQuotient__14)
);  

InternalsBlock pipeReg13(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__12),
.i_cost(cost__12),
.i_sinp(sinp__12),
.i_cosp(cosp__12),
.i_sintCosp(sintCosp__12),
.i_sintSinp(sintSinp__12),
.i_uz2(uz2__12),
.i_uxUz(uxUz__12),
.i_uyUz(uyUz__12),
.i_uySintSinp(uySintSinp__12),
.i_oneMinusUz2(oneMinusUz2__12),
.i_uyUzSintCosp(uyUzSintCosp__12),
.i_uxUzSintCosp(uxUzSintCosp__12),
.i_uxSintSinp(uxSintSinp__12),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__12),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__12),
.i_uxCost(uxCost__12),
.i_uzCost(uzCost__12),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__12),
.i_uxNumerator(uxNumerator__12),
.i_uyNumerator(uyNumerator__12),
.i_uyCost(uyCost__12),
.i_uxQuotient(uxQuotient__12),
.i_uyQuotient(uyQuotient__12),
//Outputs			 
.o_sint(sint__13),
.o_cost(cost__13),
.o_sinp(sinp__13),
.o_cosp(cosp__13),
.o_sintCosp(sintCosp__13),
.o_sintSinp(sintSinp__13),
.o_uz2(uz2__13),
.o_uxUz(uxUz__13),
.o_uyUz(uyUz__13),
.o_uySintSinp(uySintSinp__13),
.o_oneMinusUz2(oneMinusUz2__13),
.o_uyUzSintCosp(uyUzSintCosp__13),
.o_uxUzSintCosp(uxUzSintCosp__13),
.o_uxSintSinp(uxSintSinp__13),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__13),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__13),
.o_uxCost(uxCost__13),
.o_uzCost(uzCost__13),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__13),
.o_uxNumerator(uxNumerator__13),
.o_uyNumerator(uyNumerator__13),
.o_uyCost(uyCost__13),
.o_uxQuotient(uxQuotient__13),
.o_uyQuotient(uyQuotient__13)
);  

InternalsBlock pipeReg12(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__11),
.i_cost(cost__11),
.i_sinp(sinp__11),
.i_cosp(cosp__11),
.i_sintCosp(sintCosp__11),
.i_sintSinp(sintSinp__11),
.i_uz2(uz2__11),
.i_uxUz(uxUz__11),
.i_uyUz(uyUz__11),
.i_uySintSinp(uySintSinp__11),
.i_oneMinusUz2(oneMinusUz2__11),
.i_uyUzSintCosp(uyUzSintCosp__11),
.i_uxUzSintCosp(uxUzSintCosp__11),
.i_uxSintSinp(uxSintSinp__11),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__11),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__11),
.i_uxCost(uxCost__11),
.i_uzCost(uzCost__11),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__11),
.i_uxNumerator(uxNumerator__11),
.i_uyNumerator(uyNumerator__11),
.i_uyCost(uyCost__11),
.i_uxQuotient(uxQuotient__11),
.i_uyQuotient(uyQuotient__11),
//Outputs			 
.o_sint(sint__12),
.o_cost(cost__12),
.o_sinp(sinp__12),
.o_cosp(cosp__12),
.o_sintCosp(sintCosp__12),
.o_sintSinp(sintSinp__12),
.o_uz2(uz2__12),
.o_uxUz(uxUz__12),
.o_uyUz(uyUz__12),
.o_uySintSinp(uySintSinp__12),
.o_oneMinusUz2(oneMinusUz2__12),
.o_uyUzSintCosp(uyUzSintCosp__12),
.o_uxUzSintCosp(uxUzSintCosp__12),
.o_uxSintSinp(uxSintSinp__12),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__12),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__12),
.o_uxCost(uxCost__12),
.o_uzCost(uzCost__12),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__12),
.o_uxNumerator(uxNumerator__12),
.o_uyNumerator(uyNumerator__12),
.o_uyCost(uyCost__12),
.o_uxQuotient(uxQuotient__12),
.o_uyQuotient(uyQuotient__12)
);  

InternalsBlock pipeReg11(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__10),
.i_cost(cost__10),
.i_sinp(sinp__10),
.i_cosp(cosp__10),
.i_sintCosp(sintCosp__10),
.i_sintSinp(sintSinp__10),
.i_uz2(uz2__10),
.i_uxUz(uxUz__10),
.i_uyUz(uyUz__10),
.i_uySintSinp(uySintSinp__10),
.i_oneMinusUz2(oneMinusUz2__10),
.i_uyUzSintCosp(uyUzSintCosp__10),
.i_uxUzSintCosp(uxUzSintCosp__10),
.i_uxSintSinp(uxSintSinp__10),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__10),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__10),
.i_uxCost(uxCost__10),
.i_uzCost(uzCost__10),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__10),
.i_uxNumerator(uxNumerator__10),
.i_uyNumerator(uyNumerator__10),
.i_uyCost(uyCost__10),
.i_uxQuotient(uxQuotient__10),
.i_uyQuotient(uyQuotient__10),
//Outputs			 
.o_sint(sint__11),
.o_cost(cost__11),
.o_sinp(sinp__11),
.o_cosp(cosp__11),
.o_sintCosp(sintCosp__11),
.o_sintSinp(sintSinp__11),
.o_uz2(uz2__11),
.o_uxUz(uxUz__11),
.o_uyUz(uyUz__11),
.o_uySintSinp(uySintSinp__11),
.o_oneMinusUz2(oneMinusUz2__11),
.o_uyUzSintCosp(uyUzSintCosp__11),
.o_uxUzSintCosp(uxUzSintCosp__11),
.o_uxSintSinp(uxSintSinp__11),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__11),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__11),
.o_uxCost(uxCost__11),
.o_uzCost(uzCost__11),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__11),
.o_uxNumerator(uxNumerator__11),
.o_uyNumerator(uyNumerator__11),
.o_uyCost(uyCost__11),
.o_uxQuotient(uxQuotient__11),
.o_uyQuotient(uyQuotient__11)
);  

InternalsBlock pipeReg10(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__9),
.i_cost(cost__9),
.i_sinp(sinp__9),
.i_cosp(cosp__9),
.i_sintCosp(sintCosp__9),
.i_sintSinp(sintSinp__9),
.i_uz2(uz2__9),
.i_uxUz(uxUz__9),
.i_uyUz(uyUz__9),
.i_uySintSinp(uySintSinp__9),
.i_oneMinusUz2(oneMinusUz2__9),
.i_uyUzSintCosp(uyUzSintCosp__9),
.i_uxUzSintCosp(uxUzSintCosp__9),
.i_uxSintSinp(uxSintSinp__9),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__9),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__9),
.i_uxCost(uxCost__9),
.i_uzCost(uzCost__9),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__9),
.i_uxNumerator(uxNumerator__9),
.i_uyNumerator(uyNumerator__9),
.i_uyCost(uyCost__9),
.i_uxQuotient(uxQuotient__9),
.i_uyQuotient(uyQuotient__9),
//Outputs			 
.o_sint(sint__10),
.o_cost(cost__10),
.o_sinp(sinp__10),
.o_cosp(cosp__10),
.o_sintCosp(sintCosp__10),
.o_sintSinp(sintSinp__10),
.o_uz2(uz2__10),
.o_uxUz(uxUz__10),
.o_uyUz(uyUz__10),
.o_uySintSinp(uySintSinp__10),
.o_oneMinusUz2(oneMinusUz2__10),
.o_uyUzSintCosp(uyUzSintCosp__10),
.o_uxUzSintCosp(uxUzSintCosp__10),
.o_uxSintSinp(uxSintSinp__10),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__10),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__10),
.o_uxCost(uxCost__10),
.o_uzCost(uzCost__10),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__10),
.o_uxNumerator(uxNumerator__10),
.o_uyNumerator(uyNumerator__10),
.o_uyCost(uyCost__10),
.o_uxQuotient(uxQuotient__10),
.o_uyQuotient(uyQuotient__10)
);  

InternalsBlock pipeReg9(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__8),
.i_cost(cost__8),
.i_sinp(sinp__8),
.i_cosp(cosp__8),
.i_sintCosp(sintCosp__8),
.i_sintSinp(sintSinp__8),
.i_uz2(uz2__8),
.i_uxUz(uxUz__8),
.i_uyUz(uyUz__8),
.i_uySintSinp(uySintSinp__8),
.i_oneMinusUz2(oneMinusUz2__8),
.i_uyUzSintCosp(uyUzSintCosp__8),
.i_uxUzSintCosp(uxUzSintCosp__8),
.i_uxSintSinp(uxSintSinp__8),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__8),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__8),
.i_uxCost(uxCost__8),
.i_uzCost(uzCost__8),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__8),
.i_uxNumerator(uxNumerator__8),
.i_uyNumerator(uyNumerator__8),
.i_uyCost(uyCost__8),
.i_uxQuotient(uxQuotient__8),
.i_uyQuotient(uyQuotient__8),
//Outputs			 
.o_sint(sint__9),
.o_cost(cost__9),
.o_sinp(sinp__9),
.o_cosp(cosp__9),
.o_sintCosp(sintCosp__9),
.o_sintSinp(sintSinp__9),
.o_uz2(uz2__9),
.o_uxUz(uxUz__9),
.o_uyUz(uyUz__9),
.o_uySintSinp(uySintSinp__9),
.o_oneMinusUz2(oneMinusUz2__9),
.o_uyUzSintCosp(uyUzSintCosp__9),
.o_uxUzSintCosp(uxUzSintCosp__9),
.o_uxSintSinp(uxSintSinp__9),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__9),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__9),
.o_uxCost(uxCost__9),
.o_uzCost(uzCost__9),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__9),
.o_uxNumerator(uxNumerator__9),
.o_uyNumerator(uyNumerator__9),
.o_uyCost(uyCost__9),
.o_uxQuotient(uxQuotient__9),
.o_uyQuotient(uyQuotient__9)
);  

InternalsBlock pipeReg8(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__7),
.i_cost(cost__7),
.i_sinp(sinp__7),
.i_cosp(cosp__7),
.i_sintCosp(sintCosp__7),
.i_sintSinp(sintSinp__7),
.i_uz2(uz2__7),
.i_uxUz(uxUz__7),
.i_uyUz(uyUz__7),
.i_uySintSinp(uySintSinp__7),
.i_oneMinusUz2(oneMinusUz2__7),
.i_uyUzSintCosp(uyUzSintCosp__7),
.i_uxUzSintCosp(uxUzSintCosp__7),
.i_uxSintSinp(uxSintSinp__7),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__7),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__7),
.i_uxCost(uxCost__7),
.i_uzCost(uzCost__7),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__7),
.i_uxNumerator(uxNumerator__7),
.i_uyNumerator(uyNumerator__7),
.i_uyCost(uyCost__7),
.i_uxQuotient(uxQuotient__7),
.i_uyQuotient(uyQuotient__7),
//Outputs			 
.o_sint(sint__8),
.o_cost(cost__8),
.o_sinp(sinp__8),
.o_cosp(cosp__8),
.o_sintCosp(sintCosp__8),
.o_sintSinp(sintSinp__8),
.o_uz2(uz2__8),
.o_uxUz(uxUz__8),
.o_uyUz(uyUz__8),
.o_uySintSinp(uySintSinp__8),
.o_oneMinusUz2(oneMinusUz2__8),
.o_uyUzSintCosp(uyUzSintCosp__8),
.o_uxUzSintCosp(uxUzSintCosp__8),
.o_uxSintSinp(uxSintSinp__8),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__8),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__8),
.o_uxCost(uxCost__8),
.o_uzCost(uzCost__8),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__8),
.o_uxNumerator(uxNumerator__8),
.o_uyNumerator(uyNumerator__8),
.o_uyCost(uyCost__8),
.o_uxQuotient(uxQuotient__8),
.o_uyQuotient(uyQuotient__8)
);  

InternalsBlock pipeReg7(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__6),
.i_cost(cost__6),
.i_sinp(sinp__6),
.i_cosp(cosp__6),
.i_sintCosp(sintCosp__6),
.i_sintSinp(sintSinp__6),
.i_uz2(uz2__6),
.i_uxUz(uxUz__6),
.i_uyUz(uyUz__6),
.i_uySintSinp(uySintSinp__6),
.i_oneMinusUz2(oneMinusUz2__6),
.i_uyUzSintCosp(uyUzSintCosp__6),
.i_uxUzSintCosp(uxUzSintCosp__6),
.i_uxSintSinp(uxSintSinp__6),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__6),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__6),
.i_uxCost(uxCost__6),
.i_uzCost(uzCost__6),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__6),
.i_uxNumerator(uxNumerator__6),
.i_uyNumerator(uyNumerator__6),
.i_uyCost(uyCost__6),
.i_uxQuotient(uxQuotient__6),
.i_uyQuotient(uyQuotient__6),
//Outputs			 
.o_sint(sint__7),
.o_cost(cost__7),
.o_sinp(sinp__7),
.o_cosp(cosp__7),
.o_sintCosp(sintCosp__7),
.o_sintSinp(sintSinp__7),
.o_uz2(uz2__7),
.o_uxUz(uxUz__7),
.o_uyUz(uyUz__7),
.o_uySintSinp(uySintSinp__7),
.o_oneMinusUz2(oneMinusUz2__7),
.o_uyUzSintCosp(uyUzSintCosp__7),
.o_uxUzSintCosp(uxUzSintCosp__7),
.o_uxSintSinp(uxSintSinp__7),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__7),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__7),
.o_uxCost(uxCost__7),
.o_uzCost(uzCost__7),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__7),
.o_uxNumerator(uxNumerator__7),
.o_uyNumerator(uyNumerator__7),
.o_uyCost(uyCost__7),
.o_uxQuotient(uxQuotient__7),
.o_uyQuotient(uyQuotient__7)
);  

InternalsBlock pipeReg6(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__5),
.i_cost(cost__5),
.i_sinp(sinp__5),
.i_cosp(cosp__5),
.i_sintCosp(sintCosp__5),
.i_sintSinp(sintSinp__5),
.i_uz2(uz2__5),
.i_uxUz(uxUz__5),
.i_uyUz(uyUz__5),
.i_uySintSinp(uySintSinp__5),
.i_oneMinusUz2(oneMinusUz2__5),
.i_uyUzSintCosp(uyUzSintCosp__5),
.i_uxUzSintCosp(uxUzSintCosp__5),
.i_uxSintSinp(uxSintSinp__5),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__5),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__5),
.i_uxCost(uxCost__5),
.i_uzCost(uzCost__5),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__5),
.i_uxNumerator(uxNumerator__5),
.i_uyNumerator(uyNumerator__5),
.i_uyCost(uyCost__5),
.i_uxQuotient(uxQuotient__5),
.i_uyQuotient(uyQuotient__5),
//Outputs			 
.o_sint(sint__6),
.o_cost(cost__6),
.o_sinp(sinp__6),
.o_cosp(cosp__6),
.o_sintCosp(sintCosp__6),
.o_sintSinp(sintSinp__6),
.o_uz2(uz2__6),
.o_uxUz(uxUz__6),
.o_uyUz(uyUz__6),
.o_uySintSinp(uySintSinp__6),
.o_oneMinusUz2(oneMinusUz2__6),
.o_uyUzSintCosp(uyUzSintCosp__6),
.o_uxUzSintCosp(uxUzSintCosp__6),
.o_uxSintSinp(uxSintSinp__6),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__6),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__6),
.o_uxCost(uxCost__6),
.o_uzCost(uzCost__6),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__6),
.o_uxNumerator(uxNumerator__6),
.o_uyNumerator(uyNumerator__6),
.o_uyCost(uyCost__6),
.o_uxQuotient(uxQuotient__6),
.o_uyQuotient(uyQuotient__6)
);  

InternalsBlock pipeReg5(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__4),
.i_cost(cost__4),
.i_sinp(sinp__4),
.i_cosp(cosp__4),
.i_sintCosp(sintCosp__4),
.i_sintSinp(sintSinp__4),
.i_uz2(uz2__4),
.i_uxUz(uxUz__4),
.i_uyUz(uyUz__4),
.i_uySintSinp(uySintSinp__4),
.i_oneMinusUz2(oneMinusUz2__4),
.i_uyUzSintCosp(uyUzSintCosp__4),
.i_uxUzSintCosp(uxUzSintCosp__4),
.i_uxSintSinp(uxSintSinp__4),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__4),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__4),
.i_uxCost(uxCost__4),
.i_uzCost(uzCost__4),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__4),
.i_uxNumerator(uxNumerator__4),
.i_uyNumerator(uyNumerator__4),
.i_uyCost(uyCost__4),
.i_uxQuotient(uxQuotient__4),
.i_uyQuotient(uyQuotient__4),
//Outputs			 
.o_sint(sint__5),
.o_cost(cost__5),
.o_sinp(sinp__5),
.o_cosp(cosp__5),
.o_sintCosp(sintCosp__5),
.o_sintSinp(sintSinp__5),
.o_uz2(uz2__5),
.o_uxUz(uxUz__5),
.o_uyUz(uyUz__5),
.o_uySintSinp(uySintSinp__5),
.o_oneMinusUz2(oneMinusUz2__5),
.o_uyUzSintCosp(uyUzSintCosp__5),
.o_uxUzSintCosp(uxUzSintCosp__5),
.o_uxSintSinp(uxSintSinp__5),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__5),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__5),
.o_uxCost(uxCost__5),
.o_uzCost(uzCost__5),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__5),
.o_uxNumerator(uxNumerator__5),
.o_uyNumerator(uyNumerator__5),
.o_uyCost(uyCost__5),
.o_uxQuotient(uxQuotient__5),
.o_uyQuotient(uyQuotient__5)
);  


//since these will be replaced later


 assign 	sint__0						= 32'b00000000000000000000000000000000;
 assign		cost__0						= 32'b00000000000000000000000000000000;
 assign		sinp__0						= 32'b00000000000000000000000000000000;
 assign		cosp__0						= 32'b00000000000000000000000000000000;
 assign		sintCosp__0					= 32'b00000000000000000000000000000000;
 assign		sintSinp__0					= 32'b00000000000000000000000000000000;
 assign		uz2__0						= 0;
 assign		uxUz__0						= 32'b00000000000000000000000000000000;
 assign		uyUz__0						= 32'b00000000000000000000000000000000;
 assign		uySintSinp__0				= 32'b00000000000000000000000000000000;
 assign		oneMinusUz2__0				= 0;
 assign		uyUzSintCosp__0				= 32'b00000000000000000000000000000000;
 assign		uxUzSintCosp__0				= 32'b00000000000000000000000000000000;
 assign		uxSintSinp__0				= 32'b00000000000000000000000000000000;
 assign		sqrtOneMinusUz2__0			= 32'b00000000000000000000000000000000;
 assign		sintCospSqrtOneMinusUz2__0	= 32'b00000000000000000000000000000000;
 assign		uxCost__0					= 32'b00000000000000000000000000000000;
 assign		uzCost__0					= 32'b00000000000000000000000000000000;
 assign		sqrtOneMinusUz2_inv__0		= 32'b00000000000000000000000000000000;
 assign		uxNumerator__0				= 32'b00000000000000000000000000000000;
 assign		uyNumerator__0				= 32'b00000000000000000000000000000000;
 assign		uyCost__0					= 32'b00000000000000000000000000000000;
 assign		uxQuotient__0				= 32'b00000000000000000000000000000000;
 assign		uyQuotient__0				= 32'b00000000000000000000000000000000;

InternalsBlock pipeReg1(
//Inputs
.clock(clock),
.reset(reset),
.enable(enable),
.i_sint(sint__0),
.i_cost(cost__0),
.i_sinp(sinp__0),
.i_cosp(cosp__0),
.i_sintCosp(sintCosp__0),
.i_sintSinp(sintSinp__0),
.i_uz2(uz2__0),
.i_uxUz(uxUz__0),
.i_uyUz(uyUz__0),
.i_uySintSinp(uySintSinp__0),
.i_oneMinusUz2(oneMinusUz2__0),
.i_uyUzSintCosp(uyUzSintCosp__0),
.i_uxUzSintCosp(uxUzSintCosp__0),
.i_uxSintSinp(uxSintSinp__0),
.i_sqrtOneMinusUz2(sqrtOneMinusUz2__0),
.i_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__0),
.i_uxCost(uxCost__0),
.i_uzCost(uzCost__0),
.i_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__0),
.i_uxNumerator(uxNumerator__0),
.i_uyNumerator(uyNumerator__0),
.i_uyCost(uyCost__0),
.i_uxQuotient(uxQuotient__0),
.i_uyQuotient(uyQuotient__0),
//Outputs			 
.o_sint(sint__1),
.o_cost(cost__1),
.o_sinp(sinp__1),
.o_cosp(cosp__1),
.o_sintCosp(sintCosp__1),
.o_sintSinp(sintSinp__1),
.o_uz2(uz2__1),
.o_uxUz(uxUz__1),
.o_uyUz(uyUz__1),
.o_uySintSinp(uySintSinp__1),
.o_oneMinusUz2(oneMinusUz2__1),
.o_uyUzSintCosp(uyUzSintCosp__1),
.o_uxUzSintCosp(uxUzSintCosp__1),
.o_uxSintSinp(uxSintSinp__1),
.o_sqrtOneMinusUz2(sqrtOneMinusUz2__1),
.o_sintCospSqrtOneMinusUz2(sintCospSqrtOneMinusUz2__1),
.o_uxCost(uxCost__1),
.o_uzCost(uzCost__1),
.o_sqrtOneMinusUz2_inv(sqrtOneMinusUz2_inv__1),
.o_uxNumerator(uxNumerator__1),
.o_uyNumerator(uyNumerator__1),
.o_uyCost(uyCost__1),
.o_uxQuotient(uxQuotient__1),
.o_uyQuotient(uyQuotient__1)
);

//-------------SYNCHRONOUS LOGIC----------------------
//
//
//
//
//
//
//
//
//
//
//
//
//This is the end of the generate statement, and the beginning of the
//synchronous logic.  On the clock event, the outputs calculated from
//this block are put on the output pins for reading (registered
//outputs, as per the convention).




//Assign outputs from block on positive clock edge.
always @ (posedge clock) begin
	if(reset_new) begin
		//Reset internal non-pipelined registers here.
		ux_scatterer	<= 32'h00000000;
		uy_scatterer	<= 32'h00000000;
		uz_scatterer	<= 32'h7FFFFFFF;
	end else if (enable) begin
		ux_scatterer <= new_ux;
		uy_scatterer <= new_uy;
		uz_scatterer <= new_uz;
	end
end

//-------------ASYNCHRONOUS LOGIC----------------------
//
//
//
//
//
//
//
//
//
//
//
//
//This is where the asynchronous logic takes place.  Things that
//occur here include setting up wiring to send to the multipliers,
//divide unit, and square root unit.  Also, products brought in
//from the wrapper are placed on the appropriate wires for placement
//in the pipeline.




//-------------OPERAND SETUP----------------------


//NAMING CONVENTION:
//opX_Y_Z, op stands for operand, X stands for the multiplication number for
//that clock cycle, Y stands for the clock cycle, Z is either 1 or 2 for the
//first or second operand for this multiply
//
//COMMENTING CONVENTIONS:
//CC X means that the values being calculated will be ready for the Xth register
//location, where 0 is the register prior to any calculations being done, 1 is
//after the 1st clock cycle of calculation, etc.

//CC 2
assign	op1_2_1						=	i_uz1;
assign	op1_2_2						=	i_uz1;
	
//CC 3
//SUBTRACTION, see math results
	
//CC 4
assign	op1_4_1						=	i_ux3;
assign	op1_4_2						=	i_uz3;

//CC 5 -- NOOP, line up with reflector
	
//CC `SQRT+5 -- Started in CC 6
assign	sqrtOperand1_6				=	oneMinusUz2__5;

//CC `SQRT+`DIV+6 -- Started in CC `SQRT+5
assign	divNumerator1_16			=	`INTMAX_2;
//assign	divDenominator1_16			=	sqrtOneMinusUz2[`SQRT+5];
assign	divDenominator1_16			=	new_sqrtOneMinusUz2;

//CC `SQRT+`DIV+3
assign op1_33_1						=	sint_Mem;
assign op1_33_2						=	cosp_Mem;

assign op2_33_1						=	sint_Mem;
assign op2_33_2						=	sinp_Mem;

assign op3_33_1						=	i_uy32;
assign op3_33_2						=	i_uz32;

//CC `SQRT+`DIV+4
assign op1_34_1						=	i_ux33;
assign op1_34_2						=	sintSinp__33;

assign op2_34_1						=	i_uy33;
assign op2_34_2						=	sintSinp__33;

assign op3_34_1						=	uxUz__33;
assign op3_34_2						=	sintCosp__33;

assign op4_34_1						=	uyUz__33;
assign op4_34_2						=	sintCosp__33;

//CC `SQRT+`DIV+5
//2 SUBS (see math results)
//`DIVISION COMPLETE (see math results)

//CC `SQRT+`DIV+6 -- Division is now complete and can be read.
assign op1_36_1						=	uxNumerator__35;
assign op1_36_2						=	new_sqrtOneMinusUz2_inv;


assign op2_36_1						=	uyNumerator__35;
assign op2_36_2						=	new_sqrtOneMinusUz2_inv;

assign op3_36_1						=	sintCosp__35;
assign op3_36_2						=	sqrtOneMinusUz2__35;

assign op4_36_1						=	i_ux35;
assign op4_36_2						=	cost__35;

assign op5_36_1						=	i_uy35;
assign op5_36_2						=	cost__35;

assign op6_36_1						=	i_uz35;
assign op6_36_2						=	cost__35;


//-------------MATH RESULTS----------------------


//NAMING CONVENTION:
//new_VAR means that the variable named VAR will be stored into the register
//pipeline at the clock cycle indicated by the comments above it.
//
//prod stands for product, quot stands for quotient, sqrt stands for square root
//prodX_Y means the Xth product which started calculation at the Yth clock cycle
//Similarly for quot and sqrtResult.
//
//
//COMMENTING CONVENTIONS:
//CC X means that the values being calculated will be ready for the Xth register
//location, where 0 is the register prior to any calculations being done, 1 is
//after the 1st clock cycle of calculation, etc.


//Used to determine whether or not the divide operation overflowed.
//or U1(div_overflow, quot1_16[62], quot1_16[61], quot1_16[60], quot1_16[59], quot1_16[58], quot1_16[57], quot1_16[56], quot1_16[55], quot1_16[54], quot1_16[53], quot1_16[52], quot1_16[51], quot1_16[50], quot1_16[49], quot1_16[48], quot1_16[47]);
assign div_overflow = quot1_16[62]| quot1_16[61]| quot1_16[60]| quot1_16[59]| quot1_16[58]| quot1_16[57]| quot1_16[56]| quot1_16[55]| quot1_16[54]| quot1_16[53]| quot1_16[52]| quot1_16[51]| quot1_16[50]| quot1_16[49]| quot1_16[48]| quot1_16[47];

//Used to determine whether or not the multiply operation overflowed.
//or U2(toAnd1_36_1, prod1_36[62], prod1_36[61], prod1_36[60], prod1_36[59], prod1_36[58], prod1_36[57], prod1_36[56], prod1_36[55], prod1_36[54], prod1_36[53], prod1_36[52], prod1_36[51], prod1_36[50], prod1_36[49], prod1_36[48], prod1_36[47], prod1_36[46]);
assign toAnd1_36_1 = prod1_36[62]| prod1_36[61]| prod1_36[60]| prod1_36[59]| prod1_36[58]| prod1_36[57]| prod1_36[56]| prod1_36[55]| prod1_36[54]| prod1_36[53]| prod1_36[52]| prod1_36[51]| prod1_36[50]| prod1_36[49]| prod1_36[48]| prod1_36[47]| prod1_36[46];
//Used to determine whether or not the multiply operation overflowed in the negative direction.
//or U3(toAnd1_36_2, ~prod1_36[62], ~prod1_36[61], ~prod1_36[60], ~prod1_36[59], ~prod1_36[58], ~prod1_36[57], ~prod1_36[56], ~prod1_36[55], ~prod1_36[54], ~prod1_36[53], ~prod1_36[52], ~prod1_36[51], ~prod1_36[50], ~prod1_36[49], ~prod1_36[48], ~prod1_36[47], ~prod1_36[46]);
assign toAnd1_36_2 = ~prod1_36[62]| ~prod1_36[61]| ~prod1_36[60]| ~prod1_36[59]| ~prod1_36[58]| ~prod1_36[57]| ~prod1_36[56]| ~prod1_36[55]| ~prod1_36[54]| ~prod1_36[53]| ~prod1_36[52]| ~prod1_36[51]| ~prod1_36[50]| ~prod1_36[49]| ~prod1_36[48]| ~prod1_36[47]| ~prod1_36[46];

//and U4(overflow1_36, ~prod1_36[63], toAnd1_36_1);
assign overflow1_36 = ~prod1_36[63]| toAnd1_36_1;
//and U5(negOverflow1_36, prod1_36[63], toAnd1_36_2);
assign negOverflow1_36 = prod1_36[63]| toAnd1_36_2;


//Used to determine whether or not the multiply operation overflowed.
//or U6(toAnd2_36_1, prod2_36[62], prod2_36[61], prod2_36[60], prod2_36[59], prod2_36[58], prod2_36[57], prod2_36[56], prod2_36[55], prod2_36[54], prod2_36[53], prod2_36[52], prod2_36[51], prod2_36[50], prod2_36[49], prod2_36[48], prod2_36[47], prod2_36[46]);
assign toAnd2_36_1 = prod2_36[62]| prod2_36[61]| prod2_36[60]| prod2_36[59]| prod2_36[58]| prod2_36[57]| prod2_36[56]| prod2_36[55]| prod2_36[54]| prod2_36[53]| prod2_36[52]| prod2_36[51]| prod2_36[50]| prod2_36[49]| prod2_36[48]| prod2_36[47]| prod2_36[46];
//Used to determine whether or not the multiply operation overflowed in the negative direction.
//or U7(toAnd2_36_2, ~prod2_36[62], ~prod2_36[61], ~prod2_36[60], ~prod2_36[59], ~prod2_36[58], ~prod2_36[57], ~prod2_36[56], ~prod2_36[55], ~prod2_36[54], ~prod2_36[53], ~prod2_36[52], ~prod2_36[51], ~prod2_36[50], ~prod2_36[49], ~prod2_36[48], ~prod2_36[47], ~prod2_36[46]);
assign toAnd2_36_2 = ~prod2_36[62]| ~prod2_36[61]| ~prod2_36[60]| ~prod2_36[59]| ~prod2_36[58]| ~prod2_36[57]| ~prod2_36[56]| ~prod2_36[55]| ~prod2_36[54]| ~prod2_36[53]| ~prod2_36[52]| ~prod2_36[51]| ~prod2_36[50]| ~prod2_36[49]| ~prod2_36[48]| ~prod2_36[47]| ~prod2_36[46];

//and U8(overflow2_36, ~prod2_36[63], toAnd2_36_1);
assign overflow2_36 = ~prod2_36[63]|toAnd2_36_1;
//and U9(negOverflow2_36, prod2_36[63], toAnd2_36_2);
assign negOverflow2_36 = prod2_36[63]|toAnd2_36_2;



//CC 2
assign new_uz2						= prod1_2;
//CC 3
sub_64b		oneMinusUz2_sub(
				.dataa(`INTMAX_2),
				.datab(uz2__2),
				.result(new_oneMinusUz2)
		);

//CC 4
assign new_uxUz						= prod1_4;
//CC `SQRT+5
assign new_sqrtOneMinusUz2			= sqrtResult1_6;
//CC `SQRT+`DIV+3
assign new_sintCosp					= prod1_33;
assign new_sintSinp					= prod2_33;
assign new_uyUz						= prod3_33;
//CC `SQRT+`DIV+4
assign new_sint						= sint_Mem;
assign new_cost						= cost_Mem;
assign new_sinp						= sinp_Mem;
assign new_cosp						= cosp_Mem;
assign new_uxSintSinp				= prod1_34;
assign new_uySintSinp				= prod2_34;
assign new_uxUzSintCosp				= prod3_34;
assign new_uyUzSintCosp				= prod4_34;
//CC `SQRT+`DIV+5
sub_32b		uxNumer_sub(
				.dataa(uxUzSintCosp__34),
				.datab(uySintSinp__34),
				.overflow(uxNumerOverflow),
				.result(new_uxNumerator)
			);

add_32b		uyNumer_add(
				.dataa(uyUzSintCosp__34),
				.datab(uxSintSinp__34),
				.overflow(uyNumerOverflow),
				.result(new_uyNumerator)
			);


//Possibility for division overflow (whereby the inverse is too large).  Data storage for this
//value is 15 bits left of the decimal, and 16 bits to the right.
assign new_sqrtOneMinusUz2_inv			=  (div_overflow) ? `INTMAX		:	{quot1_16[63:63], quot1_16[46:16]};

//CC `SQRT+`DIV+6
always @ (overflow1_36 or negOverflow1_36 or prod1_36 or overflow2_36 or negOverflow2_36 or prod2_36) begin
	case ({overflow1_36, negOverflow1_36})
	0:	new_uxQuotient = {prod1_36[63:63], prod1_36[45:15]};
	1:	new_uxQuotient = `INTMIN;
	2:	new_uxQuotient = `INTMAX;
	//Should never occur
	3:	new_uxQuotient = {prod1_36[63:63], prod1_36[45:15]};
	endcase
	
	case ({overflow2_36, negOverflow2_36})
	
	0:	new_uyQuotient = {prod2_36[63:63], prod2_36[45:15]};
	1:	new_uyQuotient = `INTMIN;
	2:	new_uyQuotient = `INTMAX;
		//Should never occur
	3:	new_uyQuotient = {prod2_36[63:63], prod2_36[45:15]};
	endcase
end

//always @ (*) begin
//	new_uxQuotient = {prod1_36[63:63], prod1_36[47:16]};
//	new_uyQuotient = {prod2_36[63:63], prod2_36[47:16]};
//end

assign new_sintCospSqrtOneMinusUz2	= prod3_36;
assign new_uxCost					= prod4_36;
assign new_uyCost					= prod5_36;
assign new_uzCost					= prod6_36;



//-----------------------FINAL RESULT CALCULATIONS--------------
//
//
//
//
//
//
//
//At this point, all calculations have been completed, save the
//final results.  This part of the code decides whether or not the
//current calculation involved a normal (orthogonal) incident or not,
//and uses this information to determine how to calculate the
//final results.  Final results are put on wires new_ux, new_uy, and
//new_uz, where they are output to registers ux_scatterer,
//uy_scatterer, and uz_scatterer on the clock event for synchronization
//(registered outputs, as per the convention).



//Determine whether or not the photon calculation was done on a photon that
//was normal (orthogonal) to the plane of interest.  This is to avoid divide
//by zero errors
always @ (i_uz36) begin
	//If uz >= `INTMAX-3 || uz <= -`INTMAX+3, normal incident
	if(i_uz36 == 32'h7FFFFFFF || i_uz36 == 32'h7FFFFFFE || i_uz36 == 32'h7FFFFFFD || i_uz36 == 32'h7FFFFFFC || i_uz36 == 32'h80000000 || i_uz36 == 32'h80000001 || i_uz36 == 32'h80000002 || i_uz36 == 32'h80000003 || i_uz36 == 32'h80000004) begin
		normalIncident = 1'b1;
	end else begin
		normalIncident = 1'b0;
	end
end



//Assign calculation values for final ux result
assign ux_add_1 = (normalIncident) ? sintCosp__36	:	uxQuotient__36;
assign ux_add_2 = (normalIncident) ? 32'b0		:	uxCost__36;

add_32b		ux_add(
				.dataa(ux_add_1),
				.datab(ux_add_2),
				.overflow(uxOverflow),
				.result(new_ux)
			);

//Assign calculation values for final uy result
assign uy_add_1 = (normalIncident)	? sintSinp__36	:	uyQuotient__36;
assign uy_add_2 = (normalIncident)	? 32'b0		:	uyCost__36;

add_32b		uy_add(
				.dataa(uy_add_1),
				.datab(uy_add_2),
				.overflow(uyOverflow),
				.result(new_uy)
			);




//Assign calculation values for final uz result.
//First MUX implements SIGN(uz) function.
assign normalUz = (i_uz36 >=0)		? cost__36		:	-cost__36;
assign uz_sub_1 = (normalIncident)	? normalUz			:	uzCost__36;
assign uz_sub_2 = (normalIncident)	? 32'b0		:	sintCospSqrtOneMinusUz2__36;

sub_32b		uz_sub(
				.dataa(uz_sub_1),
				.datab(uz_sub_2),
				.overflow(uzOverflow),
				.result(new_uz)
			);
			
				

endmodule


//***********************************
//Mathematical modules
//***********************************

module sub_64b (dataa, datab, result);

	input [63:0] dataa;
	input [63:0] datab;
	output [63:0] result;

	assign result = dataa - datab;

endmodule

module add_32b (dataa, datab, overflow, result);

	input [31:0] dataa;
	input [31:0] datab;
	output overflow;
	output [31:0] result;

	wire [32:0]computation; //one extra bit to account for overflow
	
	assign computation = dataa + datab;
	assign overflow = computation[32];
	assign result = computation[31:0];

endmodule 

module sub_32b (dataa, datab, overflow, result); 

	input [31:0] dataa;
	input [31:0] datab;
	output overflow;
	output [31:0] result;

	wire [32:0]computation; //one extra bit to account for overflow
	
	assign computation = dataa - datab;
	assign overflow = computation[32];
	assign result = computation[31:0];

endmodule 

module Mult_32b (dataa, datab, result); //now signed version!

	input [31:0]dataa;
	input [31:0]datab;
	output [63:0]result;
	
	// assign result = dataa * datab; 
	
	wire [31:0]a;
	wire [31:0]b;
  assign a = dataa;
  assign b = datab;

	reg [63:0]c;
	assign result = c;
	
	reg is_neg_a;
	reg is_neg_b;
	reg [31:0]a_tmp;
	reg [31:0]b_tmp;
	reg [63:0]mult_tmp;
	reg [63:0]c_tmp;

always@(a or b or is_neg_a or is_neg_b or a_tmp or b_tmp or c)
begin
	if(a[31] == 1) begin
		a_tmp = -a;
		is_neg_a = 1;
	end else
	begin
		a_tmp = a;
		is_neg_a = 0;
	end

	if(b[31] == 1) begin
		b_tmp = -b;
		is_neg_b = 1;
	end else
	begin
		b_tmp = b;
		is_neg_b = 0;
	end

	mult_tmp = a_tmp * b_tmp;

	if( is_neg_a != is_neg_b) begin
		c_tmp = -mult_tmp;
	end else
	begin
		c_tmp = mult_tmp;
	end
end

always@(c_tmp)
begin
	c = c_tmp;
end

endmodule



module Div_64b (clock, denom, numer, quotient, remain);
	input clock;
	input [63:0]numer;
	input [31:0]denom;
	output [63:0]quotient;
	reg [63:0]quotient;
	output [31:0]remain;
	reg [31:0]remain;
	
	wire [63:0]quotient_temp;
	wire [31:0]remain_temp;
	Div_64b_unsigned div_temp(.clock(clock), .denom_(denom), .numer_(numer), .quotient(quotient_temp), .remain(remain_temp));
	
	always @ (numer or denom or quotient_temp or remain_temp) begin
		if ( numer[63]^denom[31] ) begin // only one is negative
			quotient = -quotient_temp;
			remain = -remain_temp;
		end else begin
			quotient = quotient_temp;
			remain = remain_temp;
		end
	end 
	
endmodule


/*module Div_64b (clock, denom, numer, quotient, remain);
	input clock;
	input [63:0]numer;
	input [31:0]denom;
	output [63:0]quotient;
	reg [63:0]quotient;
	output [31:0]remain;
	reg [31:0]remain; */
	
module Div_64b_unsigned (clock, denom_, numer_, quotient, remain);
	input clock;
	input [63:0]numer_;
	input [31:0]denom_;
	output [63:0]quotient;
	output [31:0]remain;

	reg [63:0]numer;
	reg [31:0]denom0;
	
	always @ (posedge clock) 
	begin
		numer <= numer_;
		denom0 <= denom_;
	end

///////////////////////////////////////////////////Unchanged starts here	
	reg [94:0]numer_temp_63; //need to add bits
	reg [94:0]numer_temp_62;
	reg [94:0]numer_temp_61;
	reg [94:0]numer_temp_60_d, numer_temp_60_q;
	reg [94:0]numer_temp_59;
	reg [94:0]numer_temp_58;
	reg [94:0]numer_temp_57_d, numer_temp_57_q;
	reg [94:0]numer_temp_56;
	reg [94:0]numer_temp_55;
	reg [94:0]numer_temp_54_d, numer_temp_54_q;
	reg [94:0]numer_temp_53;
	reg [94:0]numer_temp_52;
	reg [94:0]numer_temp_51_d, numer_temp_51_q;
	reg [94:0]numer_temp_50;
	reg [94:0]numer_temp_49;
	reg [94:0]numer_temp_48_d, numer_temp_48_q;
	reg [94:0]numer_temp_47;
	reg [94:0]numer_temp_46;
	reg [94:0]numer_temp_45_d, numer_temp_45_q;
	reg [94:0]numer_temp_44;
	reg [94:0]numer_temp_43;
	reg [94:0]numer_temp_42_d, numer_temp_42_q;
	reg [94:0]numer_temp_41;
	reg [94:0]numer_temp_40;
	reg [94:0]numer_temp_39_d, numer_temp_39_q;
	reg [94:0]numer_temp_38;
	reg [94:0]numer_temp_37;
	reg [94:0]numer_temp_36_d, numer_temp_36_q;
	reg [94:0]numer_temp_35;
	reg [94:0]numer_temp_34;
	reg [94:0]numer_temp_33_d, numer_temp_33_q;
	reg [94:0]numer_temp_32;
	reg [94:0]numer_temp_31;
	reg [94:0]numer_temp_30_d, numer_temp_30_q;
	reg [94:0]numer_temp_29;
	reg [94:0]numer_temp_28;
	reg [94:0]numer_temp_27_d, numer_temp_27_q;
	reg [94:0]numer_temp_26;
	reg [94:0]numer_temp_25;
	reg [94:0]numer_temp_24;
	reg [94:0]numer_temp_23_d, numer_temp_23_q;
	reg [94:0]numer_temp_22;
	reg [94:0]numer_temp_21;
	reg [94:0]numer_temp_20;
	reg [94:0]numer_temp_19_d, numer_temp_19_q;
	reg [94:0]numer_temp_18;
	reg [94:0]numer_temp_17;
	reg [94:0]numer_temp_16;
	reg [94:0]numer_temp_15_d, numer_temp_15_q;
	reg [94:0]numer_temp_14;
	reg [94:0]numer_temp_13;
	reg [94:0]numer_temp_12;
	reg [94:0]numer_temp_11_d, numer_temp_11_q;
	reg [94:0]numer_temp_10;
	reg [94:0]numer_temp_9;
	reg [94:0]numer_temp_8;
	reg [94:0]numer_temp_7_d, numer_temp_7_q;
	reg [94:0]numer_temp_6;
	reg [94:0]numer_temp_5;
	reg [94:0]numer_temp_4;
	reg [94:0]numer_temp_3_d, numer_temp_3_q;
	reg [94:0]numer_temp_2;
	reg [94:0]numer_temp_1_d, numer_temp_1_q;
	reg [94:0]numer_temp_0;
	reg [94:0]numer_temp;

		//The dummy pipeline (20 clock cycles)
	reg [63:0]quo0_d;
	reg [63:0]quo1_d;
	reg [63:0]quo2_d;
	reg [63:0]quo3_d;
	reg [63:0]quo4_d;
	reg [63:0]quo5_d;
	reg [63:0]quo6_d;
	reg [63:0]quo7_d;
	reg [63:0]quo8_d;
	reg [63:0]quo9_d;
	reg [63:0]quo10_d;
	reg [63:0]quo11_d;
	reg [63:0]quo12_d;
	reg [63:0]quo13_d;
	reg [63:0]quo14_d;
	reg [63:0]quo15_d;
	reg [63:0]quo16_d;
	reg [63:0]quo17_d;
	reg [63:0]quo18_d;
	reg [63:0]quo19_d;
	
	reg [63:0]quo0_q;
	reg [63:0]quo1_q;
	reg [63:0]quo2_q;
	reg [63:0]quo3_q;
	reg [63:0]quo4_q;
	reg [63:0]quo5_q;
	reg [63:0]quo6_q;
	reg [63:0]quo7_q;
	reg [63:0]quo8_q;
	reg [63:0]quo9_q;
	reg [63:0]quo10_q;
	reg [63:0]quo11_q;
	reg [63:0]quo12_q;
	reg [63:0]quo13_q;
	reg [63:0]quo14_q;
	reg [63:0]quo15_q;
	reg [63:0]quo16_q;
	reg [63:0]quo17_q;
	reg [63:0]quo18_q;
	
	reg [31:0]denom1;
	reg [31:0]denom2;
	reg [31:0]denom3;	
	reg [31:0]denom4;
	reg [31:0]denom5;	
	reg [31:0]denom6;
	reg [31:0]denom7;	
	reg [31:0]denom8;
	reg [31:0]denom9;	
	reg [31:0]denom10;
	reg [31:0]denom11;	
	reg [31:0]denom12;
	reg [31:0]denom13;	
	reg [31:0]denom14;
	reg [31:0]denom15;	
	reg [31:0]denom16;
	reg [31:0]denom17;	
	reg [31:0]denom18;
	reg [31:0]denom19;
	
	
	always @(numer or denom0) begin
		numer_temp_63 = {31'b0, numer};
		
		//quo0[63]
		if (numer_temp_63[94:63] >= denom0 ) begin
			quo0_d[63] = 1'b1;
			numer_temp_62 = {numer_temp_63[94:63] - denom0, numer_temp_63[62:0]};
		end else begin
			quo0_d[63] = 1'b0;
			numer_temp_62 = numer_temp_63;
		end
		
		//quo0[62]
		if (numer_temp_62[94:62] >= denom0 ) begin
			quo0_d[62] = 1'b1;
			numer_temp_61 = {numer_temp_62[94:62] - denom0, numer_temp_62[61:0]};
		end else begin
			quo0_d[62] = 1'b0;
           numer_temp_61 = numer_temp_62;
		end
		//quo0[61]
		if (numer_temp_61[94:61] >= denom0 ) begin
			quo0_d[61] = 1'b1;
			numer_temp_60_d = {numer_temp_61[94:61] - denom0, numer_temp_61[60:0]};
		end else begin
			quo0_d[61] = 1'b0;
           numer_temp_60_d = numer_temp_61;
		end
		quo0_d[60:0] = 61'b0;
	end
	
	always @ (posedge clock) begin
		quo0_q <= quo0_d;
		numer_temp_60_q <= numer_temp_60_d;
		denom1 <= denom0;
	end
		
	always @(numer_temp_60_q or denom1 or quo0_q) begin
		quo1_d[63:61] = quo0_q[63:61];
	
		//quo1_d[60]
		if (numer_temp_60_q[94:60] >= denom1 ) begin
			quo1_d[60] = 1'b1;
			numer_temp_59 = {numer_temp_60_q[94:60] - denom1, numer_temp_60_q[59:0]};
		end else begin
			quo1_d[60] = 1'b0;
           numer_temp_59 = numer_temp_60_q;
		end
		//quo1_d[59]
		if (numer_temp_59[94:59] >= denom1 ) begin
			quo1_d[59] = 1'b1;
			numer_temp_58 = {numer_temp_59[94:59] - denom1, numer_temp_59[58:0]};
		end else begin
			quo1_d[59] = 1'b0;
           numer_temp_58 = numer_temp_59;
		end
		//quo1_d[58]
		if (numer_temp_58[94:58] >= denom1 ) begin
			quo1_d[58] = 1'b1;
			numer_temp_57_d = {numer_temp_58[94:58] - denom1, numer_temp_58[57:0]};
		end else begin
			quo1_d[58] = 1'b0;
           numer_temp_57_d = numer_temp_58;
		end
		quo1_d[57:0] = 58'b0;
	end

	always @ (posedge clock) begin
		quo1_q <= quo1_d;
		numer_temp_57_q <= numer_temp_57_d;
		denom2 <= denom1;
	end
		
	always @ (numer_temp_57_q or denom2 or quo1_q) begin
		quo2_d[63:58] = quo1_q[63:58];
	
		//quo2_d[57]
		if (numer_temp_57_q[94:57] >= denom2 ) begin
			quo2_d[57] = 1'b1;
			numer_temp_56 = {numer_temp_57_q[94:57] - denom2, numer_temp_57_q[56:0]};
		end else begin
			quo2_d[57] = 1'b0;
           numer_temp_56 = numer_temp_57_q;
		end
		//quo2_d[56]
		if (numer_temp_56[94:56] >= denom2 ) begin
			quo2_d[56] = 1'b1;
			numer_temp_55 = {numer_temp_56[94:56] - denom2, numer_temp_56[55:0]};
		end else begin
			quo2_d[56] = 1'b0;
           numer_temp_55 = numer_temp_56;
		end
		//quo2_d[55]
		if (numer_temp_55[94:55] >= denom2 ) begin
			quo2_d[55] = 1'b1;
			numer_temp_54_d = {numer_temp_55[94:55] - denom2, numer_temp_55[54:0]};
		end else begin
			quo2_d[55] = 1'b0;
           numer_temp_54_d = numer_temp_55;
		end
		quo2_d[54:0] = 55'b0;
	end
	
	
	always @ (posedge clock) begin
		quo2_q <= quo2_d;
		numer_temp_54_q <= numer_temp_54_d;
		denom3 <= denom2;
	end
	
	always @ (numer_temp_54_q or denom3 or quo2_q) begin
		quo3_d[63:55] = quo2_q[63:55];
	
		//quo3_d[54]
		if (numer_temp_54_q[94:54] >= denom3 ) begin
			quo3_d[54] = 1'b1;
			numer_temp_53 = {numer_temp_54_q[94:54] - denom3, numer_temp_54_q[53:0]};
		end else begin
			quo3_d[54] = 1'b0;
           numer_temp_53 = numer_temp_54_q;
		end
		//quo3_d[53]
		if (numer_temp_53[94:53] >= denom3 ) begin
			quo3_d[53] = 1'b1;
			numer_temp_52 = {numer_temp_53[94:53] - denom3, numer_temp_53[52:0]};
		end else begin
			quo3_d[53] = 1'b0;
           numer_temp_52 = numer_temp_53;
		end
		//quo3_d[52]
		if (numer_temp_52[94:52] >= denom3 ) begin
			quo3_d[52] = 1'b1;
			numer_temp_51_d = {numer_temp_52[94:52] - denom3, numer_temp_52[51:0]};
		end else begin
			quo3_d[52] = 1'b0;
           numer_temp_51_d = numer_temp_52;
		end
		quo3_d[51:0] = 52'b0;
	end
		
	always @ (posedge clock) begin
		quo3_q <= quo3_d;
		numer_temp_51_q <= numer_temp_51_d;
		denom4 <= denom3;
	end
		
	always @ (numer_temp_51_q or denom4 or quo3_q) begin
		quo4_d[63:52] = quo3_q[63:52];
	
		//quo4[51]
		if (numer_temp_51_q[94:51] >= denom4 ) begin
			quo4_d[51] = 1'b1;
			numer_temp_50 = {numer_temp_51_q[94:51] - denom4, numer_temp_51_q[50:0]};
		end else begin
			quo4_d[51] = 1'b0;
           numer_temp_50 = numer_temp_51_q;
		end
		//quo4_d[50]
		if (numer_temp_50[94:50] >= denom4 ) begin
			quo4_d[50] = 1'b1;
			numer_temp_49 = {numer_temp_50[94:50] - denom4, numer_temp_50[49:0]};
		end else begin
			quo4_d[50] = 1'b0;
           numer_temp_49 = numer_temp_50;
		end
		//quo4_d[49]
		if (numer_temp_49[94:49] >= denom4 ) begin
			quo4_d[49] = 1'b1;
			numer_temp_48_d = {numer_temp_49[94:49] - denom4, numer_temp_49[48:0]};
		end else begin
			quo4_d[49] = 1'b0;
           numer_temp_48_d = numer_temp_49;
		end
		quo4_d[48:0] = 49'b0;
	end
		
	always @ (posedge clock) begin
		quo4_q <= quo4_d;
		numer_temp_48_q <= numer_temp_48_d;
		denom5 <= denom4;
	end
		
	always @ (numer_temp_48_q or denom5 or quo4_q) begin
		quo5_d[63:49] = quo4_q[63:49];

	//quo5_d[48]
		if (numer_temp_48_q[94:48] >= denom5 ) begin
			quo5_d[48] = 1'b1;
			numer_temp_47 = {numer_temp_48_q[94:48] - denom5, numer_temp_48_q[47:0]};
		end else begin
			quo5_d[48] = 1'b0;
           numer_temp_47 = numer_temp_48_q;
		end
		//quo5_d[47]
		if (numer_temp_47[94:47] >= denom5 ) begin
			quo5_d[47] = 1'b1;
			numer_temp_46 = {numer_temp_47[94:47] - denom5, numer_temp_47[46:0]};
		end else begin
			quo5_d[47] = 1'b0;
           numer_temp_46 = numer_temp_47;
		end
		//quo5_d[46]
		if (numer_temp_46[94:46] >= denom5 ) begin
			quo5_d[46] = 1'b1;
			numer_temp_45_d = {numer_temp_46[94:46] - denom5, numer_temp_46[45:0]};
		end else begin
			quo5_d[46] = 1'b0;
           numer_temp_45_d = numer_temp_46;
		end
		quo5_d[45:0] = 46'b0;
	end
		
	always @ (posedge clock) begin
		quo5_q <= quo5_d;
		numer_temp_45_q <= numer_temp_45_d;
		denom6 <= denom5;
	end
		
	always @ (numer_temp_45_q or denom6 or quo5_q) begin
		quo6_d[63:46] = quo5_q[63:46];
	
		//quo6_d[45]
		if (numer_temp_45_q[94:45] >= denom6 ) begin
			quo6_d[45] = 1'b1;
			numer_temp_44 = {numer_temp_45_q[94:45] - denom6, numer_temp_45_q[44:0]};
		end else begin
			quo6_d[45] = 1'b0;
           numer_temp_44 = numer_temp_45_q;
		end
		//quo6_d[44]
		if (numer_temp_44[94:44] >= denom6 ) begin
			quo6_d[44] = 1'b1;
			numer_temp_43 = {numer_temp_44[94:44] - denom6, numer_temp_44[43:0]};
		end else begin
			quo6_d[44] = 1'b0;
           numer_temp_43 = numer_temp_44;
		end
		//quo6_d[43]
		if (numer_temp_43[94:43] >= denom6 ) begin
			quo6_d[43] = 1'b1;
			numer_temp_42_d = {numer_temp_43[94:43] - denom6, numer_temp_43[42:0]};
		end else begin
			quo6_d[43] = 1'b0;
           numer_temp_42_d = numer_temp_43;
		end
		quo6_d[42:0] = 43'b0;
	end
	
	always @ (posedge clock) begin
		quo6_q<= quo6_d;
		numer_temp_42_q <= numer_temp_42_d;
		denom7 <= denom6;
	end
	
	always @ (numer_temp_42_q or denom7 or quo6_q) begin
		quo7_d[63:43] = quo6_q[63:43];
	
		//quo7_d[42]
		if (numer_temp_42_q[94:42] >= denom7 ) begin
			quo7_d[42] = 1'b1;
			numer_temp_41 = {numer_temp_42_q[94:42] - denom7, numer_temp_42_q[41:0]};
		end else begin
			quo7_d[42] = 1'b0;
           numer_temp_41 = numer_temp_42_q;
		end
		//quo7_d[41]
		if (numer_temp_41[94:41] >= denom7 ) begin
			quo7_d[41] = 1'b1;
			numer_temp_40 = {numer_temp_41[94:41] - denom7, numer_temp_41[40:0]};
		end else begin
			quo7_d[41] = 1'b0;
           numer_temp_40 = numer_temp_41;
		end
		//quo7_d[40]
		if (numer_temp_40[94:40] >= denom7 ) begin
			quo7_d[40] = 1'b1;
			numer_temp_39_d = {numer_temp_40[94:40] - denom7, numer_temp_40[39:0]};
		end else begin
			quo7_d[40] = 1'b0;
           numer_temp_39_d = numer_temp_40;
		end
		quo7_d[39:0] = 40'b0;
	end
		
	always @ (posedge clock) begin
		quo7_q <= quo7_d;
		numer_temp_39_q <= numer_temp_39_d;
		denom8 <= denom7;
	end
		
	always @ (numer_temp_39_q or denom8 or quo7_q) begin
		quo8_d[63:40] = quo7_q[63:40];
	
		//quo8[39]
		if (numer_temp_39_q[94:39] >= denom8 ) begin
			quo8_d[39] = 1'b1;
			numer_temp_38 = {numer_temp_39_q[94:39] - denom8, numer_temp_39_q[38:0]};
		end else begin
			quo8_d[39] = 1'b0;
           numer_temp_38 = numer_temp_39_q;
		end
		//quo8_d[38]
		if (numer_temp_38[94:38] >= denom8 ) begin
			quo8_d[38] = 1'b1;
			numer_temp_37 = {numer_temp_38[94:38] - denom8, numer_temp_38[37:0]};
		end else begin
			quo8_d[38] = 1'b0;
           numer_temp_37 = numer_temp_38;
		end
		//quo8_d[37]
		if (numer_temp_37[94:37] >= denom8 ) begin
			quo8_d[37] = 1'b1;
			numer_temp_36_d = {numer_temp_37[94:37] - denom8, numer_temp_37[36:0]};
		end else begin
			quo8_d[37] = 1'b0;
           numer_temp_36_d = numer_temp_37;
		end
		quo8_d[36:0] = 37'b0;
	end
		
	always @ (posedge clock) begin
		quo8_q <= quo8_d;
		numer_temp_36_q <= numer_temp_36_d;
		denom9 <= denom8;
	end
		
	always @ (numer_temp_36_q or denom9 or quo8_q) begin
		quo9_d[63:37] = quo8_q[63:37];
	
		//quo9[36]
		if (numer_temp_36_q[94:36] >= denom9 ) begin
			quo9_d[36] = 1'b1;
			numer_temp_35 = {numer_temp_36_q[94:36] - denom9, numer_temp_36_q[35:0]};
		end else begin
			quo9_d[36] = 1'b0;
           numer_temp_35 = numer_temp_36_q;
		end
		//quo9_d[35]
		if (numer_temp_35[94:35] >= denom9 ) begin
			quo9_d[35] = 1'b1;
			numer_temp_34 = {numer_temp_35[94:35] - denom9, numer_temp_35[34:0]};
		end else begin
			quo9_d[35] = 1'b0;
           numer_temp_34 = numer_temp_35;
		end
		//quo9_d[34]
		if (numer_temp_34[94:34] >= denom9 ) begin
			quo9_d[34] = 1'b1;
			numer_temp_33_d = {numer_temp_34[94:34] - denom9, numer_temp_34[33:0]};
		end else begin
			quo9_d[34] = 1'b0;
           numer_temp_33_d = numer_temp_34;
		end
		quo9_d[33:0] = 34'b0;
	end
		
	always @ (posedge clock) begin
		quo9_q <= quo9_d;
		numer_temp_33_q <= numer_temp_33_d;
		denom10 <= denom9;
	end
		
	always @ (numer_temp_33_q or denom10 or quo9_q) begin
		quo10_d[63:34] = quo9_q[63:34];
	
		//quo10_d[33]
		if (numer_temp_33_q[94:33] >= denom10 ) begin
			quo10_d[33] = 1'b1;
			numer_temp_32 = {numer_temp_33_q[94:33] - denom10, numer_temp_33_q[32:0]};
		end else begin
			quo10_d[33] = 1'b0;
           numer_temp_32 = numer_temp_33_q;
		end
		//quo10_d[32]
		if (numer_temp_32[94:32] >= denom10 ) begin
			quo10_d[32] = 1'b1;
			numer_temp_31 = {numer_temp_32[94:32] - denom10, numer_temp_32[31:0]};
		end else begin
			quo10_d[32] = 1'b0;
           numer_temp_31 = numer_temp_32;
		end
		//quo10_d[31]
		if (numer_temp_31[94:31] >= denom10 ) begin
			quo10_d[31] = 1'b1;
			numer_temp_30_d = {numer_temp_31[94:31] - denom10, numer_temp_31[30:0]};
		end else begin
			quo10_d[31] = 1'b0;
           numer_temp_30_d = numer_temp_31;
		end
		quo10_d[30:0] = 31'b0;
	end
	
	always @ (posedge clock) begin
		quo10_q <= quo10_d;
		numer_temp_30_q <= numer_temp_30_d;
		denom11 <= denom10;
	end
		
	always @ (numer_temp_30_q or denom11 or quo10_q) begin 
		quo11_d[63:31] = quo10_q[63:31];
	
		//quo11[30]
		if (numer_temp_30_q[94:30] >= denom11 ) begin
			quo11_d[30] = 1'b1;
			numer_temp_29 = {numer_temp_30_q[94:30] - denom11, numer_temp_30_q[29:0]};
		end else begin
			quo11_d[30] = 1'b0;
           numer_temp_29 = numer_temp_30_q;
		end
		//quo11_d[29]
		if (numer_temp_29[94:29] >= denom11 ) begin
			quo11_d[29] = 1'b1;
			numer_temp_28 = {numer_temp_29[94:29] - denom11, numer_temp_29[28:0]};
		end else begin
			quo11_d[29] = 1'b0;
           numer_temp_28 = numer_temp_29;
		end
		//quo11_d[28]
		if (numer_temp_28[94:28] >= denom11 ) begin
			quo11_d[28] = 1'b1;
			numer_temp_27_d = {numer_temp_28[94:28] - denom11, numer_temp_28[27:0]};
		end else begin
			quo11_d[28] = 1'b0;
           numer_temp_27_d = numer_temp_28;
		end
		quo11_d[27:0] = 28'b0;
	end
		
	always @ (posedge clock) begin
		quo11_q <= quo11_d;
		numer_temp_27_q <= numer_temp_27_d;
		denom12 <= denom11;
	end
	
	always @ (numer_temp_27_q or denom12 or quo11_q) begin
		quo12_d[63:28] = quo11_q[63:28];
	
		//quo12[27]
		if (numer_temp_27_q[94:27] >= denom12 ) begin
			quo12_d[27] = 1'b1;
			numer_temp_26 = {numer_temp_27_q[94:27] - denom12, numer_temp_27_q[26:0]};
		end else begin
			quo12_d[27] = 1'b0;
           numer_temp_26 = numer_temp_27_q;
		end
		//quo12_d[26]
		if (numer_temp_26[94:26] >= denom12 ) begin
			quo12_d[26] = 1'b1;
			numer_temp_25 = {numer_temp_26[94:26] - denom12, numer_temp_26[25:0]};
		end else begin
			quo12_d[26] = 1'b0;
           numer_temp_25 = numer_temp_26;
		end
		//quo12_d[25]
		if (numer_temp_25[94:25] >= denom12 ) begin
			quo12_d[25] = 1'b1;
			numer_temp_24 = {numer_temp_25[94:25] - denom12, numer_temp_25[24:0]};
		end else begin
			quo12_d[25] = 1'b0;
           numer_temp_24 = numer_temp_25;
		end
		//quo12_d[24]
		if (numer_temp_24[94:24] >= denom12 ) begin
			quo12_d[24] = 1'b1;
			numer_temp_23_d = {numer_temp_24[94:24] - denom12, numer_temp_24[23:0]};
		end else begin
			quo12_d[24] = 1'b0;
           numer_temp_23_d = numer_temp_24;
		end
		quo12_d[23:0] = 24'b0;
	end
		
	always @ (posedge clock) begin
		quo12_q <= quo12_d;
		numer_temp_23_q <= numer_temp_23_d;
		denom13 <= denom12;
	end
	
	always @ (numer_temp_23_q or denom13 or quo12_q) begin
		quo13_d[63:24] = quo12_q[63:24];
	
		//quo13_d[23]
		if (numer_temp_23_q[94:23] >= denom13 ) begin
			quo13_d[23] = 1'b1;
			numer_temp_22 = {numer_temp_23_q[94:23] - denom13, numer_temp_23_q[22:0]};
		end else begin
			quo13_d[23] = 1'b0;
           numer_temp_22 = numer_temp_23_q;
		end
		//quo13_d[22]
		if (numer_temp_22[94:22] >= denom13 ) begin
			quo13_d[22] = 1'b1;
			numer_temp_21 = {numer_temp_22[94:22] - denom13, numer_temp_22[21:0]};
		end else begin
			quo13_d[22] = 1'b0;
           numer_temp_21 = numer_temp_22;
		end
		//quo13_d[21]
		if (numer_temp_21[94:21] >= denom13 ) begin
			quo13_d[21] = 1'b1;
			numer_temp_20 = {numer_temp_21[94:21] - denom13, numer_temp_21[20:0]};
		end else begin
			quo13_d[21] = 1'b0;
           numer_temp_20 = numer_temp_21;
		end
		//quo13_d[20]
		if (numer_temp_20[94:20] >= denom13 ) begin
			quo13_d[20] = 1'b1;
			numer_temp_19_d = {numer_temp_20[94:20] - denom13, numer_temp_20[19:0]};
		end else begin
			quo13_d[20] = 1'b0;
           numer_temp_19_d = numer_temp_20;
		end
		quo13_d[19:0] = 20'b0;
	end
	
	always @ (posedge clock) begin
		quo13_q <= quo13_d;
		numer_temp_19_q <= numer_temp_19_d;
		denom14 <= denom13;
	end
		
	always @ (numer_temp_19_q or denom14 or quo13_q) begin
		quo14_d[63:20] = quo13_q[63:20];
	
		//quo14_d[19]
		if (numer_temp_19_q[94:19] >= denom14 ) begin
			quo14_d[19] = 1'b1;
			numer_temp_18 = {numer_temp_19_q[94:19] - denom14, numer_temp_19_q[18:0]};
		end else begin
			quo14_d[19] = 1'b0;
           numer_temp_18 = numer_temp_19_q;
		end
		//quo14_d[18]
		if (numer_temp_18[94:18] >= denom14 ) begin
			quo14_d[18] = 1'b1;
			numer_temp_17 = {numer_temp_18[94:18] - denom14, numer_temp_18[17:0]};
		end else begin
			quo14_d[18] = 1'b0;
           numer_temp_17 = numer_temp_18;
		end
		//quo14_d[17]
		if (numer_temp_17[94:17] >= denom14 ) begin
			quo14_d[17] = 1'b1;
			numer_temp_16 = {numer_temp_17[94:17] - denom14, numer_temp_17[16:0]};
		end else begin
			quo14_d[17] = 1'b0;
           numer_temp_16 = numer_temp_17;
		end
		//quo14_d[16]
		if (numer_temp_16[94:16] >= denom14 ) begin
			quo14_d[16] = 1'b1;
			numer_temp_15_d = {numer_temp_16[94:16] - denom14, numer_temp_16[15:0]};
		end else begin
			quo14_d[16] = 1'b0;
           numer_temp_15_d = numer_temp_16;
		end
		quo14_d[15:0] = 16'b0;
	end
		
	always @ (posedge clock) begin
		quo14_q <= quo14_d;
		numer_temp_15_q <= numer_temp_15_d;
		denom15 <= denom14;
	end
		
	always @ (numer_temp_15_q or denom15 or quo14_q) begin
		quo15_d[63:16] = quo14_q[63:16];
	
		//quo15_d[15]
		if (numer_temp_15_q[94:15] >= denom15 ) begin
			quo15_d[15] = 1'b1;
			numer_temp_14 = {numer_temp_15_q[94:15] - denom15, numer_temp_15_q[14:0]};
		end else begin
			quo15_d[15] = 1'b0;
           numer_temp_14 = numer_temp_15_q;
		end
		//quo15_d[14]
		if (numer_temp_14[94:14] >= denom15 ) begin
			quo15_d[14] = 1'b1;
			numer_temp_13 = {numer_temp_14[94:14] - denom15, numer_temp_14[13:0]};
		end else begin
			quo15_d[14] = 1'b0;
           numer_temp_13 = numer_temp_14;
		end
		//quo15_d[13]
		if (numer_temp_13[94:13] >= denom15 ) begin
			quo15_d[13] = 1'b1;
			numer_temp_12 = {numer_temp_13[94:13] - denom15, numer_temp_13[12:0]};
		end else begin
			quo15_d[13] = 1'b0;
           numer_temp_12 = numer_temp_13;
		end
		//quo15_d[12]
		if (numer_temp_12[94:12] >= denom15 ) begin
			quo15_d[12] = 1'b1;
			numer_temp_11_d = {numer_temp_12[94:12] - denom15, numer_temp_12[11:0]};
		end else begin
			quo15_d[12] = 1'b0;
           numer_temp_11_d = numer_temp_12;
		end
		quo15_d[11:0] = 12'b0;
	end
		
	always @ (posedge clock) begin
		quo15_q <= quo15_d;
		numer_temp_11_q <= numer_temp_11_d;
		denom16 <= denom15;
	end
		
	always @ (numer_temp_11_q or denom16 or quo15_q) begin
		quo16_d[63:12] = quo15_q[63:12];
	
		//quo16_d[11]
		if (numer_temp_11_q[94:11] >= denom16 ) begin
			quo16_d[11] = 1'b1;
			numer_temp_10 = {numer_temp_11_q[94:11] - denom16, numer_temp_11_q[10:0]};
		end else begin
			quo16_d[11] = 1'b0;
           numer_temp_10 = numer_temp_11_q;
		end
		//quo16_d[10]
		if (numer_temp_10[94:10] >= denom16 ) begin
			quo16_d[10] = 1'b1;
			numer_temp_9 = {numer_temp_10[94:10] - denom16, numer_temp_10[9:0]};
		end else begin
			quo16_d[10] = 1'b0;
           numer_temp_9 = numer_temp_10;
		end
		//quo16_d[9]
		if (numer_temp_9[94:9] >= denom16 ) begin
			quo16_d[9] = 1'b1;
			numer_temp_8 = {numer_temp_9[94:9] - denom16, numer_temp_9[8:0]};
		end else begin
			quo16_d[9] = 1'b0;
           numer_temp_8 = numer_temp_9;
		end
		//quo16_d[8]
		if (numer_temp_8[94:8] >= denom16 ) begin
			quo16_d[8] = 1'b1;
			numer_temp_7_d = {numer_temp_8[94:8] - denom16, numer_temp_8[7:0]};
		end else begin
			quo16_d[8] = 1'b0;
           numer_temp_7_d = numer_temp_8;
		end
		quo16_d[7:0] = 8'b0;
	end
	
	always @ (posedge clock) begin
		quo16_q <= quo16_d;
		numer_temp_7_q <= numer_temp_7_d;
		denom17 <= denom16;
	end
		
	always @ (numer_temp_7_q or denom17 or quo16_q) begin
		quo17_d[63:8] = quo16_q[63:8];
	
		//quo17_d[7]
		if (numer_temp_7_q[94:7] >= denom17 ) begin
			quo17_d[7] = 1'b1;
			numer_temp_6 = {numer_temp_7_q[94:7] - denom17, numer_temp_7_q[6:0]};
		end else begin
			quo17_d[7] = 1'b0;
           numer_temp_6 = numer_temp_7_q;
		end
		//quo17_d[6]
		if (numer_temp_6[94:6] >= denom17 ) begin
			quo17_d[6] = 1'b1;
			numer_temp_5 = {numer_temp_6[94:6] - denom17, numer_temp_6[5:0]};
		end else begin
			quo17_d[6] = 1'b0;
           numer_temp_5 = numer_temp_6;
		end
		//quo17_d[5]
		if (numer_temp_5[94:5] >= denom17 ) begin
			quo17_d[5] = 1'b1;
			numer_temp_4 = {numer_temp_5[94:5] - denom17, numer_temp_5[4:0]};
		end else begin
			quo17_d[5] = 1'b0;
           numer_temp_4 = numer_temp_5;
		end
		//quo17_d[4]
		if (numer_temp_4[94:4] >= denom17 ) begin
			quo17_d[4] = 1'b1;
			numer_temp_3_d = {numer_temp_4[94:4] - denom17, numer_temp_4[3:0]};
		end else begin
			quo17_d[4] = 1'b0;
           numer_temp_3_d = numer_temp_4;
		end
		quo17_d[3:0] = 4'b0;
	end
	
	always @ (posedge clock) begin
		quo17_q <= quo17_d;
		numer_temp_3_q <= numer_temp_3_d;
		denom18 <= denom17;
	end
		
	always @ (numer_temp_3_q or denom18 or quo17_q) begin
		quo18_d[63:4] = quo17_q[63:4];
		
		//quo18_d[3]
		if (numer_temp_3_q[94:3] >= denom18 ) begin
			quo18_d[3] = 1'b1;
			numer_temp_2 = {numer_temp_3_q[94:3] - denom18, numer_temp_3_q[2:0]};
		end else begin
			quo18_d[3] = 1'b0;
           numer_temp_2 = numer_temp_3_q;
		end
		//quo18_d[2]
		if (numer_temp_2[94:2] >= denom18 ) begin
			quo18_d[2] = 1'b1;
			numer_temp_1_d = {numer_temp_2[94:2] - denom18, numer_temp_2[1:0]};
		end else begin
			quo18_d[2] = 1'b0;
           numer_temp_1_d = numer_temp_2;
		end
		quo18_d[1:0] = 2'b0;
	end
		
	always @ (posedge clock) begin 
		quo18_q <= quo18_d;
		numer_temp_1_q <= numer_temp_1_d;
		denom19 <= denom18;
	end
		
	always @ (numer_temp_1_q or denom19 or quo18_q) begin
		quo19_d[63:2] = quo18_q[63:2];
		//quo19_d[1]
		if (numer_temp_1_q[94:1] >= denom19 ) begin
			quo19_d[1] = 1'b1;
			numer_temp_0 = {numer_temp_1_q[94:1] - denom19, numer_temp_1_q[0:0]};
		end else begin
			quo19_d[1] = 1'b0;
           numer_temp_0 = numer_temp_1_q;
	
		end
		//quo19_d[0]
		if (numer_temp_0[94:0] >= denom19 ) begin
			quo19_d[0] = 1'b1;
			numer_temp = numer_temp_0[94:0] - denom19;
		end else begin
			quo19_d[0] = 1'b0;
           numer_temp = numer_temp_0;
		end	
	end
	
	assign quotient = quo19_d;
	assign remain = numer_temp[31:0];

	
	
endmodule 

/*module sqrt_64b (clk, num, res);
	input clk;
	input [63:0]num;
	output [31:0]res;
	reg [31:0]res;*/
	
//`timescale 1 ns / 1 ps

module Sqrt_64b (clk, num_, res);
	input clk;
	input [63:0]num_;
	output [31:0]res;
	reg [31:0]res;
	
	reg [63:0]num;
	
	always @ (posedge clk)
	begin
		num <= num_;
	end
	
///////////////////////////////////////////////////Unchanged starts here
	
//	reg [63:0] one_[32:0];
//	reg [63:0] res_[32:0];
//	reg [63:0] op_[32:0];

	wire [63:0]one__0;
    reg  [63:0]one__1;
    reg  [63:0]one__2;
    reg  [63:0]one__3_d, one__3_q;
    reg  [63:0]one__4;
    reg  [63:0]one__5;
    reg  [63:0]one__6;
    reg  [63:0]one__7_d, one__7_q;
    reg  [63:0]one__8;
    reg  [63:0]one__9;
    reg  [63:0]one__10;
    reg  [63:0]one__11_d, one__11_q;
    reg  [63:0]one__12;
    reg  [63:0]one__13;
    reg  [63:0]one__14;
    reg  [63:0]one__15_d, one__15_q;
    reg  [63:0]one__16;
    reg  [63:0]one__17;
    reg  [63:0]one__18_d, one__18_q;
    reg  [63:0]one__19;
    reg  [63:0]one__20;
    reg  [63:0]one__21_d, one__21_q;
    reg  [63:0]one__22;
    reg  [63:0]one__23;
    reg  [63:0]one__24_d, one__24_q;
    reg  [63:0]one__25;
    reg  [63:0]one__26;
    reg  [63:0]one__27_d, one__27_q;
    reg  [63:0]one__28;
    reg  [63:0]one__29;
    reg  [63:0]one__30_d, one__30_q;
    reg  [63:0]one__31;
	reg  [63:0]one__32;

	wire [63:0]res__0;
    reg  [63:0]res__1;
    reg  [63:0]res__2;
    reg  [63:0]res__3_d, res__3_q;
    reg  [63:0]res__4;
    reg  [63:0]res__5;
    reg  [63:0]res__6;
    reg  [63:0]res__7_d, res__7_q;
    reg  [63:0]res__8;
    reg  [63:0]res__9;
    reg  [63:0]res__10;
    reg  [63:0]res__11_d, res__11_q;
    reg  [63:0]res__12;
    reg  [63:0]res__13;
    reg  [63:0]res__14;
    reg  [63:0]res__15_d, res__15_q;
    reg  [63:0]res__16;
    reg  [63:0]res__17;
    reg  [63:0]res__18_d, res__18_q;
    reg  [63:0]res__19;
    reg  [63:0]res__20;
    reg  [63:0]res__21_d, res__21_q;
    reg  [63:0]res__22;
    reg  [63:0]res__23;
    reg  [63:0]res__24_d, res__24_q;
    reg  [63:0]res__25;
    reg  [63:0]res__26;
    reg  [63:0]res__27_d, res__27_q;
    reg  [63:0]res__28;
    reg  [63:0]res__29;
    reg  [63:0]res__30_d, res__30_q;
    reg  [63:0]res__31;
	reg  [63:0]res__32;
    
	wire [63:0]op__0;
	reg  [63:0]op__1;
    reg  [63:0]op__2;
    reg  [63:0]op__3_d, op__3_q;
    reg  [63:0]op__4;
    reg  [63:0]op__5;
    reg  [63:0]op__6;
    reg  [63:0]op__7_d, op__7_q;
    reg  [63:0]op__8;
    reg  [63:0]op__9;
    reg  [63:0]op__10;
    reg  [63:0]op__11_d, op__11_q;
    reg  [63:0]op__12;
    reg  [63:0]op__13;
    reg  [63:0]op__14;
    reg  [63:0]op__15_d, op__15_q;
    reg  [63:0]op__16;
    reg  [63:0]op__17;
    reg  [63:0]op__18_d, op__18_q;
    reg  [63:0]op__19;
    reg  [63:0]op__20;
    reg  [63:0]op__21_d, op__21_q;
    reg  [63:0]op__22;
    reg  [63:0]op__23;
    reg  [63:0]op__24_d, op__24_q;
    reg  [63:0]op__25;
    reg  [63:0]op__26;
    reg  [63:0]op__27_d, op__27_q;
    reg  [63:0]op__28;
    reg  [63:0]op__29;
    reg  [63:0]op__30_d, op__30_q;
    reg  [63:0]op__31;
	reg  [63:0]op__32;

	
	reg [63:0]one; //This is the one that is selected in first expanded loop
	reg [31:0]one_tmp;
	
	always @ (num) begin
		
		//The first for-loop:
		//all of these will be zero no matter how 'one' is selected.
		one[1] = 0;
		one[3] = 0;
		one[5] = 0;
		one[7] = 0;
		one[9] = 0;
		one[11] = 0;
		one[13] = 0;
		one[15] = 0;
		one[17] = 0;
		one[19] = 0;
		one[21] = 0;
		one[23] = 0;
		one[25] = 0;
		one[27] = 0;
		one[29] = 0;
		one[31] = 0;
		one[33] = 0;
		one[35] = 0;
		one[37] = 0;
		one[39] = 0;
		one[41] = 0;
		one[43] = 0;
		one[45] = 0;
		one[47] = 0;
		one[49] = 0;
		one[51] = 0;
		one[53] = 0;
		one[55] = 0;
		one[57] = 0;
		one[59] = 0;
		one[61] = 0;
		one[63] = 0;
		
		one_tmp[0] = num[0]|num[1];
		one_tmp[1] = num[2]|num[3];
		one_tmp[2] = num[4]|num[5];
		one_tmp[3] = num[6]|num[7];
		one_tmp[4] = num[8]|num[9];
		one_tmp[5] = num[10]|num[11];
		one_tmp[6] = num[12]|num[13];
		one_tmp[7] = num[14]|num[15];
		one_tmp[8] = num[16]|num[17];
		one_tmp[9] = num[18]|num[19];
		one_tmp[10] = num[20]|num[21];
		one_tmp[11] = num[22]|num[23];
		one_tmp[12] = num[24]|num[25];
		one_tmp[13] = num[26]|num[27];
		one_tmp[14] = num[28]|num[29];
		one_tmp[15] = num[30]|num[31];
		one_tmp[16] = num[32]|num[33];
		one_tmp[17] = num[34]|num[35];
		one_tmp[18] = num[36]|num[37];
		one_tmp[19] = num[38]|num[39];
		one_tmp[20] = num[40]|num[41];
		one_tmp[21] = num[42]|num[43];
		one_tmp[22] = num[44]|num[45];
		one_tmp[23] = num[46]|num[47];
		one_tmp[24] = num[48]|num[49];
		one_tmp[25] = num[50]|num[51];
		one_tmp[26] = num[52]|num[53];
		one_tmp[27] = num[54]|num[55];
		one_tmp[28] = num[56]|num[57];
		one_tmp[29] = num[58]|num[59];
		one_tmp[30] = num[60]|num[61];
		one_tmp[31] = num[62]|num[63];
		
		one[0] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&~one_tmp[27]&~one_tmp[26]&~one_tmp[25]&~one_tmp[24]&~one_tmp[23]&~one_tmp[22]&~one_tmp[21]&~one_tmp[20]&~one_tmp[19]&~one_tmp[18]&~one_tmp[17]&~one_tmp[16]&~one_tmp[15]&~one_tmp[14]&~one_tmp[13]&~one_tmp[12]&~one_tmp[11]&~one_tmp[10]&~one_tmp[9]&~one_tmp[8]&~one_tmp[7]&~one_tmp[6]&~one_tmp[5]&~one_tmp[4]&~one_tmp[3]&~one_tmp[2]&~one_tmp[1]&one_tmp[0];
		one[2] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&~one_tmp[27]&~one_tmp[26]&~one_tmp[25]&~one_tmp[24]&~one_tmp[23]&~one_tmp[22]&~one_tmp[21]&~one_tmp[20]&~one_tmp[19]&~one_tmp[18]&~one_tmp[17]&~one_tmp[16]&~one_tmp[15]&~one_tmp[14]&~one_tmp[13]&~one_tmp[12]&~one_tmp[11]&~one_tmp[10]&~one_tmp[9]&~one_tmp[8]&~one_tmp[7]&~one_tmp[6]&~one_tmp[5]&~one_tmp[4]&~one_tmp[3]&~one_tmp[2]&one_tmp[1];
		one[4] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&~one_tmp[27]&~one_tmp[26]&~one_tmp[25]&~one_tmp[24]&~one_tmp[23]&~one_tmp[22]&~one_tmp[21]&~one_tmp[20]&~one_tmp[19]&~one_tmp[18]&~one_tmp[17]&~one_tmp[16]&~one_tmp[15]&~one_tmp[14]&~one_tmp[13]&~one_tmp[12]&~one_tmp[11]&~one_tmp[10]&~one_tmp[9]&~one_tmp[8]&~one_tmp[7]&~one_tmp[6]&~one_tmp[5]&~one_tmp[4]&~one_tmp[3]&one_tmp[2];
		one[6] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&~one_tmp[27]&~one_tmp[26]&~one_tmp[25]&~one_tmp[24]&~one_tmp[23]&~one_tmp[22]&~one_tmp[21]&~one_tmp[20]&~one_tmp[19]&~one_tmp[18]&~one_tmp[17]&~one_tmp[16]&~one_tmp[15]&~one_tmp[14]&~one_tmp[13]&~one_tmp[12]&~one_tmp[11]&~one_tmp[10]&~one_tmp[9]&~one_tmp[8]&~one_tmp[7]&~one_tmp[6]&~one_tmp[5]&~one_tmp[4]&one_tmp[3];
		one[8] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&~one_tmp[27]&~one_tmp[26]&~one_tmp[25]&~one_tmp[24]&~one_tmp[23]&~one_tmp[22]&~one_tmp[21]&~one_tmp[20]&~one_tmp[19]&~one_tmp[18]&~one_tmp[17]&~one_tmp[16]&~one_tmp[15]&~one_tmp[14]&~one_tmp[13]&~one_tmp[12]&~one_tmp[11]&~one_tmp[10]&~one_tmp[9]&~one_tmp[8]&~one_tmp[7]&~one_tmp[6]&~one_tmp[5]&one_tmp[4];
		one[10] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&~one_tmp[27]&~one_tmp[26]&~one_tmp[25]&~one_tmp[24]&~one_tmp[23]&~one_tmp[22]&~one_tmp[21]&~one_tmp[20]&~one_tmp[19]&~one_tmp[18]&~one_tmp[17]&~one_tmp[16]&~one_tmp[15]&~one_tmp[14]&~one_tmp[13]&~one_tmp[12]&~one_tmp[11]&~one_tmp[10]&~one_tmp[9]&~one_tmp[8]&~one_tmp[7]&~one_tmp[6]&one_tmp[5];
		one[12] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&~one_tmp[27]&~one_tmp[26]&~one_tmp[25]&~one_tmp[24]&~one_tmp[23]&~one_tmp[22]&~one_tmp[21]&~one_tmp[20]&~one_tmp[19]&~one_tmp[18]&~one_tmp[17]&~one_tmp[16]&~one_tmp[15]&~one_tmp[14]&~one_tmp[13]&~one_tmp[12]&~one_tmp[11]&~one_tmp[10]&~one_tmp[9]&~one_tmp[8]&~one_tmp[7]&one_tmp[6];
		one[14] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&~one_tmp[27]&~one_tmp[26]&~one_tmp[25]&~one_tmp[24]&~one_tmp[23]&~one_tmp[22]&~one_tmp[21]&~one_tmp[20]&~one_tmp[19]&~one_tmp[18]&~one_tmp[17]&~one_tmp[16]&~one_tmp[15]&~one_tmp[14]&~one_tmp[13]&~one_tmp[12]&~one_tmp[11]&~one_tmp[10]&~one_tmp[9]&~one_tmp[8]&one_tmp[7];
		one[16] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&~one_tmp[27]&~one_tmp[26]&~one_tmp[25]&~one_tmp[24]&~one_tmp[23]&~one_tmp[22]&~one_tmp[21]&~one_tmp[20]&~one_tmp[19]&~one_tmp[18]&~one_tmp[17]&~one_tmp[16]&~one_tmp[15]&~one_tmp[14]&~one_tmp[13]&~one_tmp[12]&~one_tmp[11]&~one_tmp[10]&~one_tmp[9]&one_tmp[8];
		one[18] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&~one_tmp[27]&~one_tmp[26]&~one_tmp[25]&~one_tmp[24]&~one_tmp[23]&~one_tmp[22]&~one_tmp[21]&~one_tmp[20]&~one_tmp[19]&~one_tmp[18]&~one_tmp[17]&~one_tmp[16]&~one_tmp[15]&~one_tmp[14]&~one_tmp[13]&~one_tmp[12]&~one_tmp[11]&~one_tmp[10]&one_tmp[9];
		one[20] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&~one_tmp[27]&~one_tmp[26]&~one_tmp[25]&~one_tmp[24]&~one_tmp[23]&~one_tmp[22]&~one_tmp[21]&~one_tmp[20]&~one_tmp[19]&~one_tmp[18]&~one_tmp[17]&~one_tmp[16]&~one_tmp[15]&~one_tmp[14]&~one_tmp[13]&~one_tmp[12]&~one_tmp[11]&one_tmp[10];
		one[22] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&~one_tmp[27]&~one_tmp[26]&~one_tmp[25]&~one_tmp[24]&~one_tmp[23]&~one_tmp[22]&~one_tmp[21]&~one_tmp[20]&~one_tmp[19]&~one_tmp[18]&~one_tmp[17]&~one_tmp[16]&~one_tmp[15]&~one_tmp[14]&~one_tmp[13]&~one_tmp[12]&one_tmp[11];
		one[24] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&~one_tmp[27]&~one_tmp[26]&~one_tmp[25]&~one_tmp[24]&~one_tmp[23]&~one_tmp[22]&~one_tmp[21]&~one_tmp[20]&~one_tmp[19]&~one_tmp[18]&~one_tmp[17]&~one_tmp[16]&~one_tmp[15]&~one_tmp[14]&~one_tmp[13]&one_tmp[12];
		one[26] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&~one_tmp[27]&~one_tmp[26]&~one_tmp[25]&~one_tmp[24]&~one_tmp[23]&~one_tmp[22]&~one_tmp[21]&~one_tmp[20]&~one_tmp[19]&~one_tmp[18]&~one_tmp[17]&~one_tmp[16]&~one_tmp[15]&~one_tmp[14]&one_tmp[13];
		one[28] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&~one_tmp[27]&~one_tmp[26]&~one_tmp[25]&~one_tmp[24]&~one_tmp[23]&~one_tmp[22]&~one_tmp[21]&~one_tmp[20]&~one_tmp[19]&~one_tmp[18]&~one_tmp[17]&~one_tmp[16]&~one_tmp[15]&one_tmp[14];
		one[30] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&~one_tmp[27]&~one_tmp[26]&~one_tmp[25]&~one_tmp[24]&~one_tmp[23]&~one_tmp[22]&~one_tmp[21]&~one_tmp[20]&~one_tmp[19]&~one_tmp[18]&~one_tmp[17]&~one_tmp[16]&one_tmp[15];
		one[32] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&~one_tmp[27]&~one_tmp[26]&~one_tmp[25]&~one_tmp[24]&~one_tmp[23]&~one_tmp[22]&~one_tmp[21]&~one_tmp[20]&~one_tmp[19]&~one_tmp[18]&~one_tmp[17]&one_tmp[16];
		one[34] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&~one_tmp[27]&~one_tmp[26]&~one_tmp[25]&~one_tmp[24]&~one_tmp[23]&~one_tmp[22]&~one_tmp[21]&~one_tmp[20]&~one_tmp[19]&~one_tmp[18]&one_tmp[17];
		one[36] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&~one_tmp[27]&~one_tmp[26]&~one_tmp[25]&~one_tmp[24]&~one_tmp[23]&~one_tmp[22]&~one_tmp[21]&~one_tmp[20]&~one_tmp[19]&one_tmp[18];
		one[38] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&~one_tmp[27]&~one_tmp[26]&~one_tmp[25]&~one_tmp[24]&~one_tmp[23]&~one_tmp[22]&~one_tmp[21]&~one_tmp[20]&one_tmp[19];
		one[40] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&~one_tmp[27]&~one_tmp[26]&~one_tmp[25]&~one_tmp[24]&~one_tmp[23]&~one_tmp[22]&~one_tmp[21]&one_tmp[20];
		one[42] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&~one_tmp[27]&~one_tmp[26]&~one_tmp[25]&~one_tmp[24]&~one_tmp[23]&~one_tmp[22]&one_tmp[21];
		one[44] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&~one_tmp[27]&~one_tmp[26]&~one_tmp[25]&~one_tmp[24]&~one_tmp[23]&one_tmp[22];
		one[46] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&~one_tmp[27]&~one_tmp[26]&~one_tmp[25]&~one_tmp[24]&one_tmp[23];
		one[48] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&~one_tmp[27]&~one_tmp[26]&~one_tmp[25]&one_tmp[24];
		one[50] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&~one_tmp[27]&~one_tmp[26]&one_tmp[25];
		one[52] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&~one_tmp[27]&one_tmp[26];
		one[54] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&~one_tmp[28]&one_tmp[27];
		one[56] = ~one_tmp[31]&~one_tmp[30]&~one_tmp[29]&one_tmp[28];
		one[58] = ~one_tmp[31]&~one_tmp[30]&one_tmp[29];
		one[60] = ~one_tmp[31]&one_tmp[30];
		one[62] = one_tmp[31];
	end
	
//	//2nd for-loop:
//	integer i;
//	always @ (*) begin
//		op_[0] = num;
//		one_[0] = one;
//		res_[0] = 64'b0;
//		res = 63'b0;
//		res_assigned = 1'b0;
//	
//		for (i = 0; i <= 31; i=i+1) begin
//			if ((one_[i] == 0) & ~res_assigned) begin
//				res = res_[i];
//				res_assigned = 1'b1;
//			end
//			
//			//Define the next stage:
//			if (op_[i] >= res_[i] + one_[i]) begin
//				op_[i+1] = op_[i] - res_[i] - one_[i];
//				res_[i+1] = (res_[i]>>1) + one_[i];
//			end else begin
//				op_[i+1] = op_[i]; //this line had to be added for the verilog version.
//				res_[i+1] = (res_[i]>>1);
//			end
//			one_[i+1] = (one_[i] >> 2);
//		end
//	
//		//Add the part for really big numbers later:
//		if (~res_assigned) begin
//			res = res_[32];
//		end
//	end

	//If-statement about defining the next stage:
	assign op__0 = num;
	assign res__0 = 64'b0;
	assign one__0 = one;
	
	always @ (res__0 or op__0 or one__0) begin

       //i = 0
       if (op__0 >= res__0 + one__0) begin
           op__1 = op__0 - res__0 - one__0;
           res__1 = (res__0>>1) + one__0;
       end else begin
           op__1 = op__0;
           res__1 = (res__0>>1);
       end
       one__1 = (one__0 >> 2);

       //i = 1
       if (op__1 >= res__1 + one__1) begin
           op__2 = op__1 - res__1 - one__1;
           res__2 = (res__1>>1) + one__1;
       end else begin
           op__2 = op__1;
           res__2 = (res__1>>1);
       end
       one__2 = (one__1 >> 2);

       //i = 2
       if (op__2 >= res__2 + one__2) begin
           op__3_d = op__2 - res__2 - one__2;
           res__3_d = (res__2>>1) + one__2;
       end else begin
           op__3_d = op__2;
           res__3_d = (res__2>>1);
       end
       one__3_d = (one__2 >> 2);
	end
	
	always @ (posedge clk) begin
		op__3_q <= op__3_d;
		res__3_q <= res__3_d;
		one__3_q <= one__3_d;
	end
	
	always @ (op__3_q or res__3_q or one__3_q) begin
       //i = 3
       if (op__3_q >= res__3_q + one__3_q) begin
           op__4 = op__3_q - res__3_q - one__3_q;
           res__4 = (res__3_q>>1) + one__3_q;
       end else begin
           op__4 = op__3_q;
           res__4 = (res__3_q>>1);
       end
       one__4 = (one__3_q >> 2);

       //i = 4
       if (op__4 >= res__4 + one__4) begin
           op__5 = op__4 - res__4 - one__4;
           res__5 = (res__4>>1) + one__4;
       end else begin
           op__5 = op__4;
           res__5 = (res__4>>1);
       end
       one__5 = (one__4 >> 2);

       //i = 5
       if (op__5 >= res__5 + one__5) begin
           op__6 = op__5 - res__5 - one__5;
           res__6 = (res__5>>1) + one__5;
       end else begin
           op__6 = op__5;
           res__6 = (res__5>>1);
       end
       one__6 = (one__5 >> 2);

       //i = 6
       if (op__6 >= res__6 + one__6) begin
           op__7_d = op__6 - res__6 - one__6;
           res__7_d = (res__6>>1) + one__6;
       end else begin
           op__7_d = op__6;
           res__7_d = (res__6>>1);
       end
       one__7_d = (one__6 >> 2);
	end
		 
	always @ (posedge clk) begin
		op__7_q <= op__7_d;
		one__7_q <= one__7_d;
		res__7_q <= res__7_d;
	end
	
	always @ (op__7_q or res__7_q or one__7_q) begin
       //i = 7
       if (op__7_q >= res__7_q + one__7_q) begin
           op__8 = op__7_q - res__7_q - one__7_q;
           res__8 = (res__7_q>>1) + one__7_q;
       end else begin
           op__8 = op__7_q;
           res__8 = (res__7_q>>1);
       end
       one__8 = (one__7_q >> 2);

       //i = 8
       if (op__8 >= res__8 + one__8) begin
           op__9 = op__8 - res__8 - one__8;
           res__9 = (res__8>>1) + one__8;
       end else begin
           op__9 = op__8;
           res__9 = (res__8>>1);
       end
       one__9 = (one__8 >> 2);

       //i = 9
       if (op__9 >= res__9 + one__9) begin
           op__10 = op__9 - res__9 - one__9;
           res__10 = (res__9>>1) + one__9;
       end else begin
           op__10 = op__9;
           res__10 = (res__9>>1);
       end
       one__10 = (one__9 >> 2);

       //i = 10
       if (op__10 >= res__10 + one__10) begin
           op__11_d = op__10 - res__10 - one__10;
           res__11_d = (res__10>>1) + one__10;
       end else begin
           op__11_d = op__10;
           res__11_d = (res__10>>1);
       end
       one__11_d = (one__10 >> 2);
	end
		 
	always @ (posedge clk) begin
		op__11_q <= op__11_d;
		one__11_q <= one__11_d;
		res__11_q <= res__11_d;
	end
	
	always @ (op__11_q or res__11_q or one__11_q) begin	 
       //i = 11
       if (op__11_q >= res__11_q + one__11_q) begin
           op__12 = op__11_q - res__11_q - one__11_q;
           res__12 = (res__11_q>>1) + one__11_q;
       end else begin
           op__12 = op__11_q;
           res__12 = (res__11_q>>1);
       end
       one__12 = (one__11_q >> 2);

       //i = 12
       if (op__12 >= res__12 + one__12) begin
           op__13 = op__12 - res__12 - one__12;
           res__13 = (res__12>>1) + one__12;
       end else begin
           op__13 = op__12;
           res__13 = (res__12>>1);
       end
       one__13 = (one__12 >> 2);

       //i = 13
       if (op__13 >= res__13 + one__13) begin
           op__14 = op__13 - res__13 - one__13;
           res__14 = (res__13>>1) + one__13;
       end else begin
           op__14 = op__13;
           res__14 = (res__13>>1);
       end
       one__14 = (one__13 >> 2);

       //i = 14
       if (op__14 >= res__14 + one__14) begin
           op__15_d = op__14 - res__14 - one__14;
           res__15_d = (res__14>>1) + one__14;
       end else begin
           op__15_d = op__14;
           res__15_d = (res__14>>1);
       end
       one__15_d = (one__14 >> 2);
	end
	
	always @ (posedge clk) begin
		op__15_q <= op__15_d;
		one__15_q <= one__15_d;
		res__15_q <= res__15_d;
	end
	
	always @ (op__15_q or res__15_q or one__15_q) begin
       //i = 15
       if (op__15_q >= res__15_q + one__15_q) begin
           op__16 = op__15_q - res__15_q - one__15_q;
           res__16 = (res__15_q>>1) + one__15_q;
       end else begin
           op__16 = op__15_q;
           res__16 = (res__15_q>>1);
       end
       one__16 = (one__15_q >> 2);

       //i = 16
       if (op__16 >= res__16 + one__16) begin
           op__17 = op__16 - res__16 - one__16;
           res__17 = (res__16>>1) + one__16;
       end else begin
           op__17 = op__16;
           res__17 = (res__16>>1);
       end
       one__17 = (one__16 >> 2);

       //i = 17
       if (op__17 >= res__17 + one__17) begin
           op__18_d = op__17 - res__17 - one__17;
           res__18_d = (res__17>>1) + one__17;
       end else begin
           op__18_d = op__17;
           res__18_d = (res__17>>1);
       end
       one__18_d = (one__17 >> 2);
	end
	
	always @ (posedge clk) begin
		op__18_q <= op__18_d;
		one__18_q <= one__18_d;
		res__18_q <= res__18_d;
	end
	
	always @ (op__18_q or res__18_q or one__18_q) begin
       //i = 18
       if (op__18_q >= res__18_q + one__18_q) begin
           op__19 = op__18_q - res__18_q - one__18_q;
           res__19 = (res__18_q>>1) + one__18_q;
       end else begin
           op__19 = op__18_q;
           res__19 = (res__18_q>>1);
       end
       one__19 = (one__18_q >> 2);

       //i = 19
       if (op__19 >= res__19 + one__19) begin
           op__20 = op__19 - res__19 - one__19;
           res__20 = (res__19>>1) + one__19;
       end else begin
           op__20 = op__19;
           res__20 = (res__19>>1);
       end
       one__20 = (one__19 >> 2);

       //i = 20
       if (op__20 >= res__20 + one__20) begin
           op__21_d = op__20 - res__20 - one__20;
           res__21_d = (res__20>>1) + one__20;
       end else begin
           op__21_d = op__20;
           res__21_d = (res__20>>1);
       end
       one__21_d = (one__20 >> 2);
	end
	
	always @ (posedge clk) begin
		op__21_q <= op__21_d;
		one__21_q <= one__21_d;
		res__21_q <= res__21_d;
	end
		 
	always @ (op__21_q or res__21_q or one__21_q) begin 
       //i = 21
       if (op__21_q >= res__21_q + one__21_q) begin
           op__22 = op__21_q - res__21_q - one__21_q;
           res__22 = (res__21_q>>1) + one__21_q;
       end else begin
           op__22 = op__21_q;
           res__22 = (res__21_q>>1);
       end
       one__22 = (one__21_q >> 2);

       //i = 22
       if (op__22 >= res__22 + one__22) begin
           op__23 = op__22 - res__22 - one__22;
           res__23 = (res__22>>1) + one__22;
       end else begin
           op__23 = op__22;
           res__23 = (res__22>>1);
       end
       one__23 = (one__22 >> 2);

       //i = 23
       if (op__23 >= res__23 + one__23) begin
           op__24_d = op__23 - res__23 - one__23;
           res__24_d = (res__23>>1) + one__23;
       end else begin
           op__24_d = op__23;
           res__24_d = (res__23>>1);
       end
       one__24_d = (one__23 >> 2);
	end
		 
	always @ (posedge clk) begin
		op__24_q <= op__24_d;
		one__24_q <= one__24_d;
		res__24_q <= res__24_d;
	end
		  
	always @ (op__24_q or res__24_q or one__24_q) begin
       //i = 24
       if (op__24_q >= res__24_q + one__24_q) begin
           op__25 = op__24_q - res__24_q - one__24_q;
           res__25 = (res__24_q>>1) + one__24_q;
       end else begin
           op__25 = op__24_q;
           res__25 = (res__24_q>>1);
       end
       one__25 = (one__24_q >> 2);

       //i = 25
       if (op__25 >= res__25 + one__25) begin
           op__26 = op__25 - res__25 - one__25;
           res__26 = (res__25>>1) + one__25;
       end else begin
           op__26 = op__25;
           res__26 = (res__25>>1);
       end
       one__26 = (one__25 >> 2);

       //i = 26
       if (op__26 >= res__26 + one__26) begin
           op__27_d = op__26 - res__26 - one__26;
           res__27_d = (res__26>>1) + one__26;
       end else begin
           op__27_d = op__26;
           res__27_d = (res__26>>1);
       end
       one__27_d = (one__26 >> 2);
	end

	always @ (posedge clk) begin
		op__27_q <= op__27_d;
		one__27_q <= one__27_d;
		res__27_q <= res__27_d;
	end
	
	always @ (op__27_q or res__27_q or one__27_q) begin
       //i = 27
       if (op__27_q >= res__27_q + one__27_q) begin
           op__28 = op__27_q - res__27_q - one__27_q;
           res__28 = (res__27_q>>1) + one__27_q;
       end else begin
           op__28 = op__27_q;
           res__28 = (res__27_q>>1);
       end
       one__28 = (one__27_q >> 2);

       //i = 28
       if (op__28 >= res__28 + one__28) begin
           op__29 = op__28 - res__28 - one__28;
           res__29 = (res__28>>1) + one__28;
       end else begin
           op__29 = op__28;
           res__29 = (res__28>>1);
       end
       one__29 = (one__28 >> 2);

       //i = 29
       if (op__29 >= res__29 + one__29) begin
           op__30_d = op__29 - res__29 - one__29;
           res__30_d = (res__29>>1) + one__29;
       end else begin
           op__30_d = op__29;
           res__30_d = (res__29>>1);
       end
       one__30_d = (one__29 >> 2);
	end
	
	always @ (posedge clk) begin
		op__30_q <= op__30_d;
		one__30_q <= one__30_d;
		res__30_q <= res__30_d;
	end
	
	always @ (*) begin
       //i = 30
       if (op__30_q >= res__30_q + one__30_q) begin
           op__31 = op__30_q - res__30_q - one__30_q;
           res__31 = (res__30_q>>1) + one__30_q;
       end else begin
           op__31 = op__30_q;
           res__31 = (res__30_q>>1);
       end
       one__31 = (one__30_q >> 2);

       //i = 31
       if (op__31 >= res__31 + one__31) begin
           op__32 = op__31 - res__31 - one__31;
           res__32 = (res__31>>1) + one__31;
       end else begin
           op__32 = op__31;
           res__32 = (res__31>>1);
       end
       one__32 = (one__31 >> 2);
	end


	//If-statement about assigning res:
	always @ (*) begin
		if(one__0 == 0) begin
			res = res__0[31:0];
         end else if (one__1 == 0) begin
             res = res__1[31:0];
         end else if (one__2 == 0) begin
             res = res__2[31:0];
         end else if (one__3_q == 0) begin
             res = res__3_q[31:0];
         end else if (one__4 == 0) begin
             res = res__4[31:0];
         end else if (one__5 == 0) begin
             res = res__5[31:0];
         end else if (one__6 == 0) begin
             res = res__6[31:0];
         end else if (one__7_q == 0) begin
             res = res__7_q[31:0];
         end else if (one__8 == 0) begin
             res = res__8[31:0];
         end else if (one__9 == 0) begin
             res = res__9[31:0];
         end else if (one__10 == 0) begin
             res = res__10[31:0];
         end else if (one__11_q == 0) begin
             res = res__11_q[31:0];
         end else if (one__12 == 0) begin
             res = res__12[31:0];
         end else if (one__13 == 0) begin
             res = res__13[31:0];
         end else if (one__14 == 0) begin
             res = res__14[31:0];
         end else if (one__15_q == 0) begin
             res = res__15_q[31:0];
         end else if (one__16 == 0) begin
             res = res__16[31:0];
         end else if (one__17 == 0) begin
             res = res__17[31:0];
         end else if (one__18_q == 0) begin
             res = res__18_q[31:0];
         end else if (one__19 == 0) begin
             res = res__19[31:0];
         end else if (one__20 == 0) begin
             res = res__20[31:0];
         end else if (one__21_q == 0) begin
             res = res__21_q[31:0];
         end else if (one__22 == 0) begin
             res = res__22[31:0];
         end else if (one__23 == 0) begin
             res = res__23[31:0];
         end else if (one__24_q == 0) begin
             res = res__24_q[31:0];
         end else if (one__25 == 0) begin
             res = res__25[31:0];
         end else if (one__26 == 0) begin
             res = res__26[31:0];
         end else if (one__27_q == 0) begin
             res = res__27_q[31:0];
         end else if (one__28 == 0) begin
             res = res__28[31:0];
         end else if (one__29 == 0) begin
             res = res__29[31:0];
         end else if (one__30_q == 0) begin
             res = res__30_q[31:0];
         end else if (one__31 == 0) begin
             res = res__31[31:0];
		end else begin
			 res = res__32[31:0];
		end
		
	end

	
endmodule 	


