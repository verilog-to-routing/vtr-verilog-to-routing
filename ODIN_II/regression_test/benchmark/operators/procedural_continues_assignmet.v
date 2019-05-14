module TEST (r, clk, q);
input r, clk;
output q;
reg q;

always @ (r) 
begin
    if (r) 
        assign q = 0;
    else 
        deassign q;
end

always @ (posedge clk) 
begin
    q = 1;
end

endmodule
