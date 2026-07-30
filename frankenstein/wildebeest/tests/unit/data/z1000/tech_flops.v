//tech_flops.v
//Peter Grossmann
//19 May 2022
//$Id$
//$Log$

//This file provides a generic tech mapping option suitable for many
//(not all) FPGA architectures that define their flop flop primitives
//with names that match the naming conventions used here (i.e.
//architectures generated with the FPGA Architect standard library).

//Part of the trick to making this library work for you is matching it
//to your synthesis flow.  The library contains every flip flop type
//that yosys defines, so if your architecture doesn't support all of them
//you need to constrain your synthesis flow to only map the design to
//the subset of flop types that your architecture supports.  This can be
//done with the dfflegalize command, which is integrated into FPGA
//Architect's reference synthesis flow.

//Where appropriate, a few hooks are provided in the library to help
//lessen the effort on downstream development in adopting this setup.
//Read available code comments and documentation carefully to make sure
//your setup works with this library in the way you wish.  If it does not
//meet your needs, creating a custom flop tech library for your architecture
//may be required.

//Excluding latches and scan flops, Yosys supports a total of 54 different 
//flop permutations It is useful to group them into categories to help organize
//common cases for what subsets an architecture might choose to support.

//There are eight flops that are fundamental to FPGA Architect generator
//options.  If you only support these eight flops, you can synthesize
//a very wide range of designs without adding additional configuration bits
//and flip flop modes.  Early versions of FPGA Architect supported only
//these eight flops.  They are presented here as a group.  The remaining
//46 flop variants supported by Yosys are all variants of these eight
//flops.

//*****************************************
//
// Begin "Core 8" Flop types
//
//*****************************************

(* techmap_celltype = "$_FF_ $_DFF_P_" *)
module tech_dff 
  (
   C,
   D,
   Q
   );

   input C;
   input D;
   output Q;

   dff _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .Q(Q) );
      
endmodule // tech_dff
			
(* techmap_celltype = "$_DFF_PN0_" *)
module tech_dffr 
  (
   C,
   D,
   R,
   Q
   );

   input C;
   input R;
   input D;
   output Q;

   dffr _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .R(R), .Q(Q) );

endmodule // tech_dffr

(* techmap_celltype = "$_DFF_PN1_" *)
module tech_dffs 
  (
   C,
   D,
   R,
   Q
   );

   input C;
   input R;
   input D;
   output Q;

   dffs _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .S(R), .Q(Q) );
      
endmodule // tech_dffs
			
(* techmap_celltype = "$_DFFSR_PNN_" *)
module tech_dffrs 
  (
   C,
   D,
   R,
   S,
   Q
   );

   input C;
   input R;
   input S;
   input D;
   output Q;

   dffrs _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .R(R), .S(S), .Q(Q) );
      
endmodule // tech_dffrs

(* techmap_celltype = "$_DFFE_PP_" *)
module tech_dffe 
  (
   C,
   D,
   E,
   Q
   );

   input C;
   input D;
   input E;
   output Q;

   dffe _TECHMAP_REPLACE_ 
     ( .clk(C), .E(E), .D(D), .Q(Q) );
      
endmodule // tech_dffe
			
(* techmap_celltype = "$_DFFE_PN0P_" *)
module tech_dffer 
  (
   C,
   D,
   E,
   R,
   Q
   );

   input C;
   input R;
   input D;
   input E;
   output Q;

   dffer _TECHMAP_REPLACE_ 
     ( .clk(C), .E(E), .D(D), .R(R), .Q(Q) );

endmodule // tech_dffer

(* techmap_celltype = "$_DFFE_PN1P_" *)
module tech_dffes 
  (
   C,
   D,
   E,
   R,
   Q
   );

   input C;
   input R;
   input D;
   input E;
   output Q;

   dffes _TECHMAP_REPLACE_ 
     ( .clk(C), .E(E), .D(D), .S(R), .Q(Q) );
      
endmodule // tech_dffes

