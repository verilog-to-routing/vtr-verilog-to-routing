// DEFINES
`define BITS 2         // Bit width of the operands

module 	bm_dag3_lpm_log_mod(clock, 
		reset_n, 
		first, 
		sceond,
		third, 
		fourth, 
		out0,
		out1);

// SIGNAL DECLARATIONS
input	clock;
input 	reset_n;

input [`BITS-1:0] first;
input [`BITS-1:0] sceond;
input third;
input fourth;

output [`BITS-1:0] out0;
output  out1;

wire [`BITS-1:0]    out0;
wire     out1;

wire [`BITS-1:0] temp_a;
wire [`BITS-1:0] temp_b;
wire temp_c;
wire temp_d;

a top_a(clock, first, sceond, temp_a);
b top_b(clock, first, sceond, temp_b);
c top_c(clock, third, fourth, temp_c);
d top_d(clock, third, fourth, temp_d);

assign out0 = temp_a & temp_b;
assign out1 = temp_c + temp_d;

endmodule

/*---------------------------------------------------------*/
module a(clock,
		fifth,
		sixth,
		out2);

input	clock;
input [`BITS-1:0] fifth;
input [`BITS-1:0] sixth;
output [`BITS-1:0]    out2;
reg [`BITS-1:0]    out2;
wire [`BITS-1:0] temp1;

d mya_d(clock, fifth[0], sixth[0], temp1[0]);
d mya_d2(clock, fifth[0], sixth[1], temp1[1]);

always @(posedge clock)
begin
	out2 <= fifth & sixth & temp1;
end

endmodule

/*---------------------------------------------------------*/
module b(clock,
		seventh,
		eight,
		out3);

input	clock;
input [`BITS-1:0] seventh;
input [`BITS-1:0] eight;
reg [`BITS-1:0] temp2;
wire temp3;
output [`BITS-1:0]    out3;
reg [`BITS-1:0] out3;

c myb_c(clock, seventh[0], eight[0], temp3);

always @(posedge clock)
begin
	temp2 <= seventh | eight ^ temp3;
	out3 <= seventh ^ temp2;
end

endmodule

/*---------------------------------------------------------*/
module 	c(clock, 
		ninth, 
		tenth, 
		out4);

// SIGNAL DECLARATIONS
input	clock;
input ninth;
input tenth;
output  out4;
wire     out4;
wire temp4;
wire temp5;

d myc_d(clock, ninth, tenth, temp5);

assign out4 = temp4 ^ tenth;
assign temp4 = ninth - temp5;

endmodule

/*---------------------------------------------------------*/
module 	d(clock, 
		eleventh, 
		twelfth, 
		out5);

// SIGNAL DECLARATIONS
input	clock;
input eleventh;
input twelfth;
output  out5;
reg     out5;
reg temp6;

always @(posedge clock)
begin
	temp6 <= eleventh ^ twelfth;
	out5 <= temp6 | twelfth;
end

endmodule


