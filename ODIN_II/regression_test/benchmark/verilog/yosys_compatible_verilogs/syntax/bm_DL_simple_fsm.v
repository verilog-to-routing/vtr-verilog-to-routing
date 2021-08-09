/* pg 468 - simple fsm.  Note modified to synchronous */

module bm_DL_simple_fsm (Clock, Resetn, w, z); 
  input Clock, Resetn, w; 
  output z; 
  reg [1:0] y, Y; 
  parameter [1:0] A = 2'b00, B = 2'b01, C = 2'b10; 

  // Define the next state combinational circuit 
  always @(w or y) 
    case (y) 
      A:  if (w)    Y = B; 
        else    Y = A; 
      B:  if (w)    Y = C; 
        else    Y = A; 
      C:  if (w)    Y = C; 
        else    Y = A; 
      default:      Y = 2'b00; 
    endcase 

  // Define the sequential block 
  always @(posedge Clock) 
    if (Resetn == 1'b0) y <= A; 
    else y <= Y; 

  // Define output 
  assign z = (y == C); 

endmodule
