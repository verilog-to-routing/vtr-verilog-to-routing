`default_nettype none

module modify_count #(
    parameter MOD_VALUE = 10
) (
    input wire logic clk,
    reset,
    increment,
    output logic rolling_over,
    output logic [3:0] count = 0
);

  always_ff @(posedge clk) begin
    if (reset) count <= 4'b0000;
    else if (increment) begin
      if (rolling_over) count <= 4'b0000;
      else count <= count + 4'b0001;
    end
  end

  always_comb begin
    if (increment && (count == MOD_VALUE - 1)) rolling_over = 1'b1;
    else rolling_over = 1'b0;
  end

endmodule