(* techmap_celltype = "$_DFFSRE_PNNP_" *)
module tech_dffers 
  (
   C,
   D,
   E,
   R,
   S,
   Q
   );

   input C;
   input R;
   input S;
   input D;
   input E;
   output Q;

   dffers _TECHMAP_REPLACE_ 
     ( .clk(C), .E(E), .D(D), .R(R), .S(S), .Q(Q) );
      
endmodule // tech_dffers

//*****************************************
//
// End "Core 8" Flop types
//
//*****************************************

//A next logical expansion to build additional architecture support on top
//of these "Core 8" flops would be to offer variants of them that trigger
//on the falling edge of the clock.  This is relatively inexpensive to 
//implement, as you need only a single configuration bit and an inverter
//to mux in a clock inversion to support this type.  The negative edge
//clock versions of the Core 8 are provided here next:
			
//*****************************************
//
// Begin "Negative Core 8" Flop types
//
//*****************************************

(* techmap_celltype = "$_DFF_N_" *)
module tech_dffn
  (
   C,
   D,
   Q
   );

   input C;
   input D;
   output Q;

   dffn _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .Q(Q) );
      
endmodule // tech_dffn
			
(* techmap_celltype = "$_DFF_NN0_" *)
module tech_dffnr
  (
   C,
   D,
   R,
   Q
   );

   input C;
   input R;
   input D;
   output Q;

   dffnr _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .R(R), .Q(Q) );

endmodule // tech_dffnr

(* techmap_celltype = "$_DFF_NN1_" *)
module tech_dffns 
  (
   C,
   D,
   R,
   Q
   );

   input C;
   input R;
   input D;
   output Q;

   dffns _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .S(R), .Q(Q) );
      
endmodule // tech_dffns
			
(* techmap_celltype = "$_DFFSR_NNN_" *)
module tech_dffnrs 
  (
   C,
   D,
   R,
   S,
   Q
   );

   input C;
   input R;
   input S;
   input D;
   output Q;

   dffnrs _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .R(R), .S(S), .Q(Q) );
      
endmodule // tech_dffnrs

(* techmap_celltype = "$_DFFE_NP_" *)
module tech_dffen 
  (
   C,
   D,
   E,
   Q
   );

   input C;
   input D;
   input E;
   output Q;

   dffen _TECHMAP_REPLACE_ 
     ( .clk(C), .E(E), .D(D), .Q(Q) );
      
endmodule // tech_dffen
			
(* techmap_celltype = "$_DFFE_NN0P_" *)
module tech_dffenr 
  (
   C,
   D,
   E,
   R,
   Q
   );

   input C;
   input R;
   input D;
   input E;
   output Q;

   dffenr _TECHMAP_REPLACE_ 
     ( .clk(C), .E(E), .D(D), .R(R), .Q(Q) );

endmodule // tech_dffenr

(* techmap_celltype = "$_DFFE_NN1P_" *)
module tech_dffens 
  (
   C,
   D,
   E,
   R,
   Q
   );

   input C;
   input R;
   input D;
   input E;
   output Q;

   dffens _TECHMAP_REPLACE_ 
     ( .clk(C), .E(E), .D(D), .S(R), .Q(Q) );
      
endmodule // tech_dffens
	
(* techmap_celltype = "$_DFFSRE_NNNP_" *)
module tech_dffenrs 
  (
   C,
   D,
   E,
   R,
   S,
   Q
   );

   input C;
   input R;
   input S;
   input D;
   input E;
   output Q;

   dffenrs _TECHMAP_REPLACE_ 
     ( .clk(C), .E(E), .D(D), .R(R), .S(S), .Q(Q) );
      
endmodule // tech_dffenrs

//*****************************************
//
// End "Negative Core 8" Flop types
//
//*****************************************

//An similar additional type of support is to allow either
//edge of the set/reset/enable signals to be active.  Support
//of these types is where the number of flop permutations really
//explodes, and where having a naming convention that is easy
//to parse is critical.  Above, we have sacrificed a bit of
//clarity in exchange for brevity of flop names, so that when
//they are the only types used the flop types are compact while
//still easy to understand.  When permuting the set/reset/enable
//polarities, this breaks down.  We'll deal with this by simply
//mimicing Yosys's convention as a suffix stacked on top of our
//Core 8/Negative Core 8 naming conventions.  Do this as follows:

