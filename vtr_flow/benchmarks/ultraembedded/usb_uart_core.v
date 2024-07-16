//-----------------------------------------------------------------
//                       USB Serial Port
//                            V0.1
//                     Ultra-Embedded.com
//                       Copyright 2020
//
//                 Email: admin@ultra-embedded.com
//
//                         License: LGPL
//-----------------------------------------------------------------
//
// This source file may be used and distributed without         
// restriction provided that this copyright statement is not    
// removed from the file and that any derivative work contains  
// the original copyright notice and the associated disclaimer. 
//
// This source file is free software; you can redistribute it   
// and/or modify it under the terms of the GNU Lesser General   
// Public License as published by the Free Software Foundation; 
// either version 2.1 of the License, or (at your option) any   
// later version.
//
// This source is distributed in the hope that it will be       
// useful, but WITHOUT ANY WARRANTY; without even the implied   
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      
// PURPOSE.  See the GNU Lesser General Public License for more 
// details.
//
// You should have received a copy of the GNU Lesser General    
// Public License along with this source; if not, write to the 
// Free Software Foundation, Inc., 59 Temple Place, Suite 330, 
// Boston, MA  02111-1307  USA
//-----------------------------------------------------------------

//-----------------------------------------------------------------
//                          Generated File
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// Definitions
//-----------------------------------------------------------------

// Tokens
`define PID_OUT                    8'hE1
`define PID_IN                     8'h69
`define PID_SOF                    8'hA5
`define PID_SETUP                  8'h2D

// Data
`define PID_DATA0                  8'hC3
`define PID_DATA1                  8'h4B
`define PID_DATA2                  8'h87
`define PID_MDATA                  8'h0F

// Handshake
`define PID_ACK                    8'hD2
`define PID_NAK                    8'h5A
`define PID_STALL                  8'h1E
`define PID_NYET                   8'h96

// Special
`define PID_PRE                    8'h3C
`define PID_ERR                    8'h3C
`define PID_SPLIT                  8'h78
`define PID_PING                   8'hB4
//-----------------------------------------------------------------
//                       USB Serial Port
//                            V0.1
//                     Ultra-Embedded.com
//                       Copyright 2020
//
//                 Email: admin@ultra-embedded.com
//
//                         License: LGPL
//-----------------------------------------------------------------
//
// This source file may be used and distributed without         
// restriction provided that this copyright statement is not    
// removed from the file and that any derivative work contains  
// the original copyright notice and the associated disclaimer. 
//
// This source file is free software; you can redistribute it   
// and/or modify it under the terms of the GNU Lesser General   
// Public License as published by the Free Software Foundation; 
// either version 2.1 of the License, or (at your option) any   
// later version.
//
// This source is distributed in the hope that it will be       
// useful, but WITHOUT ANY WARRANTY; without even the implied   
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      
// PURPOSE.  See the GNU Lesser General Public License for more 
// details.
//
// You should have received a copy of the GNU Lesser General    
// Public License along with this source; if not, write to the 
// Free Software Foundation, Inc., 59 Temple Place, Suite 330, 
// Boston, MA  02111-1307  USA
//-----------------------------------------------------------------

//-----------------------------------------------------------------
//                          Generated File
//-----------------------------------------------------------------

module ulpi_wrapper
(
    // Inputs
     input           ulpi_clk60_i
    ,input           ulpi_rst_i
    ,input  [  7:0]  ulpi_data_out_i
    ,input           ulpi_dir_i
    ,input           ulpi_nxt_i
    ,input  [  7:0]  utmi_data_out_i
    ,input           utmi_txvalid_i
    ,input  [  1:0]  utmi_op_mode_i
    ,input  [  1:0]  utmi_xcvrselect_i
    ,input           utmi_termselect_i
    ,input           utmi_dppulldown_i
    ,input           utmi_dmpulldown_i

    // Outputs
    ,output [  7:0]  ulpi_data_in_o
    ,output          ulpi_stp_o
    ,output [  7:0]  utmi_data_in_o
    ,output          utmi_txready_o
    ,output          utmi_rxvalid_o
    ,output          utmi_rxactive_o
    ,output          utmi_rxerror_o
    ,output [  1:0]  utmi_linestate_o
);



//-----------------------------------------------------------------
// Module: UTMI+ to ULPI Wrapper
//
// Description:
//   - Converts from UTMI interface to reduced pin count ULPI.
//   - No support for low power mode.
//   - I/O synchronous to 60MHz ULPI clock input (from PHY)
//   - Tested against SMSC/Microchip USB3300 in device mode.
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// States
//-----------------------------------------------------------------
localparam STATE_W          = 2;
localparam STATE_IDLE       = 2'd0;
localparam STATE_CMD        = 2'd1;
localparam STATE_DATA       = 2'd2;
localparam STATE_REG        = 2'd3;

reg [STATE_W-1:0]   state_q;

//-----------------------------------------------------------------
// Local Params
//-----------------------------------------------------------------
localparam REG_FUNC_CTRL = 8'h84;
localparam REG_OTG_CTRL  = 8'h8a;
localparam REG_TRANSMIT  = 8'h40;
localparam REG_WRITE     = 8'h80;
localparam REG_READ      = 8'hC0;

//-----------------------------------------------------------------
// UTMI Mode Select
//-----------------------------------------------------------------
reg         mode_update_q;
reg [1:0]   xcvrselect_q;
reg         termselect_q;
reg [1:0]   opmode_q;
reg         phy_reset_q;
reg         mode_write_q;

// Detect register write completion
wire mode_complete_w = (state_q == STATE_REG &&
                        mode_write_q         && 
                        ulpi_nxt_i           && 
                        !ulpi_dir_i);           // Not interrupted by a Rx

always @ (posedge ulpi_clk60_i or posedge ulpi_rst_i)
if (ulpi_rst_i)
begin
    mode_update_q   <= 1'b0;
    xcvrselect_q    <= 2'b0;
    termselect_q    <= 1'b0;
    opmode_q        <= 2'b11;
    phy_reset_q     <= 1'b1;
end
else
begin
    xcvrselect_q    <= utmi_xcvrselect_i;
    termselect_q    <= utmi_termselect_i;
    opmode_q        <= utmi_op_mode_i;

    if (mode_update_q && mode_complete_w)
    begin
        mode_update_q <= 1'b0;
        phy_reset_q   <= 1'b0;
    end
    else if (opmode_q     != utmi_op_mode_i     ||
             termselect_q != utmi_termselect_i ||
             xcvrselect_q != utmi_xcvrselect_i)
        mode_update_q <= 1'b1;
end

//-----------------------------------------------------------------
// UTMI OTG Control
//-----------------------------------------------------------------
reg otg_update_q;
reg dppulldown_q;
reg dmpulldown_q;
reg otg_write_q;

// Detect register write completion
wire otg_complete_w  = (state_q == STATE_REG &&
                        otg_write_q         && 
                        ulpi_nxt_i           && 
                        !ulpi_dir_i);           // Not interrupted by a Rx

always @ (posedge ulpi_clk60_i or posedge ulpi_rst_i)
if (ulpi_rst_i)
begin
    otg_update_q    <= 1'b0;
    dppulldown_q    <= 1'b1;
    dmpulldown_q    <= 1'b1;
end
else
begin
    dppulldown_q    <= utmi_dppulldown_i;
    dmpulldown_q    <= utmi_dmpulldown_i;

    if (otg_update_q && otg_complete_w)
        otg_update_q <= 1'b0;
    else if (dppulldown_q != utmi_dppulldown_i ||
             dmpulldown_q != utmi_dmpulldown_i)
        otg_update_q <= 1'b1;
end

//-----------------------------------------------------------------
// Bus turnaround detect
//-----------------------------------------------------------------
reg ulpi_dir_q;

always @ (posedge ulpi_clk60_i or posedge ulpi_rst_i)
if (ulpi_rst_i)
    ulpi_dir_q <= 1'b0;
else
    ulpi_dir_q <= ulpi_dir_i;

wire turnaround_w = ulpi_dir_q ^ ulpi_dir_i;

//-----------------------------------------------------------------
// Rx - Tx delay
//-----------------------------------------------------------------
localparam TX_DELAY_W       = 3;
localparam TX_START_DELAY   = 3'd7;

reg [TX_DELAY_W-1:0] tx_delay_q;

