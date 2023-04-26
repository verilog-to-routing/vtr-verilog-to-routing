module dispatcher # (
	parameter DATAW = 512,
	parameter BYTEW = DATAW / 8,
	parameter METAW = 8,
	parameter USERW = 32,
	parameter FIFO_DEPTH = 512
)(
	input  clk,
	input  rst,
	// Tx interface
	output tx_tvalid,
	output [DATAW-1:0] tx_tdata,
	output [BYTEW-1:0] tx_tstrb,
	output [BYTEW-1:0] tx_tkeep,
	output [METAW-1:0] tx_tid,
	output [METAW-1:0] tx_tdest,
	output [USERW-1:0] tx_tuser,
	output tx_tlast,
	input  tx_tready,
	// External FIFO IO
	input  [DATAW/8-1:0] ififo_wdata,
	input  ififo_wen,
	output ififo_rdy
);

wire fifo_full_signal, fifo_almost_full_signal, fifo_empty_signal;
wire [DATAW-1:0] fifo_rdata;

fifo_dispatcher # (
	.DATAW(DATAW),
	.DEPTH(FIFO_DEPTH)
) fifo_inst (
	.clk(clk),
	.rst(rst),
	.push(ififo_wen),
	.idata({(8){ififo_wdata}}),
	.pop(tx_tvalid && tx_tready),
	.odata(fifo_rdata),
	.empty(fifo_empty_signal),
	.full(fifo_full_signal),
	.almost_full(fifo_almost_full_signal)
);

assign tx_tvalid = !fifo_empty_signal;
assign tx_tstrb = fifo_rdata[2*BYTEW+2*METAW-1:BYTEW+2*METAW];
assign tx_tkeep = fifo_rdata[BYTEW+3*METAW-1:3*METAW];
assign tx_tid = fifo_rdata[2*METAW-1:METAW];
assign tx_tdest = fifo_rdata[METAW-1:0];
assign tx_tlast = fifo_rdata[DATAW-1];
assign tx_tdata = fifo_rdata;
assign tx_tuser = fifo_rdata[USERW-1:0];
assign ififo_rdy = !fifo_full_signal;

endmodule

module fifo_dispatcher # (
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