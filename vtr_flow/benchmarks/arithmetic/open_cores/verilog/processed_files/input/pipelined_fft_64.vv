/////////////////////////////////////////////////////////////////////
////                                                             ////
////  Storage Buffer                                             ////
////                                                             ////
////  Authors: Anatoliy Sergienko, Volodya Lepeha                ////
////  Company: Unicore Systems http://unicore.co.ua              ////
////                                                             ////
////  Downloaded from: http://www.opencores.org                  ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2006-2010 Unicore Systems LTD                 ////
//// www.unicore.co.ua                                           ////
//// o.uzenkov@unicore.co.ua                                     ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
//// THIS SOFTWARE IS PROVIDED "AS IS"                           ////
//// AND ANY EXPRESSED OR IMPLIED WARRANTIES,                    ////
//// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED                  ////
//// WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT              ////
//// AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.        ////
//// IN NO EVENT SHALL THE UNICORE SYSTEMS OR ITS                ////
//// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,            ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL            ////
//// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT         ////
//// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,               ////
//// DATA, OR PROFITS; OR BUSINESS INTERRUPTION)                 ////
//// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,              ////
//// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT              ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING                 ////
//// IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,                 ////
//// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.          ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
// Design_Version       : 1.0
// File name            : BUFRAM64C1.v
// File Revision        :
// Last modification    : Sun Sep 30 20:11:56 2007
/////////////////////////////////////////////////////////////////////
// FUNCTION: FIFO - buffer with direct input order and 8-th inverse
//           output order
// FILES: BUFRAM64C1.v	- 1-st,2-nd,3-d data buffer, contains:
//        RAM2x64C_1.v - dual ported synchronous RAM, contains:
//	    RAM64.v -single ported synchronous RAM
// PROPERTIES: 1)Has the volume of 2x64 complex data
//		   2)Contains 2- port RAM and address counter
//		   3)Has 64-clock cycle period starting with the START
//               impulse and continuing forever
//		   4)Signal RDY precedes the 1-st correct datum output
//               from the buffer
/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
////                                                             ////
////  Parameter file                                             ////
////                                                             ////
////  Authors: Anatoliy Sergienko, Volodya Lepeha                ////
////  Company: Unicore Systems http://unicore.co.ua              ////
////                                                             ////
////  Downloaded from: http://www.opencores.org                  ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2006-2010 Unicore Systems LTD                 ////
//// www.unicore.co.ua                                           ////
//// o.uzenkov@unicore.co.ua                                     ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
//// THIS SOFTWARE IS PROVIDED "AS IS"                           ////
//// AND ANY EXPRESSED OR IMPLIED WARRANTIES,                    ////
//// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED                  ////
//// WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT              ////
//// AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.        ////
//// IN NO EVENT SHALL THE UNICORE SYSTEMS OR ITS                ////
//// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,            ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL            ////
//// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT         ////
//// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,               ////
//// DATA, OR PROFITS; OR BUSINESS INTERRUPTION)                 ////
//// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,              ////
//// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT              ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING                 ////
//// IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,                 ////
//// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.          ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
// Design_Version       : 1.0
// File name            : fft64_config.inc
// File Revision        :
// Last modification    : Sun Sep 30 20:11:56 2007
/////////////////////////////////////////////////////////////////////

//input data bit width
`define USFFT64paramnb parameter nb=16;

//2-nd stage data bit width
`define USFFT64paramnb2 parameter nb2=nb+2;

//twiddle factor bit width
`define USFFT64paramnw parameter nw=15;

//when is absent then FFT, when is present then IFFT
//`define USFFT64paramifft

//buffer number 2 or 3
`define USFFT64parambuffers3

// buffer type: 1 ports in RAMS else -2 ports RAMS
// NOTE: will need to uncomment RAM64 module definition
//`define USFFT64bufferports1

//Coeficient 0.707 bit width is increased
`define USFFT64bitwidth_0707_high

`define BUFRAM64C1_CUSTOM_NB (NB_VALUE) \
module BUFRAM64C1_NBNB_VALUE ( CLK ,RST ,ED ,START ,DR ,DI ,RDY ,DOR ,DOI ); \
	localparam local_nb = NB_VALUE; \
	output RDY ; \
	reg RDY ; \
	output [local_nb-1:0] DOR ; \
	wire [local_nb-1:0] DOR ; \
	output [local_nb-1:0] DOI ; \
	wire [local_nb-1:0] DOI ; \
 \
	input CLK ; \
	wire CLK ; \
	input RST ; \
	wire RST ; \
	input ED ; \
	wire ED ; \
	input START ; \
	wire START ; \
	input [local_nb-1:0] DR ; \
	wire [local_nb-1:0] DR ; \
	input [local_nb-1:0] DI ; \
	wire [local_nb-1:0] DI ; \
 \
	wire odd, we; \
	wire [5:0] addrw,addrr; \
	reg [6:0] addr; \
	reg [7:0] ct2;		//counter for the RDY signal \
 \
	always @(posedge CLK)	//   CTADDR \
		begin \
			if (RST) begin \
					addr<=6'b000000; \
					ct2<= 7'b1000001; \
				RDY<=1'b0; end \
			else if (START) begin \
					addr<=6'b000000; \
					ct2<= 6'b000000; \
				RDY<=1'b0;end \
			else if (ED)	begin \
					addr<=addr+1; \
					if (ct2!=65) begin \
						ct2<=ct2+1; \
					end \
					if (ct2==64) begin \
						RDY<=1'b1; \
					end else begin \
						RDY<=1'b0; \
					end \
				end \
		end \
 \
 \
assign	addrw=	addr[5:0]; \
assign	odd=addr[6];	   			// signal which switches the 2 parts of the buffer \
assign	addrr={addr[2 : 0], addr[5 : 3]};	  // 8-th inverse output address \
assign	we = ED; \
 \
	RAM2x64C_1	URAM(.CLK(CLK),.ED(ED),.WE(we),.ODD(odd), \
	.ADDRW(addrw),	.ADDRR(addrr), \
	.DR(DR),.DI(DI), \
	.DOR(DOR),	.DOI(DOI)); \
	defparam URAM.nb = NB_VALUE; \
 \
endmodule

