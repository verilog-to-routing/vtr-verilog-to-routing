/* ********************************************************************************* */
/*										     */
/*        		Main module for biquad filter			             */
/*                                                                                   */
/*  Author:  Chuck Cox (chuck100@home.com)                                           */
/*                                                                                   */
/* This filter core uses a wishbone interface for compatibility with other cores:    */
/*										     */
/* Wishbone general description:  16x5 register file				     */
/* Supported cycles:  Slave Read/write, block read/write, RMW			     */
/* Data port size:  16 bit							     */
/* Data port granularity: 16 bit						     */
/* Data port maximum operand size:  16 bit					     */
/*                                                                                   */
/*      Addr	register							     */
/*      ----    --------							     */
/*	0x0	Filter coefficient a11						     */
/*	0x1	Filter coefficient a12						     */
/*      0x2	Filter coefficient b10						     */
/*	0x3	Filter coefficient b11						     */
/*	0x4	Filter coefficient b12						     */
/*                                                                                   */
/*  Filter coefficients need to be written as 16 bit twos complement fractional	     */
/*  numbers.  For example:  0100_0000_0000_0001 = 2^-1 + 2^-15 = .500030517578125    */
/*										     */
/*  The equation for the filter implemented with this core is                        */
/*  y[n] = b10 * x[n] + b11 * x[n-1] + b12 * x[n-2] + a11 * y[n-1] + a12 * y[n-2]    */
/* 										     */
/*  This biquad filter is parameterized.  If a filter with coefficients less than    */
/*  16 bits in length is selected via parameters then the most significant bits of   */
/*  the value written to the filter coefficient register shall be used (ie	     */
/*  coefficients shall be truncated as required by parameter value COEFWIDTH).	     */
/*										     */
/*  See comments in biquad module for more details on filtering algorthm.	     */
/* ********************************************************************************* */
`define ACCUM	2
`define COEFWIDTH	8
`define DATAWIDTH	8

/* Must be less than or equal to COEFWIDTH-2 */


module iir1
	(
	clk_i,		/* Wishbone clk */
	rst_i,		/* Wishbone asynchronous active high reset */
	we_i,		/* Wishbone write enable */
	stb_i,		/* Wishbone strobe */
	ack_o,		/* Wishbone ack */
	dat_i,		/* Wishbone input data */
	dat_o,		/* Wishbone output data */
	adr_i,		/* Wishbone address bus */
	dspclk,		/* DSP processing clock */
	nreset,		/* active low asynchronous reset for filter block */
	x,		/* input data for filter */
	valid,		/* data valid input */
	y		/* filter output data */
	);

input			clk_i;
input			rst_i;
input			we_i;
input			stb_i;
output			ack_o;
input	[15:0]		dat_i;
output	[15:0]		dat_o;
input	[2:0]		adr_i;
input			dspclk;
input			nreset;
input	[`DATAWIDTH-1:0]	x;
input			valid;
output	[`DATAWIDTH-1:0]	y;


wire	[15:0]	a11;
wire	[15:0]	a12;
wire	[15:0]	b10;
wire	[15:0]	b11;
wire	[15:0]	b12;

/* Filter module */
biquad biquadi
	(
	.clk(dspclk),				/* clock */
	.nreset(nreset),			/* active low reset */
	.x(x),					/* data input */
	.valid(valid),				/* input data valid */
	.a11(a11[15:16-`COEFWIDTH]),		/* filter pole coefficient */
	.a12(a12[15:16-`COEFWIDTH]),		/* filter pole coefficient */
	.b10(b10[15:16-`COEFWIDTH]),		/* filter zero coefficient */
	.b11(b11[15:16-`COEFWIDTH]),		/* filter zero coefficient */
	.b12(b12[15:16-`COEFWIDTH]),		/* filter zero coefficient */
	.yout(y)				/* filter output */
	);

/* Wishbone interface module */
coefio coefioi
	(
	.clk_i(clk_i),
	.rst_i(rst_i),
	.we_i(we_i),	
	.stb_i(stb_i),
	.ack_o(ack_o),
	.dat_i(dat_i),
	.dat_o(dat_o),
	.adr_i(adr_i),
	.a11(a11),
	.a12(a12),
	.b10(b10),
	.b11(b11),
	.b12(b12)
	);



