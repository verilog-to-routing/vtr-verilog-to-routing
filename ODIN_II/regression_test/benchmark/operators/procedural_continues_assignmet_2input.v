module TEST (r,d, clk, q);
input r,d, clk;

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
    q = d;
end

endmodule
