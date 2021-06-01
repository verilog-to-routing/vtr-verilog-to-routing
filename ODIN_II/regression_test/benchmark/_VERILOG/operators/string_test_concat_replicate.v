module top(input clk, output out, output [239:0]tmp);

parameter string_hex = 240'h48656c6c6f20776f726c6421207468697320697320612070726f6772616d;
parameter string_input = { "He", {2{"l"}}, "o world! ", "this is a program" };

always @(posedge clk)
begin
    out = ( string_hex == string_input );
    tmp = string_input;
end

endmodule