//-----------------------------------------------------------------
//                      Baseline JPEG Decoder
//                             V0.1
//                       Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//-----------------------------------------------------------------
//                      License: Apache 2.0
// This IP can be freely used in commercial projects, however you may
// want access to unreleased materials such as verification environments,
// or test vectors, as well as changes to the IP for integration purposes.
// If this is the case, contact the above address.
// I am interested to hear how and where this IP is used, so please get
// in touch!
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

module jpeg_bitbuffer
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           img_start_i
    ,input           img_end_i
    ,input           inport_valid_i
    ,input  [  7:0]  inport_data_i
    ,input           inport_last_i
    ,input  [  5:0]  outport_pop_i

    // Outputs
    ,output          inport_accept_o
    ,output          outport_valid_o
    ,output [ 31:0]  outport_data_o
    ,output          outport_last_o
);



//-----------------------------------------------------------------
// Registers
//-----------------------------------------------------------------
reg [7:0] ram_q[7:0];
reg [5:0] rd_ptr_q;
reg [5:0] wr_ptr_q;
reg [6:0] count_q;
reg       drain_q;

//-----------------------------------------------------------------
// Input side FIFO
//-----------------------------------------------------------------
reg [6:0] count_r; 
always @ *
begin
    count_r = count_q;

    // Count up
    if (inport_valid_i && inport_accept_o)
        count_r = count_r + 7'd8;

    // Count down
    if (outport_valid_o && (|outport_pop_i))
        count_r = count_r - outport_pop_i;
end

always @ (posedge clk_i )
if (rst_i)
begin
    count_q   <= 7'b0;
    rd_ptr_q  <= 6'b0;
    wr_ptr_q  <= 6'b0;
    drain_q   <= 1'b0;
end
else if (img_start_i)
begin
    count_q   <= 7'b0;
    rd_ptr_q  <= 6'b0;
    wr_ptr_q  <= 6'b0;
    drain_q   <= 1'b0;
end
else
begin
    // End of image
    if (inport_last_i)
        drain_q <= 1'b1;

    // Push
    if (inport_valid_i && inport_accept_o)
    begin
        ram_q[wr_ptr_q[5:3]] <= inport_data_i;
        wr_ptr_q             <= wr_ptr_q + 6'd8;
    end

    // Pop
    if (outport_valid_o && (|outport_pop_i))
        rd_ptr_q <= rd_ptr_q + outport_pop_i;

    count_q <= count_r;
end

