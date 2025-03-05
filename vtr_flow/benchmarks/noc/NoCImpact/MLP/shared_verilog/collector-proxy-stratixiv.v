module collector # (
	parameter DATAW = 512,
	parameter BYTEW = DATAW / 8,
	parameter METAW = 8,
	parameter USERW = 32,
	parameter FIFO_DEPTH = 512
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
	// External FIFO IO
	output [DATAW/8-1:0] ofifo_rdata,
	input  ofifo_ren,
	output ofifo_rdy
);

wire fifo_full_signal, fifo_almost_full_signal, fifo_empty_signal;
wire [DATAW-1:0] fifo_rdata;

fifo_collector # (
	.DATAW(DATAW),
	.DEPTH(FIFO_DEPTH)
) fifo_inst (
	.clk(clk),
	.rst(rst),
	.push(rx_tvalid && rx_tready),
	.idata(rx_tdata),
	.pop(ofifo_ren),
	.odata(fifo_rdata),
	.empty(fifo_empty_signal),
	.full(fifo_full_signal),
	.almost_full(fifo_almost_full_signal)
);

assign rx_tready = !fifo_empty_signal;
assign ofifo_rdy = !fifo_full_signal;

genvar i;
generate 
for (i = 0; i < DATAW/8; i = i + 1) begin: gen_rdata
	assign ofifo_rdata[i] = ^fifo_rdata[(i + 1) * 8 - 1: i * 8];
end
endgenerate

endmodule

module fifo_collector # (
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