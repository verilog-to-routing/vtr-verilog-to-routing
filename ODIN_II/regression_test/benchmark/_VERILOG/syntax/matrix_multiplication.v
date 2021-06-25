//Single Port RAM
module my_single_port_ram (addr0, ce0, d0, we0, q0,  clk);

  parameter DWIDTH = 16;
  parameter AWIDTH = 4;
  parameter MEM_SIZE = 16;
  
  input [AWIDTH-1:0] addr0;
  input ce0;
  input [DWIDTH-1:0] d0;
  input we0;
  output [DWIDTH-1:0] q0;
  input clk;
  
  reg [DWIDTH-1:0] q0;
  reg [DWIDTH-1:0] ram[MEM_SIZE-1:0];
  
  always @(posedge clk)  
  begin 
      if (ce0) 
      begin
          if (we0) 
          begin 
              ram[addr0] <= d0; 
          end 
          q0 <= ram[addr0];
      end
  end

endmodule

//Multiplier module (just wraps the * operator in a module
//so that we can use verilog style 'for generate' stmts in
//the main matrix multiplier module)
// module qmult(i_multiplicand,i_multiplier,o_result);

//   input [17:0] i_multiplicand;
//   input [17:0] i_multiplier;
//   output [17:0] o_result;
  
//   assign o_result = i_multiplicand * i_multiplier;

// endmodule

//Adder module (just wraps the + operator in a module
//so that we can use verilog style 'for generate' stmts in
//the main matrix multiplier module)
// module qadd(a,b,c);

//   input [17:0] a;
//   input [17:0] b;
//   output [17:0] c;
  
//   assign c = a + b;
// endmodule

//The main module. Multiplies two 4x4 matrices.
//The two inputs matrices (A and B) are stored
//in BRAMs first. The output matrix (C) is also
//stored in a BRAM.
module matrix_multiplication(clk, reset, we1, we2, start, data_in1, data_in2, data_out);
  input clk;
  input reset;
  input we1;  
  input we2;  
  input start;
  input [17:0] data_in1;
  input [17:0] data_in2;
  output [17:0] data_out;  

 wire [17:0] mat_A;  
 wire [17:0] mat_B;  
 reg wen;  
 reg [17:0] data_in;  
 reg [3:0] addr_in;  
 reg [4:0] address;  

 //Local (distributed) memory
 reg [17:0] matrixA[3:0][3:0];
 reg [17:0] matrixB[3:0][3:0];  

 wire [17:0] tmp1[3:0][3:0];
 wire [17:0] tmp2[3:0][3:0];
 wire [17:0] tmp3[3:0][3:0];
 wire [17:0] tmp4[3:0][3:0];
 wire [17:0] tmp5[3:0][3:0];
 wire [17:0] tmp6[3:0][3:0];
 wire [17:0] tmp7[3:0][3:0];  

  // BRAM matrix A  
  my_single_port_ram matrix_A_u (
    .addr0(addr_in),
    .ce0(1'b1), 
    .d0(data_in1), 
    .we0(we1), 
    .q0(mat_A), 
    .clk(clk));

  // BRAM matrix B  
  my_single_port_ram matrix_B_u(
    .addr0(addr_in),
    .ce0(1'b1), 
    .d0(data_in2), 
    .we0(we2), 
    .q0(mat_B), 
    .clk(clk));
          
    always @(posedge clk or posedge reset)  
      begin  
           if(reset && !start) begin  
                addr_in <= 0;  
           end  
           else begin  
                if(addr_in<15) begin
                  addr_in <= addr_in + 1;  
                end else begin
                  addr_in <= addr_in;  
                end  

                matrixA[addr_in/4][addr_in-(addr_in/4)*4] <= mat_A ;  
                matrixB[addr_in/4][addr_in-(addr_in/4)*4] <= mat_B ;  
           end  
      end  

    assign tmp1[0][0] = matrixA[0][0] * matrixB[0][0];
    assign tmp2[0][0] = matrixA[0][1] * matrixB[1][0];
    assign tmp3[0][0] = matrixA[0][2] * matrixB[2][0];
    assign tmp4[0][0] = matrixA[0][3] * matrixB[3][0];

    assign tmp5[0][0] = tmp1[0][0] * tmp2[0][0];
    assign tmp6[0][0] = tmp3[0][0] * tmp4[0][0];
    assign tmp7[0][0] = tmp5[0][0] * tmp6[0][0];

    //   genvar i,j,k;  
    //   generate  
    //   for(i=0;i<4;i=i+1) begin:gen1  
    //   for(j=0;j<4;j=j+1) begin:gen2  
           // fixed point multiplication  
          //  qmult mult_u1(.i_multiplicand(matrixA[0][0]),.i_multiplier(matrixB[0][0]),.o_result(tmp1));  
          //  qmult mult_u2(.i_multiplicand(matrixA[0][1]),.i_multiplier(matrixB[1][0]),.o_result(tmp2));  
          //  qmult mult_u3(.i_multiplicand(matrixA[0][2]),.i_multiplier(matrixB[2][0]),.o_result(tmp3));  
          //  qmult mult_u4(.i_multiplicand(matrixA[0][3]),.i_multiplier(matrixB[3][0]),.o_result(tmp4));  
           // fixed point addition  
          //  qadd  Add_u1(.a(tmp1),.b(tmp2),.c(tmp5));  
          //  qadd  Add_u2(.a(tmp3),.b(tmp4),.c(tmp6));  
          //  qadd  Add_u3(.a(tmp5),.b(tmp6),.c(tmp7));  
    //   end  
    //   end  
    //   endgenerate  
      
      always @(posedge clk or posedge reset)  
      begin  
           if(reset && !start) begin  
                address <= 0;  
                wen <= 0;  
                end  
           else begin  
                address <= address + 1;  
                if(address<16) begin  
                     wen <= 1;  
                     data_in <= tmp7[0][0];  
                end  
                else  
                begin  
                     wen <= 0;            
                end  
           end  
      end  

      // BRAM matrix C  
      my_single_port_ram matrix_out_u(
        .addr0(address[3:0]),
        .ce0(1'b1), 
        .d0(data_in), 
        .we0(wen), 
        .q0(data_out), 
        .clk(clk));      
 endmodule