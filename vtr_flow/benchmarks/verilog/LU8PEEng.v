//auto-generated top.v
//top level module of LU factorization
//by Wei Zhang

`define NWIDTH 6'b010100
`define BLOCKWIDTH 4'b0101
`define DDRWIDTH 7'b0100000
`define DDRNUMDQS 4'b0100
`define DDRSIZEWIDTH 6'b011000
`define BURSTLEN 3'b010
`define MEMCONWIDTH 8'b01000000
`define MEMCONNUMBYTES 5'b01000
`define RAMWIDTH 10'b0100000000
`define RAMNUMBYTES 7'b0100000
`define RAMSIZEWIDTH 4'b0101
`define TOPWIDTH 7'b0100000
`define rFIFOINPUTWIDTH 8'b01000000
`define wFIFOINPUTWIDTH 10'b0100000000
`define mFIFOWIDTH 6'b011100
`define aFIFOWIDTH 4'b0101

module LU8PEEng (clk, //ref_clk, global_reset_n,
 start, N, offset, done,
		//mem_addr, mem_ba, mem_cas_n, mem_cke, mem_clk, mem_clk_n, mem_cs_n,
burst_begin,
mem_local_be,
mem_local_read_req,
mem_local_size,
mem_local_wdata,
mem_local_write_req,
mem_local_rdata,
mem_local_rdata_valid,
mem_local_ready,
mem_local_wdata_req,
reset_n,
mem_local_addr
//Cong: dummy output
//a_junk,
//w_junk,
//m_junk,
//r_junk,
//Cong:dummy output
//junk_r,
//junk_r1,
//junk_r2,
//junk_r3,
//junk_top
	);

input start;
input[`NWIDTH-1:0] N;
input[`DDRSIZEWIDTH-1:0] offset;
output done;
input clk;

output burst_begin;
output [`MEMCONNUMBYTES-1:0] mem_local_be;
output mem_local_read_req;
output [`BURSTLEN-1:0] mem_local_size;
output [`MEMCONWIDTH-1:0] mem_local_wdata;
output mem_local_write_req;
output [`DDRSIZEWIDTH-1:0] mem_local_addr;
input [`MEMCONWIDTH-1:0] mem_local_rdata;
input mem_local_rdata_valid;
input mem_local_ready;
input reset_n;
input mem_local_wdata_req;
wire[`BLOCKWIDTH-1:0] m, n, loop;
wire[1:0] mode;
wire comp_start, comp_done;
wire dtu_write_req, dtu_read_req, dtu_ack, dtu_done;
wire [`DDRSIZEWIDTH-1:0] dtu_mem_addr;
wire [`RAMSIZEWIDTH-1:0] dtu_ram_addr;
wire [`BLOCKWIDTH-1:0] dtu_size;
wire left_sel;

wire[`RAMWIDTH-1:0] curWriteDataMem, curReadDataMem;
wire[`RAMSIZEWIDTH-1:0] curWriteAddrMem, curReadAddrMem;
wire[`RAMNUMBYTES-1:0] curWriteByteEnMem;
wire curWriteEnMem;
wire[`RAMWIDTH-1:0] leftWriteDataMem;
wire[`RAMSIZEWIDTH-1:0] leftWriteAddrMem;
wire[`RAMNUMBYTES-1:0] leftWriteByteEnMem;
wire leftWriteEnMem;
wire curMemSel, leftMemSel;

wire burst_begin;
wire [`MEMCONNUMBYTES-1:0] mem_local_be;
wire mem_local_read_req;
wire [`BURSTLEN-1:0] mem_local_size;
wire [`MEMCONWIDTH-1:0] mem_local_wdata;
wire mem_local_write_req;
wire [`MEMCONWIDTH-1:0] mem_local_rdata;
wire mem_local_rdata_valid;
wire mem_local_ready;
wire mem_local_wdata_req;
wire reset_n;
wire [`DDRSIZEWIDTH-1:0] mem_local_addr;

wire[`RAMWIDTH-1:0] ram_write_data, ram_read_data;
wire[`RAMSIZEWIDTH-1:0] ram_write_addr, ram_read_addr;
wire[`RAMNUMBYTES-1:0] ram_write_byte_en;
wire ram_write_en;

MarshallerController MC (clk, start, done, N, offset,
	comp_start, m, n, loop, mode, comp_done, curMemSel, leftMemSel,
	dtu_write_req, dtu_read_req, dtu_mem_addr, dtu_ram_addr, dtu_size, dtu_ack, dtu_done, left_sel);

// block that computes the LU factorization, with answer stored back into ram block
LU compBlock (clk, comp_start, m, n, loop, mode, comp_done,
			curReadAddrMem, curReadDataMem, curWriteByteEnMem, curWriteDataMem, curWriteAddrMem, curWriteEnMem, curMemSel,
			leftWriteByteEnMem, leftWriteDataMem, leftWriteAddrMem, leftWriteEnMem, leftMemSel);

DataTransferUnit DTU (.clk(clk), .dtu_write_req(dtu_write_req), .dtu_read_req(dtu_read_req), .dtu_mem_addr(dtu_mem_addr), .dtu_ram_addr(dtu_ram_addr), .dtu_size(dtu_size), .dtu_ack(dtu_ack), .dtu_done(dtu_done),
		.ram_read_addr(ram_read_addr), .ram_read_data(ram_read_data), .ram_write_byte_en(ram_write_byte_en), .ram_write_data(ram_write_data), .ram_write_addr(ram_write_addr), .ram_write_en(ram_write_en),
		.mem_rdata(mem_local_rdata), .mem_rdata_valid(mem_local_rdata_valid), .mem_ready(mem_local_ready), .mem_wdata_req(mem_local_wdata_req), .reset_n(reset_n),
		.burst_begin(burst_begin), .mem_local_addr(mem_local_addr), .mem_be(mem_local_be), .mem_read_req(mem_local_read_req), .mem_size(mem_local_size),
		.mem_wdata(mem_local_wdata), .mem_write_req(mem_local_write_req)
		//Cong: dummy output
		);

assign curReadAddrMem = ram_read_addr;
assign curWriteByteEnMem = ram_write_byte_en;
assign curWriteDataMem = ram_write_data;
assign curWriteAddrMem = ram_write_addr;
assign curWriteEnMem = ram_write_en && (left_sel == 0);
assign leftWriteByteEnMem = ram_write_byte_en;
assign leftWriteDataMem = ram_write_data;
assign leftWriteAddrMem = ram_write_addr;
assign leftWriteEnMem = ram_write_en && (left_sel == 1);
assign ram_read_data = curReadDataMem;
endmodule
`define BLOCKM 6'b010000
`define BLOCKN 6'b010000
`define BLOCKMDIVK 3'b010
`define MEMBLOCKM 5'b01000
`define MEMBLOCKN 5'b01000
`define NWIDTH 6'b010100
`define BLOCKWIDTH 4'b0101
`define DDRSIZEWIDTH 6'b011000
`define RAMSIZEWIDTH 4'b0101
`define START 1'b0 //0
`define SETUP 2'b01 //1
`define FIRST 3'b010 //2
`define MODE0_SETUP 3'b011 //3
`define MODE0_WAIT 4'b0100 //4
`define MODE0 4'b0101 //5
`define MODE1_SETUP 4'b0110 //6
`define MODE1_WAIT 4'b0111 //7
`define MODE1 5'b01000 //8
`define MODE2_SETUP 5'b01001 //9
`define MODE2_WAIT 5'b01010 //10
`define MODE2 5'b01011 //11
`define MODE3_SETUP 5'b01100 //12
`define MODE3_WAIT 5'b01101 //13
`define MODE3 5'b01110 //14
`define STALL 5'b01111 //15
`define STALL_WAIT 6'b010000 //16
`define WAIT 6'b010001 //17
`define FINAL_WRITE 6'b010010 //18
`define FINAL_WAIT 6'b010011 //19
`define IDLE 6'b010100 //20
`define LAST_SETUP 6'b010101 //21
`define LAST_SETUP_WAIT 6'b010110 //22
`define LAST 6'b010111 //23
`define LAST_WAIT 6'b011000 //24
`define MEM_IDLE 1'b0 //0
`define MEM_WRITE 2'b01 //1
`define MEM_WRITE_WAIT 3'b010 //2
`define MEM_CHECK_DONE 3'b011 //3
`define MEM_READ 4'b0100 //4
`define MEM_READ_WAIT 4'b0101 //5
`define MEM_DONE 4'b0110 //6
`define MEM_WAIT_DONE 4'b0111 //7

module MarshallerController (clk, start, done, input_N, offset,
	comp_start, block_m, block_n, loop, mode, comp_done, cur_mem_sel, left_mem_sel,
	dtu_write_req, dtu_read_req, dtu_mem_addr, dtu_ram_addr, dtu_size, dtu_ack, dtu_done, left_sel);


input clk;
input start;
output done;
input [`NWIDTH-1:0] input_N;
input [`DDRSIZEWIDTH-1:0] offset;

// for computation section
output comp_start;
output [`BLOCKWIDTH-1:0] block_m, block_n, loop;
output [1:0] mode;
input comp_done;
output cur_mem_sel, left_mem_sel;

// for data marshaller section
output dtu_write_req,	dtu_read_req;
output [`DDRSIZEWIDTH-1:0] dtu_mem_addr;
output [`RAMSIZEWIDTH-1:0] dtu_ram_addr;
output [`BLOCKWIDTH-1:0] dtu_size;
input dtu_ack, dtu_done;
output left_sel;

reg [4:0] cur_state, next_state;
reg [`NWIDTH-1:0] comp_N, N, mcount, ncount, Ndivk, mem_N;
reg [1:0] mode;
reg [`BLOCKWIDTH-1:0] block_m, block_n, loop, read_n;
reg [`BLOCKWIDTH-1:0] write_n, write_n_buf;
reg left_mem_sel, cur_mem_sel, no_left_switch;

reg [3:0] cur_mem_state, next_mem_state;
reg [`RAMSIZEWIDTH-1:0] ram_addr;
reg [`DDRSIZEWIDTH-1:0] mem_addr;
reg [`DDRSIZEWIDTH-1:0] mem_base, mem_top, mem_write, mem_left, mem_cur;
reg [`DDRSIZEWIDTH-1:0] mem_write_buf;
reg [`BLOCKWIDTH-1:0] mem_count;
reg [1:0] mem_read;
reg [`BLOCKWIDTH-1:0] mem_write_size, mem_write_size_buf, mem_read_size;
wire mem_done;

assign done = (cur_state == `IDLE);
assign dtu_ram_addr = ram_addr;
assign dtu_mem_addr = mem_addr;
assign dtu_size = (cur_mem_state == `MEM_WRITE) ? mem_write_size : mem_read_size;
assign comp_start = (cur_state == `MODE0)||(cur_state == `MODE1)||(cur_state == `MODE2)||(cur_state == `MODE3)||(cur_state == `FIRST)||(cur_state == `LAST);
assign dtu_write_req = (cur_mem_state == `MEM_WRITE);
assign dtu_read_req = (cur_mem_state == `MEM_READ);
assign mem_done = (cur_mem_state == `MEM_DONE)&&(dtu_done == 1'b1);
assign left_sel = mem_read == 2'b01 && (cur_mem_state == `MEM_READ || cur_mem_state == `MEM_READ_WAIT || cur_mem_state == `MEM_WAIT_DONE);

