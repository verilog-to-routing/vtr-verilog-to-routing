module simple_op(out);

    parameter N = 3;
    parameter WIDTH = N+2;

    output reg [WIDTH -1:0] out;
    integer i;

    always @(*) begin
        for (i = 0;i <= N;i = i+1)begin
            out[i] = 1; 
        end
    end 
endmodule 