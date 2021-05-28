module top_module(
    a,
    c,
    clk,
    rst
);

input [7:0] a;
input clk, rst;
output [7:0] c;
integer i;

always @ ( posedge clk )
begin
    case(rst)
        0: for(i = 0; i<8; i = i + 1)
           begin 
               c[i] = a[i];
           end
        1: c = 0;
    endcase
end
endmodule


/* 
  [ODIN_HARD 8_bit_for_pass_through]
  -V ~/Desktop/yosys+odin/syntax/with_inout_files/8_bit_for_pass_through/8_bit_for_pass_through.v -a ~/Desktop/yosys+odin/arch/k6_frac_N10_frac_chain_depop50_mem32K_40nm.xml -o ~/Desktop/yosys+odin/syntax/with_inout_files/8_bit_for_pass_through/odin_hard_8_bit_for_pass_through.blif -g 100
  
  [YOSYS_BLIF_HARD_8_bit_for_pass_through]
  -b ~/Desktop/yosys+odin/syntax/with_inout_files/8_bit_for_pass_through/yosys_8_bit_for_pass_through.blif -a ~/Desktop/yosys+odin/arch/k6_frac_N10_frac_chain_depop50_mem32K_40nm.xml -o ~/Desktop/yosys+odin/syntax/with_inout_files/8_bit_for_pass_through/yosys+odin_hard_8_bit_for_pass_through.blif --coarsen -t ~/Desktop/yosys+odin/syntax/with_inout_files/8_bit_for_pass_through/8_bit_for_pass_through_input -T ~/Desktop/yosys+odin/syntax/with_inout_files/8_bit_for_pass_through/8_bit_for_pass_through_output
  
  [ODIN_SOFT_8_bit_for_pass_through]
  -V ~/Desktop/yosys+odin/syntax/with_inout_files/8_bit_for_pass_through/8_bit_for_pass_through.v -o ~/Desktop/yosys+odin/syntax/with_inout_files/8_bit_for_pass_through/odin_soft_8_bit_for_pass_through.blif -t ~/Desktop/yosys+odin/syntax/with_inout_files/8_bit_for_pass_through/8_bit_for_pass_through_input -T ~/Desktop/yosys+odin/syntax/with_inout_files/8_bit_for_pass_through/8_bit_for_pass_through_output
  
  [YOSYS_BLIF_SOFT_8_bit_for_pass_through]
  -b ~/Desktop/yosys+odin/syntax/with_inout_files/8_bit_for_pass_through/yosys_8_bit_for_pass_through.blif -o ~/Desktop/yosys+odin/syntax/with_inout_files/8_bit_for_pass_through/yosys+odin_soft_8_bit_for_pass_through.blif --coarsen -t ~/Desktop/yosys+odin/syntax/with_inout_files/8_bit_for_pass_through/8_bit_for_pass_through_input -T ~/Desktop/yosys+odin/syntax/with_inout_files/8_bit_for_pass_through/8_bit_for_pass_through_output
  
  [YOSYS_CMD]
  
  arg 1: /syntax/with_inout_files/8_bit_for_pass_through
  arg 2: 8_bit_for_pass_through
  
  read_verilog ~/Desktop/yosys+odin/syntax/with_inout_files/8_bit_for_pass_through/8_bit_for_pass_through.v; proc; opt; write_blif -top top_module ~/Desktop/yosys+odin/syntax/with_inout_files/8_bit_for_pass_through/yosys_8_bit_for_pass_through.blif;
*/