`BUFRAM64C1_CUSTOM_NB(16)
`BUFRAM64C1_CUSTOM_NB(18)
`BUFRAM64C1_CUSTOM_NB(19)


/////////////////////////////////////////////////////////////////////
////                                                             ////
////  Normalization unit                                         ////
////                                                             ////
////  Authors: Anatoliy Sergienko, Volodya Lepeha                ////
////  Company: Unicore Systems http://unicore.co.ua              ////
////                                                             ////
////  Downloaded from: http://www.opencores.org                  ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2006-2010 Unicore Systems LTD                 ////
//// www.unicore.co.ua                                           ////
//// o.uzenkov@unicore.co.ua                                     ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
//// THIS SOFTWARE IS PROVIDED "AS IS"                           ////
//// AND ANY EXPRESSED OR IMPLIED WARRANTIES,                    ////
//// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED                  ////
//// WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT              ////
//// AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.        ////
//// IN NO EVENT SHALL THE UNICORE SYSTEMS OR ITS                ////
//// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,            ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL            ////
//// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT         ////
//// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,               ////
//// DATA, OR PROFITS; OR BUSINESS INTERRUPTION)                 ////
//// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,              ////
//// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT              ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING                 ////
//// IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,                 ////
//// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.          ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
// Design_Version       : 1.0
// File name            : CNORM.v
// File Revision        :
// Last modification    : Sun Sep 30 20:11:56 2007
/////////////////////////////////////////////////////////////////////
// FUNCTION: shifting left up to 3 bits
// PROPERTIES:  1)shifting left up to 3 bits controlled by
//                the 2-bit code SHIFT
//		    2)Is registered
//		    3)Overflow detector detects the overflow event
//                by the given shift condition. The detector is
//                zeroed by the START signal
//              4)RDY is the START signal delayed to a single
//                clock cycle
/////////////////////////////////////////////////////////////////////
module CNORM ( CLK ,ED ,START ,DR ,DI ,SHIFT ,OVF ,RDY ,DOR ,DOI );
	`USFFT64paramnb

	output OVF ;
	reg OVF ;
	output RDY ;
	reg RDY ;
	output [nb+1:0] DOR ;
	wire [nb+1:0] DOR ;
	output [nb+1:0] DOI ;
	wire [nb+1:0] DOI ;

	input CLK ;
	wire CLK ;
	input ED ;
	wire ED ;
	input START ;
	wire START ;
	input [nb+2:0] DR ;
	wire [nb+2:0] DR ;
	input [nb+2:0] DI ;
	wire [nb+2:0] DI ;
	input [1:0] SHIFT ;
	wire [1:0] SHIFT ;

	reg [nb+2:0] diri,diii;

	always @ (DR or SHIFT) begin
		case (SHIFT)
			2'h0: begin
				diri = DR;
			end
			2'h1: begin
				diri[(nb+2):1] = DR[(nb+2)-1:0];
				diri[0:0] = 1'b0;
			end
			2'h2: begin
				diri[(nb+2):2] = DR[(nb+2)-2:0];
				diri[1:0] = 2'b00;
			end
			2'h3: begin
				diri[(nb+2):3] = DR[(nb+2)-3:0];
				diri[2:0] = 3'b000;
			end
		endcase
	end

	always @ (DI or SHIFT) begin
		case (SHIFT)
			2'h0: begin
				diii = DI;
			end
			2'h1: begin
				diii[(nb+2):1] = DI[(nb+2)-1:0];
				diii[0:0] = 1'b0;
			end
			2'h2: begin
				diii[(nb+2):2] = DI[(nb+2)-2:0];
				diii[1:0] = 2'b00;
			end
			2'h3: begin
				diii[(nb+2):3] = DI[(nb+2)-3:0];
				diii[2:0] = 3'b000;
			end
		endcase
	end

reg [nb+2:0]	dir,dii;
    always @( posedge CLK )    begin
			if (ED)	  begin
					dir<=diri[nb+2:1];
     				dii<=diii[nb+2:1];
		end
	end


 always @( posedge CLK ) 	begin
		  	if (ED)	  begin
				RDY<=START;
				if (START)
					OVF<=0;
				else
					case (SHIFT)
					2'b01 : OVF<= (DR[nb+2] != DR[nb+1]) || (DI[nb+2] != DI[nb+1]);
					2'b10 : OVF<= (DR[nb+2] != DR[nb+1]) || (DI[nb+2] != DI[nb+1]) ||
						(DR[nb+2] != DR[nb]) || (DI[nb+2] != DI[nb]);
					2'b11 : OVF<= (DR[nb+2] != DR[nb+1]) || (DI[nb+2] != DI[nb+1])||
						(DR[nb+2] != DR[nb]) || (DI[nb+2] != DI[nb]) ||
						(DR[nb+2] != DR[nb+1]) || (DI[nb-1] != DI[nb-1]);
					endcase
				end
			end

	assign DOR= dir;
	assign DOI= dii;

endmodule

/////////////////////////////////////////////////////////////////////
////                                                             ////
////  8-point FFT, First stage of FFT 64 processor               ////
////                                                             ////
////  Authors: Anatoliy Sergienko, Volodya Lepeha                ////
////  Company: Unicore Systems http://unicore.co.ua              ////
////                                                             ////
////  Downloaded from: http://www.opencores.org                  ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2006-2010 Unicore Systems LTD                 ////
//// www.unicore.co.ua                                           ////
//// o.uzenkov@unicore.co.ua                                     ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
//// THIS SOFTWARE IS PROVIDED "AS IS"                           ////
//// AND ANY EXPRESSED OR IMPLIED WARRANTIES,                    ////
//// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED                  ////
//// WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT              ////
//// AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.        ////
//// IN NO EVENT SHALL THE UNICORE SYSTEMS OR ITS                ////
//// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,            ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL            ////
//// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT         ////
//// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,               ////
//// DATA, OR PROFITS; OR BUSINESS INTERRUPTION)                 ////
//// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,              ////
//// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT              ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING                 ////
//// IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,                 ////
//// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.          ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
// Design_Version       : 1.0
// File name            : FFT8.v
// File Revision        :
// Last modification    : Sun Sep 30 20:11:56 2007
/////////////////////////////////////////////////////////////////////
// FUNCTION: 8-point FFT
// FILES: FFT8.v - 1-st stage, contains
//        MPU707.v - multiplier to the factor 0.707.
// PROPERTIES:1) Fully pipelined
//		  2) Each clock cycle complex datum is entered
//               and complex result is outputted
//		  3) Has 8-clock cycle period starting with the START
//               impulse and continuing forever
//		  4) rounding	is not used
//		  5)Algorithm is from the book "H.J.Nussbaumer FFT and
//              convolution algorithms".
//		  6)IFFT is performed by substituting the output result
//              order to the reversed one
//		    (by exchanging - to + and + to -)
/////////////////////////////////////////////////////////////////////
//Algorithm:
//  procedure FFT8(
//		 D: in MEMOC8;  -- input array
//		 DO:out MEMOC8)  -- output ARRAY
//		is
//		variable t1,t2,t3,t4,t5,t6,t7,t8,m0,m1,m2,m3,m4,m5,m6,m7: complex;
//		variable s1,s2,s3,s4: complex;
//	begin
//		t1:=D(0) + D(4);
//		m3:=D(0) - D(4);
//		t2:=D(6) + D(2);
// 	    m6:=CBASE_j*(D(6)-D(2));
//		t3:=D(1) + D(5);
//		t4:=D(1) - D(5);
//		t5:=D(3) + D(7);
//		t6:=D(3) - D(7);
//		t8:=t5 + t3;
//		m5:=CBASE_j*(t5-t3);
//		t7:=t1 + t2;
//		m2:=t1 - t2;
//		m0:=t7 + t8;
//		m1:=t7 - t8;
//		m4:=SQRT(0.5)*(t4 - t6);
//		m7:=-CBASE_j*SQRT(0.5)*(t4 + t6);
//		s1:=m3 + m4;
//		s2:=m3 - m4;
//		s3:=m6 + m7;
//		s4:=m6 - m7;
//		DO(0):=m0;
//		DO(4):=m1;
//		DO(1):=s1 + s3;
//		DO(7):=s1 - s3;
//		DO(2):=m2 + m5;
//		DO(6):=m2 - m5;
//		DO(5):=s2 + s4;
//		DO(3):=s2 - s4;
//	end procedure;
/////////////////////////////////////////////////////////////////////

