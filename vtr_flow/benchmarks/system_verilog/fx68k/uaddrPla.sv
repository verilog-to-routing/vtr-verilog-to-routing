//
// FX68K
//
// Opcode to uaddr entry routines (A2-A2-A3) PLA
//

`timescale 1 ns / 1 ns

`define NMA_BITS   10

`define SFTM1 'h3C7
`define SRRW1 'h382
`define SRIW1 'h381
`define SRRL1 'h386
`define SRIL1 'h385
`define BSRI1 'h089
`define BSRW1 'h0A9
`define BBCI1 'h308
`define BBCW1 'h068
`define RLQL1 'h23B
`define ADRW1 'h006
`define PINW1 'h21C
`define PDCW1 'h103
`define ADSW1 'h1C2
`define AIXW0 'h1E3
`define ABWW1 'h00A
`define ABLW1 'h1E2
`define TRAP1 'h1D0
`define LINK1 'h30B
`define UNLK1 'h119
`define LUSP1 'h2F5
`define SUSP1 'h230
`define TRPV1 'h06D
`define RSET1 'h3A6
`define B 'h363
`define STOP1 'h3A2
`define RTR1 'h12A
`define RTS1 'h126


module pla_lined(
   input [3:0] movEa,
   input [3:0] col,

   input [15:0] opcode,
   input [15:0] lineBmap,
   
   output palIll,
   output logic [`NMA_BITS-1:0] plaA1,
   output logic [`NMA_BITS-1:0] plaA2,
   output logic [`NMA_BITS-1:0] plaA3
   );

   wire [3:0] line = opcode[ 15:12];
   wire [2:0] row86 = opcode[8:6];
   
   logic [15:0] arIll;
   logic [`NMA_BITS-1:0] arA1[ 15:0];
   logic [`NMA_BITS-1:0] arA23[ 15:0];
   logic [`NMA_BITS-1:0] scA3;

   logic illMisc;
   logic [`NMA_BITS-1:0] a1Misc;
   
   /*
   reg [`NMA_BITS-1:0] lineMask[ 15:0];  
   always_comb begin
       integer i;
       for( i = 0; i < 16; i = i + 1)
         lineMask[i] = { 16{lineBmap[ i]}};
   end
   */

   assign palIll = (| (arIll & lineBmap));
   
   // Only line 0 has 3 subs
   assign plaA1 = arA1[ line];
   assign plaA2 = arA23[ line];
   assign plaA3 = lineBmap[0] ? scA3 : arA23[ line];
   
   
   // Simple lines
   always_comb begin
      // Line 6: Branch
      arIll[ 'h6] = 1'b0;
      arA23[ 'h6] = 'X;
      if( opcode[ 11:8] == 4'h1)
         arA1[ 'h6] = (| opcode[7:0]) ? `BSRI1 : `BSRW1;
      else
         arA1[ 'h6] = (| opcode[7:0]) ? `BBCI1 : `BBCW1;
                         
      // Line 7: moveq
      arIll[ 'h7] = opcode[ 8];
      arA23[ 'h7] = 'X;
      arA1[ 'h7] = `RLQL1;

      // Line A & F      
      arIll[ 'ha] = 1'b1;       arIll[ 'hf] = 1'b1;
      arA1[ 'ha]  = 'X;         arA1[ 'hf]  = 'X;
      arA23[ 'ha] = 'X;         arA23[ 'hf] = 'X;
      
   end   

   // Special lines

   // Line e: shifts
   always_comb begin
      if( ~opcode[11] & opcode[7] & opcode[6])
      begin
         arA23[ 'he] = `SFTM1;
         unique case( col)
            2: begin arIll[ 'he] = 1'b0; arA1[ 'he] = `ADRW1; end
            3: begin arIll[ 'he] = 1'b0; arA1[ 'he] = `PINW1; end
            4: begin arIll[ 'he] = 1'b0; arA1[ 'he] = `PDCW1; end
            5: begin arIll[ 'he] = 1'b0; arA1[ 'he] = `ADSW1; end
            6: begin arIll[ 'he] = 1'b0; arA1[ 'he] = `AIXW0; end
            7: begin arIll[ 'he] = 1'b0; arA1[ 'he] = `ABWW1; end
            8: begin arIll[ 'he] = 1'b0; arA1[ 'he] = `ABLW1; end
            default: begin arIll[ 'he] = 1'b1; arA1[ 'he] = 'X; end
         endcase
      end
      else
      begin
         arA23[ 'he] = 'X;
         unique case( opcode[ 7:6])
         2'b00,
         2'b01: begin
                  arIll[ 'he] = 1'b0;
                  arA1[ 'he]  = opcode[ 5] ? `SRRW1 : `SRIW1;
               end
         2'b10: begin
                  arIll[ 'he] = 1'b0;
                  arA1[ 'he]  = opcode[ 5] ? `SRRL1 : `SRIL1;
              end
         2'b11: begin arIll[ 'he] = 1'b1; arA1[ 'he]  = 'X; end
         endcase
      end
   end

   // Misc. line 4 row
   always_comb begin
      illMisc = 1'b0;
      case( opcode[ 5:3])
      3'b000,
      3'b001:      a1Misc = `TRAP1;
      3'b010:      a1Misc = `LINK1;
      3'b011:      a1Misc = `UNLK1;
      3'b100:      a1Misc = `LUSP1;
      3'b101:      a1Misc = `SUSP1;
      
      3'b110:   
         case( opcode[ 2:0])
         3'b110:   a1Misc = `TRPV1;
         3'b000:   a1Misc = `RSET1;
         3'b001:   a1Misc = `B;
         3'b010:   a1Misc = `STOP1;
         3'b011:   a1Misc = `RTR1;
         3'b111:   a1Misc = `RTR1;
         3'b101:   a1Misc = `RTS1;
         default:  begin  illMisc = 1'b1; a1Misc = 'X; end
         endcase
         
      default:  begin  illMisc = 1'b1; a1Misc = 'X; end
      endcase
   end

//
// Past here
//


//
// Line: 0
//
always_comb begin

if( (opcode[11:6] & 'h1F) == 'h8) begin
    unique case ( col)
     0: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h100; scA3 = 'X; end
     1: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
     2: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h006; scA3 = 'h299; end
     3: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h21C; scA3 = 'h299; end
     4: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h103; scA3 = 'h299; end
     5: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1C2; scA3 = 'h299; end
     6: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1E3; scA3 = 'h299; end
     7: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h00A; scA3 = 'h299; end
     8: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1E2; scA3 = 'h299; end
     9: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    10: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    11: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1CC; scA3 = 'X; end
    default: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    endcase
end

else if( (opcode[11:6] & 'h37) == 'h0) begin
    unique case ( col)
     0: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h100; scA3 = 'X; end
     1: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
     2: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h006; scA3 = 'h299; end
     3: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h21C; scA3 = 'h299; end
     4: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h103; scA3 = 'h299; end
     5: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1C2; scA3 = 'h299; end
     6: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1E3; scA3 = 'h299; end
     7: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h00A; scA3 = 'h299; end
     8: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1E2; scA3 = 'h299; end
     9: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    10: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    11: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1CC; scA3 = 'X; end
    default: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    endcase
end

else if( (opcode[11:6] & 'h1F) == 'h9) begin
    unique case ( col)
     0: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h100; scA3 = 'X; end
     1: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
     2: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h006; scA3 = 'h299; end
     3: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h21C; scA3 = 'h299; end
     4: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h103; scA3 = 'h299; end
     5: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1C2; scA3 = 'h299; end
     6: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1E3; scA3 = 'h299; end
     7: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h00A; scA3 = 'h299; end
     8: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1E2; scA3 = 'h299; end
     9: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    10: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    11: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1CC; scA3 = 'X; end
    default: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    endcase
end

else if( (opcode[11:6] & 'h37) == 'h1) begin
    unique case ( col)
     0: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h100; scA3 = 'X; end
     1: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
     2: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h006; scA3 = 'h299; end
     3: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h21C; scA3 = 'h299; end
     4: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h103; scA3 = 'h299; end
     5: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1C2; scA3 = 'h299; end
     6: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1E3; scA3 = 'h299; end
     7: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h00A; scA3 = 'h299; end
     8: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1E2; scA3 = 'h299; end
     9: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    10: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    11: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1CC; scA3 = 'X; end
    default: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    endcase
end

else if( (opcode[11:6] & 'h1F) == 'hA) begin
    unique case ( col)
     0: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h10C; scA3 = 'X; end
     1: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
     2: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h00B; scA3 = 'h29D; end
     3: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h00F; scA3 = 'h29D; end
     4: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h179; scA3 = 'h29D; end
     5: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h1C6; scA3 = 'h29D; end
     6: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h1E7; scA3 = 'h29D; end
     7: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h00E; scA3 = 'h29D; end
     8: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h1E6; scA3 = 'h29D; end
     9: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    10: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    11: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    default: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    endcase
end

else if( (opcode[11:6] & 'h37) == 'h2) begin
    unique case ( col)
     0: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h10C; scA3 = 'X; end
     1: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
     2: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h00B; scA3 = 'h29D; end
     3: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h00F; scA3 = 'h29D; end
     4: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h179; scA3 = 'h29D; end
     5: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h1C6; scA3 = 'h29D; end
     6: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h1E7; scA3 = 'h29D; end
     7: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h00E; scA3 = 'h29D; end
     8: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h1E6; scA3 = 'h29D; end
     9: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    10: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    11: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    default: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    endcase
end

else if( (opcode[11:6] & 'h37) == 'h10) begin
    unique case ( col)
     0: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h100; scA3 = 'X; end
     1: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
     2: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h006; scA3 = 'h299; end
     3: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h21C; scA3 = 'h299; end
     4: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h103; scA3 = 'h299; end
     5: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1C2; scA3 = 'h299; end
     6: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1E3; scA3 = 'h299; end
     7: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h00A; scA3 = 'h299; end
     8: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1E2; scA3 = 'h299; end
     9: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    10: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    11: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    default: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    endcase
end

else if( (opcode[11:6] & 'h37) == 'h11) begin
    unique case ( col)
     0: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h100; scA3 = 'X; end
     1: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
     2: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h006; scA3 = 'h299; end
     3: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h21C; scA3 = 'h299; end
     4: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h103; scA3 = 'h299; end
     5: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1C2; scA3 = 'h299; end
     6: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1E3; scA3 = 'h299; end
     7: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h00A; scA3 = 'h299; end
     8: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1E2; scA3 = 'h299; end
     9: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    10: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    11: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    default: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    endcase
end

else if( (opcode[11:6] & 'h37) == 'h12) begin
    unique case ( col)
     0: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h10C; scA3 = 'X; end
     1: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
     2: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h00B; scA3 = 'h29D; end
     3: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h00F; scA3 = 'h29D; end
     4: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h179; scA3 = 'h29D; end
     5: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h1C6; scA3 = 'h29D; end
     6: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h1E7; scA3 = 'h29D; end
     7: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h00E; scA3 = 'h29D; end
     8: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h1E6; scA3 = 'h29D; end
     9: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    10: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    11: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    default: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    endcase
end

else if( (opcode[11:6] & 'h7) == 'h4) begin
    unique case ( col)
     0: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E7; arA23[ 'h0] = 'X;  scA3 = 'X; end
     1: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h1D2; arA23[ 'h0] = 'X;  scA3 = 'X; end
     2: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h006; arA23[ 'h0] = 'X;  scA3 = 'h215; end
     3: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h21C; arA23[ 'h0] = 'X;  scA3 = 'h215; end
     4: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h103; arA23[ 'h0] = 'X;  scA3 = 'h215; end
     5: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h1C2; arA23[ 'h0] = 'X;  scA3 = 'h215; end
     6: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h1E3; arA23[ 'h0] = 'X;  scA3 = 'h215; end
     7: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h00A; arA23[ 'h0] = 'X;  scA3 = 'h215; end
     8: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h1E2; arA23[ 'h0] = 'X;  scA3 = 'h215; end
     9: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h1C2; arA23[ 'h0] = 'X;  scA3 = 'h215; end
    10: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h1E3; arA23[ 'h0] = 'X;  scA3 = 'h215; end
    11: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h0EA; arA23[ 'h0] = 'h0AB; scA3 = 'X; end
    default: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    endcase
end

else if( (opcode[11:6] & 'h7) == 'h5) begin
    unique case ( col)
     0: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3EF; arA23[ 'h0] = 'X;  scA3 = 'X; end
     1: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h1D6; arA23[ 'h0] = 'X;  scA3 = 'X; end
     2: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h006; arA23[ 'h0] = 'X;  scA3 = 'h081; end
     3: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h21C; arA23[ 'h0] = 'X;  scA3 = 'h081; end
     4: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h103; arA23[ 'h0] = 'X;  scA3 = 'h081; end
     5: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h1C2; arA23[ 'h0] = 'X;  scA3 = 'h081; end
     6: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h1E3; arA23[ 'h0] = 'X;  scA3 = 'h081; end
     7: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h00A; arA23[ 'h0] = 'X;  scA3 = 'h081; end
     8: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h1E2; arA23[ 'h0] = 'X;  scA3 = 'h081; end
     9: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    10: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    11: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    default: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    endcase
end

else if( (opcode[11:6] & 'h7) == 'h7) begin
    unique case ( col)
     0: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3EF; arA23[ 'h0] = 'X;  scA3 = 'X; end
     1: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h1CE; arA23[ 'h0] = 'X;  scA3 = 'X; end
     2: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h006; arA23[ 'h0] = 'X;  scA3 = 'h081; end
     3: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h21C; arA23[ 'h0] = 'X;  scA3 = 'h081; end
     4: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h103; arA23[ 'h0] = 'X;  scA3 = 'h081; end
     5: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h1C2; arA23[ 'h0] = 'X;  scA3 = 'h081; end
     6: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h1E3; arA23[ 'h0] = 'X;  scA3 = 'h081; end
     7: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h00A; arA23[ 'h0] = 'X;  scA3 = 'h081; end
     8: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h1E2; arA23[ 'h0] = 'X;  scA3 = 'h081; end
     9: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    10: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    11: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    default: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    endcase
end

else if( (opcode[11:6] & 'h7) == 'h6) begin
    unique case ( col)
     0: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3EB; arA23[ 'h0] = 'X;  scA3 = 'X; end
     1: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h1CA; arA23[ 'h0] = 'X;  scA3 = 'X; end
     2: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h006; arA23[ 'h0] = 'X;  scA3 = 'h069; end
     3: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h21C; arA23[ 'h0] = 'X;  scA3 = 'h069; end
     4: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h103; arA23[ 'h0] = 'X;  scA3 = 'h069; end
     5: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h1C2; arA23[ 'h0] = 'X;  scA3 = 'h069; end
     6: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h1E3; arA23[ 'h0] = 'X;  scA3 = 'h069; end
     7: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h00A; arA23[ 'h0] = 'X;  scA3 = 'h069; end
     8: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h1E2; arA23[ 'h0] = 'X;  scA3 = 'h069; end
     9: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    10: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    11: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    default: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    endcase
end

else if( opcode[11:6] == 'h20) begin
    unique case ( col)
     0: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h3E7; scA3 = 'X; end
     1: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
     2: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h006; scA3 = 'h215; end
     3: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h21C; scA3 = 'h215; end
     4: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h103; scA3 = 'h215; end
     5: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1C2; scA3 = 'h215; end
     6: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1E3; scA3 = 'h215; end
     7: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h00A; scA3 = 'h215; end
     8: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1E2; scA3 = 'h215; end
     9: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1C2; scA3 = 'h215; end
    10: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1E3; scA3 = 'h215; end
    11: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    default: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    endcase
end

else if( opcode[11:6] == 'h21) begin
    unique case ( col)
     0: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h3EF; scA3 = 'X; end
     1: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
     2: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h006; scA3 = 'h081; end
     3: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h21C; scA3 = 'h081; end
     4: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h103; scA3 = 'h081; end
     5: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1C2; scA3 = 'h081; end
     6: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1E3; scA3 = 'h081; end
     7: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h00A; scA3 = 'h081; end
     8: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1E2; scA3 = 'h081; end
     9: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    10: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    11: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    default: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    endcase
end

else if( opcode[11:6] == 'h23) begin
    unique case ( col)
     0: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h3EF; scA3 = 'X; end
     1: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
     2: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h006; scA3 = 'h081; end
     3: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h21C; scA3 = 'h081; end
     4: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h103; scA3 = 'h081; end
     5: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1C2; scA3 = 'h081; end
     6: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1E3; scA3 = 'h081; end
     7: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h00A; scA3 = 'h081; end
     8: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1E2; scA3 = 'h081; end
     9: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    10: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    11: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    default: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    endcase
end

else if( opcode[11:6] == 'h22) begin
    unique case ( col)
     0: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h3EB; scA3 = 'X; end
     1: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
     2: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h006; scA3 = 'h069; end
     3: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h21C; scA3 = 'h069; end
     4: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h103; scA3 = 'h069; end
     5: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1C2; scA3 = 'h069; end
     6: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1E3; scA3 = 'h069; end
     7: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h00A; scA3 = 'h069; end
     8: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1E2; scA3 = 'h069; end
     9: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    10: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    11: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    default: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    endcase
end

else if( opcode[11:6] == 'h30) begin
    unique case ( col)
     0: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h108; scA3 = 'X; end
     1: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
     2: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h006; scA3 = 'h087; end
     3: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h21C; scA3 = 'h087; end
     4: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h103; scA3 = 'h087; end
     5: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1C2; scA3 = 'h087; end
     6: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1E3; scA3 = 'h087; end
     7: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h00A; scA3 = 'h087; end
     8: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1E2; scA3 = 'h087; end
     9: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    10: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    11: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    default: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    endcase
end

else if( opcode[11:6] == 'h31) begin
    unique case ( col)
     0: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h108; scA3 = 'X; end
     1: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
     2: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h006; scA3 = 'h087; end
     3: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h21C; scA3 = 'h087; end
     4: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h103; scA3 = 'h087; end
     5: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1C2; scA3 = 'h087; end
     6: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1E3; scA3 = 'h087; end
     7: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h00A; scA3 = 'h087; end
     8: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h2B9; arA23[ 'h0] = 'h1E2; scA3 = 'h087; end
     9: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    10: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    11: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    default: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    endcase
end

else if( opcode[11:6] == 'h32) begin
    unique case ( col)
     0: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h104; scA3 = 'X; end
     1: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
     2: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h00B; scA3 = 'h08F; end
     3: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h00F; scA3 = 'h08F; end
     4: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h179; scA3 = 'h08F; end
     5: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h1C6; scA3 = 'h08F; end
     6: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h1E7; scA3 = 'h08F; end
     7: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h00E; scA3 = 'h08F; end
     8: begin arIll[ 'h0] = 1'b0; arA1[ 'h0] = 'h3E0; arA23[ 'h0] = 'h1E6; scA3 = 'h08F; end
     9: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    10: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    11: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    default: begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X;    scA3 = 'X; end
    endcase
end

else begin arIll[ 'h0] = 1'b1; arA1[ 'h0] = 'X   ; arA23[ 'h0] = 'X; scA3 = 'X; end

end


//
// Line: 4
//
always_comb begin

if( (opcode[11:6] & 'h27) == 'h0) begin
    unique case ( col)
     0: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h133; arA23[ 'h4] = 'X; end
     1: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     2: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h006; arA23[ 'h4] = 'h2B8; end
     3: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h21C; arA23[ 'h4] = 'h2B8; end
     4: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h103; arA23[ 'h4] = 'h2B8; end
     5: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1C2; arA23[ 'h4] = 'h2B8; end
     6: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E3; arA23[ 'h4] = 'h2B8; end
     7: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h00A; arA23[ 'h4] = 'h2B8; end
     8: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E2; arA23[ 'h4] = 'h2B8; end
     9: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    10: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    11: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    default: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    endcase
end

else if( (opcode[11:6] & 'h27) == 'h1) begin
    unique case ( col)
     0: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h133; arA23[ 'h4] = 'X; end
     1: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     2: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h006; arA23[ 'h4] = 'h2B8; end
     3: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h21C; arA23[ 'h4] = 'h2B8; end
     4: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h103; arA23[ 'h4] = 'h2B8; end
     5: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1C2; arA23[ 'h4] = 'h2B8; end
     6: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E3; arA23[ 'h4] = 'h2B8; end
     7: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h00A; arA23[ 'h4] = 'h2B8; end
     8: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E2; arA23[ 'h4] = 'h2B8; end
     9: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    10: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    11: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    default: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    endcase
end

else if( (opcode[11:6] & 'h27) == 'h2) begin
    unique case ( col)
     0: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h137; arA23[ 'h4] = 'X; end
     1: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     2: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h00B; arA23[ 'h4] = 'h2BC; end
     3: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h00F; arA23[ 'h4] = 'h2BC; end
     4: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h179; arA23[ 'h4] = 'h2BC; end
     5: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1C6; arA23[ 'h4] = 'h2BC; end
     6: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E7; arA23[ 'h4] = 'h2BC; end
     7: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h00E; arA23[ 'h4] = 'h2BC; end
     8: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E6; arA23[ 'h4] = 'h2BC; end
     9: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    10: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    11: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    default: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    endcase
end

else if( opcode[11:6] == 'h3) begin
    unique case ( col)
     0: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h3A5; arA23[ 'h4] = 'X; end
     1: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     2: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h006; arA23[ 'h4] = 'h3A1; end
     3: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h21C; arA23[ 'h4] = 'h3A1; end
     4: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h103; arA23[ 'h4] = 'h3A1; end
     5: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1C2; arA23[ 'h4] = 'h3A1; end
     6: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E3; arA23[ 'h4] = 'h3A1; end
     7: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h00A; arA23[ 'h4] = 'h3A1; end
     8: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E2; arA23[ 'h4] = 'h3A1; end
     9: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    10: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    11: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    default: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    endcase
end

else if( opcode[11:6] == 'h13) begin
    unique case ( col)
     0: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h301; arA23[ 'h4] = 'X; end
     1: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     2: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h006; arA23[ 'h4] = 'h159; end
     3: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h21C; arA23[ 'h4] = 'h159; end
     4: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h103; arA23[ 'h4] = 'h159; end
     5: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1C2; arA23[ 'h4] = 'h159; end
     6: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E3; arA23[ 'h4] = 'h159; end
     7: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h00A; arA23[ 'h4] = 'h159; end
     8: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E2; arA23[ 'h4] = 'h159; end
     9: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1C2; arA23[ 'h4] = 'h159; end
    10: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E3; arA23[ 'h4] = 'h159; end
    11: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h0EA; arA23[ 'h4] = 'h301; end
    default: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    endcase
end

else if( opcode[11:6] == 'h1B) begin
    unique case ( col)
     0: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h301; arA23[ 'h4] = 'X; end
     1: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     2: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h006; arA23[ 'h4] = 'h159; end
     3: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h21C; arA23[ 'h4] = 'h159; end
     4: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h103; arA23[ 'h4] = 'h159; end
     5: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1C2; arA23[ 'h4] = 'h159; end
     6: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E3; arA23[ 'h4] = 'h159; end
     7: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h00A; arA23[ 'h4] = 'h159; end
     8: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E2; arA23[ 'h4] = 'h159; end
     9: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1C2; arA23[ 'h4] = 'h159; end
    10: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E3; arA23[ 'h4] = 'h159; end
    11: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h0EA; arA23[ 'h4] = 'h301; end
    default: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    endcase
end

else if( opcode[11:6] == 'h20) begin
    unique case ( col)
     0: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h13B; arA23[ 'h4] = 'X; end
     1: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     2: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h006; arA23[ 'h4] = 'h15C; end
     3: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h21C; arA23[ 'h4] = 'h15C; end
     4: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h103; arA23[ 'h4] = 'h15C; end
     5: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1C2; arA23[ 'h4] = 'h15C; end
     6: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E3; arA23[ 'h4] = 'h15C; end
     7: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h00A; arA23[ 'h4] = 'h15C; end
     8: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E2; arA23[ 'h4] = 'h15C; end
     9: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    10: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    11: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    default: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    endcase
end

else if( opcode[11:6] == 'h21) begin
    unique case ( col)
     0: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h341; arA23[ 'h4] = 'X; end
     1: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     2: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h17C; arA23[ 'h4] = 'X; end
     3: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     4: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     5: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h17D; arA23[ 'h4] = 'X; end
     6: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1FF; arA23[ 'h4] = 'X; end
     7: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h178; arA23[ 'h4] = 'X; end
     8: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1FA; arA23[ 'h4] = 'X; end
     9: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h17D; arA23[ 'h4] = 'X; end
    10: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1FF; arA23[ 'h4] = 'X; end
    11: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    default: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    endcase
end

else if( opcode[11:6] == 'h22) begin
    unique case ( col)
     0: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h133; arA23[ 'h4] = 'X; end
     1: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     2: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h3A0; arA23[ 'h4] = 'X; end
     3: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     4: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h3A4; arA23[ 'h4] = 'X; end
     5: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1F1; arA23[ 'h4] = 'X; end
     6: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h325; arA23[ 'h4] = 'X; end
     7: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1ED; arA23[ 'h4] = 'X; end
     8: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E5; arA23[ 'h4] = 'X; end
     9: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    10: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    11: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    default: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    endcase
end

else if( opcode[11:6] == 'h23) begin
    unique case ( col)
     0: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h232; arA23[ 'h4] = 'X; end
     1: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     2: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h3A0; arA23[ 'h4] = 'X; end
     3: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     4: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h3A4; arA23[ 'h4] = 'X; end
     5: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1F1; arA23[ 'h4] = 'X; end
     6: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h325; arA23[ 'h4] = 'X; end
     7: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1ED; arA23[ 'h4] = 'X; end
     8: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E5; arA23[ 'h4] = 'X; end
     9: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    10: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    11: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    default: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    endcase
end

else if( opcode[11:6] == 'h28) begin
    unique case ( col)
     0: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h12D; arA23[ 'h4] = 'X; end
     1: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     2: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h006; arA23[ 'h4] = 'h3C3; end
     3: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h21C; arA23[ 'h4] = 'h3C3; end
     4: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h103; arA23[ 'h4] = 'h3C3; end
     5: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1C2; arA23[ 'h4] = 'h3C3; end
     6: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E3; arA23[ 'h4] = 'h3C3; end
     7: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h00A; arA23[ 'h4] = 'h3C3; end
     8: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E2; arA23[ 'h4] = 'h3C3; end
     9: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    10: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    11: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    default: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    endcase
end

else if( opcode[11:6] == 'h29) begin
    unique case ( col)
     0: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h12D; arA23[ 'h4] = 'X; end
     1: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     2: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h006; arA23[ 'h4] = 'h3C3; end
     3: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h21C; arA23[ 'h4] = 'h3C3; end
     4: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h103; arA23[ 'h4] = 'h3C3; end
     5: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1C2; arA23[ 'h4] = 'h3C3; end
     6: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E3; arA23[ 'h4] = 'h3C3; end
     7: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h00A; arA23[ 'h4] = 'h3C3; end
     8: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E2; arA23[ 'h4] = 'h3C3; end
     9: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    10: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    11: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    default: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    endcase
end

else if( opcode[11:6] == 'h2A) begin
    unique case ( col)
     0: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h125; arA23[ 'h4] = 'X; end
     1: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     2: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h00B; arA23[ 'h4] = 'h3CB; end
     3: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h00F; arA23[ 'h4] = 'h3CB; end
     4: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h179; arA23[ 'h4] = 'h3CB; end
     5: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1C6; arA23[ 'h4] = 'h3CB; end
     6: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E7; arA23[ 'h4] = 'h3CB; end
     7: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h00E; arA23[ 'h4] = 'h3CB; end
     8: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E6; arA23[ 'h4] = 'h3CB; end
     9: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    10: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    11: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    default: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    endcase
end

else if( opcode[11:6] == 'h2B) begin
    unique case ( col)
     0: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h345; arA23[ 'h4] = 'X; end
     1: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     2: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h006; arA23[ 'h4] = 'h343; end
     3: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h21C; arA23[ 'h4] = 'h343; end
     4: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h103; arA23[ 'h4] = 'h343; end
     5: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1C2; arA23[ 'h4] = 'h343; end
     6: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E3; arA23[ 'h4] = 'h343; end
     7: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h00A; arA23[ 'h4] = 'h343; end
     8: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E2; arA23[ 'h4] = 'h343; end
     9: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    10: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    11: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    default: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    endcase
end

else if( (opcode[11:6] & 'h3E) == 'h32) begin
    unique case ( col)
     0: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     1: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     2: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h127; arA23[ 'h4] = 'X; end
     3: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h123; arA23[ 'h4] = 'X; end
     4: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     5: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1FD; arA23[ 'h4] = 'X; end
     6: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1F5; arA23[ 'h4] = 'X; end
     7: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1F9; arA23[ 'h4] = 'X; end
     8: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E9; arA23[ 'h4] = 'X; end
     9: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1FD; arA23[ 'h4] = 'X; end
    10: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1F5; arA23[ 'h4] = 'X; end
    11: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    default: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    endcase
end

else if( (opcode[11:6] & 'h7) == 'h6) begin
    unique case ( col)
     0: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h152; arA23[ 'h4] = 'X; end
     1: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     2: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h006; arA23[ 'h4] = 'h151; end
     3: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h21C; arA23[ 'h4] = 'h151; end
     4: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h103; arA23[ 'h4] = 'h151; end
     5: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1C2; arA23[ 'h4] = 'h151; end
     6: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E3; arA23[ 'h4] = 'h151; end
     7: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h00A; arA23[ 'h4] = 'h151; end
     8: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E2; arA23[ 'h4] = 'h151; end
     9: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1C2; arA23[ 'h4] = 'h151; end
    10: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1E3; arA23[ 'h4] = 'h151; end
    11: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h0EA; arA23[ 'h4] = 'h152; end
    default: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    endcase
end

else if( (opcode[11:6] & 'h7) == 'h7) begin
    unique case ( col)
     0: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     1: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     2: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h2F1; arA23[ 'h4] = 'X; end
     3: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     4: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     5: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h2F2; arA23[ 'h4] = 'X; end
     6: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1FB; arA23[ 'h4] = 'X; end
     7: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h275; arA23[ 'h4] = 'X; end
     8: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h3E4; arA23[ 'h4] = 'X; end
     9: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h2F2; arA23[ 'h4] = 'X; end
    10: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1FB; arA23[ 'h4] = 'X; end
    11: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    default: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    endcase
end

else if( opcode[11:6] == 'h3A) begin
    unique case ( col)
     0: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     1: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     2: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h273; arA23[ 'h4] = 'X; end
     3: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     4: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     5: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h2B0; arA23[ 'h4] = 'X; end
     6: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1F3; arA23[ 'h4] = 'X; end
     7: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h293; arA23[ 'h4] = 'X; end
     8: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1F2; arA23[ 'h4] = 'X; end
     9: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h2B0; arA23[ 'h4] = 'X; end
    10: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1F3; arA23[ 'h4] = 'X; end
    11: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    default: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    endcase
end

else if( opcode[11:6] == 'h3B) begin
    unique case ( col)
     0: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     1: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     2: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h255; arA23[ 'h4] = 'X; end
     3: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     4: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
     5: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h2B4; arA23[ 'h4] = 'X; end
     6: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1F7; arA23[ 'h4] = 'X; end
     7: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h297; arA23[ 'h4] = 'X; end
     8: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1F6; arA23[ 'h4] = 'X; end
     9: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h2B4; arA23[ 'h4] = 'X; end
    10: begin arIll[ 'h4] = 1'b0; arA1[ 'h4] = 'h1F7; arA23[ 'h4] = 'X; end
    11: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    default: begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end
    endcase
end

else if( opcode[11:6] == 'h39) begin
     arIll[ 'h4] = illMisc; arA1[ 'h4] = a1Misc   ; arA23[ 'h4] = 'X;
end

else begin arIll[ 'h4] = 1'b1; arA1[ 'h4] = 'X   ; arA23[ 'h4] = 'X; end

end

always_comb begin

//
// Line: 1
//
unique case( movEa)

0: // Row: 0
    unique case ( col)
     0: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h121; arA23[ 'h1] = 'X; end
     1: begin arIll[ 'h1] = 1'b1; arA1[ 'h1] = 'X   ; arA23[ 'h1] = 'X; end
     2: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h006; arA23[ 'h1] = 'h29B; end
     3: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h21C; arA23[ 'h1] = 'h29B; end
     4: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h103; arA23[ 'h1] = 'h29B; end
     5: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1C2; arA23[ 'h1] = 'h29B; end
     6: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1E3; arA23[ 'h1] = 'h29B; end
     7: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h00A; arA23[ 'h1] = 'h29B; end
     8: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1E2; arA23[ 'h1] = 'h29B; end
     9: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1C2; arA23[ 'h1] = 'h29B; end
    10: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1E3; arA23[ 'h1] = 'h29B; end
    11: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h0EA; arA23[ 'h1] = 'h121; end
    default: begin arIll[ 'h1] = 1'b1; arA1[ 'h1] = 'X   ; arA23[ 'h1] = 'X; end
    endcase

2: // Row: 2
    unique case ( col)
     0: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h2FA; arA23[ 'h1] = 'X; end
     1: begin arIll[ 'h1] = 1'b1; arA1[ 'h1] = 'X   ; arA23[ 'h1] = 'X; end
     2: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h006; arA23[ 'h1] = 'h3AB; end
     3: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h21C; arA23[ 'h1] = 'h3AB; end
     4: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h103; arA23[ 'h1] = 'h3AB; end
     5: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1C2; arA23[ 'h1] = 'h3AB; end
     6: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1E3; arA23[ 'h1] = 'h3AB; end
     7: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h00A; arA23[ 'h1] = 'h3AB; end
     8: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1E2; arA23[ 'h1] = 'h3AB; end
     9: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1C2; arA23[ 'h1] = 'h3AB; end
    10: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1E3; arA23[ 'h1] = 'h3AB; end
    11: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h0EA; arA23[ 'h1] = 'h2FA; end
    default: begin arIll[ 'h1] = 1'b1; arA1[ 'h1] = 'X   ; arA23[ 'h1] = 'X; end
    endcase

3: // Row: 3
    unique case ( col)
     0: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h2FE; arA23[ 'h1] = 'X; end
     1: begin arIll[ 'h1] = 1'b1; arA1[ 'h1] = 'X   ; arA23[ 'h1] = 'X; end
     2: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h006; arA23[ 'h1] = 'h3AF; end
     3: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h21C; arA23[ 'h1] = 'h3AF; end
     4: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h103; arA23[ 'h1] = 'h3AF; end
     5: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1C2; arA23[ 'h1] = 'h3AF; end
     6: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1E3; arA23[ 'h1] = 'h3AF; end
     7: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h00A; arA23[ 'h1] = 'h3AF; end
     8: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1E2; arA23[ 'h1] = 'h3AF; end
     9: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1C2; arA23[ 'h1] = 'h3AF; end
    10: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1E3; arA23[ 'h1] = 'h3AF; end
    11: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h0EA; arA23[ 'h1] = 'h2FE; end
    default: begin arIll[ 'h1] = 1'b1; arA1[ 'h1] = 'X   ; arA23[ 'h1] = 'X; end
    endcase

4: // Row: 4
    unique case ( col)
     0: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h2F8; arA23[ 'h1] = 'X; end
     1: begin arIll[ 'h1] = 1'b1; arA1[ 'h1] = 'X   ; arA23[ 'h1] = 'X; end
     2: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h006; arA23[ 'h1] = 'h38B; end
     3: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h21C; arA23[ 'h1] = 'h38B; end
     4: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h103; arA23[ 'h1] = 'h38B; end
     5: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1C2; arA23[ 'h1] = 'h38B; end
     6: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1E3; arA23[ 'h1] = 'h38B; end
     7: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h00A; arA23[ 'h1] = 'h38B; end
     8: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1E2; arA23[ 'h1] = 'h38B; end
     9: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1C2; arA23[ 'h1] = 'h38B; end
    10: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1E3; arA23[ 'h1] = 'h38B; end
    11: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h0EA; arA23[ 'h1] = 'h2F8; end
    default: begin arIll[ 'h1] = 1'b1; arA1[ 'h1] = 'X   ; arA23[ 'h1] = 'X; end
    endcase

5: // Row: 5
    unique case ( col)
     0: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h2DA; arA23[ 'h1] = 'X; end
     1: begin arIll[ 'h1] = 1'b1; arA1[ 'h1] = 'X   ; arA23[ 'h1] = 'X; end
     2: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h006; arA23[ 'h1] = 'h38A; end
     3: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h21C; arA23[ 'h1] = 'h38A; end
     4: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h103; arA23[ 'h1] = 'h38A; end
     5: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1C2; arA23[ 'h1] = 'h38A; end
     6: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1E3; arA23[ 'h1] = 'h38A; end
     7: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h00A; arA23[ 'h1] = 'h38A; end
     8: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1E2; arA23[ 'h1] = 'h38A; end
     9: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1C2; arA23[ 'h1] = 'h38A; end
    10: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1E3; arA23[ 'h1] = 'h38A; end
    11: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h0EA; arA23[ 'h1] = 'h2DA; end
    default: begin arIll[ 'h1] = 1'b1; arA1[ 'h1] = 'X   ; arA23[ 'h1] = 'X; end
    endcase

6: // Row: 6
    unique case ( col)
     0: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1EB; arA23[ 'h1] = 'X; end
     1: begin arIll[ 'h1] = 1'b1; arA1[ 'h1] = 'X   ; arA23[ 'h1] = 'X; end
     2: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h006; arA23[ 'h1] = 'h298; end
     3: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h21C; arA23[ 'h1] = 'h298; end
     4: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h103; arA23[ 'h1] = 'h298; end
     5: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1C2; arA23[ 'h1] = 'h298; end
     6: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1E3; arA23[ 'h1] = 'h298; end
     7: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h00A; arA23[ 'h1] = 'h298; end
     8: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1E2; arA23[ 'h1] = 'h298; end
     9: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1C2; arA23[ 'h1] = 'h298; end
    10: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1E3; arA23[ 'h1] = 'h298; end
    11: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h0EA; arA23[ 'h1] = 'h1EB; end
    default: begin arIll[ 'h1] = 1'b1; arA1[ 'h1] = 'X   ; arA23[ 'h1] = 'X; end
    endcase

7: // Row: 7
    unique case ( col)
     0: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h2D9; arA23[ 'h1] = 'X; end
     1: begin arIll[ 'h1] = 1'b1; arA1[ 'h1] = 'X   ; arA23[ 'h1] = 'X; end
     2: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h006; arA23[ 'h1] = 'h388; end
     3: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h21C; arA23[ 'h1] = 'h388; end
     4: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h103; arA23[ 'h1] = 'h388; end
     5: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1C2; arA23[ 'h1] = 'h388; end
     6: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1E3; arA23[ 'h1] = 'h388; end
     7: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h00A; arA23[ 'h1] = 'h388; end
     8: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1E2; arA23[ 'h1] = 'h388; end
     9: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1C2; arA23[ 'h1] = 'h388; end
    10: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1E3; arA23[ 'h1] = 'h388; end
    11: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h0EA; arA23[ 'h1] = 'h2D9; end
    default: begin arIll[ 'h1] = 1'b1; arA1[ 'h1] = 'X   ; arA23[ 'h1] = 'X; end
    endcase

8: // Row: 8
    unique case ( col)
     0: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1EA; arA23[ 'h1] = 'X; end
     1: begin arIll[ 'h1] = 1'b1; arA1[ 'h1] = 'X   ; arA23[ 'h1] = 'X; end
     2: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h006; arA23[ 'h1] = 'h32B; end
     3: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h21C; arA23[ 'h1] = 'h32B; end
     4: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h103; arA23[ 'h1] = 'h32B; end
     5: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1C2; arA23[ 'h1] = 'h32B; end
     6: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1E3; arA23[ 'h1] = 'h32B; end
     7: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h00A; arA23[ 'h1] = 'h32B; end
     8: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1E2; arA23[ 'h1] = 'h32B; end
     9: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1C2; arA23[ 'h1] = 'h32B; end
    10: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h1E3; arA23[ 'h1] = 'h32B; end
    11: begin arIll[ 'h1] = 1'b0; arA1[ 'h1] = 'h0EA; arA23[ 'h1] = 'h1EA; end
    default: begin arIll[ 'h1] = 1'b1; arA1[ 'h1] = 'X   ; arA23[ 'h1] = 'X; end
    endcase
default:  begin arIll[ 'h1] = 1'b1; arA1[ 'h1] = 'X   ; arA23[ 'h1] = 'X; end
endcase

//
// Line: 2
//
unique case( movEa)

0: // Row: 0
    unique case ( col)
     0: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h129; arA23[ 'h2] = 'X; end
     1: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h129; arA23[ 'h2] = 'X; end
     2: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h00B; arA23[ 'h2] = 'h29F; end
     3: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h00F; arA23[ 'h2] = 'h29F; end
     4: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h179; arA23[ 'h2] = 'h29F; end
     5: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1C6; arA23[ 'h2] = 'h29F; end
     6: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1E7; arA23[ 'h2] = 'h29F; end
     7: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h00E; arA23[ 'h2] = 'h29F; end
     8: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1E6; arA23[ 'h2] = 'h29F; end
     9: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1C6; arA23[ 'h2] = 'h29F; end
    10: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1E7; arA23[ 'h2] = 'h29F; end
    11: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h0A7; arA23[ 'h2] = 'h129; end
    default: begin arIll[ 'h2] = 1'b1; arA1[ 'h2] = 'X   ; arA23[ 'h2] = 'X; end
    endcase

1: // Row: 1
    unique case ( col)
     0: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h129; arA23[ 'h2] = 'X; end
     1: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h129; arA23[ 'h2] = 'X; end
     2: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h00B; arA23[ 'h2] = 'h29F; end
     3: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h00F; arA23[ 'h2] = 'h29F; end
     4: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h179; arA23[ 'h2] = 'h29F; end
     5: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1C6; arA23[ 'h2] = 'h29F; end
     6: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1E7; arA23[ 'h2] = 'h29F; end
     7: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h00E; arA23[ 'h2] = 'h29F; end
     8: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1E6; arA23[ 'h2] = 'h29F; end
     9: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1C6; arA23[ 'h2] = 'h29F; end
    10: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1E7; arA23[ 'h2] = 'h29F; end
    11: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h0A7; arA23[ 'h2] = 'h129; end
    default: begin arIll[ 'h2] = 1'b1; arA1[ 'h2] = 'X   ; arA23[ 'h2] = 'X; end
    endcase

2: // Row: 2
    unique case ( col)
     0: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h2F9; arA23[ 'h2] = 'X; end
     1: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h2F9; arA23[ 'h2] = 'X; end
     2: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h00B; arA23[ 'h2] = 'h3A9; end
     3: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h00F; arA23[ 'h2] = 'h3A9; end
     4: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h179; arA23[ 'h2] = 'h3A9; end
     5: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1C6; arA23[ 'h2] = 'h3A9; end
     6: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1E7; arA23[ 'h2] = 'h3A9; end
     7: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h00E; arA23[ 'h2] = 'h3A9; end
     8: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1E6; arA23[ 'h2] = 'h3A9; end
     9: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1C6; arA23[ 'h2] = 'h3A9; end
    10: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1E7; arA23[ 'h2] = 'h3A9; end
    11: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h0A7; arA23[ 'h2] = 'h2F9; end
    default: begin arIll[ 'h2] = 1'b1; arA1[ 'h2] = 'X   ; arA23[ 'h2] = 'X; end
    endcase

3: // Row: 3
    unique case ( col)
     0: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h2FD; arA23[ 'h2] = 'X; end
     1: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h2FD; arA23[ 'h2] = 'X; end
     2: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h00B; arA23[ 'h2] = 'h3AD; end
     3: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h00F; arA23[ 'h2] = 'h3AD; end
     4: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h179; arA23[ 'h2] = 'h3AD; end
     5: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1C6; arA23[ 'h2] = 'h3AD; end
     6: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1E7; arA23[ 'h2] = 'h3AD; end
     7: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h00E; arA23[ 'h2] = 'h3AD; end
     8: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1E6; arA23[ 'h2] = 'h3AD; end
     9: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1C6; arA23[ 'h2] = 'h3AD; end
    10: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1E7; arA23[ 'h2] = 'h3AD; end
    11: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h0A7; arA23[ 'h2] = 'h2FD; end
    default: begin arIll[ 'h2] = 1'b1; arA1[ 'h2] = 'X   ; arA23[ 'h2] = 'X; end
    endcase

4: // Row: 4
    unique case ( col)
     0: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h2FC; arA23[ 'h2] = 'X; end
     1: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h2FC; arA23[ 'h2] = 'X; end
     2: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h00B; arA23[ 'h2] = 'h38F; end
     3: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h00F; arA23[ 'h2] = 'h38F; end
     4: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h179; arA23[ 'h2] = 'h38F; end
     5: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1C6; arA23[ 'h2] = 'h38F; end
     6: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1E7; arA23[ 'h2] = 'h38F; end
     7: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h00E; arA23[ 'h2] = 'h38F; end
     8: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1E6; arA23[ 'h2] = 'h38F; end
     9: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1C6; arA23[ 'h2] = 'h38F; end
    10: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1E7; arA23[ 'h2] = 'h38F; end
    11: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h0A7; arA23[ 'h2] = 'h2FC; end
    default: begin arIll[ 'h2] = 1'b1; arA1[ 'h2] = 'X   ; arA23[ 'h2] = 'X; end
    endcase

5: // Row: 5
    unique case ( col)
     0: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h2DE; arA23[ 'h2] = 'X; end
     1: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h2DE; arA23[ 'h2] = 'X; end
     2: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h00B; arA23[ 'h2] = 'h38E; end
     3: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h00F; arA23[ 'h2] = 'h38E; end
     4: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h179; arA23[ 'h2] = 'h38E; end
     5: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1C6; arA23[ 'h2] = 'h38E; end
     6: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1E7; arA23[ 'h2] = 'h38E; end
     7: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h00E; arA23[ 'h2] = 'h38E; end
     8: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1E6; arA23[ 'h2] = 'h38E; end
     9: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1C6; arA23[ 'h2] = 'h38E; end
    10: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1E7; arA23[ 'h2] = 'h38E; end
    11: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h0A7; arA23[ 'h2] = 'h2DE; end
    default: begin arIll[ 'h2] = 1'b1; arA1[ 'h2] = 'X   ; arA23[ 'h2] = 'X; end
    endcase

6: // Row: 6
    unique case ( col)
     0: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1EF; arA23[ 'h2] = 'X; end
     1: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1EF; arA23[ 'h2] = 'X; end
     2: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h00B; arA23[ 'h2] = 'h29C; end
     3: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h00F; arA23[ 'h2] = 'h29C; end
     4: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h179; arA23[ 'h2] = 'h29C; end
     5: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1C6; arA23[ 'h2] = 'h29C; end
     6: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1E7; arA23[ 'h2] = 'h29C; end
     7: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h00E; arA23[ 'h2] = 'h29C; end
     8: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1E6; arA23[ 'h2] = 'h29C; end
     9: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1C6; arA23[ 'h2] = 'h29C; end
    10: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1E7; arA23[ 'h2] = 'h29C; end
    11: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h0A7; arA23[ 'h2] = 'h1EF; end
    default: begin arIll[ 'h2] = 1'b1; arA1[ 'h2] = 'X   ; arA23[ 'h2] = 'X; end
    endcase

7: // Row: 7
    unique case ( col)
     0: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h2DD; arA23[ 'h2] = 'X; end
     1: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h2DD; arA23[ 'h2] = 'X; end
     2: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h00B; arA23[ 'h2] = 'h38C; end
     3: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h00F; arA23[ 'h2] = 'h38C; end
     4: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h179; arA23[ 'h2] = 'h38C; end
     5: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1C6; arA23[ 'h2] = 'h38C; end
     6: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1E7; arA23[ 'h2] = 'h38C; end
     7: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h00E; arA23[ 'h2] = 'h38C; end
     8: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1E6; arA23[ 'h2] = 'h38C; end
     9: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1C6; arA23[ 'h2] = 'h38C; end
    10: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1E7; arA23[ 'h2] = 'h38C; end
    11: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h0A7; arA23[ 'h2] = 'h2DD; end
    default: begin arIll[ 'h2] = 1'b1; arA1[ 'h2] = 'X   ; arA23[ 'h2] = 'X; end
    endcase

8: // Row: 8
    unique case ( col)
     0: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1EE; arA23[ 'h2] = 'X; end
     1: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1EE; arA23[ 'h2] = 'X; end
     2: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h00B; arA23[ 'h2] = 'h30F; end
     3: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h00F; arA23[ 'h2] = 'h30F; end
     4: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h179; arA23[ 'h2] = 'h30F; end
     5: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1C6; arA23[ 'h2] = 'h30F; end
     6: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1E7; arA23[ 'h2] = 'h30F; end
     7: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h00E; arA23[ 'h2] = 'h30F; end
     8: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1E6; arA23[ 'h2] = 'h30F; end
     9: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1C6; arA23[ 'h2] = 'h30F; end
    10: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h1E7; arA23[ 'h2] = 'h30F; end
    11: begin arIll[ 'h2] = 1'b0; arA1[ 'h2] = 'h0A7; arA23[ 'h2] = 'h1EE; end
    default: begin arIll[ 'h2] = 1'b1; arA1[ 'h2] = 'X   ; arA23[ 'h2] = 'X; end
    endcase
default:  begin arIll[ 'h2] = 1'b1; arA1[ 'h2] = 'X   ; arA23[ 'h2] = 'X; end
endcase

//
// Line: 3
//
unique case( movEa)

0: // Row: 0
    unique case ( col)
     0: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h121; arA23[ 'h3] = 'X; end
     1: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h121; arA23[ 'h3] = 'X; end
     2: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h006; arA23[ 'h3] = 'h29B; end
     3: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h21C; arA23[ 'h3] = 'h29B; end
     4: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h103; arA23[ 'h3] = 'h29B; end
     5: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1C2; arA23[ 'h3] = 'h29B; end
     6: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1E3; arA23[ 'h3] = 'h29B; end
     7: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h00A; arA23[ 'h3] = 'h29B; end
     8: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1E2; arA23[ 'h3] = 'h29B; end
     9: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1C2; arA23[ 'h3] = 'h29B; end
    10: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1E3; arA23[ 'h3] = 'h29B; end
    11: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h0EA; arA23[ 'h3] = 'h121; end
    default: begin arIll[ 'h3] = 1'b1; arA1[ 'h3] = 'X   ; arA23[ 'h3] = 'X; end
    endcase

1: // Row: 1
    unique case ( col)
     0: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h279; arA23[ 'h3] = 'X; end
     1: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h279; arA23[ 'h3] = 'X; end
     2: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h006; arA23[ 'h3] = 'h158; end
     3: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h21C; arA23[ 'h3] = 'h158; end
     4: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h103; arA23[ 'h3] = 'h158; end
     5: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1C2; arA23[ 'h3] = 'h158; end
     6: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1E3; arA23[ 'h3] = 'h158; end
     7: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h00A; arA23[ 'h3] = 'h158; end
     8: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1E2; arA23[ 'h3] = 'h158; end
     9: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1C2; arA23[ 'h3] = 'h158; end
    10: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1E3; arA23[ 'h3] = 'h158; end
    11: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h0EA; arA23[ 'h3] = 'h279; end
    default: begin arIll[ 'h3] = 1'b1; arA1[ 'h3] = 'X   ; arA23[ 'h3] = 'X; end
    endcase

2: // Row: 2
    unique case ( col)
     0: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h2FA; arA23[ 'h3] = 'X; end
     1: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h2FA; arA23[ 'h3] = 'X; end
     2: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h006; arA23[ 'h3] = 'h3AB; end
     3: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h21C; arA23[ 'h3] = 'h3AB; end
     4: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h103; arA23[ 'h3] = 'h3AB; end
     5: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1C2; arA23[ 'h3] = 'h3AB; end
     6: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1E3; arA23[ 'h3] = 'h3AB; end
     7: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h00A; arA23[ 'h3] = 'h3AB; end
     8: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1E2; arA23[ 'h3] = 'h3AB; end
     9: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1C2; arA23[ 'h3] = 'h3AB; end
    10: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1E3; arA23[ 'h3] = 'h3AB; end
    11: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h0EA; arA23[ 'h3] = 'h2FA; end
    default: begin arIll[ 'h3] = 1'b1; arA1[ 'h3] = 'X   ; arA23[ 'h3] = 'X; end
    endcase

3: // Row: 3
    unique case ( col)
     0: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h2FE; arA23[ 'h3] = 'X; end
     1: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h2FE; arA23[ 'h3] = 'X; end
     2: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h006; arA23[ 'h3] = 'h3AF; end
     3: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h21C; arA23[ 'h3] = 'h3AF; end
     4: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h103; arA23[ 'h3] = 'h3AF; end
     5: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1C2; arA23[ 'h3] = 'h3AF; end
     6: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1E3; arA23[ 'h3] = 'h3AF; end
     7: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h00A; arA23[ 'h3] = 'h3AF; end
     8: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1E2; arA23[ 'h3] = 'h3AF; end
     9: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1C2; arA23[ 'h3] = 'h3AF; end
    10: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1E3; arA23[ 'h3] = 'h3AF; end
    11: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h0EA; arA23[ 'h3] = 'h2FE; end
    default: begin arIll[ 'h3] = 1'b1; arA1[ 'h3] = 'X   ; arA23[ 'h3] = 'X; end
    endcase

4: // Row: 4
    unique case ( col)
     0: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h2F8; arA23[ 'h3] = 'X; end
     1: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h2F8; arA23[ 'h3] = 'X; end
     2: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h006; arA23[ 'h3] = 'h38B; end
     3: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h21C; arA23[ 'h3] = 'h38B; end
     4: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h103; arA23[ 'h3] = 'h38B; end
     5: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1C2; arA23[ 'h3] = 'h38B; end
     6: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1E3; arA23[ 'h3] = 'h38B; end
     7: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h00A; arA23[ 'h3] = 'h38B; end
     8: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1E2; arA23[ 'h3] = 'h38B; end
     9: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1C2; arA23[ 'h3] = 'h38B; end
    10: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1E3; arA23[ 'h3] = 'h38B; end
    11: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h0EA; arA23[ 'h3] = 'h2F8; end
    default: begin arIll[ 'h3] = 1'b1; arA1[ 'h3] = 'X   ; arA23[ 'h3] = 'X; end
    endcase

5: // Row: 5
    unique case ( col)
     0: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h2DA; arA23[ 'h3] = 'X; end
     1: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h2DA; arA23[ 'h3] = 'X; end
     2: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h006; arA23[ 'h3] = 'h38A; end
     3: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h21C; arA23[ 'h3] = 'h38A; end
     4: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h103; arA23[ 'h3] = 'h38A; end
     5: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1C2; arA23[ 'h3] = 'h38A; end
     6: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1E3; arA23[ 'h3] = 'h38A; end
     7: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h00A; arA23[ 'h3] = 'h38A; end
     8: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1E2; arA23[ 'h3] = 'h38A; end
     9: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1C2; arA23[ 'h3] = 'h38A; end
    10: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1E3; arA23[ 'h3] = 'h38A; end
    11: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h0EA; arA23[ 'h3] = 'h2DA; end
    default: begin arIll[ 'h3] = 1'b1; arA1[ 'h3] = 'X   ; arA23[ 'h3] = 'X; end
    endcase

6: // Row: 6
    unique case ( col)
     0: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1EB; arA23[ 'h3] = 'X; end
     1: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1EB; arA23[ 'h3] = 'X; end
     2: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h006; arA23[ 'h3] = 'h298; end
     3: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h21C; arA23[ 'h3] = 'h298; end
     4: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h103; arA23[ 'h3] = 'h298; end
     5: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1C2; arA23[ 'h3] = 'h298; end
     6: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1E3; arA23[ 'h3] = 'h298; end
     7: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h00A; arA23[ 'h3] = 'h298; end
     8: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1E2; arA23[ 'h3] = 'h298; end
     9: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1C2; arA23[ 'h3] = 'h298; end
    10: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1E3; arA23[ 'h3] = 'h298; end
    11: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h0EA; arA23[ 'h3] = 'h1EB; end
    default: begin arIll[ 'h3] = 1'b1; arA1[ 'h3] = 'X   ; arA23[ 'h3] = 'X; end
    endcase

7: // Row: 7
    unique case ( col)
     0: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h2D9; arA23[ 'h3] = 'X; end
     1: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h2D9; arA23[ 'h3] = 'X; end
     2: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h006; arA23[ 'h3] = 'h388; end
     3: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h21C; arA23[ 'h3] = 'h388; end
     4: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h103; arA23[ 'h3] = 'h388; end
     5: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1C2; arA23[ 'h3] = 'h388; end
     6: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1E3; arA23[ 'h3] = 'h388; end
     7: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h00A; arA23[ 'h3] = 'h388; end
     8: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1E2; arA23[ 'h3] = 'h388; end
     9: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1C2; arA23[ 'h3] = 'h388; end
    10: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1E3; arA23[ 'h3] = 'h388; end
    11: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h0EA; arA23[ 'h3] = 'h2D9; end
    default: begin arIll[ 'h3] = 1'b1; arA1[ 'h3] = 'X   ; arA23[ 'h3] = 'X; end
    endcase

8: // Row: 8
    unique case ( col)
     0: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1EA; arA23[ 'h3] = 'X; end
     1: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1EA; arA23[ 'h3] = 'X; end
     2: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h006; arA23[ 'h3] = 'h32B; end
     3: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h21C; arA23[ 'h3] = 'h32B; end
     4: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h103; arA23[ 'h3] = 'h32B; end
     5: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1C2; arA23[ 'h3] = 'h32B; end
     6: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1E3; arA23[ 'h3] = 'h32B; end
     7: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h00A; arA23[ 'h3] = 'h32B; end
     8: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1E2; arA23[ 'h3] = 'h32B; end
     9: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1C2; arA23[ 'h3] = 'h32B; end
    10: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h1E3; arA23[ 'h3] = 'h32B; end
    11: begin arIll[ 'h3] = 1'b0; arA1[ 'h3] = 'h0EA; arA23[ 'h3] = 'h1EA; end
    default: begin arIll[ 'h3] = 1'b1; arA1[ 'h3] = 'X   ; arA23[ 'h3] = 'X; end
    endcase
default:  begin arIll[ 'h3] = 1'b1; arA1[ 'h3] = 'X   ; arA23[ 'h3] = 'X; end
endcase

//
// Line: 5
//
unique case( row86)

3'b000: // Row: 0
    unique case ( col)
     0: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h2D8; arA23[ 'h5] = 'X; end
     1: begin arIll[ 'h5] = 1'b1; arA1[ 'h5] = 'X   ; arA23[ 'h5] = 'X; end
     2: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h006; arA23[ 'h5] = 'h2F3; end
     3: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h21C; arA23[ 'h5] = 'h2F3; end
     4: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h103; arA23[ 'h5] = 'h2F3; end
     5: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h1C2; arA23[ 'h5] = 'h2F3; end
     6: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h1E3; arA23[ 'h5] = 'h2F3; end
     7: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h00A; arA23[ 'h5] = 'h2F3; end
     8: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h1E2; arA23[ 'h5] = 'h2F3; end
    default: begin arIll[ 'h5] = 1'b1; arA1[ 'h5] = 'X   ; arA23[ 'h5] = 'X; end
    endcase

3'b001: // Row: 1
    unique case ( col)
     0: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h2D8; arA23[ 'h5] = 'X; end
     1: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h2DC; arA23[ 'h5] = 'X; end
     2: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h006; arA23[ 'h5] = 'h2F3; end
     3: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h21C; arA23[ 'h5] = 'h2F3; end
     4: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h103; arA23[ 'h5] = 'h2F3; end
     5: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h1C2; arA23[ 'h5] = 'h2F3; end
     6: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h1E3; arA23[ 'h5] = 'h2F3; end
     7: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h00A; arA23[ 'h5] = 'h2F3; end
     8: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h1E2; arA23[ 'h5] = 'h2F3; end
    default: begin arIll[ 'h5] = 1'b1; arA1[ 'h5] = 'X   ; arA23[ 'h5] = 'X; end
    endcase

3'b010: // Row: 2
    unique case ( col)
     0: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h2DC; arA23[ 'h5] = 'X; end
     1: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h2DC; arA23[ 'h5] = 'X; end
     2: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h00B; arA23[ 'h5] = 'h2F7; end
     3: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h00F; arA23[ 'h5] = 'h2F7; end
     4: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h179; arA23[ 'h5] = 'h2F7; end
     5: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h1C6; arA23[ 'h5] = 'h2F7; end
     6: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h1E7; arA23[ 'h5] = 'h2F7; end
     7: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h00E; arA23[ 'h5] = 'h2F7; end
     8: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h1E6; arA23[ 'h5] = 'h2F7; end
    default: begin arIll[ 'h5] = 1'b1; arA1[ 'h5] = 'X   ; arA23[ 'h5] = 'X; end
    endcase

3'b011: // Row: 3
    unique case ( col)
     0: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h384; arA23[ 'h5] = 'X; end
     1: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h06C; arA23[ 'h5] = 'X; end
     2: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h006; arA23[ 'h5] = 'h380; end
     3: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h21C; arA23[ 'h5] = 'h380; end
     4: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h103; arA23[ 'h5] = 'h380; end
     5: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h1C2; arA23[ 'h5] = 'h380; end
     6: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h1E3; arA23[ 'h5] = 'h380; end
     7: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h00A; arA23[ 'h5] = 'h380; end
     8: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h1E2; arA23[ 'h5] = 'h380; end
    default: begin arIll[ 'h5] = 1'b1; arA1[ 'h5] = 'X   ; arA23[ 'h5] = 'X; end
    endcase

3'b100: // Row: 4
    unique case ( col)
     0: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h2D8; arA23[ 'h5] = 'X; end
     1: begin arIll[ 'h5] = 1'b1; arA1[ 'h5] = 'X   ; arA23[ 'h5] = 'X; end
     2: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h006; arA23[ 'h5] = 'h2F3; end
     3: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h21C; arA23[ 'h5] = 'h2F3; end
     4: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h103; arA23[ 'h5] = 'h2F3; end
     5: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h1C2; arA23[ 'h5] = 'h2F3; end
     6: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h1E3; arA23[ 'h5] = 'h2F3; end
     7: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h00A; arA23[ 'h5] = 'h2F3; end
     8: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h1E2; arA23[ 'h5] = 'h2F3; end
    default: begin arIll[ 'h5] = 1'b1; arA1[ 'h5] = 'X   ; arA23[ 'h5] = 'X; end
    endcase

3'b101: // Row: 5
    unique case ( col)
     0: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h2D8; arA23[ 'h5] = 'X; end
     1: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h2DC; arA23[ 'h5] = 'X; end
     2: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h006; arA23[ 'h5] = 'h2F3; end
     3: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h21C; arA23[ 'h5] = 'h2F3; end
     4: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h103; arA23[ 'h5] = 'h2F3; end
     5: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h1C2; arA23[ 'h5] = 'h2F3; end
     6: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h1E3; arA23[ 'h5] = 'h2F3; end
     7: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h00A; arA23[ 'h5] = 'h2F3; end
     8: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h1E2; arA23[ 'h5] = 'h2F3; end
    default: begin arIll[ 'h5] = 1'b1; arA1[ 'h5] = 'X   ; arA23[ 'h5] = 'X; end
    endcase

3'b110: // Row: 6
    unique case ( col)
     0: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h2DC; arA23[ 'h5] = 'X; end
     1: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h2DC; arA23[ 'h5] = 'X; end
     2: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h00B; arA23[ 'h5] = 'h2F7; end
     3: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h00F; arA23[ 'h5] = 'h2F7; end
     4: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h179; arA23[ 'h5] = 'h2F7; end
     5: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h1C6; arA23[ 'h5] = 'h2F7; end
     6: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h1E7; arA23[ 'h5] = 'h2F7; end
     7: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h00E; arA23[ 'h5] = 'h2F7; end
     8: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h1E6; arA23[ 'h5] = 'h2F7; end
    default: begin arIll[ 'h5] = 1'b1; arA1[ 'h5] = 'X   ; arA23[ 'h5] = 'X; end
    endcase

3'b111: // Row: 7
    unique case ( col)
     0: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h384; arA23[ 'h5] = 'X; end
     1: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h06C; arA23[ 'h5] = 'X; end
     2: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h006; arA23[ 'h5] = 'h380; end
     3: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h21C; arA23[ 'h5] = 'h380; end
     4: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h103; arA23[ 'h5] = 'h380; end
     5: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h1C2; arA23[ 'h5] = 'h380; end
     6: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h1E3; arA23[ 'h5] = 'h380; end
     7: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h00A; arA23[ 'h5] = 'h380; end
     8: begin arIll[ 'h5] = 1'b0; arA1[ 'h5] = 'h1E2; arA23[ 'h5] = 'h380; end
    default: begin arIll[ 'h5] = 1'b1; arA1[ 'h5] = 'X   ; arA23[ 'h5] = 'X; end
    endcase
endcase

//
// Line: 8
//
unique case( row86)

3'b000: // Row: 0
    unique case ( col)
     0: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1C1; arA23[ 'h8] = 'X; end
     1: begin arIll[ 'h8] = 1'b1; arA1[ 'h8] = 'X   ; arA23[ 'h8] = 'X; end
     2: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h006; arA23[ 'h8] = 'h1C3; end
     3: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h21C; arA23[ 'h8] = 'h1C3; end
     4: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h103; arA23[ 'h8] = 'h1C3; end
     5: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1C2; arA23[ 'h8] = 'h1C3; end
     6: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1E3; arA23[ 'h8] = 'h1C3; end
     7: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h00A; arA23[ 'h8] = 'h1C3; end
     8: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1E2; arA23[ 'h8] = 'h1C3; end
     9: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1C2; arA23[ 'h8] = 'h1C3; end
    10: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1E3; arA23[ 'h8] = 'h1C3; end
    11: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h0EA; arA23[ 'h8] = 'h1C1; end
    default: begin arIll[ 'h8] = 1'b1; arA1[ 'h8] = 'X   ; arA23[ 'h8] = 'X; end
    endcase

3'b001: // Row: 1
    unique case ( col)
     0: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1C1; arA23[ 'h8] = 'X; end
     1: begin arIll[ 'h8] = 1'b1; arA1[ 'h8] = 'X   ; arA23[ 'h8] = 'X; end
     2: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h006; arA23[ 'h8] = 'h1C3; end
     3: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h21C; arA23[ 'h8] = 'h1C3; end
     4: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h103; arA23[ 'h8] = 'h1C3; end
     5: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1C2; arA23[ 'h8] = 'h1C3; end
     6: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1E3; arA23[ 'h8] = 'h1C3; end
     7: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h00A; arA23[ 'h8] = 'h1C3; end
     8: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1E2; arA23[ 'h8] = 'h1C3; end
     9: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1C2; arA23[ 'h8] = 'h1C3; end
    10: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1E3; arA23[ 'h8] = 'h1C3; end
    11: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h0EA; arA23[ 'h8] = 'h1C1; end
    default: begin arIll[ 'h8] = 1'b1; arA1[ 'h8] = 'X   ; arA23[ 'h8] = 'X; end
    endcase

3'b010: // Row: 2
    unique case ( col)
     0: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1C5; arA23[ 'h8] = 'X; end
     1: begin arIll[ 'h8] = 1'b1; arA1[ 'h8] = 'X   ; arA23[ 'h8] = 'X; end
     2: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h00B; arA23[ 'h8] = 'h1CB; end
     3: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h00F; arA23[ 'h8] = 'h1CB; end
     4: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h179; arA23[ 'h8] = 'h1CB; end
     5: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1C6; arA23[ 'h8] = 'h1CB; end
     6: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1E7; arA23[ 'h8] = 'h1CB; end
     7: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h00E; arA23[ 'h8] = 'h1CB; end
     8: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1E6; arA23[ 'h8] = 'h1CB; end
     9: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1C6; arA23[ 'h8] = 'h1CB; end
    10: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1E7; arA23[ 'h8] = 'h1CB; end
    11: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h0A7; arA23[ 'h8] = 'h1C5; end
    default: begin arIll[ 'h8] = 1'b1; arA1[ 'h8] = 'X   ; arA23[ 'h8] = 'X; end
    endcase

3'b011: // Row: 3
    unique case ( col)
     0: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h0A6; arA23[ 'h8] = 'X; end
     1: begin arIll[ 'h8] = 1'b1; arA1[ 'h8] = 'X   ; arA23[ 'h8] = 'X; end
     2: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h006; arA23[ 'h8] = 'h0A4; end
     3: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h21C; arA23[ 'h8] = 'h0A4; end
     4: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h103; arA23[ 'h8] = 'h0A4; end
     5: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1C2; arA23[ 'h8] = 'h0A4; end
     6: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1E3; arA23[ 'h8] = 'h0A4; end
     7: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h00A; arA23[ 'h8] = 'h0A4; end
     8: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1E2; arA23[ 'h8] = 'h0A4; end
     9: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1C2; arA23[ 'h8] = 'h0A4; end
    10: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1E3; arA23[ 'h8] = 'h0A4; end
    11: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h0EA; arA23[ 'h8] = 'h0A6; end
    default: begin arIll[ 'h8] = 1'b1; arA1[ 'h8] = 'X   ; arA23[ 'h8] = 'X; end
    endcase

3'b100: // Row: 4
    unique case ( col)
     0: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1CD; arA23[ 'h8] = 'X; end
     1: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h107; arA23[ 'h8] = 'X; end
     2: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h006; arA23[ 'h8] = 'h299; end
     3: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h21C; arA23[ 'h8] = 'h299; end
     4: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h103; arA23[ 'h8] = 'h299; end
     5: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1C2; arA23[ 'h8] = 'h299; end
     6: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1E3; arA23[ 'h8] = 'h299; end
     7: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h00A; arA23[ 'h8] = 'h299; end
     8: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1E2; arA23[ 'h8] = 'h299; end
     9: begin arIll[ 'h8] = 1'b1; arA1[ 'h8] = 'X   ; arA23[ 'h8] = 'X; end
    10: begin arIll[ 'h8] = 1'b1; arA1[ 'h8] = 'X   ; arA23[ 'h8] = 'X; end
    11: begin arIll[ 'h8] = 1'b1; arA1[ 'h8] = 'X   ; arA23[ 'h8] = 'X; end
    default: begin arIll[ 'h8] = 1'b1; arA1[ 'h8] = 'X   ; arA23[ 'h8] = 'X; end
    endcase

3'b101: // Row: 5
    unique case ( col)
     0: begin arIll[ 'h8] = 1'b1; arA1[ 'h8] = 'X   ; arA23[ 'h8] = 'X; end
     1: begin arIll[ 'h8] = 1'b1; arA1[ 'h8] = 'X   ; arA23[ 'h8] = 'X; end
     2: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h006; arA23[ 'h8] = 'h299; end
     3: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h21C; arA23[ 'h8] = 'h299; end
     4: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h103; arA23[ 'h8] = 'h299; end
     5: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1C2; arA23[ 'h8] = 'h299; end
     6: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1E3; arA23[ 'h8] = 'h299; end
     7: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h00A; arA23[ 'h8] = 'h299; end
     8: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1E2; arA23[ 'h8] = 'h299; end
     9: begin arIll[ 'h8] = 1'b1; arA1[ 'h8] = 'X   ; arA23[ 'h8] = 'X; end
    10: begin arIll[ 'h8] = 1'b1; arA1[ 'h8] = 'X   ; arA23[ 'h8] = 'X; end
    11: begin arIll[ 'h8] = 1'b1; arA1[ 'h8] = 'X   ; arA23[ 'h8] = 'X; end
    default: begin arIll[ 'h8] = 1'b1; arA1[ 'h8] = 'X   ; arA23[ 'h8] = 'X; end
    endcase

3'b110: // Row: 6
    unique case ( col)
     0: begin arIll[ 'h8] = 1'b1; arA1[ 'h8] = 'X   ; arA23[ 'h8] = 'X; end
     1: begin arIll[ 'h8] = 1'b1; arA1[ 'h8] = 'X   ; arA23[ 'h8] = 'X; end
     2: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h00B; arA23[ 'h8] = 'h29D; end
     3: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h00F; arA23[ 'h8] = 'h29D; end
     4: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h179; arA23[ 'h8] = 'h29D; end
     5: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1C6; arA23[ 'h8] = 'h29D; end
     6: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1E7; arA23[ 'h8] = 'h29D; end
     7: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h00E; arA23[ 'h8] = 'h29D; end
     8: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1E6; arA23[ 'h8] = 'h29D; end
     9: begin arIll[ 'h8] = 1'b1; arA1[ 'h8] = 'X   ; arA23[ 'h8] = 'X; end
    10: begin arIll[ 'h8] = 1'b1; arA1[ 'h8] = 'X   ; arA23[ 'h8] = 'X; end
    11: begin arIll[ 'h8] = 1'b1; arA1[ 'h8] = 'X   ; arA23[ 'h8] = 'X; end
    default: begin arIll[ 'h8] = 1'b1; arA1[ 'h8] = 'X   ; arA23[ 'h8] = 'X; end
    endcase

3'b111: // Row: 7
    unique case ( col)
     0: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h0AE; arA23[ 'h8] = 'X; end
     1: begin arIll[ 'h8] = 1'b1; arA1[ 'h8] = 'X   ; arA23[ 'h8] = 'X; end
     2: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h006; arA23[ 'h8] = 'h0AC; end
     3: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h21C; arA23[ 'h8] = 'h0AC; end
     4: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h103; arA23[ 'h8] = 'h0AC; end
     5: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1C2; arA23[ 'h8] = 'h0AC; end
     6: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1E3; arA23[ 'h8] = 'h0AC; end
     7: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h00A; arA23[ 'h8] = 'h0AC; end
     8: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1E2; arA23[ 'h8] = 'h0AC; end
     9: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1C2; arA23[ 'h8] = 'h0AC; end
    10: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h1E3; arA23[ 'h8] = 'h0AC; end
    11: begin arIll[ 'h8] = 1'b0; arA1[ 'h8] = 'h0EA; arA23[ 'h8] = 'h0AE; end
    default: begin arIll[ 'h8] = 1'b1; arA1[ 'h8] = 'X   ; arA23[ 'h8] = 'X; end
    endcase
endcase

//
// Line: 9
//
unique case( row86)

3'b000: // Row: 0
    unique case ( col)
     0: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1C1; arA23[ 'h9] = 'X; end
     1: begin arIll[ 'h9] = 1'b1; arA1[ 'h9] = 'X   ; arA23[ 'h9] = 'X; end
     2: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h006; arA23[ 'h9] = 'h1C3; end
     3: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h21C; arA23[ 'h9] = 'h1C3; end
     4: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h103; arA23[ 'h9] = 'h1C3; end
     5: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1C2; arA23[ 'h9] = 'h1C3; end
     6: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1E3; arA23[ 'h9] = 'h1C3; end
     7: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h00A; arA23[ 'h9] = 'h1C3; end
     8: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1E2; arA23[ 'h9] = 'h1C3; end
     9: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1C2; arA23[ 'h9] = 'h1C3; end
    10: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1E3; arA23[ 'h9] = 'h1C3; end
    11: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h0EA; arA23[ 'h9] = 'h1C1; end
    default: begin arIll[ 'h9] = 1'b1; arA1[ 'h9] = 'X   ; arA23[ 'h9] = 'X; end
    endcase

3'b001: // Row: 1
    unique case ( col)
     0: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1C1; arA23[ 'h9] = 'X; end
     1: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1C1; arA23[ 'h9] = 'X; end
     2: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h006; arA23[ 'h9] = 'h1C3; end
     3: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h21C; arA23[ 'h9] = 'h1C3; end
     4: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h103; arA23[ 'h9] = 'h1C3; end
     5: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1C2; arA23[ 'h9] = 'h1C3; end
     6: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1E3; arA23[ 'h9] = 'h1C3; end
     7: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h00A; arA23[ 'h9] = 'h1C3; end
     8: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1E2; arA23[ 'h9] = 'h1C3; end
     9: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1C2; arA23[ 'h9] = 'h1C3; end
    10: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1E3; arA23[ 'h9] = 'h1C3; end
    11: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h0EA; arA23[ 'h9] = 'h1C1; end
    default: begin arIll[ 'h9] = 1'b1; arA1[ 'h9] = 'X   ; arA23[ 'h9] = 'X; end
    endcase

3'b010: // Row: 2
    unique case ( col)
     0: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1C5; arA23[ 'h9] = 'X; end
     1: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1C5; arA23[ 'h9] = 'X; end
     2: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h00B; arA23[ 'h9] = 'h1CB; end
     3: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h00F; arA23[ 'h9] = 'h1CB; end
     4: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h179; arA23[ 'h9] = 'h1CB; end
     5: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1C6; arA23[ 'h9] = 'h1CB; end
     6: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1E7; arA23[ 'h9] = 'h1CB; end
     7: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h00E; arA23[ 'h9] = 'h1CB; end
     8: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1E6; arA23[ 'h9] = 'h1CB; end
     9: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1C6; arA23[ 'h9] = 'h1CB; end
    10: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1E7; arA23[ 'h9] = 'h1CB; end
    11: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h0A7; arA23[ 'h9] = 'h1C5; end
    default: begin arIll[ 'h9] = 1'b1; arA1[ 'h9] = 'X   ; arA23[ 'h9] = 'X; end
    endcase

3'b011: // Row: 3
    unique case ( col)
     0: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1C9; arA23[ 'h9] = 'X; end
     1: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1C9; arA23[ 'h9] = 'X; end
     2: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h006; arA23[ 'h9] = 'h1C7; end
     3: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h21C; arA23[ 'h9] = 'h1C7; end
     4: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h103; arA23[ 'h9] = 'h1C7; end
     5: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1C2; arA23[ 'h9] = 'h1C7; end
     6: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1E3; arA23[ 'h9] = 'h1C7; end
     7: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h00A; arA23[ 'h9] = 'h1C7; end
     8: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1E2; arA23[ 'h9] = 'h1C7; end
     9: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1C2; arA23[ 'h9] = 'h1C7; end
    10: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1E3; arA23[ 'h9] = 'h1C7; end
    11: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h0EA; arA23[ 'h9] = 'h1C9; end
    default: begin arIll[ 'h9] = 1'b1; arA1[ 'h9] = 'X   ; arA23[ 'h9] = 'X; end
    endcase

3'b100: // Row: 4
    unique case ( col)
     0: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1C1; arA23[ 'h9] = 'X; end
     1: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h10F; arA23[ 'h9] = 'X; end
     2: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h006; arA23[ 'h9] = 'h299; end
     3: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h21C; arA23[ 'h9] = 'h299; end
     4: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h103; arA23[ 'h9] = 'h299; end
     5: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1C2; arA23[ 'h9] = 'h299; end
     6: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1E3; arA23[ 'h9] = 'h299; end
     7: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h00A; arA23[ 'h9] = 'h299; end
     8: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1E2; arA23[ 'h9] = 'h299; end
     9: begin arIll[ 'h9] = 1'b1; arA1[ 'h9] = 'X   ; arA23[ 'h9] = 'X; end
    10: begin arIll[ 'h9] = 1'b1; arA1[ 'h9] = 'X   ; arA23[ 'h9] = 'X; end
    11: begin arIll[ 'h9] = 1'b1; arA1[ 'h9] = 'X   ; arA23[ 'h9] = 'X; end
    default: begin arIll[ 'h9] = 1'b1; arA1[ 'h9] = 'X   ; arA23[ 'h9] = 'X; end
    endcase

3'b101: // Row: 5
    unique case ( col)
     0: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1C1; arA23[ 'h9] = 'X; end
     1: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h10F; arA23[ 'h9] = 'X; end
     2: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h006; arA23[ 'h9] = 'h299; end
     3: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h21C; arA23[ 'h9] = 'h299; end
     4: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h103; arA23[ 'h9] = 'h299; end
     5: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1C2; arA23[ 'h9] = 'h299; end
     6: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1E3; arA23[ 'h9] = 'h299; end
     7: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h00A; arA23[ 'h9] = 'h299; end
     8: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1E2; arA23[ 'h9] = 'h299; end
     9: begin arIll[ 'h9] = 1'b1; arA1[ 'h9] = 'X   ; arA23[ 'h9] = 'X; end
    10: begin arIll[ 'h9] = 1'b1; arA1[ 'h9] = 'X   ; arA23[ 'h9] = 'X; end
    11: begin arIll[ 'h9] = 1'b1; arA1[ 'h9] = 'X   ; arA23[ 'h9] = 'X; end
    default: begin arIll[ 'h9] = 1'b1; arA1[ 'h9] = 'X   ; arA23[ 'h9] = 'X; end
    endcase

3'b110: // Row: 6
    unique case ( col)
     0: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1C5; arA23[ 'h9] = 'X; end
     1: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h10B; arA23[ 'h9] = 'X; end
     2: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h00B; arA23[ 'h9] = 'h29D; end
     3: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h00F; arA23[ 'h9] = 'h29D; end
     4: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h179; arA23[ 'h9] = 'h29D; end
     5: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1C6; arA23[ 'h9] = 'h29D; end
     6: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1E7; arA23[ 'h9] = 'h29D; end
     7: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h00E; arA23[ 'h9] = 'h29D; end
     8: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1E6; arA23[ 'h9] = 'h29D; end
     9: begin arIll[ 'h9] = 1'b1; arA1[ 'h9] = 'X   ; arA23[ 'h9] = 'X; end
    10: begin arIll[ 'h9] = 1'b1; arA1[ 'h9] = 'X   ; arA23[ 'h9] = 'X; end
    11: begin arIll[ 'h9] = 1'b1; arA1[ 'h9] = 'X   ; arA23[ 'h9] = 'X; end
    default: begin arIll[ 'h9] = 1'b1; arA1[ 'h9] = 'X   ; arA23[ 'h9] = 'X; end
    endcase

3'b111: // Row: 7
    unique case ( col)
     0: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1C5; arA23[ 'h9] = 'X; end
     1: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1C5; arA23[ 'h9] = 'X; end
     2: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h00B; arA23[ 'h9] = 'h1CB; end
     3: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h00F; arA23[ 'h9] = 'h1CB; end
     4: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h179; arA23[ 'h9] = 'h1CB; end
     5: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1C6; arA23[ 'h9] = 'h1CB; end
     6: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1E7; arA23[ 'h9] = 'h1CB; end
     7: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h00E; arA23[ 'h9] = 'h1CB; end
     8: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1E6; arA23[ 'h9] = 'h1CB; end
     9: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1C6; arA23[ 'h9] = 'h1CB; end
    10: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h1E7; arA23[ 'h9] = 'h1CB; end
    11: begin arIll[ 'h9] = 1'b0; arA1[ 'h9] = 'h0A7; arA23[ 'h9] = 'h1C5; end
    default: begin arIll[ 'h9] = 1'b1; arA1[ 'h9] = 'X   ; arA23[ 'h9] = 'X; end
    endcase
endcase

//
// Line: B
//
unique case( row86)

3'b000: // Row: 0
    unique case ( col)
     0: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1D1; arA23[ 'hb] = 'X; end
     1: begin arIll[ 'hb] = 1'b1; arA1[ 'hb] = 'X   ; arA23[ 'hb] = 'X; end
     2: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h006; arA23[ 'hb] = 'h1D3; end
     3: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h21C; arA23[ 'hb] = 'h1D3; end
     4: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h103; arA23[ 'hb] = 'h1D3; end
     5: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1C2; arA23[ 'hb] = 'h1D3; end
     6: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1E3; arA23[ 'hb] = 'h1D3; end
     7: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h00A; arA23[ 'hb] = 'h1D3; end
     8: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1E2; arA23[ 'hb] = 'h1D3; end
     9: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1C2; arA23[ 'hb] = 'h1D3; end
    10: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1E3; arA23[ 'hb] = 'h1D3; end
    11: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h0EA; arA23[ 'hb] = 'h1D1; end
    default: begin arIll[ 'hb] = 1'b1; arA1[ 'hb] = 'X   ; arA23[ 'hb] = 'X; end
    endcase

3'b001: // Row: 1
    unique case ( col)
     0: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1D1; arA23[ 'hb] = 'X; end
     1: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1D1; arA23[ 'hb] = 'X; end
     2: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h006; arA23[ 'hb] = 'h1D3; end
     3: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h21C; arA23[ 'hb] = 'h1D3; end
     4: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h103; arA23[ 'hb] = 'h1D3; end
     5: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1C2; arA23[ 'hb] = 'h1D3; end
     6: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1E3; arA23[ 'hb] = 'h1D3; end
     7: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h00A; arA23[ 'hb] = 'h1D3; end
     8: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1E2; arA23[ 'hb] = 'h1D3; end
     9: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1C2; arA23[ 'hb] = 'h1D3; end
    10: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1E3; arA23[ 'hb] = 'h1D3; end
    11: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h0EA; arA23[ 'hb] = 'h1D1; end
    default: begin arIll[ 'hb] = 1'b1; arA1[ 'hb] = 'X   ; arA23[ 'hb] = 'X; end
    endcase

3'b010: // Row: 2
    unique case ( col)
     0: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1D5; arA23[ 'hb] = 'X; end
     1: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1D5; arA23[ 'hb] = 'X; end
     2: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h00B; arA23[ 'hb] = 'h1D7; end
     3: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h00F; arA23[ 'hb] = 'h1D7; end
     4: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h179; arA23[ 'hb] = 'h1D7; end
     5: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1C6; arA23[ 'hb] = 'h1D7; end
     6: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1E7; arA23[ 'hb] = 'h1D7; end
     7: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h00E; arA23[ 'hb] = 'h1D7; end
     8: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1E6; arA23[ 'hb] = 'h1D7; end
     9: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1C6; arA23[ 'hb] = 'h1D7; end
    10: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1E7; arA23[ 'hb] = 'h1D7; end
    11: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h0A7; arA23[ 'hb] = 'h1D5; end
    default: begin arIll[ 'hb] = 1'b1; arA1[ 'hb] = 'X   ; arA23[ 'hb] = 'X; end
    endcase

3'b011: // Row: 3
    unique case ( col)
     0: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1D9; arA23[ 'hb] = 'X; end
     1: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1D9; arA23[ 'hb] = 'X; end
     2: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h006; arA23[ 'hb] = 'h1CF; end
     3: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h21C; arA23[ 'hb] = 'h1CF; end
     4: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h103; arA23[ 'hb] = 'h1CF; end
     5: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1C2; arA23[ 'hb] = 'h1CF; end
     6: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1E3; arA23[ 'hb] = 'h1CF; end
     7: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h00A; arA23[ 'hb] = 'h1CF; end
     8: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1E2; arA23[ 'hb] = 'h1CF; end
     9: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1C2; arA23[ 'hb] = 'h1CF; end
    10: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1E3; arA23[ 'hb] = 'h1CF; end
    11: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h0EA; arA23[ 'hb] = 'h1D9; end
    default: begin arIll[ 'hb] = 1'b1; arA1[ 'hb] = 'X   ; arA23[ 'hb] = 'X; end
    endcase

3'b100: // Row: 4
    unique case ( col)
     0: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h100; arA23[ 'hb] = 'X; end
     1: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h06B; arA23[ 'hb] = 'X; end
     2: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h006; arA23[ 'hb] = 'h299; end
     3: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h21C; arA23[ 'hb] = 'h299; end
     4: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h103; arA23[ 'hb] = 'h299; end
     5: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1C2; arA23[ 'hb] = 'h299; end
     6: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1E3; arA23[ 'hb] = 'h299; end
     7: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h00A; arA23[ 'hb] = 'h299; end
     8: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1E2; arA23[ 'hb] = 'h299; end
     9: begin arIll[ 'hb] = 1'b1; arA1[ 'hb] = 'X   ; arA23[ 'hb] = 'X; end
    10: begin arIll[ 'hb] = 1'b1; arA1[ 'hb] = 'X   ; arA23[ 'hb] = 'X; end
    11: begin arIll[ 'hb] = 1'b1; arA1[ 'hb] = 'X   ; arA23[ 'hb] = 'X; end
    default: begin arIll[ 'hb] = 1'b1; arA1[ 'hb] = 'X   ; arA23[ 'hb] = 'X; end
    endcase

3'b101: // Row: 5
    unique case ( col)
     0: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h100; arA23[ 'hb] = 'X; end
     1: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h06B; arA23[ 'hb] = 'X; end
     2: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h006; arA23[ 'hb] = 'h299; end
     3: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h21C; arA23[ 'hb] = 'h299; end
     4: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h103; arA23[ 'hb] = 'h299; end
     5: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1C2; arA23[ 'hb] = 'h299; end
     6: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1E3; arA23[ 'hb] = 'h299; end
     7: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h00A; arA23[ 'hb] = 'h299; end
     8: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1E2; arA23[ 'hb] = 'h299; end
     9: begin arIll[ 'hb] = 1'b1; arA1[ 'hb] = 'X   ; arA23[ 'hb] = 'X; end
    10: begin arIll[ 'hb] = 1'b1; arA1[ 'hb] = 'X   ; arA23[ 'hb] = 'X; end
    11: begin arIll[ 'hb] = 1'b1; arA1[ 'hb] = 'X   ; arA23[ 'hb] = 'X; end
    default: begin arIll[ 'hb] = 1'b1; arA1[ 'hb] = 'X   ; arA23[ 'hb] = 'X; end
    endcase

3'b110: // Row: 6
    unique case ( col)
     0: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h10C; arA23[ 'hb] = 'X; end
     1: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h06F; arA23[ 'hb] = 'X; end
     2: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h00B; arA23[ 'hb] = 'h29D; end
     3: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h00F; arA23[ 'hb] = 'h29D; end
     4: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h179; arA23[ 'hb] = 'h29D; end
     5: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1C6; arA23[ 'hb] = 'h29D; end
     6: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1E7; arA23[ 'hb] = 'h29D; end
     7: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h00E; arA23[ 'hb] = 'h29D; end
     8: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1E6; arA23[ 'hb] = 'h29D; end
     9: begin arIll[ 'hb] = 1'b1; arA1[ 'hb] = 'X   ; arA23[ 'hb] = 'X; end
    10: begin arIll[ 'hb] = 1'b1; arA1[ 'hb] = 'X   ; arA23[ 'hb] = 'X; end
    11: begin arIll[ 'hb] = 1'b1; arA1[ 'hb] = 'X   ; arA23[ 'hb] = 'X; end
    default: begin arIll[ 'hb] = 1'b1; arA1[ 'hb] = 'X   ; arA23[ 'hb] = 'X; end
    endcase

3'b111: // Row: 7
    unique case ( col)
     0: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1D5; arA23[ 'hb] = 'X; end
     1: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1D5; arA23[ 'hb] = 'X; end
     2: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h00B; arA23[ 'hb] = 'h1D7; end
     3: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h00F; arA23[ 'hb] = 'h1D7; end
     4: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h179; arA23[ 'hb] = 'h1D7; end
     5: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1C6; arA23[ 'hb] = 'h1D7; end
     6: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1E7; arA23[ 'hb] = 'h1D7; end
     7: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h00E; arA23[ 'hb] = 'h1D7; end
     8: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1E6; arA23[ 'hb] = 'h1D7; end
     9: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1C6; arA23[ 'hb] = 'h1D7; end
    10: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h1E7; arA23[ 'hb] = 'h1D7; end
    11: begin arIll[ 'hb] = 1'b0; arA1[ 'hb] = 'h0A7; arA23[ 'hb] = 'h1D5; end
    default: begin arIll[ 'hb] = 1'b1; arA1[ 'hb] = 'X   ; arA23[ 'hb] = 'X; end
    endcase
endcase

//
// Line: C
//
unique case( row86)

3'b000: // Row: 0
    unique case ( col)
     0: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1C1; arA23[ 'hc] = 'X; end
     1: begin arIll[ 'hc] = 1'b1; arA1[ 'hc] = 'X   ; arA23[ 'hc] = 'X; end
     2: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h006; arA23[ 'hc] = 'h1C3; end
     3: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h21C; arA23[ 'hc] = 'h1C3; end
     4: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h103; arA23[ 'hc] = 'h1C3; end
     5: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1C2; arA23[ 'hc] = 'h1C3; end
     6: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1E3; arA23[ 'hc] = 'h1C3; end
     7: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h00A; arA23[ 'hc] = 'h1C3; end
     8: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1E2; arA23[ 'hc] = 'h1C3; end
     9: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1C2; arA23[ 'hc] = 'h1C3; end
    10: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1E3; arA23[ 'hc] = 'h1C3; end
    11: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h0EA; arA23[ 'hc] = 'h1C1; end
    default: begin arIll[ 'hc] = 1'b1; arA1[ 'hc] = 'X   ; arA23[ 'hc] = 'X; end
    endcase

3'b001: // Row: 1
    unique case ( col)
     0: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1C1; arA23[ 'hc] = 'X; end
     1: begin arIll[ 'hc] = 1'b1; arA1[ 'hc] = 'X   ; arA23[ 'hc] = 'X; end
     2: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h006; arA23[ 'hc] = 'h1C3; end
     3: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h21C; arA23[ 'hc] = 'h1C3; end
     4: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h103; arA23[ 'hc] = 'h1C3; end
     5: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1C2; arA23[ 'hc] = 'h1C3; end
     6: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1E3; arA23[ 'hc] = 'h1C3; end
     7: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h00A; arA23[ 'hc] = 'h1C3; end
     8: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1E2; arA23[ 'hc] = 'h1C3; end
     9: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1C2; arA23[ 'hc] = 'h1C3; end
    10: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1E3; arA23[ 'hc] = 'h1C3; end
    11: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h0EA; arA23[ 'hc] = 'h1C1; end
    default: begin arIll[ 'hc] = 1'b1; arA1[ 'hc] = 'X   ; arA23[ 'hc] = 'X; end
    endcase

3'b010: // Row: 2
    unique case ( col)
     0: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1C5; arA23[ 'hc] = 'X; end
     1: begin arIll[ 'hc] = 1'b1; arA1[ 'hc] = 'X   ; arA23[ 'hc] = 'X; end
     2: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h00B; arA23[ 'hc] = 'h1CB; end
     3: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h00F; arA23[ 'hc] = 'h1CB; end
     4: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h179; arA23[ 'hc] = 'h1CB; end
     5: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1C6; arA23[ 'hc] = 'h1CB; end
     6: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1E7; arA23[ 'hc] = 'h1CB; end
     7: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h00E; arA23[ 'hc] = 'h1CB; end
     8: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1E6; arA23[ 'hc] = 'h1CB; end
     9: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1C6; arA23[ 'hc] = 'h1CB; end
    10: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1E7; arA23[ 'hc] = 'h1CB; end
    11: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h0A7; arA23[ 'hc] = 'h1C5; end
    default: begin arIll[ 'hc] = 1'b1; arA1[ 'hc] = 'X   ; arA23[ 'hc] = 'X; end
    endcase

3'b011: // Row: 3
    unique case ( col)
     0: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h15B; arA23[ 'hc] = 'X; end
     1: begin arIll[ 'hc] = 1'b1; arA1[ 'hc] = 'X   ; arA23[ 'hc] = 'X; end
     2: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h006; arA23[ 'hc] = 'h15A; end
     3: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h21C; arA23[ 'hc] = 'h15A; end
     4: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h103; arA23[ 'hc] = 'h15A; end
     5: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1C2; arA23[ 'hc] = 'h15A; end
     6: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1E3; arA23[ 'hc] = 'h15A; end
     7: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h00A; arA23[ 'hc] = 'h15A; end
     8: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1E2; arA23[ 'hc] = 'h15A; end
     9: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1C2; arA23[ 'hc] = 'h15A; end
    10: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1E3; arA23[ 'hc] = 'h15A; end
    11: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h0EA; arA23[ 'hc] = 'h15B; end
    default: begin arIll[ 'hc] = 1'b1; arA1[ 'hc] = 'X   ; arA23[ 'hc] = 'X; end
    endcase

3'b100: // Row: 4
    unique case ( col)
     0: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1CD; arA23[ 'hc] = 'X; end
     1: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h107; arA23[ 'hc] = 'X; end
     2: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h006; arA23[ 'hc] = 'h299; end
     3: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h21C; arA23[ 'hc] = 'h299; end
     4: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h103; arA23[ 'hc] = 'h299; end
     5: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1C2; arA23[ 'hc] = 'h299; end
     6: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1E3; arA23[ 'hc] = 'h299; end
     7: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h00A; arA23[ 'hc] = 'h299; end
     8: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1E2; arA23[ 'hc] = 'h299; end
     9: begin arIll[ 'hc] = 1'b1; arA1[ 'hc] = 'X   ; arA23[ 'hc] = 'X; end
    10: begin arIll[ 'hc] = 1'b1; arA1[ 'hc] = 'X   ; arA23[ 'hc] = 'X; end
    11: begin arIll[ 'hc] = 1'b1; arA1[ 'hc] = 'X   ; arA23[ 'hc] = 'X; end
    default: begin arIll[ 'hc] = 1'b1; arA1[ 'hc] = 'X   ; arA23[ 'hc] = 'X; end
    endcase

3'b101: // Row: 5
    unique case ( col)
     0: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h3E3; arA23[ 'hc] = 'X; end
     1: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h3E3; arA23[ 'hc] = 'X; end
     2: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h006; arA23[ 'hc] = 'h299; end
     3: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h21C; arA23[ 'hc] = 'h299; end
     4: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h103; arA23[ 'hc] = 'h299; end
     5: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1C2; arA23[ 'hc] = 'h299; end
     6: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1E3; arA23[ 'hc] = 'h299; end
     7: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h00A; arA23[ 'hc] = 'h299; end
     8: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1E2; arA23[ 'hc] = 'h299; end
     9: begin arIll[ 'hc] = 1'b1; arA1[ 'hc] = 'X   ; arA23[ 'hc] = 'X; end
    10: begin arIll[ 'hc] = 1'b1; arA1[ 'hc] = 'X   ; arA23[ 'hc] = 'X; end
    11: begin arIll[ 'hc] = 1'b1; arA1[ 'hc] = 'X   ; arA23[ 'hc] = 'X; end
    default: begin arIll[ 'hc] = 1'b1; arA1[ 'hc] = 'X   ; arA23[ 'hc] = 'X; end
    endcase

3'b110: // Row: 6
    unique case ( col)
     0: begin arIll[ 'hc] = 1'b1; arA1[ 'hc] = 'X   ; arA23[ 'hc] = 'X; end
     1: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h3E3; arA23[ 'hc] = 'X; end
     2: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h00B; arA23[ 'hc] = 'h29D; end
     3: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h00F; arA23[ 'hc] = 'h29D; end
     4: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h179; arA23[ 'hc] = 'h29D; end
     5: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1C6; arA23[ 'hc] = 'h29D; end
     6: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1E7; arA23[ 'hc] = 'h29D; end
     7: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h00E; arA23[ 'hc] = 'h29D; end
     8: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1E6; arA23[ 'hc] = 'h29D; end
     9: begin arIll[ 'hc] = 1'b1; arA1[ 'hc] = 'X   ; arA23[ 'hc] = 'X; end
    10: begin arIll[ 'hc] = 1'b1; arA1[ 'hc] = 'X   ; arA23[ 'hc] = 'X; end
    11: begin arIll[ 'hc] = 1'b1; arA1[ 'hc] = 'X   ; arA23[ 'hc] = 'X; end
    default: begin arIll[ 'hc] = 1'b1; arA1[ 'hc] = 'X   ; arA23[ 'hc] = 'X; end
    endcase

3'b111: // Row: 7
    unique case ( col)
     0: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h15B; arA23[ 'hc] = 'X; end
     1: begin arIll[ 'hc] = 1'b1; arA1[ 'hc] = 'X   ; arA23[ 'hc] = 'X; end
     2: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h006; arA23[ 'hc] = 'h15A; end
     3: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h21C; arA23[ 'hc] = 'h15A; end
     4: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h103; arA23[ 'hc] = 'h15A; end
     5: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1C2; arA23[ 'hc] = 'h15A; end
     6: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1E3; arA23[ 'hc] = 'h15A; end
     7: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h00A; arA23[ 'hc] = 'h15A; end
     8: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1E2; arA23[ 'hc] = 'h15A; end
     9: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1C2; arA23[ 'hc] = 'h15A; end
    10: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h1E3; arA23[ 'hc] = 'h15A; end
    11: begin arIll[ 'hc] = 1'b0; arA1[ 'hc] = 'h0EA; arA23[ 'hc] = 'h15B; end
    default: begin arIll[ 'hc] = 1'b1; arA1[ 'hc] = 'X   ; arA23[ 'hc] = 'X; end
    endcase
endcase

//
// Line: D
//
unique case( row86)

3'b000: // Row: 0
    unique case ( col)
     0: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1C1; arA23[ 'hd] = 'X; end
     1: begin arIll[ 'hd] = 1'b1; arA1[ 'hd] = 'X   ; arA23[ 'hd] = 'X; end
     2: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h006; arA23[ 'hd] = 'h1C3; end
     3: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h21C; arA23[ 'hd] = 'h1C3; end
     4: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h103; arA23[ 'hd] = 'h1C3; end
     5: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1C2; arA23[ 'hd] = 'h1C3; end
     6: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1E3; arA23[ 'hd] = 'h1C3; end
     7: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h00A; arA23[ 'hd] = 'h1C3; end
     8: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1E2; arA23[ 'hd] = 'h1C3; end
     9: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1C2; arA23[ 'hd] = 'h1C3; end
    10: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1E3; arA23[ 'hd] = 'h1C3; end
    11: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h0EA; arA23[ 'hd] = 'h1C1; end
    default: begin arIll[ 'hd] = 1'b1; arA1[ 'hd] = 'X   ; arA23[ 'hd] = 'X; end
    endcase

3'b001: // Row: 1
    unique case ( col)
     0: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1C1; arA23[ 'hd] = 'X; end
     1: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1C1; arA23[ 'hd] = 'X; end
     2: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h006; arA23[ 'hd] = 'h1C3; end
     3: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h21C; arA23[ 'hd] = 'h1C3; end
     4: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h103; arA23[ 'hd] = 'h1C3; end
     5: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1C2; arA23[ 'hd] = 'h1C3; end
     6: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1E3; arA23[ 'hd] = 'h1C3; end
     7: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h00A; arA23[ 'hd] = 'h1C3; end
     8: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1E2; arA23[ 'hd] = 'h1C3; end
     9: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1C2; arA23[ 'hd] = 'h1C3; end
    10: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1E3; arA23[ 'hd] = 'h1C3; end
    11: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h0EA; arA23[ 'hd] = 'h1C1; end
    default: begin arIll[ 'hd] = 1'b1; arA1[ 'hd] = 'X   ; arA23[ 'hd] = 'X; end
    endcase

3'b010: // Row: 2
    unique case ( col)
     0: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1C5; arA23[ 'hd] = 'X; end
     1: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1C5; arA23[ 'hd] = 'X; end
     2: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h00B; arA23[ 'hd] = 'h1CB; end
     3: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h00F; arA23[ 'hd] = 'h1CB; end
     4: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h179; arA23[ 'hd] = 'h1CB; end
     5: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1C6; arA23[ 'hd] = 'h1CB; end
     6: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1E7; arA23[ 'hd] = 'h1CB; end
     7: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h00E; arA23[ 'hd] = 'h1CB; end
     8: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1E6; arA23[ 'hd] = 'h1CB; end
     9: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1C6; arA23[ 'hd] = 'h1CB; end
    10: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1E7; arA23[ 'hd] = 'h1CB; end
    11: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h0A7; arA23[ 'hd] = 'h1C5; end
    default: begin arIll[ 'hd] = 1'b1; arA1[ 'hd] = 'X   ; arA23[ 'hd] = 'X; end
    endcase

3'b011: // Row: 3
    unique case ( col)
     0: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1C9; arA23[ 'hd] = 'X; end
     1: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1C9; arA23[ 'hd] = 'X; end
     2: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h006; arA23[ 'hd] = 'h1C7; end
     3: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h21C; arA23[ 'hd] = 'h1C7; end
     4: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h103; arA23[ 'hd] = 'h1C7; end
     5: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1C2; arA23[ 'hd] = 'h1C7; end
     6: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1E3; arA23[ 'hd] = 'h1C7; end
     7: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h00A; arA23[ 'hd] = 'h1C7; end
     8: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1E2; arA23[ 'hd] = 'h1C7; end
     9: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1C2; arA23[ 'hd] = 'h1C7; end
    10: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1E3; arA23[ 'hd] = 'h1C7; end
    11: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h0EA; arA23[ 'hd] = 'h1C9; end
    default: begin arIll[ 'hd] = 1'b1; arA1[ 'hd] = 'X   ; arA23[ 'hd] = 'X; end
    endcase

3'b100: // Row: 4
    unique case ( col)
     0: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1C1; arA23[ 'hd] = 'X; end
     1: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h10F; arA23[ 'hd] = 'X; end
     2: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h006; arA23[ 'hd] = 'h299; end
     3: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h21C; arA23[ 'hd] = 'h299; end
     4: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h103; arA23[ 'hd] = 'h299; end
     5: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1C2; arA23[ 'hd] = 'h299; end
     6: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1E3; arA23[ 'hd] = 'h299; end
     7: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h00A; arA23[ 'hd] = 'h299; end
     8: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1E2; arA23[ 'hd] = 'h299; end
     9: begin arIll[ 'hd] = 1'b1; arA1[ 'hd] = 'X   ; arA23[ 'hd] = 'X; end
    10: begin arIll[ 'hd] = 1'b1; arA1[ 'hd] = 'X   ; arA23[ 'hd] = 'X; end
    11: begin arIll[ 'hd] = 1'b1; arA1[ 'hd] = 'X   ; arA23[ 'hd] = 'X; end
    default: begin arIll[ 'hd] = 1'b1; arA1[ 'hd] = 'X   ; arA23[ 'hd] = 'X; end
    endcase

3'b101: // Row: 5
    unique case ( col)
     0: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1C1; arA23[ 'hd] = 'X; end
     1: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h10F; arA23[ 'hd] = 'X; end
     2: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h006; arA23[ 'hd] = 'h299; end
     3: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h21C; arA23[ 'hd] = 'h299; end
     4: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h103; arA23[ 'hd] = 'h299; end
     5: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1C2; arA23[ 'hd] = 'h299; end
     6: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1E3; arA23[ 'hd] = 'h299; end
     7: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h00A; arA23[ 'hd] = 'h299; end
     8: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1E2; arA23[ 'hd] = 'h299; end
     9: begin arIll[ 'hd] = 1'b1; arA1[ 'hd] = 'X   ; arA23[ 'hd] = 'X; end
    10: begin arIll[ 'hd] = 1'b1; arA1[ 'hd] = 'X   ; arA23[ 'hd] = 'X; end
    11: begin arIll[ 'hd] = 1'b1; arA1[ 'hd] = 'X   ; arA23[ 'hd] = 'X; end
    default: begin arIll[ 'hd] = 1'b1; arA1[ 'hd] = 'X   ; arA23[ 'hd] = 'X; end
    endcase

3'b110: // Row: 6
    unique case ( col)
     0: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1C5; arA23[ 'hd] = 'X; end
     1: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h10B; arA23[ 'hd] = 'X; end
     2: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h00B; arA23[ 'hd] = 'h29D; end
     3: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h00F; arA23[ 'hd] = 'h29D; end
     4: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h179; arA23[ 'hd] = 'h29D; end
     5: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1C6; arA23[ 'hd] = 'h29D; end
     6: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1E7; arA23[ 'hd] = 'h29D; end
     7: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h00E; arA23[ 'hd] = 'h29D; end
     8: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1E6; arA23[ 'hd] = 'h29D; end
     9: begin arIll[ 'hd] = 1'b1; arA1[ 'hd] = 'X   ; arA23[ 'hd] = 'X; end
    10: begin arIll[ 'hd] = 1'b1; arA1[ 'hd] = 'X   ; arA23[ 'hd] = 'X; end
    11: begin arIll[ 'hd] = 1'b1; arA1[ 'hd] = 'X   ; arA23[ 'hd] = 'X; end
    default: begin arIll[ 'hd] = 1'b1; arA1[ 'hd] = 'X   ; arA23[ 'hd] = 'X; end
    endcase

3'b111: // Row: 7
    unique case ( col)
     0: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1C5; arA23[ 'hd] = 'X; end
     1: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1C5; arA23[ 'hd] = 'X; end
     2: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h00B; arA23[ 'hd] = 'h1CB; end
     3: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h00F; arA23[ 'hd] = 'h1CB; end
     4: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h179; arA23[ 'hd] = 'h1CB; end
     5: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1C6; arA23[ 'hd] = 'h1CB; end
     6: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1E7; arA23[ 'hd] = 'h1CB; end
     7: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h00E; arA23[ 'hd] = 'h1CB; end
     8: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1E6; arA23[ 'hd] = 'h1CB; end
     9: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1C6; arA23[ 'hd] = 'h1CB; end
    10: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h1E7; arA23[ 'hd] = 'h1CB; end
    11: begin arIll[ 'hd] = 1'b0; arA1[ 'hd] = 'h0A7; arA23[ 'hd] = 'h1C5; end
    default: begin arIll[ 'hd] = 1'b1; arA1[ 'hd] = 'X   ; arA23[ 'hd] = 'X; end
    endcase
endcase
end


endmodule
