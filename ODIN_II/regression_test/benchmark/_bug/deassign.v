module (Clk, Reset, CntOut);

input Clk;
input Reset;
output CntOut;

reg Cnt;

always @(posedge Clk)
  Cnt = Cnt + 1;

always @(Reset)
begin
  if (Reset)
    assign Cnt = 0;       // prevents counting until Reset is 0
  else
    deassign Cnt;         // resume counting on next posedge Clk
end

assign CntOut = Cnt;

endmodule