module FFT8 (
	CLK,
	RST,
	ED,
	START,
	DIR,
	DII,
	RDY,
	DOR,
	DOI
);

	`USFFT64paramnb

	input ED ;
	wire ED ;
	input RST ;
	wire RST ;
	input CLK ;
	wire CLK ;
	input [nb-1:0] DII ;
	wire [nb-1:0] DII ;
	input START ;
	wire START ;
	input [nb-1:0] DIR ;
	wire [nb-1:0] DIR ;

	output [nb+2:0] DOI ;
	wire [nb+2:0] DOI ;
	output [nb+2:0] DOR ;
	wire [nb+2:0] DOR ;
	output RDY ;
	reg RDY ;

	reg [2:0] ct; //main phase counter
	reg [3:0] ctd; //delay counter

	always @(   posedge CLK) begin	//Control counter
			//
			if (RST)	begin
					ct<=0;
					ctd<=15;
				RDY<=0;  end
			else if (START)	  begin
					ct<=0;
					ctd<=0;
				RDY<=0;   end
			else if (ED) begin
					ct<=ct+1;
					if (ctd !=4'b1111)
						ctd<=ctd+1;
					if (ctd==12 ) begin
						RDY<=1;
					end else begin
						RDY<=0;
					end
				end

		end

	reg	[nb-1: 0] dr,d1r,d2r,d3r,d4r,di,d1i,d2i,d3i,d4i;
	always @(posedge CLK)	  // input register file
		begin
			if (ED) 	begin
					dr<=DIR;
					d1r<=dr;
					d2r<=d1r;
					d3r<=d2r;
					d4r<=d3r;
					di<=DII;
					d1i<=di;
					d2i<=d1i;
					d3i<=d2i;
					d4i<=d3i;
				end
		end

	reg	[nb:0]	s1r,s2r,s1d1r,s1d2r,s1d3r,s2d1r,s2d2r,s2d3r;
	reg	[nb:0]	s1i,s2i,s1d1i,s1d2i,s1d3i,s2d1i,s2d2i,s2d3i;
	always @(posedge CLK)	begin		   // S1,S2 =t1-t6,m3 and delayed
			if (ED && ((ct==5) || (ct==6) || (ct==7) || (ct==0))) begin
					s1r<=d4r + dr ;
					s1i<=d4i + di ;
					s2r<=d4r - dr ;
					s2i<= d4i - di;
				end
			if	(ED)   begin
					s1d1r<=s1r;
					s1d2r<=s1d1r;
					s1d1i<=s1i;
					s1d2i<=s1d1i;
					if (ct==0 || ct==1)	 begin	  //## note for vhdl
							s1d3r<=s1d2r;
							s1d3i<=s1d2i;
						end
					if (ct==6 || ct==7 || ct==0) begin
							s2d1r<=s2r;
							s2d2r<=s2d1r;
							s2d1i<=s2i;
							s2d2i<=s2d1i;
						end
					if (ct==0) begin
							s2d3r<=s2d2r;
							s2d3i<=s2d2i;
						end
				end
		end


	reg [nb+1:0]	s3r,s4r,s3d1r,s3d2r,s3d3r;
	reg [nb+1:0]	s3i,s4i,s3d1i,s3d2i,s3d3i;
	always @(posedge CLK)	begin		  //ALU	S3:
			if (ED)
				case (ct)
					0: begin s3r<=  s1d2r+s1r;	 	   //t7
						s3i<= s1d2i+ s1i ;end
					1: begin s3r<=  s1d3r - s1d1r;	 	 //m2
						s3i<= s1d3i - s1d1i; end
					2: begin s3r<= s1d3r +s1r;	 	 //t8
						s3i<= s1d3i+ s1i ; end
					3: begin s3r<=  s1d3r - s1r;	 	 //
						s3i<= s1d3i - s1i ; end
				endcase

			if	(ED) begin
					if (ct==1 || ct==2 || ct==3) begin
							s3d1r<=s3r;						//t8
							s3d1i<=s3i;
						end
					if ( ct==2 || ct==3) begin
							s3d2r<=s3d1r;	  				//m2
							s3d3r<=s3d2r;				   //t7
							s3d2i<=s3d1i;
							s3d3i<=s3d2i;
						end
				end
		end

	always @ (posedge CLK)	begin		  // S4
			if (ED)	begin
					if (ct==1) begin
							s4r<= s2d2r + s2r;
						s4i<= s2d2i + s2i; end
					else if (ct==2) begin
							s4r<=s2d2r - s2r;
							s4i<= s2d2i - s2i;
						end
				end
		end

	wire em;
	assign	em = ((ct==2 || ct==3 || ct==4)&& ED);

	wire [nb+1:0] m4m7r,m4m7i;
	MPU707 UMR( .CLK(CLK),.DO(m4m7r),.DI(s4r),.EI(em));	 //  UMR
	MPU707 UMI( .CLK(CLK),.DO(m4m7i),.DI(s4i),.EI(em));	 //  UMR
	defparam UMR.nb = 16;
	defparam UMI.nb = 16;

	reg [nb+1:0]	sjr,sji, m6r,m6i;
	// wire [nb+1:0] a_zero_reg;
	// assign a_zero_reg = 0;
	always @ (posedge CLK)	begin		   //multiply by J
			if (ED) begin
					case  (ct)
						3: begin sjr<= s2d1i;	                //m6
							sji<= 0 - s2d1r; end
						4: begin sjr<= m4m7i;	//m7
							sji<= 0 - m4m7r;end
						6: begin sjr<= s3i;		//m5
							sji<= 0 - s3r;	  end
					endcase
					if (ct==4) begin
							m6r<=sjr;				 //m6
							m6i<=sji;
						end
				end
		end

	reg  [nb+2:0]	s5r,s5d1r,s5d2r,q1r;
	reg  [nb+2:0]	s5i,s5d1i,s5d2i,q1i;
	always @ (posedge CLK)		     // 	S5:
		if (ED)
			case  (ct)
				5: begin q1r<=s2d3r +m4m7r ;	   //	 S1
						q1i<=s2d3i +m4m7i ;
						s5r<=m6r + sjr;
					s5i<=m6i + sji; end
				6: begin 	s5r<=m6r - sjr;
						s5i<=m6i - sji;
						s5d1r<=s5r;
					s5d1i<=s5i; end
				7: begin	 s5r<=s2d3r - m4m7r;
						s5i<=s2d3i - m4m7i;
						s5d1r<=s5r;
						s5d1i<=s5i;
						s5d2r<=s5d1r;
						s5d2i<=s5d1i;
					end
			endcase

	reg  [nb+3:0]	s6r,s6i	;
	`ifdef paramifft
	always @ (posedge CLK)	begin		 //  S6-- result adder
			if (ED)
				case  (ct)
					5: begin s6r<=s3d3r +s3d1r ;	  // --	 D0
						s6i<=s3d3i +s3d1i ;end	   //--	 D0
					6: begin
								s6r<=q1r - s5r ;
							s6i<=q1i - s5i ;	 end
					7:  begin
								s6r<=s3d2r - sjr ;
							s6i<=s3d2i - sji ;	  end
					0:  begin
								s6r<=s5r + s5d1r ;
							s6i<= s5i +s5d1i ;	end
					1:begin	s6r<=s3d3r - s3d1r ;	    //--	 D4
						s6i<=s3d3i - s3d1i ; end
					2: begin
								s6r<= s5r - s5d1r ;	       //	 D5
							s6i<= s5i - s5d1i ;	   end
					3: begin  							    //	 D6
								s6r<=s3d3r + sjr ;
								s6i<=s3d3i + sji ;
							end
					4:   begin 									   //	 D0
								s6r<= q1r + s5d2r ;
								s6i<= q1i + s5d2i ;
							end
				endcase
		end

   `else
			always @ (posedge CLK)	begin		 //  S6-- result adder
			if (ED)
				case  (ct)
					5: begin s6r<=s3d3r +s3d1r ;	  // --	 D0
						s6i<=s3d3i +s3d1i ;end	   //--	 D0
					6:  begin
								s6r<=q1r + s5r ;	             //--	 D1
							s6i<=q1i + s5i ; end
						7:   begin
								s6r<=s3d2r +sjr ;	         //--	 D2
							s6i<=s3d2i +sji ;	   end
					0:   begin
								s6r<=s5r - s5d1r ;	               // --	 D3
							s6i<= s5i - s5d1i ;end

					1:begin	s6r<=s3d3r - s3d1r ;	    //--	 D4
						s6i<=s3d3i - s3d1i ; end
					2:   begin
								s6r<=s5r + s5d1r ;	              //--	 D5
							s6i<=s5i + s5d1i ; end

					3:  begin
								s6r<= s3d3r - sjr ;	        //	 D6
							s6i<=s3d3i - sji ;	end

					4:   begin
								s6r<= q1r - s5d2r ;	         //	 D0
							s6i<=  q1i - s5d2i ;	end

				endcase
		end
	`endif

	// assign #1	DOR=s6r[nb+2:0];
	// assign #1	DOI= s6i[nb+2:0];
	assign DOR=s6r[nb+2:0];
	assign DOI= s6i[nb+2:0];

