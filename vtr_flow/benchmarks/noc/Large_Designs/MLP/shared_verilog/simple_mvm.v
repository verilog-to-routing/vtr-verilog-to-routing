/* Simplified MVM used for FPT'23 */

module mvm_top (
	clk,
	rst,
	rx_tvalid,
	rx_tdata,
	rx_tstrb,
	rx_tkeep,
	rx_tid,
	rx_tdest,
	rx_tuser,
	rx_tlast,
	rx_tready,	
	tx_tvalid,
	tx_tdata,
	tx_tstrb,
	tx_tkeep,
	tx_tid,
	tx_tdest,
	tx_tuser,
	tx_tlast,
	tx_tready
);

input wire          clk;
input wire          rst;
// Rx interface
input wire          rx_tvalid;
input wire  [511:0] rx_tdata;
input wire  [ 63:0] rx_tstrb;
input wire  [ 63:0] rx_tkeep;
input wire  [  7:0] rx_tid;
input wire  [  7:0] rx_tdest;
input wire  [ 31:0] rx_tuser;
input wire          rx_tlast;
output wire         rx_tready;	
// Tx interface
output wire         tx_tvalid;
output wire [511:0] tx_tdata;
output wire [ 63:0] tx_tstrb;
output wire [ 63:0] tx_tkeep;
output wire [  7:0] tx_tid;
output wire [  7:0] tx_tdest;
output wire [ 31:0] tx_tuser;
output wire         tx_tlast;
input  wire         tx_tready;

// Hook up unused Rx signals to dummy registers to avoid being synthesized away
reg [63:0] dummy_rx_tstrb;
reg [63:0] dummy_rx_tkeep;
reg [63:0] dummy_rx_tdest;

always @ (posedge clk) begin
	dummy_rx_tstrb <= rx_tstrb;
	dummy_rx_tkeep <= rx_tkeep;
	dummy_rx_tdest <= rx_tdest;
end

wire [8:0] inst_raddr;
assign inst_raddr = rx_tuser[15:9];
wire [8:0] inst_waddr;
assign inst_waddr = rx_tuser[8:0];
wire inst_wen;
assign inst_wen = (rx_tid == 0);
wire [511:0] inst_wdata;
assign inst_wdata = rx_tdata;
wire [511:0] inst_rdata; 
wire [8:0] inst_rf_raddr, inst_accum_raddr;
wire inst_reduce, inst_accum_en, inst_release, inst_jump, inst_en, inst_last;

memory_block instruction_fifo (.clk(clk), .waddr(inst_waddr), .wen(inst_wen), .wdata(inst_wdata), .raddr(inst_raddr), .rdata(inst_rdata));

assign inst_rf_raddr = inst_rdata[23:15];
assign inst_accum_raddr = inst_rdata[14:6];
assign inst_last = inst_rdata[5];
assign inst_reduce = inst_rdata[2];
assign inst_accum_en = inst_rdata[3];
assign inst_release = inst_rdata[4];
assign inst_jump = inst_rdata[1];
assign inst_en = inst_rdata[0];

wire input_fifo_empty, input_fifo_full;
wire [511:0] input_fifo_idata;
assign input_fifo_idata = rx_tdata;
wire [511:0] input_fifo_odata;
wire input_fifo_push, input_fifo_pop;
assign input_fifo_push = (rx_tid == 2);
assign input_fifo_pop = inst_last;

fifo_mvm input_fifo (.clk(clk), .rst(rst), .push(input_fifo_push), .idata(input_fifo_idata), .pop(input_fifo_pop), .odata(input_fifo_odata), .empty(input_fifo_empty), .full(input_fifo_full));

wire reduction_fifo_empty, reduction_fifo_full;
wire [511:0] reduction_fifo_idata;
assign reduction_fifo_idata = rx_tdata;
wire [511:0] reduction_fifo_odata;
wire reduction_fifo_push, reduction_fifo_pop;
assign reduction_fifo_push = (rx_tid == 1);
assign reduction_fifo_pop = inst_reduce && !reduction_fifo_empty;

fifo_mvm reduction_fifo (.clk(clk), .rst(rst), .push(reduction_fifo_push), .idata(reduction_fifo_idata), .pop(reduction_fifo_pop), .odata(reduction_fifo_odata), .empty(reduction_fifo_empty), .full(reduction_fifo_full));

