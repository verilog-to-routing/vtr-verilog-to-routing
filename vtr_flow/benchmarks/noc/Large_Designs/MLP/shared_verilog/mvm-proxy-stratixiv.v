module mvm # (
	parameter DATAW = 512,
	parameter BYTEW = DATAW / 8,
	parameter METAW = 8,
	parameter USERW = 32,
	parameter LANES = DATAW / 32,
	parameter DPES  = DATAW / 32,
	parameter RFWIDTH = DATAW,
	parameter RFDEPTH = 512,
	parameter RFADDRW = $clog2(RFDEPTH),
	parameter INSTW = 2 * RFADDRW + 6, // {rf_raddr, accum_raddr, last, release, accum_en, reduce, jump, en}
	parameter INSTD = 512,
	parameter INSTADDRW = $clog2(INSTD),
	parameter INPTW = DATAW,
	parameter INPTD = 64,
	parameter ACCMW = DATAW,
	parameter ACCMD = 64,
	parameter OUPTW = DATAW,
	parameter OUPTD = 64,
	parameter DPE_DELAY = 9,
	parameter RF_DELAY = 2
)(
	input  clk,
	input  rst,
	// Rx interface
	input  rx_tvalid,
	input  [DATAW-1:0] rx_tdata,
	input  [BYTEW-1:0] rx_tstrb,
	input  [BYTEW-1:0] rx_tkeep,
	input  [METAW-1:0] rx_tid,
	input  [METAW-1:0] rx_tdest,
	input  [USERW-1:0] rx_tuser,
	input  rx_tlast,
	output rx_tready,	
	// Tx interface
	output tx_tvalid,
	output [DATAW-1:0] tx_tdata,
	output [BYTEW-1:0] tx_tstrb,
	output [BYTEW-1:0] tx_tkeep,
	output [METAW-1:0] tx_tid,
	output [METAW-1:0] tx_tdest,
	output [USERW-1:0] tx_tuser,
	output tx_tlast,
	input  tx_tready
);

// Hook up unused Rx signals to dummy registers to avoid being synthesized away
(*noprune*) reg [BYTEW-1:0] dummy_rx_tstrb;
(*noprune*) reg [BYTEW-1:0] dummy_rx_tkeep;
(*noprune*) reg [BYTEW-1:0] dummy_rx_tdest;
always @ (posedge clk) begin
	dummy_rx_tstrb <= rx_tstrb;
	dummy_rx_tkeep <= rx_tkeep;
	dummy_rx_tdest <= rx_tdest;
end

reg  [INSTADDRW-1:0] inst_waddr, inst_raddr;
reg  inst_wen;
reg  [INSTW-1:0] inst_wdata;
wire [INSTW-1:0] inst_rdata; 

wire input_fifo_empty, input_fifo_full;
reg  [INPTW-1:0] input_fifo_idata;
wire [INPTW-1:0] input_fifo_odata;
reg  input_fifo_push, input_fifo_pop;

wire reduction_fifo_empty, reduction_fifo_full;
reg  [ACCMW-1:0] reduction_fifo_idata;
wire [ACCMW-1:0] reduction_fifo_odata;
reg  reduction_fifo_push, reduction_fifo_pop;

wire output_fifo_empty, output_fifo_full, output_fifo_almost_full;
wire [OUPTW-1:0] output_fifo_odata;
reg  output_fifo_pop;

reg  [RFADDRW-1:0] rf_waddr;
reg  rf_wen [0:DPES-1];
reg  [RFWIDTH-1:0] rf_wdata;
wire [RFWIDTH-1:0] rf_rdata [0:DPES-1]; 

wire [RFADDRW-1:0] accum_mem_waddr;
wire [RFWIDTH-1:0] accum_mem_rdata;

wire [OUPTW-1:0] dpe_results;
wire [DPES-1:0] dpe_ovalid;

wire [RFADDRW-1:0] inst_rf_raddr, inst_accum_raddr;
wire inst_reduce, inst_accum_en, inst_release, inst_jump, inst_en, inst_last;

reg rxtready, txtvalid;
reg [INSTW-1:0] r_inst;
reg r_inst_valid, r_inst_accum_en, r_inst_release, r_inst_reduce;
reg [INPTW-1:0] r_input_operands;
reg [ACCMW-1:0] r_reduction_operands;

memory_block # (
	.DATAW(INSTW),
	.DEPTH(INSTD)
) instruction_fifo (
	.clk(clk),
	.rst(rst),
	.waddr(inst_waddr),
	.wen(inst_wen),
	.wdata(inst_wdata),
	.raddr(inst_raddr),
	.rdata(inst_rdata)
);

assign inst_rf_raddr = inst_rdata[23:15];
assign inst_accum_raddr = inst_rdata[14:6];
assign inst_last = inst_rdata[5];
assign inst_reduce = inst_rdata[2];
assign inst_accum_en = inst_rdata[3];
assign inst_release = inst_rdata[4];
assign inst_jump = inst_rdata[1];
assign inst_en = inst_rdata[0];

fifo_mvm # (
	.DATAW(INPTW),
	.DEPTH(INPTD)
) input_fifo (
	.clk(clk),
	.rst(rst),
	.push(input_fifo_push),
	.idata(input_fifo_idata),
	.pop(input_fifo_pop),
	.odata(input_fifo_odata),
	.empty(input_fifo_empty),
	.full(input_fifo_full)
);

fifo_mvm # (
	.DATAW(ACCMW),
	.DEPTH(ACCMD)
) reduction_fifo (
	.clk(clk),
	.rst(rst),
	.push(reduction_fifo_push),
	.idata(reduction_fifo_idata),
	.pop(reduction_fifo_pop),
	.odata(reduction_fifo_odata),
	.empty(reduction_fifo_empty),
	.full(reduction_fifo_full)
);

memory_block # (
	.DATAW(RFWIDTH),
	.DEPTH(RFDEPTH)
) accum_mem (
	.clk(clk),
	.rst(rst),
	.waddr(accum_mem_waddr),
	.wen(dpe_ovalid[0]),
	.wdata(dpe_results),
	.raddr(inst_accum_raddr),
	.rdata(accum_mem_rdata)
);
pipeline # (.DELAY(24), .WIDTH(RFADDRW)) paccum_addr ( .clk(clk), .rst(rst), .data_in(inst_accum_raddr), .data_out(accum_mem_waddr) );

