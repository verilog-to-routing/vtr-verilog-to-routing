module simple_task (
    clk,
    reset,
    a,
    b,    
    out,
    clk_out
    );

    input   clk;
    input   reset;
    input   [1:0] a,b;

    output  [2:0] out;
    output  clk_out;

    assign clk_out = clk;

always @(posedge clk)
begin
 my_task(.rst(reset),.y(a),.x(b));
end

task my_task(  input [1:0] x,y, input rst, output [2:0] z );


  begin 
    case(rst)
      1'b0:       z <= x + y;
      default:    z <= 1'b0;
    endcase
  end  
endtask


endmodule