//Define a three-letter suffix in which the meaning of the suffix
//is <reset polarity><set polarity><enable polarity>
//and each polarity is denoted as follows:
//
// p - positive edge / active high polarity
// n - negative edge / active low polarity
// x - signal not present
//
//In this convention, our Core 8 would have the following
//expanded names:
//
// dff    | dff_xxx
// dffr   | dffr_nxx
// dffs   | dffs_xnx
// dffrs  | dffrs_nnx
// dffe   | dffe_xxp
// dffer  | dffer_nxp
// dffes  | dffes_xnp
// dffers | dffers_nnp

//As you can see, this convention introduces a bit of field redundancy,
//in exchange for the preservation of the original Core 8 names

//Like the core 8, the negative edge clock versions simply insert an 'n'
//in the apprpropriate location prior to the underscore.

//First, cover the positive edge versions of flops with just a set OR a
//reset (one new version edge)

(* techmap_celltype = "$_DFF_PP0_" *)
module tech_dffr_pxx
  (
   C,
   D,
   R,
   Q
   );

   input C;
   input R;
   input D;
   output Q;

   dffr_pxx _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .R(R), .Q(Q) );

endmodule // tech_dffr_pxx
      
(* techmap_celltype = "$_DFF_PP1_" *)
module tech_dffs_xpx
  (
   C,
   D,
   R,
   Q
   );

   input C;
   input R;
   input D;
   output Q;

   dffs_xpx _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .S(R), .Q(Q) );
      
endmodule // tech_dffs_xpx

//Next, cover the positive edge versions of flops with a set AND a
//reset (three versions)

(* techmap_celltype = "$_DFFSR_PPN_" *)
module tech_dffrs_pnx
  (
   C,
   D,
   R,
   S,
   Q
   );

   input C;
   input R;
   input S;
   input D;
   output Q;

   dffrs_pnx _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .R(R), .S(S), .Q(Q) );
      
endmodule // tech_dffrs_pnx

(* techmap_celltype = "$_DFFSR_PNP_" *)
module tech_dffrs_npx
  (
   C,
   D,
   R,
   S,
   Q
   );

   input C;
   input R;
   input S;
   input D;
   output Q;

   dffrs_npx _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .R(R), .S(S), .Q(Q) );
      
endmodule // tech_dffrs_npx

(* techmap_celltype = "$_DFFSR_PPP_" *)
module tech_dffrs_ppx
  (
   C,
   D,
   R,
   S,
   Q
   );

   input C;
   input R;
   input S;
   input D;
   output Q;

   dffrs_ppx _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .R(R), .S(S), .Q(Q) );
      
endmodule // tech_dffrs_ppx

//Do this all over again for flops with active high enables

(* techmap_celltype = "$_DFFE_PP0P_" *)
module tech_dffer_pxp
  (
   C,
   D,
   E,
   R,
   Q
   );

   input C;
   input R;
   input D;
   input E;
   output Q;

   dffer_pxp _TECHMAP_REPLACE_ 
     ( .clk(C), .E(E), .D(D), .R(R), .Q(Q) );

endmodule // tech_dffer_pxp
      
(* techmap_celltype = "$_DFFE_PP1P_" *)
module tech_dffes_xpp
  (
   C,
   D,
   E,
   R,
   Q
   );

   input C;
   input R;
   input D;
   input E;
   output Q;

   dffes_xpp _TECHMAP_REPLACE_ 
     ( .clk(C), .E(E), .D(D), .S(R), .Q(Q) );
      
endmodule // tech_dffes_xpp

(* techmap_celltype = "$_DFFSRE_PPNP_" *)
module tech_dffers_pnp
  (
   C,
   D,
   E,
   R,
   S,
   Q
   );

   input C;
   input R;
   input S;
   input D;
   input E;
   output Q;

   dffers_pnp _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .R(R), .S(S), .Q(Q) );
      
endmodule // tech_dffers_pnp

