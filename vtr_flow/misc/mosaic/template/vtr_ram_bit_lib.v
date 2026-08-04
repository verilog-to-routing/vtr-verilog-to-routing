// vtr_ram_bit_lib.v
// legacy static copy: synthesis.tcl uses the generated
// $archRulesDir/vtr_ram_bit_lib.v (from templates/vtr_ram_bit_lib.v.tmpl).
//
// 1-bit -lib stubs used after chtype from vtr_*_ram_bit.
// the arch model ports are unsized so vpr width is 1. scalar data and out
// (not data[0]) keep the blif matching what vanilla emits.
// addr width comes from the arch (VTR_RAM_ABITS); 15 is the k6 fallback.

`ifndef VTR_RAM_ABITS
`define VTR_RAM_ABITS 15
`endif

(* blackbox *)
module single_port_ram (
    input clk,
    input [`VTR_RAM_ABITS-1:0] addr,
    input data,
    input we,
    output out
);
endmodule

(* blackbox *)
module dual_port_ram (
    input clk,
    input [`VTR_RAM_ABITS-1:0] addr1,
    input [`VTR_RAM_ABITS-1:0] addr2,
    input data1,
    input data2,
    input we1,
    input we2,
    output out1,
    output out2
);
endmodule
