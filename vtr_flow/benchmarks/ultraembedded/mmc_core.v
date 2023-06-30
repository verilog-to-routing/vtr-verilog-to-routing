//-----------------------------------------------------------------
//                MMC (and derivative standards) Host
//                            V0.1
//                     Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2020 Ultra-Embedded.com
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

`define MMC_CONTROL    8'h0

    `define MMC_CONTROL_START      31
    `define MMC_CONTROL_START_DEFAULT    0
    `define MMC_CONTROL_START_B          31
    `define MMC_CONTROL_START_T          31
    `define MMC_CONTROL_START_W          1
    `define MMC_CONTROL_START_R          31:31

    `define MMC_CONTROL_ABORT      30
    `define MMC_CONTROL_ABORT_DEFAULT    0
    `define MMC_CONTROL_ABORT_B          30
    `define MMC_CONTROL_ABORT_T          30
    `define MMC_CONTROL_ABORT_W          1
    `define MMC_CONTROL_ABORT_R          30:30

    `define MMC_CONTROL_FIFO_RST      29
    `define MMC_CONTROL_FIFO_RST_DEFAULT    0
    `define MMC_CONTROL_FIFO_RST_B          29
    `define MMC_CONTROL_FIFO_RST_T          29
    `define MMC_CONTROL_FIFO_RST_W          1
    `define MMC_CONTROL_FIFO_RST_R          29:29

    `define MMC_CONTROL_BLOCK_CNT_DEFAULT    0
    `define MMC_CONTROL_BLOCK_CNT_B          8
    `define MMC_CONTROL_BLOCK_CNT_T          15
    `define MMC_CONTROL_BLOCK_CNT_W          8
    `define MMC_CONTROL_BLOCK_CNT_R          15:8

    `define MMC_CONTROL_WRITE      5
    `define MMC_CONTROL_WRITE_DEFAULT    0
    `define MMC_CONTROL_WRITE_B          5
    `define MMC_CONTROL_WRITE_T          5
    `define MMC_CONTROL_WRITE_W          1
    `define MMC_CONTROL_WRITE_R          5:5

    `define MMC_CONTROL_DMA_EN      4
    `define MMC_CONTROL_DMA_EN_DEFAULT    0
    `define MMC_CONTROL_DMA_EN_B          4
    `define MMC_CONTROL_DMA_EN_T          4
    `define MMC_CONTROL_DMA_EN_W          1
    `define MMC_CONTROL_DMA_EN_R          4:4

    `define MMC_CONTROL_WIDE_MODE      3
    `define MMC_CONTROL_WIDE_MODE_DEFAULT    0
    `define MMC_CONTROL_WIDE_MODE_B          3
    `define MMC_CONTROL_WIDE_MODE_T          3
    `define MMC_CONTROL_WIDE_MODE_W          1
    `define MMC_CONTROL_WIDE_MODE_R          3:3

    `define MMC_CONTROL_DATA_EXP      2
    `define MMC_CONTROL_DATA_EXP_DEFAULT    0
    `define MMC_CONTROL_DATA_EXP_B          2
    `define MMC_CONTROL_DATA_EXP_T          2
    `define MMC_CONTROL_DATA_EXP_W          1
    `define MMC_CONTROL_DATA_EXP_R          2:2

    `define MMC_CONTROL_RESP136_EXP      1
    `define MMC_CONTROL_RESP136_EXP_DEFAULT    0
    `define MMC_CONTROL_RESP136_EXP_B          1
    `define MMC_CONTROL_RESP136_EXP_T          1
    `define MMC_CONTROL_RESP136_EXP_W          1
    `define MMC_CONTROL_RESP136_EXP_R          1:1

    `define MMC_CONTROL_RESP48_EXP      0
    `define MMC_CONTROL_RESP48_EXP_DEFAULT    0
    `define MMC_CONTROL_RESP48_EXP_B          0
    `define MMC_CONTROL_RESP48_EXP_T          0
    `define MMC_CONTROL_RESP48_EXP_W          1
    `define MMC_CONTROL_RESP48_EXP_R          0:0

`define MMC_CLOCK    8'h4

    `define MMC_CLOCK_DIV_DEFAULT    8
    `define MMC_CLOCK_DIV_B          0
    `define MMC_CLOCK_DIV_T          7
    `define MMC_CLOCK_DIV_W          8
    `define MMC_CLOCK_DIV_R          7:0

`define MMC_STATUS    8'h8

    `define MMC_STATUS_CMD_IN      8
    `define MMC_STATUS_CMD_IN_DEFAULT    0
    `define MMC_STATUS_CMD_IN_B          8
    `define MMC_STATUS_CMD_IN_T          8
    `define MMC_STATUS_CMD_IN_W          1
    `define MMC_STATUS_CMD_IN_R          8:8

    `define MMC_STATUS_DAT_IN_DEFAULT    0
    `define MMC_STATUS_DAT_IN_B          4
    `define MMC_STATUS_DAT_IN_T          7
    `define MMC_STATUS_DAT_IN_W          4
    `define MMC_STATUS_DAT_IN_R          7:4

    `define MMC_STATUS_FIFO_EMPTY      3
    `define MMC_STATUS_FIFO_EMPTY_DEFAULT    0
    `define MMC_STATUS_FIFO_EMPTY_B          3
    `define MMC_STATUS_FIFO_EMPTY_T          3
    `define MMC_STATUS_FIFO_EMPTY_W          1
    `define MMC_STATUS_FIFO_EMPTY_R          3:3

    `define MMC_STATUS_FIFO_FULL      2
    `define MMC_STATUS_FIFO_FULL_DEFAULT    0
    `define MMC_STATUS_FIFO_FULL_B          2
    `define MMC_STATUS_FIFO_FULL_T          2
    `define MMC_STATUS_FIFO_FULL_W          1
    `define MMC_STATUS_FIFO_FULL_R          2:2

    `define MMC_STATUS_CRC_ERR      1
    `define MMC_STATUS_CRC_ERR_DEFAULT    0
    `define MMC_STATUS_CRC_ERR_B          1
    `define MMC_STATUS_CRC_ERR_T          1
    `define MMC_STATUS_CRC_ERR_W          1
    `define MMC_STATUS_CRC_ERR_R          1:1

    `define MMC_STATUS_BUSY      0
    `define MMC_STATUS_BUSY_DEFAULT    0
    `define MMC_STATUS_BUSY_B          0
    `define MMC_STATUS_BUSY_T          0
    `define MMC_STATUS_BUSY_W          1
    `define MMC_STATUS_BUSY_R          0:0

`define MMC_CMD0    8'hc

    `define MMC_CMD0_VALUE_DEFAULT    0
    `define MMC_CMD0_VALUE_B          0
    `define MMC_CMD0_VALUE_T          31
    `define MMC_CMD0_VALUE_W          32
    `define MMC_CMD0_VALUE_R          31:0

`define MMC_CMD1    8'h10

    `define MMC_CMD1_VALUE_DEFAULT    0
    `define MMC_CMD1_VALUE_B          0
    `define MMC_CMD1_VALUE_T          15
    `define MMC_CMD1_VALUE_W          16
    `define MMC_CMD1_VALUE_R          15:0

`define MMC_RESP0    8'h14

    `define MMC_RESP0_VALUE_DEFAULT    0
    `define MMC_RESP0_VALUE_B          0
    `define MMC_RESP0_VALUE_T          31
    `define MMC_RESP0_VALUE_W          32
    `define MMC_RESP0_VALUE_R          31:0

`define MMC_RESP1    8'h18

    `define MMC_RESP1_VALUE_DEFAULT    0
    `define MMC_RESP1_VALUE_B          0
    `define MMC_RESP1_VALUE_T          31
    `define MMC_RESP1_VALUE_W          32
    `define MMC_RESP1_VALUE_R          31:0

`define MMC_RESP2    8'h1c

    `define MMC_RESP2_VALUE_DEFAULT    0
    `define MMC_RESP2_VALUE_B          0
    `define MMC_RESP2_VALUE_T          31
    `define MMC_RESP2_VALUE_W          32
    `define MMC_RESP2_VALUE_R          31:0

`define MMC_RESP3    8'h20

    `define MMC_RESP3_VALUE_DEFAULT    0
    `define MMC_RESP3_VALUE_B          0
    `define MMC_RESP3_VALUE_T          31
    `define MMC_RESP3_VALUE_W          32
    `define MMC_RESP3_VALUE_R          31:0

`define MMC_RESP4    8'h24

    `define MMC_RESP4_VALUE_DEFAULT    0
    `define MMC_RESP4_VALUE_B          0
    `define MMC_RESP4_VALUE_T          31
    `define MMC_RESP4_VALUE_W          32
    `define MMC_RESP4_VALUE_R          31:0

