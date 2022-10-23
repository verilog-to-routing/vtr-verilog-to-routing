//////////////////////////////////////////////////////////////////////////////
// Author: Aman Arora
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// This benchmark is derived from the RTL provided with DNNWeaver 2.0 (http://dnnweaver.org/)
//////////////////////////////////////////////////////////////////////////////
`timescale 1ns/1ps
module ram
#(
  parameter integer DATA_WIDTH    = 10,
  parameter integer ADDR_WIDTH    = 12,
  parameter integer OUTPUT_REG    = 0
)
(
  input  wire                         clk,
  input  wire                         reset,

  input  wire                         s_read_req,
  input  wire [ ADDR_WIDTH  -1 : 0 ]  s_read_addr,
  output wire [ DATA_WIDTH  -1 : 0 ]  s_read_data,

  input  wire                         s_write_req,
  input  wire [ ADDR_WIDTH  -1 : 0 ]  s_write_addr,
  input  wire [ DATA_WIDTH  -1 : 0 ]  s_write_data
);

  //reg  [ DATA_WIDTH -1 : 0 ] mem [ 0 : 1<<ADDR_WIDTH ];


  // always @(posedge clk)
  // begin: RAM_WRITE
  //   if (s_write_req)
  //     mem[s_write_addr] <= s_write_data;
  //   else if (s_read_req)
  //     _s_read_data <= mem[s_read_addr];
  // end
  // assign s_read_data = _s_read_data;

	reg [DATA_WIDTH-1:0] _s_write_out;

`ifndef hard_mem

reg [DATA_WIDTH-1:0] ram[(1<<ADDR_WIDTH)-1:0];
reg [ DATA_WIDTH  -1 : 0 ]  _s_read_data;
  
always @ (posedge clk) begin 
  if (s_write_req) begin
      ram[s_write_addr] <= s_write_data;
  end
  _s_read_data <= ram[s_read_addr];
  //_s_write_out <= ram[s_write_addr];
end
  
//always @ (posedge clk) begin 
  //if (!s_read_req) begin
  //    ram[s_read_addr] <= 0;
  //end 
  //else begin
  //_s_read_data <= ram[s_read_addr];
  //end
//end
assign s_read_data = _s_read_data;

`else  
  
  defparam dpram.ADDR_WIDTH = ADDR_WIDTH;
	defparam dpram.DATA_WIDTH = DATA_WIDTH;
	dual_port_ram dpram(
		.clk (clk),
		// consider as write port
		.we1(s_write_req),
		.addr1(s_write_addr),
		.data1(s_write_data),
		.out1(_s_write_out),

		// consider as read port
		.we2(1'b0),
		.addr2(s_read_addr),
		.data2({DATA_WIDTH{1'bx}}),
		.out2(s_read_data)
	);

`endif

  // defparam spram.ADDR_WIDTH = ADDR_WIDTH;
  // defparam spram.DATA_WIDTH = DATA_WIDTH;
  // single_port_ram spram(
	// .we(s_write_req),
	// .addr((s_write_req) ? s_write_addr
	// 		     : (s_read_req) ? s_read_addr
	// 		     		     : {`DATA_WIDTH{1'bx}}),
	// .data(s_write_data),
	// .out(s_read_data),
	// .clk(clk)
  // );

  //generate
  //  if (OUTPUT_REG == 0)
  //    assign s_read_data = mem[s_read_addr];
  //  else begin
  //    reg [DATA_WIDTH-1:0] _s_read_data;
  //    always @(posedge clk)
  //    begin
  //      if (reset)
  //        _s_read_data <= 0;
  //      else if (s_read_req)
  //        _s_read_data <= mem[s_read_addr];
  //    end
  //    assign s_read_data = _s_read_data;
  //  end
  //endgenerate
endmodule



`timescale 1ns/1ps
module fifo
#(  // Parameters
  parameter          DATA_WIDTH                   = 64,
  parameter          INIT                         = "init.mif",
  parameter          ADDR_WIDTH                   = 4,
  parameter          RAM_DEPTH                    = (1 << ADDR_WIDTH),
  parameter          INITIALIZE_FIFO              = "no",
  parameter          TYPE                         = "distributed"
)(  // Ports
  input  wire                                         clk,
  input  wire                                         reset,
  input  wire                                         s_write_req,
  input  wire                                         s_read_req,
  input  wire  [ DATA_WIDTH           -1 : 0 ]        s_write_data,
  output wire  [ DATA_WIDTH           -1 : 0 ]        s_read_data,
  output wire                                         s_read_ready,
  output wire                                         s_write_ready,
  output wire                                         almost_full,
  output wire                                         almost_empty,
  output wire  [ ADDR_WIDTH			    : 0 ]			fifo_count
);

// Port Declarations
// ******************************************************************
// Internal variables
// ******************************************************************
  reg                                         empty;
  reg                                         full;

  reg  [ ADDR_WIDTH              : 0 ]        r_fifo_count;

  reg  [ ADDR_WIDTH           -1 : 0 ]        wr_pointer; //Write Pointer
  reg  [ ADDR_WIDTH           -1 : 0 ]        rd_pointer; //Read Pointer

  reg _almost_full;
  reg _almost_empty;

  //(* ram_style = TYPE *)
  //reg     [DATA_WIDTH   -1 : 0 ]    mem[0:RAM_DEPTH-1];
// ******************************************************************
// FIFO Logic
// ******************************************************************
  initial begin
    if (INITIALIZE_FIFO == "yes") begin
      $readmemh(INIT, mem, 0, RAM_DEPTH-1);
    end
  end

  always @ (r_fifo_count)
  begin : FIFO_STATUS
    empty   = (r_fifo_count == 0);
    full    = (r_fifo_count == RAM_DEPTH);
  end

  always @(posedge clk)
  begin
    if (reset)
      _almost_full <= 1'b0;
    else if (s_write_req && !s_read_req && r_fifo_count == RAM_DEPTH-4)
      _almost_full <= 1'b1;
    else if (~s_write_req && s_read_req && r_fifo_count == RAM_DEPTH-4)
      _almost_full <= 1'b0;
  end
  assign almost_full = _almost_full;

  always @(posedge clk)
  begin
    if (reset)
      _almost_empty <= 1'b0;
    else if (~s_write_req && s_read_req && r_fifo_count == 4)
      _almost_empty <= 1'b1;
    else if (s_write_req && ~s_read_req && r_fifo_count == 4)
      _almost_empty <= 1'b0;
  end
  assign almost_empty = _almost_empty;

  assign s_read_ready = !empty;
  assign s_write_ready = !full;

  always @ (posedge clk)
  begin : FIFO_COUNTER
    if (reset)
      r_fifo_count <= 0;

    else if (s_write_req && (!s_read_req||s_read_req&&empty) && !full)
      r_fifo_count <= r_fifo_count + 1;

    else if (s_read_req && (!s_write_req||s_write_req&&full) && !empty)
      r_fifo_count <= r_fifo_count - 1;
  end

  always @ (posedge clk)
  begin : WRITE_PTR
    if (reset) begin
      wr_pointer <= 0;
    end
    else if (s_write_req && !full) begin
      wr_pointer <= wr_pointer + 1;
    end
  end

  always @ (posedge clk)
  begin : READ_PTR
    if (reset) begin
      rd_pointer <= 0;
    end
    else if (s_read_req && !empty) begin
      rd_pointer <= rd_pointer + 1;
    end
  end

  //always @ (posedge clk)
  //begin : WRITE
  //  if (s_write_req & !full) begin
  //    mem[wr_pointer] <= s_write_data;
  //  end
  //end

  //always @ (posedge clk)
  //begin : READ
  //  if (reset) begin
  //    s_read_data <= 0;
  //  end
  //  if (s_read_req && !empty) begin
  //    s_read_data <= mem[rd_pointer];
  //  end
  //  else begin
  //    s_read_data <= s_read_data;
  //  end
  //end

  ram #(
    .ADDR_WIDTH                     ( $clog2(RAM_DEPTH)),
    .DATA_WIDTH                     ( DATA_WIDTH)
  ) u_ram_fifo(
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .s_write_addr                   ( wr_pointer),
    .s_write_req                    ( s_write_req & !full),
    .s_write_data                   ( s_write_data),
    .s_read_addr                    ( rd_pointer),
    .s_read_req                     ( s_read_req && !empty ),
    .s_read_data                    ( s_read_data)
  );
  
  assign fifo_count = r_fifo_count;

endmodule


`timescale 1ns/1ps
module fifo_asymmetric
#(  // Parameters
    parameter integer  WR_DATA_WIDTH                = 64,
    parameter integer  RD_DATA_WIDTH                = 64,
    parameter integer  WR_ADDR_WIDTH                = 4,
    parameter integer  RD_ADDR_WIDTH                = 4,
	 parameter integer  ADDR_WIDTH                   = RD_DATA_WIDTH < WR_DATA_WIDTH ? WR_ADDR_WIDTH : RD_ADDR_WIDTH
)(  // Ports
    input  wire                                         clk,
    input  wire                                         reset,
    input  wire                                         s_write_req,
    input  wire                                         s_read_req,
    input  wire  [ WR_DATA_WIDTH        -1 : 0 ]        s_write_data,
    output wire  [ RD_DATA_WIDTH        -1 : 0 ]        s_read_data,
    output wire                                         s_read_ready,
    output wire                                         s_write_ready,
    output wire                                         almost_full,
    output wire                                         almost_empty,
	 output wire  [ ADDR_WIDTH              : 0 ]        fifo_count
);

    localparam          NUM_FIFO                     = RD_DATA_WIDTH < WR_DATA_WIDTH ? WR_DATA_WIDTH / RD_DATA_WIDTH : RD_DATA_WIDTH / WR_DATA_WIDTH;
    localparam          FIFO_ID_W                    = $clog2(NUM_FIFO);
    

    wire [ NUM_FIFO             -1 : 0 ]        local_s_write_ready;
    wire [ NUM_FIFO             -1 : 0 ]        local_almost_full;
    wire [ NUM_FIFO             -1 : 0 ]        local_s_read_ready;
    wire [ NUM_FIFO             -1 : 0 ]        local_almost_empty;
	 wire [ NUM_FIFO * (ADDR_WIDTH + 1) - 1  : 0 ]        local_fifo_count;

genvar i;

generate
if (WR_DATA_WIDTH > RD_DATA_WIDTH) begin: WR_GT_RD
	reg  [ FIFO_ID_W            -1 : 0 ]        rd_ptr;
	reg  [ FIFO_ID_W            -1 : 0 ]        rd_ptr_dly;

	
	assign s_read_ready = local_s_read_ready[rd_ptr];
	assign s_write_ready = &local_s_write_ready;
	assign almost_empty = local_almost_empty[rd_ptr];
	assign almost_full = |local_almost_full;

	always @(posedge clk) begin
	 if (reset)
		rd_ptr <= 0;
	 else if (s_read_req && s_read_ready) begin
		if (rd_ptr == NUM_FIFO-1)
		  rd_ptr <= 0;
		else
		  rd_ptr <= rd_ptr + 1'b1;
	 end
	end

	always @(posedge clk) begin
	 if (s_read_req && s_read_ready)
		rd_ptr_dly <= rd_ptr;
	end

	for (i=0; i<NUM_FIFO; i=i+1) begin: FIFO_INST
		 wire [ RD_DATA_WIDTH        -1 : 0 ]        _s_write_data;
		 wire                                        _s_write_req;
		 wire                                        _s_write_ready;
		 wire                                        _almost_full;

		 wire [ RD_DATA_WIDTH        -1 : 0 ]        _s_read_data;
		 wire                                        _s_read_req;
		 wire                                        _s_read_ready;
		 wire                                        _almost_empty;
		 
		 wire [ADDR_WIDTH : 0] _fifo_count;

		 assign _s_write_req = s_write_req;
		 assign _s_write_data = s_write_data[i*RD_DATA_WIDTH+:RD_DATA_WIDTH];
		 assign local_s_write_ready[i] = _s_write_ready;
		 assign local_almost_full[i] = _almost_full;

		 assign _s_read_req = s_read_req && (rd_ptr == i);
		 assign s_read_data = rd_ptr_dly == i ? _s_read_data : 'b0;
		 assign local_s_read_ready[i] = _s_read_ready;
		 assign local_almost_empty[i] = _almost_empty;
		 
		 assign local_fifo_count[(i+1)*(ADDR_WIDTH+1)-1:i*(ADDR_WIDTH+1)] = _fifo_count;

		 fifo #(
		 .DATA_WIDTH                     ( RD_DATA_WIDTH                  ),
		 .ADDR_WIDTH                     ( ADDR_WIDTH                     )
		 ) u_fifo (
		 .clk                            ( clk                            ), //input
		 .reset                          ( reset                          ), //input
		 .s_write_req                    ( _s_write_req                   ), //input
		 .s_write_data                   ( _s_write_data                  ), //input
		 .s_write_ready                  ( _s_write_ready                 ), //output
		 .s_read_req                     ( _s_read_req                    ), //input
		 .s_read_ready                   ( _s_read_ready                  ), //output
		 .s_read_data                    ( _s_read_data                   ), //output
		 .almost_full                    ( _almost_full                   ), //output
		 .almost_empty                   ( _almost_empty                  ), //output
		 .fifo_count							( _fifo_count							)
		 );
	end
	assign fifo_count = local_fifo_count[NUM_FIFO*(ADDR_WIDTH+1)-1:(NUM_FIFO-1)*(ADDR_WIDTH+1)];
	//assign fifo_count = FIFO_INST[NUM_FIFO-1].u_fifo.fifo_count;
	
end else begin: RD_GT_WR

	reg  [ FIFO_ID_W            -1 : 0 ]        wr_ptr;


	always @(posedge clk) begin
		if (reset)
			wr_ptr <= 0;
		else if (s_write_req && s_write_ready) begin
			if (wr_ptr == NUM_FIFO-1)
			  wr_ptr <= 0;
			else
			  wr_ptr <= wr_ptr + 1'b1;
		end
	end

	assign s_read_ready = &local_s_read_ready;
	assign s_write_ready = local_s_write_ready[wr_ptr];
	assign almost_empty = |local_almost_empty;
	assign almost_full = local_almost_full[wr_ptr];

	for (i=0; i<NUM_FIFO; i=i+1) begin: FIFO_INST
		 wire [ WR_DATA_WIDTH        -1 : 0 ]        _s_write_data;
		 wire                                        _s_write_req;
		 wire                                        _s_write_ready;
		 wire                                        _almost_full;

		 wire [ WR_DATA_WIDTH        -1 : 0 ]        _s_read_data;
		 wire                                        _s_read_req;
		 wire                                        _s_read_ready;
		 wire                                        _almost_empty;
		 wire [ADDR_WIDTH:0] _fifo_count;

		 assign _s_write_req = s_write_req && (wr_ptr == i);
		 assign _s_write_data = s_write_data;
		 assign local_s_write_ready[i] = _s_write_ready;
		 assign local_almost_full[i] = _almost_full;

		 assign _s_read_req = s_read_req;
		 assign s_read_data[i*WR_DATA_WIDTH+:WR_DATA_WIDTH] = _s_read_data;
		 assign local_s_read_ready[i] = _s_read_ready;
		 assign local_almost_empty[i] = _almost_empty;
		 
		 assign local_fifo_count[(i+1)*(ADDR_WIDTH+1)-1:i*(ADDR_WIDTH+1)] = _fifo_count;

		 fifo #(
		 .DATA_WIDTH                     ( WR_DATA_WIDTH                  ),
		 .ADDR_WIDTH                     ( ADDR_WIDTH                     )
		 ) u_fifo (
		 .clk                            ( clk                            ), //input
		 .reset                          ( reset                          ), //input
		 .s_write_req                    ( _s_write_req                   ), //input
		 .s_write_data                   ( _s_write_data                  ), //input
		 .s_write_ready                  ( _s_write_ready                 ), //output
		 .s_read_req                     ( _s_read_req                    ), //input
		 .s_read_ready                   ( _s_read_ready                  ), //output
		 .s_read_data                    ( _s_read_data                   ), //output
		 .almost_full                    ( _almost_full                   ), //output
		 .almost_empty                   ( _almost_empty                  ), //output
		 .fifo_count							( _fifo_count							)
		 );
	end
	assign fifo_count = local_fifo_count[ADDR_WIDTH:0];
	//assign fifo_count = FIFO_INST[0].u_fifo.fifo_count;
end
endgenerate

endmodule
//
// DnnWeaver2 controller - decoder
//
// Hardik Sharma
// (hsharma@gatech.edu)

`timescale 1ns/1ps
module decoder #(
    parameter integer  IMEM_ADDR_W                  = 10,
    parameter integer  DDR_ADDR_W                   = 42,
  // Internal
    parameter integer  INST_W                       = 32,
    parameter integer  BUF_TYPE_W                   = 2,
    parameter integer  IMM_WIDTH                    = 16,
    parameter integer  OP_CODE_W                    = 4,
    parameter integer  OP_SPEC_W                    = 7,
    parameter integer  LOOP_ID_W                    = 5,
    parameter integer  LOOP_ITER_W                  = IMM_WIDTH,
    parameter integer  ADDR_STRIDE_W                = 2*IMM_WIDTH,
    parameter integer  MEM_REQ_SIZE_W               = IMM_WIDTH,
    parameter integer  STATE_W                      = 3
) (
    input  wire                                         clk,
    input  wire                                         reset,
  // Instruction memory
    input  wire  [ INST_W               -1 : 0 ]        imem_read_data,
    output wire  [ IMEM_ADDR_W          -1 : 0 ]        imem_read_addr,
    output wire                                         imem_read_req,
  // Handshake
    input  wire                                         start,
    output wire                                         done,
    output wire                                         loop_ctrl_start,
    input  wire                                         loop_ctrl_done,
    input  wire                                         block_done,
    output wire                                         last_block,
  // Loop strides
    output wire                                         cfg_loop_iter_v,
    output wire  [ LOOP_ITER_W          -1 : 0 ]        cfg_loop_iter,
    output wire  [ LOOP_ID_W            -1 : 0 ]        cfg_loop_iter_loop_id,
  // Loop strides
    output wire                                         cfg_loop_stride_v,
    output wire  [ 2                    -1 : 0 ]        cfg_loop_stride_type,
    output wire  [ ADDR_STRIDE_W        -1 : 0 ]        cfg_loop_stride,
    output wire  [ LOOP_ID_W            -1 : 0 ]        cfg_loop_stride_loop_id,
    output wire  [ BUF_TYPE_W           -1 : 0 ]        cfg_loop_stride_id,
  // Mem access
    output wire                                         cfg_mem_req_v,
    output wire  [ MEM_REQ_SIZE_W       -1 : 0 ]        cfg_mem_req_size,
    output wire  [ 2                    -1 : 0 ]        cfg_mem_req_type, // 0: RD, 1:WR
    output wire  [ LOOP_ID_W            -1 : 0 ]        cfg_mem_req_loop_id, // specify which scratchpad
    output wire  [ BUF_TYPE_W           -1 : 0 ]        cfg_mem_req_id, // specify which scratchpad
  // DDR Address
    output wire  [ DDR_ADDR_W           -1 : 0 ]        ibuf_base_addr,
    output wire  [ DDR_ADDR_W           -1 : 0 ]        wbuf_base_addr,
    output wire  [ DDR_ADDR_W           -1 : 0 ]        obuf_base_addr,
    output wire  [ DDR_ADDR_W           -1 : 0 ]        bias_base_addr,
  // Buf access
    output wire                                         cfg_buf_req_v,
    output wire  [ MEM_REQ_SIZE_W       -1 : 0 ]        cfg_buf_req_size,
    output wire                                         cfg_buf_req_type, // 0: RD, 1: WR
    output wire  [ BUF_TYPE_W           -1 : 0 ]        cfg_buf_req_loop_id, // specify which scratchpad
  // PU
    output wire  [ INST_W               -1 : 0 ]        cfg_pu_inst, // instructions for PU
    output wire                                         cfg_pu_inst_v,  // instructions for PU
    output wire                                         pu_block_start 
);

//=============================================================
// Localparams
//=============================================================
    localparam integer  FSM_IDLE                     = 0; // IDLE
    localparam integer  FSM_DECODE                   = 1; // Decode and Configure Block
    localparam integer  FSM_PU_BLOCK                 = 2; // Wait for execution of inst block
    localparam integer  FSM_EXECUTE                  = 3; // Wait for execution of inst block
    localparam integer  FSM_NEXT_BLOCK               = 4; // Check for next block
    localparam integer  FSM_DONE_WAIT                = 5; // Wait to ensure no RAW hazard
    localparam integer  FSM_DONE                     = 6; // Done
    
    localparam integer  OP_SETUP                     = 0;
    localparam integer  OP_LDMEM                     = 1;
    localparam integer  OP_STMEM                     = 2;
    localparam integer  OP_RDBUF                     = 3;
    localparam integer  OP_WRBUF                     = 4;
    localparam integer  OP_GENADDR_HI                = 5;
    localparam integer  OP_GENADDR_LO                = 6;
    localparam integer  OP_LOOP                      = 7;
    localparam integer  OP_BLOCK_END                 = 8;
    localparam integer  OP_BASE_ADDR                 = 9;
    localparam integer  OP_PU_BLOCK_START            = 10;
    localparam integer  OP_COMPUTE_R                 = 11;
    localparam integer  OP_COMPUTE_I                 = 12;

    localparam integer  MEM_LOAD                     = 0;
    localparam integer  MEM_STORE                    = 1;
    localparam integer  BUF_READ                     = 0;
    localparam integer  BUF_WRITE                    = 1;
//=============================================================

//=============================================================
// Wires/Regs
//=============================================================
    reg  [ 7                       : 0 ]        done_wait_d;
    reg  [ 7                       : 0 ]        done_wait_q;
    reg  [ IMM_WIDTH            -1 : 0 ]        loop_stride_hi;


    wire                                        pu_block_end;

    wire [ IMM_WIDTH            -1 : 0 ]        pu_num_instructions;

    reg  [ STATE_W              -1 : 0 ]        state_q;
    reg  [ STATE_W              -1 : 0 ]        state_d;
    wire [ STATE_W              -1 : 0 ]        state;

    wire [ OP_CODE_W            -1 : 0 ]        op_code;
    wire [ OP_SPEC_W            -1 : 0 ]        op_spec;
    wire [ LOOP_ID_W            -1 : 0 ]        loop_level;
    wire [ LOOP_ID_W            -1 : 0 ]        loop_id;
    wire [ IMM_WIDTH            -1 : 0 ]        immediate;

    wire [ BUF_TYPE_W           -1 : 0 ]        buf_id;

    wire                                        inst_valid;
    reg                                         _inst_valid;
    wire                                        block_end;

    reg  [ IMM_WIDTH            -1 : 0 ]        pu_inst_counter_d;
    reg  [ IMM_WIDTH            -1 : 0 ]        pu_inst_counter_q;

    reg  [ IMEM_ADDR_W          -1 : 0 ]        addr_d;
    reg  [ IMEM_ADDR_W          -1 : 0 ]        addr_q;

    wire                                        base_addr_v;
    wire [ BUF_TYPE_W           -1 : 0 ]        base_addr_id;
    wire [ 2                    -1 : 0 ]        base_addr_part;
  wire [ IMM_WIDTH + LOOP_ID_W            -1 : 0 ]        base_addr;
//=============================================================

//=============================================================
// Logic
//=============================================================
  // Ops
    assign loop_ctrl_start = block_end;

    assign imem_read_req = state == FSM_DECODE || state == FSM_PU_BLOCK;
    assign imem_read_addr = addr_q;
  always @(posedge clk)
  begin
    if (reset)
      addr_q <= {IMEM_ADDR_W{1'b0}};
    else
      addr_q <= addr_d;
  end

  // Decode instructions
    assign {op_code, op_spec, loop_id, immediate} = imem_read_data;
    assign buf_id = op_spec[5:3];

    assign block_end = op_code == OP_BLOCK_END && _inst_valid && state == FSM_DECODE;

    assign cfg_loop_iter_v = (op_code == OP_LOOP) && inst_valid;
    assign cfg_loop_iter = immediate;
    assign cfg_loop_iter_loop_id = loop_id;

    assign cfg_loop_stride_v = (op_code == OP_GENADDR_LO) && inst_valid;
    assign cfg_loop_stride[IMM_WIDTH-1:0] = immediate;
    assign cfg_loop_stride_id = buf_id;
    assign cfg_loop_stride_type = op_spec[1:0];
    assign cfg_loop_stride_loop_id = loop_id;

    assign cfg_mem_req_v = (op_code == OP_LDMEM || op_code == OP_STMEM) && inst_valid;
    assign cfg_mem_req_size = immediate;
    assign cfg_mem_req_type = op_code == OP_LDMEM ? MEM_LOAD : MEM_STORE;
    assign cfg_mem_req_loop_id = loop_id;
    assign cfg_mem_req_id = buf_id;

    assign cfg_buf_req_v = (op_code == OP_RDBUF || op_code == OP_WRBUF) && inst_valid;
    assign cfg_buf_req_size = immediate;
    assign cfg_buf_req_type = op_code == OP_RDBUF ? BUF_READ : BUF_WRITE;
    assign cfg_buf_req_loop_id = buf_id;

    assign base_addr_v = (op_code == OP_BASE_ADDR) && inst_valid;
    assign base_addr = {loop_id, immediate};
    assign base_addr_id = buf_id;
    assign base_addr_part = op_spec[1:0];

    assign pu_num_instructions = immediate;
    assign pu_block_start = inst_valid && (op_code == OP_PU_BLOCK_START);
    assign pu_block_end = state == FSM_PU_BLOCK && pu_inst_counter_q == 0;
    assign cfg_pu_inst_v = state == FSM_PU_BLOCK;
    assign cfg_pu_inst = imem_read_data;
//=============================================================

//=============================================================
// FSM
//=============================================================
    reg                                         last_block_d;
    reg                                         last_block_q;
    assign last_block = last_block_q;
always @(posedge clk)
begin
  if (reset)
    last_block_q <= 0;
  else
    last_block_q <= last_block_d;
end

always @(posedge clk)
begin
  if (reset)
    done_wait_q <= 0;
  else
    done_wait_q <= done_wait_d;
end


  always @(*)
  begin: FSM
    state_d = state_q;
    addr_d = addr_q;
    pu_inst_counter_d = pu_inst_counter_q;
    last_block_d = last_block_q;
    done_wait_d = done_wait_q;
    case(state_q)
      FSM_IDLE: begin
        if (start) begin
          state_d = FSM_DECODE;
          addr_d = 0;
        end
      end
      FSM_DECODE: begin
        if (loop_ctrl_start) begin
          state_d = FSM_EXECUTE;
          last_block_d = immediate[0];
        end
        else if (pu_block_start) begin
          state_d = FSM_PU_BLOCK;
          addr_d = addr_q + 1'b1;
          pu_inst_counter_d = pu_num_instructions;
        end
        else
          addr_d = addr_q + 1'b1;
      end
      FSM_PU_BLOCK: begin
        addr_d = addr_q + 1'b1;
        if (pu_block_end) begin
          state_d = FSM_DECODE;
        end
        else begin
          pu_inst_counter_d = pu_inst_counter_q - 1'b1;
        end
      end
      FSM_EXECUTE: begin
        if (block_done) begin
          state_d = FSM_NEXT_BLOCK;
        end
      end
      FSM_NEXT_BLOCK: begin
        if (last_block_q) begin
          done_wait_d = 8'd128;
          state_d = FSM_DONE_WAIT;
        end
        else begin
          state_d = FSM_DECODE;
        end
      end
      FSM_DONE_WAIT: begin
        if (done_wait_d == 8'd0) begin
          state_d = FSM_DONE;
        end
        else
          done_wait_d = done_wait_d - 1'b1;
      end
      FSM_DONE: begin
        state_d = FSM_IDLE;
      end
    endcase
  end

    assign done = state_q == FSM_DONE;

  always @(posedge clk)
  begin
    if (reset)
      _inst_valid <= 1'b0;
    else
      _inst_valid <= imem_read_req;
  end
    assign inst_valid = _inst_valid && !block_end && state == FSM_DECODE;

  always @(posedge clk)
  begin
    if (reset)
      pu_inst_counter_q <= 0;
    else
      pu_inst_counter_q <= pu_inst_counter_d;
  end

  always @(posedge clk)
  begin
    if (reset)
      state_q <= FSM_IDLE;
    else
      state_q <= state_d;
  end

    assign state = state_q;
//=============================================================

//=============================================================
// Base Address
//=============================================================
  genvar i;
  generate
    for (i=0; i<2; i=i+1)
    begin: BASE_ADDR_CFG

    reg  [ 21                   -1 : 0 ]        _obuf_base_addr;
    reg  [ 21                   -1 : 0 ]        _bias_base_addr;
    reg  [ 21                   -1 : 0 ]        _ibuf_base_addr;
    reg  [ 21                   -1 : 0 ]        _wbuf_base_addr;

    assign ibuf_base_addr[i*21+:21] = _ibuf_base_addr;
    assign wbuf_base_addr[i*21+:21] = _wbuf_base_addr;
    assign obuf_base_addr[i*21+:21] = _obuf_base_addr;
    assign bias_base_addr[i*21+:21] = _bias_base_addr;

      always @(posedge clk)
        if (reset)
          _ibuf_base_addr <= 0;
        else if (base_addr_v && base_addr_id == 0 && base_addr_part == i)
          _ibuf_base_addr <= base_addr;
        else if (block_done)
          _ibuf_base_addr <= 0;

      always @(posedge clk)
        if (reset)
          _wbuf_base_addr <= 0;
        else if (base_addr_v && base_addr_id == 2 && base_addr_part == i)
          _wbuf_base_addr <= base_addr;
        else if (block_done)
          _wbuf_base_addr <= 0;

      always @(posedge clk)
        if (reset)
          _obuf_base_addr <= 0;
        else if (base_addr_v && base_addr_id == 1 && base_addr_part == i)
          _obuf_base_addr <= base_addr;
        else if (block_done)
          _obuf_base_addr <= 0;

      always @(posedge clk)
        if (reset)
          _bias_base_addr <= 0;
        else if (base_addr_v && base_addr_id == 3 && base_addr_part == i)
          _bias_base_addr <= base_addr;
        else if (block_done)
          _bias_base_addr <= 0;

    end
  endgenerate

  always @(posedge clk)
  begin
    if (reset)
      loop_stride_hi <= 0;
    else begin
      if (cfg_loop_stride_v || block_done)
        loop_stride_hi <= 0;
      else if (op_code == OP_GENADDR_HI && inst_valid)
        loop_stride_hi <= immediate;
    end
  end

    assign cfg_loop_stride[ADDR_STRIDE_W-1:IMM_WIDTH] = loop_stride_hi;
//=============================================================

//=============================================================
// VCD
//=============================================================
  `ifdef COCOTB_TOPLEVEL_decoder
    initial begin
    $dumpfile("decoder.vcd");
    $dumpvars(0, decoder);
    end
  `endif
//=============================================================

endmodule
//
// 2:1 Mux
//
// Hardik Sharma
// (hsharma@gatech.edu)

`timescale 1ns/1ps
module mux_2_1 #(
  parameter integer WIDTH     = 8,        // Data Width
  parameter integer IN_WIDTH  = 2*WIDTH,  // Input Width = 2 * Data Width
  parameter integer OUT_WIDTH = WIDTH     // Output Width
) (
  input  wire                                     sel,
  input  wire        [ IN_WIDTH       -1 : 0 ]    data_in,
  output wire        [ OUT_WIDTH      -1 : 0 ]    data_out
);

assign data_out = sel ? data_in[WIDTH+:WIDTH] : data_in[0+:WIDTH];

endmodule
//
// 2-D systolic array
//
// Hardik Sharma
// (hsharma@gatech.edu)

`timescale 1ns/1ps
module systolic_array #(
    parameter integer  ARRAY_N                      = 4,
    parameter integer  ARRAY_M                      = 4,
    parameter          DTYPE                        = "FXP", // FXP for Fixed-point, FP32 for single precision, FP16 for half-precision

    parameter integer  ACT_WIDTH                    = 16,
    parameter integer  WGT_WIDTH                    = 16,
    parameter integer  BIAS_WIDTH                   = 32,
    parameter integer  ACC_WIDTH                    = 48,

      // General
    parameter integer  MULT_OUT_WIDTH               = ACT_WIDTH + WGT_WIDTH,
    parameter integer  PE_OUT_WIDTH                 = MULT_OUT_WIDTH + $clog2(ARRAY_N),

    parameter integer  SYSTOLIC_OUT_WIDTH           = ARRAY_M * ACC_WIDTH,
    parameter integer  IBUF_DATA_WIDTH              = ARRAY_N * ACT_WIDTH,
    parameter integer  WBUF_DATA_WIDTH              = ARRAY_N * ARRAY_M * WGT_WIDTH,
    parameter integer  OUT_WIDTH                    = ARRAY_M * ACC_WIDTH,
    parameter integer  BBUF_DATA_WIDTH              = ARRAY_M * BIAS_WIDTH,

  // Address for buffers
    parameter integer  OBUF_ADDR_WIDTH              = 16,
    parameter integer  BBUF_ADDR_WIDTH              = 16
        ) (
    input  wire                                         clk,
    input  wire                                         reset,

    input  wire                                         acc_clear,

    input  wire  [ IBUF_DATA_WIDTH      -1 : 0 ]        ibuf_read_data,

    output wire                                         sys_bias_read_req,
    output wire  [ BBUF_ADDR_WIDTH      -1 : 0 ]        sys_bias_read_addr,
    input  wire                                         bias_read_req,
    input  wire  [ BBUF_ADDR_WIDTH      -1 : 0 ]        bias_read_addr,
    input  wire  [ BBUF_DATA_WIDTH      -1 : 0 ]        bbuf_read_data,
    input  wire                                         bias_prev_sw,

    input  wire  [ WBUF_DATA_WIDTH      -1 : 0 ]        wbuf_read_data,
    input  wire  [ OUT_WIDTH            -1 : 0 ]        obuf_read_data,
    input  wire  [ OBUF_ADDR_WIDTH      -1 : 0 ]        obuf_read_addr,
    output wire                                         sys_obuf_read_req,
    output wire  [ OBUF_ADDR_WIDTH      -1 : 0 ]        sys_obuf_read_addr,
    input  wire                                         obuf_write_req,
    output wire  [ OUT_WIDTH            -1 : 0 ]        obuf_write_data,
    input  wire  [ OBUF_ADDR_WIDTH      -1 : 0 ]        obuf_write_addr,
    output wire                                         sys_obuf_write_req,
    output wire  [ OBUF_ADDR_WIDTH      -1 : 0 ]        sys_obuf_write_addr
);

//=========================================
// Localparams
//=========================================
//=========================================
// Wires and Regs
//=========================================

  //FSM to see if we can accumulate or not
    reg  [ 2                    -1 : 0 ]        acc_state_d;
    reg  [ 2                    -1 : 0 ]        acc_state_q;


    wire [ OUT_WIDTH            -1 : 0 ]        accumulator_out;
    wire                                        acc_out_valid;
    wire [ ARRAY_M              -1 : 0 ]        acc_out_valid_;
    wire                                        acc_out_valid_all;
    wire [ SYSTOLIC_OUT_WIDTH   -1 : 0 ]        systolic_out;

    wire [ ARRAY_M              -1 : 0 ]        systolic_out_valid;
    wire [ ARRAY_N              -1 : 0 ]        _systolic_out_valid;

    wire [ OBUF_ADDR_WIDTH      -1 : 0 ]        systolic_out_addr;
    wire [ OBUF_ADDR_WIDTH      -1 : 0 ]        _systolic_out_addr;

    wire                                        _addr_eq;
    reg                                         addr_eq;
    wire [ ARRAY_N              -1 : 0 ]        _acc;
    wire [ ARRAY_M              -1 : 0 ]        acc;
    wire [ OBUF_ADDR_WIDTH      -1 : 0 ]        _systolic_in_addr;

    wire [ BBUF_ADDR_WIDTH      -1 : 0 ]        _bias_read_addr;
    wire                                        _bias_read_req;

    wire [ ARRAY_M              -1 : 0 ]        systolic_acc_clear;
    wire [ ARRAY_M              -1 : 0 ]        _systolic_acc_clear;
//=========================================
// Systolic Array - Begin
//=========================================
// TODO: Add groups
genvar n, m;
generate
for (m=0; m<ARRAY_M; m=m+1)
begin: LOOP_INPUT_FORWARD
for (n=0; n<ARRAY_N; n=n+1)
begin: LOOP_OUTPUT_FORWARD

    wire [ ACT_WIDTH            -1 : 0 ]        a;       // Input Operand a
    wire [ WGT_WIDTH            -1 : 0 ]        b;       // Input Operand b
    wire [ PE_OUT_WIDTH         -1 : 0 ]        pe_out;  // Output of signed spatial multiplier
    wire [ PE_OUT_WIDTH         -1 : 0 ]        c;       // Output  of mac

  //==============================================================
  // Operands for the parametric PE
  // Operands are delayed by a cycle when forwarding
  if (m == 0)
  begin
    assign a = ibuf_read_data[n*ACT_WIDTH+:ACT_WIDTH];
  end
  else
  begin
    wire [ ACT_WIDTH            -1 : 0 ]        fwd_a;
    assign fwd_a = LOOP_INPUT_FORWARD[m-1].LOOP_OUTPUT_FORWARD[n].a;
    // register_sync #(ACT_WIDTH) fwd_a_reg (clk, reset, fwd_a, a);
    assign a = fwd_a;
  end

    assign b = wbuf_read_data[(m+n*ARRAY_M)*WGT_WIDTH+:WGT_WIDTH];
  //==============================================================

  wire [1:0] prev_level_mode = 0;

    localparam          PE_MODE                      = n == 0 ? "MULT" : "FMA";

  // output forwarding
  if (n == 0)
    assign c = {PE_OUT_WIDTH{1'b0}};
  else
    assign c = LOOP_INPUT_FORWARD[m].LOOP_OUTPUT_FORWARD[n-1].pe_out;

  pe #(
    .PE_MODE                        ( PE_MODE                        ),
    .ACT_WIDTH                      ( ACT_WIDTH                      ),
    .WGT_WIDTH                      ( WGT_WIDTH                      ),
    .PE_OUT_WIDTH                   ( PE_OUT_WIDTH                   )
  ) pe_inst (
    .clk                            ( clk                            ),  // input
    .reset                          ( reset                          ),  // input
    .a                              ( a                              ),  // input
    .b                              ( b                              ),  // input
    .c                              ( c                              ),  // input
    .out                            ( pe_out                         )   // output // pe_out = a * b + c
    );

  if (n == ARRAY_N - 1)
  begin
    assign systolic_out[m*PE_OUT_WIDTH+:PE_OUT_WIDTH] = pe_out;
  end

end
end
endgenerate
//=========================================
// Systolic Array - End
//=========================================

  genvar i;




//=========================================
// Accumulate logic
//=========================================

    reg  [ OBUF_ADDR_WIDTH      -1 : 0 ]        prev_obuf_write_addr;

  always @(posedge clk)
  begin
    if (obuf_write_req)
      prev_obuf_write_addr <= obuf_write_addr;
  end

    localparam integer  ACC_INVALID                  = 0;
    localparam integer  ACC_VALID                    = 1;

  // If the current read address and the previous write address are the same, accumulate
    assign _addr_eq = (obuf_write_addr == prev_obuf_write_addr) && (obuf_write_req) && (acc_state_q != ACC_INVALID);
    wire acc_clear_dly1;
  register_sync #(1) acc_clear_dlyreg (clk, reset, acc_clear, acc_clear_dly1);
  always @(posedge clk)
  begin
    if (reset)
      addr_eq <= 1'b0;
    else
      addr_eq <= _addr_eq;
  end

  always @(*)
  begin
    acc_state_d = acc_state_q;
    case (acc_state_q)
      ACC_INVALID: begin
        if (obuf_write_req)
          acc_state_d = ACC_VALID;
      end
      ACC_VALID: begin
        if (acc_clear_dly1)
          acc_state_d = ACC_INVALID;
      end
    endcase
  end

  always @(posedge clk)
  begin
    if (reset)
      acc_state_q <= ACC_INVALID;
    else
      acc_state_q <= acc_state_d;
  end
//=========================================

//=========================================
// Output assignments
//=========================================

  register_sync #(1) out_valid_delay (clk, reset, obuf_write_req, _systolic_out_valid[0]);
  register_sync #(OBUF_ADDR_WIDTH) out_addr_delay (clk, reset, obuf_write_addr, _systolic_out_addr);
  register_sync #(OBUF_ADDR_WIDTH) in_addr_delay (clk, reset, obuf_read_addr, _systolic_in_addr);

  register_sync #(1) out_acc_delay (clk, reset, addr_eq && _systolic_out_valid, _acc[0]);

  generate
    for (i=1; i<ARRAY_N; i=i+1)
    begin: COL_ACC
      register_sync #(1) out_valid_delay (clk, reset, _acc[i-1], _acc[i]);
    end
    for (i=1; i<ARRAY_M; i=i+1)
    begin: ROW_ACC
      // register_sync #(1) out_valid_delay (clk, reset, acc[i-1], acc[i]);
    assign acc[i] = acc[i-1];
    end
  endgenerate
  //assign acc[0] = _acc[ARRAY_N-1];
  register_sync #(1) acc_delay (clk, reset, _acc[ARRAY_N-1], acc[0]);


  generate
    for (i=1; i<ARRAY_N; i=i+1)
    begin: COL_VALID_OUT
      register_sync #(1) out_valid_delay (clk, reset, _systolic_out_valid[i-1], _systolic_out_valid[i]);
    end
    for (i=1; i<ARRAY_M; i=i+1)
    begin: ROW_VALID_OUT
      register_sync #(1) out_valid_delay (clk, reset, systolic_out_valid[i-1], systolic_out_valid[i]);
    end
  endgenerate
    assign systolic_out_valid[0] = _systolic_out_valid[ARRAY_N-1];


  generate
    for (i=0; i<ARRAY_N+2; i=i+1)
    begin: COL_ADDR_OUT
    wire [ OBUF_ADDR_WIDTH      -1 : 0 ]        prev_addr;
    wire [ OBUF_ADDR_WIDTH      -1 : 0 ]        next_addr;
      if (i==0)
    assign prev_addr = _systolic_out_addr;
      else
    assign prev_addr = COL_ADDR_OUT[i-1].next_addr;
      register_sync #(OBUF_ADDR_WIDTH) out_addr (clk, reset, prev_addr, next_addr);
    end
  endgenerate

    assign sys_obuf_write_addr = COL_ADDR_OUT[ARRAY_N+1].next_addr;


  generate
    for (i=1; i<ARRAY_N; i=i+1)
    begin: COL_ADDR_IN
    wire [ OBUF_ADDR_WIDTH      -1 : 0 ]        prev_addr;
    wire [ OBUF_ADDR_WIDTH      -1 : 0 ]        next_addr;
      if (i==1)
    assign prev_addr = _systolic_in_addr;
      else
    assign prev_addr = COL_ADDR_IN[i-1].next_addr;
      register_sync #(OBUF_ADDR_WIDTH) out_addr (clk, reset, prev_addr, next_addr);
    end
  endgenerate
    assign sys_obuf_read_addr = COL_ADDR_IN[ARRAY_N-1].next_addr;

  // Delay logic for bias reads
  register_sync #(BBUF_ADDR_WIDTH) bias_addr_delay (clk, reset, bias_read_addr, _bias_read_addr);
  register_sync #(1) bias_req_delay (clk, reset, bias_read_req, _bias_read_req);
  generate
    for (i=1; i<ARRAY_N; i=i+1)
    begin: BBUF_COL_ADDR_IN
    wire [ BBUF_ADDR_WIDTH      -1 : 0 ]        prev_addr;
    wire [ BBUF_ADDR_WIDTH      -1 : 0 ]        next_addr;
    wire                                        prev_req;
    wire                                        next_req;
      if (i==1) begin
    assign prev_addr = _bias_read_addr;
    assign prev_req = _bias_read_req;
      end
      else begin
    assign prev_addr = BBUF_COL_ADDR_IN[i-1].next_addr;
    assign prev_req = BBUF_COL_ADDR_IN[i-1].next_req;
      end
      register_sync #(BBUF_ADDR_WIDTH) out_addr (clk, reset, prev_addr, next_addr);
      register_sync #(1) out_req (clk, reset, prev_req, next_req);
    end
  endgenerate
    assign sys_bias_read_addr = BBUF_COL_ADDR_IN[ARRAY_N-1].next_addr;
    assign sys_bias_read_req = BBUF_COL_ADDR_IN[ARRAY_N-1].next_req;

  //=========================================


  //=========================================
  // Output assignments
  //=========================================
    assign obuf_write_data = accumulator_out;
    assign sys_obuf_read_req = systolic_out_valid[0];
  register_sync #(1) acc_out_vld (clk, reset, systolic_out_valid[0], acc_out_valid);
    wire                                        _sys_obuf_write_req;
  register_sync #(1) sys_obuf_write_req_delay (clk, reset, acc_out_valid, _sys_obuf_write_req);
  register_sync #(1) _sys_obuf_write_req_delay (clk, reset, _sys_obuf_write_req, sys_obuf_write_req);
  // assign sys_obuf_write_req = acc_out_valid;

    assign acc_out_valid_[0] = acc_out_valid && ~addr_eq;
    assign acc_out_valid_all = |acc_out_valid_;

generate
for (i=1; i<ARRAY_M; i=i+1)
begin: OBUF_VALID_OUT
      register_sync #(1) obuf_output_delay (clk, reset, acc_out_valid_[i-1], acc_out_valid_[i]);
end
endgenerate

    wire [ ARRAY_N              -1 : 0 ]        col_bias_sw;
    wire [ ARRAY_M              -1 : 0 ]        bias_sel;
    wire                                        _bias_sel;
  // assign col_bias_sw[0] = bias_prev_sw;
  register_sync #(1) row_bias_sel_delay (clk, reset, bias_prev_sw, col_bias_sw[0]);
  register_sync #(1) col_bias_sel_delay (clk, reset, col_bias_sw[ARRAY_N-1], _bias_sel);
  register_sync #(1) _bias_sel_delay (clk, reset, _bias_sel, bias_sel[0]);
  generate
    for (i=1; i<ARRAY_N; i=i+1)
    begin: ADD_SRC_SEL_COL
      register_sync #(1) col_bias_sel_delay (clk, reset, col_bias_sw[i-1], col_bias_sw[i]);
    end
    for (i=1; i<ARRAY_M; i=i+1)
    begin: ADD_SRC_SEL
      //register_sync #(1) bias_sel_delay (clk, reset, bias_sel[i-1], bias_sel[i]);
    assign bias_sel[i] = bias_sel[i-1];
    end
  endgenerate

    wire [ ARRAY_M              -1 : 0 ]        acc_enable;
    assign acc_enable[0] = _sys_obuf_write_req;

generate
for (i=1; i<ARRAY_M; i=i+1)
begin: ACC_ENABLE
      //register_sync #(1) acc_enable_delay (clk, reset, acc_enable[i-1], acc_enable[i]);
    assign acc_enable[i] = acc_enable[i-1];
end
endgenerate

//=========================================

//=========================================
// Accumulator
//=========================================
generate
for (i=0; i<ARRAY_M; i=i+1)
begin: ACCUMULATOR

    wire [ ACC_WIDTH            -1 : 0 ]        obuf_in;
    wire [ PE_OUT_WIDTH         -1 : 0 ]        sys_col_out;
    wire [ ACC_WIDTH            -1 : 0 ]        acc_out_q;

    wire                                        local_acc;
    wire                                        local_bias_sel;
    wire                                        local_acc_enable;

    assign local_acc_enable = acc_enable[i];
    assign local_acc = acc[i];
    assign local_bias_sel = bias_sel[i];

    wire [ ACC_WIDTH            -1 : 0 ]        local_bias_data;
    wire [ ACC_WIDTH            -1 : 0 ]        local_obuf_data;

    assign local_bias_data = $signed(bbuf_read_data[BIAS_WIDTH*i+:BIAS_WIDTH]);
    assign local_obuf_data = obuf_read_data[ACC_WIDTH*i+:ACC_WIDTH];

    assign obuf_in = ~local_bias_sel ? local_bias_data : local_obuf_data;
    assign accumulator_out[ACC_WIDTH*i+:ACC_WIDTH] = acc_out_q;
    assign sys_col_out = systolic_out[PE_OUT_WIDTH*i+:PE_OUT_WIDTH];

  wire signed [ ACC_WIDTH    -1 : 0 ]        add_in;
    assign add_in = local_acc ? acc_out_q : obuf_in;

    signed_adder #(
    .DTYPE                          ( DTYPE                          ),
    .REGISTER_OUTPUT                ( "TRUE"                         ),
    .IN1_WIDTH                      ( PE_OUT_WIDTH                   ),
    .IN2_WIDTH                      ( ACC_WIDTH                      ),
    .OUT_WIDTH                      ( ACC_WIDTH                      )
    ) adder_inst (
    .clk                            ( clk                            ),  // input
    .reset                          ( reset                          ),  // input
    .enable                         ( local_acc_enable               ),
    .a                              ( sys_col_out                    ),
    .b                              ( add_in                         ),
    .out                            ( acc_out_q                      )
      );
end
endgenerate
//=========================================

`ifdef COCOTB_TOPLEVEL_systolic_array
  initial begin
    $dumpfile("systolic_array.vcd");
    $dumpvars(0, systolic_array);
  end
`endif

endmodule
`timescale 1ns/1ps
module axi_master
#(
// ******************************************************************
// Parameters
// ******************************************************************
    parameter integer  AXI_ADDR_WIDTH               = 32,
    parameter integer  AXI_DATA_WIDTH               = 32,
    parameter integer  AXI_BURST_WIDTH              = 8,
    parameter integer  BURST_LEN                    = 1 << AXI_BURST_WIDTH,

    parameter integer  AXI_SUPPORTS_WRITE           = 1,
    parameter integer  AXI_SUPPORTS_READ            = 1,

    parameter          TX_SIZE_WIDTH                = 10,
    parameter integer  C_OFFSET_WIDTH               = AXI_ADDR_WIDTH < 16 ? AXI_ADDR_WIDTH - 1 : 16,

    parameter integer  WSTRB_W                      = AXI_DATA_WIDTH/8,

    parameter integer  AXI_ID_WIDTH                 = 1
)
(
// ******************************************************************
// IO
// ******************************************************************
    // System Signals
    input  wire                                         clk,
    input  wire                                         reset,

    // Master Interface Write Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        m_axi_awaddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        m_axi_awlen,
    output wire  [ 3                    -1 : 0 ]        m_axi_awsize,
    output wire  [ 2                    -1 : 0 ]        m_axi_awburst,
    output wire                                         m_axi_awvalid,
    input  wire                                         m_axi_awready,
    // Master Interface Write Data
    output wire  [ AXI_DATA_WIDTH       -1 : 0 ]        m_axi_wdata,
    output wire  [ WSTRB_W              -1 : 0 ]        m_axi_wstrb,
    output wire                                         m_axi_wlast,
    output wire                                         m_axi_wvalid,
    input  wire                                         m_axi_wready,
    // Master Interface Write Response
    input  wire  [ 2                    -1 : 0 ]        m_axi_bresp,
    input  wire                                         m_axi_bvalid,
    output wire                                         m_axi_bready,
    // Master Interface Read Address
    output wire  [ AXI_ID_WIDTH         -1 : 0 ]        m_axi_arid,
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        m_axi_araddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        m_axi_arlen,
    output wire  [ 3                    -1 : 0 ]        m_axi_arsize,
    output wire  [ 2                    -1 : 0 ]        m_axi_arburst,
    output wire                                         m_axi_arvalid,
    input  wire                                         m_axi_arready,
    // Master Interface Read Data
    input  wire  [ AXI_ID_WIDTH         -1 : 0 ]        m_axi_rid,
    input  wire  [ AXI_DATA_WIDTH       -1 : 0 ]        m_axi_rdata,
    input  wire  [ 2                    -1 : 0 ]        m_axi_rresp,
    input  wire                                         m_axi_rlast,
    input  wire                                         m_axi_rvalid,
    output wire                                         m_axi_rready,

    // WRITE from BRAM to DDR
    input  wire                                         mem_read_ready,
    output wire                                         mem_read_req,
    input  wire  [ AXI_DATA_WIDTH       -1 : 0 ]        mem_read_data,
    // READ from DDR to BRAM
    output wire  [ AXI_ID_WIDTH         -1 : 0 ]        mem_write_id,
    output wire  [ AXI_DATA_WIDTH       -1 : 0 ]        mem_write_data,
    output wire                                         mem_write_req,
    input  wire                                         mem_write_ready,

    // Memory Controller Interface - Read
    input  wire  [ AXI_ID_WIDTH         -1 : 0 ]        rd_req_id,
    input  wire                                         rd_req,
    output wire                                         rd_done,
    output wire                                         rd_ready,
    input  wire  [ TX_SIZE_WIDTH        -1 : 0 ]        rd_req_size,
    input  wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        rd_addr,
    // Memory Controller Interface - Write
    input  wire  [ AXI_ID_WIDTH         -1 : 0 ]        wr_req_id,
    input  wire                                         wr_req,
    output wire                                         wr_ready,
    input  wire  [ TX_SIZE_WIDTH        -1 : 0 ]        wr_req_size,
    input  wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        wr_addr,
    output wire                                         wr_done
);

//==============================================================================
// Local parameters
//==============================================================================
    localparam integer  REQ_BUF_DATA_W               = AXI_ADDR_WIDTH + TX_SIZE_WIDTH + AXI_ID_WIDTH;
//==============================================================================

//==============================================================================
// Wires/Regs
//==============================================================================

    reg                                         mem_read_valid_d;
    reg                                         mem_read_valid_q;

    wire                                        rnext;

  // Local address counters
    reg  [ AXI_ID_WIDTH         -1 : 0 ]        arid_d;
    reg  [ AXI_ID_WIDTH         -1 : 0 ]        arid_q;
    reg  [ C_OFFSET_WIDTH       -1 : 0 ]        araddr_offset_d;
    reg  [ C_OFFSET_WIDTH       -1 : 0 ]        araddr_offset_q;
    reg                                         arvalid_d;
    reg                                         arvalid_q;

    reg  [ C_OFFSET_WIDTH       -1 : 0 ]        awaddr_offset_d;
    reg  [ C_OFFSET_WIDTH       -1 : 0 ]        awaddr_offset_q;
    reg                                         awvalid_d;
    reg                                         awvalid_q;

    wire                                        rready;



    wire                                        rx_req_id_buf_pop;
    wire                                        rx_req_id_buf_push;
    wire                                        rx_req_id_buf_rd_ready;
    wire                                        rx_req_id_buf_wr_ready;
    wire [ AXI_ID_WIDTH         -1 : 0 ]        rx_req_id_buf_data_in;
    wire [ AXI_ID_WIDTH         -1 : 0 ]        rx_req_id_buf_data_out;

    wire                                        rx_req_id_buf_almost_empty;
    wire                                        rx_req_id_buf_almost_full;


    wire                                        wr_req_buf_pop;
    wire                                        wr_req_buf_push;
    wire                                        wr_req_buf_rd_ready;
    wire                                        wr_req_buf_wr_ready;
    wire [ REQ_BUF_DATA_W       -1 : 0 ]        wr_req_buf_data_in;
    wire [ REQ_BUF_DATA_W       -1 : 0 ]        wr_req_buf_data_out;

    wire                                        wr_req_buf_almost_empty;
    wire                                        wr_req_buf_almost_full;

    wire                                        wdata_req_buf_almost_full;
    wire                                        wdata_req_buf_almost_empty;
    wire                                        wdata_req_buf_pop;
    wire                                        wdata_req_buf_push;
    wire                                        wdata_req_buf_rd_ready;
    wire                                        wdata_req_buf_wr_ready;
    wire [ AXI_BURST_WIDTH      -1 : 0 ]        wdata_req_buf_data_in;
    wire [ AXI_BURST_WIDTH      -1 : 0 ]        wdata_req_buf_data_out;

    wire [ TX_SIZE_WIDTH        -1 : 0 ]        wx_req_size_buf;
    wire [ AXI_ADDR_WIDTH       -1 : 0 ]        wx_addr_buf;

    reg  [ TX_SIZE_WIDTH        -1 : 0 ]        wx_size_d;
    reg  [ TX_SIZE_WIDTH        -1 : 0 ]        wx_size_q;


    wire                                        rd_req_buf_pop;
    wire                                        rd_req_buf_push;
    wire                                        rd_req_buf_rd_ready;
    wire                                        rd_req_buf_wr_ready;
    wire [ REQ_BUF_DATA_W       -1 : 0 ]        rd_req_buf_data_in;
    wire [ REQ_BUF_DATA_W       -1 : 0 ]        rd_req_buf_data_out;
    wire [ TX_SIZE_WIDTH        -1 : 0 ]        rx_req_size_buf;
    wire [ AXI_ID_WIDTH         -1 : 0 ]        rx_req_id;
    wire [ AXI_ADDR_WIDTH       -1 : 0 ]        rx_addr_buf;

    wire                                        rd_req_buf_almost_empty;
    wire                                        rd_req_buf_almost_full;

    reg  [ TX_SIZE_WIDTH        -1 : 0 ]        rx_size_d;
    reg  [ TX_SIZE_WIDTH        -1 : 0 ]        rx_size_q;

  // Reads
    reg  [ AXI_BURST_WIDTH      -1 : 0 ]        arlen_d;
    reg  [ AXI_BURST_WIDTH      -1 : 0 ]        arlen_q;

  // Writes
    reg  [ AXI_BURST_WIDTH      -1 : 0 ]        awlen_d;
    reg  [ AXI_BURST_WIDTH      -1 : 0 ]        awlen_q;

  // Read done
    reg  [ 8                    -1 : 0 ]        axi_outstanding_reads;
    reg                                         rd_done_q;

  // Write done
    reg  [ 8                    -1 : 0 ]        axi_outstanding_writes;
    reg                                         wr_done_q;
//==============================================================================


//==============================================================================
// Tie-offs
//==============================================================================
  // Read Address (AR)
    assign m_axi_arsize = $clog2(AXI_DATA_WIDTH/8);
    assign m_axi_arburst = 2'b01;

    assign m_axi_awsize = $clog2(AXI_DATA_WIDTH/8);
    assign m_axi_awburst = 2'b01;

    assign m_axi_wstrb = {WSTRB_W{1'b1}};

    assign m_axi_bready = 1'b1;

  // Data
    assign mem_write_req  = rnext;
    assign mem_write_data = m_axi_rdata;
    // assign mem_write_id = m_axi_rid; BUG: AXI Smartconnect doesn't respond
    // with correct RID

//==============================================================================


//==============================================================================
// AR channel
//==============================================================================

    localparam integer  AR_IDLE                      = 0;
    localparam integer  AR_REQ_READ                  = 1;
    localparam integer  AR_SEND                      = 2;
    localparam integer  AR_WAIT                      = 3;

    reg  [ 2                    -1 : 0 ]        ar_state_d;
    reg  [ 2                    -1 : 0 ]        ar_state_q;

    assign m_axi_arlen = arlen_q;
    assign m_axi_arvalid = arvalid_q;
    assign m_axi_araddr = {rx_addr_buf[AXI_ADDR_WIDTH-1:C_OFFSET_WIDTH], araddr_offset_q};
    assign m_axi_arid = arid_q;

    assign rd_req_buf_pop       = ar_state_q == AR_IDLE;
    assign rd_req_buf_push      = rd_req;
    assign rd_ready = ~rd_req_buf_almost_full;
    assign rd_req_buf_data_in   = {rd_req_id, rd_req_size, rd_addr};
    assign {rx_req_id, rx_req_size_buf, rx_addr_buf} = rd_req_buf_data_out;


  always @(*)
  begin
    ar_state_d = ar_state_q;
    araddr_offset_d = araddr_offset_q;
    arid_d = arid_q;
    arvalid_d = arvalid_q;
    rx_size_d = rx_size_q;
    arlen_d = arlen_q;
    case(ar_state_q)
      AR_IDLE: begin
        if (rd_req_buf_rd_ready)
          ar_state_d = AR_REQ_READ;
      end
      AR_REQ_READ: begin
        ar_state_d = AR_SEND;
        araddr_offset_d = rx_addr_buf;
        arid_d = rx_req_id;
        rx_size_d = rx_req_size_buf;
      end
      AR_SEND: begin
        if (~rx_req_id_buf_almost_full) begin
          arvalid_d = wdata_req_buf_wr_ready;
          ar_state_d = AR_WAIT;
          arlen_d = (rx_size_q >= BURST_LEN) ? BURST_LEN-1: (rx_size_q-1);
          rx_size_d = rx_size_q >= BURST_LEN ? rx_size_d - BURST_LEN : 0;
        end
      end
      AR_WAIT: begin
        arvalid_d = wdata_req_buf_wr_ready;
        if (m_axi_arvalid && m_axi_arready) begin
          arvalid_d = 1'b0;
          araddr_offset_d = araddr_offset_q + BURST_LEN * AXI_DATA_WIDTH / 8;
          if (rx_size_q == 0) begin
            ar_state_d = AR_IDLE;
          end
          else begin
            ar_state_d = AR_SEND;
          end
        end
      end
    endcase
  end

  always @(posedge clk)
  begin
    if (reset) begin
      arlen_q <= 0;
      arvalid_q <= 1'b0;
      ar_state_q <= AR_IDLE;
      araddr_offset_q  <= 'b0;
      rx_size_q <= 0;
      arid_q <= 0;
    end else begin
      arlen_q <= arlen_d;
      arvalid_q <= arvalid_d;
      ar_state_q <= ar_state_d;
      araddr_offset_q  <= araddr_offset_d;
      rx_size_q <= rx_size_d;
      arid_q <= arid_d;
    end
  end

  //
  //The FIFO stores the read requests
  //
  fifo #(
    .DATA_WIDTH                     ( REQ_BUF_DATA_W                 ),
    .ADDR_WIDTH                     ( 3                              )
  ) rd_req_buf (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .s_read_req                     ( rd_req_buf_pop                 ), //input
    .s_read_ready                   ( rd_req_buf_rd_ready            ), //output
    .s_read_data                    ( rd_req_buf_data_out            ), //output
    .s_write_req                    ( rd_req_buf_push                ), //input
    .s_write_ready                  ( rd_req_buf_wr_ready            ), //output
    .s_write_data                   ( rd_req_buf_data_in             ), //input
    .almost_full                    ( rd_req_buf_almost_full         ), //output
    .almost_empty                   ( rd_req_buf_almost_empty        )  //output
  );
//==============================================================================

//==============================================================================
// Read channel
//==============================================================================

    localparam integer  R_IDLE                       = 0;
    localparam integer  R_READ                       = 1;

    reg                                         r_state_d;
    reg                                         r_state_q;

  assign rx_req_id_buf_push = (ar_state_q == AR_SEND) && ~rx_req_id_buf_almost_full;
  assign rx_req_id_buf_data_in = arid_q;
  assign rx_req_id_buf_pop = r_state_q == R_IDLE;
  assign mem_write_id = rx_req_id_buf_data_out;
  //
  //The FIFO stores the read request IDs
  //
  fifo #(
    .DATA_WIDTH                     ( AXI_ID_WIDTH                   ),
    .ADDR_WIDTH                     ( 5                              )
  ) rx_req_id_buf (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .s_read_req                     ( rx_req_id_buf_pop              ), //input
    .s_read_ready                   ( rx_req_id_buf_rd_ready         ), //output
    .s_read_data                    ( rx_req_id_buf_data_out         ), //output
    .s_write_req                    ( rx_req_id_buf_push             ), //input
    .s_write_ready                  ( rx_req_id_buf_wr_ready         ), //output
    .s_write_data                   ( rx_req_id_buf_data_in          ), //input
    .almost_full                    ( rx_req_id_buf_almost_full      ), //output
    .almost_empty                   ( rx_req_id_buf_almost_empty     )  //output
  );


    always @(*)
    begin
      r_state_d = r_state_q;
      case (r_state_q)
        R_IDLE: begin
          if (rx_req_id_buf_rd_ready)
            r_state_d = R_READ;
        end
        R_READ: begin
          if (m_axi_rready && m_axi_rlast)
            r_state_d = R_IDLE;
        end
      endcase
    end

    always @(posedge clk)
    begin
      if (reset)
        r_state_q <= R_IDLE;
      else
        r_state_q <= r_state_d;
    end

  // Read and Read Response (R)
    assign m_axi_rready = rready;
    assign rready = (AXI_SUPPORTS_READ == 1) && mem_write_ready && r_state_q == R_READ;
    assign rnext = m_axi_rvalid && m_axi_rready;
//==============================================================================

//==============================================================================
// CL - read ready
//==============================================================================
    wire                                        rburst_complete;
    wire                                        rburst_req;
    assign rburst_complete = m_axi_rlast && m_axi_rready;
    assign rburst_req = ar_state_q == AR_SEND && ~rx_req_id_buf_almost_full;

  always @(posedge clk)
  begin
    if (reset)
      axi_outstanding_reads <= 0;
    else if (rburst_req && ~rburst_complete)
        axi_outstanding_reads <= axi_outstanding_reads + 1'b1;
    else if (!rburst_req && rburst_complete)
        axi_outstanding_reads <= axi_outstanding_reads - 1'b1;
  end

  always @(posedge clk)
  begin
    rd_done_q <= (axi_outstanding_reads == 0 && ar_state_q == AR_IDLE);
  end
    assign rd_done = rd_done_q && ~rd_req_buf_rd_ready;
//==============================================================================

//==============================================================================
// CL - write ready
//==============================================================================
    wire                                        wburst_complete;
    wire                                        wburst_req;

    reg  [ 2                    -1 : 0 ]        aw_state_d;
    reg  [ 2                    -1 : 0 ]        aw_state_q;

    localparam integer  AW_IDLE                      = 0;
    localparam integer  AW_REQ_READ                  = 1;
    localparam integer  AW_SEND                      = 2;
    localparam integer  AW_WAIT                      = 3;

    assign wburst_complete = m_axi_wlast && m_axi_wready;
    assign wburst_req = aw_state_q == AW_SEND && ~wdata_req_buf_almost_full;
  always @(posedge clk)
  begin
    if (reset)
      axi_outstanding_writes <= 0;
    else if (wburst_req && ~wburst_complete)
        axi_outstanding_writes <= axi_outstanding_writes + 1'b1;
    else if (!wburst_req && wburst_complete)
        axi_outstanding_writes <= axi_outstanding_writes - 1'b1;
  end

  always @(posedge clk)
  begin
    wr_done_q <= axi_outstanding_writes == 0 && aw_state_q == AW_IDLE;
  end
    assign wr_done = wr_done_q;
//==============================================================================

//==============================================================================
// AW channel
//==============================================================================
    assign wr_req_buf_pop       = aw_state_q == AW_IDLE;
    assign wr_req_buf_push      = wr_req;
    assign wr_ready = ~wr_req_buf_almost_full;
    assign wr_req_buf_data_in   = {wr_req_size, wr_addr};
    assign {wx_req_size_buf, wx_addr_buf} = wr_req_buf_data_out;

  always @(*)
  begin
    aw_state_d = aw_state_q;
    awaddr_offset_d = awaddr_offset_q;
    awvalid_d = awvalid_q;
    wx_size_d = wx_size_q;
    awlen_d = awlen_q;
    case(aw_state_q)
      AW_IDLE: begin
        if (wr_req_buf_rd_ready)
          aw_state_d = AW_REQ_READ;
      end
      AW_REQ_READ: begin
        aw_state_d = AW_SEND;
        awaddr_offset_d = wx_addr_buf;
        wx_size_d = wx_req_size_buf;
      end
      AW_SEND: begin
        if (~wdata_req_buf_almost_full) begin
          awvalid_d = 1'b1;
          aw_state_d = AW_WAIT;
          awlen_d = (wx_size_q >= BURST_LEN) ? BURST_LEN-1: (wx_size_q-1);
          wx_size_d = wx_size_q >= BURST_LEN ? wx_size_d - BURST_LEN : 0;
        end
      end
      AW_WAIT: begin
        if (m_axi_awvalid && m_axi_awready) begin
          awvalid_d = 1'b0;
          awaddr_offset_d = awaddr_offset_q + BURST_LEN * AXI_DATA_WIDTH / 8;
          if (wx_size_q == 0) begin
            aw_state_d = AW_IDLE;
          end
          else begin
            aw_state_d = AW_SEND;
          end
        end
      end
    endcase
  end

    assign m_axi_awvalid = awvalid_q;
    assign m_axi_awlen = awlen_q;
    assign m_axi_awaddr = {wx_addr_buf[AXI_ADDR_WIDTH-1:C_OFFSET_WIDTH], awaddr_offset_q};

  always @(posedge clk)
  begin
    if (reset) begin
      awlen_q <= 0;
      awvalid_q <= 1'b0;
      aw_state_q <= AR_IDLE;
      awaddr_offset_q  <= 'b0;
      wx_size_q <= 0;
    end else begin
      awlen_q <= awlen_d;
      awvalid_q <= awvalid_d;
      aw_state_q <= aw_state_d;
      awaddr_offset_q  <= awaddr_offset_d;
      wx_size_q <= wx_size_d;
    end
  end

  //
  //The FIFO stores the read requests
  //
  fifo #(
    .DATA_WIDTH                     ( REQ_BUF_DATA_W                 ),
    .ADDR_WIDTH                     ( 4                              )
  ) awr_req_buf (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .s_read_req                     ( wr_req_buf_pop                 ), //input
    .s_read_ready                   ( wr_req_buf_rd_ready            ), //output
    .s_read_data                    ( wr_req_buf_data_out            ), //output
    .s_write_req                    ( wr_req_buf_push                ), //input
    .s_write_ready                  ( wr_req_buf_wr_ready            ), //output
    .s_write_data                   ( wr_req_buf_data_in             ), //input
    .almost_full                    ( wr_req_buf_almost_full         ), //output
    .almost_empty                   ( wr_req_buf_almost_empty        )  //output
  );
//==============================================================================

//==============================================================================
// Write Data (W) Channel
//==============================================================================
    reg  [ 2                    -1 : 0 ]        w_state_d;
    reg  [ 2                    -1 : 0 ]        w_state_q;

    reg  [ AXI_BURST_WIDTH      -1 : 0 ]        wlen_count_d;
    reg  [ AXI_BURST_WIDTH      -1 : 0 ]        wlen_count_q;

    localparam integer  W_IDLE                       = 0;
    localparam integer  W_WAIT                       = 1;
    localparam integer  W_SEND                       = 2;

  always @(*)
  begin
    w_state_d = w_state_q;
    wlen_count_d = wlen_count_q;
    case(w_state_q)
      W_IDLE: begin
        if (wdata_req_buf_rd_ready && mem_read_ready)
          w_state_d = W_SEND;
      end
      W_SEND: begin
        if (m_axi_wready) begin
          if (~m_axi_wlast)
              wlen_count_d = wlen_count_q + mem_read_valid_q;
          else begin
            wlen_count_d = 0;
            w_state_d = W_IDLE;
          end
        end
      end
    endcase
  end

    assign m_axi_wlast = (wlen_count_q == wdata_req_buf_data_out) && mem_read_valid_q;
    assign m_axi_wvalid = mem_read_valid_q;
    assign m_axi_wdata = mem_read_data;
  // assign m_axi_wdata = 'b0;
    assign mem_read_req = mem_read_ready && (w_state_q != W_IDLE) && ~m_axi_wlast && (~mem_read_valid_q || m_axi_wready);

    always @(posedge clk)
    begin
      if (reset)
        mem_read_valid_q <= 1'b0;
      else
        mem_read_valid_q <= mem_read_valid_d;
    end

    always @(*)
    begin
      mem_read_valid_d = mem_read_valid_q;
      case (mem_read_valid_q)
        0: begin
          if (mem_read_req)
            mem_read_valid_d = 1;
        end
        1: begin
          if (m_axi_wready && ~mem_read_req)
            mem_read_valid_d = 0;
        end
      endcase
    end

  always @(posedge clk)
  begin
    if (reset) begin
      wlen_count_q <= 0;
      w_state_q <= W_IDLE;
    end
    else begin
      wlen_count_q <= wlen_count_d;
      w_state_q <= w_state_d;
    end
  end

    assign wdata_req_buf_pop = w_state_q == W_IDLE && mem_read_ready;
    assign wdata_req_buf_push = m_axi_awvalid && m_axi_awready;
    assign wdata_req_buf_data_in = m_axi_awlen;

  //
  //The FIFO stores the read requests
  //
  fifo #(
    .DATA_WIDTH                     ( AXI_BURST_WIDTH                ),
    .ADDR_WIDTH                     ( 4                              )
  ) wdata_req_buf (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .s_read_req                     ( wdata_req_buf_pop              ), //input
    .s_read_ready                   ( wdata_req_buf_rd_ready         ), //output
    .s_read_data                    ( wdata_req_buf_data_out         ), //output
    .s_write_req                    ( wdata_req_buf_push             ), //input
    .s_write_ready                  ( wdata_req_buf_wr_ready         ), //output
    .s_write_data                   ( wdata_req_buf_data_in          ), //input
    .almost_full                    ( wdata_req_buf_almost_full      ), //output
    .almost_empty                   ( wdata_req_buf_almost_empty     )  //output
  );
//==============================================================================



`ifdef COCOTB_SIM

reg [15:0] _rid_mismatch_count;
always @(posedge clk)
begin
  if (reset)
    _rid_mismatch_count <= 0;
  else if (m_axi_rvalid && m_axi_rready)
    _rid_mismatch_count <= (m_axi_rid != mem_write_id) + _rid_mismatch_count;
end

  integer missed_wdata_push;
  always @(posedge clk)
    if (reset)
      missed_wdata_push <=0;
    else
      missed_wdata_push <= missed_wdata_push + (wdata_req_buf_push && ~wdata_req_buf_wr_ready);


  integer missed_wr_req_count;
  always @(posedge clk)
    if (reset)
      missed_wr_req_count <=0;
    else
      missed_wr_req_count <=wr_req && ~wr_req_buf_wr_ready;

  integer wr_req_count;
  always @(posedge clk)
    if (reset)
      wr_req_count <=0;
    else
      wr_req_count <=wr_req_count + (wr_req && wr_ready);
`endif //COCOTB_SIM


`ifdef COCOTB_TOPLEVEL_axi_master
  initial
  begin
    $dumpfile("axi_master.vcd");
    $dumpvars(0,axi_master);
  end
`endif

endmodule
//
// Tag logic for double buffering
//
// Hardik Sharma
// (hsharma@gatech.edu)

`timescale 1ns/1ps
module tag_logic #(
    parameter integer  STORE_ENABLED                = 1
)
(
    input  wire                                         clk,
    input  wire                                         reset,
    input  wire                                         tag_req,
    input  wire                                         tag_reuse,
    input  wire                                         tag_bias_prev_sw,
    input  wire                                         tag_ddr_pe_sw,
    output wire                                         tag_ready,
    output wire                                         tag_done,
    input  wire                                         tag_flush,
    input  wire                                         compute_tag_done,
    output wire                                         next_compute_tag,
//    output wire                                         compute_tag_reuse,
    output wire                                         compute_bias_prev_sw,
    output wire                                         compute_tag_ready,
    input  wire                                         ldmem_tag_done,
    output wire                                         ldmem_tag_ready,
    input  wire                                         stmem_tag_done,
    output wire                                         stmem_ddr_pe_sw,
    output wire                                         stmem_tag_ready
);

//==============================================================================
// Wires/Regs
//==============================================================================
    localparam integer  TAG_FREE                     = 0;
    localparam integer  TAG_LDMEM                    = 1;
    localparam integer  TAG_COMPUTE                  = 2;
    localparam integer  TAG_COMPUTE_CHECK            = 3;
    localparam integer  TAG_STMEM                    = 4;

    localparam integer  TAG_STATE_W                  = 3;

    localparam integer  REUSE_STATE_W                = 1;
    localparam integer  REUSE_FALSE                  = 0;
    localparam integer  REUSE_TRUE                   = 1;

    reg                                         tag_flush_state_d;
    reg                                         tag_flush_state_q;
    reg tag_reuse_state_d;
    reg tag_reuse_state_q;

    reg [2 : 0] tag_reuse_counter;

    reg                                         tag_ddr_pe_sw_q;
    reg                                         compute_ddr_pe_sw;
    reg                                         _stmem_ddr_pe_sw;
    reg                                         tag_bias_prev_sw_q;
    reg                                         reuse_tag_bias_prev_sw_q;
    reg  [ TAG_STATE_W          -1 : 0 ]        tag_state_d;
    reg  [ TAG_STATE_W          -1 : 0 ]        tag_state_q;
//==============================================================================

//==============================================================================
// Tag allocation
//==============================================================================

    assign tag_done = tag_state_q == TAG_FREE;

    assign ldmem_tag_ready = tag_state_q == TAG_LDMEM;
    assign compute_tag_ready = tag_state_q == TAG_COMPUTE;
    assign stmem_tag_ready = tag_state_q == TAG_STMEM;
    assign tag_ready = tag_state_q == TAG_FREE;

    assign compute_bias_prev_sw = tag_bias_prev_sw_q;
    assign stmem_ddr_pe_sw = _stmem_ddr_pe_sw;

  always @(*)
  begin: TAG0_STATE
    tag_state_d = tag_state_q;
    case (tag_state_q)
      TAG_FREE: begin
        if (tag_req) begin
          tag_state_d = TAG_LDMEM;
        end
      end
      TAG_LDMEM: begin
        if (ldmem_tag_done)
          tag_state_d = TAG_COMPUTE;
      end
      TAG_COMPUTE_CHECK: begin
        if (tag_reuse_counter == 0 && tag_flush_state_q == 1) begin
          if (STORE_ENABLED)
            tag_state_d = TAG_STMEM;
          else
            tag_state_d = TAG_FREE;
        end
        else if (tag_reuse_counter != 0)
          tag_state_d = TAG_COMPUTE;
      end
      TAG_COMPUTE: begin
        if (compute_tag_done)
          tag_state_d = TAG_COMPUTE_CHECK;
      end
      TAG_STMEM: begin
        if (stmem_tag_done)
          tag_state_d = TAG_FREE;
      end
    endcase
  end

  always @(posedge clk)
  begin
    if (reset) begin
      tag_state_q <= TAG_FREE;
    end
    else begin
      tag_state_q <= tag_state_d;
    end
  end

  always @(*)
  begin
    tag_flush_state_d = tag_flush_state_q;
    case (tag_flush_state_q)
      0: begin
        if (tag_flush && tag_state_q != TAG_FREE)
          tag_flush_state_d = 1;
      end
      1: begin
        if (tag_state_q == TAG_COMPUTE_CHECK && tag_reuse_counter == 0)
          tag_flush_state_d = 0;
      end
    endcase
  end

  always @(posedge clk)
  begin
    if (reset)
      tag_flush_state_q <= 0;
    else
      tag_flush_state_q <= tag_flush_state_d;
  end

  assign next_compute_tag = tag_state_q == TAG_COMPUTE_CHECK && tag_flush_state_q == 1 && tag_reuse_counter == 0;

  always @(posedge clk)
  begin
    if (reset)
      tag_reuse_counter <= 0;
    else begin
      if (compute_tag_done && ~(tag_req || tag_reuse) && tag_reuse_counter != 0)
        tag_reuse_counter <= tag_reuse_counter - 1'b1;
      else if (~compute_tag_done && (tag_reuse || tag_req))
        tag_reuse_counter <= tag_reuse_counter + 1'b1;
    end
  end

  always @(posedge clk)
  begin
    if (reset) begin
      compute_ddr_pe_sw <= 1'b0;
    end else if (ldmem_tag_done || tag_state_q == TAG_COMPUTE_CHECK) begin
      compute_ddr_pe_sw <= tag_ddr_pe_sw_q;
    end
  end

  always @(posedge clk)
  begin
    if (reset) begin
      _stmem_ddr_pe_sw <= 1'b0;
    end else if (compute_tag_done) begin
      _stmem_ddr_pe_sw <= compute_ddr_pe_sw;
    end
  end

  always @(posedge clk)
  begin
    if (reset) begin
      tag_bias_prev_sw_q <= 1'b0;
    end
    else if (tag_req && tag_ready) begin
      tag_bias_prev_sw_q <= tag_bias_prev_sw;
    end
    else if (compute_tag_done)
      tag_bias_prev_sw_q <= reuse_tag_bias_prev_sw_q;
  end

  always @(posedge clk)
  begin
    if (reset) begin
      tag_ddr_pe_sw_q <= 1'b0;
    end
    else if ((tag_req && tag_ready) || tag_reuse) begin
      tag_ddr_pe_sw_q <= tag_ddr_pe_sw;
    end
  end

  always @(posedge clk)
    if (reset)
      reuse_tag_bias_prev_sw_q <= 1'b0;
    else if (tag_reuse)
      reuse_tag_bias_prev_sw_q <= tag_bias_prev_sw;
//==============================================================================

//==============================================================================
// VCD
//==============================================================================
`ifdef COCOTB_TOPLEVEL_tag_logic
initial begin
  $dumpfile("tag_logic.vcd");
  $dumpvars(0, tag_logic);
end
`endif
//==============================================================================

endmodule
//
// Memory Walker - stride
//
// Hardik Sharma
// (hsharma@gatech.edu)

`timescale 1ns/1ps
module mem_walker_stride #(
  // Internal Parameters
  parameter integer  ADDR_WIDTH                   = 48,
  parameter integer  ADDR_STRIDE_W                = 16,
  parameter integer  LOOP_ID_W                    = 5
) (
  input  wire                                         clk,
  input  wire                                         reset,
  // From loop controller
  input  wire  [ ADDR_WIDTH           -1 : 0 ]        base_addr,
  input  wire                                         loop_ctrl_done,
  input  wire  [ LOOP_ID_W            -1 : 0 ]        loop_index,
  input  wire                                         loop_index_valid,
  input  wire                                         loop_init,
  input  wire                                         loop_enter,
  input  wire                                         loop_exit,
  // Address offset - from instruction decoder
  input  wire                                         cfg_addr_stride_v,
  input  wire  [ ADDR_STRIDE_W        -1 : 0 ]        cfg_addr_stride,
  output wire  [ ADDR_WIDTH           -1 : 0 ]        addr_out,
  output wire                                         addr_out_valid
);

//=============================================================
// Wires/Regs
//=============================================================
  reg  [ LOOP_ID_W            -1 : 0 ]        addr_stride_wr_ptr;
  wire                                        addr_stride_wr_req;
  wire [ ADDR_STRIDE_W        -1 : 0 ]        addr_stride_wr_data;

  wire [ LOOP_ID_W            -1 : 0 ]        addr_stride_rd_ptr;
  wire                                        addr_stride_rd_req;
  wire [ ADDR_STRIDE_W        -1 : 0 ]        addr_stride_rd_data;

  wire [ LOOP_ID_W            -1 : 0 ]        addr_offset_wr_ptr;
  wire                                        addr_offset_wr_req;
  wire [ ADDR_WIDTH           -1 : 0 ]        addr_offset_wr_data;

  wire [ LOOP_ID_W            -1 : 0 ]        addr_offset_rd_ptr;
  wire                                        addr_offset_rd_req;
  wire [ ADDR_WIDTH           -1 : 0 ]        addr_offset_rd_data;

  wire [ ADDR_WIDTH           -1 : 0 ]        prev_addr;

  wire [ ADDR_WIDTH           -1 : 0 ]        offset_updated;

  reg  [ ADDR_WIDTH           -1 : 0 ]        _addr_out;
  wire                                        _addr_out_valid;
  
  reg                                         loop_enter_q;

//=============================================================

//=============================================================
// Address stride buffer
//    This module stores the address strides
//=============================================================
  always @(posedge clk)
  begin:WR_PTR
    if (reset)
      addr_stride_wr_ptr <= 'b0;
    else begin
      if (cfg_addr_stride_v)
        addr_stride_wr_ptr <= addr_stride_wr_ptr + 1'b1;
      else if (loop_ctrl_done)
        addr_stride_wr_ptr <= 'b0;
    end
  end

  assign addr_stride_wr_req = cfg_addr_stride_v;
  assign addr_stride_wr_data = cfg_addr_stride;

  assign addr_stride_rd_ptr = loop_index;
  assign addr_stride_rd_req = loop_index_valid || loop_enter;

  ram #(
    .ADDR_WIDTH                     ( LOOP_ID_W                      ),
    .DATA_WIDTH                     ( ADDR_STRIDE_W                  )
  ) stride_buf (
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .s_write_addr                   ( addr_stride_wr_ptr             ),
    .s_write_req                    ( addr_stride_wr_req             ),
    .s_write_data                   ( addr_stride_wr_data            ),
    .s_read_addr                    ( addr_stride_rd_ptr             ),
    .s_read_req                     ( addr_stride_rd_req             ),
    .s_read_data                    ( addr_stride_rd_data            )
  );

//=============================================================


//=============================================================
// Offset buffer
//    This module stores the current offset
//=============================================================
  assign addr_offset_wr_ptr = cfg_addr_stride_v ? addr_stride_wr_ptr : loop_index;
  assign addr_offset_wr_req = (cfg_addr_stride_v || loop_enter || loop_index_valid);
  assign addr_offset_wr_data = cfg_addr_stride_v ? 'b0 : offset_updated;
  assign prev_addr = loop_init ? base_addr : (loop_enter && loop_enter_q) ? addr_out : addr_offset_rd_data;
  assign offset_updated = prev_addr + addr_stride_rd_data;

  assign addr_offset_rd_ptr = loop_index;
  assign addr_offset_rd_req = loop_index_valid || loop_enter;

  ram #(
    .ADDR_WIDTH                     ( LOOP_ID_W                      ),
    .DATA_WIDTH                     ( ADDR_WIDTH                     )
  ) offset_buf (
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .s_write_addr                   ( addr_offset_wr_ptr             ),
    .s_write_req                    ( addr_offset_wr_req             ),
    .s_write_data                   ( addr_offset_wr_data            ),
    .s_read_addr                    ( addr_offset_rd_ptr             ),
    .s_read_req                     ( addr_offset_rd_req             ),
    .s_read_data                    ( addr_offset_rd_data            )
  );

//=============================================================


//=============================================================
// Output address stride logic
//=============================================================

  assign _addr_out_valid = loop_index_valid;

  always @(posedge clk)
  begin
    if (reset)
      loop_enter_q <= 1'b0;
    else
      loop_enter_q <= loop_enter;
  end

  always @(posedge clk)
  begin
    if (reset)
      _addr_out <= 0;
    else if (loop_init)
      _addr_out <= base_addr;
    else if (loop_enter && !loop_enter_q)
      _addr_out <= addr_offset_rd_data;
    else if (loop_index_valid)
      _addr_out <= _addr_out + addr_stride_rd_data;
  end

  assign addr_out_valid = _addr_out_valid;
  assign addr_out = _addr_out;
//=============================================================



//=============================================================
// VCD
//=============================================================
`ifdef COCOTB_TOPLEVEL_mem_walker_stride
initial begin
  $dumpfile("mem_walker_stride.vcd");
  $dumpvars(0, mem_walker_stride);
end
`endif
//=============================================================

endmodule
//
// Tag logic for double buffering
//
// Hardik Sharma
// (hsharma@gatech.edu)

`timescale 1ns/1ps
module tag_sync #(
    parameter integer  NUM_TAGS                     = 2,
    parameter integer  TAG_W                        = $clog2(NUM_TAGS),
    parameter integer  STORE_ENABLED                = 1
)
(
    input  wire                                         clk,
    input  wire                                         reset,
    input  wire                                         block_done,
    input  wire                                         tag_req,
    input  wire                                         tag_reuse,
    input  wire                                         tag_bias_prev_sw,
    input  wire                                         tag_ddr_pe_sw,
    output wire                                         tag_ready,
    output wire  [ TAG_W                -1 : 0 ]        tag,
    output wire                                         tag_done,
    input  wire                                         compute_tag_done,
    output wire                                         compute_tag_ready,
    output wire                                         compute_bias_prev_sw,
    output wire  [ TAG_W                -1 : 0 ]        compute_tag,
    input  wire                                         ldmem_tag_done,
    output wire                                         ldmem_tag_ready,
    output wire  [ TAG_W                -1 : 0 ]        ldmem_tag,
    input  wire  [ TAG_W                -1 : 0 ]        raw_stmem_tag,
    output wire                                         raw_stmem_tag_ready,
    output wire                                         stmem_ddr_pe_sw,
    input  wire                                         stmem_tag_done,
    output wire                                         stmem_tag_ready,
    output wire  [ TAG_W                -1 : 0 ]        stmem_tag
);

//==============================================================================
// Wires/Regs
//==============================================================================
    reg  [ TAG_W                -1 : 0 ]        prev_tag;
    reg  [ TAG_W                -1 : 0 ]        tag_alloc;
    reg  [ TAG_W                -1 : 0 ]        ldmem_tag_alloc;
    reg  [ TAG_W                -1 : 0 ]        compute_tag_alloc;
    reg  [ TAG_W                -1 : 0 ]        stmem_tag_alloc;
    reg  [ 2                    -1 : 0 ]        tag0_state_d;
    reg  [ 2                    -1 : 0 ]        tag0_state_q;
    reg  [ 2                    -1 : 0 ]        tag1_state_d;
    reg  [ 2                    -1 : 0 ]        tag1_state_q;

    wire next_compute_tag;

    wire [ NUM_TAGS             -1 : 0 ]        local_next_compute_tag;
    wire [ NUM_TAGS             -1 : 0 ]        local_tag_ready;
    wire [ NUM_TAGS             -1 : 0 ]        local_compute_tag_ready;
//    wire [ NUM_TAGS             -1 : 0 ]        local_compute_tag_reuse;
    wire [ NUM_TAGS             -1 : 0 ]        local_bias_prev_sw;
    wire [ NUM_TAGS             -1 : 0 ]        local_stmem_ddr_pe_sw;
    wire [ NUM_TAGS             -1 : 0 ]        local_ldmem_tag_ready;
    wire [ NUM_TAGS             -1 : 0 ]        local_stmem_tag_ready;

    localparam integer  TAG_FREE                     = 0;
    localparam integer  TAG_LDMEM                    = 1;
    localparam integer  TAG_COMPUTE                  = 2;
    localparam integer  TAG_STMEM                    = 3;

//    wire                                        compute_tag_reuse;

    wire                                        cache_hit;
    wire                                        cache_flush;
//==============================================================================

//==============================================================================
// Tag allocation
//==============================================================================

    assign cache_hit = tag_reuse;
    assign cache_flush = (tag_req && ~tag_reuse) || block_done;

  always @(posedge clk)
  begin
    if (reset)
      tag_alloc <= 'b0;
    else if (tag_req && tag_ready && ~cache_hit) begin
      if (tag_alloc == NUM_TAGS-1)
        tag_alloc <= 'b0;
      else
        tag_alloc <= tag_alloc + 1'b1;
    end
  end
  always @(posedge clk)
  begin
    if (reset)
      prev_tag <= 'b0;
    else if (tag_req && tag_ready && ~cache_hit) begin
      prev_tag <= tag_alloc;
    end
  end

  always @(posedge clk)
  begin
    if (reset)
      ldmem_tag_alloc <= 'b0;
    else if (ldmem_tag_done)
      if (ldmem_tag_alloc == NUM_TAGS-1)
        ldmem_tag_alloc <= 'b0;
      else
        ldmem_tag_alloc <= ldmem_tag_alloc + 1'b1;
  end

  always @(posedge clk)
  begin
    if (reset)
      compute_tag_alloc <= 'b0;
    else if (next_compute_tag)
      if (compute_tag_alloc == NUM_TAGS-1)
        compute_tag_alloc <= 'b0;
      else
        compute_tag_alloc <= compute_tag_alloc + 1'b1;
  end

  always @(posedge clk)
  begin
    if (reset)
      stmem_tag_alloc <= 'b0;
    else if (stmem_tag_done)
      if (stmem_tag_alloc == NUM_TAGS-1)
        stmem_tag_alloc <= 'b0;
      else
        stmem_tag_alloc <= stmem_tag_alloc + 1'b1;
  end

    assign tag_done = &local_tag_ready;

    // Buffer hit/miss logic
    assign tag = tag_reuse ? prev_tag: tag_alloc;
    assign tag_ready = local_tag_ready[prev_tag] || local_tag_ready[tag_alloc];

    assign next_compute_tag = local_next_compute_tag[compute_tag_alloc];

    assign ldmem_tag = ldmem_tag_alloc;
    assign compute_tag = compute_tag_alloc;
    assign stmem_tag = stmem_tag_alloc;

    assign ldmem_tag_ready = local_ldmem_tag_ready[ldmem_tag];
    assign compute_tag_ready = local_compute_tag_ready[compute_tag];
    assign compute_bias_prev_sw = local_bias_prev_sw[compute_tag];
    assign stmem_ddr_pe_sw = local_stmem_ddr_pe_sw[stmem_tag];
    assign stmem_tag_ready = local_stmem_tag_ready[stmem_tag];

    assign raw_stmem_tag_ready = local_stmem_tag_ready[raw_stmem_tag];

//    assign compute_tag_reuse = local_compute_tag_reuse[compute_tag];

  genvar t;
  generate
    for (t=0; t<NUM_TAGS; t=t+1)
    begin: TAG_GEN

    wire                                        _tag_flush;


    wire                                        _next_compute_tag;

    wire                                        _tag_req;
    wire                                        _tag_reuse;
    wire                                        _tag_bias_prev_sw;
    wire                                        _tag_ddr_pe_sw;
    wire                                        _tag_ready;
    wire                                        _tag_done;
    wire                                        _compute_tag_done;
    wire                                        _compute_bias_prev_sw;
    wire                                        _compute_tag_ready;
//    wire                                        _compute_tag_reuse;
    wire                                        _ldmem_tag_done;
    wire                                        _ldmem_tag_ready;
    wire                                        _stmem_tag_done;
    wire                                        _stmem_tag_ready;
    wire                                        _stmem_ddr_pe_sw;

    assign _tag_reuse = tag_reuse && compute_tag_alloc == t;

    assign _tag_req = tag_req && ~tag_reuse && tag_ready && tag == t;
    assign _tag_bias_prev_sw = tag_bias_prev_sw;
    assign _tag_ddr_pe_sw = tag_ddr_pe_sw;
      // assign _tag_done = tag_done && tag == t;
    assign _ldmem_tag_done = ldmem_tag_done && ldmem_tag == t;
    assign _compute_tag_done = compute_tag_done && compute_tag == t;
    assign _stmem_tag_done = stmem_tag_done && stmem_tag == t;

    assign local_tag_ready[t] = _tag_ready;
    assign local_ldmem_tag_ready[t] = _ldmem_tag_ready;
    assign local_compute_tag_ready[t] = _compute_tag_ready;
//    assign local_compute_tag_reuse[t] = _compute_tag_reuse;
    assign local_bias_prev_sw[t] = _compute_bias_prev_sw;
    assign local_stmem_tag_ready[t] = _stmem_tag_ready;
    assign local_stmem_ddr_pe_sw[t] = _stmem_ddr_pe_sw;

    assign local_next_compute_tag[t] = _next_compute_tag;

    assign _tag_flush = cache_flush && prev_tag == t;

      tag_logic local_tag (
    .clk                            ( clk                            ), // input
    .reset                          ( reset                          ), // input
    .next_compute_tag               ( _next_compute_tag              ), // output
    .tag_req                        ( _tag_req                       ), // input
    .tag_reuse                      ( _tag_reuse                     ), // input
    .tag_bias_prev_sw               ( _tag_bias_prev_sw              ), // input
    .tag_ddr_pe_sw                  ( _tag_ddr_pe_sw                 ), // input
    .tag_ready                      ( _tag_ready                     ), // output
    .tag_done                       ( _tag_done                      ), // input
    .tag_flush                      ( _tag_flush                     ), // input
    .compute_tag_done               ( _compute_tag_done              ), // input
//    .compute_tag_reuse              ( _compute_tag_reuse             ), // input
    .compute_bias_prev_sw           ( _compute_bias_prev_sw          ), // output
    .compute_tag_ready              ( _compute_tag_ready             ), // output
    .ldmem_tag_done                 ( _ldmem_tag_done                ), // input
    .ldmem_tag_ready                ( _ldmem_tag_ready               ), // output
    .stmem_ddr_pe_sw                ( _stmem_ddr_pe_sw               ), // output
    .stmem_tag_done                 ( _stmem_tag_done                ), // input
    .stmem_tag_ready                ( _stmem_tag_ready               ) // output
        );

    end
  endgenerate
//==============================================================================

//=============================================================
// VCD
//=============================================================
`ifdef COCOTB_TOPLEVEL_tag_logic
initial begin
  $dumpfile("tag_logic.vcd");
  $dumpvars(0, tag_logic);
end
`endif
//=============================================================
endmodule
//
// DnnWeaver v2 controller - Custom Logic (CL) Wrapper
//
// Hardik Sharma
// (hsharma@gatech.edu)

`timescale 1ns/1ps
module cl_wrapper #(
    parameter integer  INST_W                       = 32,
    parameter integer  INST_ADDR_W                  = 5,
    parameter integer  IFIFO_ADDR_W                 = 10,
    parameter integer  BUF_TYPE_W                   = 2,
    parameter integer  OP_CODE_W                    = 5,
    parameter integer  OP_SPEC_W                    = 6,
    parameter integer  LOOP_ID_W                    = 5,

  // Systolic Array
    parameter integer  ARRAY_N                      = 32,
    parameter integer  ARRAY_M                      = 32,

  // Precision
    parameter integer  DATA_WIDTH                   = 8,
    parameter integer  BIAS_WIDTH                   = 16,
    parameter integer  ACC_WIDTH                    = 32,

  // Buffers
    parameter integer  NUM_TAGS                     = 2,
    parameter integer  IBUF_CAPACITY_BITS           = ARRAY_N * DATA_WIDTH * 2048 / NUM_TAGS,
    parameter integer  WBUF_CAPACITY_BITS           = ARRAY_N * ARRAY_M * DATA_WIDTH * 256 / NUM_TAGS,
    parameter integer  OBUF_CAPACITY_BITS           = ARRAY_M * ACC_WIDTH * 2048 / NUM_TAGS,
    parameter integer  BBUF_CAPACITY_BITS           = ARRAY_M * BIAS_WIDTH * 2048 / NUM_TAGS,

  // Buffer Addr Width
    parameter integer  IBUF_ADDR_WIDTH              = $clog2(IBUF_CAPACITY_BITS / ARRAY_N / DATA_WIDTH),
    parameter integer  WBUF_ADDR_WIDTH              = $clog2(WBUF_CAPACITY_BITS / ARRAY_N / ARRAY_M / DATA_WIDTH),
    parameter integer  OBUF_ADDR_WIDTH              = $clog2(OBUF_CAPACITY_BITS / ARRAY_M / ACC_WIDTH),
    parameter integer  BBUF_ADDR_WIDTH              = $clog2(BBUF_CAPACITY_BITS / ARRAY_M / BIAS_WIDTH),

  // AXI DATA
    parameter integer  AXI_ADDR_WIDTH               = 42,
    parameter integer  AXI_BURST_WIDTH              = 8,
    parameter integer  IBUF_AXI_DATA_WIDTH          = 256,
    parameter integer  IBUF_WSTRB_W                 = IBUF_AXI_DATA_WIDTH/8,
    parameter integer  OBUF_AXI_DATA_WIDTH          = 256,
    parameter integer  OBUF_WSTRB_W                 = OBUF_AXI_DATA_WIDTH/8,
    parameter integer  PU_AXI_DATA_WIDTH            = 256,
    parameter integer  PU_WSTRB_W                   = PU_AXI_DATA_WIDTH/8,
    parameter integer  WBUF_AXI_DATA_WIDTH          = 256,
    parameter integer  WBUF_WSTRB_W                 = WBUF_AXI_DATA_WIDTH/8,
    parameter integer  BBUF_AXI_DATA_WIDTH          = 256,
    parameter integer  BBUF_WSTRB_W                 = BBUF_AXI_DATA_WIDTH/8,
    parameter integer  AXI_ID_WIDTH                 = 1,
  // AXI Instructions
    parameter integer  INST_ADDR_WIDTH              = 16,
    parameter integer  INST_DATA_WIDTH              = 32,
    parameter integer  INST_WSTRB_WIDTH             = INST_DATA_WIDTH/8,
    parameter integer  INST_BURST_WIDTH             = 8,
  // AXI-Lite
    parameter integer  CTRL_ADDR_WIDTH              = 16,
    parameter integer  CTRL_DATA_WIDTH              = 32,
    parameter integer  CTRL_WSTRB_WIDTH             = CTRL_DATA_WIDTH/8
) (
    input  wire                                         clk,
    input  wire                                         reset,

  // PCIe -> CL_wrapper AXI4-Lite interface
    // Slave Write address
    input  wire                                         pci_cl_ctrl_awvalid,
    input  wire  [ CTRL_ADDR_WIDTH      -1 : 0 ]        pci_cl_ctrl_awaddr,
    output wire                                         pci_cl_ctrl_awready,
    // Slave Write data
    input  wire                                         pci_cl_ctrl_wvalid,
    input  wire  [ CTRL_DATA_WIDTH      -1 : 0 ]        pci_cl_ctrl_wdata,
    input  wire  [ CTRL_WSTRB_WIDTH     -1 : 0 ]        pci_cl_ctrl_wstrb,
    output wire                                         pci_cl_ctrl_wready,
    //Write response
    output wire                                         pci_cl_ctrl_bvalid,
    output wire  [ 2                    -1 : 0 ]        pci_cl_ctrl_bresp,
    input  wire                                         pci_cl_ctrl_bready,
    //Read address
    input  wire                                         pci_cl_ctrl_arvalid,
    input  wire  [ CTRL_ADDR_WIDTH      -1 : 0 ]        pci_cl_ctrl_araddr,
    output wire                                         pci_cl_ctrl_arready,
    //Read data/response
    output wire                                         pci_cl_ctrl_rvalid,
    output wire  [ CTRL_DATA_WIDTH      -1 : 0 ]        pci_cl_ctrl_rdata,
    output wire  [ 2                    -1 : 0 ]        pci_cl_ctrl_rresp,
    input  wire                                         pci_cl_ctrl_rready,

  // PCIe -> CL_wrapper AXI4 interface
    // Slave Interface Write Address
    input  wire  [ INST_ADDR_WIDTH      -1 : 0 ]        pci_cl_data_awaddr,
    input  wire  [ INST_BURST_WIDTH     -1 : 0 ]        pci_cl_data_awlen,
    input  wire  [ 3                    -1 : 0 ]        pci_cl_data_awsize,
    input  wire  [ 2                    -1 : 0 ]        pci_cl_data_awburst,
    input  wire                                         pci_cl_data_awvalid,
    output wire                                         pci_cl_data_awready,
  // Slave Interface Write Data
    input  wire  [ INST_DATA_WIDTH      -1 : 0 ]        pci_cl_data_wdata,
    input  wire  [ INST_WSTRB_WIDTH     -1 : 0 ]        pci_cl_data_wstrb,
    input  wire                                         pci_cl_data_wlast,
    input  wire                                         pci_cl_data_wvalid,
    output wire                                         pci_cl_data_wready,
  // Slave Interface Write Response
    output wire  [ 2                    -1 : 0 ]        pci_cl_data_bresp,
    output wire                                         pci_cl_data_bvalid,
    input  wire                                         pci_cl_data_bready,
  // Slave Interface Read Address
    input  wire  [ INST_ADDR_WIDTH      -1 : 0 ]        pci_cl_data_araddr,
    input  wire  [ INST_BURST_WIDTH     -1 : 0 ]        pci_cl_data_arlen,
    input  wire  [ 3                    -1 : 0 ]        pci_cl_data_arsize,
    input  wire  [ 2                    -1 : 0 ]        pci_cl_data_arburst,
    input  wire                                         pci_cl_data_arvalid,
    output wire                                         pci_cl_data_arready,
  // Slave Interface Read Data
    output wire  [ INST_DATA_WIDTH      -1 : 0 ]        pci_cl_data_rdata,
    output wire  [ 2                    -1 : 0 ]        pci_cl_data_rresp,
    output wire                                         pci_cl_data_rlast,
    output wire                                         pci_cl_data_rvalid,
    input  wire                                         pci_cl_data_rready,

  // CL_wrapper -> DDR0 AXI4 interface
    // Master Interface Write Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        cl_ddr0_awaddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        cl_ddr0_awlen,
    output wire  [ 3                    -1 : 0 ]        cl_ddr0_awsize,
    output wire  [ 2                    -1 : 0 ]        cl_ddr0_awburst,
    output wire                                         cl_ddr0_awvalid,
    input  wire                                         cl_ddr0_awready,
    // Master Interface Write Data
    output wire  [ IBUF_AXI_DATA_WIDTH  -1 : 0 ]        cl_ddr0_wdata,
    output wire  [ IBUF_WSTRB_W         -1 : 0 ]        cl_ddr0_wstrb,
    output wire                                         cl_ddr0_wlast,
    output wire                                         cl_ddr0_wvalid,
    input  wire                                         cl_ddr0_wready,
    // Master Interface Write Response
    input  wire  [ 2                    -1 : 0 ]        cl_ddr0_bresp,
    input  wire                                         cl_ddr0_bvalid,
    output wire                                         cl_ddr0_bready,
    // Master Interface Read Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        cl_ddr0_araddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        cl_ddr0_arlen,
    output wire  [ 3                    -1 : 0 ]        cl_ddr0_arsize,
    output wire  [ 2                    -1 : 0 ]        cl_ddr0_arburst,
    output wire                                         cl_ddr0_arvalid,
    output wire  [ AXI_ID_WIDTH         -1 : 0 ]        cl_ddr0_arid,
    input  wire                                         cl_ddr0_arready,
    // Master Interface Read Data
    input  wire  [ IBUF_AXI_DATA_WIDTH  -1 : 0 ]        cl_ddr0_rdata,
    input  wire  [ AXI_ID_WIDTH         -1 : 0 ]        cl_ddr0_rid,
    input  wire  [ 2                    -1 : 0 ]        cl_ddr0_rresp,
    input  wire                                         cl_ddr0_rlast,
    input  wire                                         cl_ddr0_rvalid,
    output wire                                         cl_ddr0_rready,

  // CL_wrapper -> DDR1 AXI4 interface
    // Master Interface Write Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        cl_ddr1_awaddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        cl_ddr1_awlen,
    output wire  [ 3                    -1 : 0 ]        cl_ddr1_awsize,
    output wire  [ 2                    -1 : 0 ]        cl_ddr1_awburst,
    output wire                                         cl_ddr1_awvalid,
    input  wire                                         cl_ddr1_awready,
    // Master Interface Write Data
    output wire  [ OBUF_AXI_DATA_WIDTH  -1 : 0 ]        cl_ddr1_wdata,
    output wire  [ OBUF_WSTRB_W         -1 : 0 ]        cl_ddr1_wstrb,
    output wire                                         cl_ddr1_wlast,
    output wire                                         cl_ddr1_wvalid,
    input  wire                                         cl_ddr1_wready,
    // Master Interface Write Response
    input  wire  [ 2                    -1 : 0 ]        cl_ddr1_bresp,
    input  wire                                         cl_ddr1_bvalid,
    output wire                                         cl_ddr1_bready,
    // Master Interface Read Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        cl_ddr1_araddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        cl_ddr1_arlen,
    output wire  [ 3                    -1 : 0 ]        cl_ddr1_arsize,
    output wire  [ 2                    -1 : 0 ]        cl_ddr1_arburst,
    output wire                                         cl_ddr1_arvalid,
    output wire  [ AXI_ID_WIDTH         -1 : 0 ]        cl_ddr1_arid,
    input  wire                                         cl_ddr1_arready,
    // Master Interface Read Data
    input  wire  [ OBUF_AXI_DATA_WIDTH  -1 : 0 ]        cl_ddr1_rdata,
    input  wire  [ AXI_ID_WIDTH         -1 : 0 ]        cl_ddr1_rid,
    input  wire  [ 2                    -1 : 0 ]        cl_ddr1_rresp,
    input  wire                                         cl_ddr1_rlast,
    input  wire                                         cl_ddr1_rvalid,
    output wire                                         cl_ddr1_rready,

  // CL_wrapper -> DDR2 AXI4 interface
    // Master Interface Write Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        cl_ddr2_awaddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        cl_ddr2_awlen,
    output wire  [ 3                    -1 : 0 ]        cl_ddr2_awsize,
    output wire  [ 2                    -1 : 0 ]        cl_ddr2_awburst,
    output wire                                         cl_ddr2_awvalid,
    input  wire                                         cl_ddr2_awready,
    // Master Interface Write Data
    output wire  [ WBUF_AXI_DATA_WIDTH  -1 : 0 ]        cl_ddr2_wdata,
    output wire  [ WBUF_WSTRB_W         -1 : 0 ]        cl_ddr2_wstrb,
    output wire                                         cl_ddr2_wlast,
    output wire                                         cl_ddr2_wvalid,
    input  wire                                         cl_ddr2_wready,
    // Master Interface Write Response
    input  wire  [ 2                    -1 : 0 ]        cl_ddr2_bresp,
    input  wire                                         cl_ddr2_bvalid,
    output wire                                         cl_ddr2_bready,
    // Master Interface Read Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        cl_ddr2_araddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        cl_ddr2_arlen,
    output wire  [ 3                    -1 : 0 ]        cl_ddr2_arsize,
    output wire  [ 2                    -1 : 0 ]        cl_ddr2_arburst,
    output wire                                         cl_ddr2_arvalid,
    output wire  [ AXI_ID_WIDTH         -1 : 0 ]        cl_ddr2_arid,
    input  wire                                         cl_ddr2_arready,
    // Master Interface Read Data
    input  wire  [ WBUF_AXI_DATA_WIDTH  -1 : 0 ]        cl_ddr2_rdata,
    input  wire  [ AXI_ID_WIDTH         -1 : 0 ]        cl_ddr2_rid,
    input  wire  [ 2                    -1 : 0 ]        cl_ddr2_rresp,
    input  wire                                         cl_ddr2_rlast,
    input  wire                                         cl_ddr2_rvalid,
    output wire                                         cl_ddr2_rready,

  // CL_wrapper -> DDR3 AXI4 interface
    // Master Interface Write Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        cl_ddr3_awaddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        cl_ddr3_awlen,
    output wire  [ 3                    -1 : 0 ]        cl_ddr3_awsize,
    output wire  [ 2                    -1 : 0 ]        cl_ddr3_awburst,
    output wire                                         cl_ddr3_awvalid,
    input  wire                                         cl_ddr3_awready,
    // Master Interface Write Data
    output wire  [ BBUF_AXI_DATA_WIDTH  -1 : 0 ]        cl_ddr3_wdata,
    output wire  [ BBUF_WSTRB_W         -1 : 0 ]        cl_ddr3_wstrb,
    output wire                                         cl_ddr3_wlast,
    output wire                                         cl_ddr3_wvalid,
    input  wire                                         cl_ddr3_wready,
    // Master Interface Write Response
    input  wire  [ 2                    -1 : 0 ]        cl_ddr3_bresp,
    input  wire                                         cl_ddr3_bvalid,
    output wire                                         cl_ddr3_bready,
    // Master Interface Read Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        cl_ddr3_araddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        cl_ddr3_arlen,
    output wire  [ 3                    -1 : 0 ]        cl_ddr3_arsize,
    output wire  [ 2                    -1 : 0 ]        cl_ddr3_arburst,
    output wire                                         cl_ddr3_arvalid,
    output wire  [ AXI_ID_WIDTH         -1 : 0 ]        cl_ddr3_arid,
    input  wire                                         cl_ddr3_arready,
    // Master Interface Read Data
    input  wire  [ BBUF_AXI_DATA_WIDTH  -1 : 0 ]        cl_ddr3_rdata,
    input  wire  [ AXI_ID_WIDTH         -1 : 0 ]        cl_ddr3_rid,
    input  wire  [ 2                    -1 : 0 ]        cl_ddr3_rresp,
    input  wire                                         cl_ddr3_rlast,
    input  wire                                         cl_ddr3_rvalid,
    output wire                                         cl_ddr3_rready,


  // CL_wrapper -> DDR3 AXI4 interface
    // Master Interface Write Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        cl_ddr4_awaddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        cl_ddr4_awlen,
    output wire  [ 3                    -1 : 0 ]        cl_ddr4_awsize,
    output wire  [ 2                    -1 : 0 ]        cl_ddr4_awburst,
    output wire                                         cl_ddr4_awvalid,
    input  wire                                         cl_ddr4_awready,
    // Master Interface Write Data
    output wire  [ PU_AXI_DATA_WIDTH    -1 : 0 ]        cl_ddr4_wdata,
    output wire  [ PU_WSTRB_W           -1 : 0 ]        cl_ddr4_wstrb,
    output wire                                         cl_ddr4_wlast,
    output wire                                         cl_ddr4_wvalid,
    input  wire                                         cl_ddr4_wready,
    // Master Interface Write Response
    input  wire  [ 2                    -1 : 0 ]        cl_ddr4_bresp,
    input  wire                                         cl_ddr4_bvalid,
    output wire                                         cl_ddr4_bready,
    // Master Interface Read Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        cl_ddr4_araddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        cl_ddr4_arlen,
    output wire  [ 3                    -1 : 0 ]        cl_ddr4_arsize,
    output wire  [ 2                    -1 : 0 ]        cl_ddr4_arburst,
    output wire                                         cl_ddr4_arvalid,
    output wire  [ AXI_ID_WIDTH         -1 : 0 ]        cl_ddr4_arid,
    input  wire                                         cl_ddr4_arready,
    // Master Interface Read Data
    input  wire  [ PU_AXI_DATA_WIDTH    -1 : 0 ]        cl_ddr4_rdata,
    input  wire  [ AXI_ID_WIDTH         -1 : 0 ]        cl_ddr4_rid,
    input  wire  [ 2                    -1 : 0 ]        cl_ddr4_rresp,
    input  wire                                         cl_ddr4_rlast,
    input  wire                                         cl_ddr4_rvalid,
    output wire                                         cl_ddr4_rready

);

//=============================================================
// Wires/Regs
//=============================================================
//=============================================================

//=============================================================
// Comb Logic
//=============================================================
//=============================================================

//=============================================================
// DnnWeaver2 Wrapper
//=============================================================
  dnnweaver2_controller #(
    .ARRAY_N                        ( ARRAY_N                        ),
    .ARRAY_M                        ( ARRAY_M                        ),
    .DATA_WIDTH                     ( DATA_WIDTH                     ),
    .BIAS_WIDTH                     ( BIAS_WIDTH                     ),
    .ACC_WIDTH                      ( ACC_WIDTH                      ),

    .IBUF_CAPACITY_BITS             ( IBUF_CAPACITY_BITS             ),
    .WBUF_CAPACITY_BITS             ( WBUF_CAPACITY_BITS             ),
    .OBUF_CAPACITY_BITS             ( OBUF_CAPACITY_BITS             ),
    .BBUF_CAPACITY_BITS             ( BBUF_CAPACITY_BITS             ),

    .AXI_ADDR_WIDTH                 ( AXI_ADDR_WIDTH                 ),
    .AXI_BURST_WIDTH                ( AXI_BURST_WIDTH                ),
    .IBUF_AXI_DATA_WIDTH            ( IBUF_AXI_DATA_WIDTH            ),
    .OBUF_AXI_DATA_WIDTH            ( OBUF_AXI_DATA_WIDTH            ),
    .PU_AXI_DATA_WIDTH              ( PU_AXI_DATA_WIDTH              ),
    .WBUF_AXI_DATA_WIDTH            ( WBUF_AXI_DATA_WIDTH            ),
    .BBUF_AXI_DATA_WIDTH            ( BBUF_AXI_DATA_WIDTH            ),
    .INST_ADDR_WIDTH                ( INST_ADDR_WIDTH                ),
    .INST_DATA_WIDTH                ( INST_DATA_WIDTH                ),
    .CTRL_ADDR_WIDTH                ( CTRL_ADDR_WIDTH                ),
    .CTRL_DATA_WIDTH                ( CTRL_DATA_WIDTH                )
  ) u_bf_wrap (
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),

    .pci_cl_data_awaddr             ( pci_cl_data_awaddr             ),
    .pci_cl_data_awlen              ( pci_cl_data_awlen              ),
    .pci_cl_data_awsize             ( pci_cl_data_awsize             ),
    .pci_cl_data_awburst            ( pci_cl_data_awburst            ),
    .pci_cl_data_awvalid            ( pci_cl_data_awvalid            ),
    .pci_cl_data_awready            ( pci_cl_data_awready            ),
    .pci_cl_data_wdata              ( pci_cl_data_wdata              ),
    .pci_cl_data_wstrb              ( pci_cl_data_wstrb              ),
    .pci_cl_data_wlast              ( pci_cl_data_wlast              ),
    .pci_cl_data_wvalid             ( pci_cl_data_wvalid             ),
    .pci_cl_data_wready             ( pci_cl_data_wready             ),
    .pci_cl_data_bresp              ( pci_cl_data_bresp              ),
    .pci_cl_data_bvalid             ( pci_cl_data_bvalid             ),
    .pci_cl_data_bready             ( pci_cl_data_bready             ),
    .pci_cl_data_araddr             ( pci_cl_data_araddr             ),
    .pci_cl_data_arlen              ( pci_cl_data_arlen              ),
    .pci_cl_data_arsize             ( pci_cl_data_arsize             ),
    .pci_cl_data_arburst            ( pci_cl_data_arburst            ),
    .pci_cl_data_arvalid            ( pci_cl_data_arvalid            ),
    .pci_cl_data_arready            ( pci_cl_data_arready            ),
    .pci_cl_data_rdata              ( pci_cl_data_rdata              ),
    .pci_cl_data_rresp              ( pci_cl_data_rresp              ),
    .pci_cl_data_rlast              ( pci_cl_data_rlast              ),
    .pci_cl_data_rvalid             ( pci_cl_data_rvalid             ),
    .pci_cl_data_rready             ( pci_cl_data_rready             ),

    .pci_cl_ctrl_awvalid            ( pci_cl_ctrl_awvalid            ),
    .pci_cl_ctrl_awaddr             ( pci_cl_ctrl_awaddr             ),
    .pci_cl_ctrl_awready            ( pci_cl_ctrl_awready            ),
    .pci_cl_ctrl_wvalid             ( pci_cl_ctrl_wvalid             ),
    .pci_cl_ctrl_wdata              ( pci_cl_ctrl_wdata              ),
    .pci_cl_ctrl_wstrb              ( pci_cl_ctrl_wstrb              ),
    .pci_cl_ctrl_wready             ( pci_cl_ctrl_wready             ),
    .pci_cl_ctrl_bvalid             ( pci_cl_ctrl_bvalid             ),
    .pci_cl_ctrl_bresp              ( pci_cl_ctrl_bresp              ),
    .pci_cl_ctrl_bready             ( pci_cl_ctrl_bready             ),
    .pci_cl_ctrl_arvalid            ( pci_cl_ctrl_arvalid            ),
    .pci_cl_ctrl_araddr             ( pci_cl_ctrl_araddr             ),
    .pci_cl_ctrl_arready            ( pci_cl_ctrl_arready            ),
    .pci_cl_ctrl_rvalid             ( pci_cl_ctrl_rvalid             ),
    .pci_cl_ctrl_rdata              ( pci_cl_ctrl_rdata              ),
    .pci_cl_ctrl_rresp              ( pci_cl_ctrl_rresp              ),
    .pci_cl_ctrl_rready             ( pci_cl_ctrl_rready             ),

    .cl_ddr0_awaddr                 ( cl_ddr0_awaddr                 ),
    .cl_ddr0_awlen                  ( cl_ddr0_awlen                  ),
    .cl_ddr0_awsize                 ( cl_ddr0_awsize                 ),
    .cl_ddr0_awburst                ( cl_ddr0_awburst                ),
    .cl_ddr0_awvalid                ( cl_ddr0_awvalid                ),
    .cl_ddr0_awready                ( cl_ddr0_awready                ),
    .cl_ddr0_wdata                  ( cl_ddr0_wdata                  ),
    .cl_ddr0_wstrb                  ( cl_ddr0_wstrb                  ),
    .cl_ddr0_wlast                  ( cl_ddr0_wlast                  ),
    .cl_ddr0_wvalid                 ( cl_ddr0_wvalid                 ),
    .cl_ddr0_wready                 ( cl_ddr0_wready                 ),
    .cl_ddr0_bresp                  ( cl_ddr0_bresp                  ),
    .cl_ddr0_bvalid                 ( cl_ddr0_bvalid                 ),
    .cl_ddr0_bready                 ( cl_ddr0_bready                 ),
    .cl_ddr0_araddr                 ( cl_ddr0_araddr                 ),
    .cl_ddr0_arlen                  ( cl_ddr0_arlen                  ),
    .cl_ddr0_arsize                 ( cl_ddr0_arsize                 ),
    .cl_ddr0_arburst                ( cl_ddr0_arburst                ),
    .cl_ddr0_arvalid                ( cl_ddr0_arvalid                ),
    .cl_ddr0_arid                   ( cl_ddr0_arid                   ),
    .cl_ddr0_arready                ( cl_ddr0_arready                ),
    .cl_ddr0_rdata                  ( cl_ddr0_rdata                  ),
    .cl_ddr0_rid                    ( cl_ddr0_rid                    ),
    .cl_ddr0_rresp                  ( cl_ddr0_rresp                  ),
    .cl_ddr0_rlast                  ( cl_ddr0_rlast                  ),
    .cl_ddr0_rvalid                 ( cl_ddr0_rvalid                 ),
    .cl_ddr0_rready                 ( cl_ddr0_rready                 ),

    .cl_ddr1_awaddr                 ( cl_ddr1_awaddr                 ),
    .cl_ddr1_awlen                  ( cl_ddr1_awlen                  ),
    .cl_ddr1_awsize                 ( cl_ddr1_awsize                 ),
    .cl_ddr1_awburst                ( cl_ddr1_awburst                ),
    .cl_ddr1_awvalid                ( cl_ddr1_awvalid                ),
    .cl_ddr1_awready                ( cl_ddr1_awready                ),
    .cl_ddr1_wdata                  ( cl_ddr1_wdata                  ),
    .cl_ddr1_wstrb                  ( cl_ddr1_wstrb                  ),
    .cl_ddr1_wlast                  ( cl_ddr1_wlast                  ),
    .cl_ddr1_wvalid                 ( cl_ddr1_wvalid                 ),
    .cl_ddr1_wready                 ( cl_ddr1_wready                 ),
    .cl_ddr1_bresp                  ( cl_ddr1_bresp                  ),
    .cl_ddr1_bvalid                 ( cl_ddr1_bvalid                 ),
    .cl_ddr1_bready                 ( cl_ddr1_bready                 ),
    .cl_ddr1_araddr                 ( cl_ddr1_araddr                 ),
    .cl_ddr1_arlen                  ( cl_ddr1_arlen                  ),
    .cl_ddr1_arsize                 ( cl_ddr1_arsize                 ),
    .cl_ddr1_arburst                ( cl_ddr1_arburst                ),
    .cl_ddr1_arvalid                ( cl_ddr1_arvalid                ),
    .cl_ddr1_arid                   ( cl_ddr1_arid                   ),
    .cl_ddr1_arready                ( cl_ddr1_arready                ),
    .cl_ddr1_rdata                  ( cl_ddr1_rdata                  ),
    .cl_ddr1_rid                    ( cl_ddr1_rid                    ),
    .cl_ddr1_rresp                  ( cl_ddr1_rresp                  ),
    .cl_ddr1_rlast                  ( cl_ddr1_rlast                  ),
    .cl_ddr1_rvalid                 ( cl_ddr1_rvalid                 ),
    .cl_ddr1_rready                 ( cl_ddr1_rready                 ),

    .cl_ddr2_awaddr                 ( cl_ddr2_awaddr                 ),
    .cl_ddr2_awlen                  ( cl_ddr2_awlen                  ),
    .cl_ddr2_awsize                 ( cl_ddr2_awsize                 ),
    .cl_ddr2_awburst                ( cl_ddr2_awburst                ),
    .cl_ddr2_awvalid                ( cl_ddr2_awvalid                ),
    .cl_ddr2_awready                ( cl_ddr2_awready                ),
    .cl_ddr2_wdata                  ( cl_ddr2_wdata                  ),
    .cl_ddr2_wstrb                  ( cl_ddr2_wstrb                  ),
    .cl_ddr2_wlast                  ( cl_ddr2_wlast                  ),
    .cl_ddr2_wvalid                 ( cl_ddr2_wvalid                 ),
    .cl_ddr2_wready                 ( cl_ddr2_wready                 ),
    .cl_ddr2_bresp                  ( cl_ddr2_bresp                  ),
    .cl_ddr2_bvalid                 ( cl_ddr2_bvalid                 ),
    .cl_ddr2_bready                 ( cl_ddr2_bready                 ),
    .cl_ddr2_araddr                 ( cl_ddr2_araddr                 ),
    .cl_ddr2_arlen                  ( cl_ddr2_arlen                  ),
    .cl_ddr2_arsize                 ( cl_ddr2_arsize                 ),
    .cl_ddr2_arburst                ( cl_ddr2_arburst                ),
    .cl_ddr2_arvalid                ( cl_ddr2_arvalid                ),
    .cl_ddr2_arid                   ( cl_ddr2_arid                   ),
    .cl_ddr2_arready                ( cl_ddr2_arready                ),
    .cl_ddr2_rdata                  ( cl_ddr2_rdata                  ),
    .cl_ddr2_rid                    ( cl_ddr2_rid                    ),
    .cl_ddr2_rresp                  ( cl_ddr2_rresp                  ),
    .cl_ddr2_rlast                  ( cl_ddr2_rlast                  ),
    .cl_ddr2_rvalid                 ( cl_ddr2_rvalid                 ),
    .cl_ddr2_rready                 ( cl_ddr2_rready                 ),

    .cl_ddr3_awaddr                 ( cl_ddr3_awaddr                 ),
    .cl_ddr3_awlen                  ( cl_ddr3_awlen                  ),
    .cl_ddr3_awsize                 ( cl_ddr3_awsize                 ),
    .cl_ddr3_awburst                ( cl_ddr3_awburst                ),
    .cl_ddr3_awvalid                ( cl_ddr3_awvalid                ),
    .cl_ddr3_awready                ( cl_ddr3_awready                ),
    .cl_ddr3_wdata                  ( cl_ddr3_wdata                  ),
    .cl_ddr3_wstrb                  ( cl_ddr3_wstrb                  ),
    .cl_ddr3_wlast                  ( cl_ddr3_wlast                  ),
    .cl_ddr3_wvalid                 ( cl_ddr3_wvalid                 ),
    .cl_ddr3_wready                 ( cl_ddr3_wready                 ),
    .cl_ddr3_bresp                  ( cl_ddr3_bresp                  ),
    .cl_ddr3_bvalid                 ( cl_ddr3_bvalid                 ),
    .cl_ddr3_bready                 ( cl_ddr3_bready                 ),
    .cl_ddr3_araddr                 ( cl_ddr3_araddr                 ),
    .cl_ddr3_arlen                  ( cl_ddr3_arlen                  ),
    .cl_ddr3_arsize                 ( cl_ddr3_arsize                 ),
    .cl_ddr3_arburst                ( cl_ddr3_arburst                ),
    .cl_ddr3_arvalid                ( cl_ddr3_arvalid                ),
    .cl_ddr3_arid                   ( cl_ddr3_arid                   ),
    .cl_ddr3_arready                ( cl_ddr3_arready                ),
    .cl_ddr3_rdata                  ( cl_ddr3_rdata                  ),
    .cl_ddr3_rid                    ( cl_ddr3_rid                    ),
    .cl_ddr3_rresp                  ( cl_ddr3_rresp                  ),
    .cl_ddr3_rlast                  ( cl_ddr3_rlast                  ),
    .cl_ddr3_rvalid                 ( cl_ddr3_rvalid                 ),
    .cl_ddr3_rready                 ( cl_ddr3_rready                 ),

    .cl_ddr4_awaddr                 ( cl_ddr4_awaddr                 ),
    .cl_ddr4_awlen                  ( cl_ddr4_awlen                  ),
    .cl_ddr4_awsize                 ( cl_ddr4_awsize                 ),
    .cl_ddr4_awburst                ( cl_ddr4_awburst                ),
    .cl_ddr4_awvalid                ( cl_ddr4_awvalid                ),
    .cl_ddr4_awready                ( cl_ddr4_awready                ),
    .cl_ddr4_wdata                  ( cl_ddr4_wdata                  ),
    .cl_ddr4_wstrb                  ( cl_ddr4_wstrb                  ),
    .cl_ddr4_wlast                  ( cl_ddr4_wlast                  ),
    .cl_ddr4_wvalid                 ( cl_ddr4_wvalid                 ),
    .cl_ddr4_wready                 ( cl_ddr4_wready                 ),
    .cl_ddr4_bresp                  ( cl_ddr4_bresp                  ),
    .cl_ddr4_bvalid                 ( cl_ddr4_bvalid                 ),
    .cl_ddr4_bready                 ( cl_ddr4_bready                 ),
    .cl_ddr4_araddr                 ( cl_ddr4_araddr                 ),
    .cl_ddr4_arlen                  ( cl_ddr4_arlen                  ),
    .cl_ddr4_arsize                 ( cl_ddr4_arsize                 ),
    .cl_ddr4_arburst                ( cl_ddr4_arburst                ),
    .cl_ddr4_arvalid                ( cl_ddr4_arvalid                ),
    .cl_ddr4_arid                   ( cl_ddr4_arid                   ),
    .cl_ddr4_arready                ( cl_ddr4_arready                ),
    .cl_ddr4_rdata                  ( cl_ddr4_rdata                  ),
    .cl_ddr4_rid                    ( cl_ddr4_rid                    ),
    .cl_ddr4_rresp                  ( cl_ddr4_rresp                  ),
    .cl_ddr4_rlast                  ( cl_ddr4_rlast                  ),
    .cl_ddr4_rvalid                 ( cl_ddr4_rvalid                 ),
    .cl_ddr4_rready                 ( cl_ddr4_rready                 )
  );
//=============================================================

//=============================================================
// VCD
//=============================================================
`ifdef COCOTB_TOPLEVEL_cl_wrapper
  initial begin
    $dumpfile("cl_wrapper.vcd");
    $dumpvars(0, cl_wrapper);
  end
`endif
//=============================================================

endmodule

//
// Wrapper for memory
//
// Hardik Sharma
// (hsharma@gatech.edu)

`timescale 1ns/1ps
module pu_ld_obuf_wrapper #(
  // Internal Parameters
    parameter integer  MEM_ID                       = 0,
    parameter integer  STORE_ENABLED                = MEM_ID == 1 ? 1 : 0,
    parameter integer  MEM_REQ_W                    = 16,
    parameter integer  ADDR_WIDTH                   = 8,
    parameter integer  LOOP_ITER_W                  = 16,
    parameter integer  ADDR_STRIDE_W                = ADDR_WIDTH,
    parameter integer  LOOP_ID_W                    = 5,
    parameter integer  BUF_TYPE_W                   = 2,
    parameter integer  NUM_TAGS                     = 4,
    parameter integer  TAG_W                        = $clog2(NUM_TAGS),

    parameter integer  OBUF_AXI_DATA_WIDTH          = 256,
    parameter integer  SIMD_INTERIM_WIDTH           = 512,
    parameter integer  NUM_FIFO                     = SIMD_INTERIM_WIDTH / OBUF_AXI_DATA_WIDTH,

  // AXI
    parameter integer  AXI_DATA_WIDTH               = 64,
    parameter integer  AXI_BURST_WIDTH              = 8,
    parameter integer  WSTRB_W                      = AXI_DATA_WIDTH/8
) (
    input  wire                                         clk,
    input  wire                                         reset,

    input  wire                                         start,
    output wire                                         done,
    input  wire  [ ADDR_WIDTH           -1 : 0 ]        base_addr,

  // Programming
    input  wire                                         cfg_loop_stride_v,
    input  wire  [ ADDR_STRIDE_W        -1 : 0 ]        cfg_loop_stride,
    input  wire  [ 3                    -1 : 0 ]        cfg_loop_stride_type,

    input  wire                                         cfg_loop_iter_v,
    input  wire  [ LOOP_ITER_W          -1 : 0 ]        cfg_loop_iter,
    input  wire  [ 3                    -1 : 0 ]        cfg_loop_iter_type,

  // LD
    output wire                                         mem_req,
    input  wire                                         mem_ready,
    output wire  [ ADDR_WIDTH           -1 : 0 ]        mem_addr,

    input  wire                                         obuf_ld_stream_write_ready
);

//==============================================================================
// Localparams
//==============================================================================
//==============================================================================

//==============================================================================
// Wires/Regs
//==============================================================================
    reg  [ LOOP_ID_W            -1 : 0 ]        mem_loop_id_counter;

    wire                                        fifo_stall;
    wire                                        fsm_stall;
    wire                                        loop_ctrl_stall;
    wire [ LOOP_ID_W            -1 : 0 ]        loop_ctrl_index;
    wire                                        loop_ctrl_index_valid;
    wire                                        loop_ctrl_init;
    wire                                        loop_ctrl_done;
    wire                                        loop_ctrl_enter;
    wire                                        loop_ctrl_exit;
    wire                                        loop_ctrl_next_addr;

    wire [ ADDR_WIDTH           -1 : 0 ]        ld_addr;
    reg  [ ADDR_WIDTH           -1 : 0 ]        ld_addr_d;
    reg  [ ADDR_WIDTH           -1 : 0 ]        ld_addr_q;
    wire                                        ld_addr_valid;

    wire                                        obuf_ld_loop_iter_v;
//==============================================================================

//==============================================================================
// Assigns
//==============================================================================
    reg                                         mem_access_state_d;
    reg                                         mem_access_state_q;
    reg                                         done_state_d;
    reg                                         done_state_q;
    assign fifo_stall = ~mem_ready || ~obuf_ld_stream_write_ready;
    assign loop_ctrl_stall = fifo_stall || fsm_stall;
    assign mem_req = (~fifo_stall) && (ld_addr_valid || fsm_stall);
    assign loop_ctrl_next_addr = loop_ctrl_index_valid && ~loop_ctrl_stall;
    assign done = mem_access_state_q == 0 && done_state_q == 1;
    // assign done = loop_ctrl_done;
//==============================================================================

//==============================================================================
// OBUF LD Address Generation
//==============================================================================

    // Need done state for the case when we need to stall after the loop
    // controller has finished

    localparam          FIFO_ID_WIDTH                = $clog2(NUM_FIFO);
    reg  [ FIFO_ID_WIDTH        -1 : 0 ]        fifo_id_d;
    reg  [ FIFO_ID_WIDTH        -1 : 0 ]        fifo_id_q;

    always @(posedge clk)
    begin
      if (reset) begin
        mem_access_state_q <= 1'b0;
        fifo_id_q <= 0;
        ld_addr_q <= 0;
      end else begin
        mem_access_state_q <= mem_access_state_d;
        fifo_id_q <= fifo_id_d;
        ld_addr_q <= ld_addr_d;
      end
    end

    assign fsm_stall = mem_access_state_q == 1;

generate
if (NUM_FIFO == 1) begin
    assign mem_addr = ld_addr;
end else begin
    assign mem_addr = mem_access_state_q ? {ld_addr_q, fifo_id_q} : {ld_addr, fifo_id_q};
end
endgenerate

    always @(*)
    begin: MEM_ACCESS_STATE
      mem_access_state_d = mem_access_state_q;
      fifo_id_d = fifo_id_q;
      ld_addr_d = ld_addr_q;
      case (mem_access_state_q)
        0: begin
          if (mem_req && NUM_FIFO > 1) begin
            mem_access_state_d = 1;
            fifo_id_d = 1;
            ld_addr_d = ld_addr;
          end
        end
        1: begin
          if (mem_req) begin
            if (fifo_id_q == NUM_FIFO-1) begin
              fifo_id_d = 0;
              mem_access_state_d = 0;
            end else begin
              fifo_id_d = fifo_id_q + 1;
            end
          end
        end
      endcase
    end

    always @(posedge clk)
    begin
      if (reset)
        done_state_q <= 1'b0;
      else
        done_state_q <= done_state_d;
    end

    always @(*)
    begin
      done_state_d = done_state_q;
      case (done_state_q)
        1'b0: begin
          if (loop_ctrl_done)
            done_state_d = 1'b1;
        end
        1'b1: begin
          if (done)
            done_state_d = 1'b0;
        end
      endcase
    end
//==============================================================================

//==============================================================================
// Address generators
//==============================================================================
    wire                                        obuf_ld_stride_v;
    assign obuf_ld_stride_v = cfg_loop_stride_v && cfg_loop_stride_type == 0;
  mem_walker_stride #(
    .ADDR_WIDTH                     ( ADDR_WIDTH                     ),
    .ADDR_STRIDE_W                  ( ADDR_STRIDE_W                  ),
    .LOOP_ID_W                      ( LOOP_ID_W                      )
  ) mws_ld (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .base_addr                      ( base_addr                      ), //input
    .loop_ctrl_done                 ( loop_ctrl_done                 ), //input
    .loop_index                     ( loop_ctrl_index                ), //input
    .loop_index_valid               ( loop_ctrl_next_addr            ), //input
    .loop_init                      ( loop_ctrl_init                 ), //input
    .loop_enter                     ( loop_ctrl_enter                ), //input
    .loop_exit                      ( loop_ctrl_exit                 ), //input
    .cfg_addr_stride_v              ( obuf_ld_stride_v               ), //input
    .cfg_addr_stride                ( cfg_loop_stride                ), //input
    .addr_out                       ( ld_addr                        ), //output
    .addr_out_valid                 ( ld_addr_valid                  )  //output
  );
//==============================================================================

//==============================================================================
// Loop controller
//==============================================================================
  always@(posedge clk)
  begin
    if (reset)
      mem_loop_id_counter <= 'b0;
    else begin
      if (obuf_ld_loop_iter_v)
        mem_loop_id_counter <= mem_loop_id_counter + 1'b1;
      else if (start)
        mem_loop_id_counter <= 'b0;
    end
  end

    assign obuf_ld_loop_iter_v = cfg_loop_iter_v && cfg_loop_iter_type == 0;

  controller_fsm #(
    .LOOP_ID_W                      ( LOOP_ID_W                      ),
    .LOOP_ITER_W                    ( LOOP_ITER_W                    ),
    .IMEM_ADDR_W                    ( LOOP_ID_W                      )
  ) loop_ctrl (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .stall                          ( loop_ctrl_stall                ), //input
    .cfg_loop_iter_v                ( obuf_ld_loop_iter_v            ), //input
    .cfg_loop_iter                  ( cfg_loop_iter                  ), //input
    .cfg_loop_iter_loop_id          ( mem_loop_id_counter            ), //input
    .start                          ( start                          ), //input
    .done                           ( loop_ctrl_done                 ), //output
    .loop_init                      ( loop_ctrl_init                 ), //output
    .loop_enter                     ( loop_ctrl_enter                ), //output
    .loop_last_iter                 (                                ), //output
    .loop_exit                      ( loop_ctrl_exit                 ), //output
    .loop_index                     ( loop_ctrl_index                ), //output
    .loop_index_valid               ( loop_ctrl_index_valid          )  //output
  );
//==============================================================================

//==============================================================================
// VCD
//==============================================================================
`ifdef COCOTB_TOPLEVEL_pu_ld_obuf_wrapper
initial begin
  $dumpfile("pu_ld_obuf_wrapper.vcd");
  $dumpvars(0, pu_ld_obuf_wrapper);
end
`endif
//==============================================================================
endmodule
`timescale 1ns / 1ps
module gen_pu_ctrl #(
    parameter integer  ADDR_WIDTH                   = 42,
    parameter integer  BUF_TYPE_W                   = 2,
    parameter integer  INST_WIDTH                   = 32,
    parameter integer  DATA_WIDTH                   = 32,
    parameter integer  IMEM_ADDR_WIDTH              = 5, // 1K = 1 BRAM
    parameter integer  IMM_WIDTH                    = 16,
    parameter integer  ADDR_STRIDE_W                = 32,
    parameter integer  FN_WIDTH                     = 3,
    parameter integer  RF_ADDR_WIDTH                = 4,
    parameter integer  OP_CODE_W                    = 4,
    parameter integer  OP_SPEC_W                    = 7,
    parameter integer  LOOP_ID_W                    = 5,
    parameter integer  LOOP_ITER_W                  = IMM_WIDTH
) (
    input  wire                                         clk,
    input  wire                                         reset,

  // DEBUG
    output wire  [ 3                    -1 : 0 ]        pu_ctrl_state,

  // Handshake - program
    input  wire                                         pu_block_start,
    input  wire                                         pu_compute_start,
    output wire                                         pu_compute_ready,

  // Handshake - compute
    output wire                                         done,

  // Buffer instruction write (to PE) interface
  // TODO: connect inst_wr_req
    input  wire                                         inst_wr_req,
    input  wire  [ INST_WIDTH           -1 : 0 ]        inst_wr_data,
    output wire                                         inst_wr_ready,

  // data streamer - loop iterations
    output wire                                         cfg_loop_iter_v,
    output wire  [ IMM_WIDTH            -1 : 0 ]        cfg_loop_iter,
    output wire  [ 3                    -1 : 0 ]        cfg_loop_iter_type,

  // data streamer - address generation
    output wire                                         cfg_loop_stride_v,
    output wire  [ ADDR_STRIDE_W        -1 : 0 ]        cfg_loop_stride,
    output wire  [ 3                    -1 : 0 ]        cfg_loop_stride_type,

  // ddr ld streamer
    output wire                                         cfg_mem_req_v,
    output wire  [ 2                    -1 : 0 ]        cfg_mem_req_type,

    output wire  [ ADDR_WIDTH           -1 : 0 ]        tag_ld0_base_addr,
    output wire  [ ADDR_WIDTH           -1 : 0 ]        tag_ld1_base_addr,
    output wire  [ ADDR_WIDTH           -1 : 0 ]        tag_st_base_addr,

  // data streamer - address generation
    output wire                                         alu_fn_valid,
    output wire  [ FN_WIDTH             -1 : 0 ]        alu_fn,
    output wire  [ RF_ADDR_WIDTH        -1 : 0 ]        alu_in0_addr,
    output wire                                         alu_in1_src,
    output wire  [ RF_ADDR_WIDTH        -1 : 0 ]        alu_in1_addr,
    output wire  [ RF_ADDR_WIDTH        -1 : 0 ]        alu_out_addr,
    output wire  [ IMM_WIDTH            -1 : 0 ]        alu_imm,

  // From controller
    output wire                                         obuf_ld_stream_read_req,
    input  wire                                         obuf_ld_stream_read_ready,
    output wire                                         ddr_ld0_stream_read_req,
    input  wire                                         ddr_ld0_stream_read_ready,
    output wire                                         ddr_ld1_stream_read_req,
    input  wire                                         ddr_ld1_stream_read_ready,
    output wire                                         ddr_st_stream_write_req,
    input  wire                                         ddr_st_stream_write_ready,
    input  wire                                         ddr_st_done
);

//==============================================================================
// Localparams
//==============================================================================
    localparam          BASE_ADDR_PART_W             = IMM_WIDTH + LOOP_ID_W;
    localparam integer  PU_CTRL_IDLE                 = 0;
    localparam integer  PU_CTRL_DECODE               = 1;
    localparam integer  PU_CTRL_COMPUTE_START        = 2;
    localparam integer  PU_BASE_ADDR_CALC            = 3;
    localparam integer  PU_CTRL_COMPUTE_WAIT         = 4;
    localparam integer  PU_CTRL_COMPUTE              = 5;
    localparam integer  PU_CTRL_COMPUTE_DONE         = 6;
    localparam integer  PU_CTRL_DONE                 = 7;

    localparam integer  OP_SETUP                     = 0;
    localparam integer  OP_LDMEM                     = 1;
    localparam integer  OP_STMEM                     = 2;
    localparam integer  OP_RDBUF                     = 3;
    localparam integer  OP_WRBUF                     = 4;
    localparam integer  OP_GENADDR_HI                = 5;
    localparam integer  OP_GENADDR_LO                = 6;
    localparam integer  OP_LOOP                      = 7;
    localparam integer  OP_BLOCK_REPEAT              = 8;
    localparam integer  OP_BASE_ADDR                 = 9;
    localparam integer  OP_PU_BLOCK                  = 10;
    localparam integer  OP_COMPUTE_R                 = 11;
    localparam integer  OP_COMPUTE_I                 = 12;

    localparam          LD_OBUF                      = 0;
    localparam          LD0_DDR                      = 1;
    localparam          LD1_DDR                      = 2;
//==============================================================================

//==============================================================================
// Wires & Regs
//==============================================================================
    reg  [ IMM_WIDTH            -1 : 0 ]        loop_stride_hi;

    wire [ ADDR_WIDTH           -1 : 0 ]        st_addr;
    wire                                        st_addr_valid;

    wire [ ADDR_WIDTH           -1 : 0 ]        ld0_addr;
    wire                                        ld0_addr_valid;
    wire [ ADDR_WIDTH           -1 : 0 ]        ld1_addr;
    wire                                        ld1_addr_valid;

    reg  [ 1                    -1 : 0 ]        stmem_state_d;
    reg  [ 1                    -1 : 0 ]        stmem_state_q;

    reg                                         loop_status_d;
    reg                                         loop_status_q;

    reg  [ ADDR_WIDTH           -1 : 0 ]        tag_ld0_base_addr_d;
    reg  [ ADDR_WIDTH           -1 : 0 ]        tag_ld0_base_addr_q;
    reg  [ ADDR_WIDTH           -1 : 0 ]        tag_ld1_base_addr_d;
    reg  [ ADDR_WIDTH           -1 : 0 ]        tag_ld1_base_addr_q;
    reg  [ ADDR_WIDTH           -1 : 0 ]        tag_st_base_addr_d;
    reg  [ ADDR_WIDTH           -1 : 0 ]        tag_st_base_addr_q;

    wire                                        loop_ctrl_loop_iter_v;
    wire [ IMM_WIDTH            -1 : 0 ]        loop_ctrl_loop_iter;
    wire                                        loop_ctrl_loop_done;
    wire                                        loop_ctrl_loop_init;
    wire                                        loop_ctrl_loop_enter;
    wire                                        loop_ctrl_loop_exit;
    wire [ LOOP_ID_W            -1 : 0 ]        loop_ctrl_loop_index;
    wire                                        loop_ctrl_loop_index_valid;
    wire                                        loop_ctrl_loop_index_step;

    wire                                        loop_ctrl_start;
    wire                                        loop_ctrl_stall;
    reg  [ LOOP_ID_W            -1 : 0 ]        loop_ctrl_loop_id_counter;
    wire                                        st_stride_v;
    wire [ ADDR_STRIDE_W        -1 : 0 ]        st_stride;
    wire                                        ld0_stride_v;
    wire [ ADDR_STRIDE_W        -1 : 0 ]        ld0_stride;
    wire                                        ld1_stride_v;
    wire [ ADDR_STRIDE_W        -1 : 0 ]        ld1_stride;

    wire [ ADDR_WIDTH           -1 : 0 ]        ld0_tensor_base_addr;
    wire [ ADDR_WIDTH           -1 : 0 ]        ld1_tensor_base_addr;
    wire [ ADDR_WIDTH           -1 : 0 ]        st_tensor_base_addr;
    wire                                        cfg_loop_stride_base;

    reg  [ 3                    -1 : 0 ]        pu_ctrl_state_d;
    reg  [ 3                    -1 : 0 ]        pu_ctrl_state_q;
    wire                                        instruction_valid;
    wire                                        pu_block_end;

    wire [ OP_CODE_W            -1 : 0 ]        op_code;
    wire [ OP_SPEC_W            -1 : 0 ]        op_spec;
    wire [ LOOP_ID_W            -1 : 0 ]        loop_id;
    wire [ IMM_WIDTH            -1 : 0 ]        imm;

    wire                                        stall;
    wire                                        _alu_fn_valid;
    wire                                        _obuf_ld_stream_read_req;
    wire                                        _ddr_ld0_stream_read_req;
    wire                                        _ddr_ld1_stream_read_req;
    wire                                        _ddr_st_stream_write_req;
    wire                                        _ddr_st_stream_write_req_dly1;
    wire                                        _ddr_st_stream_write_req_dly2;
    wire                                        _ddr_st_stream_write_req_dly3;

    wire [ IMM_WIDTH            -1 : 0 ]        block_inst_repeat;
    reg  [ IMM_WIDTH            -1 : 0 ]        block_inst_repeat_d;
    reg  [ IMM_WIDTH            -1 : 0 ]        block_inst_repeat_q;
    reg  [ IMM_WIDTH            -1 : 0 ]        repeat_counter_d;
    reg  [ IMM_WIDTH            -1 : 0 ]        repeat_counter_q;

    //reg  [ INST_WIDTH           -1 : 0 ]        mem [ 0 : 1<<IMEM_ADDR_WIDTH ];
    reg  [ IMEM_ADDR_WIDTH      -1 : 0 ]        imem_wr_addr;
    reg  [ IMEM_ADDR_WIDTH      -1 : 0 ]        imem_rd_addr;
    reg  [ IMEM_ADDR_WIDTH      -1 : 0 ]        curr_imem_rd_addr;
    wire [ IMEM_ADDR_WIDTH      -1 : 0 ]        next_imem_rd_addr;
    wire                                        imem_wr_req;
    wire                                        imem_rd_req;

    wire [ INST_WIDTH           -1 : 0 ]        imem_wr_data;
    wire [ INST_WIDTH           -1 : 0 ]        imem_rd_data;

    reg  [ IMEM_ADDR_WIDTH      -1 : 0 ]        last_inst_addr;
    wire                                        last_inst;
//==============================================================================

//==============================================================================
// Repeat logic
//==============================================================================
    assign imem_wr_req = (op_code == OP_COMPUTE_R || op_code == OP_COMPUTE_I) && pu_ctrl_state_q == PU_CTRL_DECODE;

  always @(posedge clk)
  begin
    if (reset) begin
      imem_wr_addr <= 0;
    end
    else if (done)
      imem_wr_addr <= 0;
    else if (imem_wr_req) begin
      imem_wr_addr <= imem_wr_addr + 1'b1;
    end
  end

  always @(posedge clk)
  begin
    if (reset)
      last_inst_addr <= 0;
    else if (imem_wr_req)
      last_inst_addr <= imem_wr_addr;
  end

  always @(posedge clk)
  begin
    if (reset) begin
      imem_rd_addr <= 0;
    end
    else begin
      if ((pu_ctrl_state_q == PU_CTRL_COMPUTE) && (~stall || (imem_rd_addr == curr_imem_rd_addr)))
        imem_rd_addr <= next_imem_rd_addr;
      else if (pu_ctrl_state_q == PU_CTRL_DONE)
        imem_rd_addr <= 0;
    end
  end

  always @(posedge clk)
  begin
    if (reset)
      curr_imem_rd_addr <= 0;
    else if (imem_rd_req)
      curr_imem_rd_addr <= imem_rd_addr;
  end

    assign next_imem_rd_addr = imem_rd_addr == last_inst_addr ? 0 : imem_rd_addr + 1'b1;
    assign imem_rd_req = (pu_ctrl_state_q == PU_CTRL_DECODE || pu_ctrl_state_q == PU_CTRL_COMPUTE) && ~stall;
    assign last_inst = last_inst_addr == curr_imem_rd_addr;

  always @(posedge clk)
  begin
    if (reset)
      repeat_counter_q <= 0;
    else
      repeat_counter_q <= repeat_counter_d;
  end

    assign imem_wr_data = inst_wr_data;

//  always @(posedge clk)
//  begin: RAM_WRITE
//    if (imem_wr_req)
//      mem[imem_wr_addr] <= imem_wr_data;
//  end
//
//  always @(posedge clk)
//  begin: RAM_READ
//    if (imem_rd_req)
//      imem_rd_data <= mem[imem_rd_addr];
//  end

  ram #(
    .ADDR_WIDTH                     ( IMEM_ADDR_WIDTH),
    .DATA_WIDTH                     ( INST_WIDTH)
  ) u_ram_imem(
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .s_write_addr                   ( imem_wr_addr),
    .s_write_req                    ( imem_wr_req),
    .s_write_data                   ( imem_wr_data),
    .s_read_addr                    ( imem_rd_addr),
    .s_read_req                     ( imem_rd_req),
    .s_read_data                    ( imem_rd_data)
  );
//==============================================================================

//==============================================================================
// FSM for the controller
//==============================================================================
  always @(posedge clk)
  begin
    if (reset)
      pu_ctrl_state_q <= PU_CTRL_IDLE;
    else
      pu_ctrl_state_q <= pu_ctrl_state_d;
  end
    assign pu_ctrl_state = pu_ctrl_state_q;

  always @(posedge clk)
  begin
    if (reset)
      block_inst_repeat_q <= 0;
    else
      block_inst_repeat_q <= block_inst_repeat_d;
  end

  always @(*)
  begin
    pu_ctrl_state_d = pu_ctrl_state_q;
    repeat_counter_d = repeat_counter_q;
    block_inst_repeat_d = block_inst_repeat_q;
    case (pu_ctrl_state_q)
      PU_CTRL_IDLE: begin
        if (pu_block_start)
          pu_ctrl_state_d = PU_CTRL_DECODE;
      end
      PU_CTRL_DECODE: begin
        if (pu_block_end) begin
          pu_ctrl_state_d = PU_CTRL_COMPUTE_START;
          block_inst_repeat_d = block_inst_repeat;
        end
      end
      PU_CTRL_COMPUTE_START: begin
        // Get base addresses
        repeat_counter_d = block_inst_repeat_q;
        if (stmem_state_q == 0)
          pu_ctrl_state_d = PU_BASE_ADDR_CALC;
      end
      PU_BASE_ADDR_CALC: begin
        if (stmem_state_q == 1) begin
          pu_ctrl_state_d = PU_CTRL_COMPUTE_WAIT;
        end
      end
      PU_CTRL_COMPUTE_WAIT: begin
        if (pu_compute_start)
          pu_ctrl_state_d = PU_CTRL_COMPUTE;
      end
      PU_CTRL_COMPUTE: begin
        if (last_inst && ~stall) begin
          if (repeat_counter_q == 0) begin
            pu_ctrl_state_d = PU_CTRL_COMPUTE_DONE;
          end
          else
            repeat_counter_d = repeat_counter_q - 1'b1;
        end
      end
      PU_CTRL_COMPUTE_DONE: begin
        if (loop_status_q)
          pu_ctrl_state_d = PU_CTRL_DONE;
        else
          pu_ctrl_state_d = PU_CTRL_COMPUTE_START;
      end
      PU_CTRL_DONE: begin
        pu_ctrl_state_d = PU_CTRL_IDLE;
      end
    endcase
  end

  always @(posedge clk)
  begin
    tag_st_base_addr_q <= tag_st_base_addr_d;
  end
    assign tag_st_base_addr = tag_st_base_addr_q;

    assign pu_compute_ready = pu_ctrl_state_q == PU_CTRL_COMPUTE_WAIT;
    assign done = pu_ctrl_state_q == PU_CTRL_DONE;
    assign pu_block_end = op_code == OP_BLOCK_REPEAT;
    assign instruction_valid = pu_ctrl_state_q == PU_CTRL_DECODE;
    assign inst_wr_ready = pu_ctrl_state_q != PU_CTRL_COMPUTE;
//==============================================================================

//==============================================================================
// Decode
//==============================================================================
    assign {op_code, op_spec, loop_id, imm} = inst_wr_data;
    assign cfg_loop_iter_v = instruction_valid && op_code == OP_LOOP;
    assign cfg_loop_iter = imm;
    assign cfg_loop_iter_type = op_spec[2:0];

    assign cfg_loop_stride_v = instruction_valid && op_code == OP_GENADDR_LO;
    assign cfg_loop_stride[IMM_WIDTH-1:0] = imm;
    assign cfg_loop_stride_type = op_spec[2:0];

    assign cfg_mem_req_v = instruction_valid && op_code == OP_LDMEM;
    assign cfg_mem_req_type = op_spec[5:3];

    assign block_inst_repeat = imm;

  always @(posedge clk)
  begin
    if (reset)
      loop_stride_hi <= 0;
    else begin
      if (cfg_loop_stride_v || done)
        loop_stride_hi <= 0;
      else if (op_code == OP_GENADDR_HI && instruction_valid)
        loop_stride_hi <= imm;
    end
  end

    assign cfg_loop_stride[ADDR_STRIDE_W-1:IMM_WIDTH] = loop_stride_hi;
//==============================================================================

//==============================================================================
// Stall logic
//==============================================================================
    assign _alu_fn_valid = pu_ctrl_state_q == PU_CTRL_COMPUTE;
    assign alu_fn_valid = _alu_fn_valid && ~stall;
    assign {alu_in1_src, alu_fn, alu_imm, alu_in0_addr, alu_out_addr} = imem_rd_data;
    assign alu_in1_addr = alu_imm[3:0];

    assign _obuf_ld_stream_read_req = _alu_fn_valid &&
                                     ((alu_in0_addr[3] == 1 &&
                                       alu_in0_addr[2:0] == LD_OBUF) ||
                                       (~alu_in1_src &&
                                         alu_in1_addr[3] == 1 &&
                                         alu_in1_addr[2:0] == LD_OBUF));

    assign _ddr_ld0_stream_read_req = _alu_fn_valid &&
                                     ((alu_in0_addr[3] == 1 &&
                                       alu_in0_addr[2:0] == LD0_DDR) ||
                                       (~alu_in1_src &&
                                         alu_in1_addr[3] == 1 &&
                                         alu_in1_addr[2:0] == LD0_DDR));

    assign _ddr_ld1_stream_read_req = _alu_fn_valid &&
                                     ((alu_in0_addr[3] == 1 &&
                                       alu_in0_addr[2:0] == LD1_DDR) ||
                                       (~alu_in1_src &&
                                         alu_in1_addr[3] == 1 &&
                                         alu_in1_addr[2:0] == LD1_DDR));

    assign _ddr_st_stream_write_req = _alu_fn_valid &&
                                      (alu_out_addr[3] == 1);

    register_sync_with_enable #(1) ddr_st_delay
    (clk, reset, 1'b1, _ddr_st_stream_write_req && ~stall, _ddr_st_stream_write_req_dly1);
    register_sync_with_enable #(1) ddr_st_delay2
    (clk, reset, 1'b1, _ddr_st_stream_write_req_dly1, _ddr_st_stream_write_req_dly2);
    register_sync_with_enable #(1) ddr_st_delay3
    (clk, reset, 1'b1, _ddr_st_stream_write_req_dly2, _ddr_st_stream_write_req_dly3);

    assign ddr_st_stream_write_req = _ddr_st_stream_write_req_dly2;
    assign obuf_ld_stream_read_req = _obuf_ld_stream_read_req && ~stall;
    assign ddr_ld0_stream_read_req = _ddr_ld0_stream_read_req && ~stall;
    assign ddr_ld1_stream_read_req = _ddr_ld1_stream_read_req && ~stall;

    assign stall = (_obuf_ld_stream_read_req && ~obuf_ld_stream_read_ready) ||
                   (_ddr_ld0_stream_read_req && ~ddr_ld0_stream_read_ready) ||
                   (_ddr_ld1_stream_read_req && ~ddr_ld1_stream_read_ready) ||
                   (_ddr_st_stream_write_req && ~ddr_st_stream_write_ready);
//==============================================================================

//==============================================================================
// Base Address
//==============================================================================
    wire                                        base_addr_v;
    wire [ BUF_TYPE_W           -1 : 0 ]        base_addr_id;
    wire [ 2                    -1 : 0 ]        base_addr_part;
    wire [ BASE_ADDR_PART_W     -1 : 0 ]        base_addr;
    wire [ 3                    -1 : 0 ]        buf_id;
    assign base_addr_v = pu_ctrl_state == PU_CTRL_DECODE && (op_code == OP_BASE_ADDR);
    assign base_addr = {loop_id, imm};
    assign base_addr_id = buf_id;
    assign buf_id = op_spec[5:3];
    assign base_addr_part = op_spec[1:0];

genvar i;
generate
for (i=0; i<2; i=i+1)
begin: BASE_ADDR_CFG

    reg  [ BASE_ADDR_PART_W     -1 : 0 ]        part_ld0_base_addr;
    reg  [ BASE_ADDR_PART_W     -1 : 0 ]        part_ld1_base_addr;
    reg  [ BASE_ADDR_PART_W     -1 : 0 ]        part_st_base_addr;

    assign ld0_tensor_base_addr[i*BASE_ADDR_PART_W+:BASE_ADDR_PART_W] = part_ld0_base_addr;
    assign ld1_tensor_base_addr[i*BASE_ADDR_PART_W+:BASE_ADDR_PART_W] = part_ld1_base_addr;
    assign st_tensor_base_addr[i*BASE_ADDR_PART_W+:BASE_ADDR_PART_W]  = part_st_base_addr;

      always @(posedge clk)
        if (reset)
          part_ld0_base_addr <= 0;
        else if (base_addr_v && base_addr_id == 2 && base_addr_part == i)
          part_ld0_base_addr <= base_addr;
        else if (done)
          part_ld0_base_addr <= 0;

      always @(posedge clk)
        if (reset)
          part_ld1_base_addr <= 0;
        else if (base_addr_v && base_addr_id == 3 && base_addr_part == i)
          part_ld1_base_addr <= base_addr;
        else if (done)
          part_ld1_base_addr <= 0;

      always @(posedge clk)
        if (reset)
          part_st_base_addr <= 0;
        else if (base_addr_v && base_addr_id == 1 && base_addr_part == i)
          part_st_base_addr <= base_addr;
        else if (done)
          part_st_base_addr <= 0;

end
endgenerate
//==============================================================================

//==============================================================================
// Controller
//==============================================================================
    assign loop_ctrl_start = pu_ctrl_state_q == PU_CTRL_COMPUTE_START;
    assign loop_ctrl_stall = ~(pu_ctrl_state_q == PU_BASE_ADDR_CALC && stmem_state_q == 0);
    assign loop_ctrl_loop_iter_v = cfg_loop_iter_v && cfg_loop_iter_type == 5
                                   && pu_ctrl_state_q == PU_CTRL_DECODE;
    assign loop_ctrl_loop_iter = cfg_loop_iter;

    assign loop_ctrl_loop_index_step = loop_ctrl_loop_index_valid && ~loop_ctrl_stall;

    assign st_stride_v = cfg_loop_stride_v && cfg_loop_stride_type == 5
                         && pu_ctrl_state_q == PU_CTRL_DECODE;
    assign st_stride = cfg_loop_stride;

    assign ld0_stride_v = cfg_loop_stride_v && cfg_loop_stride_type == 6
                          && pu_ctrl_state_q == PU_CTRL_DECODE;
    assign ld0_stride = cfg_loop_stride;

    assign ld1_stride_v = cfg_loop_stride_v && cfg_loop_stride_type == 7
                         && pu_ctrl_state_q == PU_CTRL_DECODE;
    assign ld1_stride = cfg_loop_stride;

always @(posedge clk)
begin
  if (reset)
    loop_ctrl_loop_id_counter <= 0;
  else begin
    if (pu_ctrl_state_q == 0)
      loop_ctrl_loop_id_counter <= 0;
    else if (loop_ctrl_loop_iter_v)
      loop_ctrl_loop_id_counter <= loop_ctrl_loop_id_counter + 1'b1;
  end
end

  controller_fsm #(
    .LOOP_ID_W                      ( LOOP_ID_W                      ),
    .LOOP_ITER_W                    ( LOOP_ITER_W                    ),
    .IMEM_ADDR_W                    ( LOOP_ID_W                      )
  ) loop_ctrl_st (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .stall                          ( loop_ctrl_stall                ), //input
    .cfg_loop_iter_v                ( loop_ctrl_loop_iter_v          ), //input
    .cfg_loop_iter                  ( loop_ctrl_loop_iter            ), //input
    .cfg_loop_iter_loop_id          ( loop_ctrl_loop_id_counter      ), //input
    .start                          ( loop_ctrl_start                ), //input
    .done                           ( loop_ctrl_loop_done            ), //output
    .loop_init                      ( loop_ctrl_loop_init            ), //output
    .loop_enter                     ( loop_ctrl_loop_enter           ), //output
    .loop_last_iter                 (                                ), //output
    .loop_exit                      ( loop_ctrl_loop_exit            ), //output
    .loop_index                     ( loop_ctrl_loop_index           ), //output
    .loop_index_valid               ( loop_ctrl_loop_index_valid     )  //output
  );

  mem_walker_stride #(
    .ADDR_WIDTH                     ( ADDR_WIDTH                     ),
    .ADDR_STRIDE_W                  ( ADDR_STRIDE_W                  ),
    .LOOP_ID_W                      ( LOOP_ID_W                      )
  ) mws_st (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .base_addr                      ( st_tensor_base_addr            ), //input
    .loop_ctrl_done                 ( loop_ctrl_loop_done            ), //input
    .loop_index                     ( loop_ctrl_loop_index           ), //input
    .loop_index_valid               ( loop_ctrl_loop_index_step      ), //input
    .loop_init                      ( loop_ctrl_loop_init            ), //input
    .loop_enter                     ( loop_ctrl_loop_enter           ), //input
    .loop_exit                      ( loop_ctrl_loop_exit            ), //input
    .cfg_addr_stride_v              ( st_stride_v                    ), //input
    .cfg_addr_stride                ( st_stride                      ), //input
    .addr_out                       ( st_addr                        ), //output
    .addr_out_valid                 ( st_addr_valid                  )  //output
  );

  mem_walker_stride #(
    .ADDR_WIDTH                     ( ADDR_WIDTH                     ),
    .ADDR_STRIDE_W                  ( ADDR_STRIDE_W                  ),
    .LOOP_ID_W                      ( LOOP_ID_W                      )
  ) mws_ld0 (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .base_addr                      ( ld0_tensor_base_addr           ), //input
    .loop_ctrl_done                 ( loop_ctrl_loop_done            ), //input
    .loop_index                     ( loop_ctrl_loop_index           ), //input
    .loop_index_valid               ( loop_ctrl_loop_index_step      ), //input
    .loop_init                      ( loop_ctrl_loop_init            ), //input
    .loop_enter                     ( loop_ctrl_loop_enter           ), //input
    .loop_exit                      ( loop_ctrl_loop_exit            ), //input
    .cfg_addr_stride_v              ( ld0_stride_v                   ), //input
    .cfg_addr_stride                ( ld0_stride                     ), //input
    .addr_out                       ( ld0_addr                       ), //output
    .addr_out_valid                 ( ld0_addr_valid                 )  //output
  );

  mem_walker_stride #(
    .ADDR_WIDTH                     ( ADDR_WIDTH                     ),
    .ADDR_STRIDE_W                  ( ADDR_STRIDE_W                  ),
    .LOOP_ID_W                      ( LOOP_ID_W                      )
  ) mws_ld1 (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .base_addr                      ( ld1_tensor_base_addr           ), //input
    .loop_ctrl_done                 ( loop_ctrl_loop_done            ), //input
    .loop_index                     ( loop_ctrl_loop_index           ), //input
    .loop_index_valid               ( loop_ctrl_loop_index_step      ), //input
    .loop_init                      ( loop_ctrl_loop_init            ), //input
    .loop_enter                     ( loop_ctrl_loop_enter           ), //input
    .loop_exit                      ( loop_ctrl_loop_exit            ), //input
    .cfg_addr_stride_v              ( ld1_stride_v                   ), //input
    .cfg_addr_stride                ( ld1_stride                     ), //input
    .addr_out                       ( ld1_addr                       ), //output
    .addr_out_valid                 ( ld1_addr_valid                 )  //output
  );
//==============================================================================


//==============================================================================
// LD0 and LD1 base addr
//==============================================================================
always @(posedge clk)
begin
  if (reset)
    tag_ld0_base_addr_q <= 0;
  else if (ld0_addr_valid)
    tag_ld0_base_addr_q <= ld0_addr;
end
always @(posedge clk)
begin
  if (reset)
    tag_ld1_base_addr_q <= 0;
  else if (ld1_addr_valid)
    tag_ld1_base_addr_q <= ld1_addr;
end
    assign tag_ld0_base_addr = tag_ld0_base_addr_q;
    assign tag_ld1_base_addr = tag_ld1_base_addr_q;
//==============================================================================

//==============================================================================
// Store control
//==============================================================================
always @(posedge clk)
begin
  if (reset)
    stmem_state_q <= 0;
  else
    stmem_state_q <= stmem_state_d;
end
always @(*)
begin
  stmem_state_d = stmem_state_q;
  tag_st_base_addr_d = tag_st_base_addr_q;
  case (stmem_state_q)
    0: begin
      if (st_addr_valid) begin
        stmem_state_d = 1;
        tag_st_base_addr_d = st_addr;
      end
    end
    1: begin
      if (ddr_st_done)
        stmem_state_d = 0;
    end
  endcase
end

always @(posedge clk)
begin
  if (reset)
    loop_status_q <= 0;
  else
    loop_status_q <= loop_status_d;
end

always @(*)
begin
  loop_status_d = loop_status_q;
  case(loop_status_q)
    0: begin
      if (loop_ctrl_loop_done)
        loop_status_d = 1;
    end
    1: begin
      if (pu_ctrl_state_q == PU_CTRL_DONE)
        loop_status_d = 0;
    end
  endcase
end
//==============================================================================

endmodule
`timescale 1ns / 1ps
module gen_pu #(
  // Instruction width for PU controller
    parameter integer  INST_WIDTH                   = 32,
  // Data width
    parameter integer  DATA_WIDTH                   = 16,
    parameter integer  ACC_DATA_WIDTH               = 64,
  // SIMD Data width
    parameter integer  SIMD_LANES                   = 4,
    parameter integer  SIMD_DATA_WIDTH              = DATA_WIDTH * SIMD_LANES,

    parameter integer  OBUF_AXI_DATA_WIDTH          = ACC_DATA_WIDTH * SIMD_LANES,
    parameter integer  SIMD_INTERIM_WIDTH           = SIMD_LANES * ACC_DATA_WIDTH,
    parameter integer  NUM_FIFO                     = SIMD_INTERIM_WIDTH / OBUF_AXI_DATA_WIDTH,

    parameter integer  RF_ADDR_WIDTH                = 3,
    parameter integer  SRC_ADDR_WIDTH               = 4,

    parameter integer  OP_WIDTH                     = 3,
    parameter integer  FN_WIDTH                     = 3,
    parameter integer  IMM_WIDTH                    = 16,
    parameter integer  ADDR_STRIDE_W                = 32,

    parameter integer  OBUF_ADDR_WIDTH              = 12,

    parameter integer  AXI_ADDR_WIDTH               = 42,
    parameter integer  AXI_ID_WIDTH                 = 1,
    parameter integer  AXI_BURST_WIDTH              = 8,
    parameter integer  AXI_DATA_WIDTH               = SIMD_DATA_WIDTH,
    parameter integer  AXI_WSTRB_WIDTH              = AXI_DATA_WIDTH / 8
)
(
    input  wire                                         clk,
    input  wire                                         reset,

  // Handshake
    output wire                                         done,
    output wire  [ 3                    -1 : 0 ]        pu_ctrl_state,

    input  wire                                         pu_block_start,
    input  wire                                         pu_compute_start,
    output wire                                         pu_compute_ready,
    output wire                                         pu_compute_done,
    output wire                                         pu_write_done,

  // Buffer instruction write (to PE) interface
    input  wire                                         inst_wr_req,
    input  wire  [ INST_WIDTH           -1 : 0 ]        inst_wr_data,
    output wire                                         inst_wr_ready,

  // Load from Sys Array OBUF - ADDR
    output wire                                         ld_obuf_req,
    output wire  [ OBUF_ADDR_WIDTH      -1 : 0 ]        ld_obuf_addr,
    input  wire                                         ld_obuf_ready,

  // Load from Sys Array OBUF - DATA
    input  wire  [ OBUF_AXI_DATA_WIDTH  -1 : 0 ]        obuf_ld_stream_write_data,
    input  wire                                         obuf_ld_stream_write_req,

  // CL_wrapper -> DDR0 AXI4 interface
  // Master Interface Write Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        pu_ddr_awaddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        pu_ddr_awlen,
    output wire  [ 3                    -1 : 0 ]        pu_ddr_awsize,
    output wire  [ 2                    -1 : 0 ]        pu_ddr_awburst,
    output wire                                         pu_ddr_awvalid,
    input  wire                                         pu_ddr_awready,
  // Master Interface Write Data
    output wire  [ AXI_DATA_WIDTH       -1 : 0 ]        pu_ddr_wdata,
    output wire  [ AXI_WSTRB_WIDTH      -1 : 0 ]        pu_ddr_wstrb,
    output wire                                         pu_ddr_wlast,
    output wire                                         pu_ddr_wvalid,
    input  wire                                         pu_ddr_wready,
  // Master Interface Write Response
    input  wire  [ 2                    -1 : 0 ]        pu_ddr_bresp,
    input  wire                                         pu_ddr_bvalid,
    output wire                                         pu_ddr_bready,
  // Master Interface Read Address
    output wire  [ AXI_ID_WIDTH         -1 : 0 ]        pu_ddr_arid,
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        pu_ddr_araddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        pu_ddr_arlen,
    output wire  [ 3                    -1 : 0 ]        pu_ddr_arsize,
    output wire  [ 2                    -1 : 0 ]        pu_ddr_arburst,
    output wire                                         pu_ddr_arvalid,
    input  wire                                         pu_ddr_arready,
  // Master Interface Read Data
    input  wire  [ AXI_ID_WIDTH         -1 : 0 ]        pu_ddr_rid,
    input  wire  [ AXI_DATA_WIDTH       -1 : 0 ]        pu_ddr_rdata,
    input  wire  [ 2                    -1 : 0 ]        pu_ddr_rresp,
    input  wire                                         pu_ddr_rlast,
    input  wire                                         pu_ddr_rvalid,
    output wire                                         pu_ddr_rready,

    output wire  [ INST_WIDTH           -1 : 0 ]        obuf_ld_stream_read_count,
    output wire  [ INST_WIDTH           -1 : 0 ]        obuf_ld_stream_write_count,
    output wire  [ INST_WIDTH           -1 : 0 ]        ddr_st_stream_read_count,
    output wire  [ INST_WIDTH           -1 : 0 ]        ddr_st_stream_write_count,
    output wire  [ INST_WIDTH           -1 : 0 ]        ld0_stream_counts,
    output wire  [ INST_WIDTH           -1 : 0 ]        ld1_stream_counts,
    output wire  [ INST_WIDTH           -1 : 0 ]        axi_wr_fifo_counts

  );

//==============================================================================
// Wires and Regs
//==============================================================================
    wire                                        obuf_ld_stream_read_req;
    wire                                        obuf_ld_stream_read_ready;
    wire                                        ddr_ld0_stream_read_req;
    wire                                        ddr_ld0_stream_read_ready;
    wire                                        ddr_ld1_stream_read_req;
    wire                                        ddr_ld1_stream_read_ready;
    wire                                        ddr_st_stream_write_req;
    wire                                        ddr_st_stream_write_ready;

    wire                                        ld_obuf_done;

    wire                                        ddr_st_done;

    wire                                        ld_data_ready;
    wire                                        ld_data_valid;

    wire                                        cfg_loop_iter_v;
    wire [ IMM_WIDTH            -1 : 0 ]        cfg_loop_iter;
    wire [ 3                    -1 : 0 ]        cfg_loop_iter_type;

    wire                                        cfg_loop_stride_v;
    wire [ ADDR_STRIDE_W        -1 : 0 ]        cfg_loop_stride;
    wire [ 3                    -1 : 0 ]        cfg_loop_stride_type;

    wire                                        cfg_mem_req_v;
    wire [ 2                    -1 : 0 ]        cfg_mem_req_type;

    wire                                        alu_fn_valid;
    wire [ 3                    -1 : 0 ]        alu_fn;
    wire [ 4                    -1 : 0 ]        alu_in0_addr;
    wire                                        alu_in1_src;
    wire [ 4                    -1 : 0 ]        alu_in1_addr;
    wire [ IMM_WIDTH            -1 : 0 ]        alu_imm;
    wire [ 4                    -1 : 0 ]        alu_out_addr;

    wire                                        obuf_ld_stream_write_ready;
    wire                                        ddr_ld0_stream_write_req;
    wire                                        ddr_ld0_stream_write_ready;
    wire [ AXI_DATA_WIDTH       -1 : 0 ]        ddr_ld0_stream_write_data;
    wire                                        ddr_ld1_stream_write_req;
    wire                                        ddr_ld1_stream_write_ready;
    wire [ AXI_DATA_WIDTH       -1 : 0 ]        ddr_ld1_stream_write_data;
    wire                                        ddr_st_stream_read_req;
    wire [ AXI_DATA_WIDTH       -1 : 0 ]        ddr_st_stream_read_data;
    wire                                        ddr_st_stream_read_ready;

    wire                                        ld_obuf_start;
    wire [ OBUF_ADDR_WIDTH      -1 : 0 ]        ld_obuf_base_addr;

    wire                                        pu_ddr_start;
    wire                                        pu_ddr_done;
    wire                                        pu_ddr_data_valid;

    wire [ AXI_ADDR_WIDTH       -1 : 0 ]        pu_ddr_st_base_addr;
    wire [ AXI_ADDR_WIDTH       -1 : 0 ]        pu_ddr_ld0_base_addr;
    wire [ AXI_ADDR_WIDTH       -1 : 0 ]        pu_ddr_ld1_base_addr;

    wire                                        pu_ld0_read_req;
    wire                                        pu_ld1_read_req;

    wire                                        ddr_ld0_req;
    wire                                        ddr_ld0_ready;
    wire [ SIMD_DATA_WIDTH      -1 : 0 ]        ddr_ld0_data;
    wire                                        ddr_ld1_req;
    wire                                        ddr_ld1_ready;
    wire [ SIMD_DATA_WIDTH      -1 : 0 ]        ddr_ld1_data;
    wire                                        ddr_st_req;
    wire                                        ddr_st_ready;
    wire [ SIMD_DATA_WIDTH      -1 : 0 ]        ddr_st_data;
//==============================================================================

//==============================================================================
// Assigns
//==============================================================================
    assign pu_write_done = ddr_st_done;
//==============================================================================

//==============================================================================
// Gen PU controller
//==============================================================================
  gen_pu_ctrl #(
    .ADDR_WIDTH                     ( AXI_ADDR_WIDTH                 )
  )
  u_ctrl (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input

    .pu_block_start                 ( pu_block_start                 ), //input
    .pu_compute_start               ( pu_compute_start               ), //input
    .pu_compute_ready               ( pu_compute_ready               ), //output

    .tag_ld0_base_addr              ( pu_ddr_ld0_base_addr           ), //output
    .tag_ld1_base_addr              ( pu_ddr_ld1_base_addr           ), //output
    .tag_st_base_addr               ( pu_ddr_st_base_addr            ), //output

    .pu_ctrl_state                  ( pu_ctrl_state                  ), //output
    .done                           ( done                           ), //output

    .inst_wr_req                    ( inst_wr_req                    ), //input
    .inst_wr_data                   ( inst_wr_data                   ), //input
    .inst_wr_ready                  ( inst_wr_ready                  ), //output

    .cfg_loop_iter_v                ( cfg_loop_iter_v                ), //output
    .cfg_loop_iter                  ( cfg_loop_iter                  ), //output
    .cfg_loop_iter_type             ( cfg_loop_iter_type             ), //output

    .cfg_mem_req_v                  ( cfg_mem_req_v                  ), //output
    .cfg_mem_req_type               ( cfg_mem_req_type               ), //output

    .cfg_loop_stride_v              ( cfg_loop_stride_v              ), //output
    .cfg_loop_stride                ( cfg_loop_stride                ), //output
    .cfg_loop_stride_type           ( cfg_loop_stride_type           ), //output

    .obuf_ld_stream_read_req        ( obuf_ld_stream_read_req        ), //output
    .obuf_ld_stream_read_ready      ( obuf_ld_stream_read_ready      ), //input
    .ddr_ld0_stream_read_req        ( ddr_ld0_stream_read_req        ), //output
    .ddr_ld0_stream_read_ready      ( ddr_ld0_stream_read_ready      ), //input
    .ddr_ld1_stream_read_req        ( ddr_ld1_stream_read_req        ), //output
    .ddr_ld1_stream_read_ready      ( ddr_ld1_stream_read_ready      ), //input
    .ddr_st_stream_write_req        ( ddr_st_stream_write_req        ), //output
    .ddr_st_stream_write_ready      ( ddr_st_stream_write_ready      ), //input
    .ddr_st_done                    ( ddr_st_done                    ), //input

    .alu_fn_valid                   ( alu_fn_valid                   ), //output
    .alu_in0_addr                   ( alu_in0_addr                   ), //output
    .alu_in1_src                    ( alu_in1_src                    ), //output
    .alu_in1_addr                   ( alu_in1_addr                   ), //output
    .alu_imm                        ( alu_imm                        ), //output
    .alu_out_addr                   ( alu_out_addr                   ), //output
    .alu_fn                         ( alu_fn                         )  //output
  );
//==============================================================================

//==============================================================================
// LD Obuf stream
//==============================================================================
    assign ld_obuf_start = pu_compute_start;
    assign ld_obuf_base_addr = 0;
    assign pu_compute_done = ld_obuf_done;

    wire [ OBUF_ADDR_WIDTH      -1 : 0 ]        ld_obuf_stride;
    assign ld_obuf_stride = cfg_loop_stride;

  pu_ld_obuf_wrapper #(
    .NUM_FIFO                       ( NUM_FIFO                       ),
    .SIMD_INTERIM_WIDTH             ( SIMD_INTERIM_WIDTH             ),
    .OBUF_AXI_DATA_WIDTH            ( OBUF_AXI_DATA_WIDTH            ),
    .ADDR_WIDTH                     ( OBUF_ADDR_WIDTH                )
  )
  u_ld_obuf_wrapper (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .start                          ( ld_obuf_start                  ), //input
    .done                           ( ld_obuf_done                   ), //output
    .base_addr                      ( ld_obuf_base_addr              ), //input
    .cfg_loop_iter_v                ( cfg_loop_iter_v                ), //output
    .cfg_loop_iter                  ( cfg_loop_iter                  ), //output
    .cfg_loop_iter_type             ( cfg_loop_iter_type             ), //output
    .cfg_loop_stride_v              ( cfg_loop_stride_v              ), //output
    .cfg_loop_stride                ( ld_obuf_stride                 ), //output
    .cfg_loop_stride_type           ( cfg_loop_stride_type           ), //output
    .mem_req                        ( ld_obuf_req                    ), //output
    .mem_ready                      ( ld_obuf_ready                  ), //output
    .mem_addr                       ( ld_obuf_addr                   ), //output
    .obuf_ld_stream_write_ready     ( obuf_ld_stream_write_ready     )  //input
  );
//==============================================================================

//==============================================================================
// LD/ST DDR stream
//==============================================================================
    assign pu_ddr_start = pu_compute_start;

  ldst_ddr_wrapper #(
    .SIMD_DATA_WIDTH                ( SIMD_DATA_WIDTH                ),
    .ADDR_STRIDE_W                  ( ADDR_STRIDE_W                  ),
    .AXI_DATA_WIDTH                 ( AXI_DATA_WIDTH                 ),
    .AXI_ADDR_WIDTH                 ( AXI_ADDR_WIDTH                 )
  )
  u_ldst_ddr_wrapper (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .start                          ( pu_ddr_start                   ), //input
    .pu_block_start                 ( pu_block_start                 ), //input
    .done                           ( ddr_st_done                    ), //output
    .st_base_addr                   ( pu_ddr_st_base_addr            ), //input
    .ld0_base_addr                  ( pu_ddr_ld0_base_addr           ), //input
    .ld1_base_addr                  ( pu_ddr_ld1_base_addr           ), //input
    .cfg_loop_stride_v              ( cfg_loop_stride_v              ), //input
    .cfg_loop_stride                ( cfg_loop_stride                ), //input
    .cfg_loop_stride_type           ( cfg_loop_stride_type           ), //input
    .cfg_loop_iter_v                ( cfg_loop_iter_v                ), //input
    .cfg_loop_iter                  ( cfg_loop_iter                  ), //input
    .cfg_loop_iter_type             ( cfg_loop_iter_type             ), //input

    .cfg_mem_req_v                  ( cfg_mem_req_v                  ), //input
    .cfg_mem_req_type               ( cfg_mem_req_type               ), //input

    .ddr_st_stream_read_req         ( ddr_st_stream_read_req         ), //output
    .ddr_st_stream_read_ready       ( ddr_st_stream_read_ready       ), //input
    .ddr_st_stream_read_data        ( ddr_st_stream_read_data        ), //input

    .ddr_ld0_stream_write_req       ( ddr_ld0_stream_write_req       ), //output
    .ddr_ld0_stream_write_data      ( ddr_ld0_stream_write_data      ), //input
    .ddr_ld0_stream_write_ready     ( ddr_ld0_stream_write_ready     ), //input

    .ddr_ld1_stream_write_req       ( ddr_ld1_stream_write_req       ), //output
    .ddr_ld1_stream_write_data      ( ddr_ld1_stream_write_data      ), //input
    .ddr_ld1_stream_write_ready     ( ddr_ld1_stream_write_ready     ), //input

    .pu_ddr_awaddr                  ( pu_ddr_awaddr                  ), //output
    .pu_ddr_awlen                   ( pu_ddr_awlen                   ), //output
    .pu_ddr_awsize                  ( pu_ddr_awsize                  ), //output
    .pu_ddr_awburst                 ( pu_ddr_awburst                 ), //output
    .pu_ddr_awvalid                 ( pu_ddr_awvalid                 ), //output
    .pu_ddr_awready                 ( pu_ddr_awready                 ), //input
    .pu_ddr_wdata                   ( pu_ddr_wdata                   ), //output
    .pu_ddr_wstrb                   ( pu_ddr_wstrb                   ), //output
    .pu_ddr_wlast                   ( pu_ddr_wlast                   ), //output
    .pu_ddr_wvalid                  ( pu_ddr_wvalid                  ), //output
    .pu_ddr_wready                  ( pu_ddr_wready                  ), //input
    .pu_ddr_bresp                   ( pu_ddr_bresp                   ), //input
    .pu_ddr_bvalid                  ( pu_ddr_bvalid                  ), //input
    .pu_ddr_bready                  ( pu_ddr_bready                  ), //output
    .pu_ddr_arid                    ( pu_ddr_arid                    ), //output
    .pu_ddr_araddr                  ( pu_ddr_araddr                  ), //output
    .pu_ddr_arlen                   ( pu_ddr_arlen                   ), //output
    .pu_ddr_arsize                  ( pu_ddr_arsize                  ), //output
    .pu_ddr_arburst                 ( pu_ddr_arburst                 ), //output
    .pu_ddr_arvalid                 ( pu_ddr_arvalid                 ), //output
    .pu_ddr_arready                 ( pu_ddr_arready                 ), //input
    .pu_ddr_rid                     ( pu_ddr_rid                     ), //input
    .pu_ddr_rdata                   ( pu_ddr_rdata                   ), //input
    .pu_ddr_rresp                   ( pu_ddr_rresp                   ), //input
    .pu_ddr_rlast                   ( pu_ddr_rlast                   ), //input
    .pu_ddr_rvalid                  ( pu_ddr_rvalid                  ), //input
    .pu_ddr_rready                  ( pu_ddr_rready                  )  //output
  );
//==============================================================================

//==============================================================================
// SIMD core - RF + ALU
//==============================================================================
    // assign ddr_st_stream_write_count = {u_ldst_ddr_wrapper.u_axi_mm_master.awr_req_buf.fifo_count, u_ldst_ddr_wrapper.u_axi_mm_master.wdata_req_buf.fifo_count};
    //wire [15:0] ddr_wr_awr_req_buf = u_ldst_ddr_wrapper.u_axi_mm_master.awr_req_buf.fifo_count;
    //wire [15:0] ddr_wr_wr_req_buf = u_ldst_ddr_wrapper.u_axi_mm_master.wdata_req_buf.fifo_count;
    //assign axi_wr_fifo_counts = {ddr_wr_awr_req_buf, ddr_wr_wr_req_buf};
  simd_pu_core #(
    .DATA_WIDTH                     ( DATA_WIDTH                     ),
    .OBUF_AXI_DATA_WIDTH            ( OBUF_AXI_DATA_WIDTH            ),
    .AXI_DATA_WIDTH                 ( AXI_DATA_WIDTH                 ),
    .ACC_DATA_WIDTH                 ( ACC_DATA_WIDTH                 ),
    .SIMD_LANES                     ( SIMD_LANES                     ),
    .SIMD_DATA_WIDTH                ( SIMD_DATA_WIDTH                ),
    .SRC_ADDR_WIDTH                 ( SRC_ADDR_WIDTH                 ),
    .OP_WIDTH                       ( OP_WIDTH                       ),
    .FN_WIDTH                       ( FN_WIDTH                       ),
    .IMM_WIDTH                      ( IMM_WIDTH                      )
  ) simd_core (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input

    .obuf_ld_stream_write_req       ( obuf_ld_stream_write_req       ), //input
    .obuf_ld_stream_write_ready     ( obuf_ld_stream_write_ready     ), //output
    .obuf_ld_stream_write_data      ( obuf_ld_stream_write_data      ), //output

    // DEBUG
    .obuf_ld_stream_read_count      ( obuf_ld_stream_read_count      ), //output
    .obuf_ld_stream_write_count     ( obuf_ld_stream_write_count     ), //output
    .ddr_st_stream_read_count       ( ddr_st_stream_read_count       ), //output
    .ddr_st_stream_write_count      ( ddr_st_stream_write_count      ), //output
    .ld0_stream_counts              ( ld0_stream_counts              ), //output
    .ld1_stream_counts              ( ld1_stream_counts              ), //output
    // DEBUG


    .obuf_ld_stream_read_req        ( obuf_ld_stream_read_req        ), //input
    .obuf_ld_stream_read_ready      ( obuf_ld_stream_read_ready      ), //output
    .ddr_ld0_stream_read_req        ( ddr_ld0_stream_read_req        ), //input
    .ddr_ld0_stream_read_ready      ( ddr_ld0_stream_read_ready      ), //output
    .ddr_ld1_stream_read_req        ( ddr_ld1_stream_read_req        ), //input
    .ddr_ld1_stream_read_ready      ( ddr_ld1_stream_read_ready      ), //output
    .ddr_st_stream_write_req        ( ddr_st_stream_write_req        ), //input
    .ddr_st_stream_write_ready      ( ddr_st_stream_write_ready      ), //output

    .ddr_st_stream_read_req         ( ddr_st_stream_read_req         ), //input
    .ddr_st_stream_read_data        ( ddr_st_stream_read_data        ), //output
    .ddr_st_stream_read_ready       ( ddr_st_stream_read_ready       ), //output

    .ddr_ld0_stream_write_req       ( ddr_ld0_stream_write_req       ), //input
    .ddr_ld0_stream_write_data      ( ddr_ld0_stream_write_data      ), //output
    .ddr_ld0_stream_write_ready     ( ddr_ld0_stream_write_ready     ), //output

    .ddr_ld1_stream_write_req       ( ddr_ld1_stream_write_req       ), //input
    .ddr_ld1_stream_write_data      ( ddr_ld1_stream_write_data      ), //output
    .ddr_ld1_stream_write_ready     ( ddr_ld1_stream_write_ready     ), //output

    .alu_fn_valid                   ( alu_fn_valid                   ), //input
    .alu_fn                         ( alu_fn                         ), //input
    .alu_imm                        ( alu_imm                        ), //input
    .alu_in0_addr                   ( alu_in0_addr                   ), //input
    .alu_in1_src                    ( alu_in1_src                    ), //input
    .alu_in1_addr                   ( alu_in1_addr                   ), //input
    .alu_out_addr                   ( alu_out_addr                   )  //input
  );
//==============================================================================

//==============================================================================
// VCD
//==============================================================================
  `ifdef COCOTB_TOPLEVEL_gen_pu
  initial begin
    $dumpfile("gen_pu.vcd");
    $dumpvars(0, gen_pu);
  end
  `endif
//==============================================================================

endmodule
`timescale 1ns / 1ps
module simd_pu_core #(
  // Instruction width for PU controller
    parameter integer  INST_WIDTH                   = 32,
  // Data width
    parameter integer  DATA_WIDTH                   = 16,
    parameter integer  ACC_DATA_WIDTH               = 32,
    parameter integer  SIMD_LANES                   = 1,
    parameter integer  SIMD_DATA_WIDTH              = SIMD_LANES * DATA_WIDTH,
    parameter integer  SIMD_INTERIM_WIDTH           = SIMD_LANES * ACC_DATA_WIDTH,
    parameter integer  OBUF_AXI_DATA_WIDTH          = 256,

    parameter integer  AXI_DATA_WIDTH               = 64,

    parameter integer  SRC_ADDR_WIDTH               = 4,
    parameter integer  RF_ADDR_WIDTH                = SRC_ADDR_WIDTH-1,

    parameter integer  OP_WIDTH                     = 3,
    parameter integer  FN_WIDTH                     = 3,
    parameter integer  IMM_WIDTH                    = 16
)
(
    input  wire                                         clk,
    input  wire                                         reset,

    input  wire                                         alu_fn_valid,
    input  wire  [ FN_WIDTH             -1 : 0 ]        alu_fn,
    input  wire  [ IMM_WIDTH            -1 : 0 ]        alu_imm,

    input  wire  [ SRC_ADDR_WIDTH       -1 : 0 ]        alu_in0_addr,
    input  wire                                         alu_in1_src,
    input  wire  [ SRC_ADDR_WIDTH       -1 : 0 ]        alu_in1_addr,
    input  wire  [ SRC_ADDR_WIDTH       -1 : 0 ]        alu_out_addr,

  // From controller
    input  wire                                         obuf_ld_stream_read_req,
    output wire                                         obuf_ld_stream_read_ready,
    input  wire                                         ddr_ld0_stream_read_req,
    output wire                                         ddr_ld0_stream_read_ready,
    input  wire                                         ddr_ld1_stream_read_req,
    output wire                                         ddr_ld1_stream_read_ready,
    input  wire                                         ddr_st_stream_write_req,
    output wire                                         ddr_st_stream_write_ready,

  // From DDR
    input  wire                                         ddr_st_stream_read_req,
    output wire  [ AXI_DATA_WIDTH       -1 : 0 ]        ddr_st_stream_read_data,
    output wire                                         ddr_st_stream_read_ready,

    input  wire                                         ddr_ld0_stream_write_req,
    input  wire  [ AXI_DATA_WIDTH       -1 : 0 ]        ddr_ld0_stream_write_data,
    output wire                                         ddr_ld0_stream_write_ready,

    input  wire                                         ddr_ld1_stream_write_req,
    input  wire  [ AXI_DATA_WIDTH       -1 : 0 ]        ddr_ld1_stream_write_data,
    output wire                                         ddr_ld1_stream_write_ready,

  // From OBUF
    input  wire                                         obuf_ld_stream_write_req,
    input  wire  [ OBUF_AXI_DATA_WIDTH  -1 : 0 ]        obuf_ld_stream_write_data,
    output wire                                         obuf_ld_stream_write_ready,

    output wire  [ INST_WIDTH           -1 : 0 ]        obuf_ld_stream_read_count,
    output wire  [ INST_WIDTH           -1 : 0 ]        obuf_ld_stream_write_count,
    output wire  [ INST_WIDTH           -1 : 0 ]        ddr_st_stream_read_count,
    output wire  [ INST_WIDTH           -1 : 0 ]        ddr_st_stream_write_count,
    output wire  [ INST_WIDTH           -1 : 0 ]        ld0_stream_counts,
    output wire  [ INST_WIDTH           -1 : 0 ]        ld1_stream_counts
);

//==============================================================================
// Localparams
//==============================================================================
//==============================================================================

//==============================================================================
// Wires & Regs
//==============================================================================
    wire [ SIMD_INTERIM_WIDTH   -1 : 0 ]        alu_in0_data;
    wire [ SIMD_INTERIM_WIDTH   -1 : 0 ]        alu_in1_data;

    wire [ SIMD_INTERIM_WIDTH   -1 : 0 ]        obuf_ld_stream_read_data;

    wire [ SIMD_DATA_WIDTH      -1 : 0 ]        ddr_ld0_stream_read_data;
    wire                                        ld0_req_buf_write_ready;
    wire                                        ld0_req_buf_almost_full;
    wire                                        ld0_req_buf_almost_empty;
	 wire [15:0] ld0_req_buf_fifo_count;

    wire [ SIMD_DATA_WIDTH      -1 : 0 ]        ddr_ld1_stream_read_data;
    wire                                        ld1_req_buf_write_ready;
    wire                                        ld1_req_buf_almost_full;
    wire                                        ld1_req_buf_almost_empty;
	 wire [15:0] ld1_req_buf_fifo_count;

    wire                                        st_req_buf_almost_full;
    wire                                        st_req_buf_almost_empty;
	 wire [15:0] st_req_buf_fifo_count;
    wire [ SIMD_DATA_WIDTH      -1 : 0 ]        ddr_st_stream_write_data;

    wire [ FN_WIDTH             -1 : 0 ]        alu_fn_stage2;
    wire                                        alu_fn_valid_stage2;
    wire                                        alu_fn_valid_stage3;
    wire [ IMM_WIDTH            -1 : 0 ]        alu_imm_stage2;
    wire                                        alu_in1_src_stage2;
    wire [ SRC_ADDR_WIDTH       -1 : 0 ]        alu_in0_addr_stage2;
    wire [ SRC_ADDR_WIDTH       -1 : 0 ]        alu_in1_addr_stage2;

    wire                                        ld_req_buf_almost_full;
    wire                                        ld_req_buf_almost_empty;
	 wire [15:0] ld_req_buf_fifo_count;

    wire                                        alu_in0_req;
    wire                                        alu_in1_req;
    wire [ SIMD_INTERIM_WIDTH   -1 : 0 ]        alu_out;
    reg  [ SIMD_INTERIM_WIDTH   -1 : 0 ]        alu_out_fwd;

  // chaining consecutive ops
    wire                                        chain_rs0;
    wire                                        chain_rs1;
    wire                                        chain_rs0_stage2;
    wire                                        chain_rs1_stage2;

  // forwarding between ops
    wire                                        fwd_rs0;
    wire                                        fwd_rs1;
    wire                                        fwd_rs0_stage2;
    wire                                        fwd_rs1_stage2;

    wire [ SRC_ADDR_WIDTH       -1 : 0 ]        alu_out_addr_stage2;
    wire [ SRC_ADDR_WIDTH       -1 : 0 ]        alu_out_addr_stage3;

    genvar i;
//==============================================================================

//==============================================================================
// Chaining/Forwarding logic
//==============================================================================
    assign chain_rs0 = alu_fn_valid && alu_fn_valid_stage2 && (alu_in0_addr[2:0] == alu_out_addr_stage2[2:0]);
    assign chain_rs1 = alu_fn_valid && alu_fn_valid_stage2 && (alu_in1_addr[2:0] == alu_out_addr_stage2[2:0]);

    assign fwd_rs0 = (alu_fn_valid && alu_fn_valid_stage3 && (alu_in0_addr == alu_out_addr_stage3));
    assign fwd_rs1 = (alu_fn_valid && alu_fn_valid_stage3 && (alu_in1_addr == alu_out_addr_stage3));
//==============================================================================

//==============================================================================
// Registers
//==============================================================================
  register_sync_with_enable #(1) stage2_chain_rs0
  (clk, reset, 1'b1, chain_rs0, chain_rs0_stage2);

  register_sync_with_enable #(1) stage2_chain_rs1
  (clk, reset, 1'b1, chain_rs1, chain_rs1_stage2);

  register_sync_with_enable #(1) stage2_fwd_rs0
  (clk, reset, 1'b1, fwd_rs0, fwd_rs0_stage2);

  register_sync_with_enable #(1) stage2_fwd_rs1
  (clk, reset, 1'b1, fwd_rs1, fwd_rs1_stage2);

  register_sync_with_enable #(SRC_ADDR_WIDTH) stage2_alu_out_addr
  (clk, reset, 1'b1, alu_out_addr, alu_out_addr_stage2);
  register_sync_with_enable #(SRC_ADDR_WIDTH) stage3_alu_out_addr
  (clk, reset, 1'b1, alu_out_addr_stage2, alu_out_addr_stage3);
//==============================================================================

//==============================================================================
// Assigns
//==============================================================================
//==============================================================================

//==============================================================================
// PU OBUF LD FIFO
//==============================================================================
    assign obuf_ld_stream_write_ready = ~ld_req_buf_almost_full;
  fifo_asymmetric #(
    .WR_DATA_WIDTH                  ( OBUF_AXI_DATA_WIDTH            ),
    .RD_DATA_WIDTH                  ( SIMD_INTERIM_WIDTH             ),
    .WR_ADDR_WIDTH                  ( 3                              )
  ) ld_req_buf (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .s_write_req                    ( obuf_ld_stream_write_req       ), //input
    .s_write_data                   ( obuf_ld_stream_write_data      ), //input
    .s_write_ready                  (                                ), //output
    .s_read_req                     ( obuf_ld_stream_read_req        ), //input
    .s_read_ready                   ( obuf_ld_stream_read_ready      ), //output
    .s_read_data                    ( obuf_ld_stream_read_data       ), //output
    .almost_full                    ( ld_req_buf_almost_full         ), //output
    .almost_empty                   ( ld_req_buf_almost_empty        ), //output
	 .fifo_count							( ld_req_buf_fifo_count 			)
  );
//==============================================================================

//==============================================================================
// PU Store FIFO
//==============================================================================
    assign ddr_st_stream_write_ready = ~st_req_buf_almost_full;
generate
for (i=0; i<SIMD_LANES; i=i+1)
begin: gen_ddr_st_stream_write
    assign ddr_st_stream_write_data[i*DATA_WIDTH+:DATA_WIDTH] = alu_out[i*ACC_DATA_WIDTH+:DATA_WIDTH];
end
endgenerate

  fifo_asymmetric #(
    .WR_DATA_WIDTH                  ( SIMD_DATA_WIDTH                ),
    .RD_DATA_WIDTH                  ( AXI_DATA_WIDTH                 ),
    .WR_ADDR_WIDTH                  ( 3                              )
  ) st_req_buf (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .s_write_req                    ( ddr_st_stream_write_req        ), //input
    .s_write_data                   ( ddr_st_stream_write_data       ), //output
    .s_write_ready                  (                                ), //output
    .s_read_req                     ( ddr_st_stream_read_req         ), //input
    .s_read_ready                   ( ddr_st_stream_read_ready       ), //output
    .s_read_data                    ( ddr_st_stream_read_data        ), //input
    .almost_full                    ( st_req_buf_almost_full         ), //output
    .almost_empty                   ( st_req_buf_almost_empty        ), //output
	 .fifo_count							( st_req_buf_fifo_count 			)
  );
//==============================================================================

//==============================================================================
// PU LD0 FIFO
//==============================================================================
    assign ddr_ld0_stream_write_ready = ~ld0_req_buf_almost_full;
  fifo_asymmetric #(
    .RD_DATA_WIDTH                  ( SIMD_DATA_WIDTH                ),
    .WR_DATA_WIDTH                  ( AXI_DATA_WIDTH                 ),
    .RD_ADDR_WIDTH                  ( 6                              )
  ) ld0_req_buf (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .s_write_req                    ( ddr_ld0_stream_write_req       ), //input
    .s_write_data                   ( ddr_ld0_stream_write_data      ), //output
    .s_write_ready                  ( ld0_req_buf_write_ready        ), //output
    .s_read_req                     ( ddr_ld0_stream_read_req        ), //input
    .s_read_ready                   ( ddr_ld0_stream_read_ready      ), //output
    .s_read_data                    ( ddr_ld0_stream_read_data       ), //input
    .almost_full                    ( ld0_req_buf_almost_full        ), //output
    .almost_empty                   ( ld0_req_buf_almost_empty       ), //output
	 .fifo_count							( ld0_req_buf_fifo_count 			)
  );
`ifdef COCOTB_SIM
  integer ld0_total_writes;
  always @(posedge clk)
  begin
    if (reset)
      ld0_total_writes <= 0;
    else if (ddr_ld0_stream_write_req && ddr_ld0_stream_write_ready)
      ld0_total_writes <= ld0_total_writes + 1;
  end

  integer ld0_total_reads;
  always @(posedge clk)
  begin
    if (reset)
      ld0_total_reads <= 0;
    else if (ddr_ld0_stream_read_req && ddr_ld0_stream_read_ready)
      ld0_total_reads <= ld0_total_reads + 1;
  end
`endif // COCOTB_SIM
//==============================================================================

//==============================================================================
// PU LD1 FIFO
//==============================================================================
    assign ddr_ld1_stream_write_ready = ~ld1_req_buf_almost_full;
  fifo_asymmetric #(
    .RD_DATA_WIDTH                  ( SIMD_DATA_WIDTH                ),
    .WR_DATA_WIDTH                  ( AXI_DATA_WIDTH                 ),
    .RD_ADDR_WIDTH                  ( 6                              )
  ) ld1_req_buf (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .s_write_req                    ( ddr_ld1_stream_write_req       ), //input
    .s_write_data                   ( ddr_ld1_stream_write_data      ), //output
    .s_write_ready                  ( ld1_req_buf_write_ready        ), //output
    .s_read_req                     ( ddr_ld1_stream_read_req        ), //input
    .s_read_ready                   ( ddr_ld1_stream_read_ready      ), //output
    .s_read_data                    ( ddr_ld1_stream_read_data       ), //input
    .almost_full                    ( ld1_req_buf_almost_full        ), //output
    .almost_empty                   ( ld1_req_buf_almost_empty       ), //output
	 .fifo_count							( ld1_req_buf_fifo_count 			)
  );
`ifdef COCOTB_SIM
  integer ld1_total_writes;
  always @(posedge clk)
  begin
    if (reset)
      ld1_total_writes <= 0;
    else if (ddr_ld1_stream_write_req && ddr_ld1_stream_write_ready)
      ld1_total_writes <= ld1_total_writes + 1;
  end

  integer ld1_total_reads;
  always @(posedge clk)
  begin
    if (reset)
      ld1_total_reads <= 0;
    else if (ddr_ld1_stream_read_req && ddr_ld1_stream_read_ready)
      ld1_total_reads <= ld1_total_reads + 1;
  end
`endif // COCOTB_SIM
//==============================================================================

//==============================================================================
// delays
//==============================================================================
    register_sync_with_enable #(FN_WIDTH) alu_fn_delay_reg1
    (clk, reset, 1'b1, alu_fn, alu_fn_stage2);

    register_sync_with_enable #(1) alu_fn_valid_delay_reg1
    (clk, reset, 1'b1, alu_fn_valid, alu_fn_valid_stage2);
    register_sync_with_enable #(1) alu_fn_valid_delay_reg2
    (clk, reset, 1'b1, alu_fn_valid_stage2, alu_fn_valid_stage3);

    register_sync_with_enable #(IMM_WIDTH) alu_imm_delay_reg1
    (clk, reset, 1'b1, alu_imm, alu_imm_stage2);

    register_sync_with_enable #(1) alu_in1_src_delay_reg1
    (clk, reset, 1'b1, alu_in1_src, alu_in1_src_stage2);

    register_sync_with_enable #(SRC_ADDR_WIDTH) alu_in0_addr_delay_reg1
    (clk, reset, 1'b1, alu_in0_addr, alu_in0_addr_stage2);

    register_sync_with_enable #(SRC_ADDR_WIDTH) alu_in1_addr_delay_reg1
    (clk, reset, 1'b1, alu_in1_addr, alu_in1_addr_stage2);
//==============================================================================

//==============================================================================
// Register File
//==============================================================================
    assign alu_in0_req = 1'b1;
    assign alu_in1_req = 1'b1;
    wire                                        alu_wb_req;
    assign alu_wb_req = alu_fn_valid_stage3 && ~ddr_st_stream_write_req;

    wire [ RF_ADDR_WIDTH        -1 : 0 ]        regfile_in0_addr;
    wire [ RF_ADDR_WIDTH        -1 : 0 ]        regfile_in1_addr;
    wire [ RF_ADDR_WIDTH        -1 : 0 ]        regfile_out_addr;
    assign regfile_in0_addr = alu_in0_addr;
    assign regfile_in1_addr = alu_in1_addr;
    assign regfile_out_addr = alu_out_addr_stage3;
  reg_file #(
    .DATA_WIDTH                     ( SIMD_INTERIM_WIDTH             ),
    .ADDR_WIDTH                     ( RF_ADDR_WIDTH                  )
  ) c_regfile (
    .clk                            ( clk                            ), //input
    .rd_req_0                       ( alu_in0_req                    ), //input
    .rd_addr_0                      ( regfile_in0_addr               ), //input
    .rd_data_0                      ( alu_in0_data                   ), //output
    .rd_req_1                       ( alu_in1_req                    ), //input
    .rd_addr_1                      ( regfile_in1_addr               ), //input
    .rd_data_1                      ( alu_in1_data                   ), //output
    .wr_req_0                       ( alu_wb_req                     ), //input
    .wr_addr_0                      ( regfile_out_addr               ), //input
    .wr_data_0                      ( alu_out                        )  //input
    );
//==============================================================================

//==============================================================================
// PU ALU
//==============================================================================
    wire [ SIMD_INTERIM_WIDTH   -1 : 0 ]        alu_in0;
    wire [ SIMD_INTERIM_WIDTH   -1 : 0 ]        alu_in1;
    wire [ SIMD_INTERIM_WIDTH   -1 : 0 ]        ld_data_in0;
    wire [ RF_ADDR_WIDTH        -1 : 0 ]        ld_type_in0;
    wire [ SIMD_INTERIM_WIDTH   -1 : 0 ]        ld_data_in1;
    wire [ RF_ADDR_WIDTH        -1 : 0 ]        ld_type_in1;

    assign ld_type_in0 = alu_in0_addr_stage2;
    assign ld_type_in1 = alu_in1_addr_stage2;


generate
for (i=0; i<SIMD_LANES; i=i+1)
begin: gen_ld_data_in
    assign ld_data_in0[i*ACC_DATA_WIDTH+:ACC_DATA_WIDTH] = ld_type_in0 == 0 ?
                                                           obuf_ld_stream_read_data[i*ACC_DATA_WIDTH+:ACC_DATA_WIDTH] :
                                                           ld_type_in0 == 1 ? ddr_ld0_stream_read_data[i*DATA_WIDTH+:DATA_WIDTH] :
                                                                              ddr_ld1_stream_read_data[i*DATA_WIDTH+:DATA_WIDTH];
    assign ld_data_in1[i*ACC_DATA_WIDTH+:ACC_DATA_WIDTH] = ld_type_in1 == 0 ?
                                                           obuf_ld_stream_read_data[i*ACC_DATA_WIDTH+:ACC_DATA_WIDTH] :
                                                           ld_type_in1 == 1 ? ddr_ld0_stream_read_data[i*DATA_WIDTH+:DATA_WIDTH] :
                                                                              ddr_ld1_stream_read_data[i*DATA_WIDTH+:DATA_WIDTH];
end
endgenerate

    assign alu_in0 = alu_in0_addr_stage2[3] ? ld_data_in0 :
                     chain_rs0_stage2 ? alu_out :
                     fwd_rs0_stage2 ? alu_out_fwd : alu_in0_data;
    assign alu_in1 = alu_in1_addr_stage2[3] ? ld_data_in1 :
                     chain_rs1_stage2 ? alu_out :
                     fwd_rs1_stage2 ? alu_out_fwd : alu_in1_data;

generate
for (i=0; i<SIMD_LANES; i=i+1)
begin: ALU_INST
    wire [ ACC_DATA_WIDTH       -1 : 0 ]        local_alu_in0;
    wire [ DATA_WIDTH           -1 : 0 ]        local_alu_in1;
    wire [ ACC_DATA_WIDTH       -1 : 0 ]        local_alu_out;

    assign local_alu_in0 = alu_in0[i*ACC_DATA_WIDTH+ACC_DATA_WIDTH-1:i*ACC_DATA_WIDTH];
    assign local_alu_in1 = alu_in1[i*ACC_DATA_WIDTH+DATA_WIDTH-1:i*ACC_DATA_WIDTH];
    assign alu_out[i*ACC_DATA_WIDTH+:ACC_DATA_WIDTH] = local_alu_out;

  pu_alu #(
    .DATA_WIDTH                     ( DATA_WIDTH                     ),
    .ACC_DATA_WIDTH                 ( ACC_DATA_WIDTH                 ),
    .IMM_WIDTH                      ( IMM_WIDTH                      ),
    .FN_WIDTH                       ( FN_WIDTH                       )
  ) scalar_alu (
    .clk                            ( clk                            ), //input
    .fn_valid                       ( alu_fn_valid_stage2            ), //input
    .fn                             ( alu_fn_stage2                  ), //input
    .imm                            ( alu_imm_stage2                 ), //input
    .alu_in0                        ( local_alu_in0                  ), //input
    .alu_in1_src                    ( alu_in1_src_stage2             ), //input
    .alu_in1                        ( local_alu_in1                  ), //input
    .alu_out                        ( local_alu_out                  )  //output
  );
end
endgenerate

  always @(posedge clk)
    alu_out_fwd <= alu_out;
//==============================================================================

//==============================================================================
// DEBUG Counters
//==============================================================================
    reg  [ 16                   -1 : 0 ]        _obuf_ld_stream_read_count;
    reg  [ 16                   -1 : 0 ]        _obuf_ld_stream_write_count;
    reg  [ 16                   -1 : 0 ]        _ddr_st_stream_read_count;
    reg  [ 16                   -1 : 0 ]        _ddr_st_stream_write_count;

    reg  [ 16                   -1 : 0 ]        _ddr_ld0_stream_read_count;
    reg  [ 16                   -1 : 0 ]        _ddr_ld0_stream_write_count;
    reg  [ 16                   -1 : 0 ]        _ddr_ld1_stream_read_count;
    reg  [ 16                   -1 : 0 ]        _ddr_ld1_stream_write_count;

    wire [ 16                   -1 : 0 ]        _ld_req_buf_fifo_count;
    wire [ 16                   -1 : 0 ]        _st_req_buf_fifo_count;
    wire [ 16                   -1 : 0 ]        _ld0_req_buf_fifo_count;
    wire [ 16                   -1 : 0 ]        _ld1_req_buf_fifo_count;

always @(posedge clk)
begin
  if (reset) begin
    _obuf_ld_stream_read_count <= 0;
    _obuf_ld_stream_write_count <= 0;
    _ddr_st_stream_read_count <= 0;
    _ddr_st_stream_write_count <= 0;
    _ddr_ld0_stream_read_count <= 0;
    _ddr_ld0_stream_write_count <= 0;
    _ddr_ld1_stream_read_count <= 0;
    _ddr_ld1_stream_write_count <= 0;
  end else begin
    if (obuf_ld_stream_read_req)
      _obuf_ld_stream_read_count <= _obuf_ld_stream_read_count + 1'b1;
    if (obuf_ld_stream_write_req)
    _obuf_ld_stream_write_count <= _obuf_ld_stream_write_count + 1'b1;
    if (ddr_st_stream_read_req)
    _ddr_st_stream_read_count <= _ddr_st_stream_read_count + 1'b1;
    if (ddr_st_stream_write_req)
    _ddr_st_stream_write_count <= _ddr_st_stream_write_count + 1'b1;
    if (ddr_ld0_stream_read_req)
      _ddr_ld0_stream_read_count <= _ddr_ld0_stream_read_count + 1'b1;
    if (ddr_ld0_stream_write_req)
      _ddr_ld0_stream_write_count <= _ddr_ld0_stream_write_count + 1'b1;
    if (ddr_ld1_stream_read_req)
      _ddr_ld1_stream_read_count <= _ddr_ld1_stream_read_count + 1'b1;
    if (ddr_ld1_stream_write_req)
      _ddr_ld1_stream_write_count <= _ddr_ld1_stream_write_count + 1'b1;
  end
end

    assign _ld_req_buf_fifo_count = ld_req_buf_fifo_count;
    assign _st_req_buf_fifo_count = st_req_buf_fifo_count;
    assign _ld0_req_buf_fifo_count = ld0_req_buf_fifo_count;
    assign _ld1_req_buf_fifo_count = ld1_req_buf_fifo_count;



    assign obuf_ld_stream_read_count = {_obuf_ld_stream_read_count, _obuf_ld_stream_write_count};
    assign obuf_ld_stream_write_count = {_ddr_st_stream_read_count, _ddr_st_stream_write_count};
    assign ddr_st_stream_read_count = {_ld_req_buf_fifo_count, _st_req_buf_fifo_count};
    assign ddr_st_stream_write_count = {_ld1_req_buf_fifo_count, _ld0_req_buf_fifo_count};
    assign ld0_stream_counts = {_ddr_ld0_stream_read_count, _ddr_ld0_stream_write_count};
    assign ld1_stream_counts = {_ddr_ld1_stream_read_count, _ddr_ld1_stream_write_count};
//==============================================================================




//==============================================================================
// VCD
//==============================================================================
`ifdef COCOTB_TOPLEVEL_simd_pu_core
  initial begin
    $dumpfile("simd_pu_core.vcd");
    $dumpvars(0, simd_pu_core);
  end
`endif
//==============================================================================

endmodule
`timescale 1ns / 1ps
module pu_alu #(
    parameter integer  DATA_WIDTH                   = 16,
    parameter integer  ACC_DATA_WIDTH               = 32,
    parameter integer  IMM_WIDTH                    = 16,
    parameter integer  FN_WIDTH                     = 2
) (
    input  wire                                         clk,
    input  wire                                         fn_valid,
    input  wire  [ FN_WIDTH             -1 : 0 ]        fn,
    input  wire  [ IMM_WIDTH            -1 : 0 ]        imm,
    input  wire  [ ACC_DATA_WIDTH       -1 : 0 ]        alu_in0,
    input  wire                                         alu_in1_src,
    input  wire  [ DATA_WIDTH           -1 : 0 ]        alu_in1,
    output wire  [ ACC_DATA_WIDTH       -1 : 0 ]        alu_out
);

    reg  signed [ ACC_DATA_WIDTH           -1 : 0 ]        alu_out_d;
    reg  signed [ ACC_DATA_WIDTH           -1 : 0 ]        alu_out_q;

  // Instruction types
    localparam integer  FN_NOP                       = 0;
    localparam integer  FN_ADD                       = 1;
    localparam integer  FN_SUB                       = 2;
    localparam integer  FN_MUL                       = 3;
    localparam integer  FN_MVHI                      = 4;

    localparam integer  FN_MAX                       = 5;
    localparam integer  FN_MIN                       = 6;

    localparam integer  FN_RSHIFT                    = 7;

    wire signed [ DATA_WIDTH           -1 : 0 ]        _alu_in1;
    wire signed [ DATA_WIDTH           -1 : 0 ]        _alu_in0;

    wire signed[ ACC_DATA_WIDTH           -1 : 0 ]        add_out;
    wire signed[ ACC_DATA_WIDTH           -1 : 0 ]        sub_out;
    wire signed[ ACC_DATA_WIDTH           -1 : 0 ]        mul_out;
    wire signed[ ACC_DATA_WIDTH           -1 : 0 ]        max_out;
    wire signed[ ACC_DATA_WIDTH           -1 : 0 ]        min_out;
    wire signed[ ACC_DATA_WIDTH           -1 : 0 ]        rshift_out;
    wire signed[ ACC_DATA_WIDTH       -1 : 0 ]        _rshift_out;
    wire [ DATA_WIDTH           -1 : 0 ]        mvhi_out;
    wire                                        gt_out;

    wire [ 5                    -1 : 0 ]        shift_amount;

    assign _alu_in1 = alu_in1_src ? imm : alu_in1;
    assign _alu_in0 = alu_in0;
    assign add_out = _alu_in0 + _alu_in1;
    assign sub_out = _alu_in0 - _alu_in1;
    assign mul_out = _alu_in0 * _alu_in1;
    assign gt_out = _alu_in0 > _alu_in1;
    assign max_out = gt_out ? _alu_in0 : _alu_in1;
    assign min_out = ~gt_out ? _alu_in0 : _alu_in1;
    assign mvhi_out = {imm, 16'b0};

    assign shift_amount = _alu_in1;
    assign _rshift_out = $signed(alu_in0) >>> shift_amount;

    wire signed [ DATA_WIDTH           -1 : 0 ]        _max;
    wire signed [ DATA_WIDTH           -1 : 0 ]        _min;
    wire                                        overflow;
    wire                                        sign;

    assign overflow = (_rshift_out > _max) || (_rshift_out < _min);
    assign sign = $signed(alu_in0) < 0;

    assign _max = (1 << (DATA_WIDTH - 1)) - 1;
    assign _min = -(1 << (DATA_WIDTH - 1));

    assign rshift_out = overflow ? sign ? _min : _max : _rshift_out;

  always @(*)
  begin
    case (fn)
      FN_NOP: alu_out_d = alu_in0;
      FN_ADD: alu_out_d = add_out;
      FN_SUB: alu_out_d = sub_out;
      FN_MUL: alu_out_d = mul_out;
      FN_MVHI: alu_out_d = mvhi_out;
      FN_MAX: alu_out_d = max_out;
      FN_MIN: alu_out_d = min_out;
      FN_RSHIFT: alu_out_d = rshift_out;
      default: alu_out_d = 'd0;
    endcase
  end

  always @(posedge clk)
  begin
    if (fn_valid)
      alu_out_q <= alu_out_d;
  end

    assign alu_out = alu_out_q;

endmodule
`timescale 1ns / 1ps
module reg_file #(
    parameter integer  DATA_WIDTH                   = 32,
    parameter integer  ADDR_WIDTH                   = 4
) (
    input  wire                                         clk,
    input  wire                                         rd_req_0,
    input  wire  [ ADDR_WIDTH           -1 : 0 ]        rd_addr_0,
    output wire  [ DATA_WIDTH           -1 : 0 ]        rd_data_0,
    input  wire                                         rd_req_1,
    input  wire  [ ADDR_WIDTH           -1 : 0 ]        rd_addr_1,
    output wire  [ DATA_WIDTH           -1 : 0 ]        rd_data_1,
    input  wire                                         wr_req_0,
    input  wire  [ ADDR_WIDTH           -1 : 0 ]        wr_addr_0,
    input  wire  [ DATA_WIDTH           -1 : 0 ]        wr_data_0
);

////=========================================
//// Wires and Regs
////=========================================
//    (* ram_style = "distributed" *)
//    reg  [ DATA_WIDTH           -1 : 0 ] mem [0 : (1 << ADDR_WIDTH) - 1];
//    reg  [ DATA_WIDTH           -1 : 0 ] mem_copy [0 : (1 << ADDR_WIDTH) - 1];
//    reg  [ DATA_WIDTH           -1 : 0 ]        rd_data_0_q;
//    reg  [ DATA_WIDTH           -1 : 0 ]        rd_data_1_q;
////=========================================
//
//
//  always @(posedge clk)
//  begin
//    if (rd_req_0)
//      rd_data_0_q <= mem[rd_addr_0];
//  end
//    assign rd_data_0 = rd_data_0_q;
//
//  always @(posedge clk)
//  begin
//    if (rd_req_1)
//      rd_data_1_q <= mem_copy[rd_addr_1];
//  end
//    assign rd_data_1 = rd_data_1_q;
//
//  always @(posedge clk)
//  begin
//    if (wr_req_0)
//      mem[wr_addr_0] <= wr_data_0;
//      mem_copy[wr_addr_0] <= wr_data_0;
//  end
  wire reset;
  ram #(
    .ADDR_WIDTH                     ( ADDR_WIDTH),
    .DATA_WIDTH                     ( DATA_WIDTH)
  ) u_ram1(
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .s_write_addr                   ( wr_addr_0),
    .s_write_req                    ( wr_req_0),
    .s_write_data                   ( wr_data_0),
    .s_read_addr                    ( rd_addr_0),
    .s_read_req                     ( rd_req_0),
    .s_read_data                    ( rd_data_0)
  );

  ram #(
    .ADDR_WIDTH                     ( ADDR_WIDTH),
    .DATA_WIDTH                     ( DATA_WIDTH)
  ) u_ram2(
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .s_write_addr                   ( wr_addr_0),
    .s_write_req                    ( wr_req_0),
    .s_write_data                   ( wr_data_0),
    .s_read_addr                    ( rd_addr_1),
    .s_read_req                     ( rd_req_1),
    .s_read_data                    ( rd_data_1)
  );

endmodule
//
// Wrapper for memory
//
// Hardik Sharma
// (hsharma@gatech.edu)

`timescale 1ns/1ps
module ldst_ddr_wrapper #(
  // Internal Parameters
    parameter integer  MEM_ID                       = 0,
    parameter integer  STORE_ENABLED                = MEM_ID == 1 ? 1 : 0,
    parameter integer  MEM_REQ_W                    = 16,
    parameter integer  LOOP_ITER_W                  = 16,
    parameter integer  ADDR_STRIDE_W                = 16,
    parameter integer  LOOP_ID_W                    = 5,
    parameter integer  BUF_TYPE_W                   = 2,
    parameter integer  NUM_TAGS                     = 4,
    parameter integer  TAG_W                        = $clog2(NUM_TAGS),
    parameter integer  SIMD_DATA_WIDTH              = 256,

  // AXI
    parameter integer  AXI_ID_WIDTH                 = 1,
    parameter integer  AXI_ADDR_WIDTH               = 42,
    parameter integer  AXI_DATA_WIDTH               = 64,
    parameter integer  AXI_BURST_WIDTH              = 8,
    parameter integer  WSTRB_W                      = AXI_DATA_WIDTH/8
) (
    input  wire                                         clk,
    input  wire                                         reset,

    input  wire                                         pu_block_start,
    input  wire                                         start,
    output wire                                         done,

    input  wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        st_base_addr,
    input  wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        ld0_base_addr,
    input  wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        ld1_base_addr,

  // Programming
    input  wire                                         cfg_loop_stride_v,
    input  wire  [ ADDR_STRIDE_W        -1 : 0 ]        cfg_loop_stride,
    input  wire  [ 3                    -1 : 0 ]        cfg_loop_stride_type,

    input  wire  [ LOOP_ITER_W          -1 : 0 ]        cfg_loop_iter,
    input  wire                                         cfg_loop_iter_v,
    input  wire  [ 3                    -1 : 0 ]        cfg_loop_iter_type,

    input  wire                                         cfg_mem_req_v,
    input  wire  [ 2                    -1 : 0 ]        cfg_mem_req_type,

  // Master Interface Write Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        pu_ddr_awaddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        pu_ddr_awlen,
    output wire  [ 3                    -1 : 0 ]        pu_ddr_awsize,
    output wire  [ 2                    -1 : 0 ]        pu_ddr_awburst,
    output wire                                         pu_ddr_awvalid,
    input  wire                                         pu_ddr_awready,
  // Master Interface Write Data
    output wire  [ AXI_DATA_WIDTH       -1 : 0 ]        pu_ddr_wdata,
    output wire  [ WSTRB_W              -1 : 0 ]        pu_ddr_wstrb,
    output wire                                         pu_ddr_wlast,
    output wire                                         pu_ddr_wvalid,
    input  wire                                         pu_ddr_wready,
  // Master Interface Write Response
    input  wire  [ 2                    -1 : 0 ]        pu_ddr_bresp,
    input  wire                                         pu_ddr_bvalid,
    output wire                                         pu_ddr_bready,
  // Master Interface Read Address
    output wire  [ 1                    -1 : 0 ]        pu_ddr_arid,
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        pu_ddr_araddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        pu_ddr_arlen,
    output wire  [ 3                    -1 : 0 ]        pu_ddr_arsize,
    output wire  [ 2                    -1 : 0 ]        pu_ddr_arburst,
    output wire                                         pu_ddr_arvalid,
    input  wire                                         pu_ddr_arready,
  // Master Interface Read Data
    input  wire  [ 1                    -1 : 0 ]        pu_ddr_rid,
    input  wire  [ AXI_DATA_WIDTH       -1 : 0 ]        pu_ddr_rdata,
    input  wire  [ 2                    -1 : 0 ]        pu_ddr_rresp,
    input  wire                                         pu_ddr_rlast,
    input  wire                                         pu_ddr_rvalid,
    output wire                                         pu_ddr_rready,

  // LD0
    output wire                                         ddr_ld0_stream_write_req,
    input  wire                                         ddr_ld0_stream_write_ready,
    output wire  [ AXI_DATA_WIDTH       -1 : 0 ]        ddr_ld0_stream_write_data,

  // LD1
    output wire                                         ddr_ld1_stream_write_req,
    input  wire                                         ddr_ld1_stream_write_ready,
    output wire  [ AXI_DATA_WIDTH       -1 : 0 ]        ddr_ld1_stream_write_data,

  // Stores
    output wire                                         ddr_st_stream_read_req,
    input  wire                                         ddr_st_stream_read_ready,
    input  wire  [ AXI_DATA_WIDTH       -1 : 0 ]        ddr_st_stream_read_data

);

//==============================================================================
// Localparams
//==============================================================================
//==============================================================================

//==============================================================================
// Wires/Regs
//==============================================================================
    wire                                        st_done;
    wire [ AXI_DATA_WIDTH       -1 : 0 ]        ddr_ld0_data;
    wire [ AXI_DATA_WIDTH       -1 : 0 ]        ddr_ld1_data;
  // Loads
    wire                                        mem_write_req;
    wire                                        mem_write_ready;
    wire [ AXI_DATA_WIDTH       -1 : 0 ]        mem_write_data;
    wire [ AXI_ID_WIDTH         -1 : 0 ]        mem_write_id;

    wire                                        ld0_req_buf_almost_full;
    wire                                        ld0_req_buf_almost_empty;

    wire [ AXI_ID_WIDTH         -1 : 0 ]        ld_req_id;

    wire [ MEM_REQ_W            -1 : 0 ]        st_req_size;

    wire                                        st_stall;
    wire [ AXI_ADDR_WIDTH       -1 : 0 ]        st_addr;
    wire                                        st_addr_req;
    wire                                        st_addr_valid;
    wire [ ADDR_STRIDE_W        -1 : 0 ]        st_stride;
    wire                                        st_stride_v;
    wire                                        st_ready;
    reg  [ LOOP_ID_W            -1 : 0 ]        st_loop_id_counter;
    wire                                        st_loop_iter_v;
    wire [ LOOP_ITER_W          -1 : 0 ]        st_loop_iter;
    wire                                        st_loop_done;
    wire                                        st_loop_init;
    wire                                        st_loop_enter;
    wire                                        st_loop_exit;
    wire [ LOOP_ID_W            -1 : 0 ]        st_loop_index;
    wire                                        st_loop_index_valid;
    wire                                        st_loop_index_step;

    wire [ AXI_ADDR_WIDTH       -1 : 0 ]        ld_addr;
    wire                                        ld_addr_req;
    wire                                        ld_ready;
    wire                                        ld_done;
    wire [ MEM_REQ_W            -1 : 0 ]        ld_req_size;

    wire                                        ld0_stall;
    wire [ AXI_ADDR_WIDTH       -1 : 0 ]        ld0_addr;
    wire                                        ld0_addr_req;
    wire [ ADDR_STRIDE_W        -1 : 0 ]        ld0_stride;
    wire                                        ld0_stride_v;
    reg                                         ld0_required;
    wire                                        ld0_ready;
    reg  [ LOOP_ID_W            -1 : 0 ]        ld0_loop_id_counter;
    wire                                        ld0_loop_iter_v;
    wire [ LOOP_ITER_W          -1 : 0 ]        ld0_loop_iter;
    wire                                        ld0_loop_done;
    wire                                        ld0_loop_init;
    wire                                        ld0_loop_enter;
    wire                                        ld0_loop_exit;
    wire [ LOOP_ID_W            -1 : 0 ]        ld0_loop_index;
    wire                                        ld0_loop_index_valid;
    wire                                        ld0_loop_index_step;

    wire                                        ld1_stall;
    wire [ AXI_ADDR_WIDTH       -1 : 0 ]        ld1_addr;
    wire                                        ld1_addr_req;
    wire [ ADDR_STRIDE_W        -1 : 0 ]        ld1_stride;
    wire                                        ld1_stride_v;
    reg                                         ld1_required;
    wire                                        ld1_ready;
    reg  [ LOOP_ID_W            -1 : 0 ]        ld1_loop_id_counter;
    wire                                        ld1_loop_iter_v;
    wire [ LOOP_ITER_W          -1 : 0 ]        ld1_loop_iter;
    wire                                        ld1_loop_done;
    wire                                        ld1_loop_init;
    wire                                        ld1_loop_enter;
    wire                                        ld1_loop_exit;
    wire [ LOOP_ID_W            -1 : 0 ]        ld1_loop_index;
    wire                                        ld1_loop_index_valid;
    wire                                        ld1_loop_index_step;
//==============================================================================

//==============================================================================
// LD/ST required
//==============================================================================
  always @(posedge clk)
  begin
    if (reset)
      ld0_required <= 1'b0;
    else begin
      if (pu_block_start)
        ld0_required <= 1'b0;
      else if (cfg_mem_req_v && cfg_mem_req_type == 2)
        ld0_required <= 1'b1;
    end
  end

  always @(posedge clk)
  begin
    if (reset)
      ld1_required <= 1'b0;
    else begin
      if (pu_block_start)
        ld1_required <= 1'b0;
      else if (cfg_mem_req_v && cfg_mem_req_type == 3)
        ld1_required <= 1'b1;
    end
  end

    assign st_req_size = SIMD_DATA_WIDTH/AXI_DATA_WIDTH;
//==============================================================================

//==============================================================================
// Assigns
//==============================================================================
    assign st_stride_v  = cfg_loop_stride_v && (cfg_loop_stride_type == 1);
    assign ld0_stride_v = cfg_loop_stride_v && (cfg_loop_stride_type == 2);
    assign ld1_stride_v = cfg_loop_stride_v && (cfg_loop_stride_type == 3);

    assign st_stall  = ~st_ready;
    assign ld0_stall = ld0_required && ~ld0_ready;
    assign ld1_stall = ld1_required && ~ld1_ready;
    assign st_addr_req = st_addr_valid && ~st_stall;
//==============================================================================

//==============================================================================
// FSM for Loads
//==============================================================================
    reg                                         ld_addr_state_d;
    reg                                         ld_addr_state_q;
  always @(posedge clk)
  begin
    if (reset)
      ld_addr_state_q <= 1'b0;
    else
      ld_addr_state_q <= ld_addr_state_d;
  end
  always @(*)
  begin
    ld_addr_state_d = ld_addr_state_q;
    case (ld_addr_state_q)
      0: begin
        if (ld0_required && ld0_addr_req && ld_ready)
          ld_addr_state_d = 1'b1;
      end
      1: begin
        if (ld1_required && ld1_addr_req && ld_ready)
          ld_addr_state_d = 1'b0;
      end
    endcase
  end

    assign ld0_ready = ld_ready && ld_addr_state_q == 1'b0;
    assign ld1_ready = ld_ready && ld_addr_state_q == 1'b1;

    assign ld_req_size = SIMD_DATA_WIDTH / AXI_DATA_WIDTH;
    assign ld_addr = ld_addr_state_q == 1'b0 ? ld0_addr : ld1_addr;
    assign ld_addr_req = (ld_addr_state_q == 1'b0 ? ld0_addr_req && ld0_required : ld1_addr_req && ld1_required) && ld_ready;
    assign ld_req_id = ld_addr_state_q;

    assign ddr_ld0_stream_write_req = mem_write_id == 1'b0 && mem_write_req;
    assign ddr_ld0_stream_write_data = mem_write_data;

    assign ddr_ld1_stream_write_req = mem_write_id == 1'b1 && mem_write_req;
    assign ddr_ld1_stream_write_data = mem_write_data;

    // assign mem_write_ready = mem_write_id == 1'b0 ? ddr_ld0_stream_write_ready : ddr_ld1_stream_write_ready;
    assign mem_write_ready = ddr_ld0_stream_write_ready && ddr_ld1_stream_write_ready;
    // assign mem_write_ready = (ddr_ld0_stream_write_ready || ~ld0_required) &&
                             // (ddr_ld1_stream_write_ready || ~ld1_required);
//==============================================================================

//==============================================================================
// FSM for Stores
//==============================================================================
  reg [2-1:0] st_state_d;
  reg [2-1:0] st_state_q;
  reg [5-1:0] wait_cycles_d;
  reg [5-1:0] wait_cycles_q;
  localparam integer ST_IDLE = 0;
  localparam integer ST_BUSY = 1;
  localparam integer ST_WAIT = 2;
  localparam integer ST_DONE = 3;

  always @(posedge clk)
  begin
    if (reset) begin
      st_state_q <= ST_IDLE;
      wait_cycles_q <= 0;
    end else begin
      st_state_q <= st_state_d;
      wait_cycles_q <= wait_cycles_d;
    end
  end

  always @(*)
  begin
    st_state_d = st_state_q;
    wait_cycles_d = wait_cycles_q;
    case (st_state_q)
      ST_IDLE: begin
        if (start)
          st_state_d = ST_BUSY;
      end
      ST_BUSY: begin
        if (st_loop_done) begin
          st_state_d = ST_WAIT;
          wait_cycles_d = 4;
        end
      end
      ST_WAIT: begin
        if (wait_cycles_q != 0)
          wait_cycles_d = wait_cycles_d - 1'b1;
        else if (st_done)
          st_state_d = ST_DONE;
      end
      ST_DONE: begin
        st_state_d = ST_IDLE;
      end
    endcase
  end

  assign done = st_state_q == ST_DONE;
//==============================================================================

//==============================================================================
// Loop controller - ST
//==============================================================================
  always@(posedge clk)
  begin
    if (reset)
      st_loop_id_counter <= 'b0;
    else begin
      if (cfg_loop_iter_v && cfg_loop_iter_type == 1)
        st_loop_id_counter <= st_loop_id_counter + 1'b1;
      else if (start)
        st_loop_id_counter <= 'b0;
    end
  end

    assign st_loop_iter_v = cfg_loop_iter_v && cfg_loop_iter_type == 1;
    assign st_loop_iter = cfg_loop_iter;

  controller_fsm #(
    .LOOP_ID_W                      ( LOOP_ID_W                      ),
    .LOOP_ITER_W                    ( LOOP_ITER_W                    ),
    .IMEM_ADDR_W                    ( LOOP_ID_W                      )
  ) loop_ctrl_st (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .stall                          ( st_stall                       ), //input
    .cfg_loop_iter_v                ( st_loop_iter_v                 ), //input
    .cfg_loop_iter                  ( st_loop_iter                   ), //input
    .cfg_loop_iter_loop_id          ( st_loop_id_counter             ), //input
    .start                          ( start                          ), //input
    .done                           ( st_loop_done                   ), //output
    .loop_init                      ( st_loop_init                   ), //output
    .loop_enter                     ( st_loop_enter                  ), //output
    .loop_last_iter                 (                                ), //output
    .loop_exit                      ( st_loop_exit                   ), //output
    .loop_index                     ( st_loop_index                  ), //output
    .loop_index_valid               ( st_loop_index_valid            )  //output
  );
//==============================================================================

//==============================================================================
// Address generators - ST
//==============================================================================
    assign st_stride = cfg_loop_stride * SIMD_DATA_WIDTH / 8;
    assign st_loop_index_step = st_loop_index_valid && ~st_stall;
  mem_walker_stride #(
    .ADDR_WIDTH                     ( AXI_ADDR_WIDTH                 ),
    .ADDR_STRIDE_W                  ( ADDR_STRIDE_W                  ),
    .LOOP_ID_W                      ( LOOP_ID_W                      )
  ) mws_st (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .base_addr                      ( st_base_addr                   ), //input
    .loop_ctrl_done                 ( st_loop_done                   ), //input
    .loop_index                     ( st_loop_index                  ), //input
    .loop_index_valid               ( st_loop_index_step             ), //input
    .loop_init                      ( st_loop_init                   ), //input
    .loop_enter                     ( st_loop_enter                  ), //input
    .loop_exit                      ( st_loop_exit                   ), //input
    .cfg_addr_stride_v              ( st_stride_v                    ), //input
    .cfg_addr_stride                ( st_stride                      ), //input
    .addr_out                       ( st_addr                        ), //output
    .addr_out_valid                 ( st_addr_valid                  )  //output
  );
//==============================================================================

//==============================================================================
// Loop controller - LD0
//==============================================================================
  always@(posedge clk)
  begin
    if (reset)
      ld0_loop_id_counter <= 'b0;
    else begin
      if (cfg_loop_iter_v && cfg_loop_iter_type == 2)
        ld0_loop_id_counter <= ld0_loop_id_counter + 1'b1;
      else if (start)
        ld0_loop_id_counter <= 'b0;
    end
  end

    assign ld0_loop_iter_v = cfg_loop_iter_v && cfg_loop_iter_type == 2;
    assign ld0_loop_iter = cfg_loop_iter;

  controller_fsm #(
    .LOOP_ID_W                      ( LOOP_ID_W                      ),
    .LOOP_ITER_W                    ( LOOP_ITER_W                    ),
    .IMEM_ADDR_W                    ( LOOP_ID_W                      )
  ) loop_ctrl_ld0 (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .stall                          ( ld0_stall                      ), //input
    .cfg_loop_iter_v                ( ld0_loop_iter_v                ), //input
    .cfg_loop_iter                  ( ld0_loop_iter                  ), //input
    .cfg_loop_iter_loop_id          ( ld0_loop_id_counter            ), //input
    .start                          ( start                          ), //input
    .done                           ( ld0_loop_done                  ), //output
    .loop_init                      ( ld0_loop_init                  ), //output
    .loop_enter                     ( ld0_loop_enter                 ), //output
    .loop_last_iter                 (                                ), //output
    .loop_exit                      ( ld0_loop_exit                  ), //output
    .loop_index                     ( ld0_loop_index                 ), //output
    .loop_index_valid               ( ld0_loop_index_valid           )  //output
  );
//==============================================================================

//==============================================================================
// Address generators - LD0
//==============================================================================
    assign ld0_loop_index_step = ld0_loop_index_valid && ~ld0_stall;
    assign ld0_stride = cfg_loop_stride * SIMD_DATA_WIDTH / 8;
  mem_walker_stride #(
    .ADDR_WIDTH                     ( AXI_ADDR_WIDTH                 ),
    .ADDR_STRIDE_W                  ( ADDR_STRIDE_W                  ),
    .LOOP_ID_W                      ( LOOP_ID_W                      )
  ) mws_ld0 (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .base_addr                      ( ld0_base_addr                  ), //input
    .loop_ctrl_done                 ( ld0_loop_done                  ), //input
    .loop_index                     ( ld0_loop_index                 ), //input
    .loop_index_valid               ( ld0_loop_index_step            ), //input
    .loop_init                      ( ld0_loop_init                  ), //input
    .loop_enter                     ( ld0_loop_enter                 ), //input
    .loop_exit                      ( ld0_loop_exit                  ), //input
    .cfg_addr_stride_v              ( ld0_stride_v                   ), //input
    .cfg_addr_stride                ( ld0_stride                     ), //input
    .addr_out                       ( ld0_addr                       ), //output
    .addr_out_valid                 ( ld0_addr_req                   )  //output
  );
//==============================================================================

//==============================================================================
// Loop controller - LD1
//==============================================================================
  always@(posedge clk)
  begin
    if (reset)
      ld1_loop_id_counter <= 'b0;
    else begin
      if (cfg_loop_iter_v && cfg_loop_iter_type == 3)
        ld1_loop_id_counter <= ld1_loop_id_counter + 1'b1;
      else if (start)
        ld1_loop_id_counter <= 'b0;
    end
  end

    assign ld1_loop_iter_v = cfg_loop_iter_v && cfg_loop_iter_type == 3;
    assign ld1_loop_iter = cfg_loop_iter;

  controller_fsm #(
    .LOOP_ID_W                      ( LOOP_ID_W                      ),
    .LOOP_ITER_W                    ( LOOP_ITER_W                    ),
    .IMEM_ADDR_W                    ( LOOP_ID_W                      )
  ) loop_ctrl_ld1 (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .stall                          ( ld1_stall                      ), //input
    .cfg_loop_iter_v                ( ld1_loop_iter_v                ), //input
    .cfg_loop_iter                  ( ld1_loop_iter                  ), //input
    .cfg_loop_iter_loop_id          ( ld1_loop_id_counter            ), //input
    .start                          ( start                          ), //input
    .done                           ( ld1_loop_done                  ), //output
    .loop_init                      ( ld1_loop_init                  ), //output
    .loop_enter                     ( ld1_loop_enter                 ), //output
    .loop_last_iter                 (                                ), //output
    .loop_exit                      ( ld1_loop_exit                  ), //output
    .loop_index                     ( ld1_loop_index                 ), //output
    .loop_index_valid               ( ld1_loop_index_valid           )  //output
  );
//==============================================================================

//==============================================================================
// Address generators - LD1
//==============================================================================
    assign ld1_loop_index_step = ld1_loop_index_valid && ~ld1_stall;
    assign ld1_stride = cfg_loop_stride * SIMD_DATA_WIDTH / 8;
  mem_walker_stride #(
    .ADDR_WIDTH                     ( AXI_ADDR_WIDTH                 ),
    .ADDR_STRIDE_W                  ( ADDR_STRIDE_W                  ),
    .LOOP_ID_W                      ( LOOP_ID_W                      )
  ) mws_ld1 (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .base_addr                      ( ld1_base_addr                  ), //input
    .loop_ctrl_done                 ( ld1_loop_done                  ), //input
    .loop_index                     ( ld1_loop_index                 ), //input
    .loop_index_valid               ( ld1_loop_index_step            ), //input
    .loop_init                      ( ld1_loop_init                  ), //input
    .loop_enter                     ( ld1_loop_enter                 ), //input
    .loop_exit                      ( ld1_loop_exit                  ), //input
    .cfg_addr_stride_v              ( ld1_stride_v                   ), //input
    .cfg_addr_stride                ( ld1_stride                     ), //input
    .addr_out                       ( ld1_addr                       ), //output
    .addr_out_valid                 ( ld1_addr_req                   )  //output
  );
//==============================================================================

//==============================================================================
// AXI4 Memory Mapped interface
//==============================================================================
  wire [AXI_ID_WIDTH-1:0] st_addr_req_id;
  assign st_addr_req_id = 0;
  axi_master #(
    .TX_SIZE_WIDTH                  ( MEM_REQ_W                      ),
    .AXI_DATA_WIDTH                 ( AXI_DATA_WIDTH                 ),
    .AXI_ADDR_WIDTH                 ( AXI_ADDR_WIDTH                 ),
    .AXI_BURST_WIDTH                ( AXI_BURST_WIDTH                )
  ) u_axi_mm_master (
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .m_axi_awaddr                   ( pu_ddr_awaddr                  ),
    .m_axi_awlen                    ( pu_ddr_awlen                   ),
    .m_axi_awsize                   ( pu_ddr_awsize                  ),
    .m_axi_awburst                  ( pu_ddr_awburst                 ),
    .m_axi_awvalid                  ( pu_ddr_awvalid                 ),
    .m_axi_awready                  ( pu_ddr_awready                 ),
    .m_axi_wdata                    ( pu_ddr_wdata                   ),
    .m_axi_wstrb                    ( pu_ddr_wstrb                   ),
    .m_axi_wlast                    ( pu_ddr_wlast                   ),
    .m_axi_wvalid                   ( pu_ddr_wvalid                  ),
    .m_axi_wready                   ( pu_ddr_wready                  ),
    .m_axi_bresp                    ( pu_ddr_bresp                   ),
    .m_axi_bvalid                   ( pu_ddr_bvalid                  ),
    .m_axi_bready                   ( pu_ddr_bready                  ),
    .m_axi_arid                     ( pu_ddr_arid                    ),
    .m_axi_araddr                   ( pu_ddr_araddr                  ),
    .m_axi_arlen                    ( pu_ddr_arlen                   ),
    .m_axi_arsize                   ( pu_ddr_arsize                  ),
    .m_axi_arburst                  ( pu_ddr_arburst                 ),
    .m_axi_arvalid                  ( pu_ddr_arvalid                 ),
    .m_axi_arready                  ( pu_ddr_arready                 ),
    .m_axi_rid                      ( pu_ddr_rid                     ),
    .m_axi_rdata                    ( pu_ddr_rdata                   ),
    .m_axi_rresp                    ( pu_ddr_rresp                   ),
    .m_axi_rlast                    ( pu_ddr_rlast                   ),
    .m_axi_rvalid                   ( pu_ddr_rvalid                  ),
    .m_axi_rready                   ( pu_ddr_rready                  ),
    // Buffer
    .mem_write_id                   ( mem_write_id                   ),
    .mem_write_req                  ( mem_write_req                  ),
    .mem_write_data                 ( mem_write_data                 ),
    .mem_write_ready                ( mem_write_ready                ),
    .mem_read_req                   ( ddr_st_stream_read_req         ),
    .mem_read_data                  ( ddr_st_stream_read_data        ),
    .mem_read_ready                 ( ddr_st_stream_read_ready       ),
    // AXI RD Req
    .rd_req_id                      ( ld_req_id                      ),
    .rd_req                         ( ld_addr_req                    ),
    .rd_done                        ( ld_done                        ),
    .rd_ready                       ( ld_ready                       ),
    .rd_req_size                    ( ld_req_size                    ),
    .rd_addr                        ( ld_addr                        ),
    // AXI WR Req
    .wr_req_id                      ( st_addr_req_id                 ),
    .wr_req                         ( st_addr_req                    ),
    .wr_ready                       ( st_ready                       ),
    .wr_req_size                    ( st_req_size                    ),
    .wr_addr                        ( st_addr                        ),
    .wr_done                        ( st_done                        )
  );
//==============================================================================

//==============================================================================
// VCD
//==============================================================================
`ifdef COCOTB_TOPLEVEL_pu_ld_obuf_wrapper
initial begin
  $dumpfile("pu_ld_obuf_wrapper.vcd");
  $dumpvars(0, pu_ld_obuf_wrapper);
end
`endif
//==============================================================================
endmodule
//
// Register
//
// Hardik Sharma
// (hsharma@gatech.edu)

`timescale 1ns/1ps
module register_sync_with_enable #(
  parameter integer WIDTH                 = 8
) (
  input  wire                             clk,
  input  wire                             reset,
  input  wire                             enable,
  input  wire        [ WIDTH -1 : 0 ]     in,
  output wire        [ WIDTH -1 : 0 ]     out
);

  reg [ WIDTH -1 : 0 ] out_reg;

  always @(posedge clk)
  begin
    if (reset)
      out_reg <= 'b0;
    else if (enable)
      out_reg <= in;
  end

  assign out = out_reg;

endmodule
//
// IBUF
//
// Hardik Sharma
// (hsharma@gatech.edu)
`timescale 1ns/1ps
module ibuf #(
    parameter integer  TAG_W                        = 2,  // Log number of banks
    parameter integer  MEM_DATA_WIDTH               = 64,
    parameter integer  ARRAY_N                      = 1,
    parameter integer  DATA_WIDTH                   = 32,
    parameter integer  BUF_ADDR_WIDTH               = 10,

    parameter integer  GROUP_SIZE                   = MEM_DATA_WIDTH / DATA_WIDTH,
    parameter integer  GROUP_ID_W                   = GROUP_SIZE == 1 ? 0 : $clog2(GROUP_SIZE),
    parameter integer  BUF_ID_W                     = $clog2(ARRAY_N) - GROUP_ID_W,

    parameter integer  MEM_ADDR_WIDTH               = BUF_ADDR_WIDTH + BUF_ID_W,
    parameter integer  BUF_DATA_WIDTH               = ARRAY_N * DATA_WIDTH
)
(
    input  wire                                         clk,
    input  wire                                         reset,

    input  wire                                         mem_write_req,
    input  wire  [ MEM_ADDR_WIDTH       -1 : 0 ]        mem_write_addr,
    input  wire  [ MEM_DATA_WIDTH       -1 : 0 ]        mem_write_data,

    input  wire                                         buf_read_req,
    input  wire  [ BUF_ADDR_WIDTH       -1 : 0 ]        buf_read_addr,
    output wire  [ BUF_DATA_WIDTH       -1 : 0 ]        buf_read_data
  );

  genvar n;
  generate
    for (n=0; n<ARRAY_N; n=n+1)
    begin: LOOP_N

      localparam integer  LOCAL_ADDR_W                 = BUF_ADDR_WIDTH;
      localparam integer  LOCAL_BUF_ID                 = n/GROUP_SIZE;

      wire                                        local_buf_read_req;
      wire [ LOCAL_ADDR_W         -1 : 0 ]        local_buf_read_addr;
      wire [ DATA_WIDTH           -1 : 0 ]        local_buf_read_data;

      assign buf_read_data[(n)*DATA_WIDTH+:DATA_WIDTH] = local_buf_read_data;

      wire                                        buf_read_req_fwd;
      wire [ LOCAL_ADDR_W         -1 : 0 ]        buf_read_addr_fwd;
      register_sync #(1) read_req_fwd (clk, reset, local_buf_read_req, buf_read_req_fwd);
      register_sync #(LOCAL_ADDR_W) read_addr_fwd (clk, reset, local_buf_read_addr, buf_read_addr_fwd);

      if (n == 0) begin
        assign local_buf_read_req = buf_read_req;
        assign local_buf_read_addr = buf_read_addr;
      end
      else begin
        assign local_buf_read_req = LOOP_N[n-1].buf_read_req_fwd;
        assign local_buf_read_addr = LOOP_N[n-1].buf_read_addr_fwd;
      end

      wire [ BUF_ID_W             -1 : 0 ]        local_mem_write_buf_id;
      wire                                        local_mem_write_req;
      wire [ LOCAL_ADDR_W         -1 : 0 ]        local_mem_write_addr;
      wire [ DATA_WIDTH           -1 : 0 ]        local_mem_write_data;

      wire [ BUF_ID_W             -1 : 0 ]        buf_id;
      assign buf_id = LOCAL_BUF_ID;

      if (BUF_ID_W == 0) begin
        assign local_mem_write_addr = mem_write_addr;
        assign local_mem_write_req = mem_write_req;
        assign local_mem_write_data = mem_write_data[(n%GROUP_SIZE)*DATA_WIDTH+:DATA_WIDTH];
      end
      else begin
        assign {local_mem_write_addr, local_mem_write_buf_id} = mem_write_addr;
        assign local_mem_write_req = mem_write_req && local_mem_write_buf_id == buf_id;
        assign local_mem_write_data = mem_write_data[(n%GROUP_SIZE)*DATA_WIDTH+:DATA_WIDTH];
      end

      ram #(
        .ADDR_WIDTH                     ( LOCAL_ADDR_W                   ),
        .DATA_WIDTH                     ( DATA_WIDTH                     ),
        .OUTPUT_REG                     ( 1                              )
      ) u_ram (
        .clk                            ( clk                            ),
        .reset                          ( reset                          ),
        .s_write_addr                   ( local_mem_write_addr           ),
        .s_write_req                    ( local_mem_write_req            ),
        .s_write_data                   ( local_mem_write_data           ),
        .s_read_addr                    ( local_buf_read_addr            ),
        .s_read_req                     ( local_buf_read_req             ),
        .s_read_data                    ( local_buf_read_data            )
        );

    end
  endgenerate

//=============================================================
// VCD
//=============================================================
  `ifdef COCOTB_TOPLEVEL_buffer
    initial begin
      $dumpfile("buffer.vcd");
      $dumpvars(0, buffer);
    end
  `endif
//=============================================================
endmodule
//
// Wrapper for memory
//
// Hardik Sharma
// (hsharma@gatech.edu)

`timescale 1ns/1ps
module bbuf_mem_wrapper #(
  // Internal Parameters
    parameter integer  MEM_ID                       = 3,
    parameter integer  STORE_ENABLED                = 0,
    parameter integer  MEM_REQ_W                    = 16,
    parameter integer  ADDR_WIDTH                   = 8,
    parameter integer  DATA_WIDTH                   = 32,
    parameter integer  LOOP_ITER_W                  = 16,
    parameter integer  ADDR_STRIDE_W                = 32,
    parameter integer  LOOP_ID_W                    = 5,
    parameter integer  BUF_TYPE_W                   = 2,
    parameter integer  NUM_TAGS                     = 4,
    parameter integer  TAG_W                        = $clog2(NUM_TAGS),

  // AXI
    parameter integer  AXI_ID_WIDTH                 = 1,
    parameter integer  AXI_ADDR_WIDTH               = 42,
    parameter integer  AXI_DATA_WIDTH               = 64,
    parameter integer  AXI_BURST_WIDTH              = 8,
    parameter integer  WSTRB_W                      = AXI_DATA_WIDTH/8,

  // Buffer
    parameter integer  ARRAY_N                      = 4,
    parameter integer  ARRAY_M                      = 4,
    parameter integer  BUF_DATA_WIDTH               = DATA_WIDTH * ARRAY_M,
    parameter integer  BUF_ADDR_W                   = 16,
    parameter integer  MEM_ADDR_W                   = BUF_ADDR_W + $clog2(BUF_DATA_WIDTH / AXI_DATA_WIDTH),
    parameter integer  TAG_BUF_ADDR_W               = BUF_ADDR_W + TAG_W,
    parameter integer  TAG_MEM_ADDR_W               = MEM_ADDR_W + TAG_W
) (
    input  wire                                         clk,
    input  wire                                         reset,

    input  wire                                         tag_req,
    input  wire                                         tag_reuse,
    input  wire                                         tag_bias_prev_sw,
    input  wire                                         tag_ddr_pe_sw,
    output wire                                         tag_ready,
    output wire                                         tag_done,
    input  wire                                         compute_done,
    input  wire                                         block_done,
    input  wire  [ ADDR_WIDTH           -1 : 0 ]        tag_base_ld_addr,

    output wire                                         compute_ready,
    output wire                                         compute_bias_prev_sw,

  // Programming
    input  wire                                         cfg_loop_stride_v,
    input  wire  [ ADDR_STRIDE_W        -1 : 0 ]        cfg_loop_stride,
    input  wire  [ LOOP_ID_W            -1 : 0 ]        cfg_loop_stride_loop_id,
    input  wire  [ BUF_TYPE_W           -1 : 0 ]        cfg_loop_stride_id,
    input  wire  [ 2                    -1 : 0 ]        cfg_loop_stride_type,

    input  wire                                         cfg_loop_iter_v,
    input  wire  [ LOOP_ITER_W          -1 : 0 ]        cfg_loop_iter,
    input  wire  [ LOOP_ID_W            -1 : 0 ]        cfg_loop_iter_loop_id,

    input  wire                                         cfg_mem_req_v,
    input  wire  [ BUF_TYPE_W           -1 : 0 ]        cfg_mem_req_id,
    input  wire  [ MEM_REQ_W            -1 : 0 ]        cfg_mem_req_size,
    input  wire  [ LOOP_ID_W            -1 : 0 ]        cfg_mem_req_loop_id,
    input  wire  [ 2                    -1 : 0 ]        cfg_mem_req_type,

  // Systolic Array
    output wire  [ BUF_DATA_WIDTH       -1 : 0 ]        buf_read_data,
    input  wire                                         buf_read_req,
    input  wire  [ BUF_ADDR_W           -1 : 0 ]        buf_read_addr,

  // CL_wrapper -> DDR AXI4 interface
    // Master Interface Write Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        mws_awaddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        mws_awlen,
    output wire  [ 3                    -1 : 0 ]        mws_awsize,
    output wire  [ 2                    -1 : 0 ]        mws_awburst,
    output wire                                         mws_awvalid,
    input  wire                                         mws_awready,
    // Master Interface Write Data
    output wire  [ AXI_DATA_WIDTH       -1 : 0 ]        mws_wdata,
    output wire  [ WSTRB_W              -1 : 0 ]        mws_wstrb,
    output wire                                         mws_wlast,
    output wire                                         mws_wvalid,
    input  wire                                         mws_wready,
    // Master Interface Write Response
    input  wire  [ 2                    -1 : 0 ]        mws_bresp,
    input  wire                                         mws_bvalid,
    output wire                                         mws_bready,
    // Master Interface Read Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        mws_araddr,
    output wire  [ AXI_ID_WIDTH         -1 : 0 ]        mws_arid,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        mws_arlen,
    output wire  [ 3                    -1 : 0 ]        mws_arsize,
    output wire  [ 2                    -1 : 0 ]        mws_arburst,
    output wire                                         mws_arvalid,
    input  wire                                         mws_arready,
    // Master Interface Read Data
    input  wire  [ AXI_DATA_WIDTH       -1 : 0 ]        mws_rdata,
    input  wire  [ AXI_ID_WIDTH         -1 : 0 ]        mws_rid,
    input  wire  [ 2                    -1 : 0 ]        mws_rresp,
    input  wire                                         mws_rlast,
    input  wire                                         mws_rvalid,
    output wire                                         mws_rready
);

//==============================================================================
// Localparams
//==============================================================================
    localparam integer  LDMEM_IDLE                   = 0;
    localparam integer  LDMEM_CHECK_RAW              = 1;
    localparam integer  LDMEM_BUSY                   = 2;
    localparam integer  LDMEM_WAIT_0                 = 3;
    localparam integer  LDMEM_WAIT_1                 = 4;
    localparam integer  LDMEM_WAIT_2                 = 5;
    localparam integer  LDMEM_WAIT_3                 = 6;
    localparam integer  LDMEM_DONE                   = 7;

    localparam integer  STMEM_IDLE                   = 0;
    localparam integer  STMEM_DDR                    = 1;
    localparam integer  STMEM_WAIT_0                 = 2;
    localparam integer  STMEM_WAIT_1                 = 3;
    localparam integer  STMEM_WAIT_2                 = 4;
    localparam integer  STMEM_WAIT_3                 = 5;
    localparam integer  STMEM_DONE                   = 6;
    localparam integer  STMEM_PU                     = 7;

    localparam integer  MEM_LD                       = 0;
    localparam integer  MEM_ST                       = 1;
    localparam integer  MEM_RD                       = 2;
    localparam integer  MEM_WR                       = 3;
//==============================================================================

//==============================================================================
// Wires/Regs
//==============================================================================
    wire                                        compute_tag_done;
    wire                                        compute_tag_reuse;
    wire                                        compute_tag_ready;
    wire [ TAG_W                -1 : 0 ]        compute_tag;
    wire [ TAG_W                -1 : 0 ]        compute_tag_delayed;
    wire                                        ldmem_tag_done;
    wire                                        ldmem_tag_ready;
    wire [ TAG_W                -1 : 0 ]        ldmem_tag;
    wire                                        stmem_tag_done;
    wire                                        stmem_tag_ready;
    wire [ TAG_W                -1 : 0 ]        stmem_tag;
    wire                                        stmem_ddr_pe_sw;

    reg  [ 4                    -1 : 0 ]        ldmem_state_d;
    reg  [ 4                    -1 : 0 ]        ldmem_state_q;

    reg  [ 3                    -1 : 0 ]        stmem_state_d;
    reg  [ 3                    -1 : 0 ]        stmem_state_q;

    wire                                        ld_mem_req_v;
    wire                                        st_mem_req_v;

    wire [ TAG_W                -1 : 0 ]        tag;


    reg                                         ld_iter_v_q;
    reg  [ LOOP_ITER_W          -1 : 0 ]        iter_q;

    reg  [ LOOP_ID_W            -1 : 0 ]        ld_loop_id_counter;

    wire [ LOOP_ID_W            -1 : 0 ]        mws_ld_loop_iter_loop_id;
    wire [ LOOP_ITER_W          -1 : 0 ]        mws_ld_loop_iter;
    wire                                        mws_ld_loop_iter_v;
    wire                                        mws_ld_start;
    wire                                        mws_ld_done;
    wire                                        mws_ld_stall;
    wire                                        mws_ld_init;
    wire                                        mws_ld_enter;
    wire                                        mws_ld_exit;
    wire [ LOOP_ID_W            -1 : 0 ]        mws_ld_index;
    wire                                        mws_ld_index_valid;
    wire                                        mws_ld_step;

    wire [ LOOP_ID_W            -1 : 0 ]        mws_st_loop_iter_loop_id;
    wire [ LOOP_ITER_W          -1 : 0 ]        mws_st_loop_iter;
    wire                                        mws_st_loop_iter_v;
    wire                                        mws_st_start;
    wire                                        mws_st_done;
    wire                                        mws_st_stall;
    wire                                        mws_st_init;
    wire                                        mws_st_enter;
    wire                                        mws_st_exit;
    wire [ LOOP_ID_W            -1 : 0 ]        mws_st_index;
    wire                                        mws_st_index_valid;
    wire                                        mws_st_step;

    wire                                        ld_stride_v;
    wire [ ADDR_STRIDE_W        -1 : 0 ]        ld_stride;
    wire [ BUF_TYPE_W           -1 : 0 ]        ld_stride_id;
    wire                                        st_stride_v;
    wire [ ADDR_STRIDE_W        -1 : 0 ]        st_stride;
    wire [ BUF_TYPE_W           -1 : 0 ]        st_stride_id;

    wire [ ADDR_WIDTH           -1 : 0 ]        ld_addr;
    wire [ ADDR_WIDTH           -1 : 0 ]        mws_ld_base_addr;
    wire                                        ld_addr_v;
    wire [ ADDR_WIDTH           -1 : 0 ]        st_addr;
    wire [ ADDR_WIDTH           -1 : 0 ]        mws_st_base_addr;
    wire                                        st_addr_v;


    reg  [ MEM_REQ_W            -1 : 0 ]        ld_req_size;
    reg  [ MEM_REQ_W            -1 : 0 ]        st_req_size;

    wire                                        ld_req_valid_d;
    reg                                         ld_req_valid_q;

    //reg  [ ADDR_WIDTH           -1 : 0 ]        tag_ld_addr[0:NUM_TAGS-1];

    reg  [ ADDR_WIDTH           -1 : 0 ]        ld_req_addr;

    // reg  [ MEM_REQ_W            -1 : 0 ]        ld_req_loop_id;
    reg  [ MEM_REQ_W            -1 : 0 ]        st_req_loop_id;

    wire                                        axi_rd_req;
    wire [ AXI_ID_WIDTH         -1 : 0 ]        axi_rd_req_id;
    wire                                        axi_rd_done;
    wire [ MEM_REQ_W            -1 : 0 ]        axi_rd_req_size;
    wire                                        axi_rd_ready;
    wire [ AXI_ADDR_WIDTH       -1 : 0 ]        axi_rd_addr;

    wire                                        axi_wr_req;
    wire [ AXI_ID_WIDTH         -1 : 0 ]        axi_wr_req_id;
    wire                                        axi_wr_done;
    wire [ MEM_REQ_W            -1 : 0 ]        axi_wr_req_size;
    wire                                        axi_wr_ready;
    wire [ AXI_ADDR_WIDTH       -1 : 0 ]        axi_wr_addr;

    wire [ AXI_ID_WIDTH         -1 : 0 ]        mem_write_id;
    wire                                        mem_write_req;
    wire [ AXI_DATA_WIDTH       -1 : 0 ]        mem_write_data;
    reg  [ MEM_ADDR_W           -1 : 0 ]        mem_write_addr;
    wire                                        mem_write_ready;
    wire [ AXI_DATA_WIDTH       -1 : 0 ]        mem_read_data;
    reg  [ MEM_ADDR_W           -1 : 0 ]        mem_read_addr;
    wire                                        mem_read_req;
    wire                                        mem_read_ready;

  // Adding register to buf read data
    wire [ BUF_DATA_WIDTH       -1 : 0 ]        _buf_read_data;
//==============================================================================

//==============================================================================
// Assigns
//==============================================================================
    assign ld_stride = cfg_loop_stride;
    assign ld_stride_v = cfg_loop_stride_v && cfg_loop_stride_loop_id == 1 + MEM_ID && cfg_loop_stride_type == MEM_LD && cfg_loop_stride_id == MEM_ID;
    assign st_stride = cfg_loop_stride;
    assign st_stride_v = cfg_loop_stride_v && cfg_loop_stride_loop_id == 1 + MEM_ID && cfg_loop_stride_type == MEM_ST && cfg_loop_stride_id == MEM_ID;

    //assign mws_ld_base_addr = tag_ld_addr[ldmem_tag];
    assign axi_rd_req = ld_req_valid_q;
    assign axi_rd_req_size = ld_req_size * (ARRAY_M * DATA_WIDTH) / AXI_DATA_WIDTH;
    assign axi_rd_addr = ld_req_addr;

    assign axi_wr_req = 1'b0;
    assign axi_wr_req_id = 1'b0;
    assign axi_wr_req_size = 0;
    assign axi_wr_addr = 0;
//==============================================================================

//==============================================================================
// Address generators
//==============================================================================
    assign mws_ld_stall = ~ldmem_tag_ready || ~axi_rd_ready;
    assign mws_ld_step = mws_ld_index_valid && !mws_ld_stall;
  mem_walker_stride #(
    .ADDR_WIDTH                     ( ADDR_WIDTH                     ),
    .ADDR_STRIDE_W                  ( ADDR_STRIDE_W                  ),
    .LOOP_ID_W                      ( LOOP_ID_W                      )
  ) mws_ld (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .base_addr                      ( mws_ld_base_addr               ), //input
    .loop_ctrl_done                 ( mws_ld_done                    ), //input
    .loop_index                     ( mws_ld_index                   ), //input
    .loop_index_valid               ( mws_ld_step                    ), //input
    .loop_init                      ( mws_ld_init                    ), //input
    .loop_enter                     ( mws_ld_enter                   ), //input
    .loop_exit                      ( mws_ld_exit                    ), //input
    .cfg_addr_stride_v              ( ld_stride_v                    ), //input
    .cfg_addr_stride                ( ld_stride                      ), //input
    .addr_out                       ( ld_addr                        ), //output
    .addr_out_valid                 ( ld_addr_v                      )  //output
  );
  generate
  if (STORE_ENABLED == 1) begin: STORE
    assign mws_st_step = mws_st_index_valid && !mws_st_stall;
    assign mws_st_stall = ~stmem_tag_ready || ~axi_wr_ready;
  mem_walker_stride #(
    .ADDR_WIDTH                     ( ADDR_WIDTH                     ),
    .ADDR_STRIDE_W                  ( ADDR_STRIDE_W                  ),
    .LOOP_ID_W                      ( LOOP_ID_W                      )
  ) mws_st (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .base_addr                      ( mws_st_base_addr               ), //input
    .loop_ctrl_done                 ( mws_st_done                    ), //input
    .loop_index                     ( mws_st_index                   ), //input
    .loop_index_valid               ( mws_st_step                    ), //input
    .loop_init                      ( mws_st_init                    ), //input
    .loop_enter                     ( mws_st_enter                   ), //input
    .loop_exit                      ( mws_st_exit                    ), //input
    .cfg_addr_stride_v              ( st_stride_v                    ), //input
    .cfg_addr_stride                ( st_stride                      ), //input
    .addr_out                       ( st_addr                        ), //output
    .addr_out_valid                 ( st_addr_v                      )  //output
    );

    //assign mws_st_step = mws_st_index_valid && !mws_st_stall;
    //assign mws_st_stall = ~stmem_tag_ready || ~axi_wr_ready;
  end
  endgenerate
//==============================================================================

//=============================================================
// Loop controller
//=============================================================
  always@(posedge clk)
  begin
    if (reset)
      ld_loop_id_counter <= 'b0;
    else begin
      if (mws_ld_loop_iter_v)
        ld_loop_id_counter <= ld_loop_id_counter + 1'b1;
      else if (tag_req && tag_ready)
        ld_loop_id_counter <= 'b0;
    end
  end

  always @(posedge clk)
  begin
    if (reset)
      ld_iter_v_q <= 1'b0;
    else begin
      if (cfg_loop_iter_v && cfg_loop_iter_loop_id == 1 + MEM_ID)
        ld_iter_v_q <= 1'b1;
      else if (cfg_loop_iter_v || ld_stride_v)
        ld_iter_v_q <= 1'b0;
    end
  end


    assign mws_ld_start = ldmem_state_q == LDMEM_BUSY;
    assign mws_ld_loop_iter_v = ld_stride_v && ld_iter_v_q;
    assign mws_ld_loop_iter = iter_q;
    assign mws_ld_loop_iter_loop_id = ld_loop_id_counter;

  always @(posedge clk)
  begin
    if (reset) begin
      iter_q <= 'b0;
    end
    else if (cfg_loop_iter_v && cfg_loop_iter_loop_id == 1 + MEM_ID) begin
      iter_q <= cfg_loop_iter;
    end
  end

  controller_fsm #(
    .LOOP_ID_W                      ( LOOP_ID_W                      ),
    .LOOP_ITER_W                    ( LOOP_ITER_W                    ),
    .IMEM_ADDR_W                    ( LOOP_ID_W                      )
  ) mws_ld_ctrl (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .stall                          ( mws_ld_stall                   ), //input
    .cfg_loop_iter_v                ( mws_ld_loop_iter_v             ), //input
    .cfg_loop_iter                  ( mws_ld_loop_iter               ), //input
    .cfg_loop_iter_loop_id          ( mws_ld_loop_iter_loop_id       ), //input
    .start                          ( mws_ld_start                   ), //input
    .done                           ( mws_ld_done                    ), //output
    .loop_init                      ( mws_ld_init                    ), //output
    .loop_enter                     ( mws_ld_enter                   ), //output  
    .loop_last_iter                 (                                ), //output
    .loop_exit                      ( mws_ld_exit                    ), //output
    .loop_index                     ( mws_ld_index                   ), //output
    .loop_index_valid               ( mws_ld_index_valid             )  //output
  );
//=============================================================

//==============================================================================
// Memory Request generation
//==============================================================================
    assign ld_mem_req_v = cfg_mem_req_v && cfg_mem_req_loop_id == (1 + MEM_ID) && cfg_mem_req_type == MEM_LD && cfg_mem_req_id == MEM_ID;
  always @(posedge clk)
  begin
    if (reset) begin
      ld_req_size <= 'b0;
      // ld_req_loop_id <= 'b0;
    end
    else if (ld_mem_req_v) begin
      ld_req_size <= cfg_mem_req_size;
      // ld_req_loop_id <= ld_loop_id_counter;
    end
  end

  // assign ld_req_valid_d = (ld_req_loop_id == mws_ld_index) && (mws_ld_enter || mws_ld_step);
    // assign ld_req_valid_d = (ld_req_loop_id == mws_ld_index) && ld_addr_v;
    assign ld_req_valid_d = ld_addr_v;

  always @(posedge clk)
  begin
    if (reset) begin
      ld_req_valid_q <= 1'b0;
      ld_req_addr <= 'b0;
    end
    else begin
      ld_req_valid_q <= ld_req_valid_d;
      ld_req_addr <= ld_addr;
    end
  end

  //always @(posedge clk)
  //begin
  //  if (tag_req && tag_ready) begin
  //    tag_ld_addr[tag] <= tag_base_ld_addr;
  //  end
  //end

  ram #(
    .ADDR_WIDTH                     ( $clog2(NUM_TAGS)),
    .DATA_WIDTH                     ( ADDR_WIDTH)
  ) u_ram_tag_ld_1(
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .s_write_addr                   ( tag),
    .s_write_req                    ( tag_req && tag_ready),
    .s_write_data                   ( tag_base_ld_addr),
    .s_read_addr                    ( ldmem_tag),
    .s_read_req                     ( 1'b1),
    .s_read_data                    ( mws_ld_base_addr)
  );

  // wire [ 31                      : 0 ]        tag0_ld_addr;
  // wire [ 31                      : 0 ]        tag1_ld_addr;
  // wire [ 31                      : 0 ]        tag0_st_addr;
  // wire [ 31                      : 0 ]        tag1_st_addr;
  // assign tag0_ld_addr = tag_ld_addr[0];
  // assign tag1_ld_addr = tag_ld_addr[1];
//==============================================================================

//==============================================================================
// Tag-based synchronization for double buffering
//==============================================================================
    reg                                         raw;
    reg  [ TAG_W                -1 : 0 ]        raw_stmem_tag_d;
    reg  [ TAG_W                -1 : 0 ]        raw_stmem_tag_q;
    wire [ TAG_W                -1 : 0 ]        raw_stmem_tag;
    wire                                        raw_stmem_tag_ready;
    wire [ ADDR_WIDTH           -1 : 0 ]        raw_stmem_st_addr;

  always @(posedge clk)
  begin
    if (reset)
      raw_stmem_tag_q <= 0;
    else
      raw_stmem_tag_q <= raw_stmem_tag_d;
  end

  always @(*)
  begin
    ldmem_state_d = ldmem_state_q;
    raw_stmem_tag_d = raw_stmem_tag_q;
    case(ldmem_state_q)
      LDMEM_IDLE: begin
        if (ldmem_tag_ready) begin
          ldmem_state_d = LDMEM_BUSY;
        end
      end
      LDMEM_BUSY: begin
        if (mws_ld_done)
          ldmem_state_d = LDMEM_WAIT_0;
      end
      LDMEM_WAIT_0: begin
        ldmem_state_d = LDMEM_WAIT_1;
      end
      LDMEM_WAIT_1: begin
        ldmem_state_d = LDMEM_WAIT_2;
      end
      LDMEM_WAIT_2: begin
        ldmem_state_d = LDMEM_WAIT_3;
      end
      LDMEM_WAIT_3: begin
        if (axi_rd_done)
          ldmem_state_d = LDMEM_DONE;
      end
      LDMEM_DONE: begin
        ldmem_state_d = LDMEM_IDLE;
      end
    endcase
  end

  always @(posedge clk)
  begin
    if (reset)
      ldmem_state_q <= LDMEM_IDLE;
    else
      ldmem_state_q <= ldmem_state_d;
  end

  wire pu_done = 1'b1;

  always @(*)
  begin
    stmem_state_d = stmem_state_q;
    case(stmem_state_q)
      STMEM_IDLE: begin
        if (stmem_tag_ready) begin
            stmem_state_d = STMEM_DONE;
        end
      end
      STMEM_DONE: begin
        stmem_state_d = STMEM_IDLE;
      end
    endcase
  end

  always @(posedge clk)
  begin
    if (reset)
      stmem_state_q <= STMEM_IDLE;
    else
      stmem_state_q <= stmem_state_d;
  end

    wire                                        ldmem_ready;

    assign compute_tag_done = compute_done;
    assign compute_ready = compute_tag_ready;

    assign ldmem_tag_done = ldmem_state_q == LDMEM_DONE;
    assign ldmem_ready = ldmem_tag_ready;
  // assign ldmem_tag_done = mws_ld_done;

    assign stmem_tag_done = stmem_state_q == STMEM_DONE;

  tag_sync  #(
    .NUM_TAGS                       ( NUM_TAGS                       )
  )
  mws_tag (
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .block_done                     ( block_done                     ),
    .tag_req                        ( tag_req                        ),
    .tag_reuse                      ( tag_reuse                      ),
    .tag_bias_prev_sw               ( tag_bias_prev_sw               ),
    .tag_ddr_pe_sw                  ( tag_ddr_pe_sw                  ), //input
    .tag_ready                      ( tag_ready                      ),
    .tag                            ( tag                            ),
    .tag_done                       ( tag_done                       ),
    .raw_stmem_tag                  ( raw_stmem_tag_q                ),
    .raw_stmem_tag_ready            ( raw_stmem_tag_ready            ),
    .compute_tag_done               ( compute_tag_done               ),
    .compute_tag_ready              ( compute_tag_ready              ),
    .compute_bias_prev_sw           ( compute_bias_prev_sw           ),
    .compute_tag                    ( compute_tag                    ),
    .ldmem_tag_done                 ( ldmem_tag_done                 ),
    .ldmem_tag_ready                ( ldmem_tag_ready                ),
    .ldmem_tag                      ( ldmem_tag                      ),
    .stmem_ddr_pe_sw                ( stmem_ddr_pe_sw                ),
    .stmem_tag_done                 ( stmem_tag_done                 ),
    .stmem_tag_ready                ( stmem_tag_ready                ),
    .stmem_tag                      ( stmem_tag                      )
  );
//==============================================================================


//==============================================================================
// AXI4 Memory Mapped interface
//==============================================================================
    assign mem_write_ready = 1'b1;
    assign mem_read_ready = 1'b1;
    assign axi_rd_req_id = 0;
    assign mem_read_data = 0;
  axi_master #(
    .TX_SIZE_WIDTH                  ( MEM_REQ_W                      ),
    .AXI_DATA_WIDTH                 ( AXI_DATA_WIDTH                 ),
    .AXI_ADDR_WIDTH                 ( AXI_ADDR_WIDTH                 ),
    .AXI_BURST_WIDTH                ( AXI_BURST_WIDTH                )
  ) u_axi_mm_master (
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .m_axi_awaddr                   ( mws_awaddr                     ),
    .m_axi_awlen                    ( mws_awlen                      ),
    .m_axi_awsize                   ( mws_awsize                     ),
    .m_axi_awburst                  ( mws_awburst                    ),
    .m_axi_awvalid                  ( mws_awvalid                    ),
    .m_axi_awready                  ( mws_awready                    ),
    .m_axi_wdata                    ( mws_wdata                      ),
    .m_axi_wstrb                    ( mws_wstrb                      ),
    .m_axi_wlast                    ( mws_wlast                      ),
    .m_axi_wvalid                   ( mws_wvalid                     ),
    .m_axi_wready                   ( mws_wready                     ),
    .m_axi_bresp                    ( mws_bresp                      ),
    .m_axi_bvalid                   ( mws_bvalid                     ),
    .m_axi_bready                   ( mws_bready                     ),
    .m_axi_araddr                   ( mws_araddr                     ),
    .m_axi_arid                     ( mws_arid                       ),
    .m_axi_arlen                    ( mws_arlen                      ),
    .m_axi_arsize                   ( mws_arsize                     ),
    .m_axi_arburst                  ( mws_arburst                    ),
    .m_axi_arvalid                  ( mws_arvalid                    ),
    .m_axi_arready                  ( mws_arready                    ),
    .m_axi_rdata                    ( mws_rdata                      ),
    .m_axi_rid                      ( mws_rid                        ),
    .m_axi_rresp                    ( mws_rresp                      ),
    .m_axi_rlast                    ( mws_rlast                      ),
    .m_axi_rvalid                   ( mws_rvalid                     ),
    .m_axi_rready                   ( mws_rready                     ),
    // Buffer
    .mem_write_req                  ( mem_write_req                  ),
    .mem_write_id                   ( mem_write_id                   ),
    .mem_write_data                 ( mem_write_data                 ),
    .mem_write_ready                ( mem_write_ready                ),
    .mem_read_data                  ( mem_read_data                  ),
    .mem_read_req                   ( mem_read_req                   ),
    .mem_read_ready                 ( mem_read_ready                 ),
    // AXI RD Req
    .rd_req                         ( axi_rd_req                     ),
    .rd_req_id                      ( axi_rd_req_id                  ),
    .rd_done                        ( axi_rd_done                    ),
    .rd_ready                       ( axi_rd_ready                   ),
    .rd_req_size                    ( axi_rd_req_size                ),
    .rd_addr                        ( axi_rd_addr                    ),
    // AXI WR Req
    .wr_req                         ( axi_wr_req                     ),
    .wr_req_id                      ( axi_wr_req_id                  ),
    .wr_ready                       ( axi_wr_ready                   ),
    .wr_req_size                    ( axi_wr_req_size                ),
    .wr_addr                        ( axi_wr_addr                    ),
    .wr_done                        ( axi_wr_done                    )
  );
//==============================================================================

//==============================================================================
// Dual-port RAM
//==============================================================================
  always @(posedge clk)
  begin
    if (reset)
      mem_write_addr <= 0;
    else begin
      if (mem_write_req)
        mem_write_addr <= mem_write_addr + 1'b1;
      else if (ldmem_state_q == LDMEM_DONE)
        mem_write_addr <= 0;
    end
  end

    wire [ TAG_MEM_ADDR_W       -1 : 0 ]        tag_mem_write_addr;
    wire [ TAG_BUF_ADDR_W       -1 : 0 ]        tag_buf_read_addr;

    assign tag_mem_write_addr = {ldmem_tag, mem_write_addr};

  genvar i;
  generate
    if (MEM_ID == 1 || MEM_ID == 3)
    begin: OBUF_TAG_DELAY
      for (i=0; i<ARRAY_N+3; i=i+1)
      begin: TAG_DELAY_LOOP
        wire [TAG_W-1:0] prev_tag, next_tag;
        if (i==0)
    assign prev_tag = compute_tag;
        else
    assign prev_tag = OBUF_TAG_DELAY.TAG_DELAY_LOOP[i-1].next_tag;
        register_sync #(TAG_W) tag_delay (clk, reset, prev_tag, next_tag);
      end
      // Increased compute tag delays
      // Might need a separate compute_tag_delayed for buf_read
    assign compute_tag_delayed = OBUF_TAG_DELAY.TAG_DELAY_LOOP[ARRAY_N+2].next_tag;
    end
    else begin
    assign compute_tag_delayed = compute_tag;
    end
  endgenerate

    assign tag_buf_read_addr = {compute_tag_delayed, buf_read_addr};

  register_sync #(BUF_DATA_WIDTH)
  buf_read_data_delay (clk, reset, _buf_read_data, buf_read_data);

  bbuf #(
    .TAG_W                          ( TAG_W                          ),
    .BUF_ADDR_WIDTH                 ( TAG_BUF_ADDR_W                 ),
    .ARRAY_M                        ( ARRAY_M                        ),
    .MEM_DATA_WIDTH                 ( AXI_DATA_WIDTH                 ),
    .DATA_WIDTH                     ( DATA_WIDTH                     )
  ) buf_ram (
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .mem_write_addr                 ( tag_mem_write_addr             ),
    .mem_write_req                  ( mem_write_req                  ),
    .mem_write_data                 ( mem_write_data                 ),
    .buf_read_addr                  ( tag_buf_read_addr              ),
    .buf_read_req                   ( buf_read_req                   ),
    .buf_read_data                  ( _buf_read_data                 )
  );
//==============================================================================


//==============================================================================

`ifdef COCOTB_SIM
  integer req_count;
  always @(posedge clk)
  begin
    if (reset) req_count <= 0;
    else req_count = req_count + (tag_req && tag_ready);
  end

  integer wr_req_count=0;
  always @(posedge clk)
    if (reset)
      wr_req_count <= 0;
    else
      wr_req_count <= wr_req_count + axi_wr_req;

  integer rd_req_count=0;
  integer missed_rd_req_count=0;
  always @(posedge clk)
    if (reset)
      rd_req_count <= 0;
    else
      rd_req_count <= rd_req_count + axi_rd_req;
  always @(posedge clk)
    if (reset)
      missed_rd_req_count <= 0;
    else
      missed_rd_req_count <= missed_rd_req_count + (axi_rd_req && ~axi_rd_ready);
`endif


//=============================================================
// VCD
//=============================================================
`ifdef COCOTB_TOPLEVEL_mem_wrapper
initial begin
  $dumpfile("mem_wrapper.vcd");
  $dumpvars(0, mem_wrapper);
end
`endif
//=============================================================
endmodule
//
// DnnWeaver2 performance monitor
//
// Hardik Sharma
// (hsharma@gatech.edu)

`timescale 1ns/1ps
module performance_monitor #(
    parameter integer  STATS_WIDTH                  = 32,
    parameter integer  INST_BURST_WIDTH             = 8,
    parameter integer  AXI_BURST_WIDTH              = 8
) (
    input  wire                                         clk,
    input  wire                                         reset,
    input  wire  [ 3                    -1 : 0 ]        dnnweaver2_state,

    input  wire                                         tag_req,
    input  wire                                         tag_ready,

    input  wire                                         ibuf_tag_done,
    input  wire                                         wbuf_tag_done,
    input  wire                                         obuf_tag_done,
    input  wire                                         bias_tag_done,

    input  wire                                         decoder_start,

    input  wire                                         pci_cl_data_awvalid,
    input  wire  [ INST_BURST_WIDTH     -1 : 0 ]        pci_cl_data_awlen,
    input  wire                                         pci_cl_data_awready,
    input  wire                                         pci_cl_data_arvalid,
    input  wire  [ INST_BURST_WIDTH     -1 : 0 ]        pci_cl_data_arlen,
    input  wire                                         pci_cl_data_arready,
    input  wire                                         pci_cl_data_wvalid,
    input  wire                                         pci_cl_data_wready,
    input  wire                                         pci_cl_data_rvalid,
    input  wire                                         pci_cl_data_rready,

    output wire  [ STATS_WIDTH          -1 : 0 ]        decode_cycles,
    output wire  [ STATS_WIDTH          -1 : 0 ]        execute_cycles,
    output wire  [ STATS_WIDTH          -1 : 0 ]        busy_cycles,

    output wire  [ STATS_WIDTH          -1 : 0 ]        tag_started,
    output wire  [ STATS_WIDTH          -1 : 0 ]        block_started,
    output wire  [ STATS_WIDTH          -1 : 0 ]        block_finished,

    output wire  [ STATS_WIDTH          -1 : 0 ]        axi_wr_id,
    output wire  [ STATS_WIDTH          -1 : 0 ]        axi_write_req,
    output wire  [ STATS_WIDTH          -1 : 0 ]        axi_write_finished,
    output wire  [ STATS_WIDTH          -1 : 0 ]        axi_read_req,
    output wire  [ STATS_WIDTH          -1 : 0 ]        axi_read_finished,

  // Snoop CL DDR0
    // AR channel
    input  wire                                         snoop_cl_ddr0_arvalid,
    input  wire                                         snoop_cl_ddr0_arready,
    input  wire  [ AXI_BURST_WIDTH      -1 : 0 ]        snoop_cl_ddr0_arlen,
    // R channel
    input  wire                                         snoop_cl_ddr0_rvalid,
    input  wire                                         snoop_cl_ddr0_rready,

  // Snoop CL DDR1
    // AW channel
    input  wire                                         snoop_cl_ddr1_awvalid,
    input  wire                                         snoop_cl_ddr1_awready,
    input  wire  [ AXI_BURST_WIDTH      -1 : 0 ]        snoop_cl_ddr1_awlen,
    // AR channel
    input  wire                                         snoop_cl_ddr1_arvalid,
    input  wire                                         snoop_cl_ddr1_arready,
    input  wire  [ AXI_BURST_WIDTH      -1 : 0 ]        snoop_cl_ddr1_arlen,
    // W channel
    input  wire                                         snoop_cl_ddr1_wvalid,
    input  wire                                         snoop_cl_ddr1_wready,
    // R channel
    input  wire                                         snoop_cl_ddr1_rvalid,
    input  wire                                         snoop_cl_ddr1_rready,

  // Snoop CL DDR2
    // AR channel
    input  wire                                         snoop_cl_ddr2_arvalid,
    input  wire                                         snoop_cl_ddr2_arready,
    input  wire  [ AXI_BURST_WIDTH      -1 : 0 ]        snoop_cl_ddr2_arlen,
    // R channel
    input  wire                                         snoop_cl_ddr2_rvalid,
    input  wire                                         snoop_cl_ddr2_rready,

  // Snoop CL DDR3
    // AR channel
    input  wire                                         snoop_cl_ddr3_arvalid,
    input  wire                                         snoop_cl_ddr3_arready,
    input  wire  [ AXI_BURST_WIDTH      -1 : 0 ]        snoop_cl_ddr3_arlen,
    // R channel
    input  wire                                         snoop_cl_ddr3_rvalid,
    input  wire                                         snoop_cl_ddr3_rready,

  // CL DDR Stats
    // CL DDR0 - IBUF
    output wire  [ STATS_WIDTH          -1 : 0 ]        pmon_cl_ddr0_read_req,
    output wire  [ STATS_WIDTH          -1 : 0 ]        pmon_cl_ddr0_read_finished,
    // CL DDR1 - OBUF
    output wire  [ STATS_WIDTH          -1 : 0 ]        pmon_cl_ddr1_write_req,
    output wire  [ STATS_WIDTH          -1 : 0 ]        pmon_cl_ddr1_write_finished,
    output wire  [ STATS_WIDTH          -1 : 0 ]        pmon_cl_ddr1_read_req,
    output wire  [ STATS_WIDTH          -1 : 0 ]        pmon_cl_ddr1_read_finished,
    // CL DDR1 - WBUF
    output wire  [ STATS_WIDTH          -1 : 0 ]        pmon_cl_ddr2_read_req,
    output wire  [ STATS_WIDTH          -1 : 0 ]        pmon_cl_ddr2_read_finished,
    // CL DDR1 - BIAS
    output wire  [ STATS_WIDTH          -1 : 0 ]        pmon_cl_ddr3_read_req,
    output wire  [ STATS_WIDTH          -1 : 0 ]        pmon_cl_ddr3_read_finished,
    // CL DDR1 - OBUF
    output wire  [ STATS_WIDTH          -1 : 0 ]        pmon_cl_ddr4_write_req,
    output wire  [ STATS_WIDTH          -1 : 0 ]        pmon_cl_ddr4_write_finished,
    output wire  [ STATS_WIDTH          -1 : 0 ]        pmon_cl_ddr4_read_req,
    output wire  [ STATS_WIDTH          -1 : 0 ]        pmon_cl_ddr4_read_finished,

  // Snoop CL DDR4
    // AW channel
    input  wire                                         snoop_cl_ddr4_awvalid,
    input  wire                                         snoop_cl_ddr4_awready,
    input  wire  [ AXI_BURST_WIDTH      -1 : 0 ]        snoop_cl_ddr4_awlen,
    // AR channel
    input  wire                                         snoop_cl_ddr4_arvalid,
    input  wire                                         snoop_cl_ddr4_arready,
    input  wire  [ AXI_BURST_WIDTH      -1 : 0 ]        snoop_cl_ddr4_arlen,
    // W channel
    input  wire                                         snoop_cl_ddr4_wvalid,
    input  wire                                         snoop_cl_ddr4_wready,
    // R channel
    input  wire                                         snoop_cl_ddr4_rvalid,
    input  wire                                         snoop_cl_ddr4_rready

  );

//=============================================================
// Localparam
//=============================================================
  // dnnweaver2 controller state
    localparam integer  IDLE                         = 0;
    localparam integer  DECODE                       = 1;
    localparam integer  BASE_LOOP                    = 2;
    localparam integer  MEM_WAIT                     = 3;
//=============================================================

//=============================================================
// Wires/Regs
//=============================================================
    reg  [ STATS_WIDTH          -1 : 0 ]        decode_cycles_d;
    reg  [ STATS_WIDTH          -1 : 0 ]        decode_cycles_q;
    reg  [ STATS_WIDTH          -1 : 0 ]        execute_cycles_d;
    reg  [ STATS_WIDTH          -1 : 0 ]        execute_cycles_q;
    reg  [ STATS_WIDTH          -1 : 0 ]        busy_cycles_d;
    reg  [ STATS_WIDTH          -1 : 0 ]        busy_cycles_q;

    reg  [ STATS_WIDTH          -1 : 0 ]        _tag_started;
    reg  [ STATS_WIDTH          -1 : 0 ]        _block_started;
    reg  [ STATS_WIDTH          -1 : 0 ]        _block_finished;

    reg  [ STATS_WIDTH          -1 : 0 ]        _axi_wr_id;
    reg  [ STATS_WIDTH          -1 : 0 ]        _axi_write_req;
    reg  [ STATS_WIDTH          -1 : 0 ]        _axi_write_finished;
    reg  [ STATS_WIDTH          -1 : 0 ]        _axi_read_req;
    reg  [ STATS_WIDTH          -1 : 0 ]        _axi_read_finished;

    reg  [ STATS_WIDTH          -1 : 0 ]        _cl_ddr0_read_req;
    reg  [ STATS_WIDTH          -1 : 0 ]        _cl_ddr0_read_finished;

    reg  [ STATS_WIDTH          -1 : 0 ]        _cl_ddr1_write_req;
    reg  [ STATS_WIDTH          -1 : 0 ]        _cl_ddr1_write_finished;
    reg  [ STATS_WIDTH          -1 : 0 ]        _cl_ddr1_read_req;
    reg  [ STATS_WIDTH          -1 : 0 ]        _cl_ddr1_read_finished;

    reg  [ STATS_WIDTH          -1 : 0 ]        _cl_ddr2_read_req;
    reg  [ STATS_WIDTH          -1 : 0 ]        _cl_ddr2_read_finished;

    reg  [ STATS_WIDTH          -1 : 0 ]        _cl_ddr3_read_req;
    reg  [ STATS_WIDTH          -1 : 0 ]        _cl_ddr3_read_finished;

    reg  [ STATS_WIDTH          -1 : 0 ]        _cl_ddr4_write_req;
    reg  [ STATS_WIDTH          -1 : 0 ]        _cl_ddr4_write_finished;
    reg  [ STATS_WIDTH          -1 : 0 ]        _cl_ddr4_read_req;
    reg  [ STATS_WIDTH          -1 : 0 ]        _cl_ddr4_read_finished;

//=============================================================

//=============================================================
// Performance stats
//=============================================================
  always @(posedge clk)
  begin
    if (reset) begin
      busy_cycles_q <= 0;
      decode_cycles_q <= 0;
      execute_cycles_q <= 0;
    end
    else begin
      busy_cycles_q <= busy_cycles_d;
      decode_cycles_q <= decode_cycles_d;
      execute_cycles_q <= execute_cycles_d;
    end
  end

  always @(*)
  begin
    busy_cycles_d = busy_cycles_q;
    decode_cycles_d = decode_cycles_q;
    execute_cycles_d = execute_cycles_q;
    case(dnnweaver2_state)
      IDLE: begin
        if (decoder_start) begin
          busy_cycles_d = 0;
          decode_cycles_d = 0;
          execute_cycles_d = 0;
        end
      end
      DECODE: begin
        busy_cycles_d = busy_cycles_q + 1'b1;
        decode_cycles_d = decode_cycles_q + 1'b1;
      end
      BASE_LOOP: begin
        execute_cycles_d = execute_cycles_q + 1'b1;
        busy_cycles_d = busy_cycles_q + 1'b1;
      end
      MEM_WAIT: begin
        execute_cycles_d = execute_cycles_q + 1'b1;
        busy_cycles_d = busy_cycles_q + 1'b1;
      end
    endcase
  end

  always @(posedge clk)
  begin
    if (reset) begin
      _axi_write_req <= 0;
      _axi_write_finished <= 0;
      _axi_read_req <= 0;
      _axi_read_finished <= 0;
    end else begin
      if (pci_cl_data_awvalid && pci_cl_data_awready)
        _axi_write_req <= _axi_write_req + pci_cl_data_awlen + 1'b1;
      if (pci_cl_data_wvalid && pci_cl_data_wready)
        _axi_write_finished <= _axi_write_finished + 1'b1;
      if (pci_cl_data_arvalid && pci_cl_data_arready)
        _axi_read_req <= _axi_read_req + pci_cl_data_arlen + 1'b1;
      if (pci_cl_data_rvalid && pci_cl_data_rready)
        _axi_read_finished <= _axi_read_finished + 1'b1;
    end
  end

  always @(posedge clk)
  begin
    if (reset) begin
      _block_started <= 0;
      _block_finished <= 0;
      _tag_started <= 0;
    end else begin
      if (decoder_start)
        _block_started <= _block_started + 1'b1;
      if (ibuf_tag_done && wbuf_tag_done && obuf_tag_done && dnnweaver2_state == MEM_WAIT)
        _block_finished <= _block_finished + 1'b1;
      if (tag_req && tag_ready)
        _tag_started <= _tag_started + 1'b1;
    end
  end

  always @(posedge clk)
  begin
    if (reset) begin
      _axi_wr_id <= 0;
    end else if (pci_cl_data_awvalid && pci_cl_data_awready) begin
      _axi_wr_id <= _axi_wr_id + 1'b1;
    end
  end
//=============================================================

//=============================================================
// CL DDR Snoop
//=============================================================
  always @(posedge clk)
  begin
    if (reset) begin
      _cl_ddr0_read_req <= 0;
      _cl_ddr0_read_finished <= 0;
    end else begin
      if (decoder_start) begin
        _cl_ddr0_read_req <= 0;
        _cl_ddr0_read_finished <= 0;
      end else begin
        if (snoop_cl_ddr0_arvalid && snoop_cl_ddr0_arready)
          _cl_ddr0_read_req <= _cl_ddr0_read_req + snoop_cl_ddr0_arlen + 1'b1;
        if (snoop_cl_ddr0_rvalid && snoop_cl_ddr0_rready)
          _cl_ddr0_read_finished <= _cl_ddr0_read_finished + 1'b1;
      end
    end
  end

  always @(posedge clk)
  begin
    if (reset) begin
      _cl_ddr1_write_req <= 0;
      _cl_ddr1_write_finished <= 0;
      _cl_ddr1_read_req <= 0;
      _cl_ddr1_read_finished <= 0;
    end else begin
      if (decoder_start) begin
        _cl_ddr1_write_req <= 0;
        _cl_ddr1_write_finished <= 0;
        _cl_ddr1_read_req <= 0;
        _cl_ddr1_read_finished <= 0;
      end else begin
        if (snoop_cl_ddr1_awvalid && snoop_cl_ddr1_awready)
          _cl_ddr1_write_req <= _cl_ddr1_write_req + snoop_cl_ddr1_awlen + 1'b1;
        if (snoop_cl_ddr1_wvalid && snoop_cl_ddr1_wready)
          _cl_ddr1_write_finished <= _cl_ddr1_write_finished + 1'b1;
        if (snoop_cl_ddr1_arvalid && snoop_cl_ddr1_arready)
          _cl_ddr1_read_req <= _cl_ddr1_read_req + snoop_cl_ddr1_arlen + 1'b1;
        if (snoop_cl_ddr1_rvalid && snoop_cl_ddr1_rready)
          _cl_ddr1_read_finished <= _cl_ddr1_read_finished + 1'b1;
      end
    end
  end

  always @(posedge clk)
  begin
    if (reset) begin
      _cl_ddr2_read_req <= 0;
      _cl_ddr2_read_finished <= 0;
    end else begin
      if (decoder_start) begin
        _cl_ddr2_read_req <= 0;
        _cl_ddr2_read_finished <= 0;
      end else begin
        if (snoop_cl_ddr2_arvalid && snoop_cl_ddr2_arready)
          _cl_ddr2_read_req <= _cl_ddr2_read_req + snoop_cl_ddr2_arlen + 1'b1;
        if (snoop_cl_ddr2_rvalid && snoop_cl_ddr2_rready)
          _cl_ddr2_read_finished <= _cl_ddr2_read_finished + 1'b1;
      end
    end
  end

  always @(posedge clk)
  begin
    if (reset) begin
      _cl_ddr3_read_req <= 0;
      _cl_ddr3_read_finished <= 0;
    end else begin
      if (decoder_start) begin
        _cl_ddr3_read_req <= 0;
        _cl_ddr3_read_finished <= 0;
      end else begin
        if (snoop_cl_ddr3_arvalid && snoop_cl_ddr3_arready)
          _cl_ddr3_read_req <= _cl_ddr3_read_req + snoop_cl_ddr3_arlen + 1'b1;
        if (snoop_cl_ddr3_rvalid && snoop_cl_ddr3_rready)
          _cl_ddr3_read_finished <= _cl_ddr3_read_finished + 1'b1;
      end
    end
  end

  always @(posedge clk)
  begin
    if (reset) begin
      _cl_ddr4_write_req <= 0;
      _cl_ddr4_write_finished <= 0;
      _cl_ddr4_read_req <= 0;
      _cl_ddr4_read_finished <= 0;
    end else begin
      if (decoder_start) begin
        _cl_ddr4_write_req <= 0;
        _cl_ddr4_write_finished <= 0;
        _cl_ddr4_read_req <= 0;
        _cl_ddr4_read_finished <= 0;
      end else begin
        if (snoop_cl_ddr4_awvalid && snoop_cl_ddr4_awready)
          _cl_ddr4_write_req <= _cl_ddr4_write_req + snoop_cl_ddr4_awlen + 1'b1;
        if (snoop_cl_ddr4_wvalid && snoop_cl_ddr4_wready)
          _cl_ddr4_write_finished <= _cl_ddr4_write_finished + 1'b1;
        if (snoop_cl_ddr4_arvalid && snoop_cl_ddr4_arready)
          _cl_ddr4_read_req <= _cl_ddr4_read_req + snoop_cl_ddr4_arlen + 1'b1;
        if (snoop_cl_ddr4_rvalid && snoop_cl_ddr4_rready)
          _cl_ddr4_read_finished <= _cl_ddr4_read_finished + 1'b1;
      end
    end
  end
//=============================================================

//=============================================================
// Assigns
//=============================================================
    assign decode_cycles = decode_cycles_q;
    assign execute_cycles = execute_cycles_q;
    assign busy_cycles = busy_cycles_q;

    assign axi_wr_id = _axi_wr_id;
    assign axi_write_req = _axi_write_req;
    assign axi_write_finished = _axi_write_finished;
    assign axi_read_req = _axi_read_req;
    assign axi_read_finished = _axi_read_finished;

    assign tag_started = _tag_started;
    assign block_started = _block_started;
    assign block_finished = _block_finished;

    assign pmon_cl_ddr0_read_req = _cl_ddr0_read_req;
    assign pmon_cl_ddr0_read_finished = _cl_ddr0_read_finished;

    assign pmon_cl_ddr1_write_req = _cl_ddr1_write_req;
    assign pmon_cl_ddr1_write_finished = _cl_ddr1_write_finished;
    assign pmon_cl_ddr1_read_req = _cl_ddr1_read_req;
    assign pmon_cl_ddr1_read_finished = _cl_ddr1_read_finished;

    assign pmon_cl_ddr2_read_req = _cl_ddr2_read_req;
    assign pmon_cl_ddr2_read_finished = _cl_ddr2_read_finished;

    assign pmon_cl_ddr3_read_req = _cl_ddr3_read_req;
    assign pmon_cl_ddr3_read_finished = _cl_ddr3_read_finished;

    assign pmon_cl_ddr4_write_req = _cl_ddr4_write_req;
    assign pmon_cl_ddr4_write_finished = _cl_ddr4_write_finished;
    assign pmon_cl_ddr4_read_req = _cl_ddr4_read_req;
    assign pmon_cl_ddr4_read_finished = _cl_ddr4_read_finished;

//=============================================================

//=============================================================
// VCD
//=============================================================
  `ifdef COCOTB_TOPLEVEL_performance_monitor
  initial begin
    $dumpfile("performance_monitor.vcd");
    $dumpvars(0, performance_monitor);
  end
  `endif
//=============================================================

endmodule
//
// Instruction Memory
//
// Hardik Sharma
// (hsharma@gatech.edu)
`timescale 1ns/1ps
module instruction_memory
#(
    parameter integer  DATA_WIDTH                   = 32,
    parameter integer  SIZE_IN_BITS                 = 1<<16,
    parameter integer  ADDR_WIDTH                   = $clog2(SIZE_IN_BITS/DATA_WIDTH),
  // Instructions
    parameter integer  INST_DATA_WIDTH              = 32,
    parameter integer  INST_ADDR_WIDTH              = 32,
    parameter integer  INST_WSTRB_WIDTH             = INST_DATA_WIDTH/8,
    parameter integer  INST_BURST_WIDTH             = 8
)
(
  // clk, reset
    input  wire                                         clk,
    input  wire                                         reset,

  // Decoder <- imem
    input  wire                                         s_read_req_b,
    input  wire  [ ADDR_WIDTH           -1 : 0 ]        s_read_addr_b,
    output wire  [ DATA_WIDTH           -1 : 0 ]        s_read_data_b,

  // PCIe -> CL_wrapper AXI4 interface
  // Slave Interface Write Address
    input  wire  [ INST_ADDR_WIDTH      -1 : 0 ]        pci_cl_data_awaddr,
    input  wire  [ INST_BURST_WIDTH     -1 : 0 ]        pci_cl_data_awlen,
    input  wire  [ 3                    -1 : 0 ]        pci_cl_data_awsize,
    input  wire  [ 2                    -1 : 0 ]        pci_cl_data_awburst,
    input  wire                                         pci_cl_data_awvalid,
    output wire                                         pci_cl_data_awready,
  // Slave Interface Write Data
    input  wire  [ INST_DATA_WIDTH      -1 : 0 ]        pci_cl_data_wdata,
    input  wire  [ INST_WSTRB_WIDTH     -1 : 0 ]        pci_cl_data_wstrb,
    input  wire                                         pci_cl_data_wlast,
    input  wire                                         pci_cl_data_wvalid,
    output wire                                         pci_cl_data_wready,
  // Slave Interface Write Response
    output wire  [ 2                    -1 : 0 ]        pci_cl_data_bresp,
    output wire                                         pci_cl_data_bvalid,
    input  wire                                         pci_cl_data_bready,
  // Slave Interface Read Address
    input  wire  [ INST_ADDR_WIDTH      -1 : 0 ]        pci_cl_data_araddr,
    input  wire  [ INST_BURST_WIDTH     -1 : 0 ]        pci_cl_data_arlen,
    input  wire  [ 3                    -1 : 0 ]        pci_cl_data_arsize,
    input  wire  [ 2                    -1 : 0 ]        pci_cl_data_arburst,
    input  wire                                         pci_cl_data_arvalid,
    output wire                                         pci_cl_data_arready,
  // Slave Interface Read Data
    output wire  [ INST_DATA_WIDTH      -1 : 0 ]        pci_cl_data_rdata,
    output wire  [ 2                    -1 : 0 ]        pci_cl_data_rresp,
    output wire                                         pci_cl_data_rlast,
    output wire                                         pci_cl_data_rvalid,
    input  wire                                         pci_cl_data_rready
);

//=============================================================
// Localparams
//=============================================================
  // Width of state
    localparam integer  STATE_W                      = 3;
  // States
    localparam integer  IMEM_IDLE                    = 0;
    localparam integer  IMEM_WR_ADDR                 = 1;
    localparam integer  IMEM_WR_DATA                 = 2;
    localparam integer  IMEM_RD_ADDR                 = 3;
    localparam integer  IMEM_RD_REQ                  = 4;
    localparam integer  IMEM_RD_DATA                 = 5;
  // RD count
    localparam          R_COUNT_W                    = INST_BURST_WIDTH + 1;
  // Bytes per word
    localparam BYTES_PER_WORD = DATA_WIDTH / 8;
    localparam BYTE_ADDR_W = $clog2(BYTES_PER_WORD);
//=============================================================

//=============================================================
// Wires/Regs
//=============================================================
  // Host <-> imem
    wire                                        s_req_a;
    wire                                        s_wr_en_a;
    wire [ DATA_WIDTH           -1 : 0 ]        s_read_data_a;
    wire [ ADDR_WIDTH           -1 : 0 ]        s_read_addr_a;
    wire [ DATA_WIDTH           -1 : 0 ]        s_write_data_a;
    wire [ ADDR_WIDTH           -1 : 0 ]        s_write_addr_a;
  // FSM for writes to instruction memory (imem)
    reg  [ STATE_W              -1 : 0 ]        imem_state_q;
    reg  [ STATE_W              -1 : 0 ]        imem_state_d;
  // writes address for instruction memory (imem)
    reg  [ INST_ADDR_WIDTH      -1 : 0 ]        w_addr_d;
    reg  [ INST_ADDR_WIDTH      -1 : 0 ]        w_addr_q;

  // read address for instruction memory (imem)
    reg  [ INST_ADDR_WIDTH      -1 : 0 ]        r_addr_d;
    reg  [ INST_ADDR_WIDTH      -1 : 0 ]        r_addr_q;
  // read counter
    reg  [ R_COUNT_W            -1 : 0 ]        r_count_d;
    reg  [ R_COUNT_W            -1 : 0 ]        r_count_q;
    reg  [ R_COUNT_W            -1 : 0 ]        r_count_max_d;
    reg  [ R_COUNT_W            -1 : 0 ]        r_count_max_q;

    reg  [ DATA_WIDTH           -1 : 0 ]        _s_read_data_a;
    reg  [ DATA_WIDTH           -1 : 0 ]        _s_read_data_b;
//=============================================================

//=============================================================
// Assigns
//=============================================================
    //assign s_read_data_a = _s_read_data_a;
    //assign s_read_data_b = _s_read_data_b;

    assign pci_cl_data_awready = imem_state_q == IMEM_WR_ADDR;
    assign pci_cl_data_wready = imem_state_q == IMEM_WR_DATA;

    assign pci_cl_data_bvalid = imem_state_q == IMEM_WR_DATA && pci_cl_data_wlast;
    assign pci_cl_data_bresp = 0;

    assign pci_cl_data_arready = imem_state_q == IMEM_RD_ADDR;
    assign pci_cl_data_rvalid = imem_state_q == IMEM_RD_DATA;
    assign pci_cl_data_rlast = imem_state_q == IMEM_RD_DATA && pci_cl_data_rready && r_count_q == r_count_max_q;


    assign s_write_addr_a = w_addr_q[INST_ADDR_WIDTH-1:BYTE_ADDR_W];
    assign s_write_data_a = pci_cl_data_wdata;

    assign s_read_addr_a = r_addr_q[INST_ADDR_WIDTH-1:BYTE_ADDR_W];
    assign s_req_a = ((imem_state_q == IMEM_RD_REQ ||
                     (imem_state_q == IMEM_RD_DATA && pci_cl_data_rready))
                     || (imem_state_q == IMEM_WR_DATA && pci_cl_data_wvalid));
    assign s_wr_en_a = imem_state_q == IMEM_WR_DATA;

    assign pci_cl_data_rdata = s_read_data_a;
    assign pci_cl_data_rresp = 2'b0;
//=============================================================

//=============================================================
// FSM
//=============================================================
  always @(*)
  begin: READ_FSM
    imem_state_d = imem_state_q;
    r_addr_d = r_addr_q;
    r_count_max_d = r_count_max_q;
    r_count_d = r_count_q;
    w_addr_d = w_addr_q;
    case(imem_state_q)
      IMEM_IDLE: begin
        if (pci_cl_data_awvalid)
        begin
          imem_state_d = IMEM_WR_ADDR;
        end else if (pci_cl_data_arvalid)
        begin
          imem_state_d = IMEM_RD_ADDR;
        end
      end
      IMEM_WR_ADDR: begin
        if (pci_cl_data_awvalid) begin
          w_addr_d = pci_cl_data_awaddr;
          imem_state_d = IMEM_WR_DATA;
        end
        else
          imem_state_d = IMEM_IDLE;
      end
      IMEM_WR_DATA: begin
        if (pci_cl_data_wlast)
          imem_state_d = IMEM_IDLE;
        if (pci_cl_data_wvalid)
          w_addr_d = w_addr_d + BYTES_PER_WORD;
      end
      IMEM_RD_ADDR: begin
        if (pci_cl_data_arvalid) begin
          r_addr_d = pci_cl_data_araddr;
          r_count_max_d = pci_cl_data_arlen;
          r_count_d = 0;
          imem_state_d = IMEM_RD_REQ;
        end
        else
          imem_state_d = IMEM_IDLE;
      end
      IMEM_RD_REQ: begin
        r_addr_d = r_addr_d + BYTES_PER_WORD;
        imem_state_d = IMEM_RD_DATA;
      end
      IMEM_RD_DATA: begin
        if (pci_cl_data_rlast)
          imem_state_d = IMEM_IDLE;
        if (pci_cl_data_rvalid && pci_cl_data_rready) begin
          r_addr_d = r_addr_d + BYTES_PER_WORD;
          r_count_d = r_count_d + 1'b1;
        end
      end
    endcase
  end

  always @(posedge clk)
  begin
    if (reset)
      imem_state_q <= IMEM_IDLE;
    else
      imem_state_q <= imem_state_d;
  end

  always @(posedge clk)
  begin
    if (reset)
      w_addr_q <= 0;
    else
      w_addr_q <= w_addr_d;
  end

  always @(posedge clk)
  begin
    if (reset)
      r_addr_q <= 0;
    else
      r_addr_q <= r_addr_d;
  end

  always @(posedge clk)
  begin
    if (reset)
      r_count_q <= 0;
    else
      r_count_q <= r_count_d;
  end

  always @(posedge clk)
  begin
    if (reset)
      r_count_max_q <= 0;
    else
      r_count_max_q <= r_count_max_d;
  end
//=============================================================

//=============================================================
// Dual port ram
//=============================================================
//  reg  [ DATA_WIDTH -1 : 0 ] mem [ 0 : 1<<ADDR_WIDTH ];
//  reg  [ DATA_WIDTH -1 : 0 ] mem_copy [ 0 : 1<<ADDR_WIDTH ];
//
//  always @(posedge clk)
//  begin: RAM_WRITE_PORT_A
//    if (s_req_a) begin
//      if (s_wr_en_a) begin
//        mem[s_write_addr_a] <= s_write_data_a;
//        mem_copy[s_write_addr_a] <= s_write_data_a;
//      end
//      else
//        _s_read_data_a <= mem[s_read_addr_a];
//    end
//  end
//
//  always @(posedge clk)
//  begin: RAM_WRITE_PORT_B
//    if (s_read_req_b) begin
//        _s_read_data_b <= mem_copy[s_read_addr_b];
//    end
//  end



  ram #(
    .ADDR_WIDTH                     ( ADDR_WIDTH),
    .DATA_WIDTH                     ( DATA_WIDTH)
  ) u_ram1(
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .s_write_addr                   ( s_write_addr_a),
    .s_write_req                    ( s_wr_en_a),
    .s_write_data                   ( s_write_data_a),
    .s_read_addr                    ( s_read_addr_a),
    .s_read_req                     ( s_req_a),
    .s_read_data                    ( s_read_data_a)
  );

  ram #(
    .ADDR_WIDTH                     ( ADDR_WIDTH),
    .DATA_WIDTH                     ( DATA_WIDTH)
  ) u_ram2(
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .s_write_addr                   ( s_write_addr_a),
    .s_write_req                    ( s_wr_en_a),
    .s_write_data                   ( s_write_data_a),
    .s_read_addr                    ( s_read_addr_b),
    .s_read_req                     ( s_read_req_b),
    .s_read_data                    ( s_read_data_b)
  );

//=============================================================


//=============================================================
// VCD
//=============================================================
`ifdef COCOTB_TOPLEVEL_instruction_memory
  initial begin
    $dumpfile("instruction_memory.vcd");
    $dumpvars(0, instruction_memory);
  end
`endif
//=============================================================
endmodule
`timescale 1ns/1ps
module obuf_bias_sel_logic #(
    parameter integer  LOOP_ID_W                    = 5,
    parameter integer  ADDR_STRIDE_W                = 16
) (
    input  wire                                         clk,
    input  wire                                         reset,
  // Handshake
    input  wire                                         start,
    input  wire                                         done,
    input  wire  [ ADDR_STRIDE_W        -1 : 0 ]        obuf_stride,
    input  wire                                         obuf_stride_v,
    input  wire                                         loop_last_iter,
    input  wire                                         loop_stall,
    input  wire                                         loop_enter,
    input  wire                                         loop_exit,
    input  wire                                         loop_index_valid,
    input  wire  [ LOOP_ID_W            -1 : 0 ]        loop_index,
    output wire                                         bias_prev_sw,
    output wire                                         ddr_pe_sw
);


//=============================================================
// Wires & Regs
//=============================================================
  // Logic to track if a loop variable has dependency on obuf address or not
    //  When the address for obuf depends on the loop variable, it inherits the
    //  modified status from the previous loop as we enter the current loop
    //  Otherwise, the current loop's modified status is changed to modified
    //  upon exit
    reg  [ LOOP_ID_W            -1 : 0 ]        loop_id;

  // Store if current loop variable has a dependency to obuf
    reg obuf_loop_dep [(1<<LOOP_ID_W)-1:0];
  // Store if current loop variable should use obuf for bias
    reg bias_obuf_status [(1<<LOOP_ID_W)-1:0];
  // Store if current loop variable should be stored in ddr or pe
//    reg ddr_pe_status [(1<<LOOP_ID_W)-1:0];
//=============================================================

//=============================================================
// FSM
//=============================================================
    reg  [ 2                    -1 : 0 ]        state_d;
    reg  [ 2                    -1 : 0 ]        state_q;
    wire [ 2                    -1 : 0 ]        state;

    localparam integer  ST_IDLE                      = 0;
    localparam integer  ST_BUSY                      = 1;

  always @(*)
  begin
    state_d = state_q;
    case(state_q)
      ST_IDLE: begin
        if (start)
          state_d = ST_BUSY;
      end
      ST_BUSY: begin
        if (done)
          state_d = ST_IDLE;
      end
    endcase
  end

    assign state = state_q;
  always @(posedge clk)
  begin
    if (reset)
      state_q <= ST_IDLE;
    else
      state_q <= state_d;
  end
//=============================================================

//=============================================================
// Main logic
//=============================================================
  always @(posedge clk)
  begin
    if (reset)
      loop_id <= 1'b0;
    else begin
      if (done)
        loop_id <= 1'b0;
      else if (obuf_stride_v)
        loop_id <= loop_id + 1'b1;
    end
  end

    wire                                        curr_loop_dep;
    reg                                         curr_loop_dep_r;

  always @(posedge clk)
  begin
    if (obuf_stride_v)
      obuf_loop_dep[loop_id] <= obuf_stride != 0;
    curr_loop_dep_r <= obuf_loop_dep[loop_index];
  end
  assign curr_loop_dep = curr_loop_dep_r;

    reg                                         prev_bias_status;
    wire                                        curr_bias_status;
    reg                                         curr_bias_status_r;

    wire                                        curr_ddr_status;
    reg                                         prev_ddr_status;


    reg                                         loop_exit_dly;
    reg                                         loop_enter_dly;
  always @(posedge clk)
    loop_exit_dly <= loop_exit;

  always @(posedge clk)
    loop_enter_dly <= loop_enter;

  always @(posedge clk)
  begin
    if (reset)
      prev_bias_status <= 1'b0;
    else begin
      if (state != ST_BUSY)
        prev_bias_status <= 1'b0;
      else begin
        if (loop_enter && loop_exit_dly)
          prev_bias_status <= curr_bias_status;
        else if (loop_index_valid && ~loop_stall && ~curr_loop_dep)
          prev_bias_status <= 1'b1;
      end
    end
  end

  //Replacing the code for bias_obuf_status
  wire we_;
  reg [ LOOP_ID_W            -1 : 0 ]        waddr_;
  reg wdata_;
  assign we_ = (state==ST_IDLE) || loop_enter_dly || (loop_exit && ~curr_loop_dep);

  always @(posedge clk)
  begin
    if (we_) 
      bias_obuf_status[waddr_] <= wdata_;
    curr_bias_status_r <= bias_obuf_status[loop_index];
  end

  assign  curr_bias_status = curr_bias_status_r;

  always @(posedge clk)
  begin
    if (state == ST_IDLE) begin
      //bias_obuf_status[loop_id] <= 1'b0;
      waddr_ <= loop_id;
      wdata_ <= 1'b0;
    end
    else begin
      if (loop_enter_dly) begin
        //bias_obuf_status[loop_index] <= prev_bias_status;
        waddr_ <= loop_index;
        wdata_ <= prev_bias_status;
      end
      else if (loop_exit && ~curr_loop_dep) begin
        //bias_obuf_status[loop_index] <= 1'b1;
        waddr_ <= loop_index;
        wdata_ <= 1'b1;
      end
    end
  end

  //always @(posedge clk)
  //begin
  //  if (state == ST_IDLE) begin
  //    bias_obuf_status[loop_id] <= 1'b0;
  //  end
  //  else begin
  //    if (loop_enter_dly) begin
  //      bias_obuf_status[loop_index] <= prev_bias_status;
  //    end
  //    else if (loop_exit && ~curr_loop_dep) begin
  //      bias_obuf_status[loop_index] <= 1'b1;
  //    end
  //  end
  //end

    localparam integer  WR_DDR                       = 0;
    localparam integer  WR_PE                        = 1;

  always @(posedge clk)
  begin
    if (reset)
      prev_ddr_status <= WR_PE;
    else begin
      if (state != ST_BUSY)
        prev_ddr_status <= WR_PE;
      else begin
        if ((loop_enter || loop_index_valid) && ~curr_loop_dep)
          prev_ddr_status <= loop_last_iter ? WR_PE : WR_DDR;
      end
    end
  end

  
    //assign curr_bias_status = bias_obuf_status[loop_index];
    //assign curr_loop_dep = obuf_loop_dep[loop_index];

  wire loop_0_dep = obuf_loop_dep[0];
  wire loop_1_dep = obuf_loop_dep[1];
  wire loop_2_dep = obuf_loop_dep[2];
  wire loop_3_dep = obuf_loop_dep[3];
  wire loop_4_dep = obuf_loop_dep[4];
  wire loop_5_dep = obuf_loop_dep[5];
  wire loop_6_dep = obuf_loop_dep[6];

    assign bias_prev_sw = prev_bias_status;
    assign ddr_pe_sw = prev_ddr_status;
//=============================================================

//=============================================================
// VCD
//=============================================================
  `ifdef COCOTB_TOPLEVEL_obuf_bias_sel_logic
    initial begin
      $dumpfile("obuf_bias_sel_logic.vcd");
      $dumpvars(0, obuf_bias_sel_logic);
    end
  `endif
//=============================================================

endmodule
//
// DnnWeaver2 controller
//
// Hardik Sharma
// (hsharma@gatech.edu)

`timescale 1ns/1ps
module controller #(
    parameter integer  NUM_TAGS                     = 2,
    parameter integer  TAG_W                        = $clog2(NUM_TAGS),
    parameter integer  ADDR_WIDTH                   = 42,
    parameter integer  IBUF_ADDR_WIDTH              = ADDR_WIDTH,
    parameter integer  WBUF_ADDR_WIDTH              = ADDR_WIDTH,
    parameter integer  OBUF_ADDR_WIDTH              = ADDR_WIDTH,
    parameter integer  BBUF_ADDR_WIDTH              = ADDR_WIDTH,
  // Instructions
    parameter integer  INST_DATA_WIDTH              = 32,
    parameter integer  INST_ADDR_WIDTH              = 32,
    parameter integer  INST_WSTRB_WIDTH             = INST_DATA_WIDTH/8,
    parameter integer  INST_BURST_WIDTH             = 8,
  // Decoder
    parameter integer  BUF_TYPE_W                   = 2,
    parameter integer  LOOP_ITER_W                  = 16,
    parameter integer  ADDR_STRIDE_W                = 32,
    parameter integer  MEM_REQ_W                    = 16,
    parameter integer  LOOP_ID_W                    = 5,
  // AXI-Lite
    parameter integer  CTRL_ADDR_WIDTH              = 32,
    parameter integer  CTRL_DATA_WIDTH              = 32,
    parameter integer  CTRL_WSTRB_WIDTH             = CTRL_DATA_WIDTH/8,
  // AXI
    parameter integer  AXI_BURST_WIDTH              = 8,
  // Instruction Mem
    parameter integer  IMEM_ADDR_WIDTH              = 8
) (
    input  wire                                         clk,
    input  wire                                         reset,

  // controller <-> compute handshakes
    output wire                                         tag_flush,
    output wire                                         tag_req,
    output wire                                         ibuf_tag_reuse,
    output wire                                         obuf_tag_reuse,
    output wire                                         wbuf_tag_reuse,
    output wire                                         bias_tag_reuse,
    input  wire                                         tag_ready,
    input  wire                                         ibuf_tag_done,
    input  wire                                         wbuf_tag_done,
    input  wire                                         obuf_tag_done,
    input  wire                                         bias_tag_done,

    input  wire                                         compute_done,
    input  wire                                         pu_compute_done,
    input  wire                                         pu_write_done,
    input  wire                                         pu_compute_start,
    input  wire  [ 3                    -1 : 0 ]        pu_ctrl_state,
    input  wire  [ 4                    -1 : 0 ]        stmem_state,
    input  wire  [ TAG_W                -1 : 0 ]        stmem_tag,
    input  wire                                         stmem_ddr_pe_sw,
    input  wire                                         ld_obuf_req,
    input  wire                                         ld_obuf_ready,

  // Load/Store addresses
    // Bias load address
    output wire  [ BBUF_ADDR_WIDTH      -1 : 0 ]        bias_ld_addr,
    output wire                                         bias_ld_addr_v,
    // IBUF load address
    output wire  [ IBUF_ADDR_WIDTH      -1 : 0 ]        ibuf_ld_addr,
    output wire                                         ibuf_ld_addr_v,
    // WBUF load address
    output wire  [ WBUF_ADDR_WIDTH      -1 : 0 ]        wbuf_ld_addr,
    output wire                                         wbuf_ld_addr_v,
    // OBUF load/store address
    output wire  [ OBUF_ADDR_WIDTH      -1 : 0 ]        obuf_ld_addr,
    output wire                                         obuf_ld_addr_v,
    output wire  [ OBUF_ADDR_WIDTH      -1 : 0 ]        obuf_st_addr,
    output wire                                         obuf_st_addr_v,

  // Load bias or obuf
    output wire                                         tag_bias_prev_sw,
    output wire                                         tag_ddr_pe_sw,

  // PCIe -> CL_wrapper AXI4-Lite interface
    // Slave Write address
    input  wire                                         pci_cl_ctrl_awvalid,
    input  wire  [ CTRL_ADDR_WIDTH      -1 : 0 ]        pci_cl_ctrl_awaddr,
    output wire                                         pci_cl_ctrl_awready,
    // Slave Write data
    input  wire                                         pci_cl_ctrl_wvalid,
    input  wire  [ CTRL_DATA_WIDTH      -1 : 0 ]        pci_cl_ctrl_wdata,
    input  wire  [ CTRL_WSTRB_WIDTH     -1 : 0 ]        pci_cl_ctrl_wstrb,
    output wire                                         pci_cl_ctrl_wready,
    // Slave Write response
    output wire                                         pci_cl_ctrl_bvalid,
    output wire  [ 2                    -1 : 0 ]        pci_cl_ctrl_bresp,
    input  wire                                         pci_cl_ctrl_bready,
    // Slave Read address
    input  wire                                         pci_cl_ctrl_arvalid,
    input  wire  [ CTRL_ADDR_WIDTH      -1 : 0 ]        pci_cl_ctrl_araddr,
    output wire                                         pci_cl_ctrl_arready,
    // Slave Read data/response
    output wire                                         pci_cl_ctrl_rvalid,
    output wire  [ CTRL_DATA_WIDTH      -1 : 0 ]        pci_cl_ctrl_rdata,
    output wire  [ 2                    -1 : 0 ]        pci_cl_ctrl_rresp,
    input  wire                                         pci_cl_ctrl_rready,

  // PCIe -> CL_wrapper AXI4 interface
    // Slave Interface Write Address
    input  wire  [ INST_ADDR_WIDTH      -1 : 0 ]        pci_cl_data_awaddr,
    input  wire  [ INST_BURST_WIDTH     -1 : 0 ]        pci_cl_data_awlen,
    input  wire  [ 3                    -1 : 0 ]        pci_cl_data_awsize,
    input  wire  [ 2                    -1 : 0 ]        pci_cl_data_awburst,
    input  wire                                         pci_cl_data_awvalid,
    output wire                                         pci_cl_data_awready,
    // Slave Interface Write Data
    input  wire  [ INST_DATA_WIDTH      -1 : 0 ]        pci_cl_data_wdata,
    input  wire  [ INST_WSTRB_WIDTH     -1 : 0 ]        pci_cl_data_wstrb,
    input  wire                                         pci_cl_data_wlast,
    input  wire                                         pci_cl_data_wvalid,
    output wire                                         pci_cl_data_wready,
    // Slave Interface Write Response
    output wire  [ 2                    -1 : 0 ]        pci_cl_data_bresp,
    output wire                                         pci_cl_data_bvalid,
    input  wire                                         pci_cl_data_bready,
    // Slave Interface Read Address
    input  wire  [ INST_ADDR_WIDTH      -1 : 0 ]        pci_cl_data_araddr,
    input  wire  [ INST_BURST_WIDTH     -1 : 0 ]        pci_cl_data_arlen,
    input  wire  [ 3                    -1 : 0 ]        pci_cl_data_arsize,
    input  wire  [ 2                    -1 : 0 ]        pci_cl_data_arburst,
    input  wire                                         pci_cl_data_arvalid,
    output wire                                         pci_cl_data_arready,
    // Slave Interface Read Data
    output wire  [ INST_DATA_WIDTH      -1 : 0 ]        pci_cl_data_rdata,
    output wire  [ 2                    -1 : 0 ]        pci_cl_data_rresp,
    output wire                                         pci_cl_data_rlast,
    output wire                                         pci_cl_data_rvalid,
    input  wire                                         pci_cl_data_rready,

    input  wire                                         ibuf_compute_ready,
    input  wire                                         wbuf_compute_ready,
    input  wire                                         obuf_compute_ready,
    input  wire                                         bias_compute_ready,

  // Programming interface
    // Loop iterations
    output wire  [ LOOP_ITER_W          -1 : 0 ]        cfg_loop_iter,
    output wire  [ LOOP_ID_W            -1 : 0 ]        cfg_loop_iter_loop_id,
    output wire                                         cfg_loop_iter_v,
    // Loop stride
    output wire  [ ADDR_STRIDE_W        -1 : 0 ]        cfg_loop_stride,
    output wire                                         cfg_loop_stride_v,
    output wire  [ BUF_TYPE_W           -1 : 0 ]        cfg_loop_stride_id,
    output wire  [ 2                    -1 : 0 ]        cfg_loop_stride_type,
    output wire  [ LOOP_ID_W            -1 : 0 ]        cfg_loop_stride_loop_id,
    // Memory request
    output wire  [ MEM_REQ_W            -1 : 0 ]        cfg_mem_req_size,
    output wire                                         cfg_mem_req_v,
    output wire  [ 2                    -1 : 0 ]        cfg_mem_req_type,
    output wire  [ BUF_TYPE_W           -1 : 0 ]        cfg_mem_req_id,
    output wire  [ LOOP_ID_W            -1 : 0 ]        cfg_mem_req_loop_id,
    // Buffer request
    output wire  [ MEM_REQ_W            -1 : 0 ]        cfg_buf_req_size,
    output wire                                         cfg_buf_req_v,
    output wire                                         cfg_buf_req_type,
    output wire  [ BUF_TYPE_W           -1 : 0 ]        cfg_buf_req_loop_id,

    output wire                                         cfg_pu_inst_v,
    output wire  [ INST_DATA_WIDTH      -1 : 0 ]        cfg_pu_inst,
    output wire                                         pu_block_start,

  // Snoop CL DDR0
    // AR channel
    input  wire  [ CTRL_ADDR_WIDTH      -1 : 0 ]        snoop_cl_ddr0_araddr,
    input  wire                                         snoop_cl_ddr0_arvalid,
    input  wire                                         snoop_cl_ddr0_arready,
    input  wire  [ AXI_BURST_WIDTH      -1 : 0 ]        snoop_cl_ddr0_arlen,
    // R channel
    input  wire                                         snoop_cl_ddr0_rvalid,
    input  wire                                         snoop_cl_ddr0_rready,

  // Snoop CL DDR1
    // AW channel
    input  wire  [ CTRL_ADDR_WIDTH      -1 : 0 ]        snoop_cl_ddr1_awaddr,
    input  wire                                         snoop_cl_ddr1_awvalid,
    input  wire                                         snoop_cl_ddr1_awready,
    input  wire  [ AXI_BURST_WIDTH      -1 : 0 ]        snoop_cl_ddr1_awlen,
    // AR channel
    input  wire  [ CTRL_ADDR_WIDTH      -1 : 0 ]        snoop_cl_ddr1_araddr,
    input  wire                                         snoop_cl_ddr1_arvalid,
    input  wire                                         snoop_cl_ddr1_arready,
    input  wire  [ AXI_BURST_WIDTH      -1 : 0 ]        snoop_cl_ddr1_arlen,
    // W channel
    input  wire                                         snoop_cl_ddr1_wvalid,
    input  wire                                         snoop_cl_ddr1_wready,
    // R channel
    input  wire                                         snoop_cl_ddr1_rvalid,
    input  wire                                         snoop_cl_ddr1_rready,

  // Snoop CL DDR2
    // AR channel
    input  wire  [ CTRL_ADDR_WIDTH      -1 : 0 ]        snoop_cl_ddr2_araddr,
    input  wire                                         snoop_cl_ddr2_arvalid,
    input  wire                                         snoop_cl_ddr2_arready,
    input  wire  [ AXI_BURST_WIDTH      -1 : 0 ]        snoop_cl_ddr2_arlen,
    // R channel
    input  wire                                         snoop_cl_ddr2_rvalid,
    input  wire                                         snoop_cl_ddr2_rready,

  // Snoop CL DDR3
    // AR channel
    input  wire  [ CTRL_ADDR_WIDTH      -1 : 0 ]        snoop_cl_ddr3_araddr,
    input  wire                                         snoop_cl_ddr3_arvalid,
    input  wire                                         snoop_cl_ddr3_arready,
    input  wire  [ AXI_BURST_WIDTH      -1 : 0 ]        snoop_cl_ddr3_arlen,
    // R channel
    input  wire                                         snoop_cl_ddr3_rvalid,
    input  wire                                         snoop_cl_ddr3_rready,

  // Snoop CL DDR4
    // AW channel
    input  wire  [ CTRL_ADDR_WIDTH      -1 : 0 ]        snoop_cl_ddr4_awaddr,
    input  wire                                         snoop_cl_ddr4_awvalid,
    input  wire                                         snoop_cl_ddr4_awready,
    input  wire  [ AXI_BURST_WIDTH      -1 : 0 ]        snoop_cl_ddr4_awlen,
    // AR channel
    input  wire  [ CTRL_ADDR_WIDTH      -1 : 0 ]        snoop_cl_ddr4_araddr,
    input  wire                                         snoop_cl_ddr4_arvalid,
    input  wire                                         snoop_cl_ddr4_arready,
    input  wire  [ AXI_BURST_WIDTH      -1 : 0 ]        snoop_cl_ddr4_arlen,
    // W channel
    input  wire                                         snoop_cl_ddr4_wvalid,
    input  wire                                         snoop_cl_ddr4_wready,
    // R channel
    input  wire                                         snoop_cl_ddr4_rvalid,
    input  wire                                         snoop_cl_ddr4_rready,

    input  wire  [ INST_DATA_WIDTH      -1 : 0 ]        obuf_ld_stream_read_count,
    input  wire  [ INST_DATA_WIDTH      -1 : 0 ]        obuf_ld_stream_write_count,
    input  wire  [ INST_DATA_WIDTH      -1 : 0 ]        ddr_st_stream_read_count,
    input  wire  [ INST_DATA_WIDTH      -1 : 0 ]        ddr_st_stream_write_count,
    input  wire  [ INST_DATA_WIDTH      -1 : 0 ]        ld0_stream_counts,
    input  wire  [ INST_DATA_WIDTH      -1 : 0 ]        ld1_stream_counts,
    input  wire  [ INST_DATA_WIDTH      -1 : 0 ]        axi_wr_fifo_counts
  );

//=============================================================
// Localparam
//=============================================================
  // DnnWeaver2 controller state
    localparam integer  IDLE                         = 0;
    localparam integer  DECODE                       = 1;
    localparam integer  BASE_LOOP                    = 2;
    localparam integer  MEM_WAIT                     = 3;
    localparam integer  PU_WR_WAIT                   = 4;
    localparam integer  BLOCK_DONE                   = 5;
    localparam integer  DONE                         = 6;

    localparam integer  TM_STATE_WIDTH               = 2;
    localparam integer  TM_IDLE                      = 0;
    localparam integer  TM_REQUEST                   = 1;
    localparam integer  TM_CHECK                     = 2;
    localparam integer  TM_FLUSH                     = 3;
//=============================================================

//=============================================================
// Wires/Regs
//=============================================================
    wire                                        last_block;
  // --------Debug--------
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        pmon_busy_cycles;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        pmon_decode_cycles;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        pmon_execute_cycles;

    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        pmon_block_started;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        pmon_block_finished;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        pmon_tag_started;

    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        pmon_axi_wr_id;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        pmon_axi_write_req;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        pmon_axi_write_finished;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        pmon_axi_read_req;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        pmon_axi_read_finished;

    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        pmon_cl_ddr0_read_req;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        pmon_cl_ddr0_read_finished;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        pmon_cl_ddr1_write_req;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        pmon_cl_ddr1_write_finished;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        pmon_cl_ddr1_read_req;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        pmon_cl_ddr1_read_finished;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        pmon_cl_ddr2_read_req;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        pmon_cl_ddr2_read_finished;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        pmon_cl_ddr3_read_req;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        pmon_cl_ddr3_read_finished;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        pmon_cl_ddr4_write_req;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        pmon_cl_ddr4_write_finished;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        pmon_cl_ddr4_read_req;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        pmon_cl_ddr4_read_finished;

  // --------Debug--------

  // DnnWeaver2 states
    reg  [ 3                    -1 : 0 ]        dnnweaver2_state_d;
    reg  [ 3                    -1 : 0 ]        dnnweaver2_state_q;
    wire [ 3                    -1 : 0 ]        dnnweaver2_state;

  // Base addresses
    wire [ IBUF_ADDR_WIDTH      -1 : 0 ]        ibuf_base_addr;
    wire [ IBUF_ADDR_WIDTH      -1 : 0 ]        wbuf_base_addr;
    wire [ IBUF_ADDR_WIDTH      -1 : 0 ]        obuf_base_addr;
    wire [ IBUF_ADDR_WIDTH      -1 : 0 ]        bias_base_addr;

  // Handshake signals for main loop controller
    wire                                        base_loop_ctrl_start;
    wire                                        base_loop_ctrl_done;

    wire                                        block_done;
    wire                                        dnnweaver2_done;

  // Handshake signals for decoder
    wire                                        decoder_start;
    wire                                        decoder_done;

  // Instruction memory Read Port - Decoder
    wire                                        inst_read_req;
    wire [ IMEM_ADDR_WIDTH      -1 : 0 ]        inst_read_addr;
    wire [ INST_DATA_WIDTH      -1 : 0 ]        inst_read_data;

  // Start address for fetching/decoding instruction block
    wire [ IMEM_ADDR_WIDTH      -1 : 0 ]        num_blocks;

  // resetn for axi slave
    wire                                        resetn;

  // Slave registers
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg0_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg0_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg1_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg1_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg2_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg2_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg3_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg3_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg4_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg4_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg5_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg5_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg6_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg6_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg7_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg7_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg8_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg8_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg9_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg9_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg10_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg10_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg11_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg11_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg12_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg12_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg13_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg13_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg14_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg14_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg15_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg15_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg16_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg16_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg17_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg17_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg18_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg18_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg19_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg19_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg20_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg20_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg21_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg21_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg22_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg22_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg23_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg23_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg24_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg24_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg25_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg25_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg26_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg26_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg27_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg27_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg28_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg28_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg29_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg29_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg30_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg30_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg31_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg31_out;
  // Slave registers end

  // Accelerator start logic
    wire                                        start_bit_d;
    reg                                         start_bit_q;

  // TM State
    reg  [ TM_STATE_WIDTH       -1 : 0 ]        tm_state_d;
    reg  [ TM_STATE_WIDTH       -1 : 0 ]        tm_state_q;

    reg                                         tm_ibuf_tag_reuse_d;
    reg                                         tm_ibuf_tag_reuse_q;
    reg                                         tm_obuf_tag_reuse_d;
    reg                                         tm_obuf_tag_reuse_q;
    reg                                         tm_wbuf_tag_reuse_d;
    reg                                         tm_wbuf_tag_reuse_q;
    reg                                         tm_bias_tag_reuse_d;
    reg                                         tm_bias_tag_reuse_q;

    reg  [ ADDR_WIDTH           -1 : 0 ]        tm_ibuf_tag_addr_d;
    reg  [ ADDR_WIDTH           -1 : 0 ]        tm_ibuf_tag_addr_q;
    reg  [ ADDR_WIDTH           -1 : 0 ]        tm_obuf_tag_addr_d;
    reg  [ ADDR_WIDTH           -1 : 0 ]        tm_obuf_tag_addr_q;
    reg  [ ADDR_WIDTH           -1 : 0 ]        tm_wbuf_tag_addr_d;
    reg  [ ADDR_WIDTH           -1 : 0 ]        tm_wbuf_tag_addr_q;
    reg  [ ADDR_WIDTH           -1 : 0 ]        tm_bias_tag_addr_d;
    reg  [ ADDR_WIDTH           -1 : 0 ]        tm_bias_tag_addr_q;

    wire                                        base_ctrl_tag_req;
    wire                                        base_ctrl_tag_ready;

    assign tag_req = tm_state_q == TM_REQUEST;

  always @(posedge clk)
  begin
    if(reset)
      tm_state_q <= TM_IDLE;
    else
      tm_state_q <= tm_state_d;
  end

  always @(posedge clk)
  begin
    if(reset) begin
      tm_ibuf_tag_reuse_q <= 1'b0;
      tm_obuf_tag_reuse_q <= 1'b0;
      tm_wbuf_tag_reuse_q <= 1'b0;
      tm_bias_tag_reuse_q <= 1'b0;
      tm_ibuf_tag_addr_q  <= 0;
      tm_obuf_tag_addr_q  <= 0;
      tm_wbuf_tag_addr_q  <= 0;
      tm_bias_tag_addr_q  <= 0;
    end else begin
      tm_ibuf_tag_reuse_q <= tm_ibuf_tag_reuse_d;
      tm_obuf_tag_reuse_q <= tm_obuf_tag_reuse_d;
      tm_wbuf_tag_reuse_q <= tm_wbuf_tag_reuse_d;
      tm_bias_tag_reuse_q <= tm_bias_tag_reuse_d;
      tm_ibuf_tag_addr_q  <= tm_ibuf_tag_addr_d;
      tm_obuf_tag_addr_q  <= tm_obuf_tag_addr_d;
      tm_wbuf_tag_addr_q  <= tm_wbuf_tag_addr_d;
      tm_bias_tag_addr_q  <= tm_bias_tag_addr_d;
    end
  end

  always @(*)
  begin
  
    tm_state_d = tm_state_q;
    
    tm_ibuf_tag_reuse_d = 1'b0;
    tm_obuf_tag_reuse_d = 1'b0;
    tm_wbuf_tag_reuse_d = 1'b0;
    tm_bias_tag_reuse_d = 1'b0;
    
    tm_ibuf_tag_addr_d = tm_ibuf_tag_addr_q;
    tm_obuf_tag_addr_d = tm_obuf_tag_addr_q;
    tm_wbuf_tag_addr_d = tm_wbuf_tag_addr_q;
    tm_bias_tag_addr_d = tm_bias_tag_addr_q;
    
    case(tm_state_q)
      TM_IDLE: begin
        if (base_ctrl_tag_req && tag_ready)
          tm_state_d = TM_REQUEST;
      end
      TM_REQUEST: begin
        if (tag_ready) begin
          tm_state_d = TM_CHECK;
          tm_ibuf_tag_addr_d = ibuf_ld_addr;
          tm_obuf_tag_addr_d = obuf_ld_addr;
          tm_wbuf_tag_addr_d = wbuf_ld_addr;
          tm_bias_tag_addr_d = bias_ld_addr;
        end
      end
      TM_CHECK: begin
        if (base_ctrl_tag_req && tag_ready) begin
          tm_state_d = TM_REQUEST;
          tm_ibuf_tag_reuse_d = tm_ibuf_tag_addr_q == ibuf_ld_addr;
          tm_obuf_tag_reuse_d = tm_obuf_tag_addr_q == obuf_ld_addr;
          tm_wbuf_tag_reuse_d = tm_wbuf_tag_addr_q == wbuf_ld_addr;
          tm_bias_tag_reuse_d = tm_bias_tag_addr_q == bias_ld_addr;
        end
        else if (dnnweaver2_state_q == MEM_WAIT)
        begin
          tm_state_d = TM_FLUSH;
        end
      end
      TM_FLUSH: begin
        tm_state_d = TM_IDLE;
      end
    endcase
  end

  assign tag_flush = tm_state_q == TM_FLUSH;

    assign ibuf_tag_reuse = tm_ibuf_tag_reuse_q;
    assign obuf_tag_reuse = tm_obuf_tag_reuse_q;
    assign wbuf_tag_reuse = tm_wbuf_tag_reuse_q;
    assign bias_tag_reuse = tm_bias_tag_reuse_q;

    assign base_ctrl_tag_ready = tag_ready && tm_state_q == TM_REQUEST;
//=============================================================

//=============================================================
// Accelerator Start logic
//=============================================================
  always @(posedge clk)
  begin
    if (reset)
      start_bit_q <= 1'b0;
    else
      start_bit_q <= start_bit_d;
  end
//=============================================================

//=============================================================
// FSM
//=============================================================
  always @(posedge clk)
  begin
    if (reset) begin
      dnnweaver2_state_q <= IDLE;
    end
    else begin
      dnnweaver2_state_q <= dnnweaver2_state_d;
    end
  end

  always @(*)
  begin
    dnnweaver2_state_d = dnnweaver2_state_q;
    case(dnnweaver2_state_q)
      IDLE: begin
        if (decoder_start) begin
          dnnweaver2_state_d = DECODE;
        end
      end
      DECODE: begin
        if (base_loop_ctrl_start)
          dnnweaver2_state_d = BASE_LOOP;
      end
      BASE_LOOP: begin
        if (base_loop_ctrl_done)
          dnnweaver2_state_d = MEM_WAIT;
      end
      MEM_WAIT: begin
        if (ibuf_tag_done && wbuf_tag_done && obuf_tag_done && bias_tag_done)
          dnnweaver2_state_d = PU_WR_WAIT;
      end
      PU_WR_WAIT: begin
        if (pu_write_done) begin
          dnnweaver2_state_d = BLOCK_DONE;
        end
      end
      BLOCK_DONE: begin
        if (~last_block)
          dnnweaver2_state_d = DECODE;
        else
          dnnweaver2_state_d = DONE;
      end
      DONE: begin
        dnnweaver2_state_d = IDLE;
      end
    endcase
  end
    assign block_done = dnnweaver2_state == BLOCK_DONE;
    assign dnnweaver2_done = dnnweaver2_state == DONE;
//=============================================================

//=============================================================
// Debug
//=============================================================
    reg  [ CTRL_DATA_WIDTH      -1 : 0 ]        tag_req_count;
    reg  [ CTRL_DATA_WIDTH      -1 : 0 ]        compute_done_count;
    reg  [ CTRL_DATA_WIDTH      -1 : 0 ]        pu_compute_done_count;
    reg  [ CTRL_DATA_WIDTH      -1 : 0 ]        pu_compute_start_count;
    always @(posedge clk)
    begin
      if (reset)
        tag_req_count <= 0;
      else if (tm_state_q == TM_REQUEST)
        tag_req_count <= tag_req_count + 1'b1;
    end

    always @(posedge clk)
    begin
      if (reset)
        compute_done_count <= 0;
      else if (compute_done)
        compute_done_count <= compute_done_count + 1'b1;
    end

    always @(posedge clk)
    begin
      if (reset)
        pu_compute_done_count <= 0;
      else if (pu_compute_done)
        pu_compute_done_count <= pu_compute_done_count + 1'b1;
    end

    always @(posedge clk)
    begin
      if (reset)
        pu_compute_start_count <= 0;
      else if (pu_compute_start)
        pu_compute_start_count <= pu_compute_start_count + 1'b1;
    end
//=============================================================

//=============================================================
// Assigns
//=============================================================
    assign dnnweaver2_state = dnnweaver2_state_q;

    assign resetn = ~reset;

    assign num_blocks = slv_reg1_out;

    assign start_bit_d = slv_reg0_out[0];
    assign decoder_start = (start_bit_q ^ start_bit_d) && dnnweaver2_state_q == IDLE;

    assign slv_reg0_in = slv_reg0_out; // Used as start trigger
    assign slv_reg1_in = slv_reg1_out; // Used as start address

    assign slv_reg2_in = dnnweaver2_state;
    assign slv_reg3_in = tag_req_count;
    assign slv_reg4_in = compute_done_count;
    assign slv_reg5_in = pu_compute_done_count;
    assign slv_reg6_in = pu_compute_start_count;

    // assign slv_reg3_in = pmon_decode_cycles;
    // assign slv_reg4_in = pmon_execute_cycles;
    // assign slv_reg5_in = pmon_busy_cycles;

    assign slv_reg7_in = {stmem_state, 14'b0, stmem_ddr_pe_sw, stmem_tag};

    assign slv_reg8_in = pmon_axi_write_req;
    assign slv_reg9_in = pmon_axi_write_finished;
    assign slv_reg10_in = pmon_axi_read_req;
    assign slv_reg11_in = pmon_axi_read_finished;
    assign slv_reg12_in = pmon_axi_wr_id;

    assign slv_reg13_in = ld0_stream_counts;
    assign slv_reg14_in = ld1_stream_counts;
    assign slv_reg15_in = axi_wr_fifo_counts;

    assign slv_reg16_in = pmon_cl_ddr0_read_req;
    assign slv_reg17_in = pmon_cl_ddr0_read_finished;

    assign slv_reg18_in = pmon_cl_ddr1_write_req;
    assign slv_reg19_in = pmon_cl_ddr1_write_finished;
    assign slv_reg20_in = pmon_cl_ddr1_read_req;
    assign slv_reg21_in = pmon_cl_ddr1_read_finished;

    assign slv_reg22_in = obuf_ld_stream_read_count;
    assign slv_reg23_in = obuf_ld_stream_write_count;
    assign slv_reg24_in = ddr_st_stream_read_count;
    assign slv_reg25_in = ddr_st_stream_write_count;

    assign slv_reg26_in = pmon_cl_ddr4_write_req;
    assign slv_reg27_in = pmon_cl_ddr4_write_finished;
    assign slv_reg28_in = pmon_cl_ddr4_read_req;
    assign slv_reg29_in = pmon_cl_ddr4_read_finished;

    assign slv_reg30_in = pu_ctrl_state;

    reg  [ CTRL_DATA_WIDTH      -1 : 0 ]        ld_obuf_read_counter;

    always @(posedge clk)
    begin
      if (reset)
        ld_obuf_read_counter <= 0;
      else if (ld_obuf_req)
        ld_obuf_read_counter <= ld_obuf_read_counter + 1'b1;
    end
    assign slv_reg31_in = ld_obuf_read_counter;

//=============================================================

//=============================================================
// Performance monitor
//=============================================================
  performance_monitor #(
    .STATS_WIDTH                    ( CTRL_DATA_WIDTH                )
  ) u_perf_mon (
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .dnnweaver2_state                ( dnnweaver2_state_q              ), //input
    .tag_req                        ( tag_req                        ), //input
    .tag_ready                      ( tag_ready                      ), //input

    .decoder_start                  ( decoder_start                  ), //input

    .ibuf_tag_done                  ( ibuf_tag_done                  ), //input
    .wbuf_tag_done                  ( wbuf_tag_done                  ), //input
    .obuf_tag_done                  ( obuf_tag_done                  ), //input
    .bias_tag_done                  ( bias_tag_done                  ), //input

    .pci_cl_data_awvalid            ( pci_cl_data_awvalid            ), //input
    .pci_cl_data_awlen              ( pci_cl_data_awlen              ), //input
    .pci_cl_data_awready            ( pci_cl_data_awready            ), //input
    .pci_cl_data_arvalid            ( pci_cl_data_arvalid            ), //input
    .pci_cl_data_arlen              ( pci_cl_data_arlen              ), //input
    .pci_cl_data_arready            ( pci_cl_data_arready            ), //input
    .pci_cl_data_wvalid             ( pci_cl_data_wvalid             ), //input
    .pci_cl_data_wready             ( pci_cl_data_wready             ), //input
    .pci_cl_data_rvalid             ( pci_cl_data_rvalid             ), //input
    .pci_cl_data_rready             ( pci_cl_data_rready             ), //input

    .snoop_cl_ddr0_arvalid          ( snoop_cl_ddr0_arvalid          ), //input
    .snoop_cl_ddr0_arready          ( snoop_cl_ddr0_arready          ), //input
    .snoop_cl_ddr0_arlen            ( snoop_cl_ddr0_arlen            ), //input
    .snoop_cl_ddr0_rvalid           ( snoop_cl_ddr0_rvalid           ), //input
    .snoop_cl_ddr0_rready           ( snoop_cl_ddr0_rready           ), //input

    .snoop_cl_ddr1_awvalid          ( snoop_cl_ddr1_awvalid          ), //input
    .snoop_cl_ddr1_awready          ( snoop_cl_ddr1_awready          ), //input
    .snoop_cl_ddr1_awlen            ( snoop_cl_ddr1_awlen            ), //input
    .snoop_cl_ddr1_arvalid          ( snoop_cl_ddr1_arvalid          ), //input
    .snoop_cl_ddr1_arready          ( snoop_cl_ddr1_arready          ), //input
    .snoop_cl_ddr1_arlen            ( snoop_cl_ddr1_arlen            ), //input
    .snoop_cl_ddr1_wvalid           ( snoop_cl_ddr1_wvalid           ), //input
    .snoop_cl_ddr1_wready           ( snoop_cl_ddr1_wready           ), //input
    .snoop_cl_ddr1_rvalid           ( snoop_cl_ddr1_rvalid           ), //input
    .snoop_cl_ddr1_rready           ( snoop_cl_ddr1_rready           ), //input

    .snoop_cl_ddr2_arvalid          ( snoop_cl_ddr2_arvalid          ), //input
    .snoop_cl_ddr2_arready          ( snoop_cl_ddr2_arready          ), //input
    .snoop_cl_ddr2_arlen            ( snoop_cl_ddr2_arlen            ), //input
    .snoop_cl_ddr2_rvalid           ( snoop_cl_ddr2_rvalid           ), //input
    .snoop_cl_ddr2_rready           ( snoop_cl_ddr2_rready           ), //input

    .snoop_cl_ddr3_arvalid          ( snoop_cl_ddr3_arvalid          ), //input
    .snoop_cl_ddr3_arready          ( snoop_cl_ddr3_arready          ), //input
    .snoop_cl_ddr3_arlen            ( snoop_cl_ddr3_arlen            ), //input
    .snoop_cl_ddr3_rvalid           ( snoop_cl_ddr3_rvalid           ), //input
    .snoop_cl_ddr3_rready           ( snoop_cl_ddr3_rready           ), //input

    .snoop_cl_ddr4_awvalid          ( snoop_cl_ddr4_awvalid          ), //input
    .snoop_cl_ddr4_awready          ( snoop_cl_ddr4_awready          ), //input
    .snoop_cl_ddr4_awlen            ( snoop_cl_ddr4_awlen            ), //input
    .snoop_cl_ddr4_arvalid          ( snoop_cl_ddr4_arvalid          ), //input
    .snoop_cl_ddr4_arready          ( snoop_cl_ddr4_arready          ), //input
    .snoop_cl_ddr4_arlen            ( snoop_cl_ddr4_arlen            ), //input
    .snoop_cl_ddr4_wvalid           ( snoop_cl_ddr4_wvalid           ), //input
    .snoop_cl_ddr4_wready           ( snoop_cl_ddr4_wready           ), //input
    .snoop_cl_ddr4_rvalid           ( snoop_cl_ddr4_rvalid           ), //input
    .snoop_cl_ddr4_rready           ( snoop_cl_ddr4_rready           ), //input

    .pmon_cl_ddr0_read_req          ( pmon_cl_ddr0_read_req          ), //output
    .pmon_cl_ddr0_read_finished     ( pmon_cl_ddr0_read_finished     ), //output
    .pmon_cl_ddr1_write_req         ( pmon_cl_ddr1_write_req         ), //output
    .pmon_cl_ddr1_write_finished    ( pmon_cl_ddr1_write_finished    ), //output
    .pmon_cl_ddr1_read_req          ( pmon_cl_ddr1_read_req          ), //output
    .pmon_cl_ddr1_read_finished     ( pmon_cl_ddr1_read_finished     ), //output
    .pmon_cl_ddr2_read_req          ( pmon_cl_ddr2_read_req          ), //output
    .pmon_cl_ddr2_read_finished     ( pmon_cl_ddr2_read_finished     ), //output
    .pmon_cl_ddr3_read_req          ( pmon_cl_ddr3_read_req          ), //output
    .pmon_cl_ddr3_read_finished     ( pmon_cl_ddr3_read_finished     ), //output
    .pmon_cl_ddr4_write_req         ( pmon_cl_ddr4_write_req         ), //output
    .pmon_cl_ddr4_write_finished    ( pmon_cl_ddr4_write_finished    ), //output
    .pmon_cl_ddr4_read_req          ( pmon_cl_ddr4_read_req          ), //output
    .pmon_cl_ddr4_read_finished     ( pmon_cl_ddr4_read_finished     ), //output

    .decode_cycles                  ( pmon_decode_cycles             ), //output
    .execute_cycles                 ( pmon_execute_cycles            ), //output
    .busy_cycles                    ( pmon_busy_cycles               ), //output

    .tag_started                    ( pmon_tag_started               ), //output
    .block_started                  ( pmon_block_started             ), //output
    .block_finished                 ( pmon_block_finished            ), //output

    .axi_wr_id                      ( pmon_axi_wr_id                 ), //output
    .axi_write_req                  ( pmon_axi_write_req             ), //output
    .axi_write_finished             ( pmon_axi_write_finished        ), //output
    .axi_read_req                   ( pmon_axi_read_req              ), //output
    .axi_read_finished              ( pmon_axi_read_finished         )  //output
  );
//=============================================================

//=============================================================
// Instruction Memory
//=============================================================
  instruction_memory #(
    .DATA_WIDTH                     ( INST_DATA_WIDTH                ),
    .ADDR_WIDTH                     ( IMEM_ADDR_WIDTH                )
  ) imem (
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .pci_cl_data_awaddr             ( pci_cl_data_awaddr             ), //input
    .pci_cl_data_awlen              ( pci_cl_data_awlen              ), //input
    .pci_cl_data_awsize             ( pci_cl_data_awsize             ), //input
    .pci_cl_data_awburst            ( pci_cl_data_awburst            ), //input
    .pci_cl_data_awvalid            ( pci_cl_data_awvalid            ), //input
    .pci_cl_data_awready            ( pci_cl_data_awready            ), //output
    .pci_cl_data_wdata              ( pci_cl_data_wdata              ), //input
    .pci_cl_data_wstrb              ( pci_cl_data_wstrb              ), //input
    .pci_cl_data_wlast              ( pci_cl_data_wlast              ), //input
    .pci_cl_data_wvalid             ( pci_cl_data_wvalid             ), //input
    .pci_cl_data_wready             ( pci_cl_data_wready             ), //output
    .pci_cl_data_bresp              ( pci_cl_data_bresp              ), //output
    .pci_cl_data_bvalid             ( pci_cl_data_bvalid             ), //output
    .pci_cl_data_bready             ( pci_cl_data_bready             ), //input
    .pci_cl_data_araddr             ( pci_cl_data_araddr             ), //input
    .pci_cl_data_arlen              ( pci_cl_data_arlen              ), //input
    .pci_cl_data_arsize             ( pci_cl_data_arsize             ), //input
    .pci_cl_data_arburst            ( pci_cl_data_arburst            ), //input
    .pci_cl_data_arvalid            ( pci_cl_data_arvalid            ), //input
    .pci_cl_data_arready            ( pci_cl_data_arready            ), //output
    .pci_cl_data_rdata              ( pci_cl_data_rdata              ), //output
    .pci_cl_data_rresp              ( pci_cl_data_rresp              ), //output
    .pci_cl_data_rlast              ( pci_cl_data_rlast              ), //output
    .pci_cl_data_rvalid             ( pci_cl_data_rvalid             ), //output
    .pci_cl_data_rready             ( pci_cl_data_rready             ), //input
    .s_read_addr_b                  ( inst_read_addr                 ), //input
    .s_read_req_b                   ( inst_read_req                  ), //input
    .s_read_data_b                  ( inst_read_data                 )  //output
  );
//=============================================================

//=============================================================
// Status/Control AXI4-Lite
//=============================================================
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        ibuf_rd_addr;
    wire                                        ibuf_rd_addr_v;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        obuf_wr_addr;
    wire                                        obuf_wr_addr_v;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        obuf_rd_addr;
    wire                                        obuf_rd_addr_v;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        wbuf_rd_addr;
    wire                                        wbuf_rd_addr_v;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        bias_rd_addr;
    wire                                        bias_rd_addr_v;

    assign ibuf_rd_addr = snoop_cl_ddr0_araddr;
    assign ibuf_rd_addr_v = snoop_cl_ddr0_arvalid && snoop_cl_ddr0_arready;
    assign obuf_wr_addr = snoop_cl_ddr1_awaddr;
    assign obuf_wr_addr_v = snoop_cl_ddr1_awvalid && snoop_cl_ddr1_awready;
    assign obuf_rd_addr = snoop_cl_ddr1_araddr;
    assign obuf_rd_addr_v = snoop_cl_ddr1_arvalid && snoop_cl_ddr1_arready;
    assign wbuf_rd_addr = snoop_cl_ddr2_araddr;
    assign wbuf_rd_addr_v = snoop_cl_ddr2_arvalid && snoop_cl_ddr2_arready;
    assign bias_rd_addr = snoop_cl_ddr3_araddr;
    assign bias_rd_addr_v = snoop_cl_ddr3_arvalid && snoop_cl_ddr3_arready;

  axi4lite_slave #(
    .AXIS_ADDR_WIDTH                ( CTRL_ADDR_WIDTH                ),
    .AXIS_DATA_WIDTH                ( CTRL_DATA_WIDTH                )
  ) status_ctrl_slv   (
    .clk                            ( clk                            ),
    .resetn                         ( resetn                         ),
  // Slave registers
    .slv_reg0_in                    ( slv_reg0_in                    ),
    .slv_reg0_out                   ( slv_reg0_out                   ),
    .slv_reg1_in                    ( slv_reg1_in                    ),
    .slv_reg1_out                   ( slv_reg1_out                   ),
    .slv_reg2_in                    ( slv_reg2_in                    ),
    .slv_reg2_out                   ( slv_reg2_out                   ),
    .slv_reg3_in                    ( slv_reg3_in                    ),
    .slv_reg3_out                   ( slv_reg3_out                   ),
    .slv_reg4_in                    ( slv_reg4_in                    ),
    .slv_reg4_out                   ( slv_reg4_out                   ),
    .slv_reg5_in                    ( slv_reg5_in                    ),
    .slv_reg5_out                   ( slv_reg5_out                   ),
    .slv_reg6_in                    ( slv_reg6_in                    ),
    .slv_reg6_out                   ( slv_reg6_out                   ),
    .slv_reg7_in                    ( slv_reg7_in                    ),
    .slv_reg7_out                   ( slv_reg7_out                   ),
    .slv_reg8_in                    ( slv_reg8_in                    ),
    .slv_reg8_out                   ( slv_reg8_out                   ),
    .slv_reg9_in                    ( slv_reg9_in                    ),
    .slv_reg9_out                   ( slv_reg9_out                   ),
    .slv_reg10_in                   ( slv_reg10_in                   ),
    .slv_reg10_out                  ( slv_reg10_out                  ),
    .slv_reg11_in                   ( slv_reg11_in                   ),
    .slv_reg11_out                  ( slv_reg11_out                  ),
    .slv_reg12_in                   ( slv_reg12_in                   ),
    .slv_reg12_out                  ( slv_reg12_out                  ),
    .slv_reg13_in                   ( slv_reg13_in                   ),
    .slv_reg13_out                  ( slv_reg13_out                  ),
    .slv_reg14_in                   ( slv_reg14_in                   ),
    .slv_reg14_out                  ( slv_reg14_out                  ),
    .slv_reg15_in                   ( slv_reg15_in                   ),
    .slv_reg15_out                  ( slv_reg15_out                  ),

    .slv_reg16_in                   ( slv_reg16_in                   ),
    .slv_reg16_out                  ( slv_reg16_out                  ),
    .slv_reg17_in                   ( slv_reg17_in                   ),
    .slv_reg17_out                  ( slv_reg17_out                  ),
    .slv_reg18_in                   ( slv_reg18_in                   ),
    .slv_reg18_out                  ( slv_reg18_out                  ),
    .slv_reg19_in                   ( slv_reg19_in                   ),
    .slv_reg19_out                  ( slv_reg19_out                  ),
    .slv_reg20_in                   ( slv_reg20_in                   ),
    .slv_reg20_out                  ( slv_reg20_out                  ),
    .slv_reg21_in                   ( slv_reg21_in                   ),
    .slv_reg21_out                  ( slv_reg21_out                  ),
    .slv_reg22_in                   ( slv_reg22_in                   ),
    .slv_reg22_out                  ( slv_reg22_out                  ),
    .slv_reg23_in                   ( slv_reg23_in                   ),
    .slv_reg23_out                  ( slv_reg23_out                  ),
    .slv_reg24_in                   ( slv_reg24_in                   ),
    .slv_reg24_out                  ( slv_reg24_out                  ),
    .slv_reg25_in                   ( slv_reg25_in                   ),
    .slv_reg25_out                  ( slv_reg25_out                  ),
    .slv_reg26_in                   ( slv_reg26_in                   ),
    .slv_reg26_out                  ( slv_reg26_out                  ),
    .slv_reg27_in                   ( slv_reg27_in                   ),
    .slv_reg27_out                  ( slv_reg27_out                  ),
    .slv_reg28_in                   ( slv_reg28_in                   ),
    .slv_reg28_out                  ( slv_reg28_out                  ),
    .slv_reg29_in                   ( slv_reg29_in                   ),
    .slv_reg29_out                  ( slv_reg29_out                  ),
    .slv_reg30_in                   ( slv_reg30_in                   ),
    .slv_reg30_out                  ( slv_reg30_out                  ),
    .slv_reg31_in                   ( slv_reg31_in                   ),
    .slv_reg31_out                  ( slv_reg31_out                  ),

    .decoder_start                  ( decoder_start                  ),
    .ibuf_rd_addr                   ( ibuf_rd_addr                   ),
    .ibuf_rd_addr_v                 ( ibuf_rd_addr_v                 ),
    .obuf_wr_addr                   ( obuf_wr_addr                   ),
    .obuf_wr_addr_v                 ( obuf_wr_addr_v                 ),
    .obuf_rd_addr                   ( obuf_rd_addr                   ),
    .obuf_rd_addr_v                 ( obuf_rd_addr_v                 ),
    .wbuf_rd_addr                   ( wbuf_rd_addr                   ),
    .wbuf_rd_addr_v                 ( wbuf_rd_addr_v                 ),
    .bias_rd_addr                   ( bias_rd_addr                   ),
    .bias_rd_addr_v                 ( bias_rd_addr_v                 ),

    .s_axi_awaddr                   ( pci_cl_ctrl_awaddr             ),
    .s_axi_awvalid                  ( pci_cl_ctrl_awvalid            ),
    .s_axi_awready                  ( pci_cl_ctrl_awready            ),
    .s_axi_wdata                    ( pci_cl_ctrl_wdata              ),
    .s_axi_wstrb                    ( pci_cl_ctrl_wstrb              ),
    .s_axi_wvalid                   ( pci_cl_ctrl_wvalid             ),
    .s_axi_wready                   ( pci_cl_ctrl_wready             ),
    .s_axi_bresp                    ( pci_cl_ctrl_bresp              ),
    .s_axi_bvalid                   ( pci_cl_ctrl_bvalid             ),
    .s_axi_bready                   ( pci_cl_ctrl_bready             ),
    .s_axi_araddr                   ( pci_cl_ctrl_araddr             ),
    .s_axi_arvalid                  ( pci_cl_ctrl_arvalid            ),
    .s_axi_arready                  ( pci_cl_ctrl_arready            ),
    .s_axi_rdata                    ( pci_cl_ctrl_rdata              ),
    .s_axi_rresp                    ( pci_cl_ctrl_rresp              ),
    .s_axi_rvalid                   ( pci_cl_ctrl_rvalid             ),
    .s_axi_rready                   ( pci_cl_ctrl_rready             )
  );
//=============================================================

//=============================================================
// Decoder
//=============================================================
  decoder #(
    .IMEM_ADDR_W                    ( IMEM_ADDR_WIDTH                )
  ) instruction_decoder (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .imem_read_data                 ( inst_read_data                 ), //input
    .imem_read_addr                 ( inst_read_addr                 ), //output
    .imem_read_req                  ( inst_read_req                  ), //output
    .start                          ( decoder_start                  ), //input
    .done                           ( decoder_done                   ), //output
    .loop_ctrl_start                ( base_loop_ctrl_start           ), //output
    .loop_ctrl_done                 ( base_loop_ctrl_done            ), //input
    .block_done                     ( block_done                     ), //input
    .last_block                     ( last_block                     ), //output
    .cfg_loop_iter_v                ( cfg_loop_iter_v                ), //output
    .cfg_loop_iter                  ( cfg_loop_iter                  ), //output
    .cfg_loop_iter_loop_id          ( cfg_loop_iter_loop_id          ), //output
    .cfg_loop_stride_v              ( cfg_loop_stride_v              ), //output
    .cfg_loop_stride                ( cfg_loop_stride                ), //output
    .cfg_loop_stride_loop_id        ( cfg_loop_stride_loop_id        ), //output
    .cfg_loop_stride_type           ( cfg_loop_stride_type           ), //output
    .cfg_loop_stride_id             ( cfg_loop_stride_id             ), //output
    .ibuf_base_addr                 ( ibuf_base_addr                 ), //output
    .wbuf_base_addr                 ( wbuf_base_addr                 ), //output
    .obuf_base_addr                 ( obuf_base_addr                 ), //output
    .bias_base_addr                 ( bias_base_addr                 ), //output
    .cfg_mem_req_v                  ( cfg_mem_req_v                  ), //output
    .cfg_mem_req_size               ( cfg_mem_req_size               ), //output
    .cfg_mem_req_type               ( cfg_mem_req_type               ), //output
    .cfg_mem_req_id                 ( cfg_mem_req_id                 ), //output
    .cfg_mem_req_loop_id            ( cfg_mem_req_loop_id            ), //output
    .cfg_buf_req_v                  ( cfg_buf_req_v                  ), //output
    .cfg_buf_req_size               ( cfg_buf_req_size               ), //output
    .cfg_buf_req_type               ( cfg_buf_req_type               ), //output
    .cfg_buf_req_loop_id            ( cfg_buf_req_loop_id            ), //output
    .cfg_pu_inst                    ( cfg_pu_inst                    ), //output
    .cfg_pu_inst_v                  ( cfg_pu_inst_v                  ), //output
    .pu_block_start                 ( pu_block_start                 )  //output
  );
//=============================================================

//=============================================================
// Base address generator
//    This module is in charge of the outer loops [16 - 31]
//=============================================================
  base_addr_gen #(
    .BASE_ID                        ( 1                              ),
    .MEM_REQ_W                      ( MEM_REQ_W                      ),
    .IBUF_ADDR_WIDTH                ( IBUF_ADDR_WIDTH                ),
    .WBUF_ADDR_WIDTH                ( WBUF_ADDR_WIDTH                ),
    .OBUF_ADDR_WIDTH                ( OBUF_ADDR_WIDTH                ),
    .BBUF_ADDR_WIDTH                ( BBUF_ADDR_WIDTH                ),
    .LOOP_ITER_W                    ( LOOP_ITER_W                    ),
    .ADDR_STRIDE_W                  ( ADDR_STRIDE_W                  ),
    .LOOP_ID_W                      ( LOOP_ID_W                      )
  ) base_ctrl (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input

    .start                          ( base_loop_ctrl_start           ), //input
    .done                           ( base_loop_ctrl_done            ), //output

    .tag_req                        ( base_ctrl_tag_req              ), //output
    .tag_ready                      ( base_ctrl_tag_ready            ), //output

    .cfg_loop_iter_v                ( cfg_loop_iter_v                ), //input
    .cfg_loop_iter                  ( cfg_loop_iter                  ), //input
    .cfg_loop_iter_loop_id          ( cfg_loop_iter_loop_id          ), //input

    .cfg_loop_stride_v              ( cfg_loop_stride_v              ), //input
    .cfg_loop_stride                ( cfg_loop_stride                ), //input
    .cfg_loop_stride_loop_id        ( cfg_loop_stride_loop_id        ), //input
    .cfg_loop_stride_type           ( cfg_loop_stride_type           ), //input
    .cfg_loop_stride_id             ( cfg_loop_stride_id             ), //input

    .obuf_base_addr                 ( obuf_base_addr                 ), //input
    .obuf_ld_addr                   ( obuf_ld_addr                   ), //output
    .obuf_ld_addr_v                 ( obuf_ld_addr_v                 ), //output
    .obuf_st_addr                   ( obuf_st_addr                   ), //output
    .obuf_st_addr_v                 ( obuf_st_addr_v                 ), //output
    .ibuf_base_addr                 ( ibuf_base_addr                 ), //input
    .ibuf_ld_addr                   ( ibuf_ld_addr                   ), //output
    .ibuf_ld_addr_v                 ( ibuf_ld_addr_v                 ), //output
    .wbuf_base_addr                 ( wbuf_base_addr                 ), //input
    .wbuf_ld_addr                   ( wbuf_ld_addr                   ), //output
    .wbuf_ld_addr_v                 ( wbuf_ld_addr_v                 ), //output
    .bias_base_addr                 ( bias_base_addr                 ), //input
    .bias_ld_addr                   ( bias_ld_addr                   ), //output
    .bias_ld_addr_v                 ( bias_ld_addr_v                 ), //output

    .bias_prev_sw                   ( tag_bias_prev_sw               ), //output
    .ddr_pe_sw                      ( tag_ddr_pe_sw                  )  //output
  );
//=============================================================

//=============================================================
// VCD
//=============================================================
  `ifdef COCOTB_TOPLEVEL_controller
  initial begin
    $dumpfile("controller.vcd");
    $dumpvars(0, controller);
  end
  `endif
//=============================================================

endmodule
//
// Loop Controller
//
// (1) RAM to hold loop instructions (max iter count)
// (2) RAM to hold loop states (current iter count)
// (3) FSM that starts when we get the start signal, stops when done
// (4) Stack for the head pointer
//
// Update the loop iterations in controller_fsm when exiting loop
// Update the loop offset in mem-walker-stride when entering loop
//
// Hardik Sharma
// (hsharma@gatech.edu)

`timescale 1ns/1ps
module controller_fsm #(
  parameter integer  LOOP_ID_W                    = 5,
  parameter integer  LOOP_ITER_W                  = 16,
  parameter integer  IMEM_ADDR_W                  = 5,
  // Internal Parameters
  parameter integer  STATE_W                      = 3,
  parameter integer  LOOP_STATE_W                 = LOOP_ID_W,
  parameter integer  STACK_DEPTH                  = (1 << IMEM_ADDR_W)
) (
  input  wire                                         clk,
  input  wire                                         reset,

  // Start and Done handshake signals
  input  wire                                         start,
  output wire                                         done,
  input  wire                                         stall,

  // Loop instruction valid
  input  wire                                         cfg_loop_iter_v,
  input  wire  [ LOOP_ITER_W          -1 : 0 ]        cfg_loop_iter,
  input  wire  [ LOOP_ID_W            -1 : 0 ]        cfg_loop_iter_loop_id,

  output wire  [ LOOP_ID_W            -1 : 0 ]        loop_index,
  output wire                                         loop_index_valid,
  output wire                                         loop_last_iter,
  output wire                                         loop_init,
  output wire                                         loop_enter,
  output wire                                         loop_exit
);


//=============================================================
// Wires/Regs
//=============================================================

  wire                                        loop_wr_req;
  wire [ IMEM_ADDR_W          -1 : 0 ]        loop_wr_ptr;
  wire [ LOOP_ITER_W          -1 : 0 ]        loop_wr_max_iter;

  reg  [ IMEM_ADDR_W          -1 : 0 ]        max_loop_ptr;
  wire [ IMEM_ADDR_W          -1 : 0 ]        loop_rd_ptr;

  wire                                        loop_rd_v;
  wire [ LOOP_ITER_W          -1 : 0 ]        loop_rd_max;

  wire [ IMEM_ADDR_W          -1 : 0 ]        iter_wr_ptr;
  wire                                        iter_wr_v;
  wire [ LOOP_ITER_W          -1 : 0 ]        iter_wr_data;

  reg  [ IMEM_ADDR_W          -1 : 0 ]        loop_index_q;
  reg  [ IMEM_ADDR_W          -1 : 0 ]        loop_index_d; // d -> q

  wire [ IMEM_ADDR_W          -1 : 0 ]        iter_rd_ptr;
  wire                                        iter_rd_v;
  wire [ LOOP_ITER_W          -1 : 0 ]        iter_rd_data;

  reg  [ IMEM_ADDR_W          -1 : 0 ]        stall_rd_ptr;

  wire [ STATE_W              -1 : 0 ]        state;
  reg  [ STATE_W              -1 : 0 ]        state_q;
  reg  [ STATE_W              -1 : 0 ]        state_d;

//=============================================================

//=============================================================
// Loop Instruction Buffer
//=============================================================

  always @(posedge clk)
  begin: MAX_LOOP_PTR
    if (loop_wr_req)
      max_loop_ptr <= cfg_loop_iter_loop_id;
  end

  assign loop_rd_v = iter_rd_v;
  assign loop_rd_ptr = iter_rd_ptr;

  //
  //This module stores the loop max iterations.
  //
  assign loop_wr_ptr = cfg_loop_iter_loop_id;
  assign loop_wr_req = cfg_loop_iter_v;
  assign loop_wr_max_iter = cfg_loop_iter;
  ram #(
    .ADDR_WIDTH                     ( IMEM_ADDR_W                    ),
    .DATA_WIDTH                     ( LOOP_ITER_W                    )
  ) loop_buf (
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .s_write_addr                   ( loop_wr_ptr                    ),
    .s_write_req                    ( loop_wr_req                    ),
    .s_write_data                   ( loop_wr_max_iter               ),
    .s_read_addr                    ( loop_rd_ptr                    ),
    .s_read_req                     ( loop_rd_v                      ),
    .s_read_data                    ( loop_rd_max                    )
  );
//=============================================================

//=============================================================
// Loop Counters
//=============================================================
  //
  //This module stores the current loop iterations.
  //

  ram #(
    .ADDR_WIDTH                     ( IMEM_ADDR_W                    ),
    .DATA_WIDTH                     ( LOOP_ITER_W                    )
  ) iter_buf (
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .s_write_addr                   ( iter_wr_ptr                    ),
    .s_write_req                    ( iter_wr_v                      ),
    .s_write_data                   ( iter_wr_data                   ),
    .s_read_addr                    ( iter_rd_ptr                    ),
    .s_read_req                     ( iter_rd_v                      ),
    .s_read_data                    ( iter_rd_data                   )
  );
//=============================================================

//=============================================================
// FSM
//=============================================================

  localparam integer  IDLE                         = 0;
  localparam integer  INIT_LOOP                    = 1;
  localparam integer  ENTER_LOOP                   = 2;
  localparam integer  INNER_LOOP                   = 3;
  localparam integer  EXIT_LOOP                    = 4;

  always @(*)
  begin
    state_d = state_q;
    loop_index_d = loop_index_q;
    case (state_q)
      IDLE: begin
        loop_index_d = max_loop_ptr;
        if (start) begin
          state_d = INIT_LOOP;
        end
      end
      INIT_LOOP: begin
        if (max_loop_ptr != 0)
          loop_index_d = loop_index_q - 1'b1;
        if (loop_index_q == 1 || loop_index_q == 0)
          state_d = INNER_LOOP;
      end
      ENTER_LOOP: begin
        loop_index_d = loop_index_q - 1'b1;
        if (loop_index_q == 1)
          state_d = INNER_LOOP;
      end
      INNER_LOOP: begin
        if (done)
          state_d = IDLE;
        else if (loop_last_iter && !stall)
        begin
          if (max_loop_ptr != 0) begin
            loop_index_d = loop_index_q + 1'b1;
            state_d = EXIT_LOOP;
          end
          else
            state_d = IDLE;
        end
      end
      EXIT_LOOP: begin
        if (done)
        begin
          loop_index_d = 0;
          state_d = IDLE;
        end
        else if (loop_last_iter)
          loop_index_d = loop_index_q + 1'b1;
        else if (!loop_last_iter)
          state_d = ENTER_LOOP;
      end
      default: begin
        state_d = IDLE;
        loop_index_d = 0;
      end
    endcase
  end

  always @(posedge clk)
  begin
    if (reset)
      loop_index_q <= 'b0;
    else
      loop_index_q <= loop_index_d;
  end

  assign loop_index = loop_index_q;

  always @(posedge clk)
  begin
    if (reset)
      state_q <= 'b0;
    else
      state_q <= state_d;
  end

  assign state = state_q;

//=============================================================

//=============================================================
  // Loop Iteration logic:
  //
  // Set iter counts to zero when initializing the max iters
  // Otherwise, increment write pointer and read pointer every
  // cycle
  //   max_loop_ptr keeps track of the last loop
  //   iter_rd signals correspond to the current iter count
  //   loop_last_iter whenever the current count == max count
//=============================================================
  // assign done = (loop_index == max_loop_ptr) && loop_last_iter;
  assign done = (((state == EXIT_LOOP) && (loop_index == max_loop_ptr)) || ((state == INNER_LOOP) && (max_loop_ptr == 0) && (~stall))) && loop_last_iter;
  assign iter_rd_v = state != IDLE;

  // The below three assign statements update the loop iterations
  assign iter_wr_v = loop_wr_req || state == EXIT_LOOP || (state == INNER_LOOP && !stall);

  assign iter_wr_data = state == IDLE ? 'b0 :
                        loop_last_iter ? 'b0 : iter_rd_data + 1'b1;
  assign iter_wr_ptr = state == IDLE ? cfg_loop_iter_loop_id : loop_index;

  assign loop_last_iter = iter_rd_data == loop_rd_max;

//=============================================================


//=============================================================
// OFFSET generation
//=============================================================

  assign iter_rd_ptr = loop_index;
  assign loop_index_valid = state == INNER_LOOP;

  assign loop_enter = state == ENTER_LOOP || state == INIT_LOOP;
  assign loop_init = state == INIT_LOOP;
  assign loop_exit = state == EXIT_LOOP;

//=============================================================

//=============================================================
// VCD
//=============================================================
`ifdef COCOTB_TOPLEVEL_controller_fsm
initial begin
  $dumpfile("controller_fsm.vcd");
  $dumpvars(0, controller_fsm);
end
`endif
//=============================================================

endmodule
//
// OBUF - Output Buffer
//
// Hardik Sharma
// (hsharma@gatech.edu)
`timescale 1ns/1ps
module obuf #(
  parameter integer  TAG_W                        = 2,  // Log number of banks
  parameter integer  MEM_DATA_WIDTH               = 64,
  parameter integer  ARRAY_M                      = 2,
  parameter integer  DATA_WIDTH                   = 32,
  parameter integer  BUF_ADDR_WIDTH               = 10,

  parameter integer  GROUP_SIZE                   = MEM_DATA_WIDTH / DATA_WIDTH,
  parameter integer  GROUP_ID_W                   = GROUP_SIZE == 1 ? 0 : $clog2(GROUP_SIZE),
  parameter integer  BUF_ID_W                     = $clog2(ARRAY_M) - GROUP_ID_W,

  parameter integer  MEM_ADDR_WIDTH               = BUF_ADDR_WIDTH + BUF_ID_W,
  parameter integer  BUF_DATA_WIDTH               = ARRAY_M * DATA_WIDTH
)
(
  input  wire                                         clk,
  input  wire                                         reset,

  input  wire                                         mem_read_req,
  input  wire  [ MEM_ADDR_WIDTH       -1 : 0 ]        mem_read_addr,
  output wire  [ MEM_DATA_WIDTH       -1 : 0 ]        mem_read_data,

  input  wire                                         mem_write_req,
  input  wire  [ MEM_ADDR_WIDTH       -1 : 0 ]        mem_write_addr,
  input  wire  [ MEM_DATA_WIDTH       -1 : 0 ]        mem_write_data,

  input  wire                                         buf_read_req,
  input  wire  [ BUF_ADDR_WIDTH       -1 : 0 ]        buf_read_addr,
  output wire  [ BUF_DATA_WIDTH       -1 : 0 ]        buf_read_data,

  input  wire                                         buf_write_req,
  input  wire  [ BUF_ADDR_WIDTH       -1 : 0 ]        buf_write_addr,
  input  wire  [ BUF_DATA_WIDTH       -1 : 0 ]        buf_write_data
  );

  wire  [ DATA_WIDTH*ARRAY_M-1 : 0 ]        mem_read_data_raw;

  genvar m;
  generate
    for (m=0; m<ARRAY_M; m=m+1)
    begin: LOOP_M

      localparam integer  LOCAL_ADDR_W                 = BUF_ADDR_WIDTH;
      localparam integer  LOCAL_BUF_ID                 = m/GROUP_SIZE;

      wire                                        local_buf_write_req;
      wire [ LOCAL_ADDR_W         -1 : 0 ]        local_buf_write_addr;
      wire [ DATA_WIDTH           -1 : 0 ]        local_buf_write_data;

      wire                                        local_buf_read_req;
      wire [ LOCAL_ADDR_W         -1 : 0 ]        local_buf_read_addr;
      wire [ DATA_WIDTH           -1 : 0 ]        local_buf_read_data;

      assign local_buf_write_data = buf_write_data[(m)*DATA_WIDTH+:DATA_WIDTH];
      assign buf_read_data[(m)*DATA_WIDTH+:DATA_WIDTH] = local_buf_read_data;

      assign local_buf_read_req = buf_read_req;
      assign local_buf_write_req = buf_write_req;
      assign local_buf_write_addr = buf_write_addr;
      assign local_buf_read_addr = buf_read_addr;

      wire [ BUF_ID_W             -1 : 0 ]        local_mem_write_buf_id;
      wire                                        local_mem_write_req;
      wire [ LOCAL_ADDR_W         -1 : 0 ]        local_mem_write_addr;
      wire [ DATA_WIDTH           -1 : 0 ]        local_mem_write_data;

      wire                                        local_mem_read_req;
      wire [ LOCAL_ADDR_W         -1 : 0 ]        local_mem_read_addr;
      wire [ DATA_WIDTH           -1 : 0 ]        local_mem_read_data;

      wire [ BUF_ID_W             -1 : 0 ]        buf_id;
      assign buf_id = LOCAL_BUF_ID;

      if (BUF_ID_W == 0) begin
        assign local_mem_write_addr = mem_write_addr;
        assign local_mem_write_req = mem_write_req;
        assign local_mem_write_data = mem_write_data[(m%GROUP_SIZE)*DATA_WIDTH+:DATA_WIDTH];

        assign local_mem_read_addr = mem_read_addr;
        assign local_mem_read_req = mem_read_req;
        assign mem_read_data[(m%GROUP_SIZE)*DATA_WIDTH+:DATA_WIDTH] = local_mem_read_data;
      end
      else begin
        wire [ BUF_ID_W             -1 : 0 ]        local_mem_read_buf_id;
        reg  [ BUF_ID_W             -1 : 0 ]        local_mem_read_buf_id_dly;

        assign {local_mem_write_addr, local_mem_write_buf_id} = mem_write_addr;
        assign local_mem_write_req = mem_write_req && local_mem_write_buf_id == buf_id;
        assign local_mem_write_data = mem_write_data[(m%GROUP_SIZE)*DATA_WIDTH+:DATA_WIDTH];

        assign {local_mem_read_addr, local_mem_read_buf_id} = mem_read_addr;
        assign local_mem_read_req = mem_read_req && local_mem_read_buf_id == buf_id;
              
        //The line below was the original code. This creates a complex spatial and temporal muxing structure, which yosys
        //can't infer correctly. So, I (aman) have replaced this with the line below and a new generate statement below (search for LOOP_G).
        //assign mem_read_data[(m%GROUP_SIZE)*DATA_WIDTH+:DATA_WIDTH] = local_mem_read_buf_id_dly == buf_id ? local_mem_read_data : 'bz;
	   		assign mem_read_data_raw[m*DATA_WIDTH+:DATA_WIDTH] = local_mem_read_data;

        // register_sync#(BUF_ID_W) id_dly (clk, reset, local_mem_read_buf_id, local_mem_read_buf_id_dly);

        always @(posedge clk)
        begin
          if (reset)
            local_mem_read_buf_id_dly <= 0;
          else if (mem_read_req)
            local_mem_read_buf_id_dly <= local_mem_read_buf_id;
        end

      end

      banked_ram #(
        .TAG_W                          ( TAG_W                          ),
        .ADDR_WIDTH                     ( LOCAL_ADDR_W                   ),
        .DATA_WIDTH                     ( DATA_WIDTH                     )
      ) buf_ram (
        .clk                            ( clk                            ),
        .reset                          ( reset                          ),
        .s_write_addr_a                 ( local_mem_write_addr           ),
        .s_write_req_a                  ( local_mem_write_req            ),
        .s_write_data_a                 ( local_mem_write_data           ),
        .s_read_addr_a                  ( local_mem_read_addr            ),
        .s_read_req_a                   ( local_mem_read_req             ),
        .s_read_data_a                  ( local_mem_read_data            ),
        .s_write_addr_b                 ( local_buf_write_addr           ),
        .s_write_req_b                  ( local_buf_write_req            ),
        .s_write_data_b                 ( local_buf_write_data           ),
        .s_read_addr_b                  ( local_buf_read_addr            ),
        .s_read_req_b                   ( local_buf_read_req             ),
        .s_read_data_b                  ( local_buf_read_data            )
        );

    end
  endgenerate

  // This is the new generate statement that hardcodes some aspects to make it easier for yosys to infer the hardware.
  genvar g;
  generate
    for (g=0; g<GROUP_SIZE; g=g+1)
    begin: LOOP_G

       assign mem_read_data[g*DATA_WIDTH+:DATA_WIDTH] = mem_read_data_raw[(0+g)*DATA_WIDTH+:DATA_WIDTH] | 
                                                        mem_read_data_raw[(4+g)*DATA_WIDTH+:DATA_WIDTH] | 
                                                        mem_read_data_raw[(8+g)*DATA_WIDTH+:DATA_WIDTH] | 
                                                        mem_read_data_raw[(12+g)*DATA_WIDTH+:DATA_WIDTH] |
                                                        mem_read_data_raw[(16+g)*DATA_WIDTH+:DATA_WIDTH] |
                                                        mem_read_data_raw[(20+g)*DATA_WIDTH+:DATA_WIDTH] |
                                                        mem_read_data_raw[(24+g)*DATA_WIDTH+:DATA_WIDTH] |
                                                        mem_read_data_raw[(28+g)*DATA_WIDTH+:DATA_WIDTH] ;

//                                                        mem_read_data_raw[(32+g)*DATA_WIDTH+:DATA_WIDTH] |
//                                                        mem_read_data_raw[(36+g)*DATA_WIDTH+:DATA_WIDTH] |
//                                                        mem_read_data_raw[(40+g)*DATA_WIDTH+:DATA_WIDTH] |
//                                                        mem_read_data_raw[(44+g)*DATA_WIDTH+:DATA_WIDTH] |
//                                                        mem_read_data_raw[(48+g)*DATA_WIDTH+:DATA_WIDTH] |
//                                                        mem_read_data_raw[(52+g)*DATA_WIDTH+:DATA_WIDTH] |
//                                                        mem_read_data_raw[(56+g)*DATA_WIDTH+:DATA_WIDTH] |
//                                                        mem_read_data_raw[(60+g)*DATA_WIDTH+:DATA_WIDTH] ;

    end
  endgenerate

//=============================================================
// VCD
//=============================================================
  `ifdef COCOTB_TOPLEVEL_buffer
    initial begin
      $dumpfile("buffer.vcd");
      $dumpvars(0, buffer);
    end
  `endif
//=============================================================
endmodule

//
// n:1 Mux
//
// Hardik Sharma
// (hsharma@gatech.edu)

`timescale 1ns/1ps
module mux_n_1 #(
  parameter integer WIDTH     = 8,                // Data Width
  parameter integer LOG2_N    = 7,                // Log_2(Num of inputs)
  parameter integer IN_WIDTH  = (1<<LOG2_N)*WIDTH,// Input Width = 2 * Data Width
  parameter integer OUT_WIDTH = WIDTH,            // Output Width
  parameter integer TOP_MODULE = 1                 // Output Width
) (
  input  wire        [ LOG2_N         -1 : 0 ]    sel,
  input  wire        [ IN_WIDTH       -1 : 0 ]    data_in,
  output wire        [ OUT_WIDTH      -1 : 0 ]    data_out
);

//In our design, WIDTH is 16 and LOG2N is 1
//So, I'm hardcoding the design below for these values
//to avoid having a recursive call that Yosys may not like

//genvar ii, jj;
//generate
//if (LOG2_N == 0)
//begin
//  assign data_out = data_in;
//end
//else if (LOG2_N > 1)
//begin
//  localparam integer SEL_LOW_WIDTH = LOG2_N-1; // select at lower level has 1 less width
//  localparam integer IN_LOW_WIDTH  = IN_WIDTH / 2; // Input at lower level has half width
//  localparam integer OUT_LOW_WIDTH = OUT_WIDTH; // Output at lower level has same width
//
//  wire [ SEL_LOW_WIDTH  -1 : 0 ] sel_low;
//  wire [ IN_LOW_WIDTH   -1 : 0 ] in_0;
//  wire [ IN_LOW_WIDTH   -1 : 0 ] in_1;
//  wire [ OUT_LOW_WIDTH  -1 : 0 ] out_0;
//  wire [ OUT_LOW_WIDTH  -1 : 0 ] out_1;
//
//  assign sel_low = sel[LOG2_N-2: 0];
//  assign in_0 = data_in[0+:IN_LOW_WIDTH];
//  assign in_1 = data_in[IN_LOW_WIDTH+:IN_LOW_WIDTH];
//
//  mux_n_1 #(
//    .WIDTH          ( WIDTH         ),
//    .TOP_MODULE     ( 0             ),
//    .LOG2_N         ( SEL_LOW_WIDTH )
//  ) mux_0 (
//    .sel            ( sel_low       ),
//    .data_in        ( in_0          ),
//    .data_out       ( out_0         )
//  );
//
//  mux_n_1 #(
//    .WIDTH          ( WIDTH         ),
//    .TOP_MODULE     ( 0             ),
//    .LOG2_N         ( SEL_LOW_WIDTH )
//  ) mux_1 (
//    .sel            ( sel_low       ),
//    .data_in        ( in_1          ),
//    .data_out       ( out_1         )
//  );
//
//  wire sel_curr = sel[LOG2_N-1];
//  localparam IN_CURR_WIDTH = 2 * OUT_WIDTH;
//  wire [ IN_CURR_WIDTH -1 : 0 ] in_curr = {out_1, out_0};
//
//  mux_2_1 #(
//    .WIDTH          ( WIDTH         )
//  ) mux_inst_curr (
//    .sel            ( sel_curr      ),
//    .data_in        ( in_curr       ),
//    .data_out       ( data_out      )
//  );
//end
//else
//begin
  mux_2_1 #(
    .WIDTH          ( WIDTH         )
  ) mux_inst_curr (
    .sel            ( sel           ),
    .data_in        ( data_in       ),
    .data_out       ( data_out      )
  );
//end
//endgenerate
//=========================================
// Debugging: COCOTB VCD
//=========================================
`ifdef COCOTB_TOPLEVEL_mux_n_1
if (TOP_MODULE == 1)
begin
  initial begin
    $dumpfile("mux_n_1.vcd");
    $dumpvars(0, mux_n_1);
  end
end
`endif

endmodule
//
// Base address generator
//
// Hardik Sharma
// (hsharma@gatech.edu)

`timescale 1ns/1ps
module base_addr_gen #(
  // Internal Parameters
  parameter integer  BASE_ID                      = 1,
  parameter integer  IBUF_MEM_ID                  = 0,
  parameter integer  OBUF_MEM_ID                  = 1,
  parameter integer  WBUF_MEM_ID                  = 2,
  parameter integer  BBUF_MEM_ID                  = 3,

  parameter integer  MEM_REQ_W                    = 16,
  parameter integer  IBUF_ADDR_WIDTH              = 8,
  parameter integer  WBUF_ADDR_WIDTH              = 8,
  parameter integer  OBUF_ADDR_WIDTH              = 8,
  parameter integer  BBUF_ADDR_WIDTH              = 8,
  parameter integer  DATA_WIDTH                   = 32,
  parameter integer  LOOP_ITER_W                  = 16,
  parameter integer  ADDR_STRIDE_W                = 16,
  parameter integer  LOOP_ID_W                    = 5,
  parameter integer  BUF_TYPE_W                   = 2
) (
  input  wire                                         clk,
  input  wire                                         reset,

  input  wire                                         start,
  output wire                                         done,

  output wire                                         tag_req,
  input  wire                                         tag_ready,

  // Programming
  input  wire                                         cfg_loop_iter_v,
  input  wire  [ LOOP_ITER_W          -1 : 0 ]        cfg_loop_iter,
  input  wire  [ LOOP_ID_W            -1 : 0 ]        cfg_loop_iter_loop_id,

  // Programming
  input  wire                                         cfg_loop_stride_v,
  input  wire  [ ADDR_STRIDE_W        -1 : 0 ]        cfg_loop_stride,
  input  wire  [ LOOP_ID_W            -1 : 0 ]        cfg_loop_stride_loop_id,
  input  wire  [ BUF_TYPE_W           -1 : 0 ]        cfg_loop_stride_id,
  input  wire  [ 2                    -1 : 0 ]        cfg_loop_stride_type,

  // Address - OBUF LD/ST
  input  wire  [ OBUF_ADDR_WIDTH      -1 : 0 ]        obuf_base_addr,
  output wire  [ OBUF_ADDR_WIDTH      -1 : 0 ]        obuf_ld_addr,
  output wire                                         obuf_ld_addr_v,
  output wire  [ OBUF_ADDR_WIDTH      -1 : 0 ]        obuf_st_addr,
  output wire                                         obuf_st_addr_v,
  // Address - IBUF LD
  input  wire  [ IBUF_ADDR_WIDTH      -1 : 0 ]        ibuf_base_addr,
  output wire  [ IBUF_ADDR_WIDTH      -1 : 0 ]        ibuf_ld_addr,
  output wire                                         ibuf_ld_addr_v,
  // Address - WBUF LD
  input  wire  [ WBUF_ADDR_WIDTH      -1 : 0 ]        wbuf_base_addr,
  output wire  [ WBUF_ADDR_WIDTH      -1 : 0 ]        wbuf_ld_addr,
  output wire                                         wbuf_ld_addr_v,
  // Address - BIAS LD
  input  wire  [ BBUF_ADDR_WIDTH      -1 : 0 ]        bias_base_addr,
  output wire  [ BBUF_ADDR_WIDTH      -1 : 0 ]        bias_ld_addr,
  output wire                                         bias_ld_addr_v,

  output wire                                         bias_prev_sw,
  output wire                                         ddr_pe_sw
);

//==============================================================================
// Wires/Regs
//==============================================================================
  // Programming - Base loop
  wire                                        cfg_base_loop_iter_v;
  wire [ LOOP_ITER_W          -1 : 0 ]        cfg_base_loop_iter;
  reg  [ LOOP_ID_W            -1 : 0 ]        cfg_base_loop_iter_loop_id;

  // Base loop
  wire                                        base_loop_start;
  wire                                        base_loop_done;
  wire                                        base_loop_stall;
  wire                                        base_loop_init;
  wire                                        base_loop_enter;
  wire                                        base_loop_exit;
  wire                                        base_loop_last_iter;
  wire [ LOOP_ID_W            -1 : 0 ]        base_loop_index;
  wire                                        base_loop_index_valid;
  wire                                        _base_loop_index_valid;

  // Programming - OBUF LD/ST
  wire                                        obuf_stride_v;
  wire [ ADDR_STRIDE_W        -1 : 0 ]        obuf_stride;
  // Programming - Bias
  wire                                        bias_stride_v;
  wire [ ADDR_STRIDE_W        -1 : 0 ]        bias_stride;
  // Programming - OBUF ST
  wire                                        ibuf_stride_v;
  wire [ ADDR_STRIDE_W        -1 : 0 ]        ibuf_stride;
  // Programming - OBUF ST
  wire                                        wbuf_stride_v;
  wire [ ADDR_STRIDE_W        -1 : 0 ]        wbuf_stride;


  wire [ OBUF_ADDR_WIDTH      -1 : 0 ]        obuf_addr;
  wire                                        obuf_addr_v;


  reg  [ MEM_REQ_W            -1 : 0 ]        obuf_ld_req_size;
  reg  [ MEM_REQ_W            -1 : 0 ]        obuf_st_req_size;

  wire                                        obuf_ld_req_valid;
  wire                                        obuf_st_req_valid;

  reg  [ MEM_REQ_W            -1 : 0 ]        obuf_ld_req_loop_id;
  reg  [ MEM_REQ_W            -1 : 0 ]        obuf_st_req_loop_id;

  wire                                        cfg_base_stride_v;
//==============================================================================

//==============================================================================
// Assigns
//==============================================================================
  assign cfg_base_loop_iter_v = cfg_loop_iter_v && cfg_loop_iter_loop_id == BASE_ID * 16;
  assign cfg_base_loop_iter = cfg_loop_iter;

  assign cfg_base_stride_v = cfg_loop_stride_v && cfg_loop_stride_loop_id == BASE_ID * 16;

  assign obuf_stride = cfg_loop_stride;
  assign obuf_stride_v = cfg_base_stride_v && cfg_loop_stride_type[0] == 1'b0 && cfg_loop_stride_id == OBUF_MEM_ID;
  assign bias_stride = cfg_loop_stride;
  assign bias_stride_v = cfg_base_stride_v && cfg_loop_stride_type[0] == 1'b0 && cfg_loop_stride_id == BBUF_MEM_ID;
  assign ibuf_stride = cfg_loop_stride;
  assign ibuf_stride_v = cfg_base_stride_v && cfg_loop_stride_type[0] == 1'b0 && cfg_loop_stride_id == IBUF_MEM_ID;
  assign wbuf_stride = cfg_loop_stride;
  assign wbuf_stride_v = cfg_base_stride_v && cfg_loop_stride_type[0] == 1'b0 && cfg_loop_stride_id == WBUF_MEM_ID;
//==============================================================================

//==============================================================================
// Address generators
//==============================================================================
  mem_walker_stride #(
    .ADDR_WIDTH                     ( OBUF_ADDR_WIDTH                ),
    .ADDR_STRIDE_W                  ( ADDR_STRIDE_W                  ),
    .LOOP_ID_W                      ( LOOP_ID_W                      )
  ) mws_obuf_ld (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .base_addr                      ( obuf_base_addr                 ), //input
    .loop_ctrl_done                 ( base_loop_done                 ), //input
    .loop_index                     ( base_loop_index                ), //input
    .loop_index_valid               ( _base_loop_index_valid         ), //input
    .loop_init                      ( base_loop_init                 ), //input
    .loop_enter                     ( base_loop_enter                ), //input
    .loop_exit                      ( base_loop_exit                 ), //input
    .cfg_addr_stride_v              ( obuf_stride_v                  ), //input
    .cfg_addr_stride                ( obuf_stride                    ), //input
    .addr_out                       ( obuf_addr                      ), //output
    .addr_out_valid                 ( obuf_addr_v                    )  //output
  );

  assign obuf_st_addr = obuf_addr;
  assign obuf_st_addr_v = obuf_addr_v;

  assign obuf_ld_addr = obuf_addr;
  assign obuf_ld_addr_v = obuf_addr_v;

  obuf_bias_sel_logic #(
    .LOOP_ID_W                      ( LOOP_ID_W                      ),
    .ADDR_STRIDE_W                  ( ADDR_STRIDE_W                  )
  ) u_sel_logic (
    .clk                            ( clk                            ), // input
    .reset                          ( reset                          ), // input
    .start                          ( start                          ), // input
    .done                           ( done                           ), // input
    .obuf_stride                    ( obuf_stride                    ), // input
    .obuf_stride_v                  ( obuf_stride_v                  ), // input
    .loop_stall                     ( base_loop_stall                ), // input
    .loop_enter                     ( base_loop_enter                ), // input
    .loop_exit                      ( base_loop_exit                 ), // input
    .loop_last_iter                 ( base_loop_last_iter            ), // input
    .loop_index_valid               ( base_loop_index_valid          ), // input
    .loop_index                     ( base_loop_index                ), // input
    .bias_prev_sw                   ( bias_prev_sw                   ), // output
    .ddr_pe_sw                      ( ddr_pe_sw                      )  // output
  );

  mem_walker_stride #(
    .ADDR_WIDTH                     ( BBUF_ADDR_WIDTH                ),
    .ADDR_STRIDE_W                  ( ADDR_STRIDE_W                  ),
    .LOOP_ID_W                      ( LOOP_ID_W                      )
  ) mws_bias_ld (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .base_addr                      ( bias_base_addr                 ), //input
    .loop_ctrl_done                 ( base_loop_done                 ), //input
    .loop_index                     ( base_loop_index                ), //input
    .loop_index_valid               ( _base_loop_index_valid         ), //input
    .loop_init                      ( base_loop_init                 ), //input
    .loop_enter                     ( base_loop_enter                ), //input
    .loop_exit                      ( base_loop_exit                 ), //input
    .cfg_addr_stride_v              ( bias_stride_v                  ), //input
    .cfg_addr_stride                ( bias_stride                    ), //input
    .addr_out                       ( bias_ld_addr                   ), //output
    .addr_out_valid                 ( bias_ld_addr_v                 )  //output
  );

  mem_walker_stride #(
    .ADDR_WIDTH                     ( IBUF_ADDR_WIDTH                ),
    .ADDR_STRIDE_W                  ( ADDR_STRIDE_W                  ),
    .LOOP_ID_W                      ( LOOP_ID_W                      )
  ) mws_ibuf_ld (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .base_addr                      ( ibuf_base_addr                 ), //input
    .loop_ctrl_done                 ( base_loop_done                 ), //input
    .loop_index                     ( base_loop_index                ), //input
    .loop_index_valid               ( _base_loop_index_valid         ), //input
    .loop_init                      ( base_loop_init                 ), //input
    .loop_enter                     ( base_loop_enter                ), //input
    .loop_exit                      ( base_loop_exit                 ), //input
    .cfg_addr_stride_v              ( ibuf_stride_v                  ), //input
    .cfg_addr_stride                ( ibuf_stride                    ), //input
    .addr_out                       ( ibuf_ld_addr                   ), //output
    .addr_out_valid                 ( ibuf_ld_addr_v                 )  //output
  );

  mem_walker_stride #(
    .ADDR_WIDTH                     ( WBUF_ADDR_WIDTH                ),
    .ADDR_STRIDE_W                  ( ADDR_STRIDE_W                  ),
    .LOOP_ID_W                      ( LOOP_ID_W                      )
  ) mws_wbuf_ld (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .base_addr                      ( wbuf_base_addr                 ), //input
    .loop_ctrl_done                 ( base_loop_done                 ), //input
    .loop_index                     ( base_loop_index                ), //input
    .loop_index_valid               ( _base_loop_index_valid         ), //input
    .loop_init                      ( base_loop_init                 ), //input
    .loop_enter                     ( base_loop_enter                ), //input
    .loop_exit                      ( base_loop_exit                 ), //input
    .cfg_addr_stride_v              ( wbuf_stride_v                  ), //input
    .cfg_addr_stride                ( wbuf_stride                    ), //input
    .addr_out                       ( wbuf_ld_addr                   ), //output
    .addr_out_valid                 ( wbuf_ld_addr_v                 )  //output
  );
//==============================================================================

//==============================================================================
// Base loop controller
//==============================================================================
  assign base_loop_start = start;
  assign base_loop_stall = !tag_ready;
  assign done = base_loop_done;
  assign tag_req = base_loop_index_valid; // && tag_ready;
  assign _base_loop_index_valid = tag_req && tag_ready;

  // assign cfg_base_loop_iter_loop_id = {1'b0, cfg_loop_iter_loop_id[3:0]};
  always @(posedge clk)
  begin
    if (reset)
      cfg_base_loop_iter_loop_id <= 0;
    else begin
      if (start)
        cfg_base_loop_iter_loop_id <= 0;
      else if (cfg_base_loop_iter_v)
        cfg_base_loop_iter_loop_id <= cfg_base_loop_iter_loop_id + 1'b1;
    end
  end

  controller_fsm #(
    .LOOP_ID_W                      ( LOOP_ID_W                      ),
    .LOOP_ITER_W                    ( LOOP_ITER_W                    ),
    .IMEM_ADDR_W                    ( LOOP_ID_W                      )
  ) base_loop_ctrl (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .cfg_loop_iter_v                ( cfg_base_loop_iter_v           ), //input
    .cfg_loop_iter                  ( cfg_base_loop_iter             ), //input
    .cfg_loop_iter_loop_id          ( cfg_base_loop_iter_loop_id     ), //input
    .start                          ( base_loop_start                ), //input
    .done                           ( base_loop_done                 ), //output
    .stall                          ( base_loop_stall                ), //input
    .loop_init                      ( base_loop_init                 ), //output
    .loop_enter                     ( base_loop_enter                ), //output
    .loop_exit                      ( base_loop_exit                 ), //output
    .loop_last_iter                 ( base_loop_last_iter            ), //output
    .loop_index                     ( base_loop_index                ), //output
    .loop_index_valid               ( base_loop_index_valid          )  //output
  );
//==============================================================================

//==============================================================================
// VCD
//==============================================================================
`ifdef COCOTB_TOPLEVEL_base_addr_gen
initial begin
  $dumpfile("base_addr_gen.vcd");
  $dumpvars(0, base_addr_gen);
end
`endif
//==============================================================================
endmodule
//
// Signed Adder
// Implements: out = a + b
//
// Hardik Sharma
// (hsharma@gatech.edu)

`timescale 1ns/1ps
module signed_adder #(
    parameter integer  DTYPE                        = "FXP",
    parameter          REGISTER_OUTPUT              = "FALSE",
    parameter integer  IN1_WIDTH                    = 20,
    parameter integer  IN2_WIDTH                    = 32,
    parameter integer  OUT_WIDTH                    = 32
) (
    input  wire                                         clk,
    input  wire                                         reset,
    input  wire                                         enable,
    input  wire  [ IN1_WIDTH            -1 : 0 ]        a,
    input  wire  [ IN2_WIDTH            -1 : 0 ]        b,
    output wire  [ OUT_WIDTH            -1 : 0 ]        out
  );

  generate
    if (DTYPE == "FXP") begin
      wire signed [ IN1_WIDTH-1:0] _a;
      wire signed [ IN2_WIDTH-1:0] _b;
      wire signed [ OUT_WIDTH-1:0] alu_out;
      assign _a = a;
      assign _b = b;
      assign alu_out = _a + _b;
      if (REGISTER_OUTPUT == "TRUE") begin
        reg [OUT_WIDTH-1:0] _alu_out;
        always @(posedge clk)
        begin
          if (enable)
            _alu_out <= alu_out;
        end
        assign out = _alu_out;
      end else
        assign out = alu_out;
    end
    else if (DTYPE == "FP32") begin
      fp32_add add (
        .clk                            ( clk                            ),
        .a                              ( a                              ),
        .b                              ( b                              ),
        .result                         ( out                            )
        );
    end
    else if (DTYPE == "FP16") begin
      fp_mixed_add add (
        .clk                            ( clk                            ),
        .a                              ( a                              ),
        .b                              ( b                              ),
        .result                         ( out                            )
        );
    end
  endgenerate

endmodule
//
// Wrapper for memory
//
// Hardik Sharma
// (hsharma@gatech.edu)

`timescale 1ns/1ps
module obuf_mem_wrapper #(
  // Internal Parameters
    parameter integer  MEM_ID                       = 1,
    parameter integer  STORE_ENABLED                = 1,
    parameter integer  MEM_REQ_W                    = 16,
    parameter integer  ADDR_WIDTH                   = 8,
    parameter integer  DATA_WIDTH                   = 64,
    parameter integer  LOOP_ITER_W                  = 16,
    parameter integer  ADDR_STRIDE_W                = 32,
    parameter integer  LOOP_ID_W                    = 5,
    parameter integer  BUF_TYPE_W                   = 2,
    parameter integer  NUM_TAGS                     = 4,
    parameter integer  TAG_W                        = $clog2(NUM_TAGS),

  // AXI
    parameter integer  AXI_ID_WIDTH                 = 1,
    parameter integer  AXI_ADDR_WIDTH               = 42,
    parameter integer  AXI_DATA_WIDTH               = 64,
    parameter integer  AXI_BURST_WIDTH              = 8,
    parameter integer  WSTRB_W                      = AXI_DATA_WIDTH/8,

  // Buffer
    parameter integer  ARRAY_N                      = 4,
    parameter integer  ARRAY_M                      = 4,
    parameter integer  BUF_DATA_WIDTH               = DATA_WIDTH * ARRAY_M,
    parameter integer  BUF_ADDR_W                   = 16,
    parameter integer  MEM_ADDR_W                   = BUF_ADDR_W + $clog2(BUF_DATA_WIDTH / AXI_DATA_WIDTH),
    parameter integer  TAG_BUF_ADDR_W               = BUF_ADDR_W + TAG_W,
    parameter integer  TAG_MEM_ADDR_W               = MEM_ADDR_W + TAG_W
) (
    input  wire                                         clk,
    input  wire                                         reset,

    input  wire                                         tag_req,
    input  wire                                         tag_reuse,
    input  wire                                         tag_bias_prev_sw,
    input  wire                                         tag_ddr_pe_sw,
    output wire                                         tag_ready,
    output wire                                         tag_done,
    input  wire                                         compute_done,
    input  wire                                         block_done,
    input  wire  [ ADDR_WIDTH           -1 : 0 ]        tag_base_ld_addr,
    input  wire  [ ADDR_WIDTH           -1 : 0 ]        tag_base_st_addr,

    output wire                                         compute_ready,
    output wire                                         compute_bias_prev_sw,

  // Programming
    input  wire                                         cfg_loop_stride_v,
    input  wire  [ ADDR_STRIDE_W        -1 : 0 ]        cfg_loop_stride,
    input  wire  [ LOOP_ID_W            -1 : 0 ]        cfg_loop_stride_loop_id,
    input  wire  [ BUF_TYPE_W           -1 : 0 ]        cfg_loop_stride_id,
    input  wire  [ 2                    -1 : 0 ]        cfg_loop_stride_type,

    input  wire                                         cfg_loop_iter_v,
    input  wire  [ LOOP_ITER_W          -1 : 0 ]        cfg_loop_iter,
    input  wire  [ LOOP_ID_W            -1 : 0 ]        cfg_loop_iter_loop_id,

    input  wire                                         cfg_mem_req_v,
    input  wire  [ BUF_TYPE_W           -1 : 0 ]        cfg_mem_req_id,
    input  wire  [ MEM_REQ_W            -1 : 0 ]        cfg_mem_req_size,
    input  wire  [ LOOP_ID_W            -1 : 0 ]        cfg_mem_req_loop_id,
    input  wire  [ 2                    -1 : 0 ]        cfg_mem_req_type,

  // Systolic Array
    input  wire  [ BUF_DATA_WIDTH       -1 : 0 ]        buf_write_data,
    input  wire                                         buf_write_req,
    input  wire  [ BUF_ADDR_W           -1 : 0 ]        buf_write_addr,
    output wire  [ BUF_DATA_WIDTH       -1 : 0 ]        buf_read_data,
    input  wire                                         buf_read_req,
    input  wire  [ BUF_ADDR_W           -1 : 0 ]        buf_read_addr,

  // PU
    input  wire                                         pu_buf_read_req,
    input  wire  [ MEM_ADDR_W           -1 : 0 ]        pu_buf_read_addr,
    output wire                                         pu_buf_read_ready,

    output wire  [ AXI_DATA_WIDTH       -1 : 0 ]        obuf_ld_stream_write_data,
    output wire                                         obuf_ld_stream_write_req,

    output wire                                         pu_compute_start,
    input  wire                                         pu_compute_ready,
    input  wire                                         pu_compute_done,

  // CL_wrapper -> DDR AXI4 interface
    // Master Interface Write Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        mws_awaddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        mws_awlen,
    output wire  [ 3                    -1 : 0 ]        mws_awsize,
    output wire  [ 2                    -1 : 0 ]        mws_awburst,
    output wire                                         mws_awvalid,
    input  wire                                         mws_awready,
    // Master Interface Write Data
    output wire  [ AXI_DATA_WIDTH       -1 : 0 ]        mws_wdata,
    output wire  [ WSTRB_W              -1 : 0 ]        mws_wstrb,
    output wire                                         mws_wlast,
    output wire                                         mws_wvalid,
    input  wire                                         mws_wready,
    // Master Interface Write Response
    input  wire  [ 2                    -1 : 0 ]        mws_bresp,
    input  wire                                         mws_bvalid,
    output wire                                         mws_bready,
    // Master Interface Read Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        mws_araddr,
    output wire  [ AXI_ID_WIDTH         -1 : 0 ]        mws_arid,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        mws_arlen,
    output wire  [ 3                    -1 : 0 ]        mws_arsize,
    output wire  [ 2                    -1 : 0 ]        mws_arburst,
    output wire                                         mws_arvalid,
    input  wire                                         mws_arready,
    // Master Interface Read Data
    input  wire  [ AXI_DATA_WIDTH       -1 : 0 ]        mws_rdata,
    input  wire  [ AXI_ID_WIDTH         -1 : 0 ]        mws_rid,
    input  wire  [ 2                    -1 : 0 ]        mws_rresp,
    input  wire                                         mws_rlast,
    input  wire                                         mws_rvalid,
    output wire                                         mws_rready,

    output wire  [ 4                    -1 : 0 ]        stmem_state,
    output wire  [ TAG_W                -1 : 0 ]        stmem_tag,
    output wire                                         stmem_ddr_pe_sw
);

//==============================================================================
// Localparams
//==============================================================================
    localparam integer  LDMEM_IDLE                   = 0;
    localparam integer  LDMEM_CHECK_RAW              = 1;
    localparam integer  LDMEM_BUSY                   = 2;
    localparam integer  LDMEM_WAIT_0                 = 3;
    localparam integer  LDMEM_WAIT_1                 = 4;
    localparam integer  LDMEM_WAIT_2                 = 5;
    localparam integer  LDMEM_WAIT_3                 = 6;
    localparam integer  LDMEM_DONE                   = 7;

    localparam integer  STMEM_IDLE                   = 0;
    localparam integer  STMEM_COMPUTE_WAIT           = 1;
    localparam integer  STMEM_DDR                    = 2;
    localparam integer  STMEM_DDR_WAIT               = 3;
    localparam integer  STMEM_DONE                   = 4;
    localparam integer  STMEM_PU                     = 5;

    localparam integer  MEM_LD                       = 0;
    localparam integer  MEM_ST                       = 1;
    localparam integer  MEM_RD                       = 2;
    localparam integer  MEM_WR                       = 3;
//==============================================================================

//==============================================================================
// Wires/Regs
//==============================================================================
    wire                                        compute_tag_done;
    wire                                        compute_tag_reuse;
    wire                                        compute_tag_ready;
    wire [ TAG_W                -1 : 0 ]        compute_tag;
    wire [ TAG_W                -1 : 0 ]        compute_tag_delayed;
    wire                                        ldmem_tag_done;
    wire                                        ldmem_tag_ready;
    wire [ TAG_W                -1 : 0 ]        ldmem_tag;
    wire                                        stmem_tag_done;
    wire                                        stmem_tag_ready;

    reg  [ 4                    -1 : 0 ]        ldmem_state_d;
    reg  [ 4                    -1 : 0 ]        ldmem_state_q;

    reg  [ 4                    -1 : 0 ]        stmem_state_d;
    reg  [ 4                    -1 : 0 ]        stmem_state_q;

    wire                                        ld_mem_req_v;
    wire                                        st_mem_req_v;

    wire [ TAG_W                -1 : 0 ]        tag;


    reg                                         ld_iter_v_q;
    reg  [ LOOP_ITER_W          -1 : 0 ]        iter_q;
    reg                                         st_iter_v_q;

    reg  [ LOOP_ID_W            -1 : 0 ]        ld_loop_id_counter;
    reg  [ LOOP_ID_W            -1 : 0 ]        st_loop_id_counter;

    wire [ LOOP_ID_W            -1 : 0 ]        mws_ld_loop_iter_loop_id;
    wire [ LOOP_ITER_W          -1 : 0 ]        mws_ld_loop_iter;
    wire                                        mws_ld_loop_iter_v;
    wire                                        mws_ld_start;
    wire                                        mws_ld_done;
    wire                                        mws_ld_stall;
    wire                                        mws_ld_init;
    wire                                        mws_ld_enter;
    wire                                        mws_ld_exit;
    wire [ LOOP_ID_W            -1 : 0 ]        mws_ld_index;
    wire                                        mws_ld_index_valid;
    wire                                        mws_ld_step;

    wire [ LOOP_ID_W            -1 : 0 ]        mws_st_loop_iter_loop_id;
    wire [ LOOP_ITER_W          -1 : 0 ]        mws_st_loop_iter;
    wire                                        mws_st_loop_iter_v;
    wire                                        mws_st_start;
    wire                                        mws_st_done;
    wire                                        mws_st_stall;
    wire                                        mws_st_init;
    wire                                        mws_st_enter;
    wire                                        mws_st_exit;
    wire [ LOOP_ID_W            -1 : 0 ]        mws_st_index;
    wire                                        mws_st_index_valid;
    wire                                        mws_st_step;

    wire                                        ld_stride_v;
    wire [ ADDR_STRIDE_W        -1 : 0 ]        ld_stride;
    wire [ BUF_TYPE_W           -1 : 0 ]        ld_stride_id;
    wire                                        st_stride_v;
    wire [ ADDR_STRIDE_W        -1 : 0 ]        st_stride;
    wire [ BUF_TYPE_W           -1 : 0 ]        st_stride_id;

    wire [ ADDR_WIDTH           -1 : 0 ]        ld_addr;
    wire [ ADDR_WIDTH           -1 : 0 ]        mws_ld_base_addr;
    wire                                        ld_addr_v;
    wire [ ADDR_WIDTH           -1 : 0 ]        st_addr;
    wire [ ADDR_WIDTH           -1 : 0 ]        mws_st_base_addr;
    wire                                        st_addr_v;


    reg  [ MEM_REQ_W            -1 : 0 ]        ld_req_size;
    reg  [ MEM_REQ_W            -1 : 0 ]        st_req_size;

    wire                                        ld_req_valid_d;
    wire                                        st_req_valid_d;

    reg                                         ld_req_valid_q;
    reg                                         st_req_valid_q;

    //reg  [ ADDR_WIDTH           -1 : 0 ]        tag_ld_addr[0:NUM_TAGS-1];
    //reg  [ ADDR_WIDTH           -1 : 0 ]        tag_st_addr[0:NUM_TAGS-1];
    //reg  [ ADDR_WIDTH           -1 : 0 ]        tag_st_addr_copy[0:NUM_TAGS-1];

    reg  [ ADDR_WIDTH           -1 : 0 ]        ld_req_addr;
    reg  [ ADDR_WIDTH           -1 : 0 ]        st_req_addr;

    reg  [ MEM_REQ_W            -1 : 0 ]        st_req_loop_id;

    wire                                        axi_rd_req;
    wire [ AXI_ID_WIDTH         -1 : 0 ]        axi_rd_req_id;
    wire                                        axi_rd_done;
    wire [ MEM_REQ_W            -1 : 0 ]        axi_rd_req_size;
    wire                                        axi_rd_ready;
    wire [ AXI_ADDR_WIDTH       -1 : 0 ]        axi_rd_addr;

    wire                                        axi_wr_req;
    wire [ AXI_ID_WIDTH         -1 : 0 ]        axi_wr_req_id;
    wire                                        axi_wr_done;
    wire [ MEM_REQ_W            -1 : 0 ]        axi_wr_req_size;
    wire                                        axi_wr_ready;
    wire [ AXI_ADDR_WIDTH       -1 : 0 ]        axi_wr_addr;

    wire                                        mem_write_req;
    wire [ AXI_ID_WIDTH         -1 : 0 ]        mem_write_id;
    wire [ AXI_DATA_WIDTH       -1 : 0 ]        mem_write_data;
    reg  [ MEM_ADDR_W           -1 : 0 ]        mem_write_addr;
    wire                                        mem_write_ready;
    wire [ AXI_DATA_WIDTH       -1 : 0 ]        mem_read_data;
    wire [ MEM_ADDR_W           -1 : 0 ]        mem_read_addr;
    reg  [ MEM_ADDR_W           -1 : 0 ]        axi_mem_read_addr;
    wire                                        axi_mem_read_req;
    wire                                        axi_mem_read_ready;
    wire                                        mem_read_req;
    wire                                        mem_read_ready;

  // Adding register to buf read data
    wire [ BUF_DATA_WIDTH       -1 : 0 ]        _buf_read_data;
//==============================================================================

//==============================================================================
// Assigns
//==============================================================================
    assign ld_stride = cfg_loop_stride;
    assign ld_stride_v = cfg_loop_stride_v && cfg_loop_stride_loop_id == 1 + MEM_ID && cfg_loop_stride_type == MEM_LD && cfg_loop_stride_id == MEM_ID;
    assign st_stride = cfg_loop_stride;
    assign st_stride_v = cfg_loop_stride_v && cfg_loop_stride_loop_id == 1 + MEM_ID && cfg_loop_stride_type == MEM_ST && cfg_loop_stride_id == MEM_ID;

    //assign mws_ld_base_addr = tag_ld_addr[ldmem_tag];
    //assign mws_st_base_addr = tag_st_addr[stmem_tag];
    assign axi_rd_req = ld_req_valid_q;
    assign axi_rd_req_size = ld_req_size * (ARRAY_M * DATA_WIDTH) / AXI_DATA_WIDTH;
    assign axi_rd_addr = ld_req_addr;

    assign axi_wr_req = st_req_valid_q;
    assign axi_wr_req_id = 1'b0;
    assign axi_wr_req_size = st_req_size * (ARRAY_M * DATA_WIDTH) / AXI_DATA_WIDTH;
    assign axi_wr_addr = st_req_addr;
//==============================================================================

//==============================================================================
//==============================================================================
    reg                                         read_req_dly1;
  always @(posedge clk)
  begin
    if (reset) begin
      read_req_dly1 <= 1'b0;
    end else begin
      read_req_dly1 <= pu_buf_read_req;
    end
  end
    assign obuf_ld_stream_write_req = read_req_dly1;
    assign obuf_ld_stream_write_data = mem_read_data;
//==============================================================================

//==============================================================================
// Address generators
//==============================================================================
    assign mws_ld_stall = ~ldmem_tag_ready || ~axi_rd_ready;
    assign mws_ld_step = mws_ld_index_valid && !mws_ld_stall;

  mem_walker_stride #(
    .ADDR_WIDTH                     ( ADDR_WIDTH                     ),
    .ADDR_STRIDE_W                  ( ADDR_STRIDE_W                  ),
    .LOOP_ID_W                      ( LOOP_ID_W                      )
  ) mws_ld (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .base_addr                      ( mws_ld_base_addr               ), //input
    .loop_ctrl_done                 ( mws_ld_done                    ), //input
    .loop_index                     ( mws_ld_index                   ), //input
    .loop_index_valid               ( mws_ld_step                    ), //input
    .loop_init                      ( mws_ld_init                    ), //input
    .loop_enter                     ( mws_ld_enter                   ), //input
    .loop_exit                      ( mws_ld_exit                    ), //input
    .cfg_addr_stride_v              ( ld_stride_v                    ), //input
    .cfg_addr_stride                ( ld_stride                      ), //input
    .addr_out                       ( ld_addr                        ), //output
    .addr_out_valid                 ( ld_addr_v                      )  //output
  );
    assign mws_st_step = mws_st_index_valid && !mws_st_stall;
    assign mws_st_stall = ~stmem_tag_ready || ~axi_wr_ready;
    wire                                        _mws_st_done;
    assign _mws_st_done = mws_st_done || mws_ld_done; // Added for the cases when the mws_st is programmed but not used
  mem_walker_stride #(
    .ADDR_WIDTH                     ( ADDR_WIDTH                     ),
    .ADDR_STRIDE_W                  ( ADDR_STRIDE_W                  ),
    .LOOP_ID_W                      ( LOOP_ID_W                      )
  ) mws_st (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .base_addr                      ( mws_st_base_addr               ), //input
    .loop_ctrl_done                 ( _mws_st_done                   ), //input
    .loop_index                     ( mws_st_index                   ), //input
    .loop_index_valid               ( mws_st_step                    ), //input
    .loop_init                      ( mws_st_init                    ), //input
    .loop_enter                     ( mws_st_enter                   ), //input
    .loop_exit                      ( mws_st_exit                    ), //input
    .cfg_addr_stride_v              ( st_stride_v                    ), //input
    .cfg_addr_stride                ( st_stride                      ), //input
    .addr_out                       ( st_addr                        ), //output
    .addr_out_valid                 ( st_addr_v                      )  //output
    );

    //assign mws_st_step = mws_st_index_valid && !mws_st_stall;
    //assign mws_st_stall = ~stmem_tag_ready || ~axi_wr_ready;
//==============================================================================

//=============================================================
// Loop controller
//=============================================================
  always@(posedge clk)
  begin
    if (reset)
      ld_loop_id_counter <= 'b0;
    else begin
      if (mws_ld_loop_iter_v)
        ld_loop_id_counter <= ld_loop_id_counter + 1'b1;
      else if (tag_req && tag_ready)
        ld_loop_id_counter <= 'b0;
    end
  end

  always @(posedge clk)
  begin
    if (reset)
      ld_iter_v_q <= 1'b0;
    else begin
      if (cfg_loop_iter_v && cfg_loop_iter_loop_id == 1 + MEM_ID)
        ld_iter_v_q <= 1'b1;
      else if (cfg_loop_iter_v || ld_stride_v)
        ld_iter_v_q <= 1'b0;
    end
  end


  always @(posedge clk)
  begin
    if (reset)
      st_iter_v_q <= 1'b0;
    else begin
      if (cfg_loop_iter_v && cfg_loop_iter_loop_id == 1 + MEM_ID)
        st_iter_v_q <= 1'b1;
      else if (cfg_loop_iter_v || st_stride_v)
        st_iter_v_q <= 1'b0;
    end
  end

  always@(posedge clk)
  begin
    if (reset)
      st_loop_id_counter <= 'b0;
    else begin
      if (mws_st_loop_iter_v)
        st_loop_id_counter <= st_loop_id_counter + 1'b1;
      else if (tag_req && tag_ready)
        st_loop_id_counter <= 'b0;
    end
  end

    assign mws_ld_start = (ldmem_state_q == LDMEM_BUSY);
    assign mws_ld_loop_iter_v = ld_stride_v && ld_iter_v_q;
    assign mws_ld_loop_iter = iter_q;
    assign mws_ld_loop_iter_loop_id = ld_loop_id_counter;

  always @(posedge clk)
  begin
    if (reset) begin
      iter_q <= 'b0;
    end
    else if (cfg_loop_iter_v && cfg_loop_iter_loop_id == 1 + MEM_ID) begin
      iter_q <= cfg_loop_iter;
    end
  end

  controller_fsm #(
    .LOOP_ID_W                      ( LOOP_ID_W                      ),
    .LOOP_ITER_W                    ( LOOP_ITER_W                    ),
    .IMEM_ADDR_W                    ( LOOP_ID_W                      )
  ) mws_ld_ctrl (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .stall                          ( mws_ld_stall                   ), //input
    .cfg_loop_iter_v                ( mws_ld_loop_iter_v             ), //input
    .cfg_loop_iter                  ( mws_ld_loop_iter               ), //input
    .cfg_loop_iter_loop_id          ( mws_ld_loop_iter_loop_id       ), //input
    .start                          ( mws_ld_start                   ), //input
    .done                           ( mws_ld_done                    ), //output
    .loop_init                      ( mws_ld_init                    ), //output
    .loop_enter                     ( mws_ld_enter                   ), //output
    .loop_last_iter                 (                                ), //output
    .loop_exit                      ( mws_ld_exit                    ), //output
    .loop_index                     ( mws_ld_index                   ), //output
    .loop_index_valid               ( mws_ld_index_valid             )  //output
  );

    assign mws_st_loop_iter_loop_id = st_loop_id_counter;
    assign mws_st_start = stmem_state_q == STMEM_DDR;
    assign mws_st_loop_iter = iter_q;

    assign mws_st_loop_iter_v = st_stride_v && st_iter_v_q;

  controller_fsm #(
    .LOOP_ID_W                      ( LOOP_ID_W                      ),
    .LOOP_ITER_W                    ( LOOP_ITER_W                    ),
    .IMEM_ADDR_W                    ( LOOP_ID_W                      )
  ) mws_st_ctrl (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .stall                          ( mws_st_stall                   ), //input
    .cfg_loop_iter_v                ( mws_st_loop_iter_v             ), //input
    .cfg_loop_iter                  ( mws_st_loop_iter               ), //input
    .cfg_loop_iter_loop_id          ( mws_st_loop_iter_loop_id       ), //input
    .start                          ( mws_st_start                   ), //input
    .done                           ( mws_st_done                    ), //output
    .loop_init                      ( mws_st_init                    ), //output
    .loop_last_iter                 (                                ), //output
    .loop_enter                     ( mws_st_enter                   ), //output
    .loop_exit                      ( mws_st_exit                    ), //output
    .loop_index                     ( mws_st_index                   ), //output
    .loop_index_valid               ( mws_st_index_valid             )  //output
  );
//=============================================================

//==============================================================================
// Memory Request generation
//==============================================================================
    assign ld_mem_req_v = cfg_mem_req_v && cfg_mem_req_loop_id == (1 + MEM_ID) && cfg_mem_req_type == MEM_LD && cfg_mem_req_id == MEM_ID;
  always @(posedge clk)
  begin
    if (reset) begin
      ld_req_size <= 'b0;
    end
    else if (ld_mem_req_v) begin
      ld_req_size <= cfg_mem_req_size;
    end
  end

    assign st_mem_req_v = cfg_mem_req_v && cfg_mem_req_loop_id == (1 + MEM_ID) && cfg_mem_req_type == MEM_ST && cfg_mem_req_id == MEM_ID;
  always @(posedge clk)
  begin
    if (reset) begin
      st_req_size <= 'b0;
      st_req_loop_id <= 'b0;
    end
    else if (st_mem_req_v) begin
      st_req_size <= cfg_mem_req_size;
      st_req_loop_id <= st_loop_id_counter;
    end
  end

  // assign ld_req_valid_d = (ld_req_loop_id == mws_ld_index) && (mws_ld_enter || mws_ld_step);
    // assign ld_req_valid_d = (ld_req_loop_id == mws_ld_index) && ld_addr_v;
    assign ld_req_valid_d = ld_addr_v;
  // assign st_req_valid_d = STORE_ENABLED == 1 && (st_req_loop_id == mws_st_index) && (mws_st_step  || mws_st_exit);
    assign st_req_valid_d = STORE_ENABLED == 1 && (st_req_loop_id == mws_st_index) && st_addr_v;

  always @(posedge clk)
  begin
    if (reset) begin
      ld_req_valid_q <= 1'b0;
      ld_req_addr <= 'b0;
      st_req_valid_q <= 1'b0;
      st_req_addr <= 1'b0;
    end
    else begin
      ld_req_valid_q <= ld_req_valid_d;
      ld_req_addr <= ld_addr;
      st_req_valid_q <= st_req_valid_d;
      st_req_addr <= st_addr;
    end
  end

  //always @(posedge clk)
  //begin
  //  if (tag_req && tag_ready) begin
  //    tag_ld_addr[tag] <= tag_base_ld_addr;
  //    tag_st_addr[tag] <= tag_base_st_addr;
  //    tag_st_addr_copy[tag] <= tag_base_st_addr;
  //  end
  //end

  // wire [ 31                      : 0 ]        tag0_ld_addr;
  // wire [ 31                      : 0 ]        tag1_ld_addr;
  // wire [ 31                      : 0 ]        tag0_st_addr;
  // wire [ 31                      : 0 ]        tag1_st_addr;
  // assign tag0_ld_addr = tag_ld_addr[0];
  // assign tag1_ld_addr = tag_ld_addr[1];
  // assign tag0_st_addr = tag_st_addr[0];
  // assign tag1_st_addr = tag_st_addr[1];
//==============================================================================

//==============================================================================
// Tag-based synchronization for double buffering
//==============================================================================
    reg                                         raw;
    reg  [ TAG_W                -1 : 0 ]        raw_stmem_tag_d;
    reg  [ TAG_W                -1 : 0 ]        raw_stmem_tag_q;
    wire [ TAG_W                -1 : 0 ]        raw_stmem_tag;
    wire                                        raw_stmem_tag_ready;
    wire [ ADDR_WIDTH           -1 : 0 ]        raw_stmem_st_addr;

  always @(posedge clk)
  begin
    if (reset)
      raw_stmem_tag_q <= 0;
    else
      raw_stmem_tag_q <= raw_stmem_tag_d;
  end

    assign raw_stmem_tag = raw_stmem_tag_q;
    //assign raw_stmem_st_addr = tag_st_addr_copy[raw_stmem_tag];

  ram #(
    .ADDR_WIDTH                     ( $clog2(NUM_TAGS)),
    .DATA_WIDTH                     ( ADDR_WIDTH)
  ) u_ram_tag_st1(
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .s_write_addr                   ( tag),
    .s_write_req                    ( tag_req && tag_ready),
    .s_write_data                   ( tag_base_st_addr),
    .s_read_addr                    ( raw_stmem_tag),
    .s_read_req                     (1'b1 ),
    .s_read_data                    ( raw_stmem_st_addr)
  );

  ram #(
    .ADDR_WIDTH                     ( $clog2(NUM_TAGS)),
    .DATA_WIDTH                     ( ADDR_WIDTH)
  ) u_ram_tag_st2(
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .s_write_addr                   ( tag),
    .s_write_req                    ( tag_req && tag_ready),
    .s_write_data                   ( tag_base_st_addr),
    .s_read_addr                    ( stmem_tag),
    .s_read_req                     ( 1'b1),
    .s_read_data                    ( mws_st_base_addr)
  );

  ram #(
    .ADDR_WIDTH                     ( $clog2(NUM_TAGS)),
    .DATA_WIDTH                     ( ADDR_WIDTH)
  ) u_ram_tag_ld(
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .s_write_addr                   ( tag),
    .s_write_req                    ( tag_req && tag_ready),
    .s_write_data                   ( tag_base_ld_addr),
    .s_read_addr                    ( ldmem_tag),
    .s_read_req                     ( 1'b1),
    .s_read_data                    ( mws_ld_base_addr)
  );


    assign stmem_state = stmem_state_q;
  always @(*)
  begin
    ldmem_state_d = ldmem_state_q;
    raw_stmem_tag_d = raw_stmem_tag_q;
    case(ldmem_state_q)
      LDMEM_IDLE: begin
        if (ldmem_tag_ready) begin
          if (ldmem_tag == stmem_tag)
            ldmem_state_d = LDMEM_BUSY;
          else begin
            ldmem_state_d = LDMEM_CHECK_RAW;
            raw_stmem_tag_d = stmem_tag;
          end
        end
      end
      LDMEM_CHECK_RAW: begin
        if (raw_stmem_st_addr != mws_ld_base_addr)
          ldmem_state_d = LDMEM_BUSY;
        else if (stmem_state_q == STMEM_DONE)
          ldmem_state_d = LDMEM_IDLE;
      end
      LDMEM_BUSY: begin
        if (mws_ld_done)
          ldmem_state_d = LDMEM_WAIT_0;
      end
      LDMEM_WAIT_0: begin
        ldmem_state_d = LDMEM_WAIT_1;
      end
      LDMEM_WAIT_1: begin
        ldmem_state_d = LDMEM_WAIT_2;
      end
      LDMEM_WAIT_2: begin
        ldmem_state_d = LDMEM_WAIT_3;
      end
      LDMEM_WAIT_3: begin
        if (axi_rd_done)
          ldmem_state_d = LDMEM_DONE;
      end
      LDMEM_DONE: begin
        ldmem_state_d = LDMEM_IDLE;
      end
    endcase
  end

  always @(posedge clk)
  begin
    if (reset)
      ldmem_state_q <= LDMEM_IDLE;
    else
      ldmem_state_q <= ldmem_state_d;
  end

    reg                                         pu_start_d;
    reg                                         pu_start_q;
    assign pu_compute_start = pu_start_q;

  always @(posedge clk)
  begin
    if (reset)
      pu_start_q <= 1'b0;
    else
      pu_start_q <= pu_start_d;
  end

    localparam integer  WAIT_CYCLE_WIDTH             = $clog2(ARRAY_N) > 5 ? $clog2(ARRAY_N) : 5;
    reg  [ WAIT_CYCLE_WIDTH        : 0 ]        wait_cycles_d;
    reg  [ WAIT_CYCLE_WIDTH        : 0 ]        wait_cycles_q;

  always @(posedge clk)
  begin
    if (reset)
      wait_cycles_q <= 0;
    else
      wait_cycles_q <= wait_cycles_d;
  end

  always @(*)
  begin
    stmem_state_d = stmem_state_q;
    pu_start_d = 1'b0;
    wait_cycles_d = wait_cycles_q;
    case(stmem_state_q)
      STMEM_IDLE: begin
        if (stmem_tag_ready) begin
          stmem_state_d = STMEM_COMPUTE_WAIT;
          wait_cycles_d = ARRAY_N;
        end
      end
      STMEM_COMPUTE_WAIT: begin
        if (wait_cycles_q == 0) begin
          if (~stmem_ddr_pe_sw)
            stmem_state_d = STMEM_DDR;
          else if (pu_compute_ready) begin
            stmem_state_d = STMEM_PU;
            pu_start_d = 1'b1;
            wait_cycles_d = ARRAY_N + 1'b1;
          end
        end
        else
          wait_cycles_d = wait_cycles_q - 1'b1;
      end
      STMEM_PU: begin
        if (pu_compute_done)
          stmem_state_d = STMEM_DONE;
      end
      STMEM_DDR: begin
        if (mws_st_done) begin
          stmem_state_d = STMEM_DDR_WAIT;
          wait_cycles_d = 4;
        end
      end
      STMEM_DDR_WAIT: begin
        if (wait_cycles_q == 0) begin
          if (axi_wr_done) begin
            stmem_state_d = STMEM_DONE;
            wait_cycles_d = 0;
          end
        end else
          wait_cycles_d = wait_cycles_q - 1'b1;
      end
      STMEM_DONE: begin
        stmem_state_d = STMEM_IDLE;
      end
    endcase
  end

  always @(posedge clk)
  begin
    if (reset)
      stmem_state_q <= STMEM_IDLE;
    else
      stmem_state_q <= stmem_state_d;
  end


    wire                                        ldmem_ready;

    assign compute_tag_done = compute_done;
    assign compute_ready = compute_tag_ready;

    assign ldmem_tag_done = ldmem_state_q == LDMEM_DONE;
    assign ldmem_ready = ldmem_tag_ready;
  // assign ldmem_tag_done = mws_ld_done;

    assign stmem_tag_done = stmem_state_q == STMEM_DONE;

  tag_sync  #(
    .NUM_TAGS                       ( NUM_TAGS                       )
  )
  mws_tag (
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .block_done                     ( block_done                     ),
    .tag_req                        ( tag_req                        ),
    .tag_reuse                      ( tag_reuse                      ),
    .tag_bias_prev_sw               ( tag_bias_prev_sw               ),
    .tag_ddr_pe_sw                  ( tag_ddr_pe_sw                  ), //input
    .tag_ready                      ( tag_ready                      ),
    .tag                            ( tag                            ),
    .tag_done                       ( tag_done                       ),
    .raw_stmem_tag                  ( raw_stmem_tag_q                ),
    .raw_stmem_tag_ready            ( raw_stmem_tag_ready            ),
    .compute_tag_done               ( compute_tag_done               ),
    .compute_tag_ready              ( compute_tag_ready              ),
    .compute_bias_prev_sw           ( compute_bias_prev_sw           ),
    .compute_tag                    ( compute_tag                    ),
    .ldmem_tag_done                 ( ldmem_tag_done                 ),
    .ldmem_tag_ready                ( ldmem_tag_ready                ),
    .ldmem_tag                      ( ldmem_tag                      ),
    .stmem_ddr_pe_sw                ( stmem_ddr_pe_sw                ),
    .stmem_tag_done                 ( stmem_tag_done                 ),
    .stmem_tag_ready                ( stmem_tag_ready                ),
    .stmem_tag                      ( stmem_tag                      )
  );
//==============================================================================

//==============================================================================
// AXI4 Memory Mapped interface
//==============================================================================
    assign mem_write_ready = 1'b1;
    assign mem_read_ready = 1'b1;
    assign axi_rd_req_id = 0;
  axi_master #(
    .TX_SIZE_WIDTH                  ( MEM_REQ_W                      ),
    .AXI_DATA_WIDTH                 ( AXI_DATA_WIDTH                 ),
    .AXI_ADDR_WIDTH                 ( AXI_ADDR_WIDTH                 ),
    .AXI_BURST_WIDTH                ( AXI_BURST_WIDTH                )
  ) u_axi_mm_master (
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .m_axi_awaddr                   ( mws_awaddr                     ),
    .m_axi_awlen                    ( mws_awlen                      ),
    .m_axi_awsize                   ( mws_awsize                     ),
    .m_axi_awburst                  ( mws_awburst                    ),
    .m_axi_awvalid                  ( mws_awvalid                    ),
    .m_axi_awready                  ( mws_awready                    ),
    .m_axi_wdata                    ( mws_wdata                      ),
    .m_axi_wstrb                    ( mws_wstrb                      ),
    .m_axi_wlast                    ( mws_wlast                      ),
    .m_axi_wvalid                   ( mws_wvalid                     ),
    .m_axi_wready                   ( mws_wready                     ),
    .m_axi_bresp                    ( mws_bresp                      ),
    .m_axi_bvalid                   ( mws_bvalid                     ),
    .m_axi_bready                   ( mws_bready                     ),
    .m_axi_araddr                   ( mws_araddr                     ),
    .m_axi_arid                     ( mws_arid                       ),
    .m_axi_arlen                    ( mws_arlen                      ),
    .m_axi_arsize                   ( mws_arsize                     ),
    .m_axi_arburst                  ( mws_arburst                    ),
    .m_axi_arvalid                  ( mws_arvalid                    ),
    .m_axi_arready                  ( mws_arready                    ),
    .m_axi_rdata                    ( mws_rdata                      ),
    .m_axi_rid                      ( mws_rid                        ),
    .m_axi_rresp                    ( mws_rresp                      ),
    .m_axi_rlast                    ( mws_rlast                      ),
    .m_axi_rvalid                   ( mws_rvalid                     ),
    .m_axi_rready                   ( mws_rready                     ),
    // Buffer
    .mem_write_id                   ( mem_write_id                   ),
    .mem_write_req                  ( mem_write_req                  ),
    .mem_write_data                 ( mem_write_data                 ),
    .mem_write_ready                ( mem_write_ready                ),
    .mem_read_data                  ( mem_read_data                  ),
    .mem_read_req                   ( axi_mem_read_req               ),
    .mem_read_ready                 ( axi_mem_read_ready             ),
    // AXI RD Req
    .rd_req                         ( axi_rd_req                     ),
    .rd_req_id                      ( axi_rd_req_id                  ),
    .rd_done                        ( axi_rd_done                    ),
    .rd_ready                       ( axi_rd_ready                   ),
    .rd_req_size                    ( axi_rd_req_size                ),
    .rd_addr                        ( axi_rd_addr                    ),
    // AXI WR Req
    .wr_req                         ( axi_wr_req                     ),
    .wr_req_id                      ( axi_wr_req_id                  ),
    .wr_ready                       ( axi_wr_ready                   ),
    .wr_req_size                    ( axi_wr_req_size                ),
    .wr_addr                        ( axi_wr_addr                    ),
    .wr_done                        ( axi_wr_done                    )
  );
//==============================================================================

//==============================================================================
// Dual-port RAM
//==============================================================================
  always @(posedge clk)
  begin
    if (reset)
      axi_mem_read_addr <= 0;
    else begin
      if (mem_read_req)
        axi_mem_read_addr <= axi_mem_read_addr + 1'b1;
      else if (stmem_state_q == STMEM_DONE)
        axi_mem_read_addr <= 0;
    end
  end

    assign mem_read_addr = stmem_state_q == STMEM_PU ? pu_buf_read_addr : axi_mem_read_addr;
    assign mem_read_req = stmem_state_q == STMEM_PU ? pu_buf_read_req : axi_mem_read_req;
    assign axi_mem_read_ready = stmem_state_q != STMEM_PU;
    assign pu_buf_read_ready = stmem_state_q == STMEM_PU;

  always @(posedge clk)
  begin
    if (reset)
      mem_write_addr <= 0;
    else begin
      if (mem_write_req)
        mem_write_addr <= mem_write_addr + 1'b1;
      else if (ldmem_state_q == LDMEM_DONE)
        mem_write_addr <= 0;
    end
  end

    wire [ TAG_MEM_ADDR_W       -1 : 0 ]        tag_mem_read_addr;
    wire [ TAG_MEM_ADDR_W       -1 : 0 ]        tag_mem_write_addr;

    wire [ TAG_BUF_ADDR_W       -1 : 0 ]        tag_buf_read_addr;
    wire [ TAG_BUF_ADDR_W       -1 : 0 ]        tag_buf_write_addr;

    assign tag_mem_read_addr = {stmem_tag, mem_read_addr};
    assign tag_mem_write_addr = {ldmem_tag, mem_write_addr};

  genvar i;
  generate
    if (MEM_ID == 1 || MEM_ID == 3)
    begin: OBUF_TAG_DELAY
      for (i=0; i<ARRAY_N+3; i=i+1)
      begin: TAG_DELAY_LOOP
        wire [TAG_W-1:0] prev_tag, next_tag;
        if (i==0)
    assign prev_tag = compute_tag;
        else
    assign prev_tag = OBUF_TAG_DELAY.TAG_DELAY_LOOP[i-1].next_tag;
        register_sync #(TAG_W) tag_delay (clk, reset, prev_tag, next_tag);
      end
      // Increased compute tag delays
      // Might need a separate compute_tag_delayed for buf_read
    assign compute_tag_delayed = OBUF_TAG_DELAY.TAG_DELAY_LOOP[ARRAY_N+2].next_tag;
    end
    else begin
    assign compute_tag_delayed = compute_tag;
    end
  endgenerate

    assign tag_buf_read_addr = {compute_tag_delayed, buf_read_addr};
    assign tag_buf_write_addr = {compute_tag_delayed, buf_write_addr};

  register_sync #(BUF_DATA_WIDTH)
  buf_read_data_delay (clk, reset, _buf_read_data, buf_read_data);

  obuf #(
    .TAG_W                          ( TAG_W                          ),
    .BUF_ADDR_WIDTH                 ( TAG_BUF_ADDR_W                 ),
    .ARRAY_M                        ( ARRAY_M                        ),
    .MEM_DATA_WIDTH                 ( AXI_DATA_WIDTH                 ),
    .DATA_WIDTH                     ( DATA_WIDTH                     )
  ) buf_ram (
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .mem_write_addr                 ( tag_mem_write_addr             ),
    .mem_write_req                  ( mem_write_req                  ),
    .mem_write_data                 ( mem_write_data                 ),
    .mem_read_addr                  ( tag_mem_read_addr              ),
    .mem_read_req                   ( mem_read_req                   ),
    .mem_read_data                  ( mem_read_data                  ),
    .buf_write_addr                 ( tag_buf_write_addr             ),
    .buf_write_req                  ( buf_write_req                  ),
    .buf_write_data                 ( buf_write_data                 ),
    .buf_read_addr                  ( tag_buf_read_addr              ),
    .buf_read_req                   ( buf_read_req                   ),
    .buf_read_data                  ( _buf_read_data                 )
  );
//==============================================================================

//==============================================================================
`ifdef COCOTB_SIM
  integer wr_req_count=0;
  integer rd_req_count=0;
  integer missed_rd_req_count=0;
  integer req_count;

  always @(posedge clk)
    if (reset)
      wr_req_count <= 0;
    else
      wr_req_count <= wr_req_count + axi_wr_req;

  always @(posedge clk)
    if (reset)
      rd_req_count <= 0;
    else
      rd_req_count <= rd_req_count + axi_rd_req;

  always @(posedge clk)
    if (reset)
      missed_rd_req_count <= 0;
    else
      missed_rd_req_count <= missed_rd_req_count + (axi_rd_req && ~axi_rd_ready);

  always @(posedge clk)
  begin
    if (reset) req_count <= 0;
    else req_count = req_count + (tag_req && tag_ready);
  end
`endif
//==============================================================================

//=============================================================
// VCD
//=============================================================
`ifdef COCOTB_TOPLEVEL_mem_wrapper
initial begin
  $dumpfile("mem_wrapper.vcd");
  $dumpvars(0, mem_wrapper);
end
`endif
//=============================================================
endmodule


//
// Register
//
// Hardik Sharma
// (hsharma@gatech.edu)

`timescale 1ns/1ps
module register_sync #(
  parameter integer WIDTH                 = 8
) (
  input  wire                             clk,
  input  wire                             reset,
  // input  wire                             enable,
  input  wire        [ WIDTH -1 : 0 ]     in,
  output wire        [ WIDTH -1 : 0 ]     out
);

  reg [ WIDTH -1 : 0 ] out_reg;

  always @(posedge clk)
  begin
    // if (reset)
      // out_reg <= 'b0;
    // else if (enable)
      out_reg <= in;
  end

  assign out = out_reg;

endmodule
`timescale 1ns/1ps
module axi4lite_slave #
(
    parameter integer  AXIS_ADDR_WIDTH              = 32,
    parameter integer  AXIS_DATA_WIDTH              = 32,
    parameter integer  AXIS_WSTRB_WIDTH             = AXIS_DATA_WIDTH/8,
    parameter integer  ADDRBUF_ADDR_WIDTH           = 10
)
(
  // Slave registers
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg0_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg0_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg1_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg1_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg2_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg2_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg3_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg3_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg4_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg4_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg5_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg5_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg6_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg6_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg7_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg7_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg8_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg8_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg9_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg9_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg10_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg10_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg11_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg11_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg12_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg12_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg13_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg13_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg14_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg14_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg15_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg15_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg16_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg16_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg17_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg17_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg18_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg18_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg19_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg19_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg20_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg20_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg21_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg21_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg22_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg22_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg23_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg23_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg24_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg24_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg25_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg25_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg26_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg26_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg27_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg27_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg28_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg28_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg29_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg29_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg30_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg30_out,
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg31_in,
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        slv_reg31_out,

    input  wire                                         decoder_start,
    input  wire  [ AXIS_ADDR_WIDTH      -1 : 0 ]        ibuf_rd_addr,
    input  wire                                         ibuf_rd_addr_v,
    input  wire  [ AXIS_ADDR_WIDTH      -1 : 0 ]        obuf_wr_addr,
    input  wire                                         obuf_wr_addr_v,
    input  wire  [ AXIS_ADDR_WIDTH      -1 : 0 ]        obuf_rd_addr,
    input  wire                                         obuf_rd_addr_v,
    input  wire  [ AXIS_ADDR_WIDTH      -1 : 0 ]        wbuf_rd_addr,
    input  wire                                         wbuf_rd_addr_v,
    input  wire  [ AXIS_ADDR_WIDTH      -1 : 0 ]        bias_rd_addr,
    input  wire                                         bias_rd_addr_v,
  // Slave registers end

    input  wire                                         clk,
    input  wire                                         resetn,
    // Slave Write address
    input  wire  [ AXIS_ADDR_WIDTH      -1 : 0 ]        s_axi_awaddr,
    input  wire                                         s_axi_awvalid,
    output wire                                         s_axi_awready,
    // Slave Write data
    input  wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        s_axi_wdata,
    input  wire  [ AXIS_WSTRB_WIDTH     -1 : 0 ]        s_axi_wstrb,
    input  wire                                         s_axi_wvalid,
    output wire                                         s_axi_wready,
    //Write response
    output wire  [ 1                       : 0 ]        s_axi_bresp,
    output wire                                         s_axi_bvalid,
    input  wire                                         s_axi_bready,
    //Read address
    input  wire  [ AXIS_ADDR_WIDTH      -1 : 0 ]        s_axi_araddr,
    input  wire                                         s_axi_arvalid,
    output wire                                         s_axi_arready,
    //Read data/response
    output wire  [ AXIS_DATA_WIDTH      -1 : 0 ]        s_axi_rdata,
    output wire  [ 1                       : 0 ]        s_axi_rresp,
    output wire                                         s_axi_rvalid,
    input  wire                                         s_axi_rready

);

//=============================================================
// Localparams
//=============================================================
    localparam integer  ADDR_LSB                     = (AXIS_DATA_WIDTH/32) + 1;
    localparam integer  OPT_MEM_ADDR_BITS            = 5;

    localparam          LOGIC                        = 0;
    localparam          IBUF_RD_ADDR                 = 1;
    localparam          OBUF_RD_ADDR                 = 2;
    localparam          OBUF_WR_ADDR                 = 3;
    localparam          WBUF_RD_ADDR                 = 4;
    localparam          BIAS_RD_ADDR                 = 5;
//=============================================================

//=============================================================
// Wires/Regs
//=============================================================
    integer                                     byte_index;

    wire                                        reset;

    wire [ ADDRBUF_ADDR_WIDTH   -1 : 0 ]        ibuf_rd_addr_rd_ptr;
    wire [ ADDRBUF_ADDR_WIDTH   -1 : 0 ]        obuf_rd_addr_rd_ptr;
    wire [ ADDRBUF_ADDR_WIDTH   -1 : 0 ]        obuf_wr_addr_rd_ptr;
    wire [ ADDRBUF_ADDR_WIDTH   -1 : 0 ]        wbuf_rd_addr_rd_ptr;
    wire [ ADDRBUF_ADDR_WIDTH   -1 : 0 ]        bias_rd_addr_rd_ptr;

    reg  [ ADDRBUF_ADDR_WIDTH   -1 : 0 ]        ibuf_rd_addr_wr_ptr;
    reg  [ ADDRBUF_ADDR_WIDTH   -1 : 0 ]        obuf_rd_addr_wr_ptr;
    reg  [ ADDRBUF_ADDR_WIDTH   -1 : 0 ]        obuf_wr_addr_wr_ptr;
    reg  [ ADDRBUF_ADDR_WIDTH   -1 : 0 ]        wbuf_rd_addr_wr_ptr;
    reg  [ ADDRBUF_ADDR_WIDTH   -1 : 0 ]        bias_rd_addr_wr_ptr;

    wire [ AXIS_ADDR_WIDTH      -1 : 0 ]        ibuf_rd_addr_rdata;
    wire [ AXIS_ADDR_WIDTH      -1 : 0 ]        obuf_wr_addr_rdata;
    wire [ AXIS_ADDR_WIDTH      -1 : 0 ]        obuf_rd_addr_rdata;
    wire [ AXIS_ADDR_WIDTH      -1 : 0 ]        wbuf_rd_addr_rdata;
    wire [ AXIS_ADDR_WIDTH      -1 : 0 ]        bias_rd_addr_rdata;
  // AXI4LITE signals
    reg  [ AXIS_ADDR_WIDTH      -1 : 0 ]        axi_awaddr;
    reg                                         axi_awready;
    reg                                         axi_wready;
    reg  [ 1                       : 0 ]        axi_bresp;
    reg                                         axi_bvalid;
    reg  [ AXIS_ADDR_WIDTH      -1 : 0 ]        axi_araddr;
    reg                                         axi_arready;
    wire [ AXIS_DATA_WIDTH      -1 : 0 ]        axi_rdata;
    reg  [ 1                       : 0 ]        axi_rresp;
    reg                                         axi_rvalid;
    wire                                        slv_reg_rden;
    wire                                        slv_reg_wren;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        reg_data_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg0_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg1_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg2_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg3_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg4_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg5_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg6_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg7_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg8_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg9_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg10_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg11_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg12_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg13_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg14_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg15_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg16_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg17_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg18_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg19_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg20_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg21_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg22_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg23_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg24_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg25_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg26_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg27_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg28_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg29_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg30_out;
    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        _slv_reg31_out;
    wire [ 3                    -1 : 0 ]        addr_type;
//    reg  [ 3                    -1 : 0 ]        addr_type_delay;

    reg  [ AXIS_DATA_WIDTH      -1 : 0 ]        logic_data;
//=============================================================

//=============================================================
// Assigns
//=============================================================
  // I/O Connections assignments
    assign s_axi_awready    = axi_awready;
    assign s_axi_wready    = axi_wready;
    assign s_axi_bresp    = axi_bresp;
    assign s_axi_bvalid    = axi_bvalid;
    assign s_axi_arready    = axi_arready;
    assign s_axi_rdata    = axi_rdata;
    assign s_axi_rresp    = axi_rresp;
    assign s_axi_rvalid    = axi_rvalid;

    assign reset = ~resetn;
//=============================================================

//=============================================================
// Main logic
//=============================================================
  // Implement axi_awready generation
    // axi_awready is asserted for one clk clock cycle when both
    // s_axi_awvalid and s_axi_wvalid are asserted. axi_awready is
    // de-asserted when reset is low.

  always @( posedge clk )
  begin
    if ( resetn == 1'b0 )
    begin
      axi_awready <= 1'b0;
    end
    else
    begin
      if (~axi_awready && s_axi_awvalid && s_axi_wvalid)
      begin
        // slave is ready to accept write address when
        // there is a valid write address and write data
        // on the write address and data bus. This design
        // expects no outstanding transactions.
        axi_awready <= 1'b1;
      end
      else
      begin
        axi_awready <= 1'b0;
      end
    end
  end

  // Implement axi_awaddr latching
    // This process is used to latch the address when both
    // s_axi_awvalid and s_axi_wvalid are valid.

  always @( posedge clk )
  begin
    if ( resetn == 1'b0 )
    begin
      axi_awaddr <= 0;
    end
    else
    begin
      if (~axi_awready && s_axi_awvalid && s_axi_wvalid)
      begin
        // Write Address latching
        axi_awaddr <= s_axi_awaddr;
      end
    end
  end

  // Implement axi_wready generation
    // axi_wready is asserted for one clk clock cycle when both
    // s_axi_awvalid and s_axi_wvalid are asserted. axi_wready is
    // de-asserted when reset is low.

  always @( posedge clk )
  begin
    if ( resetn == 1'b0 )
    begin
      axi_wready <= 1'b0;
    end
    else
    begin
      if (~axi_wready && s_axi_wvalid && s_axi_awvalid)
      begin
        // slave is ready to accept write data when
        // there is a valid write address and write data
        // on the write address and data bus. This design
        // expects no outstanding transactions.
        axi_wready <= 1'b1;
      end
      else
      begin
        axi_wready <= 1'b0;
      end
    end
  end

  // Implement memory mapped register select and write logic generation
    // The write data is accepted and written to memory mapped registers when
    // axi_awready, s_axi_wvalid, axi_wready and s_axi_wvalid are asserted. Write strobes are used to
    // select byte enables of slave registers while writing.
    // These registers are cleared when reset (active low) is applied.
    // Slave register write enable is asserted when valid address and data are available
    // and the slave is ready to accept the write address and write data.
    assign slv_reg_wren = axi_wready && s_axi_wvalid && axi_awready && s_axi_awvalid;


    assign slv_reg0_out = _slv_reg0_out;
    assign slv_reg1_out = _slv_reg1_out;
    assign slv_reg2_out = _slv_reg2_out;
    assign slv_reg3_out = _slv_reg3_out;
    assign slv_reg4_out = _slv_reg4_out;
    assign slv_reg5_out = _slv_reg5_out;
    assign slv_reg6_out = _slv_reg6_out;
    assign slv_reg7_out = _slv_reg7_out;
    assign slv_reg8_out = _slv_reg8_out;
    assign slv_reg9_out = _slv_reg9_out;
    assign slv_reg10_out = _slv_reg10_out;
    assign slv_reg11_out = _slv_reg11_out;
    assign slv_reg12_out = _slv_reg12_out;
    assign slv_reg13_out = _slv_reg13_out;
    assign slv_reg14_out = _slv_reg14_out;
    assign slv_reg15_out = _slv_reg15_out;
    assign slv_reg16_out = _slv_reg16_out;
    assign slv_reg17_out = _slv_reg17_out;
    assign slv_reg18_out = _slv_reg18_out;
    assign slv_reg19_out = _slv_reg19_out;
    assign slv_reg20_out = _slv_reg20_out;
    assign slv_reg21_out = _slv_reg21_out;
    assign slv_reg22_out = _slv_reg22_out;
    assign slv_reg23_out = _slv_reg23_out;
    assign slv_reg24_out = _slv_reg24_out;
    assign slv_reg25_out = _slv_reg25_out;
    assign slv_reg26_out = _slv_reg26_out;
    assign slv_reg27_out = _slv_reg27_out;
    assign slv_reg28_out = _slv_reg28_out;
    assign slv_reg29_out = _slv_reg29_out;
    assign slv_reg30_out = _slv_reg30_out;
    assign slv_reg31_out = _slv_reg31_out;

  always @( posedge clk )
  begin
    if ( resetn == 1'b0 )
    begin
      _slv_reg0_out <= 0;
      _slv_reg1_out <= 0;
      _slv_reg2_out <= 0;
      _slv_reg3_out <= 0;
      _slv_reg4_out <= 0;
      _slv_reg5_out <= 0;
      _slv_reg6_out <= 0;
      _slv_reg7_out <= 0;

      _slv_reg8_out <= 0;
      _slv_reg9_out <= 0;
      _slv_reg10_out <= 0;
      _slv_reg11_out <= 0;
      _slv_reg12_out <= 0;
      _slv_reg13_out <= 0;
      _slv_reg14_out <= 0;
      _slv_reg15_out <= 0;

      _slv_reg16_out <= 0;
      _slv_reg17_out <= 0;
      _slv_reg18_out <= 0;
      _slv_reg19_out <= 0;
      _slv_reg20_out <= 0;
      _slv_reg21_out <= 0;
      _slv_reg22_out <= 0;
      _slv_reg23_out <= 0;

      _slv_reg24_out <= 0;
      _slv_reg25_out <= 0;
      _slv_reg26_out <= 0;
      _slv_reg27_out <= 0;
      _slv_reg28_out <= 0;
      _slv_reg29_out <= 0;
      _slv_reg30_out <= 0;
      _slv_reg31_out <= 0;
    end
    else begin
      if (slv_reg_wren)
      begin
        case ( axi_awaddr[ADDR_LSB+OPT_MEM_ADDR_BITS:ADDR_LSB] )
          5'd0:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg0_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd1:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg1_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd2:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg2_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd3:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg3_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd4:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg4_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd5:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg5_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd6:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg6_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd7:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg7_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd8:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg8_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd9:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg9_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd10:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg10_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd11:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg11_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd12:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg12_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd13:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg13_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd14:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg14_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd15:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg15_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd16:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg16_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd17:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg17_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd18:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg18_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd19:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg19_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd20:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg20_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd21:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg21_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd22:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg22_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd23:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg23_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd24:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg24_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd25:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg25_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd26:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg26_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd27:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg27_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd28:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg28_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd29:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg29_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd30:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg30_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          5'd31:
            for ( byte_index = 0; byte_index <= (AXIS_DATA_WIDTH/8)-1; byte_index = byte_index+1 )
              if ( s_axi_wstrb[byte_index] == 1 ) begin
                _slv_reg31_out[(byte_index*8) +: 8] <= s_axi_wdata[(byte_index*8) +: 8];
              end
          default : begin

            _slv_reg0_out <= _slv_reg0_out;
            _slv_reg1_out <= _slv_reg1_out;
            _slv_reg2_out <= _slv_reg2_out;
            _slv_reg3_out <= _slv_reg3_out;
            _slv_reg4_out <= _slv_reg4_out;
            _slv_reg5_out <= _slv_reg5_out;
            _slv_reg6_out <= _slv_reg6_out;
            _slv_reg7_out <= _slv_reg7_out;

            _slv_reg8_out <= _slv_reg8_out;
            _slv_reg9_out <= _slv_reg9_out;
            _slv_reg10_out <= _slv_reg10_out;
            _slv_reg11_out <= _slv_reg11_out;
            _slv_reg12_out <= _slv_reg12_out;
            _slv_reg13_out <= _slv_reg13_out;
            _slv_reg14_out <= _slv_reg14_out;
            _slv_reg15_out <= _slv_reg15_out;

            _slv_reg16_out <= _slv_reg16_out;
            _slv_reg17_out <= _slv_reg17_out;
            _slv_reg18_out <= _slv_reg18_out;
            _slv_reg19_out <= _slv_reg19_out;
            _slv_reg20_out <= _slv_reg20_out;
            _slv_reg21_out <= _slv_reg21_out;
            _slv_reg22_out <= _slv_reg22_out;
            _slv_reg23_out <= _slv_reg23_out;

            _slv_reg24_out <= _slv_reg24_out;
            _slv_reg25_out <= _slv_reg25_out;
            _slv_reg26_out <= _slv_reg26_out;
            _slv_reg27_out <= _slv_reg27_out;
            _slv_reg28_out <= _slv_reg28_out;
            _slv_reg29_out <= _slv_reg29_out;
            _slv_reg30_out <= _slv_reg30_out;
            _slv_reg31_out <= _slv_reg31_out;

          end
        endcase
      end
    end
  end

  // Implement write response logic generation
    // The write response and response valid signals are asserted by the slave
    // when axi_wready, s_axi_wvalid, axi_wready and s_axi_wvalid are asserted.
    // This marks the acceptance of address and indicates the status of
    // write transaction.

  always @( posedge clk )
  begin
    if ( resetn == 1'b0 )
    begin
      axi_bvalid  <= 0;
      axi_bresp   <= 2'b0;
    end
    else
    begin
      if (axi_awready && s_axi_awvalid && ~axi_bvalid && axi_wready && s_axi_wvalid)
      begin
        // indicates a valid write response is available
        axi_bvalid <= 1'b1;
        axi_bresp  <= 2'b0; // 'OKAY' response
      end                   // work error responses in future
      else
      begin
        if (s_axi_bready && axi_bvalid)
          //check if bready is asserted while bvalid is high)
          //(there is a possibility that bready is always asserted high)
        begin
          axi_bvalid <= 1'b0;
        end
      end
    end
  end

  // Implement axi_arready generation
    // axi_arready is asserted for one clk clock cycle when
    // s_axi_arvalid is asserted. axi_awready is
    // de-asserted when reset (active low) is asserted.
    // The read address is also latched when s_axi_arvalid is
    // asserted. axi_araddr is reset to zero on reset assertion.

  always @( posedge clk )
  begin
    if ( resetn == 1'b0 )
    begin
      axi_arready <= 1'b0;
      axi_araddr  <= 32'b0;
    end
    else
    begin
      if (~axi_arready && s_axi_arvalid)
      begin
        // indicates that the slave has acceped the valid read address
        axi_arready <= 1'b1;
        // Read address latching
        axi_araddr  <= s_axi_araddr;
      end
      else
      begin
        axi_arready <= 1'b0;
      end
    end
  end

  // Implement axi_arvalid generation
    // axi_rvalid is asserted for one clk clock cycle when both
    // s_axi_arvalid and axi_arready are asserted. The slave registers
    // data are available on the axi_rdata bus at this instance. The
    // assertion of axi_rvalid marks the validity of read data on the
    // bus and axi_rresp indicates the status of read transaction.axi_rvalid
    // is deasserted on reset (active low). axi_rresp and axi_rdata are
    // cleared to zero on reset (active low).
  always @( posedge clk )
  begin
    if ( resetn == 1'b0 )
    begin
      axi_rvalid <= 0;
      axi_rresp  <= 0;
    end
    else
    begin
      if (axi_arready && s_axi_arvalid && ~axi_rvalid)
      begin
        // Valid read data is available at the read data bus
        axi_rvalid <= 1'b1;
        axi_rresp  <= 2'b0; // 'OKAY' response
      end
      else if (axi_rvalid && s_axi_rready)
      begin
        // Read data is accepted by the master
        axi_rvalid <= 1'b0;
      end
    end
  end

  // Implement memory mapped register select and read logic generation
    // Slave register read enable is asserted when valid address is available
    // and the slave is ready to accept the read address.
    assign slv_reg_rden = axi_arready & s_axi_arvalid & ~axi_rvalid;


    assign addr_type = axi_araddr[ADDR_LSB+ADDRBUF_ADDR_WIDTH+2:ADDR_LSB+ADDRBUF_ADDR_WIDTH];

//  always @(posedge clk)
//    addr_type_delay <= addr_type;


  always @(*)
  begin
    // case (addr_type)
    // LOGIC: begin
    case (axi_araddr[ADDR_LSB+OPT_MEM_ADDR_BITS:ADDR_LSB])
      5'd0    : reg_data_out = slv_reg0_in;
      5'd1    : reg_data_out = slv_reg1_in;
      5'd2    : reg_data_out = slv_reg2_in;
      5'd3    : reg_data_out = slv_reg3_in;
      5'd4    : reg_data_out = slv_reg4_in;
      5'd5    : reg_data_out = slv_reg5_in;
      5'd6    : reg_data_out = slv_reg6_in;
      5'd7    : reg_data_out = slv_reg7_in;
      5'd8    : reg_data_out = slv_reg8_in;
      5'd9    : reg_data_out = slv_reg9_in;
      5'd10   : reg_data_out = slv_reg10_in;
      5'd11   : reg_data_out = slv_reg11_in;
      5'd12   : reg_data_out = slv_reg12_in;
      5'd13   : reg_data_out = slv_reg13_in;
      5'd14   : reg_data_out = slv_reg14_in;
      5'd15   : reg_data_out = slv_reg15_in;
      5'd16   : reg_data_out = slv_reg16_in;
      5'd17   : reg_data_out = slv_reg17_in;
      5'd18   : reg_data_out = slv_reg18_in;
      5'd19   : reg_data_out = slv_reg19_in;
      5'd20   : reg_data_out = slv_reg20_in;
      5'd21   : reg_data_out = slv_reg21_in;
      5'd22   : reg_data_out = slv_reg22_in;
      5'd23   : reg_data_out = slv_reg23_in;
      5'd24   : reg_data_out = slv_reg24_in;
      5'd25   : reg_data_out = slv_reg25_in;
      5'd26   : reg_data_out = slv_reg26_in;
      5'd27   : reg_data_out = slv_reg27_in;
      5'd28   : reg_data_out = slv_reg28_in;
      5'd29   : reg_data_out = slv_reg29_in;
      5'd30   : reg_data_out = slv_reg30_in;
      5'd31   : reg_data_out = slv_reg31_in;
      default : reg_data_out = 32'hDEADBEEF;
    endcase
    // end
    // IBUF_RD_ADDR : reg_data_out = ibuf_rd_addr_rdata;
    //OBUF_WR_ADDR : reg_data_out = obuf_wr_addr_rdata;
    // OBUF_RD_ADDR : reg_data_out = obuf_rd_addr_rdata;
    // WBUF_RD_ADDR : reg_data_out = wbuf_rd_addr_rdata;
    // BIAS_RD_ADDR : reg_data_out = bias_rd_addr_rdata;
    // default      : reg_data_out = 32'hDEADBEEF;
    // endcase
  end

  // Output register or memory read data
  always @( posedge clk )
  begin
    if ( resetn == 1'b0 )
    begin
      logic_data  <= 0;
    end
    else
    begin
      // When there is a valid read address (s_axi_arvalid) with
      // acceptance of read address by the slave (axi_arready),
      // output the read dada
      if (slv_reg_rden)
      begin
        logic_data <= reg_data_out;     // register read data
      end
    end
  end

    assign axi_rdata = logic_data;

  //
  //                 addr_type_delay == IBUF_RD_ADDR ? ibuf_rd_addr_rdata :
  //                 addr_type_delay == OBUF_WR_ADDR ? obuf_wr_addr_rdata :
  //                 addr_type_delay == OBUF_RD_ADDR ? obuf_rd_addr_rdata :
  //                 addr_type_delay == WBUF_RD_ADDR ? wbuf_rd_addr_rdata :
  //                                                   bias_rd_addr_rdata;
  //

    assign ibuf_rd_addr_rd_ptr = s_axi_araddr[ADDR_LSB+ADDRBUF_ADDR_WIDTH-1:ADDR_LSB];
    assign obuf_wr_addr_rd_ptr = s_axi_araddr[ADDR_LSB+ADDRBUF_ADDR_WIDTH-1:ADDR_LSB];
    assign obuf_rd_addr_rd_ptr = s_axi_araddr[ADDR_LSB+ADDRBUF_ADDR_WIDTH-1:ADDR_LSB];
    assign wbuf_rd_addr_rd_ptr = s_axi_araddr[ADDR_LSB+ADDRBUF_ADDR_WIDTH-1:ADDR_LSB];
    assign bias_rd_addr_rd_ptr = s_axi_araddr[ADDR_LSB+ADDRBUF_ADDR_WIDTH-1:ADDR_LSB];

  always @(posedge clk)
  begin
    if (~resetn)
      ibuf_rd_addr_wr_ptr <= 0;
    else begin
      if (decoder_start)
        ibuf_rd_addr_wr_ptr <= 0;
      else if (ibuf_rd_addr_v)
        ibuf_rd_addr_wr_ptr <= ibuf_rd_addr_wr_ptr + 1'b1;
    end
  end

  always @(posedge clk)
  begin
    if (~resetn)
      obuf_rd_addr_wr_ptr <= 0;
    else begin
      if (decoder_start)
        obuf_rd_addr_wr_ptr <= 0;
      else if (obuf_rd_addr_v)
        obuf_rd_addr_wr_ptr <= obuf_rd_addr_wr_ptr + 1'b1;
    end
  end

  always @(posedge clk)
  begin
    if (~resetn)
      obuf_wr_addr_wr_ptr <= 0;
    else begin
      if (decoder_start)
        obuf_wr_addr_wr_ptr <= 0;
      else if (obuf_wr_addr_v)
        obuf_wr_addr_wr_ptr <= obuf_wr_addr_wr_ptr + 1'b1;
    end
  end

  always @(posedge clk)
  begin
    if (~resetn)
      wbuf_rd_addr_wr_ptr <= 0;
    else begin
      if (decoder_start)
        wbuf_rd_addr_wr_ptr <= 0;
      else if (wbuf_rd_addr_v)
        wbuf_rd_addr_wr_ptr <= wbuf_rd_addr_wr_ptr + 1'b1;
    end
  end

  always @(posedge clk)
  begin
    if (~resetn)
      bias_rd_addr_wr_ptr <= 0;
    else begin
      if (decoder_start)
        bias_rd_addr_wr_ptr <= 0;
      else if (bias_rd_addr_v)
        bias_rd_addr_wr_ptr <= bias_rd_addr_wr_ptr + 1'b1;
    end
  end
//=============================================================

//=============================================================
// RAMs to store addresses
//=============================================================
  ram #(
    .DATA_WIDTH                     ( AXIS_ADDR_WIDTH                ),
    .ADDR_WIDTH                     ( ADDRBUF_ADDR_WIDTH             ),
    .OUTPUT_REG                     ( 1                              )
  ) u_ibuf_rd_ram (
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .s_read_req                     ( s_axi_arvalid && ~s_axi_arready ),
    .s_read_addr                    ( ibuf_rd_addr_rd_ptr            ),
    .s_read_data                    ( ibuf_rd_addr_rdata             ),
    .s_write_req                    ( ibuf_rd_addr_v                 ),
    .s_write_addr                   ( ibuf_rd_addr_wr_ptr            ),
    .s_write_data                   ( ibuf_rd_addr                   )
  );

  ram #(
    .DATA_WIDTH                     ( AXIS_ADDR_WIDTH                ),
    .ADDR_WIDTH                     ( ADDRBUF_ADDR_WIDTH             ),
    .OUTPUT_REG                     ( 1                              )
  ) u_obuf_rd_ram (
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .s_read_req                     ( s_axi_arvalid && ~s_axi_arready ),
    .s_read_addr                    ( obuf_rd_addr_rd_ptr            ),
    .s_read_data                    ( obuf_rd_addr_rdata             ),
    .s_write_req                    ( obuf_rd_addr_v                 ),
    .s_write_addr                   ( obuf_rd_addr_wr_ptr            ),
    .s_write_data                   ( obuf_rd_addr                   )
  );

  ram #(
    .DATA_WIDTH                     ( AXIS_ADDR_WIDTH                ),
    .ADDR_WIDTH                     ( ADDRBUF_ADDR_WIDTH             ),
    .OUTPUT_REG                     ( 1                              )
  ) u_obuf_wr_ram (
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .s_read_req                     ( s_axi_arvalid && ~s_axi_arready ),
    .s_read_addr                    ( obuf_wr_addr_rd_ptr            ),
    .s_read_data                    ( obuf_wr_addr_rdata             ),
    .s_write_req                    ( obuf_wr_addr_v                 ),
    .s_write_addr                   ( obuf_wr_addr_wr_ptr            ),
    .s_write_data                   ( obuf_wr_addr                   )
  );

  ram #(
    .DATA_WIDTH                     ( AXIS_ADDR_WIDTH                ),
    .ADDR_WIDTH                     ( ADDRBUF_ADDR_WIDTH             ),
    .OUTPUT_REG                     ( 1                              )
  ) u_wbuf_rd_ram (
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .s_read_req                     ( s_axi_arvalid && ~s_axi_arready ),
    .s_read_addr                    ( wbuf_rd_addr_rd_ptr            ),
    .s_read_data                    ( wbuf_rd_addr_rdata             ),
    .s_write_req                    ( wbuf_rd_addr_v                 ),
    .s_write_addr                   ( wbuf_rd_addr_wr_ptr            ),
    .s_write_data                   ( wbuf_rd_addr                   )
  );

  ram #(
    .DATA_WIDTH                     ( AXIS_ADDR_WIDTH                ),
    .ADDR_WIDTH                     ( ADDRBUF_ADDR_WIDTH             ),
    .OUTPUT_REG                     ( 1                              )
  ) u_bias_rd_ram (
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .s_read_req                     ( s_axi_arvalid && ~s_axi_arready ),
    .s_read_addr                    ( bias_rd_addr_rd_ptr            ),
    .s_read_data                    ( bias_rd_addr_rdata             ),
    .s_write_req                    ( bias_rd_addr_v                 ),
    .s_write_addr                   ( bias_rd_addr_wr_ptr            ),
    .s_write_data                   ( bias_rd_addr                   )
  );
//=============================================================

endmodule

//
// DnnWeaver2 controller
//
// Hardik Sharma
// (hsharma@gatech.edu)

`timescale 1ns/1ps
module dnnweaver2_controller #(
    parameter integer  NUM_TAGS                     = 2,
    parameter integer  ADDR_WIDTH                   = 42,
    parameter integer  ARRAY_N                      = 2,
    parameter integer  ARRAY_M                      = 2,

  // Precision
    parameter integer  DATA_WIDTH                   = 16,
    parameter integer  BIAS_WIDTH                   = 32,
    parameter integer  ACC_WIDTH                    = 64,

  // Buffers
    parameter integer  IBUF_CAPACITY_BITS           = ARRAY_N * DATA_WIDTH * 2048,
    parameter integer  WBUF_CAPACITY_BITS           = ARRAY_N * ARRAY_M * DATA_WIDTH * 512,
    parameter integer  OBUF_CAPACITY_BITS           = ARRAY_M * ACC_WIDTH * 2048,
    parameter integer  BBUF_CAPACITY_BITS           = ARRAY_M * BIAS_WIDTH * 2048,

  // Buffer Addr Width
    parameter integer  IBUF_ADDR_WIDTH              = $clog2(IBUF_CAPACITY_BITS / ARRAY_N / DATA_WIDTH),
    parameter integer  WBUF_ADDR_WIDTH              = $clog2(WBUF_CAPACITY_BITS / ARRAY_N / ARRAY_M / DATA_WIDTH),
    parameter integer  OBUF_ADDR_WIDTH              = $clog2(OBUF_CAPACITY_BITS / ARRAY_M / ACC_WIDTH),
    parameter integer  BBUF_ADDR_WIDTH              = $clog2(BBUF_CAPACITY_BITS / ARRAY_M / BIAS_WIDTH),

  // Instructions
    parameter integer  INST_ADDR_WIDTH              = 32,
    parameter integer  INST_DATA_WIDTH              = 32,
    parameter integer  INST_WSTRB_WIDTH             = INST_DATA_WIDTH / 8,
    parameter integer  INST_BURST_WIDTH             = 8,
    parameter integer  LOOP_ITER_W                  = 16,
    parameter integer  ADDR_STRIDE_W                = 32,
    parameter integer  MEM_REQ_W                    = 16,
    parameter integer  BUF_TYPE_W                   = 2,
    parameter integer  LOOP_ID_W                    = 5,
  // AGU
    parameter integer  OFFSET_W                     = ADDR_WIDTH,
  // AXI
    parameter integer  AXI_ADDR_WIDTH               = 42,
    parameter integer  AXI_ID_WIDTH                 = 1,
    parameter integer  AXI_BURST_WIDTH              = 8,
    parameter integer  TID_WIDTH                    = 4,
    parameter integer  IBUF_AXI_DATA_WIDTH          = 64,
    parameter integer  IBUF_WSTRB_W                 = IBUF_AXI_DATA_WIDTH/8,
    parameter integer  WBUF_AXI_DATA_WIDTH          = 64,
    parameter integer  WBUF_WSTRB_W                 = WBUF_AXI_DATA_WIDTH/8,
    parameter integer  OBUF_AXI_DATA_WIDTH          = 256,
    parameter integer  OBUF_WSTRB_W                 = OBUF_AXI_DATA_WIDTH/8,
    parameter integer  PU_AXI_DATA_WIDTH            = 64,
    parameter integer  PU_WSTRB_W                   = PU_AXI_DATA_WIDTH/8,
    parameter integer  BBUF_AXI_DATA_WIDTH          = 64,
    parameter integer  BBUF_WSTRB_W                 = BBUF_AXI_DATA_WIDTH/8,
  // AXI-Lite
    parameter integer  CTRL_ADDR_WIDTH              = 32,
    parameter integer  CTRL_DATA_WIDTH              = 32,
    parameter integer  CTRL_WSTRB_WIDTH             = CTRL_DATA_WIDTH/8,
  // Instruction Mem
    parameter integer  IMEM_ADDR_W                  = 7,
  // Systolic Array
    parameter integer  TAG_W                        = $clog2(NUM_TAGS),
    parameter          DTYPE                        = "FXP", // FXP for dnnweaver2, FP32 for single precision, FP16 for half-precision
    parameter integer  WBUF_DATA_WIDTH              = ARRAY_N * ARRAY_M * DATA_WIDTH,
    parameter integer  BBUF_DATA_WIDTH              = ARRAY_M * BIAS_WIDTH,
    parameter integer  IBUF_DATA_WIDTH              = ARRAY_N * DATA_WIDTH,
    parameter integer  OBUF_DATA_WIDTH              = ARRAY_M * ACC_WIDTH,

  // Buffer Addr width for PU access to OBUF
    parameter integer  PU_OBUF_ADDR_WIDTH           = OBUF_ADDR_WIDTH + $clog2(OBUF_DATA_WIDTH / OBUF_AXI_DATA_WIDTH)

) (
    input  wire                                         clk,
    input  wire                                         reset,

  // PCIe -> CL_wrapper AXI4-Lite interface
  // Slave Write address
    input  wire                                         pci_cl_ctrl_awvalid,
    input  wire  [ CTRL_ADDR_WIDTH      -1 : 0 ]        pci_cl_ctrl_awaddr,
    output wire                                         pci_cl_ctrl_awready,
  // Slave Write data
    input  wire                                         pci_cl_ctrl_wvalid,
    input  wire  [ CTRL_DATA_WIDTH      -1 : 0 ]        pci_cl_ctrl_wdata,
    input  wire  [ CTRL_WSTRB_WIDTH     -1 : 0 ]        pci_cl_ctrl_wstrb,
    output wire                                         pci_cl_ctrl_wready,
  // Slave Write response
    output wire                                         pci_cl_ctrl_bvalid,
    output wire  [ 2                    -1 : 0 ]        pci_cl_ctrl_bresp,
    input  wire                                         pci_cl_ctrl_bready,
  // Slave Read address
    input  wire                                         pci_cl_ctrl_arvalid,
    input  wire  [ CTRL_ADDR_WIDTH      -1 : 0 ]        pci_cl_ctrl_araddr,
    output wire                                         pci_cl_ctrl_arready,
  // Slave Read data/response
    output wire                                         pci_cl_ctrl_rvalid,
    output wire  [ CTRL_DATA_WIDTH      -1 : 0 ]        pci_cl_ctrl_rdata,
    output wire  [ 2                    -1 : 0 ]        pci_cl_ctrl_rresp,
    input  wire                                         pci_cl_ctrl_rready,

  // PCIe -> CL_wrapper AXI4 interface
  // Slave Interface Write Address
    input  wire  [ INST_ADDR_WIDTH      -1 : 0 ]        pci_cl_data_awaddr,
    input  wire  [ INST_BURST_WIDTH     -1 : 0 ]        pci_cl_data_awlen,
    input  wire  [ 3                    -1 : 0 ]        pci_cl_data_awsize,
    input  wire  [ 2                    -1 : 0 ]        pci_cl_data_awburst,
    input  wire                                         pci_cl_data_awvalid,
    output wire                                         pci_cl_data_awready,
  // Slave Interface Write Data
    input  wire  [ INST_DATA_WIDTH      -1 : 0 ]        pci_cl_data_wdata,
    input  wire  [ INST_WSTRB_WIDTH     -1 : 0 ]        pci_cl_data_wstrb,
    input  wire                                         pci_cl_data_wlast,
    input  wire                                         pci_cl_data_wvalid,
    output wire                                         pci_cl_data_wready,
  // Slave Interface Write Response
    output wire  [ 2                    -1 : 0 ]        pci_cl_data_bresp,
    output wire                                         pci_cl_data_bvalid,
    input  wire                                         pci_cl_data_bready,
  // Slave Interface Read Address
    input  wire  [ INST_ADDR_WIDTH      -1 : 0 ]        pci_cl_data_araddr,
    input  wire  [ INST_BURST_WIDTH     -1 : 0 ]        pci_cl_data_arlen,
    input  wire  [ 3                    -1 : 0 ]        pci_cl_data_arsize,
    input  wire  [ 2                    -1 : 0 ]        pci_cl_data_arburst,
    input  wire                                         pci_cl_data_arvalid,
    output wire                                         pci_cl_data_arready,
  // Slave Interface Read Data
    output wire  [ INST_DATA_WIDTH      -1 : 0 ]        pci_cl_data_rdata,
    output wire  [ 2                    -1 : 0 ]        pci_cl_data_rresp,
    output wire                                         pci_cl_data_rlast,
    output wire                                         pci_cl_data_rvalid,
    input  wire                                         pci_cl_data_rready,

  // CL_wrapper -> DDR0 AXI4 interface
  // Master Interface Write Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        cl_ddr0_awaddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        cl_ddr0_awlen,
    output wire  [ 3                    -1 : 0 ]        cl_ddr0_awsize,
    output wire  [ 2                    -1 : 0 ]        cl_ddr0_awburst,
    output wire                                         cl_ddr0_awvalid,
    input  wire                                         cl_ddr0_awready,
  // Master Interface Write Data
    output wire  [ IBUF_AXI_DATA_WIDTH  -1 : 0 ]        cl_ddr0_wdata,
    output wire  [ IBUF_WSTRB_W         -1 : 0 ]        cl_ddr0_wstrb,
    output wire                                         cl_ddr0_wlast,
    output wire                                         cl_ddr0_wvalid,
    input  wire                                         cl_ddr0_wready,
  // Master Interface Write Response
    input  wire  [ 2                    -1 : 0 ]        cl_ddr0_bresp,
    input  wire                                         cl_ddr0_bvalid,
    output wire                                         cl_ddr0_bready,
  // Master Interface Read Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        cl_ddr0_araddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        cl_ddr0_arlen,
    output wire  [ 3                    -1 : 0 ]        cl_ddr0_arsize,
    output wire  [ 2                    -1 : 0 ]        cl_ddr0_arburst,
    output wire                                         cl_ddr0_arvalid,
    output wire  [ AXI_ID_WIDTH         -1 : 0 ]        cl_ddr0_arid,
    input  wire                                         cl_ddr0_arready,
  // Master Interface Read Data
    input  wire  [ IBUF_AXI_DATA_WIDTH  -1 : 0 ]        cl_ddr0_rdata,
    input  wire  [ AXI_ID_WIDTH         -1 : 0 ]        cl_ddr0_rid,
    input  wire  [ 2                    -1 : 0 ]        cl_ddr0_rresp,
    input  wire                                         cl_ddr0_rlast,
    input  wire                                         cl_ddr0_rvalid,
    output wire                                         cl_ddr0_rready,

  // CL_wrapper -> DDR1 AXI4 interface
  // Master Interface Write Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        cl_ddr1_awaddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        cl_ddr1_awlen,
    output wire  [ 3                    -1 : 0 ]        cl_ddr1_awsize,
    output wire  [ 2                    -1 : 0 ]        cl_ddr1_awburst,
    output wire                                         cl_ddr1_awvalid,
    input  wire                                         cl_ddr1_awready,
  // Master Interface Write Data
    output wire  [ OBUF_AXI_DATA_WIDTH  -1 : 0 ]        cl_ddr1_wdata,
    output wire  [ OBUF_WSTRB_W         -1 : 0 ]        cl_ddr1_wstrb,
    output wire                                         cl_ddr1_wlast,
    output wire                                         cl_ddr1_wvalid,
    input  wire                                         cl_ddr1_wready,
  // Master Interface Write Response
    input  wire  [ 2                    -1 : 0 ]        cl_ddr1_bresp,
    input  wire                                         cl_ddr1_bvalid,
    output wire                                         cl_ddr1_bready,
  // Master Interface Read Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        cl_ddr1_araddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        cl_ddr1_arlen,
    output wire  [ 3                    -1 : 0 ]        cl_ddr1_arsize,
    output wire  [ 2                    -1 : 0 ]        cl_ddr1_arburst,
    output wire                                         cl_ddr1_arvalid,
    output wire  [ AXI_ID_WIDTH         -1 : 0 ]        cl_ddr1_arid,
    input  wire                                         cl_ddr1_arready,
  // Master Interface Read Data
    input  wire  [ OBUF_AXI_DATA_WIDTH  -1 : 0 ]        cl_ddr1_rdata,
    input  wire  [ AXI_ID_WIDTH         -1 : 0 ]        cl_ddr1_rid,
    input  wire  [ 2                    -1 : 0 ]        cl_ddr1_rresp,
    input  wire                                         cl_ddr1_rlast,
    input  wire                                         cl_ddr1_rvalid,
    output wire                                         cl_ddr1_rready,

  // CL_wrapper -> DDR2 AXI4 interface
  // Master Interface Write Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        cl_ddr2_awaddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        cl_ddr2_awlen,
    output wire  [ 3                    -1 : 0 ]        cl_ddr2_awsize,
    output wire  [ 2                    -1 : 0 ]        cl_ddr2_awburst,
    output wire                                         cl_ddr2_awvalid,
    input  wire                                         cl_ddr2_awready,
  // Master Interface Write Data
    output wire  [ WBUF_AXI_DATA_WIDTH  -1 : 0 ]        cl_ddr2_wdata,
    output wire  [ WBUF_WSTRB_W         -1 : 0 ]        cl_ddr2_wstrb,
    output wire                                         cl_ddr2_wlast,
    output wire                                         cl_ddr2_wvalid,
    input  wire                                         cl_ddr2_wready,
  // Master Interface Write Response
    input  wire  [ 2                    -1 : 0 ]        cl_ddr2_bresp,
    input  wire                                         cl_ddr2_bvalid,
    output wire                                         cl_ddr2_bready,
  // Master Interface Read Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        cl_ddr2_araddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        cl_ddr2_arlen,
    output wire  [ 3                    -1 : 0 ]        cl_ddr2_arsize,
    output wire  [ 2                    -1 : 0 ]        cl_ddr2_arburst,
    output wire                                         cl_ddr2_arvalid,
    output wire  [ AXI_ID_WIDTH         -1 : 0 ]        cl_ddr2_arid,
    input  wire                                         cl_ddr2_arready,
  // Master Interface Read Data
    input  wire  [ WBUF_AXI_DATA_WIDTH  -1 : 0 ]        cl_ddr2_rdata,
    input  wire  [ AXI_ID_WIDTH         -1 : 0 ]        cl_ddr2_rid,
    input  wire  [ 2                    -1 : 0 ]        cl_ddr2_rresp,
    input  wire                                         cl_ddr2_rlast,
    input  wire                                         cl_ddr2_rvalid,
    output wire                                         cl_ddr2_rready,

  // CL_wrapper -> DDR3 AXI4 interface
  // Master Interface Write Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        cl_ddr3_awaddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        cl_ddr3_awlen,
    output wire  [ 3                    -1 : 0 ]        cl_ddr3_awsize,
    output wire  [ 2                    -1 : 0 ]        cl_ddr3_awburst,
    output wire                                         cl_ddr3_awvalid,
    input  wire                                         cl_ddr3_awready,
  // Master Interface Write Data
    output wire  [ BBUF_AXI_DATA_WIDTH  -1 : 0 ]        cl_ddr3_wdata,
    output wire  [ BBUF_WSTRB_W         -1 : 0 ]        cl_ddr3_wstrb,
    output wire                                         cl_ddr3_wlast,
    output wire                                         cl_ddr3_wvalid,
    input  wire                                         cl_ddr3_wready,
  // Master Interface Write Response
    input  wire  [ 2                    -1 : 0 ]        cl_ddr3_bresp,
    input  wire                                         cl_ddr3_bvalid,
    output wire                                         cl_ddr3_bready,
  // Master Interface Read Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        cl_ddr3_araddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        cl_ddr3_arlen,
    output wire  [ 3                    -1 : 0 ]        cl_ddr3_arsize,
    output wire  [ 2                    -1 : 0 ]        cl_ddr3_arburst,
    output wire                                         cl_ddr3_arvalid,
    output wire  [ AXI_ID_WIDTH         -1 : 0 ]        cl_ddr3_arid,
    input  wire                                         cl_ddr3_arready,
  // Master Interface Read Data
    input  wire  [ BBUF_AXI_DATA_WIDTH  -1 : 0 ]        cl_ddr3_rdata,
    input  wire  [ AXI_ID_WIDTH         -1 : 0 ]        cl_ddr3_rid,
    input  wire  [ 2                    -1 : 0 ]        cl_ddr3_rresp,
    input  wire                                         cl_ddr3_rlast,
    input  wire                                         cl_ddr3_rvalid,
    output wire                                         cl_ddr3_rready,

  // CL_wrapper -> DDR3 AXI4 interface
  // Master Interface Write Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        cl_ddr4_awaddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        cl_ddr4_awlen,
    output wire  [ 3                    -1 : 0 ]        cl_ddr4_awsize,
    output wire  [ 2                    -1 : 0 ]        cl_ddr4_awburst,
    output wire                                         cl_ddr4_awvalid,
    input  wire                                         cl_ddr4_awready,
  // Master Interface Write Data
    output wire  [ PU_AXI_DATA_WIDTH    -1 : 0 ]        cl_ddr4_wdata,
    output wire  [ PU_WSTRB_W           -1 : 0 ]        cl_ddr4_wstrb,
    output wire                                         cl_ddr4_wlast,
    output wire                                         cl_ddr4_wvalid,
    input  wire                                         cl_ddr4_wready,
  // Master Interface Write Response
    input  wire  [ 2                    -1 : 0 ]        cl_ddr4_bresp,
    input  wire                                         cl_ddr4_bvalid,
    output wire                                         cl_ddr4_bready,
  // Master Interface Read Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        cl_ddr4_araddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        cl_ddr4_arlen,
    output wire  [ 3                    -1 : 0 ]        cl_ddr4_arsize,
    output wire  [ 2                    -1 : 0 ]        cl_ddr4_arburst,
    output wire                                         cl_ddr4_arvalid,
    output wire  [ AXI_ID_WIDTH         -1 : 0 ]        cl_ddr4_arid,
    input  wire                                         cl_ddr4_arready,
  // Master Interface Read Data
    input  wire  [ PU_AXI_DATA_WIDTH    -1 : 0 ]        cl_ddr4_rdata,
    input  wire  [ AXI_ID_WIDTH         -1 : 0 ]        cl_ddr4_rid,
    input  wire  [ 2                    -1 : 0 ]        cl_ddr4_rresp,
    input  wire                                         cl_ddr4_rlast,
    input  wire                                         cl_ddr4_rvalid,
    output wire                                         cl_ddr4_rready
  );

//=============================================================
// Localparams
//=============================================================
    localparam integer  STATE_W                      = 1;
    localparam integer  ACCUMULATOR_WIDTH            = 48;
  // localparam integer  PMAX                         = DATA_WIDTH;
  // localparam integer  PMIN                         = DATA_WIDTH;
//=============================================================

//=============================================================
// Wires/Regs
//=============================================================
    wire [ INST_DATA_WIDTH      -1 : 0 ]        obuf_ld_stream_read_count;
    wire [ INST_DATA_WIDTH      -1 : 0 ]        obuf_ld_stream_write_count;
    wire [ INST_DATA_WIDTH      -1 : 0 ]        ddr_st_stream_read_count;
    wire [ INST_DATA_WIDTH      -1 : 0 ]        ddr_st_stream_write_count;
    wire [ INST_DATA_WIDTH      -1 : 0 ]        ld0_stream_counts;
    wire [ INST_DATA_WIDTH      -1 : 0 ]        ld1_stream_counts;
    wire [ INST_DATA_WIDTH      -1 : 0 ]        axi_wr_fifo_counts;

  // OBUF STMEM state
    wire [ 4                    -1 : 0 ]        stmem_state;
    wire [ TAG_W                -1 : 0 ]        stmem_tag;
    wire                                        stmem_ddr_pe_sw;

  // PU
    wire                                        pu_compute_start;
    wire                                        pu_compute_ready;
    wire                                        pu_compute_done;
    wire                                        pu_write_done;
    wire [ 3                    -1 : 0 ]        pu_ctrl_state;
    wire                                        pu_done;
    wire                                        pu_inst_wr_ready;
  // PU -> OBUF addr
    wire                                        ld_obuf_req;
    wire                                        ld_obuf_ready;
    wire [ PU_OBUF_ADDR_WIDTH   -1 : 0 ]        ld_obuf_addr;
  // OBUF -> PU addr
    wire                                        obuf_ld_stream_write_req;
    wire [ OBUF_AXI_DATA_WIDTH  -1 : 0 ]        obuf_ld_stream_write_data;

  // Snoop
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        snoop_cl_ddr0_araddr;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        snoop_cl_ddr1_araddr;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        snoop_cl_ddr1_awaddr;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        snoop_cl_ddr2_araddr;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        snoop_cl_ddr3_araddr;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        snoop_cl_ddr4_araddr;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        snoop_cl_ddr4_awaddr;

  // PU Instructions
    wire                                        cfg_pu_inst_v;
    wire [ INST_DATA_WIDTH      -1 : 0 ]        cfg_pu_inst;
    wire                                        pu_block_start;
    wire                                        pu_block_end;
  // Systolic array
    wire                                        acc_clear;
    wire [ OBUF_DATA_WIDTH      -1 : 0 ]        sys_obuf_write_data;

  // switch between bias and obuf
    wire                                        rd_bias_prev_sw;

  // Loop iterations
    wire [ LOOP_ITER_W          -1 : 0 ]        cfg_loop_iter;
    wire [ LOOP_ID_W            -1 : 0 ]        cfg_loop_iter_loop_id;
    wire                                        cfg_loop_iter_v;
  // Loop stride
    wire [ 16                   -1 : 0 ]        cfg_loop_stride_lo;
    wire [ ADDR_STRIDE_W        -1 : 0 ]        cfg_loop_stride;
    wire                                        cfg_loop_stride_v;
    wire [ BUF_TYPE_W           -1 : 0 ]        cfg_loop_stride_id;
    wire [ 2                    -1 : 0 ]        cfg_loop_stride_type;
    wire [ LOOP_ID_W            -1 : 0 ]        cfg_loop_stride_loop_id;

  // Memory request
    wire [ MEM_REQ_W            -1 : 0 ]        cfg_mem_req_size;
    wire                                        cfg_mem_req_v;
    wire [ 2                    -1 : 0 ]        cfg_mem_req_type;
    wire [ BUF_TYPE_W           -1 : 0 ]        cfg_mem_req_id;
    wire [ LOOP_ID_W            -1 : 0 ]        cfg_mem_req_loop_id;
  // Buffer request
    wire [ MEM_REQ_W            -1 : 0 ]        cfg_buf_req_size;
    wire                                        cfg_buf_req_v;
    wire                                        cfg_buf_req_type;
    wire [ BUF_TYPE_W           -1 : 0 ]        cfg_buf_req_loop_id;

    wire                                        main_start;
    wire                                        main_done;

  // Address - OBUF
    wire [ ADDR_WIDTH           -1 : 0 ]        obuf_base_addr;
    wire [ ADDR_WIDTH           -1 : 0 ]        obuf_ld_addr;
    wire                                        obuf_ld_addr_v;
    wire [ ADDR_WIDTH           -1 : 0 ]        obuf_st_addr;
    wire                                        obuf_st_addr_v;
  // Address - IBUF
    wire [ ADDR_WIDTH           -1 : 0 ]        ibuf_base_addr;
    wire [ ADDR_WIDTH           -1 : 0 ]        ibuf_ld_addr;
    wire                                        ibuf_ld_addr_v;
  // Address - WBUF
    wire [ ADDR_WIDTH           -1 : 0 ]        wbuf_base_addr;
    wire [ ADDR_WIDTH           -1 : 0 ]        wbuf_ld_addr;
    wire                                        wbuf_ld_addr_v;
    wire [ ADDR_WIDTH           -1 : 0 ]        wbuf_st_addr;
    wire                                        wbuf_st_addr_v;
  // Address - BIAS
    wire [ ADDR_WIDTH           -1 : 0 ]        bias_base_addr;
    wire [ ADDR_WIDTH           -1 : 0 ]        bias_ld_addr;
    wire                                        bias_ld_addr_v;
    wire [ ADDR_WIDTH           -1 : 0 ]        bias_st_addr;
    wire                                        bias_st_addr_v;
  // Address - OBUF
    wire [ OBUF_ADDR_WIDTH      -1 : 0 ]        obuf_rd_addr;
    wire                                        obuf_rd_addr_v;
    wire [ OBUF_ADDR_WIDTH      -1 : 0 ]        obuf_wr_addr;
    wire                                        obuf_wr_addr_v;
  // Address - IBUF
    wire [ IBUF_ADDR_WIDTH      -1 : 0 ]        ibuf_rd_addr;
    wire                                        ibuf_rd_addr_v;
    wire [ IBUF_ADDR_WIDTH      -1 : 0 ]        ibuf_wr_addr;
    wire                                        ibuf_wr_addr_v;
  // Address - WBUF
    wire [ WBUF_ADDR_WIDTH      -1 : 0 ]        wbuf_rd_addr;
    wire                                        wbuf_rd_addr_v;
    wire [ WBUF_ADDR_WIDTH      -1 : 0 ]        wbuf_wr_addr;
    wire                                        wbuf_wr_addr_v;
  // Select logic for bias (0) or obuf_read_data (1)
    wire                                        compute_bias_prev_sw;
    wire                                        tag_bias_prev_sw;
    wire                                        tag_ddr_pe_sw;

  // IBUF
    wire [ IBUF_DATA_WIDTH      -1 : 0 ]        ibuf_read_data;
    wire                                        ibuf_read_req;
    wire [ IBUF_ADDR_WIDTH      -1 : 0 ]        ibuf_read_addr;

  // WBUF
    wire [ WBUF_DATA_WIDTH      -1 : 0 ]        wbuf_read_data;
    wire                                        wbuf_read_req;
    wire [ WBUF_ADDR_WIDTH      -1 : 0 ]        wbuf_read_addr;

  // BIAS
    wire [ BBUF_DATA_WIDTH      -1 : 0 ]        bbuf_read_data;
    wire                                        bias_read_req;
    wire [ BBUF_ADDR_WIDTH      -1 : 0 ]        bias_read_addr;
    wire                                        sys_bias_read_req;
    wire [ BBUF_ADDR_WIDTH      -1 : 0 ]        sys_bias_read_addr;

  // OBUF
    wire [ OBUF_DATA_WIDTH      -1 : 0 ]        obuf_write_data;
    wire                                        obuf_write_req;
    wire [ OBUF_ADDR_WIDTH      -1 : 0 ]        obuf_write_addr;
    wire [ OBUF_DATA_WIDTH      -1 : 0 ]        obuf_read_data;
    wire                                        obuf_read_req;
    wire [ OBUF_ADDR_WIDTH      -1 : 0 ]        obuf_read_addr;

    wire                                        sys_obuf_write_req;
    wire [ OBUF_ADDR_WIDTH      -1 : 0 ]        sys_obuf_write_addr;

    wire                                        sys_obuf_read_req;
    wire [ OBUF_ADDR_WIDTH      -1 : 0 ]        sys_obuf_read_addr;

  // Slave registers
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg0_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg0_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg1_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg1_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg2_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg2_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg3_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg3_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg4_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg4_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg5_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg5_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg6_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg6_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg7_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg7_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg8_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg8_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg9_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg9_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg10_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg10_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg11_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg11_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg12_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg12_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg13_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg13_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg14_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg14_out;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg15_in;
    wire [ CTRL_DATA_WIDTH      -1 : 0 ]        slv_reg15_out;
  // Slave registers end


    wire [ IMEM_ADDR_W          -1 : 0 ]        start_addr;

    reg  [ STATE_W              -1 : 0 ]        w_state_q;
    reg  [ STATE_W              -1 : 0 ]        w_state_d;

    reg  [ STATE_W              -1 : 0 ]        r_state_q;
    reg  [ STATE_W              -1 : 0 ]        r_state_d;

    reg  [ AXI_ADDR_WIDTH       -1 : 0 ]        w_addr_d;
    reg  [ AXI_ADDR_WIDTH       -1 : 0 ]        w_addr_q;

    wire                                        imem_read_req_a;
    wire [ IMEM_ADDR_W          -1 : 0 ]        imem_read_addr_a;
    wire [ INST_DATA_WIDTH      -1 : 0 ]        imem_read_data_a;

    wire                                        imem_write_req_a;
    wire [ IMEM_ADDR_W          -1 : 0 ]        imem_write_addr_a;
    wire [ INST_DATA_WIDTH      -1 : 0 ]        imem_write_data_a;

    wire                                        imem_read_req_b;
    wire [ IMEM_ADDR_W          -1 : 0 ]        imem_read_addr_b;
    wire [ INST_DATA_WIDTH      -1 : 0 ]        imem_read_data_b;

    wire                                        ibuf_tag_ready;
    wire                                        ibuf_tag_done;
    wire                                        ibuf_compute_ready;

    wire                                        wbuf_tag_ready;
    wire                                        wbuf_tag_done;
    wire                                        wbuf_compute_ready;

    wire                                        obuf_tag_ready;
    wire                                        obuf_tag_done;
    wire                                        obuf_compute_ready;
    wire                                        obuf_bias_prev_sw;

    wire                                        bias_tag_ready;
    wire                                        bias_tag_done;
    wire                                        bias_compute_ready;

    wire                                        tag_flush;
    wire                                        tag_req;
    wire                                        ibuf_tag_reuse;
    wire                                        obuf_tag_reuse;
    wire                                        wbuf_tag_reuse;
    wire                                        bias_tag_reuse;
    wire                                        sync_tag_req;
    wire                                        tag_ready;

    wire                                        compute_done;
    wire                                        compute_req;

    wire [ IBUF_ADDR_WIDTH      -1 : 0 ]        tie_ibuf_buf_base_addr;
    wire [ WBUF_ADDR_WIDTH      -1 : 0 ]        tie_wbuf_buf_base_addr;
    wire [ OBUF_ADDR_WIDTH      -1 : 0 ]        tie_obuf_buf_base_addr;
    wire [ BBUF_ADDR_WIDTH      -1 : 0 ]        tie_bias_buf_base_addr;

    wire                                        sys_array_c_sel;
//=============================================================

//=============================================================
// Assigns
//=============================================================
  // TODO: bias tag handling
  // Use the bias tag ready when obuf not needed
    assign compute_req = ibuf_compute_ready && wbuf_compute_ready && obuf_compute_ready && bias_compute_ready;
    assign tag_ready = (ibuf_tag_ready && wbuf_tag_ready && obuf_tag_ready && bias_tag_ready);

  // ST tie-offs
    assign wbuf_st_addr_v = 1'b0;
    assign wbuf_st_addr = 'b0;
    assign bias_st_addr_v = 1'b0;
    assign bias_st_addr = 'b0;

  // Address tie-off
    assign tie_ibuf_buf_base_addr = {IBUF_ADDR_WIDTH{1'b0}};
    assign tie_wbuf_buf_base_addr = {WBUF_ADDR_WIDTH{1'b0}};
    assign tie_obuf_buf_base_addr = {OBUF_ADDR_WIDTH{1'b0}};
    assign tie_bias_buf_base_addr = {BBUF_ADDR_WIDTH{1'b0}};

  // Buf write port tie-offs

  // Systolic array
    assign acc_clear = compute_done;

  // Synchronize tag req
    assign sync_tag_req = tag_req && ibuf_tag_ready && wbuf_tag_ready && obuf_tag_ready && bias_tag_ready;

  // Snoop
    assign snoop_cl_ddr0_araddr = cl_ddr0_araddr;
    assign snoop_cl_ddr1_araddr = cl_ddr1_araddr;
    assign snoop_cl_ddr1_awaddr = cl_ddr1_awaddr;
    assign snoop_cl_ddr2_araddr = cl_ddr2_araddr;
    assign snoop_cl_ddr3_araddr = cl_ddr3_araddr;
    assign snoop_cl_ddr4_araddr = cl_ddr4_araddr;
    assign snoop_cl_ddr4_awaddr = cl_ddr4_awaddr;

//=============================================================

//=============================================================
// Base controller
//    This module is in charge of the outer loops [16 - 31]
//=============================================================
  controller #(
    .CTRL_ADDR_WIDTH                ( CTRL_ADDR_WIDTH                ),
    .CTRL_DATA_WIDTH                ( CTRL_DATA_WIDTH                ),
    .INST_ADDR_WIDTH                ( INST_ADDR_WIDTH                )
  ) u_ctrl (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input

    .tag_flush                      ( tag_flush                      ), //output
    .tag_req                        ( tag_req                        ), //output
    .tag_ready                      ( tag_ready                      ), //input

    .compute_done                   ( compute_done                   ), //input
    .pu_compute_start               ( pu_compute_start               ), //input
    .pu_compute_done                ( pu_compute_done                ), //input
    .pu_write_done                  ( pu_write_done                  ), //input
    .pu_ctrl_state                  ( pu_ctrl_state                  ), //input

    //DEBUG
    .obuf_ld_stream_read_count      ( obuf_ld_stream_read_count      ), //input
    .obuf_ld_stream_write_count     ( obuf_ld_stream_write_count     ), //input
    .ddr_st_stream_read_count       ( ddr_st_stream_read_count       ), //input
    .ddr_st_stream_write_count      ( ddr_st_stream_write_count      ), //input
    .ld0_stream_counts              ( ld0_stream_counts              ), //output
    .ld1_stream_counts              ( ld1_stream_counts              ), //output
    .axi_wr_fifo_counts             ( axi_wr_fifo_counts             ), //output
    //DEBUG

    .ibuf_tag_reuse                 ( ibuf_tag_reuse                 ), //output
    .obuf_tag_reuse                 ( obuf_tag_reuse                 ), //output
    .wbuf_tag_reuse                 ( wbuf_tag_reuse                 ), //output
    .bias_tag_reuse                 ( bias_tag_reuse                 ), //output

    .ibuf_tag_done                  ( ibuf_tag_done                  ), //input
    .wbuf_tag_done                  ( wbuf_tag_done                  ), //input
    .obuf_tag_done                  ( obuf_tag_done                  ), //input
    .bias_tag_done                  ( bias_tag_done                  ), //input

    .tag_bias_prev_sw               ( tag_bias_prev_sw               ), //output
    .tag_ddr_pe_sw                  ( tag_ddr_pe_sw                  ), //output
    .bias_ld_addr                   ( bias_ld_addr                   ), //output
    .bias_ld_addr_v                 ( bias_ld_addr_v                 ), //output
    .ibuf_ld_addr                   ( ibuf_ld_addr                   ), //output
    .ibuf_ld_addr_v                 ( ibuf_ld_addr_v                 ), //output
    .wbuf_ld_addr                   ( wbuf_ld_addr                   ), //output
    .wbuf_ld_addr_v                 ( wbuf_ld_addr_v                 ), //output
    .obuf_ld_addr                   ( obuf_ld_addr                   ), //output
    .obuf_ld_addr_v                 ( obuf_ld_addr_v                 ), //output
    .obuf_st_addr                   ( obuf_st_addr                   ), //output
    .obuf_st_addr_v                 ( obuf_st_addr_v                 ), //output

    .stmem_state                    ( stmem_state                    ), //input
    .stmem_tag                      ( stmem_tag                      ), //input
    .stmem_ddr_pe_sw                ( stmem_ddr_pe_sw                ), //input

    .pci_cl_ctrl_awvalid            ( pci_cl_ctrl_awvalid            ), //input
    .pci_cl_ctrl_awaddr             ( pci_cl_ctrl_awaddr             ), //input
    .pci_cl_ctrl_awready            ( pci_cl_ctrl_awready            ), //output
    .pci_cl_ctrl_wvalid             ( pci_cl_ctrl_wvalid             ), //input
    .pci_cl_ctrl_wdata              ( pci_cl_ctrl_wdata              ), //input
    .pci_cl_ctrl_wstrb              ( pci_cl_ctrl_wstrb              ), //input
    .pci_cl_ctrl_wready             ( pci_cl_ctrl_wready             ), //output
    .pci_cl_ctrl_bvalid             ( pci_cl_ctrl_bvalid             ), //output
    .pci_cl_ctrl_bresp              ( pci_cl_ctrl_bresp              ), //output
    .pci_cl_ctrl_bready             ( pci_cl_ctrl_bready             ), //input
    .pci_cl_ctrl_arvalid            ( pci_cl_ctrl_arvalid            ), //input
    .pci_cl_ctrl_araddr             ( pci_cl_ctrl_araddr             ), //input
    .pci_cl_ctrl_arready            ( pci_cl_ctrl_arready            ), //output
    .pci_cl_ctrl_rvalid             ( pci_cl_ctrl_rvalid             ), //output
    .pci_cl_ctrl_rdata              ( pci_cl_ctrl_rdata              ), //output
    .pci_cl_ctrl_rresp              ( pci_cl_ctrl_rresp              ), //output
    .pci_cl_ctrl_rready             ( pci_cl_ctrl_rready             ), //input

    .pci_cl_data_awaddr             ( pci_cl_data_awaddr             ), //input
    .pci_cl_data_awlen              ( pci_cl_data_awlen              ), //input
    .pci_cl_data_awsize             ( pci_cl_data_awsize             ), //input
    .pci_cl_data_awburst            ( pci_cl_data_awburst            ), //input
    .pci_cl_data_awvalid            ( pci_cl_data_awvalid            ), //input
    .pci_cl_data_awready            ( pci_cl_data_awready            ), //output
    .pci_cl_data_wdata              ( pci_cl_data_wdata              ), //input
    .pci_cl_data_wstrb              ( pci_cl_data_wstrb              ), //input
    .pci_cl_data_wlast              ( pci_cl_data_wlast              ), //input
    .pci_cl_data_wvalid             ( pci_cl_data_wvalid             ), //input
    .pci_cl_data_wready             ( pci_cl_data_wready             ), //output
    .pci_cl_data_bresp              ( pci_cl_data_bresp              ), //output
    .pci_cl_data_bvalid             ( pci_cl_data_bvalid             ), //output
    .pci_cl_data_bready             ( pci_cl_data_bready             ), //input
    .pci_cl_data_araddr             ( pci_cl_data_araddr             ), //input
    .pci_cl_data_arlen              ( pci_cl_data_arlen              ), //input
    .pci_cl_data_arsize             ( pci_cl_data_arsize             ), //input
    .pci_cl_data_arburst            ( pci_cl_data_arburst            ), //input
    .pci_cl_data_arvalid            ( pci_cl_data_arvalid            ), //input
    .pci_cl_data_arready            ( pci_cl_data_arready            ), //output
    .pci_cl_data_rdata              ( pci_cl_data_rdata              ), //output
    .pci_cl_data_rresp              ( pci_cl_data_rresp              ), //output
    .pci_cl_data_rlast              ( pci_cl_data_rlast              ), //output
    .pci_cl_data_rvalid             ( pci_cl_data_rvalid             ), //output
    .pci_cl_data_rready             ( pci_cl_data_rready             ), //input

    .ibuf_compute_ready             ( ibuf_compute_ready             ), //input
    .wbuf_compute_ready             ( wbuf_compute_ready             ), //input
    .obuf_compute_ready             ( obuf_compute_ready             ), //input
    .bias_compute_ready             ( bias_compute_ready             ), //input

    .cfg_loop_iter                  ( cfg_loop_iter                  ), //output
    .cfg_loop_iter_loop_id          ( cfg_loop_iter_loop_id          ), //output
    .cfg_loop_iter_v                ( cfg_loop_iter_v                ), //output
    .cfg_loop_stride                ( cfg_loop_stride                ), //output
    .cfg_loop_stride_v              ( cfg_loop_stride_v              ), //output
    .cfg_loop_stride_id             ( cfg_loop_stride_id             ), //output
    .cfg_loop_stride_type           ( cfg_loop_stride_type           ), //output
    .cfg_loop_stride_loop_id        ( cfg_loop_stride_loop_id        ), //output
    .cfg_mem_req_size               ( cfg_mem_req_size               ), //output
    .cfg_mem_req_v                  ( cfg_mem_req_v                  ), //output
    .cfg_mem_req_type               ( cfg_mem_req_type               ), //output
    .cfg_mem_req_id                 ( cfg_mem_req_id                 ), //output
    .cfg_mem_req_loop_id            ( cfg_mem_req_loop_id            ), //output
    .cfg_buf_req_size               ( cfg_buf_req_size               ), //output
    .cfg_buf_req_v                  ( cfg_buf_req_v                  ), //output
    .cfg_buf_req_type               ( cfg_buf_req_type               ), //output
    .cfg_buf_req_loop_id            ( cfg_buf_req_loop_id            ), //output

    .cfg_pu_inst                    ( cfg_pu_inst                    ), //output
    .cfg_pu_inst_v                  ( cfg_pu_inst_v                  ), //output
    .pu_block_start                 ( pu_block_start                 ), //output

    .snoop_cl_ddr0_araddr           ( snoop_cl_ddr0_araddr           ), //input
    .snoop_cl_ddr0_arvalid          ( cl_ddr0_arvalid                ), //input
    .snoop_cl_ddr0_arready          ( cl_ddr0_arready                ), //input
    .snoop_cl_ddr0_arlen            ( cl_ddr0_arlen                  ), //input
    .snoop_cl_ddr0_rvalid           ( cl_ddr0_rvalid                 ), //input
    .snoop_cl_ddr0_rready           ( cl_ddr0_rready                 ), //input

    .snoop_cl_ddr1_awaddr           ( snoop_cl_ddr1_awaddr           ), //input
    .snoop_cl_ddr1_awvalid          ( cl_ddr1_awvalid                ), //input
    .snoop_cl_ddr1_awready          ( cl_ddr1_awready                ), //input
    .snoop_cl_ddr1_awlen            ( cl_ddr1_awlen                  ), //input
    .snoop_cl_ddr1_araddr           ( snoop_cl_ddr1_araddr           ), //input
    .snoop_cl_ddr1_arvalid          ( cl_ddr1_arvalid                ), //input
    .snoop_cl_ddr1_arready          ( cl_ddr1_arready                ), //input
    .snoop_cl_ddr1_arlen            ( cl_ddr1_arlen                  ), //input
    .snoop_cl_ddr1_wvalid           ( cl_ddr1_wvalid                 ), //input
    .snoop_cl_ddr1_wready           ( cl_ddr1_wready                 ), //input
    .snoop_cl_ddr1_rvalid           ( cl_ddr1_rvalid                 ), //input
    .snoop_cl_ddr1_rready           ( cl_ddr1_rready                 ), //input

    .snoop_cl_ddr2_araddr           ( snoop_cl_ddr2_araddr           ), //input
    .snoop_cl_ddr2_arvalid          ( cl_ddr2_arvalid                ), //input
    .snoop_cl_ddr2_arready          ( cl_ddr2_arready                ), //input
    .snoop_cl_ddr2_arlen            ( cl_ddr2_arlen                  ), //input
    .snoop_cl_ddr2_rvalid           ( cl_ddr2_rvalid                 ), //input
    .snoop_cl_ddr2_rready           ( cl_ddr2_rready                 ), //input

    .snoop_cl_ddr3_araddr           ( snoop_cl_ddr3_araddr           ), //input
    .snoop_cl_ddr3_arvalid          ( cl_ddr3_arvalid                ), //input
    .snoop_cl_ddr3_arready          ( cl_ddr3_arready                ), //input
    .snoop_cl_ddr3_arlen            ( cl_ddr3_arlen                  ), //input
    .snoop_cl_ddr3_rvalid           ( cl_ddr3_rvalid                 ), //input
    .snoop_cl_ddr3_rready           ( cl_ddr3_rready                 ), //input

    .snoop_cl_ddr4_awaddr           ( snoop_cl_ddr4_awaddr           ), //input
    .snoop_cl_ddr4_awvalid          ( cl_ddr4_awvalid                ), //input
    .snoop_cl_ddr4_awready          ( cl_ddr4_awready                ), //input
    .snoop_cl_ddr4_awlen            ( cl_ddr4_awlen                  ), //input
    .snoop_cl_ddr4_araddr           ( snoop_cl_ddr4_araddr           ), //input
    .snoop_cl_ddr4_arvalid          ( cl_ddr4_arvalid                ), //input
    .snoop_cl_ddr4_arready          ( cl_ddr4_arready                ), //input
    .snoop_cl_ddr4_arlen            ( cl_ddr4_arlen                  ), //input
    .snoop_cl_ddr4_wvalid           ( cl_ddr4_wvalid                 ), //input
    .snoop_cl_ddr4_wready           ( cl_ddr4_wready                 ), //input
    .snoop_cl_ddr4_rvalid           ( cl_ddr4_rvalid                 ), //input
    .snoop_cl_ddr4_rready           ( cl_ddr4_rready                 ), //input

    .ld_obuf_req                    ( ld_obuf_req                    ), //input
    .ld_obuf_ready                  ( ld_obuf_ready                  )  //input

  );
//=============================================================

//=============================================================
// Compute controller
//    This module is in charge of the compute loops [0 - 15]
//=============================================================
    assign cfg_loop_stride_lo = cfg_loop_stride;
  base_addr_gen #(
    .BASE_ID                        ( 0                              ),
    .MEM_REQ_W                      ( MEM_REQ_W                      ),
    .IBUF_ADDR_WIDTH                ( IBUF_ADDR_WIDTH                ),
    .WBUF_ADDR_WIDTH                ( WBUF_ADDR_WIDTH                ),
    .OBUF_ADDR_WIDTH                ( OBUF_ADDR_WIDTH                ),
    .BBUF_ADDR_WIDTH                ( BBUF_ADDR_WIDTH                ),
    .LOOP_ITER_W                    ( LOOP_ITER_W                    ),
    .LOOP_ID_W                      ( LOOP_ID_W                      )
  ) compute_ctrl (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input

    .start                          ( compute_req                    ), //input
    .done                           ( compute_done                   ), //output

    //TODO
    .tag_req                        (                                ), //output
    .tag_ready                      ( 1'b1                           ), //input

    .cfg_loop_iter_v                ( cfg_loop_iter_v                ), //input
    .cfg_loop_iter                  ( cfg_loop_iter                  ), //input
    .cfg_loop_iter_loop_id          ( cfg_loop_iter_loop_id          ), //input

    .cfg_loop_stride_v              ( cfg_loop_stride_v              ), //input
    .cfg_loop_stride                ( cfg_loop_stride_lo             ), //input
    .cfg_loop_stride_loop_id        ( cfg_loop_stride_loop_id        ), //input
    .cfg_loop_stride_type           ( cfg_loop_stride_type           ), //input
    .cfg_loop_stride_id             ( cfg_loop_stride_id             ), //input

    .ibuf_base_addr                 ( tie_ibuf_buf_base_addr         ), //input
    .wbuf_base_addr                 ( tie_wbuf_buf_base_addr         ), //input
    .obuf_base_addr                 ( tie_obuf_buf_base_addr         ), //input
    .bias_base_addr                 ( tie_bias_buf_base_addr         ), //input

    .obuf_ld_addr                   ( obuf_read_addr                 ), //output
    .obuf_ld_addr_v                 ( obuf_read_req                  ), //output
    .obuf_st_addr                   ( obuf_write_addr                ), //output
    .obuf_st_addr_v                 ( obuf_write_req                 ), //output
    .ibuf_ld_addr                   ( ibuf_read_addr                 ), //output
    .ibuf_ld_addr_v                 ( ibuf_read_req                  ), //output
    .wbuf_ld_addr                   ( wbuf_read_addr                 ), //output
    .wbuf_ld_addr_v                 ( wbuf_read_req                  ), //output

    .bias_ld_addr                   ( bias_read_addr                 ), //output
    .bias_ld_addr_v                 ( bias_read_req                  ), //output

    .bias_prev_sw                   ( rd_bias_prev_sw                ), //output
    .ddr_pe_sw                      (                                )  //output
    );
//=============================================================

//=============================================================
// 4x Memory wrappers - IBUF, WBUF, OBUF, Bias
//=============================================================
  ibuf_mem_wrapper #(
    // Internal Parameters
    .AXI_DATA_WIDTH                 ( IBUF_AXI_DATA_WIDTH            ),
    .AXI_BURST_WIDTH                ( AXI_BURST_WIDTH                ),
    .MEM_ID                         ( 0                              ),
    .NUM_TAGS                       ( NUM_TAGS                       ),
    .ARRAY_N                        ( ARRAY_N                        ),
    .DATA_WIDTH                     ( DATA_WIDTH                     ),
    .MEM_REQ_W                      ( MEM_REQ_W                      ),
    .ADDR_WIDTH                     ( ADDR_WIDTH                     ),
    .BUF_ADDR_W                     ( IBUF_ADDR_WIDTH                ),
    .LOOP_ITER_W                    ( LOOP_ITER_W                    ),
    .LOOP_ID_W                      ( LOOP_ID_W                      )
  ) ibuf_mem (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input

    .compute_done                   ( compute_done                   ), //input
    .compute_ready                  ( ibuf_compute_ready             ), //input
    .compute_bias_prev_sw           (                                ), //output

    .block_done                     ( tag_flush                      ), //input
    .tag_req                        ( sync_tag_req                   ), //input
    .tag_reuse                      ( ibuf_tag_reuse                 ), //input
    .tag_bias_prev_sw               ( tag_bias_prev_sw               ), //input
    .tag_ddr_pe_sw                  ( tag_ddr_pe_sw                  ), //input
    .tag_ready                      ( ibuf_tag_ready                 ), //output
    .tag_done                       ( ibuf_tag_done                  ), //output

    .tag_base_ld_addr               ( ibuf_ld_addr                   ), //input

    .cfg_loop_iter_v                ( cfg_loop_iter_v                ), //input
    .cfg_loop_iter                  ( cfg_loop_iter                  ), //input
    .cfg_loop_iter_loop_id          ( cfg_loop_iter_loop_id          ), //input

    .cfg_loop_stride_v              ( cfg_loop_stride_v              ), //input
    .cfg_loop_stride                ( cfg_loop_stride                ), //input
    .cfg_loop_stride_loop_id        ( cfg_loop_stride_loop_id        ), //input
    .cfg_loop_stride_type           ( cfg_loop_stride_type           ), //input
    .cfg_loop_stride_id             ( cfg_loop_stride_id             ), //input

    .cfg_mem_req_v                  ( cfg_mem_req_v                  ),
    .cfg_mem_req_size               ( cfg_mem_req_size               ),
    .cfg_mem_req_type               ( cfg_mem_req_type               ), // 0: RD, 1:WR
    .cfg_mem_req_id                 ( cfg_mem_req_id                 ), // specify which scratchpad
    .cfg_mem_req_loop_id            ( cfg_mem_req_loop_id            ), // specify which loop

    .buf_read_data                  ( ibuf_read_data                 ),
    .buf_read_req                   ( ibuf_read_req                  ),
    .buf_read_addr                  ( ibuf_read_addr                 ),

    .mws_awaddr                     ( cl_ddr0_awaddr                 ),
    .mws_awlen                      ( cl_ddr0_awlen                  ),
    .mws_awsize                     ( cl_ddr0_awsize                 ),
    .mws_awburst                    ( cl_ddr0_awburst                ),
    .mws_awvalid                    ( cl_ddr0_awvalid                ),
    .mws_awready                    ( cl_ddr0_awready                ),
    .mws_wdata                      ( cl_ddr0_wdata                  ),
    .mws_wstrb                      ( cl_ddr0_wstrb                  ),
    .mws_wlast                      ( cl_ddr0_wlast                  ),
    .mws_wvalid                     ( cl_ddr0_wvalid                 ),
    .mws_wready                     ( cl_ddr0_wready                 ),
    .mws_bresp                      ( cl_ddr0_bresp                  ),
    .mws_bvalid                     ( cl_ddr0_bvalid                 ),
    .mws_bready                     ( cl_ddr0_bready                 ),
    .mws_araddr                     ( cl_ddr0_araddr                 ),
    .mws_arid                       ( cl_ddr0_arid                   ),
    .mws_arlen                      ( cl_ddr0_arlen                  ),
    .mws_arsize                     ( cl_ddr0_arsize                 ),
    .mws_arburst                    ( cl_ddr0_arburst                ),
    .mws_arvalid                    ( cl_ddr0_arvalid                ),
    .mws_arready                    ( cl_ddr0_arready                ),
    .mws_rdata                      ( cl_ddr0_rdata                  ),
    .mws_rid                        ( cl_ddr0_rid                    ),
    .mws_rresp                      ( cl_ddr0_rresp                  ),
    .mws_rlast                      ( cl_ddr0_rlast                  ),
    .mws_rvalid                     ( cl_ddr0_rvalid                 ),
    .mws_rready                     ( cl_ddr0_rready                 )
    );

  wbuf_mem_wrapper #(
    // Internal Parameters
    .AXI_DATA_WIDTH                 ( WBUF_AXI_DATA_WIDTH            ),
    .AXI_BURST_WIDTH                ( AXI_BURST_WIDTH                ),
    .MEM_ID                         ( 2                              ),
    .NUM_TAGS                       ( NUM_TAGS                       ),
    .ARRAY_N                        ( ARRAY_N                        ),
    .DATA_WIDTH                     ( DATA_WIDTH                     ),
    .MEM_REQ_W                      ( MEM_REQ_W                      ),
    .ADDR_WIDTH                     ( ADDR_WIDTH                     ),
    .BUF_ADDR_W                     ( WBUF_ADDR_WIDTH                ),
    .LOOP_ITER_W                    ( LOOP_ITER_W                    ),
    .LOOP_ID_W                      ( LOOP_ID_W                      )
  ) wbuf_mem (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input

    .compute_done                   ( compute_done                   ), //input
    .compute_ready                  ( wbuf_compute_ready             ), //input
    .compute_bias_prev_sw           (                                ), //output
    .block_done                     ( tag_flush                      ), //input
    .tag_req                        ( sync_tag_req                   ), //input
    .tag_reuse                      ( wbuf_tag_reuse                 ), //input
    .tag_bias_prev_sw               ( tag_bias_prev_sw               ), //input
    .tag_ddr_pe_sw                  ( tag_ddr_pe_sw                  ), //input
    .tag_ready                      ( wbuf_tag_ready                 ), //output
    .tag_done                       ( wbuf_tag_done                  ), //output

    .tag_base_ld_addr               ( wbuf_ld_addr                   ), //input

    .cfg_loop_iter_v                ( cfg_loop_iter_v                ), //input
    .cfg_loop_iter                  ( cfg_loop_iter                  ), //input
    .cfg_loop_iter_loop_id          ( cfg_loop_iter_loop_id          ), //input

    .cfg_loop_stride_v              ( cfg_loop_stride_v              ), //input
    .cfg_loop_stride                ( cfg_loop_stride                ), //input
    .cfg_loop_stride_loop_id        ( cfg_loop_stride_loop_id        ), //input
    .cfg_loop_stride_type           ( cfg_loop_stride_type           ), //input
    .cfg_loop_stride_id             ( cfg_loop_stride_id             ), //input

    .cfg_mem_req_v                  ( cfg_mem_req_v                  ),
    .cfg_mem_req_size               ( cfg_mem_req_size               ),
    .cfg_mem_req_type               ( cfg_mem_req_type               ), // 0: RD, 1:WR
    .cfg_mem_req_id                 ( cfg_mem_req_id                 ), // specify which scratchpad
    .cfg_mem_req_loop_id            ( cfg_mem_req_loop_id            ), // specify which loop

    .buf_read_data                  ( wbuf_read_data                 ),
    .buf_read_req                   ( wbuf_read_req                  ),
    .buf_read_addr                  ( wbuf_read_addr                 ),

    .mws_awaddr                     ( cl_ddr2_awaddr                 ),
    .mws_awlen                      ( cl_ddr2_awlen                  ),
    .mws_awsize                     ( cl_ddr2_awsize                 ),
    .mws_awburst                    ( cl_ddr2_awburst                ),
    .mws_awvalid                    ( cl_ddr2_awvalid                ),
    .mws_awready                    ( cl_ddr2_awready                ),
    .mws_wdata                      ( cl_ddr2_wdata                  ),
    .mws_wstrb                      ( cl_ddr2_wstrb                  ),
    .mws_wlast                      ( cl_ddr2_wlast                  ),
    .mws_wvalid                     ( cl_ddr2_wvalid                 ),
    .mws_wready                     ( cl_ddr2_wready                 ),
    .mws_bresp                      ( cl_ddr2_bresp                  ),
    .mws_bvalid                     ( cl_ddr2_bvalid                 ),
    .mws_bready                     ( cl_ddr2_bready                 ),
    .mws_araddr                     ( cl_ddr2_araddr                 ),
    .mws_arid                       ( cl_ddr2_arid                   ),
    .mws_arlen                      ( cl_ddr2_arlen                  ),
    .mws_arsize                     ( cl_ddr2_arsize                 ),
    .mws_arburst                    ( cl_ddr2_arburst                ),
    .mws_arvalid                    ( cl_ddr2_arvalid                ),
    .mws_arready                    ( cl_ddr2_arready                ),
    .mws_rdata                      ( cl_ddr2_rdata                  ),
    .mws_rid                        ( cl_ddr2_rid                    ),
    .mws_rresp                      ( cl_ddr2_rresp                  ),
    .mws_rlast                      ( cl_ddr2_rlast                  ),
    .mws_rvalid                     ( cl_ddr2_rvalid                 ),
    .mws_rready                     ( cl_ddr2_rready                 )
    );

  obuf_mem_wrapper #(
    // Internal Parameters
    .AXI_DATA_WIDTH                 ( OBUF_AXI_DATA_WIDTH            ),
    .AXI_BURST_WIDTH                ( AXI_BURST_WIDTH                ),
    .MEM_ID                         ( 1                              ),
    .NUM_TAGS                       ( NUM_TAGS                       ),
    .ARRAY_N                        ( ARRAY_N                        ),
    .ARRAY_M                        ( ARRAY_M                        ),
    .DATA_WIDTH                     ( 64                             ),
    .MEM_REQ_W                      ( MEM_REQ_W                      ),
    .ADDR_WIDTH                     ( ADDR_WIDTH                     ),
    .BUF_ADDR_W                     ( OBUF_ADDR_WIDTH                ),
    .LOOP_ITER_W                    ( LOOP_ITER_W                    ),
    .LOOP_ID_W                      ( LOOP_ID_W                      )
  ) obuf_mem (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input

    .compute_done                   ( compute_done                   ), //input
    .compute_ready                  ( obuf_compute_ready             ), //output
    .compute_bias_prev_sw           ( obuf_bias_prev_sw              ), //output
    .block_done                     ( tag_flush                      ), //input
    .tag_req                        ( sync_tag_req                   ), //input
    .tag_reuse                      ( obuf_tag_reuse                 ), //input
    .tag_bias_prev_sw               ( tag_bias_prev_sw               ), //input
    .tag_ddr_pe_sw                  ( tag_ddr_pe_sw                  ), //input
    .tag_ready                      ( obuf_tag_ready                 ), //output
    .tag_done                       ( obuf_tag_done                  ), //output

    .tag_base_ld_addr               ( obuf_ld_addr                   ), //input
    .tag_base_st_addr               ( obuf_st_addr                   ), //input

    .cfg_loop_iter_v                ( cfg_loop_iter_v                ), //input
    .cfg_loop_iter                  ( cfg_loop_iter                  ), //input
    .cfg_loop_iter_loop_id          ( cfg_loop_iter_loop_id          ), //input

    .cfg_loop_stride_v              ( cfg_loop_stride_v              ), //input
    .cfg_loop_stride                ( cfg_loop_stride                ), //input
    .cfg_loop_stride_loop_id        ( cfg_loop_stride_loop_id        ), //input
    .cfg_loop_stride_type           ( cfg_loop_stride_type           ), //input
    .cfg_loop_stride_id             ( cfg_loop_stride_id             ), //input

    .cfg_mem_req_v                  ( cfg_mem_req_v                  ),
    .cfg_mem_req_size               ( cfg_mem_req_size               ),
    .cfg_mem_req_type               ( cfg_mem_req_type               ), // 0: RD, 1:WR
    .cfg_mem_req_id                 ( cfg_mem_req_id                 ), // specify which scratchpad
    .cfg_mem_req_loop_id            ( cfg_mem_req_loop_id            ), // specify which loop

    .buf_write_data                 ( obuf_write_data                ),
    .buf_write_req                  ( sys_obuf_write_req             ),
    .buf_write_addr                 ( sys_obuf_write_addr            ),
    .buf_read_data                  ( obuf_read_data                 ),
    .buf_read_req                   ( sys_obuf_read_req              ),
    .buf_read_addr                  ( sys_obuf_read_addr             ),

    .pu_buf_read_ready              ( ld_obuf_ready                  ),
    .pu_buf_read_req                ( ld_obuf_req                    ),
    .pu_buf_read_addr               ( ld_obuf_addr                   ),

    .pu_compute_start               ( pu_compute_start               ), //output
    .pu_compute_done                ( pu_compute_done                ), //input
    .pu_compute_ready               ( pu_compute_ready               ), //input

    .obuf_ld_stream_write_req       ( obuf_ld_stream_write_req       ),
    .obuf_ld_stream_write_data      ( obuf_ld_stream_write_data      ),

    .stmem_state                    ( stmem_state                    ), //output
    .stmem_tag                      ( stmem_tag                      ), //output
    .stmem_ddr_pe_sw                ( stmem_ddr_pe_sw                ), //output

    .mws_awaddr                     ( cl_ddr1_awaddr                 ),
    .mws_awlen                      ( cl_ddr1_awlen                  ),
    .mws_awsize                     ( cl_ddr1_awsize                 ),
    .mws_awburst                    ( cl_ddr1_awburst                ),
    .mws_awvalid                    ( cl_ddr1_awvalid                ),
    .mws_awready                    ( cl_ddr1_awready                ),
    .mws_wdata                      ( cl_ddr1_wdata                  ),
    .mws_wstrb                      ( cl_ddr1_wstrb                  ),
    .mws_wlast                      ( cl_ddr1_wlast                  ),
    .mws_wvalid                     ( cl_ddr1_wvalid                 ),
    .mws_wready                     ( cl_ddr1_wready                 ),
    .mws_bresp                      ( cl_ddr1_bresp                  ),
    .mws_bvalid                     ( cl_ddr1_bvalid                 ),
    .mws_bready                     ( cl_ddr1_bready                 ),
    .mws_araddr                     ( cl_ddr1_araddr                 ),
    .mws_arid                       ( cl_ddr1_arid                   ),
    .mws_arlen                      ( cl_ddr1_arlen                  ),
    .mws_arsize                     ( cl_ddr1_arsize                 ),
    .mws_arburst                    ( cl_ddr1_arburst                ),
    .mws_arvalid                    ( cl_ddr1_arvalid                ),
    .mws_arready                    ( cl_ddr1_arready                ),
    .mws_rdata                      ( cl_ddr1_rdata                  ),
    .mws_rid                        ( cl_ddr1_rid                    ),
    .mws_rresp                      ( cl_ddr1_rresp                  ),
    .mws_rlast                      ( cl_ddr1_rlast                  ),
    .mws_rvalid                     ( cl_ddr1_rvalid                 ),
    .mws_rready                     ( cl_ddr1_rready                 )
    );

  bbuf_mem_wrapper #(
    // Internal Parameters
    .AXI_DATA_WIDTH                 ( BBUF_AXI_DATA_WIDTH            ),
    .AXI_BURST_WIDTH                ( AXI_BURST_WIDTH                ),
    .MEM_ID                         ( 3                              ),
    .NUM_TAGS                       ( NUM_TAGS                       ),
    .ARRAY_N                        ( ARRAY_N                        ),
    .ARRAY_M                        ( ARRAY_M                        ),
    .DATA_WIDTH                     ( 32                             ),
    .MEM_REQ_W                      ( MEM_REQ_W                      ),
    .ADDR_WIDTH                     ( ADDR_WIDTH                     ),
    .BUF_ADDR_W                     ( BBUF_ADDR_WIDTH                ),
    .LOOP_ITER_W                    ( LOOP_ITER_W                    ),
    .LOOP_ID_W                      ( LOOP_ID_W                      )
  ) bbuf_mem (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input

    .compute_done                   ( compute_done                   ), //input
    .compute_ready                  ( bias_compute_ready             ), //input
    .compute_bias_prev_sw           (                                ), //output
    .block_done                     ( tag_flush                      ), //input
    .tag_req                        ( sync_tag_req                   ), //input
    .tag_reuse                      ( bias_tag_reuse                 ), //input
    .tag_bias_prev_sw               ( tag_bias_prev_sw               ), //input
    .tag_ddr_pe_sw                  ( tag_ddr_pe_sw                  ), //input
    .tag_ready                      ( bias_tag_ready                 ), //output
    .tag_done                       ( bias_tag_done                  ), //output

    .tag_base_ld_addr               ( bias_ld_addr                   ), //input

    .cfg_loop_iter_v                ( cfg_loop_iter_v                ), //input
    .cfg_loop_iter                  ( cfg_loop_iter                  ), //input
    .cfg_loop_iter_loop_id          ( cfg_loop_iter_loop_id          ), //input

    .cfg_loop_stride_v              ( cfg_loop_stride_v              ), //input
    .cfg_loop_stride                ( cfg_loop_stride                ), //input
    .cfg_loop_stride_loop_id        ( cfg_loop_stride_loop_id        ), //input
    .cfg_loop_stride_type           ( cfg_loop_stride_type           ), //input
    .cfg_loop_stride_id             ( cfg_loop_stride_id             ), //input

    .cfg_mem_req_v                  ( cfg_mem_req_v                  ),
    .cfg_mem_req_size               ( cfg_mem_req_size               ),
    .cfg_mem_req_type               ( cfg_mem_req_type               ), // 0: RD, 1:WR
    .cfg_mem_req_id                 ( cfg_mem_req_id                 ), // specify which scratchpad
    .cfg_mem_req_loop_id            ( cfg_mem_req_loop_id            ), // specify which loop

    .buf_read_data                  ( bbuf_read_data                 ),
    .buf_read_req                   ( sys_bias_read_req              ),
    .buf_read_addr                  ( sys_bias_read_addr             ),

    .mws_awaddr                     ( cl_ddr3_awaddr                 ),
    .mws_awlen                      ( cl_ddr3_awlen                  ),
    .mws_awsize                     ( cl_ddr3_awsize                 ),
    .mws_awburst                    ( cl_ddr3_awburst                ),
    .mws_awvalid                    ( cl_ddr3_awvalid                ),
    .mws_awready                    ( cl_ddr3_awready                ),
    .mws_wdata                      ( cl_ddr3_wdata                  ),
    .mws_wstrb                      ( cl_ddr3_wstrb                  ),
    .mws_wlast                      ( cl_ddr3_wlast                  ),
    .mws_wvalid                     ( cl_ddr3_wvalid                 ),
    .mws_wready                     ( cl_ddr3_wready                 ),
    .mws_bresp                      ( cl_ddr3_bresp                  ),
    .mws_bvalid                     ( cl_ddr3_bvalid                 ),
    .mws_bready                     ( cl_ddr3_bready                 ),
    .mws_araddr                     ( cl_ddr3_araddr                 ),
    .mws_arid                       ( cl_ddr3_arid                   ),
    .mws_arlen                      ( cl_ddr3_arlen                  ),
    .mws_arsize                     ( cl_ddr3_arsize                 ),
    .mws_arburst                    ( cl_ddr3_arburst                ),
    .mws_arvalid                    ( cl_ddr3_arvalid                ),
    .mws_arready                    ( cl_ddr3_arready                ),
    .mws_rdata                      ( cl_ddr3_rdata                  ),
    .mws_rid                        ( cl_ddr3_rid                    ),
    .mws_rresp                      ( cl_ddr3_rresp                  ),
    .mws_rlast                      ( cl_ddr3_rlast                  ),
    .mws_rvalid                     ( cl_ddr3_rvalid                 ),
    .mws_rready                     ( cl_ddr3_rready                 )
    );
//=============================================================

//=============================================================
// Systolic Array
//=============================================================
  // Only select bias (0) if rd_bias_prev_sw == 0 and obuf_bias_prev_sw == 0;
    assign sys_array_c_sel = rd_bias_prev_sw || obuf_bias_prev_sw;
  systolic_array #(
    .OBUF_ADDR_WIDTH                ( OBUF_ADDR_WIDTH                ),
    .BBUF_ADDR_WIDTH                ( BBUF_ADDR_WIDTH                ),
    .ACT_WIDTH                      ( DATA_WIDTH                     ),
    .WGT_WIDTH                      ( DATA_WIDTH                     ),
    .BIAS_WIDTH                     ( BIAS_WIDTH                     ),
    .ACC_WIDTH                      ( ACC_WIDTH                      ),
    .ARRAY_N                        ( ARRAY_N                        ),
    .ARRAY_M                        ( ARRAY_M                        )
  ) sys_array (
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .acc_clear                      ( acc_clear                      ),

    .ibuf_read_data                 ( ibuf_read_data                 ),

    .wbuf_read_data                 ( wbuf_read_data                 ),

    .bbuf_read_data                 ( bbuf_read_data                 ),
    .bias_read_req                  ( bias_read_req                  ),
    .bias_read_addr                 ( bias_read_addr                 ),
    .sys_bias_read_req              ( sys_bias_read_req              ),
    .sys_bias_read_addr             ( sys_bias_read_addr             ),
    .bias_prev_sw                   ( sys_array_c_sel                ),

    .obuf_read_data                 ( obuf_read_data                 ),
    .obuf_read_addr                 ( obuf_read_addr                 ),
    .sys_obuf_read_req              ( sys_obuf_read_req              ),
    .sys_obuf_read_addr             ( sys_obuf_read_addr             ),
    .obuf_write_req                 ( obuf_write_req                 ),
    .obuf_write_addr                ( obuf_write_addr                ),
    .obuf_write_data                ( sys_obuf_write_data            ),
    .sys_obuf_write_req             ( sys_obuf_write_req             ),
    .sys_obuf_write_addr            ( sys_obuf_write_addr            )
  );


    wire [ 64                   -1 : 0 ]        obuf_out0;
    wire [ 64                   -1 : 0 ]        obuf_out1;
    wire [ 64                   -1 : 0 ]        obuf_out2;
    wire [ 64                   -1 : 0 ]        obuf_out3;

    wire [ 64                   -1 : 0 ]        obuf_out4;
    wire [ 64                   -1 : 0 ]        obuf_out5;
    wire [ 64                   -1 : 0 ]        obuf_out6;
    wire [ 64                   -1 : 0 ]        obuf_out7;

    wire [ 64                   -1 : 0 ]        obuf_in0;
    wire [ 64                   -1 : 0 ]        obuf_in1;
    wire [ 64                   -1 : 0 ]        obuf_in2;
    wire [ 64                   -1 : 0 ]        obuf_in3;

    wire [ 32                   -1 : 0 ]        bias_in0;
    wire [ 32                   -1 : 0 ]        bias_in1;
    wire [ 32                   -1 : 0 ]        bias_in2;
    wire [ 32                   -1 : 0 ]        bias_in3;

    wire [ 64                   -1 : 0 ]        obuf_mem_out0;
    wire [ 64                   -1 : 0 ]        obuf_mem_out1;

    wire [ 16                   -1 : 0 ]        ibuf_in0;
    wire [ 16                   -1 : 0 ]        ibuf_in1;
    wire [ 16                   -1 : 0 ]        ibuf_in2;
    wire [ 16                   -1 : 0 ]        ibuf_in3;

    wire [ 16                   -1 : 0 ]        ibuf_in4;
    wire [ 16                   -1 : 0 ]        ibuf_in5;
    wire [ 16                   -1 : 0 ]        ibuf_in6;
    wire [ 16                   -1 : 0 ]        ibuf_in7;

    wire [ 16                   -1 : 0 ]        wbuf_in0;
    wire [ 16                   -1 : 0 ]        wbuf_in1;
    wire [ 16                   -1 : 0 ]        wbuf_in2;
    wire [ 16                   -1 : 0 ]        wbuf_in3;

    wire [ 16                   -1 : 0 ]        wbuf_in4;
    wire [ 16                   -1 : 0 ]        wbuf_in5;
    wire [ 16                   -1 : 0 ]        wbuf_in6;
    wire [ 16                   -1 : 0 ]        wbuf_in7;

    wire [ 16                   -1 : 0 ]        wbuf_in8;
    wire [ 16                   -1 : 0 ]        wbuf_in9;
    wire [ 16                   -1 : 0 ]        wbuf_in10;
    wire [ 16                   -1 : 0 ]        wbuf_in11;

    wire [ 16                   -1 : 0 ]        wbuf_in12;
    wire [ 16                   -1 : 0 ]        wbuf_in13;
    wire [ 16                   -1 : 0 ]        wbuf_in14;
    wire [ 16                   -1 : 0 ]        wbuf_in15;

    assign {obuf_out7,obuf_out6,obuf_out5,obuf_out4,
      obuf_out3, obuf_out2, obuf_out1, obuf_out0} = sys_obuf_write_data;
    assign {obuf_in3, obuf_in2, obuf_in1, obuf_in0} = obuf_read_data;
    assign {bias_in3, bias_in2, bias_in1, bias_in0} = bbuf_read_data;
    assign {obuf_mem_out1, obuf_mem_out0} = cl_ddr1_wdata;

    assign ibuf_in0 = ibuf_read_data[15:0];
    assign ibuf_in1 = ibuf_read_data[31:16];
    assign ibuf_in2 = ibuf_read_data[47:32];
    assign ibuf_in3 = ibuf_read_data[63:48];

    assign ibuf_in4 = ibuf_read_data[79 :64];
    assign ibuf_in5 = ibuf_read_data[95 :80];
    assign ibuf_in6 = ibuf_read_data[111:96];
    assign ibuf_in7 = ibuf_read_data[127:112];

    assign wbuf_in0 = wbuf_read_data[15:0];
    assign wbuf_in1 = wbuf_read_data[31:16];
    assign wbuf_in2 = wbuf_read_data[47:32];
    assign wbuf_in3 = wbuf_read_data[63:48];

    assign wbuf_in4 = wbuf_read_data[79:64];
    assign wbuf_in5 = wbuf_read_data[95:80];
    assign wbuf_in6 = wbuf_read_data[111:96];
    assign wbuf_in7 = wbuf_read_data[127:112];

    assign wbuf_in8 = wbuf_read_data[143:128];
    assign wbuf_in9 = wbuf_read_data[159:144];
    assign wbuf_in10 = wbuf_read_data[175:160];
    assign wbuf_in11 = wbuf_read_data[191:176];

    assign wbuf_in12 = wbuf_read_data[207:192];
    assign wbuf_in13 = wbuf_read_data[223:208];
    assign wbuf_in14 = wbuf_read_data[239:224];
    assign wbuf_in15 = wbuf_read_data[255:240];

    assign obuf_write_data = sys_obuf_write_data;
//=============================================================

//=============================================================
// PU
//=============================================================
  gen_pu #(
    .INST_WIDTH                     ( INST_DATA_WIDTH                ),
    .DATA_WIDTH                     ( DATA_WIDTH                     ),
    .ACC_DATA_WIDTH                 ( 64                             ),
    .SIMD_LANES                     ( ARRAY_M                        ),
    .OBUF_AXI_DATA_WIDTH            ( OBUF_AXI_DATA_WIDTH            ),
    .AXI_DATA_WIDTH                 ( PU_AXI_DATA_WIDTH              ),
    .AXI_ADDR_WIDTH                 ( AXI_ADDR_WIDTH                 ),
    .OBUF_ADDR_WIDTH                ( PU_OBUF_ADDR_WIDTH             ),
    .AXI_BURST_WIDTH                ( AXI_BURST_WIDTH                )
  ) u_pu
  (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .done                           ( pu_done                        ), //output
    //DEBUG
    .obuf_ld_stream_read_count      ( obuf_ld_stream_read_count      ), //output
    .obuf_ld_stream_write_count     ( obuf_ld_stream_write_count     ), //output
    .ddr_st_stream_read_count       ( ddr_st_stream_read_count       ), //output
    .ddr_st_stream_write_count      ( ddr_st_stream_write_count      ), //output
    .ld0_stream_counts              ( ld0_stream_counts              ), //output
    .ld1_stream_counts              ( ld1_stream_counts              ), //output
    .axi_wr_fifo_counts             ( axi_wr_fifo_counts             ), //output
    //DEBUG
    .pu_ctrl_state                  ( pu_ctrl_state                  ), //output
    .obuf_ld_stream_write_req       ( obuf_ld_stream_write_req       ), //input
    .obuf_ld_stream_write_data      ( obuf_ld_stream_write_data      ), //input
    .inst_wr_req                    ( cfg_pu_inst_v                  ), //input
    .inst_wr_data                   ( cfg_pu_inst                    ), //input
    .pu_block_start                 ( pu_block_start                 ), //input
    .pu_compute_start               ( pu_compute_start               ), //input
    .pu_compute_ready               ( pu_compute_ready               ), //output
    .pu_compute_done                ( pu_compute_done                ), //output
    .pu_write_done                  ( pu_write_done                  ), //output
    .inst_wr_ready                  ( pu_inst_wr_ready               ), //output
    .ld_obuf_req                    ( ld_obuf_req                    ), //output
    .ld_obuf_addr                   ( ld_obuf_addr                   ), //output
    .ld_obuf_ready                  ( ld_obuf_ready                  ), //input
    .pu_ddr_awaddr                  ( cl_ddr4_awaddr                 ), //output
    .pu_ddr_awlen                   ( cl_ddr4_awlen                  ), //output
    .pu_ddr_awsize                  ( cl_ddr4_awsize                 ), //output
    .pu_ddr_awburst                 ( cl_ddr4_awburst                ), //output
    .pu_ddr_awvalid                 ( cl_ddr4_awvalid                ), //output
    .pu_ddr_awready                 ( cl_ddr4_awready                ), //input
    .pu_ddr_wdata                   ( cl_ddr4_wdata                  ), //output
    .pu_ddr_wstrb                   ( cl_ddr4_wstrb                  ), //output
    .pu_ddr_wlast                   ( cl_ddr4_wlast                  ), //output
    .pu_ddr_wvalid                  ( cl_ddr4_wvalid                 ), //output
    .pu_ddr_wready                  ( cl_ddr4_wready                 ), //input
    .pu_ddr_bresp                   ( cl_ddr4_bresp                  ), //input
    .pu_ddr_bvalid                  ( cl_ddr4_bvalid                 ), //input
    .pu_ddr_bready                  ( cl_ddr4_bready                 ), //output
    .pu_ddr_araddr                  ( cl_ddr4_araddr                 ), //output
    .pu_ddr_arid                    ( cl_ddr4_arid                   ), //output
    .pu_ddr_arlen                   ( cl_ddr4_arlen                  ), //output
    .pu_ddr_arsize                  ( cl_ddr4_arsize                 ), //output
    .pu_ddr_arburst                 ( cl_ddr4_arburst                ), //output
    .pu_ddr_arvalid                 ( cl_ddr4_arvalid                ), //output
    .pu_ddr_arready                 ( cl_ddr4_arready                ), //input
    .pu_ddr_rdata                   ( cl_ddr4_rdata                  ), //input
    .pu_ddr_rid                     ( cl_ddr4_rid                    ), //input
    .pu_ddr_rresp                   ( cl_ddr4_rresp                  ), //input
    .pu_ddr_rlast                   ( cl_ddr4_rlast                  ), //input
    .pu_ddr_rvalid                  ( cl_ddr4_rvalid                 ), //input
    .pu_ddr_rready                  ( cl_ddr4_rready                 ) //output
  );
//=============================================================

//=============================================================
// VCD
//=============================================================
  `ifdef COCOTB_TOPLEVEL_dnnweaver2_controller
  initial begin
    $dumpfile("dnnweaver2_controller.vcd");
    $dumpvars(0, dnnweaver2_controller);
  end
  `endif
//=============================================================

endmodule

//
// Banked RAM
//  Allows simultaneous accesses for LD/ST and RD/WR instructions
//
// Hardik Sharma
// (hsharma@gatech.edu)
`timescale 1ns/1ps
module banked_ram
#(
    parameter integer  TAG_W                        = 2,
    parameter integer  NUM_TAGS                     = (1<<TAG_W),
    parameter integer  DATA_WIDTH                   = 16,
    parameter integer  ADDR_WIDTH                   = 13,
    parameter integer  LOCAL_ADDR_W                 = ADDR_WIDTH - TAG_W
)
(
    input  wire                                         clk,
    input  wire                                         reset,

  // LD/ST
    input  wire                                         s_read_req_a,
    input  wire  [ ADDR_WIDTH           -1 : 0 ]        s_read_addr_a,
    output wire  [ DATA_WIDTH           -1 : 0 ]        s_read_data_a,

    input  wire                                         s_write_req_a,
    input  wire  [ ADDR_WIDTH           -1 : 0 ]        s_write_addr_a,
    input  wire  [ DATA_WIDTH           -1 : 0 ]        s_write_data_a,

  // RD/WR
    input  wire                                         s_read_req_b,
    input  wire  [ ADDR_WIDTH           -1 : 0 ]        s_read_addr_b,
    output wire  [ DATA_WIDTH           -1 : 0 ]        s_read_data_b,

    input  wire                                         s_write_req_b,
    input  wire  [ ADDR_WIDTH           -1 : 0 ]        s_write_addr_b,
    input  wire  [ DATA_WIDTH           -1 : 0 ]        s_write_data_b
);

//=============================================================
// Localparams
//=============================================================
    localparam          LOCAL_READ_WIDTH             = DATA_WIDTH * NUM_TAGS;
//=============================================================


//=============================================================
// Wires/Regs
//=============================================================
  genvar i;
    wire [ TAG_W                -1 : 0 ]        wr_tag_a;
    wire [ LOCAL_ADDR_W         -1 : 0 ]        wr_addr_a;
    wire [ TAG_W                -1 : 0 ]        wr_tag_b;
    wire [ LOCAL_ADDR_W         -1 : 0 ]        wr_addr_b;

    wire [ TAG_W                -1 : 0 ]        rd_tag_a;
    wire [ TAG_W                -1 : 0 ]        rd_tag_b;
    reg  [ TAG_W                -1 : 0 ]        rd_tag_a_dly;
    reg  [ TAG_W                -1 : 0 ]        rd_tag_b_dly;
    wire [ LOCAL_ADDR_W         -1 : 0 ]        rd_addr_a;
    wire [ LOCAL_ADDR_W         -1 : 0 ]        rd_addr_b;

    wire [ LOCAL_READ_WIDTH     -1 : 0 ]        local_read_data_a;
    wire [ LOCAL_READ_WIDTH     -1 : 0 ]        local_read_data_b;

//=============================================================

//=============================================================
// Assigns
//=============================================================
    assign {wr_tag_a, wr_addr_a} = s_write_addr_a;
    assign {wr_tag_b, wr_addr_b} = s_write_addr_b;

    assign {rd_tag_a, rd_addr_a} = s_read_addr_a;
    assign {rd_tag_b, rd_addr_b} = s_read_addr_b;

    always @(posedge clk)
    begin
      if (reset)
        rd_tag_a_dly <= 0;
      else if (s_read_req_a)
        rd_tag_a_dly <= rd_tag_a;
    end

    always @(posedge clk)
    begin
      if (reset)
        rd_tag_b_dly <= 0;
      else if (s_read_req_b)
        rd_tag_b_dly <= rd_tag_b;
    end
//=============================================================


//=============================================================
// RAM logic
//=============================================================
generate
  for (i=0; i<NUM_TAGS; i=i+1)
  begin: BANK_INST

    //(* ram_style = "block" *)
    //reg  [ DATA_WIDTH -1 : 0 ] bank_mem [ 0 : 1<<(LOCAL_ADDR_W) - 1 ];

    wire [ DATA_WIDTH           -1 : 0 ]        wdata;
    wire [ DATA_WIDTH           -1 : 0 ]        rdata;

    wire [ LOCAL_ADDR_W         -1 : 0 ]        waddr;
    wire [ LOCAL_ADDR_W         -1 : 0 ]        raddr;

    wire                                        local_wr_req_a;
    wire                                        local_wr_req_b;

    wire                                        local_rd_req_a;
    wire                                        local_rd_req_b;

    wire                                        local_rd_req_a_dly;
    wire                                        local_rd_req_b_dly;

    // Write port
    assign local_wr_req_a = (wr_tag_a == i) && s_write_req_a;
    assign local_wr_req_b = (wr_tag_b == i) && s_write_req_b;

    assign wdata = local_wr_req_a ? s_write_data_a : s_write_data_b;
    assign waddr = local_wr_req_a ? wr_addr_a : wr_addr_b;

    //always @(posedge clk)
    //begin: RAM_WRITE
    //  if (local_wr_req_a || local_wr_req_b)
    //    bank_mem[waddr] <= wdata;
    //end

  ram #(
    .ADDR_WIDTH                     ( LOCAL_ADDR_W),
    .DATA_WIDTH                     ( DATA_WIDTH)
  ) u_ram_bank_mem(
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .s_write_addr                   ( waddr),
    .s_write_req                    ( local_wr_req_a || local_wr_req_b),
    .s_write_data                   ( wdata),
    .s_read_addr                    ( raddr),
    .s_read_req                     ( local_rd_req_a || local_rd_req_b),
    .s_read_data                    ( rdata)
  );


    // Read port
    assign local_rd_req_a = (rd_tag_a == i) && s_read_req_a;
    assign local_rd_req_b = (rd_tag_b == i) && s_read_req_b;

    assign raddr = local_rd_req_a ? rd_addr_a  : rd_addr_b;
    register_sync #(1) reg_local_rd_req_a (clk, reset, local_rd_req_a, local_rd_req_a_dly);
    register_sync #(1) reg_local_rd_req_b (clk, reset, local_rd_req_b, local_rd_req_b_dly);
    // assign s_read_data_a = local_rd_req_a_dly ? rdata : {DATA_WIDTH{1'b0}};
    // assign s_read_data_b = local_rd_req_b_dly ? rdata : {DATA_WIDTH{1'b0}};

    assign local_read_data_a[i*DATA_WIDTH+:DATA_WIDTH] = rdata;
    assign local_read_data_b[i*DATA_WIDTH+:DATA_WIDTH] = rdata;

    //always @(posedge clk)
    //begin: RAM_READ
    //  if (local_rd_req_a || local_rd_req_b)
    //    rdata <= bank_mem[raddr];
    //end

    //assign rdata = bank_mem[raddr];


`ifdef simulation
    integer idx;
    initial begin
      for (idx=0; idx< (1<<LOCAL_ADDR_W); idx=idx+1)
      begin
        bank_mem[idx] = 32'hDEADBEEF;
      end
    end
`endif //simulation


  end
endgenerate
//=============================================================

//=============================================================
// Mux
//=============================================================
  mux_n_1 #(
    .WIDTH                          ( DATA_WIDTH                     ),
    .LOG2_N                         ( TAG_W                          )
  ) read_a_mux (
    .sel                            ( rd_tag_a_dly                   ),
    .data_in                        ( local_read_data_a              ),
    .data_out                       ( s_read_data_a                  )
  );

  mux_n_1 #(
    .WIDTH                          ( DATA_WIDTH                     ),
    .LOG2_N                         ( TAG_W                          )
  ) read_b_mux (
    .sel                            ( rd_tag_b_dly                   ),
    .data_in                        ( local_read_data_b              ),
    .data_out                       ( s_read_data_b                  )
  );
//=============================================================

endmodule
//
// Wrapper for memory
//
// Hardik Sharma
// (hsharma@gatech.edu)

`timescale 1ns/1ps
module ibuf_mem_wrapper #(
  // Internal Parameters
    parameter integer  MEM_ID                       = 0,
    parameter integer  MEM_REQ_W                    = 16,
    parameter integer  ADDR_WIDTH                   = 8,
    parameter integer  DATA_WIDTH                   = 32,
    parameter integer  LOOP_ITER_W                  = 16,
    parameter integer  ADDR_STRIDE_W                = 32,
    parameter integer  LOOP_ID_W                    = 5,
    parameter integer  BUF_TYPE_W                   = 2,
    parameter integer  NUM_TAGS                     = 4,
    parameter integer  TAG_W                        = $clog2(NUM_TAGS),

  // AXI
    parameter integer  AXI_ADDR_WIDTH               = 42,
    parameter integer  AXI_ID_WIDTH                 = 1,
    parameter integer  AXI_DATA_WIDTH               = 64,
    parameter integer  AXI_BURST_WIDTH              = 8,
    parameter integer  WSTRB_W                      = AXI_DATA_WIDTH/8,

  // Buffer
    parameter integer  ARRAY_N                      = 2,
    parameter integer  ARRAY_M                      = MEM_ID == 2 ? ARRAY_N : 1,
    parameter integer  BUF_DATA_WIDTH               = DATA_WIDTH * ARRAY_N * ARRAY_M,
    parameter integer  BUF_ADDR_W                   = 16,
    parameter integer  MEM_ADDR_W                   = BUF_ADDR_W + $clog2(BUF_DATA_WIDTH / AXI_DATA_WIDTH),
    parameter integer  TAG_BUF_ADDR_W               = BUF_ADDR_W + TAG_W,
    parameter integer  TAG_MEM_ADDR_W               = MEM_ADDR_W + TAG_W
) (
    input  wire                                         clk,
    input  wire                                         reset,

    input  wire                                         tag_req,
    input  wire                                         tag_reuse,
    input  wire                                         tag_bias_prev_sw,
    input  wire                                         tag_ddr_pe_sw,
    output wire                                         tag_ready,
    output wire                                         tag_done,
    input  wire                                         compute_done,
    input  wire                                         block_done,
    input  wire  [ ADDR_WIDTH           -1 : 0 ]        tag_base_ld_addr,

    output wire                                         compute_ready,
    output wire                                         compute_bias_prev_sw,

  // Programming
    input  wire                                         cfg_loop_stride_v,
    input  wire  [ ADDR_STRIDE_W        -1 : 0 ]        cfg_loop_stride,
    input  wire  [ LOOP_ID_W            -1 : 0 ]        cfg_loop_stride_loop_id,
    input  wire  [ BUF_TYPE_W           -1 : 0 ]        cfg_loop_stride_id,
    input  wire  [ 2                    -1 : 0 ]        cfg_loop_stride_type,

    input  wire                                         cfg_loop_iter_v,
    input  wire  [ LOOP_ITER_W          -1 : 0 ]        cfg_loop_iter,
    input  wire  [ LOOP_ID_W            -1 : 0 ]        cfg_loop_iter_loop_id,

    input  wire                                         cfg_mem_req_v,
    input  wire  [ BUF_TYPE_W           -1 : 0 ]        cfg_mem_req_id,
    input  wire  [ MEM_REQ_W            -1 : 0 ]        cfg_mem_req_size,
    input  wire  [ LOOP_ID_W            -1 : 0 ]        cfg_mem_req_loop_id,
    input  wire  [ 2                    -1 : 0 ]        cfg_mem_req_type,

  // Systolic Array
    output wire  [ BUF_DATA_WIDTH       -1 : 0 ]        buf_read_data,
    input  wire                                         buf_read_req,
    input  wire  [ BUF_ADDR_W           -1 : 0 ]        buf_read_addr,

  // CL_wrapper -> DDR AXI4 interface
    // Master Interface Write Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        mws_awaddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        mws_awlen,
    output wire  [ 3                    -1 : 0 ]        mws_awsize,
    output wire  [ 2                    -1 : 0 ]        mws_awburst,
    output wire                                         mws_awvalid,
    input  wire                                         mws_awready,
    // Master Interface Write Data
    output wire  [ AXI_DATA_WIDTH       -1 : 0 ]        mws_wdata,
    output wire  [ WSTRB_W              -1 : 0 ]        mws_wstrb,
    output wire                                         mws_wlast,
    output wire                                         mws_wvalid,
    input  wire                                         mws_wready,
    // Master Interface Write Response
    input  wire  [ 2                    -1 : 0 ]        mws_bresp,
    input  wire                                         mws_bvalid,
    output wire                                         mws_bready,
    // Master Interface Read Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        mws_araddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        mws_arlen,
    output wire  [ 3                    -1 : 0 ]        mws_arsize,
    output wire  [ 2                    -1 : 0 ]        mws_arburst,
    output wire                                         mws_arvalid,
    output wire  [ AXI_ID_WIDTH         -1 : 0 ]        mws_arid,
    input  wire                                         mws_arready,
    // Master Interface Read Data
    input  wire  [ AXI_DATA_WIDTH       -1 : 0 ]        mws_rdata,
    input  wire  [ 2                    -1 : 0 ]        mws_rresp,
    input  wire                                         mws_rlast,
    input  wire                                         mws_rvalid,
    input  wire  [ AXI_ID_WIDTH         -1 : 0 ]        mws_rid,
    output wire                                         mws_rready
);

//==============================================================================
// Localparams
//==============================================================================
    localparam integer  LDMEM_IDLE                   = 0;
    localparam integer  LDMEM_CHECK_RAW              = 1;
    localparam integer  LDMEM_BUSY                   = 2;
    localparam integer  LDMEM_WAIT_0                 = 3;
    localparam integer  LDMEM_WAIT_1                 = 4;
    localparam integer  LDMEM_WAIT_2                 = 5;
    localparam integer  LDMEM_WAIT_3                 = 6;
    localparam integer  LDMEM_DONE                   = 7;

    localparam integer  STMEM_IDLE                   = 0;
    localparam integer  STMEM_DDR                    = 1;
    localparam integer  STMEM_WAIT_0                 = 2;
    localparam integer  STMEM_WAIT_1                 = 3;
    localparam integer  STMEM_WAIT_2                 = 4;
    localparam integer  STMEM_WAIT_3                 = 5;
    localparam integer  STMEM_DONE                   = 6;
    localparam integer  STMEM_PU                     = 7;

    localparam integer  MEM_LD                       = 0;
    localparam integer  MEM_ST                       = 1;
    localparam integer  MEM_RD                       = 2;
    localparam integer  MEM_WR                       = 3;
//==============================================================================

//==============================================================================
// Wires/Regs
//==============================================================================
    wire [ TAG_MEM_ADDR_W       -1 : 0 ]        tag_mem_write_addr;
    wire [ TAG_BUF_ADDR_W       -1 : 0 ]        tag_buf_read_addr;

    wire                                        compute_tag_done;
    wire                                        compute_tag_reuse;
    wire                                        compute_tag_ready;
    wire [ TAG_W                -1 : 0 ]        compute_tag;
    wire                                        ldmem_tag_done;
    wire                                        ldmem_tag_ready;
    wire [ TAG_W                -1 : 0 ]        ldmem_tag;
    wire                                        stmem_tag_done;
    wire                                        stmem_tag_ready;
    wire [ TAG_W                -1 : 0 ]        stmem_tag;
    wire                                        stmem_ddr_pe_sw;

    reg  [ 4                    -1 : 0 ]        ldmem_state_d;
    reg  [ 4                    -1 : 0 ]        ldmem_state_q;

    reg  [ 3                    -1 : 0 ]        stmem_state_d;
    reg  [ 3                    -1 : 0 ]        stmem_state_q;

    wire                                        ld_mem_req_v;
    wire                                        st_mem_req_v;

    wire [ TAG_W                -1 : 0 ]        tag;


    reg                                         ld_iter_v_q;
    reg  [ LOOP_ITER_W          -1 : 0 ]        iter_q;
    reg                                         st_iter_v_q;

    reg  [ LOOP_ID_W            -1 : 0 ]        ld_loop_id_counter;
    reg  [ LOOP_ID_W            -1 : 0 ]        st_loop_id_counter;

    wire [ LOOP_ID_W            -1 : 0 ]        mws_ld_loop_iter_loop_id;
    wire [ LOOP_ITER_W          -1 : 0 ]        mws_ld_loop_iter;
    wire                                        mws_ld_loop_iter_v;
    wire                                        mws_ld_start;
    wire                                        mws_ld_done;
    wire                                        mws_ld_stall;
    wire                                        mws_ld_init;
    wire                                        mws_ld_enter;
    wire                                        mws_ld_exit;
    wire [ LOOP_ID_W            -1 : 0 ]        mws_ld_index;
    wire                                        mws_ld_index_valid;
    wire                                        mws_ld_step;

    wire                                        mws_st_stall;
    wire                                        mws_st_init;
    wire                                        mws_st_enter;
    wire                                        mws_st_exit;
    wire [ LOOP_ID_W            -1 : 0 ]        mws_st_index;
    wire                                        mws_st_index_valid;
    wire                                        mws_st_step;

    wire                                        ld_stride_v;
    wire [ ADDR_STRIDE_W        -1 : 0 ]        ld_stride;
    wire [ BUF_TYPE_W           -1 : 0 ]        ld_stride_id;
    wire                                        st_stride_v;
    wire [ ADDR_STRIDE_W        -1 : 0 ]        st_stride;
    wire [ BUF_TYPE_W           -1 : 0 ]        st_stride_id;

    wire [ ADDR_WIDTH           -1 : 0 ]        ld_addr;
    wire [ ADDR_WIDTH           -1 : 0 ]        mws_ld_base_addr;
    wire                                        ld_addr_v;


    reg  [ MEM_REQ_W            -1 : 0 ]        ld_req_size;
    wire                                        ld_req_valid_d;
    reg                                         ld_req_valid_q;
    reg  [ ADDR_WIDTH           -1 : 0 ]        ld_req_addr;

    //reg  [ ADDR_WIDTH           -1 : 0 ]        tag_ld_addr[0:NUM_TAGS-1];

    
    
    wire                                        axi_rd_req;
    wire [ AXI_ID_WIDTH         -1 : 0 ]        axi_rd_req_id;
    wire                                        axi_rd_done;
    wire [ MEM_REQ_W            -1 : 0 ]        axi_rd_req_size;
    wire                                        axi_rd_ready;
    wire [ AXI_ADDR_WIDTH       -1 : 0 ]        axi_rd_addr;

    wire                                        axi_wr_req;
    wire [ AXI_ID_WIDTH         -1 : 0 ]        axi_wr_req_id;
    wire                                        axi_wr_done;
    wire [ MEM_REQ_W            -1 : 0 ]        axi_wr_req_size;
    wire                                        axi_wr_ready;
    wire [ AXI_ADDR_WIDTH       -1 : 0 ]        axi_wr_addr;

    wire                                        mem_write_req;
    wire [ AXI_DATA_WIDTH       -1 : 0 ]        mem_write_data;
    reg  [ MEM_ADDR_W           -1 : 0 ]        mem_write_addr;
    wire                                        mem_write_ready;
    wire [ AXI_DATA_WIDTH       -1 : 0 ]        mem_read_data;
    wire                                        mem_read_req;
    wire                                        mem_read_ready;

  // Adding register to buf read data
    wire [ BUF_DATA_WIDTH       -1 : 0 ]        _buf_read_data;

  // Read-after-write
    reg                                         raw;
    wire [ TAG_W                -1 : 0 ]        raw_stmem_tag;
    wire                                        raw_stmem_tag_ready;
    wire [ ADDR_WIDTH           -1 : 0 ]        raw_stmem_st_addr;
    wire                                        pu_done;
    wire [ AXI_ID_WIDTH         -1 : 0 ]        mem_write_id;
    wire                                        ldmem_ready;
//==============================================================================

//==============================================================================
// Assigns
//==============================================================================
    assign pu_done= 1'b1;

    assign ld_stride = cfg_loop_stride;
    assign ld_stride_v = cfg_loop_stride_v && cfg_loop_stride_loop_id == 1 + MEM_ID && cfg_loop_stride_type == MEM_LD && cfg_loop_stride_id == MEM_ID;

    //assign mws_ld_base_addr = tag_ld_addr[ldmem_tag];
    assign axi_rd_req = ld_req_valid_q;
    assign axi_rd_req_size = ld_req_size * (ARRAY_N * ARRAY_M * DATA_WIDTH) / AXI_DATA_WIDTH;
    assign axi_rd_addr = ld_req_addr;

    assign axi_wr_req = 1'b0;
    assign axi_wr_req_id = 1'b0;
    assign axi_wr_req_size = 0;
    assign axi_wr_addr = 0;
//==============================================================================

//==============================================================================
// Address generators
//==============================================================================
    assign mws_ld_stall = ~ldmem_tag_ready || ~axi_rd_ready;
    assign mws_ld_step = mws_ld_index_valid && !mws_ld_stall;
  mem_walker_stride #(
    .ADDR_WIDTH                     ( ADDR_WIDTH                     ),
    .ADDR_STRIDE_W                  ( ADDR_STRIDE_W                  ),
    .LOOP_ID_W                      ( LOOP_ID_W                      )
  ) mws_ld (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .base_addr                      ( mws_ld_base_addr               ), //input
    .loop_ctrl_done                 ( mws_ld_done                    ), //input
    .loop_index                     ( mws_ld_index                   ), //input
    .loop_index_valid               ( mws_ld_step                    ), //input
    .loop_init                      ( mws_ld_init                    ), //input
    .loop_enter                     ( mws_ld_enter                   ), //input
    .loop_exit                      ( mws_ld_exit                    ), //input
    .cfg_addr_stride_v              ( ld_stride_v                    ), //input
    .cfg_addr_stride                ( ld_stride                      ), //input
    .addr_out                       ( ld_addr                        ), //output
    .addr_out_valid                 ( ld_addr_v                      )  //output
  );
//==============================================================================

//=============================================================
// Loop controller
//=============================================================
  always@(posedge clk)
  begin
    if (reset)
      ld_loop_id_counter <= 'b0;
    else begin
      if (mws_ld_loop_iter_v)
        ld_loop_id_counter <= ld_loop_id_counter + 1'b1;
      else if (tag_req && tag_ready)
        ld_loop_id_counter <= 'b0;
    end
  end

  always @(posedge clk)
  begin
    if (reset)
      ld_iter_v_q <= 1'b0;
    else begin
      if (cfg_loop_iter_v && cfg_loop_iter_loop_id == 1 + MEM_ID)
        ld_iter_v_q <= 1'b1;
      else if (cfg_loop_iter_v || ld_stride_v)
        ld_iter_v_q <= 1'b0;
    end
  end


    assign mws_ld_start = ldmem_state_q == LDMEM_BUSY;
    assign mws_ld_loop_iter_v = ld_stride_v && ld_iter_v_q;
    assign mws_ld_loop_iter = iter_q;
    assign mws_ld_loop_iter_loop_id = ld_loop_id_counter;

  always @(posedge clk)
  begin
    if (reset) begin
      iter_q <= 'b0;
    end
    else if (cfg_loop_iter_v && cfg_loop_iter_loop_id == 1 + MEM_ID) begin
      iter_q <= cfg_loop_iter;
    end
  end

  controller_fsm #(
    .LOOP_ID_W                      ( LOOP_ID_W                      ),
    .LOOP_ITER_W                    ( LOOP_ITER_W                    ),
    .IMEM_ADDR_W                    ( LOOP_ID_W                      )
  ) mws_ld_ctrl (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .stall                          ( mws_ld_stall                   ), //input
    .cfg_loop_iter_v                ( mws_ld_loop_iter_v             ), //input
    .cfg_loop_iter                  ( mws_ld_loop_iter               ), //input
    .cfg_loop_iter_loop_id          ( mws_ld_loop_iter_loop_id       ), //input
    .start                          ( mws_ld_start                   ), //input
    .done                           ( mws_ld_done                    ), //output
    .loop_init                      ( mws_ld_init                    ), //output
    .loop_enter                     ( mws_ld_enter                   ), //output  
    .loop_last_iter                 (                                ), //output
    .loop_exit                      ( mws_ld_exit                    ), //output
    .loop_index                     ( mws_ld_index                   ), //output
    .loop_index_valid               ( mws_ld_index_valid             )  //output
  );
//=============================================================

//==============================================================================
// Memory Request generation
//==============================================================================
    assign ld_mem_req_v = cfg_mem_req_v && cfg_mem_req_loop_id == (1 + MEM_ID) && cfg_mem_req_type == MEM_LD && cfg_mem_req_id == MEM_ID;
  always @(posedge clk)
  begin
    if (reset) begin
      ld_req_size <= 'b0;
//      ld_req_loop_id <= 'b0;
    end
    else if (ld_mem_req_v) begin
      ld_req_size <= cfg_mem_req_size;
//      ld_req_loop_id <= ld_loop_id_counter;
    end
  end

  
  // assign ld_req_valid_d = (ld_req_loop_id == mws_ld_index) && (mws_ld_enter || mws_ld_step);
    // assign ld_req_valid_d = (ld_req_loop_id == mws_ld_index) && ld_addr_v;
    assign ld_req_valid_d = ld_addr_v;

  always @(posedge clk)
  begin
    if (reset) begin
      ld_req_valid_q <= 1'b0;
      ld_req_addr <= 'b0;
    end
    else begin
      ld_req_valid_q <= ld_req_valid_d;
      ld_req_addr <= ld_addr;
    end
  end

  //always @(posedge clk)
  //begin
  //  if (tag_req && tag_ready) begin
  //    tag_ld_addr[tag] <= tag_base_ld_addr;
  //  end
  //end

  ram #(
    .ADDR_WIDTH                     ( $clog2(NUM_TAGS)),
    .DATA_WIDTH                     ( ADDR_WIDTH)
  ) u_ram_tag_ld_2(
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .s_write_addr                   ( tag),
    .s_write_req                    ( tag_req && tag_ready),
    .s_write_data                   ( tag_base_ld_addr),
    .s_read_addr                    ( ldmem_tag),
    .s_read_req                     ( 1'b1),
    .s_read_data                    ( mws_ld_base_addr)
  );

  // wire [ 31                      : 0 ]        tag0_ld_addr;
  // wire [ 31                      : 0 ]        tag1_ld_addr;
  // wire [ 31                      : 0 ]        tag0_st_addr;
  // wire [ 31                      : 0 ]        tag1_st_addr;
  // assign tag0_ld_addr = tag_ld_addr[0];
  // assign tag1_ld_addr = tag_ld_addr[1];
//==============================================================================

//==============================================================================
// Tag-based synchronization for double buffering
//==============================================================================
    assign raw_stmem_tag = 0;

  always @(*)
  begin
    ldmem_state_d = ldmem_state_q;
    case(ldmem_state_q)
      LDMEM_IDLE: begin
        if (ldmem_tag_ready) begin
            ldmem_state_d = LDMEM_BUSY;
        end
      end
      LDMEM_BUSY: begin
        if (mws_ld_done)
          ldmem_state_d = LDMEM_WAIT_0;
      end
      LDMEM_WAIT_0: begin
        ldmem_state_d = LDMEM_WAIT_1;
      end
      LDMEM_WAIT_1: begin
        ldmem_state_d = LDMEM_WAIT_2;
      end
      LDMEM_WAIT_2: begin
        ldmem_state_d = LDMEM_WAIT_3;
      end
      LDMEM_WAIT_3: begin
        if (axi_rd_done)
          ldmem_state_d = LDMEM_DONE;
      end
      LDMEM_DONE: begin
        ldmem_state_d = LDMEM_IDLE;
      end
    endcase
  end

  always @(posedge clk)
  begin
    if (reset)
      ldmem_state_q <= LDMEM_IDLE;
    else
      ldmem_state_q <= ldmem_state_d;
  end

  always @(*)
  begin
    stmem_state_d = stmem_state_q;
    case(stmem_state_q)
      STMEM_IDLE: begin
        if (stmem_tag_ready) begin
          stmem_state_d = STMEM_DONE;
        end
      end
      STMEM_DONE: begin
        stmem_state_d = STMEM_IDLE;
      end
    endcase
  end

  always @(posedge clk)
  begin
    if (reset)
      stmem_state_q <= STMEM_IDLE;
    else
      stmem_state_q <= stmem_state_d;
  end

    assign compute_tag_done = compute_done;
    assign compute_ready = compute_tag_ready;

    assign ldmem_tag_done = ldmem_state_q == LDMEM_DONE;
    assign ldmem_ready = ldmem_tag_ready;
  // assign ldmem_tag_done = mws_ld_done;

    assign stmem_tag_done = stmem_state_q == STMEM_DONE;

  tag_sync  #(
    .NUM_TAGS                       ( NUM_TAGS                       )
  )
  mws_tag (
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .block_done                     ( block_done                     ),
    .tag_req                        ( tag_req                        ),
    .tag_reuse                      ( tag_reuse                      ),
    .tag_bias_prev_sw               ( tag_bias_prev_sw               ),
    .tag_ddr_pe_sw                  ( tag_ddr_pe_sw                  ), //input
    .tag_ready                      ( tag_ready                      ),
    .tag                            ( tag                            ),
    .tag_done                       ( tag_done                       ),
    .raw_stmem_tag                  ( raw_stmem_tag                  ),
    .raw_stmem_tag_ready            ( raw_stmem_tag_ready            ),
    .compute_tag_done               ( compute_tag_done               ),
    .compute_tag_ready              ( compute_tag_ready              ),
    .compute_bias_prev_sw           ( compute_bias_prev_sw           ),
    .compute_tag                    ( compute_tag                    ),
    .ldmem_tag_done                 ( ldmem_tag_done                 ),
    .ldmem_tag_ready                ( ldmem_tag_ready                ),
    .ldmem_tag                      ( ldmem_tag                      ),
    .stmem_ddr_pe_sw                ( stmem_ddr_pe_sw                ),
    .stmem_tag_done                 ( stmem_tag_done                 ),
    .stmem_tag_ready                ( stmem_tag_ready                ),
    .stmem_tag                      ( stmem_tag                      )
  );
//==============================================================================


//==============================================================================
// AXI4 Memory Mapped interface
//==============================================================================
    assign mem_write_ready = 1'b1;
    assign mem_read_ready = 1'b0;
    assign axi_rd_req_id = 0;
    assign mem_read_data = 0;
  axi_master #(
    .TX_SIZE_WIDTH                  ( MEM_REQ_W                      ),
    .AXI_DATA_WIDTH                 ( AXI_DATA_WIDTH                 ),
    .AXI_ADDR_WIDTH                 ( AXI_ADDR_WIDTH                 ),
    .AXI_BURST_WIDTH                ( AXI_BURST_WIDTH                )
  ) u_axi_mm_master (
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .m_axi_awaddr                   ( mws_awaddr                     ),
    .m_axi_awlen                    ( mws_awlen                      ),
    .m_axi_awsize                   ( mws_awsize                     ),
    .m_axi_awburst                  ( mws_awburst                    ),
    .m_axi_awvalid                  ( mws_awvalid                    ),
    .m_axi_awready                  ( mws_awready                    ),
    .m_axi_wdata                    ( mws_wdata                      ),
    .m_axi_wstrb                    ( mws_wstrb                      ),
    .m_axi_wlast                    ( mws_wlast                      ),
    .m_axi_wvalid                   ( mws_wvalid                     ),
    .m_axi_wready                   ( mws_wready                     ),
    .m_axi_bresp                    ( mws_bresp                      ),
    .m_axi_bvalid                   ( mws_bvalid                     ),
    .m_axi_bready                   ( mws_bready                     ),
    .m_axi_araddr                   ( mws_araddr                     ),
    .m_axi_arid                     ( mws_arid                       ),
    .m_axi_arlen                    ( mws_arlen                      ),
    .m_axi_arsize                   ( mws_arsize                     ),
    .m_axi_arburst                  ( mws_arburst                    ),
    .m_axi_arvalid                  ( mws_arvalid                    ),
    .m_axi_arready                  ( mws_arready                    ),
    .m_axi_rdata                    ( mws_rdata                      ),
    .m_axi_rid                      ( mws_rid                        ),
    .m_axi_rresp                    ( mws_rresp                      ),
    .m_axi_rlast                    ( mws_rlast                      ),
    .m_axi_rvalid                   ( mws_rvalid                     ),
    .m_axi_rready                   ( mws_rready                     ),
    // Buffer
    .mem_write_id                   ( mem_write_id                   ),
    .mem_write_req                  ( mem_write_req                  ),
    .mem_write_data                 ( mem_write_data                 ),
    .mem_write_ready                ( mem_write_ready                ),
    .mem_read_data                  ( mem_read_data                  ),
    .mem_read_req                   ( mem_read_req                   ),
    .mem_read_ready                 ( mem_read_ready                 ),
    // AXI RD Req
    .rd_req_id                      ( axi_rd_req_id                  ),
    .rd_req                         ( axi_rd_req                     ),
    .rd_done                        ( axi_rd_done                    ),
    .rd_ready                       ( axi_rd_ready                   ),
    .rd_req_size                    ( axi_rd_req_size                ),
    .rd_addr                        ( axi_rd_addr                    ),
    // AXI WR Req
    .wr_req                         ( axi_wr_req                     ),
    .wr_req_id                      ( axi_wr_req_id                  ),
    .wr_ready                       ( axi_wr_ready                   ),
    .wr_req_size                    ( axi_wr_req_size                ),
    .wr_addr                        ( axi_wr_addr                    ),
    .wr_done                        ( axi_wr_done                    )
  );
//==============================================================================

`ifdef COCOTB_SIM
  integer req_count;
  always @(posedge clk)
  begin
    if (reset) req_count <= 0;
    else req_count = req_count + (tag_req && tag_ready);
  end
`endif //COCOTB_SIM
//==============================================================================
// Dual-port RAM
//==============================================================================
  always @(posedge clk)
  begin
    if (reset)
      mem_write_addr <= 0;
    else begin
      if (mem_write_req)
        mem_write_addr <= mem_write_addr + 1'b1;
      else if (ldmem_state_q == LDMEM_DONE)
        mem_write_addr <= 0;
    end
  end

    assign tag_mem_write_addr = {ldmem_tag, mem_write_addr};
    assign tag_buf_read_addr = {compute_tag, buf_read_addr};

  register_sync #(BUF_DATA_WIDTH)
  buf_read_data_delay (clk, reset, _buf_read_data, buf_read_data);

  ibuf #(
    .TAG_W                          ( TAG_W                          ),
    .BUF_ADDR_WIDTH                 ( TAG_BUF_ADDR_W                 ),
    .ARRAY_N                        ( ARRAY_N                        ),
    .MEM_DATA_WIDTH                 ( AXI_DATA_WIDTH                 ),
    .DATA_WIDTH                     ( DATA_WIDTH                     )
  ) buf_ram (
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .mem_write_addr                 ( tag_mem_write_addr             ),
    .mem_write_req                  ( mem_write_req                  ),
    .mem_write_data                 ( mem_write_data                 ),
    .buf_read_addr                  ( tag_buf_read_addr              ),
    .buf_read_req                   ( buf_read_req                   ),
    .buf_read_data                  ( _buf_read_data                 )
  );
//==============================================================================


//==============================================================================

`ifdef COCOTB_SIM
  integer wr_req_count=0;
  always @(posedge clk)
    if (reset)
      wr_req_count <= 0;
    else
      wr_req_count <= wr_req_count + axi_wr_req;

  integer rd_req_count=0;
  integer missed_rd_req_count=0;
  always @(posedge clk)
    if (reset)
      rd_req_count <= 0;
    else
      rd_req_count <= rd_req_count + axi_rd_req;
  always @(posedge clk)
    if (reset)
      missed_rd_req_count <= 0;
    else
      missed_rd_req_count <= missed_rd_req_count + (axi_rd_req && ~axi_rd_ready);
`endif


//=============================================================
// VCD
//=============================================================
`ifdef COCOTB_TOPLEVEL_mem_wrapper
initial begin
  $dumpfile("mem_wrapper.vcd");
  $dumpvars(0, mem_wrapper);
end
`endif
//=============================================================
endmodule
//
// Wrapper for memory
//
// Hardik Sharma
// (hsharma@gatech.edu)

`timescale 1ns/1ps
module wbuf_mem_wrapper #(
  // Internal Parameters
    parameter integer  MEM_ID                       = 0,
    parameter integer  STORE_ENABLED                = MEM_ID == 1 ? 1 : 0,
    parameter integer  MEM_REQ_W                    = 16,
    parameter integer  ADDR_WIDTH                   = 8,
    parameter integer  DATA_WIDTH                   = 32,
    parameter integer  LOOP_ITER_W                  = 16,
    parameter integer  ADDR_STRIDE_W                = 32,
    parameter integer  LOOP_ID_W                    = 5,
    parameter integer  BUF_TYPE_W                   = 2,
    parameter integer  NUM_TAGS                     = 4,
    parameter integer  TAG_W                        = $clog2(NUM_TAGS),

  // AXI
    parameter integer  AXI_ADDR_WIDTH               = 42,
    parameter integer  AXI_ID_WIDTH                 = 1,
    parameter integer  AXI_DATA_WIDTH               = 64,
    parameter integer  AXI_BURST_WIDTH              = 8,
    parameter integer  WSTRB_W                      = AXI_DATA_WIDTH/8,

  // Buffer
    parameter integer  ARRAY_N                      = 2,
    parameter integer  ARRAY_M                      = MEM_ID == 2 ? ARRAY_N : 1,
    parameter integer  BUF_DATA_WIDTH               = DATA_WIDTH * ARRAY_N * ARRAY_M,
    parameter integer  BUF_ADDR_W                   = 16,
    parameter integer  MEM_ADDR_W                   = BUF_ADDR_W + $clog2(BUF_DATA_WIDTH / AXI_DATA_WIDTH),
    parameter integer  TAG_BUF_ADDR_W               = BUF_ADDR_W + TAG_W,
    parameter integer  TAG_MEM_ADDR_W               = MEM_ADDR_W + TAG_W
) (
    input  wire                                         clk,
    input  wire                                         reset,

    input  wire                                         tag_req,
    input  wire                                         tag_reuse,
    input  wire                                         tag_bias_prev_sw,
    input  wire                                         tag_ddr_pe_sw,
    output wire                                         tag_ready,
    output wire                                         tag_done,
    input  wire                                         compute_done,
    input  wire                                         block_done,
    input  wire  [ ADDR_WIDTH           -1 : 0 ]        tag_base_ld_addr,

    output wire                                         compute_ready,
    output wire                                         compute_bias_prev_sw,

  // Programming
    input  wire                                         cfg_loop_stride_v,
    input  wire  [ ADDR_STRIDE_W        -1 : 0 ]        cfg_loop_stride,
    input  wire  [ LOOP_ID_W            -1 : 0 ]        cfg_loop_stride_loop_id,
    input  wire  [ BUF_TYPE_W           -1 : 0 ]        cfg_loop_stride_id,
    input  wire  [ 2                    -1 : 0 ]        cfg_loop_stride_type,

    input  wire                                         cfg_loop_iter_v,
    input  wire  [ LOOP_ITER_W          -1 : 0 ]        cfg_loop_iter,
    input  wire  [ LOOP_ID_W            -1 : 0 ]        cfg_loop_iter_loop_id,

    input  wire                                         cfg_mem_req_v,
    input  wire  [ BUF_TYPE_W           -1 : 0 ]        cfg_mem_req_id,
    input  wire  [ MEM_REQ_W            -1 : 0 ]        cfg_mem_req_size,
    input  wire  [ LOOP_ID_W            -1 : 0 ]        cfg_mem_req_loop_id,
    input  wire  [ 2                    -1 : 0 ]        cfg_mem_req_type,

  // Systolic Array
    output wire  [ BUF_DATA_WIDTH       -1 : 0 ]        buf_read_data,
    input  wire                                         buf_read_req,
    input  wire  [ BUF_ADDR_W           -1 : 0 ]        buf_read_addr,

  // CL_wrapper -> DDR AXI4 interface
    // Master Interface Write Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        mws_awaddr,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        mws_awlen,
    output wire  [ 3                    -1 : 0 ]        mws_awsize,
    output wire  [ 2                    -1 : 0 ]        mws_awburst,
    output wire                                         mws_awvalid,
    input  wire                                         mws_awready,
    // Master Interface Write Data
    output wire  [ AXI_DATA_WIDTH       -1 : 0 ]        mws_wdata,
    output wire  [ WSTRB_W              -1 : 0 ]        mws_wstrb,
    output wire                                         mws_wlast,
    output wire                                         mws_wvalid,
    input  wire                                         mws_wready,
    // Master Interface Write Response
    input  wire  [ 2                    -1 : 0 ]        mws_bresp,
    input  wire                                         mws_bvalid,
    output wire                                         mws_bready,
    // Master Interface Read Address
    output wire  [ AXI_ADDR_WIDTH       -1 : 0 ]        mws_araddr,
    output wire  [ AXI_ID_WIDTH         -1 : 0 ]        mws_arid,
    output wire  [ AXI_BURST_WIDTH      -1 : 0 ]        mws_arlen,
    output wire  [ 3                    -1 : 0 ]        mws_arsize,
    output wire  [ 2                    -1 : 0 ]        mws_arburst,
    output wire                                         mws_arvalid,
    input  wire                                         mws_arready,
    // Master Interface Read Data
    input  wire  [ AXI_DATA_WIDTH       -1 : 0 ]        mws_rdata,
    input  wire  [ AXI_ID_WIDTH         -1 : 0 ]        mws_rid,
    input  wire  [ 2                    -1 : 0 ]        mws_rresp,
    input  wire                                         mws_rlast,
    input  wire                                         mws_rvalid,
    output wire                                         mws_rready
);

//==============================================================================
// Localparams
//==============================================================================
    localparam integer  LDMEM_IDLE                   = 0;
    localparam integer  LDMEM_CHECK_RAW              = 1;
    localparam integer  LDMEM_BUSY                   = 2;
    localparam integer  LDMEM_WAIT_0                 = 3;
    localparam integer  LDMEM_WAIT_1                 = 4;
    localparam integer  LDMEM_WAIT_2                 = 5;
    localparam integer  LDMEM_WAIT_3                 = 6;
    localparam integer  LDMEM_DONE                   = 7;

    localparam integer  STMEM_IDLE                   = 0;
    localparam integer  STMEM_DDR                    = 1;
    localparam integer  STMEM_WAIT_0                 = 2;
    localparam integer  STMEM_WAIT_1                 = 3;
    localparam integer  STMEM_WAIT_2                 = 4;
    localparam integer  STMEM_WAIT_3                 = 5;
    localparam integer  STMEM_DONE                   = 6;
    localparam integer  STMEM_PU                     = 7;

    localparam integer  MEM_LD                       = 0;
    localparam integer  MEM_ST                       = 1;
    localparam integer  MEM_RD                       = 2;
    localparam integer  MEM_WR                       = 3;
//==============================================================================

//==============================================================================
// Wires/Regs
//==============================================================================
    wire                                        compute_tag_done;
    wire                                        compute_tag_reuse;
    wire                                        compute_tag_ready;
    wire [ TAG_W                -1 : 0 ]        compute_tag;
    wire [ TAG_W                -1 : 0 ]        compute_tag_delayed;
    wire                                        ldmem_tag_done;
    wire                                        ldmem_tag_ready;
    wire [ TAG_W                -1 : 0 ]        ldmem_tag;
    wire                                        stmem_tag_done;
    wire                                        stmem_tag_ready;
    wire [ TAG_W                -1 : 0 ]        stmem_tag;
    wire                                        stmem_ddr_pe_sw;

    reg  [ 4                    -1 : 0 ]        ldmem_state_d;
    reg  [ 4                    -1 : 0 ]        ldmem_state_q;

    reg  [ 3                    -1 : 0 ]        stmem_state_d;
    reg  [ 3                    -1 : 0 ]        stmem_state_q;

    wire                                        ld_mem_req_v;
    wire                                        st_mem_req_v;

    wire [ TAG_W                -1 : 0 ]        tag;


    reg                                         ld_iter_v_q;
    reg  [ LOOP_ITER_W          -1 : 0 ]        iter_q;

    reg  [ LOOP_ID_W            -1 : 0 ]        ld_loop_id_counter;

    wire [ LOOP_ID_W            -1 : 0 ]        mws_ld_loop_iter_loop_id;
    wire [ LOOP_ITER_W          -1 : 0 ]        mws_ld_loop_iter;
    wire                                        mws_ld_loop_iter_v;
    wire                                        mws_ld_start;
    wire                                        mws_ld_done;
    wire                                        mws_ld_stall;
    wire                                        mws_ld_init;
    wire                                        mws_ld_enter;
    wire                                        mws_ld_exit;
    wire [ LOOP_ID_W            -1 : 0 ]        mws_ld_index;
    wire                                        mws_ld_index_valid;
    wire                                        mws_ld_step;

    wire                                        ld_stride_v;
    wire [ ADDR_STRIDE_W        -1 : 0 ]        ld_stride;
    wire [ BUF_TYPE_W           -1 : 0 ]        ld_stride_id;

    wire [ ADDR_WIDTH           -1 : 0 ]        ld_addr;
    wire [ ADDR_WIDTH           -1 : 0 ]        mws_ld_base_addr;
    wire                                        ld_addr_v;

    reg  [ MEM_REQ_W            -1 : 0 ]        ld_req_size;

    wire                                        ld_req_valid_d;

    reg                                         ld_req_valid_q;

    //reg  [ ADDR_WIDTH           -1 : 0 ]        tag_ld_addr[0:NUM_TAGS-1];

    reg  [ ADDR_WIDTH           -1 : 0 ]        ld_req_addr;

    wire                                        axi_rd_req;
    wire [ AXI_ID_WIDTH         -1 : 0 ]        axi_rd_req_id;
    wire                                        axi_rd_done;
    wire [ MEM_REQ_W            -1 : 0 ]        axi_rd_req_size;
    wire                                        axi_rd_ready;
    wire [ AXI_ADDR_WIDTH       -1 : 0 ]        axi_rd_addr;

    wire                                        axi_wr_req;
    wire [ AXI_ID_WIDTH         -1 : 0 ]        axi_wr_req_id;
    wire                                        axi_wr_done;
    wire [ MEM_REQ_W            -1 : 0 ]        axi_wr_req_size;
    wire                                        axi_wr_ready;
    wire [ AXI_ADDR_WIDTH       -1 : 0 ]        axi_wr_addr;

    wire [ AXI_ID_WIDTH         -1 : 0 ]        mem_write_id;
    wire                                        mem_write_req;
    wire [ AXI_DATA_WIDTH       -1 : 0 ]        mem_write_data;
    reg  [ MEM_ADDR_W           -1 : 0 ]        mem_write_addr;
    wire                                        mem_write_ready;
    // Mem reads disabled
    wire [ AXI_DATA_WIDTH       -1 : 0 ]        mem_read_data;
    wire                                        mem_read_req;
    wire                                        mem_read_ready;

  // Adding register to buf read data
    wire [ BUF_DATA_WIDTH       -1 : 0 ]        _buf_read_data;
//==============================================================================

//==============================================================================
// Assigns
//==============================================================================
    assign ld_stride = cfg_loop_stride;
    assign ld_stride_v = cfg_loop_stride_v && cfg_loop_stride_loop_id == 1 + MEM_ID && cfg_loop_stride_type == MEM_LD && cfg_loop_stride_id == MEM_ID;

    //assign mws_ld_base_addr = tag_ld_addr[ldmem_tag];
  
    assign axi_rd_req = ld_req_valid_q;
    assign axi_rd_req_size = ld_req_size * (ARRAY_N * ARRAY_M * DATA_WIDTH) / AXI_DATA_WIDTH;
    assign axi_rd_addr = ld_req_addr;

    assign axi_wr_req = 1'b0;
    assign axi_wr_req_id = 1'b0;
    assign axi_wr_req_size = 0;
    assign axi_wr_addr = 0;
//==============================================================================

//==============================================================================
// Address generators
//==============================================================================
    assign mws_ld_stall = ~ldmem_tag_ready || ~axi_rd_ready;
    assign mws_ld_step = mws_ld_index_valid && !mws_ld_stall;
  mem_walker_stride #(
    .ADDR_WIDTH                     ( ADDR_WIDTH                     ),
    .ADDR_STRIDE_W                  ( ADDR_STRIDE_W                  ),
    .LOOP_ID_W                      ( LOOP_ID_W                      )
  ) mws_ld (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .base_addr                      ( mws_ld_base_addr               ), //input
    .loop_ctrl_done                 ( mws_ld_done                    ), //input
    .loop_index                     ( mws_ld_index                   ), //input
    .loop_index_valid               ( mws_ld_step                    ), //input
    .loop_init                      ( mws_ld_init                    ), //input
    .loop_enter                     ( mws_ld_enter                   ), //input
    .loop_exit                      ( mws_ld_exit                    ), //input
    .cfg_addr_stride_v              ( ld_stride_v                    ), //input
    .cfg_addr_stride                ( ld_stride                      ), //input
    .addr_out                       ( ld_addr                        ), //output
    .addr_out_valid                 ( ld_addr_v                      )  //output
  );
//==============================================================================

//=============================================================
// Loop controller
//=============================================================
  always@(posedge clk)
  begin
    if (reset)
      ld_loop_id_counter <= 'b0;
    else begin
      if (mws_ld_loop_iter_v)
        ld_loop_id_counter <= ld_loop_id_counter + 1'b1;
      else if (tag_req && tag_ready)
        ld_loop_id_counter <= 'b0;
    end
  end

  always @(posedge clk)
  begin
    if (reset)
      ld_iter_v_q <= 1'b0;
    else begin
      if (cfg_loop_iter_v && cfg_loop_iter_loop_id == 1 + MEM_ID)
        ld_iter_v_q <= 1'b1;
      else if (cfg_loop_iter_v || ld_stride_v)
        ld_iter_v_q <= 1'b0;
    end
  end


    assign mws_ld_start = ldmem_state_q == LDMEM_BUSY;
    assign mws_ld_loop_iter_v = ld_stride_v && ld_iter_v_q;
    assign mws_ld_loop_iter = iter_q;
    assign mws_ld_loop_iter_loop_id = ld_loop_id_counter;

  always @(posedge clk)
  begin
    if (reset) begin
      iter_q <= 'b0;
    end
    else if (cfg_loop_iter_v && cfg_loop_iter_loop_id == 1 + MEM_ID) begin
      iter_q <= cfg_loop_iter;
    end
  end

  controller_fsm #(
    .LOOP_ID_W                      ( LOOP_ID_W                      ),
    .LOOP_ITER_W                    ( LOOP_ITER_W                    ),
    .IMEM_ADDR_W                    ( LOOP_ID_W                      )
  ) mws_ld_ctrl (
    .clk                            ( clk                            ), //input
    .reset                          ( reset                          ), //input
    .stall                          ( mws_ld_stall                   ), //input
    .cfg_loop_iter_v                ( mws_ld_loop_iter_v             ), //input
    .cfg_loop_iter                  ( mws_ld_loop_iter               ), //input
    .cfg_loop_iter_loop_id          ( mws_ld_loop_iter_loop_id       ), //input
    .start                          ( mws_ld_start                   ), //input
    .done                           ( mws_ld_done                    ), //output
    .loop_init                      ( mws_ld_init                    ), //output
    .loop_enter                     ( mws_ld_enter                   ), //output  
    .loop_last_iter                 (                                ), //output
    .loop_exit                      ( mws_ld_exit                    ), //output
    .loop_index                     ( mws_ld_index                   ), //output
    .loop_index_valid               ( mws_ld_index_valid             )  //output
  );
//=============================================================

//==============================================================================
// Memory Request generation
//==============================================================================
    assign ld_mem_req_v = cfg_mem_req_v && cfg_mem_req_loop_id == (1 + MEM_ID) && cfg_mem_req_type == MEM_LD && cfg_mem_req_id == MEM_ID;
  always @(posedge clk)
  begin
    if (reset) begin
      ld_req_size <= 'b0;
      // ld_req_loop_id <= 'b0;
    end
    else if (ld_mem_req_v) begin
      ld_req_size <= cfg_mem_req_size;
      // ld_req_loop_id <= ld_loop_id_counter;
    end
  end

  // assign ld_req_valid_d = (ld_req_loop_id == mws_ld_index) && (mws_ld_enter || mws_ld_step);
    // assign ld_req_valid_d = (ld_req_loop_id == mws_ld_index) && ld_addr_v;
    assign ld_req_valid_d = ld_addr_v;

  always @(posedge clk)
  begin
    if (reset) begin
      ld_req_valid_q <= 1'b0;
      ld_req_addr <= 'b0;
    end
    else begin
      ld_req_valid_q <= ld_req_valid_d;
      ld_req_addr <= ld_addr;
    end
  end

  //always @(posedge clk)
  //begin
  //  if (tag_req && tag_ready) begin
  //    tag_ld_addr[tag] <= tag_base_ld_addr;
  //  end
  //end

  ram #(
    .ADDR_WIDTH                     ( $clog2(NUM_TAGS)),
    .DATA_WIDTH                     ( ADDR_WIDTH)
  ) u_ram_tag_ld_3(
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .s_write_addr                   ( tag),
    .s_write_req                    ( tag_req && tag_ready),
    .s_write_data                   ( tag_base_ld_addr),
    .s_read_addr                    ( ldmem_tag),
    .s_read_req                     ( 1'b1),
    .s_read_data                    ( mws_ld_base_addr)
  );

  // wire [ 31                      : 0 ]        tag0_ld_addr;
  // wire [ 31                      : 0 ]        tag1_ld_addr;
  // wire [ 31                      : 0 ]        tag0_st_addr;
  // wire [ 31                      : 0 ]        tag1_st_addr;
  // assign tag0_ld_addr = tag_ld_addr[0];
  // assign tag1_ld_addr = tag_ld_addr[1];
  // assign tag0_st_addr = tag_st_addr[0];
  // assign tag1_st_addr = tag_st_addr[1];
//==============================================================================

//==============================================================================
// Tag-based synchronization for double buffering
//==============================================================================
    wire [ TAG_W                -1 : 0 ]        raw_stmem_tag;
    wire                                        raw_stmem_tag_ready;
    assign raw_stmem_tag = 0;

  always @(*)
  begin
    ldmem_state_d = ldmem_state_q;
    case(ldmem_state_q)
      LDMEM_IDLE: begin
        if (ldmem_tag_ready) begin
          ldmem_state_d = LDMEM_BUSY;
        end
      end
      LDMEM_BUSY: begin
        if (mws_ld_done)
          ldmem_state_d = LDMEM_WAIT_0;
      end
      LDMEM_WAIT_0: begin
        ldmem_state_d = LDMEM_WAIT_1;
      end
      LDMEM_WAIT_1: begin
        ldmem_state_d = LDMEM_WAIT_2;
      end
      LDMEM_WAIT_2: begin
        ldmem_state_d = LDMEM_WAIT_3;
      end
      LDMEM_WAIT_3: begin
        if (axi_rd_done)
          ldmem_state_d = LDMEM_DONE;
      end
      LDMEM_DONE: begin
        ldmem_state_d = LDMEM_IDLE;
      end
    endcase
  end

  always @(posedge clk)
  begin
    if (reset)
      ldmem_state_q <= LDMEM_IDLE;
    else
      ldmem_state_q <= ldmem_state_d;
  end


  wire pu_done = 1'b1;

  always @(*)
  begin
    stmem_state_d = stmem_state_q;
    case(stmem_state_q)
      STMEM_IDLE: begin
        if (stmem_tag_ready) begin
            stmem_state_d = STMEM_DONE;
        end
      end
      STMEM_DONE: begin
        stmem_state_d = STMEM_IDLE;
      end
    endcase
  end

  always @(posedge clk)
  begin
    if (reset)
      stmem_state_q <= STMEM_IDLE;
    else
      stmem_state_q <= stmem_state_d;
  end


    wire                                        ldmem_ready;

    assign compute_tag_done = compute_done;
    assign compute_ready = compute_tag_ready;

    assign ldmem_tag_done = ldmem_state_q == LDMEM_DONE;
    assign ldmem_ready = ldmem_tag_ready;
  // assign ldmem_tag_done = mws_ld_done;

    assign stmem_tag_done = stmem_state_q == STMEM_DONE;

  tag_sync  #(
    .NUM_TAGS                       ( NUM_TAGS                       )
  )
  mws_tag (
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .block_done                     ( block_done                     ),
    .tag_req                        ( tag_req                        ),
    .tag_reuse                      ( tag_reuse                      ),
    .tag_bias_prev_sw               ( tag_bias_prev_sw               ),
    .tag_ddr_pe_sw                  ( tag_ddr_pe_sw                  ), //input
    .tag_ready                      ( tag_ready                      ),
    .tag                            ( tag                            ),
    .tag_done                       ( tag_done                       ),
    .raw_stmem_tag                  ( raw_stmem_tag                  ),
    .raw_stmem_tag_ready            ( raw_stmem_tag_ready            ),
    .compute_tag_done               ( compute_tag_done               ),
    .compute_tag_ready              ( compute_tag_ready              ),
    .compute_bias_prev_sw           ( compute_bias_prev_sw           ),
    .compute_tag                    ( compute_tag                    ),
    .ldmem_tag_done                 ( ldmem_tag_done                 ),
    .ldmem_tag_ready                ( ldmem_tag_ready                ),
    .ldmem_tag                      ( ldmem_tag                      ),
    .stmem_ddr_pe_sw                ( stmem_ddr_pe_sw                ),
    .stmem_tag_done                 ( stmem_tag_done                 ),
    .stmem_tag_ready                ( stmem_tag_ready                ),
    .stmem_tag                      ( stmem_tag                      )
  );
//==============================================================================


//==============================================================================
// AXI4 Memory Mapped interface
//==============================================================================
    assign mem_write_ready = 1'b1;
    assign mem_read_ready = 1'b1;
    assign axi_rd_req_id = 1'b0;
    assign mem_read_data = 0;
  axi_master #(
    .TX_SIZE_WIDTH                  ( MEM_REQ_W                      ),
    .AXI_DATA_WIDTH                 ( AXI_DATA_WIDTH                 ),
    .AXI_ADDR_WIDTH                 ( AXI_ADDR_WIDTH                 ),
    .AXI_BURST_WIDTH                ( AXI_BURST_WIDTH                )
  ) u_axi_mm_master (
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .m_axi_awaddr                   ( mws_awaddr                     ),
    .m_axi_awlen                    ( mws_awlen                      ),
    .m_axi_awsize                   ( mws_awsize                     ),
    .m_axi_awburst                  ( mws_awburst                    ),
    .m_axi_awvalid                  ( mws_awvalid                    ),
    .m_axi_awready                  ( mws_awready                    ),
    .m_axi_wdata                    ( mws_wdata                      ),
    .m_axi_wstrb                    ( mws_wstrb                      ),
    .m_axi_wlast                    ( mws_wlast                      ),
    .m_axi_wvalid                   ( mws_wvalid                     ),
    .m_axi_wready                   ( mws_wready                     ),
    .m_axi_bresp                    ( mws_bresp                      ),
    .m_axi_bvalid                   ( mws_bvalid                     ),
    .m_axi_bready                   ( mws_bready                     ),
    .m_axi_araddr                   ( mws_araddr                     ),
    .m_axi_arid                     ( mws_arid                       ),
    .m_axi_arlen                    ( mws_arlen                      ),
    .m_axi_arsize                   ( mws_arsize                     ),
    .m_axi_arburst                  ( mws_arburst                    ),
    .m_axi_arvalid                  ( mws_arvalid                    ),
    .m_axi_arready                  ( mws_arready                    ),
    .m_axi_rdata                    ( mws_rdata                      ),
    .m_axi_rid                      ( mws_rid                        ),
    .m_axi_rresp                    ( mws_rresp                      ),
    .m_axi_rlast                    ( mws_rlast                      ),
    .m_axi_rvalid                   ( mws_rvalid                     ),
    .m_axi_rready                   ( mws_rready                     ),
    // Buffer
    .mem_write_id                   ( mem_write_id                   ),
    .mem_write_req                  ( mem_write_req                  ),
    .mem_write_data                 ( mem_write_data                 ),
    .mem_write_ready                ( mem_write_ready                ),
    .mem_read_data                  ( mem_read_data                  ),
    .mem_read_req                   ( mem_read_req                   ),
    .mem_read_ready                 ( mem_read_ready                 ),
    // AXI RD Req
    .rd_req_id                      ( axi_rd_req_id                  ),
    .rd_req                         ( axi_rd_req                     ),
    .rd_done                        ( axi_rd_done                    ),
    .rd_ready                       ( axi_rd_ready                   ),
    .rd_req_size                    ( axi_rd_req_size                ),
    .rd_addr                        ( axi_rd_addr                    ),
    // AXI WR Req
    .wr_req                         ( axi_wr_req                     ),
    .wr_req_id                      ( axi_wr_req_id                  ),
    .wr_ready                       ( axi_wr_ready                   ),
    .wr_req_size                    ( axi_wr_req_size                ),
    .wr_addr                        ( axi_wr_addr                    ),
    .wr_done                        ( axi_wr_done                    )
  );
//==============================================================================

//==============================================================================
// Dual-port RAM
//==============================================================================
  always @(posedge clk)
  begin
    if (reset)
      mem_write_addr <= 0;
    else begin
      if (mem_write_req)
        mem_write_addr <= mem_write_addr + 1'b1;
      else if (ldmem_state_q == LDMEM_DONE)
        mem_write_addr <= 0;
    end
  end

    wire [ TAG_MEM_ADDR_W       -1 : 0 ]        tag_mem_read_addr;
    wire [ TAG_MEM_ADDR_W       -1 : 0 ]        tag_mem_write_addr;

    wire [ TAG_BUF_ADDR_W       -1 : 0 ]        tag_buf_read_addr;

    assign tag_mem_write_addr = {ldmem_tag, mem_write_addr};

    assign compute_tag_delayed = compute_tag;

    assign tag_buf_read_addr = {compute_tag_delayed, buf_read_addr};

  register_sync #(BUF_DATA_WIDTH)
  buf_read_data_delay (clk, reset, _buf_read_data, buf_read_data);

  wbuf #(
    .TAG_W                          ( TAG_W                          ),
    .BUF_ADDR_WIDTH                 ( TAG_BUF_ADDR_W                 ),
    .ARRAY_N                        ( ARRAY_N                        ),
    .ARRAY_M                        ( ARRAY_M                        ),
    .MEM_DATA_WIDTH                 ( AXI_DATA_WIDTH                 ),
    .DATA_WIDTH                     ( DATA_WIDTH                     )
  ) buf_ram (
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .mem_write_addr                 ( tag_mem_write_addr             ),
    .mem_write_req                  ( mem_write_req                  ),
    .mem_write_data                 ( mem_write_data                 ),
    .buf_read_addr                  ( tag_buf_read_addr              ),
    .buf_read_req                   ( buf_read_req                   ),
    .buf_read_data                  ( _buf_read_data                 )
  );
//==============================================================================


//==============================================================================

`ifdef COCOTB_SIM
  integer wr_req_count=0;
  integer rd_req_count=0;
  integer missed_rd_req_count=0;
  integer req_count;

  always @(posedge clk)
    if (reset)
      wr_req_count <= 0;
    else
      wr_req_count <= wr_req_count + axi_wr_req;

  always @(posedge clk)
    if (reset)
      rd_req_count <= 0;
    else
      rd_req_count <= rd_req_count + axi_rd_req;

  always @(posedge clk)
    if (reset)
      missed_rd_req_count <= 0;
    else
      missed_rd_req_count <= missed_rd_req_count + (axi_rd_req && ~axi_rd_ready);

  always @(posedge clk)
  begin
    if (reset) req_count <= 0;
    else req_count = req_count + (tag_req && tag_ready);
  end
`endif


//=============================================================
// VCD
//=============================================================
`ifdef COCOTB_TOPLEVEL_mem_wrapper
initial begin
  $dumpfile("mem_wrapper.vcd");
  $dumpvars(0, mem_wrapper);
end
`endif
//=============================================================
endmodule
//
// BBUF - Bias Buffer
//
// Hardik Sharma
// (hsharma@gatech.edu)
`timescale 1ns/1ps
module bbuf #(
  parameter integer  TAG_W                        = 2,  // Log number of banks
  parameter integer  MEM_DATA_WIDTH               = 64,
  parameter integer  ARRAY_M                      = 2,
  parameter integer  DATA_WIDTH                   = 32,
  parameter integer  BUF_ADDR_WIDTH               = 10,

  parameter integer  GROUP_SIZE                   = MEM_DATA_WIDTH / DATA_WIDTH,
  parameter integer  GROUP_ID_W                   = GROUP_SIZE == 1 ? 0 : $clog2(GROUP_SIZE),
  parameter integer  BUF_ID_W                     = $clog2(ARRAY_M) - GROUP_ID_W,

  parameter integer  MEM_ADDR_WIDTH               = BUF_ADDR_WIDTH + BUF_ID_W,
  parameter integer  BUF_DATA_WIDTH               = ARRAY_M * DATA_WIDTH
)
(
  input  wire                                         clk,
  input  wire                                         reset,

  input  wire                                         mem_write_req,
  input  wire  [ MEM_ADDR_WIDTH       -1 : 0 ]        mem_write_addr,
  input  wire  [ MEM_DATA_WIDTH       -1 : 0 ]        mem_write_data,

  input  wire                                         buf_read_req,
  input  wire  [ BUF_ADDR_WIDTH       -1 : 0 ]        buf_read_addr,
  output wire  [ BUF_DATA_WIDTH       -1 : 0 ]        buf_read_data
);

  genvar m;
  generate
    for (m=0; m<ARRAY_M; m=m+1)
    begin: LOOP_N

      localparam integer  LOCAL_ADDR_W                 = BUF_ADDR_WIDTH;
      localparam integer  LOCAL_BUF_ID                 = m/GROUP_SIZE;

      wire                                        local_buf_read_req;
      wire [ LOCAL_ADDR_W         -1 : 0 ]        local_buf_read_addr;
      wire [ DATA_WIDTH           -1 : 0 ]        local_buf_read_data;

      assign buf_read_data[(m)*DATA_WIDTH+:DATA_WIDTH] = local_buf_read_data;

      assign local_buf_read_req = buf_read_req;
      assign local_buf_read_addr = buf_read_addr;

      wire [ BUF_ID_W             -1 : 0 ]        local_mem_write_buf_id;
      wire                                        local_mem_write_req;
      wire [ LOCAL_ADDR_W         -1 : 0 ]        local_mem_write_addr;
      wire [ DATA_WIDTH           -1 : 0 ]        local_mem_write_data;

      wire [ BUF_ID_W             -1 : 0 ]        buf_id;
      assign buf_id = LOCAL_BUF_ID;

      if (BUF_ID_W == 0) begin
        assign local_mem_write_addr = mem_write_addr;
        assign local_mem_write_req = mem_write_req;
        assign local_mem_write_data = mem_write_data[(m%GROUP_SIZE)*DATA_WIDTH+:DATA_WIDTH];
      end
      else begin
        assign {local_mem_write_addr, local_mem_write_buf_id} = mem_write_addr;
        assign local_mem_write_req = mem_write_req && local_mem_write_buf_id == buf_id;
        assign local_mem_write_data = mem_write_data[(m%GROUP_SIZE)*DATA_WIDTH+:DATA_WIDTH];
      end

      ram #(
        .ADDR_WIDTH                     ( LOCAL_ADDR_W                   ),
        .DATA_WIDTH                     ( DATA_WIDTH                     ),
        .OUTPUT_REG                     ( 1                              )
      ) u_ram (
        .clk                            ( clk                            ),
        .reset                          ( reset                          ),
        .s_write_addr                   ( local_mem_write_addr           ),
        .s_write_req                    ( local_mem_write_req            ),
        .s_write_data                   ( local_mem_write_data           ),
        .s_read_addr                    ( local_buf_read_addr            ),
        .s_read_req                     ( local_buf_read_req             ),
        .s_read_data                    ( local_buf_read_data            )
        );


    end
  endgenerate

//=============================================================
// VCD
//=============================================================
  `ifdef COCOTB_TOPLEVEL_buffer
  initial begin
    $dumpfile("buffer.vcd");
    $dumpvars(0, buffer);
  end
  `endif
//=============================================================
endmodule
//
// WBUF
//
// Hardik Sharma
// (hsharma@gatech.edu)
`timescale 1ns/1ps
module wbuf #(
    parameter integer  TAG_W                        = 2,  // Log number of banks
    parameter integer  MEM_DATA_WIDTH               = 64,
    parameter integer  ARRAY_N                      = 64,
    parameter integer  ARRAY_M                      = 64,
    parameter integer  DATA_WIDTH                   = 16,
    parameter integer  BUF_ADDR_WIDTH               = 9,

    parameter integer  GROUP_SIZE                   = (DATA_WIDTH * ARRAY_M) / MEM_DATA_WIDTH,
    parameter integer  NUM_GROUPS                   = MEM_DATA_WIDTH / DATA_WIDTH,
    parameter integer  GROUP_ID_W                   = GROUP_SIZE == 1 ? 0 : $clog2(GROUP_SIZE),
    parameter integer  BUF_ID_N_W                   = $clog2(ARRAY_N),
    parameter integer  BUF_ID_W                     = BUF_ID_N_W + GROUP_ID_W,

    parameter integer  MEM_ADDR_WIDTH               = BUF_ADDR_WIDTH + BUF_ID_W,
    parameter integer  BUF_DATA_WIDTH               = ARRAY_N * ARRAY_M * DATA_WIDTH
)
(
    input  wire                                         clk,
    input  wire                                         reset,

    input  wire                                         mem_write_req,
    input  wire  [ MEM_ADDR_WIDTH       -1 : 0 ]        mem_write_addr,
    input  wire  [ MEM_DATA_WIDTH       -1 : 0 ]        mem_write_data,

    input  wire                                         buf_read_req,
    input  wire  [ BUF_ADDR_WIDTH       -1 : 0 ]        buf_read_addr,
    output wire  [ BUF_DATA_WIDTH       -1 : 0 ]        buf_read_data
  );

genvar n, m;
generate
for (m=0; m<GROUP_SIZE; m=m+1)
begin: LOOP_M
for (n=0; n<ARRAY_N; n=n+1)
begin: LOOP_N

    localparam integer  LOCAL_ADDR_W                 = BUF_ADDR_WIDTH;
    localparam integer  LOCAL_BUF_ID                 = m + n*GROUP_SIZE;

    wire                                        local_buf_read_req;
    wire [ LOCAL_ADDR_W         -1 : 0 ]        local_buf_read_addr;
    wire [ MEM_DATA_WIDTH       -1 : 0 ]        local_buf_read_data;

    assign buf_read_data[(m+n*GROUP_SIZE)*MEM_DATA_WIDTH+:MEM_DATA_WIDTH] = local_buf_read_data;

    wire                                        buf_read_req_fwd;
    wire [ LOCAL_ADDR_W         -1 : 0 ]        buf_read_addr_fwd;

  if (m == 0) begin
      register_sync #(1) read_req_fwd (clk, reset, local_buf_read_req, buf_read_req_fwd);
      register_sync #(LOCAL_ADDR_W) read_addr_fwd (clk, reset, local_buf_read_addr, buf_read_addr_fwd);
  end else begin
      assign buf_read_req_fwd = local_buf_read_req;
      assign buf_read_addr_fwd = local_buf_read_addr;
  end

  if (n == 0) begin
    assign local_buf_read_req = buf_read_req;
    assign local_buf_read_addr = buf_read_addr;
  end
  else begin
    assign local_buf_read_req = LOOP_M[0].LOOP_N[n-1].buf_read_req_fwd;
    assign local_buf_read_addr = LOOP_M[0].LOOP_N[n-1].buf_read_addr_fwd;
  end

    wire [ BUF_ID_W             -1 : 0 ]        local_mem_write_buf_id;
    wire                                        local_mem_write_req;
    wire [ LOCAL_ADDR_W         -1 : 0 ]        local_mem_write_addr;
    wire [ MEM_DATA_WIDTH       -1 : 0 ]        local_mem_write_data;

    wire [ BUF_ID_W             -1 : 0 ]        buf_id;
    assign buf_id = LOCAL_BUF_ID;

  if (BUF_ID_W == 0) begin
    assign local_mem_write_addr = mem_write_addr;
    assign local_mem_write_req = mem_write_req;
    assign local_mem_write_data = mem_write_data;
  end
  else begin
    assign {local_mem_write_addr, local_mem_write_buf_id} = mem_write_addr;
    assign local_mem_write_req = mem_write_req && local_mem_write_buf_id == buf_id;
    assign local_mem_write_data = mem_write_data;
  end

  ram #(
    .ADDR_WIDTH                     ( LOCAL_ADDR_W                   ),
    .DATA_WIDTH                     ( MEM_DATA_WIDTH                 ),
    .OUTPUT_REG                     ( 1                              )
  ) u_ram (
    .clk                            ( clk                            ),
    .reset                          ( reset                          ),
    .s_write_addr                   ( local_mem_write_addr           ),
    .s_write_req                    ( local_mem_write_req            ),
    .s_write_data                   ( local_mem_write_data           ),
    .s_read_addr                    ( local_buf_read_addr            ),
    .s_read_req                     ( local_buf_read_req             ),
    .s_read_data                    ( local_buf_read_data            )
    );

end
end
endgenerate

//=============================================================
// VCD
//=============================================================
  `ifdef COCOTB_TOPLEVEL_buffer
  initial begin
    $dumpfile("buffer.vcd");
    $dumpvars(0, buffer);
  end
  `endif
//=============================================================
endmodule
//
// Precision configurable PE
//
// Hardik Sharma
// (hsharma@gatech.edu)

`timescale 1ns/1ps
module pe #(
  parameter          PE_MODE                      = "FMA",
  parameter integer  ACT_WIDTH                    = 16,
  parameter integer  WGT_WIDTH                    = 16,
  parameter integer  MULT_OUT_WIDTH               = ACT_WIDTH + WGT_WIDTH,
  parameter integer  PE_OUT_WIDTH                 = MULT_OUT_WIDTH

) (
  input  wire                                         clk,
  input  wire                                         reset,
  input  wire  [ ACT_WIDTH            -1 : 0 ]        a,
  input  wire  [ WGT_WIDTH            -1 : 0 ]        b,
  input  wire  [ PE_OUT_WIDTH         -1 : 0 ]        c,
  output wire  [ PE_OUT_WIDTH         -1 : 0 ]        out
);

  wire signed [ MULT_OUT_WIDTH       -1 : 0 ]        mult_out;
  wire signed [ACT_WIDTH-1:0] _a;
  wire signed [WGT_WIDTH-1:0] _b;
  
  assign _a = a;
  assign _b = b;
  
  assign mult_out = _a * _b;
	
  generate
  if (PE_MODE == "FMA") begin
    signed_adder #(
      .REGISTER_OUTPUT                ( "TRUE"                         ),
      .IN1_WIDTH                      ( MULT_OUT_WIDTH                 ),
      .IN2_WIDTH                      ( PE_OUT_WIDTH                   ),
      .OUT_WIDTH                      ( PE_OUT_WIDTH                   )
    ) adder_inst (
      .clk                            ( clk                            ),  // input
      .reset                          ( reset                          ),  // input
      .enable                         (1'b1),
      .a                              ( mult_out                       ),
      .b                              ( c                              ),
      .out                            ( out                            )
    );
  end else begin
    wire [ PE_OUT_WIDTH         -1 : 0 ]        alu_out;
    assign alu_out = $signed(mult_out);

    wire enable = 1'b1;
    register_sync #(
      .WIDTH                          ( PE_OUT_WIDTH                   )
    ) reg_inst (
      .clk                            ( clk                            ),  // input
      .reset                          ( reset                          ),  // input
      .in                             ( alu_out                        ),  // input
      .out                            ( out                            )   // output
    );

  end
  endgenerate

endmodule

