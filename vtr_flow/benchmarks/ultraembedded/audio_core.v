//-----------------------------------------------------------------
//                      Audio Controller
//                           V0.1
//                     Ultra-Embedded.com
//                     Copyright 2012-2019
//
//                 Email: admin@ultra-embedded.com
//
//                         License: GPL
// If you would like a version with a more permissive license for
// use in closed source commercial applications please contact me
// for details.
//-----------------------------------------------------------------
//
// This file is open source HDL; you can redistribute it and/or 
// modify it under the terms of the GNU General Public License as 
// published by the Free Software Foundation; either version 2 of 
// the License, or (at your option) any later version.
//
// This file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public 
// License along with this file; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
//-----------------------------------------------------------------

//-----------------------------------------------------------------
//                          Generated File
//-----------------------------------------------------------------

`define AUDIO_CFG    8'h0

    `define AUDIO_CFG_INT_THRESHOLD_DEFAULT    0
    `define AUDIO_CFG_INT_THRESHOLD_B          0
    `define AUDIO_CFG_INT_THRESHOLD_T          15
    `define AUDIO_CFG_INT_THRESHOLD_W          16
    `define AUDIO_CFG_INT_THRESHOLD_R          15:0

    `define AUDIO_CFG_BYTE_SWAP      16
    `define AUDIO_CFG_BYTE_SWAP_DEFAULT    0
    `define AUDIO_CFG_BYTE_SWAP_B          16
    `define AUDIO_CFG_BYTE_SWAP_T          16
    `define AUDIO_CFG_BYTE_SWAP_W          1
    `define AUDIO_CFG_BYTE_SWAP_R          16:16

    `define AUDIO_CFG_CH_SWAP      17
    `define AUDIO_CFG_CH_SWAP_DEFAULT    0
    `define AUDIO_CFG_CH_SWAP_B          17
    `define AUDIO_CFG_CH_SWAP_T          17
    `define AUDIO_CFG_CH_SWAP_W          1
    `define AUDIO_CFG_CH_SWAP_R          17:17

    `define AUDIO_CFG_TARGET_DEFAULT    0
    `define AUDIO_CFG_TARGET_B          18
    `define AUDIO_CFG_TARGET_T          19
    `define AUDIO_CFG_TARGET_W          2
    `define AUDIO_CFG_TARGET_R          19:18

    `define AUDIO_CFG_VOL_CTRL_DEFAULT    0
    `define AUDIO_CFG_VOL_CTRL_B          24
    `define AUDIO_CFG_VOL_CTRL_T          26
    `define AUDIO_CFG_VOL_CTRL_W          3
    `define AUDIO_CFG_VOL_CTRL_R          26:24

    `define AUDIO_CFG_BUFFER_RST      31
    `define AUDIO_CFG_BUFFER_RST_DEFAULT    0
    `define AUDIO_CFG_BUFFER_RST_B          31
    `define AUDIO_CFG_BUFFER_RST_T          31
    `define AUDIO_CFG_BUFFER_RST_W          1
    `define AUDIO_CFG_BUFFER_RST_R          31:31

`define AUDIO_STATUS    8'h4

    `define AUDIO_STATUS_LEVEL_DEFAULT    0
    `define AUDIO_STATUS_LEVEL_B          16
    `define AUDIO_STATUS_LEVEL_T          31
    `define AUDIO_STATUS_LEVEL_W          16
    `define AUDIO_STATUS_LEVEL_R          31:16

    `define AUDIO_STATUS_FULL      1
    `define AUDIO_STATUS_FULL_DEFAULT    0
    `define AUDIO_STATUS_FULL_B          1
    `define AUDIO_STATUS_FULL_T          1
    `define AUDIO_STATUS_FULL_W          1
    `define AUDIO_STATUS_FULL_R          1:1

    `define AUDIO_STATUS_EMPTY      0
    `define AUDIO_STATUS_EMPTY_DEFAULT    0
    `define AUDIO_STATUS_EMPTY_B          0
    `define AUDIO_STATUS_EMPTY_T          0
    `define AUDIO_STATUS_EMPTY_W          1
    `define AUDIO_STATUS_EMPTY_R          0:0

`define AUDIO_CLK_DIV    8'h8

    `define AUDIO_CLK_DIV_WHOLE_CYCLES_DEFAULT    0
    `define AUDIO_CLK_DIV_WHOLE_CYCLES_B          0
    `define AUDIO_CLK_DIV_WHOLE_CYCLES_T          15
    `define AUDIO_CLK_DIV_WHOLE_CYCLES_W          16
    `define AUDIO_CLK_DIV_WHOLE_CYCLES_R          15:0

`define AUDIO_CLK_FRAC    8'hc

    `define AUDIO_CLK_FRAC_NUMERATOR_DEFAULT    0
    `define AUDIO_CLK_FRAC_NUMERATOR_B          0
    `define AUDIO_CLK_FRAC_NUMERATOR_T          15
    `define AUDIO_CLK_FRAC_NUMERATOR_W          16
    `define AUDIO_CLK_FRAC_NUMERATOR_R          15:0

    `define AUDIO_CLK_FRAC_DENOMINATOR_DEFAULT    0
    `define AUDIO_CLK_FRAC_DENOMINATOR_B          16
    `define AUDIO_CLK_FRAC_DENOMINATOR_T          31
    `define AUDIO_CLK_FRAC_DENOMINATOR_W          16
    `define AUDIO_CLK_FRAC_DENOMINATOR_R          31:16

`define AUDIO_FIFO_WRITE    8'h20

    `define AUDIO_FIFO_WRITE_CH_B_DEFAULT    0
    `define AUDIO_FIFO_WRITE_CH_B_B          0
    `define AUDIO_FIFO_WRITE_CH_B_T          15
    `define AUDIO_FIFO_WRITE_CH_B_W          16
    `define AUDIO_FIFO_WRITE_CH_B_R          15:0

    `define AUDIO_FIFO_WRITE_CH_A_DEFAULT    0
    `define AUDIO_FIFO_WRITE_CH_A_B          16
    `define AUDIO_FIFO_WRITE_CH_A_T          31
    `define AUDIO_FIFO_WRITE_CH_A_W          16
    `define AUDIO_FIFO_WRITE_CH_A_R          31:16