`define MMC_TX    8'h28

    `define MMC_TX_DATA_DEFAULT    0
    `define MMC_TX_DATA_B          0
    `define MMC_TX_DATA_T          31
    `define MMC_TX_DATA_W          32
    `define MMC_TX_DATA_R          31:0

`define MMC_RX    8'h2c

    `define MMC_RX_DATA_DEFAULT    0
    `define MMC_RX_DATA_B          0
    `define MMC_RX_DATA_T          31
    `define MMC_RX_DATA_W          32
    `define MMC_RX_DATA_R          31:0

`define MMC_DMA    8'h30

    `define MMC_DMA_ADDR_DEFAULT    0
    `define MMC_DMA_ADDR_B          0
    `define MMC_DMA_ADDR_T          31
    `define MMC_DMA_ADDR_W          32
    `define MMC_DMA_ADDR_R          31:0

//-----------------------------------------------------------------
//                MMC (and derivative standards) Host
//                            V0.1
//                     Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2020 Ultra-Embedded.com
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

module mmc_card_fifo
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input  [ 31:0]  data_in_i
    ,input           push_i
    ,input           pop_i
    ,input           flush_i

    // Outputs
    ,output [ 31:0]  data_out_o
    ,output          accept_o
    ,output          valid_o
    ,output [ 10:0]  level_o
);



//-----------------------------------------------------------------
// Registers
//-----------------------------------------------------------------
reg [9:0]   rd_ptr_q;
reg [9:0]   wr_ptr_q;

//-----------------------------------------------------------------
// Write Side
//-----------------------------------------------------------------
wire [9:0] write_next_w = wr_ptr_q + 10'd1;

wire full_w = (write_next_w == rd_ptr_q);

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    wr_ptr_q <= 10'b0;
else if (flush_i)
    wr_ptr_q <= 10'b0;
// Push
else if (push_i & !full_w)
    wr_ptr_q <= write_next_w;

//-----------------------------------------------------------------
// Read Side
//-----------------------------------------------------------------
wire read_ok_w = (wr_ptr_q != rd_ptr_q);
reg  rd_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    rd_q <= 1'b0;
else if (flush_i)
    rd_q <= 1'b0;
else
    rd_q <= read_ok_w;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    rd_ptr_q     <= 10'b0;
else if (flush_i)
    rd_ptr_q     <= 10'b0;
// Read address increment
else if (read_ok_w && ((!valid_o) || (valid_o && pop_i)))
    rd_ptr_q <= rd_ptr_q + 10'd1;

//-------------------------------------------------------------------
// Read Skid Buffer
//-------------------------------------------------------------------
reg                rd_skid_q;
reg [31:0] rd_skid_data_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
begin
    rd_skid_q <= 1'b0;
    rd_skid_data_q <= 32'b0;
end
else if (flush_i)
begin
    rd_skid_q <= 1'b0;
    rd_skid_data_q <= 32'b0;
end
else if (valid_o && !pop_i)
begin
    rd_skid_q      <= 1'b1;
    rd_skid_data_q <= data_out_o;
end
else
begin
    rd_skid_q      <= 1'b0;
    rd_skid_data_q <= 32'b0;
end

//-------------------------------------------------------------------
// Combinatorial
//-------------------------------------------------------------------
assign valid_o       = rd_skid_q | rd_q;
assign accept_o      = !full_w;

//-------------------------------------------------------------------
// Dual port RAM
//-------------------------------------------------------------------
wire [31:0] data_out_w;

