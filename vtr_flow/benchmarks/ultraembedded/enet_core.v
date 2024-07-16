//-----------------------------------------------------------------
//               Ethernet MAC 10/100 Mbps Interface
//                            V0.1.0
//               github.com/ultraembedded/core_enet
//                        Copyright 2021
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2021 github.com/ultraembedded
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//-----------------------------------------------------------------
//-------------------------------------------------------------------
// Dual port RAM
//-------------------------------------------------------------------
module enet_dp_ram
#(
    parameter WIDTH   = 32
   ,parameter ADDR_W  = 5
)
(
    // Inputs
     input                clk0_i
    ,input  [ADDR_W-1:0]  addr0_i
    ,input  [WIDTH-1:0]   data0_i
    ,input                wr0_i
    ,input                clk1_i
    ,input  [ADDR_W-1:0]  addr1_i
    ,input  [WIDTH-1:0]   data1_i
    ,input                wr1_i

    // Outputs
    ,output [WIDTH-1:0]  data0_o
    ,output [WIDTH-1:0]  data1_o
);

/* verilator lint_off MULTIDRIVEN */
reg [WIDTH-1:0]   ram [(2**ADDR_W)-1:0] /*verilator public*/;
/* verilator lint_on MULTIDRIVEN */

reg [WIDTH-1:0] ram_read0_q;
reg [WIDTH-1:0] ram_read1_q;

// Synchronous write
always @ (posedge clk0_i)
begin
    if (wr0_i)
        ram[addr0_i] <= data0_i;

    ram_read0_q <= ram[addr0_i];
end

always @ (posedge clk1_i)
begin
    if (wr1_i)
        ram[addr1_i] <= data1_i;

    ram_read1_q <= ram[addr1_i];
end

assign data0_o = ram_read0_q;
assign data1_o = ram_read1_q;

endmodule //-----------------------------------------------------------------
//               Ethernet MAC 10/100 Mbps Interface
//                            V0.1.0
//               github.com/ultraembedded/core_enet
//                        Copyright 2021
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2021 github.com/ultraembedded
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//-----------------------------------------------------------------
module enet_mac_rx
(
     input           clk_i
    ,input           rst_i

    ,input           promiscuous_i
    ,input  [ 47:0]  mac_addr_i

    ,input           cfg_wr_i
    ,input  [ 31:0]  cfg_addr_i
    ,input  [ 31:0]  cfg_data_wr_i
    ,output [ 31:0]  cfg_data_rd_o

    ,output [ 31:0]  ram_data_rd_o

    ,output          interrupt_o

    ,input           inport_tvalid_i
    ,input  [ 31:0]  inport_tdata_i
    ,input  [  3:0]  inport_tstrb_i
    ,input           inport_tlast_i
    ,input           inport_crc_valid_i
);

//-----------------------------------------------------------------
// Address check
//-----------------------------------------------------------------
reg first_q;

always @ (posedge clk_i )
if (rst_i)
    first_q <= 1'b1;
else if (inport_tvalid_i && inport_tlast_i)
    first_q <= 1'b1;
else  if (inport_tvalid_i)
    first_q <= 1'b0;

reg second_q;

always @ (posedge clk_i )
if (rst_i)
    second_q <= 1'b0;
else if (inport_tvalid_i)
    second_q <= first_q;

reg        valid_q;
reg [31:0] word0_q;
reg [3:0]  strb0_q;
reg        last_q;
reg        crc_ok_q;

always @ (posedge clk_i )
if (rst_i)
    valid_q <= 1'b0;
else if (inport_tvalid_i)
    valid_q <= 1'b1;
else if (valid_q && last_q)
    valid_q <= 1'b0;

always @ (posedge clk_i)
if (inport_tvalid_i)
    word0_q <= inport_tdata_i;

always @ (posedge clk_i)
if (inport_tvalid_i)
    strb0_q <= inport_tstrb_i;

always @ (posedge clk_i)
if (inport_tvalid_i)
    last_q <= inport_tlast_i;

always @ (posedge clk_i)
if (inport_tvalid_i)
    crc_ok_q <= inport_crc_valid_i;

wire        check_da_w = inport_tvalid_i && !first_q && second_q;
wire [47:0] da_w       = {inport_tdata_i[15:0], word0_q};

