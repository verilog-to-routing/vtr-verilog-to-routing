/* Simplified dispatcher used for FPT'23 */

module dispatcher (
	clk,
	rst,
	tx_tvalid,
	tx_tdata,
	tx_tstrb,
	tx_tkeep,
	tx_tid,
	tx_tdest,
	tx_tuser,
	tx_tlast,
	tx_tready,
	ififo_wdata,
	ififo_wen,
	ififo_rdy
);

input  clk;
input  rst;
// Tx interface
output tx_tvalid;
output [511:0] tx_tdata;
output [63:0] tx_tstrb;
output [63:0] tx_tkeep;
output [7:0] tx_tid;
output [7:0] tx_tdest;
output [31:0] tx_tuser;
output tx_tlast;
input  tx_tready;
// External FIFO IO
input  [63:0] ififo_wdata;
input  ififo_wen;
output ififo_rdy;

wire fifo_full_signal, fifo_almost_full_signal, fifo_empty_signal;
wire [511:0] fifo_rdata;
wire [511:0] fifo_wdata;
assign fifo_wdata[ 63:  0] = ififo_wdata;
assign fifo_wdata[127: 64] = ififo_wdata;
assign fifo_wdata[191:128] = ififo_wdata;
assign fifo_wdata[255:192] = ififo_wdata;
assign fifo_wdata[319:256] = ififo_wdata;
assign fifo_wdata[383:320] = ififo_wdata;
assign fifo_wdata[447:384] = ififo_wdata;
assign fifo_wdata[511:448] = ififo_wdata;

fifo_dispatcher fifo_inst (
	.clk(clk),
	.rst(rst),
	.push(ififo_wen),
	.idata(fifo_wdata),
	.pop(tx_tvalid && tx_tready),
	.odata(fifo_rdata),
	.empty(fifo_empty_signal),
	.full(fifo_full_signal)
);

reg [63:0] r_tx_tstrb;
reg [63:0] r_tx_tkeep;
reg [7:0] r_tx_tid;
reg [7:0] r_tx_tdest;
reg [31:0] r_tx_tuser;
reg r_tx_tlast;

always @ (posedge clk) begin
	if (rst) begin
		r_tx_tstrb <= 0;
		r_tx_tkeep <= 0;
		r_tx_tid   <= 0;
		r_tx_tdest <= 0;
		r_tx_tuser <= 0;
		r_tx_tlast <= 0;
	end else begin
		r_tx_tstrb <= ififo_wdata[63:0];
		r_tx_tkeep <= ififo_wdata[63:0];
		r_tx_tid   <= ififo_wdata[7:0] ^ ififo_wdata[15:8];
		r_tx_tdest <= ififo_wdata[7:0] ^ ififo_wdata[23:16];
		r_tx_tuser <= ififo_wdata[31:0] ^ ififo_wdata[63:32];
		r_tx_tlast <= ififo_wdata[63] ^ ififo_wdata[0];
	end
end

assign tx_tstrb = r_tx_tstrb;
assign tx_tkeep = r_tx_tkeep;
assign tx_tid   = r_tx_tid;
assign tx_tdest = r_tx_tdest;
assign tx_tuser = r_tx_tuser; 
assign tx_tlast = r_tx_tlast; 

assign tx_tvalid = !fifo_empty_signal;
assign tx_tdata = fifo_rdata;
assign ififo_rdy = !fifo_full_signal;

endmodule

module fifo_dispatcher (
  clk,
  rst,
  push,
  idata,
  pop,
  odata,
  empty,
  full
);

input  wire clk;
input  wire rst;
input  wire push;
input  wire [511:0] idata;
input  wire pop;
output wire [511:0] odata;
output reg empty;
output reg full;

reg [8:0] head_ptr;
reg [8:0] tail_ptr;

bram_inst b0  (.clk(clk), .wen(push), .wdata(idata[(32*1)-1:32*0]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*1)-1:32*0]));
bram_inst b1  (.clk(clk), .wen(push), .wdata(idata[(32*2)-1:32*1]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*2)-1:32*1]));
bram_inst b2  (.clk(clk), .wen(push), .wdata(idata[(32*3)-1:32*2]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*3)-1:32*2]));
bram_inst b3  (.clk(clk), .wen(push), .wdata(idata[(32*4)-1:32*3]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*4)-1:32*3]));
bram_inst b4  (.clk(clk), .wen(push), .wdata(idata[(32*5)-1:32*4]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*5)-1:32*4]));
bram_inst b5  (.clk(clk), .wen(push), .wdata(idata[(32*6)-1:32*5]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*6)-1:32*5]));
bram_inst b6  (.clk(clk), .wen(push), .wdata(idata[(32*7)-1:32*6]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*7)-1:32*6]));
bram_inst b7  (.clk(clk), .wen(push), .wdata(idata[(32*8)-1:32*7]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*8)-1:32*7]));
bram_inst b8  (.clk(clk), .wen(push), .wdata(idata[(32*9)-1:32*8]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*9)-1:32*8]));
bram_inst b9  (.clk(clk), .wen(push), .wdata(idata[(32*10)-1:32*9]),  .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*10)-1:32*9]));
bram_inst b10 (.clk(clk), .wen(push), .wdata(idata[(32*11)-1:32*10]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*11)-1:32*10]));
bram_inst b11 (.clk(clk), .wen(push), .wdata(idata[(32*12)-1:32*11]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*12)-1:32*11]));
bram_inst b12 (.clk(clk), .wen(push), .wdata(idata[(32*13)-1:32*12]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*13)-1:32*12]));
bram_inst b13 (.clk(clk), .wen(push), .wdata(idata[(32*14)-1:32*13]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*14)-1:32*13]));
bram_inst b14 (.clk(clk), .wen(push), .wdata(idata[(32*15)-1:32*14]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*15)-1:32*14]));
bram_inst b15 (.clk(clk), .wen(push), .wdata(idata[(32*16)-1:32*15]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*16)-1:32*15]));

always @ (posedge clk) begin
	if (rst) begin
		head_ptr <= 0;
		tail_ptr <= 0;
	end else begin
		if (push) tail_ptr <= tail_ptr + 1;		
		if (pop)  head_ptr <= head_ptr + 1;
	end
end

always @ (*) begin
  if (tail_ptr == head_ptr) begin
    empty = 1'b1;
  end else begin
    empty = 1'b0;
  end

  if (tail_ptr+1 == head_ptr) begin
    full = 1'b1;
  end else begin
    full = 1'b0;
  end
end

endmodule


module bram_inst(
	clk,
	wen,
	wdata,
	waddr,
	raddr,
	rdata
);

input  wire        clk;
input  wire        wen;
input  wire [31:0] wdata;
input  wire [ 8:0] waddr;
input  wire [ 8:0] raddr;
output wire [31:0] rdata;

wire [39:0] rtemp;
wire [39:0] wtemp;
wire [8:0] addrtemp;
assign rdata = rtemp[31:0];
assign wtemp = {8'd0, wdata};
assign addrtemp = waddr | raddr;

single_port_ram bram_instance(
	.clk(clk),
	.we(wen),
	.data(wtemp),
	.addr(addrtemp),
	.out(rtemp)
);

endmodule
