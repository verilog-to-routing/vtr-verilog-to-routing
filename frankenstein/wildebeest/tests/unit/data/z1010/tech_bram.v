//tech_bram.v
//Peter Grossmann
//19 May 2022
//$Id$
//$Log$

module \$_tech_spram_
  # (
     parameter PORT_A_WIDTH = 64,
     parameter PORT_A_ABITS = 15,
     parameter PORT_A_WR_EN_WIDTH = 1,
     parameter PORT_A_OPTION_RDWR = "OLD"
     ) 
   (
    input                            PORT_A_CLK,
    input                            PORT_A_CLK_EN,
    input [(PORT_A_WR_EN_WIDTH-1):0] PORT_A_WR_EN,
    input                            PORT_A_RD_EN,
    input [(PORT_A_ABITS-1):0]       PORT_A_ADDR,
    input [PORT_A_WIDTH-1:0]         PORT_A_WR_DATA,
    output [PORT_A_WIDTH-1:0]        PORT_A_RD_DATA
    );

   generate

      if (PORT_A_WIDTH > 64) begin

	 wire _TECHMAP_FAIL_ = 1'b1;

      end
      else if (PORT_A_WIDTH > 32) begin

	 spram_512x64 _TECHMAP_REPLACE_ (
					  .clk(PORT_A_CLK),
					  .ce(PORT_A_CLK_EN),
					  .we(PORT_A_WR_EN),
					  .re(PORT_A_RD_EN),
					  .addr(PORT_A_ADDR[14:6]),
					  .din(PORT_A_WR_DATA),
					  .dout(PORT_A_RD_DATA)
					  );	 

      end // if (PORT_A_WIDTH > 32)
      else if (PORT_A_WIDTH > 16) begin
	 
	 spram_1024x32 _TECHMAP_REPLACE_ (
					  .clk(PORT_A_CLK),
					  .ce(PORT_A_CLK_EN),
					  .we(PORT_A_WR_EN),
					  .re(PORT_A_RD_EN),
					  .addr(PORT_A_ADDR[14:5]),
					  .din(PORT_A_WR_DATA),
					  .dout(PORT_A_RD_DATA)
					  );	 
      end // if (PORT_A_WIDTH > 16)
      else if (PORT_A_WIDTH > 8) begin
	 
	 spram_2048x16 _TECHMAP_REPLACE_ (
					  .clk(PORT_A_CLK),
					  .ce(PORT_A_CLK_EN),
					  .we(PORT_A_WR_EN),
					  .re(PORT_A_RD_EN),
					  .addr(PORT_A_ADDR[14:4]),
					  .din(PORT_A_WR_DATA),
					  .dout(PORT_A_RD_DATA)
					  );	 
      end // if (PORT_A_WIDTH > 8)
      else if (PORT_A_WIDTH > 4) begin
	 
	 spram_4096x8 _TECHMAP_REPLACE_ (
					 .clk(PORT_A_CLK),
					 .ce(PORT_A_CLK_EN),
					 .we(PORT_A_WR_EN),
					 .re(PORT_A_RD_EN),
					 .addr(PORT_A_ADDR[14:3]),
					 .din(PORT_A_WR_DATA),
					 .dout(PORT_A_RD_DATA)
					 );	 
      end // if (PORT_A_WIDTH > 4)
      else if (PORT_A_WIDTH > 2) begin
	 
	 spram_8192x4 _TECHMAP_REPLACE_ (
					 .clk(PORT_A_CLK),
					 .ce(PORT_A_CLK_EN),
					 .we(PORT_A_WR_EN),
					 .re(PORT_A_RD_EN),
					 .addr(PORT_A_ADDR[14:2]),
					 .din(PORT_A_WR_DATA),
					 .dout(PORT_A_RD_DATA)
					 );	 
      end // if (PORT_A_WIDTH > 2)
      else if (PORT_A_WIDTH > 1) begin
	 
	 spram_16384x2 _TECHMAP_REPLACE_ (
					  .clk(PORT_A_CLK),
					  .ce(PORT_A_CLK_EN),
					  .we(PORT_A_WR_EN),
					  .re(PORT_A_RD_EN),
					  .addr(PORT_A_ADDR[14:1]),
					  .din(PORT_A_WR_DATA),
					  .dout(PORT_A_RD_DATA)
					  );	 
      end // if (PORT_A_WIDTH > 1)
      else begin
	 
	 spram_32768x1 _TECHMAP_REPLACE_ (
					  .clk(PORT_A_CLK),
					  .ce(PORT_A_CLK_EN),
					  .we(PORT_A_WR_EN),
					  .re(PORT_A_RD_EN),
					  .addr(PORT_A_ADDR),
					  .din(PORT_A_WR_DATA),
					  .dout(PORT_A_RD_DATA)
					  );	 
      end // else: !if(PORT_A_WIDTH > 1)
      
   endgenerate
   
   
