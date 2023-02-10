/*
 * Copyright 2022 CAS—Atlantic (University of New Brunswick, CASA)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "odin_types.h"

const char *edge_type_e_STR[] = {
  "UNDEFINED_SENSITIVITY",   "FALLING_EDGE_SENSITIVITY", "RISING_EDGE_SENSITIVITY",
  "ACTIVE_HIGH_SENSITIVITY", "ACTIVE_LOW_SENSITIVITY",   "ASYNCHRONOUS_SENSITIVITY",
};

const char *_ZERO_GND_ZERO[] = {"ZERO_GND_ZERO", "ZGZ"};

const char *_ONE_VCC_CNS[] = {
  "ONE_VCC_CNS",
  "OVC",
};

const char *_ZERO_PAD_ZERO[] = {"ZERO_PAD_ZERO", "ZPZ"};

const char *ZERO_GND_ZERO = _ZERO_GND_ZERO[ODIN_STRING_TYPE];
const char *ONE_VCC_CNS = _ONE_VCC_CNS[ODIN_STRING_TYPE];
const char *ZERO_PAD_ZERO = _ZERO_PAD_ZERO[ODIN_STRING_TYPE];

const char *SINGLE_PORT_RAM_string = "single_port_ram";
const char *DUAL_PORT_RAM_string = "dual_port_ram";
const char *LUTRAM_string = "lutram_ram";

const char *operation_list_STR[][2] = {
  {"NO_OP", "nOP"},
  {"FF_NODE", "FF"},
  {"BUF_NODE", "BUF"},
  {"MULTI_PORT_MUX", "nMUX"}, // port 1 = control, port 2+ = mux options
  {"INPUT_NODE", "IN"},
  {"OUTPUT_NODE", "OUT"},
  {"GND_NODE", "GND"},
  {"VCC_NODE", "VCC"},
  {"CLOCK_NODE", "CLK"},
  {"ADD", "ADD"},             // +
  {"MINUS", "MIN"},           // -
  {"BITWISE_NOT", "bNOT"},    // ~
  {"BITWISE_AND", "bAND"},    // &
  {"BITWISE_OR", "bOR"},      // |
  {"BITWISE_NAND", "bNAND"},  // ~&
  {"BITWISE_NOR", "bNOR"},    // ~|
  {"BITWISE_XNOR", "bXNOR"},  // ~^
  {"BITWISE_XOR", "bXOR"},    // ^
  {"LOGICAL_NOT", "lNOT"},    // !
  {"LOGICAL_OR", "lOR"},      // ||
  {"LOGICAL_AND", "lAND"},    // &&
  {"LOGICAL_NAND", "lNAND"},  // No Symbol
  {"LOGICAL_NOR", "lNOR"},    // No Symbol
  {"LOGICAL_XNOR", "lXNOR"},  // No symbol
  {"LOGICAL_XOR", "lXOR"},    // No Symbol
  {"MULTIPLY", "MUL"},        // *
  {"DIVIDE", "DIV"},          // /
  {"MODULO", "MOD"},          // %
  {"POWER", "POW"},           // **
  {"LT", "LT"},               // <
  {"GT", "GT"},               // >
  {"LOGICAL_EQUAL", "lEQ"},   // ==
  {"NOT_EQUAL", "lNEQ"},      // !=
  {"LTE", "LTE"},             // <=
  {"GTE", "GTE"},             // >=
  {"SR", "SR"},               // >>
  {"ASR", "ASR"},             // >>>
  {"SL", "SL"},               // <<
  {"ASL", "ASL"},             // <<<
  {"CASE_EQUAL", "cEQ"},      // ===
  {"CASE_NOT_EQUAL", "cNEQ"}, // !==
  {"ADDER_FUNC", "ADDER"},
  {"CARRY_FUNC", "CARRY"},
  {"MUX_2", "MUX_2"},
  {"BLIF_FUNCTION", "BLIFf"},
  {"NETLIST_FUNCTION", "NETf"},
  {"SMUX_2", "SMUX_2"}, // MUX_2 with single bit selector (no need to add not selector as the second pin)
  {"MEMORY", "MEM"},
  {"PAD_NODE", "PAD"},
  {"HARD_IP", "HARD"},
  {"GENERIC", "GEN"},   /*added for the unknown node type */
  {"CLOG2", "CL2"},     // $clog2
  {"UNSIGNED", "UNSG"}, // $unsigned
  {"SIGNED", "SG"},     // $signed
  // [START] operations to cover yosys subckt
  {"MULTI_BIT_MUX_2", "nbMUX"},      // like MUX_2 but with n-bit input/output
  {"MULTIPORT_nBIT_SMUX", "npbMUX"}, // n-bit input/output in multi port mux
  {"PMUX", "pMUX"},                  // Multiplexer with many inputs using one-hot select signal
  {"SDFF", "sDFF"},                  // data, S to reset value and output port
  {"DFFE", "DFFe"},                  // data, enable to output port
  {"SDFFE", "sDFFe"},                // data, synchronous reset value and enable to output port
  {"SDFFCE", "sDFFce"},              // data, synchronous reset value and enable to reset value and output port
  {"DFFSR", "DFFsr"},                // data, clear and set to output port
  {"DFFSRE", "DFFsre"},              // data, clear and set with enable to output port
  {"DLATCH", "Dlatch"},              // datato output port based on polarity without clk
  {"ADLATCH", "aDlatch"},            // datato output port based on polarity without clk
  {"SETCLR", "setclr"},              // set or clear an input pins
  {"SPRAM", "spRAM"},                // representing primitive single port ram
  {"DPRAM", "dpRAM"},                // representing primitive dual port ram
  {"YMEM", "yRAM"},                  // representing primitive dual port ram
  {"YMEM2", "yRAM"},                 // representing primitive dual port ram
  {"ROM", "ROM"},
  {"BRAM", "bRAM"},    // block of memry generated in yosys subcircuit formet blif file
                       // [END] operations to cover yosys subckt
  {"ERROR OOB", "OOB"} // should not reach this
};