genvar dpe_id;
generate 
for (dpe_id = 0; dpe_id < DPES; dpe_id = dpe_id + 1) begin: generate_mrf_dpe
	memory_block # (
		.DATAW(RFWIDTH),
		.DEPTH(RFDEPTH)
	) rf (
		.clk(clk),
		.rst(rst),
		.waddr(rf_waddr),
		.wen(rf_wen[dpe_id]),
		.wdata(rf_wdata),
		.raddr(inst_rf_raddr),
		.rdata(rf_rdata[dpe_id])
	);
	
	fp_dpe dpe (
		.clk(clk),
		.rst(rst),
		.ivalid(r_inst_valid && r_inst_release),
		.accum(r_inst_accum_en),
		.reduce(r_inst_reduce),
		.dataa(r_input_operands),
		.datab(rf_rdata[dpe_id]),
		.datac(r_reduction_operands[(dpe_id + 1) * 32 - 1: dpe_id * 32]),
		.datad(accum_mem_rdata[(dpe_id + 1) * 32 - 1: dpe_id * 32]),
		.result(dpe_results[(dpe_id + 1) * 32 - 1: dpe_id * 32]),
		.ovalid(dpe_ovalid[dpe_id])
	);
end
endgenerate

// Specify if ready to accept input
always @ (*) begin
	if (rx_tvalid && rx_tid == 0) begin
		rxtready <= 1'b1;
	end else if (rx_tvalid && rx_tid == 1) begin
		rxtready <= !reduction_fifo_full;
	end else if (rx_tvalid && rx_tid == 2) begin
		rxtready <= !input_fifo_full;
	end else if (rx_tvalid && rx_tid == 3) begin
		rxtready <= 1'b1;
	end else begin
		rxtready <= 1'b0;
	end
end

// Read from input interface and steer to destination mem/FIFO
integer i;
always @ (posedge clk) begin
	if (rx_tvalid && rx_tready) begin
		if (rx_tid == 0) begin
			inst_wdata <= rx_tdata[INSTW-1:0];
			inst_waddr <= rx_tuser[INSTADDRW-1:0];
			inst_wen <= 1'b1;
			reduction_fifo_push <= 1'b0;
			input_fifo_push <= 1'b0;
			for (i = 0; i < DPES; i = i + 1) rf_wen[i] <= 1'b0;
		end else if (rx_tid == 1) begin
			reduction_fifo_idata <= rx_tdata[ACCMW-1:0];
			inst_wen <= 1'b0;
			reduction_fifo_push <= 1'b1;
			input_fifo_push <= 1'b0;
			for (i = 0; i < DPES; i = i + 1) rf_wen[i] <= 1'b0;
		end else if (rx_tid == 2) begin
			input_fifo_idata <= rx_tdata[INPTW-1:0];
			input_fifo_push <= 1'b1;
			inst_wen <= 1'b0;
			reduction_fifo_push <= 1'b0;
			for (i = 0; i < DPES; i = i + 1) rf_wen[i] <= 1'b0;
		end else if (rx_tid == 3) begin
			for (i = 0; i < DPES; i = i + 1) rf_wen[i] <= rx_tuser[RFADDRW+i];
			rf_wdata <= rx_tdata[RFWIDTH-1:0];
			rf_waddr <= rx_tuser[RFADDRW-1:0];
			inst_wen <= 1'b0;
			reduction_fifo_push <= 1'b0;
			input_fifo_push <= 1'b0;
		end
	end else begin
		inst_wen <= 1'b0;
		reduction_fifo_push <= 1'b0;
		input_fifo_push <= 1'b0;
		for (i = 0; i < DPES; i = i + 1) rf_wen[i] <= 1'b0;
	end
end

