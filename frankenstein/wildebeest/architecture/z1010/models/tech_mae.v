//tech_mae.v
//Peter Grossmann
//19 September 2023
//$Id$
//$Log$

`ifdef BLACKBOX_MACROS
(* blackbox *)
`else
`endif
(* extract_order=8 *)
module efpga_adder
  # (
     parameter INPUT_WIDTH = 18,
     parameter OUTPUT_WIDTH = 40
     )
   (
    input [(INPUT_WIDTH-1):0] 	a,
    input [(INPUT_WIDTH-1):0] 	b,
    output [(OUTPUT_WIDTH-1):0] y
    );

   /* verilator lint_off WIDTH */
   assign y = a + b;
   /* verilator lint_on WIDTH */

endmodule

`ifdef BLACKBOX_MACROS
(* blackbox *)
`else
`endif
(* extract_order=7 *)
module efpga_adder_regi
  # (
     parameter INPUT_WIDTH = 18,
     parameter OUTPUT_WIDTH = 40
     )
   (
    input 			clk,
    input 			resetn,
    input [(INPUT_WIDTH-1):0] 	a,
    input [(INPUT_WIDTH-1):0] 	b,
    output [(OUTPUT_WIDTH-1):0] y
    );

   reg [(INPUT_WIDTH-1):0] 	a_reg;
   reg [(INPUT_WIDTH-1):0] 	b_reg;

   always @(posedge clk or negedge resetn) begin
      if (~resetn) begin
	 a_reg <= 'h0;
	 b_reg <= 'h0;
      end
      else begin
	 a_reg <= a;
	 b_reg <= b;
      end
   end // always @ (posedge clk or negedge resetn)

   /* verilator lint_off WIDTH */
   assign y = a_reg + b_reg;
   /* verilator lint_on WIDTH */

endmodule

`ifdef BLACKBOX_MACROS
(* blackbox *)
`else
`endif
(* extract_order=7 *)
module efpga_adder_rego
  # (
     parameter INPUT_WIDTH = 18,
     parameter OUTPUT_WIDTH = 40
     )
   (
    input 			clk,
    input 			resetn,
    input [(INPUT_WIDTH-1):0] 	a,
    input [(INPUT_WIDTH-1):0] 	b,
    output reg [(OUTPUT_WIDTH-1):0] y
    );

   always @(posedge clk or negedge resetn) begin
      if (~resetn) begin
	 y <= 'h0;
      end
      else begin
	 /* verilator lint_off WIDTH */
	 y <= a + b;
	 /* verilator lint_on WIDTH */
      end
   end

endmodule

`ifdef BLACKBOX_MACROS
(* blackbox *)
`else
`endif
(* extract_order=6 *)
module efpga_adder_regio
  # (
     parameter INPUT_WIDTH = 18,
     parameter OUTPUT_WIDTH = 40
     )
   (
    input 			clk,
    input 			resetn,
    input [(INPUT_WIDTH-1):0] 	a,
    input [(INPUT_WIDTH-1):0] 	b,
    output reg [(OUTPUT_WIDTH-1):0] y
    );

   reg [(INPUT_WIDTH-1):0] 	a_reg;
   reg [(INPUT_WIDTH-1):0] 	b_reg;

   always @(posedge clk or negedge resetn) begin
      if (~resetn) begin
	 a_reg <= 'h0;
	 b_reg <= 'h0;
      end
      else begin
	 a_reg <= a;
	 b_reg <= b;
      end
   end // always @ (posedge clk or negedge resetn)

   always @(posedge clk or negedge resetn) begin
      if (~resetn) begin
	 y <= 'h0;
      end
      else begin
	 /* verilator lint_off WIDTH */
	 y <= a_reg + b_reg;
	 /* verilator lint_on WIDTH */
      end
   end

endmodule