(* techmap_celltype = "$_DFFSRE_PNPP_" *)
module tech_dffers_npp
  (
   C,
   D,
   E,
   R,
   S,
   Q
   );

   input C;
   input R;
   input S;
   input D;
   input E;
   output Q;

   dffers_npp _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .R(R), .S(S), .Q(Q) );
      
endmodule // tech_dffers_npp

(* techmap_celltype = "$_DFFSRE_PPPP_" *)
module tech_dffers_ppp
  (
   C,
   D,
   E,
   R,
   S,
   Q
   );

   input C;
   input R;
   input S;
   input D;
   input E;
   output Q;

   dffers_ppp _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .R(R), .S(S), .Q(Q) );
      
endmodule // tech_dffers_ppp

//Make versions of Core 8 flops with active low enables

(* techmap_celltype = "$_DFFE_PN_" *)
module tech_dffe_xxn
  (
   C,
   D,
   E,
   Q
   );

   input C;
   input D;
   input E;
   output Q;

   dffe_xxn _TECHMAP_REPLACE_ 
     ( .clk(C), .E(E), .D(D), .Q(Q) );
      
endmodule // tech_dffe
			
(* techmap_celltype = "$_DFFE_PN0N_" *)
module tech_dffer_nxn
  (
   C,
   D,
   E,
   R,
   Q
   );

   input C;
   input R;
   input D;
   input E;
   output Q;

   dffer_nxn _TECHMAP_REPLACE_ 
     ( .clk(C), .E(E), .D(D), .R(R), .Q(Q) );

endmodule // tech_dffer

(* techmap_celltype = "$_DFFE_PN1N_" *)
module tech_dffes_xnn
  (
   C,
   D,
   E,
   R,
   Q
   );

   input C;
   input R;
   input D;
   input E;
   output Q;

   dffes_xnn _TECHMAP_REPLACE_ 
     ( .clk(C), .E(E), .D(D), .S(R), .Q(Q) );
      
endmodule // tech_dffes

(* techmap_celltype = "$_DFFSRE_PNNN_" *)
module tech_dffers_nnn
  (
   C,
   D,
   E,
   R,
   S,
   Q
   );

   input C;
   input R;
   input S;
   input D;
   input E;
   output Q;

   dffers_nnn _TECHMAP_REPLACE_ 
     ( .clk(C), .E(E), .D(D), .R(R), .S(S), .Q(Q) );
      
endmodule // tech_dffers

//Now do all the non-core flops with active low enables

(* techmap_celltype = "$_DFFE_PP0N_" *)
module tech_dffer_pxn
  (
   C,
   D,
   E,
   R,
   Q
   );

   input C;
   input R;
   input D;
   input E;
   output Q;

   dffer_pxn _TECHMAP_REPLACE_ 
     ( .clk(C), .E(E), .D(D), .R(R), .Q(Q) );

endmodule // tech_dffer_pxn
      
(* techmap_celltype = "$_DFFE_PP1N_" *)
module tech_dffes_xpn
  (
   C,
   D,
   E,
   R,
   Q
   );

   input C;
   input R;
   input D;
   input E;
   output Q;

   dffes_xpn _TECHMAP_REPLACE_ 
     ( .clk(C), .E(E), .D(D), .S(R), .Q(Q) );
      
endmodule // tech_dffes_xpn

(* techmap_celltype = "$_DFFSRE_PPNN_" *)
module tech_dffers_pnn
  (
   C,
   D,
   E,
   R,
   S,
   Q
   );

   input C;
   input R;
   input S;
   input D;
   input E;
   output Q;

   dffers_pnn _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .R(R), .S(S), .Q(Q) );
      
endmodule // tech_dffers_pnn

(* techmap_celltype = "$_DFFSRE_PNPN_" *)
module tech_dffers_npn
  (
   C,
   D,
   E,
   R,
   S,
   Q
   );

   input C;
   input R;
   input S;
   input D;
   input E;
   output Q;

   dffers_npn _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .R(R), .S(S), .Q(Q) );
      
endmodule // tech_dffers_npn

