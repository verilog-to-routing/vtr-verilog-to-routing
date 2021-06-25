//////////////////////////////////////////////////////////////////////////////
// Author: Aman Arora
//////////////////////////////////////////////////////////////////////////////

`timescale 1ns / 1ps

/////////////////////////////////////////////////////////////////////////
// GEMM (General Matrix Multiply) layer
// Precision BF16
// Size of the layer 20x20
// AXI interface to control/observe the unit
// RAMs that store inputs and outputs are not a part of this design
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
// When using an FPGA architecture which has a complex DSP block
// (eg. COFFE_22m/k6n10LB_mem20K_complexDSP_customSB_22nm.xml), define  
// the following macro. But comment it out if using an FPGA architecture 
// with a simpler DSP (just a fixed point multiplier) like in the
// flagship arch timing/k6_frac_N10_frac_chain_depop50_mem32K_40nm.xml
/////////////////////////////////////////////////////////////////////////

`define BFLOAT16 

// IEEE Half Precision => EXPONENT = 5, MANTISSA = 10
// BFLOAT16 => EXPONENT = 8, MANTISSA = 7 

`ifdef BFLOAT16
`define EXPONENT 8
`define MANTISSA 7
`else // for ieee half precision fp16
`define EXPONENT 5
`define MANTISSA 10
`endif

`define SIGN 1
`define DWIDTH (`SIGN+`EXPONENT+`MANTISSA)

`define AWIDTH 8
`define MEM_SIZE 256

`define MAT_MUL_SIZE 20
`define MASK_WIDTH 20
//This define isn't needed for this design, because we set a_loc and b_loc to 0
`define LOG2_MAT_MUL_SIZE 4 

`define BB_MAT_MUL_SIZE `MAT_MUL_SIZE
`define NUM_CYCLES_IN_MAC 3
`define MEM_ACCESS_LATENCY 1
`define ADDR_STRIDE_WIDTH 8

/////////////////////////////////////////////////////////////////////////
// The AXI code is taken from the RTL generator by Xilinx Vivado
// block design.
/////////////////////////////////////////////////////////////////////////

module gemm_layer #
	(
		// Parameters of Axi Slave Bus Interface S00_AXI
		parameter C_S00_AXI_DATA_WIDTH	= 32,
		parameter C_S00_AXI_ADDR_WIDTH	= 6
	)
	(
		// Users to add ports here
		output wire [14:0] bram_addr_a,
		input wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_a,
		output wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_a,
		output wire[`MASK_WIDTH-1:0] bram_we_a,	
		output wire bram_en_a,

		output wire [14:0] bram_addr_b,
		input wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_b,
		output wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_b,
		output wire[`MASK_WIDTH-1:0] bram_we_b,	
		output wire bram_en_b,
		
		output wire [14:0] bram_addr_c,
		input wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_c,
		output wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_c,
		output wire[`MASK_WIDTH-1:0] bram_we_c,	
		output wire bram_en_c,
		// User ports ends
		// Do not modify the ports beyond this line


		// Ports of Axi Slave Bus Interface S00_AXI
		input wire  s00_axi_aclk,
		input wire  s00_axi_aresetn,
		input wire [C_S00_AXI_ADDR_WIDTH-1 : 0] s00_axi_awaddr,
		input wire [2 : 0] s00_axi_awprot,
		input wire  s00_axi_awvalid,
		output wire  s00_axi_awready,
		input wire [C_S00_AXI_DATA_WIDTH-1 : 0] s00_axi_wdata,
		input wire [(C_S00_AXI_DATA_WIDTH/8)-1 : 0] s00_axi_wstrb,
		input wire  s00_axi_wvalid,
		output wire  s00_axi_wready,
		output wire [1 : 0] s00_axi_bresp,
		output wire  s00_axi_bvalid,
		input wire  s00_axi_bready,
		input wire [C_S00_AXI_ADDR_WIDTH-1 : 0] s00_axi_araddr,
		input wire [2 : 0] s00_axi_arprot,
		input wire  s00_axi_arvalid,
		output wire  s00_axi_arready,
		output wire [C_S00_AXI_DATA_WIDTH-1 : 0] s00_axi_rdata,
		output wire [1 : 0] s00_axi_rresp,
		output wire  s00_axi_rvalid,
		input wire  s00_axi_rready
	);

  // Instantiation of Axi Bus Interface S00_AXI
	gemm_0_S00_AXI # ( 
		.C_S_AXI_DATA_WIDTH(C_S00_AXI_DATA_WIDTH),
		.C_S_AXI_ADDR_WIDTH(C_S00_AXI_ADDR_WIDTH)
	) gemm_0_S00_AXI_inst (
		.bram_addr_a(bram_addr_a),
		.bram_rdata_a(bram_rdata_a),
		.bram_wdata_a(bram_wdata_a),
		.bram_we_a(bram_we_a),
		.bram_en_a(bram_en_a),
		.bram_addr_b(bram_addr_b),
		.bram_rdata_b(bram_rdata_b),
		.bram_wdata_b(bram_wdata_b),
		.bram_we_b(bram_we_b),
		.bram_en_b(bram_en_b),
		.bram_addr_c(bram_addr_c),
		.bram_rdata_c(bram_rdata_c),
		.bram_wdata_c(bram_wdata_c),
		.bram_we_c(bram_we_c),
		.bram_en_c(bram_en_c),
		.S_AXI_ACLK(s00_axi_aclk),
		.S_AXI_ARESETN(s00_axi_aresetn),
		.S_AXI_AWADDR(s00_axi_awaddr),
		.S_AXI_AWPROT(s00_axi_awprot),
		.S_AXI_AWVALID(s00_axi_awvalid),
		.S_AXI_AWREADY(s00_axi_awready),
		.S_AXI_WDATA(s00_axi_wdata),
		.S_AXI_WSTRB(s00_axi_wstrb),
		.S_AXI_WVALID(s00_axi_wvalid),
		.S_AXI_WREADY(s00_axi_wready),
		.S_AXI_BRESP(s00_axi_bresp),
		.S_AXI_BVALID(s00_axi_bvalid),
		.S_AXI_BREADY(s00_axi_bready),
		.S_AXI_ARADDR(s00_axi_araddr),
		.S_AXI_ARPROT(s00_axi_arprot),
		.S_AXI_ARVALID(s00_axi_arvalid),
		.S_AXI_ARREADY(s00_axi_arready),
		.S_AXI_RDATA(s00_axi_rdata),
		.S_AXI_RRESP(s00_axi_rresp),
		.S_AXI_RVALID(s00_axi_rvalid),
		.S_AXI_RREADY(s00_axi_rready)
	);

	endmodule

	module gemm_0_S00_AXI #
(
		// Width of S_AXI data bus
		parameter C_S_AXI_DATA_WIDTH	= 32,
		// Width of S_AXI address bus
		parameter C_S_AXI_ADDR_WIDTH	= 6
	)
	(
		// Users to add ports here
		output wire [14:0] bram_addr_a,
		input wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_a,
		output wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_a,
		output wire [`MASK_WIDTH-1:0] bram_we_a,
		output wire bram_en_a,

		output wire [14:0] bram_addr_b,
		input wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_b,
		output wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_b,
		output wire [`MASK_WIDTH-1:0] bram_we_b,
		output wire bram_en_b,
		
		output wire [14:0] bram_addr_c,
		input wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_c,
		output wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_c,
		output wire [`MASK_WIDTH-1:0] bram_we_c,
		output wire bram_en_c,
		
		// User ports ends
		// Do not modify the ports beyond this line

		// Global Clock Signal
		input wire  S_AXI_ACLK,
		// Global Reset Signal. This Signal is Active LOW
		input wire  S_AXI_ARESETN,
		// Write address (issued by master, acceped by Slave)
		input wire [C_S_AXI_ADDR_WIDTH-1 : 0] S_AXI_AWADDR,
		// Write channel Protection type. This signal indicates the
    		// privilege and security level of the transaction, and whether
    		// the transaction is a data access or an instruction access.
		input wire [2 : 0] S_AXI_AWPROT,
		// Write address valid. This signal indicates that the master signaling
    		// valid write address and control information.
		input wire  S_AXI_AWVALID,
		// Write address ready. This signal indicates that the slave is ready
    		// to accept an address and associated control signals.
		output wire  S_AXI_AWREADY,
		// Write data (issued by master, acceped by Slave) 
		input wire [C_S_AXI_DATA_WIDTH-1 : 0] S_AXI_WDATA,
		// Write strobes. This signal indicates which byte lanes hold
    		// valid data. There is one write strobe bit for each eight
    		// bits of the write data bus.    
		input wire [(C_S_AXI_DATA_WIDTH/8)-1 : 0] S_AXI_WSTRB,
		// Write valid. This signal indicates that valid write
    		// data and strobes are available.
		input wire  S_AXI_WVALID,
		// Write ready. This signal indicates that the slave
    		// can accept the write data.
		output wire  S_AXI_WREADY,
		// Write response. This signal indicates the status
    		// of the write transaction.
		output wire [1 : 0] S_AXI_BRESP,
		// Write response valid. This signal indicates that the channel
    		// is signaling a valid write response.
		output wire  S_AXI_BVALID,
		// Response ready. This signal indicates that the master
    		// can accept a write response.
		input wire  S_AXI_BREADY,
		// Read address (issued by master, acceped by Slave)
		input wire [C_S_AXI_ADDR_WIDTH-1 : 0] S_AXI_ARADDR,
		// Protection type. This signal indicates the privilege
    		// and security level of the transaction, and whether the
    		// transaction is a data access or an instruction access.
		input wire [2 : 0] S_AXI_ARPROT,
		// Read address valid. This signal indicates that the channel
    		// is signaling valid read address and control information.
		input wire  S_AXI_ARVALID,
		// Read address ready. This signal indicates that the slave is
    		// ready to accept an address and associated control signals.
		output wire  S_AXI_ARREADY,
		// Read data (issued by slave)
		output wire [C_S_AXI_DATA_WIDTH-1 : 0] S_AXI_RDATA,
		// Read response. This signal indicates the status of the
    		// read transfer.
		output wire [1 : 0] S_AXI_RRESP,
		// Read valid. This signal indicates that the channel is
    		// signaling the required read data.
		output wire  S_AXI_RVALID,
		// Read ready. This signal indicates that the master can
    		// accept the read data and response information.
		input wire  S_AXI_RREADY
	);

	// AXI4LITE signals
	reg [C_S_AXI_ADDR_WIDTH-1 : 0] 	axi_awaddr;
	reg  	axi_awready;
	reg  	axi_wready;
	reg [1 : 0] 	axi_bresp;
	reg  	axi_bvalid;
	reg [C_S_AXI_ADDR_WIDTH-1 : 0] 	axi_araddr;
	reg  	axi_arready;
	reg [C_S_AXI_DATA_WIDTH-1 : 0] 	axi_rdata;
	reg [1 : 0] 	axi_rresp;
	reg  	axi_rvalid;

	// Example-specific design signals
	// local parameter for addressing 32 bit / 64 bit C_S_AXI_DATA_WIDTH
	// ADDR_LSB is used for addressing 32/64 bit registers/memories
	// ADDR_LSB = 2 for 32 bits (n downto 2)
	// ADDR_LSB = 3 for 64 bits (n downto 3)
	localparam ADDR_LSB = (C_S_AXI_DATA_WIDTH/32) + 1;
	localparam OPT_MEM_ADDR_BITS = 3;
	//----------------------------------------------
	//-- Signals for user logic register space example
	//------------------------------------------------
	//-- Number of Slave Registers 10
	reg [C_S_AXI_DATA_WIDTH-1:0]	slv_reg0;
	reg [C_S_AXI_DATA_WIDTH-1:0]	slv_reg1;
	reg [C_S_AXI_DATA_WIDTH-1:0]	slv_reg2;
	reg [C_S_AXI_DATA_WIDTH-1:0]	slv_reg3;
	reg [C_S_AXI_DATA_WIDTH-1:0]	slv_reg4;
	reg [C_S_AXI_DATA_WIDTH-1:0]	slv_reg5;
	reg [C_S_AXI_DATA_WIDTH-1:0]	slv_reg6;
	reg [C_S_AXI_DATA_WIDTH-1:0]	slv_reg7;
	reg [C_S_AXI_DATA_WIDTH-1:0]	slv_reg8;
	reg [C_S_AXI_DATA_WIDTH-1:0]	slv_reg9;
	wire	 slv_reg_rden;
	wire	 slv_reg_wren;
	reg [C_S_AXI_DATA_WIDTH-1:0]	 reg_data_out;
	reg [31:0] byte_index;
	reg	 aw_en;

	// I/O Connections assignments

	assign S_AXI_AWREADY	= axi_awready;
	assign S_AXI_WREADY	= axi_wready;
	assign S_AXI_BRESP	= axi_bresp;
	assign S_AXI_BVALID	= axi_bvalid;
	assign S_AXI_ARREADY	= axi_arready;
	assign S_AXI_RDATA	= axi_rdata;
	assign S_AXI_RRESP	= axi_rresp;
	assign S_AXI_RVALID	= axi_rvalid;
	// Implement axi_awready generation
	// axi_awready is asserted for one S_AXI_ACLK clock cycle when both
	// S_AXI_AWVALID and S_AXI_WVALID are asserted. axi_awready is
	// de-asserted when reset is low.

	always @( posedge S_AXI_ACLK )
	begin
	  if ( S_AXI_ARESETN == 1'b0 )
	    begin
	      axi_awready <= 1'b0;
	      aw_en <= 1'b1;
	    end 
	  else
	    begin    
	      if (~axi_awready && S_AXI_AWVALID && S_AXI_WVALID && aw_en)
	        begin
	          // slave is ready to accept write address when 
	          // there is a valid write address and write data
	          // on the write address and data bus. This design 
	          // expects no outstanding transactions. 
	          axi_awready <= 1'b1;
	          aw_en <= 1'b0;
	        end
	        else if (S_AXI_BREADY && axi_bvalid)
	            begin
	              aw_en <= 1'b1;
	              axi_awready <= 1'b0;
	            end
	      else           
	        begin
	          axi_awready <= 1'b0;
	        end
	    end 
	end       

	// Implement axi_awaddr latching
	// This process is used to latch the address when both 
	// S_AXI_AWVALID and S_AXI_WVALID are valid. 

	always @( posedge S_AXI_ACLK )
	begin
	  if ( S_AXI_ARESETN == 1'b0 )
	    begin
	      axi_awaddr <= 0;
	    end 
	  else
	    begin    
	      if (~axi_awready && S_AXI_AWVALID && S_AXI_WVALID && aw_en)
	        begin
	          // Write Address latching 
	          axi_awaddr <= S_AXI_AWADDR;
	        end
	    end 
	end       

	// Implement axi_wready generation
	// axi_wready is asserted for one S_AXI_ACLK clock cycle when both
	// S_AXI_AWVALID and S_AXI_WVALID are asserted. axi_wready is 
	// de-asserted when reset is low. 

	always @( posedge S_AXI_ACLK )
	begin
	  if ( S_AXI_ARESETN == 1'b0 )
	    begin
	      axi_wready <= 1'b0;
	    end 
	  else
	    begin    
	      if (~axi_wready && S_AXI_WVALID && S_AXI_AWVALID && aw_en )
	        begin
	          // slave is ready to accept write data when 
	          // there is a valid write address and write data
	          // on the write address and data bus. This design 
	          // expects no outstanding transactions. 
	          axi_wready <= 1'b1;
	        end
	      else
	        begin
	          axi_wready <= 1'b0;
	        end
	    end 
	end       

	// Implement memory mapped register select and write logic generation
	// The write data is accepted and written to memory mapped registers when
	// axi_awready, S_AXI_WVALID, axi_wready and S_AXI_WVALID are asserted. Write strobes are used to
	// select byte enables of slave registers while writing.
	// These registers are cleared when reset (active low) is applied.
	// Slave register write enable is asserted when valid address and data are available
	// and the slave is ready to accept the write address and write data.
	assign slv_reg_wren = axi_wready && S_AXI_WVALID && axi_awready && S_AXI_AWVALID;

	always @( posedge S_AXI_ACLK )
	begin
	  if ( S_AXI_ARESETN == 1'b0 )
	    begin
	      slv_reg0 <= 0;
	      slv_reg1 <= 0;
	      slv_reg2 <= 0;
	      slv_reg3 <= 0;
	      slv_reg4 <= 0;
	      slv_reg5 <= 0;
	      slv_reg6 <= 0;
	      slv_reg7 <= 0;
	      slv_reg8 <= 0;
	      slv_reg9 <= 0;
	    end 
	  else begin
	    if (slv_reg_wren)
	      begin
	        case ( axi_awaddr[ADDR_LSB+OPT_MEM_ADDR_BITS:ADDR_LSB] )
	          4'h0:
	            for ( byte_index = 0; byte_index <= (C_S_AXI_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
	              if ( S_AXI_WSTRB[byte_index] == 1 ) begin
	                // Respective byte enables are asserted as per write strobes 
	                // Slave register 0
	                slv_reg0[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
	              end  
	          4'h1:
	            for ( byte_index = 0; byte_index <= (C_S_AXI_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
	              if ( S_AXI_WSTRB[byte_index] == 1 ) begin
	                // Respective byte enables are asserted as per write strobes 
	                // Slave register 1
	                slv_reg1[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
	              end  
	          4'h2:
	            for ( byte_index = 0; byte_index <= (C_S_AXI_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
	              if ( S_AXI_WSTRB[byte_index] == 1 ) begin
	                // Respective byte enables are asserted as per write strobes 
	                // Slave register 2
	                slv_reg2[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
	              end  
	          4'h3:
	            for ( byte_index = 0; byte_index <= (C_S_AXI_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
	              if ( S_AXI_WSTRB[byte_index] == 1 ) begin
	                // Respective byte enables are asserted as per write strobes 
	                // Slave register 3
	                slv_reg3[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
	              end  
	          4'h4:
	            for ( byte_index = 0; byte_index <= (C_S_AXI_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
	              if ( S_AXI_WSTRB[byte_index] == 1 ) begin
	                // Respective byte enables are asserted as per write strobes 
	                // Slave register 4
	                slv_reg4[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
	              end  
	          4'h5:
	            for ( byte_index = 0; byte_index <= (C_S_AXI_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
	              if ( S_AXI_WSTRB[byte_index] == 1 ) begin
	                // Respective byte enables are asserted as per write strobes 
	                // Slave register 5
	                slv_reg5[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
	              end  
	          4'h6:
	            for ( byte_index = 0; byte_index <= (C_S_AXI_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
	              if ( S_AXI_WSTRB[byte_index] == 1 ) begin
	                // Respective byte enables are asserted as per write strobes 
	                // Slave register 6
	                slv_reg6[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
	              end  
	          4'h7:
	            for ( byte_index = 0; byte_index <= (C_S_AXI_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
	              if ( S_AXI_WSTRB[byte_index] == 1 ) begin
	                // Respective byte enables are asserted as per write strobes 
	                // Slave register 7
	                slv_reg7[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
	              end  
	          4'h8:
	            for ( byte_index = 0; byte_index <= (C_S_AXI_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
	              if ( S_AXI_WSTRB[byte_index] == 1 ) begin
	                // Respective byte enables are asserted as per write strobes 
	                // Slave register 8
	                slv_reg8[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
	              end  
	          4'h9:
	            for ( byte_index = 0; byte_index <= (C_S_AXI_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
	              if ( S_AXI_WSTRB[byte_index] == 1 ) begin
	                // Respective byte enables are asserted as per write strobes 
	                // Slave register 9
	                slv_reg9[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
	              end  
	          default : begin
	                      slv_reg0 <= slv_reg0;
	                      slv_reg1 <= slv_reg1;
	                      slv_reg2 <= slv_reg2;
	                      slv_reg3 <= slv_reg3;
	                      slv_reg4 <= slv_reg4;
	                      slv_reg5 <= slv_reg5;
	                      slv_reg6 <= slv_reg6;
	                      slv_reg7 <= slv_reg7;
	                      slv_reg8 <= slv_reg8;
	                      slv_reg9 <= slv_reg9;
	                    end
	        endcase
	      end
	  end
	end    

	// Implement write response logic generation
	// The write response and response valid signals are asserted by the slave 
	// when axi_wready, S_AXI_WVALID, axi_wready and S_AXI_WVALID are asserted.  
	// This marks the acceptance of address and indicates the status of 
	// write transaction.

	always @( posedge S_AXI_ACLK )
	begin
	  if ( S_AXI_ARESETN == 1'b0 )
	    begin
	      axi_bvalid  <= 0;
	      axi_bresp   <= 2'b0;
	    end 
	  else
	    begin    
	      if (axi_awready && S_AXI_AWVALID && ~axi_bvalid && axi_wready && S_AXI_WVALID)
	        begin
	          // indicates a valid write response is available
	          axi_bvalid <= 1'b1;
	          axi_bresp  <= 2'b0; // 'OKAY' response 
	        end                   // work error responses in future
	      else
	        begin
	          if (S_AXI_BREADY && axi_bvalid) 
	            //check if bready is asserted while bvalid is high) 
	            //(there is a possibility that bready is always asserted high)   
	            begin
	              axi_bvalid <= 1'b0; 
	            end  
	        end
	    end
	end   

	// Implement axi_arready generation
	// axi_arready is asserted for one S_AXI_ACLK clock cycle when
	// S_AXI_ARVALID is asserted. axi_awready is 
	// de-asserted when reset (active low) is asserted. 
	// The read address is also latched when S_AXI_ARVALID is 
	// asserted. axi_araddr is reset to zero on reset assertion.

	always @( posedge S_AXI_ACLK )
	begin
	  if ( S_AXI_ARESETN == 1'b0 )
	    begin
	      axi_arready <= 1'b0;
	      axi_araddr  <= 32'b0;
	    end 
	  else
	    begin    
	      if (~axi_arready && S_AXI_ARVALID)
	        begin
	          // indicates that the slave has acceped the valid read address
	          axi_arready <= 1'b1;
	          // Read address latching
	          axi_araddr  <= S_AXI_ARADDR;
	        end
	      else
	        begin
	          axi_arready <= 1'b0;
	        end
	    end 
	end       

	// Implement axi_arvalid generation
	// axi_rvalid is asserted for one S_AXI_ACLK clock cycle when both 
	// S_AXI_ARVALID and axi_arready are asserted. The slave registers 
	// data are available on the axi_rdata bus at this instance. The 
	// assertion of axi_rvalid marks the validity of read data on the 
	// bus and axi_rresp indicates the status of read transaction.axi_rvalid 
	// is deasserted on reset (active low). axi_rresp and axi_rdata are 
	// cleared to zero on reset (active low).  
	always @( posedge S_AXI_ACLK )
	begin
	  if ( S_AXI_ARESETN == 1'b0 )
	    begin
	      axi_rvalid <= 0;
	      axi_rresp  <= 0;
	    end 
	  else
	    begin    
	      if (axi_arready && S_AXI_ARVALID && ~axi_rvalid)
	        begin
	          // Valid read data is available at the read data bus
	          axi_rvalid <= 1'b1;
	          axi_rresp  <= 2'b0; // 'OKAY' response
	        end   
	      else if (axi_rvalid && S_AXI_RREADY)
	        begin
	          // Read data is accepted by the master
	          axi_rvalid <= 1'b0;
	        end                
	    end
	end    

	reg [3:0] state;
	reg start_mat_mul;
	wire done_mat_mul;
	reg mat_mul_done_status;

	// Implement memory mapped register select and read logic generation
	// Slave register read enable is asserted when valid address is available
	// and the slave is ready to accept the read address.
	assign slv_reg_rden = axi_arready & S_AXI_ARVALID & ~axi_rvalid;
	always @(*)
	begin
	      // Address decoding for reading registers
	      case ( axi_araddr[ADDR_LSB+OPT_MEM_ADDR_BITS:ADDR_LSB] )
	        4'h0   : reg_data_out <= slv_reg0;
	        4'h1   : reg_data_out <= {31'h0, mat_mul_done_status};
	        4'h2   : reg_data_out <= slv_reg2; //addresses for mat
	        4'h3   : reg_data_out <= slv_reg3;
	        4'h4   : reg_data_out <= slv_reg4;
	        4'h5   : reg_data_out <= state;  //debug
	        4'h6   : reg_data_out <= slv_reg6; //validity masks
	        4'h7   : reg_data_out <= slv_reg7;
	        4'h8   : reg_data_out <= slv_reg8;
	        4'h9   : reg_data_out <= 32'hdead_beef ;  //debug
	        default : reg_data_out <= 0;
	      endcase
	end

	// Output register or memory read data
	always @( posedge S_AXI_ACLK )
	begin
	  if ( S_AXI_ARESETN == 1'b0 )
	    begin
	      axi_rdata  <= 0;
	    end 
	  else
	    begin    
	      // When there is a valid read address (S_AXI_ARVALID) with 
	      // acceptance of read address by the slave (axi_arready), 
	      // output the read dada 
	      if (slv_reg_rden)
	        begin
	          axi_rdata <= reg_data_out;     // register read data
	        end   
	    end
	end    

	// Add user logic here
	
	
	wire start_reg;	
	assign start_reg = slv_reg0[0];	
	
	wire clear_done_reg;
	assign clear_done_reg = slv_reg1[0];
	
	wire clk;
    assign clk = S_AXI_ACLK;
    wire reset;
    assign reset = ~S_AXI_ARESETN;
	wire resetn;
	assign resetn = S_AXI_ARESETN;
	
	always @( posedge clk) begin
      if (resetn == 1'b0) begin
        state <= 4'b0000;
        start_mat_mul <= 1'b0;
		mat_mul_done_status <= 1'b0;
      end else begin
        case (state)
        4'b0000: begin
          start_mat_mul <= 1'b0;
		  mat_mul_done_status <= 1'b0;
          if (start_reg == 1'b1) begin
            state <= 4'b0001;
          end else begin
            state <= 4'b0000;
          end
        end
        
        4'b0001: begin
          start_mat_mul <= 1'b1;	      
          state <= 4'b1010;                    
        end      
        
        
        4'b1010: begin                 
          if (done_mat_mul == 1'b1) begin
            start_mat_mul <= 1'b0;
            state <= 4'b1000;
          end
          else begin
            state <= 4'b1010;
          end
        end

       4'b1000: begin
         if (clear_done_reg == 1'b1) begin
           state <= 4'b0000;
		   mat_mul_done_status <= 1'b0;
         end
         else begin
           state <= 4'b1000;
		   mat_mul_done_status <= 1'b1;
         end
       end
      endcase  
	end 
  end




//Connections for bram c (output matrix)
//bram_addr_c -> connected to u_matmul_4x4 block
//bram_rdata_c -> not used
//bram_wdata_c -> connected to u_matmul_4x4 block
//bram_we_c -> set to 1 when c_data is available
//bram_en_c -> hardcoded to 1 
  
  wire c_data_available;
  assign bram_en_c = 1'b1;
  assign bram_we_c = (c_data_available) ? 4'b1111 : 4'b0000;  

//Connections for bram a (first input matrix)
//bram_addr_a -> connected to u_matmul_4x4
//bram_rdata_a -> connected to u_matmul_4x4
//bram_wdata_a -> hardcoded to 0 (this block only reads from bram a)
//bram_we_a -> hardcoded to 0 (this block only reads from bram a)
//bram_en_a -> hardcoded to 1

  assign bram_wdata_a = 32'b0;
  assign bram_en_a = 1'b1;
  assign bram_we_a = 4'b0;
  
//Connections for bram b (second input matrix)
//bram_addr_b -> connected to u_matmul_4x4
//bram_rdata_b -> connected to u_matmul_4x4
//bram_wdata_b -> hardcoded to 0 (this block only reads from bram b)
//bram_we_b -> hardcoded to 0 (this block only reads from bram b)
//bram_en_b -> hardcoded to 1

  assign bram_wdata_b = 32'b0;
  assign bram_en_b = 1'b1;
  assign bram_we_b = 4'b0;
  
//NC wires 
wire [`BB_MAT_MUL_SIZE*`DWIDTH-1:0] a_data_out_NC;
wire [`BB_MAT_MUL_SIZE*`DWIDTH-1:0] b_data_out_NC;
wire [`BB_MAT_MUL_SIZE*`DWIDTH-1:0] a_data_in_NC;
wire [`BB_MAT_MUL_SIZE*`DWIDTH-1:0] b_data_in_NC;

matmul_20x20_systolic u_matmul(
  .clk(clk),
  .reset(reset),
  .pe_reset(reset),
  .start_mat_mul(start_mat_mul),
  .done_mat_mul(done_mat_mul),
  .address_mat_a(slv_reg2[`AWIDTH-1:0]),
  .address_mat_b(slv_reg3[`AWIDTH-1:0]),
  .address_mat_c(slv_reg4[`AWIDTH-1:0]),
  .address_stride_a({{(`ADDR_STRIDE_WIDTH-1){1'b0}},1'b1}),
  .address_stride_b({{(`ADDR_STRIDE_WIDTH-1){1'b0}},1'b1}),
  .address_stride_c({{(`ADDR_STRIDE_WIDTH-1){1'b0}},1'b1}),
  .a_data(bram_rdata_a),
  .b_data(bram_rdata_b),
  .a_data_in(a_data_in_NC),
  .b_data_in(b_data_in_NC),
  .c_data_in({`BB_MAT_MUL_SIZE*`DWIDTH{1'b0}}),
  .c_data_out(bram_wdata_c),
  .a_data_out(a_data_out_NC),
  .b_data_out(b_data_out_NC),
  .a_addr(bram_addr_a),
  .b_addr(bram_addr_b),
  .c_addr(bram_addr_c),
  .c_data_available(c_data_available),
  .validity_mask_a_rows(slv_reg6[`MASK_WIDTH-1:0]),
  .validity_mask_a_cols(slv_reg7[`MASK_WIDTH-1:0]),
  .validity_mask_b_rows(slv_reg7[`MASK_WIDTH-1:0]),
  .validity_mask_b_cols(slv_reg8[`MASK_WIDTH-1:0]),
  .a_loc(8'd0),
  .b_loc(8'd0)
);

endmodule

//////////////////////////////////////////////////////////////////////////////////
// The main matrix multiplication module
//////////////////////////////////////////////////////////////////////////////////

module matmul_20x20_systolic(
 clk,
 reset,
 pe_reset,
 start_mat_mul,
 done_mat_mul,
 address_mat_a,
 address_mat_b,
 address_mat_c,
 address_stride_a,
 address_stride_b,
 address_stride_c,
 a_data,
 b_data,
 a_data_in, //Data values coming in from previous matmul - systolic connections
 b_data_in,
 c_data_in, //Data values coming in from previous matmul - systolic shifting
 c_data_out, //Data values going out to next matmul - systolic shifting
 a_data_out,
 b_data_out,
 a_addr,
 b_addr,
 c_addr,
 c_data_available,

 validity_mask_a_rows,
 validity_mask_a_cols,
 validity_mask_b_rows,
 validity_mask_b_cols,
  
 a_loc,
 b_loc
);

 input clk;
 input reset;
 input pe_reset;
 input start_mat_mul;
 output done_mat_mul;
 input [`AWIDTH-1:0] address_mat_a;
 input [`AWIDTH-1:0] address_mat_b;
 input [`AWIDTH-1:0] address_mat_c;
 input [`ADDR_STRIDE_WIDTH-1:0] address_stride_a;
 input [`ADDR_STRIDE_WIDTH-1:0] address_stride_b;
 input [`ADDR_STRIDE_WIDTH-1:0] address_stride_c;
 input [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data;
 input [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data;
 input [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_in;
 input [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_in;
 input [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_in;
 output [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_out;
 output [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_out;
 output [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_out;
 output [`AWIDTH-1:0] a_addr;
 output [`AWIDTH-1:0] b_addr;
 output [`AWIDTH-1:0] c_addr;
 output c_data_available;

 input [`MASK_WIDTH-1:0] validity_mask_a_rows;
 input [`MASK_WIDTH-1:0] validity_mask_a_cols;
 input [`MASK_WIDTH-1:0] validity_mask_b_rows;
 input [`MASK_WIDTH-1:0] validity_mask_b_cols;

 input [7:0] a_loc;
 input [7:0] b_loc;

//////////////////////////////////////////////////////////////////////////
// Logic for clock counting and when to assert done
//////////////////////////////////////////////////////////////////////////

reg done_mat_mul;
//This is 7 bits because the expectation is that clock count will be pretty
//small. For large matmuls, this will need to increased to have more bits.
//In general, a systolic multiplier takes 4*N-2+P cycles, where N is the size 
//of the matmul and P is the number of pipleine stages in the MAC block.
reg [7:0] clk_cnt;

//Finding out number of cycles to assert matmul done.
//When we have to save the outputs to accumulators, then we don't need to
//shift out data. So, we can assert done_mat_mul early.
//In the normal case, we have to include the time to shift out the results. 
//Note: the count expression used to contain "4*final_mat_mul_size", but 
//to avoid multiplication, we now use "final_mat_mul_size<<2"
wire [7:0] clk_cnt_for_done;

assign clk_cnt_for_done = 
                          (81);  
    
always @(posedge clk) begin
  if (reset || ~start_mat_mul) begin
    clk_cnt <= 0;
    done_mat_mul <= 0;
  end
  else if (clk_cnt == clk_cnt_for_done) begin
    done_mat_mul <= 1;
    clk_cnt <= clk_cnt + 1;

  end
  else if (done_mat_mul == 0) begin
    clk_cnt <= clk_cnt + 1;

  end    
  else begin
    done_mat_mul <= 0;
    clk_cnt <= clk_cnt + 1;
  end
end
wire [`DWIDTH-1:0] a0_data;
wire [`DWIDTH-1:0] a1_data;
wire [`DWIDTH-1:0] a2_data;
wire [`DWIDTH-1:0] a3_data;
wire [`DWIDTH-1:0] a4_data;
wire [`DWIDTH-1:0] a5_data;
wire [`DWIDTH-1:0] a6_data;
wire [`DWIDTH-1:0] a7_data;
wire [`DWIDTH-1:0] a8_data;
wire [`DWIDTH-1:0] a9_data;
wire [`DWIDTH-1:0] a10_data;
wire [`DWIDTH-1:0] a11_data;
wire [`DWIDTH-1:0] a12_data;
wire [`DWIDTH-1:0] a13_data;
wire [`DWIDTH-1:0] a14_data;
wire [`DWIDTH-1:0] a15_data;
wire [`DWIDTH-1:0] a16_data;
wire [`DWIDTH-1:0] a17_data;
wire [`DWIDTH-1:0] a18_data;
wire [`DWIDTH-1:0] a19_data;
wire [`DWIDTH-1:0] b0_data;
wire [`DWIDTH-1:0] b1_data;
wire [`DWIDTH-1:0] b2_data;
wire [`DWIDTH-1:0] b3_data;
wire [`DWIDTH-1:0] b4_data;
wire [`DWIDTH-1:0] b5_data;
wire [`DWIDTH-1:0] b6_data;
wire [`DWIDTH-1:0] b7_data;
wire [`DWIDTH-1:0] b8_data;
wire [`DWIDTH-1:0] b9_data;
wire [`DWIDTH-1:0] b10_data;
wire [`DWIDTH-1:0] b11_data;
wire [`DWIDTH-1:0] b12_data;
wire [`DWIDTH-1:0] b13_data;
wire [`DWIDTH-1:0] b14_data;
wire [`DWIDTH-1:0] b15_data;
wire [`DWIDTH-1:0] b16_data;
wire [`DWIDTH-1:0] b17_data;
wire [`DWIDTH-1:0] b18_data;
wire [`DWIDTH-1:0] b19_data;
wire [`DWIDTH-1:0] a1_data_delayed_1;
wire [`DWIDTH-1:0] a2_data_delayed_1;
wire [`DWIDTH-1:0] a2_data_delayed_2;
wire [`DWIDTH-1:0] a3_data_delayed_1;
wire [`DWIDTH-1:0] a3_data_delayed_2;
wire [`DWIDTH-1:0] a3_data_delayed_3;
wire [`DWIDTH-1:0] a4_data_delayed_1;
wire [`DWIDTH-1:0] a4_data_delayed_2;
wire [`DWIDTH-1:0] a4_data_delayed_3;
wire [`DWIDTH-1:0] a4_data_delayed_4;
wire [`DWIDTH-1:0] a5_data_delayed_1;
wire [`DWIDTH-1:0] a5_data_delayed_2;
wire [`DWIDTH-1:0] a5_data_delayed_3;
wire [`DWIDTH-1:0] a5_data_delayed_4;
wire [`DWIDTH-1:0] a5_data_delayed_5;
wire [`DWIDTH-1:0] a6_data_delayed_1;
wire [`DWIDTH-1:0] a6_data_delayed_2;
wire [`DWIDTH-1:0] a6_data_delayed_3;
wire [`DWIDTH-1:0] a6_data_delayed_4;
wire [`DWIDTH-1:0] a6_data_delayed_5;
wire [`DWIDTH-1:0] a6_data_delayed_6;
wire [`DWIDTH-1:0] a7_data_delayed_1;
wire [`DWIDTH-1:0] a7_data_delayed_2;
wire [`DWIDTH-1:0] a7_data_delayed_3;
wire [`DWIDTH-1:0] a7_data_delayed_4;
wire [`DWIDTH-1:0] a7_data_delayed_5;
wire [`DWIDTH-1:0] a7_data_delayed_6;
wire [`DWIDTH-1:0] a7_data_delayed_7;
wire [`DWIDTH-1:0] a8_data_delayed_1;
wire [`DWIDTH-1:0] a8_data_delayed_2;
wire [`DWIDTH-1:0] a8_data_delayed_3;
wire [`DWIDTH-1:0] a8_data_delayed_4;
wire [`DWIDTH-1:0] a8_data_delayed_5;
wire [`DWIDTH-1:0] a8_data_delayed_6;
wire [`DWIDTH-1:0] a8_data_delayed_7;
wire [`DWIDTH-1:0] a8_data_delayed_8;
wire [`DWIDTH-1:0] a9_data_delayed_1;
wire [`DWIDTH-1:0] a9_data_delayed_2;
wire [`DWIDTH-1:0] a9_data_delayed_3;
wire [`DWIDTH-1:0] a9_data_delayed_4;
wire [`DWIDTH-1:0] a9_data_delayed_5;
wire [`DWIDTH-1:0] a9_data_delayed_6;
wire [`DWIDTH-1:0] a9_data_delayed_7;
wire [`DWIDTH-1:0] a9_data_delayed_8;
wire [`DWIDTH-1:0] a9_data_delayed_9;
wire [`DWIDTH-1:0] a10_data_delayed_1;
wire [`DWIDTH-1:0] a10_data_delayed_2;
wire [`DWIDTH-1:0] a10_data_delayed_3;
wire [`DWIDTH-1:0] a10_data_delayed_4;
wire [`DWIDTH-1:0] a10_data_delayed_5;
wire [`DWIDTH-1:0] a10_data_delayed_6;
wire [`DWIDTH-1:0] a10_data_delayed_7;
wire [`DWIDTH-1:0] a10_data_delayed_8;
wire [`DWIDTH-1:0] a10_data_delayed_9;
wire [`DWIDTH-1:0] a10_data_delayed_10;
wire [`DWIDTH-1:0] a11_data_delayed_1;
wire [`DWIDTH-1:0] a11_data_delayed_2;
wire [`DWIDTH-1:0] a11_data_delayed_3;
wire [`DWIDTH-1:0] a11_data_delayed_4;
wire [`DWIDTH-1:0] a11_data_delayed_5;
wire [`DWIDTH-1:0] a11_data_delayed_6;
wire [`DWIDTH-1:0] a11_data_delayed_7;
wire [`DWIDTH-1:0] a11_data_delayed_8;
wire [`DWIDTH-1:0] a11_data_delayed_9;
wire [`DWIDTH-1:0] a11_data_delayed_10;
wire [`DWIDTH-1:0] a11_data_delayed_11;
wire [`DWIDTH-1:0] a12_data_delayed_1;
wire [`DWIDTH-1:0] a12_data_delayed_2;
wire [`DWIDTH-1:0] a12_data_delayed_3;
wire [`DWIDTH-1:0] a12_data_delayed_4;
wire [`DWIDTH-1:0] a12_data_delayed_5;
wire [`DWIDTH-1:0] a12_data_delayed_6;
wire [`DWIDTH-1:0] a12_data_delayed_7;
wire [`DWIDTH-1:0] a12_data_delayed_8;
wire [`DWIDTH-1:0] a12_data_delayed_9;
wire [`DWIDTH-1:0] a12_data_delayed_10;
wire [`DWIDTH-1:0] a12_data_delayed_11;
wire [`DWIDTH-1:0] a12_data_delayed_12;
wire [`DWIDTH-1:0] a13_data_delayed_1;
wire [`DWIDTH-1:0] a13_data_delayed_2;
wire [`DWIDTH-1:0] a13_data_delayed_3;
wire [`DWIDTH-1:0] a13_data_delayed_4;
wire [`DWIDTH-1:0] a13_data_delayed_5;
wire [`DWIDTH-1:0] a13_data_delayed_6;
wire [`DWIDTH-1:0] a13_data_delayed_7;
wire [`DWIDTH-1:0] a13_data_delayed_8;
wire [`DWIDTH-1:0] a13_data_delayed_9;
wire [`DWIDTH-1:0] a13_data_delayed_10;
wire [`DWIDTH-1:0] a13_data_delayed_11;
wire [`DWIDTH-1:0] a13_data_delayed_12;
wire [`DWIDTH-1:0] a13_data_delayed_13;
wire [`DWIDTH-1:0] a14_data_delayed_1;
wire [`DWIDTH-1:0] a14_data_delayed_2;
wire [`DWIDTH-1:0] a14_data_delayed_3;
wire [`DWIDTH-1:0] a14_data_delayed_4;
wire [`DWIDTH-1:0] a14_data_delayed_5;
wire [`DWIDTH-1:0] a14_data_delayed_6;
wire [`DWIDTH-1:0] a14_data_delayed_7;
wire [`DWIDTH-1:0] a14_data_delayed_8;
wire [`DWIDTH-1:0] a14_data_delayed_9;
wire [`DWIDTH-1:0] a14_data_delayed_10;
wire [`DWIDTH-1:0] a14_data_delayed_11;
wire [`DWIDTH-1:0] a14_data_delayed_12;
wire [`DWIDTH-1:0] a14_data_delayed_13;
wire [`DWIDTH-1:0] a14_data_delayed_14;
wire [`DWIDTH-1:0] a15_data_delayed_1;
wire [`DWIDTH-1:0] a15_data_delayed_2;
wire [`DWIDTH-1:0] a15_data_delayed_3;
wire [`DWIDTH-1:0] a15_data_delayed_4;
wire [`DWIDTH-1:0] a15_data_delayed_5;
wire [`DWIDTH-1:0] a15_data_delayed_6;
wire [`DWIDTH-1:0] a15_data_delayed_7;
wire [`DWIDTH-1:0] a15_data_delayed_8;
wire [`DWIDTH-1:0] a15_data_delayed_9;
wire [`DWIDTH-1:0] a15_data_delayed_10;
wire [`DWIDTH-1:0] a15_data_delayed_11;
wire [`DWIDTH-1:0] a15_data_delayed_12;
wire [`DWIDTH-1:0] a15_data_delayed_13;
wire [`DWIDTH-1:0] a15_data_delayed_14;
wire [`DWIDTH-1:0] a15_data_delayed_15;
wire [`DWIDTH-1:0] a16_data_delayed_1;
wire [`DWIDTH-1:0] a16_data_delayed_2;
wire [`DWIDTH-1:0] a16_data_delayed_3;
wire [`DWIDTH-1:0] a16_data_delayed_4;
wire [`DWIDTH-1:0] a16_data_delayed_5;
wire [`DWIDTH-1:0] a16_data_delayed_6;
wire [`DWIDTH-1:0] a16_data_delayed_7;
wire [`DWIDTH-1:0] a16_data_delayed_8;
wire [`DWIDTH-1:0] a16_data_delayed_9;
wire [`DWIDTH-1:0] a16_data_delayed_10;
wire [`DWIDTH-1:0] a16_data_delayed_11;
wire [`DWIDTH-1:0] a16_data_delayed_12;
wire [`DWIDTH-1:0] a16_data_delayed_13;
wire [`DWIDTH-1:0] a16_data_delayed_14;
wire [`DWIDTH-1:0] a16_data_delayed_15;
wire [`DWIDTH-1:0] a16_data_delayed_16;
wire [`DWIDTH-1:0] a17_data_delayed_1;
wire [`DWIDTH-1:0] a17_data_delayed_2;
wire [`DWIDTH-1:0] a17_data_delayed_3;
wire [`DWIDTH-1:0] a17_data_delayed_4;
wire [`DWIDTH-1:0] a17_data_delayed_5;
wire [`DWIDTH-1:0] a17_data_delayed_6;
wire [`DWIDTH-1:0] a17_data_delayed_7;
wire [`DWIDTH-1:0] a17_data_delayed_8;
wire [`DWIDTH-1:0] a17_data_delayed_9;
wire [`DWIDTH-1:0] a17_data_delayed_10;
wire [`DWIDTH-1:0] a17_data_delayed_11;
wire [`DWIDTH-1:0] a17_data_delayed_12;
wire [`DWIDTH-1:0] a17_data_delayed_13;
wire [`DWIDTH-1:0] a17_data_delayed_14;
wire [`DWIDTH-1:0] a17_data_delayed_15;
wire [`DWIDTH-1:0] a17_data_delayed_16;
wire [`DWIDTH-1:0] a17_data_delayed_17;
wire [`DWIDTH-1:0] a18_data_delayed_1;
wire [`DWIDTH-1:0] a18_data_delayed_2;
wire [`DWIDTH-1:0] a18_data_delayed_3;
wire [`DWIDTH-1:0] a18_data_delayed_4;
wire [`DWIDTH-1:0] a18_data_delayed_5;
wire [`DWIDTH-1:0] a18_data_delayed_6;
wire [`DWIDTH-1:0] a18_data_delayed_7;
wire [`DWIDTH-1:0] a18_data_delayed_8;
wire [`DWIDTH-1:0] a18_data_delayed_9;
wire [`DWIDTH-1:0] a18_data_delayed_10;
wire [`DWIDTH-1:0] a18_data_delayed_11;
wire [`DWIDTH-1:0] a18_data_delayed_12;
wire [`DWIDTH-1:0] a18_data_delayed_13;
wire [`DWIDTH-1:0] a18_data_delayed_14;
wire [`DWIDTH-1:0] a18_data_delayed_15;
wire [`DWIDTH-1:0] a18_data_delayed_16;
wire [`DWIDTH-1:0] a18_data_delayed_17;
wire [`DWIDTH-1:0] a18_data_delayed_18;
wire [`DWIDTH-1:0] a19_data_delayed_1;
wire [`DWIDTH-1:0] a19_data_delayed_2;
wire [`DWIDTH-1:0] a19_data_delayed_3;
wire [`DWIDTH-1:0] a19_data_delayed_4;
wire [`DWIDTH-1:0] a19_data_delayed_5;
wire [`DWIDTH-1:0] a19_data_delayed_6;
wire [`DWIDTH-1:0] a19_data_delayed_7;
wire [`DWIDTH-1:0] a19_data_delayed_8;
wire [`DWIDTH-1:0] a19_data_delayed_9;
wire [`DWIDTH-1:0] a19_data_delayed_10;
wire [`DWIDTH-1:0] a19_data_delayed_11;
wire [`DWIDTH-1:0] a19_data_delayed_12;
wire [`DWIDTH-1:0] a19_data_delayed_13;
wire [`DWIDTH-1:0] a19_data_delayed_14;
wire [`DWIDTH-1:0] a19_data_delayed_15;
wire [`DWIDTH-1:0] a19_data_delayed_16;
wire [`DWIDTH-1:0] a19_data_delayed_17;
wire [`DWIDTH-1:0] a19_data_delayed_18;
wire [`DWIDTH-1:0] a19_data_delayed_19;
wire [`DWIDTH-1:0] b1_data_delayed_1;
wire [`DWIDTH-1:0] b2_data_delayed_1;
wire [`DWIDTH-1:0] b2_data_delayed_2;
wire [`DWIDTH-1:0] b3_data_delayed_1;
wire [`DWIDTH-1:0] b3_data_delayed_2;
wire [`DWIDTH-1:0] b3_data_delayed_3;
wire [`DWIDTH-1:0] b4_data_delayed_1;
wire [`DWIDTH-1:0] b4_data_delayed_2;
wire [`DWIDTH-1:0] b4_data_delayed_3;
wire [`DWIDTH-1:0] b4_data_delayed_4;
wire [`DWIDTH-1:0] b5_data_delayed_1;
wire [`DWIDTH-1:0] b5_data_delayed_2;
wire [`DWIDTH-1:0] b5_data_delayed_3;
wire [`DWIDTH-1:0] b5_data_delayed_4;
wire [`DWIDTH-1:0] b5_data_delayed_5;
wire [`DWIDTH-1:0] b6_data_delayed_1;
wire [`DWIDTH-1:0] b6_data_delayed_2;
wire [`DWIDTH-1:0] b6_data_delayed_3;
wire [`DWIDTH-1:0] b6_data_delayed_4;
wire [`DWIDTH-1:0] b6_data_delayed_5;
wire [`DWIDTH-1:0] b6_data_delayed_6;
wire [`DWIDTH-1:0] b7_data_delayed_1;
wire [`DWIDTH-1:0] b7_data_delayed_2;
wire [`DWIDTH-1:0] b7_data_delayed_3;
wire [`DWIDTH-1:0] b7_data_delayed_4;
wire [`DWIDTH-1:0] b7_data_delayed_5;
wire [`DWIDTH-1:0] b7_data_delayed_6;
wire [`DWIDTH-1:0] b7_data_delayed_7;
wire [`DWIDTH-1:0] b8_data_delayed_1;
wire [`DWIDTH-1:0] b8_data_delayed_2;
wire [`DWIDTH-1:0] b8_data_delayed_3;
wire [`DWIDTH-1:0] b8_data_delayed_4;
wire [`DWIDTH-1:0] b8_data_delayed_5;
wire [`DWIDTH-1:0] b8_data_delayed_6;
wire [`DWIDTH-1:0] b8_data_delayed_7;
wire [`DWIDTH-1:0] b8_data_delayed_8;
wire [`DWIDTH-1:0] b9_data_delayed_1;
wire [`DWIDTH-1:0] b9_data_delayed_2;
wire [`DWIDTH-1:0] b9_data_delayed_3;
wire [`DWIDTH-1:0] b9_data_delayed_4;
wire [`DWIDTH-1:0] b9_data_delayed_5;
wire [`DWIDTH-1:0] b9_data_delayed_6;
wire [`DWIDTH-1:0] b9_data_delayed_7;
wire [`DWIDTH-1:0] b9_data_delayed_8;
wire [`DWIDTH-1:0] b9_data_delayed_9;
wire [`DWIDTH-1:0] b10_data_delayed_1;
wire [`DWIDTH-1:0] b10_data_delayed_2;
wire [`DWIDTH-1:0] b10_data_delayed_3;
wire [`DWIDTH-1:0] b10_data_delayed_4;
wire [`DWIDTH-1:0] b10_data_delayed_5;
wire [`DWIDTH-1:0] b10_data_delayed_6;
wire [`DWIDTH-1:0] b10_data_delayed_7;
wire [`DWIDTH-1:0] b10_data_delayed_8;
wire [`DWIDTH-1:0] b10_data_delayed_9;
wire [`DWIDTH-1:0] b10_data_delayed_10;
wire [`DWIDTH-1:0] b11_data_delayed_1;
wire [`DWIDTH-1:0] b11_data_delayed_2;
wire [`DWIDTH-1:0] b11_data_delayed_3;
wire [`DWIDTH-1:0] b11_data_delayed_4;
wire [`DWIDTH-1:0] b11_data_delayed_5;
wire [`DWIDTH-1:0] b11_data_delayed_6;
wire [`DWIDTH-1:0] b11_data_delayed_7;
wire [`DWIDTH-1:0] b11_data_delayed_8;
wire [`DWIDTH-1:0] b11_data_delayed_9;
wire [`DWIDTH-1:0] b11_data_delayed_10;
wire [`DWIDTH-1:0] b11_data_delayed_11;
wire [`DWIDTH-1:0] b12_data_delayed_1;
wire [`DWIDTH-1:0] b12_data_delayed_2;
wire [`DWIDTH-1:0] b12_data_delayed_3;
wire [`DWIDTH-1:0] b12_data_delayed_4;
wire [`DWIDTH-1:0] b12_data_delayed_5;
wire [`DWIDTH-1:0] b12_data_delayed_6;
wire [`DWIDTH-1:0] b12_data_delayed_7;
wire [`DWIDTH-1:0] b12_data_delayed_8;
wire [`DWIDTH-1:0] b12_data_delayed_9;
wire [`DWIDTH-1:0] b12_data_delayed_10;
wire [`DWIDTH-1:0] b12_data_delayed_11;
wire [`DWIDTH-1:0] b12_data_delayed_12;
wire [`DWIDTH-1:0] b13_data_delayed_1;
wire [`DWIDTH-1:0] b13_data_delayed_2;
wire [`DWIDTH-1:0] b13_data_delayed_3;
wire [`DWIDTH-1:0] b13_data_delayed_4;
wire [`DWIDTH-1:0] b13_data_delayed_5;
wire [`DWIDTH-1:0] b13_data_delayed_6;
wire [`DWIDTH-1:0] b13_data_delayed_7;
wire [`DWIDTH-1:0] b13_data_delayed_8;
wire [`DWIDTH-1:0] b13_data_delayed_9;
wire [`DWIDTH-1:0] b13_data_delayed_10;
wire [`DWIDTH-1:0] b13_data_delayed_11;
wire [`DWIDTH-1:0] b13_data_delayed_12;
wire [`DWIDTH-1:0] b13_data_delayed_13;
wire [`DWIDTH-1:0] b14_data_delayed_1;
wire [`DWIDTH-1:0] b14_data_delayed_2;
wire [`DWIDTH-1:0] b14_data_delayed_3;
wire [`DWIDTH-1:0] b14_data_delayed_4;
wire [`DWIDTH-1:0] b14_data_delayed_5;
wire [`DWIDTH-1:0] b14_data_delayed_6;
wire [`DWIDTH-1:0] b14_data_delayed_7;
wire [`DWIDTH-1:0] b14_data_delayed_8;
wire [`DWIDTH-1:0] b14_data_delayed_9;
wire [`DWIDTH-1:0] b14_data_delayed_10;
wire [`DWIDTH-1:0] b14_data_delayed_11;
wire [`DWIDTH-1:0] b14_data_delayed_12;
wire [`DWIDTH-1:0] b14_data_delayed_13;
wire [`DWIDTH-1:0] b14_data_delayed_14;
wire [`DWIDTH-1:0] b15_data_delayed_1;
wire [`DWIDTH-1:0] b15_data_delayed_2;
wire [`DWIDTH-1:0] b15_data_delayed_3;
wire [`DWIDTH-1:0] b15_data_delayed_4;
wire [`DWIDTH-1:0] b15_data_delayed_5;
wire [`DWIDTH-1:0] b15_data_delayed_6;
wire [`DWIDTH-1:0] b15_data_delayed_7;
wire [`DWIDTH-1:0] b15_data_delayed_8;
wire [`DWIDTH-1:0] b15_data_delayed_9;
wire [`DWIDTH-1:0] b15_data_delayed_10;
wire [`DWIDTH-1:0] b15_data_delayed_11;
wire [`DWIDTH-1:0] b15_data_delayed_12;
wire [`DWIDTH-1:0] b15_data_delayed_13;
wire [`DWIDTH-1:0] b15_data_delayed_14;
wire [`DWIDTH-1:0] b15_data_delayed_15;
wire [`DWIDTH-1:0] b16_data_delayed_1;
wire [`DWIDTH-1:0] b16_data_delayed_2;
wire [`DWIDTH-1:0] b16_data_delayed_3;
wire [`DWIDTH-1:0] b16_data_delayed_4;
wire [`DWIDTH-1:0] b16_data_delayed_5;
wire [`DWIDTH-1:0] b16_data_delayed_6;
wire [`DWIDTH-1:0] b16_data_delayed_7;
wire [`DWIDTH-1:0] b16_data_delayed_8;
wire [`DWIDTH-1:0] b16_data_delayed_9;
wire [`DWIDTH-1:0] b16_data_delayed_10;
wire [`DWIDTH-1:0] b16_data_delayed_11;
wire [`DWIDTH-1:0] b16_data_delayed_12;
wire [`DWIDTH-1:0] b16_data_delayed_13;
wire [`DWIDTH-1:0] b16_data_delayed_14;
wire [`DWIDTH-1:0] b16_data_delayed_15;
wire [`DWIDTH-1:0] b16_data_delayed_16;
wire [`DWIDTH-1:0] b17_data_delayed_1;
wire [`DWIDTH-1:0] b17_data_delayed_2;
wire [`DWIDTH-1:0] b17_data_delayed_3;
wire [`DWIDTH-1:0] b17_data_delayed_4;
wire [`DWIDTH-1:0] b17_data_delayed_5;
wire [`DWIDTH-1:0] b17_data_delayed_6;
wire [`DWIDTH-1:0] b17_data_delayed_7;
wire [`DWIDTH-1:0] b17_data_delayed_8;
wire [`DWIDTH-1:0] b17_data_delayed_9;
wire [`DWIDTH-1:0] b17_data_delayed_10;
wire [`DWIDTH-1:0] b17_data_delayed_11;
wire [`DWIDTH-1:0] b17_data_delayed_12;
wire [`DWIDTH-1:0] b17_data_delayed_13;
wire [`DWIDTH-1:0] b17_data_delayed_14;
wire [`DWIDTH-1:0] b17_data_delayed_15;
wire [`DWIDTH-1:0] b17_data_delayed_16;
wire [`DWIDTH-1:0] b17_data_delayed_17;
wire [`DWIDTH-1:0] b18_data_delayed_1;
wire [`DWIDTH-1:0] b18_data_delayed_2;
wire [`DWIDTH-1:0] b18_data_delayed_3;
wire [`DWIDTH-1:0] b18_data_delayed_4;
wire [`DWIDTH-1:0] b18_data_delayed_5;
wire [`DWIDTH-1:0] b18_data_delayed_6;
wire [`DWIDTH-1:0] b18_data_delayed_7;
wire [`DWIDTH-1:0] b18_data_delayed_8;
wire [`DWIDTH-1:0] b18_data_delayed_9;
wire [`DWIDTH-1:0] b18_data_delayed_10;
wire [`DWIDTH-1:0] b18_data_delayed_11;
wire [`DWIDTH-1:0] b18_data_delayed_12;
wire [`DWIDTH-1:0] b18_data_delayed_13;
wire [`DWIDTH-1:0] b18_data_delayed_14;
wire [`DWIDTH-1:0] b18_data_delayed_15;
wire [`DWIDTH-1:0] b18_data_delayed_16;
wire [`DWIDTH-1:0] b18_data_delayed_17;
wire [`DWIDTH-1:0] b18_data_delayed_18;
wire [`DWIDTH-1:0] b19_data_delayed_1;
wire [`DWIDTH-1:0] b19_data_delayed_2;
wire [`DWIDTH-1:0] b19_data_delayed_3;
wire [`DWIDTH-1:0] b19_data_delayed_4;
wire [`DWIDTH-1:0] b19_data_delayed_5;
wire [`DWIDTH-1:0] b19_data_delayed_6;
wire [`DWIDTH-1:0] b19_data_delayed_7;
wire [`DWIDTH-1:0] b19_data_delayed_8;
wire [`DWIDTH-1:0] b19_data_delayed_9;
wire [`DWIDTH-1:0] b19_data_delayed_10;
wire [`DWIDTH-1:0] b19_data_delayed_11;
wire [`DWIDTH-1:0] b19_data_delayed_12;
wire [`DWIDTH-1:0] b19_data_delayed_13;
wire [`DWIDTH-1:0] b19_data_delayed_14;
wire [`DWIDTH-1:0] b19_data_delayed_15;
wire [`DWIDTH-1:0] b19_data_delayed_16;
wire [`DWIDTH-1:0] b19_data_delayed_17;
wire [`DWIDTH-1:0] b19_data_delayed_18;
wire [`DWIDTH-1:0] b19_data_delayed_19;


//////////////////////////////////////////////////////////////////////////
// Instantiation of systolic data setup
//////////////////////////////////////////////////////////////////////////
systolic_data_setup u_systolic_data_setup(
.clk(clk),
.reset(reset),
.start_mat_mul(start_mat_mul),
.a_addr(a_addr),
.b_addr(b_addr),
.address_mat_a(address_mat_a),
.address_mat_b(address_mat_b),
.address_stride_a(address_stride_a),
.address_stride_b(address_stride_b),
.a_data(a_data),
.b_data(b_data),
.clk_cnt(clk_cnt),
.a0_data(a0_data),
.b0_data(b0_data),
.a1_data_delayed_1(a1_data_delayed_1),
.b1_data_delayed_1(b1_data_delayed_1),
.a2_data_delayed_2(a2_data_delayed_2),
.b2_data_delayed_2(b2_data_delayed_2),
.a3_data_delayed_3(a3_data_delayed_3),
.b3_data_delayed_3(b3_data_delayed_3),
.a4_data_delayed_4(a4_data_delayed_4),
.b4_data_delayed_4(b4_data_delayed_4),
.a5_data_delayed_5(a5_data_delayed_5),
.b5_data_delayed_5(b5_data_delayed_5),
.a6_data_delayed_6(a6_data_delayed_6),
.b6_data_delayed_6(b6_data_delayed_6),
.a7_data_delayed_7(a7_data_delayed_7),
.b7_data_delayed_7(b7_data_delayed_7),
.a8_data_delayed_8(a8_data_delayed_8),
.b8_data_delayed_8(b8_data_delayed_8),
.a9_data_delayed_9(a9_data_delayed_9),
.b9_data_delayed_9(b9_data_delayed_9),
.a10_data_delayed_10(a10_data_delayed_10),
.b10_data_delayed_10(b10_data_delayed_10),
.a11_data_delayed_11(a11_data_delayed_11),
.b11_data_delayed_11(b11_data_delayed_11),
.a12_data_delayed_12(a12_data_delayed_12),
.b12_data_delayed_12(b12_data_delayed_12),
.a13_data_delayed_13(a13_data_delayed_13),
.b13_data_delayed_13(b13_data_delayed_13),
.a14_data_delayed_14(a14_data_delayed_14),
.b14_data_delayed_14(b14_data_delayed_14),
.a15_data_delayed_15(a15_data_delayed_15),
.b15_data_delayed_15(b15_data_delayed_15),
.a16_data_delayed_16(a16_data_delayed_16),
.b16_data_delayed_16(b16_data_delayed_16),
.a17_data_delayed_17(a17_data_delayed_17),
.b17_data_delayed_17(b17_data_delayed_17),
.a18_data_delayed_18(a18_data_delayed_18),
.b18_data_delayed_18(b18_data_delayed_18),
.a19_data_delayed_19(a19_data_delayed_19),
.b19_data_delayed_19(b19_data_delayed_19),

.validity_mask_a_rows(validity_mask_a_rows),
.validity_mask_a_cols(validity_mask_a_cols),
.validity_mask_b_rows(validity_mask_b_rows),
.validity_mask_b_cols(validity_mask_b_cols),

.a_loc(a_loc),
.b_loc(b_loc)
);

//////////////////////////////////////////////////////////////////////////
// Logic to mux data_in coming from neighboring matmuls
//////////////////////////////////////////////////////////////////////////
wire [`DWIDTH-1:0] a0;
wire [`DWIDTH-1:0] a1;
wire [`DWIDTH-1:0] a2;
wire [`DWIDTH-1:0] a3;
wire [`DWIDTH-1:0] a4;
wire [`DWIDTH-1:0] a5;
wire [`DWIDTH-1:0] a6;
wire [`DWIDTH-1:0] a7;
wire [`DWIDTH-1:0] a8;
wire [`DWIDTH-1:0] a9;
wire [`DWIDTH-1:0] a10;
wire [`DWIDTH-1:0] a11;
wire [`DWIDTH-1:0] a12;
wire [`DWIDTH-1:0] a13;
wire [`DWIDTH-1:0] a14;
wire [`DWIDTH-1:0] a15;
wire [`DWIDTH-1:0] a16;
wire [`DWIDTH-1:0] a17;
wire [`DWIDTH-1:0] a18;
wire [`DWIDTH-1:0] a19;
wire [`DWIDTH-1:0] b0;
wire [`DWIDTH-1:0] b1;
wire [`DWIDTH-1:0] b2;
wire [`DWIDTH-1:0] b3;
wire [`DWIDTH-1:0] b4;
wire [`DWIDTH-1:0] b5;
wire [`DWIDTH-1:0] b6;
wire [`DWIDTH-1:0] b7;
wire [`DWIDTH-1:0] b8;
wire [`DWIDTH-1:0] b9;
wire [`DWIDTH-1:0] b10;
wire [`DWIDTH-1:0] b11;
wire [`DWIDTH-1:0] b12;
wire [`DWIDTH-1:0] b13;
wire [`DWIDTH-1:0] b14;
wire [`DWIDTH-1:0] b15;
wire [`DWIDTH-1:0] b16;
wire [`DWIDTH-1:0] b17;
wire [`DWIDTH-1:0] b18;
wire [`DWIDTH-1:0] b19;

wire [`DWIDTH-1:0] a0_data_in;
wire [`DWIDTH-1:0] a1_data_in;
wire [`DWIDTH-1:0] a2_data_in;
wire [`DWIDTH-1:0] a3_data_in;
wire [`DWIDTH-1:0] a4_data_in;
wire [`DWIDTH-1:0] a5_data_in;
wire [`DWIDTH-1:0] a6_data_in;
wire [`DWIDTH-1:0] a7_data_in;
wire [`DWIDTH-1:0] a8_data_in;
wire [`DWIDTH-1:0] a9_data_in;
wire [`DWIDTH-1:0] a10_data_in;
wire [`DWIDTH-1:0] a11_data_in;
wire [`DWIDTH-1:0] a12_data_in;
wire [`DWIDTH-1:0] a13_data_in;
wire [`DWIDTH-1:0] a14_data_in;
wire [`DWIDTH-1:0] a15_data_in;
wire [`DWIDTH-1:0] a16_data_in;
wire [`DWIDTH-1:0] a17_data_in;
wire [`DWIDTH-1:0] a18_data_in;
wire [`DWIDTH-1:0] a19_data_in;

assign a0_data_in = a_data_in[1*`DWIDTH-1:0*`DWIDTH];
assign a1_data_in = a_data_in[2*`DWIDTH-1:1*`DWIDTH];
assign a2_data_in = a_data_in[3*`DWIDTH-1:2*`DWIDTH];
assign a3_data_in = a_data_in[4*`DWIDTH-1:3*`DWIDTH];
assign a4_data_in = a_data_in[5*`DWIDTH-1:4*`DWIDTH];
assign a5_data_in = a_data_in[6*`DWIDTH-1:5*`DWIDTH];
assign a6_data_in = a_data_in[7*`DWIDTH-1:6*`DWIDTH];
assign a7_data_in = a_data_in[8*`DWIDTH-1:7*`DWIDTH];
assign a8_data_in = a_data_in[9*`DWIDTH-1:8*`DWIDTH];
assign a9_data_in = a_data_in[10*`DWIDTH-1:9*`DWIDTH];
assign a10_data_in = a_data_in[11*`DWIDTH-1:10*`DWIDTH];
assign a11_data_in = a_data_in[12*`DWIDTH-1:11*`DWIDTH];
assign a12_data_in = a_data_in[13*`DWIDTH-1:12*`DWIDTH];
assign a13_data_in = a_data_in[14*`DWIDTH-1:13*`DWIDTH];
assign a14_data_in = a_data_in[15*`DWIDTH-1:14*`DWIDTH];
assign a15_data_in = a_data_in[16*`DWIDTH-1:15*`DWIDTH];
assign a16_data_in = a_data_in[17*`DWIDTH-1:16*`DWIDTH];
assign a17_data_in = a_data_in[18*`DWIDTH-1:17*`DWIDTH];
assign a18_data_in = a_data_in[19*`DWIDTH-1:18*`DWIDTH];
assign a19_data_in = a_data_in[20*`DWIDTH-1:19*`DWIDTH];

wire [`DWIDTH-1:0] b0_data_in;
wire [`DWIDTH-1:0] b1_data_in;
wire [`DWIDTH-1:0] b2_data_in;
wire [`DWIDTH-1:0] b3_data_in;
wire [`DWIDTH-1:0] b4_data_in;
wire [`DWIDTH-1:0] b5_data_in;
wire [`DWIDTH-1:0] b6_data_in;
wire [`DWIDTH-1:0] b7_data_in;
wire [`DWIDTH-1:0] b8_data_in;
wire [`DWIDTH-1:0] b9_data_in;
wire [`DWIDTH-1:0] b10_data_in;
wire [`DWIDTH-1:0] b11_data_in;
wire [`DWIDTH-1:0] b12_data_in;
wire [`DWIDTH-1:0] b13_data_in;
wire [`DWIDTH-1:0] b14_data_in;
wire [`DWIDTH-1:0] b15_data_in;
wire [`DWIDTH-1:0] b16_data_in;
wire [`DWIDTH-1:0] b17_data_in;
wire [`DWIDTH-1:0] b18_data_in;
wire [`DWIDTH-1:0] b19_data_in;

assign b0_data_in = b_data_in[1*`DWIDTH-1:0*`DWIDTH];
assign b1_data_in = b_data_in[2*`DWIDTH-1:1*`DWIDTH];
assign b2_data_in = b_data_in[3*`DWIDTH-1:2*`DWIDTH];
assign b3_data_in = b_data_in[4*`DWIDTH-1:3*`DWIDTH];
assign b4_data_in = b_data_in[5*`DWIDTH-1:4*`DWIDTH];
assign b5_data_in = b_data_in[6*`DWIDTH-1:5*`DWIDTH];
assign b6_data_in = b_data_in[7*`DWIDTH-1:6*`DWIDTH];
assign b7_data_in = b_data_in[8*`DWIDTH-1:7*`DWIDTH];
assign b8_data_in = b_data_in[9*`DWIDTH-1:8*`DWIDTH];
assign b9_data_in = b_data_in[10*`DWIDTH-1:9*`DWIDTH];
assign b10_data_in = b_data_in[11*`DWIDTH-1:10*`DWIDTH];
assign b11_data_in = b_data_in[12*`DWIDTH-1:11*`DWIDTH];
assign b12_data_in = b_data_in[13*`DWIDTH-1:12*`DWIDTH];
assign b13_data_in = b_data_in[14*`DWIDTH-1:13*`DWIDTH];
assign b14_data_in = b_data_in[15*`DWIDTH-1:14*`DWIDTH];
assign b15_data_in = b_data_in[16*`DWIDTH-1:15*`DWIDTH];
assign b16_data_in = b_data_in[17*`DWIDTH-1:16*`DWIDTH];
assign b17_data_in = b_data_in[18*`DWIDTH-1:17*`DWIDTH];
assign b18_data_in = b_data_in[19*`DWIDTH-1:18*`DWIDTH];
assign b19_data_in = b_data_in[20*`DWIDTH-1:19*`DWIDTH];

assign a0 = (b_loc==0) ? a0_data           : a0_data_in;
assign a1 = (b_loc==0) ? a1_data_delayed_1 : a1_data_in;
assign a2 = (b_loc==0) ? a2_data_delayed_2 : a2_data_in;
assign a3 = (b_loc==0) ? a3_data_delayed_3 : a3_data_in;
assign a4 = (b_loc==0) ? a4_data_delayed_4 : a4_data_in;
assign a5 = (b_loc==0) ? a5_data_delayed_5 : a5_data_in;
assign a6 = (b_loc==0) ? a6_data_delayed_6 : a6_data_in;
assign a7 = (b_loc==0) ? a7_data_delayed_7 : a7_data_in;
assign a8 = (b_loc==0) ? a8_data_delayed_8 : a8_data_in;
assign a9 = (b_loc==0) ? a9_data_delayed_9 : a9_data_in;
assign a10 = (b_loc==0) ? a10_data_delayed_10 : a10_data_in;
assign a11 = (b_loc==0) ? a11_data_delayed_11 : a11_data_in;
assign a12 = (b_loc==0) ? a12_data_delayed_12 : a12_data_in;
assign a13 = (b_loc==0) ? a13_data_delayed_13 : a13_data_in;
assign a14 = (b_loc==0) ? a14_data_delayed_14 : a14_data_in;
assign a15 = (b_loc==0) ? a15_data_delayed_15 : a15_data_in;
assign a16 = (b_loc==0) ? a16_data_delayed_16 : a16_data_in;
assign a17 = (b_loc==0) ? a17_data_delayed_17 : a17_data_in;
assign a18 = (b_loc==0) ? a18_data_delayed_18 : a18_data_in;
assign a19 = (b_loc==0) ? a19_data_delayed_19 : a19_data_in;

assign b0 = (a_loc==0) ? b0_data           : b0_data_in;
assign b1 = (a_loc==0) ? b1_data_delayed_1 : b1_data_in;
assign b2 = (a_loc==0) ? b2_data_delayed_2 : b2_data_in;
assign b3 = (a_loc==0) ? b3_data_delayed_3 : b3_data_in;
assign b4 = (a_loc==0) ? b4_data_delayed_4 : b4_data_in;
assign b5 = (a_loc==0) ? b5_data_delayed_5 : b5_data_in;
assign b6 = (a_loc==0) ? b6_data_delayed_6 : b6_data_in;
assign b7 = (a_loc==0) ? b7_data_delayed_7 : b7_data_in;
assign b8 = (a_loc==0) ? b8_data_delayed_8 : b8_data_in;
assign b9 = (a_loc==0) ? b9_data_delayed_9 : b9_data_in;
assign b10 = (a_loc==0) ? b10_data_delayed_10 : b10_data_in;
assign b11 = (a_loc==0) ? b11_data_delayed_11 : b11_data_in;
assign b12 = (a_loc==0) ? b12_data_delayed_12 : b12_data_in;
assign b13 = (a_loc==0) ? b13_data_delayed_13 : b13_data_in;
assign b14 = (a_loc==0) ? b14_data_delayed_14 : b14_data_in;
assign b15 = (a_loc==0) ? b15_data_delayed_15 : b15_data_in;
assign b16 = (a_loc==0) ? b16_data_delayed_16 : b16_data_in;
assign b17 = (a_loc==0) ? b17_data_delayed_17 : b17_data_in;
assign b18 = (a_loc==0) ? b18_data_delayed_18 : b18_data_in;
assign b19 = (a_loc==0) ? b19_data_delayed_19 : b19_data_in;

wire [`DWIDTH-1:0] matrixC0_0;
wire [`DWIDTH-1:0] matrixC0_1;
wire [`DWIDTH-1:0] matrixC0_2;
wire [`DWIDTH-1:0] matrixC0_3;
wire [`DWIDTH-1:0] matrixC0_4;
wire [`DWIDTH-1:0] matrixC0_5;
wire [`DWIDTH-1:0] matrixC0_6;
wire [`DWIDTH-1:0] matrixC0_7;
wire [`DWIDTH-1:0] matrixC0_8;
wire [`DWIDTH-1:0] matrixC0_9;
wire [`DWIDTH-1:0] matrixC0_10;
wire [`DWIDTH-1:0] matrixC0_11;
wire [`DWIDTH-1:0] matrixC0_12;
wire [`DWIDTH-1:0] matrixC0_13;
wire [`DWIDTH-1:0] matrixC0_14;
wire [`DWIDTH-1:0] matrixC0_15;
wire [`DWIDTH-1:0] matrixC0_16;
wire [`DWIDTH-1:0] matrixC0_17;
wire [`DWIDTH-1:0] matrixC0_18;
wire [`DWIDTH-1:0] matrixC0_19;
wire [`DWIDTH-1:0] matrixC1_0;
wire [`DWIDTH-1:0] matrixC1_1;
wire [`DWIDTH-1:0] matrixC1_2;
wire [`DWIDTH-1:0] matrixC1_3;
wire [`DWIDTH-1:0] matrixC1_4;
wire [`DWIDTH-1:0] matrixC1_5;
wire [`DWIDTH-1:0] matrixC1_6;
wire [`DWIDTH-1:0] matrixC1_7;
wire [`DWIDTH-1:0] matrixC1_8;
wire [`DWIDTH-1:0] matrixC1_9;
wire [`DWIDTH-1:0] matrixC1_10;
wire [`DWIDTH-1:0] matrixC1_11;
wire [`DWIDTH-1:0] matrixC1_12;
wire [`DWIDTH-1:0] matrixC1_13;
wire [`DWIDTH-1:0] matrixC1_14;
wire [`DWIDTH-1:0] matrixC1_15;
wire [`DWIDTH-1:0] matrixC1_16;
wire [`DWIDTH-1:0] matrixC1_17;
wire [`DWIDTH-1:0] matrixC1_18;
wire [`DWIDTH-1:0] matrixC1_19;
wire [`DWIDTH-1:0] matrixC2_0;
wire [`DWIDTH-1:0] matrixC2_1;
wire [`DWIDTH-1:0] matrixC2_2;
wire [`DWIDTH-1:0] matrixC2_3;
wire [`DWIDTH-1:0] matrixC2_4;
wire [`DWIDTH-1:0] matrixC2_5;
wire [`DWIDTH-1:0] matrixC2_6;
wire [`DWIDTH-1:0] matrixC2_7;
wire [`DWIDTH-1:0] matrixC2_8;
wire [`DWIDTH-1:0] matrixC2_9;
wire [`DWIDTH-1:0] matrixC2_10;
wire [`DWIDTH-1:0] matrixC2_11;
wire [`DWIDTH-1:0] matrixC2_12;
wire [`DWIDTH-1:0] matrixC2_13;
wire [`DWIDTH-1:0] matrixC2_14;
wire [`DWIDTH-1:0] matrixC2_15;
wire [`DWIDTH-1:0] matrixC2_16;
wire [`DWIDTH-1:0] matrixC2_17;
wire [`DWIDTH-1:0] matrixC2_18;
wire [`DWIDTH-1:0] matrixC2_19;
wire [`DWIDTH-1:0] matrixC3_0;
wire [`DWIDTH-1:0] matrixC3_1;
wire [`DWIDTH-1:0] matrixC3_2;
wire [`DWIDTH-1:0] matrixC3_3;
wire [`DWIDTH-1:0] matrixC3_4;
wire [`DWIDTH-1:0] matrixC3_5;
wire [`DWIDTH-1:0] matrixC3_6;
wire [`DWIDTH-1:0] matrixC3_7;
wire [`DWIDTH-1:0] matrixC3_8;
wire [`DWIDTH-1:0] matrixC3_9;
wire [`DWIDTH-1:0] matrixC3_10;
wire [`DWIDTH-1:0] matrixC3_11;
wire [`DWIDTH-1:0] matrixC3_12;
wire [`DWIDTH-1:0] matrixC3_13;
wire [`DWIDTH-1:0] matrixC3_14;
wire [`DWIDTH-1:0] matrixC3_15;
wire [`DWIDTH-1:0] matrixC3_16;
wire [`DWIDTH-1:0] matrixC3_17;
wire [`DWIDTH-1:0] matrixC3_18;
wire [`DWIDTH-1:0] matrixC3_19;
wire [`DWIDTH-1:0] matrixC4_0;
wire [`DWIDTH-1:0] matrixC4_1;
wire [`DWIDTH-1:0] matrixC4_2;
wire [`DWIDTH-1:0] matrixC4_3;
wire [`DWIDTH-1:0] matrixC4_4;
wire [`DWIDTH-1:0] matrixC4_5;
wire [`DWIDTH-1:0] matrixC4_6;
wire [`DWIDTH-1:0] matrixC4_7;
wire [`DWIDTH-1:0] matrixC4_8;
wire [`DWIDTH-1:0] matrixC4_9;
wire [`DWIDTH-1:0] matrixC4_10;
wire [`DWIDTH-1:0] matrixC4_11;
wire [`DWIDTH-1:0] matrixC4_12;
wire [`DWIDTH-1:0] matrixC4_13;
wire [`DWIDTH-1:0] matrixC4_14;
wire [`DWIDTH-1:0] matrixC4_15;
wire [`DWIDTH-1:0] matrixC4_16;
wire [`DWIDTH-1:0] matrixC4_17;
wire [`DWIDTH-1:0] matrixC4_18;
wire [`DWIDTH-1:0] matrixC4_19;
wire [`DWIDTH-1:0] matrixC5_0;
wire [`DWIDTH-1:0] matrixC5_1;
wire [`DWIDTH-1:0] matrixC5_2;
wire [`DWIDTH-1:0] matrixC5_3;
wire [`DWIDTH-1:0] matrixC5_4;
wire [`DWIDTH-1:0] matrixC5_5;
wire [`DWIDTH-1:0] matrixC5_6;
wire [`DWIDTH-1:0] matrixC5_7;
wire [`DWIDTH-1:0] matrixC5_8;
wire [`DWIDTH-1:0] matrixC5_9;
wire [`DWIDTH-1:0] matrixC5_10;
wire [`DWIDTH-1:0] matrixC5_11;
wire [`DWIDTH-1:0] matrixC5_12;
wire [`DWIDTH-1:0] matrixC5_13;
wire [`DWIDTH-1:0] matrixC5_14;
wire [`DWIDTH-1:0] matrixC5_15;
wire [`DWIDTH-1:0] matrixC5_16;
wire [`DWIDTH-1:0] matrixC5_17;
wire [`DWIDTH-1:0] matrixC5_18;
wire [`DWIDTH-1:0] matrixC5_19;
wire [`DWIDTH-1:0] matrixC6_0;
wire [`DWIDTH-1:0] matrixC6_1;
wire [`DWIDTH-1:0] matrixC6_2;
wire [`DWIDTH-1:0] matrixC6_3;
wire [`DWIDTH-1:0] matrixC6_4;
wire [`DWIDTH-1:0] matrixC6_5;
wire [`DWIDTH-1:0] matrixC6_6;
wire [`DWIDTH-1:0] matrixC6_7;
wire [`DWIDTH-1:0] matrixC6_8;
wire [`DWIDTH-1:0] matrixC6_9;
wire [`DWIDTH-1:0] matrixC6_10;
wire [`DWIDTH-1:0] matrixC6_11;
wire [`DWIDTH-1:0] matrixC6_12;
wire [`DWIDTH-1:0] matrixC6_13;
wire [`DWIDTH-1:0] matrixC6_14;
wire [`DWIDTH-1:0] matrixC6_15;
wire [`DWIDTH-1:0] matrixC6_16;
wire [`DWIDTH-1:0] matrixC6_17;
wire [`DWIDTH-1:0] matrixC6_18;
wire [`DWIDTH-1:0] matrixC6_19;
wire [`DWIDTH-1:0] matrixC7_0;
wire [`DWIDTH-1:0] matrixC7_1;
wire [`DWIDTH-1:0] matrixC7_2;
wire [`DWIDTH-1:0] matrixC7_3;
wire [`DWIDTH-1:0] matrixC7_4;
wire [`DWIDTH-1:0] matrixC7_5;
wire [`DWIDTH-1:0] matrixC7_6;
wire [`DWIDTH-1:0] matrixC7_7;
wire [`DWIDTH-1:0] matrixC7_8;
wire [`DWIDTH-1:0] matrixC7_9;
wire [`DWIDTH-1:0] matrixC7_10;
wire [`DWIDTH-1:0] matrixC7_11;
wire [`DWIDTH-1:0] matrixC7_12;
wire [`DWIDTH-1:0] matrixC7_13;
wire [`DWIDTH-1:0] matrixC7_14;
wire [`DWIDTH-1:0] matrixC7_15;
wire [`DWIDTH-1:0] matrixC7_16;
wire [`DWIDTH-1:0] matrixC7_17;
wire [`DWIDTH-1:0] matrixC7_18;
wire [`DWIDTH-1:0] matrixC7_19;
wire [`DWIDTH-1:0] matrixC8_0;
wire [`DWIDTH-1:0] matrixC8_1;
wire [`DWIDTH-1:0] matrixC8_2;
wire [`DWIDTH-1:0] matrixC8_3;
wire [`DWIDTH-1:0] matrixC8_4;
wire [`DWIDTH-1:0] matrixC8_5;
wire [`DWIDTH-1:0] matrixC8_6;
wire [`DWIDTH-1:0] matrixC8_7;
wire [`DWIDTH-1:0] matrixC8_8;
wire [`DWIDTH-1:0] matrixC8_9;
wire [`DWIDTH-1:0] matrixC8_10;
wire [`DWIDTH-1:0] matrixC8_11;
wire [`DWIDTH-1:0] matrixC8_12;
wire [`DWIDTH-1:0] matrixC8_13;
wire [`DWIDTH-1:0] matrixC8_14;
wire [`DWIDTH-1:0] matrixC8_15;
wire [`DWIDTH-1:0] matrixC8_16;
wire [`DWIDTH-1:0] matrixC8_17;
wire [`DWIDTH-1:0] matrixC8_18;
wire [`DWIDTH-1:0] matrixC8_19;
wire [`DWIDTH-1:0] matrixC9_0;
wire [`DWIDTH-1:0] matrixC9_1;
wire [`DWIDTH-1:0] matrixC9_2;
wire [`DWIDTH-1:0] matrixC9_3;
wire [`DWIDTH-1:0] matrixC9_4;
wire [`DWIDTH-1:0] matrixC9_5;
wire [`DWIDTH-1:0] matrixC9_6;
wire [`DWIDTH-1:0] matrixC9_7;
wire [`DWIDTH-1:0] matrixC9_8;
wire [`DWIDTH-1:0] matrixC9_9;
wire [`DWIDTH-1:0] matrixC9_10;
wire [`DWIDTH-1:0] matrixC9_11;
wire [`DWIDTH-1:0] matrixC9_12;
wire [`DWIDTH-1:0] matrixC9_13;
wire [`DWIDTH-1:0] matrixC9_14;
wire [`DWIDTH-1:0] matrixC9_15;
wire [`DWIDTH-1:0] matrixC9_16;
wire [`DWIDTH-1:0] matrixC9_17;
wire [`DWIDTH-1:0] matrixC9_18;
wire [`DWIDTH-1:0] matrixC9_19;
wire [`DWIDTH-1:0] matrixC10_0;
wire [`DWIDTH-1:0] matrixC10_1;
wire [`DWIDTH-1:0] matrixC10_2;
wire [`DWIDTH-1:0] matrixC10_3;
wire [`DWIDTH-1:0] matrixC10_4;
wire [`DWIDTH-1:0] matrixC10_5;
wire [`DWIDTH-1:0] matrixC10_6;
wire [`DWIDTH-1:0] matrixC10_7;
wire [`DWIDTH-1:0] matrixC10_8;
wire [`DWIDTH-1:0] matrixC10_9;
wire [`DWIDTH-1:0] matrixC10_10;
wire [`DWIDTH-1:0] matrixC10_11;
wire [`DWIDTH-1:0] matrixC10_12;
wire [`DWIDTH-1:0] matrixC10_13;
wire [`DWIDTH-1:0] matrixC10_14;
wire [`DWIDTH-1:0] matrixC10_15;
wire [`DWIDTH-1:0] matrixC10_16;
wire [`DWIDTH-1:0] matrixC10_17;
wire [`DWIDTH-1:0] matrixC10_18;
wire [`DWIDTH-1:0] matrixC10_19;
wire [`DWIDTH-1:0] matrixC11_0;
wire [`DWIDTH-1:0] matrixC11_1;
wire [`DWIDTH-1:0] matrixC11_2;
wire [`DWIDTH-1:0] matrixC11_3;
wire [`DWIDTH-1:0] matrixC11_4;
wire [`DWIDTH-1:0] matrixC11_5;
wire [`DWIDTH-1:0] matrixC11_6;
wire [`DWIDTH-1:0] matrixC11_7;
wire [`DWIDTH-1:0] matrixC11_8;
wire [`DWIDTH-1:0] matrixC11_9;
wire [`DWIDTH-1:0] matrixC11_10;
wire [`DWIDTH-1:0] matrixC11_11;
wire [`DWIDTH-1:0] matrixC11_12;
wire [`DWIDTH-1:0] matrixC11_13;
wire [`DWIDTH-1:0] matrixC11_14;
wire [`DWIDTH-1:0] matrixC11_15;
wire [`DWIDTH-1:0] matrixC11_16;
wire [`DWIDTH-1:0] matrixC11_17;
wire [`DWIDTH-1:0] matrixC11_18;
wire [`DWIDTH-1:0] matrixC11_19;
wire [`DWIDTH-1:0] matrixC12_0;
wire [`DWIDTH-1:0] matrixC12_1;
wire [`DWIDTH-1:0] matrixC12_2;
wire [`DWIDTH-1:0] matrixC12_3;
wire [`DWIDTH-1:0] matrixC12_4;
wire [`DWIDTH-1:0] matrixC12_5;
wire [`DWIDTH-1:0] matrixC12_6;
wire [`DWIDTH-1:0] matrixC12_7;
wire [`DWIDTH-1:0] matrixC12_8;
wire [`DWIDTH-1:0] matrixC12_9;
wire [`DWIDTH-1:0] matrixC12_10;
wire [`DWIDTH-1:0] matrixC12_11;
wire [`DWIDTH-1:0] matrixC12_12;
wire [`DWIDTH-1:0] matrixC12_13;
wire [`DWIDTH-1:0] matrixC12_14;
wire [`DWIDTH-1:0] matrixC12_15;
wire [`DWIDTH-1:0] matrixC12_16;
wire [`DWIDTH-1:0] matrixC12_17;
wire [`DWIDTH-1:0] matrixC12_18;
wire [`DWIDTH-1:0] matrixC12_19;
wire [`DWIDTH-1:0] matrixC13_0;
wire [`DWIDTH-1:0] matrixC13_1;
wire [`DWIDTH-1:0] matrixC13_2;
wire [`DWIDTH-1:0] matrixC13_3;
wire [`DWIDTH-1:0] matrixC13_4;
wire [`DWIDTH-1:0] matrixC13_5;
wire [`DWIDTH-1:0] matrixC13_6;
wire [`DWIDTH-1:0] matrixC13_7;
wire [`DWIDTH-1:0] matrixC13_8;
wire [`DWIDTH-1:0] matrixC13_9;
wire [`DWIDTH-1:0] matrixC13_10;
wire [`DWIDTH-1:0] matrixC13_11;
wire [`DWIDTH-1:0] matrixC13_12;
wire [`DWIDTH-1:0] matrixC13_13;
wire [`DWIDTH-1:0] matrixC13_14;
wire [`DWIDTH-1:0] matrixC13_15;
wire [`DWIDTH-1:0] matrixC13_16;
wire [`DWIDTH-1:0] matrixC13_17;
wire [`DWIDTH-1:0] matrixC13_18;
wire [`DWIDTH-1:0] matrixC13_19;
wire [`DWIDTH-1:0] matrixC14_0;
wire [`DWIDTH-1:0] matrixC14_1;
wire [`DWIDTH-1:0] matrixC14_2;
wire [`DWIDTH-1:0] matrixC14_3;
wire [`DWIDTH-1:0] matrixC14_4;
wire [`DWIDTH-1:0] matrixC14_5;
wire [`DWIDTH-1:0] matrixC14_6;
wire [`DWIDTH-1:0] matrixC14_7;
wire [`DWIDTH-1:0] matrixC14_8;
wire [`DWIDTH-1:0] matrixC14_9;
wire [`DWIDTH-1:0] matrixC14_10;
wire [`DWIDTH-1:0] matrixC14_11;
wire [`DWIDTH-1:0] matrixC14_12;
wire [`DWIDTH-1:0] matrixC14_13;
wire [`DWIDTH-1:0] matrixC14_14;
wire [`DWIDTH-1:0] matrixC14_15;
wire [`DWIDTH-1:0] matrixC14_16;
wire [`DWIDTH-1:0] matrixC14_17;
wire [`DWIDTH-1:0] matrixC14_18;
wire [`DWIDTH-1:0] matrixC14_19;
wire [`DWIDTH-1:0] matrixC15_0;
wire [`DWIDTH-1:0] matrixC15_1;
wire [`DWIDTH-1:0] matrixC15_2;
wire [`DWIDTH-1:0] matrixC15_3;
wire [`DWIDTH-1:0] matrixC15_4;
wire [`DWIDTH-1:0] matrixC15_5;
wire [`DWIDTH-1:0] matrixC15_6;
wire [`DWIDTH-1:0] matrixC15_7;
wire [`DWIDTH-1:0] matrixC15_8;
wire [`DWIDTH-1:0] matrixC15_9;
wire [`DWIDTH-1:0] matrixC15_10;
wire [`DWIDTH-1:0] matrixC15_11;
wire [`DWIDTH-1:0] matrixC15_12;
wire [`DWIDTH-1:0] matrixC15_13;
wire [`DWIDTH-1:0] matrixC15_14;
wire [`DWIDTH-1:0] matrixC15_15;
wire [`DWIDTH-1:0] matrixC15_16;
wire [`DWIDTH-1:0] matrixC15_17;
wire [`DWIDTH-1:0] matrixC15_18;
wire [`DWIDTH-1:0] matrixC15_19;
wire [`DWIDTH-1:0] matrixC16_0;
wire [`DWIDTH-1:0] matrixC16_1;
wire [`DWIDTH-1:0] matrixC16_2;
wire [`DWIDTH-1:0] matrixC16_3;
wire [`DWIDTH-1:0] matrixC16_4;
wire [`DWIDTH-1:0] matrixC16_5;
wire [`DWIDTH-1:0] matrixC16_6;
wire [`DWIDTH-1:0] matrixC16_7;
wire [`DWIDTH-1:0] matrixC16_8;
wire [`DWIDTH-1:0] matrixC16_9;
wire [`DWIDTH-1:0] matrixC16_10;
wire [`DWIDTH-1:0] matrixC16_11;
wire [`DWIDTH-1:0] matrixC16_12;
wire [`DWIDTH-1:0] matrixC16_13;
wire [`DWIDTH-1:0] matrixC16_14;
wire [`DWIDTH-1:0] matrixC16_15;
wire [`DWIDTH-1:0] matrixC16_16;
wire [`DWIDTH-1:0] matrixC16_17;
wire [`DWIDTH-1:0] matrixC16_18;
wire [`DWIDTH-1:0] matrixC16_19;
wire [`DWIDTH-1:0] matrixC17_0;
wire [`DWIDTH-1:0] matrixC17_1;
wire [`DWIDTH-1:0] matrixC17_2;
wire [`DWIDTH-1:0] matrixC17_3;
wire [`DWIDTH-1:0] matrixC17_4;
wire [`DWIDTH-1:0] matrixC17_5;
wire [`DWIDTH-1:0] matrixC17_6;
wire [`DWIDTH-1:0] matrixC17_7;
wire [`DWIDTH-1:0] matrixC17_8;
wire [`DWIDTH-1:0] matrixC17_9;
wire [`DWIDTH-1:0] matrixC17_10;
wire [`DWIDTH-1:0] matrixC17_11;
wire [`DWIDTH-1:0] matrixC17_12;
wire [`DWIDTH-1:0] matrixC17_13;
wire [`DWIDTH-1:0] matrixC17_14;
wire [`DWIDTH-1:0] matrixC17_15;
wire [`DWIDTH-1:0] matrixC17_16;
wire [`DWIDTH-1:0] matrixC17_17;
wire [`DWIDTH-1:0] matrixC17_18;
wire [`DWIDTH-1:0] matrixC17_19;
wire [`DWIDTH-1:0] matrixC18_0;
wire [`DWIDTH-1:0] matrixC18_1;
wire [`DWIDTH-1:0] matrixC18_2;
wire [`DWIDTH-1:0] matrixC18_3;
wire [`DWIDTH-1:0] matrixC18_4;
wire [`DWIDTH-1:0] matrixC18_5;
wire [`DWIDTH-1:0] matrixC18_6;
wire [`DWIDTH-1:0] matrixC18_7;
wire [`DWIDTH-1:0] matrixC18_8;
wire [`DWIDTH-1:0] matrixC18_9;
wire [`DWIDTH-1:0] matrixC18_10;
wire [`DWIDTH-1:0] matrixC18_11;
wire [`DWIDTH-1:0] matrixC18_12;
wire [`DWIDTH-1:0] matrixC18_13;
wire [`DWIDTH-1:0] matrixC18_14;
wire [`DWIDTH-1:0] matrixC18_15;
wire [`DWIDTH-1:0] matrixC18_16;
wire [`DWIDTH-1:0] matrixC18_17;
wire [`DWIDTH-1:0] matrixC18_18;
wire [`DWIDTH-1:0] matrixC18_19;
wire [`DWIDTH-1:0] matrixC19_0;
wire [`DWIDTH-1:0] matrixC19_1;
wire [`DWIDTH-1:0] matrixC19_2;
wire [`DWIDTH-1:0] matrixC19_3;
wire [`DWIDTH-1:0] matrixC19_4;
wire [`DWIDTH-1:0] matrixC19_5;
wire [`DWIDTH-1:0] matrixC19_6;
wire [`DWIDTH-1:0] matrixC19_7;
wire [`DWIDTH-1:0] matrixC19_8;
wire [`DWIDTH-1:0] matrixC19_9;
wire [`DWIDTH-1:0] matrixC19_10;
wire [`DWIDTH-1:0] matrixC19_11;
wire [`DWIDTH-1:0] matrixC19_12;
wire [`DWIDTH-1:0] matrixC19_13;
wire [`DWIDTH-1:0] matrixC19_14;
wire [`DWIDTH-1:0] matrixC19_15;
wire [`DWIDTH-1:0] matrixC19_16;
wire [`DWIDTH-1:0] matrixC19_17;
wire [`DWIDTH-1:0] matrixC19_18;
wire [`DWIDTH-1:0] matrixC19_19;

wire row_latch_en;
//////////////////////////////////////////////////////////////////////////
// Instantiation of the output logic
//////////////////////////////////////////////////////////////////////////
output_logic u_output_logic(
.start_mat_mul(start_mat_mul),
.done_mat_mul(done_mat_mul),
.address_mat_c(address_mat_c),
.address_stride_c(address_stride_c),
.c_data_out(c_data_out),
.c_data_in(c_data_in),
.c_addr(c_addr),
.c_data_available(c_data_available),
.clk_cnt(clk_cnt),
.row_latch_en(row_latch_en),
.matrixC0_0(matrixC0_0),
.matrixC0_1(matrixC0_1),
.matrixC0_2(matrixC0_2),
.matrixC0_3(matrixC0_3),
.matrixC0_4(matrixC0_4),
.matrixC0_5(matrixC0_5),
.matrixC0_6(matrixC0_6),
.matrixC0_7(matrixC0_7),
.matrixC0_8(matrixC0_8),
.matrixC0_9(matrixC0_9),
.matrixC0_10(matrixC0_10),
.matrixC0_11(matrixC0_11),
.matrixC0_12(matrixC0_12),
.matrixC0_13(matrixC0_13),
.matrixC0_14(matrixC0_14),
.matrixC0_15(matrixC0_15),
.matrixC0_16(matrixC0_16),
.matrixC0_17(matrixC0_17),
.matrixC0_18(matrixC0_18),
.matrixC0_19(matrixC0_19),
.matrixC1_0(matrixC1_0),
.matrixC1_1(matrixC1_1),
.matrixC1_2(matrixC1_2),
.matrixC1_3(matrixC1_3),
.matrixC1_4(matrixC1_4),
.matrixC1_5(matrixC1_5),
.matrixC1_6(matrixC1_6),
.matrixC1_7(matrixC1_7),
.matrixC1_8(matrixC1_8),
.matrixC1_9(matrixC1_9),
.matrixC1_10(matrixC1_10),
.matrixC1_11(matrixC1_11),
.matrixC1_12(matrixC1_12),
.matrixC1_13(matrixC1_13),
.matrixC1_14(matrixC1_14),
.matrixC1_15(matrixC1_15),
.matrixC1_16(matrixC1_16),
.matrixC1_17(matrixC1_17),
.matrixC1_18(matrixC1_18),
.matrixC1_19(matrixC1_19),
.matrixC2_0(matrixC2_0),
.matrixC2_1(matrixC2_1),
.matrixC2_2(matrixC2_2),
.matrixC2_3(matrixC2_3),
.matrixC2_4(matrixC2_4),
.matrixC2_5(matrixC2_5),
.matrixC2_6(matrixC2_6),
.matrixC2_7(matrixC2_7),
.matrixC2_8(matrixC2_8),
.matrixC2_9(matrixC2_9),
.matrixC2_10(matrixC2_10),
.matrixC2_11(matrixC2_11),
.matrixC2_12(matrixC2_12),
.matrixC2_13(matrixC2_13),
.matrixC2_14(matrixC2_14),
.matrixC2_15(matrixC2_15),
.matrixC2_16(matrixC2_16),
.matrixC2_17(matrixC2_17),
.matrixC2_18(matrixC2_18),
.matrixC2_19(matrixC2_19),
.matrixC3_0(matrixC3_0),
.matrixC3_1(matrixC3_1),
.matrixC3_2(matrixC3_2),
.matrixC3_3(matrixC3_3),
.matrixC3_4(matrixC3_4),
.matrixC3_5(matrixC3_5),
.matrixC3_6(matrixC3_6),
.matrixC3_7(matrixC3_7),
.matrixC3_8(matrixC3_8),
.matrixC3_9(matrixC3_9),
.matrixC3_10(matrixC3_10),
.matrixC3_11(matrixC3_11),
.matrixC3_12(matrixC3_12),
.matrixC3_13(matrixC3_13),
.matrixC3_14(matrixC3_14),
.matrixC3_15(matrixC3_15),
.matrixC3_16(matrixC3_16),
.matrixC3_17(matrixC3_17),
.matrixC3_18(matrixC3_18),
.matrixC3_19(matrixC3_19),
.matrixC4_0(matrixC4_0),
.matrixC4_1(matrixC4_1),
.matrixC4_2(matrixC4_2),
.matrixC4_3(matrixC4_3),
.matrixC4_4(matrixC4_4),
.matrixC4_5(matrixC4_5),
.matrixC4_6(matrixC4_6),
.matrixC4_7(matrixC4_7),
.matrixC4_8(matrixC4_8),
.matrixC4_9(matrixC4_9),
.matrixC4_10(matrixC4_10),
.matrixC4_11(matrixC4_11),
.matrixC4_12(matrixC4_12),
.matrixC4_13(matrixC4_13),
.matrixC4_14(matrixC4_14),
.matrixC4_15(matrixC4_15),
.matrixC4_16(matrixC4_16),
.matrixC4_17(matrixC4_17),
.matrixC4_18(matrixC4_18),
.matrixC4_19(matrixC4_19),
.matrixC5_0(matrixC5_0),
.matrixC5_1(matrixC5_1),
.matrixC5_2(matrixC5_2),
.matrixC5_3(matrixC5_3),
.matrixC5_4(matrixC5_4),
.matrixC5_5(matrixC5_5),
.matrixC5_6(matrixC5_6),
.matrixC5_7(matrixC5_7),
.matrixC5_8(matrixC5_8),
.matrixC5_9(matrixC5_9),
.matrixC5_10(matrixC5_10),
.matrixC5_11(matrixC5_11),
.matrixC5_12(matrixC5_12),
.matrixC5_13(matrixC5_13),
.matrixC5_14(matrixC5_14),
.matrixC5_15(matrixC5_15),
.matrixC5_16(matrixC5_16),
.matrixC5_17(matrixC5_17),
.matrixC5_18(matrixC5_18),
.matrixC5_19(matrixC5_19),
.matrixC6_0(matrixC6_0),
.matrixC6_1(matrixC6_1),
.matrixC6_2(matrixC6_2),
.matrixC6_3(matrixC6_3),
.matrixC6_4(matrixC6_4),
.matrixC6_5(matrixC6_5),
.matrixC6_6(matrixC6_6),
.matrixC6_7(matrixC6_7),
.matrixC6_8(matrixC6_8),
.matrixC6_9(matrixC6_9),
.matrixC6_10(matrixC6_10),
.matrixC6_11(matrixC6_11),
.matrixC6_12(matrixC6_12),
.matrixC6_13(matrixC6_13),
.matrixC6_14(matrixC6_14),
.matrixC6_15(matrixC6_15),
.matrixC6_16(matrixC6_16),
.matrixC6_17(matrixC6_17),
.matrixC6_18(matrixC6_18),
.matrixC6_19(matrixC6_19),
.matrixC7_0(matrixC7_0),
.matrixC7_1(matrixC7_1),
.matrixC7_2(matrixC7_2),
.matrixC7_3(matrixC7_3),
.matrixC7_4(matrixC7_4),
.matrixC7_5(matrixC7_5),
.matrixC7_6(matrixC7_6),
.matrixC7_7(matrixC7_7),
.matrixC7_8(matrixC7_8),
.matrixC7_9(matrixC7_9),
.matrixC7_10(matrixC7_10),
.matrixC7_11(matrixC7_11),
.matrixC7_12(matrixC7_12),
.matrixC7_13(matrixC7_13),
.matrixC7_14(matrixC7_14),
.matrixC7_15(matrixC7_15),
.matrixC7_16(matrixC7_16),
.matrixC7_17(matrixC7_17),
.matrixC7_18(matrixC7_18),
.matrixC7_19(matrixC7_19),
.matrixC8_0(matrixC8_0),
.matrixC8_1(matrixC8_1),
.matrixC8_2(matrixC8_2),
.matrixC8_3(matrixC8_3),
.matrixC8_4(matrixC8_4),
.matrixC8_5(matrixC8_5),
.matrixC8_6(matrixC8_6),
.matrixC8_7(matrixC8_7),
.matrixC8_8(matrixC8_8),
.matrixC8_9(matrixC8_9),
.matrixC8_10(matrixC8_10),
.matrixC8_11(matrixC8_11),
.matrixC8_12(matrixC8_12),
.matrixC8_13(matrixC8_13),
.matrixC8_14(matrixC8_14),
.matrixC8_15(matrixC8_15),
.matrixC8_16(matrixC8_16),
.matrixC8_17(matrixC8_17),
.matrixC8_18(matrixC8_18),
.matrixC8_19(matrixC8_19),
.matrixC9_0(matrixC9_0),
.matrixC9_1(matrixC9_1),
.matrixC9_2(matrixC9_2),
.matrixC9_3(matrixC9_3),
.matrixC9_4(matrixC9_4),
.matrixC9_5(matrixC9_5),
.matrixC9_6(matrixC9_6),
.matrixC9_7(matrixC9_7),
.matrixC9_8(matrixC9_8),
.matrixC9_9(matrixC9_9),
.matrixC9_10(matrixC9_10),
.matrixC9_11(matrixC9_11),
.matrixC9_12(matrixC9_12),
.matrixC9_13(matrixC9_13),
.matrixC9_14(matrixC9_14),
.matrixC9_15(matrixC9_15),
.matrixC9_16(matrixC9_16),
.matrixC9_17(matrixC9_17),
.matrixC9_18(matrixC9_18),
.matrixC9_19(matrixC9_19),
.matrixC10_0(matrixC10_0),
.matrixC10_1(matrixC10_1),
.matrixC10_2(matrixC10_2),
.matrixC10_3(matrixC10_3),
.matrixC10_4(matrixC10_4),
.matrixC10_5(matrixC10_5),
.matrixC10_6(matrixC10_6),
.matrixC10_7(matrixC10_7),
.matrixC10_8(matrixC10_8),
.matrixC10_9(matrixC10_9),
.matrixC10_10(matrixC10_10),
.matrixC10_11(matrixC10_11),
.matrixC10_12(matrixC10_12),
.matrixC10_13(matrixC10_13),
.matrixC10_14(matrixC10_14),
.matrixC10_15(matrixC10_15),
.matrixC10_16(matrixC10_16),
.matrixC10_17(matrixC10_17),
.matrixC10_18(matrixC10_18),
.matrixC10_19(matrixC10_19),
.matrixC11_0(matrixC11_0),
.matrixC11_1(matrixC11_1),
.matrixC11_2(matrixC11_2),
.matrixC11_3(matrixC11_3),
.matrixC11_4(matrixC11_4),
.matrixC11_5(matrixC11_5),
.matrixC11_6(matrixC11_6),
.matrixC11_7(matrixC11_7),
.matrixC11_8(matrixC11_8),
.matrixC11_9(matrixC11_9),
.matrixC11_10(matrixC11_10),
.matrixC11_11(matrixC11_11),
.matrixC11_12(matrixC11_12),
.matrixC11_13(matrixC11_13),
.matrixC11_14(matrixC11_14),
.matrixC11_15(matrixC11_15),
.matrixC11_16(matrixC11_16),
.matrixC11_17(matrixC11_17),
.matrixC11_18(matrixC11_18),
.matrixC11_19(matrixC11_19),
.matrixC12_0(matrixC12_0),
.matrixC12_1(matrixC12_1),
.matrixC12_2(matrixC12_2),
.matrixC12_3(matrixC12_3),
.matrixC12_4(matrixC12_4),
.matrixC12_5(matrixC12_5),
.matrixC12_6(matrixC12_6),
.matrixC12_7(matrixC12_7),
.matrixC12_8(matrixC12_8),
.matrixC12_9(matrixC12_9),
.matrixC12_10(matrixC12_10),
.matrixC12_11(matrixC12_11),
.matrixC12_12(matrixC12_12),
.matrixC12_13(matrixC12_13),
.matrixC12_14(matrixC12_14),
.matrixC12_15(matrixC12_15),
.matrixC12_16(matrixC12_16),
.matrixC12_17(matrixC12_17),
.matrixC12_18(matrixC12_18),
.matrixC12_19(matrixC12_19),
.matrixC13_0(matrixC13_0),
.matrixC13_1(matrixC13_1),
.matrixC13_2(matrixC13_2),
.matrixC13_3(matrixC13_3),
.matrixC13_4(matrixC13_4),
.matrixC13_5(matrixC13_5),
.matrixC13_6(matrixC13_6),
.matrixC13_7(matrixC13_7),
.matrixC13_8(matrixC13_8),
.matrixC13_9(matrixC13_9),
.matrixC13_10(matrixC13_10),
.matrixC13_11(matrixC13_11),
.matrixC13_12(matrixC13_12),
.matrixC13_13(matrixC13_13),
.matrixC13_14(matrixC13_14),
.matrixC13_15(matrixC13_15),
.matrixC13_16(matrixC13_16),
.matrixC13_17(matrixC13_17),
.matrixC13_18(matrixC13_18),
.matrixC13_19(matrixC13_19),
.matrixC14_0(matrixC14_0),
.matrixC14_1(matrixC14_1),
.matrixC14_2(matrixC14_2),
.matrixC14_3(matrixC14_3),
.matrixC14_4(matrixC14_4),
.matrixC14_5(matrixC14_5),
.matrixC14_6(matrixC14_6),
.matrixC14_7(matrixC14_7),
.matrixC14_8(matrixC14_8),
.matrixC14_9(matrixC14_9),
.matrixC14_10(matrixC14_10),
.matrixC14_11(matrixC14_11),
.matrixC14_12(matrixC14_12),
.matrixC14_13(matrixC14_13),
.matrixC14_14(matrixC14_14),
.matrixC14_15(matrixC14_15),
.matrixC14_16(matrixC14_16),
.matrixC14_17(matrixC14_17),
.matrixC14_18(matrixC14_18),
.matrixC14_19(matrixC14_19),
.matrixC15_0(matrixC15_0),
.matrixC15_1(matrixC15_1),
.matrixC15_2(matrixC15_2),
.matrixC15_3(matrixC15_3),
.matrixC15_4(matrixC15_4),
.matrixC15_5(matrixC15_5),
.matrixC15_6(matrixC15_6),
.matrixC15_7(matrixC15_7),
.matrixC15_8(matrixC15_8),
.matrixC15_9(matrixC15_9),
.matrixC15_10(matrixC15_10),
.matrixC15_11(matrixC15_11),
.matrixC15_12(matrixC15_12),
.matrixC15_13(matrixC15_13),
.matrixC15_14(matrixC15_14),
.matrixC15_15(matrixC15_15),
.matrixC15_16(matrixC15_16),
.matrixC15_17(matrixC15_17),
.matrixC15_18(matrixC15_18),
.matrixC15_19(matrixC15_19),
.matrixC16_0(matrixC16_0),
.matrixC16_1(matrixC16_1),
.matrixC16_2(matrixC16_2),
.matrixC16_3(matrixC16_3),
.matrixC16_4(matrixC16_4),
.matrixC16_5(matrixC16_5),
.matrixC16_6(matrixC16_6),
.matrixC16_7(matrixC16_7),
.matrixC16_8(matrixC16_8),
.matrixC16_9(matrixC16_9),
.matrixC16_10(matrixC16_10),
.matrixC16_11(matrixC16_11),
.matrixC16_12(matrixC16_12),
.matrixC16_13(matrixC16_13),
.matrixC16_14(matrixC16_14),
.matrixC16_15(matrixC16_15),
.matrixC16_16(matrixC16_16),
.matrixC16_17(matrixC16_17),
.matrixC16_18(matrixC16_18),
.matrixC16_19(matrixC16_19),
.matrixC17_0(matrixC17_0),
.matrixC17_1(matrixC17_1),
.matrixC17_2(matrixC17_2),
.matrixC17_3(matrixC17_3),
.matrixC17_4(matrixC17_4),
.matrixC17_5(matrixC17_5),
.matrixC17_6(matrixC17_6),
.matrixC17_7(matrixC17_7),
.matrixC17_8(matrixC17_8),
.matrixC17_9(matrixC17_9),
.matrixC17_10(matrixC17_10),
.matrixC17_11(matrixC17_11),
.matrixC17_12(matrixC17_12),
.matrixC17_13(matrixC17_13),
.matrixC17_14(matrixC17_14),
.matrixC17_15(matrixC17_15),
.matrixC17_16(matrixC17_16),
.matrixC17_17(matrixC17_17),
.matrixC17_18(matrixC17_18),
.matrixC17_19(matrixC17_19),
.matrixC18_0(matrixC18_0),
.matrixC18_1(matrixC18_1),
.matrixC18_2(matrixC18_2),
.matrixC18_3(matrixC18_3),
.matrixC18_4(matrixC18_4),
.matrixC18_5(matrixC18_5),
.matrixC18_6(matrixC18_6),
.matrixC18_7(matrixC18_7),
.matrixC18_8(matrixC18_8),
.matrixC18_9(matrixC18_9),
.matrixC18_10(matrixC18_10),
.matrixC18_11(matrixC18_11),
.matrixC18_12(matrixC18_12),
.matrixC18_13(matrixC18_13),
.matrixC18_14(matrixC18_14),
.matrixC18_15(matrixC18_15),
.matrixC18_16(matrixC18_16),
.matrixC18_17(matrixC18_17),
.matrixC18_18(matrixC18_18),
.matrixC18_19(matrixC18_19),
.matrixC19_0(matrixC19_0),
.matrixC19_1(matrixC19_1),
.matrixC19_2(matrixC19_2),
.matrixC19_3(matrixC19_3),
.matrixC19_4(matrixC19_4),
.matrixC19_5(matrixC19_5),
.matrixC19_6(matrixC19_6),
.matrixC19_7(matrixC19_7),
.matrixC19_8(matrixC19_8),
.matrixC19_9(matrixC19_9),
.matrixC19_10(matrixC19_10),
.matrixC19_11(matrixC19_11),
.matrixC19_12(matrixC19_12),
.matrixC19_13(matrixC19_13),
.matrixC19_14(matrixC19_14),
.matrixC19_15(matrixC19_15),
.matrixC19_16(matrixC19_16),
.matrixC19_17(matrixC19_17),
.matrixC19_18(matrixC19_18),
.matrixC19_19(matrixC19_19),

.clk(clk),
.reset(reset)
);

//////////////////////////////////////////////////////////////////////////
// Instantiations of the actual PEs
//////////////////////////////////////////////////////////////////////////
systolic_pe_matrix u_systolic_pe_matrix(
.clk(clk),
.reset(reset),
.pe_reset(pe_reset),
.a0(a0),
.a1(a1),
.a2(a2),
.a3(a3),
.a4(a4),
.a5(a5),
.a6(a6),
.a7(a7),
.a8(a8),
.a9(a9),
.a10(a10),
.a11(a11),
.a12(a12),
.a13(a13),
.a14(a14),
.a15(a15),
.a16(a16),
.a17(a17),
.a18(a18),
.a19(a19),
.b0(b0),
.b1(b1),
.b2(b2),
.b3(b3),
.b4(b4),
.b5(b5),
.b6(b6),
.b7(b7),
.b8(b8),
.b9(b9),
.b10(b10),
.b11(b11),
.b12(b12),
.b13(b13),
.b14(b14),
.b15(b15),
.b16(b16),
.b17(b17),
.b18(b18),
.b19(b19),
.matrixC0_0(matrixC0_0),
.matrixC0_1(matrixC0_1),
.matrixC0_2(matrixC0_2),
.matrixC0_3(matrixC0_3),
.matrixC0_4(matrixC0_4),
.matrixC0_5(matrixC0_5),
.matrixC0_6(matrixC0_6),
.matrixC0_7(matrixC0_7),
.matrixC0_8(matrixC0_8),
.matrixC0_9(matrixC0_9),
.matrixC0_10(matrixC0_10),
.matrixC0_11(matrixC0_11),
.matrixC0_12(matrixC0_12),
.matrixC0_13(matrixC0_13),
.matrixC0_14(matrixC0_14),
.matrixC0_15(matrixC0_15),
.matrixC0_16(matrixC0_16),
.matrixC0_17(matrixC0_17),
.matrixC0_18(matrixC0_18),
.matrixC0_19(matrixC0_19),
.matrixC1_0(matrixC1_0),
.matrixC1_1(matrixC1_1),
.matrixC1_2(matrixC1_2),
.matrixC1_3(matrixC1_3),
.matrixC1_4(matrixC1_4),
.matrixC1_5(matrixC1_5),
.matrixC1_6(matrixC1_6),
.matrixC1_7(matrixC1_7),
.matrixC1_8(matrixC1_8),
.matrixC1_9(matrixC1_9),
.matrixC1_10(matrixC1_10),
.matrixC1_11(matrixC1_11),
.matrixC1_12(matrixC1_12),
.matrixC1_13(matrixC1_13),
.matrixC1_14(matrixC1_14),
.matrixC1_15(matrixC1_15),
.matrixC1_16(matrixC1_16),
.matrixC1_17(matrixC1_17),
.matrixC1_18(matrixC1_18),
.matrixC1_19(matrixC1_19),
.matrixC2_0(matrixC2_0),
.matrixC2_1(matrixC2_1),
.matrixC2_2(matrixC2_2),
.matrixC2_3(matrixC2_3),
.matrixC2_4(matrixC2_4),
.matrixC2_5(matrixC2_5),
.matrixC2_6(matrixC2_6),
.matrixC2_7(matrixC2_7),
.matrixC2_8(matrixC2_8),
.matrixC2_9(matrixC2_9),
.matrixC2_10(matrixC2_10),
.matrixC2_11(matrixC2_11),
.matrixC2_12(matrixC2_12),
.matrixC2_13(matrixC2_13),
.matrixC2_14(matrixC2_14),
.matrixC2_15(matrixC2_15),
.matrixC2_16(matrixC2_16),
.matrixC2_17(matrixC2_17),
.matrixC2_18(matrixC2_18),
.matrixC2_19(matrixC2_19),
.matrixC3_0(matrixC3_0),
.matrixC3_1(matrixC3_1),
.matrixC3_2(matrixC3_2),
.matrixC3_3(matrixC3_3),
.matrixC3_4(matrixC3_4),
.matrixC3_5(matrixC3_5),
.matrixC3_6(matrixC3_6),
.matrixC3_7(matrixC3_7),
.matrixC3_8(matrixC3_8),
.matrixC3_9(matrixC3_9),
.matrixC3_10(matrixC3_10),
.matrixC3_11(matrixC3_11),
.matrixC3_12(matrixC3_12),
.matrixC3_13(matrixC3_13),
.matrixC3_14(matrixC3_14),
.matrixC3_15(matrixC3_15),
.matrixC3_16(matrixC3_16),
.matrixC3_17(matrixC3_17),
.matrixC3_18(matrixC3_18),
.matrixC3_19(matrixC3_19),
.matrixC4_0(matrixC4_0),
.matrixC4_1(matrixC4_1),
.matrixC4_2(matrixC4_2),
.matrixC4_3(matrixC4_3),
.matrixC4_4(matrixC4_4),
.matrixC4_5(matrixC4_5),
.matrixC4_6(matrixC4_6),
.matrixC4_7(matrixC4_7),
.matrixC4_8(matrixC4_8),
.matrixC4_9(matrixC4_9),
.matrixC4_10(matrixC4_10),
.matrixC4_11(matrixC4_11),
.matrixC4_12(matrixC4_12),
.matrixC4_13(matrixC4_13),
.matrixC4_14(matrixC4_14),
.matrixC4_15(matrixC4_15),
.matrixC4_16(matrixC4_16),
.matrixC4_17(matrixC4_17),
.matrixC4_18(matrixC4_18),
.matrixC4_19(matrixC4_19),
.matrixC5_0(matrixC5_0),
.matrixC5_1(matrixC5_1),
.matrixC5_2(matrixC5_2),
.matrixC5_3(matrixC5_3),
.matrixC5_4(matrixC5_4),
.matrixC5_5(matrixC5_5),
.matrixC5_6(matrixC5_6),
.matrixC5_7(matrixC5_7),
.matrixC5_8(matrixC5_8),
.matrixC5_9(matrixC5_9),
.matrixC5_10(matrixC5_10),
.matrixC5_11(matrixC5_11),
.matrixC5_12(matrixC5_12),
.matrixC5_13(matrixC5_13),
.matrixC5_14(matrixC5_14),
.matrixC5_15(matrixC5_15),
.matrixC5_16(matrixC5_16),
.matrixC5_17(matrixC5_17),
.matrixC5_18(matrixC5_18),
.matrixC5_19(matrixC5_19),
.matrixC6_0(matrixC6_0),
.matrixC6_1(matrixC6_1),
.matrixC6_2(matrixC6_2),
.matrixC6_3(matrixC6_3),
.matrixC6_4(matrixC6_4),
.matrixC6_5(matrixC6_5),
.matrixC6_6(matrixC6_6),
.matrixC6_7(matrixC6_7),
.matrixC6_8(matrixC6_8),
.matrixC6_9(matrixC6_9),
.matrixC6_10(matrixC6_10),
.matrixC6_11(matrixC6_11),
.matrixC6_12(matrixC6_12),
.matrixC6_13(matrixC6_13),
.matrixC6_14(matrixC6_14),
.matrixC6_15(matrixC6_15),
.matrixC6_16(matrixC6_16),
.matrixC6_17(matrixC6_17),
.matrixC6_18(matrixC6_18),
.matrixC6_19(matrixC6_19),
.matrixC7_0(matrixC7_0),
.matrixC7_1(matrixC7_1),
.matrixC7_2(matrixC7_2),
.matrixC7_3(matrixC7_3),
.matrixC7_4(matrixC7_4),
.matrixC7_5(matrixC7_5),
.matrixC7_6(matrixC7_6),
.matrixC7_7(matrixC7_7),
.matrixC7_8(matrixC7_8),
.matrixC7_9(matrixC7_9),
.matrixC7_10(matrixC7_10),
.matrixC7_11(matrixC7_11),
.matrixC7_12(matrixC7_12),
.matrixC7_13(matrixC7_13),
.matrixC7_14(matrixC7_14),
.matrixC7_15(matrixC7_15),
.matrixC7_16(matrixC7_16),
.matrixC7_17(matrixC7_17),
.matrixC7_18(matrixC7_18),
.matrixC7_19(matrixC7_19),
.matrixC8_0(matrixC8_0),
.matrixC8_1(matrixC8_1),
.matrixC8_2(matrixC8_2),
.matrixC8_3(matrixC8_3),
.matrixC8_4(matrixC8_4),
.matrixC8_5(matrixC8_5),
.matrixC8_6(matrixC8_6),
.matrixC8_7(matrixC8_7),
.matrixC8_8(matrixC8_8),
.matrixC8_9(matrixC8_9),
.matrixC8_10(matrixC8_10),
.matrixC8_11(matrixC8_11),
.matrixC8_12(matrixC8_12),
.matrixC8_13(matrixC8_13),
.matrixC8_14(matrixC8_14),
.matrixC8_15(matrixC8_15),
.matrixC8_16(matrixC8_16),
.matrixC8_17(matrixC8_17),
.matrixC8_18(matrixC8_18),
.matrixC8_19(matrixC8_19),
.matrixC9_0(matrixC9_0),
.matrixC9_1(matrixC9_1),
.matrixC9_2(matrixC9_2),
.matrixC9_3(matrixC9_3),
.matrixC9_4(matrixC9_4),
.matrixC9_5(matrixC9_5),
.matrixC9_6(matrixC9_6),
.matrixC9_7(matrixC9_7),
.matrixC9_8(matrixC9_8),
.matrixC9_9(matrixC9_9),
.matrixC9_10(matrixC9_10),
.matrixC9_11(matrixC9_11),
.matrixC9_12(matrixC9_12),
.matrixC9_13(matrixC9_13),
.matrixC9_14(matrixC9_14),
.matrixC9_15(matrixC9_15),
.matrixC9_16(matrixC9_16),
.matrixC9_17(matrixC9_17),
.matrixC9_18(matrixC9_18),
.matrixC9_19(matrixC9_19),
.matrixC10_0(matrixC10_0),
.matrixC10_1(matrixC10_1),
.matrixC10_2(matrixC10_2),
.matrixC10_3(matrixC10_3),
.matrixC10_4(matrixC10_4),
.matrixC10_5(matrixC10_5),
.matrixC10_6(matrixC10_6),
.matrixC10_7(matrixC10_7),
.matrixC10_8(matrixC10_8),
.matrixC10_9(matrixC10_9),
.matrixC10_10(matrixC10_10),
.matrixC10_11(matrixC10_11),
.matrixC10_12(matrixC10_12),
.matrixC10_13(matrixC10_13),
.matrixC10_14(matrixC10_14),
.matrixC10_15(matrixC10_15),
.matrixC10_16(matrixC10_16),
.matrixC10_17(matrixC10_17),
.matrixC10_18(matrixC10_18),
.matrixC10_19(matrixC10_19),
.matrixC11_0(matrixC11_0),
.matrixC11_1(matrixC11_1),
.matrixC11_2(matrixC11_2),
.matrixC11_3(matrixC11_3),
.matrixC11_4(matrixC11_4),
.matrixC11_5(matrixC11_5),
.matrixC11_6(matrixC11_6),
.matrixC11_7(matrixC11_7),
.matrixC11_8(matrixC11_8),
.matrixC11_9(matrixC11_9),
.matrixC11_10(matrixC11_10),
.matrixC11_11(matrixC11_11),
.matrixC11_12(matrixC11_12),
.matrixC11_13(matrixC11_13),
.matrixC11_14(matrixC11_14),
.matrixC11_15(matrixC11_15),
.matrixC11_16(matrixC11_16),
.matrixC11_17(matrixC11_17),
.matrixC11_18(matrixC11_18),
.matrixC11_19(matrixC11_19),
.matrixC12_0(matrixC12_0),
.matrixC12_1(matrixC12_1),
.matrixC12_2(matrixC12_2),
.matrixC12_3(matrixC12_3),
.matrixC12_4(matrixC12_4),
.matrixC12_5(matrixC12_5),
.matrixC12_6(matrixC12_6),
.matrixC12_7(matrixC12_7),
.matrixC12_8(matrixC12_8),
.matrixC12_9(matrixC12_9),
.matrixC12_10(matrixC12_10),
.matrixC12_11(matrixC12_11),
.matrixC12_12(matrixC12_12),
.matrixC12_13(matrixC12_13),
.matrixC12_14(matrixC12_14),
.matrixC12_15(matrixC12_15),
.matrixC12_16(matrixC12_16),
.matrixC12_17(matrixC12_17),
.matrixC12_18(matrixC12_18),
.matrixC12_19(matrixC12_19),
.matrixC13_0(matrixC13_0),
.matrixC13_1(matrixC13_1),
.matrixC13_2(matrixC13_2),
.matrixC13_3(matrixC13_3),
.matrixC13_4(matrixC13_4),
.matrixC13_5(matrixC13_5),
.matrixC13_6(matrixC13_6),
.matrixC13_7(matrixC13_7),
.matrixC13_8(matrixC13_8),
.matrixC13_9(matrixC13_9),
.matrixC13_10(matrixC13_10),
.matrixC13_11(matrixC13_11),
.matrixC13_12(matrixC13_12),
.matrixC13_13(matrixC13_13),
.matrixC13_14(matrixC13_14),
.matrixC13_15(matrixC13_15),
.matrixC13_16(matrixC13_16),
.matrixC13_17(matrixC13_17),
.matrixC13_18(matrixC13_18),
.matrixC13_19(matrixC13_19),
.matrixC14_0(matrixC14_0),
.matrixC14_1(matrixC14_1),
.matrixC14_2(matrixC14_2),
.matrixC14_3(matrixC14_3),
.matrixC14_4(matrixC14_4),
.matrixC14_5(matrixC14_5),
.matrixC14_6(matrixC14_6),
.matrixC14_7(matrixC14_7),
.matrixC14_8(matrixC14_8),
.matrixC14_9(matrixC14_9),
.matrixC14_10(matrixC14_10),
.matrixC14_11(matrixC14_11),
.matrixC14_12(matrixC14_12),
.matrixC14_13(matrixC14_13),
.matrixC14_14(matrixC14_14),
.matrixC14_15(matrixC14_15),
.matrixC14_16(matrixC14_16),
.matrixC14_17(matrixC14_17),
.matrixC14_18(matrixC14_18),
.matrixC14_19(matrixC14_19),
.matrixC15_0(matrixC15_0),
.matrixC15_1(matrixC15_1),
.matrixC15_2(matrixC15_2),
.matrixC15_3(matrixC15_3),
.matrixC15_4(matrixC15_4),
.matrixC15_5(matrixC15_5),
.matrixC15_6(matrixC15_6),
.matrixC15_7(matrixC15_7),
.matrixC15_8(matrixC15_8),
.matrixC15_9(matrixC15_9),
.matrixC15_10(matrixC15_10),
.matrixC15_11(matrixC15_11),
.matrixC15_12(matrixC15_12),
.matrixC15_13(matrixC15_13),
.matrixC15_14(matrixC15_14),
.matrixC15_15(matrixC15_15),
.matrixC15_16(matrixC15_16),
.matrixC15_17(matrixC15_17),
.matrixC15_18(matrixC15_18),
.matrixC15_19(matrixC15_19),
.matrixC16_0(matrixC16_0),
.matrixC16_1(matrixC16_1),
.matrixC16_2(matrixC16_2),
.matrixC16_3(matrixC16_3),
.matrixC16_4(matrixC16_4),
.matrixC16_5(matrixC16_5),
.matrixC16_6(matrixC16_6),
.matrixC16_7(matrixC16_7),
.matrixC16_8(matrixC16_8),
.matrixC16_9(matrixC16_9),
.matrixC16_10(matrixC16_10),
.matrixC16_11(matrixC16_11),
.matrixC16_12(matrixC16_12),
.matrixC16_13(matrixC16_13),
.matrixC16_14(matrixC16_14),
.matrixC16_15(matrixC16_15),
.matrixC16_16(matrixC16_16),
.matrixC16_17(matrixC16_17),
.matrixC16_18(matrixC16_18),
.matrixC16_19(matrixC16_19),
.matrixC17_0(matrixC17_0),
.matrixC17_1(matrixC17_1),
.matrixC17_2(matrixC17_2),
.matrixC17_3(matrixC17_3),
.matrixC17_4(matrixC17_4),
.matrixC17_5(matrixC17_5),
.matrixC17_6(matrixC17_6),
.matrixC17_7(matrixC17_7),
.matrixC17_8(matrixC17_8),
.matrixC17_9(matrixC17_9),
.matrixC17_10(matrixC17_10),
.matrixC17_11(matrixC17_11),
.matrixC17_12(matrixC17_12),
.matrixC17_13(matrixC17_13),
.matrixC17_14(matrixC17_14),
.matrixC17_15(matrixC17_15),
.matrixC17_16(matrixC17_16),
.matrixC17_17(matrixC17_17),
.matrixC17_18(matrixC17_18),
.matrixC17_19(matrixC17_19),
.matrixC18_0(matrixC18_0),
.matrixC18_1(matrixC18_1),
.matrixC18_2(matrixC18_2),
.matrixC18_3(matrixC18_3),
.matrixC18_4(matrixC18_4),
.matrixC18_5(matrixC18_5),
.matrixC18_6(matrixC18_6),
.matrixC18_7(matrixC18_7),
.matrixC18_8(matrixC18_8),
.matrixC18_9(matrixC18_9),
.matrixC18_10(matrixC18_10),
.matrixC18_11(matrixC18_11),
.matrixC18_12(matrixC18_12),
.matrixC18_13(matrixC18_13),
.matrixC18_14(matrixC18_14),
.matrixC18_15(matrixC18_15),
.matrixC18_16(matrixC18_16),
.matrixC18_17(matrixC18_17),
.matrixC18_18(matrixC18_18),
.matrixC18_19(matrixC18_19),
.matrixC19_0(matrixC19_0),
.matrixC19_1(matrixC19_1),
.matrixC19_2(matrixC19_2),
.matrixC19_3(matrixC19_3),
.matrixC19_4(matrixC19_4),
.matrixC19_5(matrixC19_5),
.matrixC19_6(matrixC19_6),
.matrixC19_7(matrixC19_7),
.matrixC19_8(matrixC19_8),
.matrixC19_9(matrixC19_9),
.matrixC19_10(matrixC19_10),
.matrixC19_11(matrixC19_11),
.matrixC19_12(matrixC19_12),
.matrixC19_13(matrixC19_13),
.matrixC19_14(matrixC19_14),
.matrixC19_15(matrixC19_15),
.matrixC19_16(matrixC19_16),
.matrixC19_17(matrixC19_17),
.matrixC19_18(matrixC19_18),
.matrixC19_19(matrixC19_19),

.a_data_out(a_data_out),
.b_data_out(b_data_out)
);

endmodule


//////////////////////////////////////////////////////////////////////////
// Output logic
//////////////////////////////////////////////////////////////////////////
module output_logic(
start_mat_mul,
done_mat_mul,
address_mat_c,
address_stride_c,
c_data_in,
c_data_out, //Data values going out to next matmul - systolic shifting
c_addr,
c_data_available,
clk_cnt,
row_latch_en,
matrixC0_0,
matrixC0_1,
matrixC0_2,
matrixC0_3,
matrixC0_4,
matrixC0_5,
matrixC0_6,
matrixC0_7,
matrixC0_8,
matrixC0_9,
matrixC0_10,
matrixC0_11,
matrixC0_12,
matrixC0_13,
matrixC0_14,
matrixC0_15,
matrixC0_16,
matrixC0_17,
matrixC0_18,
matrixC0_19,
matrixC1_0,
matrixC1_1,
matrixC1_2,
matrixC1_3,
matrixC1_4,
matrixC1_5,
matrixC1_6,
matrixC1_7,
matrixC1_8,
matrixC1_9,
matrixC1_10,
matrixC1_11,
matrixC1_12,
matrixC1_13,
matrixC1_14,
matrixC1_15,
matrixC1_16,
matrixC1_17,
matrixC1_18,
matrixC1_19,
matrixC2_0,
matrixC2_1,
matrixC2_2,
matrixC2_3,
matrixC2_4,
matrixC2_5,
matrixC2_6,
matrixC2_7,
matrixC2_8,
matrixC2_9,
matrixC2_10,
matrixC2_11,
matrixC2_12,
matrixC2_13,
matrixC2_14,
matrixC2_15,
matrixC2_16,
matrixC2_17,
matrixC2_18,
matrixC2_19,
matrixC3_0,
matrixC3_1,
matrixC3_2,
matrixC3_3,
matrixC3_4,
matrixC3_5,
matrixC3_6,
matrixC3_7,
matrixC3_8,
matrixC3_9,
matrixC3_10,
matrixC3_11,
matrixC3_12,
matrixC3_13,
matrixC3_14,
matrixC3_15,
matrixC3_16,
matrixC3_17,
matrixC3_18,
matrixC3_19,
matrixC4_0,
matrixC4_1,
matrixC4_2,
matrixC4_3,
matrixC4_4,
matrixC4_5,
matrixC4_6,
matrixC4_7,
matrixC4_8,
matrixC4_9,
matrixC4_10,
matrixC4_11,
matrixC4_12,
matrixC4_13,
matrixC4_14,
matrixC4_15,
matrixC4_16,
matrixC4_17,
matrixC4_18,
matrixC4_19,
matrixC5_0,
matrixC5_1,
matrixC5_2,
matrixC5_3,
matrixC5_4,
matrixC5_5,
matrixC5_6,
matrixC5_7,
matrixC5_8,
matrixC5_9,
matrixC5_10,
matrixC5_11,
matrixC5_12,
matrixC5_13,
matrixC5_14,
matrixC5_15,
matrixC5_16,
matrixC5_17,
matrixC5_18,
matrixC5_19,
matrixC6_0,
matrixC6_1,
matrixC6_2,
matrixC6_3,
matrixC6_4,
matrixC6_5,
matrixC6_6,
matrixC6_7,
matrixC6_8,
matrixC6_9,
matrixC6_10,
matrixC6_11,
matrixC6_12,
matrixC6_13,
matrixC6_14,
matrixC6_15,
matrixC6_16,
matrixC6_17,
matrixC6_18,
matrixC6_19,
matrixC7_0,
matrixC7_1,
matrixC7_2,
matrixC7_3,
matrixC7_4,
matrixC7_5,
matrixC7_6,
matrixC7_7,
matrixC7_8,
matrixC7_9,
matrixC7_10,
matrixC7_11,
matrixC7_12,
matrixC7_13,
matrixC7_14,
matrixC7_15,
matrixC7_16,
matrixC7_17,
matrixC7_18,
matrixC7_19,
matrixC8_0,
matrixC8_1,
matrixC8_2,
matrixC8_3,
matrixC8_4,
matrixC8_5,
matrixC8_6,
matrixC8_7,
matrixC8_8,
matrixC8_9,
matrixC8_10,
matrixC8_11,
matrixC8_12,
matrixC8_13,
matrixC8_14,
matrixC8_15,
matrixC8_16,
matrixC8_17,
matrixC8_18,
matrixC8_19,
matrixC9_0,
matrixC9_1,
matrixC9_2,
matrixC9_3,
matrixC9_4,
matrixC9_5,
matrixC9_6,
matrixC9_7,
matrixC9_8,
matrixC9_9,
matrixC9_10,
matrixC9_11,
matrixC9_12,
matrixC9_13,
matrixC9_14,
matrixC9_15,
matrixC9_16,
matrixC9_17,
matrixC9_18,
matrixC9_19,
matrixC10_0,
matrixC10_1,
matrixC10_2,
matrixC10_3,
matrixC10_4,
matrixC10_5,
matrixC10_6,
matrixC10_7,
matrixC10_8,
matrixC10_9,
matrixC10_10,
matrixC10_11,
matrixC10_12,
matrixC10_13,
matrixC10_14,
matrixC10_15,
matrixC10_16,
matrixC10_17,
matrixC10_18,
matrixC10_19,
matrixC11_0,
matrixC11_1,
matrixC11_2,
matrixC11_3,
matrixC11_4,
matrixC11_5,
matrixC11_6,
matrixC11_7,
matrixC11_8,
matrixC11_9,
matrixC11_10,
matrixC11_11,
matrixC11_12,
matrixC11_13,
matrixC11_14,
matrixC11_15,
matrixC11_16,
matrixC11_17,
matrixC11_18,
matrixC11_19,
matrixC12_0,
matrixC12_1,
matrixC12_2,
matrixC12_3,
matrixC12_4,
matrixC12_5,
matrixC12_6,
matrixC12_7,
matrixC12_8,
matrixC12_9,
matrixC12_10,
matrixC12_11,
matrixC12_12,
matrixC12_13,
matrixC12_14,
matrixC12_15,
matrixC12_16,
matrixC12_17,
matrixC12_18,
matrixC12_19,
matrixC13_0,
matrixC13_1,
matrixC13_2,
matrixC13_3,
matrixC13_4,
matrixC13_5,
matrixC13_6,
matrixC13_7,
matrixC13_8,
matrixC13_9,
matrixC13_10,
matrixC13_11,
matrixC13_12,
matrixC13_13,
matrixC13_14,
matrixC13_15,
matrixC13_16,
matrixC13_17,
matrixC13_18,
matrixC13_19,
matrixC14_0,
matrixC14_1,
matrixC14_2,
matrixC14_3,
matrixC14_4,
matrixC14_5,
matrixC14_6,
matrixC14_7,
matrixC14_8,
matrixC14_9,
matrixC14_10,
matrixC14_11,
matrixC14_12,
matrixC14_13,
matrixC14_14,
matrixC14_15,
matrixC14_16,
matrixC14_17,
matrixC14_18,
matrixC14_19,
matrixC15_0,
matrixC15_1,
matrixC15_2,
matrixC15_3,
matrixC15_4,
matrixC15_5,
matrixC15_6,
matrixC15_7,
matrixC15_8,
matrixC15_9,
matrixC15_10,
matrixC15_11,
matrixC15_12,
matrixC15_13,
matrixC15_14,
matrixC15_15,
matrixC15_16,
matrixC15_17,
matrixC15_18,
matrixC15_19,
matrixC16_0,
matrixC16_1,
matrixC16_2,
matrixC16_3,
matrixC16_4,
matrixC16_5,
matrixC16_6,
matrixC16_7,
matrixC16_8,
matrixC16_9,
matrixC16_10,
matrixC16_11,
matrixC16_12,
matrixC16_13,
matrixC16_14,
matrixC16_15,
matrixC16_16,
matrixC16_17,
matrixC16_18,
matrixC16_19,
matrixC17_0,
matrixC17_1,
matrixC17_2,
matrixC17_3,
matrixC17_4,
matrixC17_5,
matrixC17_6,
matrixC17_7,
matrixC17_8,
matrixC17_9,
matrixC17_10,
matrixC17_11,
matrixC17_12,
matrixC17_13,
matrixC17_14,
matrixC17_15,
matrixC17_16,
matrixC17_17,
matrixC17_18,
matrixC17_19,
matrixC18_0,
matrixC18_1,
matrixC18_2,
matrixC18_3,
matrixC18_4,
matrixC18_5,
matrixC18_6,
matrixC18_7,
matrixC18_8,
matrixC18_9,
matrixC18_10,
matrixC18_11,
matrixC18_12,
matrixC18_13,
matrixC18_14,
matrixC18_15,
matrixC18_16,
matrixC18_17,
matrixC18_18,
matrixC18_19,
matrixC19_0,
matrixC19_1,
matrixC19_2,
matrixC19_3,
matrixC19_4,
matrixC19_5,
matrixC19_6,
matrixC19_7,
matrixC19_8,
matrixC19_9,
matrixC19_10,
matrixC19_11,
matrixC19_12,
matrixC19_13,
matrixC19_14,
matrixC19_15,
matrixC19_16,
matrixC19_17,
matrixC19_18,
matrixC19_19,

clk,
reset
);

input clk;
input reset;
input start_mat_mul;
input done_mat_mul;
input [`AWIDTH-1:0] address_mat_c;
input [`ADDR_STRIDE_WIDTH-1:0] address_stride_c;
input [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_in;
output [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_out;
output [`AWIDTH-1:0] c_addr;
output c_data_available;
input [7:0] clk_cnt;
output row_latch_en;
input [`DWIDTH-1:0] matrixC0_0;
input [`DWIDTH-1:0] matrixC0_1;
input [`DWIDTH-1:0] matrixC0_2;
input [`DWIDTH-1:0] matrixC0_3;
input [`DWIDTH-1:0] matrixC0_4;
input [`DWIDTH-1:0] matrixC0_5;
input [`DWIDTH-1:0] matrixC0_6;
input [`DWIDTH-1:0] matrixC0_7;
input [`DWIDTH-1:0] matrixC0_8;
input [`DWIDTH-1:0] matrixC0_9;
input [`DWIDTH-1:0] matrixC0_10;
input [`DWIDTH-1:0] matrixC0_11;
input [`DWIDTH-1:0] matrixC0_12;
input [`DWIDTH-1:0] matrixC0_13;
input [`DWIDTH-1:0] matrixC0_14;
input [`DWIDTH-1:0] matrixC0_15;
input [`DWIDTH-1:0] matrixC0_16;
input [`DWIDTH-1:0] matrixC0_17;
input [`DWIDTH-1:0] matrixC0_18;
input [`DWIDTH-1:0] matrixC0_19;
input [`DWIDTH-1:0] matrixC1_0;
input [`DWIDTH-1:0] matrixC1_1;
input [`DWIDTH-1:0] matrixC1_2;
input [`DWIDTH-1:0] matrixC1_3;
input [`DWIDTH-1:0] matrixC1_4;
input [`DWIDTH-1:0] matrixC1_5;
input [`DWIDTH-1:0] matrixC1_6;
input [`DWIDTH-1:0] matrixC1_7;
input [`DWIDTH-1:0] matrixC1_8;
input [`DWIDTH-1:0] matrixC1_9;
input [`DWIDTH-1:0] matrixC1_10;
input [`DWIDTH-1:0] matrixC1_11;
input [`DWIDTH-1:0] matrixC1_12;
input [`DWIDTH-1:0] matrixC1_13;
input [`DWIDTH-1:0] matrixC1_14;
input [`DWIDTH-1:0] matrixC1_15;
input [`DWIDTH-1:0] matrixC1_16;
input [`DWIDTH-1:0] matrixC1_17;
input [`DWIDTH-1:0] matrixC1_18;
input [`DWIDTH-1:0] matrixC1_19;
input [`DWIDTH-1:0] matrixC2_0;
input [`DWIDTH-1:0] matrixC2_1;
input [`DWIDTH-1:0] matrixC2_2;
input [`DWIDTH-1:0] matrixC2_3;
input [`DWIDTH-1:0] matrixC2_4;
input [`DWIDTH-1:0] matrixC2_5;
input [`DWIDTH-1:0] matrixC2_6;
input [`DWIDTH-1:0] matrixC2_7;
input [`DWIDTH-1:0] matrixC2_8;
input [`DWIDTH-1:0] matrixC2_9;
input [`DWIDTH-1:0] matrixC2_10;
input [`DWIDTH-1:0] matrixC2_11;
input [`DWIDTH-1:0] matrixC2_12;
input [`DWIDTH-1:0] matrixC2_13;
input [`DWIDTH-1:0] matrixC2_14;
input [`DWIDTH-1:0] matrixC2_15;
input [`DWIDTH-1:0] matrixC2_16;
input [`DWIDTH-1:0] matrixC2_17;
input [`DWIDTH-1:0] matrixC2_18;
input [`DWIDTH-1:0] matrixC2_19;
input [`DWIDTH-1:0] matrixC3_0;
input [`DWIDTH-1:0] matrixC3_1;
input [`DWIDTH-1:0] matrixC3_2;
input [`DWIDTH-1:0] matrixC3_3;
input [`DWIDTH-1:0] matrixC3_4;
input [`DWIDTH-1:0] matrixC3_5;
input [`DWIDTH-1:0] matrixC3_6;
input [`DWIDTH-1:0] matrixC3_7;
input [`DWIDTH-1:0] matrixC3_8;
input [`DWIDTH-1:0] matrixC3_9;
input [`DWIDTH-1:0] matrixC3_10;
input [`DWIDTH-1:0] matrixC3_11;
input [`DWIDTH-1:0] matrixC3_12;
input [`DWIDTH-1:0] matrixC3_13;
input [`DWIDTH-1:0] matrixC3_14;
input [`DWIDTH-1:0] matrixC3_15;
input [`DWIDTH-1:0] matrixC3_16;
input [`DWIDTH-1:0] matrixC3_17;
input [`DWIDTH-1:0] matrixC3_18;
input [`DWIDTH-1:0] matrixC3_19;
input [`DWIDTH-1:0] matrixC4_0;
input [`DWIDTH-1:0] matrixC4_1;
input [`DWIDTH-1:0] matrixC4_2;
input [`DWIDTH-1:0] matrixC4_3;
input [`DWIDTH-1:0] matrixC4_4;
input [`DWIDTH-1:0] matrixC4_5;
input [`DWIDTH-1:0] matrixC4_6;
input [`DWIDTH-1:0] matrixC4_7;
input [`DWIDTH-1:0] matrixC4_8;
input [`DWIDTH-1:0] matrixC4_9;
input [`DWIDTH-1:0] matrixC4_10;
input [`DWIDTH-1:0] matrixC4_11;
input [`DWIDTH-1:0] matrixC4_12;
input [`DWIDTH-1:0] matrixC4_13;
input [`DWIDTH-1:0] matrixC4_14;
input [`DWIDTH-1:0] matrixC4_15;
input [`DWIDTH-1:0] matrixC4_16;
input [`DWIDTH-1:0] matrixC4_17;
input [`DWIDTH-1:0] matrixC4_18;
input [`DWIDTH-1:0] matrixC4_19;
input [`DWIDTH-1:0] matrixC5_0;
input [`DWIDTH-1:0] matrixC5_1;
input [`DWIDTH-1:0] matrixC5_2;
input [`DWIDTH-1:0] matrixC5_3;
input [`DWIDTH-1:0] matrixC5_4;
input [`DWIDTH-1:0] matrixC5_5;
input [`DWIDTH-1:0] matrixC5_6;
input [`DWIDTH-1:0] matrixC5_7;
input [`DWIDTH-1:0] matrixC5_8;
input [`DWIDTH-1:0] matrixC5_9;
input [`DWIDTH-1:0] matrixC5_10;
input [`DWIDTH-1:0] matrixC5_11;
input [`DWIDTH-1:0] matrixC5_12;
input [`DWIDTH-1:0] matrixC5_13;
input [`DWIDTH-1:0] matrixC5_14;
input [`DWIDTH-1:0] matrixC5_15;
input [`DWIDTH-1:0] matrixC5_16;
input [`DWIDTH-1:0] matrixC5_17;
input [`DWIDTH-1:0] matrixC5_18;
input [`DWIDTH-1:0] matrixC5_19;
input [`DWIDTH-1:0] matrixC6_0;
input [`DWIDTH-1:0] matrixC6_1;
input [`DWIDTH-1:0] matrixC6_2;
input [`DWIDTH-1:0] matrixC6_3;
input [`DWIDTH-1:0] matrixC6_4;
input [`DWIDTH-1:0] matrixC6_5;
input [`DWIDTH-1:0] matrixC6_6;
input [`DWIDTH-1:0] matrixC6_7;
input [`DWIDTH-1:0] matrixC6_8;
input [`DWIDTH-1:0] matrixC6_9;
input [`DWIDTH-1:0] matrixC6_10;
input [`DWIDTH-1:0] matrixC6_11;
input [`DWIDTH-1:0] matrixC6_12;
input [`DWIDTH-1:0] matrixC6_13;
input [`DWIDTH-1:0] matrixC6_14;
input [`DWIDTH-1:0] matrixC6_15;
input [`DWIDTH-1:0] matrixC6_16;
input [`DWIDTH-1:0] matrixC6_17;
input [`DWIDTH-1:0] matrixC6_18;
input [`DWIDTH-1:0] matrixC6_19;
input [`DWIDTH-1:0] matrixC7_0;
input [`DWIDTH-1:0] matrixC7_1;
input [`DWIDTH-1:0] matrixC7_2;
input [`DWIDTH-1:0] matrixC7_3;
input [`DWIDTH-1:0] matrixC7_4;
input [`DWIDTH-1:0] matrixC7_5;
input [`DWIDTH-1:0] matrixC7_6;
input [`DWIDTH-1:0] matrixC7_7;
input [`DWIDTH-1:0] matrixC7_8;
input [`DWIDTH-1:0] matrixC7_9;
input [`DWIDTH-1:0] matrixC7_10;
input [`DWIDTH-1:0] matrixC7_11;
input [`DWIDTH-1:0] matrixC7_12;
input [`DWIDTH-1:0] matrixC7_13;
input [`DWIDTH-1:0] matrixC7_14;
input [`DWIDTH-1:0] matrixC7_15;
input [`DWIDTH-1:0] matrixC7_16;
input [`DWIDTH-1:0] matrixC7_17;
input [`DWIDTH-1:0] matrixC7_18;
input [`DWIDTH-1:0] matrixC7_19;
input [`DWIDTH-1:0] matrixC8_0;
input [`DWIDTH-1:0] matrixC8_1;
input [`DWIDTH-1:0] matrixC8_2;
input [`DWIDTH-1:0] matrixC8_3;
input [`DWIDTH-1:0] matrixC8_4;
input [`DWIDTH-1:0] matrixC8_5;
input [`DWIDTH-1:0] matrixC8_6;
input [`DWIDTH-1:0] matrixC8_7;
input [`DWIDTH-1:0] matrixC8_8;
input [`DWIDTH-1:0] matrixC8_9;
input [`DWIDTH-1:0] matrixC8_10;
input [`DWIDTH-1:0] matrixC8_11;
input [`DWIDTH-1:0] matrixC8_12;
input [`DWIDTH-1:0] matrixC8_13;
input [`DWIDTH-1:0] matrixC8_14;
input [`DWIDTH-1:0] matrixC8_15;
input [`DWIDTH-1:0] matrixC8_16;
input [`DWIDTH-1:0] matrixC8_17;
input [`DWIDTH-1:0] matrixC8_18;
input [`DWIDTH-1:0] matrixC8_19;
input [`DWIDTH-1:0] matrixC9_0;
input [`DWIDTH-1:0] matrixC9_1;
input [`DWIDTH-1:0] matrixC9_2;
input [`DWIDTH-1:0] matrixC9_3;
input [`DWIDTH-1:0] matrixC9_4;
input [`DWIDTH-1:0] matrixC9_5;
input [`DWIDTH-1:0] matrixC9_6;
input [`DWIDTH-1:0] matrixC9_7;
input [`DWIDTH-1:0] matrixC9_8;
input [`DWIDTH-1:0] matrixC9_9;
input [`DWIDTH-1:0] matrixC9_10;
input [`DWIDTH-1:0] matrixC9_11;
input [`DWIDTH-1:0] matrixC9_12;
input [`DWIDTH-1:0] matrixC9_13;
input [`DWIDTH-1:0] matrixC9_14;
input [`DWIDTH-1:0] matrixC9_15;
input [`DWIDTH-1:0] matrixC9_16;
input [`DWIDTH-1:0] matrixC9_17;
input [`DWIDTH-1:0] matrixC9_18;
input [`DWIDTH-1:0] matrixC9_19;
input [`DWIDTH-1:0] matrixC10_0;
input [`DWIDTH-1:0] matrixC10_1;
input [`DWIDTH-1:0] matrixC10_2;
input [`DWIDTH-1:0] matrixC10_3;
input [`DWIDTH-1:0] matrixC10_4;
input [`DWIDTH-1:0] matrixC10_5;
input [`DWIDTH-1:0] matrixC10_6;
input [`DWIDTH-1:0] matrixC10_7;
input [`DWIDTH-1:0] matrixC10_8;
input [`DWIDTH-1:0] matrixC10_9;
input [`DWIDTH-1:0] matrixC10_10;
input [`DWIDTH-1:0] matrixC10_11;
input [`DWIDTH-1:0] matrixC10_12;
input [`DWIDTH-1:0] matrixC10_13;
input [`DWIDTH-1:0] matrixC10_14;
input [`DWIDTH-1:0] matrixC10_15;
input [`DWIDTH-1:0] matrixC10_16;
input [`DWIDTH-1:0] matrixC10_17;
input [`DWIDTH-1:0] matrixC10_18;
input [`DWIDTH-1:0] matrixC10_19;
input [`DWIDTH-1:0] matrixC11_0;
input [`DWIDTH-1:0] matrixC11_1;
input [`DWIDTH-1:0] matrixC11_2;
input [`DWIDTH-1:0] matrixC11_3;
input [`DWIDTH-1:0] matrixC11_4;
input [`DWIDTH-1:0] matrixC11_5;
input [`DWIDTH-1:0] matrixC11_6;
input [`DWIDTH-1:0] matrixC11_7;
input [`DWIDTH-1:0] matrixC11_8;
input [`DWIDTH-1:0] matrixC11_9;
input [`DWIDTH-1:0] matrixC11_10;
input [`DWIDTH-1:0] matrixC11_11;
input [`DWIDTH-1:0] matrixC11_12;
input [`DWIDTH-1:0] matrixC11_13;
input [`DWIDTH-1:0] matrixC11_14;
input [`DWIDTH-1:0] matrixC11_15;
input [`DWIDTH-1:0] matrixC11_16;
input [`DWIDTH-1:0] matrixC11_17;
input [`DWIDTH-1:0] matrixC11_18;
input [`DWIDTH-1:0] matrixC11_19;
input [`DWIDTH-1:0] matrixC12_0;
input [`DWIDTH-1:0] matrixC12_1;
input [`DWIDTH-1:0] matrixC12_2;
input [`DWIDTH-1:0] matrixC12_3;
input [`DWIDTH-1:0] matrixC12_4;
input [`DWIDTH-1:0] matrixC12_5;
input [`DWIDTH-1:0] matrixC12_6;
input [`DWIDTH-1:0] matrixC12_7;
input [`DWIDTH-1:0] matrixC12_8;
input [`DWIDTH-1:0] matrixC12_9;
input [`DWIDTH-1:0] matrixC12_10;
input [`DWIDTH-1:0] matrixC12_11;
input [`DWIDTH-1:0] matrixC12_12;
input [`DWIDTH-1:0] matrixC12_13;
input [`DWIDTH-1:0] matrixC12_14;
input [`DWIDTH-1:0] matrixC12_15;
input [`DWIDTH-1:0] matrixC12_16;
input [`DWIDTH-1:0] matrixC12_17;
input [`DWIDTH-1:0] matrixC12_18;
input [`DWIDTH-1:0] matrixC12_19;
input [`DWIDTH-1:0] matrixC13_0;
input [`DWIDTH-1:0] matrixC13_1;
input [`DWIDTH-1:0] matrixC13_2;
input [`DWIDTH-1:0] matrixC13_3;
input [`DWIDTH-1:0] matrixC13_4;
input [`DWIDTH-1:0] matrixC13_5;
input [`DWIDTH-1:0] matrixC13_6;
input [`DWIDTH-1:0] matrixC13_7;
input [`DWIDTH-1:0] matrixC13_8;
input [`DWIDTH-1:0] matrixC13_9;
input [`DWIDTH-1:0] matrixC13_10;
input [`DWIDTH-1:0] matrixC13_11;
input [`DWIDTH-1:0] matrixC13_12;
input [`DWIDTH-1:0] matrixC13_13;
input [`DWIDTH-1:0] matrixC13_14;
input [`DWIDTH-1:0] matrixC13_15;
input [`DWIDTH-1:0] matrixC13_16;
input [`DWIDTH-1:0] matrixC13_17;
input [`DWIDTH-1:0] matrixC13_18;
input [`DWIDTH-1:0] matrixC13_19;
input [`DWIDTH-1:0] matrixC14_0;
input [`DWIDTH-1:0] matrixC14_1;
input [`DWIDTH-1:0] matrixC14_2;
input [`DWIDTH-1:0] matrixC14_3;
input [`DWIDTH-1:0] matrixC14_4;
input [`DWIDTH-1:0] matrixC14_5;
input [`DWIDTH-1:0] matrixC14_6;
input [`DWIDTH-1:0] matrixC14_7;
input [`DWIDTH-1:0] matrixC14_8;
input [`DWIDTH-1:0] matrixC14_9;
input [`DWIDTH-1:0] matrixC14_10;
input [`DWIDTH-1:0] matrixC14_11;
input [`DWIDTH-1:0] matrixC14_12;
input [`DWIDTH-1:0] matrixC14_13;
input [`DWIDTH-1:0] matrixC14_14;
input [`DWIDTH-1:0] matrixC14_15;
input [`DWIDTH-1:0] matrixC14_16;
input [`DWIDTH-1:0] matrixC14_17;
input [`DWIDTH-1:0] matrixC14_18;
input [`DWIDTH-1:0] matrixC14_19;
input [`DWIDTH-1:0] matrixC15_0;
input [`DWIDTH-1:0] matrixC15_1;
input [`DWIDTH-1:0] matrixC15_2;
input [`DWIDTH-1:0] matrixC15_3;
input [`DWIDTH-1:0] matrixC15_4;
input [`DWIDTH-1:0] matrixC15_5;
input [`DWIDTH-1:0] matrixC15_6;
input [`DWIDTH-1:0] matrixC15_7;
input [`DWIDTH-1:0] matrixC15_8;
input [`DWIDTH-1:0] matrixC15_9;
input [`DWIDTH-1:0] matrixC15_10;
input [`DWIDTH-1:0] matrixC15_11;
input [`DWIDTH-1:0] matrixC15_12;
input [`DWIDTH-1:0] matrixC15_13;
input [`DWIDTH-1:0] matrixC15_14;
input [`DWIDTH-1:0] matrixC15_15;
input [`DWIDTH-1:0] matrixC15_16;
input [`DWIDTH-1:0] matrixC15_17;
input [`DWIDTH-1:0] matrixC15_18;
input [`DWIDTH-1:0] matrixC15_19;
input [`DWIDTH-1:0] matrixC16_0;
input [`DWIDTH-1:0] matrixC16_1;
input [`DWIDTH-1:0] matrixC16_2;
input [`DWIDTH-1:0] matrixC16_3;
input [`DWIDTH-1:0] matrixC16_4;
input [`DWIDTH-1:0] matrixC16_5;
input [`DWIDTH-1:0] matrixC16_6;
input [`DWIDTH-1:0] matrixC16_7;
input [`DWIDTH-1:0] matrixC16_8;
input [`DWIDTH-1:0] matrixC16_9;
input [`DWIDTH-1:0] matrixC16_10;
input [`DWIDTH-1:0] matrixC16_11;
input [`DWIDTH-1:0] matrixC16_12;
input [`DWIDTH-1:0] matrixC16_13;
input [`DWIDTH-1:0] matrixC16_14;
input [`DWIDTH-1:0] matrixC16_15;
input [`DWIDTH-1:0] matrixC16_16;
input [`DWIDTH-1:0] matrixC16_17;
input [`DWIDTH-1:0] matrixC16_18;
input [`DWIDTH-1:0] matrixC16_19;
input [`DWIDTH-1:0] matrixC17_0;
input [`DWIDTH-1:0] matrixC17_1;
input [`DWIDTH-1:0] matrixC17_2;
input [`DWIDTH-1:0] matrixC17_3;
input [`DWIDTH-1:0] matrixC17_4;
input [`DWIDTH-1:0] matrixC17_5;
input [`DWIDTH-1:0] matrixC17_6;
input [`DWIDTH-1:0] matrixC17_7;
input [`DWIDTH-1:0] matrixC17_8;
input [`DWIDTH-1:0] matrixC17_9;
input [`DWIDTH-1:0] matrixC17_10;
input [`DWIDTH-1:0] matrixC17_11;
input [`DWIDTH-1:0] matrixC17_12;
input [`DWIDTH-1:0] matrixC17_13;
input [`DWIDTH-1:0] matrixC17_14;
input [`DWIDTH-1:0] matrixC17_15;
input [`DWIDTH-1:0] matrixC17_16;
input [`DWIDTH-1:0] matrixC17_17;
input [`DWIDTH-1:0] matrixC17_18;
input [`DWIDTH-1:0] matrixC17_19;
input [`DWIDTH-1:0] matrixC18_0;
input [`DWIDTH-1:0] matrixC18_1;
input [`DWIDTH-1:0] matrixC18_2;
input [`DWIDTH-1:0] matrixC18_3;
input [`DWIDTH-1:0] matrixC18_4;
input [`DWIDTH-1:0] matrixC18_5;
input [`DWIDTH-1:0] matrixC18_6;
input [`DWIDTH-1:0] matrixC18_7;
input [`DWIDTH-1:0] matrixC18_8;
input [`DWIDTH-1:0] matrixC18_9;
input [`DWIDTH-1:0] matrixC18_10;
input [`DWIDTH-1:0] matrixC18_11;
input [`DWIDTH-1:0] matrixC18_12;
input [`DWIDTH-1:0] matrixC18_13;
input [`DWIDTH-1:0] matrixC18_14;
input [`DWIDTH-1:0] matrixC18_15;
input [`DWIDTH-1:0] matrixC18_16;
input [`DWIDTH-1:0] matrixC18_17;
input [`DWIDTH-1:0] matrixC18_18;
input [`DWIDTH-1:0] matrixC18_19;
input [`DWIDTH-1:0] matrixC19_0;
input [`DWIDTH-1:0] matrixC19_1;
input [`DWIDTH-1:0] matrixC19_2;
input [`DWIDTH-1:0] matrixC19_3;
input [`DWIDTH-1:0] matrixC19_4;
input [`DWIDTH-1:0] matrixC19_5;
input [`DWIDTH-1:0] matrixC19_6;
input [`DWIDTH-1:0] matrixC19_7;
input [`DWIDTH-1:0] matrixC19_8;
input [`DWIDTH-1:0] matrixC19_9;
input [`DWIDTH-1:0] matrixC19_10;
input [`DWIDTH-1:0] matrixC19_11;
input [`DWIDTH-1:0] matrixC19_12;
input [`DWIDTH-1:0] matrixC19_13;
input [`DWIDTH-1:0] matrixC19_14;
input [`DWIDTH-1:0] matrixC19_15;
input [`DWIDTH-1:0] matrixC19_16;
input [`DWIDTH-1:0] matrixC19_17;
input [`DWIDTH-1:0] matrixC19_18;
input [`DWIDTH-1:0] matrixC19_19;
wire row_latch_en;


//////////////////////////////////////////////////////////////////////////
// Logic to capture matrix C data from the PEs and shift it out
//////////////////////////////////////////////////////////////////////////
//assign row_latch_en = (clk_cnt==(`MAT_MUL_SIZE + (a_loc+b_loc) * `BB_MAT_MUL_SIZE + 10 +  `NUM_CYCLES_IN_MAC - 1));
//Writing the line above to avoid multiplication:
//assign row_latch_en = (clk_cnt==(`MAT_MUL_SIZE + ((a_loc+b_loc) << `LOG2_MAT_MUL_SIZE) + 10 +  `NUM_CYCLES_IN_MAC - 1));

assign row_latch_en =  
                       ((clk_cnt == 62 ));
    
reg c_data_available;
reg [`AWIDTH-1:0] c_addr;
reg start_capturing_c_data;
integer counter;
reg [20*`DWIDTH-1:0] c_data_out;
reg [20*`DWIDTH-1:0] c_data_out_1;
reg [20*`DWIDTH-1:0] c_data_out_2;
reg [20*`DWIDTH-1:0] c_data_out_3;
reg [20*`DWIDTH-1:0] c_data_out_4;
reg [20*`DWIDTH-1:0] c_data_out_5;
reg [20*`DWIDTH-1:0] c_data_out_6;
reg [20*`DWIDTH-1:0] c_data_out_7;
reg [20*`DWIDTH-1:0] c_data_out_8;
reg [20*`DWIDTH-1:0] c_data_out_9;
reg [20*`DWIDTH-1:0] c_data_out_10;
reg [20*`DWIDTH-1:0] c_data_out_11;
reg [20*`DWIDTH-1:0] c_data_out_12;
reg [20*`DWIDTH-1:0] c_data_out_13;
reg [20*`DWIDTH-1:0] c_data_out_14;
reg [20*`DWIDTH-1:0] c_data_out_15;
reg [20*`DWIDTH-1:0] c_data_out_16;
reg [20*`DWIDTH-1:0] c_data_out_17;
reg [20*`DWIDTH-1:0] c_data_out_18;
reg [20*`DWIDTH-1:0] c_data_out_19;
wire condition_to_start_shifting_output;
assign condition_to_start_shifting_output = 
                          row_latch_en ;  

  
//For larger matmuls, this logic will have more entries in the case statement
always @(posedge clk) begin
  if (reset | ~start_mat_mul) begin
    start_capturing_c_data <= 1'b0;
    c_data_available <= 1'b0;
    c_addr <= address_mat_c + address_stride_c;
    c_data_out <= 0;
    counter <= 0;

    c_data_out_1 <= 0;
    c_data_out_2 <= 0;
    c_data_out_3 <= 0;
    c_data_out_4 <= 0;
    c_data_out_5 <= 0;
    c_data_out_6 <= 0;
    c_data_out_7 <= 0;
    c_data_out_8 <= 0;
    c_data_out_9 <= 0;
    c_data_out_10 <= 0;
    c_data_out_11 <= 0;
    c_data_out_12 <= 0;
    c_data_out_13 <= 0;
    c_data_out_14 <= 0;
    c_data_out_15 <= 0;
    c_data_out_16 <= 0;
    c_data_out_17 <= 0;
    c_data_out_18 <= 0;
    c_data_out_19 <= 0;
  end else if (condition_to_start_shifting_output) begin
    start_capturing_c_data <= 1'b1;
    c_data_available <= 1'b1;
    c_addr <= c_addr - address_stride_c;
    c_data_out <= {matrixC19_19, matrixC18_19, matrixC17_19, matrixC16_19, matrixC15_19, matrixC14_19, matrixC13_19, matrixC12_19, matrixC11_19, matrixC10_19, matrixC9_19, matrixC8_19, matrixC7_19, matrixC6_19, matrixC5_19, matrixC4_19, matrixC3_19, matrixC2_19, matrixC1_19, matrixC0_19};
      c_data_out_1 <= {matrixC19_18, matrixC18_18, matrixC17_18, matrixC16_18, matrixC15_18, matrixC14_18, matrixC13_18, matrixC12_18, matrixC11_18, matrixC10_18, matrixC9_18, matrixC8_18, matrixC7_18, matrixC6_18, matrixC5_18, matrixC4_18, matrixC3_18, matrixC2_18, matrixC1_18, matrixC0_18};
      c_data_out_2 <= {matrixC19_17, matrixC18_17, matrixC17_17, matrixC16_17, matrixC15_17, matrixC14_17, matrixC13_17, matrixC12_17, matrixC11_17, matrixC10_17, matrixC9_17, matrixC8_17, matrixC7_17, matrixC6_17, matrixC5_17, matrixC4_17, matrixC3_17, matrixC2_17, matrixC1_17, matrixC0_17};
      c_data_out_3 <= {matrixC19_16, matrixC18_16, matrixC17_16, matrixC16_16, matrixC15_16, matrixC14_16, matrixC13_16, matrixC12_16, matrixC11_16, matrixC10_16, matrixC9_16, matrixC8_16, matrixC7_16, matrixC6_16, matrixC5_16, matrixC4_16, matrixC3_16, matrixC2_16, matrixC1_16, matrixC0_16};
      c_data_out_4 <= {matrixC19_15, matrixC18_15, matrixC17_15, matrixC16_15, matrixC15_15, matrixC14_15, matrixC13_15, matrixC12_15, matrixC11_15, matrixC10_15, matrixC9_15, matrixC8_15, matrixC7_15, matrixC6_15, matrixC5_15, matrixC4_15, matrixC3_15, matrixC2_15, matrixC1_15, matrixC0_15};
      c_data_out_5 <= {matrixC19_14, matrixC18_14, matrixC17_14, matrixC16_14, matrixC15_14, matrixC14_14, matrixC13_14, matrixC12_14, matrixC11_14, matrixC10_14, matrixC9_14, matrixC8_14, matrixC7_14, matrixC6_14, matrixC5_14, matrixC4_14, matrixC3_14, matrixC2_14, matrixC1_14, matrixC0_14};
      c_data_out_6 <= {matrixC19_13, matrixC18_13, matrixC17_13, matrixC16_13, matrixC15_13, matrixC14_13, matrixC13_13, matrixC12_13, matrixC11_13, matrixC10_13, matrixC9_13, matrixC8_13, matrixC7_13, matrixC6_13, matrixC5_13, matrixC4_13, matrixC3_13, matrixC2_13, matrixC1_13, matrixC0_13};
      c_data_out_7 <= {matrixC19_12, matrixC18_12, matrixC17_12, matrixC16_12, matrixC15_12, matrixC14_12, matrixC13_12, matrixC12_12, matrixC11_12, matrixC10_12, matrixC9_12, matrixC8_12, matrixC7_12, matrixC6_12, matrixC5_12, matrixC4_12, matrixC3_12, matrixC2_12, matrixC1_12, matrixC0_12};
      c_data_out_8 <= {matrixC19_11, matrixC18_11, matrixC17_11, matrixC16_11, matrixC15_11, matrixC14_11, matrixC13_11, matrixC12_11, matrixC11_11, matrixC10_11, matrixC9_11, matrixC8_11, matrixC7_11, matrixC6_11, matrixC5_11, matrixC4_11, matrixC3_11, matrixC2_11, matrixC1_11, matrixC0_11};
      c_data_out_9 <= {matrixC19_10, matrixC18_10, matrixC17_10, matrixC16_10, matrixC15_10, matrixC14_10, matrixC13_10, matrixC12_10, matrixC11_10, matrixC10_10, matrixC9_10, matrixC8_10, matrixC7_10, matrixC6_10, matrixC5_10, matrixC4_10, matrixC3_10, matrixC2_10, matrixC1_10, matrixC0_10};
      c_data_out_10 <= {matrixC19_9, matrixC18_9, matrixC17_9, matrixC16_9, matrixC15_9, matrixC14_9, matrixC13_9, matrixC12_9, matrixC11_9, matrixC10_9, matrixC9_9, matrixC8_9, matrixC7_9, matrixC6_9, matrixC5_9, matrixC4_9, matrixC3_9, matrixC2_9, matrixC1_9, matrixC0_9};
      c_data_out_11 <= {matrixC19_8, matrixC18_8, matrixC17_8, matrixC16_8, matrixC15_8, matrixC14_8, matrixC13_8, matrixC12_8, matrixC11_8, matrixC10_8, matrixC9_8, matrixC8_8, matrixC7_8, matrixC6_8, matrixC5_8, matrixC4_8, matrixC3_8, matrixC2_8, matrixC1_8, matrixC0_8};
      c_data_out_12 <= {matrixC19_7, matrixC18_7, matrixC17_7, matrixC16_7, matrixC15_7, matrixC14_7, matrixC13_7, matrixC12_7, matrixC11_7, matrixC10_7, matrixC9_7, matrixC8_7, matrixC7_7, matrixC6_7, matrixC5_7, matrixC4_7, matrixC3_7, matrixC2_7, matrixC1_7, matrixC0_7};
      c_data_out_13 <= {matrixC19_6, matrixC18_6, matrixC17_6, matrixC16_6, matrixC15_6, matrixC14_6, matrixC13_6, matrixC12_6, matrixC11_6, matrixC10_6, matrixC9_6, matrixC8_6, matrixC7_6, matrixC6_6, matrixC5_6, matrixC4_6, matrixC3_6, matrixC2_6, matrixC1_6, matrixC0_6};
      c_data_out_14 <= {matrixC19_5, matrixC18_5, matrixC17_5, matrixC16_5, matrixC15_5, matrixC14_5, matrixC13_5, matrixC12_5, matrixC11_5, matrixC10_5, matrixC9_5, matrixC8_5, matrixC7_5, matrixC6_5, matrixC5_5, matrixC4_5, matrixC3_5, matrixC2_5, matrixC1_5, matrixC0_5};
      c_data_out_15 <= {matrixC19_4, matrixC18_4, matrixC17_4, matrixC16_4, matrixC15_4, matrixC14_4, matrixC13_4, matrixC12_4, matrixC11_4, matrixC10_4, matrixC9_4, matrixC8_4, matrixC7_4, matrixC6_4, matrixC5_4, matrixC4_4, matrixC3_4, matrixC2_4, matrixC1_4, matrixC0_4};
      c_data_out_16 <= {matrixC19_3, matrixC18_3, matrixC17_3, matrixC16_3, matrixC15_3, matrixC14_3, matrixC13_3, matrixC12_3, matrixC11_3, matrixC10_3, matrixC9_3, matrixC8_3, matrixC7_3, matrixC6_3, matrixC5_3, matrixC4_3, matrixC3_3, matrixC2_3, matrixC1_3, matrixC0_3};
      c_data_out_17 <= {matrixC19_2, matrixC18_2, matrixC17_2, matrixC16_2, matrixC15_2, matrixC14_2, matrixC13_2, matrixC12_2, matrixC11_2, matrixC10_2, matrixC9_2, matrixC8_2, matrixC7_2, matrixC6_2, matrixC5_2, matrixC4_2, matrixC3_2, matrixC2_2, matrixC1_2, matrixC0_2};
      c_data_out_18 <= {matrixC19_1, matrixC18_1, matrixC17_1, matrixC16_1, matrixC15_1, matrixC14_1, matrixC13_1, matrixC12_1, matrixC11_1, matrixC10_1, matrixC9_1, matrixC8_1, matrixC7_1, matrixC6_1, matrixC5_1, matrixC4_1, matrixC3_1, matrixC2_1, matrixC1_1, matrixC0_1};
      c_data_out_19 <= {matrixC19_0, matrixC18_0, matrixC17_0, matrixC16_0, matrixC15_0, matrixC14_0, matrixC13_0, matrixC12_0, matrixC11_0, matrixC10_0, matrixC9_0, matrixC8_0, matrixC7_0, matrixC6_0, matrixC5_0, matrixC4_0, matrixC3_0, matrixC2_0, matrixC1_0, matrixC0_0};

    counter <= counter + 1;
  end else if (done_mat_mul) begin
    start_capturing_c_data <= 1'b0;
    c_data_available <= 1'b0;
    c_addr <= address_mat_c + address_stride_c;
    c_data_out <= 0;

    c_data_out_1 <= 0;
    c_data_out_2 <= 0;
    c_data_out_3 <= 0;
    c_data_out_4 <= 0;
    c_data_out_5 <= 0;
    c_data_out_6 <= 0;
    c_data_out_7 <= 0;
    c_data_out_8 <= 0;
    c_data_out_9 <= 0;
    c_data_out_10 <= 0;
    c_data_out_11 <= 0;
    c_data_out_12 <= 0;
    c_data_out_13 <= 0;
    c_data_out_14 <= 0;
    c_data_out_15 <= 0;
    c_data_out_16 <= 0;
    c_data_out_17 <= 0;
    c_data_out_18 <= 0;
    c_data_out_19 <= 0;
  end 
  else if (counter >= `MAT_MUL_SIZE) begin
    c_data_out <= c_data_out_1;
    c_addr <= c_addr - address_stride_c; 

    c_data_out_1 <= c_data_out_2;
    c_data_out_2 <= c_data_out_3;
    c_data_out_3 <= c_data_out_4;
    c_data_out_4 <= c_data_out_5;
    c_data_out_5 <= c_data_out_6;
    c_data_out_6 <= c_data_out_7;
    c_data_out_7 <= c_data_out_8;
    c_data_out_8 <= c_data_out_9;
    c_data_out_9 <= c_data_out_10;
    c_data_out_10 <= c_data_out_11;
    c_data_out_11 <= c_data_out_12;
    c_data_out_12 <= c_data_out_13;
    c_data_out_13 <= c_data_out_14;
    c_data_out_14 <= c_data_out_15;
    c_data_out_15 <= c_data_out_16;
    c_data_out_16 <= c_data_out_17;
    c_data_out_17 <= c_data_out_18;
    c_data_out_18 <= c_data_out_19;
    c_data_out_19 <= c_data_in;
  end
  else if (start_capturing_c_data) begin
    c_data_available <= 1'b1;
    c_addr <= c_addr - address_stride_c; 
    counter <= counter + 1;
    c_data_out <= c_data_out_1;

    c_data_out_1 <= c_data_out_2;
    c_data_out_2 <= c_data_out_3;
    c_data_out_3 <= c_data_out_4;
    c_data_out_4 <= c_data_out_5;
    c_data_out_5 <= c_data_out_6;
    c_data_out_6 <= c_data_out_7;
    c_data_out_7 <= c_data_out_8;
    c_data_out_8 <= c_data_out_9;
    c_data_out_9 <= c_data_out_10;
    c_data_out_10 <= c_data_out_11;
    c_data_out_11 <= c_data_out_12;
    c_data_out_12 <= c_data_out_13;
    c_data_out_13 <= c_data_out_14;
    c_data_out_14 <= c_data_out_15;
    c_data_out_15 <= c_data_out_16;
    c_data_out_16 <= c_data_out_17;
    c_data_out_17 <= c_data_out_18;
    c_data_out_18 <= c_data_out_19;
    c_data_out_19 <= c_data_in;
  end
end

endmodule


//////////////////////////////////////////////////////////////////////////
// Systolic data setup
//////////////////////////////////////////////////////////////////////////
module systolic_data_setup(
clk,
reset,
start_mat_mul,
a_addr,
b_addr,
address_mat_a,
address_mat_b,
address_stride_a,
address_stride_b,
a_data,
b_data,
clk_cnt,
a0_data,
b0_data,
a1_data_delayed_1,
b1_data_delayed_1,
a2_data_delayed_2,
b2_data_delayed_2,
a3_data_delayed_3,
b3_data_delayed_3,
a4_data_delayed_4,
b4_data_delayed_4,
a5_data_delayed_5,
b5_data_delayed_5,
a6_data_delayed_6,
b6_data_delayed_6,
a7_data_delayed_7,
b7_data_delayed_7,
a8_data_delayed_8,
b8_data_delayed_8,
a9_data_delayed_9,
b9_data_delayed_9,
a10_data_delayed_10,
b10_data_delayed_10,
a11_data_delayed_11,
b11_data_delayed_11,
a12_data_delayed_12,
b12_data_delayed_12,
a13_data_delayed_13,
b13_data_delayed_13,
a14_data_delayed_14,
b14_data_delayed_14,
a15_data_delayed_15,
b15_data_delayed_15,
a16_data_delayed_16,
b16_data_delayed_16,
a17_data_delayed_17,
b17_data_delayed_17,
a18_data_delayed_18,
b18_data_delayed_18,
a19_data_delayed_19,
b19_data_delayed_19,

validity_mask_a_rows,
validity_mask_a_cols,
validity_mask_b_rows,
validity_mask_b_cols,

a_loc,
b_loc
);

input clk;
input reset;
input start_mat_mul;
output [`AWIDTH-1:0] a_addr;
output [`AWIDTH-1:0] b_addr;
input [`AWIDTH-1:0] address_mat_a;
input [`AWIDTH-1:0] address_mat_b;
input [`ADDR_STRIDE_WIDTH-1:0] address_stride_a;
input [`ADDR_STRIDE_WIDTH-1:0] address_stride_b;
input [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data;
input [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data;
input [7:0] clk_cnt;
output [`DWIDTH-1:0] a0_data;
output [`DWIDTH-1:0] b0_data;
output [`DWIDTH-1:0] a1_data_delayed_1;
output [`DWIDTH-1:0] b1_data_delayed_1;
output [`DWIDTH-1:0] a2_data_delayed_2;
output [`DWIDTH-1:0] b2_data_delayed_2;
output [`DWIDTH-1:0] a3_data_delayed_3;
output [`DWIDTH-1:0] b3_data_delayed_3;
output [`DWIDTH-1:0] a4_data_delayed_4;
output [`DWIDTH-1:0] b4_data_delayed_4;
output [`DWIDTH-1:0] a5_data_delayed_5;
output [`DWIDTH-1:0] b5_data_delayed_5;
output [`DWIDTH-1:0] a6_data_delayed_6;
output [`DWIDTH-1:0] b6_data_delayed_6;
output [`DWIDTH-1:0] a7_data_delayed_7;
output [`DWIDTH-1:0] b7_data_delayed_7;
output [`DWIDTH-1:0] a8_data_delayed_8;
output [`DWIDTH-1:0] b8_data_delayed_8;
output [`DWIDTH-1:0] a9_data_delayed_9;
output [`DWIDTH-1:0] b9_data_delayed_9;
output [`DWIDTH-1:0] a10_data_delayed_10;
output [`DWIDTH-1:0] b10_data_delayed_10;
output [`DWIDTH-1:0] a11_data_delayed_11;
output [`DWIDTH-1:0] b11_data_delayed_11;
output [`DWIDTH-1:0] a12_data_delayed_12;
output [`DWIDTH-1:0] b12_data_delayed_12;
output [`DWIDTH-1:0] a13_data_delayed_13;
output [`DWIDTH-1:0] b13_data_delayed_13;
output [`DWIDTH-1:0] a14_data_delayed_14;
output [`DWIDTH-1:0] b14_data_delayed_14;
output [`DWIDTH-1:0] a15_data_delayed_15;
output [`DWIDTH-1:0] b15_data_delayed_15;
output [`DWIDTH-1:0] a16_data_delayed_16;
output [`DWIDTH-1:0] b16_data_delayed_16;
output [`DWIDTH-1:0] a17_data_delayed_17;
output [`DWIDTH-1:0] b17_data_delayed_17;
output [`DWIDTH-1:0] a18_data_delayed_18;
output [`DWIDTH-1:0] b18_data_delayed_18;
output [`DWIDTH-1:0] a19_data_delayed_19;
output [`DWIDTH-1:0] b19_data_delayed_19;

input [`MASK_WIDTH-1:0] validity_mask_a_rows;
input [`MASK_WIDTH-1:0] validity_mask_a_cols;
input [`MASK_WIDTH-1:0] validity_mask_b_rows;
input [`MASK_WIDTH-1:0] validity_mask_b_cols;

input [7:0] a_loc;
input [7:0] b_loc;
wire [`DWIDTH-1:0] a0_data;
wire [`DWIDTH-1:0] a1_data;
wire [`DWIDTH-1:0] a2_data;
wire [`DWIDTH-1:0] a3_data;
wire [`DWIDTH-1:0] a4_data;
wire [`DWIDTH-1:0] a5_data;
wire [`DWIDTH-1:0] a6_data;
wire [`DWIDTH-1:0] a7_data;
wire [`DWIDTH-1:0] a8_data;
wire [`DWIDTH-1:0] a9_data;
wire [`DWIDTH-1:0] a10_data;
wire [`DWIDTH-1:0] a11_data;
wire [`DWIDTH-1:0] a12_data;
wire [`DWIDTH-1:0] a13_data;
wire [`DWIDTH-1:0] a14_data;
wire [`DWIDTH-1:0] a15_data;
wire [`DWIDTH-1:0] a16_data;
wire [`DWIDTH-1:0] a17_data;
wire [`DWIDTH-1:0] a18_data;
wire [`DWIDTH-1:0] a19_data;
wire [`DWIDTH-1:0] b0_data;
wire [`DWIDTH-1:0] b1_data;
wire [`DWIDTH-1:0] b2_data;
wire [`DWIDTH-1:0] b3_data;
wire [`DWIDTH-1:0] b4_data;
wire [`DWIDTH-1:0] b5_data;
wire [`DWIDTH-1:0] b6_data;
wire [`DWIDTH-1:0] b7_data;
wire [`DWIDTH-1:0] b8_data;
wire [`DWIDTH-1:0] b9_data;
wire [`DWIDTH-1:0] b10_data;
wire [`DWIDTH-1:0] b11_data;
wire [`DWIDTH-1:0] b12_data;
wire [`DWIDTH-1:0] b13_data;
wire [`DWIDTH-1:0] b14_data;
wire [`DWIDTH-1:0] b15_data;
wire [`DWIDTH-1:0] b16_data;
wire [`DWIDTH-1:0] b17_data;
wire [`DWIDTH-1:0] b18_data;
wire [`DWIDTH-1:0] b19_data;

//////////////////////////////////////////////////////////////////////////
// Logic to generate addresses to BRAM A
//////////////////////////////////////////////////////////////////////////
reg [`AWIDTH-1:0] a_addr;
reg a_mem_access; //flag that tells whether the matmul is trying to access memory or not

always @(posedge clk) begin
  //(clk_cnt >= a_loc*`MAT_MUL_SIZE+final_mat_mul_size) begin
  //Writing the line above to avoid multiplication:

  if (reset || ~start_mat_mul || (clk_cnt >= (a_loc<<`LOG2_MAT_MUL_SIZE)+20)) begin
  
      a_addr <= address_mat_a-address_stride_a;
  
    a_mem_access <= 0;
  end
  //else if ((clk_cnt >= a_loc*`MAT_MUL_SIZE) && (clk_cnt < a_loc*`MAT_MUL_SIZE+final_mat_mul_size)) begin
  //Writing the line above to avoid multiplication:

  else if ((clk_cnt >= (a_loc<<`LOG2_MAT_MUL_SIZE)) && (clk_cnt < (a_loc<<`LOG2_MAT_MUL_SIZE)+20)) begin
  
      a_addr <= a_addr + address_stride_a;
  
    a_mem_access <= 1;
  end
end

//////////////////////////////////////////////////////////////////////////
// Logic to generate valid signals for data coming from BRAM A
//////////////////////////////////////////////////////////////////////////
reg [7:0] a_mem_access_counter;
always @(posedge clk) begin
  if (reset || ~start_mat_mul) begin
    a_mem_access_counter <= 0;
  end
  else if (a_mem_access == 1) begin
    a_mem_access_counter <= a_mem_access_counter + 1;  
  end
  else begin
    a_mem_access_counter <= 0;
  end
end

wire a_data_valid; //flag that tells whether the data from memory is valid
assign a_data_valid = 
     ((validity_mask_a_cols[0]==1'b0 && a_mem_access_counter==1) ||
      (validity_mask_a_cols[1]==1'b0 && a_mem_access_counter==2) ||
      (validity_mask_a_cols[2]==1'b0 && a_mem_access_counter==3) ||
      (validity_mask_a_cols[3]==1'b0 && a_mem_access_counter==4) ||
      (validity_mask_a_cols[4]==1'b0 && a_mem_access_counter==5) ||
      (validity_mask_a_cols[5]==1'b0 && a_mem_access_counter==6) ||
      (validity_mask_a_cols[6]==1'b0 && a_mem_access_counter==7) ||
      (validity_mask_a_cols[7]==1'b0 && a_mem_access_counter==8) ||
      (validity_mask_a_cols[8]==1'b0 && a_mem_access_counter==9) ||
      (validity_mask_a_cols[9]==1'b0 && a_mem_access_counter==10) ||
      (validity_mask_a_cols[10]==1'b0 && a_mem_access_counter==11) ||
      (validity_mask_a_cols[11]==1'b0 && a_mem_access_counter==12) ||
      (validity_mask_a_cols[12]==1'b0 && a_mem_access_counter==13) ||
      (validity_mask_a_cols[13]==1'b0 && a_mem_access_counter==14) ||
      (validity_mask_a_cols[14]==1'b0 && a_mem_access_counter==15) ||
      (validity_mask_a_cols[15]==1'b0 && a_mem_access_counter==16) ||
      (validity_mask_a_cols[16]==1'b0 && a_mem_access_counter==17) ||
      (validity_mask_a_cols[17]==1'b0 && a_mem_access_counter==18) ||
      (validity_mask_a_cols[18]==1'b0 && a_mem_access_counter==19) ||
      (validity_mask_a_cols[19]==1'b0 && a_mem_access_counter==20)) ?
    
    1'b0 : (a_mem_access_counter >= `MEM_ACCESS_LATENCY);

//////////////////////////////////////////////////////////////////////////
// Logic to delay certain parts of the data received from BRAM A (systolic data setup)
//////////////////////////////////////////////////////////////////////////
assign a0_data = a_data[1*`DWIDTH-1:0*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[0]}};
assign a1_data = a_data[2*`DWIDTH-1:1*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[1]}};
assign a2_data = a_data[3*`DWIDTH-1:2*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[2]}};
assign a3_data = a_data[4*`DWIDTH-1:3*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[3]}};
assign a4_data = a_data[5*`DWIDTH-1:4*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[4]}};
assign a5_data = a_data[6*`DWIDTH-1:5*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[5]}};
assign a6_data = a_data[7*`DWIDTH-1:6*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[6]}};
assign a7_data = a_data[8*`DWIDTH-1:7*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[7]}};
assign a8_data = a_data[9*`DWIDTH-1:8*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[8]}};
assign a9_data = a_data[10*`DWIDTH-1:9*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[9]}};
assign a10_data = a_data[11*`DWIDTH-1:10*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[10]}};
assign a11_data = a_data[12*`DWIDTH-1:11*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[11]}};
assign a12_data = a_data[13*`DWIDTH-1:12*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[12]}};
assign a13_data = a_data[14*`DWIDTH-1:13*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[13]}};
assign a14_data = a_data[15*`DWIDTH-1:14*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[14]}};
assign a15_data = a_data[16*`DWIDTH-1:15*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[15]}};
assign a16_data = a_data[17*`DWIDTH-1:16*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[16]}};
assign a17_data = a_data[18*`DWIDTH-1:17*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[17]}};
assign a18_data = a_data[19*`DWIDTH-1:18*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[18]}};
assign a19_data = a_data[20*`DWIDTH-1:19*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[19]}};

reg [`DWIDTH-1:0] a1_data_delayed_1;
reg [`DWIDTH-1:0] a2_data_delayed_1;
reg [`DWIDTH-1:0] a2_data_delayed_2;
reg [`DWIDTH-1:0] a3_data_delayed_1;
reg [`DWIDTH-1:0] a3_data_delayed_2;
reg [`DWIDTH-1:0] a3_data_delayed_3;
reg [`DWIDTH-1:0] a4_data_delayed_1;
reg [`DWIDTH-1:0] a4_data_delayed_2;
reg [`DWIDTH-1:0] a4_data_delayed_3;
reg [`DWIDTH-1:0] a4_data_delayed_4;
reg [`DWIDTH-1:0] a5_data_delayed_1;
reg [`DWIDTH-1:0] a5_data_delayed_2;
reg [`DWIDTH-1:0] a5_data_delayed_3;
reg [`DWIDTH-1:0] a5_data_delayed_4;
reg [`DWIDTH-1:0] a5_data_delayed_5;
reg [`DWIDTH-1:0] a6_data_delayed_1;
reg [`DWIDTH-1:0] a6_data_delayed_2;
reg [`DWIDTH-1:0] a6_data_delayed_3;
reg [`DWIDTH-1:0] a6_data_delayed_4;
reg [`DWIDTH-1:0] a6_data_delayed_5;
reg [`DWIDTH-1:0] a6_data_delayed_6;
reg [`DWIDTH-1:0] a7_data_delayed_1;
reg [`DWIDTH-1:0] a7_data_delayed_2;
reg [`DWIDTH-1:0] a7_data_delayed_3;
reg [`DWIDTH-1:0] a7_data_delayed_4;
reg [`DWIDTH-1:0] a7_data_delayed_5;
reg [`DWIDTH-1:0] a7_data_delayed_6;
reg [`DWIDTH-1:0] a7_data_delayed_7;
reg [`DWIDTH-1:0] a8_data_delayed_1;
reg [`DWIDTH-1:0] a8_data_delayed_2;
reg [`DWIDTH-1:0] a8_data_delayed_3;
reg [`DWIDTH-1:0] a8_data_delayed_4;
reg [`DWIDTH-1:0] a8_data_delayed_5;
reg [`DWIDTH-1:0] a8_data_delayed_6;
reg [`DWIDTH-1:0] a8_data_delayed_7;
reg [`DWIDTH-1:0] a8_data_delayed_8;
reg [`DWIDTH-1:0] a9_data_delayed_1;
reg [`DWIDTH-1:0] a9_data_delayed_2;
reg [`DWIDTH-1:0] a9_data_delayed_3;
reg [`DWIDTH-1:0] a9_data_delayed_4;
reg [`DWIDTH-1:0] a9_data_delayed_5;
reg [`DWIDTH-1:0] a9_data_delayed_6;
reg [`DWIDTH-1:0] a9_data_delayed_7;
reg [`DWIDTH-1:0] a9_data_delayed_8;
reg [`DWIDTH-1:0] a9_data_delayed_9;
reg [`DWIDTH-1:0] a10_data_delayed_1;
reg [`DWIDTH-1:0] a10_data_delayed_2;
reg [`DWIDTH-1:0] a10_data_delayed_3;
reg [`DWIDTH-1:0] a10_data_delayed_4;
reg [`DWIDTH-1:0] a10_data_delayed_5;
reg [`DWIDTH-1:0] a10_data_delayed_6;
reg [`DWIDTH-1:0] a10_data_delayed_7;
reg [`DWIDTH-1:0] a10_data_delayed_8;
reg [`DWIDTH-1:0] a10_data_delayed_9;
reg [`DWIDTH-1:0] a10_data_delayed_10;
reg [`DWIDTH-1:0] a11_data_delayed_1;
reg [`DWIDTH-1:0] a11_data_delayed_2;
reg [`DWIDTH-1:0] a11_data_delayed_3;
reg [`DWIDTH-1:0] a11_data_delayed_4;
reg [`DWIDTH-1:0] a11_data_delayed_5;
reg [`DWIDTH-1:0] a11_data_delayed_6;
reg [`DWIDTH-1:0] a11_data_delayed_7;
reg [`DWIDTH-1:0] a11_data_delayed_8;
reg [`DWIDTH-1:0] a11_data_delayed_9;
reg [`DWIDTH-1:0] a11_data_delayed_10;
reg [`DWIDTH-1:0] a11_data_delayed_11;
reg [`DWIDTH-1:0] a12_data_delayed_1;
reg [`DWIDTH-1:0] a12_data_delayed_2;
reg [`DWIDTH-1:0] a12_data_delayed_3;
reg [`DWIDTH-1:0] a12_data_delayed_4;
reg [`DWIDTH-1:0] a12_data_delayed_5;
reg [`DWIDTH-1:0] a12_data_delayed_6;
reg [`DWIDTH-1:0] a12_data_delayed_7;
reg [`DWIDTH-1:0] a12_data_delayed_8;
reg [`DWIDTH-1:0] a12_data_delayed_9;
reg [`DWIDTH-1:0] a12_data_delayed_10;
reg [`DWIDTH-1:0] a12_data_delayed_11;
reg [`DWIDTH-1:0] a12_data_delayed_12;
reg [`DWIDTH-1:0] a13_data_delayed_1;
reg [`DWIDTH-1:0] a13_data_delayed_2;
reg [`DWIDTH-1:0] a13_data_delayed_3;
reg [`DWIDTH-1:0] a13_data_delayed_4;
reg [`DWIDTH-1:0] a13_data_delayed_5;
reg [`DWIDTH-1:0] a13_data_delayed_6;
reg [`DWIDTH-1:0] a13_data_delayed_7;
reg [`DWIDTH-1:0] a13_data_delayed_8;
reg [`DWIDTH-1:0] a13_data_delayed_9;
reg [`DWIDTH-1:0] a13_data_delayed_10;
reg [`DWIDTH-1:0] a13_data_delayed_11;
reg [`DWIDTH-1:0] a13_data_delayed_12;
reg [`DWIDTH-1:0] a13_data_delayed_13;
reg [`DWIDTH-1:0] a14_data_delayed_1;
reg [`DWIDTH-1:0] a14_data_delayed_2;
reg [`DWIDTH-1:0] a14_data_delayed_3;
reg [`DWIDTH-1:0] a14_data_delayed_4;
reg [`DWIDTH-1:0] a14_data_delayed_5;
reg [`DWIDTH-1:0] a14_data_delayed_6;
reg [`DWIDTH-1:0] a14_data_delayed_7;
reg [`DWIDTH-1:0] a14_data_delayed_8;
reg [`DWIDTH-1:0] a14_data_delayed_9;
reg [`DWIDTH-1:0] a14_data_delayed_10;
reg [`DWIDTH-1:0] a14_data_delayed_11;
reg [`DWIDTH-1:0] a14_data_delayed_12;
reg [`DWIDTH-1:0] a14_data_delayed_13;
reg [`DWIDTH-1:0] a14_data_delayed_14;
reg [`DWIDTH-1:0] a15_data_delayed_1;
reg [`DWIDTH-1:0] a15_data_delayed_2;
reg [`DWIDTH-1:0] a15_data_delayed_3;
reg [`DWIDTH-1:0] a15_data_delayed_4;
reg [`DWIDTH-1:0] a15_data_delayed_5;
reg [`DWIDTH-1:0] a15_data_delayed_6;
reg [`DWIDTH-1:0] a15_data_delayed_7;
reg [`DWIDTH-1:0] a15_data_delayed_8;
reg [`DWIDTH-1:0] a15_data_delayed_9;
reg [`DWIDTH-1:0] a15_data_delayed_10;
reg [`DWIDTH-1:0] a15_data_delayed_11;
reg [`DWIDTH-1:0] a15_data_delayed_12;
reg [`DWIDTH-1:0] a15_data_delayed_13;
reg [`DWIDTH-1:0] a15_data_delayed_14;
reg [`DWIDTH-1:0] a15_data_delayed_15;
reg [`DWIDTH-1:0] a16_data_delayed_1;
reg [`DWIDTH-1:0] a16_data_delayed_2;
reg [`DWIDTH-1:0] a16_data_delayed_3;
reg [`DWIDTH-1:0] a16_data_delayed_4;
reg [`DWIDTH-1:0] a16_data_delayed_5;
reg [`DWIDTH-1:0] a16_data_delayed_6;
reg [`DWIDTH-1:0] a16_data_delayed_7;
reg [`DWIDTH-1:0] a16_data_delayed_8;
reg [`DWIDTH-1:0] a16_data_delayed_9;
reg [`DWIDTH-1:0] a16_data_delayed_10;
reg [`DWIDTH-1:0] a16_data_delayed_11;
reg [`DWIDTH-1:0] a16_data_delayed_12;
reg [`DWIDTH-1:0] a16_data_delayed_13;
reg [`DWIDTH-1:0] a16_data_delayed_14;
reg [`DWIDTH-1:0] a16_data_delayed_15;
reg [`DWIDTH-1:0] a16_data_delayed_16;
reg [`DWIDTH-1:0] a17_data_delayed_1;
reg [`DWIDTH-1:0] a17_data_delayed_2;
reg [`DWIDTH-1:0] a17_data_delayed_3;
reg [`DWIDTH-1:0] a17_data_delayed_4;
reg [`DWIDTH-1:0] a17_data_delayed_5;
reg [`DWIDTH-1:0] a17_data_delayed_6;
reg [`DWIDTH-1:0] a17_data_delayed_7;
reg [`DWIDTH-1:0] a17_data_delayed_8;
reg [`DWIDTH-1:0] a17_data_delayed_9;
reg [`DWIDTH-1:0] a17_data_delayed_10;
reg [`DWIDTH-1:0] a17_data_delayed_11;
reg [`DWIDTH-1:0] a17_data_delayed_12;
reg [`DWIDTH-1:0] a17_data_delayed_13;
reg [`DWIDTH-1:0] a17_data_delayed_14;
reg [`DWIDTH-1:0] a17_data_delayed_15;
reg [`DWIDTH-1:0] a17_data_delayed_16;
reg [`DWIDTH-1:0] a17_data_delayed_17;
reg [`DWIDTH-1:0] a18_data_delayed_1;
reg [`DWIDTH-1:0] a18_data_delayed_2;
reg [`DWIDTH-1:0] a18_data_delayed_3;
reg [`DWIDTH-1:0] a18_data_delayed_4;
reg [`DWIDTH-1:0] a18_data_delayed_5;
reg [`DWIDTH-1:0] a18_data_delayed_6;
reg [`DWIDTH-1:0] a18_data_delayed_7;
reg [`DWIDTH-1:0] a18_data_delayed_8;
reg [`DWIDTH-1:0] a18_data_delayed_9;
reg [`DWIDTH-1:0] a18_data_delayed_10;
reg [`DWIDTH-1:0] a18_data_delayed_11;
reg [`DWIDTH-1:0] a18_data_delayed_12;
reg [`DWIDTH-1:0] a18_data_delayed_13;
reg [`DWIDTH-1:0] a18_data_delayed_14;
reg [`DWIDTH-1:0] a18_data_delayed_15;
reg [`DWIDTH-1:0] a18_data_delayed_16;
reg [`DWIDTH-1:0] a18_data_delayed_17;
reg [`DWIDTH-1:0] a18_data_delayed_18;
reg [`DWIDTH-1:0] a19_data_delayed_1;
reg [`DWIDTH-1:0] a19_data_delayed_2;
reg [`DWIDTH-1:0] a19_data_delayed_3;
reg [`DWIDTH-1:0] a19_data_delayed_4;
reg [`DWIDTH-1:0] a19_data_delayed_5;
reg [`DWIDTH-1:0] a19_data_delayed_6;
reg [`DWIDTH-1:0] a19_data_delayed_7;
reg [`DWIDTH-1:0] a19_data_delayed_8;
reg [`DWIDTH-1:0] a19_data_delayed_9;
reg [`DWIDTH-1:0] a19_data_delayed_10;
reg [`DWIDTH-1:0] a19_data_delayed_11;
reg [`DWIDTH-1:0] a19_data_delayed_12;
reg [`DWIDTH-1:0] a19_data_delayed_13;
reg [`DWIDTH-1:0] a19_data_delayed_14;
reg [`DWIDTH-1:0] a19_data_delayed_15;
reg [`DWIDTH-1:0] a19_data_delayed_16;
reg [`DWIDTH-1:0] a19_data_delayed_17;
reg [`DWIDTH-1:0] a19_data_delayed_18;
reg [`DWIDTH-1:0] a19_data_delayed_19;


always @(posedge clk) begin
  if (reset || ~start_mat_mul || clk_cnt==0) begin
    a1_data_delayed_1 <= 0;
    a2_data_delayed_1 <= 0;
    a2_data_delayed_2 <= 0;
    a3_data_delayed_1 <= 0;
    a3_data_delayed_2 <= 0;
    a3_data_delayed_3 <= 0;
    a4_data_delayed_1 <= 0;
    a4_data_delayed_2 <= 0;
    a4_data_delayed_3 <= 0;
    a4_data_delayed_4 <= 0;
    a5_data_delayed_1 <= 0;
    a5_data_delayed_2 <= 0;
    a5_data_delayed_3 <= 0;
    a5_data_delayed_4 <= 0;
    a5_data_delayed_5 <= 0;
    a6_data_delayed_1 <= 0;
    a6_data_delayed_2 <= 0;
    a6_data_delayed_3 <= 0;
    a6_data_delayed_4 <= 0;
    a6_data_delayed_5 <= 0;
    a6_data_delayed_6 <= 0;
    a7_data_delayed_1 <= 0;
    a7_data_delayed_2 <= 0;
    a7_data_delayed_3 <= 0;
    a7_data_delayed_4 <= 0;
    a7_data_delayed_5 <= 0;
    a7_data_delayed_6 <= 0;
    a7_data_delayed_7 <= 0;
    a8_data_delayed_1 <= 0;
    a8_data_delayed_2 <= 0;
    a8_data_delayed_3 <= 0;
    a8_data_delayed_4 <= 0;
    a8_data_delayed_5 <= 0;
    a8_data_delayed_6 <= 0;
    a8_data_delayed_7 <= 0;
    a8_data_delayed_8 <= 0;
    a9_data_delayed_1 <= 0;
    a9_data_delayed_2 <= 0;
    a9_data_delayed_3 <= 0;
    a9_data_delayed_4 <= 0;
    a9_data_delayed_5 <= 0;
    a9_data_delayed_6 <= 0;
    a9_data_delayed_7 <= 0;
    a9_data_delayed_8 <= 0;
    a9_data_delayed_9 <= 0;
    a10_data_delayed_1 <= 0;
    a10_data_delayed_2 <= 0;
    a10_data_delayed_3 <= 0;
    a10_data_delayed_4 <= 0;
    a10_data_delayed_5 <= 0;
    a10_data_delayed_6 <= 0;
    a10_data_delayed_7 <= 0;
    a10_data_delayed_8 <= 0;
    a10_data_delayed_9 <= 0;
    a10_data_delayed_10 <= 0;
    a11_data_delayed_1 <= 0;
    a11_data_delayed_2 <= 0;
    a11_data_delayed_3 <= 0;
    a11_data_delayed_4 <= 0;
    a11_data_delayed_5 <= 0;
    a11_data_delayed_6 <= 0;
    a11_data_delayed_7 <= 0;
    a11_data_delayed_8 <= 0;
    a11_data_delayed_9 <= 0;
    a11_data_delayed_10 <= 0;
    a11_data_delayed_11 <= 0;
    a12_data_delayed_1 <= 0;
    a12_data_delayed_2 <= 0;
    a12_data_delayed_3 <= 0;
    a12_data_delayed_4 <= 0;
    a12_data_delayed_5 <= 0;
    a12_data_delayed_6 <= 0;
    a12_data_delayed_7 <= 0;
    a12_data_delayed_8 <= 0;
    a12_data_delayed_9 <= 0;
    a12_data_delayed_10 <= 0;
    a12_data_delayed_11 <= 0;
    a12_data_delayed_12 <= 0;
    a13_data_delayed_1 <= 0;
    a13_data_delayed_2 <= 0;
    a13_data_delayed_3 <= 0;
    a13_data_delayed_4 <= 0;
    a13_data_delayed_5 <= 0;
    a13_data_delayed_6 <= 0;
    a13_data_delayed_7 <= 0;
    a13_data_delayed_8 <= 0;
    a13_data_delayed_9 <= 0;
    a13_data_delayed_10 <= 0;
    a13_data_delayed_11 <= 0;
    a13_data_delayed_12 <= 0;
    a13_data_delayed_13 <= 0;
    a14_data_delayed_1 <= 0;
    a14_data_delayed_2 <= 0;
    a14_data_delayed_3 <= 0;
    a14_data_delayed_4 <= 0;
    a14_data_delayed_5 <= 0;
    a14_data_delayed_6 <= 0;
    a14_data_delayed_7 <= 0;
    a14_data_delayed_8 <= 0;
    a14_data_delayed_9 <= 0;
    a14_data_delayed_10 <= 0;
    a14_data_delayed_11 <= 0;
    a14_data_delayed_12 <= 0;
    a14_data_delayed_13 <= 0;
    a14_data_delayed_14 <= 0;
    a15_data_delayed_1 <= 0;
    a15_data_delayed_2 <= 0;
    a15_data_delayed_3 <= 0;
    a15_data_delayed_4 <= 0;
    a15_data_delayed_5 <= 0;
    a15_data_delayed_6 <= 0;
    a15_data_delayed_7 <= 0;
    a15_data_delayed_8 <= 0;
    a15_data_delayed_9 <= 0;
    a15_data_delayed_10 <= 0;
    a15_data_delayed_11 <= 0;
    a15_data_delayed_12 <= 0;
    a15_data_delayed_13 <= 0;
    a15_data_delayed_14 <= 0;
    a15_data_delayed_15 <= 0;
    a16_data_delayed_1 <= 0;
    a16_data_delayed_2 <= 0;
    a16_data_delayed_3 <= 0;
    a16_data_delayed_4 <= 0;
    a16_data_delayed_5 <= 0;
    a16_data_delayed_6 <= 0;
    a16_data_delayed_7 <= 0;
    a16_data_delayed_8 <= 0;
    a16_data_delayed_9 <= 0;
    a16_data_delayed_10 <= 0;
    a16_data_delayed_11 <= 0;
    a16_data_delayed_12 <= 0;
    a16_data_delayed_13 <= 0;
    a16_data_delayed_14 <= 0;
    a16_data_delayed_15 <= 0;
    a16_data_delayed_16 <= 0;
    a17_data_delayed_1 <= 0;
    a17_data_delayed_2 <= 0;
    a17_data_delayed_3 <= 0;
    a17_data_delayed_4 <= 0;
    a17_data_delayed_5 <= 0;
    a17_data_delayed_6 <= 0;
    a17_data_delayed_7 <= 0;
    a17_data_delayed_8 <= 0;
    a17_data_delayed_9 <= 0;
    a17_data_delayed_10 <= 0;
    a17_data_delayed_11 <= 0;
    a17_data_delayed_12 <= 0;
    a17_data_delayed_13 <= 0;
    a17_data_delayed_14 <= 0;
    a17_data_delayed_15 <= 0;
    a17_data_delayed_16 <= 0;
    a17_data_delayed_17 <= 0;
    a18_data_delayed_1 <= 0;
    a18_data_delayed_2 <= 0;
    a18_data_delayed_3 <= 0;
    a18_data_delayed_4 <= 0;
    a18_data_delayed_5 <= 0;
    a18_data_delayed_6 <= 0;
    a18_data_delayed_7 <= 0;
    a18_data_delayed_8 <= 0;
    a18_data_delayed_9 <= 0;
    a18_data_delayed_10 <= 0;
    a18_data_delayed_11 <= 0;
    a18_data_delayed_12 <= 0;
    a18_data_delayed_13 <= 0;
    a18_data_delayed_14 <= 0;
    a18_data_delayed_15 <= 0;
    a18_data_delayed_16 <= 0;
    a18_data_delayed_17 <= 0;
    a18_data_delayed_18 <= 0;
    a19_data_delayed_1 <= 0;
    a19_data_delayed_2 <= 0;
    a19_data_delayed_3 <= 0;
    a19_data_delayed_4 <= 0;
    a19_data_delayed_5 <= 0;
    a19_data_delayed_6 <= 0;
    a19_data_delayed_7 <= 0;
    a19_data_delayed_8 <= 0;
    a19_data_delayed_9 <= 0;
    a19_data_delayed_10 <= 0;
    a19_data_delayed_11 <= 0;
    a19_data_delayed_12 <= 0;
    a19_data_delayed_13 <= 0;
    a19_data_delayed_14 <= 0;
    a19_data_delayed_15 <= 0;
    a19_data_delayed_16 <= 0;
    a19_data_delayed_17 <= 0;
    a19_data_delayed_18 <= 0;
    a19_data_delayed_19 <= 0;

  end
  else begin
  a1_data_delayed_1 <= a1_data;
  a2_data_delayed_1 <= a2_data;
  a3_data_delayed_1 <= a3_data;
  a4_data_delayed_1 <= a4_data;
  a5_data_delayed_1 <= a5_data;
  a6_data_delayed_1 <= a6_data;
  a7_data_delayed_1 <= a7_data;
  a8_data_delayed_1 <= a8_data;
  a9_data_delayed_1 <= a9_data;
  a10_data_delayed_1 <= a10_data;
  a11_data_delayed_1 <= a11_data;
  a12_data_delayed_1 <= a12_data;
  a13_data_delayed_1 <= a13_data;
  a14_data_delayed_1 <= a14_data;
  a15_data_delayed_1 <= a15_data;
  a16_data_delayed_1 <= a16_data;
  a17_data_delayed_1 <= a17_data;
  a18_data_delayed_1 <= a18_data;
  a19_data_delayed_1 <= a19_data;
  a2_data_delayed_2 <= a2_data_delayed_1;
  a3_data_delayed_2 <= a3_data_delayed_1;
  a3_data_delayed_3 <= a3_data_delayed_2;
  a4_data_delayed_2 <= a4_data_delayed_1;
  a4_data_delayed_3 <= a4_data_delayed_2;
  a4_data_delayed_4 <= a4_data_delayed_3;
  a5_data_delayed_2 <= a5_data_delayed_1;
  a5_data_delayed_3 <= a5_data_delayed_2;
  a5_data_delayed_4 <= a5_data_delayed_3;
  a5_data_delayed_5 <= a5_data_delayed_4;
  a6_data_delayed_2 <= a6_data_delayed_1;
  a6_data_delayed_3 <= a6_data_delayed_2;
  a6_data_delayed_4 <= a6_data_delayed_3;
  a6_data_delayed_5 <= a6_data_delayed_4;
  a6_data_delayed_6 <= a6_data_delayed_5;
  a7_data_delayed_2 <= a7_data_delayed_1;
  a7_data_delayed_3 <= a7_data_delayed_2;
  a7_data_delayed_4 <= a7_data_delayed_3;
  a7_data_delayed_5 <= a7_data_delayed_4;
  a7_data_delayed_6 <= a7_data_delayed_5;
  a7_data_delayed_7 <= a7_data_delayed_6;
  a8_data_delayed_2 <= a8_data_delayed_1;
  a8_data_delayed_3 <= a8_data_delayed_2;
  a8_data_delayed_4 <= a8_data_delayed_3;
  a8_data_delayed_5 <= a8_data_delayed_4;
  a8_data_delayed_6 <= a8_data_delayed_5;
  a8_data_delayed_7 <= a8_data_delayed_6;
  a8_data_delayed_8 <= a8_data_delayed_7;
  a9_data_delayed_2 <= a9_data_delayed_1;
  a9_data_delayed_3 <= a9_data_delayed_2;
  a9_data_delayed_4 <= a9_data_delayed_3;
  a9_data_delayed_5 <= a9_data_delayed_4;
  a9_data_delayed_6 <= a9_data_delayed_5;
  a9_data_delayed_7 <= a9_data_delayed_6;
  a9_data_delayed_8 <= a9_data_delayed_7;
  a9_data_delayed_9 <= a9_data_delayed_8;
  a10_data_delayed_2 <= a10_data_delayed_1;
  a10_data_delayed_3 <= a10_data_delayed_2;
  a10_data_delayed_4 <= a10_data_delayed_3;
  a10_data_delayed_5 <= a10_data_delayed_4;
  a10_data_delayed_6 <= a10_data_delayed_5;
  a10_data_delayed_7 <= a10_data_delayed_6;
  a10_data_delayed_8 <= a10_data_delayed_7;
  a10_data_delayed_9 <= a10_data_delayed_8;
  a10_data_delayed_10 <= a10_data_delayed_9;
  a11_data_delayed_2 <= a11_data_delayed_1;
  a11_data_delayed_3 <= a11_data_delayed_2;
  a11_data_delayed_4 <= a11_data_delayed_3;
  a11_data_delayed_5 <= a11_data_delayed_4;
  a11_data_delayed_6 <= a11_data_delayed_5;
  a11_data_delayed_7 <= a11_data_delayed_6;
  a11_data_delayed_8 <= a11_data_delayed_7;
  a11_data_delayed_9 <= a11_data_delayed_8;
  a11_data_delayed_10 <= a11_data_delayed_9;
  a11_data_delayed_11 <= a11_data_delayed_10;
  a12_data_delayed_2 <= a12_data_delayed_1;
  a12_data_delayed_3 <= a12_data_delayed_2;
  a12_data_delayed_4 <= a12_data_delayed_3;
  a12_data_delayed_5 <= a12_data_delayed_4;
  a12_data_delayed_6 <= a12_data_delayed_5;
  a12_data_delayed_7 <= a12_data_delayed_6;
  a12_data_delayed_8 <= a12_data_delayed_7;
  a12_data_delayed_9 <= a12_data_delayed_8;
  a12_data_delayed_10 <= a12_data_delayed_9;
  a12_data_delayed_11 <= a12_data_delayed_10;
  a12_data_delayed_12 <= a12_data_delayed_11;
  a13_data_delayed_2 <= a13_data_delayed_1;
  a13_data_delayed_3 <= a13_data_delayed_2;
  a13_data_delayed_4 <= a13_data_delayed_3;
  a13_data_delayed_5 <= a13_data_delayed_4;
  a13_data_delayed_6 <= a13_data_delayed_5;
  a13_data_delayed_7 <= a13_data_delayed_6;
  a13_data_delayed_8 <= a13_data_delayed_7;
  a13_data_delayed_9 <= a13_data_delayed_8;
  a13_data_delayed_10 <= a13_data_delayed_9;
  a13_data_delayed_11 <= a13_data_delayed_10;
  a13_data_delayed_12 <= a13_data_delayed_11;
  a13_data_delayed_13 <= a13_data_delayed_12;
  a14_data_delayed_2 <= a14_data_delayed_1;
  a14_data_delayed_3 <= a14_data_delayed_2;
  a14_data_delayed_4 <= a14_data_delayed_3;
  a14_data_delayed_5 <= a14_data_delayed_4;
  a14_data_delayed_6 <= a14_data_delayed_5;
  a14_data_delayed_7 <= a14_data_delayed_6;
  a14_data_delayed_8 <= a14_data_delayed_7;
  a14_data_delayed_9 <= a14_data_delayed_8;
  a14_data_delayed_10 <= a14_data_delayed_9;
  a14_data_delayed_11 <= a14_data_delayed_10;
  a14_data_delayed_12 <= a14_data_delayed_11;
  a14_data_delayed_13 <= a14_data_delayed_12;
  a14_data_delayed_14 <= a14_data_delayed_13;
  a15_data_delayed_2 <= a15_data_delayed_1;
  a15_data_delayed_3 <= a15_data_delayed_2;
  a15_data_delayed_4 <= a15_data_delayed_3;
  a15_data_delayed_5 <= a15_data_delayed_4;
  a15_data_delayed_6 <= a15_data_delayed_5;
  a15_data_delayed_7 <= a15_data_delayed_6;
  a15_data_delayed_8 <= a15_data_delayed_7;
  a15_data_delayed_9 <= a15_data_delayed_8;
  a15_data_delayed_10 <= a15_data_delayed_9;
  a15_data_delayed_11 <= a15_data_delayed_10;
  a15_data_delayed_12 <= a15_data_delayed_11;
  a15_data_delayed_13 <= a15_data_delayed_12;
  a15_data_delayed_14 <= a15_data_delayed_13;
  a15_data_delayed_15 <= a15_data_delayed_14;
  a16_data_delayed_2 <= a16_data_delayed_1;
  a16_data_delayed_3 <= a16_data_delayed_2;
  a16_data_delayed_4 <= a16_data_delayed_3;
  a16_data_delayed_5 <= a16_data_delayed_4;
  a16_data_delayed_6 <= a16_data_delayed_5;
  a16_data_delayed_7 <= a16_data_delayed_6;
  a16_data_delayed_8 <= a16_data_delayed_7;
  a16_data_delayed_9 <= a16_data_delayed_8;
  a16_data_delayed_10 <= a16_data_delayed_9;
  a16_data_delayed_11 <= a16_data_delayed_10;
  a16_data_delayed_12 <= a16_data_delayed_11;
  a16_data_delayed_13 <= a16_data_delayed_12;
  a16_data_delayed_14 <= a16_data_delayed_13;
  a16_data_delayed_15 <= a16_data_delayed_14;
  a16_data_delayed_16 <= a16_data_delayed_15;
  a17_data_delayed_2 <= a17_data_delayed_1;
  a17_data_delayed_3 <= a17_data_delayed_2;
  a17_data_delayed_4 <= a17_data_delayed_3;
  a17_data_delayed_5 <= a17_data_delayed_4;
  a17_data_delayed_6 <= a17_data_delayed_5;
  a17_data_delayed_7 <= a17_data_delayed_6;
  a17_data_delayed_8 <= a17_data_delayed_7;
  a17_data_delayed_9 <= a17_data_delayed_8;
  a17_data_delayed_10 <= a17_data_delayed_9;
  a17_data_delayed_11 <= a17_data_delayed_10;
  a17_data_delayed_12 <= a17_data_delayed_11;
  a17_data_delayed_13 <= a17_data_delayed_12;
  a17_data_delayed_14 <= a17_data_delayed_13;
  a17_data_delayed_15 <= a17_data_delayed_14;
  a17_data_delayed_16 <= a17_data_delayed_15;
  a17_data_delayed_17 <= a17_data_delayed_16;
  a18_data_delayed_2 <= a18_data_delayed_1;
  a18_data_delayed_3 <= a18_data_delayed_2;
  a18_data_delayed_4 <= a18_data_delayed_3;
  a18_data_delayed_5 <= a18_data_delayed_4;
  a18_data_delayed_6 <= a18_data_delayed_5;
  a18_data_delayed_7 <= a18_data_delayed_6;
  a18_data_delayed_8 <= a18_data_delayed_7;
  a18_data_delayed_9 <= a18_data_delayed_8;
  a18_data_delayed_10 <= a18_data_delayed_9;
  a18_data_delayed_11 <= a18_data_delayed_10;
  a18_data_delayed_12 <= a18_data_delayed_11;
  a18_data_delayed_13 <= a18_data_delayed_12;
  a18_data_delayed_14 <= a18_data_delayed_13;
  a18_data_delayed_15 <= a18_data_delayed_14;
  a18_data_delayed_16 <= a18_data_delayed_15;
  a18_data_delayed_17 <= a18_data_delayed_16;
  a18_data_delayed_18 <= a18_data_delayed_17;
  a19_data_delayed_2 <= a19_data_delayed_1;
  a19_data_delayed_3 <= a19_data_delayed_2;
  a19_data_delayed_4 <= a19_data_delayed_3;
  a19_data_delayed_5 <= a19_data_delayed_4;
  a19_data_delayed_6 <= a19_data_delayed_5;
  a19_data_delayed_7 <= a19_data_delayed_6;
  a19_data_delayed_8 <= a19_data_delayed_7;
  a19_data_delayed_9 <= a19_data_delayed_8;
  a19_data_delayed_10 <= a19_data_delayed_9;
  a19_data_delayed_11 <= a19_data_delayed_10;
  a19_data_delayed_12 <= a19_data_delayed_11;
  a19_data_delayed_13 <= a19_data_delayed_12;
  a19_data_delayed_14 <= a19_data_delayed_13;
  a19_data_delayed_15 <= a19_data_delayed_14;
  a19_data_delayed_16 <= a19_data_delayed_15;
  a19_data_delayed_17 <= a19_data_delayed_16;
  a19_data_delayed_18 <= a19_data_delayed_17;
  a19_data_delayed_19 <= a19_data_delayed_18;
 
  end
end

//////////////////////////////////////////////////////////////////////////
// Logic to generate addresses to BRAM B
//////////////////////////////////////////////////////////////////////////
reg [`AWIDTH-1:0] b_addr;
reg b_mem_access; //flag that tells whether the matmul is trying to access memory or not
always @(posedge clk) begin
  //else if (clk_cnt >= b_loc*`MAT_MUL_SIZE+final_mat_mul_size) begin
  //Writing the line above to avoid multiplication:

  if ((reset || ~start_mat_mul) || (clk_cnt >= (b_loc<<`LOG2_MAT_MUL_SIZE)+20)) begin

      b_addr <= address_mat_b - address_stride_b;
  
    b_mem_access <= 0;
  end
  //else if ((clk_cnt >= b_loc*`MAT_MUL_SIZE) && (clk_cnt < b_loc*`MAT_MUL_SIZE+final_mat_mul_size)) begin
  //Writing the line above to avoid multiplication:

  else if ((clk_cnt >= (b_loc<<`LOG2_MAT_MUL_SIZE)) && (clk_cnt < (b_loc<<`LOG2_MAT_MUL_SIZE)+20)) begin

      b_addr <= b_addr + address_stride_b;
  
    b_mem_access <= 1;
  end
end 

//////////////////////////////////////////////////////////////////////////
// Logic to generate valid signals for data coming from BRAM B
//////////////////////////////////////////////////////////////////////////
reg [7:0] b_mem_access_counter;
always @(posedge clk) begin
  if (reset || ~start_mat_mul) begin
    b_mem_access_counter <= 0;
  end
  else if (b_mem_access == 1) begin
    b_mem_access_counter <= b_mem_access_counter + 1;  
  end
  else begin
    b_mem_access_counter <= 0;
  end
end

wire b_data_valid; //flag that tells whether the data from memory is valid
assign b_data_valid = 
     ((validity_mask_b_rows[0]==1'b0 && b_mem_access_counter==1) ||
      (validity_mask_b_rows[1]==1'b0 && b_mem_access_counter==2) ||
      (validity_mask_b_rows[2]==1'b0 && b_mem_access_counter==3) ||
      (validity_mask_b_rows[3]==1'b0 && b_mem_access_counter==4) ||
      (validity_mask_b_rows[4]==1'b0 && b_mem_access_counter==5) ||
      (validity_mask_b_rows[5]==1'b0 && b_mem_access_counter==6) ||
      (validity_mask_b_rows[6]==1'b0 && b_mem_access_counter==7) ||
      (validity_mask_b_rows[7]==1'b0 && b_mem_access_counter==8) ||
      (validity_mask_b_rows[8]==1'b0 && b_mem_access_counter==9) ||
      (validity_mask_b_rows[9]==1'b0 && b_mem_access_counter==10) ||
      (validity_mask_b_rows[10]==1'b0 && b_mem_access_counter==11) ||
      (validity_mask_b_rows[11]==1'b0 && b_mem_access_counter==12) ||
      (validity_mask_b_rows[12]==1'b0 && b_mem_access_counter==13) ||
      (validity_mask_b_rows[13]==1'b0 && b_mem_access_counter==14) ||
      (validity_mask_b_rows[14]==1'b0 && b_mem_access_counter==15) ||
      (validity_mask_b_rows[15]==1'b0 && b_mem_access_counter==16) ||
      (validity_mask_b_rows[16]==1'b0 && b_mem_access_counter==17) ||
      (validity_mask_b_rows[17]==1'b0 && b_mem_access_counter==18) ||
      (validity_mask_b_rows[18]==1'b0 && b_mem_access_counter==19) ||
      (validity_mask_b_rows[19]==1'b0 && b_mem_access_counter==20)) ?
    
        1'b0 : (b_mem_access_counter >= `MEM_ACCESS_LATENCY);

//////////////////////////////////////////////////////////////////////////
// Logic to delay certain parts of the data received from BRAM B (systolic data setup)
//////////////////////////////////////////////////////////////////////////
assign b0_data = b_data[1*`DWIDTH-1:0*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[0]}};
assign b1_data = b_data[2*`DWIDTH-1:1*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[1]}};
assign b2_data = b_data[3*`DWIDTH-1:2*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[2]}};
assign b3_data = b_data[4*`DWIDTH-1:3*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[3]}};
assign b4_data = b_data[5*`DWIDTH-1:4*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[4]}};
assign b5_data = b_data[6*`DWIDTH-1:5*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[5]}};
assign b6_data = b_data[7*`DWIDTH-1:6*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[6]}};
assign b7_data = b_data[8*`DWIDTH-1:7*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[7]}};
assign b8_data = b_data[9*`DWIDTH-1:8*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[8]}};
assign b9_data = b_data[10*`DWIDTH-1:9*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[9]}};
assign b10_data = b_data[11*`DWIDTH-1:10*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[10]}};
assign b11_data = b_data[12*`DWIDTH-1:11*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[11]}};
assign b12_data = b_data[13*`DWIDTH-1:12*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[12]}};
assign b13_data = b_data[14*`DWIDTH-1:13*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[13]}};
assign b14_data = b_data[15*`DWIDTH-1:14*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[14]}};
assign b15_data = b_data[16*`DWIDTH-1:15*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[15]}};
assign b16_data = b_data[17*`DWIDTH-1:16*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[16]}};
assign b17_data = b_data[18*`DWIDTH-1:17*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[17]}};
assign b18_data = b_data[19*`DWIDTH-1:18*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[18]}};
assign b19_data = b_data[20*`DWIDTH-1:19*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[19]}};

reg [`DWIDTH-1:0] b1_data_delayed_1;
reg [`DWIDTH-1:0] b2_data_delayed_1;
reg [`DWIDTH-1:0] b2_data_delayed_2;
reg [`DWIDTH-1:0] b3_data_delayed_1;
reg [`DWIDTH-1:0] b3_data_delayed_2;
reg [`DWIDTH-1:0] b3_data_delayed_3;
reg [`DWIDTH-1:0] b4_data_delayed_1;
reg [`DWIDTH-1:0] b4_data_delayed_2;
reg [`DWIDTH-1:0] b4_data_delayed_3;
reg [`DWIDTH-1:0] b4_data_delayed_4;
reg [`DWIDTH-1:0] b5_data_delayed_1;
reg [`DWIDTH-1:0] b5_data_delayed_2;
reg [`DWIDTH-1:0] b5_data_delayed_3;
reg [`DWIDTH-1:0] b5_data_delayed_4;
reg [`DWIDTH-1:0] b5_data_delayed_5;
reg [`DWIDTH-1:0] b6_data_delayed_1;
reg [`DWIDTH-1:0] b6_data_delayed_2;
reg [`DWIDTH-1:0] b6_data_delayed_3;
reg [`DWIDTH-1:0] b6_data_delayed_4;
reg [`DWIDTH-1:0] b6_data_delayed_5;
reg [`DWIDTH-1:0] b6_data_delayed_6;
reg [`DWIDTH-1:0] b7_data_delayed_1;
reg [`DWIDTH-1:0] b7_data_delayed_2;
reg [`DWIDTH-1:0] b7_data_delayed_3;
reg [`DWIDTH-1:0] b7_data_delayed_4;
reg [`DWIDTH-1:0] b7_data_delayed_5;
reg [`DWIDTH-1:0] b7_data_delayed_6;
reg [`DWIDTH-1:0] b7_data_delayed_7;
reg [`DWIDTH-1:0] b8_data_delayed_1;
reg [`DWIDTH-1:0] b8_data_delayed_2;
reg [`DWIDTH-1:0] b8_data_delayed_3;
reg [`DWIDTH-1:0] b8_data_delayed_4;
reg [`DWIDTH-1:0] b8_data_delayed_5;
reg [`DWIDTH-1:0] b8_data_delayed_6;
reg [`DWIDTH-1:0] b8_data_delayed_7;
reg [`DWIDTH-1:0] b8_data_delayed_8;
reg [`DWIDTH-1:0] b9_data_delayed_1;
reg [`DWIDTH-1:0] b9_data_delayed_2;
reg [`DWIDTH-1:0] b9_data_delayed_3;
reg [`DWIDTH-1:0] b9_data_delayed_4;
reg [`DWIDTH-1:0] b9_data_delayed_5;
reg [`DWIDTH-1:0] b9_data_delayed_6;
reg [`DWIDTH-1:0] b9_data_delayed_7;
reg [`DWIDTH-1:0] b9_data_delayed_8;
reg [`DWIDTH-1:0] b9_data_delayed_9;
reg [`DWIDTH-1:0] b10_data_delayed_1;
reg [`DWIDTH-1:0] b10_data_delayed_2;
reg [`DWIDTH-1:0] b10_data_delayed_3;
reg [`DWIDTH-1:0] b10_data_delayed_4;
reg [`DWIDTH-1:0] b10_data_delayed_5;
reg [`DWIDTH-1:0] b10_data_delayed_6;
reg [`DWIDTH-1:0] b10_data_delayed_7;
reg [`DWIDTH-1:0] b10_data_delayed_8;
reg [`DWIDTH-1:0] b10_data_delayed_9;
reg [`DWIDTH-1:0] b10_data_delayed_10;
reg [`DWIDTH-1:0] b11_data_delayed_1;
reg [`DWIDTH-1:0] b11_data_delayed_2;
reg [`DWIDTH-1:0] b11_data_delayed_3;
reg [`DWIDTH-1:0] b11_data_delayed_4;
reg [`DWIDTH-1:0] b11_data_delayed_5;
reg [`DWIDTH-1:0] b11_data_delayed_6;
reg [`DWIDTH-1:0] b11_data_delayed_7;
reg [`DWIDTH-1:0] b11_data_delayed_8;
reg [`DWIDTH-1:0] b11_data_delayed_9;
reg [`DWIDTH-1:0] b11_data_delayed_10;
reg [`DWIDTH-1:0] b11_data_delayed_11;
reg [`DWIDTH-1:0] b12_data_delayed_1;
reg [`DWIDTH-1:0] b12_data_delayed_2;
reg [`DWIDTH-1:0] b12_data_delayed_3;
reg [`DWIDTH-1:0] b12_data_delayed_4;
reg [`DWIDTH-1:0] b12_data_delayed_5;
reg [`DWIDTH-1:0] b12_data_delayed_6;
reg [`DWIDTH-1:0] b12_data_delayed_7;
reg [`DWIDTH-1:0] b12_data_delayed_8;
reg [`DWIDTH-1:0] b12_data_delayed_9;
reg [`DWIDTH-1:0] b12_data_delayed_10;
reg [`DWIDTH-1:0] b12_data_delayed_11;
reg [`DWIDTH-1:0] b12_data_delayed_12;
reg [`DWIDTH-1:0] b13_data_delayed_1;
reg [`DWIDTH-1:0] b13_data_delayed_2;
reg [`DWIDTH-1:0] b13_data_delayed_3;
reg [`DWIDTH-1:0] b13_data_delayed_4;
reg [`DWIDTH-1:0] b13_data_delayed_5;
reg [`DWIDTH-1:0] b13_data_delayed_6;
reg [`DWIDTH-1:0] b13_data_delayed_7;
reg [`DWIDTH-1:0] b13_data_delayed_8;
reg [`DWIDTH-1:0] b13_data_delayed_9;
reg [`DWIDTH-1:0] b13_data_delayed_10;
reg [`DWIDTH-1:0] b13_data_delayed_11;
reg [`DWIDTH-1:0] b13_data_delayed_12;
reg [`DWIDTH-1:0] b13_data_delayed_13;
reg [`DWIDTH-1:0] b14_data_delayed_1;
reg [`DWIDTH-1:0] b14_data_delayed_2;
reg [`DWIDTH-1:0] b14_data_delayed_3;
reg [`DWIDTH-1:0] b14_data_delayed_4;
reg [`DWIDTH-1:0] b14_data_delayed_5;
reg [`DWIDTH-1:0] b14_data_delayed_6;
reg [`DWIDTH-1:0] b14_data_delayed_7;
reg [`DWIDTH-1:0] b14_data_delayed_8;
reg [`DWIDTH-1:0] b14_data_delayed_9;
reg [`DWIDTH-1:0] b14_data_delayed_10;
reg [`DWIDTH-1:0] b14_data_delayed_11;
reg [`DWIDTH-1:0] b14_data_delayed_12;
reg [`DWIDTH-1:0] b14_data_delayed_13;
reg [`DWIDTH-1:0] b14_data_delayed_14;
reg [`DWIDTH-1:0] b15_data_delayed_1;
reg [`DWIDTH-1:0] b15_data_delayed_2;
reg [`DWIDTH-1:0] b15_data_delayed_3;
reg [`DWIDTH-1:0] b15_data_delayed_4;
reg [`DWIDTH-1:0] b15_data_delayed_5;
reg [`DWIDTH-1:0] b15_data_delayed_6;
reg [`DWIDTH-1:0] b15_data_delayed_7;
reg [`DWIDTH-1:0] b15_data_delayed_8;
reg [`DWIDTH-1:0] b15_data_delayed_9;
reg [`DWIDTH-1:0] b15_data_delayed_10;
reg [`DWIDTH-1:0] b15_data_delayed_11;
reg [`DWIDTH-1:0] b15_data_delayed_12;
reg [`DWIDTH-1:0] b15_data_delayed_13;
reg [`DWIDTH-1:0] b15_data_delayed_14;
reg [`DWIDTH-1:0] b15_data_delayed_15;
reg [`DWIDTH-1:0] b16_data_delayed_1;
reg [`DWIDTH-1:0] b16_data_delayed_2;
reg [`DWIDTH-1:0] b16_data_delayed_3;
reg [`DWIDTH-1:0] b16_data_delayed_4;
reg [`DWIDTH-1:0] b16_data_delayed_5;
reg [`DWIDTH-1:0] b16_data_delayed_6;
reg [`DWIDTH-1:0] b16_data_delayed_7;
reg [`DWIDTH-1:0] b16_data_delayed_8;
reg [`DWIDTH-1:0] b16_data_delayed_9;
reg [`DWIDTH-1:0] b16_data_delayed_10;
reg [`DWIDTH-1:0] b16_data_delayed_11;
reg [`DWIDTH-1:0] b16_data_delayed_12;
reg [`DWIDTH-1:0] b16_data_delayed_13;
reg [`DWIDTH-1:0] b16_data_delayed_14;
reg [`DWIDTH-1:0] b16_data_delayed_15;
reg [`DWIDTH-1:0] b16_data_delayed_16;
reg [`DWIDTH-1:0] b17_data_delayed_1;
reg [`DWIDTH-1:0] b17_data_delayed_2;
reg [`DWIDTH-1:0] b17_data_delayed_3;
reg [`DWIDTH-1:0] b17_data_delayed_4;
reg [`DWIDTH-1:0] b17_data_delayed_5;
reg [`DWIDTH-1:0] b17_data_delayed_6;
reg [`DWIDTH-1:0] b17_data_delayed_7;
reg [`DWIDTH-1:0] b17_data_delayed_8;
reg [`DWIDTH-1:0] b17_data_delayed_9;
reg [`DWIDTH-1:0] b17_data_delayed_10;
reg [`DWIDTH-1:0] b17_data_delayed_11;
reg [`DWIDTH-1:0] b17_data_delayed_12;
reg [`DWIDTH-1:0] b17_data_delayed_13;
reg [`DWIDTH-1:0] b17_data_delayed_14;
reg [`DWIDTH-1:0] b17_data_delayed_15;
reg [`DWIDTH-1:0] b17_data_delayed_16;
reg [`DWIDTH-1:0] b17_data_delayed_17;
reg [`DWIDTH-1:0] b18_data_delayed_1;
reg [`DWIDTH-1:0] b18_data_delayed_2;
reg [`DWIDTH-1:0] b18_data_delayed_3;
reg [`DWIDTH-1:0] b18_data_delayed_4;
reg [`DWIDTH-1:0] b18_data_delayed_5;
reg [`DWIDTH-1:0] b18_data_delayed_6;
reg [`DWIDTH-1:0] b18_data_delayed_7;
reg [`DWIDTH-1:0] b18_data_delayed_8;
reg [`DWIDTH-1:0] b18_data_delayed_9;
reg [`DWIDTH-1:0] b18_data_delayed_10;
reg [`DWIDTH-1:0] b18_data_delayed_11;
reg [`DWIDTH-1:0] b18_data_delayed_12;
reg [`DWIDTH-1:0] b18_data_delayed_13;
reg [`DWIDTH-1:0] b18_data_delayed_14;
reg [`DWIDTH-1:0] b18_data_delayed_15;
reg [`DWIDTH-1:0] b18_data_delayed_16;
reg [`DWIDTH-1:0] b18_data_delayed_17;
reg [`DWIDTH-1:0] b18_data_delayed_18;
reg [`DWIDTH-1:0] b19_data_delayed_1;
reg [`DWIDTH-1:0] b19_data_delayed_2;
reg [`DWIDTH-1:0] b19_data_delayed_3;
reg [`DWIDTH-1:0] b19_data_delayed_4;
reg [`DWIDTH-1:0] b19_data_delayed_5;
reg [`DWIDTH-1:0] b19_data_delayed_6;
reg [`DWIDTH-1:0] b19_data_delayed_7;
reg [`DWIDTH-1:0] b19_data_delayed_8;
reg [`DWIDTH-1:0] b19_data_delayed_9;
reg [`DWIDTH-1:0] b19_data_delayed_10;
reg [`DWIDTH-1:0] b19_data_delayed_11;
reg [`DWIDTH-1:0] b19_data_delayed_12;
reg [`DWIDTH-1:0] b19_data_delayed_13;
reg [`DWIDTH-1:0] b19_data_delayed_14;
reg [`DWIDTH-1:0] b19_data_delayed_15;
reg [`DWIDTH-1:0] b19_data_delayed_16;
reg [`DWIDTH-1:0] b19_data_delayed_17;
reg [`DWIDTH-1:0] b19_data_delayed_18;
reg [`DWIDTH-1:0] b19_data_delayed_19;


always @(posedge clk) begin
  if (reset || ~start_mat_mul || clk_cnt==0) begin
    b1_data_delayed_1 <= 0;
    b2_data_delayed_1 <= 0;
    b2_data_delayed_2 <= 0;
    b3_data_delayed_1 <= 0;
    b3_data_delayed_2 <= 0;
    b3_data_delayed_3 <= 0;
    b4_data_delayed_1 <= 0;
    b4_data_delayed_2 <= 0;
    b4_data_delayed_3 <= 0;
    b4_data_delayed_4 <= 0;
    b5_data_delayed_1 <= 0;
    b5_data_delayed_2 <= 0;
    b5_data_delayed_3 <= 0;
    b5_data_delayed_4 <= 0;
    b5_data_delayed_5 <= 0;
    b6_data_delayed_1 <= 0;
    b6_data_delayed_2 <= 0;
    b6_data_delayed_3 <= 0;
    b6_data_delayed_4 <= 0;
    b6_data_delayed_5 <= 0;
    b6_data_delayed_6 <= 0;
    b7_data_delayed_1 <= 0;
    b7_data_delayed_2 <= 0;
    b7_data_delayed_3 <= 0;
    b7_data_delayed_4 <= 0;
    b7_data_delayed_5 <= 0;
    b7_data_delayed_6 <= 0;
    b7_data_delayed_7 <= 0;
    b8_data_delayed_1 <= 0;
    b8_data_delayed_2 <= 0;
    b8_data_delayed_3 <= 0;
    b8_data_delayed_4 <= 0;
    b8_data_delayed_5 <= 0;
    b8_data_delayed_6 <= 0;
    b8_data_delayed_7 <= 0;
    b8_data_delayed_8 <= 0;
    b9_data_delayed_1 <= 0;
    b9_data_delayed_2 <= 0;
    b9_data_delayed_3 <= 0;
    b9_data_delayed_4 <= 0;
    b9_data_delayed_5 <= 0;
    b9_data_delayed_6 <= 0;
    b9_data_delayed_7 <= 0;
    b9_data_delayed_8 <= 0;
    b9_data_delayed_9 <= 0;
    b10_data_delayed_1 <= 0;
    b10_data_delayed_2 <= 0;
    b10_data_delayed_3 <= 0;
    b10_data_delayed_4 <= 0;
    b10_data_delayed_5 <= 0;
    b10_data_delayed_6 <= 0;
    b10_data_delayed_7 <= 0;
    b10_data_delayed_8 <= 0;
    b10_data_delayed_9 <= 0;
    b10_data_delayed_10 <= 0;
    b11_data_delayed_1 <= 0;
    b11_data_delayed_2 <= 0;
    b11_data_delayed_3 <= 0;
    b11_data_delayed_4 <= 0;
    b11_data_delayed_5 <= 0;
    b11_data_delayed_6 <= 0;
    b11_data_delayed_7 <= 0;
    b11_data_delayed_8 <= 0;
    b11_data_delayed_9 <= 0;
    b11_data_delayed_10 <= 0;
    b11_data_delayed_11 <= 0;
    b12_data_delayed_1 <= 0;
    b12_data_delayed_2 <= 0;
    b12_data_delayed_3 <= 0;
    b12_data_delayed_4 <= 0;
    b12_data_delayed_5 <= 0;
    b12_data_delayed_6 <= 0;
    b12_data_delayed_7 <= 0;
    b12_data_delayed_8 <= 0;
    b12_data_delayed_9 <= 0;
    b12_data_delayed_10 <= 0;
    b12_data_delayed_11 <= 0;
    b12_data_delayed_12 <= 0;
    b13_data_delayed_1 <= 0;
    b13_data_delayed_2 <= 0;
    b13_data_delayed_3 <= 0;
    b13_data_delayed_4 <= 0;
    b13_data_delayed_5 <= 0;
    b13_data_delayed_6 <= 0;
    b13_data_delayed_7 <= 0;
    b13_data_delayed_8 <= 0;
    b13_data_delayed_9 <= 0;
    b13_data_delayed_10 <= 0;
    b13_data_delayed_11 <= 0;
    b13_data_delayed_12 <= 0;
    b13_data_delayed_13 <= 0;
    b14_data_delayed_1 <= 0;
    b14_data_delayed_2 <= 0;
    b14_data_delayed_3 <= 0;
    b14_data_delayed_4 <= 0;
    b14_data_delayed_5 <= 0;
    b14_data_delayed_6 <= 0;
    b14_data_delayed_7 <= 0;
    b14_data_delayed_8 <= 0;
    b14_data_delayed_9 <= 0;
    b14_data_delayed_10 <= 0;
    b14_data_delayed_11 <= 0;
    b14_data_delayed_12 <= 0;
    b14_data_delayed_13 <= 0;
    b14_data_delayed_14 <= 0;
    b15_data_delayed_1 <= 0;
    b15_data_delayed_2 <= 0;
    b15_data_delayed_3 <= 0;
    b15_data_delayed_4 <= 0;
    b15_data_delayed_5 <= 0;
    b15_data_delayed_6 <= 0;
    b15_data_delayed_7 <= 0;
    b15_data_delayed_8 <= 0;
    b15_data_delayed_9 <= 0;
    b15_data_delayed_10 <= 0;
    b15_data_delayed_11 <= 0;
    b15_data_delayed_12 <= 0;
    b15_data_delayed_13 <= 0;
    b15_data_delayed_14 <= 0;
    b15_data_delayed_15 <= 0;
    b16_data_delayed_1 <= 0;
    b16_data_delayed_2 <= 0;
    b16_data_delayed_3 <= 0;
    b16_data_delayed_4 <= 0;
    b16_data_delayed_5 <= 0;
    b16_data_delayed_6 <= 0;
    b16_data_delayed_7 <= 0;
    b16_data_delayed_8 <= 0;
    b16_data_delayed_9 <= 0;
    b16_data_delayed_10 <= 0;
    b16_data_delayed_11 <= 0;
    b16_data_delayed_12 <= 0;
    b16_data_delayed_13 <= 0;
    b16_data_delayed_14 <= 0;
    b16_data_delayed_15 <= 0;
    b16_data_delayed_16 <= 0;
    b17_data_delayed_1 <= 0;
    b17_data_delayed_2 <= 0;
    b17_data_delayed_3 <= 0;
    b17_data_delayed_4 <= 0;
    b17_data_delayed_5 <= 0;
    b17_data_delayed_6 <= 0;
    b17_data_delayed_7 <= 0;
    b17_data_delayed_8 <= 0;
    b17_data_delayed_9 <= 0;
    b17_data_delayed_10 <= 0;
    b17_data_delayed_11 <= 0;
    b17_data_delayed_12 <= 0;
    b17_data_delayed_13 <= 0;
    b17_data_delayed_14 <= 0;
    b17_data_delayed_15 <= 0;
    b17_data_delayed_16 <= 0;
    b17_data_delayed_17 <= 0;
    b18_data_delayed_1 <= 0;
    b18_data_delayed_2 <= 0;
    b18_data_delayed_3 <= 0;
    b18_data_delayed_4 <= 0;
    b18_data_delayed_5 <= 0;
    b18_data_delayed_6 <= 0;
    b18_data_delayed_7 <= 0;
    b18_data_delayed_8 <= 0;
    b18_data_delayed_9 <= 0;
    b18_data_delayed_10 <= 0;
    b18_data_delayed_11 <= 0;
    b18_data_delayed_12 <= 0;
    b18_data_delayed_13 <= 0;
    b18_data_delayed_14 <= 0;
    b18_data_delayed_15 <= 0;
    b18_data_delayed_16 <= 0;
    b18_data_delayed_17 <= 0;
    b18_data_delayed_18 <= 0;
    b19_data_delayed_1 <= 0;
    b19_data_delayed_2 <= 0;
    b19_data_delayed_3 <= 0;
    b19_data_delayed_4 <= 0;
    b19_data_delayed_5 <= 0;
    b19_data_delayed_6 <= 0;
    b19_data_delayed_7 <= 0;
    b19_data_delayed_8 <= 0;
    b19_data_delayed_9 <= 0;
    b19_data_delayed_10 <= 0;
    b19_data_delayed_11 <= 0;
    b19_data_delayed_12 <= 0;
    b19_data_delayed_13 <= 0;
    b19_data_delayed_14 <= 0;
    b19_data_delayed_15 <= 0;
    b19_data_delayed_16 <= 0;
    b19_data_delayed_17 <= 0;
    b19_data_delayed_18 <= 0;
    b19_data_delayed_19 <= 0;

  end
  else begin
  b1_data_delayed_1 <= b1_data;
  b2_data_delayed_1 <= b2_data;
  b3_data_delayed_1 <= b3_data;
  b4_data_delayed_1 <= b4_data;
  b5_data_delayed_1 <= b5_data;
  b6_data_delayed_1 <= b6_data;
  b7_data_delayed_1 <= b7_data;
  b8_data_delayed_1 <= b8_data;
  b9_data_delayed_1 <= b9_data;
  b10_data_delayed_1 <= b10_data;
  b11_data_delayed_1 <= b11_data;
  b12_data_delayed_1 <= b12_data;
  b13_data_delayed_1 <= b13_data;
  b14_data_delayed_1 <= b14_data;
  b15_data_delayed_1 <= b15_data;
  b16_data_delayed_1 <= b16_data;
  b17_data_delayed_1 <= b17_data;
  b18_data_delayed_1 <= b18_data;
  b19_data_delayed_1 <= b19_data;
  b2_data_delayed_2 <= b2_data_delayed_1;
  b3_data_delayed_2 <= b3_data_delayed_1;
  b3_data_delayed_3 <= b3_data_delayed_2;
  b4_data_delayed_2 <= b4_data_delayed_1;
  b4_data_delayed_3 <= b4_data_delayed_2;
  b4_data_delayed_4 <= b4_data_delayed_3;
  b5_data_delayed_2 <= b5_data_delayed_1;
  b5_data_delayed_3 <= b5_data_delayed_2;
  b5_data_delayed_4 <= b5_data_delayed_3;
  b5_data_delayed_5 <= b5_data_delayed_4;
  b6_data_delayed_2 <= b6_data_delayed_1;
  b6_data_delayed_3 <= b6_data_delayed_2;
  b6_data_delayed_4 <= b6_data_delayed_3;
  b6_data_delayed_5 <= b6_data_delayed_4;
  b6_data_delayed_6 <= b6_data_delayed_5;
  b7_data_delayed_2 <= b7_data_delayed_1;
  b7_data_delayed_3 <= b7_data_delayed_2;
  b7_data_delayed_4 <= b7_data_delayed_3;
  b7_data_delayed_5 <= b7_data_delayed_4;
  b7_data_delayed_6 <= b7_data_delayed_5;
  b7_data_delayed_7 <= b7_data_delayed_6;
  b8_data_delayed_2 <= b8_data_delayed_1;
  b8_data_delayed_3 <= b8_data_delayed_2;
  b8_data_delayed_4 <= b8_data_delayed_3;
  b8_data_delayed_5 <= b8_data_delayed_4;
  b8_data_delayed_6 <= b8_data_delayed_5;
  b8_data_delayed_7 <= b8_data_delayed_6;
  b8_data_delayed_8 <= b8_data_delayed_7;
  b9_data_delayed_2 <= b9_data_delayed_1;
  b9_data_delayed_3 <= b9_data_delayed_2;
  b9_data_delayed_4 <= b9_data_delayed_3;
  b9_data_delayed_5 <= b9_data_delayed_4;
  b9_data_delayed_6 <= b9_data_delayed_5;
  b9_data_delayed_7 <= b9_data_delayed_6;
  b9_data_delayed_8 <= b9_data_delayed_7;
  b9_data_delayed_9 <= b9_data_delayed_8;
  b10_data_delayed_2 <= b10_data_delayed_1;
  b10_data_delayed_3 <= b10_data_delayed_2;
  b10_data_delayed_4 <= b10_data_delayed_3;
  b10_data_delayed_5 <= b10_data_delayed_4;
  b10_data_delayed_6 <= b10_data_delayed_5;
  b10_data_delayed_7 <= b10_data_delayed_6;
  b10_data_delayed_8 <= b10_data_delayed_7;
  b10_data_delayed_9 <= b10_data_delayed_8;
  b10_data_delayed_10 <= b10_data_delayed_9;
  b11_data_delayed_2 <= b11_data_delayed_1;
  b11_data_delayed_3 <= b11_data_delayed_2;
  b11_data_delayed_4 <= b11_data_delayed_3;
  b11_data_delayed_5 <= b11_data_delayed_4;
  b11_data_delayed_6 <= b11_data_delayed_5;
  b11_data_delayed_7 <= b11_data_delayed_6;
  b11_data_delayed_8 <= b11_data_delayed_7;
  b11_data_delayed_9 <= b11_data_delayed_8;
  b11_data_delayed_10 <= b11_data_delayed_9;
  b11_data_delayed_11 <= b11_data_delayed_10;
  b12_data_delayed_2 <= b12_data_delayed_1;
  b12_data_delayed_3 <= b12_data_delayed_2;
  b12_data_delayed_4 <= b12_data_delayed_3;
  b12_data_delayed_5 <= b12_data_delayed_4;
  b12_data_delayed_6 <= b12_data_delayed_5;
  b12_data_delayed_7 <= b12_data_delayed_6;
  b12_data_delayed_8 <= b12_data_delayed_7;
  b12_data_delayed_9 <= b12_data_delayed_8;
  b12_data_delayed_10 <= b12_data_delayed_9;
  b12_data_delayed_11 <= b12_data_delayed_10;
  b12_data_delayed_12 <= b12_data_delayed_11;
  b13_data_delayed_2 <= b13_data_delayed_1;
  b13_data_delayed_3 <= b13_data_delayed_2;
  b13_data_delayed_4 <= b13_data_delayed_3;
  b13_data_delayed_5 <= b13_data_delayed_4;
  b13_data_delayed_6 <= b13_data_delayed_5;
  b13_data_delayed_7 <= b13_data_delayed_6;
  b13_data_delayed_8 <= b13_data_delayed_7;
  b13_data_delayed_9 <= b13_data_delayed_8;
  b13_data_delayed_10 <= b13_data_delayed_9;
  b13_data_delayed_11 <= b13_data_delayed_10;
  b13_data_delayed_12 <= b13_data_delayed_11;
  b13_data_delayed_13 <= b13_data_delayed_12;
  b14_data_delayed_2 <= b14_data_delayed_1;
  b14_data_delayed_3 <= b14_data_delayed_2;
  b14_data_delayed_4 <= b14_data_delayed_3;
  b14_data_delayed_5 <= b14_data_delayed_4;
  b14_data_delayed_6 <= b14_data_delayed_5;
  b14_data_delayed_7 <= b14_data_delayed_6;
  b14_data_delayed_8 <= b14_data_delayed_7;
  b14_data_delayed_9 <= b14_data_delayed_8;
  b14_data_delayed_10 <= b14_data_delayed_9;
  b14_data_delayed_11 <= b14_data_delayed_10;
  b14_data_delayed_12 <= b14_data_delayed_11;
  b14_data_delayed_13 <= b14_data_delayed_12;
  b14_data_delayed_14 <= b14_data_delayed_13;
  b15_data_delayed_2 <= b15_data_delayed_1;
  b15_data_delayed_3 <= b15_data_delayed_2;
  b15_data_delayed_4 <= b15_data_delayed_3;
  b15_data_delayed_5 <= b15_data_delayed_4;
  b15_data_delayed_6 <= b15_data_delayed_5;
  b15_data_delayed_7 <= b15_data_delayed_6;
  b15_data_delayed_8 <= b15_data_delayed_7;
  b15_data_delayed_9 <= b15_data_delayed_8;
  b15_data_delayed_10 <= b15_data_delayed_9;
  b15_data_delayed_11 <= b15_data_delayed_10;
  b15_data_delayed_12 <= b15_data_delayed_11;
  b15_data_delayed_13 <= b15_data_delayed_12;
  b15_data_delayed_14 <= b15_data_delayed_13;
  b15_data_delayed_15 <= b15_data_delayed_14;
  b16_data_delayed_2 <= b16_data_delayed_1;
  b16_data_delayed_3 <= b16_data_delayed_2;
  b16_data_delayed_4 <= b16_data_delayed_3;
  b16_data_delayed_5 <= b16_data_delayed_4;
  b16_data_delayed_6 <= b16_data_delayed_5;
  b16_data_delayed_7 <= b16_data_delayed_6;
  b16_data_delayed_8 <= b16_data_delayed_7;
  b16_data_delayed_9 <= b16_data_delayed_8;
  b16_data_delayed_10 <= b16_data_delayed_9;
  b16_data_delayed_11 <= b16_data_delayed_10;
  b16_data_delayed_12 <= b16_data_delayed_11;
  b16_data_delayed_13 <= b16_data_delayed_12;
  b16_data_delayed_14 <= b16_data_delayed_13;
  b16_data_delayed_15 <= b16_data_delayed_14;
  b16_data_delayed_16 <= b16_data_delayed_15;
  b17_data_delayed_2 <= b17_data_delayed_1;
  b17_data_delayed_3 <= b17_data_delayed_2;
  b17_data_delayed_4 <= b17_data_delayed_3;
  b17_data_delayed_5 <= b17_data_delayed_4;
  b17_data_delayed_6 <= b17_data_delayed_5;
  b17_data_delayed_7 <= b17_data_delayed_6;
  b17_data_delayed_8 <= b17_data_delayed_7;
  b17_data_delayed_9 <= b17_data_delayed_8;
  b17_data_delayed_10 <= b17_data_delayed_9;
  b17_data_delayed_11 <= b17_data_delayed_10;
  b17_data_delayed_12 <= b17_data_delayed_11;
  b17_data_delayed_13 <= b17_data_delayed_12;
  b17_data_delayed_14 <= b17_data_delayed_13;
  b17_data_delayed_15 <= b17_data_delayed_14;
  b17_data_delayed_16 <= b17_data_delayed_15;
  b17_data_delayed_17 <= b17_data_delayed_16;
  b18_data_delayed_2 <= b18_data_delayed_1;
  b18_data_delayed_3 <= b18_data_delayed_2;
  b18_data_delayed_4 <= b18_data_delayed_3;
  b18_data_delayed_5 <= b18_data_delayed_4;
  b18_data_delayed_6 <= b18_data_delayed_5;
  b18_data_delayed_7 <= b18_data_delayed_6;
  b18_data_delayed_8 <= b18_data_delayed_7;
  b18_data_delayed_9 <= b18_data_delayed_8;
  b18_data_delayed_10 <= b18_data_delayed_9;
  b18_data_delayed_11 <= b18_data_delayed_10;
  b18_data_delayed_12 <= b18_data_delayed_11;
  b18_data_delayed_13 <= b18_data_delayed_12;
  b18_data_delayed_14 <= b18_data_delayed_13;
  b18_data_delayed_15 <= b18_data_delayed_14;
  b18_data_delayed_16 <= b18_data_delayed_15;
  b18_data_delayed_17 <= b18_data_delayed_16;
  b18_data_delayed_18 <= b18_data_delayed_17;
  b19_data_delayed_2 <= b19_data_delayed_1;
  b19_data_delayed_3 <= b19_data_delayed_2;
  b19_data_delayed_4 <= b19_data_delayed_3;
  b19_data_delayed_5 <= b19_data_delayed_4;
  b19_data_delayed_6 <= b19_data_delayed_5;
  b19_data_delayed_7 <= b19_data_delayed_6;
  b19_data_delayed_8 <= b19_data_delayed_7;
  b19_data_delayed_9 <= b19_data_delayed_8;
  b19_data_delayed_10 <= b19_data_delayed_9;
  b19_data_delayed_11 <= b19_data_delayed_10;
  b19_data_delayed_12 <= b19_data_delayed_11;
  b19_data_delayed_13 <= b19_data_delayed_12;
  b19_data_delayed_14 <= b19_data_delayed_13;
  b19_data_delayed_15 <= b19_data_delayed_14;
  b19_data_delayed_16 <= b19_data_delayed_15;
  b19_data_delayed_17 <= b19_data_delayed_16;
  b19_data_delayed_18 <= b19_data_delayed_17;
  b19_data_delayed_19 <= b19_data_delayed_18;
 
  end
end
endmodule


//////////////////////////////////////////////////////////////////////////
// Systolically connected PEs
//////////////////////////////////////////////////////////////////////////
module systolic_pe_matrix(
clk,
reset,
pe_reset,
a0,
a1,
a2,
a3,
a4,
a5,
a6,
a7,
a8,
a9,
a10,
a11,
a12,
a13,
a14,
a15,
a16,
a17,
a18,
a19,
b0,
b1,
b2,
b3,
b4,
b5,
b6,
b7,
b8,
b9,
b10,
b11,
b12,
b13,
b14,
b15,
b16,
b17,
b18,
b19,
matrixC0_0,
matrixC0_1,
matrixC0_2,
matrixC0_3,
matrixC0_4,
matrixC0_5,
matrixC0_6,
matrixC0_7,
matrixC0_8,
matrixC0_9,
matrixC0_10,
matrixC0_11,
matrixC0_12,
matrixC0_13,
matrixC0_14,
matrixC0_15,
matrixC0_16,
matrixC0_17,
matrixC0_18,
matrixC0_19,
matrixC1_0,
matrixC1_1,
matrixC1_2,
matrixC1_3,
matrixC1_4,
matrixC1_5,
matrixC1_6,
matrixC1_7,
matrixC1_8,
matrixC1_9,
matrixC1_10,
matrixC1_11,
matrixC1_12,
matrixC1_13,
matrixC1_14,
matrixC1_15,
matrixC1_16,
matrixC1_17,
matrixC1_18,
matrixC1_19,
matrixC2_0,
matrixC2_1,
matrixC2_2,
matrixC2_3,
matrixC2_4,
matrixC2_5,
matrixC2_6,
matrixC2_7,
matrixC2_8,
matrixC2_9,
matrixC2_10,
matrixC2_11,
matrixC2_12,
matrixC2_13,
matrixC2_14,
matrixC2_15,
matrixC2_16,
matrixC2_17,
matrixC2_18,
matrixC2_19,
matrixC3_0,
matrixC3_1,
matrixC3_2,
matrixC3_3,
matrixC3_4,
matrixC3_5,
matrixC3_6,
matrixC3_7,
matrixC3_8,
matrixC3_9,
matrixC3_10,
matrixC3_11,
matrixC3_12,
matrixC3_13,
matrixC3_14,
matrixC3_15,
matrixC3_16,
matrixC3_17,
matrixC3_18,
matrixC3_19,
matrixC4_0,
matrixC4_1,
matrixC4_2,
matrixC4_3,
matrixC4_4,
matrixC4_5,
matrixC4_6,
matrixC4_7,
matrixC4_8,
matrixC4_9,
matrixC4_10,
matrixC4_11,
matrixC4_12,
matrixC4_13,
matrixC4_14,
matrixC4_15,
matrixC4_16,
matrixC4_17,
matrixC4_18,
matrixC4_19,
matrixC5_0,
matrixC5_1,
matrixC5_2,
matrixC5_3,
matrixC5_4,
matrixC5_5,
matrixC5_6,
matrixC5_7,
matrixC5_8,
matrixC5_9,
matrixC5_10,
matrixC5_11,
matrixC5_12,
matrixC5_13,
matrixC5_14,
matrixC5_15,
matrixC5_16,
matrixC5_17,
matrixC5_18,
matrixC5_19,
matrixC6_0,
matrixC6_1,
matrixC6_2,
matrixC6_3,
matrixC6_4,
matrixC6_5,
matrixC6_6,
matrixC6_7,
matrixC6_8,
matrixC6_9,
matrixC6_10,
matrixC6_11,
matrixC6_12,
matrixC6_13,
matrixC6_14,
matrixC6_15,
matrixC6_16,
matrixC6_17,
matrixC6_18,
matrixC6_19,
matrixC7_0,
matrixC7_1,
matrixC7_2,
matrixC7_3,
matrixC7_4,
matrixC7_5,
matrixC7_6,
matrixC7_7,
matrixC7_8,
matrixC7_9,
matrixC7_10,
matrixC7_11,
matrixC7_12,
matrixC7_13,
matrixC7_14,
matrixC7_15,
matrixC7_16,
matrixC7_17,
matrixC7_18,
matrixC7_19,
matrixC8_0,
matrixC8_1,
matrixC8_2,
matrixC8_3,
matrixC8_4,
matrixC8_5,
matrixC8_6,
matrixC8_7,
matrixC8_8,
matrixC8_9,
matrixC8_10,
matrixC8_11,
matrixC8_12,
matrixC8_13,
matrixC8_14,
matrixC8_15,
matrixC8_16,
matrixC8_17,
matrixC8_18,
matrixC8_19,
matrixC9_0,
matrixC9_1,
matrixC9_2,
matrixC9_3,
matrixC9_4,
matrixC9_5,
matrixC9_6,
matrixC9_7,
matrixC9_8,
matrixC9_9,
matrixC9_10,
matrixC9_11,
matrixC9_12,
matrixC9_13,
matrixC9_14,
matrixC9_15,
matrixC9_16,
matrixC9_17,
matrixC9_18,
matrixC9_19,
matrixC10_0,
matrixC10_1,
matrixC10_2,
matrixC10_3,
matrixC10_4,
matrixC10_5,
matrixC10_6,
matrixC10_7,
matrixC10_8,
matrixC10_9,
matrixC10_10,
matrixC10_11,
matrixC10_12,
matrixC10_13,
matrixC10_14,
matrixC10_15,
matrixC10_16,
matrixC10_17,
matrixC10_18,
matrixC10_19,
matrixC11_0,
matrixC11_1,
matrixC11_2,
matrixC11_3,
matrixC11_4,
matrixC11_5,
matrixC11_6,
matrixC11_7,
matrixC11_8,
matrixC11_9,
matrixC11_10,
matrixC11_11,
matrixC11_12,
matrixC11_13,
matrixC11_14,
matrixC11_15,
matrixC11_16,
matrixC11_17,
matrixC11_18,
matrixC11_19,
matrixC12_0,
matrixC12_1,
matrixC12_2,
matrixC12_3,
matrixC12_4,
matrixC12_5,
matrixC12_6,
matrixC12_7,
matrixC12_8,
matrixC12_9,
matrixC12_10,
matrixC12_11,
matrixC12_12,
matrixC12_13,
matrixC12_14,
matrixC12_15,
matrixC12_16,
matrixC12_17,
matrixC12_18,
matrixC12_19,
matrixC13_0,
matrixC13_1,
matrixC13_2,
matrixC13_3,
matrixC13_4,
matrixC13_5,
matrixC13_6,
matrixC13_7,
matrixC13_8,
matrixC13_9,
matrixC13_10,
matrixC13_11,
matrixC13_12,
matrixC13_13,
matrixC13_14,
matrixC13_15,
matrixC13_16,
matrixC13_17,
matrixC13_18,
matrixC13_19,
matrixC14_0,
matrixC14_1,
matrixC14_2,
matrixC14_3,
matrixC14_4,
matrixC14_5,
matrixC14_6,
matrixC14_7,
matrixC14_8,
matrixC14_9,
matrixC14_10,
matrixC14_11,
matrixC14_12,
matrixC14_13,
matrixC14_14,
matrixC14_15,
matrixC14_16,
matrixC14_17,
matrixC14_18,
matrixC14_19,
matrixC15_0,
matrixC15_1,
matrixC15_2,
matrixC15_3,
matrixC15_4,
matrixC15_5,
matrixC15_6,
matrixC15_7,
matrixC15_8,
matrixC15_9,
matrixC15_10,
matrixC15_11,
matrixC15_12,
matrixC15_13,
matrixC15_14,
matrixC15_15,
matrixC15_16,
matrixC15_17,
matrixC15_18,
matrixC15_19,
matrixC16_0,
matrixC16_1,
matrixC16_2,
matrixC16_3,
matrixC16_4,
matrixC16_5,
matrixC16_6,
matrixC16_7,
matrixC16_8,
matrixC16_9,
matrixC16_10,
matrixC16_11,
matrixC16_12,
matrixC16_13,
matrixC16_14,
matrixC16_15,
matrixC16_16,
matrixC16_17,
matrixC16_18,
matrixC16_19,
matrixC17_0,
matrixC17_1,
matrixC17_2,
matrixC17_3,
matrixC17_4,
matrixC17_5,
matrixC17_6,
matrixC17_7,
matrixC17_8,
matrixC17_9,
matrixC17_10,
matrixC17_11,
matrixC17_12,
matrixC17_13,
matrixC17_14,
matrixC17_15,
matrixC17_16,
matrixC17_17,
matrixC17_18,
matrixC17_19,
matrixC18_0,
matrixC18_1,
matrixC18_2,
matrixC18_3,
matrixC18_4,
matrixC18_5,
matrixC18_6,
matrixC18_7,
matrixC18_8,
matrixC18_9,
matrixC18_10,
matrixC18_11,
matrixC18_12,
matrixC18_13,
matrixC18_14,
matrixC18_15,
matrixC18_16,
matrixC18_17,
matrixC18_18,
matrixC18_19,
matrixC19_0,
matrixC19_1,
matrixC19_2,
matrixC19_3,
matrixC19_4,
matrixC19_5,
matrixC19_6,
matrixC19_7,
matrixC19_8,
matrixC19_9,
matrixC19_10,
matrixC19_11,
matrixC19_12,
matrixC19_13,
matrixC19_14,
matrixC19_15,
matrixC19_16,
matrixC19_17,
matrixC19_18,
matrixC19_19,

a_data_out,
b_data_out
);

input clk;
input reset;
input pe_reset;
input [`DWIDTH-1:0] a0;
input [`DWIDTH-1:0] a1;
input [`DWIDTH-1:0] a2;
input [`DWIDTH-1:0] a3;
input [`DWIDTH-1:0] a4;
input [`DWIDTH-1:0] a5;
input [`DWIDTH-1:0] a6;
input [`DWIDTH-1:0] a7;
input [`DWIDTH-1:0] a8;
input [`DWIDTH-1:0] a9;
input [`DWIDTH-1:0] a10;
input [`DWIDTH-1:0] a11;
input [`DWIDTH-1:0] a12;
input [`DWIDTH-1:0] a13;
input [`DWIDTH-1:0] a14;
input [`DWIDTH-1:0] a15;
input [`DWIDTH-1:0] a16;
input [`DWIDTH-1:0] a17;
input [`DWIDTH-1:0] a18;
input [`DWIDTH-1:0] a19;
input [`DWIDTH-1:0] b0;
input [`DWIDTH-1:0] b1;
input [`DWIDTH-1:0] b2;
input [`DWIDTH-1:0] b3;
input [`DWIDTH-1:0] b4;
input [`DWIDTH-1:0] b5;
input [`DWIDTH-1:0] b6;
input [`DWIDTH-1:0] b7;
input [`DWIDTH-1:0] b8;
input [`DWIDTH-1:0] b9;
input [`DWIDTH-1:0] b10;
input [`DWIDTH-1:0] b11;
input [`DWIDTH-1:0] b12;
input [`DWIDTH-1:0] b13;
input [`DWIDTH-1:0] b14;
input [`DWIDTH-1:0] b15;
input [`DWIDTH-1:0] b16;
input [`DWIDTH-1:0] b17;
input [`DWIDTH-1:0] b18;
input [`DWIDTH-1:0] b19;
output [`DWIDTH-1:0] matrixC0_0;
output [`DWIDTH-1:0] matrixC0_1;
output [`DWIDTH-1:0] matrixC0_2;
output [`DWIDTH-1:0] matrixC0_3;
output [`DWIDTH-1:0] matrixC0_4;
output [`DWIDTH-1:0] matrixC0_5;
output [`DWIDTH-1:0] matrixC0_6;
output [`DWIDTH-1:0] matrixC0_7;
output [`DWIDTH-1:0] matrixC0_8;
output [`DWIDTH-1:0] matrixC0_9;
output [`DWIDTH-1:0] matrixC0_10;
output [`DWIDTH-1:0] matrixC0_11;
output [`DWIDTH-1:0] matrixC0_12;
output [`DWIDTH-1:0] matrixC0_13;
output [`DWIDTH-1:0] matrixC0_14;
output [`DWIDTH-1:0] matrixC0_15;
output [`DWIDTH-1:0] matrixC0_16;
output [`DWIDTH-1:0] matrixC0_17;
output [`DWIDTH-1:0] matrixC0_18;
output [`DWIDTH-1:0] matrixC0_19;
output [`DWIDTH-1:0] matrixC1_0;
output [`DWIDTH-1:0] matrixC1_1;
output [`DWIDTH-1:0] matrixC1_2;
output [`DWIDTH-1:0] matrixC1_3;
output [`DWIDTH-1:0] matrixC1_4;
output [`DWIDTH-1:0] matrixC1_5;
output [`DWIDTH-1:0] matrixC1_6;
output [`DWIDTH-1:0] matrixC1_7;
output [`DWIDTH-1:0] matrixC1_8;
output [`DWIDTH-1:0] matrixC1_9;
output [`DWIDTH-1:0] matrixC1_10;
output [`DWIDTH-1:0] matrixC1_11;
output [`DWIDTH-1:0] matrixC1_12;
output [`DWIDTH-1:0] matrixC1_13;
output [`DWIDTH-1:0] matrixC1_14;
output [`DWIDTH-1:0] matrixC1_15;
output [`DWIDTH-1:0] matrixC1_16;
output [`DWIDTH-1:0] matrixC1_17;
output [`DWIDTH-1:0] matrixC1_18;
output [`DWIDTH-1:0] matrixC1_19;
output [`DWIDTH-1:0] matrixC2_0;
output [`DWIDTH-1:0] matrixC2_1;
output [`DWIDTH-1:0] matrixC2_2;
output [`DWIDTH-1:0] matrixC2_3;
output [`DWIDTH-1:0] matrixC2_4;
output [`DWIDTH-1:0] matrixC2_5;
output [`DWIDTH-1:0] matrixC2_6;
output [`DWIDTH-1:0] matrixC2_7;
output [`DWIDTH-1:0] matrixC2_8;
output [`DWIDTH-1:0] matrixC2_9;
output [`DWIDTH-1:0] matrixC2_10;
output [`DWIDTH-1:0] matrixC2_11;
output [`DWIDTH-1:0] matrixC2_12;
output [`DWIDTH-1:0] matrixC2_13;
output [`DWIDTH-1:0] matrixC2_14;
output [`DWIDTH-1:0] matrixC2_15;
output [`DWIDTH-1:0] matrixC2_16;
output [`DWIDTH-1:0] matrixC2_17;
output [`DWIDTH-1:0] matrixC2_18;
output [`DWIDTH-1:0] matrixC2_19;
output [`DWIDTH-1:0] matrixC3_0;
output [`DWIDTH-1:0] matrixC3_1;
output [`DWIDTH-1:0] matrixC3_2;
output [`DWIDTH-1:0] matrixC3_3;
output [`DWIDTH-1:0] matrixC3_4;
output [`DWIDTH-1:0] matrixC3_5;
output [`DWIDTH-1:0] matrixC3_6;
output [`DWIDTH-1:0] matrixC3_7;
output [`DWIDTH-1:0] matrixC3_8;
output [`DWIDTH-1:0] matrixC3_9;
output [`DWIDTH-1:0] matrixC3_10;
output [`DWIDTH-1:0] matrixC3_11;
output [`DWIDTH-1:0] matrixC3_12;
output [`DWIDTH-1:0] matrixC3_13;
output [`DWIDTH-1:0] matrixC3_14;
output [`DWIDTH-1:0] matrixC3_15;
output [`DWIDTH-1:0] matrixC3_16;
output [`DWIDTH-1:0] matrixC3_17;
output [`DWIDTH-1:0] matrixC3_18;
output [`DWIDTH-1:0] matrixC3_19;
output [`DWIDTH-1:0] matrixC4_0;
output [`DWIDTH-1:0] matrixC4_1;
output [`DWIDTH-1:0] matrixC4_2;
output [`DWIDTH-1:0] matrixC4_3;
output [`DWIDTH-1:0] matrixC4_4;
output [`DWIDTH-1:0] matrixC4_5;
output [`DWIDTH-1:0] matrixC4_6;
output [`DWIDTH-1:0] matrixC4_7;
output [`DWIDTH-1:0] matrixC4_8;
output [`DWIDTH-1:0] matrixC4_9;
output [`DWIDTH-1:0] matrixC4_10;
output [`DWIDTH-1:0] matrixC4_11;
output [`DWIDTH-1:0] matrixC4_12;
output [`DWIDTH-1:0] matrixC4_13;
output [`DWIDTH-1:0] matrixC4_14;
output [`DWIDTH-1:0] matrixC4_15;
output [`DWIDTH-1:0] matrixC4_16;
output [`DWIDTH-1:0] matrixC4_17;
output [`DWIDTH-1:0] matrixC4_18;
output [`DWIDTH-1:0] matrixC4_19;
output [`DWIDTH-1:0] matrixC5_0;
output [`DWIDTH-1:0] matrixC5_1;
output [`DWIDTH-1:0] matrixC5_2;
output [`DWIDTH-1:0] matrixC5_3;
output [`DWIDTH-1:0] matrixC5_4;
output [`DWIDTH-1:0] matrixC5_5;
output [`DWIDTH-1:0] matrixC5_6;
output [`DWIDTH-1:0] matrixC5_7;
output [`DWIDTH-1:0] matrixC5_8;
output [`DWIDTH-1:0] matrixC5_9;
output [`DWIDTH-1:0] matrixC5_10;
output [`DWIDTH-1:0] matrixC5_11;
output [`DWIDTH-1:0] matrixC5_12;
output [`DWIDTH-1:0] matrixC5_13;
output [`DWIDTH-1:0] matrixC5_14;
output [`DWIDTH-1:0] matrixC5_15;
output [`DWIDTH-1:0] matrixC5_16;
output [`DWIDTH-1:0] matrixC5_17;
output [`DWIDTH-1:0] matrixC5_18;
output [`DWIDTH-1:0] matrixC5_19;
output [`DWIDTH-1:0] matrixC6_0;
output [`DWIDTH-1:0] matrixC6_1;
output [`DWIDTH-1:0] matrixC6_2;
output [`DWIDTH-1:0] matrixC6_3;
output [`DWIDTH-1:0] matrixC6_4;
output [`DWIDTH-1:0] matrixC6_5;
output [`DWIDTH-1:0] matrixC6_6;
output [`DWIDTH-1:0] matrixC6_7;
output [`DWIDTH-1:0] matrixC6_8;
output [`DWIDTH-1:0] matrixC6_9;
output [`DWIDTH-1:0] matrixC6_10;
output [`DWIDTH-1:0] matrixC6_11;
output [`DWIDTH-1:0] matrixC6_12;
output [`DWIDTH-1:0] matrixC6_13;
output [`DWIDTH-1:0] matrixC6_14;
output [`DWIDTH-1:0] matrixC6_15;
output [`DWIDTH-1:0] matrixC6_16;
output [`DWIDTH-1:0] matrixC6_17;
output [`DWIDTH-1:0] matrixC6_18;
output [`DWIDTH-1:0] matrixC6_19;
output [`DWIDTH-1:0] matrixC7_0;
output [`DWIDTH-1:0] matrixC7_1;
output [`DWIDTH-1:0] matrixC7_2;
output [`DWIDTH-1:0] matrixC7_3;
output [`DWIDTH-1:0] matrixC7_4;
output [`DWIDTH-1:0] matrixC7_5;
output [`DWIDTH-1:0] matrixC7_6;
output [`DWIDTH-1:0] matrixC7_7;
output [`DWIDTH-1:0] matrixC7_8;
output [`DWIDTH-1:0] matrixC7_9;
output [`DWIDTH-1:0] matrixC7_10;
output [`DWIDTH-1:0] matrixC7_11;
output [`DWIDTH-1:0] matrixC7_12;
output [`DWIDTH-1:0] matrixC7_13;
output [`DWIDTH-1:0] matrixC7_14;
output [`DWIDTH-1:0] matrixC7_15;
output [`DWIDTH-1:0] matrixC7_16;
output [`DWIDTH-1:0] matrixC7_17;
output [`DWIDTH-1:0] matrixC7_18;
output [`DWIDTH-1:0] matrixC7_19;
output [`DWIDTH-1:0] matrixC8_0;
output [`DWIDTH-1:0] matrixC8_1;
output [`DWIDTH-1:0] matrixC8_2;
output [`DWIDTH-1:0] matrixC8_3;
output [`DWIDTH-1:0] matrixC8_4;
output [`DWIDTH-1:0] matrixC8_5;
output [`DWIDTH-1:0] matrixC8_6;
output [`DWIDTH-1:0] matrixC8_7;
output [`DWIDTH-1:0] matrixC8_8;
output [`DWIDTH-1:0] matrixC8_9;
output [`DWIDTH-1:0] matrixC8_10;
output [`DWIDTH-1:0] matrixC8_11;
output [`DWIDTH-1:0] matrixC8_12;
output [`DWIDTH-1:0] matrixC8_13;
output [`DWIDTH-1:0] matrixC8_14;
output [`DWIDTH-1:0] matrixC8_15;
output [`DWIDTH-1:0] matrixC8_16;
output [`DWIDTH-1:0] matrixC8_17;
output [`DWIDTH-1:0] matrixC8_18;
output [`DWIDTH-1:0] matrixC8_19;
output [`DWIDTH-1:0] matrixC9_0;
output [`DWIDTH-1:0] matrixC9_1;
output [`DWIDTH-1:0] matrixC9_2;
output [`DWIDTH-1:0] matrixC9_3;
output [`DWIDTH-1:0] matrixC9_4;
output [`DWIDTH-1:0] matrixC9_5;
output [`DWIDTH-1:0] matrixC9_6;
output [`DWIDTH-1:0] matrixC9_7;
output [`DWIDTH-1:0] matrixC9_8;
output [`DWIDTH-1:0] matrixC9_9;
output [`DWIDTH-1:0] matrixC9_10;
output [`DWIDTH-1:0] matrixC9_11;
output [`DWIDTH-1:0] matrixC9_12;
output [`DWIDTH-1:0] matrixC9_13;
output [`DWIDTH-1:0] matrixC9_14;
output [`DWIDTH-1:0] matrixC9_15;
output [`DWIDTH-1:0] matrixC9_16;
output [`DWIDTH-1:0] matrixC9_17;
output [`DWIDTH-1:0] matrixC9_18;
output [`DWIDTH-1:0] matrixC9_19;
output [`DWIDTH-1:0] matrixC10_0;
output [`DWIDTH-1:0] matrixC10_1;
output [`DWIDTH-1:0] matrixC10_2;
output [`DWIDTH-1:0] matrixC10_3;
output [`DWIDTH-1:0] matrixC10_4;
output [`DWIDTH-1:0] matrixC10_5;
output [`DWIDTH-1:0] matrixC10_6;
output [`DWIDTH-1:0] matrixC10_7;
output [`DWIDTH-1:0] matrixC10_8;
output [`DWIDTH-1:0] matrixC10_9;
output [`DWIDTH-1:0] matrixC10_10;
output [`DWIDTH-1:0] matrixC10_11;
output [`DWIDTH-1:0] matrixC10_12;
output [`DWIDTH-1:0] matrixC10_13;
output [`DWIDTH-1:0] matrixC10_14;
output [`DWIDTH-1:0] matrixC10_15;
output [`DWIDTH-1:0] matrixC10_16;
output [`DWIDTH-1:0] matrixC10_17;
output [`DWIDTH-1:0] matrixC10_18;
output [`DWIDTH-1:0] matrixC10_19;
output [`DWIDTH-1:0] matrixC11_0;
output [`DWIDTH-1:0] matrixC11_1;
output [`DWIDTH-1:0] matrixC11_2;
output [`DWIDTH-1:0] matrixC11_3;
output [`DWIDTH-1:0] matrixC11_4;
output [`DWIDTH-1:0] matrixC11_5;
output [`DWIDTH-1:0] matrixC11_6;
output [`DWIDTH-1:0] matrixC11_7;
output [`DWIDTH-1:0] matrixC11_8;
output [`DWIDTH-1:0] matrixC11_9;
output [`DWIDTH-1:0] matrixC11_10;
output [`DWIDTH-1:0] matrixC11_11;
output [`DWIDTH-1:0] matrixC11_12;
output [`DWIDTH-1:0] matrixC11_13;
output [`DWIDTH-1:0] matrixC11_14;
output [`DWIDTH-1:0] matrixC11_15;
output [`DWIDTH-1:0] matrixC11_16;
output [`DWIDTH-1:0] matrixC11_17;
output [`DWIDTH-1:0] matrixC11_18;
output [`DWIDTH-1:0] matrixC11_19;
output [`DWIDTH-1:0] matrixC12_0;
output [`DWIDTH-1:0] matrixC12_1;
output [`DWIDTH-1:0] matrixC12_2;
output [`DWIDTH-1:0] matrixC12_3;
output [`DWIDTH-1:0] matrixC12_4;
output [`DWIDTH-1:0] matrixC12_5;
output [`DWIDTH-1:0] matrixC12_6;
output [`DWIDTH-1:0] matrixC12_7;
output [`DWIDTH-1:0] matrixC12_8;
output [`DWIDTH-1:0] matrixC12_9;
output [`DWIDTH-1:0] matrixC12_10;
output [`DWIDTH-1:0] matrixC12_11;
output [`DWIDTH-1:0] matrixC12_12;
output [`DWIDTH-1:0] matrixC12_13;
output [`DWIDTH-1:0] matrixC12_14;
output [`DWIDTH-1:0] matrixC12_15;
output [`DWIDTH-1:0] matrixC12_16;
output [`DWIDTH-1:0] matrixC12_17;
output [`DWIDTH-1:0] matrixC12_18;
output [`DWIDTH-1:0] matrixC12_19;
output [`DWIDTH-1:0] matrixC13_0;
output [`DWIDTH-1:0] matrixC13_1;
output [`DWIDTH-1:0] matrixC13_2;
output [`DWIDTH-1:0] matrixC13_3;
output [`DWIDTH-1:0] matrixC13_4;
output [`DWIDTH-1:0] matrixC13_5;
output [`DWIDTH-1:0] matrixC13_6;
output [`DWIDTH-1:0] matrixC13_7;
output [`DWIDTH-1:0] matrixC13_8;
output [`DWIDTH-1:0] matrixC13_9;
output [`DWIDTH-1:0] matrixC13_10;
output [`DWIDTH-1:0] matrixC13_11;
output [`DWIDTH-1:0] matrixC13_12;
output [`DWIDTH-1:0] matrixC13_13;
output [`DWIDTH-1:0] matrixC13_14;
output [`DWIDTH-1:0] matrixC13_15;
output [`DWIDTH-1:0] matrixC13_16;
output [`DWIDTH-1:0] matrixC13_17;
output [`DWIDTH-1:0] matrixC13_18;
output [`DWIDTH-1:0] matrixC13_19;
output [`DWIDTH-1:0] matrixC14_0;
output [`DWIDTH-1:0] matrixC14_1;
output [`DWIDTH-1:0] matrixC14_2;
output [`DWIDTH-1:0] matrixC14_3;
output [`DWIDTH-1:0] matrixC14_4;
output [`DWIDTH-1:0] matrixC14_5;
output [`DWIDTH-1:0] matrixC14_6;
output [`DWIDTH-1:0] matrixC14_7;
output [`DWIDTH-1:0] matrixC14_8;
output [`DWIDTH-1:0] matrixC14_9;
output [`DWIDTH-1:0] matrixC14_10;
output [`DWIDTH-1:0] matrixC14_11;
output [`DWIDTH-1:0] matrixC14_12;
output [`DWIDTH-1:0] matrixC14_13;
output [`DWIDTH-1:0] matrixC14_14;
output [`DWIDTH-1:0] matrixC14_15;
output [`DWIDTH-1:0] matrixC14_16;
output [`DWIDTH-1:0] matrixC14_17;
output [`DWIDTH-1:0] matrixC14_18;
output [`DWIDTH-1:0] matrixC14_19;
output [`DWIDTH-1:0] matrixC15_0;
output [`DWIDTH-1:0] matrixC15_1;
output [`DWIDTH-1:0] matrixC15_2;
output [`DWIDTH-1:0] matrixC15_3;
output [`DWIDTH-1:0] matrixC15_4;
output [`DWIDTH-1:0] matrixC15_5;
output [`DWIDTH-1:0] matrixC15_6;
output [`DWIDTH-1:0] matrixC15_7;
output [`DWIDTH-1:0] matrixC15_8;
output [`DWIDTH-1:0] matrixC15_9;
output [`DWIDTH-1:0] matrixC15_10;
output [`DWIDTH-1:0] matrixC15_11;
output [`DWIDTH-1:0] matrixC15_12;
output [`DWIDTH-1:0] matrixC15_13;
output [`DWIDTH-1:0] matrixC15_14;
output [`DWIDTH-1:0] matrixC15_15;
output [`DWIDTH-1:0] matrixC15_16;
output [`DWIDTH-1:0] matrixC15_17;
output [`DWIDTH-1:0] matrixC15_18;
output [`DWIDTH-1:0] matrixC15_19;
output [`DWIDTH-1:0] matrixC16_0;
output [`DWIDTH-1:0] matrixC16_1;
output [`DWIDTH-1:0] matrixC16_2;
output [`DWIDTH-1:0] matrixC16_3;
output [`DWIDTH-1:0] matrixC16_4;
output [`DWIDTH-1:0] matrixC16_5;
output [`DWIDTH-1:0] matrixC16_6;
output [`DWIDTH-1:0] matrixC16_7;
output [`DWIDTH-1:0] matrixC16_8;
output [`DWIDTH-1:0] matrixC16_9;
output [`DWIDTH-1:0] matrixC16_10;
output [`DWIDTH-1:0] matrixC16_11;
output [`DWIDTH-1:0] matrixC16_12;
output [`DWIDTH-1:0] matrixC16_13;
output [`DWIDTH-1:0] matrixC16_14;
output [`DWIDTH-1:0] matrixC16_15;
output [`DWIDTH-1:0] matrixC16_16;
output [`DWIDTH-1:0] matrixC16_17;
output [`DWIDTH-1:0] matrixC16_18;
output [`DWIDTH-1:0] matrixC16_19;
output [`DWIDTH-1:0] matrixC17_0;
output [`DWIDTH-1:0] matrixC17_1;
output [`DWIDTH-1:0] matrixC17_2;
output [`DWIDTH-1:0] matrixC17_3;
output [`DWIDTH-1:0] matrixC17_4;
output [`DWIDTH-1:0] matrixC17_5;
output [`DWIDTH-1:0] matrixC17_6;
output [`DWIDTH-1:0] matrixC17_7;
output [`DWIDTH-1:0] matrixC17_8;
output [`DWIDTH-1:0] matrixC17_9;
output [`DWIDTH-1:0] matrixC17_10;
output [`DWIDTH-1:0] matrixC17_11;
output [`DWIDTH-1:0] matrixC17_12;
output [`DWIDTH-1:0] matrixC17_13;
output [`DWIDTH-1:0] matrixC17_14;
output [`DWIDTH-1:0] matrixC17_15;
output [`DWIDTH-1:0] matrixC17_16;
output [`DWIDTH-1:0] matrixC17_17;
output [`DWIDTH-1:0] matrixC17_18;
output [`DWIDTH-1:0] matrixC17_19;
output [`DWIDTH-1:0] matrixC18_0;
output [`DWIDTH-1:0] matrixC18_1;
output [`DWIDTH-1:0] matrixC18_2;
output [`DWIDTH-1:0] matrixC18_3;
output [`DWIDTH-1:0] matrixC18_4;
output [`DWIDTH-1:0] matrixC18_5;
output [`DWIDTH-1:0] matrixC18_6;
output [`DWIDTH-1:0] matrixC18_7;
output [`DWIDTH-1:0] matrixC18_8;
output [`DWIDTH-1:0] matrixC18_9;
output [`DWIDTH-1:0] matrixC18_10;
output [`DWIDTH-1:0] matrixC18_11;
output [`DWIDTH-1:0] matrixC18_12;
output [`DWIDTH-1:0] matrixC18_13;
output [`DWIDTH-1:0] matrixC18_14;
output [`DWIDTH-1:0] matrixC18_15;
output [`DWIDTH-1:0] matrixC18_16;
output [`DWIDTH-1:0] matrixC18_17;
output [`DWIDTH-1:0] matrixC18_18;
output [`DWIDTH-1:0] matrixC18_19;
output [`DWIDTH-1:0] matrixC19_0;
output [`DWIDTH-1:0] matrixC19_1;
output [`DWIDTH-1:0] matrixC19_2;
output [`DWIDTH-1:0] matrixC19_3;
output [`DWIDTH-1:0] matrixC19_4;
output [`DWIDTH-1:0] matrixC19_5;
output [`DWIDTH-1:0] matrixC19_6;
output [`DWIDTH-1:0] matrixC19_7;
output [`DWIDTH-1:0] matrixC19_8;
output [`DWIDTH-1:0] matrixC19_9;
output [`DWIDTH-1:0] matrixC19_10;
output [`DWIDTH-1:0] matrixC19_11;
output [`DWIDTH-1:0] matrixC19_12;
output [`DWIDTH-1:0] matrixC19_13;
output [`DWIDTH-1:0] matrixC19_14;
output [`DWIDTH-1:0] matrixC19_15;
output [`DWIDTH-1:0] matrixC19_16;
output [`DWIDTH-1:0] matrixC19_17;
output [`DWIDTH-1:0] matrixC19_18;
output [`DWIDTH-1:0] matrixC19_19;

output [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_out;
output [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_out;

wire [`DWIDTH-1:0] a0_0to0_1, a0_1to0_2, a0_2to0_3, a0_3to0_4, a0_4to0_5, a0_5to0_6, a0_6to0_7, a0_7to0_8, a0_8to0_9, a0_9to0_10, a0_10to0_11, a0_11to0_12, a0_12to0_13, a0_13to0_14, a0_14to0_15, a0_15to0_16, a0_16to0_17, a0_17to0_18, a0_18to0_19, a0_19to0_20;
wire [`DWIDTH-1:0] a1_0to1_1, a1_1to1_2, a1_2to1_3, a1_3to1_4, a1_4to1_5, a1_5to1_6, a1_6to1_7, a1_7to1_8, a1_8to1_9, a1_9to1_10, a1_10to1_11, a1_11to1_12, a1_12to1_13, a1_13to1_14, a1_14to1_15, a1_15to1_16, a1_16to1_17, a1_17to1_18, a1_18to1_19, a1_19to1_20;
wire [`DWIDTH-1:0] a2_0to2_1, a2_1to2_2, a2_2to2_3, a2_3to2_4, a2_4to2_5, a2_5to2_6, a2_6to2_7, a2_7to2_8, a2_8to2_9, a2_9to2_10, a2_10to2_11, a2_11to2_12, a2_12to2_13, a2_13to2_14, a2_14to2_15, a2_15to2_16, a2_16to2_17, a2_17to2_18, a2_18to2_19, a2_19to2_20;
wire [`DWIDTH-1:0] a3_0to3_1, a3_1to3_2, a3_2to3_3, a3_3to3_4, a3_4to3_5, a3_5to3_6, a3_6to3_7, a3_7to3_8, a3_8to3_9, a3_9to3_10, a3_10to3_11, a3_11to3_12, a3_12to3_13, a3_13to3_14, a3_14to3_15, a3_15to3_16, a3_16to3_17, a3_17to3_18, a3_18to3_19, a3_19to3_20;
wire [`DWIDTH-1:0] a4_0to4_1, a4_1to4_2, a4_2to4_3, a4_3to4_4, a4_4to4_5, a4_5to4_6, a4_6to4_7, a4_7to4_8, a4_8to4_9, a4_9to4_10, a4_10to4_11, a4_11to4_12, a4_12to4_13, a4_13to4_14, a4_14to4_15, a4_15to4_16, a4_16to4_17, a4_17to4_18, a4_18to4_19, a4_19to4_20;
wire [`DWIDTH-1:0] a5_0to5_1, a5_1to5_2, a5_2to5_3, a5_3to5_4, a5_4to5_5, a5_5to5_6, a5_6to5_7, a5_7to5_8, a5_8to5_9, a5_9to5_10, a5_10to5_11, a5_11to5_12, a5_12to5_13, a5_13to5_14, a5_14to5_15, a5_15to5_16, a5_16to5_17, a5_17to5_18, a5_18to5_19, a5_19to5_20;
wire [`DWIDTH-1:0] a6_0to6_1, a6_1to6_2, a6_2to6_3, a6_3to6_4, a6_4to6_5, a6_5to6_6, a6_6to6_7, a6_7to6_8, a6_8to6_9, a6_9to6_10, a6_10to6_11, a6_11to6_12, a6_12to6_13, a6_13to6_14, a6_14to6_15, a6_15to6_16, a6_16to6_17, a6_17to6_18, a6_18to6_19, a6_19to6_20;
wire [`DWIDTH-1:0] a7_0to7_1, a7_1to7_2, a7_2to7_3, a7_3to7_4, a7_4to7_5, a7_5to7_6, a7_6to7_7, a7_7to7_8, a7_8to7_9, a7_9to7_10, a7_10to7_11, a7_11to7_12, a7_12to7_13, a7_13to7_14, a7_14to7_15, a7_15to7_16, a7_16to7_17, a7_17to7_18, a7_18to7_19, a7_19to7_20;
wire [`DWIDTH-1:0] a8_0to8_1, a8_1to8_2, a8_2to8_3, a8_3to8_4, a8_4to8_5, a8_5to8_6, a8_6to8_7, a8_7to8_8, a8_8to8_9, a8_9to8_10, a8_10to8_11, a8_11to8_12, a8_12to8_13, a8_13to8_14, a8_14to8_15, a8_15to8_16, a8_16to8_17, a8_17to8_18, a8_18to8_19, a8_19to8_20;
wire [`DWIDTH-1:0] a9_0to9_1, a9_1to9_2, a9_2to9_3, a9_3to9_4, a9_4to9_5, a9_5to9_6, a9_6to9_7, a9_7to9_8, a9_8to9_9, a9_9to9_10, a9_10to9_11, a9_11to9_12, a9_12to9_13, a9_13to9_14, a9_14to9_15, a9_15to9_16, a9_16to9_17, a9_17to9_18, a9_18to9_19, a9_19to9_20;
wire [`DWIDTH-1:0] a10_0to10_1, a10_1to10_2, a10_2to10_3, a10_3to10_4, a10_4to10_5, a10_5to10_6, a10_6to10_7, a10_7to10_8, a10_8to10_9, a10_9to10_10, a10_10to10_11, a10_11to10_12, a10_12to10_13, a10_13to10_14, a10_14to10_15, a10_15to10_16, a10_16to10_17, a10_17to10_18, a10_18to10_19, a10_19to10_20;
wire [`DWIDTH-1:0] a11_0to11_1, a11_1to11_2, a11_2to11_3, a11_3to11_4, a11_4to11_5, a11_5to11_6, a11_6to11_7, a11_7to11_8, a11_8to11_9, a11_9to11_10, a11_10to11_11, a11_11to11_12, a11_12to11_13, a11_13to11_14, a11_14to11_15, a11_15to11_16, a11_16to11_17, a11_17to11_18, a11_18to11_19, a11_19to11_20;
wire [`DWIDTH-1:0] a12_0to12_1, a12_1to12_2, a12_2to12_3, a12_3to12_4, a12_4to12_5, a12_5to12_6, a12_6to12_7, a12_7to12_8, a12_8to12_9, a12_9to12_10, a12_10to12_11, a12_11to12_12, a12_12to12_13, a12_13to12_14, a12_14to12_15, a12_15to12_16, a12_16to12_17, a12_17to12_18, a12_18to12_19, a12_19to12_20;
wire [`DWIDTH-1:0] a13_0to13_1, a13_1to13_2, a13_2to13_3, a13_3to13_4, a13_4to13_5, a13_5to13_6, a13_6to13_7, a13_7to13_8, a13_8to13_9, a13_9to13_10, a13_10to13_11, a13_11to13_12, a13_12to13_13, a13_13to13_14, a13_14to13_15, a13_15to13_16, a13_16to13_17, a13_17to13_18, a13_18to13_19, a13_19to13_20;
wire [`DWIDTH-1:0] a14_0to14_1, a14_1to14_2, a14_2to14_3, a14_3to14_4, a14_4to14_5, a14_5to14_6, a14_6to14_7, a14_7to14_8, a14_8to14_9, a14_9to14_10, a14_10to14_11, a14_11to14_12, a14_12to14_13, a14_13to14_14, a14_14to14_15, a14_15to14_16, a14_16to14_17, a14_17to14_18, a14_18to14_19, a14_19to14_20;
wire [`DWIDTH-1:0] a15_0to15_1, a15_1to15_2, a15_2to15_3, a15_3to15_4, a15_4to15_5, a15_5to15_6, a15_6to15_7, a15_7to15_8, a15_8to15_9, a15_9to15_10, a15_10to15_11, a15_11to15_12, a15_12to15_13, a15_13to15_14, a15_14to15_15, a15_15to15_16, a15_16to15_17, a15_17to15_18, a15_18to15_19, a15_19to15_20;
wire [`DWIDTH-1:0] a16_0to16_1, a16_1to16_2, a16_2to16_3, a16_3to16_4, a16_4to16_5, a16_5to16_6, a16_6to16_7, a16_7to16_8, a16_8to16_9, a16_9to16_10, a16_10to16_11, a16_11to16_12, a16_12to16_13, a16_13to16_14, a16_14to16_15, a16_15to16_16, a16_16to16_17, a16_17to16_18, a16_18to16_19, a16_19to16_20;
wire [`DWIDTH-1:0] a17_0to17_1, a17_1to17_2, a17_2to17_3, a17_3to17_4, a17_4to17_5, a17_5to17_6, a17_6to17_7, a17_7to17_8, a17_8to17_9, a17_9to17_10, a17_10to17_11, a17_11to17_12, a17_12to17_13, a17_13to17_14, a17_14to17_15, a17_15to17_16, a17_16to17_17, a17_17to17_18, a17_18to17_19, a17_19to17_20;
wire [`DWIDTH-1:0] a18_0to18_1, a18_1to18_2, a18_2to18_3, a18_3to18_4, a18_4to18_5, a18_5to18_6, a18_6to18_7, a18_7to18_8, a18_8to18_9, a18_9to18_10, a18_10to18_11, a18_11to18_12, a18_12to18_13, a18_13to18_14, a18_14to18_15, a18_15to18_16, a18_16to18_17, a18_17to18_18, a18_18to18_19, a18_19to18_20;
wire [`DWIDTH-1:0] a19_0to19_1, a19_1to19_2, a19_2to19_3, a19_3to19_4, a19_4to19_5, a19_5to19_6, a19_6to19_7, a19_7to19_8, a19_8to19_9, a19_9to19_10, a19_10to19_11, a19_11to19_12, a19_12to19_13, a19_13to19_14, a19_14to19_15, a19_15to19_16, a19_16to19_17, a19_17to19_18, a19_18to19_19, a19_19to19_20;

wire [`DWIDTH-1:0] b0_0to1_0, b1_0to2_0, b2_0to3_0, b3_0to4_0, b4_0to5_0, b5_0to6_0, b6_0to7_0, b7_0to8_0, b8_0to9_0, b9_0to10_0, b10_0to11_0, b11_0to12_0, b12_0to13_0, b13_0to14_0, b14_0to15_0, b15_0to16_0, b16_0to17_0, b17_0to18_0, b18_0to19_0, b19_0to20_0;
wire [`DWIDTH-1:0] b0_1to1_1, b1_1to2_1, b2_1to3_1, b3_1to4_1, b4_1to5_1, b5_1to6_1, b6_1to7_1, b7_1to8_1, b8_1to9_1, b9_1to10_1, b10_1to11_1, b11_1to12_1, b12_1to13_1, b13_1to14_1, b14_1to15_1, b15_1to16_1, b16_1to17_1, b17_1to18_1, b18_1to19_1, b19_1to20_1;
wire [`DWIDTH-1:0] b0_2to1_2, b1_2to2_2, b2_2to3_2, b3_2to4_2, b4_2to5_2, b5_2to6_2, b6_2to7_2, b7_2to8_2, b8_2to9_2, b9_2to10_2, b10_2to11_2, b11_2to12_2, b12_2to13_2, b13_2to14_2, b14_2to15_2, b15_2to16_2, b16_2to17_2, b17_2to18_2, b18_2to19_2, b19_2to20_2;
wire [`DWIDTH-1:0] b0_3to1_3, b1_3to2_3, b2_3to3_3, b3_3to4_3, b4_3to5_3, b5_3to6_3, b6_3to7_3, b7_3to8_3, b8_3to9_3, b9_3to10_3, b10_3to11_3, b11_3to12_3, b12_3to13_3, b13_3to14_3, b14_3to15_3, b15_3to16_3, b16_3to17_3, b17_3to18_3, b18_3to19_3, b19_3to20_3;
wire [`DWIDTH-1:0] b0_4to1_4, b1_4to2_4, b2_4to3_4, b3_4to4_4, b4_4to5_4, b5_4to6_4, b6_4to7_4, b7_4to8_4, b8_4to9_4, b9_4to10_4, b10_4to11_4, b11_4to12_4, b12_4to13_4, b13_4to14_4, b14_4to15_4, b15_4to16_4, b16_4to17_4, b17_4to18_4, b18_4to19_4, b19_4to20_4;
wire [`DWIDTH-1:0] b0_5to1_5, b1_5to2_5, b2_5to3_5, b3_5to4_5, b4_5to5_5, b5_5to6_5, b6_5to7_5, b7_5to8_5, b8_5to9_5, b9_5to10_5, b10_5to11_5, b11_5to12_5, b12_5to13_5, b13_5to14_5, b14_5to15_5, b15_5to16_5, b16_5to17_5, b17_5to18_5, b18_5to19_5, b19_5to20_5;
wire [`DWIDTH-1:0] b0_6to1_6, b1_6to2_6, b2_6to3_6, b3_6to4_6, b4_6to5_6, b5_6to6_6, b6_6to7_6, b7_6to8_6, b8_6to9_6, b9_6to10_6, b10_6to11_6, b11_6to12_6, b12_6to13_6, b13_6to14_6, b14_6to15_6, b15_6to16_6, b16_6to17_6, b17_6to18_6, b18_6to19_6, b19_6to20_6;
wire [`DWIDTH-1:0] b0_7to1_7, b1_7to2_7, b2_7to3_7, b3_7to4_7, b4_7to5_7, b5_7to6_7, b6_7to7_7, b7_7to8_7, b8_7to9_7, b9_7to10_7, b10_7to11_7, b11_7to12_7, b12_7to13_7, b13_7to14_7, b14_7to15_7, b15_7to16_7, b16_7to17_7, b17_7to18_7, b18_7to19_7, b19_7to20_7;
wire [`DWIDTH-1:0] b0_8to1_8, b1_8to2_8, b2_8to3_8, b3_8to4_8, b4_8to5_8, b5_8to6_8, b6_8to7_8, b7_8to8_8, b8_8to9_8, b9_8to10_8, b10_8to11_8, b11_8to12_8, b12_8to13_8, b13_8to14_8, b14_8to15_8, b15_8to16_8, b16_8to17_8, b17_8to18_8, b18_8to19_8, b19_8to20_8;
wire [`DWIDTH-1:0] b0_9to1_9, b1_9to2_9, b2_9to3_9, b3_9to4_9, b4_9to5_9, b5_9to6_9, b6_9to7_9, b7_9to8_9, b8_9to9_9, b9_9to10_9, b10_9to11_9, b11_9to12_9, b12_9to13_9, b13_9to14_9, b14_9to15_9, b15_9to16_9, b16_9to17_9, b17_9to18_9, b18_9to19_9, b19_9to20_9;
wire [`DWIDTH-1:0] b0_10to1_10, b1_10to2_10, b2_10to3_10, b3_10to4_10, b4_10to5_10, b5_10to6_10, b6_10to7_10, b7_10to8_10, b8_10to9_10, b9_10to10_10, b10_10to11_10, b11_10to12_10, b12_10to13_10, b13_10to14_10, b14_10to15_10, b15_10to16_10, b16_10to17_10, b17_10to18_10, b18_10to19_10, b19_10to20_10;
wire [`DWIDTH-1:0] b0_11to1_11, b1_11to2_11, b2_11to3_11, b3_11to4_11, b4_11to5_11, b5_11to6_11, b6_11to7_11, b7_11to8_11, b8_11to9_11, b9_11to10_11, b10_11to11_11, b11_11to12_11, b12_11to13_11, b13_11to14_11, b14_11to15_11, b15_11to16_11, b16_11to17_11, b17_11to18_11, b18_11to19_11, b19_11to20_11;
wire [`DWIDTH-1:0] b0_12to1_12, b1_12to2_12, b2_12to3_12, b3_12to4_12, b4_12to5_12, b5_12to6_12, b6_12to7_12, b7_12to8_12, b8_12to9_12, b9_12to10_12, b10_12to11_12, b11_12to12_12, b12_12to13_12, b13_12to14_12, b14_12to15_12, b15_12to16_12, b16_12to17_12, b17_12to18_12, b18_12to19_12, b19_12to20_12;
wire [`DWIDTH-1:0] b0_13to1_13, b1_13to2_13, b2_13to3_13, b3_13to4_13, b4_13to5_13, b5_13to6_13, b6_13to7_13, b7_13to8_13, b8_13to9_13, b9_13to10_13, b10_13to11_13, b11_13to12_13, b12_13to13_13, b13_13to14_13, b14_13to15_13, b15_13to16_13, b16_13to17_13, b17_13to18_13, b18_13to19_13, b19_13to20_13;
wire [`DWIDTH-1:0] b0_14to1_14, b1_14to2_14, b2_14to3_14, b3_14to4_14, b4_14to5_14, b5_14to6_14, b6_14to7_14, b7_14to8_14, b8_14to9_14, b9_14to10_14, b10_14to11_14, b11_14to12_14, b12_14to13_14, b13_14to14_14, b14_14to15_14, b15_14to16_14, b16_14to17_14, b17_14to18_14, b18_14to19_14, b19_14to20_14;
wire [`DWIDTH-1:0] b0_15to1_15, b1_15to2_15, b2_15to3_15, b3_15to4_15, b4_15to5_15, b5_15to6_15, b6_15to7_15, b7_15to8_15, b8_15to9_15, b9_15to10_15, b10_15to11_15, b11_15to12_15, b12_15to13_15, b13_15to14_15, b14_15to15_15, b15_15to16_15, b16_15to17_15, b17_15to18_15, b18_15to19_15, b19_15to20_15;
wire [`DWIDTH-1:0] b0_16to1_16, b1_16to2_16, b2_16to3_16, b3_16to4_16, b4_16to5_16, b5_16to6_16, b6_16to7_16, b7_16to8_16, b8_16to9_16, b9_16to10_16, b10_16to11_16, b11_16to12_16, b12_16to13_16, b13_16to14_16, b14_16to15_16, b15_16to16_16, b16_16to17_16, b17_16to18_16, b18_16to19_16, b19_16to20_16;
wire [`DWIDTH-1:0] b0_17to1_17, b1_17to2_17, b2_17to3_17, b3_17to4_17, b4_17to5_17, b5_17to6_17, b6_17to7_17, b7_17to8_17, b8_17to9_17, b9_17to10_17, b10_17to11_17, b11_17to12_17, b12_17to13_17, b13_17to14_17, b14_17to15_17, b15_17to16_17, b16_17to17_17, b17_17to18_17, b18_17to19_17, b19_17to20_17;
wire [`DWIDTH-1:0] b0_18to1_18, b1_18to2_18, b2_18to3_18, b3_18to4_18, b4_18to5_18, b5_18to6_18, b6_18to7_18, b7_18to8_18, b8_18to9_18, b9_18to10_18, b10_18to11_18, b11_18to12_18, b12_18to13_18, b13_18to14_18, b14_18to15_18, b15_18to16_18, b16_18to17_18, b17_18to18_18, b18_18to19_18, b19_18to20_18;
wire [`DWIDTH-1:0] b0_19to1_19, b1_19to2_19, b2_19to3_19, b3_19to4_19, b4_19to5_19, b5_19to6_19, b6_19to7_19, b7_19to8_19, b8_19to9_19, b9_19to10_19, b10_19to11_19, b11_19to12_19, b12_19to13_19, b13_19to14_19, b14_19to15_19, b15_19to16_19, b16_19to17_19, b17_19to18_19, b18_19to19_19, b19_19to20_19;

//////////////////////////////////////////////////////////////////////////
// Instantiations of the actual PEs
//////////////////////////////////////////////////////////////////////////
//For larger matmul, more PEs will be needed
wire effective_rst;
assign effective_rst = reset | pe_reset;

processing_element pe0_0(.reset(effective_rst), .clk(clk),  .in_a(a0),      .in_b(b0),  .out_a(a0_0to0_1), .out_b(b0_0to1_0), .out_c(matrixC0_0));
processing_element pe0_1(.reset(effective_rst), .clk(clk),  .in_a(a0_0to0_1), .in_b(b1),  .out_a(a0_1to0_2), .out_b(b0_1to1_1), .out_c(matrixC0_1));
processing_element pe0_2(.reset(effective_rst), .clk(clk),  .in_a(a0_1to0_2), .in_b(b2),  .out_a(a0_2to0_3), .out_b(b0_2to1_2), .out_c(matrixC0_2));
processing_element pe0_3(.reset(effective_rst), .clk(clk),  .in_a(a0_2to0_3), .in_b(b3),  .out_a(a0_3to0_4), .out_b(b0_3to1_3), .out_c(matrixC0_3));
processing_element pe0_4(.reset(effective_rst), .clk(clk),  .in_a(a0_3to0_4), .in_b(b4),  .out_a(a0_4to0_5), .out_b(b0_4to1_4), .out_c(matrixC0_4));
processing_element pe0_5(.reset(effective_rst), .clk(clk),  .in_a(a0_4to0_5), .in_b(b5),  .out_a(a0_5to0_6), .out_b(b0_5to1_5), .out_c(matrixC0_5));
processing_element pe0_6(.reset(effective_rst), .clk(clk),  .in_a(a0_5to0_6), .in_b(b6),  .out_a(a0_6to0_7), .out_b(b0_6to1_6), .out_c(matrixC0_6));
processing_element pe0_7(.reset(effective_rst), .clk(clk),  .in_a(a0_6to0_7), .in_b(b7),  .out_a(a0_7to0_8), .out_b(b0_7to1_7), .out_c(matrixC0_7));
processing_element pe0_8(.reset(effective_rst), .clk(clk),  .in_a(a0_7to0_8), .in_b(b8),  .out_a(a0_8to0_9), .out_b(b0_8to1_8), .out_c(matrixC0_8));
processing_element pe0_9(.reset(effective_rst), .clk(clk),  .in_a(a0_8to0_9), .in_b(b9),  .out_a(a0_9to0_10), .out_b(b0_9to1_9), .out_c(matrixC0_9));
processing_element pe0_10(.reset(effective_rst), .clk(clk),  .in_a(a0_9to0_10), .in_b(b10),  .out_a(a0_10to0_11), .out_b(b0_10to1_10), .out_c(matrixC0_10));
processing_element pe0_11(.reset(effective_rst), .clk(clk),  .in_a(a0_10to0_11), .in_b(b11),  .out_a(a0_11to0_12), .out_b(b0_11to1_11), .out_c(matrixC0_11));
processing_element pe0_12(.reset(effective_rst), .clk(clk),  .in_a(a0_11to0_12), .in_b(b12),  .out_a(a0_12to0_13), .out_b(b0_12to1_12), .out_c(matrixC0_12));
processing_element pe0_13(.reset(effective_rst), .clk(clk),  .in_a(a0_12to0_13), .in_b(b13),  .out_a(a0_13to0_14), .out_b(b0_13to1_13), .out_c(matrixC0_13));
processing_element pe0_14(.reset(effective_rst), .clk(clk),  .in_a(a0_13to0_14), .in_b(b14),  .out_a(a0_14to0_15), .out_b(b0_14to1_14), .out_c(matrixC0_14));
processing_element pe0_15(.reset(effective_rst), .clk(clk),  .in_a(a0_14to0_15), .in_b(b15),  .out_a(a0_15to0_16), .out_b(b0_15to1_15), .out_c(matrixC0_15));
processing_element pe0_16(.reset(effective_rst), .clk(clk),  .in_a(a0_15to0_16), .in_b(b16),  .out_a(a0_16to0_17), .out_b(b0_16to1_16), .out_c(matrixC0_16));
processing_element pe0_17(.reset(effective_rst), .clk(clk),  .in_a(a0_16to0_17), .in_b(b17),  .out_a(a0_17to0_18), .out_b(b0_17to1_17), .out_c(matrixC0_17));
processing_element pe0_18(.reset(effective_rst), .clk(clk),  .in_a(a0_17to0_18), .in_b(b18),  .out_a(a0_18to0_19), .out_b(b0_18to1_18), .out_c(matrixC0_18));
processing_element pe0_19(.reset(effective_rst), .clk(clk),  .in_a(a0_18to0_19), .in_b(b19),  .out_a(a0_19to0_20), .out_b(b0_19to1_19), .out_c(matrixC0_19));

processing_element pe1_0(.reset(effective_rst), .clk(clk),  .in_a(a1), .in_b(b0_0to1_0),  .out_a(a1_0to1_1), .out_b(b1_0to2_0), .out_c(matrixC1_0));
processing_element pe2_0(.reset(effective_rst), .clk(clk),  .in_a(a2), .in_b(b1_0to2_0),  .out_a(a2_0to2_1), .out_b(b2_0to3_0), .out_c(matrixC2_0));
processing_element pe3_0(.reset(effective_rst), .clk(clk),  .in_a(a3), .in_b(b2_0to3_0),  .out_a(a3_0to3_1), .out_b(b3_0to4_0), .out_c(matrixC3_0));
processing_element pe4_0(.reset(effective_rst), .clk(clk),  .in_a(a4), .in_b(b3_0to4_0),  .out_a(a4_0to4_1), .out_b(b4_0to5_0), .out_c(matrixC4_0));
processing_element pe5_0(.reset(effective_rst), .clk(clk),  .in_a(a5), .in_b(b4_0to5_0),  .out_a(a5_0to5_1), .out_b(b5_0to6_0), .out_c(matrixC5_0));
processing_element pe6_0(.reset(effective_rst), .clk(clk),  .in_a(a6), .in_b(b5_0to6_0),  .out_a(a6_0to6_1), .out_b(b6_0to7_0), .out_c(matrixC6_0));
processing_element pe7_0(.reset(effective_rst), .clk(clk),  .in_a(a7), .in_b(b6_0to7_0),  .out_a(a7_0to7_1), .out_b(b7_0to8_0), .out_c(matrixC7_0));
processing_element pe8_0(.reset(effective_rst), .clk(clk),  .in_a(a8), .in_b(b7_0to8_0),  .out_a(a8_0to8_1), .out_b(b8_0to9_0), .out_c(matrixC8_0));
processing_element pe9_0(.reset(effective_rst), .clk(clk),  .in_a(a9), .in_b(b8_0to9_0),  .out_a(a9_0to9_1), .out_b(b9_0to10_0), .out_c(matrixC9_0));
processing_element pe10_0(.reset(effective_rst), .clk(clk),  .in_a(a10), .in_b(b9_0to10_0),  .out_a(a10_0to10_1), .out_b(b10_0to11_0), .out_c(matrixC10_0));
processing_element pe11_0(.reset(effective_rst), .clk(clk),  .in_a(a11), .in_b(b10_0to11_0),  .out_a(a11_0to11_1), .out_b(b11_0to12_0), .out_c(matrixC11_0));
processing_element pe12_0(.reset(effective_rst), .clk(clk),  .in_a(a12), .in_b(b11_0to12_0),  .out_a(a12_0to12_1), .out_b(b12_0to13_0), .out_c(matrixC12_0));
processing_element pe13_0(.reset(effective_rst), .clk(clk),  .in_a(a13), .in_b(b12_0to13_0),  .out_a(a13_0to13_1), .out_b(b13_0to14_0), .out_c(matrixC13_0));
processing_element pe14_0(.reset(effective_rst), .clk(clk),  .in_a(a14), .in_b(b13_0to14_0),  .out_a(a14_0to14_1), .out_b(b14_0to15_0), .out_c(matrixC14_0));
processing_element pe15_0(.reset(effective_rst), .clk(clk),  .in_a(a15), .in_b(b14_0to15_0),  .out_a(a15_0to15_1), .out_b(b15_0to16_0), .out_c(matrixC15_0));
processing_element pe16_0(.reset(effective_rst), .clk(clk),  .in_a(a16), .in_b(b15_0to16_0),  .out_a(a16_0to16_1), .out_b(b16_0to17_0), .out_c(matrixC16_0));
processing_element pe17_0(.reset(effective_rst), .clk(clk),  .in_a(a17), .in_b(b16_0to17_0),  .out_a(a17_0to17_1), .out_b(b17_0to18_0), .out_c(matrixC17_0));
processing_element pe18_0(.reset(effective_rst), .clk(clk),  .in_a(a18), .in_b(b17_0to18_0),  .out_a(a18_0to18_1), .out_b(b18_0to19_0), .out_c(matrixC18_0));
processing_element pe19_0(.reset(effective_rst), .clk(clk),  .in_a(a19), .in_b(b18_0to19_0),  .out_a(a19_0to19_1), .out_b(b19_0to20_0), .out_c(matrixC19_0));

processing_element pe1_1(.reset(effective_rst), .clk(clk),  .in_a(a1_0to1_1), .in_b(b0_1to1_1),  .out_a(a1_1to1_2), .out_b(b1_1to2_1), .out_c(matrixC1_1));
processing_element pe1_2(.reset(effective_rst), .clk(clk),  .in_a(a1_1to1_2), .in_b(b0_2to1_2),  .out_a(a1_2to1_3), .out_b(b1_2to2_2), .out_c(matrixC1_2));
processing_element pe1_3(.reset(effective_rst), .clk(clk),  .in_a(a1_2to1_3), .in_b(b0_3to1_3),  .out_a(a1_3to1_4), .out_b(b1_3to2_3), .out_c(matrixC1_3));
processing_element pe1_4(.reset(effective_rst), .clk(clk),  .in_a(a1_3to1_4), .in_b(b0_4to1_4),  .out_a(a1_4to1_5), .out_b(b1_4to2_4), .out_c(matrixC1_4));
processing_element pe1_5(.reset(effective_rst), .clk(clk),  .in_a(a1_4to1_5), .in_b(b0_5to1_5),  .out_a(a1_5to1_6), .out_b(b1_5to2_5), .out_c(matrixC1_5));
processing_element pe1_6(.reset(effective_rst), .clk(clk),  .in_a(a1_5to1_6), .in_b(b0_6to1_6),  .out_a(a1_6to1_7), .out_b(b1_6to2_6), .out_c(matrixC1_6));
processing_element pe1_7(.reset(effective_rst), .clk(clk),  .in_a(a1_6to1_7), .in_b(b0_7to1_7),  .out_a(a1_7to1_8), .out_b(b1_7to2_7), .out_c(matrixC1_7));
processing_element pe1_8(.reset(effective_rst), .clk(clk),  .in_a(a1_7to1_8), .in_b(b0_8to1_8),  .out_a(a1_8to1_9), .out_b(b1_8to2_8), .out_c(matrixC1_8));
processing_element pe1_9(.reset(effective_rst), .clk(clk),  .in_a(a1_8to1_9), .in_b(b0_9to1_9),  .out_a(a1_9to1_10), .out_b(b1_9to2_9), .out_c(matrixC1_9));
processing_element pe1_10(.reset(effective_rst), .clk(clk),  .in_a(a1_9to1_10), .in_b(b0_10to1_10),  .out_a(a1_10to1_11), .out_b(b1_10to2_10), .out_c(matrixC1_10));
processing_element pe1_11(.reset(effective_rst), .clk(clk),  .in_a(a1_10to1_11), .in_b(b0_11to1_11),  .out_a(a1_11to1_12), .out_b(b1_11to2_11), .out_c(matrixC1_11));
processing_element pe1_12(.reset(effective_rst), .clk(clk),  .in_a(a1_11to1_12), .in_b(b0_12to1_12),  .out_a(a1_12to1_13), .out_b(b1_12to2_12), .out_c(matrixC1_12));
processing_element pe1_13(.reset(effective_rst), .clk(clk),  .in_a(a1_12to1_13), .in_b(b0_13to1_13),  .out_a(a1_13to1_14), .out_b(b1_13to2_13), .out_c(matrixC1_13));
processing_element pe1_14(.reset(effective_rst), .clk(clk),  .in_a(a1_13to1_14), .in_b(b0_14to1_14),  .out_a(a1_14to1_15), .out_b(b1_14to2_14), .out_c(matrixC1_14));
processing_element pe1_15(.reset(effective_rst), .clk(clk),  .in_a(a1_14to1_15), .in_b(b0_15to1_15),  .out_a(a1_15to1_16), .out_b(b1_15to2_15), .out_c(matrixC1_15));
processing_element pe1_16(.reset(effective_rst), .clk(clk),  .in_a(a1_15to1_16), .in_b(b0_16to1_16),  .out_a(a1_16to1_17), .out_b(b1_16to2_16), .out_c(matrixC1_16));
processing_element pe1_17(.reset(effective_rst), .clk(clk),  .in_a(a1_16to1_17), .in_b(b0_17to1_17),  .out_a(a1_17to1_18), .out_b(b1_17to2_17), .out_c(matrixC1_17));
processing_element pe1_18(.reset(effective_rst), .clk(clk),  .in_a(a1_17to1_18), .in_b(b0_18to1_18),  .out_a(a1_18to1_19), .out_b(b1_18to2_18), .out_c(matrixC1_18));
processing_element pe1_19(.reset(effective_rst), .clk(clk),  .in_a(a1_18to1_19), .in_b(b0_19to1_19),  .out_a(a1_19to1_20), .out_b(b1_19to2_19), .out_c(matrixC1_19));
processing_element pe2_1(.reset(effective_rst), .clk(clk),  .in_a(a2_0to2_1), .in_b(b1_1to2_1),  .out_a(a2_1to2_2), .out_b(b2_1to3_1), .out_c(matrixC2_1));
processing_element pe2_2(.reset(effective_rst), .clk(clk),  .in_a(a2_1to2_2), .in_b(b1_2to2_2),  .out_a(a2_2to2_3), .out_b(b2_2to3_2), .out_c(matrixC2_2));
processing_element pe2_3(.reset(effective_rst), .clk(clk),  .in_a(a2_2to2_3), .in_b(b1_3to2_3),  .out_a(a2_3to2_4), .out_b(b2_3to3_3), .out_c(matrixC2_3));
processing_element pe2_4(.reset(effective_rst), .clk(clk),  .in_a(a2_3to2_4), .in_b(b1_4to2_4),  .out_a(a2_4to2_5), .out_b(b2_4to3_4), .out_c(matrixC2_4));
processing_element pe2_5(.reset(effective_rst), .clk(clk),  .in_a(a2_4to2_5), .in_b(b1_5to2_5),  .out_a(a2_5to2_6), .out_b(b2_5to3_5), .out_c(matrixC2_5));
processing_element pe2_6(.reset(effective_rst), .clk(clk),  .in_a(a2_5to2_6), .in_b(b1_6to2_6),  .out_a(a2_6to2_7), .out_b(b2_6to3_6), .out_c(matrixC2_6));
processing_element pe2_7(.reset(effective_rst), .clk(clk),  .in_a(a2_6to2_7), .in_b(b1_7to2_7),  .out_a(a2_7to2_8), .out_b(b2_7to3_7), .out_c(matrixC2_7));
processing_element pe2_8(.reset(effective_rst), .clk(clk),  .in_a(a2_7to2_8), .in_b(b1_8to2_8),  .out_a(a2_8to2_9), .out_b(b2_8to3_8), .out_c(matrixC2_8));
processing_element pe2_9(.reset(effective_rst), .clk(clk),  .in_a(a2_8to2_9), .in_b(b1_9to2_9),  .out_a(a2_9to2_10), .out_b(b2_9to3_9), .out_c(matrixC2_9));
processing_element pe2_10(.reset(effective_rst), .clk(clk),  .in_a(a2_9to2_10), .in_b(b1_10to2_10),  .out_a(a2_10to2_11), .out_b(b2_10to3_10), .out_c(matrixC2_10));
processing_element pe2_11(.reset(effective_rst), .clk(clk),  .in_a(a2_10to2_11), .in_b(b1_11to2_11),  .out_a(a2_11to2_12), .out_b(b2_11to3_11), .out_c(matrixC2_11));
processing_element pe2_12(.reset(effective_rst), .clk(clk),  .in_a(a2_11to2_12), .in_b(b1_12to2_12),  .out_a(a2_12to2_13), .out_b(b2_12to3_12), .out_c(matrixC2_12));
processing_element pe2_13(.reset(effective_rst), .clk(clk),  .in_a(a2_12to2_13), .in_b(b1_13to2_13),  .out_a(a2_13to2_14), .out_b(b2_13to3_13), .out_c(matrixC2_13));
processing_element pe2_14(.reset(effective_rst), .clk(clk),  .in_a(a2_13to2_14), .in_b(b1_14to2_14),  .out_a(a2_14to2_15), .out_b(b2_14to3_14), .out_c(matrixC2_14));
processing_element pe2_15(.reset(effective_rst), .clk(clk),  .in_a(a2_14to2_15), .in_b(b1_15to2_15),  .out_a(a2_15to2_16), .out_b(b2_15to3_15), .out_c(matrixC2_15));
processing_element pe2_16(.reset(effective_rst), .clk(clk),  .in_a(a2_15to2_16), .in_b(b1_16to2_16),  .out_a(a2_16to2_17), .out_b(b2_16to3_16), .out_c(matrixC2_16));
processing_element pe2_17(.reset(effective_rst), .clk(clk),  .in_a(a2_16to2_17), .in_b(b1_17to2_17),  .out_a(a2_17to2_18), .out_b(b2_17to3_17), .out_c(matrixC2_17));
processing_element pe2_18(.reset(effective_rst), .clk(clk),  .in_a(a2_17to2_18), .in_b(b1_18to2_18),  .out_a(a2_18to2_19), .out_b(b2_18to3_18), .out_c(matrixC2_18));
processing_element pe2_19(.reset(effective_rst), .clk(clk),  .in_a(a2_18to2_19), .in_b(b1_19to2_19),  .out_a(a2_19to2_20), .out_b(b2_19to3_19), .out_c(matrixC2_19));
processing_element pe3_1(.reset(effective_rst), .clk(clk),  .in_a(a3_0to3_1), .in_b(b2_1to3_1),  .out_a(a3_1to3_2), .out_b(b3_1to4_1), .out_c(matrixC3_1));
processing_element pe3_2(.reset(effective_rst), .clk(clk),  .in_a(a3_1to3_2), .in_b(b2_2to3_2),  .out_a(a3_2to3_3), .out_b(b3_2to4_2), .out_c(matrixC3_2));
processing_element pe3_3(.reset(effective_rst), .clk(clk),  .in_a(a3_2to3_3), .in_b(b2_3to3_3),  .out_a(a3_3to3_4), .out_b(b3_3to4_3), .out_c(matrixC3_3));
processing_element pe3_4(.reset(effective_rst), .clk(clk),  .in_a(a3_3to3_4), .in_b(b2_4to3_4),  .out_a(a3_4to3_5), .out_b(b3_4to4_4), .out_c(matrixC3_4));
processing_element pe3_5(.reset(effective_rst), .clk(clk),  .in_a(a3_4to3_5), .in_b(b2_5to3_5),  .out_a(a3_5to3_6), .out_b(b3_5to4_5), .out_c(matrixC3_5));
processing_element pe3_6(.reset(effective_rst), .clk(clk),  .in_a(a3_5to3_6), .in_b(b2_6to3_6),  .out_a(a3_6to3_7), .out_b(b3_6to4_6), .out_c(matrixC3_6));
processing_element pe3_7(.reset(effective_rst), .clk(clk),  .in_a(a3_6to3_7), .in_b(b2_7to3_7),  .out_a(a3_7to3_8), .out_b(b3_7to4_7), .out_c(matrixC3_7));
processing_element pe3_8(.reset(effective_rst), .clk(clk),  .in_a(a3_7to3_8), .in_b(b2_8to3_8),  .out_a(a3_8to3_9), .out_b(b3_8to4_8), .out_c(matrixC3_8));
processing_element pe3_9(.reset(effective_rst), .clk(clk),  .in_a(a3_8to3_9), .in_b(b2_9to3_9),  .out_a(a3_9to3_10), .out_b(b3_9to4_9), .out_c(matrixC3_9));
processing_element pe3_10(.reset(effective_rst), .clk(clk),  .in_a(a3_9to3_10), .in_b(b2_10to3_10),  .out_a(a3_10to3_11), .out_b(b3_10to4_10), .out_c(matrixC3_10));
processing_element pe3_11(.reset(effective_rst), .clk(clk),  .in_a(a3_10to3_11), .in_b(b2_11to3_11),  .out_a(a3_11to3_12), .out_b(b3_11to4_11), .out_c(matrixC3_11));
processing_element pe3_12(.reset(effective_rst), .clk(clk),  .in_a(a3_11to3_12), .in_b(b2_12to3_12),  .out_a(a3_12to3_13), .out_b(b3_12to4_12), .out_c(matrixC3_12));
processing_element pe3_13(.reset(effective_rst), .clk(clk),  .in_a(a3_12to3_13), .in_b(b2_13to3_13),  .out_a(a3_13to3_14), .out_b(b3_13to4_13), .out_c(matrixC3_13));
processing_element pe3_14(.reset(effective_rst), .clk(clk),  .in_a(a3_13to3_14), .in_b(b2_14to3_14),  .out_a(a3_14to3_15), .out_b(b3_14to4_14), .out_c(matrixC3_14));
processing_element pe3_15(.reset(effective_rst), .clk(clk),  .in_a(a3_14to3_15), .in_b(b2_15to3_15),  .out_a(a3_15to3_16), .out_b(b3_15to4_15), .out_c(matrixC3_15));
processing_element pe3_16(.reset(effective_rst), .clk(clk),  .in_a(a3_15to3_16), .in_b(b2_16to3_16),  .out_a(a3_16to3_17), .out_b(b3_16to4_16), .out_c(matrixC3_16));
processing_element pe3_17(.reset(effective_rst), .clk(clk),  .in_a(a3_16to3_17), .in_b(b2_17to3_17),  .out_a(a3_17to3_18), .out_b(b3_17to4_17), .out_c(matrixC3_17));
processing_element pe3_18(.reset(effective_rst), .clk(clk),  .in_a(a3_17to3_18), .in_b(b2_18to3_18),  .out_a(a3_18to3_19), .out_b(b3_18to4_18), .out_c(matrixC3_18));
processing_element pe3_19(.reset(effective_rst), .clk(clk),  .in_a(a3_18to3_19), .in_b(b2_19to3_19),  .out_a(a3_19to3_20), .out_b(b3_19to4_19), .out_c(matrixC3_19));
processing_element pe4_1(.reset(effective_rst), .clk(clk),  .in_a(a4_0to4_1), .in_b(b3_1to4_1),  .out_a(a4_1to4_2), .out_b(b4_1to5_1), .out_c(matrixC4_1));
processing_element pe4_2(.reset(effective_rst), .clk(clk),  .in_a(a4_1to4_2), .in_b(b3_2to4_2),  .out_a(a4_2to4_3), .out_b(b4_2to5_2), .out_c(matrixC4_2));
processing_element pe4_3(.reset(effective_rst), .clk(clk),  .in_a(a4_2to4_3), .in_b(b3_3to4_3),  .out_a(a4_3to4_4), .out_b(b4_3to5_3), .out_c(matrixC4_3));
processing_element pe4_4(.reset(effective_rst), .clk(clk),  .in_a(a4_3to4_4), .in_b(b3_4to4_4),  .out_a(a4_4to4_5), .out_b(b4_4to5_4), .out_c(matrixC4_4));
processing_element pe4_5(.reset(effective_rst), .clk(clk),  .in_a(a4_4to4_5), .in_b(b3_5to4_5),  .out_a(a4_5to4_6), .out_b(b4_5to5_5), .out_c(matrixC4_5));
processing_element pe4_6(.reset(effective_rst), .clk(clk),  .in_a(a4_5to4_6), .in_b(b3_6to4_6),  .out_a(a4_6to4_7), .out_b(b4_6to5_6), .out_c(matrixC4_6));
processing_element pe4_7(.reset(effective_rst), .clk(clk),  .in_a(a4_6to4_7), .in_b(b3_7to4_7),  .out_a(a4_7to4_8), .out_b(b4_7to5_7), .out_c(matrixC4_7));
processing_element pe4_8(.reset(effective_rst), .clk(clk),  .in_a(a4_7to4_8), .in_b(b3_8to4_8),  .out_a(a4_8to4_9), .out_b(b4_8to5_8), .out_c(matrixC4_8));
processing_element pe4_9(.reset(effective_rst), .clk(clk),  .in_a(a4_8to4_9), .in_b(b3_9to4_9),  .out_a(a4_9to4_10), .out_b(b4_9to5_9), .out_c(matrixC4_9));
processing_element pe4_10(.reset(effective_rst), .clk(clk),  .in_a(a4_9to4_10), .in_b(b3_10to4_10),  .out_a(a4_10to4_11), .out_b(b4_10to5_10), .out_c(matrixC4_10));
processing_element pe4_11(.reset(effective_rst), .clk(clk),  .in_a(a4_10to4_11), .in_b(b3_11to4_11),  .out_a(a4_11to4_12), .out_b(b4_11to5_11), .out_c(matrixC4_11));
processing_element pe4_12(.reset(effective_rst), .clk(clk),  .in_a(a4_11to4_12), .in_b(b3_12to4_12),  .out_a(a4_12to4_13), .out_b(b4_12to5_12), .out_c(matrixC4_12));
processing_element pe4_13(.reset(effective_rst), .clk(clk),  .in_a(a4_12to4_13), .in_b(b3_13to4_13),  .out_a(a4_13to4_14), .out_b(b4_13to5_13), .out_c(matrixC4_13));
processing_element pe4_14(.reset(effective_rst), .clk(clk),  .in_a(a4_13to4_14), .in_b(b3_14to4_14),  .out_a(a4_14to4_15), .out_b(b4_14to5_14), .out_c(matrixC4_14));
processing_element pe4_15(.reset(effective_rst), .clk(clk),  .in_a(a4_14to4_15), .in_b(b3_15to4_15),  .out_a(a4_15to4_16), .out_b(b4_15to5_15), .out_c(matrixC4_15));
processing_element pe4_16(.reset(effective_rst), .clk(clk),  .in_a(a4_15to4_16), .in_b(b3_16to4_16),  .out_a(a4_16to4_17), .out_b(b4_16to5_16), .out_c(matrixC4_16));
processing_element pe4_17(.reset(effective_rst), .clk(clk),  .in_a(a4_16to4_17), .in_b(b3_17to4_17),  .out_a(a4_17to4_18), .out_b(b4_17to5_17), .out_c(matrixC4_17));
processing_element pe4_18(.reset(effective_rst), .clk(clk),  .in_a(a4_17to4_18), .in_b(b3_18to4_18),  .out_a(a4_18to4_19), .out_b(b4_18to5_18), .out_c(matrixC4_18));
processing_element pe4_19(.reset(effective_rst), .clk(clk),  .in_a(a4_18to4_19), .in_b(b3_19to4_19),  .out_a(a4_19to4_20), .out_b(b4_19to5_19), .out_c(matrixC4_19));
processing_element pe5_1(.reset(effective_rst), .clk(clk),  .in_a(a5_0to5_1), .in_b(b4_1to5_1),  .out_a(a5_1to5_2), .out_b(b5_1to6_1), .out_c(matrixC5_1));
processing_element pe5_2(.reset(effective_rst), .clk(clk),  .in_a(a5_1to5_2), .in_b(b4_2to5_2),  .out_a(a5_2to5_3), .out_b(b5_2to6_2), .out_c(matrixC5_2));
processing_element pe5_3(.reset(effective_rst), .clk(clk),  .in_a(a5_2to5_3), .in_b(b4_3to5_3),  .out_a(a5_3to5_4), .out_b(b5_3to6_3), .out_c(matrixC5_3));
processing_element pe5_4(.reset(effective_rst), .clk(clk),  .in_a(a5_3to5_4), .in_b(b4_4to5_4),  .out_a(a5_4to5_5), .out_b(b5_4to6_4), .out_c(matrixC5_4));
processing_element pe5_5(.reset(effective_rst), .clk(clk),  .in_a(a5_4to5_5), .in_b(b4_5to5_5),  .out_a(a5_5to5_6), .out_b(b5_5to6_5), .out_c(matrixC5_5));
processing_element pe5_6(.reset(effective_rst), .clk(clk),  .in_a(a5_5to5_6), .in_b(b4_6to5_6),  .out_a(a5_6to5_7), .out_b(b5_6to6_6), .out_c(matrixC5_6));
processing_element pe5_7(.reset(effective_rst), .clk(clk),  .in_a(a5_6to5_7), .in_b(b4_7to5_7),  .out_a(a5_7to5_8), .out_b(b5_7to6_7), .out_c(matrixC5_7));
processing_element pe5_8(.reset(effective_rst), .clk(clk),  .in_a(a5_7to5_8), .in_b(b4_8to5_8),  .out_a(a5_8to5_9), .out_b(b5_8to6_8), .out_c(matrixC5_8));
processing_element pe5_9(.reset(effective_rst), .clk(clk),  .in_a(a5_8to5_9), .in_b(b4_9to5_9),  .out_a(a5_9to5_10), .out_b(b5_9to6_9), .out_c(matrixC5_9));
processing_element pe5_10(.reset(effective_rst), .clk(clk),  .in_a(a5_9to5_10), .in_b(b4_10to5_10),  .out_a(a5_10to5_11), .out_b(b5_10to6_10), .out_c(matrixC5_10));
processing_element pe5_11(.reset(effective_rst), .clk(clk),  .in_a(a5_10to5_11), .in_b(b4_11to5_11),  .out_a(a5_11to5_12), .out_b(b5_11to6_11), .out_c(matrixC5_11));
processing_element pe5_12(.reset(effective_rst), .clk(clk),  .in_a(a5_11to5_12), .in_b(b4_12to5_12),  .out_a(a5_12to5_13), .out_b(b5_12to6_12), .out_c(matrixC5_12));
processing_element pe5_13(.reset(effective_rst), .clk(clk),  .in_a(a5_12to5_13), .in_b(b4_13to5_13),  .out_a(a5_13to5_14), .out_b(b5_13to6_13), .out_c(matrixC5_13));
processing_element pe5_14(.reset(effective_rst), .clk(clk),  .in_a(a5_13to5_14), .in_b(b4_14to5_14),  .out_a(a5_14to5_15), .out_b(b5_14to6_14), .out_c(matrixC5_14));
processing_element pe5_15(.reset(effective_rst), .clk(clk),  .in_a(a5_14to5_15), .in_b(b4_15to5_15),  .out_a(a5_15to5_16), .out_b(b5_15to6_15), .out_c(matrixC5_15));
processing_element pe5_16(.reset(effective_rst), .clk(clk),  .in_a(a5_15to5_16), .in_b(b4_16to5_16),  .out_a(a5_16to5_17), .out_b(b5_16to6_16), .out_c(matrixC5_16));
processing_element pe5_17(.reset(effective_rst), .clk(clk),  .in_a(a5_16to5_17), .in_b(b4_17to5_17),  .out_a(a5_17to5_18), .out_b(b5_17to6_17), .out_c(matrixC5_17));
processing_element pe5_18(.reset(effective_rst), .clk(clk),  .in_a(a5_17to5_18), .in_b(b4_18to5_18),  .out_a(a5_18to5_19), .out_b(b5_18to6_18), .out_c(matrixC5_18));
processing_element pe5_19(.reset(effective_rst), .clk(clk),  .in_a(a5_18to5_19), .in_b(b4_19to5_19),  .out_a(a5_19to5_20), .out_b(b5_19to6_19), .out_c(matrixC5_19));
processing_element pe6_1(.reset(effective_rst), .clk(clk),  .in_a(a6_0to6_1), .in_b(b5_1to6_1),  .out_a(a6_1to6_2), .out_b(b6_1to7_1), .out_c(matrixC6_1));
processing_element pe6_2(.reset(effective_rst), .clk(clk),  .in_a(a6_1to6_2), .in_b(b5_2to6_2),  .out_a(a6_2to6_3), .out_b(b6_2to7_2), .out_c(matrixC6_2));
processing_element pe6_3(.reset(effective_rst), .clk(clk),  .in_a(a6_2to6_3), .in_b(b5_3to6_3),  .out_a(a6_3to6_4), .out_b(b6_3to7_3), .out_c(matrixC6_3));
processing_element pe6_4(.reset(effective_rst), .clk(clk),  .in_a(a6_3to6_4), .in_b(b5_4to6_4),  .out_a(a6_4to6_5), .out_b(b6_4to7_4), .out_c(matrixC6_4));
processing_element pe6_5(.reset(effective_rst), .clk(clk),  .in_a(a6_4to6_5), .in_b(b5_5to6_5),  .out_a(a6_5to6_6), .out_b(b6_5to7_5), .out_c(matrixC6_5));
processing_element pe6_6(.reset(effective_rst), .clk(clk),  .in_a(a6_5to6_6), .in_b(b5_6to6_6),  .out_a(a6_6to6_7), .out_b(b6_6to7_6), .out_c(matrixC6_6));
processing_element pe6_7(.reset(effective_rst), .clk(clk),  .in_a(a6_6to6_7), .in_b(b5_7to6_7),  .out_a(a6_7to6_8), .out_b(b6_7to7_7), .out_c(matrixC6_7));
processing_element pe6_8(.reset(effective_rst), .clk(clk),  .in_a(a6_7to6_8), .in_b(b5_8to6_8),  .out_a(a6_8to6_9), .out_b(b6_8to7_8), .out_c(matrixC6_8));
processing_element pe6_9(.reset(effective_rst), .clk(clk),  .in_a(a6_8to6_9), .in_b(b5_9to6_9),  .out_a(a6_9to6_10), .out_b(b6_9to7_9), .out_c(matrixC6_9));
processing_element pe6_10(.reset(effective_rst), .clk(clk),  .in_a(a6_9to6_10), .in_b(b5_10to6_10),  .out_a(a6_10to6_11), .out_b(b6_10to7_10), .out_c(matrixC6_10));
processing_element pe6_11(.reset(effective_rst), .clk(clk),  .in_a(a6_10to6_11), .in_b(b5_11to6_11),  .out_a(a6_11to6_12), .out_b(b6_11to7_11), .out_c(matrixC6_11));
processing_element pe6_12(.reset(effective_rst), .clk(clk),  .in_a(a6_11to6_12), .in_b(b5_12to6_12),  .out_a(a6_12to6_13), .out_b(b6_12to7_12), .out_c(matrixC6_12));
processing_element pe6_13(.reset(effective_rst), .clk(clk),  .in_a(a6_12to6_13), .in_b(b5_13to6_13),  .out_a(a6_13to6_14), .out_b(b6_13to7_13), .out_c(matrixC6_13));
processing_element pe6_14(.reset(effective_rst), .clk(clk),  .in_a(a6_13to6_14), .in_b(b5_14to6_14),  .out_a(a6_14to6_15), .out_b(b6_14to7_14), .out_c(matrixC6_14));
processing_element pe6_15(.reset(effective_rst), .clk(clk),  .in_a(a6_14to6_15), .in_b(b5_15to6_15),  .out_a(a6_15to6_16), .out_b(b6_15to7_15), .out_c(matrixC6_15));
processing_element pe6_16(.reset(effective_rst), .clk(clk),  .in_a(a6_15to6_16), .in_b(b5_16to6_16),  .out_a(a6_16to6_17), .out_b(b6_16to7_16), .out_c(matrixC6_16));
processing_element pe6_17(.reset(effective_rst), .clk(clk),  .in_a(a6_16to6_17), .in_b(b5_17to6_17),  .out_a(a6_17to6_18), .out_b(b6_17to7_17), .out_c(matrixC6_17));
processing_element pe6_18(.reset(effective_rst), .clk(clk),  .in_a(a6_17to6_18), .in_b(b5_18to6_18),  .out_a(a6_18to6_19), .out_b(b6_18to7_18), .out_c(matrixC6_18));
processing_element pe6_19(.reset(effective_rst), .clk(clk),  .in_a(a6_18to6_19), .in_b(b5_19to6_19),  .out_a(a6_19to6_20), .out_b(b6_19to7_19), .out_c(matrixC6_19));
processing_element pe7_1(.reset(effective_rst), .clk(clk),  .in_a(a7_0to7_1), .in_b(b6_1to7_1),  .out_a(a7_1to7_2), .out_b(b7_1to8_1), .out_c(matrixC7_1));
processing_element pe7_2(.reset(effective_rst), .clk(clk),  .in_a(a7_1to7_2), .in_b(b6_2to7_2),  .out_a(a7_2to7_3), .out_b(b7_2to8_2), .out_c(matrixC7_2));
processing_element pe7_3(.reset(effective_rst), .clk(clk),  .in_a(a7_2to7_3), .in_b(b6_3to7_3),  .out_a(a7_3to7_4), .out_b(b7_3to8_3), .out_c(matrixC7_3));
processing_element pe7_4(.reset(effective_rst), .clk(clk),  .in_a(a7_3to7_4), .in_b(b6_4to7_4),  .out_a(a7_4to7_5), .out_b(b7_4to8_4), .out_c(matrixC7_4));
processing_element pe7_5(.reset(effective_rst), .clk(clk),  .in_a(a7_4to7_5), .in_b(b6_5to7_5),  .out_a(a7_5to7_6), .out_b(b7_5to8_5), .out_c(matrixC7_5));
processing_element pe7_6(.reset(effective_rst), .clk(clk),  .in_a(a7_5to7_6), .in_b(b6_6to7_6),  .out_a(a7_6to7_7), .out_b(b7_6to8_6), .out_c(matrixC7_6));
processing_element pe7_7(.reset(effective_rst), .clk(clk),  .in_a(a7_6to7_7), .in_b(b6_7to7_7),  .out_a(a7_7to7_8), .out_b(b7_7to8_7), .out_c(matrixC7_7));
processing_element pe7_8(.reset(effective_rst), .clk(clk),  .in_a(a7_7to7_8), .in_b(b6_8to7_8),  .out_a(a7_8to7_9), .out_b(b7_8to8_8), .out_c(matrixC7_8));
processing_element pe7_9(.reset(effective_rst), .clk(clk),  .in_a(a7_8to7_9), .in_b(b6_9to7_9),  .out_a(a7_9to7_10), .out_b(b7_9to8_9), .out_c(matrixC7_9));
processing_element pe7_10(.reset(effective_rst), .clk(clk),  .in_a(a7_9to7_10), .in_b(b6_10to7_10),  .out_a(a7_10to7_11), .out_b(b7_10to8_10), .out_c(matrixC7_10));
processing_element pe7_11(.reset(effective_rst), .clk(clk),  .in_a(a7_10to7_11), .in_b(b6_11to7_11),  .out_a(a7_11to7_12), .out_b(b7_11to8_11), .out_c(matrixC7_11));
processing_element pe7_12(.reset(effective_rst), .clk(clk),  .in_a(a7_11to7_12), .in_b(b6_12to7_12),  .out_a(a7_12to7_13), .out_b(b7_12to8_12), .out_c(matrixC7_12));
processing_element pe7_13(.reset(effective_rst), .clk(clk),  .in_a(a7_12to7_13), .in_b(b6_13to7_13),  .out_a(a7_13to7_14), .out_b(b7_13to8_13), .out_c(matrixC7_13));
processing_element pe7_14(.reset(effective_rst), .clk(clk),  .in_a(a7_13to7_14), .in_b(b6_14to7_14),  .out_a(a7_14to7_15), .out_b(b7_14to8_14), .out_c(matrixC7_14));
processing_element pe7_15(.reset(effective_rst), .clk(clk),  .in_a(a7_14to7_15), .in_b(b6_15to7_15),  .out_a(a7_15to7_16), .out_b(b7_15to8_15), .out_c(matrixC7_15));
processing_element pe7_16(.reset(effective_rst), .clk(clk),  .in_a(a7_15to7_16), .in_b(b6_16to7_16),  .out_a(a7_16to7_17), .out_b(b7_16to8_16), .out_c(matrixC7_16));
processing_element pe7_17(.reset(effective_rst), .clk(clk),  .in_a(a7_16to7_17), .in_b(b6_17to7_17),  .out_a(a7_17to7_18), .out_b(b7_17to8_17), .out_c(matrixC7_17));
processing_element pe7_18(.reset(effective_rst), .clk(clk),  .in_a(a7_17to7_18), .in_b(b6_18to7_18),  .out_a(a7_18to7_19), .out_b(b7_18to8_18), .out_c(matrixC7_18));
processing_element pe7_19(.reset(effective_rst), .clk(clk),  .in_a(a7_18to7_19), .in_b(b6_19to7_19),  .out_a(a7_19to7_20), .out_b(b7_19to8_19), .out_c(matrixC7_19));
processing_element pe8_1(.reset(effective_rst), .clk(clk),  .in_a(a8_0to8_1), .in_b(b7_1to8_1),  .out_a(a8_1to8_2), .out_b(b8_1to9_1), .out_c(matrixC8_1));
processing_element pe8_2(.reset(effective_rst), .clk(clk),  .in_a(a8_1to8_2), .in_b(b7_2to8_2),  .out_a(a8_2to8_3), .out_b(b8_2to9_2), .out_c(matrixC8_2));
processing_element pe8_3(.reset(effective_rst), .clk(clk),  .in_a(a8_2to8_3), .in_b(b7_3to8_3),  .out_a(a8_3to8_4), .out_b(b8_3to9_3), .out_c(matrixC8_3));
processing_element pe8_4(.reset(effective_rst), .clk(clk),  .in_a(a8_3to8_4), .in_b(b7_4to8_4),  .out_a(a8_4to8_5), .out_b(b8_4to9_4), .out_c(matrixC8_4));
processing_element pe8_5(.reset(effective_rst), .clk(clk),  .in_a(a8_4to8_5), .in_b(b7_5to8_5),  .out_a(a8_5to8_6), .out_b(b8_5to9_5), .out_c(matrixC8_5));
processing_element pe8_6(.reset(effective_rst), .clk(clk),  .in_a(a8_5to8_6), .in_b(b7_6to8_6),  .out_a(a8_6to8_7), .out_b(b8_6to9_6), .out_c(matrixC8_6));
processing_element pe8_7(.reset(effective_rst), .clk(clk),  .in_a(a8_6to8_7), .in_b(b7_7to8_7),  .out_a(a8_7to8_8), .out_b(b8_7to9_7), .out_c(matrixC8_7));
processing_element pe8_8(.reset(effective_rst), .clk(clk),  .in_a(a8_7to8_8), .in_b(b7_8to8_8),  .out_a(a8_8to8_9), .out_b(b8_8to9_8), .out_c(matrixC8_8));
processing_element pe8_9(.reset(effective_rst), .clk(clk),  .in_a(a8_8to8_9), .in_b(b7_9to8_9),  .out_a(a8_9to8_10), .out_b(b8_9to9_9), .out_c(matrixC8_9));
processing_element pe8_10(.reset(effective_rst), .clk(clk),  .in_a(a8_9to8_10), .in_b(b7_10to8_10),  .out_a(a8_10to8_11), .out_b(b8_10to9_10), .out_c(matrixC8_10));
processing_element pe8_11(.reset(effective_rst), .clk(clk),  .in_a(a8_10to8_11), .in_b(b7_11to8_11),  .out_a(a8_11to8_12), .out_b(b8_11to9_11), .out_c(matrixC8_11));
processing_element pe8_12(.reset(effective_rst), .clk(clk),  .in_a(a8_11to8_12), .in_b(b7_12to8_12),  .out_a(a8_12to8_13), .out_b(b8_12to9_12), .out_c(matrixC8_12));
processing_element pe8_13(.reset(effective_rst), .clk(clk),  .in_a(a8_12to8_13), .in_b(b7_13to8_13),  .out_a(a8_13to8_14), .out_b(b8_13to9_13), .out_c(matrixC8_13));
processing_element pe8_14(.reset(effective_rst), .clk(clk),  .in_a(a8_13to8_14), .in_b(b7_14to8_14),  .out_a(a8_14to8_15), .out_b(b8_14to9_14), .out_c(matrixC8_14));
processing_element pe8_15(.reset(effective_rst), .clk(clk),  .in_a(a8_14to8_15), .in_b(b7_15to8_15),  .out_a(a8_15to8_16), .out_b(b8_15to9_15), .out_c(matrixC8_15));
processing_element pe8_16(.reset(effective_rst), .clk(clk),  .in_a(a8_15to8_16), .in_b(b7_16to8_16),  .out_a(a8_16to8_17), .out_b(b8_16to9_16), .out_c(matrixC8_16));
processing_element pe8_17(.reset(effective_rst), .clk(clk),  .in_a(a8_16to8_17), .in_b(b7_17to8_17),  .out_a(a8_17to8_18), .out_b(b8_17to9_17), .out_c(matrixC8_17));
processing_element pe8_18(.reset(effective_rst), .clk(clk),  .in_a(a8_17to8_18), .in_b(b7_18to8_18),  .out_a(a8_18to8_19), .out_b(b8_18to9_18), .out_c(matrixC8_18));
processing_element pe8_19(.reset(effective_rst), .clk(clk),  .in_a(a8_18to8_19), .in_b(b7_19to8_19),  .out_a(a8_19to8_20), .out_b(b8_19to9_19), .out_c(matrixC8_19));
processing_element pe9_1(.reset(effective_rst), .clk(clk),  .in_a(a9_0to9_1), .in_b(b8_1to9_1),  .out_a(a9_1to9_2), .out_b(b9_1to10_1), .out_c(matrixC9_1));
processing_element pe9_2(.reset(effective_rst), .clk(clk),  .in_a(a9_1to9_2), .in_b(b8_2to9_2),  .out_a(a9_2to9_3), .out_b(b9_2to10_2), .out_c(matrixC9_2));
processing_element pe9_3(.reset(effective_rst), .clk(clk),  .in_a(a9_2to9_3), .in_b(b8_3to9_3),  .out_a(a9_3to9_4), .out_b(b9_3to10_3), .out_c(matrixC9_3));
processing_element pe9_4(.reset(effective_rst), .clk(clk),  .in_a(a9_3to9_4), .in_b(b8_4to9_4),  .out_a(a9_4to9_5), .out_b(b9_4to10_4), .out_c(matrixC9_4));
processing_element pe9_5(.reset(effective_rst), .clk(clk),  .in_a(a9_4to9_5), .in_b(b8_5to9_5),  .out_a(a9_5to9_6), .out_b(b9_5to10_5), .out_c(matrixC9_5));
processing_element pe9_6(.reset(effective_rst), .clk(clk),  .in_a(a9_5to9_6), .in_b(b8_6to9_6),  .out_a(a9_6to9_7), .out_b(b9_6to10_6), .out_c(matrixC9_6));
processing_element pe9_7(.reset(effective_rst), .clk(clk),  .in_a(a9_6to9_7), .in_b(b8_7to9_7),  .out_a(a9_7to9_8), .out_b(b9_7to10_7), .out_c(matrixC9_7));
processing_element pe9_8(.reset(effective_rst), .clk(clk),  .in_a(a9_7to9_8), .in_b(b8_8to9_8),  .out_a(a9_8to9_9), .out_b(b9_8to10_8), .out_c(matrixC9_8));
processing_element pe9_9(.reset(effective_rst), .clk(clk),  .in_a(a9_8to9_9), .in_b(b8_9to9_9),  .out_a(a9_9to9_10), .out_b(b9_9to10_9), .out_c(matrixC9_9));
processing_element pe9_10(.reset(effective_rst), .clk(clk),  .in_a(a9_9to9_10), .in_b(b8_10to9_10),  .out_a(a9_10to9_11), .out_b(b9_10to10_10), .out_c(matrixC9_10));
processing_element pe9_11(.reset(effective_rst), .clk(clk),  .in_a(a9_10to9_11), .in_b(b8_11to9_11),  .out_a(a9_11to9_12), .out_b(b9_11to10_11), .out_c(matrixC9_11));
processing_element pe9_12(.reset(effective_rst), .clk(clk),  .in_a(a9_11to9_12), .in_b(b8_12to9_12),  .out_a(a9_12to9_13), .out_b(b9_12to10_12), .out_c(matrixC9_12));
processing_element pe9_13(.reset(effective_rst), .clk(clk),  .in_a(a9_12to9_13), .in_b(b8_13to9_13),  .out_a(a9_13to9_14), .out_b(b9_13to10_13), .out_c(matrixC9_13));
processing_element pe9_14(.reset(effective_rst), .clk(clk),  .in_a(a9_13to9_14), .in_b(b8_14to9_14),  .out_a(a9_14to9_15), .out_b(b9_14to10_14), .out_c(matrixC9_14));
processing_element pe9_15(.reset(effective_rst), .clk(clk),  .in_a(a9_14to9_15), .in_b(b8_15to9_15),  .out_a(a9_15to9_16), .out_b(b9_15to10_15), .out_c(matrixC9_15));
processing_element pe9_16(.reset(effective_rst), .clk(clk),  .in_a(a9_15to9_16), .in_b(b8_16to9_16),  .out_a(a9_16to9_17), .out_b(b9_16to10_16), .out_c(matrixC9_16));
processing_element pe9_17(.reset(effective_rst), .clk(clk),  .in_a(a9_16to9_17), .in_b(b8_17to9_17),  .out_a(a9_17to9_18), .out_b(b9_17to10_17), .out_c(matrixC9_17));
processing_element pe9_18(.reset(effective_rst), .clk(clk),  .in_a(a9_17to9_18), .in_b(b8_18to9_18),  .out_a(a9_18to9_19), .out_b(b9_18to10_18), .out_c(matrixC9_18));
processing_element pe9_19(.reset(effective_rst), .clk(clk),  .in_a(a9_18to9_19), .in_b(b8_19to9_19),  .out_a(a9_19to9_20), .out_b(b9_19to10_19), .out_c(matrixC9_19));
processing_element pe10_1(.reset(effective_rst), .clk(clk),  .in_a(a10_0to10_1), .in_b(b9_1to10_1),  .out_a(a10_1to10_2), .out_b(b10_1to11_1), .out_c(matrixC10_1));
processing_element pe10_2(.reset(effective_rst), .clk(clk),  .in_a(a10_1to10_2), .in_b(b9_2to10_2),  .out_a(a10_2to10_3), .out_b(b10_2to11_2), .out_c(matrixC10_2));
processing_element pe10_3(.reset(effective_rst), .clk(clk),  .in_a(a10_2to10_3), .in_b(b9_3to10_3),  .out_a(a10_3to10_4), .out_b(b10_3to11_3), .out_c(matrixC10_3));
processing_element pe10_4(.reset(effective_rst), .clk(clk),  .in_a(a10_3to10_4), .in_b(b9_4to10_4),  .out_a(a10_4to10_5), .out_b(b10_4to11_4), .out_c(matrixC10_4));
processing_element pe10_5(.reset(effective_rst), .clk(clk),  .in_a(a10_4to10_5), .in_b(b9_5to10_5),  .out_a(a10_5to10_6), .out_b(b10_5to11_5), .out_c(matrixC10_5));
processing_element pe10_6(.reset(effective_rst), .clk(clk),  .in_a(a10_5to10_6), .in_b(b9_6to10_6),  .out_a(a10_6to10_7), .out_b(b10_6to11_6), .out_c(matrixC10_6));
processing_element pe10_7(.reset(effective_rst), .clk(clk),  .in_a(a10_6to10_7), .in_b(b9_7to10_7),  .out_a(a10_7to10_8), .out_b(b10_7to11_7), .out_c(matrixC10_7));
processing_element pe10_8(.reset(effective_rst), .clk(clk),  .in_a(a10_7to10_8), .in_b(b9_8to10_8),  .out_a(a10_8to10_9), .out_b(b10_8to11_8), .out_c(matrixC10_8));
processing_element pe10_9(.reset(effective_rst), .clk(clk),  .in_a(a10_8to10_9), .in_b(b9_9to10_9),  .out_a(a10_9to10_10), .out_b(b10_9to11_9), .out_c(matrixC10_9));
processing_element pe10_10(.reset(effective_rst), .clk(clk),  .in_a(a10_9to10_10), .in_b(b9_10to10_10),  .out_a(a10_10to10_11), .out_b(b10_10to11_10), .out_c(matrixC10_10));
processing_element pe10_11(.reset(effective_rst), .clk(clk),  .in_a(a10_10to10_11), .in_b(b9_11to10_11),  .out_a(a10_11to10_12), .out_b(b10_11to11_11), .out_c(matrixC10_11));
processing_element pe10_12(.reset(effective_rst), .clk(clk),  .in_a(a10_11to10_12), .in_b(b9_12to10_12),  .out_a(a10_12to10_13), .out_b(b10_12to11_12), .out_c(matrixC10_12));
processing_element pe10_13(.reset(effective_rst), .clk(clk),  .in_a(a10_12to10_13), .in_b(b9_13to10_13),  .out_a(a10_13to10_14), .out_b(b10_13to11_13), .out_c(matrixC10_13));
processing_element pe10_14(.reset(effective_rst), .clk(clk),  .in_a(a10_13to10_14), .in_b(b9_14to10_14),  .out_a(a10_14to10_15), .out_b(b10_14to11_14), .out_c(matrixC10_14));
processing_element pe10_15(.reset(effective_rst), .clk(clk),  .in_a(a10_14to10_15), .in_b(b9_15to10_15),  .out_a(a10_15to10_16), .out_b(b10_15to11_15), .out_c(matrixC10_15));
processing_element pe10_16(.reset(effective_rst), .clk(clk),  .in_a(a10_15to10_16), .in_b(b9_16to10_16),  .out_a(a10_16to10_17), .out_b(b10_16to11_16), .out_c(matrixC10_16));
processing_element pe10_17(.reset(effective_rst), .clk(clk),  .in_a(a10_16to10_17), .in_b(b9_17to10_17),  .out_a(a10_17to10_18), .out_b(b10_17to11_17), .out_c(matrixC10_17));
processing_element pe10_18(.reset(effective_rst), .clk(clk),  .in_a(a10_17to10_18), .in_b(b9_18to10_18),  .out_a(a10_18to10_19), .out_b(b10_18to11_18), .out_c(matrixC10_18));
processing_element pe10_19(.reset(effective_rst), .clk(clk),  .in_a(a10_18to10_19), .in_b(b9_19to10_19),  .out_a(a10_19to10_20), .out_b(b10_19to11_19), .out_c(matrixC10_19));
processing_element pe11_1(.reset(effective_rst), .clk(clk),  .in_a(a11_0to11_1), .in_b(b10_1to11_1),  .out_a(a11_1to11_2), .out_b(b11_1to12_1), .out_c(matrixC11_1));
processing_element pe11_2(.reset(effective_rst), .clk(clk),  .in_a(a11_1to11_2), .in_b(b10_2to11_2),  .out_a(a11_2to11_3), .out_b(b11_2to12_2), .out_c(matrixC11_2));
processing_element pe11_3(.reset(effective_rst), .clk(clk),  .in_a(a11_2to11_3), .in_b(b10_3to11_3),  .out_a(a11_3to11_4), .out_b(b11_3to12_3), .out_c(matrixC11_3));
processing_element pe11_4(.reset(effective_rst), .clk(clk),  .in_a(a11_3to11_4), .in_b(b10_4to11_4),  .out_a(a11_4to11_5), .out_b(b11_4to12_4), .out_c(matrixC11_4));
processing_element pe11_5(.reset(effective_rst), .clk(clk),  .in_a(a11_4to11_5), .in_b(b10_5to11_5),  .out_a(a11_5to11_6), .out_b(b11_5to12_5), .out_c(matrixC11_5));
processing_element pe11_6(.reset(effective_rst), .clk(clk),  .in_a(a11_5to11_6), .in_b(b10_6to11_6),  .out_a(a11_6to11_7), .out_b(b11_6to12_6), .out_c(matrixC11_6));
processing_element pe11_7(.reset(effective_rst), .clk(clk),  .in_a(a11_6to11_7), .in_b(b10_7to11_7),  .out_a(a11_7to11_8), .out_b(b11_7to12_7), .out_c(matrixC11_7));
processing_element pe11_8(.reset(effective_rst), .clk(clk),  .in_a(a11_7to11_8), .in_b(b10_8to11_8),  .out_a(a11_8to11_9), .out_b(b11_8to12_8), .out_c(matrixC11_8));
processing_element pe11_9(.reset(effective_rst), .clk(clk),  .in_a(a11_8to11_9), .in_b(b10_9to11_9),  .out_a(a11_9to11_10), .out_b(b11_9to12_9), .out_c(matrixC11_9));
processing_element pe11_10(.reset(effective_rst), .clk(clk),  .in_a(a11_9to11_10), .in_b(b10_10to11_10),  .out_a(a11_10to11_11), .out_b(b11_10to12_10), .out_c(matrixC11_10));
processing_element pe11_11(.reset(effective_rst), .clk(clk),  .in_a(a11_10to11_11), .in_b(b10_11to11_11),  .out_a(a11_11to11_12), .out_b(b11_11to12_11), .out_c(matrixC11_11));
processing_element pe11_12(.reset(effective_rst), .clk(clk),  .in_a(a11_11to11_12), .in_b(b10_12to11_12),  .out_a(a11_12to11_13), .out_b(b11_12to12_12), .out_c(matrixC11_12));
processing_element pe11_13(.reset(effective_rst), .clk(clk),  .in_a(a11_12to11_13), .in_b(b10_13to11_13),  .out_a(a11_13to11_14), .out_b(b11_13to12_13), .out_c(matrixC11_13));
processing_element pe11_14(.reset(effective_rst), .clk(clk),  .in_a(a11_13to11_14), .in_b(b10_14to11_14),  .out_a(a11_14to11_15), .out_b(b11_14to12_14), .out_c(matrixC11_14));
processing_element pe11_15(.reset(effective_rst), .clk(clk),  .in_a(a11_14to11_15), .in_b(b10_15to11_15),  .out_a(a11_15to11_16), .out_b(b11_15to12_15), .out_c(matrixC11_15));
processing_element pe11_16(.reset(effective_rst), .clk(clk),  .in_a(a11_15to11_16), .in_b(b10_16to11_16),  .out_a(a11_16to11_17), .out_b(b11_16to12_16), .out_c(matrixC11_16));
processing_element pe11_17(.reset(effective_rst), .clk(clk),  .in_a(a11_16to11_17), .in_b(b10_17to11_17),  .out_a(a11_17to11_18), .out_b(b11_17to12_17), .out_c(matrixC11_17));
processing_element pe11_18(.reset(effective_rst), .clk(clk),  .in_a(a11_17to11_18), .in_b(b10_18to11_18),  .out_a(a11_18to11_19), .out_b(b11_18to12_18), .out_c(matrixC11_18));
processing_element pe11_19(.reset(effective_rst), .clk(clk),  .in_a(a11_18to11_19), .in_b(b10_19to11_19),  .out_a(a11_19to11_20), .out_b(b11_19to12_19), .out_c(matrixC11_19));
processing_element pe12_1(.reset(effective_rst), .clk(clk),  .in_a(a12_0to12_1), .in_b(b11_1to12_1),  .out_a(a12_1to12_2), .out_b(b12_1to13_1), .out_c(matrixC12_1));
processing_element pe12_2(.reset(effective_rst), .clk(clk),  .in_a(a12_1to12_2), .in_b(b11_2to12_2),  .out_a(a12_2to12_3), .out_b(b12_2to13_2), .out_c(matrixC12_2));
processing_element pe12_3(.reset(effective_rst), .clk(clk),  .in_a(a12_2to12_3), .in_b(b11_3to12_3),  .out_a(a12_3to12_4), .out_b(b12_3to13_3), .out_c(matrixC12_3));
processing_element pe12_4(.reset(effective_rst), .clk(clk),  .in_a(a12_3to12_4), .in_b(b11_4to12_4),  .out_a(a12_4to12_5), .out_b(b12_4to13_4), .out_c(matrixC12_4));
processing_element pe12_5(.reset(effective_rst), .clk(clk),  .in_a(a12_4to12_5), .in_b(b11_5to12_5),  .out_a(a12_5to12_6), .out_b(b12_5to13_5), .out_c(matrixC12_5));
processing_element pe12_6(.reset(effective_rst), .clk(clk),  .in_a(a12_5to12_6), .in_b(b11_6to12_6),  .out_a(a12_6to12_7), .out_b(b12_6to13_6), .out_c(matrixC12_6));
processing_element pe12_7(.reset(effective_rst), .clk(clk),  .in_a(a12_6to12_7), .in_b(b11_7to12_7),  .out_a(a12_7to12_8), .out_b(b12_7to13_7), .out_c(matrixC12_7));
processing_element pe12_8(.reset(effective_rst), .clk(clk),  .in_a(a12_7to12_8), .in_b(b11_8to12_8),  .out_a(a12_8to12_9), .out_b(b12_8to13_8), .out_c(matrixC12_8));
processing_element pe12_9(.reset(effective_rst), .clk(clk),  .in_a(a12_8to12_9), .in_b(b11_9to12_9),  .out_a(a12_9to12_10), .out_b(b12_9to13_9), .out_c(matrixC12_9));
processing_element pe12_10(.reset(effective_rst), .clk(clk),  .in_a(a12_9to12_10), .in_b(b11_10to12_10),  .out_a(a12_10to12_11), .out_b(b12_10to13_10), .out_c(matrixC12_10));
processing_element pe12_11(.reset(effective_rst), .clk(clk),  .in_a(a12_10to12_11), .in_b(b11_11to12_11),  .out_a(a12_11to12_12), .out_b(b12_11to13_11), .out_c(matrixC12_11));
processing_element pe12_12(.reset(effective_rst), .clk(clk),  .in_a(a12_11to12_12), .in_b(b11_12to12_12),  .out_a(a12_12to12_13), .out_b(b12_12to13_12), .out_c(matrixC12_12));
processing_element pe12_13(.reset(effective_rst), .clk(clk),  .in_a(a12_12to12_13), .in_b(b11_13to12_13),  .out_a(a12_13to12_14), .out_b(b12_13to13_13), .out_c(matrixC12_13));
processing_element pe12_14(.reset(effective_rst), .clk(clk),  .in_a(a12_13to12_14), .in_b(b11_14to12_14),  .out_a(a12_14to12_15), .out_b(b12_14to13_14), .out_c(matrixC12_14));
processing_element pe12_15(.reset(effective_rst), .clk(clk),  .in_a(a12_14to12_15), .in_b(b11_15to12_15),  .out_a(a12_15to12_16), .out_b(b12_15to13_15), .out_c(matrixC12_15));
processing_element pe12_16(.reset(effective_rst), .clk(clk),  .in_a(a12_15to12_16), .in_b(b11_16to12_16),  .out_a(a12_16to12_17), .out_b(b12_16to13_16), .out_c(matrixC12_16));
processing_element pe12_17(.reset(effective_rst), .clk(clk),  .in_a(a12_16to12_17), .in_b(b11_17to12_17),  .out_a(a12_17to12_18), .out_b(b12_17to13_17), .out_c(matrixC12_17));
processing_element pe12_18(.reset(effective_rst), .clk(clk),  .in_a(a12_17to12_18), .in_b(b11_18to12_18),  .out_a(a12_18to12_19), .out_b(b12_18to13_18), .out_c(matrixC12_18));
processing_element pe12_19(.reset(effective_rst), .clk(clk),  .in_a(a12_18to12_19), .in_b(b11_19to12_19),  .out_a(a12_19to12_20), .out_b(b12_19to13_19), .out_c(matrixC12_19));
processing_element pe13_1(.reset(effective_rst), .clk(clk),  .in_a(a13_0to13_1), .in_b(b12_1to13_1),  .out_a(a13_1to13_2), .out_b(b13_1to14_1), .out_c(matrixC13_1));
processing_element pe13_2(.reset(effective_rst), .clk(clk),  .in_a(a13_1to13_2), .in_b(b12_2to13_2),  .out_a(a13_2to13_3), .out_b(b13_2to14_2), .out_c(matrixC13_2));
processing_element pe13_3(.reset(effective_rst), .clk(clk),  .in_a(a13_2to13_3), .in_b(b12_3to13_3),  .out_a(a13_3to13_4), .out_b(b13_3to14_3), .out_c(matrixC13_3));
processing_element pe13_4(.reset(effective_rst), .clk(clk),  .in_a(a13_3to13_4), .in_b(b12_4to13_4),  .out_a(a13_4to13_5), .out_b(b13_4to14_4), .out_c(matrixC13_4));
processing_element pe13_5(.reset(effective_rst), .clk(clk),  .in_a(a13_4to13_5), .in_b(b12_5to13_5),  .out_a(a13_5to13_6), .out_b(b13_5to14_5), .out_c(matrixC13_5));
processing_element pe13_6(.reset(effective_rst), .clk(clk),  .in_a(a13_5to13_6), .in_b(b12_6to13_6),  .out_a(a13_6to13_7), .out_b(b13_6to14_6), .out_c(matrixC13_6));
processing_element pe13_7(.reset(effective_rst), .clk(clk),  .in_a(a13_6to13_7), .in_b(b12_7to13_7),  .out_a(a13_7to13_8), .out_b(b13_7to14_7), .out_c(matrixC13_7));
processing_element pe13_8(.reset(effective_rst), .clk(clk),  .in_a(a13_7to13_8), .in_b(b12_8to13_8),  .out_a(a13_8to13_9), .out_b(b13_8to14_8), .out_c(matrixC13_8));
processing_element pe13_9(.reset(effective_rst), .clk(clk),  .in_a(a13_8to13_9), .in_b(b12_9to13_9),  .out_a(a13_9to13_10), .out_b(b13_9to14_9), .out_c(matrixC13_9));
processing_element pe13_10(.reset(effective_rst), .clk(clk),  .in_a(a13_9to13_10), .in_b(b12_10to13_10),  .out_a(a13_10to13_11), .out_b(b13_10to14_10), .out_c(matrixC13_10));
processing_element pe13_11(.reset(effective_rst), .clk(clk),  .in_a(a13_10to13_11), .in_b(b12_11to13_11),  .out_a(a13_11to13_12), .out_b(b13_11to14_11), .out_c(matrixC13_11));
processing_element pe13_12(.reset(effective_rst), .clk(clk),  .in_a(a13_11to13_12), .in_b(b12_12to13_12),  .out_a(a13_12to13_13), .out_b(b13_12to14_12), .out_c(matrixC13_12));
processing_element pe13_13(.reset(effective_rst), .clk(clk),  .in_a(a13_12to13_13), .in_b(b12_13to13_13),  .out_a(a13_13to13_14), .out_b(b13_13to14_13), .out_c(matrixC13_13));
processing_element pe13_14(.reset(effective_rst), .clk(clk),  .in_a(a13_13to13_14), .in_b(b12_14to13_14),  .out_a(a13_14to13_15), .out_b(b13_14to14_14), .out_c(matrixC13_14));
processing_element pe13_15(.reset(effective_rst), .clk(clk),  .in_a(a13_14to13_15), .in_b(b12_15to13_15),  .out_a(a13_15to13_16), .out_b(b13_15to14_15), .out_c(matrixC13_15));
processing_element pe13_16(.reset(effective_rst), .clk(clk),  .in_a(a13_15to13_16), .in_b(b12_16to13_16),  .out_a(a13_16to13_17), .out_b(b13_16to14_16), .out_c(matrixC13_16));
processing_element pe13_17(.reset(effective_rst), .clk(clk),  .in_a(a13_16to13_17), .in_b(b12_17to13_17),  .out_a(a13_17to13_18), .out_b(b13_17to14_17), .out_c(matrixC13_17));
processing_element pe13_18(.reset(effective_rst), .clk(clk),  .in_a(a13_17to13_18), .in_b(b12_18to13_18),  .out_a(a13_18to13_19), .out_b(b13_18to14_18), .out_c(matrixC13_18));
processing_element pe13_19(.reset(effective_rst), .clk(clk),  .in_a(a13_18to13_19), .in_b(b12_19to13_19),  .out_a(a13_19to13_20), .out_b(b13_19to14_19), .out_c(matrixC13_19));
processing_element pe14_1(.reset(effective_rst), .clk(clk),  .in_a(a14_0to14_1), .in_b(b13_1to14_1),  .out_a(a14_1to14_2), .out_b(b14_1to15_1), .out_c(matrixC14_1));
processing_element pe14_2(.reset(effective_rst), .clk(clk),  .in_a(a14_1to14_2), .in_b(b13_2to14_2),  .out_a(a14_2to14_3), .out_b(b14_2to15_2), .out_c(matrixC14_2));
processing_element pe14_3(.reset(effective_rst), .clk(clk),  .in_a(a14_2to14_3), .in_b(b13_3to14_3),  .out_a(a14_3to14_4), .out_b(b14_3to15_3), .out_c(matrixC14_3));
processing_element pe14_4(.reset(effective_rst), .clk(clk),  .in_a(a14_3to14_4), .in_b(b13_4to14_4),  .out_a(a14_4to14_5), .out_b(b14_4to15_4), .out_c(matrixC14_4));
processing_element pe14_5(.reset(effective_rst), .clk(clk),  .in_a(a14_4to14_5), .in_b(b13_5to14_5),  .out_a(a14_5to14_6), .out_b(b14_5to15_5), .out_c(matrixC14_5));
processing_element pe14_6(.reset(effective_rst), .clk(clk),  .in_a(a14_5to14_6), .in_b(b13_6to14_6),  .out_a(a14_6to14_7), .out_b(b14_6to15_6), .out_c(matrixC14_6));
processing_element pe14_7(.reset(effective_rst), .clk(clk),  .in_a(a14_6to14_7), .in_b(b13_7to14_7),  .out_a(a14_7to14_8), .out_b(b14_7to15_7), .out_c(matrixC14_7));
processing_element pe14_8(.reset(effective_rst), .clk(clk),  .in_a(a14_7to14_8), .in_b(b13_8to14_8),  .out_a(a14_8to14_9), .out_b(b14_8to15_8), .out_c(matrixC14_8));
processing_element pe14_9(.reset(effective_rst), .clk(clk),  .in_a(a14_8to14_9), .in_b(b13_9to14_9),  .out_a(a14_9to14_10), .out_b(b14_9to15_9), .out_c(matrixC14_9));
processing_element pe14_10(.reset(effective_rst), .clk(clk),  .in_a(a14_9to14_10), .in_b(b13_10to14_10),  .out_a(a14_10to14_11), .out_b(b14_10to15_10), .out_c(matrixC14_10));
processing_element pe14_11(.reset(effective_rst), .clk(clk),  .in_a(a14_10to14_11), .in_b(b13_11to14_11),  .out_a(a14_11to14_12), .out_b(b14_11to15_11), .out_c(matrixC14_11));
processing_element pe14_12(.reset(effective_rst), .clk(clk),  .in_a(a14_11to14_12), .in_b(b13_12to14_12),  .out_a(a14_12to14_13), .out_b(b14_12to15_12), .out_c(matrixC14_12));
processing_element pe14_13(.reset(effective_rst), .clk(clk),  .in_a(a14_12to14_13), .in_b(b13_13to14_13),  .out_a(a14_13to14_14), .out_b(b14_13to15_13), .out_c(matrixC14_13));
processing_element pe14_14(.reset(effective_rst), .clk(clk),  .in_a(a14_13to14_14), .in_b(b13_14to14_14),  .out_a(a14_14to14_15), .out_b(b14_14to15_14), .out_c(matrixC14_14));
processing_element pe14_15(.reset(effective_rst), .clk(clk),  .in_a(a14_14to14_15), .in_b(b13_15to14_15),  .out_a(a14_15to14_16), .out_b(b14_15to15_15), .out_c(matrixC14_15));
processing_element pe14_16(.reset(effective_rst), .clk(clk),  .in_a(a14_15to14_16), .in_b(b13_16to14_16),  .out_a(a14_16to14_17), .out_b(b14_16to15_16), .out_c(matrixC14_16));
processing_element pe14_17(.reset(effective_rst), .clk(clk),  .in_a(a14_16to14_17), .in_b(b13_17to14_17),  .out_a(a14_17to14_18), .out_b(b14_17to15_17), .out_c(matrixC14_17));
processing_element pe14_18(.reset(effective_rst), .clk(clk),  .in_a(a14_17to14_18), .in_b(b13_18to14_18),  .out_a(a14_18to14_19), .out_b(b14_18to15_18), .out_c(matrixC14_18));
processing_element pe14_19(.reset(effective_rst), .clk(clk),  .in_a(a14_18to14_19), .in_b(b13_19to14_19),  .out_a(a14_19to14_20), .out_b(b14_19to15_19), .out_c(matrixC14_19));
processing_element pe15_1(.reset(effective_rst), .clk(clk),  .in_a(a15_0to15_1), .in_b(b14_1to15_1),  .out_a(a15_1to15_2), .out_b(b15_1to16_1), .out_c(matrixC15_1));
processing_element pe15_2(.reset(effective_rst), .clk(clk),  .in_a(a15_1to15_2), .in_b(b14_2to15_2),  .out_a(a15_2to15_3), .out_b(b15_2to16_2), .out_c(matrixC15_2));
processing_element pe15_3(.reset(effective_rst), .clk(clk),  .in_a(a15_2to15_3), .in_b(b14_3to15_3),  .out_a(a15_3to15_4), .out_b(b15_3to16_3), .out_c(matrixC15_3));
processing_element pe15_4(.reset(effective_rst), .clk(clk),  .in_a(a15_3to15_4), .in_b(b14_4to15_4),  .out_a(a15_4to15_5), .out_b(b15_4to16_4), .out_c(matrixC15_4));
processing_element pe15_5(.reset(effective_rst), .clk(clk),  .in_a(a15_4to15_5), .in_b(b14_5to15_5),  .out_a(a15_5to15_6), .out_b(b15_5to16_5), .out_c(matrixC15_5));
processing_element pe15_6(.reset(effective_rst), .clk(clk),  .in_a(a15_5to15_6), .in_b(b14_6to15_6),  .out_a(a15_6to15_7), .out_b(b15_6to16_6), .out_c(matrixC15_6));
processing_element pe15_7(.reset(effective_rst), .clk(clk),  .in_a(a15_6to15_7), .in_b(b14_7to15_7),  .out_a(a15_7to15_8), .out_b(b15_7to16_7), .out_c(matrixC15_7));
processing_element pe15_8(.reset(effective_rst), .clk(clk),  .in_a(a15_7to15_8), .in_b(b14_8to15_8),  .out_a(a15_8to15_9), .out_b(b15_8to16_8), .out_c(matrixC15_8));
processing_element pe15_9(.reset(effective_rst), .clk(clk),  .in_a(a15_8to15_9), .in_b(b14_9to15_9),  .out_a(a15_9to15_10), .out_b(b15_9to16_9), .out_c(matrixC15_9));
processing_element pe15_10(.reset(effective_rst), .clk(clk),  .in_a(a15_9to15_10), .in_b(b14_10to15_10),  .out_a(a15_10to15_11), .out_b(b15_10to16_10), .out_c(matrixC15_10));
processing_element pe15_11(.reset(effective_rst), .clk(clk),  .in_a(a15_10to15_11), .in_b(b14_11to15_11),  .out_a(a15_11to15_12), .out_b(b15_11to16_11), .out_c(matrixC15_11));
processing_element pe15_12(.reset(effective_rst), .clk(clk),  .in_a(a15_11to15_12), .in_b(b14_12to15_12),  .out_a(a15_12to15_13), .out_b(b15_12to16_12), .out_c(matrixC15_12));
processing_element pe15_13(.reset(effective_rst), .clk(clk),  .in_a(a15_12to15_13), .in_b(b14_13to15_13),  .out_a(a15_13to15_14), .out_b(b15_13to16_13), .out_c(matrixC15_13));
processing_element pe15_14(.reset(effective_rst), .clk(clk),  .in_a(a15_13to15_14), .in_b(b14_14to15_14),  .out_a(a15_14to15_15), .out_b(b15_14to16_14), .out_c(matrixC15_14));
processing_element pe15_15(.reset(effective_rst), .clk(clk),  .in_a(a15_14to15_15), .in_b(b14_15to15_15),  .out_a(a15_15to15_16), .out_b(b15_15to16_15), .out_c(matrixC15_15));
processing_element pe15_16(.reset(effective_rst), .clk(clk),  .in_a(a15_15to15_16), .in_b(b14_16to15_16),  .out_a(a15_16to15_17), .out_b(b15_16to16_16), .out_c(matrixC15_16));
processing_element pe15_17(.reset(effective_rst), .clk(clk),  .in_a(a15_16to15_17), .in_b(b14_17to15_17),  .out_a(a15_17to15_18), .out_b(b15_17to16_17), .out_c(matrixC15_17));
processing_element pe15_18(.reset(effective_rst), .clk(clk),  .in_a(a15_17to15_18), .in_b(b14_18to15_18),  .out_a(a15_18to15_19), .out_b(b15_18to16_18), .out_c(matrixC15_18));
processing_element pe15_19(.reset(effective_rst), .clk(clk),  .in_a(a15_18to15_19), .in_b(b14_19to15_19),  .out_a(a15_19to15_20), .out_b(b15_19to16_19), .out_c(matrixC15_19));
processing_element pe16_1(.reset(effective_rst), .clk(clk),  .in_a(a16_0to16_1), .in_b(b15_1to16_1),  .out_a(a16_1to16_2), .out_b(b16_1to17_1), .out_c(matrixC16_1));
processing_element pe16_2(.reset(effective_rst), .clk(clk),  .in_a(a16_1to16_2), .in_b(b15_2to16_2),  .out_a(a16_2to16_3), .out_b(b16_2to17_2), .out_c(matrixC16_2));
processing_element pe16_3(.reset(effective_rst), .clk(clk),  .in_a(a16_2to16_3), .in_b(b15_3to16_3),  .out_a(a16_3to16_4), .out_b(b16_3to17_3), .out_c(matrixC16_3));
processing_element pe16_4(.reset(effective_rst), .clk(clk),  .in_a(a16_3to16_4), .in_b(b15_4to16_4),  .out_a(a16_4to16_5), .out_b(b16_4to17_4), .out_c(matrixC16_4));
processing_element pe16_5(.reset(effective_rst), .clk(clk),  .in_a(a16_4to16_5), .in_b(b15_5to16_5),  .out_a(a16_5to16_6), .out_b(b16_5to17_5), .out_c(matrixC16_5));
processing_element pe16_6(.reset(effective_rst), .clk(clk),  .in_a(a16_5to16_6), .in_b(b15_6to16_6),  .out_a(a16_6to16_7), .out_b(b16_6to17_6), .out_c(matrixC16_6));
processing_element pe16_7(.reset(effective_rst), .clk(clk),  .in_a(a16_6to16_7), .in_b(b15_7to16_7),  .out_a(a16_7to16_8), .out_b(b16_7to17_7), .out_c(matrixC16_7));
processing_element pe16_8(.reset(effective_rst), .clk(clk),  .in_a(a16_7to16_8), .in_b(b15_8to16_8),  .out_a(a16_8to16_9), .out_b(b16_8to17_8), .out_c(matrixC16_8));
processing_element pe16_9(.reset(effective_rst), .clk(clk),  .in_a(a16_8to16_9), .in_b(b15_9to16_9),  .out_a(a16_9to16_10), .out_b(b16_9to17_9), .out_c(matrixC16_9));
processing_element pe16_10(.reset(effective_rst), .clk(clk),  .in_a(a16_9to16_10), .in_b(b15_10to16_10),  .out_a(a16_10to16_11), .out_b(b16_10to17_10), .out_c(matrixC16_10));
processing_element pe16_11(.reset(effective_rst), .clk(clk),  .in_a(a16_10to16_11), .in_b(b15_11to16_11),  .out_a(a16_11to16_12), .out_b(b16_11to17_11), .out_c(matrixC16_11));
processing_element pe16_12(.reset(effective_rst), .clk(clk),  .in_a(a16_11to16_12), .in_b(b15_12to16_12),  .out_a(a16_12to16_13), .out_b(b16_12to17_12), .out_c(matrixC16_12));
processing_element pe16_13(.reset(effective_rst), .clk(clk),  .in_a(a16_12to16_13), .in_b(b15_13to16_13),  .out_a(a16_13to16_14), .out_b(b16_13to17_13), .out_c(matrixC16_13));
processing_element pe16_14(.reset(effective_rst), .clk(clk),  .in_a(a16_13to16_14), .in_b(b15_14to16_14),  .out_a(a16_14to16_15), .out_b(b16_14to17_14), .out_c(matrixC16_14));
processing_element pe16_15(.reset(effective_rst), .clk(clk),  .in_a(a16_14to16_15), .in_b(b15_15to16_15),  .out_a(a16_15to16_16), .out_b(b16_15to17_15), .out_c(matrixC16_15));
processing_element pe16_16(.reset(effective_rst), .clk(clk),  .in_a(a16_15to16_16), .in_b(b15_16to16_16),  .out_a(a16_16to16_17), .out_b(b16_16to17_16), .out_c(matrixC16_16));
processing_element pe16_17(.reset(effective_rst), .clk(clk),  .in_a(a16_16to16_17), .in_b(b15_17to16_17),  .out_a(a16_17to16_18), .out_b(b16_17to17_17), .out_c(matrixC16_17));
processing_element pe16_18(.reset(effective_rst), .clk(clk),  .in_a(a16_17to16_18), .in_b(b15_18to16_18),  .out_a(a16_18to16_19), .out_b(b16_18to17_18), .out_c(matrixC16_18));
processing_element pe16_19(.reset(effective_rst), .clk(clk),  .in_a(a16_18to16_19), .in_b(b15_19to16_19),  .out_a(a16_19to16_20), .out_b(b16_19to17_19), .out_c(matrixC16_19));
processing_element pe17_1(.reset(effective_rst), .clk(clk),  .in_a(a17_0to17_1), .in_b(b16_1to17_1),  .out_a(a17_1to17_2), .out_b(b17_1to18_1), .out_c(matrixC17_1));
processing_element pe17_2(.reset(effective_rst), .clk(clk),  .in_a(a17_1to17_2), .in_b(b16_2to17_2),  .out_a(a17_2to17_3), .out_b(b17_2to18_2), .out_c(matrixC17_2));
processing_element pe17_3(.reset(effective_rst), .clk(clk),  .in_a(a17_2to17_3), .in_b(b16_3to17_3),  .out_a(a17_3to17_4), .out_b(b17_3to18_3), .out_c(matrixC17_3));
processing_element pe17_4(.reset(effective_rst), .clk(clk),  .in_a(a17_3to17_4), .in_b(b16_4to17_4),  .out_a(a17_4to17_5), .out_b(b17_4to18_4), .out_c(matrixC17_4));
processing_element pe17_5(.reset(effective_rst), .clk(clk),  .in_a(a17_4to17_5), .in_b(b16_5to17_5),  .out_a(a17_5to17_6), .out_b(b17_5to18_5), .out_c(matrixC17_5));
processing_element pe17_6(.reset(effective_rst), .clk(clk),  .in_a(a17_5to17_6), .in_b(b16_6to17_6),  .out_a(a17_6to17_7), .out_b(b17_6to18_6), .out_c(matrixC17_6));
processing_element pe17_7(.reset(effective_rst), .clk(clk),  .in_a(a17_6to17_7), .in_b(b16_7to17_7),  .out_a(a17_7to17_8), .out_b(b17_7to18_7), .out_c(matrixC17_7));
processing_element pe17_8(.reset(effective_rst), .clk(clk),  .in_a(a17_7to17_8), .in_b(b16_8to17_8),  .out_a(a17_8to17_9), .out_b(b17_8to18_8), .out_c(matrixC17_8));
processing_element pe17_9(.reset(effective_rst), .clk(clk),  .in_a(a17_8to17_9), .in_b(b16_9to17_9),  .out_a(a17_9to17_10), .out_b(b17_9to18_9), .out_c(matrixC17_9));
processing_element pe17_10(.reset(effective_rst), .clk(clk),  .in_a(a17_9to17_10), .in_b(b16_10to17_10),  .out_a(a17_10to17_11), .out_b(b17_10to18_10), .out_c(matrixC17_10));
processing_element pe17_11(.reset(effective_rst), .clk(clk),  .in_a(a17_10to17_11), .in_b(b16_11to17_11),  .out_a(a17_11to17_12), .out_b(b17_11to18_11), .out_c(matrixC17_11));
processing_element pe17_12(.reset(effective_rst), .clk(clk),  .in_a(a17_11to17_12), .in_b(b16_12to17_12),  .out_a(a17_12to17_13), .out_b(b17_12to18_12), .out_c(matrixC17_12));
processing_element pe17_13(.reset(effective_rst), .clk(clk),  .in_a(a17_12to17_13), .in_b(b16_13to17_13),  .out_a(a17_13to17_14), .out_b(b17_13to18_13), .out_c(matrixC17_13));
processing_element pe17_14(.reset(effective_rst), .clk(clk),  .in_a(a17_13to17_14), .in_b(b16_14to17_14),  .out_a(a17_14to17_15), .out_b(b17_14to18_14), .out_c(matrixC17_14));
processing_element pe17_15(.reset(effective_rst), .clk(clk),  .in_a(a17_14to17_15), .in_b(b16_15to17_15),  .out_a(a17_15to17_16), .out_b(b17_15to18_15), .out_c(matrixC17_15));
processing_element pe17_16(.reset(effective_rst), .clk(clk),  .in_a(a17_15to17_16), .in_b(b16_16to17_16),  .out_a(a17_16to17_17), .out_b(b17_16to18_16), .out_c(matrixC17_16));
processing_element pe17_17(.reset(effective_rst), .clk(clk),  .in_a(a17_16to17_17), .in_b(b16_17to17_17),  .out_a(a17_17to17_18), .out_b(b17_17to18_17), .out_c(matrixC17_17));
processing_element pe17_18(.reset(effective_rst), .clk(clk),  .in_a(a17_17to17_18), .in_b(b16_18to17_18),  .out_a(a17_18to17_19), .out_b(b17_18to18_18), .out_c(matrixC17_18));
processing_element pe17_19(.reset(effective_rst), .clk(clk),  .in_a(a17_18to17_19), .in_b(b16_19to17_19),  .out_a(a17_19to17_20), .out_b(b17_19to18_19), .out_c(matrixC17_19));
processing_element pe18_1(.reset(effective_rst), .clk(clk),  .in_a(a18_0to18_1), .in_b(b17_1to18_1),  .out_a(a18_1to18_2), .out_b(b18_1to19_1), .out_c(matrixC18_1));
processing_element pe18_2(.reset(effective_rst), .clk(clk),  .in_a(a18_1to18_2), .in_b(b17_2to18_2),  .out_a(a18_2to18_3), .out_b(b18_2to19_2), .out_c(matrixC18_2));
processing_element pe18_3(.reset(effective_rst), .clk(clk),  .in_a(a18_2to18_3), .in_b(b17_3to18_3),  .out_a(a18_3to18_4), .out_b(b18_3to19_3), .out_c(matrixC18_3));
processing_element pe18_4(.reset(effective_rst), .clk(clk),  .in_a(a18_3to18_4), .in_b(b17_4to18_4),  .out_a(a18_4to18_5), .out_b(b18_4to19_4), .out_c(matrixC18_4));
processing_element pe18_5(.reset(effective_rst), .clk(clk),  .in_a(a18_4to18_5), .in_b(b17_5to18_5),  .out_a(a18_5to18_6), .out_b(b18_5to19_5), .out_c(matrixC18_5));
processing_element pe18_6(.reset(effective_rst), .clk(clk),  .in_a(a18_5to18_6), .in_b(b17_6to18_6),  .out_a(a18_6to18_7), .out_b(b18_6to19_6), .out_c(matrixC18_6));
processing_element pe18_7(.reset(effective_rst), .clk(clk),  .in_a(a18_6to18_7), .in_b(b17_7to18_7),  .out_a(a18_7to18_8), .out_b(b18_7to19_7), .out_c(matrixC18_7));
processing_element pe18_8(.reset(effective_rst), .clk(clk),  .in_a(a18_7to18_8), .in_b(b17_8to18_8),  .out_a(a18_8to18_9), .out_b(b18_8to19_8), .out_c(matrixC18_8));
processing_element pe18_9(.reset(effective_rst), .clk(clk),  .in_a(a18_8to18_9), .in_b(b17_9to18_9),  .out_a(a18_9to18_10), .out_b(b18_9to19_9), .out_c(matrixC18_9));
processing_element pe18_10(.reset(effective_rst), .clk(clk),  .in_a(a18_9to18_10), .in_b(b17_10to18_10),  .out_a(a18_10to18_11), .out_b(b18_10to19_10), .out_c(matrixC18_10));
processing_element pe18_11(.reset(effective_rst), .clk(clk),  .in_a(a18_10to18_11), .in_b(b17_11to18_11),  .out_a(a18_11to18_12), .out_b(b18_11to19_11), .out_c(matrixC18_11));
processing_element pe18_12(.reset(effective_rst), .clk(clk),  .in_a(a18_11to18_12), .in_b(b17_12to18_12),  .out_a(a18_12to18_13), .out_b(b18_12to19_12), .out_c(matrixC18_12));
processing_element pe18_13(.reset(effective_rst), .clk(clk),  .in_a(a18_12to18_13), .in_b(b17_13to18_13),  .out_a(a18_13to18_14), .out_b(b18_13to19_13), .out_c(matrixC18_13));
processing_element pe18_14(.reset(effective_rst), .clk(clk),  .in_a(a18_13to18_14), .in_b(b17_14to18_14),  .out_a(a18_14to18_15), .out_b(b18_14to19_14), .out_c(matrixC18_14));
processing_element pe18_15(.reset(effective_rst), .clk(clk),  .in_a(a18_14to18_15), .in_b(b17_15to18_15),  .out_a(a18_15to18_16), .out_b(b18_15to19_15), .out_c(matrixC18_15));
processing_element pe18_16(.reset(effective_rst), .clk(clk),  .in_a(a18_15to18_16), .in_b(b17_16to18_16),  .out_a(a18_16to18_17), .out_b(b18_16to19_16), .out_c(matrixC18_16));
processing_element pe18_17(.reset(effective_rst), .clk(clk),  .in_a(a18_16to18_17), .in_b(b17_17to18_17),  .out_a(a18_17to18_18), .out_b(b18_17to19_17), .out_c(matrixC18_17));
processing_element pe18_18(.reset(effective_rst), .clk(clk),  .in_a(a18_17to18_18), .in_b(b17_18to18_18),  .out_a(a18_18to18_19), .out_b(b18_18to19_18), .out_c(matrixC18_18));
processing_element pe18_19(.reset(effective_rst), .clk(clk),  .in_a(a18_18to18_19), .in_b(b17_19to18_19),  .out_a(a18_19to18_20), .out_b(b18_19to19_19), .out_c(matrixC18_19));
processing_element pe19_1(.reset(effective_rst), .clk(clk),  .in_a(a19_0to19_1), .in_b(b18_1to19_1),  .out_a(a19_1to19_2), .out_b(b19_1to20_1), .out_c(matrixC19_1));
processing_element pe19_2(.reset(effective_rst), .clk(clk),  .in_a(a19_1to19_2), .in_b(b18_2to19_2),  .out_a(a19_2to19_3), .out_b(b19_2to20_2), .out_c(matrixC19_2));
processing_element pe19_3(.reset(effective_rst), .clk(clk),  .in_a(a19_2to19_3), .in_b(b18_3to19_3),  .out_a(a19_3to19_4), .out_b(b19_3to20_3), .out_c(matrixC19_3));
processing_element pe19_4(.reset(effective_rst), .clk(clk),  .in_a(a19_3to19_4), .in_b(b18_4to19_4),  .out_a(a19_4to19_5), .out_b(b19_4to20_4), .out_c(matrixC19_4));
processing_element pe19_5(.reset(effective_rst), .clk(clk),  .in_a(a19_4to19_5), .in_b(b18_5to19_5),  .out_a(a19_5to19_6), .out_b(b19_5to20_5), .out_c(matrixC19_5));
processing_element pe19_6(.reset(effective_rst), .clk(clk),  .in_a(a19_5to19_6), .in_b(b18_6to19_6),  .out_a(a19_6to19_7), .out_b(b19_6to20_6), .out_c(matrixC19_6));
processing_element pe19_7(.reset(effective_rst), .clk(clk),  .in_a(a19_6to19_7), .in_b(b18_7to19_7),  .out_a(a19_7to19_8), .out_b(b19_7to20_7), .out_c(matrixC19_7));
processing_element pe19_8(.reset(effective_rst), .clk(clk),  .in_a(a19_7to19_8), .in_b(b18_8to19_8),  .out_a(a19_8to19_9), .out_b(b19_8to20_8), .out_c(matrixC19_8));
processing_element pe19_9(.reset(effective_rst), .clk(clk),  .in_a(a19_8to19_9), .in_b(b18_9to19_9),  .out_a(a19_9to19_10), .out_b(b19_9to20_9), .out_c(matrixC19_9));
processing_element pe19_10(.reset(effective_rst), .clk(clk),  .in_a(a19_9to19_10), .in_b(b18_10to19_10),  .out_a(a19_10to19_11), .out_b(b19_10to20_10), .out_c(matrixC19_10));
processing_element pe19_11(.reset(effective_rst), .clk(clk),  .in_a(a19_10to19_11), .in_b(b18_11to19_11),  .out_a(a19_11to19_12), .out_b(b19_11to20_11), .out_c(matrixC19_11));
processing_element pe19_12(.reset(effective_rst), .clk(clk),  .in_a(a19_11to19_12), .in_b(b18_12to19_12),  .out_a(a19_12to19_13), .out_b(b19_12to20_12), .out_c(matrixC19_12));
processing_element pe19_13(.reset(effective_rst), .clk(clk),  .in_a(a19_12to19_13), .in_b(b18_13to19_13),  .out_a(a19_13to19_14), .out_b(b19_13to20_13), .out_c(matrixC19_13));
processing_element pe19_14(.reset(effective_rst), .clk(clk),  .in_a(a19_13to19_14), .in_b(b18_14to19_14),  .out_a(a19_14to19_15), .out_b(b19_14to20_14), .out_c(matrixC19_14));
processing_element pe19_15(.reset(effective_rst), .clk(clk),  .in_a(a19_14to19_15), .in_b(b18_15to19_15),  .out_a(a19_15to19_16), .out_b(b19_15to20_15), .out_c(matrixC19_15));
processing_element pe19_16(.reset(effective_rst), .clk(clk),  .in_a(a19_15to19_16), .in_b(b18_16to19_16),  .out_a(a19_16to19_17), .out_b(b19_16to20_16), .out_c(matrixC19_16));
processing_element pe19_17(.reset(effective_rst), .clk(clk),  .in_a(a19_16to19_17), .in_b(b18_17to19_17),  .out_a(a19_17to19_18), .out_b(b19_17to20_17), .out_c(matrixC19_17));
processing_element pe19_18(.reset(effective_rst), .clk(clk),  .in_a(a19_17to19_18), .in_b(b18_18to19_18),  .out_a(a19_18to19_19), .out_b(b19_18to20_18), .out_c(matrixC19_18));
processing_element pe19_19(.reset(effective_rst), .clk(clk),  .in_a(a19_18to19_19), .in_b(b18_19to19_19),  .out_a(a19_19to19_20), .out_b(b19_19to20_19), .out_c(matrixC19_19));
assign a_data_out = {a19_19to19_20,a18_19to18_20,a17_19to17_20,a16_19to16_20,a15_19to15_20,a14_19to14_20,a13_19to13_20,a12_19to12_20,a11_19to11_20,a10_19to10_20,a9_19to9_20,a8_19to8_20,a7_19to7_20,a6_19to6_20,a5_19to5_20,a4_19to4_20,a3_19to3_20,a2_19to2_20,a1_19to1_20,a0_19to0_20};
assign b_data_out = {b19_19to20_19,b19_18to20_18,b19_17to20_17,b19_16to20_16,b19_15to20_15,b19_14to20_14,b19_13to20_13,b19_12to20_12,b19_11to20_11,b19_10to20_10,b19_9to20_9,b19_8to20_8,b19_7to20_7,b19_6to20_6,b19_5to20_5,b19_4to20_4,b19_3to20_3,b19_2to20_2,b19_1to20_1,b19_0to20_0};

endmodule

//////////////////////////////////////////////////////////////////////////
// Definition of a processing element (basically a MAC)
//////////////////////////////////////////////////////////////////////////
module processing_element(
 reset, 
 clk, 
 in_a,
 in_b, 
 out_a, 
 out_b, 
 out_c
 );

 input reset;
 input clk;
 input  [`DWIDTH-1:0] in_a;
 input  [`DWIDTH-1:0] in_b;
 output [`DWIDTH-1:0] out_a;
 output [`DWIDTH-1:0] out_b;
 output [`DWIDTH-1:0] out_c;  //reduced precision

 reg [`DWIDTH-1:0] out_a;
 reg [`DWIDTH-1:0] out_b;
 wire [`DWIDTH-1:0] out_c;

 wire [`DWIDTH-1:0] out_mac;

 assign out_c = out_mac;

 `ifdef complex_dsp
 mac_fp u_mac(.a(in_a), .b(in_b), .out(out_mac), .reset(reset), .clk(clk));
 `else
 seq_mac u_mac(.a(in_a), .b(in_b), .out(out_mac), .reset(reset), .clk(clk));
 `endif

 always @(posedge clk)begin
    if(reset) begin
      out_a<=0;
      out_b<=0;
    end
    else begin  
      out_a<=in_a;
      out_b<=in_b;
    end
 end
 
endmodule

//////////////////////////////////////////////////////////////////////////
// Multiply-and-accumulate (MAC) block
//////////////////////////////////////////////////////////////////////////
//module seq_mac(a, b, out, reset, clk);
//input [`DWIDTH-1:0] a;
//input [`DWIDTH-1:0] b;
//input reset;
//input clk;
//output [`DWIDTH-1:0] out;
//
//reg [2*`DWIDTH-1:0] out_temp;
//wire [`DWIDTH-1:0] mul_out;
//wire [2*`DWIDTH-1:0] add_out;
//
//reg [`DWIDTH-1:0] a_flopped;
//reg [`DWIDTH-1:0] b_flopped;
//
//wire [2*`DWIDTH-1:0] mul_out_temp;
//reg [2*`DWIDTH-1:0] mul_out_temp_reg;
//
//always @(posedge clk) begin
//  if (reset) begin
//    a_flopped <= 0;
//    b_flopped <= 0;
//  end else begin
//    a_flopped <= a;
//    b_flopped <= b;
//  end
//end
//
////assign mul_out = a * b;
//qmult mult_u1(.i_multiplicand(a_flopped), .i_multiplier(b_flopped), .o_result(mul_out_temp));
//
//always @(posedge clk) begin
//  if (reset) begin
//    mul_out_temp_reg <= 0;
//  end else begin
//    mul_out_temp_reg <= mul_out_temp;
//  end
//end
//
////we just truncate the higher bits of the product
////assign add_out = mul_out + out;
//qadd add_u1(.a(out_temp), .b(mul_out_temp_reg), .c(add_out));
//
//always @(posedge clk) begin
//  if (reset) begin
//    out_temp <= 0;
//  end else begin
//    out_temp <= add_out;
//  end
//end
//
////down cast the result
//assign out = 
//    (out_temp[2*`DWIDTH-1] == 0) ?  //positive number
//        (
//           (|(out_temp[2*`DWIDTH-2 : `DWIDTH-1])) ?  //is any bit from 14:7 is 1, that means overlfow
//             {out_temp[2*`DWIDTH-1] , {(`DWIDTH-1){1'b1}}} : //sign bit and then all 1s
//             {out_temp[2*`DWIDTH-1] , out_temp[`DWIDTH-2:0]} 
//        )
//        : //negative number
//        (
//           (|(out_temp[2*`DWIDTH-2 : `DWIDTH-1])) ?  //is any bit from 14:7 is 0, that means overlfow
//             {out_temp[2*`DWIDTH-1] , out_temp[`DWIDTH-2:0]} :
//             {out_temp[2*`DWIDTH-1] , {(`DWIDTH-1){1'b0}}} //sign bit and then all 0s
//        );
//
//endmodule
//
//module qmult(i_multiplicand,i_multiplier,o_result);
//input [`DWIDTH-1:0] i_multiplicand;
//input [`DWIDTH-1:0] i_multiplier;
//output [2*`DWIDTH-1:0] o_result;
//
//assign o_result = i_multiplicand * i_multiplier;
////DW02_mult #(`DWIDTH,`DWIDTH) u_mult(.A(i_multiplicand), .B(i_multiplier), .TC(1'b1), .PRODUCT(o_result));
//
//endmodule
//
//module qadd(a,b,c);
//input [2*`DWIDTH-1:0] a;
//input [2*`DWIDTH-1:0] b;
//output [2*`DWIDTH-1:0] c;
//
//assign c = a + b;
////DW01_add #(`DWIDTH) u_add(.A(a), .B(b), .CI(1'b0), .SUM(c), .CO());
//endmodule

`ifndef complex_dsp

//////////////////////////////////////////////////////////////////////////
// Multiply-and-accumulate (MAC) block
//////////////////////////////////////////////////////////////////////////
module seq_mac(a, b, out, reset, clk);
input [`DWIDTH-1:0] a;
input [`DWIDTH-1:0] b;
input reset;
input clk;
output [`DWIDTH-1:0] out;

reg [2*`DWIDTH-1:0] out_temp;
wire [2*`DWIDTH-1:0] mul_out;
wire [2*`DWIDTH-1:0] add_out;

reg [`DWIDTH-1:0] a_flopped;
reg [`DWIDTH-1:0] b_flopped;

wire [2*`DWIDTH-1:0] mul_out_temp;
reg [2*`DWIDTH-1:0] mul_out_temp_reg;

always @(posedge clk) begin
  if (reset) begin
    a_flopped <= 0;
    b_flopped <= 0;
  end else begin
    a_flopped <= a;
    b_flopped <= b;
  end
end

//assign mul_out = a * b;
qmult mult_u1(.clk(clk), .rst(reset), .i_multiplicand(a_flopped), .i_multiplier(b_flopped), .o_result(mul_out_temp));

always @(posedge clk) begin
  if (reset) begin
    mul_out_temp_reg <= 0;
  end else begin
    mul_out_temp_reg <= mul_out_temp;
  end
end

assign mul_out = mul_out_temp_reg;

qadd add_u1(.clk(clk), .rst(reset), .a(out_temp), .b(mul_out), .c(add_out));

always @(posedge clk) begin
  if (reset) begin
    out_temp <= 0;
  end else begin
    out_temp <= add_out;
  end
end

//fp32 to fp16 conversion
wire [15:0] fpadd_16_result;
fp32_to_fp16 u_32to16 (.a(out_temp), .b(out));

endmodule


//////////////////////////////////////////////////////////////////////////
// Multiplier
//////////////////////////////////////////////////////////////////////////
module qmult(clk,rst,i_multiplicand,i_multiplier,o_result);
input clk;
input rst;
input [`DWIDTH-1:0] i_multiplicand;
input [`DWIDTH-1:0] i_multiplier;
output [2*`DWIDTH-1:0] o_result;

wire fpmult_16_clk_NC;
wire fpmult_16_rst_NC;
wire [15:0] fpmult_16_result;
wire [4:0] fpmult_16_flags;

FPMult_16 u_fpmult_16(
   .clk(clk),
   .rst(rst),
   .a(i_multiplicand[15:0]),
   .b(i_multiplier[15:0]),
   .result(fpmult_16_result),
   .flags(fpmult_16_flags)
 );

//Convert fp16 to fp32
fp16_to_fp32 u_16to32 (.a(fpmult_16_result), .b(o_result));

endmodule


//////////////////////////////////////////////////////////////////////////
// Adder
//////////////////////////////////////////////////////////////////////////
module qadd(clk,rst,a,b,c);
input clk;
input rst;
input [2*`DWIDTH-1:0] a;
input [2*`DWIDTH-1:0] b;
output [2*`DWIDTH-1:0] c;

wire fpadd_32_clk_NC;
wire fpadd_32_rst_NC;
wire [4:0] fpadd_32_flags;

FPAddSub_single u_fpaddsub_32(
  .clk(clk),
  .rst(rst),
  .a(a),
  .b(b),
  .operation(1'b0), 
  .result(c),
  .flags(fpadd_32_flags));

endmodule
`endif

`ifndef complex_dsp

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Definition of a 16-bit floating point multiplier
// This is a heavily modified version of:
// https://github.com/fbrosser/DSP48E1-FP/tree/master/src/FPMult
// Original author: Fredrik Brosser
// Abridged by: Samidh Mehta
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

module FPMult_16(
		clk,
		rst,
		a,
		b,
		result,
		flags
    );
	
	// Input Ports
	input clk ;							// Clock
	input rst ;							// Reset signal
	input [`DWIDTH-1:0] a;						// Input A, a 32-bit floating point number
	input [`DWIDTH-1:0] b;						// Input B, a 32-bit floating point number
	
	// Output ports
	output [`DWIDTH-1:0] result ;					// Product, result of the operation, 32-bit FP number
	output [4:0] flags ;						// Flags indicating exceptions according to IEEE754
	
	// Internal signals
	wire [`DWIDTH-1:0] Z_int ;					// Product, result of the operation, 32-bit FP number
	wire [4:0] Flags_int ;						// Flags indicating exceptions according to IEEE754
	
	wire Sa ;							// A's sign
	wire Sb ;							// B's sign
	wire Sp ;							// Product sign
	wire [`EXPONENT-1:0] Ea ;					// A's exponent
	wire [`EXPONENT-1:0] Eb ;					// B's exponent
	wire [2*`MANTISSA+1:0] Mp ;					// Product mantissa
	wire [4:0] InputExc ;						// Exceptions in inputs
	wire [`MANTISSA-1:0] NormM ;					// Normalized mantissa
	wire [`EXPONENT:0] NormE ;					// Normalized exponent
	wire [`MANTISSA:0] RoundM ;					// Normalized mantissa
	wire [`EXPONENT:0] RoundE ;					// Normalized exponent
	wire [`MANTISSA:0] RoundMP ;					// Normalized mantissa
	wire [`EXPONENT:0] RoundEP ;					// Normalized exponent
	wire GRS ;

	//reg [63:0] pipe_0;						// Pipeline register Input->Prep
	reg [2*`DWIDTH-1:0] pipe_0;					// Pipeline register Input->Prep

	//reg [92:0] pipe_1;						// Pipeline register Prep->Execute
	//reg [3*`MANTISSA+2*`EXPONENT+7:0] pipe_1;			// Pipeline register Prep->Execute
	reg [3*`MANTISSA+2*`EXPONENT+18:0] pipe_1;

	//reg [38:0] pipe_2;						// Pipeline register Execute->Normalize
	reg [`MANTISSA+`EXPONENT+7:0] pipe_2;				// Pipeline register Execute->Normalize
	
	//reg [72:0] pipe_3;						// Pipeline register Normalize->Round
	reg [2*`MANTISSA+2*`EXPONENT+10:0] pipe_3;			// Pipeline register Normalize->Round

	//reg [36:0] pipe_4;						// Pipeline register Round->Output
	reg [`DWIDTH+4:0] pipe_4;					// Pipeline register Round->Output
	
	assign result = pipe_4[`DWIDTH+4:5] ;
	assign flags = pipe_4[4:0] ;
	
	// Prepare the operands for alignment and check for exceptions
	FPMult_PrepModule PrepModule(clk, rst, pipe_0[2*`DWIDTH-1:`DWIDTH], pipe_0[`DWIDTH-1:0], Sa, Sb, Ea[`EXPONENT-1:0], Eb[`EXPONENT-1:0], Mp[2*`MANTISSA+1:0], InputExc[4:0]) ;

	// Perform (unsigned) mantissa multiplication
	FPMult_ExecuteModule ExecuteModule(pipe_1[3*`MANTISSA+`EXPONENT*2+7:2*`MANTISSA+2*`EXPONENT+8], pipe_1[2*`MANTISSA+2*`EXPONENT+7:2*`MANTISSA+7], pipe_1[2*`MANTISSA+6:5], pipe_1[2*`MANTISSA+2*`EXPONENT+6:2*`MANTISSA+`EXPONENT+7], pipe_1[2*`MANTISSA+`EXPONENT+6:2*`MANTISSA+7], pipe_1[2*`MANTISSA+2*`EXPONENT+8], pipe_1[2*`MANTISSA+2*`EXPONENT+7], Sp, NormE[`EXPONENT:0], NormM[`MANTISSA-1:0], GRS) ;

	// Round result and if necessary, perform a second (post-rounding) normalization step
	FPMult_NormalizeModule NormalizeModule(pipe_2[`MANTISSA-1:0], pipe_2[`MANTISSA+`EXPONENT:`MANTISSA], RoundE[`EXPONENT:0], RoundEP[`EXPONENT:0], RoundM[`MANTISSA:0], RoundMP[`MANTISSA:0]) ;		

	// Round result and if necessary, perform a second (post-rounding) normalization step
	//FPMult_RoundModule RoundModule(pipe_3[47:24], pipe_3[23:0], pipe_3[65:57], pipe_3[56:48], pipe_3[66], pipe_3[67], pipe_3[72:68], Z_int[31:0], Flags_int[4:0]) ;		
	FPMult_RoundModule RoundModule(pipe_3[2*`MANTISSA+1:`MANTISSA+1], pipe_3[`MANTISSA:0], pipe_3[2*`MANTISSA+2*`EXPONENT+3:2*`MANTISSA+`EXPONENT+3], pipe_3[2*`MANTISSA+`EXPONENT+2:2*`MANTISSA+2], pipe_3[2*`MANTISSA+2*`EXPONENT+4], pipe_3[2*`MANTISSA+2*`EXPONENT+5], pipe_3[2*`MANTISSA+2*`EXPONENT+10:2*`MANTISSA+2*`EXPONENT+6], Z_int[`DWIDTH-1:0], Flags_int[4:0]) ;		

//adding always@ (*) instead of posedge clock to make design combinational
	always @ (posedge clk) begin	
		if(rst) begin
			pipe_0 <= 0;
			pipe_1 <= 0;
			pipe_2 <= 0; 
			pipe_3 <= 0;
			pipe_4 <= 0;
		end 
		else begin		
			/* PIPE 0
				[2*`DWIDTH-1:`DWIDTH] A
				[`DWIDTH-1:0] B
			*/
                       pipe_0 <= {a, b} ;


			/* PIPE 1
				[2*`EXPONENT+3*`MANTISSA + 18: 2*`EXPONENT+2*`MANTISSA + 18] //pipe_0[`DWIDTH+`MANTISSA-1:`DWIDTH] , mantissa of A
				[2*`EXPONENT+2*`MANTISSA + 17 :2*`EXPONENT+2*`MANTISSA + 9] // pipe_0[8:0]
				[2*`EXPONENT+2*`MANTISSA + 8] Sa
				[2*`EXPONENT+2*`MANTISSA + 7] Sb
				[2*`EXPONENT+2*`MANTISSA + 6:`EXPONENT+2*`MANTISSA+7] Ea
				[`EXPONENT +2*`MANTISSA+6:2*`MANTISSA+7] Eb
				[2*`MANTISSA+1+5:5] Mp
				[4:0] InputExc
			*/
			//pipe_1 <= {pipe_0[`DWIDTH+`MANTISSA-1:`DWIDTH], pipe_0[`MANTISSA_MUL_SPLIT_LSB-1:0], Sa, Sb, Ea[`EXPONENT-1:0], Eb[`EXPONENT-1:0], Mp[2*`MANTISSA-1:0], InputExc[4:0]} ;
			pipe_1 <= {pipe_0[`DWIDTH+`MANTISSA-1:`DWIDTH], pipe_0[8:0], Sa, Sb, Ea[`EXPONENT-1:0], Eb[`EXPONENT-1:0], Mp[2*`MANTISSA+1:0], InputExc[4:0]} ;
			
			/* PIPE 2
				[`EXPONENT + `MANTISSA + 7:`EXPONENT + `MANTISSA + 3] InputExc
				[`EXPONENT + `MANTISSA + 2] GRS
				[`EXPONENT + `MANTISSA + 1] Sp
				[`EXPONENT + `MANTISSA:`MANTISSA] NormE
				[`MANTISSA-1:0] NormM
			*/
			pipe_2 <= {pipe_1[4:0], GRS, Sp, NormE[`EXPONENT:0], NormM[`MANTISSA-1:0]} ;
			/* PIPE 3
				[2*`EXPONENT+2*`MANTISSA+10:2*`EXPONENT+2*`MANTISSA+6] InputExc
				[2*`EXPONENT+2*`MANTISSA+5] GRS
				[2*`EXPONENT+2*`MANTISSA+4] Sp	
				[2*`EXPONENT+2*`MANTISSA+3:`EXPONENT+2*`MANTISSA+3] RoundE
				[`EXPONENT+2*`MANTISSA+2:2*`MANTISSA+2] RoundEP
				[2*`MANTISSA+1:`MANTISSA+1] RoundM
				[`MANTISSA:0] RoundMP
			*/
			pipe_3 <= {pipe_2[`EXPONENT+`MANTISSA+7:`EXPONENT+`MANTISSA+1], RoundE[`EXPONENT:0], RoundEP[`EXPONENT:0], RoundM[`MANTISSA:0], RoundMP[`MANTISSA:0]} ;
			/* PIPE 4
				[`DWIDTH+4:5] Z
				[4:0] Flags
			*/				
			pipe_4 <= {Z_int[`DWIDTH-1:0], Flags_int[4:0]} ;
		end
	end
		
endmodule



module FPMult_PrepModule (
		clk,
		rst,
		a,
		b,
		Sa,
		Sb,
		Ea,
		Eb,
		Mp,
		InputExc
	);
	
	// Input ports
	input clk ;
	input rst ;
	input [`DWIDTH-1:0] a ;								// Input A, a 32-bit floating point number
	input [`DWIDTH-1:0] b ;								// Input B, a 32-bit floating point number
	
	// Output ports
	output Sa ;										// A's sign
	output Sb ;										// B's sign
	output [`EXPONENT-1:0] Ea ;								// A's exponent
	output [`EXPONENT-1:0] Eb ;								// B's exponent
	output [2*`MANTISSA+1:0] Mp ;							// Mantissa product
	output [4:0] InputExc ;						// Input numbers are exceptions
	
	// Internal signals							// If signal is high...
	wire ANaN ;										// A is a signalling NaN
	wire BNaN ;										// B is a signalling NaN
	wire AInf ;										// A is infinity
	wire BInf ;										// B is infinity
    wire [`MANTISSA-1:0] Ma;
    wire [`MANTISSA-1:0] Mb;
	
	assign ANaN = &(a[`DWIDTH-2:`MANTISSA]) &  |(a[`DWIDTH-2:`MANTISSA]) ;			// All one exponent and not all zero mantissa - NaN
	assign BNaN = &(b[`DWIDTH-2:`MANTISSA]) &  |(b[`MANTISSA-1:0]);			// All one exponent and not all zero mantissa - NaN
	assign AInf = &(a[`DWIDTH-2:`MANTISSA]) & ~|(a[`DWIDTH-2:`MANTISSA]) ;		// All one exponent and all zero mantissa - Infinity
	assign BInf = &(b[`DWIDTH-2:`MANTISSA]) & ~|(b[`DWIDTH-2:`MANTISSA]) ;		// All one exponent and all zero mantissa - Infinity
	
	// Check for any exceptions and put all flags into exception vector
	assign InputExc = {(ANaN | BNaN | AInf | BInf), ANaN, BNaN, AInf, BInf} ;
	//assign InputExc = {(ANaN | ANaN | BNaN |BNaN), ANaN, ANaN, BNaN,BNaN} ;
	
	// Take input numbers apart
	assign Sa = a[`DWIDTH-1] ;							// A's sign
	assign Sb = b[`DWIDTH-1] ;							// B's sign
	assign Ea = a[`DWIDTH-2:`MANTISSA];						// Store A's exponent in Ea, unless A is an exception
	assign Eb = b[`DWIDTH-2:`MANTISSA];						// Store B's exponent in Eb, unless B is an exception	
//    assign Ma = a[`MANTISSA_MSB:`MANTISSA_LSB];
  //  assign Mb = b[`MANTISSA_MSB:`MANTISSA_LSB];
	


	//assign Mp = ({4'b0001, a[`MANTISSA-1:0]}*{4'b0001, b[`MANTISSA-1:9]}) ;
	assign Mp = ({1'b1,a[`MANTISSA-1:0]}*{1'b1, b[`MANTISSA-1:0]}) ;

	
    //We multiply part of the mantissa here
    //Full mantissa of A
    //Bits MANTISSA_MUL_SPLIT_MSB:MANTISSA_MUL_SPLIT_LSB of B
   // wire [`ACTUAL_MANTISSA-1:0] inp_A;
   // wire [`ACTUAL_MANTISSA-1:0] inp_B;
   // assign inp_A = {1'b1, Ma};
   // assign inp_B = {{(`MANTISSA-(`MANTISSA_MUL_SPLIT_MSB-`MANTISSA_MUL_SPLIT_LSB+1)){1'b0}}, 1'b1, Mb[`MANTISSA_MUL_SPLIT_MSB:`MANTISSA_MUL_SPLIT_LSB]};
   // DW02_mult #(`ACTUAL_MANTISSA,`ACTUAL_MANTISSA) u_mult(.A(inp_A), .B(inp_B), .TC(1'b0), .PRODUCT(Mp));
endmodule


module FPMult_ExecuteModule(
		a,
		b,
		MpC,
		Ea,
		Eb,
		Sa,
		Sb,
		Sp,
		NormE,
		NormM,
		GRS
    );

	// Input ports
	input [`MANTISSA-1:0] a ;
	input [2*`EXPONENT:0] b ;
	input [2*`MANTISSA+1:0] MpC ;
	input [`EXPONENT-1:0] Ea ;						// A's exponent
	input [`EXPONENT-1:0] Eb ;						// B's exponent
	input Sa ;								// A's sign
	input Sb ;								// B's sign
	
	// Output ports
	output Sp ;								// Product sign
	output [`EXPONENT:0] NormE ;													// Normalized exponent
	output [`MANTISSA-1:0] NormM ;												// Normalized mantissa
	output GRS ;
	
	wire [2*`MANTISSA+1:0] Mp ;
	
	assign Sp = (Sa ^ Sb) ;												// Equal signs give a positive product
	
   // wire [`ACTUAL_MANTISSA-1:0] inp_a;
   // wire [`ACTUAL_MANTISSA-1:0] inp_b;
   // assign inp_a = {1'b1, a};
   // assign inp_b = {{(`MANTISSA-`MANTISSA_MUL_SPLIT_LSB){1'b0}}, 1'b0, b};
   // DW02_mult #(`ACTUAL_MANTISSA,`ACTUAL_MANTISSA) u_mult(.A(inp_a), .B(inp_b), .TC(1'b0), .PRODUCT(Mp_temp));
   // DW01_add #(2*`ACTUAL_MANTISSA) u_add(.A(Mp_temp), .B(MpC<<`MANTISSA_MUL_SPLIT_LSB), .CI(1'b0), .SUM(Mp), .CO());

	//assign Mp = (MpC<<(2*`EXPONENT+1)) + ({4'b0001, a[`MANTISSA-1:0]}*{1'b0, b[2*`EXPONENT:0]}) ;
	assign Mp = MpC;


	assign NormM = (Mp[2*`MANTISSA+1] ? Mp[2*`MANTISSA:`MANTISSA+1] : Mp[2*`MANTISSA-1:`MANTISSA]); 	// Check for overflow
	assign NormE = (Ea + Eb + Mp[2*`MANTISSA+1]);								// If so, increment exponent
	
	assign GRS = ((Mp[`MANTISSA]&(Mp[`MANTISSA+1]))|(|Mp[`MANTISSA-1:0])) ;
	
endmodule

module FPMult_NormalizeModule(
		NormM,
		NormE,
		RoundE,
		RoundEP,
		RoundM,
		RoundMP
    );

	// Input Ports
	input [`MANTISSA-1:0] NormM ;									// Normalized mantissa
	input [`EXPONENT:0] NormE ;									// Normalized exponent

	// Output Ports
	output [`EXPONENT:0] RoundE ;
	output [`EXPONENT:0] RoundEP ;
	output [`MANTISSA:0] RoundM ;
	output [`MANTISSA:0] RoundMP ; 
	
// EXPONENT = 5 
// EXPONENT -1 = 4
// NEED to subtract 2^4 -1 = 15

wire [`EXPONENT-1 : 0] bias;

assign bias =  ((1<< (`EXPONENT -1)) -1);

	assign RoundE = NormE - bias ;
	assign RoundEP = NormE - bias -1 ;
	assign RoundM = NormM ;
	assign RoundMP = NormM ;

endmodule

module FPMult_RoundModule(
		RoundM,
		RoundMP,
		RoundE,
		RoundEP,
		Sp,
		GRS,
		InputExc,
		Z,
		Flags
    );

	// Input Ports
	input [`MANTISSA:0] RoundM ;									// Normalized mantissa
	input [`MANTISSA:0] RoundMP ;									// Normalized exponent
	input [`EXPONENT:0] RoundE ;									// Normalized mantissa + 1
	input [`EXPONENT:0] RoundEP ;									// Normalized exponent + 1
	input Sp ;												// Product sign
	input GRS ;
	input [4:0] InputExc ;
	
	// Output Ports
	output [`DWIDTH-1:0] Z ;										// Final product
	output [4:0] Flags ;
	
	// Internal Signals
	wire [`EXPONENT:0] FinalE ;									// Rounded exponent
	wire [`MANTISSA:0] FinalM;
	wire [`MANTISSA:0] PreShiftM;
	
	assign PreShiftM = GRS ? RoundMP : RoundM ;	// Round up if R and (G or S)
	
	// Post rounding normalization (potential one bit shift> use shifted mantissa if there is overflow)
	assign FinalM = (PreShiftM[`MANTISSA] ? {1'b0, PreShiftM[`MANTISSA:1]} : PreShiftM[`MANTISSA:0]) ;
	
	assign FinalE = (PreShiftM[`MANTISSA] ? RoundEP : RoundE) ; // Increment exponent if a shift was done
	
	assign Z = {Sp, FinalE[`EXPONENT-1:0], FinalM[`MANTISSA-1:0]} ;   // Putting the pieces together
	assign Flags = InputExc[4:0];

endmodule

`endif

 
//////////////////////////////////////////////////////////////////////////
// A floating point 16-bit to floating point 32-bit converter
//////////////////////////////////////////////////////////////////////////
`ifndef complex_dsp
module fp16_to_fp32 (input [15:0] a , output [31:0] b);

reg [31:0]b_temp;
reg [3:0] j;
reg [3:0] k;
reg [3:0] k_temp;
always @ (*) begin

if ( a [14: 0] == 15'b0 ) begin //signed zero
	b_temp [31] = a[15]; //sign bit
	b_temp[30:0] = 31'b0;
end

else begin

	if ( a[14 : 10] == 5'b0 ) begin //denormalized (covert to normalized)
		
		for (j=0; j<=9; j=j+1) begin
			if (a[j] == 1'b1) begin 
			    k_temp = j;	
			end
		end
	k = 9 - k_temp;

	b_temp [22:0] = ( (a [9:0] << (k+1'b1)) & 10'h3FF ) << 13;
	b_temp [30:23] =  7'd127 - 4'd15 - k;
	b_temp [31] = a[15];
	end

	else if ( a[14 : 10] == 5'b11111 ) begin //Infinity/ NAN
	b_temp [22:0] = a [9:0] << 13;
	b_temp [30:23] = 8'hFF;
	b_temp [31] = a[15];
	end

	else begin //Normalized Number
	b_temp [22:0] = a [9:0] << 13;
	b_temp [30:23] =  7'd127 - 4'd15 + a[14:10];
	b_temp [31] = a[15];
	end
end
end

assign b = b_temp;


endmodule

//////////////////////////////////////////////////////////////////////////
// A floating point 32-bit to floating point 16-bit converter
//////////////////////////////////////////////////////////////////////////
module fp32_to_fp16 (input [31:0] a , output [15:0] b);

reg [15:0]b_temp;
//integer j;
//reg [3:0]k;
always @ (*) begin

if ( a [30: 0] == 15'b0 ) begin //signed zero
	b_temp [15] = a[30]; //sign bit
	b_temp [14:0] = 15'b0; 
end

else begin

	if ( a[30 : 23] <= 8'd112  &&  a[30 : 23] >= 8'd103 ) begin //denormalized (covert to normalized)
		
	b_temp [9:0] = {1'b1, a[22:13]} >> {8'd112 - a[30 : 23] + 1'b1} ;  
	b_temp [14:10] =  5'b0;
	b_temp [15] = a[31];
	end

	else if ( a[ 30 : 23] == 8'b11111111 ) begin //Infinity/ NAN
	b_temp [9:0] = a [22:13];
	b_temp [14:10] = 5'h1F;
	b_temp [15] = a[31];
	end

	else begin //Normalized Number
	b_temp [9:0] = a [22:13];
	b_temp [14:10] = 4'd15 - 7'd127  + a[30:23]; //number should be in the range which can be depicted by fp16 (exp for fp32: 70h, 8Eh ; normalized exp for fp32: -15 to 15)
	b_temp [15] = a[31];
	end
end
end

assign b = b_temp;


endmodule
`endif

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Definition of a 32-bit floating point adder/subtractor
// This is a heavily modified version of:
// https://github.com/fbrosser/DSP48E1-FP/tree/master/src/FP_AddSub
// Original author: Fredrik Brosser
// Abridged by: Samidh Mehta
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

`ifndef complex_dsp
module FPAddSub_single(
		clk,
		rst,
		a,
		b,
		operation,
		result,
		flags
	);

// Clock and reset
	input clk ;										// Clock signal
	input rst ;										// Reset (active high, resets pipeline registers)
	
	// Input ports
	input [31:0] a ;								// Input A, a 32-bit floating point number
	input [31:0] b ;								// Input B, a 32-bit floating point number
	input operation ;								// Operation select signal
	
	// Output ports
	output [31:0] result ;						// Result of the operation
	output [4:0] flags ;							// Flags indicating exceptions according to IEEE754

	reg [68:0]pipe_1;
	reg [54:0]pipe_2;
	reg [45:0]pipe_3;


//internal module wires

//output ports
	wire Opout;
	wire Sa;
	wire Sb;
	wire MaxAB;
	wire [7:0] CExp;
	wire [4:0] Shift;
	wire [22:0] Mmax;
	wire [4:0] InputExc;
	wire [23:0] Mmin_3;

	wire [32:0] SumS_5 ;
	wire [4:0] Shift_1;							
	wire PSgn ;							
	wire Opr ;	
	
	wire [22:0] NormM ;				// Normalized mantissa
	wire [8:0] NormE ;					// Adjusted exponent
	wire ZeroSum ;						// Zero flag
	wire NegE ;							// Flag indicating negative exponent
	wire R ;								// Round bit
	wire S ;								// Final sticky bit
	wire FG ;

FPAddSub_a M1(a,b,operation,Opout,Sa,Sb,MaxAB,CExp,Shift,Mmax,InputExc,Mmin_3);

FpAddSub_b M2(pipe_1[51:29],pipe_1[23:0],pipe_1[67],pipe_1[66],pipe_1[65],pipe_1[68],SumS_5,Shift_1,PSgn,Opr);

FPAddSub_c M3(pipe_2[54:22],pipe_2[21:17],pipe_2[16:9],NormM,NormE,ZeroSum,NegE,R,S,FG);

FPAddSub_d M4(pipe_3[13],pipe_3[22:14],pipe_3[45:23],pipe_3[11],pipe_3[10],pipe_3[9],pipe_3[8],pipe_3[7],pipe_3[6],pipe_3[5],pipe_3[12],pipe_3[4:0],result,flags );


always @ (posedge clk) begin	
		if(rst) begin
			pipe_1 <= 0;
			pipe_2 <= 0;
			pipe_3 <= 0;
		end 
		else begin
/*
pipe_1:
	[68] Opout;
	[67] Sa;
	[66] Sb;
	[65] MaxAB;
	[64:57] CExp;
	[56:52] Shift;
	[51:29] Mmax;
	[28:24] InputExc;
	[23:0] Mmin_3;	

*/

pipe_1 <= {Opout,Sa,Sb,MaxAB,CExp,Shift,Mmax,InputExc,Mmin_3};

/*
pipe_2:
	[54:22]SumS_5;
	[21:17]Shift;
	[16:9]CExp;	
	[8]Sa;
	[7]Sb;
	[6]operation;
	[5]MaxAB;	
	[4:0]InputExc
*/

pipe_2 <= {SumS_5,Shift_1,pipe_1[64:57], pipe_1[67], pipe_1[66], pipe_1[68], pipe_1[65], pipe_1[28:24] };

/*
pipe_3:
	[45:23] NormM ;				
	[22:14] NormE ;					
	[13]ZeroSum ;						
	[12]NegE ;							
	[11]R ;								
	[10]S ;								
	[9]FG ;
	[8]Sa;
	[7]Sb;
	[6]operation;
	[5]MaxAB;	
	[4:0]InputExc
*/

pipe_3 <= {NormM,NormE,ZeroSum,NegE,R,S,FG, pipe_2[8], pipe_2[7], pipe_2[6], pipe_2[5], pipe_2[4:0] };

end
end

endmodule

// Prealign + Align + Shift 1 + Shift 2
module FPAddSub_a(
		A,
		B,
		operation,
		Opout,
		Sa,
		Sb,
		MaxAB,
		CExp,
		Shift,
		Mmax,
		InputExc,
		Mmin_3
		
		
	);
	
	// Input ports
	input [31:0] A ;										// Input A, a 32-bit floating point number
	input [31:0] B ;										// Input B, a 32-bit floating point number
	input operation ;
	
	//output ports
	output Opout;
	output Sa;
	output Sb;
	output MaxAB;
	output [7:0] CExp;
	output [4:0] Shift;
	output [22:0] Mmax;
	output [4:0] InputExc;
	output [23:0] Mmin_3;	
							
	wire [9:0] ShiftDet ;							
	wire [30:0] Aout ;
	wire [30:0] Bout ;
	

	// Internal signals									// If signal is high...
	wire ANaN ;												// A is a NaN (Not-a-Number)
	wire BNaN ;												// B is a NaN
	wire AInf ;												// A is infinity
	wire BInf ;												// B is infinity
	wire [7:0] DAB ;										// ExpA - ExpB					
	wire [7:0] DBA ;										// ExpB - ExpA	
	
	assign ANaN = &(A[30:23]) & |(A[22:0]) ;		// All one exponent and not all zero mantissa - NaN
	assign BNaN = &(B[30:23]) & |(B[22:0]);		// All one exponent and not all zero mantissa - NaN
	assign AInf = &(A[30:23]) & ~|(A[22:0]) ;	// All one exponent and all zero mantissa - Infinity
	assign BInf = &(B[30:23]) & ~|(B[22:0]) ;	// All one exponent and all zero mantissa - Infinity
	
	// Put all flags into exception vector
	assign InputExc = {(ANaN | BNaN | AInf | BInf), ANaN, BNaN, AInf, BInf} ;
	
	//assign DAB = (A[30:23] - B[30:23]) ;
	//assign DBA = (B[30:23] - A[30:23]) ;
  assign DAB = (A[30:23] + ~(B[30:23]) + 1) ;
	assign DBA = (B[30:23] + ~(A[30:23]) + 1) ;

	assign Sa = A[31] ;									// A's sign bit
	assign Sb = B[31] ;									// B's sign	bit
	assign ShiftDet = {DBA[4:0], DAB[4:0]} ;		// Shift data
	assign Opout = operation ;
	assign Aout = A[30:0] ;
	assign Bout = B[30:0] ;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Output ports
													// Number of steps to smaller mantissa shift right
	wire [22:0] Mmin_1 ;							// Smaller mantissa 
	
	// Internal signals
	//wire BOF ;										// Check for shifting overflow if B is larger
	//wire AOF ;										// Check for shifting overflow if A is larger
	
	assign MaxAB = (Aout[30:0] < Bout[30:0]) ;	
	//assign BOF = ShiftDet[9:5] < 25 ;		// Cannot shift more than 25 bits
	//assign AOF = ShiftDet[4:0] < 25 ;		// Cannot shift more than 25 bits
	
	// Determine final shift value
	//assign Shift = MaxAB ? (BOF ? ShiftDet[9:5] : 5'b11001) : (AOF ? ShiftDet[4:0] : 5'b11001) ;
	
	assign Shift = MaxAB ? ShiftDet[9:5] : ShiftDet[4:0] ;
	
	// Take out smaller mantissa and append shift space
	assign Mmin_1 = MaxAB ? Aout[22:0] : Bout[22:0] ; 
	
	// Take out larger mantissa	
	assign Mmax = MaxAB ? Bout[22:0]: Aout[22:0] ;	
	
	// Common exponent
	assign CExp = (MaxAB ? Bout[30:23] : Aout[30:23]) ;	

// Input ports
					// Smaller mantissa after 16|12|8|4 shift
	wire [2:0] Shift_1 ;						// Shift amount
	
	assign Shift_1 = Shift [4:2];

	wire [23:0] Mmin_2 ;						// The smaller mantissa
	
	// Internal signals
	reg	  [23:0]		Lvl1;
	reg	  [23:0]		Lvl2;
	wire    [47:0]    Stage1;	
	integer           i;                // Loop variable
	
	always @(*) begin						
		// Rotate by 16?
		Lvl1 <= Shift_1[2] ? {17'b00000000000000001, Mmin_1[22:16]} : {1'b1, Mmin_1}; 
	end
	
	assign Stage1 = {Lvl1, Lvl1};
	
	always @(*) begin    					// Rotate {0 | 4 | 8 | 12} bits
	  case (Shift_1[1:0])
			// Rotate by 0	
			2'b00:  Lvl2 <= Stage1[23:0];       			
			// Rotate by 4	
			2'b01:  begin for (i=0; i<=23; i=i+1) begin Lvl2[i] <= Stage1[i+4]; end Lvl2[23:19] <= 0; end
			// Rotate by 8
			2'b10:  begin for (i=0; i<=23; i=i+1) begin Lvl2[i] <= Stage1[i+8]; end Lvl2[23:15] <= 0; end
			// Rotate by 12	
			2'b11:  begin for (i=0; i<=23; i=i+1) begin Lvl2[i] <= Stage1[i+12]; end Lvl2[23:11] <= 0; end
	  endcase
	end
	
	// Assign output to next shift stage
	assign Mmin_2 = Lvl2;
								// Smaller mantissa after 16|12|8|4 shift
	wire [1:0] Shift_2 ;						// Shift amount
	
	assign Shift_2 =Shift  [1:0] ;
					// The smaller mantissa
	
	// Internal Signal
	reg	  [23:0]		Lvl3;
	wire    [47:0]    Stage2;	
	integer           j;               // Loop variable
	
	assign Stage2 = {Mmin_2, Mmin_2};

	always @(*) begin    // Rotate {0 | 1 | 2 | 3} bits
	  case (Shift_2[1:0])
			// Rotate by 0
			2'b00:  Lvl3 <= Stage2[23:0];   
			// Rotate by 1
			2'b01:  begin for (j=0; j<=23; j=j+1)  begin Lvl3[j] <= Stage2[j+1]; end Lvl3[23] <= 0; end 
			// Rotate by 2
			2'b10:  begin for (j=0; j<=23; j=j+1)  begin Lvl3[j] <= Stage2[j+2]; end Lvl3[23:22] <= 0; end 
			// Rotate by 3
			2'b11:  begin for (j=0; j<=23; j=j+1)  begin Lvl3[j] <= Stage2[j+3]; end Lvl3[23:21] <= 0; end 	  
	  endcase
	end
	
	// Assign output
	assign Mmin_3 = Lvl3;	

	
endmodule

module FpAddSub_b(
		Mmax,
		Mmin,
		Sa,
		Sb,
		MaxAB,
		OpMode,
		SumS_5,
		Shift,
		PSgn,
		Opr
);
	input [22:0] Mmax ;					// The larger mantissa
	input [23:0] Mmin ;					// The smaller mantissa
	input Sa ;								// Sign bit of larger number
	input Sb ;								// Sign bit of smaller number
	input MaxAB ;							// Indicates the larger number (0/A, 1/B)
	input OpMode ;							// Operation to be performed (0/Add, 1/Sub)
	
	// Output ports
	wire [32:0] Sum ;	
						// Output ports
	output [32:0] SumS_5 ;					// Mantissa after 16|0 shift
	output [4:0] Shift ;					// Shift amount				// The result of the operation
	output PSgn ;							// The sign for the result
	output Opr ;							// The effective (performed) operation

	assign Opr = (OpMode^Sa^Sb); 		// Resolve sign to determine operation

	// Perform effective operation
	assign Sum = (OpMode^Sa^Sb) ? ({1'b1, Mmax, 8'b00000000} - {Mmin, 8'b00000000}) : ({1'b1, Mmax, 8'b00000000} + {Mmin, 8'b00000000}) ;
	// Assign result sign
	assign PSgn = (MaxAB ? Sb : Sa) ;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// Determine normalization shift amount by finding leading nought
	assign Shift =  ( 
		Sum[32] ? 5'b00000 :	 
		Sum[31] ? 5'b00001 : 
		Sum[30] ? 5'b00010 : 
		Sum[29] ? 5'b00011 : 
		Sum[28] ? 5'b00100 : 
		Sum[27] ? 5'b00101 : 
		Sum[26] ? 5'b00110 : 
		Sum[25] ? 5'b00111 :
		Sum[24] ? 5'b01000 :
		Sum[23] ? 5'b01001 :
		Sum[22] ? 5'b01010 :
		Sum[21] ? 5'b01011 :
		Sum[20] ? 5'b01100 :
		Sum[19] ? 5'b01101 :
		Sum[18] ? 5'b01110 :
		Sum[17] ? 5'b01111 :
		Sum[16] ? 5'b10000 :
		Sum[15] ? 5'b10001 :
		Sum[14] ? 5'b10010 :
		Sum[13] ? 5'b10011 :
		Sum[12] ? 5'b10100 :
		Sum[11] ? 5'b10101 :
		Sum[10] ? 5'b10110 :
		Sum[9] ? 5'b10111 :
		Sum[8] ? 5'b11000 :
		Sum[7] ? 5'b11001 : 5'b11010
	);
	
	reg	  [32:0]		Lvl1;
	
	always @(*) begin
		// Rotate by 16?
		Lvl1 <= Shift[4] ? {Sum[16:0], 16'b0000000000000000} : Sum; 
	end
	
	// Assign outputs
	assign SumS_5 = Lvl1;	

endmodule

module FPAddSub_c(
		SumS_5,
		Shift,
		CExp,
		NormM,
		NormE,
		ZeroSum,
		NegE,
		R,
		S,
		FG
	);
	
	// Input ports
	input [32:0] SumS_5 ;						// Smaller mantissa after 16|12|8|4 shift
	
	input [4:0] Shift ;						// Shift amount
	
// Input ports
	
	input [7:0] CExp ;
	

	// Output ports
	output [22:0] NormM ;				// Normalized mantissa
	output [8:0] NormE ;					// Adjusted exponent
	output ZeroSum ;						// Zero flag
	output NegE ;							// Flag indicating negative exponent
	output R ;								// Round bit
	output S ;								// Final sticky bit
	output FG ;


	wire [3:0]Shift_1;
	assign Shift_1 = Shift [3:0];
	// Output ports
	wire [32:0] SumS_7 ;						// The smaller mantissa
	
	reg	  [32:0]		Lvl2;
	wire    [65:0]    Stage1;	
	reg	  [32:0]		Lvl3;
	wire    [65:0]    Stage2;	
	integer           i;               	// Loop variable
	
	assign Stage1 = {SumS_5, SumS_5};

	always @(*) begin    					// Rotate {0 | 4 | 8 | 12} bits
	  case (Shift[3:2])
			// Rotate by 0
			2'b00: Lvl2 <= Stage1[32:0];       		
			// Rotate by 4
			2'b01: begin for (i=65; i>=33; i=i-1) begin Lvl2[i-33] <= Stage1[i-4]; end Lvl2[3:0] <= 0; end
			// Rotate by 8
			2'b10: begin for (i=65; i>=33; i=i-1) begin Lvl2[i-33] <= Stage1[i-8]; end Lvl2[7:0] <= 0; end
			// Rotate by 12
			2'b11: begin for (i=65; i>=33; i=i-1) begin Lvl2[i-33] <= Stage1[i-12]; end Lvl2[11:0] <= 0; end
	  endcase
	end
	
	assign Stage2 = {Lvl2, Lvl2};

	always @(*) begin   				 		// Rotate {0 | 1 | 2 | 3} bits
	  case (Shift_1[1:0])
			// Rotate by 0
			2'b00:  Lvl3 <= Stage2[32:0];
			// Rotate by 1
			2'b01: begin for (i=65; i>=33; i=i-1) begin Lvl3[i-33] <= Stage2[i-1]; end Lvl3[0] <= 0; end 
			// Rotate by 2
			2'b10: begin for (i=65; i>=33; i=i-1) begin Lvl3[i-33] <= Stage2[i-2]; end Lvl3[1:0] <= 0; end
			// Rotate by 3
			2'b11: begin for (i=65; i>=33; i=i-1) begin Lvl3[i-33] <= Stage2[i-3]; end Lvl3[2:0] <= 0; end
	  endcase
	end
	
	// Assign outputs
	assign SumS_7 = Lvl3;						// Take out smaller mantissa



	
	// Internal signals
	wire MSBShift ;						// Flag indicating that a second shift is needed
	wire [8:0] ExpOF ;					// MSB set in sum indicates overflow
	wire [8:0] ExpOK ;					// MSB not set, no adjustment
	
	// Calculate normalized exponent and mantissa, check for all-zero sum
	assign MSBShift = SumS_7[32] ;		// Check MSB in unnormalized sum
	assign ZeroSum = ~|SumS_7 ;			// Check for all zero sum
	assign ExpOK = CExp - Shift ;		// Adjust exponent for new normalized mantissa
	assign NegE = ExpOK[8] ;			// Check for exponent overflow
	assign ExpOF = CExp - Shift + 1'b1 ;		// If MSB set, add one to exponent(x2)
	assign NormE = MSBShift ? ExpOF : ExpOK ;			// Check for exponent overflow
	assign NormM = SumS_7[31:9] ;		// The new, normalized mantissa
	
	// Also need to compute sticky and round bits for the rounding stage
	assign FG = SumS_7[8] ; 
	assign R = SumS_7[7] ;
	assign S = |SumS_7[6:0] ;		
	
endmodule

module FPAddSub_d(
		ZeroSum,
		NormE,
		NormM,
		R,
		S,
		G,
		Sa,
		Sb,
		Ctrl,
		MaxAB,
		NegE,
		InputExc,
		P,
		Flags 
    );

	// Input ports
	input ZeroSum ;					// Sum is zero
	input [8:0] NormE ;				// Normalized exponent
	input [22:0] NormM ;				// Normalized mantissa
	input R ;							// Round bit
	input S ;							// Sticky bit
	input G ;
	input Sa ;							// A's sign bit
	input Sb ;							// B's sign bit
	input Ctrl ;						// Control bit (operation)
	input MaxAB ;
	

	input NegE ;						// Negative exponent?
	input [4:0] InputExc ;					// Exceptions in inputs A and B

	// Output ports
	output [31:0] P ;					// Final result
	output [4:0] Flags ;				// Exception flags
	
	// 
	wire [31:0] Z ;					// Final result
	wire EOF ;
	
	// Internal signals
	wire [23:0] RoundUpM ;			// Rounded up sum with room for overflow
	wire [22:0] RoundM ;				// The final rounded sum
	wire [8:0] RoundE ;				// Rounded exponent (note extra bit due to poential overflow	)
	wire RoundUp ;						// Flag indicating that the sum should be rounded up
	wire ExpAdd ;						// May have to add 1 to compensate for overflow 
	wire RoundOF ;						// Rounding overflow
	wire FSgn;
	// The cases where we need to round upwards (= adding one) in Round to nearest, tie to even
	assign RoundUp = (G & ((R | S) | NormM[0])) ;
	
	// Note that in the other cases (rounding down), the sum is already 'rounded'
	assign RoundUpM = (NormM + 1) ;								// The sum, rounded up by 1
	assign RoundM = (RoundUp ? RoundUpM[22:0] : NormM) ; 	// Compute final mantissa	
	assign RoundOF = RoundUp & RoundUpM[23] ; 				// Check for overflow when rounding up

	// Calculate post-rounding exponent
	assign ExpAdd = (RoundOF ? 1'b1 : 1'b0) ; 				// Add 1 to exponent to compensate for overflow
	assign RoundE = ZeroSum ? 8'b00000000 : (NormE + ExpAdd) ; 							// Final exponent

	// If zero, need to determine sign according to rounding
	assign FSgn = (ZeroSum & (Sa ^ Sb)) | (ZeroSum ? (Sa & Sb & ~Ctrl) : ((~MaxAB & Sa) | ((Ctrl ^ Sb) & (MaxAB | Sa)))) ;

	// Assign final result
	assign Z = {FSgn, RoundE[7:0], RoundM[22:0]} ;
	
	// Indicate exponent overflow
	assign EOF = RoundE[8];

/////////////////////////////////////////////////////////////////////////////////////////////////////////



	
	// Internal signals
	wire Overflow ;					// Overflow flag
	wire Underflow ;					// Underflow flag
	wire DivideByZero ;				// Divide-by-Zero flag (always 0 in Add/Sub)
	wire Invalid ;						// Invalid inputs or result
	wire Inexact ;						// Result is inexact because of rounding
	
	// Exception flags
	
	// Result is too big to be represented
	assign Overflow = EOF | InputExc[1] | InputExc[0] ;
	
	// Result is too small to be represented
	assign Underflow = NegE & (R | S);
	
	// Infinite result computed exactly from finite operands
	assign DivideByZero = &(Z[30:23]) & ~|(Z[30:23]) & ~InputExc[1] & ~InputExc[0];
	
	// Invalid inputs or operation
	assign Invalid = |(InputExc[4:2]) ;
	
	// Inexact answer due to rounding, overflow or underflow
	assign Inexact = (R | S) | Overflow | Underflow;
	
	// Put pieces together to form final result
	assign P = Z ;
	
	// Collect exception flags	
	assign Flags = {Overflow, Underflow, DivideByZero, Invalid, Inexact} ; 	
	
endmodule
`endif


