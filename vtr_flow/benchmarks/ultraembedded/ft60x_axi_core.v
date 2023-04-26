//-----------------------------------------------------------------
//          FTDI FT601 SuperSpeed USB3.0 to AXI bus master
//                              V0.1
//                        Ultra-Embedded.com
//                         Copyright 2020
//
//                 Email: admin@ultra-embedded.com
//
//                          License: GPL
//-----------------------------------------------------------------
//
// This source code is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This source code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this source code.  If not, see <https://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------

module ft60x_fifo
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           ftdi_rxf_i
    ,input           ftdi_txe_i
    ,input  [ 31:0]  ftdi_data_in_i
    ,input  [  3:0]  ftdi_be_in_i
    ,input           inport_valid_i
    ,input  [ 31:0]  inport_data_i
    ,input           outport_accept_i

    // Outputs
    ,output          ftdi_wrn_o
    ,output          ftdi_rdn_o
    ,output          ftdi_oen_o
    ,output [ 31:0]  ftdi_data_out_o
    ,output [  3:0]  ftdi_be_out_o
    ,output          inport_accept_o
    ,output          outport_valid_o
    ,output [ 31:0]  outport_data_o
);



//-----------------------------------------------------------------
// Rx RAM: FT60x -> Target
//-----------------------------------------------------------------
reg  [10:0] rx_wr_ptr_q;
wire [10:0] rx_wr_ptr_next_w = rx_wr_ptr_q + 11'd1;
reg  [10:0] rx_rd_ptr_q;
wire [10:0] rx_rd_ptr_next_w = rx_rd_ptr_q + 11'd1;

// Retime input data
reg  [31:0] rd_data_q;
reg         rd_valid_q;
wire        rx_valid_w;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    rd_valid_q  <= 1'b0;
else
    rd_valid_q  <= rx_valid_w;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    rd_data_q  <= 32'b0;
else
    rd_data_q  <= ftdi_data_in_i;

// Write pointer
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    rx_wr_ptr_q  <= 11'b0;
else if (rd_valid_q)
    rx_wr_ptr_q  <= rx_wr_ptr_next_w;

wire [31:0] rx_data_w;