// EDDIE: new enum value for ids to replace MEMORY from operation_t
/* supported input/output file extensions */
strmap<file_type_e> file_type_strmap({{"ilang", file_type_e::_ILANG},
                                      {"verilog", file_type_e::_VERILOG},
                                      {"verilog_header", file_type_e::_VERILOG_HEADER},
                                      {"blif", file_type_e::_BLIF},
                                      {"eblif", file_type_e::_EBLIF},
                                      {"undef", file_type_e::_UNDEFINED}});

/**
 * global hashmap of yosys subckt types
 * Technically, Yosys only outputs the following hard blocks
 * as (.subckt) record in its output BLIF file
 *
 *  FIRST_ELEMENT: Yosys model names showing in a blif file
 *  SECOND_ELEMENT: corresponding Odin-II cell type
 *
 * NOTE: to add support for a new type, first you would find a
 * corresponding Odin-II cell type or create a new one matching
 * with the new type corresponding model (.subckt) in BLIF file,
 * and add it to the following typemap. Then, you would need to
 * specify the model input-output order in the Odin-II BLIF Reader.
 * At the end, a resolve_XXX_node function needs to be implemented
 * in the BLIF Elaboration phase to make the new node compatible
 * with the Odin-II partial mapper.
 */
strmap<operation_list> yosys_subckt_strmap({
  {"$_ANDNOT_", SKIP},
  {"$_AND_", SKIP},          // (A, B, Y)
  {"$_AOI3_", SKIP},         // (A, B, C, Y)
  {"$_AOI4_", SKIP},         // (A, B, C, Y)
  {"$_BUF_", SKIP},          // (A, Y)
  {"$_DFFE_NN0N_", SKIP},    // (D, C, R, E, Q)
  {"$_DFFE_NN0P_", SKIP},    // (D, C, R, E, Q)
  {"$_DFFE_NN1N_", SKIP},    // (D, C, R, E, Q)
  {"$_DFFE_NN1P_", SKIP},    // (D, C, R, E, Q)
  {"$_DFFE_NN_", SKIP},      // (D, C, E, Q)
  {"$_DFFE_NP0N_", SKIP},    // (D, C, R, E, Q)
  {"$_DFFE_NP0P_", SKIP},    // (D, C, R, E, Q)
  {"$_DFFE_NP1N_", SKIP},    // (D, C, R, E, Q)
  {"$_DFFE_NP1P_", SKIP},    // (D, C, R, E, Q)
  {"$_DFFE_NP_", SKIP},      // (D, C, E, Q)
  {"$_DFFE_PN0N_", SKIP},    // (D, C, R, E, Q)
  {"$_DFFE_PN0P_", SKIP},    // (D, C, R, E, Q)
  {"$_DFFE_PN1N_", SKIP},    // (D, C, R, E, Q)
  {"$_DFFE_PN1P_", SKIP},    // (D, C, R, E, Q)
  {"$_DFFE_PN_", SKIP},      // (D, C, E, Q)
  {"$_DFFE_PP0N_", SKIP},    // (D, C, R, E, Q)
  {"$_DFFE_PP0P_", SKIP},    // (D, C, R, E, Q)
  {"$_DFFE_PP1N_", SKIP},    // (D, C, R, E, Q)
  {"$_DFFE_PP1P_", SKIP},    // (D, C, R, E, Q)
  {"$_DFFE_PP_", SKIP},      // (D, C, E, Q)
  {"$_DFFSRE_NNNN_", SKIP},  // (C, S, R, E, D, Q)
  {"$_DFFSRE_NNNP_", SKIP},  // (C, S, R, E, D, Q)
  {"$_DFFSRE_NNPN_", SKIP},  // (C, S, R, E, D, Q)
  {"$_DFFSRE_NNPP_", SKIP},  // (C, S, R, E, D, Q)
  {"$_DFFSRE_NPNN_", SKIP},  // (C, S, R, E, D, Q)
  {"$_DFFSRE_NPNP_", SKIP},  // (C, S, R, E, D, Q)
  {"$_DFFSRE_NPPN_", SKIP},  // (C, S, R, E, D, Q)
  {"$_DFFSRE_NPPP_", SKIP},  // (C, S, R, E, D, Q)
  {"$_DFFSRE_PNNN_", SKIP},  // (C, S, R, E, D, Q)
  {"$_DFFSRE_PNNP_", SKIP},  // (C, S, R, E, D, Q)
  {"$_DFFSRE_PNPN_", SKIP},  // (C, S, R, E, D, Q)
  {"$_DFFSRE_PNPP_", SKIP},  // (C, S, R, E, D, Q)
  {"$_DFFSRE_PPNN_", SKIP},  // (C, S, R, E, D, Q)
  {"$_DFFSRE_PPNP_", SKIP},  // (C, S, R, E, D, Q)
  {"$_DFFSRE_PPPN_", SKIP},  // (C, S, R, E, D, Q)
  {"$_DFFSRE_PPPP_", SKIP},  // (C, S, R, E, D, Q)
  {"$_DFFSR_NNN_", SKIP},    // (C, S, R, D, Q)
  {"$_DFFSR_NNP_", SKIP},    // (C, S, R, D, Q)
  {"$_DFFSR_NPN_", SKIP},    // (C, S, R, D, Q)
  {"$_DFFSR_NPP_", SKIP},    // (C, S, R, D, Q)
  {"$_DFFSR_PNN_", SKIP},    // (C, S, R, D, Q)
  {"$_DFFSR_PNP_", SKIP},    // (C, S, R, D, Q)
  {"$_DFFSR_PPN_", SKIP},    // (C, S, R, D, Q)
  {"$_DFFSR_PPP_", SKIP},    // (C, S, R, D, Q)
  {"$_DFF_NN0_", SKIP},      // (D, C, R, Q)
  {"$_DFF_NN1_", SKIP},      // (D, C, R, Q)
  {"$_DFF_NP0_", SKIP},      // (D, C, R, Q)
  {"$_DFF_NP1_", SKIP},      // (D, C, R, Q)
  {"$_DFF_N_", SKIP},        // (D, C, Q)
  {"$_DFF_PN0_", SKIP},      // (D, C, R, Q)
  {"$_DFF_PN1_", SKIP},      // (D, C, R, Q)
  {"$_DFF_PP0_", SKIP},      // (D, C, R, Q)
  {"$_DFF_PP1_", SKIP},      // (D, C, R, Q)
  {"$_DFF_P_", SKIP},        // (D, C, Q)
  {"$_DLATCHSR_NNN_", SKIP}, // (E, S, R, D, Q)
  {"$_DLATCHSR_NNP_", SKIP}, // (E, S, R, D, Q)
  {"$_DLATCHSR_NPN_", SKIP}, // (E, S, R, D, Q)
  {"$_DLATCHSR_NPP_", SKIP}, // (E, S, R, D, Q)
  {"$_DLATCHSR_PNN_", SKIP}, // (E, S, R, D, Q)
  {"$_DLATCHSR_PNP_", SKIP}, // (E, S, R, D, Q)
  {"$_DLATCHSR_PPN_", SKIP}, // (E, S, R, D, Q)
  {"$_DLATCHSR_PPP_", SKIP}, // (E, S, R, D, Q)
  {"$_DLATCH_NN0_", SKIP},   // (E, R, D, Q)
  {"$_DLATCH_NN1_", SKIP},   // (E, R, D, Q)
  {"$_DLATCH_NP0_", SKIP},   // (E, R, D, Q)
  {"$_DLATCH_NP1_", SKIP},   // (E, R, D, Q)
  {"$_DLATCH_N_", SKIP},     // (E, D, Q)
  {"$_DLATCH_PN0_", SKIP},   // (E, R, D, Q)
  {"$_DLATCH_PN1_", SKIP},   // (E, R, D, Q)
  {"$_DLATCH_PP0_", SKIP},   // (E, R, D, Q)
  {"$_DLATCH_PP1_", SKIP},   // (E, R, D, Q)
  {"$_DLATCH_P_", SKIP},     // (E, D, Q)
  {"$_FF_", SKIP},           // (D, Q)
  {"$_MUX16_", SKIP},        // (A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, S, T, U, V, Y)
  {"$_MUX4_", SKIP},         // (A, B, C, D, S, T, Y)
  {"$_MUX8_", SKIP},         // (A, B, C, D, E, F, G, H, S, T, U, Y)
  {"$_MUX_", SKIP},          // (A, B, S, Y)
  {"$_NAND_", SKIP},         // (A, B, Y)
  {"$_NMUX_", SKIP},         // (A, B, S, Y)
  {"$_NOR_", SKIP},          // (A, B, Y)
  {"$_NOT_", SKIP},          // (A, Y)
  {"$_OAI3_", SKIP},         // (A, B, C, Y)
  {"$_OAI4_", SKIP},         // (A, B, C, Y)
  {"$_ORNOT_", SKIP},        // (A, B, Y)
  {"$_OR_", SKIP},           // (A, B, Y)
  {"$_SDFFCE_NN0N_", SKIP},  // (D, C, R, E, Q)
  {"$_SDFFCE_NN0P_", SKIP},  // (D, C, R, E, Q)
  {"$_SDFFCE_NN1N_", SKIP},  // (D, C, R, E, Q)
  {"$_SDFFCE_NN1P_", SKIP},  // (D, C, R, E, Q)
  {"$_SDFFCE_NP0N_", SKIP},  // (D, C, R, E, Q)
  {"$_SDFFCE_NP0P_", SKIP},  // (D, C, R, E, Q)
  {"$_SDFFCE_NP1N_", SKIP},  // (D, C, R, E, Q)
  {"$_SDFFCE_NP1P_", SKIP},  // (D, C, R, E, Q)
  {"$_SDFFCE_PN0N_", SKIP},  // (D, C, R, E, Q)
  {"$_SDFFCE_PN0P_", SKIP},  // (D, C, R, E, Q)
  {"$_SDFFCE_PN1N_", SKIP},  // (D, C, R, E, Q)
  {"$_SDFFCE_PN1P_", SKIP},  // (D, C, R, E, Q)
  {"$_SDFFCE_PP0N_", SKIP},  // (D, C, R, E, Q)
  {"$_SDFFCE_PP0P_", SKIP},  // (D, C, R, E, Q)
  {"$_SDFFCE_PP1N_", SKIP},  // (D, C, R, E, Q)
  {"$_SDFFCE_PP1P_", SKIP},  // (D, C, R, E, Q)
  {"$_SDFFE_NN0N_", SKIP},   // (D, C, R, E, Q)
  {"$_SDFFE_NN0P_", SKIP},   // (D, C, R, E, Q)
  {"$_SDFFE_NN1N_", SKIP},   // (D, C, R, E, Q)
  {"$_SDFFE_NN1P_", SKIP},   // (D, C, R, E, Q)
  {"$_SDFFE_NP0N_", SKIP},   // (D, C, R, E, Q)
  {"$_SDFFE_NP0P_", SKIP},   // (D, C, R, E, Q)
  {"$_SDFFE_NP1N_", SKIP},   // (D, C, R, E, Q)
  {"$_SDFFE_NP1P_", SKIP},   // (D, C, R, E, Q)
  {"$_SDFFE_PN0N_", SKIP},   // (D, C, R, E, Q)
  {"$_SDFFE_PN0P_", SKIP},   // (D, C, R, E, Q)
  {"$_SDFFE_PN1N_", SKIP},   // (D, C, R, E, Q)
  {"$_SDFFE_PN1P_", SKIP},   // (D, C, R, E, Q)
  {"$_SDFFE_PP0N_", SKIP},   // (D, C, R, E, Q)
  {"$_SDFFE_PP0P_", SKIP},   // (D, C, R, E, Q)
  {"$_SDFFE_PP1N_", SKIP},   // (D, C, R, E, Q)
  {"$_SDFFE_PP1P_", SKIP},   // (D, C, R, E, Q)
  {"$_SDFF_NN0_", SKIP},     // (D, C, R, Q)
  {"$_SDFF_NN1_", SKIP},     // (D, C, R, Q)
  {"$_SDFF_NP0_", SKIP},     // (D, C, R, Q)
  {"$_SDFF_NP1_", SKIP},     // (D, C, R, Q)
  {"$_SDFF_PN0_", SKIP},     // (D, C, R, Q)
  {"$_SDFF_PN1_", SKIP},     // (D, C, R, Q)
  {"$_SDFF_PP0_", SKIP},     // (D, C, R, Q)
  {"$_SDFF_PP1_", SKIP},     // (D, C, R, Q)
  {"$_SR_NN_", SKIP},        // (S, R, Q)
  {"$_SR_NP_", SKIP},        // (S, R, Q)
  {"$_SR_PN_", SKIP},        // (S, R, Q)
  {"$_SR_PP_", SKIP},        // (S, R, Q)
  {"$_TBUF_", SKIP},         // (A, E, Y)
  {"$_XNOR_", SKIP},         // (A, B, Y)
  {"$_XOR_", SKIP},          // (A, B, Y)
  {"$add", ADD},             // (A, B, Y)
  {"$adff", SKIP},           // (CLK, ARST, D, Q)
  {"$adffe", SKIP},          // (CLK, ARST, EN, D, Q)
  {"$adlatch", SKIP},        // (EN, ARST, D, Q)
  {"$allconst", SKIP},       // (Y)
  {"$allseq", SKIP},         // (Y)
  {"$alu", SKIP},            // (A, B, CI, BI, X, Y, CO)
  {"$and", SKIP},            // (A, B, Y)
  {"$anyconst", SKIP},       // (Y)
  {"$anyseq", SKIP},         // (Y)
  {"$assert", SKIP},         // (A, EN)
  {"$assume", SKIP},         // (A, EN)
  {"$concat", SKIP},         // (A, B, Y)
  {"$cover", SKIP},          // (A, EN)
  {"$dff", SKIP},            // (CLK, D, Q)
  {"$dffe", SKIP},           // (CLK, EN, D, Q)
  {"$dffsr", SKIP},          // (CLK, SET, CLR, D, Q)
  {"$dffsre", SKIP},         // (CLK, SET, CLR, EN, D, Q)
  {"$div", SKIP},            // (A, B, Y)
  {"$divfloor", SKIP},       // (A, B, Y)
  {"$dlatch", SKIP},         // (EN, D, Q)
  {"$dlatchsr", SKIP},       // (EN, SET, CLR, D, Q)
  {"$eq", SKIP},             // (A, B, Y)
  {"$equiv", SKIP},          // (A, B, Y)
  {"$eqx", SKIP},            // (A, B, Y)
  {"$fa", SKIP},             // (A, B, C, X, Y)
  {"$fair", SKIP},           // (A, EN)
  {"$ff", SKIP},             // (D, Q)
  {"$fsm", SKIP},            // (CLK, ARST, CTRL_IN, CTRL_OUT)
  {"$ge", SKIP},             // (A, B, Y)
  {"$gt", SKIP},             // (A, B, Y)
  {"$initstate", SKIP},      // (Y)
  {"$lcu", SKIP},            // (P, G, CI, CO)
  {"$le", SKIP},             // (A, B, Y)
  {"$live", SKIP},           // (A, EN)
  {"$logic_and", SKIP},      // (A, B, Y)
  {"$logic_not", SKIP},      // (A, Y)
  {"$logic_or", SKIP},       // (A, B, Y)
  {"$lt", SKIP},             // (A, B, Y)
  {"$lut", SKIP},            // (A, Y)
  {"$mem", YMEM},
  {"$mem_v2", YMEM2},
  {"$macc", SKIP},        // (A, B, Y)
  {"$meminit", SKIP},     // (ADDR, DATA)
  {"$memrd", SKIP},       // (CLK, EN, ADDR, DATA)
  {"$memwr", SKIP},       // (CLK, EN, ADDR, DATA)
  {"$mod", SKIP},         // (A, B, Y)
  {"$modfloor", SKIP},    // (A, B, Y)
  {"$mul", MULTIPLY},     // (A, B, Y)
  {"$mux", SKIP},         // (A, B, S, Y)
  {"$ne", SKIP},          // (A, B, Y)
  {"$neg", SKIP},         // (A, Y)
  {"$nex", SKIP},         // (A, B, Y)
  {"$not", SKIP},         // (A, Y)
  {"$or", SKIP},          // (A, B, Y)
  {"$pmux", SKIP},        // (A, B, S, Y)
  {"$pos", SKIP},         // (A, Y)
  {"$pow", SKIP},         // (A, B, Y)
  {"$reduce_and", SKIP},  // (A, Y)
  {"$reduce_bool", SKIP}, // (A, Y)
  {"$reduce_or", SKIP},   // (A, Y)
  {"$reduce_xnor", SKIP}, // (A, Y)
  {"$reduce_xor", SKIP},  // (A, Y)
  {"$sdff", SKIP},        // (CLK, SRST, D, Q)
  {"$sdffce", SKIP},      // (CLK, SRST, EN, D, Q)
  {"$sdffe", SKIP},       // (CLK, SRST, EN, D, Q)
  {"$shift", SKIP},       // (A, B, Y)
  {"$shiftx", SKIP},      // (A, B, Y)
  {"$shl", SKIP},         // (A, B, Y)
  {"$shr", SKIP},         // (A, B, Y)
  {"$slice", SKIP},       // (A, Y)
  {"$sop", SKIP},         // (A, Y)
  {"$specify2", SKIP},    // (EN, SRC, DST)
  {"$specify3", SKIP},    // (EN, SRC, DST, DAT)
  {"$specrule", SKIP},    // (EN_SRC, EN_DST, SRC, DST)
  {"$sr", SKIP},          // (SET, CLR, Q)
  {"$sshl", SKIP},        // (A, B, Y)
  {"$sshr", SKIP},        // (A, B, Y)
  {"$sub", MINUS},        // (A, B, Y)
  {"$tribuf", SKIP},      // (A, EN, Y)
  {"$xnor", SKIP},        // (A, B, Y)

  /*********** VTR Primitive modules START ***********/
  {"$xor", SKIP},                            // (A, B, Y)
  {"LUT_K", SKIP},                           // (in, out)
  {"DFF", FF_NODE},                          // (clock, D, Q)
  {"fpga_interconnect", operation_list_END}, // (datain, dataout)
  {"mux", SMUX_2},                           // (select, x, y, z)
  {"adder", ADD},                            // (a, b, out)
  {"multiply", MULTIPLY},                    // (a, b, cin, cout, sumout)
  {"single_port_ram", SPRAM},                // (clock, addr, data, we, out)
  {"dual_port_ram", DPRAM}                   // (clock, addr1, addr2, data1, data2, we1, we2, out1, out2)
});