wire [8:0] accum_mem_waddr;
wire [511:0] accum_mem_rdata;
wire [17:0] temp_accum_addr, delay_accum_addr;
assign temp_accum_addr = {9'b0, inst_accum_raddr};

dpe_pipeline accum_addr_pipeline (.clk(clk), .rst(rst), .data_in(temp_accum_addr), .data_out(delay_accum_addr));

memory_block accum_mem (.clk(clk), .waddr(delay_accum_addr[8:0]), .wen(dpe_ovalid[0]), .wdata(dpe_results), .raddr(inst_accum_raddr), .rdata(accum_mem_rdata));

wire [8:0] rf_waddr;
assign rf_waddr = rx_tuser[8:0];
wire [511:0] rf_wdata;
assign rf_wdata = rx_tdata;
wire [15:0] rf_wen;
assign rf_wen = rx_tuser[24:9];
wire [511:0] rf_rdata_1, rf_rdata_2, rf_rdata_3, rf_rdata_4, rf_rdata_5, rf_rdata_6, rf_rdata_7, rf_rdata_8,
             rf_rdata_9, rf_rdata_10, rf_rdata_11, rf_rdata_12, rf_rdata_13, rf_rdata_14, rf_rdata_15, rf_rdata_16;

output wire [511:0] dpe_results;
output wire [15:0] dpe_ovalid;

memory_block rf_01(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[0]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_1));
memory_block rf_02(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[1]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_2));
memory_block rf_03(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[2]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_3));
memory_block rf_04(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[3]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_4));
memory_block rf_05(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[4]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_5));
memory_block rf_06(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[5]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_6));
memory_block rf_07(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[6]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_7));
memory_block rf_08(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[7]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_8));
memory_block rf_09(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[8]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_9));
memory_block rf_10(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[9]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_10));
memory_block rf_11(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[10]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_11));
memory_block rf_12(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[11]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_12));
memory_block rf_13(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[12]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_13));
memory_block rf_14(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[13]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_14));
memory_block rf_15(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[14]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_15));
memory_block rf_16(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[15]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_16));