//-----------------------------------------------------------------
//                      Audio Controller
//                           V0.1
//                     Ultra-Embedded.com
//                     Copyright 2012-2019
//
//                 Email: admin@ultra-embedded.com
//
//                         License: GPL
// If you would like a version with a more permissive license for
// use in closed source commercial applications please contact me
// for details.
//-----------------------------------------------------------------
//
// This file is open source HDL; you can redistribute it and/or 
// modify it under the terms of the GNU General Public License as 
// published by the Free Software Foundation; either version 2 of 
// the License, or (at your option) any later version.
//
// This file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public 
// License along with this file; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
//-----------------------------------------------------------------

//-----------------------------------------------------------------
//                          Generated File
//-----------------------------------------------------------------

module audio_dac
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           audio_clk_i
    ,input           inport_tvalid_i
    ,input  [ 31:0]  inport_tdata_i
    ,input  [  3:0]  inport_tstrb_i
    ,input  [  3:0]  inport_tdest_i
    ,input           inport_tlast_i

    // Outputs
    ,output          inport_tready_o
    ,output          audio_l_o
    ,output          audio_r_o
);




//-----------------------------------------------------------------
// External clock source
//-----------------------------------------------------------------
wire    bit_clock_w;

assign bit_clock_w = audio_clk_i;

//-----------------------------------------------------------------
// Buffer
//-----------------------------------------------------------------
reg [15:0] left_q;
reg [15:0] right_q;
reg        pop_q;

always @ (posedge rst_i or posedge clk_i )
begin
   if (rst_i)
   begin
        right_q <= 16'b0;
        left_q  <= 16'b0;
        pop_q   <= 1'b0;
   end
   else if (bit_clock_w && inport_tvalid_i)
   begin
        {right_q, left_q} <= inport_tdata_i;
        pop_q   <= 1'b1;
   end
   else
        pop_q   <= 1'b0;
end

assign inport_tready_o = pop_q;

//-----------------------------------------------------------------
// DAC instances
//-----------------------------------------------------------------
sigma_dac
#( .NBITS(16) )
u_dac_l
(
    .clk_i(clk_i),
    .rst_i(rst_i),

    .din_i(left_q),
    .dout_o(audio_l_o)
);

sigma_dac
#( .NBITS(16) )
u_dac_r
(
    .clk_i(clk_i),
    .rst_i(rst_i),

    .din_i(right_q),
    .dout_o(audio_r_o)
);


endmodule
//-----------------------------------------------------------------
//                      Audio Controller
//                           V0.1
//                     Ultra-Embedded.com
//                     Copyright 2012-2019
//
//                 Email: admin@ultra-embedded.com
//
//                         License: GPL
// If you would like a version with a more permissive license for
// use in closed source commercial applications please contact me
// for details.
//-----------------------------------------------------------------
//
// This file is open source HDL; you can redistribute it and/or 
// modify it under the terms of the GNU General Public License as 
// published by the Free Software Foundation; either version 2 of 
// the License, or (at your option) any later version.
//
// This file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public 
// License along with this file; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
//-----------------------------------------------------------------

//-----------------------------------------------------------------
//                          Generated File
//-----------------------------------------------------------------

module audio_fifo
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
);



//-----------------------------------------------------------------
// Registers
//-----------------------------------------------------------------
reg [10:0]   rd_ptr_q;
reg [10:0]   wr_ptr_q;

//-----------------------------------------------------------------
// Write Side
//-----------------------------------------------------------------
wire [10:0] write_next_w = wr_ptr_q + 11'd1;

wire full_w = (write_next_w == rd_ptr_q);

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    wr_ptr_q <= 11'b0;
else if (flush_i)
    wr_ptr_q <= 11'b0;
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
    rd_ptr_q     <= 11'b0;
else if (flush_i)
    rd_ptr_q     <= 11'b0;
// Read address increment
else if (read_ok_w && ((!valid_o) || (valid_o && pop_i)))
    rd_ptr_q <= rd_ptr_q + 11'd1;

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

fifo_ram_dp_2048_11
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

endmodule

//-------------------------------------------------------------------
// Dual port RAM
//-------------------------------------------------------------------
module fifo_ram_dp_2048_11
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

/* verilator lint_off MULTIDRIVEN */
reg [31:0]   ram [2047:0] /*verilator public*/;
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
//                      Audio Controller
//                           V0.1
//                     Ultra-Embedded.com
//                     Copyright 2012-2019
//
//                 Email: admin@ultra-embedded.com
//
//                         License: GPL
// If you would like a version with a more permissive license for
// use in closed source commercial applications please contact me
// for details.
//-----------------------------------------------------------------
//
// This file is open source HDL; you can redistribute it and/or 
// modify it under the terms of the GNU General Public License as 
// published by the Free Software Foundation; either version 2 of 
// the License, or (at your option) any later version.
//
// This file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public 
// License along with this file; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
//-----------------------------------------------------------------

//-----------------------------------------------------------------
//                          Generated File
//-----------------------------------------------------------------

module audio_spdif
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           audio_clk_i
    ,input           inport_tvalid_i
    ,input  [ 31:0]  inport_tdata_i
    ,input  [  3:0]  inport_tstrb_i
    ,input  [  3:0]  inport_tdest_i
    ,input           inport_tlast_i

    // Outputs
    ,output          inport_tready_o
    ,output          spdif_o
);




wire bit_clock_w = audio_clk_i;

//-----------------------------------------------------------------
// Core SPDIF
//-----------------------------------------------------------------
spdif_core
u_core
(
    .clk_i(clk_i),
    .rst_i(rst_i),

    .bit_out_en_i(bit_clock_w),

    .spdif_o(spdif_o),

    .sample_i(inport_tdata_i),
    .sample_req_o(inport_tready_o)
);


