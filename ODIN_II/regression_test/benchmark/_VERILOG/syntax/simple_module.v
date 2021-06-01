module simple_module (
    clk,
    reset,
    a,
    b,    
    out,
    clk_out
    );

    input   clk;
    input   reset;
    input   [1:0] a,b;

    output  [2:0] out;
    output  clk_out;

    assign clk_out = clk;


    xor_task    my_mod(.clk(clk),
                        .x(a),
                        .y(b),
                        .rst(reset),
                        .z(out));



endmodule

module xor_task(clk,x,y,rst,z);
input clk;
input [1:0] x,y;
input rst;
output [2:0] z;
    
always @(posedge clk)
begin
    case(rst)
        1'b0:       z <= x + y;
        default:    z <= 1'b0;
    endcase
end
    
endmodule