(* techmap_celltype = "$_DFFSRE_PPPN_" *)
module tech_dffers_ppn
  (
   C,
   D,
   E,
   R,
   S,
   Q
   );

   input C;
   input R;
   input S;
   input D;
   input E;
   output Q;

   dffers_ppn _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .R(R), .S(S), .Q(Q) );
      
endmodule // tech_dffers_ppn

//Now repeat ALL the non-core flops we just made with negative-edge
//triggered clocks

//First, cover the positive edge versions of flops with just a set OR a
//reset (one new version edge)

(* techmap_celltype = "$_DFF_NP0_" *)
module tech_dffnr_pxx
  (
   C,
   D,
   R,
   Q
   );

   input C;
   input R;
   input D;
   output Q;

   dffnr_pxx _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .R(R), .Q(Q) );

endmodule // tech_dffnr_pxx
      
(* techmap_celltype = "$_DFF_NP1_" *)
module tech_dffns_xpx
  (
   C,
   D,
   R,
   Q
   );

   input C;
   input R;
   input D;
   output Q;

   dffns_xpx _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .S(R), .Q(Q) );
      
endmodule // tech_dffns_xpx

//Next, cover the positive edge versions of flops with a set AND a
//reset (three versions)

(* techmap_celltype = "$_DFFSR_NPN_" *)
module tech_dffnrs_pnx
  (
   C,
   D,
   R,
   S,
   Q
   );

   input C;
   input R;
   input S;
   input D;
   output Q;

   dffnrs_pnx _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .R(R), .S(S), .Q(Q) );
      
endmodule // tech_dffnrs_pnx

(* techmap_celltype = "$_DFFSR_NNP_" *)
module tech_dffnrs_npx
  (
   C,
   D,
   R,
   S,
   Q
   );

   input C;
   input R;
   input S;
   input D;
   output Q;

   dffnrs_npx _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .R(R), .S(S), .Q(Q) );
      
endmodule // tech_dffnrs_npx

(* techmap_celltype = "$_DFFSR_NPP_" *)
module tech_dffnrs_ppx
  (
   C,
   D,
   R,
   S,
   Q
   );

   input C;
   input R;
   input S;
   input D;
   output Q;

   dffnrs_ppx _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .R(R), .S(S), .Q(Q) );
      
endmodule // tech_dffnrs_ppx

//Do this all over again for flops with active high enables

(* techmap_celltype = "$_DFFE_NP0P_" *)
module tech_dffenr_pxp
  (
   C,
   D,
   E,
   R,
   Q
   );

   input C;
   input R;
   input D;
   input E;
   output Q;

   dffenr_pxp _TECHMAP_REPLACE_ 
     ( .clk(C), .E(E), .D(D), .R(R), .Q(Q) );

endmodule // tech_dffenr_pxp
      
(* techmap_celltype = "$_DFFE_NP1P_" *)
module tech_dffens_xpp
  (
   C,
   D,
   E,
   R,
   Q
   );

   input C;
   input R;
   input D;
   input E;
   output Q;

   dffens_xpp _TECHMAP_REPLACE_ 
     ( .clk(C), .E(E), .D(D), .S(R), .Q(Q) );
      
endmodule // tech_dffens_xpp

(* techmap_celltype = "$_DFFSRE_NPNP_" *)
module tech_dffenrs_pnp
  (
   C,
   D,
   E,
   R,
   S,
   Q
   );

   input C;
   input R;
   input S;
   input D;
   input E;
   output Q;

   dffenrs_pnp _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .R(R), .S(S), .Q(Q) );
      
endmodule // tech_dffenrs_pnp

(* techmap_celltype = "$_DFFSRE_NNPP_" *)
module tech_dffenrs_npp
  (
   C,
   D,
   E,
   R,
   S,
   Q
   );

   input C;
   input R;
   input S;
   input D;
   input E;
   output Q;

   dffenrs_npp _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .R(R), .S(S), .Q(Q) );
      
endmodule // tech_dffenrs_npp

