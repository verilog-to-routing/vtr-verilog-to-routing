//-----------------------------------------------------------------
//                     Basic Peripheral SoC
//                           V1.1
//                     Ultra-Embedded.com
//                     Copyright 2014-2020
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

module axi4lite_dist
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           inport_awvalid_i
    ,input  [ 31:0]  inport_awaddr_i
    ,input           inport_wvalid_i
    ,input  [ 31:0]  inport_wdata_i
    ,input  [  3:0]  inport_wstrb_i
    ,input           inport_bready_i
    ,input           inport_arvalid_i
    ,input  [ 31:0]  inport_araddr_i
    ,input           inport_rready_i
    ,input           outport0_awready_i
    ,input           outport0_wready_i
    ,input           outport0_bvalid_i
    ,input  [  1:0]  outport0_bresp_i
    ,input           outport0_arready_i
    ,input           outport0_rvalid_i
    ,input  [ 31:0]  outport0_rdata_i
    ,input  [  1:0]  outport0_rresp_i
    ,input           outport1_awready_i
    ,input           outport1_wready_i
    ,input           outport1_bvalid_i
    ,input  [  1:0]  outport1_bresp_i
    ,input           outport1_arready_i
    ,input           outport1_rvalid_i
    ,input  [ 31:0]  outport1_rdata_i
    ,input  [  1:0]  outport1_rresp_i
    ,input           outport2_awready_i
    ,input           outport2_wready_i
    ,input           outport2_bvalid_i
    ,input  [  1:0]  outport2_bresp_i
    ,input           outport2_arready_i
    ,input           outport2_rvalid_i
    ,input  [ 31:0]  outport2_rdata_i
    ,input  [  1:0]  outport2_rresp_i
    ,input           outport3_awready_i
    ,input           outport3_wready_i
    ,input           outport3_bvalid_i
    ,input  [  1:0]  outport3_bresp_i
    ,input           outport3_arready_i
    ,input           outport3_rvalid_i
    ,input  [ 31:0]  outport3_rdata_i
    ,input  [  1:0]  outport3_rresp_i
    ,input           outport4_awready_i
    ,input           outport4_wready_i
    ,input           outport4_bvalid_i
    ,input  [  1:0]  outport4_bresp_i
    ,input           outport4_arready_i
    ,input           outport4_rvalid_i
    ,input  [ 31:0]  outport4_rdata_i
    ,input  [  1:0]  outport4_rresp_i
    ,input           outport5_awready_i
    ,input           outport5_wready_i
    ,input           outport5_bvalid_i
    ,input  [  1:0]  outport5_bresp_i
    ,input           outport5_arready_i
    ,input           outport5_rvalid_i
    ,input  [ 31:0]  outport5_rdata_i
    ,input  [  1:0]  outport5_rresp_i
    ,input           outport6_awready_i
    ,input           outport6_wready_i
    ,input           outport6_bvalid_i
    ,input  [  1:0]  outport6_bresp_i
    ,input           outport6_arready_i
    ,input           outport6_rvalid_i
    ,input  [ 31:0]  outport6_rdata_i
    ,input  [  1:0]  outport6_rresp_i
    ,input           outport7_awready_i
    ,input           outport7_wready_i
    ,input           outport7_bvalid_i
    ,input  [  1:0]  outport7_bresp_i
    ,input           outport7_arready_i
    ,input           outport7_rvalid_i
    ,input  [ 31:0]  outport7_rdata_i
    ,input  [  1:0]  outport7_rresp_i

    // Outputs
    ,output          inport_awready_o
    ,output          inport_wready_o
    ,output          inport_bvalid_o
    ,output [  1:0]  inport_bresp_o
    ,output          inport_arready_o
    ,output          inport_rvalid_o
    ,output [ 31:0]  inport_rdata_o
    ,output [  1:0]  inport_rresp_o
    ,output          outport0_awvalid_o
    ,output [ 31:0]  outport0_awaddr_o
    ,output          outport0_wvalid_o
    ,output [ 31:0]  outport0_wdata_o
    ,output [  3:0]  outport0_wstrb_o
    ,output          outport0_bready_o
    ,output          outport0_arvalid_o
    ,output [ 31:0]  outport0_araddr_o
    ,output          outport0_rready_o
    ,output          outport1_awvalid_o
    ,output [ 31:0]  outport1_awaddr_o
    ,output          outport1_wvalid_o
    ,output [ 31:0]  outport1_wdata_o
    ,output [  3:0]  outport1_wstrb_o
    ,output          outport1_bready_o
    ,output          outport1_arvalid_o
    ,output [ 31:0]  outport1_araddr_o
    ,output          outport1_rready_o
    ,output          outport2_awvalid_o
    ,output [ 31:0]  outport2_awaddr_o
    ,output          outport2_wvalid_o
    ,output [ 31:0]  outport2_wdata_o
    ,output [  3:0]  outport2_wstrb_o
    ,output          outport2_bready_o
    ,output          outport2_arvalid_o
    ,output [ 31:0]  outport2_araddr_o
    ,output          outport2_rready_o
    ,output          outport3_awvalid_o
    ,output [ 31:0]  outport3_awaddr_o
    ,output          outport3_wvalid_o
    ,output [ 31:0]  outport3_wdata_o
    ,output [  3:0]  outport3_wstrb_o
    ,output          outport3_bready_o
    ,output          outport3_arvalid_o
    ,output [ 31:0]  outport3_araddr_o
    ,output          outport3_rready_o
    ,output          outport4_awvalid_o
    ,output [ 31:0]  outport4_awaddr_o
    ,output          outport4_wvalid_o
    ,output [ 31:0]  outport4_wdata_o
    ,output [  3:0]  outport4_wstrb_o
    ,output          outport4_bready_o
    ,output          outport4_arvalid_o
    ,output [ 31:0]  outport4_araddr_o
    ,output          outport4_rready_o
    ,output          outport5_awvalid_o
    ,output [ 31:0]  outport5_awaddr_o
    ,output          outport5_wvalid_o
    ,output [ 31:0]  outport5_wdata_o
    ,output [  3:0]  outport5_wstrb_o
    ,output          outport5_bready_o
    ,output          outport5_arvalid_o
    ,output [ 31:0]  outport5_araddr_o
    ,output          outport5_rready_o
    ,output          outport6_awvalid_o
    ,output [ 31:0]  outport6_awaddr_o
    ,output          outport6_wvalid_o
    ,output [ 31:0]  outport6_wdata_o
    ,output [  3:0]  outport6_wstrb_o
    ,output          outport6_bready_o
    ,output          outport6_arvalid_o
    ,output [ 31:0]  outport6_araddr_o
    ,output          outport6_rready_o
    ,output          outport7_awvalid_o
    ,output [ 31:0]  outport7_awaddr_o
    ,output          outport7_wvalid_o
    ,output [ 31:0]  outport7_wdata_o
    ,output [  3:0]  outport7_wstrb_o
    ,output          outport7_bready_o
    ,output          outport7_arvalid_o
    ,output [ 31:0]  outport7_araddr_o
    ,output          outport7_rready_o
);



//-----------------------------------------------------------------
// Read Dist
//-----------------------------------------------------------------
reg [2:0] read_sel_r;
reg [2:0] read_sel_q;
reg read_pending_q;
reg read_pending_r;

always @ *
begin
    read_pending_r = read_pending_q;

    // Read response
    if (inport_rvalid_o && inport_rready_i)
        read_pending_r = 1'b0;
    // New request
    else if (!read_pending_r && inport_arvalid_i)
        read_pending_r = inport_arready_o;

    // Address to port selection
    if (!read_pending_q)
        read_sel_r  = inport_araddr_i[26:24];
    else
        read_sel_r  = read_sel_q;
end

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
begin
    read_sel_q      <= 3'b0;
    read_pending_q  <= 1'b0;
end
else
begin
    read_sel_q      <= read_sel_r;
    read_pending_q  <= read_pending_r;
end