ft60x_ram_dp
u_rx_ram
(
     .clk0_i(clk_i)
    ,.rst0_i(rst_i)
    ,.clk1_i(clk_i)
    ,.rst1_i(rst_i)

    // Write Port
    ,.wr0_i(rd_valid_q)
    ,.addr0_i(rx_wr_ptr_q)
    ,.data0_i(rd_data_q)
    ,.data0_o()

    // Read Port
    ,.wr1_i(1'b0)
    ,.addr1_i(rx_rd_ptr_q)
    ,.data1_i(32'b0)
    ,.data1_o(rx_data_w)
);

wire [11:0] rx_level_w = (rx_rd_ptr_q <= rx_wr_ptr_q) ? (rx_wr_ptr_q - rx_rd_ptr_q) : (12'd2048 - rx_rd_ptr_q) + rx_wr_ptr_q;
wire        rx_space_w = (12'd2048 - rx_level_w) > 12'd1024;

// Delayed write pointer
reg [10:0] rx_wr_ptr2_q;
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    rx_wr_ptr2_q  <= 11'b0;
else
    rx_wr_ptr2_q  <= rx_wr_ptr_q;

// Read Side
wire read_ok_w = (rx_wr_ptr2_q != rx_rd_ptr_q);
reg  rd_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    rd_q <= 1'b0;
else
    rd_q <= read_ok_w;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    rx_rd_ptr_q <= 11'b0;
// Read address increment
else if (read_ok_w && ((!outport_valid_o) || (outport_valid_o && outport_accept_i)))
    rx_rd_ptr_q <= rx_rd_ptr_next_w;

// Read Skid Buffer
reg        rd_skid_q;
reg [31:0] rd_skid_data_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
begin
    rd_skid_q      <= 1'b0;
    rd_skid_data_q <= 32'b0;
end
else if (outport_valid_o && !outport_accept_i)
begin
    rd_skid_q      <= 1'b1;
    rd_skid_data_q <= outport_data_o;
end
else
begin
    rd_skid_q      <= 1'b0;
    rd_skid_data_q <= 32'b0;
end

assign outport_valid_o = rd_skid_q | rd_q;
assign outport_data_o  = rd_skid_q ? rd_skid_data_q : rx_data_w;

//-----------------------------------------------------------------
// Tx RAM: Target -> FT60x
//-----------------------------------------------------------------
reg  [10:0] tx_wr_ptr_q;
wire [10:0] tx_wr_ptr_next_w = tx_wr_ptr_q + 11'd1;
reg  [10:0] tx_rd_ptr_q;
wire [10:0] tx_rd_ptr_next_w = tx_rd_ptr_q + 11'd1;
wire [10:0] tx_rd_ptr_prev_w = tx_rd_ptr_q - 11'd2;

wire [31:0] tx_data_w;

ft60x_ram_dp
u_tx_ram
(
     .clk0_i(clk_i)
    ,.rst0_i(rst_i)
    ,.clk1_i(clk_i)
    ,.rst1_i(rst_i)

    // Write Port
    ,.wr0_i(inport_valid_i & inport_accept_o)
    ,.addr0_i(tx_wr_ptr_q)
    ,.data0_i(inport_data_i)
    ,.data0_o()

    // Read Port
    ,.wr1_i(1'b0)
    ,.addr1_i(tx_rd_ptr_q)
    ,.data1_i(32'b0)
    ,.data1_o(tx_data_w)
);

// Write pointer
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    tx_wr_ptr_q  <= 11'b0;
else if (inport_valid_i & inport_accept_o)
    tx_wr_ptr_q  <= tx_wr_ptr_next_w;

// Delayed write pointer (pesamistic)
reg  [10:0] tx_wr_ptr2_q;
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    tx_wr_ptr2_q  <= 11'b0;
else
    tx_wr_ptr2_q  <= tx_wr_ptr_q;

// Write push timeout
localparam TX_BACKOFF_THRESH = 16'h00FF;

reg [15:0] tx_idle_cycles_q;
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    tx_idle_cycles_q  <= 16'b0;
else if (inport_valid_i)
    tx_idle_cycles_q  <= 16'b0;
else if (tx_idle_cycles_q != TX_BACKOFF_THRESH)
    tx_idle_cycles_q <= tx_idle_cycles_q + 16'd1;

wire tx_timeout_w = (tx_idle_cycles_q == TX_BACKOFF_THRESH);

wire [11:0] tx_level_w = (tx_rd_ptr_q <= tx_wr_ptr2_q) ? (tx_wr_ptr2_q - tx_rd_ptr_q) : (12'd2048 - tx_rd_ptr_q) + tx_wr_ptr2_q;
wire        tx_ready_w = (tx_level_w >= (1024 / 4)) || (tx_timeout_w && tx_level_w != 12'd0);

assign inport_accept_o = (tx_level_w < 12'd2000);

reg [11:0] tx_level_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    tx_level_q  <= 12'b0;
else
    tx_level_q  <= tx_level_w;

//-----------------------------------------------------------------
// Defines / Local params
//-----------------------------------------------------------------
localparam STATE_W           = 3;
localparam STATE_IDLE        = 3'd0;
localparam STATE_TX_START    = 3'd1;
localparam STATE_TX          = 3'd2;
localparam STATE_TURNAROUND  = 3'd3;
localparam STATE_RX_START    = 3'd4;
localparam STATE_RX          = 3'd5;

reg [STATE_W-1:0] state_q;

wire rx_ready_w = !ftdi_rxf_i;
wire tx_space_w = !ftdi_txe_i;

assign rx_valid_w = rx_ready_w & (state_q == STATE_RX);

//-----------------------------------------------------------------
// Next State Logic
//-----------------------------------------------------------------
reg [STATE_W-1:0] next_state_r;
reg [2:0]         state_count_q;
always @ *
begin
    next_state_r = state_q;

    case (state_q)
    //-----------------------------------------
    // STATE_IDLE
    //-----------------------------------------
    STATE_IDLE :
    begin
        if (rx_ready_w && rx_space_w)
            next_state_r = STATE_RX_START;
        else if (tx_space_w && tx_ready_w)
            next_state_r = STATE_TX_START; 
    end
    //-----------------------------------------
    // STATE_TX_START
    //-----------------------------------------
    STATE_TX_START :
    begin
        next_state_r  = STATE_TX;
    end
    //-----------------------------------------
    // STATE_TX
    //-----------------------------------------
    STATE_TX :
    begin
        if (!tx_space_w || tx_level_q == 12'd0)
            next_state_r  = STATE_TURNAROUND;
    end
    //-----------------------------------------
    // STATE_TURNAROUND
    //-----------------------------------------
    STATE_TURNAROUND :
    begin
        if (state_count_q == 3'b0)
            next_state_r  = STATE_IDLE;
    end
    //-----------------------------------------
    // STATE_RX_START
    //-----------------------------------------
    STATE_RX_START :
    begin
        next_state_r  = STATE_RX;
    end
    //-----------------------------------------
    // STATE_RX
    //-----------------------------------------
    STATE_RX :
    begin
        if (!rx_ready_w /*|| rx_full_next_w*/)
            next_state_r  = STATE_TURNAROUND;
    end
    default:
        ;
   endcase
end

// Update state
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    state_q   <= STATE_IDLE;
else
    state_q   <= next_state_r;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    state_count_q   <= 3'b0;
else if (state_q == STATE_TX && next_state_r == STATE_TURNAROUND)
    state_count_q   <= 3'd7;
else if (state_q == STATE_RX && next_state_r == STATE_TURNAROUND)
    state_count_q   <= 3'd7;
else if (state_count_q != 3'b0)
    state_count_q   <= state_count_q - 3'd1;

// Read pointer
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    tx_rd_ptr_q <= 11'b0;
else if (((state_q == STATE_IDLE && next_state_r == STATE_TX_START) || (state_q == STATE_TX_START) || (state_q == STATE_TX && tx_space_w)) && (tx_rd_ptr_q != tx_wr_ptr2_q) && (tx_level_q != 12'b0))
    tx_rd_ptr_q <= tx_rd_ptr_next_w;
else if ((state_q == STATE_TX) && !tx_space_w && tx_level_q == 12'b0)
    tx_rd_ptr_q <= tx_rd_ptr_q - 11'd1;
else if ((state_q == STATE_TX) && !tx_space_w)
    tx_rd_ptr_q <= tx_rd_ptr_prev_w;

//-----------------------------------------------------------------
// RD/WR/OE
//-----------------------------------------------------------------
// Xilinx placement pragmas:
//synthesis attribute IOB of rdn_q is "TRUE"
//synthesis attribute IOB of wrn_q is "TRUE"
//synthesis attribute IOB of oen_q is "TRUE"
//synthesis attribute IOB of data_q is "TRUE"

reg        rdn_q;
reg        wrn_q;
reg        oen_q;
reg [35:0] data_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    oen_q   <= 1'b1;
else if (state_q == STATE_IDLE && next_state_r == STATE_RX_START)
    oen_q   <= 1'b0;
else if (state_q == STATE_RX && next_state_r == STATE_TURNAROUND)
    oen_q   <= 1'b1;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    rdn_q   <= 1'b1;
else if (state_q == STATE_RX_START && next_state_r == STATE_RX)
    rdn_q   <= 1'b0;
else if (state_q == STATE_RX && next_state_r == STATE_TURNAROUND)
    rdn_q   <= 1'b1;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    wrn_q   <= 1'b1;
else if (state_q == STATE_TX_START && next_state_r == STATE_TX)
    wrn_q   <= 1'b0;
else if (state_q == STATE_TX && next_state_r == STATE_TURNAROUND)
    wrn_q   <= 1'b1;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    data_q  <= 36'b0;
else
    data_q  <= {4'b1111, tx_data_w};

assign ftdi_wrn_o      = wrn_q;
assign ftdi_rdn_o      = rdn_q;
assign ftdi_oen_o      = oen_q;
assign ftdi_data_out_o = data_q[31:0];
assign ftdi_be_out_o   = data_q[35:32];

endmodule


module ft60x_ram_dp
(
    // Inputs
     input           clk0_i
    ,input           rst0_i
    ,input  [ 10:0]  addr0_i
    ,input  [ 31:0]  data0_i
    ,input           wr0_i
    ,input           clk1_i
    ,input           rst1_i
    ,input  [ 10:0]  addr1_i
    ,input  [ 31:0]  data1_i
    ,input           wr1_i

    // Outputs
    ,output [ 31:0]  data0_o
    ,output [ 31:0]  data1_o
);

//-----------------------------------------------------------------
// Dual Port RAM 8KB
// Mode: Read First
//-----------------------------------------------------------------
/* verilator lint_off MULTIDRIVEN */
reg [31:0]   ram [2047:0] /*verilator public*/;
/* verilator lint_on MULTIDRIVEN */

reg [31:0] ram_read0_q;
reg [31:0] ram_read1_q;


// Synchronous write
always @ (posedge clk0_i)
begin
    if (wr0_i)
        ram[addr0_i][31:0] <= data0_i[31:0];

    ram_read0_q <= ram[addr0_i];
end

always @ (posedge clk1_i)
begin
    if (wr1_i)
        ram[addr1_i][31:0] <= data1_i[31:0];

    ram_read1_q <= ram[addr1_i];
end

assign data0_o = ram_read0_q;
assign data1_o = ram_read1_q;


endmodule
//-----------------------------------------------------------------
//          FTDI FT601 SuperSpeed USB3.0 to AXI bus master
//                              V0.1
//                        Ultra-Embedded.com
//                         Copyright 2020
//
//                 Email: admin@ultra-embedded.com
//
//                          License: GPL
//-----------------------------------------------------------------
//
// This source code is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This source code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this source code.  If not, see <https://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------

module ft60x_axi_retime
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           inport_awvalid_i
    ,input  [ 31:0]  inport_awaddr_i
    ,input  [  3:0]  inport_awid_i
    ,input  [  7:0]  inport_awlen_i
    ,input  [  1:0]  inport_awburst_i
    ,input           inport_wvalid_i
    ,input  [ 31:0]  inport_wdata_i
    ,input  [  3:0]  inport_wstrb_i
    ,input           inport_wlast_i
    ,input           inport_bready_i
    ,input           inport_arvalid_i
    ,input  [ 31:0]  inport_araddr_i
    ,input  [  3:0]  inport_arid_i
    ,input  [  7:0]  inport_arlen_i
    ,input  [  1:0]  inport_arburst_i
    ,input           inport_rready_i
    ,input           outport_awready_i
    ,input           outport_wready_i
    ,input           outport_bvalid_i
    ,input  [  1:0]  outport_bresp_i
    ,input  [  3:0]  outport_bid_i
    ,input           outport_arready_i
    ,input           outport_rvalid_i
    ,input  [ 31:0]  outport_rdata_i
    ,input  [  1:0]  outport_rresp_i
    ,input  [  3:0]  outport_rid_i
    ,input           outport_rlast_i

    // Outputs
    ,output          inport_awready_o
    ,output          inport_wready_o
    ,output          inport_bvalid_o
    ,output [  1:0]  inport_bresp_o
    ,output [  3:0]  inport_bid_o
    ,output          inport_arready_o
    ,output          inport_rvalid_o
    ,output [ 31:0]  inport_rdata_o
    ,output [  1:0]  inport_rresp_o
    ,output [  3:0]  inport_rid_o
    ,output          inport_rlast_o
    ,output          outport_awvalid_o
    ,output [ 31:0]  outport_awaddr_o
    ,output [  3:0]  outport_awid_o
    ,output [  7:0]  outport_awlen_o
    ,output [  1:0]  outport_awburst_o
    ,output          outport_wvalid_o
    ,output [ 31:0]  outport_wdata_o
    ,output [  3:0]  outport_wstrb_o
    ,output          outport_wlast_o
    ,output          outport_bready_o
    ,output          outport_arvalid_o
    ,output [ 31:0]  outport_araddr_o
    ,output [  3:0]  outport_arid_o
    ,output [  7:0]  outport_arlen_o
    ,output [  1:0]  outport_arburst_o
    ,output          outport_rready_o
);




//-----------------------------------------------------------------
// Write Command Request
//-----------------------------------------------------------------
wire [45:0] write_cmd_req_out_w;

axi4retime_fifo2x46
u_write_cmd_req
(
    .clk_i(clk_i),
    .rst_i(rst_i),

    .push_i(inport_awvalid_i),
    .data_in_i({inport_awaddr_i, inport_awid_i, inport_awlen_i, inport_awburst_i}),
    .accept_o(inport_awready_o),

    .valid_o(outport_awvalid_o),
    .data_out_o(write_cmd_req_out_w),
    .pop_i(outport_awready_i)
);

assign {outport_awaddr_o, outport_awid_o, outport_awlen_o, outport_awburst_o} = write_cmd_req_out_w;

//-----------------------------------------------------------------
// Write Data Request
//-----------------------------------------------------------------
wire [36:0] write_data_req_out_w;

axi4retime_fifo2x37
u_write_data_req
(
    .clk_i(clk_i),
    .rst_i(rst_i),

    .push_i(inport_wvalid_i),
    .data_in_i({inport_wlast_i, inport_wstrb_i, inport_wdata_i}),
    .accept_o(inport_wready_o),

    .valid_o(outport_wvalid_o),
    .data_out_o(write_data_req_out_w),
    .pop_i(outport_wready_i)
);

assign {outport_wlast_o, outport_wstrb_o, outport_wdata_o} = write_data_req_out_w;

//-----------------------------------------------------------------
// Write Response
//-----------------------------------------------------------------
wire [5:0] write_resp_out_w;

axi4retime_fifo2x6
u_write_resp
(
    .clk_i(clk_i),
    .rst_i(rst_i),

    .push_i(outport_bvalid_i),
    .data_in_i({outport_bresp_i, outport_bid_i}),
    .accept_o(outport_bready_o),

    .valid_o(inport_bvalid_o),
    .data_out_o(write_resp_out_w),
    .pop_i(inport_bready_i)
);

assign {inport_bresp_o, inport_bid_o} = write_resp_out_w;

//-----------------------------------------------------------------
// Read Request
//-----------------------------------------------------------------
wire [45:0] read_req_out_w;

axi4retime_fifo2x46
u_read_req
(
    .clk_i(clk_i),
    .rst_i(rst_i),

    .push_i(inport_arvalid_i),
    .data_in_i({inport_araddr_i, inport_arid_i, inport_arlen_i, inport_arburst_i}),
    .accept_o(inport_arready_o),

    .valid_o(outport_arvalid_o),
    .data_out_o(read_req_out_w),
    .pop_i(outport_arready_i)
);

assign {outport_araddr_o, outport_arid_o, outport_arlen_o, outport_arburst_o} = read_req_out_w;

//-----------------------------------------------------------------
// Read Response
//-----------------------------------------------------------------
wire [38:0] read_resp_out_w;

axi4retime_fifo2x39
u_read_resp
(
    .clk_i(clk_i),
    .rst_i(rst_i),

    .push_i(outport_rvalid_i),
    .data_in_i({outport_rdata_i, outport_rresp_i, outport_rid_i, outport_rlast_i}),
    .accept_o(outport_rready_o),

    .valid_o(inport_rvalid_o),
    .data_out_o(read_resp_out_w),
    .pop_i(inport_rready_i)
);

assign {inport_rdata_o, inport_rresp_o, inport_rid_o, inport_rlast_o} = read_resp_out_w;

endmodule

//-----------------------------------------------------------------
//                       FIFO modules
//-----------------------------------------------------------------

module axi4retime_fifo2x37
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input  [ 36:0]  data_in_i
    ,input           push_i
    ,input           pop_i

    // Outputs
    ,output [ 36:0]  data_out_o
    ,output          accept_o
    ,output          valid_o
);



//-----------------------------------------------------------------
// Registers
//-----------------------------------------------------------------
reg [36:0]    ram_q[1:0];
reg [0:0]   rd_ptr_q;
reg [0:0]   wr_ptr_q;
reg [1:0]  count_q;

//-----------------------------------------------------------------
// Sequential
//-----------------------------------------------------------------
always @ (posedge clk_i or posedge rst_i)
begin
    if (rst_i == 1'b1)
    begin
        count_q   <= 2'b0;
        rd_ptr_q  <= 1'b0;
        wr_ptr_q  <= 1'b0;
    end
    else
    begin
        // Push
        if (push_i & accept_o)
        begin
            ram_q[wr_ptr_q] <= data_in_i;
            wr_ptr_q        <= wr_ptr_q + 1'd1;
        end

        // Pop
        if (pop_i & valid_o)
        begin
            rd_ptr_q      <= rd_ptr_q + 1'd1;
        end

        // Count up
        if ((push_i & accept_o) & ~(pop_i & valid_o))
        begin
            count_q <= count_q + 2'd1;
        end
        // Count down
        else if (~(push_i & accept_o) & (pop_i & valid_o))
        begin
            count_q <= count_q - 2'd1;
        end
    end
end

//-------------------------------------------------------------------
// Combinatorial
//-------------------------------------------------------------------
assign valid_o       = (count_q != 2'd0);
assign accept_o      = (count_q != 2'd2);




assign data_out_o    = ram_q[rd_ptr_q];



endmodule

module axi4retime_fifo2x39
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input  [ 38:0]  data_in_i
    ,input           push_i
    ,input           pop_i

    // Outputs
    ,output [ 38:0]  data_out_o
    ,output          accept_o
    ,output          valid_o
);



//-----------------------------------------------------------------
// Registers
//-----------------------------------------------------------------
reg [38:0]    ram_q[1:0];
reg [0:0]   rd_ptr_q;
reg [0:0]   wr_ptr_q;
reg [1:0]  count_q;

//-----------------------------------------------------------------
// Sequential
//-----------------------------------------------------------------
always @ (posedge clk_i or posedge rst_i)
begin
    if (rst_i == 1'b1)
    begin
        count_q   <= 2'b0;
        rd_ptr_q  <= 1'b0;
        wr_ptr_q  <= 1'b0;
    end
    else
    begin
        // Push
        if (push_i & accept_o)
        begin
            ram_q[wr_ptr_q] <= data_in_i;
            wr_ptr_q        <= wr_ptr_q + 1'd1;
        end

        // Pop
        if (pop_i & valid_o)
        begin
            rd_ptr_q      <= rd_ptr_q + 1'd1;
        end

        // Count up
        if ((push_i & accept_o) & ~(pop_i & valid_o))
        begin
            count_q <= count_q + 2'd1;
        end
        // Count down
        else if (~(push_i & accept_o) & (pop_i & valid_o))
        begin
            count_q <= count_q - 2'd1;
        end
    end
end

//-------------------------------------------------------------------
// Combinatorial
//-------------------------------------------------------------------
assign valid_o       = (count_q != 2'd0);
assign accept_o      = (count_q != 2'd2);




assign data_out_o    = ram_q[rd_ptr_q];



endmodule


module axi4retime_fifo2x46
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input  [ 45:0]  data_in_i
    ,input           push_i
    ,input           pop_i

    // Outputs
    ,output [ 45:0]  data_out_o
    ,output          accept_o
    ,output          valid_o
);



//-----------------------------------------------------------------
// Registers
//-----------------------------------------------------------------
reg [45:0]    ram_q[1:0];
reg [0:0]   rd_ptr_q;
reg [0:0]   wr_ptr_q;
reg [1:0]  count_q;

//-----------------------------------------------------------------
// Sequential
//-----------------------------------------------------------------
always @ (posedge clk_i or posedge rst_i)
begin
    if (rst_i == 1'b1)
    begin
        count_q   <= 2'b0;
        rd_ptr_q  <= 1'b0;
        wr_ptr_q  <= 1'b0;
    end
    else
    begin
        // Push
        if (push_i & accept_o)
        begin
            ram_q[wr_ptr_q] <= data_in_i;
            wr_ptr_q        <= wr_ptr_q + 1'd1;
        end

        // Pop
        if (pop_i & valid_o)
        begin
            rd_ptr_q      <= rd_ptr_q + 1'd1;
        end

        // Count up
        if ((push_i & accept_o) & ~(pop_i & valid_o))
        begin
            count_q <= count_q + 2'd1;
        end
        // Count down
        else if (~(push_i & accept_o) & (pop_i & valid_o))
        begin
            count_q <= count_q - 2'd1;
        end
    end
end

//-------------------------------------------------------------------
// Combinatorial
//-------------------------------------------------------------------
assign valid_o       = (count_q != 2'd0);
assign accept_o      = (count_q != 2'd2);




assign data_out_o    = ram_q[rd_ptr_q];



endmodule

module axi4retime_fifo2x6
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input  [  5:0]  data_in_i
    ,input           push_i
    ,input           pop_i

    // Outputs
    ,output [  5:0]  data_out_o
    ,output          accept_o
    ,output          valid_o
);



//-----------------------------------------------------------------
// Registers
//-----------------------------------------------------------------
reg [5:0]    ram_q[1:0];
reg [0:0]   rd_ptr_q;
reg [0:0]   wr_ptr_q;
reg [1:0]  count_q;

//-----------------------------------------------------------------
// Sequential
//-----------------------------------------------------------------
always @ (posedge clk_i or posedge rst_i)
begin
    if (rst_i == 1'b1)
    begin
        count_q   <= 2'b0;
        rd_ptr_q  <= 1'b0;
        wr_ptr_q  <= 1'b0;
    end
    else
    begin
        // Push
        if (push_i & accept_o)
        begin
            ram_q[wr_ptr_q] <= data_in_i;
            wr_ptr_q        <= wr_ptr_q + 1'd1;
        end

        // Pop
        if (pop_i & valid_o)
        begin
            rd_ptr_q      <= rd_ptr_q + 1'd1;
        end

        // Count up
        if ((push_i & accept_o) & ~(pop_i & valid_o))
        begin
            count_q <= count_q + 2'd1;
        end
        // Count down
        else if (~(push_i & accept_o) & (pop_i & valid_o))
        begin
            count_q <= count_q - 2'd1;
        end
    end
end

//-------------------------------------------------------------------
// Combinatorial
//-------------------------------------------------------------------
assign valid_o       = (count_q != 2'd0);
assign accept_o      = (count_q != 2'd2);




assign data_out_o    = ram_q[rd_ptr_q];




endmodule
//-----------------------------------------------------------------
//          FTDI FT601 SuperSpeed USB3.0 to AXI bus master
//                              V0.1
//                        Ultra-Embedded.com
//                         Copyright 2020
//
//                 Email: admin@ultra-embedded.com
//
//                          License: GPL
//-----------------------------------------------------------------
//
// This source code is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This source code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this source code.  If not, see <https://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------

module ft60x_axi
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
     parameter RETIME_AXI       = 1
    ,parameter AXI_ID           = 0
)
//-----------------------------------------------------------------
// Ports
//-----------------------------------------------------------------
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           ftdi_rxf_i
    ,input           ftdi_txe_i
    ,input  [ 31:0]  ftdi_data_in_i
    ,input  [  3:0]  ftdi_be_in_i
    ,input           outport_awready_i
    ,input           outport_wready_i
    ,input           outport_bvalid_i
    ,input  [  1:0]  outport_bresp_i
    ,input  [  3:0]  outport_bid_i
    ,input           outport_arready_i
    ,input           outport_rvalid_i
    ,input  [ 31:0]  outport_rdata_i
    ,input  [  1:0]  outport_rresp_i
    ,input  [  3:0]  outport_rid_i
    ,input           outport_rlast_i
    ,input  [ 31:0]  gpio_inputs_i

    // Outputs
    ,output          ftdi_wrn_o
    ,output          ftdi_rdn_o
    ,output          ftdi_oen_o
    ,output [ 31:0]  ftdi_data_out_o
    ,output [  3:0]  ftdi_be_out_o
    ,output          outport_awvalid_o
    ,output [ 31:0]  outport_awaddr_o
    ,output [  3:0]  outport_awid_o
    ,output [  7:0]  outport_awlen_o
    ,output [  1:0]  outport_awburst_o
    ,output          outport_wvalid_o
    ,output [ 31:0]  outport_wdata_o
    ,output [  3:0]  outport_wstrb_o
    ,output          outport_wlast_o
    ,output          outport_bready_o
    ,output          outport_arvalid_o
    ,output [ 31:0]  outport_araddr_o
    ,output [  3:0]  outport_arid_o
    ,output [  7:0]  outport_arlen_o
    ,output [  1:0]  outport_arburst_o
    ,output          outport_rready_o
    ,output [ 31:0]  gpio_outputs_o
);



//-----------------------------------------------------------------
// AXI interface retiming
//-----------------------------------------------------------------
wire          outport_awvalid_w;
wire [ 31:0]  outport_awaddr_w;
wire [  3:0]  outport_awid_w;
wire [  7:0]  outport_awlen_w;
wire [  1:0]  outport_awburst_w;
wire          outport_wvalid_w;
wire [ 31:0]  outport_wdata_w;
wire [  3:0]  outport_wstrb_w;
wire          outport_wlast_w;
wire          outport_bready_w;
wire          outport_arvalid_w;
wire [ 31:0]  outport_araddr_w;
wire [  3:0]  outport_arid_w;
wire [  7:0]  outport_arlen_w;
wire [  1:0]  outport_arburst_w;
wire          outport_rready_w;
wire          outport_awready_w;
wire          outport_wready_w;
wire          outport_bvalid_w;
wire [  1:0]  outport_bresp_w;
wire [  3:0]  outport_bid_w;
wire          outport_arready_w;
wire          outport_rvalid_w;
wire [ 31:0]  outport_rdata_w;
wire [  1:0]  outport_rresp_w;
wire [  3:0]  outport_rid_w;
wire          outport_rlast_w;

generate
if (RETIME_AXI)
begin
    ft60x_axi_retime 
    u_retime
    (
         .clk_i(clk_i)
        ,.rst_i(rst_i)

        ,.inport_awvalid_i(outport_awvalid_w)
        ,.inport_awaddr_i(outport_awaddr_w)
        ,.inport_awid_i(outport_awid_w)
        ,.inport_awlen_i(outport_awlen_w)
        ,.inport_awburst_i(outport_awburst_w)
        ,.inport_wvalid_i(outport_wvalid_w)
        ,.inport_wdata_i(outport_wdata_w)
        ,.inport_wstrb_i(outport_wstrb_w)
        ,.inport_wlast_i(outport_wlast_w)
        ,.inport_bready_i(outport_bready_w)
        ,.inport_arvalid_i(outport_arvalid_w)
        ,.inport_araddr_i(outport_araddr_w)
        ,.inport_arid_i(outport_arid_w)
        ,.inport_arlen_i(outport_arlen_w)
        ,.inport_arburst_i(outport_arburst_w)
        ,.inport_rready_i(outport_rready_w)
        ,.inport_awready_o(outport_awready_w)
        ,.inport_wready_o(outport_wready_w)
        ,.inport_bvalid_o(outport_bvalid_w)
        ,.inport_bresp_o(outport_bresp_w)
        ,.inport_bid_o(outport_bid_w)
        ,.inport_arready_o(outport_arready_w)
        ,.inport_rvalid_o(outport_rvalid_w)
        ,.inport_rdata_o(outport_rdata_w)
        ,.inport_rresp_o(outport_rresp_w)
        ,.inport_rid_o(outport_rid_w)
        ,.inport_rlast_o(outport_rlast_w)

        ,.outport_awvalid_o(outport_awvalid_o)
        ,.outport_awaddr_o(outport_awaddr_o)
        ,.outport_awid_o(outport_awid_o)
        ,.outport_awlen_o(outport_awlen_o)
        ,.outport_awburst_o(outport_awburst_o)
        ,.outport_wvalid_o(outport_wvalid_o)
        ,.outport_wdata_o(outport_wdata_o)
        ,.outport_wstrb_o(outport_wstrb_o)
        ,.outport_wlast_o(outport_wlast_o)
        ,.outport_bready_o(outport_bready_o)
        ,.outport_arvalid_o(outport_arvalid_o)
        ,.outport_araddr_o(outport_araddr_o)
        ,.outport_arid_o(outport_arid_o)
        ,.outport_arlen_o(outport_arlen_o)
        ,.outport_arburst_o(outport_arburst_o)
        ,.outport_rready_o(outport_rready_o)
        ,.outport_awready_i(outport_awready_i)
        ,.outport_wready_i(outport_wready_i)
        ,.outport_bvalid_i(outport_bvalid_i)
        ,.outport_bresp_i(outport_bresp_i)
        ,.outport_bid_i(outport_bid_i)
        ,.outport_arready_i(outport_arready_i)
        ,.outport_rvalid_i(outport_rvalid_i)
        ,.outport_rdata_i(outport_rdata_i)
        ,.outport_rresp_i(outport_rresp_i)
        ,.outport_rid_i(outport_rid_i)
        ,.outport_rlast_i(outport_rlast_i)
    );
end
else
begin
    assign outport_arvalid_o = outport_arvalid_w;
    assign outport_araddr_o  = outport_araddr_w;
    assign outport_arid_o    = outport_arid_w;
    assign outport_arlen_o   = outport_arlen_w;
    assign outport_arburst_o = outport_arburst_w;
    assign outport_awvalid_o = outport_awvalid_w;
    assign outport_awaddr_o  = outport_awaddr_w;
    assign outport_awid_o    = outport_awid_w;
    assign outport_awlen_o   = outport_awlen_w;
    assign outport_awburst_o = outport_awburst_w;
    assign outport_wvalid_o  = outport_wvalid_w;
    assign outport_wdata_o   = outport_wdata_w;
    assign outport_wstrb_o   = outport_wstrb_w;
    assign outport_wlast_o   = outport_wlast_w;
    assign outport_rready_o  = outport_rready_w;
    assign outport_bready_o  = outport_bready_w;

    assign outport_awready_w = outport_awready_i;
    assign outport_wready_w  = outport_wready_i;
    assign outport_bvalid_w  = outport_bvalid_i;
    assign outport_bresp_w   = outport_bresp_i;
    assign outport_bid_w     = outport_bid_i;
    assign outport_arready_w = outport_arready_i;
    assign outport_rvalid_w  = outport_rvalid_i;
    assign outport_rdata_w   = outport_rdata_i;
    assign outport_rresp_w   = outport_rresp_i;
    assign outport_rid_w     = outport_rid_i;
    assign outport_rlast_w   = outport_rlast_i;
end
endgenerate

//-----------------------------------------------------------------
// FT60x <-> Stream
//-----------------------------------------------------------------
wire          rx_valid_w;
wire [ 31:0]  rx_data_w;
wire          rx_accept_w;

wire          tx_valid_w;
wire [ 31:0]  tx_data_w;
wire          tx_accept_w;

ft60x_fifo
u_ram
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.ftdi_rxf_i(ftdi_rxf_i)
    ,.ftdi_txe_i(ftdi_txe_i)
    ,.ftdi_data_in_i(ftdi_data_in_i)
    ,.ftdi_be_in_i(ftdi_be_in_i)
    ,.ftdi_wrn_o(ftdi_wrn_o)
    ,.ftdi_rdn_o(ftdi_rdn_o)
    ,.ftdi_oen_o(ftdi_oen_o)
    ,.ftdi_data_out_o(ftdi_data_out_o)
    ,.ftdi_be_out_o(ftdi_be_out_o)

    ,.inport_valid_i(tx_valid_w)
    ,.inport_data_i(tx_data_w)
    ,.inport_accept_o(tx_accept_w)

    ,.outport_valid_o(rx_valid_w)
    ,.outport_data_o(rx_data_w)
    ,.outport_accept_i(rx_accept_w)
);

//-----------------------------------------------------------------
// Defines / Local params
//-----------------------------------------------------------------
localparam STATE_W           = 4;
localparam STATE_IDLE        = 4'd0;
localparam STATE_CMD_REQ     = 4'd1;
localparam STATE_CMD_ADDR    = 4'd2;
localparam STATE_ECHO        = 4'd3;
localparam STATE_STATUS      = 4'd4;
localparam STATE_READ_CMD    = 4'd5;
localparam STATE_READ_DATA   = 4'd6;
localparam STATE_WRITE_CMD   = 4'd7;
localparam STATE_WRITE_DATA  = 4'd8;
localparam STATE_WRITE_RESP  = 4'd9;
localparam STATE_DRAIN       = 4'd10;
localparam STATE_GPIO_WR     = 4'd11;
localparam STATE_GPIO_RD     = 4'd12;

localparam CMD_ID_ECHO       = 8'h01;
localparam CMD_ID_DRAIN      = 8'h02;
localparam CMD_ID_READ       = 8'h10;
localparam CMD_ID_WRITE8_NP  = 8'h20; // 8-bit write (with response)
localparam CMD_ID_WRITE16_NP = 8'h21; // 16-bit write (with response)
localparam CMD_ID_WRITE_NP   = 8'h22; // 32-bit write (with response)
localparam CMD_ID_WRITE8     = 8'h30; // 8-bit write
localparam CMD_ID_WRITE16    = 8'h31; // 16-bit write
localparam CMD_ID_WRITE      = 8'h32; // 32-bit write
localparam CMD_ID_GPIO_WR    = 8'h40;
localparam CMD_ID_GPIO_RD    = 8'h41;

reg [STATE_W-1:0] state_q;
reg [7:0]         cmd_len_q;
reg [31:0]        cmd_addr_q;
reg [15:0]        cmd_seq_q;
reg [7:0]         cmd_id_q;

reg [7:0]         stat_len_q;
reg [1:0]         stat_resp_q;

//-----------------------------------------------------------------
// Next State Logic
//-----------------------------------------------------------------
reg [STATE_W-1:0] next_state_r;
always @ *
begin
    next_state_r = state_q;

    case (state_q)
    //-----------------------------------------
    // STATE_IDLE
    //-----------------------------------------
    STATE_IDLE :
    begin
        if (rx_valid_w)
            next_state_r = STATE_CMD_REQ;
    end
    //-----------------------------------------
    // STATE_CMD_REQ
    //-----------------------------------------
    STATE_CMD_REQ :
    begin
        if (rx_valid_w) next_state_r  = STATE_CMD_ADDR;
    end
    //-----------------------------------------
    // STATE_CMD_ADDR
    //-----------------------------------------
    STATE_CMD_ADDR :
    begin
        if (cmd_id_q == CMD_ID_ECHO && cmd_len_q != 8'b0)
            next_state_r = STATE_ECHO;
        else if (cmd_id_q == CMD_ID_ECHO && cmd_len_q == 8'b0)
            next_state_r = STATE_STATUS;
        else if (cmd_id_q == CMD_ID_READ)
            next_state_r = STATE_READ_CMD;
        else if (cmd_id_q == CMD_ID_WRITE8_NP  ||
                 cmd_id_q == CMD_ID_WRITE16_NP || 
                 cmd_id_q == CMD_ID_WRITE_NP   ||
                 cmd_id_q == CMD_ID_WRITE8     ||
                 cmd_id_q == CMD_ID_WRITE16    || 
                 cmd_id_q == CMD_ID_WRITE)
            next_state_r = STATE_WRITE_CMD;
        else if (cmd_id_q == CMD_ID_DRAIN)
            next_state_r = STATE_DRAIN;
        else if (cmd_id_q == CMD_ID_GPIO_WR)
            next_state_r = STATE_GPIO_WR;
        else if (cmd_id_q == CMD_ID_GPIO_RD)
            next_state_r = STATE_GPIO_RD;
    end
    //-----------------------------------------
    // STATE_ECHO
    //-----------------------------------------
    STATE_ECHO :
    begin
        if ((stat_len_q + 8'd1) == cmd_len_q && tx_accept_w)
            next_state_r = STATE_STATUS;
    end
    //-----------------------------------------
    // STATE_READ_CMD
    //-----------------------------------------
    STATE_READ_CMD :
    begin
        if (outport_arready_w)
            next_state_r = STATE_READ_DATA;
    end
    //-----------------------------------------
    // STATE_READ_DATA
    //-----------------------------------------
    STATE_READ_DATA :
    begin
        if (outport_rvalid_w && outport_rready_w && outport_rlast_w)
            next_state_r = STATE_STATUS;
    end
    //-----------------------------------------
    // STATE_WRITE_CMD
    //-----------------------------------------
    STATE_WRITE_CMD :
    begin
        if (outport_awready_w)
            next_state_r = STATE_WRITE_DATA;
    end
    //-----------------------------------------
    // STATE_WRITE_DATA
    //-----------------------------------------
    STATE_WRITE_DATA :
    begin
        if (outport_wvalid_w && outport_wlast_w && outport_wready_w)
            next_state_r = STATE_WRITE_RESP;
    end
    //-----------------------------------------
    // STATE_WRITE_RESP
    //-----------------------------------------
    STATE_WRITE_RESP :
    begin
        if (outport_bvalid_w && outport_bready_w)
        begin
            if (cmd_id_q == CMD_ID_WRITE8   ||
                cmd_id_q == CMD_ID_WRITE16  || 
                cmd_id_q == CMD_ID_WRITE)
                next_state_r = STATE_IDLE;
            else
                next_state_r = STATE_STATUS;
        end
    end
    //-----------------------------------------
    // STATE_STATUS
    //-----------------------------------------
    STATE_STATUS :
    begin
        if (tx_accept_w) next_state_r = STATE_IDLE;
    end
    //-----------------------------------------
    // STATE_DRAIN
    //-----------------------------------------
    STATE_DRAIN :
    begin
        if (!rx_valid_w) next_state_r = STATE_IDLE;
    end
    //-----------------------------------------
    // STATE_GPIO_WR
    //-----------------------------------------
    STATE_GPIO_WR :
    begin
        if (rx_valid_w) next_state_r = STATE_STATUS;
    end
    //-----------------------------------------
    // STATE_GPIO_RD
    //-----------------------------------------
    STATE_GPIO_RD :
    begin
        if (tx_accept_w) next_state_r = STATE_STATUS;
    end
    default:
        ;
   endcase
end

// Update state
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    state_q   <= STATE_IDLE;
else
    state_q   <= next_state_r;

//-----------------------------------------------------------------
// Command capture
//-----------------------------------------------------------------
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    cmd_id_q <= 8'b0;
else if (state_q != STATE_CMD_REQ && next_state_r == STATE_CMD_REQ)
    cmd_id_q <= rx_data_w[7:0];

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    cmd_len_q <= 8'b0;
else if (state_q != STATE_CMD_REQ && next_state_r == STATE_CMD_REQ)
    cmd_len_q <= rx_data_w[15:8];

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    cmd_seq_q <= 16'b0;
else if (state_q != STATE_CMD_REQ && next_state_r == STATE_CMD_REQ)
    cmd_seq_q <= rx_data_w[31:16];

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    cmd_addr_q <= 32'b0;
else if (state_q != STATE_CMD_ADDR && next_state_r == STATE_CMD_ADDR)
    cmd_addr_q <= rx_data_w;

//-----------------------------------------------------------------
// Length
//-----------------------------------------------------------------
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    stat_len_q <= 8'b0;
else if (state_q != STATE_CMD_REQ && next_state_r == STATE_CMD_REQ)
    stat_len_q <= 8'b0;
else if (state_q == STATE_ECHO)
    stat_len_q <= stat_len_q + 8'd1;
else if (state_q == STATE_WRITE_DATA && outport_wvalid_w && outport_wready_w)
    stat_len_q <= stat_len_q + 8'd1;

//-----------------------------------------------------------------
// Write Mask
//-----------------------------------------------------------------
reg [3:0] write_strb_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    write_strb_q <= 4'b0;
else if (state_q != STATE_CMD_ADDR && next_state_r == STATE_CMD_ADDR)
begin
    if (cmd_id_q == CMD_ID_WRITE8 || cmd_id_q == CMD_ID_WRITE8_NP)
    begin
        case (rx_data_w[1:0])
        2'b00: write_strb_q <= 4'b0001;
        2'b01: write_strb_q <= 4'b0010;
        2'b10: write_strb_q <= 4'b0100;
        2'b11: write_strb_q <= 4'b1000;
        endcase
    end
    else if (cmd_id_q == CMD_ID_WRITE16 || cmd_id_q == CMD_ID_WRITE16_NP)
    begin
        case ({rx_data_w[1],1'b0})
        2'b00: write_strb_q <= 4'b0011;
        2'b10: write_strb_q <= 4'b1100;
        default: ;
        endcase
    end
    else
        write_strb_q <= 4'b1111;
end

//-----------------------------------------------------------------
// Handshaking
//-----------------------------------------------------------------
reg rx_accept_r;
always @ *
begin
    rx_accept_r = 1'b0;

    case (state_q)
    STATE_IDLE,
    STATE_CMD_REQ,
    STATE_GPIO_WR,
    STATE_DRAIN :     rx_accept_r = 1'b1;
    STATE_CMD_ADDR :  rx_accept_r = 1'b0;
    STATE_ECHO:       rx_accept_r = tx_accept_w;
    STATE_WRITE_DATA: rx_accept_r = outport_wready_w;
    default:
        ;
   endcase
end

assign rx_accept_w = rx_accept_r;

reg        tx_valid_r;
reg [31:0] tx_data_r;

reg [31:0] gpio_in_q;

always @ *
begin
    tx_valid_r = 1'b0;
    tx_data_r  = 32'b0;

    case (state_q)
    STATE_ECHO:
    begin
        tx_valid_r = rx_valid_w;
        tx_data_r  = rx_data_w;
    end
    STATE_STATUS:
    begin
        tx_valid_r = 1'b1;
        tx_data_r  = {14'b0, stat_resp_q, cmd_seq_q};
    end
    STATE_READ_DATA:
    begin
        tx_valid_r = outport_rvalid_w;
        tx_data_r  = outport_rdata_w;
    end
    STATE_GPIO_RD:
    begin
        tx_valid_r = 1'b1;
        tx_data_r  = gpio_in_q[31:0];
    end
    default:
        ;
   endcase
end

assign tx_valid_w = tx_valid_r;
assign tx_data_w  = tx_data_r;

//-----------------------------------------------------------------
// AXI Read
//-----------------------------------------------------------------
assign outport_arvalid_w = (state_q == STATE_READ_CMD);
assign outport_araddr_w  = cmd_addr_q;
assign outport_arid_w    = AXI_ID;
assign outport_arlen_w   = cmd_len_q - 8'd1;
assign outport_arburst_w = 2'b01;

assign outport_rready_w  = (state_q == STATE_READ_DATA) && tx_accept_w;

//-----------------------------------------------------------------
// AXI Write
//-----------------------------------------------------------------
assign outport_awvalid_w = (state_q == STATE_WRITE_CMD);
assign outport_awaddr_w  = cmd_addr_q;
assign outport_awid_w    = AXI_ID;
assign outport_awlen_w   = cmd_len_q - 8'd1;
assign outport_awburst_w = 2'b01;

assign outport_wvalid_w  = (state_q == STATE_WRITE_DATA) && rx_valid_w;
assign outport_wdata_w   = rx_data_w;
assign outport_wstrb_w   = write_strb_q;
assign outport_wlast_w   = (stat_len_q + 8'd1) == cmd_len_q;

assign outport_bready_w  = 1'b1;

//-----------------------------------------------------------------
// AXI Response
//-----------------------------------------------------------------
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    stat_resp_q <= 2'b0;
else if (state_q == STATE_IDLE)
    stat_resp_q <= 2'b0;
else if (outport_bvalid_w && outport_bready_w)
    stat_resp_q <= outport_bresp_w;
else if (outport_rvalid_w && outport_rlast_w && outport_rready_w)
    stat_resp_q <= outport_rresp_w;

//-----------------------------------------------------------------
// GPIO Outputs
//-----------------------------------------------------------------
reg [31:0] gpio_out_q;
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    gpio_out_q <= 32'b0;
else if (state_q == STATE_GPIO_WR && rx_valid_w)
    gpio_out_q <= rx_data_w[31:0];

assign gpio_outputs_o = gpio_out_q;

//-----------------------------------------------------------------
// GPIO Inputs
//-----------------------------------------------------------------
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    gpio_in_q <= 32'b0;
else
    gpio_in_q <= gpio_inputs_i;


endmodule
