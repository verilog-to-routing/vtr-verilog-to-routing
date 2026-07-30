// vtr_hardblock_lib.v
// fallback static for k6. normally regenerated from the arch xml by
// vtr_arch_rules. -lib stubs sized to the arch max port widths.
//
// +/parmys/vtr_primitives.v defaults to WIDTH=1 and -lib freezes that so
// write_blif would emit 1-bit blackboxes and leave the rest of the bus
// undriven. these stubs use the real max widths instead.
//
// the ram stubs are only here so the first hierarchy can resolve rtl
// instances without -auto-top wandering into a whitebox. DATA_WIDTH has to
// beat every rtl memory in the suite or high bits get truncated at read.
// the whitebox later bit-slices them. keep the modules bodiless.

(* blackbox *)
module adder #(
    parameter WIDTH = 1
) (
    input [WIDTH-1:0] a,
    input [WIDTH-1:0] b,
    input cin,
    output cout,
    output [WIDTH-1:0] sumout
);
endmodule

// default WIDTH is the widest mode so -lib does not freeze a 1-bit multiply
(* blackbox *)
module multiply #(
    parameter WIDTH = 36
) (
    input [WIDTH-1:0] a,
    input [WIDTH-1:0] b,
    output [2*WIDTH-1:0] out
);
endmodule

// 512 beats the widest rtl memory in the suite (313 on mkDelayWorker32B)
(* blackbox *)
module single_port_ram #(
    parameter ADDR_WIDTH = 16,
    parameter DATA_WIDTH = 512
) (
    input clk,
    input [ADDR_WIDTH-1:0] addr,
    input [DATA_WIDTH-1:0] data,
    input we,
    output [DATA_WIDTH-1:0] out
);
endmodule

(* blackbox *)
module dual_port_ram #(
    parameter ADDR_WIDTH = 16,
    parameter DATA_WIDTH = 512
) (
    input clk,
    input [ADDR_WIDTH-1:0] addr1,
    input [ADDR_WIDTH-1:0] addr2,
    input [DATA_WIDTH-1:0] data1,
    input [DATA_WIDTH-1:0] data2,
    input we1,
    input we2,
    output [DATA_WIDTH-1:0] out1,
    output [DATA_WIDTH-1:0] out2
);
endmodule

(* blackbox *)
module mux (
    input select,
    input x,
    input y,
    output z
);
endmodule

(* blackbox *)
module fpga_interconnect (
    input datain,
    output dataout
);
endmodule