always @ (posedge ulpi_clk60_i or posedge ulpi_rst_i)
if (ulpi_rst_i)
    tx_delay_q <= {TX_DELAY_W{1'b0}};
else if (utmi_rxactive_o)
    tx_delay_q <= TX_START_DELAY;
else if (tx_delay_q != {TX_DELAY_W{1'b0}})
    tx_delay_q <= tx_delay_q - 1;

wire tx_delay_complete_w = (tx_delay_q == {TX_DELAY_W{1'b0}});

//-----------------------------------------------------------------
// Tx Buffer - decouple UTMI Tx from PHY I/O
//-----------------------------------------------------------------
reg [7:0] tx_buffer_q[0:1];
reg       tx_valid_q[0:1];
reg       tx_wr_idx_q;
reg       tx_rd_idx_q;

wire      utmi_tx_ready_w;
wire      utmi_tx_accept_w;

always @ (posedge ulpi_clk60_i or posedge ulpi_rst_i)
if (ulpi_rst_i)
begin
    tx_buffer_q[0] <= 8'b0;
    tx_buffer_q[1] <= 8'b0;
    tx_valid_q[0]  <= 1'b0;
    tx_valid_q[1]  <= 1'b0;
    tx_wr_idx_q    <= 1'b0;
    tx_rd_idx_q    <= 1'b0;
end    
else
begin
    // Push
    if (utmi_txvalid_i && utmi_txready_o)
    begin
        tx_buffer_q[tx_wr_idx_q] <= utmi_data_out_i;
        tx_valid_q[tx_wr_idx_q]  <= 1'b1;

        tx_wr_idx_q <= tx_wr_idx_q + 1'b1;
    end

    // Pop
    if (utmi_tx_ready_w && utmi_tx_accept_w)
    begin
        tx_valid_q[tx_rd_idx_q]  <= 1'b0;
        tx_rd_idx_q <= tx_rd_idx_q + 1'b1;
    end
end

// Tx buffer space (only accept after Rx->Tx turnaround delay)
assign utmi_txready_o  = ~tx_valid_q[tx_wr_idx_q] & tx_delay_complete_w;

assign utmi_tx_ready_w = tx_valid_q[tx_rd_idx_q];

wire [7:0] utmi_tx_data_w = tx_buffer_q[tx_rd_idx_q];

//-----------------------------------------------------------------
// Implementation
//-----------------------------------------------------------------

// Xilinx placement pragmas:
//synthesis attribute IOB of ulpi_data_q is "TRUE"
//synthesis attribute IOB of ulpi_stp_q is "TRUE"

reg [7:0]           ulpi_data_q;
reg                 ulpi_stp_q;
reg [7:0]           data_q;

reg                 utmi_rxvalid_q;
reg                 utmi_rxerror_q;
reg                 utmi_rxactive_q;
reg [1:0]           utmi_linestate_q;
reg [7:0]           utmi_data_q;

always @ (posedge ulpi_clk60_i or posedge ulpi_rst_i)
if (ulpi_rst_i)
begin
    state_q             <= STATE_IDLE;
    ulpi_data_q         <= 8'b0;
    data_q              <= 8'b0;
    ulpi_stp_q          <= 1'b1;

    utmi_rxvalid_q      <= 1'b0;
    utmi_rxerror_q      <= 1'b0;
    utmi_rxactive_q     <= 1'b0;
    utmi_linestate_q    <= 2'b0;
    utmi_data_q         <= 8'b0;

    mode_write_q        <= 1'b0;
    otg_write_q         <= 1'b0;
end
else
begin
    ulpi_stp_q          <= 1'b0;
    utmi_rxvalid_q      <= 1'b0;

    // Turnaround: Input + NXT - set RX_ACTIVE
    if (turnaround_w && ulpi_dir_i && ulpi_nxt_i)
    begin
        utmi_rxactive_q <= 1'b1;

        // Register write - abort
        if (state_q == STATE_REG)
        begin
            state_q       <= STATE_IDLE;
            ulpi_data_q   <= 8'b0;  // IDLE
        end
    end
    // Turnaround: Input -> Output - reset RX_ACTIVE
    else if (turnaround_w && !ulpi_dir_i)
    begin
        utmi_rxactive_q <= 1'b0;

        // Register write - abort
        if (state_q == STATE_REG)
        begin
            state_q       <= STATE_IDLE;
            ulpi_data_q   <= 8'b0;  // IDLE
        end
    end
    // Non-turnaround cycle
    else if (!turnaround_w)
    begin
        //-----------------------------------------------------------------
        // Input: RX_CMD (status)
        //-----------------------------------------------------------------
        if (ulpi_dir_i && !ulpi_nxt_i)
        begin
            // Phy status
            utmi_linestate_q <= ulpi_data_out_i[1:0];

            case (ulpi_data_out_i[5:4])
            2'b00:
            begin
                utmi_rxactive_q <= 1'b0;
                utmi_rxerror_q  <= 1'b0;
            end
            2'b01: 
            begin
                utmi_rxactive_q <= 1'b1;
                utmi_rxerror_q  <= 1'b0;
            end
            2'b11:
            begin
                utmi_rxactive_q <= 1'b1;
                utmi_rxerror_q  <= 1'b1;
            end
            default:
                ; // HOST_DISCONNECTED
            endcase
        end
        //-----------------------------------------------------------------
        // Input: RX_DATA
        //-----------------------------------------------------------------
        else if (ulpi_dir_i && ulpi_nxt_i)
        begin
            utmi_rxvalid_q  <= 1'b1;
            utmi_data_q     <= ulpi_data_out_i;
        end
        //-----------------------------------------------------------------
        // Output
        //-----------------------------------------------------------------
        else if (!ulpi_dir_i)
        begin        
            // IDLE: Pending mode update
            if ((state_q == STATE_IDLE) && mode_update_q)
            begin
                data_q        <= {1'b0, 1'b1, phy_reset_q, opmode_q, termselect_q, xcvrselect_q};
                ulpi_data_q   <= REG_FUNC_CTRL;

                otg_write_q   <= 1'b0;
                mode_write_q  <= 1'b1;

                state_q       <= STATE_CMD;
            end
            // IDLE: Pending OTG control update
            else if ((state_q == STATE_IDLE) && otg_update_q)
            begin
                data_q        <= {5'b0, dmpulldown_q, dppulldown_q, 1'b0};
                ulpi_data_q   <= REG_OTG_CTRL;

                otg_write_q   <= 1'b1;
                mode_write_q  <= 1'b0;

                state_q       <= STATE_CMD;
            end
            // IDLE: Pending transmit
            else if ((state_q == STATE_IDLE) && utmi_tx_ready_w)
            begin
                ulpi_data_q <= REG_TRANSMIT | {4'b0, utmi_tx_data_w[3:0]};
                state_q     <= STATE_DATA;
            end
            // Command
            else if ((state_q == STATE_CMD) && ulpi_nxt_i)
            begin
                // Write Register
                state_q     <= STATE_REG;
                ulpi_data_q <= data_q;
            end
            // Data (register write)
            else if (state_q == STATE_REG && ulpi_nxt_i)
            begin
                state_q       <= STATE_IDLE;
                ulpi_data_q   <= 8'b0;  // IDLE
                ulpi_stp_q    <= 1'b1;

                otg_write_q   <= 1'b0;
                mode_write_q  <= 1'b0;
            end
            // Data
            else if (state_q == STATE_DATA && ulpi_nxt_i)
            begin
                // End of packet
                if (!utmi_tx_ready_w)
                begin
                    state_q       <= STATE_IDLE;
                    ulpi_data_q   <= 8'b0;  // IDLE
                    ulpi_stp_q    <= 1'b1;
                end
                else
                begin
                    state_q        <= STATE_DATA;
                    ulpi_data_q    <= utmi_tx_data_w;
                end
            end
        end
    end
end

// Accept from buffer
assign utmi_tx_accept_w = ((state_q == STATE_IDLE) && !(mode_update_q || otg_update_q || turnaround_w) && !ulpi_dir_i) ||
                          (state_q == STATE_DATA && ulpi_nxt_i && !ulpi_dir_i);

//-----------------------------------------------------------------
// Assignments
//-----------------------------------------------------------------
// ULPI Interface
assign ulpi_data_in_o       = ulpi_data_q;
assign ulpi_stp_o           = ulpi_stp_q;

// UTMI Interface
assign utmi_linestate_o     = utmi_linestate_q;
assign utmi_data_in_o       = utmi_data_q;
assign utmi_rxerror_o       = utmi_rxerror_q;
assign utmi_rxactive_o      = utmi_rxactive_q;
assign utmi_rxvalid_o       = utmi_rxvalid_q;



endmodule
//-----------------------------------------------------------------
//                       USB Serial Port
//                            V0.1
//                     Ultra-Embedded.com
//                       Copyright 2020
//
//                 Email: admin@ultra-embedded.com
//
//                         License: LGPL
//-----------------------------------------------------------------
//
// This source file may be used and distributed without         
// restriction provided that this copyright statement is not    
// removed from the file and that any derivative work contains  
// the original copyright notice and the associated disclaimer. 
//
// This source file is free software; you can redistribute it   
// and/or modify it under the terms of the GNU Lesser General   
// Public License as published by the Free Software Foundation; 
// either version 2.1 of the License, or (at your option) any   
// later version.
//
// This source is distributed in the hope that it will be       
// useful, but WITHOUT ANY WARRANTY; without even the implied   
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      
// PURPOSE.  See the GNU Lesser General Public License for more 
// details.
//
// You should have received a copy of the GNU Lesser General    
// Public License along with this source; if not, write to the 
// Free Software Foundation, Inc., 59 Temple Place, Suite 330, 
// Boston, MA  02111-1307  USA
//-----------------------------------------------------------------

//-----------------------------------------------------------------
//                          Generated File
//-----------------------------------------------------------------

module usb_cdc_core
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           enable_i
    ,input  [  7:0]  utmi_data_in_i
    ,input           utmi_txready_i
    ,input           utmi_rxvalid_i
    ,input           utmi_rxactive_i
    ,input           utmi_rxerror_i
    ,input  [  1:0]  utmi_linestate_i
    ,input           inport_valid_i
    ,input  [  7:0]  inport_data_i
    ,input           outport_accept_i

    // Outputs
    ,output [  7:0]  utmi_data_out_o
    ,output          utmi_txvalid_o
    ,output [  1:0]  utmi_op_mode_o
    ,output [  1:0]  utmi_xcvrselect_o
    ,output          utmi_termselect_o
    ,output          utmi_dppulldown_o
    ,output          utmi_dmpulldown_o
    ,output          inport_accept_o
    ,output          outport_valid_o
    ,output [  7:0]  outport_data_o
);




parameter USB_SPEED_HS = "False"; // True or False

//-----------------------------------------------------------------
// Defines
//-----------------------------------------------------------------
// Device class
`define DEV_CLASS_RESERVED              8'h00
`define DEV_CLASS_AUDIO                 8'h01
`define DEV_CLASS_COMMS                 8'h02
`define DEV_CLASS_HID                   8'h03
`define DEV_CLASS_MONITOR               8'h04
`define DEV_CLASS_PHY_IF                8'h05
`define DEV_CLASS_POWER                 8'h06
`define DEV_CLASS_PRINTER               8'h07
`define DEV_CLASS_STORAGE               8'h08
`define DEV_CLASS_HUB                   8'h09
`define DEV_CLASS_TMC                   8'hFE
`define DEV_CLASS_VENDOR_CUSTOM         8'hFF

// Standard requests (via SETUP packets)
`define REQ_GET_STATUS                  8'h00
`define REQ_CLEAR_FEATURE               8'h01
`define REQ_SET_FEATURE                 8'h03
`define REQ_SET_ADDRESS                 8'h05
`define REQ_GET_DESCRIPTOR              8'h06
`define REQ_SET_DESCRIPTOR              8'h07
`define REQ_GET_CONFIGURATION           8'h08
`define REQ_SET_CONFIGURATION           8'h09
`define REQ_GET_INTERFACE               8'h0A
`define REQ_SET_INTERFACE               8'h0B
`define REQ_SYNC_FRAME                  8'h0C

// Descriptor types
`define DESC_DEVICE                     8'h01
`define DESC_CONFIGURATION              8'h02
`define DESC_STRING                     8'h03
`define DESC_INTERFACE                  8'h04
`define DESC_ENDPOINT                   8'h05
`define DESC_DEV_QUALIFIER              8'h06
`define DESC_OTHER_SPEED_CONF           8'h07
`define DESC_IF_POWER                   8'h08

// Endpoints
`define ENDPOINT_DIR_MASK               8'h80
`define ENDPOINT_DIR_R                  7
`define ENDPOINT_DIR_IN                 1'b1
`define ENDPOINT_DIR_OUT                1'b0
`define ENDPOINT_ADDR_MASK              8'h7F
`define ENDPOINT_TYPE_MASK              8'h3
`define ENDPOINT_TYPE_CONTROL           0
`define ENDPOINT_TYPE_ISO               1
`define ENDPOINT_TYPE_BULK              2
`define ENDPOINT_TYPE_INTERRUPT         3

// Device Requests (bmRequestType)
`define USB_RECIPIENT_MASK              8'h1F
`define USB_RECIPIENT_DEVICE            8'h00
`define USB_RECIPIENT_INTERFACE         8'h01
`define USB_RECIPIENT_ENDPOINT          8'h02
`define USB_REQUEST_TYPE_MASK           8'h60
`define USB_STANDARD_REQUEST            8'h00
`define USB_CLASS_REQUEST               8'h20
`define USB_VENDOR_REQUEST              8'h40

// USB device addresses are 7-bits
`define USB_ADDRESS_MASK                8'h7F

// USB Feature Selectors
`define USB_FEATURE_ENDPOINT_STATE      16'h0000
`define USB_FEATURE_REMOTE_WAKEUP       16'h0001
`define USB_FEATURE_TEST_MODE           16'h0002

// String Descriptors
`define UNICODE_LANGUAGE_STR_ID         8'd0
`define MANUFACTURER_STR_ID             8'd1
`define PRODUCT_NAME_STR_ID             8'd2
`define SERIAL_NUM_STR_ID               8'd3

`define CDC_ENDPOINT_BULK_OUT           1
`define CDC_ENDPOINT_BULK_IN            2
`define CDC_ENDPOINT_INTR_IN            3

`define CDC_SEND_ENCAPSULATED_COMMAND   8'h00
`define CDC_GET_ENCAPSULATED_RESPONSE   8'h01
`define CDC_GET_LINE_CODING             8'h21
`define CDC_SET_LINE_CODING             8'h20
`define CDC_SET_CONTROL_LINE_STATE      8'h22
`define CDC_SEND_BREAK                  8'h23

// Descriptor ROM offsets / sizes
`define ROM_DESC_DEVICE_ADDR            8'd0
`define ROM_DESC_DEVICE_SIZE            16'd18
`define ROM_DESC_CONF_ADDR              8'd18
`define ROM_DESC_CONF_SIZE              16'd67
`define ROM_DESC_STR_LANG_ADDR          8'd85
`define ROM_DESC_STR_LANG_SIZE          16'd4
`define ROM_DESC_STR_MAN_ADDR           8'd89
`define ROM_DESC_STR_MAN_SIZE           16'd30
`define ROM_DESC_STR_PROD_ADDR          8'd119
`define ROM_DESC_STR_PROD_SIZE          16'd30
`define ROM_DESC_STR_SERIAL_ADDR        8'd149
`define ROM_DESC_STR_SERIAL_SIZE        16'd14
`define ROM_CDC_LINE_CODING_ADDR        8'd163
`define ROM_CDC_LINE_CODING_SIZE        16'd7

//-----------------------------------------------------------------
// Wires
//-----------------------------------------------------------------
wire         usb_reset_w;
reg  [6:0]   device_addr_q;

wire         usb_ep0_tx_rd_w;
wire [7:0]   usb_ep0_tx_data_w;
wire         usb_ep0_tx_empty_w;

wire         usb_ep0_rx_wr_w;
wire [7:0]   usb_ep0_rx_data_w;
wire         usb_ep0_rx_full_w;
wire         usb_ep1_tx_rd_w;
wire [7:0]   usb_ep1_tx_data_w;
wire         usb_ep1_tx_empty_w;

wire         usb_ep1_rx_wr_w;
wire [7:0]   usb_ep1_rx_data_w;
wire         usb_ep1_rx_full_w;
wire         usb_ep2_tx_rd_w;
wire [7:0]   usb_ep2_tx_data_w;
wire         usb_ep2_tx_empty_w;

wire         usb_ep2_rx_wr_w;
wire [7:0]   usb_ep2_rx_data_w;
wire         usb_ep2_rx_full_w;
wire         usb_ep3_tx_rd_w;
wire [7:0]   usb_ep3_tx_data_w;
wire         usb_ep3_tx_empty_w;

wire         usb_ep3_rx_wr_w;
wire [7:0]   usb_ep3_rx_data_w;
wire         usb_ep3_rx_full_w;

// Rx SIE Interface (shared)
wire        rx_strb_w;
wire [7:0]  rx_data_w;
wire        rx_last_w;
wire        rx_crc_err_w;

// EP0 Rx SIE Interface
wire        ep0_rx_space_w;
wire        ep0_rx_valid_w;
wire        ep0_rx_setup_w;

// EP0 Tx SIE Interface
wire        ep0_tx_ready_w;
wire        ep0_tx_data_valid_w;
wire        ep0_tx_data_strb_w;
wire [7:0]  ep0_tx_data_w;
wire        ep0_tx_data_last_w;
wire        ep0_tx_data_accept_w;
wire        ep0_tx_stall_w;
// EP1 Rx SIE Interface
wire        ep1_rx_space_w;
wire        ep1_rx_valid_w;
wire        ep1_rx_setup_w;

// EP1 Tx SIE Interface
wire        ep1_tx_ready_w;
wire        ep1_tx_data_valid_w;
wire        ep1_tx_data_strb_w;
wire [7:0]  ep1_tx_data_w;
wire        ep1_tx_data_last_w;
wire        ep1_tx_data_accept_w;
wire        ep1_tx_stall_w;
// EP2 Rx SIE Interface
wire        ep2_rx_space_w;
wire        ep2_rx_valid_w;
wire        ep2_rx_setup_w;

// EP2 Tx SIE Interface
wire        ep2_tx_ready_w;
wire        ep2_tx_data_valid_w;
wire        ep2_tx_data_strb_w;
wire [7:0]  ep2_tx_data_w;
wire        ep2_tx_data_last_w;
wire        ep2_tx_data_accept_w;
wire        ep2_tx_stall_w;
// EP3 Rx SIE Interface
wire        ep3_rx_space_w;
wire        ep3_rx_valid_w;
wire        ep3_rx_setup_w;

// EP3 Tx SIE Interface
wire        ep3_tx_ready_w;
wire        ep3_tx_data_valid_w;
wire        ep3_tx_data_strb_w;
wire [7:0]  ep3_tx_data_w;
wire        ep3_tx_data_last_w;
wire        ep3_tx_data_accept_w;
wire        ep3_tx_stall_w;

wire utmi_chirp_en_w;
wire usb_hs_w;

//-----------------------------------------------------------------
// Transceiver Control (high speed)
//-----------------------------------------------------------------
generate 
if (USB_SPEED_HS == "True")
begin

localparam STATE_W                       = 3;
localparam STATE_IDLE                    = 3'd0;
localparam STATE_WAIT_RST                = 3'd1;
localparam STATE_SEND_CHIRP_K            = 3'd2;
localparam STATE_WAIT_CHIRP_JK           = 3'd3;
localparam STATE_FULLSPEED               = 3'd4;
localparam STATE_HIGHSPEED               = 3'd5;
reg [STATE_W-1:0] state_q;
reg [STATE_W-1:0] next_state_r;

// 60MHz clock rate
`define USB_RST_W  20
reg [`USB_RST_W-1:0] usb_rst_time_q;
reg [7:0]            chirp_count_q;
reg [1:0]            last_linestate_q;

localparam DETACH_TIME    = 20'd60000;  // 1ms -> T0
localparam ATTACH_FS_TIME = 20'd180000; // T0 + 3ms = T1
localparam CHIRPK_TIME    = 20'd246000; // T1 + ~1ms
localparam HS_RESET_TIME  = 20'd600000; // T0 + 10ms = T9
localparam HS_CHIRP_COUNT = 8'd5;

reg [  1:0]  utmi_op_mode_r;
reg [  1:0]  utmi_xcvrselect_r;
reg          utmi_termselect_r;
reg          utmi_dppulldown_r;
reg          utmi_dmpulldown_r;

always @ *
begin
    next_state_r = state_q;

    // Default - disconnect
    utmi_op_mode_r    = 2'd1;
    utmi_xcvrselect_r = 2'd0;
    utmi_termselect_r = 1'b0;
    utmi_dppulldown_r = 1'b0;
    utmi_dmpulldown_r = 1'b0;

    case (state_q)
    STATE_IDLE:
    begin
        // Detached
        if (enable_i && usb_rst_time_q >= DETACH_TIME)
            next_state_r = STATE_WAIT_RST;
    end
    STATE_WAIT_RST:
    begin
        // Assert FS mode, check for SE0 (T0)
        utmi_op_mode_r    = 2'd0;
        utmi_xcvrselect_r = 2'd1;
        utmi_termselect_r = 1'b1;
        utmi_dppulldown_r = 1'b0;
        utmi_dmpulldown_r = 1'b0;

        // Wait for SE0 (T1), send device chirp K
        if (usb_rst_time_q >= ATTACH_FS_TIME)
            next_state_r = STATE_SEND_CHIRP_K;
    end
    STATE_SEND_CHIRP_K:
    begin
        // Send chirp K
        utmi_op_mode_r    = 2'd2;
        utmi_xcvrselect_r = 2'd0;
        utmi_termselect_r = 1'b1;
        utmi_dppulldown_r = 1'b0;
        utmi_dmpulldown_r = 1'b0;

        // End of device chirp K (T2)
        if (usb_rst_time_q >= CHIRPK_TIME)
            next_state_r = STATE_WAIT_CHIRP_JK;
    end
    STATE_WAIT_CHIRP_JK:
    begin
        // Stop sending chirp K and wait for downstream port chirps
        utmi_op_mode_r    = 2'd2;
        utmi_xcvrselect_r = 2'd0;
        utmi_termselect_r = 1'b1;
        utmi_dppulldown_r = 1'b0;
        utmi_dmpulldown_r = 1'b0;

        // Required number of chirps detected, move to HS mode (T7)
        if (chirp_count_q >= HS_CHIRP_COUNT)
            next_state_r = STATE_HIGHSPEED;
        // Time out waiting for chirps, fallback to FS mode
        else if (usb_rst_time_q >= HS_RESET_TIME)
            next_state_r = STATE_FULLSPEED;
    end
    STATE_FULLSPEED:
    begin
        utmi_op_mode_r    = 2'd0;
        utmi_xcvrselect_r = 2'd1;
        utmi_termselect_r = 1'b1;
        utmi_dppulldown_r = 1'b0;
        utmi_dmpulldown_r = 1'b0;

        // USB reset detected...
        if (usb_rst_time_q >= HS_RESET_TIME && usb_reset_w)
            next_state_r = STATE_WAIT_RST;
    end
    STATE_HIGHSPEED:
    begin
        // Enter HS mode
        utmi_op_mode_r    = 2'd0;
        utmi_xcvrselect_r = 2'd0;
        utmi_termselect_r = 1'b0;
        utmi_dppulldown_r = 1'b0;
        utmi_dmpulldown_r = 1'b0;

        // Long SE0 - could be reset or suspend
        // TODO: Should revert to FS mode and check...
        if (usb_rst_time_q >= HS_RESET_TIME && usb_reset_w)
            next_state_r = STATE_WAIT_RST;
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

// Time since T0 (start of HS reset)
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    usb_rst_time_q <= `USB_RST_W'b0;
// Entering wait for reset state
else if (next_state_r == STATE_WAIT_RST && state_q != STATE_WAIT_RST)
    usb_rst_time_q <=  `USB_RST_W'b0;
// Waiting for reset, reset count on line state toggle
else if (state_q == STATE_WAIT_RST && (utmi_linestate_i != 2'b00))
    usb_rst_time_q <=  `USB_RST_W'b0;
else if (usb_rst_time_q != {(`USB_RST_W){1'b1}})
    usb_rst_time_q <= usb_rst_time_q + `USB_RST_W'd1;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    last_linestate_q   <= 2'b0;
else
    last_linestate_q   <= utmi_linestate_i;

// Chirp counter
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    chirp_count_q   <= 8'b0;
else if (state_q == STATE_SEND_CHIRP_K)
    chirp_count_q   <= 8'b0;
else if (state_q == STATE_WAIT_CHIRP_JK && (last_linestate_q != utmi_linestate_i) && chirp_count_q != 8'hFF)
    chirp_count_q   <= chirp_count_q + 8'd1;

assign utmi_op_mode_o    = utmi_op_mode_r;
assign utmi_xcvrselect_o = utmi_xcvrselect_r;
assign utmi_termselect_o = utmi_termselect_r;
assign utmi_dppulldown_o = utmi_dppulldown_r;
assign utmi_dmpulldown_o = utmi_dmpulldown_r;

assign utmi_chirp_en_w   = (state_q == STATE_SEND_CHIRP_K);
assign usb_hs_w          = (state_q == STATE_HIGHSPEED);

end
else
begin
//-----------------------------------------------------------------
// Transceiver Control
//-----------------------------------------------------------------
reg [  1:0]  utmi_op_mode_r;
reg [  1:0]  utmi_xcvrselect_r;
reg          utmi_termselect_r;
reg          utmi_dppulldown_r;
reg          utmi_dmpulldown_r;

always @ *
begin
    if (enable_i)
    begin
        utmi_op_mode_r    = 2'd0;
        utmi_xcvrselect_r = 2'd1;
        utmi_termselect_r = 1'b1;
        utmi_dppulldown_r = 1'b0;
        utmi_dmpulldown_r = 1'b0;
    end
    else
    begin
        utmi_op_mode_r    = 2'd1;
        utmi_xcvrselect_r = 2'd0;
        utmi_termselect_r = 1'b0;
        utmi_dppulldown_r = 1'b0;
        utmi_dmpulldown_r = 1'b0;
    end
end

assign utmi_op_mode_o    = utmi_op_mode_r;
assign utmi_xcvrselect_o = utmi_xcvrselect_r;
assign utmi_termselect_o = utmi_termselect_r;
assign utmi_dppulldown_o = utmi_dppulldown_r;
assign utmi_dmpulldown_o = utmi_dmpulldown_r;

assign utmi_chirp_en_w   = 1'b0;
assign usb_hs_w          = 1'b0;

end
endgenerate

//-----------------------------------------------------------------
// Core
//-----------------------------------------------------------------
usbf_device_core
u_core
(
    .clk_i(clk_i),
    .rst_i(rst_i),

    .intr_o(),

    // UTMI interface
    .utmi_data_o(utmi_data_out_o),
    .utmi_data_i(utmi_data_in_i),
    .utmi_txvalid_o(utmi_txvalid_o),
    .utmi_txready_i(utmi_txready_i),
    .utmi_rxvalid_i(utmi_rxvalid_i),
    .utmi_rxactive_i(utmi_rxactive_i),
    .utmi_rxerror_i(utmi_rxerror_i),
    .utmi_linestate_i(utmi_linestate_i),

    .reg_chirp_en_i(utmi_chirp_en_w),
    .reg_int_en_sof_i(1'b0),

    .reg_dev_addr_i(device_addr_q),

    // Rx SIE Interface (shared)
    .rx_strb_o(rx_strb_w),
    .rx_data_o(rx_data_w),
    .rx_last_o(rx_last_w),
    .rx_crc_err_o(rx_crc_err_w),

    // EP0 Config
    .ep0_iso_i(1'b0),
    .ep0_stall_i(ep0_tx_stall_w),
    .ep0_cfg_int_rx_i(1'b0),
    .ep0_cfg_int_tx_i(1'b0),

    // EP0 Rx SIE Interface
    .ep0_rx_setup_o(ep0_rx_setup_w),
    .ep0_rx_valid_o(ep0_rx_valid_w),
    .ep0_rx_space_i(ep0_rx_space_w),

    // EP0 Tx SIE Interface
    .ep0_tx_ready_i(ep0_tx_ready_w),
    .ep0_tx_data_valid_i(ep0_tx_data_valid_w),
    .ep0_tx_data_strb_i(ep0_tx_data_strb_w),
    .ep0_tx_data_i(ep0_tx_data_w),
    .ep0_tx_data_last_i(ep0_tx_data_last_w),
    .ep0_tx_data_accept_o(ep0_tx_data_accept_w),

    // EP1 Config
    .ep1_iso_i(1'b0),
    .ep1_stall_i(ep1_tx_stall_w),
    .ep1_cfg_int_rx_i(1'b0),
    .ep1_cfg_int_tx_i(1'b0),

    // EP1 Rx SIE Interface
    .ep1_rx_setup_o(ep1_rx_setup_w),
    .ep1_rx_valid_o(ep1_rx_valid_w),
    .ep1_rx_space_i(ep1_rx_space_w),

    // EP1 Tx SIE Interface
    .ep1_tx_ready_i(ep1_tx_ready_w),
    .ep1_tx_data_valid_i(ep1_tx_data_valid_w),
    .ep1_tx_data_strb_i(ep1_tx_data_strb_w),
    .ep1_tx_data_i(ep1_tx_data_w),
    .ep1_tx_data_last_i(ep1_tx_data_last_w),
    .ep1_tx_data_accept_o(ep1_tx_data_accept_w),

    // EP2 Config
    .ep2_iso_i(1'b0),
    .ep2_stall_i(ep2_tx_stall_w),
    .ep2_cfg_int_rx_i(1'b0),
    .ep2_cfg_int_tx_i(1'b0),

    // EP2 Rx SIE Interface
    .ep2_rx_setup_o(ep2_rx_setup_w),
    .ep2_rx_valid_o(ep2_rx_valid_w),
    .ep2_rx_space_i(ep2_rx_space_w),

    // EP2 Tx SIE Interface
    .ep2_tx_ready_i(ep2_tx_ready_w),
    .ep2_tx_data_valid_i(ep2_tx_data_valid_w),
    .ep2_tx_data_strb_i(ep2_tx_data_strb_w),
    .ep2_tx_data_i(ep2_tx_data_w),
    .ep2_tx_data_last_i(ep2_tx_data_last_w),
    .ep2_tx_data_accept_o(ep2_tx_data_accept_w),

    // EP3 Config
    .ep3_iso_i(1'b0),
    .ep3_stall_i(ep3_tx_stall_w),
    .ep3_cfg_int_rx_i(1'b0),
    .ep3_cfg_int_tx_i(1'b0),

    // EP3 Rx SIE Interface
    .ep3_rx_setup_o(ep3_rx_setup_w),
    .ep3_rx_valid_o(ep3_rx_valid_w),
    .ep3_rx_space_i(ep3_rx_space_w),

    // EP3 Tx SIE Interface
    .ep3_tx_ready_i(ep3_tx_ready_w),
    .ep3_tx_data_valid_i(ep3_tx_data_valid_w),
    .ep3_tx_data_strb_i(ep3_tx_data_strb_w),
    .ep3_tx_data_i(ep3_tx_data_w),
    .ep3_tx_data_last_i(ep3_tx_data_last_w),
    .ep3_tx_data_accept_o(ep3_tx_data_accept_w),

    // Status
    .reg_sts_rst_clr_i(1'b1),
    .reg_sts_rst_o(usb_reset_w),
    .reg_sts_frame_num_o()
);

assign ep0_rx_space_w = 1'b1;

//-----------------------------------------------------------------
// USB: Setup packet capture (limited to 8 bytes for USB-FS)
//-----------------------------------------------------------------
reg [7:0] setup_packet_q[0:7];
reg [2:0] setup_wr_idx_q;
reg       setup_frame_q;
reg       setup_valid_q;
reg       setup_data_q;
reg       status_ready_q; // STATUS response received

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
begin
    setup_packet_q[0]  <= 8'b0;
    setup_packet_q[1]  <= 8'b0;
    setup_packet_q[2]  <= 8'b0;
    setup_packet_q[3]  <= 8'b0;
    setup_packet_q[4]  <= 8'b0;
    setup_packet_q[5]  <= 8'b0;
    setup_packet_q[6]  <= 8'b0;
    setup_packet_q[7]  <= 8'b0;
    setup_wr_idx_q     <= 3'b0;
    setup_valid_q      <= 1'b0;
    setup_frame_q      <= 1'b0;
    setup_data_q       <= 1'b0;
    status_ready_q     <= 1'b0;
end
// SETUP token received
else if (ep0_rx_setup_w)
begin
    setup_packet_q[0]  <= 8'b0;
    setup_packet_q[1]  <= 8'b0;
    setup_packet_q[2]  <= 8'b0;
    setup_packet_q[3]  <= 8'b0;
    setup_packet_q[4]  <= 8'b0;
    setup_packet_q[5]  <= 8'b0;
    setup_packet_q[6]  <= 8'b0;
    setup_packet_q[7]  <= 8'b0;
    setup_wr_idx_q     <= 3'b0;
    setup_valid_q      <= 1'b0;
    setup_frame_q      <= 1'b1;
    setup_data_q       <= 1'b0;
    status_ready_q     <= 1'b0;
end
// Valid DATA for setup frame
else if (ep0_rx_valid_w && rx_strb_w)
begin
    setup_packet_q[setup_wr_idx_q] <= rx_data_w;
    setup_wr_idx_q      <= setup_wr_idx_q + 3'd1;
    setup_valid_q       <= setup_frame_q && rx_last_w;
    setup_data_q        <= !setup_frame_q && rx_last_w;
    if (rx_last_w)
        setup_frame_q   <= 1'b0;
end
// Detect STATUS stage (ACK for SETUP GET requests)
// TODO: Not quite correct .... 
else if (ep0_rx_valid_w && !rx_strb_w && rx_last_w)
begin
    setup_valid_q       <= 1'b0;
    status_ready_q      <= 1'b1;
end
else
    setup_valid_q       <= 1'b0;

//-----------------------------------------------------------------
// SETUP request decode
//-----------------------------------------------------------------
wire [7:0]  bmRequestType_w = setup_packet_q[0];
wire [7:0]  bRequest_w      = setup_packet_q[1];
wire [15:0] wValue_w        = {setup_packet_q[3], setup_packet_q[2]};
wire [15:0] wIndex_w        = {setup_packet_q[5], setup_packet_q[4]};
wire [15:0] wLength         = {setup_packet_q[7], setup_packet_q[6]};

wire setup_get_w            = setup_valid_q && (bmRequestType_w[`ENDPOINT_DIR_R] == `ENDPOINT_DIR_IN);
wire setup_set_w            = setup_valid_q && (bmRequestType_w[`ENDPOINT_DIR_R] == `ENDPOINT_DIR_OUT);
wire setup_no_data_w        = setup_set_w && (wLength == 16'b0);

// For REQ_GET_DESCRIPTOR
wire [7:0]  bDescriptorType_w  = setup_packet_q[3];
wire [7:0]  bDescriptorIndex_w = setup_packet_q[2];

//-----------------------------------------------------------------
// Process setup request
//-----------------------------------------------------------------
reg        ctrl_stall_r; // Send STALL
reg        ctrl_ack_r;   // Send STATUS (ZLP)
reg [15:0] ctrl_get_len_r;

reg [7:0]  desc_addr_r;

reg        addressed_q;
reg        addressed_r;
reg [6:0]  device_addr_r;

reg        configured_q;
reg        configured_r;

reg        set_with_data_q;
reg        set_with_data_r;
wire       data_status_zlp_w;

always @ *
begin
    ctrl_stall_r    = 1'b0;
    ctrl_get_len_r  = 16'b0;
    ctrl_ack_r      = 1'b0;
    desc_addr_r     = 8'b0;
    device_addr_r   = device_addr_q;
    addressed_r     = addressed_q;
    configured_r    = configured_q;
    set_with_data_r = set_with_data_q;

    if (setup_valid_q)
    begin
        set_with_data_r = 1'b0;

        case (bmRequestType_w & `USB_REQUEST_TYPE_MASK)
        `USB_STANDARD_REQUEST:
        begin
            case (bRequest_w)
            `REQ_GET_STATUS:
            begin
                $display("GET_STATUS");
            end
            `REQ_CLEAR_FEATURE:
            begin
                $display("CLEAR_FEATURE");
                ctrl_ack_r = setup_set_w && setup_no_data_w;
            end
            `REQ_SET_FEATURE:
            begin
                $display("SET_FEATURE");
                ctrl_ack_r = setup_set_w && setup_no_data_w;
            end
            `REQ_SET_ADDRESS:
            begin
                $display("SET_ADDRESS: Set device address %d", wValue_w[6:0]);
                ctrl_ack_r    = setup_set_w && setup_no_data_w;
                device_addr_r = wValue_w[6:0];
                addressed_r   = 1'b1;
            end
            `REQ_GET_DESCRIPTOR:
            begin
                $display("GET_DESCRIPTOR: Type %d", bDescriptorType_w);

                case (bDescriptorType_w)
                `DESC_DEVICE:
                begin
                    desc_addr_r    = `ROM_DESC_DEVICE_ADDR;
                    ctrl_get_len_r = `ROM_DESC_DEVICE_SIZE;
                end
                `DESC_CONFIGURATION:
                begin
                    desc_addr_r    = `ROM_DESC_CONF_ADDR;
                    ctrl_get_len_r = `ROM_DESC_CONF_SIZE;
                end
                `DESC_STRING:
                begin
                    case (bDescriptorIndex_w)
                    `UNICODE_LANGUAGE_STR_ID:
                    begin
                        desc_addr_r    = `ROM_DESC_STR_LANG_ADDR;
                        ctrl_get_len_r = `ROM_DESC_STR_LANG_SIZE;
                    end
                    `MANUFACTURER_STR_ID:
                    begin
                        desc_addr_r    = `ROM_DESC_STR_MAN_ADDR;
                        ctrl_get_len_r = `ROM_DESC_STR_MAN_SIZE;
                    end
                    `PRODUCT_NAME_STR_ID:
                    begin
                        desc_addr_r    = `ROM_DESC_STR_PROD_ADDR;
                        ctrl_get_len_r = `ROM_DESC_STR_PROD_SIZE;
                    end
                    `SERIAL_NUM_STR_ID:
                    begin
                        desc_addr_r    = `ROM_DESC_STR_SERIAL_ADDR;
                        ctrl_get_len_r = `ROM_DESC_STR_SERIAL_SIZE;
                    end
                    default:
                        ;
                    endcase
                end
                default:
                    ;
                endcase
            end
            `REQ_GET_CONFIGURATION:
            begin
                $display("GET_CONF");
            end
            `REQ_SET_CONFIGURATION:
            begin
                $display("SET_CONF: Configuration %x", wValue_w);

                if (wValue_w == 16'd0)
                begin
                    configured_r = 1'b0;
                    ctrl_ack_r   = setup_set_w && setup_no_data_w;
                end
                // Only support one configuration for now
                else if (wValue_w == 16'd1)
                begin
                    configured_r = 1'b1;
                    ctrl_ack_r   = setup_set_w && setup_no_data_w;
                end
                else
                    ctrl_stall_r = 1'b1;
            end
            `REQ_GET_INTERFACE:
            begin
                $display("GET_INTERFACE");
                ctrl_stall_r = 1'b1;
            end
            `REQ_SET_INTERFACE:
            begin
                $display("SET_INTERFACE: %x %x", wValue_w, wIndex_w);
                if (wValue_w == 16'd0 && wIndex_w == 16'd0)
                    ctrl_ack_r   = setup_set_w && setup_no_data_w;
                else
                    ctrl_stall_r = 1'b1;
            end
            default:
            begin
                ctrl_stall_r = 1'b1;
            end
            endcase
        end
        `USB_VENDOR_REQUEST:
        begin
            // None supported
            ctrl_stall_r = 1'b1;
        end
        `USB_CLASS_REQUEST:
        begin
            case (bRequest_w)
            `CDC_GET_LINE_CODING:
            begin
                $display("CDC_GET_LINE_CODING");
                desc_addr_r    = `ROM_CDC_LINE_CODING_ADDR;
                ctrl_get_len_r = `ROM_CDC_LINE_CODING_SIZE;
            end
            default:
            begin
                ctrl_ack_r      = setup_set_w && setup_no_data_w;
                set_with_data_r = setup_set_w && !setup_no_data_w;
            end
            endcase
        end
        default:
        begin
            ctrl_stall_r = 1'b1;
        end
        endcase
    end
    else if (data_status_zlp_w)
        set_with_data_r = 1'b0;
end

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
begin
    device_addr_q   <= 7'b0;
    addressed_q     <= 1'b0;
    configured_q    <= 1'b0;
    set_with_data_q <= 1'b0;
end
else if (usb_reset_w)
begin
    device_addr_q   <= 7'b0;
    addressed_q     <= 1'b0;
    configured_q    <= 1'b0;
    set_with_data_q <= 1'b0;
end
else
begin
    device_addr_q   <= device_addr_r;
    addressed_q     <= addressed_r;
    configured_q    <= configured_r;
    set_with_data_q <= set_with_data_r;
end

//-----------------------------------------------------------------
// SETUP response
//-----------------------------------------------------------------
reg        ctrl_sending_q;
reg [15:0] ctrl_send_idx_q;
reg [15:0] ctrl_send_len_q;
wire       ctrl_send_zlp_w = ctrl_sending_q && (ctrl_send_len_q != wLength);

reg        ctrl_sending_r;
reg [15:0] ctrl_send_idx_r;
reg [15:0] ctrl_send_len_r;

reg        ctrl_txvalid_q;
reg [7:0]  ctrl_txdata_q;
reg        ctrl_txstrb_q;
reg        ctrl_txlast_q;
reg        ctrl_txstall_q;

reg        ctrl_txvalid_r;
reg [7:0]  ctrl_txdata_r;
reg        ctrl_txstrb_r;
reg        ctrl_txlast_r;
reg        ctrl_txstall_r;

wire       ctrl_send_accept_w = ep0_tx_data_accept_w || !ep0_tx_data_valid_w;

reg [7:0]  desc_addr_q;
wire[7:0]  desc_data_w;

always @ *
begin
    ctrl_sending_r  = ctrl_sending_q;
    ctrl_send_idx_r = ctrl_send_idx_q;
    ctrl_send_len_r = ctrl_send_len_q;

    ctrl_txvalid_r  = ctrl_txvalid_q;
    ctrl_txdata_r   = ctrl_txdata_q;
    ctrl_txstrb_r   = ctrl_txstrb_q;
    ctrl_txlast_r   = ctrl_txlast_q;
    ctrl_txstall_r  = ctrl_txstall_q;

    // New SETUP request
    if (setup_valid_q)
    begin
        // Send STALL
        if (ctrl_stall_r)
        begin
            ctrl_txvalid_r  = 1'b1;
            ctrl_txstrb_r   = 1'b0;
            ctrl_txlast_r   = 1'b1;
            ctrl_txstall_r  = 1'b1;
        end
        // Send STATUS response (ZLP)
        else if (ctrl_ack_r)
        begin
            ctrl_txvalid_r  = 1'b1;
            ctrl_txstrb_r   = 1'b0;
            ctrl_txlast_r   = 1'b1;
            ctrl_txstall_r  = 1'b0;
        end
        else
        begin
            ctrl_sending_r  = setup_get_w && !ctrl_stall_r;
            ctrl_send_idx_r = 16'b0;
            ctrl_send_len_r = ctrl_get_len_r;
            ctrl_txstall_r  = 1'b0;
        end
    end
    // Abort control send when STATUS received
    else if (status_ready_q)
    begin
        ctrl_sending_r  = 1'b0;
        ctrl_send_idx_r = 16'b0;
        ctrl_send_len_r = 16'b0;

        ctrl_txvalid_r  = 1'b0;
    end
    // Send STATUS response (ZLP)
    else if (set_with_data_q && setup_data_q)
    begin
        ctrl_txvalid_r  = 1'b1;
        ctrl_txstrb_r   = 1'b0;
        ctrl_txlast_r   = 1'b1;
        ctrl_txstall_r  = 1'b0;
    end
    else if (ctrl_sending_r && ctrl_send_accept_w)
    begin
        // TODO: Send ZLP on exact multiple lengths...
        ctrl_txvalid_r  = 1'b1;
        ctrl_txdata_r   = desc_data_w;
        ctrl_txstrb_r   = 1'b1;
        ctrl_txlast_r   = usb_hs_w ? (ctrl_send_idx_r[5:0] == 6'b111111) : (ctrl_send_idx_r[2:0] == 3'b111);

        // Increment send index
        ctrl_send_idx_r = ctrl_send_idx_r + 16'd1;

        // TODO: Detect need for ZLP
        if (ctrl_send_idx_r == wLength)
        begin
            ctrl_sending_r = 1'b0;
            ctrl_txlast_r  = 1'b1;
        end
    end
    else if (ctrl_send_accept_w)
        ctrl_txvalid_r  = 1'b0;
end

assign data_status_zlp_w = set_with_data_q && setup_data_q && ctrl_send_accept_w;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
begin
    ctrl_sending_q  <= 1'b0;
    ctrl_send_idx_q <= 16'b0;
    ctrl_send_len_q <= 16'b0;
    ctrl_txvalid_q  <= 1'b0;
    ctrl_txdata_q   <= 8'b0;
    ctrl_txstrb_q   <= 1'b0;
    ctrl_txlast_q   <= 1'b0;
    ctrl_txstall_q  <= 1'b0;
    desc_addr_q     <= 8'b0;
end
else if (usb_reset_w)
begin
    ctrl_sending_q  <= 1'b0;
    ctrl_send_idx_q <= 16'b0;
    ctrl_send_len_q <= 16'b0;
    ctrl_txvalid_q  <= 1'b0;
    ctrl_txdata_q   <= 8'b0;
    ctrl_txstrb_q   <= 1'b0;
    ctrl_txlast_q   <= 1'b0;
    ctrl_txstall_q  <= 1'b0;
    desc_addr_q     <= 8'b0;
end
else
begin
    ctrl_sending_q  <= ctrl_sending_r;
    ctrl_send_idx_q <= ctrl_send_idx_r;
    ctrl_send_len_q <= ctrl_send_len_r;
    ctrl_txvalid_q  <= ctrl_txvalid_r;
    ctrl_txdata_q   <= ctrl_txdata_r;
    ctrl_txstrb_q   <= ctrl_txstrb_r;
    ctrl_txlast_q   <= ctrl_txlast_r;
    ctrl_txstall_q  <= ctrl_txstall_r;

    if (setup_valid_q)
        desc_addr_q     <= desc_addr_r;
    else if (ctrl_sending_r && ctrl_send_accept_w)
        desc_addr_q     <= desc_addr_q + 8'd1;
end

assign ep0_tx_ready_w      = ctrl_txvalid_q;
assign ep0_tx_data_valid_w = ctrl_txvalid_q;
assign ep0_tx_data_strb_w  = ctrl_txstrb_q;
assign ep0_tx_data_w       = ctrl_txdata_q;
assign ep0_tx_data_last_w  = ctrl_txlast_q;
assign ep0_tx_stall_w      = ctrl_txstall_q;

//-----------------------------------------------------------------
// Descriptor ROM
//-----------------------------------------------------------------
usb_desc_rom
u_rom
(
    .hs_i(usb_hs_w),
    .addr_i(desc_addr_q),
    .data_o(desc_data_w)
);

//-----------------------------------------------------------------
// Unused Endpoints
//-----------------------------------------------------------------
assign ep1_tx_ready_w      = 1'b0;
assign ep1_tx_data_valid_w = 1'b0;
assign ep1_tx_data_strb_w  = 1'b0;
assign ep1_tx_data_w       = 8'b0;
assign ep1_tx_data_last_w  = 1'b0;
assign ep1_tx_stall_w      = 1'b0;
assign ep3_tx_ready_w      = 1'b0;
assign ep3_tx_data_valid_w = 1'b0;
assign ep3_tx_data_strb_w  = 1'b0;
assign ep3_tx_data_w       = 8'b0;
assign ep3_tx_data_last_w  = 1'b0;
assign ep3_tx_stall_w      = 1'b0;

assign ep2_rx_space_w      = 1'b0;
assign ep3_rx_space_w      = 1'b0;

//-----------------------------------------------------------------
// Stream I/O
//-----------------------------------------------------------------
reg        inport_valid_q;
reg [7:0]  inport_data_q;
reg [10:0] inport_cnt_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
begin
    inport_valid_q <= 1'b0;
    inport_data_q  <= 8'b0;
end
else if (inport_accept_o)
begin
    inport_valid_q <= inport_valid_i;
    inport_data_q  <= inport_data_i;
end

wire [10:0] max_packet_w   = usb_hs_w ? 11'd511 : 11'd63;
wire        inport_last_w  = !inport_valid_i || (inport_cnt_q == max_packet_w);

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    inport_cnt_q  <= 11'b0;
else if (inport_last_w && ep2_tx_data_accept_w)
    inport_cnt_q  <= 11'b0;
else if (inport_valid_q && ep2_tx_data_accept_w)
    inport_cnt_q  <= inport_cnt_q + 11'd1;

assign ep2_tx_data_valid_w = inport_valid_q;
assign ep2_tx_data_w       = inport_data_q;
assign ep2_tx_ready_w      = ep2_tx_data_valid_w;
assign ep2_tx_data_strb_w  = ep2_tx_data_valid_w;
assign ep2_tx_data_last_w  = inport_last_w;
assign inport_accept_o     = !inport_valid_q | ep2_tx_data_accept_w;

assign outport_valid_o  = ep1_rx_valid_w && rx_strb_w;
assign outport_data_o   = rx_data_w;
assign ep1_rx_space_w   = outport_accept_i;



endmodule
//-----------------------------------------------------------------
//                       USB Serial Port
//                            V0.1
//                     Ultra-Embedded.com
//                       Copyright 2020
//
//                 Email: admin@ultra-embedded.com
//
//                         License: LGPL
//-----------------------------------------------------------------
//
// This source file may be used and distributed without         
// restriction provided that this copyright statement is not    
// removed from the file and that any derivative work contains  
// the original copyright notice and the associated disclaimer. 
//
// This source file is free software; you can redistribute it   
// and/or modify it under the terms of the GNU Lesser General   
// Public License as published by the Free Software Foundation; 
// either version 2.1 of the License, or (at your option) any   
// later version.
//
// This source is distributed in the hope that it will be       
// useful, but WITHOUT ANY WARRANTY; without even the implied   
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      
// PURPOSE.  See the GNU Lesser General Public License for more 
// details.
//
// You should have received a copy of the GNU Lesser General    
// Public License along with this source; if not, write to the 
// Free Software Foundation, Inc., 59 Temple Place, Suite 330, 
// Boston, MA  02111-1307  USA
//-----------------------------------------------------------------

//-----------------------------------------------------------------
//                          Generated File
//-----------------------------------------------------------------
module usb_desc_rom
(
    input        hs_i,
    input  [7:0] addr_i,
    output [7:0] data_o
);

reg [7:0] desc_rom_r;

always @ *
begin
    case (addr_i)
    8'd0: desc_rom_r = 8'h12;
    8'd1: desc_rom_r = 8'h01;
    8'd2: desc_rom_r = 8'h00;
    8'd3: desc_rom_r = 8'h02;
    8'd4: desc_rom_r = 8'h02;
    8'd5: desc_rom_r = 8'h00;
    8'd6: desc_rom_r = 8'h00;
    8'd7: desc_rom_r = hs_i ? 8'h40 : 8'h08;
    8'd8: desc_rom_r = 8'h50;  // VID_L
    8'd9: desc_rom_r = 8'h1d;  // VID_H
    8'd10: desc_rom_r = 8'h49; // PID_L 
    8'd11: desc_rom_r = 8'h61; // PID_H
    8'd12: desc_rom_r = 8'h01;
    8'd13: desc_rom_r = 8'h01;
    8'd14: desc_rom_r = 8'h00;
    8'd15: desc_rom_r = 8'h00;
    8'd16: desc_rom_r = 8'h00;
    8'd17: desc_rom_r = 8'h01;
    8'd18: desc_rom_r = 8'h09;
    8'd19: desc_rom_r = 8'h02;
    8'd20: desc_rom_r = 8'h43;
    8'd21: desc_rom_r = 8'h00;
    8'd22: desc_rom_r = 8'h02;
    8'd23: desc_rom_r = 8'h01;
    8'd24: desc_rom_r = 8'h00;
    8'd25: desc_rom_r = 8'h80;
    8'd26: desc_rom_r = 8'h32;
    8'd27: desc_rom_r = 8'h09;
    8'd28: desc_rom_r = 8'h04;
    8'd29: desc_rom_r = 8'h00;
    8'd30: desc_rom_r = 8'h00;
    8'd31: desc_rom_r = 8'h01;
    8'd32: desc_rom_r = 8'h02;
    8'd33: desc_rom_r = 8'h02;
    8'd34: desc_rom_r = 8'h01;
    8'd35: desc_rom_r = 8'h00;
    8'd36: desc_rom_r = 8'h05;
    8'd37: desc_rom_r = 8'h24;
    8'd38: desc_rom_r = 8'h00;
    8'd39: desc_rom_r = 8'h10;
    8'd40: desc_rom_r = 8'h01;
    8'd41: desc_rom_r = 8'h05;
    8'd42: desc_rom_r = 8'h24;
    8'd43: desc_rom_r = 8'h01;
    8'd44: desc_rom_r = 8'h03;
    8'd45: desc_rom_r = 8'h01;
    8'd46: desc_rom_r = 8'h04;
    8'd47: desc_rom_r = 8'h24;
    8'd48: desc_rom_r = 8'h02;
    8'd49: desc_rom_r = 8'h06;
    8'd50: desc_rom_r = 8'h05;
    8'd51: desc_rom_r = 8'h24;
    8'd52: desc_rom_r = 8'h06;
    8'd53: desc_rom_r = 8'h00;
    8'd54: desc_rom_r = 8'h01;
    8'd55: desc_rom_r = 8'h07;
    8'd56: desc_rom_r = 8'h05;
    8'd57: desc_rom_r = 8'h83;
    8'd58: desc_rom_r = 8'h03;
    8'd59: desc_rom_r = 8'h40;
    8'd60: desc_rom_r = 8'h00;
    8'd61: desc_rom_r = 8'h02;
    8'd62: desc_rom_r = 8'h09;
    8'd63: desc_rom_r = 8'h04;
    8'd64: desc_rom_r = 8'h01;
    8'd65: desc_rom_r = 8'h00;
    8'd66: desc_rom_r = 8'h02;
    8'd67: desc_rom_r = 8'h0a;
    8'd68: desc_rom_r = 8'h00;
    8'd69: desc_rom_r = 8'h00;
    8'd70: desc_rom_r = 8'h00;
    8'd71: desc_rom_r = 8'h07;
    8'd72: desc_rom_r = 8'h05;
    8'd73: desc_rom_r = 8'h01;
    8'd74: desc_rom_r = 8'h02;
    8'd75: desc_rom_r = hs_i ? 8'h00 : 8'h40;
    8'd76: desc_rom_r = hs_i ? 8'h02 : 8'h00;
    8'd77: desc_rom_r = 8'h00;
    8'd78: desc_rom_r = 8'h07;
    8'd79: desc_rom_r = 8'h05;
    8'd80: desc_rom_r = 8'h82;
    8'd81: desc_rom_r = 8'h02;
    8'd82: desc_rom_r = hs_i ? 8'h00 : 8'h40;
    8'd83: desc_rom_r = hs_i ? 8'h02 : 8'h00;
    8'd84: desc_rom_r = 8'h00;
    8'd85: desc_rom_r = 8'h04;
    8'd86: desc_rom_r = 8'h03;
    8'd87: desc_rom_r = 8'h09;
    8'd88: desc_rom_r = 8'h04;
    8'd89: desc_rom_r = 8'h1e;
    8'd90: desc_rom_r = 8'h03;
    8'd91: desc_rom_r = 8'h55;
    8'd92: desc_rom_r = 8'h00;
    8'd93: desc_rom_r = 8'h4c;
    8'd94: desc_rom_r = 8'h00;
    8'd95: desc_rom_r = 8'h54;
    8'd96: desc_rom_r = 8'h00;
    8'd97: desc_rom_r = 8'h52;
    8'd98: desc_rom_r = 8'h00;
    8'd99: desc_rom_r = 8'h41;
    8'd100: desc_rom_r = 8'h00;
    8'd101: desc_rom_r = 8'h2d;
    8'd102: desc_rom_r = 8'h00;
    8'd103: desc_rom_r = 8'h45;
    8'd104: desc_rom_r = 8'h00;
    8'd105: desc_rom_r = 8'h4d;
    8'd106: desc_rom_r = 8'h00;
    8'd107: desc_rom_r = 8'h42;
    8'd108: desc_rom_r = 8'h00;
    8'd109: desc_rom_r = 8'h45;
    8'd110: desc_rom_r = 8'h00;
    8'd111: desc_rom_r = 8'h44;
    8'd112: desc_rom_r = 8'h00;
    8'd113: desc_rom_r = 8'h44;
    8'd114: desc_rom_r = 8'h00;
    8'd115: desc_rom_r = 8'h45;
    8'd116: desc_rom_r = 8'h00;
    8'd117: desc_rom_r = 8'h44;
    8'd118: desc_rom_r = 8'h00;
    8'd119: desc_rom_r = 8'h1e;
    8'd120: desc_rom_r = 8'h03;
    8'd121: desc_rom_r = 8'h55;
    8'd122: desc_rom_r = 8'h00;
    8'd123: desc_rom_r = 8'h53;
    8'd124: desc_rom_r = 8'h00;
    8'd125: desc_rom_r = 8'h42;
    8'd126: desc_rom_r = 8'h00;
    8'd127: desc_rom_r = 8'h20;
    8'd128: desc_rom_r = 8'h00;
    8'd129: desc_rom_r = 8'h44;
    8'd130: desc_rom_r = 8'h00;
    8'd131: desc_rom_r = 8'h45;
    8'd132: desc_rom_r = 8'h00;
    8'd133: desc_rom_r = 8'h4d;
    8'd134: desc_rom_r = 8'h00;
    8'd135: desc_rom_r = 8'h4f;
    8'd136: desc_rom_r = 8'h00;
    8'd137: desc_rom_r = 8'h20;
    8'd138: desc_rom_r = 8'h00;
    8'd139: desc_rom_r = 8'h20;
    8'd140: desc_rom_r = 8'h00;
    8'd141: desc_rom_r = 8'h20;
    8'd142: desc_rom_r = 8'h00;
    8'd143: desc_rom_r = 8'h20;
    8'd144: desc_rom_r = 8'h00;
    8'd145: desc_rom_r = 8'h20;
    8'd146: desc_rom_r = 8'h00;
    8'd147: desc_rom_r = 8'h20;
    8'd148: desc_rom_r = 8'h00;
    8'd149: desc_rom_r = 8'h0e;
    8'd150: desc_rom_r = 8'h03;
    8'd151: desc_rom_r = 8'h30;
    8'd152: desc_rom_r = 8'h00;
    8'd153: desc_rom_r = 8'h30;
    8'd154: desc_rom_r = 8'h00;
    8'd155: desc_rom_r = 8'h30;
    8'd156: desc_rom_r = 8'h00;
    8'd157: desc_rom_r = 8'h30;
    8'd158: desc_rom_r = 8'h00;
    8'd159: desc_rom_r = 8'h30;
    8'd160: desc_rom_r = 8'h00;
    8'd161: desc_rom_r = 8'h30;
    8'd162: desc_rom_r = 8'h00;
    8'd163: desc_rom_r = 8'h00;
    8'd164: desc_rom_r = 8'hc2;
    8'd165: desc_rom_r = 8'h01;
    8'd166: desc_rom_r = 8'h00;
    8'd167: desc_rom_r = 8'h00;
    8'd168: desc_rom_r = 8'h00;
    8'd169: desc_rom_r = 8'h08;
    default: desc_rom_r = 8'h00;
    endcase
end

assign data_o = desc_rom_r;

endmodule
//-----------------------------------------------------------------
//                       USB Serial Port
//                            V0.1
//                     Ultra-Embedded.com
//                       Copyright 2020
//
//                 Email: admin@ultra-embedded.com
//
//                         License: LGPL
//-----------------------------------------------------------------
//
// This source file may be used and distributed without         
// restriction provided that this copyright statement is not    
// removed from the file and that any derivative work contains  
// the original copyright notice and the associated disclaimer. 
//
// This source file is free software; you can redistribute it   
// and/or modify it under the terms of the GNU Lesser General   
// Public License as published by the Free Software Foundation; 
// either version 2.1 of the License, or (at your option) any   
// later version.
//
// This source is distributed in the hope that it will be       
// useful, but WITHOUT ANY WARRANTY; without even the implied   
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      
// PURPOSE.  See the GNU Lesser General Public License for more 
// details.
//
// You should have received a copy of the GNU Lesser General    
// Public License along with this source; if not, write to the 
// Free Software Foundation, Inc., 59 Temple Place, Suite 330, 
// Boston, MA  02111-1307  USA
//-----------------------------------------------------------------

//-----------------------------------------------------------------
//                          Generated File
//-----------------------------------------------------------------

module usbf_crc16
(
    // Inputs
     input  [ 15:0]  crc_in_i
    ,input  [  7:0]  din_i

    // Outputs
    ,output [ 15:0]  crc_out_o
);



//-----------------------------------------------------------------
// Logic
//-----------------------------------------------------------------
assign crc_out_o[15] =    din_i[0] ^ din_i[1] ^ din_i[2] ^ din_i[3] ^ din_i[4] ^ din_i[5] ^ din_i[6] ^ din_i[7] ^ 
                        crc_in_i[7] ^ crc_in_i[6] ^ crc_in_i[5] ^ crc_in_i[4] ^ crc_in_i[3] ^ crc_in_i[2] ^ crc_in_i[1] ^ crc_in_i[0];
assign crc_out_o[14] =    din_i[0] ^ din_i[1] ^ din_i[2] ^ din_i[3] ^ din_i[4] ^ din_i[5] ^ din_i[6] ^
                        crc_in_i[6] ^ crc_in_i[5] ^ crc_in_i[4] ^ crc_in_i[3] ^ crc_in_i[2] ^ crc_in_i[1] ^ crc_in_i[0];
assign crc_out_o[13] =    din_i[6] ^ din_i[7] ^ 
                        crc_in_i[7] ^ crc_in_i[6];
assign crc_out_o[12] =    din_i[5] ^ din_i[6] ^ 
                        crc_in_i[6] ^ crc_in_i[5];
assign crc_out_o[11] =    din_i[4] ^ din_i[5] ^ 
                        crc_in_i[5] ^ crc_in_i[4];
assign crc_out_o[10] =    din_i[3] ^ din_i[4] ^ 
                        crc_in_i[4] ^ crc_in_i[3];
assign crc_out_o[9] =     din_i[2] ^ din_i[3] ^ 
                        crc_in_i[3] ^ crc_in_i[2];
assign crc_out_o[8] =     din_i[1] ^ din_i[2] ^ 
                        crc_in_i[2] ^ crc_in_i[1];
assign crc_out_o[7] =     din_i[0] ^ din_i[1] ^ 
                        crc_in_i[15] ^ crc_in_i[1] ^ crc_in_i[0];
assign crc_out_o[6] =     din_i[0] ^ 
                        crc_in_i[14] ^ crc_in_i[0];
assign crc_out_o[5] =     crc_in_i[13];
assign crc_out_o[4] =     crc_in_i[12];
assign crc_out_o[3] =     crc_in_i[11];
assign crc_out_o[2] =     crc_in_i[10];
assign crc_out_o[1] =     crc_in_i[9];
assign crc_out_o[0] =     din_i[0] ^ din_i[1] ^ din_i[2] ^ din_i[3] ^ din_i[4] ^ din_i[5] ^ din_i[6] ^ din_i[7] ^
                        crc_in_i[8] ^ crc_in_i[7] ^ crc_in_i[6] ^ crc_in_i[5] ^ crc_in_i[4] ^ crc_in_i[3] ^ crc_in_i[2] ^ crc_in_i[1] ^ crc_in_i[0];


endmodule
//-----------------------------------------------------------------
//                       USB Serial Port
//                            V0.1
//                     Ultra-Embedded.com
//                       Copyright 2020
//
//                 Email: admin@ultra-embedded.com
//
//                         License: LGPL
//-----------------------------------------------------------------
//
// This source file may be used and distributed without         
// restriction provided that this copyright statement is not    
// removed from the file and that any derivative work contains  
// the original copyright notice and the associated disclaimer. 
//
// This source file is free software; you can redistribute it   
// and/or modify it under the terms of the GNU Lesser General   
// Public License as published by the Free Software Foundation; 
// either version 2.1 of the License, or (at your option) any   
// later version.
//
// This source is distributed in the hope that it will be       
// useful, but WITHOUT ANY WARRANTY; without even the implied   
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      
// PURPOSE.  See the GNU Lesser General Public License for more 
// details.
//
// You should have received a copy of the GNU Lesser General    
// Public License along with this source; if not, write to the 
// Free Software Foundation, Inc., 59 Temple Place, Suite 330, 
// Boston, MA  02111-1307  USA
//-----------------------------------------------------------------

//-----------------------------------------------------------------
//                          Generated File
//-----------------------------------------------------------------

module usbf_device_core
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input  [  7:0]  utmi_data_i
    ,input           utmi_txready_i
    ,input           utmi_rxvalid_i
    ,input           utmi_rxactive_i
    ,input           utmi_rxerror_i
    ,input  [  1:0]  utmi_linestate_i
    ,input           ep0_stall_i
    ,input           ep0_iso_i
    ,input           ep0_cfg_int_rx_i
    ,input           ep0_cfg_int_tx_i
    ,input           ep0_rx_space_i
    ,input           ep0_tx_ready_i
    ,input           ep0_tx_data_valid_i
    ,input           ep0_tx_data_strb_i
    ,input  [  7:0]  ep0_tx_data_i
    ,input           ep0_tx_data_last_i
    ,input           ep1_stall_i
    ,input           ep1_iso_i
    ,input           ep1_cfg_int_rx_i
    ,input           ep1_cfg_int_tx_i
    ,input           ep1_rx_space_i
    ,input           ep1_tx_ready_i
    ,input           ep1_tx_data_valid_i
    ,input           ep1_tx_data_strb_i
    ,input  [  7:0]  ep1_tx_data_i
    ,input           ep1_tx_data_last_i
    ,input           ep2_stall_i
    ,input           ep2_iso_i
    ,input           ep2_cfg_int_rx_i
    ,input           ep2_cfg_int_tx_i
    ,input           ep2_rx_space_i
    ,input           ep2_tx_ready_i
    ,input           ep2_tx_data_valid_i
    ,input           ep2_tx_data_strb_i
    ,input  [  7:0]  ep2_tx_data_i
    ,input           ep2_tx_data_last_i
    ,input           ep3_stall_i
    ,input           ep3_iso_i
    ,input           ep3_cfg_int_rx_i
    ,input           ep3_cfg_int_tx_i
    ,input           ep3_rx_space_i
    ,input           ep3_tx_ready_i
    ,input           ep3_tx_data_valid_i
    ,input           ep3_tx_data_strb_i
    ,input  [  7:0]  ep3_tx_data_i
    ,input           ep3_tx_data_last_i
    ,input           reg_chirp_en_i
    ,input           reg_int_en_sof_i
    ,input           reg_sts_rst_clr_i
    ,input  [  6:0]  reg_dev_addr_i

    // Outputs
    ,output          intr_o
    ,output [  7:0]  utmi_data_o
    ,output          utmi_txvalid_o
    ,output          rx_strb_o
    ,output [  7:0]  rx_data_o
    ,output          rx_last_o
    ,output          rx_crc_err_o
    ,output          ep0_rx_setup_o
    ,output          ep0_rx_valid_o
    ,output          ep0_tx_data_accept_o
    ,output          ep1_rx_setup_o
    ,output          ep1_rx_valid_o
    ,output          ep1_tx_data_accept_o
    ,output          ep2_rx_setup_o
    ,output          ep2_rx_valid_o
    ,output          ep2_tx_data_accept_o
    ,output          ep3_rx_setup_o
    ,output          ep3_rx_valid_o
    ,output          ep3_tx_data_accept_o
    ,output          reg_sts_rst_o
    ,output [ 10:0]  reg_sts_frame_num_o
);



//-----------------------------------------------------------------
// Defines:
//-----------------------------------------------------------------
// `include "usbf_defs.v"

`define USB_RESET_CNT_W     15

localparam STATE_W                       = 3;
localparam STATE_RX_IDLE                 = 3'd0;
localparam STATE_RX_DATA                 = 3'd1;
localparam STATE_RX_DATA_READY           = 3'd2;
localparam STATE_RX_DATA_IGNORE          = 3'd3;
localparam STATE_TX_DATA                 = 3'd4;
localparam STATE_TX_DATA_COMPLETE        = 3'd5;
localparam STATE_TX_HANDSHAKE            = 3'd6;
localparam STATE_TX_CHIRP                = 3'd7;
reg [STATE_W-1:0] state_q;

//-----------------------------------------------------------------
// Reset detection
//-----------------------------------------------------------------
reg [`USB_RESET_CNT_W-1:0] se0_cnt_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    se0_cnt_q <= `USB_RESET_CNT_W'b0;
else if (utmi_linestate_i == 2'b0)
begin
    if (!se0_cnt_q[`USB_RESET_CNT_W-1])
        se0_cnt_q <= se0_cnt_q + `USB_RESET_CNT_W'd1;
end    
else
    se0_cnt_q <= `USB_RESET_CNT_W'b0;

wire usb_rst_w = se0_cnt_q[`USB_RESET_CNT_W-1];

//-----------------------------------------------------------------
// Wire / Regs
//-----------------------------------------------------------------
`define USB_FRAME_W    11
wire [`USB_FRAME_W-1:0] frame_num_w;

wire                    frame_valid_w;

`define USB_DEV_W      7
wire [`USB_DEV_W-1:0]   token_dev_w;

`define USB_EP_W       4
wire [`USB_EP_W-1:0]    token_ep_w;

`define USB_PID_W      8
wire [`USB_PID_W-1:0]   token_pid_w;

wire                    token_valid_w;

wire                    rx_data_valid_w;
wire                    rx_data_complete_w;

wire                    rx_handshake_w;

reg                     tx_data_valid_r;
reg                     tx_data_strb_r;
reg  [7:0]              tx_data_r;
reg                     tx_data_last_r;
wire                    tx_data_accept_w;

reg                     tx_valid_q;
reg [7:0]               tx_pid_q;
wire                    tx_accept_w;

reg                     rx_space_q;
reg                     rx_space_r;
reg                     tx_ready_r;
reg                     out_data_bit_r;
reg                     in_data_bit_r;

reg                     ep_stall_r;
reg                     ep_iso_r;

reg                     rx_enable_q;
reg                     rx_setup_q;

reg                     ep0_out_data_bit_q;
reg                     ep0_in_data_bit_q;
reg                     ep1_out_data_bit_q;
reg                     ep1_in_data_bit_q;
reg                     ep2_out_data_bit_q;
reg                     ep2_in_data_bit_q;
reg                     ep3_out_data_bit_q;
reg                     ep3_in_data_bit_q;

reg [`USB_DEV_W-1:0]    current_addr_q;

//-----------------------------------------------------------------
// SIE - TX
//-----------------------------------------------------------------
usbf_sie_tx
u_sie_tx
(
    .clk_i(clk_i),
    .rst_i(rst_i),
    
    .enable_i(~usb_rst_w),
    .chirp_i(reg_chirp_en_i),

    // UTMI Interface
    .utmi_data_o(utmi_data_o),
    .utmi_txvalid_o(utmi_txvalid_o),
    .utmi_txready_i(utmi_txready_i),

    // Request
    .tx_valid_i(tx_valid_q),
    .tx_pid_i(tx_pid_q),
    .tx_accept_o(tx_accept_w),

    // Data
    .data_valid_i(tx_data_valid_r),
    .data_strb_i(tx_data_strb_r),
    .data_i(tx_data_r),
    .data_last_i(tx_data_last_r),
    .data_accept_o(tx_data_accept_w)
);

always @ *
begin
    tx_data_valid_r = 1'b0;
    tx_data_strb_r  = 1'b0;
    tx_data_r       = 8'b0;
    tx_data_last_r  = 1'b0;

    case (token_ep_w)
    4'd0:
    begin
        tx_data_valid_r = ep0_tx_data_valid_i;
        tx_data_strb_r  = ep0_tx_data_strb_i;
        tx_data_r       = ep0_tx_data_i;
        tx_data_last_r  = ep0_tx_data_last_i;
    end
    4'd1:
    begin
        tx_data_valid_r = ep1_tx_data_valid_i;
        tx_data_strb_r  = ep1_tx_data_strb_i;
        tx_data_r       = ep1_tx_data_i;
        tx_data_last_r  = ep1_tx_data_last_i;
    end
    4'd2:
    begin
        tx_data_valid_r = ep2_tx_data_valid_i;
        tx_data_strb_r  = ep2_tx_data_strb_i;
        tx_data_r       = ep2_tx_data_i;
        tx_data_last_r  = ep2_tx_data_last_i;
    end
    4'd3:
    begin
        tx_data_valid_r = ep3_tx_data_valid_i;
        tx_data_strb_r  = ep3_tx_data_strb_i;
        tx_data_r       = ep3_tx_data_i;
        tx_data_last_r  = ep3_tx_data_last_i;
    end
    default:
        ;
    endcase    
end

assign ep0_tx_data_accept_o = tx_data_accept_w & (token_ep_w == 4'd0);
assign ep1_tx_data_accept_o = tx_data_accept_w & (token_ep_w == 4'd1);
assign ep2_tx_data_accept_o = tx_data_accept_w & (token_ep_w == 4'd2);
assign ep3_tx_data_accept_o = tx_data_accept_w & (token_ep_w == 4'd3);

always @ *
begin
    rx_space_r     = 1'b0;
    tx_ready_r     = 1'b0;
    out_data_bit_r = 1'b0;
    in_data_bit_r  = 1'b0;

    ep_stall_r = 1'b0;
    ep_iso_r   = 1'b0;

    case (token_ep_w)
    4'd0:
    begin
        rx_space_r    = ep0_rx_space_i;
        tx_ready_r    = ep0_tx_ready_i;
        out_data_bit_r= ep0_out_data_bit_q;
        in_data_bit_r = ep0_in_data_bit_q;
        ep_stall_r    = ep0_stall_i;
        ep_iso_r      = ep0_iso_i;
    end
    4'd1:
    begin
        rx_space_r    = ep1_rx_space_i;
        tx_ready_r    = ep1_tx_ready_i;
        out_data_bit_r= ep1_out_data_bit_q;
        in_data_bit_r = ep1_in_data_bit_q;
        ep_stall_r    = ep1_stall_i;
        ep_iso_r      = ep1_iso_i;
    end
    4'd2:
    begin
        rx_space_r    = ep2_rx_space_i;
        tx_ready_r    = ep2_tx_ready_i;
        out_data_bit_r= ep2_out_data_bit_q;
        in_data_bit_r = ep2_in_data_bit_q;
        ep_stall_r    = ep2_stall_i;
        ep_iso_r      = ep2_iso_i;
    end
    4'd3:
    begin
        rx_space_r    = ep3_rx_space_i;
        tx_ready_r    = ep3_tx_ready_i;
        out_data_bit_r= ep3_out_data_bit_q;
        in_data_bit_r = ep3_in_data_bit_q;
        ep_stall_r    = ep3_stall_i;
        ep_iso_r      = ep3_iso_i;
    end
    default:
        ;
    endcase
end

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    rx_space_q <= 1'b0;
else if (state_q == STATE_RX_IDLE)
    rx_space_q <= rx_space_r;

//-----------------------------------------------------------------
// SIE - RX
//-----------------------------------------------------------------
usbf_sie_rx
u_sie_rx
(
    .clk_i(clk_i),
    .rst_i(rst_i),
    
    .enable_i(~usb_rst_w && ~reg_chirp_en_i),

    // UTMI Interface
    .utmi_data_i(utmi_data_i),
    .utmi_rxvalid_i(utmi_rxvalid_i),
    .utmi_rxactive_i(utmi_rxactive_i),

    .current_addr_i(current_addr_q),

    .pid_o(token_pid_w),

    .frame_valid_o(frame_valid_w),
    .frame_number_o(reg_sts_frame_num_o),

    .token_valid_o(token_valid_w),
    .token_addr_o(token_dev_w),
    .token_ep_o(token_ep_w),
    .token_crc_err_o(),

    .handshake_valid_o(rx_handshake_w),

    .data_valid_o(rx_data_valid_w),
    .data_strb_o(rx_strb_o),
    .data_o(rx_data_o),
    .data_last_o(rx_last_o),

    .data_complete_o(rx_data_complete_w),
    .data_crc_err_o(rx_crc_err_o)
);

assign ep0_rx_valid_o = rx_enable_q & rx_data_valid_w & (token_ep_w == 4'd0);
assign ep0_rx_setup_o = rx_setup_q & (token_ep_w == 4'd0);
assign ep1_rx_valid_o = rx_enable_q & rx_data_valid_w & (token_ep_w == 4'd1);
assign ep1_rx_setup_o = rx_setup_q & (token_ep_w == 4'd0);
assign ep2_rx_valid_o = rx_enable_q & rx_data_valid_w & (token_ep_w == 4'd2);
assign ep2_rx_setup_o = rx_setup_q & (token_ep_w == 4'd0);
assign ep3_rx_valid_o = rx_enable_q & rx_data_valid_w & (token_ep_w == 4'd3);
assign ep3_rx_setup_o = rx_setup_q & (token_ep_w == 4'd0);

//-----------------------------------------------------------------
// Next state
//-----------------------------------------------------------------
reg [STATE_W-1:0] next_state_r;

always @ *
begin
    next_state_r = state_q;

    //-----------------------------------------
    // State Machine
    //-----------------------------------------
    case (state_q)

    //-----------------------------------------
    // IDLE
    //-----------------------------------------
    STATE_RX_IDLE :
    begin
        // Token received (OUT, IN, SETUP, PING)
        if (token_valid_w)
        begin
            //-------------------------------
            // IN transfer (device -> host)
            //-------------------------------
            if (token_pid_w == `PID_IN)
            begin
                // Stalled endpoint?
                if (ep_stall_r)
                    next_state_r  = STATE_TX_HANDSHAKE;
                // Some data to TX?
                else if (tx_ready_r)
                    next_state_r  = STATE_TX_DATA;
                // No data to TX
                else
                    next_state_r  = STATE_TX_HANDSHAKE;
            end
            //-------------------------------
            // PING transfer (device -> host)
            //-------------------------------
            else if (token_pid_w == `PID_PING)
            begin
                next_state_r  = STATE_TX_HANDSHAKE;
            end
            //-------------------------------
            // OUT transfer (host -> device)
            //-------------------------------
            else if (token_pid_w == `PID_OUT)
            begin
                // Stalled endpoint?
                if (ep_stall_r)
                    next_state_r  = STATE_RX_DATA_IGNORE;
                // Some space to rx
                else if (rx_space_r)
                    next_state_r  = STATE_RX_DATA;
                // No rx space, ignore receive
                else
                    next_state_r  = STATE_RX_DATA_IGNORE;
            end
            //-------------------------------
            // SETUP transfer (host -> device)
            //-------------------------------
            else if (token_pid_w == `PID_SETUP)
            begin
                // Some space to rx
                if (rx_space_r)
                    next_state_r  = STATE_RX_DATA;
                // No rx space, ignore receive
                else
                    next_state_r  = STATE_RX_DATA_IGNORE;
            end
        end
        else if (reg_chirp_en_i)
            next_state_r  = STATE_TX_CHIRP;
    end

    //-----------------------------------------
    // RX_DATA
    //-----------------------------------------
    STATE_RX_DATA :
    begin
        // TODO: Exit data state handling?

        // TODO: Sort out ISO data bit handling
        // Check for expected DATAx PID
        if ((token_pid_w == `PID_DATA0 &&  out_data_bit_r && !ep_iso_r) ||
            (token_pid_w == `PID_DATA1 && !out_data_bit_r && !ep_iso_r))
            next_state_r  = STATE_RX_DATA_IGNORE;
        // Receive complete
        else if (rx_data_valid_w && rx_last_o)
            next_state_r  = STATE_RX_DATA_READY;
    end
    //-----------------------------------------
    // RX_DATA_IGNORE
    //-----------------------------------------
    STATE_RX_DATA_IGNORE :
    begin
        // Receive complete
        if (rx_data_valid_w && rx_last_o)
            next_state_r  = STATE_RX_DATA_READY;
    end
    //-----------------------------------------
    // RX_DATA_READY
    //-----------------------------------------
    STATE_RX_DATA_READY :
    begin
        if (rx_data_complete_w)
        begin
            // No response on CRC16 error
            if (rx_crc_err_o)
                next_state_r  = STATE_RX_IDLE;
            // ISO endpoint, no response?
            else if (ep_iso_r)
                next_state_r  = STATE_RX_IDLE;
            else
                next_state_r  = STATE_TX_HANDSHAKE;
        end
    end
    //-----------------------------------------
    // TX_DATA
    //-----------------------------------------
    STATE_TX_DATA :
    begin
        if (!tx_valid_q || tx_accept_w)
            if (tx_data_valid_r && tx_data_last_r && tx_data_accept_w)
                next_state_r  = STATE_TX_DATA_COMPLETE;
    end
    //-----------------------------------------
    // TX_HANDSHAKE
    //-----------------------------------------
    STATE_TX_DATA_COMPLETE :
    begin
        next_state_r  = STATE_RX_IDLE;
    end
    //-----------------------------------------
    // TX_HANDSHAKE
    //-----------------------------------------
    STATE_TX_HANDSHAKE :
    begin
        if (tx_accept_w)
            next_state_r  = STATE_RX_IDLE;
    end
    //-----------------------------------------
    // TX_CHIRP
    //-----------------------------------------
    STATE_TX_CHIRP :
    begin
        if (!reg_chirp_en_i)
            next_state_r  = STATE_RX_IDLE;
    end

    default :
       ;

    endcase

    //-----------------------------------------
    // USB Bus Reset (HOST->DEVICE)
    //----------------------------------------- 
    if (usb_rst_w && !reg_chirp_en_i)
        next_state_r  = STATE_RX_IDLE;
end

// Update state
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    state_q   <= STATE_RX_IDLE;
else
    state_q   <= next_state_r;

//-----------------------------------------------------------------
// Response
//-----------------------------------------------------------------
reg         tx_valid_r;
reg [7:0]   tx_pid_r;

always @ *
begin
    tx_valid_r = 1'b0;
    tx_pid_r   = 8'b0;

    case (state_q)
    //-----------------------------------------
    // IDLE
    //-----------------------------------------
    STATE_RX_IDLE :
    begin
        // Token received (OUT, IN, SETUP, PING)
        if (token_valid_w)
        begin
            //-------------------------------
            // IN transfer (device -> host)
            //-------------------------------
            if (token_pid_w == `PID_IN)
            begin
                // Stalled endpoint?
                if (ep_stall_r)
                begin
                    tx_valid_r = 1'b1;
                    tx_pid_r   = `PID_STALL;
                end
                // Some data to TX?
                else if (tx_ready_r)
                begin
                    tx_valid_r = 1'b1;
                    // TODO: Handle MDATA for ISOs
                    tx_pid_r   = in_data_bit_r ? `PID_DATA1 : `PID_DATA0;
                end
                // No data to TX
                else
                begin
                    tx_valid_r = 1'b1;
                    tx_pid_r   = `PID_NAK;
                end
            end
            //-------------------------------
            // PING transfer (device -> host)
            //-------------------------------
            else if (token_pid_w == `PID_PING)
            begin
                // Stalled endpoint?
                if (ep_stall_r)
                begin
                    tx_valid_r = 1'b1;
                    tx_pid_r   = `PID_STALL;
                end
                // Data ready to RX
                else if (rx_space_r)
                begin
                    tx_valid_r = 1'b1;
                    tx_pid_r   = `PID_ACK;
                end
                // No data to TX
                else
                begin
                    tx_valid_r = 1'b1;
                    tx_pid_r   = `PID_NAK;
                end
            end
        end
    end

    //-----------------------------------------
    // RX_DATA_READY
    //-----------------------------------------
    STATE_RX_DATA_READY :
    begin
       // Receive complete
       if (rx_data_complete_w)
       begin
            // No response on CRC16 error
            if (rx_crc_err_o)
                ;
            // ISO endpoint, no response?
            else if (ep_iso_r)
                ;
            // Send STALL?
            else if (ep_stall_r)
            begin
                tx_valid_r = 1'b1;
                tx_pid_r   = `PID_STALL;
            end
            // DATAx bit mismatch
            else if ( (token_pid_w == `PID_DATA0 && out_data_bit_r) ||
                      (token_pid_w == `PID_DATA1 && !out_data_bit_r) )
            begin
                // Ack transfer to resync
                tx_valid_r = 1'b1;
                tx_pid_r   = `PID_ACK;
            end
            // Send NAK
            else if (!rx_space_q)
            begin
                tx_valid_r = 1'b1;
                tx_pid_r   = `PID_NAK;
            end
            // TODO: USB 2.0, no more buffer space, return NYET
            else
            begin
                tx_valid_r = 1'b1;
                tx_pid_r   = `PID_ACK;
            end
       end
    end

    //-----------------------------------------
    // TX_CHIRP
    //-----------------------------------------
    STATE_TX_CHIRP :
    begin
        tx_valid_r = 1'b1;
        tx_pid_r   = 8'b0;
    end

    default :
       ;

    endcase
end

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    tx_valid_q <= 1'b0;
else if (!tx_valid_q || tx_accept_w)
    tx_valid_q <= tx_valid_r;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    tx_pid_q <= 8'b0;
else if (!tx_valid_q || tx_accept_w)
    tx_pid_q <= tx_pid_r;

//-----------------------------------------------------------------
// Receive enable
//-----------------------------------------------------------------
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    rx_enable_q <= 1'b0;
else if (usb_rst_w ||reg_chirp_en_i)
    rx_enable_q <= 1'b0;
else
    rx_enable_q <= (state_q == STATE_RX_DATA);

//-----------------------------------------------------------------
// Receive SETUP: Pulse on SETUP packet receive
//-----------------------------------------------------------------
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    rx_setup_q <= 1'b0;
else if (usb_rst_w ||reg_chirp_en_i)
    rx_setup_q <= 1'b0;
else if ((state_q == STATE_RX_IDLE) && token_valid_w && (token_pid_w == `PID_SETUP) && (token_ep_w == 4'd0))
    rx_setup_q <= 1'b1;
else
    rx_setup_q <= 1'b0;

//-----------------------------------------------------------------
// Set Address
//-----------------------------------------------------------------
reg addr_update_pending_q;

wire ep0_tx_zlp_w = ep0_tx_data_valid_i && (ep0_tx_data_strb_i == 1'b0) && 
                    ep0_tx_data_last_i && ep0_tx_data_accept_o;

reg sent_status_zlp_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    sent_status_zlp_q   <= 1'b0;
else if (usb_rst_w)
    sent_status_zlp_q   <= 1'b0;
else if (ep0_tx_zlp_w)
    sent_status_zlp_q   <= 1'b1;
else if (rx_handshake_w)
    sent_status_zlp_q   <= 1'b0;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    addr_update_pending_q   <= 1'b0;
else if ((sent_status_zlp_q && addr_update_pending_q && rx_handshake_w && token_pid_w == `PID_ACK) || usb_rst_w)
    addr_update_pending_q   <= 1'b0;
// TODO: Use write strobe
else if (reg_dev_addr_i != current_addr_q)
    addr_update_pending_q   <= 1'b1;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    current_addr_q  <= `USB_DEV_W'b0;
else if (usb_rst_w)
    current_addr_q  <= `USB_DEV_W'b0;
else if (sent_status_zlp_q && addr_update_pending_q && rx_handshake_w && token_pid_w == `PID_ACK)
    current_addr_q  <= reg_dev_addr_i;

//-----------------------------------------------------------------
// Endpoint data bit toggle
//-----------------------------------------------------------------
reg new_out_bit_r;
reg new_in_bit_r;

always @ *
begin
    new_out_bit_r = out_data_bit_r;
    new_in_bit_r  = in_data_bit_r;

    case (state_q)
    //-----------------------------------------
    // RX_DATA_READY
    //-----------------------------------------
    STATE_RX_DATA_READY :
    begin
       // Receive complete
       if (rx_data_complete_w)
       begin
            // No toggle on CRC16 error
            if (rx_crc_err_o)
                ;
            // ISO endpoint, no response?
            else if (ep_iso_r)
                ; // TODO: HS handling
            // STALL?
            else if (ep_stall_r)
                ;
            // DATAx bit mismatch
            else if ( (token_pid_w == `PID_DATA0 && out_data_bit_r) ||
                      (token_pid_w == `PID_DATA1 && !out_data_bit_r) )
                ;
            // NAKd
            else if (!rx_space_q)
                ;
            // Data accepted - toggle data bit
            else
                new_out_bit_r = !out_data_bit_r;
       end
    end
    //-----------------------------------------
    // RX_IDLE
    //-----------------------------------------
    STATE_RX_IDLE :
    begin
        // Token received (OUT, IN, SETUP, PING)
        if (token_valid_w)
        begin
            // SETUP packets always start with DATA0
            if (token_pid_w == `PID_SETUP)
            begin
                new_out_bit_r = 1'b0;
                new_in_bit_r  = 1'b1;
            end
        end
        // ACK received
        else if (rx_handshake_w && token_pid_w == `PID_ACK)
        begin
            new_in_bit_r = !in_data_bit_r;
        end
    end
    default:
        ;
    endcase
end

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
begin
    ep0_out_data_bit_q <= 1'b0;
    ep0_in_data_bit_q  <= 1'b0;
end
else if (usb_rst_w)
begin
    ep0_out_data_bit_q <= 1'b0;
    ep0_in_data_bit_q  <= 1'b0;
end
else if (token_ep_w == 4'd0)
begin
    ep0_out_data_bit_q <= new_out_bit_r;
    ep0_in_data_bit_q  <= new_in_bit_r;
end
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
begin
    ep1_out_data_bit_q <= 1'b0;
    ep1_in_data_bit_q  <= 1'b0;
end
else if (usb_rst_w)
begin
    ep1_out_data_bit_q <= 1'b0;
    ep1_in_data_bit_q  <= 1'b0;
end
else if (token_ep_w == 4'd1)
begin
    ep1_out_data_bit_q <= new_out_bit_r;
    ep1_in_data_bit_q  <= new_in_bit_r;
end
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
begin
    ep2_out_data_bit_q <= 1'b0;
    ep2_in_data_bit_q  <= 1'b0;
end
else if (usb_rst_w)
begin
    ep2_out_data_bit_q <= 1'b0;
    ep2_in_data_bit_q  <= 1'b0;
end
else if (token_ep_w == 4'd2)
begin
    ep2_out_data_bit_q <= new_out_bit_r;
    ep2_in_data_bit_q  <= new_in_bit_r;
end
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
begin
    ep3_out_data_bit_q <= 1'b0;
    ep3_in_data_bit_q  <= 1'b0;
end
else if (usb_rst_w)
begin
    ep3_out_data_bit_q <= 1'b0;
    ep3_in_data_bit_q  <= 1'b0;
end
else if (token_ep_w == 4'd3)
begin
    ep3_out_data_bit_q <= new_out_bit_r;
    ep3_in_data_bit_q  <= new_in_bit_r;
end

//-----------------------------------------------------------------
// Reset event
//-----------------------------------------------------------------
reg rst_event_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    rst_event_q <= 1'b0;
else if (usb_rst_w)
    rst_event_q <= 1'b1;
else if (reg_sts_rst_clr_i)
    rst_event_q <= 1'b0;

assign reg_sts_rst_o = rst_event_q;

//-----------------------------------------------------------------
// Interrupts
//-----------------------------------------------------------------
reg intr_q;

reg cfg_int_rx_r;
reg cfg_int_tx_r;

always @ *
begin
    cfg_int_rx_r = 1'b0;
    cfg_int_tx_r = 1'b0;

    case (token_ep_w)
    4'd0:
    begin
        cfg_int_rx_r = ep0_cfg_int_rx_i;
        cfg_int_tx_r = ep0_cfg_int_tx_i;
    end
    4'd1:
    begin
        cfg_int_rx_r = ep1_cfg_int_rx_i;
        cfg_int_tx_r = ep1_cfg_int_tx_i;
    end
    4'd2:
    begin
        cfg_int_rx_r = ep2_cfg_int_rx_i;
        cfg_int_tx_r = ep2_cfg_int_tx_i;
    end
    4'd3:
    begin
        cfg_int_rx_r = ep3_cfg_int_rx_i;
        cfg_int_tx_r = ep3_cfg_int_tx_i;
    end
    default:
        ;
    endcase
end

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    intr_q <= 1'b0;
// SOF
else if (frame_valid_w && reg_int_en_sof_i)
    intr_q <= 1'b1;
// Reset event
else if (!rst_event_q && usb_rst_w)
    intr_q <= 1'b1;
// Rx ready
else if (state_q == STATE_RX_DATA_READY && rx_space_q && cfg_int_rx_r)
    intr_q <= 1'b1;
// Tx complete
else if (state_q == STATE_TX_DATA_COMPLETE && cfg_int_tx_r)
    intr_q <= 1'b1;    
else
    intr_q <= 1'b0;

assign intr_o = intr_q;

//-------------------------------------------------------------------
// Debug
//-------------------------------------------------------------------
`ifdef verilator
/* verilator lint_off WIDTH */
reg [79:0] dbg_state;

always @ *
begin
    dbg_state = "-";

    case (state_q)
    STATE_RX_IDLE: dbg_state = "IDLE";
    STATE_RX_DATA: dbg_state = "RX_DATA";
    STATE_RX_DATA_READY: dbg_state = "RX_DATA_READY";
    STATE_RX_DATA_IGNORE: dbg_state = "RX_IGNORE";
    STATE_TX_DATA: dbg_state = "TX_DATA";
    STATE_TX_DATA_COMPLETE: dbg_state = "TX_DATA_COMPLETE";
    STATE_TX_HANDSHAKE: dbg_state = "TX_HANDSHAKE";
    STATE_TX_CHIRP: dbg_state = "CHIRP";
    endcase
end

reg [79:0] dbg_pid;
reg [7:0]  dbg_pid_r;
always @ *
begin
    dbg_pid = "-";

    if (tx_valid_q && tx_accept_w)
        dbg_pid_r = tx_pid_q;
    else if (token_valid_w || rx_handshake_w || rx_data_valid_w)
        dbg_pid_r = token_pid_w;
    else
        dbg_pid_r = 8'b0;

    case (dbg_pid_r)
    // Token
    `PID_OUT:
        dbg_pid = "OUT";
    `PID_IN:
        dbg_pid = "IN";
    `PID_SOF:
        dbg_pid = "SOF";
    `PID_SETUP:
        dbg_pid = "SETUP";
    `PID_PING:
        dbg_pid = "PING";
    // Data
    `PID_DATA0:
        dbg_pid = "DATA0";
    `PID_DATA1:
        dbg_pid = "DATA1";
    `PID_DATA2:
        dbg_pid = "DATA2";
    `PID_MDATA:
        dbg_pid = "MDATA";
    // Handshake
    `PID_ACK:
        dbg_pid = "ACK";
    `PID_NAK:
        dbg_pid = "NAK";
    `PID_STALL:
        dbg_pid = "STALL";
    `PID_NYET:
        dbg_pid = "NYET";
    // Special
    `PID_PRE:
        dbg_pid = "PRE/ERR";
    `PID_SPLIT:
        dbg_pid = "SPLIT";
    default:
        ;
    endcase
end
/* verilator lint_on WIDTH */
`endif


endmodule
//-----------------------------------------------------------------
//                       USB Serial Port
//                            V0.1
//                     Ultra-Embedded.com
//                       Copyright 2020
//
//                 Email: admin@ultra-embedded.com
//
//                         License: LGPL
//-----------------------------------------------------------------
//
// This source file may be used and distributed without         
// restriction provided that this copyright statement is not    
// removed from the file and that any derivative work contains  
// the original copyright notice and the associated disclaimer. 
//
// This source file is free software; you can redistribute it   
// and/or modify it under the terms of the GNU Lesser General   
// Public License as published by the Free Software Foundation; 
// either version 2.1 of the License, or (at your option) any   
// later version.
//
// This source is distributed in the hope that it will be       
// useful, but WITHOUT ANY WARRANTY; without even the implied   
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      
// PURPOSE.  See the GNU Lesser General Public License for more 
// details.
//
// You should have received a copy of the GNU Lesser General    
// Public License along with this source; if not, write to the 
// Free Software Foundation, Inc., 59 Temple Place, Suite 330, 
// Boston, MA  02111-1307  USA
//-----------------------------------------------------------------

//-----------------------------------------------------------------
//                          Generated File
//-----------------------------------------------------------------

module usbf_sie_rx
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           enable_i
    ,input  [  7:0]  utmi_data_i
    ,input           utmi_rxvalid_i
    ,input           utmi_rxactive_i
    ,input  [  6:0]  current_addr_i

    // Outputs
    ,output [  7:0]  pid_o
    ,output          frame_valid_o
    ,output [ 10:0]  frame_number_o
    ,output          token_valid_o
    ,output [  6:0]  token_addr_o
    ,output [  3:0]  token_ep_o
    ,output          token_crc_err_o
    ,output          handshake_valid_o
    ,output          data_valid_o
    ,output          data_strb_o
    ,output [  7:0]  data_o
    ,output          data_last_o
    ,output          data_crc_err_o
    ,output          data_complete_o
);



//-----------------------------------------------------------------
// Defines:
//-----------------------------------------------------------------
// `include "usbf_defs.v"

localparam STATE_W                       = 4;
localparam STATE_RX_IDLE                 = 4'd0;
localparam STATE_RX_TOKEN2               = 4'd1;
localparam STATE_RX_TOKEN3               = 4'd2;
localparam STATE_RX_TOKEN_COMPLETE       = 4'd3;
localparam STATE_RX_SOF2                 = 4'd4;
localparam STATE_RX_SOF3                 = 4'd5;
localparam STATE_RX_DATA                 = 4'd6;
localparam STATE_RX_DATA_COMPLETE        = 4'd7;
localparam STATE_RX_IGNORED              = 4'd8;
reg [STATE_W-1:0] state_q;

//-----------------------------------------------------------------
// Wire / Regs
//-----------------------------------------------------------------
`define USB_FRAME_W    11
reg [`USB_FRAME_W-1:0]      frame_num_q;

`define USB_DEV_W      7
reg [`USB_DEV_W-1:0]        token_dev_q;

`define USB_EP_W       4
reg [`USB_EP_W-1:0]         token_ep_q;

`define USB_PID_W      8
reg [`USB_PID_W-1:0]        token_pid_q;

//-----------------------------------------------------------------
// Data delay (to strip the CRC16 trailing bytes)
//-----------------------------------------------------------------
reg [31:0] data_buffer_q;
reg [3:0]  data_valid_q;
reg [3:0]  rx_active_q;

wire shift_en_w = (utmi_rxvalid_i & utmi_rxactive_i) || !utmi_rxactive_i;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    data_buffer_q <= 32'b0;
else if (shift_en_w)
    data_buffer_q <= {utmi_data_i, data_buffer_q[31:8]};

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    data_valid_q <= 4'b0;
else if (shift_en_w)
    data_valid_q <= {(utmi_rxvalid_i & utmi_rxactive_i), data_valid_q[3:1]};
else
    data_valid_q <= {data_valid_q[3:1], 1'b0};

reg [1:0] data_crc_q;
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    data_crc_q <= 2'b0;
else if (shift_en_w)
    data_crc_q <= {!utmi_rxactive_i, data_crc_q[1]};

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    rx_active_q <= 4'b0;
else
    rx_active_q <= {utmi_rxactive_i, rx_active_q[3:1]};

wire [7:0] data_w       = data_buffer_q[7:0];
wire       data_ready_w = data_valid_q[0];
wire       crc_byte_w   = data_crc_q[0];
wire       rx_active_w  = rx_active_q[0];

wire       address_match_w = (token_dev_q == current_addr_i);

//-----------------------------------------------------------------
// Next state
//-----------------------------------------------------------------
reg [STATE_W-1:0] next_state_r;

always @ *
begin
    next_state_r = state_q;

    case (state_q)

    //-----------------------------------------
    // IDLE
    //-----------------------------------------
    STATE_RX_IDLE :
    begin
       if (data_ready_w)
       begin
           // Decode PID
           case (data_w)

              `PID_OUT, `PID_IN, `PID_SETUP, `PID_PING:
                    next_state_r  = STATE_RX_TOKEN2;

              `PID_SOF:
                    next_state_r  = STATE_RX_SOF2;

              `PID_DATA0, `PID_DATA1, `PID_DATA2, `PID_MDATA:
              begin
                    next_state_r  = STATE_RX_DATA;
              end

              `PID_ACK, `PID_NAK, `PID_STALL, `PID_NYET:
                    next_state_r  = STATE_RX_IDLE;

              default : // SPLIT / ERR
                    next_state_r  = STATE_RX_IGNORED;
           endcase
       end
    end

    //-----------------------------------------
    // RX_IGNORED: Unknown / unsupported
    //-----------------------------------------
    STATE_RX_IGNORED :
    begin
        // Wait until the end of the packet
        if (!rx_active_w)
           next_state_r = STATE_RX_IDLE;
    end

    //-----------------------------------------
    // SOF (BYTE 2)
    //-----------------------------------------
    STATE_RX_SOF2 :
    begin
       if (data_ready_w)
           next_state_r = STATE_RX_SOF3;
       else if (!rx_active_w)
           next_state_r = STATE_RX_IDLE;
    end

    //-----------------------------------------
    // SOF (BYTE 3)
    //-----------------------------------------
    STATE_RX_SOF3 :
    begin
       if (data_ready_w || !rx_active_w)
           next_state_r = STATE_RX_IDLE;
    end

    //-----------------------------------------
    // TOKEN (IN/OUT/SETUP) (Address/Endpoint)
    //-----------------------------------------
    STATE_RX_TOKEN2 :
    begin
       if (data_ready_w)
           next_state_r = STATE_RX_TOKEN3;
       else if (!rx_active_w)
           next_state_r = STATE_RX_IDLE;
    end

    //-----------------------------------------
    // TOKEN (IN/OUT/SETUP) (Endpoint/CRC)
    //-----------------------------------------
    STATE_RX_TOKEN3 :
    begin
       if (data_ready_w)
           next_state_r = STATE_RX_TOKEN_COMPLETE;
       else if (!rx_active_w)
           next_state_r = STATE_RX_IDLE;
    end

    //-----------------------------------------
    // RX_TOKEN_COMPLETE
    //-----------------------------------------
    STATE_RX_TOKEN_COMPLETE :
    begin
        next_state_r  = STATE_RX_IDLE;
    end

    //-----------------------------------------
    // RX_DATA
    //-----------------------------------------
    STATE_RX_DATA :
    begin
       // Receive complete
       if (crc_byte_w)
            next_state_r = STATE_RX_DATA_COMPLETE;
    end

    //-----------------------------------------
    // RX_DATA_COMPLETE
    //-----------------------------------------
    STATE_RX_DATA_COMPLETE :
    begin
        if (!rx_active_w)
            next_state_r = STATE_RX_IDLE;
    end

    default :
       ;

    endcase
end

// Update state
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    state_q   <= STATE_RX_IDLE;
else if (!enable_i)
    state_q   <= STATE_RX_IDLE;
else
    state_q   <= next_state_r;

//-----------------------------------------------------------------
// Handshake:
//-----------------------------------------------------------------
reg handshake_valid_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    handshake_valid_q <= 1'b0;
else if (state_q == STATE_RX_IDLE && data_ready_w)
begin
    case (data_w)
    `PID_ACK, `PID_NAK, `PID_STALL, `PID_NYET:
        handshake_valid_q <= address_match_w;
    default :
        handshake_valid_q <= 1'b0;
    endcase
end
else
    handshake_valid_q <= 1'b0;

assign handshake_valid_o = handshake_valid_q;

//-----------------------------------------------------------------
// SOF: Frame number
//-----------------------------------------------------------------
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    frame_num_q         <= `USB_FRAME_W'b0;
else if (state_q == STATE_RX_SOF2 && data_ready_w)
    frame_num_q         <= {3'b0, data_w};
else if (state_q == STATE_RX_SOF3 && data_ready_w)
    frame_num_q         <= {data_w[2:0], frame_num_q[7:0]};
else if (!enable_i)
    frame_num_q         <= `USB_FRAME_W'b0;

assign frame_number_o = frame_num_q;

reg frame_valid_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    frame_valid_q <= 1'b0;
else
    frame_valid_q <= (state_q == STATE_RX_SOF3 && data_ready_w);

assign frame_valid_o = frame_valid_q;

//-----------------------------------------------------------------
// Token: PID
//-----------------------------------------------------------------
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    token_pid_q <= `USB_PID_W'b0;
else if (state_q == STATE_RX_IDLE && data_ready_w)
    token_pid_q <= data_w;
else if (!enable_i)
    token_pid_q <= `USB_PID_W'b0;

assign pid_o = token_pid_q;

reg token_valid_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    token_valid_q <= 1'b0;
else
    token_valid_q <= (state_q == STATE_RX_TOKEN_COMPLETE) && address_match_w;

assign token_valid_o = token_valid_q;

//-----------------------------------------------------------------
// Token: Device Address
//-----------------------------------------------------------------
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    token_dev_q <= `USB_DEV_W'b0;
else if (state_q == STATE_RX_TOKEN2 && data_ready_w)
    token_dev_q <= data_w[6:0];
else if (!enable_i)
    token_dev_q <= `USB_DEV_W'b0;

assign token_addr_o = token_dev_q;

//-----------------------------------------------------------------
// Token: Endpoint
//-----------------------------------------------------------------
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    token_ep_q      <= `USB_EP_W'b0;
else if (state_q == STATE_RX_TOKEN2 && data_ready_w)
    token_ep_q[0]   <= data_w[7];
else if (state_q == STATE_RX_TOKEN3 && data_ready_w)
    token_ep_q[3:1] <= data_w[2:0];
else if (!enable_i)
    token_ep_q      <= `USB_EP_W'b0;

assign token_ep_o = token_ep_q;
assign token_crc_err_o = 1'b0;

wire [7:0] input_data_w  = data_w;
wire       input_ready_w = state_q == STATE_RX_DATA && data_ready_w && !crc_byte_w;

//-----------------------------------------------------------------
// CRC16: Generate CRC16 on incoming data bytes
//-----------------------------------------------------------------
reg [15:0]  crc_sum_q;
wire [15:0] crc_out_w;
reg         crc_err_q;

usbf_crc16
u_crc16
(
    .crc_in_i(crc_sum_q),
    .din_i(data_w),
    .crc_out_o(crc_out_w)
);

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    crc_sum_q   <= 16'hFFFF;
else if (state_q == STATE_RX_IDLE)
    crc_sum_q   <= 16'hFFFF;
else if (data_ready_w)
    crc_sum_q   <= crc_out_w;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    crc_err_q   <= 1'b0;
else if (state_q == STATE_RX_IDLE)
    crc_err_q   <= 1'b0;
else if (state_q == STATE_RX_DATA_COMPLETE && next_state_r == STATE_RX_IDLE)
    crc_err_q   <= (crc_sum_q != 16'hB001);

assign data_crc_err_o = crc_err_q;

reg data_complete_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    data_complete_q   <= 1'b0;
else if (state_q == STATE_RX_DATA_COMPLETE && next_state_r == STATE_RX_IDLE)
    data_complete_q   <= 1'b1;
else
    data_complete_q   <= 1'b0;

assign data_complete_o = data_complete_q;

reg data_zlp_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    data_zlp_q   <= 1'b0;
else if (state_q == STATE_RX_IDLE && next_state_r == STATE_RX_DATA)
    data_zlp_q   <= 1'b1;
else if (input_ready_w)
    data_zlp_q   <= 1'b0;

//-----------------------------------------------------------------
// Data Output
//-----------------------------------------------------------------
reg        valid_q;
reg        last_q;
reg [7:0]  data_q;
reg        mask_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
begin
    valid_q  <= 1'b0;
    data_q   <= 8'b0;
    mask_q   <= 1'b0;
    last_q   <= 1'b0;
end
else
begin
    valid_q  <= input_ready_w || ((state_q == STATE_RX_DATA) && crc_byte_w && data_zlp_q);
    data_q   <= input_data_w;
    mask_q   <= input_ready_w;
    last_q   <= (state_q == STATE_RX_DATA) && crc_byte_w;
end

// Data
assign data_valid_o = valid_q;
assign data_strb_o  = mask_q;
assign data_o       = data_q;
assign data_last_o  = last_q | crc_byte_w;


endmodule
//-----------------------------------------------------------------
//                       USB Serial Port
//                            V0.1
//                     Ultra-Embedded.com
//                       Copyright 2020
//
//                 Email: admin@ultra-embedded.com
//
//                         License: LGPL
//-----------------------------------------------------------------
//
// This source file may be used and distributed without         
// restriction provided that this copyright statement is not    
// removed from the file and that any derivative work contains  
// the original copyright notice and the associated disclaimer. 
//
// This source file is free software; you can redistribute it   
// and/or modify it under the terms of the GNU Lesser General   
// Public License as published by the Free Software Foundation; 
// either version 2.1 of the License, or (at your option) any   
// later version.
//
// This source is distributed in the hope that it will be       
// useful, but WITHOUT ANY WARRANTY; without even the implied   
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      
// PURPOSE.  See the GNU Lesser General Public License for more 
// details.
//
// You should have received a copy of the GNU Lesser General    
// Public License along with this source; if not, write to the 
// Free Software Foundation, Inc., 59 Temple Place, Suite 330, 
// Boston, MA  02111-1307  USA
//-----------------------------------------------------------------

//-----------------------------------------------------------------
//                          Generated File
//-----------------------------------------------------------------

module usbf_sie_tx
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           enable_i
    ,input           chirp_i
    ,input           utmi_txready_i
    ,input           tx_valid_i
    ,input  [  7:0]  tx_pid_i
    ,input           data_valid_i
    ,input           data_strb_i
    ,input  [  7:0]  data_i
    ,input           data_last_i

    // Outputs
    ,output [  7:0]  utmi_data_o
    ,output          utmi_txvalid_o
    ,output          tx_accept_o
    ,output          data_accept_o
);



//-----------------------------------------------------------------
// Defines:
//-----------------------------------------------------------------
// `include "usbf_defs.v"

localparam STATE_W                       = 3;
localparam STATE_TX_IDLE                 = 3'd0;
localparam STATE_TX_PID                  = 3'd1;
localparam STATE_TX_DATA                 = 3'd2;
localparam STATE_TX_CRC1                 = 3'd3;
localparam STATE_TX_CRC2                 = 3'd4;
localparam STATE_TX_DONE                 = 3'd5;
localparam STATE_TX_CHIRP                = 3'd6;

reg [STATE_W-1:0] state_q;
reg [STATE_W-1:0] next_state_r;

//-----------------------------------------------------------------
// Request Type
//-----------------------------------------------------------------
reg data_pid_q;
reg data_zlp_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
begin
    data_pid_q <= 1'b0;
    data_zlp_q <= 1'b0;
end
else if (!enable_i)
begin
    data_pid_q <= 1'b0;
    data_zlp_q <= 1'b0;
end
else if (tx_valid_i && tx_accept_o)
begin
    case (tx_pid_i)

    `PID_MDATA, `PID_DATA2, `PID_DATA0, `PID_DATA1:
    begin
        data_pid_q <= 1'b1;
        data_zlp_q <= data_valid_i && (data_strb_i == 1'b0) && data_last_i;
    end

    default :
    begin
        data_pid_q <= 1'b0;
        data_zlp_q <= 1'b0;
    end
    endcase
end
else if (next_state_r == STATE_TX_CRC1)
begin
    data_pid_q <= 1'b0;
    data_zlp_q <= 1'b0;
end

assign tx_accept_o = (state_q == STATE_TX_IDLE);

//-----------------------------------------------------------------
// Next state
//-----------------------------------------------------------------
always @ *
begin
    next_state_r = state_q;

    //-----------------------------------------
    // State Machine
    //-----------------------------------------
    case (state_q)

    //-----------------------------------------
    // IDLE
    //-----------------------------------------
    STATE_TX_IDLE :
    begin
        if (chirp_i)
            next_state_r  = STATE_TX_CHIRP;
        else if (tx_valid_i)
            next_state_r  = STATE_TX_PID;
    end

    //-----------------------------------------
    // TX_PID
    //-----------------------------------------
    STATE_TX_PID :
    begin
        // Data accepted
        if (utmi_txready_i)
        begin
            if (data_zlp_q)
                next_state_r = STATE_TX_CRC1;
            else if (data_pid_q)
                next_state_r = STATE_TX_DATA;
            else
                next_state_r = STATE_TX_DONE;
        end
    end

    //-----------------------------------------
    // TX_DATA
    //-----------------------------------------
    STATE_TX_DATA :
    begin
        // Data accepted
        if (utmi_txready_i)
        begin
            // Generate CRC16 at end of packet
            if (data_last_i)
                next_state_r  = STATE_TX_CRC1;
        end
    end

    //-----------------------------------------
    // TX_CRC1 (first byte)
    //-----------------------------------------
    STATE_TX_CRC1 :
    begin
        // Data sent?
        if (utmi_txready_i)
            next_state_r  = STATE_TX_CRC2;
    end

    //-----------------------------------------
    // TX_CRC (second byte)
    //-----------------------------------------
    STATE_TX_CRC2 :
    begin
        // Data sent?
        if (utmi_txready_i)
            next_state_r  = STATE_TX_DONE;
    end

    //-----------------------------------------
    // TX_DONE
    //-----------------------------------------
    STATE_TX_DONE :
    begin
        // Data sent?
        if (!utmi_txvalid_o || utmi_txready_i)
            next_state_r  = STATE_TX_IDLE;
    end

    //-----------------------------------------
    // TX_CHIRP
    //-----------------------------------------
    STATE_TX_CHIRP :
    begin
        if (!chirp_i)
            next_state_r  = STATE_TX_IDLE;
    end

    default :
       ;

    endcase

    // USB reset but not chirping...
    if (!enable_i && !chirp_i)
        next_state_r  = STATE_TX_IDLE;
end

// Update state
always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    state_q   <= STATE_TX_IDLE;
else
    state_q   <= next_state_r;

//-----------------------------------------------------------------
// Data Input
//-----------------------------------------------------------------
reg       input_valid_r;
reg [7:0] input_byte_r;
reg       input_last_r;
always @ *
begin
    input_valid_r = data_strb_i & data_pid_q;
    input_byte_r  = data_i;
    input_last_r  = data_last_i;
end

reg data_accept_r;
always @ *
begin
    if (state_q == STATE_TX_DATA)
        data_accept_r = utmi_txready_i;
    else if (state_q == STATE_TX_PID && data_zlp_q)
        data_accept_r = utmi_txready_i;
    else
        data_accept_r = 1'b0;
end

assign data_accept_o = data_accept_r;

//-----------------------------------------------------------------
// CRC16: Generate CRC16 on outgoing data
//-----------------------------------------------------------------
reg [15:0]  crc_sum_q;
wire [15:0] crc_out_w;
reg         crc_err_q;

usbf_crc16
u_crc16
(
    .crc_in_i(crc_sum_q),
    .din_i(utmi_data_o),
    .crc_out_o(crc_out_w)
);

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
    crc_sum_q   <= 16'hFFFF;
else if (state_q == STATE_TX_IDLE)
    crc_sum_q   <= 16'hFFFF;
else if (state_q == STATE_TX_DATA && utmi_txvalid_o && utmi_txready_i)
    crc_sum_q   <= crc_out_w;

//-----------------------------------------------------------------
// Output
//-----------------------------------------------------------------
reg       valid_q;
reg [7:0] data_q;

always @ (posedge clk_i or posedge rst_i)
if (rst_i)
begin
    valid_q <= 1'b0;
    data_q  <= 8'b0;
end
else if (!enable_i)
begin
    valid_q <= 1'b0;
    data_q  <= 8'b0;
end
else if (tx_valid_i && tx_accept_o)
begin
    valid_q <= 1'b1;
    data_q  <= tx_pid_i;
end
else if (utmi_txready_i)
begin
    valid_q <= 1'b0;
    data_q  <= 8'b0;
end

reg       utmi_txvalid_r;
reg [7:0] utmi_data_r;

always @ *
begin
    if (state_q == STATE_TX_CHIRP)
    begin
        utmi_txvalid_r = 1'b1;
        utmi_data_r    = 8'b0;
    end
    else if (state_q == STATE_TX_CRC1)
    begin
        utmi_txvalid_r = 1'b1;
        utmi_data_r    = crc_sum_q[7:0] ^ 8'hFF;
    end
    else if (state_q == STATE_TX_CRC2)
    begin
        utmi_txvalid_r = 1'b1;
        utmi_data_r    = crc_sum_q[15:8] ^ 8'hFF;
    end
    else if (state_q == STATE_TX_DATA)
    begin
        utmi_txvalid_r = data_valid_i;
        utmi_data_r    = data_i;
    end
    else
    begin
        utmi_txvalid_r = valid_q;
        utmi_data_r    = data_q;
    end
end

assign utmi_txvalid_o = utmi_txvalid_r;
assign utmi_data_o    = utmi_data_r;


endmodule
//-----------------------------------------------------------------
//                       USB Serial Port
//                            V0.1
//                     Ultra-Embedded.com
//                       Copyright 2020
//
//                 Email: admin@ultra-embedded.com
//
//                         License: LGPL
//-----------------------------------------------------------------
//
// This source file may be used and distributed without         
// restriction provided that this copyright statement is not    
// removed from the file and that any derivative work contains  
// the original copyright notice and the associated disclaimer. 
//
// This source file is free software; you can redistribute it   
// and/or modify it under the terms of the GNU Lesser General   
// Public License as published by the Free Software Foundation; 
// either version 2.1 of the License, or (at your option) any   
// later version.
//
// This source is distributed in the hope that it will be       
// useful, but WITHOUT ANY WARRANTY; without even the implied   
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      
// PURPOSE.  See the GNU Lesser General Public License for more 
// details.
//
// You should have received a copy of the GNU Lesser General    
// Public License along with this source; if not, write to the 
// Free Software Foundation, Inc., 59 Temple Place, Suite 330, 
// Boston, MA  02111-1307  USA
//-----------------------------------------------------------------

//-----------------------------------------------------------------
//                          Generated File
//-----------------------------------------------------------------

module usb_cdc_top
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
     parameter BAUDRATE         = 1000000
)
//-----------------------------------------------------------------
// Ports
//-----------------------------------------------------------------
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input  [  7:0]  ulpi_data_out_i
    ,input           ulpi_dir_i
    ,input           ulpi_nxt_i
    ,input           tx_i

    // Outputs
    ,output [  7:0]  ulpi_data_in_o
    ,output          ulpi_stp_o
    ,output          rx_o
);

wire  [  7:0]  utmi_data_out_w;
wire  [  7:0]  usb_rx_data_w;
wire           usb_tx_accept_w;
wire           enable_w = 1'h1;
wire  [  1:0]  utmi_xcvrselect_w;
wire           utmi_termselect_w;
wire           utmi_rxvalid_w;
wire  [  1:0]  utmi_op_mode_w;
wire  [  7:0]  utmi_data_in_w;
wire           utmi_rxerror_w;
wire           utmi_rxactive_w;
wire  [  1:0]  utmi_linestate_w;
wire           usb_tx_valid_w;
wire           usb_rx_accept_w;
wire           utmi_dppulldown_w;
wire  [  7:0]  usb_tx_data_w;
wire           usb_rx_valid_w;
wire           utmi_txready_w;
wire           utmi_txvalid_w;
wire           utmi_dmpulldown_w;


usb_cdc_core
u_usb
(
    // Inputs
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.enable_i(enable_w)
    ,.utmi_data_in_i(utmi_data_in_w)
    ,.utmi_txready_i(utmi_txready_w)
    ,.utmi_rxvalid_i(utmi_rxvalid_w)
    ,.utmi_rxactive_i(utmi_rxactive_w)
    ,.utmi_rxerror_i(utmi_rxerror_w)
    ,.utmi_linestate_i(utmi_linestate_w)
    ,.inport_valid_i(usb_tx_valid_w)
    ,.inport_data_i(usb_tx_data_w)
    ,.outport_accept_i(usb_rx_accept_w)

    // Outputs
    ,.utmi_data_out_o(utmi_data_out_w)
    ,.utmi_txvalid_o(utmi_txvalid_w)
    ,.utmi_op_mode_o(utmi_op_mode_w)
    ,.utmi_xcvrselect_o(utmi_xcvrselect_w)
    ,.utmi_termselect_o(utmi_termselect_w)
    ,.utmi_dppulldown_o(utmi_dppulldown_w)
    ,.utmi_dmpulldown_o(utmi_dmpulldown_w)
    ,.inport_accept_o(usb_tx_accept_w)
    ,.outport_valid_o(usb_rx_valid_w)
    ,.outport_data_o(usb_rx_data_w)
);


ulpi_wrapper
u_usb_phy
(
    // Inputs
     .ulpi_clk60_i(clk_i)
    ,.ulpi_rst_i(rst_i)
    ,.ulpi_data_out_i(ulpi_data_out_i)
    ,.ulpi_dir_i(ulpi_dir_i)
    ,.ulpi_nxt_i(ulpi_nxt_i)
    ,.utmi_data_out_i(utmi_data_out_w)
    ,.utmi_txvalid_i(utmi_txvalid_w)
    ,.utmi_op_mode_i(utmi_op_mode_w)
    ,.utmi_xcvrselect_i(utmi_xcvrselect_w)
    ,.utmi_termselect_i(utmi_termselect_w)
    ,.utmi_dppulldown_i(utmi_dppulldown_w)
    ,.utmi_dmpulldown_i(utmi_dmpulldown_w)

    // Outputs
    ,.ulpi_data_in_o(ulpi_data_in_o)
    ,.ulpi_stp_o(ulpi_stp_o)
    ,.utmi_data_in_o(utmi_data_in_w)
    ,.utmi_txready_o(utmi_txready_w)
    ,.utmi_rxvalid_o(utmi_rxvalid_w)
    ,.utmi_rxactive_o(utmi_rxactive_w)
    ,.utmi_rxerror_o(utmi_rxerror_w)
    ,.utmi_linestate_o(utmi_linestate_w)
);


//-----------------------------------------------------------------
// Output FIFO
//-----------------------------------------------------------------
wire       tx_valid_w;
wire [7:0] tx_data_w;
wire       tx_accept_w;

usb_cdc_fifo
#(
    .WIDTH(8),
    .DEPTH(128),
    .ADDR_W(7)
)
u_fifo_tx
(
    .clk_i(clk_i),
    .rst_i(rst_i),

    // In
    .push_i(tx_valid_w),
    .data_in_i(tx_data_w),
    .accept_o(tx_accept_w),

    // Out
    .pop_i(usb_tx_accept_w),
    .data_out_o(usb_tx_data_w),
    .valid_o(usb_tx_valid_w)
);

//-----------------------------------------------------------------
// Input FIFO
//-----------------------------------------------------------------
wire       rx_valid_w;
wire [7:0] rx_data_w;
wire       rx_accept_w;

usb_cdc_fifo
#(
    .WIDTH(8),
    .DEPTH(64),
    .ADDR_W(6)
)
u_fifo_rx
(
    .clk_i(clk_i),
    .rst_i(rst_i),

    // In
    .push_i(usb_rx_valid_w),
    .data_in_i(usb_rx_data_w),
    .accept_o(usb_rx_accept_w),

    // Out
    .pop_i(rx_accept_w),
    .data_out_o(rx_data_w),
    .valid_o(rx_valid_w)
);

//-----------------------------------------------------------------
// Registers
//-----------------------------------------------------------------

// Configuration
localparam   STOP_BITS = 1'b0; // 0 = 1, 1 = 2
localparam   CLK_FREQ  = 60000000;
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
   rxd_ms_q <= tx_i;
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
   if (tx_accept_w)
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

assign tx_data_w   = rx_data_q;
assign tx_valid_w  = rx_ready_q;

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
else if (rx_valid_w)
begin
    tx_shift_reg_q <= rx_data_w;
    tx_busy_q      <= 1'b1;
    tx_complete_q  <= 1'b0;
end
else
    tx_complete_q  <= 1'b0;

assign rx_accept_w = ~tx_busy_q;

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

assign rx_o = txd_q;


endmodule

module usb_cdc_fifo
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
always @ (posedge clk_i )
if (rst_i)
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