//-----------------------------------------------------------------
// Read Request
//-----------------------------------------------------------------
assign outport0_arvalid_o =  inport_arvalid_i && (read_sel_r == 3'd0) && !read_pending_q;
assign outport0_araddr_o  =  inport_araddr_i;
assign outport1_arvalid_o =  inport_arvalid_i && (read_sel_r == 3'd1) && !read_pending_q;
assign outport1_araddr_o  =  inport_araddr_i;
assign outport2_arvalid_o =  inport_arvalid_i && (read_sel_r == 3'd2) && !read_pending_q;
assign outport2_araddr_o  =  inport_araddr_i;
assign outport3_arvalid_o =  inport_arvalid_i && (read_sel_r == 3'd3) && !read_pending_q;
assign outport3_araddr_o  =  inport_araddr_i;
assign outport4_arvalid_o =  inport_arvalid_i && (read_sel_r == 3'd4) && !read_pending_q;
assign outport4_araddr_o  =  inport_araddr_i;
assign outport5_arvalid_o =  inport_arvalid_i && (read_sel_r == 3'd5) && !read_pending_q;
assign outport5_araddr_o  =  inport_araddr_i;
assign outport6_arvalid_o =  inport_arvalid_i && (read_sel_r == 3'd6) && !read_pending_q;
assign outport6_araddr_o  =  inport_araddr_i;
assign outport7_arvalid_o =  inport_arvalid_i && (read_sel_r == 3'd7) && !read_pending_q;
assign outport7_araddr_o  =  inport_araddr_i;

//-----------------------------------------------------------------
// Read Request Accept
//-----------------------------------------------------------------
reg inport_arready_r;

always @ *
begin
    inport_arready_r  = 1'b0;

    case (read_sel_r)
    3'd0:
        inport_arready_r = outport0_arready_i;
    3'd1:
        inport_arready_r = outport1_arready_i;
    3'd2:
        inport_arready_r = outport2_arready_i;
    3'd3:
        inport_arready_r = outport3_arready_i;
    3'd4:
        inport_arready_r = outport4_arready_i;
    3'd5:
        inport_arready_r = outport5_arready_i;
    3'd6:
        inport_arready_r = outport6_arready_i;
    3'd7:
        inport_arready_r = outport7_arready_i;
    default :
        ;
    endcase
end

assign inport_arready_o = inport_arready_r && !read_pending_q;

//-----------------------------------------------------------------
// Read Response
//-----------------------------------------------------------------
reg        inport_rvalid_r;
reg [31:0] inport_rdata_r;
reg [1:0]  inport_rresp_r;

always @ *
begin
    inport_rvalid_r  = 1'b0;
    inport_rdata_r   = 32'b0;
    inport_rresp_r   = 2'b0;

    case (read_sel_q)
    3'd0:
    begin
        inport_rvalid_r = outport0_rvalid_i;
        inport_rdata_r  = outport0_rdata_i;
        inport_rresp_r  = outport0_rresp_i;
    end
    3'd1:
    begin
        inport_rvalid_r = outport1_rvalid_i;
        inport_rdata_r  = outport1_rdata_i;
        inport_rresp_r  = outport1_rresp_i;
    end
    3'd2:
    begin
        inport_rvalid_r = outport2_rvalid_i;
        inport_rdata_r  = outport2_rdata_i;
        inport_rresp_r  = outport2_rresp_i;
    end
    3'd3:
    begin
        inport_rvalid_r = outport3_rvalid_i;
        inport_rdata_r  = outport3_rdata_i;
        inport_rresp_r  = outport3_rresp_i;
    end
    3'd4:
    begin
        inport_rvalid_r = outport4_rvalid_i;
        inport_rdata_r  = outport4_rdata_i;
        inport_rresp_r  = outport4_rresp_i;
    end
    3'd5:
    begin
        inport_rvalid_r = outport5_rvalid_i;
        inport_rdata_r  = outport5_rdata_i;
        inport_rresp_r  = outport5_rresp_i;
    end
    3'd6:
    begin
        inport_rvalid_r = outport6_rvalid_i;
        inport_rdata_r  = outport6_rdata_i;
        inport_rresp_r  = outport6_rresp_i;
    end
    3'd7:
    begin
        inport_rvalid_r = outport7_rvalid_i;
        inport_rdata_r  = outport7_rdata_i;
        inport_rresp_r  = outport7_rresp_i;
    end
    default :
        ;
    endcase
end

assign inport_rvalid_o = inport_rvalid_r;
assign inport_rdata_o  = inport_rdata_r;
assign inport_rresp_o  = inport_rresp_r;

//-----------------------------------------------------------------
// Read Response accept
//-----------------------------------------------------------------
assign outport0_rready_o = inport_rready_i && (read_sel_q == 3'd0);
assign outport1_rready_o = inport_rready_i && (read_sel_q == 3'd1);
assign outport2_rready_o = inport_rready_i && (read_sel_q == 3'd2);
assign outport3_rready_o = inport_rready_i && (read_sel_q == 3'd3);
assign outport4_rready_o = inport_rready_i && (read_sel_q == 3'd4);
assign outport5_rready_o = inport_rready_i && (read_sel_q == 3'd5);
assign outport6_rready_o = inport_rready_i && (read_sel_q == 3'd6);
assign outport7_rready_o = inport_rready_i && (read_sel_q == 3'd7);

//-----------------------------------------------------------------
// Write command tracking
//-----------------------------------------------------------------
reg  awvalid_q;
reg  wvalid_q;
wire wr_cmd_accepted_w  = (inport_awvalid_i && inport_awready_o) || awvalid_q;
wire wr_data_accepted_w = (inport_wvalid_i  && inport_wready_o)  || wvalid_q;

reg awvalid_r;

always @ *
begin
    awvalid_r   = awvalid_q;

    // Address ready, data not ready
    if (inport_awvalid_i && inport_awready_o && !wr_data_accepted_w)
        awvalid_r = 1'b1;
    else if (wr_data_accepted_w)
        awvalid_r = 1'b0;
end

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    awvalid_q   <= 1'b0;
else
    awvalid_q   <= awvalid_r;

//-----------------------------------------------------------------
// Write data tracking
//-----------------------------------------------------------------
reg wvalid_r;

always @ *
begin
    wvalid_r   = wvalid_q;

    // Data ready, address not ready
    if (inport_wvalid_i && inport_wready_o && !wr_cmd_accepted_w)
        wvalid_r = 1'b1;
    else if (wr_cmd_accepted_w)
        wvalid_r = 1'b0;
end

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    wvalid_q   <= 1'b0;
else
    wvalid_q   <= wvalid_r;

//-----------------------------------------------------------------
// Write Dist
//-----------------------------------------------------------------
reg [2:0] write_sel_r;
reg [2:0] write_sel_q;
reg write_pending_q;
reg write_pending_r;

always @ *
begin
    write_pending_r  = write_pending_q;
    write_sel_r      = write_sel_q;

    // Write response
    if (inport_bvalid_o && inport_bready_i)
        write_pending_r = 1'b0;
    // New request - both command and data accepted
    else if (wr_cmd_accepted_w && wr_data_accepted_w)
        write_pending_r = 1'b1;

    // New request - latch address to port selection
    if (inport_awvalid_i && !awvalid_q && !write_pending_q)
        write_sel_r     = inport_awaddr_i[26:24];
end

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
begin
    write_sel_q      <= 3'b0;
    write_pending_q  <= 1'b0;
end
else
begin
    write_sel_q      <= write_sel_r;
    write_pending_q  <= write_pending_r;
end

//-----------------------------------------------------------------
// Write Request
//-----------------------------------------------------------------
assign outport0_awvalid_o =  inport_awvalid_i && (write_sel_r == 3'd0) && !awvalid_q && !write_pending_q;
assign outport0_awaddr_o  =  inport_awaddr_i;
assign outport0_wvalid_o  =  inport_wvalid_i && (inport_awvalid_i || awvalid_q) && (write_sel_r == 3'd0) && !wvalid_q && !write_pending_q;
assign outport0_wdata_o   =  inport_wdata_i;
assign outport0_wstrb_o   =  inport_wstrb_i;
assign outport1_awvalid_o =  inport_awvalid_i && (write_sel_r == 3'd1) && !awvalid_q && !write_pending_q;
assign outport1_awaddr_o  =  inport_awaddr_i;
assign outport1_wvalid_o  =  inport_wvalid_i && (inport_awvalid_i || awvalid_q) && (write_sel_r == 3'd1) && !wvalid_q && !write_pending_q;
assign outport1_wdata_o   =  inport_wdata_i;
assign outport1_wstrb_o   =  inport_wstrb_i;
assign outport2_awvalid_o =  inport_awvalid_i && (write_sel_r == 3'd2) && !awvalid_q && !write_pending_q;
assign outport2_awaddr_o  =  inport_awaddr_i;
assign outport2_wvalid_o  =  inport_wvalid_i && (inport_awvalid_i || awvalid_q) && (write_sel_r == 3'd2) && !wvalid_q && !write_pending_q;
assign outport2_wdata_o   =  inport_wdata_i;
assign outport2_wstrb_o   =  inport_wstrb_i;
assign outport3_awvalid_o =  inport_awvalid_i && (write_sel_r == 3'd3) && !awvalid_q && !write_pending_q;
assign outport3_awaddr_o  =  inport_awaddr_i;
assign outport3_wvalid_o  =  inport_wvalid_i && (inport_awvalid_i || awvalid_q) && (write_sel_r == 3'd3) && !wvalid_q && !write_pending_q;
assign outport3_wdata_o   =  inport_wdata_i;
assign outport3_wstrb_o   =  inport_wstrb_i;
assign outport4_awvalid_o =  inport_awvalid_i && (write_sel_r == 3'd4) && !awvalid_q && !write_pending_q;
assign outport4_awaddr_o  =  inport_awaddr_i;
assign outport4_wvalid_o  =  inport_wvalid_i && (inport_awvalid_i || awvalid_q) && (write_sel_r == 3'd4) && !wvalid_q && !write_pending_q;
assign outport4_wdata_o   =  inport_wdata_i;
assign outport4_wstrb_o   =  inport_wstrb_i;
assign outport5_awvalid_o =  inport_awvalid_i && (write_sel_r == 3'd5) && !awvalid_q && !write_pending_q;
assign outport5_awaddr_o  =  inport_awaddr_i;
assign outport5_wvalid_o  =  inport_wvalid_i && (inport_awvalid_i || awvalid_q) && (write_sel_r == 3'd5) && !wvalid_q && !write_pending_q;
assign outport5_wdata_o   =  inport_wdata_i;
assign outport5_wstrb_o   =  inport_wstrb_i;
assign outport6_awvalid_o =  inport_awvalid_i && (write_sel_r == 3'd6) && !awvalid_q && !write_pending_q;
assign outport6_awaddr_o  =  inport_awaddr_i;
assign outport6_wvalid_o  =  inport_wvalid_i && (inport_awvalid_i || awvalid_q) && (write_sel_r == 3'd6) && !wvalid_q && !write_pending_q;
assign outport6_wdata_o   =  inport_wdata_i;
assign outport6_wstrb_o   =  inport_wstrb_i;
assign outport7_awvalid_o =  inport_awvalid_i && (write_sel_r == 3'd7) && !awvalid_q && !write_pending_q;
assign outport7_awaddr_o  =  inport_awaddr_i;
assign outport7_wvalid_o  =  inport_wvalid_i && (inport_awvalid_i || awvalid_q) && (write_sel_r == 3'd7) && !wvalid_q && !write_pending_q;
assign outport7_wdata_o   =  inport_wdata_i;
assign outport7_wstrb_o   =  inport_wstrb_i;

//-----------------------------------------------------------------
// Write Request Accept
//-----------------------------------------------------------------
reg inport_awready_r;
reg inport_wready_r;

always @ *
begin
    inport_awready_r  = 1'b0;
    inport_wready_r   = 1'b0;

    case (write_sel_r)
    3'd0:
    begin
        inport_awready_r = outport0_awready_i;
        inport_wready_r  = outport0_wready_i;
    end
    3'd1:
    begin
        inport_awready_r = outport1_awready_i;
        inport_wready_r  = outport1_wready_i;
    end
    3'd2:
    begin
        inport_awready_r = outport2_awready_i;
        inport_wready_r  = outport2_wready_i;
    end
    3'd3:
    begin
        inport_awready_r = outport3_awready_i;
        inport_wready_r  = outport3_wready_i;
    end
    3'd4:
    begin
        inport_awready_r = outport4_awready_i;
        inport_wready_r  = outport4_wready_i;
    end
    3'd5:
    begin
        inport_awready_r = outport5_awready_i;
        inport_wready_r  = outport5_wready_i;
    end
    3'd6:
    begin
        inport_awready_r = outport6_awready_i;
        inport_wready_r  = outport6_wready_i;
    end
    3'd7:
    begin
        inport_awready_r = outport7_awready_i;
        inport_wready_r  = outport7_wready_i;
    end
    default :
        ;
    endcase
end

assign inport_awready_o = inport_awready_r && !awvalid_q && !write_pending_q;
assign inport_wready_o  = inport_wready_r  && !wvalid_q  && !write_pending_q;

//-----------------------------------------------------------------
// Write Response
//-----------------------------------------------------------------
reg        inport_bvalid_r;
reg [1:0]  inport_bresp_r;

always @ *
begin
    inport_bvalid_r  = 1'b0;
    inport_bresp_r   = 2'b0;

    case (write_sel_q)
    3'd0:
    begin
        inport_bvalid_r = outport0_bvalid_i;
        inport_bresp_r  = outport0_bresp_i;
    end
    3'd1:
    begin
        inport_bvalid_r = outport1_bvalid_i;
        inport_bresp_r  = outport1_bresp_i;
    end
    3'd2:
    begin
        inport_bvalid_r = outport2_bvalid_i;
        inport_bresp_r  = outport2_bresp_i;
    end
    3'd3:
    begin
        inport_bvalid_r = outport3_bvalid_i;
        inport_bresp_r  = outport3_bresp_i;
    end
    3'd4:
    begin
        inport_bvalid_r = outport4_bvalid_i;
        inport_bresp_r  = outport4_bresp_i;
    end
    3'd5:
    begin
        inport_bvalid_r = outport5_bvalid_i;
        inport_bresp_r  = outport5_bresp_i;
    end
    3'd6:
    begin
        inport_bvalid_r = outport6_bvalid_i;
        inport_bresp_r  = outport6_bresp_i;
    end
    3'd7:
    begin
        inport_bvalid_r = outport7_bvalid_i;
        inport_bresp_r  = outport7_bresp_i;
    end
    default :
        ;
    endcase
end

assign inport_bvalid_o = inport_bvalid_r;
assign inport_bresp_o  = inport_bresp_r;

//-----------------------------------------------------------------
// Write Response accept
//-----------------------------------------------------------------
assign outport0_bready_o = inport_bready_i && (write_sel_q == 3'd0);
assign outport1_bready_o = inport_bready_i && (write_sel_q == 3'd1);
assign outport2_bready_o = inport_bready_i && (write_sel_q == 3'd2);
assign outport3_bready_o = inport_bready_i && (write_sel_q == 3'd3);
assign outport4_bready_o = inport_bready_i && (write_sel_q == 3'd4);
assign outport5_bready_o = inport_bready_i && (write_sel_q == 3'd5);
assign outport6_bready_o = inport_bready_i && (write_sel_q == 3'd6);
assign outport7_bready_o = inport_bready_i && (write_sel_q == 3'd7);



endmodule
//-----------------------------------------------------------------
//                     Basic Peripheral SoC
//                           V1.1
//                     Ultra-Embedded.com
//                     Copyright 2014-2020
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

`define SPI_DGIER    8'h1c

    `define SPI_DGIER_GIE      31
    `define SPI_DGIER_GIE_DEFAULT    0
    `define SPI_DGIER_GIE_B          31
    `define SPI_DGIER_GIE_T          31
    `define SPI_DGIER_GIE_W          1
    `define SPI_DGIER_GIE_R          31:31

`define SPI_IPISR    8'h20

    `define SPI_IPISR_TX_EMPTY      2
    `define SPI_IPISR_TX_EMPTY_DEFAULT    0
    `define SPI_IPISR_TX_EMPTY_B          2
    `define SPI_IPISR_TX_EMPTY_T          2
    `define SPI_IPISR_TX_EMPTY_W          1
    `define SPI_IPISR_TX_EMPTY_R          2:2

`define SPI_IPIER    8'h28

    `define SPI_IPIER_TX_EMPTY      2
    `define SPI_IPIER_TX_EMPTY_DEFAULT    0
    `define SPI_IPIER_TX_EMPTY_B          2
    `define SPI_IPIER_TX_EMPTY_T          2
    `define SPI_IPIER_TX_EMPTY_W          1
    `define SPI_IPIER_TX_EMPTY_R          2:2

`define SPI_SRR    8'h40

    `define SPI_SRR_RESET_DEFAULT    0
    `define SPI_SRR_RESET_B          0
    `define SPI_SRR_RESET_T          31
    `define SPI_SRR_RESET_W          32
    `define SPI_SRR_RESET_R          31:0

`define SPI_CR    8'h60

    `define SPI_CR_LOOP      0
    `define SPI_CR_LOOP_DEFAULT    0
    `define SPI_CR_LOOP_B          0
    `define SPI_CR_LOOP_T          0
    `define SPI_CR_LOOP_W          1
    `define SPI_CR_LOOP_R          0:0

    `define SPI_CR_SPE      1
    `define SPI_CR_SPE_DEFAULT    0
    `define SPI_CR_SPE_B          1
    `define SPI_CR_SPE_T          1
    `define SPI_CR_SPE_W          1
    `define SPI_CR_SPE_R          1:1

    `define SPI_CR_MASTER      2
    `define SPI_CR_MASTER_DEFAULT    0
    `define SPI_CR_MASTER_B          2
    `define SPI_CR_MASTER_T          2
    `define SPI_CR_MASTER_W          1
    `define SPI_CR_MASTER_R          2:2

    `define SPI_CR_CPOL      3
    `define SPI_CR_CPOL_DEFAULT    0
    `define SPI_CR_CPOL_B          3
    `define SPI_CR_CPOL_T          3
    `define SPI_CR_CPOL_W          1
    `define SPI_CR_CPOL_R          3:3

    `define SPI_CR_CPHA      4
    `define SPI_CR_CPHA_DEFAULT    0
    `define SPI_CR_CPHA_B          4
    `define SPI_CR_CPHA_T          4
    `define SPI_CR_CPHA_W          1
    `define SPI_CR_CPHA_R          4:4

    `define SPI_CR_TXFIFO_RST      5
    `define SPI_CR_TXFIFO_RST_DEFAULT    0
    `define SPI_CR_TXFIFO_RST_B          5
    `define SPI_CR_TXFIFO_RST_T          5
    `define SPI_CR_TXFIFO_RST_W          1
    `define SPI_CR_TXFIFO_RST_R          5:5

    `define SPI_CR_RXFIFO_RST      6
    `define SPI_CR_RXFIFO_RST_DEFAULT    0
    `define SPI_CR_RXFIFO_RST_B          6
    `define SPI_CR_RXFIFO_RST_T          6
    `define SPI_CR_RXFIFO_RST_W          1
    `define SPI_CR_RXFIFO_RST_R          6:6

    `define SPI_CR_MANUAL_SS      7
    `define SPI_CR_MANUAL_SS_DEFAULT    0
    `define SPI_CR_MANUAL_SS_B          7
    `define SPI_CR_MANUAL_SS_T          7
    `define SPI_CR_MANUAL_SS_W          1
    `define SPI_CR_MANUAL_SS_R          7:7

    `define SPI_CR_TRANS_INHIBIT      8
    `define SPI_CR_TRANS_INHIBIT_DEFAULT    0
    `define SPI_CR_TRANS_INHIBIT_B          8
    `define SPI_CR_TRANS_INHIBIT_T          8
    `define SPI_CR_TRANS_INHIBIT_W          1
    `define SPI_CR_TRANS_INHIBIT_R          8:8

    `define SPI_CR_LSB_FIRST      9
    `define SPI_CR_LSB_FIRST_DEFAULT    0
    `define SPI_CR_LSB_FIRST_B          9
    `define SPI_CR_LSB_FIRST_T          9
    `define SPI_CR_LSB_FIRST_W          1
    `define SPI_CR_LSB_FIRST_R          9:9

`define SPI_SR    8'h64

    `define SPI_SR_RX_EMPTY      0
    `define SPI_SR_RX_EMPTY_DEFAULT    0
    `define SPI_SR_RX_EMPTY_B          0
    `define SPI_SR_RX_EMPTY_T          0
    `define SPI_SR_RX_EMPTY_W          1
    `define SPI_SR_RX_EMPTY_R          0:0

    `define SPI_SR_RX_FULL      1
    `define SPI_SR_RX_FULL_DEFAULT    0
    `define SPI_SR_RX_FULL_B          1
    `define SPI_SR_RX_FULL_T          1
    `define SPI_SR_RX_FULL_W          1
    `define SPI_SR_RX_FULL_R          1:1

    `define SPI_SR_TX_EMPTY      2
    `define SPI_SR_TX_EMPTY_DEFAULT    0
    `define SPI_SR_TX_EMPTY_B          2
    `define SPI_SR_TX_EMPTY_T          2
    `define SPI_SR_TX_EMPTY_W          1
    `define SPI_SR_TX_EMPTY_R          2:2

    `define SPI_SR_TX_FULL      3
    `define SPI_SR_TX_FULL_DEFAULT    0
    `define SPI_SR_TX_FULL_B          3
    `define SPI_SR_TX_FULL_T          3
    `define SPI_SR_TX_FULL_W          1
    `define SPI_SR_TX_FULL_R          3:3

`define SPI_DTR    8'h68

    `define SPI_DTR_DATA_DEFAULT    0
    `define SPI_DTR_DATA_B          0
    `define SPI_DTR_DATA_T          7
    `define SPI_DTR_DATA_W          8
    `define SPI_DTR_DATA_R          7:0

`define SPI_DRR    8'h6c

    `define SPI_DRR_DATA_DEFAULT    0
    `define SPI_DRR_DATA_B          0
    `define SPI_DRR_DATA_T          7
    `define SPI_DRR_DATA_W          8
    `define SPI_DRR_DATA_R          7:0

`define SPI_SSR    8'h70

    `define SPI_SSR_VALUE_DEFAULT    1
    `define SPI_SSR_VALUE_B          0
    `define SPI_SSR_VALUE_T          7
    `define SPI_SSR_VALUE_W          8
    `define SPI_SSR_VALUE_R          7:0

//-----------------------------------------------------------------
//                     Basic Peripheral SoC
//                           V1.1
//                     Ultra-Embedded.com
//                     Copyright 2014-2020
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

// `include "spi_lite_defs.v"

//-----------------------------------------------------------------
// Module:  SPI-Lite Peripheral (Xilinx IP emulation)
//-----------------------------------------------------------------
module spi_lite
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
     parameter C_SCK_RATIO      = 32
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
    ,input          spi_miso_i

    // Outputs
    ,output         cfg_awready_o
    ,output         cfg_wready_o
    ,output         cfg_bvalid_o
    ,output [1:0]   cfg_bresp_o
    ,output         cfg_arready_o
    ,output         cfg_rvalid_o
    ,output [31:0]  cfg_rdata_o
    ,output [1:0]   cfg_rresp_o
    ,output         spi_clk_o
    ,output         spi_mosi_o
    ,output [7:0]   spi_cs_o
    ,output         intr_o
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
// Register spi_dgier
//-----------------------------------------------------------------
reg spi_dgier_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    spi_dgier_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `SPI_DGIER))
    spi_dgier_wr_q <= 1'b1;
else
    spi_dgier_wr_q <= 1'b0;

// spi_dgier_gie [internal]
reg        spi_dgier_gie_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    spi_dgier_gie_q <= 1'd`SPI_DGIER_GIE_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `SPI_DGIER))
    spi_dgier_gie_q <= cfg_wdata_i[`SPI_DGIER_GIE_R];

wire        spi_dgier_gie_out_w = spi_dgier_gie_q;


//-----------------------------------------------------------------
// Register spi_ipisr
//-----------------------------------------------------------------
reg spi_ipisr_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    spi_ipisr_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `SPI_IPISR))
    spi_ipisr_wr_q <= 1'b1;
else
    spi_ipisr_wr_q <= 1'b0;

// spi_ipisr_tx_empty [external]
wire        spi_ipisr_tx_empty_out_w = wr_data_q[`SPI_IPISR_TX_EMPTY_R];


//-----------------------------------------------------------------
// Register spi_ipier
//-----------------------------------------------------------------
reg spi_ipier_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    spi_ipier_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `SPI_IPIER))
    spi_ipier_wr_q <= 1'b1;
else
    spi_ipier_wr_q <= 1'b0;

// spi_ipier_tx_empty [internal]
reg        spi_ipier_tx_empty_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    spi_ipier_tx_empty_q <= 1'd`SPI_IPIER_TX_EMPTY_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `SPI_IPIER))
    spi_ipier_tx_empty_q <= cfg_wdata_i[`SPI_IPIER_TX_EMPTY_R];

wire        spi_ipier_tx_empty_out_w = spi_ipier_tx_empty_q;


//-----------------------------------------------------------------
// Register spi_srr
//-----------------------------------------------------------------
reg spi_srr_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    spi_srr_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `SPI_SRR))
    spi_srr_wr_q <= 1'b1;
else
    spi_srr_wr_q <= 1'b0;

// spi_srr_reset [auto_clr]
reg [31:0]  spi_srr_reset_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    spi_srr_reset_q <= 32'd`SPI_SRR_RESET_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `SPI_SRR))
    spi_srr_reset_q <= cfg_wdata_i[`SPI_SRR_RESET_R];
else
    spi_srr_reset_q <= 32'd`SPI_SRR_RESET_DEFAULT;

wire [31:0]  spi_srr_reset_out_w = spi_srr_reset_q;


//-----------------------------------------------------------------
// Register spi_cr
//-----------------------------------------------------------------
reg spi_cr_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    spi_cr_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `SPI_CR))
    spi_cr_wr_q <= 1'b1;
else
    spi_cr_wr_q <= 1'b0;

// spi_cr_loop [internal]
reg        spi_cr_loop_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    spi_cr_loop_q <= 1'd`SPI_CR_LOOP_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `SPI_CR))
    spi_cr_loop_q <= cfg_wdata_i[`SPI_CR_LOOP_R];

wire        spi_cr_loop_out_w = spi_cr_loop_q;


// spi_cr_spe [internal]
reg        spi_cr_spe_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    spi_cr_spe_q <= 1'd`SPI_CR_SPE_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `SPI_CR))
    spi_cr_spe_q <= cfg_wdata_i[`SPI_CR_SPE_R];

wire        spi_cr_spe_out_w = spi_cr_spe_q;


// spi_cr_master [internal]
reg        spi_cr_master_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    spi_cr_master_q <= 1'd`SPI_CR_MASTER_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `SPI_CR))
    spi_cr_master_q <= cfg_wdata_i[`SPI_CR_MASTER_R];

wire        spi_cr_master_out_w = spi_cr_master_q;


// spi_cr_cpol [internal]
reg        spi_cr_cpol_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    spi_cr_cpol_q <= 1'd`SPI_CR_CPOL_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `SPI_CR))
    spi_cr_cpol_q <= cfg_wdata_i[`SPI_CR_CPOL_R];

wire        spi_cr_cpol_out_w = spi_cr_cpol_q;


// spi_cr_cpha [internal]
reg        spi_cr_cpha_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    spi_cr_cpha_q <= 1'd`SPI_CR_CPHA_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `SPI_CR))
    spi_cr_cpha_q <= cfg_wdata_i[`SPI_CR_CPHA_R];

wire        spi_cr_cpha_out_w = spi_cr_cpha_q;


// spi_cr_txfifo_rst [auto_clr]
reg        spi_cr_txfifo_rst_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    spi_cr_txfifo_rst_q <= 1'd`SPI_CR_TXFIFO_RST_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `SPI_CR))
    spi_cr_txfifo_rst_q <= cfg_wdata_i[`SPI_CR_TXFIFO_RST_R];
else
    spi_cr_txfifo_rst_q <= 1'd`SPI_CR_TXFIFO_RST_DEFAULT;

wire        spi_cr_txfifo_rst_out_w = spi_cr_txfifo_rst_q;


// spi_cr_rxfifo_rst [auto_clr]
reg        spi_cr_rxfifo_rst_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    spi_cr_rxfifo_rst_q <= 1'd`SPI_CR_RXFIFO_RST_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `SPI_CR))
    spi_cr_rxfifo_rst_q <= cfg_wdata_i[`SPI_CR_RXFIFO_RST_R];
else
    spi_cr_rxfifo_rst_q <= 1'd`SPI_CR_RXFIFO_RST_DEFAULT;

wire        spi_cr_rxfifo_rst_out_w = spi_cr_rxfifo_rst_q;


// spi_cr_manual_ss [internal]
reg        spi_cr_manual_ss_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    spi_cr_manual_ss_q <= 1'd`SPI_CR_MANUAL_SS_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `SPI_CR))
    spi_cr_manual_ss_q <= cfg_wdata_i[`SPI_CR_MANUAL_SS_R];

wire        spi_cr_manual_ss_out_w = spi_cr_manual_ss_q;


// spi_cr_trans_inhibit [internal]
reg        spi_cr_trans_inhibit_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    spi_cr_trans_inhibit_q <= 1'd`SPI_CR_TRANS_INHIBIT_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `SPI_CR))
    spi_cr_trans_inhibit_q <= cfg_wdata_i[`SPI_CR_TRANS_INHIBIT_R];

wire        spi_cr_trans_inhibit_out_w = spi_cr_trans_inhibit_q;


// spi_cr_lsb_first [internal]
reg        spi_cr_lsb_first_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    spi_cr_lsb_first_q <= 1'd`SPI_CR_LSB_FIRST_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `SPI_CR))
    spi_cr_lsb_first_q <= cfg_wdata_i[`SPI_CR_LSB_FIRST_R];

wire        spi_cr_lsb_first_out_w = spi_cr_lsb_first_q;


//-----------------------------------------------------------------
// Register spi_sr
//-----------------------------------------------------------------
reg spi_sr_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    spi_sr_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `SPI_SR))
    spi_sr_wr_q <= 1'b1;
else
    spi_sr_wr_q <= 1'b0;





//-----------------------------------------------------------------
// Register spi_dtr
//-----------------------------------------------------------------
reg spi_dtr_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    spi_dtr_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `SPI_DTR))
    spi_dtr_wr_q <= 1'b1;
else
    spi_dtr_wr_q <= 1'b0;

// spi_dtr_data [external]
wire [7:0]  spi_dtr_data_out_w = wr_data_q[`SPI_DTR_DATA_R];


//-----------------------------------------------------------------
// Register spi_drr
//-----------------------------------------------------------------
reg spi_drr_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    spi_drr_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `SPI_DRR))
    spi_drr_wr_q <= 1'b1;
else
    spi_drr_wr_q <= 1'b0;


//-----------------------------------------------------------------
// Register spi_ssr
//-----------------------------------------------------------------
reg spi_ssr_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    spi_ssr_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `SPI_SSR))
    spi_ssr_wr_q <= 1'b1;
else
    spi_ssr_wr_q <= 1'b0;

// spi_ssr_value [internal]
reg [7:0]  spi_ssr_value_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    spi_ssr_value_q <= 8'd`SPI_SSR_VALUE_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `SPI_SSR))
    spi_ssr_value_q <= cfg_wdata_i[`SPI_SSR_VALUE_R];

wire [7:0]  spi_ssr_value_out_w = spi_ssr_value_q;


wire        spi_ipisr_tx_empty_in_w;
wire        spi_sr_rx_empty_in_w;
wire        spi_sr_rx_full_in_w;
wire        spi_sr_tx_empty_in_w;
wire        spi_sr_tx_full_in_w;
wire [7:0]  spi_drr_data_in_w;


//-----------------------------------------------------------------
// Read mux
//-----------------------------------------------------------------
reg [31:0] data_r;

always @ *
begin
    data_r = 32'b0;

    case (cfg_araddr_i[7:0])

    `SPI_DGIER:
    begin
        data_r[`SPI_DGIER_GIE_R] = spi_dgier_gie_q;
    end
    `SPI_IPISR:
    begin
        data_r[`SPI_IPISR_TX_EMPTY_R] = spi_ipisr_tx_empty_in_w;
    end
    `SPI_IPIER:
    begin
        data_r[`SPI_IPIER_TX_EMPTY_R] = spi_ipier_tx_empty_q;
    end
    `SPI_SRR:
    begin
    end
    `SPI_CR:
    begin
        data_r[`SPI_CR_LOOP_R] = spi_cr_loop_q;
        data_r[`SPI_CR_SPE_R] = spi_cr_spe_q;
        data_r[`SPI_CR_MASTER_R] = spi_cr_master_q;
        data_r[`SPI_CR_CPOL_R] = spi_cr_cpol_q;
        data_r[`SPI_CR_CPHA_R] = spi_cr_cpha_q;
        data_r[`SPI_CR_MANUAL_SS_R] = spi_cr_manual_ss_q;
        data_r[`SPI_CR_TRANS_INHIBIT_R] = spi_cr_trans_inhibit_q;
        data_r[`SPI_CR_LSB_FIRST_R] = spi_cr_lsb_first_q;
    end
    `SPI_SR:
    begin
        data_r[`SPI_SR_RX_EMPTY_R] = spi_sr_rx_empty_in_w;
        data_r[`SPI_SR_RX_FULL_R] = spi_sr_rx_full_in_w;
        data_r[`SPI_SR_TX_EMPTY_R] = spi_sr_tx_empty_in_w;
        data_r[`SPI_SR_TX_FULL_R] = spi_sr_tx_full_in_w;
    end
    `SPI_DRR:
    begin
        data_r[`SPI_DRR_DATA_R] = spi_drr_data_in_w;
    end
    `SPI_SSR:
    begin
        data_r[`SPI_SSR_VALUE_R] = spi_ssr_value_q;
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

wire spi_cr_rd_req_w = read_en_w & (cfg_araddr_i[7:0] == `SPI_CR);
wire spi_drr_rd_req_w = read_en_w & (cfg_araddr_i[7:0] == `SPI_DRR);

wire spi_ipisr_wr_req_w = spi_ipisr_wr_q;
wire spi_cr_wr_req_w = spi_cr_wr_q;
wire spi_dtr_wr_req_w = spi_dtr_wr_q;
wire spi_drr_wr_req_w = spi_drr_wr_q;


//-----------------------------------------------------------------
// TX FIFO
//-----------------------------------------------------------------
wire       sw_reset_w      = spi_srr_reset_out_w == 32'h0000000A;
wire       tx_fifo_flush_w = sw_reset_w | spi_cr_txfifo_rst_out_w;
wire       rx_fifo_flush_w = sw_reset_w | spi_cr_rxfifo_rst_out_w;

wire       tx_accept_w;
wire       tx_ready_w;
wire [7:0] tx_data_raw_w;
wire       tx_pop_w;

spi_lite_fifo
#(
    .WIDTH(8),
    .DEPTH(4),
    .ADDR_W(2)
)
u_tx_fifo
(
    .clk_i(clk_i),
    .rst_i(rst_i),

    .flush_i(tx_fifo_flush_w),

    .data_in_i(spi_dtr_data_out_w),
    .push_i(spi_dtr_wr_req_w),
    .accept_o(tx_accept_w),

    .pop_i(tx_pop_w),
    .data_out_o(tx_data_raw_w),
    .valid_o(tx_ready_w)
);

assign spi_sr_tx_empty_in_w = ~tx_ready_w;
assign spi_sr_tx_full_in_w  = ~tx_accept_w;

// Reverse order if LSB first
wire [7:0] tx_data_w = spi_cr_lsb_first_out_w ? 
    {
      tx_data_raw_w[0]
    , tx_data_raw_w[1]
    , tx_data_raw_w[2]
    , tx_data_raw_w[3]
    , tx_data_raw_w[4]
    , tx_data_raw_w[5]
    , tx_data_raw_w[6]
    , tx_data_raw_w[7]
    } : tx_data_raw_w;

//-----------------------------------------------------------------
// RX FIFO
//-----------------------------------------------------------------
wire       rx_accept_w;
wire       rx_ready_w;
wire [7:0] rx_data_w;
wire       rx_push_w;

spi_lite_fifo
#(
    .WIDTH(8),
    .DEPTH(4),
    .ADDR_W(2)
)
u_rx_fifo
(
    .clk_i(clk_i),
    .rst_i(rst_i),

    .flush_i(rx_fifo_flush_w),

    .data_in_i(rx_data_w),
    .push_i(rx_push_w),
    .accept_o(rx_accept_w),

    .pop_i(spi_drr_rd_req_w),
    .data_out_o(spi_drr_data_in_w),
    .valid_o(rx_ready_w)
);


assign spi_sr_rx_empty_in_w = ~rx_ready_w;
assign spi_sr_rx_full_in_w  = ~rx_accept_w;

//-----------------------------------------------------------------
// Configuration
//-----------------------------------------------------------------
wire [31:0]     clk_div_w = C_SCK_RATIO;

//-----------------------------------------------------------------
// Registers
//-----------------------------------------------------------------
reg   active_q;
reg [5:0] bit_count_q;
reg [7:0]   shift_reg_q;
reg [31:0]    clk_div_q;
reg   done_q;

// Xilinx placement pragmas:
//synthesis attribute IOB of spi_clk_q is "TRUE"
//synthesis attribute IOB of spi_mosi_q is "TRUE"
//synthesis attribute IOB of spi_cs_o is "TRUE"
reg   spi_clk_q;
reg   spi_mosi_q;

//-----------------------------------------------------------------
// Implementation
//-----------------------------------------------------------------
wire enable_w = spi_cr_spe_out_w & spi_cr_master_out_w & ~spi_cr_trans_inhibit_out_w;

// Something to do, SPI enabled...
wire start_w = enable_w & ~active_q & ~done_q & tx_ready_w;

// Loopback more or normal
wire miso_w = spi_cr_loop_out_w ? spi_mosi_o : spi_miso_i;

// SPI Clock Generator
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    clk_div_q <= 32'd0;
else if (start_w || sw_reset_w || clk_div_q == 32'd0)
    clk_div_q <= clk_div_w;
else
    clk_div_q <= clk_div_q - 32'd1;

wire clk_en_w = (clk_div_q == 32'd0);

//-----------------------------------------------------------------
// Sample, Drive pulse generation
//-----------------------------------------------------------------
reg sample_r;
reg drive_r;

always @ *
begin
    sample_r = 1'b0;
    drive_r  = 1'b0;

    // SPI = IDLE
    if (start_w)    
        drive_r  = ~spi_cr_cpha_out_w; // Drive initial data (CPHA=0)
    // SPI = ACTIVE
    else if (active_q && clk_en_w)
    begin
        // Sample
        // CPHA=0, sample on the first edge
        // CPHA=1, sample on the second edge
        if (bit_count_q[0] == spi_cr_cpha_out_w)
            sample_r = 1'b1;
        // Drive (CPHA = 1)
        else if (spi_cr_cpha_out_w)
            drive_r = 1'b1;
        // Drive (CPHA = 0)
        else 
            drive_r = (bit_count_q != 6'b0) && (bit_count_q != 6'd15);
    end
end

//-----------------------------------------------------------------
// Shift register
//-----------------------------------------------------------------
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
begin
    shift_reg_q    <= 8'b0;
    spi_clk_q      <= 1'b0;
    spi_mosi_q     <= 1'b0;
end
else
begin
    // SPI = RESET (or potentially update CPOL)
    if (sw_reset_w || (spi_cr_wr_req_w & !start_w))
    begin
        shift_reg_q    <= 8'b0;
        spi_clk_q      <= spi_cr_cpol_out_w;
    end
    // SPI = IDLE
    else if (start_w)
    begin
        spi_clk_q      <= spi_cr_cpol_out_w;

        // CPHA = 0
        if (drive_r)
        begin
            spi_mosi_q    <= tx_data_w[7];
            shift_reg_q   <= {tx_data_w[6:0], 1'b0};
        end
        // CPHA = 1
        else
            shift_reg_q   <= tx_data_w;
    end
    // SPI = ACTIVE
    else if (active_q && clk_en_w)
    begin
        // Toggle SPI clock output
        if (!spi_cr_loop_out_w)
            spi_clk_q <= ~spi_clk_q;

        // Drive MOSI
        if (drive_r)
        begin
            spi_mosi_q  <= shift_reg_q[7];
            shift_reg_q <= {shift_reg_q[6:0],1'b0};
        end
        // Sample MISO
        else if (sample_r)
            shift_reg_q[0] <= miso_w;
    end
end

//-----------------------------------------------------------------
// Bit counter
//-----------------------------------------------------------------
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
begin
    bit_count_q    <= 6'b0;
    active_q       <= 1'b0;
    done_q         <= 1'b0;
end
else if (sw_reset_w)
begin
    bit_count_q    <= 6'b0;
    active_q       <= 1'b0;
    done_q         <= 1'b0;
end
else if (start_w)
begin
    bit_count_q    <= 6'b0;
    active_q       <= 1'b1;
    done_q         <= 1'b0;
end
else if (active_q && clk_en_w)
begin
    // End of SPI transfer reached
    if (bit_count_q == 6'd15)
    begin
        // Go back to IDLE active_q
        active_q  <= 1'b0;

        // Set transfer complete flags
        done_q   <= 1'b1;
    end
    // Increment cycle counter
    else 
        bit_count_q <= bit_count_q + 6'd1;
end
else
    done_q         <= 1'b0;

// Delayed done_q for FIFO level check
reg check_tx_level_q;
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    check_tx_level_q <= 1'b0;
else
    check_tx_level_q <= done_q;

// Interrupt
reg intr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    intr_q <= 1'b0;
else if (check_tx_level_q && spi_ipier_tx_empty_out_w && spi_ipisr_tx_empty_in_w)
    intr_q <= 1'b1;
else if (spi_ipisr_wr_req_w && spi_ipisr_tx_empty_out_w)
    intr_q <= 1'b0;

assign spi_ipisr_tx_empty_in_w = spi_sr_tx_empty_in_w;

//-----------------------------------------------------------------
// Assignments
//-----------------------------------------------------------------
assign spi_clk_o            = spi_clk_q;
assign spi_mosi_o           = spi_mosi_q;

// Reverse order if LSB first
assign rx_data_w = spi_cr_lsb_first_out_w ? 
    {
      shift_reg_q[0]
    , shift_reg_q[1]
    , shift_reg_q[2]
    , shift_reg_q[3]
    , shift_reg_q[4]
    , shift_reg_q[5]
    , shift_reg_q[6]
    , shift_reg_q[7]
    } : shift_reg_q;


assign rx_push_w            = done_q;
assign tx_pop_w             = done_q;

assign spi_cs_o             = spi_ssr_value_out_w;
assign intr_o               = spi_dgier_gie_out_w & intr_q;

endmodule

//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------

module spi_lite_fifo
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
    parameter WIDTH   = 8,
    parameter DEPTH   = 4,
    parameter ADDR_W  = 2
)
//-----------------------------------------------------------------
// Ports
//-----------------------------------------------------------------
(
    // Inputs
     input               clk_i
    ,input               rst_i
    ,input  [WIDTH-1:0]  data_in_i
    ,input               push_i
    ,input               pop_i
    ,input               flush_i

    // Outputs
    ,output [WIDTH-1:0]  data_out_o
    ,output              accept_o
    ,output              valid_o
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
//                     Basic Peripheral SoC
//                           V1.1
//                     Ultra-Embedded.com
//                     Copyright 2014-2020
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

`define GPIO_DIRECTION    8'h0

    `define GPIO_DIRECTION_OUTPUT_DEFAULT    0
    `define GPIO_DIRECTION_OUTPUT_B          0
    `define GPIO_DIRECTION_OUTPUT_T          31
    `define GPIO_DIRECTION_OUTPUT_W          32
    `define GPIO_DIRECTION_OUTPUT_R          31:0

`define GPIO_INPUT    8'h4

    `define GPIO_INPUT_VALUE_DEFAULT    0
    `define GPIO_INPUT_VALUE_B          0
    `define GPIO_INPUT_VALUE_T          31
    `define GPIO_INPUT_VALUE_W          32
    `define GPIO_INPUT_VALUE_R          31:0

`define GPIO_OUTPUT    8'h8

    `define GPIO_OUTPUT_DATA_DEFAULT    0
    `define GPIO_OUTPUT_DATA_B          0
    `define GPIO_OUTPUT_DATA_T          31
    `define GPIO_OUTPUT_DATA_W          32
    `define GPIO_OUTPUT_DATA_R          31:0

`define GPIO_OUTPUT_SET    8'hc

    `define GPIO_OUTPUT_SET_DATA_DEFAULT    0
    `define GPIO_OUTPUT_SET_DATA_B          0
    `define GPIO_OUTPUT_SET_DATA_T          31
    `define GPIO_OUTPUT_SET_DATA_W          32
    `define GPIO_OUTPUT_SET_DATA_R          31:0

`define GPIO_OUTPUT_CLR    8'h10

    `define GPIO_OUTPUT_CLR_DATA_DEFAULT    0
    `define GPIO_OUTPUT_CLR_DATA_B          0
    `define GPIO_OUTPUT_CLR_DATA_T          31
    `define GPIO_OUTPUT_CLR_DATA_W          32
    `define GPIO_OUTPUT_CLR_DATA_R          31:0

`define GPIO_INT_MASK    8'h14

    `define GPIO_INT_MASK_ENABLE_DEFAULT    0
    `define GPIO_INT_MASK_ENABLE_B          0
    `define GPIO_INT_MASK_ENABLE_T          31
    `define GPIO_INT_MASK_ENABLE_W          32
    `define GPIO_INT_MASK_ENABLE_R          31:0

`define GPIO_INT_SET    8'h18

    `define GPIO_INT_SET_SW_IRQ_DEFAULT    0
    `define GPIO_INT_SET_SW_IRQ_B          0
    `define GPIO_INT_SET_SW_IRQ_T          31
    `define GPIO_INT_SET_SW_IRQ_W          32
    `define GPIO_INT_SET_SW_IRQ_R          31:0

`define GPIO_INT_CLR    8'h1c

    `define GPIO_INT_CLR_ACK_DEFAULT    0
    `define GPIO_INT_CLR_ACK_B          0
    `define GPIO_INT_CLR_ACK_T          31
    `define GPIO_INT_CLR_ACK_W          32
    `define GPIO_INT_CLR_ACK_R          31:0

`define GPIO_INT_STATUS    8'h20

    `define GPIO_INT_STATUS_RAW_DEFAULT    0
    `define GPIO_INT_STATUS_RAW_B          0
    `define GPIO_INT_STATUS_RAW_T          31
    `define GPIO_INT_STATUS_RAW_W          32
    `define GPIO_INT_STATUS_RAW_R          31:0

`define GPIO_INT_LEVEL    8'h24

    `define GPIO_INT_LEVEL_ACTIVE_HIGH_DEFAULT    0
    `define GPIO_INT_LEVEL_ACTIVE_HIGH_B          0
    `define GPIO_INT_LEVEL_ACTIVE_HIGH_T          31
    `define GPIO_INT_LEVEL_ACTIVE_HIGH_W          32
    `define GPIO_INT_LEVEL_ACTIVE_HIGH_R          31:0

`define GPIO_INT_MODE    8'h28

    `define GPIO_INT_MODE_EDGE_DEFAULT    0
    `define GPIO_INT_MODE_EDGE_B          0
    `define GPIO_INT_MODE_EDGE_T          31
    `define GPIO_INT_MODE_EDGE_W          32
    `define GPIO_INT_MODE_EDGE_R          31:0

//-----------------------------------------------------------------
//                     Basic Peripheral SoC
//                           V1.1
//                     Ultra-Embedded.com
//                     Copyright 2014-2020
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

// `include "gpio_defs.v"

//-----------------------------------------------------------------
// Module:  General Purpose IO Peripheral
//-----------------------------------------------------------------
module gpio
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
    ,input  [31:0]  gpio_input_i

    // Outputs
    ,output         cfg_awready_o
    ,output         cfg_wready_o
    ,output         cfg_bvalid_o
    ,output [1:0]   cfg_bresp_o
    ,output         cfg_arready_o
    ,output         cfg_rvalid_o
    ,output [31:0]  cfg_rdata_o
    ,output [1:0]   cfg_rresp_o
    ,output [31:0]  gpio_output_o
    ,output [31:0]  gpio_output_enable_o
    ,output         intr_o
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
// Register gpio_direction
//-----------------------------------------------------------------
reg gpio_direction_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    gpio_direction_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `GPIO_DIRECTION))
    gpio_direction_wr_q <= 1'b1;
else
    gpio_direction_wr_q <= 1'b0;

// gpio_direction_output [internal]
reg [31:0]  gpio_direction_output_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    gpio_direction_output_q <= 32'd`GPIO_DIRECTION_OUTPUT_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `GPIO_DIRECTION))
    gpio_direction_output_q <= cfg_wdata_i[`GPIO_DIRECTION_OUTPUT_R];

wire [31:0]  gpio_direction_output_out_w = gpio_direction_output_q;


//-----------------------------------------------------------------
// Register gpio_input
//-----------------------------------------------------------------
reg gpio_input_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    gpio_input_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `GPIO_INPUT))
    gpio_input_wr_q <= 1'b1;
else
    gpio_input_wr_q <= 1'b0;


//-----------------------------------------------------------------
// Register gpio_output
//-----------------------------------------------------------------
reg gpio_output_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    gpio_output_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `GPIO_OUTPUT))
    gpio_output_wr_q <= 1'b1;
else
    gpio_output_wr_q <= 1'b0;

// gpio_output_data [external]
wire [31:0]  gpio_output_data_out_w = wr_data_q[`GPIO_OUTPUT_DATA_R];


//-----------------------------------------------------------------
// Register gpio_output_set
//-----------------------------------------------------------------
reg gpio_output_set_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    gpio_output_set_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `GPIO_OUTPUT_SET))
    gpio_output_set_wr_q <= 1'b1;
else
    gpio_output_set_wr_q <= 1'b0;

// gpio_output_set_data [external]
wire [31:0]  gpio_output_set_data_out_w = wr_data_q[`GPIO_OUTPUT_SET_DATA_R];


//-----------------------------------------------------------------
// Register gpio_output_clr
//-----------------------------------------------------------------
reg gpio_output_clr_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    gpio_output_clr_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `GPIO_OUTPUT_CLR))
    gpio_output_clr_wr_q <= 1'b1;
else
    gpio_output_clr_wr_q <= 1'b0;

// gpio_output_clr_data [external]
wire [31:0]  gpio_output_clr_data_out_w = wr_data_q[`GPIO_OUTPUT_CLR_DATA_R];


//-----------------------------------------------------------------
// Register gpio_int_mask
//-----------------------------------------------------------------
reg gpio_int_mask_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    gpio_int_mask_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `GPIO_INT_MASK))
    gpio_int_mask_wr_q <= 1'b1;
else
    gpio_int_mask_wr_q <= 1'b0;

// gpio_int_mask_enable [internal]
reg [31:0]  gpio_int_mask_enable_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    gpio_int_mask_enable_q <= 32'd`GPIO_INT_MASK_ENABLE_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `GPIO_INT_MASK))
    gpio_int_mask_enable_q <= cfg_wdata_i[`GPIO_INT_MASK_ENABLE_R];

wire [31:0]  gpio_int_mask_enable_out_w = gpio_int_mask_enable_q;


//-----------------------------------------------------------------
// Register gpio_int_set
//-----------------------------------------------------------------
reg gpio_int_set_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    gpio_int_set_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `GPIO_INT_SET))
    gpio_int_set_wr_q <= 1'b1;
else
    gpio_int_set_wr_q <= 1'b0;

// gpio_int_set_sw_irq [external]
wire [31:0]  gpio_int_set_sw_irq_out_w = wr_data_q[`GPIO_INT_SET_SW_IRQ_R];


//-----------------------------------------------------------------
// Register gpio_int_clr
//-----------------------------------------------------------------
reg gpio_int_clr_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    gpio_int_clr_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `GPIO_INT_CLR))
    gpio_int_clr_wr_q <= 1'b1;
else
    gpio_int_clr_wr_q <= 1'b0;

// gpio_int_clr_ack [external]
wire [31:0]  gpio_int_clr_ack_out_w = wr_data_q[`GPIO_INT_CLR_ACK_R];


//-----------------------------------------------------------------
// Register gpio_int_status
//-----------------------------------------------------------------
reg gpio_int_status_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    gpio_int_status_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `GPIO_INT_STATUS))
    gpio_int_status_wr_q <= 1'b1;
else
    gpio_int_status_wr_q <= 1'b0;


//-----------------------------------------------------------------
// Register gpio_int_level
//-----------------------------------------------------------------
reg gpio_int_level_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    gpio_int_level_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `GPIO_INT_LEVEL))
    gpio_int_level_wr_q <= 1'b1;
else
    gpio_int_level_wr_q <= 1'b0;

// gpio_int_level_active_high [internal]
reg [31:0]  gpio_int_level_active_high_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    gpio_int_level_active_high_q <= 32'd`GPIO_INT_LEVEL_ACTIVE_HIGH_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `GPIO_INT_LEVEL))
    gpio_int_level_active_high_q <= cfg_wdata_i[`GPIO_INT_LEVEL_ACTIVE_HIGH_R];

wire [31:0]  gpio_int_level_active_high_out_w = gpio_int_level_active_high_q;


//-----------------------------------------------------------------
// Register gpio_int_mode
//-----------------------------------------------------------------
reg gpio_int_mode_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    gpio_int_mode_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `GPIO_INT_MODE))
    gpio_int_mode_wr_q <= 1'b1;
else
    gpio_int_mode_wr_q <= 1'b0;

// gpio_int_mode_edge [internal]
reg [31:0]  gpio_int_mode_edge_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    gpio_int_mode_edge_q <= 32'd`GPIO_INT_MODE_EDGE_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `GPIO_INT_MODE))
    gpio_int_mode_edge_q <= cfg_wdata_i[`GPIO_INT_MODE_EDGE_R];

wire [31:0]  gpio_int_mode_edge_out_w = gpio_int_mode_edge_q;


wire [31:0]  gpio_input_value_in_w;
wire [31:0]  gpio_output_data_in_w;
wire [31:0]  gpio_int_status_raw_in_w;


//-----------------------------------------------------------------
// Read mux
//-----------------------------------------------------------------
reg [31:0] data_r;

always @ *
begin
    data_r = 32'b0;

    case (cfg_araddr_i[7:0])

    `GPIO_DIRECTION:
    begin
        data_r[`GPIO_DIRECTION_OUTPUT_R] = gpio_direction_output_q;
    end
    `GPIO_INPUT:
    begin
        data_r[`GPIO_INPUT_VALUE_R] = gpio_input_value_in_w;
    end
    `GPIO_OUTPUT:
    begin
        data_r[`GPIO_OUTPUT_DATA_R] = gpio_output_data_in_w;
    end
    `GPIO_INT_MASK:
    begin
        data_r[`GPIO_INT_MASK_ENABLE_R] = gpio_int_mask_enable_q;
    end
    `GPIO_INT_STATUS:
    begin
        data_r[`GPIO_INT_STATUS_RAW_R] = gpio_int_status_raw_in_w;
    end
    `GPIO_INT_LEVEL:
    begin
        data_r[`GPIO_INT_LEVEL_ACTIVE_HIGH_R] = gpio_int_level_active_high_q;
    end
    `GPIO_INT_MODE:
    begin
        data_r[`GPIO_INT_MODE_EDGE_R] = gpio_int_mode_edge_q;
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

wire gpio_output_rd_req_w = read_en_w & (cfg_araddr_i[7:0] == `GPIO_OUTPUT);

wire gpio_output_wr_req_w = gpio_output_wr_q;
wire gpio_output_set_wr_req_w = gpio_output_set_wr_q;
wire gpio_output_clr_wr_req_w = gpio_output_clr_wr_q;
wire gpio_int_set_wr_req_w = gpio_int_set_wr_q;
wire gpio_int_clr_wr_req_w = gpio_int_clr_wr_q;


//-----------------------------------------------------------------
// Inputs
//-----------------------------------------------------------------
// Resync inputs
reg [31:0] input_ms;
reg [31:0] input_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
begin
    input_ms <= 32'b0;
    input_q  <= 32'b0;
end
else
begin
    input_q  <= input_ms;
    input_ms <= gpio_input_i;
end

assign gpio_input_value_in_w = input_q;

//-----------------------------------------------------------------
// Outputs
//-----------------------------------------------------------------
reg [31:0] output_q;
reg [31:0] output_next_r;

always @ *
begin
    output_next_r = output_q;

    if (gpio_output_set_wr_req_w)
        output_next_r = output_q | gpio_output_set_data_out_w;
    else if (gpio_output_clr_wr_req_w)
        output_next_r = output_q & ~gpio_output_clr_data_out_w;
    else if (gpio_output_wr_req_w)
        output_next_r = gpio_output_data_out_w;
end

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    output_q  <= 32'b0;
else
    output_q  <= output_next_r;

assign gpio_output_data_in_w = output_q;
assign gpio_output_o         = output_q;

//-----------------------------------------------------------------
// Interrupts
//-----------------------------------------------------------------
reg intr_q;

reg [31:0] interrupt_raw_q;
reg [31:0] interrupt_raw_r;

reg [31:0] input_last_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    input_last_q  <= 32'b0;
else
    input_last_q  <= gpio_input_value_in_w;

wire [31:0] active_low_w   = (~gpio_int_level_active_high_out_w);
wire [31:0] falling_edge_w =  (~gpio_int_level_active_high_out_w);
wire [31:0] level_mask_w   = (~gpio_int_mode_edge_out_w);
wire [31:0] edge_mask_w    =   gpio_int_mode_edge_out_w;

wire [31:0] level_active_w = (gpio_input_value_in_w ^ active_low_w) & level_mask_w;

wire [31:0] edge_detect_w  = (input_last_q ^ gpio_input_value_in_w);
wire [31:0] edge_active_w  = (edge_detect_w & (gpio_input_value_in_w ^ falling_edge_w)) & edge_mask_w;

reg [31:0] interrupt_level_r;
always @ *
begin
    interrupt_raw_r = interrupt_raw_q;

    // Clear (ACK)
    if (gpio_int_clr_wr_req_w)
        interrupt_raw_r = interrupt_raw_r & ~gpio_int_clr_ack_out_w;

    // New interrupts
    interrupt_raw_r = interrupt_raw_r | level_active_w | edge_active_w;

    // Set (SW IRQ)
    if (gpio_int_set_wr_req_w)
        interrupt_raw_r = interrupt_raw_r | gpio_int_set_sw_irq_out_w;
end

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
begin
    intr_q <= 1'b0;
    interrupt_raw_q  <= 32'b0;
end
else
begin
    intr_q           <= |(interrupt_raw_q & gpio_int_mask_enable_out_w);
    interrupt_raw_q  <= interrupt_raw_r;
end

assign gpio_int_status_raw_in_w = interrupt_raw_q;
assign intr_o                   = intr_q;

//-----------------------------------------------------------------
// Assignments
//-----------------------------------------------------------------
assign gpio_output_enable_o  = gpio_direction_output_out_w;


endmodule
//-----------------------------------------------------------------
//                     Basic Peripheral SoC
//                           V1.1
//                     Ultra-Embedded.com
//                     Copyright 2014-2020
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

`define TIMER_CTRL0    8'h8

    `define TIMER_CTRL0_INTERRUPT      1
    `define TIMER_CTRL0_INTERRUPT_DEFAULT    0
    `define TIMER_CTRL0_INTERRUPT_B          1
    `define TIMER_CTRL0_INTERRUPT_T          1
    `define TIMER_CTRL0_INTERRUPT_W          1
    `define TIMER_CTRL0_INTERRUPT_R          1:1

    `define TIMER_CTRL0_ENABLE      2
    `define TIMER_CTRL0_ENABLE_DEFAULT    0
    `define TIMER_CTRL0_ENABLE_B          2
    `define TIMER_CTRL0_ENABLE_T          2
    `define TIMER_CTRL0_ENABLE_W          1
    `define TIMER_CTRL0_ENABLE_R          2:2

`define TIMER_CMP0    8'hc

    `define TIMER_CMP0_VALUE_DEFAULT    0
    `define TIMER_CMP0_VALUE_B          0
    `define TIMER_CMP0_VALUE_T          31
    `define TIMER_CMP0_VALUE_W          32
    `define TIMER_CMP0_VALUE_R          31:0

`define TIMER_VAL0    8'h10

    `define TIMER_VAL0_CURRENT_DEFAULT    0
    `define TIMER_VAL0_CURRENT_B          0
    `define TIMER_VAL0_CURRENT_T          31
    `define TIMER_VAL0_CURRENT_W          32
    `define TIMER_VAL0_CURRENT_R          31:0

`define TIMER_CTRL1    8'h14

    `define TIMER_CTRL1_INTERRUPT      1
    `define TIMER_CTRL1_INTERRUPT_DEFAULT    0
    `define TIMER_CTRL1_INTERRUPT_B          1
    `define TIMER_CTRL1_INTERRUPT_T          1
    `define TIMER_CTRL1_INTERRUPT_W          1
    `define TIMER_CTRL1_INTERRUPT_R          1:1

    `define TIMER_CTRL1_ENABLE      2
    `define TIMER_CTRL1_ENABLE_DEFAULT    0
    `define TIMER_CTRL1_ENABLE_B          2
    `define TIMER_CTRL1_ENABLE_T          2
    `define TIMER_CTRL1_ENABLE_W          1
    `define TIMER_CTRL1_ENABLE_R          2:2

`define TIMER_CMP1    8'h18

    `define TIMER_CMP1_VALUE_DEFAULT    0
    `define TIMER_CMP1_VALUE_B          0
    `define TIMER_CMP1_VALUE_T          31
    `define TIMER_CMP1_VALUE_W          32
    `define TIMER_CMP1_VALUE_R          31:0

`define TIMER_VAL1    8'h1c

    `define TIMER_VAL1_CURRENT_DEFAULT    0
    `define TIMER_VAL1_CURRENT_B          0
    `define TIMER_VAL1_CURRENT_T          31
    `define TIMER_VAL1_CURRENT_W          32
    `define TIMER_VAL1_CURRENT_R          31:0

//-----------------------------------------------------------------
//                     Basic Peripheral SoC
//                           V1.1
//                     Ultra-Embedded.com
//                     Copyright 2014-2020
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

// `include "timer_defs.v"

//-----------------------------------------------------------------
// Module:  System Tick Timer
//-----------------------------------------------------------------
module timer
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
    ,output         intr_o
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
// Register timer_ctrl0
//-----------------------------------------------------------------
reg timer_ctrl0_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    timer_ctrl0_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `TIMER_CTRL0))
    timer_ctrl0_wr_q <= 1'b1;
else
    timer_ctrl0_wr_q <= 1'b0;

// timer_ctrl0_interrupt [internal]
reg        timer_ctrl0_interrupt_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    timer_ctrl0_interrupt_q <= 1'd`TIMER_CTRL0_INTERRUPT_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `TIMER_CTRL0))
    timer_ctrl0_interrupt_q <= cfg_wdata_i[`TIMER_CTRL0_INTERRUPT_R];

wire        timer_ctrl0_interrupt_out_w = timer_ctrl0_interrupt_q;


// timer_ctrl0_enable [internal]
reg        timer_ctrl0_enable_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    timer_ctrl0_enable_q <= 1'd`TIMER_CTRL0_ENABLE_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `TIMER_CTRL0))
    timer_ctrl0_enable_q <= cfg_wdata_i[`TIMER_CTRL0_ENABLE_R];

wire        timer_ctrl0_enable_out_w = timer_ctrl0_enable_q;


//-----------------------------------------------------------------
// Register timer_cmp0
//-----------------------------------------------------------------
reg timer_cmp0_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    timer_cmp0_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `TIMER_CMP0))
    timer_cmp0_wr_q <= 1'b1;
else
    timer_cmp0_wr_q <= 1'b0;

// timer_cmp0_value [internal]
reg [31:0]  timer_cmp0_value_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    timer_cmp0_value_q <= 32'd`TIMER_CMP0_VALUE_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `TIMER_CMP0))
    timer_cmp0_value_q <= cfg_wdata_i[`TIMER_CMP0_VALUE_R];

wire [31:0]  timer_cmp0_value_out_w = timer_cmp0_value_q;


//-----------------------------------------------------------------
// Register timer_val0
//-----------------------------------------------------------------
reg timer_val0_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    timer_val0_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `TIMER_VAL0))
    timer_val0_wr_q <= 1'b1;
else
    timer_val0_wr_q <= 1'b0;

// timer_val0_current [external]
wire [31:0]  timer_val0_current_out_w = wr_data_q[`TIMER_VAL0_CURRENT_R];


//-----------------------------------------------------------------
// Register timer_ctrl1
//-----------------------------------------------------------------
reg timer_ctrl1_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    timer_ctrl1_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `TIMER_CTRL1))
    timer_ctrl1_wr_q <= 1'b1;
else
    timer_ctrl1_wr_q <= 1'b0;

// timer_ctrl1_interrupt [internal]
reg        timer_ctrl1_interrupt_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    timer_ctrl1_interrupt_q <= 1'd`TIMER_CTRL1_INTERRUPT_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `TIMER_CTRL1))
    timer_ctrl1_interrupt_q <= cfg_wdata_i[`TIMER_CTRL1_INTERRUPT_R];

wire        timer_ctrl1_interrupt_out_w = timer_ctrl1_interrupt_q;


// timer_ctrl1_enable [internal]
reg        timer_ctrl1_enable_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    timer_ctrl1_enable_q <= 1'd`TIMER_CTRL1_ENABLE_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `TIMER_CTRL1))
    timer_ctrl1_enable_q <= cfg_wdata_i[`TIMER_CTRL1_ENABLE_R];

wire        timer_ctrl1_enable_out_w = timer_ctrl1_enable_q;


//-----------------------------------------------------------------
// Register timer_cmp1
//-----------------------------------------------------------------
reg timer_cmp1_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    timer_cmp1_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `TIMER_CMP1))
    timer_cmp1_wr_q <= 1'b1;
else
    timer_cmp1_wr_q <= 1'b0;

// timer_cmp1_value [internal]
reg [31:0]  timer_cmp1_value_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    timer_cmp1_value_q <= 32'd`TIMER_CMP1_VALUE_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `TIMER_CMP1))
    timer_cmp1_value_q <= cfg_wdata_i[`TIMER_CMP1_VALUE_R];

wire [31:0]  timer_cmp1_value_out_w = timer_cmp1_value_q;


//-----------------------------------------------------------------
// Register timer_val1
//-----------------------------------------------------------------
reg timer_val1_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    timer_val1_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `TIMER_VAL1))
    timer_val1_wr_q <= 1'b1;
else
    timer_val1_wr_q <= 1'b0;

// timer_val1_current [external]
wire [31:0]  timer_val1_current_out_w = wr_data_q[`TIMER_VAL1_CURRENT_R];


wire [31:0]  timer_val0_current_in_w;
wire [31:0]  timer_val1_current_in_w;


//-----------------------------------------------------------------
// Read mux
//-----------------------------------------------------------------
reg [31:0] data_r;

always @ *
begin
    data_r = 32'b0;

    case (cfg_araddr_i[7:0])

    `TIMER_CTRL0:
    begin
        data_r[`TIMER_CTRL0_INTERRUPT_R] = timer_ctrl0_interrupt_q;
        data_r[`TIMER_CTRL0_ENABLE_R] = timer_ctrl0_enable_q;
    end
    `TIMER_CMP0:
    begin
        data_r[`TIMER_CMP0_VALUE_R] = timer_cmp0_value_q;
    end
    `TIMER_VAL0:
    begin
        data_r[`TIMER_VAL0_CURRENT_R] = timer_val0_current_in_w;
    end
    `TIMER_CTRL1:
    begin
        data_r[`TIMER_CTRL1_INTERRUPT_R] = timer_ctrl1_interrupt_q;
        data_r[`TIMER_CTRL1_ENABLE_R] = timer_ctrl1_enable_q;
    end
    `TIMER_CMP1:
    begin
        data_r[`TIMER_CMP1_VALUE_R] = timer_cmp1_value_q;
    end
    `TIMER_VAL1:
    begin
        data_r[`TIMER_VAL1_CURRENT_R] = timer_val1_current_in_w;
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


wire timer_val0_wr_req_w = timer_val0_wr_q;
wire timer_val1_wr_req_w = timer_val1_wr_q;

//-----------------------------------------------------------------
// Timer0
//-----------------------------------------------------------------
reg [31:0] timer0_value_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    timer0_value_q <= 32'b0;
else if (timer_val0_wr_req_w)
    timer0_value_q <= timer_val0_current_out_w;
else if (timer_ctrl0_enable_out_w)
    timer0_value_q <= timer0_value_q + 32'd1;

assign timer_val0_current_in_w = timer0_value_q;

wire timer0_irq_w = (timer_val0_current_in_w == timer_cmp0_value_out_w) && timer_ctrl0_interrupt_out_w && timer_ctrl0_enable_out_w;

//-----------------------------------------------------------------
// Timer1
//-----------------------------------------------------------------
reg [31:0] timer1_value_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    timer1_value_q <= 32'b0;
else if (timer_val1_wr_req_w)
    timer1_value_q <= timer_val1_current_out_w;
else if (timer_ctrl1_enable_out_w)
    timer1_value_q <= timer1_value_q + 32'd1;

assign timer_val1_current_in_w = timer1_value_q;

wire timer1_irq_w = (timer_val1_current_in_w == timer_cmp1_value_out_w) && timer_ctrl1_interrupt_out_w && timer_ctrl1_enable_out_w;


//-----------------------------------------------------------------
// IRQ output
//-----------------------------------------------------------------
reg intr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    intr_q <= 1'b0;
else if (1'b0
        | timer0_irq_w
        | timer1_irq_w
)
    intr_q <= 1'b1;
else
    intr_q <= 1'b0;

//-----------------------------------------------------------------
// Assignments
//-----------------------------------------------------------------
assign intr_o = intr_q;



endmodule
//-----------------------------------------------------------------
//                     Basic Peripheral SoC
//                           V1.1
//                     Ultra-Embedded.com
//                     Copyright 2014-2020
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

`define IRQ_ISR    8'h0

    `define IRQ_ISR_STATUS_DEFAULT    0
    `define IRQ_ISR_STATUS_B          0
    `define IRQ_ISR_STATUS_T          6
    `define IRQ_ISR_STATUS_W          7
    `define IRQ_ISR_STATUS_R          6:0

`define IRQ_IPR    8'h4

    `define IRQ_IPR_PENDING_DEFAULT    0
    `define IRQ_IPR_PENDING_B          0
    `define IRQ_IPR_PENDING_T          6
    `define IRQ_IPR_PENDING_W          7
    `define IRQ_IPR_PENDING_R          6:0

`define IRQ_IER    8'h8

    `define IRQ_IER_ENABLE_DEFAULT    0
    `define IRQ_IER_ENABLE_B          0
    `define IRQ_IER_ENABLE_T          6
    `define IRQ_IER_ENABLE_W          7
    `define IRQ_IER_ENABLE_R          6:0

`define IRQ_IAR    8'hc

    `define IRQ_IAR_ACK_DEFAULT    0
    `define IRQ_IAR_ACK_B          0
    `define IRQ_IAR_ACK_T          6
    `define IRQ_IAR_ACK_W          7
    `define IRQ_IAR_ACK_R          6:0

`define IRQ_SIE    8'h10

    `define IRQ_SIE_SET_DEFAULT    0
    `define IRQ_SIE_SET_B          0
    `define IRQ_SIE_SET_T          6
    `define IRQ_SIE_SET_W          7
    `define IRQ_SIE_SET_R          6:0

`define IRQ_CIE    8'h14

    `define IRQ_CIE_CLR_DEFAULT    0
    `define IRQ_CIE_CLR_B          0
    `define IRQ_CIE_CLR_T          6
    `define IRQ_CIE_CLR_W          7
    `define IRQ_CIE_CLR_R          6:0

`define IRQ_IVR    8'h18

    `define IRQ_IVR_VECTOR_DEFAULT    0
    `define IRQ_IVR_VECTOR_B          0
    `define IRQ_IVR_VECTOR_T          31
    `define IRQ_IVR_VECTOR_W          32
    `define IRQ_IVR_VECTOR_R          31:0

`define IRQ_MER    8'h1c

    `define IRQ_MER_ME      0
    `define IRQ_MER_ME_DEFAULT    0
    `define IRQ_MER_ME_B          0
    `define IRQ_MER_ME_T          0
    `define IRQ_MER_ME_W          1
    `define IRQ_MER_ME_R          0:0

//-----------------------------------------------------------------
//                     Basic Peripheral SoC
//                           V1.1
//                     Ultra-Embedded.com
//                     Copyright 2014-2020
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

// `include "irq_ctrl_defs.v"

//-----------------------------------------------------------------
// Module:  IRQ Controller
//-----------------------------------------------------------------
module irq_ctrl
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
    ,input          interrupt0_i
    ,input          interrupt1_i
    ,input          interrupt2_i
    ,input          interrupt3_i
    ,input          interrupt4_i
    ,input          interrupt5_i
    ,input          interrupt6_i

    // Outputs
    ,output         cfg_awready_o
    ,output         cfg_wready_o
    ,output         cfg_bvalid_o
    ,output [1:0]   cfg_bresp_o
    ,output         cfg_arready_o
    ,output         cfg_rvalid_o
    ,output [31:0]  cfg_rdata_o
    ,output [1:0]   cfg_rresp_o
    ,output         intr_o
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
// Register irq_isr
//-----------------------------------------------------------------
reg irq_isr_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    irq_isr_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `IRQ_ISR))
    irq_isr_wr_q <= 1'b1;
else
    irq_isr_wr_q <= 1'b0;

// irq_isr_status [external]
wire [6:0]  irq_isr_status_out_w = wr_data_q[`IRQ_ISR_STATUS_R];


//-----------------------------------------------------------------
// Register irq_ipr
//-----------------------------------------------------------------
reg irq_ipr_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    irq_ipr_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `IRQ_IPR))
    irq_ipr_wr_q <= 1'b1;
else
    irq_ipr_wr_q <= 1'b0;


//-----------------------------------------------------------------
// Register irq_ier
//-----------------------------------------------------------------
reg irq_ier_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    irq_ier_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `IRQ_IER))
    irq_ier_wr_q <= 1'b1;
else
    irq_ier_wr_q <= 1'b0;

// irq_ier_enable [external]
wire [6:0]  irq_ier_enable_out_w = wr_data_q[`IRQ_IER_ENABLE_R];


//-----------------------------------------------------------------
// Register irq_iar
//-----------------------------------------------------------------
reg irq_iar_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    irq_iar_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `IRQ_IAR))
    irq_iar_wr_q <= 1'b1;
else
    irq_iar_wr_q <= 1'b0;

// irq_iar_ack [auto_clr]
reg [6:0]  irq_iar_ack_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    irq_iar_ack_q <= 7'd`IRQ_IAR_ACK_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `IRQ_IAR))
    irq_iar_ack_q <= cfg_wdata_i[`IRQ_IAR_ACK_R];
else
    irq_iar_ack_q <= 7'd`IRQ_IAR_ACK_DEFAULT;

wire [6:0]  irq_iar_ack_out_w = irq_iar_ack_q;


//-----------------------------------------------------------------
// Register irq_sie
//-----------------------------------------------------------------
reg irq_sie_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    irq_sie_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `IRQ_SIE))
    irq_sie_wr_q <= 1'b1;
else
    irq_sie_wr_q <= 1'b0;

// irq_sie_set [external]
wire [6:0]  irq_sie_set_out_w = wr_data_q[`IRQ_SIE_SET_R];


//-----------------------------------------------------------------
// Register irq_cie
//-----------------------------------------------------------------
reg irq_cie_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    irq_cie_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `IRQ_CIE))
    irq_cie_wr_q <= 1'b1;
else
    irq_cie_wr_q <= 1'b0;

// irq_cie_clr [external]
wire [6:0]  irq_cie_clr_out_w = wr_data_q[`IRQ_CIE_CLR_R];


//-----------------------------------------------------------------
// Register irq_ivr
//-----------------------------------------------------------------
reg irq_ivr_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    irq_ivr_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `IRQ_IVR))
    irq_ivr_wr_q <= 1'b1;
else
    irq_ivr_wr_q <= 1'b0;

// irq_ivr_vector [external]
wire [31:0]  irq_ivr_vector_out_w = wr_data_q[`IRQ_IVR_VECTOR_R];


//-----------------------------------------------------------------
// Register irq_mer
//-----------------------------------------------------------------
reg irq_mer_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    irq_mer_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `IRQ_MER))
    irq_mer_wr_q <= 1'b1;
else
    irq_mer_wr_q <= 1'b0;

// irq_mer_me [internal]
reg        irq_mer_me_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    irq_mer_me_q <= 1'd`IRQ_MER_ME_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `IRQ_MER))
    irq_mer_me_q <= cfg_wdata_i[`IRQ_MER_ME_R];

wire        irq_mer_me_out_w = irq_mer_me_q;


wire [6:0]  irq_isr_status_in_w;
wire [6:0]  irq_ipr_pending_in_w;
wire [6:0]  irq_ier_enable_in_w;
wire [31:0]  irq_ivr_vector_in_w;


//-----------------------------------------------------------------
// Read mux
//-----------------------------------------------------------------
reg [31:0] data_r;

always @ *
begin
    data_r = 32'b0;

    case (cfg_araddr_i[7:0])

    `IRQ_ISR:
    begin
        data_r[`IRQ_ISR_STATUS_R] = irq_isr_status_in_w;
    end
    `IRQ_IPR:
    begin
        data_r[`IRQ_IPR_PENDING_R] = irq_ipr_pending_in_w;
    end
    `IRQ_IER:
    begin
        data_r[`IRQ_IER_ENABLE_R] = irq_ier_enable_in_w;
    end
    `IRQ_IVR:
    begin
        data_r[`IRQ_IVR_VECTOR_R] = irq_ivr_vector_in_w;
    end
    `IRQ_MER:
    begin
        data_r[`IRQ_MER_ME_R] = irq_mer_me_q;
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


wire irq_isr_wr_req_w = irq_isr_wr_q;
wire irq_ier_wr_req_w = irq_ier_wr_q;
wire irq_sie_wr_req_w = irq_sie_wr_q;
wire irq_cie_wr_req_w = irq_cie_wr_q;
wire irq_ivr_wr_req_w = irq_ivr_wr_q;

wire [6:0] irq_input_w;

irq_ctrl_resync
u_irq0_sync
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.async_i(interrupt0_i)
    ,.sync_o(irq_input_w[0])
);

irq_ctrl_resync
u_irq1_sync
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.async_i(interrupt1_i)
    ,.sync_o(irq_input_w[1])
);

irq_ctrl_resync
u_irq2_sync
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.async_i(interrupt2_i)
    ,.sync_o(irq_input_w[2])
);