endmodule
/////////////////////////////////////////////////////////////////////
////                                                             ////
////  Multiplier by 0.7071                                       ////
////                                                             ////
////  Authors: Anatoliy Sergienko, Volodya Lepeha                ////
////  Company: Unicore Systems http://unicore.co.ua              ////
////                                                             ////
////  Downloaded from: http://www.opencores.org                  ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2006-2010 Unicore Systems LTD                 ////
//// www.unicore.co.ua                                           ////
//// o.uzenkov@unicore.co.ua                                     ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
//// THIS SOFTWARE IS PROVIDED "AS IS"                           ////
//// AND ANY EXPRESSED OR IMPLIED WARRANTIES,                    ////
//// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED                  ////
//// WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT              ////
//// AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.        ////
//// IN NO EVENT SHALL THE UNICORE SYSTEMS OR ITS                ////
//// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,            ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL            ////
//// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT         ////
//// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,               ////
//// DATA, OR PROFITS; OR BUSINESS INTERRUPTION)                 ////
//// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,              ////
//// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT              ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING                 ////
//// IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,                 ////
//// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.          ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
// Design_Version       : 1.0
// File name            : MPU707.v
// File Revision        :
// Last modification    : Sun Sep 30 20:11:56 2007
/////////////////////////////////////////////////////////////////////
// FUNCTION: Constant multiplier
// PROPERTIES:1)Is based on shifts right and add
//		  2)for short input bit width 0.7071 is approximated as
//              10110101 then rounding is not used
//		  3)for long input bit width 0.7071 is approximated as
//              10110101000000101
//		  4)hardware is 3 or 4 adders
/////////////////////////////////////////////////////////////////////

module MPU707 ( CLK ,DO ,DI ,EI );
`USFFT64paramnb

	input CLK ;
	wire CLK ;
	input [nb+1:0] DI ;
	wire [nb+1:0] DI ;
	input EI ;
	wire EI ;

	output [nb+1:0] DO ;
	reg [nb+1:0] DO ;

	reg [nb+5 :0] dx5;
	reg	[nb+2 : 0] dt;
	wire [nb+6 : 0]  dx5p;
	wire   [nb+6 : 0] dot;

	always @(posedge CLK)
		begin
			if (EI) begin
					dx5<=DI+(DI <<2);	 //multiply by 5
					dt<=DI;
					DO<=dot >>4;
				end
		end

	`ifdef USFFT64bitwidth_0707_high
	assign   dot=	(dx5p+(dt>>4)+(dx5>>12));	   // multiply by 10110101000000101
	`else
	assign    dot=		(dx5p+(dt>>4) )	;  // multiply by 10110101
	`endif

		assign	dx5p=(dx5<<1)+(dx5>>2);		// multiply by 101101


endmodule
/////////////////////////////////////////////////////////////////////
////                                                             ////
////  2-port RAM                                                 ////
////                                                             ////
////  Authors: Anatoliy Sergienko, Volodya Lepeha                ////
////  Company: Unicore Systems http://unicore.co.ua              ////
////                                                             ////
////  Downloaded from: http://www.opencores.org                  ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2006-2010 Unicore Systems LTD                 ////
//// www.unicore.co.ua                                           ////
//// o.uzenkov@unicore.co.ua                                     ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
//// THIS SOFTWARE IS PROVIDED "AS IS"                           ////
//// AND ANY EXPRESSED OR IMPLIED WARRANTIES,                    ////
//// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED                  ////
//// WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT              ////
//// AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.        ////
//// IN NO EVENT SHALL THE UNICORE SYSTEMS OR ITS                ////
//// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,            ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL            ////
//// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT         ////
//// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,               ////
//// DATA, OR PROFITS; OR BUSINESS INTERRUPTION)                 ////
//// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,              ////
//// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT              ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING                 ////
//// IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,                 ////
//// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.          ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
// Design_Version       : 1.0
// File name            : RAM2x64C_1.v
// File Revision        :
// Last modification    : Sun Sep 30 20:11:56 2007
/////////////////////////////////////////////////////////////////////
// FUNCTION: 2-port RAM with 1 port to write and 1 port to read
// FILES: RAM2x64C_1.v - dual ported synchronous RAM, contains:
//	    RAM64.v -single ported synchronous RAM
// PROPERTIES: 1)Has the volume of 2x64 complex data
//	         2)Contains 4 single port RAMs for real and
//               imaginary parts of data in the 2-fold volume
//		     Two halves of RAM are switched on and off in the
//               write mode by the signal ODD
//		   3)RAM is synchronous one, the read datum is
//               outputted in 2 cycles after the address setting
//		   4)Can be substituted to any 2-port synchronous
//		     RAM for example, to one RAMB16_S36_S36 in XilinxFPGAs
/////////////////////////////////////////////////////////////////////