endmodule
//-----------------------------------------------------------------
//                      Audio Controller
//                           V0.1
//                     Ultra-Embedded.com
//                     Copyright 2012-2019
//
//                 Email: admin@ultra-embedded.com
//
//                         License: GPL
// If you would like a version with a more permissive license for
// use in closed source commercial applications please contact me
// for details.
//-----------------------------------------------------------------
//
// This file is open source HDL; you can redistribute it and/or 
// modify it under the terms of the GNU General Public License as 
// published by the Free Software Foundation; either version 2 of 
// the License, or (at your option) any later version.
//
// This file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public 
// License along with this file; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
//-----------------------------------------------------------------

//-----------------------------------------------------------------
//                          Generated File
//-----------------------------------------------------------------
module sigma_dac
#(
    parameter [31:0] NBITS = 16
)
(
    input             clk_i,
    input             rst_i,

    input [NBITS-1:0] din_i,
    output            dout_o
);

//-----------------------------------------------------------------
// Implementation: Described in Xilinx xapp154.pdf
//-----------------------------------------------------------------
reg [NBITS+2:0] delta_adder_r;
reg [NBITS+2:0] sigma_adder_r;
reg [NBITS+2:0] sigma_latch_q;
reg [NBITS+2:0] delta_r;

//synthesis attribute IOB of dac_q is "TRUE"
reg             dac_q;

wire [NBITS-1:0]  input_unsigned_w = din_i + 'd32768;

/* verilator lint_off WIDTH */
always @ * 
    delta_r = {sigma_latch_q[NBITS+2], sigma_latch_q[NBITS+2]} << (NBITS+1);

always @ *
    delta_adder_r = input_unsigned_w + delta_r;
/* verilator lint_on WIDTH */

always @ *
    sigma_adder_r = delta_adder_r + sigma_latch_q;

always @(posedge clk_i or posedge rst_i) 
begin
    if (rst_i)
    begin
        sigma_latch_q <= 1'b1 << (NBITS+1);
        dac_q         <= 1'b0;
    end
    else
    begin
        sigma_latch_q <= sigma_adder_r;
        dac_q         <= sigma_latch_q[NBITS+2];
    end
end

assign dout_o = dac_q;

endmodule
//-----------------------------------------------------------------
//                      Audio Controller
//                           V0.1
//                     Ultra-Embedded.com
//                     Copyright 2012-2019
//
//                 Email: admin@ultra-embedded.com
//
//                         License: GPL
// If you would like a version with a more permissive license for
// use in closed source commercial applications please contact me
// for details.
//-----------------------------------------------------------------
//
// This file is open source HDL; you can redistribute it and/or 
// modify it under the terms of the GNU General Public License as 
// published by the Free Software Foundation; either version 2 of 
// the License, or (at your option) any later version.
//
// This file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public 
// License along with this file; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
//-----------------------------------------------------------------

//-----------------------------------------------------------------
//                          Generated File
//-----------------------------------------------------------------

module audio_i2s
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           audio_clk_i
    ,input           inport_tvalid_i
    ,input  [ 31:0]  inport_tdata_i
    ,input  [  3:0]  inport_tstrb_i
    ,input  [  3:0]  inport_tdest_i
    ,input           inport_tlast_i

    // Outputs
    ,output          inport_tready_o
    ,output          i2s_sck_o
    ,output          i2s_sdata_o
    ,output          i2s_ws_o
);



//-----------------------------------------------------------------
// Registers
//-----------------------------------------------------------------
reg         sck_q;
reg         sdata_q;
reg         ws_q;
reg [4:0]   bit_count_q;

wire bit_clock_w = audio_clk_i;

//-----------------------------------------------------------------
// Buffer
//-----------------------------------------------------------------
reg [31:0] buffer_q;
reg        pop_q;

always @ (posedge rst_i or posedge clk_i)
if (rst_i)
begin
    buffer_q <= 32'b0;
    pop_q    <= 1'b0;