irq_ctrl_resync
u_irq3_sync
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.async_i(interrupt3_i)
    ,.sync_o(irq_input_w[3])
);

irq_ctrl_resync
u_irq4_sync
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.async_i(interrupt4_i)
    ,.sync_o(irq_input_w[4])
);

irq_ctrl_resync
u_irq5_sync
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.async_i(interrupt5_i)
    ,.sync_o(irq_input_w[5])
);

irq_ctrl_resync
u_irq6_sync
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.async_i(interrupt6_i)
    ,.sync_o(irq_input_w[6])
);


//-----------------------------------------------------------------
// IRQ Enable
//-----------------------------------------------------------------
reg [6:0] irq_enable_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    irq_enable_q <= 7'b0;
else if (irq_ier_wr_req_w)
    irq_enable_q <= irq_ier_enable_out_w;
else if (irq_sie_wr_req_w)
    irq_enable_q <= irq_enable_q | irq_sie_set_out_w;
else if (irq_cie_wr_req_w)
    irq_enable_q <= irq_enable_q & ~irq_cie_clr_out_w;

assign irq_ier_enable_in_w = irq_enable_q;

//-----------------------------------------------------------------
// IRQ Pending
//-----------------------------------------------------------------
reg [6:0] irq_pending_q;

wire [6:0] irq_sw_int_w = {7{irq_isr_wr_req_w}} & irq_isr_status_out_w;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    irq_pending_q <= 7'b0;
else
    irq_pending_q <= irq_input_w | irq_sw_int_w | (irq_pending_q & ~irq_iar_ack_out_w);