module RAM2x64C_1 ( CLK ,ED ,WE ,ODD ,ADDRW ,ADDRR ,DR ,DI ,DOR ,DOI );
	`USFFT64paramnb


	output [nb-1:0] DOR ;
	wire [nb-1:0] DOR ;
	output [nb-1:0] DOI ;
	wire [nb-1:0] DOI ;
	input CLK ;
	wire CLK ;
	input ED ;
	wire ED ;
	input WE ;	     //write enable
	wire WE ;
	input ODD ;	  // RAM part switshing
	wire ODD ;
	input [5:0] ADDRW ;
	wire [5:0] ADDRW ;
	input [5:0] ADDRR ;
	wire [5:0] ADDRR ;
	input [nb-1:0] DR ;
	wire [nb-1:0] DR ;
	input [nb-1:0] DI ;
	wire [nb-1:0] DI ;

	reg	oddd,odd2;
	always @( posedge CLK) begin //switch which reswiches the RAM parts
			if (ED)	begin
					oddd<=ODD;
					odd2<=oddd;
				end
		end
	`ifdef 	USFFT64bufferports1
	//One-port RAMs are used
	wire we0,we1;
	wire	[nb-1:0] dor0,dor1,doi0,doi1;
	wire	[5:0] addr0,addr1;



	assign	addr0 =ODD?  ADDRW: ADDRR;		//MUXA0
	assign	addr1 = ~ODD? ADDRW:ADDRR;	// MUXA1
	assign	we0   =ODD?  WE: 0;		     // MUXW0:
	assign	we1   =~ODD? WE: 0;			 // MUXW1:

	//1-st half - write when odd=1	 read when odd=0
	RAM64 URAM0(.CLK(CLK),.ED(ED),.WE(we0), .ADDR(addr0),.DI(DR),.DO(dor0)); //
	defparam URAM0.nb = 16;
	RAM64 URAM1(.CLK(CLK),.ED(ED),.WE(we0), .ADDR(addr0),.DI(DI),.DO(doi0));
	defparam URAM1.nb = 16;

	//2-d half
	RAM64 URAM2(.CLK(CLK),.ED(ED),.WE(we1), .ADDR(addr1),.DI(DR),.DO(dor1));//
	defparam URAM2.nb = 16;
	RAM64 URAM3(.CLK(CLK),.ED(ED),.WE(we1), .ADDR(addr1),.DI(DI),.DO(doi1));
	defparam URAM3.nb = 16;

	assign	DOR=~odd2? dor0 : dor1;		 // MUXDR:
	assign	DOI=~odd2? doi0 : doi1;	//  MUXDI:

	`else
	//Two-port RAM is used
	wire [6:0] addrr2 = {ODD,ADDRR};
	wire [6:0] addrw2 = {~ODD,ADDRW};
	wire [2*nb-1:0] di= {DR,DI} ;
	wire [2*nb-1:0] doi;

	reg [2*nb-1:0] ram [127:0];
	reg [6:0] read_addra;
	always @(posedge CLK) begin
			if (ED)
				begin
					if (WE)
						ram[addrw2] <= di;
					read_addra <= addrr2;
				end
		end
	assign doi = ram[read_addra];

	assign	DOR=doi[2*nb-1:nb];		 // Real read data
	assign	DOI=doi[nb-1:0];		 // Imaginary read data


	`endif
endmodule
/////////////////////////////////////////////////////////////////////
////                                                             ////
////  1-port synchronous RAM                                     ////
////                                                             ////
////  Authors: Anatoliy Sergienko, Volodya Lepeha                ////
////  Company: Unicore Systems http://unicore.co.ua              ////
////                                                             ////
////  Downloaded from: http://www.opencores.org                  ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2006-2010 Unicore Systems LTD                 ////
//// www.unicore.co.ua                                           ////
//// o.uzenkov@unicore.co.ua                                     ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
//// THIS SOFTWARE IS PROVIDED "AS IS"                           ////
//// AND ANY EXPRESSED OR IMPLIED WARRANTIES,                    ////
//// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED                  ////
//// WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT              ////
//// AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.        ////
//// IN NO EVENT SHALL THE UNICORE SYSTEMS OR ITS                ////
//// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,            ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL            ////
//// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT         ////
//// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,               ////
//// DATA, OR PROFITS; OR BUSINESS INTERRUPTION)                 ////
//// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,              ////
//// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT              ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING                 ////
//// IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,                 ////
//// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.          ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
// Design_Version       : 1.0
// File name            : RAM64.v
// File Revision        :
// Last modification    : Sun Sep 30 20:11:56 2007
/////////////////////////////////////////////////////////////////////
// FUNCTION: 1-port synchronous RAM
// FILES:    RAM64.v -single ported synchronous RAM
// PROPERTIES: 1) Has the volume of 64 data
//	         2) RAM is synchronous one, the read datum is outputted
//                in 2 cycles after the address setting
//		   3) Can be substituted to any 2-port synchronous RAM
/////////////////////////////////////////////////////////////////////

// commented out because it is not used in the default config,
// and ODIN II then thinks that there are 2 top level modules.
// module RAM64 ( CLK, ED,WE ,ADDR ,DI ,DO );
// 	`USFFT64paramnb

// 	output [nb-1:0] DO ;
// 	reg [nb-1:0] DO ;
// 	input CLK ;
// 	wire CLK ;
// 	input ED;
// 	input WE ;
// 	wire WE ;
// 	input [5:0] ADDR ;
// 	wire [5:0] ADDR ;
// 	input [nb-1:0] DI ;
// 	wire [nb-1:0] DI ;
// 	reg [nb-1:0] mem [63:0];
// 	reg [5:0] addrrd;

// 	always @(posedge CLK) begin
// 			if (ED) begin
// 					if (WE)		mem[ADDR] <= DI;
// 					addrrd <= ADDR;	         //storing the address
// 					DO <= mem[addrrd];	   // registering the read datum
// 				end
// 		end


// endmodule
/////////////////////////////////////////////////////////////////////
////                                                             ////
////  rotating unit, stays between 2 stages of FFT pipeline      ////
////                                                             ////
////  Authors: Anatoliy Sergienko, Volodya Lepeha                ////
////  Company: Unicore Systems http://unicore.co.ua              ////
////                                                             ////
////  Downloaded from: http://www.opencores.org                  ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2006-2010 Unicore Systems LTD                 ////
//// www.unicore.co.ua                                           ////
//// o.uzenkov@unicore.co.ua                                     ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
//// THIS SOFTWARE IS PROVIDED "AS IS"                           ////
//// AND ANY EXPRESSED OR IMPLIED WARRANTIES,                    ////
//// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED                  ////
//// WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT              ////
//// AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.        ////
//// IN NO EVENT SHALL THE UNICORE SYSTEMS OR ITS                ////
//// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,            ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL            ////
//// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT         ////
//// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,               ////
//// DATA, OR PROFITS; OR BUSINESS INTERRUPTION)                 ////
//// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,              ////
//// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT              ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING                 ////
//// IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,                 ////
//// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.          ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
// Design_Version       : 1.0
// File name            :
// File Revision        :
// Last modification    : Sun Sep 30 20:11:56 2007
/////////////////////////////////////////////////////////////////////
// FUNCTION: complex multiplication to the twiddle factors proper to the 64  point FFT
// PROPERTIES: 1) Has 64-clock cycle period starting with the START impulse and continuing forever
//	         2) rounding	is not used
/////////////////////////////////////////////////////////////////////

