`define WIDTH 4

module top_module (clk, rst, x, y, q);

    input   clk, rst;
    input   [`WIDTH-1:0] x; 
    input   [`WIDTH-1:0] y;
    output  [`WIDTH-1:0] q;

    always @(posedge clk)
    begin
        my_task(.a(rst ? x : x >> 1), .b(y - 2), .so(q));
    end

    task my_task(
        input  [`WIDTH-1:0] a,
        input  [`WIDTH-1:0] b,
        output [`WIDTH-1:0] so
        );

        so <= a + b;  

    endtask


endmodule