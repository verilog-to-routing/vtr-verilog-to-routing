/* Simplified dispatcher used for FPT'23 */

module collector (
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
  ofifo_rdata,
  ofifo_ren,
  ofifo_rdy
);

input  clk;
input  rst;
// Rx interface
input  rx_tvalid;
input  [511:0] rx_tdata;
input  [63:0] rx_tstrb;
input  [63:0] rx_tkeep;
input  [7:0] rx_tid;
input  [7:0] rx_tdest;
input  [31:0] rx_tuser;
input  rx_tlast;
output rx_tready;
// External FIFO IO
output [63:0] ofifo_rdata;
input  ofifo_ren;
output ofifo_rdy;

wire fifo_push;
assign fifo_push = rx_tvalid && rx_tready;
wire fifo_full_signal, fifo_empty_signal;
wire [511:0] fifo_rdata;
assign ofifo_rdata[0] = fifo_rdata[0] ^ fifo_rdata[1] ^ fifo_rdata[2] ^ fifo_rdata[3] ^ fifo_rdata[4] ^ fifo_rdata[5] ^ fifo_rdata[6] ^ fifo_rdata[7]; 
assign ofifo_rdata[1] = fifo_rdata[8] ^ fifo_rdata[9] ^ fifo_rdata[10] ^ fifo_rdata[11] ^ fifo_rdata[12] ^ fifo_rdata[13] ^ fifo_rdata[14] ^ fifo_rdata[15];
assign ofifo_rdata[2] = fifo_rdata[16] ^ fifo_rdata[17] ^ fifo_rdata[18] ^ fifo_rdata[19] ^ fifo_rdata[20] ^ fifo_rdata[21] ^ fifo_rdata[22] ^ fifo_rdata[23];
assign ofifo_rdata[3] = fifo_rdata[24] ^ fifo_rdata[25] ^ fifo_rdata[26] ^ fifo_rdata[27] ^ fifo_rdata[28] ^ fifo_rdata[29] ^ fifo_rdata[30] ^ fifo_rdata[31];
assign ofifo_rdata[4] = fifo_rdata[32] ^ fifo_rdata[33] ^ fifo_rdata[34] ^ fifo_rdata[35] ^ fifo_rdata[36] ^ fifo_rdata[37] ^ fifo_rdata[38] ^ fifo_rdata[39];
assign ofifo_rdata[5] = fifo_rdata[40] ^ fifo_rdata[41] ^ fifo_rdata[42] ^ fifo_rdata[43] ^ fifo_rdata[44] ^ fifo_rdata[45] ^ fifo_rdata[46] ^ fifo_rdata[47];
assign ofifo_rdata[6] = fifo_rdata[48] ^ fifo_rdata[49] ^ fifo_rdata[50] ^ fifo_rdata[51] ^ fifo_rdata[52] ^ fifo_rdata[53] ^ fifo_rdata[54] ^ fifo_rdata[55];
assign ofifo_rdata[7] = fifo_rdata[56] ^ fifo_rdata[57] ^ fifo_rdata[58] ^ fifo_rdata[59] ^ fifo_rdata[60] ^ fifo_rdata[61] ^ fifo_rdata[62] ^ fifo_rdata[63];
assign ofifo_rdata[8] = fifo_rdata[64] ^ fifo_rdata[65] ^ fifo_rdata[66] ^ fifo_rdata[67] ^ fifo_rdata[68] ^ fifo_rdata[69] ^ fifo_rdata[70] ^ fifo_rdata[71];
assign ofifo_rdata[9] = fifo_rdata[72] ^ fifo_rdata[73] ^ fifo_rdata[74] ^ fifo_rdata[75] ^ fifo_rdata[76] ^ fifo_rdata[77] ^ fifo_rdata[78] ^ fifo_rdata[79];
assign ofifo_rdata[10] = fifo_rdata[80] ^ fifo_rdata[81] ^ fifo_rdata[82] ^ fifo_rdata[83] ^ fifo_rdata[84] ^ fifo_rdata[85] ^ fifo_rdata[86] ^ fifo_rdata[87];
assign ofifo_rdata[11] = fifo_rdata[88] ^ fifo_rdata[89] ^ fifo_rdata[90] ^ fifo_rdata[91] ^ fifo_rdata[92] ^ fifo_rdata[93] ^ fifo_rdata[94] ^ fifo_rdata[95];
assign ofifo_rdata[12] = fifo_rdata[96] ^ fifo_rdata[97] ^ fifo_rdata[98] ^ fifo_rdata[99] ^ fifo_rdata[100] ^ fifo_rdata[101] ^ fifo_rdata[102] ^ fifo_rdata[103];
assign ofifo_rdata[13] = fifo_rdata[104] ^ fifo_rdata[105] ^ fifo_rdata[106] ^ fifo_rdata[107] ^ fifo_rdata[108] ^ fifo_rdata[109] ^ fifo_rdata[110] ^ fifo_rdata[111]; 
assign ofifo_rdata[14] = fifo_rdata[112] ^ fifo_rdata[113] ^ fifo_rdata[114] ^ fifo_rdata[115] ^ fifo_rdata[116] ^ fifo_rdata[117] ^ fifo_rdata[118] ^ fifo_rdata[119]; 
assign ofifo_rdata[15] = fifo_rdata[120] ^ fifo_rdata[121] ^ fifo_rdata[122] ^ fifo_rdata[123] ^ fifo_rdata[124] ^ fifo_rdata[125] ^ fifo_rdata[126] ^ fifo_rdata[127]; 
assign ofifo_rdata[16] = fifo_rdata[128] ^ fifo_rdata[129] ^ fifo_rdata[130] ^ fifo_rdata[131] ^ fifo_rdata[132] ^ fifo_rdata[133] ^ fifo_rdata[134] ^ fifo_rdata[135]; 
assign ofifo_rdata[17] = fifo_rdata[136] ^ fifo_rdata[137] ^ fifo_rdata[138] ^ fifo_rdata[139] ^ fifo_rdata[140] ^ fifo_rdata[141] ^ fifo_rdata[142] ^ fifo_rdata[143]; 
assign ofifo_rdata[18] = fifo_rdata[144] ^ fifo_rdata[145] ^ fifo_rdata[146] ^ fifo_rdata[147] ^ fifo_rdata[148] ^ fifo_rdata[149] ^ fifo_rdata[150] ^ fifo_rdata[151]; 
assign ofifo_rdata[19] = fifo_rdata[152] ^ fifo_rdata[153] ^ fifo_rdata[154] ^ fifo_rdata[155] ^ fifo_rdata[156] ^ fifo_rdata[157] ^ fifo_rdata[158] ^ fifo_rdata[159]; 
assign ofifo_rdata[20] = fifo_rdata[160] ^ fifo_rdata[161] ^ fifo_rdata[162] ^ fifo_rdata[163] ^ fifo_rdata[164] ^ fifo_rdata[165] ^ fifo_rdata[166] ^ fifo_rdata[167]; 
assign ofifo_rdata[21] = fifo_rdata[168] ^ fifo_rdata[169] ^ fifo_rdata[170] ^ fifo_rdata[171] ^ fifo_rdata[172] ^ fifo_rdata[173] ^ fifo_rdata[174] ^ fifo_rdata[175]; 
assign ofifo_rdata[22] = fifo_rdata[176] ^ fifo_rdata[177] ^ fifo_rdata[178] ^ fifo_rdata[179] ^ fifo_rdata[180] ^ fifo_rdata[181] ^ fifo_rdata[182] ^ fifo_rdata[183]; 
assign ofifo_rdata[23] = fifo_rdata[184] ^ fifo_rdata[185] ^ fifo_rdata[186] ^ fifo_rdata[187] ^ fifo_rdata[188] ^ fifo_rdata[189] ^ fifo_rdata[190] ^ fifo_rdata[191]; 
assign ofifo_rdata[24] = fifo_rdata[192] ^ fifo_rdata[193] ^ fifo_rdata[194] ^ fifo_rdata[195] ^ fifo_rdata[196] ^ fifo_rdata[197] ^ fifo_rdata[198] ^ fifo_rdata[199]; 
assign ofifo_rdata[25] = fifo_rdata[200] ^ fifo_rdata[201] ^ fifo_rdata[202] ^ fifo_rdata[203] ^ fifo_rdata[204] ^ fifo_rdata[205] ^ fifo_rdata[206] ^ fifo_rdata[207]; 
assign ofifo_rdata[26] = fifo_rdata[208] ^ fifo_rdata[209] ^ fifo_rdata[210] ^ fifo_rdata[211] ^ fifo_rdata[212] ^ fifo_rdata[213] ^ fifo_rdata[214] ^ fifo_rdata[215]; 
assign ofifo_rdata[27] = fifo_rdata[216] ^ fifo_rdata[217] ^ fifo_rdata[218] ^ fifo_rdata[219] ^ fifo_rdata[220] ^ fifo_rdata[221] ^ fifo_rdata[222] ^ fifo_rdata[223]; 
assign ofifo_rdata[28] = fifo_rdata[224] ^ fifo_rdata[225] ^ fifo_rdata[226] ^ fifo_rdata[227] ^ fifo_rdata[228] ^ fifo_rdata[229] ^ fifo_rdata[230] ^ fifo_rdata[231]; 
assign ofifo_rdata[29] = fifo_rdata[232] ^ fifo_rdata[233] ^ fifo_rdata[234] ^ fifo_rdata[235] ^ fifo_rdata[236] ^ fifo_rdata[237] ^ fifo_rdata[238] ^ fifo_rdata[239]; 
assign ofifo_rdata[30] = fifo_rdata[240] ^ fifo_rdata[241] ^ fifo_rdata[242] ^ fifo_rdata[243] ^ fifo_rdata[244] ^ fifo_rdata[245] ^ fifo_rdata[246] ^ fifo_rdata[247]; 
assign ofifo_rdata[31] = fifo_rdata[248] ^ fifo_rdata[249] ^ fifo_rdata[250] ^ fifo_rdata[251] ^ fifo_rdata[252] ^ fifo_rdata[253] ^ fifo_rdata[254] ^ fifo_rdata[255]; 
assign ofifo_rdata[32] = fifo_rdata[256] ^ fifo_rdata[257] ^ fifo_rdata[258] ^ fifo_rdata[259] ^ fifo_rdata[260] ^ fifo_rdata[261] ^ fifo_rdata[262] ^ fifo_rdata[263]; 
assign ofifo_rdata[33] = fifo_rdata[264] ^ fifo_rdata[265] ^ fifo_rdata[266] ^ fifo_rdata[267] ^ fifo_rdata[268] ^ fifo_rdata[269] ^ fifo_rdata[270] ^ fifo_rdata[271]; 
assign ofifo_rdata[34] = fifo_rdata[272] ^ fifo_rdata[273] ^ fifo_rdata[274] ^ fifo_rdata[275] ^ fifo_rdata[276] ^ fifo_rdata[277] ^ fifo_rdata[278] ^ fifo_rdata[279]; 
assign ofifo_rdata[35] = fifo_rdata[280] ^ fifo_rdata[281] ^ fifo_rdata[282] ^ fifo_rdata[283] ^ fifo_rdata[284] ^ fifo_rdata[285] ^ fifo_rdata[286] ^ fifo_rdata[287]; 
assign ofifo_rdata[36] = fifo_rdata[288] ^ fifo_rdata[289] ^ fifo_rdata[290] ^ fifo_rdata[291] ^ fifo_rdata[292] ^ fifo_rdata[293] ^ fifo_rdata[294] ^ fifo_rdata[295]; 
assign ofifo_rdata[37] = fifo_rdata[296] ^ fifo_rdata[297] ^ fifo_rdata[298] ^ fifo_rdata[299] ^ fifo_rdata[300] ^ fifo_rdata[301] ^ fifo_rdata[302] ^ fifo_rdata[303]; 
assign ofifo_rdata[38] = fifo_rdata[304] ^ fifo_rdata[305] ^ fifo_rdata[306] ^ fifo_rdata[307] ^ fifo_rdata[308] ^ fifo_rdata[309] ^ fifo_rdata[310] ^ fifo_rdata[311]; 
assign ofifo_rdata[39] = fifo_rdata[312] ^ fifo_rdata[313] ^ fifo_rdata[314] ^ fifo_rdata[315] ^ fifo_rdata[316] ^ fifo_rdata[317] ^ fifo_rdata[318] ^ fifo_rdata[319]; 
assign ofifo_rdata[40] = fifo_rdata[320] ^ fifo_rdata[321] ^ fifo_rdata[322] ^ fifo_rdata[323] ^ fifo_rdata[324] ^ fifo_rdata[325] ^ fifo_rdata[326] ^ fifo_rdata[327]; 
assign ofifo_rdata[41] = fifo_rdata[328] ^ fifo_rdata[329] ^ fifo_rdata[330] ^ fifo_rdata[331] ^ fifo_rdata[332] ^ fifo_rdata[333] ^ fifo_rdata[334] ^ fifo_rdata[335]; 
assign ofifo_rdata[42] = fifo_rdata[336] ^ fifo_rdata[337] ^ fifo_rdata[338] ^ fifo_rdata[339] ^ fifo_rdata[340] ^ fifo_rdata[341] ^ fifo_rdata[342] ^ fifo_rdata[343]; 
assign ofifo_rdata[43] = fifo_rdata[344] ^ fifo_rdata[345] ^ fifo_rdata[346] ^ fifo_rdata[347] ^ fifo_rdata[348] ^ fifo_rdata[349] ^ fifo_rdata[350] ^ fifo_rdata[351]; 
assign ofifo_rdata[44] = fifo_rdata[352] ^ fifo_rdata[353] ^ fifo_rdata[354] ^ fifo_rdata[355] ^ fifo_rdata[356] ^ fifo_rdata[357] ^ fifo_rdata[358] ^ fifo_rdata[359]; 
assign ofifo_rdata[45] = fifo_rdata[360] ^ fifo_rdata[361] ^ fifo_rdata[362] ^ fifo_rdata[363] ^ fifo_rdata[364] ^ fifo_rdata[365] ^ fifo_rdata[366] ^ fifo_rdata[367]; 
assign ofifo_rdata[46] = fifo_rdata[368] ^ fifo_rdata[369] ^ fifo_rdata[370] ^ fifo_rdata[371] ^ fifo_rdata[372] ^ fifo_rdata[373] ^ fifo_rdata[374] ^ fifo_rdata[375]; 
assign ofifo_rdata[47] = fifo_rdata[376] ^ fifo_rdata[377] ^ fifo_rdata[378] ^ fifo_rdata[379] ^ fifo_rdata[380] ^ fifo_rdata[381] ^ fifo_rdata[382] ^ fifo_rdata[383]; 
assign ofifo_rdata[48] = fifo_rdata[384] ^ fifo_rdata[385] ^ fifo_rdata[386] ^ fifo_rdata[387] ^ fifo_rdata[388] ^ fifo_rdata[389] ^ fifo_rdata[390] ^ fifo_rdata[391]; 
assign ofifo_rdata[49] = fifo_rdata[392] ^ fifo_rdata[393] ^ fifo_rdata[394] ^ fifo_rdata[395] ^ fifo_rdata[396] ^ fifo_rdata[397] ^ fifo_rdata[398] ^ fifo_rdata[399]; 
assign ofifo_rdata[50] = fifo_rdata[400] ^ fifo_rdata[401] ^ fifo_rdata[402] ^ fifo_rdata[403] ^ fifo_rdata[404] ^ fifo_rdata[405] ^ fifo_rdata[406] ^ fifo_rdata[407]; 
assign ofifo_rdata[51] = fifo_rdata[408] ^ fifo_rdata[409] ^ fifo_rdata[410] ^ fifo_rdata[411] ^ fifo_rdata[412] ^ fifo_rdata[413] ^ fifo_rdata[414] ^ fifo_rdata[415]; 
assign ofifo_rdata[52] = fifo_rdata[416] ^ fifo_rdata[417] ^ fifo_rdata[418] ^ fifo_rdata[419] ^ fifo_rdata[420] ^ fifo_rdata[421] ^ fifo_rdata[422] ^ fifo_rdata[423]; 
assign ofifo_rdata[53] = fifo_rdata[424] ^ fifo_rdata[425] ^ fifo_rdata[426] ^ fifo_rdata[427] ^ fifo_rdata[428] ^ fifo_rdata[429] ^ fifo_rdata[430] ^ fifo_rdata[431]; 
assign ofifo_rdata[54] = fifo_rdata[432] ^ fifo_rdata[433] ^ fifo_rdata[434] ^ fifo_rdata[435] ^ fifo_rdata[436] ^ fifo_rdata[437] ^ fifo_rdata[438] ^ fifo_rdata[439]; 
assign ofifo_rdata[55] = fifo_rdata[440] ^ fifo_rdata[441] ^ fifo_rdata[442] ^ fifo_rdata[443] ^ fifo_rdata[444] ^ fifo_rdata[445] ^ fifo_rdata[446] ^ fifo_rdata[447]; 
assign ofifo_rdata[56] = fifo_rdata[448] ^ fifo_rdata[449] ^ fifo_rdata[450] ^ fifo_rdata[451] ^ fifo_rdata[452] ^ fifo_rdata[453] ^ fifo_rdata[454] ^ fifo_rdata[455]; 
assign ofifo_rdata[57] = fifo_rdata[456] ^ fifo_rdata[457] ^ fifo_rdata[458] ^ fifo_rdata[459] ^ fifo_rdata[460] ^ fifo_rdata[461] ^ fifo_rdata[462] ^ fifo_rdata[463]; 
assign ofifo_rdata[58] = fifo_rdata[464] ^ fifo_rdata[465] ^ fifo_rdata[466] ^ fifo_rdata[467] ^ fifo_rdata[468] ^ fifo_rdata[469] ^ fifo_rdata[470] ^ fifo_rdata[471]; 
assign ofifo_rdata[59] = fifo_rdata[472] ^ fifo_rdata[473] ^ fifo_rdata[474] ^ fifo_rdata[475] ^ fifo_rdata[476] ^ fifo_rdata[477] ^ fifo_rdata[478] ^ fifo_rdata[479]; 
assign ofifo_rdata[60] = fifo_rdata[480] ^ fifo_rdata[481] ^ fifo_rdata[482] ^ fifo_rdata[483] ^ fifo_rdata[484] ^ fifo_rdata[485] ^ fifo_rdata[486] ^ fifo_rdata[487]; 
assign ofifo_rdata[61] = fifo_rdata[488] ^ fifo_rdata[489] ^ fifo_rdata[490] ^ fifo_rdata[491] ^ fifo_rdata[492] ^ fifo_rdata[493] ^ fifo_rdata[494] ^ fifo_rdata[495]; 
assign ofifo_rdata[62] = fifo_rdata[496] ^ fifo_rdata[497] ^ fifo_rdata[498] ^ fifo_rdata[499] ^ fifo_rdata[500] ^ fifo_rdata[501] ^ fifo_rdata[502] ^ fifo_rdata[503]; 
assign ofifo_rdata[63] = fifo_rdata[504] ^ fifo_rdata[505] ^ fifo_rdata[506] ^ fifo_rdata[507] ^ fifo_rdata[508] ^ fifo_rdata[509] ^ fifo_rdata[510] ^ fifo_rdata[511];


fifo_collector fifo_inst (
	.clk(clk),
	.rst(rst),
	.push(fifo_push),
	.idata(rx_tdata),
	.pop(ofifo_ren),
	.odata(fifo_rdata),
	.empty(fifo_empty_signal),
	.full(fifo_full_signal)
);

assign rx_tready = !fifo_empty_signal;
assign ofifo_rdy = !fifo_full_signal;

endmodule

module fifo_collector (
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