wire dpe_ivalid;
assign dpe_ivalid = inst_en && inst_release;
dpe dpe_01 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_1), .datac(reduction_fifo_odata[(32*1)-1:32*0]), .datad(accum_mem_rdata[(32*1)-1:32*0]), .result(dpe_results[(32*1)-1:32*0]), .ovalid(dpe_ovalid[0]));
dpe dpe_02 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_2), .datac(reduction_fifo_odata[(32*2)-1:32*1]), .datad(accum_mem_rdata[(32*2)-1:32*1]), .result(dpe_results[(32*2)-1:32*1]), .ovalid(dpe_ovalid[1]));
dpe dpe_03 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_3), .datac(reduction_fifo_odata[(32*3)-1:32*2]), .datad(accum_mem_rdata[(32*3)-1:32*2]), .result(dpe_results[(32*3)-1:32*2]), .ovalid(dpe_ovalid[2]));
dpe dpe_04 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_4), .datac(reduction_fifo_odata[(32*4)-1:32*3]), .datad(accum_mem_rdata[(32*4)-1:32*3]), .result(dpe_results[(32*4)-1:32*3]), .ovalid(dpe_ovalid[3]));
dpe dpe_05 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_5), .datac(reduction_fifo_odata[(32*5)-1:32*4]), .datad(accum_mem_rdata[(32*5)-1:32*4]), .result(dpe_results[(32*5)-1:32*4]), .ovalid(dpe_ovalid[4]));
dpe dpe_06 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_6), .datac(reduction_fifo_odata[(32*6)-1:32*5]), .datad(accum_mem_rdata[(32*6)-1:32*5]), .result(dpe_results[(32*6)-1:32*5]), .ovalid(dpe_ovalid[5]));
dpe dpe_07 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_7), .datac(reduction_fifo_odata[(32*7)-1:32*6]), .datad(accum_mem_rdata[(32*7)-1:32*6]), .result(dpe_results[(32*7)-1:32*6]), .ovalid(dpe_ovalid[6]));
dpe dpe_08 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_8), .datac(reduction_fifo_odata[(32*8)-1:32*7]), .datad(accum_mem_rdata[(32*8)-1:32*7]), .result(dpe_results[(32*8)-1:32*7]), .ovalid(dpe_ovalid[7]));
dpe dpe_09 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_9), .datac(reduction_fifo_odata[(32*9)-1:32*8]), .datad(accum_mem_rdata[(32*9)-1:32*8]), .result(dpe_results[(32*9)-1:32*8]), .ovalid(dpe_ovalid[8]));
dpe dpe_10 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_10), .datac(reduction_fifo_odata[(32*10)-1:32*9]), .datad(accum_mem_rdata[(32*10)-1:32*9]), .result(dpe_results[(32*10)-1:32*9]), .ovalid(dpe_ovalid[9]));
dpe dpe_11 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_11), .datac(reduction_fifo_odata[(32*11)-1:32*10]), .datad(accum_mem_rdata[(32*11)-1:32*10]), .result(dpe_results[(32*11)-1:32*10]), .ovalid(dpe_ovalid[10]));
dpe dpe_12 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_12), .datac(reduction_fifo_odata[(32*12)-1:32*11]), .datad(accum_mem_rdata[(32*12)-1:32*11]), .result(dpe_results[(32*12)-1:32*11]), .ovalid(dpe_ovalid[11]));
dpe dpe_13 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_13), .datac(reduction_fifo_odata[(32*13)-1:32*12]), .datad(accum_mem_rdata[(32*13)-1:32*12]), .result(dpe_results[(32*13)-1:32*12]), .ovalid(dpe_ovalid[12]));
dpe dpe_14 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_14), .datac(reduction_fifo_odata[(32*14)-1:32*13]), .datad(accum_mem_rdata[(32*14)-1:32*13]), .result(dpe_results[(32*14)-1:32*13]), .ovalid(dpe_ovalid[13]));
dpe dpe_15 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_15), .datac(reduction_fifo_odata[(32*15)-1:32*14]), .datad(accum_mem_rdata[(32*15)-1:32*14]), .result(dpe_results[(32*15)-1:32*14]), .ovalid(dpe_ovalid[14]));
dpe dpe_16 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_16), .datac(reduction_fifo_odata[(32*16)-1:32*15]), .datad(accum_mem_rdata[(32*16)-1:32*15]), .result(dpe_results[(32*16)-1:32*15]), .ovalid(dpe_ovalid[15]));

wire output_fifo_empty, output_fifo_full;
wire [511:0] output_fifo_odata;
wire output_fifo_pop;
assign output_fifo_pop = tx_tready && !output_fifo_empty;
fifo_mvm output_fifo (.clk(clk), .rst(rst), .push(dpe_ovalid[0]), .idata(dpe_results), .pop(output_fifo_pop), .odata(output_fifo_odata), .empty(output_fifo_empty), .full(output_fifo_full));

reg [ 63:0] r_tx_tstrb;
reg [ 63:0] r_tx_tkeep;
reg [  7:0] r_tx_tid;
reg [  7:0] r_tx_tdest;
reg [ 31:0] r_tx_tuser;
reg         r_tx_tlast;
always @ (posedge clk) begin
  if (rst) begin
    r_tx_tstrb <= 0;
    r_tx_tkeep <= 0;
    r_tx_tid   <= 0;
    r_tx_tdest <= 0;
    r_tx_tuser <= 0;
    r_tx_tlast <= 0;
  end else begin
    r_tx_tstrb <= rx_tstrb;
    r_tx_tkeep <= rx_tkeep;
    r_tx_tid   <= rx_tid;
    r_tx_tdest <= rx_tdest;
    r_tx_tuser <= rx_tuser;
    r_tx_tlast <= rx_tlast;
  end
end

assign tx_tvalid = tx_tready && !output_fifo_empty;
assign tx_tdata  = output_fifo_odata;
assign tx_tstrb  = r_tx_tstrb;
assign tx_tkeep  = r_tx_tkeep;
assign tx_tid    = r_tx_tid; 
assign tx_tdest  = r_tx_tdest;
assign tx_tuser  = r_tx_tuser;
assign tx_tlast  = r_tx_tlast;
assign rx_tready = !input_fifo_full;