endmodule 

module \$_tech_sdpram_
  # (
     parameter PORT_A_WIDTH = 32,
     parameter PORT_A_ABITS = 15,
     parameter PORT_A_WR_EN_WIDTH = 1,
     parameter PORT_B_WIDTH = 32,
     parameter PORT_B_ABITS = 15,
     parameter PORT_B_RD_EN_WIDTH = 1,
     parameter OPTION_SYNC_MODE = 0
     ) 
   (
    input 			     CLK_C,
    input 			     PORT_A_CLK,
    input 			     PORT_A_CLK_EN,
    input [(PORT_A_WR_EN_WIDTH-1):0] PORT_A_WR_EN,
    input [(PORT_A_ABITS-1):0] 	     PORT_A_ADDR,
    input [PORT_A_WIDTH-1:0] 	     PORT_A_WR_DATA,
   
    input 			     PORT_B_CLK,
    input 			     PORT_B_CLK_EN,
    input [(PORT_B_RD_EN_WIDTH-1):0] PORT_B_RD_EN,
    input [(PORT_B_ABITS-1):0] 	     PORT_B_ADDR,
    output [PORT_B_WIDTH-1:0] 	     PORT_B_RD_DATA
    );

   wire                              CLK_A_ASSIGNED;
   wire                              CLK_B_ASSIGNED;
   
   
   generate

      if (OPTION_SYNC_MODE == 1) begin
         assign CLK_A_ASSIGNED = CLK_C;
         assign CLK_B_ASSIGNED = CLK_C;
      end
      else begin
         assign CLK_A_ASSIGNED = PORT_A_CLK;
         assign CLK_B_ASSIGNED = PORT_B_CLK;
      end
      
      if (PORT_A_WIDTH > 32) begin
	 wire _TECHMAP_FAIL_ = 1'b1;
      end
      else if (PORT_A_WIDTH > 16) begin
	 
	 sdpram_1024x32 _TECHMAP_REPLACE_ (
					   .clk_a(CLK_A_ASSIGNED),
					   .clk_b(CLK_B_ASSIGNED),
					   .ce_a(PORT_A_CLK_EN),
					   .we_a(PORT_A_WR_EN),
					   .addr_a(PORT_A_ADDR[14:5]),
					   .din_a(PORT_A_WR_DATA),
					   .ce_b(PORT_B_CLK_EN),
					   .re_b(PORT_B_RD_EN),
					   .addr_b(PORT_B_ADDR[14:5]),
					   .dout_b(PORT_B_RD_DATA)
					   );	 
      end // if (PORT_A_WIDTH > 16)
      else if (PORT_A_WIDTH > 8) begin
	 
	 sdpram_2048x16 _TECHMAP_REPLACE_ (
					   .clk_a(CLK_A_ASSIGNED),
					   .clk_b(CLK_B_ASSIGNED),
					   .ce_a(PORT_A_CLK_EN),
					   .we_a(PORT_A_WR_EN),
					   .addr_a(PORT_A_ADDR[14:4]),
					   .din_a(PORT_A_WR_DATA),
					   .ce_b(PORT_B_CLK_EN),
					   .re_b(PORT_B_RD_EN),
					   .addr_b(PORT_B_ADDR[14:4]),
					   .dout_b(PORT_B_RD_DATA)
					   );	 
      end // if (PORT_A_WIDTH > 8)
      else if (PORT_A_WIDTH > 4) begin
	 
	 sdpram_4096x8 _TECHMAP_REPLACE_ (
					  .clk_a(CLK_A_ASSIGNED),
					  .clk_b(CLK_B_ASSIGNED),
					  .ce_a(PORT_A_CLK_EN),
					  .we_a(PORT_A_WR_EN),
					  .addr_a(PORT_A_ADDR[14:3]),
					  .din_a(PORT_A_WR_DATA),
					  .ce_b(PORT_B_CLK_EN),
					  .re_b(PORT_B_RD_EN),
					  .addr_b(PORT_B_ADDR[14:3]),
					  .dout_b(PORT_B_RD_DATA)
					  );	 
      end // if (PORT_A_WIDTH > 4)
      else if (PORT_A_WIDTH > 2) begin
	 
	 sdpram_8192x4 _TECHMAP_REPLACE_ (
					  .clk_a(CLK_A_ASSIGNED),
					  .clk_b(CLK_B_ASSIGNED),
					  .ce_a(PORT_A_CLK_EN),
					  .we_a(PORT_A_WR_EN),
					  .addr_a(PORT_A_ADDR[14:2]),
					  .din_a(PORT_A_WR_DATA),
					  .ce_b(PORT_B_CLK_EN),
					  .re_b(PORT_B_RD_EN),
					  .addr_b(PORT_B_ADDR[14:2]),
					  .dout_b(PORT_B_RD_DATA)
					  );	 
      end // if (PORT_A_WIDTH > 2)
      else if (PORT_A_WIDTH > 1) begin
	 
	 sdpram_16384x2 _TECHMAP_REPLACE_ (
					   .clk_a(CLK_A_ASSIGNED),
					   .clk_b(CLK_B_ASSIGNED),
					   .ce_a(PORT_A_CLK_EN),
					   .we_a(PORT_A_WR_EN),
					   .addr_a(PORT_A_ADDR[14:1]),
					   .din_a(PORT_A_WR_DATA),
					   .ce_b(PORT_B_CLK_EN),
					   .re_b(PORT_B_RD_EN),
					   .addr_b(PORT_B_ADDR[14:1]),
					   .dout_b(PORT_B_RD_DATA)
					   );	 
      end // if (PORT_A_WIDTH > 2)
      else begin
	 
	 sdpram_32768x1 _TECHMAP_REPLACE_ (
					   .clk_a(CLK_A_ASSIGNED),
					   .clk_b(CLK_B_ASSIGNED),
					   .ce_a(PORT_A_CLK_EN),
					   .we_a(PORT_A_WR_EN),
					   .addr_a(PORT_A_ADDR),
					   .din_a(PORT_A_WR_DATA),
					   .ce_b(PORT_B_CLK_EN),
					   .re_b(PORT_B_RD_EN),
					   .addr_b(PORT_B_ADDR),
					   .dout_b(PORT_B_RD_DATA)
					   );	 
      end // else: !if(PORT_A_WIDTH > 1)
      
   endgenerate

