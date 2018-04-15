

module single_port_ram_21_8(
			clk,
			data,
			we,
			addr, 
			out
			);
`define ADDR_WIDTH_21_8 8
`define DATA_WIDTH_21_8 21

    input 				clk;
    input	[`DATA_WIDTH_21_8-1:0] 	data;
    input 				we;
    input	[`ADDR_WIDTH_21_8-1:0] 	addr;

    
    output	[`DATA_WIDTH_21_8-1:0] 	out;
    reg		[`DATA_WIDTH_21_8-1:0] 	out;
     
    reg 	[`DATA_WIDTH_21_8-1:0] 	RAM[255:0];
     
    always @ (posedge clk) 
    begin 
        if (we) 
	begin
	RAM[addr] <= data;
        out <= RAM[addr]; 
	end
    end 
     
endmodule



module single_port_ram_128_8(
			clk,
			data,
			we,
			addr, 
			out
			);
`define ADDR_WIDTH_128_8 8
`define DATA_WIDTH_128_8 128

    input 					clk;
    input	[`DATA_WIDTH_128_8-1:0] 	data;
    input 					we;
    input	[`ADDR_WIDTH_128_8-1:0] 	addr;

    
    output	[`DATA_WIDTH_128_8-1:0] 	out;
    reg		[`DATA_WIDTH_128_8-1:0] 	out;
     
    reg 	[`DATA_WIDTH_128_8-1:0] 	RAM[255:0];
     
    always @ (posedge clk) 
    begin 
        if (we) 
	begin
	RAM[addr] <= data;
        out <= RAM[addr]; 
	end
    end 
     
endmodule



module a25_icache 
 

 		(
		i_clk,
		i_core_stall,
		o_stall,
 
		i_select,
		i_address,     
		i_address_nxt,
		i_cache_enable, 
		i_cache_flush, 
  		o_read_data,                                                       
 
		o_wb_req,       
		i_wb_read_data,                 
		i_wb_ready
		);
 
 
// Instruction Types
localparam [3:0]    REGOP       = 4'h0, // Data processing
                    MULT        = 4'h1, // Multiply
                    SWAP        = 4'h2, // Single Data Swap
                    TRANS       = 4'h3, // Single data transfer
                    MTRANS      = 4'h4, // Multi-word data transfer
                    BRANCH      = 4'h5, // Branch
                    CODTRANS    = 4'h6, // Co-processor data transfer
                    COREGOP     = 4'h7, // Co-processor data operation
                    CORTRANS    = 4'h8, // Co-processor register transfer
                    SWI         = 4'h9; // software interrupt
 
 
// Opcodes
localparam [3:0] AND = 4'h0,        // Logical AND
                 EOR = 4'h1,        // Logical Exclusive OR
                 SUB = 4'h2,        // Subtract
                 RSB = 4'h3,        // Reverse Subtract
                 ADD = 4'h4,        // Add
                 ADC = 4'h5,        // Add with Carry
                 SBC = 4'h6,        // Subtract with Carry
                 RSC = 4'h7,        // Reverse Subtract with Carry
                 TST = 4'h8,        // Test  (using AND operator)
                 TEQ = 4'h9,        // Test Equivalence (using EOR operator)
                 CMP = 4'ha,       // Compare (using Subtract operator)
                 CMN = 4'hb,       // Compare Negated
                 ORR = 4'hc,       // Logical OR
                 MOV = 4'hd,       // Move
                 BIC = 4'he,       // Bit Clear (using AND & NOT operators)
                 MVN = 4'hf;       // Move NOT
 
// Condition Encoding
localparam [3:0] EQ  = 4'h0,        // Equal            / Z set
                 NE  = 4'h1,        // Not equal        / Z clear
                 CS  = 4'h2,        // Carry set        / C set
                 CC  = 4'h3,        // Carry clear      / C clear
                 MI  = 4'h4,        // Minus            / N set
                 PL  = 4'h5,        // Plus             / N clear
                 VS  = 4'h6,        // Overflow         / V set
                 VC  = 4'h7,        // No overflow      / V clear
                 HI  = 4'h8,        // Unsigned higher  / C set and Z clear
                 LS  = 4'h9,        // Unsigned lower
                                    // or same          / C clear or Z set
                 GE  = 4'ha,        // Signed greater 
                                    // than or equal    / N == V
                 LT  = 4'hb,        // Signed less than / N != V
                 GT  = 4'hc,        // Signed greater
                                    // than             / Z == 0, N == V
                 LE  = 4'hd,        // Signed less than
                                    // or equal         / Z == 1, N != V
                 AL  = 4'he,        // Always
                 NV  = 4'hf;        // Never
 
// Any instruction with a condition field of 0b1111 is UNPREDICTABLE.                
 
// Shift Types
localparam [1:0] LSL = 2'h0,
                 LSR = 2'h1,
                 ASR = 2'h2,
                 RRX = 2'h3,
                 ROR = 2'h3; 
 
// Modes
localparam [1:0] SVC  =  2'b11,  // Supervisor
                 IRQ  =  2'b10,  // Interrupt
                 FIRQ =  2'b01,  // Fast Interrupt
                 USR  =  2'b00;  // User
 
// One-Hot Mode encodings
localparam [5:0] OH_USR  = 0,
                 OH_IRQ  = 1,
                 OH_FIRQ = 2,
                 OH_SVC  = 3;
 
 
 
`ifndef _A25_CONFIG_DEFINES
`define _A25_CONFIG_DEFINES
 

 
`define A25_ICACHE_WAYS 4
`define A25_DCACHE_WAYS 4

`endif

parameter CACHE_LINES          = 256;  
 
// This cannot be changed without some major surgeory on
// this module                                       
parameter CACHE_WORDS_PER_LINE = 4;
 

parameter WAYS              = `A25_ICACHE_WAYS;
 

parameter CACHE_ADDR_WIDTH  = 8;                     // = 8
parameter WORD_SEL_WIDTH    = 2;               // = 2
parameter TAG_ADDR_WIDTH    = 20;  // = 20
parameter TAG_WIDTH         = 21;                          // = 21, including Valid flag
parameter CACHE_LINE_WIDTH  = 128;                  // = 128
parameter TAG_ADDR32_LSB    = 12;       // = 12
parameter CACHE_ADDR32_MSB  = 11;   // = 11
parameter CACHE_ADDR32_LSB  = 4;   // = 4
parameter WORD_SEL_MSB      = 3;                     // = 3
parameter WORD_SEL_LSB      = 2;                          // = 2
// ---------------------------------------------------------


input                               i_clk;
input                               i_core_stall;
output                              o_stall;
 
// Read / Write requests from core
input                               i_select;
input      [31:0]                   i_address;          // registered address from execute
input      [31:0]                   i_address_nxt;      // un-registered version of address from execute stage
input                               i_cache_enable;     // from co-processor 15 configuration register
input                               i_cache_flush;      // from co-processor 15 register
 
output     [127:0]                  o_read_data;                                                       
 
// WB Read Request                                                          
output                              o_wb_req;          // Read Request
input      [127:0]                  i_wb_read_data;                 
input                               i_wb_ready;


// One-hot encoded
localparam       C_INIT   = 0,
                 C_CORE   = 1,
                 C_FILL   = 2,
                 C_INVA   = 3,
                 C_STATES = 4;
 
localparam [3:0] CS_INIT            = 4'd0,
                 CS_IDLE            = 4'd1,
                 CS_FILL0           = 4'd2,
                 CS_FILL1           = 4'd3,
                 CS_FILL2           = 4'd4,
                 CS_FILL3           = 4'd5,
                 CS_FILL4           = 4'd6,
                 CS_FILL_COMPLETE   = 4'd7,
                 CS_TURN_AROUND     = 4'd8,
                 CS_WRITE_HIT1      = 4'd9,
                 CS_EX_DELETE       = 4'd10;
 
//reg                              o_wb_req; //jing+
//reg                              o_stall; //jing+
//reg     [127:0]                  o_read_data; //jing+                                                      

reg  [3:0]                  c_state    = 4'd1 ;   // c_state    = CS_IDLE
reg  [C_STATES-1:0]         source_sel = 4'b10;   //1'd1 << C_CORE 
reg  [CACHE_ADDR_WIDTH:0]   init_count = 9'd0;
 
wire [TAG_WIDTH-1:0]        tag_rdata_way0; 
wire [TAG_WIDTH-1:0]        tag_rdata_way1; 
wire [TAG_WIDTH-1:0]        tag_rdata_way2; 
wire [TAG_WIDTH-1:0]        tag_rdata_way3; 
wire [CACHE_LINE_WIDTH-1:0] data_rdata_way0; 
wire [CACHE_LINE_WIDTH-1:0] data_rdata_way1; 
wire [CACHE_LINE_WIDTH-1:0] data_rdata_way2; 
wire [CACHE_LINE_WIDTH-1:0] data_rdata_way3; 
wire [WAYS-1:0]             data_wenable_way; 
wire [WAYS-1:0]             data_hit_way; 
wire [WAYS-1:0]             tag_wenable_way; 
reg  [WAYS-1:0]             select_way = 4'd0; 
wire [WAYS-1:0]             next_way; 
reg  [WAYS-1:0]             valid_bits_r = 4'd0;
 
reg  [3:0]                  random_num = 4'hf;
 
wire [CACHE_ADDR_WIDTH-1:0] tag_address; 
wire [TAG_WIDTH-1:0]        tag_wdata; 
wire                        tag_wenable; 
 
wire [CACHE_ADDR_WIDTH-1:0] data_address; 
wire [31:0]                 write_data_word; 
 
wire                        idle_hit; 
reg                        read_miss; //jing
wire                        read_miss_fill; 
wire                        invalid_read; 
wire                        fill_state; 
 
reg  [31:0]                 miss_address = 32'd0;
wire [CACHE_LINE_WIDTH-1:0] hit_rdata; 
 
wire                        cache_busy_stall; 
wire                        read_stall; 
 
wire                        enable; 
wire [CACHE_ADDR_WIDTH-1:0] address; 
wire [31:0]                 address_c; 
reg  [31:0]                 address_r = 32'd0;
 
reg  [31:0]                 wb_address = 32'd0;
wire                        wb_hit; //jing - add wire -> reg
wire                        read_buf_hit; //jing - add wire -> reg
reg  [127:0]                read_buf_data_r;
reg  [31:0]                 read_buf_addr_r;
reg                         read_buf_valid_r;
//genvar                      i;
 
// ======================================
// Address to use for cache access
// ======================================
// If currently stalled then the address for the next
// cycle will be the same as it is in the current cycle
//
assign address_c = i_core_stall ? i_address    : 
                                  i_address_nxt; 
 
assign address   = address_c[CACHE_ADDR32_MSB:CACHE_ADDR32_LSB];
 
 
// ======================================
// Outputs
// ======================================
assign o_read_data      = wb_hit       ? i_wb_read_data  : 
                          read_buf_hit ? read_buf_data_r :
                                         hit_rdata ;
 
 
// Don't allow the cache to stall the wb i/f for an exclusive access
// The cache needs a couple of cycles to flush a potential copy of the exclusive
// address, but the wb can do the access in parallel. So there is no
// stall in the state CS_EX_DELETE, even though the cache is out of action. 
// This works fine as long as the wb is stalling the core
//assign o_stall          = read_stall  || cache_busy_stall;
always @ ( posedge i_clk ) 
	o_stall         <= read_stall  || cache_busy_stall;
 
assign o_wb_req         = read_miss && c_state == CS_IDLE;
 

// ======================================
// Read Buffer
// ======================================
always@(posedge i_clk)
    if ( i_cache_flush )
        read_buf_valid_r <= 1'd0;
    else if (i_wb_ready && c_state == CS_FILL3)
        begin
        read_buf_data_r  <= i_wb_read_data;
        read_buf_addr_r  <= miss_address;
        read_buf_valid_r <= 1'd1;
        end
    else if (o_wb_req)
        read_buf_valid_r <= 1'd0;
 
 
assign read_buf_hit     = read_buf_valid_r && i_address[31:4] == read_buf_addr_r[31:4];
 
// ======================================
// Cache State Machine
// ======================================
 
// Little State Machine to Flush Tag RAMS
always @ ( posedge i_clk )
    if ( i_cache_flush )
        begin
        c_state     <= CS_INIT;
        source_sel  <= 4'd1;  //1'd1 << C_INIT
        init_count  <= 9'd0;
        `ifdef A25_CACHE_DEBUG  
        `TB_DEBUG_MESSAGE  
        $display("Cache Flush");
        `endif            
        end
    else    
        case ( c_state )
            CS_INIT :
                if ( init_count < CACHE_LINES [CACHE_ADDR_WIDTH:0] )
                    begin
                    init_count  <= init_count + 1'd1;
   		    source_sel  <= 4'b1;  //1'd1 << C_INIT
                    end
                else
                    begin
                    source_sel  <= 4'b10;  //1'd1 << C_CORE
                    c_state     <= CS_TURN_AROUND;
                    end 
 
             CS_IDLE :
                begin
                    source_sel  <= 4'b10;  //1'd1 << C_CORE
 
                if ( read_miss ) 
                    c_state <= CS_FILL3; 
               end
 
 
             CS_FILL3 :
                begin
                // Pick a way to write the cache update into
                // Either pick one of the invalid caches, or if all are valid, then pick
                // one randomly
                select_way  <= next_way; 
                random_num  <= {random_num[2], random_num[1], random_num[0], 
                                 random_num[3]^random_num[2]};
 
                // third read of burst of 4
                // wb read request asserted, wait for ack
                if ( i_wb_ready ) 
                    begin
                    c_state     <= CS_FILL_COMPLETE;
                    end
                end
 
 
             // Write the read fetch data in this cycle
             CS_FILL_COMPLETE : 
                begin
                // Back to normal cache operations, but
                // use physical address for first read as
                // address moved before the stall was asserted for the read_miss
                // However don't use it if its a non-cached address!
                source_sel  <= 4'b10;  //1'd1 << C_CORE        
                c_state     <= CS_TURN_AROUND;    
                end                                 
 
 
             // Ignore the tag read data in this cycle   
             // Wait 1 cycle to pre-read the cache and return to normal operation                 
             CS_TURN_AROUND : 
                begin
                c_state     <= CS_IDLE;
                end
 
        endcase                       
 
 
// ======================================
// Miss Address
// ======================================
always @ ( posedge i_clk )
    if ( c_state == CS_IDLE )
        miss_address <= i_address;
 
 
always @ ( posedge i_clk )
    address_r <= address_c;
 
assign invalid_read = address_r != i_address;
 
 
always @(posedge i_clk)
    if ( o_wb_req )
        wb_address <= i_address;
    else if ( i_wb_ready && fill_state )    
        wb_address <= {wb_address[31:4], wb_address[3:2] + 1'd1, 2'd0};
 
assign fill_state       = c_state == CS_FILL3;
assign wb_hit           = i_address == wb_address && i_wb_ready && fill_state;
 
assign tag_address      = read_miss_fill     ? miss_address      [CACHE_ADDR32_MSB:CACHE_ADDR32_LSB] :
                          source_sel[C_INIT] ? init_count[CACHE_ADDR_WIDTH-1:0]                      :
                                               address                                               ;
 
 
assign data_address     = read_miss_fill     ? miss_address[CACHE_ADDR32_MSB:CACHE_ADDR32_LSB] : 
                                               address                                         ;
 
 
assign tag_wdata        = read_miss_fill     ? {1'd1, miss_address[31:12]} :     //  [31:TAG_ADDR32_LSB]
                                               21'd0                       ;   //   {TAG_WIDTH{1'd0}}   TAG_WIDTH =21
 
 
assign read_miss_fill   = c_state == CS_FILL3 && i_wb_ready;
 
 
 
assign tag_wenable      = read_miss_fill     ? 1'd1  :
                          source_sel[C_INVA] ? 1'd1  :
                          source_sel[C_FILL] ? 1'd1  :
                          source_sel[C_INIT] ? 1'd1  :
                          source_sel[C_CORE] ? 1'd0  :
                                               1'd0  ;
 
 
assign enable           = i_select && i_cache_enable;
 
assign idle_hit         = |data_hit_way;
 
assign read_miss        = enable && !idle_hit && !invalid_read;
 
assign read_stall       = (i_select && i_cache_enable) && !(|data_hit_way) && !wb_hit && !read_buf_hit;
//assign read_stall       = enable && !idle_hit && !wb_hit && !read_buf_hit;
 
assign cache_busy_stall = (c_state == CS_TURN_AROUND  && enable && !read_buf_hit) || c_state == CS_INIT;
 
 
// ======================================
// Instantiate RAMS
// ======================================
 
//generate
//    for ( i=0; i<WAYS;i=i+1 ) begin : rams
 
//            #(
//            .DATA_WIDTH                 ( TAG_WIDTH             ),
//            .INITIALIZE_TO_ZERO         ( 1                     ),
//            .ADDRESS_WIDTH              ( CACHE_ADDR_WIDTH      ))

        single_port_ram_21_8 u_tag0 (
            .clk               ( i_clk                 ),
            .data              ( tag_wdata             ),
            .we                ( tag_wenable_way[0]    ),
            .addr              ( tag_address           ),
            .out               ( tag_rdata_way0        )
            );
 
//           #(
//            .DATA_WIDTH    ( CACHE_LINE_WIDTH) ,
//            .ADDRESS_WIDTH ( CACHE_ADDR_WIDTH) )
        single_port_ram_128_8 u_data0 (
            .clk                ( i_clk                         ),
            .data               ( i_wb_read_data                ),
            .we                 ( data_wenable_way[0]           ),
            .addr               ( data_address                  ),
//            .i_byte_enable              ( {CACHE_LINE_WIDTH/8{1'd1}}    ),
            .out                ( data_rdata_way0               )
            );       

        // Per tag-ram write-enable
        assign tag_wenable_way[0]  = tag_wenable && ( select_way[0] || source_sel[C_INIT] );
 
        // Per data-ram write-enable
        assign data_wenable_way[0] = ( source_sel[C_FILL] || read_miss_fill ) && select_way[0];
 
        // Per data-ram idle_hit flag
        assign data_hit_way[0]     = tag_rdata_way0[TAG_WIDTH-1] &&                                                  
                                     tag_rdata_way0[19:0] == i_address[31:12] &&  
                                     c_state == CS_IDLE;

//            #(
//            .DATA_WIDTH                 ( TAG_WIDTH             ),
//            .INITIALIZE_TO_ZERO         ( 1                     ),
//            .ADDRESS_WIDTH              ( CACHE_ADDR_WIDTH      ))
        single_port_ram_21_8 u_tag1 (
            .clk                ( i_clk                 ),
            .data               ( tag_wdata             ),
            .we                 ( tag_wenable_way[1]    ),
            .addr               ( tag_address           ),
 
            .out                ( tag_rdata_way1      )
            );
  
//     #(
//            .DATA_WIDTH    ( CACHE_LINE_WIDTH) ,
//            .ADDRESS_WIDTH ( CACHE_ADDR_WIDTH) )
        single_port_ram_128_8 u_data1 (
            .clk                 ( i_clk                         ),
            .data                ( i_wb_read_data                ),
            .we                  ( data_wenable_way[1]           ),
            .addr                ( data_address                  ),
//            .i_byte_enable              ( {CACHE_LINE_WIDTH/8{1'd1}}    ),
            .out                 ( data_rdata_way1             )
            ); 

        // Per tag-ram write-enable
        assign tag_wenable_way[1]  = tag_wenable && ( select_way[1] || source_sel[C_INIT] );
 
        // Per data-ram write-enable
        assign data_wenable_way[1] = ( source_sel[C_FILL] || read_miss_fill ) && select_way[1];
 
        // Per data-ram idle_hit flag
        assign data_hit_way[1]     = tag_rdata_way1[TAG_WIDTH-1] &&                                                  
                                     tag_rdata_way1[19:0] == i_address[31:12] &&  
                                     c_state == CS_IDLE;  

//            #(
//            .DATA_WIDTH                 ( TAG_WIDTH             ),
//            .INITIALIZE_TO_ZERO         ( 1                     ),
//            .ADDRESS_WIDTH              ( CACHE_ADDR_WIDTH      ))
        single_port_ram_21_8 u_tag2 (
            .clk                ( i_clk                 ),
            .data               ( tag_wdata             ),
            .we                 ( tag_wenable_way[2]    ),
            .addr               ( tag_address           ),
 
            .out                ( tag_rdata_way2      )
            );
     
//          #(
//            .DATA_WIDTH    ( CACHE_LINE_WIDTH) ,
//            .ADDRESS_WIDTH ( CACHE_ADDR_WIDTH) )
        single_port_ram_128_8 u_data2 (
            .clk                 ( i_clk                         ),
            .data                ( i_wb_read_data                ),
            .we                  ( data_wenable_way[2]           ),
            .addr                ( data_address                  ),
//            .i_byte_enable              ( {CACHE_LINE_WIDTH/8{1'd1}}    ),
            .out                 ( data_rdata_way2             )
            );  

        // Per tag-ram write-enable
        assign tag_wenable_way[2]  = tag_wenable && ( select_way[2] || source_sel[C_INIT] );
 
        // Per data-ram write-enable
        assign data_wenable_way[2] = ( source_sel[C_FILL] || read_miss_fill ) && select_way[2];
 
        // Per data-ram idle_hit flag
        assign data_hit_way[2]     = tag_rdata_way2[TAG_WIDTH-1] &&                                                  
                                     tag_rdata_way2[19:0] == i_address[31:12] &&  
                                     c_state == CS_IDLE;  

//            #(
//            .DATA_WIDTH                 ( TAG_WIDTH             ),
//            .INITIALIZE_TO_ZERO         ( 1                     ),
//            .ADDRESS_WIDTH              ( CACHE_ADDR_WIDTH      ))
       single_port_ram_21_8 u_tag3 (
            .clk                ( i_clk                 ),
            .data               ( tag_wdata             ),
            .we                 ( tag_wenable_way[3]    ),
            .addr               ( tag_address           ),
 
            .out                ( tag_rdata_way3      )
            );


                                             
 
//          #(
//           .DATA_WIDTH    ( CACHE_LINE_WIDTH) ,
//            .ADDRESS_WIDTH ( CACHE_ADDR_WIDTH) )
       single_port_ram_128_8 u_data3 (
            .clk                 ( i_clk                         ),
            .data                ( i_wb_read_data                ),
            .we                  ( data_wenable_way[3]           ),
            .addr                ( data_address                  ),
//            .i_byte_enable              ( {CACHE_LINE_WIDTH/8{1'd1}}    ),
            .out                 ( data_rdata_way3             )
            ); 

 
        // Per tag-ram write-enable
        assign tag_wenable_way[3]  = tag_wenable && ( select_way[3] || source_sel[C_INIT] );
 
        // Per data-ram write-enable
        assign data_wenable_way[3] = ( source_sel[C_FILL] || read_miss_fill ) && select_way[3];
 
        // Per data-ram idle_hit flag
        assign data_hit_way[3]     = tag_rdata_way3[TAG_WIDTH-1] &&                                                  
                                     tag_rdata_way3[19:0] == i_address[31:12] &&  
                                     c_state == CS_IDLE;                                                               
//    end                                                         
//endgenerate

 
    always @ ( posedge i_clk )
        if ( c_state == CS_IDLE )
            valid_bits_r <= {tag_rdata_way3[TAG_WIDTH-1], 
                             tag_rdata_way2[TAG_WIDTH-1], 
                             tag_rdata_way1[TAG_WIDTH-1], 
                             tag_rdata_way0[TAG_WIDTH-1]};

 
    assign hit_rdata    = data_hit_way[0] ? data_rdata_way0 :
                          data_hit_way[1] ? data_rdata_way1 :
                          data_hit_way[2] ? data_rdata_way2 :
                          data_hit_way[3] ? data_rdata_way3 :
			128'hffffffffffffffffffffffffffffffff;
                                //     {CACHE_LINE_WIDTH{1'd1}} ;  // all 1's for debug
 


   assign next_way   = 	valid_bits_r[0] == 1'd0 ? 4'b0001:
			valid_bits_r[1] == 1'd0 ? 4'b0010:
			valid_bits_r[2] == 1'd0 ? 4'b0100:
			valid_bits_r[3] == 1'd0 ? 4'b1000:
						  (
						    random_num[3:1] == 3'd0 ? 4'b0100:
						    random_num[3:1] == 3'd1 ? 4'b0100:
						    random_num[3:1] == 3'd2 ? 4'b1000:
						    random_num[3:1] == 3'd3 ? 4'b1000:
						    random_num[3:1] == 3'd4 ? 4'b0001:
						    random_num[3:1] == 3'd5 ? 4'b0001:
						    			      4'b0010
							);


endmodule
 
 
 
 
module a25_fetch(
		i_clk,
		i_mem_stall,
		i_exec_stall,
		i_conflict,       
		o_fetch_stall,    
                                            
		i_system_rdy,   
 
		i_iaddress,
		i_iaddress_valid,
		i_iaddress_nxt,    
		o_fetch_instruction,
 
		i_cache_enable,
		i_cache_flush,     
		i_cacheable_area,  
 
		o_wb_req,
		o_wb_address,
		i_wb_read_data,
		i_wb_ready 
		);

input                       i_clk;
input                       i_mem_stall;
input                       i_exec_stall;
input                       i_conflict;         // Decode stage stall pipeline because of an instruction conflict
output                      o_fetch_stall;      // when this is asserted all registers 
                                                // in decode and exec stages are frozen
input                       i_system_rdy;       // External system can stall core with this signal
 
input       [31:0]          i_iaddress;
input                       i_iaddress_valid;
input       [31:0]          i_iaddress_nxt;     // un-registered version of address to the cache rams
output      [31:0]          o_fetch_instruction;
 
input                       i_cache_enable;     // cache enable
input                       i_cache_flush;      // cache flush
input       [31:0]          i_cacheable_area;   // each bit corresponds to 2MB address space
 
output                      o_wb_req;
//reg 			    o_wb_req; //jing+
output      [31:0]          o_wb_address;
input       [127:0]         i_wb_read_data;
input                       i_wb_ready;
 
wire                        core_stall;		//jing - add wire -> reg
reg                        cache_stall;		//jing- add wire -> reg
//reg                        o_fetch_stall;	//jing+
wire    [127:0]             cache_read_data128; 
wire    [31:0]              cache_read_data; 
wire                        sel_cache; 
wire                        uncached_instruction_read; 
wire                        address_cachable; 
wire                         icache_wb_req; 	
wire                        wait_wb; 
reg                         wb_req_r = 1'd0;
wire    [31:0]              wb_rdata32; 


 
// e.g. 24 for 32MBytes, 26 for 128MBytes
localparam MAIN_MSB             = 26; 
 
// e.g. 13 for 4k words
localparam BOOT_MSB             = 13;
 
localparam MAIN_BASE            = 32'h00000000; /*  Main Memory            */
localparam BOOT_BASE            = 32'h00000000; /*  Cachable Boot Memory   */
localparam AMBER_TM_BASE        = 16'h1300;      /*  Timers Module          */
localparam AMBER_IC_BASE        = 16'h1400;      /*  Interrupt Controller   */
localparam AMBER_UART0_BASE     = 16'h1600;      /*  UART 0                 */
localparam AMBER_UART1_BASE     = 16'h1700;      /*  UART 1                 */
localparam ETHMAC_BASE          = 16'h2000;      /*  Ethernet MAC           */
localparam HIBOOT_BASE          = 32'h28000000; /*  Uncachable Boot Memory */
localparam TEST_BASE            = 16'hf000;      /*  Test Module            */
 

assign address_cachable         = (
					 ( i_iaddress >= 32'h00000000  &&  i_iaddress < 32'h7fff) 
					|| ( 
						(i_iaddress >= MAIN_BASE  &&  i_iaddress < (MAIN_BASE   + 32'hfffffff)) 
							&& !( (i_iaddress >= BOOT_BASE && i_iaddress < (BOOT_BASE  + 32'h7fff))
											||(i_iaddress[31:14] == HIBOOT_BASE>>(14)))
					   )
				  )
				&& ((i_iaddress[25:21] == 5'b00000) ? i_cacheable_area[0] :
					(i_iaddress[25:21] == 5'b00001) ? i_cacheable_area[1] :
					(i_iaddress[25:21] == 5'b00010) ? i_cacheable_area[2] :
					(i_iaddress[25:21] == 5'b00011) ? i_cacheable_area[3] :
					(i_iaddress[25:21] == 5'b00100) ? i_cacheable_area[4] :
					(i_iaddress[25:21] == 5'b00101) ? i_cacheable_area[5] :
					(i_iaddress[25:21] == 5'b00110) ? i_cacheable_area[6] :
					(i_iaddress[25:21] == 5'b00111) ? i_cacheable_area[7] :
					(i_iaddress[25:21] == 5'b01000) ? i_cacheable_area[8] :
					(i_iaddress[25:21] == 5'b01001) ? i_cacheable_area[9] :
					(i_iaddress[25:21] == 5'b01010) ? i_cacheable_area[10] :
					(i_iaddress[25:21] == 5'b01011) ? i_cacheable_area[11] :
					(i_iaddress[25:21] == 5'b01100) ? i_cacheable_area[12] :
					(i_iaddress[25:21] == 5'b01101) ? i_cacheable_area[13] :
					(i_iaddress[25:21] == 5'b01110) ? i_cacheable_area[14] :
					(i_iaddress[25:21] == 5'b01111) ? i_cacheable_area[15] :
					(i_iaddress[25:21] == 5'b10000) ? i_cacheable_area[16] :
					(i_iaddress[25:21] == 5'b10001) ? i_cacheable_area[17] :
					(i_iaddress[25:21] == 5'b10010) ? i_cacheable_area[18] :
					(i_iaddress[25:21] == 5'b10011) ? i_cacheable_area[19] :
					(i_iaddress[25:21] == 5'b10100) ? i_cacheable_area[20] :
					(i_iaddress[25:21] == 5'b10101) ? i_cacheable_area[21] :
					(i_iaddress[25:21] == 5'b10110) ? i_cacheable_area[22] :
					(i_iaddress[25:21] == 5'b10111) ? i_cacheable_area[23] :
					(i_iaddress[25:21] == 5'b11000) ? i_cacheable_area[24] :
					(i_iaddress[25:21] == 5'b11001) ? i_cacheable_area[25] :
					(i_iaddress[25:21] == 5'b11010) ? i_cacheable_area[26] :
					(i_iaddress[25:21] == 5'b11011) ? i_cacheable_area[27] :
					(i_iaddress[25:21] == 5'b11100) ? i_cacheable_area[28] :
					(i_iaddress[25:21] == 5'b11101) ? i_cacheable_area[29] :
					(i_iaddress[25:21] == 5'b11110) ? i_cacheable_area[30] :
					i_cacheable_area[31] );

//i_cacheable_area[i_iaddress[25:21]];
 
assign sel_cache         = address_cachable && i_iaddress_valid && i_cache_enable;
 
// Don't start wishbone transfers when the cache is stalling the core
// The cache stalls the core during its initialization sequence
assign uncached_instruction_read = !sel_cache && i_iaddress_valid && !cache_stall;
 
// Return read data either from the wishbone bus or the cache 
assign cache_read_data     = i_iaddress[3:2] == 2'd0    ? cache_read_data128[ 31: 0] :
                             i_iaddress[3:2] == 2'd1    ? cache_read_data128[ 63:32] :
                             i_iaddress[3:2] == 2'd2    ? cache_read_data128[ 95:64] :
                                                          cache_read_data128[127:96] ;
 
assign wb_rdata32 = i_iaddress[3:2] == 2'd0 ? i_wb_read_data[ 31: 0] :
                    i_iaddress[3:2] == 2'd1 ? i_wb_read_data[ 63:32] :
                    i_iaddress[3:2] == 2'd2 ? i_wb_read_data[ 95:64] :
                                              i_wb_read_data[127:96] ;
 
assign o_fetch_instruction = sel_cache                  ? cache_read_data : 
                             uncached_instruction_read  ? wb_rdata32      :
                                                          32'hffeeddcc    ;
 
// Stall the instruction decode and execute stages of the core
// when the fetch stage needs more than 1 cycle to return the requested
// read data

assign o_fetch_stall    = !i_system_rdy || wait_wb || cache_stall;
 
assign o_wb_address     = i_iaddress;
assign o_wb_req         = icache_wb_req || uncached_instruction_read;
 
assign wait_wb          = (o_wb_req || wb_req_r) && !i_wb_ready;
 
always @(posedge i_clk)
    wb_req_r <= o_wb_req && !i_wb_ready;
 
assign core_stall = o_fetch_stall || i_mem_stall || i_exec_stall || i_conflict;
 
// ======================================
// L1 Instruction Cache
// ======================================
a25_icache u_cache (
    .i_clk                      ( i_clk                 ),
    .i_core_stall               ( core_stall            ),
    .o_stall                    ( cache_stall           ),
 
    .i_select                   ( sel_cache             ),
    .i_address                  ( i_iaddress            ),
    .i_address_nxt              ( i_iaddress_nxt        ),
    .i_cache_enable             ( i_cache_enable        ),
    .i_cache_flush              ( i_cache_flush         ),
    .o_read_data                ( cache_read_data128    ),
 
    .o_wb_req                   ( icache_wb_req         ),
    .i_wb_read_data             ( i_wb_read_data        ),
    .i_wb_ready                 ( i_wb_ready            )
);
 
 
endmodule
 



module a25_decode(
		i_clk,
		i_fetch_instruction,
		i_core_stall,                   
		i_irq,                        
		i_firq,                        
		i_dabt,                    
		i_iabt,                         
		i_adex,                        
		i_execute_iaddress,             
	//	i_execute_daddress,             
		i_abt_status,                   
		i_execute_status_bits,      
		i_multiply_done,             
 

		o_imm32,
		o_imm_shift_amount,
		o_shift_imm_zero,
		o_condition,    
		o_decode_exclusive,      
		o_decode_iaccess,     
		o_decode_daccess,       
		o_status_bits_mode,  
		o_status_bits_irq_mask,
		o_status_bits_firq_mask,
 
		o_rm_sel,
		o_rs_sel,
		o_load_rd,                

		o_rn_sel,
		o_barrel_shift_amount_sel,
		o_barrel_shift_data_sel,
		o_barrel_shift_function,
		o_alu_function,
		o_multiply_function,
		o_interrupt_vector_sel,
		o_iaddress_sel,
		o_daddress_sel,
		o_pc_sel,
		o_byte_enable_sel,      
		o_status_bits_sel,
		o_reg_write_sel,
		o_user_mode_regs_store_nxt,
		o_firq_not_user_mode,
 
		o_write_data_wen,
		o_base_address_wen,      
                                                          
		o_pc_wen,
		o_reg_bank_wen,
		o_status_bits_flags_wen,
		o_status_bits_mode_wen,
		o_status_bits_irq_mask_wen,
		o_status_bits_firq_mask_wen,
 
		o_copro_opcode1,
		o_copro_opcode2,
		o_copro_crn,    
		o_copro_crm,
		o_copro_num,
		o_copro_operation, 
                                                    
		o_copro_write_data_wen,
		o_iabt_trigger,
		o_iabt_address,
		o_iabt_status,
		o_dabt_trigger,
		o_dabt_address,
		o_dabt_status,
		o_conflict,
		o_rn_use_read,
		o_rm_use_read,
		o_rs_use_read,
		o_rd_use_read
);

/************************* IO Declarations *********************/
input                       i_clk;
input       [31:0]          i_fetch_instruction;
input                       i_core_stall;                   // stall all stages of the Amber core at the same time
input                       i_irq;                          // interrupt request
input                       i_firq;                         // Fast interrupt request
input                       i_dabt;                         // data abort interrupt request
input                       i_iabt;                         // instruction pre-fetch abort flag
input                       i_adex;                         // Address Exception
input       [31:0]          i_execute_iaddress;             // Registered instruction address output by execute stage
//input       [31:0]          i_execute_daddress;             // Registered instruction address output by execute stage
input       [7:0]           i_abt_status;                   // Abort status
input       [31:0]          i_execute_status_bits;         // current status bits values in execute stage
input                       i_multiply_done;                // multiply unit is nearly done
 
 
// --------------------------------------------------
// Control signals to execute stage
// --------------------------------------------------
output  [31:0]          o_imm32;
output  [4:0]           o_imm_shift_amount;
output                  o_shift_imm_zero;
output  [3:0]           o_condition;             // 4'he = al
output                  o_decode_exclusive;       // exclusive access request ( swap instruction )
output                  o_decode_iaccess;        // Indicates an instruction access
output                  o_decode_daccess;         // Indicates a data access
output  [1:0]           o_status_bits_mode;     // SVC
output                  o_status_bits_irq_mask;
output                  o_status_bits_firq_mask;
 
output  [3:0]           o_rm_sel;
output  [3:0]           o_rs_sel;
output  [7:0]           o_load_rd;               // [7] load flags with PC
                                                            // [6] load status bits with PC
                                                            // [5] Write into User Mode register
                                                            // [4] zero-extend load
                                                            // [3:0] destination register, Rd
output  [3:0]           o_rn_sel;
output  [1:0]           o_barrel_shift_amount_sel;
output  [1:0]           o_barrel_shift_data_sel;
output  [1:0]           o_barrel_shift_function;
output  [8:0]           o_alu_function;
output  [1:0]           o_multiply_function;
output  [2:0]           o_interrupt_vector_sel;
output  [3:0]           o_iaddress_sel;
output  [3:0]           o_daddress_sel;
output  [2:0]           o_pc_sel;
output  [1:0]           o_byte_enable_sel;        // byte, halfword or word write
output  [2:0]           o_status_bits_sel;
output  [2:0]           o_reg_write_sel;
output                  o_user_mode_regs_store_nxt;
output                  o_firq_not_user_mode;
 
output                  o_write_data_wen;
output                  o_base_address_wen;     // save ldm base address register
                                                            // in case of data abort
output                  o_pc_wen;
output  [14:0]          o_reg_bank_wen;
output                  o_status_bits_flags_wen;
output                  o_status_bits_mode_wen;
output                  o_status_bits_irq_mask_wen;
output                  o_status_bits_firq_mask_wen;
 
// --------------------------------------------------
// Co-Processor interface
// --------------------------------------------------
output  [2:0]           o_copro_opcode1;
output  [2:0]           o_copro_opcode2;
output  [3:0]           o_copro_crn;    
output  [3:0]           o_copro_crm;
output  [3:0]           o_copro_num;
output  [1:0]           o_copro_operation; // 0 = no operation, 
                                                     // 1 = Move to Amber Core Register from Coprocessor
                                                     // 2 = Move to Coprocessor from Amber Core Register
output                  o_copro_write_data_wen;
output                  o_iabt_trigger;
output  [31:0]          o_iabt_address;
output  [7:0]           o_iabt_status;
output                  o_dabt_trigger;
output  [31:0]          o_dabt_address;
output  [7:0]           o_dabt_status;
output                  o_conflict;
output                  o_rn_use_read;
output                  o_rm_use_read;
output                  o_rs_use_read;
output                  o_rd_use_read;




/*********************** Signal Declarations *******************/

reg     [31:0]      o_imm32 = 32'd0;
reg     [4:0]       o_imm_shift_amount = 5'd0;
reg           	    o_shift_imm_zero = 1'd0;
reg     [3:0]       o_condition = 4'he;             // 4'he = al
reg                 o_decode_exclusive = 1'd0;       // exclusive access request ( swap instruction )
reg           	    o_decode_iaccess = 1'd1;        // Indicates an instruction access
reg           	    o_decode_daccess = 1'd0;         // Indicates a data access
reg     [1:0]       o_status_bits_mode = 2'b11;     // SVC
reg          	    o_status_bits_irq_mask = 1'd1;
reg           	    o_status_bits_firq_mask = 1'd1;
 
reg     [3:0]       o_rm_sel  = 4'd0;
reg     [3:0]       o_rs_sel  = 4'd0;
reg     [7:0]       o_load_rd = 8'd0;                // [7] load flags with PC
                                                            
reg     [3:0]       o_rn_sel  = 4'd0;
reg     [1:0]       o_barrel_shift_amount_sel = 2'd0;
reg     [1:0]       o_barrel_shift_data_sel = 2'd0;
reg     [1:0]       o_barrel_shift_function = 2'd0;
reg     [8:0]       o_alu_function = 9'd0;
reg     [1:0] 	    o_multiply_function = 2'd0;
reg     [2:0]       o_interrupt_vector_sel = 3'd0;
reg     [3:0]       o_iaddress_sel = 4'd2;
reg     [3:0]       o_daddress_sel = 4'd2;
reg     [2:0]       o_pc_sel = 3'd2;
reg     [1:0]       o_byte_enable_sel = 2'd0;        // byte, halfword or word write
reg     [2:0]       o_status_bits_sel = 3'd0;
reg     [2:0]       o_reg_write_sel;
reg           	    o_user_mode_regs_store_nxt;
reg           	    o_firq_not_user_mode;
 
reg           	    o_write_data_wen = 1'd0;
reg           	    o_base_address_wen = 1'd0;       // save ldm base address register
                                                            // in case of data abort
reg           	    o_pc_wen = 1'd1;
reg     [14:0]      o_reg_bank_wen = 15'd0;
reg           	    o_status_bits_flags_wen = 1'd0;
reg           	    o_status_bits_mode_wen = 1'd0;
reg           	    o_status_bits_irq_mask_wen = 1'd0;
reg           	    o_status_bits_firq_mask_wen = 1'd0;
 
// --------------------------------------------------
// Co-Processor interface
// --------------------------------------------------
reg     [2:0]       o_copro_opcode1 = 3'd0;
reg     [2:0]       o_copro_opcode2 = 3'd0;
reg     [3:0]       o_copro_crn = 4'd0;    
reg     [3:0]       o_copro_crm = 4'd0;
reg     [3:0]       o_copro_num = 4'd0;
reg     [1:0]       o_copro_operation = 2'd0; // 0 = no operation, 
                                                     // 1 = Move to Amber Core Register from Coprocessor
                                                     // 2 = Move to Coprocessor from Amber Core Register
reg           o_copro_write_data_wen = 1'd0;
reg           o_rn_use_read;
reg           o_rm_use_read;
reg           o_rs_use_read;
reg           o_rd_use_read;
 
// Instruction Types
localparam [3:0]    REGOP       = 4'h0, // Data processing
                    MULT        = 4'h1, // Multiply
                    SWAP        = 4'h2, // Single Data Swap
                    TRANS       = 4'h3, // Single data transfer
                    MTRANS      = 4'h4, // Multi-word data transfer
                    BRANCH      = 4'h5, // Branch
                    CODTRANS    = 4'h6, // Co-processor data transfer
                    COREGOP     = 4'h7, // Co-processor data operation
                    CORTRANS    = 4'h8, // Co-processor register transfer
                    SWI         = 4'h9; // software interrupt
 
 
// Opcodes
localparam [3:0] AND = 4'h0,        // Logical AND
                 EOR = 4'h1,        // Logical Exclusive OR
                 SUB = 4'h2,        // Subtract
                 RSB = 4'h3,        // Reverse Subtract
                 ADD = 4'h4,        // Add
                 ADC = 4'h5,        // Add with Carry
                 SBC = 4'h6,        // Subtract with Carry
                 RSC = 4'h7,        // Reverse Subtract with Carry
                 TST = 4'h8,        // Test  (using AND operator)
                 TEQ = 4'h9,        // Test Equivalence (using EOR operator)
                 CMP = 4'ha,       // Compare (using Subtract operator)
                 CMN = 4'hb,       // Compare Negated
                 ORR = 4'hc,       // Logical OR
                 MOV = 4'hd,       // Move
                 BIC = 4'he,       // Bit Clear (using AND & NOT operators)
                 MVN = 4'hf;       // Move NOT
 
// Condition Encoding
localparam [3:0] EQ  = 4'h0,        // Equal            / Z set
                 NE  = 4'h1,        // Not equal        / Z clear
                 CS  = 4'h2,        // Carry set        / C set
                 CC  = 4'h3,        // Carry clear      / C clear
                 MI  = 4'h4,        // Minus            / N set
                 PL  = 4'h5,        // Plus             / N clear
                 VS  = 4'h6,        // Overflow         / V set
                 VC  = 4'h7,        // No overflow      / V clear
                 HI  = 4'h8,        // Unsigned higher  / C set and Z clear
                 LS  = 4'h9,        // Unsigned lower
                                    // or same          / C clear or Z set
                 GE  = 4'ha,        // Signed greater 
                                    // than or equal    / N == V
                 LT  = 4'hb,        // Signed less than / N != V
                 GT  = 4'hc,        // Signed greater
                                    // than             / Z == 0, N == V
                 LE  = 4'hd,        // Signed less than
                                    // or equal         / Z == 1, N != V
                 AL  = 4'he,        // Always
                 NV  = 4'hf;        // Never
 
// Any instruction with a condition field of 0b1111 is UNPREDICTABLE.                
 
// Shift Types
localparam [1:0] LSL = 2'h0,
                 LSR = 2'h1,
                 ASR = 2'h2,
                 RRX = 2'h3,
                 ROR = 2'h3; 
 
// Modes
localparam [1:0] SVC  =  2'b11,  // Supervisor
                 IRQ  =  2'b10,  // Interrupt
                 FIRQ =  2'b01,  // Fast Interrupt
                 USR  =  2'b00;  // User
 
// One-Hot Mode encodings
localparam [5:0] OH_USR  = 0,
                 OH_IRQ  = 1,
                 OH_FIRQ = 2,
                 OH_SVC  = 3;
 
 

 
localparam [4:0] RST_WAIT1      = 5'd0,
                 RST_WAIT2      = 5'd1,
                 INT_WAIT1      = 5'd2,
                 INT_WAIT2      = 5'd3,
                 EXECUTE        = 5'd4,
                 PRE_FETCH_EXEC = 5'd5,  // Execute the Pre-Fetched Instruction
                 MEM_WAIT1      = 5'd6,  // conditionally decode current instruction, in case
                                         // previous instruction does not execute in S2
                 MEM_WAIT2      = 5'd7,
                 PC_STALL1      = 5'd8,  // Program Counter altered
                                         // conditionally decude current instruction, in case
                                         // previous instruction does not execute in S2
                 PC_STALL2      = 5'd9,
                 MTRANS_EXEC1   = 5'd10,
                 MTRANS_EXEC2   = 5'd11,
                 MTRANS_ABORT   = 5'd12,
                 MULT_PROC1     = 5'd13,  // first cycle, save pre fetch instruction
                 MULT_PROC2     = 5'd14,  // do multiplication
                 MULT_STORE     = 5'd15,  // save RdLo
                 MULT_ACCUMU    = 5'd16,  // Accumulate add lower 32 bits
                 SWAP_WRITE     = 5'd17,
                 SWAP_WAIT1     = 5'd18,
                 SWAP_WAIT2     = 5'd19,
                 COPRO_WAIT     = 5'd20;
 
 
// ========================================================
// Internal signals
// ========================================================
wire    [31:0]         instruction;
wire    [3:0]          type;                    // regop, mem access etc.
wire                   instruction_iabt;        // abort flag, follows the instruction
wire                   instruction_adex;        // address exception flag, follows the instruction
wire    [31:0]         instruction_address;     // instruction virtual address, follows 
                                                // the instruction
wire    [7:0]          instruction_iabt_status; // abort status, follows the instruction
wire    [1:0]          instruction_sel;
wire    [3:0]          opcode;
wire    [7:0]          imm8;
wire    [31:0]         offset12;
wire    [31:0]         offset24;
wire    [4:0]          shift_imm;
 
wire                   opcode_compare;
wire                   mem_op;
wire                   load_op;
wire                   store_op;
wire                   write_pc;
wire                   current_write_pc;
reg                    load_pc_nxt;
reg                    load_pc_r = 1'd0;
wire                   immediate_shift_op;
wire                   rds_use_rs;
wire                   branch;
wire                   mem_op_pre_indexed;
wire                   mem_op_post_indexed;
 
// Flop inputs
wire    [31:0]         imm32_nxt;
wire    [4:0]          imm_shift_amount_nxt;
wire                   shift_imm_zero_nxt;
wire    [3:0]          condition_nxt;
reg                    decode_exclusive_nxt;
reg                    decode_iaccess_nxt;
reg                    decode_daccess_nxt;
 
reg     [1:0]          barrel_shift_function_nxt;
wire    [8:0]          alu_function_nxt;
reg     [1:0]          multiply_function_nxt;
reg     [1:0]          status_bits_mode_nxt;
reg                    status_bits_irq_mask_nxt;
reg                    status_bits_firq_mask_nxt;
 
wire    [3:0]          rm_sel_nxt;
wire    [3:0]          rs_sel_nxt;
 
wire    [3:0]          rn_sel_nxt;
reg     [1:0]          barrel_shift_amount_sel_nxt;
reg     [1:0]          barrel_shift_data_sel_nxt;
reg     [3:0]          iaddress_sel_nxt;
reg     [3:0]          daddress_sel_nxt;
reg     [2:0]          pc_sel_nxt;
reg     [1:0]          byte_enable_sel_nxt;
reg     [2:0]          status_bits_sel_nxt;
reg     [2:0]          reg_write_sel_nxt;
wire                   firq_not_user_mode_nxt;
 
// ALU Function signals
reg                    alu_swap_sel_nxt;
reg                    alu_not_sel_nxt;
reg     [1:0]          alu_cin_sel_nxt;
reg                    alu_cout_sel_nxt;
reg     [3:0]          alu_out_sel_nxt;
 
reg                    write_data_wen_nxt;
reg                    copro_write_data_wen_nxt;
reg                    base_address_wen_nxt;
reg                    pc_wen_nxt;
reg     [14:0]         reg_bank_wen_nxt;
reg                    status_bits_flags_wen_nxt;
reg                    status_bits_mode_wen_nxt;
reg                    status_bits_irq_mask_wen_nxt;
reg                    status_bits_firq_mask_wen_nxt;
 
reg                    saved_current_instruction_wen;   // saved load instruction
reg                    pre_fetch_instruction_wen;       // pre-fetch instruction
 
reg     [4:0]          control_state = 5'd0;   //RST_WAIT1
reg     [4:0]          control_state_nxt;
 
 
wire                   dabt;
reg                    dabt_reg = 1'd0;
reg                    dabt_reg_d1;
reg                    iabt_reg = 1'd0;
reg                    adex_reg = 1'd0;
reg     [31:0]         fetch_address_r = 32'd0;
reg     [7:0]          abt_status_reg = 8'd0;
reg     [31:0]         fetch_instruction_r = 32'd0;
reg     [3:0]          fetch_instruction_type_r = 4'd0;
reg     [31:0]         saved_current_instruction = 32'd0;
reg     [3:0]          saved_current_instruction_type = 4'd0;
reg                    saved_current_instruction_iabt = 1'd0;          // access abort flag
reg                    saved_current_instruction_adex = 1'd0;          // address exception
reg     [31:0]         saved_current_instruction_address = 32'd0;       // virtual address of abort instruction
reg     [7:0]          saved_current_instruction_iabt_status = 8'd0;   // status of abort instruction
reg     [31:0]         pre_fetch_instruction = 32'd0;
reg     [3:0]          pre_fetch_instruction_type = 4'd0;
reg                    pre_fetch_instruction_iabt = 1'd0;              // access abort flag
reg                    pre_fetch_instruction_adex = 1'd0;              // address exception
reg     [31:0]         pre_fetch_instruction_address = 32'd0;           // virtual address of abort instruction
reg     [7:0]          pre_fetch_instruction_iabt_status = 8'd0;       // status of abort instruction
reg     [31:0]         hold_instruction = 32'd0;
reg     [3:0]          hold_instruction_type = 4'd0;
reg                    hold_instruction_iabt = 1'd0;                   // access abort flag
reg                    hold_instruction_adex = 1'd0;                   // address exception
reg     [31:0]         hold_instruction_address = 32'd0;                // virtual address of abort instruction
reg     [7:0]          hold_instruction_iabt_status = 8'd0;            // status of abort instruction
 
wire                   instruction_valid;
wire                   instruction_execute;
reg                    instruction_execute_r = 1'd0;
 
reg     [3:0]          mtrans_reg1;             // the current register being accessed as part of stm/ldm
reg     [3:0]          mtrans_reg2;             // the next register being accessed as part of stm/ldm
reg     [31:0]         mtrans_instruction_nxt;
wire    [15:0]         mtrans_reg2_mask;
 
wire   [31:0]          mtrans_base_reg_change;
wire   [4:0]           mtrans_num_registers;
wire                   use_saved_current_instruction;
wire                   use_hold_instruction;
wire                   use_pre_fetch_instruction;
wire                   interrupt;
wire                   interrupt_or_conflict;
wire   [1:0]           interrupt_mode;
wire   [2:0]           next_interrupt;
reg                    irq = 1'd0;
reg                    firq = 1'd0;
wire		       firq_request;
wire                   irq_request;
wire                   swi_request;
wire                   und_request;
wire                   dabt_request;
reg    [1:0]           copro_operation_nxt;
reg                    restore_base_address = 1'd0;
reg                    restore_base_address_nxt;
 
wire                   regop_set_flags;
 
wire    [7:0]          load_rd_nxt;
wire                   load_rd_byte;
wire                   ldm_user_mode;
wire                   ldm_status_bits; 
wire                   ldm_flags; 
wire    [6:0]          load_rd_d1_nxt;
reg     [6:0]          load_rd_d1 = 7'd0;  // MSB is the valid bit
 
wire                   rn_valid;
wire                   rm_valid;
wire                   rs_valid;
wire                   rd_valid;
wire                   stm_valid;
wire                   rn_conflict1;
wire                   rn_conflict2;
wire                   rm_conflict1;
wire                   rm_conflict2;
wire                   rs_conflict1;
wire                   rs_conflict2;
wire                   rd_conflict1;
wire                   rd_conflict2;
wire                   stm_conflict1a;
wire                   stm_conflict1b;
wire                   stm_conflict2a;
wire                   stm_conflict2b;
wire                   conflict1;          // Register conflict1 with ldr operation
wire                   conflict2;          // Register conflict1 with ldr operation
wire                   conflict;           // Register conflict1 with ldr operation
reg                    conflict_r = 1'd0;
reg                    rn_conflict1_r = 1'd0;
reg                    rm_conflict1_r = 1'd0;
reg                    rs_conflict1_r = 1'd0;
reg                    rd_conflict1_r = 1'd0;
wire	[11:0]	       i_fetch; 
 
// ========================================================
// Instruction Abort and Data Abort outputs
// ========================================================


assign o_iabt_trigger     = instruction_iabt && o_status_bits_mode == SVC && control_state == INT_WAIT1;
assign o_iabt_address     = instruction_address;
assign o_iabt_status      = instruction_iabt_status;
 
assign o_dabt_trigger     = dabt_reg && !dabt_reg_d1;
assign o_dabt_address     = fetch_address_r;
assign o_dabt_status      = abt_status_reg;
 
 
// ========================================================
// Instruction Decode
// ========================================================
 
// for instructions that take more than one cycle
// the instruction is saved in the 'saved_mem_instruction'
// register and then that register is used for the rest of
// the execution of the instruction.
// But if the instruction does not execute because of the
// condition, then need to select the next instruction to
// decode
assign use_saved_current_instruction = instruction_execute &&
                          ( control_state == MEM_WAIT1     ||
                            control_state == MEM_WAIT2     ||
                            control_state == MTRANS_EXEC1  ||
                            control_state == MTRANS_EXEC2  || 
                            control_state == MTRANS_ABORT  ||
                            control_state == MULT_PROC1    ||
                            control_state == MULT_PROC2    ||
                            control_state == MULT_ACCUMU   ||
                            control_state == MULT_STORE    ||
                            control_state == INT_WAIT1     ||
                            control_state == INT_WAIT2     ||
                            control_state == SWAP_WRITE    ||
                            control_state == SWAP_WAIT1    ||
                            control_state == SWAP_WAIT2    ||
                            control_state == COPRO_WAIT     );
 
assign use_hold_instruction = conflict_r;
 
assign use_pre_fetch_instruction = control_state == PRE_FETCH_EXEC;
 
 
assign instruction_sel  =         use_hold_instruction           ? 2'd3 :  // hold_instruction
                                  use_saved_current_instruction  ? 2'd1 :  // saved_current_instruction 
                                  use_pre_fetch_instruction      ? 2'd2 :  // pre_fetch_instruction     
                                                                   2'd0 ;  // fetch_instruction_r               
 
assign instruction      =         instruction_sel == 2'd0 ? fetch_instruction_r       :
                                  instruction_sel == 2'd1 ? saved_current_instruction :
                                  instruction_sel == 2'd3 ? hold_instruction          :
                                                            pre_fetch_instruction     ;
 
assign type             =         instruction_sel == 2'd0 ? fetch_instruction_type_r       :
                                  instruction_sel == 2'd1 ? saved_current_instruction_type :
                                  instruction_sel == 2'd3 ? hold_instruction_type          :
                                                            pre_fetch_instruction_type     ;                                                       
 
// abort flag
assign instruction_iabt =         instruction_sel == 2'd0 ? iabt_reg                       :
                                  instruction_sel == 2'd1 ? saved_current_instruction_iabt :
                                  instruction_sel == 2'd3 ? hold_instruction_iabt          :
                                                            pre_fetch_instruction_iabt     ;
 
assign instruction_address =      instruction_sel == 2'd0 ? fetch_address_r                   :
                                  instruction_sel == 2'd1 ? saved_current_instruction_address :
                                  instruction_sel == 2'd3 ? hold_instruction_address          :
                                                            pre_fetch_instruction_address     ;
 
assign instruction_iabt_status =  instruction_sel == 2'd0 ? abt_status_reg                        :
                                  instruction_sel == 2'd1 ? saved_current_instruction_iabt_status :
                                  instruction_sel == 2'd3 ? hold_instruction_iabt_status          :
                                                            pre_fetch_instruction_iabt_status     ;
 
// instruction address exception
assign instruction_adex =         instruction_sel == 2'd0 ? adex_reg                       :
                                  instruction_sel == 2'd1 ? saved_current_instruction_adex :
                                  instruction_sel == 2'd3 ? hold_instruction_adex          :
                                                            pre_fetch_instruction_adex     ;
 
 
// ========================================================
// Fixed fields within the instruction
// ========================================================
 
assign opcode               = instruction[24:21];
assign condition_nxt        = instruction[31:28];
 
assign rm_sel_nxt           = instruction[3:0];
assign rn_sel_nxt           = branch ? 4'd15 : instruction[19:16]; // Use PC to calculate branch destination
assign rs_sel_nxt           = control_state == SWAP_WRITE  ? instruction[3:0]   : // Rm gets written out to memory
                              type == MTRANS               ? mtrans_reg1         :
                              branch                       ? 4'd15              : // Update the PC
                              rds_use_rs                   ? instruction[11:8]  : 
                                                             instruction[15:12] ;
 
// Load from memory into registers
assign ldm_user_mode        = type == MTRANS && {instruction[22:20],instruction[15]} == 4'b1010;
assign ldm_flags            = type == MTRANS && rs_sel_nxt == 4'd15 && instruction[20] && instruction[22];
assign ldm_status_bits      = type == MTRANS && rs_sel_nxt == 4'd15 && instruction[20] && instruction[22] && i_execute_status_bits[1:0] != USR;
assign load_rd_byte         = (type == TRANS || type == SWAP) && instruction[22];
assign load_rd_nxt          = {ldm_flags, ldm_status_bits, ldm_user_mode, load_rd_byte, rs_sel_nxt};
 
 
                            // MSB indicates valid dirty target register
assign load_rd_d1_nxt       = {o_decode_daccess && !o_write_data_wen, o_load_rd[3:0]};
assign shift_imm            = instruction[11:7];
assign offset12             = { 20'h0, instruction[11:0]};
assign offset24             = {{6{instruction[23]}}, instruction[23:0], 2'd0 }; // sign extend
assign imm8                 = instruction[7:0];
 
assign immediate_shift_op   = instruction[25];
assign rds_use_rs           = (type == REGOP && !instruction[25] && instruction[4]) ||
                              (type == MULT && 
                               (control_state == MULT_PROC1  || 
                                control_state == MULT_PROC2  ||
//                                instruction_valid && !interrupt )) ;
// remove the '!conflict' term from the interrupt logic used here
// to break a combinational loop
                                (instruction_valid && !interrupt_or_conflict))) ;
 
 
assign branch               = type == BRANCH;
assign opcode_compare       = opcode == CMP || opcode == CMN || opcode == TEQ || opcode == TST ;
assign mem_op               = type == TRANS;
assign load_op              = mem_op && instruction[20];
assign store_op             = mem_op && !instruction[20];
assign write_pc             = (pc_wen_nxt && pc_sel_nxt != 3'd0) || load_pc_r || load_pc_nxt;
assign current_write_pc     = (pc_wen_nxt && pc_sel_nxt != 3'd0) || load_pc_nxt;
assign regop_set_flags      = type == REGOP && instruction[20]; 
 
assign mem_op_pre_indexed   =  instruction[24] && instruction[21];
assign mem_op_post_indexed  = !instruction[24];
 
assign imm32_nxt            =  // add 0 to Rm
                               type == MULT               ? {  32'd0                      } :   //MULT        = 4'h1, 
 
                               // 4 x number of registers
                               type == MTRANS             ? {  mtrans_base_reg_change     } :   //MTRANS      = 4'h4
                               type == BRANCH             ? {  offset24                   } :   //BRANCH      = 4'h5
                               type == TRANS              ? {  offset12                   } :   //TRANS       = 4'h3
                               instruction[11:8] == 4'h0  ? {            24'd0, imm8[7:0] } :
                               instruction[11:8] == 4'h1  ? { imm8[1:0], 24'd0, imm8[7:2] } :
                               instruction[11:8] == 4'h2  ? { imm8[3:0], 24'd0, imm8[7:4] } :
                               instruction[11:8] == 4'h3  ? { imm8[5:0], 24'd0, imm8[7:6] } :
                               instruction[11:8] == 4'h4  ? { imm8[7:0], 24'd0            } :
                               instruction[11:8] == 4'h5  ? { 2'd0,  imm8[7:0], 22'd0     } :
                               instruction[11:8] == 4'h6  ? { 4'd0,  imm8[7:0], 20'd0     } :
                               instruction[11:8] == 4'h7  ? { 6'd0,  imm8[7:0], 18'd0     } :
                               instruction[11:8] == 4'h8  ? { 8'd0,  imm8[7:0], 16'd0     } :
                               instruction[11:8] == 4'h9  ? { 10'd0, imm8[7:0], 14'd0     } :
                               instruction[11:8] == 4'ha  ? { 12'd0, imm8[7:0], 12'd0     } :
                               instruction[11:8] == 4'hb  ? { 14'd0, imm8[7:0], 10'd0     } :
                               instruction[11:8] == 4'hc  ? { 16'd0, imm8[7:0], 8'd0      } :
                               instruction[11:8] == 4'hd  ? { 18'd0, imm8[7:0], 6'd0      } :
                               instruction[11:8] == 4'he  ? { 20'd0, imm8[7:0], 4'd0      } :
                                                            { 22'd0, imm8[7:0],2'd0      } ;
 
 
assign imm_shift_amount_nxt = shift_imm ;
 
       // This signal is encoded in the decode stage because 
       // it is on the critical path in the execute stage
assign shift_imm_zero_nxt   = imm_shift_amount_nxt == 5'd0 &&       // immediate amount = 0
                              barrel_shift_amount_sel_nxt == 2'd2;  // shift immediate amount
 
assign alu_function_nxt     = { alu_swap_sel_nxt, 
                                alu_not_sel_nxt, 
                                alu_cin_sel_nxt,
                                alu_cout_sel_nxt, 
                                alu_out_sel_nxt  };
 
// ========================================================
// Register Conflict Detection
// ========================================================
assign rn_valid       = type == REGOP || type == MULT || type == SWAP || type == TRANS || type == MTRANS || type == CODTRANS;
assign rm_valid       = type == REGOP || type == MULT || type == SWAP || (type == TRANS && immediate_shift_op);
assign rs_valid       = rds_use_rs;
assign rd_valid       = (type == TRANS  && store_op) || (type == REGOP || type == SWAP);
assign stm_valid      = type == MTRANS && !instruction[20];   // stm instruction
 
 
assign rn_conflict1   = instruction_execute   && rn_valid  && ( load_rd_d1_nxt[4] && rn_sel_nxt         == load_rd_d1_nxt[3:0] );
assign rn_conflict2   = instruction_execute_r && rn_valid  && ( load_rd_d1    [4] && rn_sel_nxt         == load_rd_d1    [3:0] );
assign rm_conflict1   = instruction_execute   && rm_valid  && ( load_rd_d1_nxt[4] && rm_sel_nxt         == load_rd_d1_nxt[3:0] );
assign rm_conflict2   = instruction_execute_r && rm_valid  && ( load_rd_d1    [4] && rm_sel_nxt         == load_rd_d1    [3:0] );
assign rs_conflict1   = instruction_execute   && rs_valid  && ( load_rd_d1_nxt[4] && rs_sel_nxt         == load_rd_d1_nxt[3:0] );
assign rs_conflict2   = instruction_execute_r && rs_valid  && ( load_rd_d1    [4] && rs_sel_nxt         == load_rd_d1    [3:0] );
assign rd_conflict1   = instruction_execute   && rd_valid  && ( load_rd_d1_nxt[4] && instruction[15:12] == load_rd_d1_nxt[3:0] );
assign rd_conflict2   = instruction_execute_r && rd_valid  && ( load_rd_d1    [4] && instruction[15:12] == load_rd_d1    [3:0] );
 
assign stm_conflict1a = instruction_execute   && stm_valid && ( load_rd_d1_nxt[4] && mtrans_reg1        == load_rd_d1_nxt[3:0] ); 
assign stm_conflict1b = instruction_execute   && stm_valid && ( load_rd_d1_nxt[4] && mtrans_reg2        == load_rd_d1_nxt[3:0] ); 
assign stm_conflict2a = instruction_execute_r && stm_valid && ( load_rd_d1    [4] && mtrans_reg1        == load_rd_d1    [3:0] );
assign stm_conflict2b = instruction_execute_r && stm_valid && ( load_rd_d1    [4] && mtrans_reg2        == load_rd_d1    [3:0] );
 
assign conflict1      = instruction_valid &&
                        (rn_conflict1 || rm_conflict1 || rs_conflict1 || rd_conflict1 || 
                         stm_conflict1a || stm_conflict1b);
 
assign conflict2      = instruction_valid && (stm_conflict2a || stm_conflict2b);
 
assign conflict       = conflict1 || conflict2;
 
 
always @( posedge i_clk )
    if ( !i_core_stall )
        begin
        conflict_r              <= conflict;
        instruction_execute_r   <= instruction_execute;
        rn_conflict1_r          <= rn_conflict1 && instruction_execute;
        rm_conflict1_r          <= rm_conflict1 && instruction_execute;
        rs_conflict1_r          <= rs_conflict1 && instruction_execute;
        rd_conflict1_r          <= rd_conflict1 && instruction_execute;
        o_rn_use_read           <= instruction_valid && ( rn_conflict1_r || rn_conflict2 );
        o_rm_use_read           <= instruction_valid && ( rm_conflict1_r || rm_conflict2 );
        o_rs_use_read           <= instruction_valid && ( rs_conflict1_r || rs_conflict2 );
        o_rd_use_read           <= instruction_valid && ( rd_conflict1_r || rd_conflict2 );
        end
 
assign o_conflict = conflict;
 
 
// ========================================================
// MTRANS Operations
// ========================================================
 
   // Bit 15 = r15
   // Bit 0  = r0
   // In ldm and stm instructions r0 is loaded or stored first 
always @*

	if	(instruction[0] == 1'b1)			 mtrans_reg1 = 4'h0;
	else if (instruction[1:0] == 2'b10)			 mtrans_reg1 = 4'h1;
	else if (instruction[2:0] == 3'b100)			 mtrans_reg1 = 4'h2;
	else if (instruction[3:0] == 4'b1000)			 mtrans_reg1 = 4'h3;
	else if (instruction[4:0] == 5'b10000)			 mtrans_reg1 = 4'h4;
	else if (instruction[5:0] == 6'b100000)			 mtrans_reg1 = 4'h5;
	else if (instruction[6:0] == 7'b1000000)		 mtrans_reg1 = 4'h6;
	else if (instruction[7:0] == 8'b10000000)	 	 mtrans_reg1 = 4'h7;
	else if (instruction[8:0] == 9'b100000000)	 	 mtrans_reg1 = 4'h8;
	else if (instruction[9:0] == 10'b1000000000)		 mtrans_reg1 = 4'h9;
	else if (instruction[10:0] == 11'b10000000000)	 	 mtrans_reg1 = 4'ha;
	else if (instruction[11:0] == 12'b100000000000)		 mtrans_reg1 = 4'hb;
	else if (instruction[12:0] == 13'b1000000000000)	 mtrans_reg1 = 4'hc;
	else if (instruction[13:0] == 14'b10000000000000)	 mtrans_reg1 = 4'hd;
	else if (instruction[14:0] == 15'b100000000000000)	 mtrans_reg1 = 4'he;
	else 							 mtrans_reg1 = 4'hf;

//    casez ( instruction[15:0] )
//    16'b???????????????1 : mtrans_reg1 = 4'h0 ;
//    16'b??????????????10 : mtrans_reg1 = 4'h1 ;
//    16'b?????????????100 : mtrans_reg1 = 4'h2 ;
//    16'b????????????1000 : mtrans_reg1 = 4'h3 ;
//    16'b???????????10000 : mtrans_reg1 = 4'h4 ;
//    16'b??????????100000 : mtrans_reg1 = 4'h5 ;
//    16'b?????????1000000 : mtrans_reg1 = 4'h6 ;
//    16'b????????10000000 : mtrans_reg1 = 4'h7 ;
//    16'b???????100000000 : mtrans_reg1 = 4'h8 ;
//    16'b??????1000000000 : mtrans_reg1 = 4'h9 ;
//    16'b?????10000000000 : mtrans_reg1 = 4'ha ;
//    16'b????100000000000 : mtrans_reg1 = 4'hb ;
//    16'b???1000000000000 : mtrans_reg1 = 4'hc ;
//    16'b??10000000000000 : mtrans_reg1 = 4'hd ;
//    16'b?100000000000000 : mtrans_reg1 = 4'he ;
//    default              : mtrans_reg1 = 4'hf ;
//    endcase
 
 

//assign mtrans_reg2_mask = 1'd1<<mtrans_reg1;  //1'd1<<mtrans_reg1
assign mtrans_reg2_mask = mtrans_reg1 == 4'h0 ?  16'b1 :
					 4'h1 ?  16'b10 :
					 4'h2 ?  16'b100 :
					 4'h3 ?  16'b1000 :
					 4'h4 ?  16'b10000 :
					 4'h5 ?  16'b100000 :
					 4'h6 ?  16'b1000000 :
					 4'h7 ?  16'b10000000 :
					 4'h8 ?  16'b100000000 :
					 4'h9 ?  16'b1000000000 :
					 4'ha ?  16'b10000000000 :
					 4'hb ?  16'b100000000000 :
					 4'hc ?  16'b1000000000000 :
					 4'hd ?  16'b10000000000000 :
					 4'he ?  16'b100000000000000 :
					         16'b1000000000000000;



 
always @*
/*    casez ( instruction[15:0] & ~mtrans_reg2_mask )
    16'b???????????????1 : mtrans_reg2 = 4'h0 ;
    16'b??????????????10 : mtrans_reg2 = 4'h1 ;
    16'b?????????????100 : mtrans_reg2 = 4'h2 ;
    16'b????????????1000 : mtrans_reg2 = 4'h3 ;
    16'b???????????10000 : mtrans_reg2 = 4'h4 ;
    16'b??????????100000 : mtrans_reg2 = 4'h5 ;
    16'b?????????1000000 : mtrans_reg2 = 4'h6 ;
    16'b????????10000000 : mtrans_reg2 = 4'h7 ;
    16'b???????100000000 : mtrans_reg2 = 4'h8 ;
    16'b??????1000000000 : mtrans_reg2 = 4'h9 ;
    16'b?????10000000000 : mtrans_reg2 = 4'ha ;
    16'b????100000000000 : mtrans_reg2 = 4'hb ;
    16'b???1000000000000 : mtrans_reg2 = 4'hc ;
    16'b??10000000000000 : mtrans_reg2 = 4'hd ;
    16'b?100000000000000 : mtrans_reg2 = 4'he ;
    default              : mtrans_reg2 = 4'hf ;
   endcase
*/
	if	(~mtrans_reg2_mask & instruction[0] == 1'b1)				 mtrans_reg2 = 4'h0 ;
	else if (~mtrans_reg2_mask & instruction[1:0] == 2'b10)				 mtrans_reg2 = 4'h1 ;
	else if (~mtrans_reg2_mask & instruction[2:0] == 3'b100)			 mtrans_reg2 = 4'h2 ;
	else if (~mtrans_reg2_mask & instruction[3:0] == 4'b1000)			 mtrans_reg2 = 4'h3 ;
	else if (~mtrans_reg2_mask & instruction[4:0] == 5'b10000)			 mtrans_reg2 = 4'h4 ;
	else if (~mtrans_reg2_mask & instruction[5:0] == 6'b100000)			 mtrans_reg2 = 4'h5 ;
	else if (~mtrans_reg2_mask & instruction[6:0] == 7'b1000000)			 mtrans_reg2 = 4'h6 ;
	else if (~mtrans_reg2_mask & instruction[7:0] == 8'b10000000)	 		 mtrans_reg2 = 4'h7 ;
	else if (~mtrans_reg2_mask & instruction[8:0] == 9'b100000000)	 		 mtrans_reg2 = 4'h8 ;
	else if (~mtrans_reg2_mask & instruction[9:0] == 10'b1000000000)		 mtrans_reg2 = 4'h9 ;
	else if (~mtrans_reg2_mask & instruction[10:0] == 11'b10000000000)	 	 mtrans_reg2 = 4'ha ;
	else if (~mtrans_reg2_mask & instruction[11:0] == 12'b100000000000)		 mtrans_reg2 = 4'hb ;
	else if (~mtrans_reg2_mask & instruction[12:0] == 13'b1000000000000)		 mtrans_reg2 = 4'hc ;
	else if (~mtrans_reg2_mask & instruction[13:0] == 14'b10000000000000)		 mtrans_reg2 = 4'hd ;
	else if (~mtrans_reg2_mask & instruction[14:0] == 15'b100000000000000)		 mtrans_reg2 = 4'he ;
	else 										 mtrans_reg2 = 4'hf ;

 
always @( * )
//    casez (instruction[15:0])
//    16'b???????????????1 : mtrans_instruction_nxt = {instruction[31:16], instruction[15: 1],  1'd0};
//    16'b??????????????10 : mtrans_instruction_nxt = {instruction[31:16], instruction[15: 2],  2'd0}; 
//    16'b?????????????100 : mtrans_instruction_nxt = {instruction[31:16], instruction[15: 3],  3'd0}; 
//    16'b????????????1000 : mtrans_instruction_nxt = {instruction[31:16], instruction[15: 4],  4'd0}; 
//    16'b???????????10000 : mtrans_instruction_nxt = {instruction[31:16], instruction[15: 5],  5'd0}; 
//    16'b??????????100000 : mtrans_instruction_nxt = {instruction[31:16], instruction[15: 6],  6'd0}; 
//    16'b?????????1000000 : mtrans_instruction_nxt = {instruction[31:16], instruction[15: 7],  7'd0}; 
//    16'b????????10000000 : mtrans_instruction_nxt = {instruction[31:16], instruction[15: 8],  8'd0}; 
//    16'b???????100000000 : mtrans_instruction_nxt = {instruction[31:16], instruction[15: 9],  9'd0}; 
//    16'b??????1000000000 : mtrans_instruction_nxt = {instruction[31:16], instruction[15:10], 10'd0}; 
//    16'b?????10000000000 : mtrans_instruction_nxt = {instruction[31:16], instruction[15:11], 11'd0}; 
//    16'b????100000000000 : mtrans_instruction_nxt = {instruction[31:16], instruction[15:12], 12'd0}; 
//    16'b???1000000000000 : mtrans_instruction_nxt = {instruction[31:16], instruction[15:13], 13'd0}; 
//    16'b??10000000000000 : mtrans_instruction_nxt = {instruction[31:16], instruction[15:14], 14'd0}; 
//    16'b?100000000000000 : mtrans_instruction_nxt = {instruction[31:16], instruction[15   ], 15'd0}; 
//   default              : mtrans_instruction_nxt = {instruction[31:16],                     16'd0}; 
//    endcase


	if	(instruction[0] == 1'b1)			 mtrans_instruction_nxt = {instruction[31:16], instruction[15: 1],  1'd0};
	else if (instruction[1:0] == 2'b10)			 mtrans_instruction_nxt = {instruction[31:16], instruction[15: 2],  2'd0}; 
	else if (instruction[2:0] == 3'b100)			 mtrans_instruction_nxt = {instruction[31:16], instruction[15: 3],  3'd0}; 
	else if (instruction[3:0] == 4'b1000)			 mtrans_instruction_nxt = {instruction[31:16], instruction[15: 4],  4'd0}; 
	else if (instruction[4:0] == 5'b10000)			 mtrans_instruction_nxt = {instruction[31:16], instruction[15: 5],  5'd0}; 
	else if (instruction[5:0] == 6'b100000)			 mtrans_instruction_nxt = {instruction[31:16], instruction[15: 6],  6'd0}; 
	else if (instruction[6:0] == 7'b1000000)		 mtrans_instruction_nxt = {instruction[31:16], instruction[15: 7],  7'd0}; 
	else if (instruction[7:0] == 8'b10000000)	 	 mtrans_instruction_nxt = {instruction[31:16], instruction[15: 8],  8'd0}; 
	else if (instruction[8:0] == 9'b100000000)	 	 mtrans_instruction_nxt = {instruction[31:16], instruction[15: 9],  9'd0}; 
	else if (instruction[9:0] == 10'b1000000000)		 mtrans_instruction_nxt = {instruction[31:16], instruction[15:10], 10'd0}; 
	else if (instruction[10:0] == 11'b10000000000)	 	 mtrans_instruction_nxt = {instruction[31:16], instruction[15:11], 11'd0}; 
	else if (instruction[11:0] == 12'b100000000000)		 mtrans_instruction_nxt = {instruction[31:16], instruction[15:12], 12'd0}; 
	else if (instruction[12:0] == 13'b1000000000000)	 mtrans_instruction_nxt = {instruction[31:16], instruction[15:13], 13'd0}; 
	else if (instruction[13:0] == 14'b10000000000000)	 mtrans_instruction_nxt = {instruction[31:16], instruction[15:14], 14'd0}; 
	else if (instruction[14:0] == 15'b100000000000000)	 mtrans_instruction_nxt = {instruction[31:16], instruction[  15 ], 15'd0}; 
	else 							 mtrans_instruction_nxt = {instruction[31:16], 16'd0};
 
 
// number of registers to be stored
assign mtrans_num_registers =   {4'd0, instruction[15]} + 
                                {4'd0, instruction[14]} + 
                                {4'd0, instruction[13]} + 
                                {4'd0, instruction[12]} +
                                {4'd0, instruction[11]} +
                                {4'd0, instruction[10]} +
                                {4'd0, instruction[ 9]} +
                                {4'd0, instruction[ 8]} +
                                {4'd0, instruction[ 7]} +
                                {4'd0, instruction[ 6]} +
                                {4'd0, instruction[ 5]} +
                                {4'd0, instruction[ 4]} +
                                {4'd0, instruction[ 3]} +
                                {4'd0, instruction[ 2]} +
                                {4'd0, instruction[ 1]} +
                                {4'd0, instruction[ 0]} ;
 
// 4 x number of registers to be stored
assign mtrans_base_reg_change = {25'd0, mtrans_num_registers, 2'd0};
 
// ========================================================
// Interrupts
// ========================================================
 
assign firq_request = firq && !i_execute_status_bits[26];
assign irq_request  = irq  && !i_execute_status_bits[27];
assign swi_request  = type == SWI;
assign dabt_request = dabt_reg;
 
// copro15 and copro13 only supports reg trans opcodes
// all other opcodes involving co-processors cause an 
// undefined instrution interrupt
assign und_request  =   type == CODTRANS || 
                        type == COREGOP  || 
                      ( type == CORTRANS && instruction[11:8] != 4'd15 );
 
 
  // in order of priority !!                 
  // Highest 
  // 1 Reset
  // 2 Data Abort (including data TLB miss)
  // 3 FIRQ
  // 4 IRQ
  // 5 Prefetch Abort (including prefetch TLB miss)
  // 6 Undefined instruction, SWI
  // Lowest                        
assign next_interrupt = dabt_request     ? 3'd1 :  // Data Abort
                        firq_request     ? 3'd2 :  // FIRQ
                        irq_request      ? 3'd3 :  // IRQ
                        instruction_adex ? 3'd4 :  // Address Exception 
                        instruction_iabt ? 3'd5 :  // PreFetch Abort, only triggered 
                                                   // if the instruction is used
                        und_request      ? 3'd6 :  // Undefined Instruction
                        swi_request      ? 3'd7 :  // SWI
                                           3'd0 ;  // none             
 
 
// SWI and undefined instructions do not cause an interrupt in the decode
// stage. They only trigger interrupts if they arfe executed, so the
// interrupt is triggered if the execute condition is met in the execute stage
assign interrupt      = next_interrupt != 3'd0 && 
                        next_interrupt != 3'd7 &&  // SWI
                        next_interrupt != 3'd6 &&  // undefined interrupt
                        !conflict               ;  // Wait for conflicts to resolve before
                                                   // triggering int
 
 
// Added to use in rds_use_rs logic to break a combinational loop invloving
// the conflict signal
assign interrupt_or_conflict 
                     =  next_interrupt != 3'd0 && 
                        next_interrupt != 3'd7 &&  // SWI
                        next_interrupt != 3'd6  ;  // undefined interrupt
 
assign interrupt_mode = next_interrupt == 3'd2 ? FIRQ :
                        next_interrupt == 3'd3 ? IRQ  :
                        next_interrupt == 3'd4 ? SVC  :
                        next_interrupt == 3'd5 ? SVC  :
                        next_interrupt == 3'd6 ? SVC  :
                        next_interrupt == 3'd7 ? SVC  :
                        next_interrupt == 3'd1 ? SVC  :
                                                 USR  ;
 
always @*
	begin
		if ( instruction_valid && !interrupt && !conflict )
		begin
			if ( type == SWI || und_request )
				status_bits_mode_nxt          = interrupt_mode;
			else 
				status_bits_mode_nxt            = i_execute_status_bits[1:0];
		end
 
		else if ( instruction_valid && interrupt &&  next_interrupt != 3'd6 )
			status_bits_mode_nxt            = interrupt_mode;

		else if ( control_state == INT_WAIT1 ) 
        		status_bits_mode_nxt            = o_status_bits_mode;   // Supervisor mode

		else 
			status_bits_mode_nxt            = i_execute_status_bits[1:0];
	end



always @*
	begin
		if ( instruction_valid && !interrupt && !conflict )
		begin
			if ( type == SWI || und_request )
				status_bits_irq_mask_nxt        = 1'd1;
			else
				status_bits_irq_mask_nxt        = o_status_bits_irq_mask;
		end

		else if ( instruction_valid && interrupt &&  next_interrupt != 3'd6 )
			status_bits_irq_mask_nxt        = 1'd1;

		else 
			status_bits_irq_mask_nxt        = o_status_bits_irq_mask;

	end


always @*
	begin
		if ( instruction_valid && interrupt &&  next_interrupt != 3'd6 && next_interrupt == 3'd2)
			status_bits_firq_mask_nxt        = 1'd1;
			
		else
			status_bits_firq_mask_nxt       = o_status_bits_firq_mask;
	end


always @*
	begin
		if ( instruction_valid && !interrupt && !conflict &&(type == SWAP))		
			decode_exclusive_nxt            = 1'd1;
		else
			decode_exclusive_nxt            = 1'd0;
	end


always @*
	begin
		if ( instruction_valid && !interrupt && !conflict )
		begin
			if ( mem_op )
				decode_daccess_nxt              = 1'd1;
			else if ( type == MTRANS )
				decode_daccess_nxt              = 1'd1; 
			else if ( type == SWAP )
				decode_daccess_nxt              = 1'd1;
			else
				decode_daccess_nxt              = 1'd0;
		end
		else if ( control_state == MTRANS_EXEC1 && !conflict &&instruction_execute)		 
			decode_daccess_nxt          = 1'd1;

		else if ( control_state == MTRANS_EXEC2 )
			decode_daccess_nxt          = 1'd1;

		else if ( control_state == SWAP_WRITE && instruction_execute && !conflict )
			decode_daccess_nxt          = 1'd1;
		else
			decode_daccess_nxt             = 1'd0;
	end

always @*
	begin
		if ( instruction_valid && !interrupt && !conflict && type == SWAP)
			decode_iaccess_nxt              = 1'd0;

		else if ( control_state == MTRANS_EXEC1 && !conflict && instruction_execute &&( mtrans_num_registers > 4'd2 ))
			decode_iaccess_nxt      = 1'd0;  
		
		else if ( control_state == MTRANS_EXEC2 && ( mtrans_num_registers > 4'd2 ))	
			decode_iaccess_nxt      = 1'd0;  

		else if ( control_state == SWAP_WRITE && instruction_execute && !conflict )
			decode_iaccess_nxt              = 1'd0;

		else 	
			decode_iaccess_nxt              = 1'd1;
	end


always @*
	begin
		if ( instruction_valid && !interrupt && !conflict )
		begin
			if ( type == CORTRANS && !und_request )
			begin
				if ( instruction[20] )
					copro_operation_nxt         = 2'd1; 
				else
					copro_operation_nxt      = 2'd0;
			end
			else 
				copro_operation_nxt             = 2'd0;
		end
			
		else if ( control_state == COPRO_WAIT && instruction_execute && !conflict && !instruction[20])
			copro_operation_nxt      = 2'd2; 

		else
			copro_operation_nxt             = 2'd0;
	end


always @*
	begin
		if ( instruction_valid && !interrupt && !conflict )
		begin
			if ( mem_op && (load_op && instruction[15:12]  == 4'd15))
				saved_current_instruction_wen   = 1'd1; 
			else if ( type == MTRANS )
				saved_current_instruction_wen   = 1'd1;
			else if ( type == MULT )
				saved_current_instruction_wen   = 1'd1;
			else if ( type == SWAP )
				saved_current_instruction_wen   = 1'd1;
			else  if ( type == CORTRANS && !und_request )
				saved_current_instruction_wen   = 1'd1;
			else
				saved_current_instruction_wen   = 1'd0;
		end
		
		else if ( instruction_valid && interrupt &&  next_interrupt != 3'd6 )
			saved_current_instruction_wen   = 1'd1; 
		
		else if ( control_state == MTRANS_EXEC1 && !conflict && instruction_execute &&( instruction[20] && mtrans_reg1 == 4'd15 ))
			saved_current_instruction_wen   = 1'd1; 
		
		else if ( control_state == MTRANS_EXEC2 && ( instruction[20] && mtrans_reg1 == 4'd15 ))
			saved_current_instruction_wen   = 1'd1;		
		
		else
			saved_current_instruction_wen   = 1'd0;
	end

always @*
	begin
		if ( control_state == MEM_WAIT1 && !conflict )
			pre_fetch_instruction_wen   = 1'd1;
		else if ( control_state == MTRANS_EXEC1 && !conflict )
		        pre_fetch_instruction_wen   = 1'd1;
		else if ( control_state == MULT_PROC1 && instruction_execute && !conflict )
			pre_fetch_instruction_wen   = 1'd1;
		else if ( control_state == SWAP_WRITE && instruction_execute && !conflict )
			pre_fetch_instruction_wen   = 1'd1;
		else if ( control_state == COPRO_WAIT && instruction_execute && !conflict )
			pre_fetch_instruction_wen   = 1'd1;
		else
			pre_fetch_instruction_wen   = 1'd0;
	end

always @*
	begin
		if ( instruction_valid && !interrupt && !conflict && type == MTRANS)
		//     restore_base_address_nxt        <= instruction[20] && 
       		//                                         (instruction[15:0] & (1'd1 << instruction[19:16]));
			restore_base_address_nxt        = instruction[20] && 
                                                		(instruction[15:0] & (
								instruction[19:16] == 4'h1 ? 16'b10:
								instruction[19:16] == 4'h2 ? 16'b100:
								instruction[19:16] == 4'h3 ? 16'b1000:
								instruction[19:16] == 4'h4 ? 16'b10000:
								instruction[19:16] == 4'h5 ? 16'b100000:
								instruction[19:16] == 4'h6 ? 16'b1000000:
								instruction[19:16] == 4'h7 ? 16'b10000000:
								instruction[19:16] == 4'h8 ? 16'b100000000:
								instruction[19:16] == 4'h9 ? 16'b1000000000:
								instruction[19:16] == 4'ha ? 16'b10000000000:
								instruction[19:16] == 4'hb ? 16'b100000000000:
								instruction[19:16] == 4'hc ? 16'b1000000000000:
								instruction[19:16] == 4'hd ? 16'b10000000000000:
								instruction[19:16] == 4'he ? 16'b100000000000000:
								instruction[19:16] == 4'hf ? 16'b1000000000000000:
											     16'b1));
		else
			restore_base_address_nxt        = restore_base_address;
	end

always @*
	begin
		if ( instruction_valid && !interrupt && !conflict )
		begin
			if ( type == REGOP )
			begin
				if ( !immediate_shift_op && instruction[4] )
					barrel_shift_amount_sel_nxt = 2'd1;
				else if ( !immediate_shift_op && !instruction[4] ) 
                			barrel_shift_amount_sel_nxt = 2'd2; 	
				else
					barrel_shift_amount_sel_nxt = 2'd0; 
			end
			else if ( mem_op && ( type == TRANS && instruction[25] && shift_imm != 5'd0 ) )
				barrel_shift_amount_sel_nxt = 2'd2;
			else
				barrel_shift_amount_sel_nxt = 2'd0; 
		end
		else
			barrel_shift_amount_sel_nxt = 2'd0; 
	end

always @*
	begin
		if ( instruction_valid && !interrupt && !conflict )
		begin
			if(type == REGOP &&  !immediate_shift_op)
				barrel_shift_data_sel_nxt = 2'd2;
			
			else if (mem_op && instruction[25] && type ==  TRANS)
				barrel_shift_data_sel_nxt = 2'd2;
			else if ( type == SWAP )
				barrel_shift_data_sel_nxt = 2'd2;
			else
				barrel_shift_data_sel_nxt = 2'd0;
		end
		else if ( control_state == SWAP_WRITE && instruction_execute && !conflict )
			barrel_shift_data_sel_nxt       = 2'd2;
		else
			barrel_shift_data_sel_nxt       = 2'd0;
	end

always @*
	begin
		if ( instruction_valid && !interrupt && !conflict )	
		begin
			if ( type == REGOP && !immediate_shift_op)
				barrel_shift_function_nxt  = instruction[6:5];
			else if ( mem_op && type == TRANS && instruction[25] && shift_imm != 5'd0 )
				barrel_shift_function_nxt   = instruction[6:5];
			else 
				barrel_shift_function_nxt       = 2'd0;
		end
		else
			barrel_shift_function_nxt       = 2'd0;
	end

always @*
	begin
		if ( instruction_valid && !interrupt && !conflict && type == MULT)
		begin	
			multiply_function_nxt[0]        = 1'd1;
			if( instruction[21] )
				multiply_function_nxt[1]    = 1'd1;
			else
				multiply_function_nxt[1]    = 1'd0;
		end
		else if ( control_state == MULT_PROC1 && instruction_execute && !conflict )
			multiply_function_nxt       = o_multiply_function;
		else if ( control_state == MULT_PROC2 )
			multiply_function_nxt   = o_multiply_function;
		else if ( control_state == MULT_STORE )
			multiply_function_nxt = o_multiply_function;
		else if ( control_state == MULT_ACCUMU )
			multiply_function_nxt = o_multiply_function;
		else
			multiply_function_nxt           = 2'd0;
	end



always @*
	begin
		if ( instruction_valid && !interrupt && !conflict )
		begin
			if( type == REGOP && !opcode_compare && instruction[15:12]  == 4'd15)
				iaddress_sel_nxt      = 4'd1;
			else if ( type == BRANCH )
				iaddress_sel_nxt      = 4'd1; 
			else if ( type == MTRANS && mtrans_num_registers > 4'd1)
				iaddress_sel_nxt      = 4'd3;
			else if ( type == CORTRANS && !und_request )
				iaddress_sel_nxt      = 4'd3;
			else if ( type == SWI || und_request )
				iaddress_sel_nxt      = 4'd2;
			else
				iaddress_sel_nxt      = 4'd0;

		end
		else if ( instruction_valid && interrupt &&  next_interrupt != 3'd6 )
			iaddress_sel_nxt                = 4'd2; 
			 
		else if ( control_state == MEM_WAIT1 && !conflict &&instruction_execute)
			iaddress_sel_nxt            = 4'd3;
		
		else if ( control_state == MEM_WAIT2 && !dabt  && (( type == TRANS && instruction[15:12]  == 4'd15 ) || 
               						 ( type == MTRANS && instruction[20] && mtrans_reg1 == 4'd15 )))

			iaddress_sel_nxt = 4'd3;

		else  if ( control_state == MTRANS_EXEC1 && !conflict && instruction_execute && mtrans_num_registers != 4'd1)
			
			iaddress_sel_nxt        = 4'd3; 
		
		else if ( control_state == MTRANS_EXEC2  && mtrans_num_registers > 4'd1)		
			iaddress_sel_nxt        = 4'd3; 
		else if ( control_state == MULT_PROC2 )
			iaddress_sel_nxt        = 4'd3;
		else if ( control_state == MULT_ACCUMU )
			iaddress_sel_nxt        = 4'd3;
		else if ( control_state == SWAP_WAIT1  && instruction_execute) 
			iaddress_sel_nxt            = 4'd3;
		else if ( control_state == SWAP_WAIT1  && !dabt && instruction[15:12]  == 4'd15)
			iaddress_sel_nxt = 4'd3; 
		else 
		 	iaddress_sel_nxt = 4'd0;
	end 


always @*
	begin
		if ( instruction_valid && !interrupt && !conflict )
		begin
			if ( mem_op )
			begin
				if ( mem_op_post_indexed )
					daddress_sel_nxt = 4'd4; // Rn
            			else   
               				daddress_sel_nxt = 4'd1; // alu out
			end	
			else if ( type == MTRANS )	
				if ( instruction[23] )
				begin
					if ( instruction[24] )    // increment before
                    				daddress_sel_nxt = 4'd7; // Rn + 4
               				 else    
                    				daddress_sel_nxt = 4'd4; // Rn
                		end
				else
				begin
					if ( !instruction[24] )    // decrement after
                    				daddress_sel_nxt  = 4'd6; // alu out + 4
                			else
                    				daddress_sel_nxt  = 4'd1; // alu out
                		end
			else if	( type == SWAP )
				daddress_sel_nxt                = 4'd4; 
			else
				daddress_sel_nxt                = 4'd0;
		end
		else if ( control_state == MTRANS_EXEC1 && !conflict && instruction_execute )
			daddress_sel_nxt            = 4'd5;
		else if ( control_state == MTRANS_EXEC2 )
			daddress_sel_nxt            = 4'd5;	
		else if ( control_state == SWAP_WRITE && instruction_execute && !conflict )
			daddress_sel_nxt            = 4'd4;
		else
			daddress_sel_nxt            = 4'd0;	
	end


always @*
	begin
		if ( instruction_valid && !interrupt && !conflict )
		begin
			if ( type == REGOP && !opcode_compare && instruction[15:12]  == 4'd15 )	
				pc_sel_nxt       = 3'd1;
			else if ( mem_op && (mem_op_pre_indexed || mem_op_post_indexed) && rn_sel_nxt  == 4'd15 )	
				pc_sel_nxt       = 3'd1; 
			else if ( type == BRANCH )
				pc_sel_nxt       = 3'd1; 
			else if ( type == SWI || und_request )
				pc_sel_nxt       = 3'd2; 
			else
				pc_sel_nxt       = 3'd0; 
		end
		else if ( instruction_valid && interrupt &&  next_interrupt != 3'd6 )
			pc_sel_nxt                      = 3'd2; 
		else if ( control_state == MEM_WAIT2  && !dabt &&  (( type == TRANS && instruction[15:12]  == 4'd15 ) || 
                					( type == MTRANS && instruction[20] && mtrans_reg1 == 4'd15 )))
			pc_sel_nxt       = 3'd3;
		
		else if ( control_state == SWAP_WAIT1 && !dabt && instruction[15:12]  == 4'd15 ) 
			
			pc_sel_nxt       = 3'd3;
		else
			pc_sel_nxt       = 3'd0;
	end

		
always @*
	begin
		if ( instruction_valid && !interrupt && !conflict )
		begin
			if ( mem_op && ( load_op && instruction[15:12]  == 4'd15 ))
				load_pc_nxt                     = 1'd1;
			else if ( type == MTRANS && ( instruction[20] && mtrans_reg1 == 4'd15 ))
				load_pc_nxt                     = 1'd1;
			else
				load_pc_nxt                     = 1'd0;
		end
		else if ( control_state == MEM_WAIT1 && !conflict && instruction_execute)
			load_pc_nxt                 = load_pc_r;
		else if ( control_state == MEM_WAIT2 && !dabt && (( type == TRANS && instruction[15:12]  == 4'd15 ) || 
               							 ( type == MTRANS && instruction[20] && mtrans_reg1 == 4'd15 )))
			load_pc_nxt      = load_pc_r;
		
		else if ( control_state == MTRANS_EXEC1 && !conflict && instruction_execute && ( instruction[20] && mtrans_reg1 == 4'd15 ))
			load_pc_nxt                     = 1'd1;
		else if ( control_state == MTRANS_EXEC2 && ( instruction[20] && mtrans_reg1 == 4'd15 ))
			load_pc_nxt                     = 1'd1;
		else if ( control_state == SWAP_WRITE && instruction_execute && !conflict )
			load_pc_nxt                     = load_pc_r;
		else if ( control_state == SWAP_WAIT1 && !dabt && instruction[15:12]  == 4'd15 )
 			load_pc_nxt      = load_pc_r;
		else
			load_pc_nxt                     = 1'd0;
	end

always @*
	begin
		if ( instruction_valid && !interrupt && !conflict )
		begin
			if ( mem_op && store_op	&& type == TRANS && instruction[22] )
				 byte_enable_sel_nxt = 2'd1; 
			else
				 byte_enable_sel_nxt = 2'd0;
		end
		else if ( control_state == SWAP_WRITE && instruction_execute && !conflict && instruction[22])
			byte_enable_sel_nxt = 2'd1; 
		else
			byte_enable_sel_nxt = 2'd0;
	end 

always @*
	begin
		if ( instruction_valid && !interrupt && !conflict && regop_set_flags && instruction[15:12] == 4'd15)
			status_bits_sel_nxt       = 3'd1; 				
		else if ( control_state == MULT_STORE && instruction[20])
			status_bits_sel_nxt       = 3'd4;
		else if ( control_state == COPRO_WAIT && instruction_execute && !conflict && instruction[20] && instruction[15:12]  == 4'd15 )
			status_bits_sel_nxt       = 3'd3;
		else
			status_bits_sel_nxt       = 3'd0;
	end


always @*
	begin
		if ( instruction_valid && !interrupt && !conflict )
		begin
			if ( type == BRANCH && instruction[24] )
				reg_write_sel_nxt 	= 3'd1;
			else if ( type == SWI || und_request )
				reg_write_sel_nxt       = 3'd1; 
			else
				reg_write_sel_nxt       = 3'd0;

		end
		else if ( instruction_valid && interrupt &&  next_interrupt != 3'd6 )
		begin
			if ( next_interrupt == 3'd4 )
          			reg_write_sel_nxt               = 3'd7;   
       			 else
           		 	reg_write_sel_nxt               = 3'd1;    
		end
		else if ( control_state == MTRANS_ABORT && restore_base_address )
			reg_write_sel_nxt     = 3'd6;             
		else if ( control_state == MULT_STORE )
			reg_write_sel_nxt     = 3'd2;
		else if ( control_state == COPRO_WAIT && instruction_execute && !conflict && instruction[20])	
 			reg_write_sel_nxt     = 3'd5;  
		else
			reg_write_sel_nxt     = 3'd0;
	end


always @*
	begin
		if ( instruction_valid && !interrupt && !conflict && type == MTRANS && instruction[22:20] == 3'b100 )
			o_user_mode_regs_store_nxt = 1'd1;
		else if ( control_state == MTRANS_EXEC1 && !conflict && instruction_execute && instruction[22:20] == 3'b100 )
			o_user_mode_regs_store_nxt = 1'd1;
		else if ( control_state == MTRANS_EXEC2 && instruction[22:20] == 3'b100 )
			o_user_mode_regs_store_nxt = 1'd1;
		else
			o_user_mode_regs_store_nxt = 1'd0;
	end


always @*
	begin
		if ( instruction_valid && !interrupt && !conflict && type == REGOP )
		begin
			if ( opcode == RSB )
				alu_swap_sel_nxt = 1'd1;
			else if ( opcode == RSC )
				alu_swap_sel_nxt = 1'd1;
			else
				alu_swap_sel_nxt = 1'd0;
		end
		else
			alu_swap_sel_nxt = 1'd0;
	end

always @*
	begin
		if ( instruction_valid && !interrupt && !conflict )
		begin
			if( type == REGOP )
			begin
				if ( opcode == SUB || opcode == CMP ) 
					alu_not_sel_nxt  = 1'd1;
				else if ( opcode == SBC )
					alu_not_sel_nxt  = 1'd1;
				else if ( opcode == RSB )
					alu_not_sel_nxt  = 1'd1;
				else if ( opcode == RSC )
					alu_not_sel_nxt  = 1'd1;
				else if ( opcode == BIC )
					alu_not_sel_nxt  = 1'd1;
				else if ( opcode == MVN )
					alu_not_sel_nxt  = 1'd1;
				else 
					alu_not_sel_nxt  = 1'd0;
			end
			else if ( mem_op &&  !instruction[23])
				alu_not_sel_nxt  = 1'd1;
			else if ( type == MTRANS && !instruction[23])
				alu_not_sel_nxt  = 1'd1;
			else
				alu_not_sel_nxt  = 1'd0;
		end
		else
			alu_not_sel_nxt  = 1'd0;
	end


always @*
	begin
		if ( instruction_valid && !interrupt && !conflict )
		begin
			if( type == REGOP )
			begin
				if ( opcode == ADC ) 
					alu_cin_sel_nxt  = 2'd2;
				else if ( opcode == SUB || opcode == CMP ) 
					alu_cin_sel_nxt  = 2'd1; 
				else if ( opcode == SBC )
					alu_cin_sel_nxt  = 2'd2;
				else if ( opcode == RSB )
					alu_cin_sel_nxt  = 2'd1; 
				else if ( opcode == RSC )
					alu_cin_sel_nxt  = 2'd2; 
				else
					alu_cin_sel_nxt  = 2'd0;
			end
			else if ( mem_op && !instruction[23])
				alu_cin_sel_nxt  = 2'd1; 
			else if ( type == MTRANS && !instruction[23])
				alu_cin_sel_nxt  = 2'd1;
			else
				alu_cin_sel_nxt  = 2'd0;
		end
		else
			alu_cin_sel_nxt  = 2'd0;
	end

always @*
	begin
		if ( instruction_valid && !interrupt && !conflict && type == REGOP )
		begin			
			if ( opcode == AND || opcode == TST )
				alu_cout_sel_nxt = 1'd1;
			else if ( opcode == EOR || opcode == TEQ )
				alu_cout_sel_nxt = 1'd1;
			else if ( opcode == ORR )
				alu_cout_sel_nxt = 1'd1; 
			else if ( opcode == BIC )
				alu_cout_sel_nxt = 1'd1; 
			else if ( opcode == MOV )
				alu_cout_sel_nxt = 1'd1; 
			else if ( opcode == MVN )
				alu_cout_sel_nxt = 1'd1;			
			else	
				alu_cout_sel_nxt = 1'd0;
		end
		else 
			alu_cout_sel_nxt = 1'd0;
	end


always @*
	begin
		if ( instruction_valid && !interrupt && !conflict )
		begin
			if( type == REGOP )
			begin
				if ( opcode == ADD || opcode == CMN )
					alu_out_sel_nxt  = 4'd1; 
				else if ( opcode == ADC )
					alu_out_sel_nxt  = 4'd1;
				else if ( opcode == SUB || opcode == CMP )
					alu_out_sel_nxt  = 4'd1; 
				else if ( opcode == SBC ) 
					alu_out_sel_nxt  = 4'd1;
				else if ( opcode == RSB )
					alu_out_sel_nxt  = 4'd1;
				else if ( opcode == RSC )
					alu_out_sel_nxt  = 4'd1; 
				else if ( opcode == AND || opcode == TST )
					alu_out_sel_nxt  = 4'd8;
				else if ( opcode == EOR || opcode == TEQ ) 
					alu_out_sel_nxt  = 4'd6;
				else if ( opcode == ORR )
					alu_out_sel_nxt  = 4'd7;
				else if ( opcode == BIC )
					alu_out_sel_nxt  = 4'd8;	
				else
					alu_out_sel_nxt  = 4'd0;
			end
			else if ( mem_op )
				alu_out_sel_nxt          = 4'd1;
			else if ( type == BRANCH )
				alu_out_sel_nxt          = 4'd1;
			else if ( type == MTRANS )
				alu_out_sel_nxt          = 4'd1;
			else
				alu_out_sel_nxt          = 4'd0;
		end
		else
			alu_out_sel_nxt          = 4'd0;
	end

always @*
	begin
		if ( instruction_valid && !interrupt && !conflict )
		begin
			if ( mem_op && store_op )
				write_data_wen_nxt = 1'd1;
			else if ( type == MTRANS && !instruction[20] )
				write_data_wen_nxt = 1'd1; 
			else
				write_data_wen_nxt = 1'd0;
		end
		else if ( control_state == MTRANS_EXEC1 && !conflict && instruction_execute && !instruction[20] )
			write_data_wen_nxt = 1'd1;
		else if ( control_state == MTRANS_EXEC2 && !instruction[20] )
			write_data_wen_nxt = 1'd1;
		else if ( control_state == SWAP_WRITE && instruction_execute && !conflict )
			write_data_wen_nxt = 1'd1;
		else
			write_data_wen_nxt = 1'd0;
	end

always @*
	begin		
		if(instruction_valid && !interrupt && !conflict && (type == CORTRANS && !und_request) && ! instruction[20])
			copro_write_data_wen_nxt = 1'd1;
		else
			copro_write_data_wen_nxt = 1'd0;
	end

always @*
	begin		
		if(instruction_valid && !interrupt && !conflict && type == MTRANS)
			base_address_wen_nxt            = 1'd1;			
		else
			base_address_wen_nxt            = 1'd0;
	end

always @*
	begin
		if ( instruction_valid && !interrupt && !conflict )
		begin
			if ( mem_op &&(load_op && instruction[15:12]  == 4'd15) )
				pc_wen_nxt        = 1'd0;
			else if ( type == MTRANS )
			begin
				if ( mtrans_num_registers > 4'd1 )
					pc_wen_nxt              = 1'd0;
				else if ( instruction[20] && mtrans_reg1 == 4'd15 ) 
					pc_wen_nxt                = 1'd0;
				else 
					pc_wen_nxt                = 1'd1;
			end
			else if ( type == MULT )
				pc_wen_nxt                      = 1'd0;
			else if ( type == SWAP )
				pc_wen_nxt                      = 1'd0;
			else if ( type == CORTRANS && !und_request )
				pc_wen_nxt                      = 1'd0;
			else
				pc_wen_nxt                      = 1'd1;
		end
		else if ( control_state == MEM_WAIT1 && !conflict && instruction_execute)
			 pc_wen_nxt                  = 1'd0;
		else if ( control_state == MEM_WAIT2 && !dabt)
			pc_wen_nxt                  = 1'd0; 
		else if ( control_state == MTRANS_EXEC1 && !conflict && instruction_execute )
		begin
			if ( mtrans_num_registers != 4'd1 )
				pc_wen_nxt              = 1'd0;
			else if ( instruction[20] && mtrans_reg1 == 4'd15 ) 
				pc_wen_nxt                      = 1'd0;
			else 
				pc_wen_nxt                      = 1'd1;	
		end
		else if ( control_state == MTRANS_EXEC2 )
		begin
			if ( mtrans_num_registers > 4'd1 )
				pc_wen_nxt              = 1'd0;
			else if ( instruction[20] && mtrans_reg1 == 4'd15 )
				pc_wen_nxt                      = 1'd0;
			else 
				pc_wen_nxt                      = 1'd1;	
		end
		else if ( control_state == MULT_PROC1 && instruction_execute && !conflict )
			pc_wen_nxt              = 1'd0;
		else if ( control_state == MULT_PROC2 )	 
			pc_wen_nxt              = 1'd0;
		else  if ( control_state == MULT_ACCUMU )
			pc_wen_nxt              = 1'd0;
		else if ( control_state == SWAP_WRITE && instruction_execute && !conflict  &&  instruction_execute)
			pc_wen_nxt              = 1'd0;
		else if ( control_state == SWAP_WAIT1 &&instruction_execute )
			pc_wen_nxt              = 1'd0; 
		else 
			pc_wen_nxt              = 1'd1;
	end

always @*
	begin
		if ( instruction_valid && !interrupt && !conflict )
		begin
			if ( type == REGOP && !opcode_compare && instruction[15:12]  != 4'd15)
				 //reg_bank_wen_nxt = decode (instruction[15:12]);	
				   reg_bank_wen_nxt =   instruction[15:12] == 4'h0 ? 15'h0001:
							instruction[15:12] == 4'h1 ? 15'h0002:
							instruction[15:12] == 4'h2 ? 15'h0004:
							instruction[15:12] == 4'h3 ? 15'h0008:
							instruction[15:12] == 4'h4 ? 15'h0010:
							instruction[15:12] == 4'h5 ? 15'h0020:
							instruction[15:12] == 4'h6 ? 15'h0040:
							instruction[15:12] == 4'h7 ? 15'h0080:
							instruction[15:12] == 4'h8 ? 15'h0100:
							instruction[15:12] == 4'h9 ? 15'h0200:
							instruction[15:12] == 4'ha ? 15'h0400:
							instruction[15:12] == 4'hb ? 15'h0800:
							instruction[15:12] == 4'hc ? 15'h1000:
							instruction[15:12] == 4'hd ? 15'h2000:
							instruction[15:12] == 4'he ? 15'h4000:
									    	     15'h0000;
			else if ( mem_op && ( mem_op_pre_indexed || mem_op_post_indexed ) && rn_sel_nxt  != 4'd15)
			//	reg_bank_wen_nxt = decode ( rn_sel_nxt );
			 	 reg_bank_wen_nxt =    	rn_sel_nxt == 4'h0 ? 15'h0001:
							rn_sel_nxt == 4'h1 ? 15'h0002:
							rn_sel_nxt == 4'h2 ? 15'h0004:
							rn_sel_nxt == 4'h3 ? 15'h0008:
							rn_sel_nxt == 4'h4 ? 15'h0010:
							rn_sel_nxt == 4'h5 ? 15'h0020:
							rn_sel_nxt == 4'h6 ? 15'h0040:
							rn_sel_nxt == 4'h7 ? 15'h0080:
							rn_sel_nxt == 4'h8 ? 15'h0100:
							rn_sel_nxt == 4'h9 ? 15'h0200:
							rn_sel_nxt == 4'ha ? 15'h0400:
							rn_sel_nxt == 4'hb ? 15'h0800:
							rn_sel_nxt == 4'hc ? 15'h1000:
							rn_sel_nxt == 4'hd ? 15'h2000:
							rn_sel_nxt == 4'he ? 15'h4000:
									     15'h0000;
			else if ( type == BRANCH && instruction[24])
				//reg_bank_wen_nxt  = decode (4'd14);
				reg_bank_wen_nxt  = 15'h4000;
			else if ( type == MTRANS && instruction[21] )
				//reg_bank_wen_nxt  = decode (rn_sel_nxt);
				  reg_bank_wen_nxt =  	rn_sel_nxt == 4'h0 ? 15'h0001:
							rn_sel_nxt == 4'h1 ? 15'h0002:
							rn_sel_nxt == 4'h2 ? 15'h0004:
							rn_sel_nxt == 4'h3 ? 15'h0008:
							rn_sel_nxt == 4'h4 ? 15'h0010:
							rn_sel_nxt == 4'h5 ? 15'h0020:
							rn_sel_nxt == 4'h6 ? 15'h0040:
							rn_sel_nxt == 4'h7 ? 15'h0080:
							rn_sel_nxt == 4'h8 ? 15'h0100:
							rn_sel_nxt == 4'h9 ? 15'h0200:
							rn_sel_nxt == 4'ha ? 15'h0400:
							rn_sel_nxt == 4'hb ? 15'h0800:
							rn_sel_nxt == 4'hc ? 15'h1000:
							rn_sel_nxt == 4'hd ? 15'h2000:
							rn_sel_nxt == 4'he ? 15'h4000:
									     15'h0000;
			else if ( type == SWI || und_request )
				//reg_bank_wen_nxt   = decode (4'd14);	
				reg_bank_wen_nxt  = 15'h4000;
		 	else
				reg_bank_wen_nxt  = 15'h0;
		end
		else if ( instruction_valid && interrupt &&  next_interrupt != 3'd6 ) 
				//reg_bank_wen_nxt   = decode (4'd14);	
				reg_bank_wen_nxt  = 15'h4000;
		else if ( control_state == MTRANS_ABORT && restore_base_address)
				//reg_bank_wen_nxt  = decode ( instruction[19:16] );
				  reg_bank_wen_nxt  = 	 instruction[19:16] == 4'h0 ? 15'h0001:
							 instruction[19:16] == 4'h1 ? 15'h0002:
							 instruction[19:16] == 4'h2 ? 15'h0004:
							 instruction[19:16] == 4'h3 ? 15'h0008:
							 instruction[19:16] == 4'h4 ? 15'h0010:
							 instruction[19:16] == 4'h5 ? 15'h0020:
							 instruction[19:16] == 4'h6 ? 15'h0040:
						 	 instruction[19:16] == 4'h7 ? 15'h0080:
						 	 instruction[19:16] == 4'h8 ? 15'h0100:
							 instruction[19:16] == 4'h9 ? 15'h0200:
							 instruction[19:16] == 4'ha ? 15'h0400:
							 instruction[19:16] == 4'hb ? 15'h0800:
						 	 instruction[19:16] == 4'hc ? 15'h1000:
							 instruction[19:16] == 4'hd ? 15'h2000:
							 instruction[19:16] == 4'he ? 15'h4000:
									    	      15'h0000;
		else if ( control_state == MULT_STORE )
		begin
			if ( type == MULT )
				//reg_bank_wen_nxt  = decode ( instruction[19:16] );
				  reg_bank_wen_nxt  = 	 instruction[19:16] == 4'h0 ? 15'h0001:
							 instruction[19:16] == 4'h1 ? 15'h0002:
							 instruction[19:16] == 4'h2 ? 15'h0004:
							 instruction[19:16] == 4'h3 ? 15'h0008:
							 instruction[19:16] == 4'h4 ? 15'h0010:
							 instruction[19:16] == 4'h5 ? 15'h0020:
							 instruction[19:16] == 4'h6 ? 15'h0040:
						 	 instruction[19:16] == 4'h7 ? 15'h0080:
						 	 instruction[19:16] == 4'h8 ? 15'h0100:
							 instruction[19:16] == 4'h9 ? 15'h0200:
							 instruction[19:16] == 4'ha ? 15'h0400:
							 instruction[19:16] == 4'hb ? 15'h0800:
						 	 instruction[19:16] == 4'hc ? 15'h1000:
							 instruction[19:16] == 4'hd ? 15'h2000:
							 instruction[19:16] == 4'he ? 15'h4000:
									    	      15'h0000;
			else
				 //reg_bank_wen_nxt = decode (instruction[15:12]);	
				   reg_bank_wen_nxt =   instruction[15:12] == 4'h0 ? 15'h0001:
							instruction[15:12] == 4'h1 ? 15'h0002:
							instruction[15:12] == 4'h2 ? 15'h0004:
							instruction[15:12] == 4'h3 ? 15'h0008:
							instruction[15:12] == 4'h4 ? 15'h0010:
							instruction[15:12] == 4'h5 ? 15'h0020:
							instruction[15:12] == 4'h6 ? 15'h0040:
							instruction[15:12] == 4'h7 ? 15'h0080:
							instruction[15:12] == 4'h8 ? 15'h0100:
							instruction[15:12] == 4'h9 ? 15'h0200:
							instruction[15:12] == 4'ha ? 15'h0400:
							instruction[15:12] == 4'hb ? 15'h0800:
							instruction[15:12] == 4'hc ? 15'h1000:
							instruction[15:12] == 4'hd ? 15'h2000:
							instruction[15:12] == 4'he ? 15'h4000:
									    	     15'h0000;
		end
		else if ( control_state == COPRO_WAIT && instruction_execute && !conflict && instruction[20] && instruction[15:12] != 4'd15)
				 //reg_bank_wen_nxt = decode (instruction[15:12]);	
				   reg_bank_wen_nxt =   instruction[15:12] == 4'h0 ? 15'h0001:
							instruction[15:12] == 4'h1 ? 15'h0002:
							instruction[15:12] == 4'h2 ? 15'h0004:
							instruction[15:12] == 4'h3 ? 15'h0008:
							instruction[15:12] == 4'h4 ? 15'h0010:
							instruction[15:12] == 4'h5 ? 15'h0020:
							instruction[15:12] == 4'h6 ? 15'h0040:
							instruction[15:12] == 4'h7 ? 15'h0080:
							instruction[15:12] == 4'h8 ? 15'h0100:
							instruction[15:12] == 4'h9 ? 15'h0200:
							instruction[15:12] == 4'ha ? 15'h0400:
							instruction[15:12] == 4'hb ? 15'h0800:
							instruction[15:12] == 4'hc ? 15'h1000:
							instruction[15:12] == 4'hd ? 15'h2000:
							instruction[15:12] == 4'he ? 15'h4000:
									    	     15'h0000;
		else
			reg_bank_wen_nxt                = 15'd0;
	end

always @*
	begin
		if ( instruction_valid && !interrupt && !conflict && regop_set_flags)
			status_bits_flags_wen_nxt = 1'd1;
		else if ( control_state == MULT_STORE && instruction[20])
			status_bits_flags_wen_nxt = 1'd1;
		else if ( control_state == COPRO_WAIT && instruction_execute && !conflict && instruction[20] && instruction[15:12]  == 4'd15)
			status_bits_flags_wen_nxt = 1'd1;
		else
			status_bits_flags_wen_nxt = 1'd0;
	end


always @*
	begin
		if ( instruction_valid && !interrupt && !conflict)
		begin
			if ( type == SWI || und_request )
			begin
				status_bits_mode_wen_nxt      = 1'd1;
				status_bits_irq_mask_wen_nxt  = 1'd1;
			end
			else if ( regop_set_flags && instruction[15:12] == 4'd15 && (i_execute_status_bits[1:0] != USR) )
			begin
				status_bits_mode_wen_nxt      = 1'd1;
				status_bits_irq_mask_wen_nxt  = 1'd1;
			end
			else
			begin
				status_bits_mode_wen_nxt      = 1'd0;
				status_bits_irq_mask_wen_nxt  = 1'd0;
			end
		end
		else if ( instruction_valid && interrupt &&  next_interrupt != 3'd6 )
		begin
			 status_bits_mode_wen_nxt        = 1'd1;
			 status_bits_irq_mask_wen_nxt    = 1'd1;
		end
		
		else if ( control_state == COPRO_WAIT && instruction_execute && !conflict && instruction[20] 
				&& instruction[15:12]  == 4'd15 &&  i_execute_status_bits[1:0] != USR)
		begin		
			status_bits_mode_wen_nxt         = 1'd1;
			status_bits_irq_mask_wen_nxt     = 1'd1;
		end
		else
		begin
			status_bits_mode_wen_nxt         = 1'd0;
			status_bits_irq_mask_wen_nxt     = 1'd0;
		end
	end	


always @*
	begin
		if ( instruction_valid && !interrupt && !conflict && regop_set_flags 
			&& instruction[15:12] == 4'd15 && i_execute_status_bits[1:0] != USR)
			status_bits_firq_mask_wen_nxt = 1'd1;
		else if ( instruction_valid && interrupt &&  next_interrupt != 3'd6 && next_interrupt == 3'd2 )
			status_bits_firq_mask_wen_nxt = 1'd1;
		else if ( control_state == COPRO_WAIT && instruction_execute && !conflict && instruction[20] 
			&& instruction[15:12]  == 4'd15 && i_execute_status_bits[1:0] != USR)
			status_bits_firq_mask_wen_nxt = 1'd1;
		else
			status_bits_firq_mask_wen_nxt = 1'd0;
	end		

 

// Speed up the long path from u_decode/fetch_instruction_r to u_register_bank/r8_firq
// This pre-encodes the firq_s3 signal thats used in u_register_bank
// assign firq_not_user_mode_nxt = !user_mode_regs_load_nxt && status_bits_mode_nxt == FIRQ;

assign firq_not_user_mode_nxt = status_bits_mode_nxt == FIRQ;
 
 
// ========================================================
// Next State Logic
// ========================================================
 
// this replicates the current value of the execute signal in the execute stage
//assign instruction_execute = conditional_execute ( o_condition, i_execute_status_bits[31:28] );
assign instruction_execute = ( o_condition == AL                                       			 ) ||
 		             ( o_condition == EQ  &&  i_execute_status_bits[30]       			 ) ||
		             ( o_condition == NE  && !i_execute_status_bits[30]     			 ) ||
  		             ( o_condition == CS  &&  i_execute_status_bits[29]                          ) ||
               		     ( o_condition == CC  && !i_execute_status_bits[29]                          ) ||
          		     ( o_condition == MI  &&  i_execute_status_bits[31]                          ) ||
  		             ( o_condition == PL  && !i_execute_status_bits[31]                          ) ||
     		             ( o_condition == VS  &&  i_execute_status_bits[28]                          ) ||
      		             ( o_condition == VC  && !i_execute_status_bits[28]   			 ) ||		
     		             ( o_condition == HI  &&  i_execute_status_bits[29] && !i_execute_status_bits[30] 		 ) ||
     		             ( o_condition == LS  &&  (!i_execute_status_bits[29] || i_execute_status_bits[30])          ) ||
 
      		             ( o_condition == GE  &&  i_execute_status_bits[31] == i_execute_status_bits[28]              ) ||
      		             ( o_condition == LT  &&  i_execute_status_bits[31] != i_execute_status_bits[28]              ) ||
 
       		             ( o_condition == GT  &&  !i_execute_status_bits[30] && i_execute_status_bits[31] == i_execute_status_bits[28] ) ||
        	             ( o_condition == LE  &&  (i_execute_status_bits[30] || i_execute_status_bits[31] != i_execute_status_bits[28]) ) ;
 
 
// First state of executing a new instruction
// Its complex because of conditional execution of multi-cycle instructions
assign instruction_valid = ((control_state == EXECUTE || control_state == PRE_FETCH_EXEC) ||
                              // when last instruction was multi-cycle instruction but did not execute
                              // because condition was false then act like you're in the execute state
                             (!instruction_execute && (control_state == PC_STALL1    || 
                                                       control_state == MEM_WAIT1    ||
                                                       control_state == COPRO_WAIT   ||
                                                       control_state == SWAP_WRITE   ||
                                                       control_state == MULT_PROC1   ||
                                                       control_state == MTRANS_EXEC1  ) ));
 
 
 
always @*
    begin
    // default is to hold the current state
//    control_state_nxt <= control_state;
 
    // Note: The order is important here
	
   if ( instruction_valid )
        begin
 
	if ( interrupt && !conflict )        
	    control_state_nxt = INT_WAIT1;
	else
	begin
		if ( type == MTRANS && instruction[20] && mtrans_reg1 == 4'd15 ) // Write to PC
             		control_state_nxt = MEM_WAIT1;
 
		else if ( type == MTRANS && !conflict && mtrans_num_registers != 5'd0 && mtrans_num_registers != 5'd1 )
            		control_state_nxt = MTRANS_EXEC1;
 
        	else if ( type == MULT && !conflict )
               		 control_state_nxt = MULT_PROC1;
 
        	else if ( type == SWAP && !conflict )        
                	control_state_nxt = SWAP_WRITE;
 
        	else if ( type == CORTRANS && !und_request && !conflict )        
                	control_state_nxt = COPRO_WAIT;
		else
		begin
			if ( load_op && instruction[15:12]  == 4'd15 )
				control_state_nxt = MEM_WAIT1;
			else
			begin
				if( current_write_pc )
					control_state_nxt = PC_STALL1;
				else
					control_state_nxt = EXECUTE;
			end
		end
	end		

    end

   else
   begin
    if ( control_state == RST_WAIT1 )     	control_state_nxt = RST_WAIT2;
    else if ( control_state == RST_WAIT2 )    	control_state_nxt = EXECUTE;
    else if ( control_state == INT_WAIT1 )      control_state_nxt = INT_WAIT2;
    else if ( control_state == INT_WAIT2 )      control_state_nxt = EXECUTE;
    else if ( control_state == COPRO_WAIT )     control_state_nxt = PRE_FETCH_EXEC;
    else if ( control_state == PC_STALL1 )      control_state_nxt = PC_STALL2;
    else if ( control_state == PC_STALL2 )      control_state_nxt = EXECUTE; 
    else if ( control_state == SWAP_WRITE )     control_state_nxt = SWAP_WAIT1; 
    else if ( control_state == SWAP_WAIT1 )     control_state_nxt = SWAP_WAIT2; 
    else if ( control_state == MULT_STORE )     control_state_nxt = PRE_FETCH_EXEC;  
    else if ( control_state == MTRANS_ABORT )   control_state_nxt = PRE_FETCH_EXEC;
 
    else if ( control_state == MEM_WAIT1 )	control_state_nxt = MEM_WAIT2;
 
    else if ( control_state == MEM_WAIT2   || control_state == SWAP_WAIT2    )
        begin
        if ( write_pc ) // writing to the PC!! 
            control_state_nxt = PC_STALL1;
        else
            control_state_nxt = PRE_FETCH_EXEC;
        end
 
    else if ( control_state == MTRANS_EXEC1 )  
        begin 
        if ( mtrans_instruction_nxt[15:0] != 16'd0 )
            control_state_nxt = MTRANS_EXEC2;
        else   // if the register list holds a single register 
            begin
            if ( dabt ) // data abort
                control_state_nxt = MTRANS_ABORT;
            else if ( write_pc ) // writing to the PC!! 
                control_state_nxt = MEM_WAIT1;
            else
                control_state_nxt = PRE_FETCH_EXEC;
            end

        end
 
        // Stay in State MTRANS_EXEC2 until the full list of registers to
        // load or store has been processed

    else if ( control_state == MTRANS_EXEC2 && mtrans_num_registers == 5'd1 )
        begin
        if ( dabt ) // data abort
            control_state_nxt = MTRANS_ABORT;
        else if ( write_pc ) // writing to the PC!! 
            control_state_nxt = MEM_WAIT1;
        else
            control_state_nxt = PRE_FETCH_EXEC;
        end
 
 
    else if ( control_state == MULT_PROC1 )
        begin
        if (!instruction_execute)
            control_state_nxt = PRE_FETCH_EXEC;
        else
            control_state_nxt = MULT_PROC2;
        end
 
    else if ( control_state == MULT_PROC2 )
        begin
        if ( i_multiply_done )
	begin
            if      ( o_multiply_function[1] )  // Accumulate ?
                control_state_nxt = MULT_ACCUMU;
            else    
                control_state_nxt = MULT_STORE;
	end
	else
		control_state_nxt = control_state;
        end
 
 
    else if ( control_state == MULT_ACCUMU )
        begin
        control_state_nxt = MULT_STORE;
        end
 
 
    else   //jing
	control_state_nxt = control_state;
     end
 
    end
 
 
assign 	 i_fetch = {i_fetch_instruction[27:20], i_fetch_instruction[7:4]};  //jing
// ========================================================
// Register Update
// ========================================================
always @ ( posedge i_clk )
    if ( !i_core_stall ) 
        begin        
        if (!conflict)
            begin                                                                                                  
            fetch_instruction_r         <= i_fetch_instruction;
//            fetch_instruction_type_r    <= instruction_type(i_fetch_instruction);
	
	     fetch_instruction_type_r    <= 
					(
//					{i_fetch_instruction[27:20], i_fetch_instruction[7:4]}[11:6] == 12'b00010?001001 ? SWAP:
//					{i_fetch_instruction[27:20], i_fetch_instruction[7:4]} =        12'b000000??1001 ? MULT:
//					{i_fetch_instruction[27:20], i_fetch_instruction[7:4]}[11:10] = 12'b00?????????? ? REGOP:
//					{i_fetch_instruction[27:20], i_fetch_instruction[7:4]} = 	12'b01?????????? ? TRANS:   
//					{i_fetch_instruction[27:20], i_fetch_instruction[7:4]} =	12'b100????????? ? MTRANS:  
//					{i_fetch_instruction[27:20], i_fetch_instruction[7:4]} =	12'b101????????? ? BRANCH: 
//					{i_fetch_instruction[27:20], i_fetch_instruction[7:4]} = 	12'b110????????? ? CODTRANS:
//					{i_fetch_instruction[27:20], i_fetch_instruction[7:4]} = 	12'b1110???????0 ? COREGOP:       
//					{i_fetch_instruction[27:20], i_fetch_instruction[7:4]} = 	12'b1110???????1 ? CORTRANS:
//														    SWI     

					(i_fetch[11:7]  == 5'b00010  && i_fetch[5:0] == 6'b001001)? SWAP:
					(i_fetch[11:6]  == 6'b000000 && i_fetch[3:0] == 4'b1001  )? MULT:
					 i_fetch[11:10] == 2'b00 ? REGOP:
					 i_fetch[11:10] == 2'b01 ? TRANS:
					 i_fetch[11:9]  == 3'b100 ? MTRANS:
					 i_fetch[11:9]  == 3'b101 ? BRANCH:
					 i_fetch[11:9]  == 3'b110 ? CODTRANS:
					(i_fetch[11:8]  == 4'b1110 && i_fetch[0] == 1'b0  )? COREGOP:
					(i_fetch[11:8]  == 4'b1110 && i_fetch[0] == 1'b1  )? CORTRANS:
												SWI);

            fetch_address_r             <= i_execute_iaddress;
            iabt_reg                    <= i_iabt;
            adex_reg                    <= i_adex;
            abt_status_reg              <= i_abt_status;
            end
 
        o_status_bits_mode          <= status_bits_mode_nxt;
        o_status_bits_irq_mask      <= status_bits_irq_mask_nxt;
        o_status_bits_firq_mask     <= status_bits_firq_mask_nxt;
        o_imm32                     <= imm32_nxt;
        o_imm_shift_amount          <= imm_shift_amount_nxt;
        o_shift_imm_zero            <= shift_imm_zero_nxt;
 
                                        // when have an interrupt, execute the interrupt operation
                                        // unconditionally in the execute stage
                                        // ensures that status_bits register gets updated correctly
                                        // Likewise when in middle of multi-cycle instructions
                                        // execute them unconditionally
        o_condition                 <= instruction_valid && !interrupt ? condition_nxt : AL;
        o_decode_exclusive          <= decode_exclusive_nxt;
        o_decode_iaccess            <= decode_iaccess_nxt;
        o_decode_daccess            <= decode_daccess_nxt;
 
        o_rm_sel                    <= rm_sel_nxt;
        o_rs_sel                    <= rs_sel_nxt;
        o_load_rd                   <= load_rd_nxt;
        load_rd_d1                  <= load_rd_d1_nxt; 
        load_pc_r                   <= load_pc_nxt;
        o_rn_sel                    <= rn_sel_nxt;
        o_barrel_shift_amount_sel   <= barrel_shift_amount_sel_nxt;
        o_barrel_shift_data_sel     <= barrel_shift_data_sel_nxt;
        o_barrel_shift_function     <= barrel_shift_function_nxt;
        o_alu_function              <= alu_function_nxt;
        o_multiply_function         <= multiply_function_nxt;
        o_interrupt_vector_sel      <= next_interrupt;
        o_iaddress_sel              <= iaddress_sel_nxt;
        o_daddress_sel              <= daddress_sel_nxt;
        o_pc_sel                    <= pc_sel_nxt;
        o_byte_enable_sel           <= byte_enable_sel_nxt;
        o_status_bits_sel           <= status_bits_sel_nxt;
        o_reg_write_sel             <= reg_write_sel_nxt;
        o_firq_not_user_mode        <= firq_not_user_mode_nxt;
        o_write_data_wen            <= write_data_wen_nxt;
        o_base_address_wen          <= base_address_wen_nxt;
        o_pc_wen                    <= pc_wen_nxt;
        o_reg_bank_wen              <= reg_bank_wen_nxt;
        o_status_bits_flags_wen     <= status_bits_flags_wen_nxt;
        o_status_bits_mode_wen      <= status_bits_mode_wen_nxt;
        o_status_bits_irq_mask_wen  <= status_bits_irq_mask_wen_nxt;
        o_status_bits_firq_mask_wen <= status_bits_firq_mask_wen_nxt;
 
        o_copro_opcode1             <= instruction[23:21];
        o_copro_opcode2             <= instruction[7:5];
        o_copro_crn                 <= instruction[19:16];
        o_copro_crm                 <= instruction[3:0];
        o_copro_num                 <= instruction[11:8];
        o_copro_operation           <= copro_operation_nxt;
        o_copro_write_data_wen      <= copro_write_data_wen_nxt;
        restore_base_address        <= restore_base_address_nxt;
        control_state               <= control_state_nxt;
        end
 
 
 
always @ ( posedge i_clk )
    if ( !i_core_stall )
        begin
        // sometimes this is a pre-fetch instruction
        // e.g. two ldr instructions in a row. The second ldr will be saved
        // to the pre-fetch instruction register
        // then when its decoded, a copy is saved to the saved_current_instruction
        // register
        if      ( type == MTRANS )
            begin           
            saved_current_instruction              <= mtrans_instruction_nxt;
            saved_current_instruction_type         <= type;
            saved_current_instruction_iabt         <= instruction_iabt;
            saved_current_instruction_adex         <= instruction_adex;
            saved_current_instruction_address      <= instruction_address;
            saved_current_instruction_iabt_status  <= instruction_iabt_status;
            end
        else if ( saved_current_instruction_wen ) 
            begin           
            saved_current_instruction              <= instruction;
            saved_current_instruction_type         <= type;
            saved_current_instruction_iabt         <= instruction_iabt;
            saved_current_instruction_adex         <= instruction_adex;
            saved_current_instruction_address      <= instruction_address;
            saved_current_instruction_iabt_status  <= instruction_iabt_status;
            end
 
        if      ( pre_fetch_instruction_wen )     
            begin
            pre_fetch_instruction                  <= fetch_instruction_r;      
            pre_fetch_instruction_type             <= fetch_instruction_type_r;      
            pre_fetch_instruction_iabt             <= iabt_reg; 
            pre_fetch_instruction_adex             <= adex_reg; 
            pre_fetch_instruction_address          <= fetch_address_r; 
            pre_fetch_instruction_iabt_status      <= abt_status_reg; 
            end
 
 
        // TODO possible to use saved_current_instruction instead and save some regs?          
        hold_instruction              <= instruction;
        hold_instruction_type         <= type;
        hold_instruction_iabt         <= instruction_iabt;
        hold_instruction_adex         <= instruction_adex;
        hold_instruction_address      <= instruction_address;
        hold_instruction_iabt_status  <= instruction_iabt_status;
        end
 
 
 
always @ ( posedge i_clk )
    if ( !i_core_stall )
        begin
        irq   <= i_irq;  
        firq  <= i_firq; 
 
        if ( control_state == INT_WAIT1 && o_status_bits_mode == SVC )
            begin
            dabt_reg  <= 1'd0;
            end
        else
            begin
            dabt_reg  <= dabt_reg || i_dabt;  
            end
 
        dabt_reg_d1  <= dabt_reg;   
        end  
 
assign dabt = dabt_reg || i_dabt;
 
 
// ========================================================
// Decompiler for debugging core - not synthesizable
// ========================================================
//synopsys translate_off
 
//`include "a25/debug_functions.v"
 
/*a25_decompile  u_decompile (
    .i_clk                      ( i_clk                            ),
    .i_core_stall               ( i_core_stall                     ),
    .i_instruction              ( instruction                      ),
    .i_instruction_valid        ( instruction_valid &&!conflict    ),
    .i_instruction_execute      ( instruction_execute              ),
    .i_instruction_address      ( instruction_address              ),
    .i_interrupt                ( {3{interrupt}} & next_interrupt  ),
    .i_interrupt_state          ( control_state == INT_WAIT2       ),
    .i_instruction_undefined    ( und_request                      ),
    .i_pc_sel                   ( o_pc_sel                         ),
    .i_pc_wen                   ( o_pc_wen                         )
);
*/ 
 
wire    [(15*8)-1:0]    xCONTROL_STATE;
wire    [(15*8)-1:0]    xMODE;
wire    [( 8*8)-1:0]    xTYPE;
 
assign xCONTROL_STATE        = 
                               control_state == RST_WAIT1      ? "RST_WAIT1"      :
                               control_state == RST_WAIT2      ? "RST_WAIT2"      :
 
 
                               control_state == INT_WAIT1      ? "INT_WAIT1"      :
                               control_state == INT_WAIT2      ? "INT_WAIT2"      :
                               control_state == EXECUTE        ? "EXECUTE"        :
                               control_state == PRE_FETCH_EXEC ? "PRE_FETCH_EXEC" :
                               control_state == MEM_WAIT1      ? "MEM_WAIT1"      :
                               control_state == MEM_WAIT2      ? "MEM_WAIT2"      :
                               control_state == PC_STALL1      ? "PC_STALL1"      :
                               control_state == PC_STALL2      ? "PC_STALL2"      :
                               control_state == MTRANS_EXEC1   ? "MTRANS_EXEC1"   :
                               control_state == MTRANS_EXEC2   ? "MTRANS_EXEC2"   :
                               control_state == MTRANS_ABORT   ? "MTRANS_ABORT"   :
                               control_state == MULT_PROC1     ? "MULT_PROC1"     :
                               control_state == MULT_PROC2     ? "MULT_PROC2"     :
                               control_state == MULT_STORE     ? "MULT_STORE"     :
                               control_state == MULT_ACCUMU    ? "MULT_ACCUMU"    :
                               control_state == SWAP_WRITE     ? "SWAP_WRITE"     :
                               control_state == SWAP_WAIT1     ? "SWAP_WAIT1"     :
                               control_state == SWAP_WAIT2     ? "SWAP_WAIT2"     :
								 "COPRO_WAIT"     ;
 
//assign xMODE  = mode_name ( o_status_bits_mode );

//assign xMODE  = o_status_bits_mode == USR  ? "User          " :
//                o_status_bits_mode == SVC  ? "Supervisor    " :
//                o_status_bits_mode == IRQ  ? "Interrupt     " :
//				 	       "Fast_Interrupt" ;

assign xTYPE  =  
                               type == REGOP    ? "REGOP"    :
                               type == MULT     ? "MULT"     :
                               type == SWAP     ? "SWAP"     :
                               type == TRANS    ? "TRANS"    :
                               type == MTRANS   ? "MTRANS"   :
                               type == BRANCH   ? "BRANCH"   :
                               type == CODTRANS ? "CODTRANS" :
                               type == COREGOP  ? "COREGOP"  :
                               type == CORTRANS ? "CORTRANS" :
						  "SWI"      ;
  
 
/*always @( posedge i_clk )
    if (control_state == EXECUTE && ((instruction[0] === 1'bx) || (instruction[31] === 1'bx)))
        begin
        `TB_ERROR_MESSAGE
        $display("Instruction with x's =%08h", instruction);
        end
*/
//synopsys translate_on
 
endmodule


 
module a25_shifter_quick 
(
 
i_in,
i_carry_in,
i_shift_amount,    
i_shift_imm_zero,   
i_function,
 
o_out,
o_carry_out
 
);

input       [31:0]          i_in;
input                       i_carry_in;
input       [7:0]           i_shift_amount;     // uses 8 LSBs of Rs, or a 5 bit immediate constant
input                       i_shift_imm_zero;   // high when immediate shift value of zero selected
input       [1:0]           i_function;
 
output      [31:0]          o_out;
output                      o_carry_out;

//////////////////////////////////////////////////////////////////
//                                                              //
//  Parameters file for Amber 25 Core                           //
//                                                              //
//  This file is part of the Amber project                      //
//  http://www.opencores.org/project,amber                      //
//                                                              //
//  Description                                                 //
//  Holds general parameters that are used is several core      //
//  modules                                                     //
//                                                              //
//  Author(s):                                                  //
//      - Conor Santifort, csantifort.amber@gmail.com           //
//                                                              //
//////////////////////////////////////////////////////////////////
//                                                              //
// Copyright (C) 2011 Authors and OPENCORES.ORG                 //
//                                                              //
// This source file may be used and distributed without         //
// restriction provided that this copyright statement is not    //
// removed from the file and that any derivative work contains  //
// the original copyright notice and the associated disclaimer. //
//                                                              //
// This source file is free software; you can redistribute it   //
// and/or modify it under the terms of the GNU Lesser General   //
// Public License as published by the Free Software Foundation; //
// either version 2.1 of the License, or (at your option) any   //
// later version.                                               //
//                                                              //
// This source is distributed in the hope that it will be       //
// useful, but WITHOUT ANY WARRANTY; without even the implied   //
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      //
// PURPOSE.  See the GNU Lesser General Public License for more //
// details.                                                     //
//                                                              //
// You should have received a copy of the GNU Lesser General    //
// Public License along with this source; if not, download it   //
// from http://www.opencores.org/lgpl.shtml                     //
//                                                              //
//////////////////////////////////////////////////////////////////
 
 
// Instruction Types
localparam [3:0]    REGOP       = 4'h0, // Data processing
                    MULT        = 4'h1, // Multiply
                    SWAP        = 4'h2, // Single Data Swap
                    TRANS       = 4'h3, // Single data transfer
                    MTRANS      = 4'h4, // Multi-word data transfer
                    BRANCH      = 4'h5, // Branch
                    CODTRANS    = 4'h6, // Co-processor data transfer
                    COREGOP     = 4'h7, // Co-processor data operation
                    CORTRANS    = 4'h8, // Co-processor register transfer
                    SWI         = 4'h9; // software interrupt
 
 
// Opcodes
localparam [3:0] AND = 4'h0,        // Logical AND
                 EOR = 4'h1,        // Logical Exclusive OR
                 SUB = 4'h2,        // Subtract
                 RSB = 4'h3,        // Reverse Subtract
                 ADD = 4'h4,        // Add
                 ADC = 4'h5,        // Add with Carry
                 SBC = 4'h6,        // Subtract with Carry
                 RSC = 4'h7,        // Reverse Subtract with Carry
                 TST = 4'h8,        // Test  (using AND operator)
                 TEQ = 4'h9,        // Test Equivalence (using EOR operator)
                 CMP = 4'ha,       // Compare (using Subtract operator)
                 CMN = 4'hb,       // Compare Negated
                 ORR = 4'hc,       // Logical OR
                 MOV = 4'hd,       // Move
                 BIC = 4'he,       // Bit Clear (using AND & NOT operators)
                 MVN = 4'hf;       // Move NOT
 
// Condition Encoding
localparam [3:0] EQ  = 4'h0,        // Equal            / Z set
                 NE  = 4'h1,        // Not equal        / Z clear
                 CS  = 4'h2,        // Carry set        / C set
                 CC  = 4'h3,        // Carry clear      / C clear
                 MI  = 4'h4,        // Minus            / N set
                 PL  = 4'h5,        // Plus             / N clear
                 VS  = 4'h6,        // Overflow         / V set
                 VC  = 4'h7,        // No overflow      / V clear
                 HI  = 4'h8,        // Unsigned higher  / C set and Z clear
                 LS  = 4'h9,        // Unsigned lower
                                    // or same          / C clear or Z set
                 GE  = 4'ha,        // Signed greater 
                                    // than or equal    / N == V
                 LT  = 4'hb,        // Signed less than / N != V
                 GT  = 4'hc,        // Signed greater
                                    // than             / Z == 0, N == V
                 LE  = 4'hd,        // Signed less than
                                    // or equal         / Z == 1, N != V
                 AL  = 4'he,        // Always
                 NV  = 4'hf;        // Never
 
// Any instruction with a condition field of 0b1111 is UNPREDICTABLE.                
 
// Shift Types
localparam [1:0] LSL = 2'h0,
                 LSR = 2'h1,
                 ASR = 2'h2,
                 RRX = 2'h3,
                 ROR = 2'h3; 
 
// Modes
localparam [1:0] SVC  =  2'b11,  // Supervisor
                 IRQ  =  2'b10,  // Interrupt
                 FIRQ =  2'b01,  // Fast Interrupt
                 USR  =  2'b00;  // User
 
// One-Hot Mode encodings
localparam [5:0] OH_USR  = 0,
                 OH_IRQ  = 1,
                 OH_FIRQ = 2,
                 OH_SVC  = 3;
 
 
 
  // MSB is carry out
wire [32:0] lsl_out;
wire [32:0] lsr_out;
wire [32:0] asr_out;
wire [32:0] ror_out;
 
 
// Logical shift right zero is redundant as it is the same as logical shift left zero, so
// the assembler will convert LSR #0 (and ASR #0 and ROR #0) into LSL #0, and allow
// lsr #32 to be specified.
 
// lsl #0 is a special case, where the shifter carry out is the old value of the status flags
// C flag. The contents of Rm are used directly as the second operand.
  
    // only gives the correct result if the shift value is < 4
    assign lsl_out = i_shift_imm_zero        ? {i_carry_in, i_in              } : // fall through case 
                     i_shift_amount == 2'd0  ? {i_carry_in, i_in              } : // fall through case
                     i_shift_amount == 2'd1  ? {i_in[31],   i_in[30: 0],  1'd0} :
                     i_shift_amount == 2'd2  ? {i_in[30],   i_in[29: 0],  2'd0} :
                                               {i_in[29],   i_in[28: 0],  3'd0} ; // 3
 
// The form of the shift field which might be expected to correspond to LSR #0 is used
// to encode LSR #32, which has a zero result with bit 31 of Rm as the carry output. 

    // only gives the correct result if the shift value is < 4
    assign lsr_out = i_shift_imm_zero             ? {i_in[31], 32'd0             } :
                     i_shift_amount[1:0] == 2'd0  ? {i_carry_in, i_in            } :  // fall through case
                     i_shift_amount[1:0] == 2'd1  ? {i_in[ 0],  1'd0, i_in[31: 1]} :
                     i_shift_amount[1:0] == 2'd2  ? {i_in[ 1],  2'd0, i_in[31: 2]} :
                                                    {i_in[ 2],  3'd0, i_in[31: 3]} ; // 3
 
// The form of the shift field which might be expected to give ASR #0 is used to encode
// ASR #32. Bit 31 of Rm is again used as the carry output, and each bit of operand 2 is
// also equal to bit 31 of Rm. The result is therefore all ones or all zeros, according to
// the value of bit 31 of Rm. 
 
    // only gives the correct result if the shift value is < 4
    assign asr_out = i_shift_imm_zero             ? {i_in[31], {32{i_in[31]}}             } :
                     i_shift_amount[1:0] == 2'd0  ? {i_carry_in, i_in                     } :  // fall through case
                     i_shift_amount[1:0] == 2'd1  ? {i_in[ 0], { 2{i_in[31]}}, i_in[30: 1]} :
                     i_shift_amount[1:0] == 2'd2  ? {i_in[ 1], { 3{i_in[31]}}, i_in[30: 2]} :
                                                    {i_in[ 2], { 4{i_in[31]}}, i_in[30: 3]} ; // 3
 
    // only gives the correct result if the shift value is < 4
    assign ror_out = i_shift_imm_zero             ? {i_in[ 0], i_carry_in,  i_in[31: 1]} :  // RXR, (ROR w/ imm 0)
                     i_shift_amount[1:0] == 2'd0  ? {i_carry_in, i_in                  } :  // fall through case
                     i_shift_amount[1:0] == 2'd1  ? {i_in[ 0], i_in[    0], i_in[31: 1]} :
                     i_shift_amount[1:0] == 2'd2  ? {i_in[ 1], i_in[ 1: 0], i_in[31: 2]} :
                                                    {i_in[ 2], i_in[ 2: 0], i_in[31: 3]} ; // 3
 
assign {o_carry_out, o_out} = i_function == LSL ? lsl_out :
                              i_function == LSR ? lsr_out :
                              i_function == ASR ? asr_out :
                                                  ror_out ;
 
endmodule



module a25_shifter_full 
(
 
i_in,
i_carry_in,
i_shift_amount,    
i_shift_imm_zero,   
i_function,
 
o_out,
o_carry_out
 
);

input       [31:0]          i_in;
input                       i_carry_in;
input       [7:0]           i_shift_amount;     // uses 8 LSBs of Rs, or a 5 bit immediate constant
input                       i_shift_imm_zero;   // high when immediate shift value of zero selected
input       [1:0]           i_function;
 
output      [31:0]          o_out;
output                      o_carry_out;

//////////////////////////////////////////////////////////////////
//                                                              //
//  Parameters file for Amber 25 Core                           //
//                                                              //
//  This file is part of the Amber project                      //
//  http://www.opencores.org/project,amber                      //
//                                                              //
//  Description                                                 //
//  Holds general parameters that are used is several core      //
//  modules                                                     //
//                                                              //
//  Author(s):                                                  //
//      - Conor Santifort, csantifort.amber@gmail.com           //
//                                                              //
//////////////////////////////////////////////////////////////////
//                                                              //
// Copyright (C) 2011 Authors and OPENCORES.ORG                 //
//                                                              //
// This source file may be used and distributed without         //
// restriction provided that this copyright statement is not    //
// removed from the file and that any derivative work contains  //
// the original copyright notice and the associated disclaimer. //
//                                                              //
// This source file is free software; you can redistribute it   //
// and/or modify it under the terms of the GNU Lesser General   //
// Public License as published by the Free Software Foundation; //
// either version 2.1 of the License, or (at your option) any   //
// later version.                                               //
//                                                              //
// This source is distributed in the hope that it will be       //
// useful, but WITHOUT ANY WARRANTY; without even the implied   //
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      //
// PURPOSE.  See the GNU Lesser General Public License for more //
// details.                                                     //
//                                                              //
// You should have received a copy of the GNU Lesser General    //
// Public License along with this source; if not, download it   //
// from http://www.opencores.org/lgpl.shtml                     //
//                                                              //
//////////////////////////////////////////////////////////////////
 
 
// Instruction Types
localparam [3:0]    REGOP       = 4'h0, // Data processing
                    MULT        = 4'h1, // Multiply
                    SWAP        = 4'h2, // Single Data Swap
                    TRANS       = 4'h3, // Single data transfer
                    MTRANS      = 4'h4, // Multi-word data transfer
                    BRANCH      = 4'h5, // Branch
                    CODTRANS    = 4'h6, // Co-processor data transfer
                    COREGOP     = 4'h7, // Co-processor data operation
                    CORTRANS    = 4'h8, // Co-processor register transfer
                    SWI         = 4'h9; // software interrupt
 
 
// Opcodes
localparam [3:0] AND = 4'h0,        // Logical AND
                 EOR = 4'h1,        // Logical Exclusive OR
                 SUB = 4'h2,        // Subtract
                 RSB = 4'h3,        // Reverse Subtract
                 ADD = 4'h4,        // Add
                 ADC = 4'h5,        // Add with Carry
                 SBC = 4'h6,        // Subtract with Carry
                 RSC = 4'h7,        // Reverse Subtract with Carry
                 TST = 4'h8,        // Test  (using AND operator)
                 TEQ = 4'h9,        // Test Equivalence (using EOR operator)
                 CMP = 4'ha,       // Compare (using Subtract operator)
                 CMN = 4'hb,       // Compare Negated
                 ORR = 4'hc,       // Logical OR
                 MOV = 4'hd,       // Move
                 BIC = 4'he,       // Bit Clear (using AND & NOT operators)
                 MVN = 4'hf;       // Move NOT
 
// Condition Encoding
localparam [3:0] EQ  = 4'h0,        // Equal            / Z set
                 NE  = 4'h1,        // Not equal        / Z clear
                 CS  = 4'h2,        // Carry set        / C set
                 CC  = 4'h3,        // Carry clear      / C clear
                 MI  = 4'h4,        // Minus            / N set
                 PL  = 4'h5,        // Plus             / N clear
                 VS  = 4'h6,        // Overflow         / V set
                 VC  = 4'h7,        // No overflow      / V clear
                 HI  = 4'h8,        // Unsigned higher  / C set and Z clear
                 LS  = 4'h9,        // Unsigned lower
                                    // or same          / C clear or Z set
                 GE  = 4'ha,        // Signed greater 
                                    // than or equal    / N == V
                 LT  = 4'hb,        // Signed less than / N != V
                 GT  = 4'hc,        // Signed greater
                                    // than             / Z == 0, N == V
                 LE  = 4'hd,        // Signed less than
                                    // or equal         / Z == 1, N != V
                 AL  = 4'he,        // Always
                 NV  = 4'hf;        // Never
 
// Any instruction with a condition field of 0b1111 is UNPREDICTABLE.                
 
// Shift Types
localparam [1:0] LSL = 2'h0,
                 LSR = 2'h1,
                 ASR = 2'h2,
                 RRX = 2'h3,
                 ROR = 2'h3; 
 
// Modes
localparam [1:0] SVC  =  2'b11,  // Supervisor
                 IRQ  =  2'b10,  // Interrupt
                 FIRQ =  2'b01,  // Fast Interrupt
                 USR  =  2'b00;  // User
 
// One-Hot Mode encodings
localparam [5:0] OH_USR  = 0,
                 OH_IRQ  = 1,
                 OH_FIRQ = 2,
                 OH_SVC  = 3;
 
 

  // MSB is carry out
wire [32:0] lsl_out;
wire [32:0] lsr_out;
wire [32:0] asr_out;
wire [32:0] ror_out;
 
 
// Logical shift right zero is redundant as it is the same as logical shift left zero, so
// the assembler will convert LSR #0 (and ASR #0 and ROR #0) into LSL #0, and allow
// lsr #32 to be specified.
 
// lsl #0 is a special case, where the shifter carry out is the old value of the status flags
// C flag. The contents of Rm are used directly as the second operand.
 
    assign lsl_out = i_shift_imm_zero         ? {i_carry_in, i_in              } :  // fall through case 
 
                     i_shift_amount == 8'd0  ? {i_carry_in, i_in              } :  // fall through case
                     i_shift_amount == 8'd1  ? {i_in[31],   i_in[30: 0],  1'd0} :
                     i_shift_amount == 8'd2  ? {i_in[30],   i_in[29: 0],  2'd0} :
                     i_shift_amount == 8'd3  ? {i_in[29],   i_in[28: 0],  3'd0} :
                     i_shift_amount == 8'd4  ? {i_in[28],   i_in[27: 0],  4'd0} :
                     i_shift_amount == 8'd5  ? {i_in[27],   i_in[26: 0],  5'd0} :
                     i_shift_amount == 8'd6  ? {i_in[26],   i_in[25: 0],  6'd0} :
                     i_shift_amount == 8'd7  ? {i_in[25],   i_in[24: 0],  7'd0} :
                     i_shift_amount == 8'd8  ? {i_in[24],   i_in[23: 0],  8'd0} :
                     i_shift_amount == 8'd9  ? {i_in[23],   i_in[22: 0],  9'd0} :
                     i_shift_amount == 8'd10  ? {i_in[22],   i_in[21: 0], 10'd0} :
                     i_shift_amount == 8'd11  ? {i_in[21],   i_in[20: 0], 11'd0} :
 
                     i_shift_amount == 8'd12  ? {i_in[20],   i_in[19: 0], 12'd0} :
                     i_shift_amount == 8'd13  ? {i_in[19],   i_in[18: 0], 13'd0} :
                     i_shift_amount == 8'd14  ? {i_in[18],   i_in[17: 0], 14'd0} :
                     i_shift_amount == 8'd15  ? {i_in[17],   i_in[16: 0], 15'd0} :
                     i_shift_amount == 8'd16  ? {i_in[16],   i_in[15: 0], 16'd0} :
                     i_shift_amount == 8'd17  ? {i_in[15],   i_in[14: 0], 17'd0} :
                     i_shift_amount == 8'd18  ? {i_in[14],   i_in[13: 0], 18'd0} :
                     i_shift_amount == 8'd19  ? {i_in[13],   i_in[12: 0], 19'd0} :
                     i_shift_amount == 8'd20  ? {i_in[12],   i_in[11: 0], 20'd0} :
                     i_shift_amount == 8'd21  ? {i_in[11],   i_in[10: 0], 21'd0} :
 
                     i_shift_amount == 8'd22  ? {i_in[10],   i_in[ 9: 0], 22'd0} :
                     i_shift_amount == 8'd23  ? {i_in[ 9],   i_in[ 8: 0], 23'd0} :
                     i_shift_amount == 8'd24  ? {i_in[ 8],   i_in[ 7: 0], 24'd0} :
                     i_shift_amount == 8'd25  ? {i_in[ 7],   i_in[ 6: 0], 25'd0} :
                     i_shift_amount == 8'd26  ? {i_in[ 6],   i_in[ 5: 0], 26'd0} :
                     i_shift_amount == 8'd27  ? {i_in[ 5],   i_in[ 4: 0], 27'd0} :
                     i_shift_amount == 8'd28  ? {i_in[ 4],   i_in[ 3: 0], 28'd0} :
                     i_shift_amount == 8'd29  ? {i_in[ 3],   i_in[ 2: 0], 29'd0} :
                     i_shift_amount == 8'd30  ? {i_in[ 2],   i_in[ 1: 0], 30'd0} :
                     i_shift_amount == 8'd31  ? {i_in[ 1],   i_in[ 0: 0], 31'd0} :
                     i_shift_amount == 8'd32  ? {i_in[ 0],   32'd0             } :  // 32
                                                {1'd0,       32'd0             } ;  // > 32
 

// The form of the shift field which might be expected to correspond to LSR #0 is used
// to encode LSR #32, which has a zero result with bit 31 of Rm as the carry output. 
 
                                               // carry out, < -------- out ---------->
    assign lsr_out = i_shift_imm_zero         ? {i_in[31], 32'd0             } :
                     i_shift_amount == 8'd0  ? {i_carry_in, i_in            } :  // fall through case
                     i_shift_amount == 8'd1  ? {i_in[ 0],  1'd0, i_in[31: 1]} :
                     i_shift_amount == 8'd2  ? {i_in[ 1],  2'd0, i_in[31: 2]} :
                     i_shift_amount == 8'd3  ? {i_in[ 2],  3'd0, i_in[31: 3]} :
                     i_shift_amount == 8'd4  ? {i_in[ 3],  4'd0, i_in[31: 4]} :
                     i_shift_amount == 8'd5  ? {i_in[ 4],  5'd0, i_in[31: 5]} :
                     i_shift_amount == 8'd6  ? {i_in[ 5],  6'd0, i_in[31: 6]} :
                     i_shift_amount == 8'd7  ? {i_in[ 6],  7'd0, i_in[31: 7]} :
                     i_shift_amount == 8'd8  ? {i_in[ 7],  8'd0, i_in[31: 8]} :
                     i_shift_amount == 8'd9  ? {i_in[ 8],  9'd0, i_in[31: 9]} :
 
                     i_shift_amount == 8'd10  ? {i_in[ 9], 10'd0, i_in[31:10]} :
                     i_shift_amount == 8'd11  ? {i_in[10], 11'd0, i_in[31:11]} :
                     i_shift_amount == 8'd12  ? {i_in[11], 12'd0, i_in[31:12]} :
                     i_shift_amount == 8'd13  ? {i_in[12], 13'd0, i_in[31:13]} :
                     i_shift_amount == 8'd14  ? {i_in[13], 14'd0, i_in[31:14]} :
                     i_shift_amount == 8'd15  ? {i_in[14], 15'd0, i_in[31:15]} :
                     i_shift_amount == 8'd16  ? {i_in[15], 16'd0, i_in[31:16]} :
                     i_shift_amount == 8'd17  ? {i_in[16], 17'd0, i_in[31:17]} :
                     i_shift_amount == 8'd18  ? {i_in[17], 18'd0, i_in[31:18]} :
                     i_shift_amount == 8'd19  ? {i_in[18], 19'd0, i_in[31:19]} :
 
                     i_shift_amount == 8'd20  ? {i_in[19], 20'd0, i_in[31:20]} :
                     i_shift_amount == 8'd21  ? {i_in[20], 21'd0, i_in[31:21]} :
                     i_shift_amount == 8'd22  ? {i_in[21], 22'd0, i_in[31:22]} :
                     i_shift_amount == 8'd23  ? {i_in[22], 23'd0, i_in[31:23]} :
                     i_shift_amount == 8'd24  ? {i_in[23], 24'd0, i_in[31:24]} :
                     i_shift_amount == 8'd25  ? {i_in[24], 25'd0, i_in[31:25]} :
                     i_shift_amount == 8'd26  ? {i_in[25], 26'd0, i_in[31:26]} :
                     i_shift_amount == 8'd27  ? {i_in[26], 27'd0, i_in[31:27]} :
                     i_shift_amount == 8'd28  ? {i_in[27], 28'd0, i_in[31:28]} :
                     i_shift_amount == 8'd29  ? {i_in[28], 29'd0, i_in[31:29]} :
 
                     i_shift_amount == 8'd30  ? {i_in[29], 30'd0, i_in[31:30]} :
                     i_shift_amount == 8'd31  ? {i_in[30], 31'd0, i_in[31   ]} :
                     i_shift_amount == 8'd32  ? {i_in[31], 32'd0             } :
                                                {1'd0,     32'd0             } ;  // > 32
 
// The form of the shift field which might be expected to give ASR #0 is used to encode
// ASR #32. Bit 31 of Rm is again used as the carry output, and each bit of operand 2 is
// also equal to bit 31 of Rm. The result is therefore all ones or all zeros, according to
// the value of bit 31 of Rm.
  
                                              // carry out, < -------- out ---------->
    assign asr_out = i_shift_imm_zero         ? {i_in[31], {32{i_in[31]}}             } :
                     i_shift_amount == 8'd0  ? {i_carry_in, i_in                     } :  // fall through case
                     i_shift_amount == 8'd1  ? {i_in[ 0], { 2{i_in[31]}}, i_in[30: 1]} :
                     i_shift_amount == 8'd2  ? {i_in[ 1], { 3{i_in[31]}}, i_in[30: 2]} :
                     i_shift_amount == 8'd3  ? {i_in[ 2], { 4{i_in[31]}}, i_in[30: 3]} :
                     i_shift_amount == 8'd4  ? {i_in[ 3], { 5{i_in[31]}}, i_in[30: 4]} :
                     i_shift_amount == 8'd5  ? {i_in[ 4], { 6{i_in[31]}}, i_in[30: 5]} :
                     i_shift_amount == 8'd6  ? {i_in[ 5], { 7{i_in[31]}}, i_in[30: 6]} :
                     i_shift_amount == 8'd7  ? {i_in[ 6], { 8{i_in[31]}}, i_in[30: 7]} :
                     i_shift_amount == 8'd8  ? {i_in[ 7], { 9{i_in[31]}}, i_in[30: 8]} :
                     i_shift_amount == 8'd9  ? {i_in[ 8], {10{i_in[31]}}, i_in[30: 9]} :
 
                     i_shift_amount == 8'd10  ? {i_in[ 9], {11{i_in[31]}}, i_in[30:10]} :
                     i_shift_amount == 8'd11  ? {i_in[10], {12{i_in[31]}}, i_in[30:11]} :
                     i_shift_amount == 8'd12  ? {i_in[11], {13{i_in[31]}}, i_in[30:12]} :
                     i_shift_amount == 8'd13  ? {i_in[12], {14{i_in[31]}}, i_in[30:13]} :
                     i_shift_amount == 8'd14  ? {i_in[13], {15{i_in[31]}}, i_in[30:14]} :
                     i_shift_amount == 8'd15  ? {i_in[14], {16{i_in[31]}}, i_in[30:15]} :
                     i_shift_amount == 8'd16  ? {i_in[15], {17{i_in[31]}}, i_in[30:16]} :
                     i_shift_amount == 8'd17  ? {i_in[16], {18{i_in[31]}}, i_in[30:17]} :
                     i_shift_amount == 8'd18  ? {i_in[17], {19{i_in[31]}}, i_in[30:18]} :
                     i_shift_amount == 8'd19  ? {i_in[18], {20{i_in[31]}}, i_in[30:19]} :
 
                     i_shift_amount == 8'd20  ? {i_in[19], {21{i_in[31]}}, i_in[30:20]} :
                     i_shift_amount == 8'd21  ? {i_in[20], {22{i_in[31]}}, i_in[30:21]} :
                     i_shift_amount == 8'd22  ? {i_in[21], {23{i_in[31]}}, i_in[30:22]} :
                     i_shift_amount == 8'd23  ? {i_in[22], {24{i_in[31]}}, i_in[30:23]} :
                     i_shift_amount == 8'd24  ? {i_in[23], {25{i_in[31]}}, i_in[30:24]} :
                     i_shift_amount == 8'd25  ? {i_in[24], {26{i_in[31]}}, i_in[30:25]} :
                     i_shift_amount == 8'd26  ? {i_in[25], {27{i_in[31]}}, i_in[30:26]} :
                     i_shift_amount == 8'd27  ? {i_in[26], {28{i_in[31]}}, i_in[30:27]} :
                     i_shift_amount == 8'd28  ? {i_in[27], {29{i_in[31]}}, i_in[30:28]} :
                     i_shift_amount == 8'd29  ? {i_in[28], {30{i_in[31]}}, i_in[30:29]} :
                     i_shift_amount == 8'd30  ? {i_in[29], {31{i_in[31]}}, i_in[30   ]} :
                     i_shift_amount == 8'd31  ? {i_in[30], {32{i_in[31]}}             } :
                                                {i_in[31], {32{i_in[31]}}             } ; // >= 32
  
 
                                                  // carry out, < ------- out --------->
    assign ror_out = i_shift_imm_zero              ? {i_in[ 0], i_carry_in,  i_in[31: 1]} :  // RXR, (ROR w/ imm 0)
 
                     i_shift_amount[7:0] == 8'd0  ? {i_carry_in, i_in                  } :  // fall through case
 
                     i_shift_amount[4:0] == 5'd0  ? {i_in[31], i_in                    } :  // Rs > 31
                     i_shift_amount[4:0] == 5'd1  ? {i_in[ 0], i_in[    0], i_in[31: 1]} :
                     i_shift_amount[4:0] == 5'd2  ? {i_in[ 1], i_in[ 1: 0], i_in[31: 2]} :
                     i_shift_amount[4:0] == 5'd3  ? {i_in[ 2], i_in[ 2: 0], i_in[31: 3]} :
                     i_shift_amount[4:0] == 5'd4  ? {i_in[ 3], i_in[ 3: 0], i_in[31: 4]} :
                     i_shift_amount[4:0] == 5'd5  ? {i_in[ 4], i_in[ 4: 0], i_in[31: 5]} :
                     i_shift_amount[4:0] == 5'd6  ? {i_in[ 5], i_in[ 5: 0], i_in[31: 6]} :
                     i_shift_amount[4:0] == 5'd7  ? {i_in[ 6], i_in[ 6: 0], i_in[31: 7]} :
                     i_shift_amount[4:0] == 5'd8  ? {i_in[ 7], i_in[ 7: 0], i_in[31: 8]} :
                     i_shift_amount[4:0] == 5'd9  ? {i_in[ 8], i_in[ 8: 0], i_in[31: 9]} :
 
                     i_shift_amount[4:0] == 5'd10  ? {i_in[ 9], i_in[ 9: 0], i_in[31:10]} :
                     i_shift_amount[4:0] == 5'd11  ? {i_in[10], i_in[10: 0], i_in[31:11]} :
                     i_shift_amount[4:0] == 5'd12  ? {i_in[11], i_in[11: 0], i_in[31:12]} :
                     i_shift_amount[4:0] == 5'd13  ? {i_in[12], i_in[12: 0], i_in[31:13]} :
                     i_shift_amount[4:0] == 5'd14  ? {i_in[13], i_in[13: 0], i_in[31:14]} :
                     i_shift_amount[4:0] == 5'd15  ? {i_in[14], i_in[14: 0], i_in[31:15]} :
                     i_shift_amount[4:0] == 5'd16  ? {i_in[15], i_in[15: 0], i_in[31:16]} :
                     i_shift_amount[4:0] == 5'd17  ? {i_in[16], i_in[16: 0], i_in[31:17]} :
                     i_shift_amount[4:0] == 5'd18  ? {i_in[17], i_in[17: 0], i_in[31:18]} :
                     i_shift_amount[4:0] == 5'd19  ? {i_in[18], i_in[18: 0], i_in[31:19]} :
 
                     i_shift_amount[4:0] == 5'd20  ? {i_in[19], i_in[19: 0], i_in[31:20]} :
                     i_shift_amount[4:0] == 5'd21  ? {i_in[20], i_in[20: 0], i_in[31:21]} :
                     i_shift_amount[4:0] == 5'd22  ? {i_in[21], i_in[21: 0], i_in[31:22]} :
                     i_shift_amount[4:0] == 5'd23  ? {i_in[22], i_in[22: 0], i_in[31:23]} :
                     i_shift_amount[4:0] == 5'd24  ? {i_in[23], i_in[23: 0], i_in[31:24]} :
                     i_shift_amount[4:0] == 5'd25  ? {i_in[24], i_in[24: 0], i_in[31:25]} :
                     i_shift_amount[4:0] == 5'd26  ? {i_in[25], i_in[25: 0], i_in[31:26]} :
                     i_shift_amount[4:0] == 5'd27  ? {i_in[26], i_in[26: 0], i_in[31:27]} :
                     i_shift_amount[4:0] == 5'd28  ? {i_in[27], i_in[27: 0], i_in[31:28]} :
                     i_shift_amount[4:0] == 5'd29  ? {i_in[28], i_in[28: 0], i_in[31:29]} :
 
                     i_shift_amount[4:0] == 5'd30  ? {i_in[29], i_in[29: 0], i_in[31:30]} :
                                                     {i_in[30], i_in[30: 0], i_in[31:31]} ;
  
assign {o_carry_out, o_out} = i_function == LSL ? lsl_out :
                              i_function == LSR ? lsr_out :
                              i_function == ASR ? asr_out :
                                                  ror_out ;
 
endmodule


module a25_barrel_shift (
 
			i_clk,
			i_in,
			i_carry_in,
			i_shift_amount,     
			i_shift_imm_zero,  
			i_function,
 
			o_out,
			o_carry_out,
			o_stall
 
);

/************************* IO Declarations *********************/ 
input                       i_clk;
input       [31:0]          i_in;
input                       i_carry_in;
input       [7:0]           i_shift_amount;    // uses 8 LSBs of Rs, or a 5 bit immediate constant
input                       i_shift_imm_zero;   // high when immediate shift value of zero selected
input       [1:0]           i_function;
 
output      [31:0]          o_out;
output                      o_carry_out;
output                      o_stall;

/************************* IO Declarations *********************/ 
wire [31:0] quick_out;
wire        quick_carry_out;
wire [31:0] full_out;
wire        full_carry_out;
reg  [31:0] full_out_r       = 32'd0;
reg         full_carry_out_r = 1'd0;
reg         use_quick_r      = 1'd1;
 
 
assign o_stall      = (|i_shift_amount[7:2]) & use_quick_r;
assign o_out        = use_quick_r ? quick_out : full_out_r;
assign o_carry_out  = use_quick_r ? quick_carry_out : full_carry_out_r;
 
 
// Capture the result from the full barrel shifter in case the
// quick shifter gives the wrong value
always @(posedge i_clk)
    begin
    full_out_r       <= full_out;
    full_carry_out_r <= full_carry_out;
    use_quick_r      <= !o_stall;
    end
 
// Full barrel shifter
a25_shifter_full u_shifter_full (
    .i_in               ( i_in             ),
    .i_carry_in         ( i_carry_in       ),
    .i_shift_amount     ( i_shift_amount   ),   
    .i_shift_imm_zero   ( i_shift_imm_zero ), 
    .i_function         ( i_function       ),
    .o_out              ( full_out         ),
    .o_carry_out        ( full_carry_out   )
);
 
 
 
// Quick barrel shifter
a25_shifter_quick u_shifter_quick (
    .i_in               ( i_in             ),
    .i_carry_in         ( i_carry_in       ),
    .i_shift_amount     ( i_shift_amount   ),   
    .i_shift_imm_zero   ( i_shift_imm_zero ), 
    .i_function         ( i_function       ),
    .o_out              ( quick_out        ),
    .o_carry_out        ( quick_carry_out  )
);

endmodule
 
 
 
 
 
module a25_register_bank (
 
			i_clk,
			i_core_stall,
			i_mem_stall,
 
			i_mode_idec,          
                                               
			i_mode_exec,            
                                           
			i_mode_rds_exec,      
                                                  
			i_firq_not_user_mode,
			i_rm_sel,
			i_rs_sel,
			i_rn_sel,
 
			i_pc_wen,
			i_reg_bank_wen,
 
			i_pc,                
			i_reg,
 
			i_wb_read_data,
			i_wb_read_data_valid,
			i_wb_read_data_rd,
			i_wb_mode,
 
			i_status_bits_flags,
			i_status_bits_irq_mask,
			i_status_bits_firq_mask,
 
			o_rm,
			o_rs,
			o_rd,
			o_rn,
			o_pc
 
			);
 
//`include "a25/a25_localparams.v"
//////////////////////////////////////////////////////////////////
//                                                              //
//  Parameters file for Amber 25 Core                           //
//                                                              //
//  This file is part of the Amber project                      //
//  http://www.opencores.org/project,amber                      //
//                                                              //
//  Description                                                 //
//  Holds general parameters that are used is several core      //
//  modules                                                     //
//                                                              //
//  Author(s):                                                  //
//      - Conor Santifort, csantifort.amber@gmail.com           //
//                                                              //
//////////////////////////////////////////////////////////////////
//                                                              //
// Copyright (C) 2011 Authors and OPENCORES.ORG                 //
//                                                              //
// This source file may be used and distributed without         //
// restriction provided that this copyright statement is not    //
// removed from the file and that any derivative work contains  //
// the original copyright notice and the associated disclaimer. //
//                                                              //
// This source file is free software; you can redistribute it   //
// and/or modify it under the terms of the GNU Lesser General   //
// Public License as published by the Free Software Foundation; //
// either version 2.1 of the License, or (at your option) any   //
// later version.                                               //
//                                                              //
// This source is distributed in the hope that it will be       //
// useful, but WITHOUT ANY WARRANTY; without even the implied   //
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      //
// PURPOSE.  See the GNU Lesser General Public License for more //
// details.                                                     //
//                                                              //
// You should have received a copy of the GNU Lesser General    //
// Public License along with this source; if not, download it   //
// from http://www.opencores.org/lgpl.shtml                     //
//                                                              //
//////////////////////////////////////////////////////////////////
 
 
// Instruction Types
localparam [3:0]    REGOP       = 4'h0, // Data processing
                    MULT        = 4'h1, // Multiply
                    SWAP        = 4'h2, // Single Data Swap
                    TRANS       = 4'h3, // Single data transfer
                    MTRANS      = 4'h4, // Multi-word data transfer
                    BRANCH      = 4'h5, // Branch
                    CODTRANS    = 4'h6, // Co-processor data transfer
                    COREGOP     = 4'h7, // Co-processor data operation
                    CORTRANS    = 4'h8, // Co-processor register transfer
                    SWI         = 4'h9; // software interrupt
 
 
// Opcodes
localparam [3:0] AND = 4'h0,        // Logical AND
                 EOR = 4'h1,        // Logical Exclusive OR
                 SUB = 4'h2,        // Subtract
                 RSB = 4'h3,        // Reverse Subtract
                 ADD = 4'h4,        // Add
                 ADC = 4'h5,        // Add with Carry
                 SBC = 4'h6,        // Subtract with Carry
                 RSC = 4'h7,        // Reverse Subtract with Carry
                 TST = 4'h8,        // Test  (using AND operator)
                 TEQ = 4'h9,        // Test Equivalence (using EOR operator)
                 CMP = 4'ha,       // Compare (using Subtract operator)
                 CMN = 4'hb,       // Compare Negated
                 ORR = 4'hc,       // Logical OR
                 MOV = 4'hd,       // Move
                 BIC = 4'he,       // Bit Clear (using AND & NOT operators)
                 MVN = 4'hf;       // Move NOT
 
// Condition Encoding
localparam [3:0] EQ  = 4'h0,        // Equal            / Z set
                 NE  = 4'h1,        // Not equal        / Z clear
                 CS  = 4'h2,        // Carry set        / C set
                 CC  = 4'h3,        // Carry clear      / C clear
                 MI  = 4'h4,        // Minus            / N set
                 PL  = 4'h5,        // Plus             / N clear
                 VS  = 4'h6,        // Overflow         / V set
                 VC  = 4'h7,        // No overflow      / V clear
                 HI  = 4'h8,        // Unsigned higher  / C set and Z clear
                 LS  = 4'h9,        // Unsigned lower
                                    // or same          / C clear or Z set
                 GE  = 4'ha,        // Signed greater 
                                    // than or equal    / N == V
                 LT  = 4'hb,        // Signed less than / N != V
                 GT  = 4'hc,        // Signed greater
                                    // than             / Z == 0, N == V
                 LE  = 4'hd,        // Signed less than
                                    // or equal         / Z == 1, N != V
                 AL  = 4'he,        // Always
                 NV  = 4'hf;        // Never
 
// Any instruction with a condition field of 0b1111 is UNPREDICTABLE.                
 
// Shift Types
localparam [1:0] LSL = 2'h0,
                 LSR = 2'h1,
                 ASR = 2'h2,
                 RRX = 2'h3,
                 ROR = 2'h3; 
 
// Modes
localparam [1:0] SVC  =  2'b11,  // Supervisor
                 IRQ  =  2'b10,  // Interrupt
                 FIRQ =  2'b01,  // Fast Interrupt
                 USR  =  2'b00;  // User
 
// One-Hot Mode encodings
localparam [5:0] OH_USR  = 0,
                 OH_IRQ  = 1,
                 OH_FIRQ = 2,
                 OH_SVC  = 3;
 
 
//`include "a25/a25_functions.v"

input                       i_clk;
input                       i_core_stall;
input                       i_mem_stall;
 
input       [1:0]           i_mode_idec;            // user, supervisor, irq_idec, firq_idec etc.
                                                    // Used for register writes
input       [1:0]           i_mode_exec;            // 1 periods delayed from i_mode_idec
                                                    // Used for register reads
input       [3:0]           i_mode_rds_exec;        // Use one-hot version specifically for rds, 
                                                    // includes i_user_mode_regs_store
input                       i_firq_not_user_mode;
input       [3:0]           i_rm_sel;
input       [3:0]           i_rs_sel;
input       [3:0]           i_rn_sel;
 
input                       i_pc_wen;
input       [14:0]          i_reg_bank_wen;
 
input       [23:0]          i_pc;                   // program counter [25:2]
input       [31:0]          i_reg;
 
input       [31:0]          i_wb_read_data;
input                       i_wb_read_data_valid;
input       [3:0]           i_wb_read_data_rd;
input       [1:0]           i_wb_mode;
 
input       [3:0]           i_status_bits_flags;
input                       i_status_bits_irq_mask;
input                       i_status_bits_firq_mask;
 
output      [31:0]          o_rm;
output      [31:0]          o_rs;
output      [31:0]          o_rd;
output      [31:0]          o_rn;
output      [31:0]          o_pc;

reg  	    [31:0]          o_rs;  //
reg   	    [31:0]          o_rd;  //

// User Mode Registers
reg  [31:0] r0  = 32'hdeadbeef;
reg  [31:0] r1  = 32'hdeadbeef;
reg  [31:0] r2  = 32'hdeadbeef;
reg  [31:0] r3  = 32'hdeadbeef;
reg  [31:0] r4  = 32'hdeadbeef;
reg  [31:0] r5  = 32'hdeadbeef;
reg  [31:0] r6  = 32'hdeadbeef;
reg  [31:0] r7  = 32'hdeadbeef;
reg  [31:0] r8  = 32'hdeadbeef;
reg  [31:0] r9  = 32'hdeadbeef;
reg  [31:0] r10 = 32'hdeadbeef;
reg  [31:0] r11 = 32'hdeadbeef;
reg  [31:0] r12 = 32'hdeadbeef;
reg  [31:0] r13 = 32'hdeadbeef;
reg  [31:0] r14 = 32'hdeadbeef;
reg  [23:0] r15 = 24'hc0ffee;
 
wire  [31:0] r0_out;
wire  [31:0] r1_out;
wire  [31:0] r2_out;
wire  [31:0] r3_out;
wire  [31:0] r4_out;
wire  [31:0] r5_out;
wire  [31:0] r6_out;
wire  [31:0] r7_out;
wire  [31:0] r8_out;
wire  [31:0] r9_out;
wire  [31:0] r10_out;
wire  [31:0] r11_out;
wire  [31:0] r12_out;
wire  [31:0] r13_out;
wire  [31:0] r14_out;
wire  [31:0] r15_out_rm;
wire  [31:0] r15_out_rm_nxt;
wire  [31:0] r15_out_rn;
 
wire  [31:0] r8_rds;
wire  [31:0] r9_rds;
wire  [31:0] r10_rds;
wire  [31:0] r11_rds;
wire  [31:0] r12_rds;
wire  [31:0] r13_rds;
wire  [31:0] r14_rds;
 
// Supervisor Mode Registers
reg  [31:0] r13_svc = 32'hdeadbeef;
reg  [31:0] r14_svc = 32'hdeadbeef;
 
// Interrupt Mode Registers
reg  [31:0] r13_irq = 32'hdeadbeef;
reg  [31:0] r14_irq = 32'hdeadbeef;
 
// Fast Interrupt Mode Registers
reg  [31:0] r8_firq  = 32'hdeadbeef;
reg  [31:0] r9_firq  = 32'hdeadbeef;
reg  [31:0] r10_firq = 32'hdeadbeef;
reg  [31:0] r11_firq = 32'hdeadbeef;
reg  [31:0] r12_firq = 32'hdeadbeef;
reg  [31:0] r13_firq = 32'hdeadbeef;
reg  [31:0] r14_firq = 32'hdeadbeef;
 
wire        usr_exec;
wire        svc_exec;
wire        irq_exec;
wire        firq_exec;
 
wire        usr_idec;
wire        svc_idec;
wire        irq_idec;
wire        firq_idec;
wire [14:0] read_data_wen;
wire [14:0] reg_bank_wen_c;
wire        pc_wen_c;
wire        pc_dmem_wen;

reg [14:0] decode;  //jingjing
 
 
    // Write Enables from execute stage
assign usr_idec  = i_mode_idec == USR;
assign svc_idec  = i_mode_idec == SVC;
assign irq_idec  = i_mode_idec == IRQ;
 
// pre-encoded in decode stage to speed up long path
assign firq_idec = i_firq_not_user_mode;
 
    // Read Enables from stage 1 (fetch)
assign usr_exec  = i_mode_exec == USR;
assign svc_exec  = i_mode_exec == SVC;
assign irq_exec  = i_mode_exec == IRQ;
assign firq_exec = i_mode_exec == FIRQ;

always @*
case(i_wb_read_data_rd)
	4'h0  : decode = 15'h0001  ;
	4'h1  : decode = 15'h0002  ;
	4'h2  : decode = 15'h0004  ;
	4'h3  : decode = 15'h0008  ;
	4'h4  : decode = 15'h0010  ;
	4'h5  : decode = 15'h0020  ;
	4'h6  : decode = 15'h0040  ;
	4'h7  : decode = 15'h0080  ;
	4'h8  : decode = 15'h0100  ;
	4'h9  : decode = 15'h0200  ;
	4'ha  : decode = 15'h0400  ;
	4'hb  : decode = 15'h0800  ;
	4'hc  : decode = 15'h1000  ;
	4'hd  : decode = 15'h2000  ;
	4'he  : decode = 15'h4000  ;
	default: decode = 15'h0000  ;
endcase


/* i_wb_read_data_rd == 4'h0  ? 15'h0001  :
		 i_wb_read_data_rd == 4'h1  ? 15'h0002  :
		 i_wb_read_data_rd == 4'h2  ? 15'h0004  :
		 i_wb_read_data_rd == 4'h3  ? 15'h0008  :
		 i_wb_read_data_rd == 4'h4  ? 15'h0010  :
		 i_wb_read_data_rd == 4'h5  ? 15'h0020  :

		 i_wb_read_data_rd == 4'h6  ? 15'h0040  :
		 i_wb_read_data_rd == 4'h7  ? 15'h0080  :
		 i_wb_read_data_rd == 4'h8  ? 15'h0100  :
		 i_wb_read_data_rd == 4'h9  ? 15'h0200  :
		 i_wb_read_data_rd == 4'ha  ? 15'h0400  :
		 i_wb_read_data_rd == 4'hb  ? 15'h0800  :
		 i_wb_read_data_rd == 4'hc  ? 15'h1000  :
		 i_wb_read_data_rd == 4'hd  ? 15'h2000  :
		 i_wb_read_data_rd == 4'he  ? 15'h4000  :
			         default:     15'h0000  ;
*/
//& decode (i_wb_read_data_rd); 
assign read_data_wen = {15{i_wb_read_data_valid & ~i_mem_stall}} 
			& decode;


				
assign reg_bank_wen_c = {15{~i_core_stall}} & i_reg_bank_wen;
assign pc_wen_c       = ~i_core_stall & i_pc_wen;
assign pc_dmem_wen    = i_wb_read_data_valid & ~i_mem_stall & i_wb_read_data_rd == 4'd15;
 
 
// ========================================================
// Register Update
// ========================================================


always @ ( posedge i_clk )
    begin
    // these registers are used in all modes
    r0       <= reg_bank_wen_c[0 ]               ? i_reg : read_data_wen[0 ]                      ? i_wb_read_data       : r0;  
    r1       <= reg_bank_wen_c[1 ]               ? i_reg : read_data_wen[1 ]                      ? i_wb_read_data       : r1;  
    r2       <= reg_bank_wen_c[2 ]               ? i_reg : read_data_wen[2 ]                      ? i_wb_read_data       : r2;  
    r3       <= reg_bank_wen_c[3 ]               ? i_reg : read_data_wen[3 ]                      ? i_wb_read_data       : r3;  
    r4       <= reg_bank_wen_c[4 ]               ? i_reg : read_data_wen[4 ]                      ? i_wb_read_data       : r4;  
    r5       <= reg_bank_wen_c[5 ]               ? i_reg : read_data_wen[5 ]                      ? i_wb_read_data       : r5;  
    r6       <= reg_bank_wen_c[6 ]               ? i_reg : read_data_wen[6 ]                      ? i_wb_read_data       : r6;  
    r7       <= reg_bank_wen_c[7 ]               ? i_reg : read_data_wen[7 ]                      ? i_wb_read_data       : r7;  
 
    // these registers are used in all modes, except fast irq
    r8       <= reg_bank_wen_c[8 ] && !firq_idec ? i_reg : read_data_wen[8 ] && i_wb_mode != FIRQ ? i_wb_read_data       : r8;  
    r9       <= reg_bank_wen_c[9 ] && !firq_idec ? i_reg : read_data_wen[9 ] && i_wb_mode != FIRQ ? i_wb_read_data       : r9;  
    r10      <= reg_bank_wen_c[10] && !firq_idec ? i_reg : read_data_wen[10] && i_wb_mode != FIRQ ? i_wb_read_data       : r10; 
    r11      <= reg_bank_wen_c[11] && !firq_idec ? i_reg : read_data_wen[11] && i_wb_mode != FIRQ ? i_wb_read_data       : r11; 
    r12      <= reg_bank_wen_c[12] && !firq_idec ? i_reg : read_data_wen[12] && i_wb_mode != FIRQ ? i_wb_read_data       : r12; 
 
    // these registers are used in fast irq mode
    r8_firq  <= reg_bank_wen_c[8 ] &&  firq_idec ? i_reg : read_data_wen[8 ] && i_wb_mode == FIRQ ? i_wb_read_data       : r8_firq;
    r9_firq  <= reg_bank_wen_c[9 ] &&  firq_idec ? i_reg : read_data_wen[9 ] && i_wb_mode == FIRQ ? i_wb_read_data       : r9_firq;
    r10_firq <= reg_bank_wen_c[10] &&  firq_idec ? i_reg : read_data_wen[10] && i_wb_mode == FIRQ ? i_wb_read_data       : r10_firq;
    r11_firq <= reg_bank_wen_c[11] &&  firq_idec ? i_reg : read_data_wen[11] && i_wb_mode == FIRQ ? i_wb_read_data       : r11_firq;
    r12_firq <= reg_bank_wen_c[12] &&  firq_idec ? i_reg : read_data_wen[12] && i_wb_mode == FIRQ ? i_wb_read_data       : r12_firq;
 
    // these registers are used in user mode
    r13      <= reg_bank_wen_c[13] &&  usr_idec  ? i_reg : read_data_wen[13] && i_wb_mode == USR ? i_wb_read_data        : r13;         
    r14      <= reg_bank_wen_c[14] &&  usr_idec  ? i_reg : read_data_wen[14] && i_wb_mode == USR ? i_wb_read_data        : r14;         
 
    // these registers are used in supervisor mode
    r13_svc  <= reg_bank_wen_c[13] &&  svc_idec  ? i_reg : read_data_wen[13] && i_wb_mode == SVC  ? i_wb_read_data       : r13_svc;     
    r14_svc  <= reg_bank_wen_c[14] &&  svc_idec  ? i_reg : read_data_wen[14] && i_wb_mode == SVC  ? i_wb_read_data       : r14_svc;     
 
    // these registers are used in irq mode
    r13_irq  <= reg_bank_wen_c[13] &&  irq_idec  ? i_reg : read_data_wen[13] && i_wb_mode == IRQ  ? i_wb_read_data       : r13_irq; 
    r14_irq  <= (reg_bank_wen_c[14] && irq_idec) ? i_reg : read_data_wen[14] && i_wb_mode == IRQ  ? i_wb_read_data       : r14_irq;      
 
    // these registers are used in fast irq mode
    r13_firq <= reg_bank_wen_c[13] &&  firq_idec ? i_reg : read_data_wen[13] && i_wb_mode == FIRQ ? i_wb_read_data       : r13_firq;
    r14_firq <= reg_bank_wen_c[14] &&  firq_idec ? i_reg : read_data_wen[14] && i_wb_mode == FIRQ ? i_wb_read_data       : r14_firq;  
 
    // these registers are used in all modes
    r15      <= pc_wen_c                         ?  i_pc : pc_dmem_wen                            ? i_wb_read_data[25:2] : r15;

    end
 
 
// ========================================================
// Register Read based on Mode
// ========================================================
assign r0_out = r0;
assign r1_out = r1;
assign r2_out = r2;
assign r3_out = r3;
assign r4_out = r4;
assign r5_out = r5;
assign r6_out = r6;
assign r7_out = r7;
 
assign r8_out  = firq_exec ? r8_firq  : r8;
assign r9_out  = firq_exec ? r9_firq  : r9;
assign r10_out = firq_exec ? r10_firq : r10;
assign r11_out = firq_exec ? r11_firq : r11;
assign r12_out = firq_exec ? r12_firq : r12;
 
assign r13_out = usr_exec ? r13      :
                 svc_exec ? r13_svc  :
                 irq_exec ? r13_irq  :
                          r13_firq ;
 
assign r14_out = usr_exec ? r14      :
                 svc_exec ? r14_svc  :
                 irq_exec ? r14_irq  :
                          r14_firq ;
 
 
assign r15_out_rm     = { i_status_bits_flags, 
                          i_status_bits_irq_mask, 
                          i_status_bits_firq_mask, 
                          r15, 
                          i_mode_exec};
 
assign r15_out_rm_nxt = { i_status_bits_flags, 
                          i_status_bits_irq_mask, 
                          i_status_bits_firq_mask, 
                          i_pc, 
                          i_mode_exec};
 
assign r15_out_rn     = {6'd0, r15, 2'd0};
 
 
// rds outputs
assign r8_rds  = i_mode_rds_exec[OH_FIRQ] ? r8_firq  : r8;
assign r9_rds  = i_mode_rds_exec[OH_FIRQ] ? r9_firq  : r9;
assign r10_rds = i_mode_rds_exec[OH_FIRQ] ? r10_firq : r10;
assign r11_rds = i_mode_rds_exec[OH_FIRQ] ? r11_firq : r11;
assign r12_rds = i_mode_rds_exec[OH_FIRQ] ? r12_firq : r12;
 
assign r13_rds = i_mode_rds_exec[OH_USR]  ? r13      :
                 i_mode_rds_exec[OH_SVC]  ? r13_svc  :
                 i_mode_rds_exec[OH_IRQ]  ? r13_irq  :
                                            r13_firq ;
 
assign r14_rds = i_mode_rds_exec[OH_USR]  ? r14      :
                 i_mode_rds_exec[OH_SVC]  ? r14_svc  :
                 i_mode_rds_exec[OH_IRQ]  ? r14_irq  :
                                            r14_firq ;
 
 
// ========================================================
// Program Counter out
// ========================================================
assign o_pc = r15_out_rn;
 
// ========================================================
// Rm Selector
// ========================================================
assign o_rm = i_rm_sel == 4'd0  ? r0_out  :
              i_rm_sel == 4'd1  ? r1_out  : 
              i_rm_sel == 4'd2  ? r2_out  : 
              i_rm_sel == 4'd3  ? r3_out  : 
              i_rm_sel == 4'd4  ? r4_out  : 
              i_rm_sel == 4'd5  ? r5_out  : 
              i_rm_sel == 4'd6  ? r6_out  : 
              i_rm_sel == 4'd7  ? r7_out  : 
              i_rm_sel == 4'd8  ? r8_out  : 
              i_rm_sel == 4'd9  ? r9_out  : 
              i_rm_sel == 4'd10 ? r10_out : 
              i_rm_sel == 4'd11 ? r11_out : 
              i_rm_sel == 4'd12 ? r12_out : 
              i_rm_sel == 4'd13 ? r13_out : 
              i_rm_sel == 4'd14 ? r14_out : 
               			  r15_out_rm ; 
 
 
// ========================================================
// Rds Selector
// ========================================================
always @*
    case ( i_rs_sel )
       4'd0  :  o_rs = r0_out  ;
       4'd1  :  o_rs = r1_out  ; 
       4'd2  :  o_rs = r2_out  ; 
       4'd3  :  o_rs = r3_out  ; 
       4'd4  :  o_rs = r4_out  ; 
       4'd5  :  o_rs = r5_out  ; 
       4'd6  :  o_rs = r6_out  ; 
       4'd7  :  o_rs = r7_out  ; 
       4'd8  :  o_rs = r8_rds  ; 
       4'd9  :  o_rs = r9_rds  ; 
       4'd10 :  o_rs = r10_rds ; 
       4'd11 :  o_rs = r11_rds ; 
       4'd12 :  o_rs = r12_rds ; 
       4'd13 :  o_rs = r13_rds ; 
       4'd14 :  o_rs = r14_rds ; 
      default:  o_rs = r15_out_rn ; 
    endcase


 
// ========================================================
// Rd Selector
// ========================================================

always @*
    case ( i_rs_sel )
       4'd0  :  o_rd = r0_out  ;
       4'd1  :  o_rd = r1_out  ; 
       4'd2  :  o_rd = r2_out  ; 
       4'd3  :  o_rd = r3_out  ; 
       4'd4  :  o_rd = r4_out  ; 
       4'd5  :  o_rd = r5_out  ; 
       4'd6  :  o_rd = r6_out  ; 
       4'd7  :  o_rd = r7_out  ; 
       4'd8  :  o_rd = r8_rds  ; 
       4'd9  :  o_rd = r9_rds  ; 
       4'd10 :  o_rd = r10_rds ; 
       4'd11 :  o_rd = r11_rds ; 
       4'd12 :  o_rd = r12_rds ; 
       4'd13 :  o_rd = r13_rds ; 
       4'd14 :  o_rd = r14_rds ; 
       default: o_rd = r15_out_rm_nxt ; 
    endcase
 
 
// ========================================================
// Rn Selector
// ========================================================
assign o_rn = i_rn_sel == 4'd0  ? r0_out  :
              i_rn_sel == 4'd1  ? r1_out  : 
              i_rn_sel == 4'd2  ? r2_out  : 
              i_rn_sel == 4'd3  ? r3_out  : 
              i_rn_sel == 4'd4  ? r4_out  : 
              i_rn_sel == 4'd5  ? r5_out  : 
              i_rn_sel == 4'd6  ? r6_out  : 
              i_rn_sel == 4'd7  ? r7_out  : 
              i_rn_sel == 4'd8  ? r8_out  : 
              i_rn_sel == 4'd9  ? r9_out  : 
              i_rn_sel == 4'd10 ? r10_out : 
              i_rn_sel == 4'd11 ? r11_out : 
              i_rn_sel == 4'd12 ? r12_out : 
              i_rn_sel == 4'd13 ? r13_out : 
              i_rn_sel == 4'd14 ? r14_out : 
              			  r15_out_rn ; 
 
 
endmodule
 

module a25_multiply (
			i_clk,
			i_core_stall,
 
			i_a_in,        
			i_b_in,       
			i_function,
			i_execute,
 
			o_out,
			o_flags,        
			o_done                                  
		);
 
input                       i_clk;
input                       i_core_stall;
 
input       [31:0]          i_a_in;         // Rds
input       [31:0]          i_b_in;         // Rm
input       [1:0]           i_function;
input                       i_execute;
 
output      [31:0]          o_out;
output      [1:0]           o_flags;        // [1] = N, [0] = Z
output                      o_done;   // goes high 2 cycles before completion    
 
reg         o_done = 1'd0;
wire	    enable;
wire        accumulate;
wire [33:0] multiplier;
wire [33:0] multiplier_bar;
wire [33:0] sum;
wire [33:0] sum34_b;
 
reg  [5:0]  count = 6'd0;
reg  [5:0]  count_nxt;
reg  [67:0] product = 68'd0;
reg  [67:0] product_nxt;
reg  [1:0]  flags_nxt;
wire [32:0] sum_acc1;           // the MSB is the carry out for the upper 32 bit addition
 
 
assign enable         = i_function[0];
assign accumulate     = i_function[1];
 
assign multiplier     =  { 2'd0, i_a_in} ;
assign multiplier_bar = ~{ 2'd0, i_a_in} + 34'd1 ;
 
assign sum34_b        =  product[1:0] == 2'b01 ? multiplier     :
                         product[1:0] == 2'b10 ? multiplier_bar :
                                                 34'd0          ;
 

    // -----------------------------------
    // 34-bit adder - booth multiplication
    // -----------------------------------
    assign sum =  product[67:34] + sum34_b;
 
    // ------------------------------------
    // 33-bit adder - accumulate operations
    // ------------------------------------
    assign sum_acc1 = {1'd0, product[32:1]} + {1'd0, i_a_in};

  //  assign count_nxt = count;
  
always @*
    begin

 
    // update Negative and Zero flags
    // Use registered value of product so this adds an extra cycle
    // but this avoids having the 64-bit zero comparator on the
    // main adder path
    flags_nxt   = { product[32], product[32:1] == 32'd0 }; 
 
 
    if ( count == 6'd0 )
        product_nxt = {33'd0, 1'd0, i_b_in, 1'd0 } ;
    else if ( count <= 6'd33 )
        product_nxt = { sum[33], sum, product[33:1]} ;
    else if ( count == 6'd34 && accumulate )
        begin
        // Note that bit 0 is not part of the product. It is used during the booth
        // multiplication algorithm
        product_nxt         = { product[64:33], sum_acc1[31:0], 1'd0}; // Accumulate
        end
    else
        product_nxt         = product;

 
    // Multiplication state counter
    if (count == 6'd0)  // start
        count_nxt   = enable ? 6'd1 : 6'd0;
    else if ((count == 6'd34 && !accumulate) ||  // MUL
             (count == 6'd35 &&  accumulate)  )  // MLA
        count_nxt   = 6'd0;
    else
        count_nxt   = count + 1'd1;
    end
 
 
always @ ( posedge i_clk )
    if ( !i_core_stall )
        begin
        count           <= i_execute ? count_nxt          : count;           
        product         <= i_execute ? product_nxt        : product;        
        o_done          <= i_execute ? count == 6'd31     : o_done;          
        end
 
// Outputs
assign o_out   = product[32:1]; 
assign o_flags = flags_nxt;
 
endmodule
 

 

module a25_alu (
 
		i_a_in,
		i_b_in,
		i_barrel_shift_carry,
		i_status_bits_carry,
		i_function, 
		o_out,
		o_flags
);

/************************* IO Declarations *********************/
input       [31:0]          i_a_in;
input       [31:0]          i_b_in;
input                       i_barrel_shift_carry;
input                       i_status_bits_carry;
input       [8:0]           i_function;
 
output      [31:0]          o_out;
output      [3:0]           o_flags;
 
/*********************** Signal Declarations *******************/
wire     [31:0]         a;
wire     [31:0]         b;
wire     [31:0]         b_not;
wire     [31:0]         and_out;
wire     [31:0]         or_out;
wire     [31:0]         xor_out;
wire     [31:0]         sign_ex8_out;
wire     [31:0]         sign_ex_16_out;
wire     [31:0]         zero_ex8_out;
wire     [31:0]         zero_ex_16_out;
wire     [32:0]         fadder_out;
wire                    swap_sel;
wire                    not_sel;
wire     [1:0]          cin_sel;
wire                    cout_sel;
wire     [3:0]          out_sel;
wire                    carry_in;
wire                    carry_out;
wire                    overflow_out;
wire                    fadder_carry_out;
 
assign  { swap_sel, not_sel, cin_sel, cout_sel, out_sel } = i_function;
 

assign a     = (swap_sel ) ? i_b_in : i_a_in ;
 
// ========================================================
// B Select
// ========================================================
assign b     = (swap_sel ) ? i_a_in : i_b_in ;
 
// ========================================================
// Not Select
// ========================================================
assign b_not     = (not_sel ) ? ~b : b ;
 
// ========================================================
// Cin Select
// ========================================================
assign carry_in  = (cin_sel==2'd0 ) ? 1'd0                   :
                   (cin_sel==2'd1 ) ? 1'd1                   :
                                      i_status_bits_carry    ;  // add with carry
 
// ========================================================
// Cout Select
// ========================================================
assign carry_out = (cout_sel==1'd0 ) ? fadder_carry_out     :
                                       i_barrel_shift_carry ;
 
// For non-addition/subtractions that incorporate a shift 
// operation, C is set to the last bit
// shifted out of the value by the shifter.
 
 
// ========================================================
// Overflow out
// ========================================================
// Only assert the overflow flag when using the adder
assign  overflow_out    = out_sel == 4'd1 &&
                            // overflow if adding two positive numbers and get a negative number
                          ( (!a[31] && !b_not[31] && fadder_out[31]) ||
                            // or adding two negative numbers and get a positive number
                            (a[31] && b_not[31] && !fadder_out[31])     );
 
 
// ========================================================
// ALU Operations
// ========================================================
 

assign fadder_out       = { 1'd0,a} + {1'd0,b_not} + {32'd0,carry_in};
                                              
 
assign fadder_carry_out = fadder_out[32];
assign and_out          = a & b_not;
assign or_out           = a | b_not;
assign xor_out          = a ^ b_not;
assign zero_ex8_out     = {24'd0,  b_not[7:0]};
assign zero_ex_16_out   = {16'd0,  b_not[15:0]};
assign sign_ex8_out     = {{24{b_not[7]}},  b_not[7:0]};
assign sign_ex_16_out   = {{16{b_not[15]}}, b_not[15:0]};
 
// ========================================================
// Out Select
// ========================================================
assign o_out = out_sel == 4'd0 ? b_not            : 
               out_sel == 4'd1 ? fadder_out[31:0] : 
               out_sel == 4'd2 ? zero_ex_16_out   :
               out_sel == 4'd3 ? zero_ex8_out     :
               out_sel == 4'd4 ? sign_ex_16_out   :
               out_sel == 4'd5 ? sign_ex8_out     :
               out_sel == 4'd6 ? xor_out          :
               out_sel == 4'd7 ? or_out           :
                                 and_out          ;
 
assign o_flags       = {  o_out[31],      // negative
                         |o_out == 1'd0,  // zero
                         carry_out,       // carry
                         overflow_out     // overflow
                         };
 

endmodule
 

module a25_execute (
 
i_clk,
i_core_stall,               
i_mem_stall,              
o_exec_stall,        
 
i_wb_read_data,            
i_wb_read_data_valid,   
i_wb_load_rd,              
 
i_copro_read_data,                                
i_decode_iaccess,          
i_decode_daccess,          
i_decode_load_rd,         
 
o_copro_write_data,
o_write_data,
o_iaddress,
o_iaddress_nxt,            
                                                       
o_iaddress_valid,     
o_daddress,         
o_daddress_nxt,         
                                                  
o_daddress_valid,    
o_adex,              
o_priviledged,      
o_exclusive,          
o_write_enable,
o_byte_enable,
o_exec_load_rd,      
o_status_bits,            
o_multiply_done,
 
i_status_bits_mode,
i_status_bits_irq_mask,
i_status_bits_firq_mask,
i_imm32,
i_imm_shift_amount,
i_shift_imm_zero,
i_condition,
i_decode_exclusive,    
 
i_rm_sel,
i_rs_sel,
i_rn_sel,
i_barrel_shift_amount_sel,
i_barrel_shift_data_sel,
i_barrel_shift_function,
i_alu_function,
i_multiply_function,
i_interrupt_vector_sel,
i_iaddress_sel,
i_daddress_sel,
i_pc_sel,
i_byte_enable_sel,
i_status_bits_sel,
i_reg_write_sel,
i_user_mode_regs_store_nxt,
i_firq_not_user_mode,
 
i_write_data_wen,
i_base_address_wen,   
i_pc_wen,
i_reg_bank_wen,
i_status_bits_flags_wen,
i_status_bits_mode_wen,
i_status_bits_irq_mask_wen,
i_status_bits_firq_mask_wen,
i_copro_write_data_wen,
i_conflict,
i_rn_use_read,
i_rm_use_read,
i_rs_use_read,
i_rd_use_read
);


//`include "a25/a25_localparams.v"

// Instruction Types
localparam [3:0]    REGOP       = 4'h0, // Data processing
                    MULT        = 4'h1, // Multiply
                    SWAP        = 4'h2, // Single Data Swap
                    TRANS       = 4'h3, // Single data transfer
                    MTRANS      = 4'h4, // Multi-word data transfer
                    BRANCH      = 4'h5, // Branch
                    CODTRANS    = 4'h6, // Co-processor data transfer
                    COREGOP     = 4'h7, // Co-processor data operation
                    CORTRANS    = 4'h8, // Co-processor register transfer
                    SWI         = 4'h9; // software interrupt
 
 
// Opcodes
localparam [3:0] AND = 4'h0,        // Logical AND
                 EOR = 4'h1,        // Logical Exclusive OR
                 SUB = 4'h2,        // Subtract
                 RSB = 4'h3,        // Reverse Subtract
                 ADD = 4'h4,        // Add
                 ADC = 4'h5,        // Add with Carry
                 SBC = 4'h6,        // Subtract with Carry
                 RSC = 4'h7,        // Reverse Subtract with Carry
                 TST = 4'h8,        // Test  (using AND operator)
                 TEQ = 4'h9,        // Test Equivalence (using EOR operator)
                 CMP = 4'ha,       // Compare (using Subtract operator)
                 CMN = 4'hb,       // Compare Negated
                 ORR = 4'hc,       // Logical OR
                 MOV = 4'hd,       // Move
                 BIC = 4'he,       // Bit Clear (using AND & NOT operators)
                 MVN = 4'hf;       // Move NOT
 
// Condition Encoding
localparam [3:0] EQ  = 4'h0,        // Equal            / Z set
                 NE  = 4'h1,        // Not equal        / Z clear
                 CS  = 4'h2,        // Carry set        / C set
                 CC  = 4'h3,        // Carry clear      / C clear
                 MI  = 4'h4,        // Minus            / N set
                 PL  = 4'h5,        // Plus             / N clear
                 VS  = 4'h6,        // Overflow         / V set
                 VC  = 4'h7,        // No overflow      / V clear
                 HI  = 4'h8,        // Unsigned higher  / C set and Z clear
                 LS  = 4'h9,        // Unsigned lower
                                    // or same          / C clear or Z set
                 GE  = 4'ha,        // Signed greater 
                                    // than or equal    / N == V
                 LT  = 4'hb,        // Signed less than / N != V
                 GT  = 4'hc,        // Signed greater
                                    // than             / Z == 0, N == V
                 LE  = 4'hd,        // Signed less than
                                    // or equal         / Z == 1, N != V
                 AL  = 4'he,        // Always
                 NV  = 4'hf;        // Never
 
// Any instruction with a condition field of 0b1111 is UNPREDICTABLE.                
 
// Shift Types
localparam [1:0] LSL = 2'h0,
                 LSR = 2'h1,
                 ASR = 2'h2,
                 RRX = 2'h3,
                 ROR = 2'h3; 
 
// Modes
localparam [1:0] SVC  =  2'b11,  // Supervisor
                 IRQ  =  2'b10,  // Interrupt
                 FIRQ =  2'b01,  // Fast Interrupt
                 USR  =  2'b00;  // User
 
// One-Hot Mode encodings
localparam [5:0] OH_USR  = 6'b0,	//0
                 OH_IRQ  = 6'b1,	//1
                 OH_FIRQ = 6'b10,	//2
                 OH_SVC  = 6'b11;	//3
 
 
//`include "a25/a25_functions.v"

input                       i_clk;
input                       i_core_stall;               // stall all stages of the Amber core at the same time
input                       i_mem_stall;                // data memory access stalls
output                      o_exec_stall;               // stall the core pipeline
 
input       [31:0]          i_wb_read_data;             // data reads
input                       i_wb_read_data_valid;       // read data is valid
input       [10:0]          i_wb_load_rd;               // Rd for data reads
 
input       [31:0]          i_copro_read_data;          // From Co-Processor, to either Register 
                                                        // or Memory
input                       i_decode_iaccess;           // Indicates an instruction access
input                       i_decode_daccess;           // Indicates a data access
input       [7:0]           i_decode_load_rd;           // The destination register for a load instruction
 
output      [31:0]          o_copro_write_data;
output      [31:0]          o_write_data;
output      [31:0]          o_iaddress;
output      [31:0]          o_iaddress_nxt;             // un-registered version of address to the 
                                                        // cache rams address ports
output                      o_iaddress_valid;     // High when instruction address is valid
output      [31:0]          o_daddress;         // Address to data cache
output      [31:0]          o_daddress_nxt;             // un-registered version of address to the 
                                                        // cache rams address ports
output                      o_daddress_valid;    // High when data address is valid
output                      o_adex;               // Address Exception
output                      o_priviledged;        // Priviledged access
output                      o_exclusive;          // swap access
output                      o_write_enable;
output      [3:0]           o_byte_enable;
output      [8:0]           o_exec_load_rd;       // The destination register for a load instruction
output      [31:0]          o_status_bits;              // Full PC will all status bits, but PC part zero'ed out
output                      o_multiply_done;
 
 
// --------------------------------------------------
// Control signals from Instruction Decode stage
// --------------------------------------------------
input      [1:0]            i_status_bits_mode;
input                       i_status_bits_irq_mask;
input                       i_status_bits_firq_mask;
input      [31:0]           i_imm32;
input      [4:0]            i_imm_shift_amount;
input                       i_shift_imm_zero;
input      [3:0]            i_condition;
input                       i_decode_exclusive;       // swap access
 
input      [3:0]            i_rm_sel;
input      [3:0]            i_rs_sel;
input      [3:0]            i_rn_sel;
input      [1:0]            i_barrel_shift_amount_sel;
input      [1:0]            i_barrel_shift_data_sel;
input      [1:0]            i_barrel_shift_function;
input      [8:0]            i_alu_function;
input      [1:0]            i_multiply_function;
input      [2:0]            i_interrupt_vector_sel;
input      [3:0]            i_iaddress_sel;
input      [3:0]            i_daddress_sel;
input      [2:0]            i_pc_sel;
input      [1:0]            i_byte_enable_sel;
input      [2:0]            i_status_bits_sel;
input      [2:0]            i_reg_write_sel;
input                       i_user_mode_regs_store_nxt;
input                       i_firq_not_user_mode;
 
input                       i_write_data_wen;
input                       i_base_address_wen;     // save LDM base address register, 
                                                    // in case of data abort
input                       i_pc_wen;
input      [14:0]           i_reg_bank_wen;
input                       i_status_bits_flags_wen;
input                       i_status_bits_mode_wen;
input                       i_status_bits_irq_mask_wen;
input                       i_status_bits_firq_mask_wen;
input                       i_copro_write_data_wen;
input                       i_conflict;
input                       i_rn_use_read;
input                       i_rm_use_read;
input                       i_rs_use_read;
input                       i_rd_use_read;



reg    [31:0]        o_copro_write_data = 32'd0;
reg    [31:0]        o_write_data = 32'd0;
reg    [31:0]        o_iaddress = 32'hdeaddead;

reg           	     o_iaddress_valid = 1'd0;     // High when instruction address is valid
reg    [31:0]        o_daddress = 32'h0;         // Address to data cache

reg   	             o_daddress_valid = 1'd0;     // High when data address is valid
reg            	     o_adex = 1'd0;               // Address Exception
reg            	     o_priviledged = 1'd0;        // Priviledged access
reg                  o_exclusive = 1'd0;          // swap access
reg                  o_write_enable = 1'd0;
reg    [3:0]         o_byte_enable = 4'd0;
reg    [8:0]         o_exec_load_rd = 9'd0;       // The destination register for a load instruction

 
// ========================================================
// Internal signals
// ========================================================
wire [31:0]         write_data_nxt;
wire [3:0]          byte_enable_nxt;
wire [31:0]         pc_plus4;
wire [31:0]         pc_minus4;
wire [31:0]         daddress_plus4;
wire [31:0]         alu_plus4;
wire [31:0]         rn_plus4;
wire [31:0]         alu_out;
wire [3:0]          alu_flags;
wire [31:0]         rm;
wire [31:0]         rs;
wire [31:0]         rd;
wire [31:0]         rn;
wire [31:0]         reg_bank_rn;
wire [31:0]         reg_bank_rm;
wire [31:0]         reg_bank_rs;
wire [31:0]         reg_bank_rd;
wire [31:0]         pc;
wire [31:0]         pc_nxt;
wire [31:0]         interrupt_vector;
wire [7:0]          shift_amount;
wire [31:0]         barrel_shift_in;
wire [31:0]         barrel_shift_out;
wire                barrel_shift_carry;
wire                barrel_shift_stall;
 
wire [3:0]          status_bits_flags_nxt;
reg  [3:0]          status_bits_flags = 4'd0;
wire [1:0]          status_bits_mode_nxt;
reg  [1:0]          status_bits_mode = 2'b11;    //SVC  =  2'b11
                    // one-hot encoded rs select
wire [3:0]          status_bits_mode_rds_oh_nxt;

//reg  [3:0]          status_bits_mode_rds_oh = 1'd1 << OH_SVC;
reg  [3:0]          status_bits_mode_rds_oh = 4'b1000;
wire                status_bits_mode_rds_oh_update;
wire                status_bits_irq_mask_nxt;
reg                 status_bits_irq_mask = 1'd1;
wire                status_bits_firq_mask_nxt;
reg                 status_bits_firq_mask = 1'd1;
wire [8:0]          exec_load_rd_nxt;
 
wire                execute;                    // high when condition execution is true
wire [31:0]         reg_write_nxt;
wire                pc_wen;
wire [14:0]         reg_bank_wen;
wire [31:0]         multiply_out;
wire [1:0]          multiply_flags;
reg  [31:0]         base_address = 32'd0;             // Saves base address during LDM instruction in 
                                                    // case of data abort
wire [31:0]         read_data_filtered1;
wire [31:0]         read_data_filtered;
wire [31:0]         read_data_filtered_c;
reg  [31:0]         read_data_filtered_r = 32'd0;
reg  [3:0]          load_rd_r = 4'd0;
wire [3:0]          load_rd_c;
 
wire                write_enable_nxt;
wire                daddress_valid_nxt;
wire                iaddress_valid_nxt;
wire                priviledged_nxt;      
wire                priviledged_update;
wire                iaddress_update;
wire                daddress_update;
wire                base_address_update;
wire                write_data_update;
wire                copro_write_data_update;
wire                byte_enable_update;
wire                exec_load_rd_update;
wire                write_enable_update;
wire                exclusive_update;
wire                status_bits_flags_update;
wire                status_bits_mode_update;
wire                status_bits_irq_mask_update;
wire                status_bits_firq_mask_update;
 
wire [31:0]         alu_out_pc_filtered;
wire                adex_nxt;
wire [31:0]         save_int_pc;
wire [31:0]         save_int_pc_m4;
wire                ldm_flags;
wire                ldm_status_bits;
 

// ========================================================
// Status Bits in PC register
// ========================================================
wire [1:0] status_bits_mode_out;
wire  [3:0] pc_dmem_wen;  //jing


assign status_bits_mode_out = (i_status_bits_mode_wen && i_status_bits_sel == 3'd1 && !ldm_status_bits) ? 
                                    alu_out[1:0] : status_bits_mode ;
 
assign o_status_bits = {   status_bits_flags,           // 31:28
                           status_bits_irq_mask,        // 7   27
                           status_bits_firq_mask,       // 6   26
                           24'd0,			//     25:2
                           status_bits_mode_out };      // 1:0 = mode
 

// ========================================================
// Status Bits Select
// ========================================================
assign ldm_flags                 = i_wb_read_data_valid & ~i_mem_stall & i_wb_load_rd[8];
assign ldm_status_bits           = i_wb_read_data_valid & ~i_mem_stall & i_wb_load_rd[7];
 
 
assign status_bits_flags_nxt     = ldm_flags                 ? read_data_filtered[31:28]           :
                                   i_status_bits_sel == 3'd0 ? alu_flags                           :
                                   i_status_bits_sel == 3'd1 ? alu_out          [31:28]            :
                                   i_status_bits_sel == 3'd3 ? i_copro_read_data[31:28]            :
                                   // 4 = update status_bits_flags after a multiply operation
                                                        { multiply_flags, status_bits_flags[1:0] } ;
 
assign status_bits_mode_nxt      = ldm_status_bits           ? read_data_filtered [1:0] :
                                   i_status_bits_sel == 3'd0 ? i_status_bits_mode       :
                                   i_status_bits_sel == 3'd1 ? alu_out            [1:0] :
                                                               i_copro_read_data  [1:0] ;
 
 
// Used for the Rds output of register_bank - this special version of
// status_bits_mode speeds up the critical path from status_bits_mode through the
// register_bank, barrel_shifter and alu. It moves a mux needed for the
// i_user_mode_regs_store_nxt signal back into the previous stage -
// so its really part of the decode stage even though the logic is right here
// In addition the signal is one-hot encoded to further speed up the logic
 
//assign status_bits_mode_rds_oh_nxt    = i_user_mode_regs_store_nxt ? 1'd1 << OH_USR                            :
//                                       status_bits_mode_update    ? oh_status_bits_mode(status_bits_mode_nxt) :
//                                                                     oh_status_bits_mode(status_bits_mode)     ;


assign status_bits_mode_rds_oh_nxt    = i_user_mode_regs_store_nxt ? 4'b0001                            :  //1'd1 << OH_USR 
                                        status_bits_mode_update    ?   (  status_bits_mode_nxt == SVC  ? 4'b1000  : //1'd1 << OH_SVC
							                  status_bits_mode_nxt == IRQ  ? 4'b0010  : //1'd1 << OH_IRQ
             							          status_bits_mode_nxt == FIRQ ? 4'b0100 : //1'd1 << OH_FIRQ
							            		                         4'b0001  ): //1'd1 << OH_USR
								
                                            	                      (   status_bits_mode == SVC  ? 4'b1000  : //1'd1 << OH_SVC
							                  status_bits_mode == IRQ  ? 4'b0010  : //1'd1 << OH_IRQ
             							          status_bits_mode == FIRQ ? 4'b0100 : //1'd1 << OH_FIRQ
												     4'b0001  ); //1'd1 << OH_USR
 									 
assign status_bits_irq_mask_nxt  = ldm_status_bits           ? read_data_filtered     [27] :
                                   i_status_bits_sel == 3'd0 ? i_status_bits_irq_mask      :
                                   i_status_bits_sel == 3'd1 ? alu_out                [27] :
                                                               i_copro_read_data      [27] ;
 
assign status_bits_firq_mask_nxt = ldm_status_bits           ? read_data_filtered     [26] :
                                   i_status_bits_sel == 3'd0 ? i_status_bits_firq_mask     :
                                   i_status_bits_sel == 3'd1 ? alu_out                [26] :
                                                               i_copro_read_data      [26] ;
// ========================================================
// Adders
// ========================================================
assign pc_plus4       = pc         + 32'd4;
assign pc_minus4      = pc         - 32'd4;
assign daddress_plus4 = o_daddress + 32'd4;
assign alu_plus4      = alu_out    + 32'd4;
assign rn_plus4       = rn         + 32'd4;
 
// ========================================================
// Barrel Shift Amount Select
// ========================================================
// An immediate shift value of 0 is translated into 32
assign shift_amount = i_barrel_shift_amount_sel == 2'd0 ? 8'd0                         :
                      i_barrel_shift_amount_sel == 2'd1 ? rs[7:0]                      :
                                                          {3'd0, i_imm_shift_amount  } ;
 
 
// ========================================================
// Barrel Shift Data Select
// ========================================================
assign barrel_shift_in = i_barrel_shift_data_sel == 2'd0 ? i_imm32 : rm ;
 
 
// ========================================================
// Interrupt vector Select
// ========================================================
 
assign interrupt_vector = // Reset vector
                          (i_interrupt_vector_sel == 3'd0) ? 32'h00000000 :  
                          // Data abort interrupt vector                 
                          (i_interrupt_vector_sel == 3'd1) ? 32'h00000010 :
                          // Fast interrupt vector  
                          (i_interrupt_vector_sel == 3'd2) ? 32'h0000001c :
                          // Regular interrupt vector
                          (i_interrupt_vector_sel == 3'd3) ? 32'h00000018 :
                          // Prefetch abort interrupt vector
                          (i_interrupt_vector_sel == 3'd5) ? 32'h0000000c :
                          // Undefined instruction interrupt vector
                          (i_interrupt_vector_sel == 3'd6) ? 32'h00000004 :
                          // Software (SWI) interrupt vector
                          (i_interrupt_vector_sel == 3'd7) ? 32'h00000008 :
                          // Default is the address exception interrupt
                                                             32'h00000014 ;
 
 
// ========================================================
// Address Select
// ========================================================

assign pc_dmem_wen    = i_wb_read_data_valid & ~i_mem_stall & i_wb_load_rd[3:0] == 4'd15;
//always @( posedge i_clk )
//	pc_dmem_wen    = i_wb_read_data_valid & ~i_mem_stall & i_wb_load_rd[3:0] == 4'd15;
 
// If rd is the pc, then seperate the address bits from the status bits for
// generating the next address to fetch
//assign alu_out_pc_filtered = pc_wen && i_pc_sel == 3'd1 ? pcf(alu_out) : alu_out;

assign alu_out_pc_filtered = pc_wen && i_pc_sel == 3'd1 ? {6'd0, alu_out[25:2], 2'd0} : alu_out;
 
// if current instruction does not execute because it does not meet the condition
// then address advances to next instruction
assign o_iaddress_nxt = (pc_dmem_wen)            ? {6'd0, read_data_filtered[25:2], 2'd0} :
                        (!execute)               ? pc_plus4                : 
                        (i_iaddress_sel == 4'd0) ? pc_plus4                :
                        (i_iaddress_sel == 4'd1) ? alu_out_pc_filtered     :
                        (i_iaddress_sel == 4'd2) ? interrupt_vector        :
                                                   pc                      ;
  
// if current instruction does not execute because it does not meet the condition
// then address advances to next instruction
assign o_daddress_nxt = (i_daddress_sel == 4'd1) ? alu_out_pc_filtered   :
                        (i_daddress_sel == 4'd2) ? interrupt_vector      :
                        (i_daddress_sel == 4'd4) ? rn                    :
                        (i_daddress_sel == 4'd5) ? daddress_plus4        :  // MTRANS address incrementer
                        (i_daddress_sel == 4'd6) ? alu_plus4             :  // MTRANS decrement after
                                                   rn_plus4              ;  // MTRANS increment before
 
// Data accesses use 32-bit address space, but instruction
// accesses are restricted to 26 bit space
assign adex_nxt      = |o_iaddress_nxt[31:26] && i_decode_iaccess;
 
 
// ========================================================
// Filter Read Data
// ========================================================
// mem_load_rd[10:9]-> shift ROR bytes
// mem_load_rd[8]   -> load flags with PC
// mem_load_rd[7]   -> load status bits with PC
// mem_load_rd[6:5] -> Write into this Mode registers
// mem_load_rd[4]   -> zero_extend byte
// mem_load_rd[3:0] -> Destination Register 
assign read_data_filtered1 = i_wb_load_rd[10:9] == 2'd0 ? i_wb_read_data                                :
                             i_wb_load_rd[10:9] == 2'd1 ? {i_wb_read_data[7:0],  i_wb_read_data[31:8]}  :
                             i_wb_load_rd[10:9] == 2'd2 ? {i_wb_read_data[15:0], i_wb_read_data[31:16]} :
                                                          {i_wb_read_data[23:0], i_wb_read_data[31:24]} ;
 
assign read_data_filtered  = i_wb_load_rd[4] ? {24'd0, read_data_filtered1[7:0]} : read_data_filtered1 ; 
 
 
// ========================================================
// Program Counter Select
// ========================================================
// If current instruction does not execute because it does not meet the condition
// then PC advances to next instruction
assign pc_nxt = (!execute)       ? pc_plus4                :
                i_pc_sel == 3'd0 ? pc_plus4                :
                i_pc_sel == 3'd1 ? alu_out                 :
                i_pc_sel == 3'd2 ? interrupt_vector        :
                i_pc_sel == 3'd3 ? {6'd0, read_data_filtered[25:2], 2'd0} :
                                   pc_minus4               ;
 
 
// ========================================================
// Register Write Select
// ========================================================
 
assign save_int_pc    = { status_bits_flags, 
                          status_bits_irq_mask, 
                          status_bits_firq_mask, 
                          pc[25:2], 
                          status_bits_mode      };
 
 
assign save_int_pc_m4 = { status_bits_flags, 
                          status_bits_irq_mask, 
                          status_bits_firq_mask, 
                          pc_minus4[25:2], 
                          status_bits_mode      };
 
 
assign reg_write_nxt = i_reg_write_sel == 3'd0 ? alu_out               :
                       // save pc to lr on an interrupt                    
                       i_reg_write_sel == 3'd1 ? save_int_pc_m4        :
                       // to update Rd at the end of Multiplication
                       i_reg_write_sel == 3'd2 ? multiply_out          :
                       i_reg_write_sel == 3'd3 ? o_status_bits         :
                       i_reg_write_sel == 3'd5 ? i_copro_read_data     :  // mrc
                       i_reg_write_sel == 3'd6 ? base_address          :
                                                 save_int_pc           ;  
 
 
// ========================================================
// Byte Enable Select
// ========================================================
assign byte_enable_nxt = i_byte_enable_sel == 2'd0   ? 4'b1111 :  // word write
                         i_byte_enable_sel == 2'd2   ?            // halfword write
                         ( o_daddress_nxt[1] == 1'd0 ? 4'b0011 : 
                                                       4'b1100  ) :
 
                         o_daddress_nxt[1:0] == 2'd0 ? 4'b0001 :  // byte write
                         o_daddress_nxt[1:0] == 2'd1 ? 4'b0010 :
                         o_daddress_nxt[1:0] == 2'd2 ? 4'b0100 :
                                                       4'b1000 ;
 
 
// ========================================================
// Write Data Select
// ========================================================
assign write_data_nxt = i_byte_enable_sel == 2'd0 ? rd            :
                                                    {rd[7:0],rd[7:0],rd[7:0],rd[7:0]} ;
 
 
// ========================================================
// Conditional Execution
// ========================================================
//assign execute = conditional_execute ( i_condition, status_bits_flags );

assign execute = ( i_condition == AL                                        ) ||
                 ( i_condition == EQ  &&  status_bits_flags[2]                          ) ||
                 ( i_condition == NE  && !status_bits_flags[2]                          ) ||
                 ( i_condition == CS  &&  status_bits_flags[1]                          ) ||
                 ( i_condition == CC  && !status_bits_flags[1]                          ) ||
                 ( i_condition == MI  &&  status_bits_flags[3]                          ) ||
                 ( i_condition == PL  && !status_bits_flags[3]                          ) ||
                 ( i_condition == VS  &&  status_bits_flags[0]                          ) ||
                 ( i_condition == VC  && !status_bits_flags[0]                          ) ||
                 ( i_condition == HI  &&    status_bits_flags[1] && !status_bits_flags[2]           ) ||
                 ( i_condition == LS  &&  (!status_bits_flags[1] ||  status_bits_flags[2])          ) ||
                 ( i_condition == GE  &&  status_bits_flags[3] == status_bits_flags[0]              ) ||
                 ( i_condition == LT  &&  status_bits_flags[3] != status_bits_flags[0]              ) ||
                 ( i_condition == GT  &&  !status_bits_flags[2] && status_bits_flags[3] == status_bits_flags[0] ) ||
                 ( i_condition == LE  &&  (status_bits_flags[2] || status_bits_flags[3] != status_bits_flags[0])) ;
 
 
// allow the PC to increment to the next instruction when current
// instruction does not execute
assign pc_wen       = (i_pc_wen || !execute) && !i_conflict;
 
// only update register bank if current instruction executes
//assign reg_bank_wen = {{15{execute}} & i_reg_bank_wen};
 assign reg_bank_wen = execute ==1'd1? {15'b111111111111111 & i_reg_bank_wen}   :
					{15'b0 & i_reg_bank_wen};
 
// ========================================================
// Priviledged output flag
// ========================================================
// Need to look at status_bits_mode_nxt so switch to priviledged mode
// at the same time as assert interrupt vector address

assign priviledged_nxt  = ( i_status_bits_mode_wen ? status_bits_mode_nxt : status_bits_mode ) != USR ;
 
 
// ========================================================
// Write Enable
// ========================================================
// This must be de-asserted when execute is fault

assign write_enable_nxt = execute && i_write_data_wen;
 
 
// ========================================================
// Address Valid
// ========================================================
assign daddress_valid_nxt = execute && i_decode_daccess && !i_core_stall;
 
// For some multi-cycle instructions, the stream of instrution
// reads can be paused. However if the instruction does not execute
// then the read stream must not be interrupted.
assign iaddress_valid_nxt = i_decode_iaccess || !execute;
 
// ========================================================
// Use read value from data memory instead of from register
// ========================================================
assign rn = i_rn_use_read && i_rn_sel == load_rd_c ? read_data_filtered_c : reg_bank_rn;
assign rm = i_rm_use_read && i_rm_sel == load_rd_c ? read_data_filtered_c : reg_bank_rm;
assign rs = i_rs_use_read && i_rs_sel == load_rd_c ? read_data_filtered_c : reg_bank_rs;
assign rd = i_rd_use_read && i_rs_sel == load_rd_c ? read_data_filtered_c : reg_bank_rd;
 
 
always@( posedge i_clk )
    if ( i_wb_read_data_valid )
        begin
        read_data_filtered_r <= read_data_filtered;
        load_rd_r            <= i_wb_load_rd[3:0];
        end
 
assign read_data_filtered_c = i_wb_read_data_valid ? read_data_filtered : read_data_filtered_r;
assign load_rd_c            = i_wb_read_data_valid ? i_wb_load_rd[3:0]  : load_rd_r;
 
 
// ========================================================
// Set mode for the destination registers of a mem read
// ========================================================
// The mode is either user mode, or the current mode
assign  exec_load_rd_nxt   = { i_decode_load_rd[7:6], 
                               i_decode_load_rd[5] ? USR : status_bits_mode,  // 1 bit -> 2 bits
                               i_decode_load_rd[4:0] };
 
 
// ========================================================
// Register Update
// ========================================================
assign o_exec_stall                    = barrel_shift_stall;
 
assign daddress_update                 = !i_core_stall;
assign exec_load_rd_update             = !i_core_stall && execute;
assign priviledged_update              = !i_core_stall;       
assign exclusive_update                = !i_core_stall && execute;
assign write_enable_update             = !i_core_stall;
assign write_data_update               = !i_core_stall && execute && i_write_data_wen;
assign byte_enable_update              = !i_core_stall && execute && i_write_data_wen;
 
assign iaddress_update                 = pc_dmem_wen || (!i_core_stall && !i_conflict);
assign copro_write_data_update         = !i_core_stall && execute && i_copro_write_data_wen;
 
assign base_address_update             = !i_core_stall && execute && i_base_address_wen; 
assign status_bits_flags_update        = ldm_flags       || (!i_core_stall && execute && i_status_bits_flags_wen);
assign status_bits_mode_update         = ldm_status_bits || (!i_core_stall && execute && i_status_bits_mode_wen);
assign status_bits_mode_rds_oh_update  = !i_core_stall;
assign status_bits_irq_mask_update     = ldm_status_bits || (!i_core_stall && execute && i_status_bits_irq_mask_wen);
assign status_bits_firq_mask_update    = ldm_status_bits || (!i_core_stall && execute && i_status_bits_firq_mask_wen);
 
 
always @( posedge i_clk )
    begin                                                                                                             
    o_daddress              <= daddress_update                ? o_daddress_nxt               : o_daddress;    
    o_daddress_valid        <= daddress_update                ? daddress_valid_nxt           : o_daddress_valid;
    o_exec_load_rd          <= exec_load_rd_update            ? exec_load_rd_nxt             : o_exec_load_rd;
    o_priviledged           <= priviledged_update             ? priviledged_nxt              : o_priviledged;
    o_exclusive             <= exclusive_update               ? i_decode_exclusive           : o_exclusive;
    o_write_enable          <= write_enable_update            ? write_enable_nxt             : o_write_enable;
    o_write_data            <= write_data_update              ? write_data_nxt               : o_write_data; 
    o_byte_enable           <= byte_enable_update             ? byte_enable_nxt              : o_byte_enable;
    o_iaddress              <= iaddress_update                ? o_iaddress_nxt               : o_iaddress;    
    o_iaddress_valid        <= iaddress_update                ? iaddress_valid_nxt           : o_iaddress_valid;
    o_adex                  <= iaddress_update                ? adex_nxt                     : o_adex;    
    o_copro_write_data      <= copro_write_data_update        ? write_data_nxt               : o_copro_write_data; 
 
    base_address            <= base_address_update            ? rn                           : base_address;    
 
    status_bits_flags       <= status_bits_flags_update       ? status_bits_flags_nxt        : status_bits_flags;
    status_bits_mode        <= status_bits_mode_update        ? status_bits_mode_nxt         : status_bits_mode;
    status_bits_mode_rds_oh <= status_bits_mode_rds_oh_update ? status_bits_mode_rds_oh_nxt  : status_bits_mode_rds_oh;
    status_bits_irq_mask    <= status_bits_irq_mask_update    ? status_bits_irq_mask_nxt     : status_bits_irq_mask;
    status_bits_firq_mask   <= status_bits_firq_mask_update   ? status_bits_firq_mask_nxt    : status_bits_firq_mask;
    end
 
 
// ========================================================
// Instantiate Barrel Shift
// ========================================================
a25_barrel_shift u_barrel_shift  (
    .i_clk            ( i_clk                     ),
    .i_in             ( barrel_shift_in           ),
    .i_carry_in       ( status_bits_flags[1]      ),
    .i_shift_amount   ( shift_amount              ),
    .i_shift_imm_zero ( i_shift_imm_zero          ),
    .i_function       ( i_barrel_shift_function   ),
 
    .o_out            ( barrel_shift_out          ),
    .o_carry_out      ( barrel_shift_carry        ),
    .o_stall          ( barrel_shift_stall        )
);
 
 
// ========================================================
// Instantiate ALU
// ========================================================
a25_alu u_alu (
    .i_a_in                 ( rn                    ),
    .i_b_in                 ( barrel_shift_out      ),
    .i_barrel_shift_carry   ( barrel_shift_carry    ),
    .i_status_bits_carry    ( status_bits_flags[1]  ),
    .i_function             ( i_alu_function        ),
 
    .o_out                  ( alu_out               ),
    .o_flags                ( alu_flags             )
);
 
 
// ========================================================
// Instantiate Booth 64-bit Multiplier-Accumulator
// ========================================================
a25_multiply u_multiply (
    .i_clk          ( i_clk                 ),
    .i_core_stall   ( i_core_stall          ),
    .i_a_in         ( rs                    ),
    .i_b_in         ( rm                    ),
    .i_function     ( i_multiply_function   ),
    .i_execute      ( execute               ),
    .o_out          ( multiply_out          ),
    .o_flags        ( multiply_flags        ),  // [1] = N, [0] = Z
    .o_done         ( o_multiply_done       )     
);
 
 
// ========================================================
// Instantiate Register Bank
// ========================================================
a25_register_bank u_register_bank(
    .i_clk                   ( i_clk                     ),
    .i_core_stall            ( i_core_stall              ),
    .i_mem_stall             ( i_mem_stall               ),
    .i_mode_idec             ( i_status_bits_mode        ),
    .i_mode_exec             ( status_bits_mode          ),
    .i_mode_rds_exec         ( status_bits_mode_rds_oh   ),  

    // pre-encoded in decode stage to speed up long path
    .i_firq_not_user_mode    ( i_firq_not_user_mode      ),
    .i_rm_sel                ( i_rm_sel                  ),
    .i_rs_sel                ( i_rs_sel                  ),
    .i_rn_sel                ( i_rn_sel                  ),
    .i_pc_wen                ( pc_wen                    ),
    .i_reg_bank_wen          ( reg_bank_wen              ),
    .i_pc                    ( pc_nxt[25:2]              ),
    .i_reg                   ( reg_write_nxt             ),


 
    .i_wb_read_data          ( read_data_filtered        ),
    .i_wb_read_data_valid    ( i_wb_read_data_valid      ),
    .i_wb_read_data_rd       ( i_wb_load_rd[3:0]         ),
    .i_wb_mode               ( i_wb_load_rd[6:5]         ),
 
    .i_status_bits_flags     ( status_bits_flags         ),
    .i_status_bits_irq_mask  ( status_bits_irq_mask      ),
    .i_status_bits_firq_mask ( status_bits_firq_mask     ),
 

 
    // use one-hot version for speed, combine with i_user_mode_regs_store

 
    .o_rm                    ( reg_bank_rm               ),
    .o_rs                    ( reg_bank_rs               ),
    .o_rd                    ( reg_bank_rd               ),
    .o_rn                    ( reg_bank_rn               ),
    .o_pc                    ( pc                        )
);
 
 
 
// ========================================================
// Debug - non-synthesizable code
// ========================================================
//synopsys translate_off
 
wire    [(2*8)-1:0]    xCONDITION;
wire    [(4*8)-1:0]    xMODE;
 
assign  xCONDITION           = i_condition == EQ ? "EQ"  :
                               i_condition == NE ? "NE"  :
                               i_condition == CS ? "CS"  :
                               i_condition == CC ? "CC"  :
                               i_condition == MI ? "MI"  :
                               i_condition == PL ? "PL"  :
                               i_condition == VS ? "VS"  :
                               i_condition == VC ? "VC"  :
                               i_condition == HI ? "HI"  :
                               i_condition == LS ? "LS"  :
                               i_condition == GE ? "GE"  :
                               i_condition == LT ? "LT"  :
                               i_condition == GT ? "GT"  :
                               i_condition == LE ? "LE"  :
                               i_condition == AL ? "AL"  :
                                                   "NV " ;
 
assign  xMODE  =  status_bits_mode == SVC  ? "SVC"  :
                  status_bits_mode == IRQ  ? "IRQ"  :
                  status_bits_mode == FIRQ ? "FIRQ" :
                  			     "USR"  ;
 
 
//synopsys translate_on
 
endmodule


module a25_dcache 

		(
		i_clk, 		
		i_request,
		i_exclusive,     
		i_write_data,
		i_write_enable,     
		i_address,         
		i_address_nxt,     
		i_byte_enable,
		i_cache_enable,  
		i_cache_flush,     
		i_fetch_stall,
		i_exec_stall,                                                   
		i_wb_cached_rdata,                            
		i_wb_cached_ready,  

		o_read_data,    
		o_stall, 
		o_wb_cached_req           
		);

 
 
`ifndef _A25_CONFIG_DEFINES
`define _A25_CONFIG_DEFINES
 
// Cache Ways
// Changing this parameter is the recommended
// way to change the Amber cache size; 2, 3, 4 and 8 ways are supported.
//
//   2 ways -> 8KB  cache
//   3 ways -> 12KB cache
//   4 ways -> 16KB cache
//   8 ways -> 32KB cache
//
//   e.g. if both caches have 8 ways, the total is 32KB icache + 32KB dcache = 64KB
 
`define A25_ICACHE_WAYS 4
`define A25_DCACHE_WAYS 4
 
 
// --------------------------------------------------------------------
// Debug switches 
// --------------------------------------------------------------------
 
// Enable the decompiler. The default output file is amber.dis
//`define A25_DECOMPILE
 
// Co-processor 15 debug. Registers in here control the cache
//`define A25_COPRO15_DEBUG
 
// Cache debug
//`define A25_CACHE_DEBUG
 
// --------------------------------------------------------------------
 
 
// --------------------------------------------------------------------
// File Names
// --------------------------------------------------------------------
//`ifndef A25_DECOMPILE_FILE
//    `define A25_DECOMPILE_FILE    "amber.dis"
//`endif

`endif
// ---------------------------------------------------------
// Cache Configuration
 
// Limited to Linux 4k page sizes -> 256 lines
parameter CACHE_LINES          = 256;

// This cannot be changed without some major surgeory on
// this module                                       
parameter CACHE_WORDS_PER_LINE = 4;
 
// Changing this parameter is the recommended
// way to change the overall cache size; 2, 4 and 8 ways are supported.
//   2 ways -> 8KB  cache
//   4 ways -> 16KB cache
//   8 ways -> 32KB cache
parameter WAYS              = `A25_DCACHE_WAYS;			//4
 
// derived configuration parameters
//parameter CACHE_ADDR_WIDTH  = log2 ( CACHE_LINES );                        // = 8
//parameter WORD_SEL_WIDTH    = log2 ( CACHE_WORDS_PER_LINE );               // = 2
//parameter TAG_ADDR_WIDTH    = 32 - CACHE_ADDR_WIDTH - WORD_SEL_WIDTH - 2;  // = 20
//parameter TAG_WIDTH         = TAG_ADDR_WIDTH + 1;                          // = 21, including Valid flag
//parameter CACHE_LINE_WIDTH  = CACHE_WORDS_PER_LINE * 32;                   // = 128
//parameter TAG_ADDR32_LSB    = CACHE_ADDR_WIDTH + WORD_SEL_WIDTH + 2;       // = 12
//parameter CACHE_ADDR32_MSB  = CACHE_ADDR_WIDTH + WORD_SEL_WIDTH + 2 - 1;   // = 11
//parameter CACHE_ADDR32_LSB  =                    WORD_SEL_WIDTH + 2;   // = 4
//parameter WORD_SEL_MSB      = WORD_SEL_WIDTH + 2 - 1;                      // = 3
//parameter WORD_SEL_LSB      =                  2;                           // = 2
// ---------------------------------------------------------

parameter CACHE_ADDR_WIDTH  = 8;                        // = 8
parameter WORD_SEL_WIDTH    = 2;         	      // = 2
parameter TAG_ADDR_WIDTH    = 20;  			// = 20
parameter TAG_WIDTH         = 21;                          // = 21, including Valid flag
parameter CACHE_LINE_WIDTH  = 128;                   // = 128
parameter TAG_ADDR32_LSB    = 12;     		  // = 12
parameter CACHE_ADDR32_MSB  = 11;  			 // = 11
parameter CACHE_ADDR32_LSB  = 4;  			 // = 4
parameter WORD_SEL_MSB      = 3;                      // = 3
parameter WORD_SEL_LSB      = 2;                           // = 2


input                               i_clk; 		// Read / Write requests from core
input                               i_request;
input                               i_exclusive;        // exclusive access, part of swap instruction
input      [31:0]                   i_write_data;
input                               i_write_enable;     // write request from execute stage
input      [31:0]                   i_address;          // registered address from execute
input      [31:0]                   i_address_nxt;      // un-registered version of address from execute stage
input      [3:0]                    i_byte_enable;
input                               i_cache_enable;     // from co-processor 15 configuration register
input                               i_cache_flush;      // from co-processor 15 register
 
output      [31:0]                  o_read_data;                                                       
input                               i_fetch_stall;
input                               i_exec_stall;
output                              o_stall;
 
// WB Read Request                                                          
output                              o_wb_cached_req;        // Read Request
input      [127:0]                  i_wb_cached_rdata;      // wb bus                              
input                               i_wb_cached_ready;       // wb_stb && !wb_ack
 
//////////////////////////////////////////////////////////////////
//                                                              //
//  Parameters file for Amber 25 Core                           //
//                                                              //
//  This file is part of the Amber project                      //
//  http://www.opencores.org/project,amber                      //
//                                                              //
//  Description                                                 //
//  Holds general parameters that are used is several core      //
//  modules                                                     //
//                                                              //
//  Author(s):                                                  //
//      - Conor Santifort, csantifort.amber@gmail.com           //
//                                                              //
//////////////////////////////////////////////////////////////////
//                                                              //
// Copyright (C) 2011 Authors and OPENCORES.ORG                 //
//                                                              //
// This source file may be used and distributed without         //
// restriction provided that this copyright statement is not    //
// removed from the file and that any derivative work contains  //
// the original copyright notice and the associated disclaimer. //
//                                                              //
// This source file is free software; you can redistribute it   //
// and/or modify it under the terms of the GNU Lesser General   //
// Public License as published by the Free Software Foundation; //
// either version 2.1 of the License, or (at your option) any   //
// later version.                                               //
//                                                              //
// This source is distributed in the hope that it will be       //
// useful, but WITHOUT ANY WARRANTY; without even the implied   //
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      //
// PURPOSE.  See the GNU Lesser General Public License for more //
// details.                                                     //
//                                                              //
// You should have received a copy of the GNU Lesser General    //
// Public License along with this source; if not, download it   //
// from http://www.opencores.org/lgpl.shtml                     //
//                                                              //
//////////////////////////////////////////////////////////////////
 
 
// Instruction Types
localparam [3:0]    REGOP       = 4'h0, // Data processing
                    MULT        = 4'h1, // Multiply
                    SWAP        = 4'h2, // Single Data Swap
                    TRANS       = 4'h3, // Single data transfer
                    MTRANS      = 4'h4, // Multi-word data transfer
                    BRANCH      = 4'h5, // Branch
                    CODTRANS    = 4'h6, // Co-processor data transfer
                    COREGOP     = 4'h7, // Co-processor data operation
                    CORTRANS    = 4'h8, // Co-processor register transfer
                    SWI         = 4'h9; // software interrupt
 
 
// Opcodes
localparam [3:0] AND = 4'h0,        // Logical AND
                 EOR = 4'h1,        // Logical Exclusive OR
                 SUB = 4'h2,        // Subtract
                 RSB = 4'h3,        // Reverse Subtract
                 ADD = 4'h4,        // Add
                 ADC = 4'h5,        // Add with Carry
                 SBC = 4'h6,        // Subtract with Carry
                 RSC = 4'h7,        // Reverse Subtract with Carry
                 TST = 4'h8,        // Test  (using AND operator)
                 TEQ = 4'h9,        // Test Equivalence (using EOR operator)
                 CMP = 4'ha,       // Compare (using Subtract operator)
                 CMN = 4'hb,       // Compare Negated
                 ORR = 4'hc,       // Logical OR
                 MOV = 4'hd,       // Move
                 BIC = 4'he,       // Bit Clear (using AND & NOT operators)
                 MVN = 4'hf;       // Move NOT
 
// Condition Encoding
localparam [3:0] EQ  = 4'h0,        // Equal            / Z set
                 NE  = 4'h1,        // Not equal        / Z clear
                 CS  = 4'h2,        // Carry set        / C set
                 CC  = 4'h3,        // Carry clear      / C clear
                 MI  = 4'h4,        // Minus            / N set
                 PL  = 4'h5,        // Plus             / N clear
                 VS  = 4'h6,        // Overflow         / V set
                 VC  = 4'h7,        // No overflow      / V clear
                 HI  = 4'h8,        // Unsigned higher  / C set and Z clear
                 LS  = 4'h9,        // Unsigned lower
                                    // or same          / C clear or Z set
                 GE  = 4'ha,        // Signed greater 
                                    // than or equal    / N == V
                 LT  = 4'hb,        // Signed less than / N != V
                 GT  = 4'hc,        // Signed greater
                                    // than             / Z == 0, N == V
                 LE  = 4'hd,        // Signed less than
                                    // or equal         / Z == 1, N != V
                 AL  = 4'he,        // Always
                 NV  = 4'hf;        // Never
 
// Any instruction with a condition field of 0b1111 is UNPREDICTABLE.                
 
// Shift Types
localparam [1:0] LSL = 2'h0,
                 LSR = 2'h1,
                 ASR = 2'h2,
                 RRX = 2'h3,
                 ROR = 2'h3; 
 
// Modes
localparam [1:0] SVC  =  2'b11,  // Supervisor
                 IRQ  =  2'b10,  // Interrupt
                 FIRQ =  2'b01,  // Fast Interrupt
                 USR  =  2'b00;  // User
 
// One-Hot Mode encodings
localparam [5:0] OH_USR  = 0,
                 OH_IRQ  = 1,
                 OH_FIRQ = 2,
                 OH_SVC  = 3;
 
 
// One-hot encoded
localparam       C_INIT   = 0,
                 C_CORE   = 1,
                 C_FILL   = 2,
                 C_INVA   = 3,
                 C_STATES = 4;
 
localparam [3:0] CS_INIT               = 4'd0,
                 CS_IDLE               = 4'd1,
                 CS_FILL               = 4'd2,
                 CS_FILL_COMPLETE      = 4'd3,
                 CS_TURN_AROUND        = 4'd4,
                 CS_WRITE_HIT          = 4'd5,
                 CS_WRITE_HIT_WAIT_WB  = 4'd6,
                 CS_WRITE_MISS_WAIT_WB = 4'd7,
                 CS_EX_DELETE          = 4'd8;
 
 
reg  [3:0]                  c_state    = CS_IDLE;
//reg  [C_STATES-1:0]         source_sel = 4'd1 << C_CORE;
reg  [C_STATES-1:0]         source_sel = 4'b10;
reg  [CACHE_ADDR_WIDTH:0]   init_count = 9'd0;
 
wire [TAG_WIDTH-1:0]        tag_rdata_way0;
wire [TAG_WIDTH-1:0]        tag_rdata_way1;
wire [TAG_WIDTH-1:0]        tag_rdata_way2;
wire [TAG_WIDTH-1:0]        tag_rdata_way3;
wire [CACHE_LINE_WIDTH-1:0] data_rdata_way0;
wire [CACHE_LINE_WIDTH-1:0] data_rdata_way1;
wire [CACHE_LINE_WIDTH-1:0] data_rdata_way2;
wire [CACHE_LINE_WIDTH-1:0] data_rdata_way3;
wire [WAYS-1:0]             data_wenable_way;
wire [WAYS-1:0]             data_hit_way;
reg  [WAYS-1:0]             data_hit_way_r = 4'd0;
wire [WAYS-1:0]             tag_wenable_way;
reg  [WAYS-1:0]             select_way = 4'd0;
wire [WAYS-1:0]             next_way;
reg  [WAYS-1:0]             valid_bits_r = 4'd0;
 
reg  [3:0]                  random_num = 4'hf;
 
wire [CACHE_ADDR_WIDTH-1:0] tag_address;
wire [TAG_WIDTH-1:0]        tag_wdata;
wire                        tag_wenable;
 
wire [CACHE_LINE_WIDTH-1:0] read_miss_wdata;
wire [CACHE_LINE_WIDTH-1:0] write_hit_wdata;
reg  [CACHE_LINE_WIDTH-1:0] data_wdata_r = 128'd0;
wire [CACHE_LINE_WIDTH-1:0] consecutive_write_wdata;
wire [CACHE_LINE_WIDTH-1:0] data_wdata;
wire [CACHE_ADDR_WIDTH-1:0] data_address;
wire [31:0]                 write_data_word;
 
wire                        idle_hit;
wire                        read_miss;
wire                        write_miss;
wire                        write_hit;
wire                        consecutive_write;
wire                        fill_state;
 
reg  [31:0]                 miss_address = 32'd0;
wire [CACHE_LINE_WIDTH-1:0] hit_rdata;
 
wire                        read_stall;
wire                        write_stall;
wire                        cache_busy_stall;
wire                        core_stall;
wire                        write_state;
 
wire                        request_pulse;
wire                        request_hold;
reg                         request_r = 1'd0;
wire [CACHE_ADDR_WIDTH-1:0] address;
reg  [CACHE_LINE_WIDTH-1:0] wb_rdata_burst = 128'd0;
 
wire                        exclusive_access;
wire                        ex_read_hit;
reg                         ex_read_hit_r = 1'd0;
reg  [WAYS-1:0]             ex_read_hit_way = 4'd0;
reg  [CACHE_ADDR_WIDTH-1:0] ex_read_address;
wire                        ex_read_hit_clear;
wire                        ex_read_cache_busy;
 
reg  [31:0]                 wb_address = 32'd0;
//wire                        rbuf_hit = 1'd0;
wire                        wb_hit;
wire [127:0]                read_data128;
//genvar                      i;

 
// ======================================
// Address to use for cache access
// ======================================
// If currently stalled then the address for the next
// cycle will be the same as it is in the current cycle
//
assign core_stall = i_fetch_stall || i_exec_stall || o_stall;
 
assign address = core_stall ? i_address    [CACHE_ADDR32_MSB:CACHE_ADDR32_LSB] :
                              i_address_nxt[CACHE_ADDR32_MSB:CACHE_ADDR32_LSB] ;
 
// ======================================
// Outputs
// ======================================
 
assign read_data128 = wb_hit ? i_wb_cached_rdata : hit_rdata;
 
assign o_read_data  = i_address[WORD_SEL_MSB:WORD_SEL_LSB] == 2'd0 ? read_data128 [31:0]   :
                      i_address[WORD_SEL_MSB:WORD_SEL_LSB] == 2'd1 ? read_data128 [63:32]  :
                      i_address[WORD_SEL_MSB:WORD_SEL_LSB] == 2'd2 ? read_data128 [95:64]  :
                                                                     read_data128 [127:96] ;
 
// Don't allow the cache to stall the wb i/f for an exclusive access
// The cache needs a couple of cycles to flush a potential copy of the exclusive
// address, but the wb can do the access in parallel. So there is no
// stall in the state CS_EX_DELETE, even though the cache is out of action. 
// This works fine as long as the wb is stalling the core
assign o_stall         = request_hold && ( read_stall || write_stall || cache_busy_stall || ex_read_cache_busy );
 
assign o_wb_cached_req = ( (read_miss || write_miss || write_hit) && c_state == CS_IDLE ) || consecutive_write;
 
 
// ======================================
// Cache State Machine
// ======================================
 
// Little State Machine to Flush Tag RAMS
always @ ( posedge i_clk )
    if ( i_cache_flush )
        begin
        c_state     <= CS_INIT;
        source_sel  <= 4'b1;  //1'd1 << C_INIT
        init_count  <= 9'd0;
        `ifdef A25_CACHE_DEBUG  
        `TB_DEBUG_MESSAGE  
        $display("Cache Flush");
        `endif            
        end
    else    
        case ( c_state )
            CS_INIT :
                if ( init_count < CACHE_LINES [CACHE_ADDR_WIDTH:0] )
                    begin
                    init_count  <= init_count + 1'd1;
   		    source_sel  <= 4'b1;  //1'd1 << C_INIT
                    end
                else
                    begin
                    source_sel  <= 4'b10;  //1'd1 << C_CORE
                    c_state     <= CS_TURN_AROUND;
                    end 
 
             CS_IDLE :
                begin
	        if ( ex_read_hit || ex_read_hit_r )
		            begin
		            select_way  <= data_hit_way | ex_read_hit_way;
		            c_state     <= CS_EX_DELETE;        
		            source_sel  <= 4'b1000;   //1'd1 << C_INVA
		            end
		        else if ( read_miss ) 
			begin
		            c_state <= CS_FILL; 
            		source_sel  <= 4'b10;  //1'd1 << C_CORE
			end
		        else if ( write_hit )
		            begin
            			source_sel  <= 4'b10;  //1'd1 << C_CORE
		            if ( i_wb_cached_ready )
		                c_state <= CS_WRITE_HIT;  
      
		            else    
		                c_state <= CS_WRITE_HIT_WAIT_WB;        
		            end    
		        else if ( write_miss && !i_wb_cached_ready )
			begin
            			source_sel  <= 4'b10;  //1'd1 << C_CORE
		                c_state <= CS_WRITE_MISS_WAIT_WB;        
			end
                end
 
 
             CS_FILL :
                // third read of burst of 4
                // wb read request asserted, wait for ack
                if ( i_wb_cached_ready ) 
                    begin
                    c_state     <= CS_FILL_COMPLETE;
                    source_sel  <= 4'b100;   //1'd1 << C_FILL
 
                    // Pick a way to write the cache update into
                    // Either pick one of the invalid caches, or if all are valid, then pick
                    // one randomly
 
                    select_way  <= next_way; 
                    random_num  <= {random_num[2], random_num[1], random_num[0], 
                                     random_num[3]^random_num[2]};
                    end
 
 
             // Write the read fetch data in this cycle
             CS_FILL_COMPLETE : 
                begin
                // Back to normal cache operations, but
                // use physical address for first read as
                // address moved before the stall was asserted for the read_miss
                // However don't use it if its a non-cached address!
                source_sel  <= 4'b10;  //1'd1 << C_CORE          
                c_state     <= CS_TURN_AROUND;    
                end                                 
 
 
             // Ignore the tag read data in this cycle   
             // Wait 1 cycle to pre-read the cache and return to normal operation                 
             CS_TURN_AROUND : 
                begin
                c_state     <= CS_IDLE;
                end
 
 
             // Flush the entry matching an exclusive access         
             CS_EX_DELETE:       
                begin
                `ifdef A25_CACHE_DEBUG    
                `TB_DEBUG_MESSAGE
                $display("Cache deleted Locked entry");
                `endif    
                c_state    <= CS_TURN_AROUND;
                source_sel  <= 4'b10;  //1'd1 << C_CORE 
                end
 
 
             CS_WRITE_HIT:
                if ( !consecutive_write )           
                    c_state     <= CS_IDLE;
 
 
             CS_WRITE_HIT_WAIT_WB:
                // wait for an ack on the wb bus to complete the write
                if ( i_wb_cached_ready ) 
                    c_state     <= CS_IDLE;
 
 
             CS_WRITE_MISS_WAIT_WB:
                // wait for an ack on the wb bus to complete the write
                if ( i_wb_cached_ready ) 
                    c_state     <= CS_IDLE;
 
        endcase                       
 
 
// ======================================
// Capture WB Block Read - burst of 4 words
// ======================================
always @ ( posedge i_clk )
    if ( i_wb_cached_ready )
        wb_rdata_burst <= i_wb_cached_rdata;
 
 
 
// ======================================
// Miss Address
// ======================================
always @ ( posedge i_clk )
    if ( o_wb_cached_req || write_hit )
        miss_address <= i_address;
 
always @ ( posedge i_clk )
    if ( write_hit )
        begin
        data_hit_way_r      <= data_hit_way;
        end
 
always @ ( posedge i_clk )
    if ( write_hit || consecutive_write )
        begin
        data_wdata_r   <= data_wdata;
        end
 
assign consecutive_write = miss_address[31:4] == i_address[31:4] && 
                           i_write_enable && 
                           c_state == CS_WRITE_HIT && 
                           request_pulse;
 
 
always @(posedge i_clk)
    if ( o_wb_cached_req )
        wb_address <= i_address;
    else if ( i_wb_cached_ready && fill_state )    
        wb_address <= {wb_address[31:4], wb_address[3:2] + 1'd1, 2'd0};
 
assign fill_state       = c_state == CS_FILL ;
assign wb_hit           = i_address == wb_address && i_wb_cached_ready && fill_state;
 
 
// ======================================
// Hold Requests
// ======================================
always @(posedge i_clk)
    request_r <= (request_pulse || request_r) && o_stall;
 
assign request_hold = request_pulse || request_r;
 
 
// ======================================
// Remember Read-Modify-Write Hit
// ======================================
assign ex_read_hit_clear = c_state == CS_EX_DELETE;
 
always @ ( posedge i_clk )
    if ( ex_read_hit_clear )
        begin
        ex_read_hit_r   <= 1'd0;
        ex_read_hit_way <= 4'd0;
        end
    else if ( ex_read_hit )
        begin
 
        `ifdef A25_CACHE_DEBUG
            `TB_DEBUG_MESSAGE
            $display ("Exclusive access cache hit address 0x%08h", i_address);
        `endif
 
        ex_read_hit_r   <= 1'd1;
        ex_read_hit_way <= data_hit_way;
        end
    else if ( c_state == CS_FILL_COMPLETE && ex_read_hit_r )
        ex_read_hit_way <= select_way;
 
 
always @ (posedge i_clk)
    if ( ex_read_hit )
        ex_read_address <= i_address[CACHE_ADDR32_MSB:CACHE_ADDR32_LSB];
 
 
assign tag_address      = source_sel[C_FILL] ? miss_address      [CACHE_ADDR32_MSB:CACHE_ADDR32_LSB] :
                          source_sel[C_INVA] ? ex_read_address                                       :
                          source_sel[C_INIT] ? init_count[CACHE_ADDR_WIDTH-1:0]                      :
                          source_sel[C_CORE] ? address                                               :
                                               {CACHE_ADDR_WIDTH{1'd0}}                              ;
 
 
assign data_address     = consecutive_write  ? miss_address[CACHE_ADDR32_MSB:CACHE_ADDR32_LSB] : 
                          write_hit          ? i_address   [CACHE_ADDR32_MSB:CACHE_ADDR32_LSB] :
                          source_sel[C_FILL] ? miss_address[CACHE_ADDR32_MSB:CACHE_ADDR32_LSB] : 
                          source_sel[C_CORE] ? address                                         :
                                               {CACHE_ADDR_WIDTH{1'd0}}                        ;
 
 
assign tag_wdata        = source_sel[C_FILL] ? {1'd1, miss_address[31:12]} :  // [31:TAG_ADDR32_LSB]
                                               {TAG_WIDTH{1'd0}}                       ;
 
 
    // Data comes in off the WB bus in wrap4 with the missed data word first
assign data_wdata       = write_hit && c_state == CS_IDLE ? write_hit_wdata : 
                          consecutive_write               ? consecutive_write_wdata :
                                                            read_miss_wdata ;
 
assign read_miss_wdata  = wb_rdata_burst;
 
 
assign write_hit_wdata  = i_address[3:2] == 2'd0 ? {hit_rdata[127:32], write_data_word                   } :
                          i_address[3:2] == 2'd1 ? {hit_rdata[127:64], write_data_word, hit_rdata[31:0]  } :
                          i_address[3:2] == 2'd2 ? {hit_rdata[127:96], write_data_word, hit_rdata[63:0]  } :
                                                   {                   write_data_word, hit_rdata[95:0]  } ;
wire [31:0] con_read_data_word;
wire [31:0] con_write_data_word;
 
assign consecutive_write_wdata = 
                          i_address[3:2] == 2'd0 ? {data_wdata_r[127:32], con_write_data_word                           } :
                          i_address[3:2] == 2'd1 ? {data_wdata_r[127:64], con_write_data_word, data_wdata_r[31:0]  } :
                          i_address[3:2] == 2'd2 ? {data_wdata_r[127:96], con_write_data_word, data_wdata_r[63:0]  } :
                                                   {                      con_write_data_word, data_wdata_r[95:0]  } ;
assign con_read_data_word = 
                          i_address[3:2] == 2'd0 ? data_wdata_r[ 31:  0] :
                          i_address[3:2] == 2'd1 ? data_wdata_r[ 63: 32] : 
                          i_address[3:2] == 2'd2 ? data_wdata_r[ 95: 64] : 
                                                   data_wdata_r[127: 96] ;
 
 
assign con_write_data_word  = 
                          i_byte_enable == 4'b0001 ? { con_read_data_word[31: 8], i_write_data[ 7: 0]                          } :
                          i_byte_enable == 4'b0010 ? { con_read_data_word[31:16], i_write_data[15: 8], con_read_data_word[ 7:0]} :
                          i_byte_enable == 4'b0100 ? { con_read_data_word[31:24], i_write_data[23:16], con_read_data_word[15:0]} :
                          i_byte_enable == 4'b1000 ? {                            i_write_data[31:24], con_read_data_word[23:0]} :
                          i_byte_enable == 4'b0011 ? { con_read_data_word[31:16], i_write_data[15: 0]                          } :
                          i_byte_enable == 4'b1100 ? {                            i_write_data[31:16], con_read_data_word[15:0]} :
                                                                   i_write_data                                                  ;
 
 
 
 
// Use Byte Enables
assign write_data_word  = i_byte_enable == 4'b0001 ? { o_read_data[31: 8], i_write_data[ 7: 0]                   } :
                          i_byte_enable == 4'b0010 ? { o_read_data[31:16], i_write_data[15: 8], o_read_data[ 7:0]} :
                          i_byte_enable == 4'b0100 ? { o_read_data[31:24], i_write_data[23:16], o_read_data[15:0]} :
                          i_byte_enable == 4'b1000 ? {                     i_write_data[31:24], o_read_data[23:0]} :
                          i_byte_enable == 4'b0011 ? { o_read_data[31:16], i_write_data[15: 0]                   } :
                          i_byte_enable == 4'b1100 ? {                     i_write_data[31:16], o_read_data[15:0]} :
                                                     i_write_data                                                  ;
 
 
assign tag_wenable      = source_sel[C_INVA] ? 1'd1  :
                          source_sel[C_FILL] ? 1'd1  :
                          source_sel[C_INIT] ? 1'd1  :
                          source_sel[C_CORE] ? 1'd0  :
                                               1'd0  ;
 
 
assign request_pulse    = i_request && i_cache_enable;
 
assign exclusive_access = i_exclusive && i_cache_enable;
 
 
//always@(posedge i_clk)
//	idle_hit <= |data_hit_way;

assign idle_hit         = |data_hit_way;
 
assign write_hit        = request_hold &&  i_write_enable && idle_hit;
 
assign write_miss       = request_hold &&  i_write_enable && !idle_hit && !consecutive_write;
 
assign read_miss        = request_hold && !idle_hit && !i_write_enable;
 
                          // Exclusive read idle_hit
assign ex_read_hit      = exclusive_access && !i_write_enable && idle_hit;
 
                          // Added to fix rare swap bug which occurs when the cache starts
                          // a fill just as the swap instruction starts to execute. The cache
                          // fails to check for a read idle_hit on the swap read cycle.
                          // This signal stalls the core in that case until after the
                          // fill has completed.
assign ex_read_cache_busy = exclusive_access && !i_write_enable && c_state != CS_IDLE;
 
                          // Need to stall for a write miss to wait for the current wb 
                          // read miss access to complete. Also for a write idle_hit, need 
                          // to stall for 1 cycle while the data cache is being written to
assign write_state      = c_state == CS_IDLE || c_state == CS_WRITE_HIT ||  
                          c_state == CS_WRITE_HIT_WAIT_WB ||  c_state == CS_WRITE_MISS_WAIT_WB;
 
assign write_stall      = (write_miss && !(i_wb_cached_ready && write_state)) || (write_hit && !i_wb_cached_ready);
 
//assign read_stall       = request_hold && !idle_hit && !rbuf_hit && !wb_hit && !i_write_enable;

assign read_stall       = request_hold && !idle_hit && !wb_hit && !i_write_enable;
 
assign cache_busy_stall = c_state == CS_FILL_COMPLETE || c_state == CS_TURN_AROUND || c_state == CS_INIT ||
//                          (fill_state && !rbuf_hit && !wb_hit) ||
                          (fill_state && !wb_hit) ||
                          (c_state == CS_WRITE_HIT && !consecutive_write);
 
 
// ======================================
// Instantiate RAMS
// ======================================
 
//generate
//    for ( i=0; i<WAYS;i=i+1 ) begin : rams
 
        // Tag RAMs 
//        `ifdef XILINX_SPARTAN6_FPGA
//        xs6_sram_256x21_line_en
//        `endif
 
//        `ifdef XILINX_VIRTEX6_FPGA
//        xv6_sram_256x21_line_en
//        `endif
 
//        `ifndef XILINX_FPGA
//        generic_sram_line_en 
//        `endif
 
//            #(
//            .DATA_WIDTH                 ( TAG_WIDTH             ),
//            .INITIALIZE_TO_ZERO         ( 1                     ),
//            .ADDRESS_WIDTH              ( CACHE_ADDR_WIDTH      ))
        single_port_ram_21_8 u_tag0 (
            .clk                    ( i_clk                 ),
            .data                   ( tag_wdata             ),
            .we                     ( tag_wenable_way[0]    ),
            .addr                   ( tag_address           ),
            .out                    ( tag_rdata_way0      )
            );
 
        // Data RAMs 
//        `ifdef XILINX_SPARTAN6_FPGA
//        xs6_sram_256x128_byte_en
//        `endif
 
//       `ifdef XILINX_VIRTEX6_FPGA
//       xv6_sram_256x128_byte_en
//        `endif
 
//        `ifndef XILINX_FPGA
//        generic_sram_byte_en0
//        `endif
 
//            #(
//            .DATA_WIDTH    ( CACHE_LINE_WIDTH) ,
//            .ADDRESS_WIDTH ( CACHE_ADDR_WIDTH) )
        single_port_ram_128_8 u_data0 (
            .clk                      ( i_clk                         ),
            .data                     ( data_wdata                    ),
            .we                       ( data_wenable_way[0]           ),
            .addr                     ( data_address                  ),
//            .i_byte_enable              ( {CACHE_LINE_WIDTH/8{1'd1}}    ),
            .out                      ( data_rdata_way0             )
            );                                                     
 
 
        // Per tag-ram write-enable
        assign tag_wenable_way[0]  = tag_wenable && ( select_way[0] || source_sel[C_INIT] );
 
        // Per data-ram write-enable
        assign data_wenable_way[0] = (source_sel[C_FILL] && select_way[0]) || 
                                     (write_hit && data_hit_way[0] && c_state == CS_IDLE) ||
                                     (consecutive_write && data_hit_way_r[0]);
        // Per data-ram idle_hit flag
        assign data_hit_way[0]     = tag_rdata_way0[TAG_WIDTH-1] &&                                                  
                                     tag_rdata_way0[19:0] == i_address[31:12] &&  
                                     c_state == CS_IDLE;   



 	 single_port_ram_21_8 u_tag1 (
        	.clk                    ( i_clk                 ),
         	.data                   ( tag_wdata             ),
        	.we                     ( tag_wenable_way[1]    ),
        	.addr                   ( tag_address           ),
       		.out                    ( tag_rdata_way1      )
      		);

  
        single_port_ram_128_8 u_data1 (
            .clk                      ( i_clk                         ),
            .data                     ( data_wdata                    ),
            .we                       ( data_wenable_way[1]           ),
            .addr                     ( data_address                  ),
//            .i_byte_enable              ( {CACHE_LINE_WIDTH/8{1'd1}}    ),
            .out                      ( data_rdata_way1             )
            );          

        // Per tag-ram write-enable
        assign tag_wenable_way[1]  = tag_wenable && ( select_way[1] || source_sel[C_INIT] );
 
        // Per data-ram write-enable
        assign data_wenable_way[1] = (source_sel[C_FILL] && select_way[1]) || 
                                     (write_hit && data_hit_way[1] && c_state == CS_IDLE) ||
                                     (consecutive_write && data_hit_way_r[1]);
        // Per data-ram idle_hit flag
        assign data_hit_way[1]     = tag_rdata_way1[TAG_WIDTH-1] &&                                                  
                                     tag_rdata_way1[19:0] == i_address[31:12] &&  
                                     c_state == CS_IDLE;             


  single_port_ram_21_8 u_tag2 (
            .clk                    ( i_clk                 ),
            .data                   ( tag_wdata             ),
            .we                     ( tag_wenable_way[2]    ),
            .addr                   ( tag_address           ),
            .out                    ( tag_rdata_way2      )
            );

  
        single_port_ram_128_8 u_data2 (
            .clk                      ( i_clk                         ),
            .data                     ( data_wdata                    ),
            .we                       ( data_wenable_way[2]           ),
            .addr                     ( data_address                  ),
//            .i_byte_enable              ( {CACHE_LINE_WIDTH/8{1'd1}}    ),
            .out                      ( data_rdata_way2             )
            );          

        // Per tag-ram write-enable
        assign tag_wenable_way[2]  = tag_wenable && ( select_way[2] || source_sel[C_INIT] );
 
        // Per data-ram write-enable
        assign data_wenable_way[2] = (source_sel[C_FILL] && select_way[2]) || 
                                     (write_hit && data_hit_way[2] && c_state == CS_IDLE) ||
                                     (consecutive_write && data_hit_way_r[2]);
        // Per data-ram idle_hit flag
        assign data_hit_way[2]     = tag_rdata_way2[TAG_WIDTH-1] &&                                                  
                                     tag_rdata_way2[19:0] == i_address[31:12] &&  
                                     c_state == CS_IDLE; 


  single_port_ram_21_8 u_tag3 (
            .clk                    ( i_clk                 ),
            .data                   ( tag_wdata             ),
            .we                     ( tag_wenable_way[3]    ),
            .addr                   ( tag_address           ),
            .out                    ( tag_rdata_way3      )
            );

  
        single_port_ram_128_8 u_data3 (
            .clk                      ( i_clk                         ),
            .data                     ( data_wdata                    ),
            .we                       ( data_wenable_way[3]           ),
            .addr                     ( data_address                  ),
//            .i_byte_enable              ( {CACHE_LINE_WIDTH/8{1'd1}}    ),
            .out                      ( data_rdata_way3             )
            );          

        // Per tag-ram write-enable
        assign tag_wenable_way[3]  = tag_wenable && ( select_way[3] || source_sel[C_INIT] );
 
        // Per data-ram write-enable
        assign data_wenable_way[3] = (source_sel[C_FILL] && select_way[3]) || 
                                     (write_hit && data_hit_way[3] && c_state == CS_IDLE) ||
                                     (consecutive_write && data_hit_way_r[3]);
        // Per data-ram idle_hit flag
        assign data_hit_way[3]     = tag_rdata_way3[TAG_WIDTH-1] &&                                                  
                                     tag_rdata_way3[19:0] == i_address[31:12] &&  
                                     c_state == CS_IDLE; 
                                      
//    end                                                         
//endgenerate
 
 
// ======================================
// Register Valid Bits
// ======================================
//generate
//if ( WAYS == 2 ) begin : valid_bits_2ways
 
//    always @ ( posedge i_clk )
//        if ( c_state == CS_IDLE )
//            valid_bits_r <= {tag_rdata_way[1][TAG_WIDTH-1], 
//                             tag_rdata_way[0][TAG_WIDTH-1]};
 
//end
//else if ( WAYS == 3 ) begin : valid_bits_3ways
 
//    always @ ( posedge i_clk )
//      if ( c_state == CS_IDLE )
//            valid_bits_r <= {tag_rdata_way[2][TAG_WIDTH-1], 
//                             tag_rdata_way[1][TAG_WIDTH-1], 
//                             tag_rdata_way[0][TAG_WIDTH-1]};
 
//end
//else if ( WAYS == 4 ) begin : valid_bits_4ways
 
    always @ ( posedge i_clk )
        if ( c_state == CS_IDLE )

            valid_bits_r <= {tag_rdata_way3[TAG_WIDTH-1], 
                             tag_rdata_way2[TAG_WIDTH-1], 
                             tag_rdata_way1[TAG_WIDTH-1], 
                             tag_rdata_way0[TAG_WIDTH-1]};
 
//end
//else begin : valid_bits_8ways
 
//    always @ ( posedge i_clk )
//        if ( c_state == CS_IDLE )
//            valid_bits_r <= {tag_rdata_way[7][TAG_WIDTH-1], 
//                             tag_rdata_way[6][TAG_WIDTH-1], 
//                             tag_rdata_way[5][TAG_WIDTH-1], 
//                             tag_rdata_way[4][TAG_WIDTH-1], 
//                             tag_rdata_way[3][TAG_WIDTH-1], 
//                             tag_rdata_way[2][TAG_WIDTH-1], 
//                            tag_rdata_way[1][TAG_WIDTH-1], 
//                             tag_rdata_way[0][TAG_WIDTH-1]};
 
//end
//endgenerate
 
 
// ======================================
// Select read idle_hit data
// ======================================
 
//generate
//if ( WAYS == 2 ) begin : read_data_2ways
 
//    assign hit_rdata    = data_hit_way[0] ? data_rdata_way[0] :
//                          data_hit_way[1] ? data_rdata_way[1] :
//                                     {CACHE_LINE_WIDTH{1'd1}} ;  // all 1's for debug
 
//end
//else if ( WAYS == 3 ) begin : read_data_3ways
 
//    assign hit_rdata    = data_hit_way[0] ? data_rdata_way[0] :
//                          data_hit_way[1] ? data_rdata_way[1] :
//                          data_hit_way[2] ? data_rdata_way[2] :
//                                     {CACHE_LINE_WIDTH{1'd1}} ;  // all 1's for debug
 
//end
//else if ( WAYS == 4 ) begin : read_data_4ways
 
    assign hit_rdata    = data_hit_way[0] ? data_rdata_way0 :
                          data_hit_way[1] ? data_rdata_way1 :
                          data_hit_way[2] ? data_rdata_way2 :
                          data_hit_way[3] ? data_rdata_way3 :
			128'hffffffffffffffffffffffffffffffff;
                                 //    {CACHE_LINE_WIDTH{1'd1}} ;  // all 1's for debug
 
//end
//else begin : read_data_8ways
 
//    assign hit_rdata    = data_hit_way[0] ? data_rdata_way[0] :
//                          data_hit_way[1] ? data_rdata_way[1] :
//                          data_hit_way[2] ? data_rdata_way[2] :
//                          data_hit_way[3] ? data_rdata_way[3] :
//                         data_hit_way[4] ? data_rdata_way[4] :
//                          data_hit_way[5] ? data_rdata_way[5] :
//                          data_hit_way[6] ? data_rdata_way[6] :
//                          data_hit_way[7] ? data_rdata_way[7] :
//                                     {CACHE_LINE_WIDTH{1'd1}} ;  // all 1's for debug
 
//end
//endgenerate
 
 
// ======================================
// Function to select the way to use
// for fills
// ======================================
//generate
//if ( WAYS == 2 ) begin : pick_way_2ways
 
 //   assign next_way = pick_way ( valid_bits_r, random_num );
 
//    function [WAYS-1:0] pick_way;
 //   input [WAYS-1:0] valid_bits;
//    input [3:0]      random_num;
 //   begin
 //       if (      valid_bits[0] == 1'd0 )
            // way 0 not occupied so use it
//            pick_way     = 2'b01;
//        else if ( valid_bits[1] == 1'd0 )
            // way 1 not occupied so use it
//            pick_way     = 2'b10;
//        else
//            begin
            // All ways occupied so pick one randomly
//            case (random_num[3:1])
//                3'd0, 3'd3,
//                3'd5, 3'd6: pick_way = 2'b10;
//                default:    pick_way = 2'b01;
//            endcase
//            end
//   end
//    endfunction
 
//end
//else if ( WAYS == 3 ) begin : pick_way_3ways
 
//    assign next_way = pick_way ( valid_bits_r, random_num );
 
//    function [WAYS-1:0] pick_way;
//    input [WAYS-1:0] valid_bits;
//    input [3:0]      random_num;
//    begin
//        if (      valid_bits[0] == 1'd0 )
 //           // way 0 not occupied so use it
//            pick_way     = 3'b001;
//        else if ( valid_bits[1] == 1'd0 )
//            // way 1 not occupied so use it
//            pick_way     = 3'b010;
//        else if ( valid_bits[2] == 1'd0 )
            // way 2 not occupied so use it
//            pick_way     = 3'b100;
//        else
 //           begin
            // All ways occupied so pick one randomly
 //           case (random_num[3:1])
 //               3'd0, 3'd1, 3'd2: pick_way = 3'b010;
 //               3'd2, 3'd3, 3'd4: pick_way = 3'b100;
 //               default:          pick_way = 3'b001;
//            endcase
//            end
//    end
//    endfunction
 
//end
//else if ( WAYS == 4 ) begin : pick_way_4ways
 
//    assign next_way = pick_way ( valid_bits_r, random_num );
 
assign next_way   = 	valid_bits_r[0] == 1'd0 ? 4'b0001:
			valid_bits_r[1] == 1'd0 ? 4'b0010:
			valid_bits_r[2] == 1'd0 ? 4'b0100:
			valid_bits_r[3] == 1'd0 ? 4'b1000:
						  (
						    random_num[3:1] == 3'd0 ? 4'b0100:
						    random_num[3:1] == 3'd1 ? 4'b0100:
						    random_num[3:1] == 3'd2 ? 4'b1000:
						    random_num[3:1] == 3'd3 ? 4'b1000:
						    random_num[3:1] == 3'd4 ? 4'b0001:
						    random_num[3:1] == 3'd5 ? 4'b0001:
						    			      4'b0010
							);

//    function [WAYS-1:0] pick_way;
//   input [WAYS-1:0] valid_bits;
//   input [3:0]      random_num;
//    begin
//        if (      valid_bits[0] == 1'd0 )
//            // way 0 not occupied so use it
//            pick_way     = 4'b0001;
//        else if ( valid_bits[1] == 1'd0 )
//            // way 1 not occupied so use it
//            pick_way     = 4'b0010;
//        else if ( valid_bits[2] == 1'd0 )
 //           // way 2 not occupied so use it
//            pick_way     = 4'b0100;
//        else if ( valid_bits[3] == 1'd0 )
//            // way 3 not occupied so use it
//            pick_way     = 4'b1000;
 //       else
//            begin
            // All ways occupied so pick one randomly
//            case (random_num[3:1])
//                3'd0, 3'd1: pick_way = 4'b0100;
//                3'd2, 3'd3: pick_way = 4'b1000;
//                3'd4, 3'd5: pick_way = 4'b0001;
//                default:    pick_way = 4'b0010;
//           endcase
//            end
//    end
//    endfunction
 
//end
//else begin : pick_way_8ways
 
//    assign next_way = pick_way ( valid_bits_r, random_num );
 
//    function [WAYS-1:0] pick_way;
//    input [WAYS-1:0] valid_bits;CACHE_LINE_WIDTH
//    input [3:0]      random_num;
 //   begin
 //       if (      valid_bits[0] == 1'd0 )
            // way 0 not occupied so use it
//            pick_way     = 8'b00000001;
//        else if ( valid_bits[1] == 1'd0 )
            // way 1 not occupied so use it
//            pick_way     = 8'b00000010;
//        else if ( valid_bits[2] == 1'd0 )
            // way 2 not occupied so use it
//            pick_way     = 8'b00000100;
//        else if ( valid_bits[3] == 1'd0 )
            // way 3 not occupied so use it
//            pick_way     = 8'b00001000;
//        else if ( valid_bits[4] == 1'd0 )
            // way 3 not occupied so use it
//            pick_way     = 8'b00010000;
//        else if ( valid_bits[5] == 1'd0 )
            // way 3 not occupied so use it
//            pick_way     = 8'b00100000;
//        else if ( valid_bits[6] == 1'd0 )
            // way 3 not occupied so use it
//            pick_way     = 8'b01000000;
//        else if ( valid_bits[7] == 1'd0 )
            // way 3 not occupied so use it
//            pick_way     = 8'b10000000;
//        else
 //           begin
            // All ways occupied so pick one randomly
//            case (random_num[3:1])
//                3'd0:       pick_way = 8'b00010000;
//               3'd1:       pick_way = 8'b00100000;
//                3'd2:       pick_way = 8'b01000000;
//                3'd3:       pick_way = 8'b10000000;
//                3'd4:       pick_way = 8'b00000001;
//               3'd5:       pick_way = 8'b00000010;
//                3'd6:       pick_way = 8'b00000100;
//                default:    pick_way = 8'b00001000;
//            endcase
//            end
//    end
//   endfunction
 
//end
//endgenerate

 
// ========================================================
// Debug WB bus - not synthesizable
// ========================================================
//synopsys translate_off
//wire    [(6*8)-1:0]     xSOURCE_SEL;
//wire    [(22*8)-1:0]    xC_STATE;
// 
//assign xSOURCE_SEL = source_sel[C_CORE]               ? "C_CORE"                :
//                     source_sel[C_INIT]               ? "C_INIT"                :
//                     source_sel[C_FILL]               ? "C_FILL"                :
//							"C_INVA"                ;
// 
//assign xC_STATE    = c_state == CS_INIT               ? "CS_INIT"               :
//                     c_state == CS_IDLE               ? "CS_IDLE"               :
//                     c_state == CS_FILL               ? "CS_FILL"               :
//                     c_state == CS_FILL_COMPLETE      ? "CS_FILL_COMPLETE"      :
//                     c_state == CS_EX_DELETE          ? "CS_EX_DELETE"          :
//                     c_state == CS_TURN_AROUND        ? "CS_TURN_AROUND"        :
//                     c_state == CS_WRITE_HIT          ? "CS_WRITE_HIT"          :
//                     c_state == CS_WRITE_HIT_WAIT_WB  ? "CS_WRITE_HIT_WAIT_WB"  :
//							"CS_WRITE_MISS_WAIT_WB" ;
 
/* 
generate
if ( WAYS == 2 ) begin : check_hit_2ways
 
    always @( posedge i_clk )
        if ( (data_hit_way[0] + data_hit_way[1] ) > 4'd1 )
            begin
            `TB_ERROR_MESSAGE
            $display("Hit in more than one cache ways!");                                                  
            end
 
end
else if ( WAYS == 3 ) begin : check_hit_3ways
 
    always @( posedge i_clk )
        if ( (data_hit_way[0] + data_hit_way[1] + data_hit_way[2] ) > 4'd1 )
            begin
            `TB_ERROR_MESSAGE
            $display("Hit in more than one cache ways!");                                                  
            end
 
end
else if ( WAYS == 4 ) begin : check_hit_4ways
 
    always @( posedge i_clk )
        if ( (data_hit_way[0] + data_hit_way[1] + 
              data_hit_way[2] + data_hit_way[3] ) > 4'd1 )
            begin
            `TB_ERROR_MESSAGE
            $display("Hit in more than one cache ways!");                                                  
            end
 
end
else if ( WAYS == 8 )  begin : check_hit_8ways
 
    always @( posedge i_clk )
        if ( (data_hit_way[0] + data_hit_way[1] + 
              data_hit_way[2] + data_hit_way[3] +
              data_hit_way[4] + data_hit_way[5] +
              data_hit_way[6] + data_hit_way[7] ) > 4'd1 )
            begin
            `TB_ERROR_MESSAGE
            $display("Hit in more than one cache ways!");                                                  
            end
 
end
else begin : check_hit_nways
 
    initial
        begin
        ` 
        $display("Unsupported number of ways %0d", WAYS);
        $display("Set A25_DCACHE_WAYS in a25_config_defines.v to either 2,3,4 or 8");
        end
 
end
endgenerate
*/ 
 
//synopsys translate_on
 
endmodule
 


module a25_mem(
		i_clk,
		i_fetch_stall,          
		i_exec_stall,          
		o_mem_stall,        
 
		i_daddress,
		i_daddress_valid,
		i_daddress_nxt,        
		i_write_data,
		i_write_enable,
		i_exclusive,            
		i_byte_enable,
		i_exec_load_rd,         
		i_cache_enable,       
		i_cache_flush,    
		i_cacheable_area,    
 
		o_mem_read_data,
		o_mem_read_data_valid,
		o_mem_load_rd,        
                                                       
		o_wb_cached_req,  
		o_wb_uncached_req,     
		o_wb_write,            
		o_wb_byte_enable,   
		o_wb_write_data,
		o_wb_address,                                         
		i_wb_uncached_rdata,                            
		i_wb_cached_rdata,                                
		i_wb_cached_ready,      
		i_wb_uncached_ready     
		);
 


input                       i_clk;
input                       i_fetch_stall;          // Fetch stage asserting stall
input                       i_exec_stall;           // Execute stage asserting stall
output                      o_mem_stall;            // Mem stage asserting stall
 
input       [31:0]          i_daddress;
input                       i_daddress_valid;
input       [31:0]          i_daddress_nxt;         // un-registered version of address to the cache rams
input       [31:0]          i_write_data;
input                       i_write_enable;
input                       i_exclusive;            // high for read part of swap access
input       [3:0]           i_byte_enable;
input       [8:0]           i_exec_load_rd;         // The destination register for a load instruction
input                       i_cache_enable;         // cache enable
input                       i_cache_flush;          // cache flush
input       [31:0]          i_cacheable_area;       // each bit corresponds to 2MB address space
 
output      [31:0]          o_mem_read_data;
output                      o_mem_read_data_valid;
output      [10:0]          o_mem_load_rd;          // The destination register for a load instruction
 
// Wishbone accesses                                                         
output                      o_wb_cached_req;        // Cached Request
output                      o_wb_uncached_req;      // Unached Request
output                      o_wb_write;             // Read=0, Write=1
output     [15:0]           o_wb_byte_enable;       // byte eable
output     [127:0]          o_wb_write_data;
output     [31:0]           o_wb_address;           // wb bus                                 
input      [127:0]          i_wb_uncached_rdata;    // wb bus                              
input      [127:0]          i_wb_cached_rdata;      // wb bus                              
input                       i_wb_cached_ready;      // wishbone access complete and read data valid
input                       i_wb_uncached_ready;     // wishbone access complete and read data valid
 
wire    [31:0]              cache_read_data;
wire                        address_cachable;
wire                        sel_cache_p;
wire                        sel_cache;
wire                        cached_wb_req;
wire                        uncached_data_access;
wire                        uncached_data_access_p;
wire                        cache_stall;
wire                        uncached_wb_wait;
reg                         uncached_wb_req_r = 1'd0;
reg                         uncached_wb_stop_r = 1'd0;
reg                         cached_wb_stop_r = 1'd0;
wire                        daddress_valid_p;  // pulse
reg      [31:0]             mem_read_data_r = 32'd0;
reg                         mem_read_data_valid_r = 1'd0;
reg      [10:0]             mem_load_rd_r = 11'd0;
wire     [10:0]             mem_load_rd_c;
wire     [31:0]             mem_read_data_c;
wire                        mem_read_data_valid_c;
reg                         mem_stall_r = 1'd0;
wire                        use_mem_reg;
reg                         fetch_only_stall_r = 1'd0;
wire                        fetch_only_stall;
wire                        void_output;
wire                        wb_stop;
reg                         daddress_valid_stop_r = 1'd0;
wire     [31:0]             wb_rdata32;
 
// ======================================
// Memory Decode
// ======================================

//assign address_cachable         = in_cachable_mem( i_daddress ) && i_cacheable_area[i_daddress[25:21]];
//in_cachable_mem = in_loboot_mem  ( address ) ||  in_main_mem  ( address ) ; 

 
// e.g. 24 for 32MBytes, 26 for 128MBytes
localparam MAIN_MSB             = 26; 
 
// e.g. 13 for 4k words
localparam BOOT_MSB             = 13;
 
localparam MAIN_BASE            = 32'h00000000; /*  Main Memory            */
localparam BOOT_BASE            = 32'h00000000; /*  Cachable Boot Memory   */
localparam AMBER_TM_BASE        = 16'h1300;      /*  Timers Module          */
localparam AMBER_IC_BASE        = 16'h1400;      /*  Interrupt Controller   */
localparam AMBER_UART0_BASE     = 16'h1600;      /*  UART 0                 */
localparam AMBER_UART1_BASE     = 16'h1700;      /*  UART 1                 */
localparam ETHMAC_BASE          = 16'h2000;      /*  Ethernet MAC           */
localparam HIBOOT_BASE          = 32'h28000000; /*  Uncachable Boot Memory */
localparam TEST_BASE            = 16'hf000;      /*  Test Module            */
 
 
 
//function in_loboot_mem;
//    input [31:0] address;
//begin
//in_loboot_mem  = (address >= BOOT_BASE   && 
//                 address < (BOOT_BASE   + 2**(BOOT_MSB+1)-1));
//end
//endfunction
 
 
//function in_hiboot_mem;
//    input [31:0] address;
//begin
//in_hiboot_mem  = (address[31:BOOT_MSB+1] == HIBOOT_BASE[31:BOOT_MSB+1]);
//		= (address[31:14] == HIBOOT_BASE>>(BOOT_MSB+1));   fixed

//end
//endfunction
 
 
//function in_boot_mem;
//    input [31:0] address;
//begin
//in_boot_mem  =  in_loboot_mem(address) || in_hiboot_mem(address);
//end
//endfunction
 
 
//function in_main_mem;
//    input [31:0] address;
//begin
//in_main_mem  = (address >= MAIN_BASE   && 
//                address < (MAIN_BASE   + 2**(MAIN_MSB+1)-1)) &&
//                !in_boot_mem ( address );
//end
//endfunction
 
 
// UART 0 address space
//function in_uart0;
//    input [31:0] address;
//begin
//    in_uart0 = address [31:16] == AMBER_UART0_BASE;
//end
//endfunction
 
 
// UART 1 address space
//function in_uart1;
//    input [31:0] address;
//begin
//    in_uart1 = address [31:16] == AMBER_UART1_BASE;
//end
//endfunction
 
 
// Interrupt Controller address space
//function in_ic;
//    input [31:0] address;
//begin
//    in_ic = address [31:16] == AMBER_IC_BASE;
//end
//endfunction
 
 
// Timer Module address space
//function in_tm;
//    input [31:0] address;
//begin
//    in_tm = address [31:16] == AMBER_TM_BASE;
//end
//endfunction
 
 
// Test module
//function in_test;
//    input [31:0] address;
//begin
//    in_test = address [31:16] == TEST_BASE;
//end
//endfunction
 
 
// Ethernet MAC
//function in_ethmac;
//    input [31:0] address;
//begin
//    in_ethmac = address [31:16] == ETHMAC_BASE;
//end
//endfunction
 
 
// Used in fetch.v and l2cache.v to allow accesses to these addresses
// to be cached
//function in_cachable_mem;
//    input [31:0] address;
//begin
//    in_cachable_mem = in_loboot_mem     ( address ) || 
//                      in_main_mem       ( address ) ; 
//end
//endfunction



assign address_cachable         = (
					 (i_daddress >= BOOT_BASE  &&  i_daddress < (BOOT_BASE   + 32'h7fff))     // in_loboot_mem  ( address )
					|| ( 
						(i_daddress >= MAIN_BASE  &&  i_daddress < (MAIN_BASE   + 32'hfffffff)) 
							&& !( (i_daddress >= BOOT_BASE && i_daddress < (BOOT_BASE  + 32'h7fff))
											||(i_daddress[31:14] == HIBOOT_BASE>>(14)))
					   )
				  )
				&& ((i_daddress[25:21] == 5'b00000) ? i_cacheable_area[0] :
					(i_daddress[25:21] == 5'b00001) ? i_cacheable_area[1] :
					(i_daddress[25:21] == 5'b00010) ? i_cacheable_area[2] :
					(i_daddress[25:21] == 5'b00011) ? i_cacheable_area[3] :
					(i_daddress[25:21] == 5'b00100) ? i_cacheable_area[4] :
					(i_daddress[25:21] == 5'b00101) ? i_cacheable_area[5] :
					(i_daddress[25:21] == 5'b00110) ? i_cacheable_area[6] :
					(i_daddress[25:21] == 5'b00111) ? i_cacheable_area[7] :
					(i_daddress[25:21] == 5'b01000) ? i_cacheable_area[8] :
					(i_daddress[25:21] == 5'b01001) ? i_cacheable_area[9] :
					(i_daddress[25:21] == 5'b01010) ? i_cacheable_area[10] :
					(i_daddress[25:21] == 5'b01011) ? i_cacheable_area[11] :
					(i_daddress[25:21] == 5'b01100) ? i_cacheable_area[12] :
					(i_daddress[25:21] == 5'b01101) ? i_cacheable_area[13] :
					(i_daddress[25:21] == 5'b01110) ? i_cacheable_area[14] :
					(i_daddress[25:21] == 5'b01111) ? i_cacheable_area[15] :
					(i_daddress[25:21] == 5'b10000) ? i_cacheable_area[16] :
					(i_daddress[25:21] == 5'b10001) ? i_cacheable_area[17] :
					(i_daddress[25:21] == 5'b10010) ? i_cacheable_area[18] :
					(i_daddress[25:21] == 5'b10011) ? i_cacheable_area[19] :
					(i_daddress[25:21] == 5'b10100) ? i_cacheable_area[20] :
					(i_daddress[25:21] == 5'b10101) ? i_cacheable_area[21] :
					(i_daddress[25:21] == 5'b10110) ? i_cacheable_area[22] :
					(i_daddress[25:21] == 5'b10111) ? i_cacheable_area[23] :
					(i_daddress[25:21] == 5'b11000) ? i_cacheable_area[24] :
					(i_daddress[25:21] == 5'b11001) ? i_cacheable_area[25] :
					(i_daddress[25:21] == 5'b11010) ? i_cacheable_area[26] :
					(i_daddress[25:21] == 5'b11011) ? i_cacheable_area[27] :
					(i_daddress[25:21] == 5'b11100) ? i_cacheable_area[28] :
					(i_daddress[25:21] == 5'b11101) ? i_cacheable_area[29] :
					(i_daddress[25:21] == 5'b11110) ? i_cacheable_area[30] :
					i_cacheable_area[31] );

//i_cacheable_area[i_daddress[25:21]];
assign sel_cache_p              = daddress_valid_p && address_cachable && i_cache_enable && !i_exclusive;
assign sel_cache                = i_daddress_valid && address_cachable && i_cache_enable && !i_exclusive;
assign uncached_data_access     = i_daddress_valid && !sel_cache && !cache_stall;
assign uncached_data_access_p   = daddress_valid_p && !sel_cache;
 
assign use_mem_reg              = wb_stop && !mem_stall_r;
assign o_mem_read_data          = use_mem_reg ? mem_read_data_r       : mem_read_data_c;
assign o_mem_load_rd            = use_mem_reg ? mem_load_rd_r         : mem_load_rd_c;
assign o_mem_read_data_valid    = !void_output && (use_mem_reg ? mem_read_data_valid_r : mem_read_data_valid_c);
 
 
// Return read data either from the wishbone bus or the cache
assign wb_rdata32               = i_daddress[3:2] == 2'd0 ? i_wb_uncached_rdata[ 31: 0] :
                                  i_daddress[3:2] == 2'd1 ? i_wb_uncached_rdata[ 63:32] :
                                  i_daddress[3:2] == 2'd2 ? i_wb_uncached_rdata[ 95:64] :
                                                            i_wb_uncached_rdata[127:96] ;
 
assign mem_read_data_c          = sel_cache             ? cache_read_data : 
                                  uncached_data_access  ? wb_rdata32      :
                                                          32'h76543210    ;
 
assign mem_load_rd_c            = {i_daddress[1:0], i_exec_load_rd};
assign mem_read_data_valid_c    = i_daddress_valid && !i_write_enable && !o_mem_stall;
 
assign o_mem_stall              = uncached_wb_wait || cache_stall;
 
// Request wishbone access
assign o_wb_byte_enable         = i_daddress[3:2] == 2'd0 ? {12'd0, i_byte_enable       } :
                                  i_daddress[3:2] == 2'd1 ? { 8'd0, i_byte_enable,  4'd0} :
                                  i_daddress[3:2] == 2'd2 ? { 4'd0, i_byte_enable,  8'd0} :
                                                            {       i_byte_enable, 12'd0} ;
 
assign o_wb_write               = i_write_enable;
assign o_wb_address             = {i_daddress[31:2], 2'd0};
assign o_wb_write_data          = {4{i_write_data}};
assign o_wb_cached_req          = !cached_wb_stop_r && cached_wb_req;
assign o_wb_uncached_req        = !uncached_wb_stop_r && uncached_data_access_p;
 
assign uncached_wb_wait         = (o_wb_uncached_req || uncached_wb_req_r) && !i_wb_uncached_ready;
 
always @( posedge i_clk )
    begin
    uncached_wb_req_r <=  (o_wb_uncached_req || uncached_wb_req_r) && !i_wb_uncached_ready;
    end
 
assign fetch_only_stall     = i_fetch_stall && !o_mem_stall;
 
always @( posedge i_clk )
    fetch_only_stall_r <= fetch_only_stall;
 
assign void_output = (fetch_only_stall_r && fetch_only_stall) || (fetch_only_stall_r && mem_read_data_valid_r);
 
 
// pulse this signal
assign daddress_valid_p = i_daddress_valid && !daddress_valid_stop_r;
 
always @( posedge i_clk )
    begin
    uncached_wb_stop_r      <= (uncached_wb_stop_r || (uncached_data_access_p&&!cache_stall)) && (i_fetch_stall || o_mem_stall);
    cached_wb_stop_r        <= (cached_wb_stop_r   || cached_wb_req)          && (i_fetch_stall || o_mem_stall);
    daddress_valid_stop_r   <= (daddress_valid_stop_r || daddress_valid_p)    && (i_fetch_stall || o_mem_stall);
    // hold this until the mem access completes
    mem_stall_r <= o_mem_stall;
    end
 
 
assign wb_stop = uncached_wb_stop_r || cached_wb_stop_r;
 
always @( posedge i_clk )
    if ( !wb_stop || o_mem_stall )
        begin
        mem_read_data_r         <= mem_read_data_c;
        mem_load_rd_r           <= mem_load_rd_c;
        mem_read_data_valid_r   <= mem_read_data_valid_c;
        end
 
 
// ======================================
// L1 Data Cache
// ======================================
a25_dcache u_dcache (
    .i_clk                      ( i_clk                 ),
    .i_request                  ( sel_cache_p           ),
    .i_exclusive                ( i_exclusive           ),
    .i_write_data               ( i_write_data          ),
    .i_write_enable             ( i_write_enable        ),
    .i_address                  ( i_daddress            ),
    .i_address_nxt              ( i_daddress_nxt        ),
    .i_byte_enable              ( i_byte_enable         ),
    .i_cache_enable             ( i_cache_enable        ),
    .i_cache_flush              ( i_cache_flush         ),
    .i_fetch_stall              ( i_fetch_stall         ),
    .i_exec_stall               ( i_exec_stall          ),
    .i_wb_cached_rdata          ( i_wb_cached_rdata     ),
    .i_wb_cached_ready          ( i_wb_cached_ready     ),

   .o_read_data                ( cache_read_data       ), 
   .o_stall                    ( cache_stall           ),
   .o_wb_cached_req            ( cached_wb_req         )

);
 
 
 
endmodule


 
 
module a25_write_back(
			i_clk,
			i_mem_stall,                
 
			i_mem_read_data,          
			i_mem_read_data_valid,    
			i_mem_load_rd,           
 
			o_wb_read_data,           
			o_wb_read_data_valid,     
			o_wb_load_rd,           
 
			i_daddress,
//			i_daddress_valid
			);

input                       i_clk;
input                       i_mem_stall;                // Mem stage asserting stall
 
input       [31:0]          i_mem_read_data;            // data reads
input                       i_mem_read_data_valid;      // read data is valid
input       [10:0]          i_mem_load_rd;              // Rd for data reads
 
output      [31:0]          o_wb_read_data;            // data reads
output                      o_wb_read_data_valid;    // read data is valid
output      [10:0]          o_wb_load_rd;              // Rd for data reads
 
input       [31:0]          i_daddress;
//input                       i_daddress_valid;
 
reg  [31:0]         mem_read_data_r = 32'd0;          // Register read data from Data Cache
reg                 mem_read_data_valid_r = 1'd0;    // Register read data from Data Cache
reg  [10:0]         mem_load_rd_r = 11'd0;            // Register the Rd value for loads
 
assign o_wb_read_data       = mem_read_data_r;
assign o_wb_read_data_valid = mem_read_data_valid_r;
assign o_wb_load_rd         = mem_load_rd_r;
 
 
always @( posedge i_clk )
    if ( !i_mem_stall )
        begin                                                                                                             
        mem_read_data_r         <= i_mem_read_data;
        mem_read_data_valid_r   <= i_mem_read_data_valid;
        mem_load_rd_r           <= i_mem_load_rd;
        end
 
 
// Used by a25_decompile.v, so simulation only
//synopsys translate_off    
reg  [31:0]         daddress_r = 32'd0;               // Register read data from Data Cache
always @( posedge i_clk )
    if ( !i_mem_stall )
        daddress_r              <= i_daddress;
//synopsys translate_on    
 
endmodule
 


 module a25_wishbone_buf (
 			i_clk,
 
// Core side
			i_req,
			i_write,
			i_wdata,
			i_be,
			i_addr,
			o_rdata,
			o_ack,
 
// Wishbone side
			o_valid,
			i_accepted,
			o_write,
			o_wdata,
			o_be,
			o_addr,
			i_rdata,
			i_rdata_valid
			);
 
input                       i_clk;
 
// Core side
input                       i_req;
input                       i_write;
input       [127:0]         i_wdata;
input       [15:0]          i_be;
input       [31:0]          i_addr;
output      [127:0]         o_rdata;
output                      o_ack;
 
// Wishbone side
output                      o_valid;
input                       i_accepted;
output                      o_write;
output      [127:0]         o_wdata;
output      [15:0]          o_be;
output      [31:0]          o_addr;
input       [127:0]         i_rdata;
input                       i_rdata_valid;
 
// ----------------------------------------------------
// Signals
// ----------------------------------------------------
reg  [1:0]                  wbuf_used_r     = 2'd0;
//reg  [31:0]                 wbuf_addr_r     [1:0]; 
reg  [31:0]                 wbuf_addr_r0;
reg  [31:0]                 wbuf_addr_r1;
//reg  [127:0]                wbuf_wdata_r    [1:0]; 
reg  [127:0]                wbuf_wdata_r0;
reg  [127:0]                wbuf_wdata_r1;
//reg  [15:0]                 wbuf_be_r       [1:0]; 
reg  [15:0]                 wbuf_be_r0;
reg  [15:0]                 wbuf_be_r1;
reg  [1:0]                  wbuf_write_r    = 2'd0;
reg                         wbuf_wp_r       = 1'd0;        // write buf write pointer
reg                         wbuf_rp_r       = 1'd0;        // write buf read pointer
reg                         busy_reading_r  = 1'd0;
reg                         wait_rdata_valid_r = 1'd0;
wire                        in_wreq;
reg                         ack_owed_r      = 1'd0;
reg			    push;  //wire to reg	
reg			    pop;	//wire to reg
 
// ----------------------------------------------------
// Access Buffer
// ----------------------------------------------------
assign in_wreq = i_req && i_write;
assign push    = i_req && !busy_reading_r && (wbuf_used_r == 2'd1 || (wbuf_used_r == 2'd0 && !i_accepted));
assign pop     = o_valid && i_accepted && wbuf_used_r != 2'd0;
 
always @(posedge i_clk)
    if (push && pop)
        wbuf_used_r     <= wbuf_used_r;
    else if (push)
        wbuf_used_r     <= wbuf_used_r + 1'd1;
    else if (pop)
        wbuf_used_r     <= wbuf_used_r - 1'd1;
 
always @(posedge i_clk)
    if (push && in_wreq && !o_ack)
        ack_owed_r <= 1'd1;
    else if (!i_req && o_ack)
        ack_owed_r <= 1'd0;
 
always @(posedge i_clk)
    if (push)
        begin
		if ( wbuf_wp_r == 1'd0)
		begin
		wbuf_wdata_r0   <= i_wdata;
      	  	wbuf_addr_r0   <= i_addr;
      	  	wbuf_be_r0   <= i_write ? i_be : 16'hffff;
        	wbuf_write_r [0]   <= i_write;
		end


       		else if ( wbuf_wp_r == 1'd1)
		begin
		wbuf_wdata_r1   <= i_wdata;
        	wbuf_addr_r1    <= i_addr;
        	wbuf_be_r1      <= i_write ? i_be : 16'hffff;
        	wbuf_write_r [1]   <= i_write;
		end

        	wbuf_wp_r                  <= !wbuf_wp_r;
        end
 
always @(posedge i_clk)
    if (pop)
        wbuf_rp_r                  <= !wbuf_rp_r;
 
 
// ----------------------------------------------------
// Output logic
// ----------------------------------------------------
//assign o_wdata = wbuf_used_r != 2'd0 ? wbuf_wdata_r[wbuf_rp_r] : i_wdata;
assign o_wdata = wbuf_used_r != 2'd0 ? (wbuf_rp_r == 1'd0 ? wbuf_wdata_r0 : wbuf_wdata_r1 ) : i_wdata;

//assign o_write = wbuf_used_r != 2'd0 ? wbuf_write_r[wbuf_rp_r] : i_write;
assign o_write = wbuf_used_r != 2'd0 ? (wbuf_rp_r == 1'd0 ? wbuf_write_r[0] : wbuf_write_r[1]) : i_write;

//assign o_addr  = wbuf_used_r != 2'd0 ? wbuf_addr_r [wbuf_rp_r] : i_addr;
assign o_addr  = wbuf_used_r != 2'd0 ? (wbuf_rp_r == 1'd0 ? wbuf_addr_r0 : wbuf_addr_r1 ) : i_addr;

//assign o_be    = wbuf_used_r != 2'd0 ? wbuf_be_r   [wbuf_rp_r] : i_write ? i_be : 16'hffff;
assign o_be    = wbuf_used_r != 2'd0 ? (wbuf_rp_r == 1'd0 ? wbuf_be_r0 : wbuf_be_r1) : i_write ? i_be : 16'hffff;

assign o_ack   = (in_wreq ? (wbuf_used_r == 2'd0) : i_rdata_valid) || (ack_owed_r && pop); 

assign o_valid = (wbuf_used_r != 2'd0 || i_req) && !wait_rdata_valid_r;
 
assign o_rdata = i_rdata;
 
 
always@(posedge i_clk)
    if (o_valid && !o_write)
        busy_reading_r <= 1'd1;
    else if (i_rdata_valid)
        busy_reading_r <= 1'd0;
 
always@(posedge i_clk)
    if (o_valid && !o_write && i_accepted)
        wait_rdata_valid_r <= 1'd1;
    else if (i_rdata_valid)  
        wait_rdata_valid_r <= 1'd0;
endmodule
 
 

module a25_wishbone(
			i_clk,
 
			i_port0_req,
			o_port0_ack,
			i_port0_write,
			i_port0_wdata,
			i_port0_be,
			i_port0_addr,
			o_port0_rdata,
 
			i_port1_req,
			o_port1_ack,
			i_port1_write,
			i_port1_wdata,
			i_port1_be,
			i_port1_addr,
			o_port1_rdata,
 
			i_port2_req,
			o_port2_ack,
			i_port2_write,
			i_port2_wdata,
			i_port2_be,
			i_port2_addr,
			o_port2_rdata,
 
 
			o_wb_adr,
			o_wb_sel,
			o_wb_we,
			o_wb_dat,
			o_wb_cyc,
			o_wb_stb,
			i_wb_dat,
			i_wb_ack
//			i_wb_err
		);
 
 
// ----------------------------------------------------
// Parameters
// ----------------------------------------------------
localparam WBUF = 3;
 
input                       i_clk;
 
 
// Port 0 - dcache uncached
input                       i_port0_req;
output                      o_port0_ack;
input                       i_port0_write;
input       [127:0]         i_port0_wdata;
input       [15:0]          i_port0_be;
input       [31:0]          i_port0_addr;
output      [127:0]         o_port0_rdata;
 
// Port 1 - dcache cached
input                       i_port1_req;
output                      o_port1_ack;
input                       i_port1_write;
input       [127:0]         i_port1_wdata;
input       [15:0]          i_port1_be;
input       [31:0]          i_port1_addr;
output      [127:0]         o_port1_rdata;
 
// Port 2 - instruction cache accesses, read only
input                       i_port2_req;
output                      o_port2_ack;
input                       i_port2_write;
input       [127:0]         i_port2_wdata;
input       [15:0]          i_port2_be;
input       [31:0]          i_port2_addr;
output      [127:0]         o_port2_rdata;
 
 
// 128-bit Wishbone Bus
output      [31:0]          o_wb_adr;
output      [15:0]          o_wb_sel;
output                      o_wb_we;
output      [127:0]         o_wb_dat;
output                      o_wb_cyc;
output                      o_wb_stb;
input       [127:0]         i_wb_dat;
input                       i_wb_ack;
//input                       i_wb_err;
 
// ----------------------------------------------------
// Signals
// ----------------------------------------------------

reg        		    o_wb_adr = 32'd0;
reg             	    o_wb_sel = 16'd0;
reg                         o_wb_we  = 1'd0;
reg        		    o_wb_dat = 128'd0;
reg     	            o_wb_cyc = 1'd0;
reg        	            o_wb_stb = 1'd0;
wire [WBUF-1:0]             wbuf_valid;
wire [WBUF-1:0]             wbuf_accepted;
wire [WBUF-1:0]             wbuf_write;
//wire [127:0]                wbuf_wdata          [WBUF-1:0];
//wire [15:0]                 wbuf_be             [WBUF-1:0];
//wire [31:0]                 wbuf_addr           [WBUF-1:0];
wire [127:0]                wbuf_wdata0; 
wire [127:0]                wbuf_wdata1;
wire [127:0]                wbuf_wdata2;        
wire [15:0]                 wbuf_be0;            
wire [15:0]                 wbuf_be1; 
wire [15:0]                 wbuf_be2; 
wire [31:0]                 wbuf_addr0;   
wire [31:0]                 wbuf_addr1;
wire [31:0]                 wbuf_addr2;
wire [WBUF-1:0]             wbuf_rdata_valid;
wire                        new_access;
reg  [WBUF-1:0]             serving_port = 3'd0;
 

// ----------------------------------------------------
// Instantiate the write buffers
// ----------------------------------------------------

a25_wishbone_buf u_wishbone_buf_p0 (
    .i_clk          ( i_clk                 ),
 
    .i_req          ( i_port0_req           ),
    .i_write        ( i_port0_write         ),
    .i_wdata        ( i_port0_wdata         ),
    .i_be           ( i_port0_be            ),
    .i_addr         ( i_port0_addr          ),
    .o_rdata        ( o_port0_rdata         ),
    .o_ack          ( o_port0_ack           ), 

    .o_valid        ( wbuf_valid       [0]  ),
    .i_accepted     ( wbuf_accepted    [0]  ),
    .o_write        ( wbuf_write       [0]  ),
    .o_wdata        ( wbuf_wdata0           ),
    .o_be           ( wbuf_be0              ),
    .o_addr         ( wbuf_addr0            ),
    .i_rdata        ( i_wb_dat              ),
    .i_rdata_valid  ( wbuf_rdata_valid [0]  )
    );
 
 
a25_wishbone_buf u_wishbone_buf_p1 (
    .i_clk          ( i_clk                 ),
 
    .i_req          ( i_port1_req           ),
    .i_write        ( i_port1_write         ),
    .i_wdata        ( i_port1_wdata         ),
    .i_be           ( i_port1_be            ),
    .i_addr         ( i_port1_addr          ),
    .o_rdata        ( o_port1_rdata         ),
    .o_ack          ( o_port1_ack           ),

    .o_valid        ( wbuf_valid        [1] ),
    .i_accepted     ( wbuf_accepted     [1] ),
    .o_write        ( wbuf_write        [1] ),
    .o_wdata        ( wbuf_wdata1           ),
    .o_be           ( wbuf_be1              ),
    .o_addr         ( wbuf_addr1            ),
    .i_rdata        ( i_wb_dat              ),
    .i_rdata_valid  ( wbuf_rdata_valid  [1] )
    );
 
 
a25_wishbone_buf u_wishbone_buf_p2 (
    .i_clk          ( i_clk                 ),
 
    .i_req          ( i_port2_req           ),
    .i_write        ( i_port2_write         ),
    .i_wdata        ( i_port2_wdata         ),
    .i_be           ( i_port2_be            ),
    .i_addr         ( i_port2_addr          ),
    .o_rdata        ( o_port2_rdata         ),
    .o_ack          ( o_port2_ack           ),

    .o_valid        ( wbuf_valid        [2] ),
    .i_accepted     ( wbuf_accepted     [2] ),
    .o_write        ( wbuf_write        [2] ),
    .o_wdata        ( wbuf_wdata2           ),
    .o_be           ( wbuf_be2              ),
    .o_addr         ( wbuf_addr2            ),
    .i_rdata        ( i_wb_dat              ),
    .i_rdata_valid  ( wbuf_rdata_valid  [2] )
    );    


assign new_access       = !o_wb_stb || i_wb_ack;

assign wbuf_accepted[0] = new_access &&  wbuf_valid[0];
assign wbuf_accepted[1] = new_access && !wbuf_valid[0] &&  wbuf_valid[1];
assign wbuf_accepted[2] = new_access && !wbuf_valid[0] && !wbuf_valid[1] && wbuf_valid[2];

//always @(posedge i_clk)
//	begin
    
//	wbuf_accepted[0] <= new_access &&  wbuf_valid[0];
    
//	wbuf_accepted[1] <= new_access && !wbuf_valid[0] &&  wbuf_valid[1];

//	wbuf_accepted[2] <= new_access && !wbuf_valid[0] && !wbuf_valid[1] && wbuf_valid[2];

//	end

 
always @(posedge i_clk)
    begin
    if (new_access)
        begin
        if (wbuf_valid[0])
            begin
            o_wb_adr        <= wbuf_addr0;
            o_wb_sel        <= wbuf_be0;
            o_wb_we         <= wbuf_write[0];
            o_wb_dat        <= wbuf_wdata0;
            o_wb_cyc        <= 1'd1;
            o_wb_stb        <= 1'd1;
            serving_port    <= 3'b001;
            end
        else if (wbuf_valid[1])
            begin
  //          o_wb_adr        <= wbuf_addr [1];
  //          o_wb_sel        <= wbuf_be   [1];
  //          o_wb_we         <= wbuf_write[1];
  //          o_wb_dat        <= wbuf_wdata[1];
            o_wb_adr        <= wbuf_addr1;
            o_wb_sel        <= wbuf_be1;
            o_wb_we         <= wbuf_write[1];
            o_wb_dat        <= wbuf_wdata1;
            o_wb_cyc        <= 1'd1;
            o_wb_stb        <= 1'd1;
            serving_port    <= 3'b010;
            end
        else if (wbuf_valid[2])
            begin
  //          o_wb_adr        <= wbuf_addr [2];
  //          o_wb_sel        <= wbuf_be   [2];
  //          o_wb_we         <= wbuf_write[2];
  //          o_wb_dat        <= wbuf_wdata[2];
            o_wb_adr        <= wbuf_addr2;
            o_wb_sel        <= wbuf_be2;
            o_wb_we         <= wbuf_write[2];
            o_wb_dat        <= wbuf_wdata2;
            o_wb_cyc        <= 1'd1;
            o_wb_stb        <= 1'd1;
            serving_port    <= 3'b100;
            end
        else
            begin
            o_wb_cyc        <= 1'd0;
            o_wb_stb        <= 1'd0;
 
            // Don't need to change these values because they are ignored
            // when stb is low, but it makes for a cleaner waveform, at the expense of a few gates
            o_wb_we         <= 1'd0;
            o_wb_adr        <= 32'd0;
            o_wb_dat        <= 128'd0;
 
            serving_port    <= 3'b000;
            end    
        end
    end
 
 
assign {wbuf_rdata_valid[2], wbuf_rdata_valid[1], wbuf_rdata_valid[0]} = {3{i_wb_ack & ~ o_wb_we}} & serving_port;
 
 
endmodule
 
 



 
module a25_coprocessor(
			i_clk,
			i_core_stall,     
//			i_copro_opcode1,
//			i_copro_opcode2,
			i_copro_crn,      
//			i_copro_crm,
//			i_copro_num,
			i_copro_operation,
			i_copro_write_data, 
			i_fault,       
			i_fault_status,
			i_fault_address,  
			o_copro_read_data,
			o_cache_enable,
			o_cache_flush,
			o_cacheable_area 
		);

/************************* IO Declarations *********************/
input                       i_clk;
input                       i_core_stall;     // stall all stages of the Amber core at the same time
//input       [2:0]           i_copro_opcode1;
//input       [2:0]           i_copro_opcode2;
input       [3:0]           i_copro_crn;      // Register Number 
//input       [3:0]           i_copro_crm;
//input       [3:0]           i_copro_num;
input       [1:0]           i_copro_operation;
input       [31:0]          i_copro_write_data;
 
input                       i_fault;          // high to latch the fault address and status
input       [7:0]           i_fault_status;
input       [31:0]          i_fault_address;  // the address that caused the fault
 
output      [31:0]          o_copro_read_data;
output                      o_cache_enable;
output                      o_cache_flush;
output      [31:0]          o_cacheable_area;

/*********************** Signal Declarations *******************/
reg      [31:0]          o_copro_read_data; 
// Bit 0 - Cache on(1)/off
// Bit 1 - Shared (1) or seperate User/Supervisor address space
// Bit 2 - address monitor mode(1)
reg [2:0]  cache_control = 3'b000;
 
// Bit 0 - 2MB memory from 0 to 0x01fffff cacheable(1)/not cachable
// Bit 1 - next 2MB region etc.
reg [31:0] cacheable_area = 32'h0;
 
// Marks memory regions as read only so writes are ignored by the cache
// Bit 0 - 2MB memory from 0 to 0x01fffff updateable(1)/not updateable
// Bit 1 - next 2MB region etc.
reg [31:0] updateable_area = 32'h0;
 
// Accesses to a region with a flag set in this register cause the
// cache to flush
// Bit 0 - 2MB memory from 0 to 0x01fffff
// Bit 1 - next 2MB region etc.
reg [31:0] disruptive_area = 32'h0;
 
 
reg [7:0]  fault_status  = 8'd0;
reg [31:0] fault_address = 32'b0;  // the address that caused the fault
 
wire       copro15_reg1_write;
 
 
// ---------------------------
// Outputs
// ---------------------------
assign o_cache_enable   = cache_control[0];
assign o_cache_flush    = copro15_reg1_write;
assign o_cacheable_area = cacheable_area;
 
// ---------------------------
// Capture an access fault address and status
// ---------------------------
always @ ( posedge i_clk )
    if ( !i_core_stall )
        begin
        if ( i_fault )
            begin
       
            fault_status    <= i_fault_status;
            fault_address   <= i_fault_address;
            end
        end
 
 
// ---------------------------
// Register Writes
// ---------------------------
always @ ( posedge i_clk )
    if ( !i_core_stall )         
        begin
        if ( i_copro_operation == 2'd2 )
            case ( i_copro_crn )
                4'd2: cache_control   <= i_copro_write_data[2:0];
                4'd3: cacheable_area  <= i_copro_write_data[31:0];
                4'd4: updateable_area <= i_copro_write_data[31:0];
                4'd5: disruptive_area <= i_copro_write_data[31:0];
		default: cache_control <=cache_control;
            endcase
        end
 
// Flush the cache
assign copro15_reg1_write = !i_core_stall && i_copro_operation == 2'd2 && i_copro_crn == 4'd1;
 
 
// ---------------------------
// Register Reads   
// ---------------------------
always @ ( posedge i_clk )        
    if ( !i_core_stall )
        case ( i_copro_crn )
            // ID Register - [31:24] Company id, [23:16] Manuf id, [15:8] Part type, [7:0] revision
            4'd0:    o_copro_read_data <= 32'h41560300;
            4'd2:    o_copro_read_data <= {29'd0, cache_control}; 
            4'd3:    o_copro_read_data <= cacheable_area; 
            4'd4:    o_copro_read_data <= updateable_area; 
            4'd5:    o_copro_read_data <= disruptive_area; 
            4'd6:    o_copro_read_data <= {24'd0, fault_status };
            4'd7:    o_copro_read_data <= fault_address;
            default: o_copro_read_data <= 32'd0;
        endcase
 
 
endmodule
 




 module arm_core(
		i_clk,
		i_irq,           
		i_firq,             
		i_system_rdy,     
		i_wb_dat, 
		i_wb_ack,
		i_wb_err,


//decode

//coprocessor
//		cache_enable,         // Enabel the cache
//		cache_flush,          // Flush the cache
//		cacheable_area,
//execute

//wishbone


//write_back

 

 		o_wb_adr,
		o_wb_sel,
		o_wb_we,
		o_wb_dat,
		o_wb_cyc,
		o_wb_stb
		);

input                       i_clk;
 
input                       i_irq;              // Interrupt request, active high
input                       i_firq;             // Fast Interrupt request, active high
 
input                       i_system_rdy;       // Amber is stalled when this is low
 
// Wishbone Master I/F
output      [31:0]          o_wb_adr;
output      [15:0]          o_wb_sel;
output                      o_wb_we;
input       [127:0]         i_wb_dat;
output      [127:0]         o_wb_dat;
output                      o_wb_cyc;
output                      o_wb_stb;
input                       i_wb_ack;		//Used to terminate read and write accesses
input                       i_wb_err;


//decode


//coprocessor

//input                      cache_enable;         // Enabel the cache
//input                      cache_flush;          // Flush the cache
//input      [31:0]          cacheable_area;

//execute


//wishbone


//write_back



wire      [31:0]          execute_iaddress;
wire                      execute_iaddress_valid;
wire      [31:0]          execute_iaddress_nxt;  // un-registered version of execute_address
                                                 // to the instruction cache rams
wire      [31:0]          execute_daddress;
wire                      execute_daddress_valid;
wire      [31:0]          execute_daddress_nxt; // un-registered version of execute_daddress
                                                // to the data cache rams
wire      [31:0]          write_data;
wire                      write_enable;
wire      [31:0]          fetch_instruction;
wire                      decode_exclusive;
wire                      decode_iaccess;
wire                      decode_daccess;
wire      [3:0]           byte_enable;
wire                      exclusive;            // swap access
wire                      cache_enable;         // Enabel the cache
wire                      cache_flush;          // Flush the cache
wire      [31:0]          cacheable_area;
 
wire                      fetch_stall; 
wire                      mem_stall; 
wire                      exec_stall; 
wire                      core_stall;   
 
wire     [1:0]            status_bits_mode;               
wire                      status_bits_irq_mask;           
wire                      status_bits_firq_mask;           
wire                      status_bits_flags_wen;          
wire                      status_bits_mode_wen;           
wire                      status_bits_irq_mask_wen;       
wire                      status_bits_firq_mask_wen;       
wire     [31:0]           execute_status_bits;
 
wire     [31:0]           imm32;                   
wire     [4:0]            imm_shift_amount; 
wire                      shift_imm_zero;      
wire     [3:0]            condition;               
 
wire     [3:0]            rm_sel;                  
wire     [3:0]            rs_sel;                 
wire     [7:0]            decode_load_rd;                 
wire     [8:0]            exec_load_rd;                 
wire     [3:0]            rn_sel;                  
wire     [1:0]            barrel_shift_amount_sel; 
wire     [1:0]            barrel_shift_data_sel;   
wire     [1:0]            barrel_shift_function;   
wire     [8:0]            alu_function;            
wire     [1:0]            multiply_function;       
wire     [2:0]            interrupt_vector_sel;    
wire     [3:0]            iaddress_sel;             
wire     [3:0]            daddress_sel;             
wire     [2:0]            pc_sel;                  
wire     [1:0]            byte_enable_sel;         
wire     [2:0]            status_bits_sel;                
wire     [2:0]            reg_write_sel;           
wire                      user_mode_regs_store_nxt;    
wire                      firq_not_user_mode;
 
wire                      write_data_wen;          
wire                      copro_write_data_wen;          
wire                      base_address_wen;        
wire                      pc_wen;                  
wire     [14:0]           reg_bank_wen;            
 
wire     [2:0]            copro_opcode1;
wire     [2:0]            copro_opcode2;
wire     [3:0]            copro_crn;    
wire     [3:0]            copro_crm;
wire     [3:0]            copro_num;
wire     [1:0]            copro_operation;
wire     [31:0]           copro_read_data;
wire     [31:0]           copro_write_data;
wire                      multiply_done;
 
wire                      decode_fault;
wire                      iabt_trigger;
wire                      dabt_trigger;
 
wire     [7:0]            decode_fault_status;
wire     [7:0]            iabt_fault_status;
wire     [7:0]            dabt_fault_status;
 
wire     [31:0]           decode_fault_address;
wire     [31:0]           iabt_fault_address;
wire     [31:0]           dabt_fault_address;
 
wire                      adex;
 
wire     [31:0]           mem_read_data;
wire                      mem_read_data_valid;
wire     [10:0]           mem_load_rd;  
 
wire     [31:0]           wb_read_data;
wire                      wb_read_data_valid;
wire     [10:0]           wb_load_rd;  
 
wire                      dcache_wb_cached_req;
wire                      dcache_wb_uncached_req;
wire                      dcache_wb_write;
wire     [15:0]           dcache_wb_byte_enable;
wire     [31:0]           dcache_wb_address;
wire     [127:0]          dcache_wb_cached_rdata;
wire     [127:0]          dcache_wb_write_data;
wire                      dcache_wb_cached_ready;
wire                      dcache_wb_uncached_ready;
wire     [31:0]           icache_wb_address;
wire                      icache_wb_req;
wire     [31:0]           icache_wb_adr; 
wire     [127:0]          icache_wb_read_data; 
wire                      icache_wb_ready;
 
wire                      conflict;
wire                      rn_use_read;
wire                      rm_use_read;
wire                      rs_use_read;
wire                      rd_use_read;
//jing+
wire			  priviledged;
wire      [127:0]         port0_rdata;
 
// data abort has priority
assign decode_fault_status  = dabt_trigger ? dabt_fault_status  : iabt_fault_status;
assign decode_fault_address = dabt_trigger ? dabt_fault_address : iabt_fault_address;
assign decode_fault         = dabt_trigger | iabt_trigger;
 
assign core_stall           = fetch_stall || mem_stall || exec_stall;


// ======================================
//  Fetch Stage
// ======================================
a25_fetch u_fetch (
    .i_clk                              ( i_clk                             ),
    .i_mem_stall                        ( mem_stall                         ),
    .i_exec_stall                       ( exec_stall                        ),
    .i_conflict                         ( conflict                          ),
    .o_fetch_stall                      ( fetch_stall                       ),
    .i_system_rdy                       ( i_system_rdy                      ),

  //  .i_iaddress                         ( {execute_iaddress[31:2], 2'd0}    ),
   .i_iaddress                         ( execute_iaddress   ),
    .i_iaddress_valid                   ( execute_iaddress_valid            ), 
    .i_iaddress_nxt                     ( execute_iaddress_nxt              ),
    .o_fetch_instruction                ( fetch_instruction                 ),
    .i_cache_enable                     ( cache_enable                      ),     
    .i_cache_flush                      ( cache_flush                       ), 
    .i_cacheable_area                   ( cacheable_area                    ),
 
    .o_wb_req                           ( icache_wb_req                     ),
    .o_wb_address                       ( icache_wb_address                 ),
    .i_wb_read_data                     ( icache_wb_read_data               ),
    .i_wb_ready                         ( icache_wb_ready                   )
);


// ======================================
//  Decode Stage
// ======================================
a25_decode u_decode (
    .i_clk                              ( i_clk                             ),
    .i_fetch_instruction                ( fetch_instruction                 ), 
    .i_core_stall                       ( core_stall                        ),    
    .i_irq                              ( i_irq                             ), 
    .i_firq                             ( i_firq                            ),       
    .i_dabt                             ( 1'd0                              ), 
    .i_iabt                             ( 1'd0                              ), 
    .i_adex                             ( adex                              ),                                  
 
    // Instruction fetch or data read signals
  
    .i_execute_iaddress                 ( execute_iaddress                  ),
 //   .i_execute_daddress                 ( execute_daddress                  ),
    .i_abt_status                       ( 8'd0                              ),                                          
    .i_execute_status_bits              ( execute_status_bits               ),                                          
    .i_multiply_done                    ( multiply_done                     ),                                          
 
    .o_imm32                            ( imm32                             ),
    .o_imm_shift_amount                 ( imm_shift_amount                  ),
    .o_shift_imm_zero                   ( shift_imm_zero                    ),
    .o_condition                        ( condition                         ),
    .o_decode_exclusive                 ( decode_exclusive                  ), 
    .o_decode_iaccess                   ( decode_iaccess                    ),
    .o_decode_daccess                   ( decode_daccess                    ),
    .o_status_bits_mode                 ( status_bits_mode                  ),  
    .o_status_bits_irq_mask             ( status_bits_irq_mask              ),  
    .o_status_bits_firq_mask            ( status_bits_firq_mask             ),  

    .o_rm_sel                           ( rm_sel                            ),
    .o_rs_sel                           ( rs_sel                            ),
    .o_load_rd                          ( decode_load_rd                    ),

    .o_rn_sel                           ( rn_sel                            ),
    .o_barrel_shift_amount_sel          ( barrel_shift_amount_sel           ),
    .o_barrel_shift_data_sel            ( barrel_shift_data_sel             ),
    .o_barrel_shift_function            ( barrel_shift_function             ),
    .o_alu_function                     ( alu_function                      ),
    .o_multiply_function                ( multiply_function                 ),
    .o_interrupt_vector_sel             ( interrupt_vector_sel              ),
    .o_iaddress_sel                     ( iaddress_sel                      ),
    .o_daddress_sel                     ( daddress_sel                      ),
    .o_pc_sel                           ( pc_sel                            ),
    .o_byte_enable_sel                  ( byte_enable_sel                   ),
    .o_status_bits_sel                  ( status_bits_sel                   ),
    .o_reg_write_sel                    ( reg_write_sel                     ),
    .o_user_mode_regs_store_nxt         ( user_mode_regs_store_nxt          ),
    .o_firq_not_user_mode               ( firq_not_user_mode                ),
    .o_write_data_wen                   ( write_data_wen                    ),
    .o_base_address_wen                 ( base_address_wen                  ),
    .o_pc_wen                           ( pc_wen                            ),
    .o_reg_bank_wen                     ( reg_bank_wen                      ),
    .o_status_bits_flags_wen            ( status_bits_flags_wen             ),
    .o_status_bits_mode_wen             ( status_bits_mode_wen              ),
    .o_status_bits_irq_mask_wen         ( status_bits_irq_mask_wen          ),
    .o_status_bits_firq_mask_wen        ( status_bits_firq_mask_wen         ),
 
    .o_copro_opcode1                    ( copro_opcode1                     ),                                        
    .o_copro_opcode2                    ( copro_opcode2                     ),                                        
    .o_copro_crn                        ( copro_crn                         ),                                        
    .o_copro_crm                        ( copro_crm                         ),                                        
    .o_copro_num                        ( copro_num                         ),                                        
    .o_copro_operation                  ( copro_operation                   ), 
    .o_copro_write_data_wen             ( copro_write_data_wen              ),                                        
 
    .o_iabt_trigger                     ( iabt_trigger                      ),
    .o_iabt_address                     ( iabt_fault_address                ),
    .o_iabt_status                      ( iabt_fault_status                 ),
    .o_dabt_trigger                     ( dabt_trigger                      ),
    .o_dabt_address                     ( dabt_fault_address                ),
    .o_dabt_status                      ( dabt_fault_status                 ),
 
    .o_conflict                         ( conflict                          ),
    .o_rn_use_read                      ( rn_use_read                       ),
    .o_rm_use_read                      ( rm_use_read                       ),
    .o_rs_use_read                      ( rs_use_read                       ),
    .o_rd_use_read                      ( rd_use_read                       )
);
 
// ======================================
//  Execute Stage
// ======================================
a25_execute u_execute (
    .i_clk                              ( i_clk                             ),
    .i_core_stall                       ( core_stall                        ),   
    .i_mem_stall                        ( mem_stall                         ),   
    .o_exec_stall                       ( exec_stall                        ),
 
    .i_wb_read_data                     ( wb_read_data                      ),      
    .i_wb_read_data_valid               ( wb_read_data_valid                ),
    .i_wb_load_rd                       ( wb_load_rd                        ),
 
    .i_copro_read_data                  ( copro_read_data                   ),
    .i_decode_iaccess                   ( decode_iaccess                    ),
    .i_decode_daccess                   ( decode_daccess                    ),
    .i_decode_load_rd                   ( decode_load_rd                    ),   
    .o_copro_write_data                 ( copro_write_data                  ),
    .o_write_data                       ( write_data                        ),
    .o_iaddress                         ( execute_iaddress                  ),
    .o_iaddress_nxt                     ( execute_iaddress_nxt              ),
    .o_iaddress_valid                   ( execute_iaddress_valid            ),
    .o_daddress                         ( execute_daddress                  ),
    .o_daddress_nxt                     ( execute_daddress_nxt              ),
    .o_daddress_valid                   ( execute_daddress_valid            ),

    .o_adex                             ( adex                              ),
    .o_priviledged                      ( priviledged                       ),
    .o_exclusive                        ( exclusive                         ),
    .o_write_enable                     ( write_enable                      ),
    .o_byte_enable                      ( byte_enable                       ),
    .o_exec_load_rd                     ( exec_load_rd                      ),   
    .o_status_bits                      ( execute_status_bits               ),
    .o_multiply_done                    ( multiply_done                     ),

    .i_status_bits_mode                 ( status_bits_mode                  ),   
    .i_status_bits_irq_mask             ( status_bits_irq_mask              ),   
    .i_status_bits_firq_mask            ( status_bits_firq_mask             ),   
    .i_imm32                            ( imm32                             ),   
    .i_imm_shift_amount                 ( imm_shift_amount                  ),   
    .i_shift_imm_zero                   ( shift_imm_zero                    ),   
    .i_condition                        ( condition                         ),   
    .i_decode_exclusive                 ( decode_exclusive                  ),   

    .i_rm_sel                           ( rm_sel                            ),   
    .i_rs_sel                           ( rs_sel                            ),   

    .i_rn_sel                           ( rn_sel                            ),   
    .i_barrel_shift_amount_sel          ( barrel_shift_amount_sel           ),   
    .i_barrel_shift_data_sel            ( barrel_shift_data_sel             ),   
    .i_barrel_shift_function            ( barrel_shift_function             ),   
    .i_alu_function                     ( alu_function                      ),   
    .i_multiply_function                ( multiply_function                 ),   
    .i_interrupt_vector_sel             ( interrupt_vector_sel              ),   
    .i_iaddress_sel                     ( iaddress_sel                      ),   
    .i_daddress_sel                     ( daddress_sel                      ),   
    .i_pc_sel                           ( pc_sel                            ),   
    .i_byte_enable_sel                  ( byte_enable_sel                   ),   
    .i_status_bits_sel                  ( status_bits_sel                   ),   
    .i_reg_write_sel                    ( reg_write_sel                     ),   
    .i_user_mode_regs_store_nxt         ( user_mode_regs_store_nxt          ),   
    .i_firq_not_user_mode               ( firq_not_user_mode                ),   
    .i_write_data_wen                   ( write_data_wen                    ),   
    .i_base_address_wen                 ( base_address_wen                  ),   
    .i_pc_wen                           ( pc_wen                            ),   
    .i_reg_bank_wen                     ( reg_bank_wen                      ),   
    .i_status_bits_flags_wen            ( status_bits_flags_wen             ),   
    .i_status_bits_mode_wen             ( status_bits_mode_wen              ),   
    .i_status_bits_irq_mask_wen         ( status_bits_irq_mask_wen          ),   
    .i_status_bits_firq_mask_wen        ( status_bits_firq_mask_wen         ),   
    .i_copro_write_data_wen             ( copro_write_data_wen              ),
    .i_conflict                         ( conflict                          ),
    .i_rn_use_read                      ( rn_use_read                       ),
    .i_rm_use_read                      ( rm_use_read                       ),
    .i_rs_use_read                      ( rs_use_read                       ),
    .i_rd_use_read                      ( rd_use_read                       )
);


 
// ======================================
//  Memory access stage with data cache
// ======================================
a25_mem u_mem (
    .i_clk                              ( i_clk                             ),
    .i_fetch_stall                      ( fetch_stall                       ),
    .i_exec_stall                       ( exec_stall                        ),
    .o_mem_stall                        ( mem_stall                         ),
 
    .i_daddress                         ( execute_daddress                  ),
    .i_daddress_valid                   ( execute_daddress_valid            ), 
    .i_daddress_nxt                     ( execute_daddress_nxt              ),
    .i_write_data                       ( write_data                        ),
    .i_write_enable                     ( write_enable                      ),
    .i_exclusive                        ( exclusive                         ),
    .i_byte_enable                      ( byte_enable                       ),
    .i_exec_load_rd                     ( exec_load_rd                      ),  
    .i_cache_enable                     ( cache_enable                      ),     
    .i_cache_flush                      ( cache_flush                       ), 
    .i_cacheable_area                   ( cacheable_area                    ), 
 
    .o_mem_read_data                    ( mem_read_data                     ),
    .o_mem_read_data_valid              ( mem_read_data_valid               ),
    .o_mem_load_rd                      ( mem_load_rd                       ),
 
    .o_wb_cached_req                    ( dcache_wb_cached_req              ),
    .o_wb_uncached_req                  ( dcache_wb_uncached_req            ),
    .o_wb_write                         ( dcache_wb_write                   ),
    .o_wb_byte_enable                   ( dcache_wb_byte_enable             ),
    .o_wb_write_data                    ( dcache_wb_write_data              ),
    .o_wb_address                       ( dcache_wb_address                 ),     
    .i_wb_uncached_rdata                ( dcache_wb_cached_rdata            ), 
    .i_wb_cached_rdata                  ( dcache_wb_cached_rdata            ),
    .i_wb_cached_ready                  ( dcache_wb_cached_ready            ),
    .i_wb_uncached_ready                ( dcache_wb_uncached_ready          )

); 

// ======================================
//  Write back stage with data cache
// ======================================
a25_write_back u_write_back (
    .i_clk                              ( i_clk                             ),
    .i_mem_stall                        ( mem_stall                         ),

    .i_mem_read_data                    ( mem_read_data                     ),      
    .i_mem_read_data_valid              ( mem_read_data_valid               ),
    .i_mem_load_rd                      ( mem_load_rd                       ),
 
    .o_wb_read_data                     ( wb_read_data                      ),      
    .o_wb_read_data_valid               ( wb_read_data_valid                ),
    .o_wb_load_rd                       ( wb_load_rd                        ),
    .i_daddress                         ( execute_daddress                  )
//    .i_daddress_valid                   ( execute_daddress_valid            )
);


// ======================================
//  Wishbone Master I/F
// ======================================
a25_wishbone u_wishbone (
    // CPU Side
    .i_clk                              ( i_clk                             ),
 
    // Port 0 - dcache uncached
    .i_port0_req                        ( dcache_wb_uncached_req            ),
    .o_port0_ack                        ( dcache_wb_uncached_ready          ),
    .i_port0_write                      ( dcache_wb_write                   ),
    .i_port0_wdata                      ( dcache_wb_write_data              ),
    .i_port0_be                         ( dcache_wb_byte_enable             ),
    .i_port0_addr                       ( dcache_wb_address                 ),
    .o_port0_rdata                      ( port0_rdata                       ),     //output      [127:0]         o_port0_rdata
 
    // Port 1 - dcache cached
    .i_port1_req                        ( dcache_wb_cached_req              ),
    .o_port1_ack                        ( dcache_wb_cached_ready            ),
    .i_port1_write                      ( dcache_wb_write                   ),
    .i_port1_wdata                      ( dcache_wb_write_data              ),
    .i_port1_be                         ( dcache_wb_byte_enable             ),
    .i_port1_addr                       ( dcache_wb_address                 ),
    .o_port1_rdata                      ( dcache_wb_cached_rdata            ),
 
    // Port 2 - instruction cache accesses, read only
    .i_port2_req                        ( icache_wb_req                     ),
    .o_port2_ack                        ( icache_wb_ready                   ),
    .i_port2_write                      ( 1'd0                              ),
    .i_port2_wdata                      ( 128'd0                            ),
    .i_port2_be                         ( 16'd0                             ),
    .i_port2_addr                       ( icache_wb_address                 ),
    .o_port2_rdata                      ( icache_wb_read_data               ),
 
    // Wishbone
    .o_wb_adr                           ( o_wb_adr                          ),
    .o_wb_sel                           ( o_wb_sel                          ),
    .o_wb_we                            ( o_wb_we                           ),
    .o_wb_dat                           ( o_wb_dat                          ),
    .o_wb_cyc                           ( o_wb_cyc                          ),
    .o_wb_stb                           ( o_wb_stb                          ),
    .i_wb_dat                           ( i_wb_dat                          ),
    .i_wb_ack                           ( i_wb_ack                          )
//    .i_wb_err                           ( i_wb_err                          )
);

// ======================================
//  Co-Processor #15
// ======================================
a25_coprocessor u_coprocessor (
    .i_clk                              ( i_clk                             ),
    .i_core_stall                       ( core_stall                        ),
 
//    .i_copro_opcode1                    ( copro_opcode1                     ),
//    .i_copro_opcode2                    ( copro_opcode2                     ),
    .i_copro_crn                        ( copro_crn                         ),    
//    .i_copro_crm                        ( copro_crm                         ),
//    .i_copro_num                        ( copro_num                         ),
    .i_copro_operation                  ( copro_operation                   ),
    .i_copro_write_data                 ( copro_write_data                  ),
 
    .i_fault                            ( decode_fault                      ),
    .i_fault_status                     ( decode_fault_status               ),
    .i_fault_address                    ( decode_fault_address              ), 
 
    .o_copro_read_data                  ( copro_read_data                   ),
    .o_cache_enable                     ( cache_enable                      ),
    .o_cache_flush                      ( cache_flush                       ),
    .o_cacheable_area                   ( cacheable_area                    )
);
 

endmodule

