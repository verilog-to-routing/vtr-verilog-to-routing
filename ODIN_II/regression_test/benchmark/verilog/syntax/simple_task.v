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
// xor_task  my_xor(clk,a,b,reset,out);
xor_task(a,b,reset,out);
end

task xor_task;
  input [1:0] x,y;
  input rst;
  output [2:0] z;
    begin
      case(rst)
            1'b0:       z <= x + y;
            default:    z <= 1'b0;
        endcase
    end
endtask

endmodule

// module xor_task(clk,x,y,rst,z);
//   input clk;
//   input [1:0] x,y;
//   input rst;
//   output [2:0] z;

//   always @(posedge clk)
//   begin
//     case(rst)
//           1'b0:       z <= x + y;
//           default:    z <= 1'b0;
//       endcase
//   end

// endmodule