assign irq_isr_status_in_w  = irq_pending_q;
assign irq_ipr_pending_in_w = irq_pending_q & irq_enable_q;

//-----------------------------------------------------------------
// IRQ Vector
//-----------------------------------------------------------------
reg [31:0] ivr_vector_r;
always @ *
begin
    ivr_vector_r = 32'hffffffff;

    if (irq_ipr_pending_in_w[0])
        ivr_vector_r = 32'd0;
    else
    if (irq_ipr_pending_in_w[1])
        ivr_vector_r = 32'd1;
    else
    if (irq_ipr_pending_in_w[2])
        ivr_vector_r = 32'd2;
    else
    if (irq_ipr_pending_in_w[3])
        ivr_vector_r = 32'd3;
    else
    if (irq_ipr_pending_in_w[4])
        ivr_vector_r = 32'd4;
    else
    if (irq_ipr_pending_in_w[5])
        ivr_vector_r = 32'd5;
    else
    if (irq_ipr_pending_in_w[6])
        ivr_vector_r = 32'd6;
    else
        ivr_vector_r = 32'hffffffff;
end

assign irq_ivr_vector_in_w = ivr_vector_r;

//-----------------------------------------------------------------
// IRQ output
//-----------------------------------------------------------------
reg intr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    intr_q <= 1'b0;
else
    intr_q <= irq_mer_me_out_w ? (|irq_ipr_pending_in_w) : 1'b0;