`ifdef BLACKBOX_MACROS
(* blackbox *)
`else
`endif
(* extract_order=5 *)
module efpga_acc_regi
  # (
     parameter INPUT_WIDTH = 18,
     parameter OUTPUT_WIDTH = 40
     )
   (
    input 			clk,
    input 			resetn,
    input [(INPUT_WIDTH-1):0] 	a,
    output reg [(OUTPUT_WIDTH-1):0] y
    );

   reg [(INPUT_WIDTH-1):0] 	a_reg;

   always @(posedge clk or negedge resetn) begin
      if (~resetn) begin
	 a_reg <= 'h0;
      end
      else begin
	 a_reg <= a;
      end
   end // always @ (posedge clk or negedge resetn)

   always @(posedge clk or negedge resetn) begin
      if (~resetn) begin
	 y <= 'h0;
      end
      else begin
	 /* verilator lint_off WIDTH */
	 y <= y + a_reg;
	 /* verilator lint_on WIDTH */
      end
   end

endmodule

`ifdef BLACKBOX_MACROS
(* blackbox *)
`else
`endif
(* extract_order=5 *)
module efpga_acc
  # (
     parameter INPUT_WIDTH = 18,
     parameter OUTPUT_WIDTH = 40
     )
   (
    input 			    clk,
    input 			    resetn,
    input [(INPUT_WIDTH-1):0] 	    a,
    output reg [(OUTPUT_WIDTH-1):0] y
    );

   always @(posedge clk or negedge resetn) begin
      if (~resetn) begin
	 y <= 'h0;
      end
      else begin
	 /* verilator lint_off WIDTH */
	 y <= y + a;
	 /* verilator lint_on WIDTH */
      end
   end

endmodule

`ifdef BLACKBOX_MACROS
(* blackbox *)
`else
`endif
(* extract_order=5 *)
module efpga_mult
  # (
     parameter INPUT_WIDTH = 18,
     parameter OUTPUT_WIDTH = 36
     )
   (
    input [(INPUT_WIDTH-1):0] 	a,
    input [(INPUT_WIDTH-1):0] 	b,
    output [(OUTPUT_WIDTH-1):0] y
    );

   assign y = a * b;

endmodule

`ifdef BLACKBOX_MACROS
(* blackbox *)
`else
`endif
(* extract_order=4 *)
module efpga_mult_regi
  # (
     parameter INPUT_WIDTH = 18,
     parameter OUTPUT_WIDTH = 36
     )
   (
    input 			clk,
    input 			resetn,
    input [(INPUT_WIDTH-1):0] 	a,
    input [(INPUT_WIDTH-1):0] 	b,
    output [(OUTPUT_WIDTH-1):0] y
    );

   reg [(INPUT_WIDTH-1):0] 	a_reg;
   reg [(INPUT_WIDTH-1):0] 	b_reg;

   always @(posedge clk or negedge resetn) begin
      if (~resetn) begin
	 a_reg <= 'h0;
	 b_reg <= 'h0;
      end
      else begin
	 a_reg <= a;
	 b_reg <= b;
      end
   end // always @ (posedge clk or negedge resetn)

   assign y = a_reg * b_reg;

endmodule

`ifdef BLACKBOX_MACROS
(* blackbox *)
`else
`endif
(* extract_order=3 *)
module efpga_mult_rego
  # (
     parameter INPUT_WIDTH = 18,
     parameter OUTPUT_WIDTH = 36
     )
   (
    input 			clk,
    input 			resetn,
    input [(INPUT_WIDTH-1):0] 	a,
    input [(INPUT_WIDTH-1):0] 	b,
    output reg [(OUTPUT_WIDTH-1):0] y
    );

   always @(posedge clk or negedge resetn) begin
      if (~resetn) begin
	 y <= 'h0;
      end
      else begin
	 y <= a * b;
      end
   end

endmodule

`ifdef BLACKBOX_MACROS
(* blackbox *)
`else
`endif
(* extract_order=2 *)
module efpga_mult_regio
  # (
     parameter INPUT_WIDTH = 18,
     parameter OUTPUT_WIDTH = 36
     )
   (
    input 			clk,
    input 			resetn,
    input [(INPUT_WIDTH-1):0] 	a,
    input [(INPUT_WIDTH-1):0] 	b,
    output reg [(OUTPUT_WIDTH-1):0] y
    );

   reg [(INPUT_WIDTH-1):0] 	    a_reg;
   reg [(INPUT_WIDTH-1):0] 	    b_reg;

   always @(posedge clk or negedge resetn) begin
      if (~resetn) begin
	 a_reg <= 'h0;
	 b_reg <= 'h0;
      end
      else begin
	 a_reg <= a;
	 b_reg <= b;
      end
   end // always @ (posedge clk or negedge resetn)

   always @(posedge clk or negedge resetn) begin
      if (~resetn) begin
	 y <= 'h0;
      end
      else begin
	 y <= a_reg * b_reg;
      end
   end

