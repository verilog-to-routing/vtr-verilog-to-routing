module iir (clk, reset, start, din, params, dout, ready,iir_start,iir_done);
input clk, reset, start;
input [7:0] din;
input [15:0] params;
output [7:0] dout;
reg [7:0] dout;
output ready;
reg ready;
reg temp_ready;
reg [6:0] finite_counter;
wire count0;
input iir_start;
output iir_done;
wire iir_done;
reg del_count0;

reg [15:0] a1, a2, b0, b1, b2, yk1, yk2;
reg [7:0] uk, uk1, uk2 ;
reg [28:0] ysum ;
reg [26:0] yk ;
reg [22:0] utmp;
reg [3:0] wait_counter ;
// temporary variable
wire [31:0] yo1, yo2;
//wire [23:0] b0t, b1t, b2t;
wire [22:0] b0t, b1t, b2t;
wire [22:0] b0tpaj, b1tpaj, b2tpaj;

reg [3:0] obf_state, obf_next_state ;

reg [7:0] temp_uk, temp_uk1, temp_uk2 ;
reg [15:0] temp_a1, temp_a2, temp_b0, temp_b1, temp_b2, temp_yk1, temp_yk2;
reg [28:0] temp_ysum ;
reg [26:0] temp_yk ;
reg [22:0] temp_utmp;
reg [7:0] temp_dout;
reg [3:0] temp_wait_counter ;

parameter 
idle = 4'b0001 ,
load_a2 = 4'b0010 ,
load_b0 = 4'b0011 ,
load_b1 = 4'b0100 ,
load_b2 = 4'b0101 ,
wait4_start = 4'b0110 , 
latch_din = 4'b0111 ,
compute_a = 4'b1000 ,
compute_b = 4'b1001 ,
compute_yk = 4'b1010 ,
wait4_count = 4'b1011 ,
latch_dout = 4'b1100 ;

always @(posedge clk) 
begin
	case (obf_state ) 
		idle : 
			begin
				if (iir_start)
					obf_next_state <= load_a2 ;
				else
					obf_next_state <= idle;
				temp_a1 <= params ; 
			end
		load_a2 :
			begin
				 obf_next_state <= load_b0 ;
				 temp_a2 <= params ; 
			end
		load_b0 : 
			begin
				obf_next_state <= load_b1 ;
				temp_b0 <= params ; 
			end
		load_b1 : 
			begin
				obf_next_state <= load_b2 ;
				temp_b1 <= params ; 
			end
		load_b2 : 
			begin
				obf_next_state <= wait4_start ;
				temp_b2 <= params ; 
			end
		wait4_start : 
			begin
				if (start)
				begin
					obf_next_state <= latch_din ;
					temp_uk <= din ; 
				end
				else
				begin
					obf_next_state <= wait4_start ;
					temp_uk <= uk;
				end
				temp_ready <= wait4_start;
			end
		latch_din : 
			begin
				obf_next_state <= compute_a ;
			end
		compute_a : 
			begin
				obf_next_state <= compute_b ;

				temp_ysum <= yo1[31:3] + yo2[31:3];
			end
		compute_b : 
			begin
				obf_next_state <= compute_yk ;

//				temp_utmp <= b0t[23:0] + b1t[23:0] + b2t[23:0];
				temp_utmp <= b0t + b1t + b2t;
			end
		compute_yk : 
			begin
				obf_next_state <= wait4_count ;
				temp_uk1 <= uk ;
				temp_uk2 <= uk1 ;
				temp_yk <= ysum[26:0] + {utmp[22], utmp[22], utmp[22], utmp[22], utmp};
				temp_wait_counter <= 4 ;
			end

		wait4_count : 
			begin
				if (wait_counter==0 )
				begin
					obf_next_state <= latch_dout ;

					temp_dout <= yk[26:19];
					temp_yk1 <= yk[26:11] ;
					temp_yk2 <= yk1 ;
				end
				else
				begin
					obf_next_state <= wait4_count ; 
					temp_dout <= dout;
					temp_yk1 <= yk1;
					//temp_yk2 <= yk2;
				end

				temp_wait_counter <= wait_counter - 1;
			end
		latch_dout : 
				if (count0)
					obf_next_state <= idle;
				else 
					obf_next_state <= wait4_start ;
	endcase
