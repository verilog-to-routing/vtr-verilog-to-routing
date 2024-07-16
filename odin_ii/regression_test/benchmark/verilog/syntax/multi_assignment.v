module select_bus(busout, bus0, bus1, bus2, bus3, enable, s);
output [3:0] busout;
input [3:0] bus0, bus1, bus2, bus3;
input enable;
input [1:0] s;
wire [3:0] data; // net declaration
// assignment statement with four continuous assignments
assign data = (s == 0) ? bus0 : 4'hz;
assign data = (s == 1) ? bus1 : 4'hz;
assign data = (s == 2) ? bus2 : 4'hz;
assign data = (s == 3) ? bus3 : 4'hz;
// net declaration with continuous assignment
assign busout = enable ? data : 4'hf;
endmodule