//-----------------------------------------------------------------
// Assignments
//-----------------------------------------------------------------
assign intr_o = intr_q;

endmodule

module irq_ctrl_resync
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

reg sync_ms;
reg sync_q;

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
//                     Basic Peripheral SoC
//                           V1.1
//                     Ultra-Embedded.com
//                     Copyright 2014-2020
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

`define ULITE_RX    8'h0

    `define ULITE_RX_DATA_DEFAULT    0
    `define ULITE_RX_DATA_B          0
    `define ULITE_RX_DATA_T          7
    `define ULITE_RX_DATA_W          8
    `define ULITE_RX_DATA_R          7:0

`define ULITE_TX    8'h4

    `define ULITE_TX_DATA_DEFAULT    0
    `define ULITE_TX_DATA_B          0
    `define ULITE_TX_DATA_T          7
    `define ULITE_TX_DATA_W          8
    `define ULITE_TX_DATA_R          7:0

`define ULITE_STATUS    8'h8

    `define ULITE_STATUS_IE      4
    `define ULITE_STATUS_IE_DEFAULT    0
    `define ULITE_STATUS_IE_B          4
    `define ULITE_STATUS_IE_T          4
    `define ULITE_STATUS_IE_W          1
    `define ULITE_STATUS_IE_R          4:4

    `define ULITE_STATUS_TXFULL      3
    `define ULITE_STATUS_TXFULL_DEFAULT    0
    `define ULITE_STATUS_TXFULL_B          3
    `define ULITE_STATUS_TXFULL_T          3
    `define ULITE_STATUS_TXFULL_W          1
    `define ULITE_STATUS_TXFULL_R          3:3

    `define ULITE_STATUS_TXEMPTY      2
    `define ULITE_STATUS_TXEMPTY_DEFAULT    0
    `define ULITE_STATUS_TXEMPTY_B          2
    `define ULITE_STATUS_TXEMPTY_T          2
    `define ULITE_STATUS_TXEMPTY_W          1
    `define ULITE_STATUS_TXEMPTY_R          2:2

    `define ULITE_STATUS_RXFULL      1
    `define ULITE_STATUS_RXFULL_DEFAULT    0
    `define ULITE_STATUS_RXFULL_B          1
    `define ULITE_STATUS_RXFULL_T          1
    `define ULITE_STATUS_RXFULL_W          1
    `define ULITE_STATUS_RXFULL_R          1:1

    `define ULITE_STATUS_RXVALID      0
    `define ULITE_STATUS_RXVALID_DEFAULT    0
    `define ULITE_STATUS_RXVALID_B          0
    `define ULITE_STATUS_RXVALID_T          0
    `define ULITE_STATUS_RXVALID_W          1
    `define ULITE_STATUS_RXVALID_R          0:0

`define ULITE_CONTROL    8'hc

    `define ULITE_CONTROL_IE      4
    `define ULITE_CONTROL_IE_DEFAULT    0
    `define ULITE_CONTROL_IE_B          4
    `define ULITE_CONTROL_IE_T          4
    `define ULITE_CONTROL_IE_W          1
    `define ULITE_CONTROL_IE_R          4:4

    `define ULITE_CONTROL_RST_RX      1
    `define ULITE_CONTROL_RST_RX_DEFAULT    0
    `define ULITE_CONTROL_RST_RX_B          1
    `define ULITE_CONTROL_RST_RX_T          1
    `define ULITE_CONTROL_RST_RX_W          1
    `define ULITE_CONTROL_RST_RX_R          1:1

    `define ULITE_CONTROL_RST_TX      0
    `define ULITE_CONTROL_RST_TX_DEFAULT    0
    `define ULITE_CONTROL_RST_TX_B          0
    `define ULITE_CONTROL_RST_TX_T          0
    `define ULITE_CONTROL_RST_TX_W          1
    `define ULITE_CONTROL_RST_TX_R          0:0