endmodule


module dpe (
	clk,
	rst,
	ivalid,
	accum,
	reduce,
	dataa,
	datab,
	datac,
	datad,
	result,
	ovalid
);

input  wire         clk;
input  wire         rst;
input  wire         ivalid;
input  wire         accum;
input  wire         reduce;
input  wire [511:0] dataa;
input  wire [511:0] datab;
input  wire [ 31:0] datac;
input  wire [ 31:0] datad;
output wire [ 31:0] result;
output wire         ovalid;

wire [36:0] chain_atom01_to_atom00, chain_atom02_to_atom01, chain_atom03_to_atom02, chain_atom04_to_atom03, 
						chain_atom05_to_atom04, chain_atom06_to_atom05, chain_atom07_to_atom06, chain_atom08_to_atom07, 
						chain_atom09_to_atom08, chain_atom10_to_atom09, chain_atom11_to_atom10, chain_atom12_to_atom11, 
						chain_atom13_to_atom12, chain_atom14_to_atom13, chain_atom15_to_atom14, dummy_chain;
wire [31:0] res15, res14, res13, res12, res11, res10, res09, res08, res07, res06, res05, res04, res03, res02, res01, res00;

wire [33:0] temp_datac, temp_datad, delay_datac, delay_datad;
assign temp_datac = {ivalid, accum,  datac};
assign temp_datad = {ivalid, reduce, datad};

dpe_pipeline datac_pipe (.clk(clk), .rst(rst), .data_in(temp_datac), .data_out(delay_datac));
dpe_pipeline datad_pipe (.clk(clk), .rst(rst), .data_in(temp_datad), .data_out(delay_datad));