wire addr_match_w      = promiscuous_i ||
                         (da_w == mac_addr_i) ||
                         (da_w == 48'hFFFFFFFFFFFF);

//-----------------------------------------------------------------
// Space check
//-----------------------------------------------------------------
reg       buf_idx_q;
reg [1:0] ready_q;

wire      has_space_w = ~ready_q[buf_idx_q];

reg       drop_q;
wire      drop_w      = check_da_w ? (!addr_match_w || !has_space_w) : drop_q;
wire      final_w     = valid_q && last_q;

always @ (posedge clk_i )
if (rst_i)
    drop_q <= 1'b0;
else if (check_da_w)
    drop_q <= !addr_match_w || !has_space_w;
else if (final_w)
    drop_q <= 1'b0;

//-----------------------------------------------------------------
// Rx buffer (2 buffers - 4KB RAM)
//-----------------------------------------------------------------
reg  [8:0]  rx_addr_q;

wire valid_w = valid_q && ~drop_w && (inport_tvalid_i || last_q);

enet_dp_ram
#(
     .WIDTH(32)
    ,.ADDR_W(10)
)
u_rx_ram
(
     .clk0_i(clk_i)
    ,.addr0_i({buf_idx_q, rx_addr_q})
    ,.data0_i(word0_q)
    ,.wr0_i(valid_w)
    ,.data0_o()

    ,.clk1_i(clk_i)
    ,.addr1_i(cfg_addr_i[11:2])
    ,.data1_i(32'b0)
    ,.wr1_i(1'b0)
    ,.data1_o(ram_data_rd_o)
);

always @ (posedge clk_i )
if (rst_i)
    rx_addr_q <= 9'b0;
else if (final_w)
    rx_addr_q <= 9'b0;
else if (valid_w)
    rx_addr_q <= rx_addr_q + 9'd1;

always @ (posedge clk_i )
if (rst_i)
    buf_idx_q <= 1'b0;
else if (final_w && !drop_w && crc_ok_q)
    buf_idx_q <= ~buf_idx_q;

//-----------------------------------------------------------------
// Ready flag
//-----------------------------------------------------------------
reg [1:0] ready_r;

always @ *
begin
    ready_r = ready_q;

    if (final_w && !drop_w && crc_ok_q)
        ready_r[buf_idx_q] = 1'b1;

    if (cfg_wr_i && cfg_addr_i[15:0] == 16'h17FC && ~cfg_data_wr_i[0])
        ready_r[0] = 1'b0;

    if (cfg_wr_i && cfg_addr_i[15:0] == 16'h1fFC && ~cfg_data_wr_i[0])
        ready_r[1] = 1'b0;
end

always @ (posedge clk_i )
if (rst_i)
    ready_q <= 2'b0;
else
    ready_q <= ready_r;

//-----------------------------------------------------------------
// Interrupt Enable
//-----------------------------------------------------------------
reg ie_q;

always @ (posedge clk_i )
if (rst_i)
    ie_q <= 1'b0;
else if (cfg_wr_i && cfg_addr_i[15:0] == 16'h17FC)
    ie_q <= cfg_data_wr_i[3];

//-----------------------------------------------------------------
// Interrupt Output
//-----------------------------------------------------------------
assign interrupt_o = (|ready_q) & ie_q;

//-----------------------------------------------------------------
// Register Read
//-----------------------------------------------------------------
reg [31:0] read_data_r;

always @ *
begin
    read_data_r = 32'b0;

    case (cfg_addr_i[15:0])
    16'h17FC:   read_data_r = {28'b0, ie_q, 2'b0, ready_q[0]};
    16'h1FFC:   read_data_r = {28'b0, ie_q, 2'b0, ready_q[1]};
    default: ;
    endcase
end

assign cfg_data_rd_o = read_data_r;

endmodule
//-----------------------------------------------------------------
//               Ethernet MAC 10/100 Mbps Interface
//                            V0.1.0
//               github.com/ultraembedded/core_enet
//                        Copyright 2021
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2021 github.com/ultraembedded
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//-----------------------------------------------------------------
module enet_mac_tx
(
     input           clk_i
    ,input           rst_i

    ,input           cfg_wr_i
    ,input  [ 31:0]  cfg_addr_i
    ,input  [ 31:0]  cfg_data_wr_i
    ,output [ 31:0]  cfg_data_rd_o

    ,output          glbl_irq_en_o
    ,output          interrupt_o

    ,output          mac_update_o
    ,output [47:0]   mac_addr_o

    ,output          outport_tvalid_o
    ,output [ 31:0]  outport_tdata_o
    ,output [  3:0]  outport_tstrb_o
    ,output          outport_tlast_o
    ,input           outport_tready_i
);

localparam STS_MASK      = 32'h8000001F;
localparam STS_XMIT_IE   = 3;
localparam STS_PROGRAM   = 1;
localparam STS_BUSY      = 0;

//-----------------------------------------------------------------
// GIE
//-----------------------------------------------------------------
reg gie_q;

always @ (posedge clk_i )
if (rst_i)
    gie_q <= 1'b0;
else if (cfg_wr_i && (cfg_addr_i[15:0] == 16'h07F8))
    gie_q <= cfg_data_wr_i[31];

assign glbl_irq_en_o = gie_q;

//-----------------------------------------------------------------
// Tx buffer (2 buffers - 4KB RAM)
//-----------------------------------------------------------------
/* verilator lint_off UNSIGNED */
wire tx_buf_wr_w = cfg_wr_i && (cfg_addr_i[15:0] >= 16'h0000 && cfg_addr_i[15:0] < 16'h1000);
/* verilator lint_on UNSIGNED */

reg  [9:0]  tx_addr_q;
wire [31:0] tx_data_w;

enet_dp_ram
#(
     .WIDTH(32)
    ,.ADDR_W(10)
)
u_tx_ram
(
     .clk0_i(clk_i)
    ,.addr0_i(cfg_addr_i[11:2])
    ,.data0_i(cfg_data_wr_i)
    ,.wr0_i(tx_buf_wr_w)
    ,.data0_o()

    ,.clk1_i(clk_i)
    ,.addr1_i(tx_addr_q)
    ,.data1_i(32'b0)
    ,.wr1_i(1'b0)
    ,.data1_o(tx_data_w)
);

//-----------------------------------------------------------------
// Transmit Length
//-----------------------------------------------------------------
wire txlen0_wr_w = cfg_wr_i && (cfg_addr_i[15:0] == 16'h07F4);

reg [15:0] txlen0_q;

always @ (posedge clk_i )
if (rst_i)
    txlen0_q <= 16'b0;
else if (txlen0_wr_w)
    txlen0_q <= cfg_data_wr_i[15:0];

wire txlen1_wr_w = cfg_wr_i && (cfg_addr_i[15:0] == 16'h0FF4);

reg [15:0] txlen1_q;

always @ (posedge clk_i )
if (rst_i)
    txlen1_q <= 16'b0;
else if (txlen1_wr_w)
    txlen1_q <= cfg_data_wr_i[15:0];

//-----------------------------------------------------------------
// Transmit Control
//-----------------------------------------------------------------
wire       txctl0_wr_w = cfg_wr_i && (cfg_addr_i[15:0] == 16'h07FC);
wire       txctl0_clr_busy_w;
reg [31:0] txctl0_q;

always @ (posedge clk_i )
if (rst_i)
    txctl0_q <= 32'b0;
else if (txctl0_wr_w)
    txctl0_q <= (cfg_data_wr_i & STS_MASK);
else if (txctl0_clr_busy_w)
    txctl0_q <= {txctl0_q[31:2], 2'b0};

wire       txctl1_wr_w = cfg_wr_i && (cfg_addr_i[15:0] == 16'h0FFC);
wire       txctl1_clr_busy_w;
reg [31:0] txctl1_q;

always @ (posedge clk_i )
if (rst_i)
    txctl1_q <= 32'b0;
else if (txctl1_wr_w)
    txctl1_q <= (cfg_data_wr_i & STS_MASK);
else if (txctl1_clr_busy_w)
    txctl1_q <= {txctl1_q[31:2], 2'b0};

wire tx_start0_w = txctl0_q[STS_BUSY];
wire tx_start1_w = txctl1_q[STS_BUSY];

//-----------------------------------------------------------------
// Transmit SM
//-----------------------------------------------------------------
localparam STATE_W           = 2;
localparam STATE_IDLE        = 2'd0;
localparam STATE_READ        = 2'd1;
localparam STATE_WRITE       = 2'd2;
localparam STATE_END         = 2'd3;

reg [STATE_W-1:0]           state_q;
reg [STATE_W-1:0]           next_state_r;
reg [15:0]                  tx_length_q;

//-----------------------------------------------------------------
// Next State Logic
//-----------------------------------------------------------------
always @ *
begin
    next_state_r = state_q;

    case (state_q)
    //-------------------------------
    // STATE_IDLE
    //-------------------------------
    STATE_IDLE : 
    begin
        // Perform action
        if (tx_start0_w || tx_start1_w)
        begin
            // Program MAC address
            if (txctl0_q[STS_PROGRAM] && tx_start0_w)
                next_state_r = STATE_END;
            else if (txctl1_q[STS_PROGRAM] && tx_start1_w)
                next_state_r = STATE_END;
            // Send packet
            else
                next_state_r = STATE_READ;
        end
    end
    //-------------------------------
    // STATE_READ
    //-------------------------------
    STATE_READ : 
    begin
        if (outport_tready_i)
            next_state_r = STATE_WRITE;
    end
    //-------------------------------
    // STATE_WRITE
    //-------------------------------
    STATE_WRITE : 
    begin
        if (tx_length_q == 16'd0)
            next_state_r = STATE_END;
        else
            next_state_r = STATE_READ;
    end
    //-------------------------------
    // STATE_FRAME
    //-------------------------------
    STATE_END :
    begin 
        next_state_r = STATE_IDLE;
    end
    default :
        ;
    endcase
end

// Update state
always @ (posedge clk_i )
if (rst_i)
    state_q <= STATE_IDLE;
else
    state_q <= next_state_r;

//-----------------------------------------------------------------
// Buffer select
//-----------------------------------------------------------------
reg buf_idx_q;

always @ (posedge clk_i )
if (rst_i)
    buf_idx_q <= 1'b0;
else if (state_q == STATE_IDLE)
    buf_idx_q <= tx_start1_w;

//-----------------------------------------------------------------
// Transmit Length
//-----------------------------------------------------------------
wire [15:0] tx_len_w = tx_start0_w ? txlen0_q : txlen1_q;

always @ (posedge clk_i )
if (rst_i)
    tx_length_q <= 16'b0;
else if (state_q == STATE_IDLE)
    tx_length_q <= (tx_len_w < 16'd60) ? 16'd60 : tx_len_w;
else if (state_q == STATE_READ && outport_tready_i)
begin
    if (tx_length_q >= 16'd4)
        tx_length_q <= tx_length_q - 16'd4;
    else
        tx_length_q <= 16'd0;
end

reg [15:0] frame_len_q;

always @ (posedge clk_i )
if (rst_i)
    frame_len_q <= 16'b0;
else if (state_q == STATE_IDLE)
    frame_len_q <= tx_len_w;
else if (state_q == STATE_READ && outport_tready_i)
begin
    if (frame_len_q >= 16'd4)
        frame_len_q <= frame_len_q - 16'd4;
    else
        frame_len_q <= 16'd0;
end

reg padding_q;

always @ (posedge clk_i)
if (state_q == STATE_READ && outport_tready_i)
    padding_q <= (frame_len_q == 16'd0);

assign txctl0_clr_busy_w = (state_q == STATE_END) && ~buf_idx_q;
assign txctl1_clr_busy_w = (state_q == STATE_END) &&  buf_idx_q;

//-----------------------------------------------------------------
// Interrupt Enable
//-----------------------------------------------------------------
reg ie_q;

always @ (posedge clk_i )
if (rst_i)
    ie_q <= 1'b0;
else if (cfg_wr_i && cfg_addr_i[15:0] == 16'h07FC)
    ie_q <= cfg_data_wr_i[3];

//-----------------------------------------------------------------
// Interrupt Output
//-----------------------------------------------------------------
assign interrupt_o = (state_q == STATE_END) ? ie_q : 1'b0;

//-----------------------------------------------------------------
// Transmit Buffer Address
//-----------------------------------------------------------------
always @ (posedge clk_i )
if (rst_i)
    tx_addr_q <= 10'b0;
else if (state_q == STATE_IDLE)
    tx_addr_q <= tx_start0_w ? 10'h000 : 10'h200;
else if (state_q == STATE_READ && outport_tready_i)
    tx_addr_q <= tx_addr_q + 10'd1;

//-----------------------------------------------------------------
// Transmit Output
//-----------------------------------------------------------------
reg [3:0] tx_strb_q;

always @ (posedge clk_i )
if (rst_i)
    tx_strb_q <= 4'hF;
else if (state_q == STATE_READ)
begin
    if (tx_length_q >= 16'd4)
        tx_strb_q <= 4'hF;
    else case (tx_length_q[1:0])
    2'd3:    tx_strb_q <= 4'h7;
    2'd2:    tx_strb_q <= 4'h3;
    default: tx_strb_q <= 4'h1;
    endcase
end

reg tx_last_q;

always @ (posedge clk_i )
if (rst_i)
    tx_last_q <= 1'b0;
else if (state_q == STATE_READ)
    tx_last_q <= (tx_length_q <= 16'd4);

assign outport_tvalid_o = (state_q == STATE_WRITE);
assign outport_tdata_o  = padding_q ? 32'b0 : tx_data_w;
assign outport_tstrb_o  = tx_strb_q;
assign outport_tlast_o  = tx_last_q;

//-----------------------------------------------------------------
// Temporary address storage
//-----------------------------------------------------------------
reg [47:0] tmp_addr_q;

always @ (posedge clk_i)
if (cfg_wr_i && (cfg_addr_i[15:0] == 16'h0000))
    tmp_addr_q[31:0] <= cfg_data_wr_i[31:0];
else if (cfg_wr_i && (cfg_addr_i[15:0] == 16'h0004))
    tmp_addr_q[47:32] <= cfg_data_wr_i[15:0];

assign mac_update_o = (state_q == STATE_IDLE) && txctl0_q[STS_PROGRAM] && tx_start0_w;
assign mac_addr_o   = tmp_addr_q;

//-----------------------------------------------------------------
// Register Read
//-----------------------------------------------------------------
reg [31:0] read_data_r;

always @ *
begin
    read_data_r = 32'b0;

    case (cfg_addr_i[15:0])
    16'h07F8:   read_data_r = {gie_q, 31'b0};
    16'h07F4:   read_data_r = {16'b0, txlen0_q};
    16'h0FF4:   read_data_r = {16'b0, txlen1_q};
    16'h07FC:   read_data_r = {txctl0_q[31:4], ie_q, txctl0_q[2:0]};
    16'h0FFC:   read_data_r = {txctl1_q[31:4], ie_q, txctl1_q[2:0]};
    default: ;
    endcase
end

assign cfg_data_rd_o = read_data_r;

endmodule
//-----------------------------------------------------------------
//               Ethernet MAC 10/100 Mbps Interface
//                            V0.1.0
//               github.com/ultraembedded/core_enet
//                        Copyright 2021
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2021 github.com/ultraembedded
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//-----------------------------------------------------------------
module enet_mii_cdc
#(
    parameter WIDTH = 32
)
(
    // Inputs
     input           rd_clk_i
    ,input           rd_rst_i
    ,input           rd_pop_i
    ,input           wr_clk_i
    ,input           wr_rst_i
    ,input  [WIDTH-1:0]  wr_data_i
    ,input           wr_push_i

    // Outputs
    ,output [WIDTH-1:0]  rd_data_o
    ,output          rd_empty_o
    ,output          wr_full_o
);

//-----------------------------------------------------------------
// Registers
//-----------------------------------------------------------------
reg [4:0]       rd_ptr_q;
reg [4:0]       wr_ptr_q;

//-----------------------------------------------------------------
// Write
//-----------------------------------------------------------------
wire [4:0]      wr_ptr_next_w = wr_ptr_q + 5'd1;

always @ (posedge wr_clk_i or posedge wr_rst_i)
if (wr_rst_i)
    wr_ptr_q <= 5'b0;
else if (wr_push_i & ~wr_full_o)
    wr_ptr_q <= wr_ptr_next_w;

wire [4:0] wr_rd_ptr_w;

enet_mii_cdc_resync_bus
#( .WIDTH(5))
u_resync_rd_ptr_q
(
    .wr_clk_i(rd_clk_i),
    .wr_rst_i(rd_rst_i),
    .wr_i(1'b1),
    .wr_data_i(rd_ptr_q),
    .wr_busy_o(),
    .rd_clk_i(wr_clk_i),
    .rd_rst_i(wr_rst_i),
    .rd_data_o(wr_rd_ptr_w) // Delayed version of rd_ptr_q
);

assign wr_full_o = (wr_ptr_next_w == wr_rd_ptr_w);

//-------------------------------------------------------------------
// Dual port RAM
//-------------------------------------------------------------------
wire [WIDTH-1:0] rd_data_w;

enet_mii_cdc_ram_dp
#(.WIDTH(WIDTH))
u_ram
(
    // Inputs
    .clk0_i(wr_clk_i),
    .rst0_i(wr_rst_i),
    .clk1_i(rd_clk_i),
    .rst1_i(rd_rst_i),

    // Write side
    .addr0_i(wr_ptr_q),
    .wr0_i(wr_push_i & ~wr_full_o),
    .data0_i(wr_data_i),
    .data0_o(),

    // Read side
    .addr1_i(rd_ptr_q),
    .data1_i({(WIDTH){1'b0}}),
    .wr1_i(1'b0),
    .data1_o(rd_data_w)
);

//-----------------------------------------------------------------
// Read
//-----------------------------------------------------------------
wire [4:0] rd_wr_ptr_w;

enet_mii_cdc_resync_bus
#( .WIDTH(5))
u_resync_wr_ptr_q
(
    .wr_clk_i(wr_clk_i),
    .wr_rst_i(wr_rst_i),
    .wr_i(1'b1),
    .wr_data_i(wr_ptr_q),
    .wr_busy_o(),
    .rd_clk_i(rd_clk_i),
    .rd_rst_i(rd_rst_i),
    .rd_data_o(rd_wr_ptr_w) // Delayed version of wr_ptr_q
);

//-------------------------------------------------------------------
// Read Skid Buffer
//-------------------------------------------------------------------
reg                rd_skid_q;
reg [WIDTH-1:0]    rd_skid_data_q;
reg                rd_q;

wire read_ok_w = (rd_wr_ptr_w != rd_ptr_q);
wire valid_w   = (rd_skid_q | rd_q);

always @ (posedge rd_clk_i or posedge rd_rst_i)
if (rd_rst_i)
begin
    rd_skid_q <= 1'b0;
    rd_skid_data_q <= {(WIDTH){1'b0}};
end
else if (valid_w && !rd_pop_i)
begin
    rd_skid_q      <= 1'b1;
    rd_skid_data_q <= rd_data_o;
end
else
begin
    rd_skid_q      <= 1'b0;
    rd_skid_data_q <= {(WIDTH){1'b0}};
end

assign rd_data_o = rd_skid_q ? rd_skid_data_q : rd_data_w;

//-----------------------------------------------------------------
// Read Pointer
//-----------------------------------------------------------------
always @ (posedge rd_clk_i or posedge rd_rst_i)
if (rd_rst_i)
    rd_q <= 1'b0;
else
    rd_q <= read_ok_w;

wire [4:0] rd_ptr_next_w = rd_ptr_q + 5'd1;

always @ (posedge rd_clk_i or posedge rd_rst_i)
if (rd_rst_i)
    rd_ptr_q <= 5'b0;
// Read address increment
else if (read_ok_w && ((!valid_w) || (valid_w && rd_pop_i)))
    rd_ptr_q <= rd_ptr_next_w;

assign rd_empty_o = !valid_w;

endmodule

//-------------------------------------------------------------------
// Dual port RAM
//-------------------------------------------------------------------
module enet_mii_cdc_ram_dp
#(
    parameter WIDTH = 32
)
(
    // Inputs
     input           clk0_i
    ,input           rst0_i
    ,input  [ 4:0]  addr0_i
    ,input  [WIDTH-1:0]  data0_i
    ,input           wr0_i
    ,input           clk1_i
    ,input           rst1_i
    ,input  [ 4:0]  addr1_i
    ,input  [WIDTH-1:0]  data1_i
    ,input           wr1_i

    // Outputs
    ,output [WIDTH-1:0]  data0_o
    ,output [WIDTH-1:0]  data1_o
);

/* verilator lint_off MULTIDRIVEN */
reg [WIDTH-1:0]   ram [31:0] /*verilator public*/;
/* verilator lint_on MULTIDRIVEN */

reg [WIDTH-1:0] ram_read0_q;
reg [WIDTH-1:0] ram_read1_q;

// Synchronous write
always @ (posedge clk0_i)
begin
    if (wr0_i)
        ram[addr0_i] <= data0_i;

    ram_read0_q <= ram[addr0_i];
end

always @ (posedge clk1_i)
begin
    if (wr1_i)
        ram[addr1_i] <= data1_i;

    ram_read1_q <= ram[addr1_i];
end

assign data0_o = ram_read0_q;
assign data1_o = ram_read1_q;

endmodule 

module enet_mii_cdc_resync_bus
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
    parameter WIDTH     = 4
)
//-----------------------------------------------------------------
// Ports
//-----------------------------------------------------------------
(
    input              wr_clk_i,
    input              wr_rst_i,
    input              wr_i,
    input  [WIDTH-1:0] wr_data_i,
    output             wr_busy_o,

    input              rd_clk_i,
    input              rd_rst_i,
    output [WIDTH-1:0] rd_data_o
);

wire rd_toggle_w;
wire wr_toggle_w;

//-----------------------------------------------------------------
// Write
//-----------------------------------------------------------------
wire write_req_w = wr_i && !wr_busy_o;

// Write storage for domain crossing
(* ASYNC_REG = "TRUE", DONT_TOUCH = "TRUE" *) reg [WIDTH-1:0] wr_buffer_q;

always @ (posedge wr_clk_i or posedge wr_rst_i)
if (wr_rst_i)
    wr_buffer_q <= {(WIDTH){1'b0}};
else if (write_req_w)
    wr_buffer_q <= wr_data_i;

(* DONT_TOUCH = "TRUE" *) reg wr_toggle_q;
always @ (posedge wr_clk_i or posedge wr_rst_i)
if (wr_rst_i)
    wr_toggle_q <= 1'b0;
else if (write_req_w)
    wr_toggle_q <= ~wr_toggle_q;

reg wr_busy_q;
always @ (posedge wr_clk_i or posedge wr_rst_i)
if (wr_rst_i)
    wr_busy_q <= 1'b0;
else if (write_req_w)
    wr_busy_q <= 1'b1;
else if (wr_toggle_q == wr_toggle_w)
    wr_busy_q <= 1'b0;

assign wr_busy_o = wr_busy_q;

//-----------------------------------------------------------------
// Write -> Read request
//-----------------------------------------------------------------
enet_mii_cdc_resync
u_sync_wr_toggle
(
    .clk_i(rd_clk_i),
    .rst_i(rd_rst_i),
    .async_i(wr_toggle_q),
    .sync_o(rd_toggle_w)
);

//-----------------------------------------------------------------
// Read
//-----------------------------------------------------------------
(* DONT_TOUCH = "TRUE" *) reg rd_toggle_q;

always @ (posedge rd_clk_i or posedge rd_rst_i)
if (rd_rst_i)
    rd_toggle_q <= 1'b0;
else
    rd_toggle_q <= rd_toggle_w;

// Read storage for domain crossing
(* ASYNC_REG = "TRUE", DONT_TOUCH = "TRUE" *) reg [WIDTH-1:0] rd_buffer_q;

always @ (posedge rd_clk_i or posedge rd_rst_i)
if (rd_rst_i)
    rd_buffer_q <= {(WIDTH){1'b0}};
else if (rd_toggle_q != rd_toggle_w)
    rd_buffer_q <= wr_buffer_q; // Capture from other domain

assign rd_data_o = rd_buffer_q;

//-----------------------------------------------------------------
// Read->Write response
//-----------------------------------------------------------------
enet_mii_cdc_resync
u_sync_rd_toggle
(
    .clk_i(wr_clk_i),
    .rst_i(wr_rst_i),
    .async_i(rd_toggle_q),
    .sync_o(wr_toggle_w)
);

endmodule

module enet_mii_cdc_resync
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
    parameter RESET_VAL = 1'b0
)
//-----------------------------------------------------------------
// Ports
//-----------------------------------------------------------------
(
    input  clk_i,
    input  rst_i,
    input  async_i,
    output sync_o
);

(* ASYNC_REG = "TRUE", DONT_TOUCH = "TRUE" *) reg sync_ms;
(* ASYNC_REG = "TRUE", DONT_TOUCH = "TRUE" *) reg sync_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
begin
    sync_ms  <= RESET_VAL;
    sync_q   <= RESET_VAL;
end
else
begin
    sync_ms  <= async_i;
    sync_q   <= sync_ms;
end

assign sync_o = sync_q;


endmodule
//-----------------------------------------------------------------
//               Ethernet MAC 10/100 Mbps Interface
//                            V0.1.0
//               github.com/ultraembedded/core_enet
//                        Copyright 2021
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2021 github.com/ultraembedded
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//-----------------------------------------------------------------
module enet_mii_rx
(
     input           clk_i
    ,input           rst_i

    ,input  [  3:0]  mii_rxd_i
    ,input           mii_rx_dv_i
    ,input           mii_rx_er_i

    ,input           rd_clk_i
    ,input           rd_rst_i
    ,output          valid_o
    ,output [ 31:0]  data_o
    ,output [ 3:0]   strb_o
    ,output          last_o
    ,output          crc_valid_o
);

localparam NB_PREAMBLE       = 4'h5;
localparam NB_SFD            = 4'hd;

localparam STATE_W           = 2;
localparam STATE_IDLE        = 2'd0;
localparam STATE_WAIT_SFD    = 2'd1;
localparam STATE_FRAME       = 2'd2;
localparam STATE_END         = 2'd3;

reg [STATE_W-1:0]           state_q;
reg [STATE_W-1:0]           next_state_r;

//-----------------------------------------------------------------
// Capture flops
//-----------------------------------------------------------------
reg [3:0] rxd_q;
reg       rx_dv_q;

always @ (posedge clk_i)
    rxd_q <= mii_rxd_i;

always @ (posedge clk_i )
if (rst_i)
    rx_dv_q <= 1'b0;
else
    rx_dv_q <= mii_rx_er_i ? 1'b0 : mii_rx_dv_i;

//-----------------------------------------------------------------
// Next State Logic
//-----------------------------------------------------------------
always @ *
begin
    next_state_r = state_q;

    case (state_q)
    //-------------------------------
    // STATE_IDLE
    //-------------------------------
    STATE_IDLE : 
    begin
    	// Preamble detected
        if (rx_dv_q)
            next_state_r = STATE_WAIT_SFD;
    end
    //-------------------------------
    // STATE_WAIT_SFD
    //-------------------------------
    STATE_WAIT_SFD : 
    begin
    	// SFD detected
        if (rx_dv_q && rxd_q == NB_SFD)
            next_state_r = STATE_FRAME;
    end
    //-------------------------------
    // STATE_FRAME
    //-------------------------------
    STATE_FRAME :
    begin 
        if (!rx_dv_q)
            next_state_r = STATE_END;
    end
    //-------------------------------
    // STATE_END
    //-------------------------------
    STATE_END :
    begin 
        next_state_r = STATE_IDLE;
    end
    default :
        ;
    endcase
end

// Update state
always @ (posedge clk_i )
if (rst_i)
    state_q <= STATE_IDLE;
else
    state_q <= next_state_r;

//-----------------------------------------------------------------
// Byte reassembly
//-----------------------------------------------------------------
reg rx_toggle_q;

always @ (posedge clk_i )
if (rst_i)
    rx_toggle_q <= 1'b0;
else if (state_q == STATE_WAIT_SFD)
    rx_toggle_q <= 1'b0;
else if (state_q == STATE_FRAME)
    rx_toggle_q <= ~rx_toggle_q;

reg [7:0] rx_data_q;
reg       rx_valid_q;

always @ (posedge clk_i)
    rx_data_q <= {rxd_q ,rx_data_q[7:4]};

always @ (posedge clk_i )
if (rst_i)
    rx_valid_q <= 1'b0;
else if (state_q == STATE_FRAME && rx_dv_q && rx_toggle_q)
    rx_valid_q <= 1'b1;
else
    rx_valid_q <= 1'b0;

wire       rx_valid_w  = rx_valid_q;
wire [7:0] rx_data_w   = rx_data_q;
wire       rx_active_w = (state_q == STATE_FRAME);

//-----------------------------------------------------------------
// CRC check
//-----------------------------------------------------------------
// polynomial: x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x^1 + 1
// data width: 8
// convention: the first serial bit is D[7]
function [31:0] nextCRC32_D8;

input [7:0] Data;
input [31:0] crc;
reg [7:0] d;
reg [31:0] c;
reg [31:0] newcrc;
begin
    d = Data;
    c = crc;

    newcrc[0] = d[6] ^ d[0] ^ c[24] ^ c[30];
    newcrc[1] = d[7] ^ d[6] ^ d[1] ^ d[0] ^ c[24] ^ c[25] ^ c[30] ^ c[31];
    newcrc[2] = d[7] ^ d[6] ^ d[2] ^ d[1] ^ d[0] ^ c[24] ^ c[25] ^ c[26] ^ c[30] ^ c[31];
    newcrc[3] = d[7] ^ d[3] ^ d[2] ^ d[1] ^ c[25] ^ c[26] ^ c[27] ^ c[31];
    newcrc[4] = d[6] ^ d[4] ^ d[3] ^ d[2] ^ d[0] ^ c[24] ^ c[26] ^ c[27] ^ c[28] ^ c[30];
    newcrc[5] = d[7] ^ d[6] ^ d[5] ^ d[4] ^ d[3] ^ d[1] ^ d[0] ^ c[24] ^ c[25] ^ c[27] ^ c[28] ^ c[29] ^ c[30] ^ c[31];
    newcrc[6] = d[7] ^ d[6] ^ d[5] ^ d[4] ^ d[2] ^ d[1] ^ c[25] ^ c[26] ^ c[28] ^ c[29] ^ c[30] ^ c[31];
    newcrc[7] = d[7] ^ d[5] ^ d[3] ^ d[2] ^ d[0] ^ c[24] ^ c[26] ^ c[27] ^ c[29] ^ c[31];
    newcrc[8] = d[4] ^ d[3] ^ d[1] ^ d[0] ^ c[0] ^ c[24] ^ c[25] ^ c[27] ^ c[28];
    newcrc[9] = d[5] ^ d[4] ^ d[2] ^ d[1] ^ c[1] ^ c[25] ^ c[26] ^ c[28] ^ c[29];
    newcrc[10] = d[5] ^ d[3] ^ d[2] ^ d[0] ^ c[2] ^ c[24] ^ c[26] ^ c[27] ^ c[29];
    newcrc[11] = d[4] ^ d[3] ^ d[1] ^ d[0] ^ c[3] ^ c[24] ^ c[25] ^ c[27] ^ c[28];
    newcrc[12] = d[6] ^ d[5] ^ d[4] ^ d[2] ^ d[1] ^ d[0] ^ c[4] ^ c[24] ^ c[25] ^ c[26] ^ c[28] ^ c[29] ^ c[30];
    newcrc[13] = d[7] ^ d[6] ^ d[5] ^ d[3] ^ d[2] ^ d[1] ^ c[5] ^ c[25] ^ c[26] ^ c[27] ^ c[29] ^ c[30] ^ c[31];
    newcrc[14] = d[7] ^ d[6] ^ d[4] ^ d[3] ^ d[2] ^ c[6] ^ c[26] ^ c[27] ^ c[28] ^ c[30] ^ c[31];
    newcrc[15] = d[7] ^ d[5] ^ d[4] ^ d[3] ^ c[7] ^ c[27] ^ c[28] ^ c[29] ^ c[31];
    newcrc[16] = d[5] ^ d[4] ^ d[0] ^ c[8] ^ c[24] ^ c[28] ^ c[29];
    newcrc[17] = d[6] ^ d[5] ^ d[1] ^ c[9] ^ c[25] ^ c[29] ^ c[30];
    newcrc[18] = d[7] ^ d[6] ^ d[2] ^ c[10] ^ c[26] ^ c[30] ^ c[31];
    newcrc[19] = d[7] ^ d[3] ^ c[11] ^ c[27] ^ c[31];
    newcrc[20] = d[4] ^ c[12] ^ c[28];
    newcrc[21] = d[5] ^ c[13] ^ c[29];
    newcrc[22] = d[0] ^ c[14] ^ c[24];
    newcrc[23] = d[6] ^ d[1] ^ d[0] ^ c[15] ^ c[24] ^ c[25] ^ c[30];
    newcrc[24] = d[7] ^ d[2] ^ d[1] ^ c[16] ^ c[25] ^ c[26] ^ c[31];
    newcrc[25] = d[3] ^ d[2] ^ c[17] ^ c[26] ^ c[27];
    newcrc[26] = d[6] ^ d[4] ^ d[3] ^ d[0] ^ c[18] ^ c[24] ^ c[27] ^ c[28] ^ c[30];
    newcrc[27] = d[7] ^ d[5] ^ d[4] ^ d[1] ^ c[19] ^ c[25] ^ c[28] ^ c[29] ^ c[31];
    newcrc[28] = d[6] ^ d[5] ^ d[2] ^ c[20] ^ c[26] ^ c[29] ^ c[30];
    newcrc[29] = d[7] ^ d[6] ^ d[3] ^ c[21] ^ c[27] ^ c[30] ^ c[31];
    newcrc[30] = d[7] ^ d[4] ^ c[22] ^ c[28] ^ c[31];
    newcrc[31] = d[5] ^ c[23] ^ c[29];
    nextCRC32_D8 = newcrc;
end
endfunction

wire [7:0] rev_rx_data_w;

assign rev_rx_data_w[0] = rx_data_w[7-0];
assign rev_rx_data_w[1] = rx_data_w[7-1];
assign rev_rx_data_w[2] = rx_data_w[7-2];
assign rev_rx_data_w[3] = rx_data_w[7-3];
assign rev_rx_data_w[4] = rx_data_w[7-4];
assign rev_rx_data_w[5] = rx_data_w[7-5];
assign rev_rx_data_w[6] = rx_data_w[7-6];
assign rev_rx_data_w[7] = rx_data_w[7-7];

reg [31:0] crc_q;

always @ (posedge clk_i )
if (rst_i)
    crc_q <= {32{1'b1}};
else if (state_q == STATE_WAIT_SFD)
    crc_q <= {32{1'b1}};
else if (rx_valid_w)
    crc_q <= nextCRC32_D8(rev_rx_data_w, crc_q);

wire [31:0] crc_w = ~crc_q;

reg rx_active_q;

always @ (posedge clk_i )
if (rst_i)
    rx_active_q <= 1'b0;
else
    rx_active_q <= rx_active_w;

wire crc_check_w = !rx_active_w && rx_active_q;
wire crc_valid_w = (crc_w == 32'h38FB2284);

//-----------------------------------------------------------------
// Data write index
//-----------------------------------------------------------------
wire       flush_w = !rx_active_w && rx_active_q;

reg [1:0]  idx_q;
always @ (posedge clk_i )
if (rst_i)
    idx_q <= 2'b0;
else if (flush_w)
    idx_q <= 2'b0;
else if (rx_valid_w)
    idx_q <= idx_q + 2'd1;

//-----------------------------------------------------------------
// Data
//-----------------------------------------------------------------
reg [31:0] data_q;
reg [31:0] data_r;

always @ *
begin
    data_r = 32'b0;
    case (idx_q)
    2'd0: data_r = {24'b0,  rx_data_w};
    2'd1: data_r = {16'b0,  rx_data_w, data_q[7:0]};
    2'd2: data_r = {8'b0,   rx_data_w, data_q[15:0]};
    2'd3: data_r = {rx_data_w, data_q[23:0]};
    endcase
end

always @ (posedge clk_i)
if (rx_valid_w)
    data_q <= data_r;

//-----------------------------------------------------------------
// Valid
//-----------------------------------------------------------------
reg valid_q;

always @ (posedge clk_i )
if (rst_i)
    valid_q <= 1'b0;
else if (flush_w && idx_q != 2'd0)
    valid_q <= 1'b1;
else if (rx_valid_w && idx_q == 2'd3)
    valid_q <= 1'b1;
else
    valid_q <= 1'b0;

//-----------------------------------------------------------------
// Mask
//-----------------------------------------------------------------
reg [3:0]  mask_q;

always @ (posedge clk_i)
if (rx_valid_w)
begin
    case (idx_q)
    2'd0: mask_q <= 4'b0001;
    2'd1: mask_q <= mask_q | 4'b0010;
    2'd2: mask_q <= mask_q | 4'b0100;
    2'd3: mask_q <= mask_q | 4'b1000;
    endcase
end

//-----------------------------------------------------------------
// Last
//-----------------------------------------------------------------
wire early_w = (idx_q == 2'b0) && !rx_active_w;

reg last_q;

always @ (posedge clk_i)
    last_q <= !rx_active_w && rx_active_q;

wire last_w = valid_q & (last_q | early_w);

//-----------------------------------------------------------------
// CRC Check
//-----------------------------------------------------------------
reg crc_valid_q;

always @ (posedge clk_i)
if (crc_check_w)
    crc_valid_q <= crc_valid_w;

wire crc_res_w = early_w ? crc_valid_w : crc_valid_q;

//-----------------------------------------------------------------
// CDC
//-----------------------------------------------------------------
wire rd_empty_w;

enet_mii_cdc 
#(
    .WIDTH(32+4+1+1)
)
u_cdc
(
     .wr_clk_i(clk_i)
    ,.wr_rst_i(rst_i)
    ,.wr_push_i(valid_q)
    ,.wr_data_i({crc_res_w, last_w, mask_q, data_q})
    ,.wr_full_o()

    ,.rd_clk_i(rd_clk_i)
    ,.rd_rst_i(rd_rst_i)
    ,.rd_data_o({crc_valid_o, last_o, strb_o, data_o})
    ,.rd_pop_i(1'b1)
    ,.rd_empty_o(rd_empty_w)
);

assign valid_o = ~rd_empty_w;

endmodule
//-----------------------------------------------------------------
//               Ethernet MAC 10/100 Mbps Interface
//                            V0.1.0
//               github.com/ultraembedded/core_enet
//                        Copyright 2021
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2021 github.com/ultraembedded
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//-----------------------------------------------------------------
module enet_mii_tx
(
     input           clk_i
    ,input           rst_i

    ,output [  3:0]  mii_txd_o
    ,output          mii_tx_en_o

    ,input           wr_clk_i
    ,input           wr_rst_i
    ,input           valid_i
    ,input  [ 31:0]  data_i
    ,input  [ 3:0]   strb_i
    ,input           last_i
    ,output          accept_o
);

localparam NB_PREAMBLE       = 4'h5;
localparam NB_SFD            = 4'hd;

localparam STATE_W           = 3;
localparam STATE_IDLE        = 3'd0;
localparam STATE_PREAMBLE    = 3'd1;
localparam STATE_SFD         = 3'd2;
localparam STATE_FRAME       = 3'd3;
localparam STATE_CRC         = 3'd4;
localparam STATE_END         = 3'd5;

reg [STATE_W-1:0]           state_q;
reg [STATE_W-1:0]           next_state_r;

reg [7:0]                   state_count_q;

//-----------------------------------------------------------------
// CDC
//-----------------------------------------------------------------
wire        wr_full_w;
wire        rd_empty_w;

wire        tx_ready_w;
wire [31:0] tx_data_w;
wire [3:0]  tx_strb_w;
wire        tx_last_w;
wire        tx_accept_w;

enet_mii_cdc 
#(
    .WIDTH(32+4+1)
)
u_cdc
(
     .wr_clk_i(wr_clk_i)
    ,.wr_rst_i(wr_rst_i)
    ,.wr_push_i(valid_i)
    ,.wr_data_i({last_i, strb_i, data_i})
    ,.wr_full_o(wr_full_w)

    ,.rd_clk_i(clk_i)
    ,.rd_rst_i(rst_i)
    ,.rd_data_o({tx_last_w, tx_strb_w, tx_data_w})
    ,.rd_pop_i(tx_accept_w)
    ,.rd_empty_o(rd_empty_w)
);

assign accept_o    = ~wr_full_w;
assign tx_ready_w  = ~rd_empty_w;
assign tx_accept_w = (state_q == STATE_FRAME && next_state_r == STATE_CRC) ||
                     (state_q == STATE_FRAME && state_count_q[2:0] == 3'd7);

// Xilinx placement pragmas:
//synthesis attribute IOB of mii_txd_q is "TRUE"
//synthesis attribute IOB of mii_tx_en_q is "TRUE"
reg [  3:0]  mii_txd_q;
reg          mii_tx_en_q;

//-----------------------------------------------------------------
// Next State Logic
//-----------------------------------------------------------------
always @ *
begin
    next_state_r = state_q;

    case (state_q)
    //-------------------------------
    // STATE_IDLE
    //-------------------------------
    STATE_IDLE : 
    begin
        // Something to transmit
        if (tx_ready_w)
            next_state_r = STATE_PREAMBLE;
    end
    //-------------------------------
    // STATE_PREAMBLE
    //-------------------------------
    STATE_PREAMBLE : 
    begin
        // SFD detected
        if (state_count_q == 8'd14)
            next_state_r = STATE_SFD;
    end
    //-------------------------------
    // STATE_SFD
    //-------------------------------
    STATE_SFD : 
    begin
        next_state_r = STATE_FRAME;
    end
    //-------------------------------
    // STATE_FRAME
    //-------------------------------
    STATE_FRAME :
    begin 
        if (tx_last_w)
        begin
            if (tx_strb_w == 4'hF && state_count_q[2:0] == 3'd7)
                next_state_r = STATE_CRC;
            else if (tx_strb_w == 4'h7 && state_count_q[2:0] == 3'd5)
                next_state_r = STATE_CRC;
            else if (tx_strb_w == 4'h3 && state_count_q[2:0] == 3'd3)
                next_state_r = STATE_CRC;
            else if (tx_strb_w == 4'h1 && state_count_q[2:0] == 3'd1)
                next_state_r = STATE_CRC;
        end
    end
    //-------------------------------
    // STATE_CRC
    //-------------------------------
    STATE_CRC :
    begin 
        if (state_count_q == 8'd7)
            next_state_r = STATE_END;
    end
    //-------------------------------
    // STATE_END
    //-------------------------------
    STATE_END :
    begin
        if (state_count_q == 8'd24)
            next_state_r = STATE_IDLE;
    end
    default :
        ;
    endcase
end

// Update state
always @ (posedge clk_i )
if (rst_i)
    state_q <= STATE_IDLE;
else
    state_q <= next_state_r;

always @ (posedge clk_i )
if (rst_i)
    state_count_q <= 8'b0;
else if (state_q != next_state_r || state_q == STATE_IDLE)
    state_count_q <= 8'b0;
else
    state_count_q <= state_count_q + 8'd1;

//-----------------------------------------------------------------
// CRC check
//-----------------------------------------------------------------
// polynomial: x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x^1 + 1
// data width: 8
// convention: the first serial bit is D[7]
function [31:0] nextCRC32_D8;

input [7:0] Data;
input [31:0] crc;
reg [7:0] d;
reg [31:0] c;
reg [31:0] newcrc;
begin
    d = Data;
    c = crc;

    newcrc[0] = d[6] ^ d[0] ^ c[24] ^ c[30];
    newcrc[1] = d[7] ^ d[6] ^ d[1] ^ d[0] ^ c[24] ^ c[25] ^ c[30] ^ c[31];
    newcrc[2] = d[7] ^ d[6] ^ d[2] ^ d[1] ^ d[0] ^ c[24] ^ c[25] ^ c[26] ^ c[30] ^ c[31];
    newcrc[3] = d[7] ^ d[3] ^ d[2] ^ d[1] ^ c[25] ^ c[26] ^ c[27] ^ c[31];
    newcrc[4] = d[6] ^ d[4] ^ d[3] ^ d[2] ^ d[0] ^ c[24] ^ c[26] ^ c[27] ^ c[28] ^ c[30];
    newcrc[5] = d[7] ^ d[6] ^ d[5] ^ d[4] ^ d[3] ^ d[1] ^ d[0] ^ c[24] ^ c[25] ^ c[27] ^ c[28] ^ c[29] ^ c[30] ^ c[31];
    newcrc[6] = d[7] ^ d[6] ^ d[5] ^ d[4] ^ d[2] ^ d[1] ^ c[25] ^ c[26] ^ c[28] ^ c[29] ^ c[30] ^ c[31];
    newcrc[7] = d[7] ^ d[5] ^ d[3] ^ d[2] ^ d[0] ^ c[24] ^ c[26] ^ c[27] ^ c[29] ^ c[31];
    newcrc[8] = d[4] ^ d[3] ^ d[1] ^ d[0] ^ c[0] ^ c[24] ^ c[25] ^ c[27] ^ c[28];
    newcrc[9] = d[5] ^ d[4] ^ d[2] ^ d[1] ^ c[1] ^ c[25] ^ c[26] ^ c[28] ^ c[29];
    newcrc[10] = d[5] ^ d[3] ^ d[2] ^ d[0] ^ c[2] ^ c[24] ^ c[26] ^ c[27] ^ c[29];
    newcrc[11] = d[4] ^ d[3] ^ d[1] ^ d[0] ^ c[3] ^ c[24] ^ c[25] ^ c[27] ^ c[28];
    newcrc[12] = d[6] ^ d[5] ^ d[4] ^ d[2] ^ d[1] ^ d[0] ^ c[4] ^ c[24] ^ c[25] ^ c[26] ^ c[28] ^ c[29] ^ c[30];
    newcrc[13] = d[7] ^ d[6] ^ d[5] ^ d[3] ^ d[2] ^ d[1] ^ c[5] ^ c[25] ^ c[26] ^ c[27] ^ c[29] ^ c[30] ^ c[31];
    newcrc[14] = d[7] ^ d[6] ^ d[4] ^ d[3] ^ d[2] ^ c[6] ^ c[26] ^ c[27] ^ c[28] ^ c[30] ^ c[31];
    newcrc[15] = d[7] ^ d[5] ^ d[4] ^ d[3] ^ c[7] ^ c[27] ^ c[28] ^ c[29] ^ c[31];
    newcrc[16] = d[5] ^ d[4] ^ d[0] ^ c[8] ^ c[24] ^ c[28] ^ c[29];
    newcrc[17] = d[6] ^ d[5] ^ d[1] ^ c[9] ^ c[25] ^ c[29] ^ c[30];
    newcrc[18] = d[7] ^ d[6] ^ d[2] ^ c[10] ^ c[26] ^ c[30] ^ c[31];
    newcrc[19] = d[7] ^ d[3] ^ c[11] ^ c[27] ^ c[31];
    newcrc[20] = d[4] ^ c[12] ^ c[28];
    newcrc[21] = d[5] ^ c[13] ^ c[29];
    newcrc[22] = d[0] ^ c[14] ^ c[24];
    newcrc[23] = d[6] ^ d[1] ^ d[0] ^ c[15] ^ c[24] ^ c[25] ^ c[30];
    newcrc[24] = d[7] ^ d[2] ^ d[1] ^ c[16] ^ c[25] ^ c[26] ^ c[31];
    newcrc[25] = d[3] ^ d[2] ^ c[17] ^ c[26] ^ c[27];
    newcrc[26] = d[6] ^ d[4] ^ d[3] ^ d[0] ^ c[18] ^ c[24] ^ c[27] ^ c[28] ^ c[30];
    newcrc[27] = d[7] ^ d[5] ^ d[4] ^ d[1] ^ c[19] ^ c[25] ^ c[28] ^ c[29] ^ c[31];
    newcrc[28] = d[6] ^ d[5] ^ d[2] ^ c[20] ^ c[26] ^ c[29] ^ c[30];
    newcrc[29] = d[7] ^ d[6] ^ d[3] ^ c[21] ^ c[27] ^ c[30] ^ c[31];
    newcrc[30] = d[7] ^ d[4] ^ c[22] ^ c[28] ^ c[31];
    newcrc[31] = d[5] ^ c[23] ^ c[29];
    nextCRC32_D8 = newcrc;
end
endfunction

reg       crc_en_r;
reg [7:0] crc_in_r;

always @ *
begin
    case (state_count_q[2:1])
    2'd3:    crc_in_r = tx_data_w[31:24];
    2'd2:    crc_in_r = tx_data_w[23:16];
    2'd1:    crc_in_r = tx_data_w[15:8];
    default: crc_in_r = tx_data_w[7:0];
    endcase

    crc_en_r = state_q == STATE_FRAME && !state_count_q[0];
end

wire [7:0] crc_in_rev_w;
assign crc_in_rev_w[0] = crc_in_r[7-0];
assign crc_in_rev_w[1] = crc_in_r[7-1];
assign crc_in_rev_w[2] = crc_in_r[7-2];
assign crc_in_rev_w[3] = crc_in_r[7-3];
assign crc_in_rev_w[4] = crc_in_r[7-4];
assign crc_in_rev_w[5] = crc_in_r[7-5];
assign crc_in_rev_w[6] = crc_in_r[7-6];
assign crc_in_rev_w[7] = crc_in_r[7-7];

reg [31:0] crc_q;

always @ (posedge clk_i )
if (rst_i)
    crc_q <= {32{1'b1}};
else if (state_q == STATE_SFD)
    crc_q <= {32{1'b1}};
else if (crc_en_r)
    crc_q <= nextCRC32_D8(crc_in_rev_w, crc_q);

wire [31:0] crc_rev_w = ~crc_q;

wire [31:0] crc_w;
assign crc_w[0] = crc_rev_w[31-0];
assign crc_w[1] = crc_rev_w[31-1];
assign crc_w[2] = crc_rev_w[31-2];
assign crc_w[3] = crc_rev_w[31-3];
assign crc_w[4] = crc_rev_w[31-4];
assign crc_w[5] = crc_rev_w[31-5];
assign crc_w[6] = crc_rev_w[31-6];
assign crc_w[7] = crc_rev_w[31-7];
assign crc_w[8] = crc_rev_w[31-8];
assign crc_w[9] = crc_rev_w[31-9];
assign crc_w[10] = crc_rev_w[31-10];
assign crc_w[11] = crc_rev_w[31-11];
assign crc_w[12] = crc_rev_w[31-12];
assign crc_w[13] = crc_rev_w[31-13];
assign crc_w[14] = crc_rev_w[31-14];
assign crc_w[15] = crc_rev_w[31-15];
assign crc_w[16] = crc_rev_w[31-16];
assign crc_w[17] = crc_rev_w[31-17];
assign crc_w[18] = crc_rev_w[31-18];
assign crc_w[19] = crc_rev_w[31-19];
assign crc_w[20] = crc_rev_w[31-20];
assign crc_w[21] = crc_rev_w[31-21];
assign crc_w[22] = crc_rev_w[31-22];
assign crc_w[23] = crc_rev_w[31-23];
assign crc_w[24] = crc_rev_w[31-24];
assign crc_w[25] = crc_rev_w[31-25];
assign crc_w[26] = crc_rev_w[31-26];
assign crc_w[27] = crc_rev_w[31-27];
assign crc_w[28] = crc_rev_w[31-28];
assign crc_w[29] = crc_rev_w[31-29];
assign crc_w[30] = crc_rev_w[31-30];
assign crc_w[31] = crc_rev_w[31-31];

//-----------------------------------------------------------------
// Tx
//-----------------------------------------------------------------
reg       tx_en_r;
reg [3:0] txd_r;

always @ *
begin
    tx_en_r = 1'b0;
    txd_r   = 4'b0;

    case (state_q)
    //-------------------------------
    // STATE_PREAMBLE
    //-------------------------------
    STATE_PREAMBLE : 
    begin
        tx_en_r = 1'b1;
        txd_r   = NB_PREAMBLE;
    end
    //-------------------------------
    // STATE_SFD
    //-------------------------------
    STATE_SFD : 
    begin
        tx_en_r = 1'b1;
        txd_r   = NB_SFD;
    end
    //-------------------------------
    // STATE_FRAME
    //-------------------------------
    STATE_FRAME :
    begin 
        tx_en_r = 1'b1;
        case (state_count_q[2:0])
        3'd7:    txd_r = tx_data_w[31:28];
        3'd6:    txd_r = tx_data_w[27:24];
        3'd5:    txd_r = tx_data_w[23:20];
        3'd4:    txd_r = tx_data_w[19:16];
        3'd3:    txd_r = tx_data_w[15:12];
        3'd2:    txd_r = tx_data_w[11:8];
        3'd1:    txd_r = tx_data_w[7:4];
        default: txd_r = tx_data_w[3:0];
        endcase
    end
    //-------------------------------
    // STATE_CRC
    //-------------------------------
    STATE_CRC :
    begin 
        tx_en_r = 1'b1;
        case (state_count_q[2:0])
        3'd7:    txd_r = crc_w[31:28];
        3'd6:    txd_r = crc_w[27:24];
        3'd5:    txd_r = crc_w[23:20];
        3'd4:    txd_r = crc_w[19:16];
        3'd3:    txd_r = crc_w[15:12];
        3'd2:    txd_r = crc_w[11:8];
        3'd1:    txd_r = crc_w[7:4];
        default: txd_r = crc_w[3:0];
        endcase
    end
    default :
        ;
    endcase
end

always @ (posedge clk_i )
if (rst_i)
    mii_tx_en_q <= 1'b0;
else
    mii_tx_en_q <= tx_en_r;

always @ (posedge clk_i )
if (rst_i)
    mii_txd_q <= 4'b0;
else
    mii_txd_q <= txd_r;

assign mii_tx_en_o = mii_tx_en_q;
assign mii_txd_o   = mii_txd_q;

endmodule
//-----------------------------------------------------------------
//               Ethernet MAC 10/100 Mbps Interface
//                            V0.1.0
//               github.com/ultraembedded/core_enet
//                        Copyright 2021
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2021 github.com/ultraembedded
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//-----------------------------------------------------------------

module enet_mii
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           mii_rx_clk_i
    ,input  [  3:0]  mii_rxd_i
    ,input           mii_rx_dv_i
    ,input           mii_rx_er_i
    ,input           mii_tx_clk_i
    ,input           mii_col_i
    ,input           mii_crs_i
    ,input           tx_valid_i
    ,input  [ 31:0]  tx_data_i
    ,input  [  3:0]  tx_strb_i
    ,input           tx_last_i

    // Outputs
    ,output [  3:0]  mii_txd_o
    ,output          mii_tx_en_o
    ,output          mii_reset_n_o
    ,output          busy_o
    ,output          tx_accept_o
    ,output          rx_valid_o
    ,output [ 31:0]  rx_data_o
    ,output [  3:0]  rx_strb_o
    ,output          rx_last_o
    ,output          rx_crc_valid_o
);



//-----------------------------------------------------------------
// Reset Output
//-----------------------------------------------------------------
localparam RESET_COUNT = 12'd1024;
reg [11:0] rst_cnt_q;

always @ (posedge clk_i )
if (rst_i)
    rst_cnt_q <= 12'b0;
else if (rst_cnt_q < RESET_COUNT)
    rst_cnt_q <= rst_cnt_q + 12'd1;

reg rst_n_q;

always @ (posedge clk_i )
if (rst_i)
    rst_n_q <= 1'b0;
else if (rst_cnt_q == RESET_COUNT)
    rst_n_q <= 1'b1;

assign mii_reset_n_o = rst_n_q;

assign busy_o        = ~rst_n_q;

//-----------------------------------------------------------------
// Rx
//-----------------------------------------------------------------
reg rx_clk_rst_q = 1'b1;

always @ (posedge mii_rx_clk_i)
    rx_clk_rst_q <= 1'b0;

wire        valid_w;
wire [31:0] data_w;
wire        last_w;
wire [3:0]  mask_w;
wire        crc_valid_w;

enet_mii_rx
u_rx
(
     .clk_i(mii_rx_clk_i)
    ,.rst_i(rx_clk_rst_q)

    ,.mii_rxd_i(mii_rxd_i)
    ,.mii_rx_dv_i(mii_rx_dv_i)
    ,.mii_rx_er_i(mii_rx_er_i)

    ,.rd_clk_i(clk_i)
    ,.rd_rst_i(rst_i)
    ,.valid_o(rx_valid_o)
    ,.data_o(rx_data_o)
    ,.strb_o(rx_strb_o)
    ,.last_o(rx_last_o)
    ,.crc_valid_o(rx_crc_valid_o)
);

//-----------------------------------------------------------------
// Tx
//-----------------------------------------------------------------
reg tx_clk_rst_q = 1'b1;

always @ (posedge mii_tx_clk_i)
    tx_clk_rst_q <= 1'b0;

enet_mii_tx
u_tx
(
     .clk_i(mii_tx_clk_i)
    ,.rst_i(tx_clk_rst_q)

    ,.mii_txd_o(mii_txd_o)
    ,.mii_tx_en_o(mii_tx_en_o)

    ,.wr_clk_i(clk_i)
    ,.wr_rst_i(rst_i)

    ,.valid_i(tx_valid_i)
    ,.data_i(tx_data_i)
    ,.strb_i(tx_strb_i)
    ,.last_i(tx_last_i)
    ,.accept_o(tx_accept_o)
);


endmodule
//-----------------------------------------------------------------
//               Ethernet MAC 10/100 Mbps Interface
//                            V0.1.0
//               github.com/ultraembedded/core_enet
//                        Copyright 2021
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2021 github.com/ultraembedded
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//-----------------------------------------------------------------

module enet
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
     parameter PROMISCUOUS      = 1
    ,parameter DEFAULT_MAC_ADDR_H = 0
    ,parameter DEFAULT_MAC_ADDR_L = 0
)
//-----------------------------------------------------------------
// Ports
//-----------------------------------------------------------------
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input  [ 31:0]  cfg_addr_i
    ,input  [ 31:0]  cfg_data_wr_i
    ,input           cfg_stb_i
    ,input           cfg_we_i
    ,input           mii_rx_clk_i
    ,input  [  3:0]  mii_rxd_i
    ,input           mii_rx_dv_i
    ,input           mii_rx_er_i
    ,input           mii_tx_clk_i
    ,input           mii_col_i
    ,input           mii_crs_i

    // Outputs
    ,output [ 31:0]  cfg_data_rd_o
    ,output          cfg_ack_o
    ,output          cfg_stall_o
    ,output          intr_o
    ,output [  3:0]  mii_txd_o
    ,output          mii_tx_en_o
    ,output          mii_reset_n_o
);




//-----------------------------------------------------------------
// MII Interface
//-----------------------------------------------------------------
wire          busy_w;

wire          tx_valid_w;
wire [ 31:0]  tx_data_w;
wire [  3:0]  tx_strb_w;
wire          tx_last_w;
wire          tx_ready_w;

wire          rx_valid_w;
wire [31:0]   rx_data_w;
wire [3:0]    rx_strb_w;
wire          rx_last_w;
wire          rx_crc_valid_w;

enet_mii
u_mii
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    ,.mii_rx_clk_i(mii_rx_clk_i)
    ,.mii_rxd_i(mii_rxd_i)
    ,.mii_rx_dv_i(mii_rx_dv_i)
    ,.mii_rx_er_i(mii_rx_er_i)
    ,.mii_tx_clk_i(mii_tx_clk_i)
    ,.mii_col_i(mii_col_i)
    ,.mii_crs_i(mii_crs_i)
    ,.mii_txd_o(mii_txd_o)
    ,.mii_tx_en_o(mii_tx_en_o)
    ,.mii_reset_n_o(mii_reset_n_o)

    ,.busy_o(busy_w)

    ,.tx_valid_i(tx_valid_w)
    ,.tx_data_i(tx_data_w)
    ,.tx_strb_i(tx_strb_w)
    ,.tx_last_i(tx_last_w)
    ,.tx_accept_o(tx_ready_w)

    ,.rx_valid_o(rx_valid_w)
    ,.rx_data_o(rx_data_w)
    ,.rx_strb_o(rx_strb_w)
    ,.rx_last_o(rx_last_w)
    ,.rx_crc_valid_o(rx_crc_valid_w)
);

//-----------------------------------------------------------------
// Transmit MAC
//-----------------------------------------------------------------
wire [31:0]   tx_cfg_data_rd_w;
wire          tx_interrupt_w;

wire          gie_w;

wire          mac_set_w;
wire [47:0]   mac_addr_w;

enet_mac_tx
u_mac_tx
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    ,.cfg_wr_i(cfg_stb_i && cfg_we_i && !cfg_stall_o)
    ,.cfg_addr_i(cfg_addr_i)
    ,.cfg_data_wr_i(cfg_data_wr_i)
    ,.cfg_data_rd_o(tx_cfg_data_rd_w)

    ,.glbl_irq_en_o(gie_w)

    ,.interrupt_o(tx_interrupt_w)

    ,.mac_update_o(mac_set_w)
    ,.mac_addr_o(mac_addr_w)

    ,.outport_tvalid_o(tx_valid_w)
    ,.outport_tdata_o(tx_data_w)
    ,.outport_tstrb_o(tx_strb_w)
    ,.outport_tlast_o(tx_last_w)
    ,.outport_tready_i(tx_ready_w)
);

//-----------------------------------------------------------------
// MAC Address
//-----------------------------------------------------------------
reg [47:0] mac_addr_q;

always @ (posedge clk_i )
if (rst_i)
    mac_addr_q <= {DEFAULT_MAC_ADDR_H[15:0], DEFAULT_MAC_ADDR_L[31:0]};
else if (mac_set_w)
    mac_addr_q <= mac_addr_w;

//-----------------------------------------------------------------
// Rx
//-----------------------------------------------------------------
wire [31:0] rx_cfg_data_rd_w;
wire [31:0] rx_ram_rd_w;
wire        rx_interrupt_w;

enet_mac_rx
u_mac_rx
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    ,.cfg_wr_i(cfg_stb_i && cfg_we_i && !cfg_stall_o)
    ,.cfg_addr_i(cfg_addr_i)
    ,.cfg_data_wr_i(cfg_data_wr_i)
    ,.cfg_data_rd_o(rx_cfg_data_rd_w)
    ,.ram_data_rd_o(rx_ram_rd_w)

    ,.interrupt_o(rx_interrupt_w)

    ,.promiscuous_i(PROMISCUOUS)
    ,.mac_addr_i(mac_addr_q)

    ,.inport_tvalid_i(rx_valid_w)
    ,.inport_tdata_i(rx_data_w)
    ,.inport_tstrb_i(rx_strb_w)
    ,.inport_tlast_i(rx_last_w)
    ,.inport_crc_valid_i(rx_crc_valid_w)
);

//-----------------------------------------------------------------
// Interrupt Output
//-----------------------------------------------------------------
reg interrupt_q;

always @ (posedge clk_i )
if (rst_i)
    interrupt_q <= 1'b0;
else
    interrupt_q <= gie_w && (tx_interrupt_w || rx_interrupt_w);

assign intr_o = interrupt_q;

//-----------------------------------------------------------------
// Response
//-----------------------------------------------------------------
reg ack_q;

always @ (posedge clk_i )
if (rst_i)
    ack_q <= 1'b0;
else
    ack_q <= cfg_stb_i && !cfg_stall_o;

assign cfg_ack_o   = ack_q;
assign cfg_stall_o = ack_q | busy_w;

reg rd_from_ram_q;

always @ (posedge clk_i )
if (rst_i)
    rd_from_ram_q <= 1'b0;
else if (cfg_stb_i && !cfg_stall_o)
    rd_from_ram_q <= (cfg_addr_i[15:0] >= 16'h1000 && cfg_addr_i[15:0] < 16'h1700) ||
                     (cfg_addr_i[15:0] >= 16'h1800 && cfg_addr_i[15:0] < 16'h1F00);

reg [31:0] data_q;

always @ (posedge clk_i )
if (rst_i)
    data_q <= 32'b0;
else if (cfg_stb_i && !cfg_stall_o && cfg_addr_i[15:0] < 16'h1000)
    data_q <= tx_cfg_data_rd_w;
else if (cfg_stb_i && !cfg_stall_o)
    data_q <= rx_cfg_data_rd_w;

reg [31:0] read_data_r;

always @ *
begin
    read_data_r = 32'b0;

    // Rx RAM
    if (rd_from_ram_q)
        read_data_r = rx_ram_rd_w;
    // Registers
    else
        read_data_r = data_q;
end

assign cfg_data_rd_o = read_data_r;


endmodule