module ROTATOR64 (CLK ,RST,ED,START, DR, DI, RDY, DOR, DOI);
	`USFFT64paramnb
	`USFFT64paramnw

	input RST ;
	wire RST ;
	input CLK ;
	wire CLK ;
	input ED ; //operation enable
	input [nb+1:0] DI;  //Imaginary part of data
	wire [nb+1:0]  DI ;
	input [nb+1:0]  DR ; //Real part of data
	input START ;		   //1-st Data is entered after this impulse
	wire START ;

	output [nb+1:0]  DOI ; //Imaginary part of data
	wire [nb+1:0]  DOI ;
	output [nb+1:0]  DOR ; //Real part of data
	wire [nb+1:0]  DOR ;
	output RDY ;	   //repeats START impulse following the output data
	reg RDY ;


	reg [5:0] addrw;
	reg sd1,sd2;
	always	@( posedge CLK)	  //address counter for twiddle factors
		begin
			if (RST) begin
					addrw<=0;
					sd1<=0;
					sd2<=0;
				end
			else if (START && ED)  begin
					addrw[5:0]<=0;
					sd1<=START;
					sd2<=0;
				end
			else if (ED) 	  begin
					addrw<=addrw+1;
					sd1<=START;
					sd2<=sd1;
					RDY<=sd2;
				end
		end

		wire [nw-1:0] wr,wi; //twiddle factor coefficients
	//twiddle factor ROM
	WROM64 UROM(
		.WI(wi),
		.WR(wr),
		.ADDR(addrw)
	);


	reg [nb+1 : 0] drd,did;
	reg [nw-1 : 0] wrd,wid;
	wire [nw+nb+1 : 0] drri,drii,diri,diii;
	reg [nb+2:0] drr,dri,dir,dii,dwr,dwi;

	assign  	drri=drd*wrd;
	assign	diri=did*wrd;
	assign	drii=drd*wid;
	assign	diii=did*wid;

	always @(posedge CLK)	 //complex multiplier
		begin
			if (ED) begin
					drd<=DR;
					did<=DI;
					wrd<=wr;
					wid<=wi;
					drr<=drri[nw+nb+1 :nw-1]; //msbs of multiplications are stored
					dri<=drii[nw+nb+1 : nw-1];
					dir<=diri[nw+nb+1 : nw-1];
					dii<=diii[nw+nb+1 : nw-1];
					dwr<=drr - dii;
					dwi<=dri + dir;
				end
		end
	assign DOR=dwr[nb+2:1];
	assign DOI=dwi[nb+2 :1];

endmodule
/////////////////////////////////////////////////////////////////////
////                                                             ////
////  Top level  of the  high speed FFT  core                    ////
////                                                             ////
////  Authors: Anatoliy Sergienko, Volodya Lepeha                ////
////  Company: Unicore Systems http://unicore.co.ua              ////
////                                                             ////
////  Downloaded from: http://www.opencores.org                  ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2006-2010 Unicore Systems LTD                 ////
//// www.unicore.co.ua                                           ////
//// o.uzenkov@unicore.co.ua                                     ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
//// THIS SOFTWARE IS PROVIDED "AS IS"                           ////
//// AND ANY EXPRESSED OR IMPLIED WARRANTIES,                    ////
//// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED                  ////
//// WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT              ////
//// AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.        ////
//// IN NO EVENT SHALL THE UNICORE SYSTEMS OR ITS                ////
//// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,            ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL            ////
//// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT         ////
//// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,               ////
//// DATA, OR PROFITS; OR BUSINESS INTERRUPTION)                 ////
//// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,              ////
//// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT              ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING                 ////
//// IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,                 ////
//// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.          ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
// Design_Version       : 1.0
// File name            :
// File Revision        :
// Last modification    : Sun Sep 30 20:11:56 2007
/////////////////////////////////////////////////////////////////////
// FUNCTION: Structural model of the high speed 64-complex point FFT
// PROPERTIES:
//1.Fully pipelined, 1 complex data in, 1 complex result out each
//clock cycle
//2. Input data, output data, coefficient widths are adjustable
//in range 8..16
//3. Normalization stages trigger the data overflow and shift
//data right to prevent the overflow
//4. Core can contain 2 or 3 data buffers. In the configuration of
//2 buffers the results are in the shuffled order but provided with
//the proper address.
//5. The core operation can be slowed down by the control
//of the ED input
//6. The reset RST is synchronous
/////////////////////////////////////////////////////////////////////

module USFFT64_2B ( CLK ,RST ,ED ,START ,SHIFT ,DR ,DI ,RDY ,OVF1 ,OVF2 ,ADDR ,DOR ,DOI );
	`USFFT64paramnb		  	 		//nb is the data bit width

	output RDY ;   			// in the next cycle after RDY=1 the 0-th result is present
	wire RDY ;
	output OVF1 ;			// 1 signals that an overflow occured in the 1-st stage
	wire OVF1 ;
	output OVF2 ;			// 1 signals that an overflow occured in the 2-nd stage
	wire OVF2 ;
	output [5:0] ADDR ;	//result data address/number
	wire [5:0] ADDR ;
	output [nb+2:0] DOR ;//Real part of the output data,
	wire [nb+2:0] DOR ;	 // the bit width is nb+3, can be decreased when instantiating the core
	output [nb+2:0] DOI ;//Imaginary part of the output data
	wire [nb+2:0] DOI ;

	input CLK ;        			//Clock signal is less than 320 MHz for the Xilinx Virtex5 FPGA
	wire CLK ;
	input RST ;				//Reset signal, is the synchronous one with respect to CLK
	wire RST ;
	input ED ;					//=1 enables the operation (eneabling CLK)
	wire ED ;
	input START ;  			// its falling edge starts the transform or the serie of transforms
	wire START ;			 	// and resets the overflow detectors
	input [3:0] SHIFT ;		// bits 1,0 -shift left code in the 1-st stage
	wire [3:0] SHIFT ;	   	// bits 3,2 -shift left code in the 2-nd stage
	input [nb-1:0] DR ;		// Real part of the input data,  0-th data goes just after
	wire [nb-1:0] DR ;	    // the START signal or after 63-th data of the previous transform
	input [nb-1:0] DI ;		//Imaginary part of the input data
	wire [nb-1:0] DI ;

	wire [nb-1:0] dr1,di1;
	wire [nb+1:0] dr3,di3,dr4,di4, dr5,di5 ;
	wire [nb+2:0] dr2,di2;
	wire [nb+4:0] dr6,di6;
	wire [nb+2:0] dr7,di7,dr8,di8;
	wire rdy1,rdy2,rdy3,rdy4,rdy5,rdy6,rdy7,rdy8;
	reg [5:0] addri ;
												    // input buffer =8-bit inversion ordering
	BUFRAM64C1_NB16 U_BUF1(
		.CLK(CLK),
		.RST(RST),
		.ED(ED),
		.START(START),
		.DR(DR),
		.DI(DI),
		.RDY(rdy1),
		.DOR(dr1),
		.DOI(di1)
	);

	//1-st stage of FFT
	FFT8 U_FFT1(.CLK(CLK), .RST(RST), .ED(ED),
		.START(rdy1),.DIR(dr1),.DII(di1),
		.RDY(rdy2),	.DOR(dr2),.	DOI(di2));
	defparam U_FFT1.nb = 16;

	wire [1:0] shiftl;
	assign shiftl = SHIFT[1:0];

	CNORM U_NORM1( .CLK(CLK),	.ED(ED),  //1-st normalization unit
		.START(rdy2),	// overflow detector reset
		.DR(dr2),	.DI(di2),
		.SHIFT(shiftl), //shift left bit number
		.OVF(OVF1),
		.RDY(rdy3),
		.DOR(dr3),.DOI(di3));
	defparam U_NORM1.nb = 16;

	// rotator to the angles proportional to PI/32
	ROTATOR64 U_MPU (.CLK(CLK),.RST(RST),.ED(ED),
		.START(rdy3),. DR(dr3),.DI(di3),
		.RDY(rdy4), .DOR(dr4),	.DOI(di4));

	BUFRAM64C1_NB18 U_BUF2(.CLK(CLK), .RST(RST), .ED(ED),	// intermediate buffer =8-bit inversion ordering
		.START(rdy4), .DR(dr4), .DI(di4),
		.RDY(rdy5), .DOR(dr5),	.DOI(di5));
	//2-nd stage of FFT
	FFT8 U_FFT2(.CLK(CLK), .RST(RST), .ED(ED),
		.START(rdy5),. DIR(dr5),.DII(di5),
		.RDY(rdy6), .DOR(dr6),	.DOI(di6));
	defparam U_FFT2.nb = 18;

	wire [1:0] shifth;
	assign shifth = SHIFT[3:2];
	//2-nd normalization unit
	CNORM U_NORM2 ( .CLK(CLK),	.ED(ED),
		.START(rdy6),	// overflow detector reset
		.DR(dr6),	.DI(di6),
		.SHIFT(shifth), //shift left bit number
		.OVF(OVF2),
		.RDY(rdy7),
		.DOR(dr7),	.DOI(di7));
	defparam U_NORM2.nb = 18;


	BUFRAM64C1_NB19 Ubuf3(.CLK(CLK),.RST(RST),.ED(ED),	// intermediate buffer =8-bit inversion ordering
		.START(rdy7),. DR(dr7),.DI(di7),
		.RDY(rdy8), .DOR(dr8),	.DOI(di8));



	`ifdef USFFT64parambuffers3  	 	// 3-data buffer configuratiion
	always @(posedge CLK)	begin	//POINTER to the result samples
			if (RST)
				addri<=6'b000000;
			else if (rdy8==1 )
				addri<=6'b000000;
			else if (ED)
				addri<=addri+1;
		end

		assign ADDR=  addri ;
	assign	DOR=dr8;
	assign	DOI=di8;
	assign	RDY=rdy8;

	`else
	 	always @(posedge CLK)	begin	//POINTER to the result samples
			if (RST)
				addri<=6'b000000;
			else if (rdy7)
				addri<=6'b000000;
			else if (ED)
				addri<=addri+1;
		end
	assign ADDR=  {addri[2:0] , addri[5:3]} ;
	assign	DOR= dr7;
	assign	DOI= di7;
	assign	RDY= rdy7;
	`endif
