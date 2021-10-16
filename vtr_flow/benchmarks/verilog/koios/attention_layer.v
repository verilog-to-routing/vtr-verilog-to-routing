//////////////////////////////////////////////////////////////////////////////
// Author: Aishwarya Rajen
//////////////////////////////////////////////////////////////////////////////

//`define SIMULATION_MEMORY
//`define SIMULATION_addfp

`define VECTOR_DEPTH 64 //Q,K,V vector size
`define DATA_WIDTH 16
`define VECTOR_BITS 1024 // 16 bit each (16x64)
`define NUM_WORDS 32   //num of words in the sentence
`define BUF_AWIDTH 4 //16 entries in each buffer ram
`define BUF_LOC_SIZE 4 //4 words in each addr location
`define OUT_RAM_DEPTH 512 //512 entries in output bram

/////////////////////////////////////////////////////////////////////////////
//Self-Attention Layer
/////////////////////////////////////////////////////////////////////////////
//Fig 2 (left) of the "Attention is all you need" paper.
//Design of the self-attention layer module which is useful 
//in the Transformer deep learning architecture.
//Fixed Point 16(4.12) format is used.
//This design takes in the Query Vector Q,Key Vector K and
//Value vector V dervied from the word embeddings as input.
//The Q,K,V matrices are stored and accessed through Dual Port RAMs
//This is a pipelined design comprising of matrix vector multiplcation,
//scaling, Softmax layer and a final matrix vector multiplication stage.
//Double buffering has been used at the input and output of the softmax 
//inorder to hide the latency of the softmax design.
//The final outputs also get stored in a DPRAM which is interfaced through I/O.
//////////////////////////////////////////////////////////////////////////////

`ifdef BFLOAT16
`define EXPONENT 8
`define MANTISSA 7
`else // for ieee half precision fp16
`define EXPONENT 5
`define MANTISSA 10
`endif

`define SIGN 1
`define DWIDTH (`SIGN+`EXPONENT+`MANTISSA)

module attention(
input clk,
input rst,
input start,			
input [4:0] q_rd_addr, //start address of q
input [4:0] k_rd_addr, //start address of k
input [4:0] v_rd_addr, //start address of v
input [2:0] wren_qkv_ext, //To write data into Q,K,V BRAMs externally
input [5:0] address_ext,  //External write address
input [`VECTOR_BITS-1:0] data_ext,  //Data to be written
input [`OUT_RAM_DEPTH-1:0] out_rd_addr,	//To read stored outputs from outside
output [`DATA_WIDTH-1:0] out   //16 bit output from out bram
);


reg q_en,k_en,v_en;
reg [4:0] q_addr,k_addr;
reg [5:0] v_addr;
wire [`VECTOR_BITS-1:0] q,k;
wire [(`NUM_WORDS*`DATA_WIDTH)-1:0] v;

//Dummy input/ouputs connected to DPRAM to ensure VTR doesn't optimize the BRAM out.
reg [`VECTOR_BITS-1:0] dummyin_q,dummyin_k;
reg [(`NUM_WORDS*`DATA_WIDTH)-1:0] dummyin_v;
wire [`VECTOR_BITS-1:0] dummyout_q,dummyout_k;
wire [(`NUM_WORDS*`DATA_WIDTH)-1:0] dummyout_v;

reg [4:0] count;
wire [`VECTOR_BITS-1:0] mul_out;
wire [`DATA_WIDTH-1:0] qiki, scaled_qiki;
reg flag;
wire [`NUM_WORDS*`DATA_WIDTH-1:0] mul_out2;
wire [`DATA_WIDTH-1:0] softmulv;

//Input/output connections of the buffers
reg [`BUF_AWIDTH-1:0] wr_addr_12;
reg wr_addr_34;
wire [`BUF_AWIDTH-1:0] rd_addr_12;
reg rd_addr_34;
reg [`BUF_LOC_SIZE*`DATA_WIDTH-1:0] wr_data_12;
reg [(8*`BUF_LOC_SIZE*`DATA_WIDTH)-1:0]wr_data_34;
reg [`BUF_LOC_SIZE-1:0] word_we0_12,word_we1_12;
reg [`NUM_WORDS-1:0] word_we0_34,word_we1_34;
wire [`BUF_LOC_SIZE*`DATA_WIDTH-1:0] data_to_softmax;
wire [`NUM_WORDS*`DATA_WIDTH-1:0] data_to_MVM;
reg  [`BUF_LOC_SIZE*`DATA_WIDTH-1:0] dummyin_buf12;
reg [`NUM_WORDS*`DATA_WIDTH-1:0] dummyin_buf34;
wire [`BUF_LOC_SIZE*`DATA_WIDTH-1:0] dummyout_buf12;
wire [`NUM_WORDS*`DATA_WIDTH-1:0] dummyout_buf34;
reg [1:0] word_count;

//softmax
wire [`DATA_WIDTH-1:0] soft_out0,soft_out1,soft_out2,soft_out3;
reg soft_init,soft_start;
wire soft_done;
wire [`BUF_AWIDTH-1:0] soft_strt_addr,soft_end_addr,soft_rd_addr,soft_sub0_addr,soft_sub1_addr;
reg choose_buf;
reg buff_done;

reg [4:0] soft_word_count;
wire [8*`BUF_LOC_SIZE*`DATA_WIDTH-1:0] comb_softout;
reg vector_complete;
reg strt_softmulv;
reg [5:0] v_count,v_count_ff;

reg [8:0] out_wr_addr;
reg [8:0] addr_a,addr_b;
reg wren_a,wren_b;
reg [`DATA_WIDTH-1:0] dummy_b;
wire [`DATA_WIDTH-1:0] dummy_a;
reg strt_out_write;
reg soft_out_strt, soft_out_end;
reg first_time,mvm_complete;

//BRAMs that hold Q,K,V matrices
dpram Q(.clk(clk),.address_a(q_addr),.address_b(address_ext),.wren_a(q_en),.wren_b(wren_qkv_ext[0]),.data_a(dummyin_q),.data_b(data_ext),.out_a(q),.out_b(dummyout_q));
dpram K(.clk(clk),.address_a(k_addr),.address_b(address_ext),.wren_a(k_en),.wren_b(wren_qkv_ext[0]),.data_a(dummyin_k),.data_b(data_ext),.out_a(k),.out_b(dummyout_k));
dpram_t V(.clk(clk),.address_a(v_addr),.address_b(address_ext),.wren_a(v_en),.wren_b(wren_qkv_ext[0]),.data_a(dummyin_v),.data_b(data_ext),.out_a(v),.out_b(dummyout_v));

//Multiplying Q and K vector
vecmat_mul qk_mul (.clk(clk),.reset(rst),.vector(q),.matrix(k),.tmp(mul_out));
vecmat_add qk_acc (.clk(clk),.reset(rst),.mulout(mul_out),.data_out(qiki));

//Scaling (dividing by 8)
divideby8 scale(.data(qiki),.out(scaled_qiki));

//Buffers 1 and 2 at the input to softmax block (2 buffers are present in a single bram)
//Each location stores 4 words together to feed to softmax
wordwise_bram in_buffer12
(       .addr0(wr_addr_12), 
        .d0(wr_data_12), 
        .we0(word_we0_12), 
        .q0(dummyout_buf12),  
        .addr1(rd_addr_12),   //from softmax
        .d1(dummyin_buf12),
        .we1(word_we1_12),
        .q1(data_to_softmax),  //to softmax
        .clk(clk)
);

//Buffers 3 and 4 at the output of softmax block
//Each location stores 32 words to make it easy to feed to MVM
wordwise_bram_2 out_buffer34
(       .addr0(wr_addr_34), 
        .d0(wr_data_34), 
        .we0(word_we0_34), 
        .q0(dummyout_buf34),  
        .addr1(rd_addr_34),
        .d1(dummyin_buf34),
        .we1(word_we1_34),
        .q1(data_to_MVM),   //to MVM
        .clk(clk)
);

//Softmax layer has a parallelism of 4
softmax soft(
	.inp(data_to_softmax),
	.sub0_inp(data_to_softmax),
	.sub1_inp(data_to_softmax),
	.start_addr(soft_strt_addr),
	.end_addr(soft_end_addr),
	.addr(soft_rd_addr),
	.sub0_inp_addr(soft_sub0_addr),
	.sub1_inp_addr(soft_sub1_addr),
	.outp0(soft_out0),
	.outp1(soft_out1),
	.outp2(soft_out2),
	.outp3(soft_out3),
	.init(soft_init),
	.start(soft_start),
	.done(soft_done),
	.clk(clk),
	.reset(rst)
//	.addr_sel(addr_select)
);

//Multiplying output of softmax with V vector
vecmat_mul_32 rv_mul (.clk(clk),.reset(rst),.vector(data_to_MVM),.matrix(v),.tmp(mul_out2));
vecmat_add_32 rv_acc (.clk(clk),.reset(rst),.mulout(mul_out2),.data_out(softmulv));


//output BRAM can store upto 512 elements of `DATA_WIDTH
dpram_small out_ram (.clk(clk),.address_a(out_wr_addr),.address_b(out_rd_addr),.wren_a(wren_a),.wren_b(wren_b),.data_a(softmulv),.data_b(dummy_b),.out_a(dummy_a),.out_b(out));

assign rd_addr_12 = soft_rd_addr|soft_sub0_addr|soft_sub1_addr ;
//assign rd_addr_12 = (addr_select)?soft_rd_addr:soft_sub0_addr|soft_sub1_addr ;



assign soft_strt_addr = (~choose_buf)?4'd0:4'd8;
assign soft_end_addr = (~choose_buf)?4'd8:4'd16;