end
// Capture the next sample 1 SCK period before needed
else if (bit_clock_w && (bit_count_q == 5'd0) && sck_q)
begin
    buffer_q <= inport_tdata_i;
    pop_q    <= 1'b1;
end
else
    pop_q   <= 1'b0;

assign inport_tready_o = pop_q;

//-----------------------------------------------------------------
// I2S Output Generator
//-----------------------------------------------------------------
reg next_data_q;

always @ (posedge rst_i or posedge clk_i)
if (rst_i) 
begin
    bit_count_q     <= 5'd31;
    sdata_q         <= 1'b0;
    ws_q            <= 1'b0;
    sck_q           <= 1'b0;
    next_data_q     <= 1'b0;
end 
else if (bit_clock_w)
begin
    // SCK 1->0 - Falling Edge - drive SDATA
    if (sck_q)
    begin
        // SDATA lags WS by 1 cycle
        sdata_q     <= next_data_q;

        // Drive data MSB first
        next_data_q <= buffer_q[bit_count_q];

        // WS = 0 (left), 1 = (right)
        ws_q        <= ~bit_count_q[4];

        bit_count_q <= bit_count_q - 5'd1;
    end

    sck_q <= ~sck_q;
end

assign i2s_sck_o   = sck_q;
assign i2s_sdata_o = sdata_q;
assign i2s_ws_o    = ws_q;


endmodule
//-----------------------------------------------------------------
//                      Audio Controller
//                           V0.1
//                     Ultra-Embedded.com
//                     Copyright 2012-2019
//
//                 Email: admin@ultra-embedded.com
//
//                         License: GPL
// If you would like a version with a more permissive license for
// use in closed source commercial applications please contact me
// for details.
//-----------------------------------------------------------------
//
// This file is open source HDL; you can redistribute it and/or 
// modify it under the terms of the GNU General Public License as 
// published by the Free Software Foundation; either version 2 of 
// the License, or (at your option) any later version.
//
// This file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public 
// License along with this file; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
//-----------------------------------------------------------------

//-----------------------------------------------------------------
//                          Generated File
//-----------------------------------------------------------------
//-----------------------------------------------------------------
//                        SPDIF Transmitter
//                              V0.1
//                        Ultra-Embedded.com
//                          Copyright 2012
//
//                 Email: admin@ultra-embedded.com
//
//                         License: GPL
// If you would like a version with a more permissive license for
// use in closed source commercial applications please contact me
// for details.
//-----------------------------------------------------------------
//
// This file is open source HDL; you can redistribute it and/or 
// modify it under the terms of the GNU General Public License as 
// published by the Free Software Foundation; either version 2 of 
// the License, or (at your option) any later version.
//
// This file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public 
// License along with this file; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
//-----------------------------------------------------------------
module spdif_core
(
    input           clk_i,
    input           rst_i,

    // SPDIF bit output enable
    // Single cycle pulse synchronous to clk_i which drives
    // the output bit rate.
    // For 44.1KHz, 44100x32x2x2 = 5,644,800Hz
    // For 48KHz,   48000x32x2x2 = 6,144,000Hz
    input           bit_out_en_i,

    // Output
    output          spdif_o,

    // Audio interface (16-bit x 2 = RL)
    input [31:0]    sample_i,
    output reg      sample_req_o
);

//-----------------------------------------------------------------
// Registers
//-----------------------------------------------------------------
reg [15:0]  audio_sample_q;
reg [8:0]   subframe_count_q;

reg         load_subframe_q;
reg [7:0]   preamble_q;
wire [31:0] subframe_w;

reg [5:0]   bit_count_q;
reg         bit_toggle_q;

// Xilinx: Place output flop in IOB
//synthesis attribute IOB of spdif_out_q is "TRUE"
reg         spdif_out_q;

reg [5:0]   parity_count_q;

//-----------------------------------------------------------------
// Subframe Counter
//-----------------------------------------------------------------
always @ (posedge rst_i or posedge clk_i )
begin
    if (rst_i == 1'b1)
        subframe_count_q    <= 9'd0;
    else if (load_subframe_q)
    begin
        // 192 frames (384 subframes) in an audio block
        if (subframe_count_q == 9'd383)
            subframe_count_q <= 9'd0;
        else
            subframe_count_q <= subframe_count_q + 9'd1;
    end
end

//-----------------------------------------------------------------
// Sample capture
//-----------------------------------------------------------------
reg [15:0] sample_buf_q;

always @ (posedge rst_i or posedge clk_i )
begin
   if (rst_i == 1'b1)
   begin
        audio_sample_q      <= 16'h0000;
        sample_buf_q        <= 16'h0000;
        sample_req_o        <= 1'b0;
   end
   else if (load_subframe_q)
   begin
        // Start of frame (first subframe)?
        if (subframe_count_q[0] == 1'b0)
        begin
            // Use left sample
            audio_sample_q <= sample_i[15:0];

            // Store right sample
            sample_buf_q <= sample_i[31:16];

            // Request next sample
            sample_req_o <= 1'b1;
        end
        else
        begin
            // Use right sample
            audio_sample_q <= sample_buf_q;

            sample_req_o <= 1'b0;
        end
   end
   else
        sample_req_o <= 1'b0;
end

// Timeslots 3 - 0 = Preamble
assign subframe_w[3:0] = 4'b0000;

// Timeslots 7 - 4 = 24-bit audio LSB
assign subframe_w[7:4] = 4'b0000;

// Timeslots 11 - 8 = 20-bit audio LSB
assign subframe_w[11:8] = 4'b0000;

// Timeslots 27 - 12 = 16-bit audio
assign subframe_w[27:12] = audio_sample_q;

// Timeslots 28 = Validity
assign subframe_w[28] = 1'b0; // Valid

// Timeslots 29 = User bit
assign subframe_w[29] = 1'b0;

// Timeslots 30 = Channel status bit
assign subframe_w[30] = 1'b0;

// Timeslots 31 = Even Parity bit (31:4)
assign subframe_w[31] = 1'b0;

//-----------------------------------------------------------------
// Preamble
//-----------------------------------------------------------------
localparam PREAMBLE_Z = 8'b00010111;
localparam PREAMBLE_Y = 8'b00100111;
localparam PREAMBLE_X = 8'b01000111;

reg [7:0] preamble_r;

always @ *
begin
    // Start of audio block?
    // Z(B) - Left channel
    if (subframe_count_q == 9'd0)
        preamble_r = PREAMBLE_Z; // Z(B)
    // Right Channel?
    else if (subframe_count_q[0] == 1'b1)
        preamble_r = PREAMBLE_Y; // Y(W)
    // Left Channel (but not start of block)?
    else
        preamble_r = PREAMBLE_X; // X(M)
end

always @ (posedge rst_i or posedge clk_i )
if (rst_i == 1'b1)
    preamble_q  <= 8'h00;
else if (load_subframe_q)
    preamble_q  <= preamble_r;

//-----------------------------------------------------------------
// Parity Counter
//-----------------------------------------------------------------
always @ (posedge rst_i or posedge clk_i )
begin
   if (rst_i == 1'b1)
   begin
        parity_count_q  <= 6'd0;
   end
   // Time to output a bit?
   else if (bit_out_en_i)
   begin
        // Preamble bits?
        if (bit_count_q < 6'd8)
        begin
            parity_count_q  <= 6'd0;
        end
        // Normal timeslots
        else if (bit_count_q < 6'd62)
        begin
            // On first pass through this timeslot, count number of high bits
            if (bit_count_q[0] == 0 && subframe_w[bit_count_q / 2] == 1'b1)
                parity_count_q <= parity_count_q + 6'd1;
        end
   end
end

//-----------------------------------------------------------------
// Bit Counter
//-----------------------------------------------------------------
always @ (posedge rst_i or posedge clk_i)
begin
    if (rst_i == 1'b1)
    begin
        bit_count_q     <= 6'b0;
        load_subframe_q <= 1'b1;
    end
    // Time to output a bit?
    else if (bit_out_en_i)
    begin
        // 32 timeslots (x2 for double frequency)
        if (bit_count_q == 6'd63)
        begin
            bit_count_q     <= 6'd0;
            load_subframe_q <= 1'b1;
        end
        else
        begin
            bit_count_q     <= bit_count_q + 6'd1;
            load_subframe_q <= 1'b0;
        end
    end
    else
        load_subframe_q <= 1'b0;
end

//-----------------------------------------------------------------
// Bit half toggle
//-----------------------------------------------------------------
always @ (posedge rst_i or posedge clk_i)
if (rst_i == 1'b1)
    bit_toggle_q    <= 1'b0;
// Time to output a bit?
else if (bit_out_en_i)
    bit_toggle_q <= ~bit_toggle_q;

//-----------------------------------------------------------------
// Output bit (BMC encoded)
//-----------------------------------------------------------------
reg bit_r;

always @ *
begin
    bit_r = spdif_out_q;

    // Time to output a bit?
    if (bit_out_en_i)
    begin
        // Preamble bits?
        if (bit_count_q < 6'd8)
        begin
            bit_r = preamble_q[bit_count_q[2:0]];
        end
        // Normal timeslots
        else if (bit_count_q < 6'd62)
        begin
            if (subframe_w[bit_count_q / 2] == 1'b0)
            begin
                if (bit_toggle_q == 1'b0)
                    bit_r = ~spdif_out_q;
                else
                    bit_r = spdif_out_q;
            end
            else
                bit_r = ~spdif_out_q;
        end
        // Parity timeslot
        else
        begin
            // Even number of high bits, make odd
            if (parity_count_q[0] == 1'b0)
            begin
                if (bit_toggle_q == 1'b0)
                    bit_r = ~spdif_out_q;
                else
                    bit_r = spdif_out_q;
            end
            else
                bit_r = ~spdif_out_q;
        end
    end
end

always @ (posedge rst_i or posedge clk_i )
if (rst_i == 1'b1)
    spdif_out_q <= 1'b0;
else
    spdif_out_q <= bit_r;

assign spdif_o  = spdif_out_q;

endmodule
//-----------------------------------------------------------------
//                      Audio Controller
//                           V0.1
//                     Ultra-Embedded.com
//                     Copyright 2012-2019
//
//                 Email: admin@ultra-embedded.com
//
//                         License: GPL
// If you would like a version with a more permissive license for
// use in closed source commercial applications please contact me
// for details.
//-----------------------------------------------------------------
//
// This file is open source HDL; you can redistribute it and/or 
// modify it under the terms of the GNU General Public License as 
// published by the Free Software Foundation; either version 2 of 
// the License, or (at your option) any later version.
//
// This file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public 
// License along with this file; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
//-----------------------------------------------------------------

//-----------------------------------------------------------------
//                          Generated File
//-----------------------------------------------------------------

// `include "audio_defs.v"

//-----------------------------------------------------------------
// Module:  Audio Controller
//-----------------------------------------------------------------
module audio
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

    // Outputs
    ,output         cfg_awready_o
    ,output         cfg_wready_o
    ,output         cfg_bvalid_o
    ,output [1:0]   cfg_bresp_o
    ,output         cfg_arready_o
    ,output         cfg_rvalid_o
    ,output [31:0]  cfg_rdata_o
    ,output [1:0]   cfg_rresp_o
    ,output         audio_l_o
    ,output         audio_r_o
    ,output         spdif_o
    ,output         i2s_sck_o
    ,output         i2s_sdata_o
    ,output         i2s_ws_o
    ,output         intr_o
);

//-----------------------------------------------------------------
// Retime write data
//-----------------------------------------------------------------
reg [31:0] wr_data_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    wr_data_q <= 32'b0;
else
    wr_data_q <= cfg_wdata_i;

//-----------------------------------------------------------------
// Request Logic
//-----------------------------------------------------------------
wire read_en_w  = cfg_arvalid_i & cfg_arready_o;
wire write_en_w = cfg_awvalid_i & cfg_awready_o;

//-----------------------------------------------------------------
// Accept Logic
//-----------------------------------------------------------------
assign cfg_arready_o = ~cfg_rvalid_o;
assign cfg_awready_o = ~cfg_bvalid_o && ~cfg_arvalid_i; 
assign cfg_wready_o  = cfg_awready_o;


//-----------------------------------------------------------------
// Register audio_cfg
//-----------------------------------------------------------------
reg audio_cfg_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    audio_cfg_wr_q <= 1'b0;
else if (write_en_w && (cfg_awaddr_i[7:0] == `AUDIO_CFG))
    audio_cfg_wr_q <= 1'b1;
else
    audio_cfg_wr_q <= 1'b0;

// audio_cfg_int_threshold [internal]
reg [15:0]  audio_cfg_int_threshold_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    audio_cfg_int_threshold_q <= 16'd`AUDIO_CFG_INT_THRESHOLD_DEFAULT;
else if (write_en_w && (cfg_awaddr_i[7:0] == `AUDIO_CFG))
    audio_cfg_int_threshold_q <= cfg_wdata_i[`AUDIO_CFG_INT_THRESHOLD_R];

wire [15:0]  audio_cfg_int_threshold_out_w = audio_cfg_int_threshold_q;


// audio_cfg_byte_swap [internal]
reg        audio_cfg_byte_swap_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    audio_cfg_byte_swap_q <= 1'd`AUDIO_CFG_BYTE_SWAP_DEFAULT;
else if (write_en_w && (cfg_awaddr_i[7:0] == `AUDIO_CFG))
    audio_cfg_byte_swap_q <= cfg_wdata_i[`AUDIO_CFG_BYTE_SWAP_R];

wire        audio_cfg_byte_swap_out_w = audio_cfg_byte_swap_q;


// audio_cfg_ch_swap [internal]
reg        audio_cfg_ch_swap_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    audio_cfg_ch_swap_q <= 1'd`AUDIO_CFG_CH_SWAP_DEFAULT;
else if (write_en_w && (cfg_awaddr_i[7:0] == `AUDIO_CFG))
    audio_cfg_ch_swap_q <= cfg_wdata_i[`AUDIO_CFG_CH_SWAP_R];

wire        audio_cfg_ch_swap_out_w = audio_cfg_ch_swap_q;


// audio_cfg_target [internal]
reg [1:0]  audio_cfg_target_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    audio_cfg_target_q <= 2'd`AUDIO_CFG_TARGET_DEFAULT;
else if (write_en_w && (cfg_awaddr_i[7:0] == `AUDIO_CFG))
    audio_cfg_target_q <= cfg_wdata_i[`AUDIO_CFG_TARGET_R];

wire [1:0]  audio_cfg_target_out_w = audio_cfg_target_q;


// audio_cfg_vol_ctrl [internal]
reg [2:0]  audio_cfg_vol_ctrl_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    audio_cfg_vol_ctrl_q <= 3'd`AUDIO_CFG_VOL_CTRL_DEFAULT;
else if (write_en_w && (cfg_awaddr_i[7:0] == `AUDIO_CFG))
    audio_cfg_vol_ctrl_q <= cfg_wdata_i[`AUDIO_CFG_VOL_CTRL_R];

wire [2:0]  audio_cfg_vol_ctrl_out_w = audio_cfg_vol_ctrl_q;


// audio_cfg_buffer_rst [auto_clr]
reg        audio_cfg_buffer_rst_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    audio_cfg_buffer_rst_q <= 1'd`AUDIO_CFG_BUFFER_RST_DEFAULT;
else if (write_en_w && (cfg_awaddr_i[7:0] == `AUDIO_CFG))
    audio_cfg_buffer_rst_q <= cfg_wdata_i[`AUDIO_CFG_BUFFER_RST_R];
else
    audio_cfg_buffer_rst_q <= 1'd`AUDIO_CFG_BUFFER_RST_DEFAULT;

wire        audio_cfg_buffer_rst_out_w = audio_cfg_buffer_rst_q;


//-----------------------------------------------------------------
// Register audio_status
//-----------------------------------------------------------------
reg audio_status_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    audio_status_wr_q <= 1'b0;
else if (write_en_w && (cfg_awaddr_i[7:0] == `AUDIO_STATUS))
    audio_status_wr_q <= 1'b1;
else
    audio_status_wr_q <= 1'b0;




//-----------------------------------------------------------------
// Register audio_clk_div
//-----------------------------------------------------------------
reg audio_clk_div_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    audio_clk_div_wr_q <= 1'b0;
else if (write_en_w && (cfg_awaddr_i[7:0] == `AUDIO_CLK_DIV))
    audio_clk_div_wr_q <= 1'b1;
else
    audio_clk_div_wr_q <= 1'b0;

// audio_clk_div_whole_cycles [internal]
reg [15:0]  audio_clk_div_whole_cycles_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    audio_clk_div_whole_cycles_q <= 16'd`AUDIO_CLK_DIV_WHOLE_CYCLES_DEFAULT;
else if (write_en_w && (cfg_awaddr_i[7:0] == `AUDIO_CLK_DIV))
    audio_clk_div_whole_cycles_q <= cfg_wdata_i[`AUDIO_CLK_DIV_WHOLE_CYCLES_R];

wire [15:0]  audio_clk_div_whole_cycles_out_w = audio_clk_div_whole_cycles_q;


//-----------------------------------------------------------------
// Register audio_clk_frac
//-----------------------------------------------------------------
reg audio_clk_frac_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    audio_clk_frac_wr_q <= 1'b0;
else if (write_en_w && (cfg_awaddr_i[7:0] == `AUDIO_CLK_FRAC))
    audio_clk_frac_wr_q <= 1'b1;
else
    audio_clk_frac_wr_q <= 1'b0;

// audio_clk_frac_numerator [internal]
reg [15:0]  audio_clk_frac_numerator_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    audio_clk_frac_numerator_q <= 16'd`AUDIO_CLK_FRAC_NUMERATOR_DEFAULT;
else if (write_en_w && (cfg_awaddr_i[7:0] == `AUDIO_CLK_FRAC))
    audio_clk_frac_numerator_q <= cfg_wdata_i[`AUDIO_CLK_FRAC_NUMERATOR_R];

wire [15:0]  audio_clk_frac_numerator_out_w = audio_clk_frac_numerator_q;


// audio_clk_frac_denominator [internal]
reg [15:0]  audio_clk_frac_denominator_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    audio_clk_frac_denominator_q <= 16'd`AUDIO_CLK_FRAC_DENOMINATOR_DEFAULT;
else if (write_en_w && (cfg_awaddr_i[7:0] == `AUDIO_CLK_FRAC))
    audio_clk_frac_denominator_q <= cfg_wdata_i[`AUDIO_CLK_FRAC_DENOMINATOR_R];

wire [15:0]  audio_clk_frac_denominator_out_w = audio_clk_frac_denominator_q;


//-----------------------------------------------------------------
// Register audio_fifo_write
//-----------------------------------------------------------------
reg audio_fifo_write_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    audio_fifo_write_wr_q <= 1'b0;
else if (write_en_w && (cfg_awaddr_i[7:0] >= `AUDIO_FIFO_WRITE && cfg_awaddr_i[7:0] < (`AUDIO_FIFO_WRITE + 8'd32)))
    audio_fifo_write_wr_q <= 1'b1;
else
    audio_fifo_write_wr_q <= 1'b0;

// audio_fifo_write_ch_b [external]
wire [15:0]  audio_fifo_write_ch_b_out_w = wr_data_q[`AUDIO_FIFO_WRITE_CH_B_R];


// audio_fifo_write_ch_a [external]
wire [15:0]  audio_fifo_write_ch_a_out_w = wr_data_q[`AUDIO_FIFO_WRITE_CH_A_R];


wire [15:0]  audio_status_level_in_w;
wire        audio_status_full_in_w;
wire        audio_status_empty_in_w;


//-----------------------------------------------------------------
// Read mux
//-----------------------------------------------------------------
reg [31:0] data_r;

always @ *
begin
    data_r = 32'b0;

    case (cfg_araddr_i[7:0])

    `AUDIO_CFG:
    begin
        data_r[`AUDIO_CFG_INT_THRESHOLD_R] = audio_cfg_int_threshold_q;
        data_r[`AUDIO_CFG_BYTE_SWAP_R] = audio_cfg_byte_swap_q;
        data_r[`AUDIO_CFG_CH_SWAP_R] = audio_cfg_ch_swap_q;
        data_r[`AUDIO_CFG_TARGET_R] = audio_cfg_target_q;
        data_r[`AUDIO_CFG_VOL_CTRL_R] = audio_cfg_vol_ctrl_q;
    end
    `AUDIO_STATUS:
    begin
        data_r[`AUDIO_STATUS_LEVEL_R] = audio_status_level_in_w;
        data_r[`AUDIO_STATUS_FULL_R] = audio_status_full_in_w;
        data_r[`AUDIO_STATUS_EMPTY_R] = audio_status_empty_in_w;
    end
    `AUDIO_CLK_DIV:
    begin
        data_r[`AUDIO_CLK_DIV_WHOLE_CYCLES_R] = audio_clk_div_whole_cycles_q;
    end
    `AUDIO_CLK_FRAC:
    begin
        data_r[`AUDIO_CLK_FRAC_NUMERATOR_R] = audio_clk_frac_numerator_q;
        data_r[`AUDIO_CLK_FRAC_DENOMINATOR_R] = audio_clk_frac_denominator_q;
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


wire audio_fifo_write_wr_req_w = audio_fifo_write_wr_q;

`define TARGET_W      2
`define TARGET_I2S    2'd0
`define TARGET_SPDIF  2'd1
`define TARGET_ANALOG 2'd2

//-----------------------------------------------------------------
// Write samples
//-----------------------------------------------------------------   
reg [15:0] sample_a_r;
reg [15:0] sample_b_r;

always @ *
begin
    sample_a_r = audio_fifo_write_ch_a_out_w;
    sample_b_r = audio_fifo_write_ch_b_out_w;

    if (audio_cfg_ch_swap_out_w)
    begin
        if (audio_cfg_byte_swap_out_w)
        begin
            sample_a_r = {audio_fifo_write_ch_b_out_w[7:0], audio_fifo_write_ch_b_out_w[15:8]};
            sample_b_r = {audio_fifo_write_ch_a_out_w[7:0], audio_fifo_write_ch_a_out_w[15:8]};
        end
        else
        begin
            sample_a_r = audio_fifo_write_ch_b_out_w;
            sample_b_r = audio_fifo_write_ch_a_out_w;
        end
    end
    else
    begin
        if (audio_cfg_byte_swap_out_w)
        begin
            sample_a_r = {audio_fifo_write_ch_a_out_w[7:0], audio_fifo_write_ch_a_out_w[15:8]};
            sample_b_r = {audio_fifo_write_ch_b_out_w[7:0], audio_fifo_write_ch_b_out_w[15:8]};
        end
        else
        begin
            sample_a_r = audio_fifo_write_ch_a_out_w;
            sample_b_r = audio_fifo_write_ch_b_out_w;
        end
    end
end


wire        fifo_valid_w;
wire [31:0] fifo_data_w;
reg         fifo_pop_r;
wire        fifo_accept_w;

audio_fifo
u_fifo
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    ,.flush_i(audio_cfg_buffer_rst_out_w)

    ,.push_i(audio_fifo_write_wr_q)
    ,.data_in_i({sample_b_r, sample_a_r})
    ,.accept_o(fifo_accept_w)

    ,.valid_o(fifo_valid_w)
    ,.data_out_o(fifo_data_w)
    ,.pop_i(fifo_pop_r)
);

reg [15:0] fifo_level_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    fifo_level_q <= 16'b0;
else if (audio_cfg_buffer_rst_out_w)
    fifo_level_q <= 16'b0;
// Count up
else if ((audio_fifo_write_wr_q & fifo_accept_w) & ~(fifo_pop_r & fifo_valid_w))
    fifo_level_q <= fifo_level_q + 16'd1;
// Count down
else if (~(audio_fifo_write_wr_q & fifo_accept_w) & (fifo_pop_r & fifo_valid_w))
    fifo_level_q <= fifo_level_q - 16'd1;

assign audio_status_level_in_w = fifo_level_q;
assign audio_status_full_in_w  = ~fifo_accept_w;
assign audio_status_empty_in_w = ~fifo_valid_w;

//-----------------------------------------------------------------
// Interrupt Output
//-----------------------------------------------------------------  
reg irq_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    irq_q <= 1'b0;
else
    irq_q <= (audio_status_level_in_w <= audio_cfg_int_threshold_out_w);

assign intr_o = irq_q;

//-----------------------------------------------------------------
// Clock Generator
//-----------------------------------------------------------------  
reg        clk_out_q;
reg [15:0] clk_div_q;
reg [15:0] clk_div_r;
reg [31:0] fraction_q;
reg [31:0] fraction_r;

/* verilator lint_off WIDTH */
always @ *
begin
    clk_div_r = clk_div_q;
    fraction_r = fraction_q;

    case (clk_div_q)
    0 :
    begin
        clk_div_r = clk_div_q + 16'd1;
    end
    audio_clk_div_whole_cycles_out_w - 16'd1:
    begin
        if (fraction_q < (audio_clk_frac_denominator_out_w - audio_clk_frac_numerator_out_w))
        begin
            fraction_r = fraction_q + audio_clk_frac_numerator_out_w;
            clk_div_r  = 16'd0;
        end
        else
        begin
            fraction_r = fraction_q + audio_clk_frac_numerator_out_w - audio_clk_frac_denominator_out_w;
            clk_div_r = clk_div_q + 16'd1;
        end
    end
    audio_clk_div_whole_cycles_out_w:
    begin
        clk_div_r = 16'd0;
    end

    default:
    begin
        clk_div_r = clk_div_q + 16'd1;
    end
    endcase
end
/* verilator lint_on WIDTH */

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    clk_div_q <= 16'd0;
else
    clk_div_q <= clk_div_r;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    fraction_q <= 32'd0;
else
    fraction_q <= fraction_r;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    clk_out_q <= 1'b0;
else
    clk_out_q <= (clk_div_q == 16'd0);

//-----------------------------------------------------------------
// vol_adjust: Reduce 16-bit sample amplitude by some ammount.
// Inputs: x = input, y = volume control
// Return: Volume adjusted value
//-----------------------------------------------------------------
function [15:0] vol_adjust;
    input  [15:0] x;
    input  [2:0] y;
    reg [15:0] val;
begin 

    case (y)
   //-------------------------------
   // Max volume (Vol / 0)
   //-------------------------------
   0 : 
   begin
         val = x;
   end
   //-------------------------------
   // Volume / 2
   //-------------------------------
   1 : 
   begin
        // x >> 1
        if (x[15] == 1'b1)
             val = {1'b1,x[15:1]};
        else
             val = {1'b0,x[15:1]};
   end   
   //-------------------------------
   // Volume / 4
   //-------------------------------
   2 : 
   begin
        // x >> 2
        if (x[15] == 1'b1)
             val = {2'b11,x[15:2]};
        else
             val = {2'b00,x[15:2]};
   end
   //-------------------------------
   // Volume / 8
   //-------------------------------
   3 : 
   begin
        // x >> 3
        if (x[15] == 1'b1)
             val = {3'b111,x[15:3]};
        else
             val = {3'b000,x[15:3]};
   end
   //-------------------------------
   // Volume / 16
   //-------------------------------
   4 : 
   begin
        // x >> 4
        if (x[15] == 1'b1)
             val = {4'b1111,x[15:4]};
        else
             val = {4'b0000,x[15:4]};
   end   
   //-------------------------------
   // Volume / 32
   //-------------------------------
   5 : 
   begin
        // x >> 5
        if (x[15] == 1'b1)
             val = {5'b11111,x[15:5]};
        else
             val = {5'b00000,x[15:5]};
   end    
   //-------------------------------
   // Volume / 64
   //-------------------------------
   6 : 
   begin
        // x >> 6
        if (x[15] == 1'b1)
             val = {6'b111111,x[15:6]};
        else
             val = {6'b000000,x[15:6]};         
   end
   //-------------------------------
   // Volume / 128
   //-------------------------------
   7 : 
   begin
        // x >> 7
        if (x[15] == 1'b1)
             val = {7'b1111111,x[15:7]};
        else
             val = {7'b0000000,x[15:7]};         
   end   
   default : 
        val = x;
   endcase

   vol_adjust = val;
end
endfunction

//-----------------------------------------------------------------
// Output
//-----------------------------------------------------------------
wire [15:0] output_b_w = vol_adjust(fifo_data_w[31:16], audio_cfg_vol_ctrl_out_w);
wire [15:0] output_a_w = vol_adjust(fifo_data_w[15:0],  audio_cfg_vol_ctrl_out_w);

//-----------------------------------------------------------------
// I2S
//-----------------------------------------------------------------
wire i2s_accept_w;

audio_i2s
u_i2s
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    ,.audio_clk_i(clk_out_q && (audio_cfg_target_out_w == `TARGET_I2S))

    ,.inport_tvalid_i(fifo_valid_w && (audio_cfg_target_out_w == `TARGET_I2S))
    ,.inport_tdata_i({output_b_w, output_a_w})
    ,.inport_tstrb_i(4'hF)
    ,.inport_tdest_i(4'b0)
    ,.inport_tlast_i(1'b0)
    ,.inport_tready_o(i2s_accept_w)

    ,.i2s_sck_o(i2s_sck_o)
    ,.i2s_sdata_o(i2s_sdata_o)
    ,.i2s_ws_o(i2s_ws_o)
);

//-----------------------------------------------------------------
// SPDIF
//-----------------------------------------------------------------
wire spdif_accept_w;

audio_spdif
u_spdif
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    ,.audio_clk_i(clk_out_q && (audio_cfg_target_out_w == `TARGET_SPDIF))

    ,.inport_tvalid_i(fifo_valid_w && (audio_cfg_target_out_w == `TARGET_SPDIF))
    ,.inport_tdata_i({output_b_w, output_a_w})
    ,.inport_tstrb_i(4'hF)
    ,.inport_tdest_i(4'b0)
    ,.inport_tlast_i(1'b0)
    ,.inport_tready_o(spdif_accept_w)

    ,.spdif_o(spdif_o)
);

//-----------------------------------------------------------------
// Analog (sigma delta DAC)
//-----------------------------------------------------------------
wire analog_accept_w;

audio_dac
u_dac
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    ,.audio_clk_i(clk_out_q && (audio_cfg_target_out_w == `TARGET_ANALOG))

    ,.inport_tvalid_i(fifo_valid_w && (audio_cfg_target_out_w == `TARGET_ANALOG))
    ,.inport_tdata_i({output_b_w, output_a_w})
    ,.inport_tstrb_i(4'hF)
    ,.inport_tdest_i(4'b0)
    ,.inport_tlast_i(1'b0)
    ,.inport_tready_o(analog_accept_w)

    ,.audio_l_o(audio_l_o)
    ,.audio_r_o(audio_r_o)
);


always @ *
begin
    fifo_pop_r = 1'b0;

    case (audio_cfg_target_out_w)
    `TARGET_I2S:    fifo_pop_r = i2s_accept_w;
    `TARGET_SPDIF:  fifo_pop_r = spdif_accept_w;
    `TARGET_ANALOG: fifo_pop_r = analog_accept_w;
    default:        fifo_pop_r = 1'b0;
    endcase
end


endmodule
