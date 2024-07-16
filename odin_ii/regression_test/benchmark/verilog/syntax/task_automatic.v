module simple_task (
    clk,
    reset,
    a,
    b,    
    out_1, 
    clk_out
    );

    input   clk;
    input   reset;
    input   [1:0] a,b;

    output  [2:0] out_1;
    output  clk_out;

    assign clk_out = clk;

always @(posedge clk)
begin
    my_task(a,b,reset,out_1);
end

task automatic my_task;

    input [1:0] x,y;
    input rst;
    output [2:0] z;

    case(rst)
        1'b0:       z <= x + y;
        default:    z <= 1'b0;
    endcase
    
endtask

endmodule