dsp_inst d15(.clk(clk), .reset(rst), .ax(dataa[ (16*1)-1: 16*0]), .ay(datab[ (16*1)-1: 16*0]), .bx(dataa[ (16*2)-1: 16*1]), .by(datab[ (16*2)-1: 16*1]), .chainin(                 37'd0), .result(res15), .chainout(chain_atom15_to_atom14));
dsp_inst d14(.clk(clk), .reset(rst), .ax(dataa[ (16*3)-1: 16*2]), .ay(datab[ (16*3)-1: 16*2]), .bx(dataa[ (16*4)-1: 16*3]), .by(datab[ (16*4)-1: 16*3]), .chainin(chain_atom15_to_atom14), .result(res14), .chainout(chain_atom14_to_atom13));
dsp_inst d13(.clk(clk), .reset(rst), .ax(dataa[ (16*5)-1: 16*4]), .ay(datab[ (16*5)-1: 16*4]), .bx(dataa[ (16*6)-1: 16*5]), .by(datab[ (16*6)-1: 16*5]), .chainin(chain_atom14_to_atom13), .result(res13), .chainout(chain_atom13_to_atom12));
dsp_inst d12(.clk(clk), .reset(rst), .ax(dataa[ (16*7)-1: 16*6]), .ay(datab[ (16*7)-1: 16*6]), .bx(dataa[ (16*8)-1: 16*7]), .by(datab[ (16*8)-1: 16*7]), .chainin(chain_atom13_to_atom12), .result(res12), .chainout(chain_atom12_to_atom11));
dsp_inst d11(.clk(clk), .reset(rst), .ax(dataa[ (16*9)-1: 16*8]), .ay(datab[ (16*9)-1: 16*8]), .bx(dataa[(16*10)-1: 16*9]), .by(datab[(16*10)-1: 16*9]), .chainin(chain_atom12_to_atom11), .result(res11), .chainout(chain_atom11_to_atom10));
dsp_inst d10(.clk(clk), .reset(rst), .ax(dataa[(16*11)-1:16*10]), .ay(datab[(16*11)-1:16*10]), .bx(dataa[(16*12)-1:16*11]), .by(datab[(16*12)-1:16*11]), .chainin(chain_atom11_to_atom10), .result(res10), .chainout(chain_atom10_to_atom09));
dsp_inst d09(.clk(clk), .reset(rst), .ax(dataa[(16*13)-1:16*12]), .ay(datab[(16*13)-1:16*12]), .bx(dataa[(16*14)-1:16*13]), .by(datab[(16*14)-1:16*13]), .chainin(chain_atom10_to_atom09), .result(res09), .chainout(chain_atom09_to_atom08));
dsp_inst d08(.clk(clk), .reset(rst), .ax(dataa[(16*15)-1:16*14]), .ay(datab[(16*15)-1:16*14]), .bx(dataa[(16*16)-1:16*15]), .by(datab[(16*16)-1:16*15]), .chainin(chain_atom09_to_atom08), .result(res08), .chainout(chain_atom08_to_atom07));
dsp_inst d07(.clk(clk), .reset(rst), .ax(dataa[(16*17)-1:16*16]), .ay(datab[(16*17)-1:16*16]), .bx(dataa[(16*18)-1:16*17]), .by(datab[(16*18)-1:16*17]), .chainin(chain_atom08_to_atom07), .result(res07), .chainout(chain_atom07_to_atom06));
dsp_inst d06(.clk(clk), .reset(rst), .ax(dataa[(16*19)-1:16*18]), .ay(datab[(16*19)-1:16*18]), .bx(dataa[(16*20)-1:16*19]), .by(datab[(16*20)-1:16*19]), .chainin(chain_atom07_to_atom06), .result(res06), .chainout(chain_atom06_to_atom05));
dsp_inst d05(.clk(clk), .reset(rst), .ax(dataa[(16*21)-1:16*20]), .ay(datab[(16*21)-1:16*20]), .bx(dataa[(16*22)-1:16*21]), .by(datab[(16*22)-1:16*21]), .chainin(chain_atom06_to_atom05), .result(res05), .chainout(chain_atom05_to_atom04));
dsp_inst d04(.clk(clk), .reset(rst), .ax(dataa[(16*23)-1:16*22]), .ay(datab[(16*23)-1:16*22]), .bx(dataa[(16*24)-1:16*23]), .by(datab[(16*24)-1:16*23]), .chainin(chain_atom05_to_atom04), .result(res04), .chainout(chain_atom04_to_atom03));
dsp_inst d03(.clk(clk), .reset(rst), .ax(dataa[(16*25)-1:16*24]), .ay(datab[(16*25)-1:16*24]), .bx(dataa[(16*26)-1:16*25]), .by(datab[(16*26)-1:16*25]), .chainin(chain_atom04_to_atom03), .result(res03), .chainout(chain_atom03_to_atom02));
dsp_inst d02(.clk(clk), .reset(rst), .ax(dataa[(16*27)-1:16*26]), .ay(datab[(16*27)-1:16*26]), .bx(dataa[(16*28)-1:16*27]), .by(datab[(16*28)-1:16*27]), .chainin(chain_atom03_to_atom02), .result(res02), .chainout(chain_atom02_to_atom01));
dsp_inst d01(.clk(clk), .reset(rst), .ax(dataa[(16*29)-1:16*28]), .ay(datab[(16*29)-1:16*28]), .bx(dataa[(16*30)-1:16*29]), .by(datab[(16*30)-1:16*29]), .chainin(chain_atom02_to_atom01), .result(res01), .chainout(chain_atom01_to_atom00));
dsp_inst d00(.clk(clk), .reset(rst), .ax(dataa[(16*31)-1:16*30]), .ay(datab[(16*31)-1:16*30]), .bx(dataa[(16*32)-1:16*31]), .by(datab[(16*32)-1:16*31]), .chainin(chain_atom01_to_atom00), .result(res00), .chainout(           dummy_chain));

reg [31:0] r_result;
reg r_ovalid;

always @ (posedge clk) begin
	if (rst) begin
		r_result <= 0;
		r_ovalid <= 1'b0;
	end else begin
		if (delay_datac[33]) begin
			if (delay_datac[32] && delay_datad[32]) begin
				r_result <= res00 + delay_datac[31:0] + delay_datad[31:0];
			end else if (delay_datac[32] && !delay_datad[32]) begin
				r_result <= res00 + delay_datac[31:0];
			end else if (!delay_datac[32] && delay_datad[32]) begin
				r_result <= res00 + delay_datad[31:0];
			end else begin
				r_result <= res00;
			end
		end
		r_ovalid <= delay_datac[33] && delay_datad[33];
	end
end

assign result = r_result;
assign ovalid = r_ovalid;

endmodule

module memory_block (
	clk,
	waddr,
	wen,
	wdata,
	raddr,
	rdata
);

input  wire         clk;
input  wire [  8:0] waddr;
input  wire         wen;
input  wire [511:0] wdata;
input  wire [  8:0] raddr;
output wire [511:0] rdata;

bram_inst b0  (.clk(clk), .wen(wen), .wdata(wdata[(32*1)-1:32*0]),   .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*1)-1:32*0]));
bram_inst b1  (.clk(clk), .wen(wen), .wdata(wdata[(32*2)-1:32*1]),   .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*2)-1:32*1]));
bram_inst b2  (.clk(clk), .wen(wen), .wdata(wdata[(32*3)-1:32*2]),   .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*3)-1:32*2]));
bram_inst b3  (.clk(clk), .wen(wen), .wdata(wdata[(32*4)-1:32*3]),   .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*4)-1:32*3]));
bram_inst b4  (.clk(clk), .wen(wen), .wdata(wdata[(32*5)-1:32*4]),   .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*5)-1:32*4]));
bram_inst b5  (.clk(clk), .wen(wen), .wdata(wdata[(32*6)-1:32*5]),   .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*6)-1:32*5]));
bram_inst b6  (.clk(clk), .wen(wen), .wdata(wdata[(32*7)-1:32*6]),   .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*7)-1:32*6]));
bram_inst b7  (.clk(clk), .wen(wen), .wdata(wdata[(32*8)-1:32*7]),   .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*8)-1:32*7]));
bram_inst b8  (.clk(clk), .wen(wen), .wdata(wdata[(32*9)-1:32*8]),   .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*9)-1:32*8]));
bram_inst b9  (.clk(clk), .wen(wen), .wdata(wdata[(32*10)-1:32*9]),  .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*10)-1:32*9]));
bram_inst b10 (.clk(clk), .wen(wen), .wdata(wdata[(32*11)-1:32*10]), .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*11)-1:32*10]));
bram_inst b11 (.clk(clk), .wen(wen), .wdata(wdata[(32*12)-1:32*11]), .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*12)-1:32*11]));
bram_inst b12 (.clk(clk), .wen(wen), .wdata(wdata[(32*13)-1:32*12]), .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*13)-1:32*12]));
bram_inst b13 (.clk(clk), .wen(wen), .wdata(wdata[(32*14)-1:32*13]), .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*14)-1:32*13]));
bram_inst b14 (.clk(clk), .wen(wen), .wdata(wdata[(32*15)-1:32*14]), .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*15)-1:32*14]));
bram_inst b15 (.clk(clk), .wen(wen), .wdata(wdata[(32*16)-1:32*15]), .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*16)-1:32*15]));

