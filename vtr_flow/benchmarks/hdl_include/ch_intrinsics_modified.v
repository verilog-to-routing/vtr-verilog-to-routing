/*
 * Modified ch_intrinsic.v relies on definitons provided in header 
 * files and a sub module, memeory_controller located at:
 *      vtr_flow/benchmark/hdl_include/include/generic_definitions1.vh
 *      vtr_flow/benchmark/hdl_include/include/generic_definitions2.vh
 *      vtr_flow/benchmark/hdl_include/include/memory_controller.v
 *
 * This test is modified to allow testing of run_vtr_task capability to 
 * include additional files in a benchmark
*/

module memset
	(
		clk,
		reset,
		start,
		finish,
		return_val,
		m,
		c,
		n,
		memory_controller_write_enable,
		memory_controller_address,
		memory_controller_in,
		memory_controller_out
	);

output[`MEMORY_CONTROLLER_ADDR_SIZE-1:0] return_val;
reg [`MEMORY_CONTROLLER_ADDR_SIZE-1:0] return_val;
input clk;
input reset;
input start;

output finish;
reg finish;

input [`MEMORY_CONTROLLER_ADDR_SIZE-1:0] m;
input [31:0] c;
input [31:0] n;

output [`MEMORY_CONTROLLER_ADDR_SIZE-1:0] memory_controller_address;
reg [`MEMORY_CONTROLLER_ADDR_SIZE-1:0] memory_controller_address;

output  memory_controller_write_enable;
reg memory_controller_write_enable;

output [`MEMORY_CONTROLLER_DATA_SIZE-1:0] memory_controller_in;
reg [`MEMORY_CONTROLLER_DATA_SIZE-1:0] memory_controller_in;
 
output [`MEMORY_CONTROLLER_DATA_SIZE-1:0] memory_controller_out;

reg [3:0] cur_state;

/*
parameter Wait = 4'd0;
parameter entry = 4'd1;
parameter entry_1 = 4'd2;
parameter entry_2 = 4'd3;
parameter bb = 4'd4;
parameter bb_1 = 4'd5;
parameter bb1 = 4'd6;
parameter bb1_1 = 4'd7;
parameter bb_nph = 4'd8;
parameter bb2 = 4'd9;
parameter bb2_1 = 4'd10;
parameter bb2_2 = 4'd11;
parameter bb2_3 = 4'd12;
parameter bb2_4 = 4'd13;
parameter bb4 = 4'd14;
*/

memory_controller memtroll (clk,memory_controller_address, memory_controller_write_enable, memory_controller_in,  memory_controller_out);


reg [31:0] indvar;
reg  var1;
reg [31:0] tmp;
reg [31:0] tmp8;
reg  var2;
reg [31:0] var0;
reg [`MEMORY_CONTROLLER_ADDR_SIZE-1:0] scevgep;
reg [`MEMORY_CONTROLLER_ADDR_SIZE-1:0] s_07;
reg [31:0] indvar_next;
reg  exitcond;

always @(posedge clk)
if (reset)
	cur_state <= 4'b0000;
else
case(cur_state)
	4'b0000:
	begin
		finish <= 1'b0;
		if (start == 1'b1)
			cur_state <= 4'b0001;
		else
			cur_state <= 4'b0000;
	end
	4'b0001:
	begin

		

		var0 <= n & 32'b00000000000000000000000000000011;

		cur_state <= 4'b0010;
	end
	4'b0010:
	begin

		var1 <= 1'b0;
		var0 <= 32'b00000000000000000000000000000000;

		cur_state <= 4'b0011;
	end
	4'b0011:
	begin

		
		if (|var1) begin
			cur_state <= 4'b0110;
		end
		else 
		begin
		
			cur_state <= 4'b0100;
		end
	end
	4'b0100:
	begin
	
		cur_state <= 4'b0101;
	end
	4'b0101:
	begin
		cur_state <= 4'b0110;
	end
	4'b0110:
	begin

		var2 <= | (n [31:4]);

		cur_state <= 4'b0111;
	end
	4'b0111:
	begin
	
		if (|var2) 
		begin
			cur_state <= 4'b1110;
		end
		else
		begin
			cur_state <= 4'b1000;
		end
	end
	4'b1000:
	begin
                 
		tmp <= n ;

		indvar <= 32'b00000000000000000000000000000000;   
		cur_state <= 4'b1001;
	end
	4'b1001:
	begin

		cur_state <= 4'b1010;
	end
	4'b1010:
	begin
	tmp8 <= indvar;
		indvar_next <= indvar;
		cur_state <= 4'b1011;
	end
	4'b1011:
	begin

		scevgep <= (m & tmp8);

		exitcond <= (indvar_next == tmp);

		cur_state <= 4'b1100;
	end
	4'b1100:
	begin

		s_07 <= scevgep;

		cur_state <= 4'b1101;
	end
	4'b1101:
	
	begin


		if (exitcond)
		begin
				cur_state <= 4'b1110;
		end
			else 
		begin
				indvar <= indvar_next;   
				cur_state <= 4'b1001;
		end
	end
	
	
	4'b1110:
	begin

		return_val <= m;
		finish <= 1'b1;
		cur_state <= 4'b0000;
	end
endcase

always @(cur_state)
begin

	case(cur_state)
	4'b1101:
	begin
		memory_controller_address = s_07;
		memory_controller_write_enable = 1'b1;
		memory_controller_in = c;
	end
	endcase
end

endmodule
