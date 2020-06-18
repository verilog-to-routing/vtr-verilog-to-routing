//A simple cricuit which blinks an LED on and off periodically
module blink(
    input clk,      //Input clock
    input i_reset,  //Input active-high reset
    output o_led);  //Output to LED

    //Sequential logic
    //
    //A reset-able counter which increments each clock cycle
    reg[4:0] r_counter;
    always @(posedge clk) begin
        if (i_reset) begin //When reset is high, clear counter
            r_counter <= 5'd0;
        end else begin //Otherwise increment counter each clock (note that it will overflow back to zero)
            r_counter <= r_counter + 1'b1;
        end
    end

    //Combinational logic 
    //
    //Drives o_led high if count is below a threshold
    always @(*) begin
        if (r_counter < 5'd16) begin
            o_led <= 1'b1;
        end else begin
            o_led <= 1'b0;
        end
    end

endmodule