assign inport_accept_o = (count_q <= 7'd56);

//-------------------------------------------------------------------
// Output side FIFO
//-------------------------------------------------------------------
reg [39:0] fifo_data_r;

always @ *
begin
    fifo_data_r = 40'b0;

    case (rd_ptr_q[5:3])
    3'd0: fifo_data_r = {ram_q[0], ram_q[1], ram_q[2], ram_q[3], ram_q[4]};
    3'd1: fifo_data_r = {ram_q[1], ram_q[2], ram_q[3], ram_q[4], ram_q[5]};
    3'd2: fifo_data_r = {ram_q[2], ram_q[3], ram_q[4], ram_q[5], ram_q[6]};
    3'd3: fifo_data_r = {ram_q[3], ram_q[4], ram_q[5], ram_q[6], ram_q[7]};
    3'd4: fifo_data_r = {ram_q[4], ram_q[5], ram_q[6], ram_q[7], ram_q[0]};
    3'd5: fifo_data_r = {ram_q[5], ram_q[6], ram_q[7], ram_q[0], ram_q[1]};
    3'd6: fifo_data_r = {ram_q[6], ram_q[7], ram_q[0], ram_q[1], ram_q[2]};
    3'd7: fifo_data_r = {ram_q[7], ram_q[0], ram_q[1], ram_q[2], ram_q[3]};
    endcase
end

wire [39:0] data_shifted_w = fifo_data_r << rd_ptr_q[2:0];

assign outport_valid_o  = (count_q >= 7'd32) || (drain_q && count_q != 7'd0);
assign outport_data_o   = data_shifted_w[39:8];
assign outport_last_o   = 1'b0;


`ifdef verilator
function get_valid; /*verilator public*/
begin
    get_valid = inport_valid_i && inport_accept_o;
end
endfunction
function [7:0] get_data; /*verilator public*/
begin
    get_data = inport_data_i;
end
endfunction
`endif




endmodule
//-----------------------------------------------------------------
//                      Baseline JPEG Decoder
//                             V0.1
//                       Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//-----------------------------------------------------------------
//                      License: Apache 2.0
// This IP can be freely used in commercial projects, however you may
// want access to unreleased materials such as verification environments,
// or test vectors, as well as changes to the IP for integration purposes.
// If this is the case, contact the above address.
// I am interested to hear how and where this IP is used, so please get
// in touch!
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
module jpeg_dht_std_cx_ac
(
     input  [ 15:0]  lookup_input_i
    ,output [  4:0]  lookup_width_o
    ,output [  7:0]  lookup_value_o
);

//-----------------------------------------------------------------
// Cx AC Table (standard)
//-----------------------------------------------------------------
reg [7:0] cx_ac_value_r;
reg [4:0] cx_ac_width_r;

always @ *
begin
    cx_ac_value_r = 8'b0;
    cx_ac_width_r = 5'b0;

    if (lookup_input_i[15:14] == 2'h0)
    begin
         cx_ac_value_r = 8'h00;
         cx_ac_width_r = 5'd2;
    end
    else if (lookup_input_i[15:14] == 2'h1)
    begin
         cx_ac_value_r = 8'h01;
         cx_ac_width_r = 5'd2;
    end
    else if (lookup_input_i[15:13] == 3'h4)
    begin
         cx_ac_value_r = 8'h02;
         cx_ac_width_r = 5'd3;
    end
    else if (lookup_input_i[15:12] == 4'ha)
    begin
         cx_ac_value_r = 8'h03;
         cx_ac_width_r = 5'd4;
    end
    else if (lookup_input_i[15:12] == 4'hb)
    begin
         cx_ac_value_r = 8'h11;
         cx_ac_width_r = 5'd4;
    end
    else if (lookup_input_i[15:11] == 5'h18)
    begin
         cx_ac_value_r = 8'h04;
         cx_ac_width_r = 5'd5;
    end
    else if (lookup_input_i[15:11] == 5'h19)
    begin
         cx_ac_value_r = 8'h05;
         cx_ac_width_r = 5'd5;
    end
    else if (lookup_input_i[15:11] == 5'h1a)
    begin
         cx_ac_value_r = 8'h21;
         cx_ac_width_r = 5'd5;
    end
    else if (lookup_input_i[15:11] == 5'h1b)
    begin
         cx_ac_value_r = 8'h31;
         cx_ac_width_r = 5'd5;
    end
    else if (lookup_input_i[15:10] == 6'h38)
    begin
         cx_ac_value_r = 8'h06;
         cx_ac_width_r = 5'd6;
    end
    else if (lookup_input_i[15:10] == 6'h39)
    begin
         cx_ac_value_r = 8'h12;
         cx_ac_width_r = 5'd6;
    end
    else if (lookup_input_i[15:10] == 6'h3a)
    begin
         cx_ac_value_r = 8'h41;
         cx_ac_width_r = 5'd6;
    end
    else if (lookup_input_i[15:10] == 6'h3b)
    begin
         cx_ac_value_r = 8'h51;
         cx_ac_width_r = 5'd6;
    end
    else if (lookup_input_i[15:9] == 7'h78)
    begin
         cx_ac_value_r = 8'h07;
         cx_ac_width_r = 5'd7;
    end
    else if (lookup_input_i[15:9] == 7'h79)
    begin
         cx_ac_value_r = 8'h61;
         cx_ac_width_r = 5'd7;
    end
    else if (lookup_input_i[15:9] == 7'h7a)
    begin
         cx_ac_value_r = 8'h71;
         cx_ac_width_r = 5'd7;
    end
    else if (lookup_input_i[15:8] == 8'hf6)
    begin
         cx_ac_value_r = 8'h13;
         cx_ac_width_r = 5'd8;
    end
    else if (lookup_input_i[15:8] == 8'hf7)
    begin
         cx_ac_value_r = 8'h22;
         cx_ac_width_r = 5'd8;
    end
    else if (lookup_input_i[15:8] == 8'hf8)
    begin
         cx_ac_value_r = 8'h32;
         cx_ac_width_r = 5'd8;
    end
    else if (lookup_input_i[15:8] == 8'hf9)
    begin
         cx_ac_value_r = 8'h81;
         cx_ac_width_r = 5'd8;
    end
    else if (lookup_input_i[15:7] == 9'h1f4)
    begin
         cx_ac_value_r = 8'h08;
         cx_ac_width_r = 5'd9;
    end
    else if (lookup_input_i[15:7] == 9'h1f5)
    begin
         cx_ac_value_r = 8'h14;
         cx_ac_width_r = 5'd9;
    end
    else if (lookup_input_i[15:7] == 9'h1f6)
    begin
         cx_ac_value_r = 8'h42;
         cx_ac_width_r = 5'd9;
    end
    else if (lookup_input_i[15:7] == 9'h1f7)
    begin
         cx_ac_value_r = 8'h91;
         cx_ac_width_r = 5'd9;
    end
    else if (lookup_input_i[15:7] == 9'h1f8)
    begin
         cx_ac_value_r = 8'ha1;
         cx_ac_width_r = 5'd9;
    end
    else if (lookup_input_i[15:7] == 9'h1f9)
    begin
         cx_ac_value_r = 8'hb1;
         cx_ac_width_r = 5'd9;
    end
    else if (lookup_input_i[15:7] == 9'h1fa)
    begin
         cx_ac_value_r = 8'hc1;
         cx_ac_width_r = 5'd9;
    end
    else if (lookup_input_i[15:6] == 10'h3f6)
    begin
         cx_ac_value_r = 8'h09;
         cx_ac_width_r = 5'd10;
    end
    else if (lookup_input_i[15:6] == 10'h3f7)
    begin
         cx_ac_value_r = 8'h23;
         cx_ac_width_r = 5'd10;
    end
    else if (lookup_input_i[15:6] == 10'h3f8)
    begin
         cx_ac_value_r = 8'h33;
         cx_ac_width_r = 5'd10;
    end
    else if (lookup_input_i[15:6] == 10'h3f9)
    begin
         cx_ac_value_r = 8'h52;
         cx_ac_width_r = 5'd10;
    end
    else if (lookup_input_i[15:6] == 10'h3fa)
    begin
         cx_ac_value_r = 8'hf0;
         cx_ac_width_r = 5'd10;
    end
    else if (lookup_input_i[15:5] == 11'h7f6)
    begin
         cx_ac_value_r = 8'h15;
         cx_ac_width_r = 5'd11;
    end
    else if (lookup_input_i[15:5] == 11'h7f7)
    begin
         cx_ac_value_r = 8'h62;
         cx_ac_width_r = 5'd11;
    end
    else if (lookup_input_i[15:5] == 11'h7f8)
    begin
         cx_ac_value_r = 8'h72;
         cx_ac_width_r = 5'd11;
    end
    else if (lookup_input_i[15:5] == 11'h7f9)
    begin
         cx_ac_value_r = 8'hd1;
         cx_ac_width_r = 5'd11;
    end
    else if (lookup_input_i[15:4] == 12'hff4)
    begin
         cx_ac_value_r = 8'h0a;
         cx_ac_width_r = 5'd12;
    end
    else if (lookup_input_i[15:4] == 12'hff5)
    begin
         cx_ac_value_r = 8'h16;
         cx_ac_width_r = 5'd12;
    end
    else if (lookup_input_i[15:4] == 12'hff6)
    begin
         cx_ac_value_r = 8'h24;
         cx_ac_width_r = 5'd12;
    end
    else if (lookup_input_i[15:4] == 12'hff7)
    begin
         cx_ac_value_r = 8'h34;
         cx_ac_width_r = 5'd12;
    end
    else if (lookup_input_i[15:2] == 14'h3fe0)
    begin
         cx_ac_value_r = 8'he1;
         cx_ac_width_r = 5'd14;
    end
    else if (lookup_input_i[15:1] == 15'h7fc2)
    begin
         cx_ac_value_r = 8'h25;
         cx_ac_width_r = 5'd15;
    end
    else if (lookup_input_i[15:1] == 15'h7fc3)
    begin
         cx_ac_value_r = 8'hf1;
         cx_ac_width_r = 5'd15;
    end
    else if (lookup_input_i[15:0] == 16'hff88)
    begin
         cx_ac_value_r = 8'h17;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff89)
    begin
         cx_ac_value_r = 8'h18;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff8a)
    begin
         cx_ac_value_r = 8'h19;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff8b)
    begin
         cx_ac_value_r = 8'h1a;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff8c)
    begin
         cx_ac_value_r = 8'h26;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff8d)
    begin
         cx_ac_value_r = 8'h27;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff8e)
    begin
         cx_ac_value_r = 8'h28;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff8f)
    begin
         cx_ac_value_r = 8'h29;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff90)
    begin
         cx_ac_value_r = 8'h2a;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff91)
    begin
         cx_ac_value_r = 8'h35;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff92)
    begin
         cx_ac_value_r = 8'h36;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff93)
    begin
         cx_ac_value_r = 8'h37;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff94)
    begin
         cx_ac_value_r = 8'h38;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff95)
    begin
         cx_ac_value_r = 8'h39;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff96)
    begin
         cx_ac_value_r = 8'h3a;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff97)
    begin
         cx_ac_value_r = 8'h43;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff98)
    begin
         cx_ac_value_r = 8'h44;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff99)
    begin
         cx_ac_value_r = 8'h45;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff9a)
    begin
         cx_ac_value_r = 8'h46;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff9b)
    begin
         cx_ac_value_r = 8'h47;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff9c)
    begin
         cx_ac_value_r = 8'h48;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff9d)
    begin
         cx_ac_value_r = 8'h49;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff9e)
    begin
         cx_ac_value_r = 8'h4a;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff9f)
    begin
         cx_ac_value_r = 8'h53;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffa0)
    begin
         cx_ac_value_r = 8'h54;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffa1)
    begin
         cx_ac_value_r = 8'h55;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffa2)
    begin
         cx_ac_value_r = 8'h56;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffa3)
    begin
         cx_ac_value_r = 8'h57;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffa4)
    begin
         cx_ac_value_r = 8'h58;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffa5)
    begin
         cx_ac_value_r = 8'h59;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffa6)
    begin
         cx_ac_value_r = 8'h5a;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffa7)
    begin
         cx_ac_value_r = 8'h63;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffa8)
    begin
         cx_ac_value_r = 8'h64;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffa9)
    begin
         cx_ac_value_r = 8'h65;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffaa)
    begin
         cx_ac_value_r = 8'h66;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffab)
    begin
         cx_ac_value_r = 8'h67;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffac)
    begin
         cx_ac_value_r = 8'h68;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffad)
    begin
         cx_ac_value_r = 8'h69;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffae)
    begin
         cx_ac_value_r = 8'h6a;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffaf)
    begin
         cx_ac_value_r = 8'h73;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffb0)
    begin
         cx_ac_value_r = 8'h74;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffb1)
    begin
         cx_ac_value_r = 8'h75;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffb2)
    begin
         cx_ac_value_r = 8'h76;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffb3)
    begin
         cx_ac_value_r = 8'h77;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffb4)
    begin
         cx_ac_value_r = 8'h78;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffb5)
    begin
         cx_ac_value_r = 8'h79;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffb6)
    begin
         cx_ac_value_r = 8'h7a;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffb7)
    begin
         cx_ac_value_r = 8'h82;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffb8)
    begin
         cx_ac_value_r = 8'h83;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffb9)
    begin
         cx_ac_value_r = 8'h84;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffba)
    begin
         cx_ac_value_r = 8'h85;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffbb)
    begin
         cx_ac_value_r = 8'h86;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffbc)
    begin
         cx_ac_value_r = 8'h87;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffbd)
    begin
         cx_ac_value_r = 8'h88;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffbe)
    begin
         cx_ac_value_r = 8'h89;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffbf)
    begin
         cx_ac_value_r = 8'h8a;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffc0)
    begin
         cx_ac_value_r = 8'h92;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffc1)
    begin
         cx_ac_value_r = 8'h93;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffc2)
    begin
         cx_ac_value_r = 8'h94;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffc3)
    begin
         cx_ac_value_r = 8'h95;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffc4)
    begin
         cx_ac_value_r = 8'h96;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffc5)
    begin
         cx_ac_value_r = 8'h97;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffc6)
    begin
         cx_ac_value_r = 8'h98;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffc7)
    begin
         cx_ac_value_r = 8'h99;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffc8)
    begin
         cx_ac_value_r = 8'h9a;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffc9)
    begin
         cx_ac_value_r = 8'ha2;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffca)
    begin
         cx_ac_value_r = 8'ha3;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffcb)
    begin
         cx_ac_value_r = 8'ha4;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffcc)
    begin
         cx_ac_value_r = 8'ha5;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffcd)
    begin
         cx_ac_value_r = 8'ha6;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffce)
    begin
         cx_ac_value_r = 8'ha7;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffcf)
    begin
         cx_ac_value_r = 8'ha8;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffd0)
    begin
         cx_ac_value_r = 8'ha9;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffd1)
    begin
         cx_ac_value_r = 8'haa;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffd2)
    begin
         cx_ac_value_r = 8'hb2;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffd3)
    begin
         cx_ac_value_r = 8'hb3;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffd4)
    begin
         cx_ac_value_r = 8'hb4;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffd5)
    begin
         cx_ac_value_r = 8'hb5;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffd6)
    begin
         cx_ac_value_r = 8'hb6;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffd7)
    begin
         cx_ac_value_r = 8'hb7;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffd8)
    begin
         cx_ac_value_r = 8'hb8;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffd9)
    begin
         cx_ac_value_r = 8'hb9;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffda)
    begin
         cx_ac_value_r = 8'hba;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffdb)
    begin
         cx_ac_value_r = 8'hc2;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffdc)
    begin
         cx_ac_value_r = 8'hc3;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffdd)
    begin
         cx_ac_value_r = 8'hc4;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffde)
    begin
         cx_ac_value_r = 8'hc5;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffdf)
    begin
         cx_ac_value_r = 8'hc6;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffe0)
    begin
         cx_ac_value_r = 8'hc7;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffe1)
    begin
         cx_ac_value_r = 8'hc8;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffe2)
    begin
         cx_ac_value_r = 8'hc9;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffe3)
    begin
         cx_ac_value_r = 8'hca;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffe4)
    begin
         cx_ac_value_r = 8'hd2;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffe5)
    begin
         cx_ac_value_r = 8'hd3;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffe6)
    begin
         cx_ac_value_r = 8'hd4;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffe7)
    begin
         cx_ac_value_r = 8'hd5;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffe8)
    begin
         cx_ac_value_r = 8'hd6;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffe9)
    begin
         cx_ac_value_r = 8'hd7;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffea)
    begin
         cx_ac_value_r = 8'hd8;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffeb)
    begin
         cx_ac_value_r = 8'hd9;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffec)
    begin
         cx_ac_value_r = 8'hda;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffed)
    begin
         cx_ac_value_r = 8'he2;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffee)
    begin
         cx_ac_value_r = 8'he3;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffef)
    begin
         cx_ac_value_r = 8'he4;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfff0)
    begin
         cx_ac_value_r = 8'he5;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfff1)
    begin
         cx_ac_value_r = 8'he6;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfff2)
    begin
         cx_ac_value_r = 8'he7;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfff3)
    begin
         cx_ac_value_r = 8'he8;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfff4)
    begin
         cx_ac_value_r = 8'he9;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfff5)
    begin
         cx_ac_value_r = 8'hea;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfff6)
    begin
         cx_ac_value_r = 8'hf2;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfff7)
    begin
         cx_ac_value_r = 8'hf3;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfff8)
    begin
         cx_ac_value_r = 8'hf4;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfff9)
    begin
         cx_ac_value_r = 8'hf5;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfffa)
    begin
         cx_ac_value_r = 8'hf6;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfffb)
    begin
         cx_ac_value_r = 8'hf7;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfffc)
    begin
         cx_ac_value_r = 8'hf8;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfffd)
    begin
         cx_ac_value_r = 8'hf9;
         cx_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfffe)
    begin
         cx_ac_value_r = 8'hfa;
         cx_ac_width_r = 5'd16;
    end
end

assign lookup_width_o = cx_ac_width_r;
assign lookup_value_o = cx_ac_value_r;

endmodule
//-----------------------------------------------------------------
//                      Baseline JPEG Decoder
//                             V0.1
//                       Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//-----------------------------------------------------------------
//                      License: Apache 2.0
// This IP can be freely used in commercial projects, however you may
// want access to unreleased materials such as verification environments,
// or test vectors, as well as changes to the IP for integration purposes.
// If this is the case, contact the above address.
// I am interested to hear how and where this IP is used, so please get
// in touch!
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
module jpeg_dht_std_cx_dc
(
     input  [ 15:0]  lookup_input_i
    ,output [  4:0]  lookup_width_o
    ,output [  7:0]  lookup_value_o
);

//-----------------------------------------------------------------
// Cx DC Table (standard)
//-----------------------------------------------------------------
reg [7:0] cx_dc_value_r;
reg [4:0] cx_dc_width_r;

always @ *
begin
    cx_dc_value_r = 8'b0;
    cx_dc_width_r = 5'b0;

    if (lookup_input_i[15:14] == 2'h0)
    begin
         cx_dc_value_r = 8'h00;
         cx_dc_width_r = 5'd2;
    end
    else if (lookup_input_i[15:14] == 2'h1)
    begin
         cx_dc_value_r = 8'h01;
         cx_dc_width_r = 5'd2;
    end
    else if (lookup_input_i[15:14] == 2'h2)
    begin
         cx_dc_value_r = 8'h02;
         cx_dc_width_r = 5'd2;
    end
    else if (lookup_input_i[15:13] == 3'h6)
    begin
         cx_dc_value_r = 8'h03;
         cx_dc_width_r = 5'd3;
    end
    else if (lookup_input_i[15:12] == 4'he)
    begin
         cx_dc_value_r = 8'h04;
         cx_dc_width_r = 5'd4;
    end
    else if (lookup_input_i[15:11] == 5'h1e)
    begin
         cx_dc_value_r = 8'h05;
         cx_dc_width_r = 5'd5;
    end
    else if (lookup_input_i[15:10] == 6'h3e)
    begin
         cx_dc_value_r = 8'h06;
         cx_dc_width_r = 5'd6;
    end
    else if (lookup_input_i[15:9] == 7'h7e)
    begin
         cx_dc_value_r = 8'h07;
         cx_dc_width_r = 5'd7;
    end
    else if (lookup_input_i[15:8] == 8'hfe)
    begin
         cx_dc_value_r = 8'h08;
         cx_dc_width_r = 5'd8;
    end
    else if (lookup_input_i[15:7] == 9'h1fe)
    begin
         cx_dc_value_r = 8'h09;
         cx_dc_width_r = 5'd9;
    end
    else if (lookup_input_i[15:6] == 10'h3fe)
    begin
         cx_dc_value_r = 8'h0a;
         cx_dc_width_r = 5'd10;
    end
    else if (lookup_input_i[15:5] == 11'h7fe)
    begin
         cx_dc_value_r = 8'h0b;
         cx_dc_width_r = 5'd11;
    end
end

assign lookup_width_o = cx_dc_width_r;
assign lookup_value_o = cx_dc_value_r;

endmodule

//-----------------------------------------------------------------
//                      Baseline JPEG Decoder
//                             V0.1
//                       Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//-----------------------------------------------------------------
//                      License: Apache 2.0
// This IP can be freely used in commercial projects, however you may
// want access to unreleased materials such as verification environments,
// or test vectors, as well as changes to the IP for integration purposes.
// If this is the case, contact the above address.
// I am interested to hear how and where this IP is used, so please get
// in touch!
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
module jpeg_dht_std_y_ac
(
     input  [ 15:0]  lookup_input_i
    ,output [  4:0]  lookup_width_o
    ,output [  7:0]  lookup_value_o
);

//-----------------------------------------------------------------
// Y AC Table (standard)
//-----------------------------------------------------------------
reg [7:0] y_ac_value_r;
reg [4:0] y_ac_width_r;

always @ *
begin
    y_ac_value_r = 8'b0;
    y_ac_width_r = 5'b0;

    if (lookup_input_i[15:14] == 2'h0)
    begin
         y_ac_value_r = 8'h01;
         y_ac_width_r = 5'd2;
    end
    else if (lookup_input_i[15:14] == 2'h1)
    begin
         y_ac_value_r = 8'h02;
         y_ac_width_r = 5'd2;
    end
    else if (lookup_input_i[15:13] == 3'h4)
    begin
         y_ac_value_r = 8'h03;
         y_ac_width_r = 5'd3;
    end
    else if (lookup_input_i[15:12] == 4'ha)
    begin
         y_ac_value_r = 8'h00;
         y_ac_width_r = 5'd4;
    end
    else if (lookup_input_i[15:12] == 4'hb)
    begin
         y_ac_value_r = 8'h04;
         y_ac_width_r = 5'd4;
    end
    else if (lookup_input_i[15:12] == 4'hc)
    begin
         y_ac_value_r = 8'h11;
         y_ac_width_r = 5'd4;
    end
    else if (lookup_input_i[15:11] == 5'h1a)
    begin
         y_ac_value_r = 8'h05;
         y_ac_width_r = 5'd5;
    end
    else if (lookup_input_i[15:11] == 5'h1b)
    begin
         y_ac_value_r = 8'h12;
         y_ac_width_r = 5'd5;
    end
    else if (lookup_input_i[15:11] == 5'h1c)
    begin
         y_ac_value_r = 8'h21;
         y_ac_width_r = 5'd5;
    end
    else if (lookup_input_i[15:10] == 6'h3a)
    begin
         y_ac_value_r = 8'h31;
         y_ac_width_r = 5'd6;
    end
    else if (lookup_input_i[15:10] == 6'h3b)
    begin
         y_ac_value_r = 8'h41;
         y_ac_width_r = 5'd6;
    end
    else if (lookup_input_i[15:9] == 7'h78)
    begin
         y_ac_value_r = 8'h06;
         y_ac_width_r = 5'd7;
    end
    else if (lookup_input_i[15:9] == 7'h79)
    begin
         y_ac_value_r = 8'h13;
         y_ac_width_r = 5'd7;
    end
    else if (lookup_input_i[15:9] == 7'h7a)
    begin
         y_ac_value_r = 8'h51;
         y_ac_width_r = 5'd7;
    end
    else if (lookup_input_i[15:9] == 7'h7b)
    begin
         y_ac_value_r = 8'h61;
         y_ac_width_r = 5'd7;
    end
    else if (lookup_input_i[15:8] == 8'hf8)
    begin
         y_ac_value_r = 8'h07;
         y_ac_width_r = 5'd8;
    end
    else if (lookup_input_i[15:8] == 8'hf9)
    begin
         y_ac_value_r = 8'h22;
         y_ac_width_r = 5'd8;
    end
    else if (lookup_input_i[15:8] == 8'hfa)
    begin
         y_ac_value_r = 8'h71;
         y_ac_width_r = 5'd8;
    end
    else if (lookup_input_i[15:7] == 9'h1f6)
    begin
         y_ac_value_r = 8'h14;
         y_ac_width_r = 5'd9;
    end
    else if (lookup_input_i[15:7] == 9'h1f7)
    begin
         y_ac_value_r = 8'h32;
         y_ac_width_r = 5'd9;
    end
    else if (lookup_input_i[15:7] == 9'h1f8)
    begin
         y_ac_value_r = 8'h81;
         y_ac_width_r = 5'd9;
    end
    else if (lookup_input_i[15:7] == 9'h1f9)
    begin
         y_ac_value_r = 8'h91;
         y_ac_width_r = 5'd9;
    end
    else if (lookup_input_i[15:7] == 9'h1fa)
    begin
         y_ac_value_r = 8'ha1;
         y_ac_width_r = 5'd9;
    end
    else if (lookup_input_i[15:6] == 10'h3f6)
    begin
         y_ac_value_r = 8'h08;
         y_ac_width_r = 5'd10;
    end
    else if (lookup_input_i[15:6] == 10'h3f7)
    begin
         y_ac_value_r = 8'h23;
         y_ac_width_r = 5'd10;
    end
    else if (lookup_input_i[15:6] == 10'h3f8)
    begin
         y_ac_value_r = 8'h42;
         y_ac_width_r = 5'd10;
    end
    else if (lookup_input_i[15:6] == 10'h3f9)
    begin
         y_ac_value_r = 8'hb1;
         y_ac_width_r = 5'd10;
    end
    else if (lookup_input_i[15:6] == 10'h3fa)
    begin
         y_ac_value_r = 8'hc1;
         y_ac_width_r = 5'd10;
    end
    else if (lookup_input_i[15:5] == 11'h7f6)
    begin
         y_ac_value_r = 8'h15;
         y_ac_width_r = 5'd11;
    end
    else if (lookup_input_i[15:5] == 11'h7f7)
    begin
         y_ac_value_r = 8'h52;
         y_ac_width_r = 5'd11;
    end
    else if (lookup_input_i[15:5] == 11'h7f8)
    begin
         y_ac_value_r = 8'hd1;
         y_ac_width_r = 5'd11;
    end
    else if (lookup_input_i[15:5] == 11'h7f9)
    begin
         y_ac_value_r = 8'hf0;
         y_ac_width_r = 5'd11;
    end
    else if (lookup_input_i[15:4] == 12'hff4)
    begin
         y_ac_value_r = 8'h24;
         y_ac_width_r = 5'd12;
    end
    else if (lookup_input_i[15:4] == 12'hff5)
    begin
         y_ac_value_r = 8'h33;
         y_ac_width_r = 5'd12;
    end
    else if (lookup_input_i[15:4] == 12'hff6)
    begin
         y_ac_value_r = 8'h62;
         y_ac_width_r = 5'd12;
    end
    else if (lookup_input_i[15:4] == 12'hff7)
    begin
         y_ac_value_r = 8'h72;
         y_ac_width_r = 5'd12;
    end
    else if (lookup_input_i[15:1] == 15'h7fc0)
    begin
         y_ac_value_r = 8'h82;
         y_ac_width_r = 5'd15;
    end
    else if (lookup_input_i[15:0] == 16'hff82)
    begin
         y_ac_value_r = 8'h09;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff83)
    begin
         y_ac_value_r = 8'h0a;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff84)
    begin
         y_ac_value_r = 8'h16;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff85)
    begin
         y_ac_value_r = 8'h17;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff86)
    begin
         y_ac_value_r = 8'h18;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff87)
    begin
         y_ac_value_r = 8'h19;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff88)
    begin
         y_ac_value_r = 8'h1a;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff89)
    begin
         y_ac_value_r = 8'h25;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff8a)
    begin
         y_ac_value_r = 8'h26;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff8b)
    begin
         y_ac_value_r = 8'h27;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff8c)
    begin
         y_ac_value_r = 8'h28;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff8d)
    begin
         y_ac_value_r = 8'h29;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff8e)
    begin
         y_ac_value_r = 8'h2a;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff8f)
    begin
         y_ac_value_r = 8'h34;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff90)
    begin
         y_ac_value_r = 8'h35;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff91)
    begin
         y_ac_value_r = 8'h36;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff92)
    begin
         y_ac_value_r = 8'h37;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff93)
    begin
         y_ac_value_r = 8'h38;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff94)
    begin
         y_ac_value_r = 8'h39;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff95)
    begin
         y_ac_value_r = 8'h3a;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff96)
    begin
         y_ac_value_r = 8'h43;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff97)
    begin
         y_ac_value_r = 8'h44;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff98)
    begin
         y_ac_value_r = 8'h45;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff99)
    begin
         y_ac_value_r = 8'h46;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff9a)
    begin
         y_ac_value_r = 8'h47;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff9b)
    begin
         y_ac_value_r = 8'h48;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff9c)
    begin
         y_ac_value_r = 8'h49;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff9d)
    begin
         y_ac_value_r = 8'h4a;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff9e)
    begin
         y_ac_value_r = 8'h53;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hff9f)
    begin
         y_ac_value_r = 8'h54;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffa0)
    begin
         y_ac_value_r = 8'h55;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffa1)
    begin
         y_ac_value_r = 8'h56;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffa2)
    begin
         y_ac_value_r = 8'h57;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffa3)
    begin
         y_ac_value_r = 8'h58;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffa4)
    begin
         y_ac_value_r = 8'h59;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffa5)
    begin
         y_ac_value_r = 8'h5a;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffa6)
    begin
         y_ac_value_r = 8'h63;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffa7)
    begin
         y_ac_value_r = 8'h64;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffa8)
    begin
         y_ac_value_r = 8'h65;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffa9)
    begin
         y_ac_value_r = 8'h66;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffaa)
    begin
         y_ac_value_r = 8'h67;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffab)
    begin
         y_ac_value_r = 8'h68;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffac)
    begin
         y_ac_value_r = 8'h69;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffad)
    begin
         y_ac_value_r = 8'h6a;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffae)
    begin
         y_ac_value_r = 8'h73;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffaf)
    begin
         y_ac_value_r = 8'h74;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffb0)
    begin
         y_ac_value_r = 8'h75;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffb1)
    begin
         y_ac_value_r = 8'h76;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffb2)
    begin
         y_ac_value_r = 8'h77;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffb3)
    begin
         y_ac_value_r = 8'h78;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffb4)
    begin
         y_ac_value_r = 8'h79;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffb5)
    begin
         y_ac_value_r = 8'h7a;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffb6)
    begin
         y_ac_value_r = 8'h83;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffb7)
    begin
         y_ac_value_r = 8'h84;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffb8)
    begin
         y_ac_value_r = 8'h85;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffb9)
    begin
         y_ac_value_r = 8'h86;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffba)
    begin
         y_ac_value_r = 8'h87;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffbb)
    begin
         y_ac_value_r = 8'h88;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffbc)
    begin
         y_ac_value_r = 8'h89;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffbd)
    begin
         y_ac_value_r = 8'h8a;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffbe)
    begin
         y_ac_value_r = 8'h92;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffbf)
    begin
         y_ac_value_r = 8'h93;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffc0)
    begin
         y_ac_value_r = 8'h94;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffc1)
    begin
         y_ac_value_r = 8'h95;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffc2)
    begin
         y_ac_value_r = 8'h96;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffc3)
    begin
         y_ac_value_r = 8'h97;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffc4)
    begin
         y_ac_value_r = 8'h98;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffc5)
    begin
         y_ac_value_r = 8'h99;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffc6)
    begin
         y_ac_value_r = 8'h9a;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffc7)
    begin
         y_ac_value_r = 8'ha2;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffc8)
    begin
         y_ac_value_r = 8'ha3;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffc9)
    begin
         y_ac_value_r = 8'ha4;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffca)
    begin
         y_ac_value_r = 8'ha5;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffcb)
    begin
         y_ac_value_r = 8'ha6;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffcc)
    begin
         y_ac_value_r = 8'ha7;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffcd)
    begin
         y_ac_value_r = 8'ha8;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffce)
    begin
         y_ac_value_r = 8'ha9;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffcf)
    begin
         y_ac_value_r = 8'haa;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffd0)
    begin
         y_ac_value_r = 8'hb2;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffd1)
    begin
         y_ac_value_r = 8'hb3;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffd2)
    begin
         y_ac_value_r = 8'hb4;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffd3)
    begin
         y_ac_value_r = 8'hb5;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffd4)
    begin
         y_ac_value_r = 8'hb6;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffd5)
    begin
         y_ac_value_r = 8'hb7;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffd6)
    begin
         y_ac_value_r = 8'hb8;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffd7)
    begin
         y_ac_value_r = 8'hb9;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffd8)
    begin
         y_ac_value_r = 8'hba;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffd9)
    begin
         y_ac_value_r = 8'hc2;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffda)
    begin
         y_ac_value_r = 8'hc3;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffdb)
    begin
         y_ac_value_r = 8'hc4;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffdc)
    begin
         y_ac_value_r = 8'hc5;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffdd)
    begin
         y_ac_value_r = 8'hc6;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffde)
    begin
         y_ac_value_r = 8'hc7;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffdf)
    begin
         y_ac_value_r = 8'hc8;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffe0)
    begin
         y_ac_value_r = 8'hc9;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffe1)
    begin
         y_ac_value_r = 8'hca;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffe2)
    begin
         y_ac_value_r = 8'hd2;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffe3)
    begin
         y_ac_value_r = 8'hd3;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffe4)
    begin
         y_ac_value_r = 8'hd4;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffe5)
    begin
         y_ac_value_r = 8'hd5;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffe6)
    begin
         y_ac_value_r = 8'hd6;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffe7)
    begin
         y_ac_value_r = 8'hd7;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffe8)
    begin
         y_ac_value_r = 8'hd8;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffe9)
    begin
         y_ac_value_r = 8'hd9;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffea)
    begin
         y_ac_value_r = 8'hda;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffeb)
    begin
         y_ac_value_r = 8'he1;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffec)
    begin
         y_ac_value_r = 8'he2;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffed)
    begin
         y_ac_value_r = 8'he3;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffee)
    begin
         y_ac_value_r = 8'he4;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hffef)
    begin
         y_ac_value_r = 8'he5;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfff0)
    begin
         y_ac_value_r = 8'he6;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfff1)
    begin
         y_ac_value_r = 8'he7;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfff2)
    begin
         y_ac_value_r = 8'he8;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfff3)
    begin
         y_ac_value_r = 8'he9;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfff4)
    begin
         y_ac_value_r = 8'hea;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfff5)
    begin
         y_ac_value_r = 8'hf1;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfff6)
    begin
         y_ac_value_r = 8'hf2;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfff7)
    begin
         y_ac_value_r = 8'hf3;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfff8)
    begin
         y_ac_value_r = 8'hf4;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfff9)
    begin
         y_ac_value_r = 8'hf5;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfffa)
    begin
         y_ac_value_r = 8'hf6;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfffb)
    begin
         y_ac_value_r = 8'hf7;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfffc)
    begin
         y_ac_value_r = 8'hf8;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfffd)
    begin
         y_ac_value_r = 8'hf9;
         y_ac_width_r = 5'd16;
    end
    else if (lookup_input_i[15:0] == 16'hfffe)
    begin
         y_ac_value_r = 8'hfa;
         y_ac_width_r = 5'd16;
    end
end

assign lookup_width_o = y_ac_width_r;
assign lookup_value_o = y_ac_value_r;

endmodule
//-----------------------------------------------------------------
//                      Baseline JPEG Decoder
//                             V0.1
//                       Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//-----------------------------------------------------------------
//                      License: Apache 2.0
// This IP can be freely used in commercial projects, however you may
// want access to unreleased materials such as verification environments,
// or test vectors, as well as changes to the IP for integration purposes.
// If this is the case, contact the above address.
// I am interested to hear how and where this IP is used, so please get
// in touch!
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
module jpeg_dht_std_y_dc
(
     input  [ 15:0]  lookup_input_i
    ,output [  4:0]  lookup_width_o
    ,output [  7:0]  lookup_value_o
);

//-----------------------------------------------------------------
// Y DC Table (standard)
//-----------------------------------------------------------------
reg [7:0] y_dc_value_r;
reg [4:0] y_dc_width_r;

always @ *
begin
    y_dc_value_r = 8'b0;
    y_dc_width_r = 5'b0;

    if (lookup_input_i[15:14] == 2'h0)
    begin
         y_dc_value_r = 8'h00;
         y_dc_width_r = 5'd2;
    end
    else if (lookup_input_i[15:13] == 3'h2)
    begin
         y_dc_value_r = 8'h01;
         y_dc_width_r = 5'd3;
    end
    else if (lookup_input_i[15:13] == 3'h3)
    begin
         y_dc_value_r = 8'h02;
         y_dc_width_r = 5'd3;
    end
    else if (lookup_input_i[15:13] == 3'h4)
    begin
         y_dc_value_r = 8'h03;
         y_dc_width_r = 5'd3;
    end
    else if (lookup_input_i[15:13] == 3'h5)
    begin
         y_dc_value_r = 8'h04;
         y_dc_width_r = 5'd3;
    end
    else if (lookup_input_i[15:13] == 3'h6)
    begin
         y_dc_value_r = 8'h05;
         y_dc_width_r = 5'd3;
    end
    else if (lookup_input_i[15:12] == 4'he)
    begin
         y_dc_value_r = 8'h06;
         y_dc_width_r = 5'd4;
    end
    else if (lookup_input_i[15:11] == 5'h1e)
    begin
         y_dc_value_r = 8'h07;
         y_dc_width_r = 5'd5;
    end
    else if (lookup_input_i[15:10] == 6'h3e)
    begin
         y_dc_value_r = 8'h08;
         y_dc_width_r = 5'd6;
    end
    else if (lookup_input_i[15:9] == 7'h7e)
    begin
         y_dc_value_r = 8'h09;
         y_dc_width_r = 5'd7;
    end
    else if (lookup_input_i[15:8] == 8'hfe)
    begin
         y_dc_value_r = 8'h0a;
         y_dc_width_r = 5'd8;
    end
    else if (lookup_input_i[15:7] == 9'h1fe)
    begin
         y_dc_value_r = 8'h0b;
         y_dc_width_r = 5'd9;
    end
end

assign lookup_width_o = y_dc_width_r;
assign lookup_value_o = y_dc_value_r;

endmodule//-----------------------------------------------------------------
//                      Baseline JPEG Decoder
//                             V0.1
//                       Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//-----------------------------------------------------------------
//                      License: Apache 2.0
// This IP can be freely used in commercial projects, however you may
// want access to unreleased materials such as verification environments,
// or test vectors, as well as changes to the IP for integration purposes.
// If this is the case, contact the above address.
// I am interested to hear how and where this IP is used, so please get
// in touch!
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

module jpeg_dht
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
     parameter SUPPORT_WRITABLE_DHT = 0
)
//-----------------------------------------------------------------
// Ports
//-----------------------------------------------------------------
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           cfg_valid_i
    ,input  [  7:0]  cfg_data_i
    ,input           cfg_last_i
    ,input           lookup_req_i
    ,input  [  1:0]  lookup_table_i
    ,input  [ 15:0]  lookup_input_i

    // Outputs
    ,output          cfg_accept_o
    ,output          lookup_valid_o
    ,output [  4:0]  lookup_width_o
    ,output [  7:0]  lookup_value_o
);




localparam DHT_TABLE_Y_DC      = 8'h00;
localparam DHT_TABLE_Y_AC      = 8'h10;
localparam DHT_TABLE_CX_DC     = 8'h01;
localparam DHT_TABLE_CX_AC     = 8'h11;

//---------------------------------------------------------------------
// Support writable huffman tables (e.g. file contains optimised tables)
//---------------------------------------------------------------------
generate
if (SUPPORT_WRITABLE_DHT)
begin
    reg [15:0] y_dc_min_code_q[0:15];
    reg [15:0] y_dc_max_code_q[0:15];
    reg [9:0]  y_dc_ptr_q[0:15];
    reg [15:0] y_ac_min_code_q[0:15];
    reg [15:0] y_ac_max_code_q[0:15];
    reg [9:0]  y_ac_ptr_q[0:15];
    reg [15:0] cx_dc_min_code_q[0:15];
    reg [15:0] cx_dc_max_code_q[0:15];
    reg [9:0]  cx_dc_ptr_q[0:15];
    reg [15:0] cx_ac_min_code_q[0:15];
    reg [15:0] cx_ac_max_code_q[0:15];
    reg [9:0]  cx_ac_ptr_q[0:15];

    // DHT tables can be combined into one section...
    // Reset the table state machine at the end of each table
    wire cfg_last_w = cfg_last_i || (total_entries_q == idx_q && idx_q >= 12'd16);

    //-----------------------------------------------------------------
    // Capture Index
    //-----------------------------------------------------------------
    reg [11:0] idx_q;

    always @ (posedge clk_i )
    if (rst_i)
        idx_q <= 12'hFFF;
    else if (cfg_valid_i && cfg_last_w && cfg_accept_o)
        idx_q <= 12'hFFF;
    else if (cfg_valid_i && cfg_accept_o)
        idx_q <= idx_q + 12'd1;

    //-----------------------------------------------------------------
    // Table Index
    //-----------------------------------------------------------------
    reg [7:0] cfg_table_q;

    always @ (posedge clk_i )
    if (rst_i)
        cfg_table_q <= 8'b0;
    else if (cfg_valid_i && cfg_accept_o && idx_q == 12'hFFF)
        cfg_table_q <= cfg_data_i;

    //-----------------------------------------------------------------
    // Extract symbol count (temporary)
    //-----------------------------------------------------------------
    reg [7:0]  num_entries_q[0:15];
    reg [15:0] has_entries_q; // bitmap
    reg [11:0] total_entries_q;

    always @ (posedge clk_i )
    if (rst_i)
    begin
        num_entries_q[0] <= 8'b0;
        num_entries_q[1] <= 8'b0;
        num_entries_q[2] <= 8'b0;
        num_entries_q[3] <= 8'b0;
        num_entries_q[4] <= 8'b0;
        num_entries_q[5] <= 8'b0;
        num_entries_q[6] <= 8'b0;
        num_entries_q[7] <= 8'b0;
        num_entries_q[8] <= 8'b0;
        num_entries_q[9] <= 8'b0;
        num_entries_q[10] <= 8'b0;
        num_entries_q[11] <= 8'b0;
        num_entries_q[12] <= 8'b0;
        num_entries_q[13] <= 8'b0;
        num_entries_q[14] <= 8'b0;
        num_entries_q[15] <= 8'b0;
        has_entries_q   <= 16'b0;
        total_entries_q <= 12'd15;
    end
    else if (cfg_valid_i && cfg_accept_o && idx_q < 12'd16)
    begin
        num_entries_q[idx_q[3:0]] <= cfg_data_i;
        has_entries_q[idx_q[3:0]] <= (cfg_data_i != 8'b0);
        total_entries_q           <= total_entries_q + {4'b0, cfg_data_i};
    end
    // End of DHT table
    else if (cfg_valid_i && cfg_last_w && cfg_accept_o)
    begin
        num_entries_q[0] <= 8'b0;
        num_entries_q[1] <= 8'b0;
        num_entries_q[2] <= 8'b0;
        num_entries_q[3] <= 8'b0;
        num_entries_q[4] <= 8'b0;
        num_entries_q[5] <= 8'b0;
        num_entries_q[6] <= 8'b0;
        num_entries_q[7] <= 8'b0;
        num_entries_q[8] <= 8'b0;
        num_entries_q[9] <= 8'b0;
        num_entries_q[10] <= 8'b0;
        num_entries_q[11] <= 8'b0;
        num_entries_q[12] <= 8'b0;
        num_entries_q[13] <= 8'b0;
        num_entries_q[14] <= 8'b0;
        num_entries_q[15] <= 8'b0;
        has_entries_q   <= 16'b0;
        total_entries_q <= 12'd15;
    end

    //-----------------------------------------------------------------
    // Fill tables
    //-----------------------------------------------------------------
    reg [3:0]  i_q;
    reg [7:0]  j_q;
    reg [15:0] code_q;
    reg [9:0]  next_ptr_q;

    always @ (posedge clk_i )
    if (rst_i)
    begin
        y_dc_min_code_q[0] <= 16'b0;
        y_dc_max_code_q[0] <= 16'b0;
        y_dc_ptr_q[0]      <= 10'b0;
        y_dc_min_code_q[1] <= 16'b0;
        y_dc_max_code_q[1] <= 16'b0;
        y_dc_ptr_q[1]      <= 10'b0;
        y_dc_min_code_q[2] <= 16'b0;
        y_dc_max_code_q[2] <= 16'b0;
        y_dc_ptr_q[2]      <= 10'b0;
        y_dc_min_code_q[3] <= 16'b0;
        y_dc_max_code_q[3] <= 16'b0;
        y_dc_ptr_q[3]      <= 10'b0;
        y_dc_min_code_q[4] <= 16'b0;
        y_dc_max_code_q[4] <= 16'b0;
        y_dc_ptr_q[4]      <= 10'b0;
        y_dc_min_code_q[5] <= 16'b0;
        y_dc_max_code_q[5] <= 16'b0;
        y_dc_ptr_q[5]      <= 10'b0;
        y_dc_min_code_q[6] <= 16'b0;
        y_dc_max_code_q[6] <= 16'b0;
        y_dc_ptr_q[6]      <= 10'b0;
        y_dc_min_code_q[7] <= 16'b0;
        y_dc_max_code_q[7] <= 16'b0;
        y_dc_ptr_q[7]      <= 10'b0;
        y_dc_min_code_q[8] <= 16'b0;
        y_dc_max_code_q[8] <= 16'b0;
        y_dc_ptr_q[8]      <= 10'b0;
        y_dc_min_code_q[9] <= 16'b0;
        y_dc_max_code_q[9] <= 16'b0;
        y_dc_ptr_q[9]      <= 10'b0;
        y_dc_min_code_q[10] <= 16'b0;
        y_dc_max_code_q[10] <= 16'b0;
        y_dc_ptr_q[10]      <= 10'b0;
        y_dc_min_code_q[11] <= 16'b0;
        y_dc_max_code_q[11] <= 16'b0;
        y_dc_ptr_q[11]      <= 10'b0;
        y_dc_min_code_q[12] <= 16'b0;
        y_dc_max_code_q[12] <= 16'b0;
        y_dc_ptr_q[12]      <= 10'b0;
        y_dc_min_code_q[13] <= 16'b0;
        y_dc_max_code_q[13] <= 16'b0;
        y_dc_ptr_q[13]      <= 10'b0;
        y_dc_min_code_q[14] <= 16'b0;
        y_dc_max_code_q[14] <= 16'b0;
        y_dc_ptr_q[14]      <= 10'b0;
        y_dc_min_code_q[15] <= 16'b0;
        y_dc_max_code_q[15] <= 16'b0;
        y_dc_ptr_q[15]      <= 10'b0;
        y_ac_min_code_q[0] <= 16'b0;
        y_ac_max_code_q[0] <= 16'b0;
        y_ac_ptr_q[0]      <= 10'b0;
        y_ac_min_code_q[1] <= 16'b0;
        y_ac_max_code_q[1] <= 16'b0;
        y_ac_ptr_q[1]      <= 10'b0;
        y_ac_min_code_q[2] <= 16'b0;
        y_ac_max_code_q[2] <= 16'b0;
        y_ac_ptr_q[2]      <= 10'b0;
        y_ac_min_code_q[3] <= 16'b0;
        y_ac_max_code_q[3] <= 16'b0;
        y_ac_ptr_q[3]      <= 10'b0;
        y_ac_min_code_q[4] <= 16'b0;
        y_ac_max_code_q[4] <= 16'b0;
        y_ac_ptr_q[4]      <= 10'b0;
        y_ac_min_code_q[5] <= 16'b0;
        y_ac_max_code_q[5] <= 16'b0;
        y_ac_ptr_q[5]      <= 10'b0;
        y_ac_min_code_q[6] <= 16'b0;
        y_ac_max_code_q[6] <= 16'b0;
        y_ac_ptr_q[6]      <= 10'b0;
        y_ac_min_code_q[7] <= 16'b0;
        y_ac_max_code_q[7] <= 16'b0;
        y_ac_ptr_q[7]      <= 10'b0;
        y_ac_min_code_q[8] <= 16'b0;
        y_ac_max_code_q[8] <= 16'b0;
        y_ac_ptr_q[8]      <= 10'b0;
        y_ac_min_code_q[9] <= 16'b0;
        y_ac_max_code_q[9] <= 16'b0;
        y_ac_ptr_q[9]      <= 10'b0;
        y_ac_min_code_q[10] <= 16'b0;
        y_ac_max_code_q[10] <= 16'b0;
        y_ac_ptr_q[10]      <= 10'b0;
        y_ac_min_code_q[11] <= 16'b0;
        y_ac_max_code_q[11] <= 16'b0;
        y_ac_ptr_q[11]      <= 10'b0;
        y_ac_min_code_q[12] <= 16'b0;
        y_ac_max_code_q[12] <= 16'b0;
        y_ac_ptr_q[12]      <= 10'b0;
        y_ac_min_code_q[13] <= 16'b0;
        y_ac_max_code_q[13] <= 16'b0;
        y_ac_ptr_q[13]      <= 10'b0;
        y_ac_min_code_q[14] <= 16'b0;
        y_ac_max_code_q[14] <= 16'b0;
        y_ac_ptr_q[14]      <= 10'b0;
        y_ac_min_code_q[15] <= 16'b0;
        y_ac_max_code_q[15] <= 16'b0;
        y_ac_ptr_q[15]      <= 10'b0;
        cx_dc_min_code_q[0] <= 16'b0;
        cx_dc_max_code_q[0] <= 16'b0;
        cx_dc_ptr_q[0]      <= 10'b0;
        cx_dc_min_code_q[1] <= 16'b0;
        cx_dc_max_code_q[1] <= 16'b0;
        cx_dc_ptr_q[1]      <= 10'b0;
        cx_dc_min_code_q[2] <= 16'b0;
        cx_dc_max_code_q[2] <= 16'b0;
        cx_dc_ptr_q[2]      <= 10'b0;
        cx_dc_min_code_q[3] <= 16'b0;
        cx_dc_max_code_q[3] <= 16'b0;
        cx_dc_ptr_q[3]      <= 10'b0;
        cx_dc_min_code_q[4] <= 16'b0;
        cx_dc_max_code_q[4] <= 16'b0;
        cx_dc_ptr_q[4]      <= 10'b0;
        cx_dc_min_code_q[5] <= 16'b0;
        cx_dc_max_code_q[5] <= 16'b0;
        cx_dc_ptr_q[5]      <= 10'b0;
        cx_dc_min_code_q[6] <= 16'b0;
        cx_dc_max_code_q[6] <= 16'b0;
        cx_dc_ptr_q[6]      <= 10'b0;
        cx_dc_min_code_q[7] <= 16'b0;
        cx_dc_max_code_q[7] <= 16'b0;
        cx_dc_ptr_q[7]      <= 10'b0;
        cx_dc_min_code_q[8] <= 16'b0;
        cx_dc_max_code_q[8] <= 16'b0;
        cx_dc_ptr_q[8]      <= 10'b0;
        cx_dc_min_code_q[9] <= 16'b0;
        cx_dc_max_code_q[9] <= 16'b0;
        cx_dc_ptr_q[9]      <= 10'b0;
        cx_dc_min_code_q[10] <= 16'b0;
        cx_dc_max_code_q[10] <= 16'b0;
        cx_dc_ptr_q[10]      <= 10'b0;
        cx_dc_min_code_q[11] <= 16'b0;
        cx_dc_max_code_q[11] <= 16'b0;
        cx_dc_ptr_q[11]      <= 10'b0;
        cx_dc_min_code_q[12] <= 16'b0;
        cx_dc_max_code_q[12] <= 16'b0;
        cx_dc_ptr_q[12]      <= 10'b0;
        cx_dc_min_code_q[13] <= 16'b0;
        cx_dc_max_code_q[13] <= 16'b0;
        cx_dc_ptr_q[13]      <= 10'b0;
        cx_dc_min_code_q[14] <= 16'b0;
        cx_dc_max_code_q[14] <= 16'b0;
        cx_dc_ptr_q[14]      <= 10'b0;
        cx_dc_min_code_q[15] <= 16'b0;
        cx_dc_max_code_q[15] <= 16'b0;
        cx_dc_ptr_q[15]      <= 10'b0;
        cx_ac_min_code_q[0] <= 16'b0;
        cx_ac_max_code_q[0] <= 16'b0;
        cx_ac_ptr_q[0]      <= 10'b0;
        cx_ac_min_code_q[1] <= 16'b0;
        cx_ac_max_code_q[1] <= 16'b0;
        cx_ac_ptr_q[1]      <= 10'b0;
        cx_ac_min_code_q[2] <= 16'b0;
        cx_ac_max_code_q[2] <= 16'b0;
        cx_ac_ptr_q[2]      <= 10'b0;
        cx_ac_min_code_q[3] <= 16'b0;
        cx_ac_max_code_q[3] <= 16'b0;
        cx_ac_ptr_q[3]      <= 10'b0;
        cx_ac_min_code_q[4] <= 16'b0;
        cx_ac_max_code_q[4] <= 16'b0;
        cx_ac_ptr_q[4]      <= 10'b0;
        cx_ac_min_code_q[5] <= 16'b0;
        cx_ac_max_code_q[5] <= 16'b0;
        cx_ac_ptr_q[5]      <= 10'b0;
        cx_ac_min_code_q[6] <= 16'b0;
        cx_ac_max_code_q[6] <= 16'b0;
        cx_ac_ptr_q[6]      <= 10'b0;
        cx_ac_min_code_q[7] <= 16'b0;
        cx_ac_max_code_q[7] <= 16'b0;
        cx_ac_ptr_q[7]      <= 10'b0;
        cx_ac_min_code_q[8] <= 16'b0;
        cx_ac_max_code_q[8] <= 16'b0;
        cx_ac_ptr_q[8]      <= 10'b0;
        cx_ac_min_code_q[9] <= 16'b0;
        cx_ac_max_code_q[9] <= 16'b0;
        cx_ac_ptr_q[9]      <= 10'b0;
        cx_ac_min_code_q[10] <= 16'b0;
        cx_ac_max_code_q[10] <= 16'b0;
        cx_ac_ptr_q[10]      <= 10'b0;
        cx_ac_min_code_q[11] <= 16'b0;
        cx_ac_max_code_q[11] <= 16'b0;
        cx_ac_ptr_q[11]      <= 10'b0;
        cx_ac_min_code_q[12] <= 16'b0;
        cx_ac_max_code_q[12] <= 16'b0;
        cx_ac_ptr_q[12]      <= 10'b0;
        cx_ac_min_code_q[13] <= 16'b0;
        cx_ac_max_code_q[13] <= 16'b0;
        cx_ac_ptr_q[13]      <= 10'b0;
        cx_ac_min_code_q[14] <= 16'b0;
        cx_ac_max_code_q[14] <= 16'b0;
        cx_ac_ptr_q[14]      <= 10'b0;
        cx_ac_min_code_q[15] <= 16'b0;
        cx_ac_max_code_q[15] <= 16'b0;
        cx_ac_ptr_q[15]      <= 10'b0;
        j_q    <= 8'b0;
        i_q    <= 4'b0;
        code_q <= 16'b0;
    end
    else if (idx_q < 12'd16 || idx_q == 12'hFFF)
    begin
        j_q    <= 8'b0;
        i_q    <= 4'b0;
        code_q <= 16'b0;
    end
    else if (cfg_valid_i && cfg_accept_o)
    begin
        if (j_q == 8'b0)
        begin
            case (cfg_table_q)
            DHT_TABLE_Y_DC:
            begin
                y_dc_min_code_q[i_q] <= code_q;
                y_dc_max_code_q[i_q] <= code_q + {8'b0, num_entries_q[i_q]};
                y_dc_ptr_q[i_q]      <= next_ptr_q;
            end
            DHT_TABLE_Y_AC:
            begin
                y_ac_min_code_q[i_q] <= code_q;
                y_ac_max_code_q[i_q] <= code_q + {8'b0, num_entries_q[i_q]};
                y_ac_ptr_q[i_q]      <= next_ptr_q;
            end
            DHT_TABLE_CX_DC:
            begin
                cx_dc_min_code_q[i_q] <= code_q;
                cx_dc_max_code_q[i_q] <= code_q + {8'b0, num_entries_q[i_q]};
                cx_dc_ptr_q[i_q]      <= next_ptr_q;
            end
            default:
            begin
                cx_ac_min_code_q[i_q] <= code_q;
                cx_ac_max_code_q[i_q] <= code_q + {8'b0, num_entries_q[i_q]};
                cx_ac_ptr_q[i_q]      <= next_ptr_q;
            end
            endcase
        end

        if ((j_q + 8'd1) == num_entries_q[i_q])
        begin
            j_q <= 8'b0;
            i_q <= i_q + 4'd1;
            code_q <= (code_q + 16'd1) << 1;
        end
        else
        begin
            code_q <= code_q + 16'd1;
            j_q    <= j_q + 8'd1;
        end
    end
    // Increment through empty bit widths
    else if (cfg_valid_i && !cfg_accept_o)
    begin
        i_q    <= i_q + 4'd1;
        code_q <= code_q << 1;
    end

    assign cfg_accept_o = has_entries_q[i_q] || (idx_q < 12'd16) || (idx_q == 12'hFFF);

    //-----------------------------------------------------------------
    // Code table write pointer
    //-----------------------------------------------------------------
    wire alloc_entry_w = cfg_valid_i && cfg_accept_o && (idx_q >= 12'd16 && idx_q != 12'hFFF);    

    always @ (posedge clk_i )
    if (rst_i)
        next_ptr_q <= 10'b0;
    else if (alloc_entry_w)
        next_ptr_q <= next_ptr_q + 10'd1;

    //-----------------------------------------------------------------
    // Lookup: Match shortest bit sequence
    //-----------------------------------------------------------------

    reg [3:0] y_dc_width_r;
    always @ *
    begin
        y_dc_width_r = 4'b0;

        if ({15'b0, lookup_input_i[15]} < y_dc_max_code_q[0])
            y_dc_width_r = 4'd0;
        else if ({14'b0, lookup_input_i[15:14]} < y_dc_max_code_q[1])
            y_dc_width_r = 4'd1;
        else if ({13'b0, lookup_input_i[15:13]} < y_dc_max_code_q[2])
            y_dc_width_r = 4'd2;
        else if ({12'b0, lookup_input_i[15:12]} < y_dc_max_code_q[3])
            y_dc_width_r = 4'd3;
        else if ({11'b0, lookup_input_i[15:11]} < y_dc_max_code_q[4])
            y_dc_width_r = 4'd4;
        else if ({10'b0, lookup_input_i[15:10]} < y_dc_max_code_q[5])
            y_dc_width_r = 4'd5;
        else if ({9'b0, lookup_input_i[15:9]} < y_dc_max_code_q[6])
            y_dc_width_r = 4'd6;
        else if ({8'b0, lookup_input_i[15:8]} < y_dc_max_code_q[7])
            y_dc_width_r = 4'd7;
        else if ({7'b0, lookup_input_i[15:7]} < y_dc_max_code_q[8])
            y_dc_width_r = 4'd8;
        else if ({6'b0, lookup_input_i[15:6]} < y_dc_max_code_q[9])
            y_dc_width_r = 4'd9;
        else if ({5'b0, lookup_input_i[15:5]} < y_dc_max_code_q[10])
            y_dc_width_r = 4'd10;
        else if ({4'b0, lookup_input_i[15:4]} < y_dc_max_code_q[11])
            y_dc_width_r = 4'd11;
        else if ({3'b0, lookup_input_i[15:3]} < y_dc_max_code_q[12])
            y_dc_width_r = 4'd12;
        else if ({2'b0, lookup_input_i[15:2]} < y_dc_max_code_q[13])
            y_dc_width_r = 4'd13;
        else if ({1'b0, lookup_input_i[15:1]} < y_dc_max_code_q[14])
            y_dc_width_r = 4'd14;
        else
            y_dc_width_r = 4'd15;
    end

    reg [3:0] y_ac_width_r;
    always @ *
    begin
        y_ac_width_r = 4'b0;

        if ({15'b0, lookup_input_i[15]} < y_ac_max_code_q[0])
            y_ac_width_r = 4'd0;
        else if ({14'b0, lookup_input_i[15:14]} < y_ac_max_code_q[1])
            y_ac_width_r = 4'd1;
        else if ({13'b0, lookup_input_i[15:13]} < y_ac_max_code_q[2])
            y_ac_width_r = 4'd2;
        else if ({12'b0, lookup_input_i[15:12]} < y_ac_max_code_q[3])
            y_ac_width_r = 4'd3;
        else if ({11'b0, lookup_input_i[15:11]} < y_ac_max_code_q[4])
            y_ac_width_r = 4'd4;
        else if ({10'b0, lookup_input_i[15:10]} < y_ac_max_code_q[5])
            y_ac_width_r = 4'd5;
        else if ({9'b0, lookup_input_i[15:9]} < y_ac_max_code_q[6])
            y_ac_width_r = 4'd6;
        else if ({8'b0, lookup_input_i[15:8]} < y_ac_max_code_q[7])
            y_ac_width_r = 4'd7;
        else if ({7'b0, lookup_input_i[15:7]} < y_ac_max_code_q[8])
            y_ac_width_r = 4'd8;
        else if ({6'b0, lookup_input_i[15:6]} < y_ac_max_code_q[9])
            y_ac_width_r = 4'd9;
        else if ({5'b0, lookup_input_i[15:5]} < y_ac_max_code_q[10])
            y_ac_width_r = 4'd10;
        else if ({4'b0, lookup_input_i[15:4]} < y_ac_max_code_q[11])
            y_ac_width_r = 4'd11;
        else if ({3'b0, lookup_input_i[15:3]} < y_ac_max_code_q[12])
            y_ac_width_r = 4'd12;
        else if ({2'b0, lookup_input_i[15:2]} < y_ac_max_code_q[13])
            y_ac_width_r = 4'd13;
        else if ({1'b0, lookup_input_i[15:1]} < y_ac_max_code_q[14])
            y_ac_width_r = 4'd14;
        else
            y_ac_width_r = 4'd15;
    end

    reg [3:0] cx_dc_width_r;
    always @ *
    begin
        cx_dc_width_r = 4'b0;

        if ({15'b0, lookup_input_i[15]} < cx_dc_max_code_q[0])
            cx_dc_width_r = 4'd0;
        else if ({14'b0, lookup_input_i[15:14]} < cx_dc_max_code_q[1])
            cx_dc_width_r = 4'd1;
        else if ({13'b0, lookup_input_i[15:13]} < cx_dc_max_code_q[2])
            cx_dc_width_r = 4'd2;
        else if ({12'b0, lookup_input_i[15:12]} < cx_dc_max_code_q[3])
            cx_dc_width_r = 4'd3;
        else if ({11'b0, lookup_input_i[15:11]} < cx_dc_max_code_q[4])
            cx_dc_width_r = 4'd4;
        else if ({10'b0, lookup_input_i[15:10]} < cx_dc_max_code_q[5])
            cx_dc_width_r = 4'd5;
        else if ({9'b0, lookup_input_i[15:9]} < cx_dc_max_code_q[6])
            cx_dc_width_r = 4'd6;
        else if ({8'b0, lookup_input_i[15:8]} < cx_dc_max_code_q[7])
            cx_dc_width_r = 4'd7;
        else if ({7'b0, lookup_input_i[15:7]} < cx_dc_max_code_q[8])
            cx_dc_width_r = 4'd8;
        else if ({6'b0, lookup_input_i[15:6]} < cx_dc_max_code_q[9])
            cx_dc_width_r = 4'd9;
        else if ({5'b0, lookup_input_i[15:5]} < cx_dc_max_code_q[10])
            cx_dc_width_r = 4'd10;
        else if ({4'b0, lookup_input_i[15:4]} < cx_dc_max_code_q[11])
            cx_dc_width_r = 4'd11;
        else if ({3'b0, lookup_input_i[15:3]} < cx_dc_max_code_q[12])
            cx_dc_width_r = 4'd12;
        else if ({2'b0, lookup_input_i[15:2]} < cx_dc_max_code_q[13])
            cx_dc_width_r = 4'd13;
        else if ({1'b0, lookup_input_i[15:1]} < cx_dc_max_code_q[14])
            cx_dc_width_r = 4'd14;
        else
            cx_dc_width_r = 4'd15;
    end

    reg [3:0] cx_ac_width_r;
    always @ *
    begin
        cx_ac_width_r = 4'b0;

        if ({15'b0, lookup_input_i[15]} < cx_ac_max_code_q[0])
            cx_ac_width_r = 4'd0;
        else if ({14'b0, lookup_input_i[15:14]} < cx_ac_max_code_q[1])
            cx_ac_width_r = 4'd1;
        else if ({13'b0, lookup_input_i[15:13]} < cx_ac_max_code_q[2])
            cx_ac_width_r = 4'd2;
        else if ({12'b0, lookup_input_i[15:12]} < cx_ac_max_code_q[3])
            cx_ac_width_r = 4'd3;
        else if ({11'b0, lookup_input_i[15:11]} < cx_ac_max_code_q[4])
            cx_ac_width_r = 4'd4;
        else if ({10'b0, lookup_input_i[15:10]} < cx_ac_max_code_q[5])
            cx_ac_width_r = 4'd5;
        else if ({9'b0, lookup_input_i[15:9]} < cx_ac_max_code_q[6])
            cx_ac_width_r = 4'd6;
        else if ({8'b0, lookup_input_i[15:8]} < cx_ac_max_code_q[7])
            cx_ac_width_r = 4'd7;
        else if ({7'b0, lookup_input_i[15:7]} < cx_ac_max_code_q[8])
            cx_ac_width_r = 4'd8;
        else if ({6'b0, lookup_input_i[15:6]} < cx_ac_max_code_q[9])
            cx_ac_width_r = 4'd9;
        else if ({5'b0, lookup_input_i[15:5]} < cx_ac_max_code_q[10])
            cx_ac_width_r = 4'd10;
        else if ({4'b0, lookup_input_i[15:4]} < cx_ac_max_code_q[11])
            cx_ac_width_r = 4'd11;
        else if ({3'b0, lookup_input_i[15:3]} < cx_ac_max_code_q[12])
            cx_ac_width_r = 4'd12;
        else if ({2'b0, lookup_input_i[15:2]} < cx_ac_max_code_q[13])
            cx_ac_width_r = 4'd13;
        else if ({1'b0, lookup_input_i[15:1]} < cx_ac_max_code_q[14])
            cx_ac_width_r = 4'd14;
        else
            cx_ac_width_r = 4'd15;
    end

    //-----------------------------------------------------------------
    // Lookup: Register lookup width
    //-----------------------------------------------------------------
    reg [3:0]  lookup_width_r;

    always @ *
    begin
        lookup_width_r = 4'b0;

        case (lookup_table_i)
        2'd0:    lookup_width_r = y_dc_width_r;
        2'd1:    lookup_width_r = y_ac_width_r;
        2'd2:    lookup_width_r = cx_dc_width_r;
        default: lookup_width_r = cx_ac_width_r;
        endcase
    end

    reg [3:0]  lookup_width_q;

    always @ (posedge clk_i )
    if (rst_i)
        lookup_width_q <= 4'b0;
    else
        lookup_width_q <= lookup_width_r;

    reg [1:0]  lookup_table_q;

    always @ (posedge clk_i )
    if (rst_i)
        lookup_table_q <= 2'b0;
    else
        lookup_table_q <= lookup_table_i;

    //-----------------------------------------------------------------
    // Lookup: Create RAM lookup address
    //-----------------------------------------------------------------
    reg [15:0] lookup_addr_r;
    reg [15:0] input_code_r;

    always @ *
    begin
        lookup_addr_r  = 16'b0;
        input_code_r   = 16'b0;

        case (lookup_table_q)
        2'd0:
        begin
            input_code_r   = lookup_input_i >> (15 - lookup_width_q);
            lookup_addr_r  = input_code_r - y_dc_min_code_q[lookup_width_q] + {6'b0, y_dc_ptr_q[lookup_width_q]};
        end
        2'd1:
        begin
            input_code_r   = lookup_input_i >> (15 - lookup_width_q);
            lookup_addr_r  = input_code_r - y_ac_min_code_q[lookup_width_q] + {6'b0, y_ac_ptr_q[lookup_width_q]};
        end
        2'd2:
        begin
            input_code_r   = lookup_input_i >> (15 - lookup_width_q);
            lookup_addr_r  = input_code_r - cx_dc_min_code_q[lookup_width_q] + {6'b0, cx_dc_ptr_q[lookup_width_q]};
        end
        default:
        begin
            input_code_r   = lookup_input_i >> (15 - lookup_width_q);
            lookup_addr_r  = input_code_r - cx_ac_min_code_q[lookup_width_q] + {6'b0, cx_ac_ptr_q[lookup_width_q]};
        end
        endcase
    end

    //-----------------------------------------------------------------
    // RAM for storing Huffman decode values
    //-----------------------------------------------------------------
    // LUT for decode values
    reg [7:0]  ram[0:1023];

    always @ (posedge clk_i)
    begin
        if (alloc_entry_w)
            ram[next_ptr_q] <= cfg_data_i;
    end

    reg [7:0] data_value_q;

    always @ (posedge clk_i)
    begin
        data_value_q <= ram[lookup_addr_r[9:0]];
    end

    reg lookup_valid_q;
    always @ (posedge clk_i )
    if (rst_i)
        lookup_valid_q <= 1'b0;
    else
        lookup_valid_q <= lookup_req_i;

    reg lookup_valid2_q;
    always @ (posedge clk_i )
    if (rst_i)
        lookup_valid2_q <= 1'b0;
    else
        lookup_valid2_q <= lookup_valid_q;

    reg [4:0]  lookup_width2_q;

    always @ (posedge clk_i )
    if (rst_i)
        lookup_width2_q <= 5'b0;
    else
        lookup_width2_q <= {1'b0, lookup_width_q} + 5'd1;

    assign lookup_valid_o = lookup_valid2_q;
    assign lookup_value_o = data_value_q;
    assign lookup_width_o = lookup_width2_q;
end
//---------------------------------------------------------------------
// Support only standard huffman tables (from JPEG spec).
//---------------------------------------------------------------------
else
begin
    //-----------------------------------------------------------------
    // Y DC Table (standard)
    //-----------------------------------------------------------------
    wire [7:0] y_dc_value_w;
    wire [4:0] y_dc_width_w;

    jpeg_dht_std_y_dc
    u_fixed_y_dc
    (
         .lookup_input_i(lookup_input_i)
        ,.lookup_value_o(y_dc_value_w)
        ,.lookup_width_o(y_dc_width_w)
    );

    //-----------------------------------------------------------------
    // Y AC Table (standard)
    //-----------------------------------------------------------------
    wire [7:0] y_ac_value_w;
    wire [4:0] y_ac_width_w;

    jpeg_dht_std_y_ac
    u_fixed_y_ac
    (
         .lookup_input_i(lookup_input_i)
        ,.lookup_value_o(y_ac_value_w)
        ,.lookup_width_o(y_ac_width_w)
    );

    //-----------------------------------------------------------------
    // Cx DC Table (standard)
    //-----------------------------------------------------------------
    wire [7:0] cx_dc_value_w;
    wire [4:0] cx_dc_width_w;

    jpeg_dht_std_cx_dc
    u_fixed_cx_dc
    (
         .lookup_input_i(lookup_input_i)
        ,.lookup_value_o(cx_dc_value_w)
        ,.lookup_width_o(cx_dc_width_w)
    );

    //-----------------------------------------------------------------
    // Cx AC Table (standard)
    //-----------------------------------------------------------------
    wire [7:0] cx_ac_value_w;
    wire [4:0] cx_ac_width_w;

    jpeg_dht_std_cx_ac
    u_fixed_cx_ac
    (
         .lookup_input_i(lookup_input_i)
        ,.lookup_value_o(cx_ac_value_w)
        ,.lookup_width_o(cx_ac_width_w)
    );

    //-----------------------------------------------------------------
    // Lookup
    //-----------------------------------------------------------------
    reg lookup_valid_q;

    always @ (posedge clk_i )
    if (rst_i)
        lookup_valid_q <= 1'b0;
    else
        lookup_valid_q <= lookup_req_i;

    assign lookup_valid_o = lookup_valid_q;

    reg [7:0] lookup_value_q;

    always @ (posedge clk_i )
    if (rst_i)
        lookup_value_q <= 8'b0;
    else
    begin
        case (lookup_table_i)
        2'd0: lookup_value_q <= y_dc_value_w;
        2'd1: lookup_value_q <= y_ac_value_w;
        2'd2: lookup_value_q <= cx_dc_value_w;
        2'd3: lookup_value_q <= cx_ac_value_w;
        endcase
    end

    assign lookup_value_o = lookup_value_q;

    reg [4:0] lookup_width_q;

    always @ (posedge clk_i )
    if (rst_i)
        lookup_width_q <= 5'b0;
    else
    begin
        case (lookup_table_i)
        2'd0: lookup_width_q <= y_dc_width_w;
        2'd1: lookup_width_q <= y_ac_width_w;
        2'd2: lookup_width_q <= cx_dc_width_w;
        2'd3: lookup_width_q <= cx_ac_width_w;
        endcase
    end

    assign lookup_width_o = lookup_width_q;

    assign cfg_accept_o = 1'b1;
end
endgenerate


endmodule
//-----------------------------------------------------------------
//                      Baseline JPEG Decoder
//                             V0.1
//                       Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//-----------------------------------------------------------------
//                      License: Apache 2.0
// This IP can be freely used in commercial projects, however you may
// want access to unreleased materials such as verification environments,
// or test vectors, as well as changes to the IP for integration purposes.
// If this is the case, contact the above address.
// I am interested to hear how and where this IP is used, so please get
// in touch!
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

module jpeg_dqt
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           img_start_i
    ,input           img_end_i
    ,input  [  1:0]  img_dqt_table_y_i
    ,input  [  1:0]  img_dqt_table_cb_i
    ,input  [  1:0]  img_dqt_table_cr_i
    ,input           cfg_valid_i
    ,input  [  7:0]  cfg_data_i
    ,input           cfg_last_i
    ,input           inport_valid_i
    ,input  [ 15:0]  inport_data_i
    ,input  [  5:0]  inport_idx_i
    ,input  [ 31:0]  inport_id_i
    ,input           inport_eob_i
    ,input           outport_accept_i

    // Outputs
    ,output          cfg_accept_o
    ,output          inport_blk_space_o
    ,output          outport_valid_o
    ,output [ 15:0]  outport_data_o
    ,output [  5:0]  outport_idx_o
    ,output [ 31:0]  outport_id_o
    ,output          outport_eob_o
);



//-----------------------------------------------------------------
// DQT tables
//-----------------------------------------------------------------
// 4 * 256
reg [7:0] table_dqt_q[0:255];

//-----------------------------------------------------------------
// Capture Index
//-----------------------------------------------------------------
reg [7:0] idx_q;

always @ (posedge clk_i )
if (rst_i)
    idx_q <= 8'hFF;
else if (cfg_valid_i && cfg_last_i && cfg_accept_o)
    idx_q <= 8'hFF;
else if (cfg_valid_i && cfg_accept_o)
    idx_q <= idx_q + 8'd1;

assign cfg_accept_o = 1'b1;

//-----------------------------------------------------------------
// Write DQT table
//-----------------------------------------------------------------
reg [1:0] cfg_table_q;

always @ (posedge clk_i )
if (rst_i)
    cfg_table_q <= 2'b0;
else if (cfg_valid_i && cfg_accept_o && idx_q == 8'hFF)
    cfg_table_q <= cfg_data_i[1:0];

wire [7:0] cfg_table_addr_w = {cfg_table_q, idx_q[5:0]};

wire [1:0] table_src_w[3:0];

assign table_src_w[0] = img_dqt_table_y_i;
assign table_src_w[1] = img_dqt_table_cb_i;
assign table_src_w[2] = img_dqt_table_cr_i;
assign table_src_w[3] = 2'b0;

wire [7:0] table_rd_idx_w   = {table_src_w[inport_id_i[31:30]], inport_idx_i};

wire       dqt_write_w      = cfg_valid_i && cfg_accept_o && idx_q != 8'hFF;
wire [7:0] dqt_table_addr_w = dqt_write_w ? cfg_table_addr_w : table_rd_idx_w;

reg [7:0] dqt_entry_q;

always @ (posedge clk_i )
begin
    if (dqt_write_w)
        table_dqt_q[dqt_table_addr_w] <= cfg_data_i;

    dqt_entry_q <= table_dqt_q[dqt_table_addr_w];
end

//-----------------------------------------------------------------
// dezigzag: Reverse zigzag process
//-----------------------------------------------------------------
function [5:0] dezigzag;
    input [5:0] idx;
    reg [5:0] out_idx;
begin
    case (idx)
    6'd0: out_idx = 6'd0;
    6'd1: out_idx = 6'd1;
    6'd2: out_idx = 6'd8;
    6'd3: out_idx = 6'd16;
    6'd4: out_idx = 6'd9;
    6'd5: out_idx = 6'd2;
    6'd6: out_idx = 6'd3;
    6'd7: out_idx = 6'd10;
    6'd8: out_idx = 6'd17;
    6'd9: out_idx = 6'd24;
    6'd10: out_idx = 6'd32;
    6'd11: out_idx = 6'd25;
    6'd12: out_idx = 6'd18;
    6'd13: out_idx = 6'd11;
    6'd14: out_idx = 6'd4;
    6'd15: out_idx = 6'd5;
    6'd16: out_idx = 6'd12;
    6'd17: out_idx = 6'd19;
    6'd18: out_idx = 6'd26;
    6'd19: out_idx = 6'd33;
    6'd20: out_idx = 6'd40;
    6'd21: out_idx = 6'd48;
    6'd22: out_idx = 6'd41;
    6'd23: out_idx = 6'd34;
    6'd24: out_idx = 6'd27;
    6'd25: out_idx = 6'd20;
    6'd26: out_idx = 6'd13;
    6'd27: out_idx = 6'd6;
    6'd28: out_idx = 6'd7;
    6'd29: out_idx = 6'd14;
    6'd30: out_idx = 6'd21;
    6'd31: out_idx = 6'd28;
    6'd32: out_idx = 6'd35;
    6'd33: out_idx = 6'd42;
    6'd34: out_idx = 6'd49;
    6'd35: out_idx = 6'd56;
    6'd36: out_idx = 6'd57;
    6'd37: out_idx = 6'd50;
    6'd38: out_idx = 6'd43;
    6'd39: out_idx = 6'd36;
    6'd40: out_idx = 6'd29;
    6'd41: out_idx = 6'd22;
    6'd42: out_idx = 6'd15;
    6'd43: out_idx = 6'd23;
    6'd44: out_idx = 6'd30;
    6'd45: out_idx = 6'd37;
    6'd46: out_idx = 6'd44;
    6'd47: out_idx = 6'd51;
    6'd48: out_idx = 6'd58;
    6'd49: out_idx = 6'd59;
    6'd50: out_idx = 6'd52;
    6'd51: out_idx = 6'd45;
    6'd52: out_idx = 6'd38;
    6'd53: out_idx = 6'd31;
    6'd54: out_idx = 6'd39;
    6'd55: out_idx = 6'd46;
    6'd56: out_idx = 6'd53;
    6'd57: out_idx = 6'd60;
    6'd58: out_idx = 6'd61;
    6'd59: out_idx = 6'd54;
    6'd60: out_idx = 6'd47;
    6'd61: out_idx = 6'd55;
    6'd62: out_idx = 6'd62;
    default: out_idx = 6'd63; 
    endcase

    dezigzag = out_idx;
end
endfunction

//-----------------------------------------------------------------
// Process dequantisation and dezigzag
//-----------------------------------------------------------------
reg        inport_valid_q;
reg [15:0] inport_data_q;
reg [5:0]  inport_idx_q;
reg [31:0] inport_id_q;
reg        inport_eob_q;

always @ (posedge clk_i )
if (rst_i)
    inport_valid_q <= 1'b0;
else
    inport_valid_q <= inport_valid_i && ~img_start_i;

always @ (posedge clk_i )
if (rst_i)
    inport_idx_q <= 6'b0;
else
    inport_idx_q <= inport_idx_i;

always @ (posedge clk_i )
if (rst_i)
    inport_data_q <= 16'b0;
else
    inport_data_q <= inport_data_i;

always @ (posedge clk_i )
if (rst_i)
    inport_id_q <= 32'b0;
else if (inport_valid_i)
    inport_id_q <= inport_id_i;

always @ (posedge clk_i )
if (rst_i)
    inport_eob_q <= 1'b0;
else
    inport_eob_q <= inport_eob_i;

//-----------------------------------------------------------------
// Output
//-----------------------------------------------------------------
reg               outport_valid_q;
reg signed [15:0] outport_data_q;
reg [5:0]         outport_idx_q;
reg [31:0]        outport_id_q;
reg               outport_eob_q;

always @ (posedge clk_i )
if (rst_i)
    outport_valid_q <= 1'b0;
else
    outport_valid_q <= inport_valid_q && ~img_start_i;

always @ (posedge clk_i )
if (rst_i)
    outport_data_q <= 16'b0;
else
    outport_data_q <= inport_data_q * dqt_entry_q;

always @ (posedge clk_i )
if (rst_i)
    outport_idx_q <= 6'b0;
else
    outport_idx_q <= dezigzag(inport_idx_q);

always @ (posedge clk_i )
if (rst_i)
    outport_id_q <= 32'b0;
else
    outport_id_q <= inport_id_q;

always @ (posedge clk_i )
if (rst_i)
    outport_eob_q <= 1'b0;
else
    outport_eob_q <= inport_eob_q;

assign outport_valid_o = outport_valid_q;
assign outport_data_o  = outport_data_q;
assign outport_idx_o   = outport_idx_q;
assign outport_id_o    = outport_id_q;    
assign outport_eob_o   = outport_eob_q;

// TODO: Perf
assign inport_blk_space_o = outport_accept_i && !(outport_eob_q || inport_eob_q);

`ifdef verilator
function get_valid; /*verilator public*/
begin
    get_valid = outport_valid_o;
end
endfunction
function [15:0] get_sample; /*verilator public*/
begin
    get_sample = outport_data_o;
end
endfunction
function [5:0] get_sample_idx; /*verilator public*/
begin
    get_sample_idx = outport_idx_o;
end
endfunction
`endif

endmodule
//-----------------------------------------------------------------
//                      Baseline JPEG Decoder
//                             V0.1
//                       Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//-----------------------------------------------------------------
//                      License: Apache 2.0
// This IP can be freely used in commercial projects, however you may
// want access to unreleased materials such as verification environments,
// or test vectors, as well as changes to the IP for integration purposes.
// If this is the case, contact the above address.
// I am interested to hear how and where this IP is used, so please get
// in touch!
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

module jpeg_idct_fifo
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
     parameter WIDTH            = 8
    ,parameter DEPTH            = 4
    ,parameter ADDR_W           = 2
)
//-----------------------------------------------------------------
// Ports
//-----------------------------------------------------------------
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input  [WIDTH-1:0]  data_in_i
    ,input           push_i
    ,input           pop_i
    ,input           flush_i

    // Outputs
    ,output [WIDTH-1:0]  data_out_o
    ,output          accept_o
    ,output          valid_o
);



//-----------------------------------------------------------------
// Local Params
//-----------------------------------------------------------------
localparam COUNT_W = ADDR_W + 1;

//-----------------------------------------------------------------
// Registers
//-----------------------------------------------------------------
reg [WIDTH-1:0]   ram_q[DEPTH-1:0];
reg [ADDR_W-1:0]  rd_ptr_q;
reg [ADDR_W-1:0]  wr_ptr_q;
reg [COUNT_W-1:0] count_q;

//-----------------------------------------------------------------
// Sequential
//-----------------------------------------------------------------
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
begin
    count_q   <= {(COUNT_W) {1'b0}};
    rd_ptr_q  <= {(ADDR_W) {1'b0}};
    wr_ptr_q  <= {(ADDR_W) {1'b0}};
end
else if (flush_i)
begin
    count_q   <= {(COUNT_W) {1'b0}};
    rd_ptr_q  <= {(ADDR_W) {1'b0}};
    wr_ptr_q  <= {(ADDR_W) {1'b0}};
end
else
begin
    // Push
    if (push_i & accept_o)
    begin
        ram_q[wr_ptr_q] <= data_in_i;
        wr_ptr_q        <= wr_ptr_q + 1;
    end

    // Pop
    if (pop_i & valid_o)
        rd_ptr_q      <= rd_ptr_q + 1;

    // Count up
    if ((push_i & accept_o) & ~(pop_i & valid_o))
        count_q <= count_q + 1;
    // Count down
    else if (~(push_i & accept_o) & (pop_i & valid_o))
        count_q <= count_q - 1;
end

//-------------------------------------------------------------------
// Combinatorial
//-------------------------------------------------------------------
/* verilator lint_off WIDTH */
assign valid_o       = (count_q != 0);
assign accept_o      = (count_q != DEPTH);
/* verilator lint_on WIDTH */

assign data_out_o    = ram_q[rd_ptr_q];



endmodule
//-----------------------------------------------------------------
//                      Baseline JPEG Decoder
//                             V0.1
//                       Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//-----------------------------------------------------------------
//                      License: Apache 2.0
// This IP can be freely used in commercial projects, however you may
// want access to unreleased materials such as verification environments,
// or test vectors, as well as changes to the IP for integration purposes.
// If this is the case, contact the above address.
// I am interested to hear how and where this IP is used, so please get
// in touch!
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

module jpeg_idct_ram_dp
(
    // Inputs
     input           clk0_i
    ,input           rst0_i
    ,input  [  5:0]  addr0_i
    ,input  [ 15:0]  data0_i
    ,input           wr0_i
    ,input           clk1_i
    ,input           rst1_i
    ,input  [  5:0]  addr1_i
    ,input  [ 15:0]  data1_i
    ,input           wr1_i

    // Outputs
    ,output [ 15:0]  data0_o
    ,output [ 15:0]  data1_o
);



//-----------------------------------------------------------------
// Dual Port RAM
// Mode: Read First
//-----------------------------------------------------------------
/* verilator lint_off MULTIDRIVEN */
reg [15:0]   ram [63:0] /*verilator public*/;
/* verilator lint_on MULTIDRIVEN */

reg [15:0] ram_read0_q;
reg [15:0] ram_read1_q;


// Synchronous write
always @ (posedge clk0_i)
begin
    if (wr0_i)
        ram[addr0_i][15:0] <= data0_i[15:0];

    ram_read0_q <= ram[addr0_i];
end

always @ (posedge clk1_i)
begin
    if (wr1_i)
        ram[addr1_i][15:0] <= data1_i[15:0];

    ram_read1_q <= ram[addr1_i];
end


assign data0_o = ram_read0_q;
assign data1_o = ram_read1_q;



endmodule
//-----------------------------------------------------------------
//                      Baseline JPEG Decoder
//                             V0.1
//                       Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//-----------------------------------------------------------------
//                      License: Apache 2.0
// This IP can be freely used in commercial projects, however you may
// want access to unreleased materials such as verification environments,
// or test vectors, as well as changes to the IP for integration purposes.
// If this is the case, contact the above address.
// I am interested to hear how and where this IP is used, so please get
// in touch!
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

module jpeg_idct_ram
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           img_start_i
    ,input           img_end_i
    ,input           inport_valid_i
    ,input  [ 15:0]  inport_data_i
    ,input  [  5:0]  inport_idx_i
    ,input           inport_eob_i
    ,input           outport_ready_i

    // Outputs
    ,output          inport_accept_o
    ,output          outport_valid_o
    ,output [ 15:0]  outport_data0_o
    ,output [ 15:0]  outport_data1_o
    ,output [ 15:0]  outport_data2_o
    ,output [ 15:0]  outport_data3_o
    ,output [  2:0]  outport_idx_o
);



reg [1:0]   block_wr_q;
reg [1:0]   block_rd_q;
reg [5:0]   rd_idx_q;
reg [3:0]   rd_addr_q;

wire [5:0]  wr_ptr_w = {block_wr_q, inport_idx_i[5:3], inport_idx_i[0]};

wire [15:0] outport_data0_w;
wire [15:0] outport_data1_w;
wire [15:0] outport_data2_w;
wire [15:0] outport_data3_w;

wire wr0_w = inport_valid_i && inport_accept_o && (inport_idx_i[2:0] == 3'd0 || inport_idx_i[2:0] == 3'd1);
wire wr1_w = inport_valid_i && inport_accept_o && (inport_idx_i[2:0] == 3'd2 || inport_idx_i[2:0] == 3'd3);
wire wr2_w = inport_valid_i && inport_accept_o && (inport_idx_i[2:0] == 3'd4 || inport_idx_i[2:0] == 3'd5);
wire wr3_w = inport_valid_i && inport_accept_o && (inport_idx_i[2:0] == 3'd6 || inport_idx_i[2:0] == 3'd7);

jpeg_idct_ram_dp
u_ram0
(
     .clk0_i(clk_i)
    ,.rst0_i(rst_i)
    ,.clk1_i(clk_i)
    ,.rst1_i(rst_i)

    ,.addr0_i(wr_ptr_w)
    ,.data0_i(inport_data_i)
    ,.wr0_i(wr0_w)
    ,.data0_o()

    ,.addr1_i({block_rd_q, rd_addr_q})
    ,.data1_i(16'b0)
    ,.wr1_i(1'b0)
    ,.data1_o(outport_data0_w)
);

jpeg_idct_ram_dp
u_ram1
(
     .clk0_i(clk_i)
    ,.rst0_i(rst_i)
    ,.clk1_i(clk_i)
    ,.rst1_i(rst_i)

    ,.addr0_i(wr_ptr_w)
    ,.data0_i(inport_data_i)
    ,.wr0_i(wr1_w)
    ,.data0_o()

    ,.addr1_i({block_rd_q, rd_addr_q})
    ,.data1_i(16'b0)
    ,.wr1_i(1'b0)
    ,.data1_o(outport_data1_w)
);

jpeg_idct_ram_dp
u_ram2
(
     .clk0_i(clk_i)
    ,.rst0_i(rst_i)
    ,.clk1_i(clk_i)
    ,.rst1_i(rst_i)

    ,.addr0_i(wr_ptr_w)
    ,.data0_i(inport_data_i)
    ,.wr0_i(wr2_w)
    ,.data0_o()

    ,.addr1_i({block_rd_q, rd_addr_q})
    ,.data1_i(16'b0)
    ,.wr1_i(1'b0)
    ,.data1_o(outport_data2_w)
);

jpeg_idct_ram_dp
u_ram3
(
     .clk0_i(clk_i)
    ,.rst0_i(rst_i)
    ,.clk1_i(clk_i)
    ,.rst1_i(rst_i)

    ,.addr0_i(wr_ptr_w)
    ,.data0_i(inport_data_i)
    ,.wr0_i(wr3_w)
    ,.data0_o()

    ,.addr1_i({block_rd_q, rd_addr_q})
    ,.data1_i(16'b0)
    ,.wr1_i(1'b0)
    ,.data1_o(outport_data3_w)
);

//-----------------------------------------------------------------
// Data Qualifiers
//-----------------------------------------------------------------
reg [63:0]        data_valid0_r;
reg [63:0]        data_valid0_q;

always @ *
begin
    data_valid0_r = data_valid0_q;

    // End of block read out - reset data valid state
    if (outport_valid_o && rd_idx_q[5:0] == 6'd63)
    begin
        case (block_rd_q)
        2'd0:    data_valid0_r[15:0]  = 16'b0;
        2'd1:    data_valid0_r[31:16] = 16'b0;
        2'd2:    data_valid0_r[47:32] = 16'b0;
        default: data_valid0_r[63:48] = 16'b0;
        endcase
    end

    if (wr0_w)
        data_valid0_r[wr_ptr_w] = 1'b1;
end

always @ (posedge clk_i )
if (rst_i)
    data_valid0_q <= 64'b0;
else if (img_start_i)
    data_valid0_q <= 64'b0;
else
    data_valid0_q <= data_valid0_r;

reg [63:0]        data_valid1_r;
reg [63:0]        data_valid1_q;

always @ *
begin
    data_valid1_r = data_valid1_q;

    // End of block read out - reset data valid state
    if (outport_valid_o && rd_idx_q[5:0] == 6'd63)
    begin
        case (block_rd_q)
        2'd0:    data_valid1_r[15:0]  = 16'b0;
        2'd1:    data_valid1_r[31:16] = 16'b0;
        2'd2:    data_valid1_r[47:32] = 16'b0;
        default: data_valid1_r[63:48] = 16'b0;
        endcase
    end

    if (wr1_w)
        data_valid1_r[wr_ptr_w] = 1'b1;
end

always @ (posedge clk_i )
if (rst_i)
    data_valid1_q <= 64'b0;
else if (img_start_i)
    data_valid1_q <= 64'b0;
else
    data_valid1_q <= data_valid1_r;

reg [63:0]        data_valid2_r;
reg [63:0]        data_valid2_q;

always @ *
begin
    data_valid2_r = data_valid2_q;

    // End of block read out - reset data valid state
    if (outport_valid_o && rd_idx_q[5:0] == 6'd63)
    begin
        case (block_rd_q)
        2'd0:    data_valid2_r[15:0]  = 16'b0;
        2'd1:    data_valid2_r[31:16] = 16'b0;
        2'd2:    data_valid2_r[47:32] = 16'b0;
        default: data_valid2_r[63:48] = 16'b0;
        endcase
    end

    if (wr2_w)
        data_valid2_r[wr_ptr_w] = 1'b1;
end

always @ (posedge clk_i )
if (rst_i)
    data_valid2_q <= 64'b0;
else if (img_start_i)
    data_valid2_q <= 64'b0;
else
    data_valid2_q <= data_valid2_r;

reg [63:0]        data_valid3_r;
reg [63:0]        data_valid3_q;

always @ *
begin
    data_valid3_r = data_valid3_q;

    // End of block read out - reset data valid state
    if (outport_valid_o && rd_idx_q[5:0] == 6'd63)
    begin
        case (block_rd_q)
        2'd0:    data_valid3_r[15:0]  = 16'b0;
        2'd1:    data_valid3_r[31:16] = 16'b0;
        2'd2:    data_valid3_r[47:32] = 16'b0;
        default: data_valid3_r[63:48] = 16'b0;
        endcase
    end

    if (wr3_w)
        data_valid3_r[wr_ptr_w] = 1'b1;
end

always @ (posedge clk_i )
if (rst_i)
    data_valid3_q <= 64'b0;
else if (img_start_i)
    data_valid3_q <= 64'b0;
else
    data_valid3_q <= data_valid3_r;


//-----------------------------------------------------------------
// Input Buffer
//-----------------------------------------------------------------
reg [3:0] block_ready_q;

always @ (posedge clk_i )
if (rst_i)
begin
    block_ready_q     <= 4'b0;
    block_wr_q        <= 2'b0;
    block_rd_q        <= 2'b0;
end
else if (img_start_i)
begin
    block_ready_q     <= 4'b0;
    block_wr_q        <= 2'b0;
    block_rd_q        <= 2'b0;
end
else
begin
    if (inport_eob_i && inport_accept_o)
    begin
        block_ready_q[block_wr_q] <= 1'b1;
        block_wr_q                <= block_wr_q + 2'd1;
    end

    if (outport_valid_o && rd_idx_q[5:0] == 6'd63)
    begin
        block_ready_q[block_rd_q] <= 1'b0;
        block_rd_q                <= block_rd_q + 2'd1;
    end
end

assign inport_accept_o = ~block_ready_q[block_wr_q];

//-----------------------------------------------------------------
// FSM
//-----------------------------------------------------------------
localparam STATE_W           = 2;
localparam STATE_IDLE        = 2'd0;
localparam STATE_SETUP       = 2'd1;
localparam STATE_ACTIVE      = 2'd2;

reg [STATE_W-1:0] state_q;
reg [STATE_W-1:0] next_state_r;

always @ *
begin
    next_state_r = state_q;

    case (state_q)
    STATE_IDLE:
    begin
        if (block_ready_q[block_rd_q] && outport_ready_i)
            next_state_r = STATE_SETUP;
    end
    STATE_SETUP:
    begin
        next_state_r = STATE_ACTIVE;
    end
    STATE_ACTIVE:
    begin
        if (outport_valid_o && rd_idx_q == 6'd63)
            next_state_r = STATE_IDLE;
    end
    default: ;
    endcase

    if (img_start_i)
        next_state_r = STATE_IDLE;
end

always @ (posedge clk_i )
if (rst_i)
    state_q <= STATE_IDLE;
else
    state_q <= next_state_r;

always @ (posedge clk_i )
if (rst_i)
    rd_idx_q <= 6'b0;
else if (img_start_i)
    rd_idx_q <= 6'b0;
else if (state_q == STATE_ACTIVE)
    rd_idx_q <= rd_idx_q + 6'd1;

always @ (posedge clk_i )
if (rst_i)
    rd_addr_q <= 4'b0;
else if (state_q == STATE_IDLE)
    rd_addr_q <= 4'b0;
else if (state_q == STATE_SETUP)
    rd_addr_q <= 4'd1;
else if (state_q == STATE_ACTIVE)
begin
    case (rd_idx_q[2:0])
    3'd0: rd_addr_q <= rd_addr_q - 1;
    3'd1: rd_addr_q <= rd_addr_q + 1;
    3'd2: ;
    3'd3: rd_addr_q <= rd_addr_q - 1;
    3'd4: rd_addr_q <= rd_addr_q + 1;
    3'd5: rd_addr_q <= rd_addr_q - 1;
    3'd6: rd_addr_q <= rd_addr_q + 2;
    3'd7: rd_addr_q <= rd_addr_q + 1;
    endcase
end

reg data_val0_q;
reg data_val1_q;
reg data_val2_q;
reg data_val3_q;

always @ (posedge clk_i )
if (rst_i)
begin
    data_val0_q <= 1'b0;
    data_val1_q <= 1'b0;
    data_val2_q <= 1'b0;
    data_val3_q <= 1'b0;
end
else
begin
    data_val0_q <= data_valid0_q[{block_rd_q, rd_addr_q}];
    data_val1_q <= data_valid1_q[{block_rd_q, rd_addr_q}];
    data_val2_q <= data_valid2_q[{block_rd_q, rd_addr_q}];
    data_val3_q <= data_valid3_q[{block_rd_q, rd_addr_q}];
end

assign outport_valid_o = (state_q == STATE_ACTIVE);
assign outport_idx_o   = rd_idx_q[2:0];
assign outport_data0_o = {16{data_val0_q}} & outport_data0_w;
assign outport_data1_o = {16{data_val1_q}} & outport_data1_w;
assign outport_data2_o = {16{data_val2_q}} & outport_data2_w;
assign outport_data3_o = {16{data_val3_q}} & outport_data3_w;


endmodule
//-----------------------------------------------------------------
//                      Baseline JPEG Decoder
//                             V0.1
//                       Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//-----------------------------------------------------------------
//                      License: Apache 2.0
// This IP can be freely used in commercial projects, however you may
// want access to unreleased materials such as verification environments,
// or test vectors, as well as changes to the IP for integration purposes.
// If this is the case, contact the above address.
// I am interested to hear how and where this IP is used, so please get
// in touch!
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

module jpeg_idct_transpose_ram
(
    // Inputs
     input           clk0_i
    ,input           rst0_i
    ,input  [  4:0]  addr0_i
    ,input  [ 31:0]  data0_i
    ,input           wr0_i
    ,input           clk1_i
    ,input           rst1_i
    ,input  [  4:0]  addr1_i
    ,input  [ 31:0]  data1_i
    ,input           wr1_i

    // Outputs
    ,output [ 31:0]  data0_o
    ,output [ 31:0]  data1_o
);



//-----------------------------------------------------------------
// Dual Port RAM
// Mode: Read First
//-----------------------------------------------------------------
/* verilator lint_off MULTIDRIVEN */
reg [31:0]   ram [31:0] /*verilator public*/;
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
//                      Baseline JPEG Decoder
//                             V0.1
//                       Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//-----------------------------------------------------------------
//                      License: Apache 2.0
// This IP can be freely used in commercial projects, however you may
// want access to unreleased materials such as verification environments,
// or test vectors, as well as changes to the IP for integration purposes.
// If this is the case, contact the above address.
// I am interested to hear how and where this IP is used, so please get
// in touch!
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

module jpeg_idct_transpose
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           img_start_i
    ,input           img_end_i
    ,input           inport_valid_i
    ,input  [ 31:0]  inport_data_i
    ,input  [  5:0]  inport_idx_i
    ,input           outport_ready_i

    // Outputs
    ,output          inport_accept_o
    ,output          outport_valid_o
    ,output [ 31:0]  outport_data0_o
    ,output [ 31:0]  outport_data1_o
    ,output [ 31:0]  outport_data2_o
    ,output [ 31:0]  outport_data3_o
    ,output [  2:0]  outport_idx_o
);



reg         block_wr_q;
reg         block_rd_q;
reg [5:0]   rd_idx_q;
reg [3:0]   rd_addr_q;

wire [4:0]  wr_ptr_w = {block_wr_q, inport_idx_i[3:0]};

wire [31:0] outport_data0_w;
wire [31:0] outport_data1_w;
wire [31:0] outport_data2_w;
wire [31:0] outport_data3_w;

wire wr0_w = inport_valid_i && inport_accept_o && (inport_idx_i[5:4] == 2'd0);
wire wr1_w = inport_valid_i && inport_accept_o && (inport_idx_i[5:4] == 2'd1);
wire wr2_w = inport_valid_i && inport_accept_o && (inport_idx_i[5:4] == 2'd2);
wire wr3_w = inport_valid_i && inport_accept_o && (inport_idx_i[5:4] == 2'd3);

jpeg_idct_transpose_ram
u_ram0
(
     .clk0_i(clk_i)
    ,.rst0_i(rst_i)
    ,.clk1_i(clk_i)
    ,.rst1_i(rst_i)

    ,.addr0_i(wr_ptr_w)
    ,.data0_i(inport_data_i)
    ,.wr0_i(wr0_w)
    ,.data0_o()

    ,.addr1_i({block_rd_q, rd_addr_q})
    ,.data1_i(32'b0)
    ,.wr1_i(1'b0)
    ,.data1_o(outport_data0_w)
);

jpeg_idct_transpose_ram
u_ram1
(
     .clk0_i(clk_i)
    ,.rst0_i(rst_i)
    ,.clk1_i(clk_i)
    ,.rst1_i(rst_i)

    ,.addr0_i(wr_ptr_w)
    ,.data0_i(inport_data_i)
    ,.wr0_i(wr1_w)
    ,.data0_o()

    ,.addr1_i({block_rd_q, rd_addr_q})
    ,.data1_i(32'b0)
    ,.wr1_i(1'b0)
    ,.data1_o(outport_data1_w)
);

jpeg_idct_transpose_ram
u_ram2
(
     .clk0_i(clk_i)
    ,.rst0_i(rst_i)
    ,.clk1_i(clk_i)
    ,.rst1_i(rst_i)

    ,.addr0_i(wr_ptr_w)
    ,.data0_i(inport_data_i)
    ,.wr0_i(wr2_w)
    ,.data0_o()

    ,.addr1_i({block_rd_q, rd_addr_q})
    ,.data1_i(32'b0)
    ,.wr1_i(1'b0)
    ,.data1_o(outport_data2_w)
);

jpeg_idct_transpose_ram
u_ram3
(
     .clk0_i(clk_i)
    ,.rst0_i(rst_i)
    ,.clk1_i(clk_i)
    ,.rst1_i(rst_i)

    ,.addr0_i(wr_ptr_w)
    ,.data0_i(inport_data_i)
    ,.wr0_i(wr3_w)
    ,.data0_o()

    ,.addr1_i({block_rd_q, rd_addr_q})
    ,.data1_i(32'b0)
    ,.wr1_i(1'b0)
    ,.data1_o(outport_data3_w)
);

//-----------------------------------------------------------------
// Input Buffer
//-----------------------------------------------------------------
reg [1:0] block_ready_q;

always @ (posedge clk_i )
if (rst_i)
begin
    block_ready_q     <= 2'b0;
    block_wr_q        <= 1'b0;
    block_rd_q        <= 1'b0;
end
else if (img_start_i)
begin
    block_ready_q     <= 2'b0;
    block_wr_q        <= 1'b0;
    block_rd_q        <= 1'b0;
end
else
begin
    if (inport_valid_i && inport_idx_i == 6'd63 && inport_accept_o)
    begin
        block_ready_q[block_wr_q] <= 1'b1;
        block_wr_q                <= ~block_wr_q;
    end

    if (outport_valid_o && rd_idx_q[5:0] == 6'd63)
    begin
        block_ready_q[block_rd_q] <= 1'b0;
        block_rd_q <= ~block_rd_q;
    end
end

assign inport_accept_o = ~block_ready_q[block_wr_q];

//-----------------------------------------------------------------
// FSM
//-----------------------------------------------------------------
localparam STATE_W           = 2;
localparam STATE_IDLE        = 2'd0;
localparam STATE_SETUP       = 2'd1;
localparam STATE_ACTIVE      = 2'd2;

reg [STATE_W-1:0] state_q;
reg [STATE_W-1:0] next_state_r;

always @ *
begin
    next_state_r = state_q;

    case (state_q)
    STATE_IDLE:
    begin
        if (block_ready_q[block_rd_q] && outport_ready_i)
            next_state_r = STATE_SETUP;
    end
    STATE_SETUP:
    begin
        next_state_r = STATE_ACTIVE;
    end
    STATE_ACTIVE:
    begin
        if (outport_valid_o && rd_idx_q == 6'd63)
            next_state_r = STATE_IDLE;
    end
    default: ;
    endcase

    if (img_start_i)
        next_state_r = STATE_IDLE;
end

always @ (posedge clk_i )
if (rst_i)
    state_q <= STATE_IDLE;
else
    state_q <= next_state_r;

always @ (posedge clk_i )
if (rst_i)
    rd_idx_q <= 6'b0;
else if (img_start_i)
    rd_idx_q <= 6'b0;
else if (state_q == STATE_ACTIVE)
    rd_idx_q <= rd_idx_q + 6'd1;

always @ (posedge clk_i )
if (rst_i)
    rd_addr_q <= 4'b0;
else if (state_q == STATE_IDLE)
    rd_addr_q <= 4'b0;
else if (state_q == STATE_SETUP)
    rd_addr_q <= 4'd8;
else if (state_q == STATE_ACTIVE)
begin
    case (rd_idx_q[2:0])
    3'd0: rd_addr_q <= rd_addr_q - 4'd8;
    3'd1: rd_addr_q <= rd_addr_q + 4'd8;
    3'd2: ;
    3'd3: rd_addr_q <= rd_addr_q - 4'd8;
    3'd4: rd_addr_q <= rd_addr_q + 4'd8;
    3'd5: rd_addr_q <= rd_addr_q - 4'd8;
    3'd6: rd_addr_q <= rd_addr_q + 4'd1;
    3'd7: rd_addr_q <= rd_addr_q + 4'd8;
    endcase
end

assign outport_valid_o = (state_q == STATE_ACTIVE);
assign outport_idx_o   = rd_idx_q[2:0];
assign outport_data0_o = outport_data0_w;
assign outport_data1_o = outport_data1_w;
assign outport_data2_o = outport_data2_w;
assign outport_data3_o = outport_data3_w;


endmodule
//-----------------------------------------------------------------
//                      Baseline JPEG Decoder
//                             V0.1
//                       Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//-----------------------------------------------------------------
//                      License: Apache 2.0
// This IP can be freely used in commercial projects, however you may
// want access to unreleased materials such as verification environments,
// or test vectors, as well as changes to the IP for integration purposes.
// If this is the case, contact the above address.
// I am interested to hear how and where this IP is used, so please get
// in touch!
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

module jpeg_idct
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           img_start_i
    ,input           img_end_i
    ,input           inport_valid_i
    ,input  [ 15:0]  inport_data_i
    ,input  [  5:0]  inport_idx_i
    ,input           inport_eob_i
    ,input  [ 31:0]  inport_id_i
    ,input           outport_accept_i

    // Outputs
    ,output          inport_accept_o
    ,output          outport_valid_o
    ,output [ 31:0]  outport_data_o
    ,output [  5:0]  outport_idx_o
    ,output [ 31:0]  outport_id_o
);




wire          input_valid_w;
wire [ 15:0]  input_data0_w;
wire [ 15:0]  input_data1_w;
wire [ 15:0]  input_data2_w;
wire [ 15:0]  input_data3_w;
wire [  2:0]  input_idx_w;
wire          input_ready_w;


jpeg_idct_ram
u_input
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    ,.img_start_i(img_start_i)
    ,.img_end_i(img_end_i)

    ,.inport_valid_i(inport_valid_i)
    ,.inport_data_i(inport_data_i)
    ,.inport_idx_i(inport_idx_i)
    ,.inport_eob_i(inport_eob_i)
    ,.inport_accept_o(inport_accept_o)

    ,.outport_valid_o(input_valid_w)
    ,.outport_data0_o(input_data0_w)
    ,.outport_data1_o(input_data1_w)
    ,.outport_data2_o(input_data2_w)
    ,.outport_data3_o(input_data3_w)
    ,.outport_idx_o(input_idx_w)
    ,.outport_ready_i(outport_accept_i)
);

wire          idct_x_valid_w;
wire [ 31:0]  idct_x_data_w;
wire [  5:0]  idct_x_idx_w;
wire          idct_x_accept_w;

jpeg_idct_x
u_idct_x
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    ,.img_start_i(img_start_i)
    ,.img_end_i(img_end_i)

    ,.inport_valid_i(input_valid_w)
    ,.inport_data0_i(input_data0_w)
    ,.inport_data1_i(input_data1_w)
    ,.inport_data2_i(input_data2_w)
    ,.inport_data3_i(input_data3_w)
    ,.inport_idx_i(input_idx_w)

    ,.outport_valid_o(idct_x_valid_w)
    ,.outport_data_o(idct_x_data_w)
    ,.outport_idx_o(idct_x_idx_w)
);

wire          transpose_valid_w;
wire [ 31:0]  transpose_data0_w;
wire [ 31:0]  transpose_data1_w;
wire [ 31:0]  transpose_data2_w;
wire [ 31:0]  transpose_data3_w;
wire [  2:0]  transpose_idx_w;
wire          transpose_ready_w = 1'b1;

jpeg_idct_transpose
u_transpose
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    ,.img_start_i(img_start_i)
    ,.img_end_i(img_end_i)

    ,.inport_valid_i(idct_x_valid_w)
    ,.inport_data_i(idct_x_data_w)
    ,.inport_idx_i(idct_x_idx_w)
    ,.inport_accept_o(idct_x_accept_w)

    ,.outport_valid_o(transpose_valid_w)
    ,.outport_data0_o(transpose_data0_w)
    ,.outport_data1_o(transpose_data1_w)
    ,.outport_data2_o(transpose_data2_w)
    ,.outport_data3_o(transpose_data3_w)
    ,.outport_idx_o(transpose_idx_w)
    ,.outport_ready_i(transpose_ready_w)
);

jpeg_idct_y
u_idct_y
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    ,.img_start_i(img_start_i)
    ,.img_end_i(img_end_i)

    ,.inport_valid_i(transpose_valid_w)
    ,.inport_data0_i(transpose_data0_w)
    ,.inport_data1_i(transpose_data1_w)
    ,.inport_data2_i(transpose_data2_w)
    ,.inport_data3_i(transpose_data3_w)
    ,.inport_idx_i(transpose_idx_w)

    ,.outport_valid_o(outport_valid_o)
    ,.outport_data_o(outport_data_o)
    ,.outport_idx_o(outport_idx_o)
);


jpeg_idct_fifo
#(
     .WIDTH(32)
    ,.DEPTH(8)
    ,.ADDR_W(3)
)
u_id_fifo
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    ,.flush_i(img_start_i)

    ,.push_i(inport_eob_i)
    ,.data_in_i(inport_id_i)
    ,.accept_o()

    ,.valid_o()
    ,.data_out_o(outport_id_o)
    ,.pop_i(outport_valid_o && outport_idx_o == 6'd63)
);

`ifdef verilator
function get_valid; /*verilator public*/
begin
    get_valid = outport_valid_o;
end
endfunction
function [5:0] get_sample_idx; /*verilator public*/
begin
    get_sample_idx = outport_idx_o;
end
endfunction
function [31:0] get_sample; /*verilator public*/
begin
    get_sample = outport_data_o;
end
endfunction
`endif


endmodule
//-----------------------------------------------------------------
//                      Baseline JPEG Decoder
//                             V0.1
//                       Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//-----------------------------------------------------------------
//                      License: Apache 2.0
// This IP can be freely used in commercial projects, however you may
// want access to unreleased materials such as verification environments,
// or test vectors, as well as changes to the IP for integration purposes.
// If this is the case, contact the above address.
// I am interested to hear how and where this IP is used, so please get
// in touch!
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

module jpeg_idct_x
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
     parameter OUT_SHIFT        = 11
    ,parameter INPUT_WIDTH      = 16
)
//-----------------------------------------------------------------
// Ports
//-----------------------------------------------------------------
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           img_start_i
    ,input           img_end_i
    ,input           inport_valid_i
    ,input  [ 15:0]  inport_data0_i
    ,input  [ 15:0]  inport_data1_i
    ,input  [ 15:0]  inport_data2_i
    ,input  [ 15:0]  inport_data3_i
    ,input  [  2:0]  inport_idx_i

    // Outputs
    ,output          outport_valid_o
    ,output [ 31:0]  outport_data_o
    ,output [  5:0]  outport_idx_o
);




localparam [15:0] C1_16 = 4017; // cos( pi/16) x4096
localparam [15:0] C2_16 = 3784; // cos(2pi/16) x4096
localparam [15:0] C3_16 = 3406; // cos(3pi/16) x4096
localparam [15:0] C4_16 = 2896; // cos(4pi/16) x4096
localparam [15:0] C5_16 = 2276; // cos(5pi/16) x4096
localparam [15:0] C6_16 = 1567; // cos(6pi/16) x4096
localparam [15:0] C7_16 = 799;  // cos(7pi/16) x4096

wire signed [31:0] block_in_0_1 = {{16{inport_data0_i[15]}}, inport_data0_i};
wire signed [31:0] block_in_2_3 = {{16{inport_data1_i[15]}}, inport_data1_i};
wire signed [31:0] block_in_4_5 = {{16{inport_data2_i[15]}}, inport_data2_i};
wire signed [31:0] block_in_6_7 = {{16{inport_data3_i[15]}}, inport_data3_i};

//-----------------------------------------------------------------
// IDCT
//-----------------------------------------------------------------
reg signed [31:0] i0;
reg signed [31:0] mul0_a;
reg signed [31:0] mul0_b;
reg signed [31:0] mul1_a;
reg signed [31:0] mul1_b;
reg signed [31:0] mul2_a;
reg signed [31:0] mul2_b;
reg signed [31:0] mul3_a;
reg signed [31:0] mul3_b;
reg signed [31:0] mul4_a;
reg signed [31:0] mul4_b;

always @ (posedge clk_i )
if (rst_i)
begin
    i0     <= 32'b0;
    mul0_a <= 32'b0;
    mul0_b <= 32'b0;
    mul1_a <= 32'b0;
    mul1_b <= 32'b0;
    mul2_a <= 32'b0;
    mul2_b <= 32'b0;
    mul3_a <= 32'b0;
    mul3_b <= 32'b0;
    mul4_a <= 32'b0;
    mul4_b <= 32'b0;
end
else
begin
    /* verilator lint_off WIDTH */
    case (inport_idx_i)
    3'd0:
    begin
        i0     <= block_in_0_1 + block_in_4_5;
        mul0_a <= block_in_2_3;
        mul0_b <= C2_16;
        mul1_a <= block_in_6_7;
        mul1_b <= C6_16;
    end
    3'd1:
    begin
        mul0_a <= block_in_0_1;
        mul0_b <= C1_16;
        mul1_a <= block_in_6_7;
        mul1_b <= C7_16;
        mul2_a <= block_in_4_5;
        mul2_b <= C5_16;
        mul3_a <= block_in_2_3;
        mul3_b <= C3_16;
        mul4_a <= i0;
        mul4_b <= C4_16;
    end
    3'd2:
    begin
        i0     <= block_in_0_1 - block_in_4_5;
    end
    3'd3:
    begin
        mul0_a <= block_in_0_1;
        mul0_b <= C7_16;
        mul1_a <= block_in_6_7;
        mul1_b <= C1_16;
        mul2_a <= block_in_4_5;
        mul2_b <= C3_16;
        mul3_a <= block_in_2_3;
        mul3_b <= C5_16;
    end
    3'd4:
    begin
        mul0_a <= block_in_0_1;
        mul0_b <= C7_16;
        mul1_a <= block_in_6_7;
        mul1_b <= C1_16;
        mul2_a <= block_in_4_5;
        mul2_b <= C3_16;
        mul3_a <= block_in_2_3;
        mul3_b <= C5_16;
    end
    3'd5:
    begin
        mul0_a <= block_in_2_3;
        mul0_b <= C6_16;
        mul1_a <= block_in_6_7;
        mul1_b <= C2_16;
        mul4_a <= i0;
        mul4_b <= C4_16;
    end
    default:
        ;
    endcase
    /* verilator lint_on WIDTH */
end

reg signed [31:0] mul0_q;
reg signed [31:0] mul1_q;
reg signed [31:0] mul2_q;
reg signed [31:0] mul3_q;
reg signed [31:0] mul4_q;

always @ (posedge clk_i )
if (rst_i)
begin
    mul0_q <= 32'b0;
    mul1_q <= 32'b0;
    mul2_q <= 32'b0;
    mul3_q <= 32'b0;
    mul4_q <= 32'b0;
end
else
begin
    mul0_q <= mul0_a * mul0_b;
    mul1_q <= mul1_a * mul1_b;
    mul2_q <= mul2_a * mul2_b;
    mul3_q <= mul3_a * mul3_b;
    mul4_q <= mul4_a * mul4_b;
end

reg signed [31:0] mul0;
reg signed [31:0] mul1;
reg signed [31:0] mul2;
reg signed [31:0] mul3;
reg signed [31:0] mul4;

always @ (posedge clk_i )
if (rst_i)
begin
    mul0 <= 32'b0;
    mul1 <= 32'b0;
    mul2 <= 32'b0;
    mul3 <= 32'b0;
    mul4 <= 32'b0;
end
else
begin
    mul0 <= mul0_q;
    mul1 <= mul1_q;
    mul2 <= mul2_q;
    mul3 <= mul3_q;
    mul4 <= mul4_q;
end

reg        out_stg0_valid_q;
reg [2:0]  out_stg0_idx_q;

always @ (posedge clk_i )
if (rst_i)
begin
    out_stg0_valid_q <= 1'b0;
    out_stg0_idx_q   <= 3'b0;
end
else
begin
    out_stg0_valid_q <= inport_valid_i;
    out_stg0_idx_q   <= inport_idx_i;
end

reg        out_stg1_valid_q;
reg [2:0]  out_stg1_idx_q;

always @ (posedge clk_i )
if (rst_i)
begin
    out_stg1_valid_q <= 1'b0;
    out_stg1_idx_q   <= 3'b0;
end
else
begin
    out_stg1_valid_q <= out_stg0_valid_q;
    out_stg1_idx_q   <= out_stg0_idx_q;
end

reg        out_stg2_valid_q;
reg [2:0]  out_stg2_idx_q;

always @ (posedge clk_i )
if (rst_i)
begin
    out_stg2_valid_q <= 1'b0;
    out_stg2_idx_q   <= 3'b0;
end
else
begin
    out_stg2_valid_q <= out_stg1_valid_q;
    out_stg2_idx_q   <= out_stg1_idx_q;
end

reg signed [31:0] o_s5;
reg signed [31:0] o_s6;
reg signed [31:0] o_s7;
reg signed [31:0] o_t0;
reg signed [31:0] o_t1;
reg signed [31:0] o_t2;
reg signed [31:0] o_t3;
reg signed [31:0] o_t4;
reg signed [31:0] o_t5;
reg signed [31:0] o_t6;
reg signed [31:0] o_t7;
reg signed [31:0] o_t6_5;
reg signed [31:0] o_t5_6;

always @ (posedge clk_i )
if (rst_i)
begin
    o_s5   <= 32'b0;
    o_s6   <= 32'b0;
    o_s7   <= 32'b0;
    o_t0   <= 32'b0;
    o_t1   <= 32'b0;
    o_t2   <= 32'b0;
    o_t3   <= 32'b0;
    o_t4   <= 32'b0;
    o_t5   <= 32'b0;
    o_t6   <= 32'b0;
    o_t7   <= 32'b0;
    o_t6_5 <= 32'b0;
    o_t5_6 <= 32'b0;
end
else
begin
    case (out_stg2_idx_q)
    3'd0:
    begin
        o_t3 <= mul0 + mul1; // s3
    end
    3'd1:
    begin
        o_s7 <= mul0 + mul1;
        o_s6 <= mul2 + mul3;
        o_t0 <= mul4;        // s0
    end
    3'd2:
    begin
        o_t0 <= o_t0 + o_t3; // t0
        o_t3 <= o_t0 - o_t3; // t3
        o_t7 <= o_s6 + o_s7;
    end
    3'd3:
    begin
        o_t4 <= (mul0 - mul1) + (mul2 - mul3);
    end
    3'd4:
    begin
        o_t0 <= mul0 - mul1; // s4
        o_s5 <= mul2 - mul3;    
    end
    3'd5:
    begin
        o_t3 <= mul0 - mul1; // s2
        o_t4 <= mul4; // s1
        o_t5 <= o_t0 - o_s5;
        o_t6 <= o_s7 - o_s6;
    end
    3'd6:
    begin
        o_t1 <= o_t4 + o_t3;
        o_t2 <= o_t4 - o_t3;
        o_t6_5 <= o_t6 - o_t5;
        o_t5_6 <= o_t5 + o_t6;
    end
    default:
    begin
        o_s5 <= (o_t6_5 * 181) / 256; // 1/sqrt(2)
        o_s6 <= (o_t5_6 * 181) / 256; // 1/sqrt(2)
    end
    endcase
end

reg        out_stg3_valid_q;
reg [2:0]  out_stg3_idx_q;

always @ (posedge clk_i )
if (rst_i)
begin
    out_stg3_valid_q <= 1'b0;
    out_stg3_idx_q   <= 3'b0;
end
else
begin
    out_stg3_valid_q <= out_stg2_valid_q;
    out_stg3_idx_q   <= out_stg2_idx_q;
end

reg signed [31:0] block_out[0:7];
reg signed [31:0] block_out_tmp;

always @ (posedge clk_i )
if (rst_i)
begin
    block_out[0] <= 32'b0;
    block_out[1] <= 32'b0;
    block_out[2] <= 32'b0;
    block_out[3] <= 32'b0;
    block_out[4] <= 32'b0;
    block_out[5] <= 32'b0;
    block_out[6] <= 32'b0;
    block_out[7] <= 32'b0;
    block_out_tmp <= 32'b0;
end
else if (out_stg3_valid_q)
begin
    if (out_stg3_idx_q == 3'd3)
    begin
        block_out[0] <= ((o_t0 + o_t7) >>> OUT_SHIFT);
        block_out_tmp <= ((o_t0 - o_t7) >>> OUT_SHIFT); // block_out[7]
        block_out[3] <= ((o_t3 + o_t4) >>> OUT_SHIFT);
        block_out[4] <= ((o_t3 - o_t4) >>> OUT_SHIFT);
    end

    if (out_stg3_idx_q == 3'd6)
        block_out[7] <= block_out_tmp;

    if (out_stg3_idx_q == 3'd7)
    begin
        block_out[2] <= ((o_t2 + o_s5) >>> OUT_SHIFT);
        block_out[5] <= ((o_t2 - o_s5) >>> OUT_SHIFT);
        block_out[1] <= ((o_t1 + o_s6) >>> OUT_SHIFT);
        block_out[6] <= ((o_t1 - o_s6) >>> OUT_SHIFT);
    end
end

reg [7:0] valid_q;

always @ (posedge clk_i )
if (rst_i)
    valid_q  <= 8'b0;
else if (img_start_i)
    valid_q  <= 8'b0;
else
    valid_q <= {valid_q[6:0], out_stg3_valid_q};

reg [5:0] ptr_q;

always @ (posedge clk_i )
if (rst_i)
    ptr_q <= 6'd0;
else if (img_start_i)
    ptr_q <= 6'd0;
else if (outport_valid_o)
    ptr_q <= ptr_q + 6'd1;

assign outport_valid_o = valid_q[6];
assign outport_data_o  = block_out[ptr_q[2:0]];

assign outport_idx_o   = ptr_q;



endmodule
//-----------------------------------------------------------------
//                      Baseline JPEG Decoder
//                             V0.1
//                       Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//-----------------------------------------------------------------
//                      License: Apache 2.0
// This IP can be freely used in commercial projects, however you may
// want access to unreleased materials such as verification environments,
// or test vectors, as well as changes to the IP for integration purposes.
// If this is the case, contact the above address.
// I am interested to hear how and where this IP is used, so please get
// in touch!
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

module jpeg_idct_y
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
     parameter OUT_SHIFT        = 15
    ,parameter INPUT_WIDTH      = 32
)
//-----------------------------------------------------------------
// Ports
//-----------------------------------------------------------------
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           img_start_i
    ,input           img_end_i
    ,input           inport_valid_i
    ,input  [ 31:0]  inport_data0_i
    ,input  [ 31:0]  inport_data1_i
    ,input  [ 31:0]  inport_data2_i
    ,input  [ 31:0]  inport_data3_i
    ,input  [  2:0]  inport_idx_i

    // Outputs
    ,output          outport_valid_o
    ,output [ 31:0]  outport_data_o
    ,output [  5:0]  outport_idx_o
);




localparam [15:0] C1_16 = 4017; // cos( pi/16) x4096
localparam [15:0] C2_16 = 3784; // cos(2pi/16) x4096
localparam [15:0] C3_16 = 3406; // cos(3pi/16) x4096
localparam [15:0] C4_16 = 2896; // cos(4pi/16) x4096
localparam [15:0] C5_16 = 2276; // cos(5pi/16) x4096
localparam [15:0] C6_16 = 1567; // cos(6pi/16) x4096
localparam [15:0] C7_16 = 799;  // cos(7pi/16) x4096

wire signed [31:0] block_in_0_1 = inport_data0_i;
wire signed [31:0] block_in_2_3 = inport_data1_i;
wire signed [31:0] block_in_4_5 = inport_data2_i;
wire signed [31:0] block_in_6_7 = inport_data3_i;

//-----------------------------------------------------------------
// IDCT
//-----------------------------------------------------------------
reg signed [31:0] i0;
reg signed [31:0] mul0_a;
reg signed [31:0] mul0_b;
reg signed [31:0] mul1_a;
reg signed [31:0] mul1_b;
reg signed [31:0] mul2_a;
reg signed [31:0] mul2_b;
reg signed [31:0] mul3_a;
reg signed [31:0] mul3_b;
reg signed [31:0] mul4_a;
reg signed [31:0] mul4_b;

always @ (posedge clk_i )
if (rst_i)
begin
    i0     <= 32'b0;
    mul0_a <= 32'b0;
    mul0_b <= 32'b0;
    mul1_a <= 32'b0;
    mul1_b <= 32'b0;
    mul2_a <= 32'b0;
    mul2_b <= 32'b0;
    mul3_a <= 32'b0;
    mul3_b <= 32'b0;
    mul4_a <= 32'b0;
    mul4_b <= 32'b0;
end
else
begin
    /* verilator lint_off WIDTH */
    case (inport_idx_i)
    3'd0:
    begin
        i0     <= block_in_0_1 + block_in_4_5;
        mul0_a <= block_in_2_3;
        mul0_b <= C2_16;
        mul1_a <= block_in_6_7;
        mul1_b <= C6_16;
    end
    3'd1:
    begin
        mul0_a <= block_in_0_1;
        mul0_b <= C1_16;
        mul1_a <= block_in_6_7;
        mul1_b <= C7_16;
        mul2_a <= block_in_4_5;
        mul2_b <= C5_16;
        mul3_a <= block_in_2_3;
        mul3_b <= C3_16;
        mul4_a <= i0;
        mul4_b <= C4_16;
    end
    3'd2:
    begin
        i0     <= block_in_0_1 - block_in_4_5;
    end
    3'd3:
    begin
        mul0_a <= block_in_0_1;
        mul0_b <= C7_16;
        mul1_a <= block_in_6_7;
        mul1_b <= C1_16;
        mul2_a <= block_in_4_5;
        mul2_b <= C3_16;
        mul3_a <= block_in_2_3;
        mul3_b <= C5_16;
    end
    3'd4:
    begin
        mul0_a <= block_in_0_1;
        mul0_b <= C7_16;
        mul1_a <= block_in_6_7;
        mul1_b <= C1_16;
        mul2_a <= block_in_4_5;
        mul2_b <= C3_16;
        mul3_a <= block_in_2_3;
        mul3_b <= C5_16;
    end
    3'd5:
    begin
        mul0_a <= block_in_2_3;
        mul0_b <= C6_16;
        mul1_a <= block_in_6_7;
        mul1_b <= C2_16;
        mul4_a <= i0;
        mul4_b <= C4_16;
    end
    default:
        ;
    endcase
    /* verilator lint_on WIDTH */
end

reg signed [31:0] mul0_q;
reg signed [31:0] mul1_q;
reg signed [31:0] mul2_q;
reg signed [31:0] mul3_q;
reg signed [31:0] mul4_q;

always @ (posedge clk_i )
if (rst_i)
begin
    mul0_q <= 32'b0;
    mul1_q <= 32'b0;
    mul2_q <= 32'b0;
    mul3_q <= 32'b0;
    mul4_q <= 32'b0;
end
else
begin
    mul0_q <= mul0_a * mul0_b;
    mul1_q <= mul1_a * mul1_b;
    mul2_q <= mul2_a * mul2_b;
    mul3_q <= mul3_a * mul3_b;
    mul4_q <= mul4_a * mul4_b;
end

reg signed [31:0] mul0;
reg signed [31:0] mul1;
reg signed [31:0] mul2;
reg signed [31:0] mul3;
reg signed [31:0] mul4;

always @ (posedge clk_i )
if (rst_i)
begin
    mul0 <= 32'b0;
    mul1 <= 32'b0;
    mul2 <= 32'b0;
    mul3 <= 32'b0;
    mul4 <= 32'b0;
end
else
begin
    mul0 <= mul0_q;
    mul1 <= mul1_q;
    mul2 <= mul2_q;
    mul3 <= mul3_q;
    mul4 <= mul4_q;
end

reg        out_stg0_valid_q;
reg [2:0]  out_stg0_idx_q;

always @ (posedge clk_i )
if (rst_i)
begin
    out_stg0_valid_q <= 1'b0;
    out_stg0_idx_q   <= 3'b0;
end
else
begin
    out_stg0_valid_q <= inport_valid_i;
    out_stg0_idx_q   <= inport_idx_i;
end

reg        out_stg1_valid_q;
reg [2:0]  out_stg1_idx_q;

always @ (posedge clk_i )
if (rst_i)
begin
    out_stg1_valid_q <= 1'b0;
    out_stg1_idx_q   <= 3'b0;
end
else
begin
    out_stg1_valid_q <= out_stg0_valid_q;
    out_stg1_idx_q   <= out_stg0_idx_q;
end

reg        out_stg2_valid_q;
reg [2:0]  out_stg2_idx_q;

always @ (posedge clk_i )
if (rst_i)
begin
    out_stg2_valid_q <= 1'b0;
    out_stg2_idx_q   <= 3'b0;
end
else
begin
    out_stg2_valid_q <= out_stg1_valid_q;
    out_stg2_idx_q   <= out_stg1_idx_q;
end

reg signed [31:0] o_s5;
reg signed [31:0] o_s6;
reg signed [31:0] o_s7;
reg signed [31:0] o_t0;
reg signed [31:0] o_t1;
reg signed [31:0] o_t2;
reg signed [31:0] o_t3;
reg signed [31:0] o_t4;
reg signed [31:0] o_t5;
reg signed [31:0] o_t6;
reg signed [31:0] o_t7;
reg signed [31:0] o_t6_5;
reg signed [31:0] o_t5_6;

always @ (posedge clk_i )
if (rst_i)
begin
    o_s5   <= 32'b0;
    o_s6   <= 32'b0;
    o_s7   <= 32'b0;
    o_t0   <= 32'b0;
    o_t1   <= 32'b0;
    o_t2   <= 32'b0;
    o_t3   <= 32'b0;
    o_t4   <= 32'b0;
    o_t5   <= 32'b0;
    o_t6   <= 32'b0;
    o_t7   <= 32'b0;
    o_t6_5 <= 32'b0;
    o_t5_6 <= 32'b0;
end
else
begin
    case (out_stg2_idx_q)
    3'd0:
    begin
        o_t3 <= mul0 + mul1; // s3
    end
    3'd1:
    begin
        o_s7 <= mul0 + mul1;
        o_s6 <= mul2 + mul3;
        o_t0 <= mul4;        // s0
    end
    3'd2:
    begin
        o_t0 <= o_t0 + o_t3; // t0
        o_t3 <= o_t0 - o_t3; // t3
        o_t7 <= o_s6 + o_s7;
    end
    3'd3:
    begin
        o_t4 <= (mul0 - mul1) + (mul2 - mul3);
    end
    3'd4:
    begin
        o_t0 <= mul0 - mul1; // s4
        o_s5 <= mul2 - mul3;    
    end
    3'd5:
    begin
        o_t3 <= mul0 - mul1; // s2
        o_t4 <= mul4; // s1
        o_t5 <= o_t0 - o_s5;
        o_t6 <= o_s7 - o_s6;
    end
    3'd6:
    begin
        o_t1 <= o_t4 + o_t3;
        o_t2 <= o_t4 - o_t3;
        o_t6_5 <= o_t6 - o_t5;
        o_t5_6 <= o_t5 + o_t6;
    end
    default:
    begin
        o_s5 <= (o_t6_5 * 181) / 256; // 1/sqrt(2)
        o_s6 <= (o_t5_6 * 181) / 256; // 1/sqrt(2)
    end
    endcase
end

reg        out_stg3_valid_q;
reg [2:0]  out_stg3_idx_q;

always @ (posedge clk_i )
if (rst_i)
begin
    out_stg3_valid_q <= 1'b0;
    out_stg3_idx_q   <= 3'b0;
end
else
begin
    out_stg3_valid_q <= out_stg2_valid_q;
    out_stg3_idx_q   <= out_stg2_idx_q;
end

reg signed [31:0] block_out[0:7];
reg signed [31:0] block_out_tmp;

always @ (posedge clk_i )
if (rst_i)
begin
    block_out[0] <= 32'b0;
    block_out[1] <= 32'b0;
    block_out[2] <= 32'b0;
    block_out[3] <= 32'b0;
    block_out[4] <= 32'b0;
    block_out[5] <= 32'b0;
    block_out[6] <= 32'b0;
    block_out[7] <= 32'b0;
    block_out_tmp <= 32'b0;
end
else if (out_stg3_valid_q)
begin
    if (out_stg3_idx_q == 3'd3)
    begin
        block_out[0] <= ((o_t0 + o_t7) >>> OUT_SHIFT);
        block_out_tmp <= ((o_t0 - o_t7) >>> OUT_SHIFT); // block_out[7]
        block_out[3] <= ((o_t3 + o_t4) >>> OUT_SHIFT);
        block_out[4] <= ((o_t3 - o_t4) >>> OUT_SHIFT);
    end

    if (out_stg3_idx_q == 3'd6)
        block_out[7] <= block_out_tmp;

    if (out_stg3_idx_q == 3'd7)
    begin
        block_out[2] <= ((o_t2 + o_s5) >>> OUT_SHIFT);
        block_out[5] <= ((o_t2 - o_s5) >>> OUT_SHIFT);
        block_out[1] <= ((o_t1 + o_s6) >>> OUT_SHIFT);
        block_out[6] <= ((o_t1 - o_s6) >>> OUT_SHIFT);
    end
end

reg [7:0] valid_q;

always @ (posedge clk_i )
if (rst_i)
    valid_q  <= 8'b0;
else if (img_start_i)
    valid_q  <= 8'b0;
else
    valid_q <= {valid_q[6:0], out_stg3_valid_q};

reg [5:0] ptr_q;

always @ (posedge clk_i )
if (rst_i)
    ptr_q <= 6'd0;
else if (img_start_i)
    ptr_q <= 6'd0;
else if (outport_valid_o)
    ptr_q <= ptr_q + 6'd1;

assign outport_valid_o = valid_q[6];
assign outport_data_o  = block_out[ptr_q[2:0]];



function [5:0] ptr_conv;
    input [5:0] idx;
    reg [5:0] out_idx;
begin
    case (idx)
    6'd0:  out_idx = 6'd0;
    6'd1:  out_idx = 6'd8;
    6'd2:  out_idx = 6'd16;
    6'd3:  out_idx = 6'd24;
    6'd4:  out_idx = 6'd32;
    6'd5:  out_idx = 6'd40;
    6'd6:  out_idx = 6'd48;
    6'd7:  out_idx = 6'd56;
    6'd8:  out_idx = 6'd1;
    6'd9:  out_idx = 6'd9;
    6'd10:  out_idx = 6'd17;
    6'd11:  out_idx = 6'd25;
    6'd12:  out_idx = 6'd33;
    6'd13:  out_idx = 6'd41;
    6'd14:  out_idx = 6'd49;
    6'd15:  out_idx = 6'd57;
    6'd16:  out_idx = 6'd2;
    6'd17:  out_idx = 6'd10;
    6'd18:  out_idx = 6'd18;
    6'd19:  out_idx = 6'd26;
    6'd20:  out_idx = 6'd34;
    6'd21:  out_idx = 6'd42;
    6'd22:  out_idx = 6'd50;
    6'd23:  out_idx = 6'd58;
    6'd24:  out_idx = 6'd3;
    6'd25:  out_idx = 6'd11;
    6'd26:  out_idx = 6'd19;
    6'd27:  out_idx = 6'd27;
    6'd28:  out_idx = 6'd35;
    6'd29:  out_idx = 6'd43;
    6'd30:  out_idx = 6'd51;
    6'd31:  out_idx = 6'd59;
    6'd32:  out_idx = 6'd4;
    6'd33:  out_idx = 6'd12;
    6'd34:  out_idx = 6'd20;
    6'd35:  out_idx = 6'd28;
    6'd36:  out_idx = 6'd36;
    6'd37:  out_idx = 6'd44;
    6'd38:  out_idx = 6'd52;
    6'd39:  out_idx = 6'd60;
    6'd40:  out_idx = 6'd5;
    6'd41:  out_idx = 6'd13;
    6'd42:  out_idx = 6'd21;
    6'd43:  out_idx = 6'd29;
    6'd44:  out_idx = 6'd37;
    6'd45:  out_idx = 6'd45;
    6'd46:  out_idx = 6'd53;
    6'd47:  out_idx = 6'd61;
    6'd48:  out_idx = 6'd6;
    6'd49:  out_idx = 6'd14;
    6'd50:  out_idx = 6'd22;
    6'd51:  out_idx = 6'd30;
    6'd52:  out_idx = 6'd38;
    6'd53:  out_idx = 6'd46;
    6'd54:  out_idx = 6'd54;
    6'd55:  out_idx = 6'd62;
    6'd56:  out_idx = 6'd7;
    6'd57:  out_idx = 6'd15;
    6'd58:  out_idx = 6'd23;
    6'd59:  out_idx = 6'd31;
    6'd60:  out_idx = 6'd39;
    6'd61:  out_idx = 6'd47;
    6'd62:  out_idx = 6'd55;
    default:  out_idx = 6'd63;
    endcase

    ptr_conv = out_idx;
end
endfunction


assign outport_idx_o   = ptr_conv(ptr_q);



endmodule
//-----------------------------------------------------------------
//                      Baseline JPEG Decoder
//                             V0.1
//                       Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//-----------------------------------------------------------------
//                      License: Apache 2.0
// This IP can be freely used in commercial projects, however you may
// want access to unreleased materials such as verification environments,
// or test vectors, as well as changes to the IP for integration purposes.
// If this is the case, contact the above address.
// I am interested to hear how and where this IP is used, so please get
// in touch!
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

module jpeg_input
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           inport_valid_i
    ,input  [ 31:0]  inport_data_i
    ,input  [  3:0]  inport_strb_i
    ,input           inport_last_i
    ,input           dqt_cfg_accept_i
    ,input           dht_cfg_accept_i
    ,input           data_accept_i

    // Outputs
    ,output          inport_accept_o
    ,output          img_start_o
    ,output          img_end_o
    ,output [ 15:0]  img_width_o
    ,output [ 15:0]  img_height_o
    ,output [  1:0]  img_mode_o
    ,output [  1:0]  img_dqt_table_y_o
    ,output [  1:0]  img_dqt_table_cb_o
    ,output [  1:0]  img_dqt_table_cr_o
    ,output          dqt_cfg_valid_o
    ,output [  7:0]  dqt_cfg_data_o
    ,output          dqt_cfg_last_o
    ,output          dht_cfg_valid_o
    ,output [  7:0]  dht_cfg_data_o
    ,output          dht_cfg_last_o
    ,output          data_valid_o
    ,output [  7:0]  data_data_o
    ,output          data_last_o
);



wire inport_accept_w;

//-----------------------------------------------------------------
// Input data read index
//-----------------------------------------------------------------
reg [1:0] byte_idx_q;

always @ (posedge clk_i )
if (rst_i)
    byte_idx_q <= 2'b0;
else if (inport_valid_i && inport_accept_w && inport_last_i)
    byte_idx_q <= 2'b0;
else if (inport_valid_i && inport_accept_w)
    byte_idx_q <= byte_idx_q + 2'd1;

//-----------------------------------------------------------------
// Data mux
//-----------------------------------------------------------------
reg [7:0] data_r;

always @ *
begin
    data_r = 8'b0;

    case (byte_idx_q)
    default: data_r = {8{inport_strb_i[0]}} & inport_data_i[7:0];
    2'd1:    data_r = {8{inport_strb_i[1]}} & inport_data_i[15:8];
    2'd2:    data_r = {8{inport_strb_i[2]}} & inport_data_i[23:16];
    2'd3:    data_r = {8{inport_strb_i[3]}} & inport_data_i[31:24];
    endcase
end

//-----------------------------------------------------------------
// Last data
//-----------------------------------------------------------------
reg [7:0] last_b_q;

always @ (posedge clk_i )
if (rst_i)
    last_b_q <= 8'b0;
else if (inport_valid_i && inport_accept_w)
    last_b_q <= inport_last_i ? 8'b0 : data_r;

//-----------------------------------------------------------------
// Token decoder
//-----------------------------------------------------------------
wire token_soi_w  = (last_b_q == 8'hFF && data_r == 8'hd8);
wire token_sof0_w = (last_b_q == 8'hFF && data_r == 8'hc0);
wire token_dqt_w  = (last_b_q == 8'hFF && data_r == 8'hdb);
wire token_dht_w  = (last_b_q == 8'hFF && data_r == 8'hc4);
wire token_eoi_w  = (last_b_q == 8'hFF && data_r == 8'hd9);
wire token_sos_w  = (last_b_q == 8'hFF && data_r == 8'hda);
wire token_pad_w  = (last_b_q == 8'hFF && data_r == 8'h00);

// Unsupported
wire token_sof2_w = (last_b_q == 8'hFF && data_r == 8'hc2);
wire token_dri_w  = (last_b_q == 8'hFF && data_r == 8'hdd);
wire token_rst_w  = (last_b_q == 8'hFF && data_r >= 8'hd0 && data_r <= 8'hd7);
wire token_app_w  = (last_b_q == 8'hFF && data_r >= 8'he0 && data_r <= 8'hef);
wire token_com_w  = (last_b_q == 8'hFF && data_r == 8'hfe);

//-----------------------------------------------------------------
// FSM
//-----------------------------------------------------------------
localparam STATE_W           = 5;
localparam STATE_IDLE        = 5'd0;
localparam STATE_ACTIVE      = 5'd1;
localparam STATE_UXP_LENH    = 5'd2;
localparam STATE_UXP_LENL    = 5'd3;
localparam STATE_UXP_DATA    = 5'd4;
localparam STATE_DQT_LENH    = 5'd5;
localparam STATE_DQT_LENL    = 5'd6;
localparam STATE_DQT_DATA    = 5'd7;
localparam STATE_DHT_LENH    = 5'd8;
localparam STATE_DHT_LENL    = 5'd9;
localparam STATE_DHT_DATA    = 5'd10;
localparam STATE_IMG_LENH    = 5'd11;
localparam STATE_IMG_LENL    = 5'd12;
localparam STATE_IMG_SOS     = 5'd13;
localparam STATE_IMG_DATA    = 5'd14;
localparam STATE_SOF_LENH    = 5'd15;
localparam STATE_SOF_LENL    = 5'd16;
localparam STATE_SOF_DATA    = 5'd17;

reg [STATE_W-1:0] state_q;
reg [15:0]        length_q;

reg [STATE_W-1:0] next_state_r;
always @ *
begin
    next_state_r = state_q;

    case (state_q)
    //-------------------------------------------------------------
    // IDLE - waiting for SOI
    //-------------------------------------------------------------
    STATE_IDLE :
    begin
        if (token_soi_w)
            next_state_r = STATE_ACTIVE;
    end
    //-------------------------------------------------------------
    // ACTIVE - waiting for various image markers
    //-------------------------------------------------------------
    STATE_ACTIVE :
    begin
        if (token_eoi_w)
            next_state_r = STATE_IDLE;
        else if (token_dqt_w)
            next_state_r = STATE_DQT_LENH;
        else if (token_dht_w)
            next_state_r = STATE_DHT_LENH;
        else if (token_sos_w)
            next_state_r = STATE_IMG_LENH;
        else if (token_sof0_w)
            next_state_r = STATE_SOF_LENH;
        // Unsupported
        else if (token_sof2_w ||
                 token_dri_w ||
                 token_rst_w ||
                 token_app_w ||
                 token_com_w)
            next_state_r = STATE_UXP_LENH;

    end
    //-------------------------------------------------------------
    // IMG
    //-------------------------------------------------------------
    STATE_IMG_LENH :
    begin
        if (inport_valid_i)
            next_state_r = STATE_IMG_LENL;
    end
    STATE_IMG_LENL :
    begin
        if (inport_valid_i)
            next_state_r = STATE_IMG_SOS;
    end
    STATE_IMG_SOS :
    begin
        if (inport_valid_i && length_q <= 16'd1)
            next_state_r = STATE_IMG_DATA;
    end
    STATE_IMG_DATA :
    begin
        if (token_eoi_w)
            next_state_r = STATE_IDLE;
    end
    //-------------------------------------------------------------
    // DQT
    //-------------------------------------------------------------
    STATE_DQT_LENH :
    begin
        if (inport_valid_i)
            next_state_r = STATE_DQT_LENL;
    end
    STATE_DQT_LENL :
    begin
        if (inport_valid_i)
            next_state_r = STATE_DQT_DATA;
    end
    STATE_DQT_DATA :
    begin
        if (inport_valid_i && inport_accept_w && length_q <= 16'd1)
            next_state_r = STATE_ACTIVE;
    end
    //-------------------------------------------------------------
    // SOF
    //-------------------------------------------------------------
    STATE_SOF_LENH :
    begin
        if (inport_valid_i)
            next_state_r = STATE_SOF_LENL;
    end
    STATE_SOF_LENL :
    begin
        if (inport_valid_i)
            next_state_r = STATE_SOF_DATA;
    end
    STATE_SOF_DATA :
    begin
        if (inport_valid_i && inport_accept_w && length_q <= 16'd1)
            next_state_r = STATE_ACTIVE;
    end
    //-------------------------------------------------------------
    // DHT
    //-------------------------------------------------------------
    STATE_DHT_LENH :
    begin
        if (inport_valid_i)
            next_state_r = STATE_DHT_LENL;
    end
    STATE_DHT_LENL :
    begin
        if (inport_valid_i)
            next_state_r = STATE_DHT_DATA;
    end
    STATE_DHT_DATA :
    begin
        if (inport_valid_i && inport_accept_w && length_q <= 16'd1)
            next_state_r = STATE_ACTIVE;
    end
    //-------------------------------------------------------------
    // Unsupported sections - skip
    //-------------------------------------------------------------
    STATE_UXP_LENH :
    begin
        if (inport_valid_i)
            next_state_r = STATE_UXP_LENL;
    end
    STATE_UXP_LENL :
    begin
        if (inport_valid_i)
            next_state_r = STATE_UXP_DATA;
    end
    STATE_UXP_DATA :
    begin
        if (inport_valid_i && inport_accept_w && length_q <= 16'd1)
            next_state_r = STATE_ACTIVE;
    end
    default:
        ;
    endcase

    // End of data stream
    if (inport_valid_i && inport_last_i && inport_accept_w)
        next_state_r = STATE_IDLE;
end

always @ (posedge clk_i )
if (rst_i)
    state_q <= STATE_IDLE;
else
    state_q <= next_state_r;

//-----------------------------------------------------------------
// Length
//-----------------------------------------------------------------
always @ (posedge clk_i )
if (rst_i)
    length_q <= 16'b0;
else if (state_q == STATE_UXP_LENH || state_q == STATE_DQT_LENH || 
         state_q == STATE_DHT_LENH || state_q == STATE_IMG_LENH ||
         state_q == STATE_SOF_LENH)
    length_q <= {data_r, 8'b0};
else if (state_q == STATE_UXP_LENL || state_q == STATE_DQT_LENL ||
         state_q == STATE_DHT_LENL || state_q == STATE_IMG_LENL ||
         state_q == STATE_SOF_LENL)
    length_q <= {8'b0, data_r} - 16'd2;
else if ((state_q == STATE_UXP_DATA || 
          state_q == STATE_DQT_DATA ||
          state_q == STATE_DHT_DATA ||
          state_q == STATE_SOF_DATA ||
          state_q == STATE_IMG_SOS) && inport_valid_i && inport_accept_w)
    length_q <= length_q - 16'd1;

//-----------------------------------------------------------------
// DQT
//-----------------------------------------------------------------
assign dqt_cfg_valid_o = (state_q == STATE_DQT_DATA) && inport_valid_i;
assign dqt_cfg_data_o  = data_r;
assign dqt_cfg_last_o  = inport_last_i || (length_q == 16'd1);

//-----------------------------------------------------------------
// DQT
//-----------------------------------------------------------------
assign dht_cfg_valid_o = (state_q == STATE_DHT_DATA) && inport_valid_i;
assign dht_cfg_data_o  = data_r;
assign dht_cfg_last_o  = inport_last_i || (length_q == 16'd1);

//-----------------------------------------------------------------
// Image data
//-----------------------------------------------------------------
reg       data_valid_q;
reg [7:0] data_data_q;
reg       data_last_q;

always @ (posedge clk_i )
if (rst_i)
    data_valid_q <= 1'b0;
else if (inport_valid_i && data_accept_i)
    data_valid_q <= (state_q == STATE_IMG_DATA) && (inport_valid_i && ~token_pad_w && ~token_eoi_w);
else if (state_q != STATE_IMG_DATA)
    data_valid_q <= 1'b0;

always @ (posedge clk_i )
if (rst_i)
    data_data_q <= 8'b0;
else if (inport_valid_i && data_accept_i)
    data_data_q <= data_r;

assign data_valid_o = data_valid_q && inport_valid_i && !token_eoi_w;
assign data_data_o  = data_data_q;

// NOTE: Last is delayed by one cycles (not qualified by data_valid_o)
assign data_last_o  = data_valid_q && inport_valid_i && token_eoi_w;

//-----------------------------------------------------------------
// Handshaking
//-----------------------------------------------------------------
wire last_byte_w = (byte_idx_q == 2'd3) || inport_last_i;

assign inport_accept_w =  (state_q == STATE_DQT_DATA && dqt_cfg_accept_i) ||
                          (state_q == STATE_DHT_DATA && dht_cfg_accept_i) ||
                          (state_q == STATE_IMG_DATA && (data_accept_i || token_pad_w)) ||
                          (state_q != STATE_DQT_DATA && 
                           state_q != STATE_DHT_DATA && 
                           state_q != STATE_IMG_DATA);

assign inport_accept_o = last_byte_w && inport_accept_w;

//-----------------------------------------------------------------
// Capture Index
//-----------------------------------------------------------------
reg [5:0] idx_q;

always @ (posedge clk_i )
if (rst_i)
    idx_q <= 6'b0;
else if (inport_valid_i && inport_accept_w && state_q == STATE_SOF_DATA)
    idx_q <= idx_q + 6'd1;
else if (state_q == STATE_SOF_LENH)
    idx_q <= 6'b0;

//-----------------------------------------------------------------
// SOF capture
//-----------------------------------------------------------------
reg [7:0] img_precision_q;

always @ (posedge clk_i )
if (rst_i)
    img_precision_q <= 8'b0;
else if (token_sof0_w)
    img_precision_q <= 8'b0;
else if (state_q == STATE_SOF_DATA && idx_q == 6'd0)
    img_precision_q <= data_r;

reg [15:0] img_height_q;

always @ (posedge clk_i )
if (rst_i)
    img_height_q <= 16'b0;
else if (token_sof0_w)
    img_height_q <= 16'b0;
else if (state_q == STATE_SOF_DATA && idx_q == 6'd1)
    img_height_q <= {data_r, 8'b0};
else if (state_q == STATE_SOF_DATA && idx_q == 6'd2)
    img_height_q <= {img_height_q[15:8], data_r};

assign img_height_o = img_height_q;

reg [15:0] img_width_q;

always @ (posedge clk_i )
if (rst_i)
    img_width_q <= 16'b0;
else if (token_sof0_w)
    img_width_q <= 16'b0;
else if (state_q == STATE_SOF_DATA && idx_q == 6'd3)
    img_width_q <= {data_r, 8'b0};
else if (state_q == STATE_SOF_DATA && idx_q == 6'd4)
    img_width_q <= {img_width_q[15:8], data_r};

assign img_width_o  = img_width_q;

reg [7:0] img_num_comp_q;

always @ (posedge clk_i )
if (rst_i)
    img_num_comp_q <= 8'b0;
else if (token_sof0_w)
    img_num_comp_q <= 8'b0;
else if (state_q == STATE_SOF_DATA && idx_q == 6'd5)
    img_num_comp_q <= data_r;

reg [7:0] img_y_factor_q;

always @ (posedge clk_i )
if (rst_i)
    img_y_factor_q <= 8'b0;
else if (token_sof0_w)
    img_y_factor_q <= 8'b0;
else if (state_q == STATE_SOF_DATA && idx_q == 6'd7)
    img_y_factor_q <= data_r;

reg [1:0] img_y_dqt_table_q;

always @ (posedge clk_i )
if (rst_i)
    img_y_dqt_table_q <= 2'b0;
else if (token_sof0_w)
    img_y_dqt_table_q <= 2'b0;
else if (state_q == STATE_SOF_DATA && idx_q == 6'd8)
    img_y_dqt_table_q <= data_r[1:0];

reg [7:0] img_cb_factor_q;

always @ (posedge clk_i )
if (rst_i)
    img_cb_factor_q <= 8'b0;
else if (token_sof0_w)
    img_cb_factor_q <= 8'b0;
else if (state_q == STATE_SOF_DATA && idx_q == 6'd10)
    img_cb_factor_q <= data_r;

reg [1:0] img_cb_dqt_table_q;

always @ (posedge clk_i )
if (rst_i)
    img_cb_dqt_table_q <= 2'b0;
else if (token_sof0_w)
    img_cb_dqt_table_q <= 2'b0;
else if (state_q == STATE_SOF_DATA && idx_q == 6'd11)
    img_cb_dqt_table_q <= data_r[1:0];

reg [7:0] img_cr_factor_q;

always @ (posedge clk_i )
if (rst_i)
    img_cr_factor_q <= 8'b0;
else if (token_sof0_w)
    img_cr_factor_q <= 8'b0;
else if (state_q == STATE_SOF_DATA && idx_q == 6'd13)
    img_cr_factor_q <= data_r;

reg [1:0] img_cr_dqt_table_q;

always @ (posedge clk_i )
if (rst_i)
    img_cr_dqt_table_q <= 2'b0;
else if (token_sof0_w)
    img_cr_dqt_table_q <= 2'b0;
else if (state_q == STATE_SOF_DATA && idx_q == 6'd14)
    img_cr_dqt_table_q <= data_r[1:0];

assign img_dqt_table_y_o  = img_y_dqt_table_q;
assign img_dqt_table_cb_o = img_cb_dqt_table_q;
assign img_dqt_table_cr_o = img_cr_dqt_table_q;

wire [3:0] y_horiz_factor_w  = img_y_factor_q[7:4];
wire [3:0] y_vert_factor_w   = img_y_factor_q[3:0];
wire [3:0] cb_horiz_factor_w = img_cb_factor_q[7:4];
wire [3:0] cb_vert_factor_w  = img_cb_factor_q[3:0];
wire [3:0] cr_horiz_factor_w = img_cr_factor_q[7:4];
wire [3:0] cr_vert_factor_w  = img_cr_factor_q[3:0];

localparam JPEG_MONOCHROME  = 2'd0;
localparam JPEG_YCBCR_444   = 2'd1;
localparam JPEG_YCBCR_420   = 2'd2;
localparam JPEG_UNSUPPORTED = 2'd3;

reg [1:0] img_mode_q;

always @ (posedge clk_i )
if (rst_i)
    img_mode_q <= JPEG_UNSUPPORTED;
else if (token_sof0_w)
    img_mode_q <= JPEG_UNSUPPORTED;
else if (state_q == STATE_SOF_DATA && next_state_r == STATE_ACTIVE)
begin
    // Single component (Y)
    if (img_num_comp_q == 8'd1)
        img_mode_q <= JPEG_MONOCHROME;
    // Colour image (YCbCr)
    else if (img_num_comp_q == 8'd3)
    begin
        if (y_horiz_factor_w  == 4'd1 && y_vert_factor_w  == 4'd1 &&
            cb_horiz_factor_w == 4'd1 && cb_vert_factor_w == 4'd1 &&
            cr_horiz_factor_w == 4'd1 && cr_vert_factor_w == 4'd1)
            img_mode_q <= JPEG_YCBCR_444;
        else if (y_horiz_factor_w  == 4'd2 && y_vert_factor_w  == 4'd2 &&
                 cb_horiz_factor_w == 4'd1 && cb_vert_factor_w == 4'd1 &&
                 cr_horiz_factor_w == 4'd1 && cr_vert_factor_w == 4'd1)
            img_mode_q <= JPEG_YCBCR_420;
    end
end

reg eof_q;

always @ (posedge clk_i )
if (rst_i)
    eof_q <= 1'b1;
else if (state_q == STATE_IDLE && token_soi_w)
    eof_q <= 1'b0;
else if (img_end_o)
    eof_q <= 1'b1;

reg start_q;

always @ (posedge clk_i )
if (rst_i)
    start_q <= 1'b0;
else if (inport_valid_i & token_sos_w)
    start_q <= 1'b0;
else if (state_q == STATE_IDLE && token_soi_w)
    start_q <= 1'b1;    

assign img_start_o = start_q;
assign img_end_o   = eof_q | (inport_valid_i & token_eoi_w);
assign img_mode_o  = img_mode_q;


endmodule
//-----------------------------------------------------------------
//                      Baseline JPEG Decoder
//                             V0.1
//                       Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//-----------------------------------------------------------------
//                      License: Apache 2.0
// This IP can be freely used in commercial projects, however you may
// want access to unreleased materials such as verification environments,
// or test vectors, as well as changes to the IP for integration purposes.
// If this is the case, contact the above address.
// I am interested to hear how and where this IP is used, so please get
// in touch!
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

module jpeg_mcu_id
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           img_start_i
    ,input           img_end_i
    ,input  [ 15:0]  img_width_i
    ,input  [ 15:0]  img_height_i
    ,input  [  1:0]  img_mode_i
    ,input           start_of_block_i
    ,input           end_of_block_i

    // Outputs
    ,output [ 31:0]  block_id_o
    ,output [  1:0]  block_type_o
    ,output          end_of_image_o
);



//-----------------------------------------------------------------
// Block Type (Y, Cb, Cr)
//-----------------------------------------------------------------
localparam JPEG_MONOCHROME  = 2'd0;
localparam JPEG_YCBCR_444   = 2'd1;
localparam JPEG_YCBCR_420   = 2'd2;
localparam JPEG_UNSUPPORTED = 2'd3;

localparam BLOCK_Y          = 2'd0;
localparam BLOCK_CB         = 2'd1;
localparam BLOCK_CR         = 2'd2;
localparam BLOCK_EOF        = 2'd3;

reg [1:0] block_type_q;
reg [2:0] type_idx_q;

always @ (posedge clk_i )
if (rst_i)
begin
    block_type_q <= BLOCK_Y;
    type_idx_q   <= 3'd0;
end
else if (img_start_i)
begin
    block_type_q <= BLOCK_Y;
    type_idx_q   <= 3'd0;
end
else if (start_of_block_i && end_of_image_o)
begin
    block_type_q <= BLOCK_EOF;
    type_idx_q   <= 3'd0;
end
else if (img_mode_i == JPEG_MONOCHROME)
    block_type_q <= BLOCK_Y;
else if (img_mode_i == JPEG_YCBCR_444 && end_of_block_i)
begin
    if (block_type_q == BLOCK_CR)
        block_type_q <= BLOCK_Y;
    else
        block_type_q <= block_type_q + 2'd1;
end
else if (img_mode_i == JPEG_YCBCR_420 && end_of_block_i)
begin
    type_idx_q <= type_idx_q + 3'd1;

    case (type_idx_q)
    default:
        block_type_q <= BLOCK_Y;
    3'd3:
        block_type_q <= BLOCK_CB;
    3'd4:
        block_type_q <= BLOCK_CR;
    3'd5:
    begin
        block_type_q <= BLOCK_Y;
        type_idx_q   <= 3'd0;
    end
    endcase
end

//-----------------------------------------------------------------
// Block index
//-----------------------------------------------------------------
wire [15:0] width_rnd_w   = ((img_width_i+7) / 8) * 8;
wire [15:0] block_x_max_w = width_rnd_w / 8;
wire [15:0] img_w_div4_w  = width_rnd_w / 4;

reg  [15:0] block_x_q;
reg  [15:0] block_y_q;

reg  [15:0] x_idx_q;
reg  [15:0] y_idx_q;

wire [15:0] block_x_next_w = block_x_q + 16'd1;

reg         end_of_image_q;

always @ (posedge clk_i )
if (rst_i)
begin
    block_x_q      <= 16'b0;
    block_y_q      <= 16'b0;
    x_idx_q        <= 16'b0;
    y_idx_q        <= 16'b0;
    end_of_image_q <= 1'b0;
end
else if (img_start_i)
begin
    block_x_q      <= 16'b0;
    block_y_q      <= 16'b0;
    x_idx_q        <= 16'b0;
    y_idx_q        <= 16'b0;
    end_of_image_q <= 1'b0;
end
else if (end_of_block_i && ((img_mode_i == JPEG_MONOCHROME) || (img_mode_i == JPEG_YCBCR_444 && block_type_q == BLOCK_CR)))
begin
    if (block_x_next_w == block_x_max_w)
    begin
        block_x_q <= 16'b0;
        block_y_q <= block_y_q + 16'd1;
    end
    else
        block_x_q <= block_x_next_w;

    if (img_end_i && block_x_next_w == block_x_max_w)
        end_of_image_q <= 1'b1;
end
else if (start_of_block_i && img_mode_i == JPEG_YCBCR_420 && block_type_q == BLOCK_Y)
begin
    block_x_q <= ({x_idx_q[15:2], 2'b0} / 2) + (type_idx_q[0] ? 16'd1 : 16'd0);
    block_y_q <= y_idx_q + (type_idx_q[1] ? 16'd1 : 16'd0);

    // Y component
    if (type_idx_q < 3'd4)
    begin
        if ((x_idx_q + 16'd1) == img_w_div4_w)
        begin
            x_idx_q <= 16'd0;
            y_idx_q <= y_idx_q + 16'd2;
        end
        else
            x_idx_q <= x_idx_q + 16'd1;
    end
end
else if (start_of_block_i && img_mode_i == JPEG_YCBCR_420 && block_type_q == BLOCK_CR)
begin
    if (img_end_i && block_x_next_w == block_x_max_w)
        end_of_image_q <= 1'b1;
end

//-----------------------------------------------------------------
// Outputs
//-----------------------------------------------------------------
// Block ID (type=y|cb|cr, y pos, x pos)
assign block_id_o     = {block_type_q, block_y_q[13:0], block_x_q};
assign block_type_o   = block_type_q;

// End of image detection
assign end_of_image_o = end_of_image_q;


endmodule
//-----------------------------------------------------------------
//                      Baseline JPEG Decoder
//                             V0.1
//                       Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//-----------------------------------------------------------------
//                      License: Apache 2.0
// This IP can be freely used in commercial projects, however you may
// want access to unreleased materials such as verification environments,
// or test vectors, as well as changes to the IP for integration purposes.
// If this is the case, contact the above address.
// I am interested to hear how and where this IP is used, so please get
// in touch!
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

module jpeg_mcu_proc
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           img_start_i
    ,input           img_end_i
    ,input  [ 15:0]  img_width_i
    ,input  [ 15:0]  img_height_i
    ,input  [  1:0]  img_mode_i
    ,input           inport_valid_i
    ,input  [ 31:0]  inport_data_i
    ,input           inport_last_i
    ,input           lookup_valid_i
    ,input  [  4:0]  lookup_width_i
    ,input  [  7:0]  lookup_value_i
    ,input           outport_blk_space_i

    // Outputs
    ,output [  5:0]  inport_pop_o
    ,output          lookup_req_o
    ,output [  1:0]  lookup_table_o
    ,output [ 15:0]  lookup_input_o
    ,output          outport_valid_o
    ,output [ 15:0]  outport_data_o
    ,output [  5:0]  outport_idx_o
    ,output [ 31:0]  outport_id_o
    ,output          outport_eob_o
);



//-----------------------------------------------------------------
// Block Type (Y, Cb, Cr)
//-----------------------------------------------------------------
wire start_block_w;
wire next_block_w;
wire end_of_image_w;

localparam JPEG_MONOCHROME  = 2'd0;
localparam JPEG_YCBCR_444   = 2'd1;
localparam JPEG_YCBCR_420   = 2'd2;
localparam JPEG_UNSUPPORTED = 2'd3;

localparam BLOCK_Y          = 2'd0;
localparam BLOCK_CB         = 2'd1;
localparam BLOCK_CR         = 2'd2;
localparam BLOCK_EOF        = 2'd3;

wire [1:0] block_type_w;

localparam DHT_TABLE_Y_DC_IDX  = 2'd0;
localparam DHT_TABLE_Y_AC_IDX  = 2'd1;
localparam DHT_TABLE_CX_DC_IDX = 2'd2;
localparam DHT_TABLE_CX_AC_IDX = 2'd3;

jpeg_mcu_id
u_id
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    ,.img_start_i(img_start_i)
    ,.img_end_i(img_end_i)
    ,.img_width_i(img_width_i)
    ,.img_height_i(img_height_i)
    ,.img_mode_i(img_mode_i)

    ,.start_of_block_i(start_block_w)
    ,.end_of_block_i(next_block_w)

    ,.block_id_o(outport_id_o)
    ,.block_type_o(block_type_w)
    ,.end_of_image_o(end_of_image_w)
);

//-----------------------------------------------------------------
// FSM
//-----------------------------------------------------------------
localparam STATE_W           = 5;
localparam STATE_IDLE        = 5'd0;
localparam STATE_FETCH_WORD  = 5'd1;
localparam STATE_HUFF_LOOKUP = 5'd2;
localparam STATE_OUTPUT      = 5'd3;
localparam STATE_EOB         = 5'd4;
localparam STATE_EOF         = 5'd5;

reg [STATE_W-1:0] state_q;
reg [STATE_W-1:0] next_state_r;

reg [7:0]         code_bits_q;
reg [7:0]         coeff_idx_q;

always @ *
begin
    next_state_r = state_q;

    case (state_q)
    STATE_IDLE:
    begin
        if (end_of_image_w && outport_blk_space_i)
            next_state_r = STATE_EOF;
        else if (inport_valid_i && outport_blk_space_i)
            next_state_r = STATE_FETCH_WORD;
    end
    STATE_FETCH_WORD:
    begin
        if (coeff_idx_q >= 8'd63)
            next_state_r = STATE_EOB;
        else if (inport_valid_i)
            next_state_r = STATE_HUFF_LOOKUP;
    end
    STATE_HUFF_LOOKUP:
    begin
        if (lookup_valid_i)
            next_state_r = STATE_OUTPUT;
    end
    STATE_OUTPUT:
    begin
        next_state_r = STATE_FETCH_WORD;
    end
    STATE_EOB:
    begin
        next_state_r = STATE_IDLE;
    end
    STATE_EOF:
    begin
        if (!img_end_i)
            next_state_r = STATE_IDLE;
    end
    default : ;
    endcase

    if (img_start_i)
        next_state_r = STATE_IDLE;
end

assign start_block_w = (state_q == STATE_IDLE && next_state_r != STATE_IDLE);
assign next_block_w  = (state_q == STATE_EOB);

always @ (posedge clk_i )
if (rst_i)
    state_q <= STATE_IDLE;
else
    state_q <= next_state_r;

reg first_q;

always @ (posedge clk_i )
if (rst_i)
    first_q <= 1'b1;
else if (state_q == STATE_IDLE)
    first_q <= 1'b1;
else if (state_q == STATE_OUTPUT)
    first_q <= 1'b0;

//-----------------------------------------------------------------
// Huffman code lookup stash
//-----------------------------------------------------------------
reg [7:0] code_q;

always @ (posedge clk_i )
if (rst_i)
    code_q <= 8'b0;
else if (state_q == STATE_HUFF_LOOKUP && lookup_valid_i)
    code_q <= lookup_value_i;

//-----------------------------------------------------------------
// code[3:0] = width of symbol
//-----------------------------------------------------------------
always @ (posedge clk_i )
if (rst_i)
    code_bits_q <= 8'b0;
else if (state_q == STATE_HUFF_LOOKUP && lookup_valid_i)
    code_bits_q <= {4'b0, lookup_value_i[3:0]};

//-----------------------------------------------------------------
// Lookup width flops
//-----------------------------------------------------------------
reg [4:0] lookup_width_q;

always @ (posedge clk_i )
if (rst_i)
    lookup_width_q <= 5'b0;
else if (state_q == STATE_HUFF_LOOKUP && lookup_valid_i)
    lookup_width_q <= lookup_width_i;

//-----------------------------------------------------------------
// Data for coefficient (remainder from Huffman lookup)
//-----------------------------------------------------------------
reg [15:0] input_data_q;

wire [31:0] input_shift_w = inport_data_i >> (5'd16 - lookup_width_i);

always @ (posedge clk_i )
if (rst_i)
    input_data_q <= 16'b0;
// Use remaining data for actual coeffecient
else if (state_q == STATE_HUFF_LOOKUP && lookup_valid_i)
    input_data_q <= input_shift_w[15:0];

//-----------------------------------------------------------------
// Bit buffer pop
//-----------------------------------------------------------------
reg [5:0]  pop_bits_r;

wire [4:0] coef_bits_w = {1'b0, code_q[3:0]};

always @ *
begin
    pop_bits_r = 6'b0;

    case (state_q)
    STATE_OUTPUT:
    begin
        // DC coefficient
        if (coeff_idx_q == 8'd0)
            pop_bits_r = {1'b0, lookup_width_q} + coef_bits_w;
        // End of block or ZRL (no coefficient)
        else if (code_q == 8'b0 || code_q == 8'hF0)
            pop_bits_r = {1'b0, lookup_width_q};
        else
            pop_bits_r = {1'b0, lookup_width_q} + coef_bits_w;
    end
    default : ;
    endcase
end

assign lookup_req_o   = (state_q == STATE_FETCH_WORD) & inport_valid_i;
assign lookup_input_o = inport_data_i[31:16];
assign inport_pop_o   = pop_bits_r;

reg [1:0] lookup_table_r;
always @ *
begin
    lookup_table_r = DHT_TABLE_Y_DC_IDX;

    if (first_q) // (coeff_idx_q == 8'd0)
    begin
        if (block_type_w == BLOCK_Y)
            lookup_table_r = DHT_TABLE_Y_DC_IDX;
        else
            lookup_table_r = DHT_TABLE_CX_DC_IDX;
    end
    else
    begin
        if (block_type_w == BLOCK_Y)
            lookup_table_r = DHT_TABLE_Y_AC_IDX;
        else
            lookup_table_r = DHT_TABLE_CX_AC_IDX;
    end
end
assign lookup_table_o = lookup_table_r;

//-----------------------------------------------------------------------------
// decode_number: Extract number from code / width
//-----------------------------------------------------------------------------
function [15:0] decode_number;
    input [15:0] w;
    input [4:0]  bits;
    reg signed [15:0] code;
begin
    code = w;

    if ((code & (1<<(bits - 5'd1))) == 16'b0 && bits != 5'd0)
    begin
        code = (code | ((~0) << bits)) + 1;
    end
    decode_number = code;
end
endfunction

//-----------------------------------------------------------------
// Previous DC coeffecient
//-----------------------------------------------------------------
wire [1:0] comp_idx_w = block_type_w;

reg [15:0] prev_dc_coeff_q[3:0];
reg [15:0] dc_coeff_q;

always @ (posedge clk_i )
if (rst_i)
begin
    prev_dc_coeff_q[0] <= 16'b0;
    prev_dc_coeff_q[1] <= 16'b0;
    prev_dc_coeff_q[2] <= 16'b0;
    prev_dc_coeff_q[3] <= 16'b0; // X
end
else if (img_start_i)
begin
    prev_dc_coeff_q[0] <= 16'b0;
    prev_dc_coeff_q[1] <= 16'b0;
    prev_dc_coeff_q[2] <= 16'b0;
    prev_dc_coeff_q[3] <= 16'b0; // X
end
else if (state_q == STATE_EOB)
    prev_dc_coeff_q[comp_idx_w] <= dc_coeff_q;

//-----------------------------------------------------------------
// coeff
//-----------------------------------------------------------------
reg [15:0] coeff_r;

always @ *
begin
    if (coeff_idx_q == 8'b0)
        coeff_r = decode_number(input_data_q >> (16 - coef_bits_w), coef_bits_w) + prev_dc_coeff_q[comp_idx_w];
    else
        coeff_r = decode_number(input_data_q >> (16 - coef_bits_w), coef_bits_w);
end

//-----------------------------------------------------------------
// dc_coeff
//-----------------------------------------------------------------
always @ (posedge clk_i )
if (rst_i)
    dc_coeff_q <= 16'b0;
else if (state_q == STATE_OUTPUT && coeff_idx_q == 8'b0)
    dc_coeff_q <= coeff_r;

//-----------------------------------------------------------------
// DC / AC coeff
//-----------------------------------------------------------------
reg [15:0] coeff_q;

always @ (posedge clk_i )
if (rst_i)
    coeff_q <= 16'b0;
else if (state_q == STATE_OUTPUT)
    coeff_q <= coeff_r;

//-----------------------------------------------------------------
// Coeffecient index
//-----------------------------------------------------------------
always @ (posedge clk_i )
if (rst_i)
    coeff_idx_q <= 8'b0;
else if (state_q == STATE_EOB || img_start_i)
    coeff_idx_q <= 8'b0;
else if (state_q == STATE_FETCH_WORD && !first_q && inport_valid_i)
    coeff_idx_q <= coeff_idx_q + 8'd1;
else if (state_q == STATE_OUTPUT)
begin
    // DC
    if (coeff_idx_q == 8'b0)
        ;
    // AC
    else
    begin
        // End of block
        if (code_q == 8'b0)
            coeff_idx_q <= 8'd64;
        // ZRL - 16 zeros
        else if (code_q == 8'hF0)
            coeff_idx_q <= coeff_idx_q + 8'd15;
        // RLE number zeros (0 - 15)
        else
            coeff_idx_q <= coeff_idx_q + {4'b0, code_q[7:4]};
    end
end

//-----------------------------------------------------------------
// Output push
//-----------------------------------------------------------------
reg push_q;

always @ (posedge clk_i )
if (rst_i)
    push_q <= 1'b0;
else if (state_q == STATE_OUTPUT || state_q == STATE_EOF)
    push_q <= 1'b1;
else
    push_q <= 1'b0;

assign outport_valid_o = push_q && (coeff_idx_q < 8'd64);
assign outport_data_o  = coeff_q;
assign outport_idx_o   = coeff_idx_q[5:0];
assign outport_eob_o   = (state_q == STATE_EOB) || 
                         (state_q == STATE_EOF && push_q);

`ifdef verilator
function get_valid; /*verilator public*/
begin
    get_valid = outport_valid_o && block_type_w != BLOCK_EOF;
end
endfunction
function [5:0] get_sample_idx; /*verilator public*/
begin
    get_sample_idx = outport_idx_o;
end
endfunction
function [15:0] get_sample; /*verilator public*/
begin
    get_sample = outport_data_o;
end
endfunction

function [5:0] get_bitbuffer_pop; /*verilator public*/
begin
    get_bitbuffer_pop = inport_pop_o;
end
endfunction

function get_dht_valid; /*verilator public*/
begin
    get_dht_valid = lookup_valid_i && (state_q == STATE_HUFF_LOOKUP);
end
endfunction
function [4:0] get_dht_width; /*verilator public*/
begin
    get_dht_width = lookup_width_i;
end
endfunction
function [7:0] get_dht_value; /*verilator public*/
begin
    get_dht_value = lookup_value_i;
end
endfunction
`endif


endmodule
//-----------------------------------------------------------------
//                      Baseline JPEG Decoder
//                             V0.1
//                       Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//-----------------------------------------------------------------
//                      License: Apache 2.0
// This IP can be freely used in commercial projects, however you may
// want access to unreleased materials such as verification environments,
// or test vectors, as well as changes to the IP for integration purposes.
// If this is the case, contact the above address.
// I am interested to hear how and where this IP is used, so please get
// in touch!
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

module jpeg_output_cx_ram
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input  [  5:0]  wr_idx_i
    ,input  [ 31:0]  data_in_i
    ,input           push_i
    ,input           mode420_i
    ,input           pop_i
    ,input           flush_i

    // Outputs
    ,output [ 31:0]  data_out_o
    ,output          valid_o
    ,output [ 31:0]  level_o
);



//-----------------------------------------------------------------
// Registers
//-----------------------------------------------------------------
reg [7:0]   rd_ptr_q;
reg [7:0]   wr_ptr_q;

//-----------------------------------------------------------------
// Write Side
//-----------------------------------------------------------------
wire [7:0] write_next_w = wr_ptr_q + 8'd1;

always @ (posedge clk_i )
if (rst_i)
    wr_ptr_q <= 8'b0;
else if (flush_i)
    wr_ptr_q <= 8'b0;
// Push
else if (push_i)
    wr_ptr_q <= write_next_w;

//-----------------------------------------------------------------
// Read Side
//-----------------------------------------------------------------
wire read_ok_w = (level_o > 32'd1);
reg  rd_q;

always @ (posedge clk_i )
if (rst_i)
    rd_q <= 1'b0;
else if (flush_i)
    rd_q <= 1'b0;
else
    rd_q <= read_ok_w;

wire [7:0] rd_ptr_next_w = rd_ptr_q + 8'd1;

always @ (posedge clk_i )
if (rst_i)
    rd_ptr_q <= 8'b0;
else if (flush_i)
    rd_ptr_q <= 8'b0;
else if (read_ok_w && ((!valid_o) || (valid_o && pop_i)))
    rd_ptr_q <= rd_ptr_next_w;

//-------------------------------------------------------------------
// Chroma subsampling (420) sample addressing
//-------------------------------------------------------------------
reg [7:0] cx_idx_q;

reg [5:0] cx_rd_ptr_r;

always @ *
begin
    case (cx_idx_q)
    8'd0: cx_rd_ptr_r = 6'd0;
    8'd1: cx_rd_ptr_r = 6'd1;
    8'd2: cx_rd_ptr_r = 6'd1;
    8'd3: cx_rd_ptr_r = 6'd2;
    8'd4: cx_rd_ptr_r = 6'd2;
    8'd5: cx_rd_ptr_r = 6'd3;
    8'd6: cx_rd_ptr_r = 6'd3;
    8'd7: cx_rd_ptr_r = 6'd0;
    8'd8: cx_rd_ptr_r = 6'd0;
    8'd9: cx_rd_ptr_r = 6'd1;
    8'd10: cx_rd_ptr_r = 6'd1;
    8'd11: cx_rd_ptr_r = 6'd2;
    8'd12: cx_rd_ptr_r = 6'd2;
    8'd13: cx_rd_ptr_r = 6'd3;
    8'd14: cx_rd_ptr_r = 6'd3;
    8'd15: cx_rd_ptr_r = 6'd8;
    8'd16: cx_rd_ptr_r = 6'd8;
    8'd17: cx_rd_ptr_r = 6'd9;
    8'd18: cx_rd_ptr_r = 6'd9;
    8'd19: cx_rd_ptr_r = 6'd10;
    8'd20: cx_rd_ptr_r = 6'd10;
    8'd21: cx_rd_ptr_r = 6'd11;
    8'd22: cx_rd_ptr_r = 6'd11;
    8'd23: cx_rd_ptr_r = 6'd8;
    8'd24: cx_rd_ptr_r = 6'd8;
    8'd25: cx_rd_ptr_r = 6'd9;
    8'd26: cx_rd_ptr_r = 6'd9;
    8'd27: cx_rd_ptr_r = 6'd10;
    8'd28: cx_rd_ptr_r = 6'd10;
    8'd29: cx_rd_ptr_r = 6'd11;
    8'd30: cx_rd_ptr_r = 6'd11;
    8'd31: cx_rd_ptr_r = 6'd16;
    8'd32: cx_rd_ptr_r = 6'd16;
    8'd33: cx_rd_ptr_r = 6'd17;
    8'd34: cx_rd_ptr_r = 6'd17;
    8'd35: cx_rd_ptr_r = 6'd18;
    8'd36: cx_rd_ptr_r = 6'd18;
    8'd37: cx_rd_ptr_r = 6'd19;
    8'd38: cx_rd_ptr_r = 6'd19;
    8'd39: cx_rd_ptr_r = 6'd16;
    8'd40: cx_rd_ptr_r = 6'd16;
    8'd41: cx_rd_ptr_r = 6'd17;
    8'd42: cx_rd_ptr_r = 6'd17;
    8'd43: cx_rd_ptr_r = 6'd18;
    8'd44: cx_rd_ptr_r = 6'd18;
    8'd45: cx_rd_ptr_r = 6'd19;
    8'd46: cx_rd_ptr_r = 6'd19;
    8'd47: cx_rd_ptr_r = 6'd24;
    8'd48: cx_rd_ptr_r = 6'd24;
    8'd49: cx_rd_ptr_r = 6'd25;
    8'd50: cx_rd_ptr_r = 6'd25;
    8'd51: cx_rd_ptr_r = 6'd26;
    8'd52: cx_rd_ptr_r = 6'd26;
    8'd53: cx_rd_ptr_r = 6'd27;
    8'd54: cx_rd_ptr_r = 6'd27;
    8'd55: cx_rd_ptr_r = 6'd24;
    8'd56: cx_rd_ptr_r = 6'd24;
    8'd57: cx_rd_ptr_r = 6'd25;
    8'd58: cx_rd_ptr_r = 6'd25;
    8'd59: cx_rd_ptr_r = 6'd26;
    8'd60: cx_rd_ptr_r = 6'd26;
    8'd61: cx_rd_ptr_r = 6'd27;
    8'd62: cx_rd_ptr_r = 6'd27;
    8'd63: cx_rd_ptr_r = 6'd4;
    8'd64: cx_rd_ptr_r = 6'd4;
    8'd65: cx_rd_ptr_r = 6'd5;
    8'd66: cx_rd_ptr_r = 6'd5;
    8'd67: cx_rd_ptr_r = 6'd6;
    8'd68: cx_rd_ptr_r = 6'd6;
    8'd69: cx_rd_ptr_r = 6'd7;
    8'd70: cx_rd_ptr_r = 6'd7;
    8'd71: cx_rd_ptr_r = 6'd4;
    8'd72: cx_rd_ptr_r = 6'd4;
    8'd73: cx_rd_ptr_r = 6'd5;
    8'd74: cx_rd_ptr_r = 6'd5;
    8'd75: cx_rd_ptr_r = 6'd6;
    8'd76: cx_rd_ptr_r = 6'd6;
    8'd77: cx_rd_ptr_r = 6'd7;
    8'd78: cx_rd_ptr_r = 6'd7;
    8'd79: cx_rd_ptr_r = 6'd12;
    8'd80: cx_rd_ptr_r = 6'd12;
    8'd81: cx_rd_ptr_r = 6'd13;
    8'd82: cx_rd_ptr_r = 6'd13;
    8'd83: cx_rd_ptr_r = 6'd14;
    8'd84: cx_rd_ptr_r = 6'd14;
    8'd85: cx_rd_ptr_r = 6'd15;
    8'd86: cx_rd_ptr_r = 6'd15;
    8'd87: cx_rd_ptr_r = 6'd12;
    8'd88: cx_rd_ptr_r = 6'd12;
    8'd89: cx_rd_ptr_r = 6'd13;
    8'd90: cx_rd_ptr_r = 6'd13;
    8'd91: cx_rd_ptr_r = 6'd14;
    8'd92: cx_rd_ptr_r = 6'd14;
    8'd93: cx_rd_ptr_r = 6'd15;
    8'd94: cx_rd_ptr_r = 6'd15;
    8'd95: cx_rd_ptr_r = 6'd20;
    8'd96: cx_rd_ptr_r = 6'd20;
    8'd97: cx_rd_ptr_r = 6'd21;
    8'd98: cx_rd_ptr_r = 6'd21;
    8'd99: cx_rd_ptr_r = 6'd22;
    8'd100: cx_rd_ptr_r = 6'd22;
    8'd101: cx_rd_ptr_r = 6'd23;
    8'd102: cx_rd_ptr_r = 6'd23;
    8'd103: cx_rd_ptr_r = 6'd20;
    8'd104: cx_rd_ptr_r = 6'd20;
    8'd105: cx_rd_ptr_r = 6'd21;
    8'd106: cx_rd_ptr_r = 6'd21;
    8'd107: cx_rd_ptr_r = 6'd22;
    8'd108: cx_rd_ptr_r = 6'd22;
    8'd109: cx_rd_ptr_r = 6'd23;
    8'd110: cx_rd_ptr_r = 6'd23;
    8'd111: cx_rd_ptr_r = 6'd28;
    8'd112: cx_rd_ptr_r = 6'd28;
    8'd113: cx_rd_ptr_r = 6'd29;
    8'd114: cx_rd_ptr_r = 6'd29;
    8'd115: cx_rd_ptr_r = 6'd30;
    8'd116: cx_rd_ptr_r = 6'd30;
    8'd117: cx_rd_ptr_r = 6'd31;
    8'd118: cx_rd_ptr_r = 6'd31;
    8'd119: cx_rd_ptr_r = 6'd28;
    8'd120: cx_rd_ptr_r = 6'd28;
    8'd121: cx_rd_ptr_r = 6'd29;
    8'd122: cx_rd_ptr_r = 6'd29;
    8'd123: cx_rd_ptr_r = 6'd30;
    8'd124: cx_rd_ptr_r = 6'd30;
    8'd125: cx_rd_ptr_r = 6'd31;
    8'd126: cx_rd_ptr_r = 6'd31;
    8'd127: cx_rd_ptr_r = 6'd32;
    8'd128: cx_rd_ptr_r = 6'd32;
    8'd129: cx_rd_ptr_r = 6'd33;
    8'd130: cx_rd_ptr_r = 6'd33;
    8'd131: cx_rd_ptr_r = 6'd34;
    8'd132: cx_rd_ptr_r = 6'd34;
    8'd133: cx_rd_ptr_r = 6'd35;
    8'd134: cx_rd_ptr_r = 6'd35;
    8'd135: cx_rd_ptr_r = 6'd32;
    8'd136: cx_rd_ptr_r = 6'd32;
    8'd137: cx_rd_ptr_r = 6'd33;
    8'd138: cx_rd_ptr_r = 6'd33;
    8'd139: cx_rd_ptr_r = 6'd34;
    8'd140: cx_rd_ptr_r = 6'd34;
    8'd141: cx_rd_ptr_r = 6'd35;
    8'd142: cx_rd_ptr_r = 6'd35;
    8'd143: cx_rd_ptr_r = 6'd40;
    8'd144: cx_rd_ptr_r = 6'd40;
    8'd145: cx_rd_ptr_r = 6'd41;
    8'd146: cx_rd_ptr_r = 6'd41;
    8'd147: cx_rd_ptr_r = 6'd42;
    8'd148: cx_rd_ptr_r = 6'd42;
    8'd149: cx_rd_ptr_r = 6'd43;
    8'd150: cx_rd_ptr_r = 6'd43;
    8'd151: cx_rd_ptr_r = 6'd40;
    8'd152: cx_rd_ptr_r = 6'd40;
    8'd153: cx_rd_ptr_r = 6'd41;
    8'd154: cx_rd_ptr_r = 6'd41;
    8'd155: cx_rd_ptr_r = 6'd42;
    8'd156: cx_rd_ptr_r = 6'd42;
    8'd157: cx_rd_ptr_r = 6'd43;
    8'd158: cx_rd_ptr_r = 6'd43;
    8'd159: cx_rd_ptr_r = 6'd48;
    8'd160: cx_rd_ptr_r = 6'd48;
    8'd161: cx_rd_ptr_r = 6'd49;
    8'd162: cx_rd_ptr_r = 6'd49;
    8'd163: cx_rd_ptr_r = 6'd50;
    8'd164: cx_rd_ptr_r = 6'd50;
    8'd165: cx_rd_ptr_r = 6'd51;
    8'd166: cx_rd_ptr_r = 6'd51;
    8'd167: cx_rd_ptr_r = 6'd48;
    8'd168: cx_rd_ptr_r = 6'd48;
    8'd169: cx_rd_ptr_r = 6'd49;
    8'd170: cx_rd_ptr_r = 6'd49;
    8'd171: cx_rd_ptr_r = 6'd50;
    8'd172: cx_rd_ptr_r = 6'd50;
    8'd173: cx_rd_ptr_r = 6'd51;
    8'd174: cx_rd_ptr_r = 6'd51;
    8'd175: cx_rd_ptr_r = 6'd56;
    8'd176: cx_rd_ptr_r = 6'd56;
    8'd177: cx_rd_ptr_r = 6'd57;
    8'd178: cx_rd_ptr_r = 6'd57;
    8'd179: cx_rd_ptr_r = 6'd58;
    8'd180: cx_rd_ptr_r = 6'd58;
    8'd181: cx_rd_ptr_r = 6'd59;
    8'd182: cx_rd_ptr_r = 6'd59;
    8'd183: cx_rd_ptr_r = 6'd56;
    8'd184: cx_rd_ptr_r = 6'd56;
    8'd185: cx_rd_ptr_r = 6'd57;
    8'd186: cx_rd_ptr_r = 6'd57;
    8'd187: cx_rd_ptr_r = 6'd58;
    8'd188: cx_rd_ptr_r = 6'd58;
    8'd189: cx_rd_ptr_r = 6'd59;
    8'd190: cx_rd_ptr_r = 6'd59;
    8'd191: cx_rd_ptr_r = 6'd36;
    8'd192: cx_rd_ptr_r = 6'd36;
    8'd193: cx_rd_ptr_r = 6'd37;
    8'd194: cx_rd_ptr_r = 6'd37;
    8'd195: cx_rd_ptr_r = 6'd38;
    8'd196: cx_rd_ptr_r = 6'd38;
    8'd197: cx_rd_ptr_r = 6'd39;
    8'd198: cx_rd_ptr_r = 6'd39;
    8'd199: cx_rd_ptr_r = 6'd36;
    8'd200: cx_rd_ptr_r = 6'd36;
    8'd201: cx_rd_ptr_r = 6'd37;
    8'd202: cx_rd_ptr_r = 6'd37;
    8'd203: cx_rd_ptr_r = 6'd38;
    8'd204: cx_rd_ptr_r = 6'd38;
    8'd205: cx_rd_ptr_r = 6'd39;
    8'd206: cx_rd_ptr_r = 6'd39;
    8'd207: cx_rd_ptr_r = 6'd44;
    8'd208: cx_rd_ptr_r = 6'd44;
    8'd209: cx_rd_ptr_r = 6'd45;
    8'd210: cx_rd_ptr_r = 6'd45;
    8'd211: cx_rd_ptr_r = 6'd46;
    8'd212: cx_rd_ptr_r = 6'd46;
    8'd213: cx_rd_ptr_r = 6'd47;
    8'd214: cx_rd_ptr_r = 6'd47;
    8'd215: cx_rd_ptr_r = 6'd44;
    8'd216: cx_rd_ptr_r = 6'd44;
    8'd217: cx_rd_ptr_r = 6'd45;
    8'd218: cx_rd_ptr_r = 6'd45;
    8'd219: cx_rd_ptr_r = 6'd46;
    8'd220: cx_rd_ptr_r = 6'd46;
    8'd221: cx_rd_ptr_r = 6'd47;
    8'd222: cx_rd_ptr_r = 6'd47;
    8'd223: cx_rd_ptr_r = 6'd52;
    8'd224: cx_rd_ptr_r = 6'd52;
    8'd225: cx_rd_ptr_r = 6'd53;
    8'd226: cx_rd_ptr_r = 6'd53;
    8'd227: cx_rd_ptr_r = 6'd54;
    8'd228: cx_rd_ptr_r = 6'd54;
    8'd229: cx_rd_ptr_r = 6'd55;
    8'd230: cx_rd_ptr_r = 6'd55;
    8'd231: cx_rd_ptr_r = 6'd52;
    8'd232: cx_rd_ptr_r = 6'd52;
    8'd233: cx_rd_ptr_r = 6'd53;
    8'd234: cx_rd_ptr_r = 6'd53;
    8'd235: cx_rd_ptr_r = 6'd54;
    8'd236: cx_rd_ptr_r = 6'd54;
    8'd237: cx_rd_ptr_r = 6'd55;
    8'd238: cx_rd_ptr_r = 6'd55;
    8'd239: cx_rd_ptr_r = 6'd60;
    8'd240: cx_rd_ptr_r = 6'd60;
    8'd241: cx_rd_ptr_r = 6'd61;
    8'd242: cx_rd_ptr_r = 6'd61;
    8'd243: cx_rd_ptr_r = 6'd62;
    8'd244: cx_rd_ptr_r = 6'd62;
    8'd245: cx_rd_ptr_r = 6'd63;
    8'd246: cx_rd_ptr_r = 6'd63;
    8'd247: cx_rd_ptr_r = 6'd60;
    8'd248: cx_rd_ptr_r = 6'd60;
    8'd249: cx_rd_ptr_r = 6'd61;
    8'd250: cx_rd_ptr_r = 6'd61;
    8'd251: cx_rd_ptr_r = 6'd62;
    8'd252: cx_rd_ptr_r = 6'd62;
    8'd253: cx_rd_ptr_r = 6'd63;
    8'd254: cx_rd_ptr_r = 6'd63;
    default: cx_rd_ptr_r = 6'd0;
    endcase
end

always @ (posedge clk_i )
if (rst_i)
    cx_idx_q    <= 8'b0;
else if (flush_i)
    cx_idx_q    <= 8'b0;
else if (read_ok_w && ((!valid_o) || (valid_o && pop_i)))
    cx_idx_q    <= cx_idx_q + 8'd1;

reg [1:0] cx_half_q;

always @ (posedge clk_i )
if (rst_i)
    cx_half_q    <= 2'b0;
else if (flush_i)
    cx_half_q    <= 2'b0;
else if (read_ok_w && ((!valid_o) || (valid_o && pop_i)) && cx_idx_q == 8'd255)
    cx_half_q    <= cx_half_q + 2'd1;

reg [5:0] cx_rd_ptr_q;

always @ (posedge clk_i )
if (rst_i)
    cx_rd_ptr_q <= 6'b0;
else if (read_ok_w && ((!valid_o) || (valid_o && pop_i)))
    cx_rd_ptr_q <= cx_rd_ptr_r;

wire [7:0] rd_addr_w = mode420_i ? {cx_half_q, cx_rd_ptr_q} : rd_ptr_q;

//-------------------------------------------------------------------
// Read Skid Buffer
//-------------------------------------------------------------------
reg                rd_skid_q;
reg [31:0] rd_skid_data_q;

always @ (posedge clk_i )
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

//-------------------------------------------------------------------
// Dual port RAM
//-------------------------------------------------------------------
wire [31:0] data_out_w;

jpeg_output_cx_ram_ram_dp_256_8
u_ram
(
    // Inputs
    .clk0_i(clk_i),
    .rst0_i(rst_i),
    .clk1_i(clk_i),
    .rst1_i(rst_i),

    // Write side
    .addr0_i({wr_ptr_q[7:6], wr_idx_i}),
    .wr0_i(push_i),
    .data0_i(data_in_i),
    .data0_o(),

    // Read side
    .addr1_i(rd_addr_w),
    .data1_i(32'b0),
    .wr1_i(1'b0),
    .data1_o(data_out_w)
);

assign data_out_o = rd_skid_q ? rd_skid_data_q : data_out_w;


//-------------------------------------------------------------------
// Level
//-------------------------------------------------------------------
reg [31:0]  count_q;
reg [31:0]  count_r;

always @ *
begin
    count_r = count_q;

    if (pop_i && valid_o)
        count_r = count_r - 32'd1;

    if (push_i)
        count_r = count_r + (mode420_i ? 32'd4 : 32'd1);
end

always @ (posedge clk_i )
if (rst_i)
    count_q   <= 32'b0;
else if (flush_i)
    count_q   <= 32'b0;
else
    count_q <= count_r;

assign level_o = count_q;

endmodule

//-------------------------------------------------------------------
// Dual port RAM
//-------------------------------------------------------------------
module jpeg_output_cx_ram_ram_dp_256_8
(
    // Inputs
     input           clk0_i
    ,input           rst0_i
    ,input  [ 7:0]  addr0_i
    ,input  [ 31:0]  data0_i
    ,input           wr0_i
    ,input           clk1_i
    ,input           rst1_i
    ,input  [ 7:0]  addr1_i
    ,input  [ 31:0]  data1_i
    ,input           wr1_i

    // Outputs
    ,output [ 31:0]  data0_o
    ,output [ 31:0]  data1_o
);

/* verilator lint_off MULTIDRIVEN */
reg [31:0]   ram [255:0] /*verilator public*/;
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
//                      Baseline JPEG Decoder
//                             V0.1
//                       Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//-----------------------------------------------------------------
//                      License: Apache 2.0
// This IP can be freely used in commercial projects, however you may
// want access to unreleased materials such as verification environments,
// or test vectors, as well as changes to the IP for integration purposes.
// If this is the case, contact the above address.
// I am interested to hear how and where this IP is used, so please get
// in touch!
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

module jpeg_output_fifo
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
     parameter WIDTH            = 8
    ,parameter DEPTH            = 4
    ,parameter ADDR_W           = 2
)
//-----------------------------------------------------------------
// Ports
//-----------------------------------------------------------------
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input  [WIDTH-1:0]  data_in_i
    ,input           push_i
    ,input           pop_i
    ,input           flush_i

    // Outputs
    ,output [WIDTH-1:0]  data_out_o
    ,output          accept_o
    ,output          valid_o
);



//-----------------------------------------------------------------
// Local Params
//-----------------------------------------------------------------
localparam COUNT_W = ADDR_W + 1;

//-----------------------------------------------------------------
// Registers
//-----------------------------------------------------------------
reg [WIDTH-1:0]   ram_q[DEPTH-1:0];
reg [ADDR_W-1:0]  rd_ptr_q;
reg [ADDR_W-1:0]  wr_ptr_q;
reg [COUNT_W-1:0] count_q;

//-----------------------------------------------------------------
// Sequential
//-----------------------------------------------------------------
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
begin
    count_q   <= {(COUNT_W) {1'b0}};
    rd_ptr_q  <= {(ADDR_W) {1'b0}};
    wr_ptr_q  <= {(ADDR_W) {1'b0}};
end
else if (flush_i)
begin
    count_q   <= {(COUNT_W) {1'b0}};
    rd_ptr_q  <= {(ADDR_W) {1'b0}};
    wr_ptr_q  <= {(ADDR_W) {1'b0}};
end
else
begin
    // Push
    if (push_i & accept_o)
    begin
        ram_q[wr_ptr_q] <= data_in_i;
        wr_ptr_q        <= wr_ptr_q + 1;
    end

    // Pop
    if (pop_i & valid_o)
        rd_ptr_q      <= rd_ptr_q + 1;

    // Count up
    if ((push_i & accept_o) & ~(pop_i & valid_o))
        count_q <= count_q + 1;
    // Count down
    else if (~(push_i & accept_o) & (pop_i & valid_o))
        count_q <= count_q - 1;
end

//-------------------------------------------------------------------
// Combinatorial
//-------------------------------------------------------------------
/* verilator lint_off WIDTH */
assign valid_o       = (count_q != 0);
assign accept_o      = (count_q != DEPTH);
/* verilator lint_on WIDTH */

assign data_out_o    = ram_q[rd_ptr_q];



endmodule
//-----------------------------------------------------------------
//                      Baseline JPEG Decoder
//                             V0.1
//                       Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//-----------------------------------------------------------------
//                      License: Apache 2.0
// This IP can be freely used in commercial projects, however you may
// want access to unreleased materials such as verification environments,
// or test vectors, as well as changes to the IP for integration purposes.
// If this is the case, contact the above address.
// I am interested to hear how and where this IP is used, so please get
// in touch!
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

module jpeg_output
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           img_start_i
    ,input           img_end_i
    ,input  [ 15:0]  img_width_i
    ,input  [ 15:0]  img_height_i
    ,input  [  1:0]  img_mode_i
    ,input           inport_valid_i
    ,input  [ 31:0]  inport_data_i
    ,input  [  5:0]  inport_idx_i
    ,input  [ 31:0]  inport_id_i
    ,input           outport_accept_i

    // Outputs
    ,output          inport_accept_o
    ,output          outport_valid_o
    ,output [ 15:0]  outport_width_o
    ,output [ 15:0]  outport_height_o
    ,output [ 15:0]  outport_pixel_x_o
    ,output [ 15:0]  outport_pixel_y_o
    ,output [  7:0]  outport_pixel_r_o
    ,output [  7:0]  outport_pixel_g_o
    ,output [  7:0]  outport_pixel_b_o
    ,output          idle_o
);



localparam BLOCK_Y          = 2'd0;
localparam BLOCK_CB         = 2'd1;
localparam BLOCK_CR         = 2'd2;
localparam BLOCK_EOF        = 2'd3;

localparam JPEG_MONOCHROME  = 2'd0;
localparam JPEG_YCBCR_444   = 2'd1;
localparam JPEG_YCBCR_420   = 2'd2;
localparam JPEG_UNSUPPORTED = 2'd3;

reg valid_r;
wire output_space_w = (!outport_valid_o || outport_accept_i);

//-----------------------------------------------------------------
// FIFO: Y
//-----------------------------------------------------------------
wire               y_valid_w;
wire signed [31:0] y_value_w;
wire               y_pop_w;
wire [31:0]        y_level_w;

jpeg_output_y_ram
u_ram_y
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    ,.flush_i(img_start_i)
    ,.level_o(y_level_w)

    ,.push_i(inport_valid_i && (inport_id_i[31:30] == BLOCK_Y || inport_id_i[31:30] == BLOCK_EOF))
    ,.wr_idx_i(inport_idx_i)
    ,.data_in_i(inport_data_i)

    ,.valid_o(y_valid_w)
    ,.data_out_o(y_value_w)
    ,.pop_i(y_pop_w)
);

//-----------------------------------------------------------------
// FIFO: Cb
//-----------------------------------------------------------------
wire               cb_valid_w;
wire signed [31:0] cb_value_w;
wire               cb_pop_w;
wire [31:0]        cb_level_w;

jpeg_output_cx_ram
u_ram_cb
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    ,.flush_i(img_start_i)
    ,.level_o(cb_level_w)
    ,.mode420_i(img_mode_i == JPEG_YCBCR_420)

    ,.push_i(inport_valid_i && (inport_id_i[31:30] == BLOCK_CB || inport_id_i[31:30] == BLOCK_EOF))
    ,.wr_idx_i(inport_idx_i)
    ,.data_in_i(inport_data_i)

    ,.valid_o(cb_valid_w)
    ,.data_out_o(cb_value_w)
    ,.pop_i(cb_pop_w)
);

//-----------------------------------------------------------------
// FIFO: Cr
//-----------------------------------------------------------------
wire               cr_valid_w;
wire signed [31:0] cr_value_w;
wire               cr_pop_w;
wire [31:0]        cr_level_w;

jpeg_output_cx_ram
u_ram_cr
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    ,.flush_i(img_start_i)
    ,.level_o(cr_level_w)
    ,.mode420_i(img_mode_i == JPEG_YCBCR_420)

    ,.push_i(inport_valid_i && (inport_id_i[31:30] == BLOCK_CR || inport_id_i[31:30] == BLOCK_EOF))
    ,.wr_idx_i(inport_idx_i)
    ,.data_in_i(inport_data_i)

    ,.valid_o(cr_valid_w)
    ,.data_out_o(cr_value_w)
    ,.pop_i(cr_pop_w)
);

//-----------------------------------------------------------------
// FIFO: Info
//-----------------------------------------------------------------
wire        id_valid_w;
wire [31:0] id_value_w;
wire        id_pop_w;

jpeg_output_fifo
#(
     .WIDTH(32)
    ,.DEPTH(8)
    ,.ADDR_W(3)
)
u_info
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.flush_i(img_start_i)
 
    ,.push_i(inport_valid_i && (inport_id_i[31:30] == BLOCK_Y || inport_id_i[31:30] == BLOCK_EOF) && inport_idx_i == 6'd0)
    ,.data_in_i(inport_id_i)
    ,.accept_o()

    ,.valid_o(id_valid_w)
    ,.data_out_o(id_value_w)
    ,.pop_i(id_pop_w)
);

assign inport_accept_o   = (y_level_w <= 32'd384 && cr_level_w <= 32'd128) | idle_o;

//-----------------------------------------------------------------
// Block counter (0 - 63)
//-----------------------------------------------------------------
reg [5:0] idx_q;

always @ (posedge clk_i )
if (rst_i)
    idx_q <= 6'b0;
else if (img_start_i)
    idx_q <= 6'b0;
else if (valid_r && output_space_w)
    idx_q <= idx_q + 6'd1;

//-----------------------------------------------------------------
// Subsampling counter (420 chroma subsampling)
//-----------------------------------------------------------------
reg [1:0] subsmpl_q;

always @ (posedge clk_i )
if (rst_i)
    subsmpl_q <= 2'b0;
else if (img_start_i)
    subsmpl_q <= 2'b0;
else if (valid_r && output_space_w && img_mode_i == JPEG_YCBCR_420 && idx_q == 6'd63)
    subsmpl_q <= subsmpl_q + 2'd1;

//-----------------------------------------------------------------
// YUV -> RGB
//-----------------------------------------------------------------
reg active_q;

always @ (posedge clk_i )
if (rst_i)
    active_q <= 1'b0;
else if (img_start_i)
    active_q <= 1'b0;
else if (!active_q)
begin
    if (img_mode_i == JPEG_MONOCHROME)
        active_q <= (y_level_w >= 32'd64);
    else if (img_mode_i == JPEG_YCBCR_444)
        active_q <= (y_level_w >= 32'd64) && (cb_level_w >= 32'd64) && (cr_level_w >= 32'd64);
    else if (subsmpl_q != 2'b0) // 420
        active_q <= 1'b1;
    else // 420
        active_q <= (y_level_w >= 32'd256) && (cb_level_w >= 32'd256) && (cr_level_w >= 32'd256);
end
else if (valid_r && output_space_w && idx_q == 6'd63)
    active_q <= 1'b0;

reg signed [31:0] r_conv_r;
reg signed [31:0] g_conv_r;
reg signed [31:0] b_conv_r;

wire signed [31:0] cr_1_402_w = (cr_value_w * 5743) >>> 12; // cr_value_w * 1.402
wire signed [31:0] cr_0_714_w = (cr_value_w * 2925) >>> 12; // cr_value_w * 0.71414
wire signed [31:0] cb_0_344_w = (cb_value_w * 1410) >>> 12; // cb_value_w * 0.34414
wire signed [31:0] cb_1_772_w = (cb_value_w * 7258) >>> 12; // cb_value_w * 1.772

always @ *
begin
    valid_r  = active_q;
    r_conv_r = 32'b0;
    g_conv_r = 32'b0;
    b_conv_r = 32'b0;

    if (img_mode_i == JPEG_MONOCHROME)
    begin
        r_conv_r = 128 + y_value_w;
        g_conv_r = 128 + y_value_w;
        b_conv_r = 128 + y_value_w;
    end
    else// if (img_mode_i == JPEG_YCBCR_444)
    begin
        r_conv_r = 128 + y_value_w + cr_1_402_w;
        g_conv_r = 128 + y_value_w - cb_0_344_w - cr_0_714_w;
        b_conv_r = 128 + y_value_w + cb_1_772_w;
    end
end

assign y_pop_w  = output_space_w && active_q;
assign cb_pop_w = output_space_w && active_q;
assign cr_pop_w = output_space_w && active_q;
assign id_pop_w = output_space_w && (idx_q == 6'd63);

//-----------------------------------------------------------------
// Outputs
//-----------------------------------------------------------------
reg        valid_q;
reg [15:0] pixel_x_q;
reg [15:0] pixel_y_q;
reg [7:0]  pixel_r_q;
reg [7:0]  pixel_g_q;
reg [7:0]  pixel_b_q;

always @ (posedge clk_i )
if (rst_i)
    valid_q <= 1'b0;
else if (output_space_w)
    valid_q <= valid_r && (id_value_w[31:30] != BLOCK_EOF);

wire [31:0] x_start_w = {13'b0, id_value_w[15:0],3'b0};
wire [31:0] y_start_w = {15'b0, id_value_w[29:16],3'b0};

always @ (posedge clk_i )
if (rst_i)
begin
    pixel_x_q <= 16'b0;
    pixel_y_q <= 16'b0;
end
else if (output_space_w)
begin
    /* verilator lint_off WIDTH */
    pixel_x_q <= x_start_w + (idx_q % 8);
    pixel_y_q <= y_start_w + (idx_q / 8);
    /* verilator lint_on WIDTH */
end

always @ (posedge clk_i )
if (rst_i)
begin
    pixel_r_q <= 8'b0;
    pixel_g_q <= 8'b0;
    pixel_b_q <= 8'b0;
end
else if (output_space_w)
begin
    pixel_r_q <= (|r_conv_r[31:8]) ? (r_conv_r[31:24] ^ 8'hff) : r_conv_r[7:0];
    pixel_g_q <= (|g_conv_r[31:8]) ? (g_conv_r[31:24] ^ 8'hff) : g_conv_r[7:0];
    pixel_b_q <= (|b_conv_r[31:8]) ? (b_conv_r[31:24] ^ 8'hff) : b_conv_r[7:0];
end

assign outport_valid_o   = valid_q;
assign outport_pixel_x_o = pixel_x_q;
assign outport_pixel_y_o = pixel_y_q;
assign outport_width_o   = img_width_i;
assign outport_height_o  = img_height_i;
assign outport_pixel_r_o = pixel_r_q;
assign outport_pixel_g_o = pixel_g_q;
assign outport_pixel_b_o = pixel_b_q;

//-----------------------------------------------------------------
// Idle
//-----------------------------------------------------------------
reg idle_q;

always @ (posedge clk_i )
if (rst_i)
    idle_q <= 1'b1;
else if (img_start_i)
    idle_q <= 1'b0;
else if (id_valid_w && id_value_w[31:30] == BLOCK_EOF)
    idle_q <= 1'b1;

assign idle_o = idle_q;


endmodule
//-----------------------------------------------------------------
//                      Baseline JPEG Decoder
//                             V0.1
//                       Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//-----------------------------------------------------------------
//                      License: Apache 2.0
// This IP can be freely used in commercial projects, however you may
// want access to unreleased materials such as verification environments,
// or test vectors, as well as changes to the IP for integration purposes.
// If this is the case, contact the above address.
// I am interested to hear how and where this IP is used, so please get
// in touch!
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

module jpeg_output_y_ram
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input  [  5:0]  wr_idx_i
    ,input  [ 31:0]  data_in_i
    ,input           push_i
    ,input           pop_i
    ,input           flush_i

    // Outputs
    ,output [ 31:0]  data_out_o
    ,output          valid_o
    ,output [ 31:0]  level_o
);



//-----------------------------------------------------------------
// Registers
//-----------------------------------------------------------------
reg [8:0]   rd_ptr_q;
reg [8:0]   wr_ptr_q;

//-----------------------------------------------------------------
// Write Side
//-----------------------------------------------------------------
wire [8:0] write_next_w = wr_ptr_q + 9'd1;

always @ (posedge clk_i )
if (rst_i)
    wr_ptr_q <= 9'b0;
else if (flush_i)
    wr_ptr_q <= 9'b0;
// Push
else if (push_i)
    wr_ptr_q <= write_next_w;

//-----------------------------------------------------------------
// Read Side
//-----------------------------------------------------------------
wire read_ok_w = (wr_ptr_q != rd_ptr_q);
reg  rd_q;

always @ (posedge clk_i )
if (rst_i)
    rd_q <= 1'b0;
else if (flush_i)
    rd_q <= 1'b0;
else
    rd_q <= read_ok_w;

wire [8:0] rd_ptr_next_w = rd_ptr_q + 9'd1;

always @ (posedge clk_i )
if (rst_i)
    rd_ptr_q <= 9'b0;
else if (flush_i)
    rd_ptr_q <= 9'b0;
else if (read_ok_w && ((!valid_o) || (valid_o && pop_i)))
    rd_ptr_q <= rd_ptr_next_w;

wire [8:0] rd_addr_w = rd_ptr_q;

//-------------------------------------------------------------------
// Read Skid Buffer
//-------------------------------------------------------------------
reg                rd_skid_q;
reg [31:0] rd_skid_data_q;

always @ (posedge clk_i )
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

//-------------------------------------------------------------------
// Dual port RAM
//-------------------------------------------------------------------
wire [31:0] data_out_w;

jpeg_output_y_ram_ram_dp_512_9
u_ram
(
    // Inputs
    .clk0_i(clk_i),
    .rst0_i(rst_i),
    .clk1_i(clk_i),
    .rst1_i(rst_i),

    // Write side
    .addr0_i({wr_ptr_q[8:6], wr_idx_i}),
    .wr0_i(push_i),
    .data0_i(data_in_i),
    .data0_o(),

    // Read side
    .addr1_i(rd_addr_w),
    .data1_i(32'b0),
    .wr1_i(1'b0),
    .data1_o(data_out_w)
);

assign data_out_o = rd_skid_q ? rd_skid_data_q : data_out_w;


//-------------------------------------------------------------------
// Level
//-------------------------------------------------------------------
reg [31:0]  count_q;
reg [31:0]  count_r;

always @ *
begin
    count_r = count_q;

    if (pop_i && valid_o)
        count_r = count_r - 32'd1;

    if (push_i)
        count_r = count_r + 32'd1;
end

always @ (posedge clk_i )
if (rst_i)
    count_q   <= 32'b0;
else if (flush_i)
    count_q   <= 32'b0;
else
    count_q <= count_r;

assign level_o = count_q;

endmodule

//-------------------------------------------------------------------
// Dual port RAM
//-------------------------------------------------------------------
module jpeg_output_y_ram_ram_dp_512_9
(
    // Inputs
     input           clk0_i
    ,input           rst0_i
    ,input  [ 8:0]  addr0_i
    ,input  [ 31:0]  data0_i
    ,input           wr0_i
    ,input           clk1_i
    ,input           rst1_i
    ,input  [ 8:0]  addr1_i
    ,input  [ 31:0]  data1_i
    ,input           wr1_i

    // Outputs
    ,output [ 31:0]  data0_o
    ,output [ 31:0]  data1_o
);

/* verilator lint_off MULTIDRIVEN */
reg [31:0]   ram [511:0] /*verilator public*/;
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
//                      Baseline JPEG Decoder
//                             V0.1
//                       Ultra-Embedded.com
//                        Copyright 2020
//
//                   admin@ultra-embedded.com
//-----------------------------------------------------------------
//                      License: Apache 2.0
// This IP can be freely used in commercial projects, however you may
// want access to unreleased materials such as verification environments,
// or test vectors, as well as changes to the IP for integration purposes.
// If this is the case, contact the above address.
// I am interested to hear how and where this IP is used, so please get
// in touch!
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

module jpeg_core
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
     parameter SUPPORT_WRITABLE_DHT = 0
)
//-----------------------------------------------------------------
// Ports
//-----------------------------------------------------------------
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           inport_valid_i
    ,input  [ 31:0]  inport_data_i
    ,input  [  3:0]  inport_strb_i
    ,input           inport_last_i
    ,input           outport_accept_i

    // Outputs
    ,output          inport_accept_o
    ,output          outport_valid_o
    ,output [ 15:0]  outport_width_o
    ,output [ 15:0]  outport_height_o
    ,output [ 15:0]  outport_pixel_x_o
    ,output [ 15:0]  outport_pixel_y_o
    ,output [  7:0]  outport_pixel_r_o
    ,output [  7:0]  outport_pixel_g_o
    ,output [  7:0]  outport_pixel_b_o
    ,output          idle_o
);

wire  [ 15:0]  idct_outport_data_w;
wire           dqt_inport_valid_w;
wire  [ 31:0]  dqt_inport_id_w;
wire  [ 31:0]  output_outport_data_w;
wire  [  5:0]  idct_inport_idx_w;
wire           dqt_inport_eob_w;
wire           img_start_w;
wire  [ 15:0]  img_height_w;
wire           output_inport_accept_w;
wire  [ 15:0]  img_width_w;
wire           dht_cfg_valid_w;
wire           lookup_req_w;
wire           lookup_valid_w;
wire  [  1:0]  img_dqt_table_cb_w;
wire  [  7:0]  dht_cfg_data_w;
wire           img_end_w;
wire  [ 31:0]  idct_inport_id_w;
wire  [  4:0]  lookup_width_w;
wire           idct_inport_accept_w;
wire  [  5:0]  output_inport_idx_w;
wire  [ 15:0]  dqt_outport_data_w;
wire           dqt_inport_blk_space_w;
wire           idct_inport_valid_w;
wire  [  7:0]  dqt_cfg_data_w;
wire  [  1:0]  img_dqt_table_y_w;
wire           idct_inport_eob_w;
wire           dht_cfg_accept_w;
wire  [ 31:0]  output_inport_id_w;
wire           output_inport_valid_w;
wire  [ 15:0]  lookup_input_w;
wire           dqt_cfg_accept_w;
wire  [  7:0]  lookup_value_w;
wire  [  1:0]  lookup_table_w;
wire           dht_cfg_last_w;
wire  [  5:0]  dqt_inport_idx_w;
wire  [  1:0]  img_mode_w;
wire  [  1:0]  img_dqt_table_cr_w;
wire           bb_inport_valid_w;
wire           bb_outport_last_w;
wire           bb_inport_last_w;
wire  [  5:0]  bb_outport_pop_w;
wire  [  7:0]  bb_inport_data_w;
wire           dqt_cfg_valid_w;
wire           bb_outport_valid_w;
wire           bb_inport_accept_w;
wire  [ 31:0]  bb_outport_data_w;
wire           dqt_cfg_last_w;


jpeg_input
u_jpeg_input
(
    // Inputs
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.inport_valid_i(inport_valid_i)
    ,.inport_data_i(inport_data_i)
    ,.inport_strb_i(inport_strb_i)
    ,.inport_last_i(inport_last_i)
    ,.dqt_cfg_accept_i(dqt_cfg_accept_w)
    ,.dht_cfg_accept_i(dht_cfg_accept_w)
    ,.data_accept_i(bb_inport_accept_w)

    // Outputs
    ,.inport_accept_o(inport_accept_o)
    ,.img_start_o(img_start_w)
    ,.img_end_o(img_end_w)
    ,.img_width_o(img_width_w)
    ,.img_height_o(img_height_w)
    ,.img_mode_o(img_mode_w)
    ,.img_dqt_table_y_o(img_dqt_table_y_w)
    ,.img_dqt_table_cb_o(img_dqt_table_cb_w)
    ,.img_dqt_table_cr_o(img_dqt_table_cr_w)
    ,.dqt_cfg_valid_o(dqt_cfg_valid_w)
    ,.dqt_cfg_data_o(dqt_cfg_data_w)
    ,.dqt_cfg_last_o(dqt_cfg_last_w)
    ,.dht_cfg_valid_o(dht_cfg_valid_w)
    ,.dht_cfg_data_o(dht_cfg_data_w)
    ,.dht_cfg_last_o(dht_cfg_last_w)
    ,.data_valid_o(bb_inport_valid_w)
    ,.data_data_o(bb_inport_data_w)
    ,.data_last_o(bb_inport_last_w)
);


jpeg_dht
#(
     .SUPPORT_WRITABLE_DHT(SUPPORT_WRITABLE_DHT)
)
u_jpeg_dht
(
    // Inputs
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.cfg_valid_i(dht_cfg_valid_w)
    ,.cfg_data_i(dht_cfg_data_w)
    ,.cfg_last_i(dht_cfg_last_w)
    ,.lookup_req_i(lookup_req_w)
    ,.lookup_table_i(lookup_table_w)
    ,.lookup_input_i(lookup_input_w)

    // Outputs
    ,.cfg_accept_o(dht_cfg_accept_w)
    ,.lookup_valid_o(lookup_valid_w)
    ,.lookup_width_o(lookup_width_w)
    ,.lookup_value_o(lookup_value_w)
);


jpeg_idct
u_jpeg_idct
(
    // Inputs
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.img_start_i(img_start_w)
    ,.img_end_i(img_end_w)
    ,.inport_valid_i(idct_inport_valid_w)
    ,.inport_data_i(idct_outport_data_w)
    ,.inport_idx_i(idct_inport_idx_w)
    ,.inport_eob_i(idct_inport_eob_w)
    ,.inport_id_i(idct_inport_id_w)
    ,.outport_accept_i(output_inport_accept_w)

    // Outputs
    ,.inport_accept_o(idct_inport_accept_w)
    ,.outport_valid_o(output_inport_valid_w)
    ,.outport_data_o(output_outport_data_w)
    ,.outport_idx_o(output_inport_idx_w)
    ,.outport_id_o(output_inport_id_w)
);


jpeg_dqt
u_jpeg_dqt
(
    // Inputs
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.img_start_i(img_start_w)
    ,.img_end_i(img_end_w)
    ,.img_dqt_table_y_i(img_dqt_table_y_w)
    ,.img_dqt_table_cb_i(img_dqt_table_cb_w)
    ,.img_dqt_table_cr_i(img_dqt_table_cr_w)
    ,.cfg_valid_i(dqt_cfg_valid_w)
    ,.cfg_data_i(dqt_cfg_data_w)
    ,.cfg_last_i(dqt_cfg_last_w)
    ,.inport_valid_i(dqt_inport_valid_w)
    ,.inport_data_i(dqt_outport_data_w)
    ,.inport_idx_i(dqt_inport_idx_w)
    ,.inport_id_i(dqt_inport_id_w)
    ,.inport_eob_i(dqt_inport_eob_w)
    ,.outport_accept_i(idct_inport_accept_w)

    // Outputs
    ,.cfg_accept_o(dqt_cfg_accept_w)
    ,.inport_blk_space_o(dqt_inport_blk_space_w)
    ,.outport_valid_o(idct_inport_valid_w)
    ,.outport_data_o(idct_outport_data_w)
    ,.outport_idx_o(idct_inport_idx_w)
    ,.outport_id_o(idct_inport_id_w)
    ,.outport_eob_o(idct_inport_eob_w)
);


jpeg_output
u_jpeg_output
(
    // Inputs
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.img_start_i(img_start_w)
    ,.img_end_i(img_end_w)
    ,.img_width_i(img_width_w)
    ,.img_height_i(img_height_w)
    ,.img_mode_i(img_mode_w)
    ,.inport_valid_i(output_inport_valid_w)
    ,.inport_data_i(output_outport_data_w)
    ,.inport_idx_i(output_inport_idx_w)
    ,.inport_id_i(output_inport_id_w)
    ,.outport_accept_i(outport_accept_i)

    // Outputs
    ,.inport_accept_o(output_inport_accept_w)
    ,.outport_valid_o(outport_valid_o)
    ,.outport_width_o(outport_width_o)
    ,.outport_height_o(outport_height_o)
    ,.outport_pixel_x_o(outport_pixel_x_o)
    ,.outport_pixel_y_o(outport_pixel_y_o)
    ,.outport_pixel_r_o(outport_pixel_r_o)
    ,.outport_pixel_g_o(outport_pixel_g_o)
    ,.outport_pixel_b_o(outport_pixel_b_o)
    ,.idle_o(idle_o)
);


jpeg_bitbuffer
u_jpeg_bitbuffer
(
    // Inputs
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.img_start_i(img_start_w)
    ,.img_end_i(img_end_w)
    ,.inport_valid_i(bb_inport_valid_w)
    ,.inport_data_i(bb_inport_data_w)
    ,.inport_last_i(bb_inport_last_w)
    ,.outport_pop_i(bb_outport_pop_w)

    // Outputs
    ,.inport_accept_o(bb_inport_accept_w)
    ,.outport_valid_o(bb_outport_valid_w)
    ,.outport_data_o(bb_outport_data_w)
    ,.outport_last_o(bb_outport_last_w)
);


jpeg_mcu_proc
u_jpeg_mcu_proc
(
    // Inputs
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.img_start_i(img_start_w)
    ,.img_end_i(img_end_w)
    ,.img_width_i(img_width_w)
    ,.img_height_i(img_height_w)
    ,.img_mode_i(img_mode_w)
    ,.inport_valid_i(bb_outport_valid_w)
    ,.inport_data_i(bb_outport_data_w)
    ,.inport_last_i(bb_outport_last_w)
    ,.lookup_valid_i(lookup_valid_w)
    ,.lookup_width_i(lookup_width_w)
    ,.lookup_value_i(lookup_value_w)
    ,.outport_blk_space_i(dqt_inport_blk_space_w)

    // Outputs
    ,.inport_pop_o(bb_outport_pop_w)
    ,.lookup_req_o(lookup_req_w)
    ,.lookup_table_o(lookup_table_w)
    ,.lookup_input_o(lookup_input_w)
    ,.outport_valid_o(dqt_inport_valid_w)
    ,.outport_data_o(dqt_outport_data_w)
    ,.outport_idx_o(dqt_inport_idx_w)
    ,.outport_id_o(dqt_inport_id_w)
    ,.outport_eob_o(dqt_inport_eob_w)
);



endmodule
