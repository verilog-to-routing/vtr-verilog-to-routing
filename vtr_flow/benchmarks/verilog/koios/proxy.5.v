//////////////////////////////////////////////////////////////////////////////
// Proxy/Synthetic benchmark generated from: https://github.com/UT-LCA/koios_proxy_benchmarks
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Author: Karan Mathur (modified by Aman Arora)
//////////////////////////////////////////////////////////////////////////////

module top (input clk, input reset,input [462:0] top_inp, output [319:0] top_outp); 
 


 wire [207:0] inp_dpram1;
wire [159:0] outp_dpram1;

dpram_2048_40bit_module_2 dpram1 (.clk(clk),.reset(reset),.inp(inp_dpram1),.outp(outp_dpram1)); 


 wire [509:0] inp_sysarr1;
wire [261:0] outp_sysarr1;

systolic_array_4_16bit_2 sysarr1 (.clk(clk),.reset(reset),.inp(inp_sysarr1),.outp(outp_sysarr1)); 
wire [159:0] inp_interface_1; 
wire [509:0] outp_interface_1; 

interface_1 inst_interface_1(.clk(clk),.reset(reset),.inp(inp_interface_1),.outp(outp_interface_1)); 


 wire [1571:0] inp_sysarr2;
wire [867:0] outp_sysarr2;

systolic_array_8_16bit_2 sysarr2 (.clk(clk),.reset(reset),.inp(inp_sysarr2),.outp(outp_sysarr2)); 
wire [639:0] inp_interface_2; 
wire [1571:0] outp_interface_2; 

interface_2 inst_interface_2(.clk(clk),.reset(reset),.inp(inp_interface_2),.outp(outp_interface_2)); 


 wire [782:0] inp_activ1;
wire [773:0] outp_activ1;

activation_32_8bit_module_3 activ1 (.clk(clk),.reset(reset),.inp(inp_activ1),.outp(outp_activ1)); 
wire [261:0] inp_interface_3; 
wire [782:0] outp_interface_3; 

interface_3 inst_interface_3(.clk(clk),.reset(reset),.inp(inp_interface_3),.outp(outp_interface_3)); 


 wire [1031:0] inp_activ2;
wire [1027:0] outp_activ2;

activation_32_16bit_module_2 activ2 (.clk(clk),.reset(reset),.inp(inp_activ2),.outp(outp_activ2)); 
wire [261:0] inp_interface_4; 
wire [1031:0] outp_interface_4; 

interface_4 inst_interface_4(.clk(clk),.reset(reset),.inp(inp_interface_4),.outp(outp_interface_4)); 


 wire [847:0] inp_dpram2;
wire [639:0] outp_dpram2;

dpram_4096_40bit_module_8 dpram2 (.clk(clk),.reset(reset),.inp(inp_dpram2),.outp(outp_dpram2)); 
wire [867:0] inp_interface_5; 
wire [847:0] outp_interface_5; 

interface_5 inst_interface_5(.clk(clk),.reset(reset),.inp(inp_interface_5),.outp(outp_interface_5)); 


 wire [1535:0] inp_adder_tree1;
wire [191:0] outp_adder_tree1;

adder_tree_4_16bit_6 adder_tree1 (.clk(clk),.reset(reset),.inp(inp_adder_tree1),.outp(outp_adder_tree1)); 
wire [2441:0] inp_interface_6; 
wire [1535:0] outp_interface_6; 

interface_6 inst_interface_6(.clk(clk),.reset(reset),.inp(inp_interface_6),.outp(outp_interface_6)); 


 wire [254:0] inp_sysarr3;
wire [130:0] outp_sysarr3;

systolic_array_4_16bit_1 sysarr3 (.clk(clk),.reset(reset),.inp(inp_sysarr3),.outp(outp_sysarr3)); 


 wire [575:0] inp_dpram3;
wire [479:0] outp_dpram3;

dpram_2048_60bit_module_4 dpram3 (.clk(clk),.reset(reset),.inp(inp_dpram3),.outp(outp_dpram3)); 
wire [130:0] inp_interface_8; 
wire [575:0] outp_interface_8; 

interface_8 inst_interface_8(.clk(clk),.reset(reset),.inp(inp_interface_8),.outp(outp_interface_8)); 


 wire [847:0] inp_dpram4;
wire [639:0] outp_dpram4;

dpram_4096_40bit_module_8 dpram4 (.clk(clk),.reset(reset),.inp(inp_dpram4),.outp(outp_dpram4)); 
wire [130:0] inp_interface_9; 
wire [847:0] outp_interface_9; 

interface_9 inst_interface_9(.clk(clk),.reset(reset),.inp(inp_interface_9),.outp(outp_interface_9)); 


 wire [63:0] inp_activ3;
wire [63:0] outp_activ3;

sigmoid_16bit_4 activ3 (.clk(clk),.reset(reset),.inp(inp_activ3),.outp(outp_activ3)); 
wire [130:0] inp_interface_10; 
wire [63:0] outp_interface_10; 

interface_10 inst_interface_10(.clk(clk),.reset(reset),.inp(inp_interface_10),.outp(outp_interface_10)); 


 wire [383:0] inp_adder_tree2;
wire [95:0] outp_adder_tree2;

adder_tree_3_8bit_6 adder_tree2 (.clk(clk),.reset(reset),.inp(inp_adder_tree2),.outp(outp_adder_tree2)); 
wire [1119:0] inp_interface_11; 
wire [383:0] outp_interface_11; 

interface_11 inst_interface_11(.clk(clk),.reset(reset),.inp(inp_interface_11),.outp(outp_interface_11)); 


 wire [511:0] inp_adder_tree3;
wire [63:0] outp_adder_tree3;

adder_tree_4_4bit_8 adder_tree3 (.clk(clk),.reset(reset),.inp(inp_adder_tree3),.outp(outp_adder_tree3)); 
wire [159:0] inp_interface_12; 
wire [511:0] outp_interface_12; 

interface_12 inst_interface_12(.clk(clk),.reset(reset),.inp(inp_interface_12),.outp(outp_interface_12)); 


 wire [785:0] inp_sysarr4;
wire [433:0] outp_sysarr4;

systolic_array_8_16bit_1 sysarr4 (.clk(clk),.reset(reset),.inp(inp_sysarr4),.outp(outp_sysarr4)); 
wire [255:0] inp_interface_13; 
wire [785:0] outp_interface_13; 

interface_13 inst_interface_13(.clk(clk),.reset(reset),.inp(inp_interface_13),.outp(outp_interface_13)); 


 wire [415:0] inp_dpram5;
wire [319:0] outp_dpram5;

dpram_2048_40bit_module_4 dpram5 (.clk(clk),.reset(reset),.inp(inp_dpram5),.outp(outp_dpram5)); 
wire [433:0] inp_interface_14; 
wire [415:0] outp_interface_14; 

interface_14 inst_interface_14(.clk(clk),.reset(reset),.inp(inp_interface_14),.outp(outp_interface_14)); 

assign inp_dpram1 = top_inp[207:0]; 

assign inp_sysarr1 = outp_interface_1; 
assign inp_interface_1 = {outp_dpram1}; 
 

assign inp_sysarr2 = outp_interface_2; 
assign inp_interface_2 = {outp_dpram1,outp_dpram3}; 
 

assign inp_activ1 = outp_interface_3; 
assign inp_interface_3 = {outp_sysarr1}; 
 

assign inp_activ2 = outp_interface_4; 
assign inp_interface_4 = {outp_sysarr1}; 
 

assign inp_dpram2 = outp_interface_5; 
assign inp_interface_5 = {outp_sysarr2}; 
 

assign inp_adder_tree1 = outp_interface_6; 
assign inp_interface_6 = {outp_activ1,outp_activ2,outp_dpram2}; 
 

assign inp_sysarr3 = top_inp[462:208]; 

assign inp_dpram3 = outp_interface_8; 
assign inp_interface_8 = {outp_sysarr3}; 
 

assign inp_dpram4 = outp_interface_9; 
assign inp_interface_9 = {outp_sysarr3}; 
 

assign inp_activ3 = outp_interface_10; 
assign inp_interface_10 = {outp_sysarr3}; 
 

assign inp_adder_tree2 = outp_interface_11; 
assign inp_interface_11 = {outp_dpram3,outp_dpram4}; 
 

assign inp_adder_tree3 = outp_interface_12; 
assign inp_interface_12 = {outp_adder_tree2,outp_activ3}; 
 

assign inp_sysarr4 = outp_interface_13; 
assign inp_interface_13 = {outp_adder_tree3,outp_adder_tree1}; 
 

assign inp_dpram5 = outp_interface_14; 
assign top_outp[319:0] = outp_dpram5; 
assign inp_interface_14 = {outp_sysarr4}; 
 

 endmodule 


module interface_1(input [159:0] inp, output reg [509:0] outp, input clk, input reset);
always@(posedge clk) begin 
outp[159:0] <= inp ; 
outp[319:160] <= inp ; 
outp[479:320] <= inp ; 
outp[509:480] <= inp[29:0] ; 
end 
endmodule 

module interface_2(input [639:0] inp, output reg [1571:0] outp, input clk, input reset);
always@(posedge clk) begin 
outp[639:0] <= inp ; 
outp[1279:640] <= inp ; 
outp[1571:1280] <= inp[291:0] ; 
end 
endmodule 

module interface_3(input [261:0] inp, output reg [782:0] outp, input clk, input reset);
always@(posedge clk) begin 
outp[261:0] <= inp ; 
outp[523:262] <= inp ; 
outp[782:524] <= inp[258:0] ; 
end 
endmodule 

module interface_4(input [261:0] inp, output reg [1031:0] outp, input clk, input reset);
always@(posedge clk) begin 
outp[261:0] <= inp ; 
outp[523:262] <= inp ; 
outp[785:524] <= inp ; 
outp[1031:786] <= inp[245:0] ; 
end 
endmodule 

module interface_5(input [867:0] inp, output reg [847:0] outp, input clk, input reset);
reg [867:0]intermediate_reg_0; 
always@(posedge clk) begin 
intermediate_reg_0 <= inp; 
end 
 
wire [433:0]intermediate_reg_1; 
 
mux_module mux_module_inst_1_0(.clk(clk),.reset(reset),.i1(intermediate_reg_0[867]),.i2(intermediate_reg_0[866]),.o(intermediate_reg_1[433]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1(.clk(clk),.reset(reset),.i1(intermediate_reg_0[865]),.i2(intermediate_reg_0[864]),.o(intermediate_reg_1[432]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_2(.clk(clk),.reset(reset),.i1(intermediate_reg_0[863]),.i2(intermediate_reg_0[862]),.o(intermediate_reg_1[431]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_3(.clk(clk),.reset(reset),.i1(intermediate_reg_0[861]),.i2(intermediate_reg_0[860]),.o(intermediate_reg_1[430]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_4(.clk(clk),.reset(reset),.i1(intermediate_reg_0[859]),.i2(intermediate_reg_0[858]),.o(intermediate_reg_1[429]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_5(.clk(clk),.reset(reset),.i1(intermediate_reg_0[857]),.i2(intermediate_reg_0[856]),.o(intermediate_reg_1[428]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_6(.clk(clk),.reset(reset),.i1(intermediate_reg_0[855]),.i2(intermediate_reg_0[854]),.o(intermediate_reg_1[427])); 
mux_module mux_module_inst_1_7(.clk(clk),.reset(reset),.i1(intermediate_reg_0[853]),.i2(intermediate_reg_0[852]),.o(intermediate_reg_1[426]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_8(.clk(clk),.reset(reset),.i1(intermediate_reg_0[851]),.i2(intermediate_reg_0[850]),.o(intermediate_reg_1[425])); 
mux_module mux_module_inst_1_9(.clk(clk),.reset(reset),.i1(intermediate_reg_0[849]),.i2(intermediate_reg_0[848]),.o(intermediate_reg_1[424]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_10(.clk(clk),.reset(reset),.i1(intermediate_reg_0[847]),.i2(intermediate_reg_0[846]),.o(intermediate_reg_1[423]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_11(.clk(clk),.reset(reset),.i1(intermediate_reg_0[845]),.i2(intermediate_reg_0[844]),.o(intermediate_reg_1[422])); 
mux_module mux_module_inst_1_12(.clk(clk),.reset(reset),.i1(intermediate_reg_0[843]),.i2(intermediate_reg_0[842]),.o(intermediate_reg_1[421]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_13(.clk(clk),.reset(reset),.i1(intermediate_reg_0[841]),.i2(intermediate_reg_0[840]),.o(intermediate_reg_1[420])); 
xor_module xor_module_inst_1_14(.clk(clk),.reset(reset),.i1(intermediate_reg_0[839]),.i2(intermediate_reg_0[838]),.o(intermediate_reg_1[419])); 
mux_module mux_module_inst_1_15(.clk(clk),.reset(reset),.i1(intermediate_reg_0[837]),.i2(intermediate_reg_0[836]),.o(intermediate_reg_1[418]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_16(.clk(clk),.reset(reset),.i1(intermediate_reg_0[835]),.i2(intermediate_reg_0[834]),.o(intermediate_reg_1[417]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_17(.clk(clk),.reset(reset),.i1(intermediate_reg_0[833]),.i2(intermediate_reg_0[832]),.o(intermediate_reg_1[416])); 
mux_module mux_module_inst_1_18(.clk(clk),.reset(reset),.i1(intermediate_reg_0[831]),.i2(intermediate_reg_0[830]),.o(intermediate_reg_1[415]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_19(.clk(clk),.reset(reset),.i1(intermediate_reg_0[829]),.i2(intermediate_reg_0[828]),.o(intermediate_reg_1[414]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_20(.clk(clk),.reset(reset),.i1(intermediate_reg_0[827]),.i2(intermediate_reg_0[826]),.o(intermediate_reg_1[413])); 
xor_module xor_module_inst_1_21(.clk(clk),.reset(reset),.i1(intermediate_reg_0[825]),.i2(intermediate_reg_0[824]),.o(intermediate_reg_1[412])); 
xor_module xor_module_inst_1_22(.clk(clk),.reset(reset),.i1(intermediate_reg_0[823]),.i2(intermediate_reg_0[822]),.o(intermediate_reg_1[411])); 
mux_module mux_module_inst_1_23(.clk(clk),.reset(reset),.i1(intermediate_reg_0[821]),.i2(intermediate_reg_0[820]),.o(intermediate_reg_1[410]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_24(.clk(clk),.reset(reset),.i1(intermediate_reg_0[819]),.i2(intermediate_reg_0[818]),.o(intermediate_reg_1[409])); 
mux_module mux_module_inst_1_25(.clk(clk),.reset(reset),.i1(intermediate_reg_0[817]),.i2(intermediate_reg_0[816]),.o(intermediate_reg_1[408]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_26(.clk(clk),.reset(reset),.i1(intermediate_reg_0[815]),.i2(intermediate_reg_0[814]),.o(intermediate_reg_1[407]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_27(.clk(clk),.reset(reset),.i1(intermediate_reg_0[813]),.i2(intermediate_reg_0[812]),.o(intermediate_reg_1[406]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_28(.clk(clk),.reset(reset),.i1(intermediate_reg_0[811]),.i2(intermediate_reg_0[810]),.o(intermediate_reg_1[405]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_29(.clk(clk),.reset(reset),.i1(intermediate_reg_0[809]),.i2(intermediate_reg_0[808]),.o(intermediate_reg_1[404]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_30(.clk(clk),.reset(reset),.i1(intermediate_reg_0[807]),.i2(intermediate_reg_0[806]),.o(intermediate_reg_1[403])); 
mux_module mux_module_inst_1_31(.clk(clk),.reset(reset),.i1(intermediate_reg_0[805]),.i2(intermediate_reg_0[804]),.o(intermediate_reg_1[402]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_32(.clk(clk),.reset(reset),.i1(intermediate_reg_0[803]),.i2(intermediate_reg_0[802]),.o(intermediate_reg_1[401]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_33(.clk(clk),.reset(reset),.i1(intermediate_reg_0[801]),.i2(intermediate_reg_0[800]),.o(intermediate_reg_1[400]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_34(.clk(clk),.reset(reset),.i1(intermediate_reg_0[799]),.i2(intermediate_reg_0[798]),.o(intermediate_reg_1[399])); 
mux_module mux_module_inst_1_35(.clk(clk),.reset(reset),.i1(intermediate_reg_0[797]),.i2(intermediate_reg_0[796]),.o(intermediate_reg_1[398]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_36(.clk(clk),.reset(reset),.i1(intermediate_reg_0[795]),.i2(intermediate_reg_0[794]),.o(intermediate_reg_1[397]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_37(.clk(clk),.reset(reset),.i1(intermediate_reg_0[793]),.i2(intermediate_reg_0[792]),.o(intermediate_reg_1[396]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_38(.clk(clk),.reset(reset),.i1(intermediate_reg_0[791]),.i2(intermediate_reg_0[790]),.o(intermediate_reg_1[395]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_39(.clk(clk),.reset(reset),.i1(intermediate_reg_0[789]),.i2(intermediate_reg_0[788]),.o(intermediate_reg_1[394])); 
mux_module mux_module_inst_1_40(.clk(clk),.reset(reset),.i1(intermediate_reg_0[787]),.i2(intermediate_reg_0[786]),.o(intermediate_reg_1[393]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_41(.clk(clk),.reset(reset),.i1(intermediate_reg_0[785]),.i2(intermediate_reg_0[784]),.o(intermediate_reg_1[392])); 
mux_module mux_module_inst_1_42(.clk(clk),.reset(reset),.i1(intermediate_reg_0[783]),.i2(intermediate_reg_0[782]),.o(intermediate_reg_1[391]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_43(.clk(clk),.reset(reset),.i1(intermediate_reg_0[781]),.i2(intermediate_reg_0[780]),.o(intermediate_reg_1[390]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_44(.clk(clk),.reset(reset),.i1(intermediate_reg_0[779]),.i2(intermediate_reg_0[778]),.o(intermediate_reg_1[389]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_45(.clk(clk),.reset(reset),.i1(intermediate_reg_0[777]),.i2(intermediate_reg_0[776]),.o(intermediate_reg_1[388]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_46(.clk(clk),.reset(reset),.i1(intermediate_reg_0[775]),.i2(intermediate_reg_0[774]),.o(intermediate_reg_1[387]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_47(.clk(clk),.reset(reset),.i1(intermediate_reg_0[773]),.i2(intermediate_reg_0[772]),.o(intermediate_reg_1[386])); 
xor_module xor_module_inst_1_48(.clk(clk),.reset(reset),.i1(intermediate_reg_0[771]),.i2(intermediate_reg_0[770]),.o(intermediate_reg_1[385])); 
xor_module xor_module_inst_1_49(.clk(clk),.reset(reset),.i1(intermediate_reg_0[769]),.i2(intermediate_reg_0[768]),.o(intermediate_reg_1[384])); 
mux_module mux_module_inst_1_50(.clk(clk),.reset(reset),.i1(intermediate_reg_0[767]),.i2(intermediate_reg_0[766]),.o(intermediate_reg_1[383]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_51(.clk(clk),.reset(reset),.i1(intermediate_reg_0[765]),.i2(intermediate_reg_0[764]),.o(intermediate_reg_1[382]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_52(.clk(clk),.reset(reset),.i1(intermediate_reg_0[763]),.i2(intermediate_reg_0[762]),.o(intermediate_reg_1[381]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_53(.clk(clk),.reset(reset),.i1(intermediate_reg_0[761]),.i2(intermediate_reg_0[760]),.o(intermediate_reg_1[380])); 
mux_module mux_module_inst_1_54(.clk(clk),.reset(reset),.i1(intermediate_reg_0[759]),.i2(intermediate_reg_0[758]),.o(intermediate_reg_1[379]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_55(.clk(clk),.reset(reset),.i1(intermediate_reg_0[757]),.i2(intermediate_reg_0[756]),.o(intermediate_reg_1[378]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_56(.clk(clk),.reset(reset),.i1(intermediate_reg_0[755]),.i2(intermediate_reg_0[754]),.o(intermediate_reg_1[377])); 
xor_module xor_module_inst_1_57(.clk(clk),.reset(reset),.i1(intermediate_reg_0[753]),.i2(intermediate_reg_0[752]),.o(intermediate_reg_1[376])); 
mux_module mux_module_inst_1_58(.clk(clk),.reset(reset),.i1(intermediate_reg_0[751]),.i2(intermediate_reg_0[750]),.o(intermediate_reg_1[375]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_59(.clk(clk),.reset(reset),.i1(intermediate_reg_0[749]),.i2(intermediate_reg_0[748]),.o(intermediate_reg_1[374])); 
mux_module mux_module_inst_1_60(.clk(clk),.reset(reset),.i1(intermediate_reg_0[747]),.i2(intermediate_reg_0[746]),.o(intermediate_reg_1[373]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_61(.clk(clk),.reset(reset),.i1(intermediate_reg_0[745]),.i2(intermediate_reg_0[744]),.o(intermediate_reg_1[372])); 
xor_module xor_module_inst_1_62(.clk(clk),.reset(reset),.i1(intermediate_reg_0[743]),.i2(intermediate_reg_0[742]),.o(intermediate_reg_1[371])); 
mux_module mux_module_inst_1_63(.clk(clk),.reset(reset),.i1(intermediate_reg_0[741]),.i2(intermediate_reg_0[740]),.o(intermediate_reg_1[370]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_64(.clk(clk),.reset(reset),.i1(intermediate_reg_0[739]),.i2(intermediate_reg_0[738]),.o(intermediate_reg_1[369]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_65(.clk(clk),.reset(reset),.i1(intermediate_reg_0[737]),.i2(intermediate_reg_0[736]),.o(intermediate_reg_1[368])); 
mux_module mux_module_inst_1_66(.clk(clk),.reset(reset),.i1(intermediate_reg_0[735]),.i2(intermediate_reg_0[734]),.o(intermediate_reg_1[367]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_67(.clk(clk),.reset(reset),.i1(intermediate_reg_0[733]),.i2(intermediate_reg_0[732]),.o(intermediate_reg_1[366]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_68(.clk(clk),.reset(reset),.i1(intermediate_reg_0[731]),.i2(intermediate_reg_0[730]),.o(intermediate_reg_1[365]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_69(.clk(clk),.reset(reset),.i1(intermediate_reg_0[729]),.i2(intermediate_reg_0[728]),.o(intermediate_reg_1[364])); 
mux_module mux_module_inst_1_70(.clk(clk),.reset(reset),.i1(intermediate_reg_0[727]),.i2(intermediate_reg_0[726]),.o(intermediate_reg_1[363]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_71(.clk(clk),.reset(reset),.i1(intermediate_reg_0[725]),.i2(intermediate_reg_0[724]),.o(intermediate_reg_1[362])); 
mux_module mux_module_inst_1_72(.clk(clk),.reset(reset),.i1(intermediate_reg_0[723]),.i2(intermediate_reg_0[722]),.o(intermediate_reg_1[361]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_73(.clk(clk),.reset(reset),.i1(intermediate_reg_0[721]),.i2(intermediate_reg_0[720]),.o(intermediate_reg_1[360]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_74(.clk(clk),.reset(reset),.i1(intermediate_reg_0[719]),.i2(intermediate_reg_0[718]),.o(intermediate_reg_1[359]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_75(.clk(clk),.reset(reset),.i1(intermediate_reg_0[717]),.i2(intermediate_reg_0[716]),.o(intermediate_reg_1[358])); 
mux_module mux_module_inst_1_76(.clk(clk),.reset(reset),.i1(intermediate_reg_0[715]),.i2(intermediate_reg_0[714]),.o(intermediate_reg_1[357]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_77(.clk(clk),.reset(reset),.i1(intermediate_reg_0[713]),.i2(intermediate_reg_0[712]),.o(intermediate_reg_1[356]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_78(.clk(clk),.reset(reset),.i1(intermediate_reg_0[711]),.i2(intermediate_reg_0[710]),.o(intermediate_reg_1[355]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_79(.clk(clk),.reset(reset),.i1(intermediate_reg_0[709]),.i2(intermediate_reg_0[708]),.o(intermediate_reg_1[354]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_80(.clk(clk),.reset(reset),.i1(intermediate_reg_0[707]),.i2(intermediate_reg_0[706]),.o(intermediate_reg_1[353]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_81(.clk(clk),.reset(reset),.i1(intermediate_reg_0[705]),.i2(intermediate_reg_0[704]),.o(intermediate_reg_1[352])); 
xor_module xor_module_inst_1_82(.clk(clk),.reset(reset),.i1(intermediate_reg_0[703]),.i2(intermediate_reg_0[702]),.o(intermediate_reg_1[351])); 
xor_module xor_module_inst_1_83(.clk(clk),.reset(reset),.i1(intermediate_reg_0[701]),.i2(intermediate_reg_0[700]),.o(intermediate_reg_1[350])); 
xor_module xor_module_inst_1_84(.clk(clk),.reset(reset),.i1(intermediate_reg_0[699]),.i2(intermediate_reg_0[698]),.o(intermediate_reg_1[349])); 
xor_module xor_module_inst_1_85(.clk(clk),.reset(reset),.i1(intermediate_reg_0[697]),.i2(intermediate_reg_0[696]),.o(intermediate_reg_1[348])); 
mux_module mux_module_inst_1_86(.clk(clk),.reset(reset),.i1(intermediate_reg_0[695]),.i2(intermediate_reg_0[694]),.o(intermediate_reg_1[347]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_87(.clk(clk),.reset(reset),.i1(intermediate_reg_0[693]),.i2(intermediate_reg_0[692]),.o(intermediate_reg_1[346])); 
xor_module xor_module_inst_1_88(.clk(clk),.reset(reset),.i1(intermediate_reg_0[691]),.i2(intermediate_reg_0[690]),.o(intermediate_reg_1[345])); 
mux_module mux_module_inst_1_89(.clk(clk),.reset(reset),.i1(intermediate_reg_0[689]),.i2(intermediate_reg_0[688]),.o(intermediate_reg_1[344]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_90(.clk(clk),.reset(reset),.i1(intermediate_reg_0[687]),.i2(intermediate_reg_0[686]),.o(intermediate_reg_1[343])); 
xor_module xor_module_inst_1_91(.clk(clk),.reset(reset),.i1(intermediate_reg_0[685]),.i2(intermediate_reg_0[684]),.o(intermediate_reg_1[342])); 
xor_module xor_module_inst_1_92(.clk(clk),.reset(reset),.i1(intermediate_reg_0[683]),.i2(intermediate_reg_0[682]),.o(intermediate_reg_1[341])); 
xor_module xor_module_inst_1_93(.clk(clk),.reset(reset),.i1(intermediate_reg_0[681]),.i2(intermediate_reg_0[680]),.o(intermediate_reg_1[340])); 
mux_module mux_module_inst_1_94(.clk(clk),.reset(reset),.i1(intermediate_reg_0[679]),.i2(intermediate_reg_0[678]),.o(intermediate_reg_1[339]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_95(.clk(clk),.reset(reset),.i1(intermediate_reg_0[677]),.i2(intermediate_reg_0[676]),.o(intermediate_reg_1[338]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_96(.clk(clk),.reset(reset),.i1(intermediate_reg_0[675]),.i2(intermediate_reg_0[674]),.o(intermediate_reg_1[337]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_97(.clk(clk),.reset(reset),.i1(intermediate_reg_0[673]),.i2(intermediate_reg_0[672]),.o(intermediate_reg_1[336]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_98(.clk(clk),.reset(reset),.i1(intermediate_reg_0[671]),.i2(intermediate_reg_0[670]),.o(intermediate_reg_1[335])); 
xor_module xor_module_inst_1_99(.clk(clk),.reset(reset),.i1(intermediate_reg_0[669]),.i2(intermediate_reg_0[668]),.o(intermediate_reg_1[334])); 
xor_module xor_module_inst_1_100(.clk(clk),.reset(reset),.i1(intermediate_reg_0[667]),.i2(intermediate_reg_0[666]),.o(intermediate_reg_1[333])); 
xor_module xor_module_inst_1_101(.clk(clk),.reset(reset),.i1(intermediate_reg_0[665]),.i2(intermediate_reg_0[664]),.o(intermediate_reg_1[332])); 
mux_module mux_module_inst_1_102(.clk(clk),.reset(reset),.i1(intermediate_reg_0[663]),.i2(intermediate_reg_0[662]),.o(intermediate_reg_1[331]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_103(.clk(clk),.reset(reset),.i1(intermediate_reg_0[661]),.i2(intermediate_reg_0[660]),.o(intermediate_reg_1[330])); 
mux_module mux_module_inst_1_104(.clk(clk),.reset(reset),.i1(intermediate_reg_0[659]),.i2(intermediate_reg_0[658]),.o(intermediate_reg_1[329]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_105(.clk(clk),.reset(reset),.i1(intermediate_reg_0[657]),.i2(intermediate_reg_0[656]),.o(intermediate_reg_1[328]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_106(.clk(clk),.reset(reset),.i1(intermediate_reg_0[655]),.i2(intermediate_reg_0[654]),.o(intermediate_reg_1[327])); 
xor_module xor_module_inst_1_107(.clk(clk),.reset(reset),.i1(intermediate_reg_0[653]),.i2(intermediate_reg_0[652]),.o(intermediate_reg_1[326])); 
mux_module mux_module_inst_1_108(.clk(clk),.reset(reset),.i1(intermediate_reg_0[651]),.i2(intermediate_reg_0[650]),.o(intermediate_reg_1[325]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_109(.clk(clk),.reset(reset),.i1(intermediate_reg_0[649]),.i2(intermediate_reg_0[648]),.o(intermediate_reg_1[324])); 
xor_module xor_module_inst_1_110(.clk(clk),.reset(reset),.i1(intermediate_reg_0[647]),.i2(intermediate_reg_0[646]),.o(intermediate_reg_1[323])); 
xor_module xor_module_inst_1_111(.clk(clk),.reset(reset),.i1(intermediate_reg_0[645]),.i2(intermediate_reg_0[644]),.o(intermediate_reg_1[322])); 
mux_module mux_module_inst_1_112(.clk(clk),.reset(reset),.i1(intermediate_reg_0[643]),.i2(intermediate_reg_0[642]),.o(intermediate_reg_1[321]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_113(.clk(clk),.reset(reset),.i1(intermediate_reg_0[641]),.i2(intermediate_reg_0[640]),.o(intermediate_reg_1[320]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_114(.clk(clk),.reset(reset),.i1(intermediate_reg_0[639]),.i2(intermediate_reg_0[638]),.o(intermediate_reg_1[319]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_115(.clk(clk),.reset(reset),.i1(intermediate_reg_0[637]),.i2(intermediate_reg_0[636]),.o(intermediate_reg_1[318]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_116(.clk(clk),.reset(reset),.i1(intermediate_reg_0[635]),.i2(intermediate_reg_0[634]),.o(intermediate_reg_1[317])); 
mux_module mux_module_inst_1_117(.clk(clk),.reset(reset),.i1(intermediate_reg_0[633]),.i2(intermediate_reg_0[632]),.o(intermediate_reg_1[316]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_118(.clk(clk),.reset(reset),.i1(intermediate_reg_0[631]),.i2(intermediate_reg_0[630]),.o(intermediate_reg_1[315])); 
mux_module mux_module_inst_1_119(.clk(clk),.reset(reset),.i1(intermediate_reg_0[629]),.i2(intermediate_reg_0[628]),.o(intermediate_reg_1[314]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_120(.clk(clk),.reset(reset),.i1(intermediate_reg_0[627]),.i2(intermediate_reg_0[626]),.o(intermediate_reg_1[313]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_121(.clk(clk),.reset(reset),.i1(intermediate_reg_0[625]),.i2(intermediate_reg_0[624]),.o(intermediate_reg_1[312])); 
mux_module mux_module_inst_1_122(.clk(clk),.reset(reset),.i1(intermediate_reg_0[623]),.i2(intermediate_reg_0[622]),.o(intermediate_reg_1[311]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_123(.clk(clk),.reset(reset),.i1(intermediate_reg_0[621]),.i2(intermediate_reg_0[620]),.o(intermediate_reg_1[310]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_124(.clk(clk),.reset(reset),.i1(intermediate_reg_0[619]),.i2(intermediate_reg_0[618]),.o(intermediate_reg_1[309]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_125(.clk(clk),.reset(reset),.i1(intermediate_reg_0[617]),.i2(intermediate_reg_0[616]),.o(intermediate_reg_1[308])); 
xor_module xor_module_inst_1_126(.clk(clk),.reset(reset),.i1(intermediate_reg_0[615]),.i2(intermediate_reg_0[614]),.o(intermediate_reg_1[307])); 
mux_module mux_module_inst_1_127(.clk(clk),.reset(reset),.i1(intermediate_reg_0[613]),.i2(intermediate_reg_0[612]),.o(intermediate_reg_1[306]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_128(.clk(clk),.reset(reset),.i1(intermediate_reg_0[611]),.i2(intermediate_reg_0[610]),.o(intermediate_reg_1[305]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_129(.clk(clk),.reset(reset),.i1(intermediate_reg_0[609]),.i2(intermediate_reg_0[608]),.o(intermediate_reg_1[304]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_130(.clk(clk),.reset(reset),.i1(intermediate_reg_0[607]),.i2(intermediate_reg_0[606]),.o(intermediate_reg_1[303])); 
xor_module xor_module_inst_1_131(.clk(clk),.reset(reset),.i1(intermediate_reg_0[605]),.i2(intermediate_reg_0[604]),.o(intermediate_reg_1[302])); 
mux_module mux_module_inst_1_132(.clk(clk),.reset(reset),.i1(intermediate_reg_0[603]),.i2(intermediate_reg_0[602]),.o(intermediate_reg_1[301]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_133(.clk(clk),.reset(reset),.i1(intermediate_reg_0[601]),.i2(intermediate_reg_0[600]),.o(intermediate_reg_1[300])); 
mux_module mux_module_inst_1_134(.clk(clk),.reset(reset),.i1(intermediate_reg_0[599]),.i2(intermediate_reg_0[598]),.o(intermediate_reg_1[299]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_135(.clk(clk),.reset(reset),.i1(intermediate_reg_0[597]),.i2(intermediate_reg_0[596]),.o(intermediate_reg_1[298]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_136(.clk(clk),.reset(reset),.i1(intermediate_reg_0[595]),.i2(intermediate_reg_0[594]),.o(intermediate_reg_1[297]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_137(.clk(clk),.reset(reset),.i1(intermediate_reg_0[593]),.i2(intermediate_reg_0[592]),.o(intermediate_reg_1[296])); 
mux_module mux_module_inst_1_138(.clk(clk),.reset(reset),.i1(intermediate_reg_0[591]),.i2(intermediate_reg_0[590]),.o(intermediate_reg_1[295]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_139(.clk(clk),.reset(reset),.i1(intermediate_reg_0[589]),.i2(intermediate_reg_0[588]),.o(intermediate_reg_1[294])); 
xor_module xor_module_inst_1_140(.clk(clk),.reset(reset),.i1(intermediate_reg_0[587]),.i2(intermediate_reg_0[586]),.o(intermediate_reg_1[293])); 
xor_module xor_module_inst_1_141(.clk(clk),.reset(reset),.i1(intermediate_reg_0[585]),.i2(intermediate_reg_0[584]),.o(intermediate_reg_1[292])); 
mux_module mux_module_inst_1_142(.clk(clk),.reset(reset),.i1(intermediate_reg_0[583]),.i2(intermediate_reg_0[582]),.o(intermediate_reg_1[291]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_143(.clk(clk),.reset(reset),.i1(intermediate_reg_0[581]),.i2(intermediate_reg_0[580]),.o(intermediate_reg_1[290])); 
mux_module mux_module_inst_1_144(.clk(clk),.reset(reset),.i1(intermediate_reg_0[579]),.i2(intermediate_reg_0[578]),.o(intermediate_reg_1[289]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_145(.clk(clk),.reset(reset),.i1(intermediate_reg_0[577]),.i2(intermediate_reg_0[576]),.o(intermediate_reg_1[288])); 
mux_module mux_module_inst_1_146(.clk(clk),.reset(reset),.i1(intermediate_reg_0[575]),.i2(intermediate_reg_0[574]),.o(intermediate_reg_1[287]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_147(.clk(clk),.reset(reset),.i1(intermediate_reg_0[573]),.i2(intermediate_reg_0[572]),.o(intermediate_reg_1[286])); 
xor_module xor_module_inst_1_148(.clk(clk),.reset(reset),.i1(intermediate_reg_0[571]),.i2(intermediate_reg_0[570]),.o(intermediate_reg_1[285])); 
mux_module mux_module_inst_1_149(.clk(clk),.reset(reset),.i1(intermediate_reg_0[569]),.i2(intermediate_reg_0[568]),.o(intermediate_reg_1[284]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_150(.clk(clk),.reset(reset),.i1(intermediate_reg_0[567]),.i2(intermediate_reg_0[566]),.o(intermediate_reg_1[283]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_151(.clk(clk),.reset(reset),.i1(intermediate_reg_0[565]),.i2(intermediate_reg_0[564]),.o(intermediate_reg_1[282])); 
xor_module xor_module_inst_1_152(.clk(clk),.reset(reset),.i1(intermediate_reg_0[563]),.i2(intermediate_reg_0[562]),.o(intermediate_reg_1[281])); 
mux_module mux_module_inst_1_153(.clk(clk),.reset(reset),.i1(intermediate_reg_0[561]),.i2(intermediate_reg_0[560]),.o(intermediate_reg_1[280]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_154(.clk(clk),.reset(reset),.i1(intermediate_reg_0[559]),.i2(intermediate_reg_0[558]),.o(intermediate_reg_1[279])); 
xor_module xor_module_inst_1_155(.clk(clk),.reset(reset),.i1(intermediate_reg_0[557]),.i2(intermediate_reg_0[556]),.o(intermediate_reg_1[278])); 
xor_module xor_module_inst_1_156(.clk(clk),.reset(reset),.i1(intermediate_reg_0[555]),.i2(intermediate_reg_0[554]),.o(intermediate_reg_1[277])); 
xor_module xor_module_inst_1_157(.clk(clk),.reset(reset),.i1(intermediate_reg_0[553]),.i2(intermediate_reg_0[552]),.o(intermediate_reg_1[276])); 
xor_module xor_module_inst_1_158(.clk(clk),.reset(reset),.i1(intermediate_reg_0[551]),.i2(intermediate_reg_0[550]),.o(intermediate_reg_1[275])); 
xor_module xor_module_inst_1_159(.clk(clk),.reset(reset),.i1(intermediate_reg_0[549]),.i2(intermediate_reg_0[548]),.o(intermediate_reg_1[274])); 
mux_module mux_module_inst_1_160(.clk(clk),.reset(reset),.i1(intermediate_reg_0[547]),.i2(intermediate_reg_0[546]),.o(intermediate_reg_1[273]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_161(.clk(clk),.reset(reset),.i1(intermediate_reg_0[545]),.i2(intermediate_reg_0[544]),.o(intermediate_reg_1[272]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_162(.clk(clk),.reset(reset),.i1(intermediate_reg_0[543]),.i2(intermediate_reg_0[542]),.o(intermediate_reg_1[271])); 
mux_module mux_module_inst_1_163(.clk(clk),.reset(reset),.i1(intermediate_reg_0[541]),.i2(intermediate_reg_0[540]),.o(intermediate_reg_1[270]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_164(.clk(clk),.reset(reset),.i1(intermediate_reg_0[539]),.i2(intermediate_reg_0[538]),.o(intermediate_reg_1[269]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_165(.clk(clk),.reset(reset),.i1(intermediate_reg_0[537]),.i2(intermediate_reg_0[536]),.o(intermediate_reg_1[268])); 
mux_module mux_module_inst_1_166(.clk(clk),.reset(reset),.i1(intermediate_reg_0[535]),.i2(intermediate_reg_0[534]),.o(intermediate_reg_1[267]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_167(.clk(clk),.reset(reset),.i1(intermediate_reg_0[533]),.i2(intermediate_reg_0[532]),.o(intermediate_reg_1[266]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_168(.clk(clk),.reset(reset),.i1(intermediate_reg_0[531]),.i2(intermediate_reg_0[530]),.o(intermediate_reg_1[265]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_169(.clk(clk),.reset(reset),.i1(intermediate_reg_0[529]),.i2(intermediate_reg_0[528]),.o(intermediate_reg_1[264]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_170(.clk(clk),.reset(reset),.i1(intermediate_reg_0[527]),.i2(intermediate_reg_0[526]),.o(intermediate_reg_1[263])); 
xor_module xor_module_inst_1_171(.clk(clk),.reset(reset),.i1(intermediate_reg_0[525]),.i2(intermediate_reg_0[524]),.o(intermediate_reg_1[262])); 
xor_module xor_module_inst_1_172(.clk(clk),.reset(reset),.i1(intermediate_reg_0[523]),.i2(intermediate_reg_0[522]),.o(intermediate_reg_1[261])); 
mux_module mux_module_inst_1_173(.clk(clk),.reset(reset),.i1(intermediate_reg_0[521]),.i2(intermediate_reg_0[520]),.o(intermediate_reg_1[260]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_174(.clk(clk),.reset(reset),.i1(intermediate_reg_0[519]),.i2(intermediate_reg_0[518]),.o(intermediate_reg_1[259])); 
xor_module xor_module_inst_1_175(.clk(clk),.reset(reset),.i1(intermediate_reg_0[517]),.i2(intermediate_reg_0[516]),.o(intermediate_reg_1[258])); 
mux_module mux_module_inst_1_176(.clk(clk),.reset(reset),.i1(intermediate_reg_0[515]),.i2(intermediate_reg_0[514]),.o(intermediate_reg_1[257]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_177(.clk(clk),.reset(reset),.i1(intermediate_reg_0[513]),.i2(intermediate_reg_0[512]),.o(intermediate_reg_1[256])); 
mux_module mux_module_inst_1_178(.clk(clk),.reset(reset),.i1(intermediate_reg_0[511]),.i2(intermediate_reg_0[510]),.o(intermediate_reg_1[255]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_179(.clk(clk),.reset(reset),.i1(intermediate_reg_0[509]),.i2(intermediate_reg_0[508]),.o(intermediate_reg_1[254])); 
xor_module xor_module_inst_1_180(.clk(clk),.reset(reset),.i1(intermediate_reg_0[507]),.i2(intermediate_reg_0[506]),.o(intermediate_reg_1[253])); 
mux_module mux_module_inst_1_181(.clk(clk),.reset(reset),.i1(intermediate_reg_0[505]),.i2(intermediate_reg_0[504]),.o(intermediate_reg_1[252]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_182(.clk(clk),.reset(reset),.i1(intermediate_reg_0[503]),.i2(intermediate_reg_0[502]),.o(intermediate_reg_1[251])); 
xor_module xor_module_inst_1_183(.clk(clk),.reset(reset),.i1(intermediate_reg_0[501]),.i2(intermediate_reg_0[500]),.o(intermediate_reg_1[250])); 
mux_module mux_module_inst_1_184(.clk(clk),.reset(reset),.i1(intermediate_reg_0[499]),.i2(intermediate_reg_0[498]),.o(intermediate_reg_1[249]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_185(.clk(clk),.reset(reset),.i1(intermediate_reg_0[497]),.i2(intermediate_reg_0[496]),.o(intermediate_reg_1[248])); 
xor_module xor_module_inst_1_186(.clk(clk),.reset(reset),.i1(intermediate_reg_0[495]),.i2(intermediate_reg_0[494]),.o(intermediate_reg_1[247])); 
xor_module xor_module_inst_1_187(.clk(clk),.reset(reset),.i1(intermediate_reg_0[493]),.i2(intermediate_reg_0[492]),.o(intermediate_reg_1[246])); 
xor_module xor_module_inst_1_188(.clk(clk),.reset(reset),.i1(intermediate_reg_0[491]),.i2(intermediate_reg_0[490]),.o(intermediate_reg_1[245])); 
xor_module xor_module_inst_1_189(.clk(clk),.reset(reset),.i1(intermediate_reg_0[489]),.i2(intermediate_reg_0[488]),.o(intermediate_reg_1[244])); 
mux_module mux_module_inst_1_190(.clk(clk),.reset(reset),.i1(intermediate_reg_0[487]),.i2(intermediate_reg_0[486]),.o(intermediate_reg_1[243]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_191(.clk(clk),.reset(reset),.i1(intermediate_reg_0[485]),.i2(intermediate_reg_0[484]),.o(intermediate_reg_1[242]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_192(.clk(clk),.reset(reset),.i1(intermediate_reg_0[483]),.i2(intermediate_reg_0[482]),.o(intermediate_reg_1[241])); 
mux_module mux_module_inst_1_193(.clk(clk),.reset(reset),.i1(intermediate_reg_0[481]),.i2(intermediate_reg_0[480]),.o(intermediate_reg_1[240]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_194(.clk(clk),.reset(reset),.i1(intermediate_reg_0[479]),.i2(intermediate_reg_0[478]),.o(intermediate_reg_1[239])); 
xor_module xor_module_inst_1_195(.clk(clk),.reset(reset),.i1(intermediate_reg_0[477]),.i2(intermediate_reg_0[476]),.o(intermediate_reg_1[238])); 
mux_module mux_module_inst_1_196(.clk(clk),.reset(reset),.i1(intermediate_reg_0[475]),.i2(intermediate_reg_0[474]),.o(intermediate_reg_1[237]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_197(.clk(clk),.reset(reset),.i1(intermediate_reg_0[473]),.i2(intermediate_reg_0[472]),.o(intermediate_reg_1[236]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_198(.clk(clk),.reset(reset),.i1(intermediate_reg_0[471]),.i2(intermediate_reg_0[470]),.o(intermediate_reg_1[235]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_199(.clk(clk),.reset(reset),.i1(intermediate_reg_0[469]),.i2(intermediate_reg_0[468]),.o(intermediate_reg_1[234]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_200(.clk(clk),.reset(reset),.i1(intermediate_reg_0[467]),.i2(intermediate_reg_0[466]),.o(intermediate_reg_1[233]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_201(.clk(clk),.reset(reset),.i1(intermediate_reg_0[465]),.i2(intermediate_reg_0[464]),.o(intermediate_reg_1[232]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_202(.clk(clk),.reset(reset),.i1(intermediate_reg_0[463]),.i2(intermediate_reg_0[462]),.o(intermediate_reg_1[231]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_203(.clk(clk),.reset(reset),.i1(intermediate_reg_0[461]),.i2(intermediate_reg_0[460]),.o(intermediate_reg_1[230])); 
xor_module xor_module_inst_1_204(.clk(clk),.reset(reset),.i1(intermediate_reg_0[459]),.i2(intermediate_reg_0[458]),.o(intermediate_reg_1[229])); 
mux_module mux_module_inst_1_205(.clk(clk),.reset(reset),.i1(intermediate_reg_0[457]),.i2(intermediate_reg_0[456]),.o(intermediate_reg_1[228]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_206(.clk(clk),.reset(reset),.i1(intermediate_reg_0[455]),.i2(intermediate_reg_0[454]),.o(intermediate_reg_1[227])); 
mux_module mux_module_inst_1_207(.clk(clk),.reset(reset),.i1(intermediate_reg_0[453]),.i2(intermediate_reg_0[452]),.o(intermediate_reg_1[226]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_208(.clk(clk),.reset(reset),.i1(intermediate_reg_0[451]),.i2(intermediate_reg_0[450]),.o(intermediate_reg_1[225])); 
mux_module mux_module_inst_1_209(.clk(clk),.reset(reset),.i1(intermediate_reg_0[449]),.i2(intermediate_reg_0[448]),.o(intermediate_reg_1[224]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_210(.clk(clk),.reset(reset),.i1(intermediate_reg_0[447]),.i2(intermediate_reg_0[446]),.o(intermediate_reg_1[223])); 
xor_module xor_module_inst_1_211(.clk(clk),.reset(reset),.i1(intermediate_reg_0[445]),.i2(intermediate_reg_0[444]),.o(intermediate_reg_1[222])); 
xor_module xor_module_inst_1_212(.clk(clk),.reset(reset),.i1(intermediate_reg_0[443]),.i2(intermediate_reg_0[442]),.o(intermediate_reg_1[221])); 
xor_module xor_module_inst_1_213(.clk(clk),.reset(reset),.i1(intermediate_reg_0[441]),.i2(intermediate_reg_0[440]),.o(intermediate_reg_1[220])); 
mux_module mux_module_inst_1_214(.clk(clk),.reset(reset),.i1(intermediate_reg_0[439]),.i2(intermediate_reg_0[438]),.o(intermediate_reg_1[219]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_215(.clk(clk),.reset(reset),.i1(intermediate_reg_0[437]),.i2(intermediate_reg_0[436]),.o(intermediate_reg_1[218]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_216(.clk(clk),.reset(reset),.i1(intermediate_reg_0[435]),.i2(intermediate_reg_0[434]),.o(intermediate_reg_1[217])); 
mux_module mux_module_inst_1_217(.clk(clk),.reset(reset),.i1(intermediate_reg_0[433]),.i2(intermediate_reg_0[432]),.o(intermediate_reg_1[216]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_218(.clk(clk),.reset(reset),.i1(intermediate_reg_0[431]),.i2(intermediate_reg_0[430]),.o(intermediate_reg_1[215])); 
mux_module mux_module_inst_1_219(.clk(clk),.reset(reset),.i1(intermediate_reg_0[429]),.i2(intermediate_reg_0[428]),.o(intermediate_reg_1[214]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_220(.clk(clk),.reset(reset),.i1(intermediate_reg_0[427]),.i2(intermediate_reg_0[426]),.o(intermediate_reg_1[213])); 
xor_module xor_module_inst_1_221(.clk(clk),.reset(reset),.i1(intermediate_reg_0[425]),.i2(intermediate_reg_0[424]),.o(intermediate_reg_1[212])); 
mux_module mux_module_inst_1_222(.clk(clk),.reset(reset),.i1(intermediate_reg_0[423]),.i2(intermediate_reg_0[422]),.o(intermediate_reg_1[211]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_223(.clk(clk),.reset(reset),.i1(intermediate_reg_0[421]),.i2(intermediate_reg_0[420]),.o(intermediate_reg_1[210])); 
xor_module xor_module_inst_1_224(.clk(clk),.reset(reset),.i1(intermediate_reg_0[419]),.i2(intermediate_reg_0[418]),.o(intermediate_reg_1[209])); 
xor_module xor_module_inst_1_225(.clk(clk),.reset(reset),.i1(intermediate_reg_0[417]),.i2(intermediate_reg_0[416]),.o(intermediate_reg_1[208])); 
mux_module mux_module_inst_1_226(.clk(clk),.reset(reset),.i1(intermediate_reg_0[415]),.i2(intermediate_reg_0[414]),.o(intermediate_reg_1[207]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_227(.clk(clk),.reset(reset),.i1(intermediate_reg_0[413]),.i2(intermediate_reg_0[412]),.o(intermediate_reg_1[206])); 
mux_module mux_module_inst_1_228(.clk(clk),.reset(reset),.i1(intermediate_reg_0[411]),.i2(intermediate_reg_0[410]),.o(intermediate_reg_1[205]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_229(.clk(clk),.reset(reset),.i1(intermediate_reg_0[409]),.i2(intermediate_reg_0[408]),.o(intermediate_reg_1[204])); 
mux_module mux_module_inst_1_230(.clk(clk),.reset(reset),.i1(intermediate_reg_0[407]),.i2(intermediate_reg_0[406]),.o(intermediate_reg_1[203]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_231(.clk(clk),.reset(reset),.i1(intermediate_reg_0[405]),.i2(intermediate_reg_0[404]),.o(intermediate_reg_1[202]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_232(.clk(clk),.reset(reset),.i1(intermediate_reg_0[403]),.i2(intermediate_reg_0[402]),.o(intermediate_reg_1[201]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_233(.clk(clk),.reset(reset),.i1(intermediate_reg_0[401]),.i2(intermediate_reg_0[400]),.o(intermediate_reg_1[200]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_234(.clk(clk),.reset(reset),.i1(intermediate_reg_0[399]),.i2(intermediate_reg_0[398]),.o(intermediate_reg_1[199])); 
xor_module xor_module_inst_1_235(.clk(clk),.reset(reset),.i1(intermediate_reg_0[397]),.i2(intermediate_reg_0[396]),.o(intermediate_reg_1[198])); 
mux_module mux_module_inst_1_236(.clk(clk),.reset(reset),.i1(intermediate_reg_0[395]),.i2(intermediate_reg_0[394]),.o(intermediate_reg_1[197]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_237(.clk(clk),.reset(reset),.i1(intermediate_reg_0[393]),.i2(intermediate_reg_0[392]),.o(intermediate_reg_1[196])); 
mux_module mux_module_inst_1_238(.clk(clk),.reset(reset),.i1(intermediate_reg_0[391]),.i2(intermediate_reg_0[390]),.o(intermediate_reg_1[195]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_239(.clk(clk),.reset(reset),.i1(intermediate_reg_0[389]),.i2(intermediate_reg_0[388]),.o(intermediate_reg_1[194])); 
mux_module mux_module_inst_1_240(.clk(clk),.reset(reset),.i1(intermediate_reg_0[387]),.i2(intermediate_reg_0[386]),.o(intermediate_reg_1[193]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_241(.clk(clk),.reset(reset),.i1(intermediate_reg_0[385]),.i2(intermediate_reg_0[384]),.o(intermediate_reg_1[192]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_242(.clk(clk),.reset(reset),.i1(intermediate_reg_0[383]),.i2(intermediate_reg_0[382]),.o(intermediate_reg_1[191]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_243(.clk(clk),.reset(reset),.i1(intermediate_reg_0[381]),.i2(intermediate_reg_0[380]),.o(intermediate_reg_1[190])); 
mux_module mux_module_inst_1_244(.clk(clk),.reset(reset),.i1(intermediate_reg_0[379]),.i2(intermediate_reg_0[378]),.o(intermediate_reg_1[189]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_245(.clk(clk),.reset(reset),.i1(intermediate_reg_0[377]),.i2(intermediate_reg_0[376]),.o(intermediate_reg_1[188]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_246(.clk(clk),.reset(reset),.i1(intermediate_reg_0[375]),.i2(intermediate_reg_0[374]),.o(intermediate_reg_1[187])); 
xor_module xor_module_inst_1_247(.clk(clk),.reset(reset),.i1(intermediate_reg_0[373]),.i2(intermediate_reg_0[372]),.o(intermediate_reg_1[186])); 
mux_module mux_module_inst_1_248(.clk(clk),.reset(reset),.i1(intermediate_reg_0[371]),.i2(intermediate_reg_0[370]),.o(intermediate_reg_1[185]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_249(.clk(clk),.reset(reset),.i1(intermediate_reg_0[369]),.i2(intermediate_reg_0[368]),.o(intermediate_reg_1[184])); 
mux_module mux_module_inst_1_250(.clk(clk),.reset(reset),.i1(intermediate_reg_0[367]),.i2(intermediate_reg_0[366]),.o(intermediate_reg_1[183]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_251(.clk(clk),.reset(reset),.i1(intermediate_reg_0[365]),.i2(intermediate_reg_0[364]),.o(intermediate_reg_1[182])); 
xor_module xor_module_inst_1_252(.clk(clk),.reset(reset),.i1(intermediate_reg_0[363]),.i2(intermediate_reg_0[362]),.o(intermediate_reg_1[181])); 
mux_module mux_module_inst_1_253(.clk(clk),.reset(reset),.i1(intermediate_reg_0[361]),.i2(intermediate_reg_0[360]),.o(intermediate_reg_1[180]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_254(.clk(clk),.reset(reset),.i1(intermediate_reg_0[359]),.i2(intermediate_reg_0[358]),.o(intermediate_reg_1[179]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_255(.clk(clk),.reset(reset),.i1(intermediate_reg_0[357]),.i2(intermediate_reg_0[356]),.o(intermediate_reg_1[178]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_256(.clk(clk),.reset(reset),.i1(intermediate_reg_0[355]),.i2(intermediate_reg_0[354]),.o(intermediate_reg_1[177]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_257(.clk(clk),.reset(reset),.i1(intermediate_reg_0[353]),.i2(intermediate_reg_0[352]),.o(intermediate_reg_1[176])); 
xor_module xor_module_inst_1_258(.clk(clk),.reset(reset),.i1(intermediate_reg_0[351]),.i2(intermediate_reg_0[350]),.o(intermediate_reg_1[175])); 
mux_module mux_module_inst_1_259(.clk(clk),.reset(reset),.i1(intermediate_reg_0[349]),.i2(intermediate_reg_0[348]),.o(intermediate_reg_1[174]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_260(.clk(clk),.reset(reset),.i1(intermediate_reg_0[347]),.i2(intermediate_reg_0[346]),.o(intermediate_reg_1[173])); 
mux_module mux_module_inst_1_261(.clk(clk),.reset(reset),.i1(intermediate_reg_0[345]),.i2(intermediate_reg_0[344]),.o(intermediate_reg_1[172]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_262(.clk(clk),.reset(reset),.i1(intermediate_reg_0[343]),.i2(intermediate_reg_0[342]),.o(intermediate_reg_1[171]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_263(.clk(clk),.reset(reset),.i1(intermediate_reg_0[341]),.i2(intermediate_reg_0[340]),.o(intermediate_reg_1[170]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_264(.clk(clk),.reset(reset),.i1(intermediate_reg_0[339]),.i2(intermediate_reg_0[338]),.o(intermediate_reg_1[169]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_265(.clk(clk),.reset(reset),.i1(intermediate_reg_0[337]),.i2(intermediate_reg_0[336]),.o(intermediate_reg_1[168]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_266(.clk(clk),.reset(reset),.i1(intermediate_reg_0[335]),.i2(intermediate_reg_0[334]),.o(intermediate_reg_1[167]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_267(.clk(clk),.reset(reset),.i1(intermediate_reg_0[333]),.i2(intermediate_reg_0[332]),.o(intermediate_reg_1[166])); 
xor_module xor_module_inst_1_268(.clk(clk),.reset(reset),.i1(intermediate_reg_0[331]),.i2(intermediate_reg_0[330]),.o(intermediate_reg_1[165])); 
mux_module mux_module_inst_1_269(.clk(clk),.reset(reset),.i1(intermediate_reg_0[329]),.i2(intermediate_reg_0[328]),.o(intermediate_reg_1[164]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_270(.clk(clk),.reset(reset),.i1(intermediate_reg_0[327]),.i2(intermediate_reg_0[326]),.o(intermediate_reg_1[163]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_271(.clk(clk),.reset(reset),.i1(intermediate_reg_0[325]),.i2(intermediate_reg_0[324]),.o(intermediate_reg_1[162])); 
xor_module xor_module_inst_1_272(.clk(clk),.reset(reset),.i1(intermediate_reg_0[323]),.i2(intermediate_reg_0[322]),.o(intermediate_reg_1[161])); 
mux_module mux_module_inst_1_273(.clk(clk),.reset(reset),.i1(intermediate_reg_0[321]),.i2(intermediate_reg_0[320]),.o(intermediate_reg_1[160]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_274(.clk(clk),.reset(reset),.i1(intermediate_reg_0[319]),.i2(intermediate_reg_0[318]),.o(intermediate_reg_1[159]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_275(.clk(clk),.reset(reset),.i1(intermediate_reg_0[317]),.i2(intermediate_reg_0[316]),.o(intermediate_reg_1[158]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_276(.clk(clk),.reset(reset),.i1(intermediate_reg_0[315]),.i2(intermediate_reg_0[314]),.o(intermediate_reg_1[157]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_277(.clk(clk),.reset(reset),.i1(intermediate_reg_0[313]),.i2(intermediate_reg_0[312]),.o(intermediate_reg_1[156])); 
xor_module xor_module_inst_1_278(.clk(clk),.reset(reset),.i1(intermediate_reg_0[311]),.i2(intermediate_reg_0[310]),.o(intermediate_reg_1[155])); 
mux_module mux_module_inst_1_279(.clk(clk),.reset(reset),.i1(intermediate_reg_0[309]),.i2(intermediate_reg_0[308]),.o(intermediate_reg_1[154]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_280(.clk(clk),.reset(reset),.i1(intermediate_reg_0[307]),.i2(intermediate_reg_0[306]),.o(intermediate_reg_1[153]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_281(.clk(clk),.reset(reset),.i1(intermediate_reg_0[305]),.i2(intermediate_reg_0[304]),.o(intermediate_reg_1[152]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_282(.clk(clk),.reset(reset),.i1(intermediate_reg_0[303]),.i2(intermediate_reg_0[302]),.o(intermediate_reg_1[151]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_283(.clk(clk),.reset(reset),.i1(intermediate_reg_0[301]),.i2(intermediate_reg_0[300]),.o(intermediate_reg_1[150]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_284(.clk(clk),.reset(reset),.i1(intermediate_reg_0[299]),.i2(intermediate_reg_0[298]),.o(intermediate_reg_1[149])); 
mux_module mux_module_inst_1_285(.clk(clk),.reset(reset),.i1(intermediate_reg_0[297]),.i2(intermediate_reg_0[296]),.o(intermediate_reg_1[148]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_286(.clk(clk),.reset(reset),.i1(intermediate_reg_0[295]),.i2(intermediate_reg_0[294]),.o(intermediate_reg_1[147])); 
xor_module xor_module_inst_1_287(.clk(clk),.reset(reset),.i1(intermediate_reg_0[293]),.i2(intermediate_reg_0[292]),.o(intermediate_reg_1[146])); 
mux_module mux_module_inst_1_288(.clk(clk),.reset(reset),.i1(intermediate_reg_0[291]),.i2(intermediate_reg_0[290]),.o(intermediate_reg_1[145]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_289(.clk(clk),.reset(reset),.i1(intermediate_reg_0[289]),.i2(intermediate_reg_0[288]),.o(intermediate_reg_1[144]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_290(.clk(clk),.reset(reset),.i1(intermediate_reg_0[287]),.i2(intermediate_reg_0[286]),.o(intermediate_reg_1[143])); 
mux_module mux_module_inst_1_291(.clk(clk),.reset(reset),.i1(intermediate_reg_0[285]),.i2(intermediate_reg_0[284]),.o(intermediate_reg_1[142]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_292(.clk(clk),.reset(reset),.i1(intermediate_reg_0[283]),.i2(intermediate_reg_0[282]),.o(intermediate_reg_1[141])); 
xor_module xor_module_inst_1_293(.clk(clk),.reset(reset),.i1(intermediate_reg_0[281]),.i2(intermediate_reg_0[280]),.o(intermediate_reg_1[140])); 
mux_module mux_module_inst_1_294(.clk(clk),.reset(reset),.i1(intermediate_reg_0[279]),.i2(intermediate_reg_0[278]),.o(intermediate_reg_1[139]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_295(.clk(clk),.reset(reset),.i1(intermediate_reg_0[277]),.i2(intermediate_reg_0[276]),.o(intermediate_reg_1[138])); 
xor_module xor_module_inst_1_296(.clk(clk),.reset(reset),.i1(intermediate_reg_0[275]),.i2(intermediate_reg_0[274]),.o(intermediate_reg_1[137])); 
xor_module xor_module_inst_1_297(.clk(clk),.reset(reset),.i1(intermediate_reg_0[273]),.i2(intermediate_reg_0[272]),.o(intermediate_reg_1[136])); 
xor_module xor_module_inst_1_298(.clk(clk),.reset(reset),.i1(intermediate_reg_0[271]),.i2(intermediate_reg_0[270]),.o(intermediate_reg_1[135])); 
xor_module xor_module_inst_1_299(.clk(clk),.reset(reset),.i1(intermediate_reg_0[269]),.i2(intermediate_reg_0[268]),.o(intermediate_reg_1[134])); 
xor_module xor_module_inst_1_300(.clk(clk),.reset(reset),.i1(intermediate_reg_0[267]),.i2(intermediate_reg_0[266]),.o(intermediate_reg_1[133])); 
xor_module xor_module_inst_1_301(.clk(clk),.reset(reset),.i1(intermediate_reg_0[265]),.i2(intermediate_reg_0[264]),.o(intermediate_reg_1[132])); 
mux_module mux_module_inst_1_302(.clk(clk),.reset(reset),.i1(intermediate_reg_0[263]),.i2(intermediate_reg_0[262]),.o(intermediate_reg_1[131]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_303(.clk(clk),.reset(reset),.i1(intermediate_reg_0[261]),.i2(intermediate_reg_0[260]),.o(intermediate_reg_1[130])); 
xor_module xor_module_inst_1_304(.clk(clk),.reset(reset),.i1(intermediate_reg_0[259]),.i2(intermediate_reg_0[258]),.o(intermediate_reg_1[129])); 
mux_module mux_module_inst_1_305(.clk(clk),.reset(reset),.i1(intermediate_reg_0[257]),.i2(intermediate_reg_0[256]),.o(intermediate_reg_1[128]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_306(.clk(clk),.reset(reset),.i1(intermediate_reg_0[255]),.i2(intermediate_reg_0[254]),.o(intermediate_reg_1[127])); 
mux_module mux_module_inst_1_307(.clk(clk),.reset(reset),.i1(intermediate_reg_0[253]),.i2(intermediate_reg_0[252]),.o(intermediate_reg_1[126]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_308(.clk(clk),.reset(reset),.i1(intermediate_reg_0[251]),.i2(intermediate_reg_0[250]),.o(intermediate_reg_1[125])); 
mux_module mux_module_inst_1_309(.clk(clk),.reset(reset),.i1(intermediate_reg_0[249]),.i2(intermediate_reg_0[248]),.o(intermediate_reg_1[124]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_310(.clk(clk),.reset(reset),.i1(intermediate_reg_0[247]),.i2(intermediate_reg_0[246]),.o(intermediate_reg_1[123])); 
mux_module mux_module_inst_1_311(.clk(clk),.reset(reset),.i1(intermediate_reg_0[245]),.i2(intermediate_reg_0[244]),.o(intermediate_reg_1[122]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_312(.clk(clk),.reset(reset),.i1(intermediate_reg_0[243]),.i2(intermediate_reg_0[242]),.o(intermediate_reg_1[121])); 
mux_module mux_module_inst_1_313(.clk(clk),.reset(reset),.i1(intermediate_reg_0[241]),.i2(intermediate_reg_0[240]),.o(intermediate_reg_1[120]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_314(.clk(clk),.reset(reset),.i1(intermediate_reg_0[239]),.i2(intermediate_reg_0[238]),.o(intermediate_reg_1[119])); 
mux_module mux_module_inst_1_315(.clk(clk),.reset(reset),.i1(intermediate_reg_0[237]),.i2(intermediate_reg_0[236]),.o(intermediate_reg_1[118]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_316(.clk(clk),.reset(reset),.i1(intermediate_reg_0[235]),.i2(intermediate_reg_0[234]),.o(intermediate_reg_1[117])); 
mux_module mux_module_inst_1_317(.clk(clk),.reset(reset),.i1(intermediate_reg_0[233]),.i2(intermediate_reg_0[232]),.o(intermediate_reg_1[116]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_318(.clk(clk),.reset(reset),.i1(intermediate_reg_0[231]),.i2(intermediate_reg_0[230]),.o(intermediate_reg_1[115])); 
xor_module xor_module_inst_1_319(.clk(clk),.reset(reset),.i1(intermediate_reg_0[229]),.i2(intermediate_reg_0[228]),.o(intermediate_reg_1[114])); 
mux_module mux_module_inst_1_320(.clk(clk),.reset(reset),.i1(intermediate_reg_0[227]),.i2(intermediate_reg_0[226]),.o(intermediate_reg_1[113]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_321(.clk(clk),.reset(reset),.i1(intermediate_reg_0[225]),.i2(intermediate_reg_0[224]),.o(intermediate_reg_1[112]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_322(.clk(clk),.reset(reset),.i1(intermediate_reg_0[223]),.i2(intermediate_reg_0[222]),.o(intermediate_reg_1[111])); 
mux_module mux_module_inst_1_323(.clk(clk),.reset(reset),.i1(intermediate_reg_0[221]),.i2(intermediate_reg_0[220]),.o(intermediate_reg_1[110]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_324(.clk(clk),.reset(reset),.i1(intermediate_reg_0[219]),.i2(intermediate_reg_0[218]),.o(intermediate_reg_1[109])); 
xor_module xor_module_inst_1_325(.clk(clk),.reset(reset),.i1(intermediate_reg_0[217]),.i2(intermediate_reg_0[216]),.o(intermediate_reg_1[108])); 
xor_module xor_module_inst_1_326(.clk(clk),.reset(reset),.i1(intermediate_reg_0[215]),.i2(intermediate_reg_0[214]),.o(intermediate_reg_1[107])); 
mux_module mux_module_inst_1_327(.clk(clk),.reset(reset),.i1(intermediate_reg_0[213]),.i2(intermediate_reg_0[212]),.o(intermediate_reg_1[106]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_328(.clk(clk),.reset(reset),.i1(intermediate_reg_0[211]),.i2(intermediate_reg_0[210]),.o(intermediate_reg_1[105])); 
xor_module xor_module_inst_1_329(.clk(clk),.reset(reset),.i1(intermediate_reg_0[209]),.i2(intermediate_reg_0[208]),.o(intermediate_reg_1[104])); 
xor_module xor_module_inst_1_330(.clk(clk),.reset(reset),.i1(intermediate_reg_0[207]),.i2(intermediate_reg_0[206]),.o(intermediate_reg_1[103])); 
mux_module mux_module_inst_1_331(.clk(clk),.reset(reset),.i1(intermediate_reg_0[205]),.i2(intermediate_reg_0[204]),.o(intermediate_reg_1[102]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_332(.clk(clk),.reset(reset),.i1(intermediate_reg_0[203]),.i2(intermediate_reg_0[202]),.o(intermediate_reg_1[101])); 
xor_module xor_module_inst_1_333(.clk(clk),.reset(reset),.i1(intermediate_reg_0[201]),.i2(intermediate_reg_0[200]),.o(intermediate_reg_1[100])); 
xor_module xor_module_inst_1_334(.clk(clk),.reset(reset),.i1(intermediate_reg_0[199]),.i2(intermediate_reg_0[198]),.o(intermediate_reg_1[99])); 
xor_module xor_module_inst_1_335(.clk(clk),.reset(reset),.i1(intermediate_reg_0[197]),.i2(intermediate_reg_0[196]),.o(intermediate_reg_1[98])); 
mux_module mux_module_inst_1_336(.clk(clk),.reset(reset),.i1(intermediate_reg_0[195]),.i2(intermediate_reg_0[194]),.o(intermediate_reg_1[97]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_337(.clk(clk),.reset(reset),.i1(intermediate_reg_0[193]),.i2(intermediate_reg_0[192]),.o(intermediate_reg_1[96])); 
xor_module xor_module_inst_1_338(.clk(clk),.reset(reset),.i1(intermediate_reg_0[191]),.i2(intermediate_reg_0[190]),.o(intermediate_reg_1[95])); 
xor_module xor_module_inst_1_339(.clk(clk),.reset(reset),.i1(intermediate_reg_0[189]),.i2(intermediate_reg_0[188]),.o(intermediate_reg_1[94])); 
xor_module xor_module_inst_1_340(.clk(clk),.reset(reset),.i1(intermediate_reg_0[187]),.i2(intermediate_reg_0[186]),.o(intermediate_reg_1[93])); 
mux_module mux_module_inst_1_341(.clk(clk),.reset(reset),.i1(intermediate_reg_0[185]),.i2(intermediate_reg_0[184]),.o(intermediate_reg_1[92]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_342(.clk(clk),.reset(reset),.i1(intermediate_reg_0[183]),.i2(intermediate_reg_0[182]),.o(intermediate_reg_1[91])); 
mux_module mux_module_inst_1_343(.clk(clk),.reset(reset),.i1(intermediate_reg_0[181]),.i2(intermediate_reg_0[180]),.o(intermediate_reg_1[90]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_344(.clk(clk),.reset(reset),.i1(intermediate_reg_0[179]),.i2(intermediate_reg_0[178]),.o(intermediate_reg_1[89]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_345(.clk(clk),.reset(reset),.i1(intermediate_reg_0[177]),.i2(intermediate_reg_0[176]),.o(intermediate_reg_1[88])); 
xor_module xor_module_inst_1_346(.clk(clk),.reset(reset),.i1(intermediate_reg_0[175]),.i2(intermediate_reg_0[174]),.o(intermediate_reg_1[87])); 
mux_module mux_module_inst_1_347(.clk(clk),.reset(reset),.i1(intermediate_reg_0[173]),.i2(intermediate_reg_0[172]),.o(intermediate_reg_1[86]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_348(.clk(clk),.reset(reset),.i1(intermediate_reg_0[171]),.i2(intermediate_reg_0[170]),.o(intermediate_reg_1[85]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_349(.clk(clk),.reset(reset),.i1(intermediate_reg_0[169]),.i2(intermediate_reg_0[168]),.o(intermediate_reg_1[84]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_350(.clk(clk),.reset(reset),.i1(intermediate_reg_0[167]),.i2(intermediate_reg_0[166]),.o(intermediate_reg_1[83])); 
mux_module mux_module_inst_1_351(.clk(clk),.reset(reset),.i1(intermediate_reg_0[165]),.i2(intermediate_reg_0[164]),.o(intermediate_reg_1[82]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_352(.clk(clk),.reset(reset),.i1(intermediate_reg_0[163]),.i2(intermediate_reg_0[162]),.o(intermediate_reg_1[81])); 
mux_module mux_module_inst_1_353(.clk(clk),.reset(reset),.i1(intermediate_reg_0[161]),.i2(intermediate_reg_0[160]),.o(intermediate_reg_1[80]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_354(.clk(clk),.reset(reset),.i1(intermediate_reg_0[159]),.i2(intermediate_reg_0[158]),.o(intermediate_reg_1[79]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_355(.clk(clk),.reset(reset),.i1(intermediate_reg_0[157]),.i2(intermediate_reg_0[156]),.o(intermediate_reg_1[78])); 
xor_module xor_module_inst_1_356(.clk(clk),.reset(reset),.i1(intermediate_reg_0[155]),.i2(intermediate_reg_0[154]),.o(intermediate_reg_1[77])); 
xor_module xor_module_inst_1_357(.clk(clk),.reset(reset),.i1(intermediate_reg_0[153]),.i2(intermediate_reg_0[152]),.o(intermediate_reg_1[76])); 
xor_module xor_module_inst_1_358(.clk(clk),.reset(reset),.i1(intermediate_reg_0[151]),.i2(intermediate_reg_0[150]),.o(intermediate_reg_1[75])); 
mux_module mux_module_inst_1_359(.clk(clk),.reset(reset),.i1(intermediate_reg_0[149]),.i2(intermediate_reg_0[148]),.o(intermediate_reg_1[74]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_360(.clk(clk),.reset(reset),.i1(intermediate_reg_0[147]),.i2(intermediate_reg_0[146]),.o(intermediate_reg_1[73]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_361(.clk(clk),.reset(reset),.i1(intermediate_reg_0[145]),.i2(intermediate_reg_0[144]),.o(intermediate_reg_1[72])); 
xor_module xor_module_inst_1_362(.clk(clk),.reset(reset),.i1(intermediate_reg_0[143]),.i2(intermediate_reg_0[142]),.o(intermediate_reg_1[71])); 
mux_module mux_module_inst_1_363(.clk(clk),.reset(reset),.i1(intermediate_reg_0[141]),.i2(intermediate_reg_0[140]),.o(intermediate_reg_1[70]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_364(.clk(clk),.reset(reset),.i1(intermediate_reg_0[139]),.i2(intermediate_reg_0[138]),.o(intermediate_reg_1[69])); 
mux_module mux_module_inst_1_365(.clk(clk),.reset(reset),.i1(intermediate_reg_0[137]),.i2(intermediate_reg_0[136]),.o(intermediate_reg_1[68]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_366(.clk(clk),.reset(reset),.i1(intermediate_reg_0[135]),.i2(intermediate_reg_0[134]),.o(intermediate_reg_1[67])); 
xor_module xor_module_inst_1_367(.clk(clk),.reset(reset),.i1(intermediate_reg_0[133]),.i2(intermediate_reg_0[132]),.o(intermediate_reg_1[66])); 
xor_module xor_module_inst_1_368(.clk(clk),.reset(reset),.i1(intermediate_reg_0[131]),.i2(intermediate_reg_0[130]),.o(intermediate_reg_1[65])); 
mux_module mux_module_inst_1_369(.clk(clk),.reset(reset),.i1(intermediate_reg_0[129]),.i2(intermediate_reg_0[128]),.o(intermediate_reg_1[64]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_370(.clk(clk),.reset(reset),.i1(intermediate_reg_0[127]),.i2(intermediate_reg_0[126]),.o(intermediate_reg_1[63])); 
mux_module mux_module_inst_1_371(.clk(clk),.reset(reset),.i1(intermediate_reg_0[125]),.i2(intermediate_reg_0[124]),.o(intermediate_reg_1[62]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_372(.clk(clk),.reset(reset),.i1(intermediate_reg_0[123]),.i2(intermediate_reg_0[122]),.o(intermediate_reg_1[61]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_373(.clk(clk),.reset(reset),.i1(intermediate_reg_0[121]),.i2(intermediate_reg_0[120]),.o(intermediate_reg_1[60])); 
mux_module mux_module_inst_1_374(.clk(clk),.reset(reset),.i1(intermediate_reg_0[119]),.i2(intermediate_reg_0[118]),.o(intermediate_reg_1[59]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_375(.clk(clk),.reset(reset),.i1(intermediate_reg_0[117]),.i2(intermediate_reg_0[116]),.o(intermediate_reg_1[58])); 
xor_module xor_module_inst_1_376(.clk(clk),.reset(reset),.i1(intermediate_reg_0[115]),.i2(intermediate_reg_0[114]),.o(intermediate_reg_1[57])); 
mux_module mux_module_inst_1_377(.clk(clk),.reset(reset),.i1(intermediate_reg_0[113]),.i2(intermediate_reg_0[112]),.o(intermediate_reg_1[56]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_378(.clk(clk),.reset(reset),.i1(intermediate_reg_0[111]),.i2(intermediate_reg_0[110]),.o(intermediate_reg_1[55]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_379(.clk(clk),.reset(reset),.i1(intermediate_reg_0[109]),.i2(intermediate_reg_0[108]),.o(intermediate_reg_1[54])); 
mux_module mux_module_inst_1_380(.clk(clk),.reset(reset),.i1(intermediate_reg_0[107]),.i2(intermediate_reg_0[106]),.o(intermediate_reg_1[53]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_381(.clk(clk),.reset(reset),.i1(intermediate_reg_0[105]),.i2(intermediate_reg_0[104]),.o(intermediate_reg_1[52])); 
xor_module xor_module_inst_1_382(.clk(clk),.reset(reset),.i1(intermediate_reg_0[103]),.i2(intermediate_reg_0[102]),.o(intermediate_reg_1[51])); 
xor_module xor_module_inst_1_383(.clk(clk),.reset(reset),.i1(intermediate_reg_0[101]),.i2(intermediate_reg_0[100]),.o(intermediate_reg_1[50])); 
xor_module xor_module_inst_1_384(.clk(clk),.reset(reset),.i1(intermediate_reg_0[99]),.i2(intermediate_reg_0[98]),.o(intermediate_reg_1[49])); 
xor_module xor_module_inst_1_385(.clk(clk),.reset(reset),.i1(intermediate_reg_0[97]),.i2(intermediate_reg_0[96]),.o(intermediate_reg_1[48])); 
mux_module mux_module_inst_1_386(.clk(clk),.reset(reset),.i1(intermediate_reg_0[95]),.i2(intermediate_reg_0[94]),.o(intermediate_reg_1[47]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_387(.clk(clk),.reset(reset),.i1(intermediate_reg_0[93]),.i2(intermediate_reg_0[92]),.o(intermediate_reg_1[46])); 
xor_module xor_module_inst_1_388(.clk(clk),.reset(reset),.i1(intermediate_reg_0[91]),.i2(intermediate_reg_0[90]),.o(intermediate_reg_1[45])); 
mux_module mux_module_inst_1_389(.clk(clk),.reset(reset),.i1(intermediate_reg_0[89]),.i2(intermediate_reg_0[88]),.o(intermediate_reg_1[44]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_390(.clk(clk),.reset(reset),.i1(intermediate_reg_0[87]),.i2(intermediate_reg_0[86]),.o(intermediate_reg_1[43])); 
xor_module xor_module_inst_1_391(.clk(clk),.reset(reset),.i1(intermediate_reg_0[85]),.i2(intermediate_reg_0[84]),.o(intermediate_reg_1[42])); 
mux_module mux_module_inst_1_392(.clk(clk),.reset(reset),.i1(intermediate_reg_0[83]),.i2(intermediate_reg_0[82]),.o(intermediate_reg_1[41]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_393(.clk(clk),.reset(reset),.i1(intermediate_reg_0[81]),.i2(intermediate_reg_0[80]),.o(intermediate_reg_1[40]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_394(.clk(clk),.reset(reset),.i1(intermediate_reg_0[79]),.i2(intermediate_reg_0[78]),.o(intermediate_reg_1[39]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_395(.clk(clk),.reset(reset),.i1(intermediate_reg_0[77]),.i2(intermediate_reg_0[76]),.o(intermediate_reg_1[38])); 
mux_module mux_module_inst_1_396(.clk(clk),.reset(reset),.i1(intermediate_reg_0[75]),.i2(intermediate_reg_0[74]),.o(intermediate_reg_1[37]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_397(.clk(clk),.reset(reset),.i1(intermediate_reg_0[73]),.i2(intermediate_reg_0[72]),.o(intermediate_reg_1[36]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_398(.clk(clk),.reset(reset),.i1(intermediate_reg_0[71]),.i2(intermediate_reg_0[70]),.o(intermediate_reg_1[35])); 
xor_module xor_module_inst_1_399(.clk(clk),.reset(reset),.i1(intermediate_reg_0[69]),.i2(intermediate_reg_0[68]),.o(intermediate_reg_1[34])); 
xor_module xor_module_inst_1_400(.clk(clk),.reset(reset),.i1(intermediate_reg_0[67]),.i2(intermediate_reg_0[66]),.o(intermediate_reg_1[33])); 
mux_module mux_module_inst_1_401(.clk(clk),.reset(reset),.i1(intermediate_reg_0[65]),.i2(intermediate_reg_0[64]),.o(intermediate_reg_1[32]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_402(.clk(clk),.reset(reset),.i1(intermediate_reg_0[63]),.i2(intermediate_reg_0[62]),.o(intermediate_reg_1[31]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_403(.clk(clk),.reset(reset),.i1(intermediate_reg_0[61]),.i2(intermediate_reg_0[60]),.o(intermediate_reg_1[30]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_404(.clk(clk),.reset(reset),.i1(intermediate_reg_0[59]),.i2(intermediate_reg_0[58]),.o(intermediate_reg_1[29])); 
xor_module xor_module_inst_1_405(.clk(clk),.reset(reset),.i1(intermediate_reg_0[57]),.i2(intermediate_reg_0[56]),.o(intermediate_reg_1[28])); 
xor_module xor_module_inst_1_406(.clk(clk),.reset(reset),.i1(intermediate_reg_0[55]),.i2(intermediate_reg_0[54]),.o(intermediate_reg_1[27])); 
xor_module xor_module_inst_1_407(.clk(clk),.reset(reset),.i1(intermediate_reg_0[53]),.i2(intermediate_reg_0[52]),.o(intermediate_reg_1[26])); 
xor_module xor_module_inst_1_408(.clk(clk),.reset(reset),.i1(intermediate_reg_0[51]),.i2(intermediate_reg_0[50]),.o(intermediate_reg_1[25])); 
mux_module mux_module_inst_1_409(.clk(clk),.reset(reset),.i1(intermediate_reg_0[49]),.i2(intermediate_reg_0[48]),.o(intermediate_reg_1[24]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_410(.clk(clk),.reset(reset),.i1(intermediate_reg_0[47]),.i2(intermediate_reg_0[46]),.o(intermediate_reg_1[23]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_411(.clk(clk),.reset(reset),.i1(intermediate_reg_0[45]),.i2(intermediate_reg_0[44]),.o(intermediate_reg_1[22])); 
xor_module xor_module_inst_1_412(.clk(clk),.reset(reset),.i1(intermediate_reg_0[43]),.i2(intermediate_reg_0[42]),.o(intermediate_reg_1[21])); 
xor_module xor_module_inst_1_413(.clk(clk),.reset(reset),.i1(intermediate_reg_0[41]),.i2(intermediate_reg_0[40]),.o(intermediate_reg_1[20])); 
mux_module mux_module_inst_1_414(.clk(clk),.reset(reset),.i1(intermediate_reg_0[39]),.i2(intermediate_reg_0[38]),.o(intermediate_reg_1[19]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_415(.clk(clk),.reset(reset),.i1(intermediate_reg_0[37]),.i2(intermediate_reg_0[36]),.o(intermediate_reg_1[18])); 
xor_module xor_module_inst_1_416(.clk(clk),.reset(reset),.i1(intermediate_reg_0[35]),.i2(intermediate_reg_0[34]),.o(intermediate_reg_1[17])); 
xor_module xor_module_inst_1_417(.clk(clk),.reset(reset),.i1(intermediate_reg_0[33]),.i2(intermediate_reg_0[32]),.o(intermediate_reg_1[16])); 
mux_module mux_module_inst_1_418(.clk(clk),.reset(reset),.i1(intermediate_reg_0[31]),.i2(intermediate_reg_0[30]),.o(intermediate_reg_1[15]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_419(.clk(clk),.reset(reset),.i1(intermediate_reg_0[29]),.i2(intermediate_reg_0[28]),.o(intermediate_reg_1[14])); 
xor_module xor_module_inst_1_420(.clk(clk),.reset(reset),.i1(intermediate_reg_0[27]),.i2(intermediate_reg_0[26]),.o(intermediate_reg_1[13])); 
mux_module mux_module_inst_1_421(.clk(clk),.reset(reset),.i1(intermediate_reg_0[25]),.i2(intermediate_reg_0[24]),.o(intermediate_reg_1[12]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_422(.clk(clk),.reset(reset),.i1(intermediate_reg_0[23]),.i2(intermediate_reg_0[22]),.o(intermediate_reg_1[11])); 
xor_module xor_module_inst_1_423(.clk(clk),.reset(reset),.i1(intermediate_reg_0[21]),.i2(intermediate_reg_0[20]),.o(intermediate_reg_1[10])); 
xor_module xor_module_inst_1_424(.clk(clk),.reset(reset),.i1(intermediate_reg_0[19]),.i2(intermediate_reg_0[18]),.o(intermediate_reg_1[9])); 
xor_module xor_module_inst_1_425(.clk(clk),.reset(reset),.i1(intermediate_reg_0[17]),.i2(intermediate_reg_0[16]),.o(intermediate_reg_1[8])); 
xor_module xor_module_inst_1_426(.clk(clk),.reset(reset),.i1(intermediate_reg_0[15]),.i2(intermediate_reg_0[14]),.o(intermediate_reg_1[7])); 
mux_module mux_module_inst_1_427(.clk(clk),.reset(reset),.i1(intermediate_reg_0[13]),.i2(intermediate_reg_0[12]),.o(intermediate_reg_1[6]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_428(.clk(clk),.reset(reset),.i1(intermediate_reg_0[11]),.i2(intermediate_reg_0[10]),.o(intermediate_reg_1[5])); 
xor_module xor_module_inst_1_429(.clk(clk),.reset(reset),.i1(intermediate_reg_0[9]),.i2(intermediate_reg_0[8]),.o(intermediate_reg_1[4])); 
mux_module mux_module_inst_1_430(.clk(clk),.reset(reset),.i1(intermediate_reg_0[7]),.i2(intermediate_reg_0[6]),.o(intermediate_reg_1[3]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_431(.clk(clk),.reset(reset),.i1(intermediate_reg_0[5]),.i2(intermediate_reg_0[4]),.o(intermediate_reg_1[2])); 
xor_module xor_module_inst_1_432(.clk(clk),.reset(reset),.i1(intermediate_reg_0[3]),.i2(intermediate_reg_0[2]),.o(intermediate_reg_1[1])); 
xor_module xor_module_inst_1_433(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1]),.i2(intermediate_reg_0[0]),.o(intermediate_reg_1[0])); 
always@(posedge clk) begin 
outp [433:0] <= intermediate_reg_1; 
outp[847:434] <= intermediate_reg_1[413:0] ; 
end 
endmodule 
 

module interface_6(input [2441:0] inp, output reg [1535:0] outp, input clk, input reset);
reg [2441:0]intermediate_reg_0; 
always@(posedge clk) begin 
intermediate_reg_0 <= inp; 
end 
 
wire [1220:0]intermediate_reg_1; 
 
xor_module xor_module_inst_1_0(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2441]),.i2(intermediate_reg_0[2440]),.o(intermediate_reg_1[1220])); 
mux_module mux_module_inst_1_1(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2439]),.i2(intermediate_reg_0[2438]),.o(intermediate_reg_1[1219]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_2(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2437]),.i2(intermediate_reg_0[2436]),.o(intermediate_reg_1[1218]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_3(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2435]),.i2(intermediate_reg_0[2434]),.o(intermediate_reg_1[1217]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_4(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2433]),.i2(intermediate_reg_0[2432]),.o(intermediate_reg_1[1216]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_5(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2431]),.i2(intermediate_reg_0[2430]),.o(intermediate_reg_1[1215]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_6(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2429]),.i2(intermediate_reg_0[2428]),.o(intermediate_reg_1[1214])); 
mux_module mux_module_inst_1_7(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2427]),.i2(intermediate_reg_0[2426]),.o(intermediate_reg_1[1213]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_8(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2425]),.i2(intermediate_reg_0[2424]),.o(intermediate_reg_1[1212]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_9(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2423]),.i2(intermediate_reg_0[2422]),.o(intermediate_reg_1[1211]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_10(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2421]),.i2(intermediate_reg_0[2420]),.o(intermediate_reg_1[1210])); 
xor_module xor_module_inst_1_11(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2419]),.i2(intermediate_reg_0[2418]),.o(intermediate_reg_1[1209])); 
mux_module mux_module_inst_1_12(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2417]),.i2(intermediate_reg_0[2416]),.o(intermediate_reg_1[1208]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_13(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2415]),.i2(intermediate_reg_0[2414]),.o(intermediate_reg_1[1207]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_14(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2413]),.i2(intermediate_reg_0[2412]),.o(intermediate_reg_1[1206])); 
mux_module mux_module_inst_1_15(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2411]),.i2(intermediate_reg_0[2410]),.o(intermediate_reg_1[1205]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_16(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2409]),.i2(intermediate_reg_0[2408]),.o(intermediate_reg_1[1204]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_17(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2407]),.i2(intermediate_reg_0[2406]),.o(intermediate_reg_1[1203]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_18(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2405]),.i2(intermediate_reg_0[2404]),.o(intermediate_reg_1[1202])); 
xor_module xor_module_inst_1_19(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2403]),.i2(intermediate_reg_0[2402]),.o(intermediate_reg_1[1201])); 
mux_module mux_module_inst_1_20(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2401]),.i2(intermediate_reg_0[2400]),.o(intermediate_reg_1[1200]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_21(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2399]),.i2(intermediate_reg_0[2398]),.o(intermediate_reg_1[1199])); 
xor_module xor_module_inst_1_22(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2397]),.i2(intermediate_reg_0[2396]),.o(intermediate_reg_1[1198])); 
mux_module mux_module_inst_1_23(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2395]),.i2(intermediate_reg_0[2394]),.o(intermediate_reg_1[1197]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_24(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2393]),.i2(intermediate_reg_0[2392]),.o(intermediate_reg_1[1196])); 
mux_module mux_module_inst_1_25(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2391]),.i2(intermediate_reg_0[2390]),.o(intermediate_reg_1[1195]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_26(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2389]),.i2(intermediate_reg_0[2388]),.o(intermediate_reg_1[1194]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_27(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2387]),.i2(intermediate_reg_0[2386]),.o(intermediate_reg_1[1193])); 
mux_module mux_module_inst_1_28(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2385]),.i2(intermediate_reg_0[2384]),.o(intermediate_reg_1[1192]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_29(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2383]),.i2(intermediate_reg_0[2382]),.o(intermediate_reg_1[1191])); 
xor_module xor_module_inst_1_30(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2381]),.i2(intermediate_reg_0[2380]),.o(intermediate_reg_1[1190])); 
xor_module xor_module_inst_1_31(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2379]),.i2(intermediate_reg_0[2378]),.o(intermediate_reg_1[1189])); 
mux_module mux_module_inst_1_32(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2377]),.i2(intermediate_reg_0[2376]),.o(intermediate_reg_1[1188]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_33(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2375]),.i2(intermediate_reg_0[2374]),.o(intermediate_reg_1[1187])); 
xor_module xor_module_inst_1_34(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2373]),.i2(intermediate_reg_0[2372]),.o(intermediate_reg_1[1186])); 
mux_module mux_module_inst_1_35(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2371]),.i2(intermediate_reg_0[2370]),.o(intermediate_reg_1[1185]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_36(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2369]),.i2(intermediate_reg_0[2368]),.o(intermediate_reg_1[1184]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_37(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2367]),.i2(intermediate_reg_0[2366]),.o(intermediate_reg_1[1183])); 
xor_module xor_module_inst_1_38(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2365]),.i2(intermediate_reg_0[2364]),.o(intermediate_reg_1[1182])); 
mux_module mux_module_inst_1_39(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2363]),.i2(intermediate_reg_0[2362]),.o(intermediate_reg_1[1181]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_40(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2361]),.i2(intermediate_reg_0[2360]),.o(intermediate_reg_1[1180]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_41(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2359]),.i2(intermediate_reg_0[2358]),.o(intermediate_reg_1[1179]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_42(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2357]),.i2(intermediate_reg_0[2356]),.o(intermediate_reg_1[1178]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_43(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2355]),.i2(intermediate_reg_0[2354]),.o(intermediate_reg_1[1177]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_44(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2353]),.i2(intermediate_reg_0[2352]),.o(intermediate_reg_1[1176]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_45(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2351]),.i2(intermediate_reg_0[2350]),.o(intermediate_reg_1[1175]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_46(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2349]),.i2(intermediate_reg_0[2348]),.o(intermediate_reg_1[1174]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_47(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2347]),.i2(intermediate_reg_0[2346]),.o(intermediate_reg_1[1173])); 
xor_module xor_module_inst_1_48(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2345]),.i2(intermediate_reg_0[2344]),.o(intermediate_reg_1[1172])); 
xor_module xor_module_inst_1_49(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2343]),.i2(intermediate_reg_0[2342]),.o(intermediate_reg_1[1171])); 
xor_module xor_module_inst_1_50(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2341]),.i2(intermediate_reg_0[2340]),.o(intermediate_reg_1[1170])); 
mux_module mux_module_inst_1_51(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2339]),.i2(intermediate_reg_0[2338]),.o(intermediate_reg_1[1169]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_52(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2337]),.i2(intermediate_reg_0[2336]),.o(intermediate_reg_1[1168]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_53(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2335]),.i2(intermediate_reg_0[2334]),.o(intermediate_reg_1[1167]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_54(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2333]),.i2(intermediate_reg_0[2332]),.o(intermediate_reg_1[1166]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_55(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2331]),.i2(intermediate_reg_0[2330]),.o(intermediate_reg_1[1165])); 
mux_module mux_module_inst_1_56(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2329]),.i2(intermediate_reg_0[2328]),.o(intermediate_reg_1[1164]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_57(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2327]),.i2(intermediate_reg_0[2326]),.o(intermediate_reg_1[1163])); 
xor_module xor_module_inst_1_58(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2325]),.i2(intermediate_reg_0[2324]),.o(intermediate_reg_1[1162])); 
mux_module mux_module_inst_1_59(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2323]),.i2(intermediate_reg_0[2322]),.o(intermediate_reg_1[1161]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_60(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2321]),.i2(intermediate_reg_0[2320]),.o(intermediate_reg_1[1160])); 
xor_module xor_module_inst_1_61(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2319]),.i2(intermediate_reg_0[2318]),.o(intermediate_reg_1[1159])); 
mux_module mux_module_inst_1_62(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2317]),.i2(intermediate_reg_0[2316]),.o(intermediate_reg_1[1158]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_63(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2315]),.i2(intermediate_reg_0[2314]),.o(intermediate_reg_1[1157])); 
mux_module mux_module_inst_1_64(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2313]),.i2(intermediate_reg_0[2312]),.o(intermediate_reg_1[1156]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_65(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2311]),.i2(intermediate_reg_0[2310]),.o(intermediate_reg_1[1155])); 
xor_module xor_module_inst_1_66(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2309]),.i2(intermediate_reg_0[2308]),.o(intermediate_reg_1[1154])); 
xor_module xor_module_inst_1_67(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2307]),.i2(intermediate_reg_0[2306]),.o(intermediate_reg_1[1153])); 
mux_module mux_module_inst_1_68(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2305]),.i2(intermediate_reg_0[2304]),.o(intermediate_reg_1[1152]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_69(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2303]),.i2(intermediate_reg_0[2302]),.o(intermediate_reg_1[1151])); 
xor_module xor_module_inst_1_70(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2301]),.i2(intermediate_reg_0[2300]),.o(intermediate_reg_1[1150])); 
mux_module mux_module_inst_1_71(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2299]),.i2(intermediate_reg_0[2298]),.o(intermediate_reg_1[1149]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_72(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2297]),.i2(intermediate_reg_0[2296]),.o(intermediate_reg_1[1148]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_73(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2295]),.i2(intermediate_reg_0[2294]),.o(intermediate_reg_1[1147])); 
mux_module mux_module_inst_1_74(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2293]),.i2(intermediate_reg_0[2292]),.o(intermediate_reg_1[1146]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_75(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2291]),.i2(intermediate_reg_0[2290]),.o(intermediate_reg_1[1145])); 
mux_module mux_module_inst_1_76(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2289]),.i2(intermediate_reg_0[2288]),.o(intermediate_reg_1[1144]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_77(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2287]),.i2(intermediate_reg_0[2286]),.o(intermediate_reg_1[1143]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_78(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2285]),.i2(intermediate_reg_0[2284]),.o(intermediate_reg_1[1142])); 
mux_module mux_module_inst_1_79(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2283]),.i2(intermediate_reg_0[2282]),.o(intermediate_reg_1[1141]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_80(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2281]),.i2(intermediate_reg_0[2280]),.o(intermediate_reg_1[1140]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_81(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2279]),.i2(intermediate_reg_0[2278]),.o(intermediate_reg_1[1139])); 
xor_module xor_module_inst_1_82(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2277]),.i2(intermediate_reg_0[2276]),.o(intermediate_reg_1[1138])); 
xor_module xor_module_inst_1_83(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2275]),.i2(intermediate_reg_0[2274]),.o(intermediate_reg_1[1137])); 
mux_module mux_module_inst_1_84(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2273]),.i2(intermediate_reg_0[2272]),.o(intermediate_reg_1[1136]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_85(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2271]),.i2(intermediate_reg_0[2270]),.o(intermediate_reg_1[1135]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_86(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2269]),.i2(intermediate_reg_0[2268]),.o(intermediate_reg_1[1134])); 
mux_module mux_module_inst_1_87(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2267]),.i2(intermediate_reg_0[2266]),.o(intermediate_reg_1[1133]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_88(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2265]),.i2(intermediate_reg_0[2264]),.o(intermediate_reg_1[1132]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_89(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2263]),.i2(intermediate_reg_0[2262]),.o(intermediate_reg_1[1131]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_90(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2261]),.i2(intermediate_reg_0[2260]),.o(intermediate_reg_1[1130]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_91(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2259]),.i2(intermediate_reg_0[2258]),.o(intermediate_reg_1[1129])); 
mux_module mux_module_inst_1_92(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2257]),.i2(intermediate_reg_0[2256]),.o(intermediate_reg_1[1128]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_93(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2255]),.i2(intermediate_reg_0[2254]),.o(intermediate_reg_1[1127])); 
mux_module mux_module_inst_1_94(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2253]),.i2(intermediate_reg_0[2252]),.o(intermediate_reg_1[1126]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_95(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2251]),.i2(intermediate_reg_0[2250]),.o(intermediate_reg_1[1125]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_96(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2249]),.i2(intermediate_reg_0[2248]),.o(intermediate_reg_1[1124])); 
mux_module mux_module_inst_1_97(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2247]),.i2(intermediate_reg_0[2246]),.o(intermediate_reg_1[1123]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_98(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2245]),.i2(intermediate_reg_0[2244]),.o(intermediate_reg_1[1122]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_99(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2243]),.i2(intermediate_reg_0[2242]),.o(intermediate_reg_1[1121]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_100(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2241]),.i2(intermediate_reg_0[2240]),.o(intermediate_reg_1[1120])); 
mux_module mux_module_inst_1_101(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2239]),.i2(intermediate_reg_0[2238]),.o(intermediate_reg_1[1119]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_102(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2237]),.i2(intermediate_reg_0[2236]),.o(intermediate_reg_1[1118])); 
xor_module xor_module_inst_1_103(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2235]),.i2(intermediate_reg_0[2234]),.o(intermediate_reg_1[1117])); 
xor_module xor_module_inst_1_104(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2233]),.i2(intermediate_reg_0[2232]),.o(intermediate_reg_1[1116])); 
mux_module mux_module_inst_1_105(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2231]),.i2(intermediate_reg_0[2230]),.o(intermediate_reg_1[1115]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_106(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2229]),.i2(intermediate_reg_0[2228]),.o(intermediate_reg_1[1114])); 
xor_module xor_module_inst_1_107(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2227]),.i2(intermediate_reg_0[2226]),.o(intermediate_reg_1[1113])); 
xor_module xor_module_inst_1_108(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2225]),.i2(intermediate_reg_0[2224]),.o(intermediate_reg_1[1112])); 
mux_module mux_module_inst_1_109(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2223]),.i2(intermediate_reg_0[2222]),.o(intermediate_reg_1[1111]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_110(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2221]),.i2(intermediate_reg_0[2220]),.o(intermediate_reg_1[1110]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_111(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2219]),.i2(intermediate_reg_0[2218]),.o(intermediate_reg_1[1109])); 
mux_module mux_module_inst_1_112(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2217]),.i2(intermediate_reg_0[2216]),.o(intermediate_reg_1[1108]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_113(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2215]),.i2(intermediate_reg_0[2214]),.o(intermediate_reg_1[1107])); 
mux_module mux_module_inst_1_114(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2213]),.i2(intermediate_reg_0[2212]),.o(intermediate_reg_1[1106]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_115(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2211]),.i2(intermediate_reg_0[2210]),.o(intermediate_reg_1[1105]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_116(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2209]),.i2(intermediate_reg_0[2208]),.o(intermediate_reg_1[1104])); 
xor_module xor_module_inst_1_117(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2207]),.i2(intermediate_reg_0[2206]),.o(intermediate_reg_1[1103])); 
xor_module xor_module_inst_1_118(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2205]),.i2(intermediate_reg_0[2204]),.o(intermediate_reg_1[1102])); 
mux_module mux_module_inst_1_119(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2203]),.i2(intermediate_reg_0[2202]),.o(intermediate_reg_1[1101]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_120(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2201]),.i2(intermediate_reg_0[2200]),.o(intermediate_reg_1[1100])); 
mux_module mux_module_inst_1_121(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2199]),.i2(intermediate_reg_0[2198]),.o(intermediate_reg_1[1099]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_122(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2197]),.i2(intermediate_reg_0[2196]),.o(intermediate_reg_1[1098]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_123(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2195]),.i2(intermediate_reg_0[2194]),.o(intermediate_reg_1[1097]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_124(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2193]),.i2(intermediate_reg_0[2192]),.o(intermediate_reg_1[1096])); 
mux_module mux_module_inst_1_125(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2191]),.i2(intermediate_reg_0[2190]),.o(intermediate_reg_1[1095]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_126(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2189]),.i2(intermediate_reg_0[2188]),.o(intermediate_reg_1[1094]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_127(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2187]),.i2(intermediate_reg_0[2186]),.o(intermediate_reg_1[1093])); 
mux_module mux_module_inst_1_128(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2185]),.i2(intermediate_reg_0[2184]),.o(intermediate_reg_1[1092]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_129(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2183]),.i2(intermediate_reg_0[2182]),.o(intermediate_reg_1[1091])); 
xor_module xor_module_inst_1_130(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2181]),.i2(intermediate_reg_0[2180]),.o(intermediate_reg_1[1090])); 
xor_module xor_module_inst_1_131(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2179]),.i2(intermediate_reg_0[2178]),.o(intermediate_reg_1[1089])); 
xor_module xor_module_inst_1_132(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2177]),.i2(intermediate_reg_0[2176]),.o(intermediate_reg_1[1088])); 
mux_module mux_module_inst_1_133(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2175]),.i2(intermediate_reg_0[2174]),.o(intermediate_reg_1[1087]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_134(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2173]),.i2(intermediate_reg_0[2172]),.o(intermediate_reg_1[1086]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_135(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2171]),.i2(intermediate_reg_0[2170]),.o(intermediate_reg_1[1085]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_136(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2169]),.i2(intermediate_reg_0[2168]),.o(intermediate_reg_1[1084]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_137(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2167]),.i2(intermediate_reg_0[2166]),.o(intermediate_reg_1[1083]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_138(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2165]),.i2(intermediate_reg_0[2164]),.o(intermediate_reg_1[1082])); 
xor_module xor_module_inst_1_139(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2163]),.i2(intermediate_reg_0[2162]),.o(intermediate_reg_1[1081])); 
mux_module mux_module_inst_1_140(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2161]),.i2(intermediate_reg_0[2160]),.o(intermediate_reg_1[1080]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_141(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2159]),.i2(intermediate_reg_0[2158]),.o(intermediate_reg_1[1079])); 
mux_module mux_module_inst_1_142(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2157]),.i2(intermediate_reg_0[2156]),.o(intermediate_reg_1[1078]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_143(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2155]),.i2(intermediate_reg_0[2154]),.o(intermediate_reg_1[1077])); 
mux_module mux_module_inst_1_144(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2153]),.i2(intermediate_reg_0[2152]),.o(intermediate_reg_1[1076]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_145(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2151]),.i2(intermediate_reg_0[2150]),.o(intermediate_reg_1[1075])); 
xor_module xor_module_inst_1_146(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2149]),.i2(intermediate_reg_0[2148]),.o(intermediate_reg_1[1074])); 
mux_module mux_module_inst_1_147(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2147]),.i2(intermediate_reg_0[2146]),.o(intermediate_reg_1[1073]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_148(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2145]),.i2(intermediate_reg_0[2144]),.o(intermediate_reg_1[1072]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_149(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2143]),.i2(intermediate_reg_0[2142]),.o(intermediate_reg_1[1071])); 
mux_module mux_module_inst_1_150(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2141]),.i2(intermediate_reg_0[2140]),.o(intermediate_reg_1[1070]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_151(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2139]),.i2(intermediate_reg_0[2138]),.o(intermediate_reg_1[1069]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_152(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2137]),.i2(intermediate_reg_0[2136]),.o(intermediate_reg_1[1068]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_153(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2135]),.i2(intermediate_reg_0[2134]),.o(intermediate_reg_1[1067]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_154(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2133]),.i2(intermediate_reg_0[2132]),.o(intermediate_reg_1[1066]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_155(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2131]),.i2(intermediate_reg_0[2130]),.o(intermediate_reg_1[1065]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_156(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2129]),.i2(intermediate_reg_0[2128]),.o(intermediate_reg_1[1064])); 
xor_module xor_module_inst_1_157(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2127]),.i2(intermediate_reg_0[2126]),.o(intermediate_reg_1[1063])); 
xor_module xor_module_inst_1_158(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2125]),.i2(intermediate_reg_0[2124]),.o(intermediate_reg_1[1062])); 
xor_module xor_module_inst_1_159(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2123]),.i2(intermediate_reg_0[2122]),.o(intermediate_reg_1[1061])); 
xor_module xor_module_inst_1_160(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2121]),.i2(intermediate_reg_0[2120]),.o(intermediate_reg_1[1060])); 
xor_module xor_module_inst_1_161(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2119]),.i2(intermediate_reg_0[2118]),.o(intermediate_reg_1[1059])); 
mux_module mux_module_inst_1_162(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2117]),.i2(intermediate_reg_0[2116]),.o(intermediate_reg_1[1058]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_163(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2115]),.i2(intermediate_reg_0[2114]),.o(intermediate_reg_1[1057]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_164(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2113]),.i2(intermediate_reg_0[2112]),.o(intermediate_reg_1[1056]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_165(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2111]),.i2(intermediate_reg_0[2110]),.o(intermediate_reg_1[1055])); 
xor_module xor_module_inst_1_166(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2109]),.i2(intermediate_reg_0[2108]),.o(intermediate_reg_1[1054])); 
mux_module mux_module_inst_1_167(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2107]),.i2(intermediate_reg_0[2106]),.o(intermediate_reg_1[1053]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_168(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2105]),.i2(intermediate_reg_0[2104]),.o(intermediate_reg_1[1052]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_169(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2103]),.i2(intermediate_reg_0[2102]),.o(intermediate_reg_1[1051]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_170(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2101]),.i2(intermediate_reg_0[2100]),.o(intermediate_reg_1[1050]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_171(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2099]),.i2(intermediate_reg_0[2098]),.o(intermediate_reg_1[1049]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_172(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2097]),.i2(intermediate_reg_0[2096]),.o(intermediate_reg_1[1048])); 
mux_module mux_module_inst_1_173(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2095]),.i2(intermediate_reg_0[2094]),.o(intermediate_reg_1[1047]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_174(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2093]),.i2(intermediate_reg_0[2092]),.o(intermediate_reg_1[1046])); 
mux_module mux_module_inst_1_175(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2091]),.i2(intermediate_reg_0[2090]),.o(intermediate_reg_1[1045]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_176(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2089]),.i2(intermediate_reg_0[2088]),.o(intermediate_reg_1[1044]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_177(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2087]),.i2(intermediate_reg_0[2086]),.o(intermediate_reg_1[1043]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_178(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2085]),.i2(intermediate_reg_0[2084]),.o(intermediate_reg_1[1042]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_179(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2083]),.i2(intermediate_reg_0[2082]),.o(intermediate_reg_1[1041]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_180(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2081]),.i2(intermediate_reg_0[2080]),.o(intermediate_reg_1[1040]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_181(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2079]),.i2(intermediate_reg_0[2078]),.o(intermediate_reg_1[1039])); 
xor_module xor_module_inst_1_182(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2077]),.i2(intermediate_reg_0[2076]),.o(intermediate_reg_1[1038])); 
xor_module xor_module_inst_1_183(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2075]),.i2(intermediate_reg_0[2074]),.o(intermediate_reg_1[1037])); 
xor_module xor_module_inst_1_184(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2073]),.i2(intermediate_reg_0[2072]),.o(intermediate_reg_1[1036])); 
mux_module mux_module_inst_1_185(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2071]),.i2(intermediate_reg_0[2070]),.o(intermediate_reg_1[1035]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_186(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2069]),.i2(intermediate_reg_0[2068]),.o(intermediate_reg_1[1034])); 
mux_module mux_module_inst_1_187(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2067]),.i2(intermediate_reg_0[2066]),.o(intermediate_reg_1[1033]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_188(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2065]),.i2(intermediate_reg_0[2064]),.o(intermediate_reg_1[1032])); 
xor_module xor_module_inst_1_189(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2063]),.i2(intermediate_reg_0[2062]),.o(intermediate_reg_1[1031])); 
mux_module mux_module_inst_1_190(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2061]),.i2(intermediate_reg_0[2060]),.o(intermediate_reg_1[1030]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_191(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2059]),.i2(intermediate_reg_0[2058]),.o(intermediate_reg_1[1029])); 
mux_module mux_module_inst_1_192(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2057]),.i2(intermediate_reg_0[2056]),.o(intermediate_reg_1[1028]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_193(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2055]),.i2(intermediate_reg_0[2054]),.o(intermediate_reg_1[1027]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_194(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2053]),.i2(intermediate_reg_0[2052]),.o(intermediate_reg_1[1026])); 
mux_module mux_module_inst_1_195(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2051]),.i2(intermediate_reg_0[2050]),.o(intermediate_reg_1[1025]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_196(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2049]),.i2(intermediate_reg_0[2048]),.o(intermediate_reg_1[1024])); 
xor_module xor_module_inst_1_197(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2047]),.i2(intermediate_reg_0[2046]),.o(intermediate_reg_1[1023])); 
mux_module mux_module_inst_1_198(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2045]),.i2(intermediate_reg_0[2044]),.o(intermediate_reg_1[1022]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_199(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2043]),.i2(intermediate_reg_0[2042]),.o(intermediate_reg_1[1021]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_200(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2041]),.i2(intermediate_reg_0[2040]),.o(intermediate_reg_1[1020])); 
mux_module mux_module_inst_1_201(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2039]),.i2(intermediate_reg_0[2038]),.o(intermediate_reg_1[1019]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_202(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2037]),.i2(intermediate_reg_0[2036]),.o(intermediate_reg_1[1018])); 
xor_module xor_module_inst_1_203(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2035]),.i2(intermediate_reg_0[2034]),.o(intermediate_reg_1[1017])); 
mux_module mux_module_inst_1_204(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2033]),.i2(intermediate_reg_0[2032]),.o(intermediate_reg_1[1016]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_205(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2031]),.i2(intermediate_reg_0[2030]),.o(intermediate_reg_1[1015]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_206(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2029]),.i2(intermediate_reg_0[2028]),.o(intermediate_reg_1[1014])); 
xor_module xor_module_inst_1_207(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2027]),.i2(intermediate_reg_0[2026]),.o(intermediate_reg_1[1013])); 
xor_module xor_module_inst_1_208(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2025]),.i2(intermediate_reg_0[2024]),.o(intermediate_reg_1[1012])); 
xor_module xor_module_inst_1_209(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2023]),.i2(intermediate_reg_0[2022]),.o(intermediate_reg_1[1011])); 
mux_module mux_module_inst_1_210(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2021]),.i2(intermediate_reg_0[2020]),.o(intermediate_reg_1[1010]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_211(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2019]),.i2(intermediate_reg_0[2018]),.o(intermediate_reg_1[1009]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_212(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2017]),.i2(intermediate_reg_0[2016]),.o(intermediate_reg_1[1008]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_213(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2015]),.i2(intermediate_reg_0[2014]),.o(intermediate_reg_1[1007]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_214(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2013]),.i2(intermediate_reg_0[2012]),.o(intermediate_reg_1[1006])); 
mux_module mux_module_inst_1_215(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2011]),.i2(intermediate_reg_0[2010]),.o(intermediate_reg_1[1005]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_216(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2009]),.i2(intermediate_reg_0[2008]),.o(intermediate_reg_1[1004]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_217(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2007]),.i2(intermediate_reg_0[2006]),.o(intermediate_reg_1[1003])); 
xor_module xor_module_inst_1_218(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2005]),.i2(intermediate_reg_0[2004]),.o(intermediate_reg_1[1002])); 
mux_module mux_module_inst_1_219(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2003]),.i2(intermediate_reg_0[2002]),.o(intermediate_reg_1[1001]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_220(.clk(clk),.reset(reset),.i1(intermediate_reg_0[2001]),.i2(intermediate_reg_0[2000]),.o(intermediate_reg_1[1000]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_221(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1999]),.i2(intermediate_reg_0[1998]),.o(intermediate_reg_1[999]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_222(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1997]),.i2(intermediate_reg_0[1996]),.o(intermediate_reg_1[998]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_223(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1995]),.i2(intermediate_reg_0[1994]),.o(intermediate_reg_1[997])); 
mux_module mux_module_inst_1_224(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1993]),.i2(intermediate_reg_0[1992]),.o(intermediate_reg_1[996]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_225(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1991]),.i2(intermediate_reg_0[1990]),.o(intermediate_reg_1[995])); 
mux_module mux_module_inst_1_226(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1989]),.i2(intermediate_reg_0[1988]),.o(intermediate_reg_1[994]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_227(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1987]),.i2(intermediate_reg_0[1986]),.o(intermediate_reg_1[993])); 
mux_module mux_module_inst_1_228(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1985]),.i2(intermediate_reg_0[1984]),.o(intermediate_reg_1[992]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_229(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1983]),.i2(intermediate_reg_0[1982]),.o(intermediate_reg_1[991]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_230(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1981]),.i2(intermediate_reg_0[1980]),.o(intermediate_reg_1[990])); 
mux_module mux_module_inst_1_231(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1979]),.i2(intermediate_reg_0[1978]),.o(intermediate_reg_1[989]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_232(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1977]),.i2(intermediate_reg_0[1976]),.o(intermediate_reg_1[988]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_233(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1975]),.i2(intermediate_reg_0[1974]),.o(intermediate_reg_1[987]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_234(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1973]),.i2(intermediate_reg_0[1972]),.o(intermediate_reg_1[986])); 
mux_module mux_module_inst_1_235(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1971]),.i2(intermediate_reg_0[1970]),.o(intermediate_reg_1[985]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_236(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1969]),.i2(intermediate_reg_0[1968]),.o(intermediate_reg_1[984])); 
mux_module mux_module_inst_1_237(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1967]),.i2(intermediate_reg_0[1966]),.o(intermediate_reg_1[983]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_238(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1965]),.i2(intermediate_reg_0[1964]),.o(intermediate_reg_1[982]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_239(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1963]),.i2(intermediate_reg_0[1962]),.o(intermediate_reg_1[981]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_240(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1961]),.i2(intermediate_reg_0[1960]),.o(intermediate_reg_1[980]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_241(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1959]),.i2(intermediate_reg_0[1958]),.o(intermediate_reg_1[979])); 
mux_module mux_module_inst_1_242(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1957]),.i2(intermediate_reg_0[1956]),.o(intermediate_reg_1[978]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_243(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1955]),.i2(intermediate_reg_0[1954]),.o(intermediate_reg_1[977])); 
xor_module xor_module_inst_1_244(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1953]),.i2(intermediate_reg_0[1952]),.o(intermediate_reg_1[976])); 
mux_module mux_module_inst_1_245(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1951]),.i2(intermediate_reg_0[1950]),.o(intermediate_reg_1[975]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_246(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1949]),.i2(intermediate_reg_0[1948]),.o(intermediate_reg_1[974]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_247(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1947]),.i2(intermediate_reg_0[1946]),.o(intermediate_reg_1[973])); 
mux_module mux_module_inst_1_248(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1945]),.i2(intermediate_reg_0[1944]),.o(intermediate_reg_1[972]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_249(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1943]),.i2(intermediate_reg_0[1942]),.o(intermediate_reg_1[971])); 
mux_module mux_module_inst_1_250(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1941]),.i2(intermediate_reg_0[1940]),.o(intermediate_reg_1[970]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_251(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1939]),.i2(intermediate_reg_0[1938]),.o(intermediate_reg_1[969]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_252(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1937]),.i2(intermediate_reg_0[1936]),.o(intermediate_reg_1[968])); 
xor_module xor_module_inst_1_253(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1935]),.i2(intermediate_reg_0[1934]),.o(intermediate_reg_1[967])); 
xor_module xor_module_inst_1_254(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1933]),.i2(intermediate_reg_0[1932]),.o(intermediate_reg_1[966])); 
mux_module mux_module_inst_1_255(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1931]),.i2(intermediate_reg_0[1930]),.o(intermediate_reg_1[965]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_256(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1929]),.i2(intermediate_reg_0[1928]),.o(intermediate_reg_1[964])); 
mux_module mux_module_inst_1_257(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1927]),.i2(intermediate_reg_0[1926]),.o(intermediate_reg_1[963]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_258(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1925]),.i2(intermediate_reg_0[1924]),.o(intermediate_reg_1[962])); 
mux_module mux_module_inst_1_259(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1923]),.i2(intermediate_reg_0[1922]),.o(intermediate_reg_1[961]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_260(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1921]),.i2(intermediate_reg_0[1920]),.o(intermediate_reg_1[960]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_261(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1919]),.i2(intermediate_reg_0[1918]),.o(intermediate_reg_1[959]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_262(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1917]),.i2(intermediate_reg_0[1916]),.o(intermediate_reg_1[958])); 
xor_module xor_module_inst_1_263(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1915]),.i2(intermediate_reg_0[1914]),.o(intermediate_reg_1[957])); 
mux_module mux_module_inst_1_264(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1913]),.i2(intermediate_reg_0[1912]),.o(intermediate_reg_1[956]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_265(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1911]),.i2(intermediate_reg_0[1910]),.o(intermediate_reg_1[955]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_266(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1909]),.i2(intermediate_reg_0[1908]),.o(intermediate_reg_1[954])); 
xor_module xor_module_inst_1_267(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1907]),.i2(intermediate_reg_0[1906]),.o(intermediate_reg_1[953])); 
mux_module mux_module_inst_1_268(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1905]),.i2(intermediate_reg_0[1904]),.o(intermediate_reg_1[952]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_269(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1903]),.i2(intermediate_reg_0[1902]),.o(intermediate_reg_1[951]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_270(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1901]),.i2(intermediate_reg_0[1900]),.o(intermediate_reg_1[950]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_271(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1899]),.i2(intermediate_reg_0[1898]),.o(intermediate_reg_1[949])); 
mux_module mux_module_inst_1_272(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1897]),.i2(intermediate_reg_0[1896]),.o(intermediate_reg_1[948]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_273(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1895]),.i2(intermediate_reg_0[1894]),.o(intermediate_reg_1[947])); 
mux_module mux_module_inst_1_274(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1893]),.i2(intermediate_reg_0[1892]),.o(intermediate_reg_1[946]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_275(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1891]),.i2(intermediate_reg_0[1890]),.o(intermediate_reg_1[945])); 
mux_module mux_module_inst_1_276(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1889]),.i2(intermediate_reg_0[1888]),.o(intermediate_reg_1[944]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_277(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1887]),.i2(intermediate_reg_0[1886]),.o(intermediate_reg_1[943]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_278(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1885]),.i2(intermediate_reg_0[1884]),.o(intermediate_reg_1[942])); 
mux_module mux_module_inst_1_279(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1883]),.i2(intermediate_reg_0[1882]),.o(intermediate_reg_1[941]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_280(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1881]),.i2(intermediate_reg_0[1880]),.o(intermediate_reg_1[940])); 
xor_module xor_module_inst_1_281(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1879]),.i2(intermediate_reg_0[1878]),.o(intermediate_reg_1[939])); 
mux_module mux_module_inst_1_282(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1877]),.i2(intermediate_reg_0[1876]),.o(intermediate_reg_1[938]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_283(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1875]),.i2(intermediate_reg_0[1874]),.o(intermediate_reg_1[937]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_284(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1873]),.i2(intermediate_reg_0[1872]),.o(intermediate_reg_1[936])); 
xor_module xor_module_inst_1_285(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1871]),.i2(intermediate_reg_0[1870]),.o(intermediate_reg_1[935])); 
mux_module mux_module_inst_1_286(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1869]),.i2(intermediate_reg_0[1868]),.o(intermediate_reg_1[934]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_287(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1867]),.i2(intermediate_reg_0[1866]),.o(intermediate_reg_1[933])); 
xor_module xor_module_inst_1_288(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1865]),.i2(intermediate_reg_0[1864]),.o(intermediate_reg_1[932])); 
xor_module xor_module_inst_1_289(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1863]),.i2(intermediate_reg_0[1862]),.o(intermediate_reg_1[931])); 
xor_module xor_module_inst_1_290(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1861]),.i2(intermediate_reg_0[1860]),.o(intermediate_reg_1[930])); 
xor_module xor_module_inst_1_291(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1859]),.i2(intermediate_reg_0[1858]),.o(intermediate_reg_1[929])); 
xor_module xor_module_inst_1_292(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1857]),.i2(intermediate_reg_0[1856]),.o(intermediate_reg_1[928])); 
mux_module mux_module_inst_1_293(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1855]),.i2(intermediate_reg_0[1854]),.o(intermediate_reg_1[927]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_294(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1853]),.i2(intermediate_reg_0[1852]),.o(intermediate_reg_1[926]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_295(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1851]),.i2(intermediate_reg_0[1850]),.o(intermediate_reg_1[925])); 
xor_module xor_module_inst_1_296(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1849]),.i2(intermediate_reg_0[1848]),.o(intermediate_reg_1[924])); 
mux_module mux_module_inst_1_297(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1847]),.i2(intermediate_reg_0[1846]),.o(intermediate_reg_1[923]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_298(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1845]),.i2(intermediate_reg_0[1844]),.o(intermediate_reg_1[922]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_299(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1843]),.i2(intermediate_reg_0[1842]),.o(intermediate_reg_1[921])); 
xor_module xor_module_inst_1_300(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1841]),.i2(intermediate_reg_0[1840]),.o(intermediate_reg_1[920])); 
mux_module mux_module_inst_1_301(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1839]),.i2(intermediate_reg_0[1838]),.o(intermediate_reg_1[919]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_302(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1837]),.i2(intermediate_reg_0[1836]),.o(intermediate_reg_1[918])); 
mux_module mux_module_inst_1_303(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1835]),.i2(intermediate_reg_0[1834]),.o(intermediate_reg_1[917]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_304(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1833]),.i2(intermediate_reg_0[1832]),.o(intermediate_reg_1[916]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_305(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1831]),.i2(intermediate_reg_0[1830]),.o(intermediate_reg_1[915]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_306(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1829]),.i2(intermediate_reg_0[1828]),.o(intermediate_reg_1[914]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_307(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1827]),.i2(intermediate_reg_0[1826]),.o(intermediate_reg_1[913])); 
mux_module mux_module_inst_1_308(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1825]),.i2(intermediate_reg_0[1824]),.o(intermediate_reg_1[912]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_309(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1823]),.i2(intermediate_reg_0[1822]),.o(intermediate_reg_1[911])); 
xor_module xor_module_inst_1_310(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1821]),.i2(intermediate_reg_0[1820]),.o(intermediate_reg_1[910])); 
mux_module mux_module_inst_1_311(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1819]),.i2(intermediate_reg_0[1818]),.o(intermediate_reg_1[909]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_312(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1817]),.i2(intermediate_reg_0[1816]),.o(intermediate_reg_1[908])); 
xor_module xor_module_inst_1_313(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1815]),.i2(intermediate_reg_0[1814]),.o(intermediate_reg_1[907])); 
xor_module xor_module_inst_1_314(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1813]),.i2(intermediate_reg_0[1812]),.o(intermediate_reg_1[906])); 
xor_module xor_module_inst_1_315(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1811]),.i2(intermediate_reg_0[1810]),.o(intermediate_reg_1[905])); 
xor_module xor_module_inst_1_316(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1809]),.i2(intermediate_reg_0[1808]),.o(intermediate_reg_1[904])); 
mux_module mux_module_inst_1_317(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1807]),.i2(intermediate_reg_0[1806]),.o(intermediate_reg_1[903]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_318(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1805]),.i2(intermediate_reg_0[1804]),.o(intermediate_reg_1[902])); 
mux_module mux_module_inst_1_319(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1803]),.i2(intermediate_reg_0[1802]),.o(intermediate_reg_1[901]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_320(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1801]),.i2(intermediate_reg_0[1800]),.o(intermediate_reg_1[900]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_321(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1799]),.i2(intermediate_reg_0[1798]),.o(intermediate_reg_1[899]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_322(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1797]),.i2(intermediate_reg_0[1796]),.o(intermediate_reg_1[898])); 
xor_module xor_module_inst_1_323(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1795]),.i2(intermediate_reg_0[1794]),.o(intermediate_reg_1[897])); 
mux_module mux_module_inst_1_324(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1793]),.i2(intermediate_reg_0[1792]),.o(intermediate_reg_1[896]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_325(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1791]),.i2(intermediate_reg_0[1790]),.o(intermediate_reg_1[895]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_326(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1789]),.i2(intermediate_reg_0[1788]),.o(intermediate_reg_1[894]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_327(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1787]),.i2(intermediate_reg_0[1786]),.o(intermediate_reg_1[893])); 
xor_module xor_module_inst_1_328(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1785]),.i2(intermediate_reg_0[1784]),.o(intermediate_reg_1[892])); 
xor_module xor_module_inst_1_329(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1783]),.i2(intermediate_reg_0[1782]),.o(intermediate_reg_1[891])); 
xor_module xor_module_inst_1_330(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1781]),.i2(intermediate_reg_0[1780]),.o(intermediate_reg_1[890])); 
mux_module mux_module_inst_1_331(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1779]),.i2(intermediate_reg_0[1778]),.o(intermediate_reg_1[889]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_332(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1777]),.i2(intermediate_reg_0[1776]),.o(intermediate_reg_1[888])); 
xor_module xor_module_inst_1_333(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1775]),.i2(intermediate_reg_0[1774]),.o(intermediate_reg_1[887])); 
mux_module mux_module_inst_1_334(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1773]),.i2(intermediate_reg_0[1772]),.o(intermediate_reg_1[886]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_335(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1771]),.i2(intermediate_reg_0[1770]),.o(intermediate_reg_1[885]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_336(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1769]),.i2(intermediate_reg_0[1768]),.o(intermediate_reg_1[884])); 
xor_module xor_module_inst_1_337(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1767]),.i2(intermediate_reg_0[1766]),.o(intermediate_reg_1[883])); 
mux_module mux_module_inst_1_338(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1765]),.i2(intermediate_reg_0[1764]),.o(intermediate_reg_1[882]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_339(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1763]),.i2(intermediate_reg_0[1762]),.o(intermediate_reg_1[881]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_340(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1761]),.i2(intermediate_reg_0[1760]),.o(intermediate_reg_1[880]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_341(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1759]),.i2(intermediate_reg_0[1758]),.o(intermediate_reg_1[879])); 
mux_module mux_module_inst_1_342(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1757]),.i2(intermediate_reg_0[1756]),.o(intermediate_reg_1[878]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_343(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1755]),.i2(intermediate_reg_0[1754]),.o(intermediate_reg_1[877])); 
mux_module mux_module_inst_1_344(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1753]),.i2(intermediate_reg_0[1752]),.o(intermediate_reg_1[876]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_345(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1751]),.i2(intermediate_reg_0[1750]),.o(intermediate_reg_1[875])); 
mux_module mux_module_inst_1_346(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1749]),.i2(intermediate_reg_0[1748]),.o(intermediate_reg_1[874]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_347(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1747]),.i2(intermediate_reg_0[1746]),.o(intermediate_reg_1[873])); 
mux_module mux_module_inst_1_348(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1745]),.i2(intermediate_reg_0[1744]),.o(intermediate_reg_1[872]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_349(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1743]),.i2(intermediate_reg_0[1742]),.o(intermediate_reg_1[871])); 
xor_module xor_module_inst_1_350(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1741]),.i2(intermediate_reg_0[1740]),.o(intermediate_reg_1[870])); 
xor_module xor_module_inst_1_351(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1739]),.i2(intermediate_reg_0[1738]),.o(intermediate_reg_1[869])); 
xor_module xor_module_inst_1_352(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1737]),.i2(intermediate_reg_0[1736]),.o(intermediate_reg_1[868])); 
xor_module xor_module_inst_1_353(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1735]),.i2(intermediate_reg_0[1734]),.o(intermediate_reg_1[867])); 
mux_module mux_module_inst_1_354(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1733]),.i2(intermediate_reg_0[1732]),.o(intermediate_reg_1[866]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_355(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1731]),.i2(intermediate_reg_0[1730]),.o(intermediate_reg_1[865])); 
xor_module xor_module_inst_1_356(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1729]),.i2(intermediate_reg_0[1728]),.o(intermediate_reg_1[864])); 
xor_module xor_module_inst_1_357(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1727]),.i2(intermediate_reg_0[1726]),.o(intermediate_reg_1[863])); 
mux_module mux_module_inst_1_358(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1725]),.i2(intermediate_reg_0[1724]),.o(intermediate_reg_1[862]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_359(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1723]),.i2(intermediate_reg_0[1722]),.o(intermediate_reg_1[861])); 
mux_module mux_module_inst_1_360(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1721]),.i2(intermediate_reg_0[1720]),.o(intermediate_reg_1[860]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_361(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1719]),.i2(intermediate_reg_0[1718]),.o(intermediate_reg_1[859]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_362(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1717]),.i2(intermediate_reg_0[1716]),.o(intermediate_reg_1[858])); 
mux_module mux_module_inst_1_363(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1715]),.i2(intermediate_reg_0[1714]),.o(intermediate_reg_1[857]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_364(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1713]),.i2(intermediate_reg_0[1712]),.o(intermediate_reg_1[856]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_365(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1711]),.i2(intermediate_reg_0[1710]),.o(intermediate_reg_1[855]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_366(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1709]),.i2(intermediate_reg_0[1708]),.o(intermediate_reg_1[854]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_367(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1707]),.i2(intermediate_reg_0[1706]),.o(intermediate_reg_1[853]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_368(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1705]),.i2(intermediate_reg_0[1704]),.o(intermediate_reg_1[852])); 
xor_module xor_module_inst_1_369(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1703]),.i2(intermediate_reg_0[1702]),.o(intermediate_reg_1[851])); 
mux_module mux_module_inst_1_370(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1701]),.i2(intermediate_reg_0[1700]),.o(intermediate_reg_1[850]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_371(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1699]),.i2(intermediate_reg_0[1698]),.o(intermediate_reg_1[849])); 
xor_module xor_module_inst_1_372(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1697]),.i2(intermediate_reg_0[1696]),.o(intermediate_reg_1[848])); 
xor_module xor_module_inst_1_373(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1695]),.i2(intermediate_reg_0[1694]),.o(intermediate_reg_1[847])); 
xor_module xor_module_inst_1_374(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1693]),.i2(intermediate_reg_0[1692]),.o(intermediate_reg_1[846])); 
xor_module xor_module_inst_1_375(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1691]),.i2(intermediate_reg_0[1690]),.o(intermediate_reg_1[845])); 
mux_module mux_module_inst_1_376(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1689]),.i2(intermediate_reg_0[1688]),.o(intermediate_reg_1[844]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_377(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1687]),.i2(intermediate_reg_0[1686]),.o(intermediate_reg_1[843])); 
mux_module mux_module_inst_1_378(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1685]),.i2(intermediate_reg_0[1684]),.o(intermediate_reg_1[842]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_379(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1683]),.i2(intermediate_reg_0[1682]),.o(intermediate_reg_1[841])); 
xor_module xor_module_inst_1_380(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1681]),.i2(intermediate_reg_0[1680]),.o(intermediate_reg_1[840])); 
mux_module mux_module_inst_1_381(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1679]),.i2(intermediate_reg_0[1678]),.o(intermediate_reg_1[839]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_382(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1677]),.i2(intermediate_reg_0[1676]),.o(intermediate_reg_1[838])); 
xor_module xor_module_inst_1_383(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1675]),.i2(intermediate_reg_0[1674]),.o(intermediate_reg_1[837])); 
mux_module mux_module_inst_1_384(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1673]),.i2(intermediate_reg_0[1672]),.o(intermediate_reg_1[836]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_385(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1671]),.i2(intermediate_reg_0[1670]),.o(intermediate_reg_1[835]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_386(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1669]),.i2(intermediate_reg_0[1668]),.o(intermediate_reg_1[834])); 
mux_module mux_module_inst_1_387(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1667]),.i2(intermediate_reg_0[1666]),.o(intermediate_reg_1[833]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_388(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1665]),.i2(intermediate_reg_0[1664]),.o(intermediate_reg_1[832]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_389(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1663]),.i2(intermediate_reg_0[1662]),.o(intermediate_reg_1[831]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_390(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1661]),.i2(intermediate_reg_0[1660]),.o(intermediate_reg_1[830])); 
xor_module xor_module_inst_1_391(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1659]),.i2(intermediate_reg_0[1658]),.o(intermediate_reg_1[829])); 
mux_module mux_module_inst_1_392(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1657]),.i2(intermediate_reg_0[1656]),.o(intermediate_reg_1[828]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_393(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1655]),.i2(intermediate_reg_0[1654]),.o(intermediate_reg_1[827]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_394(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1653]),.i2(intermediate_reg_0[1652]),.o(intermediate_reg_1[826])); 
xor_module xor_module_inst_1_395(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1651]),.i2(intermediate_reg_0[1650]),.o(intermediate_reg_1[825])); 
mux_module mux_module_inst_1_396(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1649]),.i2(intermediate_reg_0[1648]),.o(intermediate_reg_1[824]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_397(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1647]),.i2(intermediate_reg_0[1646]),.o(intermediate_reg_1[823]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_398(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1645]),.i2(intermediate_reg_0[1644]),.o(intermediate_reg_1[822])); 
mux_module mux_module_inst_1_399(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1643]),.i2(intermediate_reg_0[1642]),.o(intermediate_reg_1[821]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_400(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1641]),.i2(intermediate_reg_0[1640]),.o(intermediate_reg_1[820]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_401(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1639]),.i2(intermediate_reg_0[1638]),.o(intermediate_reg_1[819]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_402(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1637]),.i2(intermediate_reg_0[1636]),.o(intermediate_reg_1[818]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_403(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1635]),.i2(intermediate_reg_0[1634]),.o(intermediate_reg_1[817]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_404(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1633]),.i2(intermediate_reg_0[1632]),.o(intermediate_reg_1[816]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_405(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1631]),.i2(intermediate_reg_0[1630]),.o(intermediate_reg_1[815]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_406(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1629]),.i2(intermediate_reg_0[1628]),.o(intermediate_reg_1[814])); 
xor_module xor_module_inst_1_407(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1627]),.i2(intermediate_reg_0[1626]),.o(intermediate_reg_1[813])); 
mux_module mux_module_inst_1_408(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1625]),.i2(intermediate_reg_0[1624]),.o(intermediate_reg_1[812]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_409(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1623]),.i2(intermediate_reg_0[1622]),.o(intermediate_reg_1[811])); 
xor_module xor_module_inst_1_410(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1621]),.i2(intermediate_reg_0[1620]),.o(intermediate_reg_1[810])); 
mux_module mux_module_inst_1_411(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1619]),.i2(intermediate_reg_0[1618]),.o(intermediate_reg_1[809]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_412(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1617]),.i2(intermediate_reg_0[1616]),.o(intermediate_reg_1[808]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_413(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1615]),.i2(intermediate_reg_0[1614]),.o(intermediate_reg_1[807]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_414(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1613]),.i2(intermediate_reg_0[1612]),.o(intermediate_reg_1[806])); 
mux_module mux_module_inst_1_415(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1611]),.i2(intermediate_reg_0[1610]),.o(intermediate_reg_1[805]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_416(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1609]),.i2(intermediate_reg_0[1608]),.o(intermediate_reg_1[804]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_417(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1607]),.i2(intermediate_reg_0[1606]),.o(intermediate_reg_1[803])); 
xor_module xor_module_inst_1_418(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1605]),.i2(intermediate_reg_0[1604]),.o(intermediate_reg_1[802])); 
mux_module mux_module_inst_1_419(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1603]),.i2(intermediate_reg_0[1602]),.o(intermediate_reg_1[801]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_420(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1601]),.i2(intermediate_reg_0[1600]),.o(intermediate_reg_1[800]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_421(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1599]),.i2(intermediate_reg_0[1598]),.o(intermediate_reg_1[799])); 
mux_module mux_module_inst_1_422(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1597]),.i2(intermediate_reg_0[1596]),.o(intermediate_reg_1[798]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_423(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1595]),.i2(intermediate_reg_0[1594]),.o(intermediate_reg_1[797])); 
mux_module mux_module_inst_1_424(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1593]),.i2(intermediate_reg_0[1592]),.o(intermediate_reg_1[796]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_425(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1591]),.i2(intermediate_reg_0[1590]),.o(intermediate_reg_1[795]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_426(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1589]),.i2(intermediate_reg_0[1588]),.o(intermediate_reg_1[794]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_427(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1587]),.i2(intermediate_reg_0[1586]),.o(intermediate_reg_1[793]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_428(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1585]),.i2(intermediate_reg_0[1584]),.o(intermediate_reg_1[792])); 
mux_module mux_module_inst_1_429(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1583]),.i2(intermediate_reg_0[1582]),.o(intermediate_reg_1[791]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_430(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1581]),.i2(intermediate_reg_0[1580]),.o(intermediate_reg_1[790])); 
xor_module xor_module_inst_1_431(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1579]),.i2(intermediate_reg_0[1578]),.o(intermediate_reg_1[789])); 
mux_module mux_module_inst_1_432(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1577]),.i2(intermediate_reg_0[1576]),.o(intermediate_reg_1[788]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_433(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1575]),.i2(intermediate_reg_0[1574]),.o(intermediate_reg_1[787]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_434(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1573]),.i2(intermediate_reg_0[1572]),.o(intermediate_reg_1[786]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_435(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1571]),.i2(intermediate_reg_0[1570]),.o(intermediate_reg_1[785]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_436(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1569]),.i2(intermediate_reg_0[1568]),.o(intermediate_reg_1[784])); 
mux_module mux_module_inst_1_437(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1567]),.i2(intermediate_reg_0[1566]),.o(intermediate_reg_1[783]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_438(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1565]),.i2(intermediate_reg_0[1564]),.o(intermediate_reg_1[782]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_439(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1563]),.i2(intermediate_reg_0[1562]),.o(intermediate_reg_1[781]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_440(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1561]),.i2(intermediate_reg_0[1560]),.o(intermediate_reg_1[780]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_441(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1559]),.i2(intermediate_reg_0[1558]),.o(intermediate_reg_1[779])); 
mux_module mux_module_inst_1_442(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1557]),.i2(intermediate_reg_0[1556]),.o(intermediate_reg_1[778]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_443(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1555]),.i2(intermediate_reg_0[1554]),.o(intermediate_reg_1[777]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_444(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1553]),.i2(intermediate_reg_0[1552]),.o(intermediate_reg_1[776]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_445(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1551]),.i2(intermediate_reg_0[1550]),.o(intermediate_reg_1[775])); 
mux_module mux_module_inst_1_446(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1549]),.i2(intermediate_reg_0[1548]),.o(intermediate_reg_1[774]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_447(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1547]),.i2(intermediate_reg_0[1546]),.o(intermediate_reg_1[773])); 
mux_module mux_module_inst_1_448(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1545]),.i2(intermediate_reg_0[1544]),.o(intermediate_reg_1[772]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_449(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1543]),.i2(intermediate_reg_0[1542]),.o(intermediate_reg_1[771])); 
mux_module mux_module_inst_1_450(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1541]),.i2(intermediate_reg_0[1540]),.o(intermediate_reg_1[770]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_451(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1539]),.i2(intermediate_reg_0[1538]),.o(intermediate_reg_1[769]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_452(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1537]),.i2(intermediate_reg_0[1536]),.o(intermediate_reg_1[768])); 
xor_module xor_module_inst_1_453(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1535]),.i2(intermediate_reg_0[1534]),.o(intermediate_reg_1[767])); 
xor_module xor_module_inst_1_454(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1533]),.i2(intermediate_reg_0[1532]),.o(intermediate_reg_1[766])); 
xor_module xor_module_inst_1_455(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1531]),.i2(intermediate_reg_0[1530]),.o(intermediate_reg_1[765])); 
xor_module xor_module_inst_1_456(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1529]),.i2(intermediate_reg_0[1528]),.o(intermediate_reg_1[764])); 
mux_module mux_module_inst_1_457(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1527]),.i2(intermediate_reg_0[1526]),.o(intermediate_reg_1[763]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_458(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1525]),.i2(intermediate_reg_0[1524]),.o(intermediate_reg_1[762])); 
mux_module mux_module_inst_1_459(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1523]),.i2(intermediate_reg_0[1522]),.o(intermediate_reg_1[761]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_460(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1521]),.i2(intermediate_reg_0[1520]),.o(intermediate_reg_1[760]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_461(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1519]),.i2(intermediate_reg_0[1518]),.o(intermediate_reg_1[759]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_462(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1517]),.i2(intermediate_reg_0[1516]),.o(intermediate_reg_1[758]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_463(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1515]),.i2(intermediate_reg_0[1514]),.o(intermediate_reg_1[757])); 
mux_module mux_module_inst_1_464(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1513]),.i2(intermediate_reg_0[1512]),.o(intermediate_reg_1[756]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_465(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1511]),.i2(intermediate_reg_0[1510]),.o(intermediate_reg_1[755])); 
xor_module xor_module_inst_1_466(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1509]),.i2(intermediate_reg_0[1508]),.o(intermediate_reg_1[754])); 
xor_module xor_module_inst_1_467(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1507]),.i2(intermediate_reg_0[1506]),.o(intermediate_reg_1[753])); 
xor_module xor_module_inst_1_468(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1505]),.i2(intermediate_reg_0[1504]),.o(intermediate_reg_1[752])); 
xor_module xor_module_inst_1_469(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1503]),.i2(intermediate_reg_0[1502]),.o(intermediate_reg_1[751])); 
mux_module mux_module_inst_1_470(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1501]),.i2(intermediate_reg_0[1500]),.o(intermediate_reg_1[750]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_471(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1499]),.i2(intermediate_reg_0[1498]),.o(intermediate_reg_1[749]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_472(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1497]),.i2(intermediate_reg_0[1496]),.o(intermediate_reg_1[748])); 
mux_module mux_module_inst_1_473(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1495]),.i2(intermediate_reg_0[1494]),.o(intermediate_reg_1[747]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_474(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1493]),.i2(intermediate_reg_0[1492]),.o(intermediate_reg_1[746]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_475(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1491]),.i2(intermediate_reg_0[1490]),.o(intermediate_reg_1[745])); 
mux_module mux_module_inst_1_476(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1489]),.i2(intermediate_reg_0[1488]),.o(intermediate_reg_1[744]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_477(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1487]),.i2(intermediate_reg_0[1486]),.o(intermediate_reg_1[743]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_478(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1485]),.i2(intermediate_reg_0[1484]),.o(intermediate_reg_1[742])); 
mux_module mux_module_inst_1_479(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1483]),.i2(intermediate_reg_0[1482]),.o(intermediate_reg_1[741]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_480(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1481]),.i2(intermediate_reg_0[1480]),.o(intermediate_reg_1[740])); 
mux_module mux_module_inst_1_481(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1479]),.i2(intermediate_reg_0[1478]),.o(intermediate_reg_1[739]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_482(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1477]),.i2(intermediate_reg_0[1476]),.o(intermediate_reg_1[738])); 
mux_module mux_module_inst_1_483(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1475]),.i2(intermediate_reg_0[1474]),.o(intermediate_reg_1[737]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_484(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1473]),.i2(intermediate_reg_0[1472]),.o(intermediate_reg_1[736]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_485(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1471]),.i2(intermediate_reg_0[1470]),.o(intermediate_reg_1[735]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_486(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1469]),.i2(intermediate_reg_0[1468]),.o(intermediate_reg_1[734])); 
mux_module mux_module_inst_1_487(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1467]),.i2(intermediate_reg_0[1466]),.o(intermediate_reg_1[733]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_488(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1465]),.i2(intermediate_reg_0[1464]),.o(intermediate_reg_1[732]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_489(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1463]),.i2(intermediate_reg_0[1462]),.o(intermediate_reg_1[731]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_490(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1461]),.i2(intermediate_reg_0[1460]),.o(intermediate_reg_1[730]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_491(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1459]),.i2(intermediate_reg_0[1458]),.o(intermediate_reg_1[729])); 
xor_module xor_module_inst_1_492(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1457]),.i2(intermediate_reg_0[1456]),.o(intermediate_reg_1[728])); 
mux_module mux_module_inst_1_493(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1455]),.i2(intermediate_reg_0[1454]),.o(intermediate_reg_1[727]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_494(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1453]),.i2(intermediate_reg_0[1452]),.o(intermediate_reg_1[726]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_495(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1451]),.i2(intermediate_reg_0[1450]),.o(intermediate_reg_1[725])); 
xor_module xor_module_inst_1_496(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1449]),.i2(intermediate_reg_0[1448]),.o(intermediate_reg_1[724])); 
mux_module mux_module_inst_1_497(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1447]),.i2(intermediate_reg_0[1446]),.o(intermediate_reg_1[723]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_498(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1445]),.i2(intermediate_reg_0[1444]),.o(intermediate_reg_1[722]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_499(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1443]),.i2(intermediate_reg_0[1442]),.o(intermediate_reg_1[721]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_500(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1441]),.i2(intermediate_reg_0[1440]),.o(intermediate_reg_1[720]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_501(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1439]),.i2(intermediate_reg_0[1438]),.o(intermediate_reg_1[719]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_502(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1437]),.i2(intermediate_reg_0[1436]),.o(intermediate_reg_1[718]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_503(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1435]),.i2(intermediate_reg_0[1434]),.o(intermediate_reg_1[717]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_504(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1433]),.i2(intermediate_reg_0[1432]),.o(intermediate_reg_1[716]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_505(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1431]),.i2(intermediate_reg_0[1430]),.o(intermediate_reg_1[715])); 
xor_module xor_module_inst_1_506(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1429]),.i2(intermediate_reg_0[1428]),.o(intermediate_reg_1[714])); 
mux_module mux_module_inst_1_507(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1427]),.i2(intermediate_reg_0[1426]),.o(intermediate_reg_1[713]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_508(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1425]),.i2(intermediate_reg_0[1424]),.o(intermediate_reg_1[712]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_509(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1423]),.i2(intermediate_reg_0[1422]),.o(intermediate_reg_1[711])); 
mux_module mux_module_inst_1_510(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1421]),.i2(intermediate_reg_0[1420]),.o(intermediate_reg_1[710]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_511(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1419]),.i2(intermediate_reg_0[1418]),.o(intermediate_reg_1[709])); 
mux_module mux_module_inst_1_512(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1417]),.i2(intermediate_reg_0[1416]),.o(intermediate_reg_1[708]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_513(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1415]),.i2(intermediate_reg_0[1414]),.o(intermediate_reg_1[707])); 
mux_module mux_module_inst_1_514(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1413]),.i2(intermediate_reg_0[1412]),.o(intermediate_reg_1[706]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_515(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1411]),.i2(intermediate_reg_0[1410]),.o(intermediate_reg_1[705])); 
mux_module mux_module_inst_1_516(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1409]),.i2(intermediate_reg_0[1408]),.o(intermediate_reg_1[704]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_517(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1407]),.i2(intermediate_reg_0[1406]),.o(intermediate_reg_1[703])); 
mux_module mux_module_inst_1_518(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1405]),.i2(intermediate_reg_0[1404]),.o(intermediate_reg_1[702]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_519(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1403]),.i2(intermediate_reg_0[1402]),.o(intermediate_reg_1[701]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_520(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1401]),.i2(intermediate_reg_0[1400]),.o(intermediate_reg_1[700]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_521(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1399]),.i2(intermediate_reg_0[1398]),.o(intermediate_reg_1[699])); 
mux_module mux_module_inst_1_522(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1397]),.i2(intermediate_reg_0[1396]),.o(intermediate_reg_1[698]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_523(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1395]),.i2(intermediate_reg_0[1394]),.o(intermediate_reg_1[697])); 
xor_module xor_module_inst_1_524(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1393]),.i2(intermediate_reg_0[1392]),.o(intermediate_reg_1[696])); 
xor_module xor_module_inst_1_525(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1391]),.i2(intermediate_reg_0[1390]),.o(intermediate_reg_1[695])); 
xor_module xor_module_inst_1_526(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1389]),.i2(intermediate_reg_0[1388]),.o(intermediate_reg_1[694])); 
xor_module xor_module_inst_1_527(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1387]),.i2(intermediate_reg_0[1386]),.o(intermediate_reg_1[693])); 
mux_module mux_module_inst_1_528(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1385]),.i2(intermediate_reg_0[1384]),.o(intermediate_reg_1[692]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_529(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1383]),.i2(intermediate_reg_0[1382]),.o(intermediate_reg_1[691])); 
xor_module xor_module_inst_1_530(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1381]),.i2(intermediate_reg_0[1380]),.o(intermediate_reg_1[690])); 
xor_module xor_module_inst_1_531(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1379]),.i2(intermediate_reg_0[1378]),.o(intermediate_reg_1[689])); 
mux_module mux_module_inst_1_532(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1377]),.i2(intermediate_reg_0[1376]),.o(intermediate_reg_1[688]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_533(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1375]),.i2(intermediate_reg_0[1374]),.o(intermediate_reg_1[687]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_534(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1373]),.i2(intermediate_reg_0[1372]),.o(intermediate_reg_1[686]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_535(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1371]),.i2(intermediate_reg_0[1370]),.o(intermediate_reg_1[685]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_536(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1369]),.i2(intermediate_reg_0[1368]),.o(intermediate_reg_1[684])); 
xor_module xor_module_inst_1_537(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1367]),.i2(intermediate_reg_0[1366]),.o(intermediate_reg_1[683])); 
mux_module mux_module_inst_1_538(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1365]),.i2(intermediate_reg_0[1364]),.o(intermediate_reg_1[682]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_539(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1363]),.i2(intermediate_reg_0[1362]),.o(intermediate_reg_1[681])); 
xor_module xor_module_inst_1_540(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1361]),.i2(intermediate_reg_0[1360]),.o(intermediate_reg_1[680])); 
mux_module mux_module_inst_1_541(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1359]),.i2(intermediate_reg_0[1358]),.o(intermediate_reg_1[679]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_542(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1357]),.i2(intermediate_reg_0[1356]),.o(intermediate_reg_1[678])); 
mux_module mux_module_inst_1_543(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1355]),.i2(intermediate_reg_0[1354]),.o(intermediate_reg_1[677]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_544(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1353]),.i2(intermediate_reg_0[1352]),.o(intermediate_reg_1[676]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_545(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1351]),.i2(intermediate_reg_0[1350]),.o(intermediate_reg_1[675]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_546(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1349]),.i2(intermediate_reg_0[1348]),.o(intermediate_reg_1[674]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_547(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1347]),.i2(intermediate_reg_0[1346]),.o(intermediate_reg_1[673])); 
xor_module xor_module_inst_1_548(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1345]),.i2(intermediate_reg_0[1344]),.o(intermediate_reg_1[672])); 
mux_module mux_module_inst_1_549(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1343]),.i2(intermediate_reg_0[1342]),.o(intermediate_reg_1[671]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_550(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1341]),.i2(intermediate_reg_0[1340]),.o(intermediate_reg_1[670])); 
mux_module mux_module_inst_1_551(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1339]),.i2(intermediate_reg_0[1338]),.o(intermediate_reg_1[669]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_552(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1337]),.i2(intermediate_reg_0[1336]),.o(intermediate_reg_1[668])); 
xor_module xor_module_inst_1_553(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1335]),.i2(intermediate_reg_0[1334]),.o(intermediate_reg_1[667])); 
mux_module mux_module_inst_1_554(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1333]),.i2(intermediate_reg_0[1332]),.o(intermediate_reg_1[666]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_555(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1331]),.i2(intermediate_reg_0[1330]),.o(intermediate_reg_1[665]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_556(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1329]),.i2(intermediate_reg_0[1328]),.o(intermediate_reg_1[664]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_557(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1327]),.i2(intermediate_reg_0[1326]),.o(intermediate_reg_1[663])); 
xor_module xor_module_inst_1_558(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1325]),.i2(intermediate_reg_0[1324]),.o(intermediate_reg_1[662])); 
xor_module xor_module_inst_1_559(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1323]),.i2(intermediate_reg_0[1322]),.o(intermediate_reg_1[661])); 
mux_module mux_module_inst_1_560(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1321]),.i2(intermediate_reg_0[1320]),.o(intermediate_reg_1[660]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_561(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1319]),.i2(intermediate_reg_0[1318]),.o(intermediate_reg_1[659])); 
xor_module xor_module_inst_1_562(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1317]),.i2(intermediate_reg_0[1316]),.o(intermediate_reg_1[658])); 
xor_module xor_module_inst_1_563(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1315]),.i2(intermediate_reg_0[1314]),.o(intermediate_reg_1[657])); 
mux_module mux_module_inst_1_564(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1313]),.i2(intermediate_reg_0[1312]),.o(intermediate_reg_1[656]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_565(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1311]),.i2(intermediate_reg_0[1310]),.o(intermediate_reg_1[655])); 
xor_module xor_module_inst_1_566(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1309]),.i2(intermediate_reg_0[1308]),.o(intermediate_reg_1[654])); 
mux_module mux_module_inst_1_567(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1307]),.i2(intermediate_reg_0[1306]),.o(intermediate_reg_1[653]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_568(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1305]),.i2(intermediate_reg_0[1304]),.o(intermediate_reg_1[652]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_569(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1303]),.i2(intermediate_reg_0[1302]),.o(intermediate_reg_1[651])); 
mux_module mux_module_inst_1_570(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1301]),.i2(intermediate_reg_0[1300]),.o(intermediate_reg_1[650]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_571(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1299]),.i2(intermediate_reg_0[1298]),.o(intermediate_reg_1[649])); 
mux_module mux_module_inst_1_572(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1297]),.i2(intermediate_reg_0[1296]),.o(intermediate_reg_1[648]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_573(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1295]),.i2(intermediate_reg_0[1294]),.o(intermediate_reg_1[647])); 
mux_module mux_module_inst_1_574(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1293]),.i2(intermediate_reg_0[1292]),.o(intermediate_reg_1[646]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_575(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1291]),.i2(intermediate_reg_0[1290]),.o(intermediate_reg_1[645]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_576(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1289]),.i2(intermediate_reg_0[1288]),.o(intermediate_reg_1[644])); 
mux_module mux_module_inst_1_577(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1287]),.i2(intermediate_reg_0[1286]),.o(intermediate_reg_1[643]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_578(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1285]),.i2(intermediate_reg_0[1284]),.o(intermediate_reg_1[642]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_579(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1283]),.i2(intermediate_reg_0[1282]),.o(intermediate_reg_1[641])); 
xor_module xor_module_inst_1_580(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1281]),.i2(intermediate_reg_0[1280]),.o(intermediate_reg_1[640])); 
mux_module mux_module_inst_1_581(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1279]),.i2(intermediate_reg_0[1278]),.o(intermediate_reg_1[639]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_582(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1277]),.i2(intermediate_reg_0[1276]),.o(intermediate_reg_1[638]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_583(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1275]),.i2(intermediate_reg_0[1274]),.o(intermediate_reg_1[637]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_584(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1273]),.i2(intermediate_reg_0[1272]),.o(intermediate_reg_1[636]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_585(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1271]),.i2(intermediate_reg_0[1270]),.o(intermediate_reg_1[635]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_586(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1269]),.i2(intermediate_reg_0[1268]),.o(intermediate_reg_1[634])); 
xor_module xor_module_inst_1_587(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1267]),.i2(intermediate_reg_0[1266]),.o(intermediate_reg_1[633])); 
mux_module mux_module_inst_1_588(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1265]),.i2(intermediate_reg_0[1264]),.o(intermediate_reg_1[632]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_589(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1263]),.i2(intermediate_reg_0[1262]),.o(intermediate_reg_1[631])); 
xor_module xor_module_inst_1_590(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1261]),.i2(intermediate_reg_0[1260]),.o(intermediate_reg_1[630])); 
xor_module xor_module_inst_1_591(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1259]),.i2(intermediate_reg_0[1258]),.o(intermediate_reg_1[629])); 
xor_module xor_module_inst_1_592(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1257]),.i2(intermediate_reg_0[1256]),.o(intermediate_reg_1[628])); 
xor_module xor_module_inst_1_593(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1255]),.i2(intermediate_reg_0[1254]),.o(intermediate_reg_1[627])); 
xor_module xor_module_inst_1_594(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1253]),.i2(intermediate_reg_0[1252]),.o(intermediate_reg_1[626])); 
xor_module xor_module_inst_1_595(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1251]),.i2(intermediate_reg_0[1250]),.o(intermediate_reg_1[625])); 
mux_module mux_module_inst_1_596(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1249]),.i2(intermediate_reg_0[1248]),.o(intermediate_reg_1[624]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_597(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1247]),.i2(intermediate_reg_0[1246]),.o(intermediate_reg_1[623])); 
mux_module mux_module_inst_1_598(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1245]),.i2(intermediate_reg_0[1244]),.o(intermediate_reg_1[622]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_599(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1243]),.i2(intermediate_reg_0[1242]),.o(intermediate_reg_1[621]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_600(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1241]),.i2(intermediate_reg_0[1240]),.o(intermediate_reg_1[620]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_601(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1239]),.i2(intermediate_reg_0[1238]),.o(intermediate_reg_1[619]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_602(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1237]),.i2(intermediate_reg_0[1236]),.o(intermediate_reg_1[618])); 
mux_module mux_module_inst_1_603(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1235]),.i2(intermediate_reg_0[1234]),.o(intermediate_reg_1[617]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_604(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1233]),.i2(intermediate_reg_0[1232]),.o(intermediate_reg_1[616]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_605(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1231]),.i2(intermediate_reg_0[1230]),.o(intermediate_reg_1[615])); 
mux_module mux_module_inst_1_606(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1229]),.i2(intermediate_reg_0[1228]),.o(intermediate_reg_1[614]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_607(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1227]),.i2(intermediate_reg_0[1226]),.o(intermediate_reg_1[613])); 
xor_module xor_module_inst_1_608(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1225]),.i2(intermediate_reg_0[1224]),.o(intermediate_reg_1[612])); 
xor_module xor_module_inst_1_609(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1223]),.i2(intermediate_reg_0[1222]),.o(intermediate_reg_1[611])); 
mux_module mux_module_inst_1_610(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1221]),.i2(intermediate_reg_0[1220]),.o(intermediate_reg_1[610]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_611(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1219]),.i2(intermediate_reg_0[1218]),.o(intermediate_reg_1[609]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_612(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1217]),.i2(intermediate_reg_0[1216]),.o(intermediate_reg_1[608]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_613(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1215]),.i2(intermediate_reg_0[1214]),.o(intermediate_reg_1[607])); 
xor_module xor_module_inst_1_614(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1213]),.i2(intermediate_reg_0[1212]),.o(intermediate_reg_1[606])); 
xor_module xor_module_inst_1_615(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1211]),.i2(intermediate_reg_0[1210]),.o(intermediate_reg_1[605])); 
mux_module mux_module_inst_1_616(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1209]),.i2(intermediate_reg_0[1208]),.o(intermediate_reg_1[604]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_617(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1207]),.i2(intermediate_reg_0[1206]),.o(intermediate_reg_1[603])); 
xor_module xor_module_inst_1_618(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1205]),.i2(intermediate_reg_0[1204]),.o(intermediate_reg_1[602])); 
xor_module xor_module_inst_1_619(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1203]),.i2(intermediate_reg_0[1202]),.o(intermediate_reg_1[601])); 
mux_module mux_module_inst_1_620(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1201]),.i2(intermediate_reg_0[1200]),.o(intermediate_reg_1[600]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_621(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1199]),.i2(intermediate_reg_0[1198]),.o(intermediate_reg_1[599]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_622(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1197]),.i2(intermediate_reg_0[1196]),.o(intermediate_reg_1[598])); 
mux_module mux_module_inst_1_623(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1195]),.i2(intermediate_reg_0[1194]),.o(intermediate_reg_1[597]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_624(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1193]),.i2(intermediate_reg_0[1192]),.o(intermediate_reg_1[596]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_625(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1191]),.i2(intermediate_reg_0[1190]),.o(intermediate_reg_1[595])); 
xor_module xor_module_inst_1_626(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1189]),.i2(intermediate_reg_0[1188]),.o(intermediate_reg_1[594])); 
xor_module xor_module_inst_1_627(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1187]),.i2(intermediate_reg_0[1186]),.o(intermediate_reg_1[593])); 
xor_module xor_module_inst_1_628(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1185]),.i2(intermediate_reg_0[1184]),.o(intermediate_reg_1[592])); 
xor_module xor_module_inst_1_629(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1183]),.i2(intermediate_reg_0[1182]),.o(intermediate_reg_1[591])); 
mux_module mux_module_inst_1_630(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1181]),.i2(intermediate_reg_0[1180]),.o(intermediate_reg_1[590]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_631(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1179]),.i2(intermediate_reg_0[1178]),.o(intermediate_reg_1[589]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_632(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1177]),.i2(intermediate_reg_0[1176]),.o(intermediate_reg_1[588]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_633(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1175]),.i2(intermediate_reg_0[1174]),.o(intermediate_reg_1[587]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_634(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1173]),.i2(intermediate_reg_0[1172]),.o(intermediate_reg_1[586]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_635(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1171]),.i2(intermediate_reg_0[1170]),.o(intermediate_reg_1[585]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_636(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1169]),.i2(intermediate_reg_0[1168]),.o(intermediate_reg_1[584])); 
mux_module mux_module_inst_1_637(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1167]),.i2(intermediate_reg_0[1166]),.o(intermediate_reg_1[583]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_638(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1165]),.i2(intermediate_reg_0[1164]),.o(intermediate_reg_1[582]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_639(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1163]),.i2(intermediate_reg_0[1162]),.o(intermediate_reg_1[581])); 
mux_module mux_module_inst_1_640(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1161]),.i2(intermediate_reg_0[1160]),.o(intermediate_reg_1[580]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_641(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1159]),.i2(intermediate_reg_0[1158]),.o(intermediate_reg_1[579]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_642(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1157]),.i2(intermediate_reg_0[1156]),.o(intermediate_reg_1[578])); 
mux_module mux_module_inst_1_643(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1155]),.i2(intermediate_reg_0[1154]),.o(intermediate_reg_1[577]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_644(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1153]),.i2(intermediate_reg_0[1152]),.o(intermediate_reg_1[576])); 
xor_module xor_module_inst_1_645(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1151]),.i2(intermediate_reg_0[1150]),.o(intermediate_reg_1[575])); 
mux_module mux_module_inst_1_646(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1149]),.i2(intermediate_reg_0[1148]),.o(intermediate_reg_1[574]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_647(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1147]),.i2(intermediate_reg_0[1146]),.o(intermediate_reg_1[573])); 
xor_module xor_module_inst_1_648(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1145]),.i2(intermediate_reg_0[1144]),.o(intermediate_reg_1[572])); 
xor_module xor_module_inst_1_649(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1143]),.i2(intermediate_reg_0[1142]),.o(intermediate_reg_1[571])); 
xor_module xor_module_inst_1_650(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1141]),.i2(intermediate_reg_0[1140]),.o(intermediate_reg_1[570])); 
mux_module mux_module_inst_1_651(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1139]),.i2(intermediate_reg_0[1138]),.o(intermediate_reg_1[569]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_652(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1137]),.i2(intermediate_reg_0[1136]),.o(intermediate_reg_1[568]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_653(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1135]),.i2(intermediate_reg_0[1134]),.o(intermediate_reg_1[567])); 
xor_module xor_module_inst_1_654(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1133]),.i2(intermediate_reg_0[1132]),.o(intermediate_reg_1[566])); 
mux_module mux_module_inst_1_655(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1131]),.i2(intermediate_reg_0[1130]),.o(intermediate_reg_1[565]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_656(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1129]),.i2(intermediate_reg_0[1128]),.o(intermediate_reg_1[564])); 
xor_module xor_module_inst_1_657(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1127]),.i2(intermediate_reg_0[1126]),.o(intermediate_reg_1[563])); 
mux_module mux_module_inst_1_658(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1125]),.i2(intermediate_reg_0[1124]),.o(intermediate_reg_1[562]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_659(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1123]),.i2(intermediate_reg_0[1122]),.o(intermediate_reg_1[561])); 
mux_module mux_module_inst_1_660(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1121]),.i2(intermediate_reg_0[1120]),.o(intermediate_reg_1[560]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_661(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1119]),.i2(intermediate_reg_0[1118]),.o(intermediate_reg_1[559]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_662(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1117]),.i2(intermediate_reg_0[1116]),.o(intermediate_reg_1[558]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_663(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1115]),.i2(intermediate_reg_0[1114]),.o(intermediate_reg_1[557]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_664(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1113]),.i2(intermediate_reg_0[1112]),.o(intermediate_reg_1[556])); 
xor_module xor_module_inst_1_665(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1111]),.i2(intermediate_reg_0[1110]),.o(intermediate_reg_1[555])); 
mux_module mux_module_inst_1_666(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1109]),.i2(intermediate_reg_0[1108]),.o(intermediate_reg_1[554]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_667(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1107]),.i2(intermediate_reg_0[1106]),.o(intermediate_reg_1[553]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_668(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1105]),.i2(intermediate_reg_0[1104]),.o(intermediate_reg_1[552]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_669(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1103]),.i2(intermediate_reg_0[1102]),.o(intermediate_reg_1[551])); 
mux_module mux_module_inst_1_670(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1101]),.i2(intermediate_reg_0[1100]),.o(intermediate_reg_1[550]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_671(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1099]),.i2(intermediate_reg_0[1098]),.o(intermediate_reg_1[549])); 
mux_module mux_module_inst_1_672(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1097]),.i2(intermediate_reg_0[1096]),.o(intermediate_reg_1[548]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_673(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1095]),.i2(intermediate_reg_0[1094]),.o(intermediate_reg_1[547]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_674(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1093]),.i2(intermediate_reg_0[1092]),.o(intermediate_reg_1[546]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_675(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1091]),.i2(intermediate_reg_0[1090]),.o(intermediate_reg_1[545]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_676(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1089]),.i2(intermediate_reg_0[1088]),.o(intermediate_reg_1[544]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_677(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1087]),.i2(intermediate_reg_0[1086]),.o(intermediate_reg_1[543]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_678(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1085]),.i2(intermediate_reg_0[1084]),.o(intermediate_reg_1[542]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_679(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1083]),.i2(intermediate_reg_0[1082]),.o(intermediate_reg_1[541]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_680(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1081]),.i2(intermediate_reg_0[1080]),.o(intermediate_reg_1[540]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_681(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1079]),.i2(intermediate_reg_0[1078]),.o(intermediate_reg_1[539])); 
xor_module xor_module_inst_1_682(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1077]),.i2(intermediate_reg_0[1076]),.o(intermediate_reg_1[538])); 
xor_module xor_module_inst_1_683(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1075]),.i2(intermediate_reg_0[1074]),.o(intermediate_reg_1[537])); 
xor_module xor_module_inst_1_684(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1073]),.i2(intermediate_reg_0[1072]),.o(intermediate_reg_1[536])); 
mux_module mux_module_inst_1_685(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1071]),.i2(intermediate_reg_0[1070]),.o(intermediate_reg_1[535]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_686(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1069]),.i2(intermediate_reg_0[1068]),.o(intermediate_reg_1[534])); 
mux_module mux_module_inst_1_687(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1067]),.i2(intermediate_reg_0[1066]),.o(intermediate_reg_1[533]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_688(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1065]),.i2(intermediate_reg_0[1064]),.o(intermediate_reg_1[532])); 
mux_module mux_module_inst_1_689(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1063]),.i2(intermediate_reg_0[1062]),.o(intermediate_reg_1[531]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_690(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1061]),.i2(intermediate_reg_0[1060]),.o(intermediate_reg_1[530]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_691(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1059]),.i2(intermediate_reg_0[1058]),.o(intermediate_reg_1[529]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_692(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1057]),.i2(intermediate_reg_0[1056]),.o(intermediate_reg_1[528])); 
mux_module mux_module_inst_1_693(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1055]),.i2(intermediate_reg_0[1054]),.o(intermediate_reg_1[527]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_694(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1053]),.i2(intermediate_reg_0[1052]),.o(intermediate_reg_1[526])); 
xor_module xor_module_inst_1_695(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1051]),.i2(intermediate_reg_0[1050]),.o(intermediate_reg_1[525])); 
xor_module xor_module_inst_1_696(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1049]),.i2(intermediate_reg_0[1048]),.o(intermediate_reg_1[524])); 
mux_module mux_module_inst_1_697(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1047]),.i2(intermediate_reg_0[1046]),.o(intermediate_reg_1[523]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_698(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1045]),.i2(intermediate_reg_0[1044]),.o(intermediate_reg_1[522]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_699(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1043]),.i2(intermediate_reg_0[1042]),.o(intermediate_reg_1[521]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_700(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1041]),.i2(intermediate_reg_0[1040]),.o(intermediate_reg_1[520]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_701(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1039]),.i2(intermediate_reg_0[1038]),.o(intermediate_reg_1[519])); 
mux_module mux_module_inst_1_702(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1037]),.i2(intermediate_reg_0[1036]),.o(intermediate_reg_1[518]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_703(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1035]),.i2(intermediate_reg_0[1034]),.o(intermediate_reg_1[517]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_704(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1033]),.i2(intermediate_reg_0[1032]),.o(intermediate_reg_1[516])); 
mux_module mux_module_inst_1_705(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1031]),.i2(intermediate_reg_0[1030]),.o(intermediate_reg_1[515]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_706(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1029]),.i2(intermediate_reg_0[1028]),.o(intermediate_reg_1[514])); 
xor_module xor_module_inst_1_707(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1027]),.i2(intermediate_reg_0[1026]),.o(intermediate_reg_1[513])); 
xor_module xor_module_inst_1_708(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1025]),.i2(intermediate_reg_0[1024]),.o(intermediate_reg_1[512])); 
xor_module xor_module_inst_1_709(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1023]),.i2(intermediate_reg_0[1022]),.o(intermediate_reg_1[511])); 
mux_module mux_module_inst_1_710(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1021]),.i2(intermediate_reg_0[1020]),.o(intermediate_reg_1[510]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_711(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1019]),.i2(intermediate_reg_0[1018]),.o(intermediate_reg_1[509]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_712(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1017]),.i2(intermediate_reg_0[1016]),.o(intermediate_reg_1[508]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_713(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1015]),.i2(intermediate_reg_0[1014]),.o(intermediate_reg_1[507])); 
xor_module xor_module_inst_1_714(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1013]),.i2(intermediate_reg_0[1012]),.o(intermediate_reg_1[506])); 
xor_module xor_module_inst_1_715(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1011]),.i2(intermediate_reg_0[1010]),.o(intermediate_reg_1[505])); 
xor_module xor_module_inst_1_716(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1009]),.i2(intermediate_reg_0[1008]),.o(intermediate_reg_1[504])); 
mux_module mux_module_inst_1_717(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1007]),.i2(intermediate_reg_0[1006]),.o(intermediate_reg_1[503]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_718(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1005]),.i2(intermediate_reg_0[1004]),.o(intermediate_reg_1[502]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_719(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1003]),.i2(intermediate_reg_0[1002]),.o(intermediate_reg_1[501]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_720(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1001]),.i2(intermediate_reg_0[1000]),.o(intermediate_reg_1[500]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_721(.clk(clk),.reset(reset),.i1(intermediate_reg_0[999]),.i2(intermediate_reg_0[998]),.o(intermediate_reg_1[499]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_722(.clk(clk),.reset(reset),.i1(intermediate_reg_0[997]),.i2(intermediate_reg_0[996]),.o(intermediate_reg_1[498]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_723(.clk(clk),.reset(reset),.i1(intermediate_reg_0[995]),.i2(intermediate_reg_0[994]),.o(intermediate_reg_1[497]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_724(.clk(clk),.reset(reset),.i1(intermediate_reg_0[993]),.i2(intermediate_reg_0[992]),.o(intermediate_reg_1[496])); 
mux_module mux_module_inst_1_725(.clk(clk),.reset(reset),.i1(intermediate_reg_0[991]),.i2(intermediate_reg_0[990]),.o(intermediate_reg_1[495]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_726(.clk(clk),.reset(reset),.i1(intermediate_reg_0[989]),.i2(intermediate_reg_0[988]),.o(intermediate_reg_1[494]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_727(.clk(clk),.reset(reset),.i1(intermediate_reg_0[987]),.i2(intermediate_reg_0[986]),.o(intermediate_reg_1[493]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_728(.clk(clk),.reset(reset),.i1(intermediate_reg_0[985]),.i2(intermediate_reg_0[984]),.o(intermediate_reg_1[492])); 
xor_module xor_module_inst_1_729(.clk(clk),.reset(reset),.i1(intermediate_reg_0[983]),.i2(intermediate_reg_0[982]),.o(intermediate_reg_1[491])); 
mux_module mux_module_inst_1_730(.clk(clk),.reset(reset),.i1(intermediate_reg_0[981]),.i2(intermediate_reg_0[980]),.o(intermediate_reg_1[490]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_731(.clk(clk),.reset(reset),.i1(intermediate_reg_0[979]),.i2(intermediate_reg_0[978]),.o(intermediate_reg_1[489])); 
xor_module xor_module_inst_1_732(.clk(clk),.reset(reset),.i1(intermediate_reg_0[977]),.i2(intermediate_reg_0[976]),.o(intermediate_reg_1[488])); 
mux_module mux_module_inst_1_733(.clk(clk),.reset(reset),.i1(intermediate_reg_0[975]),.i2(intermediate_reg_0[974]),.o(intermediate_reg_1[487]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_734(.clk(clk),.reset(reset),.i1(intermediate_reg_0[973]),.i2(intermediate_reg_0[972]),.o(intermediate_reg_1[486]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_735(.clk(clk),.reset(reset),.i1(intermediate_reg_0[971]),.i2(intermediate_reg_0[970]),.o(intermediate_reg_1[485])); 
xor_module xor_module_inst_1_736(.clk(clk),.reset(reset),.i1(intermediate_reg_0[969]),.i2(intermediate_reg_0[968]),.o(intermediate_reg_1[484])); 
xor_module xor_module_inst_1_737(.clk(clk),.reset(reset),.i1(intermediate_reg_0[967]),.i2(intermediate_reg_0[966]),.o(intermediate_reg_1[483])); 
xor_module xor_module_inst_1_738(.clk(clk),.reset(reset),.i1(intermediate_reg_0[965]),.i2(intermediate_reg_0[964]),.o(intermediate_reg_1[482])); 
mux_module mux_module_inst_1_739(.clk(clk),.reset(reset),.i1(intermediate_reg_0[963]),.i2(intermediate_reg_0[962]),.o(intermediate_reg_1[481]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_740(.clk(clk),.reset(reset),.i1(intermediate_reg_0[961]),.i2(intermediate_reg_0[960]),.o(intermediate_reg_1[480]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_741(.clk(clk),.reset(reset),.i1(intermediate_reg_0[959]),.i2(intermediate_reg_0[958]),.o(intermediate_reg_1[479])); 
mux_module mux_module_inst_1_742(.clk(clk),.reset(reset),.i1(intermediate_reg_0[957]),.i2(intermediate_reg_0[956]),.o(intermediate_reg_1[478]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_743(.clk(clk),.reset(reset),.i1(intermediate_reg_0[955]),.i2(intermediate_reg_0[954]),.o(intermediate_reg_1[477]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_744(.clk(clk),.reset(reset),.i1(intermediate_reg_0[953]),.i2(intermediate_reg_0[952]),.o(intermediate_reg_1[476]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_745(.clk(clk),.reset(reset),.i1(intermediate_reg_0[951]),.i2(intermediate_reg_0[950]),.o(intermediate_reg_1[475]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_746(.clk(clk),.reset(reset),.i1(intermediate_reg_0[949]),.i2(intermediate_reg_0[948]),.o(intermediate_reg_1[474])); 
xor_module xor_module_inst_1_747(.clk(clk),.reset(reset),.i1(intermediate_reg_0[947]),.i2(intermediate_reg_0[946]),.o(intermediate_reg_1[473])); 
xor_module xor_module_inst_1_748(.clk(clk),.reset(reset),.i1(intermediate_reg_0[945]),.i2(intermediate_reg_0[944]),.o(intermediate_reg_1[472])); 
mux_module mux_module_inst_1_749(.clk(clk),.reset(reset),.i1(intermediate_reg_0[943]),.i2(intermediate_reg_0[942]),.o(intermediate_reg_1[471]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_750(.clk(clk),.reset(reset),.i1(intermediate_reg_0[941]),.i2(intermediate_reg_0[940]),.o(intermediate_reg_1[470]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_751(.clk(clk),.reset(reset),.i1(intermediate_reg_0[939]),.i2(intermediate_reg_0[938]),.o(intermediate_reg_1[469])); 
xor_module xor_module_inst_1_752(.clk(clk),.reset(reset),.i1(intermediate_reg_0[937]),.i2(intermediate_reg_0[936]),.o(intermediate_reg_1[468])); 
mux_module mux_module_inst_1_753(.clk(clk),.reset(reset),.i1(intermediate_reg_0[935]),.i2(intermediate_reg_0[934]),.o(intermediate_reg_1[467]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_754(.clk(clk),.reset(reset),.i1(intermediate_reg_0[933]),.i2(intermediate_reg_0[932]),.o(intermediate_reg_1[466])); 
xor_module xor_module_inst_1_755(.clk(clk),.reset(reset),.i1(intermediate_reg_0[931]),.i2(intermediate_reg_0[930]),.o(intermediate_reg_1[465])); 
xor_module xor_module_inst_1_756(.clk(clk),.reset(reset),.i1(intermediate_reg_0[929]),.i2(intermediate_reg_0[928]),.o(intermediate_reg_1[464])); 
xor_module xor_module_inst_1_757(.clk(clk),.reset(reset),.i1(intermediate_reg_0[927]),.i2(intermediate_reg_0[926]),.o(intermediate_reg_1[463])); 
xor_module xor_module_inst_1_758(.clk(clk),.reset(reset),.i1(intermediate_reg_0[925]),.i2(intermediate_reg_0[924]),.o(intermediate_reg_1[462])); 
mux_module mux_module_inst_1_759(.clk(clk),.reset(reset),.i1(intermediate_reg_0[923]),.i2(intermediate_reg_0[922]),.o(intermediate_reg_1[461]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_760(.clk(clk),.reset(reset),.i1(intermediate_reg_0[921]),.i2(intermediate_reg_0[920]),.o(intermediate_reg_1[460]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_761(.clk(clk),.reset(reset),.i1(intermediate_reg_0[919]),.i2(intermediate_reg_0[918]),.o(intermediate_reg_1[459])); 
xor_module xor_module_inst_1_762(.clk(clk),.reset(reset),.i1(intermediate_reg_0[917]),.i2(intermediate_reg_0[916]),.o(intermediate_reg_1[458])); 
mux_module mux_module_inst_1_763(.clk(clk),.reset(reset),.i1(intermediate_reg_0[915]),.i2(intermediate_reg_0[914]),.o(intermediate_reg_1[457]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_764(.clk(clk),.reset(reset),.i1(intermediate_reg_0[913]),.i2(intermediate_reg_0[912]),.o(intermediate_reg_1[456]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_765(.clk(clk),.reset(reset),.i1(intermediate_reg_0[911]),.i2(intermediate_reg_0[910]),.o(intermediate_reg_1[455])); 
xor_module xor_module_inst_1_766(.clk(clk),.reset(reset),.i1(intermediate_reg_0[909]),.i2(intermediate_reg_0[908]),.o(intermediate_reg_1[454])); 
xor_module xor_module_inst_1_767(.clk(clk),.reset(reset),.i1(intermediate_reg_0[907]),.i2(intermediate_reg_0[906]),.o(intermediate_reg_1[453])); 
mux_module mux_module_inst_1_768(.clk(clk),.reset(reset),.i1(intermediate_reg_0[905]),.i2(intermediate_reg_0[904]),.o(intermediate_reg_1[452]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_769(.clk(clk),.reset(reset),.i1(intermediate_reg_0[903]),.i2(intermediate_reg_0[902]),.o(intermediate_reg_1[451])); 
mux_module mux_module_inst_1_770(.clk(clk),.reset(reset),.i1(intermediate_reg_0[901]),.i2(intermediate_reg_0[900]),.o(intermediate_reg_1[450]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_771(.clk(clk),.reset(reset),.i1(intermediate_reg_0[899]),.i2(intermediate_reg_0[898]),.o(intermediate_reg_1[449]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_772(.clk(clk),.reset(reset),.i1(intermediate_reg_0[897]),.i2(intermediate_reg_0[896]),.o(intermediate_reg_1[448]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_773(.clk(clk),.reset(reset),.i1(intermediate_reg_0[895]),.i2(intermediate_reg_0[894]),.o(intermediate_reg_1[447])); 
mux_module mux_module_inst_1_774(.clk(clk),.reset(reset),.i1(intermediate_reg_0[893]),.i2(intermediate_reg_0[892]),.o(intermediate_reg_1[446]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_775(.clk(clk),.reset(reset),.i1(intermediate_reg_0[891]),.i2(intermediate_reg_0[890]),.o(intermediate_reg_1[445])); 
xor_module xor_module_inst_1_776(.clk(clk),.reset(reset),.i1(intermediate_reg_0[889]),.i2(intermediate_reg_0[888]),.o(intermediate_reg_1[444])); 
mux_module mux_module_inst_1_777(.clk(clk),.reset(reset),.i1(intermediate_reg_0[887]),.i2(intermediate_reg_0[886]),.o(intermediate_reg_1[443]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_778(.clk(clk),.reset(reset),.i1(intermediate_reg_0[885]),.i2(intermediate_reg_0[884]),.o(intermediate_reg_1[442]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_779(.clk(clk),.reset(reset),.i1(intermediate_reg_0[883]),.i2(intermediate_reg_0[882]),.o(intermediate_reg_1[441]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_780(.clk(clk),.reset(reset),.i1(intermediate_reg_0[881]),.i2(intermediate_reg_0[880]),.o(intermediate_reg_1[440]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_781(.clk(clk),.reset(reset),.i1(intermediate_reg_0[879]),.i2(intermediate_reg_0[878]),.o(intermediate_reg_1[439])); 
mux_module mux_module_inst_1_782(.clk(clk),.reset(reset),.i1(intermediate_reg_0[877]),.i2(intermediate_reg_0[876]),.o(intermediate_reg_1[438]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_783(.clk(clk),.reset(reset),.i1(intermediate_reg_0[875]),.i2(intermediate_reg_0[874]),.o(intermediate_reg_1[437])); 
xor_module xor_module_inst_1_784(.clk(clk),.reset(reset),.i1(intermediate_reg_0[873]),.i2(intermediate_reg_0[872]),.o(intermediate_reg_1[436])); 
xor_module xor_module_inst_1_785(.clk(clk),.reset(reset),.i1(intermediate_reg_0[871]),.i2(intermediate_reg_0[870]),.o(intermediate_reg_1[435])); 
mux_module mux_module_inst_1_786(.clk(clk),.reset(reset),.i1(intermediate_reg_0[869]),.i2(intermediate_reg_0[868]),.o(intermediate_reg_1[434]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_787(.clk(clk),.reset(reset),.i1(intermediate_reg_0[867]),.i2(intermediate_reg_0[866]),.o(intermediate_reg_1[433])); 
mux_module mux_module_inst_1_788(.clk(clk),.reset(reset),.i1(intermediate_reg_0[865]),.i2(intermediate_reg_0[864]),.o(intermediate_reg_1[432]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_789(.clk(clk),.reset(reset),.i1(intermediate_reg_0[863]),.i2(intermediate_reg_0[862]),.o(intermediate_reg_1[431]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_790(.clk(clk),.reset(reset),.i1(intermediate_reg_0[861]),.i2(intermediate_reg_0[860]),.o(intermediate_reg_1[430]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_791(.clk(clk),.reset(reset),.i1(intermediate_reg_0[859]),.i2(intermediate_reg_0[858]),.o(intermediate_reg_1[429]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_792(.clk(clk),.reset(reset),.i1(intermediate_reg_0[857]),.i2(intermediate_reg_0[856]),.o(intermediate_reg_1[428]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_793(.clk(clk),.reset(reset),.i1(intermediate_reg_0[855]),.i2(intermediate_reg_0[854]),.o(intermediate_reg_1[427])); 
xor_module xor_module_inst_1_794(.clk(clk),.reset(reset),.i1(intermediate_reg_0[853]),.i2(intermediate_reg_0[852]),.o(intermediate_reg_1[426])); 
xor_module xor_module_inst_1_795(.clk(clk),.reset(reset),.i1(intermediate_reg_0[851]),.i2(intermediate_reg_0[850]),.o(intermediate_reg_1[425])); 
mux_module mux_module_inst_1_796(.clk(clk),.reset(reset),.i1(intermediate_reg_0[849]),.i2(intermediate_reg_0[848]),.o(intermediate_reg_1[424]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_797(.clk(clk),.reset(reset),.i1(intermediate_reg_0[847]),.i2(intermediate_reg_0[846]),.o(intermediate_reg_1[423]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_798(.clk(clk),.reset(reset),.i1(intermediate_reg_0[845]),.i2(intermediate_reg_0[844]),.o(intermediate_reg_1[422]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_799(.clk(clk),.reset(reset),.i1(intermediate_reg_0[843]),.i2(intermediate_reg_0[842]),.o(intermediate_reg_1[421]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_800(.clk(clk),.reset(reset),.i1(intermediate_reg_0[841]),.i2(intermediate_reg_0[840]),.o(intermediate_reg_1[420]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_801(.clk(clk),.reset(reset),.i1(intermediate_reg_0[839]),.i2(intermediate_reg_0[838]),.o(intermediate_reg_1[419])); 
xor_module xor_module_inst_1_802(.clk(clk),.reset(reset),.i1(intermediate_reg_0[837]),.i2(intermediate_reg_0[836]),.o(intermediate_reg_1[418])); 
mux_module mux_module_inst_1_803(.clk(clk),.reset(reset),.i1(intermediate_reg_0[835]),.i2(intermediate_reg_0[834]),.o(intermediate_reg_1[417]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_804(.clk(clk),.reset(reset),.i1(intermediate_reg_0[833]),.i2(intermediate_reg_0[832]),.o(intermediate_reg_1[416]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_805(.clk(clk),.reset(reset),.i1(intermediate_reg_0[831]),.i2(intermediate_reg_0[830]),.o(intermediate_reg_1[415]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_806(.clk(clk),.reset(reset),.i1(intermediate_reg_0[829]),.i2(intermediate_reg_0[828]),.o(intermediate_reg_1[414]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_807(.clk(clk),.reset(reset),.i1(intermediate_reg_0[827]),.i2(intermediate_reg_0[826]),.o(intermediate_reg_1[413])); 
mux_module mux_module_inst_1_808(.clk(clk),.reset(reset),.i1(intermediate_reg_0[825]),.i2(intermediate_reg_0[824]),.o(intermediate_reg_1[412]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_809(.clk(clk),.reset(reset),.i1(intermediate_reg_0[823]),.i2(intermediate_reg_0[822]),.o(intermediate_reg_1[411]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_810(.clk(clk),.reset(reset),.i1(intermediate_reg_0[821]),.i2(intermediate_reg_0[820]),.o(intermediate_reg_1[410])); 
xor_module xor_module_inst_1_811(.clk(clk),.reset(reset),.i1(intermediate_reg_0[819]),.i2(intermediate_reg_0[818]),.o(intermediate_reg_1[409])); 
mux_module mux_module_inst_1_812(.clk(clk),.reset(reset),.i1(intermediate_reg_0[817]),.i2(intermediate_reg_0[816]),.o(intermediate_reg_1[408]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_813(.clk(clk),.reset(reset),.i1(intermediate_reg_0[815]),.i2(intermediate_reg_0[814]),.o(intermediate_reg_1[407])); 
xor_module xor_module_inst_1_814(.clk(clk),.reset(reset),.i1(intermediate_reg_0[813]),.i2(intermediate_reg_0[812]),.o(intermediate_reg_1[406])); 
xor_module xor_module_inst_1_815(.clk(clk),.reset(reset),.i1(intermediate_reg_0[811]),.i2(intermediate_reg_0[810]),.o(intermediate_reg_1[405])); 
xor_module xor_module_inst_1_816(.clk(clk),.reset(reset),.i1(intermediate_reg_0[809]),.i2(intermediate_reg_0[808]),.o(intermediate_reg_1[404])); 
mux_module mux_module_inst_1_817(.clk(clk),.reset(reset),.i1(intermediate_reg_0[807]),.i2(intermediate_reg_0[806]),.o(intermediate_reg_1[403]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_818(.clk(clk),.reset(reset),.i1(intermediate_reg_0[805]),.i2(intermediate_reg_0[804]),.o(intermediate_reg_1[402])); 
xor_module xor_module_inst_1_819(.clk(clk),.reset(reset),.i1(intermediate_reg_0[803]),.i2(intermediate_reg_0[802]),.o(intermediate_reg_1[401])); 
xor_module xor_module_inst_1_820(.clk(clk),.reset(reset),.i1(intermediate_reg_0[801]),.i2(intermediate_reg_0[800]),.o(intermediate_reg_1[400])); 
mux_module mux_module_inst_1_821(.clk(clk),.reset(reset),.i1(intermediate_reg_0[799]),.i2(intermediate_reg_0[798]),.o(intermediate_reg_1[399]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_822(.clk(clk),.reset(reset),.i1(intermediate_reg_0[797]),.i2(intermediate_reg_0[796]),.o(intermediate_reg_1[398]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_823(.clk(clk),.reset(reset),.i1(intermediate_reg_0[795]),.i2(intermediate_reg_0[794]),.o(intermediate_reg_1[397])); 
mux_module mux_module_inst_1_824(.clk(clk),.reset(reset),.i1(intermediate_reg_0[793]),.i2(intermediate_reg_0[792]),.o(intermediate_reg_1[396]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_825(.clk(clk),.reset(reset),.i1(intermediate_reg_0[791]),.i2(intermediate_reg_0[790]),.o(intermediate_reg_1[395]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_826(.clk(clk),.reset(reset),.i1(intermediate_reg_0[789]),.i2(intermediate_reg_0[788]),.o(intermediate_reg_1[394])); 
mux_module mux_module_inst_1_827(.clk(clk),.reset(reset),.i1(intermediate_reg_0[787]),.i2(intermediate_reg_0[786]),.o(intermediate_reg_1[393]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_828(.clk(clk),.reset(reset),.i1(intermediate_reg_0[785]),.i2(intermediate_reg_0[784]),.o(intermediate_reg_1[392]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_829(.clk(clk),.reset(reset),.i1(intermediate_reg_0[783]),.i2(intermediate_reg_0[782]),.o(intermediate_reg_1[391]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_830(.clk(clk),.reset(reset),.i1(intermediate_reg_0[781]),.i2(intermediate_reg_0[780]),.o(intermediate_reg_1[390]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_831(.clk(clk),.reset(reset),.i1(intermediate_reg_0[779]),.i2(intermediate_reg_0[778]),.o(intermediate_reg_1[389])); 
mux_module mux_module_inst_1_832(.clk(clk),.reset(reset),.i1(intermediate_reg_0[777]),.i2(intermediate_reg_0[776]),.o(intermediate_reg_1[388]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_833(.clk(clk),.reset(reset),.i1(intermediate_reg_0[775]),.i2(intermediate_reg_0[774]),.o(intermediate_reg_1[387])); 
mux_module mux_module_inst_1_834(.clk(clk),.reset(reset),.i1(intermediate_reg_0[773]),.i2(intermediate_reg_0[772]),.o(intermediate_reg_1[386]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_835(.clk(clk),.reset(reset),.i1(intermediate_reg_0[771]),.i2(intermediate_reg_0[770]),.o(intermediate_reg_1[385]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_836(.clk(clk),.reset(reset),.i1(intermediate_reg_0[769]),.i2(intermediate_reg_0[768]),.o(intermediate_reg_1[384]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_837(.clk(clk),.reset(reset),.i1(intermediate_reg_0[767]),.i2(intermediate_reg_0[766]),.o(intermediate_reg_1[383])); 
mux_module mux_module_inst_1_838(.clk(clk),.reset(reset),.i1(intermediate_reg_0[765]),.i2(intermediate_reg_0[764]),.o(intermediate_reg_1[382]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_839(.clk(clk),.reset(reset),.i1(intermediate_reg_0[763]),.i2(intermediate_reg_0[762]),.o(intermediate_reg_1[381]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_840(.clk(clk),.reset(reset),.i1(intermediate_reg_0[761]),.i2(intermediate_reg_0[760]),.o(intermediate_reg_1[380])); 
xor_module xor_module_inst_1_841(.clk(clk),.reset(reset),.i1(intermediate_reg_0[759]),.i2(intermediate_reg_0[758]),.o(intermediate_reg_1[379])); 
mux_module mux_module_inst_1_842(.clk(clk),.reset(reset),.i1(intermediate_reg_0[757]),.i2(intermediate_reg_0[756]),.o(intermediate_reg_1[378]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_843(.clk(clk),.reset(reset),.i1(intermediate_reg_0[755]),.i2(intermediate_reg_0[754]),.o(intermediate_reg_1[377]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_844(.clk(clk),.reset(reset),.i1(intermediate_reg_0[753]),.i2(intermediate_reg_0[752]),.o(intermediate_reg_1[376]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_845(.clk(clk),.reset(reset),.i1(intermediate_reg_0[751]),.i2(intermediate_reg_0[750]),.o(intermediate_reg_1[375])); 
xor_module xor_module_inst_1_846(.clk(clk),.reset(reset),.i1(intermediate_reg_0[749]),.i2(intermediate_reg_0[748]),.o(intermediate_reg_1[374])); 
xor_module xor_module_inst_1_847(.clk(clk),.reset(reset),.i1(intermediate_reg_0[747]),.i2(intermediate_reg_0[746]),.o(intermediate_reg_1[373])); 
xor_module xor_module_inst_1_848(.clk(clk),.reset(reset),.i1(intermediate_reg_0[745]),.i2(intermediate_reg_0[744]),.o(intermediate_reg_1[372])); 
xor_module xor_module_inst_1_849(.clk(clk),.reset(reset),.i1(intermediate_reg_0[743]),.i2(intermediate_reg_0[742]),.o(intermediate_reg_1[371])); 
mux_module mux_module_inst_1_850(.clk(clk),.reset(reset),.i1(intermediate_reg_0[741]),.i2(intermediate_reg_0[740]),.o(intermediate_reg_1[370]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_851(.clk(clk),.reset(reset),.i1(intermediate_reg_0[739]),.i2(intermediate_reg_0[738]),.o(intermediate_reg_1[369])); 
xor_module xor_module_inst_1_852(.clk(clk),.reset(reset),.i1(intermediate_reg_0[737]),.i2(intermediate_reg_0[736]),.o(intermediate_reg_1[368])); 
xor_module xor_module_inst_1_853(.clk(clk),.reset(reset),.i1(intermediate_reg_0[735]),.i2(intermediate_reg_0[734]),.o(intermediate_reg_1[367])); 
mux_module mux_module_inst_1_854(.clk(clk),.reset(reset),.i1(intermediate_reg_0[733]),.i2(intermediate_reg_0[732]),.o(intermediate_reg_1[366]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_855(.clk(clk),.reset(reset),.i1(intermediate_reg_0[731]),.i2(intermediate_reg_0[730]),.o(intermediate_reg_1[365])); 
xor_module xor_module_inst_1_856(.clk(clk),.reset(reset),.i1(intermediate_reg_0[729]),.i2(intermediate_reg_0[728]),.o(intermediate_reg_1[364])); 
xor_module xor_module_inst_1_857(.clk(clk),.reset(reset),.i1(intermediate_reg_0[727]),.i2(intermediate_reg_0[726]),.o(intermediate_reg_1[363])); 
mux_module mux_module_inst_1_858(.clk(clk),.reset(reset),.i1(intermediate_reg_0[725]),.i2(intermediate_reg_0[724]),.o(intermediate_reg_1[362]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_859(.clk(clk),.reset(reset),.i1(intermediate_reg_0[723]),.i2(intermediate_reg_0[722]),.o(intermediate_reg_1[361]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_860(.clk(clk),.reset(reset),.i1(intermediate_reg_0[721]),.i2(intermediate_reg_0[720]),.o(intermediate_reg_1[360])); 
xor_module xor_module_inst_1_861(.clk(clk),.reset(reset),.i1(intermediate_reg_0[719]),.i2(intermediate_reg_0[718]),.o(intermediate_reg_1[359])); 
xor_module xor_module_inst_1_862(.clk(clk),.reset(reset),.i1(intermediate_reg_0[717]),.i2(intermediate_reg_0[716]),.o(intermediate_reg_1[358])); 
xor_module xor_module_inst_1_863(.clk(clk),.reset(reset),.i1(intermediate_reg_0[715]),.i2(intermediate_reg_0[714]),.o(intermediate_reg_1[357])); 
xor_module xor_module_inst_1_864(.clk(clk),.reset(reset),.i1(intermediate_reg_0[713]),.i2(intermediate_reg_0[712]),.o(intermediate_reg_1[356])); 
xor_module xor_module_inst_1_865(.clk(clk),.reset(reset),.i1(intermediate_reg_0[711]),.i2(intermediate_reg_0[710]),.o(intermediate_reg_1[355])); 
xor_module xor_module_inst_1_866(.clk(clk),.reset(reset),.i1(intermediate_reg_0[709]),.i2(intermediate_reg_0[708]),.o(intermediate_reg_1[354])); 
xor_module xor_module_inst_1_867(.clk(clk),.reset(reset),.i1(intermediate_reg_0[707]),.i2(intermediate_reg_0[706]),.o(intermediate_reg_1[353])); 
mux_module mux_module_inst_1_868(.clk(clk),.reset(reset),.i1(intermediate_reg_0[705]),.i2(intermediate_reg_0[704]),.o(intermediate_reg_1[352]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_869(.clk(clk),.reset(reset),.i1(intermediate_reg_0[703]),.i2(intermediate_reg_0[702]),.o(intermediate_reg_1[351])); 
xor_module xor_module_inst_1_870(.clk(clk),.reset(reset),.i1(intermediate_reg_0[701]),.i2(intermediate_reg_0[700]),.o(intermediate_reg_1[350])); 
xor_module xor_module_inst_1_871(.clk(clk),.reset(reset),.i1(intermediate_reg_0[699]),.i2(intermediate_reg_0[698]),.o(intermediate_reg_1[349])); 
mux_module mux_module_inst_1_872(.clk(clk),.reset(reset),.i1(intermediate_reg_0[697]),.i2(intermediate_reg_0[696]),.o(intermediate_reg_1[348]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_873(.clk(clk),.reset(reset),.i1(intermediate_reg_0[695]),.i2(intermediate_reg_0[694]),.o(intermediate_reg_1[347])); 
mux_module mux_module_inst_1_874(.clk(clk),.reset(reset),.i1(intermediate_reg_0[693]),.i2(intermediate_reg_0[692]),.o(intermediate_reg_1[346]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_875(.clk(clk),.reset(reset),.i1(intermediate_reg_0[691]),.i2(intermediate_reg_0[690]),.o(intermediate_reg_1[345]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_876(.clk(clk),.reset(reset),.i1(intermediate_reg_0[689]),.i2(intermediate_reg_0[688]),.o(intermediate_reg_1[344])); 
mux_module mux_module_inst_1_877(.clk(clk),.reset(reset),.i1(intermediate_reg_0[687]),.i2(intermediate_reg_0[686]),.o(intermediate_reg_1[343]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_878(.clk(clk),.reset(reset),.i1(intermediate_reg_0[685]),.i2(intermediate_reg_0[684]),.o(intermediate_reg_1[342])); 
xor_module xor_module_inst_1_879(.clk(clk),.reset(reset),.i1(intermediate_reg_0[683]),.i2(intermediate_reg_0[682]),.o(intermediate_reg_1[341])); 
xor_module xor_module_inst_1_880(.clk(clk),.reset(reset),.i1(intermediate_reg_0[681]),.i2(intermediate_reg_0[680]),.o(intermediate_reg_1[340])); 
xor_module xor_module_inst_1_881(.clk(clk),.reset(reset),.i1(intermediate_reg_0[679]),.i2(intermediate_reg_0[678]),.o(intermediate_reg_1[339])); 
xor_module xor_module_inst_1_882(.clk(clk),.reset(reset),.i1(intermediate_reg_0[677]),.i2(intermediate_reg_0[676]),.o(intermediate_reg_1[338])); 
xor_module xor_module_inst_1_883(.clk(clk),.reset(reset),.i1(intermediate_reg_0[675]),.i2(intermediate_reg_0[674]),.o(intermediate_reg_1[337])); 
xor_module xor_module_inst_1_884(.clk(clk),.reset(reset),.i1(intermediate_reg_0[673]),.i2(intermediate_reg_0[672]),.o(intermediate_reg_1[336])); 
mux_module mux_module_inst_1_885(.clk(clk),.reset(reset),.i1(intermediate_reg_0[671]),.i2(intermediate_reg_0[670]),.o(intermediate_reg_1[335]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_886(.clk(clk),.reset(reset),.i1(intermediate_reg_0[669]),.i2(intermediate_reg_0[668]),.o(intermediate_reg_1[334])); 
mux_module mux_module_inst_1_887(.clk(clk),.reset(reset),.i1(intermediate_reg_0[667]),.i2(intermediate_reg_0[666]),.o(intermediate_reg_1[333]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_888(.clk(clk),.reset(reset),.i1(intermediate_reg_0[665]),.i2(intermediate_reg_0[664]),.o(intermediate_reg_1[332]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_889(.clk(clk),.reset(reset),.i1(intermediate_reg_0[663]),.i2(intermediate_reg_0[662]),.o(intermediate_reg_1[331])); 
xor_module xor_module_inst_1_890(.clk(clk),.reset(reset),.i1(intermediate_reg_0[661]),.i2(intermediate_reg_0[660]),.o(intermediate_reg_1[330])); 
mux_module mux_module_inst_1_891(.clk(clk),.reset(reset),.i1(intermediate_reg_0[659]),.i2(intermediate_reg_0[658]),.o(intermediate_reg_1[329]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_892(.clk(clk),.reset(reset),.i1(intermediate_reg_0[657]),.i2(intermediate_reg_0[656]),.o(intermediate_reg_1[328])); 
xor_module xor_module_inst_1_893(.clk(clk),.reset(reset),.i1(intermediate_reg_0[655]),.i2(intermediate_reg_0[654]),.o(intermediate_reg_1[327])); 
mux_module mux_module_inst_1_894(.clk(clk),.reset(reset),.i1(intermediate_reg_0[653]),.i2(intermediate_reg_0[652]),.o(intermediate_reg_1[326]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_895(.clk(clk),.reset(reset),.i1(intermediate_reg_0[651]),.i2(intermediate_reg_0[650]),.o(intermediate_reg_1[325])); 
xor_module xor_module_inst_1_896(.clk(clk),.reset(reset),.i1(intermediate_reg_0[649]),.i2(intermediate_reg_0[648]),.o(intermediate_reg_1[324])); 
mux_module mux_module_inst_1_897(.clk(clk),.reset(reset),.i1(intermediate_reg_0[647]),.i2(intermediate_reg_0[646]),.o(intermediate_reg_1[323]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_898(.clk(clk),.reset(reset),.i1(intermediate_reg_0[645]),.i2(intermediate_reg_0[644]),.o(intermediate_reg_1[322])); 
mux_module mux_module_inst_1_899(.clk(clk),.reset(reset),.i1(intermediate_reg_0[643]),.i2(intermediate_reg_0[642]),.o(intermediate_reg_1[321]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_900(.clk(clk),.reset(reset),.i1(intermediate_reg_0[641]),.i2(intermediate_reg_0[640]),.o(intermediate_reg_1[320]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_901(.clk(clk),.reset(reset),.i1(intermediate_reg_0[639]),.i2(intermediate_reg_0[638]),.o(intermediate_reg_1[319]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_902(.clk(clk),.reset(reset),.i1(intermediate_reg_0[637]),.i2(intermediate_reg_0[636]),.o(intermediate_reg_1[318]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_903(.clk(clk),.reset(reset),.i1(intermediate_reg_0[635]),.i2(intermediate_reg_0[634]),.o(intermediate_reg_1[317]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_904(.clk(clk),.reset(reset),.i1(intermediate_reg_0[633]),.i2(intermediate_reg_0[632]),.o(intermediate_reg_1[316]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_905(.clk(clk),.reset(reset),.i1(intermediate_reg_0[631]),.i2(intermediate_reg_0[630]),.o(intermediate_reg_1[315]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_906(.clk(clk),.reset(reset),.i1(intermediate_reg_0[629]),.i2(intermediate_reg_0[628]),.o(intermediate_reg_1[314]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_907(.clk(clk),.reset(reset),.i1(intermediate_reg_0[627]),.i2(intermediate_reg_0[626]),.o(intermediate_reg_1[313]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_908(.clk(clk),.reset(reset),.i1(intermediate_reg_0[625]),.i2(intermediate_reg_0[624]),.o(intermediate_reg_1[312])); 
mux_module mux_module_inst_1_909(.clk(clk),.reset(reset),.i1(intermediate_reg_0[623]),.i2(intermediate_reg_0[622]),.o(intermediate_reg_1[311]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_910(.clk(clk),.reset(reset),.i1(intermediate_reg_0[621]),.i2(intermediate_reg_0[620]),.o(intermediate_reg_1[310])); 
xor_module xor_module_inst_1_911(.clk(clk),.reset(reset),.i1(intermediate_reg_0[619]),.i2(intermediate_reg_0[618]),.o(intermediate_reg_1[309])); 
xor_module xor_module_inst_1_912(.clk(clk),.reset(reset),.i1(intermediate_reg_0[617]),.i2(intermediate_reg_0[616]),.o(intermediate_reg_1[308])); 
mux_module mux_module_inst_1_913(.clk(clk),.reset(reset),.i1(intermediate_reg_0[615]),.i2(intermediate_reg_0[614]),.o(intermediate_reg_1[307]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_914(.clk(clk),.reset(reset),.i1(intermediate_reg_0[613]),.i2(intermediate_reg_0[612]),.o(intermediate_reg_1[306]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_915(.clk(clk),.reset(reset),.i1(intermediate_reg_0[611]),.i2(intermediate_reg_0[610]),.o(intermediate_reg_1[305]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_916(.clk(clk),.reset(reset),.i1(intermediate_reg_0[609]),.i2(intermediate_reg_0[608]),.o(intermediate_reg_1[304])); 
mux_module mux_module_inst_1_917(.clk(clk),.reset(reset),.i1(intermediate_reg_0[607]),.i2(intermediate_reg_0[606]),.o(intermediate_reg_1[303]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_918(.clk(clk),.reset(reset),.i1(intermediate_reg_0[605]),.i2(intermediate_reg_0[604]),.o(intermediate_reg_1[302])); 
xor_module xor_module_inst_1_919(.clk(clk),.reset(reset),.i1(intermediate_reg_0[603]),.i2(intermediate_reg_0[602]),.o(intermediate_reg_1[301])); 
mux_module mux_module_inst_1_920(.clk(clk),.reset(reset),.i1(intermediate_reg_0[601]),.i2(intermediate_reg_0[600]),.o(intermediate_reg_1[300]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_921(.clk(clk),.reset(reset),.i1(intermediate_reg_0[599]),.i2(intermediate_reg_0[598]),.o(intermediate_reg_1[299])); 
mux_module mux_module_inst_1_922(.clk(clk),.reset(reset),.i1(intermediate_reg_0[597]),.i2(intermediate_reg_0[596]),.o(intermediate_reg_1[298]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_923(.clk(clk),.reset(reset),.i1(intermediate_reg_0[595]),.i2(intermediate_reg_0[594]),.o(intermediate_reg_1[297])); 
xor_module xor_module_inst_1_924(.clk(clk),.reset(reset),.i1(intermediate_reg_0[593]),.i2(intermediate_reg_0[592]),.o(intermediate_reg_1[296])); 
mux_module mux_module_inst_1_925(.clk(clk),.reset(reset),.i1(intermediate_reg_0[591]),.i2(intermediate_reg_0[590]),.o(intermediate_reg_1[295]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_926(.clk(clk),.reset(reset),.i1(intermediate_reg_0[589]),.i2(intermediate_reg_0[588]),.o(intermediate_reg_1[294])); 
xor_module xor_module_inst_1_927(.clk(clk),.reset(reset),.i1(intermediate_reg_0[587]),.i2(intermediate_reg_0[586]),.o(intermediate_reg_1[293])); 
xor_module xor_module_inst_1_928(.clk(clk),.reset(reset),.i1(intermediate_reg_0[585]),.i2(intermediate_reg_0[584]),.o(intermediate_reg_1[292])); 
xor_module xor_module_inst_1_929(.clk(clk),.reset(reset),.i1(intermediate_reg_0[583]),.i2(intermediate_reg_0[582]),.o(intermediate_reg_1[291])); 
xor_module xor_module_inst_1_930(.clk(clk),.reset(reset),.i1(intermediate_reg_0[581]),.i2(intermediate_reg_0[580]),.o(intermediate_reg_1[290])); 
xor_module xor_module_inst_1_931(.clk(clk),.reset(reset),.i1(intermediate_reg_0[579]),.i2(intermediate_reg_0[578]),.o(intermediate_reg_1[289])); 
mux_module mux_module_inst_1_932(.clk(clk),.reset(reset),.i1(intermediate_reg_0[577]),.i2(intermediate_reg_0[576]),.o(intermediate_reg_1[288]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_933(.clk(clk),.reset(reset),.i1(intermediate_reg_0[575]),.i2(intermediate_reg_0[574]),.o(intermediate_reg_1[287]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_934(.clk(clk),.reset(reset),.i1(intermediate_reg_0[573]),.i2(intermediate_reg_0[572]),.o(intermediate_reg_1[286]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_935(.clk(clk),.reset(reset),.i1(intermediate_reg_0[571]),.i2(intermediate_reg_0[570]),.o(intermediate_reg_1[285]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_936(.clk(clk),.reset(reset),.i1(intermediate_reg_0[569]),.i2(intermediate_reg_0[568]),.o(intermediate_reg_1[284]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_937(.clk(clk),.reset(reset),.i1(intermediate_reg_0[567]),.i2(intermediate_reg_0[566]),.o(intermediate_reg_1[283])); 
xor_module xor_module_inst_1_938(.clk(clk),.reset(reset),.i1(intermediate_reg_0[565]),.i2(intermediate_reg_0[564]),.o(intermediate_reg_1[282])); 
xor_module xor_module_inst_1_939(.clk(clk),.reset(reset),.i1(intermediate_reg_0[563]),.i2(intermediate_reg_0[562]),.o(intermediate_reg_1[281])); 
xor_module xor_module_inst_1_940(.clk(clk),.reset(reset),.i1(intermediate_reg_0[561]),.i2(intermediate_reg_0[560]),.o(intermediate_reg_1[280])); 
mux_module mux_module_inst_1_941(.clk(clk),.reset(reset),.i1(intermediate_reg_0[559]),.i2(intermediate_reg_0[558]),.o(intermediate_reg_1[279]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_942(.clk(clk),.reset(reset),.i1(intermediate_reg_0[557]),.i2(intermediate_reg_0[556]),.o(intermediate_reg_1[278])); 
xor_module xor_module_inst_1_943(.clk(clk),.reset(reset),.i1(intermediate_reg_0[555]),.i2(intermediate_reg_0[554]),.o(intermediate_reg_1[277])); 
xor_module xor_module_inst_1_944(.clk(clk),.reset(reset),.i1(intermediate_reg_0[553]),.i2(intermediate_reg_0[552]),.o(intermediate_reg_1[276])); 
mux_module mux_module_inst_1_945(.clk(clk),.reset(reset),.i1(intermediate_reg_0[551]),.i2(intermediate_reg_0[550]),.o(intermediate_reg_1[275]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_946(.clk(clk),.reset(reset),.i1(intermediate_reg_0[549]),.i2(intermediate_reg_0[548]),.o(intermediate_reg_1[274])); 
xor_module xor_module_inst_1_947(.clk(clk),.reset(reset),.i1(intermediate_reg_0[547]),.i2(intermediate_reg_0[546]),.o(intermediate_reg_1[273])); 
xor_module xor_module_inst_1_948(.clk(clk),.reset(reset),.i1(intermediate_reg_0[545]),.i2(intermediate_reg_0[544]),.o(intermediate_reg_1[272])); 
xor_module xor_module_inst_1_949(.clk(clk),.reset(reset),.i1(intermediate_reg_0[543]),.i2(intermediate_reg_0[542]),.o(intermediate_reg_1[271])); 
xor_module xor_module_inst_1_950(.clk(clk),.reset(reset),.i1(intermediate_reg_0[541]),.i2(intermediate_reg_0[540]),.o(intermediate_reg_1[270])); 
mux_module mux_module_inst_1_951(.clk(clk),.reset(reset),.i1(intermediate_reg_0[539]),.i2(intermediate_reg_0[538]),.o(intermediate_reg_1[269]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_952(.clk(clk),.reset(reset),.i1(intermediate_reg_0[537]),.i2(intermediate_reg_0[536]),.o(intermediate_reg_1[268])); 
mux_module mux_module_inst_1_953(.clk(clk),.reset(reset),.i1(intermediate_reg_0[535]),.i2(intermediate_reg_0[534]),.o(intermediate_reg_1[267]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_954(.clk(clk),.reset(reset),.i1(intermediate_reg_0[533]),.i2(intermediate_reg_0[532]),.o(intermediate_reg_1[266]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_955(.clk(clk),.reset(reset),.i1(intermediate_reg_0[531]),.i2(intermediate_reg_0[530]),.o(intermediate_reg_1[265]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_956(.clk(clk),.reset(reset),.i1(intermediate_reg_0[529]),.i2(intermediate_reg_0[528]),.o(intermediate_reg_1[264]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_957(.clk(clk),.reset(reset),.i1(intermediate_reg_0[527]),.i2(intermediate_reg_0[526]),.o(intermediate_reg_1[263])); 
xor_module xor_module_inst_1_958(.clk(clk),.reset(reset),.i1(intermediate_reg_0[525]),.i2(intermediate_reg_0[524]),.o(intermediate_reg_1[262])); 
mux_module mux_module_inst_1_959(.clk(clk),.reset(reset),.i1(intermediate_reg_0[523]),.i2(intermediate_reg_0[522]),.o(intermediate_reg_1[261]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_960(.clk(clk),.reset(reset),.i1(intermediate_reg_0[521]),.i2(intermediate_reg_0[520]),.o(intermediate_reg_1[260])); 
xor_module xor_module_inst_1_961(.clk(clk),.reset(reset),.i1(intermediate_reg_0[519]),.i2(intermediate_reg_0[518]),.o(intermediate_reg_1[259])); 
xor_module xor_module_inst_1_962(.clk(clk),.reset(reset),.i1(intermediate_reg_0[517]),.i2(intermediate_reg_0[516]),.o(intermediate_reg_1[258])); 
xor_module xor_module_inst_1_963(.clk(clk),.reset(reset),.i1(intermediate_reg_0[515]),.i2(intermediate_reg_0[514]),.o(intermediate_reg_1[257])); 
mux_module mux_module_inst_1_964(.clk(clk),.reset(reset),.i1(intermediate_reg_0[513]),.i2(intermediate_reg_0[512]),.o(intermediate_reg_1[256]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_965(.clk(clk),.reset(reset),.i1(intermediate_reg_0[511]),.i2(intermediate_reg_0[510]),.o(intermediate_reg_1[255]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_966(.clk(clk),.reset(reset),.i1(intermediate_reg_0[509]),.i2(intermediate_reg_0[508]),.o(intermediate_reg_1[254]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_967(.clk(clk),.reset(reset),.i1(intermediate_reg_0[507]),.i2(intermediate_reg_0[506]),.o(intermediate_reg_1[253])); 
xor_module xor_module_inst_1_968(.clk(clk),.reset(reset),.i1(intermediate_reg_0[505]),.i2(intermediate_reg_0[504]),.o(intermediate_reg_1[252])); 
xor_module xor_module_inst_1_969(.clk(clk),.reset(reset),.i1(intermediate_reg_0[503]),.i2(intermediate_reg_0[502]),.o(intermediate_reg_1[251])); 
xor_module xor_module_inst_1_970(.clk(clk),.reset(reset),.i1(intermediate_reg_0[501]),.i2(intermediate_reg_0[500]),.o(intermediate_reg_1[250])); 
mux_module mux_module_inst_1_971(.clk(clk),.reset(reset),.i1(intermediate_reg_0[499]),.i2(intermediate_reg_0[498]),.o(intermediate_reg_1[249]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_972(.clk(clk),.reset(reset),.i1(intermediate_reg_0[497]),.i2(intermediate_reg_0[496]),.o(intermediate_reg_1[248])); 
xor_module xor_module_inst_1_973(.clk(clk),.reset(reset),.i1(intermediate_reg_0[495]),.i2(intermediate_reg_0[494]),.o(intermediate_reg_1[247])); 
xor_module xor_module_inst_1_974(.clk(clk),.reset(reset),.i1(intermediate_reg_0[493]),.i2(intermediate_reg_0[492]),.o(intermediate_reg_1[246])); 
mux_module mux_module_inst_1_975(.clk(clk),.reset(reset),.i1(intermediate_reg_0[491]),.i2(intermediate_reg_0[490]),.o(intermediate_reg_1[245]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_976(.clk(clk),.reset(reset),.i1(intermediate_reg_0[489]),.i2(intermediate_reg_0[488]),.o(intermediate_reg_1[244])); 
xor_module xor_module_inst_1_977(.clk(clk),.reset(reset),.i1(intermediate_reg_0[487]),.i2(intermediate_reg_0[486]),.o(intermediate_reg_1[243])); 
mux_module mux_module_inst_1_978(.clk(clk),.reset(reset),.i1(intermediate_reg_0[485]),.i2(intermediate_reg_0[484]),.o(intermediate_reg_1[242]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_979(.clk(clk),.reset(reset),.i1(intermediate_reg_0[483]),.i2(intermediate_reg_0[482]),.o(intermediate_reg_1[241])); 
mux_module mux_module_inst_1_980(.clk(clk),.reset(reset),.i1(intermediate_reg_0[481]),.i2(intermediate_reg_0[480]),.o(intermediate_reg_1[240]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_981(.clk(clk),.reset(reset),.i1(intermediate_reg_0[479]),.i2(intermediate_reg_0[478]),.o(intermediate_reg_1[239])); 
mux_module mux_module_inst_1_982(.clk(clk),.reset(reset),.i1(intermediate_reg_0[477]),.i2(intermediate_reg_0[476]),.o(intermediate_reg_1[238]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_983(.clk(clk),.reset(reset),.i1(intermediate_reg_0[475]),.i2(intermediate_reg_0[474]),.o(intermediate_reg_1[237])); 
mux_module mux_module_inst_1_984(.clk(clk),.reset(reset),.i1(intermediate_reg_0[473]),.i2(intermediate_reg_0[472]),.o(intermediate_reg_1[236]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_985(.clk(clk),.reset(reset),.i1(intermediate_reg_0[471]),.i2(intermediate_reg_0[470]),.o(intermediate_reg_1[235])); 
mux_module mux_module_inst_1_986(.clk(clk),.reset(reset),.i1(intermediate_reg_0[469]),.i2(intermediate_reg_0[468]),.o(intermediate_reg_1[234]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_987(.clk(clk),.reset(reset),.i1(intermediate_reg_0[467]),.i2(intermediate_reg_0[466]),.o(intermediate_reg_1[233]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_988(.clk(clk),.reset(reset),.i1(intermediate_reg_0[465]),.i2(intermediate_reg_0[464]),.o(intermediate_reg_1[232]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_989(.clk(clk),.reset(reset),.i1(intermediate_reg_0[463]),.i2(intermediate_reg_0[462]),.o(intermediate_reg_1[231])); 
xor_module xor_module_inst_1_990(.clk(clk),.reset(reset),.i1(intermediate_reg_0[461]),.i2(intermediate_reg_0[460]),.o(intermediate_reg_1[230])); 
xor_module xor_module_inst_1_991(.clk(clk),.reset(reset),.i1(intermediate_reg_0[459]),.i2(intermediate_reg_0[458]),.o(intermediate_reg_1[229])); 
mux_module mux_module_inst_1_992(.clk(clk),.reset(reset),.i1(intermediate_reg_0[457]),.i2(intermediate_reg_0[456]),.o(intermediate_reg_1[228]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_993(.clk(clk),.reset(reset),.i1(intermediate_reg_0[455]),.i2(intermediate_reg_0[454]),.o(intermediate_reg_1[227])); 
xor_module xor_module_inst_1_994(.clk(clk),.reset(reset),.i1(intermediate_reg_0[453]),.i2(intermediate_reg_0[452]),.o(intermediate_reg_1[226])); 
mux_module mux_module_inst_1_995(.clk(clk),.reset(reset),.i1(intermediate_reg_0[451]),.i2(intermediate_reg_0[450]),.o(intermediate_reg_1[225]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_996(.clk(clk),.reset(reset),.i1(intermediate_reg_0[449]),.i2(intermediate_reg_0[448]),.o(intermediate_reg_1[224]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_997(.clk(clk),.reset(reset),.i1(intermediate_reg_0[447]),.i2(intermediate_reg_0[446]),.o(intermediate_reg_1[223]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_998(.clk(clk),.reset(reset),.i1(intermediate_reg_0[445]),.i2(intermediate_reg_0[444]),.o(intermediate_reg_1[222])); 
xor_module xor_module_inst_1_999(.clk(clk),.reset(reset),.i1(intermediate_reg_0[443]),.i2(intermediate_reg_0[442]),.o(intermediate_reg_1[221])); 
xor_module xor_module_inst_1_1000(.clk(clk),.reset(reset),.i1(intermediate_reg_0[441]),.i2(intermediate_reg_0[440]),.o(intermediate_reg_1[220])); 
mux_module mux_module_inst_1_1001(.clk(clk),.reset(reset),.i1(intermediate_reg_0[439]),.i2(intermediate_reg_0[438]),.o(intermediate_reg_1[219]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1002(.clk(clk),.reset(reset),.i1(intermediate_reg_0[437]),.i2(intermediate_reg_0[436]),.o(intermediate_reg_1[218]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1003(.clk(clk),.reset(reset),.i1(intermediate_reg_0[435]),.i2(intermediate_reg_0[434]),.o(intermediate_reg_1[217]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1004(.clk(clk),.reset(reset),.i1(intermediate_reg_0[433]),.i2(intermediate_reg_0[432]),.o(intermediate_reg_1[216]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1005(.clk(clk),.reset(reset),.i1(intermediate_reg_0[431]),.i2(intermediate_reg_0[430]),.o(intermediate_reg_1[215])); 
mux_module mux_module_inst_1_1006(.clk(clk),.reset(reset),.i1(intermediate_reg_0[429]),.i2(intermediate_reg_0[428]),.o(intermediate_reg_1[214]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1007(.clk(clk),.reset(reset),.i1(intermediate_reg_0[427]),.i2(intermediate_reg_0[426]),.o(intermediate_reg_1[213])); 
xor_module xor_module_inst_1_1008(.clk(clk),.reset(reset),.i1(intermediate_reg_0[425]),.i2(intermediate_reg_0[424]),.o(intermediate_reg_1[212])); 
xor_module xor_module_inst_1_1009(.clk(clk),.reset(reset),.i1(intermediate_reg_0[423]),.i2(intermediate_reg_0[422]),.o(intermediate_reg_1[211])); 
xor_module xor_module_inst_1_1010(.clk(clk),.reset(reset),.i1(intermediate_reg_0[421]),.i2(intermediate_reg_0[420]),.o(intermediate_reg_1[210])); 
mux_module mux_module_inst_1_1011(.clk(clk),.reset(reset),.i1(intermediate_reg_0[419]),.i2(intermediate_reg_0[418]),.o(intermediate_reg_1[209]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1012(.clk(clk),.reset(reset),.i1(intermediate_reg_0[417]),.i2(intermediate_reg_0[416]),.o(intermediate_reg_1[208])); 
mux_module mux_module_inst_1_1013(.clk(clk),.reset(reset),.i1(intermediate_reg_0[415]),.i2(intermediate_reg_0[414]),.o(intermediate_reg_1[207]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1014(.clk(clk),.reset(reset),.i1(intermediate_reg_0[413]),.i2(intermediate_reg_0[412]),.o(intermediate_reg_1[206])); 
mux_module mux_module_inst_1_1015(.clk(clk),.reset(reset),.i1(intermediate_reg_0[411]),.i2(intermediate_reg_0[410]),.o(intermediate_reg_1[205]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1016(.clk(clk),.reset(reset),.i1(intermediate_reg_0[409]),.i2(intermediate_reg_0[408]),.o(intermediate_reg_1[204])); 
xor_module xor_module_inst_1_1017(.clk(clk),.reset(reset),.i1(intermediate_reg_0[407]),.i2(intermediate_reg_0[406]),.o(intermediate_reg_1[203])); 
mux_module mux_module_inst_1_1018(.clk(clk),.reset(reset),.i1(intermediate_reg_0[405]),.i2(intermediate_reg_0[404]),.o(intermediate_reg_1[202]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1019(.clk(clk),.reset(reset),.i1(intermediate_reg_0[403]),.i2(intermediate_reg_0[402]),.o(intermediate_reg_1[201]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1020(.clk(clk),.reset(reset),.i1(intermediate_reg_0[401]),.i2(intermediate_reg_0[400]),.o(intermediate_reg_1[200])); 
xor_module xor_module_inst_1_1021(.clk(clk),.reset(reset),.i1(intermediate_reg_0[399]),.i2(intermediate_reg_0[398]),.o(intermediate_reg_1[199])); 
xor_module xor_module_inst_1_1022(.clk(clk),.reset(reset),.i1(intermediate_reg_0[397]),.i2(intermediate_reg_0[396]),.o(intermediate_reg_1[198])); 
xor_module xor_module_inst_1_1023(.clk(clk),.reset(reset),.i1(intermediate_reg_0[395]),.i2(intermediate_reg_0[394]),.o(intermediate_reg_1[197])); 
mux_module mux_module_inst_1_1024(.clk(clk),.reset(reset),.i1(intermediate_reg_0[393]),.i2(intermediate_reg_0[392]),.o(intermediate_reg_1[196]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1025(.clk(clk),.reset(reset),.i1(intermediate_reg_0[391]),.i2(intermediate_reg_0[390]),.o(intermediate_reg_1[195]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1026(.clk(clk),.reset(reset),.i1(intermediate_reg_0[389]),.i2(intermediate_reg_0[388]),.o(intermediate_reg_1[194]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1027(.clk(clk),.reset(reset),.i1(intermediate_reg_0[387]),.i2(intermediate_reg_0[386]),.o(intermediate_reg_1[193]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1028(.clk(clk),.reset(reset),.i1(intermediate_reg_0[385]),.i2(intermediate_reg_0[384]),.o(intermediate_reg_1[192]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1029(.clk(clk),.reset(reset),.i1(intermediate_reg_0[383]),.i2(intermediate_reg_0[382]),.o(intermediate_reg_1[191]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1030(.clk(clk),.reset(reset),.i1(intermediate_reg_0[381]),.i2(intermediate_reg_0[380]),.o(intermediate_reg_1[190]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1031(.clk(clk),.reset(reset),.i1(intermediate_reg_0[379]),.i2(intermediate_reg_0[378]),.o(intermediate_reg_1[189])); 
xor_module xor_module_inst_1_1032(.clk(clk),.reset(reset),.i1(intermediate_reg_0[377]),.i2(intermediate_reg_0[376]),.o(intermediate_reg_1[188])); 
xor_module xor_module_inst_1_1033(.clk(clk),.reset(reset),.i1(intermediate_reg_0[375]),.i2(intermediate_reg_0[374]),.o(intermediate_reg_1[187])); 
mux_module mux_module_inst_1_1034(.clk(clk),.reset(reset),.i1(intermediate_reg_0[373]),.i2(intermediate_reg_0[372]),.o(intermediate_reg_1[186]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1035(.clk(clk),.reset(reset),.i1(intermediate_reg_0[371]),.i2(intermediate_reg_0[370]),.o(intermediate_reg_1[185]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1036(.clk(clk),.reset(reset),.i1(intermediate_reg_0[369]),.i2(intermediate_reg_0[368]),.o(intermediate_reg_1[184])); 
xor_module xor_module_inst_1_1037(.clk(clk),.reset(reset),.i1(intermediate_reg_0[367]),.i2(intermediate_reg_0[366]),.o(intermediate_reg_1[183])); 
mux_module mux_module_inst_1_1038(.clk(clk),.reset(reset),.i1(intermediate_reg_0[365]),.i2(intermediate_reg_0[364]),.o(intermediate_reg_1[182]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1039(.clk(clk),.reset(reset),.i1(intermediate_reg_0[363]),.i2(intermediate_reg_0[362]),.o(intermediate_reg_1[181])); 
mux_module mux_module_inst_1_1040(.clk(clk),.reset(reset),.i1(intermediate_reg_0[361]),.i2(intermediate_reg_0[360]),.o(intermediate_reg_1[180]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1041(.clk(clk),.reset(reset),.i1(intermediate_reg_0[359]),.i2(intermediate_reg_0[358]),.o(intermediate_reg_1[179])); 
mux_module mux_module_inst_1_1042(.clk(clk),.reset(reset),.i1(intermediate_reg_0[357]),.i2(intermediate_reg_0[356]),.o(intermediate_reg_1[178]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1043(.clk(clk),.reset(reset),.i1(intermediate_reg_0[355]),.i2(intermediate_reg_0[354]),.o(intermediate_reg_1[177]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1044(.clk(clk),.reset(reset),.i1(intermediate_reg_0[353]),.i2(intermediate_reg_0[352]),.o(intermediate_reg_1[176])); 
xor_module xor_module_inst_1_1045(.clk(clk),.reset(reset),.i1(intermediate_reg_0[351]),.i2(intermediate_reg_0[350]),.o(intermediate_reg_1[175])); 
mux_module mux_module_inst_1_1046(.clk(clk),.reset(reset),.i1(intermediate_reg_0[349]),.i2(intermediate_reg_0[348]),.o(intermediate_reg_1[174]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1047(.clk(clk),.reset(reset),.i1(intermediate_reg_0[347]),.i2(intermediate_reg_0[346]),.o(intermediate_reg_1[173])); 
xor_module xor_module_inst_1_1048(.clk(clk),.reset(reset),.i1(intermediate_reg_0[345]),.i2(intermediate_reg_0[344]),.o(intermediate_reg_1[172])); 
mux_module mux_module_inst_1_1049(.clk(clk),.reset(reset),.i1(intermediate_reg_0[343]),.i2(intermediate_reg_0[342]),.o(intermediate_reg_1[171]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1050(.clk(clk),.reset(reset),.i1(intermediate_reg_0[341]),.i2(intermediate_reg_0[340]),.o(intermediate_reg_1[170]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1051(.clk(clk),.reset(reset),.i1(intermediate_reg_0[339]),.i2(intermediate_reg_0[338]),.o(intermediate_reg_1[169]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1052(.clk(clk),.reset(reset),.i1(intermediate_reg_0[337]),.i2(intermediate_reg_0[336]),.o(intermediate_reg_1[168])); 
mux_module mux_module_inst_1_1053(.clk(clk),.reset(reset),.i1(intermediate_reg_0[335]),.i2(intermediate_reg_0[334]),.o(intermediate_reg_1[167]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1054(.clk(clk),.reset(reset),.i1(intermediate_reg_0[333]),.i2(intermediate_reg_0[332]),.o(intermediate_reg_1[166]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1055(.clk(clk),.reset(reset),.i1(intermediate_reg_0[331]),.i2(intermediate_reg_0[330]),.o(intermediate_reg_1[165]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1056(.clk(clk),.reset(reset),.i1(intermediate_reg_0[329]),.i2(intermediate_reg_0[328]),.o(intermediate_reg_1[164])); 
xor_module xor_module_inst_1_1057(.clk(clk),.reset(reset),.i1(intermediate_reg_0[327]),.i2(intermediate_reg_0[326]),.o(intermediate_reg_1[163])); 
xor_module xor_module_inst_1_1058(.clk(clk),.reset(reset),.i1(intermediate_reg_0[325]),.i2(intermediate_reg_0[324]),.o(intermediate_reg_1[162])); 
xor_module xor_module_inst_1_1059(.clk(clk),.reset(reset),.i1(intermediate_reg_0[323]),.i2(intermediate_reg_0[322]),.o(intermediate_reg_1[161])); 
mux_module mux_module_inst_1_1060(.clk(clk),.reset(reset),.i1(intermediate_reg_0[321]),.i2(intermediate_reg_0[320]),.o(intermediate_reg_1[160]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1061(.clk(clk),.reset(reset),.i1(intermediate_reg_0[319]),.i2(intermediate_reg_0[318]),.o(intermediate_reg_1[159]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1062(.clk(clk),.reset(reset),.i1(intermediate_reg_0[317]),.i2(intermediate_reg_0[316]),.o(intermediate_reg_1[158]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1063(.clk(clk),.reset(reset),.i1(intermediate_reg_0[315]),.i2(intermediate_reg_0[314]),.o(intermediate_reg_1[157]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1064(.clk(clk),.reset(reset),.i1(intermediate_reg_0[313]),.i2(intermediate_reg_0[312]),.o(intermediate_reg_1[156]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1065(.clk(clk),.reset(reset),.i1(intermediate_reg_0[311]),.i2(intermediate_reg_0[310]),.o(intermediate_reg_1[155]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1066(.clk(clk),.reset(reset),.i1(intermediate_reg_0[309]),.i2(intermediate_reg_0[308]),.o(intermediate_reg_1[154])); 
mux_module mux_module_inst_1_1067(.clk(clk),.reset(reset),.i1(intermediate_reg_0[307]),.i2(intermediate_reg_0[306]),.o(intermediate_reg_1[153]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1068(.clk(clk),.reset(reset),.i1(intermediate_reg_0[305]),.i2(intermediate_reg_0[304]),.o(intermediate_reg_1[152]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1069(.clk(clk),.reset(reset),.i1(intermediate_reg_0[303]),.i2(intermediate_reg_0[302]),.o(intermediate_reg_1[151]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1070(.clk(clk),.reset(reset),.i1(intermediate_reg_0[301]),.i2(intermediate_reg_0[300]),.o(intermediate_reg_1[150])); 
mux_module mux_module_inst_1_1071(.clk(clk),.reset(reset),.i1(intermediate_reg_0[299]),.i2(intermediate_reg_0[298]),.o(intermediate_reg_1[149]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1072(.clk(clk),.reset(reset),.i1(intermediate_reg_0[297]),.i2(intermediate_reg_0[296]),.o(intermediate_reg_1[148])); 
xor_module xor_module_inst_1_1073(.clk(clk),.reset(reset),.i1(intermediate_reg_0[295]),.i2(intermediate_reg_0[294]),.o(intermediate_reg_1[147])); 
xor_module xor_module_inst_1_1074(.clk(clk),.reset(reset),.i1(intermediate_reg_0[293]),.i2(intermediate_reg_0[292]),.o(intermediate_reg_1[146])); 
mux_module mux_module_inst_1_1075(.clk(clk),.reset(reset),.i1(intermediate_reg_0[291]),.i2(intermediate_reg_0[290]),.o(intermediate_reg_1[145]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1076(.clk(clk),.reset(reset),.i1(intermediate_reg_0[289]),.i2(intermediate_reg_0[288]),.o(intermediate_reg_1[144])); 
mux_module mux_module_inst_1_1077(.clk(clk),.reset(reset),.i1(intermediate_reg_0[287]),.i2(intermediate_reg_0[286]),.o(intermediate_reg_1[143]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1078(.clk(clk),.reset(reset),.i1(intermediate_reg_0[285]),.i2(intermediate_reg_0[284]),.o(intermediate_reg_1[142])); 
mux_module mux_module_inst_1_1079(.clk(clk),.reset(reset),.i1(intermediate_reg_0[283]),.i2(intermediate_reg_0[282]),.o(intermediate_reg_1[141]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1080(.clk(clk),.reset(reset),.i1(intermediate_reg_0[281]),.i2(intermediate_reg_0[280]),.o(intermediate_reg_1[140]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1081(.clk(clk),.reset(reset),.i1(intermediate_reg_0[279]),.i2(intermediate_reg_0[278]),.o(intermediate_reg_1[139]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1082(.clk(clk),.reset(reset),.i1(intermediate_reg_0[277]),.i2(intermediate_reg_0[276]),.o(intermediate_reg_1[138]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1083(.clk(clk),.reset(reset),.i1(intermediate_reg_0[275]),.i2(intermediate_reg_0[274]),.o(intermediate_reg_1[137])); 
xor_module xor_module_inst_1_1084(.clk(clk),.reset(reset),.i1(intermediate_reg_0[273]),.i2(intermediate_reg_0[272]),.o(intermediate_reg_1[136])); 
mux_module mux_module_inst_1_1085(.clk(clk),.reset(reset),.i1(intermediate_reg_0[271]),.i2(intermediate_reg_0[270]),.o(intermediate_reg_1[135]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1086(.clk(clk),.reset(reset),.i1(intermediate_reg_0[269]),.i2(intermediate_reg_0[268]),.o(intermediate_reg_1[134])); 
mux_module mux_module_inst_1_1087(.clk(clk),.reset(reset),.i1(intermediate_reg_0[267]),.i2(intermediate_reg_0[266]),.o(intermediate_reg_1[133]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1088(.clk(clk),.reset(reset),.i1(intermediate_reg_0[265]),.i2(intermediate_reg_0[264]),.o(intermediate_reg_1[132])); 
xor_module xor_module_inst_1_1089(.clk(clk),.reset(reset),.i1(intermediate_reg_0[263]),.i2(intermediate_reg_0[262]),.o(intermediate_reg_1[131])); 
xor_module xor_module_inst_1_1090(.clk(clk),.reset(reset),.i1(intermediate_reg_0[261]),.i2(intermediate_reg_0[260]),.o(intermediate_reg_1[130])); 
xor_module xor_module_inst_1_1091(.clk(clk),.reset(reset),.i1(intermediate_reg_0[259]),.i2(intermediate_reg_0[258]),.o(intermediate_reg_1[129])); 
mux_module mux_module_inst_1_1092(.clk(clk),.reset(reset),.i1(intermediate_reg_0[257]),.i2(intermediate_reg_0[256]),.o(intermediate_reg_1[128]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1093(.clk(clk),.reset(reset),.i1(intermediate_reg_0[255]),.i2(intermediate_reg_0[254]),.o(intermediate_reg_1[127])); 
mux_module mux_module_inst_1_1094(.clk(clk),.reset(reset),.i1(intermediate_reg_0[253]),.i2(intermediate_reg_0[252]),.o(intermediate_reg_1[126]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1095(.clk(clk),.reset(reset),.i1(intermediate_reg_0[251]),.i2(intermediate_reg_0[250]),.o(intermediate_reg_1[125])); 
xor_module xor_module_inst_1_1096(.clk(clk),.reset(reset),.i1(intermediate_reg_0[249]),.i2(intermediate_reg_0[248]),.o(intermediate_reg_1[124])); 
mux_module mux_module_inst_1_1097(.clk(clk),.reset(reset),.i1(intermediate_reg_0[247]),.i2(intermediate_reg_0[246]),.o(intermediate_reg_1[123]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1098(.clk(clk),.reset(reset),.i1(intermediate_reg_0[245]),.i2(intermediate_reg_0[244]),.o(intermediate_reg_1[122]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1099(.clk(clk),.reset(reset),.i1(intermediate_reg_0[243]),.i2(intermediate_reg_0[242]),.o(intermediate_reg_1[121]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1100(.clk(clk),.reset(reset),.i1(intermediate_reg_0[241]),.i2(intermediate_reg_0[240]),.o(intermediate_reg_1[120]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1101(.clk(clk),.reset(reset),.i1(intermediate_reg_0[239]),.i2(intermediate_reg_0[238]),.o(intermediate_reg_1[119])); 
xor_module xor_module_inst_1_1102(.clk(clk),.reset(reset),.i1(intermediate_reg_0[237]),.i2(intermediate_reg_0[236]),.o(intermediate_reg_1[118])); 
mux_module mux_module_inst_1_1103(.clk(clk),.reset(reset),.i1(intermediate_reg_0[235]),.i2(intermediate_reg_0[234]),.o(intermediate_reg_1[117]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1104(.clk(clk),.reset(reset),.i1(intermediate_reg_0[233]),.i2(intermediate_reg_0[232]),.o(intermediate_reg_1[116]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1105(.clk(clk),.reset(reset),.i1(intermediate_reg_0[231]),.i2(intermediate_reg_0[230]),.o(intermediate_reg_1[115]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1106(.clk(clk),.reset(reset),.i1(intermediate_reg_0[229]),.i2(intermediate_reg_0[228]),.o(intermediate_reg_1[114]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1107(.clk(clk),.reset(reset),.i1(intermediate_reg_0[227]),.i2(intermediate_reg_0[226]),.o(intermediate_reg_1[113]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1108(.clk(clk),.reset(reset),.i1(intermediate_reg_0[225]),.i2(intermediate_reg_0[224]),.o(intermediate_reg_1[112]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1109(.clk(clk),.reset(reset),.i1(intermediate_reg_0[223]),.i2(intermediate_reg_0[222]),.o(intermediate_reg_1[111])); 
mux_module mux_module_inst_1_1110(.clk(clk),.reset(reset),.i1(intermediate_reg_0[221]),.i2(intermediate_reg_0[220]),.o(intermediate_reg_1[110]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1111(.clk(clk),.reset(reset),.i1(intermediate_reg_0[219]),.i2(intermediate_reg_0[218]),.o(intermediate_reg_1[109])); 
xor_module xor_module_inst_1_1112(.clk(clk),.reset(reset),.i1(intermediate_reg_0[217]),.i2(intermediate_reg_0[216]),.o(intermediate_reg_1[108])); 
mux_module mux_module_inst_1_1113(.clk(clk),.reset(reset),.i1(intermediate_reg_0[215]),.i2(intermediate_reg_0[214]),.o(intermediate_reg_1[107]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1114(.clk(clk),.reset(reset),.i1(intermediate_reg_0[213]),.i2(intermediate_reg_0[212]),.o(intermediate_reg_1[106]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1115(.clk(clk),.reset(reset),.i1(intermediate_reg_0[211]),.i2(intermediate_reg_0[210]),.o(intermediate_reg_1[105])); 
mux_module mux_module_inst_1_1116(.clk(clk),.reset(reset),.i1(intermediate_reg_0[209]),.i2(intermediate_reg_0[208]),.o(intermediate_reg_1[104]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1117(.clk(clk),.reset(reset),.i1(intermediate_reg_0[207]),.i2(intermediate_reg_0[206]),.o(intermediate_reg_1[103])); 
xor_module xor_module_inst_1_1118(.clk(clk),.reset(reset),.i1(intermediate_reg_0[205]),.i2(intermediate_reg_0[204]),.o(intermediate_reg_1[102])); 
mux_module mux_module_inst_1_1119(.clk(clk),.reset(reset),.i1(intermediate_reg_0[203]),.i2(intermediate_reg_0[202]),.o(intermediate_reg_1[101]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1120(.clk(clk),.reset(reset),.i1(intermediate_reg_0[201]),.i2(intermediate_reg_0[200]),.o(intermediate_reg_1[100]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1121(.clk(clk),.reset(reset),.i1(intermediate_reg_0[199]),.i2(intermediate_reg_0[198]),.o(intermediate_reg_1[99])); 
mux_module mux_module_inst_1_1122(.clk(clk),.reset(reset),.i1(intermediate_reg_0[197]),.i2(intermediate_reg_0[196]),.o(intermediate_reg_1[98]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1123(.clk(clk),.reset(reset),.i1(intermediate_reg_0[195]),.i2(intermediate_reg_0[194]),.o(intermediate_reg_1[97])); 
mux_module mux_module_inst_1_1124(.clk(clk),.reset(reset),.i1(intermediate_reg_0[193]),.i2(intermediate_reg_0[192]),.o(intermediate_reg_1[96]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1125(.clk(clk),.reset(reset),.i1(intermediate_reg_0[191]),.i2(intermediate_reg_0[190]),.o(intermediate_reg_1[95])); 
xor_module xor_module_inst_1_1126(.clk(clk),.reset(reset),.i1(intermediate_reg_0[189]),.i2(intermediate_reg_0[188]),.o(intermediate_reg_1[94])); 
mux_module mux_module_inst_1_1127(.clk(clk),.reset(reset),.i1(intermediate_reg_0[187]),.i2(intermediate_reg_0[186]),.o(intermediate_reg_1[93]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1128(.clk(clk),.reset(reset),.i1(intermediate_reg_0[185]),.i2(intermediate_reg_0[184]),.o(intermediate_reg_1[92]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1129(.clk(clk),.reset(reset),.i1(intermediate_reg_0[183]),.i2(intermediate_reg_0[182]),.o(intermediate_reg_1[91]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1130(.clk(clk),.reset(reset),.i1(intermediate_reg_0[181]),.i2(intermediate_reg_0[180]),.o(intermediate_reg_1[90]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1131(.clk(clk),.reset(reset),.i1(intermediate_reg_0[179]),.i2(intermediate_reg_0[178]),.o(intermediate_reg_1[89]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1132(.clk(clk),.reset(reset),.i1(intermediate_reg_0[177]),.i2(intermediate_reg_0[176]),.o(intermediate_reg_1[88])); 
mux_module mux_module_inst_1_1133(.clk(clk),.reset(reset),.i1(intermediate_reg_0[175]),.i2(intermediate_reg_0[174]),.o(intermediate_reg_1[87]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1134(.clk(clk),.reset(reset),.i1(intermediate_reg_0[173]),.i2(intermediate_reg_0[172]),.o(intermediate_reg_1[86]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1135(.clk(clk),.reset(reset),.i1(intermediate_reg_0[171]),.i2(intermediate_reg_0[170]),.o(intermediate_reg_1[85]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1136(.clk(clk),.reset(reset),.i1(intermediate_reg_0[169]),.i2(intermediate_reg_0[168]),.o(intermediate_reg_1[84]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1137(.clk(clk),.reset(reset),.i1(intermediate_reg_0[167]),.i2(intermediate_reg_0[166]),.o(intermediate_reg_1[83]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1138(.clk(clk),.reset(reset),.i1(intermediate_reg_0[165]),.i2(intermediate_reg_0[164]),.o(intermediate_reg_1[82])); 
xor_module xor_module_inst_1_1139(.clk(clk),.reset(reset),.i1(intermediate_reg_0[163]),.i2(intermediate_reg_0[162]),.o(intermediate_reg_1[81])); 
xor_module xor_module_inst_1_1140(.clk(clk),.reset(reset),.i1(intermediate_reg_0[161]),.i2(intermediate_reg_0[160]),.o(intermediate_reg_1[80])); 
xor_module xor_module_inst_1_1141(.clk(clk),.reset(reset),.i1(intermediate_reg_0[159]),.i2(intermediate_reg_0[158]),.o(intermediate_reg_1[79])); 
mux_module mux_module_inst_1_1142(.clk(clk),.reset(reset),.i1(intermediate_reg_0[157]),.i2(intermediate_reg_0[156]),.o(intermediate_reg_1[78]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1143(.clk(clk),.reset(reset),.i1(intermediate_reg_0[155]),.i2(intermediate_reg_0[154]),.o(intermediate_reg_1[77]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1144(.clk(clk),.reset(reset),.i1(intermediate_reg_0[153]),.i2(intermediate_reg_0[152]),.o(intermediate_reg_1[76])); 
xor_module xor_module_inst_1_1145(.clk(clk),.reset(reset),.i1(intermediate_reg_0[151]),.i2(intermediate_reg_0[150]),.o(intermediate_reg_1[75])); 
mux_module mux_module_inst_1_1146(.clk(clk),.reset(reset),.i1(intermediate_reg_0[149]),.i2(intermediate_reg_0[148]),.o(intermediate_reg_1[74]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1147(.clk(clk),.reset(reset),.i1(intermediate_reg_0[147]),.i2(intermediate_reg_0[146]),.o(intermediate_reg_1[73]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1148(.clk(clk),.reset(reset),.i1(intermediate_reg_0[145]),.i2(intermediate_reg_0[144]),.o(intermediate_reg_1[72]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1149(.clk(clk),.reset(reset),.i1(intermediate_reg_0[143]),.i2(intermediate_reg_0[142]),.o(intermediate_reg_1[71]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1150(.clk(clk),.reset(reset),.i1(intermediate_reg_0[141]),.i2(intermediate_reg_0[140]),.o(intermediate_reg_1[70])); 
xor_module xor_module_inst_1_1151(.clk(clk),.reset(reset),.i1(intermediate_reg_0[139]),.i2(intermediate_reg_0[138]),.o(intermediate_reg_1[69])); 
xor_module xor_module_inst_1_1152(.clk(clk),.reset(reset),.i1(intermediate_reg_0[137]),.i2(intermediate_reg_0[136]),.o(intermediate_reg_1[68])); 
xor_module xor_module_inst_1_1153(.clk(clk),.reset(reset),.i1(intermediate_reg_0[135]),.i2(intermediate_reg_0[134]),.o(intermediate_reg_1[67])); 
xor_module xor_module_inst_1_1154(.clk(clk),.reset(reset),.i1(intermediate_reg_0[133]),.i2(intermediate_reg_0[132]),.o(intermediate_reg_1[66])); 
xor_module xor_module_inst_1_1155(.clk(clk),.reset(reset),.i1(intermediate_reg_0[131]),.i2(intermediate_reg_0[130]),.o(intermediate_reg_1[65])); 
mux_module mux_module_inst_1_1156(.clk(clk),.reset(reset),.i1(intermediate_reg_0[129]),.i2(intermediate_reg_0[128]),.o(intermediate_reg_1[64]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1157(.clk(clk),.reset(reset),.i1(intermediate_reg_0[127]),.i2(intermediate_reg_0[126]),.o(intermediate_reg_1[63]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1158(.clk(clk),.reset(reset),.i1(intermediate_reg_0[125]),.i2(intermediate_reg_0[124]),.o(intermediate_reg_1[62]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1159(.clk(clk),.reset(reset),.i1(intermediate_reg_0[123]),.i2(intermediate_reg_0[122]),.o(intermediate_reg_1[61])); 
mux_module mux_module_inst_1_1160(.clk(clk),.reset(reset),.i1(intermediate_reg_0[121]),.i2(intermediate_reg_0[120]),.o(intermediate_reg_1[60]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1161(.clk(clk),.reset(reset),.i1(intermediate_reg_0[119]),.i2(intermediate_reg_0[118]),.o(intermediate_reg_1[59])); 
xor_module xor_module_inst_1_1162(.clk(clk),.reset(reset),.i1(intermediate_reg_0[117]),.i2(intermediate_reg_0[116]),.o(intermediate_reg_1[58])); 
xor_module xor_module_inst_1_1163(.clk(clk),.reset(reset),.i1(intermediate_reg_0[115]),.i2(intermediate_reg_0[114]),.o(intermediate_reg_1[57])); 
mux_module mux_module_inst_1_1164(.clk(clk),.reset(reset),.i1(intermediate_reg_0[113]),.i2(intermediate_reg_0[112]),.o(intermediate_reg_1[56]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1165(.clk(clk),.reset(reset),.i1(intermediate_reg_0[111]),.i2(intermediate_reg_0[110]),.o(intermediate_reg_1[55])); 
xor_module xor_module_inst_1_1166(.clk(clk),.reset(reset),.i1(intermediate_reg_0[109]),.i2(intermediate_reg_0[108]),.o(intermediate_reg_1[54])); 
mux_module mux_module_inst_1_1167(.clk(clk),.reset(reset),.i1(intermediate_reg_0[107]),.i2(intermediate_reg_0[106]),.o(intermediate_reg_1[53]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1168(.clk(clk),.reset(reset),.i1(intermediate_reg_0[105]),.i2(intermediate_reg_0[104]),.o(intermediate_reg_1[52])); 
xor_module xor_module_inst_1_1169(.clk(clk),.reset(reset),.i1(intermediate_reg_0[103]),.i2(intermediate_reg_0[102]),.o(intermediate_reg_1[51])); 
mux_module mux_module_inst_1_1170(.clk(clk),.reset(reset),.i1(intermediate_reg_0[101]),.i2(intermediate_reg_0[100]),.o(intermediate_reg_1[50]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1171(.clk(clk),.reset(reset),.i1(intermediate_reg_0[99]),.i2(intermediate_reg_0[98]),.o(intermediate_reg_1[49])); 
xor_module xor_module_inst_1_1172(.clk(clk),.reset(reset),.i1(intermediate_reg_0[97]),.i2(intermediate_reg_0[96]),.o(intermediate_reg_1[48])); 
mux_module mux_module_inst_1_1173(.clk(clk),.reset(reset),.i1(intermediate_reg_0[95]),.i2(intermediate_reg_0[94]),.o(intermediate_reg_1[47]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1174(.clk(clk),.reset(reset),.i1(intermediate_reg_0[93]),.i2(intermediate_reg_0[92]),.o(intermediate_reg_1[46]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1175(.clk(clk),.reset(reset),.i1(intermediate_reg_0[91]),.i2(intermediate_reg_0[90]),.o(intermediate_reg_1[45]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1176(.clk(clk),.reset(reset),.i1(intermediate_reg_0[89]),.i2(intermediate_reg_0[88]),.o(intermediate_reg_1[44]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1177(.clk(clk),.reset(reset),.i1(intermediate_reg_0[87]),.i2(intermediate_reg_0[86]),.o(intermediate_reg_1[43]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1178(.clk(clk),.reset(reset),.i1(intermediate_reg_0[85]),.i2(intermediate_reg_0[84]),.o(intermediate_reg_1[42])); 
mux_module mux_module_inst_1_1179(.clk(clk),.reset(reset),.i1(intermediate_reg_0[83]),.i2(intermediate_reg_0[82]),.o(intermediate_reg_1[41]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1180(.clk(clk),.reset(reset),.i1(intermediate_reg_0[81]),.i2(intermediate_reg_0[80]),.o(intermediate_reg_1[40])); 
xor_module xor_module_inst_1_1181(.clk(clk),.reset(reset),.i1(intermediate_reg_0[79]),.i2(intermediate_reg_0[78]),.o(intermediate_reg_1[39])); 
mux_module mux_module_inst_1_1182(.clk(clk),.reset(reset),.i1(intermediate_reg_0[77]),.i2(intermediate_reg_0[76]),.o(intermediate_reg_1[38]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1183(.clk(clk),.reset(reset),.i1(intermediate_reg_0[75]),.i2(intermediate_reg_0[74]),.o(intermediate_reg_1[37])); 
mux_module mux_module_inst_1_1184(.clk(clk),.reset(reset),.i1(intermediate_reg_0[73]),.i2(intermediate_reg_0[72]),.o(intermediate_reg_1[36]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1185(.clk(clk),.reset(reset),.i1(intermediate_reg_0[71]),.i2(intermediate_reg_0[70]),.o(intermediate_reg_1[35]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1186(.clk(clk),.reset(reset),.i1(intermediate_reg_0[69]),.i2(intermediate_reg_0[68]),.o(intermediate_reg_1[34])); 
xor_module xor_module_inst_1_1187(.clk(clk),.reset(reset),.i1(intermediate_reg_0[67]),.i2(intermediate_reg_0[66]),.o(intermediate_reg_1[33])); 
xor_module xor_module_inst_1_1188(.clk(clk),.reset(reset),.i1(intermediate_reg_0[65]),.i2(intermediate_reg_0[64]),.o(intermediate_reg_1[32])); 
mux_module mux_module_inst_1_1189(.clk(clk),.reset(reset),.i1(intermediate_reg_0[63]),.i2(intermediate_reg_0[62]),.o(intermediate_reg_1[31]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1190(.clk(clk),.reset(reset),.i1(intermediate_reg_0[61]),.i2(intermediate_reg_0[60]),.o(intermediate_reg_1[30]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1191(.clk(clk),.reset(reset),.i1(intermediate_reg_0[59]),.i2(intermediate_reg_0[58]),.o(intermediate_reg_1[29]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1192(.clk(clk),.reset(reset),.i1(intermediate_reg_0[57]),.i2(intermediate_reg_0[56]),.o(intermediate_reg_1[28]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1193(.clk(clk),.reset(reset),.i1(intermediate_reg_0[55]),.i2(intermediate_reg_0[54]),.o(intermediate_reg_1[27]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1194(.clk(clk),.reset(reset),.i1(intermediate_reg_0[53]),.i2(intermediate_reg_0[52]),.o(intermediate_reg_1[26]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1195(.clk(clk),.reset(reset),.i1(intermediate_reg_0[51]),.i2(intermediate_reg_0[50]),.o(intermediate_reg_1[25]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1196(.clk(clk),.reset(reset),.i1(intermediate_reg_0[49]),.i2(intermediate_reg_0[48]),.o(intermediate_reg_1[24]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1197(.clk(clk),.reset(reset),.i1(intermediate_reg_0[47]),.i2(intermediate_reg_0[46]),.o(intermediate_reg_1[23])); 
mux_module mux_module_inst_1_1198(.clk(clk),.reset(reset),.i1(intermediate_reg_0[45]),.i2(intermediate_reg_0[44]),.o(intermediate_reg_1[22]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1199(.clk(clk),.reset(reset),.i1(intermediate_reg_0[43]),.i2(intermediate_reg_0[42]),.o(intermediate_reg_1[21]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1200(.clk(clk),.reset(reset),.i1(intermediate_reg_0[41]),.i2(intermediate_reg_0[40]),.o(intermediate_reg_1[20])); 
mux_module mux_module_inst_1_1201(.clk(clk),.reset(reset),.i1(intermediate_reg_0[39]),.i2(intermediate_reg_0[38]),.o(intermediate_reg_1[19]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1202(.clk(clk),.reset(reset),.i1(intermediate_reg_0[37]),.i2(intermediate_reg_0[36]),.o(intermediate_reg_1[18])); 
mux_module mux_module_inst_1_1203(.clk(clk),.reset(reset),.i1(intermediate_reg_0[35]),.i2(intermediate_reg_0[34]),.o(intermediate_reg_1[17]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1204(.clk(clk),.reset(reset),.i1(intermediate_reg_0[33]),.i2(intermediate_reg_0[32]),.o(intermediate_reg_1[16])); 
xor_module xor_module_inst_1_1205(.clk(clk),.reset(reset),.i1(intermediate_reg_0[31]),.i2(intermediate_reg_0[30]),.o(intermediate_reg_1[15])); 
mux_module mux_module_inst_1_1206(.clk(clk),.reset(reset),.i1(intermediate_reg_0[29]),.i2(intermediate_reg_0[28]),.o(intermediate_reg_1[14]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1207(.clk(clk),.reset(reset),.i1(intermediate_reg_0[27]),.i2(intermediate_reg_0[26]),.o(intermediate_reg_1[13])); 
xor_module xor_module_inst_1_1208(.clk(clk),.reset(reset),.i1(intermediate_reg_0[25]),.i2(intermediate_reg_0[24]),.o(intermediate_reg_1[12])); 
xor_module xor_module_inst_1_1209(.clk(clk),.reset(reset),.i1(intermediate_reg_0[23]),.i2(intermediate_reg_0[22]),.o(intermediate_reg_1[11])); 
mux_module mux_module_inst_1_1210(.clk(clk),.reset(reset),.i1(intermediate_reg_0[21]),.i2(intermediate_reg_0[20]),.o(intermediate_reg_1[10]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1211(.clk(clk),.reset(reset),.i1(intermediate_reg_0[19]),.i2(intermediate_reg_0[18]),.o(intermediate_reg_1[9])); 
xor_module xor_module_inst_1_1212(.clk(clk),.reset(reset),.i1(intermediate_reg_0[17]),.i2(intermediate_reg_0[16]),.o(intermediate_reg_1[8])); 
xor_module xor_module_inst_1_1213(.clk(clk),.reset(reset),.i1(intermediate_reg_0[15]),.i2(intermediate_reg_0[14]),.o(intermediate_reg_1[7])); 
xor_module xor_module_inst_1_1214(.clk(clk),.reset(reset),.i1(intermediate_reg_0[13]),.i2(intermediate_reg_0[12]),.o(intermediate_reg_1[6])); 
mux_module mux_module_inst_1_1215(.clk(clk),.reset(reset),.i1(intermediate_reg_0[11]),.i2(intermediate_reg_0[10]),.o(intermediate_reg_1[5]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1216(.clk(clk),.reset(reset),.i1(intermediate_reg_0[9]),.i2(intermediate_reg_0[8]),.o(intermediate_reg_1[4]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_1217(.clk(clk),.reset(reset),.i1(intermediate_reg_0[7]),.i2(intermediate_reg_0[6]),.o(intermediate_reg_1[3]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1218(.clk(clk),.reset(reset),.i1(intermediate_reg_0[5]),.i2(intermediate_reg_0[4]),.o(intermediate_reg_1[2])); 
xor_module xor_module_inst_1_1219(.clk(clk),.reset(reset),.i1(intermediate_reg_0[3]),.i2(intermediate_reg_0[2]),.o(intermediate_reg_1[1])); 
mux_module mux_module_inst_1_1220(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1]),.i2(intermediate_reg_0[0]),.o(intermediate_reg_1[0]),.sel(intermediate_reg_0[0])); 
wire [1219:0]intermediate_wire_2; 
assign intermediate_wire_2[1219] = intermediate_reg_1[1220]^intermediate_reg_1[1219] ; 
assign intermediate_wire_2[1218:0] = intermediate_reg_1[1218:0] ; 
always@(posedge clk) begin 
outp [1219:0] <= intermediate_wire_2; 
outp[1535:1220] <= intermediate_wire_2[315:0] ; 
end 
endmodule 
 

module interface_8(input [130:0] inp, output reg [575:0] outp, input clk, input reset);
always@(posedge clk) begin 
outp[130:0] <= inp ; 
outp[261:131] <= inp ; 
outp[392:262] <= inp ; 
outp[523:393] <= inp ; 
outp[575:524] <= inp[51:0] ; 
end 
endmodule 

module interface_9(input [130:0] inp, output reg [847:0] outp, input clk, input reset);
always@(posedge clk) begin 
outp[130:0] <= inp ; 
outp[261:131] <= inp ; 
outp[392:262] <= inp ; 
outp[523:393] <= inp ; 
outp[654:524] <= inp ; 
outp[785:655] <= inp ; 
outp[847:786] <= inp[61:0] ; 
end 
endmodule 

module interface_10(input [130:0] inp, output reg [63:0] outp, input clk, input reset);
reg [130:0]intermediate_reg_0; 
always@(posedge clk) begin 
intermediate_reg_0 <= inp; 
end 
 
wire [129:0]intermediate_wire_1; 
assign intermediate_wire_1[129] = intermediate_reg_0[130]^intermediate_reg_0[129] ; 
assign intermediate_wire_1[128:0] = intermediate_reg_0[128:0] ; 
wire [64:0]intermediate_reg_1; 
 
xor_module xor_module_inst_1_0(.clk(clk),.reset(reset),.i1(intermediate_reg_0[129]),.i2(intermediate_reg_0[128]),.o(intermediate_reg_1[64])); 
mux_module mux_module_inst_1_1(.clk(clk),.reset(reset),.i1(intermediate_reg_0[127]),.i2(intermediate_reg_0[126]),.o(intermediate_reg_1[63]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_2(.clk(clk),.reset(reset),.i1(intermediate_reg_0[125]),.i2(intermediate_reg_0[124]),.o(intermediate_reg_1[62])); 
mux_module mux_module_inst_1_3(.clk(clk),.reset(reset),.i1(intermediate_reg_0[123]),.i2(intermediate_reg_0[122]),.o(intermediate_reg_1[61]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_4(.clk(clk),.reset(reset),.i1(intermediate_reg_0[121]),.i2(intermediate_reg_0[120]),.o(intermediate_reg_1[60]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_5(.clk(clk),.reset(reset),.i1(intermediate_reg_0[119]),.i2(intermediate_reg_0[118]),.o(intermediate_reg_1[59])); 
xor_module xor_module_inst_1_6(.clk(clk),.reset(reset),.i1(intermediate_reg_0[117]),.i2(intermediate_reg_0[116]),.o(intermediate_reg_1[58])); 
mux_module mux_module_inst_1_7(.clk(clk),.reset(reset),.i1(intermediate_reg_0[115]),.i2(intermediate_reg_0[114]),.o(intermediate_reg_1[57]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_8(.clk(clk),.reset(reset),.i1(intermediate_reg_0[113]),.i2(intermediate_reg_0[112]),.o(intermediate_reg_1[56]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_9(.clk(clk),.reset(reset),.i1(intermediate_reg_0[111]),.i2(intermediate_reg_0[110]),.o(intermediate_reg_1[55])); 
mux_module mux_module_inst_1_10(.clk(clk),.reset(reset),.i1(intermediate_reg_0[109]),.i2(intermediate_reg_0[108]),.o(intermediate_reg_1[54]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_11(.clk(clk),.reset(reset),.i1(intermediate_reg_0[107]),.i2(intermediate_reg_0[106]),.o(intermediate_reg_1[53])); 
xor_module xor_module_inst_1_12(.clk(clk),.reset(reset),.i1(intermediate_reg_0[105]),.i2(intermediate_reg_0[104]),.o(intermediate_reg_1[52])); 
mux_module mux_module_inst_1_13(.clk(clk),.reset(reset),.i1(intermediate_reg_0[103]),.i2(intermediate_reg_0[102]),.o(intermediate_reg_1[51]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_14(.clk(clk),.reset(reset),.i1(intermediate_reg_0[101]),.i2(intermediate_reg_0[100]),.o(intermediate_reg_1[50])); 
xor_module xor_module_inst_1_15(.clk(clk),.reset(reset),.i1(intermediate_reg_0[99]),.i2(intermediate_reg_0[98]),.o(intermediate_reg_1[49])); 
mux_module mux_module_inst_1_16(.clk(clk),.reset(reset),.i1(intermediate_reg_0[97]),.i2(intermediate_reg_0[96]),.o(intermediate_reg_1[48]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_17(.clk(clk),.reset(reset),.i1(intermediate_reg_0[95]),.i2(intermediate_reg_0[94]),.o(intermediate_reg_1[47])); 
mux_module mux_module_inst_1_18(.clk(clk),.reset(reset),.i1(intermediate_reg_0[93]),.i2(intermediate_reg_0[92]),.o(intermediate_reg_1[46]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_19(.clk(clk),.reset(reset),.i1(intermediate_reg_0[91]),.i2(intermediate_reg_0[90]),.o(intermediate_reg_1[45])); 
mux_module mux_module_inst_1_20(.clk(clk),.reset(reset),.i1(intermediate_reg_0[89]),.i2(intermediate_reg_0[88]),.o(intermediate_reg_1[44]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_21(.clk(clk),.reset(reset),.i1(intermediate_reg_0[87]),.i2(intermediate_reg_0[86]),.o(intermediate_reg_1[43]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_22(.clk(clk),.reset(reset),.i1(intermediate_reg_0[85]),.i2(intermediate_reg_0[84]),.o(intermediate_reg_1[42])); 
xor_module xor_module_inst_1_23(.clk(clk),.reset(reset),.i1(intermediate_reg_0[83]),.i2(intermediate_reg_0[82]),.o(intermediate_reg_1[41])); 
xor_module xor_module_inst_1_24(.clk(clk),.reset(reset),.i1(intermediate_reg_0[81]),.i2(intermediate_reg_0[80]),.o(intermediate_reg_1[40])); 
mux_module mux_module_inst_1_25(.clk(clk),.reset(reset),.i1(intermediate_reg_0[79]),.i2(intermediate_reg_0[78]),.o(intermediate_reg_1[39]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_26(.clk(clk),.reset(reset),.i1(intermediate_reg_0[77]),.i2(intermediate_reg_0[76]),.o(intermediate_reg_1[38])); 
mux_module mux_module_inst_1_27(.clk(clk),.reset(reset),.i1(intermediate_reg_0[75]),.i2(intermediate_reg_0[74]),.o(intermediate_reg_1[37]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_28(.clk(clk),.reset(reset),.i1(intermediate_reg_0[73]),.i2(intermediate_reg_0[72]),.o(intermediate_reg_1[36])); 
xor_module xor_module_inst_1_29(.clk(clk),.reset(reset),.i1(intermediate_reg_0[71]),.i2(intermediate_reg_0[70]),.o(intermediate_reg_1[35])); 
xor_module xor_module_inst_1_30(.clk(clk),.reset(reset),.i1(intermediate_reg_0[69]),.i2(intermediate_reg_0[68]),.o(intermediate_reg_1[34])); 
mux_module mux_module_inst_1_31(.clk(clk),.reset(reset),.i1(intermediate_reg_0[67]),.i2(intermediate_reg_0[66]),.o(intermediate_reg_1[33]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_32(.clk(clk),.reset(reset),.i1(intermediate_reg_0[65]),.i2(intermediate_reg_0[64]),.o(intermediate_reg_1[32]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_33(.clk(clk),.reset(reset),.i1(intermediate_reg_0[63]),.i2(intermediate_reg_0[62]),.o(intermediate_reg_1[31])); 
xor_module xor_module_inst_1_34(.clk(clk),.reset(reset),.i1(intermediate_reg_0[61]),.i2(intermediate_reg_0[60]),.o(intermediate_reg_1[30])); 
xor_module xor_module_inst_1_35(.clk(clk),.reset(reset),.i1(intermediate_reg_0[59]),.i2(intermediate_reg_0[58]),.o(intermediate_reg_1[29])); 
xor_module xor_module_inst_1_36(.clk(clk),.reset(reset),.i1(intermediate_reg_0[57]),.i2(intermediate_reg_0[56]),.o(intermediate_reg_1[28])); 
xor_module xor_module_inst_1_37(.clk(clk),.reset(reset),.i1(intermediate_reg_0[55]),.i2(intermediate_reg_0[54]),.o(intermediate_reg_1[27])); 
xor_module xor_module_inst_1_38(.clk(clk),.reset(reset),.i1(intermediate_reg_0[53]),.i2(intermediate_reg_0[52]),.o(intermediate_reg_1[26])); 
mux_module mux_module_inst_1_39(.clk(clk),.reset(reset),.i1(intermediate_reg_0[51]),.i2(intermediate_reg_0[50]),.o(intermediate_reg_1[25]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_40(.clk(clk),.reset(reset),.i1(intermediate_reg_0[49]),.i2(intermediate_reg_0[48]),.o(intermediate_reg_1[24]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_41(.clk(clk),.reset(reset),.i1(intermediate_reg_0[47]),.i2(intermediate_reg_0[46]),.o(intermediate_reg_1[23])); 
mux_module mux_module_inst_1_42(.clk(clk),.reset(reset),.i1(intermediate_reg_0[45]),.i2(intermediate_reg_0[44]),.o(intermediate_reg_1[22]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_43(.clk(clk),.reset(reset),.i1(intermediate_reg_0[43]),.i2(intermediate_reg_0[42]),.o(intermediate_reg_1[21])); 
xor_module xor_module_inst_1_44(.clk(clk),.reset(reset),.i1(intermediate_reg_0[41]),.i2(intermediate_reg_0[40]),.o(intermediate_reg_1[20])); 
mux_module mux_module_inst_1_45(.clk(clk),.reset(reset),.i1(intermediate_reg_0[39]),.i2(intermediate_reg_0[38]),.o(intermediate_reg_1[19]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_46(.clk(clk),.reset(reset),.i1(intermediate_reg_0[37]),.i2(intermediate_reg_0[36]),.o(intermediate_reg_1[18]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_47(.clk(clk),.reset(reset),.i1(intermediate_reg_0[35]),.i2(intermediate_reg_0[34]),.o(intermediate_reg_1[17]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_48(.clk(clk),.reset(reset),.i1(intermediate_reg_0[33]),.i2(intermediate_reg_0[32]),.o(intermediate_reg_1[16]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_49(.clk(clk),.reset(reset),.i1(intermediate_reg_0[31]),.i2(intermediate_reg_0[30]),.o(intermediate_reg_1[15])); 
xor_module xor_module_inst_1_50(.clk(clk),.reset(reset),.i1(intermediate_reg_0[29]),.i2(intermediate_reg_0[28]),.o(intermediate_reg_1[14])); 
xor_module xor_module_inst_1_51(.clk(clk),.reset(reset),.i1(intermediate_reg_0[27]),.i2(intermediate_reg_0[26]),.o(intermediate_reg_1[13])); 
xor_module xor_module_inst_1_52(.clk(clk),.reset(reset),.i1(intermediate_reg_0[25]),.i2(intermediate_reg_0[24]),.o(intermediate_reg_1[12])); 
mux_module mux_module_inst_1_53(.clk(clk),.reset(reset),.i1(intermediate_reg_0[23]),.i2(intermediate_reg_0[22]),.o(intermediate_reg_1[11]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_54(.clk(clk),.reset(reset),.i1(intermediate_reg_0[21]),.i2(intermediate_reg_0[20]),.o(intermediate_reg_1[10])); 
mux_module mux_module_inst_1_55(.clk(clk),.reset(reset),.i1(intermediate_reg_0[19]),.i2(intermediate_reg_0[18]),.o(intermediate_reg_1[9]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_56(.clk(clk),.reset(reset),.i1(intermediate_reg_0[17]),.i2(intermediate_reg_0[16]),.o(intermediate_reg_1[8]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_57(.clk(clk),.reset(reset),.i1(intermediate_reg_0[15]),.i2(intermediate_reg_0[14]),.o(intermediate_reg_1[7]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_58(.clk(clk),.reset(reset),.i1(intermediate_reg_0[13]),.i2(intermediate_reg_0[12]),.o(intermediate_reg_1[6]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_59(.clk(clk),.reset(reset),.i1(intermediate_reg_0[11]),.i2(intermediate_reg_0[10]),.o(intermediate_reg_1[5]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_60(.clk(clk),.reset(reset),.i1(intermediate_reg_0[9]),.i2(intermediate_reg_0[8]),.o(intermediate_reg_1[4]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_61(.clk(clk),.reset(reset),.i1(intermediate_reg_0[7]),.i2(intermediate_reg_0[6]),.o(intermediate_reg_1[3]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_62(.clk(clk),.reset(reset),.i1(intermediate_reg_0[5]),.i2(intermediate_reg_0[4]),.o(intermediate_reg_1[2])); 
xor_module xor_module_inst_1_63(.clk(clk),.reset(reset),.i1(intermediate_reg_0[3]),.i2(intermediate_reg_0[2]),.o(intermediate_reg_1[1])); 
mux_module mux_module_inst_1_64(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1]),.i2(intermediate_reg_0[0]),.o(intermediate_reg_1[0]),.sel(intermediate_reg_0[0])); 
wire [63:0]intermediate_wire_2; 
assign intermediate_wire_2[63] = intermediate_reg_1[64]^intermediate_reg_1[63] ; 
assign intermediate_wire_2[62:0] = intermediate_reg_1[62:0] ; 
always@(posedge clk) begin 
outp[63:0] <= intermediate_wire_2 ; 
end 
endmodule 
 

module interface_11(input [1119:0] inp, output reg [383:0] outp, input clk, input reset);
reg [1119:0]intermediate_reg_0; 
always@(posedge clk) begin 
intermediate_reg_0 <= inp; 
end 
 
wire [559:0]intermediate_reg_1; 
 
xor_module xor_module_inst_1_0(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1119]),.i2(intermediate_reg_0[1118]),.o(intermediate_reg_1[559])); 
xor_module xor_module_inst_1_1(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1117]),.i2(intermediate_reg_0[1116]),.o(intermediate_reg_1[558])); 
xor_module xor_module_inst_1_2(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1115]),.i2(intermediate_reg_0[1114]),.o(intermediate_reg_1[557])); 
xor_module xor_module_inst_1_3(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1113]),.i2(intermediate_reg_0[1112]),.o(intermediate_reg_1[556])); 
mux_module mux_module_inst_1_4(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1111]),.i2(intermediate_reg_0[1110]),.o(intermediate_reg_1[555]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_5(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1109]),.i2(intermediate_reg_0[1108]),.o(intermediate_reg_1[554])); 
mux_module mux_module_inst_1_6(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1107]),.i2(intermediate_reg_0[1106]),.o(intermediate_reg_1[553]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_7(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1105]),.i2(intermediate_reg_0[1104]),.o(intermediate_reg_1[552])); 
xor_module xor_module_inst_1_8(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1103]),.i2(intermediate_reg_0[1102]),.o(intermediate_reg_1[551])); 
xor_module xor_module_inst_1_9(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1101]),.i2(intermediate_reg_0[1100]),.o(intermediate_reg_1[550])); 
mux_module mux_module_inst_1_10(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1099]),.i2(intermediate_reg_0[1098]),.o(intermediate_reg_1[549]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_11(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1097]),.i2(intermediate_reg_0[1096]),.o(intermediate_reg_1[548]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_12(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1095]),.i2(intermediate_reg_0[1094]),.o(intermediate_reg_1[547])); 
xor_module xor_module_inst_1_13(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1093]),.i2(intermediate_reg_0[1092]),.o(intermediate_reg_1[546])); 
mux_module mux_module_inst_1_14(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1091]),.i2(intermediate_reg_0[1090]),.o(intermediate_reg_1[545]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_15(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1089]),.i2(intermediate_reg_0[1088]),.o(intermediate_reg_1[544])); 
mux_module mux_module_inst_1_16(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1087]),.i2(intermediate_reg_0[1086]),.o(intermediate_reg_1[543]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_17(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1085]),.i2(intermediate_reg_0[1084]),.o(intermediate_reg_1[542]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_18(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1083]),.i2(intermediate_reg_0[1082]),.o(intermediate_reg_1[541]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_19(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1081]),.i2(intermediate_reg_0[1080]),.o(intermediate_reg_1[540])); 
xor_module xor_module_inst_1_20(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1079]),.i2(intermediate_reg_0[1078]),.o(intermediate_reg_1[539])); 
mux_module mux_module_inst_1_21(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1077]),.i2(intermediate_reg_0[1076]),.o(intermediate_reg_1[538]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_22(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1075]),.i2(intermediate_reg_0[1074]),.o(intermediate_reg_1[537]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_23(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1073]),.i2(intermediate_reg_0[1072]),.o(intermediate_reg_1[536]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_24(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1071]),.i2(intermediate_reg_0[1070]),.o(intermediate_reg_1[535]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_25(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1069]),.i2(intermediate_reg_0[1068]),.o(intermediate_reg_1[534])); 
xor_module xor_module_inst_1_26(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1067]),.i2(intermediate_reg_0[1066]),.o(intermediate_reg_1[533])); 
xor_module xor_module_inst_1_27(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1065]),.i2(intermediate_reg_0[1064]),.o(intermediate_reg_1[532])); 
xor_module xor_module_inst_1_28(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1063]),.i2(intermediate_reg_0[1062]),.o(intermediate_reg_1[531])); 
mux_module mux_module_inst_1_29(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1061]),.i2(intermediate_reg_0[1060]),.o(intermediate_reg_1[530]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_30(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1059]),.i2(intermediate_reg_0[1058]),.o(intermediate_reg_1[529])); 
mux_module mux_module_inst_1_31(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1057]),.i2(intermediate_reg_0[1056]),.o(intermediate_reg_1[528]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_32(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1055]),.i2(intermediate_reg_0[1054]),.o(intermediate_reg_1[527]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_33(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1053]),.i2(intermediate_reg_0[1052]),.o(intermediate_reg_1[526]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_34(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1051]),.i2(intermediate_reg_0[1050]),.o(intermediate_reg_1[525])); 
xor_module xor_module_inst_1_35(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1049]),.i2(intermediate_reg_0[1048]),.o(intermediate_reg_1[524])); 
xor_module xor_module_inst_1_36(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1047]),.i2(intermediate_reg_0[1046]),.o(intermediate_reg_1[523])); 
mux_module mux_module_inst_1_37(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1045]),.i2(intermediate_reg_0[1044]),.o(intermediate_reg_1[522]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_38(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1043]),.i2(intermediate_reg_0[1042]),.o(intermediate_reg_1[521]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_39(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1041]),.i2(intermediate_reg_0[1040]),.o(intermediate_reg_1[520]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_40(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1039]),.i2(intermediate_reg_0[1038]),.o(intermediate_reg_1[519])); 
mux_module mux_module_inst_1_41(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1037]),.i2(intermediate_reg_0[1036]),.o(intermediate_reg_1[518]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_42(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1035]),.i2(intermediate_reg_0[1034]),.o(intermediate_reg_1[517]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_43(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1033]),.i2(intermediate_reg_0[1032]),.o(intermediate_reg_1[516])); 
mux_module mux_module_inst_1_44(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1031]),.i2(intermediate_reg_0[1030]),.o(intermediate_reg_1[515]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_45(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1029]),.i2(intermediate_reg_0[1028]),.o(intermediate_reg_1[514])); 
mux_module mux_module_inst_1_46(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1027]),.i2(intermediate_reg_0[1026]),.o(intermediate_reg_1[513]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_47(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1025]),.i2(intermediate_reg_0[1024]),.o(intermediate_reg_1[512])); 
xor_module xor_module_inst_1_48(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1023]),.i2(intermediate_reg_0[1022]),.o(intermediate_reg_1[511])); 
xor_module xor_module_inst_1_49(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1021]),.i2(intermediate_reg_0[1020]),.o(intermediate_reg_1[510])); 
xor_module xor_module_inst_1_50(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1019]),.i2(intermediate_reg_0[1018]),.o(intermediate_reg_1[509])); 
mux_module mux_module_inst_1_51(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1017]),.i2(intermediate_reg_0[1016]),.o(intermediate_reg_1[508]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_52(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1015]),.i2(intermediate_reg_0[1014]),.o(intermediate_reg_1[507])); 
mux_module mux_module_inst_1_53(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1013]),.i2(intermediate_reg_0[1012]),.o(intermediate_reg_1[506]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_54(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1011]),.i2(intermediate_reg_0[1010]),.o(intermediate_reg_1[505])); 
mux_module mux_module_inst_1_55(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1009]),.i2(intermediate_reg_0[1008]),.o(intermediate_reg_1[504]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_56(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1007]),.i2(intermediate_reg_0[1006]),.o(intermediate_reg_1[503])); 
mux_module mux_module_inst_1_57(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1005]),.i2(intermediate_reg_0[1004]),.o(intermediate_reg_1[502]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_58(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1003]),.i2(intermediate_reg_0[1002]),.o(intermediate_reg_1[501]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_59(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1001]),.i2(intermediate_reg_0[1000]),.o(intermediate_reg_1[500]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_60(.clk(clk),.reset(reset),.i1(intermediate_reg_0[999]),.i2(intermediate_reg_0[998]),.o(intermediate_reg_1[499]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_61(.clk(clk),.reset(reset),.i1(intermediate_reg_0[997]),.i2(intermediate_reg_0[996]),.o(intermediate_reg_1[498])); 
xor_module xor_module_inst_1_62(.clk(clk),.reset(reset),.i1(intermediate_reg_0[995]),.i2(intermediate_reg_0[994]),.o(intermediate_reg_1[497])); 
xor_module xor_module_inst_1_63(.clk(clk),.reset(reset),.i1(intermediate_reg_0[993]),.i2(intermediate_reg_0[992]),.o(intermediate_reg_1[496])); 
mux_module mux_module_inst_1_64(.clk(clk),.reset(reset),.i1(intermediate_reg_0[991]),.i2(intermediate_reg_0[990]),.o(intermediate_reg_1[495]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_65(.clk(clk),.reset(reset),.i1(intermediate_reg_0[989]),.i2(intermediate_reg_0[988]),.o(intermediate_reg_1[494])); 
xor_module xor_module_inst_1_66(.clk(clk),.reset(reset),.i1(intermediate_reg_0[987]),.i2(intermediate_reg_0[986]),.o(intermediate_reg_1[493])); 
xor_module xor_module_inst_1_67(.clk(clk),.reset(reset),.i1(intermediate_reg_0[985]),.i2(intermediate_reg_0[984]),.o(intermediate_reg_1[492])); 
xor_module xor_module_inst_1_68(.clk(clk),.reset(reset),.i1(intermediate_reg_0[983]),.i2(intermediate_reg_0[982]),.o(intermediate_reg_1[491])); 
xor_module xor_module_inst_1_69(.clk(clk),.reset(reset),.i1(intermediate_reg_0[981]),.i2(intermediate_reg_0[980]),.o(intermediate_reg_1[490])); 
xor_module xor_module_inst_1_70(.clk(clk),.reset(reset),.i1(intermediate_reg_0[979]),.i2(intermediate_reg_0[978]),.o(intermediate_reg_1[489])); 
xor_module xor_module_inst_1_71(.clk(clk),.reset(reset),.i1(intermediate_reg_0[977]),.i2(intermediate_reg_0[976]),.o(intermediate_reg_1[488])); 
xor_module xor_module_inst_1_72(.clk(clk),.reset(reset),.i1(intermediate_reg_0[975]),.i2(intermediate_reg_0[974]),.o(intermediate_reg_1[487])); 
mux_module mux_module_inst_1_73(.clk(clk),.reset(reset),.i1(intermediate_reg_0[973]),.i2(intermediate_reg_0[972]),.o(intermediate_reg_1[486]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_74(.clk(clk),.reset(reset),.i1(intermediate_reg_0[971]),.i2(intermediate_reg_0[970]),.o(intermediate_reg_1[485]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_75(.clk(clk),.reset(reset),.i1(intermediate_reg_0[969]),.i2(intermediate_reg_0[968]),.o(intermediate_reg_1[484])); 
xor_module xor_module_inst_1_76(.clk(clk),.reset(reset),.i1(intermediate_reg_0[967]),.i2(intermediate_reg_0[966]),.o(intermediate_reg_1[483])); 
xor_module xor_module_inst_1_77(.clk(clk),.reset(reset),.i1(intermediate_reg_0[965]),.i2(intermediate_reg_0[964]),.o(intermediate_reg_1[482])); 
xor_module xor_module_inst_1_78(.clk(clk),.reset(reset),.i1(intermediate_reg_0[963]),.i2(intermediate_reg_0[962]),.o(intermediate_reg_1[481])); 
mux_module mux_module_inst_1_79(.clk(clk),.reset(reset),.i1(intermediate_reg_0[961]),.i2(intermediate_reg_0[960]),.o(intermediate_reg_1[480]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_80(.clk(clk),.reset(reset),.i1(intermediate_reg_0[959]),.i2(intermediate_reg_0[958]),.o(intermediate_reg_1[479]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_81(.clk(clk),.reset(reset),.i1(intermediate_reg_0[957]),.i2(intermediate_reg_0[956]),.o(intermediate_reg_1[478]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_82(.clk(clk),.reset(reset),.i1(intermediate_reg_0[955]),.i2(intermediate_reg_0[954]),.o(intermediate_reg_1[477])); 
mux_module mux_module_inst_1_83(.clk(clk),.reset(reset),.i1(intermediate_reg_0[953]),.i2(intermediate_reg_0[952]),.o(intermediate_reg_1[476]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_84(.clk(clk),.reset(reset),.i1(intermediate_reg_0[951]),.i2(intermediate_reg_0[950]),.o(intermediate_reg_1[475]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_85(.clk(clk),.reset(reset),.i1(intermediate_reg_0[949]),.i2(intermediate_reg_0[948]),.o(intermediate_reg_1[474])); 
mux_module mux_module_inst_1_86(.clk(clk),.reset(reset),.i1(intermediate_reg_0[947]),.i2(intermediate_reg_0[946]),.o(intermediate_reg_1[473]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_87(.clk(clk),.reset(reset),.i1(intermediate_reg_0[945]),.i2(intermediate_reg_0[944]),.o(intermediate_reg_1[472]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_88(.clk(clk),.reset(reset),.i1(intermediate_reg_0[943]),.i2(intermediate_reg_0[942]),.o(intermediate_reg_1[471])); 
xor_module xor_module_inst_1_89(.clk(clk),.reset(reset),.i1(intermediate_reg_0[941]),.i2(intermediate_reg_0[940]),.o(intermediate_reg_1[470])); 
mux_module mux_module_inst_1_90(.clk(clk),.reset(reset),.i1(intermediate_reg_0[939]),.i2(intermediate_reg_0[938]),.o(intermediate_reg_1[469]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_91(.clk(clk),.reset(reset),.i1(intermediate_reg_0[937]),.i2(intermediate_reg_0[936]),.o(intermediate_reg_1[468]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_92(.clk(clk),.reset(reset),.i1(intermediate_reg_0[935]),.i2(intermediate_reg_0[934]),.o(intermediate_reg_1[467])); 
mux_module mux_module_inst_1_93(.clk(clk),.reset(reset),.i1(intermediate_reg_0[933]),.i2(intermediate_reg_0[932]),.o(intermediate_reg_1[466]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_94(.clk(clk),.reset(reset),.i1(intermediate_reg_0[931]),.i2(intermediate_reg_0[930]),.o(intermediate_reg_1[465]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_95(.clk(clk),.reset(reset),.i1(intermediate_reg_0[929]),.i2(intermediate_reg_0[928]),.o(intermediate_reg_1[464]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_96(.clk(clk),.reset(reset),.i1(intermediate_reg_0[927]),.i2(intermediate_reg_0[926]),.o(intermediate_reg_1[463])); 
mux_module mux_module_inst_1_97(.clk(clk),.reset(reset),.i1(intermediate_reg_0[925]),.i2(intermediate_reg_0[924]),.o(intermediate_reg_1[462]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_98(.clk(clk),.reset(reset),.i1(intermediate_reg_0[923]),.i2(intermediate_reg_0[922]),.o(intermediate_reg_1[461]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_99(.clk(clk),.reset(reset),.i1(intermediate_reg_0[921]),.i2(intermediate_reg_0[920]),.o(intermediate_reg_1[460])); 
mux_module mux_module_inst_1_100(.clk(clk),.reset(reset),.i1(intermediate_reg_0[919]),.i2(intermediate_reg_0[918]),.o(intermediate_reg_1[459]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_101(.clk(clk),.reset(reset),.i1(intermediate_reg_0[917]),.i2(intermediate_reg_0[916]),.o(intermediate_reg_1[458]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_102(.clk(clk),.reset(reset),.i1(intermediate_reg_0[915]),.i2(intermediate_reg_0[914]),.o(intermediate_reg_1[457])); 
mux_module mux_module_inst_1_103(.clk(clk),.reset(reset),.i1(intermediate_reg_0[913]),.i2(intermediate_reg_0[912]),.o(intermediate_reg_1[456]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_104(.clk(clk),.reset(reset),.i1(intermediate_reg_0[911]),.i2(intermediate_reg_0[910]),.o(intermediate_reg_1[455]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_105(.clk(clk),.reset(reset),.i1(intermediate_reg_0[909]),.i2(intermediate_reg_0[908]),.o(intermediate_reg_1[454])); 
xor_module xor_module_inst_1_106(.clk(clk),.reset(reset),.i1(intermediate_reg_0[907]),.i2(intermediate_reg_0[906]),.o(intermediate_reg_1[453])); 
xor_module xor_module_inst_1_107(.clk(clk),.reset(reset),.i1(intermediate_reg_0[905]),.i2(intermediate_reg_0[904]),.o(intermediate_reg_1[452])); 
xor_module xor_module_inst_1_108(.clk(clk),.reset(reset),.i1(intermediate_reg_0[903]),.i2(intermediate_reg_0[902]),.o(intermediate_reg_1[451])); 
mux_module mux_module_inst_1_109(.clk(clk),.reset(reset),.i1(intermediate_reg_0[901]),.i2(intermediate_reg_0[900]),.o(intermediate_reg_1[450]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_110(.clk(clk),.reset(reset),.i1(intermediate_reg_0[899]),.i2(intermediate_reg_0[898]),.o(intermediate_reg_1[449])); 
xor_module xor_module_inst_1_111(.clk(clk),.reset(reset),.i1(intermediate_reg_0[897]),.i2(intermediate_reg_0[896]),.o(intermediate_reg_1[448])); 
xor_module xor_module_inst_1_112(.clk(clk),.reset(reset),.i1(intermediate_reg_0[895]),.i2(intermediate_reg_0[894]),.o(intermediate_reg_1[447])); 
mux_module mux_module_inst_1_113(.clk(clk),.reset(reset),.i1(intermediate_reg_0[893]),.i2(intermediate_reg_0[892]),.o(intermediate_reg_1[446]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_114(.clk(clk),.reset(reset),.i1(intermediate_reg_0[891]),.i2(intermediate_reg_0[890]),.o(intermediate_reg_1[445]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_115(.clk(clk),.reset(reset),.i1(intermediate_reg_0[889]),.i2(intermediate_reg_0[888]),.o(intermediate_reg_1[444])); 
mux_module mux_module_inst_1_116(.clk(clk),.reset(reset),.i1(intermediate_reg_0[887]),.i2(intermediate_reg_0[886]),.o(intermediate_reg_1[443]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_117(.clk(clk),.reset(reset),.i1(intermediate_reg_0[885]),.i2(intermediate_reg_0[884]),.o(intermediate_reg_1[442])); 
xor_module xor_module_inst_1_118(.clk(clk),.reset(reset),.i1(intermediate_reg_0[883]),.i2(intermediate_reg_0[882]),.o(intermediate_reg_1[441])); 
xor_module xor_module_inst_1_119(.clk(clk),.reset(reset),.i1(intermediate_reg_0[881]),.i2(intermediate_reg_0[880]),.o(intermediate_reg_1[440])); 
xor_module xor_module_inst_1_120(.clk(clk),.reset(reset),.i1(intermediate_reg_0[879]),.i2(intermediate_reg_0[878]),.o(intermediate_reg_1[439])); 
xor_module xor_module_inst_1_121(.clk(clk),.reset(reset),.i1(intermediate_reg_0[877]),.i2(intermediate_reg_0[876]),.o(intermediate_reg_1[438])); 
mux_module mux_module_inst_1_122(.clk(clk),.reset(reset),.i1(intermediate_reg_0[875]),.i2(intermediate_reg_0[874]),.o(intermediate_reg_1[437]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_123(.clk(clk),.reset(reset),.i1(intermediate_reg_0[873]),.i2(intermediate_reg_0[872]),.o(intermediate_reg_1[436])); 
xor_module xor_module_inst_1_124(.clk(clk),.reset(reset),.i1(intermediate_reg_0[871]),.i2(intermediate_reg_0[870]),.o(intermediate_reg_1[435])); 
xor_module xor_module_inst_1_125(.clk(clk),.reset(reset),.i1(intermediate_reg_0[869]),.i2(intermediate_reg_0[868]),.o(intermediate_reg_1[434])); 
xor_module xor_module_inst_1_126(.clk(clk),.reset(reset),.i1(intermediate_reg_0[867]),.i2(intermediate_reg_0[866]),.o(intermediate_reg_1[433])); 
mux_module mux_module_inst_1_127(.clk(clk),.reset(reset),.i1(intermediate_reg_0[865]),.i2(intermediate_reg_0[864]),.o(intermediate_reg_1[432]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_128(.clk(clk),.reset(reset),.i1(intermediate_reg_0[863]),.i2(intermediate_reg_0[862]),.o(intermediate_reg_1[431])); 
xor_module xor_module_inst_1_129(.clk(clk),.reset(reset),.i1(intermediate_reg_0[861]),.i2(intermediate_reg_0[860]),.o(intermediate_reg_1[430])); 
xor_module xor_module_inst_1_130(.clk(clk),.reset(reset),.i1(intermediate_reg_0[859]),.i2(intermediate_reg_0[858]),.o(intermediate_reg_1[429])); 
mux_module mux_module_inst_1_131(.clk(clk),.reset(reset),.i1(intermediate_reg_0[857]),.i2(intermediate_reg_0[856]),.o(intermediate_reg_1[428]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_132(.clk(clk),.reset(reset),.i1(intermediate_reg_0[855]),.i2(intermediate_reg_0[854]),.o(intermediate_reg_1[427])); 
xor_module xor_module_inst_1_133(.clk(clk),.reset(reset),.i1(intermediate_reg_0[853]),.i2(intermediate_reg_0[852]),.o(intermediate_reg_1[426])); 
xor_module xor_module_inst_1_134(.clk(clk),.reset(reset),.i1(intermediate_reg_0[851]),.i2(intermediate_reg_0[850]),.o(intermediate_reg_1[425])); 
mux_module mux_module_inst_1_135(.clk(clk),.reset(reset),.i1(intermediate_reg_0[849]),.i2(intermediate_reg_0[848]),.o(intermediate_reg_1[424]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_136(.clk(clk),.reset(reset),.i1(intermediate_reg_0[847]),.i2(intermediate_reg_0[846]),.o(intermediate_reg_1[423]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_137(.clk(clk),.reset(reset),.i1(intermediate_reg_0[845]),.i2(intermediate_reg_0[844]),.o(intermediate_reg_1[422])); 
mux_module mux_module_inst_1_138(.clk(clk),.reset(reset),.i1(intermediate_reg_0[843]),.i2(intermediate_reg_0[842]),.o(intermediate_reg_1[421]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_139(.clk(clk),.reset(reset),.i1(intermediate_reg_0[841]),.i2(intermediate_reg_0[840]),.o(intermediate_reg_1[420])); 
xor_module xor_module_inst_1_140(.clk(clk),.reset(reset),.i1(intermediate_reg_0[839]),.i2(intermediate_reg_0[838]),.o(intermediate_reg_1[419])); 
xor_module xor_module_inst_1_141(.clk(clk),.reset(reset),.i1(intermediate_reg_0[837]),.i2(intermediate_reg_0[836]),.o(intermediate_reg_1[418])); 
mux_module mux_module_inst_1_142(.clk(clk),.reset(reset),.i1(intermediate_reg_0[835]),.i2(intermediate_reg_0[834]),.o(intermediate_reg_1[417]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_143(.clk(clk),.reset(reset),.i1(intermediate_reg_0[833]),.i2(intermediate_reg_0[832]),.o(intermediate_reg_1[416])); 
xor_module xor_module_inst_1_144(.clk(clk),.reset(reset),.i1(intermediate_reg_0[831]),.i2(intermediate_reg_0[830]),.o(intermediate_reg_1[415])); 
mux_module mux_module_inst_1_145(.clk(clk),.reset(reset),.i1(intermediate_reg_0[829]),.i2(intermediate_reg_0[828]),.o(intermediate_reg_1[414]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_146(.clk(clk),.reset(reset),.i1(intermediate_reg_0[827]),.i2(intermediate_reg_0[826]),.o(intermediate_reg_1[413]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_147(.clk(clk),.reset(reset),.i1(intermediate_reg_0[825]),.i2(intermediate_reg_0[824]),.o(intermediate_reg_1[412])); 
mux_module mux_module_inst_1_148(.clk(clk),.reset(reset),.i1(intermediate_reg_0[823]),.i2(intermediate_reg_0[822]),.o(intermediate_reg_1[411]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_149(.clk(clk),.reset(reset),.i1(intermediate_reg_0[821]),.i2(intermediate_reg_0[820]),.o(intermediate_reg_1[410])); 
xor_module xor_module_inst_1_150(.clk(clk),.reset(reset),.i1(intermediate_reg_0[819]),.i2(intermediate_reg_0[818]),.o(intermediate_reg_1[409])); 
mux_module mux_module_inst_1_151(.clk(clk),.reset(reset),.i1(intermediate_reg_0[817]),.i2(intermediate_reg_0[816]),.o(intermediate_reg_1[408]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_152(.clk(clk),.reset(reset),.i1(intermediate_reg_0[815]),.i2(intermediate_reg_0[814]),.o(intermediate_reg_1[407]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_153(.clk(clk),.reset(reset),.i1(intermediate_reg_0[813]),.i2(intermediate_reg_0[812]),.o(intermediate_reg_1[406]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_154(.clk(clk),.reset(reset),.i1(intermediate_reg_0[811]),.i2(intermediate_reg_0[810]),.o(intermediate_reg_1[405])); 
mux_module mux_module_inst_1_155(.clk(clk),.reset(reset),.i1(intermediate_reg_0[809]),.i2(intermediate_reg_0[808]),.o(intermediate_reg_1[404]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_156(.clk(clk),.reset(reset),.i1(intermediate_reg_0[807]),.i2(intermediate_reg_0[806]),.o(intermediate_reg_1[403])); 
mux_module mux_module_inst_1_157(.clk(clk),.reset(reset),.i1(intermediate_reg_0[805]),.i2(intermediate_reg_0[804]),.o(intermediate_reg_1[402]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_158(.clk(clk),.reset(reset),.i1(intermediate_reg_0[803]),.i2(intermediate_reg_0[802]),.o(intermediate_reg_1[401]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_159(.clk(clk),.reset(reset),.i1(intermediate_reg_0[801]),.i2(intermediate_reg_0[800]),.o(intermediate_reg_1[400])); 
xor_module xor_module_inst_1_160(.clk(clk),.reset(reset),.i1(intermediate_reg_0[799]),.i2(intermediate_reg_0[798]),.o(intermediate_reg_1[399])); 
mux_module mux_module_inst_1_161(.clk(clk),.reset(reset),.i1(intermediate_reg_0[797]),.i2(intermediate_reg_0[796]),.o(intermediate_reg_1[398]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_162(.clk(clk),.reset(reset),.i1(intermediate_reg_0[795]),.i2(intermediate_reg_0[794]),.o(intermediate_reg_1[397])); 
xor_module xor_module_inst_1_163(.clk(clk),.reset(reset),.i1(intermediate_reg_0[793]),.i2(intermediate_reg_0[792]),.o(intermediate_reg_1[396])); 
xor_module xor_module_inst_1_164(.clk(clk),.reset(reset),.i1(intermediate_reg_0[791]),.i2(intermediate_reg_0[790]),.o(intermediate_reg_1[395])); 
mux_module mux_module_inst_1_165(.clk(clk),.reset(reset),.i1(intermediate_reg_0[789]),.i2(intermediate_reg_0[788]),.o(intermediate_reg_1[394]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_166(.clk(clk),.reset(reset),.i1(intermediate_reg_0[787]),.i2(intermediate_reg_0[786]),.o(intermediate_reg_1[393]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_167(.clk(clk),.reset(reset),.i1(intermediate_reg_0[785]),.i2(intermediate_reg_0[784]),.o(intermediate_reg_1[392]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_168(.clk(clk),.reset(reset),.i1(intermediate_reg_0[783]),.i2(intermediate_reg_0[782]),.o(intermediate_reg_1[391])); 
xor_module xor_module_inst_1_169(.clk(clk),.reset(reset),.i1(intermediate_reg_0[781]),.i2(intermediate_reg_0[780]),.o(intermediate_reg_1[390])); 
mux_module mux_module_inst_1_170(.clk(clk),.reset(reset),.i1(intermediate_reg_0[779]),.i2(intermediate_reg_0[778]),.o(intermediate_reg_1[389]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_171(.clk(clk),.reset(reset),.i1(intermediate_reg_0[777]),.i2(intermediate_reg_0[776]),.o(intermediate_reg_1[388]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_172(.clk(clk),.reset(reset),.i1(intermediate_reg_0[775]),.i2(intermediate_reg_0[774]),.o(intermediate_reg_1[387])); 
mux_module mux_module_inst_1_173(.clk(clk),.reset(reset),.i1(intermediate_reg_0[773]),.i2(intermediate_reg_0[772]),.o(intermediate_reg_1[386]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_174(.clk(clk),.reset(reset),.i1(intermediate_reg_0[771]),.i2(intermediate_reg_0[770]),.o(intermediate_reg_1[385]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_175(.clk(clk),.reset(reset),.i1(intermediate_reg_0[769]),.i2(intermediate_reg_0[768]),.o(intermediate_reg_1[384])); 
mux_module mux_module_inst_1_176(.clk(clk),.reset(reset),.i1(intermediate_reg_0[767]),.i2(intermediate_reg_0[766]),.o(intermediate_reg_1[383]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_177(.clk(clk),.reset(reset),.i1(intermediate_reg_0[765]),.i2(intermediate_reg_0[764]),.o(intermediate_reg_1[382]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_178(.clk(clk),.reset(reset),.i1(intermediate_reg_0[763]),.i2(intermediate_reg_0[762]),.o(intermediate_reg_1[381])); 
xor_module xor_module_inst_1_179(.clk(clk),.reset(reset),.i1(intermediate_reg_0[761]),.i2(intermediate_reg_0[760]),.o(intermediate_reg_1[380])); 
xor_module xor_module_inst_1_180(.clk(clk),.reset(reset),.i1(intermediate_reg_0[759]),.i2(intermediate_reg_0[758]),.o(intermediate_reg_1[379])); 
mux_module mux_module_inst_1_181(.clk(clk),.reset(reset),.i1(intermediate_reg_0[757]),.i2(intermediate_reg_0[756]),.o(intermediate_reg_1[378]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_182(.clk(clk),.reset(reset),.i1(intermediate_reg_0[755]),.i2(intermediate_reg_0[754]),.o(intermediate_reg_1[377])); 
mux_module mux_module_inst_1_183(.clk(clk),.reset(reset),.i1(intermediate_reg_0[753]),.i2(intermediate_reg_0[752]),.o(intermediate_reg_1[376]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_184(.clk(clk),.reset(reset),.i1(intermediate_reg_0[751]),.i2(intermediate_reg_0[750]),.o(intermediate_reg_1[375]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_185(.clk(clk),.reset(reset),.i1(intermediate_reg_0[749]),.i2(intermediate_reg_0[748]),.o(intermediate_reg_1[374]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_186(.clk(clk),.reset(reset),.i1(intermediate_reg_0[747]),.i2(intermediate_reg_0[746]),.o(intermediate_reg_1[373])); 
mux_module mux_module_inst_1_187(.clk(clk),.reset(reset),.i1(intermediate_reg_0[745]),.i2(intermediate_reg_0[744]),.o(intermediate_reg_1[372]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_188(.clk(clk),.reset(reset),.i1(intermediate_reg_0[743]),.i2(intermediate_reg_0[742]),.o(intermediate_reg_1[371]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_189(.clk(clk),.reset(reset),.i1(intermediate_reg_0[741]),.i2(intermediate_reg_0[740]),.o(intermediate_reg_1[370])); 
mux_module mux_module_inst_1_190(.clk(clk),.reset(reset),.i1(intermediate_reg_0[739]),.i2(intermediate_reg_0[738]),.o(intermediate_reg_1[369]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_191(.clk(clk),.reset(reset),.i1(intermediate_reg_0[737]),.i2(intermediate_reg_0[736]),.o(intermediate_reg_1[368]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_192(.clk(clk),.reset(reset),.i1(intermediate_reg_0[735]),.i2(intermediate_reg_0[734]),.o(intermediate_reg_1[367]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_193(.clk(clk),.reset(reset),.i1(intermediate_reg_0[733]),.i2(intermediate_reg_0[732]),.o(intermediate_reg_1[366])); 
xor_module xor_module_inst_1_194(.clk(clk),.reset(reset),.i1(intermediate_reg_0[731]),.i2(intermediate_reg_0[730]),.o(intermediate_reg_1[365])); 
xor_module xor_module_inst_1_195(.clk(clk),.reset(reset),.i1(intermediate_reg_0[729]),.i2(intermediate_reg_0[728]),.o(intermediate_reg_1[364])); 
mux_module mux_module_inst_1_196(.clk(clk),.reset(reset),.i1(intermediate_reg_0[727]),.i2(intermediate_reg_0[726]),.o(intermediate_reg_1[363]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_197(.clk(clk),.reset(reset),.i1(intermediate_reg_0[725]),.i2(intermediate_reg_0[724]),.o(intermediate_reg_1[362])); 
mux_module mux_module_inst_1_198(.clk(clk),.reset(reset),.i1(intermediate_reg_0[723]),.i2(intermediate_reg_0[722]),.o(intermediate_reg_1[361]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_199(.clk(clk),.reset(reset),.i1(intermediate_reg_0[721]),.i2(intermediate_reg_0[720]),.o(intermediate_reg_1[360]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_200(.clk(clk),.reset(reset),.i1(intermediate_reg_0[719]),.i2(intermediate_reg_0[718]),.o(intermediate_reg_1[359])); 
mux_module mux_module_inst_1_201(.clk(clk),.reset(reset),.i1(intermediate_reg_0[717]),.i2(intermediate_reg_0[716]),.o(intermediate_reg_1[358]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_202(.clk(clk),.reset(reset),.i1(intermediate_reg_0[715]),.i2(intermediate_reg_0[714]),.o(intermediate_reg_1[357])); 
xor_module xor_module_inst_1_203(.clk(clk),.reset(reset),.i1(intermediate_reg_0[713]),.i2(intermediate_reg_0[712]),.o(intermediate_reg_1[356])); 
xor_module xor_module_inst_1_204(.clk(clk),.reset(reset),.i1(intermediate_reg_0[711]),.i2(intermediate_reg_0[710]),.o(intermediate_reg_1[355])); 
xor_module xor_module_inst_1_205(.clk(clk),.reset(reset),.i1(intermediate_reg_0[709]),.i2(intermediate_reg_0[708]),.o(intermediate_reg_1[354])); 
mux_module mux_module_inst_1_206(.clk(clk),.reset(reset),.i1(intermediate_reg_0[707]),.i2(intermediate_reg_0[706]),.o(intermediate_reg_1[353]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_207(.clk(clk),.reset(reset),.i1(intermediate_reg_0[705]),.i2(intermediate_reg_0[704]),.o(intermediate_reg_1[352]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_208(.clk(clk),.reset(reset),.i1(intermediate_reg_0[703]),.i2(intermediate_reg_0[702]),.o(intermediate_reg_1[351])); 
mux_module mux_module_inst_1_209(.clk(clk),.reset(reset),.i1(intermediate_reg_0[701]),.i2(intermediate_reg_0[700]),.o(intermediate_reg_1[350]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_210(.clk(clk),.reset(reset),.i1(intermediate_reg_0[699]),.i2(intermediate_reg_0[698]),.o(intermediate_reg_1[349])); 
xor_module xor_module_inst_1_211(.clk(clk),.reset(reset),.i1(intermediate_reg_0[697]),.i2(intermediate_reg_0[696]),.o(intermediate_reg_1[348])); 
mux_module mux_module_inst_1_212(.clk(clk),.reset(reset),.i1(intermediate_reg_0[695]),.i2(intermediate_reg_0[694]),.o(intermediate_reg_1[347]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_213(.clk(clk),.reset(reset),.i1(intermediate_reg_0[693]),.i2(intermediate_reg_0[692]),.o(intermediate_reg_1[346])); 
mux_module mux_module_inst_1_214(.clk(clk),.reset(reset),.i1(intermediate_reg_0[691]),.i2(intermediate_reg_0[690]),.o(intermediate_reg_1[345]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_215(.clk(clk),.reset(reset),.i1(intermediate_reg_0[689]),.i2(intermediate_reg_0[688]),.o(intermediate_reg_1[344])); 
xor_module xor_module_inst_1_216(.clk(clk),.reset(reset),.i1(intermediate_reg_0[687]),.i2(intermediate_reg_0[686]),.o(intermediate_reg_1[343])); 
xor_module xor_module_inst_1_217(.clk(clk),.reset(reset),.i1(intermediate_reg_0[685]),.i2(intermediate_reg_0[684]),.o(intermediate_reg_1[342])); 
mux_module mux_module_inst_1_218(.clk(clk),.reset(reset),.i1(intermediate_reg_0[683]),.i2(intermediate_reg_0[682]),.o(intermediate_reg_1[341]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_219(.clk(clk),.reset(reset),.i1(intermediate_reg_0[681]),.i2(intermediate_reg_0[680]),.o(intermediate_reg_1[340])); 
mux_module mux_module_inst_1_220(.clk(clk),.reset(reset),.i1(intermediate_reg_0[679]),.i2(intermediate_reg_0[678]),.o(intermediate_reg_1[339]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_221(.clk(clk),.reset(reset),.i1(intermediate_reg_0[677]),.i2(intermediate_reg_0[676]),.o(intermediate_reg_1[338])); 
xor_module xor_module_inst_1_222(.clk(clk),.reset(reset),.i1(intermediate_reg_0[675]),.i2(intermediate_reg_0[674]),.o(intermediate_reg_1[337])); 
mux_module mux_module_inst_1_223(.clk(clk),.reset(reset),.i1(intermediate_reg_0[673]),.i2(intermediate_reg_0[672]),.o(intermediate_reg_1[336]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_224(.clk(clk),.reset(reset),.i1(intermediate_reg_0[671]),.i2(intermediate_reg_0[670]),.o(intermediate_reg_1[335]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_225(.clk(clk),.reset(reset),.i1(intermediate_reg_0[669]),.i2(intermediate_reg_0[668]),.o(intermediate_reg_1[334])); 
xor_module xor_module_inst_1_226(.clk(clk),.reset(reset),.i1(intermediate_reg_0[667]),.i2(intermediate_reg_0[666]),.o(intermediate_reg_1[333])); 
xor_module xor_module_inst_1_227(.clk(clk),.reset(reset),.i1(intermediate_reg_0[665]),.i2(intermediate_reg_0[664]),.o(intermediate_reg_1[332])); 
mux_module mux_module_inst_1_228(.clk(clk),.reset(reset),.i1(intermediate_reg_0[663]),.i2(intermediate_reg_0[662]),.o(intermediate_reg_1[331]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_229(.clk(clk),.reset(reset),.i1(intermediate_reg_0[661]),.i2(intermediate_reg_0[660]),.o(intermediate_reg_1[330])); 
xor_module xor_module_inst_1_230(.clk(clk),.reset(reset),.i1(intermediate_reg_0[659]),.i2(intermediate_reg_0[658]),.o(intermediate_reg_1[329])); 
mux_module mux_module_inst_1_231(.clk(clk),.reset(reset),.i1(intermediate_reg_0[657]),.i2(intermediate_reg_0[656]),.o(intermediate_reg_1[328]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_232(.clk(clk),.reset(reset),.i1(intermediate_reg_0[655]),.i2(intermediate_reg_0[654]),.o(intermediate_reg_1[327])); 
mux_module mux_module_inst_1_233(.clk(clk),.reset(reset),.i1(intermediate_reg_0[653]),.i2(intermediate_reg_0[652]),.o(intermediate_reg_1[326]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_234(.clk(clk),.reset(reset),.i1(intermediate_reg_0[651]),.i2(intermediate_reg_0[650]),.o(intermediate_reg_1[325])); 
mux_module mux_module_inst_1_235(.clk(clk),.reset(reset),.i1(intermediate_reg_0[649]),.i2(intermediate_reg_0[648]),.o(intermediate_reg_1[324]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_236(.clk(clk),.reset(reset),.i1(intermediate_reg_0[647]),.i2(intermediate_reg_0[646]),.o(intermediate_reg_1[323])); 
mux_module mux_module_inst_1_237(.clk(clk),.reset(reset),.i1(intermediate_reg_0[645]),.i2(intermediate_reg_0[644]),.o(intermediate_reg_1[322]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_238(.clk(clk),.reset(reset),.i1(intermediate_reg_0[643]),.i2(intermediate_reg_0[642]),.o(intermediate_reg_1[321]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_239(.clk(clk),.reset(reset),.i1(intermediate_reg_0[641]),.i2(intermediate_reg_0[640]),.o(intermediate_reg_1[320]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_240(.clk(clk),.reset(reset),.i1(intermediate_reg_0[639]),.i2(intermediate_reg_0[638]),.o(intermediate_reg_1[319]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_241(.clk(clk),.reset(reset),.i1(intermediate_reg_0[637]),.i2(intermediate_reg_0[636]),.o(intermediate_reg_1[318])); 
xor_module xor_module_inst_1_242(.clk(clk),.reset(reset),.i1(intermediate_reg_0[635]),.i2(intermediate_reg_0[634]),.o(intermediate_reg_1[317])); 
mux_module mux_module_inst_1_243(.clk(clk),.reset(reset),.i1(intermediate_reg_0[633]),.i2(intermediate_reg_0[632]),.o(intermediate_reg_1[316]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_244(.clk(clk),.reset(reset),.i1(intermediate_reg_0[631]),.i2(intermediate_reg_0[630]),.o(intermediate_reg_1[315]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_245(.clk(clk),.reset(reset),.i1(intermediate_reg_0[629]),.i2(intermediate_reg_0[628]),.o(intermediate_reg_1[314]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_246(.clk(clk),.reset(reset),.i1(intermediate_reg_0[627]),.i2(intermediate_reg_0[626]),.o(intermediate_reg_1[313]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_247(.clk(clk),.reset(reset),.i1(intermediate_reg_0[625]),.i2(intermediate_reg_0[624]),.o(intermediate_reg_1[312])); 
mux_module mux_module_inst_1_248(.clk(clk),.reset(reset),.i1(intermediate_reg_0[623]),.i2(intermediate_reg_0[622]),.o(intermediate_reg_1[311]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_249(.clk(clk),.reset(reset),.i1(intermediate_reg_0[621]),.i2(intermediate_reg_0[620]),.o(intermediate_reg_1[310])); 
xor_module xor_module_inst_1_250(.clk(clk),.reset(reset),.i1(intermediate_reg_0[619]),.i2(intermediate_reg_0[618]),.o(intermediate_reg_1[309])); 
xor_module xor_module_inst_1_251(.clk(clk),.reset(reset),.i1(intermediate_reg_0[617]),.i2(intermediate_reg_0[616]),.o(intermediate_reg_1[308])); 
mux_module mux_module_inst_1_252(.clk(clk),.reset(reset),.i1(intermediate_reg_0[615]),.i2(intermediate_reg_0[614]),.o(intermediate_reg_1[307]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_253(.clk(clk),.reset(reset),.i1(intermediate_reg_0[613]),.i2(intermediate_reg_0[612]),.o(intermediate_reg_1[306])); 
mux_module mux_module_inst_1_254(.clk(clk),.reset(reset),.i1(intermediate_reg_0[611]),.i2(intermediate_reg_0[610]),.o(intermediate_reg_1[305]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_255(.clk(clk),.reset(reset),.i1(intermediate_reg_0[609]),.i2(intermediate_reg_0[608]),.o(intermediate_reg_1[304])); 
mux_module mux_module_inst_1_256(.clk(clk),.reset(reset),.i1(intermediate_reg_0[607]),.i2(intermediate_reg_0[606]),.o(intermediate_reg_1[303]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_257(.clk(clk),.reset(reset),.i1(intermediate_reg_0[605]),.i2(intermediate_reg_0[604]),.o(intermediate_reg_1[302])); 
mux_module mux_module_inst_1_258(.clk(clk),.reset(reset),.i1(intermediate_reg_0[603]),.i2(intermediate_reg_0[602]),.o(intermediate_reg_1[301]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_259(.clk(clk),.reset(reset),.i1(intermediate_reg_0[601]),.i2(intermediate_reg_0[600]),.o(intermediate_reg_1[300]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_260(.clk(clk),.reset(reset),.i1(intermediate_reg_0[599]),.i2(intermediate_reg_0[598]),.o(intermediate_reg_1[299])); 
xor_module xor_module_inst_1_261(.clk(clk),.reset(reset),.i1(intermediate_reg_0[597]),.i2(intermediate_reg_0[596]),.o(intermediate_reg_1[298])); 
xor_module xor_module_inst_1_262(.clk(clk),.reset(reset),.i1(intermediate_reg_0[595]),.i2(intermediate_reg_0[594]),.o(intermediate_reg_1[297])); 
xor_module xor_module_inst_1_263(.clk(clk),.reset(reset),.i1(intermediate_reg_0[593]),.i2(intermediate_reg_0[592]),.o(intermediate_reg_1[296])); 
xor_module xor_module_inst_1_264(.clk(clk),.reset(reset),.i1(intermediate_reg_0[591]),.i2(intermediate_reg_0[590]),.o(intermediate_reg_1[295])); 
xor_module xor_module_inst_1_265(.clk(clk),.reset(reset),.i1(intermediate_reg_0[589]),.i2(intermediate_reg_0[588]),.o(intermediate_reg_1[294])); 
xor_module xor_module_inst_1_266(.clk(clk),.reset(reset),.i1(intermediate_reg_0[587]),.i2(intermediate_reg_0[586]),.o(intermediate_reg_1[293])); 
mux_module mux_module_inst_1_267(.clk(clk),.reset(reset),.i1(intermediate_reg_0[585]),.i2(intermediate_reg_0[584]),.o(intermediate_reg_1[292]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_268(.clk(clk),.reset(reset),.i1(intermediate_reg_0[583]),.i2(intermediate_reg_0[582]),.o(intermediate_reg_1[291]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_269(.clk(clk),.reset(reset),.i1(intermediate_reg_0[581]),.i2(intermediate_reg_0[580]),.o(intermediate_reg_1[290]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_270(.clk(clk),.reset(reset),.i1(intermediate_reg_0[579]),.i2(intermediate_reg_0[578]),.o(intermediate_reg_1[289]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_271(.clk(clk),.reset(reset),.i1(intermediate_reg_0[577]),.i2(intermediate_reg_0[576]),.o(intermediate_reg_1[288]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_272(.clk(clk),.reset(reset),.i1(intermediate_reg_0[575]),.i2(intermediate_reg_0[574]),.o(intermediate_reg_1[287])); 
xor_module xor_module_inst_1_273(.clk(clk),.reset(reset),.i1(intermediate_reg_0[573]),.i2(intermediate_reg_0[572]),.o(intermediate_reg_1[286])); 
xor_module xor_module_inst_1_274(.clk(clk),.reset(reset),.i1(intermediate_reg_0[571]),.i2(intermediate_reg_0[570]),.o(intermediate_reg_1[285])); 
xor_module xor_module_inst_1_275(.clk(clk),.reset(reset),.i1(intermediate_reg_0[569]),.i2(intermediate_reg_0[568]),.o(intermediate_reg_1[284])); 
mux_module mux_module_inst_1_276(.clk(clk),.reset(reset),.i1(intermediate_reg_0[567]),.i2(intermediate_reg_0[566]),.o(intermediate_reg_1[283]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_277(.clk(clk),.reset(reset),.i1(intermediate_reg_0[565]),.i2(intermediate_reg_0[564]),.o(intermediate_reg_1[282]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_278(.clk(clk),.reset(reset),.i1(intermediate_reg_0[563]),.i2(intermediate_reg_0[562]),.o(intermediate_reg_1[281]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_279(.clk(clk),.reset(reset),.i1(intermediate_reg_0[561]),.i2(intermediate_reg_0[560]),.o(intermediate_reg_1[280])); 
xor_module xor_module_inst_1_280(.clk(clk),.reset(reset),.i1(intermediate_reg_0[559]),.i2(intermediate_reg_0[558]),.o(intermediate_reg_1[279])); 
mux_module mux_module_inst_1_281(.clk(clk),.reset(reset),.i1(intermediate_reg_0[557]),.i2(intermediate_reg_0[556]),.o(intermediate_reg_1[278]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_282(.clk(clk),.reset(reset),.i1(intermediate_reg_0[555]),.i2(intermediate_reg_0[554]),.o(intermediate_reg_1[277]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_283(.clk(clk),.reset(reset),.i1(intermediate_reg_0[553]),.i2(intermediate_reg_0[552]),.o(intermediate_reg_1[276]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_284(.clk(clk),.reset(reset),.i1(intermediate_reg_0[551]),.i2(intermediate_reg_0[550]),.o(intermediate_reg_1[275])); 
xor_module xor_module_inst_1_285(.clk(clk),.reset(reset),.i1(intermediate_reg_0[549]),.i2(intermediate_reg_0[548]),.o(intermediate_reg_1[274])); 
mux_module mux_module_inst_1_286(.clk(clk),.reset(reset),.i1(intermediate_reg_0[547]),.i2(intermediate_reg_0[546]),.o(intermediate_reg_1[273]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_287(.clk(clk),.reset(reset),.i1(intermediate_reg_0[545]),.i2(intermediate_reg_0[544]),.o(intermediate_reg_1[272]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_288(.clk(clk),.reset(reset),.i1(intermediate_reg_0[543]),.i2(intermediate_reg_0[542]),.o(intermediate_reg_1[271])); 
mux_module mux_module_inst_1_289(.clk(clk),.reset(reset),.i1(intermediate_reg_0[541]),.i2(intermediate_reg_0[540]),.o(intermediate_reg_1[270]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_290(.clk(clk),.reset(reset),.i1(intermediate_reg_0[539]),.i2(intermediate_reg_0[538]),.o(intermediate_reg_1[269]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_291(.clk(clk),.reset(reset),.i1(intermediate_reg_0[537]),.i2(intermediate_reg_0[536]),.o(intermediate_reg_1[268])); 
mux_module mux_module_inst_1_292(.clk(clk),.reset(reset),.i1(intermediate_reg_0[535]),.i2(intermediate_reg_0[534]),.o(intermediate_reg_1[267]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_293(.clk(clk),.reset(reset),.i1(intermediate_reg_0[533]),.i2(intermediate_reg_0[532]),.o(intermediate_reg_1[266])); 
mux_module mux_module_inst_1_294(.clk(clk),.reset(reset),.i1(intermediate_reg_0[531]),.i2(intermediate_reg_0[530]),.o(intermediate_reg_1[265]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_295(.clk(clk),.reset(reset),.i1(intermediate_reg_0[529]),.i2(intermediate_reg_0[528]),.o(intermediate_reg_1[264]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_296(.clk(clk),.reset(reset),.i1(intermediate_reg_0[527]),.i2(intermediate_reg_0[526]),.o(intermediate_reg_1[263]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_297(.clk(clk),.reset(reset),.i1(intermediate_reg_0[525]),.i2(intermediate_reg_0[524]),.o(intermediate_reg_1[262]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_298(.clk(clk),.reset(reset),.i1(intermediate_reg_0[523]),.i2(intermediate_reg_0[522]),.o(intermediate_reg_1[261])); 
xor_module xor_module_inst_1_299(.clk(clk),.reset(reset),.i1(intermediate_reg_0[521]),.i2(intermediate_reg_0[520]),.o(intermediate_reg_1[260])); 
xor_module xor_module_inst_1_300(.clk(clk),.reset(reset),.i1(intermediate_reg_0[519]),.i2(intermediate_reg_0[518]),.o(intermediate_reg_1[259])); 
xor_module xor_module_inst_1_301(.clk(clk),.reset(reset),.i1(intermediate_reg_0[517]),.i2(intermediate_reg_0[516]),.o(intermediate_reg_1[258])); 
mux_module mux_module_inst_1_302(.clk(clk),.reset(reset),.i1(intermediate_reg_0[515]),.i2(intermediate_reg_0[514]),.o(intermediate_reg_1[257]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_303(.clk(clk),.reset(reset),.i1(intermediate_reg_0[513]),.i2(intermediate_reg_0[512]),.o(intermediate_reg_1[256])); 
xor_module xor_module_inst_1_304(.clk(clk),.reset(reset),.i1(intermediate_reg_0[511]),.i2(intermediate_reg_0[510]),.o(intermediate_reg_1[255])); 
xor_module xor_module_inst_1_305(.clk(clk),.reset(reset),.i1(intermediate_reg_0[509]),.i2(intermediate_reg_0[508]),.o(intermediate_reg_1[254])); 
xor_module xor_module_inst_1_306(.clk(clk),.reset(reset),.i1(intermediate_reg_0[507]),.i2(intermediate_reg_0[506]),.o(intermediate_reg_1[253])); 
mux_module mux_module_inst_1_307(.clk(clk),.reset(reset),.i1(intermediate_reg_0[505]),.i2(intermediate_reg_0[504]),.o(intermediate_reg_1[252]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_308(.clk(clk),.reset(reset),.i1(intermediate_reg_0[503]),.i2(intermediate_reg_0[502]),.o(intermediate_reg_1[251])); 
mux_module mux_module_inst_1_309(.clk(clk),.reset(reset),.i1(intermediate_reg_0[501]),.i2(intermediate_reg_0[500]),.o(intermediate_reg_1[250]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_310(.clk(clk),.reset(reset),.i1(intermediate_reg_0[499]),.i2(intermediate_reg_0[498]),.o(intermediate_reg_1[249])); 
xor_module xor_module_inst_1_311(.clk(clk),.reset(reset),.i1(intermediate_reg_0[497]),.i2(intermediate_reg_0[496]),.o(intermediate_reg_1[248])); 
mux_module mux_module_inst_1_312(.clk(clk),.reset(reset),.i1(intermediate_reg_0[495]),.i2(intermediate_reg_0[494]),.o(intermediate_reg_1[247]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_313(.clk(clk),.reset(reset),.i1(intermediate_reg_0[493]),.i2(intermediate_reg_0[492]),.o(intermediate_reg_1[246])); 
xor_module xor_module_inst_1_314(.clk(clk),.reset(reset),.i1(intermediate_reg_0[491]),.i2(intermediate_reg_0[490]),.o(intermediate_reg_1[245])); 
mux_module mux_module_inst_1_315(.clk(clk),.reset(reset),.i1(intermediate_reg_0[489]),.i2(intermediate_reg_0[488]),.o(intermediate_reg_1[244]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_316(.clk(clk),.reset(reset),.i1(intermediate_reg_0[487]),.i2(intermediate_reg_0[486]),.o(intermediate_reg_1[243])); 
xor_module xor_module_inst_1_317(.clk(clk),.reset(reset),.i1(intermediate_reg_0[485]),.i2(intermediate_reg_0[484]),.o(intermediate_reg_1[242])); 
xor_module xor_module_inst_1_318(.clk(clk),.reset(reset),.i1(intermediate_reg_0[483]),.i2(intermediate_reg_0[482]),.o(intermediate_reg_1[241])); 
xor_module xor_module_inst_1_319(.clk(clk),.reset(reset),.i1(intermediate_reg_0[481]),.i2(intermediate_reg_0[480]),.o(intermediate_reg_1[240])); 
xor_module xor_module_inst_1_320(.clk(clk),.reset(reset),.i1(intermediate_reg_0[479]),.i2(intermediate_reg_0[478]),.o(intermediate_reg_1[239])); 
mux_module mux_module_inst_1_321(.clk(clk),.reset(reset),.i1(intermediate_reg_0[477]),.i2(intermediate_reg_0[476]),.o(intermediate_reg_1[238]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_322(.clk(clk),.reset(reset),.i1(intermediate_reg_0[475]),.i2(intermediate_reg_0[474]),.o(intermediate_reg_1[237]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_323(.clk(clk),.reset(reset),.i1(intermediate_reg_0[473]),.i2(intermediate_reg_0[472]),.o(intermediate_reg_1[236])); 
mux_module mux_module_inst_1_324(.clk(clk),.reset(reset),.i1(intermediate_reg_0[471]),.i2(intermediate_reg_0[470]),.o(intermediate_reg_1[235]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_325(.clk(clk),.reset(reset),.i1(intermediate_reg_0[469]),.i2(intermediate_reg_0[468]),.o(intermediate_reg_1[234]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_326(.clk(clk),.reset(reset),.i1(intermediate_reg_0[467]),.i2(intermediate_reg_0[466]),.o(intermediate_reg_1[233])); 
xor_module xor_module_inst_1_327(.clk(clk),.reset(reset),.i1(intermediate_reg_0[465]),.i2(intermediate_reg_0[464]),.o(intermediate_reg_1[232])); 
xor_module xor_module_inst_1_328(.clk(clk),.reset(reset),.i1(intermediate_reg_0[463]),.i2(intermediate_reg_0[462]),.o(intermediate_reg_1[231])); 
mux_module mux_module_inst_1_329(.clk(clk),.reset(reset),.i1(intermediate_reg_0[461]),.i2(intermediate_reg_0[460]),.o(intermediate_reg_1[230]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_330(.clk(clk),.reset(reset),.i1(intermediate_reg_0[459]),.i2(intermediate_reg_0[458]),.o(intermediate_reg_1[229])); 
mux_module mux_module_inst_1_331(.clk(clk),.reset(reset),.i1(intermediate_reg_0[457]),.i2(intermediate_reg_0[456]),.o(intermediate_reg_1[228]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_332(.clk(clk),.reset(reset),.i1(intermediate_reg_0[455]),.i2(intermediate_reg_0[454]),.o(intermediate_reg_1[227])); 
xor_module xor_module_inst_1_333(.clk(clk),.reset(reset),.i1(intermediate_reg_0[453]),.i2(intermediate_reg_0[452]),.o(intermediate_reg_1[226])); 
xor_module xor_module_inst_1_334(.clk(clk),.reset(reset),.i1(intermediate_reg_0[451]),.i2(intermediate_reg_0[450]),.o(intermediate_reg_1[225])); 
xor_module xor_module_inst_1_335(.clk(clk),.reset(reset),.i1(intermediate_reg_0[449]),.i2(intermediate_reg_0[448]),.o(intermediate_reg_1[224])); 
mux_module mux_module_inst_1_336(.clk(clk),.reset(reset),.i1(intermediate_reg_0[447]),.i2(intermediate_reg_0[446]),.o(intermediate_reg_1[223]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_337(.clk(clk),.reset(reset),.i1(intermediate_reg_0[445]),.i2(intermediate_reg_0[444]),.o(intermediate_reg_1[222])); 
xor_module xor_module_inst_1_338(.clk(clk),.reset(reset),.i1(intermediate_reg_0[443]),.i2(intermediate_reg_0[442]),.o(intermediate_reg_1[221])); 
xor_module xor_module_inst_1_339(.clk(clk),.reset(reset),.i1(intermediate_reg_0[441]),.i2(intermediate_reg_0[440]),.o(intermediate_reg_1[220])); 
mux_module mux_module_inst_1_340(.clk(clk),.reset(reset),.i1(intermediate_reg_0[439]),.i2(intermediate_reg_0[438]),.o(intermediate_reg_1[219]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_341(.clk(clk),.reset(reset),.i1(intermediate_reg_0[437]),.i2(intermediate_reg_0[436]),.o(intermediate_reg_1[218])); 
mux_module mux_module_inst_1_342(.clk(clk),.reset(reset),.i1(intermediate_reg_0[435]),.i2(intermediate_reg_0[434]),.o(intermediate_reg_1[217]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_343(.clk(clk),.reset(reset),.i1(intermediate_reg_0[433]),.i2(intermediate_reg_0[432]),.o(intermediate_reg_1[216]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_344(.clk(clk),.reset(reset),.i1(intermediate_reg_0[431]),.i2(intermediate_reg_0[430]),.o(intermediate_reg_1[215]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_345(.clk(clk),.reset(reset),.i1(intermediate_reg_0[429]),.i2(intermediate_reg_0[428]),.o(intermediate_reg_1[214])); 
mux_module mux_module_inst_1_346(.clk(clk),.reset(reset),.i1(intermediate_reg_0[427]),.i2(intermediate_reg_0[426]),.o(intermediate_reg_1[213]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_347(.clk(clk),.reset(reset),.i1(intermediate_reg_0[425]),.i2(intermediate_reg_0[424]),.o(intermediate_reg_1[212]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_348(.clk(clk),.reset(reset),.i1(intermediate_reg_0[423]),.i2(intermediate_reg_0[422]),.o(intermediate_reg_1[211])); 
mux_module mux_module_inst_1_349(.clk(clk),.reset(reset),.i1(intermediate_reg_0[421]),.i2(intermediate_reg_0[420]),.o(intermediate_reg_1[210]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_350(.clk(clk),.reset(reset),.i1(intermediate_reg_0[419]),.i2(intermediate_reg_0[418]),.o(intermediate_reg_1[209])); 
mux_module mux_module_inst_1_351(.clk(clk),.reset(reset),.i1(intermediate_reg_0[417]),.i2(intermediate_reg_0[416]),.o(intermediate_reg_1[208]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_352(.clk(clk),.reset(reset),.i1(intermediate_reg_0[415]),.i2(intermediate_reg_0[414]),.o(intermediate_reg_1[207])); 
xor_module xor_module_inst_1_353(.clk(clk),.reset(reset),.i1(intermediate_reg_0[413]),.i2(intermediate_reg_0[412]),.o(intermediate_reg_1[206])); 
xor_module xor_module_inst_1_354(.clk(clk),.reset(reset),.i1(intermediate_reg_0[411]),.i2(intermediate_reg_0[410]),.o(intermediate_reg_1[205])); 
mux_module mux_module_inst_1_355(.clk(clk),.reset(reset),.i1(intermediate_reg_0[409]),.i2(intermediate_reg_0[408]),.o(intermediate_reg_1[204]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_356(.clk(clk),.reset(reset),.i1(intermediate_reg_0[407]),.i2(intermediate_reg_0[406]),.o(intermediate_reg_1[203]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_357(.clk(clk),.reset(reset),.i1(intermediate_reg_0[405]),.i2(intermediate_reg_0[404]),.o(intermediate_reg_1[202]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_358(.clk(clk),.reset(reset),.i1(intermediate_reg_0[403]),.i2(intermediate_reg_0[402]),.o(intermediate_reg_1[201])); 
mux_module mux_module_inst_1_359(.clk(clk),.reset(reset),.i1(intermediate_reg_0[401]),.i2(intermediate_reg_0[400]),.o(intermediate_reg_1[200]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_360(.clk(clk),.reset(reset),.i1(intermediate_reg_0[399]),.i2(intermediate_reg_0[398]),.o(intermediate_reg_1[199]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_361(.clk(clk),.reset(reset),.i1(intermediate_reg_0[397]),.i2(intermediate_reg_0[396]),.o(intermediate_reg_1[198]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_362(.clk(clk),.reset(reset),.i1(intermediate_reg_0[395]),.i2(intermediate_reg_0[394]),.o(intermediate_reg_1[197])); 
mux_module mux_module_inst_1_363(.clk(clk),.reset(reset),.i1(intermediate_reg_0[393]),.i2(intermediate_reg_0[392]),.o(intermediate_reg_1[196]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_364(.clk(clk),.reset(reset),.i1(intermediate_reg_0[391]),.i2(intermediate_reg_0[390]),.o(intermediate_reg_1[195])); 
xor_module xor_module_inst_1_365(.clk(clk),.reset(reset),.i1(intermediate_reg_0[389]),.i2(intermediate_reg_0[388]),.o(intermediate_reg_1[194])); 
mux_module mux_module_inst_1_366(.clk(clk),.reset(reset),.i1(intermediate_reg_0[387]),.i2(intermediate_reg_0[386]),.o(intermediate_reg_1[193]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_367(.clk(clk),.reset(reset),.i1(intermediate_reg_0[385]),.i2(intermediate_reg_0[384]),.o(intermediate_reg_1[192])); 
xor_module xor_module_inst_1_368(.clk(clk),.reset(reset),.i1(intermediate_reg_0[383]),.i2(intermediate_reg_0[382]),.o(intermediate_reg_1[191])); 
xor_module xor_module_inst_1_369(.clk(clk),.reset(reset),.i1(intermediate_reg_0[381]),.i2(intermediate_reg_0[380]),.o(intermediate_reg_1[190])); 
mux_module mux_module_inst_1_370(.clk(clk),.reset(reset),.i1(intermediate_reg_0[379]),.i2(intermediate_reg_0[378]),.o(intermediate_reg_1[189]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_371(.clk(clk),.reset(reset),.i1(intermediate_reg_0[377]),.i2(intermediate_reg_0[376]),.o(intermediate_reg_1[188]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_372(.clk(clk),.reset(reset),.i1(intermediate_reg_0[375]),.i2(intermediate_reg_0[374]),.o(intermediate_reg_1[187])); 
xor_module xor_module_inst_1_373(.clk(clk),.reset(reset),.i1(intermediate_reg_0[373]),.i2(intermediate_reg_0[372]),.o(intermediate_reg_1[186])); 
xor_module xor_module_inst_1_374(.clk(clk),.reset(reset),.i1(intermediate_reg_0[371]),.i2(intermediate_reg_0[370]),.o(intermediate_reg_1[185])); 
xor_module xor_module_inst_1_375(.clk(clk),.reset(reset),.i1(intermediate_reg_0[369]),.i2(intermediate_reg_0[368]),.o(intermediate_reg_1[184])); 
xor_module xor_module_inst_1_376(.clk(clk),.reset(reset),.i1(intermediate_reg_0[367]),.i2(intermediate_reg_0[366]),.o(intermediate_reg_1[183])); 
mux_module mux_module_inst_1_377(.clk(clk),.reset(reset),.i1(intermediate_reg_0[365]),.i2(intermediate_reg_0[364]),.o(intermediate_reg_1[182]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_378(.clk(clk),.reset(reset),.i1(intermediate_reg_0[363]),.i2(intermediate_reg_0[362]),.o(intermediate_reg_1[181]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_379(.clk(clk),.reset(reset),.i1(intermediate_reg_0[361]),.i2(intermediate_reg_0[360]),.o(intermediate_reg_1[180])); 
xor_module xor_module_inst_1_380(.clk(clk),.reset(reset),.i1(intermediate_reg_0[359]),.i2(intermediate_reg_0[358]),.o(intermediate_reg_1[179])); 
xor_module xor_module_inst_1_381(.clk(clk),.reset(reset),.i1(intermediate_reg_0[357]),.i2(intermediate_reg_0[356]),.o(intermediate_reg_1[178])); 
mux_module mux_module_inst_1_382(.clk(clk),.reset(reset),.i1(intermediate_reg_0[355]),.i2(intermediate_reg_0[354]),.o(intermediate_reg_1[177]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_383(.clk(clk),.reset(reset),.i1(intermediate_reg_0[353]),.i2(intermediate_reg_0[352]),.o(intermediate_reg_1[176]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_384(.clk(clk),.reset(reset),.i1(intermediate_reg_0[351]),.i2(intermediate_reg_0[350]),.o(intermediate_reg_1[175]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_385(.clk(clk),.reset(reset),.i1(intermediate_reg_0[349]),.i2(intermediate_reg_0[348]),.o(intermediate_reg_1[174]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_386(.clk(clk),.reset(reset),.i1(intermediate_reg_0[347]),.i2(intermediate_reg_0[346]),.o(intermediate_reg_1[173])); 
xor_module xor_module_inst_1_387(.clk(clk),.reset(reset),.i1(intermediate_reg_0[345]),.i2(intermediate_reg_0[344]),.o(intermediate_reg_1[172])); 
mux_module mux_module_inst_1_388(.clk(clk),.reset(reset),.i1(intermediate_reg_0[343]),.i2(intermediate_reg_0[342]),.o(intermediate_reg_1[171]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_389(.clk(clk),.reset(reset),.i1(intermediate_reg_0[341]),.i2(intermediate_reg_0[340]),.o(intermediate_reg_1[170])); 
xor_module xor_module_inst_1_390(.clk(clk),.reset(reset),.i1(intermediate_reg_0[339]),.i2(intermediate_reg_0[338]),.o(intermediate_reg_1[169])); 
xor_module xor_module_inst_1_391(.clk(clk),.reset(reset),.i1(intermediate_reg_0[337]),.i2(intermediate_reg_0[336]),.o(intermediate_reg_1[168])); 
mux_module mux_module_inst_1_392(.clk(clk),.reset(reset),.i1(intermediate_reg_0[335]),.i2(intermediate_reg_0[334]),.o(intermediate_reg_1[167]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_393(.clk(clk),.reset(reset),.i1(intermediate_reg_0[333]),.i2(intermediate_reg_0[332]),.o(intermediate_reg_1[166]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_394(.clk(clk),.reset(reset),.i1(intermediate_reg_0[331]),.i2(intermediate_reg_0[330]),.o(intermediate_reg_1[165]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_395(.clk(clk),.reset(reset),.i1(intermediate_reg_0[329]),.i2(intermediate_reg_0[328]),.o(intermediate_reg_1[164]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_396(.clk(clk),.reset(reset),.i1(intermediate_reg_0[327]),.i2(intermediate_reg_0[326]),.o(intermediate_reg_1[163])); 
xor_module xor_module_inst_1_397(.clk(clk),.reset(reset),.i1(intermediate_reg_0[325]),.i2(intermediate_reg_0[324]),.o(intermediate_reg_1[162])); 
xor_module xor_module_inst_1_398(.clk(clk),.reset(reset),.i1(intermediate_reg_0[323]),.i2(intermediate_reg_0[322]),.o(intermediate_reg_1[161])); 
mux_module mux_module_inst_1_399(.clk(clk),.reset(reset),.i1(intermediate_reg_0[321]),.i2(intermediate_reg_0[320]),.o(intermediate_reg_1[160]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_400(.clk(clk),.reset(reset),.i1(intermediate_reg_0[319]),.i2(intermediate_reg_0[318]),.o(intermediate_reg_1[159])); 
xor_module xor_module_inst_1_401(.clk(clk),.reset(reset),.i1(intermediate_reg_0[317]),.i2(intermediate_reg_0[316]),.o(intermediate_reg_1[158])); 
xor_module xor_module_inst_1_402(.clk(clk),.reset(reset),.i1(intermediate_reg_0[315]),.i2(intermediate_reg_0[314]),.o(intermediate_reg_1[157])); 
xor_module xor_module_inst_1_403(.clk(clk),.reset(reset),.i1(intermediate_reg_0[313]),.i2(intermediate_reg_0[312]),.o(intermediate_reg_1[156])); 
mux_module mux_module_inst_1_404(.clk(clk),.reset(reset),.i1(intermediate_reg_0[311]),.i2(intermediate_reg_0[310]),.o(intermediate_reg_1[155]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_405(.clk(clk),.reset(reset),.i1(intermediate_reg_0[309]),.i2(intermediate_reg_0[308]),.o(intermediate_reg_1[154])); 
xor_module xor_module_inst_1_406(.clk(clk),.reset(reset),.i1(intermediate_reg_0[307]),.i2(intermediate_reg_0[306]),.o(intermediate_reg_1[153])); 
mux_module mux_module_inst_1_407(.clk(clk),.reset(reset),.i1(intermediate_reg_0[305]),.i2(intermediate_reg_0[304]),.o(intermediate_reg_1[152]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_408(.clk(clk),.reset(reset),.i1(intermediate_reg_0[303]),.i2(intermediate_reg_0[302]),.o(intermediate_reg_1[151])); 
mux_module mux_module_inst_1_409(.clk(clk),.reset(reset),.i1(intermediate_reg_0[301]),.i2(intermediate_reg_0[300]),.o(intermediate_reg_1[150]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_410(.clk(clk),.reset(reset),.i1(intermediate_reg_0[299]),.i2(intermediate_reg_0[298]),.o(intermediate_reg_1[149]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_411(.clk(clk),.reset(reset),.i1(intermediate_reg_0[297]),.i2(intermediate_reg_0[296]),.o(intermediate_reg_1[148]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_412(.clk(clk),.reset(reset),.i1(intermediate_reg_0[295]),.i2(intermediate_reg_0[294]),.o(intermediate_reg_1[147])); 
xor_module xor_module_inst_1_413(.clk(clk),.reset(reset),.i1(intermediate_reg_0[293]),.i2(intermediate_reg_0[292]),.o(intermediate_reg_1[146])); 
mux_module mux_module_inst_1_414(.clk(clk),.reset(reset),.i1(intermediate_reg_0[291]),.i2(intermediate_reg_0[290]),.o(intermediate_reg_1[145]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_415(.clk(clk),.reset(reset),.i1(intermediate_reg_0[289]),.i2(intermediate_reg_0[288]),.o(intermediate_reg_1[144])); 
xor_module xor_module_inst_1_416(.clk(clk),.reset(reset),.i1(intermediate_reg_0[287]),.i2(intermediate_reg_0[286]),.o(intermediate_reg_1[143])); 
xor_module xor_module_inst_1_417(.clk(clk),.reset(reset),.i1(intermediate_reg_0[285]),.i2(intermediate_reg_0[284]),.o(intermediate_reg_1[142])); 
mux_module mux_module_inst_1_418(.clk(clk),.reset(reset),.i1(intermediate_reg_0[283]),.i2(intermediate_reg_0[282]),.o(intermediate_reg_1[141]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_419(.clk(clk),.reset(reset),.i1(intermediate_reg_0[281]),.i2(intermediate_reg_0[280]),.o(intermediate_reg_1[140]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_420(.clk(clk),.reset(reset),.i1(intermediate_reg_0[279]),.i2(intermediate_reg_0[278]),.o(intermediate_reg_1[139]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_421(.clk(clk),.reset(reset),.i1(intermediate_reg_0[277]),.i2(intermediate_reg_0[276]),.o(intermediate_reg_1[138])); 
mux_module mux_module_inst_1_422(.clk(clk),.reset(reset),.i1(intermediate_reg_0[275]),.i2(intermediate_reg_0[274]),.o(intermediate_reg_1[137]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_423(.clk(clk),.reset(reset),.i1(intermediate_reg_0[273]),.i2(intermediate_reg_0[272]),.o(intermediate_reg_1[136])); 
xor_module xor_module_inst_1_424(.clk(clk),.reset(reset),.i1(intermediate_reg_0[271]),.i2(intermediate_reg_0[270]),.o(intermediate_reg_1[135])); 
mux_module mux_module_inst_1_425(.clk(clk),.reset(reset),.i1(intermediate_reg_0[269]),.i2(intermediate_reg_0[268]),.o(intermediate_reg_1[134]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_426(.clk(clk),.reset(reset),.i1(intermediate_reg_0[267]),.i2(intermediate_reg_0[266]),.o(intermediate_reg_1[133])); 
mux_module mux_module_inst_1_427(.clk(clk),.reset(reset),.i1(intermediate_reg_0[265]),.i2(intermediate_reg_0[264]),.o(intermediate_reg_1[132]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_428(.clk(clk),.reset(reset),.i1(intermediate_reg_0[263]),.i2(intermediate_reg_0[262]),.o(intermediate_reg_1[131])); 
mux_module mux_module_inst_1_429(.clk(clk),.reset(reset),.i1(intermediate_reg_0[261]),.i2(intermediate_reg_0[260]),.o(intermediate_reg_1[130]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_430(.clk(clk),.reset(reset),.i1(intermediate_reg_0[259]),.i2(intermediate_reg_0[258]),.o(intermediate_reg_1[129])); 
mux_module mux_module_inst_1_431(.clk(clk),.reset(reset),.i1(intermediate_reg_0[257]),.i2(intermediate_reg_0[256]),.o(intermediate_reg_1[128]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_432(.clk(clk),.reset(reset),.i1(intermediate_reg_0[255]),.i2(intermediate_reg_0[254]),.o(intermediate_reg_1[127]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_433(.clk(clk),.reset(reset),.i1(intermediate_reg_0[253]),.i2(intermediate_reg_0[252]),.o(intermediate_reg_1[126]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_434(.clk(clk),.reset(reset),.i1(intermediate_reg_0[251]),.i2(intermediate_reg_0[250]),.o(intermediate_reg_1[125]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_435(.clk(clk),.reset(reset),.i1(intermediate_reg_0[249]),.i2(intermediate_reg_0[248]),.o(intermediate_reg_1[124]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_436(.clk(clk),.reset(reset),.i1(intermediate_reg_0[247]),.i2(intermediate_reg_0[246]),.o(intermediate_reg_1[123])); 
mux_module mux_module_inst_1_437(.clk(clk),.reset(reset),.i1(intermediate_reg_0[245]),.i2(intermediate_reg_0[244]),.o(intermediate_reg_1[122]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_438(.clk(clk),.reset(reset),.i1(intermediate_reg_0[243]),.i2(intermediate_reg_0[242]),.o(intermediate_reg_1[121])); 
mux_module mux_module_inst_1_439(.clk(clk),.reset(reset),.i1(intermediate_reg_0[241]),.i2(intermediate_reg_0[240]),.o(intermediate_reg_1[120]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_440(.clk(clk),.reset(reset),.i1(intermediate_reg_0[239]),.i2(intermediate_reg_0[238]),.o(intermediate_reg_1[119]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_441(.clk(clk),.reset(reset),.i1(intermediate_reg_0[237]),.i2(intermediate_reg_0[236]),.o(intermediate_reg_1[118]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_442(.clk(clk),.reset(reset),.i1(intermediate_reg_0[235]),.i2(intermediate_reg_0[234]),.o(intermediate_reg_1[117]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_443(.clk(clk),.reset(reset),.i1(intermediate_reg_0[233]),.i2(intermediate_reg_0[232]),.o(intermediate_reg_1[116])); 
xor_module xor_module_inst_1_444(.clk(clk),.reset(reset),.i1(intermediate_reg_0[231]),.i2(intermediate_reg_0[230]),.o(intermediate_reg_1[115])); 
xor_module xor_module_inst_1_445(.clk(clk),.reset(reset),.i1(intermediate_reg_0[229]),.i2(intermediate_reg_0[228]),.o(intermediate_reg_1[114])); 
xor_module xor_module_inst_1_446(.clk(clk),.reset(reset),.i1(intermediate_reg_0[227]),.i2(intermediate_reg_0[226]),.o(intermediate_reg_1[113])); 
xor_module xor_module_inst_1_447(.clk(clk),.reset(reset),.i1(intermediate_reg_0[225]),.i2(intermediate_reg_0[224]),.o(intermediate_reg_1[112])); 
mux_module mux_module_inst_1_448(.clk(clk),.reset(reset),.i1(intermediate_reg_0[223]),.i2(intermediate_reg_0[222]),.o(intermediate_reg_1[111]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_449(.clk(clk),.reset(reset),.i1(intermediate_reg_0[221]),.i2(intermediate_reg_0[220]),.o(intermediate_reg_1[110])); 
mux_module mux_module_inst_1_450(.clk(clk),.reset(reset),.i1(intermediate_reg_0[219]),.i2(intermediate_reg_0[218]),.o(intermediate_reg_1[109]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_451(.clk(clk),.reset(reset),.i1(intermediate_reg_0[217]),.i2(intermediate_reg_0[216]),.o(intermediate_reg_1[108])); 
mux_module mux_module_inst_1_452(.clk(clk),.reset(reset),.i1(intermediate_reg_0[215]),.i2(intermediate_reg_0[214]),.o(intermediate_reg_1[107]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_453(.clk(clk),.reset(reset),.i1(intermediate_reg_0[213]),.i2(intermediate_reg_0[212]),.o(intermediate_reg_1[106]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_454(.clk(clk),.reset(reset),.i1(intermediate_reg_0[211]),.i2(intermediate_reg_0[210]),.o(intermediate_reg_1[105])); 
xor_module xor_module_inst_1_455(.clk(clk),.reset(reset),.i1(intermediate_reg_0[209]),.i2(intermediate_reg_0[208]),.o(intermediate_reg_1[104])); 
mux_module mux_module_inst_1_456(.clk(clk),.reset(reset),.i1(intermediate_reg_0[207]),.i2(intermediate_reg_0[206]),.o(intermediate_reg_1[103]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_457(.clk(clk),.reset(reset),.i1(intermediate_reg_0[205]),.i2(intermediate_reg_0[204]),.o(intermediate_reg_1[102])); 
mux_module mux_module_inst_1_458(.clk(clk),.reset(reset),.i1(intermediate_reg_0[203]),.i2(intermediate_reg_0[202]),.o(intermediate_reg_1[101]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_459(.clk(clk),.reset(reset),.i1(intermediate_reg_0[201]),.i2(intermediate_reg_0[200]),.o(intermediate_reg_1[100]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_460(.clk(clk),.reset(reset),.i1(intermediate_reg_0[199]),.i2(intermediate_reg_0[198]),.o(intermediate_reg_1[99]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_461(.clk(clk),.reset(reset),.i1(intermediate_reg_0[197]),.i2(intermediate_reg_0[196]),.o(intermediate_reg_1[98])); 
xor_module xor_module_inst_1_462(.clk(clk),.reset(reset),.i1(intermediate_reg_0[195]),.i2(intermediate_reg_0[194]),.o(intermediate_reg_1[97])); 
mux_module mux_module_inst_1_463(.clk(clk),.reset(reset),.i1(intermediate_reg_0[193]),.i2(intermediate_reg_0[192]),.o(intermediate_reg_1[96]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_464(.clk(clk),.reset(reset),.i1(intermediate_reg_0[191]),.i2(intermediate_reg_0[190]),.o(intermediate_reg_1[95]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_465(.clk(clk),.reset(reset),.i1(intermediate_reg_0[189]),.i2(intermediate_reg_0[188]),.o(intermediate_reg_1[94])); 
mux_module mux_module_inst_1_466(.clk(clk),.reset(reset),.i1(intermediate_reg_0[187]),.i2(intermediate_reg_0[186]),.o(intermediate_reg_1[93]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_467(.clk(clk),.reset(reset),.i1(intermediate_reg_0[185]),.i2(intermediate_reg_0[184]),.o(intermediate_reg_1[92])); 
xor_module xor_module_inst_1_468(.clk(clk),.reset(reset),.i1(intermediate_reg_0[183]),.i2(intermediate_reg_0[182]),.o(intermediate_reg_1[91])); 
mux_module mux_module_inst_1_469(.clk(clk),.reset(reset),.i1(intermediate_reg_0[181]),.i2(intermediate_reg_0[180]),.o(intermediate_reg_1[90]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_470(.clk(clk),.reset(reset),.i1(intermediate_reg_0[179]),.i2(intermediate_reg_0[178]),.o(intermediate_reg_1[89])); 
mux_module mux_module_inst_1_471(.clk(clk),.reset(reset),.i1(intermediate_reg_0[177]),.i2(intermediate_reg_0[176]),.o(intermediate_reg_1[88]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_472(.clk(clk),.reset(reset),.i1(intermediate_reg_0[175]),.i2(intermediate_reg_0[174]),.o(intermediate_reg_1[87]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_473(.clk(clk),.reset(reset),.i1(intermediate_reg_0[173]),.i2(intermediate_reg_0[172]),.o(intermediate_reg_1[86])); 
xor_module xor_module_inst_1_474(.clk(clk),.reset(reset),.i1(intermediate_reg_0[171]),.i2(intermediate_reg_0[170]),.o(intermediate_reg_1[85])); 
mux_module mux_module_inst_1_475(.clk(clk),.reset(reset),.i1(intermediate_reg_0[169]),.i2(intermediate_reg_0[168]),.o(intermediate_reg_1[84]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_476(.clk(clk),.reset(reset),.i1(intermediate_reg_0[167]),.i2(intermediate_reg_0[166]),.o(intermediate_reg_1[83])); 
mux_module mux_module_inst_1_477(.clk(clk),.reset(reset),.i1(intermediate_reg_0[165]),.i2(intermediate_reg_0[164]),.o(intermediate_reg_1[82]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_478(.clk(clk),.reset(reset),.i1(intermediate_reg_0[163]),.i2(intermediate_reg_0[162]),.o(intermediate_reg_1[81]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_479(.clk(clk),.reset(reset),.i1(intermediate_reg_0[161]),.i2(intermediate_reg_0[160]),.o(intermediate_reg_1[80]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_480(.clk(clk),.reset(reset),.i1(intermediate_reg_0[159]),.i2(intermediate_reg_0[158]),.o(intermediate_reg_1[79]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_481(.clk(clk),.reset(reset),.i1(intermediate_reg_0[157]),.i2(intermediate_reg_0[156]),.o(intermediate_reg_1[78]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_482(.clk(clk),.reset(reset),.i1(intermediate_reg_0[155]),.i2(intermediate_reg_0[154]),.o(intermediate_reg_1[77])); 
xor_module xor_module_inst_1_483(.clk(clk),.reset(reset),.i1(intermediate_reg_0[153]),.i2(intermediate_reg_0[152]),.o(intermediate_reg_1[76])); 
mux_module mux_module_inst_1_484(.clk(clk),.reset(reset),.i1(intermediate_reg_0[151]),.i2(intermediate_reg_0[150]),.o(intermediate_reg_1[75]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_485(.clk(clk),.reset(reset),.i1(intermediate_reg_0[149]),.i2(intermediate_reg_0[148]),.o(intermediate_reg_1[74]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_486(.clk(clk),.reset(reset),.i1(intermediate_reg_0[147]),.i2(intermediate_reg_0[146]),.o(intermediate_reg_1[73]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_487(.clk(clk),.reset(reset),.i1(intermediate_reg_0[145]),.i2(intermediate_reg_0[144]),.o(intermediate_reg_1[72])); 
mux_module mux_module_inst_1_488(.clk(clk),.reset(reset),.i1(intermediate_reg_0[143]),.i2(intermediate_reg_0[142]),.o(intermediate_reg_1[71]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_489(.clk(clk),.reset(reset),.i1(intermediate_reg_0[141]),.i2(intermediate_reg_0[140]),.o(intermediate_reg_1[70]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_490(.clk(clk),.reset(reset),.i1(intermediate_reg_0[139]),.i2(intermediate_reg_0[138]),.o(intermediate_reg_1[69]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_491(.clk(clk),.reset(reset),.i1(intermediate_reg_0[137]),.i2(intermediate_reg_0[136]),.o(intermediate_reg_1[68])); 
xor_module xor_module_inst_1_492(.clk(clk),.reset(reset),.i1(intermediate_reg_0[135]),.i2(intermediate_reg_0[134]),.o(intermediate_reg_1[67])); 
mux_module mux_module_inst_1_493(.clk(clk),.reset(reset),.i1(intermediate_reg_0[133]),.i2(intermediate_reg_0[132]),.o(intermediate_reg_1[66]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_494(.clk(clk),.reset(reset),.i1(intermediate_reg_0[131]),.i2(intermediate_reg_0[130]),.o(intermediate_reg_1[65])); 
xor_module xor_module_inst_1_495(.clk(clk),.reset(reset),.i1(intermediate_reg_0[129]),.i2(intermediate_reg_0[128]),.o(intermediate_reg_1[64])); 
mux_module mux_module_inst_1_496(.clk(clk),.reset(reset),.i1(intermediate_reg_0[127]),.i2(intermediate_reg_0[126]),.o(intermediate_reg_1[63]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_497(.clk(clk),.reset(reset),.i1(intermediate_reg_0[125]),.i2(intermediate_reg_0[124]),.o(intermediate_reg_1[62])); 
xor_module xor_module_inst_1_498(.clk(clk),.reset(reset),.i1(intermediate_reg_0[123]),.i2(intermediate_reg_0[122]),.o(intermediate_reg_1[61])); 
mux_module mux_module_inst_1_499(.clk(clk),.reset(reset),.i1(intermediate_reg_0[121]),.i2(intermediate_reg_0[120]),.o(intermediate_reg_1[60]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_500(.clk(clk),.reset(reset),.i1(intermediate_reg_0[119]),.i2(intermediate_reg_0[118]),.o(intermediate_reg_1[59]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_501(.clk(clk),.reset(reset),.i1(intermediate_reg_0[117]),.i2(intermediate_reg_0[116]),.o(intermediate_reg_1[58])); 
xor_module xor_module_inst_1_502(.clk(clk),.reset(reset),.i1(intermediate_reg_0[115]),.i2(intermediate_reg_0[114]),.o(intermediate_reg_1[57])); 
mux_module mux_module_inst_1_503(.clk(clk),.reset(reset),.i1(intermediate_reg_0[113]),.i2(intermediate_reg_0[112]),.o(intermediate_reg_1[56]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_504(.clk(clk),.reset(reset),.i1(intermediate_reg_0[111]),.i2(intermediate_reg_0[110]),.o(intermediate_reg_1[55])); 
mux_module mux_module_inst_1_505(.clk(clk),.reset(reset),.i1(intermediate_reg_0[109]),.i2(intermediate_reg_0[108]),.o(intermediate_reg_1[54]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_506(.clk(clk),.reset(reset),.i1(intermediate_reg_0[107]),.i2(intermediate_reg_0[106]),.o(intermediate_reg_1[53])); 
mux_module mux_module_inst_1_507(.clk(clk),.reset(reset),.i1(intermediate_reg_0[105]),.i2(intermediate_reg_0[104]),.o(intermediate_reg_1[52]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_508(.clk(clk),.reset(reset),.i1(intermediate_reg_0[103]),.i2(intermediate_reg_0[102]),.o(intermediate_reg_1[51]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_509(.clk(clk),.reset(reset),.i1(intermediate_reg_0[101]),.i2(intermediate_reg_0[100]),.o(intermediate_reg_1[50]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_510(.clk(clk),.reset(reset),.i1(intermediate_reg_0[99]),.i2(intermediate_reg_0[98]),.o(intermediate_reg_1[49]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_511(.clk(clk),.reset(reset),.i1(intermediate_reg_0[97]),.i2(intermediate_reg_0[96]),.o(intermediate_reg_1[48])); 
xor_module xor_module_inst_1_512(.clk(clk),.reset(reset),.i1(intermediate_reg_0[95]),.i2(intermediate_reg_0[94]),.o(intermediate_reg_1[47])); 
xor_module xor_module_inst_1_513(.clk(clk),.reset(reset),.i1(intermediate_reg_0[93]),.i2(intermediate_reg_0[92]),.o(intermediate_reg_1[46])); 
xor_module xor_module_inst_1_514(.clk(clk),.reset(reset),.i1(intermediate_reg_0[91]),.i2(intermediate_reg_0[90]),.o(intermediate_reg_1[45])); 
mux_module mux_module_inst_1_515(.clk(clk),.reset(reset),.i1(intermediate_reg_0[89]),.i2(intermediate_reg_0[88]),.o(intermediate_reg_1[44]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_516(.clk(clk),.reset(reset),.i1(intermediate_reg_0[87]),.i2(intermediate_reg_0[86]),.o(intermediate_reg_1[43])); 
mux_module mux_module_inst_1_517(.clk(clk),.reset(reset),.i1(intermediate_reg_0[85]),.i2(intermediate_reg_0[84]),.o(intermediate_reg_1[42]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_518(.clk(clk),.reset(reset),.i1(intermediate_reg_0[83]),.i2(intermediate_reg_0[82]),.o(intermediate_reg_1[41]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_519(.clk(clk),.reset(reset),.i1(intermediate_reg_0[81]),.i2(intermediate_reg_0[80]),.o(intermediate_reg_1[40])); 
mux_module mux_module_inst_1_520(.clk(clk),.reset(reset),.i1(intermediate_reg_0[79]),.i2(intermediate_reg_0[78]),.o(intermediate_reg_1[39]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_521(.clk(clk),.reset(reset),.i1(intermediate_reg_0[77]),.i2(intermediate_reg_0[76]),.o(intermediate_reg_1[38]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_522(.clk(clk),.reset(reset),.i1(intermediate_reg_0[75]),.i2(intermediate_reg_0[74]),.o(intermediate_reg_1[37])); 
xor_module xor_module_inst_1_523(.clk(clk),.reset(reset),.i1(intermediate_reg_0[73]),.i2(intermediate_reg_0[72]),.o(intermediate_reg_1[36])); 
xor_module xor_module_inst_1_524(.clk(clk),.reset(reset),.i1(intermediate_reg_0[71]),.i2(intermediate_reg_0[70]),.o(intermediate_reg_1[35])); 
mux_module mux_module_inst_1_525(.clk(clk),.reset(reset),.i1(intermediate_reg_0[69]),.i2(intermediate_reg_0[68]),.o(intermediate_reg_1[34]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_526(.clk(clk),.reset(reset),.i1(intermediate_reg_0[67]),.i2(intermediate_reg_0[66]),.o(intermediate_reg_1[33])); 
mux_module mux_module_inst_1_527(.clk(clk),.reset(reset),.i1(intermediate_reg_0[65]),.i2(intermediate_reg_0[64]),.o(intermediate_reg_1[32]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_528(.clk(clk),.reset(reset),.i1(intermediate_reg_0[63]),.i2(intermediate_reg_0[62]),.o(intermediate_reg_1[31]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_529(.clk(clk),.reset(reset),.i1(intermediate_reg_0[61]),.i2(intermediate_reg_0[60]),.o(intermediate_reg_1[30]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_530(.clk(clk),.reset(reset),.i1(intermediate_reg_0[59]),.i2(intermediate_reg_0[58]),.o(intermediate_reg_1[29]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_531(.clk(clk),.reset(reset),.i1(intermediate_reg_0[57]),.i2(intermediate_reg_0[56]),.o(intermediate_reg_1[28])); 
mux_module mux_module_inst_1_532(.clk(clk),.reset(reset),.i1(intermediate_reg_0[55]),.i2(intermediate_reg_0[54]),.o(intermediate_reg_1[27]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_533(.clk(clk),.reset(reset),.i1(intermediate_reg_0[53]),.i2(intermediate_reg_0[52]),.o(intermediate_reg_1[26])); 
xor_module xor_module_inst_1_534(.clk(clk),.reset(reset),.i1(intermediate_reg_0[51]),.i2(intermediate_reg_0[50]),.o(intermediate_reg_1[25])); 
xor_module xor_module_inst_1_535(.clk(clk),.reset(reset),.i1(intermediate_reg_0[49]),.i2(intermediate_reg_0[48]),.o(intermediate_reg_1[24])); 
xor_module xor_module_inst_1_536(.clk(clk),.reset(reset),.i1(intermediate_reg_0[47]),.i2(intermediate_reg_0[46]),.o(intermediate_reg_1[23])); 
xor_module xor_module_inst_1_537(.clk(clk),.reset(reset),.i1(intermediate_reg_0[45]),.i2(intermediate_reg_0[44]),.o(intermediate_reg_1[22])); 
xor_module xor_module_inst_1_538(.clk(clk),.reset(reset),.i1(intermediate_reg_0[43]),.i2(intermediate_reg_0[42]),.o(intermediate_reg_1[21])); 
xor_module xor_module_inst_1_539(.clk(clk),.reset(reset),.i1(intermediate_reg_0[41]),.i2(intermediate_reg_0[40]),.o(intermediate_reg_1[20])); 
xor_module xor_module_inst_1_540(.clk(clk),.reset(reset),.i1(intermediate_reg_0[39]),.i2(intermediate_reg_0[38]),.o(intermediate_reg_1[19])); 
xor_module xor_module_inst_1_541(.clk(clk),.reset(reset),.i1(intermediate_reg_0[37]),.i2(intermediate_reg_0[36]),.o(intermediate_reg_1[18])); 
xor_module xor_module_inst_1_542(.clk(clk),.reset(reset),.i1(intermediate_reg_0[35]),.i2(intermediate_reg_0[34]),.o(intermediate_reg_1[17])); 
xor_module xor_module_inst_1_543(.clk(clk),.reset(reset),.i1(intermediate_reg_0[33]),.i2(intermediate_reg_0[32]),.o(intermediate_reg_1[16])); 
mux_module mux_module_inst_1_544(.clk(clk),.reset(reset),.i1(intermediate_reg_0[31]),.i2(intermediate_reg_0[30]),.o(intermediate_reg_1[15]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_545(.clk(clk),.reset(reset),.i1(intermediate_reg_0[29]),.i2(intermediate_reg_0[28]),.o(intermediate_reg_1[14]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_546(.clk(clk),.reset(reset),.i1(intermediate_reg_0[27]),.i2(intermediate_reg_0[26]),.o(intermediate_reg_1[13])); 
xor_module xor_module_inst_1_547(.clk(clk),.reset(reset),.i1(intermediate_reg_0[25]),.i2(intermediate_reg_0[24]),.o(intermediate_reg_1[12])); 
mux_module mux_module_inst_1_548(.clk(clk),.reset(reset),.i1(intermediate_reg_0[23]),.i2(intermediate_reg_0[22]),.o(intermediate_reg_1[11]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_549(.clk(clk),.reset(reset),.i1(intermediate_reg_0[21]),.i2(intermediate_reg_0[20]),.o(intermediate_reg_1[10]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_550(.clk(clk),.reset(reset),.i1(intermediate_reg_0[19]),.i2(intermediate_reg_0[18]),.o(intermediate_reg_1[9]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_551(.clk(clk),.reset(reset),.i1(intermediate_reg_0[17]),.i2(intermediate_reg_0[16]),.o(intermediate_reg_1[8]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_552(.clk(clk),.reset(reset),.i1(intermediate_reg_0[15]),.i2(intermediate_reg_0[14]),.o(intermediate_reg_1[7])); 
mux_module mux_module_inst_1_553(.clk(clk),.reset(reset),.i1(intermediate_reg_0[13]),.i2(intermediate_reg_0[12]),.o(intermediate_reg_1[6]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_554(.clk(clk),.reset(reset),.i1(intermediate_reg_0[11]),.i2(intermediate_reg_0[10]),.o(intermediate_reg_1[5]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_555(.clk(clk),.reset(reset),.i1(intermediate_reg_0[9]),.i2(intermediate_reg_0[8]),.o(intermediate_reg_1[4])); 
xor_module xor_module_inst_1_556(.clk(clk),.reset(reset),.i1(intermediate_reg_0[7]),.i2(intermediate_reg_0[6]),.o(intermediate_reg_1[3])); 
mux_module mux_module_inst_1_557(.clk(clk),.reset(reset),.i1(intermediate_reg_0[5]),.i2(intermediate_reg_0[4]),.o(intermediate_reg_1[2]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_558(.clk(clk),.reset(reset),.i1(intermediate_reg_0[3]),.i2(intermediate_reg_0[2]),.o(intermediate_reg_1[1]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_559(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1]),.i2(intermediate_reg_0[0]),.o(intermediate_reg_1[0])); 
wire [279:0]intermediate_reg_2; 
 
xor_module xor_module_inst_2_0(.clk(clk),.reset(reset),.i1(intermediate_reg_1[559]),.i2(intermediate_reg_1[558]),.o(intermediate_reg_2[279])); 
mux_module mux_module_inst_2_1(.clk(clk),.reset(reset),.i1(intermediate_reg_1[557]),.i2(intermediate_reg_1[556]),.o(intermediate_reg_2[278]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_2(.clk(clk),.reset(reset),.i1(intermediate_reg_1[555]),.i2(intermediate_reg_1[554]),.o(intermediate_reg_2[277])); 
xor_module xor_module_inst_2_3(.clk(clk),.reset(reset),.i1(intermediate_reg_1[553]),.i2(intermediate_reg_1[552]),.o(intermediate_reg_2[276])); 
xor_module xor_module_inst_2_4(.clk(clk),.reset(reset),.i1(intermediate_reg_1[551]),.i2(intermediate_reg_1[550]),.o(intermediate_reg_2[275])); 
xor_module xor_module_inst_2_5(.clk(clk),.reset(reset),.i1(intermediate_reg_1[549]),.i2(intermediate_reg_1[548]),.o(intermediate_reg_2[274])); 
mux_module mux_module_inst_2_6(.clk(clk),.reset(reset),.i1(intermediate_reg_1[547]),.i2(intermediate_reg_1[546]),.o(intermediate_reg_2[273]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_7(.clk(clk),.reset(reset),.i1(intermediate_reg_1[545]),.i2(intermediate_reg_1[544]),.o(intermediate_reg_2[272]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_8(.clk(clk),.reset(reset),.i1(intermediate_reg_1[543]),.i2(intermediate_reg_1[542]),.o(intermediate_reg_2[271])); 
mux_module mux_module_inst_2_9(.clk(clk),.reset(reset),.i1(intermediate_reg_1[541]),.i2(intermediate_reg_1[540]),.o(intermediate_reg_2[270]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_10(.clk(clk),.reset(reset),.i1(intermediate_reg_1[539]),.i2(intermediate_reg_1[538]),.o(intermediate_reg_2[269]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_11(.clk(clk),.reset(reset),.i1(intermediate_reg_1[537]),.i2(intermediate_reg_1[536]),.o(intermediate_reg_2[268]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_12(.clk(clk),.reset(reset),.i1(intermediate_reg_1[535]),.i2(intermediate_reg_1[534]),.o(intermediate_reg_2[267])); 
mux_module mux_module_inst_2_13(.clk(clk),.reset(reset),.i1(intermediate_reg_1[533]),.i2(intermediate_reg_1[532]),.o(intermediate_reg_2[266]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_14(.clk(clk),.reset(reset),.i1(intermediate_reg_1[531]),.i2(intermediate_reg_1[530]),.o(intermediate_reg_2[265]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_15(.clk(clk),.reset(reset),.i1(intermediate_reg_1[529]),.i2(intermediate_reg_1[528]),.o(intermediate_reg_2[264]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_16(.clk(clk),.reset(reset),.i1(intermediate_reg_1[527]),.i2(intermediate_reg_1[526]),.o(intermediate_reg_2[263]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_17(.clk(clk),.reset(reset),.i1(intermediate_reg_1[525]),.i2(intermediate_reg_1[524]),.o(intermediate_reg_2[262])); 
mux_module mux_module_inst_2_18(.clk(clk),.reset(reset),.i1(intermediate_reg_1[523]),.i2(intermediate_reg_1[522]),.o(intermediate_reg_2[261]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_19(.clk(clk),.reset(reset),.i1(intermediate_reg_1[521]),.i2(intermediate_reg_1[520]),.o(intermediate_reg_2[260])); 
xor_module xor_module_inst_2_20(.clk(clk),.reset(reset),.i1(intermediate_reg_1[519]),.i2(intermediate_reg_1[518]),.o(intermediate_reg_2[259])); 
mux_module mux_module_inst_2_21(.clk(clk),.reset(reset),.i1(intermediate_reg_1[517]),.i2(intermediate_reg_1[516]),.o(intermediate_reg_2[258]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_22(.clk(clk),.reset(reset),.i1(intermediate_reg_1[515]),.i2(intermediate_reg_1[514]),.o(intermediate_reg_2[257]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_23(.clk(clk),.reset(reset),.i1(intermediate_reg_1[513]),.i2(intermediate_reg_1[512]),.o(intermediate_reg_2[256]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_24(.clk(clk),.reset(reset),.i1(intermediate_reg_1[511]),.i2(intermediate_reg_1[510]),.o(intermediate_reg_2[255]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_25(.clk(clk),.reset(reset),.i1(intermediate_reg_1[509]),.i2(intermediate_reg_1[508]),.o(intermediate_reg_2[254]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_26(.clk(clk),.reset(reset),.i1(intermediate_reg_1[507]),.i2(intermediate_reg_1[506]),.o(intermediate_reg_2[253])); 
xor_module xor_module_inst_2_27(.clk(clk),.reset(reset),.i1(intermediate_reg_1[505]),.i2(intermediate_reg_1[504]),.o(intermediate_reg_2[252])); 
xor_module xor_module_inst_2_28(.clk(clk),.reset(reset),.i1(intermediate_reg_1[503]),.i2(intermediate_reg_1[502]),.o(intermediate_reg_2[251])); 
xor_module xor_module_inst_2_29(.clk(clk),.reset(reset),.i1(intermediate_reg_1[501]),.i2(intermediate_reg_1[500]),.o(intermediate_reg_2[250])); 
mux_module mux_module_inst_2_30(.clk(clk),.reset(reset),.i1(intermediate_reg_1[499]),.i2(intermediate_reg_1[498]),.o(intermediate_reg_2[249]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_31(.clk(clk),.reset(reset),.i1(intermediate_reg_1[497]),.i2(intermediate_reg_1[496]),.o(intermediate_reg_2[248])); 
mux_module mux_module_inst_2_32(.clk(clk),.reset(reset),.i1(intermediate_reg_1[495]),.i2(intermediate_reg_1[494]),.o(intermediate_reg_2[247]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_33(.clk(clk),.reset(reset),.i1(intermediate_reg_1[493]),.i2(intermediate_reg_1[492]),.o(intermediate_reg_2[246])); 
mux_module mux_module_inst_2_34(.clk(clk),.reset(reset),.i1(intermediate_reg_1[491]),.i2(intermediate_reg_1[490]),.o(intermediate_reg_2[245]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_35(.clk(clk),.reset(reset),.i1(intermediate_reg_1[489]),.i2(intermediate_reg_1[488]),.o(intermediate_reg_2[244])); 
xor_module xor_module_inst_2_36(.clk(clk),.reset(reset),.i1(intermediate_reg_1[487]),.i2(intermediate_reg_1[486]),.o(intermediate_reg_2[243])); 
mux_module mux_module_inst_2_37(.clk(clk),.reset(reset),.i1(intermediate_reg_1[485]),.i2(intermediate_reg_1[484]),.o(intermediate_reg_2[242]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_38(.clk(clk),.reset(reset),.i1(intermediate_reg_1[483]),.i2(intermediate_reg_1[482]),.o(intermediate_reg_2[241])); 
mux_module mux_module_inst_2_39(.clk(clk),.reset(reset),.i1(intermediate_reg_1[481]),.i2(intermediate_reg_1[480]),.o(intermediate_reg_2[240]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_40(.clk(clk),.reset(reset),.i1(intermediate_reg_1[479]),.i2(intermediate_reg_1[478]),.o(intermediate_reg_2[239])); 
xor_module xor_module_inst_2_41(.clk(clk),.reset(reset),.i1(intermediate_reg_1[477]),.i2(intermediate_reg_1[476]),.o(intermediate_reg_2[238])); 
mux_module mux_module_inst_2_42(.clk(clk),.reset(reset),.i1(intermediate_reg_1[475]),.i2(intermediate_reg_1[474]),.o(intermediate_reg_2[237]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_43(.clk(clk),.reset(reset),.i1(intermediate_reg_1[473]),.i2(intermediate_reg_1[472]),.o(intermediate_reg_2[236]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_44(.clk(clk),.reset(reset),.i1(intermediate_reg_1[471]),.i2(intermediate_reg_1[470]),.o(intermediate_reg_2[235])); 
mux_module mux_module_inst_2_45(.clk(clk),.reset(reset),.i1(intermediate_reg_1[469]),.i2(intermediate_reg_1[468]),.o(intermediate_reg_2[234]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_46(.clk(clk),.reset(reset),.i1(intermediate_reg_1[467]),.i2(intermediate_reg_1[466]),.o(intermediate_reg_2[233]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_47(.clk(clk),.reset(reset),.i1(intermediate_reg_1[465]),.i2(intermediate_reg_1[464]),.o(intermediate_reg_2[232]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_48(.clk(clk),.reset(reset),.i1(intermediate_reg_1[463]),.i2(intermediate_reg_1[462]),.o(intermediate_reg_2[231])); 
mux_module mux_module_inst_2_49(.clk(clk),.reset(reset),.i1(intermediate_reg_1[461]),.i2(intermediate_reg_1[460]),.o(intermediate_reg_2[230]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_50(.clk(clk),.reset(reset),.i1(intermediate_reg_1[459]),.i2(intermediate_reg_1[458]),.o(intermediate_reg_2[229]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_51(.clk(clk),.reset(reset),.i1(intermediate_reg_1[457]),.i2(intermediate_reg_1[456]),.o(intermediate_reg_2[228])); 
mux_module mux_module_inst_2_52(.clk(clk),.reset(reset),.i1(intermediate_reg_1[455]),.i2(intermediate_reg_1[454]),.o(intermediate_reg_2[227]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_53(.clk(clk),.reset(reset),.i1(intermediate_reg_1[453]),.i2(intermediate_reg_1[452]),.o(intermediate_reg_2[226])); 
mux_module mux_module_inst_2_54(.clk(clk),.reset(reset),.i1(intermediate_reg_1[451]),.i2(intermediate_reg_1[450]),.o(intermediate_reg_2[225]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_55(.clk(clk),.reset(reset),.i1(intermediate_reg_1[449]),.i2(intermediate_reg_1[448]),.o(intermediate_reg_2[224]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_56(.clk(clk),.reset(reset),.i1(intermediate_reg_1[447]),.i2(intermediate_reg_1[446]),.o(intermediate_reg_2[223]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_57(.clk(clk),.reset(reset),.i1(intermediate_reg_1[445]),.i2(intermediate_reg_1[444]),.o(intermediate_reg_2[222]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_58(.clk(clk),.reset(reset),.i1(intermediate_reg_1[443]),.i2(intermediate_reg_1[442]),.o(intermediate_reg_2[221])); 
xor_module xor_module_inst_2_59(.clk(clk),.reset(reset),.i1(intermediate_reg_1[441]),.i2(intermediate_reg_1[440]),.o(intermediate_reg_2[220])); 
xor_module xor_module_inst_2_60(.clk(clk),.reset(reset),.i1(intermediate_reg_1[439]),.i2(intermediate_reg_1[438]),.o(intermediate_reg_2[219])); 
mux_module mux_module_inst_2_61(.clk(clk),.reset(reset),.i1(intermediate_reg_1[437]),.i2(intermediate_reg_1[436]),.o(intermediate_reg_2[218]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_62(.clk(clk),.reset(reset),.i1(intermediate_reg_1[435]),.i2(intermediate_reg_1[434]),.o(intermediate_reg_2[217])); 
mux_module mux_module_inst_2_63(.clk(clk),.reset(reset),.i1(intermediate_reg_1[433]),.i2(intermediate_reg_1[432]),.o(intermediate_reg_2[216]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_64(.clk(clk),.reset(reset),.i1(intermediate_reg_1[431]),.i2(intermediate_reg_1[430]),.o(intermediate_reg_2[215])); 
mux_module mux_module_inst_2_65(.clk(clk),.reset(reset),.i1(intermediate_reg_1[429]),.i2(intermediate_reg_1[428]),.o(intermediate_reg_2[214]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_66(.clk(clk),.reset(reset),.i1(intermediate_reg_1[427]),.i2(intermediate_reg_1[426]),.o(intermediate_reg_2[213]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_67(.clk(clk),.reset(reset),.i1(intermediate_reg_1[425]),.i2(intermediate_reg_1[424]),.o(intermediate_reg_2[212])); 
mux_module mux_module_inst_2_68(.clk(clk),.reset(reset),.i1(intermediate_reg_1[423]),.i2(intermediate_reg_1[422]),.o(intermediate_reg_2[211]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_69(.clk(clk),.reset(reset),.i1(intermediate_reg_1[421]),.i2(intermediate_reg_1[420]),.o(intermediate_reg_2[210]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_70(.clk(clk),.reset(reset),.i1(intermediate_reg_1[419]),.i2(intermediate_reg_1[418]),.o(intermediate_reg_2[209])); 
xor_module xor_module_inst_2_71(.clk(clk),.reset(reset),.i1(intermediate_reg_1[417]),.i2(intermediate_reg_1[416]),.o(intermediate_reg_2[208])); 
mux_module mux_module_inst_2_72(.clk(clk),.reset(reset),.i1(intermediate_reg_1[415]),.i2(intermediate_reg_1[414]),.o(intermediate_reg_2[207]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_73(.clk(clk),.reset(reset),.i1(intermediate_reg_1[413]),.i2(intermediate_reg_1[412]),.o(intermediate_reg_2[206]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_74(.clk(clk),.reset(reset),.i1(intermediate_reg_1[411]),.i2(intermediate_reg_1[410]),.o(intermediate_reg_2[205])); 
mux_module mux_module_inst_2_75(.clk(clk),.reset(reset),.i1(intermediate_reg_1[409]),.i2(intermediate_reg_1[408]),.o(intermediate_reg_2[204]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_76(.clk(clk),.reset(reset),.i1(intermediate_reg_1[407]),.i2(intermediate_reg_1[406]),.o(intermediate_reg_2[203])); 
mux_module mux_module_inst_2_77(.clk(clk),.reset(reset),.i1(intermediate_reg_1[405]),.i2(intermediate_reg_1[404]),.o(intermediate_reg_2[202]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_78(.clk(clk),.reset(reset),.i1(intermediate_reg_1[403]),.i2(intermediate_reg_1[402]),.o(intermediate_reg_2[201])); 
mux_module mux_module_inst_2_79(.clk(clk),.reset(reset),.i1(intermediate_reg_1[401]),.i2(intermediate_reg_1[400]),.o(intermediate_reg_2[200]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_80(.clk(clk),.reset(reset),.i1(intermediate_reg_1[399]),.i2(intermediate_reg_1[398]),.o(intermediate_reg_2[199]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_81(.clk(clk),.reset(reset),.i1(intermediate_reg_1[397]),.i2(intermediate_reg_1[396]),.o(intermediate_reg_2[198]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_82(.clk(clk),.reset(reset),.i1(intermediate_reg_1[395]),.i2(intermediate_reg_1[394]),.o(intermediate_reg_2[197]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_83(.clk(clk),.reset(reset),.i1(intermediate_reg_1[393]),.i2(intermediate_reg_1[392]),.o(intermediate_reg_2[196]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_84(.clk(clk),.reset(reset),.i1(intermediate_reg_1[391]),.i2(intermediate_reg_1[390]),.o(intermediate_reg_2[195]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_85(.clk(clk),.reset(reset),.i1(intermediate_reg_1[389]),.i2(intermediate_reg_1[388]),.o(intermediate_reg_2[194])); 
xor_module xor_module_inst_2_86(.clk(clk),.reset(reset),.i1(intermediate_reg_1[387]),.i2(intermediate_reg_1[386]),.o(intermediate_reg_2[193])); 
xor_module xor_module_inst_2_87(.clk(clk),.reset(reset),.i1(intermediate_reg_1[385]),.i2(intermediate_reg_1[384]),.o(intermediate_reg_2[192])); 
mux_module mux_module_inst_2_88(.clk(clk),.reset(reset),.i1(intermediate_reg_1[383]),.i2(intermediate_reg_1[382]),.o(intermediate_reg_2[191]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_89(.clk(clk),.reset(reset),.i1(intermediate_reg_1[381]),.i2(intermediate_reg_1[380]),.o(intermediate_reg_2[190])); 
mux_module mux_module_inst_2_90(.clk(clk),.reset(reset),.i1(intermediate_reg_1[379]),.i2(intermediate_reg_1[378]),.o(intermediate_reg_2[189]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_91(.clk(clk),.reset(reset),.i1(intermediate_reg_1[377]),.i2(intermediate_reg_1[376]),.o(intermediate_reg_2[188]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_92(.clk(clk),.reset(reset),.i1(intermediate_reg_1[375]),.i2(intermediate_reg_1[374]),.o(intermediate_reg_2[187]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_93(.clk(clk),.reset(reset),.i1(intermediate_reg_1[373]),.i2(intermediate_reg_1[372]),.o(intermediate_reg_2[186])); 
mux_module mux_module_inst_2_94(.clk(clk),.reset(reset),.i1(intermediate_reg_1[371]),.i2(intermediate_reg_1[370]),.o(intermediate_reg_2[185]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_95(.clk(clk),.reset(reset),.i1(intermediate_reg_1[369]),.i2(intermediate_reg_1[368]),.o(intermediate_reg_2[184]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_96(.clk(clk),.reset(reset),.i1(intermediate_reg_1[367]),.i2(intermediate_reg_1[366]),.o(intermediate_reg_2[183]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_97(.clk(clk),.reset(reset),.i1(intermediate_reg_1[365]),.i2(intermediate_reg_1[364]),.o(intermediate_reg_2[182])); 
mux_module mux_module_inst_2_98(.clk(clk),.reset(reset),.i1(intermediate_reg_1[363]),.i2(intermediate_reg_1[362]),.o(intermediate_reg_2[181]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_99(.clk(clk),.reset(reset),.i1(intermediate_reg_1[361]),.i2(intermediate_reg_1[360]),.o(intermediate_reg_2[180])); 
xor_module xor_module_inst_2_100(.clk(clk),.reset(reset),.i1(intermediate_reg_1[359]),.i2(intermediate_reg_1[358]),.o(intermediate_reg_2[179])); 
mux_module mux_module_inst_2_101(.clk(clk),.reset(reset),.i1(intermediate_reg_1[357]),.i2(intermediate_reg_1[356]),.o(intermediate_reg_2[178]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_102(.clk(clk),.reset(reset),.i1(intermediate_reg_1[355]),.i2(intermediate_reg_1[354]),.o(intermediate_reg_2[177])); 
mux_module mux_module_inst_2_103(.clk(clk),.reset(reset),.i1(intermediate_reg_1[353]),.i2(intermediate_reg_1[352]),.o(intermediate_reg_2[176]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_104(.clk(clk),.reset(reset),.i1(intermediate_reg_1[351]),.i2(intermediate_reg_1[350]),.o(intermediate_reg_2[175]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_105(.clk(clk),.reset(reset),.i1(intermediate_reg_1[349]),.i2(intermediate_reg_1[348]),.o(intermediate_reg_2[174]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_106(.clk(clk),.reset(reset),.i1(intermediate_reg_1[347]),.i2(intermediate_reg_1[346]),.o(intermediate_reg_2[173])); 
mux_module mux_module_inst_2_107(.clk(clk),.reset(reset),.i1(intermediate_reg_1[345]),.i2(intermediate_reg_1[344]),.o(intermediate_reg_2[172]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_108(.clk(clk),.reset(reset),.i1(intermediate_reg_1[343]),.i2(intermediate_reg_1[342]),.o(intermediate_reg_2[171]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_109(.clk(clk),.reset(reset),.i1(intermediate_reg_1[341]),.i2(intermediate_reg_1[340]),.o(intermediate_reg_2[170])); 
xor_module xor_module_inst_2_110(.clk(clk),.reset(reset),.i1(intermediate_reg_1[339]),.i2(intermediate_reg_1[338]),.o(intermediate_reg_2[169])); 
mux_module mux_module_inst_2_111(.clk(clk),.reset(reset),.i1(intermediate_reg_1[337]),.i2(intermediate_reg_1[336]),.o(intermediate_reg_2[168]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_112(.clk(clk),.reset(reset),.i1(intermediate_reg_1[335]),.i2(intermediate_reg_1[334]),.o(intermediate_reg_2[167]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_113(.clk(clk),.reset(reset),.i1(intermediate_reg_1[333]),.i2(intermediate_reg_1[332]),.o(intermediate_reg_2[166]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_114(.clk(clk),.reset(reset),.i1(intermediate_reg_1[331]),.i2(intermediate_reg_1[330]),.o(intermediate_reg_2[165]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_115(.clk(clk),.reset(reset),.i1(intermediate_reg_1[329]),.i2(intermediate_reg_1[328]),.o(intermediate_reg_2[164]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_116(.clk(clk),.reset(reset),.i1(intermediate_reg_1[327]),.i2(intermediate_reg_1[326]),.o(intermediate_reg_2[163]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_117(.clk(clk),.reset(reset),.i1(intermediate_reg_1[325]),.i2(intermediate_reg_1[324]),.o(intermediate_reg_2[162])); 
xor_module xor_module_inst_2_118(.clk(clk),.reset(reset),.i1(intermediate_reg_1[323]),.i2(intermediate_reg_1[322]),.o(intermediate_reg_2[161])); 
xor_module xor_module_inst_2_119(.clk(clk),.reset(reset),.i1(intermediate_reg_1[321]),.i2(intermediate_reg_1[320]),.o(intermediate_reg_2[160])); 
mux_module mux_module_inst_2_120(.clk(clk),.reset(reset),.i1(intermediate_reg_1[319]),.i2(intermediate_reg_1[318]),.o(intermediate_reg_2[159]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_121(.clk(clk),.reset(reset),.i1(intermediate_reg_1[317]),.i2(intermediate_reg_1[316]),.o(intermediate_reg_2[158]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_122(.clk(clk),.reset(reset),.i1(intermediate_reg_1[315]),.i2(intermediate_reg_1[314]),.o(intermediate_reg_2[157])); 
mux_module mux_module_inst_2_123(.clk(clk),.reset(reset),.i1(intermediate_reg_1[313]),.i2(intermediate_reg_1[312]),.o(intermediate_reg_2[156]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_124(.clk(clk),.reset(reset),.i1(intermediate_reg_1[311]),.i2(intermediate_reg_1[310]),.o(intermediate_reg_2[155]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_125(.clk(clk),.reset(reset),.i1(intermediate_reg_1[309]),.i2(intermediate_reg_1[308]),.o(intermediate_reg_2[154]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_126(.clk(clk),.reset(reset),.i1(intermediate_reg_1[307]),.i2(intermediate_reg_1[306]),.o(intermediate_reg_2[153])); 
mux_module mux_module_inst_2_127(.clk(clk),.reset(reset),.i1(intermediate_reg_1[305]),.i2(intermediate_reg_1[304]),.o(intermediate_reg_2[152]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_128(.clk(clk),.reset(reset),.i1(intermediate_reg_1[303]),.i2(intermediate_reg_1[302]),.o(intermediate_reg_2[151])); 
mux_module mux_module_inst_2_129(.clk(clk),.reset(reset),.i1(intermediate_reg_1[301]),.i2(intermediate_reg_1[300]),.o(intermediate_reg_2[150]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_130(.clk(clk),.reset(reset),.i1(intermediate_reg_1[299]),.i2(intermediate_reg_1[298]),.o(intermediate_reg_2[149])); 
mux_module mux_module_inst_2_131(.clk(clk),.reset(reset),.i1(intermediate_reg_1[297]),.i2(intermediate_reg_1[296]),.o(intermediate_reg_2[148]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_132(.clk(clk),.reset(reset),.i1(intermediate_reg_1[295]),.i2(intermediate_reg_1[294]),.o(intermediate_reg_2[147])); 
xor_module xor_module_inst_2_133(.clk(clk),.reset(reset),.i1(intermediate_reg_1[293]),.i2(intermediate_reg_1[292]),.o(intermediate_reg_2[146])); 
xor_module xor_module_inst_2_134(.clk(clk),.reset(reset),.i1(intermediate_reg_1[291]),.i2(intermediate_reg_1[290]),.o(intermediate_reg_2[145])); 
mux_module mux_module_inst_2_135(.clk(clk),.reset(reset),.i1(intermediate_reg_1[289]),.i2(intermediate_reg_1[288]),.o(intermediate_reg_2[144]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_136(.clk(clk),.reset(reset),.i1(intermediate_reg_1[287]),.i2(intermediate_reg_1[286]),.o(intermediate_reg_2[143])); 
mux_module mux_module_inst_2_137(.clk(clk),.reset(reset),.i1(intermediate_reg_1[285]),.i2(intermediate_reg_1[284]),.o(intermediate_reg_2[142]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_138(.clk(clk),.reset(reset),.i1(intermediate_reg_1[283]),.i2(intermediate_reg_1[282]),.o(intermediate_reg_2[141])); 
mux_module mux_module_inst_2_139(.clk(clk),.reset(reset),.i1(intermediate_reg_1[281]),.i2(intermediate_reg_1[280]),.o(intermediate_reg_2[140]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_140(.clk(clk),.reset(reset),.i1(intermediate_reg_1[279]),.i2(intermediate_reg_1[278]),.o(intermediate_reg_2[139])); 
mux_module mux_module_inst_2_141(.clk(clk),.reset(reset),.i1(intermediate_reg_1[277]),.i2(intermediate_reg_1[276]),.o(intermediate_reg_2[138]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_142(.clk(clk),.reset(reset),.i1(intermediate_reg_1[275]),.i2(intermediate_reg_1[274]),.o(intermediate_reg_2[137])); 
xor_module xor_module_inst_2_143(.clk(clk),.reset(reset),.i1(intermediate_reg_1[273]),.i2(intermediate_reg_1[272]),.o(intermediate_reg_2[136])); 
mux_module mux_module_inst_2_144(.clk(clk),.reset(reset),.i1(intermediate_reg_1[271]),.i2(intermediate_reg_1[270]),.o(intermediate_reg_2[135]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_145(.clk(clk),.reset(reset),.i1(intermediate_reg_1[269]),.i2(intermediate_reg_1[268]),.o(intermediate_reg_2[134]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_146(.clk(clk),.reset(reset),.i1(intermediate_reg_1[267]),.i2(intermediate_reg_1[266]),.o(intermediate_reg_2[133])); 
mux_module mux_module_inst_2_147(.clk(clk),.reset(reset),.i1(intermediate_reg_1[265]),.i2(intermediate_reg_1[264]),.o(intermediate_reg_2[132]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_148(.clk(clk),.reset(reset),.i1(intermediate_reg_1[263]),.i2(intermediate_reg_1[262]),.o(intermediate_reg_2[131]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_149(.clk(clk),.reset(reset),.i1(intermediate_reg_1[261]),.i2(intermediate_reg_1[260]),.o(intermediate_reg_2[130]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_150(.clk(clk),.reset(reset),.i1(intermediate_reg_1[259]),.i2(intermediate_reg_1[258]),.o(intermediate_reg_2[129])); 
xor_module xor_module_inst_2_151(.clk(clk),.reset(reset),.i1(intermediate_reg_1[257]),.i2(intermediate_reg_1[256]),.o(intermediate_reg_2[128])); 
xor_module xor_module_inst_2_152(.clk(clk),.reset(reset),.i1(intermediate_reg_1[255]),.i2(intermediate_reg_1[254]),.o(intermediate_reg_2[127])); 
mux_module mux_module_inst_2_153(.clk(clk),.reset(reset),.i1(intermediate_reg_1[253]),.i2(intermediate_reg_1[252]),.o(intermediate_reg_2[126]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_154(.clk(clk),.reset(reset),.i1(intermediate_reg_1[251]),.i2(intermediate_reg_1[250]),.o(intermediate_reg_2[125]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_155(.clk(clk),.reset(reset),.i1(intermediate_reg_1[249]),.i2(intermediate_reg_1[248]),.o(intermediate_reg_2[124])); 
xor_module xor_module_inst_2_156(.clk(clk),.reset(reset),.i1(intermediate_reg_1[247]),.i2(intermediate_reg_1[246]),.o(intermediate_reg_2[123])); 
xor_module xor_module_inst_2_157(.clk(clk),.reset(reset),.i1(intermediate_reg_1[245]),.i2(intermediate_reg_1[244]),.o(intermediate_reg_2[122])); 
mux_module mux_module_inst_2_158(.clk(clk),.reset(reset),.i1(intermediate_reg_1[243]),.i2(intermediate_reg_1[242]),.o(intermediate_reg_2[121]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_159(.clk(clk),.reset(reset),.i1(intermediate_reg_1[241]),.i2(intermediate_reg_1[240]),.o(intermediate_reg_2[120])); 
xor_module xor_module_inst_2_160(.clk(clk),.reset(reset),.i1(intermediate_reg_1[239]),.i2(intermediate_reg_1[238]),.o(intermediate_reg_2[119])); 
xor_module xor_module_inst_2_161(.clk(clk),.reset(reset),.i1(intermediate_reg_1[237]),.i2(intermediate_reg_1[236]),.o(intermediate_reg_2[118])); 
xor_module xor_module_inst_2_162(.clk(clk),.reset(reset),.i1(intermediate_reg_1[235]),.i2(intermediate_reg_1[234]),.o(intermediate_reg_2[117])); 
mux_module mux_module_inst_2_163(.clk(clk),.reset(reset),.i1(intermediate_reg_1[233]),.i2(intermediate_reg_1[232]),.o(intermediate_reg_2[116]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_164(.clk(clk),.reset(reset),.i1(intermediate_reg_1[231]),.i2(intermediate_reg_1[230]),.o(intermediate_reg_2[115])); 
mux_module mux_module_inst_2_165(.clk(clk),.reset(reset),.i1(intermediate_reg_1[229]),.i2(intermediate_reg_1[228]),.o(intermediate_reg_2[114]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_166(.clk(clk),.reset(reset),.i1(intermediate_reg_1[227]),.i2(intermediate_reg_1[226]),.o(intermediate_reg_2[113])); 
mux_module mux_module_inst_2_167(.clk(clk),.reset(reset),.i1(intermediate_reg_1[225]),.i2(intermediate_reg_1[224]),.o(intermediate_reg_2[112]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_168(.clk(clk),.reset(reset),.i1(intermediate_reg_1[223]),.i2(intermediate_reg_1[222]),.o(intermediate_reg_2[111])); 
mux_module mux_module_inst_2_169(.clk(clk),.reset(reset),.i1(intermediate_reg_1[221]),.i2(intermediate_reg_1[220]),.o(intermediate_reg_2[110]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_170(.clk(clk),.reset(reset),.i1(intermediate_reg_1[219]),.i2(intermediate_reg_1[218]),.o(intermediate_reg_2[109]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_171(.clk(clk),.reset(reset),.i1(intermediate_reg_1[217]),.i2(intermediate_reg_1[216]),.o(intermediate_reg_2[108]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_172(.clk(clk),.reset(reset),.i1(intermediate_reg_1[215]),.i2(intermediate_reg_1[214]),.o(intermediate_reg_2[107]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_173(.clk(clk),.reset(reset),.i1(intermediate_reg_1[213]),.i2(intermediate_reg_1[212]),.o(intermediate_reg_2[106])); 
mux_module mux_module_inst_2_174(.clk(clk),.reset(reset),.i1(intermediate_reg_1[211]),.i2(intermediate_reg_1[210]),.o(intermediate_reg_2[105]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_175(.clk(clk),.reset(reset),.i1(intermediate_reg_1[209]),.i2(intermediate_reg_1[208]),.o(intermediate_reg_2[104]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_176(.clk(clk),.reset(reset),.i1(intermediate_reg_1[207]),.i2(intermediate_reg_1[206]),.o(intermediate_reg_2[103])); 
xor_module xor_module_inst_2_177(.clk(clk),.reset(reset),.i1(intermediate_reg_1[205]),.i2(intermediate_reg_1[204]),.o(intermediate_reg_2[102])); 
xor_module xor_module_inst_2_178(.clk(clk),.reset(reset),.i1(intermediate_reg_1[203]),.i2(intermediate_reg_1[202]),.o(intermediate_reg_2[101])); 
xor_module xor_module_inst_2_179(.clk(clk),.reset(reset),.i1(intermediate_reg_1[201]),.i2(intermediate_reg_1[200]),.o(intermediate_reg_2[100])); 
mux_module mux_module_inst_2_180(.clk(clk),.reset(reset),.i1(intermediate_reg_1[199]),.i2(intermediate_reg_1[198]),.o(intermediate_reg_2[99]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_181(.clk(clk),.reset(reset),.i1(intermediate_reg_1[197]),.i2(intermediate_reg_1[196]),.o(intermediate_reg_2[98]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_182(.clk(clk),.reset(reset),.i1(intermediate_reg_1[195]),.i2(intermediate_reg_1[194]),.o(intermediate_reg_2[97]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_183(.clk(clk),.reset(reset),.i1(intermediate_reg_1[193]),.i2(intermediate_reg_1[192]),.o(intermediate_reg_2[96]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_184(.clk(clk),.reset(reset),.i1(intermediate_reg_1[191]),.i2(intermediate_reg_1[190]),.o(intermediate_reg_2[95])); 
xor_module xor_module_inst_2_185(.clk(clk),.reset(reset),.i1(intermediate_reg_1[189]),.i2(intermediate_reg_1[188]),.o(intermediate_reg_2[94])); 
xor_module xor_module_inst_2_186(.clk(clk),.reset(reset),.i1(intermediate_reg_1[187]),.i2(intermediate_reg_1[186]),.o(intermediate_reg_2[93])); 
xor_module xor_module_inst_2_187(.clk(clk),.reset(reset),.i1(intermediate_reg_1[185]),.i2(intermediate_reg_1[184]),.o(intermediate_reg_2[92])); 
xor_module xor_module_inst_2_188(.clk(clk),.reset(reset),.i1(intermediate_reg_1[183]),.i2(intermediate_reg_1[182]),.o(intermediate_reg_2[91])); 
mux_module mux_module_inst_2_189(.clk(clk),.reset(reset),.i1(intermediate_reg_1[181]),.i2(intermediate_reg_1[180]),.o(intermediate_reg_2[90]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_190(.clk(clk),.reset(reset),.i1(intermediate_reg_1[179]),.i2(intermediate_reg_1[178]),.o(intermediate_reg_2[89]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_191(.clk(clk),.reset(reset),.i1(intermediate_reg_1[177]),.i2(intermediate_reg_1[176]),.o(intermediate_reg_2[88])); 
mux_module mux_module_inst_2_192(.clk(clk),.reset(reset),.i1(intermediate_reg_1[175]),.i2(intermediate_reg_1[174]),.o(intermediate_reg_2[87]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_193(.clk(clk),.reset(reset),.i1(intermediate_reg_1[173]),.i2(intermediate_reg_1[172]),.o(intermediate_reg_2[86]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_194(.clk(clk),.reset(reset),.i1(intermediate_reg_1[171]),.i2(intermediate_reg_1[170]),.o(intermediate_reg_2[85]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_195(.clk(clk),.reset(reset),.i1(intermediate_reg_1[169]),.i2(intermediate_reg_1[168]),.o(intermediate_reg_2[84])); 
mux_module mux_module_inst_2_196(.clk(clk),.reset(reset),.i1(intermediate_reg_1[167]),.i2(intermediate_reg_1[166]),.o(intermediate_reg_2[83]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_197(.clk(clk),.reset(reset),.i1(intermediate_reg_1[165]),.i2(intermediate_reg_1[164]),.o(intermediate_reg_2[82])); 
xor_module xor_module_inst_2_198(.clk(clk),.reset(reset),.i1(intermediate_reg_1[163]),.i2(intermediate_reg_1[162]),.o(intermediate_reg_2[81])); 
mux_module mux_module_inst_2_199(.clk(clk),.reset(reset),.i1(intermediate_reg_1[161]),.i2(intermediate_reg_1[160]),.o(intermediate_reg_2[80]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_200(.clk(clk),.reset(reset),.i1(intermediate_reg_1[159]),.i2(intermediate_reg_1[158]),.o(intermediate_reg_2[79]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_201(.clk(clk),.reset(reset),.i1(intermediate_reg_1[157]),.i2(intermediate_reg_1[156]),.o(intermediate_reg_2[78]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_202(.clk(clk),.reset(reset),.i1(intermediate_reg_1[155]),.i2(intermediate_reg_1[154]),.o(intermediate_reg_2[77])); 
xor_module xor_module_inst_2_203(.clk(clk),.reset(reset),.i1(intermediate_reg_1[153]),.i2(intermediate_reg_1[152]),.o(intermediate_reg_2[76])); 
mux_module mux_module_inst_2_204(.clk(clk),.reset(reset),.i1(intermediate_reg_1[151]),.i2(intermediate_reg_1[150]),.o(intermediate_reg_2[75]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_205(.clk(clk),.reset(reset),.i1(intermediate_reg_1[149]),.i2(intermediate_reg_1[148]),.o(intermediate_reg_2[74])); 
mux_module mux_module_inst_2_206(.clk(clk),.reset(reset),.i1(intermediate_reg_1[147]),.i2(intermediate_reg_1[146]),.o(intermediate_reg_2[73]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_207(.clk(clk),.reset(reset),.i1(intermediate_reg_1[145]),.i2(intermediate_reg_1[144]),.o(intermediate_reg_2[72])); 
xor_module xor_module_inst_2_208(.clk(clk),.reset(reset),.i1(intermediate_reg_1[143]),.i2(intermediate_reg_1[142]),.o(intermediate_reg_2[71])); 
xor_module xor_module_inst_2_209(.clk(clk),.reset(reset),.i1(intermediate_reg_1[141]),.i2(intermediate_reg_1[140]),.o(intermediate_reg_2[70])); 
mux_module mux_module_inst_2_210(.clk(clk),.reset(reset),.i1(intermediate_reg_1[139]),.i2(intermediate_reg_1[138]),.o(intermediate_reg_2[69]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_211(.clk(clk),.reset(reset),.i1(intermediate_reg_1[137]),.i2(intermediate_reg_1[136]),.o(intermediate_reg_2[68]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_212(.clk(clk),.reset(reset),.i1(intermediate_reg_1[135]),.i2(intermediate_reg_1[134]),.o(intermediate_reg_2[67]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_213(.clk(clk),.reset(reset),.i1(intermediate_reg_1[133]),.i2(intermediate_reg_1[132]),.o(intermediate_reg_2[66])); 
xor_module xor_module_inst_2_214(.clk(clk),.reset(reset),.i1(intermediate_reg_1[131]),.i2(intermediate_reg_1[130]),.o(intermediate_reg_2[65])); 
mux_module mux_module_inst_2_215(.clk(clk),.reset(reset),.i1(intermediate_reg_1[129]),.i2(intermediate_reg_1[128]),.o(intermediate_reg_2[64]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_216(.clk(clk),.reset(reset),.i1(intermediate_reg_1[127]),.i2(intermediate_reg_1[126]),.o(intermediate_reg_2[63]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_217(.clk(clk),.reset(reset),.i1(intermediate_reg_1[125]),.i2(intermediate_reg_1[124]),.o(intermediate_reg_2[62]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_218(.clk(clk),.reset(reset),.i1(intermediate_reg_1[123]),.i2(intermediate_reg_1[122]),.o(intermediate_reg_2[61])); 
mux_module mux_module_inst_2_219(.clk(clk),.reset(reset),.i1(intermediate_reg_1[121]),.i2(intermediate_reg_1[120]),.o(intermediate_reg_2[60]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_220(.clk(clk),.reset(reset),.i1(intermediate_reg_1[119]),.i2(intermediate_reg_1[118]),.o(intermediate_reg_2[59])); 
xor_module xor_module_inst_2_221(.clk(clk),.reset(reset),.i1(intermediate_reg_1[117]),.i2(intermediate_reg_1[116]),.o(intermediate_reg_2[58])); 
xor_module xor_module_inst_2_222(.clk(clk),.reset(reset),.i1(intermediate_reg_1[115]),.i2(intermediate_reg_1[114]),.o(intermediate_reg_2[57])); 
mux_module mux_module_inst_2_223(.clk(clk),.reset(reset),.i1(intermediate_reg_1[113]),.i2(intermediate_reg_1[112]),.o(intermediate_reg_2[56]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_224(.clk(clk),.reset(reset),.i1(intermediate_reg_1[111]),.i2(intermediate_reg_1[110]),.o(intermediate_reg_2[55])); 
mux_module mux_module_inst_2_225(.clk(clk),.reset(reset),.i1(intermediate_reg_1[109]),.i2(intermediate_reg_1[108]),.o(intermediate_reg_2[54]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_226(.clk(clk),.reset(reset),.i1(intermediate_reg_1[107]),.i2(intermediate_reg_1[106]),.o(intermediate_reg_2[53]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_227(.clk(clk),.reset(reset),.i1(intermediate_reg_1[105]),.i2(intermediate_reg_1[104]),.o(intermediate_reg_2[52]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_228(.clk(clk),.reset(reset),.i1(intermediate_reg_1[103]),.i2(intermediate_reg_1[102]),.o(intermediate_reg_2[51]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_229(.clk(clk),.reset(reset),.i1(intermediate_reg_1[101]),.i2(intermediate_reg_1[100]),.o(intermediate_reg_2[50])); 
mux_module mux_module_inst_2_230(.clk(clk),.reset(reset),.i1(intermediate_reg_1[99]),.i2(intermediate_reg_1[98]),.o(intermediate_reg_2[49]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_231(.clk(clk),.reset(reset),.i1(intermediate_reg_1[97]),.i2(intermediate_reg_1[96]),.o(intermediate_reg_2[48]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_232(.clk(clk),.reset(reset),.i1(intermediate_reg_1[95]),.i2(intermediate_reg_1[94]),.o(intermediate_reg_2[47]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_233(.clk(clk),.reset(reset),.i1(intermediate_reg_1[93]),.i2(intermediate_reg_1[92]),.o(intermediate_reg_2[46])); 
mux_module mux_module_inst_2_234(.clk(clk),.reset(reset),.i1(intermediate_reg_1[91]),.i2(intermediate_reg_1[90]),.o(intermediate_reg_2[45]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_235(.clk(clk),.reset(reset),.i1(intermediate_reg_1[89]),.i2(intermediate_reg_1[88]),.o(intermediate_reg_2[44])); 
mux_module mux_module_inst_2_236(.clk(clk),.reset(reset),.i1(intermediate_reg_1[87]),.i2(intermediate_reg_1[86]),.o(intermediate_reg_2[43]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_237(.clk(clk),.reset(reset),.i1(intermediate_reg_1[85]),.i2(intermediate_reg_1[84]),.o(intermediate_reg_2[42]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_238(.clk(clk),.reset(reset),.i1(intermediate_reg_1[83]),.i2(intermediate_reg_1[82]),.o(intermediate_reg_2[41]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_239(.clk(clk),.reset(reset),.i1(intermediate_reg_1[81]),.i2(intermediate_reg_1[80]),.o(intermediate_reg_2[40])); 
xor_module xor_module_inst_2_240(.clk(clk),.reset(reset),.i1(intermediate_reg_1[79]),.i2(intermediate_reg_1[78]),.o(intermediate_reg_2[39])); 
xor_module xor_module_inst_2_241(.clk(clk),.reset(reset),.i1(intermediate_reg_1[77]),.i2(intermediate_reg_1[76]),.o(intermediate_reg_2[38])); 
xor_module xor_module_inst_2_242(.clk(clk),.reset(reset),.i1(intermediate_reg_1[75]),.i2(intermediate_reg_1[74]),.o(intermediate_reg_2[37])); 
xor_module xor_module_inst_2_243(.clk(clk),.reset(reset),.i1(intermediate_reg_1[73]),.i2(intermediate_reg_1[72]),.o(intermediate_reg_2[36])); 
mux_module mux_module_inst_2_244(.clk(clk),.reset(reset),.i1(intermediate_reg_1[71]),.i2(intermediate_reg_1[70]),.o(intermediate_reg_2[35]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_245(.clk(clk),.reset(reset),.i1(intermediate_reg_1[69]),.i2(intermediate_reg_1[68]),.o(intermediate_reg_2[34])); 
xor_module xor_module_inst_2_246(.clk(clk),.reset(reset),.i1(intermediate_reg_1[67]),.i2(intermediate_reg_1[66]),.o(intermediate_reg_2[33])); 
mux_module mux_module_inst_2_247(.clk(clk),.reset(reset),.i1(intermediate_reg_1[65]),.i2(intermediate_reg_1[64]),.o(intermediate_reg_2[32]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_248(.clk(clk),.reset(reset),.i1(intermediate_reg_1[63]),.i2(intermediate_reg_1[62]),.o(intermediate_reg_2[31])); 
xor_module xor_module_inst_2_249(.clk(clk),.reset(reset),.i1(intermediate_reg_1[61]),.i2(intermediate_reg_1[60]),.o(intermediate_reg_2[30])); 
mux_module mux_module_inst_2_250(.clk(clk),.reset(reset),.i1(intermediate_reg_1[59]),.i2(intermediate_reg_1[58]),.o(intermediate_reg_2[29]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_251(.clk(clk),.reset(reset),.i1(intermediate_reg_1[57]),.i2(intermediate_reg_1[56]),.o(intermediate_reg_2[28])); 
mux_module mux_module_inst_2_252(.clk(clk),.reset(reset),.i1(intermediate_reg_1[55]),.i2(intermediate_reg_1[54]),.o(intermediate_reg_2[27]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_253(.clk(clk),.reset(reset),.i1(intermediate_reg_1[53]),.i2(intermediate_reg_1[52]),.o(intermediate_reg_2[26])); 
xor_module xor_module_inst_2_254(.clk(clk),.reset(reset),.i1(intermediate_reg_1[51]),.i2(intermediate_reg_1[50]),.o(intermediate_reg_2[25])); 
mux_module mux_module_inst_2_255(.clk(clk),.reset(reset),.i1(intermediate_reg_1[49]),.i2(intermediate_reg_1[48]),.o(intermediate_reg_2[24]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_256(.clk(clk),.reset(reset),.i1(intermediate_reg_1[47]),.i2(intermediate_reg_1[46]),.o(intermediate_reg_2[23]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_257(.clk(clk),.reset(reset),.i1(intermediate_reg_1[45]),.i2(intermediate_reg_1[44]),.o(intermediate_reg_2[22])); 
mux_module mux_module_inst_2_258(.clk(clk),.reset(reset),.i1(intermediate_reg_1[43]),.i2(intermediate_reg_1[42]),.o(intermediate_reg_2[21]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_259(.clk(clk),.reset(reset),.i1(intermediate_reg_1[41]),.i2(intermediate_reg_1[40]),.o(intermediate_reg_2[20]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_260(.clk(clk),.reset(reset),.i1(intermediate_reg_1[39]),.i2(intermediate_reg_1[38]),.o(intermediate_reg_2[19]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_261(.clk(clk),.reset(reset),.i1(intermediate_reg_1[37]),.i2(intermediate_reg_1[36]),.o(intermediate_reg_2[18])); 
xor_module xor_module_inst_2_262(.clk(clk),.reset(reset),.i1(intermediate_reg_1[35]),.i2(intermediate_reg_1[34]),.o(intermediate_reg_2[17])); 
mux_module mux_module_inst_2_263(.clk(clk),.reset(reset),.i1(intermediate_reg_1[33]),.i2(intermediate_reg_1[32]),.o(intermediate_reg_2[16]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_264(.clk(clk),.reset(reset),.i1(intermediate_reg_1[31]),.i2(intermediate_reg_1[30]),.o(intermediate_reg_2[15]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_265(.clk(clk),.reset(reset),.i1(intermediate_reg_1[29]),.i2(intermediate_reg_1[28]),.o(intermediate_reg_2[14]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_266(.clk(clk),.reset(reset),.i1(intermediate_reg_1[27]),.i2(intermediate_reg_1[26]),.o(intermediate_reg_2[13]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_267(.clk(clk),.reset(reset),.i1(intermediate_reg_1[25]),.i2(intermediate_reg_1[24]),.o(intermediate_reg_2[12]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_268(.clk(clk),.reset(reset),.i1(intermediate_reg_1[23]),.i2(intermediate_reg_1[22]),.o(intermediate_reg_2[11]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_269(.clk(clk),.reset(reset),.i1(intermediate_reg_1[21]),.i2(intermediate_reg_1[20]),.o(intermediate_reg_2[10])); 
xor_module xor_module_inst_2_270(.clk(clk),.reset(reset),.i1(intermediate_reg_1[19]),.i2(intermediate_reg_1[18]),.o(intermediate_reg_2[9])); 
xor_module xor_module_inst_2_271(.clk(clk),.reset(reset),.i1(intermediate_reg_1[17]),.i2(intermediate_reg_1[16]),.o(intermediate_reg_2[8])); 
xor_module xor_module_inst_2_272(.clk(clk),.reset(reset),.i1(intermediate_reg_1[15]),.i2(intermediate_reg_1[14]),.o(intermediate_reg_2[7])); 
xor_module xor_module_inst_2_273(.clk(clk),.reset(reset),.i1(intermediate_reg_1[13]),.i2(intermediate_reg_1[12]),.o(intermediate_reg_2[6])); 
xor_module xor_module_inst_2_274(.clk(clk),.reset(reset),.i1(intermediate_reg_1[11]),.i2(intermediate_reg_1[10]),.o(intermediate_reg_2[5])); 
mux_module mux_module_inst_2_275(.clk(clk),.reset(reset),.i1(intermediate_reg_1[9]),.i2(intermediate_reg_1[8]),.o(intermediate_reg_2[4]),.sel(intermediate_reg_1[0])); 
mux_module mux_module_inst_2_276(.clk(clk),.reset(reset),.i1(intermediate_reg_1[7]),.i2(intermediate_reg_1[6]),.o(intermediate_reg_2[3]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_277(.clk(clk),.reset(reset),.i1(intermediate_reg_1[5]),.i2(intermediate_reg_1[4]),.o(intermediate_reg_2[2])); 
mux_module mux_module_inst_2_278(.clk(clk),.reset(reset),.i1(intermediate_reg_1[3]),.i2(intermediate_reg_1[2]),.o(intermediate_reg_2[1]),.sel(intermediate_reg_1[0])); 
xor_module xor_module_inst_2_279(.clk(clk),.reset(reset),.i1(intermediate_reg_1[1]),.i2(intermediate_reg_1[0]),.o(intermediate_reg_2[0])); 
always@(posedge clk) begin 
outp [279:0] <= intermediate_reg_2; 
outp[383:280] <= intermediate_reg_2[103:0] ; 
end 
endmodule 
 

module interface_12(input [159:0] inp, output reg [511:0] outp, input clk, input reset);
always@(posedge clk) begin 
outp[159:0] <= inp ; 
outp[319:160] <= inp ; 
outp[479:320] <= inp ; 
outp[511:480] <= inp[31:0] ; 
end 
endmodule 

module interface_13(input [255:0] inp, output reg [785:0] outp, input clk, input reset);
always@(posedge clk) begin 
outp[255:0] <= inp ; 
outp[511:256] <= inp ; 
outp[767:512] <= inp ; 
outp[785:768] <= inp[17:0] ; 
end 
endmodule 

module interface_14(input [433:0] inp, output reg [415:0] outp, input clk, input reset);
reg [433:0]intermediate_reg_0; 
always@(posedge clk) begin 
intermediate_reg_0 <= inp; 
end 
 
wire [216:0]intermediate_reg_1; 
 
mux_module mux_module_inst_1_0(.clk(clk),.reset(reset),.i1(intermediate_reg_0[433]),.i2(intermediate_reg_0[432]),.o(intermediate_reg_1[216]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_1(.clk(clk),.reset(reset),.i1(intermediate_reg_0[431]),.i2(intermediate_reg_0[430]),.o(intermediate_reg_1[215])); 
xor_module xor_module_inst_1_2(.clk(clk),.reset(reset),.i1(intermediate_reg_0[429]),.i2(intermediate_reg_0[428]),.o(intermediate_reg_1[214])); 
xor_module xor_module_inst_1_3(.clk(clk),.reset(reset),.i1(intermediate_reg_0[427]),.i2(intermediate_reg_0[426]),.o(intermediate_reg_1[213])); 
xor_module xor_module_inst_1_4(.clk(clk),.reset(reset),.i1(intermediate_reg_0[425]),.i2(intermediate_reg_0[424]),.o(intermediate_reg_1[212])); 
mux_module mux_module_inst_1_5(.clk(clk),.reset(reset),.i1(intermediate_reg_0[423]),.i2(intermediate_reg_0[422]),.o(intermediate_reg_1[211]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_6(.clk(clk),.reset(reset),.i1(intermediate_reg_0[421]),.i2(intermediate_reg_0[420]),.o(intermediate_reg_1[210])); 
mux_module mux_module_inst_1_7(.clk(clk),.reset(reset),.i1(intermediate_reg_0[419]),.i2(intermediate_reg_0[418]),.o(intermediate_reg_1[209]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_8(.clk(clk),.reset(reset),.i1(intermediate_reg_0[417]),.i2(intermediate_reg_0[416]),.o(intermediate_reg_1[208])); 
xor_module xor_module_inst_1_9(.clk(clk),.reset(reset),.i1(intermediate_reg_0[415]),.i2(intermediate_reg_0[414]),.o(intermediate_reg_1[207])); 
mux_module mux_module_inst_1_10(.clk(clk),.reset(reset),.i1(intermediate_reg_0[413]),.i2(intermediate_reg_0[412]),.o(intermediate_reg_1[206]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_11(.clk(clk),.reset(reset),.i1(intermediate_reg_0[411]),.i2(intermediate_reg_0[410]),.o(intermediate_reg_1[205]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_12(.clk(clk),.reset(reset),.i1(intermediate_reg_0[409]),.i2(intermediate_reg_0[408]),.o(intermediate_reg_1[204])); 
mux_module mux_module_inst_1_13(.clk(clk),.reset(reset),.i1(intermediate_reg_0[407]),.i2(intermediate_reg_0[406]),.o(intermediate_reg_1[203]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_14(.clk(clk),.reset(reset),.i1(intermediate_reg_0[405]),.i2(intermediate_reg_0[404]),.o(intermediate_reg_1[202])); 
xor_module xor_module_inst_1_15(.clk(clk),.reset(reset),.i1(intermediate_reg_0[403]),.i2(intermediate_reg_0[402]),.o(intermediate_reg_1[201])); 
xor_module xor_module_inst_1_16(.clk(clk),.reset(reset),.i1(intermediate_reg_0[401]),.i2(intermediate_reg_0[400]),.o(intermediate_reg_1[200])); 
xor_module xor_module_inst_1_17(.clk(clk),.reset(reset),.i1(intermediate_reg_0[399]),.i2(intermediate_reg_0[398]),.o(intermediate_reg_1[199])); 
mux_module mux_module_inst_1_18(.clk(clk),.reset(reset),.i1(intermediate_reg_0[397]),.i2(intermediate_reg_0[396]),.o(intermediate_reg_1[198]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_19(.clk(clk),.reset(reset),.i1(intermediate_reg_0[395]),.i2(intermediate_reg_0[394]),.o(intermediate_reg_1[197])); 
mux_module mux_module_inst_1_20(.clk(clk),.reset(reset),.i1(intermediate_reg_0[393]),.i2(intermediate_reg_0[392]),.o(intermediate_reg_1[196]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_21(.clk(clk),.reset(reset),.i1(intermediate_reg_0[391]),.i2(intermediate_reg_0[390]),.o(intermediate_reg_1[195]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_22(.clk(clk),.reset(reset),.i1(intermediate_reg_0[389]),.i2(intermediate_reg_0[388]),.o(intermediate_reg_1[194]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_23(.clk(clk),.reset(reset),.i1(intermediate_reg_0[387]),.i2(intermediate_reg_0[386]),.o(intermediate_reg_1[193]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_24(.clk(clk),.reset(reset),.i1(intermediate_reg_0[385]),.i2(intermediate_reg_0[384]),.o(intermediate_reg_1[192])); 
mux_module mux_module_inst_1_25(.clk(clk),.reset(reset),.i1(intermediate_reg_0[383]),.i2(intermediate_reg_0[382]),.o(intermediate_reg_1[191]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_26(.clk(clk),.reset(reset),.i1(intermediate_reg_0[381]),.i2(intermediate_reg_0[380]),.o(intermediate_reg_1[190])); 
mux_module mux_module_inst_1_27(.clk(clk),.reset(reset),.i1(intermediate_reg_0[379]),.i2(intermediate_reg_0[378]),.o(intermediate_reg_1[189]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_28(.clk(clk),.reset(reset),.i1(intermediate_reg_0[377]),.i2(intermediate_reg_0[376]),.o(intermediate_reg_1[188])); 
mux_module mux_module_inst_1_29(.clk(clk),.reset(reset),.i1(intermediate_reg_0[375]),.i2(intermediate_reg_0[374]),.o(intermediate_reg_1[187]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_30(.clk(clk),.reset(reset),.i1(intermediate_reg_0[373]),.i2(intermediate_reg_0[372]),.o(intermediate_reg_1[186]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_31(.clk(clk),.reset(reset),.i1(intermediate_reg_0[371]),.i2(intermediate_reg_0[370]),.o(intermediate_reg_1[185])); 
mux_module mux_module_inst_1_32(.clk(clk),.reset(reset),.i1(intermediate_reg_0[369]),.i2(intermediate_reg_0[368]),.o(intermediate_reg_1[184]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_33(.clk(clk),.reset(reset),.i1(intermediate_reg_0[367]),.i2(intermediate_reg_0[366]),.o(intermediate_reg_1[183])); 
xor_module xor_module_inst_1_34(.clk(clk),.reset(reset),.i1(intermediate_reg_0[365]),.i2(intermediate_reg_0[364]),.o(intermediate_reg_1[182])); 
mux_module mux_module_inst_1_35(.clk(clk),.reset(reset),.i1(intermediate_reg_0[363]),.i2(intermediate_reg_0[362]),.o(intermediate_reg_1[181]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_36(.clk(clk),.reset(reset),.i1(intermediate_reg_0[361]),.i2(intermediate_reg_0[360]),.o(intermediate_reg_1[180]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_37(.clk(clk),.reset(reset),.i1(intermediate_reg_0[359]),.i2(intermediate_reg_0[358]),.o(intermediate_reg_1[179])); 
xor_module xor_module_inst_1_38(.clk(clk),.reset(reset),.i1(intermediate_reg_0[357]),.i2(intermediate_reg_0[356]),.o(intermediate_reg_1[178])); 
mux_module mux_module_inst_1_39(.clk(clk),.reset(reset),.i1(intermediate_reg_0[355]),.i2(intermediate_reg_0[354]),.o(intermediate_reg_1[177]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_40(.clk(clk),.reset(reset),.i1(intermediate_reg_0[353]),.i2(intermediate_reg_0[352]),.o(intermediate_reg_1[176])); 
xor_module xor_module_inst_1_41(.clk(clk),.reset(reset),.i1(intermediate_reg_0[351]),.i2(intermediate_reg_0[350]),.o(intermediate_reg_1[175])); 
mux_module mux_module_inst_1_42(.clk(clk),.reset(reset),.i1(intermediate_reg_0[349]),.i2(intermediate_reg_0[348]),.o(intermediate_reg_1[174]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_43(.clk(clk),.reset(reset),.i1(intermediate_reg_0[347]),.i2(intermediate_reg_0[346]),.o(intermediate_reg_1[173])); 
xor_module xor_module_inst_1_44(.clk(clk),.reset(reset),.i1(intermediate_reg_0[345]),.i2(intermediate_reg_0[344]),.o(intermediate_reg_1[172])); 
xor_module xor_module_inst_1_45(.clk(clk),.reset(reset),.i1(intermediate_reg_0[343]),.i2(intermediate_reg_0[342]),.o(intermediate_reg_1[171])); 
mux_module mux_module_inst_1_46(.clk(clk),.reset(reset),.i1(intermediate_reg_0[341]),.i2(intermediate_reg_0[340]),.o(intermediate_reg_1[170]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_47(.clk(clk),.reset(reset),.i1(intermediate_reg_0[339]),.i2(intermediate_reg_0[338]),.o(intermediate_reg_1[169]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_48(.clk(clk),.reset(reset),.i1(intermediate_reg_0[337]),.i2(intermediate_reg_0[336]),.o(intermediate_reg_1[168])); 
xor_module xor_module_inst_1_49(.clk(clk),.reset(reset),.i1(intermediate_reg_0[335]),.i2(intermediate_reg_0[334]),.o(intermediate_reg_1[167])); 
xor_module xor_module_inst_1_50(.clk(clk),.reset(reset),.i1(intermediate_reg_0[333]),.i2(intermediate_reg_0[332]),.o(intermediate_reg_1[166])); 
xor_module xor_module_inst_1_51(.clk(clk),.reset(reset),.i1(intermediate_reg_0[331]),.i2(intermediate_reg_0[330]),.o(intermediate_reg_1[165])); 
xor_module xor_module_inst_1_52(.clk(clk),.reset(reset),.i1(intermediate_reg_0[329]),.i2(intermediate_reg_0[328]),.o(intermediate_reg_1[164])); 
xor_module xor_module_inst_1_53(.clk(clk),.reset(reset),.i1(intermediate_reg_0[327]),.i2(intermediate_reg_0[326]),.o(intermediate_reg_1[163])); 
xor_module xor_module_inst_1_54(.clk(clk),.reset(reset),.i1(intermediate_reg_0[325]),.i2(intermediate_reg_0[324]),.o(intermediate_reg_1[162])); 
mux_module mux_module_inst_1_55(.clk(clk),.reset(reset),.i1(intermediate_reg_0[323]),.i2(intermediate_reg_0[322]),.o(intermediate_reg_1[161]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_56(.clk(clk),.reset(reset),.i1(intermediate_reg_0[321]),.i2(intermediate_reg_0[320]),.o(intermediate_reg_1[160]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_57(.clk(clk),.reset(reset),.i1(intermediate_reg_0[319]),.i2(intermediate_reg_0[318]),.o(intermediate_reg_1[159]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_58(.clk(clk),.reset(reset),.i1(intermediate_reg_0[317]),.i2(intermediate_reg_0[316]),.o(intermediate_reg_1[158]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_59(.clk(clk),.reset(reset),.i1(intermediate_reg_0[315]),.i2(intermediate_reg_0[314]),.o(intermediate_reg_1[157]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_60(.clk(clk),.reset(reset),.i1(intermediate_reg_0[313]),.i2(intermediate_reg_0[312]),.o(intermediate_reg_1[156]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_61(.clk(clk),.reset(reset),.i1(intermediate_reg_0[311]),.i2(intermediate_reg_0[310]),.o(intermediate_reg_1[155])); 
xor_module xor_module_inst_1_62(.clk(clk),.reset(reset),.i1(intermediate_reg_0[309]),.i2(intermediate_reg_0[308]),.o(intermediate_reg_1[154])); 
xor_module xor_module_inst_1_63(.clk(clk),.reset(reset),.i1(intermediate_reg_0[307]),.i2(intermediate_reg_0[306]),.o(intermediate_reg_1[153])); 
xor_module xor_module_inst_1_64(.clk(clk),.reset(reset),.i1(intermediate_reg_0[305]),.i2(intermediate_reg_0[304]),.o(intermediate_reg_1[152])); 
mux_module mux_module_inst_1_65(.clk(clk),.reset(reset),.i1(intermediate_reg_0[303]),.i2(intermediate_reg_0[302]),.o(intermediate_reg_1[151]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_66(.clk(clk),.reset(reset),.i1(intermediate_reg_0[301]),.i2(intermediate_reg_0[300]),.o(intermediate_reg_1[150])); 
xor_module xor_module_inst_1_67(.clk(clk),.reset(reset),.i1(intermediate_reg_0[299]),.i2(intermediate_reg_0[298]),.o(intermediate_reg_1[149])); 
mux_module mux_module_inst_1_68(.clk(clk),.reset(reset),.i1(intermediate_reg_0[297]),.i2(intermediate_reg_0[296]),.o(intermediate_reg_1[148]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_69(.clk(clk),.reset(reset),.i1(intermediate_reg_0[295]),.i2(intermediate_reg_0[294]),.o(intermediate_reg_1[147]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_70(.clk(clk),.reset(reset),.i1(intermediate_reg_0[293]),.i2(intermediate_reg_0[292]),.o(intermediate_reg_1[146]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_71(.clk(clk),.reset(reset),.i1(intermediate_reg_0[291]),.i2(intermediate_reg_0[290]),.o(intermediate_reg_1[145])); 
xor_module xor_module_inst_1_72(.clk(clk),.reset(reset),.i1(intermediate_reg_0[289]),.i2(intermediate_reg_0[288]),.o(intermediate_reg_1[144])); 
xor_module xor_module_inst_1_73(.clk(clk),.reset(reset),.i1(intermediate_reg_0[287]),.i2(intermediate_reg_0[286]),.o(intermediate_reg_1[143])); 
xor_module xor_module_inst_1_74(.clk(clk),.reset(reset),.i1(intermediate_reg_0[285]),.i2(intermediate_reg_0[284]),.o(intermediate_reg_1[142])); 
mux_module mux_module_inst_1_75(.clk(clk),.reset(reset),.i1(intermediate_reg_0[283]),.i2(intermediate_reg_0[282]),.o(intermediate_reg_1[141]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_76(.clk(clk),.reset(reset),.i1(intermediate_reg_0[281]),.i2(intermediate_reg_0[280]),.o(intermediate_reg_1[140])); 
xor_module xor_module_inst_1_77(.clk(clk),.reset(reset),.i1(intermediate_reg_0[279]),.i2(intermediate_reg_0[278]),.o(intermediate_reg_1[139])); 
xor_module xor_module_inst_1_78(.clk(clk),.reset(reset),.i1(intermediate_reg_0[277]),.i2(intermediate_reg_0[276]),.o(intermediate_reg_1[138])); 
xor_module xor_module_inst_1_79(.clk(clk),.reset(reset),.i1(intermediate_reg_0[275]),.i2(intermediate_reg_0[274]),.o(intermediate_reg_1[137])); 
xor_module xor_module_inst_1_80(.clk(clk),.reset(reset),.i1(intermediate_reg_0[273]),.i2(intermediate_reg_0[272]),.o(intermediate_reg_1[136])); 
xor_module xor_module_inst_1_81(.clk(clk),.reset(reset),.i1(intermediate_reg_0[271]),.i2(intermediate_reg_0[270]),.o(intermediate_reg_1[135])); 
xor_module xor_module_inst_1_82(.clk(clk),.reset(reset),.i1(intermediate_reg_0[269]),.i2(intermediate_reg_0[268]),.o(intermediate_reg_1[134])); 
mux_module mux_module_inst_1_83(.clk(clk),.reset(reset),.i1(intermediate_reg_0[267]),.i2(intermediate_reg_0[266]),.o(intermediate_reg_1[133]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_84(.clk(clk),.reset(reset),.i1(intermediate_reg_0[265]),.i2(intermediate_reg_0[264]),.o(intermediate_reg_1[132]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_85(.clk(clk),.reset(reset),.i1(intermediate_reg_0[263]),.i2(intermediate_reg_0[262]),.o(intermediate_reg_1[131]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_86(.clk(clk),.reset(reset),.i1(intermediate_reg_0[261]),.i2(intermediate_reg_0[260]),.o(intermediate_reg_1[130])); 
mux_module mux_module_inst_1_87(.clk(clk),.reset(reset),.i1(intermediate_reg_0[259]),.i2(intermediate_reg_0[258]),.o(intermediate_reg_1[129]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_88(.clk(clk),.reset(reset),.i1(intermediate_reg_0[257]),.i2(intermediate_reg_0[256]),.o(intermediate_reg_1[128])); 
mux_module mux_module_inst_1_89(.clk(clk),.reset(reset),.i1(intermediate_reg_0[255]),.i2(intermediate_reg_0[254]),.o(intermediate_reg_1[127]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_90(.clk(clk),.reset(reset),.i1(intermediate_reg_0[253]),.i2(intermediate_reg_0[252]),.o(intermediate_reg_1[126])); 
xor_module xor_module_inst_1_91(.clk(clk),.reset(reset),.i1(intermediate_reg_0[251]),.i2(intermediate_reg_0[250]),.o(intermediate_reg_1[125])); 
mux_module mux_module_inst_1_92(.clk(clk),.reset(reset),.i1(intermediate_reg_0[249]),.i2(intermediate_reg_0[248]),.o(intermediate_reg_1[124]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_93(.clk(clk),.reset(reset),.i1(intermediate_reg_0[247]),.i2(intermediate_reg_0[246]),.o(intermediate_reg_1[123])); 
xor_module xor_module_inst_1_94(.clk(clk),.reset(reset),.i1(intermediate_reg_0[245]),.i2(intermediate_reg_0[244]),.o(intermediate_reg_1[122])); 
mux_module mux_module_inst_1_95(.clk(clk),.reset(reset),.i1(intermediate_reg_0[243]),.i2(intermediate_reg_0[242]),.o(intermediate_reg_1[121]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_96(.clk(clk),.reset(reset),.i1(intermediate_reg_0[241]),.i2(intermediate_reg_0[240]),.o(intermediate_reg_1[120]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_97(.clk(clk),.reset(reset),.i1(intermediate_reg_0[239]),.i2(intermediate_reg_0[238]),.o(intermediate_reg_1[119])); 
mux_module mux_module_inst_1_98(.clk(clk),.reset(reset),.i1(intermediate_reg_0[237]),.i2(intermediate_reg_0[236]),.o(intermediate_reg_1[118]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_99(.clk(clk),.reset(reset),.i1(intermediate_reg_0[235]),.i2(intermediate_reg_0[234]),.o(intermediate_reg_1[117]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_100(.clk(clk),.reset(reset),.i1(intermediate_reg_0[233]),.i2(intermediate_reg_0[232]),.o(intermediate_reg_1[116]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_101(.clk(clk),.reset(reset),.i1(intermediate_reg_0[231]),.i2(intermediate_reg_0[230]),.o(intermediate_reg_1[115]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_102(.clk(clk),.reset(reset),.i1(intermediate_reg_0[229]),.i2(intermediate_reg_0[228]),.o(intermediate_reg_1[114])); 
xor_module xor_module_inst_1_103(.clk(clk),.reset(reset),.i1(intermediate_reg_0[227]),.i2(intermediate_reg_0[226]),.o(intermediate_reg_1[113])); 
xor_module xor_module_inst_1_104(.clk(clk),.reset(reset),.i1(intermediate_reg_0[225]),.i2(intermediate_reg_0[224]),.o(intermediate_reg_1[112])); 
mux_module mux_module_inst_1_105(.clk(clk),.reset(reset),.i1(intermediate_reg_0[223]),.i2(intermediate_reg_0[222]),.o(intermediate_reg_1[111]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_106(.clk(clk),.reset(reset),.i1(intermediate_reg_0[221]),.i2(intermediate_reg_0[220]),.o(intermediate_reg_1[110]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_107(.clk(clk),.reset(reset),.i1(intermediate_reg_0[219]),.i2(intermediate_reg_0[218]),.o(intermediate_reg_1[109]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_108(.clk(clk),.reset(reset),.i1(intermediate_reg_0[217]),.i2(intermediate_reg_0[216]),.o(intermediate_reg_1[108]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_109(.clk(clk),.reset(reset),.i1(intermediate_reg_0[215]),.i2(intermediate_reg_0[214]),.o(intermediate_reg_1[107]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_110(.clk(clk),.reset(reset),.i1(intermediate_reg_0[213]),.i2(intermediate_reg_0[212]),.o(intermediate_reg_1[106])); 
xor_module xor_module_inst_1_111(.clk(clk),.reset(reset),.i1(intermediate_reg_0[211]),.i2(intermediate_reg_0[210]),.o(intermediate_reg_1[105])); 
xor_module xor_module_inst_1_112(.clk(clk),.reset(reset),.i1(intermediate_reg_0[209]),.i2(intermediate_reg_0[208]),.o(intermediate_reg_1[104])); 
xor_module xor_module_inst_1_113(.clk(clk),.reset(reset),.i1(intermediate_reg_0[207]),.i2(intermediate_reg_0[206]),.o(intermediate_reg_1[103])); 
xor_module xor_module_inst_1_114(.clk(clk),.reset(reset),.i1(intermediate_reg_0[205]),.i2(intermediate_reg_0[204]),.o(intermediate_reg_1[102])); 
mux_module mux_module_inst_1_115(.clk(clk),.reset(reset),.i1(intermediate_reg_0[203]),.i2(intermediate_reg_0[202]),.o(intermediate_reg_1[101]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_116(.clk(clk),.reset(reset),.i1(intermediate_reg_0[201]),.i2(intermediate_reg_0[200]),.o(intermediate_reg_1[100]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_117(.clk(clk),.reset(reset),.i1(intermediate_reg_0[199]),.i2(intermediate_reg_0[198]),.o(intermediate_reg_1[99])); 
xor_module xor_module_inst_1_118(.clk(clk),.reset(reset),.i1(intermediate_reg_0[197]),.i2(intermediate_reg_0[196]),.o(intermediate_reg_1[98])); 
mux_module mux_module_inst_1_119(.clk(clk),.reset(reset),.i1(intermediate_reg_0[195]),.i2(intermediate_reg_0[194]),.o(intermediate_reg_1[97]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_120(.clk(clk),.reset(reset),.i1(intermediate_reg_0[193]),.i2(intermediate_reg_0[192]),.o(intermediate_reg_1[96]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_121(.clk(clk),.reset(reset),.i1(intermediate_reg_0[191]),.i2(intermediate_reg_0[190]),.o(intermediate_reg_1[95])); 
mux_module mux_module_inst_1_122(.clk(clk),.reset(reset),.i1(intermediate_reg_0[189]),.i2(intermediate_reg_0[188]),.o(intermediate_reg_1[94]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_123(.clk(clk),.reset(reset),.i1(intermediate_reg_0[187]),.i2(intermediate_reg_0[186]),.o(intermediate_reg_1[93]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_124(.clk(clk),.reset(reset),.i1(intermediate_reg_0[185]),.i2(intermediate_reg_0[184]),.o(intermediate_reg_1[92])); 
mux_module mux_module_inst_1_125(.clk(clk),.reset(reset),.i1(intermediate_reg_0[183]),.i2(intermediate_reg_0[182]),.o(intermediate_reg_1[91]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_126(.clk(clk),.reset(reset),.i1(intermediate_reg_0[181]),.i2(intermediate_reg_0[180]),.o(intermediate_reg_1[90])); 
mux_module mux_module_inst_1_127(.clk(clk),.reset(reset),.i1(intermediate_reg_0[179]),.i2(intermediate_reg_0[178]),.o(intermediate_reg_1[89]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_128(.clk(clk),.reset(reset),.i1(intermediate_reg_0[177]),.i2(intermediate_reg_0[176]),.o(intermediate_reg_1[88])); 
mux_module mux_module_inst_1_129(.clk(clk),.reset(reset),.i1(intermediate_reg_0[175]),.i2(intermediate_reg_0[174]),.o(intermediate_reg_1[87]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_130(.clk(clk),.reset(reset),.i1(intermediate_reg_0[173]),.i2(intermediate_reg_0[172]),.o(intermediate_reg_1[86])); 
mux_module mux_module_inst_1_131(.clk(clk),.reset(reset),.i1(intermediate_reg_0[171]),.i2(intermediate_reg_0[170]),.o(intermediate_reg_1[85]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_132(.clk(clk),.reset(reset),.i1(intermediate_reg_0[169]),.i2(intermediate_reg_0[168]),.o(intermediate_reg_1[84])); 
mux_module mux_module_inst_1_133(.clk(clk),.reset(reset),.i1(intermediate_reg_0[167]),.i2(intermediate_reg_0[166]),.o(intermediate_reg_1[83]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_134(.clk(clk),.reset(reset),.i1(intermediate_reg_0[165]),.i2(intermediate_reg_0[164]),.o(intermediate_reg_1[82]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_135(.clk(clk),.reset(reset),.i1(intermediate_reg_0[163]),.i2(intermediate_reg_0[162]),.o(intermediate_reg_1[81])); 
xor_module xor_module_inst_1_136(.clk(clk),.reset(reset),.i1(intermediate_reg_0[161]),.i2(intermediate_reg_0[160]),.o(intermediate_reg_1[80])); 
xor_module xor_module_inst_1_137(.clk(clk),.reset(reset),.i1(intermediate_reg_0[159]),.i2(intermediate_reg_0[158]),.o(intermediate_reg_1[79])); 
xor_module xor_module_inst_1_138(.clk(clk),.reset(reset),.i1(intermediate_reg_0[157]),.i2(intermediate_reg_0[156]),.o(intermediate_reg_1[78])); 
xor_module xor_module_inst_1_139(.clk(clk),.reset(reset),.i1(intermediate_reg_0[155]),.i2(intermediate_reg_0[154]),.o(intermediate_reg_1[77])); 
mux_module mux_module_inst_1_140(.clk(clk),.reset(reset),.i1(intermediate_reg_0[153]),.i2(intermediate_reg_0[152]),.o(intermediate_reg_1[76]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_141(.clk(clk),.reset(reset),.i1(intermediate_reg_0[151]),.i2(intermediate_reg_0[150]),.o(intermediate_reg_1[75])); 
mux_module mux_module_inst_1_142(.clk(clk),.reset(reset),.i1(intermediate_reg_0[149]),.i2(intermediate_reg_0[148]),.o(intermediate_reg_1[74]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_143(.clk(clk),.reset(reset),.i1(intermediate_reg_0[147]),.i2(intermediate_reg_0[146]),.o(intermediate_reg_1[73]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_144(.clk(clk),.reset(reset),.i1(intermediate_reg_0[145]),.i2(intermediate_reg_0[144]),.o(intermediate_reg_1[72])); 
mux_module mux_module_inst_1_145(.clk(clk),.reset(reset),.i1(intermediate_reg_0[143]),.i2(intermediate_reg_0[142]),.o(intermediate_reg_1[71]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_146(.clk(clk),.reset(reset),.i1(intermediate_reg_0[141]),.i2(intermediate_reg_0[140]),.o(intermediate_reg_1[70])); 
mux_module mux_module_inst_1_147(.clk(clk),.reset(reset),.i1(intermediate_reg_0[139]),.i2(intermediate_reg_0[138]),.o(intermediate_reg_1[69]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_148(.clk(clk),.reset(reset),.i1(intermediate_reg_0[137]),.i2(intermediate_reg_0[136]),.o(intermediate_reg_1[68])); 
mux_module mux_module_inst_1_149(.clk(clk),.reset(reset),.i1(intermediate_reg_0[135]),.i2(intermediate_reg_0[134]),.o(intermediate_reg_1[67]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_150(.clk(clk),.reset(reset),.i1(intermediate_reg_0[133]),.i2(intermediate_reg_0[132]),.o(intermediate_reg_1[66])); 
mux_module mux_module_inst_1_151(.clk(clk),.reset(reset),.i1(intermediate_reg_0[131]),.i2(intermediate_reg_0[130]),.o(intermediate_reg_1[65]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_152(.clk(clk),.reset(reset),.i1(intermediate_reg_0[129]),.i2(intermediate_reg_0[128]),.o(intermediate_reg_1[64])); 
mux_module mux_module_inst_1_153(.clk(clk),.reset(reset),.i1(intermediate_reg_0[127]),.i2(intermediate_reg_0[126]),.o(intermediate_reg_1[63]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_154(.clk(clk),.reset(reset),.i1(intermediate_reg_0[125]),.i2(intermediate_reg_0[124]),.o(intermediate_reg_1[62]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_155(.clk(clk),.reset(reset),.i1(intermediate_reg_0[123]),.i2(intermediate_reg_0[122]),.o(intermediate_reg_1[61]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_156(.clk(clk),.reset(reset),.i1(intermediate_reg_0[121]),.i2(intermediate_reg_0[120]),.o(intermediate_reg_1[60])); 
mux_module mux_module_inst_1_157(.clk(clk),.reset(reset),.i1(intermediate_reg_0[119]),.i2(intermediate_reg_0[118]),.o(intermediate_reg_1[59]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_158(.clk(clk),.reset(reset),.i1(intermediate_reg_0[117]),.i2(intermediate_reg_0[116]),.o(intermediate_reg_1[58]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_159(.clk(clk),.reset(reset),.i1(intermediate_reg_0[115]),.i2(intermediate_reg_0[114]),.o(intermediate_reg_1[57])); 
xor_module xor_module_inst_1_160(.clk(clk),.reset(reset),.i1(intermediate_reg_0[113]),.i2(intermediate_reg_0[112]),.o(intermediate_reg_1[56])); 
xor_module xor_module_inst_1_161(.clk(clk),.reset(reset),.i1(intermediate_reg_0[111]),.i2(intermediate_reg_0[110]),.o(intermediate_reg_1[55])); 
xor_module xor_module_inst_1_162(.clk(clk),.reset(reset),.i1(intermediate_reg_0[109]),.i2(intermediate_reg_0[108]),.o(intermediate_reg_1[54])); 
mux_module mux_module_inst_1_163(.clk(clk),.reset(reset),.i1(intermediate_reg_0[107]),.i2(intermediate_reg_0[106]),.o(intermediate_reg_1[53]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_164(.clk(clk),.reset(reset),.i1(intermediate_reg_0[105]),.i2(intermediate_reg_0[104]),.o(intermediate_reg_1[52]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_165(.clk(clk),.reset(reset),.i1(intermediate_reg_0[103]),.i2(intermediate_reg_0[102]),.o(intermediate_reg_1[51])); 
xor_module xor_module_inst_1_166(.clk(clk),.reset(reset),.i1(intermediate_reg_0[101]),.i2(intermediate_reg_0[100]),.o(intermediate_reg_1[50])); 
mux_module mux_module_inst_1_167(.clk(clk),.reset(reset),.i1(intermediate_reg_0[99]),.i2(intermediate_reg_0[98]),.o(intermediate_reg_1[49]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_168(.clk(clk),.reset(reset),.i1(intermediate_reg_0[97]),.i2(intermediate_reg_0[96]),.o(intermediate_reg_1[48]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_169(.clk(clk),.reset(reset),.i1(intermediate_reg_0[95]),.i2(intermediate_reg_0[94]),.o(intermediate_reg_1[47]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_170(.clk(clk),.reset(reset),.i1(intermediate_reg_0[93]),.i2(intermediate_reg_0[92]),.o(intermediate_reg_1[46])); 
mux_module mux_module_inst_1_171(.clk(clk),.reset(reset),.i1(intermediate_reg_0[91]),.i2(intermediate_reg_0[90]),.o(intermediate_reg_1[45]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_172(.clk(clk),.reset(reset),.i1(intermediate_reg_0[89]),.i2(intermediate_reg_0[88]),.o(intermediate_reg_1[44])); 
xor_module xor_module_inst_1_173(.clk(clk),.reset(reset),.i1(intermediate_reg_0[87]),.i2(intermediate_reg_0[86]),.o(intermediate_reg_1[43])); 
mux_module mux_module_inst_1_174(.clk(clk),.reset(reset),.i1(intermediate_reg_0[85]),.i2(intermediate_reg_0[84]),.o(intermediate_reg_1[42]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_175(.clk(clk),.reset(reset),.i1(intermediate_reg_0[83]),.i2(intermediate_reg_0[82]),.o(intermediate_reg_1[41]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_176(.clk(clk),.reset(reset),.i1(intermediate_reg_0[81]),.i2(intermediate_reg_0[80]),.o(intermediate_reg_1[40]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_177(.clk(clk),.reset(reset),.i1(intermediate_reg_0[79]),.i2(intermediate_reg_0[78]),.o(intermediate_reg_1[39]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_178(.clk(clk),.reset(reset),.i1(intermediate_reg_0[77]),.i2(intermediate_reg_0[76]),.o(intermediate_reg_1[38]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_179(.clk(clk),.reset(reset),.i1(intermediate_reg_0[75]),.i2(intermediate_reg_0[74]),.o(intermediate_reg_1[37]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_180(.clk(clk),.reset(reset),.i1(intermediate_reg_0[73]),.i2(intermediate_reg_0[72]),.o(intermediate_reg_1[36]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_181(.clk(clk),.reset(reset),.i1(intermediate_reg_0[71]),.i2(intermediate_reg_0[70]),.o(intermediate_reg_1[35]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_182(.clk(clk),.reset(reset),.i1(intermediate_reg_0[69]),.i2(intermediate_reg_0[68]),.o(intermediate_reg_1[34]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_183(.clk(clk),.reset(reset),.i1(intermediate_reg_0[67]),.i2(intermediate_reg_0[66]),.o(intermediate_reg_1[33]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_184(.clk(clk),.reset(reset),.i1(intermediate_reg_0[65]),.i2(intermediate_reg_0[64]),.o(intermediate_reg_1[32])); 
xor_module xor_module_inst_1_185(.clk(clk),.reset(reset),.i1(intermediate_reg_0[63]),.i2(intermediate_reg_0[62]),.o(intermediate_reg_1[31])); 
mux_module mux_module_inst_1_186(.clk(clk),.reset(reset),.i1(intermediate_reg_0[61]),.i2(intermediate_reg_0[60]),.o(intermediate_reg_1[30]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_187(.clk(clk),.reset(reset),.i1(intermediate_reg_0[59]),.i2(intermediate_reg_0[58]),.o(intermediate_reg_1[29]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_188(.clk(clk),.reset(reset),.i1(intermediate_reg_0[57]),.i2(intermediate_reg_0[56]),.o(intermediate_reg_1[28])); 
xor_module xor_module_inst_1_189(.clk(clk),.reset(reset),.i1(intermediate_reg_0[55]),.i2(intermediate_reg_0[54]),.o(intermediate_reg_1[27])); 
mux_module mux_module_inst_1_190(.clk(clk),.reset(reset),.i1(intermediate_reg_0[53]),.i2(intermediate_reg_0[52]),.o(intermediate_reg_1[26]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_191(.clk(clk),.reset(reset),.i1(intermediate_reg_0[51]),.i2(intermediate_reg_0[50]),.o(intermediate_reg_1[25]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_192(.clk(clk),.reset(reset),.i1(intermediate_reg_0[49]),.i2(intermediate_reg_0[48]),.o(intermediate_reg_1[24]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_193(.clk(clk),.reset(reset),.i1(intermediate_reg_0[47]),.i2(intermediate_reg_0[46]),.o(intermediate_reg_1[23]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_194(.clk(clk),.reset(reset),.i1(intermediate_reg_0[45]),.i2(intermediate_reg_0[44]),.o(intermediate_reg_1[22]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_195(.clk(clk),.reset(reset),.i1(intermediate_reg_0[43]),.i2(intermediate_reg_0[42]),.o(intermediate_reg_1[21]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_196(.clk(clk),.reset(reset),.i1(intermediate_reg_0[41]),.i2(intermediate_reg_0[40]),.o(intermediate_reg_1[20])); 
xor_module xor_module_inst_1_197(.clk(clk),.reset(reset),.i1(intermediate_reg_0[39]),.i2(intermediate_reg_0[38]),.o(intermediate_reg_1[19])); 
mux_module mux_module_inst_1_198(.clk(clk),.reset(reset),.i1(intermediate_reg_0[37]),.i2(intermediate_reg_0[36]),.o(intermediate_reg_1[18]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_199(.clk(clk),.reset(reset),.i1(intermediate_reg_0[35]),.i2(intermediate_reg_0[34]),.o(intermediate_reg_1[17])); 
xor_module xor_module_inst_1_200(.clk(clk),.reset(reset),.i1(intermediate_reg_0[33]),.i2(intermediate_reg_0[32]),.o(intermediate_reg_1[16])); 
mux_module mux_module_inst_1_201(.clk(clk),.reset(reset),.i1(intermediate_reg_0[31]),.i2(intermediate_reg_0[30]),.o(intermediate_reg_1[15]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_202(.clk(clk),.reset(reset),.i1(intermediate_reg_0[29]),.i2(intermediate_reg_0[28]),.o(intermediate_reg_1[14])); 
xor_module xor_module_inst_1_203(.clk(clk),.reset(reset),.i1(intermediate_reg_0[27]),.i2(intermediate_reg_0[26]),.o(intermediate_reg_1[13])); 
mux_module mux_module_inst_1_204(.clk(clk),.reset(reset),.i1(intermediate_reg_0[25]),.i2(intermediate_reg_0[24]),.o(intermediate_reg_1[12]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_205(.clk(clk),.reset(reset),.i1(intermediate_reg_0[23]),.i2(intermediate_reg_0[22]),.o(intermediate_reg_1[11])); 
mux_module mux_module_inst_1_206(.clk(clk),.reset(reset),.i1(intermediate_reg_0[21]),.i2(intermediate_reg_0[20]),.o(intermediate_reg_1[10]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_207(.clk(clk),.reset(reset),.i1(intermediate_reg_0[19]),.i2(intermediate_reg_0[18]),.o(intermediate_reg_1[9])); 
mux_module mux_module_inst_1_208(.clk(clk),.reset(reset),.i1(intermediate_reg_0[17]),.i2(intermediate_reg_0[16]),.o(intermediate_reg_1[8]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_209(.clk(clk),.reset(reset),.i1(intermediate_reg_0[15]),.i2(intermediate_reg_0[14]),.o(intermediate_reg_1[7])); 
xor_module xor_module_inst_1_210(.clk(clk),.reset(reset),.i1(intermediate_reg_0[13]),.i2(intermediate_reg_0[12]),.o(intermediate_reg_1[6])); 
mux_module mux_module_inst_1_211(.clk(clk),.reset(reset),.i1(intermediate_reg_0[11]),.i2(intermediate_reg_0[10]),.o(intermediate_reg_1[5]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_212(.clk(clk),.reset(reset),.i1(intermediate_reg_0[9]),.i2(intermediate_reg_0[8]),.o(intermediate_reg_1[4]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_213(.clk(clk),.reset(reset),.i1(intermediate_reg_0[7]),.i2(intermediate_reg_0[6]),.o(intermediate_reg_1[3]),.sel(intermediate_reg_0[0])); 
mux_module mux_module_inst_1_214(.clk(clk),.reset(reset),.i1(intermediate_reg_0[5]),.i2(intermediate_reg_0[4]),.o(intermediate_reg_1[2]),.sel(intermediate_reg_0[0])); 
xor_module xor_module_inst_1_215(.clk(clk),.reset(reset),.i1(intermediate_reg_0[3]),.i2(intermediate_reg_0[2]),.o(intermediate_reg_1[1])); 
xor_module xor_module_inst_1_216(.clk(clk),.reset(reset),.i1(intermediate_reg_0[1]),.i2(intermediate_reg_0[0]),.o(intermediate_reg_1[0])); 
wire [215:0]intermediate_wire_2; 
assign intermediate_wire_2[215] = intermediate_reg_1[216]^intermediate_reg_1[215] ; 
assign intermediate_wire_2[214:0] = intermediate_reg_1[214:0] ; 
always@(posedge clk) begin 
outp [215:0] <= intermediate_wire_2; 
outp[415:216] <= intermediate_wire_2[199:0] ; 
end 
endmodule 
 

module dpram_2048_40bit_module_2(input clk, input reset, input[207:0] inp, output [159:0] outp); 

dpram_2048_40bit_module inst_0 (.clk(clk),.reset(reset),.inp(inp[103:0]),.outp(outp[79:0])); 

dpram_2048_40bit_module inst_1 (.clk(clk),.reset(reset),.inp(inp[207:104]),.outp(outp[159:80])); 

endmodule 

module systolic_array_4_16bit_2(input clk, input reset, input[509:0] inp, output [261:0] outp); 

systolic_array_4_16bit inst_0 (.clk(clk),.reset(reset),.inp(inp[254:0]),.outp(outp[130:0])); 

systolic_array_4_16bit inst_1 (.clk(clk),.reset(reset),.inp(inp[509:255]),.outp(outp[261:131])); 

endmodule 

module systolic_array_8_16bit_2(input clk, input reset, input[1571:0] inp, output [867:0] outp); 

systolic_array_8_16bit inst_0 (.clk(clk),.reset(reset),.inp(inp[785:0]),.outp(outp[433:0])); 

systolic_array_8_16bit inst_1 (.clk(clk),.reset(reset),.inp(inp[1571:786]),.outp(outp[867:434])); 

endmodule 

module activation_32_8bit_module_3(input clk, input reset, input[782:0] inp, output [773:0] outp); 

activation_32_8bit_module inst_0 (.clk(clk),.reset(reset),.inp(inp[260:0]),.outp(outp[257:0])); 

activation_32_8bit_module inst_1 (.clk(clk),.reset(reset),.inp(inp[521:261]),.outp(outp[515:258])); 

activation_32_8bit_module inst_2 (.clk(clk),.reset(reset),.inp(inp[782:522]),.outp(outp[773:516])); 

endmodule 

module activation_32_16bit_module_2(input clk, input reset, input[1031:0] inp, output [1027:0] outp); 

activation_32_16bit_module inst_0 (.clk(clk),.reset(reset),.inp(inp[515:0]),.outp(outp[513:0])); 

activation_32_16bit_module inst_1 (.clk(clk),.reset(reset),.inp(inp[1031:516]),.outp(outp[1027:514])); 

endmodule 

module dpram_4096_40bit_module_8(input clk, input reset, input[847:0] inp, output [639:0] outp); 

dpram_4096_40bit_module inst_0 (.clk(clk),.reset(reset),.inp(inp[105:0]),.outp(outp[79:0])); 

dpram_4096_40bit_module inst_1 (.clk(clk),.reset(reset),.inp(inp[211:106]),.outp(outp[159:80])); 

dpram_4096_40bit_module inst_2 (.clk(clk),.reset(reset),.inp(inp[317:212]),.outp(outp[239:160])); 

dpram_4096_40bit_module inst_3 (.clk(clk),.reset(reset),.inp(inp[423:318]),.outp(outp[319:240])); 

dpram_4096_40bit_module inst_4 (.clk(clk),.reset(reset),.inp(inp[529:424]),.outp(outp[399:320])); 

dpram_4096_40bit_module inst_5 (.clk(clk),.reset(reset),.inp(inp[635:530]),.outp(outp[479:400])); 

dpram_4096_40bit_module inst_6 (.clk(clk),.reset(reset),.inp(inp[741:636]),.outp(outp[559:480])); 

dpram_4096_40bit_module inst_7 (.clk(clk),.reset(reset),.inp(inp[847:742]),.outp(outp[639:560])); 

endmodule 

module adder_tree_4_16bit_6(input clk, input reset, input[1535:0] inp, output [191:0] outp); 

adder_tree_4_16bit inst_0 (.clk(clk),.reset(reset),.inp(inp[255:0]),.outp(outp[31:0])); 

adder_tree_4_16bit inst_1 (.clk(clk),.reset(reset),.inp(inp[511:256]),.outp(outp[63:32])); 

adder_tree_4_16bit inst_2 (.clk(clk),.reset(reset),.inp(inp[767:512]),.outp(outp[95:64])); 

adder_tree_4_16bit inst_3 (.clk(clk),.reset(reset),.inp(inp[1023:768]),.outp(outp[127:96])); 

adder_tree_4_16bit inst_4 (.clk(clk),.reset(reset),.inp(inp[1279:1024]),.outp(outp[159:128])); 

adder_tree_4_16bit inst_5 (.clk(clk),.reset(reset),.inp(inp[1535:1280]),.outp(outp[191:160])); 

endmodule 

module systolic_array_4_16bit_1(input clk, input reset, input[254:0] inp, output [130:0] outp); 

systolic_array_4_16bit inst_0 (.clk(clk),.reset(reset),.inp(inp[254:0]),.outp(outp[130:0])); 

endmodule 

module dpram_2048_60bit_module_4(input clk, input reset, input[575:0] inp, output [479:0] outp); 

dpram_2048_60bit_module inst_0 (.clk(clk),.reset(reset),.inp(inp[143:0]),.outp(outp[119:0])); 

dpram_2048_60bit_module inst_1 (.clk(clk),.reset(reset),.inp(inp[287:144]),.outp(outp[239:120])); 

dpram_2048_60bit_module inst_2 (.clk(clk),.reset(reset),.inp(inp[431:288]),.outp(outp[359:240])); 

dpram_2048_60bit_module inst_3 (.clk(clk),.reset(reset),.inp(inp[575:432]),.outp(outp[479:360])); 

endmodule 


module sigmoid_16bit_4(input clk, input reset, input[63:0] inp, output [63:0] outp); 

sigmoid_16bit inst_0 (.clk(clk),.reset(reset),.inp(inp[15:0]),.outp(outp[15:0])); 

sigmoid_16bit inst_1 (.clk(clk),.reset(reset),.inp(inp[31:16]),.outp(outp[31:16])); 

sigmoid_16bit inst_2 (.clk(clk),.reset(reset),.inp(inp[47:32]),.outp(outp[47:32])); 

sigmoid_16bit inst_3 (.clk(clk),.reset(reset),.inp(inp[63:48]),.outp(outp[63:48])); 

endmodule 

module adder_tree_3_8bit_6(input clk, input reset, input[383:0] inp, output [95:0] outp); 

adder_tree_3_8bit inst_0 (.clk(clk),.reset(reset),.inp(inp[63:0]),.outp(outp[15:0])); 

adder_tree_3_8bit inst_1 (.clk(clk),.reset(reset),.inp(inp[127:64]),.outp(outp[31:16])); 

adder_tree_3_8bit inst_2 (.clk(clk),.reset(reset),.inp(inp[191:128]),.outp(outp[47:32])); 

adder_tree_3_8bit inst_3 (.clk(clk),.reset(reset),.inp(inp[255:192]),.outp(outp[63:48])); 

adder_tree_3_8bit inst_4 (.clk(clk),.reset(reset),.inp(inp[319:256]),.outp(outp[79:64])); 

adder_tree_3_8bit inst_5 (.clk(clk),.reset(reset),.inp(inp[383:320]),.outp(outp[95:80])); 

endmodule 

module adder_tree_4_4bit_8(input clk, input reset, input[511:0] inp, output [63:0] outp); 

adder_tree_4_4bit inst_0 (.clk(clk),.reset(reset),.inp(inp[63:0]),.outp(outp[7:0])); 

adder_tree_4_4bit inst_1 (.clk(clk),.reset(reset),.inp(inp[127:64]),.outp(outp[15:8])); 

adder_tree_4_4bit inst_2 (.clk(clk),.reset(reset),.inp(inp[191:128]),.outp(outp[23:16])); 

adder_tree_4_4bit inst_3 (.clk(clk),.reset(reset),.inp(inp[255:192]),.outp(outp[31:24])); 

adder_tree_4_4bit inst_4 (.clk(clk),.reset(reset),.inp(inp[319:256]),.outp(outp[39:32])); 

adder_tree_4_4bit inst_5 (.clk(clk),.reset(reset),.inp(inp[383:320]),.outp(outp[47:40])); 

adder_tree_4_4bit inst_6 (.clk(clk),.reset(reset),.inp(inp[447:384]),.outp(outp[55:48])); 

adder_tree_4_4bit inst_7 (.clk(clk),.reset(reset),.inp(inp[511:448]),.outp(outp[63:56])); 

endmodule 

module systolic_array_8_16bit_1(input clk, input reset, input[785:0] inp, output [433:0] outp); 

systolic_array_8_16bit inst_0 (.clk(clk),.reset(reset),.inp(inp[785:0]),.outp(outp[433:0])); 

endmodule 

module dpram_2048_40bit_module_4(input clk, input reset, input[415:0] inp, output [319:0] outp); 

dpram_2048_40bit_module inst_0 (.clk(clk),.reset(reset),.inp(inp[103:0]),.outp(outp[79:0])); 

dpram_2048_40bit_module inst_1 (.clk(clk),.reset(reset),.inp(inp[207:104]),.outp(outp[159:80])); 

dpram_2048_40bit_module inst_2 (.clk(clk),.reset(reset),.inp(inp[311:208]),.outp(outp[239:160])); 

dpram_2048_40bit_module inst_3 (.clk(clk),.reset(reset),.inp(inp[415:312]),.outp(outp[319:240])); 

endmodule 
module adder_tree_1_16bit (input clk,input reset,input [31:0] inp, output [31:0] outp);

adder_tree_1stage_16bit inst(.clk(clk),.reset(reset),.inp00(inp[15:0]),.inp01(inp[31:16]),.sum_out(outp));

endmodule

module adder_tree_2_16bit (input clk, input reset, input [63:0] inp, output [31:0] outp);

adder_tree_2stage_16bit inst(.clk(clk),.reset(reset),.inp00(inp[15:0]),.inp01(inp[31:16]),.inp10(inp[47:32]),.inp11(inp[63:48]),.sum_out(outp));

endmodule

module adder_tree_3_16bit (input clk, input reset, input [127:0] inp, output [31:0] outp);

adder_tree_3stage_16bit inst (.clk(clk),.reset(reset),.inp00(inp[15:0]),.inp01(inp[31:16]),.inp10(inp[47:32]),.inp11(inp[63:48]),.inp20(inp[79:64]),.inp21(inp[95:80]),.inp30(inp[111:96]),.inp31(inp[127:112]),.sum_out(outp));

endmodule

module adder_tree_4_16bit (input clk, input reset, input [255:0] inp, output [31:0] outp);

adder_tree_4stage_16bit inst(.clk(clk),.reset(reset),.inp00(inp[15:0]),.inp01(inp[31:16]),.inp10(inp[47:32]),.inp11(inp[63:48]),.inp20(inp[79:64]),.inp21(inp[95:80]),.inp30(inp[111:96]),.inp31(inp[127:112]),.inp40(inp[143:128]),.inp41(inp[159:144]),.inp50(inp[175:160]),.inp51(inp[191:176]),.inp60(inp[207:192]),.inp61(inp[223:208]),.inp70(inp[239:224]),.inp71(inp[255:240]),.sum_out(outp));

endmodule

module adder_tree_1_8bit (input clk, input reset, input [15:0] inp, output [15:0] outp);

adder_tree_1stage_8bit inst(.clk(clk),.reset(reset),.inp00(inp[7:0]),.inp01(inp[15:8]),.sum_out(outp));

endmodule

module adder_tree_2_8bit (input clk, input reset, input [31:0] inp, output [15:0] outp);

adder_tree_2stage_8bit inst(.clk(clk),.reset(reset),.inp00(inp[7:0]),.inp01(inp[15:8]),.inp10(inp[23:16]),.inp11(inp[31:24]),.sum_out(outp));

endmodule

module adder_tree_3_8bit (input clk, input reset, input [63:0] inp, output [15:0] outp);

adder_tree_3stage_8bit inst (.clk(clk),.reset(reset),.inp00(inp[7:0]),.inp01(inp[15:8]),.inp10(inp[23:16]),.inp11(inp[31:24]),.inp20(inp[39:32]),.inp21(inp[47:40]),.inp30(inp[55:48]),.inp31(inp[63:56]),.sum_out(outp));

endmodule

module adder_tree_4_8bit (input clk, input reset, input [127:0] inp, output [15:0] outp);

adder_tree_4stage_8bit inst(.clk(clk),.reset(reset),.inp00(inp[7:0]),.inp01(inp[15:8]),.inp10(inp[23:16]),.inp11(inp[31:24]),.inp20(inp[39:32]),.inp21(inp[47:40]),.inp30(inp[55:48]),.inp31(inp[63:56]),.inp40(inp[71:64]),.inp41(inp[79:72]),.inp50(inp[87:80]),.inp51(inp[95:88]),.inp60(inp[103:96]),.inp61(inp[111:104]),.inp70(inp[119:112]),.inp71(inp[127:120]),.sum_out(outp));

endmodule

module adder_tree_1_4bit (input clk, input reset, input [7:0] inp, output [7:0] outp);

adder_tree_1stage_4bit inst(.clk(clk),.reset(reset),.inp00(inp[3:0]),.inp01(inp[7:4]),.sum_out(outp));

endmodule

module adder_tree_2_4bit (input clk, input reset, input [15:0] inp, output [7:0] outp);

adder_tree_2stage_4bit inst(.clk(clk),.reset(reset),.inp00(inp[3:0]),.inp01(inp[7:4]),.inp10(inp[11:8]),.inp11(inp[15:12]),.sum_out(outp));

endmodule

module adder_tree_3_4bit (input clk, input reset, input [31:0] inp, output [7:0] outp);

adder_tree_3stage_4bit inst (.clk(clk),.reset(reset),.inp00(inp[3:0]),.inp01(inp[7:4]),.inp10(inp[11:8]),.inp11(inp[15:12]),.inp20(inp[19:16]),.inp21(inp[23:20]),.inp30(inp[27:24]),.inp31(inp[31:28]),.sum_out(outp));

endmodule

module adder_tree_4_4bit (input clk, input reset, input [63:0] inp, output [7:0] outp);

adder_tree_4stage_4bit inst(.clk(clk),.reset(reset),.inp00(inp[3:0]),.inp01(inp[7:4]),.inp10(inp[11:8]),.inp11(inp[15:12]),.inp20(inp[19:16]),.inp21(inp[23:20]),.inp30(inp[27:24]),.inp31(inp[31:28]),.inp40(inp[35:32]),.inp41(inp[39:36]),.inp50(inp[43:40]),.inp51(inp[47:44]),.inp60(inp[51:48]),.inp61(inp[55:52]),.inp70(inp[59:56]),.inp71(inp[63:60]),.sum_out(outp));

endmodule

module adder_tree_3_fp16bit (input clk, input reset, input [131:0] inp, output [15:0] outp);

mode4_adder_tree inst(
  .inp0(inp[15:0]),
  .inp1(inp[31:16]),
  .inp2(inp[47:32]),
  .inp3(inp[63:48]),
  .inp4(inp[79:64]),
  .inp5(inp[95:80]),
  .inp6(inp[111:96]),
  .inp7(inp[127:112]),
  .mode4_stage0_run(inp[128]),
  .mode4_stage1_run(inp[129]),
  .mode4_stage2_run(inp[130]),
  .mode4_stage3_run(inp[131]),

  .clk(clk),
  .reset(reset),
  .outp(outp[15:0])
);

endmodule

module dpram_1024_32bit_module (input clk, input reset, input [85:0] inp, output [63:0] outp);

dpram inst (.clk(clk),.address_a(inp[9:0]),.address_b(inp[19:10]),.wren_a(inp[20]),.wren_b(inp[21]),.data_a(inp[53:22]),.data_b(inp[85:54]),.out_a(outp[31:0]),.out_b(outp[63:32]));

endmodule

module dpram_1024_64bit_module (input clk, input reset, input [149:0] inp, output [63:0] outp );

dpram_1024_64bit inst (.clk(clk),.address_a(inp[9:0]),.address_b(inp[19:10]),.wren_a(inp[20]),.wren_b(inp[21]),.data_a(inp[85:22]),.data_b(inp[149:86]),.out_a(outp[31:0]),.out_b(outp[63:32]));

endmodule

module dpram_2048_64bit_module (input clk, input reset, input [151:0] inp, output [127:0] outp);

dpram_2048_64bit inst (.clk(clk),.address_a(inp[10:0]),.address_b(inp[21:11]),.wren_a(inp[22]),.wren_b(inp[23]),.data_a(inp[87:24]),.data_b(inp[151:88]),.out_a(outp[63:0]),.out_b(outp[127:64]));

endmodule

module dpram_2048_32bit_module (input clk, input reset, input [87:0] inp, output [63:0] outp);

dpram_2048_32bit inst (.clk(clk),.address_a(inp[10:0]),.address_b(inp[21:11]),.wren_a(inp[22]),.wren_b(inp[23]),.data_a(inp[55:24]),.data_b(inp[87:56]),.out_a(outp[31:0]),.out_b(outp[63:32]));

endmodule

module dpram_1024_40bit_module (input clk, input reset, input [101:0] inp, output [79:0] outp);

dpram_1024_40bit inst (.clk(clk),.address_a(inp[9:0]),.address_b(inp[19:10]),.wren_a(inp[20]),.wren_b(inp[21]),.data_a(inp[61:22]),.data_b(inp[101:62]),.out_a(outp[39:0]),.out_b(outp[79:40]));

endmodule

module dpram_1024_60bit_module (input clk, input reset, input [141:0] inp, output [119:0] outp);

dpram_1024_60bit inst (.clk(clk),.address_a(inp[9:0]),.address_b(inp[19:10]),.wren_a(inp[20]),.wren_b(inp[21]),.data_a(inp[81:22]),.data_b(inp[141:82]),.out_a(outp[59:0]),.out_b(outp[119:60]));

endmodule

module dpram_2048_40bit_module (input clk, input reset, input [103:0] inp, output [79:0] outp);

dpram_2048_40bit inst (.clk(clk),.address_a(inp[10:0]),.address_b(inp[21:11]),.wren_a(inp[22]),.wren_b(inp[23]),.data_a(inp[63:24]),.data_b(inp[103:64]),.out_a(outp[39:0]),.out_b(outp[79:40]));

endmodule

module dpram_2048_60bit_module (input clk, input reset, input [143:0] inp, output [119:0] outp);

dpram_2048_60bit inst (.clk(clk),.address_a(inp[10:0]),.address_b(inp[21:11]),.wren_a(inp[22]),.wren_b(inp[23]),.data_a(inp[83:24]),.data_b(inp[143:84]),.out_a(outp[59:0]),.out_b(outp[119:60]));

endmodule

module dpram_4096_40bit_module (input clk, input reset, input [105:0] inp, output [79:0] outp);

dpram_4096_40bit inst (.clk(clk),.address_a(inp[11:0]),.address_b(inp[23:12]),.wren_a(inp[24]),.wren_b(inp[25]),.data_a(inp[65:26]),.data_b(inp[105:66]),.out_a(outp[39:0]),.out_b(outp[79:40]));

endmodule

module dpram_4096_60bit_module (input clk, input reset, input [145:0] inp, output [119:0] outp);

dpram_4096_60bit inst (.clk(clk),.address_a(inp[11:0]),.address_b(inp[23:12]),.wren_a(inp[24]),.wren_b(inp[25]),.data_a(inp[85:26]),.data_b(inp[145:86]),.out_a(outp[59:0]),.out_b(outp[119:60]));

endmodule

module spram_1024_32bit_module (input clk,input reset,input [42:0] inp, output [31:0] outp);

spram inst (.clk(clk),.address(inp[9:0]),.wren(inp[10]),.data(inp[42:11]),.out(outp));

endmodule

module spram_2048_40bit_module (input clk,input reset,input [51:0] inp, output [39:0] outp);

spram_2048_40bit inst (.clk(clk),.address(inp[10:0]),.wren(inp[11]),.data(inp[51:12]),.out(outp));

endmodule

module spram_2048_60bit_module (input clk,input reset,input [71:0] inp, output [59:0] outp);

spram_2048_60bit inst (.clk(clk),.address(inp[10:0]),.wren(inp[11]),.data(inp[71:12]),.out(outp));

endmodule

module spram_4096_40bit_module (input clk,input reset,input [52:0] inp, output [39:0] outp);

spram_4096_40bit inst (.clk(clk),.address(inp[11:0]),.wren(inp[12]),.data(inp[52:13]),.out(outp));

endmodule

module spram_4096_60bit_module (input clk,input reset,input [72:0] inp, output [59:0] outp);

spram_4096_60bit inst (.clk(clk),.address(inp[11:0]),.wren(inp[12]),.data(inp[72:13]),.out(outp));

endmodule

module dbram_2048_40bit_module (input clk,input reset,input [103:0] inp, output [79:0] outp);

dbram_2048_40bit inst (.clk(clk),.reset(reset),.address_a(inp[10:0]),.address_b(inp[21:11]),.wren_a(inp[22]),.wren_b(inp[23]),.data_a(inp[63:24]),.data_b(inp[103:64]),.out_a(outp[39:0]),.out_b(outp[79:40]));

endmodule

module dbram_2048_60bit_module (input clk,input reset,input [143:0] inp, output [119:0] outp);

dbram_2048_60bit inst (.clk(clk),.reset(reset),.address_a(inp[10:0]),.address_b(inp[21:11]),.wren_a(inp[22]),.wren_b(inp[23]),.data_a(inp[83:24]),.data_b(inp[143:84]),.out_a(outp[59:0]),.out_b(outp[119:60]));

endmodule

module dbram_4096_40bit_module (input clk,input reset,input [105:0] inp, output [79:0] outp);

dbram_4096_40bit inst (.clk(clk),.reset(reset),.address_a(inp[11:0]),.address_b(inp[23:12]),.wren_a(inp[24]),.wren_b(inp[25]),.data_a(inp[65:26]),.data_b(inp[105:66]),.out_a(outp[39:0]),.out_b(outp[79:40]));

endmodule

module dbram_4096_60bit_module (input clk,input reset,input [145:0] inp, output [119:0] outp);

dbram_4096_60bit inst (.clk(clk),.reset(reset),.address_a(inp[11:0]),.address_b(inp[23:12]),.wren_a(inp[24]),.wren_b(inp[25]),.data_a(inp[85:26]),.data_b(inp[145:86]),.out_a(outp[59:0]),.out_b(outp[119:60]));

endmodule


module fifo_256_40bit_module (input clk,input reset,input [42:0] inp, output [41:0] outp);

fifo_256_40bit inst (.clk(clk),.rst(reset),.clr(inp[0]),.din(inp[40:1]),.we(inp[41]),.dout(outp[39:0]),.re(inp[42]),.full(outp[40]),.empty(outp[41]));

endmodule

module fifo_256_60bit_module (input clk,input reset,input [62:0] inp, output [61:0] outp);

fifo_256_60bit inst (.clk(clk),.rst(reset),.clr(inp[0]),.din(inp[60:1]),.we(inp[61]),.dout(outp[59:0]),.re(inp[62]),.full(outp[60]),.empty(outp[61]));

endmodule

module fifo_512_60bit_module (input clk,input reset,input [62:0] inp, output [61:0] outp);

fifo_512_60bit inst (.clk(clk),.rst(reset),.clr(inp[0]),.din(inp[60:1]),.we(inp[61]),.dout(outp[59:0]),.re(inp[62]),.full(outp[60]),.empty(outp[61]));

endmodule

module fifo_512_40bit_module (input clk,input reset,input [42:0] inp, output [41:0] outp);

fifo_512_40bit inst (.clk(clk),.rst(reset),.clr(inp[0]),.din(inp[40:1]),.we(inp[41]),.dout(outp[39:0]),.re(inp[42]),.full(outp[40]),.empty(outp[41]));

endmodule

module tanh_16bit (input clk,input reset, input [15:0] inp, output [15:0] outp);

tanh inst (.x(inp),.tanh_out(outp));

endmodule

module sigmoid_16bit (input clk,input reset, input [15:0] inp, output [15:0] outp);

sigmoid inst (.x(inp),.sig_out(outp));

endmodule

module systolic_array_4_16bit (input clk, input reset, input [254:0] inp, output [130:0] outp);

matmul_4x4_systolic inst(
 .clk(clk),
 .reset(inp[254]),
 .pe_reset(reset),
 .start_mat_mul(inp[0]),
 .done_mat_mul(outp[0]),
 .address_mat_a(inp[11:1]),
 .address_mat_b(inp[22:12]),
 .address_mat_c(inp[33:23]),
 .address_stride_a(inp[41:34]),
 .address_stride_b(inp[49:42]),
 .address_stride_c(inp[57:50]),
 .a_data(inp[89:58]),
 .b_data(inp[121:90]),
 .a_data_in(inp[153:122]), //Data values coming in from previous matmul - systolic connections
 .b_data_in(inp[185:154]),
 .c_data_in(inp[217:186]), //Data values coming in from previous matmul - systolic shifting
 .c_data_out(outp[32:1]), //Data values going out to next matmul - systolic shifting
 .a_data_out(outp[64:33]),
 .b_data_out(outp[96:65]),
 .a_addr(outp[107:97]),
 .b_addr(outp[118:108]),
 .c_addr(outp[129:119]),
 .c_data_available(outp[130]),
 .validity_mask_a_rows(inp[221:218]),
 .validity_mask_a_cols_b_rows(inp[225:222]),
 .validity_mask_b_cols(inp[229:226]),
 .final_mat_mul_size(inp[237:230]),
 .a_loc(inp[245:238]),
 .b_loc(inp[253:246])
);

endmodule

module systolic_array_8_16bit (input clk, input reset, input [785:0] inp, output [433:0] outp);

matmul_8x8_systolic inst(
 .clk(clk),
 .reset(reset),
 .pe_reset(inp[785]),
 .start_mat_mul(inp[0]),
 .done_mat_mul(outp[0]),
 .address_mat_a(inp[16:1]),
 .address_mat_b(inp[32:17]),
 .address_mat_c(inp[48:33]),
 .address_stride_a(inp[64:49]),
 .address_stride_b(inp[80:65]),
 .address_stride_c(inp[96:81]),
 .a_data(inp[224:97]),
 .b_data(inp[352:225]),
 .a_data_in(inp[480:353]), //Data values coming in from previous matmul - systolic connections
 .b_data_in(inp[608:481]),
 .c_data_in(inp[736:609]), //Data values coming in from previous matmul - systolic shifting
 .c_data_out(outp[128:1]), //Data values going out to next matmul - systolic shifting
 .a_data_out(outp[256:129]),
 .b_data_out(outp[384:257]),
 .a_addr(outp[400:385]),
 .b_addr(outp[416:401]),
 .c_addr(outp[432:417]),
 .c_data_available(outp[433]),
 .validity_mask_a_rows(inp[744:737]),
 .validity_mask_a_cols_b_rows(inp[752:745]),
 .validity_mask_b_cols(inp[760:753]),
 .final_mat_mul_size(inp[768:761]),
 .a_loc(inp[776:769]),
 .b_loc(inp[784:777])
);

endmodule

module systolic_array_4_fp16bit (input clk, input reset, input [417:0] inp, output [223:0] outp);

matmul_4x4_fp_systolic inst(
 .clk(clk),
 .reset(reset),
 .pe_reset(inp[417]),
 .start_mat_mul(inp[0]),
 .done_mat_mul(outp[0]),
 .address_mat_a(inp[10:1]),
 .address_mat_b(inp[20:11]),
 .address_mat_c(inp[30:21]),
 .address_stride_a(inp[40:31]),
 .address_stride_b(inp[50:41]),
 .address_stride_c(inp[60:51]),
 .a_data(inp[124:61]),
 .b_data(inp[188:125]),
 .a_data_in(inp[252:189]), //Data values coming in from previous matmul - systolic connections
 .b_data_in(inp[316:253]),
 .c_data_in(inp[380:317]), //Data values coming in from previous matmul - systolic shifting
 .c_data_out(outp[64:1]), //Data values going out to next matmul - systolic shifting
 .a_data_out(outp[128:65]),
 .b_data_out(outp[192:129]),
 .a_addr(outp[202:193]),
 .b_addr(outp[212:203]),
 .c_addr(outp[222:213]),
 .c_data_available(outp[223]),
 .validity_mask_a_rows(inp[384:381]),
 .validity_mask_a_cols_b_rows(inp[388:385]),
 .validity_mask_b_cols(inp[392:389]),
 .final_mat_mul_size(inp[400:393]),
 .a_loc(inp[408:401]),
 .b_loc(inp[416:409])
);

endmodule

module dsp_chain_2_int_sop_2_module (input clk, input reset, input [147:0] inp, output [36:0] outp);

dsp_chain_2_int_sop_2 inst(.clk(clk),.reset(reset),.ax1(inp[17:0]),.ay1(inp[36:18]),.bx1(inp[54:37]),.by1(inp[73:55]),.ax2(inp[91:74]),.ay2(inp[110:92]),.bx2(inp[128:111]),.by2(inp[147:129]),.result(outp[36:0]));

endmodule

module dsp_chain_3_int_sop_2_module (input clk, input reset, input [221:0] inp, output [36:0] outp);

dsp_chain_3_int_sop_2 inst(.clk(clk),.reset(reset),.ax1(inp[17:0]),.ay1(inp[36:18]),.bx1(inp[54:37]),.by1(inp[73:55]),.ax2(inp[91:74]),.ay2(inp[110:92]),.bx2(inp[128:111]),.by2(inp[147:129]),.ax3(inp[165:148]),.ay3(inp[184:166]),.bx3(inp[202:185]),.by3(inp[221:203]),.result(outp[36:0]));

endmodule

module dsp_chain_4_int_sop_2_module (input clk, input reset, input [295:0] inp, output [36:0] outp);

dsp_chain_4_int_sop_2 inst(.clk(clk),.reset(reset),.ax1(inp[17:0]),.ay1(inp[36:18]),.bx1(inp[54:37]),.by1(inp[73:55]),.ax2(inp[91:74]),.ay2(inp[110:92]),.bx2(inp[128:111]),.by2(inp[147:129]),.ax3(inp[165:148]),.ay3(inp[184:166]),.bx3(inp[202:185]),.by3(inp[221:203]),.ax4(inp[239:222]),.ay4(inp[258:240]),.bx4(inp[276:259]),.by4(inp[295:277]),.result(outp[36:0]));

endmodule

module dsp_chain_2_fp16_sop2_mult_module (input clk, input reset, input [127:0] inp, output [31:0] outp);

dsp_chain_2_fp16_sop2_mult inst(.clk(clk),.reset(reset),.top_a1(inp[15:0]),.top_b1(inp[31:16]),.bot_a1(inp[47:32]),.bot_b1(inp[63:48]),.top_a2(inp[79:64]),.top_b2(inp[95:80]),.bot_a2(inp[111:96]),.bot_b2(inp[127:112]),.result(outp));

endmodule

module dsp_chain_3_fp16_sop2_mult_module (input clk, input reset, input [191:0] inp, output [31:0] outp);

dsp_chain_3_fp16_sop2_mult inst(.clk(clk),.reset(reset),.top_a1(inp[15:0]),.top_b1(inp[31:16]),.bot_a1(inp[47:32]),.bot_b1(inp[63:48]),.top_a2(inp[79:64]),.top_b2(inp[95:80]),.bot_a2(inp[111:96]),.bot_b2(inp[127:112]),.top_a3(inp[143:128]),.top_b3(inp[159:144]),.bot_a3(inp[175:160]),.bot_b3(inp[191:176]),.result(outp));

endmodule

module dsp_chain_4_fp16_sop2_mult_module (input clk, input reset, input [255:0] inp, output [31:0] outp);

dsp_chain_4_fp16_sop2_mult inst(.clk(clk),.reset(reset),.top_a1(inp[15:0]),.top_b1(inp[31:16]),.bot_a1(inp[47:32]),.bot_b1(inp[63:48]),.top_a2(inp[79:64]),.top_b2(inp[95:80]),.bot_a2(inp[111:96]),.bot_b2(inp[127:112]),.top_a3(inp[143:128]),.top_b3(inp[159:144]),.bot_a3(inp[175:160]),.bot_b3(inp[191:176]),.top_a4(inp[207:192]),.top_b4(inp[223:208]),.bot_a4(inp[239:224]),.bot_b4(inp[255:240]),.result(outp));

endmodule

module tensor_block_bf16_module (input clk, input reset, input [264:0] inp, output [271:0] outp);

tensor_block_bf16 inst(
	.clk(clk),
	.reset(reset),

	.data_in(inp[79:0]),
	.cascade_in(inp[159:80]),
	.acc0_in(inp[191:160]),
	.acc1_in(inp[223:192]),
	.acc2_in(inp[255:224]),
	.accumulator_input1_select(inp[258:256]),

	.out0(outp[31:0]),
	.out1(outp[63:32]),
	.out2(outp[95:64]),
	.cascade_out(outp[175:96]),
	.acc0_out(outp[207:176]),
	.acc1_out(outp[239:208]),
	.acc2_out(outp[271:240]),

	.mux1_select(inp[259]),
	.dot_unit_input_1_enable(inp[260]),
	.bank0_data_in_enable(inp[261]),
	.bank1_data_in_enable(inp[262]),
	.cascade_out_select(inp[263]),
	.dot_unit_input_2_select(inp[264])

	);

endmodule

module tensor_block_int8_module (input clk, input reset, input [264:0] inp, output [250:0] outp);

tensor_block inst(
	.clk(clk),
	.reset(reset),

	.data_in(inp[79:0]),
	.cascade_in(inp[159:80]),
	.acc0_in(inp[191:160]),
	.acc1_in(inp[223:192]),
	.acc2_in(inp[255:224]),
	.accumulator_input1_select(inp[258:256]),

	.out0(outp[24:0]),
	.out1(outp[49:25]),
	.out2(outp[74:50]),
	.cascade_out(outp[154:75]),
	.acc0_out(outp[186:155]),
	.acc1_out(outp[218:187]),
	.acc2_out(outp[250:219]),

	.mux1_select(inp[259]),
	.dot_unit_input_1_enable(inp[260]),
	.bank0_data_in_enable(inp[261]),
	.bank1_data_in_enable(inp[262]),
	.cascade_out_select(inp[263]),
	.dot_unit_input_2_select(inp[264])

	);

endmodule


module activation_32_8bit_module (input clk, input reset, input [260:0] inp, output [257:0] outp);

activation_32_8bit inst (
    .activation_type(inp[0]),
    .enable_activation(inp[1]),
    .in_data_available(inp[2]),
    .inp_data(inp[258:3]),
    .out_data(outp[255:0]),
    .out_data_available(outp[256]),
    .validity_mask(inp[260:259]),
    .done_activation(outp[257]),
    .clk(clk),
    .reset(reset)
);

endmodule

module activation_32_16bit_module (input clk, input reset, input [515:0] inp, output [513:0] outp);

activation_32_16bit inst (
    .activation_type(inp[0]),
    .enable_activation(inp[1]),
    .in_data_available(inp[2]),
    .inp_data(inp[514:3]),
    .out_data(outp[511:0]),
    .out_data_available(outp[512]),
    .validity_mask(inp[515:514]),
    .done_activation(outp[513]),
    .clk(clk),
    .reset(reset)
);

endmodule

module fsm(input clk, input reset, input i1, input i2, output reg o);
// mealy machine

reg [1:0] current_state; 
reg [1:0] next_state;

wire [1:0] inp; 
assign inp = {i2,i1}; 

always@(posedge clk) begin 
	if (reset == 1'b1) begin 
		current_state <= 1'b0; 
	end
	else begin 
		current_state <= next_state; 
	end
end

always@(posedge clk) begin 

	next_state <= current_state; 

	case(current_state)
		2'b00:	begin 
							if(inp == 2'b00) begin 
								next_state <= 2'b00; 
								o <= 1'b0; 
							end
							if (inp == 2'b01) begin 
								next_state <= 2'b11;
								o <= 1'b1;
							end
							if(inp == 2'b10) begin
  							next_state <= 2'b01;
  							o <= 1'b0;
							end
							if(inp == 2'b11) begin
							  next_state <= 2'b10;
							  o <= 1'b0;
							end
					 	end 
		2'b01:	begin 
							if(inp == 2'b00) begin
							  next_state <= 2'b01;
							  o <= 1'b1;
							end
							if(inp == 2'b01) begin
							  next_state <= 2'b01;
							  o <= 1'b0;
							end
							if(inp == 2'b10) begin
							  next_state <= 2'b10;
							  o <= 1'b1;
							end
							if(inp == 2'b11) begin
							  next_state <= 2'b00;
							  o <= 1'b1;
							end
						end
		2'b10:	begin 
							if(inp == 2'b00) begin
							  next_state <= 2'b11;
							  o <= 1'b0;
							end
							if(inp == 2'b01) begin
							  next_state <= 2'b10;
							  o <= 1'b1;
							end
							if(inp == 2'b10) begin
							  next_state <= 2'b11;
							  o <= 1'b0;
							end
							if(inp == 2'b11) begin
							  next_state <= 2'b01;
							  o <= 1'b1;
							end
						end
		2'b11:	begin 
							if(inp == 2'b00) begin
  							next_state <= 2'b00;
							  o <= 1'b1;
							end
							if(inp == 2'b01) begin
							  next_state <= 2'b11;
							  o <= 1'b1;
							end
							if(inp == 2'b10) begin
							  next_state <= 2'b10;
							  o <= 1'b1;
							end
							if(inp == 2'b11) begin
							  next_state <= 2'b01;
							  o <= 1'b1;
							end
						end
//		defualt:	begin  
//								next_state <= 2'b00;
//								o <= 1'b0; 
//							end
	endcase
end 

endmodule 
module xor_module (input clk, input reset, input i1, input i2, output reg o);

always@(posedge clk) begin 
if (reset ==1'b1) begin 
o<= 1'b0; 
end
else begin
o <= i1^i2; 
end 
end
endmodule
module mux_module (input clk, input reset, input i1, input i2, output reg o, input sel);

always@(posedge clk) begin 
if (reset ==1'b1) begin 
	o<= 1'b0; 
end

else begin
	if (sel == 1'b0) begin 
		o <= i1;
	end
	else begin
		o <= i2; 
	end 
end 

end

endmodule

`ifdef complex_dsp
module int_sop_2_dspchain (mode_sigs,
	clk,
	reset,
	ax,
	ay,
	bx,
	by,
	chainin,
	resulta,
	chainout);
input [10:0] mode_sigs;
input clk;
input reset; 
input [17:0] ax, bx;
input [18:0] ay, by;
input [36:0] chainin;

output [36:0] resulta;
output [36:0] chainout;

wire [11:0] mode_sigs_int;
assign mode_sigs_int = {1'b0, mode_sigs};

int_sop_2 inst1(.clk(clk),.reset(reset),.ax(ax),.bx(bx),.ay(ay),.by(by),.mode_sigs(mode_sigs_int),.chainin(chainin),.result(resulta),.chainout(chainout)); 

endmodule
`else
module int_sop_2_dspchain (mode_sigs,
	clk,
	reset,
	ax,
	ay,
	bx,
	by,
	chainin,
	resulta,
	chainout);
input [10:0] mode_sigs;
input clk;
input reset; 
input [17:0] ax, bx;
input [18:0] ay, by;
input [36:0] chainin;

output [36:0] resulta;
output [36:0] chainout;
reg [17:0] ax_reg;
reg [18:0] ay_reg;
reg [17:0] bx_reg;
reg [18:0] by_reg;
reg [36:0] resulta_reg;
reg [36:0] resultaxy_reg;
reg [36:0] resultbxy_reg;
always @(posedge clk) begin
  if(reset) begin
    resulta_reg <= 0;
    ax_reg <= 0;
    ay_reg <= 0;
    bx_reg <= 0;
    by_reg <= 0;
  end
  else begin
    ax_reg <= ax;
    ay_reg <= ay;
    bx_reg <= bx;
    by_reg <= by;
    resultaxy_reg <= ax_reg * ay_reg;
    resultbxy_reg <= bx_reg * by_reg;
    resulta_reg <= resultaxy_reg + resultbxy_reg + chainin;
  end
end
assign resulta = resulta_reg;
assign chainout = resulta_reg;
endmodule
`endif

`ifdef complex_dsp
module fp16_sop2_mult_dspchain (clk,reset,top_a,top_b,bot_a,bot_b,fp32_in,mode_sigs,chainin,chainout,result);
input clk; 
input reset;
input [10:0] mode_sigs; 
input [15:0] top_a,top_b,bot_a,bot_b;
input [31:0] chainin,fp32_in; 
output [31:0] chainout, result;

fp16_sop2_mult inst1(.clk(clk),.reset(reset),.top_a(top_a),.top_b(top_b),.bot_a(bot_a),.bot_b(bot_b),.fp32_in(fp32_in),.mode_sigs(mode_sigs),.chainin(chainin),.chainout(chainout),.result(result)); 

endmodule

`else
module fp16_sop2_mult_dspchain (clk,reset,top_a,top_b,bot_a,bot_b,fp32_in,mode_sigs,chainin,chainout,result);
input clk; 
input reset;
input [10:0] mode_sigs; 
input [15:0] top_a,top_b,bot_a,bot_b;
input [31:0] chainin,fp32_in; 
output [31:0] chainout, result; 

reg [15:0] top_a_reg,top_b_reg,bot_a_reg,bot_b_reg; 
reg [31:0] chainin_reg; 
reg [31:0] r1,r2,r3; 
always@(posedge clk) begin 
if(reset) begin 
top_a_reg<= 16'b0; 
top_b_reg<= 16'b0; 
bot_a_reg<= 16'b0; 
bot_b_reg<= 16'b0;
//result<=32'b0;
//chainout<=32'b0;
chainin_reg<=32'b0;   
end
else begin 
top_a_reg<=top_a; 
top_b_reg<=top_b; 
bot_a_reg<=bot_a;
bot_b_reg<=bot_b;
//chainout<=result;
chainin_reg<=chainin; 
end
end

wire [4:0] flags1,flags2,flags3,flags4; 

FPMult_16_dspchain inst1(.clk(clk),.rst(reset),.a(top_a_reg),.b(top_b_reg),.flags(flags1),.result(r1)); 
FPMult_16_dspchain inst2(.clk(clk),.rst(reset),.a(bot_a_reg),.b(bot_b_reg),.flags(flags2),.result(r2));
FPAddSub_single_dspchain inst3(.clk(clk),.rst(reset),.a(r1),.b(r2),.flags(flags3),.operation(1'b1),.result(r3));
FPAddSub_single_dspchain inst4(.clk(clk),.rst(reset),.a(r3),.b(chainin),.flags(flags4),.operation(1'b1),.result(result));
assign chainout = result; 
endmodule
//`endif

//`timescale 1ns / 1ps


// IEEE Half Precision => 5 = 5, 10 = 10



//`define IEEE_COMPLIANCE 1


//////////////////////////////////////////////////////////////////////////////////
//
// Module Name:    FPMult
//
//////////////////////////////////////////////////////////////////////////////////

module FPMult_16_dspchain(
		clk,
		rst,
		a,
		b,
		result,
		flags
    );
	
	// Input Ports
	input clk ;							// Clock
	input rst ;							// Reset signal
	input [16-1:0] a;						// Input A, a 32-bit floating point number
	input [16-1:0] b;						// Input B, a 32-bit floating point number
	
	// Output ports
	output [32-1:0] result ;					// Product, result of the operation, 32-bit FP number
	output [4:0] flags ;						// Flags indicating exceptions according to IEEE754
	
	// Internal signals
	wire [32-1:0] Z_int ;					// Product, result of the operation, 32-bit FP number
	wire [4:0] Flags_int ;						// Flags indicating exceptions according to IEEE754
	
	wire Sa ;							// A's sign
	wire Sb ;							// B's sign
	wire Sp ;							// Product sign
	wire [5-1:0] Ea ;					// A's 5
	wire [5-1:0] Eb ;					// B's 5
	wire [2*10+1:0] Mp ;					// Product 10
	wire [4:0] InputExc ;						// Exceptions in inputs
	wire [23-1:0] NormM ;					// Normalized 10
	wire [8:0] NormE ;					// Normalized 5
	wire [23:0] RoundM ;					// Normalized 10
	wire [8:0] RoundE ;					// Normalized 5
	wire [23:0] RoundMP ;					// Normalized 10
	wire [8:0] RoundEP ;					// Normalized 5
	wire GRS ;

	//reg [63:0] pipe_0;						// Pipeline register Input->Prep
	reg [2*16-1:0] pipe_0;					// Pipeline register Input->Prep

	//reg [92:0] pipe_1;						// Pipeline register Prep->Execute
	//reg [3*10+2*5+7:0] pipe_1;			// Pipeline register Prep->Execute
	reg [3*10+2*5+18:0] pipe_1;

	//reg [38:0] pipe_2;						// Pipeline register Execute->Normalize
	reg [23+8+7:0] pipe_2;				// Pipeline register Execute->Normalize

	//reg [72:0] pipe_3;						// Pipeline register Normalize->Round
	reg [2*23+2*8+10:0] pipe_3;			// Pipeline register Normalize->Round

	//reg [36:0] pipe_4;						// Pipeline register Round->Output
	reg [32+4:0] pipe_4;					// Pipeline register Round->Output
	
	assign result = pipe_4[32+4:5] ;
	assign flags = pipe_4[4:0] ;
	
	// Prepare the operands for alignment and check for exceptions
	FPMult_PrepModule_dspchain PrepModule(clk, rst, pipe_0[2*16-1:16], pipe_0[16-1:0], Sa, Sb, Ea[5-1:0], Eb[5-1:0], Mp[2*10+1:0], InputExc[4:0]) ;

	// Perform (unsigned) 10 multiplication
	FPMult_ExecuteModule_dspchain ExecuteModule(pipe_1[3*10+5*2+7:2*10+2*5+8], pipe_1[2*10+2*5+7:2*10+7], pipe_1[2*10+6:5], pipe_1[2*10+2*5+6:2*10+5+7], pipe_1[2*10+5+6:2*10+7], pipe_1[2*10+2*5+8], pipe_1[2*10+2*5+7], Sp, NormE[8:0], NormM[23-1:0], GRS) ;

	// Round result and if necessary, perform a second (post-rounding) normalization step
	FPMult_NormalizeModule_dspchain NormalizeModule(pipe_2[23-1:0], pipe_2[23+8:23], RoundE[8:0], RoundEP[8:0], RoundM[23:0], RoundMP[23:0]) ;		

	// Round result and if necessary, perform a second (post-rounding) normalization step
	//FPMult_RoundModule RoundModule(pipe_3[47:24], pipe_3[23:0], pipe_3[65:57], pipe_3[56:48], pipe_3[66], pipe_3[67], pipe_3[72:68], Z_int[31:0], Flags_int[4:0]) ;		
	FPMult_RoundModule_dspchain RoundModule(pipe_3[2*23+1:23+1], pipe_3[23:0], pipe_3[2*8+2*23+3:2*23+8+3], pipe_3[2*23+8+2:2*23+2], pipe_3[2*23+2*8+4], pipe_3[2*23+2*8+5], pipe_3[2*23+2*8+10:2*23+2*8+6], Z_int[32-1:0], Flags_int[4:0]) ;		


//adding always@ (*) instead of posedge clock to make design combinational
	always @ (*) begin	
		if(rst) begin
			pipe_0 = 0;
			pipe_1 = 0;
			pipe_2 = 0; 
			pipe_3 = 0;
			pipe_4 = 0;
		end 
		else begin		
			/* PIPE 0
				[2*16-1:16] A
				[16-1:0] B
			*/
                       pipe_0 = {a, b} ;


			/* PIPE 1
				[2*5+3*10 + 18: 2*5+2*10 + 18] //pipe_0[16+10-1:16] , 10 of A
				[2*5+2*10 + 17 :2*5+2*10 + 9] // pipe_0[8:0]
				[2*5+2*10 + 8] Sa
				[2*5+2*10 + 7] Sb
				[2*5+2*10 + 6:5+2*10+7] Ea
				[5 +2*10+6:2*10+7] Eb
				[2*10+1+5:5] Mp
				[4:0] InputExc
			*/
			//pipe_1 <= {pipe_0[16+10-1:16], pipe_0[10_MUL_SPLIT_LSB-1:0], Sa, Sb, Ea[5-1:0], Eb[5-1:0], Mp[2*10-1:0], InputExc[4:0]} ;
			pipe_1 = {pipe_0[16+10-1:16], pipe_0[8:0], Sa, Sb, Ea[5-1:0], Eb[5-1:0], Mp[2*10+1:0], InputExc[4:0]} ;
			
			/* PIPE 2
				[8 + 23 + 7:8 + 23 + 3] InputExc
				[8 + 23 + 2] GRS
				[8 + 23 + 1] Sp
				[8 + 23:23] NormE
				[23-1:0] NormM
			*/
			pipe_2 = {pipe_1[4:0], GRS, Sp, NormE[8:0], NormM[23-1:0]} ;
			/* PIPE 3
				[2*8+2*23+10:2*8+2*23+6] InputExc
				[2*8+2*23+5] GRS
				[2*8+2*23+4] Sp	
				[2*8+2*23+3:8+2*23+3] RoundE
				[8+2*23+2:2*23+2] RoundEP
				[2*23+1:23+1] RoundM
				[23:0] RoundMP
			*/
			pipe_3 = {pipe_2[8 + 23 + 7:8 + 23 + 1], RoundE[8:0], RoundEP[8:0], RoundM[23:0], RoundMP[23:0]} ;
			/* PIPE 4
				[16+4:5] Z
				[4:0] Flags
			*/				
			pipe_4 = {Z_int[32-1:0], Flags_int[4:0]} ;
		end
	end
		
endmodule



module FPMult_PrepModule_dspchain (
		clk,
		rst,
		a,
		b,
		Sa,
		Sb,
		Ea,
		Eb,
		Mp,
		InputExc
	);
	
	// Input ports
	input clk ;
	input rst ;
	input [16-1:0] a ;								// Input A, a 32-bit floating point number
	input [16-1:0] b ;								// Input B, a 32-bit floating point number
	
	// Output ports
	output Sa ;										// A's sign
	output Sb ;										// B's sign
	output [5-1:0] Ea ;								// A's 5
	output [5-1:0] Eb ;								// B's 5
	output [2*10+1:0] Mp ;							// 10 product
	output [4:0] InputExc ;						// Input numbers are exceptions
	
	// Internal signals							// If signal is high...
	wire ANaN ;										// A is a signalling NaN
	wire BNaN ;										// B is a signalling NaN
	wire AInf ;										// A is infinity
	wire BInf ;										// B is infinity
    wire [10-1:0] Ma;
    wire [10-1:0] Mb;
	
	assign ANaN = &(a[16-2:10]) &  |(a[16-2:10]) ;			// All one 5 and not all zero 10 - NaN
	assign BNaN = &(b[16-2:10]) &  |(b[10-1:0]);			// All one 5 and not all zero 10 - NaN
	assign AInf = &(a[16-2:10]) & ~|(a[16-2:10]) ;		// All one 5 and all zero 10 - Infinity
	assign BInf = &(b[16-2:10]) & ~|(b[16-2:10]) ;		// All one 5 and all zero 10 - Infinity
	
	// Check for any exceptions and put all flags into exception vector
	assign InputExc = {(ANaN | BNaN | AInf | BInf), ANaN, BNaN, AInf, BInf} ;
	//assign InputExc = {(ANaN | ANaN | BNaN |BNaN), ANaN, ANaN, BNaN,BNaN} ;
	
	// Take input numbers apart
	assign Sa = a[16-1] ;							// A's sign
	assign Sb = b[16-1] ;							// B's sign
	assign Ea = a[16-2:10];						// Store A's 5 in Ea, unless A is an exception
	assign Eb = b[16-2:10];						// Store B's 5 in Eb, unless B is an exception	
//    assign Ma = a[10_MSB:10_LSB];
  //  assign Mb = b[10_MSB:10_LSB];
	

	// Actual 10 multiplication occurs here
	//assign Mp = ({4'b0001, a[10-1:0]}*{4'b0001, b[10-1:9]}) ;
	assign Mp = ({1'b1,a[10-1:0]}*{1'b1, b[10-1:0]}) ;

	
    //We multiply part of the 10 here
    //Full 10 of A
    //Bits 10_MUL_SPLIT_MSB:10_MUL_SPLIT_LSB of B
   // wire [`ACTUAL_10-1:0] inp_A;
   // wire [`ACTUAL_10-1:0] inp_B;
   // assign inp_A = {1'b1, Ma};
   // assign inp_B = {{(10-(10_MUL_SPLIT_MSB-10_MUL_SPLIT_LSB+1)){1'b0}}, 1'b1, Mb[10_MUL_SPLIT_MSB:10_MUL_SPLIT_LSB]};
   // DW02_mult #(`ACTUAL_10,`ACTUAL_10) u_mult(.A(inp_A), .B(inp_B), .TC(1'b0), .PRODUCT(Mp));
endmodule


module FPMult_ExecuteModule_dspchain(
		a,
		b,
		MpC,
		Ea,
		Eb,
		Sa,
		Sb,
		Sp,
		NormE,
		NormM,
		GRS
    );

	// Input ports
	input [10-1:0] a ;
	input [2*5:0] b ;
	input [2*10+1:0] MpC ;
	input [5-1:0] Ea ;						// A's 5
	input [5-1:0] Eb ;						// B's 5
	input Sa ;								// A's sign
	input Sb ;								// B's sign
	
	// Output ports
	output Sp ;								// Product sign
	output [8:0] NormE ;													// Normalized 5
	output [23-1:0] NormM ;												// Normalized 10
	output GRS ;
	
	wire [2*10+1:0] Mp ;
	
	assign Sp = (Sa ^ Sb) ;												// Equal signs give a positive product
	
   // wire [`ACTUAL_10-1:0] inp_a;
   // wire [`ACTUAL_10-1:0] inp_b;
   // assign inp_a = {1'b1, a};
   // assign inp_b = {{(10-10_MUL_SPLIT_LSB){1'b0}}, 1'b0, b};
   // DW02_mult #(`ACTUAL_10,`ACTUAL_10) u_mult(.A(inp_a), .B(inp_b), .TC(1'b0), .PRODUCT(Mp_temp));
   // DW01_add #(2*`ACTUAL_10) u_add(.A(Mp_temp), .B(MpC<<10_MUL_SPLIT_LSB), .CI(1'b0), .SUM(Mp), .CO());

	//assign Mp = (MpC<<(2*5+1)) + ({4'b0001, a[10-1:0]}*{1'b0, b[2*5:0]}) ;
	assign Mp = MpC;


	assign NormM = (Mp[2*10+1] ? Mp[2*10:0] : Mp[2*10-1:0]); 	// Check for overflow
	assign NormE = (Ea + Eb + Mp[2*10+1]);								// If so, increment 5
	
	assign GRS = ((Mp[10]&(Mp[10+1]))|(|Mp[10-1:0])) ;
	
endmodule

module FPMult_NormalizeModule_dspchain(
		NormM,
		NormE,
		RoundE,
		RoundEP,
		RoundM,
		RoundMP
    );

	// Input Ports
	input [23-1:0] NormM ;									// Normalized 10
	input [8:0] NormE ;									// Normalized 5

	// Output Ports
	output [8:0] RoundE ;
	output [8:0] RoundEP ;
	output [23:0] RoundM ;
	output [23:0] RoundMP ; 
	
// 5 = 5 
// 5 -1 = 4
// NEED to subtract 2^4 -1 = 15

wire [8-1 : 0] bias;

assign bias =  ((1<< (8 -1)) -1);

	assign RoundE = NormE - 9'd15 - 9'd15 + 9'd127 ;
	assign RoundEP = NormE - 9'd15 - 9'd15 + 9'd127 ;
	assign RoundM = NormM ;
	assign RoundMP = NormM ;

endmodule

module FPMult_RoundModule_dspchain(
		RoundM,
		RoundMP,
		RoundE,
		RoundEP,
		Sp,
		GRS,
		InputExc,
		Z,
		Flags
    );

	// Input Ports
	input [23:0] RoundM ;									// Normalized 10
	input [23:0] RoundMP ;									// Normalized 5
	input [8:0] RoundE ;									// Normalized 10 + 1
	input [8:0] RoundEP ;									// Normalized 5 + 1
	input Sp ;												// Product sign
	input GRS ;
	input [4:0] InputExc ;
	
	// Output Ports
	output [32-1:0] Z ;										// Final product
	output [4:0] Flags ;
	
	// Internal Signals
	wire [8:0] FinalE ;									// Rounded 5
	wire [23:0] FinalM;
	wire [23:0] PreShiftM;
	
	assign PreShiftM = GRS ? RoundMP : RoundM ;	// Round up if R and (G or S)
	
	// Post rounding normalization (potential one bit shift> use shifted 10 if there is overflow)
	assign FinalM = (PreShiftM[23] ? {1'b0, PreShiftM[23:1]} : PreShiftM[23:0]) ;
	assign FinalE = (PreShiftM[23] ? RoundEP : RoundE) ; // Increment 5 if a shift was done
	
	
	assign Z = {Sp, FinalE[8-1:0], FinalM[21-1:0], 2'b0} ;   // Putting the pieces together
	assign Flags = InputExc[4:0];

endmodule


module FPAddSub_single_dspchain(
		clk,
		rst,
		a,
		b,
		operation,
		result,
		flags
	);

// Clock and reset
	input clk ;										// Clock signal
	input rst ;										// Reset (active high, resets pipeline registers)
	
	// Input ports
	input [31:0] a ;								// Input A, a 32-bit floating point number
	input [31:0] b ;								// Input B, a 32-bit floating point number
	input operation ;								// Operation select signal
	
	// Output ports
	output [31:0] result ;						// Result of the operation
	output [4:0] flags ;							// Flags indicating exceptions according to IEEE754

	reg [68:0]pipe_1;
	reg [54:0]pipe_2;
	reg [45:0]pipe_3;


//internal module wires

//output ports
	wire Opout;
	wire Sa;
	wire Sb;
	wire MaxAB;
	wire [7:0] CExp;
	wire [4:0] Shift;
	wire [22:0] Mmax;
	wire [4:0] InputExc;
	wire [23:0] Mmin_3;

	wire [32:0] SumS_5 ;
	wire [4:0] Shift_1;							
	wire PSgn ;							
	wire Opr ;	
	
	wire [22:0] NormM ;				// Normalized mantissa
	wire [8:0] NormE ;					// Adjusted exponent
	wire ZeroSum ;						// Zero flag
	wire NegE ;							// Flag indicating negative exponent
	wire R ;								// Round bit
	wire S ;								// Final sticky bit
	wire FG ;

FPAddSub_a_dspchain M1(a,b,operation,Opout,Sa,Sb,MaxAB,CExp,Shift,Mmax,InputExc,Mmin_3);

FpAddSub_b_dspchain M2(pipe_1[51:29],pipe_1[23:0],pipe_1[67],pipe_1[66],pipe_1[65],pipe_1[68],SumS_5,Shift_1,PSgn,Opr);

FPAddSub_c_dspchain M3(pipe_2[54:22],pipe_2[21:17],pipe_2[16:9],NormM,NormE,ZeroSum,NegE,R,S,FG);

FPAddSub_d_dspchain M4(pipe_3[13],pipe_3[22:14],pipe_3[45:23],pipe_3[11],pipe_3[10],pipe_3[9],pipe_3[8],pipe_3[7],pipe_3[6],pipe_3[5],pipe_3[12],pipe_3[4:0],result,flags );


always @ (posedge clk) begin	
		if(rst) begin
			pipe_1 <= 0;
			pipe_2 <= 0;
			pipe_3 <= 0;
		end 
		else begin
/*
pipe_1:
	[68] Opout;
	[67] Sa;
	[66] Sb;
	[65] MaxAB;
	[64:57] CExp;
	[56:52] Shift;
	[51:29] Mmax;
	[28:24] InputExc;
	[23:0] Mmin_3;	
*/

pipe_1 <= {Opout,Sa,Sb,MaxAB,CExp,Shift,Mmax,InputExc,Mmin_3};

/*
pipe_2:
	[54:22]SumS_5;
	[21:17]Shift;
	[16:9]CExp;	
	[8]Sa;
	[7]Sb;
	[6]operation;
	[5]MaxAB;	
	[4:0]InputExc
*/

pipe_2 <= {SumS_5,Shift_1,pipe_1[64:57], pipe_1[67], pipe_1[66], pipe_1[68], pipe_1[65], pipe_1[28:24] };

/*
pipe_3:
	[45:23] NormM ;				
	[22:14] NormE ;					
	[13]ZeroSum ;						
	[12]NegE ;							
	[11]R ;								
	[10]S ;								
	[9]FG ;
	[8]Sa;
	[7]Sb;
	[6]operation;
	[5]MaxAB;	
	[4:0]InputExc
*/

pipe_3 <= {NormM,NormE,ZeroSum,NegE,R,S,FG, pipe_2[8], pipe_2[7], pipe_2[6], pipe_2[5], pipe_2[4:0] };

end
end

endmodule

// Prealign + Align + Shift 1 + Shift 2
module FPAddSub_a_dspchain(
		A,
		B,
		operation,
		Opout,
		Sa,
		Sb,
		MaxAB,
		CExp,
		Shift,
		Mmax,
		InputExc,
		Mmin_3
		
		
	);
	
	// Input ports
	input [31:0] A ;										// Input A, a 32-bit floating point number
	input [31:0] B ;										// Input B, a 32-bit floating point number
	input operation ;
	
	//output ports
	output Opout;
	output Sa;
	output Sb;
	output MaxAB;
	output [7:0] CExp;
	output [4:0] Shift;
	output [22:0] Mmax;
	output [4:0] InputExc;
	output [23:0] Mmin_3;	
							
	wire [9:0] ShiftDet ;							
	wire [30:0] Aout ;
	wire [30:0] Bout ;
	

	// Internal signals									// If signal is high...
	wire ANaN ;												// A is a NaN (Not-a-Number)
	wire BNaN ;												// B is a NaN
	wire AInf ;												// A is infinity
	wire BInf ;												// B is infinity
	wire [7:0] DAB ;										// ExpA - ExpB					
	wire [7:0] DBA ;										// ExpB - ExpA	
	
	assign ANaN = &(A[30:23]) & |(A[22:0]) ;		// All one exponent and not all zero mantissa - NaN
	assign BNaN = &(B[30:23]) & |(B[22:0]);		// All one exponent and not all zero mantissa - NaN
	assign AInf = &(A[30:23]) & ~|(A[22:0]) ;	// All one exponent and all zero mantissa - Infinity
	assign BInf = &(B[30:23]) & ~|(B[22:0]) ;	// All one exponent and all zero mantissa - Infinity
	
	// Put all flags into exception vector
	assign InputExc = {(ANaN | BNaN | AInf | BInf), ANaN, BNaN, AInf, BInf} ;
	
	//assign DAB = (A[30:23] - B[30:23]) ;
	//assign DBA = (B[30:23] - A[30:23]) ;
  assign DAB = (A[30:23] + ~(B[30:23]) + 1) ;
	assign DBA = (B[30:23] + ~(A[30:23]) + 1) ;

	assign Sa = A[31] ;									// A's sign bit
	assign Sb = B[31] ;									// B's sign	bit
	assign ShiftDet = {DBA[4:0], DAB[4:0]} ;		// Shift data
	assign Opout = operation ;
	assign Aout = A[30:0] ;
	assign Bout = B[30:0] ;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Output ports
													// Number of steps to smaller mantissa shift right
	wire [22:0] Mmin_1 ;							// Smaller mantissa 
	
	// Internal signals
	//wire BOF ;										// Check for shifting overflow if B is larger
	//wire AOF ;										// Check for shifting overflow if A is larger
	
	assign MaxAB = (Aout[30:0] < Bout[30:0]) ;	
	//assign BOF = ShiftDet[9:5] < 25 ;		// Cannot shift more than 25 bits
	//assign AOF = ShiftDet[4:0] < 25 ;		// Cannot shift more than 25 bits
	
	// Determine final shift value
	//assign Shift = MaxAB ? (BOF ? ShiftDet[9:5] : 5'b11001) : (AOF ? ShiftDet[4:0] : 5'b11001) ;
	
	assign Shift = MaxAB ? ShiftDet[9:5] : ShiftDet[4:0] ;
	
	// Take out smaller mantissa and append shift space
	assign Mmin_1 = MaxAB ? Aout[22:0] : Bout[22:0] ; 
	
	// Take out larger mantissa	
	assign Mmax = MaxAB ? Bout[22:0]: Aout[22:0] ;	
	
	// Common exponent
	assign CExp = (MaxAB ? Bout[30:23] : Aout[30:23]) ;	

// Input ports
					// Smaller mantissa after 16|12|8|4 shift
	wire [2:0] Shift_1 ;						// Shift amount
	
	assign Shift_1 = Shift [4:2];

	wire [23:0] Mmin_2 ;						// The smaller mantissa
	
	// Internal signals
	reg	  [23:0]		Lvl1;
	reg	  [23:0]		Lvl2;
	wire    [47:0]    Stage1;	
	integer           i;                // Loop variable
	
	always @(*) begin						
		// Rotate by 16?
		Lvl1 <= Shift_1[2] ? {17'b00000000000000001, Mmin_1[22:16]} : {1'b1, Mmin_1}; 
	end
	
	assign Stage1 = {Lvl1, Lvl1};
	
	always @(*) begin    					// Rotate {0 | 4 | 8 | 12} bits
	  case (Shift_1[1:0])
			// Rotate by 0	
			2'b00:  Lvl2 <= Stage1[23:0];       			
			// Rotate by 4	
			2'b01:  begin for (i=0; i<=23; i=i+1) begin Lvl2[i] <= Stage1[i+4]; end Lvl2[23:19] <= 0; end
			// Rotate by 8
			2'b10:  begin for (i=0; i<=23; i=i+1) begin Lvl2[i] <= Stage1[i+8]; end Lvl2[23:15] <= 0; end
			// Rotate by 12	
			2'b11:  begin for (i=0; i<=23; i=i+1) begin Lvl2[i] <= Stage1[i+12]; end Lvl2[23:11] <= 0; end
	  endcase
	end
	
	// Assign output to next shift stage
	assign Mmin_2 = Lvl2;
								// Smaller mantissa after 16|12|8|4 shift
	wire [1:0] Shift_2 ;						// Shift amount
	
	assign Shift_2 =Shift  [1:0] ;
					// The smaller mantissa
	
	// Internal Signal
	reg	  [23:0]		Lvl3;
	wire    [47:0]    Stage2;	
	integer           j;               // Loop variable
	
	assign Stage2 = {Mmin_2, Mmin_2};

	always @(*) begin    // Rotate {0 | 1 | 2 | 3} bits
	  case (Shift_2[1:0])
			// Rotate by 0
			2'b00:  Lvl3 <= Stage2[23:0];   
			// Rotate by 1
			2'b01:  begin for (j=0; j<=23; j=j+1)  begin Lvl3[j] <= Stage2[j+1]; end Lvl3[23] <= 0; end 
			// Rotate by 2
			2'b10:  begin for (j=0; j<=23; j=j+1)  begin Lvl3[j] <= Stage2[j+2]; end Lvl3[23:22] <= 0; end 
			// Rotate by 3
			2'b11:  begin for (j=0; j<=23; j=j+1)  begin Lvl3[j] <= Stage2[j+3]; end Lvl3[23:21] <= 0; end 	  
	  endcase
	end
	
	// Assign output
	assign Mmin_3 = Lvl3;	

	
endmodule

module FpAddSub_b_dspchain(
		Mmax,
		Mmin,
		Sa,
		Sb,
		MaxAB,
		OpMode,
		SumS_5,
		Shift,
		PSgn,
		Opr
);
	input [22:0] Mmax ;					// The larger mantissa
	input [23:0] Mmin ;					// The smaller mantissa
	input Sa ;								// Sign bit of larger number
	input Sb ;								// Sign bit of smaller number
	input MaxAB ;							// Indicates the larger number (0/A, 1/B)
	input OpMode ;							// Operation to be performed (0/Add, 1/Sub)
	
	// Output ports
	wire [32:0] Sum ;	
						// Output ports
	output [32:0] SumS_5 ;					// Mantissa after 16|0 shift
	output [4:0] Shift ;					// Shift amount				// The result of the operation
	output PSgn ;							// The sign for the result
	output Opr ;							// The effective (performed) operation

	assign Opr = (OpMode^Sa^Sb); 		// Resolve sign to determine operation

	// Perform effective operation
	assign Sum = (OpMode^Sa^Sb) ? ({1'b1, Mmax, 8'b00000000} - {Mmin, 8'b00000000}) : ({1'b1, Mmax, 8'b00000000} + {Mmin, 8'b00000000}) ;
	// Assign result sign
	assign PSgn = (MaxAB ? Sb : Sa) ;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// Determine normalization shift amount by finding leading nought
	assign Shift =  ( 
		Sum[32] ? 5'b00000 :	 
		Sum[31] ? 5'b00001 : 
		Sum[30] ? 5'b00010 : 
		Sum[29] ? 5'b00011 : 
		Sum[28] ? 5'b00100 : 
		Sum[27] ? 5'b00101 : 
		Sum[26] ? 5'b00110 : 
		Sum[25] ? 5'b00111 :
		Sum[24] ? 5'b01000 :
		Sum[23] ? 5'b01001 :
		Sum[22] ? 5'b01010 :
		Sum[21] ? 5'b01011 :
		Sum[20] ? 5'b01100 :
		Sum[19] ? 5'b01101 :
		Sum[18] ? 5'b01110 :
		Sum[17] ? 5'b01111 :
		Sum[16] ? 5'b10000 :
		Sum[15] ? 5'b10001 :
		Sum[14] ? 5'b10010 :
		Sum[13] ? 5'b10011 :
		Sum[12] ? 5'b10100 :
		Sum[11] ? 5'b10101 :
		Sum[10] ? 5'b10110 :
		Sum[9] ? 5'b10111 :
		Sum[8] ? 5'b11000 :
		Sum[7] ? 5'b11001 : 5'b11010
	);
	
	reg	  [32:0]		Lvl1;
	
	always @(*) begin
		// Rotate by 16?
		Lvl1 <= Shift[4] ? {Sum[16:0], 16'b0000000000000000} : Sum; 
	end
	
	// Assign outputs
	assign SumS_5 = Lvl1;	

endmodule

module FPAddSub_c_dspchain(
		SumS_5,
		Shift,
		CExp,
		NormM,
		NormE,
		ZeroSum,
		NegE,
		R,
		S,
		FG
	);
	
	// Input ports
	input [32:0] SumS_5 ;						// Smaller mantissa after 16|12|8|4 shift
	
	input [4:0] Shift ;						// Shift amount
	
// Input ports
	
	input [7:0] CExp ;
	

	// Output ports
	output [22:0] NormM ;				// Normalized mantissa
	output [8:0] NormE ;					// Adjusted exponent
	output ZeroSum ;						// Zero flag
	output NegE ;							// Flag indicating negative exponent
	output R ;								// Round bit
	output S ;								// Final sticky bit
	output FG ;


	wire [3:0]Shift_1;
	assign Shift_1 = Shift [3:0];
	// Output ports
	wire [32:0] SumS_7 ;						// The smaller mantissa
	
	reg	  [32:0]		Lvl2;
	wire    [65:0]    Stage1;	
	reg	  [32:0]		Lvl3;
	wire    [65:0]    Stage2;	
	integer           i;               	// Loop variable
	
	assign Stage1 = {SumS_5, SumS_5};

	always @(*) begin    					// Rotate {0 | 4 | 8 | 12} bits
	  case (Shift[3:2])
			// Rotate by 0
			2'b00: Lvl2 <= Stage1[32:0];       		
			// Rotate by 4
			2'b01: begin for (i=65; i>=33; i=i-1) begin Lvl2[i-33] <= Stage1[i-4]; end Lvl2[3:0] <= 0; end
			// Rotate by 8
			2'b10: begin for (i=65; i>=33; i=i-1) begin Lvl2[i-33] <= Stage1[i-8]; end Lvl2[7:0] <= 0; end
			// Rotate by 12
			2'b11: begin for (i=65; i>=33; i=i-1) begin Lvl2[i-33] <= Stage1[i-12]; end Lvl2[11:0] <= 0; end
	  endcase
	end
	
	assign Stage2 = {Lvl2, Lvl2};

	always @(*) begin   				 		// Rotate {0 | 1 | 2 | 3} bits
	  case (Shift_1[1:0])
			// Rotate by 0
			2'b00:  Lvl3 <= Stage2[32:0];
			// Rotate by 1
			2'b01: begin for (i=65; i>=33; i=i-1) begin Lvl3[i-33] <= Stage2[i-1]; end Lvl3[0] <= 0; end 
			// Rotate by 2
			2'b10: begin for (i=65; i>=33; i=i-1) begin Lvl3[i-33] <= Stage2[i-2]; end Lvl3[1:0] <= 0; end
			// Rotate by 3
			2'b11: begin for (i=65; i>=33; i=i-1) begin Lvl3[i-33] <= Stage2[i-3]; end Lvl3[2:0] <= 0; end
	  endcase
	end
	
	// Assign outputs
	assign SumS_7 = Lvl3;						// Take out smaller mantissa



	
	// Internal signals
	wire MSBShift ;						// Flag indicating that a second shift is needed
	wire [8:0] ExpOF ;					// MSB set in sum indicates overflow
	wire [8:0] ExpOK ;					// MSB not set, no adjustment
	
	// Calculate normalized exponent and mantissa, check for all-zero sum
	assign MSBShift = SumS_7[32] ;		// Check MSB in unnormalized sum
	assign ZeroSum = ~|SumS_7 ;			// Check for all zero sum
	assign ExpOK = CExp - Shift ;		// Adjust exponent for new normalized mantissa
	assign NegE = ExpOK[8] ;			// Check for exponent overflow
	assign ExpOF = CExp - Shift + 1'b1 ;		// If MSB set, add one to exponent(x2)
	assign NormE = MSBShift ? ExpOF : ExpOK ;			// Check for exponent overflow
	assign NormM = SumS_7[31:9] ;		// The new, normalized mantissa
	
	// Also need to compute sticky and round bits for the rounding stage
	assign FG = SumS_7[8] ; 
	assign R = SumS_7[7] ;
	assign S = |SumS_7[6:0] ;		
	
endmodule

module FPAddSub_d_dspchain(
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
		NegE,
		InputExc,
		P,
		Flags 
    );

	// Input ports
	input ZeroSum ;					// Sum is zero
	input [8:0] NormE ;				// Normalized exponent
	input [22:0] NormM ;				// Normalized mantissa
	input R ;							// Round bit
	input S ;							// Sticky bit
	input G ;
	input Sa ;							// A's sign bit
	input Sb ;							// B's sign bit
	input Ctrl ;						// Control bit (operation)
	input MaxAB ;
	

	input NegE ;						// Negative exponent?
	input [4:0] InputExc ;					// Exceptions in inputs A and B

	// Output ports
	output [31:0] P ;					// Final result
	output [4:0] Flags ;				// Exception flags
	
	// 
	wire [31:0] Z ;					// Final result
	wire EOF ;
	
	// Internal signals
	wire [23:0] RoundUpM ;			// Rounded up sum with room for overflow
	wire [22:0] RoundM ;				// The final rounded sum
	wire [8:0] RoundE ;				// Rounded exponent (note extra bit due to poential overflow	)
	wire RoundUp ;						// Flag indicating that the sum should be rounded up
	wire ExpAdd ;						// May have to add 1 to compensate for overflow 
	wire RoundOF ;						// Rounding overflow
	wire FSgn;
	// The cases where we need to round upwards (= adding one) in Round to nearest, tie to even
	assign RoundUp = (G & ((R | S) | NormM[0])) ;
	
	// Note that in the other cases (rounding down), the sum is already 'rounded'
	assign RoundUpM = (NormM + 1) ;								// The sum, rounded up by 1
	assign RoundM = (RoundUp ? RoundUpM[22:0] : NormM) ; 	// Compute final mantissa	
	assign RoundOF = RoundUp & RoundUpM[23] ; 				// Check for overflow when rounding up

	// Calculate post-rounding exponent
	assign ExpAdd = (RoundOF ? 1'b1 : 1'b0) ; 				// Add 1 to exponent to compensate for overflow
	assign RoundE = ZeroSum ? 8'b00000000 : (NormE + ExpAdd) ; 							// Final exponent

	// If zero, need to determine sign according to rounding
	assign FSgn = (ZeroSum & (Sa ^ Sb)) | (ZeroSum ? (Sa & Sb & ~Ctrl) : ((~MaxAB & Sa) | ((Ctrl ^ Sb) & (MaxAB | Sa)))) ;

	// Assign final result
	assign Z = {FSgn, RoundE[7:0], RoundM[22:0]} ;
	
	// Indicate exponent overflow
	assign EOF = RoundE[8];

/////////////////////////////////////////////////////////////////////////////////////////////////////////



	
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
	assign DivideByZero = &(Z[30:23]) & ~|(Z[30:23]) & ~InputExc[1] & ~InputExc[0];
	
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


module dpram_2048_40bit (
    clk,
    address_a,
    address_b,
    wren_a,
    wren_b,
    data_a,
    data_b,
    out_a,
    out_b
);
parameter AWIDTH=11;
parameter NUM_WORDS=2048;
parameter DWIDTH=40;
input clk;
input [(AWIDTH-1):0] address_a;
input [(AWIDTH-1):0] address_b;
input  wren_a;
input  wren_b;
input [(DWIDTH-1):0] data_a;
input [(DWIDTH-1):0] data_b;
output reg [(DWIDTH-1):0] out_a;
output reg [(DWIDTH-1):0] out_b;

`ifndef hard_mem

reg [DWIDTH-1:0] ram[NUM_WORDS-1:0];
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

defparam u_dual_port_ram.ADDR_WIDTH = AWIDTH;
defparam u_dual_port_ram.DATA_WIDTH = DWIDTH;

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

`timescale 1ns / 1ps
//`define complex_dsp
`define DWIDTH 8
`define AWIDTH 11
`define MEM_SIZE 2048

`define MAT_MUL_SIZE 4
`define MASK_WIDTH 4
`define LOG2_MAT_MUL_SIZE 2

`define BB_MAT_MUL_SIZE `MAT_MUL_SIZE
`define NUM_CYCLES_IN_MAC 3
`define MEM_ACCESS_LATENCY 1
`define REG_DATAWIDTH 32
`define REG_ADDRWIDTH 8
`define ADDR_STRIDE_WIDTH 8
`define MAX_BITS_POOL 3
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 04/10/2020 11:43:24 PM
// Design Name: 
// Module Name: matmul_4x4
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////

module matmul_4x4_systolic(
 clk,
 reset,
 pe_reset,
 start_mat_mul,
 done_mat_mul,
 address_mat_a,
 address_mat_b,
 address_mat_c,
 address_stride_a,
 address_stride_b,
 address_stride_c,
 a_data,
 b_data,
 a_data_in, //Data values coming in from previous matmul - systolic connections
 b_data_in,
 c_data_in, //Data values coming in from previous matmul - systolic shifting
 c_data_out, //Data values going out to next matmul - systolic shifting
 a_data_out,
 b_data_out,
 a_addr,
 b_addr,
 c_addr,
 c_data_available,
 validity_mask_a_rows,
 validity_mask_a_cols_b_rows,
 validity_mask_b_cols,
 final_mat_mul_size,
 a_loc,
 b_loc
);

 input clk;
 input reset;
 input pe_reset;
 input start_mat_mul;
 output done_mat_mul;
 input [`AWIDTH-1:0] address_mat_a;
 input [`AWIDTH-1:0] address_mat_b;
 input [`AWIDTH-1:0] address_mat_c;
 input [`ADDR_STRIDE_WIDTH-1:0] address_stride_a;
 input [`ADDR_STRIDE_WIDTH-1:0] address_stride_b;
 input [`ADDR_STRIDE_WIDTH-1:0] address_stride_c;
 input [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data;
 input [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data;
 input [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_in;
 input [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_in;
 input [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_in;
 output [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_out;
 output [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_out;
 output [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_out;
 output [`AWIDTH-1:0] a_addr;
 output [`AWIDTH-1:0] b_addr;
 output [`AWIDTH-1:0] c_addr;
 output c_data_available;
 input [`MASK_WIDTH-1:0] validity_mask_a_rows;
 input [`MASK_WIDTH-1:0] validity_mask_a_cols_b_rows;
 input [`MASK_WIDTH-1:0] validity_mask_b_cols;
//7:0 is okay here. We aren't going to make a matmul larger than 128x128
//In fact, these will get optimized out by the synthesis tool, because
//we hardcode them at the instantiation level.
 input [7:0] final_mat_mul_size;
 input [7:0] a_loc;
 input [7:0] b_loc;

//////////////////////////////////////////////////////////////////////////
// Logic for clock counting and when to assert done
//////////////////////////////////////////////////////////////////////////

reg done_mat_mul;
//This is 7 bits because the expectation is that clock count will be pretty
//small. For large matmuls, this will need to increased to have more bits.
//In general, a systolic multiplier takes 4*N-2+P cycles, where N is the size 
//of the matmul and P is the number of pipleine stages in the MAC block.
reg [7:0] clk_cnt;

//Finding out number of cycles to assert matmul done.
//When we have to save the outputs to accumulators, then we don't need to
//shift out data. So, we can assert done_mat_mul early.
//In the normal case, we have to include the time to shift out the results. 
//Note: the count expression used to contain "4*final_mat_mul_size", but 
//to avoid multiplication, we now use "final_mat_mul_size<<2"
wire [7:0] clk_cnt_for_done;
assign clk_cnt_for_done = 
                          ((final_mat_mul_size<<2) - 2 + `NUM_CYCLES_IN_MAC) ;  

always @(posedge clk) begin
  if (reset || ~start_mat_mul) begin
    clk_cnt <= 0;
    done_mat_mul <= 0;
  end
  else if (clk_cnt == clk_cnt_for_done) begin
    done_mat_mul <= 1;
    clk_cnt <= clk_cnt + 1;
  end
  else if (done_mat_mul == 0) begin
    clk_cnt <= clk_cnt + 1;
  end    
  else begin
    done_mat_mul <= 0;
    clk_cnt <= clk_cnt + 1;
  end
end


wire [`DWIDTH-1:0] a0_data;
wire [`DWIDTH-1:0] a1_data;
wire [`DWIDTH-1:0] a2_data;
wire [`DWIDTH-1:0] a3_data;
wire [`DWIDTH-1:0] b0_data;
wire [`DWIDTH-1:0] b1_data;
wire [`DWIDTH-1:0] b2_data;
wire [`DWIDTH-1:0] b3_data;
wire [`DWIDTH-1:0] a1_data_delayed_1;
wire [`DWIDTH-1:0] a2_data_delayed_1;
wire [`DWIDTH-1:0] a2_data_delayed_2;
wire [`DWIDTH-1:0] a3_data_delayed_1;
wire [`DWIDTH-1:0] a3_data_delayed_2;
wire [`DWIDTH-1:0] a3_data_delayed_3;
wire [`DWIDTH-1:0] b1_data_delayed_1;
wire [`DWIDTH-1:0] b2_data_delayed_1;
wire [`DWIDTH-1:0] b2_data_delayed_2;
wire [`DWIDTH-1:0] b3_data_delayed_1;
wire [`DWIDTH-1:0] b3_data_delayed_2;
wire [`DWIDTH-1:0] b3_data_delayed_3;

//////////////////////////////////////////////////////////////////////////
// Instantiation of systolic data setup
//////////////////////////////////////////////////////////////////////////
systolic_data_setup u_systolic_data_setup(
.clk(clk),
.reset(reset),
.start_mat_mul(start_mat_mul),
.a_addr(a_addr),
.b_addr(b_addr),
.address_mat_a(address_mat_a),
.address_mat_b(address_mat_b),
.address_stride_a(address_stride_a),
.address_stride_b(address_stride_b),
.a_data(a_data),
.b_data(b_data),
.clk_cnt(clk_cnt),
.a0_data(a0_data),
.a1_data_delayed_1(a1_data_delayed_1),
.a2_data_delayed_2(a2_data_delayed_2),
.a3_data_delayed_3(a3_data_delayed_3),
.b0_data(b0_data),
.b1_data_delayed_1(b1_data_delayed_1),
.b2_data_delayed_2(b2_data_delayed_2),
.b3_data_delayed_3(b3_data_delayed_3),
.validity_mask_a_rows(validity_mask_a_rows),
.validity_mask_a_cols_b_rows(validity_mask_a_cols_b_rows),
.validity_mask_b_cols(validity_mask_b_cols),
.final_mat_mul_size(final_mat_mul_size),
.a_loc(a_loc),
.b_loc(b_loc)
);


//////////////////////////////////////////////////////////////////////////
// Logic to mux data_in coming from neighboring matmuls
//////////////////////////////////////////////////////////////////////////
wire [`DWIDTH-1:0] a0;
wire [`DWIDTH-1:0] a1;
wire [`DWIDTH-1:0] a2;
wire [`DWIDTH-1:0] a3;
wire [`DWIDTH-1:0] b0;
wire [`DWIDTH-1:0] b1;
wire [`DWIDTH-1:0] b2;
wire [`DWIDTH-1:0] b3;

wire [`DWIDTH-1:0] a0_data_in;
wire [`DWIDTH-1:0] a1_data_in;
wire [`DWIDTH-1:0] a2_data_in;
wire [`DWIDTH-1:0] a3_data_in;
assign a0_data_in = a_data_in[`DWIDTH-1:0];
assign a1_data_in = a_data_in[2*`DWIDTH-1:`DWIDTH];
assign a2_data_in = a_data_in[3*`DWIDTH-1:2*`DWIDTH];
assign a3_data_in = a_data_in[4*`DWIDTH-1:3*`DWIDTH];

wire [`DWIDTH-1:0] b0_data_in;
wire [`DWIDTH-1:0] b1_data_in;
wire [`DWIDTH-1:0] b2_data_in;
wire [`DWIDTH-1:0] b3_data_in;
assign b0_data_in = b_data_in[`DWIDTH-1:0];
assign b1_data_in = b_data_in[2*`DWIDTH-1:`DWIDTH];
assign b2_data_in = b_data_in[3*`DWIDTH-1:2*`DWIDTH];
assign b3_data_in = b_data_in[4*`DWIDTH-1:3*`DWIDTH];

//If b_loc is 0, that means this matmul block is on the top-row of the
//final large matmul. In that case, b will take inputs from mem.
//If b_loc != 0, that means this matmul block is not on the top-row of the
//final large matmul. In that case, b will take inputs from the matmul on top
//of this one.
assign a0 = (b_loc==0) ? a0_data           : a0_data_in;
assign a1 = (b_loc==0) ? a1_data_delayed_1 : a1_data_in;
assign a2 = (b_loc==0) ? a2_data_delayed_2 : a2_data_in;
assign a3 = (b_loc==0) ? a3_data_delayed_3 : a3_data_in;

//If a_loc is 0, that means this matmul block is on the left-col of the
//final large matmul. In that case, a will take inputs from mem.
//If a_loc != 0, that means this matmul block is not on the left-col of the
//final large matmul. In that case, a will take inputs from the matmul on left
//of this one.
assign b0 = (a_loc==0) ? b0_data           : b0_data_in;
assign b1 = (a_loc==0) ? b1_data_delayed_1 : b1_data_in;
assign b2 = (a_loc==0) ? b2_data_delayed_2 : b2_data_in;
assign b3 = (a_loc==0) ? b3_data_delayed_3 : b3_data_in;


wire [`DWIDTH-1:0] matrixC00;
wire [`DWIDTH-1:0] matrixC01;
wire [`DWIDTH-1:0] matrixC02;
wire [`DWIDTH-1:0] matrixC03;
wire [`DWIDTH-1:0] matrixC10;
wire [`DWIDTH-1:0] matrixC11;
wire [`DWIDTH-1:0] matrixC12;
wire [`DWIDTH-1:0] matrixC13;
wire [`DWIDTH-1:0] matrixC20;
wire [`DWIDTH-1:0] matrixC21;
wire [`DWIDTH-1:0] matrixC22;
wire [`DWIDTH-1:0] matrixC23;
wire [`DWIDTH-1:0] matrixC30;
wire [`DWIDTH-1:0] matrixC31;
wire [`DWIDTH-1:0] matrixC32;
wire [`DWIDTH-1:0] matrixC33;


//////////////////////////////////////////////////////////////////////////
// Instantiation of the output logic
//////////////////////////////////////////////////////////////////////////
output_logic_systolic_4x4 u_output_logic_systolic_4x4(
.clk(clk),
.reset(reset),
.start_mat_mul(start_mat_mul),
.done_mat_mul(done_mat_mul),
.address_mat_c(address_mat_c),
.address_stride_c(address_stride_c),
.c_data_out(c_data_out),
.c_data_in(c_data_in),
.c_addr(c_addr),
.c_data_available(c_data_available),
.clk_cnt(clk_cnt),
.final_mat_mul_size(final_mat_mul_size),
.matrixC00(matrixC00),
.matrixC01(matrixC01),
.matrixC02(matrixC02),
.matrixC03(matrixC03),
.matrixC10(matrixC10),
.matrixC11(matrixC11),
.matrixC12(matrixC12),
.matrixC13(matrixC13),
.matrixC20(matrixC20),
.matrixC21(matrixC21),
.matrixC22(matrixC22),
.matrixC23(matrixC23),
.matrixC30(matrixC30),
.matrixC31(matrixC31),
.matrixC32(matrixC32),
.matrixC33(matrixC33)
);

//////////////////////////////////////////////////////////////////////////
// Instantiations of the actual PEs
//////////////////////////////////////////////////////////////////////////
systolic_pe_matrix u_systolic_pe_matrix(
.reset(reset),
.clk(clk),
.pe_reset(pe_reset),
.start_mat_mul(start_mat_mul),
.a0(a0), 
.a1(a1), 
.a2(a2), 
.a3(a3),
.b0(b0), 
.b1(b1), 
.b2(b2), 
.b3(b3),
.matrixC00(matrixC00),
.matrixC01(matrixC01),
.matrixC02(matrixC02),
.matrixC03(matrixC03),
.matrixC10(matrixC10),
.matrixC11(matrixC11),
.matrixC12(matrixC12),
.matrixC13(matrixC13),
.matrixC20(matrixC20),
.matrixC21(matrixC21),
.matrixC22(matrixC22),
.matrixC23(matrixC23),
.matrixC30(matrixC30),
.matrixC31(matrixC31),
.matrixC32(matrixC32),
.matrixC33(matrixC33),
.a_data_out(a_data_out),
.b_data_out(b_data_out)
);

endmodule

//////////////////////////////////////////////////////////////////////////
// Output logic
//////////////////////////////////////////////////////////////////////////
module output_logic_systolic_4x4(
clk,
reset,
start_mat_mul,
done_mat_mul,
address_mat_c,
address_stride_c,
c_data_in,
c_data_out, //Data values going out to next matmul - systolic shifting
c_addr,
c_data_available,
clk_cnt,
final_mat_mul_size,
matrixC00,
matrixC01,
matrixC02,
matrixC03,
matrixC10,
matrixC11,
matrixC12,
matrixC13,
matrixC20,
matrixC21,
matrixC22,
matrixC23,
matrixC30,
matrixC31,
matrixC32,
matrixC33
);

input clk;
input reset;
input start_mat_mul;
input done_mat_mul;
input [`AWIDTH-1:0] address_mat_c;
input [`ADDR_STRIDE_WIDTH-1:0] address_stride_c;
input [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_in;
output [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_out;
output [`AWIDTH-1:0] c_addr;
output c_data_available;
input [7:0] clk_cnt;
//output row_latch_en;
input [7:0] final_mat_mul_size;
input [`DWIDTH-1:0] matrixC00;
input [`DWIDTH-1:0] matrixC01;
input [`DWIDTH-1:0] matrixC02;
input [`DWIDTH-1:0] matrixC03;
input [`DWIDTH-1:0] matrixC10;
input [`DWIDTH-1:0] matrixC11;
input [`DWIDTH-1:0] matrixC12;
input [`DWIDTH-1:0] matrixC13;
input [`DWIDTH-1:0] matrixC20;
input [`DWIDTH-1:0] matrixC21;
input [`DWIDTH-1:0] matrixC22;
input [`DWIDTH-1:0] matrixC23;
input [`DWIDTH-1:0] matrixC30;
input [`DWIDTH-1:0] matrixC31;
input [`DWIDTH-1:0] matrixC32;
input [`DWIDTH-1:0] matrixC33;

wire row_latch_en;

//////////////////////////////////////////////////////////////////////////
// Logic to capture matrix C data from the PEs and shift it out
//////////////////////////////////////////////////////////////////////////

//assign row_latch_en = (clk_cnt==(`MAT_MUL_SIZE + (a_loc+b_loc) * `BB_MAT_MUL_SIZE + 10 +  `NUM_CYCLES_IN_MAC - 1));
//Writing the line above to avoid multiplication:
//assign row_latch_en = (clk_cnt==(`MAT_MUL_SIZE + ((a_loc+b_loc) << `LOG2_MAT_MUL_SIZE) + 10 +  `NUM_CYCLES_IN_MAC - 1));
//Fixing bug. The line above is inaccurate. Using the line below. 
//TODO: This line needs to be fixed to include a_loc and b_loc ie. when final_mat_mul_size is different from `MAT_MUL_SIZE
assign row_latch_en =  
                       //((clk_cnt == ((`MAT_MUL_SIZE<<2) - `MAT_MUL_SIZE -2 +`NUM_CYCLES_IN_MAC)));
                       ((clk_cnt == ((final_mat_mul_size<<2) - final_mat_mul_size -1 +`NUM_CYCLES_IN_MAC)));

reg c_data_available;
reg [`AWIDTH-1:0] c_addr;
reg start_capturing_c_data;
integer counter;
reg [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_out;
reg [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_out_1;
reg [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_out_2;
reg [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_out_3;

wire [`MAT_MUL_SIZE*`DWIDTH-1:0] col0;
wire [`MAT_MUL_SIZE*`DWIDTH-1:0] col1;
wire [`MAT_MUL_SIZE*`DWIDTH-1:0] col2;
wire [`MAT_MUL_SIZE*`DWIDTH-1:0] col3;
assign col0 = {matrixC30, matrixC20, matrixC10, matrixC00};
assign col1 = {matrixC31, matrixC21, matrixC11, matrixC01};
assign col2 = {matrixC32, matrixC22, matrixC12, matrixC02};
assign col3 = {matrixC33, matrixC23, matrixC13, matrixC03};

//If save_output_to_accum is asserted, that means we are not intending to shift
//out the outputs, because the outputs are still partial sums. 
wire condition_to_start_shifting_output;
assign condition_to_start_shifting_output = 
                          row_latch_en ;  

//For larger matmuls, this logic will have more entries in the case statement
always @(posedge clk) begin
  if (reset | ~start_mat_mul) begin
    start_capturing_c_data <= 1'b0;
    c_data_available <= 1'b0;
    c_addr <= address_mat_c+address_stride_c;
    c_data_out <= 0;
    counter <= 0;
    c_data_out_1 <= 0; 
    c_data_out_2 <= 0; 
    c_data_out_3 <= 0; 
  end
  else if (condition_to_start_shifting_output) begin
    start_capturing_c_data <= 1'b1;
    c_data_available <= 1'b1;
    c_addr <= c_addr - address_stride_c;
    c_data_out <= col0; 
    c_data_out_1 <= col1; 
    c_data_out_2 <= col2; 
    c_data_out_3 <= col3; 
    counter <= counter + 1;
  end 
  else if (done_mat_mul) begin
    start_capturing_c_data <= 1'b0;
    c_data_available <= 1'b0;
    c_addr <= address_mat_c+address_stride_c;
    c_data_out <= 0;
    c_data_out_1 <= 0;
    c_data_out_2 <= 0;
    c_data_out_3 <= 0;
  end 
  else if (counter >= `MAT_MUL_SIZE) begin
    c_addr <= c_addr - address_stride_c;
    c_data_out <= c_data_out_1;
    c_data_out_1 <= c_data_out_2;
    c_data_out_2 <= c_data_out_3;
    c_data_out_3 <= c_data_in;
  end
  else if (start_capturing_c_data) begin
    c_data_available <= 1'b1;
    c_addr <= c_addr - address_stride_c;
    counter <= counter + 1;
    c_data_out <= c_data_out_1;
    c_data_out_1 <= c_data_out_2;
    c_data_out_2 <= c_data_out_3;
    c_data_out_3 <= c_data_in;
  end
end

endmodule

//////////////////////////////////////////////////////////////////////////
// Systolic data setup
//////////////////////////////////////////////////////////////////////////
module systolic_data_setup(
clk,
reset,
start_mat_mul,
a_addr,
b_addr,
address_mat_a,
address_mat_b,
address_stride_a,
address_stride_b,
a_data,
b_data,
clk_cnt,
a0_data,
a1_data_delayed_1,
a2_data_delayed_2,
a3_data_delayed_3,
b0_data,
b1_data_delayed_1,
b2_data_delayed_2,
b3_data_delayed_3,
validity_mask_a_rows,
validity_mask_a_cols_b_rows,
validity_mask_b_cols,
final_mat_mul_size,
a_loc,
b_loc
);

input clk;
input reset;
input start_mat_mul;
output [`AWIDTH-1:0] a_addr;
output [`AWIDTH-1:0] b_addr;
input [`AWIDTH-1:0] address_mat_a;
input [`AWIDTH-1:0] address_mat_b;
input [`ADDR_STRIDE_WIDTH-1:0] address_stride_a;
input [`ADDR_STRIDE_WIDTH-1:0] address_stride_b;
input [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data;
input [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data;
input [7:0] clk_cnt;
output [`DWIDTH-1:0] a0_data;
output [`DWIDTH-1:0] a1_data_delayed_1;
output [`DWIDTH-1:0] a2_data_delayed_2;
output [`DWIDTH-1:0] a3_data_delayed_3;
output [`DWIDTH-1:0] b0_data;
output [`DWIDTH-1:0] b1_data_delayed_1;
output [`DWIDTH-1:0] b2_data_delayed_2;
output [`DWIDTH-1:0] b3_data_delayed_3;
input [`MASK_WIDTH-1:0] validity_mask_a_rows;
input [`MASK_WIDTH-1:0] validity_mask_a_cols_b_rows;
input [`MASK_WIDTH-1:0] validity_mask_b_cols;
input [7:0] final_mat_mul_size;
input [7:0] a_loc;
input [7:0] b_loc;

wire [`DWIDTH-1:0] a0_data;
wire [`DWIDTH-1:0] a1_data;
wire [`DWIDTH-1:0] a2_data;
wire [`DWIDTH-1:0] a3_data;
wire [`DWIDTH-1:0] b0_data;
wire [`DWIDTH-1:0] b1_data;
wire [`DWIDTH-1:0] b2_data;
wire [`DWIDTH-1:0] b3_data;

//////////////////////////////////////////////////////////////////////////
// Logic to generate addresses to BRAM A
//////////////////////////////////////////////////////////////////////////
reg [`AWIDTH-1:0] a_addr;
reg a_mem_access; //flag that tells whether the matmul is trying to access memory or not

always @(posedge clk) begin
  //else if (clk_cnt >= a_loc*`MAT_MUL_SIZE+final_mat_mul_size) begin
  //Writing the line above to avoid multiplication:
  if ((reset || ~start_mat_mul) || (clk_cnt >= (a_loc<<`LOG2_MAT_MUL_SIZE)+final_mat_mul_size)) begin
      a_addr <= address_mat_a-address_stride_a;
    a_mem_access <= 0;
  end

  //else if ((clk_cnt >= a_loc*`MAT_MUL_SIZE) && (clk_cnt < a_loc*`MAT_MUL_SIZE+final_mat_mul_size)) begin
  //Writing the line above to avoid multiplication:
  else if ((clk_cnt >= (a_loc<<`LOG2_MAT_MUL_SIZE)) && (clk_cnt < (a_loc<<`LOG2_MAT_MUL_SIZE)+final_mat_mul_size)) begin
      a_addr <= a_addr + address_stride_a;
    a_mem_access <= 1;
  end
end  

//////////////////////////////////////////////////////////////////////////
// Logic to generate valid signals for data coming from BRAM A
//////////////////////////////////////////////////////////////////////////
reg [7:0] a_mem_access_counter;
always @(posedge clk) begin
  if (reset || ~start_mat_mul) begin
    a_mem_access_counter <= 0;
  end
  else if (a_mem_access == 1) begin
    a_mem_access_counter <= a_mem_access_counter + 1;  

  end
  else begin
    a_mem_access_counter <= 0;
  end
end

wire a_data_valid; //flag that tells whether the data from memory is valid
assign a_data_valid = 
       ((validity_mask_a_cols_b_rows[0]==1'b0 && a_mem_access_counter==1) ||
        (validity_mask_a_cols_b_rows[1]==1'b0 && a_mem_access_counter==2) ||
        (validity_mask_a_cols_b_rows[2]==1'b0 && a_mem_access_counter==3) ||
        (validity_mask_a_cols_b_rows[3]==1'b0 && a_mem_access_counter==4)) ?
        1'b0 : (a_mem_access_counter >= `MEM_ACCESS_LATENCY);

//////////////////////////////////////////////////////////////////////////
// Logic to delay certain parts of the data received from BRAM A (systolic data setup)
//////////////////////////////////////////////////////////////////////////
//Slice data into chunks and qualify it with whether it is valid or not
assign a0_data = a_data[`DWIDTH-1:0] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[0]}};
assign a1_data = a_data[2*`DWIDTH-1:`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[1]}};
assign a2_data = a_data[3*`DWIDTH-1:2*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[2]}};
assign a3_data = a_data[4*`DWIDTH-1:3*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[3]}};

//For larger matmuls, more such delaying flops will be needed
reg [`DWIDTH-1:0] a1_data_delayed_1;
reg [`DWIDTH-1:0] a2_data_delayed_1;
reg [`DWIDTH-1:0] a2_data_delayed_2;
reg [`DWIDTH-1:0] a3_data_delayed_1;
reg [`DWIDTH-1:0] a3_data_delayed_2;
reg [`DWIDTH-1:0] a3_data_delayed_3;
always @(posedge clk) begin
  if (reset || ~start_mat_mul || clk_cnt==0) begin
    a1_data_delayed_1 <= 0;
    a2_data_delayed_1 <= 0;
    a2_data_delayed_2 <= 0;
    a3_data_delayed_1 <= 0;
    a3_data_delayed_2 <= 0;
    a3_data_delayed_3 <= 0;
  end
  else begin
    a1_data_delayed_1 <= a1_data;
    a2_data_delayed_1 <= a2_data;
    a2_data_delayed_2 <= a2_data_delayed_1;
    a3_data_delayed_1 <= a3_data;
    a3_data_delayed_2 <= a3_data_delayed_1;
    a3_data_delayed_3 <= a3_data_delayed_2;
  end
end

//////////////////////////////////////////////////////////////////////////
// Logic to generate addresses to BRAM B
//////////////////////////////////////////////////////////////////////////
reg [`AWIDTH-1:0] b_addr;
reg b_mem_access; //flag that tells whether the matmul is trying to access memory or not

always @(posedge clk) begin
  //else if (clk_cnt >= b_loc*`MAT_MUL_SIZE+final_mat_mul_size) begin
  //Writing the line above to avoid multiplication:
  if ((reset || ~start_mat_mul) || (clk_cnt >= (b_loc<<`LOG2_MAT_MUL_SIZE)+final_mat_mul_size)) begin
      b_addr <= address_mat_b - address_stride_b;
    b_mem_access <= 0;
  end
  //else if ((clk_cnt >= b_loc*`MAT_MUL_SIZE) && (clk_cnt < b_loc*`MAT_MUL_SIZE+final_mat_mul_size)) begin
  //Writing the line above to avoid multiplication:
  else if ((clk_cnt >= (b_loc<<`LOG2_MAT_MUL_SIZE)) && (clk_cnt < (b_loc<<`LOG2_MAT_MUL_SIZE)+final_mat_mul_size)) begin
      b_addr <= b_addr + address_stride_b;
    b_mem_access <= 1;
  end
end  

//////////////////////////////////////////////////////////////////////////
// Logic to generate valid signals for data coming from BRAM B
//////////////////////////////////////////////////////////////////////////
reg [7:0] b_mem_access_counter;
always @(posedge clk) begin
  if (reset || ~start_mat_mul) begin
    b_mem_access_counter <= 0;
  end
  else if (b_mem_access == 1) begin
    b_mem_access_counter <= b_mem_access_counter + 1;  
  end
  else begin
    b_mem_access_counter <= 0;
  end
end

wire b_data_valid; //flag that tells whether the data from memory is valid
assign b_data_valid = 
       ((validity_mask_a_cols_b_rows[0]==1'b0 && b_mem_access_counter==1) ||
        (validity_mask_a_cols_b_rows[1]==1'b0 && b_mem_access_counter==2) ||
        (validity_mask_a_cols_b_rows[2]==1'b0 && b_mem_access_counter==3) ||
        (validity_mask_a_cols_b_rows[3]==1'b0 && b_mem_access_counter==4)) ?
        1'b0 : (b_mem_access_counter >= `MEM_ACCESS_LATENCY);


//////////////////////////////////////////////////////////////////////////
// Logic to delay certain parts of the data received from BRAM B (systolic data setup)
//////////////////////////////////////////////////////////////////////////
//Slice data into chunks and qualify it with whether it is valid or not
assign b0_data = b_data[`DWIDTH-1:0] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[0]}};
assign b1_data = b_data[2*`DWIDTH-1:`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[1]}};
assign b2_data = b_data[3*`DWIDTH-1:2*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[2]}};
assign b3_data = b_data[4*`DWIDTH-1:3*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[3]}};

//For larger matmuls, more such delaying flops will be needed
reg [`DWIDTH-1:0] b1_data_delayed_1;
reg [`DWIDTH-1:0] b2_data_delayed_1;
reg [`DWIDTH-1:0] b2_data_delayed_2;
reg [`DWIDTH-1:0] b3_data_delayed_1;
reg [`DWIDTH-1:0] b3_data_delayed_2;
reg [`DWIDTH-1:0] b3_data_delayed_3;
always @(posedge clk) begin
  if (reset || ~start_mat_mul || clk_cnt==0) begin
    b1_data_delayed_1 <= 0;
    b2_data_delayed_1 <= 0;
    b2_data_delayed_2 <= 0;
    b3_data_delayed_1 <= 0;
    b3_data_delayed_2 <= 0;
    b3_data_delayed_3 <= 0;
  end
  else begin
    b1_data_delayed_1 <= b1_data;
    b2_data_delayed_1 <= b2_data;
    b2_data_delayed_2 <= b2_data_delayed_1;
    b3_data_delayed_1 <= b3_data;
    b3_data_delayed_2 <= b3_data_delayed_1;
    b3_data_delayed_3 <= b3_data_delayed_2;
  end
end


endmodule



//////////////////////////////////////////////////////////////////////////
// Systolically connected PEs
//////////////////////////////////////////////////////////////////////////
module systolic_pe_matrix(
reset,
clk,
pe_reset,
start_mat_mul,
a0, a1, a2, a3,
b0, b1, b2, b3,
matrixC00,
matrixC01,
matrixC02,
matrixC03,
matrixC10,
matrixC11,
matrixC12,
matrixC13,
matrixC20,
matrixC21,
matrixC22,
matrixC23,
matrixC30,
matrixC31,
matrixC32,
matrixC33,
a_data_out,
b_data_out
);

input clk;
input reset;
input pe_reset;
input start_mat_mul;
input [`DWIDTH-1:0] a0;
input [`DWIDTH-1:0] a1;
input [`DWIDTH-1:0] a2;
input [`DWIDTH-1:0] a3;
input [`DWIDTH-1:0] b0;
input [`DWIDTH-1:0] b1;
input [`DWIDTH-1:0] b2;
input [`DWIDTH-1:0] b3;
output [`DWIDTH-1:0] matrixC00;
output [`DWIDTH-1:0] matrixC01;
output [`DWIDTH-1:0] matrixC02;
output [`DWIDTH-1:0] matrixC03;
output [`DWIDTH-1:0] matrixC10;
output [`DWIDTH-1:0] matrixC11;
output [`DWIDTH-1:0] matrixC12;
output [`DWIDTH-1:0] matrixC13;
output [`DWIDTH-1:0] matrixC20;
output [`DWIDTH-1:0] matrixC21;
output [`DWIDTH-1:0] matrixC22;
output [`DWIDTH-1:0] matrixC23;
output [`DWIDTH-1:0] matrixC30;
output [`DWIDTH-1:0] matrixC31;
output [`DWIDTH-1:0] matrixC32;
output [`DWIDTH-1:0] matrixC33;
output [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_out;
output [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_out;

wire [`DWIDTH-1:0] a00to01, a01to02, a02to03, a03to04;
wire [`DWIDTH-1:0] a10to11, a11to12, a12to13, a13to14;
wire [`DWIDTH-1:0] a20to21, a21to22, a22to23, a23to24;
wire [`DWIDTH-1:0] a30to31, a31to32, a32to33, a33to34;

wire [`DWIDTH-1:0] b00to10, b10to20, b20to30, b30to40; 
wire [`DWIDTH-1:0] b01to11, b11to21, b21to31, b31to41;
wire [`DWIDTH-1:0] b02to12, b12to22, b22to32, b32to42;
wire [`DWIDTH-1:0] b03to13, b13to23, b23to33, b33to43;

wire effective_rst;
assign effective_rst = reset | pe_reset;

processing_element pe00(.reset(effective_rst), .clk(clk),  .in_a(a0),      .in_b(b0),  .out_a(a00to01), .out_b(b00to10), .out_c(matrixC00));
processing_element pe01(.reset(effective_rst), .clk(clk),  .in_a(a00to01), .in_b(b1),  .out_a(a01to02), .out_b(b01to11), .out_c(matrixC01));
processing_element pe02(.reset(effective_rst), .clk(clk),  .in_a(a01to02), .in_b(b2),  .out_a(a02to03), .out_b(b02to12), .out_c(matrixC02));
processing_element pe03(.reset(effective_rst), .clk(clk),  .in_a(a02to03), .in_b(b3),  .out_a(a03to04), .out_b(b03to13), .out_c(matrixC03));

processing_element pe10(.reset(effective_rst), .clk(clk),  .in_a(a1),      .in_b(b00to10), .out_a(a10to11), .out_b(b10to20), .out_c(matrixC10));
processing_element pe11(.reset(effective_rst), .clk(clk),  .in_a(a10to11), .in_b(b01to11), .out_a(a11to12), .out_b(b11to21), .out_c(matrixC11));
processing_element pe12(.reset(effective_rst), .clk(clk),  .in_a(a11to12), .in_b(b02to12), .out_a(a12to13), .out_b(b12to22), .out_c(matrixC12));
processing_element pe13(.reset(effective_rst), .clk(clk),  .in_a(a12to13), .in_b(b03to13), .out_a(a13to14), .out_b(b13to23), .out_c(matrixC13));

processing_element pe20(.reset(effective_rst), .clk(clk),  .in_a(a2),      .in_b(b10to20), .out_a(a20to21), .out_b(b20to30), .out_c(matrixC20));
processing_element pe21(.reset(effective_rst), .clk(clk),  .in_a(a20to21), .in_b(b11to21), .out_a(a21to22), .out_b(b21to31), .out_c(matrixC21));
processing_element pe22(.reset(effective_rst), .clk(clk),  .in_a(a21to22), .in_b(b12to22), .out_a(a22to23), .out_b(b22to32), .out_c(matrixC22));
processing_element pe23(.reset(effective_rst), .clk(clk),  .in_a(a22to23), .in_b(b13to23), .out_a(a23to24), .out_b(b23to33), .out_c(matrixC23));

processing_element pe30(.reset(effective_rst), .clk(clk),  .in_a(a3),      .in_b(b20to30), .out_a(a30to31), .out_b(b30to40), .out_c(matrixC30));
processing_element pe31(.reset(effective_rst), .clk(clk),  .in_a(a30to31), .in_b(b21to31), .out_a(a31to32), .out_b(b31to41), .out_c(matrixC31));
processing_element pe32(.reset(effective_rst), .clk(clk),  .in_a(a31to32), .in_b(b22to32), .out_a(a32to33), .out_b(b32to42), .out_c(matrixC32));
processing_element pe33(.reset(effective_rst), .clk(clk),  .in_a(a32to33), .in_b(b23to33), .out_a(a33to34), .out_b(b33to43), .out_c(matrixC33));

assign a_data_out = {a33to34,a23to24,a13to14,a03to04};
assign b_data_out = {b33to43,b32to42,b31to41,b30to40};

endmodule


//////////////////////////////////////////////////////////////////////////
// Processing element (PE)
//////////////////////////////////////////////////////////////////////////
module processing_element(
 reset, 
 clk, 
 in_a,
 in_b, 
 out_a, 
 out_b, 
 out_c
 );

 input reset;
 input clk;
 input  [`DWIDTH-1:0] in_a;
 input  [`DWIDTH-1:0] in_b;
 output [`DWIDTH-1:0] out_a;
 output [`DWIDTH-1:0] out_b;
 output [`DWIDTH-1:0] out_c;  //reduced precision

 reg [`DWIDTH-1:0] out_a;
 reg [`DWIDTH-1:0] out_b;
 wire [`DWIDTH-1:0] out_c;

 wire [`DWIDTH-1:0] out_mac;

 assign out_c = out_mac;

 seq_mac u_mac(.a(in_a), .b(in_b), .out(out_mac), .reset(reset), .clk(clk));

 always @(posedge clk)begin
    if(reset) begin
      out_a<=0;
      out_b<=0;
    end
    else begin  
      out_a<=in_a;
      out_b<=in_b;
    end
 end
 
endmodule

//////////////////////////////////////////////////////////////////////////
// Multiply-and-accumulate (MAC) block
//////////////////////////////////////////////////////////////////////////
module seq_mac(a, b, out, reset, clk);
input [`DWIDTH-1:0] a;
input [`DWIDTH-1:0] b;
input reset;
input clk;
output [`DWIDTH-1:0] out;

reg [`DWIDTH-1:0] out;
wire [`DWIDTH-1:0] mul_out;
wire [`DWIDTH-1:0] add_out;

reg [`DWIDTH-1:0] a_flopped;
reg [`DWIDTH-1:0] b_flopped;

wire [2*`DWIDTH-1:0] mul_out_temp;
reg [2*`DWIDTH-1:0] mul_out_temp_reg;

always @(posedge clk) begin
  if (reset) begin
    a_flopped <= 0;
    b_flopped <= 0;
  end else begin
    a_flopped <= a;
    b_flopped <= b;
  end
end

//assign mul_out = a * b;
qmult mult_u1(.i_multiplicand(a_flopped), .i_multiplier(b_flopped), .o_result(mul_out_temp));

always @(posedge clk) begin
  if (reset) begin
    mul_out_temp_reg <= 0;
  end else begin
    mul_out_temp_reg <= mul_out_temp;
  end
end

//down cast the result
assign mul_out = 
    (mul_out_temp_reg[2*`DWIDTH-1] == 0) ?  //positive number
        (
           (|(mul_out_temp_reg[2*`DWIDTH-2 : `DWIDTH-1])) ?  //is any bit from 14:7 is 1, that means overlfow
             {mul_out_temp_reg[2*`DWIDTH-1] , {(`DWIDTH-1){1'b1}}} : //sign bit and then all 1s
             {mul_out_temp_reg[2*`DWIDTH-1] , mul_out_temp_reg[`DWIDTH-2:0]} 
        )
        : //negative number
        (
           (|(mul_out_temp_reg[2*`DWIDTH-2 : `DWIDTH-1])) ?  //is any bit from 14:7 is 0, that means overlfow
             {mul_out_temp_reg[2*`DWIDTH-1] , mul_out_temp_reg[`DWIDTH-2:0]} :
             {mul_out_temp_reg[2*`DWIDTH-1] , {(`DWIDTH-1){1'b0}}} //sign bit and then all 0s
        );


//we just truncate the higher bits of the product
//assign add_out = mul_out + out;
qadd add_u1(.a(out), .b(mul_out), .c(add_out));

always @(posedge clk) begin
  if (reset) begin
    out <= 0;
  end else begin
    out <= add_out;
  end
end

endmodule


//////////////////////////////////////////////////////////////////////////
// Multiplier
//////////////////////////////////////////////////////////////////////////
module qmult(i_multiplicand,i_multiplier,o_result);
input [`DWIDTH-1:0] i_multiplicand;
input [`DWIDTH-1:0] i_multiplier;
output [2*`DWIDTH-1:0] o_result;

assign o_result = i_multiplicand * i_multiplier;
//DW02_mult #(`DWIDTH,`DWIDTH) u_mult(.A(i_multiplicand), .B(i_multiplier), .TC(1'b1), .PRODUCT(o_result));

endmodule


//////////////////////////////////////////////////////////////////////////
// Adder
//////////////////////////////////////////////////////////////////////////
module qadd(a,b,c);
input [`DWIDTH-1:0] a;
input [`DWIDTH-1:0] b;
output [`DWIDTH-1:0] c;

assign c = a + b;
//DW01_add #(`DWIDTH) u_add(.A(a), .B(b), .CI(1'b0), .SUM(c), .CO());
endmodule


`timescale 1ns / 1ps
//`define complex_dsp
`define DWIDTH 16
`define AWIDTH 16
`define MEM_SIZE 2048
`define MAT_MUL_SIZE 8
`define MASK_WIDTH 8
`define LOG2_MAT_MUL_SIZE 3
`define BB_MAT_MUL_SIZE `MAT_MUL_SIZE
`define NUM_CYCLES_IN_MAC 3
`define MEM_ACCESS_LATENCY 1
`define REG_DATAWIDTH 32
`define REG_ADDRWIDTH 8
`define ADDR_STRIDE_WIDTH 16
`define MAX_BITS_POOL 3
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2020-07-25 21:27:45.174821
// Design Name: 
// Module Name: matmul_8x8_systolic
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////

module matmul_8x8_systolic(
 clk,
 reset,
 pe_reset,
 start_mat_mul,
 done_mat_mul,
 address_mat_a,
 address_mat_b,
 address_mat_c,
 address_stride_a,
 address_stride_b,
 address_stride_c,
 a_data,
 b_data,
 a_data_in, //Data values coming in from previous matmul - systolic connections
 b_data_in,
 c_data_in, //Data values coming in from previous matmul - systolic shifting
 c_data_out, //Data values going out to next matmul - systolic shifting
 a_data_out,
 b_data_out,
 a_addr,
 b_addr,
 c_addr,
 c_data_available,

 validity_mask_a_rows,
 validity_mask_a_cols_b_rows,
 validity_mask_b_cols,
  
final_mat_mul_size,
  
 a_loc,
 b_loc
);

 input clk;
 input reset;
 input pe_reset;
 input start_mat_mul;
 output done_mat_mul;
 input [`AWIDTH-1:0] address_mat_a;
 input [`AWIDTH-1:0] address_mat_b;
 input [`AWIDTH-1:0] address_mat_c;
 input [`ADDR_STRIDE_WIDTH-1:0] address_stride_a;
 input [`ADDR_STRIDE_WIDTH-1:0] address_stride_b;
 input [`ADDR_STRIDE_WIDTH-1:0] address_stride_c;
 input [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data;
 input [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data;
 input [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_in;
 input [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_in;
 input [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_in;
 output [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_out;
 output [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_out;
 output [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_out;
 output [`AWIDTH-1:0] a_addr;
 output [`AWIDTH-1:0] b_addr;
 output [`AWIDTH-1:0] c_addr;
 output c_data_available;

 input [`MASK_WIDTH-1:0] validity_mask_a_rows;
 input [`MASK_WIDTH-1:0] validity_mask_a_cols_b_rows;
 input [`MASK_WIDTH-1:0] validity_mask_b_cols;

//7:0 is okay here. We aren't going to make a matmul larger than 128x128
//In fact, these will get optimized out by the synthesis tool, because
//we hardcode them at the instantiation level.
 input [7:0] final_mat_mul_size;
  
 input [7:0] a_loc;
 input [7:0] b_loc;

//////////////////////////////////////////////////////////////////////////
// Logic for clock counting and when to assert done
//////////////////////////////////////////////////////////////////////////

reg done_mat_mul;
//This is 7 bits because the expectation is that clock count will be pretty
//small. For large matmuls, this will need to increased to have more bits.
//In general, a systolic multiplier takes 4*N-2+P cycles, where N is the size 
//of the matmul and P is the number of pipleine stages in the MAC block.
reg [7:0] clk_cnt;

//Finding out number of cycles to assert matmul done.
//When we have to save the outputs to accumulators, then we don't need to
//shift out data. So, we can assert done_mat_mul early.
//In the normal case, we have to include the time to shift out the results. 
//Note: the count expression used to contain "4*final_mat_mul_size", but 
//to avoid multiplication, we now use "final_mat_mul_size<<2"
wire [7:0] clk_cnt_for_done;

assign clk_cnt_for_done = 
                          ((final_mat_mul_size<<2) - 2 + `NUM_CYCLES_IN_MAC);  
    
always @(posedge clk) begin
  if (reset || ~start_mat_mul) begin
    clk_cnt <= 0;
    done_mat_mul <= 0;
  end
  else if (clk_cnt == clk_cnt_for_done) begin
    done_mat_mul <= 1;
    clk_cnt <= clk_cnt + 1;

  end
  else if (done_mat_mul == 0) begin
    clk_cnt <= clk_cnt + 1;

  end    
  else begin
    done_mat_mul <= 0;
    clk_cnt <= clk_cnt + 1;
  end
end
wire [`DWIDTH-1:0] a0_data;
wire [`DWIDTH-1:0] a1_data;
wire [`DWIDTH-1:0] a2_data;
wire [`DWIDTH-1:0] a3_data;
wire [`DWIDTH-1:0] a4_data;
wire [`DWIDTH-1:0] a5_data;
wire [`DWIDTH-1:0] a6_data;
wire [`DWIDTH-1:0] a7_data;
wire [`DWIDTH-1:0] b0_data;
wire [`DWIDTH-1:0] b1_data;
wire [`DWIDTH-1:0] b2_data;
wire [`DWIDTH-1:0] b3_data;
wire [`DWIDTH-1:0] b4_data;
wire [`DWIDTH-1:0] b5_data;
wire [`DWIDTH-1:0] b6_data;
wire [`DWIDTH-1:0] b7_data;
wire [`DWIDTH-1:0] a1_data_delayed_1;
wire [`DWIDTH-1:0] a2_data_delayed_1;
wire [`DWIDTH-1:0] a2_data_delayed_2;
wire [`DWIDTH-1:0] a3_data_delayed_1;
wire [`DWIDTH-1:0] a3_data_delayed_2;
wire [`DWIDTH-1:0] a3_data_delayed_3;
wire [`DWIDTH-1:0] a4_data_delayed_1;
wire [`DWIDTH-1:0] a4_data_delayed_2;
wire [`DWIDTH-1:0] a4_data_delayed_3;
wire [`DWIDTH-1:0] a4_data_delayed_4;
wire [`DWIDTH-1:0] a5_data_delayed_1;
wire [`DWIDTH-1:0] a5_data_delayed_2;
wire [`DWIDTH-1:0] a5_data_delayed_3;
wire [`DWIDTH-1:0] a5_data_delayed_4;
wire [`DWIDTH-1:0] a5_data_delayed_5;
wire [`DWIDTH-1:0] a6_data_delayed_1;
wire [`DWIDTH-1:0] a6_data_delayed_2;
wire [`DWIDTH-1:0] a6_data_delayed_3;
wire [`DWIDTH-1:0] a6_data_delayed_4;
wire [`DWIDTH-1:0] a6_data_delayed_5;
wire [`DWIDTH-1:0] a6_data_delayed_6;
wire [`DWIDTH-1:0] a7_data_delayed_1;
wire [`DWIDTH-1:0] a7_data_delayed_2;
wire [`DWIDTH-1:0] a7_data_delayed_3;
wire [`DWIDTH-1:0] a7_data_delayed_4;
wire [`DWIDTH-1:0] a7_data_delayed_5;
wire [`DWIDTH-1:0] a7_data_delayed_6;
wire [`DWIDTH-1:0] a7_data_delayed_7;
wire [`DWIDTH-1:0] b1_data_delayed_1;
wire [`DWIDTH-1:0] b2_data_delayed_1;
wire [`DWIDTH-1:0] b2_data_delayed_2;
wire [`DWIDTH-1:0] b3_data_delayed_1;
wire [`DWIDTH-1:0] b3_data_delayed_2;
wire [`DWIDTH-1:0] b3_data_delayed_3;
wire [`DWIDTH-1:0] b4_data_delayed_1;
wire [`DWIDTH-1:0] b4_data_delayed_2;
wire [`DWIDTH-1:0] b4_data_delayed_3;
wire [`DWIDTH-1:0] b4_data_delayed_4;
wire [`DWIDTH-1:0] b5_data_delayed_1;
wire [`DWIDTH-1:0] b5_data_delayed_2;
wire [`DWIDTH-1:0] b5_data_delayed_3;
wire [`DWIDTH-1:0] b5_data_delayed_4;
wire [`DWIDTH-1:0] b5_data_delayed_5;
wire [`DWIDTH-1:0] b6_data_delayed_1;
wire [`DWIDTH-1:0] b6_data_delayed_2;
wire [`DWIDTH-1:0] b6_data_delayed_3;
wire [`DWIDTH-1:0] b6_data_delayed_4;
wire [`DWIDTH-1:0] b6_data_delayed_5;
wire [`DWIDTH-1:0] b6_data_delayed_6;
wire [`DWIDTH-1:0] b7_data_delayed_1;
wire [`DWIDTH-1:0] b7_data_delayed_2;
wire [`DWIDTH-1:0] b7_data_delayed_3;
wire [`DWIDTH-1:0] b7_data_delayed_4;
wire [`DWIDTH-1:0] b7_data_delayed_5;
wire [`DWIDTH-1:0] b7_data_delayed_6;
wire [`DWIDTH-1:0] b7_data_delayed_7;


//////////////////////////////////////////////////////////////////////////
// Instantiation of systolic data setup
//////////////////////////////////////////////////////////////////////////
systolic_data_setup_systolic_8x8 u_systolic_data_setup_systolic_8x8(
.clk(clk),
.reset(reset),
.start_mat_mul(start_mat_mul),
.a_addr(a_addr),
.b_addr(b_addr),
.address_mat_a(address_mat_a),
.address_mat_b(address_mat_b),
.address_stride_a(address_stride_a),
.address_stride_b(address_stride_b),
.a_data(a_data),
.b_data(b_data),
.clk_cnt(clk_cnt),
.a0_data(a0_data),
.b0_data(b0_data),
.a1_data_delayed_1(a1_data_delayed_1),
.b1_data_delayed_1(b1_data_delayed_1),
.a2_data_delayed_2(a2_data_delayed_2),
.b2_data_delayed_2(b2_data_delayed_2),
.a3_data_delayed_3(a3_data_delayed_3),
.b3_data_delayed_3(b3_data_delayed_3),
.a4_data_delayed_4(a4_data_delayed_4),
.b4_data_delayed_4(b4_data_delayed_4),
.a5_data_delayed_5(a5_data_delayed_5),
.b5_data_delayed_5(b5_data_delayed_5),
.a6_data_delayed_6(a6_data_delayed_6),
.b6_data_delayed_6(b6_data_delayed_6),
.a7_data_delayed_7(a7_data_delayed_7),
.b7_data_delayed_7(b7_data_delayed_7),

.validity_mask_a_rows(validity_mask_a_rows),
.validity_mask_a_cols_b_rows(validity_mask_a_cols_b_rows),
.validity_mask_b_cols(validity_mask_b_cols),

.final_mat_mul_size(final_mat_mul_size),
  
.a_loc(a_loc),
.b_loc(b_loc)
);

//////////////////////////////////////////////////////////////////////////
// Logic to mux data_in coming from neighboring matmuls
//////////////////////////////////////////////////////////////////////////
wire [`DWIDTH-1:0] a0;
wire [`DWIDTH-1:0] a1;
wire [`DWIDTH-1:0] a2;
wire [`DWIDTH-1:0] a3;
wire [`DWIDTH-1:0] a4;
wire [`DWIDTH-1:0] a5;
wire [`DWIDTH-1:0] a6;
wire [`DWIDTH-1:0] a7;
wire [`DWIDTH-1:0] b0;
wire [`DWIDTH-1:0] b1;
wire [`DWIDTH-1:0] b2;
wire [`DWIDTH-1:0] b3;
wire [`DWIDTH-1:0] b4;
wire [`DWIDTH-1:0] b5;
wire [`DWIDTH-1:0] b6;
wire [`DWIDTH-1:0] b7;

wire [`DWIDTH-1:0] a0_data_in;
wire [`DWIDTH-1:0] a1_data_in;
wire [`DWIDTH-1:0] a2_data_in;
wire [`DWIDTH-1:0] a3_data_in;
wire [`DWIDTH-1:0] a4_data_in;
wire [`DWIDTH-1:0] a5_data_in;
wire [`DWIDTH-1:0] a6_data_in;
wire [`DWIDTH-1:0] a7_data_in;

assign a0_data_in = a_data_in[1*`DWIDTH-1:0*`DWIDTH];
assign a1_data_in = a_data_in[2*`DWIDTH-1:1*`DWIDTH];
assign a2_data_in = a_data_in[3*`DWIDTH-1:2*`DWIDTH];
assign a3_data_in = a_data_in[4*`DWIDTH-1:3*`DWIDTH];
assign a4_data_in = a_data_in[5*`DWIDTH-1:4*`DWIDTH];
assign a5_data_in = a_data_in[6*`DWIDTH-1:5*`DWIDTH];
assign a6_data_in = a_data_in[7*`DWIDTH-1:6*`DWIDTH];
assign a7_data_in = a_data_in[8*`DWIDTH-1:7*`DWIDTH];

wire [`DWIDTH-1:0] b0_data_in;
wire [`DWIDTH-1:0] b1_data_in;
wire [`DWIDTH-1:0] b2_data_in;
wire [`DWIDTH-1:0] b3_data_in;
wire [`DWIDTH-1:0] b4_data_in;
wire [`DWIDTH-1:0] b5_data_in;
wire [`DWIDTH-1:0] b6_data_in;
wire [`DWIDTH-1:0] b7_data_in;

assign b0_data_in = b_data_in[1*`DWIDTH-1:0*`DWIDTH];
assign b1_data_in = b_data_in[2*`DWIDTH-1:1*`DWIDTH];
assign b2_data_in = b_data_in[3*`DWIDTH-1:2*`DWIDTH];
assign b3_data_in = b_data_in[4*`DWIDTH-1:3*`DWIDTH];
assign b4_data_in = b_data_in[5*`DWIDTH-1:4*`DWIDTH];
assign b5_data_in = b_data_in[6*`DWIDTH-1:5*`DWIDTH];
assign b6_data_in = b_data_in[7*`DWIDTH-1:6*`DWIDTH];
assign b7_data_in = b_data_in[8*`DWIDTH-1:7*`DWIDTH];

assign a0 = (b_loc==0) ? a0_data           : a0_data_in;
assign a1 = (b_loc==0) ? a1_data_delayed_1 : a1_data_in;
assign a2 = (b_loc==0) ? a2_data_delayed_2 : a2_data_in;
assign a3 = (b_loc==0) ? a3_data_delayed_3 : a3_data_in;
assign a4 = (b_loc==0) ? a4_data_delayed_4 : a4_data_in;
assign a5 = (b_loc==0) ? a5_data_delayed_5 : a5_data_in;
assign a6 = (b_loc==0) ? a6_data_delayed_6 : a6_data_in;
assign a7 = (b_loc==0) ? a7_data_delayed_7 : a7_data_in;

assign b0 = (a_loc==0) ? b0_data           : b0_data_in;
assign b1 = (a_loc==0) ? b1_data_delayed_1 : b1_data_in;
assign b2 = (a_loc==0) ? b2_data_delayed_2 : b2_data_in;
assign b3 = (a_loc==0) ? b3_data_delayed_3 : b3_data_in;
assign b4 = (a_loc==0) ? b4_data_delayed_4 : b4_data_in;
assign b5 = (a_loc==0) ? b5_data_delayed_5 : b5_data_in;
assign b6 = (a_loc==0) ? b6_data_delayed_6 : b6_data_in;
assign b7 = (a_loc==0) ? b7_data_delayed_7 : b7_data_in;

wire [`DWIDTH-1:0] matrixC0_0;
wire [`DWIDTH-1:0] matrixC0_1;
wire [`DWIDTH-1:0] matrixC0_2;
wire [`DWIDTH-1:0] matrixC0_3;
wire [`DWIDTH-1:0] matrixC0_4;
wire [`DWIDTH-1:0] matrixC0_5;
wire [`DWIDTH-1:0] matrixC0_6;
wire [`DWIDTH-1:0] matrixC0_7;
wire [`DWIDTH-1:0] matrixC1_0;
wire [`DWIDTH-1:0] matrixC1_1;
wire [`DWIDTH-1:0] matrixC1_2;
wire [`DWIDTH-1:0] matrixC1_3;
wire [`DWIDTH-1:0] matrixC1_4;
wire [`DWIDTH-1:0] matrixC1_5;
wire [`DWIDTH-1:0] matrixC1_6;
wire [`DWIDTH-1:0] matrixC1_7;
wire [`DWIDTH-1:0] matrixC2_0;
wire [`DWIDTH-1:0] matrixC2_1;
wire [`DWIDTH-1:0] matrixC2_2;
wire [`DWIDTH-1:0] matrixC2_3;
wire [`DWIDTH-1:0] matrixC2_4;
wire [`DWIDTH-1:0] matrixC2_5;
wire [`DWIDTH-1:0] matrixC2_6;
wire [`DWIDTH-1:0] matrixC2_7;
wire [`DWIDTH-1:0] matrixC3_0;
wire [`DWIDTH-1:0] matrixC3_1;
wire [`DWIDTH-1:0] matrixC3_2;
wire [`DWIDTH-1:0] matrixC3_3;
wire [`DWIDTH-1:0] matrixC3_4;
wire [`DWIDTH-1:0] matrixC3_5;
wire [`DWIDTH-1:0] matrixC3_6;
wire [`DWIDTH-1:0] matrixC3_7;
wire [`DWIDTH-1:0] matrixC4_0;
wire [`DWIDTH-1:0] matrixC4_1;
wire [`DWIDTH-1:0] matrixC4_2;
wire [`DWIDTH-1:0] matrixC4_3;
wire [`DWIDTH-1:0] matrixC4_4;
wire [`DWIDTH-1:0] matrixC4_5;
wire [`DWIDTH-1:0] matrixC4_6;
wire [`DWIDTH-1:0] matrixC4_7;
wire [`DWIDTH-1:0] matrixC5_0;
wire [`DWIDTH-1:0] matrixC5_1;
wire [`DWIDTH-1:0] matrixC5_2;
wire [`DWIDTH-1:0] matrixC5_3;
wire [`DWIDTH-1:0] matrixC5_4;
wire [`DWIDTH-1:0] matrixC5_5;
wire [`DWIDTH-1:0] matrixC5_6;
wire [`DWIDTH-1:0] matrixC5_7;
wire [`DWIDTH-1:0] matrixC6_0;
wire [`DWIDTH-1:0] matrixC6_1;
wire [`DWIDTH-1:0] matrixC6_2;
wire [`DWIDTH-1:0] matrixC6_3;
wire [`DWIDTH-1:0] matrixC6_4;
wire [`DWIDTH-1:0] matrixC6_5;
wire [`DWIDTH-1:0] matrixC6_6;
wire [`DWIDTH-1:0] matrixC6_7;
wire [`DWIDTH-1:0] matrixC7_0;
wire [`DWIDTH-1:0] matrixC7_1;
wire [`DWIDTH-1:0] matrixC7_2;
wire [`DWIDTH-1:0] matrixC7_3;
wire [`DWIDTH-1:0] matrixC7_4;
wire [`DWIDTH-1:0] matrixC7_5;
wire [`DWIDTH-1:0] matrixC7_6;
wire [`DWIDTH-1:0] matrixC7_7;


wire row_latch_en;

//////////////////////////////////////////////////////////////////////////
// Instantiation of the output logic
//////////////////////////////////////////////////////////////////////////
output_logic_systolic_8x8 u_output_logic_systolic_8x8(
.start_mat_mul(start_mat_mul),
.done_mat_mul(done_mat_mul),
.address_mat_c(address_mat_c),
.address_stride_c(address_stride_c),
.c_data_out(c_data_out),
.c_data_in(c_data_in),
.c_addr(c_addr),
.c_data_available(c_data_available),
.clk_cnt(clk_cnt),
.row_latch_en(row_latch_en),

.final_mat_mul_size(final_mat_mul_size),
  .matrixC0_0(matrixC0_0),
.matrixC0_1(matrixC0_1),
.matrixC0_2(matrixC0_2),
.matrixC0_3(matrixC0_3),
.matrixC0_4(matrixC0_4),
.matrixC0_5(matrixC0_5),
.matrixC0_6(matrixC0_6),
.matrixC0_7(matrixC0_7),
.matrixC1_0(matrixC1_0),
.matrixC1_1(matrixC1_1),
.matrixC1_2(matrixC1_2),
.matrixC1_3(matrixC1_3),
.matrixC1_4(matrixC1_4),
.matrixC1_5(matrixC1_5),
.matrixC1_6(matrixC1_6),
.matrixC1_7(matrixC1_7),
.matrixC2_0(matrixC2_0),
.matrixC2_1(matrixC2_1),
.matrixC2_2(matrixC2_2),
.matrixC2_3(matrixC2_3),
.matrixC2_4(matrixC2_4),
.matrixC2_5(matrixC2_5),
.matrixC2_6(matrixC2_6),
.matrixC2_7(matrixC2_7),
.matrixC3_0(matrixC3_0),
.matrixC3_1(matrixC3_1),
.matrixC3_2(matrixC3_2),
.matrixC3_3(matrixC3_3),
.matrixC3_4(matrixC3_4),
.matrixC3_5(matrixC3_5),
.matrixC3_6(matrixC3_6),
.matrixC3_7(matrixC3_7),
.matrixC4_0(matrixC4_0),
.matrixC4_1(matrixC4_1),
.matrixC4_2(matrixC4_2),
.matrixC4_3(matrixC4_3),
.matrixC4_4(matrixC4_4),
.matrixC4_5(matrixC4_5),
.matrixC4_6(matrixC4_6),
.matrixC4_7(matrixC4_7),
.matrixC5_0(matrixC5_0),
.matrixC5_1(matrixC5_1),
.matrixC5_2(matrixC5_2),
.matrixC5_3(matrixC5_3),
.matrixC5_4(matrixC5_4),
.matrixC5_5(matrixC5_5),
.matrixC5_6(matrixC5_6),
.matrixC5_7(matrixC5_7),
.matrixC6_0(matrixC6_0),
.matrixC6_1(matrixC6_1),
.matrixC6_2(matrixC6_2),
.matrixC6_3(matrixC6_3),
.matrixC6_4(matrixC6_4),
.matrixC6_5(matrixC6_5),
.matrixC6_6(matrixC6_6),
.matrixC6_7(matrixC6_7),
.matrixC7_0(matrixC7_0),
.matrixC7_1(matrixC7_1),
.matrixC7_2(matrixC7_2),
.matrixC7_3(matrixC7_3),
.matrixC7_4(matrixC7_4),
.matrixC7_5(matrixC7_5),
.matrixC7_6(matrixC7_6),
.matrixC7_7(matrixC7_7),

.clk(clk),
.reset(reset)
);

//////////////////////////////////////////////////////////////////////////
// Instantiations of the actual PEs
//////////////////////////////////////////////////////////////////////////
systolic_pe_matrix_systolic_8x8 u_systolic_pe_matrix_systolic_8x8(
.clk(clk),
.reset(reset),
.pe_reset(pe_reset),
.a0(a0),
.a1(a1),
.a2(a2),
.a3(a3),
.a4(a4),
.a5(a5),
.a6(a6),
.a7(a7),
.b0(b0),
.b1(b1),
.b2(b2),
.b3(b3),
.b4(b4),
.b5(b5),
.b6(b6),
.b7(b7),
.matrixC0_0(matrixC0_0),
.matrixC0_1(matrixC0_1),
.matrixC0_2(matrixC0_2),
.matrixC0_3(matrixC0_3),
.matrixC0_4(matrixC0_4),
.matrixC0_5(matrixC0_5),
.matrixC0_6(matrixC0_6),
.matrixC0_7(matrixC0_7),
.matrixC1_0(matrixC1_0),
.matrixC1_1(matrixC1_1),
.matrixC1_2(matrixC1_2),
.matrixC1_3(matrixC1_3),
.matrixC1_4(matrixC1_4),
.matrixC1_5(matrixC1_5),
.matrixC1_6(matrixC1_6),
.matrixC1_7(matrixC1_7),
.matrixC2_0(matrixC2_0),
.matrixC2_1(matrixC2_1),
.matrixC2_2(matrixC2_2),
.matrixC2_3(matrixC2_3),
.matrixC2_4(matrixC2_4),
.matrixC2_5(matrixC2_5),
.matrixC2_6(matrixC2_6),
.matrixC2_7(matrixC2_7),
.matrixC3_0(matrixC3_0),
.matrixC3_1(matrixC3_1),
.matrixC3_2(matrixC3_2),
.matrixC3_3(matrixC3_3),
.matrixC3_4(matrixC3_4),
.matrixC3_5(matrixC3_5),
.matrixC3_6(matrixC3_6),
.matrixC3_7(matrixC3_7),
.matrixC4_0(matrixC4_0),
.matrixC4_1(matrixC4_1),
.matrixC4_2(matrixC4_2),
.matrixC4_3(matrixC4_3),
.matrixC4_4(matrixC4_4),
.matrixC4_5(matrixC4_5),
.matrixC4_6(matrixC4_6),
.matrixC4_7(matrixC4_7),
.matrixC5_0(matrixC5_0),
.matrixC5_1(matrixC5_1),
.matrixC5_2(matrixC5_2),
.matrixC5_3(matrixC5_3),
.matrixC5_4(matrixC5_4),
.matrixC5_5(matrixC5_5),
.matrixC5_6(matrixC5_6),
.matrixC5_7(matrixC5_7),
.matrixC6_0(matrixC6_0),
.matrixC6_1(matrixC6_1),
.matrixC6_2(matrixC6_2),
.matrixC6_3(matrixC6_3),
.matrixC6_4(matrixC6_4),
.matrixC6_5(matrixC6_5),
.matrixC6_6(matrixC6_6),
.matrixC6_7(matrixC6_7),
.matrixC7_0(matrixC7_0),
.matrixC7_1(matrixC7_1),
.matrixC7_2(matrixC7_2),
.matrixC7_3(matrixC7_3),
.matrixC7_4(matrixC7_4),
.matrixC7_5(matrixC7_5),
.matrixC7_6(matrixC7_6),
.matrixC7_7(matrixC7_7),

.a_data_out(a_data_out),
.b_data_out(b_data_out)
);

endmodule


//////////////////////////////////////////////////////////////////////////
// Output logic
//////////////////////////////////////////////////////////////////////////
module output_logic_systolic_8x8(
start_mat_mul,
done_mat_mul,
address_mat_c,
address_stride_c,
c_data_in,
c_data_out, //Data values going out to next matmul - systolic shifting
c_addr,
c_data_available,
clk_cnt,
row_latch_en,

final_mat_mul_size,
  matrixC0_0,
matrixC0_1,
matrixC0_2,
matrixC0_3,
matrixC0_4,
matrixC0_5,
matrixC0_6,
matrixC0_7,
matrixC1_0,
matrixC1_1,
matrixC1_2,
matrixC1_3,
matrixC1_4,
matrixC1_5,
matrixC1_6,
matrixC1_7,
matrixC2_0,
matrixC2_1,
matrixC2_2,
matrixC2_3,
matrixC2_4,
matrixC2_5,
matrixC2_6,
matrixC2_7,
matrixC3_0,
matrixC3_1,
matrixC3_2,
matrixC3_3,
matrixC3_4,
matrixC3_5,
matrixC3_6,
matrixC3_7,
matrixC4_0,
matrixC4_1,
matrixC4_2,
matrixC4_3,
matrixC4_4,
matrixC4_5,
matrixC4_6,
matrixC4_7,
matrixC5_0,
matrixC5_1,
matrixC5_2,
matrixC5_3,
matrixC5_4,
matrixC5_5,
matrixC5_6,
matrixC5_7,
matrixC6_0,
matrixC6_1,
matrixC6_2,
matrixC6_3,
matrixC6_4,
matrixC6_5,
matrixC6_6,
matrixC6_7,
matrixC7_0,
matrixC7_1,
matrixC7_2,
matrixC7_3,
matrixC7_4,
matrixC7_5,
matrixC7_6,
matrixC7_7,

clk,
reset
);

input clk;
input reset;
input start_mat_mul;
input done_mat_mul;
input [`AWIDTH-1:0] address_mat_c;
input [`ADDR_STRIDE_WIDTH-1:0] address_stride_c;
input [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_in;
output [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_out;
output [`AWIDTH-1:0] c_addr;
output c_data_available;
input [7:0] clk_cnt;
output row_latch_en;

input [7:0] final_mat_mul_size;
  input [`DWIDTH-1:0] matrixC0_0;
input [`DWIDTH-1:0] matrixC0_1;
input [`DWIDTH-1:0] matrixC0_2;
input [`DWIDTH-1:0] matrixC0_3;
input [`DWIDTH-1:0] matrixC0_4;
input [`DWIDTH-1:0] matrixC0_5;
input [`DWIDTH-1:0] matrixC0_6;
input [`DWIDTH-1:0] matrixC0_7;
input [`DWIDTH-1:0] matrixC1_0;
input [`DWIDTH-1:0] matrixC1_1;
input [`DWIDTH-1:0] matrixC1_2;
input [`DWIDTH-1:0] matrixC1_3;
input [`DWIDTH-1:0] matrixC1_4;
input [`DWIDTH-1:0] matrixC1_5;
input [`DWIDTH-1:0] matrixC1_6;
input [`DWIDTH-1:0] matrixC1_7;
input [`DWIDTH-1:0] matrixC2_0;
input [`DWIDTH-1:0] matrixC2_1;
input [`DWIDTH-1:0] matrixC2_2;
input [`DWIDTH-1:0] matrixC2_3;
input [`DWIDTH-1:0] matrixC2_4;
input [`DWIDTH-1:0] matrixC2_5;
input [`DWIDTH-1:0] matrixC2_6;
input [`DWIDTH-1:0] matrixC2_7;
input [`DWIDTH-1:0] matrixC3_0;
input [`DWIDTH-1:0] matrixC3_1;
input [`DWIDTH-1:0] matrixC3_2;
input [`DWIDTH-1:0] matrixC3_3;
input [`DWIDTH-1:0] matrixC3_4;
input [`DWIDTH-1:0] matrixC3_5;
input [`DWIDTH-1:0] matrixC3_6;
input [`DWIDTH-1:0] matrixC3_7;
input [`DWIDTH-1:0] matrixC4_0;
input [`DWIDTH-1:0] matrixC4_1;
input [`DWIDTH-1:0] matrixC4_2;
input [`DWIDTH-1:0] matrixC4_3;
input [`DWIDTH-1:0] matrixC4_4;
input [`DWIDTH-1:0] matrixC4_5;
input [`DWIDTH-1:0] matrixC4_6;
input [`DWIDTH-1:0] matrixC4_7;
input [`DWIDTH-1:0] matrixC5_0;
input [`DWIDTH-1:0] matrixC5_1;
input [`DWIDTH-1:0] matrixC5_2;
input [`DWIDTH-1:0] matrixC5_3;
input [`DWIDTH-1:0] matrixC5_4;
input [`DWIDTH-1:0] matrixC5_5;
input [`DWIDTH-1:0] matrixC5_6;
input [`DWIDTH-1:0] matrixC5_7;
input [`DWIDTH-1:0] matrixC6_0;
input [`DWIDTH-1:0] matrixC6_1;
input [`DWIDTH-1:0] matrixC6_2;
input [`DWIDTH-1:0] matrixC6_3;
input [`DWIDTH-1:0] matrixC6_4;
input [`DWIDTH-1:0] matrixC6_5;
input [`DWIDTH-1:0] matrixC6_6;
input [`DWIDTH-1:0] matrixC6_7;
input [`DWIDTH-1:0] matrixC7_0;
input [`DWIDTH-1:0] matrixC7_1;
input [`DWIDTH-1:0] matrixC7_2;
input [`DWIDTH-1:0] matrixC7_3;
input [`DWIDTH-1:0] matrixC7_4;
input [`DWIDTH-1:0] matrixC7_5;
input [`DWIDTH-1:0] matrixC7_6;
input [`DWIDTH-1:0] matrixC7_7;
wire row_latch_en;


//////////////////////////////////////////////////////////////////////////
// Logic to capture matrix C data from the PEs and shift it out
//////////////////////////////////////////////////////////////////////////
//assign row_latch_en = (clk_cnt==(`MAT_MUL_SIZE + (a_loc+b_loc) * `BB_MAT_MUL_SIZE + 10 +  `NUM_CYCLES_IN_MAC - 1));
//Writing the line above to avoid multiplication:
//assign row_latch_en = (clk_cnt==(`MAT_MUL_SIZE + ((a_loc+b_loc) << `LOG2_MAT_MUL_SIZE) + 10 +  `NUM_CYCLES_IN_MAC - 1));

assign row_latch_en =  
                       ((clk_cnt == ((final_mat_mul_size<<2) - final_mat_mul_size - 1 +`NUM_CYCLES_IN_MAC)));
    
reg c_data_available;
reg [`AWIDTH-1:0] c_addr;
reg start_capturing_c_data;
integer counter;
reg [8*`DWIDTH-1:0] c_data_out;
reg [8*`DWIDTH-1:0] c_data_out_1;
reg [8*`DWIDTH-1:0] c_data_out_2;
reg [8*`DWIDTH-1:0] c_data_out_3;
reg [8*`DWIDTH-1:0] c_data_out_4;
reg [8*`DWIDTH-1:0] c_data_out_5;
reg [8*`DWIDTH-1:0] c_data_out_6;
reg [8*`DWIDTH-1:0] c_data_out_7;
wire condition_to_start_shifting_output;
assign condition_to_start_shifting_output = 
                          row_latch_en ;  

  
//For larger matmuls, this logic will have more entries in the case statement
always @(posedge clk) begin
  if (reset | ~start_mat_mul) begin
    start_capturing_c_data <= 1'b0;
    c_data_available <= 1'b0;
    c_addr <= address_mat_c + address_stride_c;
    c_data_out <= 0;
    counter <= 0;

    c_data_out_1 <= 0;
    c_data_out_2 <= 0;
    c_data_out_3 <= 0;
    c_data_out_4 <= 0;
    c_data_out_5 <= 0;
    c_data_out_6 <= 0;
    c_data_out_7 <= 0;
  end else if (condition_to_start_shifting_output) begin
    start_capturing_c_data <= 1'b1;
    c_data_available <= 1'b1;
    c_addr <= c_addr - address_stride_c;
    c_data_out <= {matrixC7_7, matrixC6_7, matrixC5_7, matrixC4_7, matrixC3_7, matrixC2_7, matrixC1_7, matrixC0_7};
      c_data_out_1 <= {matrixC7_6, matrixC6_6, matrixC5_6, matrixC4_6, matrixC3_6, matrixC2_6, matrixC1_6, matrixC0_6};
      c_data_out_2 <= {matrixC7_5, matrixC6_5, matrixC5_5, matrixC4_5, matrixC3_5, matrixC2_5, matrixC1_5, matrixC0_5};
      c_data_out_3 <= {matrixC7_4, matrixC6_4, matrixC5_4, matrixC4_4, matrixC3_4, matrixC2_4, matrixC1_4, matrixC0_4};
      c_data_out_4 <= {matrixC7_3, matrixC6_3, matrixC5_3, matrixC4_3, matrixC3_3, matrixC2_3, matrixC1_3, matrixC0_3};
      c_data_out_5 <= {matrixC7_2, matrixC6_2, matrixC5_2, matrixC4_2, matrixC3_2, matrixC2_2, matrixC1_2, matrixC0_2};
      c_data_out_6 <= {matrixC7_1, matrixC6_1, matrixC5_1, matrixC4_1, matrixC3_1, matrixC2_1, matrixC1_1, matrixC0_1};
      c_data_out_7 <= {matrixC7_0, matrixC6_0, matrixC5_0, matrixC4_0, matrixC3_0, matrixC2_0, matrixC1_0, matrixC0_0};

    counter <= counter + 1;
  end else if (done_mat_mul) begin
    start_capturing_c_data <= 1'b0;
    c_data_available <= 1'b0;
    c_addr <= address_mat_c + address_stride_c;
    c_data_out <= 0;

    c_data_out_1 <= 0;
    c_data_out_2 <= 0;
    c_data_out_3 <= 0;
    c_data_out_4 <= 0;
    c_data_out_5 <= 0;
    c_data_out_6 <= 0;
    c_data_out_7 <= 0;
  end 
  else if (counter >= `MAT_MUL_SIZE) begin
    c_data_out <= c_data_out_1;
    c_addr <= c_addr - address_stride_c; 

    c_data_out_1 <= c_data_out_2;
    c_data_out_2 <= c_data_out_3;
    c_data_out_3 <= c_data_out_4;
    c_data_out_4 <= c_data_out_5;
    c_data_out_5 <= c_data_out_6;
    c_data_out_6 <= c_data_out_7;
    c_data_out_7 <= c_data_in;
  end
  else if (start_capturing_c_data) begin
    c_data_available <= 1'b1;
    c_addr <= c_addr - address_stride_c; 
    counter <= counter + 1;
    c_data_out <= c_data_out_1;

    c_data_out_1 <= c_data_out_2;
    c_data_out_2 <= c_data_out_3;
    c_data_out_3 <= c_data_out_4;
    c_data_out_4 <= c_data_out_5;
    c_data_out_5 <= c_data_out_6;
    c_data_out_6 <= c_data_out_7;
    c_data_out_7 <= c_data_in;
  end
end

endmodule


//////////////////////////////////////////////////////////////////////////
// Systolic data setup
//////////////////////////////////////////////////////////////////////////
module systolic_data_setup_systolic_8x8(
clk,
reset,
start_mat_mul,
a_addr,
b_addr,
address_mat_a,
address_mat_b,
address_stride_a,
address_stride_b,
a_data,
b_data,
clk_cnt,
a0_data,
b0_data,
a1_data_delayed_1,
b1_data_delayed_1,
a2_data_delayed_2,
b2_data_delayed_2,
a3_data_delayed_3,
b3_data_delayed_3,
a4_data_delayed_4,
b4_data_delayed_4,
a5_data_delayed_5,
b5_data_delayed_5,
a6_data_delayed_6,
b6_data_delayed_6,
a7_data_delayed_7,
b7_data_delayed_7,

validity_mask_a_rows,
validity_mask_a_cols_b_rows,
validity_mask_b_cols,

final_mat_mul_size,
  
a_loc,
b_loc
);

input clk;
input reset;
input start_mat_mul;
output [`AWIDTH-1:0] a_addr;
output [`AWIDTH-1:0] b_addr;
input [`AWIDTH-1:0] address_mat_a;
input [`AWIDTH-1:0] address_mat_b;
input [`ADDR_STRIDE_WIDTH-1:0] address_stride_a;
input [`ADDR_STRIDE_WIDTH-1:0] address_stride_b;
input [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data;
input [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data;
input [7:0] clk_cnt;
output [`DWIDTH-1:0] a0_data;
output [`DWIDTH-1:0] b0_data;
output [`DWIDTH-1:0] a1_data_delayed_1;
output [`DWIDTH-1:0] b1_data_delayed_1;
output [`DWIDTH-1:0] a2_data_delayed_2;
output [`DWIDTH-1:0] b2_data_delayed_2;
output [`DWIDTH-1:0] a3_data_delayed_3;
output [`DWIDTH-1:0] b3_data_delayed_3;
output [`DWIDTH-1:0] a4_data_delayed_4;
output [`DWIDTH-1:0] b4_data_delayed_4;
output [`DWIDTH-1:0] a5_data_delayed_5;
output [`DWIDTH-1:0] b5_data_delayed_5;
output [`DWIDTH-1:0] a6_data_delayed_6;
output [`DWIDTH-1:0] b6_data_delayed_6;
output [`DWIDTH-1:0] a7_data_delayed_7;
output [`DWIDTH-1:0] b7_data_delayed_7;

input [`MASK_WIDTH-1:0] validity_mask_a_rows;
input [`MASK_WIDTH-1:0] validity_mask_a_cols_b_rows;
input [`MASK_WIDTH-1:0] validity_mask_b_cols;

input [7:0] final_mat_mul_size;
  
input [7:0] a_loc;
input [7:0] b_loc;
wire [`DWIDTH-1:0] a0_data;
wire [`DWIDTH-1:0] a1_data;
wire [`DWIDTH-1:0] a2_data;
wire [`DWIDTH-1:0] a3_data;
wire [`DWIDTH-1:0] a4_data;
wire [`DWIDTH-1:0] a5_data;
wire [`DWIDTH-1:0] a6_data;
wire [`DWIDTH-1:0] a7_data;
wire [`DWIDTH-1:0] b0_data;
wire [`DWIDTH-1:0] b1_data;
wire [`DWIDTH-1:0] b2_data;
wire [`DWIDTH-1:0] b3_data;
wire [`DWIDTH-1:0] b4_data;
wire [`DWIDTH-1:0] b5_data;
wire [`DWIDTH-1:0] b6_data;
wire [`DWIDTH-1:0] b7_data;

//////////////////////////////////////////////////////////////////////////
// Logic to generate addresses to BRAM A
//////////////////////////////////////////////////////////////////////////
reg [`AWIDTH-1:0] a_addr;
reg a_mem_access; //flag that tells whether the matmul is trying to access memory or not

always @(posedge clk) begin
  //(clk_cnt >= a_loc*`MAT_MUL_SIZE+final_mat_mul_size) begin
  //Writing the line above to avoid multiplication:

  if (reset || ~start_mat_mul || (clk_cnt >= (a_loc<<`LOG2_MAT_MUL_SIZE)+final_mat_mul_size)) begin
  
      a_addr <= address_mat_a-address_stride_a;
  
    a_mem_access <= 0;
  end
  //else if ((clk_cnt >= a_loc*`MAT_MUL_SIZE) && (clk_cnt < a_loc*`MAT_MUL_SIZE+final_mat_mul_size)) begin
  //Writing the line above to avoid multiplication:

  else if ((clk_cnt >= (a_loc<<`LOG2_MAT_MUL_SIZE)) && (clk_cnt < (a_loc<<`LOG2_MAT_MUL_SIZE)+final_mat_mul_size)) begin
  
      a_addr <= a_addr + address_stride_a;
  
    a_mem_access <= 1;
  end
end

//////////////////////////////////////////////////////////////////////////
// Logic to generate valid signals for data coming from BRAM A
//////////////////////////////////////////////////////////////////////////
reg [7:0] a_mem_access_counter;
always @(posedge clk) begin
  if (reset || ~start_mat_mul) begin
    a_mem_access_counter <= 0;
  end
  else if (a_mem_access == 1) begin
    a_mem_access_counter <= a_mem_access_counter + 1;  
  end
  else begin
    a_mem_access_counter <= 0;
  end
end

wire a_data_valid; //flag that tells whether the data from memory is valid
assign a_data_valid = 
     ((validity_mask_a_cols_b_rows[0]==1'b0 && a_mem_access_counter==1) ||
      (validity_mask_a_cols_b_rows[1]==1'b0 && a_mem_access_counter==2) ||
      (validity_mask_a_cols_b_rows[2]==1'b0 && a_mem_access_counter==3) ||
      (validity_mask_a_cols_b_rows[3]==1'b0 && a_mem_access_counter==4) ||
      (validity_mask_a_cols_b_rows[4]==1'b0 && a_mem_access_counter==5) ||
      (validity_mask_a_cols_b_rows[5]==1'b0 && a_mem_access_counter==6) ||
      (validity_mask_a_cols_b_rows[6]==1'b0 && a_mem_access_counter==7) ||
      (validity_mask_a_cols_b_rows[7]==1'b0 && a_mem_access_counter==8)) ?
    
    1'b0 : (a_mem_access_counter >= `MEM_ACCESS_LATENCY);

//////////////////////////////////////////////////////////////////////////
// Logic to delay certain parts of the data received from BRAM A (systolic data setup)
//////////////////////////////////////////////////////////////////////////
assign a0_data = a_data[1*`DWIDTH-1:0*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[0]}};
assign a1_data = a_data[2*`DWIDTH-1:1*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[1]}};
assign a2_data = a_data[3*`DWIDTH-1:2*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[2]}};
assign a3_data = a_data[4*`DWIDTH-1:3*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[3]}};
assign a4_data = a_data[5*`DWIDTH-1:4*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[4]}};
assign a5_data = a_data[6*`DWIDTH-1:5*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[5]}};
assign a6_data = a_data[7*`DWIDTH-1:6*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[6]}};
assign a7_data = a_data[8*`DWIDTH-1:7*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[7]}};

reg [`DWIDTH-1:0] a1_data_delayed_1;
reg [`DWIDTH-1:0] a2_data_delayed_1;
reg [`DWIDTH-1:0] a2_data_delayed_2;
reg [`DWIDTH-1:0] a3_data_delayed_1;
reg [`DWIDTH-1:0] a3_data_delayed_2;
reg [`DWIDTH-1:0] a3_data_delayed_3;
reg [`DWIDTH-1:0] a4_data_delayed_1;
reg [`DWIDTH-1:0] a4_data_delayed_2;
reg [`DWIDTH-1:0] a4_data_delayed_3;
reg [`DWIDTH-1:0] a4_data_delayed_4;
reg [`DWIDTH-1:0] a5_data_delayed_1;
reg [`DWIDTH-1:0] a5_data_delayed_2;
reg [`DWIDTH-1:0] a5_data_delayed_3;
reg [`DWIDTH-1:0] a5_data_delayed_4;
reg [`DWIDTH-1:0] a5_data_delayed_5;
reg [`DWIDTH-1:0] a6_data_delayed_1;
reg [`DWIDTH-1:0] a6_data_delayed_2;
reg [`DWIDTH-1:0] a6_data_delayed_3;
reg [`DWIDTH-1:0] a6_data_delayed_4;
reg [`DWIDTH-1:0] a6_data_delayed_5;
reg [`DWIDTH-1:0] a6_data_delayed_6;
reg [`DWIDTH-1:0] a7_data_delayed_1;
reg [`DWIDTH-1:0] a7_data_delayed_2;
reg [`DWIDTH-1:0] a7_data_delayed_3;
reg [`DWIDTH-1:0] a7_data_delayed_4;
reg [`DWIDTH-1:0] a7_data_delayed_5;
reg [`DWIDTH-1:0] a7_data_delayed_6;
reg [`DWIDTH-1:0] a7_data_delayed_7;


always @(posedge clk) begin
  if (reset || ~start_mat_mul || clk_cnt==0) begin
    a1_data_delayed_1 <= 0;
    a2_data_delayed_1 <= 0;
    a2_data_delayed_2 <= 0;
    a3_data_delayed_1 <= 0;
    a3_data_delayed_2 <= 0;
    a3_data_delayed_3 <= 0;
    a4_data_delayed_1 <= 0;
    a4_data_delayed_2 <= 0;
    a4_data_delayed_3 <= 0;
    a4_data_delayed_4 <= 0;
    a5_data_delayed_1 <= 0;
    a5_data_delayed_2 <= 0;
    a5_data_delayed_3 <= 0;
    a5_data_delayed_4 <= 0;
    a5_data_delayed_5 <= 0;
    a6_data_delayed_1 <= 0;
    a6_data_delayed_2 <= 0;
    a6_data_delayed_3 <= 0;
    a6_data_delayed_4 <= 0;
    a6_data_delayed_5 <= 0;
    a6_data_delayed_6 <= 0;
    a7_data_delayed_1 <= 0;
    a7_data_delayed_2 <= 0;
    a7_data_delayed_3 <= 0;
    a7_data_delayed_4 <= 0;
    a7_data_delayed_5 <= 0;
    a7_data_delayed_6 <= 0;
    a7_data_delayed_7 <= 0;

  end
  else begin
  a1_data_delayed_1 <= a1_data;
  a2_data_delayed_1 <= a2_data;
  a3_data_delayed_1 <= a3_data;
  a4_data_delayed_1 <= a4_data;
  a5_data_delayed_1 <= a5_data;
  a6_data_delayed_1 <= a6_data;
  a7_data_delayed_1 <= a7_data;
  a2_data_delayed_2 <= a2_data_delayed_1;
  a3_data_delayed_2 <= a3_data_delayed_1;
  a3_data_delayed_3 <= a3_data_delayed_2;
  a4_data_delayed_2 <= a4_data_delayed_1;
  a4_data_delayed_3 <= a4_data_delayed_2;
  a4_data_delayed_4 <= a4_data_delayed_3;
  a5_data_delayed_2 <= a5_data_delayed_1;
  a5_data_delayed_3 <= a5_data_delayed_2;
  a5_data_delayed_4 <= a5_data_delayed_3;
  a5_data_delayed_5 <= a5_data_delayed_4;
  a6_data_delayed_2 <= a6_data_delayed_1;
  a6_data_delayed_3 <= a6_data_delayed_2;
  a6_data_delayed_4 <= a6_data_delayed_3;
  a6_data_delayed_5 <= a6_data_delayed_4;
  a6_data_delayed_6 <= a6_data_delayed_5;
  a7_data_delayed_2 <= a7_data_delayed_1;
  a7_data_delayed_3 <= a7_data_delayed_2;
  a7_data_delayed_4 <= a7_data_delayed_3;
  a7_data_delayed_5 <= a7_data_delayed_4;
  a7_data_delayed_6 <= a7_data_delayed_5;
  a7_data_delayed_7 <= a7_data_delayed_6;
 
  end
end

//////////////////////////////////////////////////////////////////////////
// Logic to generate addresses to BRAM B
//////////////////////////////////////////////////////////////////////////
reg [`AWIDTH-1:0] b_addr;
reg b_mem_access; //flag that tells whether the matmul is trying to access memory or not
always @(posedge clk) begin
  //else if (clk_cnt >= b_loc*`MAT_MUL_SIZE+final_mat_mul_size) begin
  //Writing the line above to avoid multiplication:

  if ((reset || ~start_mat_mul) || (clk_cnt >= (b_loc<<`LOG2_MAT_MUL_SIZE)+final_mat_mul_size)) begin

      b_addr <= address_mat_b - address_stride_b;
  
    b_mem_access <= 0;
  end
  //else if ((clk_cnt >= b_loc*`MAT_MUL_SIZE) && (clk_cnt < b_loc*`MAT_MUL_SIZE+final_mat_mul_size)) begin
  //Writing the line above to avoid multiplication:

  else if ((clk_cnt >= (b_loc<<`LOG2_MAT_MUL_SIZE)) && (clk_cnt < (b_loc<<`LOG2_MAT_MUL_SIZE)+final_mat_mul_size)) begin

      b_addr <= b_addr + address_stride_b;
  
    b_mem_access <= 1;
  end
end 

//////////////////////////////////////////////////////////////////////////
// Logic to generate valid signals for data coming from BRAM B
//////////////////////////////////////////////////////////////////////////
reg [7:0] b_mem_access_counter;
always @(posedge clk) begin
  if (reset || ~start_mat_mul) begin
    b_mem_access_counter <= 0;
  end
  else if (b_mem_access == 1) begin
    b_mem_access_counter <= b_mem_access_counter + 1;  
  end
  else begin
    b_mem_access_counter <= 0;
  end
end

wire b_data_valid; //flag that tells whether the data from memory is valid
assign b_data_valid = 
     ((validity_mask_a_cols_b_rows[0]==1'b0 && b_mem_access_counter==1) ||
      (validity_mask_a_cols_b_rows[1]==1'b0 && b_mem_access_counter==2) ||
      (validity_mask_a_cols_b_rows[2]==1'b0 && b_mem_access_counter==3) ||
      (validity_mask_a_cols_b_rows[3]==1'b0 && b_mem_access_counter==4) ||
      (validity_mask_a_cols_b_rows[4]==1'b0 && b_mem_access_counter==5) ||
      (validity_mask_a_cols_b_rows[5]==1'b0 && b_mem_access_counter==6) ||
      (validity_mask_a_cols_b_rows[6]==1'b0 && b_mem_access_counter==7) ||
      (validity_mask_a_cols_b_rows[7]==1'b0 && b_mem_access_counter==8)) ?
    
        1'b0 : (b_mem_access_counter >= `MEM_ACCESS_LATENCY);

//////////////////////////////////////////////////////////////////////////
// Logic to delay certain parts of the data received from BRAM B (systolic data setup)
//////////////////////////////////////////////////////////////////////////
assign b0_data = b_data[1*`DWIDTH-1:0*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[0]}};
assign b1_data = b_data[2*`DWIDTH-1:1*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[1]}};
assign b2_data = b_data[3*`DWIDTH-1:2*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[2]}};
assign b3_data = b_data[4*`DWIDTH-1:3*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[3]}};
assign b4_data = b_data[5*`DWIDTH-1:4*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[4]}};
assign b5_data = b_data[6*`DWIDTH-1:5*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[5]}};
assign b6_data = b_data[7*`DWIDTH-1:6*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[6]}};
assign b7_data = b_data[8*`DWIDTH-1:7*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[7]}};

reg [`DWIDTH-1:0] b1_data_delayed_1;
reg [`DWIDTH-1:0] b2_data_delayed_1;
reg [`DWIDTH-1:0] b2_data_delayed_2;
reg [`DWIDTH-1:0] b3_data_delayed_1;
reg [`DWIDTH-1:0] b3_data_delayed_2;
reg [`DWIDTH-1:0] b3_data_delayed_3;
reg [`DWIDTH-1:0] b4_data_delayed_1;
reg [`DWIDTH-1:0] b4_data_delayed_2;
reg [`DWIDTH-1:0] b4_data_delayed_3;
reg [`DWIDTH-1:0] b4_data_delayed_4;
reg [`DWIDTH-1:0] b5_data_delayed_1;
reg [`DWIDTH-1:0] b5_data_delayed_2;
reg [`DWIDTH-1:0] b5_data_delayed_3;
reg [`DWIDTH-1:0] b5_data_delayed_4;
reg [`DWIDTH-1:0] b5_data_delayed_5;
reg [`DWIDTH-1:0] b6_data_delayed_1;
reg [`DWIDTH-1:0] b6_data_delayed_2;
reg [`DWIDTH-1:0] b6_data_delayed_3;
reg [`DWIDTH-1:0] b6_data_delayed_4;
reg [`DWIDTH-1:0] b6_data_delayed_5;
reg [`DWIDTH-1:0] b6_data_delayed_6;
reg [`DWIDTH-1:0] b7_data_delayed_1;
reg [`DWIDTH-1:0] b7_data_delayed_2;
reg [`DWIDTH-1:0] b7_data_delayed_3;
reg [`DWIDTH-1:0] b7_data_delayed_4;
reg [`DWIDTH-1:0] b7_data_delayed_5;
reg [`DWIDTH-1:0] b7_data_delayed_6;
reg [`DWIDTH-1:0] b7_data_delayed_7;


always @(posedge clk) begin
  if (reset || ~start_mat_mul || clk_cnt==0) begin
    b1_data_delayed_1 <= 0;
    b2_data_delayed_1 <= 0;
    b2_data_delayed_2 <= 0;
    b3_data_delayed_1 <= 0;
    b3_data_delayed_2 <= 0;
    b3_data_delayed_3 <= 0;
    b4_data_delayed_1 <= 0;
    b4_data_delayed_2 <= 0;
    b4_data_delayed_3 <= 0;
    b4_data_delayed_4 <= 0;
    b5_data_delayed_1 <= 0;
    b5_data_delayed_2 <= 0;
    b5_data_delayed_3 <= 0;
    b5_data_delayed_4 <= 0;
    b5_data_delayed_5 <= 0;
    b6_data_delayed_1 <= 0;
    b6_data_delayed_2 <= 0;
    b6_data_delayed_3 <= 0;
    b6_data_delayed_4 <= 0;
    b6_data_delayed_5 <= 0;
    b6_data_delayed_6 <= 0;
    b7_data_delayed_1 <= 0;
    b7_data_delayed_2 <= 0;
    b7_data_delayed_3 <= 0;
    b7_data_delayed_4 <= 0;
    b7_data_delayed_5 <= 0;
    b7_data_delayed_6 <= 0;
    b7_data_delayed_7 <= 0;

  end
  else begin
  b1_data_delayed_1 <= b1_data;
  b2_data_delayed_1 <= b2_data;
  b3_data_delayed_1 <= b3_data;
  b4_data_delayed_1 <= b4_data;
  b5_data_delayed_1 <= b5_data;
  b6_data_delayed_1 <= b6_data;
  b7_data_delayed_1 <= b7_data;
  b2_data_delayed_2 <= b2_data_delayed_1;
  b3_data_delayed_2 <= b3_data_delayed_1;
  b3_data_delayed_3 <= b3_data_delayed_2;
  b4_data_delayed_2 <= b4_data_delayed_1;
  b4_data_delayed_3 <= b4_data_delayed_2;
  b4_data_delayed_4 <= b4_data_delayed_3;
  b5_data_delayed_2 <= b5_data_delayed_1;
  b5_data_delayed_3 <= b5_data_delayed_2;
  b5_data_delayed_4 <= b5_data_delayed_3;
  b5_data_delayed_5 <= b5_data_delayed_4;
  b6_data_delayed_2 <= b6_data_delayed_1;
  b6_data_delayed_3 <= b6_data_delayed_2;
  b6_data_delayed_4 <= b6_data_delayed_3;
  b6_data_delayed_5 <= b6_data_delayed_4;
  b6_data_delayed_6 <= b6_data_delayed_5;
  b7_data_delayed_2 <= b7_data_delayed_1;
  b7_data_delayed_3 <= b7_data_delayed_2;
  b7_data_delayed_4 <= b7_data_delayed_3;
  b7_data_delayed_5 <= b7_data_delayed_4;
  b7_data_delayed_6 <= b7_data_delayed_5;
  b7_data_delayed_7 <= b7_data_delayed_6;
 
  end
end
endmodule


//////////////////////////////////////////////////////////////////////////
// Systolically connected PEs
//////////////////////////////////////////////////////////////////////////
module systolic_pe_matrix_systolic_8x8(
clk,
reset,
pe_reset,
a0,
a1,
a2,
a3,
a4,
a5,
a6,
a7,
b0,
b1,
b2,
b3,
b4,
b5,
b6,
b7,
matrixC0_0,
matrixC0_1,
matrixC0_2,
matrixC0_3,
matrixC0_4,
matrixC0_5,
matrixC0_6,
matrixC0_7,
matrixC1_0,
matrixC1_1,
matrixC1_2,
matrixC1_3,
matrixC1_4,
matrixC1_5,
matrixC1_6,
matrixC1_7,
matrixC2_0,
matrixC2_1,
matrixC2_2,
matrixC2_3,
matrixC2_4,
matrixC2_5,
matrixC2_6,
matrixC2_7,
matrixC3_0,
matrixC3_1,
matrixC3_2,
matrixC3_3,
matrixC3_4,
matrixC3_5,
matrixC3_6,
matrixC3_7,
matrixC4_0,
matrixC4_1,
matrixC4_2,
matrixC4_3,
matrixC4_4,
matrixC4_5,
matrixC4_6,
matrixC4_7,
matrixC5_0,
matrixC5_1,
matrixC5_2,
matrixC5_3,
matrixC5_4,
matrixC5_5,
matrixC5_6,
matrixC5_7,
matrixC6_0,
matrixC6_1,
matrixC6_2,
matrixC6_3,
matrixC6_4,
matrixC6_5,
matrixC6_6,
matrixC6_7,
matrixC7_0,
matrixC7_1,
matrixC7_2,
matrixC7_3,
matrixC7_4,
matrixC7_5,
matrixC7_6,
matrixC7_7,

a_data_out,
b_data_out
);

input clk;
input reset;
input pe_reset;
input [`DWIDTH-1:0] a0;
input [`DWIDTH-1:0] a1;
input [`DWIDTH-1:0] a2;
input [`DWIDTH-1:0] a3;
input [`DWIDTH-1:0] a4;
input [`DWIDTH-1:0] a5;
input [`DWIDTH-1:0] a6;
input [`DWIDTH-1:0] a7;
input [`DWIDTH-1:0] b0;
input [`DWIDTH-1:0] b1;
input [`DWIDTH-1:0] b2;
input [`DWIDTH-1:0] b3;
input [`DWIDTH-1:0] b4;
input [`DWIDTH-1:0] b5;
input [`DWIDTH-1:0] b6;
input [`DWIDTH-1:0] b7;
output [`DWIDTH-1:0] matrixC0_0;
output [`DWIDTH-1:0] matrixC0_1;
output [`DWIDTH-1:0] matrixC0_2;
output [`DWIDTH-1:0] matrixC0_3;
output [`DWIDTH-1:0] matrixC0_4;
output [`DWIDTH-1:0] matrixC0_5;
output [`DWIDTH-1:0] matrixC0_6;
output [`DWIDTH-1:0] matrixC0_7;
output [`DWIDTH-1:0] matrixC1_0;
output [`DWIDTH-1:0] matrixC1_1;
output [`DWIDTH-1:0] matrixC1_2;
output [`DWIDTH-1:0] matrixC1_3;
output [`DWIDTH-1:0] matrixC1_4;
output [`DWIDTH-1:0] matrixC1_5;
output [`DWIDTH-1:0] matrixC1_6;
output [`DWIDTH-1:0] matrixC1_7;
output [`DWIDTH-1:0] matrixC2_0;
output [`DWIDTH-1:0] matrixC2_1;
output [`DWIDTH-1:0] matrixC2_2;
output [`DWIDTH-1:0] matrixC2_3;
output [`DWIDTH-1:0] matrixC2_4;
output [`DWIDTH-1:0] matrixC2_5;
output [`DWIDTH-1:0] matrixC2_6;
output [`DWIDTH-1:0] matrixC2_7;
output [`DWIDTH-1:0] matrixC3_0;
output [`DWIDTH-1:0] matrixC3_1;
output [`DWIDTH-1:0] matrixC3_2;
output [`DWIDTH-1:0] matrixC3_3;
output [`DWIDTH-1:0] matrixC3_4;
output [`DWIDTH-1:0] matrixC3_5;
output [`DWIDTH-1:0] matrixC3_6;
output [`DWIDTH-1:0] matrixC3_7;
output [`DWIDTH-1:0] matrixC4_0;
output [`DWIDTH-1:0] matrixC4_1;
output [`DWIDTH-1:0] matrixC4_2;
output [`DWIDTH-1:0] matrixC4_3;
output [`DWIDTH-1:0] matrixC4_4;
output [`DWIDTH-1:0] matrixC4_5;
output [`DWIDTH-1:0] matrixC4_6;
output [`DWIDTH-1:0] matrixC4_7;
output [`DWIDTH-1:0] matrixC5_0;
output [`DWIDTH-1:0] matrixC5_1;
output [`DWIDTH-1:0] matrixC5_2;
output [`DWIDTH-1:0] matrixC5_3;
output [`DWIDTH-1:0] matrixC5_4;
output [`DWIDTH-1:0] matrixC5_5;
output [`DWIDTH-1:0] matrixC5_6;
output [`DWIDTH-1:0] matrixC5_7;
output [`DWIDTH-1:0] matrixC6_0;
output [`DWIDTH-1:0] matrixC6_1;
output [`DWIDTH-1:0] matrixC6_2;
output [`DWIDTH-1:0] matrixC6_3;
output [`DWIDTH-1:0] matrixC6_4;
output [`DWIDTH-1:0] matrixC6_5;
output [`DWIDTH-1:0] matrixC6_6;
output [`DWIDTH-1:0] matrixC6_7;
output [`DWIDTH-1:0] matrixC7_0;
output [`DWIDTH-1:0] matrixC7_1;
output [`DWIDTH-1:0] matrixC7_2;
output [`DWIDTH-1:0] matrixC7_3;
output [`DWIDTH-1:0] matrixC7_4;
output [`DWIDTH-1:0] matrixC7_5;
output [`DWIDTH-1:0] matrixC7_6;
output [`DWIDTH-1:0] matrixC7_7;

output [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_out;
output [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_out;

wire [`DWIDTH-1:0] a0_0to0_1, a0_1to0_2, a0_2to0_3, a0_3to0_4, a0_4to0_5, a0_5to0_6, a0_6to0_7, a0_7to0_8;
wire [`DWIDTH-1:0] a1_0to1_1, a1_1to1_2, a1_2to1_3, a1_3to1_4, a1_4to1_5, a1_5to1_6, a1_6to1_7, a1_7to1_8;
wire [`DWIDTH-1:0] a2_0to2_1, a2_1to2_2, a2_2to2_3, a2_3to2_4, a2_4to2_5, a2_5to2_6, a2_6to2_7, a2_7to2_8;
wire [`DWIDTH-1:0] a3_0to3_1, a3_1to3_2, a3_2to3_3, a3_3to3_4, a3_4to3_5, a3_5to3_6, a3_6to3_7, a3_7to3_8;
wire [`DWIDTH-1:0] a4_0to4_1, a4_1to4_2, a4_2to4_3, a4_3to4_4, a4_4to4_5, a4_5to4_6, a4_6to4_7, a4_7to4_8;
wire [`DWIDTH-1:0] a5_0to5_1, a5_1to5_2, a5_2to5_3, a5_3to5_4, a5_4to5_5, a5_5to5_6, a5_6to5_7, a5_7to5_8;
wire [`DWIDTH-1:0] a6_0to6_1, a6_1to6_2, a6_2to6_3, a6_3to6_4, a6_4to6_5, a6_5to6_6, a6_6to6_7, a6_7to6_8;
wire [`DWIDTH-1:0] a7_0to7_1, a7_1to7_2, a7_2to7_3, a7_3to7_4, a7_4to7_5, a7_5to7_6, a7_6to7_7, a7_7to7_8;

wire [`DWIDTH-1:0] b0_0to1_0, b1_0to2_0, b2_0to3_0, b3_0to4_0, b4_0to5_0, b5_0to6_0, b6_0to7_0, b7_0to8_0;
wire [`DWIDTH-1:0] b0_1to1_1, b1_1to2_1, b2_1to3_1, b3_1to4_1, b4_1to5_1, b5_1to6_1, b6_1to7_1, b7_1to8_1;
wire [`DWIDTH-1:0] b0_2to1_2, b1_2to2_2, b2_2to3_2, b3_2to4_2, b4_2to5_2, b5_2to6_2, b6_2to7_2, b7_2to8_2;
wire [`DWIDTH-1:0] b0_3to1_3, b1_3to2_3, b2_3to3_3, b3_3to4_3, b4_3to5_3, b5_3to6_3, b6_3to7_3, b7_3to8_3;
wire [`DWIDTH-1:0] b0_4to1_4, b1_4to2_4, b2_4to3_4, b3_4to4_4, b4_4to5_4, b5_4to6_4, b6_4to7_4, b7_4to8_4;
wire [`DWIDTH-1:0] b0_5to1_5, b1_5to2_5, b2_5to3_5, b3_5to4_5, b4_5to5_5, b5_5to6_5, b6_5to7_5, b7_5to8_5;
wire [`DWIDTH-1:0] b0_6to1_6, b1_6to2_6, b2_6to3_6, b3_6to4_6, b4_6to5_6, b5_6to6_6, b6_6to7_6, b7_6to8_6;
wire [`DWIDTH-1:0] b0_7to1_7, b1_7to2_7, b2_7to3_7, b3_7to4_7, b4_7to5_7, b5_7to6_7, b6_7to7_7, b7_7to8_7;

//////////////////////////////////////////////////////////////////////////
// Instantiations of the actual PEs
//////////////////////////////////////////////////////////////////////////
//For larger matmul, more PEs will be needed
wire effective_rst;
assign effective_rst = reset | pe_reset;

processing_element_systolic_8x8 pe0_0(.reset(effective_rst), .clk(clk),  .in_a(a0),      .in_b(b0),  .out_a(a0_0to0_1), .out_b(b0_0to1_0), .out_c(matrixC0_0));
processing_element_systolic_8x8 pe0_1(.reset(effective_rst), .clk(clk),  .in_a(a0_0to0_1), .in_b(b1),  .out_a(a0_1to0_2), .out_b(b0_1to1_1), .out_c(matrixC0_1));
processing_element_systolic_8x8 pe0_2(.reset(effective_rst), .clk(clk),  .in_a(a0_1to0_2), .in_b(b2),  .out_a(a0_2to0_3), .out_b(b0_2to1_2), .out_c(matrixC0_2));
processing_element_systolic_8x8 pe0_3(.reset(effective_rst), .clk(clk),  .in_a(a0_2to0_3), .in_b(b3),  .out_a(a0_3to0_4), .out_b(b0_3to1_3), .out_c(matrixC0_3));
processing_element_systolic_8x8 pe0_4(.reset(effective_rst), .clk(clk),  .in_a(a0_3to0_4), .in_b(b4),  .out_a(a0_4to0_5), .out_b(b0_4to1_4), .out_c(matrixC0_4));
processing_element_systolic_8x8 pe0_5(.reset(effective_rst), .clk(clk),  .in_a(a0_4to0_5), .in_b(b5),  .out_a(a0_5to0_6), .out_b(b0_5to1_5), .out_c(matrixC0_5));
processing_element_systolic_8x8 pe0_6(.reset(effective_rst), .clk(clk),  .in_a(a0_5to0_6), .in_b(b6),  .out_a(a0_6to0_7), .out_b(b0_6to1_6), .out_c(matrixC0_6));
processing_element_systolic_8x8 pe0_7(.reset(effective_rst), .clk(clk),  .in_a(a0_6to0_7), .in_b(b7),  .out_a(a0_7to0_8), .out_b(b0_7to1_7), .out_c(matrixC0_7));

processing_element_systolic_8x8 pe1_0(.reset(effective_rst), .clk(clk),  .in_a(a1), .in_b(b0_0to1_0),  .out_a(a1_0to1_1), .out_b(b1_0to2_0), .out_c(matrixC1_0));
processing_element_systolic_8x8 pe2_0(.reset(effective_rst), .clk(clk),  .in_a(a2), .in_b(b1_0to2_0),  .out_a(a2_0to2_1), .out_b(b2_0to3_0), .out_c(matrixC2_0));
processing_element_systolic_8x8 pe3_0(.reset(effective_rst), .clk(clk),  .in_a(a3), .in_b(b2_0to3_0),  .out_a(a3_0to3_1), .out_b(b3_0to4_0), .out_c(matrixC3_0));
processing_element_systolic_8x8 pe4_0(.reset(effective_rst), .clk(clk),  .in_a(a4), .in_b(b3_0to4_0),  .out_a(a4_0to4_1), .out_b(b4_0to5_0), .out_c(matrixC4_0));
processing_element_systolic_8x8 pe5_0(.reset(effective_rst), .clk(clk),  .in_a(a5), .in_b(b4_0to5_0),  .out_a(a5_0to5_1), .out_b(b5_0to6_0), .out_c(matrixC5_0));
processing_element_systolic_8x8 pe6_0(.reset(effective_rst), .clk(clk),  .in_a(a6), .in_b(b5_0to6_0),  .out_a(a6_0to6_1), .out_b(b6_0to7_0), .out_c(matrixC6_0));
processing_element_systolic_8x8 pe7_0(.reset(effective_rst), .clk(clk),  .in_a(a7), .in_b(b6_0to7_0),  .out_a(a7_0to7_1), .out_b(b7_0to8_0), .out_c(matrixC7_0));

processing_element_systolic_8x8 pe1_1(.reset(effective_rst), .clk(clk),  .in_a(a1_0to1_1), .in_b(b0_1to1_1),  .out_a(a1_1to1_2), .out_b(b1_1to2_1), .out_c(matrixC1_1));
processing_element_systolic_8x8 pe1_2(.reset(effective_rst), .clk(clk),  .in_a(a1_1to1_2), .in_b(b0_2to1_2),  .out_a(a1_2to1_3), .out_b(b1_2to2_2), .out_c(matrixC1_2));
processing_element_systolic_8x8 pe1_3(.reset(effective_rst), .clk(clk),  .in_a(a1_2to1_3), .in_b(b0_3to1_3),  .out_a(a1_3to1_4), .out_b(b1_3to2_3), .out_c(matrixC1_3));
processing_element_systolic_8x8 pe1_4(.reset(effective_rst), .clk(clk),  .in_a(a1_3to1_4), .in_b(b0_4to1_4),  .out_a(a1_4to1_5), .out_b(b1_4to2_4), .out_c(matrixC1_4));
processing_element_systolic_8x8 pe1_5(.reset(effective_rst), .clk(clk),  .in_a(a1_4to1_5), .in_b(b0_5to1_5),  .out_a(a1_5to1_6), .out_b(b1_5to2_5), .out_c(matrixC1_5));
processing_element_systolic_8x8 pe1_6(.reset(effective_rst), .clk(clk),  .in_a(a1_5to1_6), .in_b(b0_6to1_6),  .out_a(a1_6to1_7), .out_b(b1_6to2_6), .out_c(matrixC1_6));
processing_element_systolic_8x8 pe1_7(.reset(effective_rst), .clk(clk),  .in_a(a1_6to1_7), .in_b(b0_7to1_7),  .out_a(a1_7to1_8), .out_b(b1_7to2_7), .out_c(matrixC1_7));
processing_element_systolic_8x8 pe2_1(.reset(effective_rst), .clk(clk),  .in_a(a2_0to2_1), .in_b(b1_1to2_1),  .out_a(a2_1to2_2), .out_b(b2_1to3_1), .out_c(matrixC2_1));
processing_element_systolic_8x8 pe2_2(.reset(effective_rst), .clk(clk),  .in_a(a2_1to2_2), .in_b(b1_2to2_2),  .out_a(a2_2to2_3), .out_b(b2_2to3_2), .out_c(matrixC2_2));
processing_element_systolic_8x8 pe2_3(.reset(effective_rst), .clk(clk),  .in_a(a2_2to2_3), .in_b(b1_3to2_3),  .out_a(a2_3to2_4), .out_b(b2_3to3_3), .out_c(matrixC2_3));
processing_element_systolic_8x8 pe2_4(.reset(effective_rst), .clk(clk),  .in_a(a2_3to2_4), .in_b(b1_4to2_4),  .out_a(a2_4to2_5), .out_b(b2_4to3_4), .out_c(matrixC2_4));
processing_element_systolic_8x8 pe2_5(.reset(effective_rst), .clk(clk),  .in_a(a2_4to2_5), .in_b(b1_5to2_5),  .out_a(a2_5to2_6), .out_b(b2_5to3_5), .out_c(matrixC2_5));
processing_element_systolic_8x8 pe2_6(.reset(effective_rst), .clk(clk),  .in_a(a2_5to2_6), .in_b(b1_6to2_6),  .out_a(a2_6to2_7), .out_b(b2_6to3_6), .out_c(matrixC2_6));
processing_element_systolic_8x8 pe2_7(.reset(effective_rst), .clk(clk),  .in_a(a2_6to2_7), .in_b(b1_7to2_7),  .out_a(a2_7to2_8), .out_b(b2_7to3_7), .out_c(matrixC2_7));
processing_element_systolic_8x8 pe3_1(.reset(effective_rst), .clk(clk),  .in_a(a3_0to3_1), .in_b(b2_1to3_1),  .out_a(a3_1to3_2), .out_b(b3_1to4_1), .out_c(matrixC3_1));
processing_element_systolic_8x8 pe3_2(.reset(effective_rst), .clk(clk),  .in_a(a3_1to3_2), .in_b(b2_2to3_2),  .out_a(a3_2to3_3), .out_b(b3_2to4_2), .out_c(matrixC3_2));
processing_element_systolic_8x8 pe3_3(.reset(effective_rst), .clk(clk),  .in_a(a3_2to3_3), .in_b(b2_3to3_3),  .out_a(a3_3to3_4), .out_b(b3_3to4_3), .out_c(matrixC3_3));
processing_element_systolic_8x8 pe3_4(.reset(effective_rst), .clk(clk),  .in_a(a3_3to3_4), .in_b(b2_4to3_4),  .out_a(a3_4to3_5), .out_b(b3_4to4_4), .out_c(matrixC3_4));
processing_element_systolic_8x8 pe3_5(.reset(effective_rst), .clk(clk),  .in_a(a3_4to3_5), .in_b(b2_5to3_5),  .out_a(a3_5to3_6), .out_b(b3_5to4_5), .out_c(matrixC3_5));
processing_element_systolic_8x8 pe3_6(.reset(effective_rst), .clk(clk),  .in_a(a3_5to3_6), .in_b(b2_6to3_6),  .out_a(a3_6to3_7), .out_b(b3_6to4_6), .out_c(matrixC3_6));
processing_element_systolic_8x8 pe3_7(.reset(effective_rst), .clk(clk),  .in_a(a3_6to3_7), .in_b(b2_7to3_7),  .out_a(a3_7to3_8), .out_b(b3_7to4_7), .out_c(matrixC3_7));
processing_element_systolic_8x8 pe4_1(.reset(effective_rst), .clk(clk),  .in_a(a4_0to4_1), .in_b(b3_1to4_1),  .out_a(a4_1to4_2), .out_b(b4_1to5_1), .out_c(matrixC4_1));
processing_element_systolic_8x8 pe4_2(.reset(effective_rst), .clk(clk),  .in_a(a4_1to4_2), .in_b(b3_2to4_2),  .out_a(a4_2to4_3), .out_b(b4_2to5_2), .out_c(matrixC4_2));
processing_element_systolic_8x8 pe4_3(.reset(effective_rst), .clk(clk),  .in_a(a4_2to4_3), .in_b(b3_3to4_3),  .out_a(a4_3to4_4), .out_b(b4_3to5_3), .out_c(matrixC4_3));
processing_element_systolic_8x8 pe4_4(.reset(effective_rst), .clk(clk),  .in_a(a4_3to4_4), .in_b(b3_4to4_4),  .out_a(a4_4to4_5), .out_b(b4_4to5_4), .out_c(matrixC4_4));
processing_element_systolic_8x8 pe4_5(.reset(effective_rst), .clk(clk),  .in_a(a4_4to4_5), .in_b(b3_5to4_5),  .out_a(a4_5to4_6), .out_b(b4_5to5_5), .out_c(matrixC4_5));
processing_element_systolic_8x8 pe4_6(.reset(effective_rst), .clk(clk),  .in_a(a4_5to4_6), .in_b(b3_6to4_6),  .out_a(a4_6to4_7), .out_b(b4_6to5_6), .out_c(matrixC4_6));
processing_element_systolic_8x8 pe4_7(.reset(effective_rst), .clk(clk),  .in_a(a4_6to4_7), .in_b(b3_7to4_7),  .out_a(a4_7to4_8), .out_b(b4_7to5_7), .out_c(matrixC4_7));
processing_element_systolic_8x8 pe5_1(.reset(effective_rst), .clk(clk),  .in_a(a5_0to5_1), .in_b(b4_1to5_1),  .out_a(a5_1to5_2), .out_b(b5_1to6_1), .out_c(matrixC5_1));
processing_element_systolic_8x8 pe5_2(.reset(effective_rst), .clk(clk),  .in_a(a5_1to5_2), .in_b(b4_2to5_2),  .out_a(a5_2to5_3), .out_b(b5_2to6_2), .out_c(matrixC5_2));
processing_element_systolic_8x8 pe5_3(.reset(effective_rst), .clk(clk),  .in_a(a5_2to5_3), .in_b(b4_3to5_3),  .out_a(a5_3to5_4), .out_b(b5_3to6_3), .out_c(matrixC5_3));
processing_element_systolic_8x8 pe5_4(.reset(effective_rst), .clk(clk),  .in_a(a5_3to5_4), .in_b(b4_4to5_4),  .out_a(a5_4to5_5), .out_b(b5_4to6_4), .out_c(matrixC5_4));
processing_element_systolic_8x8 pe5_5(.reset(effective_rst), .clk(clk),  .in_a(a5_4to5_5), .in_b(b4_5to5_5),  .out_a(a5_5to5_6), .out_b(b5_5to6_5), .out_c(matrixC5_5));
processing_element_systolic_8x8 pe5_6(.reset(effective_rst), .clk(clk),  .in_a(a5_5to5_6), .in_b(b4_6to5_6),  .out_a(a5_6to5_7), .out_b(b5_6to6_6), .out_c(matrixC5_6));
processing_element_systolic_8x8 pe5_7(.reset(effective_rst), .clk(clk),  .in_a(a5_6to5_7), .in_b(b4_7to5_7),  .out_a(a5_7to5_8), .out_b(b5_7to6_7), .out_c(matrixC5_7));
processing_element_systolic_8x8 pe6_1(.reset(effective_rst), .clk(clk),  .in_a(a6_0to6_1), .in_b(b5_1to6_1),  .out_a(a6_1to6_2), .out_b(b6_1to7_1), .out_c(matrixC6_1));
processing_element_systolic_8x8 pe6_2(.reset(effective_rst), .clk(clk),  .in_a(a6_1to6_2), .in_b(b5_2to6_2),  .out_a(a6_2to6_3), .out_b(b6_2to7_2), .out_c(matrixC6_2));
processing_element_systolic_8x8 pe6_3(.reset(effective_rst), .clk(clk),  .in_a(a6_2to6_3), .in_b(b5_3to6_3),  .out_a(a6_3to6_4), .out_b(b6_3to7_3), .out_c(matrixC6_3));
processing_element_systolic_8x8 pe6_4(.reset(effective_rst), .clk(clk),  .in_a(a6_3to6_4), .in_b(b5_4to6_4),  .out_a(a6_4to6_5), .out_b(b6_4to7_4), .out_c(matrixC6_4));
processing_element_systolic_8x8 pe6_5(.reset(effective_rst), .clk(clk),  .in_a(a6_4to6_5), .in_b(b5_5to6_5),  .out_a(a6_5to6_6), .out_b(b6_5to7_5), .out_c(matrixC6_5));
processing_element_systolic_8x8 pe6_6(.reset(effective_rst), .clk(clk),  .in_a(a6_5to6_6), .in_b(b5_6to6_6),  .out_a(a6_6to6_7), .out_b(b6_6to7_6), .out_c(matrixC6_6));
processing_element_systolic_8x8 pe6_7(.reset(effective_rst), .clk(clk),  .in_a(a6_6to6_7), .in_b(b5_7to6_7),  .out_a(a6_7to6_8), .out_b(b6_7to7_7), .out_c(matrixC6_7));
processing_element_systolic_8x8 pe7_1(.reset(effective_rst), .clk(clk),  .in_a(a7_0to7_1), .in_b(b6_1to7_1),  .out_a(a7_1to7_2), .out_b(b7_1to8_1), .out_c(matrixC7_1));
processing_element_systolic_8x8 pe7_2(.reset(effective_rst), .clk(clk),  .in_a(a7_1to7_2), .in_b(b6_2to7_2),  .out_a(a7_2to7_3), .out_b(b7_2to8_2), .out_c(matrixC7_2));
processing_element_systolic_8x8 pe7_3(.reset(effective_rst), .clk(clk),  .in_a(a7_2to7_3), .in_b(b6_3to7_3),  .out_a(a7_3to7_4), .out_b(b7_3to8_3), .out_c(matrixC7_3));
processing_element_systolic_8x8 pe7_4(.reset(effective_rst), .clk(clk),  .in_a(a7_3to7_4), .in_b(b6_4to7_4),  .out_a(a7_4to7_5), .out_b(b7_4to8_4), .out_c(matrixC7_4));
processing_element_systolic_8x8 pe7_5(.reset(effective_rst), .clk(clk),  .in_a(a7_4to7_5), .in_b(b6_5to7_5),  .out_a(a7_5to7_6), .out_b(b7_5to8_5), .out_c(matrixC7_5));
processing_element_systolic_8x8 pe7_6(.reset(effective_rst), .clk(clk),  .in_a(a7_5to7_6), .in_b(b6_6to7_6),  .out_a(a7_6to7_7), .out_b(b7_6to8_6), .out_c(matrixC7_6));
processing_element_systolic_8x8 pe7_7(.reset(effective_rst), .clk(clk),  .in_a(a7_6to7_7), .in_b(b6_7to7_7),  .out_a(a7_7to7_8), .out_b(b7_7to8_7), .out_c(matrixC7_7));
assign a_data_out = {a7_7to7_8,a6_7to6_8,a5_7to5_8,a4_7to4_8,a3_7to3_8,a2_7to2_8,a1_7to1_8,a0_7to0_8};
assign b_data_out = {b7_7to8_7,b7_6to8_6,b7_5to8_5,b7_4to8_4,b7_3to8_3,b7_2to8_2,b7_1to8_1,b7_0to8_0};

endmodule

module processing_element_systolic_8x8(
 reset, 
 clk, 
 in_a,
 in_b, 
 out_a, 
 out_b, 
 out_c
 );

 input reset;
 input clk;
 input  [`DWIDTH-1:0] in_a;
 input  [`DWIDTH-1:0] in_b;
 output [`DWIDTH-1:0] out_a;
 output [`DWIDTH-1:0] out_b;
 output [`DWIDTH-1:0] out_c;  //reduced precision

 reg [`DWIDTH-1:0] out_a;
 reg [`DWIDTH-1:0] out_b;
 wire [`DWIDTH-1:0] out_c;

 wire [`DWIDTH-1:0] out_mac;

 assign out_c = out_mac;

 seq_mac_systolic_8x8 u_mac(.a(in_a), .b(in_b), .out(out_mac), .reset(reset), .clk(clk));

 always @(posedge clk)begin
    if(reset) begin
      out_a<=0;
      out_b<=0;
    end
    else begin  
      out_a<=in_a;
      out_b<=in_b;
    end
 end
 
endmodule

module seq_mac_systolic_8x8(a, b, out, reset, clk);
input [`DWIDTH-1:0] a;
input [`DWIDTH-1:0] b;
input reset;
input clk;
output [`DWIDTH-1:0] out;


reg [2*`DWIDTH-1:0] out_temp;
wire [`DWIDTH-1:0] mul_out;
wire [2*`DWIDTH-1:0] add_out;

reg [`DWIDTH-1:0] a_flopped;
reg [`DWIDTH-1:0] b_flopped;

wire [2*`DWIDTH-1:0] mul_out_temp;
reg [2*`DWIDTH-1:0] mul_out_temp_reg;

always @(posedge clk) begin
  if (reset) begin
    a_flopped <= 0;
    b_flopped <= 0;
  end else begin
    a_flopped <= a;
    b_flopped <= b;
  end
end

//assign mul_out = a * b;
qmult_systolic_8x8 mult_u1(.i_multiplicand(a_flopped), .i_multiplier(b_flopped), .o_result(mul_out_temp));

always @(posedge clk) begin
  if (reset) begin
    mul_out_temp_reg <= 0;
  end else begin
    mul_out_temp_reg <= mul_out_temp;
  end
end

//we just truncate the higher bits of the product
//assign add_out = mul_out + out;
qadd_systolic_8x8 add_u1(.a(out_temp), .b(mul_out_temp_reg), .c(add_out));

always @(posedge clk) begin
  if (reset) begin
    out_temp <= 0;
  end else begin
    out_temp <= add_out;
  end
end

//down cast the result
assign out = 
    (out_temp[2*`DWIDTH-1] == 0) ?  //positive number
        (
           (|(out_temp[2*`DWIDTH-2 : `DWIDTH-1])) ?  //is any bit from 14:7 is 1, that means overlfow
             {out_temp[2*`DWIDTH-1] , {(`DWIDTH-1){1'b1}}} : //sign bit and then all 1s
             {out_temp[2*`DWIDTH-1] , out_temp[`DWIDTH-2:0]} 
        )
        : //negative number
        (
           (|(out_temp[2*`DWIDTH-2 : `DWIDTH-1])) ?  //is any bit from 14:7 is 0, that means overlfow
             {out_temp[2*`DWIDTH-1] , out_temp[`DWIDTH-2:0]} :
             {out_temp[2*`DWIDTH-1] , {(`DWIDTH-1){1'b0}}} //sign bit and then all 0s
        );


endmodule

module qmult_systolic_8x8(i_multiplicand,i_multiplier,o_result);
input [`DWIDTH-1:0] i_multiplicand;
input [`DWIDTH-1:0] i_multiplier;
output [2*`DWIDTH-1:0] o_result;

assign o_result = i_multiplicand * i_multiplier;
//DW02_mult #(`DWIDTH,`DWIDTH) u_mult(.A(i_multiplicand), .B(i_multiplier), .TC(1'b1), .PRODUCT(o_result));

endmodule

module qadd_systolic_8x8(a,b,c);
input [2*`DWIDTH-1:0] a;
input [2*`DWIDTH-1:0] b;
output [2*`DWIDTH-1:0] c;

assign c = a + b;
//DW01_add #(`DWIDTH) u_add(.A(a), .B(b), .CI(1'b0), .SUM(c), .CO());
endmodule
`define DWIDTH 8
`define DESIGN_SIZE 32
`define MASK_WIDTH 2

module activation_32_8bit(
    input activation_type,
    input enable_activation,
    input in_data_available,
    input [`DESIGN_SIZE*`DWIDTH-1:0] inp_data,
    output [`DESIGN_SIZE*`DWIDTH-1:0] out_data,
    output out_data_available,
    input [`MASK_WIDTH-1:0] validity_mask,
    output done_activation,
    input clk,
    input reset
);

reg  done_activation_internal;
reg  out_data_available_internal;
wire [`DESIGN_SIZE*`DWIDTH-1:0] out_data_internal;
reg [`DESIGN_SIZE*`DWIDTH-1:0] slope_applied_data_internal;
reg [`DESIGN_SIZE*`DWIDTH-1:0] intercept_applied_data_internal;
reg [`DESIGN_SIZE*`DWIDTH-1:0] relu_applied_data_internal;
reg [31:0] i;
reg [31:0] cycle_count;
reg activation_in_progress;

reg [(`DESIGN_SIZE*4)-1:0] address;
reg [(`DESIGN_SIZE*8)-1:0] data_slope;
reg [(`DESIGN_SIZE*8)-1:0] data_slope_flopped;
reg [(`DESIGN_SIZE*8)-1:0] data_intercept;
reg [(`DESIGN_SIZE*8)-1:0] data_intercept_delayed;
reg [(`DESIGN_SIZE*8)-1:0] data_intercept_flopped;

reg in_data_available_flopped;
reg [`DESIGN_SIZE*`DWIDTH-1:0] inp_data_flopped;

always @(posedge clk) begin
  if (reset) begin
    inp_data_flopped <= 0;
    data_slope_flopped <= 0;
  end else begin
    inp_data_flopped <= inp_data;
    data_slope_flopped <= data_slope;
  end
end

// If the activation block is not enabled, just forward the input data
assign out_data             = enable_activation ? out_data_internal : inp_data_flopped;
assign done_activation      = enable_activation ? done_activation_internal : 1'b1;
assign out_data_available   = enable_activation ? out_data_available_internal : in_data_available_flopped;

always @(posedge clk) begin
   if (reset || ~enable_activation) begin
      slope_applied_data_internal     <= 0;
      intercept_applied_data_internal <= 0; 
      relu_applied_data_internal      <= 0; 
      data_intercept_delayed      <= 0;
      data_intercept_flopped      <= 0;
      done_activation_internal    <= 0;
      out_data_available_internal <= 0;
      cycle_count                 <= 0;
      activation_in_progress      <= 0;
      in_data_available_flopped <= in_data_available;
   end else if(in_data_available || activation_in_progress) begin
      cycle_count <= cycle_count + 1;

      for (i = 0; i < `DESIGN_SIZE; i=i+1) begin
         if(activation_type==1'b1) begin // tanH
            slope_applied_data_internal[i*`DWIDTH +:`DWIDTH] <= data_slope_flopped[i*8 +: 8] * inp_data_flopped[i*`DWIDTH +:`DWIDTH];
            data_intercept_flopped[i*8 +: 8] <= data_intercept[i*8 +: 8];
            data_intercept_delayed[i*8 +: 8] <= data_intercept_flopped[i*8 +: 8];
            intercept_applied_data_internal[i*`DWIDTH +:`DWIDTH] <= slope_applied_data_internal[i*`DWIDTH +:`DWIDTH] + data_intercept_delayed[i*8 +: 8];
         end else begin // ReLU
            relu_applied_data_internal[i*`DWIDTH +:`DWIDTH] <= inp_data[i*`DWIDTH] ? {`DWIDTH{1'b0}} : inp_data[i*`DWIDTH +:`DWIDTH];
         end
      end   

      //TANH needs 1 extra cycle
      if (activation_type==1'b1) begin
         if (cycle_count==3) begin
            out_data_available_internal <= 1;
         end
      end else begin
         if (cycle_count==2) begin
           out_data_available_internal <= 1;
         end
      end

      //TANH needs 1 extra cycle
      if (activation_type==1'b1) begin
        if(cycle_count==(`DESIGN_SIZE+2)) begin
           done_activation_internal <= 1'b1;
           activation_in_progress <= 0;
        end
        else begin
           activation_in_progress <= 1;
        end
      end else begin
        if(cycle_count==(`DESIGN_SIZE+1)) begin
           done_activation_internal <= 1'b1;
           activation_in_progress <= 0;
        end
        else begin
           activation_in_progress <= 1;
        end
      end
   end
   else begin
      slope_applied_data_internal     <= 0;
      intercept_applied_data_internal <= 0; 
      relu_applied_data_internal      <= 0; 
      data_intercept_delayed      <= 0;
      data_intercept_flopped      <= 0;
      done_activation_internal    <= 0;
      out_data_available_internal <= 0;
      cycle_count                 <= 0;
      activation_in_progress      <= 0;
   end
end

assign out_data_internal = (activation_type) ? intercept_applied_data_internal : relu_applied_data_internal;

//Our equation of tanh is Y=AX+B
//A is the slope and B is the intercept.
//We store A in one LUT and B in another.
//LUT for the slope
always @(address) begin
    for (i = 0; i < `DESIGN_SIZE; i=i+1) begin
    case (address[i*4+:4])
      4'b0000: data_slope[i*8+:8] = 8'd0;
      4'b0001: data_slope[i*8+:8] = 8'd0;
      4'b0010: data_slope[i*8+:8] = 8'd2;
      4'b0011: data_slope[i*8+:8] = 8'd3;
      4'b0100: data_slope[i*8+:8] = 8'd4;
      4'b0101: data_slope[i*8+:8] = 8'd0;
      4'b0110: data_slope[i*8+:8] = 8'd4;
      4'b0111: data_slope[i*8+:8] = 8'd3;
      4'b1000: data_slope[i*8+:8] = 8'd2;
      4'b1001: data_slope[i*8+:8] = 8'd0;
      4'b1010: data_slope[i*8+:8] = 8'd0;
      default: data_slope[i*8+:8] = 8'd0;
    endcase  
    end
end

//LUT for the intercept
always @(address) begin
    for (i = 0; i < `DESIGN_SIZE; i=i+1) begin
    case (address[i*4+:4])
      4'b0000: data_intercept[i*8+:8] = 8'd127;
      4'b0001: data_intercept[i*8+:8] = 8'd99;
      4'b0010: data_intercept[i*8+:8] = 8'd46;
      4'b0011: data_intercept[i*8+:8] = 8'd18;
      4'b0100: data_intercept[i*8+:8] = 8'd0;
      4'b0101: data_intercept[i*8+:8] = 8'd0;
      4'b0110: data_intercept[i*8+:8] = 8'd0;
      4'b0111: data_intercept[i*8+:8] = -8'd18;
      4'b1000: data_intercept[i*8+:8] = -8'd46;
      4'b1001: data_intercept[i*8+:8] = -8'd99;
      4'b1010: data_intercept[i*8+:8] = -8'd127;
      default: data_intercept[i*8+:8] = 8'd0;
    endcase  
    end
end

//Logic to find address
always @(inp_data) begin
    for (i = 0; i < `DESIGN_SIZE; i=i+1) begin
        if((inp_data[i*`DWIDTH +:`DWIDTH])>=90) begin
           address[i*4+:4] = 4'b0000;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>=39 && (inp_data[i*`DWIDTH +:`DWIDTH])<90) begin
           address[i*4+:4] = 4'b0001;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>=28 && (inp_data[i*`DWIDTH +:`DWIDTH])<39) begin
           address[i*4+:4] = 4'b0010;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>=16 && (inp_data[i*`DWIDTH +:`DWIDTH])<28) begin
           address[i*4+:4] = 4'b0011;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>=1 && (inp_data[i*`DWIDTH +:`DWIDTH])<16) begin
           address[i*4+:4] = 4'b0100;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])==0) begin
           address[i*4+:4] = 4'b0101;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>-16 && (inp_data[i*`DWIDTH +:`DWIDTH])<=-1) begin
           address[i*4+:4] = 4'b0110;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>-28 && (inp_data[i*`DWIDTH +:`DWIDTH])<=-16) begin
           address[i*4+:4] = 4'b0111;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>-39 && (inp_data[i*`DWIDTH +:`DWIDTH])<=-28) begin
           address[i*4+:4] = 4'b1000;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>-90 && (inp_data[i*`DWIDTH +:`DWIDTH])<=-39) begin
           address[i*4+:4] = 4'b1001;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])<=-90) begin
           address[i*4+:4] = 4'b1010;
        end
        else begin
           address[i*4+:4] = 4'b0101;
        end
    end
end

//Adding a dummy signal to use validity_mask input, to make ODIN happy
//TODO: Need to correctly use validity_mask
wire [`MASK_WIDTH-1:0] dummy;
assign dummy = validity_mask;

endmodule
`define DWIDTH 16
`define DESIGN_SIZE 32
`define MASK_WIDTH 2

module activation_32_16bit(
    input activation_type,
    input enable_activation,
    input in_data_available,
    input [`DESIGN_SIZE*`DWIDTH-1:0] inp_data,
    output [`DESIGN_SIZE*`DWIDTH-1:0] out_data,
    output out_data_available,
    input [`MASK_WIDTH-1:0] validity_mask,
    output done_activation,
    input clk,
    input reset
);

reg  done_activation_internal;
reg  out_data_available_internal;
wire [`DESIGN_SIZE*`DWIDTH-1:0] out_data_internal;
reg [`DESIGN_SIZE*`DWIDTH-1:0] slope_applied_data_internal;
reg [`DESIGN_SIZE*`DWIDTH-1:0] intercept_applied_data_internal;
reg [`DESIGN_SIZE*`DWIDTH-1:0] relu_applied_data_internal;
reg [31:0] i;
reg [31:0] cycle_count;
reg activation_in_progress;

reg [(`DESIGN_SIZE*4)-1:0] address;
reg [(`DESIGN_SIZE*8)-1:0] data_slope;
reg [(`DESIGN_SIZE*8)-1:0] data_slope_flopped;
reg [(`DESIGN_SIZE*8)-1:0] data_intercept;
reg [(`DESIGN_SIZE*8)-1:0] data_intercept_delayed;
reg [(`DESIGN_SIZE*8)-1:0] data_intercept_flopped;

reg in_data_available_flopped;
reg [`DESIGN_SIZE*`DWIDTH-1:0] inp_data_flopped;

always @(posedge clk) begin
  if (reset) begin
    inp_data_flopped <= 0;
    data_slope_flopped <= 0;
  end else begin
    inp_data_flopped <= inp_data;
    data_slope_flopped <= data_slope;
  end
end

// If the activation block is not enabled, just forward the input data
assign out_data             = enable_activation ? out_data_internal : inp_data_flopped;
assign done_activation      = enable_activation ? done_activation_internal : 1'b1;
assign out_data_available   = enable_activation ? out_data_available_internal : in_data_available_flopped;

always @(posedge clk) begin
   if (reset || ~enable_activation) begin
      slope_applied_data_internal     <= 0;
      intercept_applied_data_internal <= 0; 
      relu_applied_data_internal      <= 0; 
      data_intercept_delayed      <= 0;
      data_intercept_flopped      <= 0;
      done_activation_internal    <= 0;
      out_data_available_internal <= 0;
      cycle_count                 <= 0;
      activation_in_progress      <= 0;
      in_data_available_flopped <= in_data_available;
   end else if(in_data_available || activation_in_progress) begin
      cycle_count <= cycle_count + 1;

      for (i = 0; i < `DESIGN_SIZE; i=i+1) begin
         if(activation_type==1'b1) begin // tanH
            slope_applied_data_internal[i*`DWIDTH +:`DWIDTH] <= data_slope_flopped[i*8 +: 8] * inp_data_flopped[i*`DWIDTH +:`DWIDTH];
            data_intercept_flopped[i*8 +: 8] <= data_intercept[i*8 +: 8];
            data_intercept_delayed[i*8 +: 8] <= data_intercept_flopped[i*8 +: 8];
            intercept_applied_data_internal[i*`DWIDTH +:`DWIDTH] <= slope_applied_data_internal[i*`DWIDTH +:`DWIDTH] + data_intercept_delayed[i*8 +: 8];
         end else begin // ReLU
            relu_applied_data_internal[i*`DWIDTH +:`DWIDTH] <= inp_data[i*`DWIDTH] ? {`DWIDTH{1'b0}} : inp_data[i*`DWIDTH +:`DWIDTH];
         end
      end   

      //TANH needs 1 extra cycle
      if (activation_type==1'b1) begin
         if (cycle_count==3) begin
            out_data_available_internal <= 1;
         end
      end else begin
         if (cycle_count==2) begin
           out_data_available_internal <= 1;
         end
      end

      //TANH needs 1 extra cycle
      if (activation_type==1'b1) begin
        if(cycle_count==(`DESIGN_SIZE+2)) begin
           done_activation_internal <= 1'b1;
           activation_in_progress <= 0;
        end
        else begin
           activation_in_progress <= 1;
        end
      end else begin
        if(cycle_count==(`DESIGN_SIZE+1)) begin
           done_activation_internal <= 1'b1;
           activation_in_progress <= 0;
        end
        else begin
           activation_in_progress <= 1;
        end
      end
   end
   else begin
      slope_applied_data_internal     <= 0;
      intercept_applied_data_internal <= 0; 
      relu_applied_data_internal      <= 0; 
      data_intercept_delayed      <= 0;
      data_intercept_flopped      <= 0;
      done_activation_internal    <= 0;
      out_data_available_internal <= 0;
      cycle_count                 <= 0;
      activation_in_progress      <= 0;
   end
end

assign out_data_internal = (activation_type) ? intercept_applied_data_internal : relu_applied_data_internal;

//Our equation of tanh is Y=AX+B
//A is the slope and B is the intercept.
//We store A in one LUT and B in another.
//LUT for the slope
always @(address) begin
    for (i = 0; i < `DESIGN_SIZE; i=i+1) begin
    case (address[i*4+:4])
      4'b0000: data_slope[i*8+:8] = 8'd0;
      4'b0001: data_slope[i*8+:8] = 8'd0;
      4'b0010: data_slope[i*8+:8] = 8'd2;
      4'b0011: data_slope[i*8+:8] = 8'd3;
      4'b0100: data_slope[i*8+:8] = 8'd4;
      4'b0101: data_slope[i*8+:8] = 8'd0;
      4'b0110: data_slope[i*8+:8] = 8'd4;
      4'b0111: data_slope[i*8+:8] = 8'd3;
      4'b1000: data_slope[i*8+:8] = 8'd2;
      4'b1001: data_slope[i*8+:8] = 8'd0;
      4'b1010: data_slope[i*8+:8] = 8'd0;
      default: data_slope[i*8+:8] = 8'd0;
    endcase  
    end
end

//LUT for the intercept
always @(address) begin
    for (i = 0; i < `DESIGN_SIZE; i=i+1) begin
    case (address[i*4+:4])
      4'b0000: data_intercept[i*8+:8] = 8'd127;
      4'b0001: data_intercept[i*8+:8] = 8'd99;
      4'b0010: data_intercept[i*8+:8] = 8'd46;
      4'b0011: data_intercept[i*8+:8] = 8'd18;
      4'b0100: data_intercept[i*8+:8] = 8'd0;
      4'b0101: data_intercept[i*8+:8] = 8'd0;
      4'b0110: data_intercept[i*8+:8] = 8'd0;
      4'b0111: data_intercept[i*8+:8] = -8'd18;
      4'b1000: data_intercept[i*8+:8] = -8'd46;
      4'b1001: data_intercept[i*8+:8] = -8'd99;
      4'b1010: data_intercept[i*8+:8] = -8'd127;
      default: data_intercept[i*8+:8] = 8'd0;
    endcase  
    end
end

//Logic to find address
always @(inp_data) begin
    for (i = 0; i < `DESIGN_SIZE; i=i+1) begin
        if((inp_data[i*`DWIDTH +:`DWIDTH])>=90) begin
           address[i*4+:4] = 4'b0000;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>=39 && (inp_data[i*`DWIDTH +:`DWIDTH])<90) begin
           address[i*4+:4] = 4'b0001;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>=28 && (inp_data[i*`DWIDTH +:`DWIDTH])<39) begin
           address[i*4+:4] = 4'b0010;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>=16 && (inp_data[i*`DWIDTH +:`DWIDTH])<28) begin
           address[i*4+:4] = 4'b0011;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>=1 && (inp_data[i*`DWIDTH +:`DWIDTH])<16) begin
           address[i*4+:4] = 4'b0100;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])==0) begin
           address[i*4+:4] = 4'b0101;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>-16 && (inp_data[i*`DWIDTH +:`DWIDTH])<=-1) begin
           address[i*4+:4] = 4'b0110;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>-28 && (inp_data[i*`DWIDTH +:`DWIDTH])<=-16) begin
           address[i*4+:4] = 4'b0111;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>-39 && (inp_data[i*`DWIDTH +:`DWIDTH])<=-28) begin
           address[i*4+:4] = 4'b1000;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>-90 && (inp_data[i*`DWIDTH +:`DWIDTH])<=-39) begin
           address[i*4+:4] = 4'b1001;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])<=-90) begin
           address[i*4+:4] = 4'b1010;
        end
        else begin
           address[i*4+:4] = 4'b0101;
        end
    end
end

//Adding a dummy signal to use validity_mask input, to make ODIN happy
//TODO: Need to correctly use validity_mask
wire [`MASK_WIDTH-1:0] dummy;
assign dummy = validity_mask;

endmodule
module dpram_4096_40bit (
    clk,
    address_a,
    address_b,
    wren_a,
    wren_b,
    data_a,
    data_b,
    out_a,
    out_b
);
parameter AWIDTH=12;
parameter NUM_WORDS=4096;
parameter DWIDTH=40;
input clk;
input [(AWIDTH-1):0] address_a;
input [(AWIDTH-1):0] address_b;
input  wren_a;
input  wren_b;
input [(DWIDTH-1):0] data_a;
input [(DWIDTH-1):0] data_b;
output reg [(DWIDTH-1):0] out_a;
output reg [(DWIDTH-1):0] out_b;

`ifndef hard_mem

reg [DWIDTH-1:0] ram[NUM_WORDS-1:0];
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

defparam u_dual_port_ram.ADDR_WIDTH = AWIDTH;
defparam u_dual_port_ram.DATA_WIDTH = DWIDTH;


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
module adder_tree_4stage_16bit(clk,reset,inp00,inp01,inp10,inp11,inp20,inp21,inp30,inp31,inp40,inp41,inp50,inp51,inp60,inp61,inp70,inp71,sum_out);

input clk;
input reset; 
input [15:0] inp00; 
input [15:0] inp01;
input [15:0] inp10; 
input [15:0] inp11;
input [15:0] inp20; 
input [15:0] inp21;
input [15:0] inp30; 
input [15:0] inp31;
input [15:0] inp40; 
input [15:0] inp41;
input [15:0] inp50; 
input [15:0] inp51;
input [15:0] inp60; 
input [15:0] inp61;
input [15:0] inp70; 
input [15:0] inp71;
output reg [31:0] sum_out;

reg [16:0] S_0_0; 
reg [16:0] S_0_1;
reg [16:0] S_0_2;
reg [16:0] S_0_3;
reg [16:0] S_0_4;
reg [16:0] S_0_5;
reg [16:0] S_0_6;
reg [16:0] S_0_7;

always@(posedge clk) begin 

S_0_0 <= inp00 + inp01; 
S_0_1 <= inp10 + inp11;
S_0_2 <= inp20 + inp21;
S_0_3 <= inp30 + inp31;
S_0_4 <= inp40 + inp41; 
S_0_5 <= inp50 + inp51;
S_0_6 <= inp60 + inp61;
S_0_7 <= inp70 + inp71;

end 

reg [17:0] S_1_0;
reg [17:0] S_1_1;
reg [17:0] S_1_2;
reg [17:0] S_1_3;

always@(posedge clk) begin 

S_1_0 <= S_0_0 + S_0_1; 
S_1_1 <= S_0_2 + S_0_3;
S_1_2 <= S_0_4 + S_0_5; 
S_1_3 <= S_0_6 + S_0_7;

end

reg [18:0] S_2_0; 
reg [18:0] S_2_1;

always@(posedge clk) begin 

S_2_0 <= S_1_0 + S_1_1; 
S_2_1 <= S_1_2 + S_1_3;

end

always@(posedge clk) begin 

if (reset == 1'b1) begin 
  sum_out <= 32'd0; 
end
else begin 
  sum_out <= S_2_0 + S_2_1; 
end

end 

endmodule 
module dpram_2048_60bit (
    clk,
    address_a,
    address_b,
    wren_a,
    wren_b,
    data_a,
    data_b,
    out_a,
    out_b
);
parameter AWIDTH=11;
parameter NUM_WORDS=2048;
parameter DWIDTH=60;
input clk;
input [(AWIDTH-1):0] address_a;
input [(AWIDTH-1):0] address_b;
input  wren_a;
input  wren_b;
input [(DWIDTH-1):0] data_a;
input [(DWIDTH-1):0] data_b;
output reg [(DWIDTH-1):0] out_a;
output reg [(DWIDTH-1):0] out_b;

`ifndef hard_mem

reg [DWIDTH-1:0] ram[NUM_WORDS-1:0];
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

defparam u_dual_port_ram.ADDR_WIDTH = AWIDTH;
defparam u_dual_port_ram.DATA_WIDTH = DWIDTH;

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
module sigmoid(
input [15:0] x,
output [15:0] sig_out
);

reg [15:0] lut;
reg [5:0] address;

assign sig_out = lut;

always @(address)
begin

       case(address)
       6'd0: lut = 16'b0000000000101101; //sig(-4.5)
       6'd1: lut = 16'b0000000000110110; //sig(-4.3)
       6'd2: lut = 16'b0000000001000010; //sig(-4.1)
       6'd3: lut = 16'b0000000001010001; //sig(-3.9)
       6'd4:  lut = 16'b0000000001100010; //sig(-3.7)
       6'd5 :  lut = 16'b0000000001111000; //sig(-3.5)
       6'd6 :  lut= 16'b0000000010010001; //sig(-3.3)
       6'd7 :  lut= 16'b0000000010110000; //sig(-3.1)
       6'd8:  lut= 16'b0000000011010101; //sig(-2.9)
       6'd9 :  lut= 16'b0000000100000010; //sig(-2.7)
       6'd10 :  lut= 16'b0000000100110110; //sig(-2.5)
       6'd11 :  lut= 16'b0000000101110101; //sig(-2.3)
       6'd12 :  lut= 16'b0000000110111110; //sig(-2.1)
       6'd13 :  lut= 16'b0000001000010100; //sig(-1.9)
       6'd14 :  lut= 16'b0000001001111000; //sig(-1.7)
       6'd15 :  lut= 16'b0000001011101011; //sig(-1.5)
       6'd16 :  lut= 16'b0000001101101101; //sig(-1.3)
       6'd17:  lut= 16'b0000001111111110; //sig(-1.1)
       6'd18 :  lut= 16'b0000010010100000; //sig(-0.9)
       6'd19 :  lut= 16'b0000010101001111; //sig(-0.7)
       6'd20 :  lut= 16'b0000011000001010; //sig(-0.5)
       6'd21 :  lut= 16'b0000011011001111; //sig(-0.3)
       6'd22 :  lut= 16'b0000011110011001; //sig(-0.1)
       6'd23 :  lut= 16'b0000100001100110; //sig(0.1)
       6'd24 :  lut= 16'b0000100100110000; //sig(0.3)
       6'd25 :  lut= 16'b0000100111110101; //sig(0.5)
       6'd26 :  lut= 16'b0000101010110000; //sig(0.7)
       6'd27 :  lut= 16'b0000101101100000; //sig(0.9)
       6'd28 :  lut= 16'b0000110000000001; //sig(1.1)
       6'd29 :  lut= 16'b0000110010010010; //sig(1.3)
       6'd30 :  lut= 16'b0000110100010100; //sig(1.5)
       6'd31 :  lut= 16'b0000110110000111; //sig(1.7)
       6'd32 :  lut= 16'b0000110111101011; //sig(1.9)
       6'd33 :  lut= 16'b0000111001000001; //sig(2.1)
       6'd34 :  lut= 16'b0000111010001010; //sig(2.3)
       6'd35 :  lut= 16'b0000111011001001; //sig(2.5)
       6'd36 :  lut= 16'b0000111011111110; //sig(2.7)
       6'd37 :  lut= 16'b0000111100101010; //sig(2.9)
       6'd38 :  lut= 16'b0000111101001111; //sig(3.1)
       6'd39 :  lut= 16'b0000111101101110; //sig(3.3)
       6'd40 :  lut= 16'b0000111110000111; //sig(3.5)
       6'd41 :  lut= 16'b0000111110011101; //sig(3.7)
       6'd42 :  lut= 16'b0000111110101110; //sig(3.9)
       6'd43 :  lut= 16'b0000111110111101; //sig(4.1)
       6'd44 :  lut= 16'b0000111111001001; //sig(4.3)
       6'd45 :  lut= 16'b0000111111010011; //sig(4.5)
       6'd46 :  lut= 16'b0000111111011011; //sig(4.7)
       6'd47 :  lut= 16'b0000000000100100; //sig(-4.7)
       6'd48:   lut= 16'b0000000000000000; //0
       6'd49:   lut= 16'b0001000000000000; //1
       default: lut=0;
        endcase
end


always@(x)
begin

    case({x[15:12]})
        4'b1000:address = 6'd48;
        4'b1001:address = 6'd48;
        4'b1010:address = 6'd48;
        4'b1011:address = 6'd48;
        4'b1100:address = 6'd48;
        4'b1101:if((x[11:0] >= 12'h000) && (x[11:0] <= 12'h333)) // -3
                    begin
                       address = 6'd8;
                    end
                else if((x[11:0] > 12'h333) && (x[11:0] <= 12'h666))
                    begin
                        address = 6'd9;
                    end
                 else if((x[11:0] > 12'h666) && (x[11:0] <= 12'h99a))
                    begin
                        address = 6'd10;
                    end
                 else if((x[11:0] > 12'h99a) && (x[11:0] <= 12'hccd))
                    begin
                        address =  6'd11;
                    end
                 else
                    begin
                        address =  6'd12;
                    end
        4'b1110:if((x[11:0] >= 12'h000) && (x[11:0] <= 12'h333)) // -2
                    begin
                        address =  6'd13;
                    end
                else if((x[11:0] > 12'h333) && (x[11:0] <= 12'h666))
                    begin
                        address =  6'd14;
                    end
                 else if((x[11:0] > 12'h666) && (x[11:0] <= 12'h99a))
                    begin
                        address = 6'd15;
                    end
                 else if((x[11:0] > 12'h99a) && (x[11:0] <= 12'hccd))
                    begin
                        address =  6'd16;
                    end
                 else
                    begin
                        address =  6'd17;
                    end
        4'b1111:if((x[11:0] >= 12'h000) && (x[11:0] <= 12'h333))  // -1
                    begin
                        address =  6'd18;
                    end
                else if((x[11:0] > 12'h333) && (x[11:0] <= 12'h666))
                    begin
                        address =  6'd19;
                    end
                 else if((x[11:0] > 12'h666) && (x[11:0] <= 12'h99a))
                    begin
                        address =  6'd20;
                    end
                 else if((x[11:0] > 12'h99a) && (x[11:0] <= 12'hccd))
                    begin
                        address =  6'd21;                                                                                     
                    end
                 else
                    begin
                        address =  6'd22;
                    end
        4'b0000:if((x[11:0] >= 12'h000) && (x[11:0] <= 12'h333)) // 0
                    begin
                        address =  6'd23;
                    end
                else if((x[11:0] > 12'h333) && (x[11:0] <= 12'h666))
                    begin
                        address =  6'd24;
                    end
                 else if((x[11:0] > 12'h666) && (x[11:0] <= 12'h99a))
                    begin
                        address =  6'd25;
                    end
                 else if((x[11:0] > 12'h99a) && (x[11:0] <= 12'hccd))
                    begin
                        address =  6'd26;
                    end
                 else
                    begin
                        address =  6'd27;
                    end
        4'b0001:if((x[11:0] >= 12'h000) && (x[11:0] <= 12'h333)) // 1
                    begin
                        address =  6'd28;
                    end
                else if((x[11:0] > 12'h333) && (x[11:0] <= 12'h666))
                    begin
                        address =  6'd29;
                    end
                else if((x[11:0] > 12'h666) && (x[11:0] <= 12'h99a))
                    begin
                        address =  6'd30;
                    end
                else if((x[11:0] > 12'h99a) && (x[11:0] <= 12'hccd))
                    begin
                        address =  6'd31;
                    end
                else
                    begin
                       address =  6'd32;
                    end
        4'b0010:if((x[11:0] >= 12'h000) && (x[11:0] <= 12'h333))  // 2
                    begin
                      address =  6'd33;
                    end
                else if((x[11:0] > 12'h333) && (x[11:0] <= 12'h666))
                    begin
                      address =  6'd34;
                    end
                 else if((x[11:0] > 12'h666) && (x[11:0] <= 12'h99a))
                    begin
                       address =  6'd35;
                    end
                 else if((x[11:0] > 12'h99a) && (x[11:0] <= 12'hccd))
                    begin
                       address =  6'd36;
                    end
                 else
                    begin
                       address =  6'd37;
                    end
        4'b0011:if((x[11:0] >= 12'h000) && (x[11:0] <= 12'h333)) // 3
                    begin
                       address =  6'd38;
                    end
                else if((x[11:0] > 12'h333) && (x[11:0] <= 12'h666))
                    begin
                      address =  6'd39;
                    end
                else if((x[11:0] > 12'h666) && (x[11:0] <= 12'h99a))
                    begin
                      address =  6'd40;
                    end
                else if((x[11:0] > 12'h99a) && (x[11:0] <= 12'hccd))
                    begin
                      address = 6'd41;
                    end
               else
                    begin
                       address = 6'd42;
                    end
        4'b0100:address = 6'd49;
        4'b0101:address = 6'd49;
        4'b0110:address = 6'd49;
        4'b0111:address = 6'd49;
       /* 4'b0100:if((x[11:0] >= 12'h000) && (x[11:0] <= 12'h333)) //4
                    begin
                      address = lut[43];
                    end
                else if((x[11:0] > 12'h333) && (x[11:0] <= 12'h666))
                    begin
                       address = lut[44];
                    end
                else if((x[11:0] > 12'h666) && (x[11:0] <= 12'h99a))
                    begin
                       address = lut[45];
                    end
                else if(x[11:0] > 12'h99a)
                    begin
                        address = lut[46];
                    end
        4'b0101: address = lut[46];
        4'b0110: address = lut[46];
        4'b0111: address = lut[46];  */
        default: address = 16'h1000;
        endcase

end

endmodule

module adder_tree_3stage_8bit (clk,reset,inp00,inp01,inp10,inp11,inp20,inp21,inp30,inp31,sum_out); 

input clk; 
input reset; 
input [7:0] inp00; 
input [7:0] inp01;
input [7:0] inp10; 
input [7:0] inp11;
input [7:0] inp20; 
input [7:0] inp21;
input [7:0] inp30; 
input [7:0] inp31;
output reg [15:0] sum_out;

reg [8:0] S_0_0; 
reg [8:0] S_0_1;
reg [8:0] S_0_2;
reg [8:0] S_0_3;

always@(posedge clk) begin 

S_0_0 <= inp00 + inp01; 
S_0_1 <= inp10 + inp11;
S_0_2 <= inp20 + inp21;
S_0_3 <= inp30 + inp31;

end 

reg [9:0] S_1_0;
reg [9:0] S_1_1;

always@(posedge clk) begin 

S_1_0 <= S_0_0 + S_0_1; 
S_1_1 <= S_0_2 + S_0_3;

end 

always@(posedge clk) begin 

if (reset == 1'b1) begin 
  sum_out <= 16'd0; 
end
else begin 
  sum_out <= S_1_0 + S_1_1; 
end

end 

endmodule 
module adder_tree_4stage_4bit(clk,reset,inp00,inp01,inp10,inp11,inp20,inp21,inp30,inp31,inp40,inp41,inp50,inp51,inp60,inp61,inp70,inp71,sum_out);

input clk;
input reset; 
input [3:0] inp00; 
input [3:0] inp01;
input [3:0] inp10; 
input [3:0] inp11;
input [3:0] inp20; 
input [3:0] inp21;
input [3:0] inp30; 
input [3:0] inp31;
input [3:0] inp40; 
input [3:0] inp41;
input [3:0] inp50; 
input [3:0] inp51;
input [3:0] inp60; 
input [3:0] inp61;
input [3:0] inp70; 
input [3:0] inp71;
output reg [7:0] sum_out;

reg [4:0] S_0_0; 
reg [4:0] S_0_1;
reg [4:0] S_0_2;
reg [4:0] S_0_3;
reg [4:0] S_0_4;
reg [4:0] S_0_5;
reg [4:0] S_0_6;
reg [4:0] S_0_7;

always@(posedge clk) begin 

S_0_0 <= inp00 + inp01; 
S_0_1 <= inp10 + inp11;
S_0_2 <= inp20 + inp21;
S_0_3 <= inp30 + inp31;
S_0_4 <= inp40 + inp41; 
S_0_5 <= inp50 + inp51;
S_0_6 <= inp60 + inp61;
S_0_7 <= inp70 + inp71;

end 

reg [5:0] S_1_0;
reg [5:0] S_1_1;
reg [5:0] S_1_2;
reg [5:0] S_1_3;

always@(posedge clk) begin 

S_1_0 <= S_0_0 + S_0_1; 
S_1_1 <= S_0_2 + S_0_3;
S_1_2 <= S_0_4 + S_0_5; 
S_1_3 <= S_0_6 + S_0_7;

end

reg [6:0] S_2_0; 
reg [6:0] S_2_1;

always@(posedge clk) begin 

S_2_0 <= S_1_0 + S_1_1; 
S_2_1 <= S_1_2 + S_1_3;

end

always@(posedge clk) begin 

if (reset == 1'b1) begin 
  sum_out <= 8'd0; 
end
else begin 
  sum_out <= S_2_0 + S_2_1; 
end

end 

endmodule 