// Issue instruction and advance instruction raddr, pop inputs
always @ (posedge clk) begin
	if (rst) begin
		r_inst_valid <= 1'b0;
		r_inst_reduce <= 1'b0;
		r_inst_accum_en <= 1'b0;
		r_inst_release <= 1'b0;
		r_inst <= 0;
		r_input_operands <= {(INPTW){1'b0}};
		r_reduction_operands <= {(INPTW){1'b0}};
	end else begin
		if (inst_en) begin
			// If not a jump instruction and involves reduction and there are inputs available
			if (!inst_jump && inst_reduce && !input_fifo_empty && !reduction_fifo_empty && !output_fifo_almost_full) begin
				inst_raddr <= inst_raddr + 1'b1;
				r_inst_valid <= 1'b1;
				input_fifo_pop <= inst_last;
				reduction_fifo_pop <= 1'b1;
			end
			// If not a jump instruction and there are inputs available
			else if (!inst_rdata[1] && !input_fifo_empty && !output_fifo_almost_full) begin
				inst_raddr <= inst_raddr + 1'b1;
				r_inst_valid <= 1'b1;
				input_fifo_pop <= inst_last;
				reduction_fifo_pop <= 1'b0;
			end 
			// If it is a jump instructions and only raddr needs to be modified
			else if (inst_rdata[1]) begin
				inst_raddr <= inst_rdata[23:15];
				r_inst_valid <= 1'b0;
				input_fifo_pop <= 1'b0;
				reduction_fifo_pop <= 1'b0;
			end else begin
				r_inst_valid <= 1'b0;
				input_fifo_pop <= 1'b0;
				reduction_fifo_pop <= 1'b0;
			end
		end else begin
			r_inst_valid <= 1'b0;
			input_fifo_pop <= 1'b0;
			reduction_fifo_pop <= 1'b0;
		end
	end
	r_inst_reduce <= inst_reduce;
	r_inst_accum_en <= inst_accum_en;
	r_inst_release <= inst_release;
	r_inst <= inst_rdata;
	r_input_operands <= input_fifo_odata;
	r_reduction_operands <= reduction_fifo_odata;
end

fifo_mvm # (
	.DATAW(OUPTW),
	.DEPTH(OUPTD),
	.ALMOST_FULL_DEPTH(OUPTD-24)
) output_fifo (
	.clk(clk),
	.rst(rst),
	.push(dpe_ovalid[0]),
	.idata(dpe_results),
	.pop(tx_tready && !output_fifo_empty),
	.odata(output_fifo_odata),
	.empty(output_fifo_empty),
	.full(output_fifo_full),
	.almost_full(output_fifo_almost_full)
);

assign rx_tready = rxtready;
assign tx_tvalid = !output_fifo_empty;
assign tx_tdata = output_fifo_odata;

// Hook up rest of Tx signals to dummy values to avoid optimizing them out
assign tx_tstrb = output_fifo_odata[BYTEW-1:0];
assign tx_tkeep = output_fifo_odata[2*BYTEW-1:BYTEW];
assign tx_tid = output_fifo_odata[OUPTW-1:OUPTW-METAW];
assign tx_tdest = output_fifo_odata[OUPTW-METAW-1:OUPTW-2*METAW];
assign tx_tuser = output_fifo_odata[OUPTW-2*METAW-1:OUPTW-2*METAW-USERW];
assign tx_tlast = output_fifo_odata[31];

endmodule

module fifo_mvm # (
	parameter DATAW = 64,
	parameter DEPTH = 128,
	parameter ADDRW = $clog2(DEPTH),
	parameter ALMOST_FULL_DEPTH = DEPTH
)(
	input  clk,
	input  rst,
	input  push,
	input  [DATAW-1:0] idata,
	input  pop,
	output [DATAW-1:0] odata,
	output empty,
	output full,
	output almost_full
);

reg [DATAW-1:0] mem [0:DEPTH-1];
reg [ADDRW-1:0] head_ptr, tail_ptr;
reg [ADDRW:0] remaining;

always @ (posedge clk) begin
	if (rst) begin
		head_ptr <= 0;
		tail_ptr <= 0;
		remaining <= DEPTH;
	end else begin
		if (!full && push) begin
			mem[tail_ptr] <= idata;
			tail_ptr <= tail_ptr + 1'b1;
		end
		
		if (!empty && pop)  begin
			head_ptr <= head_ptr + 1'b1;
		end
		
		if (!empty && pop && !full && push) begin
			remaining <= remaining;
		end else if (!empty && pop) begin
			remaining <= remaining + 1'b1;
		end else if (!full && push) begin
			remaining <= remaining - 1'b1;
		end else begin
			remaining <= remaining;
		end
	end
end

assign empty = (tail_ptr == head_ptr);
assign full = (tail_ptr + 1'b1 == head_ptr);
assign odata = mem[head_ptr];
assign almost_full = (remaining < (DEPTH - ALMOST_FULL_DEPTH));

endmodule

module memory_block # (
	parameter DATAW = 8,
	parameter DEPTH = 512,
	parameter ADDRW = $clog2(DEPTH)
)(
	input  clk,
	input  rst,
	input  [ADDRW-1:0] waddr,
	input  wen,
	input  [DATAW-1:0] wdata,
	input  [ADDRW-1:0] raddr,
	output [DATAW-1:0] rdata
);

reg [DATAW-1:0] mem [0:DEPTH-1];

reg [ADDRW-1:0] r_raddr, r_waddr;
reg [DATAW-1:0] r_wdata;
reg r_wen;

always @ (posedge clk) begin
	if (rst) begin
		r_raddr <= 0;
		r_waddr <= 0;
		r_wdata <= 0;
	end else begin
		r_raddr <= raddr;
		r_wen <= wen;
		r_waddr <= waddr;
		r_wdata <= wdata;
		
		if (r_wen) mem[r_waddr] <= r_wdata;
	end
end

assign rdata = mem[r_raddr];

endmodule

module pipeline # (
	parameter DELAY = 1,
	parameter WIDTH = 32
)(
	input  clk,
	input  rst,
	input  [WIDTH-1:0] data_in,
	output [WIDTH-1:0] data_out
);

reg [WIDTH-1:0] r_pipeline [0:DELAY-1]; 

integer i;
always @ (posedge clk) begin
	if (rst) begin
		for (i = 0; i < DELAY; i = i + 1) begin
			r_pipeline[i] <= 0;
		end
	end else begin
		r_pipeline[0] <= data_in;
		for (i = 1; i < DELAY; i = i + 1) begin
			r_pipeline[i] <= r_pipeline[i-1];
		end
	end
end

assign data_out = r_pipeline[DELAY-1];

endmodule

module fp_accum_atom  (
	input  ena,
	input  clk,
	input  rst,
	input  acc,
	input  [31:0] idata,
	output [31:0] result
);

wire [31:0] atom_result;

altmult_add	ALTMULT_ADD_component (
	.chainin (32'b0),
	.clock0 (clk),
	.dataa ({40'd0, idata}),
	.datab ({71'd0, acc}),
	.result (atom_result),
	.accum_sload (1'b0),
	.aclr0 (rst),
	.aclr1 (rst),
	.aclr2 (rst),
	.aclr3 (rst),
	.addnsub1 (1'b1),
	.addnsub1_round (1'b0),
	.addnsub3 (1'b1),
	.addnsub3_round (1'b0),
	.chainout_round (1'b0),
	.chainout_sat_overflow (),
	.chainout_saturate (1'b0),
	.clock1 (1'b1),
	.clock2 (1'b1),
	.clock3 (1'b1),
	.coefsel0 ({3{1'b0}}),
	.coefsel1 ({3{1'b0}}),
	.coefsel2 ({3{1'b0}}),
	.coefsel3 ({3{1'b0}}),
	.datac ({88{1'b0}}),
	.ena0 (ena),
	.ena1 (ena),
	.ena2 (ena),
	.ena3 (ena),
	.mult01_round (1'b0),
	.mult01_saturation (1'b0),
	.mult0_is_saturated (),
	.mult1_is_saturated (),
	.mult23_round (1'b0),
	.mult23_saturation (1'b0),
	.mult2_is_saturated (),
	.mult3_is_saturated (),
	.output_round (1'b0),
	.output_saturate (1'b0),
	.overflow (),
	.rotate (1'b0),
	.scanina ({18{1'b0}}),
	.scaninb ({18{1'b0}}),
	.scanouta (),
	.scanoutb (),
	.shift_right (1'b0),
	.signa (1'b0),
	.signb (1'b0),
	.sourcea ({4{1'b0}}),
	.sourceb ({4{1'b0}}),
	.zero_chainout (1'b0),
	.zero_loopback (1'b0));
defparam
	ALTMULT_ADD_component.accumulator = "NO",
	ALTMULT_ADD_component.addnsub_multiplier_aclr1 = "UNUSED",
	ALTMULT_ADD_component.addnsub_multiplier_aclr3 = "UNUSED",
	ALTMULT_ADD_component.addnsub_multiplier_pipeline_aclr1 = "UNUSED",
	ALTMULT_ADD_component.addnsub_multiplier_pipeline_aclr3 = "UNUSED",
	ALTMULT_ADD_component.addnsub_multiplier_pipeline_register1 = "CLOCK0",
	ALTMULT_ADD_component.addnsub_multiplier_pipeline_register3 = "CLOCK0",
	ALTMULT_ADD_component.addnsub_multiplier_register1 = "CLOCK0",
	ALTMULT_ADD_component.addnsub_multiplier_register3 = "CLOCK0",
	ALTMULT_ADD_component.chainout_aclr = "UNUSED",
	ALTMULT_ADD_component.chainout_adder = "NO",
	ALTMULT_ADD_component.chainout_register = "UNUSED",
	ALTMULT_ADD_component.dedicated_multiplier_circuitry = "YES",
	ALTMULT_ADD_component.input_aclr_a0 = "UNUSED",
	ALTMULT_ADD_component.input_aclr_a1 = "UNUSED",
	ALTMULT_ADD_component.input_aclr_a2 = "UNUSED",
	ALTMULT_ADD_component.input_aclr_a3 = "UNUSED",
	ALTMULT_ADD_component.input_aclr_b0 = "UNUSED",
	ALTMULT_ADD_component.input_aclr_b1 = "UNUSED",
	ALTMULT_ADD_component.input_aclr_b2 = "UNUSED",
	ALTMULT_ADD_component.input_aclr_b3 = "UNUSED",
	ALTMULT_ADD_component.input_register_a0 = "CLOCK0",
	ALTMULT_ADD_component.input_register_a1 = "CLOCK0",
	ALTMULT_ADD_component.input_register_a2 = "CLOCK0",
	ALTMULT_ADD_component.input_register_a3 = "CLOCK0",
	ALTMULT_ADD_component.input_register_b0 = "CLOCK0",
	ALTMULT_ADD_component.input_register_b1 = "CLOCK0",
	ALTMULT_ADD_component.input_register_b2 = "CLOCK0",
	ALTMULT_ADD_component.input_register_b3 = "CLOCK0",
	ALTMULT_ADD_component.input_source_a0 = "DATAA",
	ALTMULT_ADD_component.input_source_a1 = "DATAA",
	ALTMULT_ADD_component.input_source_a2 = "DATAA",
	ALTMULT_ADD_component.input_source_a3 = "DATAA",
	ALTMULT_ADD_component.input_source_b0 = "DATAB",
	ALTMULT_ADD_component.input_source_b1 = "DATAB",
	ALTMULT_ADD_component.input_source_b2 = "DATAB",
	ALTMULT_ADD_component.input_source_b3 = "DATAB",
	ALTMULT_ADD_component.intended_device_family = "Stratix IV",
	ALTMULT_ADD_component.lpm_type = "altmult_add",
	ALTMULT_ADD_component.multiplier1_direction = "ADD",
	ALTMULT_ADD_component.multiplier3_direction = "ADD",
	ALTMULT_ADD_component.multiplier_aclr0 = "UNUSED",
	ALTMULT_ADD_component.multiplier_aclr1 = "UNUSED",
	ALTMULT_ADD_component.multiplier_aclr2 = "UNUSED",
	ALTMULT_ADD_component.multiplier_aclr3 = "UNUSED",
	ALTMULT_ADD_component.multiplier_register0 = "CLOCK0",
	ALTMULT_ADD_component.multiplier_register1 = "CLOCK0",
	ALTMULT_ADD_component.multiplier_register2 = "CLOCK0",
	ALTMULT_ADD_component.multiplier_register3 = "CLOCK0",
	ALTMULT_ADD_component.number_of_multipliers = 4,
	ALTMULT_ADD_component.output_aclr = "UNUSED",
	ALTMULT_ADD_component.output_register = "CLOCK0",
	ALTMULT_ADD_component.port_addnsub1 = "PORT_UNUSED",
	ALTMULT_ADD_component.port_addnsub3 = "PORT_UNUSED",
	ALTMULT_ADD_component.port_signa = "PORT_UNUSED",
	ALTMULT_ADD_component.port_signb = "PORT_UNUSED",
	ALTMULT_ADD_component.representation_a = "SIGNED",
	ALTMULT_ADD_component.representation_b = "SIGNED",
	ALTMULT_ADD_component.signed_aclr_a = "UNUSED",
	ALTMULT_ADD_component.signed_aclr_b = "UNUSED",
	ALTMULT_ADD_component.signed_pipeline_aclr_a = "UNUSED",
	ALTMULT_ADD_component.signed_pipeline_aclr_b = "UNUSED",
	ALTMULT_ADD_component.signed_pipeline_register_a = "CLOCK0",
	ALTMULT_ADD_component.signed_pipeline_register_b = "CLOCK0",
	ALTMULT_ADD_component.signed_register_a = "CLOCK0",
	ALTMULT_ADD_component.signed_register_b = "CLOCK0",
	ALTMULT_ADD_component.width_a = 18,
	ALTMULT_ADD_component.width_b = 18,
	ALTMULT_ADD_component.width_chainin = 32,
	ALTMULT_ADD_component.width_result = 32,
	ALTMULT_ADD_component.zero_chainout_output_aclr = "UNUSED",
	ALTMULT_ADD_component.zero_chainout_output_register = "CLOCK0",
	ALTMULT_ADD_component.zero_loopback_aclr = "UNUSED",
	ALTMULT_ADD_component.zero_loopback_output_aclr = "UNUSED",
	ALTMULT_ADD_component.zero_loopback_output_register = "CLOCK0",
	ALTMULT_ADD_component.zero_loopback_pipeline_aclr = "UNUSED",
	ALTMULT_ADD_component.zero_loopback_pipeline_register = "CLOCK0",
	ALTMULT_ADD_component.zero_loopback_register = "CLOCK0";
	
assign result = atom_result;

endmodule

module fp_add_atom (
	input  ena,
	input  clk,
	input  rst,
	input  [31:0] dataa,
	input  [31:0] datab,
	output [31:0] result
);

wire [31:0] atom_result;

altmult_add	ALTMULT_ADD_component (
	.chainin (32'b0),
	.clock0 (clk),
	.dataa ({40'd0, dataa}),
	.datab ({40'd0, datab}),
	.result (atom_result),
	.accum_sload (1'b0),
	.aclr0 (rst),
	.aclr1 (rst),
	.aclr2 (rst),
	.aclr3 (rst),
	.addnsub1 (1'b1),
	.addnsub1_round (1'b0),
	.addnsub3 (1'b1),
	.addnsub3_round (1'b0),
	.chainout_round (1'b0),
	.chainout_sat_overflow (),
	.chainout_saturate (1'b0),
	.clock1 (1'b1),
	.clock2 (1'b1),
	.clock3 (1'b1),
	.coefsel0 ({3{1'b0}}),
	.coefsel1 ({3{1'b0}}),
	.coefsel2 ({3{1'b0}}),
	.coefsel3 ({3{1'b0}}),
	.datac ({88{1'b0}}),
	.ena0 (ena),
	.ena1 (ena),
	.ena2 (ena),
	.ena3 (ena),
	.mult01_round (1'b0),
	.mult01_saturation (1'b0),
	.mult0_is_saturated (),
	.mult1_is_saturated (),
	.mult23_round (1'b0),
	.mult23_saturation (1'b0),
	.mult2_is_saturated (),
	.mult3_is_saturated (),
	.output_round (1'b0),
	.output_saturate (1'b0),
	.overflow (),
	.rotate (1'b0),
	.scanina ({18{1'b0}}),
	.scaninb ({18{1'b0}}),
	.scanouta (),
	.scanoutb (),
	.shift_right (1'b0),
	.signa (1'b0),
	.signb (1'b0),
	.sourcea ({4{1'b0}}),
	.sourceb ({4{1'b0}}),
	.zero_chainout (1'b0),
	.zero_loopback (1'b0));
defparam
	ALTMULT_ADD_component.accumulator = "NO",
	ALTMULT_ADD_component.addnsub_multiplier_aclr1 = "UNUSED",
	ALTMULT_ADD_component.addnsub_multiplier_aclr3 = "UNUSED",
	ALTMULT_ADD_component.addnsub_multiplier_pipeline_aclr1 = "UNUSED",
	ALTMULT_ADD_component.addnsub_multiplier_pipeline_aclr3 = "UNUSED",
	ALTMULT_ADD_component.addnsub_multiplier_pipeline_register1 = "CLOCK0",
	ALTMULT_ADD_component.addnsub_multiplier_pipeline_register3 = "CLOCK0",
	ALTMULT_ADD_component.addnsub_multiplier_register1 = "CLOCK0",
	ALTMULT_ADD_component.addnsub_multiplier_register3 = "CLOCK0",
	ALTMULT_ADD_component.chainout_aclr = "UNUSED",
	ALTMULT_ADD_component.chainout_adder = "NO",
	ALTMULT_ADD_component.chainout_register = "UNUSED",
	ALTMULT_ADD_component.dedicated_multiplier_circuitry = "YES",
	ALTMULT_ADD_component.input_aclr_a0 = "UNUSED",
	ALTMULT_ADD_component.input_aclr_a1 = "UNUSED",
	ALTMULT_ADD_component.input_aclr_a2 = "UNUSED",
	ALTMULT_ADD_component.input_aclr_a3 = "UNUSED",
	ALTMULT_ADD_component.input_aclr_b0 = "UNUSED",
	ALTMULT_ADD_component.input_aclr_b1 = "UNUSED",
	ALTMULT_ADD_component.input_aclr_b2 = "UNUSED",
	ALTMULT_ADD_component.input_aclr_b3 = "UNUSED",
	ALTMULT_ADD_component.input_register_a0 = "CLOCK0",
	ALTMULT_ADD_component.input_register_a1 = "CLOCK0",
	ALTMULT_ADD_component.input_register_a2 = "CLOCK0",
	ALTMULT_ADD_component.input_register_a3 = "CLOCK0",
	ALTMULT_ADD_component.input_register_b0 = "CLOCK0",
	ALTMULT_ADD_component.input_register_b1 = "CLOCK0",
	ALTMULT_ADD_component.input_register_b2 = "CLOCK0",
	ALTMULT_ADD_component.input_register_b3 = "CLOCK0",
	ALTMULT_ADD_component.input_source_a0 = "DATAA",
	ALTMULT_ADD_component.input_source_a1 = "DATAA",
	ALTMULT_ADD_component.input_source_a2 = "DATAA",
	ALTMULT_ADD_component.input_source_a3 = "DATAA",
	ALTMULT_ADD_component.input_source_b0 = "DATAB",
	ALTMULT_ADD_component.input_source_b1 = "DATAB",
	ALTMULT_ADD_component.input_source_b2 = "DATAB",
	ALTMULT_ADD_component.input_source_b3 = "DATAB",
	ALTMULT_ADD_component.intended_device_family = "Stratix IV",
	ALTMULT_ADD_component.lpm_type = "altmult_add",
	ALTMULT_ADD_component.multiplier1_direction = "ADD",
	ALTMULT_ADD_component.multiplier3_direction = "ADD",
	ALTMULT_ADD_component.multiplier_aclr0 = "UNUSED",
	ALTMULT_ADD_component.multiplier_aclr1 = "UNUSED",
	ALTMULT_ADD_component.multiplier_aclr2 = "UNUSED",
	ALTMULT_ADD_component.multiplier_aclr3 = "UNUSED",
	ALTMULT_ADD_component.multiplier_register0 = "CLOCK0",
	ALTMULT_ADD_component.multiplier_register1 = "CLOCK0",
	ALTMULT_ADD_component.multiplier_register2 = "CLOCK0",
	ALTMULT_ADD_component.multiplier_register3 = "CLOCK0",
	ALTMULT_ADD_component.number_of_multipliers = 4,
	ALTMULT_ADD_component.output_aclr = "UNUSED",
	ALTMULT_ADD_component.output_register = "CLOCK0",
	ALTMULT_ADD_component.port_addnsub1 = "PORT_UNUSED",
	ALTMULT_ADD_component.port_addnsub3 = "PORT_UNUSED",
	ALTMULT_ADD_component.port_signa = "PORT_UNUSED",
	ALTMULT_ADD_component.port_signb = "PORT_UNUSED",
	ALTMULT_ADD_component.representation_a = "SIGNED",
	ALTMULT_ADD_component.representation_b = "SIGNED",
	ALTMULT_ADD_component.signed_aclr_a = "UNUSED",
	ALTMULT_ADD_component.signed_aclr_b = "UNUSED",
	ALTMULT_ADD_component.signed_pipeline_aclr_a = "UNUSED",
	ALTMULT_ADD_component.signed_pipeline_aclr_b = "UNUSED",
	ALTMULT_ADD_component.signed_pipeline_register_a = "CLOCK0",
	ALTMULT_ADD_component.signed_pipeline_register_b = "CLOCK0",
	ALTMULT_ADD_component.signed_register_a = "CLOCK0",
	ALTMULT_ADD_component.signed_register_b = "CLOCK0",
	ALTMULT_ADD_component.width_a = 18,
	ALTMULT_ADD_component.width_b = 18,
	ALTMULT_ADD_component.width_chainin = 32,
	ALTMULT_ADD_component.width_result = 32,
	ALTMULT_ADD_component.zero_chainout_output_aclr = "UNUSED",
	ALTMULT_ADD_component.zero_chainout_output_register = "CLOCK0",
	ALTMULT_ADD_component.zero_loopback_aclr = "UNUSED",
	ALTMULT_ADD_component.zero_loopback_output_aclr = "UNUSED",
	ALTMULT_ADD_component.zero_loopback_output_register = "CLOCK0",
	ALTMULT_ADD_component.zero_loopback_pipeline_aclr = "UNUSED",
	ALTMULT_ADD_component.zero_loopback_pipeline_register = "CLOCK0",
	ALTMULT_ADD_component.zero_loopback_register = "CLOCK0";

assign result = atom_result;

endmodule

module fp_dsp_atom # (
	parameter MODE = "sp_vector1",
	parameter CHAIN = "false"
)(
    input  ena,
    input  clk,
    input  rst,
    input  [31:0] ax,
    input  [31:0] ay,
    input  [31:0] az,
	 input  [43:0] chainin,
    output [31:0] result,
	 output [43:0] chainout
);

wire [43:0] atom_result;

altmult_add	ALTMULT_ADD_component (
	.chainin ((CHAIN)? chainin: 44'd0),
	.clock0 (clk),
	.dataa ({24'd0, az[15:0], ax}),
	.datab ({24'd0, az[31:16], ay}),
	.result (atom_result),
	.accum_sload (1'b0),
	.aclr0 (rst),
	.aclr1 (rst),
	.aclr2 (rst),
	.aclr3 (rst),
	.addnsub1 (1'b1),
	.addnsub1_round (1'b0),
	.addnsub3 (1'b1),
	.addnsub3_round (1'b0),
	.chainout_round (1'b0),
	.chainout_sat_overflow (),
	.chainout_saturate (1'b0),
	.clock1 (1'b1),
	.clock2 (1'b1),
	.clock3 (1'b1),
	.coefsel0 ({3{1'b0}}),
	.coefsel1 ({3{1'b0}}),
	.coefsel2 ({3{1'b0}}),
	.coefsel3 ({3{1'b0}}),
	.datac ({88{1'b0}}),
	.ena0 (ena),
	.ena1 (ena),
	.ena2 (ena),
	.ena3 (ena),
	.mult01_round (1'b0),
	.mult01_saturation (1'b0),
	.mult0_is_saturated (),
	.mult1_is_saturated (),
	.mult23_round (1'b0),
	.mult23_saturation (1'b0),
	.mult2_is_saturated (),
	.mult3_is_saturated (),
	.output_round (1'b0),
	.output_saturate (1'b0),
	.overflow (),
	.rotate (1'b0),
	.scanina ({18{1'b0}}),
	.scaninb ({18{1'b0}}),
	.scanouta (),
	.scanoutb (),
	.shift_right (1'b0),
	.signa (1'b0),
	.signb (1'b0),
	.sourcea ({4{1'b0}}),
	.sourceb ({4{1'b0}}),
	.zero_chainout (1'b0),
	.zero_loopback (1'b0));
defparam
	ALTMULT_ADD_component.accumulator = "NO",
	ALTMULT_ADD_component.addnsub_multiplier_aclr1 = "UNUSED",
	ALTMULT_ADD_component.addnsub_multiplier_aclr3 = "UNUSED",
	ALTMULT_ADD_component.addnsub_multiplier_pipeline_aclr1 = "UNUSED",
	ALTMULT_ADD_component.addnsub_multiplier_pipeline_aclr3 = "UNUSED",
	ALTMULT_ADD_component.addnsub_multiplier_pipeline_register1 = "CLOCK0",
	ALTMULT_ADD_component.addnsub_multiplier_pipeline_register3 = "CLOCK0",
	ALTMULT_ADD_component.addnsub_multiplier_register1 = "CLOCK0",
	ALTMULT_ADD_component.addnsub_multiplier_register3 = "CLOCK0",
	ALTMULT_ADD_component.chainout_aclr = "UNUSED",
	ALTMULT_ADD_component.chainout_adder = (CHAIN)? "YES": "NO",
	ALTMULT_ADD_component.chainout_register = (CHAIN)? "CLOCK0": "UNUSED",
	ALTMULT_ADD_component.dedicated_multiplier_circuitry = "YES",
	ALTMULT_ADD_component.input_aclr_a0 = "UNUSED",
	ALTMULT_ADD_component.input_aclr_a1 = "UNUSED",
	ALTMULT_ADD_component.input_aclr_a2 = "UNUSED",
	ALTMULT_ADD_component.input_aclr_a3 = "UNUSED",
	ALTMULT_ADD_component.input_aclr_b0 = "UNUSED",
	ALTMULT_ADD_component.input_aclr_b1 = "UNUSED",
	ALTMULT_ADD_component.input_aclr_b2 = "UNUSED",
	ALTMULT_ADD_component.input_aclr_b3 = "UNUSED",
	ALTMULT_ADD_component.input_register_a0 = "CLOCK0",
	ALTMULT_ADD_component.input_register_a1 = "CLOCK0",
	ALTMULT_ADD_component.input_register_a2 = "CLOCK0",
	ALTMULT_ADD_component.input_register_a3 = "CLOCK0",
	ALTMULT_ADD_component.input_register_b0 = "CLOCK0",
	ALTMULT_ADD_component.input_register_b1 = "CLOCK0",
	ALTMULT_ADD_component.input_register_b2 = "CLOCK0",
	ALTMULT_ADD_component.input_register_b3 = "CLOCK0",
	ALTMULT_ADD_component.input_source_a0 = "DATAA",
	ALTMULT_ADD_component.input_source_a1 = "DATAA",
	ALTMULT_ADD_component.input_source_a2 = "DATAA",
	ALTMULT_ADD_component.input_source_a3 = "DATAA",
	ALTMULT_ADD_component.input_source_b0 = "DATAB",
	ALTMULT_ADD_component.input_source_b1 = "DATAB",
	ALTMULT_ADD_component.input_source_b2 = "DATAB",
	ALTMULT_ADD_component.input_source_b3 = "DATAB",
	ALTMULT_ADD_component.intended_device_family = "Stratix IV",
	ALTMULT_ADD_component.lpm_type = "altmult_add",
	ALTMULT_ADD_component.multiplier1_direction = "ADD",
	ALTMULT_ADD_component.multiplier3_direction = "ADD",
	ALTMULT_ADD_component.multiplier_aclr0 = "UNUSED",
	ALTMULT_ADD_component.multiplier_aclr1 = "UNUSED",
	ALTMULT_ADD_component.multiplier_aclr2 = "UNUSED",
	ALTMULT_ADD_component.multiplier_aclr3 = "UNUSED",
	ALTMULT_ADD_component.multiplier_register0 = "CLOCK0",
	ALTMULT_ADD_component.multiplier_register1 = "CLOCK0",
	ALTMULT_ADD_component.multiplier_register2 = "CLOCK0",
	ALTMULT_ADD_component.multiplier_register3 = "CLOCK0",
	ALTMULT_ADD_component.number_of_multipliers = 4,
	ALTMULT_ADD_component.output_aclr = "UNUSED",
	ALTMULT_ADD_component.output_register = "CLOCK0",
	ALTMULT_ADD_component.port_addnsub1 = "PORT_UNUSED",
	ALTMULT_ADD_component.port_addnsub3 = "PORT_UNUSED",
	ALTMULT_ADD_component.port_signa = "PORT_UNUSED",
	ALTMULT_ADD_component.port_signb = "PORT_UNUSED",
	ALTMULT_ADD_component.representation_a = "SIGNED",
	ALTMULT_ADD_component.representation_b = "SIGNED",
	ALTMULT_ADD_component.signed_aclr_a = "UNUSED",
	ALTMULT_ADD_component.signed_aclr_b = "UNUSED",
	ALTMULT_ADD_component.signed_pipeline_aclr_a = "UNUSED",
	ALTMULT_ADD_component.signed_pipeline_aclr_b = "UNUSED",
	ALTMULT_ADD_component.signed_pipeline_register_a = "CLOCK0",
	ALTMULT_ADD_component.signed_pipeline_register_b = "CLOCK0",
	ALTMULT_ADD_component.signed_register_a = "CLOCK0",
	ALTMULT_ADD_component.signed_register_b = "CLOCK0",
	ALTMULT_ADD_component.width_a = 18,
	ALTMULT_ADD_component.width_b = 18,
	ALTMULT_ADD_component.width_chainin = 44,
	ALTMULT_ADD_component.width_result = 44,
	ALTMULT_ADD_component.zero_chainout_output_aclr = "UNUSED",
	ALTMULT_ADD_component.zero_chainout_output_register = "CLOCK0",
	ALTMULT_ADD_component.zero_loopback_aclr = "UNUSED",
	ALTMULT_ADD_component.zero_loopback_output_aclr = "UNUSED",
	ALTMULT_ADD_component.zero_loopback_output_register = "CLOCK0",
	ALTMULT_ADD_component.zero_loopback_pipeline_aclr = "UNUSED",
	ALTMULT_ADD_component.zero_loopback_pipeline_register = "CLOCK0",
	ALTMULT_ADD_component.zero_loopback_register = "CLOCK0";

assign result = atom_result[31:0];
assign chainout = atom_result;
	 
endmodule

module fp_dpe (
	 input  clk,
    input  rst,
	 input  ivalid,
	 input  accum,
	 input  reduce,
	 input  [511:0] dataa,
	 input  [511:0] datab,
	 input  [31:0] datac,
	 input  [31:0] datad,
    output [31:0] result,
	 output ovalid
);

wire [31:0] a [0:15];
wire [31:0] b [0:15];
wire [31:0] dpe_result;
wire [31:0] accum_result;

genvar i;
generate
for (i = 0; i < 16; i = i + 1) begin: gen_signals
	assign a[i] = dataa[(i + 1) * 32 - 1 : i * 32];
	assign b[i] = datab[(i + 1) * 32 - 1 : i * 32];
end
endgenerate

wire [43:0] chain_atom1_to_atom0, 
				chain_atom2_to_atom1, 
				chain_atom3_to_atom2, 
				chain_atom4_to_atom3, 
				chain_atom5_to_atom4, 
				chain_atom6_to_atom5, 
				chain_atom7_to_atom6,
				chain_atom8_to_atom7, 
				chain_atom9_to_atom8, 
				chain_atom10_to_atom9, 
				chain_atom11_to_atom10, 
				chain_atom12_to_atom11, 
				chain_atom13_to_atom12, 
				chain_atom14_to_atom13,
				chain_atom15_to_atom14;
wire [31:0] a0_delay, a2_delay, a4_delay, a6_delay, a8_delay, a10_delay, a12_delay, a14_delay;
wire [31:0] b0_delay, b2_delay, b4_delay, b6_delay, b8_delay, b10_delay, b12_delay, b14_delay;
wire [31:0] atom0_result, atom1_result, atom2_result, atom3_result, atom4_result, atom5_result, atom6_result,
				atom8_result, atom9_result, atom10_result, atom11_result, atom12_result, atom13_result, atom14_result;

fp_dsp_atom # (
	.MODE("sp_vector2"),
	.CHAIN("true")
) atom15 (
    .ena(1'b1),
    .clk(clk),
    .rst(rst),
    .ax(32'b0),
    .ay(a[15]),
    .az(b[15]),
	 .chainin(),
    .result(),
	 .chainout(chain_atom15_to_atom14)
);

pipeline # (.DELAY(1), .WIDTH(64)) p14 ( .clk(clk), .rst(rst), .data_in({a[14], b[14]}), .data_out({a14_delay, b14_delay}) );
fp_dsp_atom # (
	.MODE("sp_vector1"),
	.CHAIN("false")
) atom14 (
    .ena(1'b1),
    .clk(clk),
    .rst(rst),
    .ax(atom14_result),
    .ay(a14_delay),
    .az(b14_delay),
	 .chainin(chain_atom15_to_atom14),
    .result(atom14_result),
	 .chainout(chain_atom14_to_atom13)
);

fp_dsp_atom # (
	.MODE("sp_vector2"),
	.CHAIN("true")
) atom13 (
    .ena(1'b1),
    .clk(clk),
    .rst(rst),
    .ax(atom12_result),
    .ay(a[13]),
    .az(b[13]),
	 .chainin(chain_atom14_to_atom13),
    .result(atom13_result),
	 .chainout(chain_atom13_to_atom12)
);

pipeline # (.DELAY(1), .WIDTH(64)) p12 ( .clk(clk), .rst(rst), .data_in({a[12], b[12]}), .data_out({a12_delay, b12_delay}) );
fp_dsp_atom # (
	.MODE("sp_vector1"),
	.CHAIN("true")
) atom12 (
    .ena(1'b1),
    .clk(clk),
    .rst(rst),
    .ax(atom13_result),
    .ay(a12_delay),
    .az(b12_delay),
	 .chainin(chain_atom13_to_atom12),
    .result(atom12_result),
	 .chainout(chain_atom12_to_atom11)
);

fp_dsp_atom # (
	.MODE("sp_vector2"),
	.CHAIN("true")
) atom11 (
    .ena(1'b1),
    .clk(clk),
    .rst(rst),
    .ax(atom9_result),
    .ay(a[11]),
    .az(b[11]),
	 .chainin(chain_atom12_to_atom11),
    .result(atom11_result),
	 .chainout(chain_atom11_to_atom10)
);

pipeline # (.DELAY(1), .WIDTH(64)) p10 ( .clk(clk), .rst(rst), .data_in({a[10], b[10]}), .data_out({a10_delay, b10_delay}) );
fp_dsp_atom # (
	.MODE("sp_vector1"),
	.CHAIN("true")
) atom10 (
    .ena(1'b1),
    .clk(clk),
    .rst(rst),
    .ax(atom10_result),
    .ay(a10_delay),
    .az(b10_delay),
	 .chainin(chain_atom11_to_atom10),
    .result(atom10_result),
	 .chainout(chain_atom10_to_atom9)
);

fp_dsp_atom # (
	.MODE("sp_vector2"),
	.CHAIN("true")
) atom9 (
    .ena(1'b1),
    .clk(clk),
    .rst(rst),
    .ax(atom8_result),
    .ay(a[9]),
    .az(b[9]),
	 .chainin(chain_atom10_to_atom9),
    .result(atom9_result),
	 .chainout(chain_atom9_to_atom8)
);

pipeline # (.DELAY(1), .WIDTH(64)) p8 ( .clk(clk), .rst(rst), .data_in({a[8], b[8]}), .data_out({a8_delay, b8_delay}) );
fp_dsp_atom # (
	.MODE("sp_vector1"),
	.CHAIN("true")
) atom8 (
    .ena(1'b1),
    .clk(clk),
    .rst(rst),
    .ax(atom11_result),
    .ay(a8_delay),
    .az(b8_delay),
	 .chainin(chain_atom9_to_atom8),
    .result(atom8_result),
	 .chainout(chain_atom8_to_atom7)
);
				
fp_dsp_atom # (
	.MODE("sp_vector2"),
	.CHAIN("true")
) atom7 (
    .ena(1'b1),
    .clk(clk),
    .rst(rst),
    .ax(atom3_result),
    .ay(a[7]),
    .az(b[7]),
	 .chainin(chain_atom8_to_atom7),
    .result(dpe_result),
	 .chainout(chain_atom7_to_atom6)
);

pipeline # (.DELAY(1), .WIDTH(64)) p6 ( .clk(clk), .rst(rst), .data_in({a[6], b[6]}), .data_out({a6_delay, b6_delay}) );
fp_dsp_atom # (
	.MODE("sp_vector1"),
	.CHAIN("true")
) atom6 (
    .ena(1'b1),
    .clk(clk),
    .rst(rst),
    .ax(atom6_result),
    .ay(a6_delay),
    .az(b6_delay),
	 .chainin(chain_atom7_to_atom6),
    .result(atom6_result),
	 .chainout(chain_atom6_to_atom5)
);

fp_dsp_atom # (
	.MODE("sp_vector2"),
	.CHAIN("true")
) atom5 (
    .ena(1'b1),
    .clk(clk),
    .rst(rst),
    .ax(atom4_result),
    .ay(a[5]),
    .az(b[5]),
	 .chainin(chain_atom6_to_atom5),
    .result(atom5_result),
	 .chainout(chain_atom5_to_atom4)
);

pipeline # (.DELAY(1), .WIDTH(64)) p4 ( .clk(clk), .rst(rst), .data_in({a[4], b[4]}), .data_out({a4_delay, b4_delay}) );
fp_dsp_atom # (
	.MODE("sp_vector1"),
	.CHAIN("true")
) atom4 (
    .ena(1'b1),
    .clk(clk),
    .rst(rst),
    .ax(atom5_result),
    .ay(a4_delay),
    .az(b4_delay),
	 .chainin(chain_atom5_to_atom4),
    .result(atom4_result),
	 .chainout(chain_atom4_to_atom3)
);

fp_dsp_atom # (
	.MODE("sp_vector2"),
	.CHAIN("true")
) atom3 (
    .ena(1'b1),
    .clk(clk),
    .rst(rst),
    .ax(atom1_result),
    .ay(a[3]),
    .az(b[3]),
	 .chainin(chain_atom4_to_atom3),
    .result(atom3_result),
	 .chainout(chain_atom3_to_atom2)
);

pipeline # (.DELAY(1), .WIDTH(64)) p2 ( .clk(clk), .rst(rst), .data_in({a[2], b[2]}), .data_out({a2_delay, b2_delay}) );
fp_dsp_atom # (
	.MODE("sp_vector1"),
	.CHAIN("true")
) atom2 (
    .ena(1'b1),
    .clk(clk),
    .rst(rst),
    .ax(atom2_result),
    .ay(a2_delay),
    .az(b2_delay),
	 .chainin(chain_atom3_to_atom2),
    .result(atom2_result),
	 .chainout(chain_atom2_to_atom1)
);

fp_dsp_atom # (
	.MODE("sp_vector2"),
	.CHAIN("true")
) atom1 (
    .ena(1'b1),
    .clk(clk),
    .rst(rst),
    .ax(atom0_result),
    .ay(a[1]),
    .az(b[1]),
	 .chainin(chain_atom2_to_atom1),
    .result(atom1_result),
	 .chainout(chain_atom1_to_atom0)
);

pipeline # (.DELAY(1), .WIDTH(64)) p0 ( .clk(clk), .rst(rst), .data_in({a[0], b[0]}), .data_out({a0_delay, b0_delay}) );
fp_dsp_atom # (
	.MODE("sp_vector1"),
	.CHAIN("true")
) atom0 (
    .ena(1'b1),
    .clk(clk),
    .rst(rst),
    .ax(32'b0),
    .ay(a0_delay),
    .az(b0_delay),
	 .chainin(chain_atom1_to_atom0),
    .result(atom0_result),
	 .chainout()
);

wire accum_delay;
wire [31:0] accum_operand;
pipeline # (.DELAY(15), .WIDTH(1)) paccum ( .clk(clk), .rst(rst), .data_in(accum), .data_out(accum_delay) );
pipeline # (.DELAY(15), .WIDTH(32)) paccum_operand ( .clk(clk), .rst(rst), .data_in(datad & {(32){accum}}), .data_out(accum_operand) );
fp_add_atom accum_unit (
	.ena(1'b1),
	.clk(clk),
	.rst(rst),
	.dataa(dpe_result),
	.datab(accum_operand),
	.result(accum_result)
);

wire [31:0] reduce_operand;
pipeline # (.DELAY(18), .WIDTH(32)) preduce ( .clk(clk), .rst(rst), .data_in(datac & {(32){reduce}}), .data_out(reduce_operand) );
fp_add_atom reduce_unit (
	.ena(1'b1),
	.clk(clk),
	.rst(rst),
	.dataa(reduce_operand),
	.datab(accum_result),
	.result(result)
);

pipeline # (.DELAY(23), .WIDTH(1)) pvalid ( .clk(clk), .rst(rst), .data_in(ivalid), .data_out(ovalid) );

endmodule