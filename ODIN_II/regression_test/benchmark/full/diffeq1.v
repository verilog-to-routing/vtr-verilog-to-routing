 module diffeq_paj_convert (Xinport, Yinport, Uinport, Aport, DXport, Xoutport, Youtport, Uoutport, clk, reset);
    input[31:0] Xinport; 
    input[31:0] Yinport; 
    input[31:0] Uinport; 
    input[31:0] Aport; 
    input[31:0] DXport; 
    input clk; 
    input reset; 
    output[31:0] Xoutport; 
    output[31:0] Youtport; 
    output[31:0] Uoutport; 
    reg[31:0] Xoutport;
    reg[31:0] Youtport;
    reg[31:0] Uoutport;

       reg[31:0] x_var; 
       reg[31:0] y_var; 
       reg[31:0] u_var; 
       wire[31:0] temp; 
       reg looping; 
 
assign temp = u_var * DXport;
    always @(posedge clk)
    begin
		if (reset == 1'b1)
		begin
			looping <= 1'b0;
			x_var <= 0;
            y_var <= 0;
            u_var <= 0;
		end
		else
          if (looping == 1'b0)
          begin
             x_var <= Xinport; 
             y_var <= Yinport; 
             u_var <= Uinport; 
             looping <= 1'b1; 
          end
          else if (x_var < Aport)
          begin
             u_var <= (u_var - (temp/*u_var * DXport*/ * 3 * x_var)) - (DXport * 3 * y_var); 
             y_var <= y_var + temp;//(u_var * DXport); 
             x_var <= x_var + DXport; 
			looping <= looping;
          end
          else
          begin
             Xoutport <= x_var ; 
             Youtport <= y_var ; 
             Uoutport <= u_var ; 
             looping <= 1'b0; 
          end 
    end 
 endmodule