endmodule
/////////////////////////////////////////////////////////////////////
////                                                             ////
////  Twiddle factor ROM for 64-point FFT                        ////
////                                                             ////
////  Authors: Anatoliy Sergienko, Volodya Lepeha                ////
////  Company: Unicore Systems http://unicore.co.ua              ////
////                                                             ////
////  Downloaded from: http://www.opencores.org                  ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2006-2010 Unicore Systems LTD                 ////
//// www.unicore.co.ua                                           ////
//// o.uzenkov@unicore.co.ua                                     ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
//// THIS SOFTWARE IS PROVIDED "AS IS"                           ////
//// AND ANY EXPRESSED OR IMPLIED WARRANTIES,                    ////
//// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED                  ////
//// WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT              ////
//// AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.        ////
//// IN NO EVENT SHALL THE UNICORE SYSTEMS OR ITS                ////
//// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,            ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL            ////
//// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT         ////
//// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,               ////
//// DATA, OR PROFITS; OR BUSINESS INTERRUPTION)                 ////
//// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,              ////
//// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT              ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING                 ////
//// IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,                 ////
//// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.          ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
// Design_Version       : 1.0
// File name            : WROM64.v
// File Revision        :
// Last modification    : Sun Sep 30 20:11:56 2007
/////////////////////////////////////////////////////////////////////
// FUNCTION: 1-port synchronous RAM
// FILES:    RAM64.v -single ported synchronous RAM
// PROPERTIES:
//1) Has 64 complex coefficients which form a table 8x8,
//and stay in the needed order, as they are addressed
//by the simple counter
//2) 16-bit values are stored. When shorter bit width is set
//then rounding	is not used
//3) for FFT and IFFT depending on paramifft
/////////////////////////////////////////////////////////////////////

`define w0 {c0,-s0}
`define w1 {c1,-s1}
`define w2 {c2,-s2}
`define w3 {c3,-s3}
`define w4 {c4,-s4}
`define w5 {c5,-s5}
`define w6 {c6,-s6}
`define w7 {c7,-s7}
`define w8 {c8,-c8}
`define w9 {s7,-c7}
`define w10 {s6,-c6}
`define w12 {s4,-c4}
`define w14 {s2,-c2}
`define w15 {s1,-c1}
`define w16 {s0,-c0}
`define w18 {-s2, -c2}
`define w20 {-s4, -c4}
`define w21 {-s5, -c5}
`define w24 {-c8, -c8}
`define w25 {-c7, -s7}
`define w28 {-c4, -s4}
`define w30 {-c2, -s2}
`define w35 {-c3, s3}
`define w36 {-c4, s4}
`define w42 {-s6, c6}
`define w49 {s1, c1}

`define wi0 {c0,s0}
`define wi1 {c1,s1}
`define wi2 {c2,s2}
`define wi3 {c3,s3}
`define wi4 {c4,s4}
`define wi5 {c5,s5}
`define wi6 {c6,s6}
`define wi7 {c7,s7}
`define wi8 {c8,c8}
`define wi9 {s7,c7}
`define wi10 {s6,c6}
`define wi12 {s4,c4}
`define wi14 {s2,c2}
`define wi15 {s1,c1}
`define wi16 {s0,c0}
`define wi18 {-s2, c2}
`define wi20 {-s4, c4}
`define wi21 {-s5, c5}
`define wi24 {-c8, c8}
`define wi25 {-c7, s7}
`define wi28 {-c4, s4}
`define wi30 {-c2, s2}
`define wi35 {-c3, -s3}
`define wi36 {-c4, -s4}
`define wi42 {-s6, -c6}
`define wi49 {s1, -c1}

//%%GENDEFINE%% (choose_from, blocking, 0, 63)
//%%GENDEFINE%% (always_list, 0, 63)