// FSM to produce memory instructions to DTU
always @ (posedge clk)
begin
	case (cur_mem_state)
	`MEM_IDLE:
	begin
		if (cur_state == `START)
			next_mem_state <= `MEM_CHECK_DONE;
		else
			next_mem_state <= `MEM_IDLE;
	end
	`MEM_DONE:
	begin
		if (cur_state == `MODE0 || cur_state == `MODE1 || cur_state == `MODE2 || 
			cur_state == `MODE3 || cur_state == `FINAL_WRITE || cur_state == `LAST_SETUP)
			next_mem_state <= `MEM_WRITE;
		else if (cur_state == `FIRST)
			next_mem_state <= `MEM_CHECK_DONE;
		else
			next_mem_state <= `MEM_DONE;
	end
	`MEM_WRITE:
	begin
		next_mem_state <= `MEM_WRITE_WAIT;
	end
	`MEM_WRITE_WAIT:
	begin
		if (dtu_ack == 1'b1)
		begin
			if (mem_count == write_n)
				next_mem_state <= `MEM_WAIT_DONE;
			else 
				next_mem_state <= `MEM_WRITE;
		end
		else
			next_mem_state <= `MEM_WRITE_WAIT;
	end
	`MEM_WAIT_DONE:
	begin
		if (dtu_done == 1'b1)
			next_mem_state <= `MEM_CHECK_DONE;
		else
			next_mem_state <= `MEM_WAIT_DONE;
	end
	`MEM_CHECK_DONE:
	begin
		if (mem_read == 2'b10)
			next_mem_state <= `MEM_DONE;
		else
			next_mem_state <= `MEM_READ;
	end
	`MEM_READ:
	begin
		next_mem_state <= `MEM_READ_WAIT;
	end
	`MEM_READ_WAIT:
	begin
		if (dtu_ack == 1'b1)
		begin
			if (mem_count == read_n)
				next_mem_state <= `MEM_WAIT_DONE;
			else
				next_mem_state <= `MEM_READ;
		end
		else
			next_mem_state <= `MEM_READ_WAIT;
	end
	default:
		next_mem_state <= `MEM_IDLE;
	endcase
end

always @ (posedge clk)
begin
	if (cur_mem_state == `MEM_DONE || cur_mem_state == `MEM_IDLE)
	begin
		ram_addr <= 5'b0;
		mem_addr <= mem_write;
		if (next_state == `LAST_WAIT || next_state == `FINAL_WAIT || next_state == `STALL)
			mem_read <= 2'b00;
		else if (next_state == `MODE0_SETUP || next_state == `SETUP || cur_state == `MODE0 || next_state == `LAST_SETUP_WAIT)
			mem_read <= 2'b01;
		else
			mem_read <= 2'b10;
		mem_count <= 5'b0;
	end
	else if (cur_mem_state == `MEM_CHECK_DONE)
	begin
		if (mem_read == 2'b10)
		begin
			mem_addr <= mem_left;
			read_n <= loop;
		end
		else
		begin
			mem_addr <= mem_cur;
			read_n <= block_n;
		end
		mem_read <= mem_read - 2'b01;
		mem_count <= 5'b0;
		ram_addr <= 5'b0;
	end
	else if (cur_mem_state == `MEM_WRITE || cur_mem_state == `MEM_READ)
	begin
		ram_addr <= ram_addr + `BLOCKMDIVK;
		mem_addr <= mem_addr + Ndivk;
		mem_count <= mem_count + 2'b01;
	end
	
end

// FSM to determine the block LU factorization algorithm
always @ (posedge clk)
begin
	case (cur_state)
	`START:
	begin
		next_state <= `SETUP;
	end
	`SETUP:
	begin
		next_state <= `WAIT;
	end
	`WAIT:
	begin
		if (mem_done == 1'b1)
			next_state <= `FIRST;
		else
			next_state <= `WAIT;

	end
	`FIRST:
	begin
		if (mcount < comp_N)
			next_state <= `MODE1_SETUP;
		else if (ncount < comp_N)
			next_state <= `MODE2_SETUP;
		else
			next_state <= `LAST_WAIT;
	end
	`MODE0_SETUP:
	begin
		next_state <= `MODE0_WAIT;
	end
	`MODE0_WAIT:
	begin
		if (mem_done == 1'b1 && comp_done == 1'b1)
			next_state <= `MODE0;
		else
			next_state <= `MODE0_WAIT;

	end
	`MODE0:
	begin
		if (mcount < comp_N)
			next_state <= `MODE1_SETUP;
		else if (ncount < comp_N)
			next_state <= `MODE2_SETUP;
		else
		begin
			next_state <= `LAST_WAIT;
		end
	end
	`MODE1_SETUP:
	begin
		next_state <= `MODE1_WAIT;
	end
	`MODE1_WAIT:
	begin
		if (mem_done == 1'b1 && comp_done == 1'b1)
			next_state <= `MODE1;
		else
			next_state <= `MODE1_WAIT;

	end
	`MODE1:
	begin
		if (mcount < comp_N)
			next_state <= `MODE1_SETUP;
		else if (ncount < comp_N)
			next_state <= `MODE2_SETUP;
		else if (comp_N <= `BLOCKN + `BLOCKN)
			next_state <= `STALL;
		else
			next_state <= `MODE0_SETUP;
	end
	`MODE2_SETUP:
	begin
		next_state <= `MODE2_WAIT;
	end
	`MODE2_WAIT:
	begin
		if (mem_done == 1'b1 && comp_done == 1'b1)
			next_state <= `MODE2;
		else
			next_state <= `MODE2_WAIT;
	end
	`MODE2:
	begin
		if (mcount < comp_N)
			next_state <= `MODE3_SETUP;
		else if (ncount < comp_N)
			next_state <= `MODE2_SETUP;
		else if (comp_N <= `BLOCKN + `BLOCKN)
			next_state <= `STALL;
		else
			next_state <= `MODE0_SETUP;
	end
	`MODE3_SETUP:
	begin
		next_state <= `MODE3_WAIT;
	end
	`MODE3_WAIT:
	begin
		if (mem_done == 1'b1 && comp_done == 1'b1)
			next_state <= `MODE3;
		else
			next_state <= `MODE3_WAIT;
	end
	`MODE3:
	begin
		if (mcount < comp_N)
			next_state <= `MODE3_SETUP;
		else if (ncount < comp_N)
			next_state <= `MODE2_SETUP;
		else if (comp_N <= `BLOCKN + `BLOCKN)
			next_state <= `STALL;
		else
			next_state <= `MODE0_SETUP;
	end
	`STALL:
		next_state <= `STALL_WAIT;
	`STALL_WAIT:
		if (mem_done == 1'b1 && comp_done == 1'b1)
			next_state <= `LAST_SETUP;
		else
			next_state <= `STALL_WAIT;
	`LAST_SETUP:
		next_state <= `LAST_SETUP_WAIT;
	`LAST_SETUP_WAIT:
		if (mem_done == 1'b1 && comp_done == 1'b1)
			next_state <= `LAST;
		else
			next_state <= `LAST_SETUP_WAIT;
	`LAST:
		next_state <= `LAST_WAIT;
	`LAST_WAIT:
		if (mem_done == 1'b1 && comp_done == 1'b1)
			next_state <= `FINAL_WRITE;
		else
			next_state <= `LAST_WAIT;
	`FINAL_WRITE:
		next_state <= `FINAL_WAIT;
	`FINAL_WAIT:
		if (mem_done == 1'b1)
			next_state <= `IDLE;
		else
			next_state <= `FINAL_WAIT;
	`IDLE:
		if (start)
			next_state <= `SETUP;
		else
			next_state <= `IDLE;
	default:
		next_state <= `START;
	endcase
end

always @ (posedge clk)
begin
	if (start)
	begin
		cur_state <= `START;
		cur_mem_state <= `MEM_IDLE;
	end
	else
	begin
		cur_state <= next_state;
		cur_mem_state <= next_mem_state;
	end
end

always @ (cur_state)
begin
	case (cur_state)
	`MODE1:
		mode = 2'b01;
	`MODE2:
		mode = 2'b10;
	`MODE3:
		mode = 2'b11;
	default:
		mode = 2'b00;
	endcase
end

always @ (posedge clk)
begin
	if (start)
	begin
		comp_N <= input_N;
		N <= input_N;
	end
	else if (next_state == `MODE0)
	begin
		comp_N <= comp_N - `BLOCKN;
	end

	Ndivk <= ((N+`BLOCKM-1)>>4)<<3;
	mem_N <= Ndivk<<4;

	if (start)
	begin
		mem_base <= offset;
		mem_top <= offset;
		mem_left <= offset;
		mem_cur <= offset;
	end
	else if (cur_state == `MODE0_SETUP)
	begin
		mem_base <= mem_base + mem_N+`MEMBLOCKN;
		mem_top <= mem_base + mem_N+`MEMBLOCKN;
		mem_cur <= mem_base + mem_N+`MEMBLOCKN;
		mem_left <= mem_base + mem_N+`MEMBLOCKN;
	end
	else if (cur_state == `MODE1_SETUP)
	begin
		mem_cur <= mem_cur + `MEMBLOCKM;
	end	
	else if (cur_state == `MODE3_SETUP)
	begin
		mem_cur <= mem_cur + `MEMBLOCKM;
		mem_left <= mem_left + `MEMBLOCKM;
	end
	else if (cur_state == `MODE2_SETUP)
	begin
		mem_cur <= mem_top + mem_N;
		mem_top <= mem_top + mem_N;
		mem_left <= mem_base;
	end

	if (cur_state == `SETUP)
	begin
		mem_write <= 24'b0;
		mem_write_buf <= 24'b0;
		mem_write_size <= `BLOCKMDIVK;
		mem_write_size_buf <= `BLOCKMDIVK;
		write_n <= block_n;
		write_n_buf <= block_n;
	end
	else if (cur_mem_state == `MEM_CHECK_DONE && mem_read == 0)
	begin
		mem_write <= mem_write_buf;
		mem_write_buf <= mem_cur;
		mem_write_size <= mem_write_size_buf;
		mem_write_size_buf <= mem_read_size;
		write_n <= write_n_buf;
		write_n_buf <= block_n;
	end

	mem_read_size <= `BLOCKMDIVK;

	if (start) begin
		loop <= `BLOCKN;
	end else if (next_state == `LAST) begin
		loop <= comp_N[8:0] - `BLOCKN;
	end

	if (cur_state == `MODE0_SETUP || cur_state == `MODE2_SETUP || start) begin
		mcount <= `BLOCKM;
	end else if (cur_state == `MODE1_SETUP || cur_state == `MODE3_SETUP) begin
		mcount <= mcount+`BLOCKM;
	end

	if (cur_state == `MODE0_SETUP || start) begin
		ncount <= `BLOCKN;
	end else if (cur_state == `MODE2_SETUP) begin
		ncount <= ncount+`BLOCKN;
	end

	if (mcount < comp_N) begin
		block_m <= `BLOCKM;
	end else begin
		block_m <= comp_N - mcount + `BLOCKM;
	end 

	if (ncount < comp_N) begin
		block_n <= `BLOCKN;
	end else begin
		block_n <= comp_N - ncount + `BLOCKN;
	end

	if (start) begin
		cur_mem_sel <= 1'b0;
	end else if ((cur_state == `MODE0)||(cur_state == `MODE1)||(cur_state == `MODE2)||(cur_state == `MODE3)||
		 (cur_state == `FIRST)||(cur_state == `FINAL_WRITE)||(cur_state == `LAST_SETUP)||(cur_state == `LAST)) begin
		cur_mem_sel <= !cur_mem_sel;
	end 

	if (start) begin
		no_left_switch <= 1'b0;
	end else if ((cur_state == `MODE0)||(cur_state == `FIRST)) begin
		no_left_switch <= 1'b1;
	end else if ((cur_state == `MODE1)||(cur_state == `MODE2)||(cur_state == `MODE3)||
		 (cur_state == `FINAL_WRITE)||(cur_state == `LAST_SETUP)) begin
		no_left_switch <= 1'b0;
	end

	if (start) begin
		left_mem_sel <= 1'b0;
	end else if (((cur_state == `MODE0)||(cur_state ==`MODE1)||(cur_state == `MODE2)||(cur_state == `MODE3)||
		 (cur_state == `FIRST)||(cur_state == `FINAL_WRITE)||(cur_state == `LAST_SETUP))&&(no_left_switch == 1'b0)) begin
		left_mem_sel <= !left_mem_sel;
	end 
end

endmodule


//topoutputdelay = 1
//auto-generated LU.v
//datapath for computating LU factorization
//by Wei Zhang

`define rRAMSIZEWIDTH 5
`define cSETUP 4'b0000
`define cSTART 4'b0001
`define cFETCH_COL 4'b0010
`define cWAIT_COL 4'b0011
`define cFIND_REC 4'b0100
`define cMULT_COL 4'b0101
`define cUPDATE_J 4'b0110
`define cSTORE_MO 4'b0111
`define cMULT_SUB 4'b1000
`define cINCRE_I 4'b1001
`define cWAIT 4'b1010
`define cDONE 4'b1011
`define cSTORE_DIAG 4'b1100
`define cSTORE_DIAG2 4'b1101
`define cSTART_FETCH_ROW 4'b1110
`define cROW_WAIT 2'b00
`define cFETCH_ROW 2'b01
`define cDONE_FETCH_ROW 2'b10
`define cLOAD_ROW_INC_J 2'b11

`define PRECISION 7'b0100000
`define NUMPE 5'b01000
`define PEWIDTH 3'b011
`define BLOCKWIDTH 4'b0101
`define RAMWIDTH 10'b0100000000
`define RAMNUMBYTES 7'b0100000
`define RAMSIZEWIDTH 4'b0101
`define TOPSIZEWIDTH 5'b01000
`define TOPINPUTDELAY 3'b011
`define TOPOUTPUTDELAY 2'b01
`define MEMINPUTDELAY 3'b010
`define MEMOUTPUTDELAY 2'b01
`define TOPWIDTH 7'b0100000
 
module LU (clk, start, m, n, loop, mode, done, 
			curReadAddrMem, curReadDataMem, curWriteByteEnMem, curWriteDataMem, curWriteAddrMem, curWriteEnMem, curMemSel,
			leftWriteByteEnMem, leftWriteDataMem, leftWriteAddrMem, leftWriteEnMem, leftMemSel
);


input clk, start;
input[`BLOCKWIDTH-1:0] m, n, loop;
input[1:0] mode;
output done;
wire[`RAMWIDTH-1:0] curWriteData0, curWriteData1;
wire[`RAMSIZEWIDTH-1:0] curWriteAddr0, curReadAddr0, curWriteAddr1, curReadAddr1;
wire[`RAMWIDTH-1:0] curReadData0, curReadData1;
wire[`RAMNUMBYTES-1:0] curWriteByteEn0, curWriteByteEn1;
wire curWriteEn0, curWriteEn1;

input[`RAMWIDTH-1:0] curWriteDataMem;
output[`RAMWIDTH-1:0] curReadDataMem;
input[`RAMSIZEWIDTH-1:0] curWriteAddrMem, curReadAddrMem;
input[`RAMNUMBYTES-1:0] curWriteByteEnMem;
input curWriteEnMem;
input[`RAMWIDTH-1:0] leftWriteDataMem;
input[`RAMSIZEWIDTH-1:0] leftWriteAddrMem;
input[`RAMNUMBYTES-1:0] leftWriteByteEnMem;
input leftWriteEnMem;
input leftMemSel, curMemSel;

wire[`RAMWIDTH-1:0] curReadDataLU, curReadDataMem;
wire[`RAMWIDTH-1:0] curWriteDataLU, curWriteDataMem;
wire[`RAMSIZEWIDTH-1:0] curWriteAddrLU, curWriteAddrMem, curReadAddrLU, curReadAddrMem;
wire[`RAMNUMBYTES-1:0] curWriteByteEnLU, curWriteByteEnMem;
wire curWriteEnLU, curWriteEnMem;

reg[`RAMWIDTH-1:0] curReadData0Reg0;
reg[`RAMWIDTH-1:0] curReadData1Reg0;
reg[`RAMWIDTH-1:0] leftReadData0Reg0;
reg[`RAMWIDTH-1:0] leftReadData1Reg0;
reg[`RAMWIDTH-1:0] curWriteData0Reg0;
reg[`RAMWIDTH-1:0] curWriteData0Reg1;
reg[`RAMWIDTH-1:0] curWriteData1Reg0;
reg[`RAMWIDTH-1:0] curWriteData1Reg1;
reg[`RAMSIZEWIDTH-1:0] curWriteAddr0Reg0;
reg[`RAMSIZEWIDTH-1:0] curWriteAddr0Reg1;
reg[`RAMSIZEWIDTH-1:0] curReadAddr0Reg0;
reg[`RAMSIZEWIDTH-1:0] curReadAddr0Reg1;
reg[`RAMSIZEWIDTH-1:0] curWriteAddr1Reg0;
reg[`RAMSIZEWIDTH-1:0] curWriteAddr1Reg1;
reg[`RAMSIZEWIDTH-1:0] curReadAddr1Reg0;
reg[`RAMSIZEWIDTH-1:0] curReadAddr1Reg1;
reg[`RAMNUMBYTES-1:0] curWriteByteEn0Reg0;
reg[`RAMNUMBYTES-1:0] curWriteByteEn0Reg1;
reg[`RAMNUMBYTES-1:0] curWriteByteEn1Reg0;
reg[`RAMNUMBYTES-1:0] curWriteByteEn1Reg1;
reg curWriteEn0Reg0;
reg curWriteEn0Reg1;
reg curWriteEn1Reg0;
reg curWriteEn1Reg1;
reg[`RAMWIDTH-1:0] leftWriteData0Reg0;
reg[`RAMWIDTH-1:0] leftWriteData0Reg1;
reg[`RAMWIDTH-1:0] leftWriteData1Reg0;
reg[`RAMWIDTH-1:0] leftWriteData1Reg1;
reg[`RAMSIZEWIDTH-1:0] leftWriteAddr0Reg0;
reg[`RAMSIZEWIDTH-1:0] leftWriteAddr0Reg1;
reg[`RAMSIZEWIDTH-1:0] leftReadAddr0Reg0;
reg[`RAMSIZEWIDTH-1:0] leftReadAddr0Reg1;
reg[`RAMSIZEWIDTH-1:0] leftWriteAddr1Reg0;
reg[`RAMSIZEWIDTH-1:0] leftWriteAddr1Reg1;
reg[`RAMSIZEWIDTH-1:0] leftReadAddr1Reg0;
reg[`RAMSIZEWIDTH-1:0] leftReadAddr1Reg1;
reg[`RAMNUMBYTES-1:0] leftWriteByteEn0Reg0;
reg[`RAMNUMBYTES-1:0] leftWriteByteEn0Reg1;
reg[`RAMNUMBYTES-1:0] leftWriteByteEn1Reg0;
reg[`RAMNUMBYTES-1:0] leftWriteByteEn1Reg1;
reg leftWriteEn0Reg0;
reg leftWriteEn0Reg1;
reg leftWriteEn1Reg0;
reg leftWriteEn1Reg1;

reg[`PRECISION-1:0] multOperand;
reg[`PRECISION-1:0] diag;
wire[`PRECISION-1:0] recResult;
wire[`PRECISION-1:0] multA0;
wire[`PRECISION-1:0] multA1;
wire[`PRECISION-1:0] multA2;
wire[`PRECISION-1:0] multA3;
wire[`PRECISION-1:0] multA4;
wire[`PRECISION-1:0] multA5;
wire[`PRECISION-1:0] multA6;
wire[`PRECISION-1:0] multA7;
wire[`PRECISION-1:0] multResult0;
wire[`PRECISION-1:0] multResult1;
wire[`PRECISION-1:0] multResult2;
wire[`PRECISION-1:0] multResult3;
wire[`PRECISION-1:0] multResult4;
wire[`PRECISION-1:0] multResult5;
wire[`PRECISION-1:0] multResult6;
wire[`PRECISION-1:0] multResult7;
wire[`PRECISION-1:0] addA0;
wire[`PRECISION-1:0] addA1;
wire[`PRECISION-1:0] addA2;
wire[`PRECISION-1:0] addA3;
wire[`PRECISION-1:0] addA4;
wire[`PRECISION-1:0] addA5;
wire[`PRECISION-1:0] addA6;
wire[`PRECISION-1:0] addA7;
wire[`PRECISION-1:0] addResult0;
wire[`PRECISION-1:0] addResult1;
wire[`PRECISION-1:0] addResult2;
wire[`PRECISION-1:0] addResult3;
wire[`PRECISION-1:0] addResult4;
wire[`PRECISION-1:0] addResult5;
wire[`PRECISION-1:0] addResult6;
wire[`PRECISION-1:0] addResult7;
wire[`RAMWIDTH-1:0] leftReadData0, leftReadData1, leftWriteData0, leftWriteData1;
wire[`RAMSIZEWIDTH-1:0] leftWriteAddr0, leftWriteAddr1, leftReadAddr0, leftReadAddr1;
wire[`RAMNUMBYTES-1:0] leftWriteByteEn0, leftWriteByteEn1;
wire leftWriteEn0, leftWriteEn1;
wire[`RAMWIDTH-1:0] leftReadDataLU, leftWriteDataLU, leftWriteDataMem;
wire[`RAMSIZEWIDTH-1:0] leftWriteAddrLU, leftWriteAddrMem, leftReadAddrLU;
wire[`RAMNUMBYTES-1:0] leftWriteByteEnLU, leftWriteByteEnMem;
wire leftWriteEnLU, leftWriteEnMem;

wire[`PRECISION-1:0] topWriteData;
reg[`PRECISION-1:0] topWriteDataLU;
wire[`PRECISION-1:0] topReadData, topReadDataLU;
wire[`TOPSIZEWIDTH-1:0] topWriteAddr, topWriteAddrLU, topReadAddr, topReadAddrLU;
wire topWriteEn, topWriteEnLU;

reg[`PRECISION-1:0] topReadDataReg0;
reg[`PRECISION-1:0] topWriteDataReg0;
reg[`PRECISION-1:0] topWriteDataReg1;
reg[`PRECISION-1:0] topWriteDataReg2;
reg[`TOPSIZEWIDTH-1:0] topWriteAddrReg0;
reg[`TOPSIZEWIDTH-1:0] topWriteAddrReg1;
reg[`TOPSIZEWIDTH-1:0] topWriteAddrReg2;
reg[`TOPSIZEWIDTH-1:0] topReadAddrReg0;
reg[`TOPSIZEWIDTH-1:0] topReadAddrReg1;
reg[`TOPSIZEWIDTH-1:0] topReadAddrReg2;
reg topWriteEnReg0;
reg topWriteEnReg1;
reg topWriteEnReg2;
wire[`RAMWIDTH-1:0] rcWriteData;
wire leftWriteSel, curWriteSel, topSourceSel;
wire diagEn;
wire[`PEWIDTH-1:0] topWriteSel;

wire MOSel;
wire MOEn;

// control block
LUControl conBlock (clk, start, m, n, loop, mode, done, 
                    curReadAddrLU, curWriteAddrLU, curWriteByteEnLU, curWriteEnLU, curWriteSel,
                    leftReadAddrLU, leftWriteAddrLU, leftWriteByteEnLU, leftWriteEnLU,  leftWriteSel,
                    topReadAddrLU, topWriteAddrLU, topWriteEnLU, topWriteSel, topSourceSel, diagEn, MOSel, MOEn);

// fp_div unit
//floating point divider here
fpu_div rec(.clock(clk), .n(32'h3F800000), .d(diag), .div(recResult));
// on-chip memory blocks that store the matrix to be LU factorized
// store current blocks data
ram currentBlock0 (curWriteByteEn0, clk, curWriteData0, curReadAddr0, curWriteAddr0, curWriteEn0, curReadData0 );
ram1 currentBlock1 (curWriteByteEn1, clk, curWriteData1, curReadAddr1, curWriteAddr1, curWriteEn1, curReadData1 );
// store left blocks data
ram2 leftBlock0(leftWriteByteEn0, clk, leftWriteData0, leftReadAddr0, leftWriteAddr0, leftWriteEn0, leftReadData0 );

ram3 leftBlock1(leftWriteByteEn1, clk, leftWriteData1, leftReadAddr1, leftWriteAddr1, leftWriteEn1, leftReadData1 );

// store top block data
top_ram topBlock(clk, topWriteData, topReadAddr, topWriteAddr, topWriteEn, topReadDataLU );

// processing elements that does the main computation of LU factorization
mult_add PE0 (clk, multA0, multOperand, addA0, multResult0, addResult0);
mult_add PE1 (clk, multA1, multOperand, addA1, multResult1, addResult1);
mult_add PE2 (clk, multA2, multOperand, addA2, multResult2, addResult2);
mult_add PE3 (clk, multA3, multOperand, addA3, multResult3, addResult3);
mult_add PE4 (clk, multA4, multOperand, addA4, multResult4, addResult4);
mult_add PE5 (clk, multA5, multOperand, addA5, multResult5, addResult5);
mult_add PE6 (clk, multA6, multOperand, addA6, multResult6, addResult6);
mult_add PE7 (clk, multA7, multOperand, addA7, multResult7, addResult7);

// connect to ports of the left blocks
assign leftWriteDataLU = (leftWriteSel == 1'b0) ? curReadDataLU : rcWriteData;
always @ (posedge clk)
begin
	if(leftMemSel == 1'b0)
	begin
		leftWriteData0Reg0 <= leftWriteDataMem;
		leftWriteAddr0Reg0 <= leftWriteAddrMem;
		leftWriteByteEn0Reg0 <= leftWriteByteEnMem;
		leftWriteEn0Reg0 <= leftWriteEnMem;
		leftWriteData1Reg0 <= leftWriteDataLU;
		leftWriteAddr1Reg0 <= leftWriteAddrLU;
		leftWriteByteEn1Reg0 <= leftWriteByteEnLU;
		leftWriteEn1Reg0 <= leftWriteEnLU;
	end
	else
	begin
		leftWriteData0Reg0 <= leftWriteDataLU;
		leftWriteAddr0Reg0 <= leftWriteAddrLU;
		leftWriteByteEn0Reg0 <= leftWriteByteEnLU;
		leftWriteEn0Reg0 <= leftWriteEnLU;
		leftWriteData1Reg0 <= leftWriteDataMem;
		leftWriteAddr1Reg0 <= leftWriteAddrMem;
		leftWriteByteEn1Reg0 <= leftWriteByteEnMem;
		leftWriteEn1Reg0 <= leftWriteEnMem;
	end
	leftReadAddr0Reg0 <= leftReadAddrLU;
	leftReadAddr1Reg0 <= leftReadAddrLU;
	leftWriteData0Reg1 <= leftWriteData0Reg0;
	leftWriteAddr0Reg1 <= leftWriteAddr0Reg0;
	leftReadAddr0Reg1 <= leftReadAddr0Reg0;
	leftWriteByteEn0Reg1 <= leftWriteByteEn0Reg0;
	leftWriteEn0Reg1 <= leftWriteEn0Reg0;
	leftWriteData1Reg1 <= leftWriteData1Reg0;
	leftWriteAddr1Reg1 <= leftWriteAddr1Reg0;
	leftReadAddr1Reg1 <= leftReadAddr1Reg0;
	leftWriteByteEn1Reg1 <= leftWriteByteEn1Reg0;
	leftWriteEn1Reg1 <= leftWriteEn1Reg0;
end
assign leftWriteData0 = leftWriteData0Reg1;
assign leftWriteAddr0 = leftWriteAddr0Reg1;
assign leftReadAddr0 = leftReadAddr0Reg1;
assign leftWriteByteEn0 = leftWriteByteEn0Reg1;
assign leftWriteEn0 = leftWriteEn0Reg1;
assign leftWriteData1 = leftWriteData1Reg1;
assign leftWriteAddr1 = leftWriteAddr1Reg1;
assign leftReadAddr1 = leftReadAddr1Reg1;
assign leftWriteByteEn1 = leftWriteByteEn1Reg1;
assign leftWriteEn1 = leftWriteEn1Reg1;

always @ (posedge clk)
begin
		leftReadData0Reg0 <= leftReadData0;
		leftReadData1Reg0 <= leftReadData1;
end
assign leftReadDataLU = (leftMemSel == 1'b0) ? leftReadData1Reg0 : leftReadData0Reg0;
// data feed to fp div unit
always @ (posedge clk)
begin
	if (diagEn == 1'b1)
	begin
			diag <= topReadData;
	end
end
// one of the inputs to the PE
always @ (posedge clk)
begin
	if (start == 1'b1)
		multOperand <= 0;
	else if (MOEn == 1'b1)
	begin
		if (MOSel == 1'b0)
			multOperand <= recResult;
		else
			multOperand <= topReadData;
	end
end

// connections to top block memory ports
always @ (topSourceSel or topWriteSel or curReadDataLU or addResult7 or addResult6 or addResult5 or addResult4 or addResult3 or addResult2 or addResult1 or addResult0)
begin
	if (topSourceSel == 1'b0)
		case (topWriteSel)
		0:
			topWriteDataLU = curReadDataLU[255:224];
		1:
			topWriteDataLU = curReadDataLU[223:192];
		2:
			topWriteDataLU = curReadDataLU[191:160];
		3:
			topWriteDataLU = curReadDataLU[159:128];
		4:
			topWriteDataLU = curReadDataLU[127:96];
		5:
			topWriteDataLU = curReadDataLU[95:64];
		6:
			topWriteDataLU = curReadDataLU[63:32];
		7:
			topWriteDataLU = curReadDataLU[31:0];
		default:
			topWriteDataLU = curReadDataLU[`PRECISION-1:0];
		endcase
	else
		case (topWriteSel)
		0:
			topWriteDataLU = addResult7;
		1:
			topWriteDataLU = addResult6;
		2:
			topWriteDataLU = addResult5;
		3:
			topWriteDataLU = addResult4;
		4:
			topWriteDataLU = addResult3;
		5:
			topWriteDataLU = addResult2;
		6:
			topWriteDataLU = addResult1;
		7:
			topWriteDataLU = addResult0;
		default:
			topWriteDataLU = addResult0;
		endcase
end

always @ (posedge clk)
begin
	topWriteDataReg0 <= topWriteDataLU;
	topReadAddrReg0 <= topReadAddrLU;
	topWriteAddrReg0 <= topWriteAddrLU;
	topWriteEnReg0 <= topWriteEnLU;
	topWriteDataReg1 <= topWriteDataReg0;
	topReadAddrReg1 <= topReadAddrReg0;
	topWriteAddrReg1 <= topWriteAddrReg0;
	topWriteEnReg1 <= topWriteEnReg0;
	topWriteDataReg2 <= topWriteDataReg1;
	topReadAddrReg2 <= topReadAddrReg1;
	topWriteAddrReg2 <= topWriteAddrReg1;
	topWriteEnReg2 <= topWriteEnReg1;
end
assign topWriteData = topWriteDataReg2;
assign topReadAddr = topReadAddrReg2;
assign topWriteAddr = topWriteAddrReg2;
assign topWriteEn = topWriteEnReg2;
always @ (posedge clk)
begin
	topReadDataReg0 <= topReadDataLU;
end
assign topReadData = topReadDataReg0;

// connections to processing element
assign multA0 = leftReadDataLU[31:0];
assign multA1 = leftReadDataLU[63:32];
assign multA2 = leftReadDataLU[95:64];
assign multA3 = leftReadDataLU[127:96];
assign multA4 = leftReadDataLU[159:128];
assign multA5 = leftReadDataLU[191:160];
assign multA6 = leftReadDataLU[223:192];
assign multA7 = leftReadDataLU[255:224];

assign addA0 = curReadDataLU[31:0];
assign addA1 = curReadDataLU[63:32];
assign addA2 = curReadDataLU[95:64];
assign addA3 = curReadDataLU[127:96];
assign addA4 = curReadDataLU[159:128];
assign addA5 = curReadDataLU[191:160];
assign addA6 = curReadDataLU[223:192];
assign addA7 = curReadDataLU[255:224];

// connections to ports of the current blocks
assign rcWriteData[31:0] = (curWriteSel == 0) ? multResult0 : addResult0;
assign rcWriteData[63:32] = (curWriteSel == 0) ? multResult1 : addResult1;
assign rcWriteData[95:64] = (curWriteSel == 0) ? multResult2 : addResult2;
assign rcWriteData[127:96] = (curWriteSel == 0) ? multResult3 : addResult3;
assign rcWriteData[159:128] = (curWriteSel == 0) ? multResult4 : addResult4;
assign rcWriteData[191:160] = (curWriteSel == 0) ? multResult5 : addResult5;
assign rcWriteData[223:192] = (curWriteSel == 0) ? multResult6 : addResult6;
assign rcWriteData[255:224] = (curWriteSel == 0) ? multResult7 : addResult7;
assign curWriteDataLU = rcWriteData;

always @ (posedge clk)
begin
	if(curMemSel == 1'b0)
	begin
		curWriteData0Reg0 <= curWriteDataMem;
		curWriteAddr0Reg0 <= curWriteAddrMem;
		curReadAddr0Reg0 <= curReadAddrMem;
		curWriteByteEn0Reg0 <= curWriteByteEnMem;
		curWriteEn0Reg0 <= curWriteEnMem;
		curWriteData1Reg0 <= curWriteDataLU;
		curWriteAddr1Reg0 <= curWriteAddrLU;
		curReadAddr1Reg0 <= curReadAddrLU;
		curWriteByteEn1Reg0 <= curWriteByteEnLU;
		curWriteEn1Reg0 <= curWriteEnLU;
	end
	else
	begin
		curWriteData0Reg0 <= curWriteDataLU;
		curWriteAddr0Reg0 <= curWriteAddrLU;
		curReadAddr0Reg0 <= curReadAddrLU;
		curWriteByteEn0Reg0 <= curWriteByteEnLU;
		curWriteEn0Reg0 <= curWriteEnLU;
		curWriteData1Reg0 <= curWriteDataMem;
		curWriteAddr1Reg0 <= curWriteAddrMem;
		curReadAddr1Reg0 <= curReadAddrMem;
		curWriteByteEn1Reg0 <= curWriteByteEnMem;
		curWriteEn1Reg0 <= curWriteEnMem;
	end
	curWriteData0Reg1 <= curWriteData0Reg0;
	curWriteAddr0Reg1 <= curWriteAddr0Reg0;
	curReadAddr0Reg1 <= curReadAddr0Reg0;
	curWriteByteEn0Reg1 <= curWriteByteEn0Reg0;
	curWriteEn0Reg1 <= curWriteEn0Reg0;
	curWriteData1Reg1 <= curWriteData1Reg0;
	curWriteAddr1Reg1 <= curWriteAddr1Reg0;
	curReadAddr1Reg1 <= curReadAddr1Reg0;
	curWriteByteEn1Reg1 <= curWriteByteEn1Reg0;
	curWriteEn1Reg1 <= curWriteEn1Reg0;
end
assign curWriteData0 = curWriteData0Reg1;
assign curWriteAddr0 = curWriteAddr0Reg1;
assign curReadAddr0 = curReadAddr0Reg1;
assign curWriteByteEn0 = curWriteByteEn0Reg1;
assign curWriteEn0 = curWriteEn0Reg1;
assign curWriteData1 = curWriteData1Reg1;
assign curWriteAddr1 = curWriteAddr1Reg1;
assign curReadAddr1 = curReadAddr1Reg1;
assign curWriteByteEn1 = curWriteByteEn1Reg1;
assign curWriteEn1 = curWriteEn1Reg1;

always @ (posedge clk)
begin
		curReadData0Reg0 <= curReadData0;
		curReadData1Reg0 <= curReadData1;
end
assign curReadDataMem = (curMemSel == 0) ? curReadData0Reg0 : curReadData1Reg0;
assign curReadDataLU = (curMemSel == 0) ? curReadData1Reg0 : curReadData0Reg0;
endmodule

module LUControl (clk, start_in, m_in, n_in, loop_in, mode_in, done,
					curReadAddr, curWriteAddr, curWriteByteEn, curWriteEn, curWriteSel, 
					leftReadAddr, leftWriteAddr, leftWriteByteEn, leftWriteEn,  leftWriteSel,
					topReadAddr, topWriteAddr, topWriteEn, topWriteSel, topSourceSel, diagEn, MOSel, MOEn);

input clk, start_in;
input[5-1:0] m_in, n_in, loop_in;
input[1:0] mode_in;
output done;

output[32-1:0] curWriteByteEn;
output[5-1:0] curWriteAddr, curReadAddr;
output curWriteEn;

output[32-1:0] leftWriteByteEn;
output[5-1:0] leftWriteAddr, leftReadAddr;
output leftWriteEn;

output[8-1:0] topWriteAddr, topReadAddr;
output topWriteEn;

output leftWriteSel, curWriteSel, topSourceSel, diagEn;
output[3-1:0] topWriteSel;

output MOSel;
output MOEn;

reg start;
reg[15:0]startDelay;
reg[5-1:0] m, n, stop, stop2, loop;
reg[1:0] mode;
reg[3:0] nextState, currentState;
reg[1:0] nextRowState, currentRowState;
reg startFetchRow, doneFetchRow, loadRow, writeRow;
reg updateCounter;

reg[5-1:0] i1, j;
reg[8-1:0] nextTopIdx, nextTopIdx2, curTopIdx, nextTopIdxCounter;
reg[2-1:0] topIdx, topIdxCounter, mdivk;
reg[5-1:0] diagIdx, leftIdx, msIdx;
reg[3-1:0] imodk, i1modk;
reg[5-1:0] diagIdxCounter, leftIdxCounter, msIdxCounter, readRowCounter, topWriteCounter;
reg[32-1:0] byteEn, i1modkByteEn;

reg done;

reg[32-1:0] curWriteByteEn;
reg[5-1:0] curWriteAddr, curReadAddr;
reg curWriteEn;

reg[32-1:0] leftWriteByteEn;
reg[5-1:0] leftWriteAddr, leftReadAddr;
reg leftWriteEn;

reg[8-1:0] topWriteAddr, topReadAddr;
reg topWriteEn;

reg leftWriteSel, curWriteSel, topSourceSel, diagEn;
reg[3-1:0] topWriteSel;

reg MOSel;
reg MOEn;

reg[5-1:0] counter;
reg[6-1:0] divCounter;

reg[32-1:0]writeByteEnDelay0; 
reg[32-1:0]writeByteEnDelay1; 
reg[32-1:0]writeByteEnDelay2; 
reg[32-1:0]writeByteEnDelay3; 
reg[32-1:0]writeByteEnDelay4; 
reg[32-1:0]writeByteEnDelay5; 
reg[32-1:0]writeByteEnDelay6; 
reg[32-1:0]writeByteEnDelay7; 
reg[32-1:0]writeByteEnDelay8; 
reg[32-1:0]writeByteEnDelay9; 
reg[32-1:0]writeByteEnDelay10; 
reg[32-1:0]writeByteEnDelay11; 
reg[32-1:0]writeByteEnDelay12; 
reg[32-1:0]writeByteEnDelay13; 
reg[32-1:0]writeByteEnDelay14; 
reg[32-1:0]writeByteEnDelay15; 
reg[32-1:0]writeByteEnDelay16; 
reg[32-1:0]writeByteEnDelay17; 
reg[32-1:0]writeByteEnDelay18; 
reg[32-1:0]writeByteEnDelay19; 
reg[32-1:0]writeByteEnDelay20; 
reg[32-1:0]writeByteEnDelay21; 
reg[32-1:0]writeByteEnDelay22; 
reg[32-1:0]writeByteEnDelay23; 
reg[32-1:0]writeByteEnDelay24; 
reg[32-1:0]writeByteEnDelay25; 
reg[32-1:0]writeByteEnDelay26; 
reg[32-1:0]writeByteEnDelay27; 
reg[32-1:0]writeByteEnDelay28; 
reg[32-1:0]writeByteEnDelay29; 
reg[32-1:0]writeByteEnDelay30; 
reg[32-1:0]writeByteEnDelay31; 

reg[5-1:0]curWriteAddrDelay0; 
reg[5-1:0]curWriteAddrDelay1; 
reg[5-1:0]curWriteAddrDelay2; 
reg[5-1:0]curWriteAddrDelay3; 
reg[5-1:0]curWriteAddrDelay4; 
reg[5-1:0]curWriteAddrDelay5; 
reg[5-1:0]curWriteAddrDelay6; 
reg[5-1:0]curWriteAddrDelay7; 
reg[5-1:0]curWriteAddrDelay8; 
reg[5-1:0]curWriteAddrDelay9; 
reg[5-1:0]curWriteAddrDelay10; 
reg[5-1:0]curWriteAddrDelay11; 
reg[5-1:0]curWriteAddrDelay12; 
reg[5-1:0]curWriteAddrDelay13; 
reg[5-1:0]curWriteAddrDelay14; 
reg[5-1:0]curWriteAddrDelay15; 
reg[5-1:0]curWriteAddrDelay16; 
reg[5-1:0]curWriteAddrDelay17; 
reg[5-1:0]curWriteAddrDelay18; 
reg[5-1:0]curWriteAddrDelay19; 
reg[5-1:0]curWriteAddrDelay20; 
reg[5-1:0]curWriteAddrDelay21; 
reg[5-1:0]curWriteAddrDelay22; 
reg[5-1:0]curWriteAddrDelay23; 
reg[5-1:0]curWriteAddrDelay24; 
reg[5-1:0]curWriteAddrDelay25; 
reg[5-1:0]curWriteAddrDelay26; 
reg[5-1:0]curWriteAddrDelay27; 
reg[5-1:0]curWriteAddrDelay28; 
reg[5-1:0]curWriteAddrDelay29; 
reg[5-1:0]curWriteAddrDelay30; 
reg[5-1:0]curWriteAddrDelay31; 

reg[5-1:0]curReadAddrDelay0; 
reg[5-1:0]curReadAddrDelay1; 
reg[5-1:0]curReadAddrDelay2; 
reg[5-1:0]curReadAddrDelay3; 
reg[5-1:0]curReadAddrDelay4; 
reg[5-1:0]curReadAddrDelay5; 
reg[5-1:0]curReadAddrDelay6; 
reg[5-1:0]curReadAddrDelay7; 
reg[5-1:0]curReadAddrDelay8; 
reg[5-1:0]curReadAddrDelay9; 
reg[5-1:0]curReadAddrDelay10; 
reg[5-1:0]curReadAddrDelay11; 

reg[32-1:0]leftWriteEnDelay; 
reg[32-1:0]curWriteEnDelay; 
reg[5-1:0]leftWriteSelDelay; 
reg[16-1:0]curWriteSelDelay; 
reg[5-1:0]leftReadAddrDelay0; 
reg[8-1:0]topWriteAddrDelay0; 
reg[8-1:0]topWriteAddrDelay1; 
reg[8-1:0]topWriteAddrDelay2; 
reg[8-1:0]topWriteAddrDelay3; 
reg[8-1:0]topWriteAddrDelay4; 
reg[8-1:0]topWriteAddrDelay5; 
reg[8-1:0]topWriteAddrDelay6; 
reg[8-1:0]topWriteAddrDelay7; 
reg[8-1:0]topWriteAddrDelay8; 
reg[8-1:0]topWriteAddrDelay9; 
reg[8-1:0]topWriteAddrDelay10; 
reg[8-1:0]topWriteAddrDelay11; 
reg[8-1:0]topWriteAddrDelay12; 
reg[8-1:0]topWriteAddrDelay13; 
reg[8-1:0]topWriteAddrDelay14; 
reg[8-1:0]topWriteAddrDelay15; 
reg[8-1:0]topWriteAddrDelay16; 
reg[8-1:0]topWriteAddrDelay17; 
reg[8-1:0]topWriteAddrDelay18; 
reg[8-1:0]topWriteAddrDelay19; 
reg[8-1:0]topWriteAddrDelay20; 
reg[8-1:0]topWriteAddrDelay21; 
reg[8-1:0]topWriteAddrDelay22; 
reg[8-1:0]topWriteAddrDelay23; 
reg[8-1:0]topWriteAddrDelay24; 
reg[8-1:0]topWriteAddrDelay25; 
reg[8-1:0]topWriteAddrDelay26; 
reg[8-1:0]topWriteAddrDelay27; 
reg[8-1:0]topWriteAddrDelay28; 
reg[8-1:0]topWriteAddrDelay29; 
reg[8-1:0]topWriteAddrDelay30; 
reg[8-1:0]topWriteAddrDelay31; 

reg [32-1:0]topWriteEnDelay;
reg [5-1:0]topSourceSelDelay;
reg[3-1:0]topWriteSelDelay0; 
reg[3-1:0]topWriteSelDelay1; 
reg[3-1:0]topWriteSelDelay2; 
reg[3-1:0]topWriteSelDelay3; 
reg[3-1:0]topWriteSelDelay4; 
reg[3-1:0]topWriteSelDelay5; 
reg[3-1:0]topWriteSelDelay6; 
reg[3-1:0]topWriteSelDelay7; 
reg[3-1:0]topWriteSelDelay8; 
reg[3-1:0]topWriteSelDelay9; 
reg[3-1:0]topWriteSelDelay10; 
reg[3-1:0]topWriteSelDelay11; 
reg[3-1:0]topWriteSelDelay12; 
reg[3-1:0]topWriteSelDelay13; 
reg[3-1:0]topWriteSelDelay14; 
reg[3-1:0]topWriteSelDelay15; 
reg[3-1:0]topWriteSelDelay16; 
reg[3-1:0]topWriteSelDelay17; 
reg[3-1:0]topWriteSelDelay18; 
reg[3-1:0]topWriteSelDelay19; 
reg[3-1:0]topWriteSelDelay20; 
reg[3-1:0]topWriteSelDelay21; 
reg[3-1:0]topWriteSelDelay22; 
reg[3-1:0]topWriteSelDelay23; 
reg[3-1:0]topWriteSelDelay24; 
reg[3-1:0]topWriteSelDelay25; 
reg[3-1:0]topWriteSelDelay26; 
reg[3-1:0]topWriteSelDelay27; 
reg[3-1:0]topWriteSelDelay28; 
reg[3-1:0]topWriteSelDelay29; 
reg[3-1:0]topWriteSelDelay30; 
reg[3-1:0]topWriteSelDelay31; 

reg [6-1:0]diagEnDelay;
reg[6-1:0]MOEnDelay;
reg [5-1:0]waitCycles;

// register store m, n and mdivk value
always @ (posedge clk)
begin
	if (start_in == 1'b1)
	begin
		n <= n_in;
		m <= m_in;
		loop <= loop_in;
		mode <= mode_in;
	end
	if (mode[0] == 1'b0 && m == loop)
		stop <= loop;
	else
		stop <= loop+1'b1;
	stop2 <= loop;
	startDelay[0] <= start_in;
	startDelay[1] <= startDelay[0];
	startDelay[2] <= startDelay[1];
	startDelay[3] <= startDelay[2];
	startDelay[4] <= startDelay[3];
	startDelay[5] <= startDelay[4];
	startDelay[6] <= startDelay[5];
	startDelay[7] <= startDelay[6];
	startDelay[8] <= startDelay[7];
	startDelay[9] <= startDelay[8];
	startDelay[10] <= startDelay[9];
	startDelay[11] <= startDelay[10];
	startDelay[12] <= startDelay[11];
	startDelay[13] <= startDelay[12];
	startDelay[14] <= startDelay[13];
	startDelay[15] <= startDelay[14];
	start <= startDelay[15];
	mdivk <= (m+8-1)>>3;
end

// registers that store values that are used in FSM, dependent on i and/or j
always @ (posedge clk)
begin
	if (start == 1'b1)
		topIdx <= 2'b00; //offset1divk;
	else if (currentState == `cINCRE_I && i1modk == 8-1 && mode[0] == 1'b0)
		topIdx <= topIdx + 1'b1;
		
	if (start == 1'b1)
		diagIdx <= 5'b00000;
	else if (currentState == `cSTORE_DIAG && mode == 2'b01)
		diagIdx <= 2;	else if (currentState == `cINCRE_I)
	begin
		if ((imodk == 8-1 && mode == 2'b00) || (i1modk == 8-1 && mode == 2'b01))
			diagIdx <= diagIdx + 2 + 1;
		else
			diagIdx <= diagIdx + 2;
	end
	
	if (start == 1'b1)
		leftIdx <= 5'b00000;
	else if (currentState == `cINCRE_I)
	begin
		if (i1modk == 8-1 && mode[0] == 1'b0)
			leftIdx <= leftIdx + 2 + 1;
		else
			leftIdx <= leftIdx + 2;
	end

	if (start == 1'b1)
		msIdx <= 5'b00000;
	else if (currentState == `cUPDATE_J)
		if (mode[1] == 1'b0)
			msIdx <= leftIdx + 2;
		else
			msIdx <= topIdx;
	else if (nextRowState == `cLOAD_ROW_INC_J)
		msIdx <= msIdx + 2;

	if (start == 1'b1)
		imodk <= 3'b000;
	else if (currentState == `cINCRE_I)
	begin
		if (imodk == 8-1)
		imodk <= 3'b000;
		else
			imodk <= imodk + 1'b1;
	end
	
	if (start == 1'b1)
		i1modk <= 3'b001;
	else if (currentState == `cINCRE_I)
	begin
		if (i1modk == 8-1)
		i1modk <= 3'b000;
		else
			i1modk <= i1modk + 1'b1;
	end
	
	if (start == 1'b1)
		nextTopIdx <= 8'b00000000;
	else if (currentState == `cINCRE_I)
		if (mode[1] == 0)
			nextTopIdx <= nextTopIdx + n + 1;
		else
			nextTopIdx <= nextTopIdx + n;
 nextTopIdx2 <= nextTopIdx + n + 1;

	if (start == 1'b1)
		curTopIdx <= 8'b00000001;
	else if (currentState == `cUPDATE_J)
   if (mode[1] == 1'b0)
		  curTopIdx <= nextTopIdx+1;
   else
		  curTopIdx <= nextTopIdx;
	else if (nextRowState == `cLOAD_ROW_INC_J)
		curTopIdx <= curTopIdx + 1;
	
	if (start == 1'b1)
		i1 <= 5'b00001;
	else if (currentState == `cINCRE_I)
	   i1 <= i1 + 1;

	if (start == 1'b1)
		j <= 5'b00000;
	else if (currentState == `cUPDATE_J)
		if (mode[1] == 1'b0)
			j <= i1;
		else
		j <= 5'b00000;
	else if (currentRowState == `cLOAD_ROW_INC_J)
		j <= j + 1;

// compute cycles of delay in FSM
	if (currentState == `cSTORE_MO)
		waitCycles <= 32-1;
	else if (currentState == `cINCRE_I)
	begin
		if (i1 == stop-1)
			if (mode[1] == 1'b1)
				waitCycles <= 32-1 + 6 - 3;
			else
				waitCycles <= waitCycles + 5 - 2;
		else if (mode == 2'b01 && waitCycles < 32-1 - (16-1) - 4)
			waitCycles <= 32-1 - (16-1) - 4;
		else if (mode == 2'b10 && i1modk == 8-1)
			waitCycles <= 32-1 + 6 - 3;
		else if (mode == 2'b00)
			waitCycles <= waitCycles + 6 ;
	end
else if (waitCycles >5'b00000)
		waitCycles <= waitCycles - 1;

end

// determining next state of main FSM
always @ (currentState or start or mode or m or n or counter or mdivk or topIdxCounter or doneFetchRow or divCounter or j or stop2 or waitCycles or stop or i1)
begin
	case (currentState)
	`cSETUP:
	begin
		if (start == 1'b1)
			nextState = `cSTART;
		else
			nextState = `cSETUP;
		updateCounter = 1'b1;
	end
	`cSTART:
	begin
		if (mode == 2'b00)
		begin
			if (m == 1 && n == 1)
				nextState = `cDONE;
			else
				nextState = `cFETCH_COL;
		end
		else if (mode == 2'b01)
			nextState = `cSTORE_DIAG;
		else if (mode == 2'b10)
			nextState = `cSTART_FETCH_ROW;
		else
			nextState = `cUPDATE_J;
		updateCounter = 1'b1;
	end
	`cSTART_FETCH_ROW:
	begin
		if (counter == 5+6-1)
   begin
		  if (mode == 2'b00)
			  nextState = `cSTORE_DIAG;
		  else
			  nextState = `cUPDATE_J;
	  end
		else
			nextState = `cSTART_FETCH_ROW;
		updateCounter = 1'b0;
	end
	`cFETCH_COL:
		if (counter >= mdivk-1)
		begin
			if (mode == 2'b00 && counter < 5)
			begin
				nextState = `cWAIT_COL;
				updateCounter = 1'b0;
			end
			else
			begin
				if (mode == 2'b00)
					nextState = `cSTART_FETCH_ROW;
				else
					nextState = `cFIND_REC;
				updateCounter = 1'b1;
			end
		end
		else
		begin
			nextState = `cFETCH_COL;
			updateCounter = 1'b0;
		end
	`cWAIT_COL:
		if (counter >= 5)
		begin
			if (mode == 0)
				nextState = `cSTART_FETCH_ROW;
			else
				nextState = `cFIND_REC;
			updateCounter = 1;
		end
		else
		begin
			nextState = `cWAIT_COL;
			updateCounter = 0;
		end
	`cSTORE_DIAG:
	begin
		if (mode == 0)
			nextState =  `cFIND_REC;
		else
			nextState =  `cFETCH_COL;
		updateCounter = 1;
	end
	`cFIND_REC:
		if (divCounter == 56)
		begin
			if (mode == 0)
				nextState = `cMULT_COL;
			else
				nextState = `cSTORE_DIAG2;
			updateCounter = 1;
		end
		else
		begin
			nextState = `cFIND_REC;
			updateCounter = 0;
		end
	`cSTORE_DIAG2:
	begin
		nextState =  `cMULT_COL;
		updateCounter = 1;
	end
	`cMULT_COL:
		if (topIdxCounter == mdivk-1)
		begin
			nextState = `cUPDATE_J;
			updateCounter = 0;
		end
		else
		begin
			nextState = `cMULT_COL;
			updateCounter = 0;
		end
	`cUPDATE_J:
		if ((mode[1] == 1 || counter >= 16-1) && doneFetchRow == 1)
		begin
			nextState = `cSTORE_MO;
			updateCounter = 1;
		end
		else
		begin
			nextState = `cUPDATE_J;
			updateCounter = 0;
		end
	`cSTORE_MO:
	begin
		if (j == stop2)
		begin
			if (counter == mdivk-1+5-2)
				nextState = `cDONE;
			else
				nextState = `cSTORE_MO;
			updateCounter = 0;
		end
		else
		begin
			nextState =  `cMULT_SUB;
			updateCounter = 1;
		end
	end
	`cMULT_SUB:
		if (topIdxCounter == mdivk-1)
		begin
			if (j == n-1)
				nextState = `cINCRE_I;
			else
				nextState = `cMULT_SUB;
			updateCounter = 1;
		end
		else
		begin
			nextState = `cMULT_SUB;
			updateCounter = 0;
		end
	`cINCRE_I:
	begin
		nextState = `cWAIT;
		updateCounter = 1;
	end
	`cWAIT:
		if (waitCycles == 0)
		begin
			if (i1 == stop)
				nextState = `cDONE;
			else if (mode == 0)
				nextState = `cSTORE_DIAG;
			else if (mode == 1)
				nextState = `cFIND_REC;
			else
				nextState = `cUPDATE_J;
			updateCounter = 1;
		end
		else
		begin
			nextState = `cWAIT;
			updateCounter = 0;
		end
	`cDONE:
	begin
		nextState = `cDONE;
		updateCounter = 0;
	end
	default:
	begin
		nextState = `cSETUP;
		updateCounter = 1;
	end
	endcase
end

always @ (currentRowState or currentState or nextState or i1 or topIdxCounter or mdivk or msIdxCounter or readRowCounter or j or n or mode)
begin
	if (currentRowState == `cDONE_FETCH_ROW)
		doneFetchRow = 1;
	else
		doneFetchRow = 0;
		if((nextState == `cSTART_FETCH_ROW && currentState != `cSTART_FETCH_ROW && i1 == 1))
		startFetchRow = 1;
	else
		startFetchRow = 0;
	if (currentState == `cMULT_SUB && topIdxCounter+2 == mdivk)
		loadRow = 1;
	else
		loadRow = 0;
	writeRow = (msIdxCounter == readRowCounter)&&(currentState==`cMULT_SUB)&&(j!=n)&&(mode[0] == 0);
end

// second FSM that controls the control signals to temp_top block
always @ (currentRowState or nextTopIdxCounter or n or startFetchRow or loadRow or topIdx or mdivk or nextState)
begin
	case (currentRowState)
	`cFETCH_ROW:
		if (nextTopIdxCounter == n-1)
			nextRowState = `cDONE_FETCH_ROW;
		else
			nextRowState = `cFETCH_ROW;
	`cDONE_FETCH_ROW:
		if (startFetchRow == 1)
			nextRowState = `cFETCH_ROW;
		else if (loadRow == 1 || (topIdx+1 == mdivk && nextState == `cMULT_SUB))
			nextRowState = `cLOAD_ROW_INC_J;
		else
			nextRowState = `cDONE_FETCH_ROW;
	`cLOAD_ROW_INC_J:
		if (topIdx+1 == mdivk && nextState == `cMULT_SUB)
			nextRowState = `cLOAD_ROW_INC_J;
		else
			nextRowState = `cDONE_FETCH_ROW;
	default:
		nextRowState = `cDONE_FETCH_ROW;
	endcase
end

// address counters
always @ (posedge clk)
begin
	if (updateCounter == 1 || currentRowState == `cLOAD_ROW_INC_J)
		topIdxCounter <= topIdx;
	else
		topIdxCounter <= topIdxCounter + 1;

	if (updateCounter == 1)
		diagIdxCounter <= diagIdx;
	else
		diagIdxCounter <= diagIdxCounter + 1;

	if (updateCounter == 1 || currentRowState == `cLOAD_ROW_INC_J)
		msIdxCounter <= msIdx;
	else
		msIdxCounter <= msIdxCounter + 1;

	if (updateCounter == 1 || currentRowState == `cLOAD_ROW_INC_J)
		leftIdxCounter <= leftIdx;
	else
		leftIdxCounter <= leftIdxCounter + 1;
	
	if (currentState == `cFETCH_COL || currentState == `cSTORE_MO)
		topWriteCounter <= i1;
	else if (writeRow == 1 || currentRowState == `cFETCH_ROW)
		topWriteCounter <= topWriteCounter + 1;

	if (currentState == `cSTART)
		nextTopIdxCounter <= nextTopIdx;
 else if (currentState == `cSTORE_MO)
		if (mode[1] == 0)
			nextTopIdxCounter <= nextTopIdx + n + 1;
		else
			nextTopIdxCounter <= nextTopIdx + n;
	else if (writeRow == 1 || currentRowState == `cFETCH_ROW)
		nextTopIdxCounter <= nextTopIdxCounter + 1;

	if (currentState == `cSTART)
			readRowCounter <= 0; //offsetdivk;
	else if (currentState == `cSTORE_MO)
		if (mode[1] == 0)
			readRowCounter <= leftIdx + 2;
		else
			readRowCounter <= topIdx;
	else if (writeRow == 1 || currentRowState == `cFETCH_ROW)
	   readRowCounter <= readRowCounter + 2;

	if (updateCounter == 1)
		counter <= 0;
	else
		counter <= counter + 1;

	if (currentState == `cSTORE_DIAG || currentState == `cSTORE_DIAG2)
		divCounter <= 0;
	else if (divCounter < 56)
		divCounter <= divCounter + 1;

	case (i1modk) 
		3'b000: begin
			i1modkByteEn <= ~(32'b0) >> (3'b000<<2'b10);
		end
		3'b001: begin
			i1modkByteEn <= ~(32'b0) >> (3'b001<<2'b10);
		end
		3'b010: begin
			i1modkByteEn <= ~(32'b0) >> (3'b010<<2'b10);
		end
		3'b011: begin
			i1modkByteEn <= ~(32'b0) >> (3'b011<<2'b10);
		end
		3'b100: begin
			i1modkByteEn <= ~(32'b0) >> (3'b100<<2'b10);
		end
		3'b101: begin
			i1modkByteEn <= ~(32'b0) >> (3'b101<<2'b10);
		end
		3'b110: begin
			i1modkByteEn <= ~(32'b0) >> (3'b110<<2'b10);
		end
		3'b111: begin
			i1modkByteEn <= ~(32'b0) >> (3'b111<<2'b10);
		end
		default: begin
			i1modkByteEn <= ~(32'b0);
		end
	endcase
end

// compute Byte Enable
always @ (posedge clk)
begin
	if ((nextState == `cMULT_COL && currentState != `cMULT_COL) || (currentState == `cSTORE_MO) || currentRowState == `cLOAD_ROW_INC_J)
		byteEn <= i1modkByteEn;
	else
		byteEn <= 32'b11111111111111111111111111111111;
end

// update FSM state register
always @ (posedge clk)
begin
	if (start_in == 1'b1)
		currentState <= `cSETUP;
	else
		currentState <= nextState;
	if (start == 1'b1)
		currentRowState <= `cDONE_FETCH_ROW;
	else
		currentRowState <= nextRowState;
end

// delay register for control signals
// control signals are delayed to match latency of operations and/or memory access
always @ (posedge clk)
begin
	curReadAddrDelay0 <= curReadAddrDelay1;
	curReadAddrDelay1 <= curReadAddrDelay2;
	curReadAddrDelay2 <= curReadAddrDelay3;
	curReadAddrDelay3 <= curReadAddrDelay4;
	curReadAddrDelay4 <= curReadAddrDelay5;
	curReadAddrDelay5 <= curReadAddrDelay6;
	curReadAddrDelay6 <= curReadAddrDelay7;
	curReadAddrDelay7 <= curReadAddrDelay8;
	curReadAddrDelay8 <= curReadAddrDelay9;
	curReadAddrDelay9 <= curReadAddrDelay10;
	curReadAddrDelay10 <= curReadAddrDelay11;
	curReadAddrDelay11 <= msIdxCounter;
	
	curWriteAddrDelay0 <= curWriteAddrDelay1;
	curWriteAddrDelay1 <= curWriteAddrDelay2;
	curWriteAddrDelay2 <= curWriteAddrDelay3;
	curWriteAddrDelay3 <= curWriteAddrDelay4;
	if (currentState == `cFETCH_COL)
		curWriteAddrDelay4 <= diagIdxCounter;
	else
		curWriteAddrDelay4 <= curWriteAddrDelay5;
	curWriteAddrDelay5 <= curWriteAddrDelay6;
	curWriteAddrDelay6 <= curWriteAddrDelay7;
	curWriteAddrDelay7 <= curWriteAddrDelay8;
	curWriteAddrDelay8 <= curWriteAddrDelay9;
	curWriteAddrDelay9 <= curWriteAddrDelay10;
	curWriteAddrDelay10 <= curWriteAddrDelay11;
	curWriteAddrDelay11 <= curWriteAddrDelay12;
	curWriteAddrDelay12 <= curWriteAddrDelay13;
	curWriteAddrDelay13 <= curWriteAddrDelay14;
	curWriteAddrDelay14 <= curWriteAddrDelay15;
	if (currentState == `cMULT_COL)
		curWriteAddrDelay15 <= leftIdxCounter;
	else
		curWriteAddrDelay15 <= curWriteAddrDelay16;
	curWriteAddrDelay16 <= curWriteAddrDelay17;
	curWriteAddrDelay17 <= curWriteAddrDelay18;
	curWriteAddrDelay18 <= curWriteAddrDelay19;
	curWriteAddrDelay19 <= curWriteAddrDelay20;
	curWriteAddrDelay20 <= curWriteAddrDelay21;
	curWriteAddrDelay21 <= curWriteAddrDelay22;
	curWriteAddrDelay22 <= curWriteAddrDelay23;
	curWriteAddrDelay23 <= curWriteAddrDelay24;
	curWriteAddrDelay24 <= curWriteAddrDelay25;
	curWriteAddrDelay25 <= curWriteAddrDelay26;
	curWriteAddrDelay26 <= curWriteAddrDelay27;
	curWriteAddrDelay27 <= curWriteAddrDelay28;
	curWriteAddrDelay28 <= curWriteAddrDelay29;
	curWriteAddrDelay29 <= curWriteAddrDelay30;
	curWriteAddrDelay30 <= curWriteAddrDelay31;
	curWriteAddrDelay31 <= msIdxCounter;
	
	writeByteEnDelay0 <= writeByteEnDelay1;
	writeByteEnDelay1 <= writeByteEnDelay2;
	writeByteEnDelay2 <= writeByteEnDelay3;
	writeByteEnDelay3 <= writeByteEnDelay4;
	if (mode[0] == 1'b1)
		writeByteEnDelay4 <= ~0;
	else if (currentState == `cFETCH_COL)
		writeByteEnDelay4 <= byteEn;
	else
		writeByteEnDelay4 <= writeByteEnDelay5;
	writeByteEnDelay5 <= writeByteEnDelay6;
	writeByteEnDelay6 <= writeByteEnDelay7;
	writeByteEnDelay7 <= writeByteEnDelay8;
	writeByteEnDelay8 <= writeByteEnDelay9;
	writeByteEnDelay9 <= writeByteEnDelay10;
	writeByteEnDelay10 <= writeByteEnDelay11;
	writeByteEnDelay11 <= writeByteEnDelay12;
	writeByteEnDelay12 <= writeByteEnDelay13;
	writeByteEnDelay13 <= writeByteEnDelay14;
	writeByteEnDelay14 <= writeByteEnDelay15;
	if (currentState == `cMULT_COL)
		writeByteEnDelay15 <= byteEn;
	else
		writeByteEnDelay15 <= writeByteEnDelay16;
	writeByteEnDelay16 <= writeByteEnDelay17;
	writeByteEnDelay17 <= writeByteEnDelay18;
	writeByteEnDelay18 <= writeByteEnDelay19;
	writeByteEnDelay19 <= writeByteEnDelay20;
	writeByteEnDelay20 <= writeByteEnDelay21;
	writeByteEnDelay21 <= writeByteEnDelay22;
	writeByteEnDelay22 <= writeByteEnDelay23;
	writeByteEnDelay23 <= writeByteEnDelay24;
	writeByteEnDelay24 <= writeByteEnDelay25;
	writeByteEnDelay25 <= writeByteEnDelay26;
	writeByteEnDelay26 <= writeByteEnDelay27;
	writeByteEnDelay27 <= writeByteEnDelay28;
	writeByteEnDelay28 <= writeByteEnDelay29;
	writeByteEnDelay29 <= writeByteEnDelay30;
	writeByteEnDelay30 <= writeByteEnDelay31;
	writeByteEnDelay31 <= byteEn;
	
	curWriteSelDelay[0] <= curWriteSelDelay[1];
	curWriteSelDelay[1] <= curWriteSelDelay[2];
	curWriteSelDelay[2] <= curWriteSelDelay[3];
	curWriteSelDelay[3] <= curWriteSelDelay[4];
	curWriteSelDelay[4] <= curWriteSelDelay[5];
	curWriteSelDelay[5] <= curWriteSelDelay[6];
	curWriteSelDelay[6] <= curWriteSelDelay[7];
	curWriteSelDelay[7] <= curWriteSelDelay[8];
	curWriteSelDelay[8] <= curWriteSelDelay[9];
	curWriteSelDelay[9] <= curWriteSelDelay[10];
	curWriteSelDelay[10] <= curWriteSelDelay[11];
	curWriteSelDelay[11] <= curWriteSelDelay[12];
	curWriteSelDelay[12] <= curWriteSelDelay[13];
	curWriteSelDelay[13] <= curWriteSelDelay[14];
	curWriteSelDelay[14] <= curWriteSelDelay[15];
	if (currentState == `cMULT_COL)
		curWriteSelDelay[15] <= 1'b0;
	else
		curWriteSelDelay[15] <= 1'b1;

	curWriteEnDelay[0] <= curWriteEnDelay[1];
	curWriteEnDelay[1] <= curWriteEnDelay[2];
	curWriteEnDelay[2] <= curWriteEnDelay[3];
	curWriteEnDelay[3] <= curWriteEnDelay[4];
	curWriteEnDelay[4] <= curWriteEnDelay[5];
	curWriteEnDelay[5] <= curWriteEnDelay[6];
	curWriteEnDelay[6] <= curWriteEnDelay[7];
	curWriteEnDelay[7] <= curWriteEnDelay[8];
	curWriteEnDelay[8] <= curWriteEnDelay[9];
	curWriteEnDelay[9] <= curWriteEnDelay[10];
	curWriteEnDelay[10] <= curWriteEnDelay[11];
	curWriteEnDelay[11] <= curWriteEnDelay[12];
	curWriteEnDelay[12] <= curWriteEnDelay[13];
	curWriteEnDelay[13] <= curWriteEnDelay[14];
	curWriteEnDelay[14] <= curWriteEnDelay[15];
	if (currentState == `cMULT_COL)
		curWriteEnDelay[15] <= 1'b1;
	else
		curWriteEnDelay[15] <= curWriteEnDelay[16];
	curWriteEnDelay[16] <= curWriteEnDelay[17];
	curWriteEnDelay[17] <= curWriteEnDelay[18];
	curWriteEnDelay[18] <= curWriteEnDelay[19];
	curWriteEnDelay[19] <= curWriteEnDelay[20];
	curWriteEnDelay[20] <= curWriteEnDelay[21];
	curWriteEnDelay[21] <= curWriteEnDelay[22];
	curWriteEnDelay[22] <= curWriteEnDelay[23];
	curWriteEnDelay[23] <= curWriteEnDelay[24];
	curWriteEnDelay[24] <= curWriteEnDelay[25];
	curWriteEnDelay[25] <= curWriteEnDelay[26];
	curWriteEnDelay[26] <= curWriteEnDelay[27];
	curWriteEnDelay[27] <= curWriteEnDelay[28];
	curWriteEnDelay[28] <= curWriteEnDelay[29];
	curWriteEnDelay[29] <= curWriteEnDelay[30];
	curWriteEnDelay[30] <= curWriteEnDelay[31];
	if (currentState == `cMULT_SUB)
		curWriteEnDelay[31] <= 1'b1;
	else
		curWriteEnDelay[31] <= 1'b0;

	leftWriteSelDelay[0] <= leftWriteSelDelay[1];
	leftWriteSelDelay[1] <= leftWriteSelDelay[2];
	leftWriteSelDelay[2] <= leftWriteSelDelay[3];
	leftWriteSelDelay[3] <= leftWriteSelDelay[4];
	if (currentState == `cFETCH_COL)
		leftWriteSelDelay[4] <= 1'b0;
	else
		leftWriteSelDelay[4] <= 1'b1;

	leftWriteEnDelay[0] <= leftWriteEnDelay[1];
	leftWriteEnDelay[1] <= leftWriteEnDelay[2];
	leftWriteEnDelay[2] <= leftWriteEnDelay[3];
	leftWriteEnDelay[3] <= leftWriteEnDelay[4];
	if (currentState == `cFETCH_COL)
		leftWriteEnDelay[4] <= 1'b1;
	else
		leftWriteEnDelay[4] <= leftWriteEnDelay[5];
	leftWriteEnDelay[5] <= leftWriteEnDelay[6];
	leftWriteEnDelay[6] <= leftWriteEnDelay[7];
	leftWriteEnDelay[7] <= leftWriteEnDelay[8];
	leftWriteEnDelay[8] <= leftWriteEnDelay[9];
	leftWriteEnDelay[9] <= leftWriteEnDelay[10];
	leftWriteEnDelay[10] <= leftWriteEnDelay[11];
	leftWriteEnDelay[11] <= leftWriteEnDelay[12];
	leftWriteEnDelay[12] <= leftWriteEnDelay[13];
	leftWriteEnDelay[13] <= leftWriteEnDelay[14];
	leftWriteEnDelay[14] <= leftWriteEnDelay[15];
	if (currentState == `cMULT_COL)
		leftWriteEnDelay[15] <= 1'b1;
	else
		leftWriteEnDelay[15] <= leftWriteEnDelay[16];
	leftWriteEnDelay[16] <= leftWriteEnDelay[17];
	leftWriteEnDelay[17] <= leftWriteEnDelay[18];
	leftWriteEnDelay[18] <= leftWriteEnDelay[19];
	leftWriteEnDelay[19] <= leftWriteEnDelay[20];
	leftWriteEnDelay[20] <= leftWriteEnDelay[21];
	leftWriteEnDelay[21] <= leftWriteEnDelay[22];
	leftWriteEnDelay[22] <= leftWriteEnDelay[23];
	leftWriteEnDelay[23] <= leftWriteEnDelay[24];
	leftWriteEnDelay[24] <= leftWriteEnDelay[25];
	leftWriteEnDelay[25] <= leftWriteEnDelay[26];
	leftWriteEnDelay[26] <= leftWriteEnDelay[27];
	leftWriteEnDelay[27] <= leftWriteEnDelay[28];
	leftWriteEnDelay[28] <= leftWriteEnDelay[29];
	leftWriteEnDelay[29] <= leftWriteEnDelay[30];
	leftWriteEnDelay[30] <= leftWriteEnDelay[31];
	if (currentState == `cMULT_SUB && (mode == 0 || (mode == 1 && j == i1)))
		leftWriteEnDelay[31] <= 1'b1;
	else
		leftWriteEnDelay[31] <= 1'b0;

	topWriteAddrDelay0 <= topWriteAddrDelay1;
	topWriteAddrDelay1 <= topWriteAddrDelay2;
	topWriteAddrDelay2 <= topWriteAddrDelay3;
	topWriteAddrDelay3 <= topWriteAddrDelay4;
	if (currentRowState == `cFETCH_ROW)
		topWriteAddrDelay4 <= nextTopIdxCounter;
	else
		topWriteAddrDelay4 <=  topWriteAddrDelay5;
	topWriteAddrDelay5 <= topWriteAddrDelay6;
	topWriteAddrDelay6 <= topWriteAddrDelay7;
	topWriteAddrDelay7 <= topWriteAddrDelay8;
	topWriteAddrDelay8 <= topWriteAddrDelay9;
	topWriteAddrDelay9 <= topWriteAddrDelay10;
	topWriteAddrDelay10 <= topWriteAddrDelay11;
	topWriteAddrDelay11 <= topWriteAddrDelay12;
	topWriteAddrDelay12 <= topWriteAddrDelay13;
	topWriteAddrDelay13 <= topWriteAddrDelay14;
	topWriteAddrDelay14 <= topWriteAddrDelay15;
	topWriteAddrDelay15 <= topWriteAddrDelay16;
	topWriteAddrDelay16 <= topWriteAddrDelay17;
	topWriteAddrDelay17 <= topWriteAddrDelay18;
	topWriteAddrDelay18 <= topWriteAddrDelay19;
	topWriteAddrDelay19 <= topWriteAddrDelay20;
	topWriteAddrDelay20 <= topWriteAddrDelay21;
	topWriteAddrDelay21 <= topWriteAddrDelay22;
	topWriteAddrDelay22 <= topWriteAddrDelay23;
	topWriteAddrDelay23 <= topWriteAddrDelay24;
	topWriteAddrDelay24 <= topWriteAddrDelay25;
	topWriteAddrDelay25 <= topWriteAddrDelay26;
	topWriteAddrDelay26 <= topWriteAddrDelay27;
	topWriteAddrDelay27 <= topWriteAddrDelay28;
	topWriteAddrDelay28 <= topWriteAddrDelay29;
	topWriteAddrDelay29 <= topWriteAddrDelay30;
	topWriteAddrDelay30 <= topWriteAddrDelay31;
		topWriteAddrDelay31 <= nextTopIdxCounter;

	topWriteEnDelay[0] <= topWriteEnDelay[1];
	topWriteEnDelay[1] <= topWriteEnDelay[2];
	topWriteEnDelay[2] <= topWriteEnDelay[3];
	topWriteEnDelay[3] <= topWriteEnDelay[4];
	if (currentRowState == `cFETCH_ROW)
		topWriteEnDelay[4] <= 1'b1;
	else
		topWriteEnDelay[4] <=  topWriteEnDelay[5];
	topWriteEnDelay[5] <= topWriteEnDelay[6];
	topWriteEnDelay[6] <= topWriteEnDelay[7];
	topWriteEnDelay[7] <= topWriteEnDelay[8];
	topWriteEnDelay[8] <= topWriteEnDelay[9];
	topWriteEnDelay[9] <= topWriteEnDelay[10];
	topWriteEnDelay[10] <= topWriteEnDelay[11];
	topWriteEnDelay[11] <= topWriteEnDelay[12];
	topWriteEnDelay[12] <= topWriteEnDelay[13];
	topWriteEnDelay[13] <= topWriteEnDelay[14];
	topWriteEnDelay[14] <= topWriteEnDelay[15];
	topWriteEnDelay[15] <= topWriteEnDelay[16];
	topWriteEnDelay[16] <= topWriteEnDelay[17];
	topWriteEnDelay[17] <= topWriteEnDelay[18];
	topWriteEnDelay[18] <= topWriteEnDelay[19];
	topWriteEnDelay[19] <= topWriteEnDelay[20];
	topWriteEnDelay[20] <= topWriteEnDelay[21];
	topWriteEnDelay[21] <= topWriteEnDelay[22];
	topWriteEnDelay[22] <= topWriteEnDelay[23];
	topWriteEnDelay[23] <= topWriteEnDelay[24];
	topWriteEnDelay[24] <= topWriteEnDelay[25];
	topWriteEnDelay[25] <= topWriteEnDelay[26];
	topWriteEnDelay[26] <= topWriteEnDelay[27];
	topWriteEnDelay[27] <= topWriteEnDelay[28];
	topWriteEnDelay[28] <= topWriteEnDelay[29];
	topWriteEnDelay[29] <= topWriteEnDelay[30];
	topWriteEnDelay[30] <= topWriteEnDelay[31];
	topWriteEnDelay[31] <= writeRow;

	topWriteSelDelay0 <= topWriteSelDelay1;
	topWriteSelDelay1 <= topWriteSelDelay2;
	topWriteSelDelay2 <= topWriteSelDelay3;
	topWriteSelDelay3 <= topWriteSelDelay4;
	if (currentRowState == `cFETCH_ROW || currentState == `cUPDATE_J && i1 == 1)
		topWriteSelDelay4 <= imodk;
	else
		topWriteSelDelay4 <=  topWriteSelDelay5;
	topWriteSelDelay5 <= topWriteSelDelay6;
	topWriteSelDelay6 <= topWriteSelDelay7;
	topWriteSelDelay7 <= topWriteSelDelay8;
	topWriteSelDelay8 <= topWriteSelDelay9;
	topWriteSelDelay9 <= topWriteSelDelay10;
	topWriteSelDelay10 <= topWriteSelDelay11;
	topWriteSelDelay11 <= topWriteSelDelay12;
	topWriteSelDelay12 <= topWriteSelDelay13;
	topWriteSelDelay13 <= topWriteSelDelay14;
	topWriteSelDelay14 <= topWriteSelDelay15;
	topWriteSelDelay15 <= topWriteSelDelay16;
	topWriteSelDelay16 <= topWriteSelDelay17;
	topWriteSelDelay17 <= topWriteSelDelay18;
	topWriteSelDelay18 <= topWriteSelDelay19;
	topWriteSelDelay19 <= topWriteSelDelay20;
	topWriteSelDelay20 <= topWriteSelDelay21;
	topWriteSelDelay21 <= topWriteSelDelay22;
	topWriteSelDelay22 <= topWriteSelDelay23;
	topWriteSelDelay23 <= topWriteSelDelay24;
	topWriteSelDelay24 <= topWriteSelDelay25;
	topWriteSelDelay25 <= topWriteSelDelay26;
	topWriteSelDelay26 <= topWriteSelDelay27;
	topWriteSelDelay27 <= topWriteSelDelay28;
	topWriteSelDelay28 <= topWriteSelDelay29;
	topWriteSelDelay29 <= topWriteSelDelay30;
	topWriteSelDelay30 <= topWriteSelDelay31;
	topWriteSelDelay31 <= i1modk;

	topSourceSelDelay[0] <= topSourceSelDelay[1];
	topSourceSelDelay[1] <= topSourceSelDelay[2];
	topSourceSelDelay[2] <= topSourceSelDelay[3];
	topSourceSelDelay[3] <= topSourceSelDelay[4];
	if (start == 1'b1)
		topSourceSelDelay[4] <= 1'b0;
	else if (currentState == `cSTORE_MO)
		topSourceSelDelay[4] <= 1'b1;

	leftReadAddrDelay0 <= leftIdxCounter;


	diagEnDelay[0] <= diagEnDelay[1];
	diagEnDelay[1] <= diagEnDelay[2];
	diagEnDelay[2] <= diagEnDelay[3];
	diagEnDelay[3] <= diagEnDelay[4];
	diagEnDelay[4] <= diagEnDelay[5];
	diagEnDelay[5] <= (currentState == `cSTORE_DIAG || currentState == `cSTORE_DIAG2);

	MOEnDelay[0] <= MOEnDelay[1];
	MOEnDelay[1] <= MOEnDelay[2];
	MOEnDelay[2] <= MOEnDelay[3];
	MOEnDelay[3] <= MOEnDelay[4];
	MOEnDelay[4] <= MOEnDelay[5];
	if (currentState == `cSTORE_MO || currentRowState == `cLOAD_ROW_INC_J)
		MOEnDelay[5] <= 1'b1;
	else
		MOEnDelay[5] <= 1'b0;
end

// output contorl signals
always @ (posedge clk)
begin
	if (currentState == `cFETCH_COL)
		curReadAddr <= diagIdxCounter;
	else if (currentRowState == `cFETCH_ROW)
	   curReadAddr <= readRowCounter;
	else
		curReadAddr <= curReadAddrDelay0;
	curWriteAddr <= curWriteAddrDelay0;
	curWriteByteEn <= writeByteEnDelay0;
	curWriteSel <= curWriteSelDelay;
	curWriteEn <= curWriteEnDelay;

	if (currentState == `cMULT_COL)
		leftReadAddr <= leftIdxCounter;
	else
		leftReadAddr <= leftReadAddrDelay0;
	leftWriteAddr <= curWriteAddrDelay0;
	leftWriteByteEn <= writeByteEnDelay0;
	leftWriteSel <= leftWriteSelDelay;
	leftWriteEn <= leftWriteEnDelay;

	if (currentState == `cSTORE_DIAG)
		topReadAddr <= nextTopIdx;
else if (currentState == `cSTORE_DIAG2)
   topReadAddr <= nextTopIdx2;
	else
	topReadAddr <= curTopIdx;
	topWriteAddr <= topWriteAddrDelay0;
	topWriteEn <= topWriteEnDelay;
	topWriteSel <= topWriteSelDelay0;
	topSourceSel <= topSourceSelDelay;

	MOSel <= ~(currentState == `cFIND_REC);
if (currentState == `cFIND_REC)
		MOEn <= 1'b1;
	else
		MOEn <= MOEnDelay;

	diagEn <= diagEnDelay;

	if (currentState == `cDONE)
		done <= 1'b1;
	else
		done <= 1'b0;
end

endmodule

module ram (
	byteena_a,
	clk,
	data,
	rdaddress,
	wraddress,
	wren,
	q
	);

	input	[`RAMNUMBYTES-1:0]  byteena_a;
	input	  clk;
	input	[`RAMWIDTH-1:0]  data;
	input	[`rRAMSIZEWIDTH-1:0]  rdaddress;
	input	[`rRAMSIZEWIDTH-1:0]  wraddress;
	input	  wren;
	output	[`RAMWIDTH-1:0]  q;
	wire	[`RAMWIDTH-1:0]  value_out;
	wire [`RAMWIDTH-1:0] subwire;
	assign q = subwire | dummy;
	wire [`RAMWIDTH-1:0] uselessdata;
 assign uselessdata = 256'b0;
wire j;
assign j = |byteena_a;
 wire [`RAMWIDTH-1:0]dummy;
 assign dummy = value_out & 256'b0;
dual_port_ram inst1( 
.clk (clk),
.we1(wren),
.we2(1'b0),
.data1(data),
.data2(uselessdata),
.out1(value_out),
.out2(subwire),
.addr1(wraddress),
.addr2(rdaddress));


endmodule

module ram1 (
	byteena_a,
	clk,
	data,
	rdaddress,
	wraddress,
	wren,
	q
	);

	input	[`RAMNUMBYTES-1:0]  byteena_a;
	input	  clk;
	input	[`RAMWIDTH-1:0]  data;
	input	[`rRAMSIZEWIDTH-1:0]  rdaddress;
	input	[`rRAMSIZEWIDTH-1:0]  wraddress;
	input	  wren;
	output	[`RAMWIDTH-1:0]  q;
	wire	[`RAMWIDTH-1:0]  value_out;
	wire [`RAMWIDTH-1:0] subwire;
	assign q = subwire | dummy;
	wire [`RAMWIDTH-1:0] uselessdata;
 assign uselessdata = 256'b0;
wire j;
assign j = |byteena_a;
 wire [`RAMWIDTH-1:0]dummy;
 assign dummy = value_out & 256'b0;
dual_port_ram inst1( 
.clk (clk),
.we1(wren),
.we2(1'b0),
.data1(data),
.data2(uselessdata),
.out1(value_out),
.out2(subwire),
.addr1(wraddress),
.addr2(rdaddress));


endmodule

module ram2 (
	byteena_a,
	clk,
	data,
	rdaddress,
	wraddress,
	wren,
	q
	);

	input	[`RAMNUMBYTES-1:0]  byteena_a;
	input	  clk;
	input	[`RAMWIDTH-1:0]  data;
	input	[`rRAMSIZEWIDTH-1:0]  rdaddress;
	input	[`rRAMSIZEWIDTH-1:0]  wraddress;
	input	  wren;
	output	[`RAMWIDTH-1:0]  q;
	wire	[`RAMWIDTH-1:0]  value_out;
	wire [`RAMWIDTH-1:0] subwire;
	assign q = subwire | dummy;
	wire [`RAMWIDTH-1:0] uselessdata;
 assign uselessdata = 256'b0;
wire j;
assign j = |byteena_a;
 wire [`RAMWIDTH-1:0]dummy;
 assign dummy = value_out & 256'b0;
dual_port_ram inst1( 
.clk (clk),
.we1(wren),
.we2(1'b0),
.data1(data),
.data2(uselessdata),
.out1(value_out),
.out2(subwire),
.addr1(wraddress),
.addr2(rdaddress));


endmodule

module ram3 (
	byteena_a,
	clk,
	data,
	rdaddress,
	wraddress,
	wren,
	q
	);

	input	[`RAMNUMBYTES-1:0]  byteena_a;
	input	  clk;
	input	[`RAMWIDTH-1:0]  data;
	input	[`rRAMSIZEWIDTH-1:0]  rdaddress;
	input	[`rRAMSIZEWIDTH-1:0]  wraddress;
	input	  wren;
	output	[`RAMWIDTH-1:0]  q;
	wire	[`RAMWIDTH-1:0]  value_out;
	wire [`RAMWIDTH-1:0] subwire;
	assign q = subwire | dummy;
	wire [`RAMWIDTH-1:0] uselessdata;
 assign uselessdata = 256'b0;
wire j;
assign j = |byteena_a;
 wire [`RAMWIDTH-1:0]dummy;
 assign dummy = value_out & 256'b0;
dual_port_ram inst1( 
.clk (clk),
.we1(wren),
.we2(1'b0),
.data1(data),
.data2(uselessdata),
.out1(value_out),
.out2(subwire),
.addr1(wraddress),
.addr2(rdaddress));


endmodule


module top_ram (
	clk,
	data,
	rdaddress,
	wraddress,
	wren,
	q
	);

	//parameter TOPSIZE = 256, TOPSIZEWIDTH = 8, TOPWIDTH = 32;
	
	input	  clk;
	input	[32-1:0]  data;
	input	[8-1:0]  rdaddress;
	input	[8-1:0]  wraddress;
	input	  wren;
	output	[32-1:0]  q;

	wire [32-1:0] sub_wire0;
	wire [32-1:0] q;
	wire [32-1:0] junk_output;
	assign q = sub_wire0 | dummy;
	wire[32-1:0] dummy;
	assign dummy = junk_output & 32'b0;
 dual_port_ram inst2(
 .clk (clk),
 .we1(wren),
 .we2(1'b0),
 .data1(data),
 .data2(data),
 .out1(junk_output),
 .out2(sub_wire0),
 .addr1(wraddress),
 .addr2(rdaddress));

endmodule

module mult_add (clk, A, B, C, mult_result, add_result);
//parameter PRECISION = 32;
input clk;
input [32-1:0] A, B, C;
output [32-1:0] mult_result, add_result;
reg [32-1:0] mult_result;
reg [32-1:0] add_result;
wire [32-1:0] mult_comp_result;
reg [32-1:0] add_a, add_b;
wire [32-1:0] addition_result;
wire [31:0] dummy_wire;
assign dummy_wire = mult_comp_result>>2'b10;
//divsp MUL(.clk(clk), .rmode(2'b00), .fpu_op(3'b010), .opa(A), .opb(B), .ans(mult_comp_result) );
wire [4:0]dummy_wire_2;
fpmul MUL(.clk(clk), .a(A), .b(B), .y_out(mult_comp_result), .control(2'b00), .flags(dummy_wire_2));
fpu_add ADD(.clock(clk), .a1(C), .b1(dummy_wire), .sum(addition_result));
always @ (posedge clk)
begin
	add_result  <= addition_result;
	mult_result <= mult_comp_result[31:0];
end
endmodule


//`define rFIFOINPUTWIDTH 64
`define rFIFOSIZE 64
`define rFIFOSIZEWIDTH 6
`define rFIFOOUTPUTWIDTH 256
`define rFIFORSIZEWIDTH 4
	`define wFIFOINPUTWIDTH 10'b0100000000
	`define wFIFOSIZE 6'b010000
	`define wFIFOSIZEWIDTH 4'b0100
	`define wFIFOOUTPUTWIDTH 8'b01000000
	`define wFIFORSIZEWIDTH 4'b0110
 //for addr_fifo
`define aFIFOSIZE 6'b010000
`define aFIFOSIZEWIDTH 4'b0100
`define aFIFOWIDTH 4'b0101
//for memfifo
`define mFIFOSIZE 16
`define mFIFOSIZEWIDTH 4
//`define mFIFOWIDTH 28

`define BURSTLEN 3'b010
`define BURSTWIDTH 3'b010
`define DATAWIDTH 10'b0100000000
`define DATANUMBYTES 7'b0100000
`define MEMCONWIDTH 8'b01000000
`define MEMCONNUMBYTES 5'b01000
`define DDRSIZEWIDTH 6'b011000
`define FIFOSIZE 6'b010000
`define FIFOSIZEWIDTH 4'b0100
`define RAMWIDTH 10'b0100000000
`define RAMNUMBYTES 7'b0100000
`define RAMSIZEWIDTH 4'b0101
`define RATIO 4'b0100
`define RAMLAT 4'b0101
 
`define dIDLE 0
`define dWRITE 1
`define dREAD 2

module DataTransferUnit (clk, dtu_write_req, dtu_read_req, dtu_mem_addr, dtu_ram_addr, dtu_size, dtu_ack, dtu_done,
		ram_read_addr, ram_read_data, ram_write_byte_en, ram_write_data, ram_write_addr, ram_write_en,
		mem_rdata, mem_rdata_valid, mem_ready, mem_wdata_req, reset_n,
		burst_begin, mem_local_addr, mem_be, mem_read_req, mem_size, mem_wdata, mem_write_req
		);

output burst_begin;
output [`DDRSIZEWIDTH-1:0] mem_local_addr;
output [`MEMCONNUMBYTES-1: 0] mem_be;
output mem_read_req;
output [`BURSTWIDTH-1:0] mem_size;
output [`MEMCONWIDTH-1:0] mem_wdata;
output mem_write_req;
input clk;
input [`MEMCONWIDTH-1:0] mem_rdata;
input mem_rdata_valid;
input mem_ready;
input mem_wdata_req;
input reset_n;

input dtu_write_req;
input dtu_read_req;
input [`DDRSIZEWIDTH-1:0] dtu_mem_addr;
input [`RAMSIZEWIDTH-1:0] dtu_ram_addr;
input [4:0] dtu_size;
output dtu_ack;
output dtu_done;

output[`RAMWIDTH-1:0] ram_write_data;
input[`RAMWIDTH-1:0] ram_read_data;
output[`RAMSIZEWIDTH-1:0] ram_write_addr, ram_read_addr;
output[`RAMNUMBYTES-1:0] ram_write_byte_en;
output ram_write_en;

reg[`DDRSIZEWIDTH-1:0] mem_addr0;
reg[`DDRSIZEWIDTH-1:0] mem_addr1;
reg[`DDRSIZEWIDTH-1:0] mem_addr2;
reg[`DDRSIZEWIDTH-1:0] mem_addr3;
reg[`DDRSIZEWIDTH-1:0] mem_addr4;
reg[`DDRSIZEWIDTH-1:0] mem_addr5;

reg [1:0] state;
wire [`DATAWIDTH-1:0] rdata, ram_write_dataw, ram_read_dataw;

wire [`RAMSIZEWIDTH-1:0] rfifo_addr;
reg [`RAMLAT-1:0]fifo_write_reg;
reg [`RAMLAT-1:0]write_req_reg;
reg [`RAMLAT-1:0]read_req_reg;
reg [0:0]fifo_read_reg;
reg rdata_valid;
reg [1:0]test_complete_reg;
reg [`BURSTWIDTH-1:0] size_count0;
reg [`BURSTWIDTH-1:0] size_count1;
reg [`BURSTWIDTH-1:0] size_count2;
reg [`BURSTWIDTH-1:0] size_count3;
reg [`BURSTWIDTH-1:0] size_count4;

reg [`RAMSIZEWIDTH-1:0] size;
reg [`RAMSIZEWIDTH-1:0]ram_addr0;
reg [`RAMSIZEWIDTH-1:0]ram_addr1;
reg [`RAMSIZEWIDTH-1:0]ram_addr2;
reg [`RAMSIZEWIDTH-1:0]ram_addr3;
reg [`RAMSIZEWIDTH-1:0]ram_addr4;

reg [2:0] data_count;
reg ram_write_en_reg;

wire read_req;
wire write_req;
wire [`FIFOSIZEWIDTH-1:0] wfifo_count;
wire rfull, wempty, rempty, rdcmd_empty, wrcmd_full, wrcmd_empty, rdata_empty;
wire [`DATAWIDTH-1:0] mem_data;
wire not_stall;
wire fifo_write, fifo_read;
wire rdata_req;
wire [`BURSTWIDTH+`DDRSIZEWIDTH+1:0] wrmem_cmd, rdmem_cmd;
wire mem_cmd_ready, mem_cmd_issue;

// FIFOs to interact with off-chip memory
memcmd_fifo cmd_store(
	//.aclr(~reset_n),
	//.rdclk(phy_clk),
	.clk(clk),
	.data(wrmem_cmd),
	.rdreq(mem_cmd_ready),
	//.rdempty(rdcmd_empty),
	.wrreq(mem_cmd_issue),
	.full(wrcmd_full),
	.empty(wrcmd_empty),
	.q(rdmem_cmd)
	);

wfifo wdata_store(
	//.rdclk(phy_clk),
	.clk(clk),
	.data(mem_data),
	.rdreq(mem_wdata_req),
	.wrreq(fifo_write),
	.empty(wempty),
	.q(mem_wdata),
	.usedw(wfifo_count)
	);

addr_fifo raddress_store (
	.clk(clk),
	.data(ram_addr3),
	.wrreq(fifo_read),
	.rdreq(rdata_req),
	.empty(rempty),
	.full(rfull),
	.q(rfifo_addr)
	);

rfifo rdata_store(
	.clk(clk),
	.data(mem_rdata),
	.rdreq(rdata_req),
	//.wrclk(phy_clk),
	.wrreq(mem_rdata_valid),
	.empty(rdata_empty),
	.q(rdata)
	);

assign mem_cmd_ready = (mem_ready == 1'b1);// && (rdcmd_empty == 0);
assign mem_cmd_issue = (wrcmd_full == 1'b0) && (write_req == 1 || read_req == 1'b1 || wrcmd_empty == 1'b1);
assign wrmem_cmd[27:26] = size_count0;
assign wrmem_cmd[`DDRSIZEWIDTH+1:2] = mem_addr0;
assign wrmem_cmd[1] = read_req;
assign wrmem_cmd[0] = write_req;
assign mem_write_req = rdmem_cmd[0];// && rdcmd_empty == 0;
assign mem_read_req = rdmem_cmd[1];// && rdcmd_empty == 0;
assign mem_local_addr = rdmem_cmd[`DDRSIZEWIDTH+1:2];
assign burst_begin = 0;
assign mem_size = rdmem_cmd[`BURSTWIDTH+`DDRSIZEWIDTH+1:`DDRSIZEWIDTH+2];
assign mem_be = ~0;
assign fifo_write = fifo_write_reg[0];
assign write_req = (not_stall) ? write_req_reg[0] : 0;
assign read_req = (not_stall) ? read_req_reg[0] : 0;
assign fifo_read = (not_stall) ? fifo_read_reg[0] : 0;
assign not_stall = (wfifo_count < `FIFOSIZE-5) && (rfull == 0) && (wrcmd_full == 0);
assign dtu_ack = (state == `dIDLE);
assign dtu_done = (state == `dIDLE) && wempty && rempty;

assign ram_write_dataw[63:0] = rdata[255:192];
assign mem_data[63:0] = ram_read_dataw[255:192];
assign ram_write_dataw[127:64] = rdata[191:128];
assign mem_data[127:64] = ram_read_dataw[191:128];
assign ram_write_dataw[191:128] = rdata[127:64];
assign mem_data[191:128] = ram_read_dataw[127:64];
assign ram_write_dataw[255:192] = rdata[63:0];
assign mem_data[255:192] = ram_read_dataw[63:0];
assign ram_write_data = ram_write_dataw[255:0];
assign ram_read_dataw[255:0] = ram_read_data;
assign ram_write_addr = rfifo_addr;
assign ram_read_addr = ram_addr4;
assign ram_write_byte_en = ~0;
assign ram_write_en = ram_write_en_reg;
assign rdata_req = !rdata_empty;

// FSM to produce off-chip memory commands
always @ (posedge clk)
begin
	if (reset_n == 1'b0)
	begin
		state <= `dIDLE;
	end
	else
	begin
		case (state)
		`dIDLE:
		begin
			if (dtu_write_req)
				state <= `dWRITE;
			else if (dtu_read_req)
				state <= `dREAD;
			else
				state <= `dIDLE;
		end
		`dWRITE:
		begin
			if (not_stall && size == 0 && data_count < `BURSTLEN)
				state <= `dIDLE;
			else
				state <= `dWRITE;
		end
		`dREAD:
		begin
			if (not_stall && size == 0 && data_count < `BURSTLEN)
				state <= `dIDLE;
			else
				state <= `dREAD;
		end
		default:
		begin
			state <= `dIDLE;
		end
		endcase
	end
end

always @ (posedge clk)
begin

	if (reset_n == 0)
	begin
		size <= 0;
		data_count <= 0;
		size_count4 <= 1;
		mem_addr5 <= 0;
		ram_addr4 <= 0;
		fifo_write_reg[`RAMLAT-1] <= 0;
		write_req_reg[`RAMLAT-1] <= 0;
		fifo_read_reg[0] <= 0;
		read_req_reg[`RAMLAT-1] <= 0;
	end
	else if (state == `dIDLE)
	begin
		size <= dtu_size;
		size_count4 <= `BURSTLEN;
		mem_addr5 <= dtu_mem_addr;
		ram_addr4 <= dtu_ram_addr;
		fifo_write_reg[`RAMLAT-1] <= 1'b0;
		write_req_reg[`RAMLAT-1] <= 1'b0;
		fifo_read_reg[0] <= 1'b0;
		read_req_reg[`RAMLAT-1] <= 1'b0;
		data_count <= 0;
	end
	else if (data_count >= `BURSTLEN && not_stall)
	begin
		data_count <= data_count - `BURSTLEN;
		mem_addr5 <= mem_addr5 + `BURSTLEN;
		fifo_write_reg[`RAMLAT-1] <= 1'b0;
		write_req_reg[`RAMLAT-1] <= state == `dWRITE;
		fifo_read_reg[0] <= 0;
		read_req_reg[`RAMLAT-1] <= state == `dREAD;
	end
	else if (size == 0 && data_count == 0 && not_stall==1'b1)
	begin
		fifo_write_reg[`RAMLAT-1] <= 0;
		write_req_reg[`RAMLAT-1] <= 0;
		fifo_read_reg[0] <= 0;
		read_req_reg[`RAMLAT-1] <= 0;
	end
	else if (size == 0 && not_stall==1'b1)
	begin
		size_count4 <= data_count[`BURSTWIDTH-1:0];
		fifo_write_reg[`RAMLAT-1] <= 0;
		write_req_reg[`RAMLAT-1] <= state == `dWRITE;
		fifo_read_reg[0] <= 0;
		read_req_reg[`RAMLAT-1] <= state == `dREAD;
	end
	else if (not_stall==1'b1)
	begin
		size <= size - 1;
		data_count <= data_count + `RATIO - `BURSTLEN;
		mem_addr5 <= mem_addr5 + `BURSTLEN;
		ram_addr4 <= ram_addr4+1;
		fifo_write_reg[`RAMLAT-1] <= state == `dWRITE;
		write_req_reg[`RAMLAT-1] <= state == `dWRITE;
		fifo_read_reg[0] <= state == `dREAD;
		read_req_reg[`RAMLAT-1] <= state == `dREAD;
	end
	else
	begin
		fifo_write_reg[`RAMLAT-1] <= 0;
	end
end


always @ (posedge clk)
begin
	if (reset_n == 0)
	begin
		fifo_write_reg[0] <= 1'b0;
		fifo_write_reg[1] <= 1'b0;
		fifo_write_reg[2] <= 1'b0;
		fifo_write_reg[3] <= 1'b0;
	end
	else
	begin
		fifo_write_reg[0] <= fifo_write_reg[1];
		fifo_write_reg[1] <= fifo_write_reg[2];
		fifo_write_reg[2] <= fifo_write_reg[3];
		fifo_write_reg[3] <= fifo_write_reg[4];
	end

	if (reset_n == 1'b0)
	begin
		mem_addr0 <= 0;
		ram_addr0 <= 0;
		size_count0 <= 1;
		write_req_reg[0] <= 0;
		read_req_reg[0] <= 0;
		mem_addr1 <= 0;
		ram_addr1 <= 0;
		size_count1 <= 1;
		write_req_reg[1] <= 0;
		read_req_reg[1] <= 0;
		mem_addr2 <= 0;
		ram_addr2 <= 0;
		size_count2 <= 1;
		write_req_reg[2] <= 0;
		read_req_reg[2] <= 0;
		mem_addr3 <= 0;
		ram_addr3 <= 0;
		size_count3 <= 1;
		write_req_reg[3] <= 0;
		read_req_reg[3] <= 0;
		mem_addr4 <= 0;
	end
	else if (not_stall)
	begin
		size_count0 <= size_count1;
		mem_addr0 <= mem_addr1;
		ram_addr0 <= ram_addr1;
		write_req_reg[0] <= write_req_reg[1];
		read_req_reg[0] <= read_req_reg[1];
		size_count1 <= size_count2;
		mem_addr1 <= mem_addr2;
		ram_addr1 <= ram_addr2;
		write_req_reg[1] <= write_req_reg[2];
		read_req_reg[1] <= read_req_reg[2];
		size_count2 <= size_count3;
		mem_addr2 <= mem_addr3;
		ram_addr2 <= ram_addr3;
		write_req_reg[2] <= write_req_reg[3];
		read_req_reg[2] <= read_req_reg[3];
		size_count3 <= size_count4;
		mem_addr3 <= mem_addr4;
		ram_addr3 <= ram_addr4;
		write_req_reg[3] <= write_req_reg[4];
		read_req_reg[3] <= read_req_reg[4];
		mem_addr4 <= mem_addr5;
	end
	
	ram_write_en_reg  <= rdata_req;
end

endmodule

module rfifo (
	clk,
	data,
	rdreq,
	wrreq,
	empty,
	q
	);


	input	  clk;
	input	  wrreq;
	input	  rdreq;
	input	[`rFIFOINPUTWIDTH-1:0]  data;
	output	  empty;
	output	[`rFIFOOUTPUTWIDTH-1:0]  q;

	reg [`rFIFORSIZEWIDTH-1:0] wr_pointer;
	reg [`rFIFORSIZEWIDTH-1:0] rd_pointer;
	reg [`rFIFORSIZEWIDTH:0] status_cnt;
	reg [`rFIFOOUTPUTWIDTH-1:0] q ;
	reg[1:0] counter;
	wire [`rFIFOINPUTWIDTH-1:0] data_ram;
assign empty = (status_cnt == 7'b0000000);
wire [`rFIFOINPUTWIDTH-1:0]junk_input;
wire [`rFIFOINPUTWIDTH-1:0]junk_output;
assign junk_input = 64'b0000000000000000000000000000000000000000000000000000000000000000;
 always @ (posedge clk)
 begin  //WRITE_POINTER
	if (wrreq) 
	begin
 		wr_pointer <= wr_pointer + 1'b1;
	end
end
always @ (posedge clk)
begin  //READ_POINTER
	if (rdreq) 
	begin
	rd_pointer <= rd_pointer + 2'b01;
	end
end
always  @ (posedge clk )
begin  //READ_DATA
if (rdreq) 
	counter <= 0;
else 
	counter <= counter + 2'b01;
if(counter == 0)
	q[`rFIFOINPUTWIDTH-1:0] <= data_ram;
else if (counter == 1)
	q[127:64] <= data_ram;
else if (counter == 2)
	q[191:128] <= data_ram;
else if (counter == 3)
	q[255:192] <= data_ram;
end
always @ (posedge clk )
begin // : STATUS_COUNTER
	if ((rdreq) && (!wrreq) && (status_cnt != 0))
		status_cnt <= status_cnt - 1'b1;
// Write but no read.
	else if ((wrreq) && (!rdreq) && (status_cnt != 64 ))
		status_cnt <= status_cnt + 1'b1;
end 
  dual_port_ram ram_addr(
.we1      (wrreq)      , // write enable
 .we2      (rdreq)       , // Read enable
.addr1 (wr_pointer) , // address_0 input 
.addr2 (rd_pointer) , // address_q input  
.data1    (data)    , // data_0 bi-directional
.data2    (junk_input),   // data_1 bi-directional
.clk(clk),
.out1	(data_ram),
.out2	(junk_output)
 ); 


endmodule


// synopsys translate_off
//`timescale 1 ps / 1 ps
// synopsys translate_on
module wfifo (
	clk,
	data,
	rdreq,
	wrreq,
	empty,
	q,
	usedw
	);

	input	clk;
	input	  wrreq;
	input	  rdreq;
	input	[`wFIFOINPUTWIDTH-1:0]  data;
	output	  empty;
	output	[`wFIFOOUTPUTWIDTH-1:0]  q;
	output	[`wFIFOSIZEWIDTH-1:0]  usedw;
//-----------Internal variables-------------------
reg [`wFIFOSIZEWIDTH-1:0] wr_pointer;
reg [`wFIFOSIZEWIDTH-1:0] rd_pointer;
reg [`wFIFOSIZEWIDTH:0] status_cnt;
reg [`wFIFOOUTPUTWIDTH-1:0] q ;
reg[1:0] counter;
wire [`wFIFOINPUTWIDTH-1:0] data_ram ;
assign empty = (status_cnt == 5'b00000);
wire [`wFIFOINPUTWIDTH-1:0]junk_input;
wire [`wFIFOINPUTWIDTH-1:0]junk_output;
assign junk_input = 256'b0;
 always @ (posedge clk)
 begin  //WRITE_POINTER
	if (wrreq) 
	begin
 		wr_pointer <= wr_pointer + 1'b1;
	end
end
always @ (posedge clk)
begin  //READ_POINTER
	if (rdreq) 
	begin
	rd_pointer <= rd_pointer + 2'b01;
	end
end
always  @ (posedge clk )
begin  //READ_DATA
if (rdreq) 
	counter <= 0;
else 
	counter <= counter + 2'b01;
if(counter == 0)
	q <= data_ram[63:0];
else if(counter == 1)
	q <= data_ram[127:64];
else if(counter == 2)
	q <= data_ram[191:128];
else if(counter == 3)
	q <= data_ram[255:192];
end
always @ (posedge clk )
begin // : STATUS_COUNTER
	if ((rdreq) && (!wrreq) && (status_cnt != 5'b00000))
		status_cnt <= status_cnt - 1'b1;
	// Write but no read.
	else if ((wrreq) && (!rdreq) && (status_cnt != 5'b10000 )) 
		status_cnt <= status_cnt + 1'b1;
end 
assign usedw = status_cnt[`wFIFOSIZEWIDTH-1:0];
  dual_port_ram ram_addr(
.we1      (wrreq)      , // write enable
 .we2      (rdreq)       , // Read enable
.addr1 (wr_pointer) , // address_0 input 
.addr2 (rd_pointer) , // address_q input  
.data1    (data)    , // data_0 bi-directional
.data2    (junk_input),   // data_1 bi-directional
.clk(clk),
.out1	(data_ram),
.out2	(junk_output)
 ); 


endmodule

// synopsys translate_off
//`timescale 1 ps / 1 ps
// synopsys translate_on
module addr_fifo (
	clk,
	data,
	wrreq,
	rdreq,
	empty,
	full,
	q
	);

	input	  clk;
	input	[`aFIFOWIDTH-1:0]  data;
	input	  rdreq;
	input	  wrreq;
	output	  empty;
	output	  full;
	output	[`aFIFOWIDTH-1:0]  q;

reg [`aFIFOSIZEWIDTH-1:0] wr_pointer;
reg [`aFIFOSIZEWIDTH-1:0] rd_pointer;
reg [`aFIFOSIZEWIDTH:0] status_cnt;
reg [`aFIFOWIDTH-1:0] q ;
wire [`aFIFOWIDTH-1:0] data_ram ;
assign full = (status_cnt == 5'b01111);
assign empty = (status_cnt == 5'b00000);
wire [`aFIFOWIDTH-1:0]junk_input;
wire [`aFIFOWIDTH-1:0]junk_output;
assign junk_input = 5'b00000;
always @ (posedge clk)
begin  //WRITE_POINTER
if (wrreq) 
begin
wr_pointer <= wr_pointer + 1'b1;
end
end
always @ (posedge clk)
begin  //READ_POINTER
if (rdreq) 
begin
rd_pointer <= rd_pointer + 1'b1;
end
end
always  @ (posedge clk )
begin  //READ_DATA
if (rdreq) begin
q <= data_ram; 
end
end
always @ (posedge clk )
begin // : STATUS_COUNTER
	if ((rdreq) && (!wrreq) && (status_cnt != 5'b00000))
		status_cnt <= status_cnt - 1'b1;
	// Write but no read.
	else if ((wrreq) && (!rdreq) && (status_cnt != 5'b10000))
		status_cnt <= status_cnt + 1;
end
  dual_port_ram ram_addr(
.we1      (wrreq)      , // write enable
 .we2      (rdreq)       , // Read enable
.addr1 (wr_pointer) , // address_0 input 
.addr2 (rd_pointer) , // address_q input  
.data1    (data)    , // data_0 bi-directional
.data2    (junk_input),   // data_1 bi-directional
.clk(clk),
.out1	(data_ram),
.out2	(junk_output)
 ); 


endmodule

module memcmd_fifo (
	clk,
	data,
	rdreq,
	wrreq,
	full,
	empty,
	q
	);
	
	input	  clk;
	input	[`mFIFOWIDTH-1:0]  data;
	input	  wrreq;
	input	  rdreq;
	output	  full;
	output	  empty;
	output	[`mFIFOWIDTH-1:0]  q;

	reg [`mFIFOSIZEWIDTH-1:0] wr_pointer;
	reg [`mFIFOSIZEWIDTH-1:0] rd_pointer;
	reg [`mFIFOSIZEWIDTH:0] status_cnt;
	reg [`mFIFOWIDTH-1:0] q ;
	wire [`mFIFOWIDTH-1:0] data_ram;
	assign full = (status_cnt ==5'b01111);
	assign empty = (status_cnt == 5'b00000);
	wire [`mFIFOWIDTH-1:0]junk_input;
	wire [`mFIFOWIDTH-1:0]junk_output;
	assign junk_input = 28'b0000000000000000000000000000;
	always @ (posedge clk)
	begin  //WRITE_POINTER
		if (wrreq)
			begin
				wr_pointer <= wr_pointer + 1'b1;
			end
	end
	always @ (posedge clk)
	begin  //READ_POINTER
		if (rdreq)
		begin
			rd_pointer <= rd_pointer + 1'b1;
		end
	end
	always  @ (posedge clk )
	begin  //READ_DATA
		if (rdreq) begin
			q <= data_ram;
		end
	end
always @ (posedge clk )
begin // : STATUS_COUNTER
	if ((rdreq) && (!wrreq) && (status_cnt != 0))
		status_cnt <= status_cnt - 1'b1;
	else if ((wrreq) && (!rdreq) && (status_cnt != 16 ))
		status_cnt <= status_cnt + 1'b1;
end
	dual_port_ram ram_addr(
	.we1      (wrreq)      , // write enable
	.we2      (rdreq)       , // Read enable
	.addr1 (wr_pointer) , // address_0 input
	.addr2 (rd_pointer) , // address_q input
	.data1    (data)    , // data_0 bi-directional
	.data2    (junk_input),   // data_1 bi-directional
	.clk(clk),
	.out1	(data_ram),
	.out2	(junk_output));


endmodule


`define ZERO        8'b00000000  
`define ONE         8'b00000001  
`define TWO         8'b00000010  
`define THREE 		  8'b00000011  
`define FOUR		  8'b00000100  
`define FIVE		  8'b00000101  
`define SIX         8'b00000110  
`define SEVEN       8'b00000111  
`define EIGHT       8'b00001000  
`define NINE        8'b00001001  
`define TEN         8'b00001010  
`define ELEVEN      8'b00001011  
`define TWELVE      8'b00001100  
`define THIRTEEN    8'b00001101  
`define FOURTEEN    8'b00001110  
`define FIFTEEN     8'b00001111  
`define SIXTEEN     8'b00010000  
`define SEVENTEEN   8'b00010001  
`define EIGHTEEN	  8'b00010010  
`define NINETEEN    8'b00010011  
`define TWENTY		  8'b00010100  
`define TWENTYONE   8'b00010101  
`define TWENTYTWO   8'b00010110  
`define TWENTYTHREE 8'b00010111  
`define TWENTYFOUR  8'b00011000  
  
module fpu_add (clock, a1, b1, sum);  
	input clock;  
	input [31:0]a1;  
	input [31:0]b1;  
	output [31:0]sum;  
	reg [31:0]sum;  
	  
	//Split up the numbers into exponents and mantissa.  
	reg [7:0]a_exp; 
	//reg [7:0]b_exp;  
	reg [23:0]a_man; 
	reg [23:0]b_man; 
  
	reg [7:0]temp;  
	  
	reg [24:0]sum_man;  
	//reg [7:0]sum_exp;  
	  
	//introduce latency on inputs  
	reg [31:0]a;  
	reg [31:0]b;  
	  
	always @ (posedge clock) begin  
		a <= a1;  
		b <= b1;  
	end  
	  
	reg smaller; //smaller is 1 if a < b, 0 otherwise  
	  
	//Shift mantissa's to have the same exponent  
	always @ (a or b) begin  
		//a_exp = a[30:23];  
		//b_exp = b[30:23];  
		//a_man = {1'b1, a[22:0]};  
	   //b_man = {1'b1, b[22:0]};  
		  
		if (a[30:23] < b[30:23]) begin  
			temp = b[30:23] - a[30:23];  
			//a_man = {1'b1, a[22:0]} >> temp; //Expand into case statement, as below.  
			case (temp)   
				`ONE: begin  
					a_man = {1'b1, a[22:0]} >> `ONE;  
				end  
				`TWO: begin  
					a_man = {1'b1, a[22:0]} >> `TWO;  
				end  
				`THREE: begin  
					a_man = {1'b1, a[22:0]} >> `THREE;  
				end  
				`FOUR: begin  
					a_man = {1'b1, a[22:0]} >> `FOUR;  
				end  
				`FIVE: begin  
					a_man = {1'b1, a[22:0]} >> `FIVE;  
				end  
				`SIX: begin  
					a_man = {1'b1, a[22:0]} >> `SIX;  
				end  
				`SEVEN: begin  
					a_man = {1'b1, a[22:0]} >> `SEVEN;  
				end  
				`EIGHT: begin  
					a_man = {1'b1, a[22:0]} >> `EIGHT;  
				end  
				`NINE: begin  
					a_man = {1'b1, a[22:0]} >> `NINE;  
				end  
				`TEN: begin  
					a_man = {1'b1, a[22:0]} >> `TEN;  
				end  
				`ELEVEN: begin  
					a_man = {1'b1, a[22:0]} >> `ELEVEN;  
				end  
				`TWELVE: begin  
					a_man = {1'b1, a[22:0]} >> `TWELVE;  
				end  
				`THIRTEEN: begin  
					a_man = {1'b1, a[22:0]} >> `THIRTEEN;  
				end  
				`FOURTEEN: begin  
					a_man = {1'b1, a[22:0]} >> `FOURTEEN;  
				end  
				`FIFTEEN: begin  
					a_man = {1'b1, a[22:0]} >> `FIFTEEN;  
				end  
				`SIXTEEN: begin  
					a_man = {1'b1, a[22:0]} >> `SIXTEEN;  
				end  
				`SEVENTEEN: begin  
					a_man = {1'b1, a[22:0]} >> `SEVENTEEN;  
				end  
				`EIGHTEEN: begin  
					a_man = {1'b1, a[22:0]} >> `EIGHTEEN;  
				end  
				`NINETEEN: begin  
					a_man = {1'b1, a[22:0]} >> `NINETEEN;  
				end  
				`TWENTY: begin  
					a_man = {1'b1, a[22:0]} >> `TWENTY;  
				end  
				`TWENTYONE: begin  
					a_man = {1'b1, a[22:0]} >> `TWENTYONE;  
				end  
				`TWENTYTWO: begin  
					a_man = {1'b1, a[22:0]} >> `TWENTYTWO;  
				end  
				`TWENTYTHREE: begin  
					a_man = {1'b1, a[22:0]} >> `TWENTYTHREE;  
				end  
				`TWENTYFOUR: begin  
					a_man = {1'b1, a[22:0]} >> `TWENTYFOUR;  
				end  
				default: begin //More than twenty-four, shift by twenty-four. It is a boundary case.  
					a_man = {1'b1, a[22:0]} >> `TWENTYFOUR;  
				end  
			endcase   
				  
			b_man = {1'b1, b[22:0]};  
			a_exp = b[30:23];  
			//b_exp = b[30:23];  
			  
		end else if (a[30:23] > b[30:23]) begin  
			temp = a[30:23] - b[30:23];  
			a_man = {1'b1, a[22:0]};  
			//b_man = {1'b1, b[22:0]} >> temp; //Expand into case statement, as below.  
			case (temp)   
				`ONE: begin  
					b_man = {1'b1, b[22:0]} >> `ONE;  
				end  
				`TWO: begin  
					b_man = {1'b1, b[22:0]} >> `TWO;  
				end  
				`THREE: begin  
					b_man = {1'b1, b[22:0]} >> `THREE;  
				end  
				`FOUR: begin  
					b_man = {1'b1, b[22:0]} >> `FOUR;  
				end  
				`FIVE: begin  
					b_man = {1'b1, b[22:0]} >> `FIVE;  
				end  
				`SIX: begin  
					b_man = {1'b1, b[22:0]} >> `SIX;  
				end  
				`SEVEN: begin  
					b_man = {1'b1, b[22:0]} >> `SEVEN;  
				end  
				`EIGHT: begin  
					b_man = {1'b1, b[22:0]} >> `EIGHT;  
				end  
				`NINE: begin  
					b_man = {1'b1, b[22:0]} >> `NINE;  
				end  
				`TEN: begin  
					b_man = {1'b1, b[22:0]} >> `TEN;  
				end  
				`ELEVEN: begin  
					b_man = {1'b1, b[22:0]} >> `ELEVEN;  
				end  
				`TWELVE: begin  
					b_man = {1'b1, b[22:0]} >> `TWELVE;  
				end  
				`THIRTEEN: begin  
					b_man = {1'b1, b[22:0]} >> `THIRTEEN;  
				end  
				`FOURTEEN: begin  
					b_man = {1'b1, b[22:0]} >> `FOURTEEN;  
				end  
				`FIFTEEN: begin  
					b_man = {1'b1, b[22:0]} >> `FIFTEEN;  
				end  
				`SIXTEEN: begin  
					b_man = {1'b1, b[22:0]} >> `SIXTEEN;  
				end  
				`SEVENTEEN: begin  
					b_man = {1'b1, b[22:0]} >> `SEVENTEEN;  
				end  
				`EIGHTEEN: begin  
					b_man = {1'b1, b[22:0]} >> `EIGHTEEN;  
				end  
				`NINETEEN: begin  
					b_man = {1'b1, b[22:0]} >> `NINETEEN;  
				end  
				`TWENTY: begin  
					b_man = {1'b1, b[22:0]} >> `TWENTY;  
				end  
				`TWENTYONE: begin  
					b_man = {1'b1, b[22:0]} >> `TWENTYONE;  
				end  
				`TWENTYTWO: begin  
					b_man = {1'b1, b[22:0]} >> `TWENTYTWO;  
				end  
				`TWENTYTHREE: begin  
					b_man = {1'b1, b[22:0]} >> `TWENTYTHREE;  
				end  
				`TWENTYFOUR: begin  
					b_man = {1'b1, b[22:0]} >> `TWENTYFOUR;  
				end  
				default: begin //More than twenty-four, shift by twenty-four. It is a boundary case.  
					b_man = {1'b1, b[22:0]} >> `TWENTYFOUR;  
				end  
			endcase   
			  
			a_exp = a[30:23];  
			//b_exp = a[30:23];  
		end else begin  
			temp = 8'b0;  
			a_man = {1'b1, a[22:0]};  
			b_man = {1'b1, b[22:0]};  
			a_exp = a[30:23];  
		end  
		  
	end  
  
	//Perform the addition operation  
	always @ (a_man or b_man or a or b) begin  
		if (a_man < b_man) begin  
			smaller = 1'b1;  
		end else begin  
			smaller = 1'b0;  
		end  
	  
		//both positive  
		if (~a[31] && ~b[31]) begin  
			sum_man = a_man + b_man;  
			sum[31] = 1'b0;  
		end   
		  
		//both negative  
		else if (a[31] && b[31]) begin  
			sum_man = a_man + b_man;  
			sum[31] = 1'b1;  
		end  
		  
		//a pos, b neg  
		else if (~a[31] && b[31]) begin  
			if (smaller) begin //a < b  
				sum_man = b_man - a_man;  
				sum[31] = 1'b1;  
			end else begin  
				sum_man = a_man - b_man;  
				sum[31] = 1'b0;  
			end  
		end  
		  
		//a neg, b pos  
		else /*if (a[31] && ~b[31])*/ begin  
			if (smaller) begin //a < b  
				sum_man = b_man - a_man;  
				sum[31] = 1'b0;  
			end else begin  
				sum_man = a_man - b_man;  
				sum[31] = 1'b1;  
			end  
		end  
	end  
	  
	//Store the number  
	// we already have the sign.  
	  
	always @ (sum_man or a_exp) begin  
		if (sum_man[24])begin //shif sum >> by 1, add 1 to the exponent.  
			sum[22:0] = sum_man[23:1];  
			sum[30:23] = a_exp + 8'b00000001;  
			  
		end else if (sum_man[23]) begin //do nothing  
			sum[22:0] = sum_man[22:0];  
			sum[30:23] = a_exp;  
			  
		end else if (sum_man[22]) begin //shift << by 1, subtract 1 from exponent. 
			sum[22:0] = {sum_man[21:0], 1'b0}; 
			sum[30:23] = a_exp - 8'b00000001; 
 
		end else if (sum_man[21]) begin //shift << by 2, subtract 2 from exponent. 
			sum[22:0] = {sum_man[20:0], 2'b0}; 
			sum[30:23] = a_exp - 8'b00000010; 
 
		end else if (sum_man[20]) begin //shift << by 3, subtract 3 from exponent. 
			sum[22:0] = {sum_man[19:0], 3'b0}; 
			sum[30:23] = a_exp - 8'b00000011; 
 
		end else if (sum_man[19]) begin //shift << by 4, subtract 4 from exponent. 
			sum[22:0] = {sum_man[18:0], 4'b0}; 
			sum[30:23] = a_exp - 8'b00000100; 
 
		end else if (sum_man[18]) begin //shift << by 5, subtract 5 from exponent. 
			sum[22:0] = {sum_man[17:0], 5'b0}; 
			sum[30:23] = a_exp - 8'b00000101; 
 
		end else if (sum_man[17]) begin //shift << by 6, subtract 6 from exponent. 
			sum[22:0] = {sum_man[16:0], 6'b0}; 
			sum[30:23] = a_exp - 8'b00000110; 
 
		end else if (sum_man[16]) begin //shift << by 7, subtract 7 from exponent. 
			sum[22:0] = {sum_man[15:0], 7'b0}; 
			sum[30:23] = a_exp - 8'b00000111; 
 
		end else if (sum_man[15]) begin //shift << by 8, subtract 8 from exponent. 
			sum[22:0] = {sum_man[14:0], 8'b0}; 
			sum[30:23] = a_exp - 8'b00001000; 
 
		end else if (sum_man[14]) begin //shift << by 9, subtract 9 from exponent. 
			sum[22:0] = {sum_man[13:0], 9'b0}; 
			sum[30:23] = a_exp - 8'b00001001; 
 
		end else if (sum_man[13]) begin //shift << by 10, subtract 10 from exponent. 
			sum[22:0] = {sum_man[12:0], 10'b0}; 
			sum[30:23] = a_exp - 8'b00001010; 
 
		end else if (sum_man[12]) begin //shift << by 11, subtract 11 from exponent. 
			sum[22:0] = {sum_man[11:0], 11'b0}; 
			sum[30:23] = a_exp - 8'b00001011; 
 
		end else if (sum_man[11]) begin //shift << by 12, subtract 12 from exponent. 
			sum[22:0] = {sum_man[10:0], 12'b0}; 
			sum[30:23] = a_exp - 8'b00001100; 
 
		end else if (sum_man[10]) begin //shift << by 13, subtract 13 from exponent. 
			sum[22:0] = {sum_man[9:0], 13'b0}; 
			sum[30:23] = a_exp - 8'b00001101; 
 
		end else if (sum_man[9]) begin //shift << by 14, subtract 14 from exponent. 
			sum[22:0] = {sum_man[8:0], 14'b0}; 
			sum[30:23] = a_exp - 8'b00001110; 
 
		end else if (sum_man[8]) begin //shift << by 15, subtract 15 from exponent. 
			sum[22:0] = {sum_man[7:0], 15'b0}; 
			sum[30:23] = a_exp - 8'b00001111; 
 
		end else if (sum_man[7]) begin //shift << by 16, subtract 16 from exponent. 
			sum[22:0] = {sum_man[6:0], 16'b0}; 
			sum[30:23] = a_exp - 8'b00010000; 
 
		end else if (sum_man[6]) begin //shift << by 17, subtract 17 from exponent. 
			sum[22:0] = {sum_man[5:0], 17'b0}; 
			sum[30:23] = a_exp - 8'b00010001; 
 
		end else if (sum_man[5]) begin //shift << by 18, subtract 18 from exponent. 
			sum[22:0] = {sum_man[4:0], 18'b0}; 
			sum[30:23] = a_exp - 8'b00010010; 
 
		end else if (sum_man[4]) begin //shift << by 19, subtract 19 from exponent. 
			sum[22:0] = {sum_man[3:0], 19'b0}; 
			sum[30:23] = a_exp - 8'b00010011; 
 
		end else if (sum_man[3]) begin //shift << by 20, subtract 20 from exponent. 
			sum[22:0] = {sum_man[2:0], 20'b0}; 
			sum[30:23] = a_exp - 8'b00010100; 
 
		end else if (sum_man[2]) begin //shift << by 21, subtract 21 from exponent. 
			sum[22:0] = {sum_man[1:0], 21'b0}; 
			sum[30:23] = a_exp - 8'b00010101; 
 
		end else if (sum_man[1]) begin //shift << by 22, subtract 22 from exponent. 
			sum[22:0] = {sum_man[0:0], 22'b0}; 
			sum[30:23] = a_exp - 8'b00010110; 
 
		end else /*if (sum_man[0])*/ begin //shift << by 23, subtract 23 from exponent. 
			sum[22:0] = 23'b0; 
			sum[30:23] = a_exp - 8'b00010111;  
		end 
		  
	end  
  
endmodule   
	
module fpu_div(clock, n, d, div);  
//n = numerator  
//d = denomenator  
//div = result  
	input clock;  
  
	input [31:0]n;  
	input [31:0]d;  
	output [31:0]div;  
	reg [31:0]div;  
	  
	//Store the mantissa and exponents separately. Introduce the latency of 1.  
	reg [7:0]n_exp;  
	reg [7:0]d_exp;  
	reg [23:0]n_man;  
	reg [23:0]d_man;  
	reg n_sign;  
	reg d_sign;  
	  
	wire [23:0]div_man;  
	reg [7:0]div_exp;  
	  
	always @ (posedge clock) begin  
		n_exp <= n[30:23];  
		d_exp <= d[30:23];  
		n_man <= {1'b1, n[22:0]};  
		d_man <= {1'b1, d[22:0]};  
		n_sign <= n[31];  
		d_sign <= d[31];  
	end  
	  
	//Find the exponent, store in div_exp.  
	always @ (n_exp or d_exp) begin  
		if (n_exp >= d_exp) begin  
			div_exp = 8'b01111111 + (n_exp - d_exp);  
		end else begin  
			div_exp = 8'b01111111 - (d_exp - n_exp);  
		end  
	end  
	  
	//Divide the mantissas, store in div_man.  
	div_24b divide(.numer(n_man), .denom(d_man), .res(div_man));  
	  
	//Store the result. Shift exponents appropriately. Store sign.  
	//Sign  
	always @ (n_sign or d_sign) begin  
		div[31] = n_sign ^ d_sign;  
	end  
	  
	//Mantissa and Exponent  
	always @ (div_man or div_exp) begin  
		if (div_man[23]) begin //do nothing  
			div[22:0] = div_man[22:0];  
			div[30:23] = div_exp;  
		  
		end else if (div_man[22]) begin //shift << by 1, subtract 1 from exponent. 
			div[22:0] = {div_man[21:0], 1'b0}; 
			div[30:23] = div_exp - 8'b00000001; 
 
		end else if (div_man[21]) begin //shift << by 2, subtract 2 from exponent. 
			div[22:0] = {div_man[20:0], 2'b0}; 
			div[30:23] = div_exp - 8'b00000010; 
 
		end else if (div_man[20]) begin //shift << by 3, subtract 3 from exponent. 
			div[22:0] = {div_man[19:0], 3'b0}; 
			div[30:23] = div_exp - 8'b00000011; 
 
		end else if (div_man[19]) begin //shift << by 4, subtract 4 from exponent. 
			div[22:0] = {div_man[18:0], 4'b0}; 
			div[30:23] = div_exp - 8'b00000100; 
 
		end else if (div_man[18]) begin //shift << by 5, subtract 5 from exponent. 
			div[22:0] = {div_man[17:0], 5'b0}; 
			div[30:23] = div_exp - 8'b00000101; 
 
		end else if (div_man[17]) begin //shift << by 6, subtract 6 from exponent. 
			div[22:0] = {div_man[16:0], 6'b0}; 
			div[30:23] = div_exp - 8'b00000110; 
 
		end else if (div_man[16]) begin //shift << by 7, subtract 7 from exponent. 
			div[22:0] = {div_man[15:0], 7'b0}; 
			div[30:23] = div_exp - 8'b00000111; 
 
		end else if (div_man[15]) begin //shift << by 8, subtract 8 from exponent. 
			div[22:0] = {div_man[14:0], 8'b0}; 
			div[30:23] = div_exp - 8'b00001000; 
 
		end else if (div_man[14]) begin //shift << by 9, subtract 9 from exponent. 
			div[22:0] = {div_man[13:0], 9'b0}; 
			div[30:23] = div_exp - 8'b00001001; 
 
		end else if (div_man[13]) begin //shift << by 10, subtract 10 from exponent. 
			div[22:0] = {div_man[12:0], 10'b0}; 
			div[30:23] = div_exp - 8'b00001010; 
 
		end else if (div_man[12]) begin //shift << by 11, subtract 11 from exponent. 
			div[22:0] = {div_man[11:0], 11'b0}; 
			div[30:23] = div_exp - 8'b00001011; 
 
		end else if (div_man[11]) begin //shift << by 12, subtract 12 from exponent. 
			div[22:0] = {div_man[10:0], 12'b0}; 
			div[30:23] = div_exp - 8'b00001100; 
 
		end else if (div_man[10]) begin //shift << by 13, subtract 13 from exponent. 
			div[22:0] = {div_man[9:0], 13'b0}; 
			div[30:23] = div_exp - 8'b00001101; 
 
		end else if (div_man[9]) begin //shift << by 14, subtract 14 from exponent. 
			div[22:0] = {div_man[8:0], 14'b0}; 
			div[30:23] = div_exp - 8'b00001110; 
 
		end else if (div_man[8]) begin //shift << by 15, subtract 15 from exponent. 
			div[22:0] = {div_man[7:0], 15'b0}; 
			div[30:23] = div_exp - 8'b00001111; 
 
		end else if (div_man[7]) begin //shift << by 16, subtract 16 from exponent. 
			div[22:0] = {div_man[6:0], 16'b0}; 
			div[30:23] = div_exp - 8'b00010000; 
 
		end else if (div_man[6]) begin //shift << by 17, subtract 17 from exponent. 
			div[22:0] = {div_man[5:0], 17'b0}; 
			div[30:23] = div_exp - 8'b00010001; 
 
		end else if (div_man[5]) begin //shift << by 18, subtract 18 from exponent. 
			div[22:0] = {div_man[4:0], 18'b0}; 
			div[30:23] = div_exp - 8'b00010010; 
 
		end else if (div_man[4]) begin //shift << by 19, subtract 19 from exponent. 
			div[22:0] = {div_man[3:0], 19'b0}; 
			div[30:23] = div_exp - 8'b00010011; 
 
		end else if (div_man[3]) begin //shift << by 20, subtract 20 from exponent. 
			div[22:0] = {div_man[2:0], 20'b0}; 
			div[30:23] = div_exp - 8'b00010100; 
 
		end else if (div_man[2]) begin //shift << by 21, subtract 21 from exponent. 
			div[22:0] = {div_man[1:0], 21'b0}; 
			div[30:23] = div_exp - 8'b00010101; 
 
		end else if (div_man[1]) begin //shift << by 22, subtract 22 from exponent. 
			div[22:0] = {div_man[0:0], 22'b0}; 
			div[30:23] = div_exp - 8'b00010110; 
  
		end else /*if (div_man[0])*/ begin //shift << by 23, subtract 23 from exponent. 
			div[22:0] = 23'b0; 
			div[30:23] = div_exp - 8'b00010111; 
		end 
  
	end  
	  
endmodule   
  
  
  
  
  
module div_24b(numer, denom, res);  
	//input clock;  
  
	input [23:0]numer;  
	input [23:0]denom;  
	output [23:0]res;  
	reg [23:0]res;  
	  
	//Pad with 23 zeros.  
	wire [46:0]denom_pad;  
	wire [46:0]numer23; 
	reg [46:0]numer22; 
	reg [46:0]numer21; 
	reg [46:0]numer20; 
	reg [46:0]numer19; 
	reg [46:0]numer18; 
	reg [46:0]numer17; 
	reg [46:0]numer16; 
	reg [46:0]numer15; 
	reg [46:0]numer14; 
	reg [46:0]numer13; 
	reg [46:0]numer12; 
	reg [46:0]numer11; 
	reg [46:0]numer10; 
	reg [46:0]numer9; 
	reg [46:0]numer8; 
	reg [46:0]numer7; 
	reg [46:0]numer6; 
	reg [46:0]numer5; 
	reg [46:0]numer4; 
	reg [46:0]numer3; 
	reg [46:0]numer2; 
	reg [46:0]numer1;  
	reg [46:0]numer0;  
	  
	//always @ (posedge clock) begin  
	assign denom_pad = {23'b0, denom};  
	assign numer23 = {numer, 23'b0};  
	// end  
	  
	//res[23]  
	always @ (denom_pad or numer23) begin  
	  
		if (denom_pad[23:0] <= numer23[46:23]) begin 
			res[23] = 1'b1; 
			numer22 = {numer23[46:23] - denom_pad[23:0], 23'b0}; 
		end else begin 
			res[23] = 1'b0; 
			numer22 = numer23; 
		end 
 
		if (denom_pad[24:0] <= numer22[46:22]) begin 
			res[22] = 1'b1; 
			numer21 = {numer22[46:22] - denom_pad[24:0], 22'b0}; 
		end else begin 
			res[22] = 1'b0; 
			numer21 = numer22; 
		end 
 
		if (denom_pad[25:0] <= numer21[46:21]) begin 
			res[21] = 1'b1; 
			numer20 = {numer21[46:21] - denom_pad[25:0], 21'b0}; 
		end else begin 
			res[21] = 1'b0; 
			numer20 = numer21; 
		end 
 
		if (denom_pad[26:0] <= numer20[46:20]) begin 
			res[20] = 1'b1; 
			numer19 = {numer20[46:20] - denom_pad[26:0], 20'b0}; 
		end else begin 
			res[20] = 1'b0; 
			numer19 = numer20; 
		end 
 
		if (denom_pad[27:0] <= numer19[46:19]) begin 
			res[19] = 1'b1; 
			numer18 = {numer19[46:19] - denom_pad[27:0], 19'b0}; 
		end else begin 
			res[19] = 1'b0; 
			numer18 = numer19; 
		end 
 
		if (denom_pad[28:0] <= numer18[46:18]) begin 
			res[18] = 1'b1; 
			numer17 = {numer18[46:18] - denom_pad[28:0], 18'b0}; 
		end else begin 
			res[18] = 1'b0; 
			numer17 = numer18; 
		end 
 
		if (denom_pad[29:0] <= numer17[46:17]) begin 
			res[17] = 1'b1; 
			numer16 = {numer17[46:17] - denom_pad[29:0], 17'b0}; 
		end else begin 
			res[17] = 1'b0; 
			numer16 = numer17; 
		end 
 
		if (denom_pad[30:0] <= numer16[46:16]) begin 
			res[16] = 1'b1; 
			numer15 = {numer16[46:16] - denom_pad[30:0], 16'b0}; 
		end else begin 
			res[16] = 1'b0; 
			numer15 = numer16; 
		end 
 
		if (denom_pad[31:0] <= numer15[46:15]) begin 
			res[15] = 1'b1; 
			numer14 = {numer15[46:15] - denom_pad[31:0], 15'b0}; 
		end else begin 
			res[15] = 1'b0; 
			numer14 = numer15; 
		end 
 
		if (denom_pad[32:0] <= numer14[46:14]) begin 
			res[14] = 1'b1; 
			numer13 = {numer14[46:14] - denom_pad[32:0], 14'b0}; 
		end else begin 
			res[14] = 1'b0; 
			numer13 = numer14; 
		end 
 
		if (denom_pad[33:0] <= numer13[46:13]) begin 
			res[13] = 1'b1; 
			numer12 = {numer13[46:13] - denom_pad[33:0], 13'b0}; 
		end else begin 
			res[13] = 1'b0; 
			numer12 = numer13; 
		end 
 
		if (denom_pad[34:0] <= numer12[46:12]) begin 
			res[12] = 1'b1; 
			numer11 = {numer12[46:12] - denom_pad[34:0], 12'b0}; 
		end else begin 
			res[12] = 1'b0; 
			numer11 = numer12; 
		end 
 
		if (denom_pad[35:0] <= numer11[46:11]) begin 
			res[11] = 1'b1; 
			numer10 = {numer11[46:11] - denom_pad[35:0], 11'b0}; 
		end else begin 
			res[11] = 1'b0; 
			numer10 = numer11; 
		end 
 
		if (denom_pad[36:0] <= numer10[46:10]) begin 
			res[10] = 1'b1; 
			numer9 = {numer10[46:10] - denom_pad[36:0], 10'b0}; 
		end else begin 
			res[10] = 1'b0; 
			numer9 = numer10; 
		end 
 
		if (denom_pad[37:0] <= numer9[46:9]) begin 
			res[9] = 1'b1; 
			numer8 = {numer9[46:9] - denom_pad[37:0], 9'b0}; 
		end else begin 
			res[9] = 1'b0; 
			numer8 = numer9; 
		end 
 
		if (denom_pad[38:0] <= numer8[46:8]) begin 
			res[8] = 1'b1; 
			numer7 = {numer8[46:8] - denom_pad[38:0], 8'b0}; 
		end else begin 
			res[8] = 1'b0; 
			numer7 = numer8; 
		end 
 
		if (denom_pad[39:0] <= numer7[46:7]) begin 
			res[7] = 1'b1; 
			numer6 = {numer7[46:7] - denom_pad[39:0], 7'b0}; 
		end else begin 
			res[7] = 1'b0; 
			numer6 = numer7; 
		end 
 
		if (denom_pad[40:0] <= numer6[46:6]) begin 
			res[6] = 1'b1; 
			numer5 = {numer6[46:6] - denom_pad[40:0], 6'b0}; 
		end else begin 
			res[6] = 1'b0; 
			numer5 = numer6; 
		end 
 
		if (denom_pad[41:0] <= numer5[46:5]) begin 
			res[5] = 1'b1; 
			numer4 = {numer5[46:5] - denom_pad[41:0], 5'b0}; 
		end else begin 
			res[5] = 1'b0; 
			numer4 = numer5; 
		end 
 
		if (denom_pad[42:0] <= numer4[46:4]) begin 
			res[4] = 1'b1; 
			numer3 = {numer4[46:4] - denom_pad[42:0], 4'b0}; 
		end else begin 
			res[4] = 1'b0; 
			numer3 = numer4; 
		end 
 
		if (denom_pad[43:0] <= numer3[46:3]) begin 
			res[3] = 1'b1; 
			numer2 = {numer3[46:3] - denom_pad[43:0], 3'b0}; 
		end else begin 
			res[3] = 1'b0; 
			numer2 = numer3; 
		end 
 
		if (denom_pad[44:0] <= numer2[46:2]) begin 
			res[2] = 1'b1; 
			numer1 = {numer2[46:2] - denom_pad[44:0], 2'b0}; 
		end else begin 
			res[2] = 1'b0; 
			numer1 = numer2; 
		end 
 
		if (denom_pad[45:0] <= numer1[46:1]) begin 
			res[1] = 1'b1; 
			numer0 = {numer1[46:1] - denom_pad[45:0], 1'b0}; 
		end else begin 
			res[1] = 1'b0; 
			numer0 = numer1; 
		end 
  
		if (denom_pad <= numer0) begin  
			res[0] = 1'b1;  
		end else begin  
			res[0] = 1'b0;  
		end 
  
	end  
	  
endmodule   


//////////////////////////////////////////////  
//   
// constants.v  
//  
// Version 1.3  
// Written 7/11/01 David_Harris@hmc.edu & Mark_Phair@hmc.edu  
// Modifed 8/20/01 Mark_Phair@hmc.edu and Justin_Schauer@hmc.edu  
//  
// A set of constants for a parameterized floating point multiplier and adder.  
//  
//////////////////////////////////////////////  
  
//////////////////////////////////////////////  
// FREE VARIABLES  
//////////////////////////////////////////////  
  
// Widths of Fields  
`define WEXP	8  
`define WSIG	23  
`define WFLAG	5  
`define WCONTROL 5  
  
// output flag select (flags[x])  
`define DIVZERO 	0  
`define INVALID 	1  
`define INEXACT 	2  
`define OVERFLOW 	3  
`define UNDERFLOW	4  
  
//////////////////////////////////////////////  
// DEPENDENT VARIABLES  
//////////////////////////////////////////////  
  
`define WIDTH 		32 	//(`WEXP + `WSIG + 1)  
`define PRODWIDTH	48 	//(2 * (`WSIG + 1))  
`define SHIFTWIDTH	96 	//(2 * `PRODWIDTH))  
`define WPRENORM	24	// `WSIG + 1  
`define WEXPSUM		10	// `WEXP + 2  
`define BIAS		127	// (2^(`WEXP)) - 1  
`define WSIGMINUS1	22	// `WSIG - 1, used for rounding  
`define WSHIFTAMT	5	// log2(`WSIG + 1) rounded up  
  
// for trapped over/underflow  
`define UNDERBIAS	192	// 3 * 2 ^ (`WEXP -2)  
`define OVERBIAS	-192	// -`UNDERBIAS  
  
// specialized constants for fpadd  
`define	EXTRASIG	25		// `WSIG+2 this is the amount of precision needed so no  
					// subtraction errors occur  
`define	SHIFT		5		// # bits the max alignment shift will fit in (log2(`WSIG+2)  
					// rounded up to nearest int)  
`define	MAX_EXP		8'b11111110	// the maximum non-infinite exponent,  
					// `WEXP bits, the most significant  
					// `WEXP-1 bits are 1, the LSB is 0  
`define	INF_EXP		8'b11111111	// Infinity exponent, `WEXP bits, all 1  
// Max significand, `WSIG bits, all 1  
`define	MAX_SIG		23'b11111111111111111111111  
`define	WEXP_0		8'b0		// Exponent equals `WEXP'b0  
`define	WEXP_1		8'b1		// Exponent equals one `WEXP'b1  
`define	WSIG_0		23'b0		// Significand equals zero `WSIG'b0  
`define	WSIG_1		23'b1		// Significand equals one `WSIG'b1  
`define	EXTRASIG_0	25'b0		// All result bits for adder zero `EXTRASIG'b0  
  
// specialized constants for fpmul  
`define	MAXSHIFT	24		// `WSIG + 1  
  
// GENERAL SPECIAL NUMBERS - Exp + Significand of special numbers  
// plain NaN `WIDTH-1, all 1  
`define CONSTNAN	{9'b111111111,22'b0}  
// zero `WIDTH-1, all 0  
`define CONSTZERO	31'b0  
// infinity `WEXP all 1, `WSIG all 0  
`define CONSTINFINITY	{8'b11111111, 23'b0}  
// largest number maximum exponent(all 1's - 1) and maximum significand (all 1's)  
`define CONSTLARGEST	{`MAX_EXP, `MAX_SIG}  
`define PRESHIFTZEROS  48'b0 // `PRODWIDTH'b0  
  
//////////////////////////////////////////////  
//   
// fpmul.v  
//  
// Version 1.6  
// Written 07/11/01 David_Harris@hmc.edu & Mark_Phair@hmc.edu  
// Modifed 08/20/01 Mark_Phair@hmc.edu  
//  
// A parameterized floating point multiplier.  
//  
// BLOCK DESCRIPTIONS  
//  
// preprocess 	- general processing, such as zero detection, computing sign, NaN  
//  
// prenorm	- normalize denorms  
//  
// exponent	- sum the exponents, check for tininess before rounding  
//  
// multiply	- multiply the mantissae  
//  
// special	- calculate special cases, such as NaN and infinities  
//  
// shift	- shift the sig and exp if nesc.  
//  
// round	- round product  
//  
// normalize	- normalizes the result if appropriate (i.e. not a denormalized #)  
//  
// flag 	- general flag processing  
//  
// assemble	- assemble results  
//  
//////////////////////////////////////////////  
  
//////////////////////////////////////////////  
// Includes  
//////////////////////////////////////////////  
  
  
  
//////////////////////////////////////////////  
// fpmul module  
//////////////////////////////////////////////  
  
module fpmul(clk, a, b, y_out, control, flags) ; 
	  
	input clk;  
	 
  // external signals 
  input	 [`WIDTH-1:0] 	a, b;		// floating-point inputs 
  output [`WIDTH-1:0] 	y_out;		// floating-point product 
  reg [`WIDTH-1:0] y_out; 
  input  [1:0] control;	// control including rounding mode 
  output [`WFLAG-1:0]	flags;		// DIVZERO, INVALID, INEXACT,  
					// OVERFLOW, UNDERFLOW (defined in constant.v) 
 
	//intermediate y_out 
	wire [`WIDTH-1:0]y; 
					 
  // internal signals 
  wire			multsign;	// sign of product 
  wire			specialsign;	// sign of special 
 
  wire  [`WSIG:0] 	norma;		// normal-form mantissa a, 1 bit larger to hold leading 1 
  wire  [`WSIG:0] 	normb;		// normal-form mantissa b, 1 bit larger to hold leading 1 
 
  wire	[`WEXPSUM-1:0]	expa, expb;	// the two exponents, after prenormalization 
  wire	[`WEXPSUM-1:0] 	expsum;		// sum of exponents (two's complement) 
  wire	[`WEXPSUM-1:0] 	shiftexp;	// shifted exponent 
  wire	[`WEXP-1:0] 	roundexp;	// rounded, correct exponent 
 
  wire	[`PRODWIDTH-1:0] prod;		// product of mantissae 
  wire	[`PRODWIDTH-1:0] normalized;	// Normalized product 
  wire	[`SHIFTWIDTH-1:0] shiftprod;	// shifted product 
  wire	[`WSIG-1:0]	roundprod;	// rounded product 
  wire	[`WIDTH-2:0]	special;	// special case exponent and product 
 
  wire			twoormore;	// product is outside range [1,2) 
  wire			zero;		// zero detected 
  wire			infinity;	// infinity detected 
  wire			aisnan;		// NaN detected in A 
  wire			bisnan;		// NaN detected in B 
  wire			aisdenorm;	// Denormalized number detected in A 
  wire			bisdenorm;	// Denormalized number detected in B 
  wire			specialcase;	// This is a special case 
  wire			specialsigncase; // Use the special case sign 
  wire			roundoverflow;	// overflow in rounding, need to add 1 to exponent 
  wire			invalid;	// invalid operation 
  wire			overflow;	// exponent result too high, standard overflow 
  wire			inexact;	// inexact flag 
  wire			shiftloss;	// lost digits due to a shift, result inaccurate 
  wire	[1:0]		roundmode;	// rounding mode information extracted from control field   
  wire			tiny;		// Result is tiny (denormalized #) after multiplication 
  wire			stilltiny;	// Result is tiny (denormalized #) after rounding 
  wire			denormround;	// rounding occured only because the initial result was 
					//	a denormalized number. This is used to determine 
					//	underflow in cases of denormalized numbers rounding 
					//	up to normalized numbers 
 
  preprocess	preprocesser(a, b, zero, aisnan, bisnan,  
				aisdenorm, bisdenorm, infinity,  
				control, roundmode, sign);   
 
  special	specialer(a, b, special, specialsign, zero,  
				aisnan, bisnan,  
				infinity, invalid,  
				specialcase, specialsigncase); 
   
  prenorm	prenormer(a[`WIDTH-2:0], b[`WIDTH-2:0], norma, normb, expa, expb, aisdenorm, bisdenorm); 
 
  multiply_a	multiplier(norma, normb, prod, twoormore); 
 
  exponent	exponenter(expa, expb, expsum, twoormore, tiny); 
 
  normalize	normalizer(prod, normalized, tiny, twoormore);   
 
  shift		shifter(normalized, expsum, shiftprod,  
			shiftexp, shiftloss); 
 
  round		rounder(shiftprod, shiftexp, shiftloss, 
			roundprod, roundexp,  
			roundmode, sign, tiny, inexact,  
			overflow, stilltiny, denormround); 
 
		// *** To check for tininess before rounding, use tiny 
		//	To check after rounding, use stilltiny 
		// *** for underflow detect: 
		//	To check for inexact result use (inexact | (shiftloss & stilltiny)),  
		//	To check for denormilization loss use (shiftloss & stilltiny) 
//  flag		flager(invalid, overflow, inexact | shiftloss,  
//			shiftloss | inexact, 
//			/* tiny */ (stilltiny | (tiny & denormround)),  
//			specialcase, flags);  
			  
  //ODIN cannot have operations in module instantiations.  
  wire inexact_or_shiftloss;  
  assign inexact_or_shiftloss = inexact | shiftloss;  
  wire shiftloss_or_inexact;  
  assign shiftloss_or_inexact = shiftloss | inexact;  
  wire still_tiny_or_tiny_and_denormround;  
  assign still_tiny_or_tiny_and_denormround = stilltiny | (tiny & denormround);  
  
    flag		flager(invalid, overflow, inexact_or_shiftloss,  
			shiftloss_or_inexact, 
			/* tiny */ stilltiny_or_tiny_and_denormround,  
			specialcase, flags);  
	  
 
  assemble	assembler(roundprod, special, y,  
			sign, specialsign, roundexp,  
			specialcase, specialsigncase, 
			roundmode, flags[`OVERFLOW]); 
			 
	always @ (posedge clk) begin 
		y_out <= y; 
	end 
 
endmodule  
 
  
 
 
module preprocess(a, b, zero, aisnan, bisnan, aisdenorm, bisdenorm, infinity, control, roundmode, sign); 
 
  // external signals 
  input	[`WIDTH-1:0] 	a, b;		// floating-point inputs 
  output 		zero;		// is there a zero? 
  //input	[`WCONTROL-1:0]	control;	// control field  
  input [1:0] control; //the rest is unused, not necessary for ODIN. 
  output [1:0]		roundmode;	// 00 = RN; 01 = RZ; 10 = RP; 11 = RM  
  output		aisnan;		// NaN detected in A 
  output		bisnan;		// NaN detected in B 
  output		aisdenorm;	// denormalized number detected in A 
  output		bisdenorm;	// denormalized number detected in B 
  output		infinity;	// infinity detected in A 
  output		sign;		// sign of product 
 
  // internal signals 
  wire			signa, signb;	// sign of a and b 
  wire [`WEXP-1:0]	expa, expb;	// the exponents of a and b 
  wire [`WSIG-1:0]	siga, sigb;	// the significands of a and b 
  wire			aexpfull;	// the exponent of a is all 1's 
  wire			bexpfull;	// the exponent of b is all 1's 
  wire			aexpzero;	// the exponent of a is all 0's 
  wire			bexpzero;	// the exponent of b is all 0's 
  wire			asigzero;	// the significand of a is all 0's 
  wire			bsigzero;	// the significand of b is all 0's 
 
  // Sign calculation 
  assign signa 		= a[`WIDTH-1]; 
  assign signb 		= b[`WIDTH-1]; 
  assign sign = signa ^ signb; 
 
  // Significand calcuations 
 
  assign siga		= a[`WSIG-1:0]; 
  assign sigb		= b[`WSIG-1:0]; 
  // Are the significands all 0's? 
  assign asigzero	= ~|siga; 
  assign bsigzero	= ~|sigb; 
 
  // Exponent calculations 
 
  assign expa		= a[`WIDTH-2:`WIDTH-`WEXP-1]; 
  assign expb		= b[`WIDTH-2:`WIDTH-`WEXP-1]; 
  // Are the exponents all 0's? 
  assign aexpzero	= ~|expa; 
  assign bexpzero	= ~|expb; 
  // Are the exponents all 1's? 
  assign aexpfull	= &expa; 
  assign bexpfull	= &expb; 
 
  // General calculations 
 
  // Zero Detect 
  assign zero 		= (aexpzero & asigzero) | (bexpzero & bsigzero); 
 
  // NaN detect 
  assign aisnan		= aexpfull & ~asigzero; 
  assign bisnan		= bexpfull & ~bsigzero; 
 
  // Infinity detect 
  assign infinity	= (aexpfull & asigzero) | (bexpfull & bsigzero); 
 
  // Denorm detect 
  assign aisdenorm	= aexpzero & ~asigzero; 
  assign bisdenorm	= bexpzero & ~bsigzero; 
 
  // Round mode extraction 
  assign roundmode	= control[1:0]; 
 
endmodule  
 
module special (a, b, special, specialsign,  
		zero, aisnan, bisnan, infinity,  
		invalid, specialcase, specialsigncase); 
 
  // external signals 
  input	[`WIDTH-1:0] 	a, b;		// floating-point inputs 
  output [`WIDTH-2:0]	special;	// special case output, exp + sig 
  output		specialsign;	// the special-case sign 
  input			zero;		// is there a zero? 
  input			aisnan;		// NaN detected in A 
  input			bisnan;		// NaN detected in B 
  input			infinity;	// infinity detected 
  output		invalid;	// invalid operation 
  output		specialcase;	// this is a special case 
  output		specialsigncase; // Use the special sign 
 
  // internal signals 
  wire			infandzero;	// infinity and zero detected 
  wire	[`WIDTH-2:0]	highernan;	// holds inputed NaN, the higher if two are input, 
					// and dont care if neither a nor b are NaNs 
  wire			aishighernan;	// a is the higher NaN 
 
  assign infandzero	= (infinity & zero); 
 
  //#######SPECIAL ASSIGNMENT###### 
  // #######return higher NaN########## 
  // Use this block if you want to return the higher of two NaNs 
 
  assign aishighernan = (aisnan & ((a[`WSIG-1:0] >= b[`WSIG-1:0]) | ~bisnan)); 
 
  assign highernan[`WIDTH-2:0] = aishighernan ? a[`WIDTH-2:0] : b[`WIDTH-2:0]; 
 
  assign special[`WIDTH-2:0] = (aisnan | bisnan) ? (highernan[`WIDTH-2:0]) :  
			(zero ?  
			(infinity ? (`CONSTNAN) : (`CONSTZERO)) : (`CONSTINFINITY)); 
  // #######return first NaN########## 
  // Use this block to return the first NaN encountered 
//  assign special	= aisnan ? (a[`WIDTH-2:0]) :  
//			(bisnan ? (b[`WIDTH-2:0]) :  
//			(zero ?  
//			(infinity ? (`CONSTNAN) : (`CONSTZERO)) : (`CONSTINFINITY))); 
  //######END SPECIAL ASSIGNMENT####### 
 
  assign specialcase	= zero | aisnan | bisnan | infinity; 
 
  assign invalid	= infandzero; //*** need to include something about signaling NaNs here 
 
  // dont need to check if b is NaN, if it defaults to that point, and b isnt NAN 
  // then it wont be used anyway 
  assign specialsign	= infandzero ? (1'b1) : (aishighernan ? a[`WIDTH-1] : b[`WIDTH-1]); 
 
  assign specialsigncase = infandzero | aisnan | bisnan; 
 
endmodule   
 
module prenorm(a, b, norma, normb, modexpa, modexpb, aisdenorm, bisdenorm); 
 
  //input	[`WIDTH-1:0]	a, b;			// the input floating point numbers  
  input [`WIDTH-2:0] a, b;  //We don't need bit 31 here, unused in ODIN. 
  output [`WSIG:0]	norma, normb;		// the mantissae in normal form 
  output [`WEXPSUM-1:0]	modexpa, modexpb;	// the output exponents, larger to accomodate 
						//	two's complement form 
  input			aisdenorm;		// a is a denormalized number 
  input			bisdenorm;		// b is a denormalized nubmer 
 
  // internal signals 
  wire	[`WEXPSUM-1:0]	expa, expb;		// exponents in two's complement form 
						//	are negative if shifted for a 
						// 	denormalized number 
  wire	[`SHIFT-1:0]	shifta, shiftb; 	// the shift amounts 
  reg [`WSIG:0]	shifteda, shiftedb;	// the shifted significands, used to be wire, changed for ODIN. 
 
  // pull out the exponents 
  assign expa 	= a[`WIDTH-2:`WIDTH-1-`WEXP]; 
  assign expb 	= b[`WIDTH-2:`WIDTH-1-`WEXP]; 
 
  // when breaking appart for paramaterizing: 
  // ### RUN ./prenormshift.pl wsig_in ### 
assign shifta = a[23 - 1] ? 1 : 
                 a[23 - 2] ? 2 : 
                 a[23 - 3] ? 3 : 
                 a[23 - 4] ? 4 : 
                 a[23 - 5] ? 5 : 
                 a[23 - 6] ? 6 : 
                 a[23 - 7] ? 7 : 
                 a[23 - 8] ? 8 : 
                 a[23 - 9] ? 9 : 
                 a[23 - 10] ? 10 : 
                 a[23 - 11] ? 11 : 
                 a[23 - 12] ? 12 : 
                 a[23 - 13] ? 13 : 
                 a[23 - 14] ? 14 : 
                 a[23 - 15] ? 15 : 
                 a[23 - 16] ? 16 : 
                 a[23 - 17] ? 17 : 
                 a[23 - 18] ? 18 : 
                 a[23 - 19] ? 19 : 
                 a[23 - 20] ? 20 : 
                 a[23 - 21] ? 21 : 
                 a[23 - 22] ? 22 : 
                 23; // dont need to check last bit 
// if the second to last isn't 1, then the last one must be 
 
assign shiftb = b[23 - 1] ? 1 : 
                 b[23 - 2] ? 2 : 
                 b[23 - 3] ? 3 : 
                 b[23 - 4] ? 4 : 
                 b[23 - 5] ? 5 : 
                 b[23 - 6] ? 6 : 
                 b[23 - 7] ? 7 : 
                 b[23 - 8] ? 8 : 
                 b[23 - 9] ? 9 : 
                 b[23 - 10] ? 10 : 
                 b[23 - 11] ? 11 : 
                 b[23 - 12] ? 12 : 
                 b[23 - 13] ? 13 : 
                 b[23 - 14] ? 14 : 
                 b[23 - 15] ? 15 : 
                 b[23 - 16] ? 16 : 
                 b[23 - 17] ? 17 : 
                 b[23 - 18] ? 18 : 
                 b[23 - 19] ? 19 : 
                 b[23 - 20] ? 20 : 
                 b[23 - 21] ? 21 : 
                 b[23 - 22] ? 22 : 
                 23; // dont need to check last bit 
// if the second to last isn't 1, then the last one must be 
 
 
 
  // If number is a denorm, the exponent must be  
  //	decremented by the shift amount 
  assign modexpa = aisdenorm ? 1 - shifta : expa;  
  assign modexpb = bisdenorm ? 1 - shiftb : expb;  
 
  // If number is denorm, shift the significand the appropriate amount 
//  assign shifteda = a[`WSIG-1:0] << shifta;  
	//Must have constant shifts for ODIN  
	always @ (shifta or a) begin  
		case (shifta)   
			5'b00001: begin  
				shifteda = a[`WSIG-1:0] << 5'b00001; 
			end 
 
			5'b00010: begin  
				shifteda = a[`WSIG-1:0] << 5'b00010; 
			end 
 
			5'b00011: begin  
				shifteda = a[`WSIG-1:0] << 5'b00011; 
			end 
 
			5'b00100: begin  
				shifteda = a[`WSIG-1:0] << 5'b00100; 
			end 
 
			5'b00101: begin  
				shifteda = a[`WSIG-1:0] << 5'b00101; 
			end 
 
			5'b00110: begin  
				shifteda = a[`WSIG-1:0] << 5'b00110; 
			end 
 
			5'b00111: begin  
				shifteda = a[`WSIG-1:0] << 5'b00111; 
			end 
 
			5'b01000: begin  
				shifteda = a[`WSIG-1:0] << 5'b01000; 
			end 
 
			5'b01001: begin  
				shifteda = a[`WSIG-1:0] << 5'b01001; 
			end 
 
			5'b01010: begin  
				shifteda = a[`WSIG-1:0] << 5'b01010; 
			end 
 
			5'b01011: begin  
				shifteda = a[`WSIG-1:0] << 5'b01011; 
			end 
 
			5'b01100: begin  
				shifteda = a[`WSIG-1:0] << 5'b01100; 
			end 
 
			5'b01101: begin  
				shifteda = a[`WSIG-1:0] << 5'b01101; 
			end 
 
			5'b01110: begin  
				shifteda = a[`WSIG-1:0] << 5'b01110; 
			end 
 
			5'b01111: begin  
				shifteda = a[`WSIG-1:0] << 5'b01111; 
			end 
 
			5'b10000: begin  
				shifteda = a[`WSIG-1:0] << 5'b10000; 
			end 
 
			5'b10001: begin  
				shifteda = a[`WSIG-1:0] << 5'b10001; 
			end 
 
			5'b10010: begin  
				shifteda = a[`WSIG-1:0] << 5'b10010; 
			end 
 
			5'b10011: begin  
				shifteda = a[`WSIG-1:0] << 5'b10011; 
			end 
 
			5'b10100: begin  
				shifteda = a[`WSIG-1:0] << 5'b10100; 
			end 
 
			5'b10101: begin  
				shifteda = a[`WSIG-1:0] << 5'b10101; 
			end 
 
			5'b10110: begin  
				shifteda = a[`WSIG-1:0] << 5'b10110; 
			end 
 
			5'b10111: begin  
				shifteda = a[`WSIG-1:0] << 5'b10111; 
			end 
 
			default: begin //Won't be higher than 23.  
				shifteda = a[`WSIG-1:0];  
			end  
		endcase  
	end  
 
  assign norma 	= aisdenorm ? shifteda : {1'b1, a[`WSIG-1:0]}; 
 
 // assign shiftedb = b[`WSIG-1:0] << shiftb;  
	always @ (shiftb or b) begin  
		case (shiftb)   
			5'b00001: begin  
				shiftedb = b[`WSIG-1:0] << 5'b00001; 
			end 
 
			5'b00010: begin  
				shiftedb = b[`WSIG-1:0] << 5'b00010; 
			end 
 
			5'b00011: begin  
				shiftedb = b[`WSIG-1:0] << 5'b00011; 
			end 
 
			5'b00100: begin  
				shiftedb = b[`WSIG-1:0] << 5'b00100; 
			end 
 
			5'b00101: begin  
				shiftedb = b[`WSIG-1:0] << 5'b00101; 
			end 
 
			5'b00110: begin  
				shiftedb = b[`WSIG-1:0] << 5'b00110; 
			end 
 
			5'b00111: begin  
				shiftedb = b[`WSIG-1:0] << 5'b00111; 
			end 
 
			5'b01000: begin  
				shiftedb = b[`WSIG-1:0] << 5'b01000; 
			end 
 
			5'b01001: begin  
				shiftedb = b[`WSIG-1:0] << 5'b01001; 
			end 
 
			5'b01010: begin  
				shiftedb = b[`WSIG-1:0] << 5'b01010; 
			end 
 
			5'b01011: begin  
				shiftedb = b[`WSIG-1:0] << 5'b01011; 
			end 
 
			5'b01100: begin  
				shiftedb = b[`WSIG-1:0] << 5'b01100; 
			end 
 
			5'b01101: begin  
				shiftedb = b[`WSIG-1:0] << 5'b01101; 
			end 
 
			5'b01110: begin  
				shiftedb = b[`WSIG-1:0] << 5'b01110; 
			end 
 
			5'b01111: begin  
				shiftedb = b[`WSIG-1:0] << 5'b01111; 
			end 
 
			5'b10000: begin  
				shiftedb = b[`WSIG-1:0] << 5'b10000; 
			end 
 
			5'b10001: begin  
				shiftedb = b[`WSIG-1:0] << 5'b10001; 
			end 
 
			5'b10010: begin  
				shiftedb = b[`WSIG-1:0] << 5'b10010; 
			end 
 
			5'b10011: begin  
				shiftedb = b[`WSIG-1:0] << 5'b10011; 
			end 
 
			5'b10100: begin  
				shiftedb = b[`WSIG-1:0] << 5'b10100; 
			end 
 
			5'b10101: begin  
				shiftedb = b[`WSIG-1:0] << 5'b10101; 
			end 
 
			5'b10110: begin  
				shiftedb = b[`WSIG-1:0] << 5'b10110; 
			end 
 
			5'b10111: begin  
				shiftedb = b[`WSIG-1:0] << 5'b10111; 
			end 
  
			default: begin // Won't be higher than 23.  
				shiftedb = b[`WSIG-1:0];  
			end  
		endcase  
	end 
		  
  
  assign normb 	= bisdenorm ? shiftedb : {1'b1, b[`WSIG-1:0]}; 
 
endmodule  
 
module multiply_a (norma, normb, prod, twoormore); 
 
  input	 [`WSIG:0]		norma, normb;	// normalized mantissae 
 
  output [`PRODWIDTH-1:0] 	prod;		// product of mantissae 
  output			twoormore;	// Product overflowed range [1,2) 
 
  // multiplier array  
  //	(*** need a more effecient multiplier,  
  //	designware might work, though) 
  assign prod		= norma * normb; 
 
  // did the multiply overflow the range [1,2)? 
  assign twoormore	= prod[`PRODWIDTH-1]; 
 
endmodule   
 
 
  
module exponent(expa, expb, expsum, twoormore, tiny); 
 
  input	[`WEXPSUM-1:0]	expa, expb;	// the input exponents in 2's complement form 
					//	to accomodate denorms that have been 
					//	prenormalized 
  input			twoormore;	// product is outside range [1,2) 
 
  output [`WEXPSUM-1:0]	expsum;		// the sum of the exponents 
  output		tiny;		// Result is tiny (denormalized #) 
 
  // Sum the exponents, subtract the bias 
  // 	and add 1 (twoormore) if multiply went out of [1,2) range 
  assign expsum = expa + expb - `BIAS + twoormore; 
 
  // The result is tiny if the exponent is less than 1. 
  //	Because the exponent sum is in 2's-complement form, 
  //	it is negative if the first bit is 1, and zero if 
  //    all the bits are zero 
  assign tiny	= ~|expsum[`WEXPSUM-2:0] | expsum[`WEXPSUM-1]; 
 
 
endmodule  
  
 
 
  
module normalize(prod, normalized, tiny, twoormore); 
 
  // external signals 
  input  [`PRODWIDTH-1:0]	prod;		// Product of multiplication 
  output [`PRODWIDTH-1:0]	normalized;	// Normalized product 
  input				tiny;		// Result is tiny (denormalized #) 
  input				twoormore;	// Product overflowed range [1,2) 
 
  // normalize product if appropriate 
  //	There are three possible cases here: 
  //	1) tiny and prod overfl. [1,2)	-> take the whole prod, including the leading 1 
  //	2) tiny or prod overfl. [1,2)	-> dont take the first bit. its zero if its tiny, 
  //				 		and it's the implied 1 if its not 
  //	3) neither tiny nor prod overfl.-> dont take the first 2 bits, the 2nd one is the 
  //						implied 1 
  assign normalized = (tiny & twoormore) ? prod[`PRODWIDTH-1:0] : 
			((tiny ^ twoormore) ? {prod[`PRODWIDTH-2:0],1'b0} : 
			{prod[`PRODWIDTH-3:0],2'b0}); 
 
endmodule   
 
module shift(normalized, selectedexp, shiftprod, shiftexp, shiftloss); 
 
  // external signals 
  input	[`PRODWIDTH-1:0] normalized;	// normalized product of mantissae 
  input	[`WEXPSUM-1:0] 	selectedexp;	// sum of exponents 
  output [`SHIFTWIDTH-1:0] shiftprod;	// shifted and normalized product 
  output [`WEXPSUM-1:0]	shiftexp;	// shifted exponent 
  output		shiftloss;	// loss of accuaracy due to shifting 
 
  // internal signals 
  wire	[`WEXPSUM-1:0]	roundedexp;		// selected exponent + 1 if rounding caused overflow 
//  wire			negexp;		// exponent is negative 
  wire	[`WEXPSUM-1:0]	shiftamt;		// theoretical amount to shift product by 
  wire	[`WSHIFTAMT-1:0] actualshiftamt;	// actual amount to shift product by 
  wire			tozero;		// need more shifts than possible with width of significand 
  wire			doshift;	// only shift if value is nonnegative 
  wire	[`SHIFTWIDTH-1:0] preshift; 	// value before shifting, with more room to ensure lossless shifting 
  reg	[`SHIFTWIDTH-1:0] postshift;	// value after shifting, with more room to ensure lossless shifting, used to be wire, changed for ODIN. 
 
  // set up value for shifting 
  assign preshift	= {normalized, `PRESHIFTZEROS}; 
 
  // determine shift amount 
  assign shiftamt	=  -selectedexp; 
 
  // make sure shift amount is nonnegative 
  //	If the exponent is negative, the shift amount should 
  //	come out positive, otherwise there shouldn't be any 
  //	shifting to be done 
  assign doshift	= ~shiftamt[`WEXPSUM-1]; 
   
  // Determine if the result must be shifted more than 
  //	will show up in the significand, even if it rounds up 
  assign tozero		= doshift & (shiftamt > `MAXSHIFT); 
 
  // If the shift is big enough to shift all the bits out of the final significand, 
  //	then it stops being relevent how much it has been shifted. 
  assign actualshiftamt	= tozero ? `MAXSHIFT : shiftamt[`WSHIFTAMT-1:0]; 
 
  // shift significand 
  //assign postshift	= preshift >> actualshiftamt;  
  //We can only have constant shifts for ODIN:  
  always @ (actualshiftamt or preshift) begin  
		case (actualshiftamt)   
			5'b00001: begin  
				postshift = preshift >> 5'b00001; 
			end 
 
			5'b00010: begin  
				postshift = preshift >> 5'b00010; 
			end 
 
			5'b00011: begin  
				postshift = preshift >> 5'b00011; 
			end 
 
			5'b00100: begin  
				postshift = preshift >> 5'b00100; 
			end 
 
			5'b00101: begin  
				postshift = preshift >> 5'b00101; 
			end 
 
			5'b00110: begin  
				postshift = preshift >> 5'b00110; 
			end 
 
			5'b00111: begin  
				postshift = preshift >> 5'b00111; 
			end 
 
			5'b01000: begin  
				postshift = preshift >> 5'b01000; 
			end 
 
			5'b01001: begin  
				postshift = preshift >> 5'b01001; 
			end 
 
			5'b01010: begin  
				postshift = preshift >> 5'b01010; 
			end 
 
			5'b01011: begin  
				postshift = preshift >> 5'b01011; 
			end 
 
			5'b01100: begin  
				postshift = preshift >> 5'b01100; 
			end 
 
			5'b01101: begin  
				postshift = preshift >> 5'b01101; 
			end 
 
			5'b01110: begin  
				postshift = preshift >> 5'b01110; 
			end 
 
			5'b01111: begin  
				postshift = preshift >> 5'b01111; 
			end 
 
			5'b10000: begin  
				postshift = preshift >> 5'b10000; 
			end 
 
			5'b10001: begin  
				postshift = preshift >> 5'b10001; 
			end 
 
			5'b10010: begin  
				postshift = preshift >> 5'b10010; 
			end 
 
			5'b10011: begin  
				postshift = preshift >> 5'b10011; 
			end 
 
			5'b10100: begin  
				postshift = preshift >> 5'b10100; 
			end 
 
			5'b10101: begin  
				postshift = preshift >> 5'b10101; 
			end 
 
			5'b10110: begin  
				postshift = preshift >> 5'b10110; 
			end 
 
			5'b10111: begin  
				postshift = preshift >> 5'b10111; 
			end 
 
			5'b11000: begin  
				postshift = preshift >> 5'b11000; 
			end 
 
			5'b11001: begin  
				postshift = preshift >> 5'b11001; 
			end 
 
			5'b11010: begin  
				postshift = preshift >> 5'b11010; 
			end 
 
			5'b11011: begin  
				postshift = preshift >> 5'b11011; 
			end 
 
			5'b11100: begin  
				postshift = preshift >> 5'b11100; 
			end 
 
			5'b11101: begin  
				postshift = preshift >> 5'b11101; 
			end 
 
			5'b11110: begin  
				postshift = preshift >> 5'b11110; 
			end 
 
			5'b11111: begin  
				postshift = preshift >> 5'b11111; 
			end  
		  
			default: begin  
				postshift = preshift;  
			end  
		endcase  
	end 
 
 
  // assign appropriate significand 
  assign shiftprod	= doshift ? postshift :	preshift; 
 
  // determine if any bits were lost from the shift 
  //assign shiftloss	= tozero | (negexp & |postshift[`WSIG-1:0]);  
  assign shiftloss	= tozero | (doshift & |postshift[`SHIFTWIDTH-`PRODWIDTH-1:0]);  
 
  // assign appropriate exponent 
  assign shiftexp	= doshift ? 0 : selectedexp;   
 
endmodule   
  
 
 
module round(shiftprod, shiftexp, shiftloss, roundprod, roundexp, roundmode,  
		sign, tiny, inexact, overflow, stilltiny, denormround); 
 
  // external signals 
  input	[`SHIFTWIDTH-1:0] shiftprod;	// normalized and shifted product of mantissae 
  input [`WEXPSUM-1:0]	shiftexp;	// shifted exponent 
  input			shiftloss;	// bits were lost in the shifting process 
  output [`WSIG-1:0] 	roundprod;	// rounded floating-point product 
  output [`WEXP-1:0] 	roundexp;	// rounded exponent 
  input  [1:0] 		roundmode;	// 00 = RN; 01 = RZ; 10 = RP; 11 = RM 
  input			sign;		// sign bit for rounding mode direction 
  input			tiny;		// denormalized number after rounding 
  output		inexact;	// rounding occured 
  output		overflow;	// overflow occured 
  output		stilltiny;	// Result is tiny (denormalized #) after rounding 
  output		denormround;	// result was rounded only because it was a denormalized number 
 
  // internal signals 
  wire			roundzero;	// rounding towards zero 
  wire			roundinf;	// rounding towards infinity 
  wire 			stickybit;	// there one or more 1 bits in the LS bits 
  wire			denormsticky;	// sticky bit if this weren't a denorm 
  wire [`WSIG-1:0] 	MSBits;		// most significant bits 
  wire [`WSIG:0] 	MSBitsplus1; 	// most significant bits plus 1 
					//	for rounding purposes. needs to be one 
					//	bit bigger for overflow 
  wire [1:0]		roundbits;	// bits used to compute rounding decision 
  wire 			rounddecision;	// round up 
  wire			roundoverflow;	// rounding overflow occured 
  wire [`WEXPSUM-1:0]	tempexp;	// exponent after rounding 
 
  //reduce round mode to three modes 
  //	dont need round nearest, it is implied 
  //	by roundzero and roundinf being false 
  //assign roundnearest 	= ~&roundmode; 
//  assign roundzero	= &roundmode || (^roundmode && (roundmode[0] || sign)); 
  assign roundzero	= (~roundmode[1] & roundmode[0]) | (roundmode[1] & (roundmode[0] ^ sign)); 
  assign roundinf	= roundmode[1] & ~(sign ^ roundmode[0]); 
 
  // pull out the most significant bits for the product 
  assign MSBits = shiftprod[`SHIFTWIDTH-1:`SHIFTWIDTH-`WSIG]; 
 
  // add a 1 to the end of MSBits for round up 
  assign MSBitsplus1 = MSBits + 1; 
 
  // pull out the last of the most significant bits  
  //	and the first of the least significant bits 
  //	to use for calculating the rounding decision 
  assign roundbits[1:0]	= shiftprod[`SHIFTWIDTH-`WSIG:`SHIFTWIDTH-`WSIG-1]; 
 
  // calculate the sticky bit. Are any of the least significant bits 1? 
  //	also: was anything lost while shifting? 
  // *** Optimization: some of these bits are already checked from the shiftloss *** 
  // *** Optimization: stickybit can be calculated from denormsticky  
  //			with only 1 more gate, instead of duplication of effort *** 
  assign stickybit 	= |shiftprod[`SHIFTWIDTH-`WSIG-2:0] | shiftloss; 
  assign denormsticky 	= |shiftprod[`SHIFTWIDTH-`WSIG-3:0] | shiftloss; 
 
  // Compute rounding decision 
  assign rounddecision	= ~roundzero & 	( (roundbits[0]	& (roundinf | roundbits[1])) 
					| (stickybit	& (roundinf | roundbits[0])) 
					); 
 
  // Was this only rounded because it is a denorm? 
  assign denormround	= tiny & rounddecision & ~denormsticky & roundbits[0]; 
 
  // detect rounding overflow. it only overflows if: 
  // 1) the top bit of MSBitsplus1 is 1 
  // 2) it decides to round up 
  assign roundoverflow	= MSBitsplus1[`WSIG] & rounddecision; 
 
  // assign significand (and postnormalize) 
  //  rounddecision decides whether to use msbits+1 or msbits. 
  //  if using msbits+1 and there is an rounding overflow (i.e. result=2), 
  //  then should return 1 instead 
  assign roundprod = rounddecision ?  
 			(roundoverflow ? 0 :  
			MSBitsplus1[`WSIG-1:0]) : 
			MSBits; 
 
  // detect inexact 
  assign inexact	= rounddecision | stickybit | roundbits[0]; 
 
  // compensate for a rounding overflow 
  assign tempexp 	= roundoverflow + shiftexp; 
 
  // check for overflow in exponent 
  //	overflow occured if the number 
  //	is too large to be represented, 
  //	i.e. can't fit in `WEXP bits, or 
  //	all `WEXP bits are 1's 
  assign overflow	= &tempexp[`WEXP-1:0] | |tempexp[`WEXPSUM-1:`WEXP]; 
 
  // two possible cases: 
  //	1) Overflow: then exponent doesnt matter, 
  //	it will be changed to infinity anyway 
  //	2) not overflow: the leading bits will be 0 
  assign roundexp	= tempexp[`WEXP-1:0]; 
 
  // The result is tiny if the exponent is less than 1. 
  //	Because the exponent sum is NOT in 2's-complement form, 
  //	it is only less than one if its is zero, i.e. 
  //    all the bits are 0 
  assign stilltiny	= ~|roundexp; 
 
endmodule  
 
 
module flag (invalid, overflow, inexact, underflow, tiny, specialcase, flags); 
 
  input			invalid;	// invalid operation 
  input			overflow;	// the result was too large 
  input			inexact;	// The result was rounded 
  input			specialcase;	// Using special result, shouldn't throw flags 
  input			underflow;	// Underflow detected 
  input			tiny;		// The result is tiny 
 
  output [`WFLAG-1:0]	flags;		// DIVZERO, INVALID, INEXACT,  
					// OVERFLOW, UNDERFLOW (defined in constant.v) 
 
  // flags 
  assign flags[`DIVZERO]	= 1'b0; 
  assign flags[`INVALID]	= invalid; 
  assign flags[`INEXACT]	= ~specialcase & (inexact | underflow | overflow); 
  assign flags[`OVERFLOW]	= ~specialcase & overflow; 
  assign flags[`UNDERFLOW]	= tiny; //~specialcase & tiny & underflow & ~overflow; 
 
endmodule  
 
module assemble(roundprod, special, y, sign, specialsign,  
		shiftexp, specialcase, specialsigncase, 
		roundmode, overflow); 
 
  // external signals 
  input	[`WSIG-1:0] 	roundprod;	// shifted, rounded and normalized  
					// 	product of mantissae 
  input	[`WIDTH-2:0]	special;	// special case product + exponent 
  output [`WIDTH-1:0] 	y;		// floating-point product 
  input			sign;		// sign of product (+ = 0, - = 1) 
  input			specialsign;	// special case sign 
  input	[`WEXP-1:0] 	shiftexp;	// shifted exponent 
  input			specialcase;	// this is a special case 
  input			specialsigncase; // use the special case sign 
  input	[1:0]		roundmode;	// rounding mode information extracted from control field   
  input			overflow;	// overflow detected 
   
  // internal signals 
  wire	[`WIDTH-2:0]	rounded;	// final product + exponent 
  wire	[`WIDTH-2:0]	overflowvalue;	// product + exponent for overflow condition 
  wire			undenormed;	// the result was denormalized before rounding, but rounding  
					//	caused it to become a small normalized number. 
 
  // SET UP ROUNDED PRODUCT + EXPONENT 
   
  // assign significand 
  assign rounded[`WSIG-1:0]	= roundprod; 
 
  // assign exponent 
  assign rounded[`WIDTH-2:`WIDTH-`WEXP-1] = shiftexp; 
 
  // SET UP OVERFLOW CONDITION 
  assign overflowvalue[`WIDTH-2:0] = roundmode[1] ?  
				(sign ^ roundmode[0] ? `CONSTLARGEST : `CONSTINFINITY) : 
				(roundmode[0] ? `CONSTLARGEST: `CONSTINFINITY); 
 
  // FINAL PRODUCT ASSIGN  
 
  // assign sign 
  assign y[`WIDTH-1]	= specialsigncase ? specialsign : sign; 
 
  // assign product vs special vs overflowed 
  assign y[`WIDTH-2:0]	= specialcase ? special[`WIDTH-2:0] : 
				(overflow ? overflowvalue[`WIDTH-2:0] : 
				rounded[`WIDTH-2:0]); 
 
endmodule 