end 



//assign yo1 = mul_tc_16_16(yk1, a1, clk);
assign yo1 = yk1 * a1;
//assign yo2 = mul_tc_16_16(yk2, a2, clk);
assign yo2 = yk2*a2;
//assign b0t = mul_tc_8_16(uk, b0, clk);
//assign b1t = mul_tc_8_16(uk1, b1, clk);
//assign b2t = mul_tc_8_16(uk2, b2, clk);
assign b0t = uk*b0;
assign b1t = uk1*b1;
assign b2t = uk2*b2;
// paj added to solve unused high order bit
assign b0tpaj = b0t;
assign b1tpaj = b1t;
assign b2tpaj = b2t;

wire tmp_uk;
wire tmp_uk1;
wire tmp_uk2;
wire tmp_yk1;
wire tmp_yk2;
wire tmp_yk;
wire tmp_ysum;
wire tmp_utmp;
wire tmp_a1;
wire tmp_a2;
wire tmp_b0;
wire tmp_b1;
wire tmp_b2;
wire tmp_dout;
wire tmp_obf_state;
wire tmp_ready;

// async reset
assign uk = (reset)? 0: tmp_uk;
assign uk1 = (reset)? 0: tmp_uk1;
assign uk2 = (reset)? 0: tmp_uk2;
assign yk1 = (reset)? 0: tmp_yk1;
assign yk2 = (reset)? 0: tmp_yk2;
assign yk = (reset)? 0: tmp_yk;
assign ysum = (reset)? 0: tmp_ysum;
assign utmp = (reset)? 0: tmp_utmp;
assign a1 = (reset)? 0: tmp_a1;
assign a2 = (reset)? 0: tmp_a2;
assign b0 = (reset)? 0: tmp_b0;
assign b1 = (reset)? 0: tmp_b1;
assign b2 = (reset)? 0: tmp_b2;
assign dout = (reset)? 0: tmp_dout;
assign obf_state = (reset)? idle: tmp_obf_state;
assign ready = (reset)? 0: tmp_ready;

always @(posedge clk) begin
	tmp_obf_state <= obf_next_state ;
	tmp_uk1 <= temp_uk1;
	tmp_uk2 <= temp_uk2;
	tmp_yk <= temp_yk;
	tmp_uk <= temp_uk ;
	tmp_a1 <= temp_a1 ; 
	tmp_a2 <= temp_a2 ; 
	tmp_b0 <= temp_b0 ; 
	tmp_b1 <= temp_b1 ; 
	tmp_b2 <= temp_b2 ; 
	tmp_ysum <= temp_ysum;
	tmp_utmp <= temp_utmp;
	tmp_dout <= temp_dout;
	tmp_yk1 <= temp_yk1;
	tmp_yk2 <= temp_yk2;
	tmp_ready <= temp_ready;
end

// wait counter, count 4 clock after sum is calculated, to 
// time outputs are ready, and filter is ready to accept next
// input
wire tmp_wait_counter;
assign wait_counter = (reset)? 0 : tmp_wait_counter;wire tmp_finite_counter;


always @(posedge clk ) begin
	tmp_wait_counter <= temp_wait_counter ;
end

always @(posedge clk) begin
	if (reset)
		finite_counter<=100;
	else 
		if (iir_start) 
			finite_counter<=finite_counter -1;
		else 
			finite_counter<=finite_counter;
end

assign count0=finite_counter==7'b0;

always @(posedge clk) begin
	del_count0 <= count0;
end

assign iir_done = (count0 && ~del_count0); 

endmodule
