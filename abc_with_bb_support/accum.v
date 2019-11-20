module accum (clock, reset_n, D, Q);
input clock, reset_n;
input  [3:0] D;
output [3:0] Q;
reg    [3:0] tmp; 


  always @(posedge clock)
    begin
      if (reset_n)
        tmp <= 4'b0000;
      else
        tmp <= tmp + D;
    end
assign Q = tmp;
endmodule