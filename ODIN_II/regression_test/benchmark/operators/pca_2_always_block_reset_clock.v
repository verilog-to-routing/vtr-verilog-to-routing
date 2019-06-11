module assig (RST, 
	STATE, 
	CLOCK, 
	DATA_IN
); 

	input RST; 
	input CLOCK; 
	input  DATA_IN; 

	output  STATE; 
	 
	reg  STATE; 
	 
always @ ( RST )
begin
	if( RST ) 
	begin 

		assign STATE = 1'b0;
 
	end 
	else 
	begin 

		deassign STATE;
 
	end 
end  


always @ ( posedge CLOCK ) 
begin 

		STATE = DATA_IN; 

end 
 

endmodule 
