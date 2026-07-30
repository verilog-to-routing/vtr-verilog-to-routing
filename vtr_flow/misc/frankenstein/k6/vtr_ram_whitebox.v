// vtr_ram_whitebox.v
// whitebox wrappers for rtl-instantiated vtr rams.
//
// read without -lib so hierarchy expands the generate loops at the real
// DATA_WIDTH. with -lib the stub default of 1 would freeze and only dout[0]
// would be driven. the flow later chtypes the bit cells to the arch model
// names for write_blif.
//
// address is padded or truncated to 15 bits (k6 max abits). those pads
// become gnd in the blif and fix_blif_for_vpr.py rewrites them to unconn
// so vpr can pack sibling slices into wide modes.

(* blackbox *)
module vtr_sp_ram_bit (
    input clk,
    input [14:0] addr,
    input data,
    input we,
    output out
);
endmodule

(* blackbox *)
module vtr_dp_ram_bit (
    input clk,
    input [14:0] addr1,
    input [14:0] addr2,
    input data1,
    input data2,
    input we1,
    input we2,
    output out1,
    output out2
);
endmodule


module single_port_ram #(
    parameter ADDR_WIDTH = 1,
    parameter DATA_WIDTH = 1
) (
    input clk,
    input [ADDR_WIDTH-1:0] addr,
    input [DATA_WIDTH-1:0] data,
    input we,
    output [DATA_WIDTH-1:0] out
);

    // k6 max abits is 15
    wire [14:0] addrPad;
    generate
        if (ADDR_WIDTH >= 15)
            assign addrPad = addr[14:0];
        else
            assign addrPad = {{(15 - ADDR_WIDTH){1'b0}}, addr};
    endgenerate

    genvar bitIdx;
    generate
        for (bitIdx = 0; bitIdx < DATA_WIDTH; bitIdx = bitIdx + 1) begin : sliceSp
            vtr_sp_ram_bit mem (
                .clk (clk),
                .addr(addrPad),
                .data(data[bitIdx]),
                .we  (we),
                .out (out[bitIdx])
            );
        end
    endgenerate

endmodule


module dual_port_ram #(
    parameter ADDR_WIDTH = 1,
    parameter DATA_WIDTH = 1
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

    wire [14:0] addrPad1;
    wire [14:0] addrPad2;
    generate
        if (ADDR_WIDTH >= 15) begin
            assign addrPad1 = addr1[14:0];
            assign addrPad2 = addr2[14:0];
        end else begin
            assign addrPad1 = {{(15 - ADDR_WIDTH){1'b0}}, addr1};
            assign addrPad2 = {{(15 - ADDR_WIDTH){1'b0}}, addr2};
        end
    endgenerate

    genvar bitIdx;
    generate
        for (bitIdx = 0; bitIdx < DATA_WIDTH; bitIdx = bitIdx + 1) begin : sliceDp
            vtr_dp_ram_bit mem (
                .clk  (clk),
                .addr1(addrPad1),
                .addr2(addrPad2),
                .data1(data1[bitIdx]),
                .data2(data2[bitIdx]),
                .we1  (we1),
                .we2  (we2),
                .out1 (out1[bitIdx]),
                .out2 (out2[bitIdx])
            );
        end
    endgenerate

endmodule
