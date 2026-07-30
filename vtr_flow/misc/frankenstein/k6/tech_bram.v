// tech_bram.v
// fallback static for k6. normally regenerated from the arch xml by
// vtr_arch_rules. turns memory_libmap cells into 1-bit vtr ram bit cells.
//
// the arch declares the ram data ports unsized which vpr treats as 1-bit
// so every mapped memory has to be sliced the same way the whitebox does.
// the flow later chtypes the bit cells to the arch model names for
// write_blif.
//
// unused high addr bits tie to 0 here and become gnd in the blif.
// fix_blif_for_vpr.py rewrites those pads to unconn so vpr will pack
// sibling slices into wide modes instead of one 32kx1 tile per bit.
//
// recursive replace instead of a generate-for because techmap elaborates
// for-loops with the default parameters so a loop over PORT_A_WIDTH never
// actually expands for wide cells.

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


// ============================================================================
// single-port ram
// ============================================================================
module \$__VTR_SP_RAM__ # (
    parameter PORT_A_WIDTH       = 1,
    parameter PORT_A_ABITS       = 15,
    parameter PORT_A_WR_EN_WIDTH = 1,
    parameter PORT_A_RD_USED     = 1,
    parameter PORT_A_WR_USED     = 1,
    // 0 means derive once then freeze across the WIDTH-1 recursion so each
    // slice keeps the same addr mode
    parameter ADDR_BITS_FIXED    = 0
) (
    input                            PORT_A_CLK,
    input  [PORT_A_ABITS-1:0]        PORT_A_ADDR,
    input  [PORT_A_WIDTH-1:0]        PORT_A_WR_DATA,
    output [PORT_A_WIDTH-1:0]        PORT_A_RD_DATA,
    input  [PORT_A_WR_EN_WIDTH-1:0]  PORT_A_WR_EN
);

    localparam ADDR_BITS_DERIVED =
        (PORT_A_WIDTH > 64) ? 0 :
        (PORT_A_WIDTH > 32) ? 9 :
        (PORT_A_WIDTH > 16) ? 10 :
        (PORT_A_WIDTH > 8)  ? 11 :
        (PORT_A_WIDTH > 4)  ? 12 :
        (PORT_A_WIDTH > 2)  ? 13 :
        (PORT_A_WIDTH > 1)  ? 14 : 15;
    localparam ADDR_BITS =
        (ADDR_BITS_FIXED != 0) ? ADDR_BITS_FIXED : ADDR_BITS_DERIVED;
    localparam ADDR_BITS_SAFE = (ADDR_BITS == 0) ? 1 : ADDR_BITS;

    wire _TECHMAP_FAIL_ = (PORT_A_WIDTH > 64) || (ADDR_BITS == 0);

    wire [ADDR_BITS_SAFE-1:0] addrCore =
        PORT_A_ADDR[(PORT_A_ABITS-1)-:ADDR_BITS_SAFE];
    wire [14:0] addrPad = {{(15 - ADDR_BITS_SAFE){1'b0}}, addrCore};

    generate
        if (PORT_A_WIDTH > 1) begin
            \$__VTR_SP_RAM__ #(
                .PORT_A_WIDTH(PORT_A_WIDTH - 1),
                .PORT_A_ABITS(PORT_A_ABITS),
                .PORT_A_WR_EN_WIDTH(PORT_A_WR_EN_WIDTH),
                .PORT_A_RD_USED(PORT_A_RD_USED),
                .PORT_A_WR_USED(PORT_A_WR_USED),
                .ADDR_BITS_FIXED(ADDR_BITS_SAFE)
            ) _TECHMAP_REPLACE_.lo (
                .PORT_A_CLK(PORT_A_CLK),
                .PORT_A_ADDR(PORT_A_ADDR),
                .PORT_A_WR_DATA(PORT_A_WR_DATA[PORT_A_WIDTH-2:0]),
                .PORT_A_RD_DATA(PORT_A_RD_DATA[PORT_A_WIDTH-2:0]),
                .PORT_A_WR_EN(PORT_A_WR_EN)
            );
            vtr_sp_ram_bit _TECHMAP_REPLACE_.hi (
                .clk (PORT_A_CLK),
                .addr(addrPad),
                .data(PORT_A_WR_DATA[PORT_A_WIDTH-1]),
                .we  (PORT_A_WR_EN[0]),
                .out (PORT_A_RD_DATA[PORT_A_WIDTH-1])
            );
        end else begin
            vtr_sp_ram_bit _TECHMAP_REPLACE_ (
                .clk (PORT_A_CLK),
                .addr(addrPad),
                .data(PORT_A_WR_DATA[0]),
                .we  (PORT_A_WR_EN[0]),
                .out (PORT_A_RD_DATA[0])
            );
        end
    endgenerate

endmodule


// ============================================================================
// dual-port ram  both ports share CLK_C
// ============================================================================
// memory_libmap wires PORT_A_CLK and PORT_B_CLK as well as the shared CLK_C
// so all three have to be declared even though the physical cell only uses
// one clock.
module \$__VTR_DP_RAM__ # (
    parameter PORT_A_WIDTH       = 1,
    parameter PORT_A_ABITS       = 15,
    parameter PORT_A_WR_EN_WIDTH = 1,
    parameter PORT_A_RD_USED     = 1,
    parameter PORT_A_WR_USED     = 1,
    parameter PORT_B_WIDTH       = 1,
    parameter PORT_B_ABITS       = 15,
    parameter PORT_B_WR_EN_WIDTH = 1,
    parameter PORT_B_RD_USED     = 1,
    parameter PORT_B_WR_USED     = 1,
    parameter ADDR_BITS_FIXED    = 0
) (
    input                            CLK_C,
    input                            PORT_A_CLK,
    input                            PORT_B_CLK,
    input  [PORT_A_ABITS-1:0]        PORT_A_ADDR,
    input  [PORT_A_WIDTH-1:0]        PORT_A_WR_DATA,
    output [PORT_A_WIDTH-1:0]        PORT_A_RD_DATA,
    input  [PORT_A_WR_EN_WIDTH-1:0]  PORT_A_WR_EN,
    input  [PORT_B_ABITS-1:0]        PORT_B_ADDR,
    input  [PORT_B_WIDTH-1:0]        PORT_B_WR_DATA,
    output [PORT_B_WIDTH-1:0]        PORT_B_RD_DATA,
    input  [PORT_B_WR_EN_WIDTH-1:0]  PORT_B_WR_EN
);

    wire clkAssigned = CLK_C;

    localparam ADDR_BITS_DERIVED =
        (PORT_A_WIDTH > 32) ? 0 :
        (PORT_A_WIDTH > 16) ? 10 :
        (PORT_A_WIDTH > 8)  ? 11 :
        (PORT_A_WIDTH > 4)  ? 11 :
        (PORT_A_WIDTH > 2)  ? 13 :
        (PORT_A_WIDTH > 1)  ? 14 : 15;
    localparam ADDR_BITS =
        (ADDR_BITS_FIXED != 0) ? ADDR_BITS_FIXED : ADDR_BITS_DERIVED;
    localparam ADDR_BITS_SAFE = (ADDR_BITS == 0) ? 1 : ADDR_BITS;

    wire _TECHMAP_FAIL_ = (PORT_A_WIDTH > 32) || (ADDR_BITS == 0);

    wire [ADDR_BITS_SAFE-1:0] addrCoreA =
        PORT_A_ADDR[(PORT_A_ABITS-1)-:ADDR_BITS_SAFE];
    wire [ADDR_BITS_SAFE-1:0] addrCoreB =
        PORT_B_ADDR[(PORT_B_ABITS-1)-:ADDR_BITS_SAFE];
    wire [14:0] addrPadA = {{(15 - ADDR_BITS_SAFE){1'b0}}, addrCoreA};
    wire [14:0] addrPadB = {{(15 - ADDR_BITS_SAFE){1'b0}}, addrCoreB};

    generate
        if (PORT_A_WIDTH > 1) begin
            \$__VTR_DP_RAM__ #(
                .PORT_A_WIDTH(PORT_A_WIDTH - 1),
                .PORT_A_ABITS(PORT_A_ABITS),
                .PORT_A_WR_EN_WIDTH(PORT_A_WR_EN_WIDTH),
                .PORT_A_RD_USED(PORT_A_RD_USED),
                .PORT_A_WR_USED(PORT_A_WR_USED),
                .PORT_B_WIDTH(PORT_B_WIDTH - 1),
                .PORT_B_ABITS(PORT_B_ABITS),
                .PORT_B_WR_EN_WIDTH(PORT_B_WR_EN_WIDTH),
                .PORT_B_RD_USED(PORT_B_RD_USED),
                .PORT_B_WR_USED(PORT_B_WR_USED),
                .ADDR_BITS_FIXED(ADDR_BITS_SAFE)
            ) _TECHMAP_REPLACE_.lo (
                .CLK_C(CLK_C),
                .PORT_A_CLK(PORT_A_CLK),
                .PORT_B_CLK(PORT_B_CLK),
                .PORT_A_ADDR(PORT_A_ADDR),
                .PORT_A_WR_DATA(PORT_A_WR_DATA[PORT_A_WIDTH-2:0]),
                .PORT_A_RD_DATA(PORT_A_RD_DATA[PORT_A_WIDTH-2:0]),
                .PORT_A_WR_EN(PORT_A_WR_EN),
                .PORT_B_ADDR(PORT_B_ADDR),
                .PORT_B_WR_DATA(PORT_B_WR_DATA[PORT_B_WIDTH-2:0]),
                .PORT_B_RD_DATA(PORT_B_RD_DATA[PORT_B_WIDTH-2:0]),
                .PORT_B_WR_EN(PORT_B_WR_EN)
            );
            vtr_dp_ram_bit _TECHMAP_REPLACE_.hi (
                .clk  (clkAssigned),
                .addr1(addrPadA),
                .data1(PORT_A_WR_DATA[PORT_A_WIDTH-1]),
                .we1  (PORT_A_WR_EN[0]),
                .out1 (PORT_A_RD_DATA[PORT_A_WIDTH-1]),
                .addr2(addrPadB),
                .data2(PORT_B_WR_DATA[PORT_B_WIDTH-1]),
                .we2  (PORT_B_WR_EN[0]),
                .out2 (PORT_B_RD_DATA[PORT_B_WIDTH-1])
            );
        end else begin
            vtr_dp_ram_bit _TECHMAP_REPLACE_ (
                .clk  (clkAssigned),
                .addr1(addrPadA),
                .data1(PORT_A_WR_DATA[0]),
                .we1  (PORT_A_WR_EN[0]),
                .out1 (PORT_A_RD_DATA[0]),
                .addr2(addrPadB),
                .data2(PORT_B_WR_DATA[0]),
                .we2  (PORT_B_WR_EN[0]),
                .out2 (PORT_B_RD_DATA[0])
            );
        end
    endgenerate

endmodule