endmodule
/* ********************************************************************************* */
/*                      Bi-quad IIR filter section                                   */
/*                                                                                   */
/*  Author:  Chuck Cox (chuck100@home.com)                                           */
/*                                                                                   */
/*  This module implements an IIR filter section with 2 poles and                    */
/*  2 zeros.  The filter coefficients are inputs to this module.                     */
/*  Multiple bi-quad filter sections can be connected together to                    */
/*  create higher order filters.  The filter uses a signed fractional                */
/*  2's complement binary numbering system.  For example with 3 bits:                */
/*                                                                                   */
/*  000 = 0     001 = 0.25     010 = 0.5     011 = 0.75                              */
/*  100 = 0     101 = -.75     110 = -.5     111 = -.25                              */
/*                                                                                   */
/*  The equation for the filter implemented with this module is                      */
/*  y[n] = b10 * x[n] + b11 * x[n-1] + b12 * x[n-2] + a11 * y[n-1] + a12 * y[n-2]    */
/*                                                                                   */
/*  Multipliers are implemented as calls to seperate modules to allow easy           */
/*  conversion to hard or firm macros.  Multicycle multipliers can be used           */
/*  by not making input data valid on every clock.  For a 4 clock multiplier         */
/*  data will need to be valid only once every 4 clocks.                             */
/*                                                                                   */
/*                                                                                   */
/* ********************************************************************************* */


module biquad
	(
	clk,				/* clock */
	nreset,			/* active low reset */
	x,				/* data input */
	valid,			/* input data valid */
	a11,				/* filter pole coefficient */
	a12,				/* filter pole coefficient */
	b10,				/* filter zero coefficient */
	b11,				/* filter zero coefficient */
	b12,				/* filter zero coefficient */
	yout				/* filter output */
	);

// Acumulator width is (`DATAWIDTH + `ACCUM) bits.

input 								clk;
input 								nreset;
input	[`DATAWIDTH-1:0]				x;
input								valid;
input	[`COEFWIDTH-1:0]				a11;
input	[`COEFWIDTH-1:0]				a12;
input	[`COEFWIDTH-1:0]				b10;
input	[`COEFWIDTH-1:0]				b11;
input	[`COEFWIDTH-1:0]				b12;
output	[`DATAWIDTH-1:0]				yout;

reg		[`DATAWIDTH-1:0]				xvalid;
reg		[`DATAWIDTH-1:0]				xm1;
reg		[`DATAWIDTH-1:0]				xm2;
reg		[`DATAWIDTH-1:0]				xm3;
reg		[`DATAWIDTH-1:0]				xm4;
reg		[`DATAWIDTH-1:0]				xm5;
reg		[`DATAWIDTH+3+`ACCUM:0]			sumb10reg;
reg		[`DATAWIDTH+3+`ACCUM:0]			sumb11reg;
reg		[`DATAWIDTH+3+`ACCUM:0]			sumb12reg;
reg		[`DATAWIDTH+3+`ACCUM:0]			suma12reg;
reg		[`DATAWIDTH+3:0]				y;
reg		[`DATAWIDTH-1:0]				yout;

wire		[`COEFWIDTH-1:0]				sa11;
wire		[`COEFWIDTH-1:0]				sa12;
wire		[`COEFWIDTH-1:0]				sb10;
wire		[`COEFWIDTH-1:0]				sb11;
wire		[`COEFWIDTH-1:0]				sb12;
wire		[`DATAWIDTH-2:0]				tempsum;
wire		[`DATAWIDTH+3+`ACCUM:0]			tempsum2;
wire		[`DATAWIDTH + `COEFWIDTH - 3:0]		mb10out;
wire		[`DATAWIDTH+3+`ACCUM:0]			sumb10;
wire		[`DATAWIDTH + `COEFWIDTH - 3:0]		mb11out;
wire		[`DATAWIDTH+3+`ACCUM:0]			sumb11;
wire		[`DATAWIDTH + `COEFWIDTH - 3:0]		mb12out;
wire		[`DATAWIDTH+3+`ACCUM:0]			sumb12;
wire		[`DATAWIDTH + `COEFWIDTH + 1:0] 	ma12out;
wire		[`DATAWIDTH+3+`ACCUM:0]			suma12;
wire		[`DATAWIDTH + `COEFWIDTH + 1:0] 	ma11out;
wire		[`DATAWIDTH+3+`ACCUM:0]			suma11;
wire		[`DATAWIDTH+2:0]				sy;
wire		[`DATAWIDTH-1:0]				olimit;


/* Two's complement of coefficients */
assign sa11 = a11[`COEFWIDTH-1] ? (-a11) : a11;
assign sa12 = a12[`COEFWIDTH-1] ? (-a12) : a12;
assign sb10 = b10[`COEFWIDTH-1] ? (-b10) : b10;
assign sb11 = b11[`COEFWIDTH-1] ? (-b11) : b11;
assign sb12 = b12[`COEFWIDTH-1] ? (-b12) : b12;
/*
assign sa11 = a11[`COEFWIDTH-1] ? (~a11 + 1) : a11;
assign sa12 = a12[`COEFWIDTH-1] ? (~a12 + 1) : a12;
assign sb10 = b10[`COEFWIDTH-1] ? (~b10 + 1) : b10;
assign sb11 = b11[`COEFWIDTH-1] ? (~b11 + 1) : b11;
assign sb12 = b12[`COEFWIDTH-1] ? (~b12 + 1) : b12;
*/