endmodule


module \$_tech_tdpram_
  # (
     parameter PORT_A_WIDTH = 32,
     parameter PORT_A_ABITS = 15,
     parameter PORT_A_WR_EN_WIDTH = 1,
     parameter PORT_B_WIDTH = 32,
     parameter PORT_B_ABITS = 15,
     parameter PORT_B_WR_EN_WIDTH = 1,
     parameter OPTION_SYNC_MODE = 0
     ) 
   (
    input                            CLK_C,
    input                            PORT_A_CLK,
    input                            PORT_A_CLK_EN,
    input [(PORT_A_WR_EN_WIDTH-1):0] PORT_A_WR_EN,
    input                            PORT_A_RD_EN,
    input [(PORT_A_ABITS-1):0]       PORT_A_ADDR,
    input [PORT_A_WIDTH-1:0]         PORT_A_WR_DATA,
    output [PORT_A_WIDTH-1:0]        PORT_A_RD_DATA,
   
    input                            PORT_B_CLK,
    input                            PORT_B_CLK_EN,
    input [(PORT_B_WR_EN_WIDTH-1):0] PORT_B_WR_EN,
    input                            PORT_B_RD_EN,
    input [(PORT_B_ABITS-1):0]       PORT_B_ADDR,
    input [PORT_B_WIDTH-1:0]         PORT_B_WR_DATA,
    output [PORT_B_WIDTH-1:0]        PORT_B_RD_DATA
    );

   wire                              CLK_A_ASSIGNED;
   wire                              CLK_B_ASSIGNED;
   
   
   generate

      if (OPTION_SYNC_MODE == 1) begin
         assign CLK_A_ASSIGNED = CLK_C;
         assign CLK_B_ASSIGNED = CLK_C;
      end
      else begin
         assign CLK_A_ASSIGNED = PORT_A_CLK;
         assign CLK_B_ASSIGNED = PORT_B_CLK;
      end
      
      if (PORT_A_WIDTH > 32) begin
	 wire _TECHMAP_FAIL_ = 1'b1;
      end
      else if (PORT_A_WIDTH > 16) begin
	 
	 tdpram_1024x32 _TECHMAP_REPLACE_ (
					   .clk_a(CLK_A_ASSIGNED),
					   .clk_b(CLK_B_ASSIGNED),
					   .ce_a(PORT_A_CLK_EN),
					   .we_a(PORT_A_WR_EN),
					   .re_a(PORT_A_RD_EN),
					   .addr_a(PORT_A_ADDR[14:5]),
					   .din_a(PORT_A_WR_DATA),
					   .dout_a(PORT_A_RD_DATA),
					   .ce_b(PORT_B_CLK_EN),
					   .re_b(PORT_B_RD_EN),
					   .we_b(PORT_B_WR_EN),
					   .addr_b(PORT_B_ADDR[14:5]),
					   .din_b(PORT_B_WR_DATA),
					   .dout_b(PORT_B_RD_DATA)
					   );	 
      end // if (PORT_A_WIDTH > 16)
      else if (PORT_A_WIDTH > 8) begin
	 
	 tdpram_2048x16 _TECHMAP_REPLACE_ (
					   .clk_a(CLK_A_ASSIGNED),
					   .clk_b(CLK_B_ASSIGNED),
					   .ce_a(PORT_A_CLK_EN),
					   .we_a(PORT_A_WR_EN),
					   .re_a(PORT_A_RD_EN),
					   .addr_a(PORT_A_ADDR[14:4]),
					   .din_a(PORT_A_WR_DATA),
					   .dout_a(PORT_A_RD_DATA),
					   .ce_b(PORT_B_CLK_EN),
					   .re_b(PORT_B_RD_EN),
					   .we_b(PORT_B_WR_EN),
					   .addr_b(PORT_B_ADDR[14:4]),
					   .din_b(PORT_B_WR_DATA),
					   .dout_b(PORT_B_RD_DATA)
					   );	 
      end // if (PORT_A_WIDTH > 8)
      else if (PORT_A_WIDTH > 4) begin
	 
	 tdpram_4096x8 _TECHMAP_REPLACE_ (
					  .clk_a(CLK_A_ASSIGNED),
					  .clk_b(CLK_B_ASSIGNED),
					  .ce_a(PORT_A_CLK_EN),
					  .we_a(PORT_A_WR_EN),
					  .re_a(PORT_A_RD_EN),
					   .addr_a(PORT_A_ADDR[14:3]),
					  .din_a(PORT_A_WR_DATA),
					  .dout_a(PORT_A_RD_DATA),
					  .ce_b(PORT_B_CLK_EN),
					  .re_b(PORT_B_RD_EN),
					  .we_b(PORT_B_WR_EN),
					   .addr_b(PORT_B_ADDR[14:3]),
					  .din_b(PORT_B_WR_DATA),
					  .dout_b(PORT_B_RD_DATA)
					  );	 
      end // if (PORT_A_WIDTH > 4)
      else if (PORT_A_WIDTH > 2) begin
	 
	 tdpram_8192x4 _TECHMAP_REPLACE_ (
					  .clk_a(CLK_A_ASSIGNED),
					  .clk_b(CLK_B_ASSIGNED),
					  .ce_a(PORT_A_CLK_EN),
					  .we_a(PORT_A_WR_EN),
					  .re_a(PORT_A_RD_EN),
					   .addr_a(PORT_A_ADDR[14:2]),
					  .din_a(PORT_A_WR_DATA),
					  .dout_a(PORT_A_RD_DATA),
					  .ce_b(PORT_B_CLK_EN),
					  .re_b(PORT_B_RD_EN),
					  .we_b(PORT_B_WR_EN),
					   .addr_b(PORT_B_ADDR[14:2]),
					  .din_b(PORT_B_WR_DATA),
					  .dout_b(PORT_B_RD_DATA)
					  );	 
      end // if (PORT_A_WIDTH > 2)
      else if (PORT_A_WIDTH > 1) begin
	 
	 tdpram_16384x2 _TECHMAP_REPLACE_ (
					   .clk_a(CLK_A_ASSIGNED),
					   .clk_b(CLK_B_ASSIGNED),
					   .ce_a(PORT_A_CLK_EN),
					   .we_a(PORT_A_WR_EN),
					   .re_a(PORT_A_RD_EN),
					   .addr_a(PORT_A_ADDR[14:1]),
					   .din_a(PORT_A_WR_DATA),
					   .dout_a(PORT_A_RD_DATA),
					   .ce_b(PORT_B_CLK_EN),
					   .re_b(PORT_B_RD_EN),
					   .we_b(PORT_B_WR_EN),
					   .addr_b(PORT_B_ADDR[14:1]),
					   .din_b(PORT_B_WR_DATA),
					   .dout_b(PORT_B_RD_DATA)
					   );	 
      end // if (PORT_A_WIDTH > 2)
      else begin
	 
	 tdpram_32768x1 _TECHMAP_REPLACE_ (
					   .clk_a(CLK_A_ASSIGNED),
					   .clk_b(CLK_B_ASSIGNED),
					   .ce_a(PORT_A_CLK_EN),
					   .we_a(PORT_A_WR_EN),
					   .re_a(PORT_A_RD_EN),
					   .addr_a(PORT_A_ADDR),
					   .din_a(PORT_A_WR_DATA),
					   .dout_a(PORT_A_RD_DATA),
					   .ce_b(PORT_B_CLK_EN),
					   .re_b(PORT_B_RD_EN),
					   .we_b(PORT_B_WR_EN),
					   .addr_b(PORT_B_ADDR),
					   .din_b(PORT_B_WR_DATA),
					   .dout_b(PORT_B_RD_DATA)
					   );	 
      end // else: !if(PORT_A_WIDTH > 1)
      
   endgenerate
      
endmodule