module WROM64 ( WI ,WR ,ADDR );
	`USFFT64paramnw

	input [5:0] ADDR ;
	wire [5:0] ADDR ;

	output [nw-1:0] WI ;
	wire [nw-1:0] WI ;
	output [nw-1:0] WR ;
	wire [nw-1:0] WR ;

	parameter  [15:0] c0 = 16'h7fff;
	parameter  [15:0] s0 = 16'h0000;
	parameter  [15:0] c1 = 16'h7f62;
	parameter  [15:0] s1 = 16'h0c8c;
	parameter  [15:0] c2 = 16'h7d8a;
	parameter  [15:0] s2 = 16'h18f9 ;
	parameter  [15:0] c3 = 16'h7a7d;
	parameter  [15:0] s3 = 16'h2528;
	parameter  [15:0] c4 = 16'h7642;
	parameter  [15:0] s4 = 16'h30fc;
	parameter  [15:0] c5 = 16'h70e3;
	parameter  [15:0] s5 = 16'h3c57;
	parameter  [15:0] c6 = 16'h6a6e;
	parameter  [15:0] s6 = 16'h471d ;
	parameter  [15:0] c7 = 16'h62f2;
	parameter  [15:0] s7 = 16'h5134;
	parameter  [15:0] c8 = 16'h5a82;

	wire [31:0] wf [0:63] ;
	// integer	i;

	// always@(ADDR) begin
			//(w0, w0, w0,  w0,  w0,  w0,  w0,  w0,	 	0..7 // twiddle factors for FFT
			//	w0, w1, w2,  w3,  w4,  w5,  w6,  w7,   	8..15
			//	w0, w2, w4,  w6,  w8,  w10,w12,w14,	16..23
			//	w0, w3, w6,  w9,  w12,w15,w18,w21,	24..31
			//	w0, w4, w8,  w12,w16,w20,w24,w28,	32..47
			//	w0, w5, w10,w15,w20,w25,w30,w35,
			//	w0, w6, w12,w18,w24,w30,w36,w42,
			//	w0, w7, w14,w21,w28,w35,w42,w49);
			// for( i =0; i<8; i=i+1) 	 wf[i] =w0;
			assign wf[0] = `wi0 ;
			assign wf[1] = `wi0 ;
			assign wf[2] = `wi0 ;
			assign wf[3] = `wi0 ;
			assign wf[4] = `wi0 ;
			assign wf[5] = `wi0 ;
			assign wf[6] = `wi0 ;
			assign wf[7] = `wi0 ;
			// for( i =8; i<63; i=i+8)  wf[i] =w0;
			assign wf[8] = `wi0 ;
			assign wf[16] = `wi0 ;
			assign wf[24] = `wi0 ;
			assign wf[32] = `wi0 ;
			assign wf[40] = `wi0 ;
			assign wf[48] = `wi0 ;
			assign wf[56] = `wi0 ;

			assign wf[9]  = `w1 ;
			assign wf[10] = `w2 ;
			assign wf[11] = `w3 ;
			assign wf[12] = `w4 ;
			assign wf[13] = `w5 ;
			assign wf[14] = `w6 ;
			assign wf[15] = `w7 ;
			assign wf[17] = `w2 ;
			assign wf[18] = `w4 ;
			assign wf[19] = `w6 ;
			assign wf[20] = `w8 ;
			assign wf[21] = `w10 ;
			assign wf[22] = `w12 ;
			assign wf[23] = `w14 ;
			assign wf[25] = `w3 ;
			assign wf[26] = `w6 ;
			assign wf[27] = `w9 ;
			assign wf[28] = `w12 ;
			assign wf[29] = `w15 ;
			assign wf[30] = `w18 ;
			assign wf[31] = `w21 ;
			assign wf[33] = `w4 ;
			assign wf[34] = `w8 ;
			assign wf[35] = `w12 ;
			assign wf[36] = `w16 ;
			assign wf[37] = `w20 ;
			assign wf[38] = `w24 ;
			assign wf[39] = `w28 ;
			assign wf[41] = `w5 ;
			assign wf[42] = `w10 ;
			assign wf[43] = `w15 ;
			assign wf[44] = `w20 ;
			assign wf[45] = `w25 ;
			assign wf[46] = `w30 ;
			assign wf[47] = `w35 ;
			assign wf[49] = `w6 ;
			assign wf[50] = `w12 ;
			assign wf[51] = `w18 ;
			assign wf[52] = `w24 ;
			assign wf[53] = `w30 ;
			assign wf[54] = `w36 ;
			assign wf[55] = `w42 ;
			assign wf[57] = `w7 ;
			assign wf[58] = `w14 ;
			assign wf[59] = `w21 ;
			assign wf[60] = `w28 ;
			assign wf[61] = `w35 ;
			assign wf[62] = `w42 ;
			assign wf[63] = `w49 ;

		// end

	wire [31:0] wb [0:63] ;
	// always@(ADDR) begin
	//initial begin #10;
			//(w0, w0, w0,  w0,  w0,  w0,  w0,  w0,	 	 // twiddle factors for IFFT
			// for( i =0; i<8; i=i+1) 	 wb[i] =wi0;
			assign wb[0] = `wi0 ;
			assign wb[1] = `wi0 ;
			assign wb[2] = `wi0 ;
			assign wb[3] = `wi0 ;
			assign wb[4] = `wi0 ;
			assign wb[5] = `wi0 ;
			assign wb[6] = `wi0 ;
			assign wb[7] = `wi0 ;
			// for( i =8; i<63; i=i+8)  wb[i] =wi0;
			assign wb[8] = `wi0 ;
			assign wb[16] = `wi0 ;
			assign wb[24] = `wi0 ;
			assign wb[32] = `wi0 ;
			assign wb[40] = `wi0 ;
			assign wb[48] = `wi0 ;
			assign wb[56] = `wi0 ;

			assign wb[9] = `wi1 ;
			assign wb[10] = `wi2 ;
			assign wb[11] = `wi3 ;
			assign wb[12] = `wi4 ;
			assign wb[13] = `wi5 ;
			assign wb[14] = `wi6 ;
			assign wb[15] = `wi7 ;
			assign wb[17] = `wi2 ;
			assign wb[18] = `wi4 ;
			assign wb[19] = `wi6 ;
			assign wb[20] = `wi8 ;
			assign wb[21] = `wi10 ;
			assign wb[22] = `wi12 ;
			assign wb[23] = `wi14 ;
			assign wb[25] = `wi3 ;
			assign wb[26] = `wi6 ;
			assign wb[27] = `wi9 ;
			assign wb[28] = `wi12 ;
			assign wb[29] = `wi15 ;
			assign wb[30] = `wi18 ;
			assign wb[31] = `wi21 ;
			assign wb[33] = `wi4 ;
			assign wb[34] = `wi8 ;
			assign wb[35] = `wi12 ;
			assign wb[36] = `wi16 ;
			assign wb[37] = `wi20 ;
			assign wb[38] = `wi24 ;
			assign wb[39] = `wi28 ;
			assign wb[41] = `wi5 ;
			assign wb[42] = `wi10 ;
			assign wb[43] = `wi15 ;
			assign wb[44] = `wi20 ;
			assign wb[45] = `wi25 ;
			assign wb[46] = `wi30 ;
			assign wb[47] = `wi35 ;
			assign wb[49] = `wi6 ;
			assign wb[50] = `wi12 ;
			assign wb[51] = `wi18 ;
			assign wb[52] = `wi24 ;
			assign wb[53] = `wi30 ;
			assign wb[54] = `wi36 ;
			assign wb[55] = `wi42 ;
			assign wb[57] = `wi7 ;
			assign wb[58] = `wi14 ;
			assign wb[59] = `wi21 ;
			assign wb[60] = `wi28 ;
			assign wb[61] = `wi35 ;
			assign wb[62] = `wi42 ;
			assign wb[63] = `wi49 ;

		// end

	reg [31:0] reim;

	`ifdef USFFT64paramifft
		// in place of:
		// assign reim = wb[ADDR];
		always @ (ADDR or `always_list_0_63(wb)) begin
			`choose_from_blocking_0_63(ADDR,reim,wb)
		end
	`else
		// in place of:
		// assign reim = wf[ADDR];
		always @ (ADDR or `always_list_0_63(wf)) begin
			`choose_from_blocking_0_63(ADDR,reim,wf)
		end
	`endif

	assign WR =reim[31:32-nw];
	assign WI=reim[15 :16-nw];


endmodule