mmc_card_fifo_ram_dp_1024_10
u_ram
(
    // Inputs
    .clk0_i(clk_i),
    .rst0_i(rst_i),
    .clk1_i(clk_i),
    .rst1_i(rst_i),

    // Write side
    .addr0_i(wr_ptr_q),
    .wr0_i(push_i & accept_o),
    .data0_i(data_in_i),
    .data0_o(),

    // Read side
    .addr1_i(rd_ptr_q),
    .data1_i(32'b0),
    .wr1_i(1'b0),
    .data1_o(data_out_w)
);

assign data_out_o = rd_skid_q ? rd_skid_data_q : data_out_w;


//-------------------------------------------------------------------
// Level
//-------------------------------------------------------------------
reg [10:0]  count_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    count_q   <= 11'b0;
else if (flush_i)
    count_q   <= 11'b0;
// Count up
else if ((push_i & accept_o) & ~(pop_i & valid_o))
    count_q <= count_q + 11'd1;
// Count down
else if (~(push_i & accept_o) & (pop_i & valid_o))
    count_q <= count_q - 11'd1;

assign level_o = count_q;

endmodule

//-------------------------------------------------------------------
// Dual port RAM
//-------------------------------------------------------------------
module mmc_card_fifo_ram_dp_1024_10
(
    // Inputs
     input           clk0_i
    ,input           rst0_i
    ,input  [ 9:0]  addr0_i
    ,input  [ 31:0]  data0_i
    ,input           wr0_i
    ,input           clk1_i
    ,input           rst1_i
    ,input  [ 9:0]  addr1_i
    ,input  [ 31:0]  data1_i
    ,input           wr1_i

    // Outputs
    ,output [ 31:0]  data0_o
    ,output [ 31:0]  data1_o
);

/* verilator lint_off MULTIDRIVEN */
reg [31:0]   ram [1023:0] /*verilator public*/;
/* verilator lint_on MULTIDRIVEN */

reg [31:0] ram_read0_q;
reg [31:0] ram_read1_q;

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
//-----------------------------------------------------------------
//                MMC (and derivative standards) Host
//                            V0.1
//                     Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2020 Ultra-Embedded.com
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
//-----------------------------------------------------------------
// Module: Deserialiser
//-----------------------------------------------------------------
module mmc_cmd_deserialiser
(
     input          clk_i
    ,input          rst_i
    ,input          bitclk_i
    ,input          start_i
    ,input          abort_i
    ,input          r2_mode_i
    ,input          data_i
    ,output [135:0] resp_o
    ,output         active_o
    ,output         complete_o
);

reg clk_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    clk_q <= 1'b0;
else
    clk_q <= bitclk_i;

// Capture on rising edge
wire capture_w = bitclk_i & ~clk_q;

reg [7:0] index_q;

localparam STATE_W          = 3;

// Current state
localparam STATE_IDLE       = 3'd0;
localparam STATE_STARTED    = 3'd1;
localparam STATE_ACTIVE     = 3'd2;
localparam STATE_END        = 3'd3;

reg [STATE_W-1:0] state_q;
reg [STATE_W-1:0] next_state_r;

always @ *
begin
    next_state_r = state_q;

    case (state_q)
    STATE_IDLE :
    begin
        if (start_i)
            next_state_r  = STATE_STARTED;
    end
    STATE_STARTED:
    begin
        if (capture_w && !data_i)
            next_state_r  = STATE_ACTIVE;
    end
    STATE_ACTIVE:
    begin
        if ((index_q == 8'd0) & capture_w)
            next_state_r  = STATE_END;
    end
    STATE_END:
    begin
        next_state_r  = STATE_IDLE;
    end
    default :
        ;
    endcase

    if (abort_i)
        next_state_r  = STATE_IDLE;
end

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    state_q <= STATE_IDLE;
else
    state_q <= next_state_r;


always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    index_q <= 8'd47;
else if (start_i)
    index_q <= r2_mode_i ? 8'd135: 8'd46;
else if (capture_w && state_q == STATE_ACTIVE)
    index_q <= index_q - 8'd1;

reg [135:0] resp_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    resp_q <= 136'b0;
else if (state_q == STATE_IDLE && start_i)
    resp_q <= 136'b0;
else if (state_q == STATE_ACTIVE && capture_w)
    resp_q <= {resp_q[134:0], data_i};

assign active_o   = (state_q != STATE_IDLE);
assign complete_o = (state_q == STATE_END);
assign resp_o     = resp_q;

endmodule//-----------------------------------------------------------------
//                MMC (and derivative standards) Host
//                            V0.1
//                     Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2020 Ultra-Embedded.com
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
//-----------------------------------------------------------------
// Module: Serialiser
//-----------------------------------------------------------------
module mmc_cmd_serialiser
(
     input        clk_i
    ,input        rst_i
    ,input        bitclk_i
    ,input        start_i
    ,input        abort_i
    ,input [47:0] data_i
    ,output       cmd_o
    ,output       active_o
    ,output       complete_o
);

reg clk_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    clk_q <= 1'b0;
else
    clk_q <= bitclk_i;

// Drive on falling edge
wire drive_w = ~bitclk_i & clk_q;

reg [7:0] index_q;

localparam STATE_W          = 3;

// Current state
localparam STATE_IDLE       = 3'd0;
localparam STATE_STARTED    = 3'd1;
localparam STATE_ACTIVE     = 3'd2;
localparam STATE_END        = 3'd3;

reg [STATE_W-1:0] state_q;
reg [STATE_W-1:0] next_state_r;

always @ *
begin
    next_state_r = state_q;

    case (state_q)
    STATE_IDLE :
    begin
        if (start_i)
            next_state_r  = STATE_STARTED;
    end
    STATE_STARTED:
    begin
        if (drive_w)
            next_state_r  = STATE_ACTIVE;
    end
    STATE_ACTIVE:
    begin
        if ((index_q == 8'd0) & drive_w)
            next_state_r  = STATE_END;   
    end
    STATE_END:
    begin
        if (drive_w)
            next_state_r  = STATE_IDLE;
    end
    default :
        ;
    endcase

    if (abort_i)
        next_state_r  = STATE_IDLE;
end

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    state_q <= STATE_IDLE;
else
    state_q <= next_state_r;


always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    index_q <= 8'd47;
else if (start_i)
    index_q <= 8'd47;
else if (drive_w && state_q == STATE_ACTIVE)
    index_q <= index_q - 8'd1;

wire [6:0] crc7_w;
mmc_crc7
u_crc7
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.clear_i(state_q == STATE_IDLE)
    ,.bitval_i(data_i[index_q[5:0]])
    ,.enable_i(drive_w && (state_q == STATE_ACTIVE) && index_q > 8'd7)
    ,.crc_o(crc7_w)
);

reg cmd_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    cmd_q <= 1'b1;
else if (drive_w && state_q == STATE_ACTIVE)
begin
    if (index_q > 8'd7)
        cmd_q <= data_i[index_q[5:0]];
    else if (index_q == 8'd0)
        cmd_q <= 1'b1;
    else
        cmd_q <= crc7_w[index_q[5:0]-1];
end
else if (complete_o)
    cmd_q <= 1'b1;

assign cmd_o      = cmd_q;
assign active_o   = (state_q != STATE_IDLE);
assign complete_o = (state_q == STATE_END) & drive_w;

endmodule
//-----------------------------------------------------------------
//                MMC (and derivative standards) Host
//                            V0.1
//                     Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2020 Ultra-Embedded.com
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
//-----------------------------------------------------------------
// Module: CRC7
//-----------------------------------------------------------------
module mmc_crc7
(
     input        clk_i
    ,input        rst_i
    ,input        clear_i    
    ,input        bitval_i
    ,input        enable_i
    ,output [6:0] crc_o
);

reg    [6:0] crc_q;   

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    crc_q <= 7'b0;
else if (clear_i)
    crc_q <= 7'b0;
else if (enable_i)
begin
    crc_q[6] <= crc_q[5];
    crc_q[5] <= crc_q[4];
    crc_q[4] <= crc_q[3];
    crc_q[3] <= crc_q[2] ^ (bitval_i ^ crc_q[6]);
    crc_q[2] <= crc_q[1];
    crc_q[1] <= crc_q[0];
    crc_q[0] <= bitval_i ^ crc_q[6];
end

assign crc_o = crc_q;

endmodule
//-----------------------------------------------------------------
//                MMC (and derivative standards) Host
//                            V0.1
//                     Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2020 Ultra-Embedded.com
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
//-----------------------------------------------------------------
// Module: CRC16
//-----------------------------------------------------------------
module mmc_crc16
(
     input        clk_i
    ,input        rst_i
    ,input        clear_i    
    ,input        bitval_i
    ,input        enable_i
    ,output [15:0] crc_o
);

reg [15:0] crc_q;   

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    crc_q <= 16'b0;
else if (clear_i)
    crc_q <= 16'b0;
else if (enable_i)
begin
    crc_q[15] <= crc_q[14];
    crc_q[14] <= crc_q[13];
    crc_q[13] <= crc_q[12];
    crc_q[12] <= crc_q[11] ^ (bitval_i ^ crc_q[15]);
    crc_q[11] <= crc_q[10];
    crc_q[10] <= crc_q[9];
    crc_q[9]  <= crc_q[8];
    crc_q[8]  <= crc_q[7];
    crc_q[7]  <= crc_q[6];
    crc_q[6]  <= crc_q[5];
    crc_q[5]  <= crc_q[4] ^ (bitval_i ^ crc_q[15]);
    crc_q[4]  <= crc_q[3];
    crc_q[3]  <= crc_q[2];
    crc_q[2]  <= crc_q[1];
    crc_q[1]  <= crc_q[0];
    crc_q[0]  <= bitval_i ^ crc_q[15];
end

assign crc_o = crc_q;

endmodule//-----------------------------------------------------------------
//                MMC (and derivative standards) Host
//                            V0.1
//                     Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2020 Ultra-Embedded.com
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
//-----------------------------------------------------------------
// Module: Deserialiser
//-----------------------------------------------------------------
module mmc_dat_deserialiser
(
     input          clk_i
    ,input          rst_i
    ,input          bitclk_i
    ,input          start_i
    ,input          abort_i
    ,input          data_i
    ,input          mode_4bit_i
    ,input  [7:0]   block_cnt_i
    ,output         valid_o
    ,output [7:0]   data_o
    ,output         active_o
    ,output         error_o
    ,output         complete_o
);

reg clk_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    clk_q <= 1'b0;
else
    clk_q <= bitclk_i;

// Capture on rising edge
wire capture_w = bitclk_i & ~clk_q;

reg [15:0] index_q;

localparam STATE_W          = 3;

// Current state
localparam STATE_IDLE       = 3'd0;
localparam STATE_STARTED    = 3'd1;
localparam STATE_ACTIVE     = 3'd2;
localparam STATE_END        = 3'd3;

reg [STATE_W-1:0] state_q;
reg [STATE_W-1:0] next_state_r;
reg [7:0]         block_cnt_q;

always @ *
begin
    next_state_r = state_q;

    case (state_q)
    STATE_IDLE :
    begin
        if (start_i)
            next_state_r  = STATE_STARTED;
    end
    STATE_STARTED:
    begin
        if (capture_w && !data_i)
            next_state_r  = STATE_ACTIVE;
    end
    STATE_ACTIVE:
    begin
        if ((index_q == 16'd0) & capture_w)
        begin
            if (block_cnt_q != 8'd0)
                next_state_r  = STATE_STARTED;
            else
                next_state_r  = STATE_END;
        end
    end
    STATE_END:
    begin
        next_state_r  = STATE_IDLE;
    end
    default :
        ;
    endcase

    if (abort_i)
        next_state_r  = STATE_IDLE;
end

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    state_q <= STATE_IDLE;
else
    state_q <= next_state_r;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    block_cnt_q <= 8'b0;
else if ((state_q == STATE_IDLE) && start_i)
    block_cnt_q <= block_cnt_i;
else if ((state_q == STATE_ACTIVE) && (index_q == 16'd0) && capture_w)
    block_cnt_q <= block_cnt_q - 8'd1;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    index_q <= 16'd0;
else if (state_q == STATE_STARTED)
    index_q <= mode_4bit_i ? 16'd1040: 16'd4112;
else if (capture_w && state_q == STATE_ACTIVE)
    index_q <= index_q - 16'd1;

reg [7:0] data_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    data_q <= 8'b0;
else if (state_q == STATE_STARTED)
    data_q <= 8'b0;
else if (state_q == STATE_ACTIVE && capture_w)
    data_q <= {data_q[6:0], data_i};

reg [2:0] bitcnt_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    bitcnt_q <= 3'b0;
else if (state_q == STATE_STARTED)
    bitcnt_q <= 3'b0;
else if (state_q == STATE_ACTIVE && capture_w)
    bitcnt_q <= bitcnt_q + 3'd1;

reg valid_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    valid_q <= 1'b0;
else
    valid_q <= (state_q == STATE_ACTIVE && capture_w && bitcnt_q == 3'd7 && index_q > 16'd15);

assign active_o   = (state_q != STATE_IDLE);
assign complete_o = (state_q == STATE_END);
assign valid_o    = valid_q;
assign data_o     = data_q;
assign error_o    = 1'b0; // TODO: Add CRC checking

endmodule//-----------------------------------------------------------------
//                MMC (and derivative standards) Host
//                            V0.1
//                     Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2020 Ultra-Embedded.com
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
//-----------------------------------------------------------------
// Module: Serialiser
//-----------------------------------------------------------------
module mmc_dat_serialiser
(
     input          clk_i
    ,input          rst_i
    ,input          bitclk_i
    ,input          start_i
    ,input          abort_i
    ,input  [7:0]   data_i
    ,output         accept_o
    ,input          mode_4bit_i
    ,input  [7:0]   block_cnt_i
    ,output         dat_o
    ,output         dat_out_en_o
    ,output         active_o
    ,output         complete_o
);

reg clk_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    clk_q <= 1'b0;
else
    clk_q <= bitclk_i;

// Drive on falling edge
wire drive_w = ~bitclk_i & clk_q;

reg [15:0] index_q;
reg [2:0]  bitcnt_q;

localparam STATE_W          = 3;

// Current state
localparam STATE_IDLE       = 3'd0;
localparam STATE_DELAY      = 3'd1;
localparam STATE_STARTED    = 3'd2;
localparam STATE_ACTIVE     = 3'd3;
localparam STATE_END        = 3'd4;

reg [STATE_W-1:0] state_q;
reg [STATE_W-1:0] next_state_r;
reg [7:0]         block_cnt_q;

always @ *
begin
    next_state_r = state_q;

    case (state_q)
    STATE_IDLE :
    begin
        if (start_i)
            next_state_r  = STATE_DELAY;
    end
    STATE_DELAY:
    begin
        if (drive_w && bitcnt_q == 3'd7)
            next_state_r  = STATE_STARTED;
    end
    STATE_STARTED:
    begin
        if (drive_w)
            next_state_r  = STATE_ACTIVE;
    end
    STATE_ACTIVE:
    begin
        if ((index_q == 16'd0) & drive_w)
        begin
            //if (block_cnt_q != 8'd0)
            //    next_state_r  = STATE_STARTED;
            //else
                next_state_r  = STATE_END;
        end
    end
    STATE_END:
    begin
        if (drive_w)
            next_state_r  = STATE_IDLE;
    end
    default :
        ;
    endcase

    if (abort_i)
        next_state_r  = STATE_IDLE;
end

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    state_q <= STATE_IDLE;
else
    state_q <= next_state_r;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    block_cnt_q <= 8'b0;
else if ((state_q == STATE_IDLE) && start_i)
    block_cnt_q <= block_cnt_i;
else if ((state_q == STATE_ACTIVE) && (index_q == 16'd0) && drive_w)
    block_cnt_q <= block_cnt_q - 8'd1;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    index_q <= 16'd0;
else if (state_q == STATE_STARTED)
    index_q <= mode_4bit_i ? 16'd1040: 16'd4112;
else if (drive_w && state_q == STATE_ACTIVE)
    index_q <= index_q - 16'd1;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    bitcnt_q <= 3'b0;
else if (state_q == STATE_STARTED)
    bitcnt_q <= 3'b0;
else if (state_q == STATE_DELAY && drive_w)
    bitcnt_q <= bitcnt_q + 3'd1;    
else if (state_q == STATE_ACTIVE && drive_w)
    bitcnt_q <= bitcnt_q + 3'd1;

reg [7:0] data_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    data_q <= 8'b0;
else if (accept_o)
    data_q <= data_i;
else if (state_q == STATE_ACTIVE && drive_w)
    data_q <= {data_q[6:0], 1'b0};

assign accept_o = ((state_q == STATE_IDLE) && start_i) || (state_q == STATE_ACTIVE && drive_w && bitcnt_q == 3'd7);

wire [15:0] crc_w;

mmc_crc16
u_crc16
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    
    ,.clear_i(state_q == STATE_IDLE)
    ,.bitval_i(data_q[7])
    ,.enable_i(drive_w && state_q == STATE_ACTIVE && index_q > 16'd16)
    ,.crc_o(crc_w)
);

reg dat_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    dat_q <= 1'b1;
else if (drive_w && state_q == STATE_STARTED)
    dat_q <= 1'b0;
else if (drive_w && state_q == STATE_ACTIVE)
begin
    if (index_q > 16'd16)
        dat_q <= data_q[7];
    else
        dat_q <= crc_w[index_q - 1];
end
else if (complete_o)
    dat_q <= 1'b1;

assign dat_o        = dat_q;
assign dat_out_en_o = (state_q != STATE_IDLE);

assign active_o   = (state_q != STATE_IDLE);
assign complete_o = (state_q == STATE_END);

endmodule//-----------------------------------------------------------------
//                MMC (and derivative standards) Host
//                            V0.1
//                     Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2020 Ultra-Embedded.com
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

//`include "mmc_card_defs.v"

//-----------------------------------------------------------------
// Module:  MMC Peripheral
//-----------------------------------------------------------------
module mmc_card
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
     parameter MMC_FIFO_DEPTH   = 1024
    ,parameter MMC_FIFO_DEPTH_W = 10
    ,parameter AXI_ID           = 0
)
//-----------------------------------------------------------------
// Ports
//-----------------------------------------------------------------
(
    // Inputs
     input          clk_i
    ,input          rst_i
    ,input          cfg_awvalid_i
    ,input  [31:0]  cfg_awaddr_i
    ,input          cfg_wvalid_i
    ,input  [31:0]  cfg_wdata_i
    ,input  [3:0]   cfg_wstrb_i
    ,input          cfg_bready_i
    ,input          cfg_arvalid_i
    ,input  [31:0]  cfg_araddr_i
    ,input          cfg_rready_i
    ,input          mmc_cmd_in_i
    ,input  [3:0]   mmc_dat_in_i
    ,input          outport_awready_i
    ,input          outport_wready_i
    ,input          outport_bvalid_i
    ,input  [1:0]   outport_bresp_i
    ,input  [3:0]   outport_bid_i
    ,input          outport_arready_i
    ,input          outport_rvalid_i
    ,input  [31:0]  outport_rdata_i
    ,input  [1:0]   outport_rresp_i
    ,input  [3:0]   outport_rid_i
    ,input          outport_rlast_i

    // Outputs
    ,output         cfg_awready_o
    ,output         cfg_wready_o
    ,output         cfg_bvalid_o
    ,output [1:0]   cfg_bresp_o
    ,output         cfg_arready_o
    ,output         cfg_rvalid_o
    ,output [31:0]  cfg_rdata_o
    ,output [1:0]   cfg_rresp_o
    ,output         mmc_clk_o
    ,output         mmc_cmd_out_o
    ,output         mmc_cmd_out_en_o
    ,output [3:0]   mmc_dat_out_o
    ,output [3:0]   mmc_dat_out_en_o
    ,output         intr_o
    ,output         outport_awvalid_o
    ,output [31:0]  outport_awaddr_o
    ,output [3:0]   outport_awid_o
    ,output [7:0]   outport_awlen_o
    ,output [1:0]   outport_awburst_o
    ,output         outport_wvalid_o
    ,output [31:0]  outport_wdata_o
    ,output [3:0]   outport_wstrb_o
    ,output         outport_wlast_o
    ,output         outport_bready_o
    ,output         outport_arvalid_o
    ,output [31:0]  outport_araddr_o
    ,output [3:0]   outport_arid_o
    ,output [7:0]   outport_arlen_o
    ,output [1:0]   outport_arburst_o
    ,output         outport_rready_o
);

//-----------------------------------------------------------------
// Write address / data split
//-----------------------------------------------------------------
// Address but no data ready
reg awvalid_q;

// Data but no data ready
reg wvalid_q;

wire wr_cmd_accepted_w  = (cfg_awvalid_i && cfg_awready_o) || awvalid_q;
wire wr_data_accepted_w = (cfg_wvalid_i  && cfg_wready_o)  || wvalid_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    awvalid_q <= 1'b0;
else if (cfg_awvalid_i && cfg_awready_o && !wr_data_accepted_w)
    awvalid_q <= 1'b1;
else if (wr_data_accepted_w)
    awvalid_q <= 1'b0;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    wvalid_q <= 1'b0;
else if (cfg_wvalid_i && cfg_wready_o && !wr_cmd_accepted_w)
    wvalid_q <= 1'b1;
else if (wr_cmd_accepted_w)
    wvalid_q <= 1'b0;

//-----------------------------------------------------------------
// Capture address (for delayed data)
//-----------------------------------------------------------------
reg [7:0] wr_addr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    wr_addr_q <= 8'b0;
else if (cfg_awvalid_i && cfg_awready_o)
    wr_addr_q <= cfg_awaddr_i[7:0];

wire [7:0] wr_addr_w = awvalid_q ? wr_addr_q : cfg_awaddr_i[7:0];

//-----------------------------------------------------------------
// Retime write data
//-----------------------------------------------------------------
reg [31:0] wr_data_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    wr_data_q <= 32'b0;
else if (cfg_wvalid_i && cfg_wready_o)
    wr_data_q <= cfg_wdata_i;

//-----------------------------------------------------------------
// Request Logic
//-----------------------------------------------------------------
wire read_en_w  = cfg_arvalid_i & cfg_arready_o;
wire write_en_w = wr_cmd_accepted_w && wr_data_accepted_w;

//-----------------------------------------------------------------
// Accept Logic
//-----------------------------------------------------------------
assign cfg_arready_o = ~cfg_rvalid_o;
assign cfg_awready_o = ~cfg_bvalid_o && ~cfg_arvalid_i && ~awvalid_q;
assign cfg_wready_o  = ~cfg_bvalid_o && ~cfg_arvalid_i && ~wvalid_q;


//-----------------------------------------------------------------
// Register mmc_control
//-----------------------------------------------------------------
reg mmc_control_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_control_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `MMC_CONTROL))
    mmc_control_wr_q <= 1'b1;
else
    mmc_control_wr_q <= 1'b0;

// mmc_control_start [auto_clr]
reg        mmc_control_start_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_control_start_q <= 1'd`MMC_CONTROL_START_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `MMC_CONTROL))
    mmc_control_start_q <= cfg_wdata_i[`MMC_CONTROL_START_R];
else
    mmc_control_start_q <= 1'd`MMC_CONTROL_START_DEFAULT;

wire        mmc_control_start_out_w = mmc_control_start_q;


// mmc_control_abort [auto_clr]
reg        mmc_control_abort_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_control_abort_q <= 1'd`MMC_CONTROL_ABORT_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `MMC_CONTROL))
    mmc_control_abort_q <= cfg_wdata_i[`MMC_CONTROL_ABORT_R];
else
    mmc_control_abort_q <= 1'd`MMC_CONTROL_ABORT_DEFAULT;

wire        mmc_control_abort_out_w = mmc_control_abort_q;


// mmc_control_fifo_rst [auto_clr]
reg        mmc_control_fifo_rst_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_control_fifo_rst_q <= 1'd`MMC_CONTROL_FIFO_RST_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `MMC_CONTROL))
    mmc_control_fifo_rst_q <= cfg_wdata_i[`MMC_CONTROL_FIFO_RST_R];
else
    mmc_control_fifo_rst_q <= 1'd`MMC_CONTROL_FIFO_RST_DEFAULT;

wire        mmc_control_fifo_rst_out_w = mmc_control_fifo_rst_q;


// mmc_control_block_cnt [internal]
reg [7:0]  mmc_control_block_cnt_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_control_block_cnt_q <= 8'd`MMC_CONTROL_BLOCK_CNT_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `MMC_CONTROL))
    mmc_control_block_cnt_q <= cfg_wdata_i[`MMC_CONTROL_BLOCK_CNT_R];

wire [7:0]  mmc_control_block_cnt_out_w = mmc_control_block_cnt_q;


// mmc_control_write [internal]
reg        mmc_control_write_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_control_write_q <= 1'd`MMC_CONTROL_WRITE_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `MMC_CONTROL))
    mmc_control_write_q <= cfg_wdata_i[`MMC_CONTROL_WRITE_R];

wire        mmc_control_write_out_w = mmc_control_write_q;


// mmc_control_dma_en [internal]
reg        mmc_control_dma_en_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_control_dma_en_q <= 1'd`MMC_CONTROL_DMA_EN_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `MMC_CONTROL))
    mmc_control_dma_en_q <= cfg_wdata_i[`MMC_CONTROL_DMA_EN_R];

wire        mmc_control_dma_en_out_w = mmc_control_dma_en_q;


// mmc_control_wide_mode [internal]
reg        mmc_control_wide_mode_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_control_wide_mode_q <= 1'd`MMC_CONTROL_WIDE_MODE_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `MMC_CONTROL))
    mmc_control_wide_mode_q <= cfg_wdata_i[`MMC_CONTROL_WIDE_MODE_R];

wire        mmc_control_wide_mode_out_w = mmc_control_wide_mode_q;


// mmc_control_data_exp [internal]
reg        mmc_control_data_exp_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_control_data_exp_q <= 1'd`MMC_CONTROL_DATA_EXP_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `MMC_CONTROL))
    mmc_control_data_exp_q <= cfg_wdata_i[`MMC_CONTROL_DATA_EXP_R];

wire        mmc_control_data_exp_out_w = mmc_control_data_exp_q;


// mmc_control_resp136_exp [internal]
reg        mmc_control_resp136_exp_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_control_resp136_exp_q <= 1'd`MMC_CONTROL_RESP136_EXP_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `MMC_CONTROL))
    mmc_control_resp136_exp_q <= cfg_wdata_i[`MMC_CONTROL_RESP136_EXP_R];

wire        mmc_control_resp136_exp_out_w = mmc_control_resp136_exp_q;


// mmc_control_resp48_exp [internal]
reg        mmc_control_resp48_exp_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_control_resp48_exp_q <= 1'd`MMC_CONTROL_RESP48_EXP_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `MMC_CONTROL))
    mmc_control_resp48_exp_q <= cfg_wdata_i[`MMC_CONTROL_RESP48_EXP_R];

wire        mmc_control_resp48_exp_out_w = mmc_control_resp48_exp_q;


//-----------------------------------------------------------------
// Register mmc_clock
//-----------------------------------------------------------------
reg mmc_clock_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_clock_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `MMC_CLOCK))
    mmc_clock_wr_q <= 1'b1;
else
    mmc_clock_wr_q <= 1'b0;

// mmc_clock_div [internal]
reg [7:0]  mmc_clock_div_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_clock_div_q <= 8'd`MMC_CLOCK_DIV_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `MMC_CLOCK))
    mmc_clock_div_q <= cfg_wdata_i[`MMC_CLOCK_DIV_R];

wire [7:0]  mmc_clock_div_out_w = mmc_clock_div_q;


//-----------------------------------------------------------------
// Register mmc_status
//-----------------------------------------------------------------
reg mmc_status_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_status_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `MMC_STATUS))
    mmc_status_wr_q <= 1'b1;
else
    mmc_status_wr_q <= 1'b0;







//-----------------------------------------------------------------
// Register mmc_cmd0
//-----------------------------------------------------------------
reg mmc_cmd0_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_cmd0_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `MMC_CMD0))
    mmc_cmd0_wr_q <= 1'b1;
else
    mmc_cmd0_wr_q <= 1'b0;

// mmc_cmd0_value [internal]
reg [31:0]  mmc_cmd0_value_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_cmd0_value_q <= 32'd`MMC_CMD0_VALUE_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `MMC_CMD0))
    mmc_cmd0_value_q <= cfg_wdata_i[`MMC_CMD0_VALUE_R];

wire [31:0]  mmc_cmd0_value_out_w = mmc_cmd0_value_q;


//-----------------------------------------------------------------
// Register mmc_cmd1
//-----------------------------------------------------------------
reg mmc_cmd1_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_cmd1_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `MMC_CMD1))
    mmc_cmd1_wr_q <= 1'b1;
else
    mmc_cmd1_wr_q <= 1'b0;

// mmc_cmd1_value [internal]
reg [15:0]  mmc_cmd1_value_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_cmd1_value_q <= 16'd`MMC_CMD1_VALUE_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `MMC_CMD1))
    mmc_cmd1_value_q <= cfg_wdata_i[`MMC_CMD1_VALUE_R];

wire [15:0]  mmc_cmd1_value_out_w = mmc_cmd1_value_q;


//-----------------------------------------------------------------
// Register mmc_resp0
//-----------------------------------------------------------------
reg mmc_resp0_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_resp0_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `MMC_RESP0))
    mmc_resp0_wr_q <= 1'b1;
else
    mmc_resp0_wr_q <= 1'b0;


//-----------------------------------------------------------------
// Register mmc_resp1
//-----------------------------------------------------------------
reg mmc_resp1_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_resp1_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `MMC_RESP1))
    mmc_resp1_wr_q <= 1'b1;
else
    mmc_resp1_wr_q <= 1'b0;


//-----------------------------------------------------------------
// Register mmc_resp2
//-----------------------------------------------------------------
reg mmc_resp2_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_resp2_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `MMC_RESP2))
    mmc_resp2_wr_q <= 1'b1;
else
    mmc_resp2_wr_q <= 1'b0;


//-----------------------------------------------------------------
// Register mmc_resp3
//-----------------------------------------------------------------
reg mmc_resp3_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_resp3_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `MMC_RESP3))
    mmc_resp3_wr_q <= 1'b1;
else
    mmc_resp3_wr_q <= 1'b0;


//-----------------------------------------------------------------
// Register mmc_resp4
//-----------------------------------------------------------------
reg mmc_resp4_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_resp4_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `MMC_RESP4))
    mmc_resp4_wr_q <= 1'b1;
else
    mmc_resp4_wr_q <= 1'b0;


//-----------------------------------------------------------------
// Register mmc_tx
//-----------------------------------------------------------------
reg mmc_tx_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_tx_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `MMC_TX))
    mmc_tx_wr_q <= 1'b1;
else
    mmc_tx_wr_q <= 1'b0;

// mmc_tx_data [external]
wire [31:0]  mmc_tx_data_out_w = wr_data_q[`MMC_TX_DATA_R];


//-----------------------------------------------------------------
// Register mmc_rx
//-----------------------------------------------------------------
reg mmc_rx_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_rx_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `MMC_RX))
    mmc_rx_wr_q <= 1'b1;
else
    mmc_rx_wr_q <= 1'b0;


//-----------------------------------------------------------------
// Register mmc_dma
//-----------------------------------------------------------------
reg mmc_dma_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_dma_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `MMC_DMA))
    mmc_dma_wr_q <= 1'b1;
else
    mmc_dma_wr_q <= 1'b0;

// mmc_dma_addr [internal]
reg [31:0]  mmc_dma_addr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_dma_addr_q <= 32'd`MMC_DMA_ADDR_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `MMC_DMA))
    mmc_dma_addr_q <= cfg_wdata_i[`MMC_DMA_ADDR_R];

wire [31:0]  mmc_dma_addr_out_w = mmc_dma_addr_q;


wire        mmc_status_cmd_in_in_w;
wire [3:0]  mmc_status_dat_in_in_w;
wire        mmc_status_fifo_empty_in_w;
wire        mmc_status_fifo_full_in_w;
wire        mmc_status_crc_err_in_w;
wire        mmc_status_busy_in_w;
wire [31:0]  mmc_resp0_value_in_w;
wire [31:0]  mmc_resp1_value_in_w;
wire [31:0]  mmc_resp2_value_in_w;
wire [31:0]  mmc_resp3_value_in_w;
wire [31:0]  mmc_resp4_value_in_w;
wire [31:0]  mmc_rx_data_in_w;


//-----------------------------------------------------------------
// Read mux
//-----------------------------------------------------------------
reg [31:0] data_r;

always @ *
begin
    data_r = 32'b0;

    case (cfg_araddr_i[7:0])

    `MMC_CONTROL:
    begin
        data_r[`MMC_CONTROL_BLOCK_CNT_R] = mmc_control_block_cnt_q;
        data_r[`MMC_CONTROL_WRITE_R] = mmc_control_write_q;
        data_r[`MMC_CONTROL_DMA_EN_R] = mmc_control_dma_en_q;
        data_r[`MMC_CONTROL_WIDE_MODE_R] = mmc_control_wide_mode_q;
        data_r[`MMC_CONTROL_DATA_EXP_R] = mmc_control_data_exp_q;
        data_r[`MMC_CONTROL_RESP136_EXP_R] = mmc_control_resp136_exp_q;
        data_r[`MMC_CONTROL_RESP48_EXP_R] = mmc_control_resp48_exp_q;
    end
    `MMC_CLOCK:
    begin
        data_r[`MMC_CLOCK_DIV_R] = mmc_clock_div_q;
    end
    `MMC_STATUS:
    begin
        data_r[`MMC_STATUS_CMD_IN_R] = mmc_status_cmd_in_in_w;
        data_r[`MMC_STATUS_DAT_IN_R] = mmc_status_dat_in_in_w;
        data_r[`MMC_STATUS_FIFO_EMPTY_R] = mmc_status_fifo_empty_in_w;
        data_r[`MMC_STATUS_FIFO_FULL_R] = mmc_status_fifo_full_in_w;
        data_r[`MMC_STATUS_CRC_ERR_R] = mmc_status_crc_err_in_w;
        data_r[`MMC_STATUS_BUSY_R] = mmc_status_busy_in_w;
    end
    `MMC_CMD0:
    begin
        data_r[`MMC_CMD0_VALUE_R] = mmc_cmd0_value_q;
    end
    `MMC_CMD1:
    begin
        data_r[`MMC_CMD1_VALUE_R] = mmc_cmd1_value_q;
    end
    `MMC_RESP0:
    begin
        data_r[`MMC_RESP0_VALUE_R] = mmc_resp0_value_in_w;
    end
    `MMC_RESP1:
    begin
        data_r[`MMC_RESP1_VALUE_R] = mmc_resp1_value_in_w;
    end
    `MMC_RESP2:
    begin
        data_r[`MMC_RESP2_VALUE_R] = mmc_resp2_value_in_w;
    end
    `MMC_RESP3:
    begin
        data_r[`MMC_RESP3_VALUE_R] = mmc_resp3_value_in_w;
    end
    `MMC_RESP4:
    begin
        data_r[`MMC_RESP4_VALUE_R] = mmc_resp4_value_in_w;
    end
    `MMC_RX:
    begin
        data_r[`MMC_RX_DATA_R] = mmc_rx_data_in_w;
    end
    `MMC_DMA:
    begin
        data_r[`MMC_DMA_ADDR_R] = mmc_dma_addr_q;
    end
    default :
        data_r = 32'b0;
    endcase
end

//-----------------------------------------------------------------
// RVALID
//-----------------------------------------------------------------
reg rvalid_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    rvalid_q <= 1'b0;
else if (read_en_w)
    rvalid_q <= 1'b1;
else if (cfg_rready_i)
    rvalid_q <= 1'b0;

assign cfg_rvalid_o = rvalid_q;

//-----------------------------------------------------------------
// Retime read response
//-----------------------------------------------------------------
reg [31:0] rd_data_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    rd_data_q <= 32'b0;
else if (!cfg_rvalid_o || cfg_rready_i)
    rd_data_q <= data_r;

assign cfg_rdata_o = rd_data_q;
assign cfg_rresp_o = 2'b0;

//-----------------------------------------------------------------
// BVALID
//-----------------------------------------------------------------
reg bvalid_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    bvalid_q <= 1'b0;
else if (write_en_w)
    bvalid_q <= 1'b1;
else if (cfg_bready_i)
    bvalid_q <= 1'b0;

assign cfg_bvalid_o = bvalid_q;
assign cfg_bresp_o  = 2'b0;

wire mmc_rx_rd_req_w = read_en_w & (cfg_araddr_i[7:0] == `MMC_RX);

wire mmc_tx_wr_req_w = mmc_tx_wr_q;
wire mmc_rx_wr_req_w = mmc_rx_wr_q;

wire       start_w = mmc_control_start_out_w;

//-----------------------------------------------------------------
// Clock Divider
//-----------------------------------------------------------------
wire [7:0] clk_div_w = mmc_clock_div_out_w;
reg [7:0]  clk_div_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    clk_div_q <= 8'd0;
else if (clk_div_q == 8'd0)
    clk_div_q <= clk_div_w;
else
    clk_div_q <= clk_div_q - 8'd1;

wire clk_en_w     = (clk_div_q == 8'd0);

//-----------------------------------------------------------------
// State machine
//-----------------------------------------------------------------
localparam STATE_W          = 3;

// Current state
localparam STATE_IDLE       = 3'd0;
localparam STATE_CMD        = 3'd1;
localparam STATE_RESP       = 3'd2;
localparam STATE_DATA_IN    = 3'd3;
localparam STATE_DATA_OUT   = 3'd4;
localparam STATE_DMA        = 3'd5;

reg [STATE_W-1:0] state_q;
reg [STATE_W-1:0] next_state_r;

wire              cmd_complete_w;
wire              resp_complete_w;
wire              data_complete_w;
wire              write_complete_w;
wire              dma_complete_w;

always @ *
begin
    next_state_r = state_q;

    case (state_q)
    //-----------------------------------------
    // IDLE
    //-----------------------------------------
    STATE_IDLE :
    begin
        if (start_w)
            next_state_r  = STATE_CMD;
    end
    //-----------------------------------------
    // CMD
    //-----------------------------------------
    STATE_CMD:
    begin
        if (cmd_complete_w)
            next_state_r = (mmc_control_resp136_exp_out_w || mmc_control_resp48_exp_out_w) ?
                           STATE_RESP : STATE_IDLE;
    end
    //-----------------------------------------
    // RESP
    //-----------------------------------------
    STATE_RESP:
    begin
        if (resp_complete_w)
        begin
            if (mmc_control_data_exp_out_w)
                next_state_r = STATE_DATA_IN;
            else if (mmc_control_write_out_w)
                next_state_r = STATE_DATA_OUT;
            else
                next_state_r = STATE_IDLE;
        end
    end
    //-----------------------------------------
    // DATA_IN
    //-----------------------------------------
    STATE_DATA_IN:
    begin
        if (data_complete_w && mmc_control_dma_en_out_w)
            next_state_r = STATE_DMA;
        else if (data_complete_w)
            next_state_r = STATE_IDLE;
    end
    //-----------------------------------------
    // DMA
    //-----------------------------------------
    STATE_DMA:
    begin
        if (dma_complete_w)
            next_state_r = STATE_IDLE;
    end
    //-----------------------------------------
    // DATA_OUT
    //-----------------------------------------
    STATE_DATA_OUT:
    begin
        if (write_complete_w)
            next_state_r = STATE_IDLE;
    end
    default :
       ;

    endcase

    if (mmc_control_abort_out_w)
        next_state_r = STATE_IDLE;
end

// Update state
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    state_q <= STATE_IDLE;
else
    state_q <= next_state_r;

assign mmc_status_busy_in_w = (state_q != STATE_IDLE);

//-----------------------------------------------------------------
// Interrupt Output
//-----------------------------------------------------------------
reg intr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    intr_q <= 1'b0;
else
    intr_q <= (state_q != STATE_IDLE) && (next_state_r == STATE_IDLE);

assign intr_o = intr_q;

//-----------------------------------------------------------------
// Command Output
//-----------------------------------------------------------------
wire [47:0] cmd_w = {1'b0, 1'b1, mmc_cmd0_value_out_w[29:0], mmc_cmd1_value_out_w[15:8], 7'b0, 1'b1};

mmc_cmd_serialiser
u_cmd_out
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.bitclk_i(mmc_clk_o)
    ,.start_i(start_w)
    ,.abort_i(mmc_control_abort_out_w)
    ,.data_i(cmd_w)
    ,.cmd_o(mmc_cmd_out_o)
    ,.active_o(mmc_cmd_out_en_o)
    ,.complete_o(cmd_complete_w)
);    

//-----------------------------------------------------------------
// Response Input
//-----------------------------------------------------------------
wire [135:0] resp_w;

mmc_cmd_deserialiser
u_cmd_in
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.bitclk_i(mmc_clk_o)
    ,.start_i(state_q == STATE_CMD && next_state_r == STATE_RESP)
    ,.abort_i(mmc_control_abort_out_w)
    ,.data_i(mmc_cmd_in_i)
    ,.r2_mode_i(mmc_control_resp136_exp_out_w)
    ,.resp_o(resp_w)
    ,.active_o()
    ,.complete_o(resp_complete_w)
);    

assign mmc_resp0_value_in_w = resp_w[31:0];
assign mmc_resp1_value_in_w = resp_w[63:32];
assign mmc_resp2_value_in_w = resp_w[95:64];
assign mmc_resp3_value_in_w = resp_w[127:96];
assign mmc_resp4_value_in_w = {24'b0, resp_w[135:128]};


//-----------------------------------------------------------------
// Data Input
//-----------------------------------------------------------------
wire       rx_ready_w;
wire [7:0] rx_data0_w;
wire [7:0] rx_data1_w;
wire [7:0] rx_data2_w;
wire [7:0] rx_data3_w;

wire       data_start_w = state_q == STATE_RESP && next_state_r == STATE_DATA_IN;

mmc_dat_deserialiser
u_dat0_in
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    ,.bitclk_i(mmc_clk_o)
    ,.start_i(data_start_w)
    ,.abort_i(mmc_control_abort_out_w)
    ,.data_i(mmc_dat_in_i[0])
    ,.mode_4bit_i(mmc_control_wide_mode_out_w)
    ,.block_cnt_i(mmc_control_block_cnt_out_w)

    ,.valid_o(rx_ready_w)
    ,.data_o(rx_data0_w)
    ,.active_o()
    ,.error_o()
    ,.complete_o(data_complete_w)
);

mmc_dat_deserialiser
u_dat1_in
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    ,.bitclk_i(mmc_clk_o)
    ,.start_i(data_start_w & mmc_control_wide_mode_out_w)
    ,.abort_i(mmc_control_abort_out_w)
    ,.data_i(mmc_dat_in_i[1])
    ,.mode_4bit_i(1'b1)
    ,.block_cnt_i(mmc_control_block_cnt_out_w)

    ,.valid_o()
    ,.data_o(rx_data1_w)
    ,.active_o()
    ,.error_o()
    ,.complete_o()
);

mmc_dat_deserialiser
u_dat2_in
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    ,.bitclk_i(mmc_clk_o)
    ,.start_i(data_start_w & mmc_control_wide_mode_out_w)
    ,.abort_i(mmc_control_abort_out_w)
    ,.data_i(mmc_dat_in_i[2])
    ,.mode_4bit_i(1'b1)
    ,.block_cnt_i(mmc_control_block_cnt_out_w)

    ,.valid_o()
    ,.data_o(rx_data2_w)
    ,.active_o()
    ,.error_o()
    ,.complete_o()
);

mmc_dat_deserialiser
u_dat3_in
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    ,.bitclk_i(mmc_clk_o)
    ,.start_i(data_start_w & mmc_control_wide_mode_out_w)
    ,.abort_i(mmc_control_abort_out_w)
    ,.data_i(mmc_dat_in_i[3])
    ,.mode_4bit_i(1'b1)
    ,.block_cnt_i(mmc_control_block_cnt_out_w)

    ,.valid_o()
    ,.data_o(rx_data3_w)
    ,.active_o()
    ,.error_o()
    ,.complete_o()
);

wire [31:0] wide_data_w;

// Demange byte packed 4-bit
assign wide_data_w[0+0]  = rx_data0_w[6];
assign wide_data_w[0+1]  = rx_data1_w[6];
assign wide_data_w[0+2]  = rx_data2_w[6];
assign wide_data_w[0+3]  = rx_data3_w[6];
assign wide_data_w[0+4]  = rx_data0_w[7];
assign wide_data_w[0+5]  = rx_data1_w[7];
assign wide_data_w[0+6]  = rx_data2_w[7];
assign wide_data_w[0+7]  = rx_data3_w[7];
assign wide_data_w[8+0]  = rx_data0_w[4];
assign wide_data_w[8+1]  = rx_data1_w[4];
assign wide_data_w[8+2]  = rx_data2_w[4];
assign wide_data_w[8+3]  = rx_data3_w[4];
assign wide_data_w[8+4]  = rx_data0_w[5];
assign wide_data_w[8+5]  = rx_data1_w[5];
assign wide_data_w[8+6]  = rx_data2_w[5];
assign wide_data_w[8+7]  = rx_data3_w[5];
assign wide_data_w[16+0] = rx_data0_w[2];
assign wide_data_w[16+1] = rx_data1_w[2];
assign wide_data_w[16+2] = rx_data2_w[2];
assign wide_data_w[16+3] = rx_data3_w[2];
assign wide_data_w[16+4] = rx_data0_w[3];
assign wide_data_w[16+5] = rx_data1_w[3];
assign wide_data_w[16+6] = rx_data2_w[3];
assign wide_data_w[16+7] = rx_data3_w[3];
assign wide_data_w[24+0] = rx_data0_w[0];
assign wide_data_w[24+1] = rx_data1_w[0];
assign wide_data_w[24+2] = rx_data2_w[0];
assign wide_data_w[24+3] = rx_data3_w[0];
assign wide_data_w[24+4] = rx_data0_w[1];
assign wide_data_w[24+5] = rx_data1_w[1];
assign wide_data_w[24+6] = rx_data2_w[1];
assign wide_data_w[24+7] = rx_data3_w[1];

reg [31:0] rx_data_q;
reg [1:0]  rx_idx_q;
reg        rx_push_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    rx_data_q <= 32'b0;
else if (mmc_control_wide_mode_out_w)
    rx_data_q <= wide_data_w;
else if (rx_ready_w)
    rx_data_q <= {rx_data0_w,rx_data_q[31:8]};

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    rx_idx_q <= 2'b0;
else if (state_q == STATE_RESP && next_state_r == STATE_DATA_IN)
    rx_idx_q <= 2'b0;
else if (rx_ready_w && !mmc_control_wide_mode_out_w)
    rx_idx_q <= rx_idx_q + 2'd1;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    rx_push_q <= 1'b0;
else
    rx_push_q <= rx_ready_w && (mmc_control_wide_mode_out_w || (rx_idx_q == 2'd3));

//-----------------------------------------------------------------
// Data Output
//-----------------------------------------------------------------
wire       data_write_w = state_q == STATE_RESP && next_state_r == STATE_DATA_OUT;
wire       tx_ready_w;
reg [1:0]  tx_idx_q;

// Single data
wire [7:0] tx_data_w  = (tx_idx_q == 2'd0) ? fifo_data_out_w[7:0]   :
                        (tx_idx_q == 2'd1) ? fifo_data_out_w[15:8]  :
                        (tx_idx_q == 2'd2) ? fifo_data_out_w[23:16] :
                                             fifo_data_out_w[31:24];

// 4-bit data
wire [7:0] tx_data0_w;
wire [7:0] tx_data1_w;
wire [7:0] tx_data2_w;
wire [7:0] tx_data3_w;

assign tx_data0_w[6] = fifo_data_out_w[0+0];
assign tx_data1_w[6] = fifo_data_out_w[0+1];
assign tx_data2_w[6] = fifo_data_out_w[0+2];
assign tx_data3_w[6] = fifo_data_out_w[0+3];
assign tx_data0_w[7] = fifo_data_out_w[0+4];
assign tx_data1_w[7] = fifo_data_out_w[0+5];
assign tx_data2_w[7] = fifo_data_out_w[0+6];
assign tx_data3_w[7] = fifo_data_out_w[0+7];
assign tx_data0_w[4] = fifo_data_out_w[8+0];
assign tx_data1_w[4] = fifo_data_out_w[8+1];
assign tx_data2_w[4] = fifo_data_out_w[8+2];
assign tx_data3_w[4] = fifo_data_out_w[8+3];
assign tx_data0_w[5] = fifo_data_out_w[8+4];
assign tx_data1_w[5] = fifo_data_out_w[8+5];
assign tx_data2_w[5] = fifo_data_out_w[8+6];
assign tx_data3_w[5] = fifo_data_out_w[8+7];
assign tx_data0_w[2] = fifo_data_out_w[16+0];
assign tx_data1_w[2] = fifo_data_out_w[16+1];
assign tx_data2_w[2] = fifo_data_out_w[16+2];
assign tx_data3_w[2] = fifo_data_out_w[16+3];
assign tx_data0_w[3] = fifo_data_out_w[16+4];
assign tx_data1_w[3] = fifo_data_out_w[16+5];
assign tx_data2_w[3] = fifo_data_out_w[16+6];
assign tx_data3_w[3] = fifo_data_out_w[16+7];
assign tx_data0_w[0] = fifo_data_out_w[24+0];
assign tx_data1_w[0] = fifo_data_out_w[24+1];
assign tx_data2_w[0] = fifo_data_out_w[24+2];
assign tx_data3_w[0] = fifo_data_out_w[24+3];
assign tx_data0_w[1] = fifo_data_out_w[24+4];
assign tx_data1_w[1] = fifo_data_out_w[24+5];
assign tx_data2_w[1] = fifo_data_out_w[24+6];
assign tx_data3_w[1] = fifo_data_out_w[24+7];

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    tx_idx_q <= 2'b0;
else if (mmc_control_abort_out_w)
    tx_idx_q <= 2'b0;
else if (tx_ready_w && !mmc_control_wide_mode_out_w)
    tx_idx_q <= tx_idx_q + 2'd1;

mmc_dat_serialiser
u_dat0_out
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    ,.bitclk_i(mmc_clk_o)
    ,.start_i(data_write_w)
    ,.abort_i(mmc_control_abort_out_w)

    ,.data_i(mmc_control_wide_mode_out_w ? tx_data0_w : tx_data_w)
    ,.accept_o(tx_ready_w)

    ,.mode_4bit_i(mmc_control_wide_mode_out_w)
    ,.block_cnt_i(mmc_control_block_cnt_out_w)
    ,.dat_o(mmc_dat_out_o[0])
    ,.dat_out_en_o(mmc_dat_out_en_o[0])
    ,.active_o()
    ,.complete_o(write_complete_w)
);

mmc_dat_serialiser
u_dat1_out
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    ,.bitclk_i(mmc_clk_o)
    ,.start_i(data_write_w & mmc_control_wide_mode_out_w)
    ,.abort_i(mmc_control_abort_out_w)

    ,.data_i(tx_data1_w)
    ,.accept_o()

    ,.mode_4bit_i(1'b1)
    ,.block_cnt_i(mmc_control_block_cnt_out_w)
    ,.dat_o(mmc_dat_out_o[1])
    ,.dat_out_en_o(mmc_dat_out_en_o[1])
    ,.active_o()
    ,.complete_o()
);
mmc_dat_serialiser
u_dat2_out
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    ,.bitclk_i(mmc_clk_o)
    ,.start_i(data_write_w & mmc_control_wide_mode_out_w)
    ,.abort_i(mmc_control_abort_out_w)

    ,.data_i(tx_data2_w)
    ,.accept_o()

    ,.mode_4bit_i(1'b1)
    ,.block_cnt_i(mmc_control_block_cnt_out_w)
    ,.dat_o(mmc_dat_out_o[2])
    ,.dat_out_en_o(mmc_dat_out_en_o[2])
    ,.active_o()
    ,.complete_o()
);
mmc_dat_serialiser
u_dat3_out
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    ,.bitclk_i(mmc_clk_o)
    ,.start_i(data_write_w & mmc_control_wide_mode_out_w)
    ,.abort_i(mmc_control_abort_out_w)

    ,.data_i(tx_data3_w)
    ,.accept_o()

    ,.mode_4bit_i(1'b1)
    ,.block_cnt_i(mmc_control_block_cnt_out_w)
    ,.dat_o(mmc_dat_out_o[3])
    ,.dat_out_en_o(mmc_dat_out_en_o[3])
    ,.active_o()
    ,.complete_o()
);

reg tx_pop_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    tx_pop_q <= 1'b0;
else
    tx_pop_q <= tx_ready_w && (mmc_control_wide_mode_out_w || (tx_idx_q == 2'd3));

//-----------------------------------------------------------------
// Shared FIFO
//-----------------------------------------------------------------
wire        rx_accept_w;
wire        rx_valid_w;

wire        fifo_push_w    = mmc_tx_wr_req_w | rx_push_q;
wire [31:0] fifo_data_in_w = mmc_tx_wr_req_w ? mmc_tx_data_out_w : rx_data_q;

wire        dma_pop_w;
wire        fifo_pop_w     = mmc_rx_rd_req_w | dma_pop_w | tx_pop_q;
wire [31:0] fifo_data_out_w;
wire [MMC_FIFO_DEPTH_W:0] fifo_level_w;

mmc_card_fifo
u_fifo
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    ,.flush_i(mmc_control_fifo_rst_out_w)

    ,.push_i(fifo_push_w)
    ,.data_in_i(fifo_data_in_w)
    ,.accept_o(rx_accept_w)

    ,.valid_o(rx_valid_w)
    ,.data_out_o(fifo_data_out_w)
    ,.pop_i(fifo_pop_w)

    ,.level_o(fifo_level_w)
);

assign mmc_rx_data_in_w           = fifo_data_out_w;
assign mmc_status_fifo_empty_in_w = ~rx_valid_w;
assign mmc_status_fifo_full_in_w  = ~rx_accept_w;
assign mmc_status_crc_err_in_w    = 1'b0; // TODO: CRC checking

//-----------------------------------------------------------------
// Outputs
//-----------------------------------------------------------------
//synthesis attribute IOB of mmc_clk_q is "TRUE"
reg mmc_clk_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    mmc_clk_q <= 1'b0;
else if (clk_en_w)
    mmc_clk_q <= ~mmc_clk_q;

assign mmc_clk_o             = mmc_clk_q;

//-----------------------------------------------------------------
// DMA
//-----------------------------------------------------------------

// Enough data for a burst
wire can_issue_w = mmc_control_dma_en_out_w && (fifo_level_w >= 8);
reg [2:0] burst_cnt_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    burst_cnt_q <= 3'b0;
else if (start_w)
    burst_cnt_q <= 3'b0;
else if (outport_wvalid_o && outport_wready_i)
    burst_cnt_q <= burst_cnt_q + 3'd1;

reg [31:0] awaddr_q;
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    awaddr_q <= 32'b0;
else if (start_w)
    awaddr_q <= mmc_dma_addr_out_w;
else if (outport_awvalid_o && outport_awready_i)
    awaddr_q <= awaddr_q + {22'b0, outport_awlen_o, 2'b0} + 32'd4;

assign outport_awvalid_o = (burst_cnt_q == 3'd0) ? (can_issue_w & outport_wready_i) : 1'b0;
assign outport_awaddr_o  = awaddr_q;
assign outport_awlen_o   = 8'd7; // 32-bytes
assign outport_awburst_o = 2'b01; // INCR
assign outport_awid_o    = AXI_ID;

assign outport_wvalid_o = (burst_cnt_q == 3'd0) ? (can_issue_w & outport_awready_i) : 1'b1;
assign outport_wdata_o  = fifo_data_out_w;
assign outport_wstrb_o  = 4'hF;
assign outport_wlast_o  = burst_cnt_q == 3'd7;

// Pop FIFO
assign dma_pop_w        = outport_wvalid_o & outport_wready_i;

assign outport_rready_o = 1'b1;
assign outport_bready_o = 1'b1;

// Not yet
assign outport_arvalid_o = 1'b0;
assign outport_araddr_o  = 32'b0;
assign outport_arid_o    = 4'b0;
assign outport_arlen_o   = 8'b0;
assign outport_arburst_o = 2'b0;

reg [15:0] outstanding_q;
reg [15:0] outstanding_r;

always @ *
begin
    outstanding_r = outstanding_q;

    if (outport_awvalid_o && outport_awready_i)
        outstanding_r = outstanding_r + 16'd1;
    if (outport_bvalid_i && outport_bready_o)
        outstanding_r = outstanding_r - 16'd1;
end

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    outstanding_q <= 16'b0;
else
    outstanding_q <= outstanding_r;

assign dma_complete_w = ~((|fifo_level_w) || (|outstanding_q) || outport_awvalid_o || outport_wvalid_o);

//-----------------------------------------------------------------
// Input capture
//-----------------------------------------------------------------
reg clk_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    clk_q <= 1'b0;
else
    clk_q <= mmc_clk_o;

// Capture on rising edge
wire capture_w = mmc_clk_o & ~clk_q;

reg       cmd_in_q;
reg [3:0] dat_in_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    cmd_in_q <= 1'b1;
else if (capture_w)
    cmd_in_q <= mmc_cmd_in_i;

assign mmc_status_cmd_in_in_w = cmd_in_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    dat_in_q <= 4'hF;
else if (capture_w)
    dat_in_q <= mmc_dat_in_i;

assign mmc_status_dat_in_in_w = dat_in_q;


endmodule
