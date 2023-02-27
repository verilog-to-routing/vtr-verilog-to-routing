module pmux (in_a, in_b, in_s, out_y);
    
    parameter WIDTH = 10;
    parameter S_WIDTH = 20;
    
    input [WIDTH-1:0] in_a;
    input [WIDTH*S_WIDTH-1:0] in_b;
    input [S_WIDTH-1:0] in_s;
    output reg [WIDTH-1:0] out_y;
    
    integer i;
    reg found_active_sel_bit;
    
    always @* begin
        out_y = in_a;
        found_active_sel_bit = 0;
        for (i = 0; i < S_WIDTH; i = i+1)
            if (in_s[i]) begin
                out_y = found_active_sel_bit ? 'bx : in_b >> (WIDTH*i);
                found_active_sel_bit = 1;
            end
    end
    
endmodule
