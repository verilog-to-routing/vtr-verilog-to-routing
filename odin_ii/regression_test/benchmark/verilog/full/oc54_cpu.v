
/////////////////////////////////////////////////////////////////////
////                                                             ////
////  OpenCores54 DSP, CPU                                       ////
////                                                             ////
////  Author: Richard Herveille                                  ////
////          richard@asics.ws                                   ////
////          www.asics.ws                                       ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2002 Richard Herveille                        ////
////                    richard@asics.ws                         ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
////     THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY     ////
//// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   ////
//// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   ////
//// FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE AUTHOR      ////
//// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,         ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    ////
//// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE   ////
//// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR        ////
//// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF  ////
//// LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT  ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  ////
//// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         ////
//// POSSIBILITY OF SUCH DAMAGE.                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
				
//
// NOTE: Read the pipeline information for the CMPS instruction
//

//
// Xilinx Virtex-E WC: 41 CLB slices @ 130MHz
//
									 
//  CVS Log														     
//																     
//  $Id: oc54_cpu.v,v 1.1.1.1 2002/04/10 09:34:41 rherveille Exp $														     
//																     
//  $Date: 2002/04/10 09:34:41 $														 
//  $Revision: 1.1.1.1 $													 
//  $Author: rherveille $													     
//  $Locker:  $													     
//  $State: Exp $														 
//																     
// Change History:												     
//               $Log: oc54_cpu.v,v $
//               Revision 1.1.1.1  2002/04/10 09:34:41  rherveille
//               Initial relase OpenCores54x DSP (CPU)
//											 
																
module oc54_cpu (
	clk_i, 
	ena_mac_i, ena_alu_i, ena_bs_i, ena_exp_i,
	ena_treg_i, ena_acca_i, ena_accb_i,

	// mac
	mac_sel_xm_i, mac_sel_ym_i, mac_sel_ya_i,
	mac_xm_sx_i, mac_ym_sx_i,	mac_add_sub_i,

	// alu
	alu_inst_i, alu_sel_i, alu_doublet_i,

	// bs
	bs_sel_i, bs_selo_i, l_na_i,
	cssu_sel_i, is_cssu_i,

	// 
	exp_sel_i, treg_sel_i, 
	acca_sel_i, accb_sel_i,

	// common
	pb_i, cb_i, db_i,
	bp_a_i, bp_b_i,

	ovm_i, frct_i, smul_i, sxm_i, c16_i, rnd_i,
	c_i, tc_i,
	asm_i, imm_i,

	// outputs
	c_alu_o, c_bs_o, tc_cssu_o, tc_alu_o,
	ovf_a_o, zf_a_o, ovf_b_o, zf_b_o,
	trn_o, eb_o
	);

//
// parameters
//

//
// inputs & outputs
//
input         clk_i;
input         ena_mac_i, ena_alu_i, ena_bs_i, ena_exp_i;
input         ena_treg_i, ena_acca_i, ena_accb_i;
input  [1:0] mac_sel_xm_i, mac_sel_ym_i, mac_sel_ya_i;
input         mac_xm_sx_i, mac_ym_sx_i, mac_add_sub_i;
input  [6:0] alu_inst_i;
input  [ 1:0] alu_sel_i;
input         alu_doublet_i;
input  [ 1:0] bs_sel_i, bs_selo_i;
input         l_na_i;
input         cssu_sel_i, is_cssu_i;
input         exp_sel_i, treg_sel_i;
input  [ 1:0] acca_sel_i, accb_sel_i;
input  [15:0] pb_i, cb_i, db_i;
input         bp_a_i, bp_b_i;
input         ovm_i, frct_i, smul_i, sxm_i;
input         c16_i, rnd_i, c_i, tc_i, asm_i, imm_i;
output        c_alu_o, c_bs_o, tc_cssu_o, tc_alu_o;
output        ovf_a_o, zf_a_o, ovf_b_o, zf_b_o;
output [15:0] trn_o, eb_o;

//
// variables
//

wire [39:0] acc_a, acc_b, bp_ar, bp_br;
wire [39:0] mac_result, alu_result, bs_result;
wire [15:0] treg;
wire [ 5:0] exp_result;