always @(posedge clk) begin

	if (rst) begin
		q_addr <=0;
		k_addr <=0;
		q_en <=0;
		k_en <=0;
		count <=0;
 	   dummyin_q <= 0; 
	   dummyin_k <=0;
	   dummyin_v <= 0;
	   word_we0_12<=1;
	   dummyin_buf12 <= 0;
	   word_we1_12 <= 0; //softmax always reads from buff 1&2
	   wr_addr_12 <= 0;
	   word_count <= 0;
	   choose_buf <= 1;
	   first_time <=1;
	   soft_init <=0;
    end
	else begin
		if (start ==1) begin
			q_addr <= q_rd_addr;
			k_addr <= k_rd_addr;
			count <= count+1;	
		end
		else begin
			//Reading data from the K,Q BRAMs to feed to MVM 
			//K vector increments every cycle and Q increments after multiplication with the whole set of K vectors is complete
			k_addr <= k_addr+1;
			if(count==0) 
				q_addr <= q_addr+1;
			if(count>4) 
				flag <= 1; //to ensure initially there is a 4 cycle pipeline stage delay
			
			//Writing scaled output to the buffers
			if(flag == 1) begin
				if(word_count==0) 
					word_we0_12 <= 4'b0001;
				else 
					word_we0_12 <= word_we0_12 <<1;  //creating write enable for diff words in same addr loc.
			
				if(word_we0_12 == 4'b1000)
					wr_addr_12 <= wr_addr_12+1;    //addr gets incremented every 4 words

						
				word_count <= word_count+1;	

				if(count==4)
					  buff_done <= 1;	//indicates one of the buffers being full
				else if(soft_init|soft_start == 1) 
					  buff_done <= 0;   //shows buff data has been sent to softmax



			    if(buff_done == 1) begin //start softmax after one buffer is full (32 words in in_buffer12)
				//start softmax when buff is done for the first time
				//& start softmax only after it finishes outputing previous values
					if(first_time==1 || soft_out_end==1) begin  
						first_time <= 0;
    						soft_init <= 1;
    						choose_buf <= ~choose_buf;  //to alternate between the two buffers 1 & 2 within one ram
					end
    			   end
    			   else if(soft_init==1) begin
    				soft_init <= 0;
    				soft_start <= 1;
    			   end
    			   else
    				soft_start <=0;
  			
            end
	
			count <= count+1;
			wr_data_12 <= scaled_qiki<<(`DATA_WIDTH*(word_count));   //word data sent to softmax

			
		end
			
	end
end


always @(posedge clk) begin

	//SOFTMAX control
	if(rst) begin
	   soft_out_strt <= 0;
	   soft_out_end  <= 0;
	 end
	else begin
				if(soft_done==1) begin
					soft_out_strt <=1;
					soft_out_end <= 0;
				end
				else begin
					if(soft_out_strt==1)  begin
						soft_out_end <=1;
						soft_out_strt <=0;
					end
					else if(soft_start==1)  //indicates that the next set of inputs have been given to softmax
						soft_out_end <= 0;
				end
	end
end

//concatenate the 4 outputs from softmax
assign comb_softout = {{448{1'b0}},soft_out3,soft_out2,soft_out1,soft_out0}; 

//Control logic for the output side of softmax
always@(posedge clk) begin
	if(rst) begin
		word_we0_34 <= 32'h0000000f;
		wr_addr_34 <= 0;
		wr_data_34 <= 0;
	    dummyin_buf34 <=0;
		soft_word_count <=0;
		rd_addr_34 <= 0;  
		word_we1_34 <= 0;	
		strt_softmulv <= 0;
		v_en <= 0;
		v_addr <= 0;
		v_count <= 0;	
		out_wr_addr <= 0;
		wren_a <= 1;
		wren_b <= 0;
		dummy_b <=0;
	end
	else begin
		if(start==1)
			v_addr <= v_rd_addr;

		//storing output data from softmax on to buffers 3 & 4
		if(soft_done == 1) begin
			if(soft_word_count==0)
				word_we0_34 <= 32'h0000000f;  //write all words of a loc simultaneously
			else
				word_we0_34 <= word_we0_34<<4;
 
			wr_data_34 <= comb_softout<<(`DATA_WIDTH*soft_word_count);
			soft_word_count <= soft_word_count+4; //every cycle we write 4 words together
		end
		else begin
			word_we0_34 <= 0;
			if(word_we0_34==32'hf0000000) begin

				wr_addr_34 <= wr_addr_34+1;
				vector_complete <=1;	 //indicates atleast one buffer has data
			end
		end
		//MVM with V matrix control logic (1x32 * 32x64)	
		if(vector_complete==1) begin 
			v_addr <= v_addr+1;
			v_count <= v_count+1;
		end
		
		if(v_count==63) begin
				rd_addr_34 <= ~rd_addr_34;  //change addr to read data to MVM block
				mvm_complete <= 1;			//pulse indicates completion on one mvm multiplication of soft vector and V
		end
		else
			mvm_complete <= 0;

		if(v_count==4)   //output write 4 stage pipeline delay for MVM
			strt_out_write <=1;

		if(strt_out_write==1)
			out_wr_addr <= out_wr_addr+1;

	end			
end

endmodule 	


//Scaling by arithmetic right shifting	
module divideby8
(
  input [`DATA_WIDTH-1:0] data,
  output reg [`DATA_WIDTH-1:0] out
 );

 reg [`DATA_WIDTH-1:0] mask; 

 always@(*) begin
	 if (data[`DATA_WIDTH-1] == 1) 
		mask = 16'b1110000000000000;
	 else
		mask = 0;
	
	 out = mask|(data>>3);
  end
 
endmodule



module vecmat_mul #( parameter arraysize=1024,parameter vectdepth=64)  
(
 input clk,
 input reset,
 input [arraysize-1:0] vector,  //q vector
 input [arraysize-1:0] matrix,  //vector from K matrix 
 output [arraysize-1:0] tmp
 );

	signedmul mult_u0(.clk(clk),.a(vector[0*16+:16]),.b(matrix[0*16+:16]),.c(tmp[0*16+:16]));
	signedmul mult_u1(.clk(clk),.a(vector[1*16+:16]),.b(matrix[1*16+:16]),.c(tmp[1*16+:16]));
	signedmul mult_u2(.clk(clk),.a(vector[2*16+:16]),.b(matrix[2*16+:16]),.c(tmp[2*16+:16]));
	signedmul mult_u3(.clk(clk),.a(vector[3*16+:16]),.b(matrix[3*16+:16]),.c(tmp[3*16+:16]));
	signedmul mult_u4(.clk(clk),.a(vector[4*16+:16]),.b(matrix[4*16+:16]),.c(tmp[4*16+:16]));
	signedmul mult_u5(.clk(clk),.a(vector[5*16+:16]),.b(matrix[5*16+:16]),.c(tmp[5*16+:16]));
	signedmul mult_u6(.clk(clk),.a(vector[6*16+:16]),.b(matrix[6*16+:16]),.c(tmp[6*16+:16]));
	signedmul mult_u7(.clk(clk),.a(vector[7*16+:16]),.b(matrix[7*16+:16]),.c(tmp[7*16+:16]));
	signedmul mult_u8(.clk(clk),.a(vector[8*16+:16]),.b(matrix[8*16+:16]),.c(tmp[8*16+:16]));
	signedmul mult_u9(.clk(clk),.a(vector[9*16+:16]),.b(matrix[9*16+:16]),.c(tmp[9*16+:16]));
	signedmul mult_u10(.clk(clk),.a(vector[10*16+:16]),.b(matrix[10*16+:16]),.c(tmp[10*16+:16]));
	signedmul mult_u11(.clk(clk),.a(vector[11*16+:16]),.b(matrix[11*16+:16]),.c(tmp[11*16+:16]));
	signedmul mult_u12(.clk(clk),.a(vector[12*16+:16]),.b(matrix[12*16+:16]),.c(tmp[12*16+:16]));
	signedmul mult_u13(.clk(clk),.a(vector[13*16+:16]),.b(matrix[13*16+:16]),.c(tmp[13*16+:16]));
	signedmul mult_u14(.clk(clk),.a(vector[14*16+:16]),.b(matrix[14*16+:16]),.c(tmp[14*16+:16]));
	signedmul mult_u15(.clk(clk),.a(vector[15*16+:16]),.b(matrix[15*16+:16]),.c(tmp[15*16+:16]));
	signedmul mult_u16(.clk(clk),.a(vector[16*16+:16]),.b(matrix[16*16+:16]),.c(tmp[16*16+:16]));
	signedmul mult_u17(.clk(clk),.a(vector[17*16+:16]),.b(matrix[17*16+:16]),.c(tmp[17*16+:16]));
	signedmul mult_u18(.clk(clk),.a(vector[18*16+:16]),.b(matrix[18*16+:16]),.c(tmp[18*16+:16]));
	signedmul mult_u19(.clk(clk),.a(vector[19*16+:16]),.b(matrix[19*16+:16]),.c(tmp[19*16+:16]));
	signedmul mult_u20(.clk(clk),.a(vector[20*16+:16]),.b(matrix[20*16+:16]),.c(tmp[20*16+:16]));
	signedmul mult_u21(.clk(clk),.a(vector[21*16+:16]),.b(matrix[21*16+:16]),.c(tmp[21*16+:16]));
	signedmul mult_u22(.clk(clk),.a(vector[22*16+:16]),.b(matrix[22*16+:16]),.c(tmp[22*16+:16]));
	signedmul mult_u23(.clk(clk),.a(vector[23*16+:16]),.b(matrix[23*16+:16]),.c(tmp[23*16+:16]));
	signedmul mult_u24(.clk(clk),.a(vector[24*16+:16]),.b(matrix[24*16+:16]),.c(tmp[24*16+:16]));
	signedmul mult_u25(.clk(clk),.a(vector[25*16+:16]),.b(matrix[25*16+:16]),.c(tmp[25*16+:16]));
	signedmul mult_u26(.clk(clk),.a(vector[26*16+:16]),.b(matrix[26*16+:16]),.c(tmp[26*16+:16]));
	signedmul mult_u27(.clk(clk),.a(vector[27*16+:16]),.b(matrix[27*16+:16]),.c(tmp[27*16+:16]));
	signedmul mult_u28(.clk(clk),.a(vector[28*16+:16]),.b(matrix[28*16+:16]),.c(tmp[28*16+:16]));
	signedmul mult_u29(.clk(clk),.a(vector[29*16+:16]),.b(matrix[29*16+:16]),.c(tmp[29*16+:16]));
	signedmul mult_u30(.clk(clk),.a(vector[30*16+:16]),.b(matrix[30*16+:16]),.c(tmp[30*16+:16]));
	signedmul mult_u31(.clk(clk),.a(vector[31*16+:16]),.b(matrix[31*16+:16]),.c(tmp[31*16+:16]));
	signedmul mult_u32(.clk(clk),.a(vector[32*16+:16]),.b(matrix[32*16+:16]),.c(tmp[32*16+:16]));
	signedmul mult_u33(.clk(clk),.a(vector[33*16+:16]),.b(matrix[33*16+:16]),.c(tmp[33*16+:16]));
	signedmul mult_u34(.clk(clk),.a(vector[34*16+:16]),.b(matrix[34*16+:16]),.c(tmp[34*16+:16]));
	signedmul mult_u35(.clk(clk),.a(vector[35*16+:16]),.b(matrix[35*16+:16]),.c(tmp[35*16+:16]));
	signedmul mult_u36(.clk(clk),.a(vector[36*16+:16]),.b(matrix[36*16+:16]),.c(tmp[36*16+:16]));
	signedmul mult_u37(.clk(clk),.a(vector[37*16+:16]),.b(matrix[37*16+:16]),.c(tmp[37*16+:16]));
	signedmul mult_u38(.clk(clk),.a(vector[38*16+:16]),.b(matrix[38*16+:16]),.c(tmp[38*16+:16]));
	signedmul mult_u39(.clk(clk),.a(vector[39*16+:16]),.b(matrix[39*16+:16]),.c(tmp[39*16+:16]));
	signedmul mult_u40(.clk(clk),.a(vector[40*16+:16]),.b(matrix[40*16+:16]),.c(tmp[40*16+:16]));
	signedmul mult_u41(.clk(clk),.a(vector[41*16+:16]),.b(matrix[41*16+:16]),.c(tmp[41*16+:16]));
	signedmul mult_u42(.clk(clk),.a(vector[42*16+:16]),.b(matrix[42*16+:16]),.c(tmp[42*16+:16]));
	signedmul mult_u43(.clk(clk),.a(vector[43*16+:16]),.b(matrix[43*16+:16]),.c(tmp[43*16+:16]));
	signedmul mult_u44(.clk(clk),.a(vector[44*16+:16]),.b(matrix[44*16+:16]),.c(tmp[44*16+:16]));
	signedmul mult_u45(.clk(clk),.a(vector[45*16+:16]),.b(matrix[45*16+:16]),.c(tmp[45*16+:16]));
	signedmul mult_u46(.clk(clk),.a(vector[46*16+:16]),.b(matrix[46*16+:16]),.c(tmp[46*16+:16]));
	signedmul mult_u47(.clk(clk),.a(vector[47*16+:16]),.b(matrix[47*16+:16]),.c(tmp[47*16+:16]));
	signedmul mult_u48(.clk(clk),.a(vector[48*16+:16]),.b(matrix[48*16+:16]),.c(tmp[48*16+:16]));
	signedmul mult_u49(.clk(clk),.a(vector[49*16+:16]),.b(matrix[49*16+:16]),.c(tmp[49*16+:16]));
	signedmul mult_u50(.clk(clk),.a(vector[50*16+:16]),.b(matrix[50*16+:16]),.c(tmp[50*16+:16]));
	signedmul mult_u51(.clk(clk),.a(vector[51*16+:16]),.b(matrix[51*16+:16]),.c(tmp[51*16+:16]));
	signedmul mult_u52(.clk(clk),.a(vector[52*16+:16]),.b(matrix[52*16+:16]),.c(tmp[52*16+:16]));
	signedmul mult_u53(.clk(clk),.a(vector[53*16+:16]),.b(matrix[53*16+:16]),.c(tmp[53*16+:16]));
	signedmul mult_u54(.clk(clk),.a(vector[54*16+:16]),.b(matrix[54*16+:16]),.c(tmp[54*16+:16]));
	signedmul mult_u55(.clk(clk),.a(vector[55*16+:16]),.b(matrix[55*16+:16]),.c(tmp[55*16+:16]));
	signedmul mult_u56(.clk(clk),.a(vector[56*16+:16]),.b(matrix[56*16+:16]),.c(tmp[56*16+:16]));
	signedmul mult_u57(.clk(clk),.a(vector[57*16+:16]),.b(matrix[57*16+:16]),.c(tmp[57*16+:16]));
	signedmul mult_u58(.clk(clk),.a(vector[58*16+:16]),.b(matrix[58*16+:16]),.c(tmp[58*16+:16]));
	signedmul mult_u59(.clk(clk),.a(vector[59*16+:16]),.b(matrix[59*16+:16]),.c(tmp[59*16+:16]));
	signedmul mult_u60(.clk(clk),.a(vector[60*16+:16]),.b(matrix[60*16+:16]),.c(tmp[60*16+:16]));
	signedmul mult_u61(.clk(clk),.a(vector[61*16+:16]),.b(matrix[61*16+:16]),.c(tmp[61*16+:16]));
	signedmul mult_u62(.clk(clk),.a(vector[62*16+:16]),.b(matrix[62*16+:16]),.c(tmp[62*16+:16]));
	signedmul mult_u63(.clk(clk),.a(vector[63*16+:16]),.b(matrix[63*16+:16]),.c(tmp[63*16+:16]));
	


endmodule                    

module vecmat_add #(parameter arraysize=1024,parameter vectdepth=64)
 (
 input clk,reset,
 input [arraysize-1:0] mulout,
 output reg [15:0] data_out
 );
           
 wire [15:0] tmp0, tmp1 ,tmp2 ,tmp3 ,tmp4 ,tmp5 ,tmp6 ,tmp7 ,tmp8 ,tmp9 ,tmp10 ,tmp11 ,tmp12 ,tmp13 ,tmp14 ,tmp15 ,tmp16 ,tmp17 ,tmp18 ,tmp19 ,tmp20 ,tmp21 ,tmp22 ,tmp23 ,tmp24 ,tmp25 ,tmp26 ,tmp27 ,tmp28 ,tmp29 ,tmp30 ,tmp31 ,tmp32 ,tmp33 ,tmp34 ,tmp35 ,tmp36 ,tmp37 ,tmp38 ,tmp39 ,tmp40 ,tmp41 ,tmp42 ,tmp43 ,tmp44 ,tmp45 ,tmp46 ,tmp47 ,tmp48 ,tmp49 ,tmp50,tmp51 ,tmp52 ,tmp53,tmp54 ,tmp55 ,tmp56 ,tmp57 ,tmp58, tmp59 ,tmp60 ,tmp61,tmp62; 
 reg[31:0] i;
 reg [15:0] ff1,ff3,ff5,ff7,ff9,ff11,ff13,ff15,ff17,ff19,ff21,ff23,ff25,ff27,ff29,ff31;

 always @(posedge clk) begin
	if(~reset) begin
		data_out <= tmp61;

		//adding a flop pipeline stage
		ff1 <= tmp1;
		ff3 <= tmp3;
		ff5 <= tmp5;
		ff7 <= tmp7;
		ff9 <= tmp9;
		ff11 <= tmp11;
		ff13 <= tmp13;
		ff15 <= tmp15;
		ff17 <= tmp17;
		ff19 <= tmp19;
		ff21 <= tmp21;
		ff23 <= tmp23;
		ff25 <= tmp25;
		ff27 <= tmp27;
		ff29 <= tmp29;
		ff31 <= tmp31;
	end   
 end     
                                                        
        // fixed point addition  
        qadd2 Add_u0(.a(mulout[16*0+:16]),.b(mulout[16*1+:16]),.c(tmp0));
		qadd2 Add_u2(.a(mulout[16*2+:16]),.b(mulout[16*3+:16]),.c(tmp2));
		qadd2 Add_u4(.a(mulout[16*4+:16]),.b(mulout[16*5+:16]),.c(tmp4));
		qadd2 Add_u6(.a(mulout[16*6+:16]),.b(mulout[16*7+:16]),.c(tmp6));
		qadd2 Add_u8(.a(mulout[16*8+:16]),.b(mulout[16*9+:16]),.c(tmp8));
		qadd2 Add_u10(.a(mulout[16*10+:16]),.b(mulout[16*11+:16]),.c(tmp10));
		qadd2 Add_u12(.a(mulout[16*12+:16]),.b(mulout[16*13+:16]),.c(tmp12));
		qadd2 Add_u14(.a(mulout[16*14+:16]),.b(mulout[16*15+:16]),.c(tmp14));
		qadd2 Add_u16(.a(mulout[16*16+:16]),.b(mulout[16*17+:16]),.c(tmp16));
		qadd2 Add_u18(.a(mulout[16*18+:16]),.b(mulout[16*19+:16]),.c(tmp18));
		qadd2 Add_u20(.a(mulout[16*20+:16]),.b(mulout[16*21+:16]),.c(tmp20));
		qadd2 Add_u22(.a(mulout[16*22+:16]),.b(mulout[16*23+:16]),.c(tmp22));
		qadd2 Add_u24(.a(mulout[16*24+:16]),.b(mulout[16*25+:16]),.c(tmp24));
		qadd2 Add_u26(.a(mulout[16*26+:16]),.b(mulout[16*27+:16]),.c(tmp26));
		qadd2 Add_u28(.a(mulout[16*28+:16]),.b(mulout[16*29+:16]),.c(tmp28));
		qadd2 Add_u30(.a(mulout[16*30+:16]),.b(mulout[16*31+:16]),.c(tmp30));
		qadd2 Add_u32(.a(mulout[16*32+:16]),.b(mulout[16*33+:16]),.c(tmp32));
		qadd2 Add_u34(.a(mulout[16*34+:16]),.b(mulout[16*35+:16]),.c(tmp34));
		qadd2 Add_u36(.a(mulout[16*36+:16]),.b(mulout[16*37+:16]),.c(tmp36));
		qadd2 Add_u38(.a(mulout[16*38+:16]),.b(mulout[16*39+:16]),.c(tmp38));
		qadd2 Add_u40(.a(mulout[16*40+:16]),.b(mulout[16*41+:16]),.c(tmp40));
		qadd2 Add_u42(.a(mulout[16*42+:16]),.b(mulout[16*43+:16]),.c(tmp42));
		qadd2 Add_u44(.a(mulout[16*44+:16]),.b(mulout[16*45+:16]),.c(tmp44));
		qadd2 Add_u46(.a(mulout[16*46+:16]),.b(mulout[16*47+:16]),.c(tmp46));
		qadd2 Add_u48(.a(mulout[16*48+:16]),.b(mulout[16*49+:16]),.c(tmp48));
		qadd2 Add_u50(.a(mulout[16*50+:16]),.b(mulout[16*51+:16]),.c(tmp50));
		qadd2 Add_u52(.a(mulout[16*52+:16]),.b(mulout[16*53+:16]),.c(tmp52));
		qadd2 Add_u54(.a(mulout[16*54+:16]),.b(mulout[16*55+:16]),.c(tmp54));
		qadd2 Add_u56(.a(mulout[16*56+:16]),.b(mulout[16*57+:16]),.c(tmp56));
		qadd2 Add_u58(.a(mulout[16*58+:16]),.b(mulout[16*59+:16]),.c(tmp58));
		qadd2 Add_u60(.a(mulout[16*60+:16]),.b(mulout[16*61+:16]),.c(tmp60));
		qadd2 Add_u62(.a(mulout[16*62+:16]),.b(mulout[16*63+:16]),.c(tmp62));
            
			qadd2 Add_u1(.a(tmp0),.b(tmp2),.c(tmp1));
			qadd2 Add_u3(.a(tmp4),.b(tmp6),.c(tmp3));
			qadd2 Add_u5(.a(tmp8),.b(tmp10),.c(tmp5));
			qadd2 Add_u7(.a(tmp12),.b(tmp14),.c(tmp7));
			qadd2 Add_u9(.a(tmp16),.b(tmp18),.c(tmp9));
			qadd2 Add_u11(.a(tmp20),.b(tmp22),.c(tmp11));
			qadd2 Add_u13(.a(tmp24),.b(tmp26),.c(tmp13));
			qadd2 Add_u15(.a(tmp28),.b(tmp30),.c(tmp15));
			qadd2 Add_u17(.a(tmp32),.b(tmp34),.c(tmp17));
			qadd2 Add_u19(.a(tmp36),.b(tmp38),.c(tmp19));
			qadd2 Add_u21(.a(tmp40),.b(tmp42),.c(tmp21));
			qadd2 Add_u23(.a(tmp44),.b(tmp46),.c(tmp23));
			qadd2 Add_u25(.a(tmp48),.b(tmp50),.c(tmp25));
			qadd2 Add_u27(.a(tmp52),.b(tmp54),.c(tmp27));
			qadd2 Add_u29(.a(tmp56),.b(tmp58),.c(tmp29));
			qadd2 Add_u31(.a(tmp60),.b(tmp62),.c(tmp31));

			qadd2 Add_u33(.a(ff1),.b(ff3),.c(tmp33));
			qadd2 Add_u35(.a(ff5),.b(ff7),.c(tmp35));
			qadd2 Add_u37(.a(ff9),.b(ff11),.c(tmp37));
			qadd2 Add_u39(.a(ff13),.b(ff15),.c(tmp39));
			qadd2 Add_u41(.a(ff17),.b(ff19),.c(tmp41));
			qadd2 Add_u43(.a(ff21),.b(ff23),.c(tmp43));
			qadd2 Add_u45(.a(ff25),.b(ff27),.c(tmp45));
			qadd2 Add_u47(.a(ff29),.b(ff31),.c(tmp47));

			qadd2 Add_u49(.a(tmp33),.b(tmp35),.c(tmp49));
			qadd2 Add_u51(.a(tmp37),.b(tmp39),.c(tmp51));
			qadd2 Add_u53(.a(tmp41),.b(tmp43),.c(tmp53));
			qadd2 Add_u55(.a(tmp45),.b(tmp47),.c(tmp55));

			qadd2 Add_u57(.a(tmp49),.b(tmp51),.c(tmp57));
			qadd2 Add_u59(.a(tmp53),.b(tmp55),.c(tmp59));

			qadd2 Add_u61(.a(tmp57),.b(tmp59),.c(tmp61));
			
endmodule

module vecmat_mul_32 #( parameter arraysize=512,parameter vectdepth=32)  //,matsize=64)   // varraysize=1024 vectwidth=64,matsize=4096
(
 input clk,
 input reset,
 input [arraysize-1:0] vector,  //softmax vector
 input [arraysize-1:0] matrix,  //vector from V matrix 
 output [arraysize-1:0] tmp
 );
  
/*always @(posedge clk) begin
	if(~reset) begin

	    vector <= data;
		matrix <= W;
    
	end
 end */
	 

   	signedmul mult_u0(.clk(clk),.a(vector[0*16+:16]),.b(matrix[0*16+:16]),.c(tmp[0*16+:16]));
	signedmul mult_u1(.clk(clk),.a(vector[1*16+:16]),.b(matrix[1*16+:16]),.c(tmp[1*16+:16]));
	signedmul mult_u2(.clk(clk),.a(vector[2*16+:16]),.b(matrix[2*16+:16]),.c(tmp[2*16+:16]));
	signedmul mult_u3(.clk(clk),.a(vector[3*16+:16]),.b(matrix[3*16+:16]),.c(tmp[3*16+:16]));
	signedmul mult_u4(.clk(clk),.a(vector[4*16+:16]),.b(matrix[4*16+:16]),.c(tmp[4*16+:16]));
	signedmul mult_u5(.clk(clk),.a(vector[5*16+:16]),.b(matrix[5*16+:16]),.c(tmp[5*16+:16]));
	signedmul mult_u6(.clk(clk),.a(vector[6*16+:16]),.b(matrix[6*16+:16]),.c(tmp[6*16+:16]));
	signedmul mult_u7(.clk(clk),.a(vector[7*16+:16]),.b(matrix[7*16+:16]),.c(tmp[7*16+:16]));
	signedmul mult_u8(.clk(clk),.a(vector[8*16+:16]),.b(matrix[8*16+:16]),.c(tmp[8*16+:16]));
	signedmul mult_u9(.clk(clk),.a(vector[9*16+:16]),.b(matrix[9*16+:16]),.c(tmp[9*16+:16]));
	signedmul mult_u10(.clk(clk),.a(vector[10*16+:16]),.b(matrix[10*16+:16]),.c(tmp[10*16+:16]));
	signedmul mult_u11(.clk(clk),.a(vector[11*16+:16]),.b(matrix[11*16+:16]),.c(tmp[11*16+:16]));
	signedmul mult_u12(.clk(clk),.a(vector[12*16+:16]),.b(matrix[12*16+:16]),.c(tmp[12*16+:16]));
	signedmul mult_u13(.clk(clk),.a(vector[13*16+:16]),.b(matrix[13*16+:16]),.c(tmp[13*16+:16]));
	signedmul mult_u14(.clk(clk),.a(vector[14*16+:16]),.b(matrix[14*16+:16]),.c(tmp[14*16+:16]));
	signedmul mult_u15(.clk(clk),.a(vector[15*16+:16]),.b(matrix[15*16+:16]),.c(tmp[15*16+:16]));
	signedmul mult_u16(.clk(clk),.a(vector[16*16+:16]),.b(matrix[16*16+:16]),.c(tmp[16*16+:16]));
	signedmul mult_u17(.clk(clk),.a(vector[17*16+:16]),.b(matrix[17*16+:16]),.c(tmp[17*16+:16]));
	signedmul mult_u18(.clk(clk),.a(vector[18*16+:16]),.b(matrix[18*16+:16]),.c(tmp[18*16+:16]));
	signedmul mult_u19(.clk(clk),.a(vector[19*16+:16]),.b(matrix[19*16+:16]),.c(tmp[19*16+:16]));
	signedmul mult_u20(.clk(clk),.a(vector[20*16+:16]),.b(matrix[20*16+:16]),.c(tmp[20*16+:16]));
	signedmul mult_u21(.clk(clk),.a(vector[21*16+:16]),.b(matrix[21*16+:16]),.c(tmp[21*16+:16]));
	signedmul mult_u22(.clk(clk),.a(vector[22*16+:16]),.b(matrix[22*16+:16]),.c(tmp[22*16+:16]));
	signedmul mult_u23(.clk(clk),.a(vector[23*16+:16]),.b(matrix[23*16+:16]),.c(tmp[23*16+:16]));
	signedmul mult_u24(.clk(clk),.a(vector[24*16+:16]),.b(matrix[24*16+:16]),.c(tmp[24*16+:16]));
	signedmul mult_u25(.clk(clk),.a(vector[25*16+:16]),.b(matrix[25*16+:16]),.c(tmp[25*16+:16]));
	signedmul mult_u26(.clk(clk),.a(vector[26*16+:16]),.b(matrix[26*16+:16]),.c(tmp[26*16+:16]));
	signedmul mult_u27(.clk(clk),.a(vector[27*16+:16]),.b(matrix[27*16+:16]),.c(tmp[27*16+:16]));
	signedmul mult_u28(.clk(clk),.a(vector[28*16+:16]),.b(matrix[28*16+:16]),.c(tmp[28*16+:16]));
	signedmul mult_u29(.clk(clk),.a(vector[29*16+:16]),.b(matrix[29*16+:16]),.c(tmp[29*16+:16]));
	signedmul mult_u30(.clk(clk),.a(vector[30*16+:16]),.b(matrix[30*16+:16]),.c(tmp[30*16+:16]));
	signedmul mult_u31(.clk(clk),.a(vector[31*16+:16]),.b(matrix[31*16+:16]),.c(tmp[31*16+:16]));

 	
endmodule                    

module vecmat_add_32 #(parameter arraysize=512,parameter vectdepth=32)
 (
 input clk,reset,
 input [arraysize-1:0] mulout,
 output reg [15:0] data_out
 );
           
 wire [15:0] tmp0, tmp1 ,tmp2 ,tmp3 ,tmp4 ,tmp5 ,tmp6 ,tmp7 ,tmp8 ,tmp9 ,tmp10 ,tmp11 ,tmp12 ,tmp13 ,tmp14 ,tmp15 ,tmp16 ,tmp17 ,tmp18 ,tmp19 ,tmp20 ,tmp21 ,tmp22 ,tmp23 ,tmp24 ,tmp25 ,tmp26 ,tmp27 ,tmp28 ,tmp29 ,tmp30 ,tmp31 ,tmp32 ,tmp33 ,tmp34 ,tmp35 ,tmp36 ,tmp37 ,tmp38 ,tmp39 ,tmp40 ,tmp41 ,tmp42 ,tmp43 ,tmp44 ,tmp45 ,tmp46 ,tmp47 ,tmp48 ,tmp49 ,tmp50,tmp51 ,tmp52 ,tmp53,tmp54 ,tmp55 ,tmp56 ,tmp57 ,tmp58, tmp59 ,tmp60 ,tmp61,tmp62; 
 reg[31:0] i;
 reg [15:0] ff1,ff3,ff5,ff7,ff9,ff11,ff13,ff15,ff17,ff19,ff21,ff23,ff25,ff27,ff29,ff31;

 always @(posedge clk) begin
	if(~reset) begin
		data_out <= tmp57;

		//adding a flop pipeline stage
		ff1 <= tmp1;
		ff3 <= tmp3;
		ff5 <= tmp5;
		ff7 <= tmp7;
		ff9 <= tmp9;
		ff11 <= tmp11;
		ff13 <= tmp13;
		ff15 <= tmp15;
		end   
 end     
                                                        
        // fixed point addition  
        qadd2 Add_u0(.a(mulout[16*0+:16]),.b(mulout[16*1+:16]),.c(tmp0));
		qadd2 Add_u2(.a(mulout[16*2+:16]),.b(mulout[16*3+:16]),.c(tmp2));
		qadd2 Add_u4(.a(mulout[16*4+:16]),.b(mulout[16*5+:16]),.c(tmp4));
		qadd2 Add_u6(.a(mulout[16*6+:16]),.b(mulout[16*7+:16]),.c(tmp6));
		qadd2 Add_u8(.a(mulout[16*8+:16]),.b(mulout[16*9+:16]),.c(tmp8));
		qadd2 Add_u10(.a(mulout[16*10+:16]),.b(mulout[16*11+:16]),.c(tmp10));
		qadd2 Add_u12(.a(mulout[16*12+:16]),.b(mulout[16*13+:16]),.c(tmp12));
		qadd2 Add_u14(.a(mulout[16*14+:16]),.b(mulout[16*15+:16]),.c(tmp14));
		qadd2 Add_u16(.a(mulout[16*16+:16]),.b(mulout[16*17+:16]),.c(tmp16));
		qadd2 Add_u18(.a(mulout[16*18+:16]),.b(mulout[16*19+:16]),.c(tmp18));
		qadd2 Add_u20(.a(mulout[16*20+:16]),.b(mulout[16*21+:16]),.c(tmp20));
		qadd2 Add_u22(.a(mulout[16*22+:16]),.b(mulout[16*23+:16]),.c(tmp22));
		qadd2 Add_u24(.a(mulout[16*24+:16]),.b(mulout[16*25+:16]),.c(tmp24));
		qadd2 Add_u26(.a(mulout[16*26+:16]),.b(mulout[16*27+:16]),.c(tmp26));
		qadd2 Add_u28(.a(mulout[16*28+:16]),.b(mulout[16*29+:16]),.c(tmp28));
		qadd2 Add_u30(.a(mulout[16*30+:16]),.b(mulout[16*31+:16]),.c(tmp30));
	            
			qadd2 Add_u1(.a(tmp0),.b(tmp2),.c(tmp1));
			qadd2 Add_u3(.a(tmp4),.b(tmp6),.c(tmp3));
			qadd2 Add_u5(.a(tmp8),.b(tmp10),.c(tmp5));
			qadd2 Add_u7(.a(tmp12),.b(tmp14),.c(tmp7));
			qadd2 Add_u9(.a(tmp16),.b(tmp18),.c(tmp9));
			qadd2 Add_u11(.a(tmp20),.b(tmp22),.c(tmp11));
			qadd2 Add_u13(.a(tmp24),.b(tmp26),.c(tmp13));
			qadd2 Add_u15(.a(tmp28),.b(tmp30),.c(tmp15));
		
			qadd2 Add_u33(.a(ff1),.b(ff3),.c(tmp33));
			qadd2 Add_u35(.a(ff5),.b(ff7),.c(tmp35));
			qadd2 Add_u37(.a(ff9),.b(ff11),.c(tmp37));
			qadd2 Add_u39(.a(ff13),.b(ff15),.c(tmp39));

			qadd2 Add_u49(.a(tmp33),.b(tmp35),.c(tmp49));
			qadd2 Add_u51(.a(tmp37),.b(tmp39),.c(tmp51));
	
			qadd2 Add_u57(.a(tmp49),.b(tmp51),.c(tmp57));
					
endmodule

module signedmul(
  input clk,
  input [15:0] a,
  input [15:0] b,
  output [15:0] c
);

wire [31:0] result;
wire [15:0] a_new;
wire [15:0] b_new;

reg [15:0] a_ff;
reg [15:0] b_ff;
reg [31:0] result_ff;
reg a_sign,b_sign,a_sign_ff,b_sign_ff;

assign c = (b_sign_ff==a_sign_ff)?result_ff[26:12]:(~result_ff[26:12]+1'b1);
assign a_new = a[15]?(~a + 1'b1):a;
assign b_new = b[15]?(~b + 1'b1):b;
assign result = a_ff*b_ff;

always@(posedge clk) begin
	a_ff <= a_new;
	b_ff <= b_new; 

	a_sign <= a[15];
	b_sign <= b[15];
	a_sign_ff <= a_sign;
	b_sign_ff <= b_sign;
    result_ff <= result;
    
end


endmodule


module qadd2(
 input [15:0] a,
 input [15:0] b,
 output [15:0] c
    );
    
assign c = a + b;


endmodule


 
module dpram (	
input clk,
input [4:0] address_a,
input [4:0] address_b,
input  wren_a,
input  wren_b,
input [(`VECTOR_BITS-1):0] data_a,
input [(`VECTOR_BITS-1):0] data_b,
output reg [(`VECTOR_BITS-1):0] out_a,
output reg [(`VECTOR_BITS-1):0] out_b
);


`ifdef SIMULATION_MEMORY

reg [`VECTOR_BITS-1:0] ram[`NUM_WORDS-1:0];

always @ (posedge clk) begin 
  if (wren_a) begin
      ram[address_a] <= data_a;
  end
  else begin
      out_a <= ram[address_a];
  end
end
  
always @ (posedge clk) begin 
  if (wren_b) begin
      ram[address_b] <= data_b;
  end 
  else begin
      out_b <= ram[address_b];
  end
end

`else

dual_port_ram u_dual_port_ram(
.addr1(address_a),
.we1(wren_a),
.data1(data_a),
.out1(out_a),
.addr2(address_b),
.we2(wren_b),
.data2(data_b),
.out2(out_b),
.clk(clk)
);

`endif

endmodule

 
module dpram_t (	
input clk,
input [5:0] address_a,
input [5:0] address_b,
input  wren_a,
input  wren_b,
input [((`NUM_WORDS*`DATA_WIDTH)-1):0] data_a,
input [((`NUM_WORDS*`DATA_WIDTH)-1):0] data_b,
output reg [((`NUM_WORDS*`DATA_WIDTH)-1):0] out_a,
output reg [((`NUM_WORDS*`DATA_WIDTH)-1):0] out_b
);


`ifdef SIMULATION_MEMORY

reg [(`NUM_WORDS*`DATA_WIDTH)-1:0] ram[`VECTOR_DEPTH-1:0];

always @ (posedge clk) begin 
  if (wren_a) begin
      ram[address_a] <= data_a;
  end
  else begin
      out_a <= ram[address_a];
  end
end
  
always @ (posedge clk) begin 
  if (wren_b) begin
      ram[address_b] <= data_b;
  end 
  else begin
      out_b <= ram[address_b];
  end
end

`else

dual_port_ram u_dual_port_ram(
.addr1(address_a),
.we1(wren_a),
.data1(data_a),
.out1(out_a),
.addr2(address_b),
.we2(wren_b),
.data2(data_b),
.out2(out_b),
.clk(clk)
);

`endif

endmodule

module dpram_small (	
input clk,
input [8:0] address_a,
input [8:0] address_b,
input  wren_a,
input  wren_b,
input [`DATA_WIDTH-1:0] data_a,
input [`DATA_WIDTH-1:0] data_b,
output reg [`DATA_WIDTH-1:0] out_a,
output reg [`DATA_WIDTH-1:0] out_b
);


`ifdef SIMULATION_MEMORY

reg [`DATA_WIDTH-1:0] ram[`OUT_RAM_DEPTH-1:0];

always @ (posedge clk) begin 
  if (wren_a) begin
      ram[address_a] <= data_a;
  end
  else begin
      out_a <= ram[address_a];
  end
end
  
always @ (posedge clk) begin 
  if (wren_b) begin
      ram[address_b] <= data_b;
  end 
  else begin
      out_b <= ram[address_b];
  end
end

`else

dual_port_ram u_dual_port_ram(
.addr1(address_a),
.we1(wren_a),
.data1(data_a),
.out1(out_a),
.addr2(address_b),
.we2(wren_b),
.data2(data_b),
.out2(out_b),
.clk(clk)
);

`endif

endmodule



module wordwise_bram (
        addr0, 
        d0, 
        we0, 
        q0,  
        addr1,
        d1,
        we1,
        q1,
        clk);

input [`BUF_AWIDTH-1:0] addr0;
input [`BUF_AWIDTH-1:0] addr1;
input [`BUF_LOC_SIZE*`DATA_WIDTH-1:0] d0;
input [`BUF_LOC_SIZE*`DATA_WIDTH-1:0] d1;
input [`BUF_LOC_SIZE-1:0] we0;
input [`BUF_LOC_SIZE-1:0] we1;
output reg [`BUF_LOC_SIZE*`DATA_WIDTH-1:0] q0;
output reg [`BUF_LOC_SIZE*`DATA_WIDTH-1:0] q1;
input clk;

`ifdef SIMULATION_MEMORY

reg [`BUF_LOC_SIZE*`DATA_WIDTH-1:0] ram[((1<<`BUF_AWIDTH)-1):0];
reg [3:0] i;

always @(posedge clk)  
begin 
    if(we0 == 0) begin   //read
	 for (i = 0; i < `BUF_LOC_SIZE; i=i+1) begin
        	q0[i*`DATA_WIDTH +: `DATA_WIDTH] <= ram[addr0][i*`DATA_WIDTH +: `DATA_WIDTH];
    	end
    end
    else begin  //write
    	for (i = 0; i < `BUF_LOC_SIZE; i=i+1) begin
      	  if (we0[i]) ram[addr0][i*`DATA_WIDTH +: `DATA_WIDTH] <= d0[i*`DATA_WIDTH +: `DATA_WIDTH]; 
    	end   
   end 
     
end

always @(posedge clk)  
begin 
    if(we1 == 0) begin 
	 for (i = 0; i < `BUF_LOC_SIZE; i=i+1) begin
        q1[i*`DATA_WIDTH +: `DATA_WIDTH] <= ram[addr1][i*`DATA_WIDTH +: `DATA_WIDTH];
        end
   end
    else begin  
    	for (i = 0; i < `BUF_LOC_SIZE; i=i+1) begin
        	if (we1[i]) ram[addr1][i*`DATA_WIDTH +: `DATA_WIDTH] <= d1[i*`DATA_WIDTH +: `DATA_WIDTH]; 
    	end
    end    
     
end

`else
//BRAMs available in VTR FPGA architectures have one bit write-enables.
//So let's combine multiple bits into 1. We don't have a usecase of
//writing/not-writing only parts of the word anyway.
wire we0_coalesced;
assign we0_coalesced = |we0;
wire we1_coalesced;
assign we1_coalesced = |we1;

dual_port_ram u_dual_port_ram(
.addr1(addr0),
.we1(we0_coalesced),
.data1(d0),
.out1(q0),
.addr2(addr1),
.we2(we1_coalesced),
.data2(d1),
.out2(q1),
.clk(clk)
);

`endif

endmodule


module wordwise_bram_2 (
        addr0, 
        d0, 
        we0, 
        q0,  
        addr1,
        d1,
        we1,
        q1,
        clk);

input addr0;
input addr1;
input [8*`BUF_LOC_SIZE*`DATA_WIDTH-1:0] d0;
input [8*`BUF_LOC_SIZE*`DATA_WIDTH-1:0] d1;
input [8*`BUF_LOC_SIZE-1:0] we0;
input [8*`BUF_LOC_SIZE-1:0] we1;
output reg [8*`BUF_LOC_SIZE*`DATA_WIDTH-1:0] q0;
output reg [8*`BUF_LOC_SIZE*`DATA_WIDTH-1:0] q1;
input clk;

`ifdef SIMULATION_MEMORY

reg [(8*`BUF_LOC_SIZE*`DATA_WIDTH)-1:0] ram[1:0];   //ping pong buffer
reg [5:0] i;

always @(posedge clk)  
begin 
    if(we0 == 0) begin   //read
	 for (i = 0; i < `NUM_WORDS; i=i+1) begin
        	q0[i*`DATA_WIDTH +: `DATA_WIDTH] <= ram[addr0][i*`DATA_WIDTH +: `DATA_WIDTH];
    	end
    end
    else begin  //write
    	for (i = 0; i < `NUM_WORDS; i=i+1) begin
      	  if (we0[i]) ram[addr0][i*`DATA_WIDTH +: `DATA_WIDTH] <= d0[i*`DATA_WIDTH +: `DATA_WIDTH]; 
    	end   
   end 
     
end

always @(posedge clk)  
begin 
    if(we1 == 0) begin 
	 for (i = 0; i < `NUM_WORDS; i=i+1) begin
        q1[i*`DATA_WIDTH +: `DATA_WIDTH] <= ram[addr1][i*`DATA_WIDTH +: `DATA_WIDTH];
        end
   end
    else begin  
    	for (i = 0; i < `NUM_WORDS; i=i+1) begin
        	if (we1[i]) ram[addr1][i*`DATA_WIDTH +: `DATA_WIDTH] <= d1[i*`DATA_WIDTH +: `DATA_WIDTH]; 
    	end
    end    
     
end

`else
//BRAMs available in VTR FPGA architectures have one bit write-enables.
//So let's combine multiple bits into 1. We don't have a usecase of
//writing/not-writing only parts of the word anyway.
wire we0_coalesced;
assign we0_coalesced = |we0;
wire we1_coalesced;
assign we1_coalesced = |we1;

dual_port_ram u_dual_port_ram(
.addr1(addr0),
.we1(we0_coalesced),
.data1(d0),
.out1(q0),
.addr2(addr1),
.we2(we1_coalesced),
.data2(d1),
.out2(q1),
.clk(clk)
);

`endif

endmodule


/////////////////////////////////////////////////////////////////
// Softmax block
// Authors: Pragnesh Patel and Aman Arora
/////////////////////////////////////////////////////////////////

//softmax_p4_smem_rfixed16_alut_v512_b2_0_10.v
`ifndef DEFINES_DONE
`define DEFINES_DONE
`define DATAWIDTH (`SIGN+`EXPONENT+`MANTISSA)
`define IEEE_COMPLIANCE 1
`define NUM 4
`define ADDRSIZE 4
`endif

//fixed adder adds unsigned fixed numbers. Overflow flag is high in case of overflow
module softmax(
  inp,      //data in from memory to max block
  sub0_inp, //data inputs from memory to first-stage subtractors
  sub1_inp, //data inputs from memory to second-stage subtractors

  start_addr,   //the first address that contains input data in the on-chip memory
  end_addr,     //max address containing required data

  addr,          //address corresponding to data inp
  sub0_inp_addr, //address corresponding to sub0_inp
  sub1_inp_addr, //address corresponding to sub1_inp
  //addr_sel,
  outp0,
  outp1,
  outp2,
  outp3,

  clk,
  reset,
  init,   //the signal indicating to latch the new start address
  done,   //done signal asserts when the softmax calculation is over
  start); //start signal for the overall softmax operation

  input clk;
  input reset;
  input start;
  input init;

  input  [`DATAWIDTH*`NUM-1:0] inp;
  input  [`DATAWIDTH*`NUM-1:0] sub0_inp;
  input  [`DATAWIDTH*`NUM-1:0] sub1_inp;
  input  [`ADDRSIZE-1:0]       end_addr;
  input  [`ADDRSIZE-1:0]       start_addr;

  output [`ADDRSIZE-1 :0] addr;
  output  [`ADDRSIZE-1:0] sub0_inp_addr;
  output  [`ADDRSIZE-1:0] sub1_inp_addr;
  //output addr_sel;
  output [`DATAWIDTH-1:0] outp0;
  output [`DATAWIDTH-1:0] outp1;
  output [`DATAWIDTH-1:0] outp2;
  output [`DATAWIDTH-1:0] outp3;
  output done;
  
  reg [`DATAWIDTH*`NUM-1:0] inp_reg;
  reg [`ADDRSIZE-1:0] addr;
  reg [`DATAWIDTH*`NUM-1:0] sub0_inp_reg;
  reg [`DATAWIDTH*`NUM-1:0] sub1_inp_reg;
  reg [`ADDRSIZE-1:0] sub0_inp_addr;
  reg [`ADDRSIZE-1:0] sub1_inp_addr;


  ////-----------control signals--------------////
  reg mode1_start;
  reg mode1_run;
  reg mode2_start;
  reg mode2_run;

  reg mode3_stage_run2;
  reg mode3_stage_run;
  reg mode7_stage_run2;
  reg mode7_stage_run;
  
  reg mode3_run;

  reg mode1_stage1_run_a;
  reg mode1_stage0_run;
  reg mode1_stage1_run;
  wire mode1_stage2_run;
  assign mode1_stage2_run = mode1_run;

  reg mode4_stage1_run_a;
  reg mode4_stage2_run_a;
  reg mode4_stage0_run;
  reg mode4_stage1_run;
  reg mode4_stage2_run;

  reg mode5_run;
  reg mode6_run;
  reg mode7_run;
  reg presub_start;
  reg presub_run;
  reg presub_run_2;
  reg presub_run_1;
  reg presub_run_0;
  reg done;

  //assign addr_sel = mode1_run & ~mode2_start;

  always @(posedge clk)begin
    mode4_stage1_run_a <= mode4_stage1_run;
    mode4_stage2_run_a <= mode4_stage2_run;
  end

  always @(posedge clk)begin
    mode1_stage1_run_a <= mode1_stage1_run;
  end

  always @(posedge clk) begin
    if(reset) begin
      addr <= 0;
      mode1_start <= 0;
      mode1_run <= 0;
    end
    //init latch the input address
    else if(init) begin
      addr <= start_addr;
    end
    //start the mode1 max calculation
    else if(start)begin
      mode1_start <= 1;
    end
    //logic when to finish mode1 and trigger mode2 to latch the mode2 address
    else if(mode1_start && addr < end_addr) begin 
      addr <= addr + 1;
      mode1_run <= 1;
    end else if(addr == end_addr)begin 
      addr <= 0;
      mode1_run <= 0;
      mode1_start <= 0;
    end else begin
      mode1_run <= 0;
    end
  end

   //always @(posedge clk) begin
   // if(reset) begin
   //   mode1_stage2_run <= 0;
   // end
   // else if (mode1_run == 1) begin
   //   mode1_stage2_run <= 1;
   // end
   // else begin
   //   mode1_stage2_run <= 0;
   // end
   //end

  always @(posedge clk) begin
    if(reset) begin
      mode1_stage1_run <= 0;
    end
    else if (mode1_stage2_run == 1) begin
      mode1_stage1_run <= 1;
    end
    else begin
      mode1_stage1_run <= 0;
    end
  end
  
    always @(posedge clk) begin
    if(reset) begin
      mode1_stage0_run <= 0;
    end
    else if (mode1_stage1_run == 1) begin
      mode1_stage0_run <= 1;
    end
    else begin
      mode1_stage0_run <= 0;
    end
  end

  always @(posedge clk) begin
    if(reset) begin
      sub0_inp_addr <= 0;
      sub0_inp_reg <= 0;
      mode2_start <= 0;
      mode2_run <= 0;
    end
    else if (mode1_stage1_run_a & ~mode1_stage1_run) begin
    //else if ((mode1_start) && (addr == (end_addr - 1))) begin
        mode2_start <= 1;
        sub0_inp_addr <= start_addr;
	end
    //logic when to finish mode2
    else if(mode2_start && sub0_inp_addr < end_addr) begin
      sub0_inp_addr <= sub0_inp_addr + 1;
      sub0_inp_reg <= sub0_inp;
      mode2_run <= 1;
    end 
	else if(sub0_inp_addr == end_addr)begin
      sub0_inp_addr <= 0;
      sub0_inp_reg <= 0;
      mode2_run <= 0;
      mode2_start <= 0;
    end
  end	

  always @(posedge clk) begin
    if(reset) begin
      mode3_stage_run2 <= 0;
    end
    //logic when to trigger mode3
    else if(mode2_run == 1) begin
      mode3_stage_run2 <= 1;
    end else begin
      mode3_stage_run2 <= 0;
    end
  end	

  always @(posedge clk) begin
    if(reset) begin
      mode3_stage_run <= 0;
    end
    else if(mode3_stage_run2 == 1) begin
      mode3_stage_run <= 1;
    end else begin
      mode3_stage_run <= 0;
    end
  end	

  always @(posedge clk) begin
    if(reset) begin
      mode3_run <= 0;
    end
    else if(mode3_stage_run == 1) begin
      mode3_run <= 1;
    end else begin
      mode3_run <= 0;
    end
  end

  always @(posedge clk) begin
    if(reset) begin
      mode4_stage2_run <= 0;
    end
    //logic when to trigger mode4 last stage adderTree, since the final results of adderTree
    //is always ready 1 cycle after mode3 finishes, so there is no need on extra
    //logic to control the adderTree outputs
    else if (mode3_run == 1) begin
      mode4_stage2_run <= 1;
    end else begin
      mode4_stage2_run <= 0;
    end
  end

  always @(posedge clk) begin
    if(reset) begin
      mode4_stage1_run <= 0;
    end
    else if (mode4_stage2_run == 1) begin
      mode4_stage1_run <= 1;
    end else begin
      mode4_stage1_run <= 0;
    end
  end

  always @(posedge clk) begin
    if(reset) begin
      mode4_stage0_run <= 0;
    end
    else if (mode4_stage1_run == 1) begin
      mode4_stage0_run <= 1;
    end else begin
      mode4_stage0_run <= 0;
    end
  end

  reg mode5_stage0_run;
  reg mode5_stage1_run;
  reg mode5_stage2_run;  
  wire mode5_stage3_run = mode5_run;

  always @(posedge clk) begin
    if(reset) begin
      mode5_run <= 0;
    end
    //mode5 should be triggered right at the falling edge of mode4_stage1_run
    else if(mode4_stage1_run_a & ~mode4_stage1_run) begin
      mode5_run <= 1;
    end 
	else if(mode4_stage1_run == 0) begin
      mode5_run <= 0;
    end
  end
  
  always @(posedge clk) begin
    if(reset) begin
      mode5_stage2_run <= 0;
    end
    else if (mode5_stage3_run == 1) begin
      mode5_stage2_run <= 1;
    end
    else begin
      mode5_stage2_run <= 0;
    end
  end
    always @(posedge clk) begin
    if(reset) begin
      mode5_stage1_run <= 0;
    end
    else if (mode5_stage2_run == 1) begin
      mode5_stage1_run <= 1;
    end
    else begin
      mode5_stage1_run <= 0;
    end
  end
  
    always @(posedge clk) begin
    if(reset) begin
      mode5_stage0_run <= 0;
    end
    else if (mode5_stage1_run == 1) begin
      mode5_stage0_run <= 1;
    end
    else begin
      mode5_stage0_run <= 0;
    end
  end

  always @(posedge clk) begin
    if(reset) begin
      presub_start <= 0;
      sub1_inp_addr <= 0;
      sub1_inp_reg <= 0;
      presub_run <= 0;
    end
    else if(mode4_stage2_run_a & ~mode4_stage2_run) begin
      presub_start <= 1;
      sub1_inp_addr <= start_addr;
      sub1_inp_reg <= sub1_inp;
    end
    else if(~reset && presub_start && sub1_inp_addr < end_addr)begin
      sub1_inp_addr <= sub1_inp_addr + 1;
      sub1_inp_reg <= sub1_inp;
      presub_run <= 1;
    end 
	else if(sub1_inp_addr == end_addr) begin
      presub_run <= 0;
      presub_start <= 0;
      sub1_inp_addr <= 0;
      sub1_inp_reg <= 0;
    end
  end

   always @(posedge clk) begin
    if(reset) begin
      presub_run_2 <= 0;
    end
    else if (presub_run == 1) begin
      presub_run_2 <= 1;
    end
    else begin
      presub_run_2 <= 0;
    end
  end

   always @(posedge clk) begin
    if(reset) begin
      presub_run_1 <= 0;
    end
    else if (presub_run_2 == 1) begin
      presub_run_1 <= 1;
    end
    else begin
      presub_run_1 <= 0;
    end
  end

   always @(posedge clk) begin
    if(reset) begin
      presub_run_0 <= 0;
    end
    else if (presub_run_1 == 1) begin
      presub_run_0 <= 1;
    end
    else begin
      presub_run_0 <= 0;
    end
  end

  always @(posedge clk) begin
    if(reset) begin
      mode6_run <= 0;
	end  
    else if(presub_run_0) begin
      mode6_run <= 1;
    end else begin
      mode6_run <= 0;
    end
  end

  always @(posedge clk) begin
    if(reset) begin
      mode7_stage_run2 <= 0;
	end
    else if(mode6_run == 1) begin
      mode7_stage_run2 <= 1;
    end else begin
      mode7_stage_run2 <= 0;
    end
  end

  always @(posedge clk) begin
    if(reset) begin
      mode7_stage_run <= 0;
	end
    else if(mode7_stage_run2 == 1) begin
      mode7_stage_run <= 1;
    end else begin
      mode7_stage_run <= 0;
    end
  end 
  
  always @(posedge clk) begin
    if(reset) begin
      mode7_run <= 0;
	end
	else if(mode7_stage_run == 1) begin
      mode7_run <= 1;
    end else begin
      mode7_run <= 0;
    end
  end

  always @(posedge clk) begin
    if(reset) begin
      done <= 0;
	end  
    else if(mode7_run) begin
      done <= 1;
    end else begin
      done <= 0;
    end
  end

  ////------mode1 max block---------///////
  wire [`DATAWIDTH-1:0] max_outp;

  mode1_max_tree mode1_max(
      .inp0(inp[`DATAWIDTH*1-1:`DATAWIDTH*0]),
      .inp1(inp[`DATAWIDTH*2-1:`DATAWIDTH*1]),
      .inp2(inp[`DATAWIDTH*3-1:`DATAWIDTH*2]),
      .inp3(inp[`DATAWIDTH*4-1:`DATAWIDTH*3]),
      .mode1_stage2_run(mode1_stage2_run),
      .mode1_stage1_run(mode1_stage1_run),
      .mode1_stage0_run(mode1_stage0_run),
      .clk(clk),
      .reset(reset),
      .outp(max_outp));

  ////------mode2 subtraction---------///////
  wire [`DATAWIDTH-1:0] mode2_outp_sub0;
  wire [`DATAWIDTH-1:0] mode2_outp_sub1;
  wire [`DATAWIDTH-1:0] mode2_outp_sub2;
  wire [`DATAWIDTH-1:0] mode2_outp_sub3;
  mode2_sub mode2_sub(
      .a_inp0(sub0_inp_reg[`DATAWIDTH*1-1:`DATAWIDTH*0]),
      .a_inp1(sub0_inp_reg[`DATAWIDTH*2-1:`DATAWIDTH*1]),
      .a_inp2(sub0_inp_reg[`DATAWIDTH*3-1:`DATAWIDTH*2]),
      .a_inp3(sub0_inp_reg[`DATAWIDTH*4-1:`DATAWIDTH*3]),
      .outp0(mode2_outp_sub0),
      .outp1(mode2_outp_sub1),
      .outp2(mode2_outp_sub2),
      .outp3(mode2_outp_sub3),
      .b_inp(max_outp));

  reg [`DATAWIDTH-1:0] mode2_outp_sub0_reg;
  reg [`DATAWIDTH-1:0] mode2_outp_sub1_reg;
  reg [`DATAWIDTH-1:0] mode2_outp_sub2_reg;
  reg [`DATAWIDTH-1:0] mode2_outp_sub3_reg;
  always @(posedge clk) begin
    if (reset) begin
      mode2_outp_sub0_reg <= 0;
      mode2_outp_sub1_reg <= 0;
      mode2_outp_sub2_reg <= 0;
      mode2_outp_sub3_reg <= 0;
    end else if (mode2_run) begin
      mode2_outp_sub0_reg <= mode2_outp_sub0;
      mode2_outp_sub1_reg <= mode2_outp_sub1;
      mode2_outp_sub2_reg <= mode2_outp_sub2;
      mode2_outp_sub3_reg <= mode2_outp_sub3;
    end
  end

  ////------mode3 exponential---------///////
  wire [`DATAWIDTH-1:0] mode3_outp_exp0;
  wire [`DATAWIDTH-1:0] mode3_outp_exp1;
  wire [`DATAWIDTH-1:0] mode3_outp_exp2;
  wire [`DATAWIDTH-1:0] mode3_outp_exp3;
  mode3_exp mode3_exp(
      .inp0(mode2_outp_sub0_reg),
      .inp1(mode2_outp_sub1_reg),
      .inp2(mode2_outp_sub2_reg),
      .inp3(mode2_outp_sub3_reg),

      .clk(clk),
      .reset(reset),
      .stage_run(mode3_stage_run),
      .stage_run2(mode3_stage_run2),

      .outp0(mode3_outp_exp0),
      .outp1(mode3_outp_exp1),
      .outp2(mode3_outp_exp2),
      .outp3(mode3_outp_exp3)
  );

  reg [`DATAWIDTH-1:0] mode3_outp_exp0_reg;
  reg [`DATAWIDTH-1:0] mode3_outp_exp1_reg;
  reg [`DATAWIDTH-1:0] mode3_outp_exp2_reg;
  reg [`DATAWIDTH-1:0] mode3_outp_exp3_reg;
  always @(posedge clk) begin
    if (reset) begin
      mode3_outp_exp0_reg <= 0;
      mode3_outp_exp1_reg <= 0;
      mode3_outp_exp2_reg <= 0;
      mode3_outp_exp3_reg <= 0;
    end else if (mode3_run) begin
      mode3_outp_exp0_reg <= mode3_outp_exp0;
      mode3_outp_exp1_reg <= mode3_outp_exp1;
      mode3_outp_exp2_reg <= mode3_outp_exp2;
      mode3_outp_exp3_reg <= mode3_outp_exp3;
    end
  end

  //////------mode4 pipelined adder tree---------///////
  wire [`DATAWIDTH-1:0] mode4_adder_tree_outp;
  mode4_adder_tree mode4_adder_tree(
    .inp0(mode3_outp_exp0_reg),
    .inp1(mode3_outp_exp1_reg),
    .inp2(mode3_outp_exp2_reg),
    .inp3(mode3_outp_exp3_reg),
    .mode4_stage2_run(mode4_stage2_run),
    .mode4_stage1_run(mode4_stage1_run),
    .mode4_stage0_run(mode4_stage0_run),

    .clk(clk),
    .reset(reset),
    .outp(mode4_adder_tree_outp)
  );


  //////------mode5 log---------///////
  wire [`DATAWIDTH-1:0] mode5_outp_log;
  reg  [`DATAWIDTH-1:0] mode5_outp_log_reg;
  mode5_ln mode5_ln(.inp(mode4_adder_tree_outp), .outp(mode5_outp_log), .clk(clk), .reset(reset), .mode5_stage3_run(mode5_stage3_run), .mode5_stage2_run(mode5_stage2_run), .mode5_stage1_run(mode5_stage1_run) );

  always @(posedge clk) begin
    if(reset) begin
      mode5_outp_log_reg <= 0;
    end else if(mode5_stage0_run) begin
      mode5_outp_log_reg <= mode5_outp_log;
    end
  end

  //////------mode6 pre-sub---------///////
  wire [`DATAWIDTH-1:0] mode6_outp_presub0;
  wire [`DATAWIDTH-1:0] mode6_outp_presub1;
  wire [`DATAWIDTH-1:0] mode6_outp_presub2;
  wire [`DATAWIDTH-1:0] mode6_outp_presub3;
  reg [`DATAWIDTH-1:0] mode6_outp_presub0_reg_3;
  reg [`DATAWIDTH-1:0] mode6_outp_presub1_reg_3;
  reg [`DATAWIDTH-1:0] mode6_outp_presub2_reg_3;
  reg [`DATAWIDTH-1:0] mode6_outp_presub3_reg_3;
  reg [`DATAWIDTH-1:0] mode6_outp_presub0_reg_2;
  reg [`DATAWIDTH-1:0] mode6_outp_presub1_reg_2;
  reg [`DATAWIDTH-1:0] mode6_outp_presub2_reg_2;
  reg [`DATAWIDTH-1:0] mode6_outp_presub3_reg_2;
  reg [`DATAWIDTH-1:0] mode6_outp_presub0_reg_1;
  reg [`DATAWIDTH-1:0] mode6_outp_presub1_reg_1;
  reg [`DATAWIDTH-1:0] mode6_outp_presub2_reg_1;
  reg [`DATAWIDTH-1:0] mode6_outp_presub3_reg_1;
  reg [`DATAWIDTH-1:0] mode6_outp_presub0_reg_0;
  reg [`DATAWIDTH-1:0] mode6_outp_presub1_reg_0;
  reg [`DATAWIDTH-1:0] mode6_outp_presub2_reg_0;
  reg [`DATAWIDTH-1:0] mode6_outp_presub3_reg_0;



  mode6_sub pre_sub(
      .a_inp0(sub1_inp_reg[`DATAWIDTH*1-1:`DATAWIDTH*0]),
      .a_inp1(sub1_inp_reg[`DATAWIDTH*2-1:`DATAWIDTH*1]),
      .a_inp2(sub1_inp_reg[`DATAWIDTH*3-1:`DATAWIDTH*2]),
      .a_inp3(sub1_inp_reg[`DATAWIDTH*4-1:`DATAWIDTH*3]),
      .b_inp(max_outp),
      .outp0(mode6_outp_presub0),
      .outp1(mode6_outp_presub1),
      .outp2(mode6_outp_presub2),
      .outp3(mode6_outp_presub3)
  );
  always @(posedge clk) begin
    if (reset) begin
      mode6_outp_presub0_reg_3 <= 0;
      mode6_outp_presub1_reg_3 <= 0;
      mode6_outp_presub2_reg_3 <= 0;
      mode6_outp_presub3_reg_3 <= 0;
    end else if (presub_run) begin
      mode6_outp_presub0_reg_3 <= mode6_outp_presub0;
      mode6_outp_presub1_reg_3 <= mode6_outp_presub1;
      mode6_outp_presub2_reg_3 <= mode6_outp_presub2;
      mode6_outp_presub3_reg_3 <= mode6_outp_presub3;
    end
  end
  
  always @(posedge clk) begin
    if (reset) begin
      mode6_outp_presub0_reg_2 <= 0;
      mode6_outp_presub1_reg_2 <= 0;
      mode6_outp_presub2_reg_2 <= 0;
      mode6_outp_presub3_reg_2 <= 0;
    end else if (presub_run_2) begin
      mode6_outp_presub0_reg_2 <= mode6_outp_presub0_reg_3;
      mode6_outp_presub1_reg_2 <= mode6_outp_presub1_reg_3;
      mode6_outp_presub2_reg_2 <= mode6_outp_presub2_reg_3;
      mode6_outp_presub3_reg_2 <= mode6_outp_presub3_reg_3;
    end
  end

  always @(posedge clk) begin
    if (reset) begin
      mode6_outp_presub0_reg_1 <= 0;
      mode6_outp_presub1_reg_1 <= 0;
      mode6_outp_presub2_reg_1 <= 0;
      mode6_outp_presub3_reg_1 <= 0;
    end else if (presub_run_1) begin
      mode6_outp_presub0_reg_1 <= mode6_outp_presub0_reg_2;
      mode6_outp_presub1_reg_1 <= mode6_outp_presub1_reg_2;
      mode6_outp_presub2_reg_1 <= mode6_outp_presub2_reg_2;
      mode6_outp_presub3_reg_1 <= mode6_outp_presub3_reg_2;
    end
  end
  
  always @(posedge clk) begin
    if (reset) begin
      mode6_outp_presub0_reg_0 <= 0;
      mode6_outp_presub1_reg_0 <= 0;
      mode6_outp_presub2_reg_0 <= 0;
      mode6_outp_presub3_reg_0 <= 0;
    end else if (presub_run_0) begin
      mode6_outp_presub0_reg_0 <= mode6_outp_presub0_reg_1;
      mode6_outp_presub1_reg_0 <= mode6_outp_presub1_reg_1;
      mode6_outp_presub2_reg_0 <= mode6_outp_presub2_reg_1;
      mode6_outp_presub3_reg_0 <= mode6_outp_presub3_reg_1;
    end
  end

  //////------mode6 logsub ---------///////
  wire [`DATAWIDTH-1:0] mode6_outp_logsub0;
  wire [`DATAWIDTH-1:0] mode6_outp_logsub1;
  wire [`DATAWIDTH-1:0] mode6_outp_logsub2;
  wire [`DATAWIDTH-1:0] mode6_outp_logsub3;
  reg [`DATAWIDTH-1:0] mode6_outp_logsub0_reg;
  reg [`DATAWIDTH-1:0] mode6_outp_logsub1_reg;
  reg [`DATAWIDTH-1:0] mode6_outp_logsub2_reg;
  reg [`DATAWIDTH-1:0] mode6_outp_logsub3_reg;

  mode6_sub log_sub(
      .a_inp0(mode6_outp_presub0_reg_0),
      .a_inp1(mode6_outp_presub1_reg_0),
      .a_inp2(mode6_outp_presub2_reg_0),
      .a_inp3(mode6_outp_presub3_reg_0),
      .b_inp(mode5_outp_log_reg),
      .outp0(mode6_outp_logsub0),
      .outp1(mode6_outp_logsub1),
      .outp2(mode6_outp_logsub2),
      .outp3(mode6_outp_logsub3)
  );
  always @(posedge clk) begin
    if (reset) begin
      mode6_outp_logsub0_reg <= 0;
      mode6_outp_logsub1_reg <= 0;
      mode6_outp_logsub2_reg <= 0;
      mode6_outp_logsub3_reg <= 0;
    end else if (mode6_run) begin
      mode6_outp_logsub0_reg <= mode6_outp_logsub0;
      mode6_outp_logsub1_reg <= mode6_outp_logsub1;
      mode6_outp_logsub2_reg <= mode6_outp_logsub2;
      mode6_outp_logsub3_reg <= mode6_outp_logsub3;
    end
  end

  //////------mode7 exp---------///////
  wire [`DATAWIDTH-1:0] outp0_temp;
  wire [`DATAWIDTH-1:0] outp1_temp;
  wire [`DATAWIDTH-1:0] outp2_temp;
  wire [`DATAWIDTH-1:0] outp3_temp;
  reg [`DATAWIDTH-1:0] outp0;
  reg [`DATAWIDTH-1:0] outp1;
  reg [`DATAWIDTH-1:0] outp2;
  reg [`DATAWIDTH-1:0] outp3;

  mode7_exp mode7_exp(
      .inp0(mode6_outp_logsub0_reg),
      .inp1(mode6_outp_logsub1_reg),
      .inp2(mode6_outp_logsub2_reg),
      .inp3(mode6_outp_logsub3_reg),

      .clk(clk),
      .reset(reset),
      .stage_run(mode7_stage_run),
      .stage_run2(mode7_stage_run2),

      .outp0(outp0_temp),
      .outp1(outp1_temp),
      .outp2(outp2_temp),
      .outp3(outp3_temp)
  );
  always @(posedge clk) begin
    if (reset) begin
      outp0 <= 0;
      outp1 <= 0;
      outp2 <= 0;
      outp3 <= 0;
    end else if (mode7_run) begin
      outp0 <= outp0_temp;
      outp1 <= outp1_temp;
      outp2 <= outp2_temp;
      outp3 <= outp3_temp;
    end
  end

endmodule



module mode1_max_tree(
  inp0, 
  inp1, 
  inp2, 
  inp3, 
 
  mode1_stage2_run,
  mode1_stage1_run,
  mode1_stage0_run,
  clk,
  reset,
  outp

);

  input  [`DATAWIDTH-1 : 0] inp0; 
  input  [`DATAWIDTH-1 : 0] inp1; 
  input  [`DATAWIDTH-1 : 0] inp2; 
  input  [`DATAWIDTH-1 : 0] inp3; 

  input mode1_stage2_run;
  input mode1_stage1_run;	
  input mode1_stage0_run;
  input clk;
  input reset;

  output reg [`DATAWIDTH-1 : 0] outp;

  wire   [`DATAWIDTH-1 : 0] cmp0_out_stage2;
  wire   [`DATAWIDTH-1 : 0] cmp1_out_stage2;
  wire   [`DATAWIDTH-1 : 0] cmp0_out_stage1;
  wire   [`DATAWIDTH-1 : 0] cmp0_out_stage0;

  reg   [`DATAWIDTH-1 : 0] cmp0_out_stage2_reg;
  reg   [`DATAWIDTH-1 : 0] cmp1_out_stage2_reg;
  reg   [`DATAWIDTH-1 : 0] cmp0_out_stage1_reg;
  
  always @(posedge clk) begin
    if (reset) begin
      outp <= 0;
    end

    else if(~reset && mode1_stage0_run) begin
      outp <= cmp0_out_stage0;
    end

  end

wire cmp0_stage2_aeb;
wire cmp0_stage2_aneb;
wire cmp0_stage2_alb;
wire cmp0_stage2_aleb;
wire cmp0_stage2_agb;
wire cmp0_stage2_ageb;
wire cmp0_stage2_unordered;

wire cmp1_stage2_aeb;
wire cmp1_stage2_aneb;
wire cmp1_stage2_alb;
wire cmp1_stage2_aleb;
wire cmp1_stage2_agb;
wire cmp1_stage2_ageb;
wire cmp1_stage2_unordered;

wire cmp0_stage1_aeb;
wire cmp0_stage1_aneb;
wire cmp0_stage1_alb;
wire cmp0_stage1_aleb;
wire cmp0_stage1_agb;
wire cmp0_stage1_ageb;
wire cmp0_stage1_unordered;

wire cmp0_stage0_aeb;
wire cmp0_stage0_aneb;
wire cmp0_stage0_alb;
wire cmp0_stage0_aleb;
wire cmp0_stage0_agb;
wire cmp0_stage0_ageb;
wire cmp0_stage0_unordered;

comparator cmp0_stage2(.a(inp0),       .b(inp1),        .aeb(cmp0_stage2_aeb), .aneb(cmp0_stage2_aneb), .alb(cmp0_stage2_alb), .aleb(cmp0_stage2_aleb), .agb(cmp0_stage2_agb), .ageb(cmp0_stage2_ageb), .unordered(cmp0_stage2_unordered));
assign cmp0_out_stage2 = (cmp0_stage2_ageb==1'b1) ? inp0 : inp1;

comparator cmp1_stage2(.a(inp2),       .b(inp3),         .aeb(cmp1_stage2_aeb), .aneb(cmp1_stage2_aneb), .alb(cmp1_stage2_alb), .aleb(cmp1_stage2_aleb), .agb(cmp1_stage2_agb), .ageb(cmp1_stage2_ageb), .unordered(cmp1_stage2_unordered));   
assign cmp1_out_stage2 = (cmp1_stage2_ageb==1'b1) ? inp2 : inp3;

always @(posedge clk) begin
	if (reset) begin
		cmp0_out_stage2_reg <= 16'b0;
		cmp1_out_stage2_reg <= 16'b0;
	end
	else if (~reset && mode1_stage2_run) begin
		cmp0_out_stage2_reg <= cmp0_out_stage2;
		cmp1_out_stage2_reg <= cmp1_out_stage2;
	end
end

comparator cmp0_stage1(.a(cmp0_out_stage2_reg),       .b(cmp1_out_stage2_reg),          .aeb(cmp0_stage1_aeb), .aneb(cmp0_stage1_aneb), .alb(cmp0_stage1_alb), .aleb(cmp0_stage1_aleb), .agb(cmp0_stage1_agb), .ageb(cmp0_stage1_ageb), .unordered(cmp0_stage1_unordered));
assign cmp0_out_stage1 = (cmp0_stage1_ageb==1'b1) ? cmp0_out_stage2_reg: cmp1_out_stage2_reg;

always @(posedge clk) begin
	if (reset) begin
		cmp0_out_stage1_reg <= 16'b0;
	end
	else if (~reset && mode1_stage1_run) begin
		cmp0_out_stage1_reg <= cmp0_out_stage1;

	end
end

comparator cmp0_stage0(.a(outp),       .b(cmp0_out_stage1_reg),         .aeb(cmp0_stage0_aeb), .aneb(cmp0_stage0_aneb), .alb(cmp0_stage0_alb), .aleb(cmp0_stage0_aleb), .agb(cmp0_stage0_agb), .ageb(cmp0_stage0_ageb), .unordered(cmp0_stage0_unordered));
assign cmp0_out_stage0 = (cmp0_stage0_ageb==1'b1) ? outp : cmp0_out_stage1_reg;

//DW_fp_cmp #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) cmp0_stage2(.a(inp0),       .b(inp1),      .z1(cmp0_out_stage2), .zctr(1'b0), .aeqb(), .altb(), .agtb(), .unordered(), .z0(), .status0(), .status1());
//DW_fp_cmp #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) cmp1_stage2(.a(inp2),       .b(inp3),      .z1(cmp1_out_stage2), .zctr(1'b0), .aeqb(), .altb(), .agtb(), .unordered(), .z0(), .status0(), .status1());
//
//DW_fp_cmp #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) cmp0_stage1(.a(cmp0_out_stage2),       .b(cmp1_out_stage2),      .z1(cmp0_out_stage1), .zctr(1'b0), .aeqb(), .altb(), .agtb(), .unordered(), .z0(), .status0(), .status1());
//
//DW_fp_cmp #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) cmp0_stage0(.a(outp),       .b(cmp0_out_stage1),      .z1(cmp0_out_stage0), .zctr(1'b0), .aeqb(), .altb(), .agtb(), .unordered(), .z0(), .status0(), .status1());

endmodule

	  
module mode2_sub(
  a_inp0,
  a_inp1,
  a_inp2,
  a_inp3,
  outp0,
  outp1,
  outp2,
  outp3,
  b_inp
);

  input  [`DATAWIDTH-1 : 0] a_inp0;
  input  [`DATAWIDTH-1 : 0] a_inp1;
  input  [`DATAWIDTH-1 : 0] a_inp2;
  input  [`DATAWIDTH-1 : 0] a_inp3;
  output  [`DATAWIDTH-1 : 0] outp0;
  output  [`DATAWIDTH-1 : 0] outp1;
  output  [`DATAWIDTH-1 : 0] outp2;
  output  [`DATAWIDTH-1 : 0] outp3;
  input  [`DATAWIDTH-1 : 0] b_inp;


  wire clk_NC;
  wire rst_NC;
  wire [4:0] flags_NC0, flags_NC1, flags_NC2, flags_NC3;

  // 0 add, 1 sub
  fixed_point_addsub sub0(.clk(clk_NC), .rst(rst_NC), .a(a_inp0),	.b(b_inp), .operation(1'b1),	.result(outp0), .flags(flags_NC0));
  fixed_point_addsub sub1(.clk(clk_NC), .rst(rst_NC), .a(a_inp1),	.b(b_inp), .operation(1'b1),	.result(outp1), .flags(flags_NC1));
  fixed_point_addsub sub2(.clk(clk_NC), .rst(rst_NC), .a(a_inp2),	.b(b_inp), .operation(1'b1),	.result(outp2), .flags(flags_NC2));
  fixed_point_addsub sub3(.clk(clk_NC), .rst(rst_NC), .a(a_inp3),	.b(b_inp), .operation(1'b1),	.result(outp3), .flags(flags_NC3));

//  DW_fp_sub #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) sub0(.a(a_inp0), .b(b_inp), .z(outp0), .rnd(3'b000), .status());
//  DW_fp_sub #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) sub1(.a(a_inp1), .b(b_inp), .z(outp1), .rnd(3'b000), .status());
//  DW_fp_sub #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) sub2(.a(a_inp2), .b(b_inp), .z(outp2), .rnd(3'b000), .status());
//  DW_fp_sub #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) sub3(.a(a_inp3), .b(b_inp), .z(outp3), .rnd(3'b000), .status());
endmodule


module mode3_exp(
  inp0, 
  inp1, 
  inp2, 
  inp3, 

  clk,
  reset,
  stage_run,
  stage_run2,
  
  outp0, 
  outp1, 
  outp2, 
  outp3
);

  input  [`DATAWIDTH-1 : 0] inp0;
  input  [`DATAWIDTH-1 : 0] inp1;
  input  [`DATAWIDTH-1 : 0] inp2;
  input  [`DATAWIDTH-1 : 0] inp3;

  input  clk;
  input  reset;
  input  stage_run;
  input  stage_run2;

  output  [`DATAWIDTH-1 : 0] outp0;
  output  [`DATAWIDTH-1 : 0] outp1;
  output  [`DATAWIDTH-1 : 0] outp2;
  output  [`DATAWIDTH-1 : 0] outp3;
  expunit exp0(.a(inp0), .z(outp0), .status(), .stage_run(stage_run), .stage_run2(stage_run2), .clk(clk), .reset(reset));
  expunit exp1(.a(inp1), .z(outp1), .status(), .stage_run(stage_run), .stage_run2(stage_run2), .clk(clk), .reset(reset));
  expunit exp2(.a(inp2), .z(outp2), .status(), .stage_run(stage_run), .stage_run2(stage_run2), .clk(clk), .reset(reset));
  expunit exp3(.a(inp3), .z(outp3), .status(), .stage_run(stage_run), .stage_run2(stage_run2), .clk(clk), .reset(reset));
endmodule


module mode4_adder_tree(
  inp0, 
  inp1, 
  inp2, 
  inp3, 
  mode4_stage0_run,
  mode4_stage1_run,
  mode4_stage2_run,

  clk,
  reset,
  outp
);

  input clk;
  input reset;
  input  [`DATAWIDTH-1 : 0] inp0; 
  input  [`DATAWIDTH-1 : 0] inp1; 
  input  [`DATAWIDTH-1 : 0] inp2; 
  input  [`DATAWIDTH-1 : 0] inp3; 
  output [`DATAWIDTH-1 : 0] outp;
  input mode4_stage0_run;
  input mode4_stage1_run;
  input mode4_stage2_run;

  wire   [`DATAWIDTH-1 : 0] add0_out_stage2;
  reg    [`DATAWIDTH-1 : 0] add0_out_stage2_reg;
  wire   [`DATAWIDTH-1 : 0] add1_out_stage2;
  reg    [`DATAWIDTH-1 : 0] add1_out_stage2_reg;

  wire   [`DATAWIDTH-1 : 0] add0_out_stage1;
  reg    [`DATAWIDTH-1 : 0] add0_out_stage1_reg;

  wire   [`DATAWIDTH-1 : 0] add0_out_stage0;
  reg    [`DATAWIDTH-1 : 0] outp;
/*
  always @(posedge clk) begin
    if (reset) begin
      outp <= 0;
      add0_out_stage2_reg <= 0;
      add1_out_stage2_reg <= 0;
      add0_out_stage1_reg <= 0;
    end

    if(~reset && mode4_stage2_run) begin
      add0_out_stage2_reg <= add0_out_stage2;
      add1_out_stage2_reg <= add1_out_stage2;
    end

    if(~reset && mode4_stage1_run) begin
      add0_out_stage1_reg <= add0_out_stage1;
    end

    if(~reset && mode4_stage0_run) begin
      outp <= add0_out_stage0;
    end
  end
*/
always @ (posedge clk) begin
	if(~reset && mode4_stage2_run) begin
      add0_out_stage2_reg <= add0_out_stage2;
      add1_out_stage2_reg <= add1_out_stage2;
    end
	
	else if (reset) begin
		add0_out_stage2_reg <= 0;
		add1_out_stage2_reg <= 0;
	end
end		

always @ (posedge clk) begin
	if(~reset && mode4_stage1_run) begin
      add0_out_stage1_reg <= add0_out_stage1;
    end
	else if (reset) begin
	add0_out_stage1_reg <= 0;
	end
end

always @ (posedge clk) begin
	if(~reset && mode4_stage0_run) begin
		outp <= add0_out_stage0;
    end
	else if (reset) begin
		outp <= 0;
	end
end	
		
	
  wire clk_NC;
  wire rst_NC;
  wire [4:0] flags_NC0, flags_NC1, flags_NC2, flags_NC3;

  // 0 add, 1 sub
  fixed_point_addsub add0_stage2(.clk(clk_NC), .rst(rst_NC), .a(inp0),	.b(inp1), .operation(1'b0),	.result(add0_out_stage2), .flags(flags_NC0));
  fixed_point_addsub add1_stage2(.clk(clk_NC), .rst(rst_NC), .a(inp2),	.b(inp3), .operation(1'b0),	.result(add1_out_stage2), .flags(flags_NC1));
  fixed_point_addsub add0_stage1(.clk(clk_NC), .rst(rst_NC), .a(add0_out_stage2_reg),	.b(add1_out_stage2_reg), .operation(1'b0),	.result(add0_out_stage1), .flags(flags_NC2));
  fixed_point_addsub add0_stage0(.clk(clk_NC), .rst(rst_NC), .a(outp),	.b(add0_out_stage1_reg), .operation(1'b0),	.result(add0_out_stage0), .flags(flags_NC3));

  //DW_fp_add #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) add0_stage2(.a(inp0),       .b(inp1),      .z(add0_out_stage2), .rnd(3'b000),    .status());
  //DW_fp_add #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) add1_stage2(.a(inp2),       .b(inp3),      .z(add1_out_stage2), .rnd(3'b000),    .status());

  //DW_fp_add #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) add0_stage1(.a(add0_out_stage2_reg),       .b(add1_out_stage2_reg),      .z(add0_out_stage1), .rnd(3'b000),    .status());

  //DW_fp_add #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) add0_stage0(.a(outp),       .b(add0_out_stage1_reg),      .z(add0_out_stage0), .rnd(3'b000),    .status());

endmodule

module mode5_ln(inp, outp, clk, reset, mode5_stage3_run, mode5_stage2_run, mode5_stage1_run);
  input  [`DATAWIDTH-1 : 0] inp;
  output [`DATAWIDTH-1 : 0] outp;
  input clk,reset;
  input mode5_stage3_run;
  input mode5_stage2_run;
  input mode5_stage1_run;	
  
  logunit ln(.fpin(inp), .fpout(outp), .status(), .clk(clk), .reset(reset), .mode5_stage3_run(mode5_stage3_run), .mode5_stage2_run(mode5_stage2_run), .mode5_stage1_run(mode5_stage1_run));
endmodule

module mode6_sub(
  a_inp0,
  a_inp1,
  a_inp2,
  a_inp3,
  b_inp,
  outp0,
  outp1,
  outp2,
  outp3
);

  input  [`DATAWIDTH-1 : 0] a_inp0;
  input  [`DATAWIDTH-1 : 0] a_inp1;
  input  [`DATAWIDTH-1 : 0] a_inp2;
  input  [`DATAWIDTH-1 : 0] a_inp3;
  input  [`DATAWIDTH-1 : 0] b_inp;
  output  [`DATAWIDTH-1 : 0] outp0;
  output  [`DATAWIDTH-1 : 0] outp1;
  output  [`DATAWIDTH-1 : 0] outp2;
  output  [`DATAWIDTH-1 : 0] outp3;

  wire [4:0] flags_NC0, flags_NC1, flags_NC2, flags_NC3;
  // 0 add, 1 sub
  wire clk_NC, rst_NC;
  fixed_point_addsub sub0(.clk(clk_NC), .rst(rst_NC), .a(a_inp0),	.b(b_inp), .operation(1'b1),	.result(outp0), .flags(flags_NC0));
  fixed_point_addsub sub1(.clk(clk_NC), .rst(rst_NC), .a(a_inp1),	.b(b_inp), .operation(1'b1),	.result(outp1), .flags(flags_NC1));
  fixed_point_addsub sub2(.clk(clk_NC), .rst(rst_NC), .a(a_inp2),	.b(b_inp), .operation(1'b1),	.result(outp2), .flags(flags_NC2));
  fixed_point_addsub sub3(.clk(clk_NC), .rst(rst_NC), .a(a_inp3),	.b(b_inp), .operation(1'b1),	.result(outp3), .flags(flags_NC3));

//  DW_fp_sub #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) sub0(.a(a_inp0), .b(b_inp), .z(outp0), .rnd(3'b000), .status());
//  DW_fp_sub #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) sub1(.a(a_inp1), .b(b_inp), .z(outp1), .rnd(3'b000), .status());
//  DW_fp_sub #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) sub2(.a(a_inp2), .b(b_inp), .z(outp2), .rnd(3'b000), .status());
//  DW_fp_sub #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) sub3(.a(a_inp3), .b(b_inp), .z(outp3), .rnd(3'b000), .status());
endmodule


module mode7_exp(
  inp0, 
  inp1, 
  inp2, 
  inp3, 

  clk,
  reset,
  stage_run,
  stage_run2,
  
  outp0, 
  outp1, 
  outp2, 
  outp3
);

  input  [`DATAWIDTH-1 : 0] inp0;
  input  [`DATAWIDTH-1 : 0] inp1;
  input  [`DATAWIDTH-1 : 0] inp2;
  input  [`DATAWIDTH-1 : 0] inp3;

  input  clk;
  input  reset;
  input  stage_run;
  input  stage_run2;

  output  [`DATAWIDTH-1 : 0] outp0;
  output  [`DATAWIDTH-1 : 0] outp1;
  output  [`DATAWIDTH-1 : 0] outp2;
  output  [`DATAWIDTH-1 : 0] outp3;
  expunit exp0(.a(inp0), .z(outp0), .status(), .stage_run(stage_run), .stage_run2(stage_run2), .clk(clk), .reset(reset));
  expunit exp1(.a(inp1), .z(outp1), .status(), .stage_run(stage_run), .stage_run2(stage_run2), .clk(clk), .reset(reset));
  expunit exp2(.a(inp2), .z(outp2), .status(), .stage_run(stage_run), .stage_run2(stage_run2), .clk(clk), .reset(reset));
  expunit exp3(.a(inp3), .z(outp3), .status(), .stage_run(stage_run), .stage_run2(stage_run2), .clk(clk), .reset(reset));
endmodule

//============================================================================
// Fixed point add/sub module
//============================================================================
module fixed_point_addsub(
		clk,
		rst,
		a,
		b,
		operation,			// 0 add, 1 sub
		result,
		flags
	);
	
	// Clock and reset
	input clk ;										// Clock signal
	input rst ;										// Reset (active high, resets pipeline registers)
	
	// Input ports
  input [`DATAWIDTH-1:0] a ;								// Input A, a 32-bit floating point number
  input [`DATAWIDTH-1:0] b ;								// Input B, a 32-bit floating point number
	input operation ;								// Operation select signal
	
	// Output ports
  output reg [`DATAWIDTH-1:0] result ;						// Result of the operation
	output [4:0] flags ;							// Flags indicating exceptions according to IEEE754
	
  reg [`DATAWIDTH:0] result_t ;
  wire [`DATAWIDTH-1:0] b_t ;
	
	assign b_t = ~b + 1;
	
	always@(*) begin
      if (operation == 1'b0) begin
			result_t = a + b;
		end
		else begin
			result_t = a + b_t;
		end
	end

	
	always @ (*) begin	
		if (result_t[16] == 1'b1 && operation == 1'b0) begin
			result = 16'h7000;
		end
		else if (result_t[16] == 1'b1 && operation == 1'b1) begin
			result = 16'h8000;
		end
		else begin
			result = result_t[15:0];
		end
	end
	// Pipeline Registers
	//reg [79:0] pipe_1;							// Pipeline register PreAlign->Align1
	
	
endmodule


//============================================================================
// Comparator module
//============================================================================


module comparator(
a,
b,
aeb,
aneb,
alb,
aleb,
agb,
ageb,
unordered
);

input [15:0] a;
input [15:0] b;
output aeb;
output aneb;
output alb;
output aleb;
output agb;
output ageb;
output unordered;

  

reg lt;
reg eq;
reg gt;

wire [15:0] a_t;
wire [15:0] b_t;

assign a_t = (~a[15:0])+1;
assign b_t = (~b[15:0])+1;

always @(*) begin
	if (a[15] == 1'b0 && b[15] == 1'b1) begin
		if (a != b) begin
		eq = 1'b0;
		lt = 1'b0;
		gt = 1'b1;
		end
		else begin
		eq = 1'b1;
		lt = 1'b0;
		gt = 1'b0;
		end
	end
	else if (a[15] == 1'b1 && b[15] == 1'b0) begin
		if (a != b) begin
		lt = 1'b1;
		gt = 1'b0;
		eq = 1'b0;
		end
		else begin
		lt = 1'b0;
		gt = 1'b0;
		eq = 1'b1;
		end
	end
	else if (a[15] == 1'b0 && b[15] == 1'b0) begin
		if (a > b) begin
		lt = 1'b0;
		gt = 1'b1;
		eq = 1'b0;
		end
		else if (a < b) begin
		lt = 1'b1;
		gt = 1'b0;
		eq = 1'b0;
		end
		else begin
		lt = 1'b0;
		gt = 1'b0;
		eq = 1'b1;
		end
	end
	else if (a[15] == 1'b1 && b[15] == 1'b1) begin
		if (a_t > b_t) begin
		lt = 1'b1;
		gt = 1'b0;
		eq = 1'b0;
		end
		else if (a_t < b_t) begin
		lt = 1'b0;
		gt = 1'b1;
		eq = 1'b0;
		end
		else begin
		lt = 1'b0;
		gt = 1'b0;
		eq = 1'b1;
		end
	end
end


//Result flags
assign aeb = eq;
assign aneb = ~eq;
assign alb = lt;
assign aleb = lt | eq;
assign agb = gt;
assign ageb = gt | eq;

endmodule



//////////////////////////////////////////////////////
// Log unit
// Author: Pragnesh Patel
//////////////////////////////////////////////////////


module logunit (fpin, fpout, status, clk, reset, mode5_stage3_run, mode5_stage2_run, mode5_stage1_run);

	input [15:0] fpin;
	output [15:0] fpout;
	output [7:0] status;
	input clk,reset;

	input mode5_stage3_run;
    input mode5_stage2_run;
    input mode5_stage1_run;	
   // input mode5_stage0_run;
  
	wire [15: 0] fxout1;
	wire [15: 0] fxout2;
    wire [15:0] fpin_f;
    wire [15:0] fpout_f;
	reg [15:0] fpin_f_reg;
    reg [15:0] fpout_f_reg;
	reg [15: 0] fxout1_reg;
	reg [15: 0] fxout2_reg;
	//reg [15: 0] pipe1;
	//reg [15: 0] pipe2;

  int_to_float_fp16 int_float (.input_a(fpin),.output_z(fpin_f));
  FPLUT1 lut1 (.addr(fpin_f_reg[14:10]),.log(fxout1)); 
  FP8LUT2 lut2 (.addr(fpin_f_reg[9:2]),.log(fxout2)); 
`ifdef complex_dsp
adder_fp_clk u_add(.clk(clk), .a(fxout1_reg), .b(fxout2_reg), .out(fpout_f));
`else
FPAddSub u_FPAddSub (.clk(clk), .rst(1'b0), .a(fxout1_reg), .b(fxout2_reg), .operation(1'b0), .result(fpout_f), .flags());
`endif
  
  float_to_int_fp16 float_int (.input_a(fpout_f_reg),.output_z(fpout));

    always @(posedge clk) begin
	/*if (reset) begin
		fpin_f_reg <= 16'b0;
	end */
	//if (~reset && mode5_stage2_run) begin
	if (mode5_stage3_run) begin
		fpin_f_reg <= fpin_f;
	end
    end

    always @(posedge clk) begin
	/*if (reset) begin
		fpin_f_reg <= 16'b0;
	end */
	//if (~reset && mode5_stage2_run) begin
	if (mode5_stage2_run) begin
		fxout2_reg <= fxout2;
		fxout1_reg <= fxout1;		
	end
    end
  
    always @(posedge clk) begin
	/*if (reset) begin
		fpout_f_reg <= 16'b0;
	end */
	//if (~reset && mode5_stage1_run) begin
	if (mode5_stage1_run) begin
		fpout_f_reg <= fpout_f;
	end
    end
endmodule

module float_to_int_fp16(
        input_a,
        output_z);


  input     [15:0] input_a;
  output    [15:0] output_z;

  
  wire [27:0] z;
  wire [5:0] a_e, sub_a_e;
  wire a_s;
  wire [15:0] a_m;
  wire [27:0] a_m_shift;  
  
  align_t dut_align (input_a,a_m,a_e,a_s);
  sub_t dut_sub (a_e,sub_a_e);
  am_shift_t dut_am_shift (a_e,sub_a_e,a_m,a_m_shift);
  two_comp_t dut_two_comp (a_m_shift,a_s,z);
  final_out_t dut_final_out (z, a_e, output_z);
  
endmodule

module align_t (
  input [15:0] input_a,
  output [15:0] a_m,
  output [5:0] a_e,
  output a_s);

  wire [15:0] a;

  assign a = input_a;
  assign a_m[15:5] = {1'b1, a[9 : 0]};
  assign a_m[4:0] = 8'b0;
  assign a_e = a[14 : 10] - 15;
  assign a_s = a[15];

endmodule

module sub_t (
  input [5:0] a_e,
  output [5:0] sub_a_e);

assign sub_a_e = 15 - a_e;

endmodule

module am_shift_t (
  input [5:0] a_e,
  input [5:0] sub_a_e,
  input [15:0] a_m,
  output reg [27:0] a_m_shift);

always@(a_e or sub_a_e or a_m) begin
  if (a_e <= 15 && a_e >= 0 ) begin
    a_m_shift = {a_m,12'b0} >> sub_a_e;
	end
	else begin
		a_m_shift = 24'h0;
	end
  end
  
endmodule

module two_comp_t (
  input [27:0] a_m_shift,
  input a_s,
  output [27:0] z);

assign z = a_s ? -a_m_shift : a_m_shift; // 2's complement

endmodule

module final_out_t (
  input [27:0] z,
  input [5:0] a_e,
  output [15:0] output_z);
  
  reg [27:0] output_z_temp;

always@(a_e or z) begin
  if (a_e[5] == 1'b1 && a_e[4:0] == 5'd15) begin
		output_z_temp = 27'b0;
	end
  else if (a_e[5] == 0 && a_e[4:0] > 5'd15) begin
		output_z_temp = 27'hFFFF;
	end
	else begin
		output_z_temp = z << 12;
	end
  end
  assign output_z = output_z_temp[27:12];
endmodule

module int_to_float_fp16(
        input_a,
        output_z);


  input     [15:0] input_a;
  output    [15:0] output_z;

  
  wire [15:0] value;
  wire z_s;
  wire [4:0] tmp_cnt;
  wire [4:0] sub_a_e;
  wire [4:0] sub_z_e;
  wire [15:0] a_m_shift;
  wire [10:0] z_m_final;
  wire [4:0] z_e_final;
  //wire [31:0] z;
  
  align dut_align (input_a,value,z_s);
  lzc dut_lzc (value,tmp_cnt);
  sub dut_sub (tmp_cnt,sub_a_e);
  sub2 dut_sub2 (sub_a_e,sub_z_e);
  am_shift dut_am_shift (value,sub_a_e,a_m_shift);
  exception dut_exception (a_m_shift,sub_z_e,z_m_final,z_e_final);
  final_out dut_final_out (input_a,z_m_final,z_e_final,z_s,output_z);

  
endmodule

module align (
  input [15:0] a,
  output [15:0] value,
output z_s);


  assign value = a[15] ? -a : a;
  assign z_s = a[15];

endmodule

/*module align2 (
input [31:0] value,
output [31:0] z_m,
//output [7:0] z_r,
//output [7:0] z_e);


	//z_e <= 8'd31;
    z_m <= value[31:0];
    //z_r <= value[7:0];

endmodule*/

module lzc (
  input [15:0] z_m,
  output reg [4:0] tmp_cnt_final);

  wire [15:0] Sj_int;
  //wire [15:0] val32;
wire [7:0] val8;
wire [3:0] val4;
  wire [4:0] tmp_cnt;

assign Sj_int = z_m;
  
assign    tmp_cnt[4] = 1'b0;
assign    tmp_cnt[3] = (Sj_int[15:8] == 8'b0);
assign    val8 = tmp_cnt[3] ? Sj_int[7:0] : Sj_int[15:8];
assign    tmp_cnt[2] = (val8[7:4] == 4'b0);
assign    val4 = tmp_cnt[2] ? val8[3:0] : val8[7:4];
assign    tmp_cnt[1] = (val4[3:2] == 2'b0);
assign    tmp_cnt[0] = tmp_cnt[1] ? ~val4[1] : ~val4[3];

always@(Sj_int or tmp_cnt)
begin
  if (Sj_int[15:0] == 16'b0)
   tmp_cnt_final = 5'd16;
else
   begin
   tmp_cnt_final = tmp_cnt;
end
end
endmodule

module sub (
  input [4:0] a_e,
  output [4:0] sub_a_e);

assign sub_a_e = a_e;

endmodule

module sub2 (
  input [4:0] a_e,
  output [4:0] sub_a_e);

assign sub_a_e = 15 - a_e;

endmodule

module am_shift (
  input [15:0] a_m,
  input [4:0] tmp_cnt,
  output [15:0] a_m_shift);

assign a_m_shift = a_m << tmp_cnt;  
endmodule

module exception (
  input [15:0] a_m_shift,
  input [4:0] z_e,
  output reg [10:0] z_m_final,
  output reg [4:0] z_e_final
);

wire guard;
wire round_bit;
wire sticky;
  wire [10:0] z_m;

  assign guard = a_m_shift[4];
  assign round_bit = a_m_shift[3];
  assign sticky = a_m_shift[2:0] != 0;

  assign z_m = a_m_shift[15:5];

always@(guard or round_bit or sticky or z_m or z_e)
begin
if (guard && (round_bit || sticky || z_m[0])) begin
    z_m_final = z_m + 1;
   if (z_m == 11'b11111111111) begin
            z_e_final = z_e + 1;
          end
		  else z_e_final = z_e;
          end
else begin 
  z_m_final = z_m;
  z_e_final = z_e;
end
end
endmodule

module final_out (
  input [15:0] a,
  input [10:0] z_m,
  input [4:0] z_e,
  input z_s,
  output reg [15:0] output_z);

  always@(a or z_m or z_e or z_s) begin
    if (a == 16'b0) begin
		output_z = 16'b0;
	end
	else begin
      output_z[9:0] = z_m[9:0];
      output_z[14:10] = z_e + 8'd3;
      output_z[15] = z_s;
	end
  end

endmodule

module FPLUT1(addr, log);
    input [4:0] addr;
    output reg [15:0] log;

    always @(addr) begin
        case (addr)
			5'b0 		: log = 16'b1111110000000000;
			5'b1 		: log = 16'b1100100011011010;
			5'b10 		: log = 16'b1100100010000001;
			5'b11 		: log = 16'b1100100000101001;
			5'b100 		: log = 16'b1100011110100000;
			5'b101 		: log = 16'b1100011011101110;
			5'b110 		: log = 16'b1100011000111101;
			5'b111 		: log = 16'b1100010110001100;
			5'b1000 		: log = 16'b1100010011011010;
			5'b1001 		: log = 16'b1100010000101001;
			5'b1010 		: log = 16'b1100001011101110;
			5'b1011 		: log = 16'b1100000110001100;
			5'b1100 		: log = 16'b1100000000101001;
			5'b1101 		: log = 16'b1011110110001100;
			5'b1110 		: log = 16'b1011100110001100;
			5'b1111 		: log = 16'b0000000000000000;
			5'b10000 		: log = 16'b0011100110001100;
			5'b10001 		: log = 16'b0011110110001100;
			5'b10010 		: log = 16'b0100000000101001;
			5'b10011 		: log = 16'b0100000110001100;
			5'b10100 		: log = 16'b0100001011101110;
			5'b10101 		: log = 16'b0100010000101001;
			5'b10110 		: log = 16'b0100010011011010;
			5'b10111 		: log = 16'b0100010110001100;
			5'b11000 		: log = 16'b0100011000111101;
			5'b11001 		: log = 16'b0100011011101110;
			5'b11010 		: log = 16'b0100011110100000;
			5'b11011 		: log = 16'b0100100000101001;
			5'b11100 		: log = 16'b0100100010000001;
			5'b11101 		: log = 16'b0100100011011010;
			5'b11110 		: log = 16'b0100100100110011;
			5'b11111 		: log = 16'b0111110000000000;
        endcase
    end
endmodule

module FP8LUT2(addr, log);
    input [7:0] addr;
    output reg [15:0] log;

    always @(addr) begin
        case (addr)
			8'b0 		: log = 16'b0000000000000000;
			8'b1 		: log = 16'b0001101111111100;
			8'b10 		: log = 16'b0001111111111000;
			8'b11 		: log = 16'b0010000111110111;
			8'b100 		: log = 16'b0010001111110000;
			8'b101 		: log = 16'b0010010011110100;
			8'b110 		: log = 16'b0010010111101110;
			8'b111 		: log = 16'b0010011011101000;
			8'b1000 		: log = 16'b0010011111100001;
			8'b1001 		: log = 16'b0010100001101100;
			8'b1010 		: log = 16'b0010100011101000;
			8'b1011 		: log = 16'b0010100101100011;
			8'b1100 		: log = 16'b0010100111011101;
			8'b1101 		: log = 16'b0010101001010111;
			8'b1110 		: log = 16'b0010101011010001;
			8'b1111 		: log = 16'b0010101101001010;
			8'b10000 		: log = 16'b0010101111000011;
			8'b10001 		: log = 16'b0010110000011101;
			8'b10010 		: log = 16'b0010110001011001;
			8'b10011 		: log = 16'b0010110010010101;
			8'b10100 		: log = 16'b0010110011010000;
			8'b10101 		: log = 16'b0010110100001100;
			8'b10110 		: log = 16'b0010110101000111;
			8'b10111 		: log = 16'b0010110110000010;
			8'b11000 		: log = 16'b0010110110111100;
			8'b11001 		: log = 16'b0010110111110111;
			8'b11010 		: log = 16'b0010111000110001;
			8'b11011 		: log = 16'b0010111001101011;
			8'b11100 		: log = 16'b0010111010100101;
			8'b11101 		: log = 16'b0010111011011110;
			8'b11110 		: log = 16'b0010111100011000;
			8'b11111 		: log = 16'b0010111101010001;
			8'b100000 		: log = 16'b0010111110001010;
			8'b100001 		: log = 16'b0010111111000011;
			8'b100010 		: log = 16'b0010111111111011;
			8'b100011 		: log = 16'b0011000000011010;
			8'b100100 		: log = 16'b0011000000110110;
			8'b100101 		: log = 16'b0011000001010010;
			8'b100110 		: log = 16'b0011000001101110;
			8'b100111 		: log = 16'b0011000010001010;
			8'b101000 		: log = 16'b0011000010100101;
			8'b101001 		: log = 16'b0011000011000001;
			8'b101010 		: log = 16'b0011000011011100;
			8'b101011 		: log = 16'b0011000011111000;
			8'b101100 		: log = 16'b0011000100010011;
			8'b101101 		: log = 16'b0011000100101111;
			8'b101110 		: log = 16'b0011000101001010;
			8'b101111 		: log = 16'b0011000101100101;
			8'b110000 		: log = 16'b0011000110000000;
			8'b110001 		: log = 16'b0011000110011011;
			8'b110010 		: log = 16'b0011000110110110;
			8'b110011 		: log = 16'b0011000111010000;
			8'b110100 		: log = 16'b0011000111101011;
			8'b110101 		: log = 16'b0011001000000101;
			8'b110110 		: log = 16'b0011001000100000;
			8'b110111 		: log = 16'b0011001000111010;
			8'b111000 		: log = 16'b0011001001010101;
			8'b111001 		: log = 16'b0011001001101111;
			8'b111010 		: log = 16'b0011001010001001;
			8'b111011 		: log = 16'b0011001010100011;
			8'b111100 		: log = 16'b0011001010111101;
			8'b111101 		: log = 16'b0011001011010111;
			8'b111110 		: log = 16'b0011001011110001;
			8'b111111 		: log = 16'b0011001100001010;
			8'b1000000 		: log = 16'b0011001100100100;
			8'b1000001 		: log = 16'b0011001100111110;
			8'b1000010 		: log = 16'b0011001101010111;
			8'b1000011 		: log = 16'b0011001101110000;
			8'b1000100 		: log = 16'b0011001110001010;
			8'b1000101 		: log = 16'b0011001110100011;
			8'b1000110 		: log = 16'b0011001110111100;
			8'b1000111 		: log = 16'b0011001111010101;
			8'b1001000 		: log = 16'b0011001111101110;
			8'b1001001 		: log = 16'b0011010000000100;
			8'b1001010 		: log = 16'b0011010000010000;
			8'b1001011 		: log = 16'b0011010000011100;
			8'b1001100 		: log = 16'b0011010000101001;
			8'b1001101 		: log = 16'b0011010000110101;
			8'b1001110 		: log = 16'b0011010001000001;
			8'b1001111 		: log = 16'b0011010001001110;
			8'b1010000 		: log = 16'b0011010001011010;
			8'b1010001 		: log = 16'b0011010001100110;
			8'b1010010 		: log = 16'b0011010001110010;
			8'b1010011 		: log = 16'b0011010001111110;
			8'b1010100 		: log = 16'b0011010010001010;
			8'b1010101 		: log = 16'b0011010010010110;
			8'b1010110 		: log = 16'b0011010010100010;
			8'b1010111 		: log = 16'b0011010010101110;
			8'b1011000 		: log = 16'b0011010010111010;
			8'b1011001 		: log = 16'b0011010011000110;
			8'b1011010 		: log = 16'b0011010011010010;
			8'b1011011 		: log = 16'b0011010011011110;
			8'b1011100 		: log = 16'b0011010011101010;
			8'b1011101 		: log = 16'b0011010011110101;
			8'b1011110 		: log = 16'b0011010100000001;
			8'b1011111 		: log = 16'b0011010100001101;
			8'b1100000 		: log = 16'b0011010100011000;
			8'b1100001 		: log = 16'b0011010100100100;
			8'b1100010 		: log = 16'b0011010100110000;
			8'b1100011 		: log = 16'b0011010100111011;
			8'b1100100 		: log = 16'b0011010101000111;
			8'b1100101 		: log = 16'b0011010101010010;
			8'b1100110 		: log = 16'b0011010101011110;
			8'b1100111 		: log = 16'b0011010101101001;
			8'b1101000 		: log = 16'b0011010101110100;
			8'b1101001 		: log = 16'b0011010110000000;
			8'b1101010 		: log = 16'b0011010110001011;
			8'b1101011 		: log = 16'b0011010110010110;
			8'b1101100 		: log = 16'b0011010110100010;
			8'b1101101 		: log = 16'b0011010110101101;
			8'b1101110 		: log = 16'b0011010110111000;
			8'b1101111 		: log = 16'b0011010111000011;
			8'b1110000 		: log = 16'b0011010111001110;
			8'b1110001 		: log = 16'b0011010111011010;
			8'b1110010 		: log = 16'b0011010111100101;
			8'b1110011 		: log = 16'b0011010111110000;
			8'b1110100 		: log = 16'b0011010111111011;
			8'b1110101 		: log = 16'b0011011000000110;
			8'b1110110 		: log = 16'b0011011000010001;
			8'b1110111 		: log = 16'b0011011000011100;
			8'b1111000 		: log = 16'b0011011000100111;
			8'b1111001 		: log = 16'b0011011000110001;
			8'b1111010 		: log = 16'b0011011000111100;
			8'b1111011 		: log = 16'b0011011001000111;
			8'b1111100 		: log = 16'b0011011001010010;
			8'b1111101 		: log = 16'b0011011001011101;
			8'b1111110 		: log = 16'b0011011001100111;
			8'b1111111 		: log = 16'b0011011001110010;
			8'b10000000 		: log = 16'b0011011001111101;
			8'b10000001 		: log = 16'b0011011010000111;
			8'b10000010 		: log = 16'b0011011010010010;
			8'b10000011 		: log = 16'b0011011010011101;
			8'b10000100 		: log = 16'b0011011010100111;
			8'b10000101 		: log = 16'b0011011010110010;
			8'b10000110 		: log = 16'b0011011010111100;
			8'b10000111 		: log = 16'b0011011011000111;
			8'b10001000 		: log = 16'b0011011011010001;
			8'b10001001 		: log = 16'b0011011011011100;
			8'b10001010 		: log = 16'b0011011011100110;
			8'b10001011 		: log = 16'b0011011011110000;
			8'b10001100 		: log = 16'b0011011011111011;
			8'b10001101 		: log = 16'b0011011100000101;
			8'b10001110 		: log = 16'b0011011100001111;
			8'b10001111 		: log = 16'b0011011100011010;
			8'b10010000 		: log = 16'b0011011100100100;
			8'b10010001 		: log = 16'b0011011100101110;
			8'b10010010 		: log = 16'b0011011100111000;
			8'b10010011 		: log = 16'b0011011101000011;
			8'b10010100 		: log = 16'b0011011101001101;
			8'b10010101 		: log = 16'b0011011101010111;
			8'b10010110 		: log = 16'b0011011101100001;
			8'b10010111 		: log = 16'b0011011101101011;
			8'b10011000 		: log = 16'b0011011101110101;
			8'b10011001 		: log = 16'b0011011101111111;
			8'b10011010 		: log = 16'b0011011110001001;
			8'b10011011 		: log = 16'b0011011110010011;
			8'b10011100 		: log = 16'b0011011110011101;
			8'b10011101 		: log = 16'b0011011110100111;
			8'b10011110 		: log = 16'b0011011110110001;
			8'b10011111 		: log = 16'b0011011110111011;
			8'b10100000 		: log = 16'b0011011111000101;
			8'b10100001 		: log = 16'b0011011111001110;
			8'b10100010 		: log = 16'b0011011111011000;
			8'b10100011 		: log = 16'b0011011111100010;
			8'b10100100 		: log = 16'b0011011111101100;
			8'b10100101 		: log = 16'b0011011111110110;
			8'b10100110 		: log = 16'b0011011111111111;
			8'b10100111 		: log = 16'b0011100000000100;
			8'b10101000 		: log = 16'b0011100000001001;
			8'b10101001 		: log = 16'b0011100000001110;
			8'b10101010 		: log = 16'b0011100000010011;
			8'b10101011 		: log = 16'b0011100000011000;
			8'b10101100 		: log = 16'b0011100000011101;
			8'b10101101 		: log = 16'b0011100000100001;
			8'b10101110 		: log = 16'b0011100000100110;
			8'b10101111 		: log = 16'b0011100000101011;
			8'b10110000 		: log = 16'b0011100000110000;
			8'b10110001 		: log = 16'b0011100000110100;
			8'b10110010 		: log = 16'b0011100000111001;
			8'b10110011 		: log = 16'b0011100000111110;
			8'b10110100 		: log = 16'b0011100001000010;
			8'b10110101 		: log = 16'b0011100001000111;
			8'b10110110 		: log = 16'b0011100001001100;
			8'b10110111 		: log = 16'b0011100001010001;
			8'b10111000 		: log = 16'b0011100001010101;
			8'b10111001 		: log = 16'b0011100001011010;
			8'b10111010 		: log = 16'b0011100001011110;
			8'b10111011 		: log = 16'b0011100001100011;
			8'b10111100 		: log = 16'b0011100001101000;
			8'b10111101 		: log = 16'b0011100001101100;
			8'b10111110 		: log = 16'b0011100001110001;
			8'b10111111 		: log = 16'b0011100001110110;
			8'b11000000 		: log = 16'b0011100001111010;
			8'b11000001 		: log = 16'b0011100001111111;
			8'b11000010 		: log = 16'b0011100010000011;
			8'b11000011 		: log = 16'b0011100010001000;
			8'b11000100 		: log = 16'b0011100010001100;
			8'b11000101 		: log = 16'b0011100010010001;
			8'b11000110 		: log = 16'b0011100010010101;
			8'b11000111 		: log = 16'b0011100010011010;
			8'b11001000 		: log = 16'b0011100010011110;
			8'b11001001 		: log = 16'b0011100010100011;
			8'b11001010 		: log = 16'b0011100010100111;
			8'b11001011 		: log = 16'b0011100010101100;
			8'b11001100 		: log = 16'b0011100010110000;
			8'b11001101 		: log = 16'b0011100010110101;
			8'b11001110 		: log = 16'b0011100010111001;
			8'b11001111 		: log = 16'b0011100010111110;
			8'b11010000 		: log = 16'b0011100011000010;
			8'b11010001 		: log = 16'b0011100011000110;
			8'b11010010 		: log = 16'b0011100011001011;
			8'b11010011 		: log = 16'b0011100011001111;
			8'b11010100 		: log = 16'b0011100011010100;
			8'b11010101 		: log = 16'b0011100011011000;
			8'b11010110 		: log = 16'b0011100011011100;
			8'b11010111 		: log = 16'b0011100011100001;
			8'b11011000 		: log = 16'b0011100011100101;
			8'b11011001 		: log = 16'b0011100011101001;
			8'b11011010 		: log = 16'b0011100011101110;
			8'b11011011 		: log = 16'b0011100011110010;
			8'b11011100 		: log = 16'b0011100011110110;
			8'b11011101 		: log = 16'b0011100011111011;
			8'b11011110 		: log = 16'b0011100011111111;
			8'b11011111 		: log = 16'b0011100100000011;
			8'b11100000 		: log = 16'b0011100100000111;
			8'b11100001 		: log = 16'b0011100100001100;
			8'b11100010 		: log = 16'b0011100100010000;
			8'b11100011 		: log = 16'b0011100100010100;
			8'b11100100 		: log = 16'b0011100100011000;
			8'b11100101 		: log = 16'b0011100100011101;
			8'b11100110 		: log = 16'b0011100100100001;
			8'b11100111 		: log = 16'b0011100100100101;
			8'b11101000 		: log = 16'b0011100100101001;
			8'b11101001 		: log = 16'b0011100100101101;
			8'b11101010 		: log = 16'b0011100100110010;
			8'b11101011 		: log = 16'b0011100100110110;
			8'b11101100 		: log = 16'b0011100100111010;
			8'b11101101 		: log = 16'b0011100100111110;
			8'b11101110 		: log = 16'b0011100101000010;
			8'b11101111 		: log = 16'b0011100101000110;
			8'b11110000 		: log = 16'b0011100101001011;
			8'b11110001 		: log = 16'b0011100101001111;
			8'b11110010 		: log = 16'b0011100101010011;
			8'b11110011 		: log = 16'b0011100101010111;
			8'b11110100 		: log = 16'b0011100101011011;
			8'b11110101 		: log = 16'b0011100101011111;
			8'b11110110 		: log = 16'b0011100101100011;
			8'b11110111 		: log = 16'b0011100101100111;
			8'b11111000 		: log = 16'b0011100101101011;
			8'b11111001 		: log = 16'b0011100101101111;
			8'b11111010 		: log = 16'b0011100101110011;
			8'b11111011 		: log = 16'b0011100101110111;
			8'b11111100 		: log = 16'b0011100101111100;
			8'b11111101 		: log = 16'b0011100110000000;
			8'b11111110 		: log = 16'b0011100110000100;
			8'b11111111 		: log = 16'b0011100110001000;
        endcase
    end
endmodule


//////////////////////////////////////////////////////
// Exponential unit
// Author: Pragnesh Patel
//////////////////////////////////////////////////////

module expunit (a, z, status, stage_run, stage_run2, clk, reset);

	input [15:0] a;
    input stage_run;
	input stage_run2;
    input clk;
    input reset;
    output reg [15:0] z;
    output [7:0] status;
	
    reg  [31:0] LUTout_reg;
	reg  [31:0] LUTout_reg2;
    reg  [15:0] a_reg;
	reg  [15:0] a_reg2;
	reg  [15:0] a_comp_reg;
	reg  [31:0] Mult_out_reg;
    wire [31:0] LUTout;
    wire [31:0] Mult_out;
    wire [15:0] a_comp;
    wire [15:0] z_out;
  	
    
	always @(posedge clk) begin
    if(reset) begin
      LUTout_reg2 <= 0;
	  a_reg2 <= 0;
	  a_comp_reg <= 0;
    end 
	else if(stage_run2) begin
      LUTout_reg2 <= LUTout;
	  a_reg2 <= a;
	  a_comp_reg <= a_comp;
    end
    end
	
	always @(posedge clk) begin
    if(reset) begin
      Mult_out_reg <= 0;
      LUTout_reg <= 0;
	  a_reg <= 0;
    end 
	else if(stage_run) begin
      Mult_out_reg <= Mult_out;
      LUTout_reg <= LUTout_reg2;
	  a_reg <= a_reg2;
    end
    end

    assign a_comp = ~a + 1'b1;
    ExpLUT lut(.addr(a_comp[14:8]), .exp(LUTout)); 
    assign Mult_out = ~(a_comp_reg*LUTout_reg2[31:16])+1;
    assign z_out = Mult_out_reg[27:12] + LUTout_reg[15:0];

    always@(z_out or a_reg) begin
      if (a_reg[15:12] == 4'b1000) begin
        z = 12'b1;
      end
      else
        z = z_out;
    end
  
endmodule

module ExpLUT(addr, exp);
    input [6:0] addr;
    output reg [31:0] exp;

    always @(addr) begin
        case (addr)
	     7'b0000000            : exp =  32'b00001111100000100001000000000000;
	     7'b0000001            : exp =  32'b00001110100100100000111111110000;
	     7'b0000010            : exp =  32'b00001101101100000000111111010100;
	     7'b0000011            : exp =  32'b00001100110110110000111110101100;
	     7'b0000100            : exp =  32'b00001100000101000000111101111011;
	     7'b0000101            : exp =  32'b00001011010110000000111101000000;
	     7'b0000110            : exp =  32'b00001010101010000000111011111110;
	     7'b0000111            : exp =  32'b00001010000000110000111010110110;
	     7'b0001000            : exp =  32'b00001001011010000000111001101000;
	     7'b0001001            : exp =  32'b00001000110101100000111000010110;
	     7'b0001010            : exp =  32'b00001000010011010000110111000000;
	     7'b0001011            : exp =  32'b00000111110011000000110101101000;
	     7'b0001100            : exp =  32'b00000111010100110000110100001101;
	     7'b0001101            : exp =  32'b00000110111000010000110010110001;
	     7'b0001110            : exp =  32'b00000110011101110000110001010011;
	     7'b0001111            : exp =  32'b00000110000100100000101111110101;
	     7'b0010000            : exp =  32'b00000101101101000000101110010111;
	     7'b0010001            : exp =  32'b00000101010111000000101100111001;
	     7'b0010010            : exp =  32'b00000101000010010000101011011011;
	     7'b0010011            : exp =  32'b00000100101110100000101001111111;
	     7'b0010100            : exp =  32'b00000100011100010000101000100011;
	     7'b0010101            : exp =  32'b00000100001011000000100111001001;
	     7'b0010110            : exp =  32'b00000011111010110000100101110000;
	     7'b0010111            : exp =  32'b00000011101011110000100100011000;
	     7'b0011000            : exp =  32'b00000011011101010000100011000010;
	     7'b0011001            : exp =  32'b00000011010000000000100001101111;
	     7'b0011010            : exp =  32'b00000011000011010000100000011101;
	     7'b0011011            : exp =  32'b00000010110111100000011111001101;
	     7'b0011100            : exp =  32'b00000010101100010000011101111111;
	     7'b0011101            : exp =  32'b00000010100010000000011100110011;
	     7'b0011110            : exp =  32'b00000010011000000000011011101001;
	     7'b0011111            : exp =  32'b00000010001111000000011010100010;
	     7'b0100000            : exp =  32'b00000010000110010000011001011101;
	     7'b0100001            : exp =  32'b00000001111110000000011000011001;
	     7'b0100010            : exp =  32'b00000001110110100000010111011000;
	     7'b0100011            : exp =  32'b00000001101111010000010110011010;
	     7'b0100100            : exp =  32'b00000001101000100000010101011101;
	     7'b0100101            : exp =  32'b00000001100010010000010100100010;
	     7'b0100110            : exp =  32'b00000001011100010000010011101010;
	     7'b0100111            : exp =  32'b00000001010110100000010010110011;
	     7'b0101000            : exp =  32'b00000001010001010000010001111111;
	     7'b0101001            : exp =  32'b00000001001100100000010001001100;
	     7'b0101010            : exp =  32'b00000001000111110000010000011011;
	     7'b0101011            : exp =  32'b00000001000011100000001111101100;
	     7'b0101100            : exp =  32'b00000000111111010000001110111111;
	     7'b0101101            : exp =  32'b00000000111011100000001110010100;
	     7'b0101110            : exp =  32'b00000000111000000000001101101011;
	     7'b0101111            : exp =  32'b00000000110100100000001101000011;
	     7'b0110000            : exp =  32'b00000000110001010000001100011100;
	     7'b0110001            : exp =  32'b00000000101110010000001011111000;
	     7'b0110010            : exp =  32'b00000000101011100000001011010101;
	     7'b0110011            : exp =  32'b00000000101000110000001010110011;
	     7'b0110100            : exp =  32'b00000000100110010000001010010011;
	     7'b0110101            : exp =  32'b00000000100100000000001001110100;
	     7'b0110110            : exp =  32'b00000000100001110000001001010110;
	     7'b0110111            : exp =  32'b00000000011111110000001000111010;
	     7'b0111000            : exp =  32'b00000000011101110000001000011111;
	     7'b0111001            : exp =  32'b00000000011100000000001000000101;
	     7'b0111010            : exp =  32'b00000000011010010000000111101100;
	     7'b0111011            : exp =  32'b00000000011000110000000111010101;
	     7'b0111100            : exp =  32'b00000000010111010000000110111110;
	     7'b0111101            : exp =  32'b00000000010101110000000110101000;
	     7'b0111110            : exp =  32'b00000000010100100000000110010100;
	     7'b0111111            : exp =  32'b00000000010011010000000110000000;
	     7'b1000000            : exp =  32'b00000000010010000000000101101101;
	     7'b1000001            : exp =  32'b00000000010001000000000101011100;
	     7'b1000010            : exp =  32'b00000000010000000000000101001010;
	     7'b1000011            : exp =  32'b00000000001111000000000100111010;
	     7'b1000100            : exp =  32'b00000000001110000000000100101011;
	     7'b1000101            : exp =  32'b00000000001101010000000100011100;
	     7'b1000110            : exp =  32'b00000000001100010000000100001110;
	     7'b1000111            : exp =  32'b00000000001011100000000100000000;
	     7'b1001000            : exp =  32'b00000000001011000000000011110011;
	     7'b1001001            : exp =  32'b00000000001010010000000011100111;
	     7'b1001010            : exp =  32'b00000000001001100000000011011100;
	     7'b1001011            : exp =  32'b00000000001001000000000011010001;
	     7'b1001100            : exp =  32'b00000000001000100000000011000110;
	     7'b1001101            : exp =  32'b00000000001000000000000010111100;
	     7'b1001110            : exp =  32'b00000000000111100000000010110011;
	     7'b1001111            : exp =  32'b00000000000111000000000010101001;
	     7'b1010000            : exp =  32'b00000000000110100000000010100001;
	     7'b1010001            : exp =  32'b00000000000110010000000010011001;
	     7'b1010010            : exp =  32'b00000000000101110000000010010001;
	     7'b1010011            : exp =  32'b00000000000101100000000010001001;
	     7'b1010100            : exp =  32'b00000000000101000000000010000010;
	     7'b1010101            : exp =  32'b00000000000100110000000001111100;
	     7'b1010110            : exp =  32'b00000000000100100000000001110101;
	     7'b1010111            : exp =  32'b00000000000100010000000001101111;
	     7'b1011000            : exp =  32'b00000000000100000000000001101001;
	     7'b1011001            : exp =  32'b00000000000011110000000001100100;
	     7'b1011010            : exp =  32'b00000000000011100000000001011111;
	     7'b1011011            : exp =  32'b00000000000011010000000001011010;
	     7'b1011100            : exp =  32'b00000000000011000000000001010101;
	     7'b1011101            : exp =  32'b00000000000010110000000001010001;
	     7'b1011110            : exp =  32'b00000000000010110000000001001101;
	     7'b1011111            : exp =  32'b00000000000010100000000001001001;
	     7'b1100000            : exp =  32'b00000000000010010000000001000101;
	     7'b1100001            : exp =  32'b00000000000010010000000001000001;
	     7'b1100010            : exp =  32'b00000000000010000000000000111110;
	     7'b1100011            : exp =  32'b00000000000010000000000000111010;
	     7'b1100100            : exp =  32'b00000000000001110000000000110111;
	     7'b1100101            : exp =  32'b00000000000001110000000000110100;
	     7'b1100110            : exp =  32'b00000000000001100000000000110010;
	     7'b1100111            : exp =  32'b00000000000001100000000000101111;
	     7'b1101000            : exp =  32'b00000000000001010000000000101100;
	     7'b1101001            : exp =  32'b00000000000001010000000000101010;
	     7'b1101010            : exp =  32'b00000000000001010000000000101000;
	     7'b1101011            : exp =  32'b00000000000001000000000000100110;
	     7'b1101100            : exp =  32'b00000000000001000000000000100100;
	     7'b1101101            : exp =  32'b00000000000001000000000000100010;
	     7'b1101110            : exp =  32'b00000000000001000000000000100000;
	     7'b1101111            : exp =  32'b00000000000000110000000000011110;
	     7'b1110000            : exp =  32'b00000000000000110000000000011101;
	     7'b1110001            : exp =  32'b00000000000000110000000000011011;
	     7'b1110010            : exp =  32'b00000000000000110000000000011010;
	     7'b1110011            : exp =  32'b00000000000000110000000000011000;
	     7'b1110100            : exp =  32'b00000000000000100000000000010111;
	     7'b1110101            : exp =  32'b00000000000000100000000000010110;
	     7'b1110110            : exp =  32'b00000000000000100000000000010100;
	     7'b1110111            : exp =  32'b00000000000000100000000000010011;
	     7'b1111000            : exp =  32'b00000000000000100000000000010010;
	     7'b1111001            : exp =  32'b00000000000000100000000000010001;
	     7'b1111010            : exp =  32'b00000000000000010000000000010000;
	     7'b1111011            : exp =  32'b00000000000000010000000000001111;
	     7'b1111100            : exp =  32'b00000000000000010000000000001111;
	     7'b1111101            : exp =  32'b00000000000000010000000000001110;
	     7'b1111110            : exp =  32'b00000000000000010000000000001101;
	     7'b1111111            : exp =  32'b00000000000000010000000000001100;
        endcase
    end
endmodule

`ifdef SIMULATION_addfp
module adder_fp(
  input clk,
	input [15:0] a,
	input [15:0] b,
	output [15:0] out
);
	assign out = a+b;
endmodule
`endif


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Definition of a 16-bit floating point adder/subtractor
// This is a heavily modified version of:
// https://github.com/fbrosser/DSP48E1-FP/tree/master/src/FP_AddSub
// Original author: Fredrik Brosser
// Abridged by: Samidh Mehta
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

`ifndef complex_dsp

module FPAddSub(
		//bf16,
		clk,
		rst,
		a,
		b,
		operation,			// 0 add, 1 sub
		result,
		flags
	);
	//input bf16; //1 for Bfloat16, 0 for IEEE half precision

	// Clock and reset
	input clk ;										// Clock signal
	input rst ;										// Reset (active high, resets pipeline registers)
	
	// Input ports
	input [`DWIDTH-1:0] a ;								// Input A, a 32-bit floating point number
	input [`DWIDTH-1:0] b ;								// Input B, a 32-bit floating point number
	input operation ;								// Operation select signal
	
	// Output ports
	output [`DWIDTH-1:0] result ;						// Result of the operation
	output [4:0] flags ;							// Flags indicating exceptions according to IEEE754
	
	// Pipeline Registers
	//reg [79:0] pipe_1;							// Pipeline register PreAlign->Align1
	reg [2*`EXPONENT + 2*`DWIDTH + 5:0] pipe_1;							// Pipeline register PreAlign->Align1

	//reg [67:0] pipe_2;							// Pipeline register Align1->Align3
	//reg [2*`EXPONENT+ 2*`MANTISSA + 8:0] pipe_2;							// Pipeline register Align1->Align3
	wire [2*`EXPONENT+ 2*`MANTISSA + 8:0] pipe_2;

	//reg [76:0] pipe_3;	68						// Pipeline register Align1->Align3
	reg [2*`EXPONENT+ 2*`MANTISSA + 9:0] pipe_3;							// Pipeline register Align1->Align3

	//reg [69:0] pipe_4;							// Pipeline register Align3->Execute
	//reg [2*`EXPONENT+ 2*`MANTISSA + 9:0] pipe_4;							// Pipeline register Align3->Execute
	wire [2*`EXPONENT+ 2*`MANTISSA + 9:0] pipe_4;
	
	//reg [51:0] pipe_5;							// Pipeline register Execute->Normalize
	reg [`DWIDTH+`EXPONENT+11:0] pipe_5;							// Pipeline register Execute->Normalize

	//reg [56:0] pipe_6;							// Pipeline register Nomalize->NormalizeShift1
	//reg [`DWIDTH+`EXPONENT+16:0] pipe_6;							// Pipeline register Nomalize->NormalizeShift1
	wire [`DWIDTH+`EXPONENT+16:0] pipe_6;

	//reg [56:0] pipe_7;							// Pipeline register NormalizeShift2->NormalizeShift3
	//reg [`DWIDTH+`EXPONENT+16:0] pipe_7;							// Pipeline register NormalizeShift2->NormalizeShift3
	wire [`DWIDTH+`EXPONENT+16:0] pipe_7;
	//reg [54:0] pipe_8;							// Pipeline register NormalizeShift3->Round
	reg [`EXPONENT*2+`MANTISSA+15:0] pipe_8;							// Pipeline register NormalizeShift3->Round

	//reg [40:0] pipe_9;							// Pipeline register NormalizeShift3->Round
	//reg [`DWIDTH+8:0] pipe_9;							// Pipeline register NormalizeShift3->Round
	wire [`DWIDTH+8:0] pipe_9;

	// Internal wires between modules
	wire [`DWIDTH-2:0] Aout_0 ;							// A - sign
	wire [`DWIDTH-2:0] Bout_0 ;							// B - sign
	wire Opout_0 ;									// A's sign
	wire Sa_0 ;										// A's sign
	wire Sb_0 ;										// B's sign
	wire MaxAB_1 ;									// Indicates the larger of A and B(0/A, 1/B)
	wire [`EXPONENT-1:0] CExp_1 ;							// Common Exponent
	wire [`EXPONENT-1:0] Shift_1 ;							// Number of steps to smaller mantissa shift right (align)
	wire [`MANTISSA-1:0] Mmax_1 ;							// Larger mantissa
	wire [4:0] InputExc_0 ;						// Input numbers are exceptions
	wire [2*`EXPONENT-1:0] ShiftDet_0 ;
	wire [`MANTISSA-1:0] MminS_1 ;						// Smaller mantissa after 0/16 shift
	wire [`MANTISSA:0] MminS_2 ;						// Smaller mantissa after 0/4/8/12 shift
	wire [`MANTISSA:0] Mmin_3 ;							// Smaller mantissa after 0/1/2/3 shift
	wire [`DWIDTH:0] Sum_4 ;
	wire PSgn_4 ;
	wire Opr_4 ;
	wire [`EXPONENT-1:0] Shift_5 ;							// Number of steps to shift sum left (normalize)
	wire [`DWIDTH:0] SumS_5 ;							// Sum after 0/16 shift
	wire [`DWIDTH:0] SumS_6 ;							// Sum after 0/16 shift
	wire [`DWIDTH:0] SumS_7 ;							// Sum after 0/16 shift
	wire [`MANTISSA-1:0] NormM_8 ;						// Normalized mantissa
	wire [`EXPONENT:0] NormE_8;							// Adjusted exponent
	wire ZeroSum_8 ;								// Zero flag
	wire NegE_8 ;									// Flag indicating negative exponent
	wire R_8 ;										// Round bit
	wire S_8 ;										// Final sticky bit
	wire FG_8 ;										// Final sticky bit
	wire [`DWIDTH-1:0] P_int ;
	wire EOF ;
	
	// Prepare the operands for alignment and check for exceptions
	FPAddSub_PrealignModule PrealignModule
	(	// Inputs
		a, b, operation,
		// Outputs
		Sa_0, Sb_0, ShiftDet_0[2*`EXPONENT-1:0], InputExc_0[4:0], Aout_0[`DWIDTH-2:0], Bout_0[`DWIDTH-2:0], Opout_0) ;
		
	// Prepare the operands for alignment and check for exceptions
	FPAddSub_AlignModule AlignModule
	(	// Inputs
		pipe_1[2*`EXPONENT + 2*`DWIDTH + 4: 2*`EXPONENT +`DWIDTH + 6], pipe_1[2*`EXPONENT +`DWIDTH + 5 :  2*`EXPONENT +7], pipe_1[2*`EXPONENT+4:5],
		// Outputs
		CExp_1[`EXPONENT-1:0], MaxAB_1, Shift_1[`EXPONENT-1:0], MminS_1[`MANTISSA-1:0], Mmax_1[`MANTISSA-1:0]) ;	

	// Alignment Shift Stage 1
	FPAddSub_AlignShift1 AlignShift1
	(  // Inputs
		//bf16, 
		pipe_2[`MANTISSA-1:0], pipe_2[`EXPONENT+ 2*`MANTISSA + 4 : 2*`MANTISSA + 7],
		// Outputs
		MminS_2[`MANTISSA:0]) ;

	// Alignment Shift Stage 3 and compution of guard and sticky bits
	FPAddSub_AlignShift2 AlignShift2  
	(  // Inputs
		pipe_3[`MANTISSA:0], pipe_3[2*`MANTISSA+7:2*`MANTISSA+6],
		// Outputs
		Mmin_3[`MANTISSA:0]) ;
						
	// Perform mantissa addition
	FPAddSub_ExecutionModule ExecutionModule
	(  // Inputs
		pipe_4[`MANTISSA*2+5:`MANTISSA+6], pipe_4[`MANTISSA:0], pipe_4[2*`EXPONENT+ 2*`MANTISSA + 8], pipe_4[2*`EXPONENT+ 2*`MANTISSA + 7], pipe_4[2*`EXPONENT+ 2*`MANTISSA + 6], pipe_4[2*`EXPONENT+ 2*`MANTISSA + 9],
		// Outputs
		Sum_4[`DWIDTH:0], PSgn_4, Opr_4) ;
	
	// Prepare normalization of result
	FPAddSub_NormalizeModule NormalizeModule
	(  // Inputs
		pipe_5[`DWIDTH:0], 
		// Outputs
		SumS_5[`DWIDTH:0], Shift_5[4:0]) ;
					
	// Normalization Shift Stage 1
	FPAddSub_NormalizeShift1 NormalizeShift1
	(  // Inputs
		pipe_6[`DWIDTH:0], pipe_6[`DWIDTH+`EXPONENT+14:`DWIDTH+`EXPONENT+11],
		// Outputs
		SumS_7[`DWIDTH:0]) ;
		
	// Normalization Shift Stage 3 and final guard, sticky and round bits
	FPAddSub_NormalizeShift2 NormalizeShift2
	(  // Inputs
		pipe_7[`DWIDTH:0], pipe_7[`DWIDTH+`EXPONENT+5:`DWIDTH+6], pipe_7[`DWIDTH+`EXPONENT+15:`DWIDTH+`EXPONENT+11],
		// Outputs
		NormM_8[`MANTISSA-1:0], NormE_8[`EXPONENT:0], ZeroSum_8, NegE_8, R_8, S_8, FG_8) ;

	// Round and put result together
	FPAddSub_RoundModule RoundModule
	(  // Inputs
		 pipe_8[3], pipe_8[4+`EXPONENT:4], pipe_8[`EXPONENT+`MANTISSA+4:5+`EXPONENT], pipe_8[1], pipe_8[0], pipe_8[`EXPONENT*2+`MANTISSA+15], pipe_8[`EXPONENT*2+`MANTISSA+12], pipe_8[`EXPONENT*2+`MANTISSA+11], pipe_8[`EXPONENT*2+`MANTISSA+14], pipe_8[`EXPONENT*2+`MANTISSA+10], 
		// Outputs
		P_int[`DWIDTH-1:0], EOF) ;
	
	// Check for exceptions
	FPAddSub_ExceptionModule Exceptionmodule
	(  // Inputs
		pipe_9[8+`DWIDTH:9], pipe_9[8], pipe_9[7], pipe_9[6], pipe_9[5:1], pipe_9[0], 
		// Outputs
		result[`DWIDTH-1:0], flags[4:0]) ;			
	

assign pipe_2 = {pipe_1[2*`EXPONENT + 2*`DWIDTH + 5], pipe_1[2*`EXPONENT +6:2*`EXPONENT +5], MaxAB_1, CExp_1[`EXPONENT-1:0], Shift_1[`EXPONENT-1:0], Mmax_1[`MANTISSA-1:0], pipe_1[4:0], MminS_1[`MANTISSA-1:0]} ;
assign pipe_4 = {pipe_3[2*`EXPONENT+ 2*`MANTISSA + 9:`MANTISSA+1], Mmin_3[`MANTISSA:0]} ;
assign pipe_6 = {pipe_5[`DWIDTH+`EXPONENT+11], Shift_5[4:0], pipe_5[`DWIDTH+`EXPONENT+10:`DWIDTH+1], SumS_5[`DWIDTH:0]} ;
assign pipe_7 = {pipe_6[`DWIDTH+`EXPONENT+16:`DWIDTH+1], SumS_7[`DWIDTH:0]} ;
assign pipe_9 = {P_int[`DWIDTH-1:0], pipe_8[2], pipe_8[1], pipe_8[0], pipe_8[`EXPONENT+`MANTISSA+9:`EXPONENT+`MANTISSA+5], EOF} ;

	always @ (posedge clk) begin	
		if(rst) begin
			pipe_1 <= 0;
			//pipe_2 <= 0;
			pipe_3 <= 0;
			//pipe_4 <= 0;
			pipe_5 <= 0;
			//pipe_6 <= 0;
			//pipe_7 <= 0;
			pipe_8 <= 0;
			//pipe_9 <= 0;
		end 
		else begin
/* PIPE_1:
	[2*`EXPONENT + 2*`DWIDTH + 5]  Opout_0
	[2*`EXPONENT + 2*`DWIDTH + 4: 2*`EXPONENT +`DWIDTH + 6] A_out0
	[2*`EXPONENT +`DWIDTH + 5 :  2*`EXPONENT +7] Bout_0
	[2*`EXPONENT +6] Sa_0
	[2*`EXPONENT +5] Sb_0
	[2*`EXPONENT +4 : 5] ShiftDet_0
	[4:0] Input Exc
*/
			pipe_1 <= {Opout_0, Aout_0[`DWIDTH-2:0], Bout_0[`DWIDTH-2:0], Sa_0, Sb_0, ShiftDet_0[2*`EXPONENT -1:0], InputExc_0[4:0]} ;	
/* PIPE_2
[2*`EXPONENT+ 2*`MANTISSA + 8] operation
[2*`EXPONENT+ 2*`MANTISSA + 7] Sa_0
[2*`EXPONENT+ 2*`MANTISSA + 6] Sb_0
[2*`EXPONENT+ 2*`MANTISSA + 5] MaxAB_0
[2*`EXPONENT+ 2*`MANTISSA + 4:`EXPONENT+ 2*`MANTISSA + 5] CExp_0
[`EXPONENT+ 2*`MANTISSA + 4 : 2*`MANTISSA + 5] Shift_0
[2*`MANTISSA + 4:`MANTISSA + 5] Mmax_0
[`MANTISSA + 4 : `MANTISSA] InputExc_0
[`MANTISSA-1:0] MminS_1
*/
			//pipe_2 <= {pipe_1[2*`EXPONENT + 2*`DWIDTH + 5], pipe_1[2*`EXPONENT +6:2*`EXPONENT +5], MaxAB_1, CExp_1[`EXPONENT-1:0], Shift_1[`EXPONENT-1:0], Mmax_1[`MANTISSA-1:0], pipe_1[4:0], MminS_1[`MANTISSA-1:0]} ;	
/* PIPE_3
[2*`EXPONENT+ 2*`MANTISSA + 9] operation
[2*`EXPONENT+ 2*`MANTISSA + 8] Sa_0
[2*`EXPONENT+ 2*`MANTISSA + 7] Sb_0
[2*`EXPONENT+ 2*`MANTISSA + 6] MaxAB_0
[2*`EXPONENT+ 2*`MANTISSA + 5:`EXPONENT+ 2*`MANTISSA + 6] CExp_0
[`EXPONENT+ 2*`MANTISSA + 5 : 2*`MANTISSA + 6] Shift_0
[2*`MANTISSA + 5:`MANTISSA + 6] Mmax_0
[`MANTISSA + 5 : `MANTISSA + 1] InputExc_0
[`MANTISSA:0] MminS_2
*/
			pipe_3 <= {pipe_2[2*`EXPONENT+ 2*`MANTISSA + 8:`MANTISSA], MminS_2[`MANTISSA:0]} ;	
/* PIPE_4
[2*`EXPONENT+ 2*`MANTISSA + 9] operation
[2*`EXPONENT+ 2*`MANTISSA + 8] Sa_0
[2*`EXPONENT+ 2*`MANTISSA + 7] Sb_0
[2*`EXPONENT+ 2*`MANTISSA + 6] MaxAB_0
[2*`EXPONENT+ 2*`MANTISSA + 5:`EXPONENT+ 2*`MANTISSA + 6] CExp_0
[`EXPONENT+ 2*`MANTISSA + 5 : 2*`MANTISSA + 6] Shift_0
[2*`MANTISSA + 5:`MANTISSA + 6] Mmax_0
[`MANTISSA + 5 : `MANTISSA + 1] InputExc_0
[`MANTISSA:0] MminS_3
*/				
			//pipe_4 <= {pipe_3[2*`EXPONENT+ 2*`MANTISSA + 9:`MANTISSA+1], Mmin_3[`MANTISSA:0]} ;	
/* PIPE_5 :
[`DWIDTH+ `EXPONENT + 11] operation
[`DWIDTH+ `EXPONENT + 10] PSgn_4
[`DWIDTH+ `EXPONENT + 9] Opr_4
[`DWIDTH+ `EXPONENT + 8] Sa_0
[`DWIDTH+ `EXPONENT + 7] Sb_0
[`DWIDTH+ `EXPONENT + 6] MaxAB_0
[`DWIDTH+ `EXPONENT + 5 :`DWIDTH+6] CExp_0
[`DWIDTH+5:`DWIDTH+1] InputExc_0
[`DWIDTH:0] Sum_4
*/					
			pipe_5 <= {pipe_4[2*`EXPONENT+ 2*`MANTISSA + 9], PSgn_4, Opr_4, pipe_4[2*`EXPONENT+ 2*`MANTISSA + 8:`EXPONENT+ 2*`MANTISSA + 6], pipe_4[`MANTISSA+5:`MANTISSA+1], Sum_4[`DWIDTH:0]} ;
/* PIPE_6 :
[`DWIDTH+ `EXPONENT + 16] operation
[`DWIDTH+ `EXPONENT + 15:`DWIDTH+ `EXPONENT + 11] Shift_5
[`DWIDTH+ `EXPONENT + 10] PSgn_4
[`DWIDTH+ `EXPONENT + 9] Opr_4
[`DWIDTH+ `EXPONENT + 8] Sa_0
[`DWIDTH+ `EXPONENT + 7] Sb_0
[`DWIDTH+ `EXPONENT + 6] MaxAB_0
[`DWIDTH+ `EXPONENT + 5 :`DWIDTH+6] CExp_0
[`DWIDTH+5:`DWIDTH+1] InputExc_0
[`DWIDTH:0] Sum_4
*/				
			//pipe_6 <= {pipe_5[`DWIDTH+`EXPONENT+11], Shift_5[4:0], pipe_5[`DWIDTH+`EXPONENT+10:`DWIDTH+1], SumS_5[`DWIDTH:0]} ;	
/* PIPE_7 :
[`DWIDTH+ `EXPONENT + 16] operation
[`DWIDTH+ `EXPONENT + 15:`DWIDTH+ `EXPONENT + 11] Shift_5
[`DWIDTH+ `EXPONENT + 10] PSgn_4
[`DWIDTH+ `EXPONENT + 9] Opr_4
[`DWIDTH+ `EXPONENT + 8] Sa_0
[`DWIDTH+ `EXPONENT + 7] Sb_0
[`DWIDTH+ `EXPONENT + 6] MaxAB_0
[`DWIDTH+ `EXPONENT + 5 :`DWIDTH+6] CExp_0
[`DWIDTH+5:`DWIDTH+1] InputExc_0
[`DWIDTH:0] Sum_4
*/						
			//pipe_7 <= {pipe_6[`DWIDTH+`EXPONENT+16:`DWIDTH+1], SumS_7[`DWIDTH:0]} ;	
/* PIPE_8:
[2*`EXPONENT + `MANTISSA + 15] FG_8 
[2*`EXPONENT + `MANTISSA + 14] operation
[2*`EXPONENT + `MANTISSA + 13] PSgn_4
[2*`EXPONENT + `MANTISSA + 12] Sa_0
[2*`EXPONENT + `MANTISSA + 11] Sb_0
[2*`EXPONENT + `MANTISSA + 10] MaxAB_0
[2*`EXPONENT + `MANTISSA + 9:`EXPONENT + `MANTISSA + 10] CExp_0
[`EXPONENT + `MANTISSA + 9:`EXPONENT + `MANTISSA + 5] InputExc_8
[`EXPONENT + `MANTISSA + 4 :`EXPONENT + 5] NormM_8 
[`EXPONENT + 4 :4] NormE_8
[3] ZeroSum_8
[2] NegE_8
[1] R_8
[0] S_8
*/				
			pipe_8 <= {FG_8, pipe_7[`DWIDTH+`EXPONENT+16], pipe_7[`DWIDTH+`EXPONENT+10], pipe_7[`DWIDTH+`EXPONENT+8:`DWIDTH+1], NormM_8[`MANTISSA-1:0], NormE_8[`EXPONENT:0], ZeroSum_8, NegE_8, R_8, S_8} ;	
/* pipe_9:
[`DWIDTH + 8 :9] P_int
[8] NegE_8
[7] R_8
[6] S_8
[5:1] InputExc_8
[0] EOF
*/				
			//pipe_9 <= {P_int[`DWIDTH-1:0], pipe_8[2], pipe_8[1], pipe_8[0], pipe_8[`EXPONENT+`MANTISSA+9:`EXPONENT+`MANTISSA+5], EOF} ;	
		end
	end		
	
endmodule


//
// Description:	 	The pre-alignment module is responsible for taking the inputs
//							apart and checking the parts for exceptions.
//							The exponent difference is also calculated in this module.
//


module FPAddSub_PrealignModule(
		A,
		B,
		operation,
		Sa,
		Sb,
		ShiftDet,
		InputExc,
		Aout,
		Bout,
		Opout
	);
	
	// Input ports
	input [`DWIDTH-1:0] A ;										// Input A, a 32-bit floating point number
	input [`DWIDTH-1:0] B ;										// Input B, a 32-bit floating point number
	input operation ;
	
	// Output ports
	output Sa ;												// A's sign
	output Sb ;												// B's sign
	output [2*`EXPONENT-1:0] ShiftDet ;
	output [4:0] InputExc ;								// Input numbers are exceptions
	output [`DWIDTH-2:0] Aout ;
	output [`DWIDTH-2:0] Bout ;
	output Opout ;
	
	// Internal signals									// If signal is high...
	wire ANaN ;												// A is a NaN (Not-a-Number)
	wire BNaN ;												// B is a NaN
	wire AInf ;												// A is infinity
	wire BInf ;												// B is infinity
	wire [`EXPONENT-1:0] DAB ;										// ExpA - ExpB					
	wire [`EXPONENT-1:0] DBA ;										// ExpB - ExpA	
	
	assign ANaN = &(A[`DWIDTH-2:`DWIDTH-1-`EXPONENT]) & |(A[`MANTISSA-1:0]) ;		// All one exponent and not all zero mantissa - NaN
	assign BNaN = &(B[`DWIDTH-2:`DWIDTH-1-`EXPONENT]) & |(B[`MANTISSA-1:0]);		// All one exponent and not all zero mantissa - NaN
	assign AInf = &(A[`DWIDTH-2:`DWIDTH-1-`EXPONENT]) & ~|(A[`MANTISSA-1:0]) ;	// All one exponent and all zero mantissa - Infinity
	assign BInf = &(B[`DWIDTH-2:`DWIDTH-1-`EXPONENT]) & ~|(B[`MANTISSA-1:0]) ;	// All one exponent and all zero mantissa - Infinity
	
	// Put all flags into exception vector
	assign InputExc = {(ANaN | BNaN | AInf | BInf), ANaN, BNaN, AInf, BInf} ;
	
	//assign DAB = (A[30:23] - B[30:23]) ;
	//assign DBA = (B[30:23] - A[30:23]) ;
	assign DAB = (A[`DWIDTH-2:`MANTISSA] + ~(B[`DWIDTH-2:`MANTISSA]) + 1) ;
	assign DBA = (B[`DWIDTH-2:`MANTISSA] + ~(A[`DWIDTH-2:`MANTISSA]) + 1) ;
	
	assign Sa = A[`DWIDTH-1] ;									// A's sign bit
	assign Sb = B[`DWIDTH-1] ;									// B's sign	bit
	assign ShiftDet = {DBA[`EXPONENT-1:0], DAB[`EXPONENT-1:0]} ;		// Shift data
	assign Opout = operation ;
	assign Aout = A[`DWIDTH-2:0] ;
	assign Bout = B[`DWIDTH-2:0] ;
	
endmodule


//
// Description:	 	The alignment module determines the larger input operand and
//							sets the mantissas, shift and common exponent accordingly.
//


module FPAddSub_AlignModule (
		A,
		B,
		ShiftDet,
		CExp,
		MaxAB,
		Shift,
		Mmin,
		Mmax
	);
	
	// Input ports
	input [`DWIDTH-2:0] A ;								// Input A, a 32-bit floating point number
	input [`DWIDTH-2:0] B ;								// Input B, a 32-bit floating point number
	input [2*`EXPONENT-1:0] ShiftDet ;
	
	// Output ports
	output [`EXPONENT-1:0] CExp ;							// Common Exponent
	output MaxAB ;									// Incidates larger of A and B (0/A, 1/B)
	output [`EXPONENT-1:0] Shift ;							// Number of steps to smaller mantissa shift right
	output [`MANTISSA-1:0] Mmin ;							// Smaller mantissa 
	output [`MANTISSA-1:0] Mmax ;							// Larger mantissa
	
	// Internal signals
	//wire BOF ;										// Check for shifting overflow if B is larger
	//wire AOF ;										// Check for shifting overflow if A is larger
	
	assign MaxAB = (A[`DWIDTH-2:0] < B[`DWIDTH-2:0]) ;	
	//assign BOF = ShiftDet[9:5] < 25 ;		// Cannot shift more than 25 bits
	//assign AOF = ShiftDet[4:0] < 25 ;		// Cannot shift more than 25 bits
	
	// Determine final shift value
	//assign Shift = MaxAB ? (BOF ? ShiftDet[9:5] : 5'b11001) : (AOF ? ShiftDet[4:0] : 5'b11001) ;
	
	assign Shift = MaxAB ? ShiftDet[2*`EXPONENT-1:`EXPONENT] : ShiftDet[`EXPONENT-1:0] ;
	
	// Take out smaller mantissa and append shift space
	assign Mmin = MaxAB ? A[`MANTISSA-1:0] : B[`MANTISSA-1:0] ; 
	
	// Take out larger mantissa	
	assign Mmax = MaxAB ? B[`MANTISSA-1:0]: A[`MANTISSA-1:0] ;	
	
	// Common exponent
	assign CExp = (MaxAB ? B[`MANTISSA+`EXPONENT-1:`MANTISSA] : A[`MANTISSA+`EXPONENT-1:`MANTISSA]) ;		
	
endmodule


// Description:	 Alignment shift stage 1, performs 16|12|8|4 shift
//


// ONLY THIS MODULE IS HARDCODED for half precision fp16 and bfloat16
module FPAddSub_AlignShift1(
		//bf16,
		MminP,
		Shift,
		Mmin
	);
	
	// Input ports
	//input bf16;
	input [`MANTISSA-1:0] MminP ;						// Smaller mantissa after 16|12|8|4 shift
	input [`EXPONENT-3:0] Shift ;						// Shift amount. Last 2 bits of shifting are done in next stage. Hence, we have [`EXPONENT - 2] bits
	
	// Output ports
	output [`MANTISSA:0] Mmin ;						// The smaller mantissa
	

	wire bf16;
	assign bf16 = 1'b1; //hardcoding to 1, to avoid ODIN issue. a `ifdef here wasn't working. apparently, nested `ifdefs don't work

	// Internal signals
	reg	  [`MANTISSA:0]		Lvl1;
	reg	  [`MANTISSA:0]		Lvl2;
	wire    [2*`MANTISSA+1:0]    Stage1;	
	integer           i;                // Loop variable

	wire [`MANTISSA:0] temp_0; 

assign temp_0 = 0;

	always @(*) begin
		if (bf16 == 1'b1) begin						
//hardcoding for bfloat16
	//For bfloat16, we can shift the mantissa by a max of 7 bits since mantissa has a width of 7. 
	//Hence if either, bit[3]/bit[4]/bit[5]/bit[6]/bit[7] is 1, we can make it 0. This corresponds to bits [5:1] in our updated shift which doesn't contain last 2 bits.
		//Lvl1 <= (Shift[1]|Shift[2]|Shift[3]|Shift[4]|Shift[5]) ? {temp_0} : {1'b1, MminP};  // MANTISSA + 1 width	
		Lvl1 <= (|Shift[`EXPONENT-3:1]) ? {temp_0} : {1'b1, MminP};  // MANTISSA + 1 width	
		end
		else begin
		//for half precision fp16, 10 bits can be shifted. Hence, only shifts till 10 (01010)can be made. 
		Lvl1 <= Shift[2] ? {temp_0} : {1'b1, MminP};
		end
	end
	
	assign Stage1 = { temp_0, Lvl1}; //2*MANTISSA + 2 width

	always @(*) begin    					// Rotate {0 | 4 } bits
	if(bf16 == 1'b1) begin
	  case (Shift[0])
			// Rotate by 0	
			1'b0:  Lvl2 <= Stage1[`MANTISSA:0];       			
			// Rotate by 4	
			1'b1:  begin for (i=0; i<=`MANTISSA; i=i+1) begin Lvl2[i] <= Stage1[i+4]; end Lvl2[`MANTISSA:`MANTISSA-3] <= 0; end
	  endcase
	end
	else begin
	  case (Shift[1:0])					// Rotate {0 | 4 | 8} bits
			// Rotate by 0	
			2'b00:  Lvl2 <= Stage1[`MANTISSA:0];       			
			// Rotate by 4	
			2'b01:  begin for (i=0; i<=`MANTISSA; i=i+1) begin Lvl2[i] <= Stage1[i+4]; end Lvl2[`MANTISSA:`MANTISSA-3] <= 0; end
			// Rotate by 8
			2'b10:  begin for (i=0; i<=`MANTISSA; i=i+1) begin Lvl2[i] <= Stage1[i+8]; end Lvl2[`MANTISSA:`MANTISSA-7] <= 0; end
			// Rotate by 12	
			2'b11: Lvl2[`MANTISSA: 0] <= 0; 
			//2'b11:  begin for (i=0; i<=`MANTISSA; i=i+1) begin Lvl2[i] <= Stage1[i+12]; end Lvl2[`MANTISSA:`MANTISSA-12] <= 0; end
	  endcase
	end
	end

	// Assign output to next shift stage
	assign Mmin = Lvl2;
	
endmodule


// Description:	 Alignment shift stage 2, performs 3|2|1 shift
//


module FPAddSub_AlignShift2(
		MminP,
		Shift,
		Mmin
	);
	
	// Input ports
	input [`MANTISSA:0] MminP ;						// Smaller mantissa after 16|12|8|4 shift
	input [1:0] Shift ;						// Shift amount. Last 2 bits
	
	// Output ports
	output [`MANTISSA:0] Mmin ;						// The smaller mantissa
	
	// Internal Signal
	reg	  [`MANTISSA:0]		Lvl3;
	wire    [2*`MANTISSA+1:0]    Stage2;	
	integer           j;               // Loop variable
	
	assign Stage2 = {11'b0, MminP};

	always @(*) begin    // Rotate {0 | 1 | 2 | 3} bits
	  case (Shift[1:0])
			// Rotate by 0
			2'b00:  Lvl3 <= Stage2[`MANTISSA:0];   
			// Rotate by 1
			2'b01:  begin for (j=0; j<=`MANTISSA; j=j+1)  begin Lvl3[j] <= Stage2[j+1]; end Lvl3[`MANTISSA] <= 0; end 
			// Rotate by 2
			2'b10:  begin for (j=0; j<=`MANTISSA; j=j+1)  begin Lvl3[j] <= Stage2[j+2]; end Lvl3[`MANTISSA:`MANTISSA-1] <= 0; end 
			// Rotate by 3
			2'b11:  begin for (j=0; j<=`MANTISSA; j=j+1)  begin Lvl3[j] <= Stage2[j+3]; end Lvl3[`MANTISSA:`MANTISSA-2] <= 0; end 	  
	  endcase
	end
	
	// Assign output
	assign Mmin = Lvl3;						// Take out smaller mantissa				

endmodule


//
// Description:	 Module that executes the addition or subtraction on mantissas.
//


module FPAddSub_ExecutionModule(
		Mmax,
		Mmin,
		Sa,
		Sb,
		MaxAB,
		OpMode,
		Sum,
		PSgn,
		Opr
    );

	// Input ports
	input [`MANTISSA-1:0] Mmax ;					// The larger mantissa
	input [`MANTISSA:0] Mmin ;					// The smaller mantissa
	input Sa ;								// Sign bit of larger number
	input Sb ;								// Sign bit of smaller number
	input MaxAB ;							// Indicates the larger number (0/A, 1/B)
	input OpMode ;							// Operation to be performed (0/Add, 1/Sub)
	
	// Output ports
	output [`DWIDTH:0] Sum ;					// The result of the operation
	output PSgn ;							// The sign for the result
	output Opr ;							// The effective (performed) operation

	wire [`EXPONENT-1:0]temp_1;

	assign Opr = (OpMode^Sa^Sb); 		// Resolve sign to determine operation
	assign temp_1 = 0;
	// Perform effective operation
//SAMIDH_UNSURE 5--> 8

	assign Sum = (OpMode^Sa^Sb) ? ({1'b1, Mmax, temp_1} - {Mmin, temp_1}) : ({1'b1, Mmax, temp_1} + {Mmin, temp_1}) ;
	
	// Assign result sign
	assign PSgn = (MaxAB ? Sb : Sa) ;

endmodule


//
// Description:	 Determine the normalization shift amount and perform 16-shift
//


module FPAddSub_NormalizeModule(
		Sum,
		Mmin,
		Shift
    );

	// Input ports
	input [`DWIDTH:0] Sum ;					// Mantissa sum including hidden 1 and GRS
	
	// Output ports
	output [`DWIDTH:0] Mmin ;					// Mantissa after 16|0 shift
	output [4:0] Shift ;					// Shift amount
	//Changes in this doesn't matter since even Bfloat16 can't go beyond 7 shift to the mantissa (only 3 bits valid here)  
	// Determine normalization shift amount by finding leading nought
	assign Shift =  ( 
		Sum[16] ? 5'b00000 :	 
		Sum[15] ? 5'b00001 : 
		Sum[14] ? 5'b00010 : 
		Sum[13] ? 5'b00011 : 
		Sum[12] ? 5'b00100 : 
		Sum[11] ? 5'b00101 : 
		Sum[10] ? 5'b00110 : 
		Sum[9] ? 5'b00111 :
		Sum[8] ? 5'b01000 :
		Sum[7] ? 5'b01001 :
		Sum[6] ? 5'b01010 :
		Sum[5] ? 5'b01011 :
		Sum[4] ? 5'b01100 : 5'b01101
	//	Sum[19] ? 5'b01101 :
	//	Sum[18] ? 5'b01110 :
	//	Sum[17] ? 5'b01111 :
	//	Sum[16] ? 5'b10000 :
	//	Sum[15] ? 5'b10001 :
	//	Sum[14] ? 5'b10010 :
	//	Sum[13] ? 5'b10011 :
	//	Sum[12] ? 5'b10100 :
	//	Sum[11] ? 5'b10101 :
	//	Sum[10] ? 5'b10110 :
	//	Sum[9] ? 5'b10111 :
	//	Sum[8] ? 5'b11000 :
	//	Sum[7] ? 5'b11001 : 5'b11010
	);
	
	reg	  [`DWIDTH:0]		Lvl1;
	
	always @(*) begin
		// Rotate by 16?
		Lvl1 <= Shift[4] ? {Sum[8:0], 8'b00000000} : Sum; 
	end
	
	// Assign outputs
	assign Mmin = Lvl1;						// Take out smaller mantissa

endmodule


// Description:	 Normalization shift stage 1, performs 12|8|4|3|2|1|0 shift
//
//Hardcoding loop start and end values of i. To avoid ODIN limitations. i=`DWIDTH*2+1 wasn't working.

module FPAddSub_NormalizeShift1(
		MminP,
		Shift,
		Mmin
	);
	
	// Input ports
	input [`DWIDTH:0] MminP ;						// Smaller mantissa after 16|12|8|4 shift
	input [3:0] Shift ;						// Shift amount
	
	// Output ports
	output [`DWIDTH:0] Mmin ;						// The smaller mantissa
	
	reg	  [`DWIDTH:0]		Lvl2;
	wire    [2*`DWIDTH+1:0]    Stage1;	
	reg	  [`DWIDTH:0]		Lvl3;
	wire    [2*`DWIDTH+1:0]    Stage2;	
	integer           i;               	// Loop variable
	
	assign Stage1 = {MminP, MminP};

	always @(*) begin    					// Rotate {0 | 4 | 8 | 12} bits
	  case (Shift[3:2])
			// Rotate by 0
			2'b00: Lvl2 <= Stage1[`DWIDTH:0];       		
			// Rotate by 4
			2'b01: begin for (i=33; i>=17; i=i-1) begin Lvl2[i-33] <= Stage1[i-4]; end Lvl2[3:0] <= 0; end
			// Rotate by 8
			2'b10: begin for (i=33; i>=17; i=i-1) begin Lvl2[i-33] <= Stage1[i-8]; end Lvl2[7:0] <= 0; end
			// Rotate by 12
			2'b11: begin for (i=33; i>=17; i=i-1) begin Lvl2[i-33] <= Stage1[i-12]; end Lvl2[11:0] <= 0; end
	  endcase
	end
	
	assign Stage2 = {Lvl2, Lvl2};

	always @(*) begin   				 		// Rotate {0 | 1 | 2 | 3} bits
	  case (Shift[1:0])
			// Rotate by 0
			2'b00:  Lvl3 <= Stage2[`DWIDTH:0];
			// Rotate by 1
			2'b01: begin for (i=33; i>=17; i=i-1) begin Lvl3[i-`DWIDTH-1] <= Stage2[i-1]; end Lvl3[0] <= 0; end 
			// Rotate by 2
			2'b10: begin for (i=33; i>=17; i=i-1) begin Lvl3[i-`DWIDTH-1] <= Stage2[i-2]; end Lvl3[1:0] <= 0; end
			// Rotate by 3
			2'b11: begin for (i=33; i>=17; i=i-1) begin Lvl3[i-`DWIDTH-1] <= Stage2[i-3]; end Lvl3[2:0] <= 0; end
	  endcase
	end
	
	// Assign outputs
	assign Mmin = Lvl3;						// Take out smaller mantissa			
	
endmodule


// Description:	 Normalization shift stage 2, calculates post-normalization
//						 mantissa and exponent, as well as the bits used in rounding		
//


module FPAddSub_NormalizeShift2(
		PSSum,
		CExp,
		Shift,
		NormM,
		NormE,
		ZeroSum,
		NegE,
		R,
		S,
		FG
	);
	
	// Input ports
	input [`DWIDTH:0] PSSum ;					// The Pre-Shift-Sum
	input [`EXPONENT-1:0] CExp ;
	input [4:0] Shift ;					// Amount to be shifted

	// Output ports
	output [`MANTISSA-1:0] NormM ;				// Normalized mantissa
	output [`EXPONENT:0] NormE ;					// Adjusted exponent
	output ZeroSum ;						// Zero flag
	output NegE ;							// Flag indicating negative exponent
	output R ;								// Round bit
	output S ;								// Final sticky bit
	output FG ;

	// Internal signals
	wire MSBShift ;						// Flag indicating that a second shift is needed
	wire [`EXPONENT:0] ExpOF ;					// MSB set in sum indicates overflow
	wire [`EXPONENT:0] ExpOK ;					// MSB not set, no adjustment
	
	// Calculate normalized exponent and mantissa, check for all-zero sum
	assign MSBShift = PSSum[`DWIDTH] ;		// Check MSB in unnormalized sum
	assign ZeroSum = ~|PSSum ;			// Check for all zero sum
	assign ExpOK = CExp - Shift ;		// Adjust exponent for new normalized mantissa
	assign NegE = ExpOK[`EXPONENT] ;			// Check for exponent overflow
	assign ExpOF = CExp - Shift + 1'b1 ;		// If MSB set, add one to exponent(x2)
	assign NormE = MSBShift ? ExpOF : ExpOK ;			// Check for exponent overflow
	assign NormM = PSSum[`DWIDTH-1:`EXPONENT+1] ;		// The new, normalized mantissa
	
	// Also need to compute sticky and round bits for the rounding stage
	assign FG = PSSum[`EXPONENT] ; 
	assign R = PSSum[`EXPONENT-1] ;
	assign S = |PSSum[`EXPONENT-2:0] ;
	
endmodule


// Description:	 Performs 'Round to nearest, tie to even'-rounding on the
//						 normalized mantissa according to the G, R, S bits. Calculates
//						 final result and checks for exponent overflow.
//


module FPAddSub_RoundModule(
		ZeroSum,
		NormE,
		NormM,
		R,
		S,
		G,
		Sa,
		Sb,
		Ctrl,
		MaxAB,
		Z,
		EOF
    );

	// Input ports
	input ZeroSum ;					// Sum is zero
	input [`EXPONENT:0] NormE ;				// Normalized exponent
	input [`MANTISSA-1:0] NormM ;				// Normalized mantissa
	input R ;							// Round bit
	input S ;							// Sticky bit
	input G ;
	input Sa ;							// A's sign bit
	input Sb ;							// B's sign bit
	input Ctrl ;						// Control bit (operation)
	input MaxAB ;
	
	// Output ports
	output [`DWIDTH-1:0] Z ;					// Final result
	output EOF ;
	
	// Internal signals
	wire [`MANTISSA:0] RoundUpM ;			// Rounded up sum with room for overflow
	wire [`MANTISSA-1:0] RoundM ;				// The final rounded sum
	wire [`EXPONENT:0] RoundE ;				// Rounded exponent (note extra bit due to poential overflow	)
	wire RoundUp ;						// Flag indicating that the sum should be rounded up
        wire FSgn;
	wire ExpAdd ;						// May have to add 1 to compensate for overflow 
	wire RoundOF ;						// Rounding overflow
	
	wire [`EXPONENT:0]temp_2;
	assign temp_2 = 0;
	// The cases where we need to round upwards (= adding one) in Round to nearest, tie to even
	assign RoundUp = (G & ((R | S) | NormM[0])) ;
	
	// Note that in the other cases (rounding down), the sum is already 'rounded'
	assign RoundUpM = (NormM + 1) ;								// The sum, rounded up by 1
	assign RoundM = (RoundUp ? RoundUpM[`MANTISSA-1:0] : NormM) ; 	// Compute final mantissa	
	assign RoundOF = RoundUp & RoundUpM[`MANTISSA] ; 				// Check for overflow when rounding up

	// Calculate post-rounding exponent
	assign ExpAdd = (RoundOF ? 1'b1 : 1'b0) ; 				// Add 1 to exponent to compensate for overflow
	assign RoundE = ZeroSum ? temp_2 : (NormE + ExpAdd) ; 							// Final exponent

	// If zero, need to determine sign according to rounding
	assign FSgn = (ZeroSum & (Sa ^ Sb)) | (ZeroSum ? (Sa & Sb & ~Ctrl) : ((~MaxAB & Sa) | ((Ctrl ^ Sb) & (MaxAB | Sa)))) ;

	// Assign final result
	assign Z = {FSgn, RoundE[`EXPONENT-1:0], RoundM[`MANTISSA-1:0]} ;
	
	// Indicate exponent overflow
	assign EOF = RoundE[`EXPONENT];
	
endmodule


//
// Description:	 Check the final result for exception conditions and set
//						 flags accordingly.
//


module FPAddSub_ExceptionModule(
		Z,
		NegE,
		R,
		S,
		InputExc,
		EOF,
		P,
		Flags
    );
	 
	// Input ports
	input [`DWIDTH-1:0] Z	;					// Final product
	input NegE ;						// Negative exponent?
	input R ;							// Round bit
	input S ;							// Sticky bit
	input [4:0] InputExc ;			// Exceptions in inputs A and B
	input EOF ;
	
	// Output ports
	output [`DWIDTH-1:0] P ;					// Final result
	output [4:0] Flags ;				// Exception flags
	
	// Internal signals
	wire Overflow ;					// Overflow flag
	wire Underflow ;					// Underflow flag
	wire DivideByZero ;				// Divide-by-Zero flag (always 0 in Add/Sub)
	wire Invalid ;						// Invalid inputs or result
	wire Inexact ;						// Result is inexact because of rounding
	
	// Exception flags
	
	// Result is too big to be represented
	assign Overflow = EOF | InputExc[1] | InputExc[0] ;
	
	// Result is too small to be represented
	assign Underflow = NegE & (R | S);
	
	// Infinite result computed exactly from finite operands
	assign DivideByZero = &(Z[`MANTISSA+`EXPONENT-1:`MANTISSA]) & ~|(Z[`MANTISSA+`EXPONENT-1:`MANTISSA]) & ~InputExc[1] & ~InputExc[0];
	
	// Invalid inputs or operation
	assign Invalid = |(InputExc[4:2]) ;
	
	// Inexact answer due to rounding, overflow or underflow
	assign Inexact = (R | S) | Overflow | Underflow;
	
	// Put pieces together to form final result
	assign P = Z ;
	
	// Collect exception flags	
	assign Flags = {Overflow, Underflow, DivideByZero, Invalid, Inexact} ; 	
	
endmodule

`endif