//-----------------------------------------------------------------
//                     Basic Peripheral SoC
//                           V1.1
//                     Ultra-Embedded.com
//                     Copyright 2014-2020
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

// `include "uart_lite_defs.v"

//-----------------------------------------------------------------
// Module:  UART (uartlite compatable)
//-----------------------------------------------------------------
module uart_lite
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
     parameter CLK_FREQ         = 1843200
    ,parameter BAUDRATE         = 115200
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
    ,input          rx_i

    // Outputs
    ,output         cfg_awready_o
    ,output         cfg_wready_o
    ,output         cfg_bvalid_o
    ,output [1:0]   cfg_bresp_o
    ,output         cfg_arready_o
    ,output         cfg_rvalid_o
    ,output [31:0]  cfg_rdata_o
    ,output [1:0]   cfg_rresp_o
    ,output         tx_o
    ,output         intr_o
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
// Register ulite_rx
//-----------------------------------------------------------------
reg ulite_rx_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    ulite_rx_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `ULITE_RX))
    ulite_rx_wr_q <= 1'b1;
else
    ulite_rx_wr_q <= 1'b0;


//-----------------------------------------------------------------
// Register ulite_tx
//-----------------------------------------------------------------
reg ulite_tx_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    ulite_tx_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `ULITE_TX))
    ulite_tx_wr_q <= 1'b1;
else
    ulite_tx_wr_q <= 1'b0;

// ulite_tx_data [external]
wire [7:0]  ulite_tx_data_out_w = wr_data_q[`ULITE_TX_DATA_R];


//-----------------------------------------------------------------
// Register ulite_status
//-----------------------------------------------------------------
reg ulite_status_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    ulite_status_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `ULITE_STATUS))
    ulite_status_wr_q <= 1'b1;
else
    ulite_status_wr_q <= 1'b0;






//-----------------------------------------------------------------
// Register ulite_control
//-----------------------------------------------------------------
reg ulite_control_wr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    ulite_control_wr_q <= 1'b0;
else if (write_en_w && (wr_addr_w[7:0] == `ULITE_CONTROL))
    ulite_control_wr_q <= 1'b1;
else
    ulite_control_wr_q <= 1'b0;

// ulite_control_ie [internal]
reg        ulite_control_ie_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    ulite_control_ie_q <= 1'd`ULITE_CONTROL_IE_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `ULITE_CONTROL))
    ulite_control_ie_q <= cfg_wdata_i[`ULITE_CONTROL_IE_R];

wire        ulite_control_ie_out_w = ulite_control_ie_q;


// ulite_control_rst_rx [auto_clr]
reg        ulite_control_rst_rx_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    ulite_control_rst_rx_q <= 1'd`ULITE_CONTROL_RST_RX_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `ULITE_CONTROL))
    ulite_control_rst_rx_q <= cfg_wdata_i[`ULITE_CONTROL_RST_RX_R];
else
    ulite_control_rst_rx_q <= 1'd`ULITE_CONTROL_RST_RX_DEFAULT;

wire        ulite_control_rst_rx_out_w = ulite_control_rst_rx_q;


// ulite_control_rst_tx [auto_clr]
reg        ulite_control_rst_tx_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    ulite_control_rst_tx_q <= 1'd`ULITE_CONTROL_RST_TX_DEFAULT;
else if (write_en_w && (wr_addr_w[7:0] == `ULITE_CONTROL))
    ulite_control_rst_tx_q <= cfg_wdata_i[`ULITE_CONTROL_RST_TX_R];
else
    ulite_control_rst_tx_q <= 1'd`ULITE_CONTROL_RST_TX_DEFAULT;

wire        ulite_control_rst_tx_out_w = ulite_control_rst_tx_q;


wire [7:0]  ulite_rx_data_in_w;
wire        ulite_status_ie_in_w;
wire        ulite_status_txfull_in_w;
wire        ulite_status_txempty_in_w;
wire        ulite_status_rxfull_in_w;
wire        ulite_status_rxvalid_in_w;


//-----------------------------------------------------------------
// Read mux
//-----------------------------------------------------------------
reg [31:0] data_r;

always @ *
begin
    data_r = 32'b0;

    case (cfg_araddr_i[7:0])

    `ULITE_RX:
    begin
        data_r[`ULITE_RX_DATA_R] = ulite_rx_data_in_w;
    end
    `ULITE_STATUS:
    begin
        data_r[`ULITE_STATUS_IE_R] = ulite_status_ie_in_w;
        data_r[`ULITE_STATUS_TXFULL_R] = ulite_status_txfull_in_w;
        data_r[`ULITE_STATUS_TXEMPTY_R] = ulite_status_txempty_in_w;
        data_r[`ULITE_STATUS_RXFULL_R] = ulite_status_rxfull_in_w;
        data_r[`ULITE_STATUS_RXVALID_R] = ulite_status_rxvalid_in_w;
    end
    `ULITE_CONTROL:
    begin
        data_r[`ULITE_CONTROL_IE_R] = ulite_control_ie_q;
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

