  module depth_split (we, addr, datain, dataout, clk);

    input we; 
    input[11 - 1:0] addr; 
    input[2 - 1:0] datain; 
    output[2 - 1:0] dataout; 
    wire[2 - 1:0] dataout;
    input clk; 




single_port_ram new_ram(
  .clk (clk),
  .we(we),
  .data(datain),
  .out(dataout),
  .addr(addr)
  );
  
  
endmodule

    