endmodule

module fifo_mvm (
  clk,
  rst,
  push,
  idata,
  pop,
  odata,
  empty,
  full
);

input  wire clk;
input  wire rst;
input  wire push;
input  wire [511:0] idata;
input  wire pop;
output wire [511:0] odata;
output reg empty;
output reg full;

reg [8:0] head_ptr;
reg [8:0] tail_ptr;

bram_inst b0  (.clk(clk), .wen(push), .wdata(idata[(32*1)-1:32*0]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*1)-1:32*0]));
bram_inst b1  (.clk(clk), .wen(push), .wdata(idata[(32*2)-1:32*1]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*2)-1:32*1]));
bram_inst b2  (.clk(clk), .wen(push), .wdata(idata[(32*3)-1:32*2]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*3)-1:32*2]));
bram_inst b3  (.clk(clk), .wen(push), .wdata(idata[(32*4)-1:32*3]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*4)-1:32*3]));
bram_inst b4  (.clk(clk), .wen(push), .wdata(idata[(32*5)-1:32*4]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*5)-1:32*4]));
bram_inst b5  (.clk(clk), .wen(push), .wdata(idata[(32*6)-1:32*5]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*6)-1:32*5]));
bram_inst b6  (.clk(clk), .wen(push), .wdata(idata[(32*7)-1:32*6]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*7)-1:32*6]));
bram_inst b7  (.clk(clk), .wen(push), .wdata(idata[(32*8)-1:32*7]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*8)-1:32*7]));
bram_inst b8  (.clk(clk), .wen(push), .wdata(idata[(32*9)-1:32*8]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*9)-1:32*8]));
bram_inst b9  (.clk(clk), .wen(push), .wdata(idata[(32*10)-1:32*9]),  .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*10)-1:32*9]));
bram_inst b10 (.clk(clk), .wen(push), .wdata(idata[(32*11)-1:32*10]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*11)-1:32*10]));
bram_inst b11 (.clk(clk), .wen(push), .wdata(idata[(32*12)-1:32*11]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*12)-1:32*11]));
bram_inst b12 (.clk(clk), .wen(push), .wdata(idata[(32*13)-1:32*12]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*13)-1:32*12]));
bram_inst b13 (.clk(clk), .wen(push), .wdata(idata[(32*14)-1:32*13]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*14)-1:32*13]));
bram_inst b14 (.clk(clk), .wen(push), .wdata(idata[(32*15)-1:32*14]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*15)-1:32*14]));
bram_inst b15 (.clk(clk), .wen(push), .wdata(idata[(32*16)-1:32*15]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*16)-1:32*15]));

