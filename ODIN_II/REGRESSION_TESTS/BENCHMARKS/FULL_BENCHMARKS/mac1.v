module Fadder_(
carry,
  sum,
  in3,
  in2,
  in1
  );

  input in1, in2, in3;
  output sum, carry;

  assign sum=in1^in2^in3;
  assign carry=(in1&in2) | (in2&in3) | (in1&in3);

endmodule 

module Fadder_2(
  in1,
  in2,
  in3,
  sum,
carry
  );

  input in1, in2, in3;
  output sum, carry;

  assign sum=in1^in2^in3;
  assign carry=(in1&in2) | (in2&in3) | (in1&in3);

endmodule 


module Hadder_(
carry,
  sum,
  in2,
  in1
  );


  input in1, in2;
  output sum, carry;

  assign sum=in1^in2;
  assign carry=in1&in2;

endmodule 

module mult_(
  a,
  b,
  s,
  c);

  input [31:0] a;
  input [31:0] b;
  output [63:0] s;
  output [62:0] c;

  wire [31:0] a_;
  wire sign_ ;
  reg [33:0] P_0 ;
  reg [33:0] P_1 ;
  reg [33:0] P_2 ;
  reg [33:0] P_3 ;
  reg [33:0] P_4 ;
  reg [33:0] P_5 ;
  reg [33:0] P_6 ;
  reg [33:0] P_7 ;
  reg [33:0] P_8 ;
  reg [33:0] P_9 ;
  reg [33:0] P_10 ;
  reg [33:0] P_11 ;
  reg [33:0] P_12 ;
  reg [33:0] P_13 ;
  reg [33:0] P_14 ;
  reg [33:0] P_15 ;
  reg [31:0] inc ;

  assign sign_ = 1'b1 ;
  assign a_ = (~ a) ;

  always @( a or b or a_ )
    begin

    case( b[1:0] ) 
      2'b00:
        begin
        inc[0]=1'b0 ;
        inc[1]=1'b0 ;
        P_0={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      2'b01:
        begin
        inc[0]=1'b0 ;
        inc[1]=1'b0 ;
        P_0={(~a[31]),a[31],a} ;
        end
      2'b10:
        begin
        inc[0]=1'b0 ;
        inc[1]=1'b1 ;
        P_0={(~a_[31]),a_,1'b0} ;
        end
      2'b11:
        begin
        inc[0]=1'b1 ;
        inc[1]=1'b0 ;
        P_0={(~a_[31]),a_[31],a_} ;
        end
    endcase

    case({b[3:1]}) 
      3'b000:
	   begin
        inc[2]=1'b0 ;
        inc[3]=1'b0 ;
        P_1={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b111:
        begin
        inc[2]=1'b0 ;
        inc[3]=1'b0 ;
        P_1={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b001:
        begin
        inc[2]=1'b0 ;
        inc[3]=1'b0 ;
        P_1={(~a[31]),a[31],a} ;
        end
      3'b010:
        begin
        inc[2]=1'b0 ;
        inc[3]=1'b0 ;
        P_1={(~a[31]),a[31],a} ;
        end
      3'b100:
        begin
        inc[2]=1'b0 ;
        inc[3]=1'b1 ;
        P_1={(~a_[31]),a_,1'b0} ;
        end
      3'b101:
        begin
        inc[2]=1'b1 ;
        inc[3]=1'b0 ;
        P_1={(~a_[31]),a_[31],a_} ;
        end
      3'b110:
        begin
        inc[2]=1'b1 ;
        inc[3]=1'b0 ;
        P_1={(~a_[31]),a_[31],a_} ;
        end
      3'b011:
        begin
        inc[2]=1'b0 ;
        inc[3]=1'b0 ;
        P_1={(~a[31]),a,1'b0} ;
        end
    endcase

    case({b[5:3]}) 
      3'b000:
        begin
        inc[4]=1'b0 ;
        inc[5]=1'b0 ;
        P_2={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b111:
        begin
        inc[4]=1'b0 ;
        inc[5]=1'b0 ;
        P_2={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b001:
        begin
        inc[4]=1'b0 ;
        inc[5]=1'b0 ;
        P_2={(~a[31]),a[31],a} ;
        end
      3'b010:
        begin
        inc[4]=1'b0 ;
        inc[5]=1'b0 ;
        P_2={(~a[31]),a[31],a} ;
        end
      3'b100:
        begin
        inc[4]=1'b0 ;
        inc[5]=1'b1 ;
        P_2={(~a_[31]),a_,1'b0} ;
        end
      3'b101:
        begin
        inc[4]=1'b1 ;
        inc[5]=1'b0 ;
        P_2={(~a_[31]),a_[31],a_} ;
        end
      3'b110:
        begin
        inc[4]=1'b1 ;
        inc[5]=1'b0 ;
        P_2={(~a_[31]),a_[31],a_} ;
        end
      3'b011:
        begin
        inc[4]=1'b0 ;
        inc[5]=1'b0 ;
        P_2={(~a[31]),a,1'b0} ;
        end
    endcase

    case({b[7:5]}) 
      3'b000:
        begin
        inc[6]=1'b0 ;
        inc[7]=1'b0 ;
        P_3={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b111:
        begin
        inc[6]=1'b0 ;
        inc[7]=1'b0 ;
        P_3={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b001:
        begin
        inc[6]=1'b0 ;
        inc[7]=1'b0 ;
        P_3={(~a[31]),a[31],a} ;
        end
      3'b010:
        begin
        inc[6]=1'b0 ;
        inc[7]=1'b0 ;
        P_3={(~a[31]),a[31],a} ;
        end
      3'b100:
        begin
        inc[6]=1'b0 ;
        inc[7]=1'b1 ;
        P_3={(~a_[31]),a_,1'b0} ;
        end
      3'b101:
        begin
        inc[6]=1'b1 ;
        inc[7]=1'b0 ;
        P_3={(~a_[31]),a_[31],a_} ;
        end
      3'b110:
        begin
        inc[6]=1'b1 ;
        inc[7]=1'b0 ;
        P_3={(~a_[31]),a_[31],a_} ;
        end
      3'b011:
        begin
        inc[6]=1'b0 ;
        inc[7]=1'b0 ;
        P_3={(~a[31]),a,1'b0} ;
        end
    endcase

    case({b[9:7]}) 
      3'b000:
        begin
        inc[8]=1'b0 ;
        inc[9]=1'b0 ;
        P_4={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b111:
        begin
        inc[8]=1'b0 ;
        inc[9]=1'b0 ;
        P_4={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b001:
        begin
        inc[8]=1'b0 ;
        inc[9]=1'b0 ;
        P_4={(~a[31]),a[31],a} ;
        end
      3'b010:
        begin
        inc[8]=1'b0 ;
        inc[9]=1'b0 ;
        P_4={(~a[31]),a[31],a} ;
        end
      3'b100:
        begin
        inc[8]=1'b0 ;
        inc[9]=1'b1 ;
        P_4={(~a_[31]),a_,1'b0} ;
        end
      3'b101:
        begin
        inc[8]=1'b1 ;
        inc[9]=1'b0 ;
        P_4={(~a_[31]),a_[31],a_} ;
        end
      3'b110:
        begin
        inc[8]=1'b1 ;
        inc[9]=1'b0 ;
        P_4={(~a_[31]),a_[31],a_} ;
        end
      3'b011:
        begin
        inc[8]=1'b0 ;
        inc[9]=1'b0 ;
        P_4={(~a[31]),a,1'b0} ;
        end
    endcase

    case({b[11:9]}) 
      3'b000:
        begin
        inc[10]=1'b0 ;
        inc[11]=1'b0 ;
        P_5={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b111:
        begin
        inc[10]=1'b0 ;
        inc[11]=1'b0 ;
        P_5={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b001:
        begin
        inc[10]=1'b0 ;
        inc[11]=1'b0 ;
        P_5={(~a[31]),a[31],a} ;
        end
      3'b010:
        begin
        inc[10]=1'b0 ;
        inc[11]=1'b0 ;
        P_5={(~a[31]),a[31],a} ;
        end
      3'b100:
        begin
        inc[10]=1'b0 ;
        inc[11]=1'b1 ;
        P_5={(~a_[31]),a_,1'b0} ;
        end
      3'b101:
        begin
        inc[10]=1'b1 ;
        inc[11]=1'b0 ;
        P_5={(~a_[31]),a_[31],a_} ;
        end
      3'b110:
        begin
        inc[10]=1'b1 ;
        inc[11]=1'b0 ;
        P_5={(~a_[31]),a_[31],a_} ;
        end
      3'b011:
        begin
        inc[10]=1'b0 ;
        inc[11]=1'b0 ;
        P_5={(~a[31]),a,1'b0} ;
        end
    endcase

    case({b[13:11]}) 
      3'b000:
        begin
        inc[12]=1'b0 ;
        inc[13]=1'b0 ;
        P_6={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b111:
        begin
        inc[12]=1'b0 ;
        inc[13]=1'b0 ;
        P_6={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b001:
        begin
        inc[12]=1'b0 ;
        inc[13]=1'b0 ;
        P_6={(~a[31]),a[31],a} ;
        end
      3'b010:
        begin
        inc[12]=1'b0 ;
        inc[13]=1'b0 ;
        P_6={(~a[31]),a[31],a} ;
        end
      3'b100:
        begin
        inc[12]=1'b0 ;
        inc[13]=1'b1 ;
        P_6={(~a_[31]),a_,1'b0} ;
        end
      3'b101:
        begin
        inc[12]=1'b1 ;
        inc[13]=1'b0 ;
        P_6={(~a_[31]),a_[31],a_} ;
        end
      3'b110:
        begin
        inc[12]=1'b1 ;
        inc[13]=1'b0 ;
        P_6={(~a_[31]),a_[31],a_} ;
        end
      3'b011:
        begin
        inc[12]=1'b0 ;
        inc[13]=1'b0 ;
        P_6={(~a[31]),a,1'b0} ;
        end
    endcase

    case({b[15:13]}) 
      3'b000:
        begin
        inc[14]=1'b0 ;
        inc[15]=1'b0 ;
        P_7={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b111:
        begin
        inc[14]=1'b0 ;
        inc[15]=1'b0 ;
        P_7={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b001:
        begin
        inc[14]=1'b0 ;
        inc[15]=1'b0 ;
        P_7={(~a[31]),a[31],a} ;
        end
      3'b010:
        begin
        inc[14]=1'b0 ;
        inc[15]=1'b0 ;
        P_7={(~a[31]),a[31],a} ;
        end
      3'b100:
        begin
        inc[14]=1'b0 ;
        inc[15]=1'b1 ;
        P_7={(~a_[31]),a_,1'b0} ;
        end
      3'b101:
        begin
        inc[14]=1'b1 ;
        inc[15]=1'b0 ;
        P_7={(~a_[31]),a_[31],a_} ;
        end
      3'b110:
        begin
        inc[14]=1'b1 ;
        inc[15]=1'b0 ;
        P_7={(~a_[31]),a_[31],a_} ;
        end
      3'b011:
        begin
        inc[14]=1'b0 ;
        inc[15]=1'b0 ;
        P_7={(~a[31]),a,1'b0} ;
        end
    endcase

    case({b[17:15]}) 
      3'b000:
        begin
        inc[16]=1'b0 ;
        inc[17]=1'b0 ;
        P_8={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b111:
        begin
        inc[16]=1'b0 ;
        inc[17]=1'b0 ;
        P_8={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b001:
        begin
        inc[16]=1'b0 ;
        inc[17]=1'b0 ;
        P_8={(~a[31]),a[31],a} ;
        end
      3'b010:
        begin
        inc[16]=1'b0 ;
        inc[17]=1'b0 ;
        P_8={(~a[31]),a[31],a} ;
        end
      3'b100:
        begin
        inc[16]=1'b0 ;
        inc[17]=1'b1 ;
        P_8={(~a_[31]),a_,1'b0} ;
        end
      3'b101:
        begin
        inc[16]=1'b1 ;
        inc[17]=1'b0 ;
        P_8={(~a_[31]),a_[31],a_} ;
        end
      3'b110:
        begin
        inc[16]=1'b1 ;
        inc[17]=1'b0 ;
        P_8={(~a_[31]),a_[31],a_} ;
        end
      3'b011:
        begin
        inc[16]=1'b0 ;
        inc[17]=1'b0 ;
        P_8={(~a[31]),a,1'b0} ;
        end
    endcase

    case({b[19:17]}) 
      3'b000:
        begin
        inc[18]=1'b0 ;
        inc[19]=1'b0 ;
        P_9={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b111:
        begin
        inc[18]=1'b0 ;
        inc[19]=1'b0 ;
        P_9={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b001:
        begin
        inc[18]=1'b0 ;
        inc[19]=1'b0 ;
        P_9={(~a[31]),a[31],a} ;
        end
      3'b010:
        begin
        inc[18]=1'b0 ;
        inc[19]=1'b0 ;
        P_9={(~a[31]),a[31],a} ;
        end
      3'b100:
        begin
        inc[18]=1'b0 ;
        inc[19]=1'b1 ;
        P_9={(~a_[31]),a_,1'b0} ;
        end
      3'b101:
        begin
        inc[18]=1'b1 ;
        inc[19]=1'b0 ;
        P_9={(~a_[31]),a_[31],a_} ;
        end
      3'b110:
        begin
        inc[18]=1'b1 ;
        inc[19]=1'b0 ;
        P_9={(~a_[31]),a_[31],a_} ;
        end
      3'b011:
        begin
        inc[18]=1'b0 ;
        inc[19]=1'b0 ;
        P_9={(~a[31]),a,1'b0} ;
        end
    endcase

    case({b[21:19]}) 
      3'b000:
        begin
        inc[20]=1'b0 ;
        inc[21]=1'b0 ;
        P_10={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b111:
        begin
        inc[20]=1'b0 ;
        inc[21]=1'b0 ;
        P_10={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b001:
        begin
        inc[20]=1'b0 ;
        inc[21]=1'b0 ;
        P_10={(~a[31]),a[31],a} ;
        end
      3'b010:
        begin
        inc[20]=1'b0 ;
        inc[21]=1'b0 ;
        P_10={(~a[31]),a[31],a} ;
        end
      3'b100:
        begin
        inc[20]=1'b0 ;
        inc[21]=1'b1 ;
        P_10={(~a_[31]),a_,1'b0} ;
        end
      3'b101:
        begin
        inc[20]=1'b1 ;
        inc[21]=1'b0 ;
        P_10={(~a_[31]),a_[31],a_} ;
        end
      3'b110:
        begin
        inc[20]=1'b1 ;
        inc[21]=1'b0 ;
        P_10={(~a_[31]),a_[31],a_} ;
        end
      3'b011:
        begin
        inc[20]=1'b0 ;
        inc[21]=1'b0 ;
        P_10={(~a[31]),a,1'b0} ;
        end
    endcase

    case({b[23:21]}) 
      3'b000:
        begin
        inc[22]=1'b0 ;
        inc[23]=1'b0 ;
        P_11={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b111:
        begin
        inc[22]=1'b0 ;
        inc[23]=1'b0 ;
        P_11={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b001:
        begin
        inc[22]=1'b0 ;
        inc[23]=1'b0 ;
        P_11={(~a[31]),a[31],a} ;
        end
      3'b010:
        begin
        inc[22]=1'b0 ;
        inc[23]=1'b0 ;
        P_11={(~a[31]),a[31],a} ;
        end
      3'b100:
        begin
        inc[22]=1'b0 ;
        inc[23]=1'b1 ;
        P_11={(~a_[31]),a_,1'b0} ;
        end
      3'b101:
        begin
        inc[22]=1'b1 ;
        inc[23]=1'b0 ;
        P_11={(~a_[31]),a_[31],a_} ;
        end
      3'b110:
        begin
        inc[22]=1'b1 ;
        inc[23]=1'b0 ;
        P_11={(~a_[31]),a_[31],a_} ;
        end
      3'b011:
        begin
        inc[22]=1'b0 ;
        inc[23]=1'b0 ;
        P_11={(~a[31]),a,1'b0} ;
        end
    endcase

    case({b[25:23]}) 
      3'b000:
        begin
        inc[24]=1'b0 ;
        inc[25]=1'b0 ;
        P_12={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b111:
        begin
        inc[24]=1'b0 ;
        inc[25]=1'b0 ;
        P_12={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b001:
        begin
        inc[24]=1'b0 ;
        inc[25]=1'b0 ;
        P_12={(~a[31]),a[31],a} ;
        end
      3'b010:
        begin
        inc[24]=1'b0 ;
        inc[25]=1'b0 ;
        P_12={(~a[31]),a[31],a} ;
        end
      3'b100:
        begin
        inc[24]=1'b0 ;
        inc[25]=1'b1 ;
        P_12={(~a_[31]),a_,1'b0} ;
        end
      3'b101:
        begin
        inc[24]=1'b1 ;
        inc[25]=1'b0 ;
        P_12={(~a_[31]),a_[31],a_} ;
        end
      3'b110:
        begin
        inc[24]=1'b1 ;
        inc[25]=1'b0 ;
        P_12={(~a_[31]),a_[31],a_} ;
        end
      3'b011:
        begin
        inc[24]=1'b0 ;
        inc[25]=1'b0 ;
        P_12={(~a[31]),a,1'b0} ;
        end
    endcase

    case({b[27:25]}) 
      3'b000:
        begin
        inc[26]=1'b0 ;
        inc[27]=1'b0 ;
        P_13={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b111:
        begin
        inc[26]=1'b0 ;
        inc[27]=1'b0 ;
        P_13={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b001:
        begin
        inc[26]=1'b0 ;
        inc[27]=1'b0 ;
        P_13={(~a[31]),a[31],a} ;
        end
      3'b010:
        begin
        inc[26]=1'b0 ;
        inc[27]=1'b0 ;
        P_13={(~a[31]),a[31],a} ;
        end
      3'b100:
        begin
        inc[26]=1'b0 ;
        inc[27]=1'b1 ;
        P_13={(~a_[31]),a_,1'b0} ;
        end
      3'b101:
        begin
        inc[26]=1'b1 ;
        inc[27]=1'b0 ;
        P_13={(~a_[31]),a_[31],a_} ;
        end
      3'b110:
        begin
        inc[26]=1'b1 ;
        inc[27]=1'b0 ;
        P_13={(~a_[31]),a_[31],a_} ;
        end
      3'b011:
        begin
        inc[26]=1'b0 ;
        inc[27]=1'b0 ;
        P_13={(~a[31]),a,1'b0} ;
        end
    endcase

    case({b[29:27]}) 
      3'b000:
        begin
        inc[28]=1'b0 ;
        inc[29]=1'b0 ;
        P_14={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b111:
        begin
        inc[28]=1'b0 ;
        inc[29]=1'b0 ;
        P_14={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b001:
        begin
        inc[28]=1'b0 ;
        inc[29]=1'b0 ;
        P_14={(~a[31]),a[31],a} ;
        end
      3'b010:
        begin
        inc[28]=1'b0 ;
        inc[29]=1'b0 ;
        P_14={(~a[31]),a[31],a} ;
        end
      3'b100:
        begin
        inc[28]=1'b0 ;
        inc[29]=1'b1 ;
        P_14={(~a_[31]),a_,1'b0} ;
        end
      3'b101:
        begin
        inc[28]=1'b1 ;
        inc[29]=1'b0 ;
        P_14={(~a_[31]),a_[31],a_} ;
        end
      3'b110:
        begin
        inc[28]=1'b1 ;
        inc[29]=1'b0 ;
        P_14={(~a_[31]),a_[31],a_} ;
        end
      3'b011:
        begin
        inc[28]=1'b0 ;
        inc[29]=1'b0 ;
        P_14={(~a[31]),a,1'b0} ;
        end
    endcase

    case({b[31:29]}) 
      3'b000:
        begin
        inc[30]=1'b0 ;
        inc[31]=1'b0 ;
        P_15={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b111:
        begin
        inc[30]=1'b0 ;
        inc[31]=1'b0 ;
        P_15={1'b1,{33'b000000000000000000000000000000000}} ;
        end
      3'b001:
        begin
        inc[30]=1'b0 ;
        inc[31]=1'b0 ;
        P_15={(~a[31]),a[31],a} ;
        end
      3'b010:
        begin
        inc[30]=1'b0 ;
        inc[31]=1'b0 ;
        P_15={(~a[31]),a[31],a} ;
        end
      3'b100:
        begin
        inc[30]=1'b0 ;
        inc[31]=1'b1 ;
        P_15={(~a_[31]),a_,1'b0} ;
        end
      3'b101:
        begin
        inc[30]=1'b1 ;
        inc[31]=1'b0 ;
        P_15={(~a_[31]),a_[31],a_} ;
        end
      3'b110:
        begin
        inc[30]=1'b1 ;
        inc[31]=1'b0 ;
        P_15={(~a_[31]),a_[31],a_} ;
        end
      3'b011:
        begin
        inc[30]=1'b0 ;
        inc[31]=1'b0 ;
        P_15={(~a[31]),a,1'b0} ;
        end
    endcase

    end


  // ***** Bit 0 ***** //
  Hadder_ A_0_0( .carry(c[0]), .sum(s[0]), .in2(inc[0]), .in1(P_0[0]) );

  // ***** Bit 1 ***** //
  Hadder_ A_1_1( .carry(c[1]), .sum(s[1]), .in2(inc[1]), .in1(P_0[1]) );

  // ***** Bit 2 ***** //
  Fadder_ A_2_2( .carry(c[2]), .sum(s[2]), .in3(inc[2]), .in2(P_1[0]), .in1(P_0[2]) );

  // ***** Bit 3 ***** //
  Fadder_ A_3_3( .carry(c[3]), .sum(s[3]), .in3(inc[3]), .in2(P_1[1]), .in1(P_0[3]) );

  // ***** Bit 4 ***** //
  Hadder_ A_4_4( .carry(c_4_4), .sum(s_4_4), .in2(inc[4]), .in1(P_2[0]) );
  Fadder_ A_4_5( .carry(c[4]), .sum(s[4]), .in3(P_1[2]), .in2(P_0[4]), .in1(s_4_4) );

  // ***** Bit 5 ***** //
  Fadder_ A_5_6( .carry(c_5_6), .sum(s_5_6), .in3(inc[5]), .in2(P_2[1]), .in1(P_1[3]) );
  Fadder_ A_5_7( .carry(c[5]), .sum(s[5]), .in3(P_0[5]), .in2(c_4_4), .in1(s_5_6) );

  // ***** Bit 6 ***** //
  Hadder_ A_6_8( .carry(c_6_8), .sum(s_6_8), .in2(inc[6]), .in1(P_3[0]) );
  Fadder_ A_6_9( .carry(c_6_9), .sum(s_6_9), .in3(P_2[2]), .in2(P_1[4]), .in1(P_0[6]) );
  Fadder_ A_6_10( .carry(c[6]), .sum(s[6]), .in3(c_5_6), .in2(s_6_9), .in1(s_6_8) );

  // ***** Bit 7 ***** //
  Fadder_ A_7_11( .carry(c_7_11), .sum(s_7_11), .in3(inc[7]), .in2(P_3[1]), .in1(P_2[3]) );
  Fadder_ A_7_12( .carry(c_7_12), .sum(s_7_12), .in3(P_1[5]), .in2(P_0[7]), .in1(c_6_9) );
  Fadder_ A_7_13( .carry(c[7]), .sum(s[7]), .in3(c_6_8), .in2(s_7_11), .in1(s_7_12) );

  // ***** Bit 8 ***** //
  Hadder_ A_8_14( .carry(c_8_14), .sum(s_8_14), .in2(inc[8]), .in1(P_4[0]) );
  Fadder_ A_8_15( .carry(c_8_15), .sum(s_8_15), .in3(P_3[2]), .in2(P_2[4]), .in1(P_1[6]) );
  Fadder_ A_8_16( .carry(c_8_16), .sum(s_8_16), .in3(P_0[8]), .in2(c_7_11), .in1(s_8_15) );
  Fadder_ A_8_17( .carry(c[8]), .sum(s[8]), .in3(s_8_14), .in2(c_7_12), .in1(s_8_16) );

  // ***** Bit 9 ***** //
  Fadder_ A_9_18( .carry(c_9_18), .sum(s_9_18), .in3(inc[9]), .in2(P_4[1]), .in1(P_3[3]) );
  Fadder_ A_9_19( .carry(c_9_19), .sum(s_9_19), .in3(P_2[5]), .in2(P_1[7]), .in1(P_0[9]) );
  Fadder_ A_9_20( .carry(c_9_20), .sum(s_9_20), .in3(c_8_15), .in2(c_8_14), .in1(s_9_19) );
  Fadder_ A_9_21( .carry(c[9]), .sum(s[9]), .in3(s_9_18), .in2(c_8_16), .in1(s_9_20) );

  // ***** Bit 10 ***** //
  Hadder_ A_10_22( .carry(c_10_22), .sum(s_10_22), .in2(inc[10]), .in1(P_5[0]) );
  Fadder_ A_10_23( .carry(c_10_23), .sum(s_10_23), .in3(P_4[2]), .in2(P_3[4]), .in1(P_2[6]) );
  Fadder_ A_10_24( .carry(c_10_24), .sum(s_10_24), .in3(P_1[8]), .in2(P_0[10]), .in1(c_9_19) );
  Fadder_ A_10_25( .carry(c_10_25), .sum(s_10_25), .in3(c_9_18), .in2(s_10_23), .in1(s_10_22) );
  Fadder_ A_10_26( .carry(c[10]), .sum(s[10]), .in3(c_9_20), .in2(s_10_24), .in1(s_10_25) );

  // ***** Bit 11 ***** //
  Fadder_ A_11_27( .carry(c_11_27), .sum(s_11_27), .in3(inc[11]), .in2(P_5[1]), .in1(P_4[3]) );
  Fadder_ A_11_28( .carry(c_11_28), .sum(s_11_28), .in3(P_3[5]), .in2(P_2[7]), .in1(P_1[9]) );
  Fadder_ A_11_29( .carry(c_11_29), .sum(s_11_29), .in3(P_0[11]), .in2(c_10_23), .in1(c_10_22) );
  Fadder_ A_11_30( .carry(c_11_30), .sum(s_11_30), .in3(s_11_28), .in2(s_11_27), .in1(c_10_24) );
  Fadder_ A_11_31( .carry(c[11]), .sum(s[11]), .in3(s_11_29), .in2(c_10_25), .in1(s_11_30) );

  // ***** Bit 12 ***** //
  Hadder_ A_12_32( .carry(c_12_32), .sum(s_12_32), .in2(inc[12]), .in1(P_6[0]) );
  Fadder_ A_12_33( .carry(c_12_33), .sum(s_12_33), .in3(P_5[2]), .in2(P_4[4]), .in1(P_3[6]) );
  Fadder_ A_12_34( .carry(c_12_34), .sum(s_12_34), .in3(P_2[8]), .in2(P_1[10]), .in1(P_0[12]) );
  Fadder_ A_12_35( .carry(c_12_35), .sum(s_12_35), .in3(c_11_28), .in2(c_11_27), .in1(s_12_34) );
  Fadder_ A_12_36( .carry(c_12_36), .sum(s_12_36), .in3(s_12_33), .in2(s_12_32), .in1(c_11_29) );
  Fadder_ A_12_37( .carry(c[12]), .sum(s[12]), .in3(s_12_35), .in2(c_11_30), .in1(s_12_36) );

  // ***** Bit 13 ***** //
  Fadder_ A_13_38( .carry(c_13_38), .sum(s_13_38), .in3(inc[13]), .in2(P_6[1]), .in1(P_5[3]) );
  Fadder_ A_13_39( .carry(c_13_39), .sum(s_13_39), .in3(P_4[5]), .in2(P_3[7]), .in1(P_2[9]) );
  Fadder_ A_13_40( .carry(c_13_40), .sum(s_13_40), .in3(P_1[11]), .in2(P_0[13]), .in1(c_12_34) );
  Fadder_ A_13_41( .carry(c_13_41), .sum(s_13_41), .in3(c_12_33), .in2(c_12_32), .in1(s_13_39) );
  Fadder_ A_13_42( .carry(c_13_42), .sum(s_13_42), .in3(s_13_38), .in2(c_12_35), .in1(s_13_40) );
  Fadder_ A_13_43( .carry(c[13]), .sum(s[13]), .in3(s_13_41), .in2(c_12_36), .in1(s_13_42) );

  // ***** Bit 14 ***** //
  Hadder_ A_14_44( .carry(c_14_44), .sum(s_14_44), .in2(inc[14]), .in1(P_7[0]) );
  Fadder_ A_14_45( .carry(c_14_45), .sum(s_14_45), .in3(P_6[2]), .in2(P_5[4]), .in1(P_4[6]) );
  Fadder_ A_14_46( .carry(c_14_46), .sum(s_14_46), .in3(P_3[8]), .in2(P_2[10]), .in1(P_1[12]) );
  Fadder_ A_14_47( .carry(c_14_47), .sum(s_14_47), .in3(P_0[14]), .in2(c_13_39), .in1(c_13_38) );
  Fadder_ A_14_48( .carry(c_14_48), .sum(s_14_48), .in3(s_14_46), .in2(s_14_45), .in1(s_14_44) );
  Fadder_ A_14_49( .carry(c_14_49), .sum(s_14_49), .in3(c_13_40), .in2(c_13_41), .in1(s_14_47) );
  Fadder_ A_14_50( .carry(c[14]), .sum(s[14]), .in3(s_14_48), .in2(c_13_42), .in1(s_14_49) );

  // ***** Bit 15 ***** //
  Fadder_ A_15_51( .carry(c_15_51), .sum(s_15_51), .in3(inc[15]), .in2(P_7[1]), .in1(P_6[3]) );
  Fadder_ A_15_52( .carry(c_15_52), .sum(s_15_52), .in3(P_5[5]), .in2(P_4[7]), .in1(P_3[9]) );
  Fadder_ A_15_53( .carry(c_15_53), .sum(s_15_53), .in3(P_2[11]), .in2(P_1[13]), .in1(P_0[15]) );
  Fadder_ A_15_54( .carry(c_15_54), .sum(s_15_54), .in3(c_14_46), .in2(c_14_45), .in1(c_14_44) );
  Fadder_ A_15_55( .carry(c_15_55), .sum(s_15_55), .in3(s_15_53), .in2(s_15_52), .in1(s_15_51) );
  Fadder_ A_15_56( .carry(c_15_56), .sum(s_15_56), .in3(c_14_47), .in2(s_15_54), .in1(c_14_48) );
  Fadder_ A_15_57( .carry(c[15]), .sum(s[15]), .in3(s_15_55), .in2(c_14_49), .in1(s_15_56) );

  // ***** Bit 16 ***** //
  Hadder_ A_16_58( .carry(c_16_58), .sum(s_16_58), .in2(inc[16]), .in1(P_8[0]) );
  Fadder_ A_16_59( .carry(c_16_59), .sum(s_16_59), .in3(P_7[2]), .in2(P_6[4]), .in1(P_5[6]) );
  Fadder_ A_16_60( .carry(c_16_60), .sum(s_16_60), .in3(P_4[8]), .in2(P_3[10]), .in1(P_2[12]) );
  Fadder_ A_16_61( .carry(c_16_61), .sum(s_16_61), .in3(P_1[14]), .in2(P_0[16]), .in1(c_15_53) );
  Fadder_ A_16_62( .carry(c_16_62), .sum(s_16_62), .in3(c_15_52), .in2(c_15_51), .in1(s_16_60) );
  Fadder_ A_16_63( .carry(c_16_63), .sum(s_16_63), .in3(s_16_59), .in2(s_16_58), .in1(c_15_54) );
  Fadder_ A_16_64( .carry(c_16_64), .sum(s_16_64), .in3(s_16_61), .in2(c_15_55), .in1(s_16_62) );
  Fadder_ A_16_65( .carry(c[16]), .sum(s[16]), .in3(s_16_63), .in2(c_15_56), .in1(s_16_64) );

  // ***** Bit 17 ***** //
  Fadder_ A_17_66( .carry(c_17_66), .sum(s_17_66), .in3(inc[17]), .in2(P_8[1]), .in1(P_7[3]) );
  Fadder_ A_17_67( .carry(c_17_67), .sum(s_17_67), .in3(P_6[5]), .in2(P_5[7]), .in1(P_4[9]) );
  Fadder_ A_17_68( .carry(c_17_68), .sum(s_17_68), .in3(P_3[11]), .in2(P_2[13]), .in1(P_1[15]) );
  Fadder_ A_17_69( .carry(c_17_69), .sum(s_17_69), .in3(P_0[17]), .in2(c_16_60), .in1(c_16_59) );
  Fadder_ A_17_70( .carry(c_17_70), .sum(s_17_70), .in3(c_16_58), .in2(s_17_68), .in1(s_17_67) );
  Fadder_ A_17_71( .carry(c_17_71), .sum(s_17_71), .in3(s_17_66), .in2(c_16_61), .in1(c_16_62) );
  Fadder_ A_17_72( .carry(c_17_72), .sum(s_17_72), .in3(s_17_69), .in2(s_17_70), .in1(c_16_63) );
  Fadder_ A_17_73( .carry(c[17]), .sum(s[17]), .in3(s_17_71), .in2(c_16_64), .in1(s_17_72) );

  // ***** Bit 18 ***** //
  Hadder_ A_18_74( .carry(c_18_74), .sum(s_18_74), .in2(inc[18]), .in1(P_9[0]) );
  Fadder_ A_18_75( .carry(c_18_75), .sum(s_18_75), .in3(P_8[2]), .in2(P_7[4]), .in1(P_6[6]) );
  Fadder_ A_18_76( .carry(c_18_76), .sum(s_18_76), .in3(P_5[8]), .in2(P_4[10]), .in1(P_3[12]) );
  Fadder_ A_18_77( .carry(c_18_77), .sum(s_18_77), .in3(P_2[14]), .in2(P_1[16]), .in1(P_0[18]) );
  Fadder_ A_18_78( .carry(c_18_78), .sum(s_18_78), .in3(c_17_68), .in2(c_17_67), .in1(c_17_66) );
  Fadder_ A_18_79( .carry(c_18_79), .sum(s_18_79), .in3(s_18_77), .in2(s_18_76), .in1(s_18_75) );
  Fadder_ A_18_80( .carry(c_18_80), .sum(s_18_80), .in3(s_18_74), .in2(c_17_69), .in1(s_18_78) );
  Fadder_ A_18_81( .carry(c_18_81), .sum(s_18_81), .in3(c_17_70), .in2(s_18_79), .in1(c_17_71) );
  Fadder_ A_18_82( .carry(c[18]), .sum(s[18]), .in3(s_18_80), .in2(c_17_72), .in1(s_18_81) );

  // ***** Bit 19 ***** //
  Fadder_ A_19_83( .carry(c_19_83), .sum(s_19_83), .in3(inc[19]), .in2(P_9[1]), .in1(P_8[3]) );
  Fadder_ A_19_84( .carry(c_19_84), .sum(s_19_84), .in3(P_7[5]), .in2(P_6[7]), .in1(P_5[9]) );
  Fadder_ A_19_85( .carry(c_19_85), .sum(s_19_85), .in3(P_4[11]), .in2(P_3[13]), .in1(P_2[15]) );
  Fadder_ A_19_86( .carry(c_19_86), .sum(s_19_86), .in3(P_1[17]), .in2(P_0[19]), .in1(c_18_77) );
  Fadder_ A_19_87( .carry(c_19_87), .sum(s_19_87), .in3(c_18_76), .in2(c_18_75), .in1(c_18_74) );
  Fadder_ A_19_88( .carry(c_19_88), .sum(s_19_88), .in3(s_19_85), .in2(s_19_84), .in1(s_19_83) );
  Fadder_ A_19_89( .carry(c_19_89), .sum(s_19_89), .in3(c_18_78), .in2(s_19_86), .in1(s_19_87) );
  Fadder_ A_19_90( .carry(c_19_90), .sum(s_19_90), .in3(c_18_79), .in2(s_19_88), .in1(c_18_80) );
  Fadder_ A_19_91( .carry(c[19]), .sum(s[19]), .in3(s_19_89), .in2(c_18_81), .in1(s_19_90) );

  // ***** Bit 20 ***** //
  Hadder_ A_20_92( .carry(c_20_92), .sum(s_20_92), .in2(inc[20]), .in1(P_10[0]) );
  Fadder_ A_20_93( .carry(c_20_93), .sum(s_20_93), .in3(P_9[2]), .in2(P_8[4]), .in1(P_7[6]) );
  Fadder_ A_20_94( .carry(c_20_94), .sum(s_20_94), .in3(P_6[8]), .in2(P_5[10]), .in1(P_4[12]) );
  Fadder_ A_20_95( .carry(c_20_95), .sum(s_20_95), .in3(P_3[14]), .in2(P_2[16]), .in1(P_1[18]) );
  Fadder_ A_20_96( .carry(c_20_96), .sum(s_20_96), .in3(P_0[20]), .in2(c_19_85), .in1(c_19_84) );
  Fadder_ A_20_97( .carry(c_20_97), .sum(s_20_97), .in3(c_19_83), .in2(s_20_95), .in1(s_20_94) );
  Fadder_ A_20_98( .carry(c_20_98), .sum(s_20_98), .in3(s_20_93), .in2(s_20_92), .in1(c_19_86) );
  Fadder_ A_20_99( .carry(c_20_99), .sum(s_20_99), .in3(c_19_87), .in2(s_20_96), .in1(c_19_88) );
  Fadder_ A_20_100( .carry(c_20_100), .sum(s_20_100), .in3(s_20_97), .in2(s_20_98), .in1(c_19_89) );
  Fadder_ A_20_101( .carry(c[20]), .sum(s[20]), .in3(s_20_99), .in2(c_19_90), .in1(s_20_100) );

  // ***** Bit 21 ***** //
  Fadder_ A_21_102( .carry(c_21_102), .sum(s_21_102), .in3(inc[21]), .in2(P_10[1]), .in1(P_9[3]) );
  Fadder_ A_21_103( .carry(c_21_103), .sum(s_21_103), .in3(P_8[5]), .in2(P_7[7]), .in1(P_6[9]) );
  Fadder_ A_21_104( .carry(c_21_104), .sum(s_21_104), .in3(P_5[11]), .in2(P_4[13]), .in1(P_3[15]) );
  Fadder_ A_21_105( .carry(c_21_105), .sum(s_21_105), .in3(P_2[17]), .in2(P_1[19]), .in1(P_0[21]) );
  Fadder_ A_21_106( .carry(c_21_106), .sum(s_21_106), .in3(c_20_95), .in2(c_20_94), .in1(c_20_93) );
  Fadder_ A_21_107( .carry(c_21_107), .sum(s_21_107), .in3(c_20_92), .in2(s_21_105), .in1(s_21_104) );
  Fadder_ A_21_108( .carry(c_21_108), .sum(s_21_108), .in3(s_21_103), .in2(s_21_102), .in1(c_20_96) );
  Fadder_ A_21_109( .carry(c_21_109), .sum(s_21_109), .in3(s_21_106), .in2(c_20_97), .in1(s_21_107) );
  Fadder_ A_21_110( .carry(c_21_110), .sum(s_21_110), .in3(c_20_98), .in2(s_21_108), .in1(c_20_99) );
  Fadder_ A_21_111( .carry(c[21]), .sum(s[21]), .in3(s_21_109), .in2(c_20_100), .in1(s_21_110) );

  // ***** Bit 22 ***** //
  Hadder_ A_22_112( .carry(c_22_112), .sum(s_22_112), .in2(inc[22]), .in1(P_11[0]) );
  Fadder_ A_22_113( .carry(c_22_113), .sum(s_22_113), .in3(P_10[2]), .in2(P_9[4]), .in1(P_8[6]) );
  Fadder_ A_22_114( .carry(c_22_114), .sum(s_22_114), .in3(P_7[8]), .in2(P_6[10]), .in1(P_5[12]) );
  Fadder_ A_22_115( .carry(c_22_115), .sum(s_22_115), .in3(P_4[14]), .in2(P_3[16]), .in1(P_2[18]) );
  Fadder_ A_22_116( .carry(c_22_116), .sum(s_22_116), .in3(P_1[20]), .in2(P_0[22]), .in1(c_21_105) );
  Fadder_ A_22_117( .carry(c_22_117), .sum(s_22_117), .in3(c_21_104), .in2(c_21_103), .in1(c_21_102) );
  Fadder_ A_22_118( .carry(c_22_118), .sum(s_22_118), .in3(s_22_115), .in2(s_22_114), .in1(s_22_113) );
  Fadder_ A_22_119( .carry(c_22_119), .sum(s_22_119), .in3(s_22_112), .in2(c_21_106), .in1(s_22_116) );
  Fadder_ A_22_120( .carry(c_22_120), .sum(s_22_120), .in3(s_22_117), .in2(c_21_107), .in1(s_22_118) );
  Fadder_ A_22_121( .carry(c_22_121), .sum(s_22_121), .in3(c_21_108), .in2(c_21_109), .in1(s_22_119) );
  Fadder_ A_22_122( .carry(c[22]), .sum(s[22]), .in3(s_22_120), .in2(c_21_110), .in1(s_22_121) );

  // ***** Bit 23 ***** //
  Fadder_ A_23_123( .carry(c_23_123), .sum(s_23_123), .in3(inc[23]), .in2(P_11[1]), .in1(P_10[3]) );
  Fadder_ A_23_124( .carry(c_23_124), .sum(s_23_124), .in3(P_9[5]), .in2(P_8[7]), .in1(P_7[9]) );
  Fadder_ A_23_125( .carry(c_23_125), .sum(s_23_125), .in3(P_6[11]), .in2(P_5[13]), .in1(P_4[15]) );
  Fadder_ A_23_126( .carry(c_23_126), .sum(s_23_126), .in3(P_3[17]), .in2(P_2[19]), .in1(P_1[21]) );
  Fadder_ A_23_127( .carry(c_23_127), .sum(s_23_127), .in3(P_0[23]), .in2(c_22_115), .in1(c_22_114) );
  Fadder_ A_23_128( .carry(c_23_128), .sum(s_23_128), .in3(c_22_113), .in2(c_22_112), .in1(s_23_126) );
  Fadder_ A_23_129( .carry(c_23_129), .sum(s_23_129), .in3(s_23_125), .in2(s_23_124), .in1(s_23_123) );
  Fadder_ A_23_130( .carry(c_23_130), .sum(s_23_130), .in3(c_22_116), .in2(c_22_117), .in1(s_23_127) );
  Fadder_ A_23_131( .carry(c_23_131), .sum(s_23_131), .in3(c_22_118), .in2(s_23_128), .in1(s_23_129) );
  Fadder_ A_23_132( .carry(c_23_132), .sum(s_23_132), .in3(c_22_119), .in2(c_22_120), .in1(s_23_130) );
  Fadder_ A_23_133( .carry(c[23]), .sum(s[23]), .in3(s_23_131), .in2(c_22_121), .in1(s_23_132) );

  // ***** Bit 24 ***** //
  Hadder_ A_24_134( .carry(c_24_134), .sum(s_24_134), .in2(inc[24]), .in1(P_12[0]) );
  Fadder_ A_24_135( .carry(c_24_135), .sum(s_24_135), .in3(P_11[2]), .in2(P_10[4]), .in1(P_9[6]) );
  Fadder_ A_24_136( .carry(c_24_136), .sum(s_24_136), .in3(P_8[8]), .in2(P_7[10]), .in1(P_6[12]) );
  Fadder_ A_24_137( .carry(c_24_137), .sum(s_24_137), .in3(P_5[14]), .in2(P_4[16]), .in1(P_3[18]) );
  Fadder_ A_24_138( .carry(c_24_138), .sum(s_24_138), .in3(P_2[20]), .in2(P_1[22]), .in1(P_0[24]) );
  Fadder_ A_24_139( .carry(c_24_139), .sum(s_24_139), .in3(c_23_126), .in2(c_23_125), .in1(c_23_124) );
  Fadder_ A_24_140( .carry(c_24_140), .sum(s_24_140), .in3(c_23_123), .in2(s_24_138), .in1(s_24_137) );
  Fadder_ A_24_141( .carry(c_24_141), .sum(s_24_141), .in3(s_24_136), .in2(s_24_135), .in1(s_24_134) );
  Fadder_ A_24_142( .carry(c_24_142), .sum(s_24_142), .in3(c_23_127), .in2(c_23_128), .in1(s_24_139) );
  Fadder_ A_24_143( .carry(c_24_143), .sum(s_24_143), .in3(c_23_129), .in2(s_24_140), .in1(s_24_141) );
  Fadder_ A_24_144( .carry(c_24_144), .sum(s_24_144), .in3(c_23_130), .in2(s_24_142), .in1(c_23_131) );
  Fadder_ A_24_145( .carry(c[24]), .sum(s[24]), .in3(s_24_143), .in2(c_23_132), .in1(s_24_144) );

  // ***** Bit 25 ***** //
  Fadder_ A_25_146( .carry(c_25_146), .sum(s_25_146), .in3(inc[25]), .in2(P_12[1]), .in1(P_11[3]) );
  Fadder_ A_25_147( .carry(c_25_147), .sum(s_25_147), .in3(P_10[5]), .in2(P_9[7]), .in1(P_8[9]) );
  Fadder_ A_25_148( .carry(c_25_148), .sum(s_25_148), .in3(P_7[11]), .in2(P_6[13]), .in1(P_5[15]) );
  Fadder_ A_25_149( .carry(c_25_149), .sum(s_25_149), .in3(P_4[17]), .in2(P_3[19]), .in1(P_2[21]) );
  Fadder_ A_25_150( .carry(c_25_150), .sum(s_25_150), .in3(P_1[23]), .in2(P_0[25]), .in1(c_24_138) );
  Fadder_ A_25_151( .carry(c_25_151), .sum(s_25_151), .in3(c_24_137), .in2(c_24_136), .in1(c_24_135) );
  Fadder_ A_25_152( .carry(c_25_152), .sum(s_25_152), .in3(c_24_134), .in2(s_25_149), .in1(s_25_148) );
  Fadder_ A_25_153( .carry(c_25_153), .sum(s_25_153), .in3(s_25_147), .in2(s_25_146), .in1(c_24_139) );
  Fadder_ A_25_154( .carry(c_25_154), .sum(s_25_154), .in3(s_25_150), .in2(s_25_151), .in1(c_24_141) );
  Fadder_ A_25_155( .carry(c_25_155), .sum(s_25_155), .in3(c_24_140), .in2(s_25_152), .in1(c_24_142) );
  Fadder_ A_25_156( .carry(c_25_156), .sum(s_25_156), .in3(s_25_153), .in2(s_25_154), .in1(c_24_143) );
  Fadder_ A_25_157( .carry(c[25]), .sum(s[25]), .in3(s_25_155), .in2(c_24_144), .in1(s_25_156) );

  // ***** Bit 26 ***** //
  Hadder_ A_26_158( .carry(c_26_158), .sum(s_26_158), .in2(inc[26]), .in1(P_13[0]) );
  Fadder_ A_26_159( .carry(c_26_159), .sum(s_26_159), .in3(P_12[2]), .in2(P_11[4]), .in1(P_10[6]) );
  Fadder_ A_26_160( .carry(c_26_160), .sum(s_26_160), .in3(P_9[8]), .in2(P_8[10]), .in1(P_7[12]) );
  Fadder_ A_26_161( .carry(c_26_161), .sum(s_26_161), .in3(P_6[14]), .in2(P_5[16]), .in1(P_4[18]) );
  Fadder_ A_26_162( .carry(c_26_162), .sum(s_26_162), .in3(P_3[20]), .in2(P_2[22]), .in1(P_1[24]) );
  Fadder_ A_26_163( .carry(c_26_163), .sum(s_26_163), .in3(P_0[26]), .in2(c_25_149), .in1(c_25_148) );
  Fadder_ A_26_164( .carry(c_26_164), .sum(s_26_164), .in3(c_25_147), .in2(c_25_146), .in1(s_26_162) );
  Fadder_ A_26_165( .carry(c_26_165), .sum(s_26_165), .in3(s_26_161), .in2(s_26_160), .in1(s_26_159) );
  Fadder_ A_26_166( .carry(c_26_166), .sum(s_26_166), .in3(s_26_158), .in2(c_25_150), .in1(c_25_151) );
  Fadder_ A_26_167( .carry(c_26_167), .sum(s_26_167), .in3(s_26_163), .in2(c_25_152), .in1(s_26_164) );
  Fadder_ A_26_168( .carry(c_26_168), .sum(s_26_168), .in3(s_26_165), .in2(c_25_153), .in1(s_26_166) );
  Fadder_ A_26_169( .carry(c_26_169), .sum(s_26_169), .in3(c_25_154), .in2(s_26_167), .in1(c_25_155) );
  Fadder_ A_26_170( .carry(c[26]), .sum(s[26]), .in3(s_26_168), .in2(c_25_156), .in1(s_26_169) );

  // ***** Bit 27 ***** //
  Fadder_ A_27_171( .carry(c_27_171), .sum(s_27_171), .in3(inc[27]), .in2(P_13[1]), .in1(P_12[3]) );
  Fadder_ A_27_172( .carry(c_27_172), .sum(s_27_172), .in3(P_11[5]), .in2(P_10[7]), .in1(P_9[9]) );
  Fadder_ A_27_173( .carry(c_27_173), .sum(s_27_173), .in3(P_8[11]), .in2(P_7[13]), .in1(P_6[15]) );
  Fadder_ A_27_174( .carry(c_27_174), .sum(s_27_174), .in3(P_5[17]), .in2(P_4[19]), .in1(P_3[21]) );
  Fadder_ A_27_175( .carry(c_27_175), .sum(s_27_175), .in3(P_2[23]), .in2(P_1[25]), .in1(P_0[27]) );
  Fadder_ A_27_176( .carry(c_27_176), .sum(s_27_176), .in3(c_26_162), .in2(c_26_161), .in1(c_26_160) );
  Fadder_ A_27_177( .carry(c_27_177), .sum(s_27_177), .in3(c_26_159), .in2(c_26_158), .in1(s_27_175) );
  Fadder_ A_27_178( .carry(c_27_178), .sum(s_27_178), .in3(s_27_174), .in2(s_27_173), .in1(s_27_172) );
  Fadder_ A_27_179( .carry(c_27_179), .sum(s_27_179), .in3(s_27_171), .in2(c_26_163), .in1(c_26_164) );
  Fadder_ A_27_180( .carry(c_27_180), .sum(s_27_180), .in3(s_27_176), .in2(c_26_165), .in1(s_27_177) );
  Fadder_ A_27_181( .carry(c_27_181), .sum(s_27_181), .in3(s_27_178), .in2(c_26_166), .in1(s_27_179) );
  Fadder_ A_27_182( .carry(c_27_182), .sum(s_27_182), .in3(c_26_167), .in2(s_27_180), .in1(c_26_168) );
  Fadder_ A_27_183( .carry(c[27]), .sum(s[27]), .in3(s_27_181), .in2(c_26_169), .in1(s_27_182) );

  // ***** Bit 28 ***** //
  Hadder_ A_28_184( .carry(c_28_184), .sum(s_28_184), .in2(inc[28]), .in1(P_14[0]) );
  Fadder_ A_28_185( .carry(c_28_185), .sum(s_28_185), .in3(P_13[2]), .in2(P_12[4]), .in1(P_11[6]) );
  Fadder_ A_28_186( .carry(c_28_186), .sum(s_28_186), .in3(P_10[8]), .in2(P_9[10]), .in1(P_8[12]) );
  Fadder_ A_28_187( .carry(c_28_187), .sum(s_28_187), .in3(P_7[14]), .in2(P_6[16]), .in1(P_5[18]) );
  Fadder_ A_28_188( .carry(c_28_188), .sum(s_28_188), .in3(P_4[20]), .in2(P_3[22]), .in1(P_2[24]) );
  Fadder_ A_28_189( .carry(c_28_189), .sum(s_28_189), .in3(P_1[26]), .in2(P_0[28]), .in1(c_27_175) );
  Fadder_ A_28_190( .carry(c_28_190), .sum(s_28_190), .in3(c_27_174), .in2(c_27_173), .in1(c_27_172) );
  Fadder_ A_28_191( .carry(c_28_191), .sum(s_28_191), .in3(c_27_171), .in2(s_28_188), .in1(s_28_187) );
  Fadder_ A_28_192( .carry(c_28_192), .sum(s_28_192), .in3(s_28_186), .in2(s_28_185), .in1(s_28_184) );
  Fadder_ A_28_193( .carry(c_28_193), .sum(s_28_193), .in3(c_27_176), .in2(c_27_177), .in1(s_28_189) );
  Fadder_ A_28_194( .carry(c_28_194), .sum(s_28_194), .in3(s_28_190), .in2(c_27_178), .in1(s_28_191) );
  Fadder_ A_28_195( .carry(c_28_195), .sum(s_28_195), .in3(s_28_192), .in2(c_27_179), .in1(c_27_180) );
  Fadder_ A_28_196( .carry(c_28_196), .sum(s_28_196), .in3(s_28_193), .in2(s_28_194), .in1(c_27_181) );
  Fadder_ A_28_197( .carry(c[28]), .sum(s[28]), .in3(s_28_195), .in2(c_27_182), .in1(s_28_196) );

  // ***** Bit 29 ***** //
  Fadder_ A_29_198( .carry(c_29_198), .sum(s_29_198), .in3(inc[29]), .in2(P_14[1]), .in1(P_13[3]) );
  Fadder_ A_29_199( .carry(c_29_199), .sum(s_29_199), .in3(P_12[5]), .in2(P_11[7]), .in1(P_10[9]) );
  Fadder_ A_29_200( .carry(c_29_200), .sum(s_29_200), .in3(P_9[11]), .in2(P_8[13]), .in1(P_7[15]) );
  Fadder_ A_29_201( .carry(c_29_201), .sum(s_29_201), .in3(P_6[17]), .in2(P_5[19]), .in1(P_4[21]) );
  Fadder_ A_29_202( .carry(c_29_202), .sum(s_29_202), .in3(P_3[23]), .in2(P_2[25]), .in1(P_1[27]) );
  Fadder_ A_29_203( .carry(c_29_203), .sum(s_29_203), .in3(P_0[29]), .in2(c_28_188), .in1(c_28_187) );
  Fadder_ A_29_204( .carry(c_29_204), .sum(s_29_204), .in3(c_28_186), .in2(c_28_185), .in1(c_28_184) );
  Fadder_ A_29_205( .carry(c_29_205), .sum(s_29_205), .in3(s_29_202), .in2(s_29_201), .in1(s_29_200) );
  Fadder_ A_29_206( .carry(c_29_206), .sum(s_29_206), .in3(s_29_199), .in2(s_29_198), .in1(c_28_189) );
  Fadder_ A_29_207( .carry(c_29_207), .sum(s_29_207), .in3(c_28_190), .in2(s_29_203), .in1(s_29_204) );
  Fadder_ A_29_208( .carry(c_29_208), .sum(s_29_208), .in3(c_28_192), .in2(c_28_191), .in1(s_29_205) );
  Fadder_ A_29_209( .carry(c_29_209), .sum(s_29_209), .in3(s_29_206), .in2(c_28_193), .in1(c_28_194) );
  Fadder_ A_29_210( .carry(c_29_210), .sum(s_29_210), .in3(s_29_207), .in2(s_29_208), .in1(c_28_195) );
  Fadder_ A_29_211( .carry(c[29]), .sum(s[29]), .in3(s_29_209), .in2(c_28_196), .in1(s_29_210) );

  // ***** Bit 30 ***** //
  Hadder_ A_30_212( .carry(c_30_212), .sum(s_30_212), .in2(inc[30]), .in1(P_15[0]) );
  Fadder_ A_30_213( .carry(c_30_213), .sum(s_30_213), .in3(P_14[2]), .in2(P_13[4]), .in1(P_12[6]) );
  Fadder_ A_30_214( .carry(c_30_214), .sum(s_30_214), .in3(P_11[8]), .in2(P_10[10]), .in1(P_9[12]) );
  Fadder_ A_30_215( .carry(c_30_215), .sum(s_30_215), .in3(P_8[14]), .in2(P_7[16]), .in1(P_6[18]) );
  Fadder_ A_30_216( .carry(c_30_216), .sum(s_30_216), .in3(P_5[20]), .in2(P_4[22]), .in1(P_3[24]) );
  Fadder_ A_30_217( .carry(c_30_217), .sum(s_30_217), .in3(P_2[26]), .in2(P_1[28]), .in1(P_0[30]) );
  Fadder_ A_30_218( .carry(c_30_218), .sum(s_30_218), .in3(c_29_202), .in2(c_29_201), .in1(c_29_200) );
  Fadder_ A_30_219( .carry(c_30_219), .sum(s_30_219), .in3(c_29_199), .in2(c_29_198), .in1(s_30_217) );
  Fadder_ A_30_220( .carry(c_30_220), .sum(s_30_220), .in3(s_30_216), .in2(s_30_215), .in1(s_30_214) );
  Fadder_ A_30_221( .carry(c_30_221), .sum(s_30_221), .in3(s_30_213), .in2(s_30_212), .in1(c_29_204) );
  Fadder_ A_30_222( .carry(c_30_222), .sum(s_30_222), .in3(c_29_203), .in2(s_30_218), .in1(c_29_205) );
  Fadder_ A_30_223( .carry(c_30_223), .sum(s_30_223), .in3(s_30_219), .in2(s_30_220), .in1(c_29_206) );
  Fadder_ A_30_224( .carry(c_30_224), .sum(s_30_224), .in3(s_30_221), .in2(c_29_207), .in1(c_29_208) );
  Fadder_ A_30_225( .carry(c_30_225), .sum(s_30_225), .in3(s_30_222), .in2(s_30_223), .in1(c_29_209) );
  Fadder_ A_30_226( .carry(c[30]), .sum(s[30]), .in3(s_30_224), .in2(c_29_210), .in1(s_30_225) );

  // ***** Bit 31 ***** //
  Fadder_ A_31_227( .carry(c_31_227), .sum(s_31_227), .in3(inc[31]), .in2(P_15[1]), .in1(P_14[3]) );
  Fadder_ A_31_228( .carry(c_31_228), .sum(s_31_228), .in3(P_13[5]), .in2(P_12[7]), .in1(P_11[9]) );
  Fadder_ A_31_229( .carry(c_31_229), .sum(s_31_229), .in3(P_10[11]), .in2(P_9[13]), .in1(P_8[15]) );
  Fadder_ A_31_230( .carry(c_31_230), .sum(s_31_230), .in3(P_7[17]), .in2(P_6[19]), .in1(P_5[21]) );
  Fadder_ A_31_231( .carry(c_31_231), .sum(s_31_231), .in3(P_4[23]), .in2(P_3[25]), .in1(P_2[27]) );
  Fadder_ A_31_232( .carry(c_31_232), .sum(s_31_232), .in3(P_1[29]), .in2(P_0[31]), .in1(c_30_217) );
  Fadder_ A_31_233( .carry(c_31_233), .sum(s_31_233), .in3(c_30_216), .in2(c_30_215), .in1(c_30_214) );
  Fadder_ A_31_234( .carry(c_31_234), .sum(s_31_234), .in3(c_30_213), .in2(c_30_212), .in1(s_31_231) );
  Fadder_ A_31_235( .carry(c_31_235), .sum(s_31_235), .in3(s_31_230), .in2(s_31_229), .in1(s_31_228) );
  Fadder_ A_31_236( .carry(c_31_236), .sum(s_31_236), .in3(s_31_227), .in2(c_30_218), .in1(c_30_219) );
  Fadder_ A_31_237( .carry(c_31_237), .sum(s_31_237), .in3(s_31_232), .in2(s_31_233), .in1(c_30_220) );
  Fadder_ A_31_238( .carry(c_31_238), .sum(s_31_238), .in3(s_31_234), .in2(s_31_235), .in1(c_30_221) );
  Fadder_ A_31_239( .carry(c_31_239), .sum(s_31_239), .in3(c_30_222), .in2(s_31_236), .in1(s_31_237) );
  Fadder_ A_31_240( .carry(c_31_240), .sum(s_31_240), .in3(c_30_223), .in2(s_31_238), .in1(c_30_224) );
  Fadder_ A_31_241( .carry(c[31]), .sum(s[31]), .in3(s_31_239), .in2(c_30_225), .in1(s_31_240) );

  // ***** Bit 32 ***** //
  Hadder_ A_32_242( .carry(c_32_242), .sum(s_32_242), .in2(P_15[2]), .in1(P_14[4]) );
  Fadder_ A_32_243( .carry(c_32_243), .sum(s_32_243), .in3(P_13[6]), .in2(P_12[8]), .in1(P_11[10]) );
  Fadder_ A_32_244( .carry(c_32_244), .sum(s_32_244), .in3(P_10[12]), .in2(P_9[14]), .in1(P_8[16]) );
  Fadder_ A_32_245( .carry(c_32_245), .sum(s_32_245), .in3(P_7[18]), .in2(P_6[20]), .in1(P_5[22]) );
  Fadder_ A_32_246( .carry(c_32_246), .sum(s_32_246), .in3(P_4[24]), .in2(P_3[26]), .in1(P_2[28]) );
  Fadder_ A_32_247( .carry(c_32_247), .sum(s_32_247), .in3(P_1[30]), .in2(P_0[32]), .in1(c_31_231) );
  Fadder_ A_32_248( .carry(c_32_248), .sum(s_32_248), .in3(c_31_230), .in2(c_31_229), .in1(c_31_228) );
  Fadder_ A_32_249( .carry(c_32_249), .sum(s_32_249), .in3(c_31_227), .in2(s_32_246), .in1(s_32_245) );
  Fadder_ A_32_250( .carry(c_32_250), .sum(s_32_250), .in3(s_32_244), .in2(s_32_243), .in1(s_32_242) );
  Fadder_ A_32_251( .carry(c_32_251), .sum(s_32_251), .in3(c_31_232), .in2(c_31_233), .in1(c_31_234) );
  Fadder_ A_32_252( .carry(c_32_252), .sum(s_32_252), .in3(s_32_247), .in2(s_32_248), .in1(c_31_235) );
  Fadder_ A_32_253( .carry(c_32_253), .sum(s_32_253), .in3(s_32_249), .in2(s_32_250), .in1(c_31_236) );
  Fadder_ A_32_254( .carry(c_32_254), .sum(s_32_254), .in3(c_31_237), .in2(s_32_251), .in1(s_32_252) );
  Fadder_ A_32_255( .carry(c_32_255), .sum(s_32_255), .in3(c_31_238), .in2(s_32_253), .in1(c_31_239) );
  Fadder_ A_32_256( .carry(c[32]), .sum(s[32]), .in3(s_32_254), .in2(c_31_240), .in1(s_32_255) );

  // ***** Bit 33 ***** //
  Fadder_ A_33_257( .carry(c_33_257), .sum(s_33_257), .in3(sign_), .in2(P_15[3]), .in1(P_14[5]) );
  Fadder_ A_33_258( .carry(c_33_258), .sum(s_33_258), .in3(P_13[7]), .in2(P_12[9]), .in1(P_11[11]) );
  Fadder_ A_33_259( .carry(c_33_259), .sum(s_33_259), .in3(P_10[13]), .in2(P_9[15]), .in1(P_8[17]) );
  Fadder_ A_33_260( .carry(c_33_260), .sum(s_33_260), .in3(P_7[19]), .in2(P_6[21]), .in1(P_5[23]) );
  Fadder_ A_33_261( .carry(c_33_261), .sum(s_33_261), .in3(P_4[25]), .in2(P_3[27]), .in1(P_2[29]) );
  Fadder_ A_33_262( .carry(c_33_262), .sum(s_33_262), .in3(P_1[31]), .in2(P_0[33]), .in1(c_32_246) );
  Fadder_ A_33_263( .carry(c_33_263), .sum(s_33_263), .in3(c_32_245), .in2(c_32_244), .in1(c_32_243) );
  Fadder_ A_33_264( .carry(c_33_264), .sum(s_33_264), .in3(c_32_242), .in2(s_33_257), .in1(s_33_261) );
  Fadder_ A_33_265( .carry(c_33_265), .sum(s_33_265), .in3(s_33_260), .in2(s_33_259), .in1(s_33_258) );
  Fadder_ A_33_266( .carry(c_33_266), .sum(s_33_266), .in3(c_32_247), .in2(c_32_248), .in1(s_33_262) );
  Fadder_ A_33_267( .carry(c_33_267), .sum(s_33_267), .in3(s_33_263), .in2(c_32_250), .in1(c_32_249) );
  Fadder_ A_33_268( .carry(c_33_268), .sum(s_33_268), .in3(s_33_264), .in2(s_33_265), .in1(c_32_251) );
  Fadder_ A_33_269( .carry(c_33_269), .sum(s_33_269), .in3(c_32_252), .in2(s_33_266), .in1(s_33_267) );
  Fadder_ A_33_270( .carry(c_33_270), .sum(s_33_270), .in3(c_32_253), .in2(s_33_268), .in1(c_32_254) );
  Fadder_ A_33_271( .carry(c[33]), .sum(s[33]), .in3(s_33_269), .in2(c_32_255), .in1(s_33_270) );

  // ***** Bit 34 ***** //
  Hadder_ A_34_272( .carry(c_34_272), .sum(s_34_272), .in2(sign_), .in1(P_15[4]) );
  Fadder_ A_34_273( .carry(c_34_273), .sum(s_34_273), .in3(P_14[6]), .in2(P_13[8]), .in1(P_12[10]) );
  Fadder_ A_34_274( .carry(c_34_274), .sum(s_34_274), .in3(P_11[12]), .in2(P_10[14]), .in1(P_9[16]) );
  Fadder_ A_34_275( .carry(c_34_275), .sum(s_34_275), .in3(P_8[18]), .in2(P_7[20]), .in1(P_6[22]) );
  Fadder_ A_34_276( .carry(c_34_276), .sum(s_34_276), .in3(P_5[24]), .in2(P_4[26]), .in1(P_3[28]) );
  Fadder_ A_34_277( .carry(c_34_277), .sum(s_34_277), .in3(P_2[30]), .in2(P_1[32]), .in1(c_33_261) );
  Fadder_ A_34_278( .carry(c_34_278), .sum(s_34_278), .in3(c_33_260), .in2(c_33_259), .in1(c_33_258) );
  Fadder_ A_34_279( .carry(c_34_279), .sum(s_34_279), .in3(c_33_257), .in2(s_34_272), .in1(s_34_276) );
  Fadder_ A_34_280( .carry(c_34_280), .sum(s_34_280), .in3(s_34_275), .in2(s_34_274), .in1(s_34_273) );
  Fadder_ A_34_281( .carry(c_34_281), .sum(s_34_281), .in3(c_33_262), .in2(c_33_263), .in1(s_34_277) );
  Fadder_ A_34_282( .carry(c_34_282), .sum(s_34_282), .in3(c_33_264), .in2(s_34_278), .in1(c_33_265) );
  Fadder_ A_34_283( .carry(c_34_283), .sum(s_34_283), .in3(s_34_279), .in2(s_34_280), .in1(c_33_266) );
  Fadder_ A_34_284( .carry(c_34_284), .sum(s_34_284), .in3(c_33_267), .in2(s_34_281), .in1(s_34_282) );
  Fadder_ A_34_285( .carry(c_34_285), .sum(s_34_285), .in3(c_33_268), .in2(s_34_283), .in1(c_33_269) );
  Fadder_ A_34_286( .carry(c[34]), .sum(s[34]), .in3(s_34_284), .in2(c_33_270), .in1(s_34_285) );

  // ***** Bit 35 ***** //
  Fadder_ A_35_287( .carry(c_35_287), .sum(s_35_287), .in3(P_15[5]), .in2(P_14[7]), .in1(P_13[9]) );
  Fadder_ A_35_288( .carry(c_35_288), .sum(s_35_288), .in3(P_12[11]), .in2(P_11[13]), .in1(P_10[15]) );
  Fadder_ A_35_289( .carry(c_35_289), .sum(s_35_289), .in3(P_9[17]), .in2(P_8[19]), .in1(P_7[21]) );
  Fadder_ A_35_290( .carry(c_35_290), .sum(s_35_290), .in3(P_6[23]), .in2(P_5[25]), .in1(P_4[27]) );
  Fadder_ A_35_291( .carry(c_35_291), .sum(s_35_291), .in3(P_3[29]), .in2(P_2[31]), .in1(P_1[33]) );
  Fadder_ A_35_292( .carry(c_35_292), .sum(s_35_292), .in3(c_34_276), .in2(c_34_275), .in1(c_34_274) );
  Fadder_ A_35_293( .carry(c_35_293), .sum(s_35_293), .in3(c_34_273), .in2(c_34_272), .in1(s_35_291) );
  Fadder_ A_35_294( .carry(c_35_294), .sum(s_35_294), .in3(s_35_290), .in2(s_35_289), .in1(s_35_288) );
  Fadder_ A_35_295( .carry(c_35_295), .sum(s_35_295), .in3(s_35_287), .in2(c_34_277), .in1(c_34_278) );
  Fadder_ A_35_296( .carry(c_35_296), .sum(s_35_296), .in3(c_34_279), .in2(s_35_292), .in1(c_34_280) );
  Fadder_ A_35_297( .carry(c_35_297), .sum(s_35_297), .in3(s_35_293), .in2(s_35_294), .in1(c_34_281) );
  Fadder_ A_35_298( .carry(c_35_298), .sum(s_35_298), .in3(s_35_295), .in2(c_34_282), .in1(s_35_296) );
  Fadder_ A_35_299( .carry(c_35_299), .sum(s_35_299), .in3(c_34_283), .in2(s_35_297), .in1(c_34_284) );
  Fadder_ A_35_300( .carry(c[35]), .sum(s[35]), .in3(s_35_298), .in2(c_34_285), .in1(s_35_299) );

  // ***** Bit 36 ***** //
  Hadder_ A_36_301( .carry(c_36_301), .sum(s_36_301), .in2(sign_), .in1(P_15[6]) );
  Fadder_ A_36_302( .carry(c_36_302), .sum(s_36_302), .in3(P_14[8]), .in2(P_13[10]), .in1(P_12[12]) );
  Fadder_ A_36_303( .carry(c_36_303), .sum(s_36_303), .in3(P_11[14]), .in2(P_10[16]), .in1(P_9[18]) );
  Fadder_ A_36_304( .carry(c_36_304), .sum(s_36_304), .in3(P_8[20]), .in2(P_7[22]), .in1(P_6[24]) );
  Fadder_ A_36_305( .carry(c_36_305), .sum(s_36_305), .in3(P_5[26]), .in2(P_4[28]), .in1(P_3[30]) );
  Fadder_ A_36_306( .carry(c_36_306), .sum(s_36_306), .in3(P_2[32]), .in2(c_35_291), .in1(c_35_290) );
  Fadder_ A_36_307( .carry(c_36_307), .sum(s_36_307), .in3(c_35_289), .in2(c_35_288), .in1(c_35_287) );
  Fadder_ A_36_308( .carry(c_36_308), .sum(s_36_308), .in3(s_36_301), .in2(s_36_305), .in1(s_36_304) );
  Fadder_ A_36_309( .carry(c_36_309), .sum(s_36_309), .in3(s_36_303), .in2(s_36_302), .in1(c_35_292) );
  Fadder_ A_36_310( .carry(c_36_310), .sum(s_36_310), .in3(c_35_293), .in2(s_36_306), .in1(s_36_307) );
  Fadder_ A_36_311( .carry(c_36_311), .sum(s_36_311), .in3(c_35_294), .in2(s_36_308), .in1(c_35_295) );
  Fadder_ A_36_312( .carry(c_36_312), .sum(s_36_312), .in3(s_36_309), .in2(c_35_296), .in1(s_36_310) );
  Fadder_ A_36_313( .carry(c_36_313), .sum(s_36_313), .in3(c_35_297), .in2(s_36_311), .in1(c_35_298) );
  Fadder_ A_36_314( .carry(c[36]), .sum(s[36]), .in3(s_36_312), .in2(c_35_299), .in1(s_36_313) );

  // ***** Bit 37 ***** //
  Fadder_ A_37_315( .carry(c_37_315), .sum(s_37_315), .in3(P_15[7]), .in2(P_14[9]), .in1(P_13[11]) );
  Fadder_ A_37_316( .carry(c_37_316), .sum(s_37_316), .in3(P_12[13]), .in2(P_11[15]), .in1(P_10[17]) );
  Fadder_ A_37_317( .carry(c_37_317), .sum(s_37_317), .in3(P_9[19]), .in2(P_8[21]), .in1(P_7[23]) );
  Fadder_ A_37_318( .carry(c_37_318), .sum(s_37_318), .in3(P_6[25]), .in2(P_5[27]), .in1(P_4[29]) );
  Fadder_ A_37_319( .carry(c_37_319), .sum(s_37_319), .in3(P_3[31]), .in2(P_2[33]), .in1(c_36_305) );
  Fadder_ A_37_320( .carry(c_37_320), .sum(s_37_320), .in3(c_36_304), .in2(c_36_303), .in1(c_36_302) );
  Fadder_ A_37_321( .carry(c_37_321), .sum(s_37_321), .in3(c_36_301), .in2(s_37_318), .in1(s_37_317) );
  Fadder_ A_37_322( .carry(c_37_322), .sum(s_37_322), .in3(s_37_316), .in2(s_37_315), .in1(c_36_307) );
  Fadder_ A_37_323( .carry(c_37_323), .sum(s_37_323), .in3(c_36_306), .in2(s_37_319), .in1(s_37_320) );
  Fadder_ A_37_324( .carry(c_37_324), .sum(s_37_324), .in3(c_36_308), .in2(s_37_321), .in1(c_36_309) );
  Fadder_ A_37_325( .carry(c_37_325), .sum(s_37_325), .in3(s_37_322), .in2(c_36_310), .in1(s_37_323) );
  Fadder_ A_37_326( .carry(c_37_326), .sum(s_37_326), .in3(c_36_311), .in2(s_37_324), .in1(c_36_312) );
  Fadder_ A_37_327( .carry(c[37]), .sum(s[37]), .in3(s_37_325), .in2(c_36_313), .in1(s_37_326) );

  // ***** Bit 38 ***** //
  Hadder_ A_38_328( .carry(c_38_328), .sum(s_38_328), .in2(sign_), .in1(P_15[8]) );
  Fadder_ A_38_329( .carry(c_38_329), .sum(s_38_329), .in3(P_14[10]), .in2(P_13[12]), .in1(P_12[14]) );
  Fadder_ A_38_330( .carry(c_38_330), .sum(s_38_330), .in3(P_11[16]), .in2(P_10[18]), .in1(P_9[20]) );
  Fadder_ A_38_331( .carry(c_38_331), .sum(s_38_331), .in3(P_8[22]), .in2(P_7[24]), .in1(P_6[26]) );
  Fadder_ A_38_332( .carry(c_38_332), .sum(s_38_332), .in3(P_5[28]), .in2(P_4[30]), .in1(P_3[32]) );
  Fadder_ A_38_333( .carry(c_38_333), .sum(s_38_333), .in3(c_37_318), .in2(c_37_317), .in1(c_37_316) );
  Fadder_ A_38_334( .carry(c_38_334), .sum(s_38_334), .in3(c_37_315), .in2(s_38_328), .in1(s_38_332) );
  Fadder_ A_38_335( .carry(c_38_335), .sum(s_38_335), .in3(s_38_331), .in2(s_38_330), .in1(s_38_329) );
  Fadder_ A_38_336( .carry(c_38_336), .sum(s_38_336), .in3(c_37_319), .in2(c_37_320), .in1(s_38_333) );
  Fadder_ A_38_337( .carry(c_38_337), .sum(s_38_337), .in3(c_37_321), .in2(s_38_334), .in1(s_38_335) );
  Fadder_ A_38_338( .carry(c_38_338), .sum(s_38_338), .in3(c_37_322), .in2(c_37_323), .in1(s_38_336) );
  Fadder_ A_38_339( .carry(c_38_339), .sum(s_38_339), .in3(s_38_337), .in2(c_37_324), .in1(c_37_325) );
  Fadder_ A_38_340( .carry(c[38]), .sum(s[38]), .in3(s_38_338), .in2(c_37_326), .in1(s_38_339) );

  // ***** Bit 39 ***** //
  Fadder_ A_39_341( .carry(c_39_341), .sum(s_39_341), .in3(P_15[9]), .in2(P_14[11]), .in1(P_13[13]) );
  Fadder_ A_39_342( .carry(c_39_342), .sum(s_39_342), .in3(P_12[15]), .in2(P_11[17]), .in1(P_10[19]) );
  Fadder_ A_39_343( .carry(c_39_343), .sum(s_39_343), .in3(P_9[21]), .in2(P_8[23]), .in1(P_7[25]) );
  Fadder_ A_39_344( .carry(c_39_344), .sum(s_39_344), .in3(P_6[27]), .in2(P_5[29]), .in1(P_4[31]) );
  Fadder_ A_39_345( .carry(c_39_345), .sum(s_39_345), .in3(P_3[33]), .in2(c_38_332), .in1(c_38_331) );
  Fadder_ A_39_346( .carry(c_39_346), .sum(s_39_346), .in3(c_38_330), .in2(c_38_329), .in1(c_38_328) );
  Fadder_ A_39_347( .carry(c_39_347), .sum(s_39_347), .in3(s_39_344), .in2(s_39_343), .in1(s_39_342) );
  Fadder_ A_39_348( .carry(c_39_348), .sum(s_39_348), .in3(s_39_341), .in2(c_38_333), .in1(s_39_345) );
  Fadder_ A_39_349( .carry(c_39_349), .sum(s_39_349), .in3(c_38_334), .in2(s_39_346), .in1(c_38_335) );
  Fadder_ A_39_350( .carry(c_39_350), .sum(s_39_350), .in3(s_39_347), .in2(c_38_336), .in1(s_39_348) );
  Fadder_ A_39_351( .carry(c_39_351), .sum(s_39_351), .in3(s_39_349), .in2(c_38_337), .in1(c_38_338) );
  Fadder_ A_39_352( .carry(c[39]), .sum(s[39]), .in3(s_39_350), .in2(c_38_339), .in1(s_39_351) );

  // ***** Bit 40 ***** //
  Hadder_ A_40_353( .carry(c_40_353), .sum(s_40_353), .in2(sign_), .in1(P_15[10]) );
  Fadder_ A_40_354( .carry(c_40_354), .sum(s_40_354), .in3(P_14[12]), .in2(P_13[14]), .in1(P_12[16]) );
  Fadder_ A_40_355( .carry(c_40_355), .sum(s_40_355), .in3(P_11[18]), .in2(P_10[20]), .in1(P_9[22]) );
  Fadder_ A_40_356( .carry(c_40_356), .sum(s_40_356), .in3(P_8[24]), .in2(P_7[26]), .in1(P_6[28]) );
  Fadder_ A_40_357( .carry(c_40_357), .sum(s_40_357), .in3(P_5[30]), .in2(P_4[32]), .in1(c_39_344) );
  Fadder_ A_40_358( .carry(c_40_358), .sum(s_40_358), .in3(c_39_343), .in2(c_39_342), .in1(c_39_341) );
  Fadder_ A_40_359( .carry(c_40_359), .sum(s_40_359), .in3(s_40_353), .in2(s_40_356), .in1(s_40_355) );
  Fadder_ A_40_360( .carry(c_40_360), .sum(s_40_360), .in3(s_40_354), .in2(c_39_346), .in1(c_39_345) );
  Fadder_ A_40_361( .carry(c_40_361), .sum(s_40_361), .in3(s_40_357), .in2(s_40_358), .in1(c_39_347) );
  Fadder_ A_40_362( .carry(c_40_362), .sum(s_40_362), .in3(s_40_359), .in2(c_39_348), .in1(s_40_360) );
  Fadder_ A_40_363( .carry(c_40_363), .sum(s_40_363), .in3(c_39_349), .in2(s_40_361), .in1(c_39_350) );
  Fadder_ A_40_364( .carry(c[40]), .sum(s[40]), .in3(s_40_362), .in2(c_39_351), .in1(s_40_363) );

  // ***** Bit 41 ***** //
  Fadder_ A_41_365( .carry(c_41_365), .sum(s_41_365), .in3(P_15[11]), .in2(P_14[13]), .in1(P_13[15]) );
  Fadder_ A_41_366( .carry(c_41_366), .sum(s_41_366), .in3(P_12[17]), .in2(P_11[19]), .in1(P_10[21]) );
  Fadder_ A_41_367( .carry(c_41_367), .sum(s_41_367), .in3(P_9[23]), .in2(P_8[25]), .in1(P_7[27]) );
  Fadder_ A_41_368( .carry(c_41_368), .sum(s_41_368), .in3(P_6[29]), .in2(P_5[31]), .in1(P_4[33]) );
  Fadder_ A_41_369( .carry(c_41_369), .sum(s_41_369), .in3(c_40_356), .in2(c_40_355), .in1(c_40_354) );
  Fadder_ A_41_370( .carry(c_41_370), .sum(s_41_370), .in3(c_40_353), .in2(s_41_368), .in1(s_41_367) );
  Fadder_ A_41_371( .carry(c_41_371), .sum(s_41_371), .in3(s_41_366), .in2(s_41_365), .in1(c_40_357) );
  Fadder_ A_41_372( .carry(c_41_372), .sum(s_41_372), .in3(c_40_358), .in2(s_41_369), .in1(c_40_359) );
  Fadder_ A_41_373( .carry(c_41_373), .sum(s_41_373), .in3(s_41_370), .in2(c_40_360), .in1(s_41_371) );
  Fadder_ A_41_374( .carry(c_41_374), .sum(s_41_374), .in3(c_40_361), .in2(s_41_372), .in1(c_40_362) );
  Fadder_ A_41_375( .carry(c[41]), .sum(s[41]), .in3(s_41_373), .in2(c_40_363), .in1(s_41_374) );

  // ***** Bit 42 ***** //
  Hadder_ A_42_376( .carry(c_42_376), .sum(s_42_376), .in2(sign_), .in1(P_15[12]) );
  Fadder_ A_42_377( .carry(c_42_377), .sum(s_42_377), .in3(P_14[14]), .in2(P_13[16]), .in1(P_12[18]) );
  Fadder_ A_42_378( .carry(c_42_378), .sum(s_42_378), .in3(P_11[20]), .in2(P_10[22]), .in1(P_9[24]) );
  Fadder_ A_42_379( .carry(c_42_379), .sum(s_42_379), .in3(P_8[26]), .in2(P_7[28]), .in1(P_6[30]) );
  Fadder_ A_42_380( .carry(c_42_380), .sum(s_42_380), .in3(P_5[32]), .in2(c_41_368), .in1(c_41_367) );
  Fadder_ A_42_381( .carry(c_42_381), .sum(s_42_381), .in3(c_41_366), .in2(c_41_365), .in1(s_42_376) );
  Fadder_ A_42_382( .carry(c_42_382), .sum(s_42_382), .in3(s_42_379), .in2(s_42_378), .in1(s_42_377) );
  Fadder_ A_42_383( .carry(c_42_383), .sum(s_42_383), .in3(c_41_369), .in2(s_42_380), .in1(c_41_370) );
  Fadder_ A_42_384( .carry(c_42_384), .sum(s_42_384), .in3(s_42_381), .in2(s_42_382), .in1(c_41_371) );
  Fadder_ A_42_385( .carry(c_42_385), .sum(s_42_385), .in3(c_41_372), .in2(s_42_383), .in1(s_42_384) );
  Fadder_ A_42_386( .carry(c[42]), .sum(s[42]), .in3(c_41_373), .in2(c_41_374), .in1(s_42_385) );

  // ***** Bit 43 ***** //
  Fadder_ A_43_387( .carry(c_43_387), .sum(s_43_387), .in3(P_15[13]), .in2(P_14[15]), .in1(P_13[17]) );
  Fadder_ A_43_388( .carry(c_43_388), .sum(s_43_388), .in3(P_12[19]), .in2(P_11[21]), .in1(P_10[23]) );
  Fadder_ A_43_389( .carry(c_43_389), .sum(s_43_389), .in3(P_9[25]), .in2(P_8[27]), .in1(P_7[29]) );
  Fadder_ A_43_390( .carry(c_43_390), .sum(s_43_390), .in3(P_6[31]), .in2(P_5[33]), .in1(c_42_379) );
  Fadder_ A_43_391( .carry(c_43_391), .sum(s_43_391), .in3(c_42_378), .in2(c_42_377), .in1(c_42_376) );
  Fadder_ A_43_392( .carry(c_43_392), .sum(s_43_392), .in3(s_43_389), .in2(s_43_388), .in1(s_43_387) );
  Fadder_ A_43_393( .carry(c_43_393), .sum(s_43_393), .in3(c_42_380), .in2(c_42_381), .in1(s_43_390) );
  Fadder_ A_43_394( .carry(c_43_394), .sum(s_43_394), .in3(s_43_391), .in2(c_42_382), .in1(s_43_392) );
  Fadder_ A_43_395( .carry(c_43_395), .sum(s_43_395), .in3(c_42_383), .in2(s_43_393), .in1(c_42_384) );
  Fadder_ A_43_396( .carry(c[43]), .sum(s[43]), .in3(s_43_394), .in2(s_43_395), .in1(c_42_385) );

  // ***** Bit 44 ***** //
  Hadder_ A_44_397( .carry(c_44_397), .sum(s_44_397), .in2(sign_), .in1(P_15[14]) );
  Fadder_ A_44_398( .carry(c_44_398), .sum(s_44_398), .in3(P_14[16]), .in2(P_13[18]), .in1(P_12[20]) );
  Fadder_ A_44_399( .carry(c_44_399), .sum(s_44_399), .in3(P_11[22]), .in2(P_10[24]), .in1(P_9[26]) );
  Fadder_ A_44_400( .carry(c_44_400), .sum(s_44_400), .in3(P_8[28]), .in2(P_7[30]), .in1(P_6[32]) );
  Fadder_ A_44_401( .carry(c_44_401), .sum(s_44_401), .in3(c_43_389), .in2(c_43_388), .in1(c_43_387) );
  Fadder_ A_44_402( .carry(c_44_402), .sum(s_44_402), .in3(s_44_397), .in2(s_44_400), .in1(s_44_399) );
  Fadder_ A_44_403( .carry(c_44_403), .sum(s_44_403), .in3(s_44_398), .in2(c_43_390), .in1(c_43_391) );
  Fadder_ A_44_404( .carry(c_44_404), .sum(s_44_404), .in3(s_44_401), .in2(c_43_392), .in1(s_44_402) );
  Fadder_ A_44_405( .carry(c_44_405), .sum(s_44_405), .in3(c_43_393), .in2(s_44_403), .in1(c_43_394) );
  Fadder_ A_44_406( .carry(c[44]), .sum(s[44]), .in3(s_44_404), .in2(c_43_395), .in1(s_44_405) );

  // ***** Bit 45 ***** //
  Fadder_ A_45_407( .carry(c_45_407), .sum(s_45_407), .in3(P_15[15]), .in2(P_14[17]), .in1(P_13[19]) );
  Fadder_ A_45_408( .carry(c_45_408), .sum(s_45_408), .in3(P_12[21]), .in2(P_11[23]), .in1(P_10[25]) );
  Fadder_ A_45_409( .carry(c_45_409), .sum(s_45_409), .in3(P_9[27]), .in2(P_8[29]), .in1(P_7[31]) );
  Fadder_ A_45_410( .carry(c_45_410), .sum(s_45_410), .in3(P_6[33]), .in2(c_44_400), .in1(c_44_399) );
  Fadder_ A_45_411( .carry(c_45_411), .sum(s_45_411), .in3(c_44_398), .in2(c_44_397), .in1(s_45_409) );
  Fadder_ A_45_412( .carry(c_45_412), .sum(s_45_412), .in3(s_45_408), .in2(s_45_407), .in1(c_44_401) );
  Fadder_ A_45_413( .carry(c_45_413), .sum(s_45_413), .in3(s_45_410), .in2(c_44_402), .in1(s_45_411) );
  Fadder_ A_45_414( .carry(c_45_414), .sum(s_45_414), .in3(c_44_403), .in2(s_45_412), .in1(c_44_404) );
  Fadder_ A_45_415( .carry(c[45]), .sum(s[45]), .in3(s_45_413), .in2(c_44_405), .in1(s_45_414) );

  // ***** Bit 46 ***** //
  Hadder_ A_46_416( .carry(c_46_416), .sum(s_46_416), .in2(sign_), .in1(P_15[16]) );
  Fadder_ A_46_417( .carry(c_46_417), .sum(s_46_417), .in3(P_14[18]), .in2(P_13[20]), .in1(P_12[22]) );
  Fadder_ A_46_418( .carry(c_46_418), .sum(s_46_418), .in3(P_11[24]), .in2(P_10[26]), .in1(P_9[28]) );
  Fadder_ A_46_419( .carry(c_46_419), .sum(s_46_419), .in3(P_8[30]), .in2(P_7[32]), .in1(c_45_409) );
  Fadder_ A_46_420( .carry(c_46_420), .sum(s_46_420), .in3(c_45_408), .in2(c_45_407), .in1(s_46_416) );
  Fadder_ A_46_421( .carry(c_46_421), .sum(s_46_421), .in3(s_46_418), .in2(s_46_417), .in1(c_45_410) );
  Fadder_ A_46_422( .carry(c_46_422), .sum(s_46_422), .in3(c_45_411), .in2(s_46_419), .in1(s_46_420) );
  Fadder_ A_46_423( .carry(c_46_423), .sum(s_46_423), .in3(c_45_412), .in2(s_46_421), .in1(c_45_413) );
  Fadder_ A_46_424( .carry(c[46]), .sum(s[46]), .in3(s_46_422), .in2(c_45_414), .in1(s_46_423) );

  // ***** Bit 47 ***** //
  Fadder_ A_47_425( .carry(c_47_425), .sum(s_47_425), .in3(P_15[17]), .in2(P_14[19]), .in1(P_13[21]) );
  Fadder_ A_47_426( .carry(c_47_426), .sum(s_47_426), .in3(P_12[23]), .in2(P_11[25]), .in1(P_10[27]) );
  Fadder_ A_47_427( .carry(c_47_427), .sum(s_47_427), .in3(P_9[29]), .in2(P_8[31]), .in1(P_7[33]) );
  Fadder_ A_47_428( .carry(c_47_428), .sum(s_47_428), .in3(c_46_418), .in2(c_46_417), .in1(c_46_416) );
  Fadder_ A_47_429( .carry(c_47_429), .sum(s_47_429), .in3(s_47_427), .in2(s_47_426), .in1(s_47_425) );
  Fadder_ A_47_430( .carry(c_47_430), .sum(s_47_430), .in3(c_46_419), .in2(c_46_420), .in1(s_47_428) );
  Fadder_ A_47_431( .carry(c_47_431), .sum(s_47_431), .in3(s_47_429), .in2(c_46_421), .in1(c_46_422) );
  Fadder_ A_47_432( .carry(c[47]), .sum(s[47]), .in3(s_47_430), .in2(c_46_423), .in1(s_47_431) );

  // ***** Bit 48 ***** //
  Hadder_ A_48_433( .carry(c_48_433), .sum(s_48_433), .in2(sign_), .in1(P_15[18]) );
  Fadder_ A_48_434( .carry(c_48_434), .sum(s_48_434), .in3(P_14[20]), .in2(P_13[22]), .in1(P_12[24]) );
  Fadder_ A_48_435( .carry(c_48_435), .sum(s_48_435), .in3(P_11[26]), .in2(P_10[28]), .in1(P_9[30]) );
  Fadder_ A_48_436( .carry(c_48_436), .sum(s_48_436), .in3(P_8[32]), .in2(c_47_427), .in1(c_47_426) );
  Fadder_ A_48_437( .carry(c_48_437), .sum(s_48_437), .in3(c_47_425), .in2(s_48_433), .in1(s_48_435) );
  Fadder_ A_48_438( .carry(c_48_438), .sum(s_48_438), .in3(s_48_434), .in2(c_47_428), .in1(s_48_436) );
  Fadder_ A_48_439( .carry(c_48_439), .sum(s_48_439), .in3(c_47_429), .in2(s_48_437), .in1(c_47_430) );
  Fadder_ A_48_440( .carry(c[48]), .sum(s[48]), .in3(s_48_438), .in2(c_47_431), .in1(s_48_439) );

  // ***** Bit 49 ***** //
  Fadder_ A_49_441( .carry(c_49_441), .sum(s_49_441), .in3(P_15[19]), .in2(P_14[21]), .in1(P_13[23]) );
  Fadder_ A_49_442( .carry(c_49_442), .sum(s_49_442), .in3(P_12[25]), .in2(P_11[27]), .in1(P_10[29]) );
  Fadder_ A_49_443( .carry(c_49_443), .sum(s_49_443), .in3(P_9[31]), .in2(P_8[33]), .in1(c_48_435) );
  Fadder_ A_49_444( .carry(c_49_444), .sum(s_49_444), .in3(c_48_434), .in2(c_48_433), .in1(s_49_442) );
  Fadder_ A_49_445( .carry(c_49_445), .sum(s_49_445), .in3(s_49_441), .in2(c_48_436), .in1(s_49_443) );
  Fadder_ A_49_446( .carry(c_49_446), .sum(s_49_446), .in3(c_48_437), .in2(s_49_444), .in1(c_48_438) );
  Fadder_ A_49_447( .carry(c[49]), .sum(s[49]), .in3(s_49_445), .in2(c_48_439), .in1(s_49_446) );

  // ***** Bit 50 ***** //
  Hadder_ A_50_448( .carry(c_50_448), .sum(s_50_448), .in2(sign_), .in1(P_15[20]) );
  Fadder_ A_50_449( .carry(c_50_449), .sum(s_50_449), .in3(P_14[22]), .in2(P_13[24]), .in1(P_12[26]) );
  Fadder_ A_50_450( .carry(c_50_450), .sum(s_50_450), .in3(P_11[28]), .in2(P_10[30]), .in1(P_9[32]) );
  Fadder_ A_50_451( .carry(c_50_451), .sum(s_50_451), .in3(c_49_442), .in2(c_49_441), .in1(s_50_448) );
  Fadder_ A_50_452( .carry(c_50_452), .sum(s_50_452), .in3(s_50_450), .in2(s_50_449), .in1(c_49_443) );
  Fadder_ A_50_453( .carry(c_50_453), .sum(s_50_453), .in3(c_49_444), .in2(s_50_451), .in1(s_50_452) );
  Fadder_ A_50_454( .carry(c[50]), .sum(s[50]), .in3(c_49_445), .in2(c_49_446), .in1(s_50_453) );

  // ***** Bit 51 ***** //
  Fadder_ A_51_455( .carry(c_51_455), .sum(s_51_455), .in3(P_15[21]), .in2(P_14[23]), .in1(P_13[25]) );
  Fadder_ A_51_456( .carry(c_51_456), .sum(s_51_456), .in3(P_12[27]), .in2(P_11[29]), .in1(P_10[31]) );
  Fadder_ A_51_457( .carry(c_51_457), .sum(s_51_457), .in3(P_9[33]), .in2(c_50_450), .in1(c_50_449) );
  Fadder_ A_51_458( .carry(c_51_458), .sum(s_51_458), .in3(c_50_448), .in2(s_51_456), .in1(s_51_455) );
  Fadder_ A_51_459( .carry(c_51_459), .sum(s_51_459), .in3(c_50_451), .in2(s_51_457), .in1(s_51_458) );
  Fadder_ A_51_460( .carry(c[51]), .sum(s[51]), .in3(c_50_452), .in2(s_51_459), .in1(c_50_453) );

  // ***** Bit 52 ***** //
  Hadder_ A_52_461( .carry(c_52_461), .sum(s_52_461), .in2(sign_), .in1(P_15[22]) );
  Fadder_ A_52_462( .carry(c_52_462), .sum(s_52_462), .in3(P_14[24]), .in2(P_13[26]), .in1(P_12[28]) );
  Fadder_ A_52_463( .carry(c_52_463), .sum(s_52_463), .in3(P_11[30]), .in2(P_10[32]), .in1(c_51_456) );
  Fadder_ A_52_464( .carry(c_52_464), .sum(s_52_464), .in3(c_51_455), .in2(s_52_461), .in1(s_52_462) );
  Fadder_ A_52_465( .carry(c_52_465), .sum(s_52_465), .in3(c_51_457), .in2(s_52_463), .in1(c_51_458) );
  Fadder_ A_52_466( .carry(c[52]), .sum(s[52]), .in3(s_52_464), .in2(c_51_459), .in1(s_52_465) );

  // ***** Bit 53 ***** //
  Fadder_ A_53_467( .carry(c_53_467), .sum(s_53_467), .in3(P_15[23]), .in2(P_14[25]), .in1(P_13[27]) );
  Fadder_ A_53_468( .carry(c_53_468), .sum(s_53_468), .in3(P_12[29]), .in2(P_11[31]), .in1(P_10[33]) );
  Fadder_ A_53_469( .carry(c_53_469), .sum(s_53_469), .in3(c_52_462), .in2(c_52_461), .in1(s_53_468) );
  Fadder_ A_53_470( .carry(c_53_470), .sum(s_53_470), .in3(s_53_467), .in2(c_52_463), .in1(c_52_464) );
  Fadder_ A_53_471( .carry(c[53]), .sum(s[53]), .in3(s_53_469), .in2(c_52_465), .in1(s_53_470) );

  // ***** Bit 54 ***** //
  Hadder_ A_54_472( .carry(c_54_472), .sum(s_54_472), .in2(sign_), .in1(P_15[24]) );
  Fadder_ A_54_473( .carry(c_54_473), .sum(s_54_473), .in3(P_14[26]), .in2(P_13[28]), .in1(P_12[30]) );
  Fadder_ A_54_474( .carry(c_54_474), .sum(s_54_474), .in3(P_11[32]), .in2(c_53_468), .in1(c_53_467) );
  Fadder_ A_54_475( .carry(c_54_475), .sum(s_54_475), .in3(s_54_472), .in2(s_54_473), .in1(c_53_469) );
  Fadder_ A_54_476( .carry(c[54]), .sum(s[54]), .in3(s_54_474), .in2(c_53_470), .in1(s_54_475) );

  // ***** Bit 55 ***** //
  Fadder_ A_55_477( .carry(c_55_477), .sum(s_55_477), .in3(P_15[25]), .in2(P_14[27]), .in1(P_13[29]) );
  Fadder_ A_55_478( .carry(c_55_478), .sum(s_55_478), .in3(P_12[31]), .in2(P_11[33]), .in1(c_54_473) );
  Fadder_ A_55_479( .carry(c_55_479), .sum(s_55_479), .in3(c_54_472), .in2(s_55_477), .in1(c_54_474) );
  Fadder_ A_55_480( .carry(c[55]), .sum(s[55]), .in3(s_55_478), .in2(c_54_475), .in1(s_55_479) );

  // ***** Bit 56 ***** //
  Hadder_ A_56_481( .carry(c_56_481), .sum(s_56_481), .in2(sign_), .in1(P_15[26]) );
  Fadder_ A_56_482( .carry(c_56_482), .sum(s_56_482), .in3(P_14[28]), .in2(P_13[30]), .in1(P_12[32]) );
  Fadder_ A_56_483( .carry(c_56_483), .sum(s_56_483), .in3(c_55_477), .in2(s_56_481), .in1(s_56_482) );
  Fadder_ A_56_484( .carry(c[56]), .sum(s[56]), .in3(c_55_478), .in2(s_56_483), .in1(c_55_479) );

  // ***** Bit 57 ***** //
  Fadder_ A_57_485( .carry(c_57_485), .sum(s_57_485), .in3(P_15[27]), .in2(P_14[29]), .in1(P_13[31]) );
  Fadder_ A_57_486( .carry(c_57_486), .sum(s_57_486), .in3(P_12[33]), .in2(c_56_482), .in1(c_56_481) );
  Fadder_ A_57_487( .carry(c[57]), .sum(s[57]), .in3(s_57_485), .in2(s_57_486), .in1(c_56_483) );

  // ***** Bit 58 ***** //
  Hadder_ A_58_488( .carry(c_58_488), .sum(s_58_488), .in2(sign_), .in1(P_15[28]) );
  Fadder_ A_58_489( .carry(c_58_489), .sum(s_58_489), .in3(P_14[30]), .in2(P_13[32]), .in1(c_57_485) );
  Fadder_ A_58_490( .carry(c[58]), .sum(s[58]), .in3(s_58_488), .in2(c_57_486), .in1(s_58_489) );

  // ***** Bit 59 ***** //
  Fadder_ A_59_491( .carry(c_59_491), .sum(s_59_491), .in3(P_15[29]), .in2(P_14[31]), .in1(P_13[33]) );
  Fadder_ A_59_492( .carry(c[59]), .sum(s[59]), .in3(c_58_488), .in2(s_59_491), .in1(c_58_489) );

  // ***** Bit 60 ***** //
  Hadder_ A_60_493( .carry(c_60_493), .sum(s_60_493), .in2(sign_), .in1(P_15[30]) );
  Fadder_ A_60_494( .carry(c[60]), .sum(s[60]), .in3(P_14[32]), .in2(c_59_491), .in1(s_60_493) );

  // ***** Bit 61 ***** //
  Fadder_ A_61_495( .carry(c[61]), .sum(s[61]), .in3(P_15[31]), .in2(P_14[33]), .in1(c_60_493) );

  // ***** Bit 62 ***** //
  Hadder_ A_62_496( .carry(c[62]), .sum(s[62]), .in2(sign_), .in1(P_15[32]) );

  // ***** Bit 63 ***** //
  assign s[63] = P_15[33] ;

endmodule

module add_csa70(
  if_add,
  in_sum,
  in_carry,
  product,
  out_sum,
  out_carry);

  input if_add;
  input [69:0] in_sum;
  input [68:0] in_carry;
  input [69:0] product;
  output [69:0] out_sum, out_carry;

  wire inc;
  wire [69:0] comp_sum;
  wire [68:0] comp_carry;

  assign inc = (if_add==1'b1)? 1'b0 : 1'b1;
  assign comp_sum = (if_add==1'b1)? in_sum : (~in_sum);
  assign comp_carry = (if_add==1'b1)? in_carry : (~in_carry);

  Fadder_2 u0( .in1(comp_sum[0]), .in2(product[0]), .in3(inc), .sum(out_sum[0]), .carry(out_carry[0]) );
  Fadder_2 u1( .in1(comp_sum[1]), .in2(comp_carry[0]), .in3(product[1]), .sum(out_sum[1]), .carry(out_carry[1]) );
  Fadder_2 u2( .in1(comp_sum[2]), .in2(comp_carry[1]), .in3(product[2]), .sum(out_sum[2]), .carry(out_carry[2]) );
  Fadder_2 u3( .in1(comp_sum[3]), .in2(comp_carry[2]), .in3(product[3]), .sum(out_sum[3]), .carry(out_carry[3]) );
  Fadder_2 u4( .in1(comp_sum[4]), .in2(comp_carry[3]), .in3(product[4]), .sum(out_sum[4]), .carry(out_carry[4]) );
  Fadder_2 u5( .in1(comp_sum[5]), .in2(comp_carry[4]), .in3(product[5]), .sum(out_sum[5]), .carry(out_carry[5]) );
  Fadder_2 u6( .in1(comp_sum[6]), .in2(comp_carry[5]), .in3(product[6]), .sum(out_sum[6]), .carry(out_carry[6]) );
  Fadder_2 u7( .in1(comp_sum[7]), .in2(comp_carry[6]), .in3(product[7]), .sum(out_sum[7]), .carry(out_carry[7]) );
  Fadder_2 u8( .in1(comp_sum[8]), .in2(comp_carry[7]), .in3(product[8]), .sum(out_sum[8]), .carry(out_carry[8]) );
  Fadder_2 u9( .in1(comp_sum[9]), .in2(comp_carry[8]), .in3(product[9]), .sum(out_sum[9]), .carry(out_carry[9]) );
  Fadder_2 u10( .in1(comp_sum[10]), .in2(comp_carry[9]), .in3(product[10]), .sum(out_sum[10]), .carry(out_carry[10]) );
  Fadder_2 u11( .in1(comp_sum[11]), .in2(comp_carry[10]), .in3(product[11]), .sum(out_sum[11]), .carry(out_carry[11]) );
  Fadder_2 u12( .in1(comp_sum[12]), .in2(comp_carry[11]), .in3(product[12]), .sum(out_sum[12]), .carry(out_carry[12]) );
  Fadder_2 u13( .in1(comp_sum[13]), .in2(comp_carry[12]), .in3(product[13]), .sum(out_sum[13]), .carry(out_carry[13]) );
  Fadder_2 u14( .in1(comp_sum[14]), .in2(comp_carry[13]), .in3(product[14]), .sum(out_sum[14]), .carry(out_carry[14]) );
  Fadder_2 u15( .in1(comp_sum[15]), .in2(comp_carry[14]), .in3(product[15]), .sum(out_sum[15]), .carry(out_carry[15]) );
  Fadder_2 u16( .in1(comp_sum[16]), .in2(comp_carry[15]), .in3(product[16]), .sum(out_sum[16]), .carry(out_carry[16]) );
  Fadder_2 u17( .in1(comp_sum[17]), .in2(comp_carry[16]), .in3(product[17]), .sum(out_sum[17]), .carry(out_carry[17]) );
  Fadder_2 u18( .in1(comp_sum[18]), .in2(comp_carry[17]), .in3(product[18]), .sum(out_sum[18]), .carry(out_carry[18]) );
  Fadder_2 u19( .in1(comp_sum[19]), .in2(comp_carry[18]), .in3(product[19]), .sum(out_sum[19]), .carry(out_carry[19]) );
  Fadder_2 u20( .in1(comp_sum[20]), .in2(comp_carry[19]), .in3(product[20]), .sum(out_sum[20]), .carry(out_carry[20]) );
  Fadder_2 u21( .in1(comp_sum[21]), .in2(comp_carry[20]), .in3(product[21]), .sum(out_sum[21]), .carry(out_carry[21]) );
  Fadder_2 u22( .in1(comp_sum[22]), .in2(comp_carry[21]), .in3(product[22]), .sum(out_sum[22]), .carry(out_carry[22]) );
  Fadder_2 u23( .in1(comp_sum[23]), .in2(comp_carry[22]), .in3(product[23]), .sum(out_sum[23]), .carry(out_carry[23]) );
  Fadder_2 u24( .in1(comp_sum[24]), .in2(comp_carry[23]), .in3(product[24]), .sum(out_sum[24]), .carry(out_carry[24]) );
  Fadder_2 u25( .in1(comp_sum[25]), .in2(comp_carry[24]), .in3(product[25]), .sum(out_sum[25]), .carry(out_carry[25]) );
  Fadder_2 u26( .in1(comp_sum[26]), .in2(comp_carry[25]), .in3(product[26]), .sum(out_sum[26]), .carry(out_carry[26]) );
  Fadder_2 u27( .in1(comp_sum[27]), .in2(comp_carry[26]), .in3(product[27]), .sum(out_sum[27]), .carry(out_carry[27]) );
  Fadder_2 u28( .in1(comp_sum[28]), .in2(comp_carry[27]), .in3(product[28]), .sum(out_sum[28]), .carry(out_carry[28]) );
  Fadder_2 u29( .in1(comp_sum[29]), .in2(comp_carry[28]), .in3(product[29]), .sum(out_sum[29]), .carry(out_carry[29]) );
  Fadder_2 u30( .in1(comp_sum[30]), .in2(comp_carry[29]), .in3(product[30]), .sum(out_sum[30]), .carry(out_carry[30]) );
  Fadder_2 u31( .in1(comp_sum[31]), .in2(comp_carry[30]), .in3(product[31]), .sum(out_sum[31]), .carry(out_carry[31]) );
  Fadder_2 u32( .in1(comp_sum[32]), .in2(comp_carry[31]), .in3(product[32]), .sum(out_sum[32]), .carry(out_carry[32]) );
  Fadder_2 u33( .in1(comp_sum[33]), .in2(comp_carry[32]), .in3(product[33]), .sum(out_sum[33]), .carry(out_carry[33]) );
  Fadder_2 u34( .in1(comp_sum[34]), .in2(comp_carry[33]), .in3(product[34]), .sum(out_sum[34]), .carry(out_carry[34]) );
  Fadder_2 u35( .in1(comp_sum[35]), .in2(comp_carry[34]), .in3(product[35]), .sum(out_sum[35]), .carry(out_carry[35]) );
  Fadder_2 u36( .in1(comp_sum[36]), .in2(comp_carry[35]), .in3(product[36]), .sum(out_sum[36]), .carry(out_carry[36]) );
  Fadder_2 u37( .in1(comp_sum[37]), .in2(comp_carry[36]), .in3(product[37]), .sum(out_sum[37]), .carry(out_carry[37]) );
  Fadder_2 u38( .in1(comp_sum[38]), .in2(comp_carry[37]), .in3(product[38]), .sum(out_sum[38]), .carry(out_carry[38]) );
  Fadder_2 u39( .in1(comp_sum[39]), .in2(comp_carry[38]), .in3(product[39]), .sum(out_sum[39]), .carry(out_carry[39]) );
  Fadder_2 u40( .in1(comp_sum[40]), .in2(comp_carry[39]), .in3(product[40]), .sum(out_sum[40]), .carry(out_carry[40]) );
  Fadder_2 u41( .in1(comp_sum[41]), .in2(comp_carry[40]), .in3(product[41]), .sum(out_sum[41]), .carry(out_carry[41]) );
  Fadder_2 u42( .in1(comp_sum[42]), .in2(comp_carry[41]), .in3(product[42]), .sum(out_sum[42]), .carry(out_carry[42]) );
  Fadder_2 u43( .in1(comp_sum[43]), .in2(comp_carry[42]), .in3(product[43]), .sum(out_sum[43]), .carry(out_carry[43]) );
  Fadder_2 u44( .in1(comp_sum[44]), .in2(comp_carry[43]), .in3(product[44]), .sum(out_sum[44]), .carry(out_carry[44]) );
  Fadder_2 u45( .in1(comp_sum[45]), .in2(comp_carry[44]), .in3(product[45]), .sum(out_sum[45]), .carry(out_carry[45]) );
  Fadder_2 u46( .in1(comp_sum[46]), .in2(comp_carry[45]), .in3(product[46]), .sum(out_sum[46]), .carry(out_carry[46]) );
  Fadder_2 u47( .in1(comp_sum[47]), .in2(comp_carry[46]), .in3(product[47]), .sum(out_sum[47]), .carry(out_carry[47]) );
  Fadder_2 u48( .in1(comp_sum[48]), .in2(comp_carry[47]), .in3(product[48]), .sum(out_sum[48]), .carry(out_carry[48]) );
  Fadder_2 u49( .in1(comp_sum[49]), .in2(comp_carry[48]), .in3(product[49]), .sum(out_sum[49]), .carry(out_carry[49]) );
  Fadder_2 u50( .in1(comp_sum[50]), .in2(comp_carry[49]), .in3(product[50]), .sum(out_sum[50]), .carry(out_carry[50]) );
  Fadder_2 u51( .in1(comp_sum[51]), .in2(comp_carry[50]), .in3(product[51]), .sum(out_sum[51]), .carry(out_carry[51]) );
  Fadder_2 u52( .in1(comp_sum[52]), .in2(comp_carry[51]), .in3(product[52]), .sum(out_sum[52]), .carry(out_carry[52]) );
  Fadder_2 u53( .in1(comp_sum[53]), .in2(comp_carry[52]), .in3(product[53]), .sum(out_sum[53]), .carry(out_carry[53]) );
  Fadder_2 u54( .in1(comp_sum[54]), .in2(comp_carry[53]), .in3(product[54]), .sum(out_sum[54]), .carry(out_carry[54]) );
  Fadder_2 u55( .in1(comp_sum[55]), .in2(comp_carry[54]), .in3(product[55]), .sum(out_sum[55]), .carry(out_carry[55]) );
  Fadder_2 u56( .in1(comp_sum[56]), .in2(comp_carry[55]), .in3(product[56]), .sum(out_sum[56]), .carry(out_carry[56]) );
  Fadder_2 u57( .in1(comp_sum[57]), .in2(comp_carry[56]), .in3(product[57]), .sum(out_sum[57]), .carry(out_carry[57]) );
  Fadder_2 u58( .in1(comp_sum[58]), .in2(comp_carry[57]), .in3(product[58]), .sum(out_sum[58]), .carry(out_carry[58]) );
  Fadder_2 u59( .in1(comp_sum[59]), .in2(comp_carry[58]), .in3(product[59]), .sum(out_sum[59]), .carry(out_carry[59]) );
  Fadder_2 u60( .in1(comp_sum[60]), .in2(comp_carry[59]), .in3(product[60]), .sum(out_sum[60]), .carry(out_carry[60]) );
  Fadder_2 u61( .in1(comp_sum[61]), .in2(comp_carry[60]), .in3(product[61]), .sum(out_sum[61]), .carry(out_carry[61]) );
  Fadder_2 u62( .in1(comp_sum[62]), .in2(comp_carry[61]), .in3(product[62]), .sum(out_sum[62]), .carry(out_carry[62]) );
  Fadder_2 u63( .in1(comp_sum[63]), .in2(comp_carry[62]), .in3(product[63]), .sum(out_sum[63]), .carry(out_carry[63]) );
  Fadder_2 u64( .in1(comp_sum[64]), .in2(comp_carry[63]), .in3(product[64]), .sum(out_sum[64]), .carry(out_carry[64]) );
  Fadder_2 u65( .in1(comp_sum[65]), .in2(comp_carry[64]), .in3(product[65]), .sum(out_sum[65]), .carry(out_carry[65]) );
  Fadder_2 u66( .in1(comp_sum[66]), .in2(comp_carry[65]), .in3(product[66]), .sum(out_sum[66]), .carry(out_carry[66]) );
  Fadder_2 u67( .in1(comp_sum[67]), .in2(comp_carry[66]), .in3(product[67]), .sum(out_sum[67]), .carry(out_carry[67]) );
  Fadder_2 u68( .in1(comp_sum[68]), .in2(comp_carry[67]), .in3(product[68]), .sum(out_sum[68]), .carry(out_carry[68]) );
  Fadder_2 u69( .in1(comp_sum[69]), .in2(comp_carry[68]), .in3(product[69]), .sum(out_sum[69]), .carry(out_carry[69]) );

endmodule

module DW01_add2 (A,B,CI,SUM,CO);

  parameter width=8'b00100100;

  output [width-1 : 0] SUM;
  output CO;
  input [width-1 : 0] A;
  input [width-1 : 0] B;
  input CI;

  wire [width-1 : 0] SUM;
  reg [width-2 : 0] sum_temp;
  reg CO;

  assign SUM = {CO, sum_temp};

always @(A or B or CI)
  begin
    {CO, sum_temp} = A+B+CI;
  end
endmodule

module DW01_add1 (A,B,CI,SUM,CO);

  parameter width=8'b00100011;

  output [width-1 : 0] SUM;
  output CO;
  input [width-1 : 0] A;
  input [width-1 : 0] B;
  input CI;

  wire [width-1 : 0] SUM;
  reg [width-2 : 0] sum_temp;
  reg CO;
  reg c_out;

	assign SUM = {CO, sum_temp};

always @(A or B or CI)
  begin
    {CO, sum_temp} = A+B+CI;
//	sum_out = A + B;
//    plus(sum_out,c_out,A,B,CI);
//    SUM = sum_out;
//    CO =  c_out;
  end

/*
  task plus;
    output  [width-1 : 0] sumout;
    output CO;
    input [width-1 : 0] A,B;
    input CI;
    reg [width-1 : 0] sumout,SUM;
    reg CO;
    reg carry;
    reg [width-1 : 0] A,B;
    integer i;

    begin
      carry = CI;
      for (i = 0; i <= width-1; i = i + 1) begin
        sumout[i] = A[i] ^ B[i] ^ carry;
        carry = (A[i] & B[i]) | (A[i] & carry) | (carry & B[i]);
      end
      SUM = sumout;
      CO = carry;
    end
  endtask
*/
endmodule

module Adder71(
  a,
  b,
  cin,
  sum,
  cout);

  input [70:0]a, b;
  input cin;
  output [70:0]sum;
  output cout;

  wire [34:0] temp1, result1;
  wire [35:0] temp2_1, temp2_2, result2;
  wire co1, co2_1, co2_2;

  assign result1= temp1;
  assign result2= (co1==1'b0) ? temp2_1 : temp2_2;
  assign sum={result2,result1};
  assign cout=(co1==1'b0) ? co2_1 : co2_2;

  DW01_add1 u1_1(.A(a[34:0]), .B(b[34:0]), .CI(cin), .SUM(temp1), .CO(co1));
  DW01_add2 u2_1(.A(a[70:35]), .B(b[70:35]), .CI(1'b0), .SUM(temp2_1), .CO(co2_1));
  DW01_add2 u2_2(.A(a[70:35]), .B(b[70:35]), .CI(1'b1), .SUM(temp2_2), .CO(co2_2));

endmodule

module cla_csa70(
  if_add,
  sum,
  carry,
  result,
  overflow);

  input [69:0] sum;
  input [69:0] carry;
  input if_add;
  output [69:0] result;
  output overflow;
  wire temp_overflow;

  reg [69:0] result;
  reg overflow;
  wire [70:0] temp;
  wire inc1, inc2;

  assign inc1=(if_add==1'b1)? 1'b0 : 1'b1;
  assign inc2=(if_add==1'b1)? 1'b0 : 1'b1;

  Adder71 U(.a({sum[69],sum}), .b({carry,inc1}), .cin(inc2), .sum(temp), .cout(temp_overflow));
  //Adder71 U(.a({sum[69],sum}), .b({carry,inc1}), .cin(inc2), .sum(temp), .);

  always @( temp )
    begin
      result = temp[69:0];
	  overflow = temp_overflow;

/*
      if (temp[70]^temp[69]==1'b1)
        overflow = 1'b1;
      else
        overflow = 1'b0;
*/
    end

endmodule


module mac1(
  clk,
  reset,
  stall,
  if_add,
  mode,
  mulcand,
  mulier,
  value,
  cnt_overflow,
  answer,
  overflow);

  input clk;
  input reset;
  input stall;
  input if_add;
  input mode;
  input [69:0] value;
  input [31:0] mulcand;
  input [31:0] mulier;
  input cnt_overflow;
  output [69:0] answer;
  output overflow;

  reg [31:0] in1;
  reg [31:0] in2;
  wire [63:0] wire_s;
  wire [62:0] wire_c;
  reg [63:0] temp_s;
  reg [62:0] temp_c;
  wire [69:0] operand_answer;
  wire [69:0] operand_s;
  wire [68:0] operand_c;
  wire [69:0] final_sum;
  wire [69:0] final_carry;
  wire [69:0] wire_answer;
  reg [69:0] answer;
  wire [69:0] final_answer;
  wire wire_overflow;
  reg overflow;

  assign operand_answer = mode==1'b0 ? answer : 0; 
  assign final_answer=((wire_overflow&cnt_overflow)==1'b1) ? {70'b1111111111111111111111111111111111111111111111111111111111111111111111} : wire_answer;
  assign operand_s ={ temp_s[63],temp_s[63],temp_s[63],temp_s[63],temp_s[63],temp_s[63],temp_s };
  assign operand_c ={ temp_c[62],temp_c[62],temp_c[62],temp_c[62],temp_c[62],temp_c[62],temp_c };

  mult_ u1(.a(in1), .b(in2), .s(wire_s), .c(wire_c));

  add_csa70 u2( .if_add(if_add), .in_sum(operand_s), .in_carry(operand_c), .product(operand_answer), .out_sum(final_sum), .out_carry(final_carry) );

  cla_csa70 u3( .if_add(if_add), .sum(final_sum), .carry(final_carry), .result(wire_answer), .overflow(wire_overflow) );

  always @(posedge clk)
    begin
    if( reset==1'b1 )
      begin
      in1<=0;
      in2<=0;
      temp_s<=0;
      temp_c<=0;
      answer<=value;
      overflow<=0;
      end
    else
      begin
      if (stall==1'b0)
        begin
        in1<=mulcand;
        in2<=mulier;
        temp_s<=wire_s;
        temp_c<=wire_c;
        overflow<=wire_overflow;
        answer<=final_answer;
        end
      end
    end

endmodule
