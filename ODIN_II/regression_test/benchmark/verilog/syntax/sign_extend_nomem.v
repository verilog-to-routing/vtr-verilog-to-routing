`define WIDTH 32

module cmd_module (loaded_data, size,addr_key,sign_ext,result,en,clk);

input [31:0] loaded_data;
input[1:0] size ;
input [1:0] addr_key;
input sign_ext;
input clk;
input en;
output [31:0] result;
reg [31:0] result;
wire addr_key_latched;







register addr_latch (addr_key,clk,1'b1,en,addr_key_latched);

load_data_translator instance1 (loaded_data,addr_key_latched,size,sign_ext,result);

endmodule





module register(d,clk,resetn,en,q);
//parameter WIDTH=32;
// this initialization of a register has WIDTH = 2
input clk;
input resetn;
input en;
input [1:0] d;
output [1:0] q;
reg [1:0] q;

always @(posedge clk or negedge resetn)		//asynchronous reset
begin
	if (resetn==0)
		q<=0;
	else if (en==1)
		q<=d;
end

endmodule


module load_data_translator(
    d_readdatain,
    d_address,
    load_size,
    load_sign_ext,
    d_loadresult);
//parameter WIDTH=32;


input [`WIDTH-1:0] d_readdatain;
input [1:0] d_address;
input [1:0] load_size;
input load_sign_ext;
output [`WIDTH-1:0] d_loadresult;

wire d_adr_one;
assign d_adr_one = d_address [1];
reg [`WIDTH-1:0] d_loadresult;
reg sign;

always @(d_readdatain or d_address or load_size or load_sign_ext)
begin
    case (load_size)
        2'b11:
        begin
            case (d_address)
                2'b00:
					begin
					d_loadresult[7:0]=d_readdatain[31:24];
					sign = d_readdatain[31];
					end
                2'b01:
						begin
						d_loadresult[7:0]=d_readdatain[23:16];
						sign = d_readdatain[23];
						end
                2'b10: 
					begin
					d_loadresult[7:0]=d_readdatain[15:8];
					sign = d_readdatain[15];
					end
                default: 
					begin
					d_loadresult[7:0]=d_readdatain[7:0];
					sign = d_readdatain[7];
					end
            endcase
			// peter milankov note: do this by hand
			// odin II does not support multiple concatenation
            //d_loadresult[31:8]={24{load_sign_ext&d_loadresult[7]}};
			d_loadresult[31]= load_sign_ext&sign;
			d_loadresult[30]= load_sign_ext&sign;
			d_loadresult[29]= load_sign_ext&sign;
			d_loadresult[28]= load_sign_ext&sign;
			d_loadresult[27]= load_sign_ext&sign;
			d_loadresult[26]= load_sign_ext&sign;
			d_loadresult[25]= load_sign_ext&sign;
			d_loadresult[24]= load_sign_ext&sign;
			d_loadresult[23]= load_sign_ext&sign;
			d_loadresult[22]= load_sign_ext&sign;
			d_loadresult[21]= load_sign_ext&sign;
			d_loadresult[20]= load_sign_ext&sign;
			d_loadresult[19]= load_sign_ext&sign;
			d_loadresult[18]= load_sign_ext&sign;
			d_loadresult[17]= load_sign_ext&sign;
			d_loadresult[16]= load_sign_ext&sign;
			d_loadresult[15]= load_sign_ext&sign;
			d_loadresult[14]= load_sign_ext&sign;
			d_loadresult[13]= load_sign_ext&sign;
			d_loadresult[12]= load_sign_ext&sign;
			d_loadresult[11]= load_sign_ext&sign;
			d_loadresult[10]= load_sign_ext&sign;
			d_loadresult[9]= load_sign_ext&sign;
			d_loadresult[8]= load_sign_ext&sign;
        end
        2'b01:
        begin
            case (d_adr_one)
                1'b0:
					begin
						d_loadresult[15:0]=d_readdatain[31:16];
						sign = d_readdatain[31];
					end
                default:
					begin
						d_loadresult[15:0]=d_readdatain[15:0];
						sign = d_readdatain[15];
					end
            endcase
// peter milankov note sign extend is concat, do by hand
            //d_loadresult[31:16]={16{load_sign_ext&d_loadresult[15]}};
			d_loadresult[31]= load_sign_ext&sign;
			d_loadresult[30]= load_sign_ext&sign;
			d_loadresult[29]= load_sign_ext&sign;
			d_loadresult[28]= load_sign_ext&sign;
			d_loadresult[27]= load_sign_ext&sign;
			d_loadresult[26]= load_sign_ext&sign;
			d_loadresult[25]= load_sign_ext&sign;
			d_loadresult[24]= load_sign_ext&sign;
			d_loadresult[23]= load_sign_ext&sign;
			d_loadresult[22]= load_sign_ext&sign;
			d_loadresult[21]= load_sign_ext&sign;
			d_loadresult[20]= load_sign_ext&sign;
			d_loadresult[19]= load_sign_ext&sign;
			d_loadresult[18]= load_sign_ext&sign;
			d_loadresult[17]= load_sign_ext&sign;
			d_loadresult[16]= load_sign_ext&sign;
        end
        default:
begin
            d_loadresult[31:0]=d_readdatain[31:0];
			sign = d_readdatain [31];
end
    endcase
end

endmodule