assign tempsum = -xvalid[`DATAWIDTH-2:0];
//assign tempsum = ~xvalid[`DATAWIDTH-2:0] + 1;
assign tempsum2 = -{4'b0000,mb10out[`COEFWIDTH+`DATAWIDTH-3:`COEFWIDTH-2-`ACCUM]};
//assign tempsum2 = ~{4'b0000,mb10out[`COEFWIDTH+`DATAWIDTH-3:`COEFWIDTH-2-`ACCUM]} + 1;

wire tmp_xvalid;
wire tmp_xm1;
wire tmp_xm2;
wire tmp_xm3;
wire tmp_xm4;
wire tmp_xm5;

assign xvalid = (~nreset)? 0: tmp_xvalid;
assign xm1 = (~nreset)? 0: tmp_xm1;
assign xm2 = (~nreset)? 0: tmp_xm2;
assign xm3 = (~nreset)? 0: tmp_xm3;
assign xm4 = (~nreset)? 0: tmp_xm4;
assign xm5 = (~nreset)? 0: tmp_xm5;

/* clock data into pipeline.  Divide by 8.  Convert to sign magnitude. */
always @(posedge clk)
begin
	if(valid)
	begin
		tmp_xvalid <= x;
		tmp_xm1 <= (xvalid[`DATAWIDTH-1] ? ({xvalid[`DATAWIDTH-1],tempsum}) : {xvalid});
		tmp_xm2 <= xm1;
		tmp_xm3 <= xm2;
		tmp_xm4 <= xm3;
		tmp_xm5 <= xm4;
  	end
end

/* Multiply input by filter coefficient b10 */
multb multb10(.a(sb10[`COEFWIDTH-2:0]),.b(xm1[`DATAWIDTH-2:0]),.r(mb10out));