endmodule

`ifdef BLACKBOX_MACROS
(* blackbox *)
`else
`endif
(* extract_order=1 *)
module efpga_macc_regi
  # (
     parameter INPUT_WIDTH = 18,
     parameter OUTPUT_WIDTH = 40
     )
   (
    input 			    clk,
    input 			    resetn,
    input [(INPUT_WIDTH-1):0] 	    a,
    input [(INPUT_WIDTH-1):0] 	    b,
    output reg [(OUTPUT_WIDTH-1):0] y
    );

   wire [(2*INPUT_WIDTH-1):0] 	    product;

   reg [(INPUT_WIDTH-1):0] 	    a_reg;
   reg [(INPUT_WIDTH-1):0] 	    b_reg;

   always @(posedge clk or negedge resetn) begin
      if (~resetn) begin
	 a_reg <= 'h0;
	 b_reg <= 'h0;
      end
      else begin
	 a_reg <= a;
	 b_reg <= b;
      end
   end // always @ (posedge clk or negedge resetn)

   assign product = a_reg * b_reg;

   always @(posedge clk or negedge resetn) begin
      if (~resetn) begin
	 y <= 'h0;
      end
      else begin
	 /* verilator lint_off WIDTH */
	 y <= y + product;
	 /* verilator lint_on WIDTH */
      end
   end

endmodule

`ifdef BLACKBOX_MACROS
(* blackbox *)
`else
`endif
(* extract_order=1 *)
module efpga_macc
  # (
     parameter INPUT_WIDTH = 18,
     parameter OUTPUT_WIDTH = 40
     )
   (
    input 			    clk,
    input 			    resetn,
    input [(INPUT_WIDTH-1):0] 	    a,
    input [(INPUT_WIDTH-1):0] 	    b,
    output reg [(OUTPUT_WIDTH-1):0] y
    );

   wire [(2*INPUT_WIDTH-1):0] 	    product;

   assign product = a * b;

   always @(posedge clk or negedge resetn) begin
      if (~resetn) begin
	 y <= 'h0;
      end
      else begin
	 /* verilator lint_off WIDTH */
	 y <= y + product;
	 /* verilator lint_on WIDTH */
      end
   end

endmodule

`ifdef BLACKBOX_MACROS
(* blackbox *)
`else
`endif
(* extract_order=0 *)
module efpga_macc_pipe_regi
  # (
     parameter INPUT_WIDTH = 18,
     parameter OUTPUT_WIDTH = 40
     )
   (
    input 			    clk,
    input 			    resetn,
    input [(INPUT_WIDTH-1):0] 	    a,
    input [(INPUT_WIDTH-1):0] 	    b,
    output reg [(OUTPUT_WIDTH-1):0] y
    );

   reg [(2*INPUT_WIDTH-1):0] 	    product;

   reg [(INPUT_WIDTH-1):0] 	    a_reg;
   reg [(INPUT_WIDTH-1):0] 	    b_reg;

   always @(posedge clk or negedge resetn) begin
      if (~resetn) begin
	 a_reg <= 'h0;
	 b_reg <= 'h0;
      end
      else begin
	 a_reg <= a;
	 b_reg <= b;
      end
   end // always @ (posedge clk or negedge resetn)

   always @(posedge clk or negedge resetn) begin
      if (~resetn) begin
	 product <= 'h0;
      end
      else begin
	 product <= a_reg * b_reg;
      end
   end

   always @(posedge clk or negedge resetn) begin
      if (~resetn) begin
	 y <= 'h0;
      end
      else begin
	 /* verilator lint_off WIDTH */
	 y <= y + product;
	 /* verilator lint_on WIDTH */
      end
   end

endmodule // macc_pipe_regi

`ifdef BLACKBOX_MACROS
(* blackbox *)
`else
`endif
(* extract_order=0 *)
module efpga_macc_pipe
  # (
     parameter INPUT_WIDTH = 18,
     parameter OUTPUT_WIDTH = 40
     )
   (
    input 			    clk,
    input 			    resetn,
    input [(INPUT_WIDTH-1):0] 	    a,
    input [(INPUT_WIDTH-1):0] 	    b,
    output reg [(OUTPUT_WIDTH-1):0] y
    );

   reg [(2*INPUT_WIDTH-1):0] 	    product;

   always @(posedge clk or negedge resetn) begin
      if (~resetn) begin
	 product <= 'h0;
      end
      else begin
	 product <= a * b;
      end
   end

   always @(posedge clk or negedge resetn) begin
      if (~resetn) begin
	 y <= 'h0;
      end
      else begin
	 /* verilator lint_off WIDTH */
	 y <= y + product;
	 /* verilator lint_on WIDTH */
      end
   end