wire ulite_rx_rd_req_w = read_en_w & (cfg_araddr_i[7:0] == `ULITE_RX);

wire ulite_rx_wr_req_w = ulite_rx_wr_q;
wire ulite_tx_wr_req_w = ulite_tx_wr_q;

//-----------------------------------------------------------------
// Registers
//-----------------------------------------------------------------

// Configuration
localparam   STOP_BITS = 1'b0; // 0 = 1, 1 = 2
localparam   BIT_DIV   = (CLK_FREQ / BAUDRATE) - 1;

localparam   START_BIT = 4'd0;
localparam   STOP_BIT0 = 4'd9;
localparam   STOP_BIT1 = 4'd10;

// Xilinx placement pragmas:
//synthesis attribute IOB of txd_q is "TRUE"

// TX Signals
reg          tx_busy_q;
reg [3:0]    tx_bits_q;
reg [31:0]   tx_count_q;
reg [7:0]    tx_shift_reg_q;
reg          txd_q;

// RX Signals
reg          rxd_q;
reg [7:0]    rx_data_q;
reg [3:0]    rx_bits_q;
reg [31:0]   rx_count_q;
reg [7:0]    rx_shift_reg_q;
reg          rx_ready_q;
reg          rx_busy_q;

reg          rx_err_q;

//-----------------------------------------------------------------
// Re-sync RXD
//-----------------------------------------------------------------
reg rxd_ms_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
begin
   rxd_ms_q <= 1'b1;
   rxd_q    <= 1'b1;
end
else
begin
   rxd_ms_q <= rx_i;
   rxd_q    <= rxd_ms_q;
end

//-----------------------------------------------------------------
// RX Clock Divider
//-----------------------------------------------------------------
wire rx_sample_w = (rx_count_q == 32'b0);

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    rx_count_q        <= 32'b0;
else
begin
    // Inactive
    if (!rx_busy_q)
        rx_count_q    <= {1'b0, BIT_DIV[31:1]};
    // Rx bit timer
    else if (rx_count_q != 0)
        rx_count_q    <= (rx_count_q - 1);
    // Active
    else if (rx_sample_w)
    begin
        // Last bit?
        if ((rx_bits_q == STOP_BIT0 && !STOP_BITS) || (rx_bits_q == STOP_BIT1 && STOP_BITS))
            rx_count_q    <= 32'b0;
        else
            rx_count_q    <= BIT_DIV;
    end
end

//-----------------------------------------------------------------
// RX Shift Register
//-----------------------------------------------------------------
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
begin
    rx_shift_reg_q <= 8'h00;
    rx_busy_q      <= 1'b0;
end
// Rx busy
else if (rx_busy_q && rx_sample_w)
begin
    // Last bit?
    if (rx_bits_q == STOP_BIT0 && !STOP_BITS)
        rx_busy_q <= 1'b0;
    else if (rx_bits_q == STOP_BIT1 && STOP_BITS)
        rx_busy_q <= 1'b0;
    else if (rx_bits_q == START_BIT)
    begin
        // Start bit should still be low as sampling mid
        // way through start bit, so if high, error!
        if (rxd_q)
            rx_busy_q <= 1'b0;
    end
    // Rx shift register
    else 
        rx_shift_reg_q <= {rxd_q, rx_shift_reg_q[7:1]};
end
// Start bit?
else if (!rx_busy_q && rxd_q == 1'b0)
begin
    rx_shift_reg_q <= 8'h00;
    rx_busy_q      <= 1'b1;
end

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    rx_bits_q  <= START_BIT;
else if (rx_sample_w && rx_busy_q)
begin
    if ((rx_bits_q == STOP_BIT1 && STOP_BITS) || (rx_bits_q == STOP_BIT0 && !STOP_BITS))
        rx_bits_q <= START_BIT;
    else
        rx_bits_q <= rx_bits_q + 4'd1;
end
else if (!rx_busy_q && (BIT_DIV == 32'b0))
    rx_bits_q  <= START_BIT + 4'd1;
else if (!rx_busy_q)
    rx_bits_q  <= START_BIT;

//-----------------------------------------------------------------
// RX Data
//-----------------------------------------------------------------
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
begin
   rx_ready_q      <= 1'b0;
   rx_data_q       <= 8'h00;
   rx_err_q        <= 1'b0;
end
else
begin
   // If reading data, reset data state
   if (ulite_rx_rd_req_w || ulite_control_rst_rx_out_w)
   begin
       rx_ready_q <= 1'b0;
       rx_err_q   <= 1'b0;
   end

   if (rx_busy_q && rx_sample_w)
   begin
       // Stop bit
       if ((rx_bits_q == STOP_BIT1 && STOP_BITS) || (rx_bits_q == STOP_BIT0 && !STOP_BITS))
       begin
           // RXD should be still high
           if (rxd_q)
           begin
               rx_data_q      <= rx_shift_reg_q;
               rx_ready_q     <= 1'b1;
           end
           // Bad Stop bit - wait for a full bit period
           // before allowing start bit detection again
           else
           begin
               rx_ready_q      <= 1'b0;
               rx_data_q       <= 8'h00;
               rx_err_q        <= 1'b1;
           end
       end
       // Mid start bit sample - if high then error
       else if (rx_bits_q == START_BIT && rxd_q)
           rx_err_q        <= 1'b1;
   end
end

assign ulite_rx_data_in_w        = rx_data_q;
assign ulite_status_rxvalid_in_w = rx_ready_q;
assign ulite_status_rxfull_in_w  = rx_ready_q;

//-----------------------------------------------------------------
// TX Clock Divider
//-----------------------------------------------------------------
wire tx_sample_w = (tx_count_q == 32'b0);

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    tx_count_q      <= 32'b0;
else
begin
    // Idle
    if (!tx_busy_q)
        tx_count_q  <= BIT_DIV;
    // Tx bit timer
    else if (tx_count_q != 0)
        tx_count_q  <= (tx_count_q - 1);
    else if (tx_sample_w)
        tx_count_q  <= BIT_DIV;
end

//-----------------------------------------------------------------
// TX Shift Register
//-----------------------------------------------------------------
reg tx_complete_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
begin
    tx_shift_reg_q <= 8'h00;
    tx_busy_q      <= 1'b0;
    tx_complete_q  <= 1'b0;
end
// Tx busy
else if (tx_busy_q)
begin
    // Shift tx data
    if (tx_bits_q != START_BIT && tx_sample_w)
        tx_shift_reg_q <= {1'b0, tx_shift_reg_q[7:1]};

    // Last bit?
    if (tx_bits_q == STOP_BIT0 && tx_sample_w && !STOP_BITS)
    begin
        tx_busy_q      <= 1'b0;
        tx_complete_q  <= 1'b1;
    end
    else if (tx_bits_q == STOP_BIT1 && tx_sample_w && STOP_BITS)
    begin
        tx_busy_q      <= 1'b0;
        tx_complete_q  <= 1'b1;
    end
end
// Buffer data to transmit
else if (ulite_tx_wr_req_w)
begin
    tx_shift_reg_q <= ulite_tx_data_out_w;
    tx_busy_q      <= 1'b1;
    tx_complete_q  <= 1'b0;
end
else
    tx_complete_q  <= 1'b0;

assign ulite_status_txfull_in_w  = tx_busy_q;
assign ulite_status_txempty_in_w = ~tx_busy_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    tx_bits_q  <= 4'd0;
else if (tx_sample_w && tx_busy_q)
begin
    if ((tx_bits_q == STOP_BIT1 && STOP_BITS) || (tx_bits_q == STOP_BIT0 && !STOP_BITS))
        tx_bits_q <= START_BIT;
    else
        tx_bits_q <= tx_bits_q + 4'd1;
end

//-----------------------------------------------------------------
// UART Tx Pin
//-----------------------------------------------------------------
reg txd_r;

always @ *
begin
    txd_r = 1'b1;

    if (tx_busy_q)
    begin
        // Start bit (TXD = L)
        if (tx_bits_q == START_BIT)
            txd_r = 1'b0;
        // Stop bits (TXD = H)
        else if (tx_bits_q == STOP_BIT0 || tx_bits_q == STOP_BIT1)
            txd_r = 1'b1;
        // Data bits
        else
            txd_r = tx_shift_reg_q[0];
    end
end

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    txd_q <= 1'b1;
else
    txd_q <= txd_r;

assign tx_o = txd_q;

//-----------------------------------------------------------------
// Interrupt
//-----------------------------------------------------------------
reg intr_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
   intr_q <= 1'b0;
else if (tx_complete_q)
   intr_q <= 1'b1;
else if (ulite_status_rxvalid_in_w)
   intr_q <= 1'b1;
else
   intr_q <= 1'b0;

assign ulite_status_ie_in_w = ulite_control_ie_out_w;

//-----------------------------------------------------------------
// Assignments
//-----------------------------------------------------------------
assign intr_o = intr_q;


endmodule
//-----------------------------------------------------------------
//                     Basic Peripheral SoC
//                           V1.1
//                     Ultra-Embedded.com
//                     Copyright 2014-2020
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

module core_soc
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
     parameter CLK_FREQ         = 50000000
    ,parameter BAUDRATE         = 1000000
    ,parameter C_SCK_RATIO      = 50
)
//-----------------------------------------------------------------
// Ports
//-----------------------------------------------------------------
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           inport_awvalid_i
    ,input  [ 31:0]  inport_awaddr_i
    ,input           inport_wvalid_i
    ,input  [ 31:0]  inport_wdata_i
    ,input  [  3:0]  inport_wstrb_i
    ,input           inport_bready_i
    ,input           inport_arvalid_i
    ,input  [ 31:0]  inport_araddr_i
    ,input           inport_rready_i
    ,input           spi_miso_i
    ,input           uart_rx_i
    ,input  [ 31:0]  gpio_input_i
    ,input           ext1_cfg_awready_i
    ,input           ext1_cfg_wready_i
    ,input           ext1_cfg_bvalid_i
    ,input  [  1:0]  ext1_cfg_bresp_i
    ,input           ext1_cfg_arready_i
    ,input           ext1_cfg_rvalid_i
    ,input  [ 31:0]  ext1_cfg_rdata_i
    ,input  [  1:0]  ext1_cfg_rresp_i
    ,input           ext1_irq_i
    ,input           ext2_cfg_awready_i
    ,input           ext2_cfg_wready_i
    ,input           ext2_cfg_bvalid_i
    ,input  [  1:0]  ext2_cfg_bresp_i
    ,input           ext2_cfg_arready_i
    ,input           ext2_cfg_rvalid_i
    ,input  [ 31:0]  ext2_cfg_rdata_i
    ,input  [  1:0]  ext2_cfg_rresp_i
    ,input           ext2_irq_i
    ,input           ext3_cfg_awready_i
    ,input           ext3_cfg_wready_i
    ,input           ext3_cfg_bvalid_i
    ,input  [  1:0]  ext3_cfg_bresp_i
    ,input           ext3_cfg_arready_i
    ,input           ext3_cfg_rvalid_i
    ,input  [ 31:0]  ext3_cfg_rdata_i
    ,input  [  1:0]  ext3_cfg_rresp_i
    ,input           ext3_irq_i

    // Outputs
    ,output          intr_o
    ,output          inport_awready_o
    ,output          inport_wready_o
    ,output          inport_bvalid_o
    ,output [  1:0]  inport_bresp_o
    ,output          inport_arready_o
    ,output          inport_rvalid_o
    ,output [ 31:0]  inport_rdata_o
    ,output [  1:0]  inport_rresp_o
    ,output          spi_clk_o
    ,output          spi_mosi_o
    ,output [  7:0]  spi_cs_o
    ,output          uart_tx_o
    ,output [ 31:0]  gpio_output_o
    ,output [ 31:0]  gpio_output_enable_o
    ,output          ext1_cfg_awvalid_o
    ,output [ 31:0]  ext1_cfg_awaddr_o
    ,output          ext1_cfg_wvalid_o
    ,output [ 31:0]  ext1_cfg_wdata_o
    ,output [  3:0]  ext1_cfg_wstrb_o
    ,output          ext1_cfg_bready_o
    ,output          ext1_cfg_arvalid_o
    ,output [ 31:0]  ext1_cfg_araddr_o
    ,output          ext1_cfg_rready_o
    ,output          ext2_cfg_awvalid_o
    ,output [ 31:0]  ext2_cfg_awaddr_o
    ,output          ext2_cfg_wvalid_o
    ,output [ 31:0]  ext2_cfg_wdata_o
    ,output [  3:0]  ext2_cfg_wstrb_o
    ,output          ext2_cfg_bready_o
    ,output          ext2_cfg_arvalid_o
    ,output [ 31:0]  ext2_cfg_araddr_o
    ,output          ext2_cfg_rready_o
    ,output          ext3_cfg_awvalid_o
    ,output [ 31:0]  ext3_cfg_awaddr_o
    ,output          ext3_cfg_wvalid_o
    ,output [ 31:0]  ext3_cfg_wdata_o
    ,output [  3:0]  ext3_cfg_wstrb_o
    ,output          ext3_cfg_bready_o
    ,output          ext3_cfg_arvalid_o
    ,output [ 31:0]  ext3_cfg_araddr_o
    ,output          ext3_cfg_rready_o
);

wire  [ 31:0]  periph1_rdata_w;
wire  [  1:0]  periph0_bresp_w;
wire           periph2_awvalid_w;
wire  [  1:0]  periph3_rresp_w;
wire           periph4_arready_w;
wire           periph1_bready_w;
wire           periph2_arready_w;
wire           periph3_arvalid_w;
wire  [ 31:0]  periph4_rdata_w;
wire           periph3_bready_w;
wire           periph3_rvalid_w;
wire           periph4_bvalid_w;
wire           periph1_bvalid_w;
wire           periph4_arvalid_w;
wire           periph2_bready_w;
wire           periph0_rvalid_w;
wire           periph2_awready_w;
wire           periph2_arvalid_w;
wire           periph1_arvalid_w;
wire  [ 31:0]  periph2_rdata_w;
wire           periph0_awvalid_w;
wire  [ 31:0]  periph3_araddr_w;
wire  [  1:0]  periph1_rresp_w;
wire  [ 31:0]  periph0_rdata_w;
wire  [  3:0]  periph4_wstrb_w;
wire  [ 31:0]  periph2_awaddr_w;
wire  [  1:0]  periph4_bresp_w;
wire           periph1_arready_w;
wire  [ 31:0]  periph3_rdata_w;
wire           periph4_rready_w;
wire           periph1_wvalid_w;
wire  [ 31:0]  periph4_araddr_w;
wire           periph0_bvalid_w;
wire           periph0_rready_w;
wire           periph1_rready_w;
wire           periph4_rvalid_w;
wire           periph3_arready_w;
wire  [  1:0]  periph2_bresp_w;
wire           periph3_awvalid_w;
wire           periph4_wready_w;
wire  [ 31:0]  periph3_awaddr_w;
wire  [  3:0]  periph1_wstrb_w;
wire  [  1:0]  periph0_rresp_w;
wire  [  3:0]  periph0_wstrb_w;
wire  [ 31:0]  periph1_wdata_w;
wire           periph1_awready_w;
wire           interrupt1_w;
wire           interrupt0_w;
wire           interrupt3_w;
wire           interrupt2_w;
wire  [ 31:0]  periph1_awaddr_w;
wire           periph4_awvalid_w;
wire           periph3_awready_w;
wire           periph1_awvalid_w;
wire           periph3_wready_w;
wire           periph0_wready_w;
wire  [ 31:0]  periph0_awaddr_w;
wire  [  1:0]  periph3_bresp_w;
wire           periph0_arvalid_w;
wire           periph3_bvalid_w;
wire           periph0_bready_w;
wire  [ 31:0]  periph2_wdata_w;
wire           periph4_wvalid_w;
wire           periph0_wvalid_w;
wire  [  3:0]  periph2_wstrb_w;
wire           periph2_rvalid_w;
wire  [ 31:0]  periph4_awaddr_w;
wire  [  1:0]  periph4_rresp_w;
wire  [  1:0]  periph1_bresp_w;
wire           periph1_wready_w;
wire           periph2_rready_w;
wire           periph2_bvalid_w;
wire           periph2_wready_w;
wire  [ 31:0]  periph0_wdata_w;
wire  [  3:0]  periph3_wstrb_w;
wire  [ 31:0]  periph4_wdata_w;
wire           periph0_awready_w;
wire           periph1_rvalid_w;
wire  [ 31:0]  periph1_araddr_w;
wire           periph4_awready_w;
wire           periph0_arready_w;
wire  [ 31:0]  periph2_araddr_w;
wire           periph3_wvalid_w;
wire  [  1:0]  periph2_rresp_w;
wire  [ 31:0]  periph0_araddr_w;
wire           periph4_bready_w;
wire  [ 31:0]  periph3_wdata_w;
wire           periph3_rready_w;
wire           periph2_wvalid_w;


irq_ctrl
u_intc
(
    // Inputs
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.cfg_awvalid_i(periph0_awvalid_w)
    ,.cfg_awaddr_i(periph0_awaddr_w)
    ,.cfg_wvalid_i(periph0_wvalid_w)
    ,.cfg_wdata_i(periph0_wdata_w)
    ,.cfg_wstrb_i(periph0_wstrb_w)
    ,.cfg_bready_i(periph0_bready_w)
    ,.cfg_arvalid_i(periph0_arvalid_w)
    ,.cfg_araddr_i(periph0_araddr_w)
    ,.cfg_rready_i(periph0_rready_w)
    ,.interrupt0_i(interrupt0_w)
    ,.interrupt1_i(interrupt1_w)
    ,.interrupt2_i(interrupt2_w)
    ,.interrupt3_i(interrupt3_w)
    ,.interrupt4_i(ext1_irq_i)
    ,.interrupt5_i(ext2_irq_i)
    ,.interrupt6_i(ext3_irq_i)

    // Outputs
    ,.cfg_awready_o(periph0_awready_w)
    ,.cfg_wready_o(periph0_wready_w)
    ,.cfg_bvalid_o(periph0_bvalid_w)
    ,.cfg_bresp_o(periph0_bresp_w)
    ,.cfg_arready_o(periph0_arready_w)
    ,.cfg_rvalid_o(periph0_rvalid_w)
    ,.cfg_rdata_o(periph0_rdata_w)
    ,.cfg_rresp_o(periph0_rresp_w)
    ,.intr_o(intr_o)
);


axi4lite_dist
u_dist
(
    // Inputs
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.inport_awvalid_i(inport_awvalid_i)
    ,.inport_awaddr_i(inport_awaddr_i)
    ,.inport_wvalid_i(inport_wvalid_i)
    ,.inport_wdata_i(inport_wdata_i)
    ,.inport_wstrb_i(inport_wstrb_i)
    ,.inport_bready_i(inport_bready_i)
    ,.inport_arvalid_i(inport_arvalid_i)
    ,.inport_araddr_i(inport_araddr_i)
    ,.inport_rready_i(inport_rready_i)
    ,.outport0_awready_i(periph0_awready_w)
    ,.outport0_wready_i(periph0_wready_w)
    ,.outport0_bvalid_i(periph0_bvalid_w)
    ,.outport0_bresp_i(periph0_bresp_w)
    ,.outport0_arready_i(periph0_arready_w)
    ,.outport0_rvalid_i(periph0_rvalid_w)
    ,.outport0_rdata_i(periph0_rdata_w)
    ,.outport0_rresp_i(periph0_rresp_w)
    ,.outport1_awready_i(periph1_awready_w)
    ,.outport1_wready_i(periph1_wready_w)
    ,.outport1_bvalid_i(periph1_bvalid_w)
    ,.outport1_bresp_i(periph1_bresp_w)
    ,.outport1_arready_i(periph1_arready_w)
    ,.outport1_rvalid_i(periph1_rvalid_w)
    ,.outport1_rdata_i(periph1_rdata_w)
    ,.outport1_rresp_i(periph1_rresp_w)
    ,.outport2_awready_i(periph2_awready_w)
    ,.outport2_wready_i(periph2_wready_w)
    ,.outport2_bvalid_i(periph2_bvalid_w)
    ,.outport2_bresp_i(periph2_bresp_w)
    ,.outport2_arready_i(periph2_arready_w)
    ,.outport2_rvalid_i(periph2_rvalid_w)
    ,.outport2_rdata_i(periph2_rdata_w)
    ,.outport2_rresp_i(periph2_rresp_w)
    ,.outport3_awready_i(periph3_awready_w)
    ,.outport3_wready_i(periph3_wready_w)
    ,.outport3_bvalid_i(periph3_bvalid_w)
    ,.outport3_bresp_i(periph3_bresp_w)
    ,.outport3_arready_i(periph3_arready_w)
    ,.outport3_rvalid_i(periph3_rvalid_w)
    ,.outport3_rdata_i(periph3_rdata_w)
    ,.outport3_rresp_i(periph3_rresp_w)
    ,.outport4_awready_i(periph4_awready_w)
    ,.outport4_wready_i(periph4_wready_w)
    ,.outport4_bvalid_i(periph4_bvalid_w)
    ,.outport4_bresp_i(periph4_bresp_w)
    ,.outport4_arready_i(periph4_arready_w)
    ,.outport4_rvalid_i(periph4_rvalid_w)
    ,.outport4_rdata_i(periph4_rdata_w)
    ,.outport4_rresp_i(periph4_rresp_w)
    ,.outport5_awready_i(ext1_cfg_awready_i)
    ,.outport5_wready_i(ext1_cfg_wready_i)
    ,.outport5_bvalid_i(ext1_cfg_bvalid_i)
    ,.outport5_bresp_i(ext1_cfg_bresp_i)
    ,.outport5_arready_i(ext1_cfg_arready_i)
    ,.outport5_rvalid_i(ext1_cfg_rvalid_i)
    ,.outport5_rdata_i(ext1_cfg_rdata_i)
    ,.outport5_rresp_i(ext1_cfg_rresp_i)
    ,.outport6_awready_i(ext2_cfg_awready_i)
    ,.outport6_wready_i(ext2_cfg_wready_i)
    ,.outport6_bvalid_i(ext2_cfg_bvalid_i)
    ,.outport6_bresp_i(ext2_cfg_bresp_i)
    ,.outport6_arready_i(ext2_cfg_arready_i)
    ,.outport6_rvalid_i(ext2_cfg_rvalid_i)
    ,.outport6_rdata_i(ext2_cfg_rdata_i)
    ,.outport6_rresp_i(ext2_cfg_rresp_i)
    ,.outport7_awready_i(ext3_cfg_awready_i)
    ,.outport7_wready_i(ext3_cfg_wready_i)
    ,.outport7_bvalid_i(ext3_cfg_bvalid_i)
    ,.outport7_bresp_i(ext3_cfg_bresp_i)
    ,.outport7_arready_i(ext3_cfg_arready_i)
    ,.outport7_rvalid_i(ext3_cfg_rvalid_i)
    ,.outport7_rdata_i(ext3_cfg_rdata_i)
    ,.outport7_rresp_i(ext3_cfg_rresp_i)

    // Outputs
    ,.inport_awready_o(inport_awready_o)
    ,.inport_wready_o(inport_wready_o)
    ,.inport_bvalid_o(inport_bvalid_o)
    ,.inport_bresp_o(inport_bresp_o)
    ,.inport_arready_o(inport_arready_o)
    ,.inport_rvalid_o(inport_rvalid_o)
    ,.inport_rdata_o(inport_rdata_o)
    ,.inport_rresp_o(inport_rresp_o)
    ,.outport0_awvalid_o(periph0_awvalid_w)
    ,.outport0_awaddr_o(periph0_awaddr_w)
    ,.outport0_wvalid_o(periph0_wvalid_w)
    ,.outport0_wdata_o(periph0_wdata_w)
    ,.outport0_wstrb_o(periph0_wstrb_w)
    ,.outport0_bready_o(periph0_bready_w)
    ,.outport0_arvalid_o(periph0_arvalid_w)
    ,.outport0_araddr_o(periph0_araddr_w)
    ,.outport0_rready_o(periph0_rready_w)
    ,.outport1_awvalid_o(periph1_awvalid_w)
    ,.outport1_awaddr_o(periph1_awaddr_w)
    ,.outport1_wvalid_o(periph1_wvalid_w)
    ,.outport1_wdata_o(periph1_wdata_w)
    ,.outport1_wstrb_o(periph1_wstrb_w)
    ,.outport1_bready_o(periph1_bready_w)
    ,.outport1_arvalid_o(periph1_arvalid_w)
    ,.outport1_araddr_o(periph1_araddr_w)
    ,.outport1_rready_o(periph1_rready_w)
    ,.outport2_awvalid_o(periph2_awvalid_w)
    ,.outport2_awaddr_o(periph2_awaddr_w)
    ,.outport2_wvalid_o(periph2_wvalid_w)
    ,.outport2_wdata_o(periph2_wdata_w)
    ,.outport2_wstrb_o(periph2_wstrb_w)
    ,.outport2_bready_o(periph2_bready_w)
    ,.outport2_arvalid_o(periph2_arvalid_w)
    ,.outport2_araddr_o(periph2_araddr_w)
    ,.outport2_rready_o(periph2_rready_w)
    ,.outport3_awvalid_o(periph3_awvalid_w)
    ,.outport3_awaddr_o(periph3_awaddr_w)
    ,.outport3_wvalid_o(periph3_wvalid_w)
    ,.outport3_wdata_o(periph3_wdata_w)
    ,.outport3_wstrb_o(periph3_wstrb_w)
    ,.outport3_bready_o(periph3_bready_w)
    ,.outport3_arvalid_o(periph3_arvalid_w)
    ,.outport3_araddr_o(periph3_araddr_w)
    ,.outport3_rready_o(periph3_rready_w)
    ,.outport4_awvalid_o(periph4_awvalid_w)
    ,.outport4_awaddr_o(periph4_awaddr_w)
    ,.outport4_wvalid_o(periph4_wvalid_w)
    ,.outport4_wdata_o(periph4_wdata_w)
    ,.outport4_wstrb_o(periph4_wstrb_w)
    ,.outport4_bready_o(periph4_bready_w)
    ,.outport4_arvalid_o(periph4_arvalid_w)
    ,.outport4_araddr_o(periph4_araddr_w)
    ,.outport4_rready_o(periph4_rready_w)
    ,.outport5_awvalid_o(ext1_cfg_awvalid_o)
    ,.outport5_awaddr_o(ext1_cfg_awaddr_o)
    ,.outport5_wvalid_o(ext1_cfg_wvalid_o)
    ,.outport5_wdata_o(ext1_cfg_wdata_o)
    ,.outport5_wstrb_o(ext1_cfg_wstrb_o)
    ,.outport5_bready_o(ext1_cfg_bready_o)
    ,.outport5_arvalid_o(ext1_cfg_arvalid_o)
    ,.outport5_araddr_o(ext1_cfg_araddr_o)
    ,.outport5_rready_o(ext1_cfg_rready_o)
    ,.outport6_awvalid_o(ext2_cfg_awvalid_o)
    ,.outport6_awaddr_o(ext2_cfg_awaddr_o)
    ,.outport6_wvalid_o(ext2_cfg_wvalid_o)
    ,.outport6_wdata_o(ext2_cfg_wdata_o)
    ,.outport6_wstrb_o(ext2_cfg_wstrb_o)
    ,.outport6_bready_o(ext2_cfg_bready_o)
    ,.outport6_arvalid_o(ext2_cfg_arvalid_o)
    ,.outport6_araddr_o(ext2_cfg_araddr_o)
    ,.outport6_rready_o(ext2_cfg_rready_o)
    ,.outport7_awvalid_o(ext3_cfg_awvalid_o)
    ,.outport7_awaddr_o(ext3_cfg_awaddr_o)
    ,.outport7_wvalid_o(ext3_cfg_wvalid_o)
    ,.outport7_wdata_o(ext3_cfg_wdata_o)
    ,.outport7_wstrb_o(ext3_cfg_wstrb_o)
    ,.outport7_bready_o(ext3_cfg_bready_o)
    ,.outport7_arvalid_o(ext3_cfg_arvalid_o)
    ,.outport7_araddr_o(ext3_cfg_araddr_o)
    ,.outport7_rready_o(ext3_cfg_rready_o)
);


uart_lite
#(
     .CLK_FREQ(CLK_FREQ)
    ,.BAUDRATE(BAUDRATE)
)
u_uart
(
    // Inputs
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.cfg_awvalid_i(periph2_awvalid_w)
    ,.cfg_awaddr_i(periph2_awaddr_w)
    ,.cfg_wvalid_i(periph2_wvalid_w)
    ,.cfg_wdata_i(periph2_wdata_w)
    ,.cfg_wstrb_i(periph2_wstrb_w)
    ,.cfg_bready_i(periph2_bready_w)
    ,.cfg_arvalid_i(periph2_arvalid_w)
    ,.cfg_araddr_i(periph2_araddr_w)
    ,.cfg_rready_i(periph2_rready_w)
    ,.rx_i(uart_rx_i)

    // Outputs
    ,.cfg_awready_o(periph2_awready_w)
    ,.cfg_wready_o(periph2_wready_w)
    ,.cfg_bvalid_o(periph2_bvalid_w)
    ,.cfg_bresp_o(periph2_bresp_w)
    ,.cfg_arready_o(periph2_arready_w)
    ,.cfg_rvalid_o(periph2_rvalid_w)
    ,.cfg_rdata_o(periph2_rdata_w)
    ,.cfg_rresp_o(periph2_rresp_w)
    ,.tx_o(uart_tx_o)
    ,.intr_o(interrupt1_w)
);


timer
u_timer
(
    // Inputs
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.cfg_awvalid_i(periph1_awvalid_w)
    ,.cfg_awaddr_i(periph1_awaddr_w)
    ,.cfg_wvalid_i(periph1_wvalid_w)
    ,.cfg_wdata_i(periph1_wdata_w)
    ,.cfg_wstrb_i(periph1_wstrb_w)
    ,.cfg_bready_i(periph1_bready_w)
    ,.cfg_arvalid_i(periph1_arvalid_w)
    ,.cfg_araddr_i(periph1_araddr_w)
    ,.cfg_rready_i(periph1_rready_w)

    // Outputs
    ,.cfg_awready_o(periph1_awready_w)
    ,.cfg_wready_o(periph1_wready_w)
    ,.cfg_bvalid_o(periph1_bvalid_w)
    ,.cfg_bresp_o(periph1_bresp_w)
    ,.cfg_arready_o(periph1_arready_w)
    ,.cfg_rvalid_o(periph1_rvalid_w)
    ,.cfg_rdata_o(periph1_rdata_w)
    ,.cfg_rresp_o(periph1_rresp_w)
    ,.intr_o(interrupt0_w)
);


spi_lite
#(
     .C_SCK_RATIO(C_SCK_RATIO)
)
u_spi
(
    // Inputs
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.cfg_awvalid_i(periph3_awvalid_w)
    ,.cfg_awaddr_i(periph3_awaddr_w)
    ,.cfg_wvalid_i(periph3_wvalid_w)
    ,.cfg_wdata_i(periph3_wdata_w)
    ,.cfg_wstrb_i(periph3_wstrb_w)
    ,.cfg_bready_i(periph3_bready_w)
    ,.cfg_arvalid_i(periph3_arvalid_w)
    ,.cfg_araddr_i(periph3_araddr_w)
    ,.cfg_rready_i(periph3_rready_w)
    ,.spi_miso_i(spi_miso_i)

    // Outputs
    ,.cfg_awready_o(periph3_awready_w)
    ,.cfg_wready_o(periph3_wready_w)
    ,.cfg_bvalid_o(periph3_bvalid_w)
    ,.cfg_bresp_o(periph3_bresp_w)
    ,.cfg_arready_o(periph3_arready_w)
    ,.cfg_rvalid_o(periph3_rvalid_w)
    ,.cfg_rdata_o(periph3_rdata_w)
    ,.cfg_rresp_o(periph3_rresp_w)
    ,.spi_clk_o(spi_clk_o)
    ,.spi_mosi_o(spi_mosi_o)
    ,.spi_cs_o(spi_cs_o)
    ,.intr_o(interrupt2_w)
);


gpio
u_gpio
(
    // Inputs
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.cfg_awvalid_i(periph4_awvalid_w)
    ,.cfg_awaddr_i(periph4_awaddr_w)
    ,.cfg_wvalid_i(periph4_wvalid_w)
    ,.cfg_wdata_i(periph4_wdata_w)
    ,.cfg_wstrb_i(periph4_wstrb_w)
    ,.cfg_bready_i(periph4_bready_w)
    ,.cfg_arvalid_i(periph4_arvalid_w)
    ,.cfg_araddr_i(periph4_araddr_w)
    ,.cfg_rready_i(periph4_rready_w)
    ,.gpio_input_i(gpio_input_i)

    // Outputs
    ,.cfg_awready_o(periph4_awready_w)
    ,.cfg_wready_o(periph4_wready_w)
    ,.cfg_bvalid_o(periph4_bvalid_w)
    ,.cfg_bresp_o(periph4_bresp_w)
    ,.cfg_arready_o(periph4_arready_w)
    ,.cfg_rvalid_o(periph4_rvalid_w)
    ,.cfg_rdata_o(periph4_rdata_w)
    ,.cfg_rresp_o(periph4_rresp_w)
    ,.gpio_output_o(gpio_output_o)
    ,.gpio_output_enable_o(gpio_output_enable_o)
    ,.intr_o(interrupt3_w)
);



endmodule