assign sumb10 = (b10[`COEFWIDTH-1] ^ xm1[`DATAWIDTH-1]) ? (tempsum2) : ({4'b0000,mb10out[`COEFWIDTH+`DATAWIDTH-3:`COEFWIDTH-2-`ACCUM]});

/* Multiply input by filter coefficient b11 */
multb multb11(.a(sb11[`COEFWIDTH-2:0]),.b(xm3[`DATAWIDTH-2:0]),.r(mb11out));

/* Divide by two and add or subtract */
assign sumb11 = (b11[`COEFWIDTH-1] ^ xm3[`DATAWIDTH-1]) ? (sumb10reg - {4'b0000,mb11out[`COEFWIDTH+`DATAWIDTH-3:`COEFWIDTH-2-`ACCUM]}) : 
     (sumb10reg + {4'b0000,mb11out[`COEFWIDTH+`DATAWIDTH-3:`COEFWIDTH-2-`ACCUM]});

/* Multiply input by filter coefficient b12 */
multb multb12(.a(sb12[`COEFWIDTH-2:0]),.b(xm5[`DATAWIDTH-2:0]),.r(mb12out));

assign sumb12 = (b12[`COEFWIDTH-1] ^ xm5[`DATAWIDTH-1]) ? (sumb11reg - {4'b0000,mb12out[`COEFWIDTH+`DATAWIDTH-3:`COEFWIDTH-2-`ACCUM]}) : 
     (sumb11reg + {4'b0000,mb12out[`COEFWIDTH+`DATAWIDTH-3:`COEFWIDTH-2-`ACCUM]});

/* Twos complement of output for feedback */
//assign sy = y[`DATAWIDTH+3] ? (-y) : y;
assign sy = y[`DATAWIDTH+3] ? (~y + 1) : y;

/* Multiply output by filter coefficient a12 */
multa multa12(.a(sa12[`COEFWIDTH-2:0]),.b(sy[`DATAWIDTH+2:0]),.r(ma12out));

assign suma12 = (a12[`COEFWIDTH-1] ^ y[`DATAWIDTH+3]) ? (sumb12reg - {1'b0,ma12out[`COEFWIDTH+`DATAWIDTH:`COEFWIDTH-2-`ACCUM]}) : 
     (sumb12reg + {1'b0,ma12out[`COEFWIDTH+`DATAWIDTH:`COEFWIDTH-2-`ACCUM]});

/* Multiply output by filter coefficient a11 */
multa multa11(.a(sa11[`COEFWIDTH-2:0]),.b(sy[`DATAWIDTH+2:0]),.r(ma11out));

assign suma11 = (a11[`COEFWIDTH-1] ^ y[`DATAWIDTH+3]) ? (suma12reg - {1'b0,ma11out[`COEFWIDTH+`DATAWIDTH:`COEFWIDTH-2-`ACCUM]}) : 
     (suma12reg + {1'b0,ma11out[`COEFWIDTH+`DATAWIDTH:`COEFWIDTH-2-`ACCUM]});

assign olimit = {y[`DATAWIDTH+3],~y[`DATAWIDTH+3],~y[`DATAWIDTH+3],~y[`DATAWIDTH+3],~y[`DATAWIDTH+3],
				~y[`DATAWIDTH+3],~y[`DATAWIDTH+3],~y[`DATAWIDTH+3]};


wire tmp_sumb10reg;
wire tmp_sumb11reg;
wire tmp_sumb12reg;
wire tmp_suma12reg;
wire tmp_y;
wire tmp_yout;

assign sumb10reg = (~nreset)? 0 : tmp_sumb10reg;
assign sumb11reg = (~nreset)? 0 : tmp_sumb11reg;
assign sumb12reg = (~nreset)? 0 : tmp_sumb12reg;
assign suma12reg = (~nreset)? 0 : tmp_suma12reg;
assign y = (~nreset)? 0 : tmp_y;
assign yout = (~nreset)? 0 : tmp_yout;


/* State registers */
always @(posedge clk)
begin
	if(valid)
	begin
		tmp_sumb10reg <= sumb10;
		tmp_sumb11reg <= sumb11;
		tmp_sumb12reg <= sumb12;
		tmp_suma12reg <= suma12;
		tmp_y <= suma11[`DATAWIDTH+3+`ACCUM:`ACCUM];
		tmp_yout <= ( (&y[`DATAWIDTH+3:`DATAWIDTH-1]) | (~|y[`DATAWIDTH+3:`DATAWIDTH-1])  ) ? (y[`DATAWIDTH-1:0]) : (olimit) ;
	end
end


endmodule
/* ********************************************************************************* */
/*        Wishbone compatible IO module for read/write of filter coefficients        */
/*                                                                                   */
/*  Author:  Chuck Cox (chuck100@home.com)                                           */
/*                                                                                   */
/* Wishbone data:								     */
/* General description:  16x5 register file					     */
/* Supported cycles:  Slave Read/write, block read/write, RMW			     */
/* Data port size:  16 bit							     */
/* Data port granularity: 16 bit						     */
/* Data port maximum operand size:  16 bit					     */
/*                                                                                   */
/*      Addr	register							     */
/*	0x0	a11								     */
/*	0x1	a12								     */
/*      0x2	b10								     */
/*	0x3	b11								     */
/*	0x4	b12								     */
/*                                                                                   */
/*  Filter coefficients need to be written as 16 bit twos complement fractional	     */
/*  numbers.  For example:  0100_0000_0000_0001 = 2^-1 + 2^-15 = .500030517578125    */
/* 										     */
/*  The biquad filter module is parameterized.  If a filter with coefficients less   */
/*  than 16 bits in length is selected then the most significant bits shall be used. */
/* ********************************************************************************* */


module coefio
	(
	clk_i,
	rst_i,
	we_i,	
	stb_i,
	ack_o,
	dat_i,
	dat_o,
	adr_i,
	a11,
	a12,
	b10,
	b11,
	b12
	);



input 		clk_i;
input 		rst_i;
input		we_i;
input		stb_i;
output		ack_o;
input	[15:0]	dat_i;
output	[15:0]	dat_o;
input	[2:0]	adr_i;
output	[15:0]	a11;
output	[15:0]	a12;
output	[15:0]	b10;
output	[15:0]	b11;
output	[15:0]	b12;


reg	[15:0]	a11;
reg	[15:0]	a12;
reg	[15:0]	b10;
reg	[15:0]	b11;
reg	[15:0]	b12;

wire		ack_o;
wire		sel_a11;
wire		sel_a12;
wire		sel_b10;
wire		sel_b11;
wire		sel_b12;

assign sel_a11 = (adr_i == 3'b000);
assign sel_a12 = (adr_i == 3'b001);
assign sel_b10 = (adr_i == 3'b010);
assign sel_b11 = (adr_i == 3'b011);
assign sel_b12 = (adr_i == 3'b100);

assign ack_o = stb_i;

wire tmp_a11;
wire tmp_a12;
wire tmp_b10;
wire tmp_b11;
wire tmp_b12;

assign a11 = (rst_i)? 15'b11111111: tmp_a11;
assign a12 = (rst_i)? 15'b11111: tmp_a12;
assign b10 = (rst_i)? 15'b1111111: tmp_b10;
assign b11 = (rst_i)? 15'b11: tmp_b11;
assign b12 = (rst_i)? 15'b11111111: tmp_b12;

assign dat_o = 	((sel_a11) ? (a11) : 
				((sel_a12) ? (a12) : 
				((sel_b10) ? (b10) : 
				((sel_b11) ? (b11) : 
				((sel_b12) ? (b12) : 
				(16'h0000))))));

always @(posedge clk_i)
begin
	if(stb_i & we_i & sel_a11)
	begin
		tmp_a11 <= dat_i;
		tmp_a12 <= dat_i;
		tmp_b10 <= dat_i;
		tmp_b11 <= dat_i;
		tmp_b12 <= dat_i;
	end
end

endmodule
/* ********************************************************************************* */
/*                      multiplier place holder                                      */
/*                                                                                   */
/*  Author:  Chuck Cox (chuck100@home.com)                                           */
/*                                                                                   */
/*                                                                                   */
/*                                                                                   */
/* ********************************************************************************* */


module multa
	(
//	clk,
//	nreset,
	a,			/* data input */
	b,			/* input data valid */
	r			/* filter pole coefficient */
	);


//input 					clk;
//input 					nreset;
input	[`COEFWIDTH-2:0]			a;
input	[`DATAWIDTH+2:0]			b;
output	[`DATAWIDTH + `COEFWIDTH + 1:0]	r;


assign r = a*b;


endmodule
/* ********************************************************************************* */
/*                      multiplier place holder                                      */
/*                                                                                   */
/*  Author:  Chuck Cox (chuck100@home.com)                                           */
/*                                                                                   */
/*                                                                                   */
/*                                                                                   */
/* ********************************************************************************* */


module multb
	(
	a,			/* data input */
	b,			/* input data valid */
	r			/* filter pole coefficient */
	);


input	[`COEFWIDTH-2:0]			a;
input	[`DATAWIDTH-2:0]			b;
output	[`DATAWIDTH + `COEFWIDTH - 3:0]	r;


assign r = a*b;


endmodule