endmodule // macc_pipe

`ifdef BLACKBOX_MACROS
(* blackbox *)
`else
`endif
(* extract_order=2 *)
module efpga_mult_addc_regio
  # (
     parameter INPUT_WIDTH = 18,
     parameter OUTPUT_WIDTH = 40,
     localparam C_WIDTH = OUTPUT_WIDTH
     )
   (
    input 			clk,
    input 			resetn,
    input [(INPUT_WIDTH-1):0] 	a,
    input [(INPUT_WIDTH-1):0] 	b,
    input [(C_WIDTH-1):0]   c,
    output reg [(OUTPUT_WIDTH-1):0] y
    );

   reg [(INPUT_WIDTH-1):0] 	    a_reg;
   reg [(INPUT_WIDTH-1):0] 	    b_reg;
   reg [(C_WIDTH-1):0] 	    c_reg;

   always @(posedge clk or negedge resetn) begin
      if (~resetn) begin
	 a_reg <= 'h0;
	 b_reg <= 'h0;
    c_reg <= 'h0;
      end
      else begin
	 a_reg <= a;
	 b_reg <= b;
    c_reg <= c;
      end
   end

   always @(posedge clk or negedge resetn) begin
      if (~resetn) begin
	 y <= 'h0;
      end
      else begin
	 y <= (a_reg * b_reg) + c_reg;
      end
   end

endmodule

`ifdef BLACKBOX_MACROS
(* blackbox *)
`else
`endif
(* extract_order=2 *)
module efpga_mult_addc_regi
  # (
     parameter INPUT_WIDTH = 18,
     parameter OUTPUT_WIDTH = 40,
     localparam C_WIDTH = OUTPUT_WIDTH
     )
   (
    input 			clk,
    input 			resetn,
    input [(INPUT_WIDTH-1):0] 	a,
    input [(INPUT_WIDTH-1):0] 	b,
    input [(C_WIDTH-1):0]   c,
    output [(OUTPUT_WIDTH-1):0] y
    );

   reg [(INPUT_WIDTH-1):0] 	a_reg;
   reg [(INPUT_WIDTH-1):0] 	b_reg;
   reg [(C_WIDTH-1):0] 	      c_reg;

   always @(posedge clk or negedge resetn) begin
      if (~resetn) begin
	 a_reg <= 'h0;
	 b_reg <= 'h0;
    c_reg <= 'h0;
      end
      else begin
	 a_reg <= a;
	 b_reg <= b;
    c_reg <= c;
      end
   end

   assign y = (a_reg * b_reg) + c_reg;

endmodule


`ifdef BLACKBOX_MACROS
(* blackbox *)
`else
`endif
(* extract_order=2 *)
module efpga_mult_addc_rego
  # (
     parameter INPUT_WIDTH = 18,
     parameter OUTPUT_WIDTH = 40,
     localparam C_WIDTH = OUTPUT_WIDTH
     )
   (
    input 			clk,
    input 			resetn,
    input [(INPUT_WIDTH-1):0] 	a,
    input [(INPUT_WIDTH-1):0] 	b,
    input [(C_WIDTH-1):0]        c,
    output reg [(OUTPUT_WIDTH-1):0] y
    );

   always @(posedge clk or negedge resetn) begin
      if (~resetn) begin
	 y <= 'h0;
      end
      else begin
	 y <= (a * b) + c;
      end
   end

endmodule

`ifdef BLACKBOX_MACROS
(* blackbox *)
`else
`endif
(* extract_order=2 *)
module efpga_mult_addc
  # (
     parameter INPUT_WIDTH = 18,
     parameter OUTPUT_WIDTH = 40,
     localparam C_WIDTH = OUTPUT_WIDTH
     )
   (
    input [(INPUT_WIDTH-1):0] 	a,
    input [(INPUT_WIDTH-1):0] 	b,
    input [C_WIDTH-1:0] 	c,
    output [(OUTPUT_WIDTH-1):0] y
    );

   assign y = a * b + c;

endmodule
