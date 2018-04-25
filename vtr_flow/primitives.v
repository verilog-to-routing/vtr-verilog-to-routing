`timescale 1ps/1ps
//Overivew
//========
//This file contains the verilog primitives produced by VPR's
//post-synthesis netlist writer.
//
//If you wish to do back-annotated timing simulation you will need
//to link with this file during simulation.
//
//To ensure currect result when performing back-annoatation with 
//Modelsim see the notes at the end of this comment.
//
//Specifying Timing Edges
//=======================
//To perform timing back-annotation the simulator must know the delay 
//dependancies (timing edges) between the ports on each primitive.
//
//During back-annotation the simulator will attempt to annotate SDF delay
//values onto the timing edges.  It should give a warning if was unable
//to find a matching edge.
//
//
//In Verilog timing edges are specified using a specify block (delimited by the
//'specify' and 'endspecify' keywords.
//
//Inside the specify block a set of specify statements are used to describe
//the timing edges.  For example consider:
//
//  input [1:0] in;
//  output [1:0] out;
//  specify
//      (in[0] => out[0]) = "";
//      (in[1] => out[1]) = "";
//  endspecify
//
//This states that there are the following timing edges (dependancies):
//  * from in[0] to out[0]
//  * from in[1] to out[1]
//
//We could (according to the Verilog standard) equivalently have used:
//
//  input [1:0] in;
//  output [1:0] out;
//  specify
//      (in => out) = "";
//  endspecify
//
//However NOT ALL SIMULATORS TREAT MULTIBIT SPECIFY STATEMENTS CORRECTLY,
//at least by default (in particular ModelSim, see notes below).
//
//The previous examples use the 'parrallel connection' operator '=>', which
//creates parallel edges between the two operands (i.e. bit 0 to bit 0, bit
//1 to bit 1 etc.).  Note that both operands must have the same bit-width. 
//
//Verilog also supports the 'full connection' operator '*>' which will create
//a fully connected set of edges (e.g. from all-to-all). It does not require
//both operands to have the same bit-width. For example:
//
//  input [1:0] in;
//  output [2:0] out;
//  specify
//      (in *> out) = "";
//  endspecify
//
//states that there are the following timing edges (dependancies):
//  * from in[0] to out[0]
//  * from in[0] to out[1]
//  * from in[0] to out[2]
//  * from in[1] to out[0]
//  * from in[1] to out[1]
//  * from in[1] to out[2]
//
//For more details on specify blocks see Section 14 "Specify Blocks" of the
//Verilog standard (IEEE 1364-2005).
//
//Back-annotation with Modelsim
//=============================
//
//Ensuring Multi-bit Specifies are Handled Correctly: Bit-blasting
//----------------------------------------------------------------
//
//ModelSim (tested on Modelsim SE 10.4c) ignores multi-bit specify statements
//by default.
//
//This causes SDF annotation errors such as:
//
//  vsim-SDF-3261: Failed to find matching specify module path
//
//To force Modelsim to correctly interpret multi-bit specify statements you
//should provide the '+bitblast' option to the vsim executable.
//This forces it to apply specify statements using multi-bit operands to
//each bit of the operand (i.e. according to the Verilog standard).
//
//Confirming back-annotation is occuring correctly
//------------------------------------------------
//
//Another useful option is '+sdf_verbose' which produces extra output about
//SDF annotation, which can be used to verify annotation occured correctly.
//
//For example:
//
//      Summary of Verilog design objects annotated: 
//      
//           Module path delays =          5
//      
//       ******************************************************************************
//      
//       Summary of constructs read: 
//      
//                 IOPATH =          5
//
//shows that all 5 IOPATH constructs in the SDF were annotated to the verilog
//design.
//
//Example vsim Command Line
//--------------------------
//The following is an example command-line to vsim (where 'tb' is the name of your
//testbench):
//
//  vsim -t 1ps -L rtl_work -L work -voptargs="+acc" +sdf_verbose +bitblast tb




//K-input Look-Up Table
module LUT_K #(
    //The Look-up Table size (number of inputs)
    parameter K, 

    //The lut mask.  
    //Left-most (MSB) bit corresponds to all inputs logic one. 
    //Defaults to always false.
    parameter LUT_MASK={2**K{1'b0}} 
) (
    input [K-1:0] in,
    output out
);

    specify
        (in *> out) = "";
    endspecify

    assign out = LUT_MASK[in];

endmodule

//D-FlipFlop module
module DFF #(
    parameter INITIAL_VALUE=1'b0    
) (
    input clock,
    input D,
    output reg Q
);

    specify
        (clock => Q) = "";
        $setup(D, posedge clock, "");
        $hold(posedge clock, D, "");
    endspecify

    initial begin
        Q <= INITIAL_VALUE;
    end

    always@(posedge clock) begin
        Q <= D;
    end
endmodule

//Routing fpga_interconnect module
module fpga_interconnect(
    input datain,
    output dataout
);

    specify
        (datain=>dataout)="";
    endspecify

    assign dataout = datain;

endmodule


//2-to-1 mux module
module mux(
    input select,
    input x,
    input y,
    output z
);

    assign z = (x & ~select) | (y & select);

endmodule

//n-bit adder
module adder #(
    parameter WIDTH = 1   
) (
    input [WIDTH-1:0] a, 
    input [WIDTH-1:0] b, 
    input cin, 
    output cout, 
    output [WIDTH-1:0] sumout);

   specify
      (a*>sumout)="";
      (b*>sumout)="";
      (cin*>sumout)="";
      (a*>cout)="";
      (b*>cout)="";
      (cin=>cout)="";
   endspecify
   
   assign {cout, sumout} = a + b + cin;
   
endmodule
   
//nxn multiplier module
module multiply #(
    //The width of input signals
    parameter WIDTH = 1
) (
    input [WIDTH-1:0] a,
    input [WIDTH-1:0] b,
    output [2*WIDTH-1:0] out
);

    specify
        (a *> out) = "";
        (b *> out) = "";
    endspecify

    assign out = a * b;

endmodule // mult

//single_port_ram module
module single_port_ram #(
    parameter ADDR_WIDTH = 1,
    parameter DATA_WIDTH = 1
) (
    input [ADDR_WIDTH-1:0] addr,
    input [DATA_WIDTH-1:0] data,
    input we,
    input clock,
    output reg [DATA_WIDTH-1:0] out
);

    localparam MEM_DEPTH = 2 ** ADDR_WIDTH;

    reg [DATA_WIDTH-1:0] Mem[MEM_DEPTH-1:0];

    specify
        (clock*>out)="";
        $setup(addr, posedge clock, "");
        $setup(data, posedge clock, "");
        $setup(we, posedge clock, "");
        $hold(posedge clock, addr, "");
        $hold(posedge clock, data, "");
        $hold(posedge clock, we, "");
    endspecify
   
    always@(posedge clock) begin
        if(we) begin
            Mem[addr] = data;
        end
    	out = Mem[addr]; //New data read-during write behaviour (blocking assignments)
    end
   
endmodule // single_port_RAM

//dual_port_ram module
module dual_port_ram #(
    parameter ADDR_WIDTH = 1,
    parameter DATA_WIDTH = 1
) (
    input clock,

    input [ADDR_WIDTH-1:0] addr1,
    input [ADDR_WIDTH-1:0] addr2,
    input [DATA_WIDTH-1:0] data1,
    input [DATA_WIDTH-1:0] data2,
    input we1,
    input we2,
    output reg [DATA_WIDTH-1:0] out1,
    output reg [DATA_WIDTH-1:0] out2
);

    localparam MEM_DEPTH = 2 ** ADDR_WIDTH;

    reg [DATA_WIDTH-1:0] Mem[MEM_DEPTH-1:0];

    specify
        (clock*>out1)="";
        (clock*>out2)="";
        $setup(addr1, posedge clock, "");
        $setup(addr2, posedge clock, "");
        $setup(data1, posedge clock, "");
        $setup(data2, posedge clock, "");
        $setup(we1, posedge clock, "");
        $setup(we2, posedge clock, "");
        $hold(posedge clock, addr1, "");
        $hold(posedge clock, addr2, "");
        $hold(posedge clock, data1, "");
        $hold(posedge clock, data2, "");
        $hold(posedge clock, we1, "");
        $hold(posedge clock, we2, "");
    endspecify
   
    always@(posedge clock) begin //Port 1
        if(we1) begin
            Mem[addr1] = data1;
        end
        out1 = Mem[addr1]; //New data read-during write behaviour (blocking assignments)
    end

    always@(posedge clock) begin //Port 2
        if(we2) begin
            Mem[addr2] = data2;
        end
        out2 = Mem[addr2]; //New data read-during write behaviour (blocking assignments)
    end
   
endmodule // dual_port_ram