//
// module body
//

	//
	// instantiate MAC
	oc54_mac cpu_mac(
		.clk(clk_i),             // clock input
		.ena(ena_mac_i),         // MAC clock enable input
		.a(acc_a),               // accumulator A input
		.b(acc_b),               // accumulator B input
		.t(treg),                // TREG input
		.p(pb_i),                // Program Data Bus input
		.c(cb_i),                // Coefficient Data Bus input
		.d(db_i),                // Data Bus input
		.sel_xm(mac_sel_xm_i),   // select multiplier-X input
		.sel_ym(mac_sel_ym_i),   // select muliplier-Y input
		.sel_ya(mac_sel_ya_i),   // select adder-Y input
		.bp_a(bp_a_i),           // bypass accumulator A select input
		.bp_b(bp_b_i),           // bypass accumulator B select input
		.bp_ar(bp_ar),           // bypass accumulator A input
		.bp_br(bp_br),           // bypass accumulator B input
		.xm_s(mac_xm_sx_i),      // sign extend multiplier x-input
		.ym_s(mac_ym_sx_i),      // sign extend multiplier y-input
		.ovm(ovm_i),             // overflow mode input
		.frct(frct_i),           // fractional mode input
		.smul(smul_i),           // saturate on multiply input
		.add_sub(mac_add_sub_i), // add/subtract input
		.result(mac_result)      // MAC result output
	);

	//
	// instantiate ALU
	oc54_alu cpu_alu(
		.clk(clk_i),             // clock input
		.ena(ena_alu_i),         // ALU clock enable input
		.inst(alu_inst_i),       // ALU instruction
		.seli(alu_sel_i),        // ALU x-input select
		.doublet(alu_doublet_i), // double t {treg, treg}
		.a(acc_a),               // accumulator A input
		.b(acc_b),               // accumulator B input
		.s(bs_result),           // barrel shifter result input
		.t(treg),                // TREG input
		.cb(cb_i),               // Coefficient Data Bus input
		.bp_a(bp_a_i),           // bypass accumulator A select input
		.bp_b(bp_b_i),           // bypass accumulator B select input
		.bp_ar(bp_ar),           // bypass accumulator A input
		.bp_br(bp_br),           // bypass accumulator B input
		.c16(c16_i),             // c16 (double 16/long-word) input
		.sxm(sxm_i),             // sign extend mode input
		.ci(c_i),                // carry input
		.tci(tc_i),              // test/control flag input
		.co(c_alu_o),            // ALU carry output
		.tco(tc_alu_o),          // ALU test/control flag output
		.result(alu_result)      // ALU result output
	);

	//
	// instantiate barrel shifter
	oc54_bshft cpu_bs(
		.clk(clk_i),             // clock input
		.ena(ena_bs_i),          // BS clock enable input
		.a(acc_a),               // accumulator A input
		.b(acc_b),               // accumulator B input
		.cb(cb_i),               // Coefficient Data Bus input
		.db(db_i),               // Data Bus input
		.bp_a(bp_a_i),           // bypass accumulator A select input
		.bp_b(bp_b_i),           // bypass accumulator B select input
		.bp_ar(bp_ar),           // bypass accumulator A input
		.bp_br(bp_br),           // bypass accumulator B input
		.l_na(l_na_i),           // BS logical/arithmetic shift mode input
		.sxm(sxm_i),             // sign extend mode input
		.seli(bs_sel_i),         // BS operand select input
		.selo(bs_selo_i),        // BS operator select input
		.t(treg[5:0]),                // TREG input
		.asm(asm_i),             // Accumulator Shift Mode input
		.imm(imm_i),             // Opcode Immediate input
		.result(bs_result),       // BS result output
		.co(c_bs_o)             // BS carry output (1 cycle ahead)
	);

	// instantiate Compare Select Store Unit
	oc54_cssu cpu_cssu(
		.clk(clk_i),             // clock input
		.ena(ena_bs_i),          // BS/CSSU clock enable input
		.sel_acc(cssu_sel_i),    // CSSU accumulator select input
		.is_cssu(is_cssu_i),     // CSSU/NormalShift operation
		.a(acc_a),               // accumulator A input
		.b(acc_b),               // accumulator B input
		.s(bs_result),           // BarrelShifter result input
		.tco(tc_cssu_o),         // test/control flag output
		.trn(trn_o),             // Transition register output
		.result(eb_o)            // Result Data Bus output
	);

	//
	// instantiate Exponent Encoder
	oc54_exp cpu_exp_enc(
		.clk(clk_i),             // clock input
		.ena(ena_exp_i),         // Exponent Encoder clock enable input
		.sel_acc(exp_sel_i),     // ExpE. accumulator select input
		.a(acc_a),               // accumulator A input
		.b(acc_b),               // accumulator B input
		.bp_ar(bp_ar),           // bypass accumulator A input
		.bp_br(bp_br),           // bypass accumulator B input
		.bp_a(bp_a_i),           // bypass accumulator A select input
		.bp_b(bp_b_i),           // bypass accumulator B select input
		.result(exp_result)      // Exponent Encoder result output
	);
	
	//
	// instantiate Temporary Register
	oc54_treg cpu_treg(
		.clk(clk_i),             // clock input
		.ena(ena_treg_i),        // treg clock enable input
		.seli(treg_sel_i),       // treg select input
		.we(1'b1),               // treg write enable input
		.exp(exp_result),        // Exponent Encoder Result input
		.d(db_i),                // Data Bus input
		.result(treg)            // TREG
	);

	//
	// instantiate accumulators
	oc54_acc cpu_acc_a(
		.clk(clk_i),             // clock input
		.ena(ena_acca_i),        // accumulator A clock enable input
		.seli(acca_sel_i),       // accumulator A select input      
		.we(1'b1),               // write enable input
		.a(acc_a),               // accumulator A input
		.b(acc_b),               // accumulator B input
		.alu(alu_result),        // ALU result input
		.mac(mac_result),        // MAC result input
		.ovm(ovm_i),             // overflow mode input
		.rnd(rnd_i),             // mac-rounding input
		.zf(zf_a_o),             // accumulator A zero flag output
		.ovf(ovf_a_o),           // accumulator A overflow flag output
		.bp_result(bp_ar),       // bypass accumulator A output
		.result(acc_a)           // accumulator A output
	);
	
	oc54_acc cpu_acc_b(
		.clk(clk_i),             // clock input
		.ena(ena_accb_i),        // accumulator B clock enable input
		.seli(accb_sel_i),       // accumulator B select input      
		.we(1'b1),               // write enable input
		.a(acc_a),               // accumulator A input
		.b(acc_b),               // accumulator B input
		.alu(alu_result),        // ALU result input
		.mac(mac_result),        // MAC result input
		.ovm(ovm_i),             // overflow mode input
		.rnd(rnd_i),             // mac-rounding input
		.zf(zf_b_o),             // accumulator B zero flag output
		.ovf(ovf_b_o),           // accumulator B overflow flag output
		.bp_result(bp_br),       // bypass accumulator B output
		.result(acc_b)           // accumulator B output
	);
endmodule


/////////////////////////////////////////////////////////////////////
////                                                             ////
////  OpenCores54 DSP, accumulator                               ////
////                                                             ////
////  Author: Richard Herveille                                  ////
////          richard@asics.ws                                   ////
////          www.asics.ws                                       ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2002 Richard Herveille                        ////
////                    richard@asics.ws                         ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
////     THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY     ////
//// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   ////
//// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   ////
//// FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE AUTHOR      ////
//// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,         ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    ////
//// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE   ////
//// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR        ////
//// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF  ////
//// LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT  ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  ////
//// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         ////
//// POSSIBILITY OF SUCH DAMAGE.                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
																	 
//
// Xilinx Virtex-E WC: 96 CLB slices @ 51MHz
//
									 
//  CVS Log														     
//																     
//  $Id: oc54_acc.v,v 1.1.1.1 2002/04/10 09:34:39 rherveille Exp $														     
//																     
//  $Date: 2002/04/10 09:34:39 $														 
//  $Revision: 1.1.1.1 $													 
//  $Author: rherveille $													     
//  $Locker:  $													     
//  $State: Exp $														 
//																     
// Change History:												     
//               $Log: oc54_acc.v,v $
//               Revision 1.1.1.1  2002/04/10 09:34:39  rherveille
//               Initial relase OpenCores54x DSP (CPU)
//											 
																
module oc54_acc (
	clk, ena,
	seli, we,
	a, b, alu, mac,
	ovm, rnd,
	zf, ovf,
	bp_result, result 
	);

//
// parameters
//

//
// inputs & outputs
//
input         clk;
input         ena;
input  [ 1:0] seli;              // select input
input         we;                // write enable
input  [39:0] a, b, alu, mac;    // accumulators, alu, mac input
input         ovm, rnd;          // overflow mode, saturate, round
output        ovf, zf;           // carry out, overflow, zero, tc-out
output [39:0] result;            // accumulator register output
output [39:0] bp_result;         // accumulator register bypass output

reg        ovf, zf;
reg [39:0] result;

//
// variables
//
reg  [39: 0] sel_r, iresult; // select results, final result
wire         iovf;

//
// module body
//

//
// generate input selection
//

// input selection & MAC-rounding
always@(seli or a or b or alu or mac or rnd)
	case(seli) // synopsis full_case parallel_case
		2'b00: sel_r = a;
		2'b01: sel_r = b;
		2'b10: sel_r = alu;
		2'b11: sel_r = rnd ? (mac + 16'h8000) & 40'hffffff0000 : mac;
	endcase

// overflow detection
// no overflow when:
// 1) all guard bits are set (valid negative number)
// 2) no guard bits are set (valid positive number)
assign iovf = 1'b1;// !( sel_r[39:32] || (~sel_r[39:32]) );

// saturation
always@(iovf or ovm or sel_r)
	if (ovm & iovf)
		if (sel_r[39]) // negate overflow
			iresult = 40'hff80000000;
		else             // positive overflow
			iresult = 40'h007fffffff;
	else
			iresult = sel_r;

//
// generate registers
//

// generate bypass output
assign bp_result = iresult;

// result
always@(posedge clk)
	if (ena & we)
		result <= iresult;

// ovf, zf
always@(posedge clk)
	if (ena & we)
		begin
			ovf <= iovf;
			zf  <= ~|iresult;
		end

endmodule

/////////////////////////////////////////////////////////////////////
////                                                             ////
////  OpenCores54 DSP, ALU                                       ////
////                                                             ////
////  Author: Richard Herveille                                  ////
////          richard@asics.ws                                   ////
////          www.asics.ws                                       ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2002 Richard Herveille                        ////
////                    richard@asics.ws                         ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
////     THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY     ////
//// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   ////
//// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   ////
//// FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE AUTHOR      ////
//// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,         ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    ////
//// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE   ////
//// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR        ////
//// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF  ////
//// LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT  ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  ////
//// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         ////
//// POSSIBILITY OF SUCH DAMAGE.                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
						
//
// Xilinx Virtex-E WC: 617 CLB slices @ 58MHz
//
											 
//  CVS Log														     
//																     
//  $Id: oc54_alu.v,v 1.1.1.1 2002/04/10 09:34:40 rherveille Exp $														     
//																     
//  $Date: 2002/04/10 09:34:40 $														 
//  $Revision: 1.1.1.1 $													 
//  $Author: rherveille $													     
//  $Locker:  $													     
//  $State: Exp $														 
//																     
// Change History:												     
//               $Log: oc54_alu.v,v $
//               Revision 1.1.1.1  2002/04/10 09:34:40  rherveille
//               Initial relase OpenCores54x DSP (CPU)
//											 
																
////                                                             ////
////  OpenCores54 DSP, ALU defines                               ////
////                                                             ////
////  Author: Richard Herveille                                  ////
////          richard@asics.ws                                   ////
////          www.asics.ws                                       ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2002 Richard Herveille                        ////
////                    richard@asics.ws                         ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
////     THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY     ////
//// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   ////
//// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   ////
//// FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE AUTHOR      ////
//// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,         ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    ////
//// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE   ////
//// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR        ////
//// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF  ////
//// LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT  ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  ////
//// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         ////
//// POSSIBILITY OF SUCH DAMAGE.                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
																	 
//  CVS Log														     
//																     
//  $Id: oc54_alu_defines.v,v 1.1.1.1 2002/04/10 09:34:40 rherveille Exp $														     
//																     
//  $Date: 2002/04/10 09:34:40 $														 
//  $Revision: 1.1.1.1 $													 
//  $Author: rherveille $													     
//  $Locker:  $													     
//  $State: Exp $														 
//																     
// Change History:												     
//               $Log: oc54_alu_defines.v,v $
//               Revision 1.1.1.1  2002/04/10 09:34:40  rherveille
//               Initial relase OpenCores54x DSP (CPU)
//											 
																

//
// Encoding: bit[6] always zero
//           bit[5:4] instuction type
//           - 00: (dual) arithmetic
//           - 01: logical (incl. shift)
//           - 10: bit-test & compare
//           - 11: reserved
// Leonardo-Spectrum seems to like the additional zero bit[6] in
// the encoding scheme. It produces the smallest and fastest code 
// like this. (Why ???)
//

//
// arithmetic instructions
//







//
// dual arithmetic instructions
//






//
// logical instructions
//





//
// shift instructions
//





//
// bit test instructions
//







//
// condition codes for CMP-test
//






module oc54_alu (
	clk, ena, inst,
	seli, doublet,
	a, b, s, t, cb,
	bp_a, bp_b,	bp_ar, bp_br,
	c16, sxm, ci, tci, 
	co, tco,
	result
	);

//
// parameters
//

//
// inputs & outputs
//
input         clk;
input         ena;
input  [ 6:0] inst;              // instruction
input  [ 1:0] seli;              // input selection
input         doublet;           // {T, T}
input  [39:0] a, b, s;           // accumulators + shifter inputs
input  [15:0] t, cb;             // TREG + c-bus input
input  [39:0] bp_ar, bp_br;      // bypass a register, bypass b register
input         bp_a, bp_b;        // bypass select
input         c16, sxm;          // dual-mode, sign-extend-mode, 
input         ci, tci;           // carry in, tc-in
output        co, tco;           // carry out, overflow, zero, tc-out
output [39:0] result;

reg        co, tco;
reg [39:0] result;

//
// variables
//
reg [39:0] iresult;
reg        itco, ico;
reg [39:0] x;

wire [39:0] y;

reg dc16, dci, dtci;

reg [6:0] dinst;


//
// module body
//

//
// generate input selection, barrel-shifter has 1cycle delay
//
always@(posedge clk)
	if (ena)
		case(seli) // synopsis full_case parallel_case
			2'b00 : 
				if (doublet)
					x <= {8'b00000000, t, t}; // is this correct ??
				else
					x <= {sxm, t};
					// PAJ ? x <= { {39-15{sxm ? t[15] : 1'b0}}, t};
			2'b01 : x <= bp_a ? bp_ar : a;
			2'b10 : x <= bp_b ? bp_br : b;
			2'b11 : x <= cb; 
			// PAJ ? 2'b11 : x <= { {39-15{sxm ? cb[15] : 1'b0}}, cb}; 
		endcase

assign y = s; // second input from barrel-shifter

//
// delay control signals
//
always@(posedge clk)
	if (ena)
		begin
			dc16   <= c16;
			dci    <= ci;
			dtci   <= tci;
			dinst  <= inst;
		end

//
// generate ALU
//
always@(dinst or x or y or dc16 or dtci or dci)
begin

/*
	ico     = dci;
	itco    = dtci;
	iresult = x;
*/

	case(dinst) // synopsis full_case parallel_case
		//
		// Arithmetic instructions
		//
		7'b0000000 : // absolute value
			begin
				if (x[39])
					iresult = (~x) + 1'b1;
				else
					iresult = x;

					ico     = ~|(iresult);
			end

		7'b0000010 : // ALSO ADDC, ADDS. For ADD, ADDS ci = 1'b0;
			begin
				iresult = x + y + dci;
				ico     = iresult[32];
			end

		7'b0000100 : // ALSO MIN. MAX: x==accA, y==accB, MIN: x==accB, y ==accA
			begin
					ico     = (x > y);
					iresult = (x > y) ? x : y;
			end

		7'b0000001 :
			begin
					iresult = (~x) + 1'b1;
					ico     = ~|(iresult);
			end

		7'b0000011 : // ALSO SUBB, SUBS. For SUB, SUBS ci = 1'b1;
			begin
				iresult = x - y - ~dci;
				ico     = iresult[32];
			end

		7'b0000101 : // subtract conditional (for division)
			begin
				// FIXME: Is this correct ?? How does SUBC affect the carry bit ?
			//	iresult = x - y;
				ico     = iresult[32];

				if ( x > 0 )
//					iresult = (iresult << 1) + 1'b1;
					iresult = ({x[38:0], 1'b0}) + 1'b1;
				else
//					iresult = x << 1;
					iresult = {x[38:0], 1'b0};
			end
					

		//
		// Dual Precision Arithmetic instructions
		//
		7'b0001000 :
			if (dc16)  // dual add          result_hi/lo = x_hi/lo + y_hi/lo
				begin
					iresult[39:16] = x[31:16] + y[31:16];
					iresult[15: 0] = x[15: 0] + y[15: 0]; 
				end
			else      // 32bit add         result = x + y
					{ico, iresult} = x + y;
				
		7'b0001100 :
			if (dc16) // dual subtract     result_hi/lo = x_hi/lo - y_hi/lo
				begin
					iresult[39:16] = x[31:16] - y[31:16];
					iresult[15: 0] = x[15: 0] - y[15: 0]; 
				end
			else     // 32bit subtract    result = x - y
				begin
					iresult = x - y;
					ico     = iresult[32];
				end

		7'b0001101 : // ALSO DSUBT: make sure x = {T, T}
			if (dc16)	// dual reverse sub. result_hi/lo = y_hi/lo - x_hi/lo
				begin
					iresult[39:16] = y[31:16] - x[31:16];
					iresult[15: 0] = y[15: 0] - x[15: 0]; 
				end
			else     // 32bit reverse sub.result = y - x
				begin
					iresult = y - x;
					ico     = iresult[32];
				end

		7'b0001110 : // DSADT: make  sure x = {T, T}
			if (dc16)
				begin
						iresult[39:16] = y[31:16] - x[31:16];
						iresult[15: 0] = y[15: 0] + x[15: 0]; 
				end
			else
				begin
					iresult = y - x;
					ico     = iresult[32];
				end
	
		7'b0001001 : // DADST: make sure x = {T, T}
			if (dc16)
				begin
						iresult[39:16] = y[31:16] + x[31:16];
						iresult[15: 0] = y[15: 0] - x[15: 0]; 
				end
			else
				begin
					iresult = x + y;
					ico     = iresult[32];
				end
				
		//
		// logical instructions
		//
		7'b0010000 : // CMPL
					iresult = ~x;

		7'b0010001 :
					iresult = x & y;

		7'b0010010 :
					iresult = x | y;

		7'b0010011 :
					iresult = x ^ y;

		//
		// shift instructions
		//
		7'b0010100 :
			begin
					iresult[39:32] = 8'b00000000;
					iresult[31: 0] = {x[30:0], dci};
					ico            = x[31];
			end

		7'b0010101 :
			begin
					iresult[39:32] = 8'b00000000;
					iresult[31: 0] = {x[30:0], dtci};
					ico            = x[31];
			end

		7'b0010110 :
			begin
					iresult[39:32] = 8'b00000000;
					iresult[31: 0] = {dci, x[31:1]};
					ico            = x[0];
			end

		7'b0010111 :
			if (x[31] & x[30])
				begin
//					iresult = x << 1;
					iresult = {x[38:0], 1'b0};
					itco    = 1'b0;
				end
			else
				begin
					iresult = x;
					itco    = 1'b1;
				end

		//
		// bit test and compare instructions
		//
		7'b0100000 :
					itco = ~|( x[15:0] & y[15:0] );

		7'b0100001 : // BIT, BITT y=Smem, x=imm or T
					//itco = y[ ~x[3:0] ]; // maybe do ~x at a higher level ??
					itco = y[0]; // maybe do ~x at a higher level ??

		7'b0100100	: // ALSO CMPM. for CMPM [39:16]==0
					itco = ~|(x ^ y);
		7'b0100101 :
					itco = x < y;
		7'b0100110 :
					itco = x > y;
		7'b0100111 :
					itco = |(x ^ y);

		//
		// NOP
		//
		default :
			begin
				ico     = dci;
				itco    = dtci;
				iresult = x;
			end
	endcase				
end


//
// generate registers
//

// result
always@(posedge clk)
	if (ena)
		result <= iresult;

// tco, co
always@(posedge clk)
	if (ena)
		begin
			tco <= itco;
			co  <= ico;
		end

endmodule


/////////////////////////////////////////////////////////////////////
////                                                             ////
////  OpenCores54 DSP, ALU defines                               ////
////                                                             ////
////  Author: Richard Herveille                                  ////
////          richard@asics.ws                                   ////
////          www.asics.ws                                       ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2002 Richard Herveille                        ////
////                    richard@asics.ws                         ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
////     THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY     ////
//// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   ////
//// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   ////
//// FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE AUTHOR      ////
//// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,         ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    ////
//// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE   ////
//// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR        ////
//// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF  ////
//// LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT  ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  ////
//// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         ////
//// POSSIBILITY OF SUCH DAMAGE.                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
																	 
//  CVS Log														     
//																     
//  $Id: oc54_alu_defines.v,v 1.1.1.1 2002/04/10 09:34:40 rherveille Exp $														     
//																     
//  $Date: 2002/04/10 09:34:40 $														 
//  $Revision: 1.1.1.1 $													 
//  $Author: rherveille $													     
//  $Locker:  $													     
//  $State: Exp $														 
//																     
// Change History:												     
//               $Log: oc54_alu_defines.v,v $
//               Revision 1.1.1.1  2002/04/10 09:34:40  rherveille
//               Initial relase OpenCores54x DSP (CPU)
//											 
																

//
// Encoding: bit[6] always zero
//           bit[5:4] instuction type
//           - 00: (dual) arithmetic
//           - 01: logical (incl. shift)
//           - 10: bit-test & compare
//           - 11: reserved
// Leonardo-Spectrum seems to like the additional zero bit[6] in
// the encoding scheme. It produces the smallest and fastest code 
// like this. (Why ???)
//

//
// arithmetic instructions
//







//
// dual arithmetic instructions
//






//
// logical instructions
//





//
// shift instructions
//





//
// bit test instructions
//







//
// condition codes for CMP-test
//





/////////////////////////////////////////////////////////////////////
////                                                             ////
////  OpenCores54 DSP, Barrel Shifter                            ////
////                                                             ////
////  Author: Richard Herveille                                  ////
////          richard@asics.ws                                   ////
////          www.asics.ws                                       ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2002 Richard Herveille                        ////
////                    richard@asics.ws                         ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
////     THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY     ////
//// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   ////
//// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   ////
//// FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE AUTHOR      ////
//// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,         ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    ////
//// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE   ////
//// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR        ////
//// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF  ////
//// LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT  ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  ////
//// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         ////
//// POSSIBILITY OF SUCH DAMAGE.                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
					
//
// Xilinx Virtex-E WC: 348 CLB slices @ 68MHz
//
												 
//  CVS Log														     
//																     
//  $Id: oc54_bshft.v,v 1.1.1.1 2002/04/10 09:34:40 rherveille Exp $														     
//																     
//  $Date: 2002/04/10 09:34:40 $														 
//  $Revision: 1.1.1.1 $													 
//  $Author: rherveille $													     
//  $Locker:  $													     
//  $State: Exp $														 
//																     
// Change History:												     
//               $Log: oc54_bshft.v,v $
//               Revision 1.1.1.1  2002/04/10 09:34:40  rherveille
//               Initial relase OpenCores54x DSP (CPU)
//											 
																
module oc54_bshft (
	clk, ena, 
	a, b, cb, db,
	bp_a, bp_b, bp_ar, bp_br,
	l_na, sxm, seli, selo,
	t, asm, imm,
	result, co
	);

//
// parameters
//

//
// inputs & outputs
//
input         clk;
input         ena;
input  [39:0] a, b;           // accumulator
input  [15:0] cb, db;         // memory data inputs
input  [39:0] bp_ar, bp_br;   // bypass a register, bypass b register
input         bp_a, bp_b;     // bypass select
input         sxm;            // sign extend mode
input         l_na;           // logical/not arithmetic shift
input  [ 1:0] seli;           // select operand (input)
input  [ 1:0] selo;           // select operator
input  [ 5:0] t;              // TREG, 6lsbs
input  asm;            // asm bits
input  imm;            // 5bit immediate value
output [39:0] result;
output        co;             // carry out output

reg [39:0] result;
reg        co;

//
// variables
//

reg [ 5:0] shift_cnt;
reg [39:0] operand;

//
// module body
//


//
// generate shift count
//
always@(selo or t or asm or imm)
	case (selo) // synopsis full_case parallel_case
		2'b00: shift_cnt = t;
		2'b01: shift_cnt = {asm, asm, asm, asm, asm};
		2'b10: shift_cnt = {imm, imm, imm, imm, imm};
		2'b11: shift_cnt = {imm, imm, imm, imm, imm};
	endcase

//
// generate operand
//
always@(seli or bp_a or a or bp_ar or bp_b or b or bp_br or cb or db)
	case (seli) // synopsis full_case parallel_case
		2'b00 : operand = bp_b ? bp_br : b;
		2'b01 : operand = bp_a ? bp_ar : a;
		2'b10 : operand = db;       // 16bit operand databus
		2'b11 : operand = {cb, db}; // 32bit operand
	endcase

//
// generate shifter
//
always@(posedge clk)
	if (ena)
		if (l_na) // logical shift
			if (shift_cnt[5])
				begin
					result[39:32] <= 8'h0;
					//result[31: 0] <= operand[31:0] >> (~shift_cnt[4:0] +1'h1);
					result[31: 0] <= operand[31:0] >> 2;//(~shift_cnt[4:0] +1'h1);
					//co            <= operand[ ~shift_cnt[4:0] ];
					co            <= operand[0];
				end
			else if ( ~|shift_cnt[4:0] )
				begin
					result <= operand;
					co     <= 1'b0;
				end
			else
				begin
					result[39:32] <= 8'h0;
					//result[31: 0] <= operand[31:0] << shift_cnt[4:0];
					result[31: 0] <= operand[31:0] << 1;// shift_cnt[4:0];
					//co            <= operand[ 5'h1f - shift_cnt[4:0] ];
					co            <= operand[0];
				end
		else      // arithmetic shift
			if (shift_cnt[5])
				begin
					if (sxm)
						//result <= { {16{operand[39]}} ,operand} >> (~shift_cnt[4:0] +1'h1);
						result <= operand >> 4;// (~shift_cnt[4:0] +1'h1);
					else
						//result <= operand >> (~shift_cnt[4:0] +1'h1);
						result <= operand >> 3;// (~shift_cnt[4:0] +1'h1);
					//co     <= operand[ ~shift_cnt[4:0] ];
					co     <= operand[0];
				end
			else
				begin
					result <= operand << 5;// shift_cnt[4:0];
					//result <= operand << shift_cnt[4:0];
					//co     <= operand[ 6'h27 - shift_cnt[4:0] ];
					co     <= operand[0];
				end

endmodule


/////////////////////////////////////////////////////////////////////
////                                                             ////
////  OpenCores54 DSP, Compare Select and Store Unit (CSSU)      ////
////                                                             ////
////  Author: Richard Herveille                                  ////
////          richard@asics.ws                                   ////
////          www.asics.ws                                       ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2002 Richard Herveille                        ////
////                    richard@asics.ws                         ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
////     THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY     ////
//// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   ////
//// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   ////
//// FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE AUTHOR      ////
//// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,         ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    ////
//// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE   ////
//// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR        ////
//// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF  ////
//// LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT  ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  ////
//// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         ////
//// POSSIBILITY OF SUCH DAMAGE.                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
				
//
// NOTE: Read the pipeline information for the CMPS instruction
//

//
// Xilinx Virtex-E WC: 41 CLB slices @ 130MHz
//
									 
//  CVS Log														     
//																     
//  $Id: oc54_cssu.v,v 1.1.1.1 2002/04/10 09:34:41 rherveille Exp $														     
//																     
//  $Date: 2002/04/10 09:34:41 $														 
//  $Revision: 1.1.1.1 $													 
//  $Author: rherveille $													     
//  $Locker:  $													     
//  $State: Exp $														 
//																     
// Change History:												     
//               $Log: oc54_cssu.v,v $
//               Revision 1.1.1.1  2002/04/10 09:34:41  rherveille
//               Initial relase OpenCores54x DSP (CPU)
//											 
																
module oc54_cssu (
	clk, ena,
	sel_acc, is_cssu,
	a, b, s,
	tco,
	trn, result
	);

//
// parameters
//

//
// inputs & outputs
//
input         clk;
input         ena;
input         sel_acc;           // select input
input         is_cssu;           // is this a cssu operation ?
input  [39:0] a, b, s;           // accumulators, shifter input
output        tco;               // tc-out
output [15:0] trn, result;

reg        tco;
reg [15:0] trn, result;

//
// variables
//

wire [31:0] acc;      // selected accumulator
wire        acc_cmp;  // acc[31;16]>acc[15:0] ??
//
// module body
//

//
// generate input selection
//

// input selection
assign acc = sel_acc ? b[39:0] : a[39:0];

assign acc_cmp = acc[31:16] > acc[15:0];

// result
always@(posedge clk)
	if (ena)
	begin
		if (is_cssu)
			if (acc_cmp)
				result <= acc[31:16];
			else
				result <= acc[15:0];
		else
			result <= s[39:0];

		if (is_cssu)
			trn <= {trn[14:0], ~acc_cmp};

		tco <= ~acc_cmp;
	end
endmodule


/////////////////////////////////////////////////////////////////////
////                                                             ////
////  OpenCores54 DSP, Exponent Encoder                          ////
////                                                             ////
////  Author: Richard Herveille                                  ////
////          richard@asics.ws                                   ////
////          www.asics.ws                                       ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2002 Richard Herveille                        ////
////                    richard@asics.ws                         ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
////     THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY     ////
//// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   ////
//// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   ////
//// FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE AUTHOR      ////
//// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,         ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    ////
//// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE   ////
//// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR        ////
//// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF  ////
//// LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT  ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  ////
//// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         ////
//// POSSIBILITY OF SUCH DAMAGE.                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
								
//
// Xilinx Virtex-E WC: 116 CLB slices @ 74MHz, pass 4
//
									 
//  CVS Log														     
//																     
//  $Id: oc54_exp.v,v 1.1.1.1 2002/04/10 09:34:41 rherveille Exp $														     
//																     
//  $Date: 2002/04/10 09:34:41 $														 
//  $Revision: 1.1.1.1 $													 
//  $Author: rherveille $													     
//  $Locker:  $													     
//  $State: Exp $														 
//																     
// Change History:												     
//               $Log: oc54_exp.v,v $
//               Revision 1.1.1.1  2002/04/10 09:34:41  rherveille
//               Initial relase OpenCores54x DSP (CPU)
//											 
																
module oc54_exp (
	clk, ena,
	sel_acc,
	a, b,
	bp_ar, bp_br,
	bp_a, bp_b,
	result
	);

//
// parameters
//

//
// inputs & outputs
//
input         clk;
input         ena;
input         sel_acc;                  // select accumulator
input  [39:0] a, b;                     // accumulator inputs
input  [39:0] bp_ar, bp_br;             // bypass accumulator a / b
input         bp_a, bp_b;               // bypass selects
output [ 5:0] result;

reg [5:0] result;

//
// variables
//
reg [39:0] acc;

/////////////////
// module body //
/////////////////  34('1') - 1(sign bit) - 8

//
// generate input selection
//

// input selection
always@(posedge clk)
	if (ena)
		if (sel_acc)
			acc <= bp_b ? bp_br : b;
		else
			acc <= bp_a ? bp_ar : a;

//
// Generate exponent encoder
//
// The result of the exponent encoder is the number of leading
// redundant bits, except the sign-bit, -8. Exp is in the -8 to +31 range.

// 00_0110 -> 11_1001 -> 11_1010

always@(posedge clk)
	if (ena)
/*
		case (acc) // synopsis full_case parallel_case
			//
			// positive numbers
			//
			40'b01??_????_????_????_????_????_????_????_????_????:
				result <= 6'h38; // 1 (leading red. bit) - 1 (sign bit) - 8 = -8
			40'b001?_????_????_????_????_????_????_????_????_????:
				result <= 6'h39; // 2 (leading red. bit) - 1 (sign bit) - 8 = -7
			40'b0001_????_????_????_????_????_????_????_????_????:
				result <= 6'h3a; // 3 (leading red. bit) - 1 (sign bit) - 8 = -6
			40'b0000_1???_????_????_????_????_????_????_????_????:
				result <= #1 6'h3b; // 4 (leading red. bit) - 1 (sign bit) - 8 = -5

			40'b0000_01??_????_????_????_????_????_????_????_????:
				result <= #1 6'h3c; // 5 (leading red. bit) - 1 (sign bit) - 8 = -4
			40'b0000_001?_????_????_????_????_????_????_????_????:
				result <= #1 6'h3d; // 6 (leading red. bit) - 1 (sign bit) - 8 = -3
			40'b0000_0001_????_????_????_????_????_????_????_????:
				result <= #1 6'h3e; // 7 (leading red. bit) - 1 (sign bit) - 8 = -2
			40'b0000_0000_1???_????_????_????_????_????_????_????:
				result <= #1 6'h3f; // 8 (leading red. bit) - 1 (sign bit) - 8 = -1

			40'b0000_0000_01??_????_????_????_????_????_????_????:
				result <= #1 6'h00; // 9 (leading red. bit) - 1 (sign bit) - 8 = 0
			40'b0000_0000_001?_????_????_????_????_????_????_????:
				result <= #1 6'h01; //10 (leading red. bit) - 1 (sign bit) - 8 = 1
			40'b0000_0000_0001_????_????_????_????_????_????_????:
				result <= #1 6'h02; //11 (leading red. bit) - 1 (sign bit) - 8 = 2
			40'b0000_0000_0000_1???_????_????_????_????_????_????:
				result <= #1 6'h03; //12 (leading red. bit) - 1 (sign bit) - 8 = 3

			40'b0000_0000_0000_01??_????_????_????_????_????_????:
				result <= #1 6'h04; //13 (leading red. bit) - 1 (sign bit) - 8 = 4
			40'b0000_0000_0000_001?_????_????_????_????_????_????:
				result <= #1 6'h05; //14 (leading red. bit) - 1 (sign bit) - 8 = 5
			40'b0000_0000_0000_0001_????_????_????_????_????_????:
				result <= #1 6'h06; //15 (leading red. bit) - 1 (sign bit) - 8 = 6
			40'b0000_0000_0000_0000_1???_????_????_????_????_????:
				result <= #1 6'h07; //16 (leading red. bit) - 1 (sign bit) - 8 = 7

			40'b0000_0000_0000_0000_01??_????_????_????_????_????:
				result <= #1 6'h08; //17 (leading red. bit) - 1 (sign bit) - 8 = 8
			40'b0000_0000_0000_0000_001?_????_????_????_????_????:
				result <= #1 6'h09; //18 (leading red. bit) - 1 (sign bit) - 8 = 9
			40'b0000_0000_0000_0000_0001_????_????_????_????_????:
				result <= #1 6'h0a; //19 (leading red. bit) - 1 (sign bit) - 8 =10
			40'b0000_0000_0000_0000_0000_1???_????_????_????_????:
				result <= #1 6'h0b; //20 (leading red. bit) - 1 (sign bit) - 8 =11
	
			40'b0000_0000_0000_0000_0000_01??_????_????_????_????:
				result <= #1 6'h0c; //21 (leading red. bit) - 1 (sign bit) - 8 =12
			40'b0000_0000_0000_0000_0000_001?_????_????_????_????:
				result <= #1 6'h0d; //22 (leading red. bit) - 1 (sign bit) - 8 =13
			40'b0000_0000_0000_0000_0000_0001_????_????_????_????:
				result <= #1 6'h0e; //23 (leading red. bit) - 1 (sign bit) - 8 =14
			40'b0000_0000_0000_0000_0000_0000_1???_????_????_????:
				result <= #1 6'h0f; //24 (leading red. bit) - 1 (sign bit) - 8 =15

			40'b0000_0000_0000_0000_0000_0000_01??_????_????_????:
				result <= #1 6'h10; //25 (leading red. bit) - 1 (sign bit) - 8 =16
			40'b0000_0000_0000_0000_0000_0000_001?_????_????_????:
				result <= #1 6'h11; //26 (leading red. bit) - 1 (sign bit) - 8 =17
			40'b0000_0000_0000_0000_0000_0000_0001_????_????_????:
				result <= #1 6'h12; //27 (leading red. bit) - 1 (sign bit) - 8 =18
			40'b0000_0000_0000_0000_0000_0000_0000_1???_????_????:
				result <= #1 6'h13; //28 (leading red. bit) - 1 (sign bit) - 8 =19

			40'b0000_0000_0000_0000_0000_0000_0000_01??_????_????:
				result <= #1 6'h14; //29 (leading red. bit) - 1 (sign bit) - 8 =20
			40'b0000_0000_0000_0000_0000_0000_0000_001?_????_????:
				result <= #1 6'h15; //30 (leading red. bit) - 1 (sign bit) - 8 =21
			40'b0000_0000_0000_0000_0000_0000_0000_0001_????_????:
				result <= #1 6'h16; //31 (leading red. bit) - 1 (sign bit) - 8 =22
			40'b0000_0000_0000_0000_0000_0000_0000_0000_1???_????:
				result <= #1 6'h17; //32 (leading red. bit) - 1 (sign bit) - 8 =23

			40'b0000_0000_0000_0000_0000_0000_0000_0000_01??_????:
				result <= #1 6'h18; //33 (leading red. bit) - 1 (sign bit) - 8 =24
			40'b0000_0000_0000_0000_0000_0000_0000_0000_001?_????:
				result <= #1 6'h19; //34 (leading red. bit) - 1 (sign bit) - 8 =25
			40'b0000_0000_0000_0000_0000_0000_0000_0000_0001_????:
				result <= #1 6'h1a; //35 (leading red. bit) - 1 (sign bit) - 8 =26
			40'b0000_0000_0000_0000_0000_0000_0000_0000_0000_1???:
				result <= #1 6'h1b; //36 (leading red. bit) - 1 (sign bit) - 8 =27

			40'b0000_0000_0000_0000_0000_0000_0000_0000_0000_01??:
				result <= #1 6'h1c; //37 (leading red. bit) - 1 (sign bit) - 8 =28
			40'b0000_0000_0000_0000_0000_0000_0000_0000_0000_001?:
				result <= #1 6'h1d; //38 (leading red. bit) - 1 (sign bit) - 8 =29
			40'b0000_0000_0000_0000_0000_0000_0000_0000_0000_0001:
				result <= #1 6'h1e; //39 (leading red. bit) - 1 (sign bit) - 8 =30
			40'b0000_0000_0000_0000_0000_0000_0000_0000_0000_0000:
				result <= #1 6'h1f; //40 (leading red. bit) - 1 (sign bit) - 8 =31

			//
			// negative numbers
			//
			40'b10??_????_????_????_????_????_????_????_????_????:
				result <= #1 6'h38; // 1 (leading red. bit) - 1 (sign bit) - 8 = -8
			40'b110?_????_????_????_????_????_????_????_????_????:
				result <= #1 6'h39; // 2 (leading red. bit) - 1 (sign bit) - 8 = -7
			40'b1110_????_????_????_????_????_????_????_????_????:
				result <= #1 6'h3a; // 3 (leading red. bit) - 1 (sign bit) - 8 = -6
			40'b1111_0???_????_????_????_????_????_????_????_????:
				result <= #1 6'h3b; // 4 (leading red. bit) - 1 (sign bit) - 8 = -5

			40'b1111_10??_????_????_????_????_????_????_????_????:
				result <= #1 6'h3c; // 5 (leading red. bit) - 1 (sign bit) - 8 = -4
			40'b1111_110?_????_????_????_????_????_????_????_????:
				result <= #1 6'h3d; // 6 (leading red. bit) - 1 (sign bit) - 8 = -3
			40'b1111_1110_????_????_????_????_????_????_????_????:
				result <= #1 6'h3e; // 7 (leading red. bit) - 1 (sign bit) - 8 = -2
			40'b1111_1111_0???_????_????_????_????_????_????_????:
				result <= #1 6'h3f; // 8 (leading red. bit) - 1 (sign bit) - 8 = -1

			40'b1111_1111_10??_????_????_????_????_????_????_????:
				result <= #1 6'h00; // 9 (leading red. bit) - 1 (sign bit) - 8 = 0
			40'b1111_1111_110?_????_????_????_????_????_????_????:
				result <= #1 6'h01; //10 (leading red. bit) - 1 (sign bit) - 8 = 1
			40'b1111_1111_1110_????_????_????_????_????_????_????:
				result <= #1 6'h02; //11 (leading red. bit) - 1 (sign bit) - 8 = 2
			40'b1111_1111_1111_0???_????_????_????_????_????_????:
				result <= #1 6'h03; //12 (leading red. bit) - 1 (sign bit) - 8 = 3

			40'b1111_1111_1111_10??_????_????_????_????_????_????:
				result <= #1 6'h04; //13 (leading red. bit) - 1 (sign bit) - 8 = 4
			40'b1111_1111_1111_110?_????_????_????_????_????_????:
				result <= #1 6'h05; //14 (leading red. bit) - 1 (sign bit) - 8 = 5
			40'b1111_1111_1111_1110_????_????_????_????_????_????:
				result <= #1 6'h06; //15 (leading red. bit) - 1 (sign bit) - 8 = 6
			40'b1111_1111_1111_1111_0???_????_????_????_????_????:
				result <= #1 6'h07; //16 (leading red. bit) - 1 (sign bit) - 8 = 7

			40'b1111_1111_1111_1111_10??_????_????_????_????_????:
				result <= #1 6'h08; //17 (leading red. bit) - 1 (sign bit) - 8 = 8
			40'b1111_1111_1111_1111_110?_????_????_????_????_????:
				result <= #1 6'h09; //18 (leading red. bit) - 1 (sign bit) - 8 = 9
			40'b1111_1111_1111_1111_1110_????_????_????_????_????:
				result <= #1 6'h0a; //19 (leading red. bit) - 1 (sign bit) - 8 =10
			40'b1111_1111_1111_1111_1111_0???_????_????_????_????:
				result <= #1 6'h0b; //20 (leading red. bit) - 1 (sign bit) - 8 =11
	
			40'b1111_1111_1111_1111_1111_10??_????_????_????_????:
				result <= #1 6'h0c; //21 (leading red. bit) - 1 (sign bit) - 8 =12
			40'b1111_1111_1111_1111_1111_110?_????_????_????_????:
				result <= #1 6'h0d; //22 (leading red. bit) - 1 (sign bit) - 8 =13
			40'b1111_1111_1111_1111_1111_1110_????_????_????_????:
				result <= #1 6'h0e; //23 (leading red. bit) - 1 (sign bit) - 8 =14
			40'b1111_1111_1111_1111_1111_1111_0???_????_????_????:
				result <= #1 6'h0f; //24 (leading red. bit) - 1 (sign bit) - 8 =15

			40'b1111_1111_1111_1111_1111_1111_10??_????_????_????:
				result <= #1 6'h10; //25 (leading red. bit) - 1 (sign bit) - 8 =16
			40'b1111_1111_1111_1111_1111_1111_110?_????_????_????:
				result <= #1 6'h11; //26 (leading red. bit) - 1 (sign bit) - 8 =17
			40'b1111_1111_1111_1111_1111_1111_1110_????_????_????:
				result <= #1 6'h12; //27 (leading red. bit) - 1 (sign bit) - 8 =18
			40'b1111_1111_1111_1111_1111_1111_1111_0???_????_????:
				result <= #1 6'h13; //28 (leading red. bit) - 1 (sign bit) - 8 =19

			40'b1111_1111_1111_1111_1111_1111_1111_10??_????_????:
				result <= #1 6'h14; //29 (leading red. bit) - 1 (sign bit) - 8 =20
			40'b1111_1111_1111_1111_1111_1111_1111_110?_????_????:
				result <= #1 6'h15; //30 (leading red. bit) - 1 (sign bit) - 8 =21
			40'b1111_1111_1111_1111_1111_1111_1111_1110_????_????:
				result <= #1 6'h16; //31 (leading red. bit) - 1 (sign bit) - 8 =22
			40'b1111_1111_1111_1111_1111_1111_1111_1111_0???_????:
				result <= #1 6'h17; //32 (leading red. bit) - 1 (sign bit) - 8 =23

			40'b1111_1111_1111_1111_1111_1111_1111_1111_10??_????:
				result <= #1 6'h18; //33 (leading red. bit) - 1 (sign bit) - 8 =24
			40'b1111_1111_1111_1111_1111_1111_1111_1111_110?_????:
				result <= #1 6'h19; //34 (leading red. bit) - 1 (sign bit) - 8 =25
			40'b1111_1111_1111_1111_1111_1111_1111_1111_1110_????:
				result <= #1 6'h1a; //35 (leading red. bit) - 1 (sign bit) - 8 =26
			40'b1111_1111_1111_1111_1111_1111_1111_1111_1111_0???:
				result <= #1 6'h1b; //36 (leading red. bit) - 1 (sign bit) - 8 =27

			40'b1111_1111_1111_1111_1111_1111_1111_1111_1111_10??:
				result <= #1 6'h1c; //37 (leading red. bit) - 1 (sign bit) - 8 =28
			40'b1111_1111_1111_1111_1111_1111_1111_1111_1111_110?:
				result <= #1 6'h1d; //38 (leading red. bit) - 1 (sign bit) - 8 =29
			40'b1111_1111_1111_1111_1111_1111_1111_1111_1111_1110:
				result <= #1 6'h1e; //39 (leading red. bit) - 1 (sign bit) - 8 =30
			40'b1111_1111_1111_1111_1111_1111_1111_1111_1111_1111:
				result <= #1 6'h1f; //40 (leading red. bit) - 1 (sign bit) - 8 =31
		endcase
*/
		if (acc)
			result <= 6'h1f; //40 (leading red. bit) - 1 (sign bit) - 8 =31
		else
			result <= 6'h1e; //39 (leading red. bit) - 1 (sign bit) - 8 =30

endmodule

/////////////////////////////////////////////////////////////////////
////                                                             ////
////  OpenCores54 DSP, MAC                                       ////
////                                                             ////
////  Author: Richard Herveille                                  ////
////          richard@asics.ws                                   ////
////          www.asics.ws                                       ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2002 Richard Herveille                        ////
////                    richard@asics.ws                         ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
////     THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY     ////
//// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   ////
//// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   ////
//// FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE AUTHOR      ////
//// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,         ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    ////
//// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE   ////
//// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR        ////
//// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF  ////
//// LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT  ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  ////
//// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         ////
//// POSSIBILITY OF SUCH DAMAGE.                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
								
//
// Xilinx Virtex-E WC: 296 CLB slices @ 64MHz
//
									 
//  CVS Log														     
//																     
//  $Id: oc54_mac.v,v 1.1.1.1 2002/04/10 09:34:41 rherveille Exp $														     
//																     
//  $Date: 2002/04/10 09:34:41 $														 
//  $Revision: 1.1.1.1 $													 
//  $Author: rherveille $													     
//  $Locker:  $													     
//  $State: Exp $														 
//																     
// Change History:												     
//               $Log: oc54_mac.v,v $
//               Revision 1.1.1.1  2002/04/10 09:34:41  rherveille
//               Initial relase OpenCores54x DSP (CPU)
//											 
																
module oc54_mac (
	clk, ena, 
	a, b, t, p, c, d,
	sel_xm, sel_ym, sel_ya,
	bp_a, bp_b, bp_ar, bp_br,
	xm_s, ym_s,
	ovm, frct, smul, add_sub,
	result
	);

//
// parameters
//

//
// inputs & outputs
//
input         clk;
input         ena;
input  [15:0] t, p, c, d;               // TREG, p-bus, c-bus, d-bus inputs
input  [39:0] a, b;                     // accumulator inputs
input  [ 1:0] sel_xm, sel_ym, sel_ya;   // input selects
input  [39:0] bp_ar, bp_br;             // bypass accumulator a / b
input         bp_a, bp_b;               // bypass selects
input         xm_s, ym_s;               // sign extend xm, ym
input         ovm, frct, smul, add_sub;
output [39:0] result;

reg [39:0] result;

//
// variables
//
reg  [16:0] xm, ym;              // multiplier inputs
reg  [39:0] ya;                  // adder Y-input

reg  [33:0] mult_res;            // multiplier result
wire [33:0] imult_res;           // actual multiplier
reg  [39:0] iresult;             // mac-result

/////////////////
// module body //
/////////////////

//
// generate input selection
//
wire bit1;
assign bit1 = xm_s ? t[15] : 1'b0;
wire bit2;
assign bit2 = ym_s ? p[15] : 1'b0; 
// xm
always@(posedge clk)
begin
	if (ena)
		case(sel_xm) // synopsis full_case parallel_case
			2'b00 : xm <= {bit1, t};
			2'b01 : xm <= {bit1, d};
			2'b10 : xm <= bp_a ? bp_ar[32:16] : a[32:16];
			2'b11 : xm <= 17'h0;
		endcase
end

// ym
always@(posedge clk)
	if (ena)
		case(sel_ym) // synopsis full_case parallel_case
			2'b00 : ym <= {bit2, p};
			2'b01 : ym <= bp_a ? bp_ar[32:16] : a[32:16];
			2'b10 : ym <= {bit2, d};
			2'b11 : ym <= {bit2, c};
		endcase

// ya
always@(posedge clk)
	if (ena)
		case(sel_ya) // synopsis full_case parallel_case
			2'b00 : ya <= bp_a ? bp_ar : a;
			2'b01 : ya <= bp_b ? bp_br : b;
			default : ya <= 40'h0;
		endcase

//
// generate multiplier
//
assign imult_res = (xm * ym); // actual multiplier

always@(xm or ym or smul or ovm or frct or imult_res)
	if (smul && ovm && frct && (xm[15:0] == 16'h8000) && (ym[15:0] == 16'h8000) )
		mult_res = 34'h7ffffff;
	else if (frct)
		mult_res = {imult_res[32:0], 1'b0}; // (imult_res << 1)
	else
		mult_res = imult_res;

//
// generate mac-unit
//
always@(mult_res or ya or add_sub)
	if (add_sub)
		iresult = mult_res + ya;
	else
		iresult = mult_res - ya;

//
// generate registers
//

// result
always@(posedge clk)
	if (ena)
		result <= iresult;

endmodule


/////////////////////////////////////////////////////////////////////
////                                                             ////
////  OpenCores54 DSP, Temporary Register (TREG)                 ////
////                                                             ////
////  Author: Richard Herveille                                  ////
////          richard@asics.ws                                   ////
////          www.asics.ws                                       ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2002 Richard Herveille                        ////
////                    richard@asics.ws                         ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
////     THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY     ////
//// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   ////
//// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   ////
//// FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE AUTHOR      ////
//// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,         ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    ////
//// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE   ////
//// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR        ////
//// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF  ////
//// LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT  ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  ////
//// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         ////
//// POSSIBILITY OF SUCH DAMAGE.                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
				
//
// NOTE: Read the pipeline information for the CMPS instruction
//

//
// Xilinx Virtex-E WC: 41 CLB slices @ 130MHz
//
									 
//  CVS Log														     
//																     
//  $Id: oc54_treg.v,v 1.1.1.1 2002/04/10 09:34:41 rherveille Exp $														     
//																     
//  $Date: 2002/04/10 09:34:41 $														 
//  $Revision: 1.1.1.1 $													 
//  $Author: rherveille $													     
//  $Locker:  $													     
//  $State: Exp $														 
//																     
// Change History:												     
//               $Log: oc54_treg.v,v $
//               Revision 1.1.1.1  2002/04/10 09:34:41  rherveille
//               Initial relase OpenCores54x DSP (CPU)
//											 
																
module oc54_treg (
	clk, ena,
	seli, we, 
	exp, d,
	result
	);

//
// parameters
//

//
// inputs & outputs
//
input         clk;
input         ena;
input         seli;              // select input
input         we;                // store result
input  [5:0] exp;               // exponent encoder input
input  [15:0] d;                 // DB input
output [15:0] result;

reg [15:0] result;

//
// variables
//

//
// module body
//

//
// generate input selection
//

// result
always@(posedge clk)
	if (ena)
		if (we)
			result <= seli ? {10'h0, exp} : d;
endmodule