always @ (posedge clk) begin
	if (rst) begin
		head_ptr <= 0;
		tail_ptr <= 0;
	end else begin
		if (push) tail_ptr <= tail_ptr + 1;		
		if (pop)  head_ptr <= head_ptr + 1;
	end
end

always @ (*) begin
  if (tail_ptr == head_ptr) begin
    empty = 1'b1;
  end else begin
    empty = 1'b0;
  end

  if (tail_ptr+1 == head_ptr) begin
    full = 1'b1;
  end else begin
    full = 1'b0;
  end
end

endmodule


module bram_inst(
	clk,
	wen,
	wdata,
	waddr,
	raddr,
	rdata
);

input  wire        clk;
input  wire        wen;
input  wire [31:0] wdata;
input  wire [ 8:0] waddr;
input  wire [ 8:0] raddr;
output wire [31:0] rdata;

wire [39:0] rtemp;
wire [39:0] wtemp;
wire [8:0] addrtemp;
assign rdata = rtemp[31:0];
assign wtemp = {8'd0, wdata};
assign addrtemp = waddr | raddr;

single_port_ram bram_instance(
	.clk(clk),
	.we(wen),
	.data(wtemp),
	.addr(addrtemp),
	.out(rtemp)
);

endmodule


module dpe_pipeline (
	clk,
	rst,
	data_in,
	data_out
);

input  wire        clk;
input  wire        rst;
input  wire [33:0] data_in;
output wire [33:0] data_out;

reg [33:0] r_pipeline_00; 
reg [33:0] r_pipeline_01;
reg [33:0] r_pipeline_02;
reg [33:0] r_pipeline_03;  
reg [33:0] r_pipeline_04; 
reg [33:0] r_pipeline_05;
reg [33:0] r_pipeline_06;
reg [33:0] r_pipeline_07;  
reg [33:0] r_pipeline_08; 
reg [33:0] r_pipeline_09;
reg [33:0] r_pipeline_10;
reg [33:0] r_pipeline_11;  
reg [33:0] r_pipeline_12; 
reg [33:0] r_pipeline_13;
reg [33:0] r_pipeline_14;
reg [33:0] r_pipeline_15; 
reg [33:0] r_pipeline_16;
reg [33:0] r_pipeline_17;
reg [33:0] r_pipeline_18;  
reg [33:0] r_pipeline_19; 
reg [33:0] r_pipeline_20;
reg [33:0] r_pipeline_21;
reg [33:0] r_pipeline_22;  
reg [33:0] r_pipeline_23; 
reg [33:0] r_pipeline_24;
reg [33:0] r_pipeline_25;
reg [33:0] r_pipeline_26;  
reg [33:0] r_pipeline_27; 
reg [33:0] r_pipeline_28;
reg [33:0] r_pipeline_29;
reg [33:0] r_pipeline_30;
reg [33:0] r_pipeline_31;

always @ (posedge clk) begin
	if (rst) begin
		r_pipeline_00 <= 0;
		r_pipeline_01 <= 0;
		r_pipeline_02 <= 0;
		r_pipeline_03 <= 0;
		r_pipeline_04 <= 0;
		r_pipeline_05 <= 0;
		r_pipeline_06 <= 0;
		r_pipeline_07 <= 0;
		r_pipeline_08 <= 0;
		r_pipeline_09 <= 0;
		r_pipeline_10 <= 0;
		r_pipeline_11 <= 0;
		r_pipeline_12 <= 0;
		r_pipeline_13 <= 0;
		r_pipeline_14 <= 0;
		r_pipeline_15 <= 0;
		r_pipeline_16 <= 0;
		r_pipeline_17 <= 0;
		r_pipeline_18 <= 0;
		r_pipeline_19 <= 0;
		r_pipeline_20 <= 0;
		r_pipeline_21 <= 0;
		r_pipeline_22 <= 0;
		r_pipeline_23 <= 0;
		r_pipeline_24 <= 0;
		r_pipeline_25 <= 0;
		r_pipeline_26 <= 0;
		r_pipeline_27 <= 0;
		r_pipeline_28 <= 0;
		r_pipeline_29 <= 0;
		r_pipeline_30 <= 0;
		r_pipeline_31 <= 0;
	end else begin
		r_pipeline_00 <= r_pipeline_01;
		r_pipeline_01 <= r_pipeline_02;
		r_pipeline_02 <= r_pipeline_03;
		r_pipeline_03 <= r_pipeline_04;
		r_pipeline_04 <= r_pipeline_05;
		r_pipeline_05 <= r_pipeline_06;
		r_pipeline_06 <= r_pipeline_07;
		r_pipeline_07 <= r_pipeline_08;
		r_pipeline_08 <= r_pipeline_09;
		r_pipeline_09 <= r_pipeline_10;
		r_pipeline_10 <= r_pipeline_11;
		r_pipeline_11 <= r_pipeline_12;
		r_pipeline_12 <= r_pipeline_13;
		r_pipeline_13 <= r_pipeline_14;
		r_pipeline_14 <= r_pipeline_15;
		r_pipeline_15 <= r_pipeline_16;
		r_pipeline_16 <= r_pipeline_17;
		r_pipeline_17 <= r_pipeline_18;
		r_pipeline_18 <= r_pipeline_19;
		r_pipeline_19 <= r_pipeline_20;
		r_pipeline_20 <= r_pipeline_21;
		r_pipeline_21 <= r_pipeline_22;
		r_pipeline_22 <= r_pipeline_23;
		r_pipeline_23 <= r_pipeline_24;
		r_pipeline_24 <= r_pipeline_25;
		r_pipeline_25 <= r_pipeline_26;
		r_pipeline_26 <= r_pipeline_27;
		r_pipeline_27 <= r_pipeline_28;
		r_pipeline_28 <= r_pipeline_29;
		r_pipeline_29 <= r_pipeline_30;
		r_pipeline_30 <= r_pipeline_31;
		r_pipeline_31 <= data_in;
	end 
end

assign data_out = r_pipeline_00;

endmodule


module dsp_inst(
	clk,
	reset,
	ax,
	ay,
	bx,
	by,
	chainin,
	result,
	chainout
);

input  wire        clk;
input  wire        reset;
input  wire [15:0] ax;
input  wire [15:0] ay;
input  wire [15:0] bx;
input  wire [15:0] by;
input  wire [36:0] chainin;
output wire [31:0] result;
output wire [36:0] chainout;

wire [18:0] tmp_ax;
wire [19:0] tmp_ay;
wire [18:0] tmp_bx;
wire [19:0] tmp_by;
wire [36:0] tmp_result;

assign tmp_ax = {2'b0, ax};
assign tmp_ay = {3'b0, ay};
assign tmp_bx = {2'b0, bx};
assign tmp_by = {3'b0, by};
assign result = tmp_result[31:0];

int_sop_2 dsp_instance(
	.clk(clk),
	.reset(reset),
	.ax(tmp_ax),
	.ay(tmp_ay),
	.bx(tmp_bx),
	.by(tmp_by),
	.chainin(chainin),
	.result(tmp_result),
	.chainout(chainout)
);

endmodule