(* techmap_celltype = "$_DFFSRE_NPPP_" *)
module tech_dffenrs_ppp
  (
   C,
   D,
   E,
   R,
   S,
   Q
   );

   input C;
   input R;
   input S;
   input D;
   input E;
   output Q;

   dffenrs_ppp _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .R(R), .S(S), .Q(Q) );
      
endmodule // tech_dffenrs_ppp

//Make versions of Core 8 flops with active low enables

(* techmap_celltype = "$_DFFE_NN_" *)
module tech_dffen_xxn
  (
   C,
   D,
   E,
   Q
   );

   input C;
   input D;
   input E;
   output Q;

   dffen_xxn _TECHMAP_REPLACE_ 
     ( .clk(C), .E(E), .D(D), .Q(Q) );
      
endmodule // tech_dffen
			
(* techmap_celltype = "$_DFFE_NN0N_" *)
module tech_dffenr_nxn
  (
   C,
   D,
   E,
   R,
   Q
   );

   input C;
   input R;
   input D;
   input E;
   output Q;

   dffenr_nxn _TECHMAP_REPLACE_ 
     ( .clk(C), .E(E), .D(D), .R(R), .Q(Q) );

endmodule // tech_dffenr

(* techmap_celltype = "$_DFFE_NN1N_" *)
module tech_dffens_xnn
  (
   C,
   D,
   E,
   R,
   Q
   );

   input C;
   input R;
   input D;
   input E;
   output Q;

   dffens_xnn _TECHMAP_REPLACE_ 
     ( .clk(C), .E(E), .D(D), .S(R), .Q(Q) );
      
endmodule // tech_dffens

(* techmap_celltype = "$_DFFSRE_NNNN_" *)
module tech_dffenrs_nnn
  (
   C,
   D,
   E,
   R,
   S,
   Q
   );

   input C;
   input R;
   input S;
   input D;
   input E;
   output Q;

   dffenrs_nnn _TECHMAP_REPLACE_ 
     ( .clk(C), .E(E), .D(D), .R(R), .S(S), .Q(Q) );
      
endmodule // tech_dffenrs

//Now do all the non-core flops with active low enables

(* techmap_celltype = "$_DFFE_NP0N_" *)
module tech_dffenr_pxn
  (
   C,
   D,
   E,
   R,
   Q
   );

   input C;
   input R;
   input D;
   input E;
   output Q;

   dffenr_pxn _TECHMAP_REPLACE_ 
     ( .clk(C), .E(E), .D(D), .R(R), .Q(Q) );

endmodule // tech_dffenr_pxn
      
(* techmap_celltype = "$_DFFE_NP1N_" *)
module tech_dffens_xpn
  (
   C,
   D,
   E,
   R,
   Q
   );

   input C;
   input R;
   input D;
   input E;
   output Q;

   dffens_xpn _TECHMAP_REPLACE_ 
     ( .clk(C), .E(E), .D(D), .S(R), .Q(Q) );
      
endmodule // tech_dffens_xpn

(* techmap_celltype = "$_DFFSRE_NPNN_" *)
module tech_dffenrs_pnn
  (
   C,
   D,
   E,
   R,
   S,
   Q
   );

   input C;
   input R;
   input S;
   input D;
   input E;
   output Q;

   dffenrs_pnn _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .R(R), .S(S), .Q(Q) );
      
endmodule // tech_dffenrs_pnn

(* techmap_celltype = "$_DFFSRE_NNPN_" *)
module tech_dffenrs_npn
  (
   C,
   D,
   E,
   R,
   S,
   Q
   );

   input C;
   input R;
   input S;
   input D;
   input E;
   output Q;

   dffenrs_npn _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .R(R), .S(S), .Q(Q) );
      
endmodule // tech_dffenrs_npn

(* techmap_celltype = "$_DFFSRE_NPPN_" *)
module tech_dffenrs_ppn
  (
   C,
   D,
   E,
   R,
   S,
   Q
   );

   input C;
   input R;
   input S;
   input D;
   input E;
   output Q;

   dffenrs_ppn _TECHMAP_REPLACE_ 
     ( .clk(C), .D(D), .R(R), .S(S), .Q(Q) );
      
endmodule // tech_dffenrs_ppn
