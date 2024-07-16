// Generator : SpinalHDL v1.8.0    git head : 4e3563a282582b41f4eaafc503787757251d23ea
// Component : VexRiscvSecure
// Git hash  : 51b69a1527c01616f386fa5cffb993313bfec919

`timescale 1ns/1ps

module VexRiscv (
  output              dBus_cmd_valid,
  input               dBus_cmd_ready,
  output              dBus_cmd_payload_wr,
  output              dBus_cmd_payload_uncached,
  output     [31:0]   dBus_cmd_payload_address,
  output     [31:0]   dBus_cmd_payload_data,
  output     [3:0]    dBus_cmd_payload_mask,
  output     [2:0]    dBus_cmd_payload_size,
  output              dBus_cmd_payload_last,
  input               dBus_rsp_valid,
  input               dBus_rsp_payload_last,
  input      [31:0]   dBus_rsp_payload_data,
  input               dBus_rsp_payload_error,
  input               timerInterrupt,
  input               externalInterrupt,
  input               softwareInterrupt,
  input               debug_bus_cmd_valid,
  output reg          debug_bus_cmd_ready,
  input               debug_bus_cmd_payload_wr,
  input      [7:0]    debug_bus_cmd_payload_address,
  input      [31:0]   debug_bus_cmd_payload_data,
  output reg [31:0]   debug_bus_rsp_data,
  output              debug_resetOut,
  output              iBus_cmd_valid,
  input               iBus_cmd_ready,
  output reg [31:0]   iBus_cmd_payload_address,
  output     [2:0]    iBus_cmd_payload_size,
  input               iBus_rsp_valid,
  input      [31:0]   iBus_rsp_payload_data,
  input               iBus_rsp_payload_error,
  input               clk,
  input               reset,
  input               debugReset
);
  localparam ShiftCtrlEnum_DISABLE_1 = 2'd0;
  localparam ShiftCtrlEnum_SLL_1 = 2'd1;
  localparam ShiftCtrlEnum_SRL_1 = 2'd2;
  localparam ShiftCtrlEnum_SRA_1 = 2'd3;
  localparam BranchCtrlEnum_INC = 2'd0;
  localparam BranchCtrlEnum_B = 2'd1;
  localparam BranchCtrlEnum_JAL = 2'd2;
  localparam BranchCtrlEnum_JALR = 2'd3;
  localparam EnvCtrlEnum_NONE = 2'd0;
  localparam EnvCtrlEnum_XRET = 2'd1;
  localparam EnvCtrlEnum_WFI = 2'd2;
  localparam EnvCtrlEnum_ECALL = 2'd3;
  localparam AluBitwiseCtrlEnum_XOR_1 = 2'd0;
  localparam AluBitwiseCtrlEnum_OR_1 = 2'd1;
  localparam AluBitwiseCtrlEnum_AND_1 = 2'd2;
  localparam Src2CtrlEnum_RS = 2'd0;
  localparam Src2CtrlEnum_IMI = 2'd1;
  localparam Src2CtrlEnum_IMS = 2'd2;
  localparam Src2CtrlEnum_PC = 2'd3;
  localparam AluCtrlEnum_ADD_SUB = 2'd0;
  localparam AluCtrlEnum_SLT_SLTU = 2'd1;
  localparam AluCtrlEnum_BITWISE = 2'd2;
  localparam Src1CtrlEnum_RS = 2'd0;
  localparam Src1CtrlEnum_IMU = 2'd1;
  localparam Src1CtrlEnum_PC_INCREMENT = 2'd2;
  localparam Src1CtrlEnum_URS1 = 2'd3;
  localparam execute_PmpPlugin_fsm_enumDef_BOOT = 3'd0;
  localparam execute_PmpPlugin_fsm_enumDef_stateIdle = 3'd1;
  localparam execute_PmpPlugin_fsm_enumDef_stateWrite = 3'd2;
  localparam execute_PmpPlugin_fsm_enumDef_stateCfg = 3'd3;
  localparam execute_PmpPlugin_fsm_enumDef_stateAddr = 3'd4;

  reg        [31:0]   PmpPlugin_setter_io_addr;
  wire                IBusCachedPlugin_cache_io_flush;
  wire                IBusCachedPlugin_cache_io_cpu_prefetch_isValid;
  wire                IBusCachedPlugin_cache_io_cpu_fetch_isValid;
  wire                IBusCachedPlugin_cache_io_cpu_fetch_isStuck;
  wire                IBusCachedPlugin_cache_io_cpu_fetch_isRemoved;
  wire                IBusCachedPlugin_cache_io_cpu_decode_isValid;
  wire                IBusCachedPlugin_cache_io_cpu_decode_isStuck;
  wire                IBusCachedPlugin_cache_io_cpu_decode_isUser;
  reg                 IBusCachedPlugin_cache_io_cpu_fill_valid;
  wire                dataCache_1_io_cpu_execute_isValid;
  wire       [31:0]   dataCache_1_io_cpu_execute_address;
  wire                dataCache_1_io_cpu_memory_isValid;
  wire       [31:0]   dataCache_1_io_cpu_memory_address;
  reg                 dataCache_1_io_cpu_memory_mmuRsp_isIoAccess;
  reg                 dataCache_1_io_cpu_writeBack_isValid;
  wire                dataCache_1_io_cpu_writeBack_isUser;
  wire       [31:0]   dataCache_1_io_cpu_writeBack_storeData;
  wire       [31:0]   dataCache_1_io_cpu_writeBack_address;
  wire                dataCache_1_io_cpu_writeBack_fence_SW;
  wire                dataCache_1_io_cpu_writeBack_fence_SR;
  wire                dataCache_1_io_cpu_writeBack_fence_SO;
  wire                dataCache_1_io_cpu_writeBack_fence_SI;
  wire                dataCache_1_io_cpu_writeBack_fence_PW;
  wire                dataCache_1_io_cpu_writeBack_fence_PR;
  wire                dataCache_1_io_cpu_writeBack_fence_PO;
  wire                dataCache_1_io_cpu_writeBack_fence_PI;
  wire       [3:0]    dataCache_1_io_cpu_writeBack_fence_FM;
  wire                dataCache_1_io_cpu_flush_valid;
  wire                dataCache_1_io_cpu_flush_payload_singleLine;
  wire       [6:0]    dataCache_1_io_cpu_flush_payload_lineId;
  wire       [31:0]   _zz_PmpPlugin_pmpaddr_port0;
  wire       [31:0]   _zz_PmpPlugin_pmpaddr_port2;
  reg        [31:0]   _zz_RegFilePlugin_regFile_port0;
  reg        [31:0]   _zz_RegFilePlugin_regFile_port1;
  wire       [27:0]   PmpPlugin_setter_io_base;
  wire       [27:0]   PmpPlugin_setter_io_mask;
  wire                IBusCachedPlugin_cache_io_cpu_prefetch_haltIt;
  wire       [31:0]   IBusCachedPlugin_cache_io_cpu_fetch_data;
  wire       [31:0]   IBusCachedPlugin_cache_io_cpu_fetch_physicalAddress;
  wire                IBusCachedPlugin_cache_io_cpu_decode_error;
  wire                IBusCachedPlugin_cache_io_cpu_decode_mmuRefilling;
  wire                IBusCachedPlugin_cache_io_cpu_decode_mmuException;
  wire       [31:0]   IBusCachedPlugin_cache_io_cpu_decode_data;
  wire                IBusCachedPlugin_cache_io_cpu_decode_cacheMiss;
  wire       [31:0]   IBusCachedPlugin_cache_io_cpu_decode_physicalAddress;
  wire                IBusCachedPlugin_cache_io_mem_cmd_valid;
  wire       [31:0]   IBusCachedPlugin_cache_io_mem_cmd_payload_address;
  wire       [2:0]    IBusCachedPlugin_cache_io_mem_cmd_payload_size;
  wire                dataCache_1_io_cpu_execute_haltIt;
  wire                dataCache_1_io_cpu_execute_refilling;
  wire                dataCache_1_io_cpu_memory_isWrite;
  wire                dataCache_1_io_cpu_writeBack_haltIt;
  wire       [31:0]   dataCache_1_io_cpu_writeBack_data;
  wire                dataCache_1_io_cpu_writeBack_mmuException;
  wire                dataCache_1_io_cpu_writeBack_unalignedAccess;
  wire                dataCache_1_io_cpu_writeBack_accessError;
  wire                dataCache_1_io_cpu_writeBack_isWrite;
  wire                dataCache_1_io_cpu_writeBack_keepMemRspData;
  wire                dataCache_1_io_cpu_writeBack_exclusiveOk;
  wire                dataCache_1_io_cpu_flush_ready;
  wire                dataCache_1_io_cpu_redo;
  wire                dataCache_1_io_mem_cmd_valid;
  wire                dataCache_1_io_mem_cmd_payload_wr;
  wire                dataCache_1_io_mem_cmd_payload_uncached;
  wire       [31:0]   dataCache_1_io_mem_cmd_payload_address;
  wire       [31:0]   dataCache_1_io_mem_cmd_payload_data;
  wire       [3:0]    dataCache_1_io_mem_cmd_payload_mask;
  wire       [2:0]    dataCache_1_io_mem_cmd_payload_size;
  wire                dataCache_1_io_mem_cmd_payload_last;
  wire       [31:0]   _zz_execute_SHIFT_RIGHT;
  wire       [32:0]   _zz_execute_SHIFT_RIGHT_1;
  wire       [32:0]   _zz_execute_SHIFT_RIGHT_2;
  wire       [31:0]   _zz_decode_LEGAL_INSTRUCTION;
  wire       [31:0]   _zz_decode_LEGAL_INSTRUCTION_1;
  wire       [31:0]   _zz_decode_LEGAL_INSTRUCTION_2;
  wire                _zz_decode_LEGAL_INSTRUCTION_3;
  wire       [0:0]    _zz_decode_LEGAL_INSTRUCTION_4;
  wire       [12:0]   _zz_decode_LEGAL_INSTRUCTION_5;
  wire       [31:0]   _zz_decode_LEGAL_INSTRUCTION_6;
  wire       [31:0]   _zz_decode_LEGAL_INSTRUCTION_7;
  wire       [31:0]   _zz_decode_LEGAL_INSTRUCTION_8;
  wire                _zz_decode_LEGAL_INSTRUCTION_9;
  wire       [0:0]    _zz_decode_LEGAL_INSTRUCTION_10;
  wire       [6:0]    _zz_decode_LEGAL_INSTRUCTION_11;
  wire       [31:0]   _zz_decode_LEGAL_INSTRUCTION_12;
  wire       [31:0]   _zz_decode_LEGAL_INSTRUCTION_13;
  wire       [31:0]   _zz_decode_LEGAL_INSTRUCTION_14;
  wire                _zz_decode_LEGAL_INSTRUCTION_15;
  wire       [0:0]    _zz_decode_LEGAL_INSTRUCTION_16;
  wire       [0:0]    _zz_decode_LEGAL_INSTRUCTION_17;
  wire       [3:0]    _zz__zz_IBusCachedPlugin_jump_pcLoad_payload_1;
  reg        [31:0]   _zz_IBusCachedPlugin_jump_pcLoad_payload_5;
  wire       [1:0]    _zz_IBusCachedPlugin_jump_pcLoad_payload_6;
  wire       [31:0]   _zz_IBusCachedPlugin_fetchPc_pc;
  wire       [2:0]    _zz_IBusCachedPlugin_fetchPc_pc_1;
  wire       [11:0]   _zz__zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch;
  wire       [31:0]   _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch_2;
  wire       [19:0]   _zz__zz_3;
  wire       [11:0]   _zz__zz_5;
  wire       [31:0]   _zz__zz_7;
  wire       [31:0]   _zz__zz_7_1;
  wire       [19:0]   _zz__zz_IBusCachedPlugin_predictionJumpInterface_payload;
  wire       [11:0]   _zz__zz_IBusCachedPlugin_predictionJumpInterface_payload_2;
  wire                _zz_IBusCachedPlugin_predictionJumpInterface_payload_4;
  wire                _zz_IBusCachedPlugin_predictionJumpInterface_payload_5;
  wire                _zz_IBusCachedPlugin_predictionJumpInterface_payload_6;
  wire       [26:0]   _zz_io_cpu_flush_payload_lineId;
  wire       [26:0]   _zz_io_cpu_flush_payload_lineId_1;
  wire       [2:0]    _zz_DBusCachedPlugin_exceptionBus_payload_code;
  wire       [2:0]    _zz_DBusCachedPlugin_exceptionBus_payload_code_1;
  reg        [7:0]    _zz_writeBack_DBusCachedPlugin_rspShifted;
  wire       [1:0]    _zz_writeBack_DBusCachedPlugin_rspShifted_1;
  reg        [7:0]    _zz_writeBack_DBusCachedPlugin_rspShifted_2;
  wire       [0:0]    _zz_writeBack_DBusCachedPlugin_rspShifted_3;
  reg        [7:0]    _zz_when_PmpPlugin_l246;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_0_1;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_0_2;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_0_3;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_0_4;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_1;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_1_1;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_1_2;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_1_3;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_2;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_2_1;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_2_2;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_2_3;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_3;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_3_1;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_3_2;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_3_3;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_4;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_4_1;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_4_2;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_4_3;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_5;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_5_1;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_5_2;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_5_3;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_6;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_6_1;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_6_2;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_6_3;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_7;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_7_1;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_7_2;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_7_3;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_8;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_8_1;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_8_2;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_8_3;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_9;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_9_1;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_9_2;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_9_3;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_10;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_10_1;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_10_2;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_10_3;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_11;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_11_1;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_11_2;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_11_3;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_12;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_12_1;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_12_2;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_12_3;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_13;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_13_1;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_13_2;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_13_3;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_14;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_14_1;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_14_2;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_14_3;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_15;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_15_1;
  reg        [27:0]   _zz_PmpPlugin_dGuard_hits_15_2;
  wire       [3:0]    _zz_PmpPlugin_dGuard_hits_15_3;
  wire       [0:0]    _zz_when_PmpPlugin_l277;
  wire       [5:0]    _zz_when_PmpPlugin_l277_1;
  wire       [0:0]    _zz__zz_DBusCachedPlugin_mmuBus_rsp_allowRead;
  wire       [4:0]    _zz__zz_DBusCachedPlugin_mmuBus_rsp_allowRead_1;
  wire       [15:0]   _zz__zz_DBusCachedPlugin_mmuBus_rsp_allowRead_1_1;
  reg                 _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_17;
  wire       [3:0]    _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_18;
  wire       [0:0]    _zz__zz_DBusCachedPlugin_mmuBus_rsp_allowWrite;
  wire       [4:0]    _zz__zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_1;
  wire       [15:0]   _zz__zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_1_1;
  reg                 _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_17;
  wire       [3:0]    _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_18;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_0_1;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_0_2;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_0_3;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_0_4;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_1;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_1_1;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_1_2;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_1_3;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_2;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_2_1;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_2_2;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_2_3;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_3;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_3_1;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_3_2;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_3_3;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_4;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_4_1;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_4_2;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_4_3;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_5;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_5_1;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_5_2;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_5_3;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_6;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_6_1;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_6_2;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_6_3;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_7;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_7_1;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_7_2;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_7_3;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_8;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_8_1;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_8_2;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_8_3;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_9;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_9_1;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_9_2;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_9_3;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_10;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_10_1;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_10_2;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_10_3;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_11;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_11_1;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_11_2;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_11_3;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_12;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_12_1;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_12_2;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_12_3;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_13;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_13_1;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_13_2;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_13_3;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_14;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_14_1;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_14_2;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_14_3;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_15;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_15_1;
  reg        [27:0]   _zz_PmpPlugin_iGuard_hits_15_2;
  wire       [3:0]    _zz_PmpPlugin_iGuard_hits_15_3;
  wire       [0:0]    _zz_when_PmpPlugin_l299;
  wire       [5:0]    _zz_when_PmpPlugin_l299_1;
  wire       [0:0]    _zz__zz_IBusCachedPlugin_mmuBus_rsp_allowExecute;
  wire       [4:0]    _zz__zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_1;
  wire       [15:0]   _zz__zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_1_1;
  reg                 _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_17;
  wire       [3:0]    _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_18;
  wire       [31:0]   _zz__zz_decode_IS_CSR;
  wire       [31:0]   _zz__zz_decode_IS_CSR_1;
  wire       [31:0]   _zz__zz_decode_IS_CSR_2;
  wire       [31:0]   _zz__zz_decode_IS_CSR_3;
  wire                _zz__zz_decode_IS_CSR_4;
  wire       [1:0]    _zz__zz_decode_IS_CSR_5;
  wire       [31:0]   _zz__zz_decode_IS_CSR_6;
  wire       [31:0]   _zz__zz_decode_IS_CSR_7;
  wire                _zz__zz_decode_IS_CSR_8;
  wire       [31:0]   _zz__zz_decode_IS_CSR_9;
  wire       [0:0]    _zz__zz_decode_IS_CSR_10;
  wire                _zz__zz_decode_IS_CSR_11;
  wire                _zz__zz_decode_IS_CSR_12;
  wire       [26:0]   _zz__zz_decode_IS_CSR_13;
  wire                _zz__zz_decode_IS_CSR_14;
  wire       [1:0]    _zz__zz_decode_IS_CSR_15;
  wire                _zz__zz_decode_IS_CSR_16;
  wire       [0:0]    _zz__zz_decode_IS_CSR_17;
  wire       [31:0]   _zz__zz_decode_IS_CSR_18;
  wire       [31:0]   _zz__zz_decode_IS_CSR_19;
  wire       [22:0]   _zz__zz_decode_IS_CSR_20;
  wire                _zz__zz_decode_IS_CSR_21;
  wire       [1:0]    _zz__zz_decode_IS_CSR_22;
  wire       [31:0]   _zz__zz_decode_IS_CSR_23;
  wire       [31:0]   _zz__zz_decode_IS_CSR_24;
  wire                _zz__zz_decode_IS_CSR_25;
  wire       [31:0]   _zz__zz_decode_IS_CSR_26;
  wire       [0:0]    _zz__zz_decode_IS_CSR_27;
  wire       [31:0]   _zz__zz_decode_IS_CSR_28;
  wire       [31:0]   _zz__zz_decode_IS_CSR_29;
  wire       [18:0]   _zz__zz_decode_IS_CSR_30;
  wire       [1:0]    _zz__zz_decode_IS_CSR_31;
  wire       [31:0]   _zz__zz_decode_IS_CSR_32;
  wire       [31:0]   _zz__zz_decode_IS_CSR_33;
  wire                _zz__zz_decode_IS_CSR_34;
  wire       [31:0]   _zz__zz_decode_IS_CSR_35;
  wire       [0:0]    _zz__zz_decode_IS_CSR_36;
  wire                _zz__zz_decode_IS_CSR_37;
  wire                _zz__zz_decode_IS_CSR_38;
  wire       [14:0]   _zz__zz_decode_IS_CSR_39;
  wire       [0:0]    _zz__zz_decode_IS_CSR_40;
  wire       [31:0]   _zz__zz_decode_IS_CSR_41;
  wire       [3:0]    _zz__zz_decode_IS_CSR_42;
  wire       [31:0]   _zz__zz_decode_IS_CSR_43;
  wire       [31:0]   _zz__zz_decode_IS_CSR_44;
  wire                _zz__zz_decode_IS_CSR_45;
  wire       [0:0]    _zz__zz_decode_IS_CSR_46;
  wire       [31:0]   _zz__zz_decode_IS_CSR_47;
  wire       [0:0]    _zz__zz_decode_IS_CSR_48;
  wire       [0:0]    _zz__zz_decode_IS_CSR_49;
  wire       [31:0]   _zz__zz_decode_IS_CSR_50;
  wire                _zz__zz_decode_IS_CSR_51;
  wire       [31:0]   _zz__zz_decode_IS_CSR_52;
  wire       [31:0]   _zz__zz_decode_IS_CSR_53;
  wire       [0:0]    _zz__zz_decode_IS_CSR_54;
  wire       [0:0]    _zz__zz_decode_IS_CSR_55;
  wire       [3:0]    _zz__zz_decode_IS_CSR_56;
  wire       [31:0]   _zz__zz_decode_IS_CSR_57;
  wire       [31:0]   _zz__zz_decode_IS_CSR_58;
  wire                _zz__zz_decode_IS_CSR_59;
  wire       [31:0]   _zz__zz_decode_IS_CSR_60;
  wire       [0:0]    _zz__zz_decode_IS_CSR_61;
  wire       [31:0]   _zz__zz_decode_IS_CSR_62;
  wire       [31:0]   _zz__zz_decode_IS_CSR_63;
  wire       [0:0]    _zz__zz_decode_IS_CSR_64;
  wire       [31:0]   _zz__zz_decode_IS_CSR_65;
  wire       [31:0]   _zz__zz_decode_IS_CSR_66;
  wire       [10:0]   _zz__zz_decode_IS_CSR_67;
  wire       [5:0]    _zz__zz_decode_IS_CSR_68;
  wire                _zz__zz_decode_IS_CSR_69;
  wire       [31:0]   _zz__zz_decode_IS_CSR_70;
  wire       [0:0]    _zz__zz_decode_IS_CSR_71;
  wire       [31:0]   _zz__zz_decode_IS_CSR_72;
  wire       [31:0]   _zz__zz_decode_IS_CSR_73;
  wire       [2:0]    _zz__zz_decode_IS_CSR_74;
  wire                _zz__zz_decode_IS_CSR_75;
  wire       [0:0]    _zz__zz_decode_IS_CSR_76;
  wire       [31:0]   _zz__zz_decode_IS_CSR_77;
  wire       [0:0]    _zz__zz_decode_IS_CSR_78;
  wire       [31:0]   _zz__zz_decode_IS_CSR_79;
  wire                _zz__zz_decode_IS_CSR_80;
  wire                _zz__zz_decode_IS_CSR_81;
  wire       [31:0]   _zz__zz_decode_IS_CSR_82;
  wire       [0:0]    _zz__zz_decode_IS_CSR_83;
  wire       [0:0]    _zz__zz_decode_IS_CSR_84;
  wire       [0:0]    _zz__zz_decode_IS_CSR_85;
  wire       [31:0]   _zz__zz_decode_IS_CSR_86;
  wire       [31:0]   _zz__zz_decode_IS_CSR_87;
  wire       [7:0]    _zz__zz_decode_IS_CSR_88;
  wire       [0:0]    _zz__zz_decode_IS_CSR_89;
  wire       [31:0]   _zz__zz_decode_IS_CSR_90;
  wire       [31:0]   _zz__zz_decode_IS_CSR_91;
  wire                _zz__zz_decode_IS_CSR_92;
  wire                _zz__zz_decode_IS_CSR_93;
  wire       [0:0]    _zz__zz_decode_IS_CSR_94;
  wire       [4:0]    _zz__zz_decode_IS_CSR_95;
  wire       [31:0]   _zz__zz_decode_IS_CSR_96;
  wire       [31:0]   _zz__zz_decode_IS_CSR_97;
  wire       [0:0]    _zz__zz_decode_IS_CSR_98;
  wire       [31:0]   _zz__zz_decode_IS_CSR_99;
  wire       [1:0]    _zz__zz_decode_IS_CSR_100;
  wire       [31:0]   _zz__zz_decode_IS_CSR_101;
  wire       [31:0]   _zz__zz_decode_IS_CSR_102;
  wire       [31:0]   _zz__zz_decode_IS_CSR_103;
  wire       [31:0]   _zz__zz_decode_IS_CSR_104;
  wire       [4:0]    _zz__zz_decode_IS_CSR_105;
  wire                _zz__zz_decode_IS_CSR_106;
  wire       [31:0]   _zz__zz_decode_IS_CSR_107;
  wire       [31:0]   _zz__zz_decode_IS_CSR_108;
  wire       [0:0]    _zz__zz_decode_IS_CSR_109;
  wire       [0:0]    _zz__zz_decode_IS_CSR_110;
  wire       [31:0]   _zz__zz_decode_IS_CSR_111;
  wire       [1:0]    _zz__zz_decode_IS_CSR_112;
  wire       [31:0]   _zz__zz_decode_IS_CSR_113;
  wire       [31:0]   _zz__zz_decode_IS_CSR_114;
  wire       [31:0]   _zz__zz_decode_IS_CSR_115;
  wire       [31:0]   _zz__zz_decode_IS_CSR_116;
  wire       [2:0]    _zz__zz_decode_IS_CSR_117;
  wire       [1:0]    _zz__zz_decode_IS_CSR_118;
  wire       [31:0]   _zz__zz_decode_IS_CSR_119;
  wire       [31:0]   _zz__zz_decode_IS_CSR_120;
  wire                _zz__zz_decode_IS_CSR_121;
  wire                _zz__zz_decode_IS_CSR_122;
  wire                _zz__zz_decode_IS_CSR_123;
  wire       [31:0]   _zz__zz_decode_IS_CSR_124;
  wire       [31:0]   _zz__zz_decode_IS_CSR_125;
  wire                _zz_RegFilePlugin_regFile_port;
  wire                _zz_decode_RegFilePlugin_rs1Data;
  wire                _zz_RegFilePlugin_regFile_port_1;
  wire                _zz_decode_RegFilePlugin_rs2Data;
  wire       [0:0]    _zz__zz_execute_REGFILE_WRITE_DATA;
  wire       [2:0]    _zz__zz_execute_SRC1;
  wire       [4:0]    _zz__zz_execute_SRC1_1;
  wire       [11:0]   _zz__zz_execute_SRC2_2;
  wire       [31:0]   _zz_execute_SrcPlugin_addSub;
  wire       [31:0]   _zz_execute_SrcPlugin_addSub_1;
  wire       [31:0]   _zz_execute_SrcPlugin_addSub_2;
  wire       [31:0]   _zz_execute_SrcPlugin_addSub_3;
  wire       [31:0]   _zz_execute_SrcPlugin_addSub_4;
  wire       [31:0]   _zz_execute_SrcPlugin_addSub_5;
  wire       [31:0]   _zz_execute_SrcPlugin_addSub_6;
  wire       [5:0]    _zz_memory_MulDivIterativePlugin_mul_counter_valueNext;
  wire       [0:0]    _zz_memory_MulDivIterativePlugin_mul_counter_valueNext_1;
  wire       [33:0]   _zz_memory_MulDivIterativePlugin_accumulator;
  wire       [33:0]   _zz_memory_MulDivIterativePlugin_accumulator_1;
  wire       [32:0]   _zz_memory_MulDivIterativePlugin_accumulator_2;
  wire       [33:0]   _zz_memory_MulDivIterativePlugin_accumulator_3;
  wire       [32:0]   _zz_memory_MulDivIterativePlugin_accumulator_4;
  wire       [32:0]   _zz_memory_MulDivIterativePlugin_accumulator_5;
  wire       [5:0]    _zz_memory_MulDivIterativePlugin_div_counter_valueNext;
  wire       [0:0]    _zz_memory_MulDivIterativePlugin_div_counter_valueNext_1;
  wire       [32:0]   _zz_memory_MulDivIterativePlugin_div_stage_0_remainderMinusDenominator;
  wire       [31:0]   _zz_memory_MulDivIterativePlugin_div_stage_0_outRemainder;
  wire       [31:0]   _zz_memory_MulDivIterativePlugin_div_stage_0_outRemainder_1;
  wire       [32:0]   _zz_memory_MulDivIterativePlugin_div_stage_0_outNumerator;
  wire       [32:0]   _zz_memory_MulDivIterativePlugin_div_result_1;
  wire       [32:0]   _zz_memory_MulDivIterativePlugin_div_result_2;
  wire       [32:0]   _zz_memory_MulDivIterativePlugin_div_result_3;
  wire       [32:0]   _zz_memory_MulDivIterativePlugin_div_result_4;
  wire       [0:0]    _zz_memory_MulDivIterativePlugin_div_result_5;
  wire       [32:0]   _zz_memory_MulDivIterativePlugin_rs1_2;
  wire       [0:0]    _zz_memory_MulDivIterativePlugin_rs1_3;
  wire       [31:0]   _zz_memory_MulDivIterativePlugin_rs2_1;
  wire       [0:0]    _zz_memory_MulDivIterativePlugin_rs2_2;
  wire       [1:0]    _zz__zz_CsrPlugin_exceptionPortCtrl_exceptionContext_code_1;
  wire       [1:0]    _zz__zz_CsrPlugin_exceptionPortCtrl_exceptionContext_code_1_1;
  wire                _zz_when;
  wire       [19:0]   _zz__zz_execute_BranchPlugin_missAlignedTarget_2;
  wire       [11:0]   _zz__zz_execute_BranchPlugin_missAlignedTarget_4;
  wire       [31:0]   _zz__zz_execute_BranchPlugin_missAlignedTarget_6;
  wire       [31:0]   _zz__zz_execute_BranchPlugin_missAlignedTarget_6_1;
  wire       [31:0]   _zz__zz_execute_BranchPlugin_missAlignedTarget_6_2;
  wire       [19:0]   _zz__zz_execute_BranchPlugin_branch_src2_2;
  wire       [11:0]   _zz__zz_execute_BranchPlugin_branch_src2_4;
  wire                _zz_execute_BranchPlugin_branch_src2_6;
  wire                _zz_execute_BranchPlugin_branch_src2_7;
  wire                _zz_execute_BranchPlugin_branch_src2_8;
  wire       [2:0]    _zz_execute_BranchPlugin_branch_src2_9;
  wire       [31:0]   _zz_PmpPlugin_pmpaddr_port;
  reg        [7:0]    _zz_when_PmpPlugin_l209;
  wire       [3:0]    _zz_when_PmpPlugin_l209_1;
  reg        [7:0]    _zz_when_PmpPlugin_l209_1_1;
  wire       [3:0]    _zz_when_PmpPlugin_l209_1_2;
  reg        [7:0]    _zz_when_PmpPlugin_l209_2;
  wire       [3:0]    _zz_when_PmpPlugin_l209_2_1;
  reg        [7:0]    _zz_when_PmpPlugin_l209_3;
  wire       [3:0]    _zz_when_PmpPlugin_l209_3_1;
  reg        [7:0]    _zz_when_PmpPlugin_l216;
  wire       [31:0]   _zz_CsrPlugin_csrMapping_readDataInit_20;
  reg        [7:0]    _zz_CsrPlugin_csrMapping_readDataSignal;
  wire       [3:0]    _zz_CsrPlugin_csrMapping_readDataSignal_1;
  reg        [7:0]    _zz_CsrPlugin_csrMapping_readDataSignal_2;
  wire       [3:0]    _zz_CsrPlugin_csrMapping_readDataSignal_3;
  reg        [7:0]    _zz_CsrPlugin_csrMapping_readDataSignal_4;
  wire       [3:0]    _zz_CsrPlugin_csrMapping_readDataSignal_5;
  reg        [7:0]    _zz_CsrPlugin_csrMapping_readDataSignal_6;
  wire       [3:0]    _zz_CsrPlugin_csrMapping_readDataSignal_7;
  reg                 _zz_1;
  wire       [31:0]   execute_BRANCH_CALC;
  wire                execute_BRANCH_DO;
  wire       [31:0]   execute_SHIFT_RIGHT;
  wire       [31:0]   execute_REGFILE_WRITE_DATA;
  wire       [31:0]   memory_MEMORY_STORE_DATA_RF;
  wire       [31:0]   execute_MEMORY_STORE_DATA_RF;
  wire                decode_PREDICTION_HAD_BRANCHED2;
  wire                decode_DO_EBREAK;
  wire                decode_CSR_READ_OPCODE;
  wire                decode_CSR_WRITE_OPCODE;
  wire                decode_SRC2_FORCE_ZERO;
  wire       [1:0]    _zz_decode_to_execute_BRANCH_CTRL;
  wire       [1:0]    _zz_decode_to_execute_BRANCH_CTRL_1;
  wire       [1:0]    _zz_memory_to_writeBack_ENV_CTRL;
  wire       [1:0]    _zz_memory_to_writeBack_ENV_CTRL_1;
  wire       [1:0]    _zz_execute_to_memory_ENV_CTRL;
  wire       [1:0]    _zz_execute_to_memory_ENV_CTRL_1;
  wire       [1:0]    decode_ENV_CTRL;
  wire       [1:0]    _zz_decode_ENV_CTRL;
  wire       [1:0]    _zz_decode_to_execute_ENV_CTRL;
  wire       [1:0]    _zz_decode_to_execute_ENV_CTRL_1;
  wire                decode_IS_CSR;
  wire                decode_IS_DIV;
  wire                decode_IS_RS2_SIGNED;
  wire                decode_IS_RS1_SIGNED;
  wire                decode_IS_MUL;
  wire       [1:0]    _zz_execute_to_memory_SHIFT_CTRL;
  wire       [1:0]    _zz_execute_to_memory_SHIFT_CTRL_1;
  wire       [1:0]    decode_SHIFT_CTRL;
  wire       [1:0]    _zz_decode_SHIFT_CTRL;
  wire       [1:0]    _zz_decode_to_execute_SHIFT_CTRL;
  wire       [1:0]    _zz_decode_to_execute_SHIFT_CTRL_1;
  wire       [1:0]    decode_ALU_BITWISE_CTRL;
  wire       [1:0]    _zz_decode_ALU_BITWISE_CTRL;
  wire       [1:0]    _zz_decode_to_execute_ALU_BITWISE_CTRL;
  wire       [1:0]    _zz_decode_to_execute_ALU_BITWISE_CTRL_1;
  wire                decode_SRC_LESS_UNSIGNED;
  wire                decode_MEMORY_MANAGMENT;
  wire                memory_MEMORY_WR;
  wire                decode_MEMORY_WR;
  wire                execute_BYPASSABLE_MEMORY_STAGE;
  wire                decode_BYPASSABLE_MEMORY_STAGE;
  wire                decode_BYPASSABLE_EXECUTE_STAGE;
  wire       [1:0]    decode_SRC2_CTRL;
  wire       [1:0]    _zz_decode_SRC2_CTRL;
  wire       [1:0]    _zz_decode_to_execute_SRC2_CTRL;
  wire       [1:0]    _zz_decode_to_execute_SRC2_CTRL_1;
  wire       [1:0]    decode_ALU_CTRL;
  wire       [1:0]    _zz_decode_ALU_CTRL;
  wire       [1:0]    _zz_decode_to_execute_ALU_CTRL;
  wire       [1:0]    _zz_decode_to_execute_ALU_CTRL_1;
  wire       [1:0]    decode_SRC1_CTRL;
  wire       [1:0]    _zz_decode_SRC1_CTRL;
  wire       [1:0]    _zz_decode_to_execute_SRC1_CTRL;
  wire       [1:0]    _zz_decode_to_execute_SRC1_CTRL_1;
  wire                decode_MEMORY_FORCE_CONSTISTENCY;
  wire       [31:0]   writeBack_FORMAL_PC_NEXT;
  wire       [31:0]   memory_FORMAL_PC_NEXT;
  wire       [31:0]   execute_FORMAL_PC_NEXT;
  wire       [31:0]   decode_FORMAL_PC_NEXT;
  wire       [31:0]   memory_PC;
  wire       [31:0]   memory_BRANCH_CALC;
  wire                memory_BRANCH_DO;
  wire                execute_PREDICTION_HAD_BRANCHED2;
  wire                execute_BRANCH_COND_RESULT;
  wire       [1:0]    execute_BRANCH_CTRL;
  wire       [1:0]    _zz_execute_BRANCH_CTRL;
  wire       [31:0]   execute_PC;
  wire                execute_DO_EBREAK;
  wire                decode_IS_EBREAK;
  wire                execute_CSR_READ_OPCODE;
  wire                execute_CSR_WRITE_OPCODE;
  wire                execute_IS_CSR;
  wire       [1:0]    memory_ENV_CTRL;
  wire       [1:0]    _zz_memory_ENV_CTRL;
  wire       [1:0]    execute_ENV_CTRL;
  wire       [1:0]    _zz_execute_ENV_CTRL;
  wire       [1:0]    writeBack_ENV_CTRL;
  wire       [1:0]    _zz_writeBack_ENV_CTRL;
  wire                execute_IS_RS1_SIGNED;
  wire                execute_IS_DIV;
  wire                execute_IS_MUL;
  wire                execute_IS_RS2_SIGNED;
  wire                memory_IS_DIV;
  wire                memory_IS_MUL;
  wire                decode_RS2_USE;
  wire                decode_RS1_USE;
  reg        [31:0]   _zz_decode_RS2;
  wire                execute_REGFILE_WRITE_VALID;
  wire                execute_BYPASSABLE_EXECUTE_STAGE;
  wire                memory_REGFILE_WRITE_VALID;
  wire       [31:0]   memory_INSTRUCTION;
  wire                memory_BYPASSABLE_MEMORY_STAGE;
  wire                writeBack_REGFILE_WRITE_VALID;
  reg        [31:0]   decode_RS2;
  reg        [31:0]   decode_RS1;
  wire       [31:0]   memory_SHIFT_RIGHT;
  reg        [31:0]   _zz_decode_RS2_1;
  wire       [1:0]    memory_SHIFT_CTRL;
  wire       [1:0]    _zz_memory_SHIFT_CTRL;
  wire       [1:0]    execute_SHIFT_CTRL;
  wire       [1:0]    _zz_execute_SHIFT_CTRL;
  wire                execute_SRC_LESS_UNSIGNED;
  wire                execute_SRC2_FORCE_ZERO;
  wire                execute_SRC_USE_SUB_LESS;
  wire       [31:0]   _zz_execute_to_memory_PC;
  wire       [1:0]    execute_SRC2_CTRL;
  wire       [1:0]    _zz_execute_SRC2_CTRL;
  wire       [1:0]    execute_SRC1_CTRL;
  wire       [1:0]    _zz_execute_SRC1_CTRL;
  wire                decode_SRC_USE_SUB_LESS;
  wire                decode_SRC_ADD_ZERO;
  wire       [31:0]   execute_SRC_ADD_SUB;
  wire                execute_SRC_LESS;
  wire       [1:0]    execute_ALU_CTRL;
  wire       [1:0]    _zz_execute_ALU_CTRL;
  wire       [31:0]   execute_SRC2;
  wire       [31:0]   execute_SRC1;
  wire       [1:0]    execute_ALU_BITWISE_CTRL;
  wire       [1:0]    _zz_execute_ALU_BITWISE_CTRL;
  wire       [31:0]   _zz_lastStageRegFileWrite_payload_address;
  wire                _zz_lastStageRegFileWrite_valid;
  reg                 _zz_2;
  wire       [31:0]   decode_INSTRUCTION_ANTICIPATED;
  reg                 decode_REGFILE_WRITE_VALID;
  wire                decode_LEGAL_INSTRUCTION;
  wire       [1:0]    _zz_decode_BRANCH_CTRL;
  wire       [1:0]    _zz_decode_ENV_CTRL_1;
  wire       [1:0]    _zz_decode_SHIFT_CTRL_1;
  wire       [1:0]    _zz_decode_ALU_BITWISE_CTRL_1;
  wire       [1:0]    _zz_decode_SRC2_CTRL_1;
  wire       [1:0]    _zz_decode_ALU_CTRL_1;
  wire       [1:0]    _zz_decode_SRC1_CTRL_1;
  reg        [31:0]   _zz_decode_RS2_2;
  wire                writeBack_MEMORY_WR;
  wire       [31:0]   writeBack_MEMORY_STORE_DATA_RF;
  wire       [31:0]   writeBack_REGFILE_WRITE_DATA;
  wire                writeBack_MEMORY_ENABLE;
  wire       [31:0]   memory_REGFILE_WRITE_DATA;
  wire                memory_MEMORY_ENABLE;
  wire                execute_MEMORY_FORCE_CONSTISTENCY;
  wire       [31:0]   execute_RS1;
  wire                execute_MEMORY_MANAGMENT;
  wire       [31:0]   execute_RS2;
  wire                execute_MEMORY_WR;
  wire       [31:0]   execute_SRC_ADD;
  wire                execute_MEMORY_ENABLE;
  wire       [31:0]   execute_INSTRUCTION;
  wire                decode_MEMORY_ENABLE;
  wire                decode_FLUSH_ALL;
  reg                 IBusCachedPlugin_rsp_issueDetected_4;
  reg                 IBusCachedPlugin_rsp_issueDetected_3;
  reg                 IBusCachedPlugin_rsp_issueDetected_2;
  reg                 IBusCachedPlugin_rsp_issueDetected_1;
  wire       [1:0]    decode_BRANCH_CTRL;
  wire       [1:0]    _zz_decode_BRANCH_CTRL_1;
  wire       [31:0]   decode_INSTRUCTION;
  reg        [31:0]   _zz_memory_to_writeBack_FORMAL_PC_NEXT;
  reg        [31:0]   _zz_decode_to_execute_FORMAL_PC_NEXT;
  wire       [31:0]   decode_PC;
  wire       [31:0]   writeBack_PC;
  wire       [31:0]   writeBack_INSTRUCTION;
  reg                 decode_arbitration_haltItself;
  reg                 decode_arbitration_haltByOther;
  reg                 decode_arbitration_removeIt;
  wire                decode_arbitration_flushIt;
  reg                 decode_arbitration_flushNext;
  reg                 decode_arbitration_isValid;
  wire                decode_arbitration_isStuck;
  wire                decode_arbitration_isStuckByOthers;
  wire                decode_arbitration_isFlushed;
  wire                decode_arbitration_isMoving;
  wire                decode_arbitration_isFiring;
  reg                 execute_arbitration_haltItself;
  reg                 execute_arbitration_haltByOther;
  reg                 execute_arbitration_removeIt;
  reg                 execute_arbitration_flushIt;
  reg                 execute_arbitration_flushNext;
  reg                 execute_arbitration_isValid;
  wire                execute_arbitration_isStuck;
  wire                execute_arbitration_isStuckByOthers;
  wire                execute_arbitration_isFlushed;
  wire                execute_arbitration_isMoving;
  wire                execute_arbitration_isFiring;
  reg                 memory_arbitration_haltItself;
  wire                memory_arbitration_haltByOther;
  reg                 memory_arbitration_removeIt;
  wire                memory_arbitration_flushIt;
  reg                 memory_arbitration_flushNext;
  reg                 memory_arbitration_isValid;
  wire                memory_arbitration_isStuck;
  wire                memory_arbitration_isStuckByOthers;
  wire                memory_arbitration_isFlushed;
  wire                memory_arbitration_isMoving;
  wire                memory_arbitration_isFiring;
  reg                 writeBack_arbitration_haltItself;
  wire                writeBack_arbitration_haltByOther;
  reg                 writeBack_arbitration_removeIt;
  reg                 writeBack_arbitration_flushIt;
  reg                 writeBack_arbitration_flushNext;
  reg                 writeBack_arbitration_isValid;
  wire                writeBack_arbitration_isStuck;
  wire                writeBack_arbitration_isStuckByOthers;
  wire                writeBack_arbitration_isFlushed;
  wire                writeBack_arbitration_isMoving;
  wire                writeBack_arbitration_isFiring;
  wire       [31:0]   lastStageInstruction /* verilator public */ ;
  wire       [31:0]   lastStagePc /* verilator public */ ;
  wire                lastStageIsValid /* verilator public */ ;
  wire                lastStageIsFiring /* verilator public */ ;
  reg                 IBusCachedPlugin_fetcherHalt;
  wire                IBusCachedPlugin_forceNoDecodeCond;
  reg                 IBusCachedPlugin_incomingInstruction;
  wire                IBusCachedPlugin_predictionJumpInterface_valid;
  (* keep , syn_keep *) wire       [31:0]   IBusCachedPlugin_predictionJumpInterface_payload /* synthesis syn_keep = 1 */ ;
  reg                 IBusCachedPlugin_decodePrediction_cmd_hadBranch;
  wire                IBusCachedPlugin_decodePrediction_rsp_wasWrong;
  wire                IBusCachedPlugin_pcValids_0;
  wire                IBusCachedPlugin_pcValids_1;
  wire                IBusCachedPlugin_pcValids_2;
  wire                IBusCachedPlugin_pcValids_3;
  reg                 IBusCachedPlugin_decodeExceptionPort_valid;
  reg        [3:0]    IBusCachedPlugin_decodeExceptionPort_payload_code;
  wire       [31:0]   IBusCachedPlugin_decodeExceptionPort_payload_badAddr;
  wire                IBusCachedPlugin_mmuBus_cmd_0_isValid;
  wire                IBusCachedPlugin_mmuBus_cmd_0_isStuck;
  wire       [31:0]   IBusCachedPlugin_mmuBus_cmd_0_virtualAddress;
  wire                IBusCachedPlugin_mmuBus_cmd_0_bypassTranslation;
  wire       [31:0]   IBusCachedPlugin_mmuBus_rsp_physicalAddress;
  wire                IBusCachedPlugin_mmuBus_rsp_isIoAccess;
  wire                IBusCachedPlugin_mmuBus_rsp_isPaging;
  wire                IBusCachedPlugin_mmuBus_rsp_allowRead;
  wire                IBusCachedPlugin_mmuBus_rsp_allowWrite;
  reg                 IBusCachedPlugin_mmuBus_rsp_allowExecute;
  wire                IBusCachedPlugin_mmuBus_rsp_exception;
  wire                IBusCachedPlugin_mmuBus_rsp_refilling;
  wire                IBusCachedPlugin_mmuBus_rsp_bypassTranslation;
  wire                IBusCachedPlugin_mmuBus_end;
  wire                IBusCachedPlugin_mmuBus_busy;
  wire                DBusCachedPlugin_mmuBus_cmd_0_isValid;
  wire                DBusCachedPlugin_mmuBus_cmd_0_isStuck;
  wire       [31:0]   DBusCachedPlugin_mmuBus_cmd_0_virtualAddress;
  wire                DBusCachedPlugin_mmuBus_cmd_0_bypassTranslation;
  wire       [31:0]   DBusCachedPlugin_mmuBus_rsp_physicalAddress;
  wire                DBusCachedPlugin_mmuBus_rsp_isIoAccess;
  wire                DBusCachedPlugin_mmuBus_rsp_isPaging;
  reg                 DBusCachedPlugin_mmuBus_rsp_allowRead;
  reg                 DBusCachedPlugin_mmuBus_rsp_allowWrite;
  wire                DBusCachedPlugin_mmuBus_rsp_allowExecute;
  wire                DBusCachedPlugin_mmuBus_rsp_exception;
  wire                DBusCachedPlugin_mmuBus_rsp_refilling;
  wire                DBusCachedPlugin_mmuBus_rsp_bypassTranslation;
  wire                DBusCachedPlugin_mmuBus_end;
  wire                DBusCachedPlugin_mmuBus_busy;
  reg                 DBusCachedPlugin_redoBranch_valid;
  wire       [31:0]   DBusCachedPlugin_redoBranch_payload;
  reg                 DBusCachedPlugin_exceptionBus_valid;
  reg        [3:0]    DBusCachedPlugin_exceptionBus_payload_code;
  wire       [31:0]   DBusCachedPlugin_exceptionBus_payload_badAddr;
  reg                 _zz_when_DBusCachedPlugin_l393;
  wire                decodeExceptionPort_valid;
  wire       [3:0]    decodeExceptionPort_payload_code;
  wire       [31:0]   decodeExceptionPort_payload_badAddr;
  reg        [31:0]   CsrPlugin_csrMapping_readDataSignal;
  wire       [31:0]   CsrPlugin_csrMapping_readDataInit;
  wire       [31:0]   CsrPlugin_csrMapping_writeDataSignal;
  reg                 CsrPlugin_csrMapping_allowCsrSignal;
  wire                CsrPlugin_csrMapping_hazardFree;
  reg                 CsrPlugin_inWfi /* verilator public */ ;
  reg                 CsrPlugin_thirdPartyWake;
  reg                 CsrPlugin_jumpInterface_valid;
  reg        [31:0]   CsrPlugin_jumpInterface_payload;
  wire                CsrPlugin_exceptionPendings_0;
  wire                CsrPlugin_exceptionPendings_1;
  wire                CsrPlugin_exceptionPendings_2;
  wire                CsrPlugin_exceptionPendings_3;
  wire                contextSwitching;
  reg        [1:0]    CsrPlugin_privilege;
  reg                 CsrPlugin_forceMachineWire;
  reg                 CsrPlugin_selfException_valid;
  reg        [3:0]    CsrPlugin_selfException_payload_code;
  wire       [31:0]   CsrPlugin_selfException_payload_badAddr;
  reg                 CsrPlugin_allowInterrupts;
  reg                 CsrPlugin_allowException;
  reg                 CsrPlugin_allowEbreakException;
  reg                 CsrPlugin_xretAwayFromMachine;
  reg                 IBusCachedPlugin_injectionPort_valid;
  reg                 IBusCachedPlugin_injectionPort_ready;
  wire       [31:0]   IBusCachedPlugin_injectionPort_payload;
  wire                BranchPlugin_jumpInterface_valid;
  wire       [31:0]   BranchPlugin_jumpInterface_payload;
  wire                BranchPlugin_branchExceptionPort_valid;
  wire       [3:0]    BranchPlugin_branchExceptionPort_payload_code;
  wire       [31:0]   BranchPlugin_branchExceptionPort_payload_badAddr;
  reg                 BranchPlugin_inDebugNoFetchFlag;
  wire                IBusCachedPlugin_externalFlush;
  wire                IBusCachedPlugin_jump_pcLoad_valid;
  wire       [31:0]   IBusCachedPlugin_jump_pcLoad_payload;
  wire       [3:0]    _zz_IBusCachedPlugin_jump_pcLoad_payload;
  wire       [3:0]    _zz_IBusCachedPlugin_jump_pcLoad_payload_1;
  wire                _zz_IBusCachedPlugin_jump_pcLoad_payload_2;
  wire                _zz_IBusCachedPlugin_jump_pcLoad_payload_3;
  wire                _zz_IBusCachedPlugin_jump_pcLoad_payload_4;
  wire                IBusCachedPlugin_fetchPc_output_valid;
  wire                IBusCachedPlugin_fetchPc_output_ready;
  wire       [31:0]   IBusCachedPlugin_fetchPc_output_payload;
  reg        [31:0]   IBusCachedPlugin_fetchPc_pcReg /* verilator public */ ;
  reg                 IBusCachedPlugin_fetchPc_correction;
  reg                 IBusCachedPlugin_fetchPc_correctionReg;
  wire                IBusCachedPlugin_fetchPc_output_fire;
  wire                IBusCachedPlugin_fetchPc_corrected;
  reg                 IBusCachedPlugin_fetchPc_pcRegPropagate;
  reg                 IBusCachedPlugin_fetchPc_booted;
  reg                 IBusCachedPlugin_fetchPc_inc;
  wire                when_Fetcher_l134;
  wire                IBusCachedPlugin_fetchPc_output_fire_1;
  wire                when_Fetcher_l134_1;
  reg        [31:0]   IBusCachedPlugin_fetchPc_pc;
  wire                IBusCachedPlugin_fetchPc_redo_valid;
  wire       [31:0]   IBusCachedPlugin_fetchPc_redo_payload;
  reg                 IBusCachedPlugin_fetchPc_flushed;
  wire                when_Fetcher_l161;
  reg                 IBusCachedPlugin_iBusRsp_redoFetch;
  wire                IBusCachedPlugin_iBusRsp_stages_0_input_valid;
  wire                IBusCachedPlugin_iBusRsp_stages_0_input_ready;
  wire       [31:0]   IBusCachedPlugin_iBusRsp_stages_0_input_payload;
  wire                IBusCachedPlugin_iBusRsp_stages_0_output_valid;
  wire                IBusCachedPlugin_iBusRsp_stages_0_output_ready;
  wire       [31:0]   IBusCachedPlugin_iBusRsp_stages_0_output_payload;
  reg                 IBusCachedPlugin_iBusRsp_stages_0_halt;
  wire                IBusCachedPlugin_iBusRsp_stages_1_input_valid;
  wire                IBusCachedPlugin_iBusRsp_stages_1_input_ready;
  wire       [31:0]   IBusCachedPlugin_iBusRsp_stages_1_input_payload;
  wire                IBusCachedPlugin_iBusRsp_stages_1_output_valid;
  wire                IBusCachedPlugin_iBusRsp_stages_1_output_ready;
  wire       [31:0]   IBusCachedPlugin_iBusRsp_stages_1_output_payload;
  reg                 IBusCachedPlugin_iBusRsp_stages_1_halt;
  wire                IBusCachedPlugin_iBusRsp_stages_2_input_valid;
  wire                IBusCachedPlugin_iBusRsp_stages_2_input_ready;
  wire       [31:0]   IBusCachedPlugin_iBusRsp_stages_2_input_payload;
  wire                IBusCachedPlugin_iBusRsp_stages_2_output_valid;
  wire                IBusCachedPlugin_iBusRsp_stages_2_output_ready;
  wire       [31:0]   IBusCachedPlugin_iBusRsp_stages_2_output_payload;
  reg                 IBusCachedPlugin_iBusRsp_stages_2_halt;
  wire                _zz_IBusCachedPlugin_iBusRsp_stages_0_input_ready;
  wire                _zz_IBusCachedPlugin_iBusRsp_stages_1_input_ready;
  wire                _zz_IBusCachedPlugin_iBusRsp_stages_2_input_ready;
  wire                IBusCachedPlugin_iBusRsp_flush;
  wire                _zz_IBusCachedPlugin_iBusRsp_stages_0_output_ready;
  wire                _zz_IBusCachedPlugin_iBusRsp_stages_1_input_valid;
  reg                 _zz_IBusCachedPlugin_iBusRsp_stages_1_input_valid_1;
  wire                IBusCachedPlugin_iBusRsp_stages_1_output_m2sPipe_valid;
  wire                IBusCachedPlugin_iBusRsp_stages_1_output_m2sPipe_ready;
  wire       [31:0]   IBusCachedPlugin_iBusRsp_stages_1_output_m2sPipe_payload;
  reg                 _zz_IBusCachedPlugin_iBusRsp_stages_1_output_m2sPipe_valid;
  reg        [31:0]   _zz_IBusCachedPlugin_iBusRsp_stages_1_output_m2sPipe_payload;
  reg                 IBusCachedPlugin_iBusRsp_readyForError;
  wire                IBusCachedPlugin_iBusRsp_output_valid;
  wire                IBusCachedPlugin_iBusRsp_output_ready;
  wire       [31:0]   IBusCachedPlugin_iBusRsp_output_payload_pc;
  wire                IBusCachedPlugin_iBusRsp_output_payload_rsp_error;
  wire       [31:0]   IBusCachedPlugin_iBusRsp_output_payload_rsp_inst;
  wire                IBusCachedPlugin_iBusRsp_output_payload_isRvc;
  wire                when_Fetcher_l243;
  wire                when_Fetcher_l323;
  reg                 IBusCachedPlugin_injector_nextPcCalc_valids_0;
  wire                when_Fetcher_l332;
  reg                 IBusCachedPlugin_injector_nextPcCalc_valids_1;
  wire                when_Fetcher_l332_1;
  reg                 IBusCachedPlugin_injector_nextPcCalc_valids_2;
  wire                when_Fetcher_l332_2;
  reg                 IBusCachedPlugin_injector_nextPcCalc_valids_3;
  wire                when_Fetcher_l332_3;
  reg                 IBusCachedPlugin_injector_nextPcCalc_valids_4;
  wire                when_Fetcher_l332_4;
  wire                _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch;
  reg        [18:0]   _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch_1;
  wire                _zz_3;
  reg        [10:0]   _zz_4;
  wire                _zz_5;
  reg        [18:0]   _zz_6;
  reg                 _zz_7;
  wire                _zz_IBusCachedPlugin_predictionJumpInterface_payload;
  reg        [10:0]   _zz_IBusCachedPlugin_predictionJumpInterface_payload_1;
  wire                _zz_IBusCachedPlugin_predictionJumpInterface_payload_2;
  reg        [18:0]   _zz_IBusCachedPlugin_predictionJumpInterface_payload_3;
  reg        [31:0]   IBusCachedPlugin_rspCounter;
  wire                IBusCachedPlugin_s0_tightlyCoupledHit;
  reg                 IBusCachedPlugin_s1_tightlyCoupledHit;
  reg                 IBusCachedPlugin_s2_tightlyCoupledHit;
  wire                IBusCachedPlugin_rsp_iBusRspOutputHalt;
  wire                IBusCachedPlugin_rsp_issueDetected;
  reg                 IBusCachedPlugin_rsp_redoFetch;
  wire                when_IBusCachedPlugin_l239;
  wire                when_IBusCachedPlugin_l244;
  wire                when_IBusCachedPlugin_l250;
  wire                when_IBusCachedPlugin_l256;
  wire                when_IBusCachedPlugin_l267;
  reg        [31:0]   DBusCachedPlugin_rspCounter;
  wire                when_DBusCachedPlugin_l308;
  wire       [1:0]    execute_DBusCachedPlugin_size;
  reg        [31:0]   _zz_execute_MEMORY_STORE_DATA_RF;
  wire                toplevel_dataCache_1_io_cpu_flush_isStall;
  wire                when_DBusCachedPlugin_l350;
  wire                when_DBusCachedPlugin_l366;
  wire                when_DBusCachedPlugin_l393;
  wire                when_DBusCachedPlugin_l446;
  wire                when_DBusCachedPlugin_l466;
  wire       [7:0]    writeBack_DBusCachedPlugin_rspSplits_0;
  wire       [7:0]    writeBack_DBusCachedPlugin_rspSplits_1;
  wire       [7:0]    writeBack_DBusCachedPlugin_rspSplits_2;
  wire       [7:0]    writeBack_DBusCachedPlugin_rspSplits_3;
  reg        [31:0]   writeBack_DBusCachedPlugin_rspShifted;
  wire       [31:0]   writeBack_DBusCachedPlugin_rspRf;
  wire       [1:0]    switch_Misc_l226;
  wire                _zz_writeBack_DBusCachedPlugin_rspFormated;
  reg        [31:0]   _zz_writeBack_DBusCachedPlugin_rspFormated_1;
  wire                _zz_writeBack_DBusCachedPlugin_rspFormated_2;
  reg        [31:0]   _zz_writeBack_DBusCachedPlugin_rspFormated_3;
  reg        [31:0]   writeBack_DBusCachedPlugin_rspFormated;
  wire                when_DBusCachedPlugin_l492;
  reg        [7:0]    PmpPlugin_pmpcfg_0;
  reg        [7:0]    PmpPlugin_pmpcfg_1;
  reg        [7:0]    PmpPlugin_pmpcfg_2;
  reg        [7:0]    PmpPlugin_pmpcfg_3;
  reg        [7:0]    PmpPlugin_pmpcfg_4;
  reg        [7:0]    PmpPlugin_pmpcfg_5;
  reg        [7:0]    PmpPlugin_pmpcfg_6;
  reg        [7:0]    PmpPlugin_pmpcfg_7;
  reg        [7:0]    PmpPlugin_pmpcfg_8;
  reg        [7:0]    PmpPlugin_pmpcfg_9;
  reg        [7:0]    PmpPlugin_pmpcfg_10;
  reg        [7:0]    PmpPlugin_pmpcfg_11;
  reg        [7:0]    PmpPlugin_pmpcfg_12;
  reg        [7:0]    PmpPlugin_pmpcfg_13;
  reg        [7:0]    PmpPlugin_pmpcfg_14;
  reg        [7:0]    PmpPlugin_pmpcfg_15;
  reg        [27:0]   PmpPlugin_base_0;
  reg        [27:0]   PmpPlugin_base_1;
  reg        [27:0]   PmpPlugin_base_2;
  reg        [27:0]   PmpPlugin_base_3;
  reg        [27:0]   PmpPlugin_base_4;
  reg        [27:0]   PmpPlugin_base_5;
  reg        [27:0]   PmpPlugin_base_6;
  reg        [27:0]   PmpPlugin_base_7;
  reg        [27:0]   PmpPlugin_base_8;
  reg        [27:0]   PmpPlugin_base_9;
  reg        [27:0]   PmpPlugin_base_10;
  reg        [27:0]   PmpPlugin_base_11;
  reg        [27:0]   PmpPlugin_base_12;
  reg        [27:0]   PmpPlugin_base_13;
  reg        [27:0]   PmpPlugin_base_14;
  reg        [27:0]   PmpPlugin_base_15;
  reg        [27:0]   PmpPlugin_mask_0;
  reg        [27:0]   PmpPlugin_mask_1;
  reg        [27:0]   PmpPlugin_mask_2;
  reg        [27:0]   PmpPlugin_mask_3;
  reg        [27:0]   PmpPlugin_mask_4;
  reg        [27:0]   PmpPlugin_mask_5;
  reg        [27:0]   PmpPlugin_mask_6;
  reg        [27:0]   PmpPlugin_mask_7;
  reg        [27:0]   PmpPlugin_mask_8;
  reg        [27:0]   PmpPlugin_mask_9;
  reg        [27:0]   PmpPlugin_mask_10;
  reg        [27:0]   PmpPlugin_mask_11;
  reg        [27:0]   PmpPlugin_mask_12;
  reg        [27:0]   PmpPlugin_mask_13;
  reg        [27:0]   PmpPlugin_mask_14;
  reg        [27:0]   PmpPlugin_mask_15;
  reg                 execute_PmpPlugin_fsmPending;
  wire                when_PmpPlugin_l138;
  reg                 execute_PmpPlugin_fsmComplete;
  wire       [11:0]   execute_PmpPlugin_csrAddress;
  wire       [3:0]    execute_PmpPlugin_pmpNcfg;
  wire       [1:0]    execute_PmpPlugin_pmpcfgN;
  wire                execute_PmpPlugin_pmpcfgCsr;
  wire                execute_PmpPlugin_pmpaddrCsr;
  reg        [3:0]    execute_PmpPlugin_pmpNcfg_;
  reg        [1:0]    execute_PmpPlugin_pmpcfgN_;
  reg                 execute_PmpPlugin_pmpcfgCsr_;
  reg                 execute_PmpPlugin_pmpaddrCsr_;
  reg        [31:0]   execute_PmpPlugin_writeData_;
  wire                execute_PmpPlugin_fsm_wantExit;
  reg                 execute_PmpPlugin_fsm_wantStart;
  wire                execute_PmpPlugin_fsm_wantKill;
  reg                 execute_PmpPlugin_fsm_fsmEnable;
  reg        [3:0]    execute_PmpPlugin_fsm_fsmCounter;
  wire                when_PmpPlugin_l246;
  wire       [15:0]   _zz_8;
  wire       [15:0]   _zz_9;
  wire       [27:0]   _zz_PmpPlugin_dGuard_hits_0;
  wire                PmpPlugin_dGuard_hits_0;
  wire                PmpPlugin_dGuard_hits_1;
  wire                PmpPlugin_dGuard_hits_2;
  wire                PmpPlugin_dGuard_hits_3;
  wire                PmpPlugin_dGuard_hits_4;
  wire                PmpPlugin_dGuard_hits_5;
  wire                PmpPlugin_dGuard_hits_6;
  wire                PmpPlugin_dGuard_hits_7;
  wire                PmpPlugin_dGuard_hits_8;
  wire                PmpPlugin_dGuard_hits_9;
  wire                PmpPlugin_dGuard_hits_10;
  wire                PmpPlugin_dGuard_hits_11;
  wire                PmpPlugin_dGuard_hits_12;
  wire                PmpPlugin_dGuard_hits_13;
  wire                PmpPlugin_dGuard_hits_14;
  wire                PmpPlugin_dGuard_hits_15;
  wire                when_PmpPlugin_l277;
  wire       [15:0]   _zz_DBusCachedPlugin_mmuBus_rsp_allowRead;
  wire       [15:0]   _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_1;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_2;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_3;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_4;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_5;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_6;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_7;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_8;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_9;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_10;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_11;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_12;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_13;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_14;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_15;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_16;
  wire       [15:0]   _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite;
  wire       [15:0]   _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_1;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_2;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_3;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_4;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_5;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_6;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_7;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_8;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_9;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_10;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_11;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_12;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_13;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_14;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_15;
  wire                _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_16;
  wire       [27:0]   _zz_PmpPlugin_iGuard_hits_0;
  wire                PmpPlugin_iGuard_hits_0;
  wire                PmpPlugin_iGuard_hits_1;
  wire                PmpPlugin_iGuard_hits_2;
  wire                PmpPlugin_iGuard_hits_3;
  wire                PmpPlugin_iGuard_hits_4;
  wire                PmpPlugin_iGuard_hits_5;
  wire                PmpPlugin_iGuard_hits_6;
  wire                PmpPlugin_iGuard_hits_7;
  wire                PmpPlugin_iGuard_hits_8;
  wire                PmpPlugin_iGuard_hits_9;
  wire                PmpPlugin_iGuard_hits_10;
  wire                PmpPlugin_iGuard_hits_11;
  wire                PmpPlugin_iGuard_hits_12;
  wire                PmpPlugin_iGuard_hits_13;
  wire                PmpPlugin_iGuard_hits_14;
  wire                PmpPlugin_iGuard_hits_15;
  wire                when_PmpPlugin_l299;
  wire       [15:0]   _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute;
  wire       [15:0]   _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_1;
  wire                _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_2;
  wire                _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_3;
  wire                _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_4;
  wire                _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_5;
  wire                _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_6;
  wire                _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_7;
  wire                _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_8;
  wire                _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_9;
  wire                _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_10;
  wire                _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_11;
  wire                _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_12;
  wire                _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_13;
  wire                _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_14;
  wire                _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_15;
  wire                _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_16;
  wire       [32:0]   _zz_decode_IS_CSR;
  wire                _zz_decode_IS_CSR_1;
  wire                _zz_decode_IS_CSR_2;
  wire                _zz_decode_IS_CSR_3;
  wire                _zz_decode_IS_CSR_4;
  wire                _zz_decode_IS_CSR_5;
  wire                _zz_decode_IS_CSR_6;
  wire                _zz_decode_IS_CSR_7;
  wire       [1:0]    _zz_decode_SRC1_CTRL_2;
  wire       [1:0]    _zz_decode_ALU_CTRL_2;
  wire       [1:0]    _zz_decode_SRC2_CTRL_2;
  wire       [1:0]    _zz_decode_ALU_BITWISE_CTRL_2;
  wire       [1:0]    _zz_decode_SHIFT_CTRL_2;
  wire       [1:0]    _zz_decode_ENV_CTRL_2;
  wire       [1:0]    _zz_decode_BRANCH_CTRL_2;
  wire                when_RegFilePlugin_l63;
  wire       [4:0]    decode_RegFilePlugin_regFileReadAddress1;
  wire       [4:0]    decode_RegFilePlugin_regFileReadAddress2;
  wire       [31:0]   decode_RegFilePlugin_rs1Data;
  wire       [31:0]   decode_RegFilePlugin_rs2Data;
  reg                 lastStageRegFileWrite_valid /* verilator public */ ;
  reg        [4:0]    lastStageRegFileWrite_payload_address /* verilator public */ ;
  reg        [31:0]   lastStageRegFileWrite_payload_data /* verilator public */ ;
  reg                 _zz_10;
  reg        [31:0]   execute_IntAluPlugin_bitwise;
  reg        [31:0]   _zz_execute_REGFILE_WRITE_DATA;
  reg        [31:0]   _zz_execute_SRC1;
  wire                _zz_execute_SRC2;
  reg        [19:0]   _zz_execute_SRC2_1;
  wire                _zz_execute_SRC2_2;
  reg        [19:0]   _zz_execute_SRC2_3;
  reg        [31:0]   _zz_execute_SRC2_4;
  reg        [31:0]   execute_SrcPlugin_addSub;
  wire                execute_SrcPlugin_less;
  wire       [4:0]    execute_FullBarrelShifterPlugin_amplitude;
  reg        [31:0]   _zz_execute_FullBarrelShifterPlugin_reversed;
  wire       [31:0]   execute_FullBarrelShifterPlugin_reversed;
  reg        [31:0]   _zz_decode_RS2_3;
  reg                 HazardSimplePlugin_src0Hazard;
  reg                 HazardSimplePlugin_src1Hazard;
  wire                HazardSimplePlugin_writeBackWrites_valid;
  wire       [4:0]    HazardSimplePlugin_writeBackWrites_payload_address;
  wire       [31:0]   HazardSimplePlugin_writeBackWrites_payload_data;
  reg                 HazardSimplePlugin_writeBackBuffer_valid;
  reg        [4:0]    HazardSimplePlugin_writeBackBuffer_payload_address;
  reg        [31:0]   HazardSimplePlugin_writeBackBuffer_payload_data;
  wire                HazardSimplePlugin_addr0Match;
  wire                HazardSimplePlugin_addr1Match;
  wire                when_HazardSimplePlugin_l47;
  wire                when_HazardSimplePlugin_l48;
  wire                when_HazardSimplePlugin_l51;
  wire                when_HazardSimplePlugin_l45;
  wire                when_HazardSimplePlugin_l57;
  wire                when_HazardSimplePlugin_l58;
  wire                when_HazardSimplePlugin_l48_1;
  wire                when_HazardSimplePlugin_l51_1;
  wire                when_HazardSimplePlugin_l45_1;
  wire                when_HazardSimplePlugin_l57_1;
  wire                when_HazardSimplePlugin_l58_1;
  wire                when_HazardSimplePlugin_l48_2;
  wire                when_HazardSimplePlugin_l51_2;
  wire                when_HazardSimplePlugin_l45_2;
  wire                when_HazardSimplePlugin_l57_2;
  wire                when_HazardSimplePlugin_l58_2;
  wire                when_HazardSimplePlugin_l105;
  wire                when_HazardSimplePlugin_l108;
  wire                when_HazardSimplePlugin_l113;
  reg        [32:0]   memory_MulDivIterativePlugin_rs1;
  reg        [31:0]   memory_MulDivIterativePlugin_rs2;
  reg        [64:0]   memory_MulDivIterativePlugin_accumulator;
  wire                memory_MulDivIterativePlugin_frontendOk;
  reg                 memory_MulDivIterativePlugin_mul_counter_willIncrement;
  reg                 memory_MulDivIterativePlugin_mul_counter_willClear;
  reg        [5:0]    memory_MulDivIterativePlugin_mul_counter_valueNext;
  reg        [5:0]    memory_MulDivIterativePlugin_mul_counter_value;
  wire                memory_MulDivIterativePlugin_mul_counter_willOverflowIfInc;
  wire                memory_MulDivIterativePlugin_mul_counter_willOverflow;
  wire                when_MulDivIterativePlugin_l96;
  wire                when_MulDivIterativePlugin_l97;
  wire                when_MulDivIterativePlugin_l100;
  wire                when_MulDivIterativePlugin_l110;
  reg                 memory_MulDivIterativePlugin_div_needRevert;
  reg                 memory_MulDivIterativePlugin_div_counter_willIncrement;
  reg                 memory_MulDivIterativePlugin_div_counter_willClear;
  reg        [5:0]    memory_MulDivIterativePlugin_div_counter_valueNext;
  reg        [5:0]    memory_MulDivIterativePlugin_div_counter_value;
  wire                memory_MulDivIterativePlugin_div_counter_willOverflowIfInc;
  wire                memory_MulDivIterativePlugin_div_counter_willOverflow;
  reg                 memory_MulDivIterativePlugin_div_done;
  wire                when_MulDivIterativePlugin_l126;
  wire                when_MulDivIterativePlugin_l126_1;
  reg        [31:0]   memory_MulDivIterativePlugin_div_result;
  wire                when_MulDivIterativePlugin_l128;
  wire                when_MulDivIterativePlugin_l129;
  wire                when_MulDivIterativePlugin_l132;
  wire       [31:0]   _zz_memory_MulDivIterativePlugin_div_stage_0_remainderShifted;
  wire       [32:0]   memory_MulDivIterativePlugin_div_stage_0_remainderShifted;
  wire       [32:0]   memory_MulDivIterativePlugin_div_stage_0_remainderMinusDenominator;
  wire       [31:0]   memory_MulDivIterativePlugin_div_stage_0_outRemainder;
  wire       [31:0]   memory_MulDivIterativePlugin_div_stage_0_outNumerator;
  wire                when_MulDivIterativePlugin_l151;
  wire       [31:0]   _zz_memory_MulDivIterativePlugin_div_result;
  wire                when_MulDivIterativePlugin_l162;
  wire                _zz_memory_MulDivIterativePlugin_rs2;
  wire                _zz_memory_MulDivIterativePlugin_rs1;
  reg        [32:0]   _zz_memory_MulDivIterativePlugin_rs1_1;
  reg        [1:0]    _zz_CsrPlugin_privilege;
  reg        [1:0]    CsrPlugin_misa_base;
  reg        [25:0]   CsrPlugin_misa_extensions;
  reg        [1:0]    CsrPlugin_mtvec_mode;
  reg        [29:0]   CsrPlugin_mtvec_base;
  reg        [31:0]   CsrPlugin_mepc;
  reg                 CsrPlugin_mstatus_MIE;
  reg                 CsrPlugin_mstatus_MPIE;
  reg        [1:0]    CsrPlugin_mstatus_MPP;
  reg                 CsrPlugin_mip_MEIP;
  reg                 CsrPlugin_mip_MTIP;
  reg                 CsrPlugin_mip_MSIP;
  reg                 CsrPlugin_mie_MEIE;
  reg                 CsrPlugin_mie_MTIE;
  reg                 CsrPlugin_mie_MSIE;
  reg        [31:0]   CsrPlugin_mscratch;
  reg                 CsrPlugin_mcause_interrupt;
  reg        [3:0]    CsrPlugin_mcause_exceptionCode;
  reg        [31:0]   CsrPlugin_mtval;
  reg        [63:0]   CsrPlugin_mcycle;
  reg        [63:0]   CsrPlugin_minstret;
  wire                _zz_when_CsrPlugin_l1222;
  wire                _zz_when_CsrPlugin_l1222_1;
  wire                _zz_when_CsrPlugin_l1222_2;
  reg                 CsrPlugin_exceptionPortCtrl_exceptionValids_decode;
  reg                 CsrPlugin_exceptionPortCtrl_exceptionValids_execute;
  reg                 CsrPlugin_exceptionPortCtrl_exceptionValids_memory;
  reg                 CsrPlugin_exceptionPortCtrl_exceptionValids_writeBack;
  reg                 CsrPlugin_exceptionPortCtrl_exceptionValidsRegs_decode;
  reg                 CsrPlugin_exceptionPortCtrl_exceptionValidsRegs_execute;
  reg                 CsrPlugin_exceptionPortCtrl_exceptionValidsRegs_memory;
  reg                 CsrPlugin_exceptionPortCtrl_exceptionValidsRegs_writeBack;
  reg        [3:0]    CsrPlugin_exceptionPortCtrl_exceptionContext_code;
  reg        [31:0]   CsrPlugin_exceptionPortCtrl_exceptionContext_badAddr;
  wire       [1:0]    CsrPlugin_exceptionPortCtrl_exceptionTargetPrivilegeUncapped;
  wire       [1:0]    CsrPlugin_exceptionPortCtrl_exceptionTargetPrivilege;
  wire       [1:0]    _zz_CsrPlugin_exceptionPortCtrl_exceptionContext_code;
  wire                _zz_CsrPlugin_exceptionPortCtrl_exceptionContext_code_1;
  wire                when_CsrPlugin_l1179;
  wire                when_CsrPlugin_l1179_1;
  wire                when_CsrPlugin_l1179_2;
  wire                when_CsrPlugin_l1179_3;
  wire                when_CsrPlugin_l1192;
  reg                 CsrPlugin_interrupt_valid;
  reg        [3:0]    CsrPlugin_interrupt_code /* verilator public */ ;
  reg        [1:0]    CsrPlugin_interrupt_targetPrivilege;
  wire                when_CsrPlugin_l1216;
  wire                when_CsrPlugin_l1222;
  wire                when_CsrPlugin_l1222_1;
  wire                when_CsrPlugin_l1222_2;
  wire                CsrPlugin_exception;
  reg                 CsrPlugin_lastStageWasWfi;
  reg                 CsrPlugin_pipelineLiberator_pcValids_0;
  reg                 CsrPlugin_pipelineLiberator_pcValids_1;
  reg                 CsrPlugin_pipelineLiberator_pcValids_2;
  wire                CsrPlugin_pipelineLiberator_active;
  wire                when_CsrPlugin_l1255;
  wire                when_CsrPlugin_l1255_1;
  wire                when_CsrPlugin_l1255_2;
  wire                when_CsrPlugin_l1260;
  reg                 CsrPlugin_pipelineLiberator_done;
  wire                when_CsrPlugin_l1266;
  wire                CsrPlugin_interruptJump /* verilator public */ ;
  reg                 CsrPlugin_hadException /* verilator public */ ;
  reg        [1:0]    CsrPlugin_targetPrivilege;
  reg        [3:0]    CsrPlugin_trapCause;
  wire                CsrPlugin_trapCauseEbreakDebug;
  reg        [1:0]    CsrPlugin_xtvec_mode;
  reg        [29:0]   CsrPlugin_xtvec_base;
  wire                CsrPlugin_trapEnterDebug;
  wire                when_CsrPlugin_l1310;
  wire                when_CsrPlugin_l1318;
  wire                when_CsrPlugin_l1376;
  wire       [1:0]    switch_CsrPlugin_l1380;
  wire                when_CsrPlugin_l1388;
  reg                 execute_CsrPlugin_wfiWake;
  wire                when_CsrPlugin_l1439;
  wire                when_CsrPlugin_l1441;
  wire                when_CsrPlugin_l1447;
  wire                execute_CsrPlugin_blockedBySideEffects;
  reg                 execute_CsrPlugin_illegalAccess;
  reg                 execute_CsrPlugin_illegalInstruction;
  wire                when_CsrPlugin_l1460;
  wire                when_CsrPlugin_l1467;
  wire                when_CsrPlugin_l1468;
  wire                when_CsrPlugin_l1475;
  reg                 execute_CsrPlugin_writeInstruction;
  reg                 execute_CsrPlugin_readInstruction;
  wire                execute_CsrPlugin_writeEnable;
  wire                execute_CsrPlugin_readEnable;
  wire       [31:0]   execute_CsrPlugin_readToWriteData;
  wire                switch_Misc_l226_1;
  reg        [31:0]   _zz_CsrPlugin_csrMapping_writeDataSignal;
  wire                when_CsrPlugin_l1507;
  wire                when_CsrPlugin_l1511;
  wire       [11:0]   execute_CsrPlugin_csrAddress;
  reg                 DebugPlugin_firstCycle;
  reg                 DebugPlugin_secondCycle;
  reg                 DebugPlugin_resetIt;
  reg                 DebugPlugin_haltIt;
  reg                 DebugPlugin_stepIt;
  reg                 DebugPlugin_isPipBusy;
  reg                 DebugPlugin_godmode;
  wire                when_DebugPlugin_l238;
  reg                 DebugPlugin_haltedByBreak;
  reg                 DebugPlugin_debugUsed /* verilator public */ ;
  reg                 DebugPlugin_disableEbreak;
  wire                DebugPlugin_allowEBreak;
  reg        [31:0]   DebugPlugin_busReadDataReg;
  reg                 _zz_when_DebugPlugin_l257;
  wire                when_DebugPlugin_l257;
  wire       [5:0]    switch_DebugPlugin_l280;
  wire                when_DebugPlugin_l284;
  wire                when_DebugPlugin_l284_1;
  wire                when_DebugPlugin_l285;
  wire                when_DebugPlugin_l285_1;
  wire                when_DebugPlugin_l286;
  wire                when_DebugPlugin_l287;
  wire                when_DebugPlugin_l288;
  wire                when_DebugPlugin_l288_1;
  wire                when_DebugPlugin_l308;
  wire                when_DebugPlugin_l311;
  wire                when_DebugPlugin_l324;
  reg                 DebugPlugin_resetIt_regNext;
  wire                when_DebugPlugin_l344;
  wire                execute_BranchPlugin_eq;
  wire       [2:0]    switch_Misc_l226_2;
  reg                 _zz_execute_BRANCH_COND_RESULT;
  reg                 _zz_execute_BRANCH_COND_RESULT_1;
  wire                _zz_execute_BranchPlugin_missAlignedTarget;
  reg        [19:0]   _zz_execute_BranchPlugin_missAlignedTarget_1;
  wire                _zz_execute_BranchPlugin_missAlignedTarget_2;
  reg        [10:0]   _zz_execute_BranchPlugin_missAlignedTarget_3;
  wire                _zz_execute_BranchPlugin_missAlignedTarget_4;
  reg        [18:0]   _zz_execute_BranchPlugin_missAlignedTarget_5;
  reg                 _zz_execute_BranchPlugin_missAlignedTarget_6;
  wire                execute_BranchPlugin_missAlignedTarget;
  reg        [31:0]   execute_BranchPlugin_branch_src1;
  reg        [31:0]   execute_BranchPlugin_branch_src2;
  wire                _zz_execute_BranchPlugin_branch_src2;
  reg        [19:0]   _zz_execute_BranchPlugin_branch_src2_1;
  wire                _zz_execute_BranchPlugin_branch_src2_2;
  reg        [10:0]   _zz_execute_BranchPlugin_branch_src2_3;
  wire                _zz_execute_BranchPlugin_branch_src2_4;
  reg        [18:0]   _zz_execute_BranchPlugin_branch_src2_5;
  wire       [31:0]   execute_BranchPlugin_branchAdder;
  wire                when_Pipeline_l124;
  reg        [31:0]   decode_to_execute_PC;
  wire                when_Pipeline_l124_1;
  reg        [31:0]   execute_to_memory_PC;
  wire                when_Pipeline_l124_2;
  reg        [31:0]   memory_to_writeBack_PC;
  wire                when_Pipeline_l124_3;
  reg        [31:0]   decode_to_execute_INSTRUCTION;
  wire                when_Pipeline_l124_4;
  reg        [31:0]   execute_to_memory_INSTRUCTION;
  wire                when_Pipeline_l124_5;
  reg        [31:0]   memory_to_writeBack_INSTRUCTION;
  wire                when_Pipeline_l124_6;
  reg        [31:0]   decode_to_execute_FORMAL_PC_NEXT;
  wire                when_Pipeline_l124_7;
  reg        [31:0]   execute_to_memory_FORMAL_PC_NEXT;
  wire                when_Pipeline_l124_8;
  reg        [31:0]   memory_to_writeBack_FORMAL_PC_NEXT;
  wire                when_Pipeline_l124_9;
  reg                 decode_to_execute_MEMORY_FORCE_CONSTISTENCY;
  wire                when_Pipeline_l124_10;
  reg        [1:0]    decode_to_execute_SRC1_CTRL;
  wire                when_Pipeline_l124_11;
  reg                 decode_to_execute_SRC_USE_SUB_LESS;
  wire                when_Pipeline_l124_12;
  reg                 decode_to_execute_MEMORY_ENABLE;
  wire                when_Pipeline_l124_13;
  reg                 execute_to_memory_MEMORY_ENABLE;
  wire                when_Pipeline_l124_14;
  reg                 memory_to_writeBack_MEMORY_ENABLE;
  wire                when_Pipeline_l124_15;
  reg        [1:0]    decode_to_execute_ALU_CTRL;
  wire                when_Pipeline_l124_16;
  reg        [1:0]    decode_to_execute_SRC2_CTRL;
  wire                when_Pipeline_l124_17;
  reg                 decode_to_execute_REGFILE_WRITE_VALID;
  wire                when_Pipeline_l124_18;
  reg                 execute_to_memory_REGFILE_WRITE_VALID;
  wire                when_Pipeline_l124_19;
  reg                 memory_to_writeBack_REGFILE_WRITE_VALID;
  wire                when_Pipeline_l124_20;
  reg                 decode_to_execute_BYPASSABLE_EXECUTE_STAGE;
  wire                when_Pipeline_l124_21;
  reg                 decode_to_execute_BYPASSABLE_MEMORY_STAGE;
  wire                when_Pipeline_l124_22;
  reg                 execute_to_memory_BYPASSABLE_MEMORY_STAGE;
  wire                when_Pipeline_l124_23;
  reg                 decode_to_execute_MEMORY_WR;
  wire                when_Pipeline_l124_24;
  reg                 execute_to_memory_MEMORY_WR;
  wire                when_Pipeline_l124_25;
  reg                 memory_to_writeBack_MEMORY_WR;
  wire                when_Pipeline_l124_26;
  reg                 decode_to_execute_MEMORY_MANAGMENT;
  wire                when_Pipeline_l124_27;
  reg                 decode_to_execute_SRC_LESS_UNSIGNED;
  wire                when_Pipeline_l124_28;
  reg        [1:0]    decode_to_execute_ALU_BITWISE_CTRL;
  wire                when_Pipeline_l124_29;
  reg        [1:0]    decode_to_execute_SHIFT_CTRL;
  wire                when_Pipeline_l124_30;
  reg        [1:0]    execute_to_memory_SHIFT_CTRL;
  wire                when_Pipeline_l124_31;
  reg                 decode_to_execute_IS_MUL;
  wire                when_Pipeline_l124_32;
  reg                 execute_to_memory_IS_MUL;
  wire                when_Pipeline_l124_33;
  reg                 decode_to_execute_IS_RS1_SIGNED;
  wire                when_Pipeline_l124_34;
  reg                 decode_to_execute_IS_RS2_SIGNED;
  wire                when_Pipeline_l124_35;
  reg                 decode_to_execute_IS_DIV;
  wire                when_Pipeline_l124_36;
  reg                 execute_to_memory_IS_DIV;
  wire                when_Pipeline_l124_37;
  reg                 decode_to_execute_IS_CSR;
  wire                when_Pipeline_l124_38;
  reg        [1:0]    decode_to_execute_ENV_CTRL;
  wire                when_Pipeline_l124_39;
  reg        [1:0]    execute_to_memory_ENV_CTRL;
  wire                when_Pipeline_l124_40;
  reg        [1:0]    memory_to_writeBack_ENV_CTRL;
  wire                when_Pipeline_l124_41;
  reg        [1:0]    decode_to_execute_BRANCH_CTRL;
  wire                when_Pipeline_l124_42;
  reg        [31:0]   decode_to_execute_RS1;
  wire                when_Pipeline_l124_43;
  reg        [31:0]   decode_to_execute_RS2;
  wire                when_Pipeline_l124_44;
  reg                 decode_to_execute_SRC2_FORCE_ZERO;
  wire                when_Pipeline_l124_45;
  reg                 decode_to_execute_CSR_WRITE_OPCODE;
  wire                when_Pipeline_l124_46;
  reg                 decode_to_execute_CSR_READ_OPCODE;
  wire                when_Pipeline_l124_47;
  reg                 decode_to_execute_DO_EBREAK;
  wire                when_Pipeline_l124_48;
  reg                 decode_to_execute_PREDICTION_HAD_BRANCHED2;
  wire                when_Pipeline_l124_49;
  reg        [31:0]   execute_to_memory_MEMORY_STORE_DATA_RF;
  wire                when_Pipeline_l124_50;
  reg        [31:0]   memory_to_writeBack_MEMORY_STORE_DATA_RF;
  wire                when_Pipeline_l124_51;
  reg        [31:0]   execute_to_memory_REGFILE_WRITE_DATA;
  wire                when_Pipeline_l124_52;
  reg        [31:0]   memory_to_writeBack_REGFILE_WRITE_DATA;
  wire                when_Pipeline_l124_53;
  reg        [31:0]   execute_to_memory_SHIFT_RIGHT;
  wire                when_Pipeline_l124_54;
  reg                 execute_to_memory_BRANCH_DO;
  wire                when_Pipeline_l124_55;
  reg        [31:0]   execute_to_memory_BRANCH_CALC;
  wire                when_Pipeline_l151;
  wire                when_Pipeline_l154;
  wire                when_Pipeline_l151_1;
  wire                when_Pipeline_l154_1;
  wire                when_Pipeline_l151_2;
  wire                when_Pipeline_l154_2;
  reg        [2:0]    switch_Fetcher_l365;
  wire                when_Fetcher_l381;
  reg        [2:0]    execute_PmpPlugin_fsm_stateReg;
  reg        [2:0]    execute_PmpPlugin_fsm_stateNext;
  wire       [7:0]    _zz_PmpPlugin_pmpcfg_0;
  wire       [7:0]    _zz_PmpPlugin_pmpcfg_0_1;
  wire       [7:0]    _zz_PmpPlugin_pmpcfg_0_2;
  wire       [7:0]    _zz_PmpPlugin_pmpcfg_0_3;
  wire                when_PmpPlugin_l209;
  wire       [15:0]   _zz_11;
  wire                when_PmpPlugin_l209_1;
  wire       [15:0]   _zz_12;
  wire                when_PmpPlugin_l209_2;
  wire       [15:0]   _zz_13;
  wire                when_PmpPlugin_l209_3;
  wire       [15:0]   _zz_14;
  wire                when_PmpPlugin_l216;
  wire                when_PmpPlugin_l229;
  wire                when_StateMachine_l237;
  wire                when_StateMachine_l253;
  wire                when_StateMachine_l253_1;
  wire                when_StateMachine_l253_2;
  wire                when_CsrPlugin_l1589;
  reg                 execute_CsrPlugin_csr_3857;
  wire                when_CsrPlugin_l1589_1;
  reg                 execute_CsrPlugin_csr_3858;
  wire                when_CsrPlugin_l1589_2;
  reg                 execute_CsrPlugin_csr_3859;
  wire                when_CsrPlugin_l1589_3;
  reg                 execute_CsrPlugin_csr_3860;
  wire                when_CsrPlugin_l1589_4;
  reg                 execute_CsrPlugin_csr_769;
  wire                when_CsrPlugin_l1589_5;
  reg                 execute_CsrPlugin_csr_768;
  wire                when_CsrPlugin_l1589_6;
  reg                 execute_CsrPlugin_csr_836;
  wire                when_CsrPlugin_l1589_7;
  reg                 execute_CsrPlugin_csr_772;
  wire                when_CsrPlugin_l1589_8;
  reg                 execute_CsrPlugin_csr_773;
  wire                when_CsrPlugin_l1589_9;
  reg                 execute_CsrPlugin_csr_833;
  wire                when_CsrPlugin_l1589_10;
  reg                 execute_CsrPlugin_csr_832;
  wire                when_CsrPlugin_l1589_11;
  reg                 execute_CsrPlugin_csr_834;
  wire                when_CsrPlugin_l1589_12;
  reg                 execute_CsrPlugin_csr_835;
  wire                when_CsrPlugin_l1589_13;
  reg                 execute_CsrPlugin_csr_2816;
  wire                when_CsrPlugin_l1589_14;
  reg                 execute_CsrPlugin_csr_2944;
  wire                when_CsrPlugin_l1589_15;
  reg                 execute_CsrPlugin_csr_2818;
  wire                when_CsrPlugin_l1589_16;
  reg                 execute_CsrPlugin_csr_2946;
  wire                when_CsrPlugin_l1589_17;
  reg                 execute_CsrPlugin_csr_3072;
  wire                when_CsrPlugin_l1589_18;
  reg                 execute_CsrPlugin_csr_3200;
  wire                when_CsrPlugin_l1589_19;
  reg                 execute_CsrPlugin_csr_3074;
  wire                when_CsrPlugin_l1589_20;
  reg                 execute_CsrPlugin_csr_3202;
  reg        [31:0]   _zz_CsrPlugin_csrMapping_readDataInit;
  reg        [31:0]   _zz_CsrPlugin_csrMapping_readDataInit_1;
  reg        [31:0]   _zz_CsrPlugin_csrMapping_readDataInit_2;
  reg        [31:0]   _zz_CsrPlugin_csrMapping_readDataInit_3;
  wire       [1:0]    switch_CsrPlugin_l980;
  reg        [31:0]   _zz_CsrPlugin_csrMapping_readDataInit_4;
  reg        [31:0]   _zz_CsrPlugin_csrMapping_readDataInit_5;
  reg        [31:0]   _zz_CsrPlugin_csrMapping_readDataInit_6;
  reg        [31:0]   _zz_CsrPlugin_csrMapping_readDataInit_7;
  reg        [31:0]   _zz_CsrPlugin_csrMapping_readDataInit_8;
  reg        [31:0]   _zz_CsrPlugin_csrMapping_readDataInit_9;
  reg        [31:0]   _zz_CsrPlugin_csrMapping_readDataInit_10;
  reg        [31:0]   _zz_CsrPlugin_csrMapping_readDataInit_11;
  reg        [31:0]   _zz_CsrPlugin_csrMapping_readDataInit_12;
  reg        [31:0]   _zz_CsrPlugin_csrMapping_readDataInit_13;
  reg        [31:0]   _zz_CsrPlugin_csrMapping_readDataInit_14;
  reg        [31:0]   _zz_CsrPlugin_csrMapping_readDataInit_15;
  reg        [31:0]   _zz_CsrPlugin_csrMapping_readDataInit_16;
  reg        [31:0]   _zz_CsrPlugin_csrMapping_readDataInit_17;
  reg        [31:0]   _zz_CsrPlugin_csrMapping_readDataInit_18;
  reg        [31:0]   _zz_CsrPlugin_csrMapping_readDataInit_19;
  wire                when_PmpPlugin_l155;
  wire                when_PmpPlugin_l172;
  wire                when_PmpPlugin_l175;
  reg                 when_CsrPlugin_l1625;
  wire                when_CsrPlugin_l1623;
  wire                when_CsrPlugin_l1631;
  `ifndef SYNTHESIS
  reg [31:0] _zz_decode_to_execute_BRANCH_CTRL_string;
  reg [31:0] _zz_decode_to_execute_BRANCH_CTRL_1_string;
  reg [39:0] _zz_memory_to_writeBack_ENV_CTRL_string;
  reg [39:0] _zz_memory_to_writeBack_ENV_CTRL_1_string;
  reg [39:0] _zz_execute_to_memory_ENV_CTRL_string;
  reg [39:0] _zz_execute_to_memory_ENV_CTRL_1_string;
  reg [39:0] decode_ENV_CTRL_string;
  reg [39:0] _zz_decode_ENV_CTRL_string;
  reg [39:0] _zz_decode_to_execute_ENV_CTRL_string;
  reg [39:0] _zz_decode_to_execute_ENV_CTRL_1_string;
  reg [71:0] _zz_execute_to_memory_SHIFT_CTRL_string;
  reg [71:0] _zz_execute_to_memory_SHIFT_CTRL_1_string;
  reg [71:0] decode_SHIFT_CTRL_string;
  reg [71:0] _zz_decode_SHIFT_CTRL_string;
  reg [71:0] _zz_decode_to_execute_SHIFT_CTRL_string;
  reg [71:0] _zz_decode_to_execute_SHIFT_CTRL_1_string;
  reg [39:0] decode_ALU_BITWISE_CTRL_string;
  reg [39:0] _zz_decode_ALU_BITWISE_CTRL_string;
  reg [39:0] _zz_decode_to_execute_ALU_BITWISE_CTRL_string;
  reg [39:0] _zz_decode_to_execute_ALU_BITWISE_CTRL_1_string;
  reg [23:0] decode_SRC2_CTRL_string;
  reg [23:0] _zz_decode_SRC2_CTRL_string;
  reg [23:0] _zz_decode_to_execute_SRC2_CTRL_string;
  reg [23:0] _zz_decode_to_execute_SRC2_CTRL_1_string;
  reg [63:0] decode_ALU_CTRL_string;
  reg [63:0] _zz_decode_ALU_CTRL_string;
  reg [63:0] _zz_decode_to_execute_ALU_CTRL_string;
  reg [63:0] _zz_decode_to_execute_ALU_CTRL_1_string;
  reg [95:0] decode_SRC1_CTRL_string;
  reg [95:0] _zz_decode_SRC1_CTRL_string;
  reg [95:0] _zz_decode_to_execute_SRC1_CTRL_string;
  reg [95:0] _zz_decode_to_execute_SRC1_CTRL_1_string;
  reg [31:0] execute_BRANCH_CTRL_string;
  reg [31:0] _zz_execute_BRANCH_CTRL_string;
  reg [39:0] memory_ENV_CTRL_string;
  reg [39:0] _zz_memory_ENV_CTRL_string;
  reg [39:0] execute_ENV_CTRL_string;
  reg [39:0] _zz_execute_ENV_CTRL_string;
  reg [39:0] writeBack_ENV_CTRL_string;
  reg [39:0] _zz_writeBack_ENV_CTRL_string;
  reg [71:0] memory_SHIFT_CTRL_string;
  reg [71:0] _zz_memory_SHIFT_CTRL_string;
  reg [71:0] execute_SHIFT_CTRL_string;
  reg [71:0] _zz_execute_SHIFT_CTRL_string;
  reg [23:0] execute_SRC2_CTRL_string;
  reg [23:0] _zz_execute_SRC2_CTRL_string;
  reg [95:0] execute_SRC1_CTRL_string;
  reg [95:0] _zz_execute_SRC1_CTRL_string;
  reg [63:0] execute_ALU_CTRL_string;
  reg [63:0] _zz_execute_ALU_CTRL_string;
  reg [39:0] execute_ALU_BITWISE_CTRL_string;
  reg [39:0] _zz_execute_ALU_BITWISE_CTRL_string;
  reg [31:0] _zz_decode_BRANCH_CTRL_string;
  reg [39:0] _zz_decode_ENV_CTRL_1_string;
  reg [71:0] _zz_decode_SHIFT_CTRL_1_string;
  reg [39:0] _zz_decode_ALU_BITWISE_CTRL_1_string;
  reg [23:0] _zz_decode_SRC2_CTRL_1_string;
  reg [63:0] _zz_decode_ALU_CTRL_1_string;
  reg [95:0] _zz_decode_SRC1_CTRL_1_string;
  reg [31:0] decode_BRANCH_CTRL_string;
  reg [31:0] _zz_decode_BRANCH_CTRL_1_string;
  reg [95:0] _zz_decode_SRC1_CTRL_2_string;
  reg [63:0] _zz_decode_ALU_CTRL_2_string;
  reg [23:0] _zz_decode_SRC2_CTRL_2_string;
  reg [39:0] _zz_decode_ALU_BITWISE_CTRL_2_string;
  reg [71:0] _zz_decode_SHIFT_CTRL_2_string;
  reg [39:0] _zz_decode_ENV_CTRL_2_string;
  reg [31:0] _zz_decode_BRANCH_CTRL_2_string;
  reg [95:0] decode_to_execute_SRC1_CTRL_string;
  reg [63:0] decode_to_execute_ALU_CTRL_string;
  reg [23:0] decode_to_execute_SRC2_CTRL_string;
  reg [39:0] decode_to_execute_ALU_BITWISE_CTRL_string;
  reg [71:0] decode_to_execute_SHIFT_CTRL_string;
  reg [71:0] execute_to_memory_SHIFT_CTRL_string;
  reg [39:0] decode_to_execute_ENV_CTRL_string;
  reg [39:0] execute_to_memory_ENV_CTRL_string;
  reg [39:0] memory_to_writeBack_ENV_CTRL_string;
  reg [31:0] decode_to_execute_BRANCH_CTRL_string;
  reg [79:0] execute_PmpPlugin_fsm_stateReg_string;
  reg [79:0] execute_PmpPlugin_fsm_stateNext_string;
  `endif

  (* ram_style = "distributed" *) reg [31:0] PmpPlugin_pmpaddr [0:15];
  reg [31:0] RegFilePlugin_regFile [0:31] /* verilator public */ ;

  assign _zz_when = ({decodeExceptionPort_valid,IBusCachedPlugin_decodeExceptionPort_valid} != 2'b00);
  assign _zz_execute_SHIFT_RIGHT_1 = ($signed(_zz_execute_SHIFT_RIGHT_2) >>> execute_FullBarrelShifterPlugin_amplitude);
  assign _zz_execute_SHIFT_RIGHT = _zz_execute_SHIFT_RIGHT_1[31 : 0];
  assign _zz_execute_SHIFT_RIGHT_2 = {((execute_SHIFT_CTRL == ShiftCtrlEnum_SRA_1) && execute_FullBarrelShifterPlugin_reversed[31]),execute_FullBarrelShifterPlugin_reversed};
  assign _zz__zz_IBusCachedPlugin_jump_pcLoad_payload_1 = (_zz_IBusCachedPlugin_jump_pcLoad_payload - 4'b0001);
  assign _zz_IBusCachedPlugin_fetchPc_pc_1 = {IBusCachedPlugin_fetchPc_inc,2'b00};
  assign _zz_IBusCachedPlugin_fetchPc_pc = {29'd0, _zz_IBusCachedPlugin_fetchPc_pc_1};
  assign _zz__zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch = {{{decode_INSTRUCTION[31],decode_INSTRUCTION[7]},decode_INSTRUCTION[30 : 25]},decode_INSTRUCTION[11 : 8]};
  assign _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch_2 = {{_zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch_1,{{{decode_INSTRUCTION[31],decode_INSTRUCTION[7]},decode_INSTRUCTION[30 : 25]},decode_INSTRUCTION[11 : 8]}},1'b0};
  assign _zz__zz_3 = {{{decode_INSTRUCTION[31],decode_INSTRUCTION[19 : 12]},decode_INSTRUCTION[20]},decode_INSTRUCTION[30 : 21]};
  assign _zz__zz_5 = {{{decode_INSTRUCTION[31],decode_INSTRUCTION[7]},decode_INSTRUCTION[30 : 25]},decode_INSTRUCTION[11 : 8]};
  assign _zz__zz_7 = {{_zz_4,{{{decode_INSTRUCTION[31],decode_INSTRUCTION[19 : 12]},decode_INSTRUCTION[20]},decode_INSTRUCTION[30 : 21]}},1'b0};
  assign _zz__zz_7_1 = {{_zz_6,{{{decode_INSTRUCTION[31],decode_INSTRUCTION[7]},decode_INSTRUCTION[30 : 25]},decode_INSTRUCTION[11 : 8]}},1'b0};
  assign _zz__zz_IBusCachedPlugin_predictionJumpInterface_payload = {{{decode_INSTRUCTION[31],decode_INSTRUCTION[19 : 12]},decode_INSTRUCTION[20]},decode_INSTRUCTION[30 : 21]};
  assign _zz__zz_IBusCachedPlugin_predictionJumpInterface_payload_2 = {{{decode_INSTRUCTION[31],decode_INSTRUCTION[7]},decode_INSTRUCTION[30 : 25]},decode_INSTRUCTION[11 : 8]};
  assign _zz_io_cpu_flush_payload_lineId = _zz_io_cpu_flush_payload_lineId_1;
  assign _zz_io_cpu_flush_payload_lineId_1 = (execute_RS1 >>> 5);
  assign _zz_DBusCachedPlugin_exceptionBus_payload_code = (writeBack_MEMORY_WR ? 3'b111 : 3'b101);
  assign _zz_DBusCachedPlugin_exceptionBus_payload_code_1 = (writeBack_MEMORY_WR ? 3'b110 : 3'b100);
  assign _zz__zz_DBusCachedPlugin_mmuBus_rsp_allowRead_1_1 = (_zz_DBusCachedPlugin_mmuBus_rsp_allowRead - 16'h0001);
  assign _zz__zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_1_1 = (_zz_DBusCachedPlugin_mmuBus_rsp_allowWrite - 16'h0001);
  assign _zz__zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_1_1 = (_zz_IBusCachedPlugin_mmuBus_rsp_allowExecute - 16'h0001);
  assign _zz__zz_execute_REGFILE_WRITE_DATA = execute_SRC_LESS;
  assign _zz__zz_execute_SRC1 = 3'b100;
  assign _zz__zz_execute_SRC1_1 = execute_INSTRUCTION[19 : 15];
  assign _zz__zz_execute_SRC2_2 = {execute_INSTRUCTION[31 : 25],execute_INSTRUCTION[11 : 7]};
  assign _zz_execute_SrcPlugin_addSub = ($signed(_zz_execute_SrcPlugin_addSub_1) + $signed(_zz_execute_SrcPlugin_addSub_4));
  assign _zz_execute_SrcPlugin_addSub_1 = ($signed(_zz_execute_SrcPlugin_addSub_2) + $signed(_zz_execute_SrcPlugin_addSub_3));
  assign _zz_execute_SrcPlugin_addSub_2 = execute_SRC1;
  assign _zz_execute_SrcPlugin_addSub_3 = (execute_SRC_USE_SUB_LESS ? (~ execute_SRC2) : execute_SRC2);
  assign _zz_execute_SrcPlugin_addSub_4 = (execute_SRC_USE_SUB_LESS ? _zz_execute_SrcPlugin_addSub_5 : _zz_execute_SrcPlugin_addSub_6);
  assign _zz_execute_SrcPlugin_addSub_5 = 32'h00000001;
  assign _zz_execute_SrcPlugin_addSub_6 = 32'h0;
  assign _zz_memory_MulDivIterativePlugin_mul_counter_valueNext_1 = memory_MulDivIterativePlugin_mul_counter_willIncrement;
  assign _zz_memory_MulDivIterativePlugin_mul_counter_valueNext = {5'd0, _zz_memory_MulDivIterativePlugin_mul_counter_valueNext_1};
  assign _zz_memory_MulDivIterativePlugin_accumulator = (_zz_memory_MulDivIterativePlugin_accumulator_1 + _zz_memory_MulDivIterativePlugin_accumulator_3);
  assign _zz_memory_MulDivIterativePlugin_accumulator_2 = (memory_MulDivIterativePlugin_rs2[0] ? memory_MulDivIterativePlugin_rs1 : 33'h0);
  assign _zz_memory_MulDivIterativePlugin_accumulator_1 = {{1{_zz_memory_MulDivIterativePlugin_accumulator_2[32]}}, _zz_memory_MulDivIterativePlugin_accumulator_2};
  assign _zz_memory_MulDivIterativePlugin_accumulator_4 = _zz_memory_MulDivIterativePlugin_accumulator_5;
  assign _zz_memory_MulDivIterativePlugin_accumulator_3 = {{1{_zz_memory_MulDivIterativePlugin_accumulator_4[32]}}, _zz_memory_MulDivIterativePlugin_accumulator_4};
  assign _zz_memory_MulDivIterativePlugin_accumulator_5 = (memory_MulDivIterativePlugin_accumulator >>> 32);
  assign _zz_memory_MulDivIterativePlugin_div_counter_valueNext_1 = memory_MulDivIterativePlugin_div_counter_willIncrement;
  assign _zz_memory_MulDivIterativePlugin_div_counter_valueNext = {5'd0, _zz_memory_MulDivIterativePlugin_div_counter_valueNext_1};
  assign _zz_memory_MulDivIterativePlugin_div_stage_0_remainderMinusDenominator = {1'd0, memory_MulDivIterativePlugin_rs2};
  assign _zz_memory_MulDivIterativePlugin_div_stage_0_outRemainder = memory_MulDivIterativePlugin_div_stage_0_remainderMinusDenominator[31:0];
  assign _zz_memory_MulDivIterativePlugin_div_stage_0_outRemainder_1 = memory_MulDivIterativePlugin_div_stage_0_remainderShifted[31:0];
  assign _zz_memory_MulDivIterativePlugin_div_stage_0_outNumerator = {_zz_memory_MulDivIterativePlugin_div_stage_0_remainderShifted,(! memory_MulDivIterativePlugin_div_stage_0_remainderMinusDenominator[32])};
  assign _zz_memory_MulDivIterativePlugin_div_result_1 = _zz_memory_MulDivIterativePlugin_div_result_2;
  assign _zz_memory_MulDivIterativePlugin_div_result_2 = _zz_memory_MulDivIterativePlugin_div_result_3;
  assign _zz_memory_MulDivIterativePlugin_div_result_3 = ({memory_MulDivIterativePlugin_div_needRevert,(memory_MulDivIterativePlugin_div_needRevert ? (~ _zz_memory_MulDivIterativePlugin_div_result) : _zz_memory_MulDivIterativePlugin_div_result)} + _zz_memory_MulDivIterativePlugin_div_result_4);
  assign _zz_memory_MulDivIterativePlugin_div_result_5 = memory_MulDivIterativePlugin_div_needRevert;
  assign _zz_memory_MulDivIterativePlugin_div_result_4 = {32'd0, _zz_memory_MulDivIterativePlugin_div_result_5};
  assign _zz_memory_MulDivIterativePlugin_rs1_3 = _zz_memory_MulDivIterativePlugin_rs1;
  assign _zz_memory_MulDivIterativePlugin_rs1_2 = {32'd0, _zz_memory_MulDivIterativePlugin_rs1_3};
  assign _zz_memory_MulDivIterativePlugin_rs2_2 = _zz_memory_MulDivIterativePlugin_rs2;
  assign _zz_memory_MulDivIterativePlugin_rs2_1 = {31'd0, _zz_memory_MulDivIterativePlugin_rs2_2};
  assign _zz__zz_CsrPlugin_exceptionPortCtrl_exceptionContext_code_1 = (_zz_CsrPlugin_exceptionPortCtrl_exceptionContext_code & (~ _zz__zz_CsrPlugin_exceptionPortCtrl_exceptionContext_code_1_1));
  assign _zz__zz_CsrPlugin_exceptionPortCtrl_exceptionContext_code_1_1 = (_zz_CsrPlugin_exceptionPortCtrl_exceptionContext_code - 2'b01);
  assign _zz__zz_execute_BranchPlugin_missAlignedTarget_2 = {{{execute_INSTRUCTION[31],execute_INSTRUCTION[19 : 12]},execute_INSTRUCTION[20]},execute_INSTRUCTION[30 : 21]};
  assign _zz__zz_execute_BranchPlugin_missAlignedTarget_4 = {{{execute_INSTRUCTION[31],execute_INSTRUCTION[7]},execute_INSTRUCTION[30 : 25]},execute_INSTRUCTION[11 : 8]};
  assign _zz__zz_execute_BranchPlugin_missAlignedTarget_6 = {_zz_execute_BranchPlugin_missAlignedTarget_1,execute_INSTRUCTION[31 : 20]};
  assign _zz__zz_execute_BranchPlugin_missAlignedTarget_6_1 = {{_zz_execute_BranchPlugin_missAlignedTarget_3,{{{execute_INSTRUCTION[31],execute_INSTRUCTION[19 : 12]},execute_INSTRUCTION[20]},execute_INSTRUCTION[30 : 21]}},1'b0};
  assign _zz__zz_execute_BranchPlugin_missAlignedTarget_6_2 = {{_zz_execute_BranchPlugin_missAlignedTarget_5,{{{execute_INSTRUCTION[31],execute_INSTRUCTION[7]},execute_INSTRUCTION[30 : 25]},execute_INSTRUCTION[11 : 8]}},1'b0};
  assign _zz__zz_execute_BranchPlugin_branch_src2_2 = {{{execute_INSTRUCTION[31],execute_INSTRUCTION[19 : 12]},execute_INSTRUCTION[20]},execute_INSTRUCTION[30 : 21]};
  assign _zz__zz_execute_BranchPlugin_branch_src2_4 = {{{execute_INSTRUCTION[31],execute_INSTRUCTION[7]},execute_INSTRUCTION[30 : 25]},execute_INSTRUCTION[11 : 8]};
  assign _zz_execute_BranchPlugin_branch_src2_9 = 3'b100;
  assign _zz_PmpPlugin_pmpaddr_port = execute_PmpPlugin_writeData_;
  assign _zz_decode_RegFilePlugin_rs1Data = 1'b1;
  assign _zz_decode_RegFilePlugin_rs2Data = 1'b1;
  assign _zz_IBusCachedPlugin_jump_pcLoad_payload_6 = {_zz_IBusCachedPlugin_jump_pcLoad_payload_4,_zz_IBusCachedPlugin_jump_pcLoad_payload_3};
  assign _zz_writeBack_DBusCachedPlugin_rspShifted_1 = dataCache_1_io_cpu_writeBack_address[1 : 0];
  assign _zz_writeBack_DBusCachedPlugin_rspShifted_3 = dataCache_1_io_cpu_writeBack_address[1 : 1];
  assign _zz_PmpPlugin_dGuard_hits_0_2 = 4'b0000;
  assign _zz_PmpPlugin_dGuard_hits_0_4 = 4'b0000;
  assign _zz_PmpPlugin_dGuard_hits_1_1 = 4'b0001;
  assign _zz_PmpPlugin_dGuard_hits_1_3 = 4'b0001;
  assign _zz_PmpPlugin_dGuard_hits_2_1 = 4'b0010;
  assign _zz_PmpPlugin_dGuard_hits_2_3 = 4'b0010;
  assign _zz_PmpPlugin_dGuard_hits_3_1 = 4'b0011;
  assign _zz_PmpPlugin_dGuard_hits_3_3 = 4'b0011;
  assign _zz_PmpPlugin_dGuard_hits_4_1 = 4'b0100;
  assign _zz_PmpPlugin_dGuard_hits_4_3 = 4'b0100;
  assign _zz_PmpPlugin_dGuard_hits_5_1 = 4'b0101;
  assign _zz_PmpPlugin_dGuard_hits_5_3 = 4'b0101;
  assign _zz_PmpPlugin_dGuard_hits_6_1 = 4'b0110;
  assign _zz_PmpPlugin_dGuard_hits_6_3 = 4'b0110;
  assign _zz_PmpPlugin_dGuard_hits_7_1 = 4'b0111;
  assign _zz_PmpPlugin_dGuard_hits_7_3 = 4'b0111;
  assign _zz_PmpPlugin_dGuard_hits_8_1 = 4'b1000;
  assign _zz_PmpPlugin_dGuard_hits_8_3 = 4'b1000;
  assign _zz_PmpPlugin_dGuard_hits_9_1 = 4'b1001;
  assign _zz_PmpPlugin_dGuard_hits_9_3 = 4'b1001;
  assign _zz_PmpPlugin_dGuard_hits_10_1 = 4'b1010;
  assign _zz_PmpPlugin_dGuard_hits_10_3 = 4'b1010;
  assign _zz_PmpPlugin_dGuard_hits_11_1 = 4'b1011;
  assign _zz_PmpPlugin_dGuard_hits_11_3 = 4'b1011;
  assign _zz_PmpPlugin_dGuard_hits_12_1 = 4'b1100;
  assign _zz_PmpPlugin_dGuard_hits_12_3 = 4'b1100;
  assign _zz_PmpPlugin_dGuard_hits_13_1 = 4'b1101;
  assign _zz_PmpPlugin_dGuard_hits_13_3 = 4'b1101;
  assign _zz_PmpPlugin_dGuard_hits_14_1 = 4'b1110;
  assign _zz_PmpPlugin_dGuard_hits_14_3 = 4'b1110;
  assign _zz_PmpPlugin_dGuard_hits_15_1 = 4'b1111;
  assign _zz_PmpPlugin_dGuard_hits_15_3 = 4'b1111;
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_18 = {_zz_DBusCachedPlugin_mmuBus_rsp_allowRead_16,{_zz_DBusCachedPlugin_mmuBus_rsp_allowRead_15,{_zz_DBusCachedPlugin_mmuBus_rsp_allowRead_14,_zz_DBusCachedPlugin_mmuBus_rsp_allowRead_13}}};
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_18 = {_zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_16,{_zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_15,{_zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_14,_zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_13}}};
  assign _zz_PmpPlugin_iGuard_hits_0_2 = 4'b0000;
  assign _zz_PmpPlugin_iGuard_hits_0_4 = 4'b0000;
  assign _zz_PmpPlugin_iGuard_hits_1_1 = 4'b0001;
  assign _zz_PmpPlugin_iGuard_hits_1_3 = 4'b0001;
  assign _zz_PmpPlugin_iGuard_hits_2_1 = 4'b0010;
  assign _zz_PmpPlugin_iGuard_hits_2_3 = 4'b0010;
  assign _zz_PmpPlugin_iGuard_hits_3_1 = 4'b0011;
  assign _zz_PmpPlugin_iGuard_hits_3_3 = 4'b0011;
  assign _zz_PmpPlugin_iGuard_hits_4_1 = 4'b0100;
  assign _zz_PmpPlugin_iGuard_hits_4_3 = 4'b0100;
  assign _zz_PmpPlugin_iGuard_hits_5_1 = 4'b0101;
  assign _zz_PmpPlugin_iGuard_hits_5_3 = 4'b0101;
  assign _zz_PmpPlugin_iGuard_hits_6_1 = 4'b0110;
  assign _zz_PmpPlugin_iGuard_hits_6_3 = 4'b0110;
  assign _zz_PmpPlugin_iGuard_hits_7_1 = 4'b0111;
  assign _zz_PmpPlugin_iGuard_hits_7_3 = 4'b0111;
  assign _zz_PmpPlugin_iGuard_hits_8_1 = 4'b1000;
  assign _zz_PmpPlugin_iGuard_hits_8_3 = 4'b1000;
  assign _zz_PmpPlugin_iGuard_hits_9_1 = 4'b1001;
  assign _zz_PmpPlugin_iGuard_hits_9_3 = 4'b1001;
  assign _zz_PmpPlugin_iGuard_hits_10_1 = 4'b1010;
  assign _zz_PmpPlugin_iGuard_hits_10_3 = 4'b1010;
  assign _zz_PmpPlugin_iGuard_hits_11_1 = 4'b1011;
  assign _zz_PmpPlugin_iGuard_hits_11_3 = 4'b1011;
  assign _zz_PmpPlugin_iGuard_hits_12_1 = 4'b1100;
  assign _zz_PmpPlugin_iGuard_hits_12_3 = 4'b1100;
  assign _zz_PmpPlugin_iGuard_hits_13_1 = 4'b1101;
  assign _zz_PmpPlugin_iGuard_hits_13_3 = 4'b1101;
  assign _zz_PmpPlugin_iGuard_hits_14_1 = 4'b1110;
  assign _zz_PmpPlugin_iGuard_hits_14_3 = 4'b1110;
  assign _zz_PmpPlugin_iGuard_hits_15_1 = 4'b1111;
  assign _zz_PmpPlugin_iGuard_hits_15_3 = 4'b1111;
  assign _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_18 = {_zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_16,{_zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_15,{_zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_14,_zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_13}}};
  assign _zz_when_PmpPlugin_l209_1 = {execute_PmpPlugin_pmpcfgN_,2'b00};
  assign _zz_when_PmpPlugin_l209_1_2 = {execute_PmpPlugin_pmpcfgN_,2'b01};
  assign _zz_when_PmpPlugin_l209_2_1 = {execute_PmpPlugin_pmpcfgN_,2'b10};
  assign _zz_when_PmpPlugin_l209_3_1 = {execute_PmpPlugin_pmpcfgN_,2'b11};
  assign _zz_CsrPlugin_csrMapping_readDataSignal_1 = {execute_PmpPlugin_pmpcfgN,2'b11};
  assign _zz_CsrPlugin_csrMapping_readDataSignal_3 = {execute_PmpPlugin_pmpcfgN,2'b10};
  assign _zz_CsrPlugin_csrMapping_readDataSignal_5 = {execute_PmpPlugin_pmpcfgN,2'b01};
  assign _zz_CsrPlugin_csrMapping_readDataSignal_7 = {execute_PmpPlugin_pmpcfgN,2'b00};
  assign _zz_decode_LEGAL_INSTRUCTION = 32'h0000207f;
  assign _zz_decode_LEGAL_INSTRUCTION_1 = (decode_INSTRUCTION & 32'h0000407f);
  assign _zz_decode_LEGAL_INSTRUCTION_2 = 32'h00004063;
  assign _zz_decode_LEGAL_INSTRUCTION_3 = ((decode_INSTRUCTION & 32'h0000207f) == 32'h00002013);
  assign _zz_decode_LEGAL_INSTRUCTION_4 = ((decode_INSTRUCTION & 32'h0000107f) == 32'h00000013);
  assign _zz_decode_LEGAL_INSTRUCTION_5 = {((decode_INSTRUCTION & 32'h0000603f) == 32'h00000023),{((decode_INSTRUCTION & 32'h0000207f) == 32'h00000003),{((decode_INSTRUCTION & _zz_decode_LEGAL_INSTRUCTION_6) == 32'h00000003),{(_zz_decode_LEGAL_INSTRUCTION_7 == _zz_decode_LEGAL_INSTRUCTION_8),{_zz_decode_LEGAL_INSTRUCTION_9,{_zz_decode_LEGAL_INSTRUCTION_10,_zz_decode_LEGAL_INSTRUCTION_11}}}}}};
  assign _zz_decode_LEGAL_INSTRUCTION_6 = 32'h0000505f;
  assign _zz_decode_LEGAL_INSTRUCTION_7 = (decode_INSTRUCTION & 32'h0000707b);
  assign _zz_decode_LEGAL_INSTRUCTION_8 = 32'h00000063;
  assign _zz_decode_LEGAL_INSTRUCTION_9 = ((decode_INSTRUCTION & 32'h0000607f) == 32'h0000000f);
  assign _zz_decode_LEGAL_INSTRUCTION_10 = ((decode_INSTRUCTION & 32'hfc00007f) == 32'h00000033);
  assign _zz_decode_LEGAL_INSTRUCTION_11 = {((decode_INSTRUCTION & 32'h01f0707f) == 32'h0000500f),{((decode_INSTRUCTION & 32'hbe00705f) == 32'h00005013),{((decode_INSTRUCTION & _zz_decode_LEGAL_INSTRUCTION_12) == 32'h00001013),{(_zz_decode_LEGAL_INSTRUCTION_13 == _zz_decode_LEGAL_INSTRUCTION_14),{_zz_decode_LEGAL_INSTRUCTION_15,{_zz_decode_LEGAL_INSTRUCTION_16,_zz_decode_LEGAL_INSTRUCTION_17}}}}}};
  assign _zz_decode_LEGAL_INSTRUCTION_12 = 32'hfe00305f;
  assign _zz_decode_LEGAL_INSTRUCTION_13 = (decode_INSTRUCTION & 32'hbe00707f);
  assign _zz_decode_LEGAL_INSTRUCTION_14 = 32'h00000033;
  assign _zz_decode_LEGAL_INSTRUCTION_15 = ((decode_INSTRUCTION & 32'hdfffffff) == 32'h10200073);
  assign _zz_decode_LEGAL_INSTRUCTION_16 = ((decode_INSTRUCTION & 32'hffefffff) == 32'h00000073);
  assign _zz_decode_LEGAL_INSTRUCTION_17 = ((decode_INSTRUCTION & 32'hffffffff) == 32'h10500073);
  assign _zz_IBusCachedPlugin_predictionJumpInterface_payload_4 = decode_INSTRUCTION[31];
  assign _zz_IBusCachedPlugin_predictionJumpInterface_payload_5 = decode_INSTRUCTION[31];
  assign _zz_IBusCachedPlugin_predictionJumpInterface_payload_6 = decode_INSTRUCTION[7];
  assign _zz_when_PmpPlugin_l277 = PmpPlugin_dGuard_hits_6;
  assign _zz_when_PmpPlugin_l277_1 = {PmpPlugin_dGuard_hits_5,{PmpPlugin_dGuard_hits_4,{PmpPlugin_dGuard_hits_3,{PmpPlugin_dGuard_hits_2,{PmpPlugin_dGuard_hits_1,PmpPlugin_dGuard_hits_0}}}}};
  assign _zz__zz_DBusCachedPlugin_mmuBus_rsp_allowRead = PmpPlugin_dGuard_hits_5;
  assign _zz__zz_DBusCachedPlugin_mmuBus_rsp_allowRead_1 = {PmpPlugin_dGuard_hits_4,{PmpPlugin_dGuard_hits_3,{PmpPlugin_dGuard_hits_2,{PmpPlugin_dGuard_hits_1,PmpPlugin_dGuard_hits_0}}}};
  assign _zz__zz_DBusCachedPlugin_mmuBus_rsp_allowWrite = PmpPlugin_dGuard_hits_5;
  assign _zz__zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_1 = {PmpPlugin_dGuard_hits_4,{PmpPlugin_dGuard_hits_3,{PmpPlugin_dGuard_hits_2,{PmpPlugin_dGuard_hits_1,PmpPlugin_dGuard_hits_0}}}};
  assign _zz_when_PmpPlugin_l299 = PmpPlugin_iGuard_hits_6;
  assign _zz_when_PmpPlugin_l299_1 = {PmpPlugin_iGuard_hits_5,{PmpPlugin_iGuard_hits_4,{PmpPlugin_iGuard_hits_3,{PmpPlugin_iGuard_hits_2,{PmpPlugin_iGuard_hits_1,PmpPlugin_iGuard_hits_0}}}}};
  assign _zz__zz_IBusCachedPlugin_mmuBus_rsp_allowExecute = PmpPlugin_iGuard_hits_5;
  assign _zz__zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_1 = {PmpPlugin_iGuard_hits_4,{PmpPlugin_iGuard_hits_3,{PmpPlugin_iGuard_hits_2,{PmpPlugin_iGuard_hits_1,PmpPlugin_iGuard_hits_0}}}};
  assign _zz__zz_decode_IS_CSR = (decode_INSTRUCTION & 32'h0000001c);
  assign _zz__zz_decode_IS_CSR_1 = 32'h00000004;
  assign _zz__zz_decode_IS_CSR_2 = (decode_INSTRUCTION & 32'h00000058);
  assign _zz__zz_decode_IS_CSR_3 = 32'h00000040;
  assign _zz__zz_decode_IS_CSR_4 = ((decode_INSTRUCTION & 32'h10103050) == 32'h00100050);
  assign _zz__zz_decode_IS_CSR_5 = {((decode_INSTRUCTION & _zz__zz_decode_IS_CSR_6) == 32'h10000050),((decode_INSTRUCTION & _zz__zz_decode_IS_CSR_7) == 32'h00000050)};
  assign _zz__zz_decode_IS_CSR_8 = (|((decode_INSTRUCTION & _zz__zz_decode_IS_CSR_9) == 32'h00000050));
  assign _zz__zz_decode_IS_CSR_10 = (|{_zz__zz_decode_IS_CSR_11,_zz__zz_decode_IS_CSR_12});
  assign _zz__zz_decode_IS_CSR_13 = {(|_zz__zz_decode_IS_CSR_14),{(|_zz__zz_decode_IS_CSR_15),{_zz__zz_decode_IS_CSR_16,{_zz__zz_decode_IS_CSR_17,_zz__zz_decode_IS_CSR_20}}}};
  assign _zz__zz_decode_IS_CSR_6 = 32'h10203050;
  assign _zz__zz_decode_IS_CSR_7 = 32'h10103050;
  assign _zz__zz_decode_IS_CSR_9 = 32'h00103050;
  assign _zz__zz_decode_IS_CSR_11 = ((decode_INSTRUCTION & 32'h00001050) == 32'h00001050);
  assign _zz__zz_decode_IS_CSR_12 = ((decode_INSTRUCTION & 32'h00002050) == 32'h00002050);
  assign _zz__zz_decode_IS_CSR_14 = ((decode_INSTRUCTION & 32'h02004064) == 32'h02004020);
  assign _zz__zz_decode_IS_CSR_15 = {_zz_decode_IS_CSR_7,_zz_decode_IS_CSR_6};
  assign _zz__zz_decode_IS_CSR_16 = (|{_zz_decode_IS_CSR_7,{_zz_decode_IS_CSR_5,_zz_decode_IS_CSR_6}});
  assign _zz__zz_decode_IS_CSR_17 = (|(_zz__zz_decode_IS_CSR_18 == _zz__zz_decode_IS_CSR_19));
  assign _zz__zz_decode_IS_CSR_20 = {(|_zz__zz_decode_IS_CSR_21),{(|_zz__zz_decode_IS_CSR_22),{_zz__zz_decode_IS_CSR_25,{_zz__zz_decode_IS_CSR_27,_zz__zz_decode_IS_CSR_30}}}};
  assign _zz__zz_decode_IS_CSR_18 = (decode_INSTRUCTION & 32'h02004074);
  assign _zz__zz_decode_IS_CSR_19 = 32'h02000030;
  assign _zz__zz_decode_IS_CSR_21 = ((decode_INSTRUCTION & 32'h02007054) == 32'h00005010);
  assign _zz__zz_decode_IS_CSR_22 = {((decode_INSTRUCTION & _zz__zz_decode_IS_CSR_23) == 32'h40001010),((decode_INSTRUCTION & _zz__zz_decode_IS_CSR_24) == 32'h00001010)};
  assign _zz__zz_decode_IS_CSR_25 = (|((decode_INSTRUCTION & _zz__zz_decode_IS_CSR_26) == 32'h00000024));
  assign _zz__zz_decode_IS_CSR_27 = (|(_zz__zz_decode_IS_CSR_28 == _zz__zz_decode_IS_CSR_29));
  assign _zz__zz_decode_IS_CSR_30 = {(|_zz_decode_IS_CSR_5),{(|_zz__zz_decode_IS_CSR_31),{_zz__zz_decode_IS_CSR_34,{_zz__zz_decode_IS_CSR_36,_zz__zz_decode_IS_CSR_39}}}};
  assign _zz__zz_decode_IS_CSR_23 = 32'h40003054;
  assign _zz__zz_decode_IS_CSR_24 = 32'h02007054;
  assign _zz__zz_decode_IS_CSR_26 = 32'h00000064;
  assign _zz__zz_decode_IS_CSR_28 = (decode_INSTRUCTION & 32'h00001000);
  assign _zz__zz_decode_IS_CSR_29 = 32'h00001000;
  assign _zz__zz_decode_IS_CSR_31 = {((decode_INSTRUCTION & _zz__zz_decode_IS_CSR_32) == 32'h00002000),((decode_INSTRUCTION & _zz__zz_decode_IS_CSR_33) == 32'h00001000)};
  assign _zz__zz_decode_IS_CSR_34 = (|((decode_INSTRUCTION & _zz__zz_decode_IS_CSR_35) == 32'h00004008));
  assign _zz__zz_decode_IS_CSR_36 = (|{_zz__zz_decode_IS_CSR_37,_zz__zz_decode_IS_CSR_38});
  assign _zz__zz_decode_IS_CSR_39 = {(|{_zz__zz_decode_IS_CSR_40,_zz__zz_decode_IS_CSR_42}),{(|_zz__zz_decode_IS_CSR_49),{_zz__zz_decode_IS_CSR_51,{_zz__zz_decode_IS_CSR_54,_zz__zz_decode_IS_CSR_67}}}};
  assign _zz__zz_decode_IS_CSR_32 = 32'h00002010;
  assign _zz__zz_decode_IS_CSR_33 = 32'h00005000;
  assign _zz__zz_decode_IS_CSR_35 = 32'h00004048;
  assign _zz__zz_decode_IS_CSR_37 = ((decode_INSTRUCTION & 32'h00000034) == 32'h00000020);
  assign _zz__zz_decode_IS_CSR_38 = ((decode_INSTRUCTION & 32'h00000064) == 32'h00000020);
  assign _zz__zz_decode_IS_CSR_40 = ((decode_INSTRUCTION & _zz__zz_decode_IS_CSR_41) == 32'h00002040);
  assign _zz__zz_decode_IS_CSR_42 = {(_zz__zz_decode_IS_CSR_43 == _zz__zz_decode_IS_CSR_44),{_zz__zz_decode_IS_CSR_45,{_zz__zz_decode_IS_CSR_46,_zz__zz_decode_IS_CSR_48}}};
  assign _zz__zz_decode_IS_CSR_49 = ((decode_INSTRUCTION & _zz__zz_decode_IS_CSR_50) == 32'h00000020);
  assign _zz__zz_decode_IS_CSR_51 = (|(_zz__zz_decode_IS_CSR_52 == _zz__zz_decode_IS_CSR_53));
  assign _zz__zz_decode_IS_CSR_54 = (|{_zz__zz_decode_IS_CSR_55,_zz__zz_decode_IS_CSR_56});
  assign _zz__zz_decode_IS_CSR_67 = {(|_zz__zz_decode_IS_CSR_68),{_zz__zz_decode_IS_CSR_80,{_zz__zz_decode_IS_CSR_83,_zz__zz_decode_IS_CSR_88}}};
  assign _zz__zz_decode_IS_CSR_41 = 32'h00002040;
  assign _zz__zz_decode_IS_CSR_43 = (decode_INSTRUCTION & 32'h00001040);
  assign _zz__zz_decode_IS_CSR_44 = 32'h00001040;
  assign _zz__zz_decode_IS_CSR_45 = ((decode_INSTRUCTION & 32'h00100040) == 32'h00000040);
  assign _zz__zz_decode_IS_CSR_46 = ((decode_INSTRUCTION & _zz__zz_decode_IS_CSR_47) == 32'h00000040);
  assign _zz__zz_decode_IS_CSR_48 = _zz_decode_IS_CSR_2;
  assign _zz__zz_decode_IS_CSR_50 = 32'h00000020;
  assign _zz__zz_decode_IS_CSR_52 = (decode_INSTRUCTION & 32'h00000010);
  assign _zz__zz_decode_IS_CSR_53 = 32'h00000010;
  assign _zz__zz_decode_IS_CSR_55 = _zz_decode_IS_CSR_3;
  assign _zz__zz_decode_IS_CSR_56 = {(_zz__zz_decode_IS_CSR_57 == _zz__zz_decode_IS_CSR_58),{_zz__zz_decode_IS_CSR_59,{_zz__zz_decode_IS_CSR_61,_zz__zz_decode_IS_CSR_64}}};
  assign _zz__zz_decode_IS_CSR_68 = {_zz_decode_IS_CSR_4,{_zz__zz_decode_IS_CSR_69,{_zz__zz_decode_IS_CSR_71,_zz__zz_decode_IS_CSR_74}}};
  assign _zz__zz_decode_IS_CSR_80 = (|{_zz_decode_IS_CSR_3,_zz__zz_decode_IS_CSR_81});
  assign _zz__zz_decode_IS_CSR_83 = (|{_zz__zz_decode_IS_CSR_84,_zz__zz_decode_IS_CSR_85});
  assign _zz__zz_decode_IS_CSR_88 = {(|_zz__zz_decode_IS_CSR_89),{_zz__zz_decode_IS_CSR_92,{_zz__zz_decode_IS_CSR_94,_zz__zz_decode_IS_CSR_105}}};
  assign _zz__zz_decode_IS_CSR_47 = 32'h00000050;
  assign _zz__zz_decode_IS_CSR_57 = (decode_INSTRUCTION & 32'h00002030);
  assign _zz__zz_decode_IS_CSR_58 = 32'h00002010;
  assign _zz__zz_decode_IS_CSR_59 = ((decode_INSTRUCTION & _zz__zz_decode_IS_CSR_60) == 32'h00000010);
  assign _zz__zz_decode_IS_CSR_61 = (_zz__zz_decode_IS_CSR_62 == _zz__zz_decode_IS_CSR_63);
  assign _zz__zz_decode_IS_CSR_64 = (_zz__zz_decode_IS_CSR_65 == _zz__zz_decode_IS_CSR_66);
  assign _zz__zz_decode_IS_CSR_69 = ((decode_INSTRUCTION & _zz__zz_decode_IS_CSR_70) == 32'h00001010);
  assign _zz__zz_decode_IS_CSR_71 = (_zz__zz_decode_IS_CSR_72 == _zz__zz_decode_IS_CSR_73);
  assign _zz__zz_decode_IS_CSR_74 = {_zz__zz_decode_IS_CSR_75,{_zz__zz_decode_IS_CSR_76,_zz__zz_decode_IS_CSR_78}};
  assign _zz__zz_decode_IS_CSR_81 = ((decode_INSTRUCTION & _zz__zz_decode_IS_CSR_82) == 32'h00000020);
  assign _zz__zz_decode_IS_CSR_84 = _zz_decode_IS_CSR_3;
  assign _zz__zz_decode_IS_CSR_85 = (_zz__zz_decode_IS_CSR_86 == _zz__zz_decode_IS_CSR_87);
  assign _zz__zz_decode_IS_CSR_89 = (_zz__zz_decode_IS_CSR_90 == _zz__zz_decode_IS_CSR_91);
  assign _zz__zz_decode_IS_CSR_92 = (|_zz__zz_decode_IS_CSR_93);
  assign _zz__zz_decode_IS_CSR_94 = (|_zz__zz_decode_IS_CSR_95);
  assign _zz__zz_decode_IS_CSR_105 = {_zz__zz_decode_IS_CSR_106,{_zz__zz_decode_IS_CSR_109,_zz__zz_decode_IS_CSR_117}};
  assign _zz__zz_decode_IS_CSR_60 = 32'h00001030;
  assign _zz__zz_decode_IS_CSR_62 = (decode_INSTRUCTION & 32'h02002060);
  assign _zz__zz_decode_IS_CSR_63 = 32'h00002020;
  assign _zz__zz_decode_IS_CSR_65 = (decode_INSTRUCTION & 32'h02003020);
  assign _zz__zz_decode_IS_CSR_66 = 32'h00000020;
  assign _zz__zz_decode_IS_CSR_70 = 32'h00001010;
  assign _zz__zz_decode_IS_CSR_72 = (decode_INSTRUCTION & 32'h00002010);
  assign _zz__zz_decode_IS_CSR_73 = 32'h00002010;
  assign _zz__zz_decode_IS_CSR_75 = ((decode_INSTRUCTION & 32'h00000050) == 32'h00000010);
  assign _zz__zz_decode_IS_CSR_76 = ((decode_INSTRUCTION & _zz__zz_decode_IS_CSR_77) == 32'h00000004);
  assign _zz__zz_decode_IS_CSR_78 = ((decode_INSTRUCTION & _zz__zz_decode_IS_CSR_79) == 32'h0);
  assign _zz__zz_decode_IS_CSR_82 = 32'h00000070;
  assign _zz__zz_decode_IS_CSR_86 = (decode_INSTRUCTION & 32'h00000020);
  assign _zz__zz_decode_IS_CSR_87 = 32'h0;
  assign _zz__zz_decode_IS_CSR_90 = (decode_INSTRUCTION & 32'h00004014);
  assign _zz__zz_decode_IS_CSR_91 = 32'h00004010;
  assign _zz__zz_decode_IS_CSR_93 = ((decode_INSTRUCTION & 32'h00006014) == 32'h00002010);
  assign _zz__zz_decode_IS_CSR_95 = {(_zz__zz_decode_IS_CSR_96 == _zz__zz_decode_IS_CSR_97),{_zz_decode_IS_CSR_2,{_zz__zz_decode_IS_CSR_98,_zz__zz_decode_IS_CSR_100}}};
  assign _zz__zz_decode_IS_CSR_106 = (|(_zz__zz_decode_IS_CSR_107 == _zz__zz_decode_IS_CSR_108));
  assign _zz__zz_decode_IS_CSR_109 = (|{_zz__zz_decode_IS_CSR_110,_zz__zz_decode_IS_CSR_112});
  assign _zz__zz_decode_IS_CSR_117 = {(|_zz__zz_decode_IS_CSR_118),{_zz__zz_decode_IS_CSR_121,_zz__zz_decode_IS_CSR_123}};
  assign _zz__zz_decode_IS_CSR_77 = 32'h0000000c;
  assign _zz__zz_decode_IS_CSR_79 = 32'h00000028;
  assign _zz__zz_decode_IS_CSR_96 = (decode_INSTRUCTION & 32'h00000044);
  assign _zz__zz_decode_IS_CSR_97 = 32'h0;
  assign _zz__zz_decode_IS_CSR_98 = ((decode_INSTRUCTION & _zz__zz_decode_IS_CSR_99) == 32'h00002000);
  assign _zz__zz_decode_IS_CSR_100 = {(_zz__zz_decode_IS_CSR_101 == _zz__zz_decode_IS_CSR_102),(_zz__zz_decode_IS_CSR_103 == _zz__zz_decode_IS_CSR_104)};
  assign _zz__zz_decode_IS_CSR_107 = (decode_INSTRUCTION & 32'h00000058);
  assign _zz__zz_decode_IS_CSR_108 = 32'h0;
  assign _zz__zz_decode_IS_CSR_110 = ((decode_INSTRUCTION & _zz__zz_decode_IS_CSR_111) == 32'h00000040);
  assign _zz__zz_decode_IS_CSR_112 = {(_zz__zz_decode_IS_CSR_113 == _zz__zz_decode_IS_CSR_114),(_zz__zz_decode_IS_CSR_115 == _zz__zz_decode_IS_CSR_116)};
  assign _zz__zz_decode_IS_CSR_118 = {(_zz__zz_decode_IS_CSR_119 == _zz__zz_decode_IS_CSR_120),_zz_decode_IS_CSR_1};
  assign _zz__zz_decode_IS_CSR_121 = (|{_zz__zz_decode_IS_CSR_122,_zz_decode_IS_CSR_1});
  assign _zz__zz_decode_IS_CSR_123 = (|(_zz__zz_decode_IS_CSR_124 == _zz__zz_decode_IS_CSR_125));
  assign _zz__zz_decode_IS_CSR_99 = 32'h00006004;
  assign _zz__zz_decode_IS_CSR_101 = (decode_INSTRUCTION & 32'h00005004);
  assign _zz__zz_decode_IS_CSR_102 = 32'h00001000;
  assign _zz__zz_decode_IS_CSR_103 = (decode_INSTRUCTION & 32'h00004050);
  assign _zz__zz_decode_IS_CSR_104 = 32'h00004000;
  assign _zz__zz_decode_IS_CSR_111 = 32'h00000044;
  assign _zz__zz_decode_IS_CSR_113 = (decode_INSTRUCTION & 32'h00002014);
  assign _zz__zz_decode_IS_CSR_114 = 32'h00002010;
  assign _zz__zz_decode_IS_CSR_115 = (decode_INSTRUCTION & 32'h40000034);
  assign _zz__zz_decode_IS_CSR_116 = 32'h40000030;
  assign _zz__zz_decode_IS_CSR_119 = (decode_INSTRUCTION & 32'h00000014);
  assign _zz__zz_decode_IS_CSR_120 = 32'h00000004;
  assign _zz__zz_decode_IS_CSR_122 = ((decode_INSTRUCTION & 32'h00000044) == 32'h00000004);
  assign _zz__zz_decode_IS_CSR_124 = (decode_INSTRUCTION & 32'h00005048);
  assign _zz__zz_decode_IS_CSR_125 = 32'h00001008;
  assign _zz_execute_BranchPlugin_branch_src2_6 = execute_INSTRUCTION[31];
  assign _zz_execute_BranchPlugin_branch_src2_7 = execute_INSTRUCTION[31];
  assign _zz_execute_BranchPlugin_branch_src2_8 = execute_INSTRUCTION[7];
  assign _zz_CsrPlugin_csrMapping_readDataInit_20 = 32'h0;
  assign _zz_PmpPlugin_pmpaddr_port0 = PmpPlugin_pmpaddr[execute_PmpPlugin_fsm_fsmCounter];
  always @(posedge clk) begin
    if(_zz_1) begin
      PmpPlugin_pmpaddr[execute_PmpPlugin_pmpNcfg_] <= _zz_PmpPlugin_pmpaddr_port;
    end
  end

  assign _zz_PmpPlugin_pmpaddr_port2 = PmpPlugin_pmpaddr[execute_PmpPlugin_pmpNcfg];
  always @(posedge clk) begin
    if(_zz_decode_RegFilePlugin_rs1Data) begin
      _zz_RegFilePlugin_regFile_port0 <= RegFilePlugin_regFile[decode_RegFilePlugin_regFileReadAddress1];
    end
  end

  always @(posedge clk) begin
    if(_zz_decode_RegFilePlugin_rs2Data) begin
      _zz_RegFilePlugin_regFile_port1 <= RegFilePlugin_regFile[decode_RegFilePlugin_regFileReadAddress2];
    end
  end

  always @(posedge clk) begin
    if(_zz_2) begin
      RegFilePlugin_regFile[lastStageRegFileWrite_payload_address] <= lastStageRegFileWrite_payload_data;
    end
  end

  PmpSetter PmpPlugin_setter (
    .io_addr (PmpPlugin_setter_io_addr[31:0]), //i
    .io_base (PmpPlugin_setter_io_base[27:0]), //o
    .io_mask (PmpPlugin_setter_io_mask[27:0])  //o
  );
  InstructionCache IBusCachedPlugin_cache (
    .io_flush                              (IBusCachedPlugin_cache_io_flush                           ), //i
    .io_cpu_prefetch_isValid               (IBusCachedPlugin_cache_io_cpu_prefetch_isValid            ), //i
    .io_cpu_prefetch_haltIt                (IBusCachedPlugin_cache_io_cpu_prefetch_haltIt             ), //o
    .io_cpu_prefetch_pc                    (IBusCachedPlugin_iBusRsp_stages_0_input_payload[31:0]     ), //i
    .io_cpu_fetch_isValid                  (IBusCachedPlugin_cache_io_cpu_fetch_isValid               ), //i
    .io_cpu_fetch_isStuck                  (IBusCachedPlugin_cache_io_cpu_fetch_isStuck               ), //i
    .io_cpu_fetch_isRemoved                (IBusCachedPlugin_cache_io_cpu_fetch_isRemoved             ), //i
    .io_cpu_fetch_pc                       (IBusCachedPlugin_iBusRsp_stages_1_input_payload[31:0]     ), //i
    .io_cpu_fetch_data                     (IBusCachedPlugin_cache_io_cpu_fetch_data[31:0]            ), //o
    .io_cpu_fetch_mmuRsp_physicalAddress   (IBusCachedPlugin_mmuBus_rsp_physicalAddress[31:0]         ), //i
    .io_cpu_fetch_mmuRsp_isIoAccess        (IBusCachedPlugin_mmuBus_rsp_isIoAccess                    ), //i
    .io_cpu_fetch_mmuRsp_isPaging          (IBusCachedPlugin_mmuBus_rsp_isPaging                      ), //i
    .io_cpu_fetch_mmuRsp_allowRead         (IBusCachedPlugin_mmuBus_rsp_allowRead                     ), //i
    .io_cpu_fetch_mmuRsp_allowWrite        (IBusCachedPlugin_mmuBus_rsp_allowWrite                    ), //i
    .io_cpu_fetch_mmuRsp_allowExecute      (IBusCachedPlugin_mmuBus_rsp_allowExecute                  ), //i
    .io_cpu_fetch_mmuRsp_exception         (IBusCachedPlugin_mmuBus_rsp_exception                     ), //i
    .io_cpu_fetch_mmuRsp_refilling         (IBusCachedPlugin_mmuBus_rsp_refilling                     ), //i
    .io_cpu_fetch_mmuRsp_bypassTranslation (IBusCachedPlugin_mmuBus_rsp_bypassTranslation             ), //i
    .io_cpu_fetch_physicalAddress          (IBusCachedPlugin_cache_io_cpu_fetch_physicalAddress[31:0] ), //o
    .io_cpu_decode_isValid                 (IBusCachedPlugin_cache_io_cpu_decode_isValid              ), //i
    .io_cpu_decode_isStuck                 (IBusCachedPlugin_cache_io_cpu_decode_isStuck              ), //i
    .io_cpu_decode_pc                      (IBusCachedPlugin_iBusRsp_stages_2_input_payload[31:0]     ), //i
    .io_cpu_decode_physicalAddress         (IBusCachedPlugin_cache_io_cpu_decode_physicalAddress[31:0]), //o
    .io_cpu_decode_data                    (IBusCachedPlugin_cache_io_cpu_decode_data[31:0]           ), //o
    .io_cpu_decode_cacheMiss               (IBusCachedPlugin_cache_io_cpu_decode_cacheMiss            ), //o
    .io_cpu_decode_error                   (IBusCachedPlugin_cache_io_cpu_decode_error                ), //o
    .io_cpu_decode_mmuRefilling            (IBusCachedPlugin_cache_io_cpu_decode_mmuRefilling         ), //o
    .io_cpu_decode_mmuException            (IBusCachedPlugin_cache_io_cpu_decode_mmuException         ), //o
    .io_cpu_decode_isUser                  (IBusCachedPlugin_cache_io_cpu_decode_isUser               ), //i
    .io_cpu_fill_valid                     (IBusCachedPlugin_cache_io_cpu_fill_valid                  ), //i
    .io_cpu_fill_payload                   (IBusCachedPlugin_cache_io_cpu_decode_physicalAddress[31:0]), //i
    .io_mem_cmd_valid                      (IBusCachedPlugin_cache_io_mem_cmd_valid                   ), //o
    .io_mem_cmd_ready                      (iBus_cmd_ready                                            ), //i
    .io_mem_cmd_payload_address            (IBusCachedPlugin_cache_io_mem_cmd_payload_address[31:0]   ), //o
    .io_mem_cmd_payload_size               (IBusCachedPlugin_cache_io_mem_cmd_payload_size[2:0]       ), //o
    .io_mem_rsp_valid                      (iBus_rsp_valid                                            ), //i
    .io_mem_rsp_payload_data               (iBus_rsp_payload_data[31:0]                               ), //i
    .io_mem_rsp_payload_error              (iBus_rsp_payload_error                                    ), //i
    ._zz_when_Fetcher_l401                 (switch_Fetcher_l365[2:0]                                  ), //i
    ._zz_decodeStage_hit_data_1            (IBusCachedPlugin_injectionPort_payload[31:0]              ), //i
    .clk                                   (clk                                                       ), //i
    .reset                                 (reset                                                     )  //i
  );
  DataCache dataCache_1 (
    .io_cpu_execute_isValid                 (dataCache_1_io_cpu_execute_isValid               ), //i
    .io_cpu_execute_address                 (dataCache_1_io_cpu_execute_address[31:0]         ), //i
    .io_cpu_execute_haltIt                  (dataCache_1_io_cpu_execute_haltIt                ), //o
    .io_cpu_execute_args_wr                 (execute_MEMORY_WR                                ), //i
    .io_cpu_execute_args_size               (execute_DBusCachedPlugin_size[1:0]               ), //i
    .io_cpu_execute_args_totalyConsistent   (execute_MEMORY_FORCE_CONSTISTENCY                ), //i
    .io_cpu_execute_refilling               (dataCache_1_io_cpu_execute_refilling             ), //o
    .io_cpu_memory_isValid                  (dataCache_1_io_cpu_memory_isValid                ), //i
    .io_cpu_memory_isStuck                  (memory_arbitration_isStuck                       ), //i
    .io_cpu_memory_isWrite                  (dataCache_1_io_cpu_memory_isWrite                ), //o
    .io_cpu_memory_address                  (dataCache_1_io_cpu_memory_address[31:0]          ), //i
    .io_cpu_memory_mmuRsp_physicalAddress   (DBusCachedPlugin_mmuBus_rsp_physicalAddress[31:0]), //i
    .io_cpu_memory_mmuRsp_isIoAccess        (dataCache_1_io_cpu_memory_mmuRsp_isIoAccess      ), //i
    .io_cpu_memory_mmuRsp_isPaging          (DBusCachedPlugin_mmuBus_rsp_isPaging             ), //i
    .io_cpu_memory_mmuRsp_allowRead         (DBusCachedPlugin_mmuBus_rsp_allowRead            ), //i
    .io_cpu_memory_mmuRsp_allowWrite        (DBusCachedPlugin_mmuBus_rsp_allowWrite           ), //i
    .io_cpu_memory_mmuRsp_allowExecute      (DBusCachedPlugin_mmuBus_rsp_allowExecute         ), //i
    .io_cpu_memory_mmuRsp_exception         (DBusCachedPlugin_mmuBus_rsp_exception            ), //i
    .io_cpu_memory_mmuRsp_refilling         (DBusCachedPlugin_mmuBus_rsp_refilling            ), //i
    .io_cpu_memory_mmuRsp_bypassTranslation (DBusCachedPlugin_mmuBus_rsp_bypassTranslation    ), //i
    .io_cpu_writeBack_isValid               (dataCache_1_io_cpu_writeBack_isValid             ), //i
    .io_cpu_writeBack_isStuck               (writeBack_arbitration_isStuck                    ), //i
    .io_cpu_writeBack_isFiring              (writeBack_arbitration_isFiring                   ), //i
    .io_cpu_writeBack_isUser                (dataCache_1_io_cpu_writeBack_isUser              ), //i
    .io_cpu_writeBack_haltIt                (dataCache_1_io_cpu_writeBack_haltIt              ), //o
    .io_cpu_writeBack_isWrite               (dataCache_1_io_cpu_writeBack_isWrite             ), //o
    .io_cpu_writeBack_storeData             (dataCache_1_io_cpu_writeBack_storeData[31:0]     ), //i
    .io_cpu_writeBack_data                  (dataCache_1_io_cpu_writeBack_data[31:0]          ), //o
    .io_cpu_writeBack_address               (dataCache_1_io_cpu_writeBack_address[31:0]       ), //i
    .io_cpu_writeBack_mmuException          (dataCache_1_io_cpu_writeBack_mmuException        ), //o
    .io_cpu_writeBack_unalignedAccess       (dataCache_1_io_cpu_writeBack_unalignedAccess     ), //o
    .io_cpu_writeBack_accessError           (dataCache_1_io_cpu_writeBack_accessError         ), //o
    .io_cpu_writeBack_keepMemRspData        (dataCache_1_io_cpu_writeBack_keepMemRspData      ), //o
    .io_cpu_writeBack_fence_SW              (dataCache_1_io_cpu_writeBack_fence_SW            ), //i
    .io_cpu_writeBack_fence_SR              (dataCache_1_io_cpu_writeBack_fence_SR            ), //i
    .io_cpu_writeBack_fence_SO              (dataCache_1_io_cpu_writeBack_fence_SO            ), //i
    .io_cpu_writeBack_fence_SI              (dataCache_1_io_cpu_writeBack_fence_SI            ), //i
    .io_cpu_writeBack_fence_PW              (dataCache_1_io_cpu_writeBack_fence_PW            ), //i
    .io_cpu_writeBack_fence_PR              (dataCache_1_io_cpu_writeBack_fence_PR            ), //i
    .io_cpu_writeBack_fence_PO              (dataCache_1_io_cpu_writeBack_fence_PO            ), //i
    .io_cpu_writeBack_fence_PI              (dataCache_1_io_cpu_writeBack_fence_PI            ), //i
    .io_cpu_writeBack_fence_FM              (dataCache_1_io_cpu_writeBack_fence_FM[3:0]       ), //i
    .io_cpu_writeBack_exclusiveOk           (dataCache_1_io_cpu_writeBack_exclusiveOk         ), //o
    .io_cpu_redo                            (dataCache_1_io_cpu_redo                          ), //o
    .io_cpu_flush_valid                     (dataCache_1_io_cpu_flush_valid                   ), //i
    .io_cpu_flush_ready                     (dataCache_1_io_cpu_flush_ready                   ), //o
    .io_cpu_flush_payload_singleLine        (dataCache_1_io_cpu_flush_payload_singleLine      ), //i
    .io_cpu_flush_payload_lineId            (dataCache_1_io_cpu_flush_payload_lineId[6:0]     ), //i
    .io_mem_cmd_valid                       (dataCache_1_io_mem_cmd_valid                     ), //o
    .io_mem_cmd_ready                       (dBus_cmd_ready                                   ), //i
    .io_mem_cmd_payload_wr                  (dataCache_1_io_mem_cmd_payload_wr                ), //o
    .io_mem_cmd_payload_uncached            (dataCache_1_io_mem_cmd_payload_uncached          ), //o
    .io_mem_cmd_payload_address             (dataCache_1_io_mem_cmd_payload_address[31:0]     ), //o
    .io_mem_cmd_payload_data                (dataCache_1_io_mem_cmd_payload_data[31:0]        ), //o
    .io_mem_cmd_payload_mask                (dataCache_1_io_mem_cmd_payload_mask[3:0]         ), //o
    .io_mem_cmd_payload_size                (dataCache_1_io_mem_cmd_payload_size[2:0]         ), //o
    .io_mem_cmd_payload_last                (dataCache_1_io_mem_cmd_payload_last              ), //o
    .io_mem_rsp_valid                       (dBus_rsp_valid                                   ), //i
    .io_mem_rsp_payload_last                (dBus_rsp_payload_last                            ), //i
    .io_mem_rsp_payload_data                (dBus_rsp_payload_data[31:0]                      ), //i
    .io_mem_rsp_payload_error               (dBus_rsp_payload_error                           ), //i
    .clk                                    (clk                                              ), //i
    .reset                                  (reset                                            )  //i
  );
  always @(*) begin
    case(_zz_IBusCachedPlugin_jump_pcLoad_payload_6)
      2'b00 : _zz_IBusCachedPlugin_jump_pcLoad_payload_5 = DBusCachedPlugin_redoBranch_payload;
      2'b01 : _zz_IBusCachedPlugin_jump_pcLoad_payload_5 = CsrPlugin_jumpInterface_payload;
      2'b10 : _zz_IBusCachedPlugin_jump_pcLoad_payload_5 = BranchPlugin_jumpInterface_payload;
      default : _zz_IBusCachedPlugin_jump_pcLoad_payload_5 = IBusCachedPlugin_predictionJumpInterface_payload;
    endcase
  end

  always @(*) begin
    case(_zz_writeBack_DBusCachedPlugin_rspShifted_1)
      2'b00 : _zz_writeBack_DBusCachedPlugin_rspShifted = writeBack_DBusCachedPlugin_rspSplits_0;
      2'b01 : _zz_writeBack_DBusCachedPlugin_rspShifted = writeBack_DBusCachedPlugin_rspSplits_1;
      2'b10 : _zz_writeBack_DBusCachedPlugin_rspShifted = writeBack_DBusCachedPlugin_rspSplits_2;
      default : _zz_writeBack_DBusCachedPlugin_rspShifted = writeBack_DBusCachedPlugin_rspSplits_3;
    endcase
  end

  always @(*) begin
    case(_zz_writeBack_DBusCachedPlugin_rspShifted_3)
      1'b0 : _zz_writeBack_DBusCachedPlugin_rspShifted_2 = writeBack_DBusCachedPlugin_rspSplits_1;
      default : _zz_writeBack_DBusCachedPlugin_rspShifted_2 = writeBack_DBusCachedPlugin_rspSplits_3;
    endcase
  end

  always @(*) begin
    case(execute_PmpPlugin_fsm_fsmCounter)
      4'b0000 : _zz_when_PmpPlugin_l246 = PmpPlugin_pmpcfg_0;
      4'b0001 : _zz_when_PmpPlugin_l246 = PmpPlugin_pmpcfg_1;
      4'b0010 : _zz_when_PmpPlugin_l246 = PmpPlugin_pmpcfg_2;
      4'b0011 : _zz_when_PmpPlugin_l246 = PmpPlugin_pmpcfg_3;
      4'b0100 : _zz_when_PmpPlugin_l246 = PmpPlugin_pmpcfg_4;
      4'b0101 : _zz_when_PmpPlugin_l246 = PmpPlugin_pmpcfg_5;
      4'b0110 : _zz_when_PmpPlugin_l246 = PmpPlugin_pmpcfg_6;
      4'b0111 : _zz_when_PmpPlugin_l246 = PmpPlugin_pmpcfg_7;
      4'b1000 : _zz_when_PmpPlugin_l246 = PmpPlugin_pmpcfg_8;
      4'b1001 : _zz_when_PmpPlugin_l246 = PmpPlugin_pmpcfg_9;
      4'b1010 : _zz_when_PmpPlugin_l246 = PmpPlugin_pmpcfg_10;
      4'b1011 : _zz_when_PmpPlugin_l246 = PmpPlugin_pmpcfg_11;
      4'b1100 : _zz_when_PmpPlugin_l246 = PmpPlugin_pmpcfg_12;
      4'b1101 : _zz_when_PmpPlugin_l246 = PmpPlugin_pmpcfg_13;
      4'b1110 : _zz_when_PmpPlugin_l246 = PmpPlugin_pmpcfg_14;
      default : _zz_when_PmpPlugin_l246 = PmpPlugin_pmpcfg_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_0_2)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_0_1 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_0_1 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_0_1 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_0_1 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_0_1 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_0_1 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_0_1 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_0_1 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_0_1 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_0_1 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_0_1 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_0_1 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_0_1 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_0_1 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_0_1 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_dGuard_hits_0_1 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_0_4)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_0_3 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_0_3 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_0_3 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_0_3 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_0_3 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_0_3 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_0_3 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_0_3 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_0_3 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_0_3 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_0_3 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_0_3 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_0_3 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_0_3 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_0_3 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_dGuard_hits_0_3 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_1_1)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_1 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_1 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_1 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_1 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_1 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_1 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_1 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_1 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_1 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_1 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_1 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_1 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_1 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_1 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_1 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_dGuard_hits_1 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_1_3)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_1_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_1_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_1_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_1_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_1_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_1_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_1_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_1_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_1_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_1_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_1_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_1_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_1_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_1_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_1_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_dGuard_hits_1_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_2_1)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_2 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_2 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_2 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_2 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_2 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_2 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_2 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_2 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_2 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_2 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_2 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_2 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_2 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_2 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_2 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_dGuard_hits_2 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_2_3)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_2_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_2_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_2_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_2_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_2_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_2_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_2_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_2_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_2_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_2_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_2_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_2_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_2_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_2_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_2_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_dGuard_hits_2_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_3_1)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_3 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_3 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_3 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_3 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_3 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_3 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_3 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_3 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_3 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_3 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_3 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_3 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_3 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_3 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_3 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_dGuard_hits_3 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_3_3)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_3_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_3_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_3_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_3_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_3_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_3_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_3_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_3_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_3_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_3_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_3_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_3_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_3_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_3_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_3_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_dGuard_hits_3_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_4_1)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_4 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_4 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_4 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_4 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_4 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_4 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_4 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_4 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_4 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_4 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_4 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_4 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_4 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_4 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_4 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_dGuard_hits_4 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_4_3)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_4_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_4_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_4_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_4_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_4_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_4_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_4_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_4_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_4_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_4_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_4_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_4_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_4_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_4_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_4_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_dGuard_hits_4_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_5_1)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_5 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_5 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_5 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_5 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_5 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_5 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_5 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_5 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_5 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_5 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_5 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_5 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_5 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_5 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_5 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_dGuard_hits_5 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_5_3)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_5_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_5_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_5_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_5_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_5_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_5_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_5_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_5_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_5_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_5_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_5_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_5_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_5_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_5_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_5_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_dGuard_hits_5_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_6_1)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_6 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_6 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_6 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_6 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_6 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_6 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_6 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_6 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_6 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_6 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_6 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_6 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_6 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_6 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_6 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_dGuard_hits_6 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_6_3)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_6_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_6_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_6_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_6_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_6_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_6_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_6_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_6_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_6_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_6_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_6_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_6_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_6_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_6_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_6_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_dGuard_hits_6_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_7_1)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_7 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_7 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_7 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_7 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_7 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_7 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_7 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_7 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_7 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_7 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_7 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_7 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_7 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_7 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_7 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_dGuard_hits_7 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_7_3)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_7_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_7_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_7_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_7_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_7_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_7_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_7_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_7_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_7_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_7_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_7_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_7_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_7_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_7_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_7_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_dGuard_hits_7_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_8_1)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_8 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_8 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_8 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_8 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_8 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_8 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_8 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_8 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_8 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_8 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_8 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_8 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_8 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_8 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_8 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_dGuard_hits_8 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_8_3)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_8_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_8_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_8_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_8_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_8_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_8_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_8_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_8_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_8_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_8_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_8_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_8_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_8_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_8_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_8_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_dGuard_hits_8_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_9_1)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_9 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_9 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_9 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_9 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_9 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_9 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_9 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_9 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_9 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_9 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_9 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_9 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_9 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_9 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_9 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_dGuard_hits_9 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_9_3)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_9_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_9_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_9_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_9_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_9_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_9_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_9_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_9_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_9_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_9_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_9_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_9_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_9_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_9_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_9_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_dGuard_hits_9_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_10_1)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_10 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_10 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_10 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_10 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_10 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_10 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_10 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_10 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_10 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_10 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_10 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_10 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_10 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_10 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_10 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_dGuard_hits_10 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_10_3)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_10_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_10_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_10_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_10_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_10_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_10_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_10_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_10_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_10_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_10_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_10_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_10_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_10_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_10_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_10_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_dGuard_hits_10_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_11_1)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_11 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_11 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_11 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_11 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_11 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_11 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_11 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_11 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_11 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_11 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_11 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_11 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_11 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_11 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_11 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_dGuard_hits_11 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_11_3)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_11_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_11_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_11_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_11_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_11_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_11_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_11_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_11_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_11_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_11_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_11_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_11_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_11_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_11_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_11_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_dGuard_hits_11_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_12_1)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_12 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_12 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_12 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_12 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_12 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_12 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_12 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_12 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_12 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_12 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_12 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_12 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_12 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_12 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_12 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_dGuard_hits_12 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_12_3)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_12_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_12_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_12_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_12_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_12_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_12_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_12_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_12_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_12_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_12_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_12_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_12_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_12_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_12_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_12_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_dGuard_hits_12_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_13_1)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_13 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_13 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_13 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_13 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_13 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_13 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_13 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_13 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_13 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_13 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_13 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_13 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_13 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_13 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_13 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_dGuard_hits_13 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_13_3)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_13_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_13_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_13_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_13_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_13_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_13_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_13_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_13_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_13_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_13_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_13_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_13_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_13_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_13_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_13_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_dGuard_hits_13_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_14_1)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_14 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_14 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_14 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_14 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_14 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_14 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_14 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_14 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_14 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_14 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_14 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_14 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_14 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_14 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_14 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_dGuard_hits_14 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_14_3)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_14_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_14_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_14_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_14_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_14_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_14_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_14_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_14_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_14_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_14_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_14_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_14_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_14_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_14_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_14_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_dGuard_hits_14_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_15_1)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_15 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_15 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_15 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_15 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_15 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_15 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_15 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_15 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_15 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_15 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_15 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_15 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_15 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_15 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_15 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_dGuard_hits_15 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_dGuard_hits_15_3)
      4'b0000 : _zz_PmpPlugin_dGuard_hits_15_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_dGuard_hits_15_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_dGuard_hits_15_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_dGuard_hits_15_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_dGuard_hits_15_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_dGuard_hits_15_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_dGuard_hits_15_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_dGuard_hits_15_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_dGuard_hits_15_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_dGuard_hits_15_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_dGuard_hits_15_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_dGuard_hits_15_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_dGuard_hits_15_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_dGuard_hits_15_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_dGuard_hits_15_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_dGuard_hits_15_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_DBusCachedPlugin_mmuBus_rsp_allowRead_18)
      4'b0000 : _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_17 = PmpPlugin_pmpcfg_0[0];
      4'b0001 : _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_17 = PmpPlugin_pmpcfg_1[0];
      4'b0010 : _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_17 = PmpPlugin_pmpcfg_2[0];
      4'b0011 : _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_17 = PmpPlugin_pmpcfg_3[0];
      4'b0100 : _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_17 = PmpPlugin_pmpcfg_4[0];
      4'b0101 : _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_17 = PmpPlugin_pmpcfg_5[0];
      4'b0110 : _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_17 = PmpPlugin_pmpcfg_6[0];
      4'b0111 : _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_17 = PmpPlugin_pmpcfg_7[0];
      4'b1000 : _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_17 = PmpPlugin_pmpcfg_8[0];
      4'b1001 : _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_17 = PmpPlugin_pmpcfg_9[0];
      4'b1010 : _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_17 = PmpPlugin_pmpcfg_10[0];
      4'b1011 : _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_17 = PmpPlugin_pmpcfg_11[0];
      4'b1100 : _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_17 = PmpPlugin_pmpcfg_12[0];
      4'b1101 : _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_17 = PmpPlugin_pmpcfg_13[0];
      4'b1110 : _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_17 = PmpPlugin_pmpcfg_14[0];
      default : _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_17 = PmpPlugin_pmpcfg_15[0];
    endcase
  end

  always @(*) begin
    case(_zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_18)
      4'b0000 : _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_17 = PmpPlugin_pmpcfg_0[1];
      4'b0001 : _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_17 = PmpPlugin_pmpcfg_1[1];
      4'b0010 : _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_17 = PmpPlugin_pmpcfg_2[1];
      4'b0011 : _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_17 = PmpPlugin_pmpcfg_3[1];
      4'b0100 : _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_17 = PmpPlugin_pmpcfg_4[1];
      4'b0101 : _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_17 = PmpPlugin_pmpcfg_5[1];
      4'b0110 : _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_17 = PmpPlugin_pmpcfg_6[1];
      4'b0111 : _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_17 = PmpPlugin_pmpcfg_7[1];
      4'b1000 : _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_17 = PmpPlugin_pmpcfg_8[1];
      4'b1001 : _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_17 = PmpPlugin_pmpcfg_9[1];
      4'b1010 : _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_17 = PmpPlugin_pmpcfg_10[1];
      4'b1011 : _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_17 = PmpPlugin_pmpcfg_11[1];
      4'b1100 : _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_17 = PmpPlugin_pmpcfg_12[1];
      4'b1101 : _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_17 = PmpPlugin_pmpcfg_13[1];
      4'b1110 : _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_17 = PmpPlugin_pmpcfg_14[1];
      default : _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_17 = PmpPlugin_pmpcfg_15[1];
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_0_2)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_0_1 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_0_1 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_0_1 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_0_1 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_0_1 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_0_1 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_0_1 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_0_1 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_0_1 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_0_1 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_0_1 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_0_1 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_0_1 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_0_1 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_0_1 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_iGuard_hits_0_1 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_0_4)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_0_3 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_0_3 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_0_3 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_0_3 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_0_3 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_0_3 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_0_3 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_0_3 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_0_3 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_0_3 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_0_3 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_0_3 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_0_3 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_0_3 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_0_3 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_iGuard_hits_0_3 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_1_1)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_1 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_1 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_1 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_1 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_1 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_1 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_1 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_1 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_1 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_1 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_1 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_1 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_1 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_1 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_1 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_iGuard_hits_1 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_1_3)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_1_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_1_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_1_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_1_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_1_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_1_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_1_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_1_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_1_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_1_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_1_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_1_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_1_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_1_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_1_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_iGuard_hits_1_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_2_1)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_2 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_2 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_2 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_2 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_2 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_2 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_2 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_2 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_2 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_2 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_2 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_2 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_2 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_2 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_2 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_iGuard_hits_2 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_2_3)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_2_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_2_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_2_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_2_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_2_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_2_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_2_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_2_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_2_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_2_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_2_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_2_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_2_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_2_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_2_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_iGuard_hits_2_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_3_1)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_3 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_3 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_3 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_3 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_3 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_3 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_3 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_3 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_3 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_3 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_3 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_3 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_3 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_3 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_3 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_iGuard_hits_3 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_3_3)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_3_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_3_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_3_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_3_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_3_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_3_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_3_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_3_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_3_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_3_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_3_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_3_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_3_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_3_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_3_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_iGuard_hits_3_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_4_1)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_4 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_4 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_4 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_4 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_4 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_4 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_4 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_4 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_4 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_4 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_4 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_4 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_4 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_4 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_4 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_iGuard_hits_4 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_4_3)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_4_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_4_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_4_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_4_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_4_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_4_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_4_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_4_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_4_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_4_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_4_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_4_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_4_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_4_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_4_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_iGuard_hits_4_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_5_1)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_5 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_5 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_5 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_5 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_5 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_5 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_5 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_5 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_5 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_5 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_5 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_5 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_5 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_5 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_5 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_iGuard_hits_5 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_5_3)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_5_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_5_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_5_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_5_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_5_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_5_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_5_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_5_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_5_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_5_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_5_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_5_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_5_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_5_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_5_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_iGuard_hits_5_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_6_1)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_6 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_6 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_6 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_6 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_6 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_6 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_6 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_6 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_6 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_6 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_6 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_6 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_6 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_6 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_6 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_iGuard_hits_6 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_6_3)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_6_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_6_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_6_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_6_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_6_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_6_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_6_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_6_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_6_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_6_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_6_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_6_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_6_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_6_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_6_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_iGuard_hits_6_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_7_1)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_7 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_7 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_7 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_7 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_7 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_7 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_7 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_7 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_7 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_7 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_7 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_7 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_7 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_7 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_7 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_iGuard_hits_7 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_7_3)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_7_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_7_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_7_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_7_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_7_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_7_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_7_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_7_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_7_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_7_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_7_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_7_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_7_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_7_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_7_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_iGuard_hits_7_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_8_1)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_8 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_8 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_8 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_8 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_8 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_8 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_8 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_8 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_8 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_8 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_8 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_8 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_8 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_8 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_8 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_iGuard_hits_8 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_8_3)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_8_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_8_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_8_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_8_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_8_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_8_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_8_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_8_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_8_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_8_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_8_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_8_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_8_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_8_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_8_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_iGuard_hits_8_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_9_1)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_9 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_9 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_9 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_9 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_9 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_9 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_9 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_9 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_9 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_9 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_9 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_9 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_9 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_9 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_9 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_iGuard_hits_9 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_9_3)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_9_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_9_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_9_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_9_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_9_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_9_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_9_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_9_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_9_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_9_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_9_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_9_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_9_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_9_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_9_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_iGuard_hits_9_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_10_1)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_10 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_10 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_10 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_10 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_10 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_10 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_10 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_10 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_10 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_10 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_10 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_10 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_10 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_10 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_10 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_iGuard_hits_10 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_10_3)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_10_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_10_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_10_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_10_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_10_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_10_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_10_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_10_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_10_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_10_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_10_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_10_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_10_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_10_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_10_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_iGuard_hits_10_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_11_1)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_11 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_11 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_11 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_11 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_11 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_11 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_11 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_11 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_11 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_11 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_11 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_11 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_11 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_11 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_11 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_iGuard_hits_11 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_11_3)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_11_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_11_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_11_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_11_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_11_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_11_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_11_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_11_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_11_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_11_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_11_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_11_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_11_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_11_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_11_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_iGuard_hits_11_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_12_1)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_12 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_12 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_12 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_12 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_12 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_12 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_12 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_12 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_12 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_12 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_12 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_12 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_12 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_12 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_12 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_iGuard_hits_12 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_12_3)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_12_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_12_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_12_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_12_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_12_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_12_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_12_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_12_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_12_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_12_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_12_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_12_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_12_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_12_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_12_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_iGuard_hits_12_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_13_1)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_13 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_13 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_13 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_13 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_13 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_13 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_13 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_13 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_13 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_13 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_13 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_13 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_13 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_13 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_13 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_iGuard_hits_13 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_13_3)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_13_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_13_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_13_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_13_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_13_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_13_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_13_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_13_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_13_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_13_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_13_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_13_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_13_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_13_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_13_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_iGuard_hits_13_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_14_1)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_14 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_14 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_14 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_14 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_14 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_14 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_14 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_14 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_14 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_14 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_14 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_14 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_14 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_14 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_14 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_iGuard_hits_14 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_14_3)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_14_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_14_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_14_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_14_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_14_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_14_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_14_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_14_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_14_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_14_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_14_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_14_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_14_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_14_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_14_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_iGuard_hits_14_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_15_1)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_15 = PmpPlugin_mask_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_15 = PmpPlugin_mask_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_15 = PmpPlugin_mask_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_15 = PmpPlugin_mask_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_15 = PmpPlugin_mask_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_15 = PmpPlugin_mask_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_15 = PmpPlugin_mask_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_15 = PmpPlugin_mask_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_15 = PmpPlugin_mask_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_15 = PmpPlugin_mask_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_15 = PmpPlugin_mask_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_15 = PmpPlugin_mask_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_15 = PmpPlugin_mask_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_15 = PmpPlugin_mask_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_15 = PmpPlugin_mask_14;
      default : _zz_PmpPlugin_iGuard_hits_15 = PmpPlugin_mask_15;
    endcase
  end

  always @(*) begin
    case(_zz_PmpPlugin_iGuard_hits_15_3)
      4'b0000 : _zz_PmpPlugin_iGuard_hits_15_2 = PmpPlugin_base_0;
      4'b0001 : _zz_PmpPlugin_iGuard_hits_15_2 = PmpPlugin_base_1;
      4'b0010 : _zz_PmpPlugin_iGuard_hits_15_2 = PmpPlugin_base_2;
      4'b0011 : _zz_PmpPlugin_iGuard_hits_15_2 = PmpPlugin_base_3;
      4'b0100 : _zz_PmpPlugin_iGuard_hits_15_2 = PmpPlugin_base_4;
      4'b0101 : _zz_PmpPlugin_iGuard_hits_15_2 = PmpPlugin_base_5;
      4'b0110 : _zz_PmpPlugin_iGuard_hits_15_2 = PmpPlugin_base_6;
      4'b0111 : _zz_PmpPlugin_iGuard_hits_15_2 = PmpPlugin_base_7;
      4'b1000 : _zz_PmpPlugin_iGuard_hits_15_2 = PmpPlugin_base_8;
      4'b1001 : _zz_PmpPlugin_iGuard_hits_15_2 = PmpPlugin_base_9;
      4'b1010 : _zz_PmpPlugin_iGuard_hits_15_2 = PmpPlugin_base_10;
      4'b1011 : _zz_PmpPlugin_iGuard_hits_15_2 = PmpPlugin_base_11;
      4'b1100 : _zz_PmpPlugin_iGuard_hits_15_2 = PmpPlugin_base_12;
      4'b1101 : _zz_PmpPlugin_iGuard_hits_15_2 = PmpPlugin_base_13;
      4'b1110 : _zz_PmpPlugin_iGuard_hits_15_2 = PmpPlugin_base_14;
      default : _zz_PmpPlugin_iGuard_hits_15_2 = PmpPlugin_base_15;
    endcase
  end

  always @(*) begin
    case(_zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_18)
      4'b0000 : _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_17 = PmpPlugin_pmpcfg_0[2];
      4'b0001 : _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_17 = PmpPlugin_pmpcfg_1[2];
      4'b0010 : _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_17 = PmpPlugin_pmpcfg_2[2];
      4'b0011 : _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_17 = PmpPlugin_pmpcfg_3[2];
      4'b0100 : _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_17 = PmpPlugin_pmpcfg_4[2];
      4'b0101 : _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_17 = PmpPlugin_pmpcfg_5[2];
      4'b0110 : _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_17 = PmpPlugin_pmpcfg_6[2];
      4'b0111 : _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_17 = PmpPlugin_pmpcfg_7[2];
      4'b1000 : _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_17 = PmpPlugin_pmpcfg_8[2];
      4'b1001 : _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_17 = PmpPlugin_pmpcfg_9[2];
      4'b1010 : _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_17 = PmpPlugin_pmpcfg_10[2];
      4'b1011 : _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_17 = PmpPlugin_pmpcfg_11[2];
      4'b1100 : _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_17 = PmpPlugin_pmpcfg_12[2];
      4'b1101 : _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_17 = PmpPlugin_pmpcfg_13[2];
      4'b1110 : _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_17 = PmpPlugin_pmpcfg_14[2];
      default : _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_17 = PmpPlugin_pmpcfg_15[2];
    endcase
  end

  always @(*) begin
    case(_zz_when_PmpPlugin_l209_1)
      4'b0000 : _zz_when_PmpPlugin_l209 = PmpPlugin_pmpcfg_0;
      4'b0001 : _zz_when_PmpPlugin_l209 = PmpPlugin_pmpcfg_1;
      4'b0010 : _zz_when_PmpPlugin_l209 = PmpPlugin_pmpcfg_2;
      4'b0011 : _zz_when_PmpPlugin_l209 = PmpPlugin_pmpcfg_3;
      4'b0100 : _zz_when_PmpPlugin_l209 = PmpPlugin_pmpcfg_4;
      4'b0101 : _zz_when_PmpPlugin_l209 = PmpPlugin_pmpcfg_5;
      4'b0110 : _zz_when_PmpPlugin_l209 = PmpPlugin_pmpcfg_6;
      4'b0111 : _zz_when_PmpPlugin_l209 = PmpPlugin_pmpcfg_7;
      4'b1000 : _zz_when_PmpPlugin_l209 = PmpPlugin_pmpcfg_8;
      4'b1001 : _zz_when_PmpPlugin_l209 = PmpPlugin_pmpcfg_9;
      4'b1010 : _zz_when_PmpPlugin_l209 = PmpPlugin_pmpcfg_10;
      4'b1011 : _zz_when_PmpPlugin_l209 = PmpPlugin_pmpcfg_11;
      4'b1100 : _zz_when_PmpPlugin_l209 = PmpPlugin_pmpcfg_12;
      4'b1101 : _zz_when_PmpPlugin_l209 = PmpPlugin_pmpcfg_13;
      4'b1110 : _zz_when_PmpPlugin_l209 = PmpPlugin_pmpcfg_14;
      default : _zz_when_PmpPlugin_l209 = PmpPlugin_pmpcfg_15;
    endcase
  end

  always @(*) begin
    case(_zz_when_PmpPlugin_l209_1_2)
      4'b0000 : _zz_when_PmpPlugin_l209_1_1 = PmpPlugin_pmpcfg_0;
      4'b0001 : _zz_when_PmpPlugin_l209_1_1 = PmpPlugin_pmpcfg_1;
      4'b0010 : _zz_when_PmpPlugin_l209_1_1 = PmpPlugin_pmpcfg_2;
      4'b0011 : _zz_when_PmpPlugin_l209_1_1 = PmpPlugin_pmpcfg_3;
      4'b0100 : _zz_when_PmpPlugin_l209_1_1 = PmpPlugin_pmpcfg_4;
      4'b0101 : _zz_when_PmpPlugin_l209_1_1 = PmpPlugin_pmpcfg_5;
      4'b0110 : _zz_when_PmpPlugin_l209_1_1 = PmpPlugin_pmpcfg_6;
      4'b0111 : _zz_when_PmpPlugin_l209_1_1 = PmpPlugin_pmpcfg_7;
      4'b1000 : _zz_when_PmpPlugin_l209_1_1 = PmpPlugin_pmpcfg_8;
      4'b1001 : _zz_when_PmpPlugin_l209_1_1 = PmpPlugin_pmpcfg_9;
      4'b1010 : _zz_when_PmpPlugin_l209_1_1 = PmpPlugin_pmpcfg_10;
      4'b1011 : _zz_when_PmpPlugin_l209_1_1 = PmpPlugin_pmpcfg_11;
      4'b1100 : _zz_when_PmpPlugin_l209_1_1 = PmpPlugin_pmpcfg_12;
      4'b1101 : _zz_when_PmpPlugin_l209_1_1 = PmpPlugin_pmpcfg_13;
      4'b1110 : _zz_when_PmpPlugin_l209_1_1 = PmpPlugin_pmpcfg_14;
      default : _zz_when_PmpPlugin_l209_1_1 = PmpPlugin_pmpcfg_15;
    endcase
  end

  always @(*) begin
    case(_zz_when_PmpPlugin_l209_2_1)
      4'b0000 : _zz_when_PmpPlugin_l209_2 = PmpPlugin_pmpcfg_0;
      4'b0001 : _zz_when_PmpPlugin_l209_2 = PmpPlugin_pmpcfg_1;
      4'b0010 : _zz_when_PmpPlugin_l209_2 = PmpPlugin_pmpcfg_2;
      4'b0011 : _zz_when_PmpPlugin_l209_2 = PmpPlugin_pmpcfg_3;
      4'b0100 : _zz_when_PmpPlugin_l209_2 = PmpPlugin_pmpcfg_4;
      4'b0101 : _zz_when_PmpPlugin_l209_2 = PmpPlugin_pmpcfg_5;
      4'b0110 : _zz_when_PmpPlugin_l209_2 = PmpPlugin_pmpcfg_6;
      4'b0111 : _zz_when_PmpPlugin_l209_2 = PmpPlugin_pmpcfg_7;
      4'b1000 : _zz_when_PmpPlugin_l209_2 = PmpPlugin_pmpcfg_8;
      4'b1001 : _zz_when_PmpPlugin_l209_2 = PmpPlugin_pmpcfg_9;
      4'b1010 : _zz_when_PmpPlugin_l209_2 = PmpPlugin_pmpcfg_10;
      4'b1011 : _zz_when_PmpPlugin_l209_2 = PmpPlugin_pmpcfg_11;
      4'b1100 : _zz_when_PmpPlugin_l209_2 = PmpPlugin_pmpcfg_12;
      4'b1101 : _zz_when_PmpPlugin_l209_2 = PmpPlugin_pmpcfg_13;
      4'b1110 : _zz_when_PmpPlugin_l209_2 = PmpPlugin_pmpcfg_14;
      default : _zz_when_PmpPlugin_l209_2 = PmpPlugin_pmpcfg_15;
    endcase
  end

  always @(*) begin
    case(_zz_when_PmpPlugin_l209_3_1)
      4'b0000 : _zz_when_PmpPlugin_l209_3 = PmpPlugin_pmpcfg_0;
      4'b0001 : _zz_when_PmpPlugin_l209_3 = PmpPlugin_pmpcfg_1;
      4'b0010 : _zz_when_PmpPlugin_l209_3 = PmpPlugin_pmpcfg_2;
      4'b0011 : _zz_when_PmpPlugin_l209_3 = PmpPlugin_pmpcfg_3;
      4'b0100 : _zz_when_PmpPlugin_l209_3 = PmpPlugin_pmpcfg_4;
      4'b0101 : _zz_when_PmpPlugin_l209_3 = PmpPlugin_pmpcfg_5;
      4'b0110 : _zz_when_PmpPlugin_l209_3 = PmpPlugin_pmpcfg_6;
      4'b0111 : _zz_when_PmpPlugin_l209_3 = PmpPlugin_pmpcfg_7;
      4'b1000 : _zz_when_PmpPlugin_l209_3 = PmpPlugin_pmpcfg_8;
      4'b1001 : _zz_when_PmpPlugin_l209_3 = PmpPlugin_pmpcfg_9;
      4'b1010 : _zz_when_PmpPlugin_l209_3 = PmpPlugin_pmpcfg_10;
      4'b1011 : _zz_when_PmpPlugin_l209_3 = PmpPlugin_pmpcfg_11;
      4'b1100 : _zz_when_PmpPlugin_l209_3 = PmpPlugin_pmpcfg_12;
      4'b1101 : _zz_when_PmpPlugin_l209_3 = PmpPlugin_pmpcfg_13;
      4'b1110 : _zz_when_PmpPlugin_l209_3 = PmpPlugin_pmpcfg_14;
      default : _zz_when_PmpPlugin_l209_3 = PmpPlugin_pmpcfg_15;
    endcase
  end

  always @(*) begin
    case(execute_PmpPlugin_pmpNcfg_)
      4'b0000 : _zz_when_PmpPlugin_l216 = PmpPlugin_pmpcfg_0;
      4'b0001 : _zz_when_PmpPlugin_l216 = PmpPlugin_pmpcfg_1;
      4'b0010 : _zz_when_PmpPlugin_l216 = PmpPlugin_pmpcfg_2;
      4'b0011 : _zz_when_PmpPlugin_l216 = PmpPlugin_pmpcfg_3;
      4'b0100 : _zz_when_PmpPlugin_l216 = PmpPlugin_pmpcfg_4;
      4'b0101 : _zz_when_PmpPlugin_l216 = PmpPlugin_pmpcfg_5;
      4'b0110 : _zz_when_PmpPlugin_l216 = PmpPlugin_pmpcfg_6;
      4'b0111 : _zz_when_PmpPlugin_l216 = PmpPlugin_pmpcfg_7;
      4'b1000 : _zz_when_PmpPlugin_l216 = PmpPlugin_pmpcfg_8;
      4'b1001 : _zz_when_PmpPlugin_l216 = PmpPlugin_pmpcfg_9;
      4'b1010 : _zz_when_PmpPlugin_l216 = PmpPlugin_pmpcfg_10;
      4'b1011 : _zz_when_PmpPlugin_l216 = PmpPlugin_pmpcfg_11;
      4'b1100 : _zz_when_PmpPlugin_l216 = PmpPlugin_pmpcfg_12;
      4'b1101 : _zz_when_PmpPlugin_l216 = PmpPlugin_pmpcfg_13;
      4'b1110 : _zz_when_PmpPlugin_l216 = PmpPlugin_pmpcfg_14;
      default : _zz_when_PmpPlugin_l216 = PmpPlugin_pmpcfg_15;
    endcase
  end

  always @(*) begin
    case(_zz_CsrPlugin_csrMapping_readDataSignal_1)
      4'b0000 : _zz_CsrPlugin_csrMapping_readDataSignal = PmpPlugin_pmpcfg_0;
      4'b0001 : _zz_CsrPlugin_csrMapping_readDataSignal = PmpPlugin_pmpcfg_1;
      4'b0010 : _zz_CsrPlugin_csrMapping_readDataSignal = PmpPlugin_pmpcfg_2;
      4'b0011 : _zz_CsrPlugin_csrMapping_readDataSignal = PmpPlugin_pmpcfg_3;
      4'b0100 : _zz_CsrPlugin_csrMapping_readDataSignal = PmpPlugin_pmpcfg_4;
      4'b0101 : _zz_CsrPlugin_csrMapping_readDataSignal = PmpPlugin_pmpcfg_5;
      4'b0110 : _zz_CsrPlugin_csrMapping_readDataSignal = PmpPlugin_pmpcfg_6;
      4'b0111 : _zz_CsrPlugin_csrMapping_readDataSignal = PmpPlugin_pmpcfg_7;
      4'b1000 : _zz_CsrPlugin_csrMapping_readDataSignal = PmpPlugin_pmpcfg_8;
      4'b1001 : _zz_CsrPlugin_csrMapping_readDataSignal = PmpPlugin_pmpcfg_9;
      4'b1010 : _zz_CsrPlugin_csrMapping_readDataSignal = PmpPlugin_pmpcfg_10;
      4'b1011 : _zz_CsrPlugin_csrMapping_readDataSignal = PmpPlugin_pmpcfg_11;
      4'b1100 : _zz_CsrPlugin_csrMapping_readDataSignal = PmpPlugin_pmpcfg_12;
      4'b1101 : _zz_CsrPlugin_csrMapping_readDataSignal = PmpPlugin_pmpcfg_13;
      4'b1110 : _zz_CsrPlugin_csrMapping_readDataSignal = PmpPlugin_pmpcfg_14;
      default : _zz_CsrPlugin_csrMapping_readDataSignal = PmpPlugin_pmpcfg_15;
    endcase
  end

  always @(*) begin
    case(_zz_CsrPlugin_csrMapping_readDataSignal_3)
      4'b0000 : _zz_CsrPlugin_csrMapping_readDataSignal_2 = PmpPlugin_pmpcfg_0;
      4'b0001 : _zz_CsrPlugin_csrMapping_readDataSignal_2 = PmpPlugin_pmpcfg_1;
      4'b0010 : _zz_CsrPlugin_csrMapping_readDataSignal_2 = PmpPlugin_pmpcfg_2;
      4'b0011 : _zz_CsrPlugin_csrMapping_readDataSignal_2 = PmpPlugin_pmpcfg_3;
      4'b0100 : _zz_CsrPlugin_csrMapping_readDataSignal_2 = PmpPlugin_pmpcfg_4;
      4'b0101 : _zz_CsrPlugin_csrMapping_readDataSignal_2 = PmpPlugin_pmpcfg_5;
      4'b0110 : _zz_CsrPlugin_csrMapping_readDataSignal_2 = PmpPlugin_pmpcfg_6;
      4'b0111 : _zz_CsrPlugin_csrMapping_readDataSignal_2 = PmpPlugin_pmpcfg_7;
      4'b1000 : _zz_CsrPlugin_csrMapping_readDataSignal_2 = PmpPlugin_pmpcfg_8;
      4'b1001 : _zz_CsrPlugin_csrMapping_readDataSignal_2 = PmpPlugin_pmpcfg_9;
      4'b1010 : _zz_CsrPlugin_csrMapping_readDataSignal_2 = PmpPlugin_pmpcfg_10;
      4'b1011 : _zz_CsrPlugin_csrMapping_readDataSignal_2 = PmpPlugin_pmpcfg_11;
      4'b1100 : _zz_CsrPlugin_csrMapping_readDataSignal_2 = PmpPlugin_pmpcfg_12;
      4'b1101 : _zz_CsrPlugin_csrMapping_readDataSignal_2 = PmpPlugin_pmpcfg_13;
      4'b1110 : _zz_CsrPlugin_csrMapping_readDataSignal_2 = PmpPlugin_pmpcfg_14;
      default : _zz_CsrPlugin_csrMapping_readDataSignal_2 = PmpPlugin_pmpcfg_15;
    endcase
  end

  always @(*) begin
    case(_zz_CsrPlugin_csrMapping_readDataSignal_5)
      4'b0000 : _zz_CsrPlugin_csrMapping_readDataSignal_4 = PmpPlugin_pmpcfg_0;
      4'b0001 : _zz_CsrPlugin_csrMapping_readDataSignal_4 = PmpPlugin_pmpcfg_1;
      4'b0010 : _zz_CsrPlugin_csrMapping_readDataSignal_4 = PmpPlugin_pmpcfg_2;
      4'b0011 : _zz_CsrPlugin_csrMapping_readDataSignal_4 = PmpPlugin_pmpcfg_3;
      4'b0100 : _zz_CsrPlugin_csrMapping_readDataSignal_4 = PmpPlugin_pmpcfg_4;
      4'b0101 : _zz_CsrPlugin_csrMapping_readDataSignal_4 = PmpPlugin_pmpcfg_5;
      4'b0110 : _zz_CsrPlugin_csrMapping_readDataSignal_4 = PmpPlugin_pmpcfg_6;
      4'b0111 : _zz_CsrPlugin_csrMapping_readDataSignal_4 = PmpPlugin_pmpcfg_7;
      4'b1000 : _zz_CsrPlugin_csrMapping_readDataSignal_4 = PmpPlugin_pmpcfg_8;
      4'b1001 : _zz_CsrPlugin_csrMapping_readDataSignal_4 = PmpPlugin_pmpcfg_9;
      4'b1010 : _zz_CsrPlugin_csrMapping_readDataSignal_4 = PmpPlugin_pmpcfg_10;
      4'b1011 : _zz_CsrPlugin_csrMapping_readDataSignal_4 = PmpPlugin_pmpcfg_11;
      4'b1100 : _zz_CsrPlugin_csrMapping_readDataSignal_4 = PmpPlugin_pmpcfg_12;
      4'b1101 : _zz_CsrPlugin_csrMapping_readDataSignal_4 = PmpPlugin_pmpcfg_13;
      4'b1110 : _zz_CsrPlugin_csrMapping_readDataSignal_4 = PmpPlugin_pmpcfg_14;
      default : _zz_CsrPlugin_csrMapping_readDataSignal_4 = PmpPlugin_pmpcfg_15;
    endcase
  end

  always @(*) begin
    case(_zz_CsrPlugin_csrMapping_readDataSignal_7)
      4'b0000 : _zz_CsrPlugin_csrMapping_readDataSignal_6 = PmpPlugin_pmpcfg_0;
      4'b0001 : _zz_CsrPlugin_csrMapping_readDataSignal_6 = PmpPlugin_pmpcfg_1;
      4'b0010 : _zz_CsrPlugin_csrMapping_readDataSignal_6 = PmpPlugin_pmpcfg_2;
      4'b0011 : _zz_CsrPlugin_csrMapping_readDataSignal_6 = PmpPlugin_pmpcfg_3;
      4'b0100 : _zz_CsrPlugin_csrMapping_readDataSignal_6 = PmpPlugin_pmpcfg_4;
      4'b0101 : _zz_CsrPlugin_csrMapping_readDataSignal_6 = PmpPlugin_pmpcfg_5;
      4'b0110 : _zz_CsrPlugin_csrMapping_readDataSignal_6 = PmpPlugin_pmpcfg_6;
      4'b0111 : _zz_CsrPlugin_csrMapping_readDataSignal_6 = PmpPlugin_pmpcfg_7;
      4'b1000 : _zz_CsrPlugin_csrMapping_readDataSignal_6 = PmpPlugin_pmpcfg_8;
      4'b1001 : _zz_CsrPlugin_csrMapping_readDataSignal_6 = PmpPlugin_pmpcfg_9;
      4'b1010 : _zz_CsrPlugin_csrMapping_readDataSignal_6 = PmpPlugin_pmpcfg_10;
      4'b1011 : _zz_CsrPlugin_csrMapping_readDataSignal_6 = PmpPlugin_pmpcfg_11;
      4'b1100 : _zz_CsrPlugin_csrMapping_readDataSignal_6 = PmpPlugin_pmpcfg_12;
      4'b1101 : _zz_CsrPlugin_csrMapping_readDataSignal_6 = PmpPlugin_pmpcfg_13;
      4'b1110 : _zz_CsrPlugin_csrMapping_readDataSignal_6 = PmpPlugin_pmpcfg_14;
      default : _zz_CsrPlugin_csrMapping_readDataSignal_6 = PmpPlugin_pmpcfg_15;
    endcase
  end

  `ifndef SYNTHESIS
  always @(*) begin
    case(_zz_decode_to_execute_BRANCH_CTRL)
      BranchCtrlEnum_INC : _zz_decode_to_execute_BRANCH_CTRL_string = "INC ";
      BranchCtrlEnum_B : _zz_decode_to_execute_BRANCH_CTRL_string = "B   ";
      BranchCtrlEnum_JAL : _zz_decode_to_execute_BRANCH_CTRL_string = "JAL ";
      BranchCtrlEnum_JALR : _zz_decode_to_execute_BRANCH_CTRL_string = "JALR";
      default : _zz_decode_to_execute_BRANCH_CTRL_string = "????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_to_execute_BRANCH_CTRL_1)
      BranchCtrlEnum_INC : _zz_decode_to_execute_BRANCH_CTRL_1_string = "INC ";
      BranchCtrlEnum_B : _zz_decode_to_execute_BRANCH_CTRL_1_string = "B   ";
      BranchCtrlEnum_JAL : _zz_decode_to_execute_BRANCH_CTRL_1_string = "JAL ";
      BranchCtrlEnum_JALR : _zz_decode_to_execute_BRANCH_CTRL_1_string = "JALR";
      default : _zz_decode_to_execute_BRANCH_CTRL_1_string = "????";
    endcase
  end
  always @(*) begin
    case(_zz_memory_to_writeBack_ENV_CTRL)
      EnvCtrlEnum_NONE : _zz_memory_to_writeBack_ENV_CTRL_string = "NONE ";
      EnvCtrlEnum_XRET : _zz_memory_to_writeBack_ENV_CTRL_string = "XRET ";
      EnvCtrlEnum_WFI : _zz_memory_to_writeBack_ENV_CTRL_string = "WFI  ";
      EnvCtrlEnum_ECALL : _zz_memory_to_writeBack_ENV_CTRL_string = "ECALL";
      default : _zz_memory_to_writeBack_ENV_CTRL_string = "?????";
    endcase
  end
  always @(*) begin
    case(_zz_memory_to_writeBack_ENV_CTRL_1)
      EnvCtrlEnum_NONE : _zz_memory_to_writeBack_ENV_CTRL_1_string = "NONE ";
      EnvCtrlEnum_XRET : _zz_memory_to_writeBack_ENV_CTRL_1_string = "XRET ";
      EnvCtrlEnum_WFI : _zz_memory_to_writeBack_ENV_CTRL_1_string = "WFI  ";
      EnvCtrlEnum_ECALL : _zz_memory_to_writeBack_ENV_CTRL_1_string = "ECALL";
      default : _zz_memory_to_writeBack_ENV_CTRL_1_string = "?????";
    endcase
  end
  always @(*) begin
    case(_zz_execute_to_memory_ENV_CTRL)
      EnvCtrlEnum_NONE : _zz_execute_to_memory_ENV_CTRL_string = "NONE ";
      EnvCtrlEnum_XRET : _zz_execute_to_memory_ENV_CTRL_string = "XRET ";
      EnvCtrlEnum_WFI : _zz_execute_to_memory_ENV_CTRL_string = "WFI  ";
      EnvCtrlEnum_ECALL : _zz_execute_to_memory_ENV_CTRL_string = "ECALL";
      default : _zz_execute_to_memory_ENV_CTRL_string = "?????";
    endcase
  end
  always @(*) begin
    case(_zz_execute_to_memory_ENV_CTRL_1)
      EnvCtrlEnum_NONE : _zz_execute_to_memory_ENV_CTRL_1_string = "NONE ";
      EnvCtrlEnum_XRET : _zz_execute_to_memory_ENV_CTRL_1_string = "XRET ";
      EnvCtrlEnum_WFI : _zz_execute_to_memory_ENV_CTRL_1_string = "WFI  ";
      EnvCtrlEnum_ECALL : _zz_execute_to_memory_ENV_CTRL_1_string = "ECALL";
      default : _zz_execute_to_memory_ENV_CTRL_1_string = "?????";
    endcase
  end
  always @(*) begin
    case(decode_ENV_CTRL)
      EnvCtrlEnum_NONE : decode_ENV_CTRL_string = "NONE ";
      EnvCtrlEnum_XRET : decode_ENV_CTRL_string = "XRET ";
      EnvCtrlEnum_WFI : decode_ENV_CTRL_string = "WFI  ";
      EnvCtrlEnum_ECALL : decode_ENV_CTRL_string = "ECALL";
      default : decode_ENV_CTRL_string = "?????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_ENV_CTRL)
      EnvCtrlEnum_NONE : _zz_decode_ENV_CTRL_string = "NONE ";
      EnvCtrlEnum_XRET : _zz_decode_ENV_CTRL_string = "XRET ";
      EnvCtrlEnum_WFI : _zz_decode_ENV_CTRL_string = "WFI  ";
      EnvCtrlEnum_ECALL : _zz_decode_ENV_CTRL_string = "ECALL";
      default : _zz_decode_ENV_CTRL_string = "?????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_to_execute_ENV_CTRL)
      EnvCtrlEnum_NONE : _zz_decode_to_execute_ENV_CTRL_string = "NONE ";
      EnvCtrlEnum_XRET : _zz_decode_to_execute_ENV_CTRL_string = "XRET ";
      EnvCtrlEnum_WFI : _zz_decode_to_execute_ENV_CTRL_string = "WFI  ";
      EnvCtrlEnum_ECALL : _zz_decode_to_execute_ENV_CTRL_string = "ECALL";
      default : _zz_decode_to_execute_ENV_CTRL_string = "?????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_to_execute_ENV_CTRL_1)
      EnvCtrlEnum_NONE : _zz_decode_to_execute_ENV_CTRL_1_string = "NONE ";
      EnvCtrlEnum_XRET : _zz_decode_to_execute_ENV_CTRL_1_string = "XRET ";
      EnvCtrlEnum_WFI : _zz_decode_to_execute_ENV_CTRL_1_string = "WFI  ";
      EnvCtrlEnum_ECALL : _zz_decode_to_execute_ENV_CTRL_1_string = "ECALL";
      default : _zz_decode_to_execute_ENV_CTRL_1_string = "?????";
    endcase
  end
  always @(*) begin
    case(_zz_execute_to_memory_SHIFT_CTRL)
      ShiftCtrlEnum_DISABLE_1 : _zz_execute_to_memory_SHIFT_CTRL_string = "DISABLE_1";
      ShiftCtrlEnum_SLL_1 : _zz_execute_to_memory_SHIFT_CTRL_string = "SLL_1    ";
      ShiftCtrlEnum_SRL_1 : _zz_execute_to_memory_SHIFT_CTRL_string = "SRL_1    ";
      ShiftCtrlEnum_SRA_1 : _zz_execute_to_memory_SHIFT_CTRL_string = "SRA_1    ";
      default : _zz_execute_to_memory_SHIFT_CTRL_string = "?????????";
    endcase
  end
  always @(*) begin
    case(_zz_execute_to_memory_SHIFT_CTRL_1)
      ShiftCtrlEnum_DISABLE_1 : _zz_execute_to_memory_SHIFT_CTRL_1_string = "DISABLE_1";
      ShiftCtrlEnum_SLL_1 : _zz_execute_to_memory_SHIFT_CTRL_1_string = "SLL_1    ";
      ShiftCtrlEnum_SRL_1 : _zz_execute_to_memory_SHIFT_CTRL_1_string = "SRL_1    ";
      ShiftCtrlEnum_SRA_1 : _zz_execute_to_memory_SHIFT_CTRL_1_string = "SRA_1    ";
      default : _zz_execute_to_memory_SHIFT_CTRL_1_string = "?????????";
    endcase
  end
  always @(*) begin
    case(decode_SHIFT_CTRL)
      ShiftCtrlEnum_DISABLE_1 : decode_SHIFT_CTRL_string = "DISABLE_1";
      ShiftCtrlEnum_SLL_1 : decode_SHIFT_CTRL_string = "SLL_1    ";
      ShiftCtrlEnum_SRL_1 : decode_SHIFT_CTRL_string = "SRL_1    ";
      ShiftCtrlEnum_SRA_1 : decode_SHIFT_CTRL_string = "SRA_1    ";
      default : decode_SHIFT_CTRL_string = "?????????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_SHIFT_CTRL)
      ShiftCtrlEnum_DISABLE_1 : _zz_decode_SHIFT_CTRL_string = "DISABLE_1";
      ShiftCtrlEnum_SLL_1 : _zz_decode_SHIFT_CTRL_string = "SLL_1    ";
      ShiftCtrlEnum_SRL_1 : _zz_decode_SHIFT_CTRL_string = "SRL_1    ";
      ShiftCtrlEnum_SRA_1 : _zz_decode_SHIFT_CTRL_string = "SRA_1    ";
      default : _zz_decode_SHIFT_CTRL_string = "?????????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_to_execute_SHIFT_CTRL)
      ShiftCtrlEnum_DISABLE_1 : _zz_decode_to_execute_SHIFT_CTRL_string = "DISABLE_1";
      ShiftCtrlEnum_SLL_1 : _zz_decode_to_execute_SHIFT_CTRL_string = "SLL_1    ";
      ShiftCtrlEnum_SRL_1 : _zz_decode_to_execute_SHIFT_CTRL_string = "SRL_1    ";
      ShiftCtrlEnum_SRA_1 : _zz_decode_to_execute_SHIFT_CTRL_string = "SRA_1    ";
      default : _zz_decode_to_execute_SHIFT_CTRL_string = "?????????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_to_execute_SHIFT_CTRL_1)
      ShiftCtrlEnum_DISABLE_1 : _zz_decode_to_execute_SHIFT_CTRL_1_string = "DISABLE_1";
      ShiftCtrlEnum_SLL_1 : _zz_decode_to_execute_SHIFT_CTRL_1_string = "SLL_1    ";
      ShiftCtrlEnum_SRL_1 : _zz_decode_to_execute_SHIFT_CTRL_1_string = "SRL_1    ";
      ShiftCtrlEnum_SRA_1 : _zz_decode_to_execute_SHIFT_CTRL_1_string = "SRA_1    ";
      default : _zz_decode_to_execute_SHIFT_CTRL_1_string = "?????????";
    endcase
  end
  always @(*) begin
    case(decode_ALU_BITWISE_CTRL)
      AluBitwiseCtrlEnum_XOR_1 : decode_ALU_BITWISE_CTRL_string = "XOR_1";
      AluBitwiseCtrlEnum_OR_1 : decode_ALU_BITWISE_CTRL_string = "OR_1 ";
      AluBitwiseCtrlEnum_AND_1 : decode_ALU_BITWISE_CTRL_string = "AND_1";
      default : decode_ALU_BITWISE_CTRL_string = "?????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_ALU_BITWISE_CTRL)
      AluBitwiseCtrlEnum_XOR_1 : _zz_decode_ALU_BITWISE_CTRL_string = "XOR_1";
      AluBitwiseCtrlEnum_OR_1 : _zz_decode_ALU_BITWISE_CTRL_string = "OR_1 ";
      AluBitwiseCtrlEnum_AND_1 : _zz_decode_ALU_BITWISE_CTRL_string = "AND_1";
      default : _zz_decode_ALU_BITWISE_CTRL_string = "?????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_to_execute_ALU_BITWISE_CTRL)
      AluBitwiseCtrlEnum_XOR_1 : _zz_decode_to_execute_ALU_BITWISE_CTRL_string = "XOR_1";
      AluBitwiseCtrlEnum_OR_1 : _zz_decode_to_execute_ALU_BITWISE_CTRL_string = "OR_1 ";
      AluBitwiseCtrlEnum_AND_1 : _zz_decode_to_execute_ALU_BITWISE_CTRL_string = "AND_1";
      default : _zz_decode_to_execute_ALU_BITWISE_CTRL_string = "?????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_to_execute_ALU_BITWISE_CTRL_1)
      AluBitwiseCtrlEnum_XOR_1 : _zz_decode_to_execute_ALU_BITWISE_CTRL_1_string = "XOR_1";
      AluBitwiseCtrlEnum_OR_1 : _zz_decode_to_execute_ALU_BITWISE_CTRL_1_string = "OR_1 ";
      AluBitwiseCtrlEnum_AND_1 : _zz_decode_to_execute_ALU_BITWISE_CTRL_1_string = "AND_1";
      default : _zz_decode_to_execute_ALU_BITWISE_CTRL_1_string = "?????";
    endcase
  end
  always @(*) begin
    case(decode_SRC2_CTRL)
      Src2CtrlEnum_RS : decode_SRC2_CTRL_string = "RS ";
      Src2CtrlEnum_IMI : decode_SRC2_CTRL_string = "IMI";
      Src2CtrlEnum_IMS : decode_SRC2_CTRL_string = "IMS";
      Src2CtrlEnum_PC : decode_SRC2_CTRL_string = "PC ";
      default : decode_SRC2_CTRL_string = "???";
    endcase
  end
  always @(*) begin
    case(_zz_decode_SRC2_CTRL)
      Src2CtrlEnum_RS : _zz_decode_SRC2_CTRL_string = "RS ";
      Src2CtrlEnum_IMI : _zz_decode_SRC2_CTRL_string = "IMI";
      Src2CtrlEnum_IMS : _zz_decode_SRC2_CTRL_string = "IMS";
      Src2CtrlEnum_PC : _zz_decode_SRC2_CTRL_string = "PC ";
      default : _zz_decode_SRC2_CTRL_string = "???";
    endcase
  end
  always @(*) begin
    case(_zz_decode_to_execute_SRC2_CTRL)
      Src2CtrlEnum_RS : _zz_decode_to_execute_SRC2_CTRL_string = "RS ";
      Src2CtrlEnum_IMI : _zz_decode_to_execute_SRC2_CTRL_string = "IMI";
      Src2CtrlEnum_IMS : _zz_decode_to_execute_SRC2_CTRL_string = "IMS";
      Src2CtrlEnum_PC : _zz_decode_to_execute_SRC2_CTRL_string = "PC ";
      default : _zz_decode_to_execute_SRC2_CTRL_string = "???";
    endcase
  end
  always @(*) begin
    case(_zz_decode_to_execute_SRC2_CTRL_1)
      Src2CtrlEnum_RS : _zz_decode_to_execute_SRC2_CTRL_1_string = "RS ";
      Src2CtrlEnum_IMI : _zz_decode_to_execute_SRC2_CTRL_1_string = "IMI";
      Src2CtrlEnum_IMS : _zz_decode_to_execute_SRC2_CTRL_1_string = "IMS";
      Src2CtrlEnum_PC : _zz_decode_to_execute_SRC2_CTRL_1_string = "PC ";
      default : _zz_decode_to_execute_SRC2_CTRL_1_string = "???";
    endcase
  end
  always @(*) begin
    case(decode_ALU_CTRL)
      AluCtrlEnum_ADD_SUB : decode_ALU_CTRL_string = "ADD_SUB ";
      AluCtrlEnum_SLT_SLTU : decode_ALU_CTRL_string = "SLT_SLTU";
      AluCtrlEnum_BITWISE : decode_ALU_CTRL_string = "BITWISE ";
      default : decode_ALU_CTRL_string = "????????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_ALU_CTRL)
      AluCtrlEnum_ADD_SUB : _zz_decode_ALU_CTRL_string = "ADD_SUB ";
      AluCtrlEnum_SLT_SLTU : _zz_decode_ALU_CTRL_string = "SLT_SLTU";
      AluCtrlEnum_BITWISE : _zz_decode_ALU_CTRL_string = "BITWISE ";
      default : _zz_decode_ALU_CTRL_string = "????????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_to_execute_ALU_CTRL)
      AluCtrlEnum_ADD_SUB : _zz_decode_to_execute_ALU_CTRL_string = "ADD_SUB ";
      AluCtrlEnum_SLT_SLTU : _zz_decode_to_execute_ALU_CTRL_string = "SLT_SLTU";
      AluCtrlEnum_BITWISE : _zz_decode_to_execute_ALU_CTRL_string = "BITWISE ";
      default : _zz_decode_to_execute_ALU_CTRL_string = "????????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_to_execute_ALU_CTRL_1)
      AluCtrlEnum_ADD_SUB : _zz_decode_to_execute_ALU_CTRL_1_string = "ADD_SUB ";
      AluCtrlEnum_SLT_SLTU : _zz_decode_to_execute_ALU_CTRL_1_string = "SLT_SLTU";
      AluCtrlEnum_BITWISE : _zz_decode_to_execute_ALU_CTRL_1_string = "BITWISE ";
      default : _zz_decode_to_execute_ALU_CTRL_1_string = "????????";
    endcase
  end
  always @(*) begin
    case(decode_SRC1_CTRL)
      Src1CtrlEnum_RS : decode_SRC1_CTRL_string = "RS          ";
      Src1CtrlEnum_IMU : decode_SRC1_CTRL_string = "IMU         ";
      Src1CtrlEnum_PC_INCREMENT : decode_SRC1_CTRL_string = "PC_INCREMENT";
      Src1CtrlEnum_URS1 : decode_SRC1_CTRL_string = "URS1        ";
      default : decode_SRC1_CTRL_string = "????????????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_SRC1_CTRL)
      Src1CtrlEnum_RS : _zz_decode_SRC1_CTRL_string = "RS          ";
      Src1CtrlEnum_IMU : _zz_decode_SRC1_CTRL_string = "IMU         ";
      Src1CtrlEnum_PC_INCREMENT : _zz_decode_SRC1_CTRL_string = "PC_INCREMENT";
      Src1CtrlEnum_URS1 : _zz_decode_SRC1_CTRL_string = "URS1        ";
      default : _zz_decode_SRC1_CTRL_string = "????????????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_to_execute_SRC1_CTRL)
      Src1CtrlEnum_RS : _zz_decode_to_execute_SRC1_CTRL_string = "RS          ";
      Src1CtrlEnum_IMU : _zz_decode_to_execute_SRC1_CTRL_string = "IMU         ";
      Src1CtrlEnum_PC_INCREMENT : _zz_decode_to_execute_SRC1_CTRL_string = "PC_INCREMENT";
      Src1CtrlEnum_URS1 : _zz_decode_to_execute_SRC1_CTRL_string = "URS1        ";
      default : _zz_decode_to_execute_SRC1_CTRL_string = "????????????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_to_execute_SRC1_CTRL_1)
      Src1CtrlEnum_RS : _zz_decode_to_execute_SRC1_CTRL_1_string = "RS          ";
      Src1CtrlEnum_IMU : _zz_decode_to_execute_SRC1_CTRL_1_string = "IMU         ";
      Src1CtrlEnum_PC_INCREMENT : _zz_decode_to_execute_SRC1_CTRL_1_string = "PC_INCREMENT";
      Src1CtrlEnum_URS1 : _zz_decode_to_execute_SRC1_CTRL_1_string = "URS1        ";
      default : _zz_decode_to_execute_SRC1_CTRL_1_string = "????????????";
    endcase
  end
  always @(*) begin
    case(execute_BRANCH_CTRL)
      BranchCtrlEnum_INC : execute_BRANCH_CTRL_string = "INC ";
      BranchCtrlEnum_B : execute_BRANCH_CTRL_string = "B   ";
      BranchCtrlEnum_JAL : execute_BRANCH_CTRL_string = "JAL ";
      BranchCtrlEnum_JALR : execute_BRANCH_CTRL_string = "JALR";
      default : execute_BRANCH_CTRL_string = "????";
    endcase
  end
  always @(*) begin
    case(_zz_execute_BRANCH_CTRL)
      BranchCtrlEnum_INC : _zz_execute_BRANCH_CTRL_string = "INC ";
      BranchCtrlEnum_B : _zz_execute_BRANCH_CTRL_string = "B   ";
      BranchCtrlEnum_JAL : _zz_execute_BRANCH_CTRL_string = "JAL ";
      BranchCtrlEnum_JALR : _zz_execute_BRANCH_CTRL_string = "JALR";
      default : _zz_execute_BRANCH_CTRL_string = "????";
    endcase
  end
  always @(*) begin
    case(memory_ENV_CTRL)
      EnvCtrlEnum_NONE : memory_ENV_CTRL_string = "NONE ";
      EnvCtrlEnum_XRET : memory_ENV_CTRL_string = "XRET ";
      EnvCtrlEnum_WFI : memory_ENV_CTRL_string = "WFI  ";
      EnvCtrlEnum_ECALL : memory_ENV_CTRL_string = "ECALL";
      default : memory_ENV_CTRL_string = "?????";
    endcase
  end
  always @(*) begin
    case(_zz_memory_ENV_CTRL)
      EnvCtrlEnum_NONE : _zz_memory_ENV_CTRL_string = "NONE ";
      EnvCtrlEnum_XRET : _zz_memory_ENV_CTRL_string = "XRET ";
      EnvCtrlEnum_WFI : _zz_memory_ENV_CTRL_string = "WFI  ";
      EnvCtrlEnum_ECALL : _zz_memory_ENV_CTRL_string = "ECALL";
      default : _zz_memory_ENV_CTRL_string = "?????";
    endcase
  end
  always @(*) begin
    case(execute_ENV_CTRL)
      EnvCtrlEnum_NONE : execute_ENV_CTRL_string = "NONE ";
      EnvCtrlEnum_XRET : execute_ENV_CTRL_string = "XRET ";
      EnvCtrlEnum_WFI : execute_ENV_CTRL_string = "WFI  ";
      EnvCtrlEnum_ECALL : execute_ENV_CTRL_string = "ECALL";
      default : execute_ENV_CTRL_string = "?????";
    endcase
  end
  always @(*) begin
    case(_zz_execute_ENV_CTRL)
      EnvCtrlEnum_NONE : _zz_execute_ENV_CTRL_string = "NONE ";
      EnvCtrlEnum_XRET : _zz_execute_ENV_CTRL_string = "XRET ";
      EnvCtrlEnum_WFI : _zz_execute_ENV_CTRL_string = "WFI  ";
      EnvCtrlEnum_ECALL : _zz_execute_ENV_CTRL_string = "ECALL";
      default : _zz_execute_ENV_CTRL_string = "?????";
    endcase
  end
  always @(*) begin
    case(writeBack_ENV_CTRL)
      EnvCtrlEnum_NONE : writeBack_ENV_CTRL_string = "NONE ";
      EnvCtrlEnum_XRET : writeBack_ENV_CTRL_string = "XRET ";
      EnvCtrlEnum_WFI : writeBack_ENV_CTRL_string = "WFI  ";
      EnvCtrlEnum_ECALL : writeBack_ENV_CTRL_string = "ECALL";
      default : writeBack_ENV_CTRL_string = "?????";
    endcase
  end
  always @(*) begin
    case(_zz_writeBack_ENV_CTRL)
      EnvCtrlEnum_NONE : _zz_writeBack_ENV_CTRL_string = "NONE ";
      EnvCtrlEnum_XRET : _zz_writeBack_ENV_CTRL_string = "XRET ";
      EnvCtrlEnum_WFI : _zz_writeBack_ENV_CTRL_string = "WFI  ";
      EnvCtrlEnum_ECALL : _zz_writeBack_ENV_CTRL_string = "ECALL";
      default : _zz_writeBack_ENV_CTRL_string = "?????";
    endcase
  end
  always @(*) begin
    case(memory_SHIFT_CTRL)
      ShiftCtrlEnum_DISABLE_1 : memory_SHIFT_CTRL_string = "DISABLE_1";
      ShiftCtrlEnum_SLL_1 : memory_SHIFT_CTRL_string = "SLL_1    ";
      ShiftCtrlEnum_SRL_1 : memory_SHIFT_CTRL_string = "SRL_1    ";
      ShiftCtrlEnum_SRA_1 : memory_SHIFT_CTRL_string = "SRA_1    ";
      default : memory_SHIFT_CTRL_string = "?????????";
    endcase
  end
  always @(*) begin
    case(_zz_memory_SHIFT_CTRL)
      ShiftCtrlEnum_DISABLE_1 : _zz_memory_SHIFT_CTRL_string = "DISABLE_1";
      ShiftCtrlEnum_SLL_1 : _zz_memory_SHIFT_CTRL_string = "SLL_1    ";
      ShiftCtrlEnum_SRL_1 : _zz_memory_SHIFT_CTRL_string = "SRL_1    ";
      ShiftCtrlEnum_SRA_1 : _zz_memory_SHIFT_CTRL_string = "SRA_1    ";
      default : _zz_memory_SHIFT_CTRL_string = "?????????";
    endcase
  end
  always @(*) begin
    case(execute_SHIFT_CTRL)
      ShiftCtrlEnum_DISABLE_1 : execute_SHIFT_CTRL_string = "DISABLE_1";
      ShiftCtrlEnum_SLL_1 : execute_SHIFT_CTRL_string = "SLL_1    ";
      ShiftCtrlEnum_SRL_1 : execute_SHIFT_CTRL_string = "SRL_1    ";
      ShiftCtrlEnum_SRA_1 : execute_SHIFT_CTRL_string = "SRA_1    ";
      default : execute_SHIFT_CTRL_string = "?????????";
    endcase
  end
  always @(*) begin
    case(_zz_execute_SHIFT_CTRL)
      ShiftCtrlEnum_DISABLE_1 : _zz_execute_SHIFT_CTRL_string = "DISABLE_1";
      ShiftCtrlEnum_SLL_1 : _zz_execute_SHIFT_CTRL_string = "SLL_1    ";
      ShiftCtrlEnum_SRL_1 : _zz_execute_SHIFT_CTRL_string = "SRL_1    ";
      ShiftCtrlEnum_SRA_1 : _zz_execute_SHIFT_CTRL_string = "SRA_1    ";
      default : _zz_execute_SHIFT_CTRL_string = "?????????";
    endcase
  end
  always @(*) begin
    case(execute_SRC2_CTRL)
      Src2CtrlEnum_RS : execute_SRC2_CTRL_string = "RS ";
      Src2CtrlEnum_IMI : execute_SRC2_CTRL_string = "IMI";
      Src2CtrlEnum_IMS : execute_SRC2_CTRL_string = "IMS";
      Src2CtrlEnum_PC : execute_SRC2_CTRL_string = "PC ";
      default : execute_SRC2_CTRL_string = "???";
    endcase
  end
  always @(*) begin
    case(_zz_execute_SRC2_CTRL)
      Src2CtrlEnum_RS : _zz_execute_SRC2_CTRL_string = "RS ";
      Src2CtrlEnum_IMI : _zz_execute_SRC2_CTRL_string = "IMI";
      Src2CtrlEnum_IMS : _zz_execute_SRC2_CTRL_string = "IMS";
      Src2CtrlEnum_PC : _zz_execute_SRC2_CTRL_string = "PC ";
      default : _zz_execute_SRC2_CTRL_string = "???";
    endcase
  end
  always @(*) begin
    case(execute_SRC1_CTRL)
      Src1CtrlEnum_RS : execute_SRC1_CTRL_string = "RS          ";
      Src1CtrlEnum_IMU : execute_SRC1_CTRL_string = "IMU         ";
      Src1CtrlEnum_PC_INCREMENT : execute_SRC1_CTRL_string = "PC_INCREMENT";
      Src1CtrlEnum_URS1 : execute_SRC1_CTRL_string = "URS1        ";
      default : execute_SRC1_CTRL_string = "????????????";
    endcase
  end
  always @(*) begin
    case(_zz_execute_SRC1_CTRL)
      Src1CtrlEnum_RS : _zz_execute_SRC1_CTRL_string = "RS          ";
      Src1CtrlEnum_IMU : _zz_execute_SRC1_CTRL_string = "IMU         ";
      Src1CtrlEnum_PC_INCREMENT : _zz_execute_SRC1_CTRL_string = "PC_INCREMENT";
      Src1CtrlEnum_URS1 : _zz_execute_SRC1_CTRL_string = "URS1        ";
      default : _zz_execute_SRC1_CTRL_string = "????????????";
    endcase
  end
  always @(*) begin
    case(execute_ALU_CTRL)
      AluCtrlEnum_ADD_SUB : execute_ALU_CTRL_string = "ADD_SUB ";
      AluCtrlEnum_SLT_SLTU : execute_ALU_CTRL_string = "SLT_SLTU";
      AluCtrlEnum_BITWISE : execute_ALU_CTRL_string = "BITWISE ";
      default : execute_ALU_CTRL_string = "????????";
    endcase
  end
  always @(*) begin
    case(_zz_execute_ALU_CTRL)
      AluCtrlEnum_ADD_SUB : _zz_execute_ALU_CTRL_string = "ADD_SUB ";
      AluCtrlEnum_SLT_SLTU : _zz_execute_ALU_CTRL_string = "SLT_SLTU";
      AluCtrlEnum_BITWISE : _zz_execute_ALU_CTRL_string = "BITWISE ";
      default : _zz_execute_ALU_CTRL_string = "????????";
    endcase
  end
  always @(*) begin
    case(execute_ALU_BITWISE_CTRL)
      AluBitwiseCtrlEnum_XOR_1 : execute_ALU_BITWISE_CTRL_string = "XOR_1";
      AluBitwiseCtrlEnum_OR_1 : execute_ALU_BITWISE_CTRL_string = "OR_1 ";
      AluBitwiseCtrlEnum_AND_1 : execute_ALU_BITWISE_CTRL_string = "AND_1";
      default : execute_ALU_BITWISE_CTRL_string = "?????";
    endcase
  end
  always @(*) begin
    case(_zz_execute_ALU_BITWISE_CTRL)
      AluBitwiseCtrlEnum_XOR_1 : _zz_execute_ALU_BITWISE_CTRL_string = "XOR_1";
      AluBitwiseCtrlEnum_OR_1 : _zz_execute_ALU_BITWISE_CTRL_string = "OR_1 ";
      AluBitwiseCtrlEnum_AND_1 : _zz_execute_ALU_BITWISE_CTRL_string = "AND_1";
      default : _zz_execute_ALU_BITWISE_CTRL_string = "?????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_BRANCH_CTRL)
      BranchCtrlEnum_INC : _zz_decode_BRANCH_CTRL_string = "INC ";
      BranchCtrlEnum_B : _zz_decode_BRANCH_CTRL_string = "B   ";
      BranchCtrlEnum_JAL : _zz_decode_BRANCH_CTRL_string = "JAL ";
      BranchCtrlEnum_JALR : _zz_decode_BRANCH_CTRL_string = "JALR";
      default : _zz_decode_BRANCH_CTRL_string = "????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_ENV_CTRL_1)
      EnvCtrlEnum_NONE : _zz_decode_ENV_CTRL_1_string = "NONE ";
      EnvCtrlEnum_XRET : _zz_decode_ENV_CTRL_1_string = "XRET ";
      EnvCtrlEnum_WFI : _zz_decode_ENV_CTRL_1_string = "WFI  ";
      EnvCtrlEnum_ECALL : _zz_decode_ENV_CTRL_1_string = "ECALL";
      default : _zz_decode_ENV_CTRL_1_string = "?????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_SHIFT_CTRL_1)
      ShiftCtrlEnum_DISABLE_1 : _zz_decode_SHIFT_CTRL_1_string = "DISABLE_1";
      ShiftCtrlEnum_SLL_1 : _zz_decode_SHIFT_CTRL_1_string = "SLL_1    ";
      ShiftCtrlEnum_SRL_1 : _zz_decode_SHIFT_CTRL_1_string = "SRL_1    ";
      ShiftCtrlEnum_SRA_1 : _zz_decode_SHIFT_CTRL_1_string = "SRA_1    ";
      default : _zz_decode_SHIFT_CTRL_1_string = "?????????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_ALU_BITWISE_CTRL_1)
      AluBitwiseCtrlEnum_XOR_1 : _zz_decode_ALU_BITWISE_CTRL_1_string = "XOR_1";
      AluBitwiseCtrlEnum_OR_1 : _zz_decode_ALU_BITWISE_CTRL_1_string = "OR_1 ";
      AluBitwiseCtrlEnum_AND_1 : _zz_decode_ALU_BITWISE_CTRL_1_string = "AND_1";
      default : _zz_decode_ALU_BITWISE_CTRL_1_string = "?????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_SRC2_CTRL_1)
      Src2CtrlEnum_RS : _zz_decode_SRC2_CTRL_1_string = "RS ";
      Src2CtrlEnum_IMI : _zz_decode_SRC2_CTRL_1_string = "IMI";
      Src2CtrlEnum_IMS : _zz_decode_SRC2_CTRL_1_string = "IMS";
      Src2CtrlEnum_PC : _zz_decode_SRC2_CTRL_1_string = "PC ";
      default : _zz_decode_SRC2_CTRL_1_string = "???";
    endcase
  end
  always @(*) begin
    case(_zz_decode_ALU_CTRL_1)
      AluCtrlEnum_ADD_SUB : _zz_decode_ALU_CTRL_1_string = "ADD_SUB ";
      AluCtrlEnum_SLT_SLTU : _zz_decode_ALU_CTRL_1_string = "SLT_SLTU";
      AluCtrlEnum_BITWISE : _zz_decode_ALU_CTRL_1_string = "BITWISE ";
      default : _zz_decode_ALU_CTRL_1_string = "????????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_SRC1_CTRL_1)
      Src1CtrlEnum_RS : _zz_decode_SRC1_CTRL_1_string = "RS          ";
      Src1CtrlEnum_IMU : _zz_decode_SRC1_CTRL_1_string = "IMU         ";
      Src1CtrlEnum_PC_INCREMENT : _zz_decode_SRC1_CTRL_1_string = "PC_INCREMENT";
      Src1CtrlEnum_URS1 : _zz_decode_SRC1_CTRL_1_string = "URS1        ";
      default : _zz_decode_SRC1_CTRL_1_string = "????????????";
    endcase
  end
  always @(*) begin
    case(decode_BRANCH_CTRL)
      BranchCtrlEnum_INC : decode_BRANCH_CTRL_string = "INC ";
      BranchCtrlEnum_B : decode_BRANCH_CTRL_string = "B   ";
      BranchCtrlEnum_JAL : decode_BRANCH_CTRL_string = "JAL ";
      BranchCtrlEnum_JALR : decode_BRANCH_CTRL_string = "JALR";
      default : decode_BRANCH_CTRL_string = "????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_BRANCH_CTRL_1)
      BranchCtrlEnum_INC : _zz_decode_BRANCH_CTRL_1_string = "INC ";
      BranchCtrlEnum_B : _zz_decode_BRANCH_CTRL_1_string = "B   ";
      BranchCtrlEnum_JAL : _zz_decode_BRANCH_CTRL_1_string = "JAL ";
      BranchCtrlEnum_JALR : _zz_decode_BRANCH_CTRL_1_string = "JALR";
      default : _zz_decode_BRANCH_CTRL_1_string = "????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_SRC1_CTRL_2)
      Src1CtrlEnum_RS : _zz_decode_SRC1_CTRL_2_string = "RS          ";
      Src1CtrlEnum_IMU : _zz_decode_SRC1_CTRL_2_string = "IMU         ";
      Src1CtrlEnum_PC_INCREMENT : _zz_decode_SRC1_CTRL_2_string = "PC_INCREMENT";
      Src1CtrlEnum_URS1 : _zz_decode_SRC1_CTRL_2_string = "URS1        ";
      default : _zz_decode_SRC1_CTRL_2_string = "????????????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_ALU_CTRL_2)
      AluCtrlEnum_ADD_SUB : _zz_decode_ALU_CTRL_2_string = "ADD_SUB ";
      AluCtrlEnum_SLT_SLTU : _zz_decode_ALU_CTRL_2_string = "SLT_SLTU";
      AluCtrlEnum_BITWISE : _zz_decode_ALU_CTRL_2_string = "BITWISE ";
      default : _zz_decode_ALU_CTRL_2_string = "????????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_SRC2_CTRL_2)
      Src2CtrlEnum_RS : _zz_decode_SRC2_CTRL_2_string = "RS ";
      Src2CtrlEnum_IMI : _zz_decode_SRC2_CTRL_2_string = "IMI";
      Src2CtrlEnum_IMS : _zz_decode_SRC2_CTRL_2_string = "IMS";
      Src2CtrlEnum_PC : _zz_decode_SRC2_CTRL_2_string = "PC ";
      default : _zz_decode_SRC2_CTRL_2_string = "???";
    endcase
  end
  always @(*) begin
    case(_zz_decode_ALU_BITWISE_CTRL_2)
      AluBitwiseCtrlEnum_XOR_1 : _zz_decode_ALU_BITWISE_CTRL_2_string = "XOR_1";
      AluBitwiseCtrlEnum_OR_1 : _zz_decode_ALU_BITWISE_CTRL_2_string = "OR_1 ";
      AluBitwiseCtrlEnum_AND_1 : _zz_decode_ALU_BITWISE_CTRL_2_string = "AND_1";
      default : _zz_decode_ALU_BITWISE_CTRL_2_string = "?????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_SHIFT_CTRL_2)
      ShiftCtrlEnum_DISABLE_1 : _zz_decode_SHIFT_CTRL_2_string = "DISABLE_1";
      ShiftCtrlEnum_SLL_1 : _zz_decode_SHIFT_CTRL_2_string = "SLL_1    ";
      ShiftCtrlEnum_SRL_1 : _zz_decode_SHIFT_CTRL_2_string = "SRL_1    ";
      ShiftCtrlEnum_SRA_1 : _zz_decode_SHIFT_CTRL_2_string = "SRA_1    ";
      default : _zz_decode_SHIFT_CTRL_2_string = "?????????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_ENV_CTRL_2)
      EnvCtrlEnum_NONE : _zz_decode_ENV_CTRL_2_string = "NONE ";
      EnvCtrlEnum_XRET : _zz_decode_ENV_CTRL_2_string = "XRET ";
      EnvCtrlEnum_WFI : _zz_decode_ENV_CTRL_2_string = "WFI  ";
      EnvCtrlEnum_ECALL : _zz_decode_ENV_CTRL_2_string = "ECALL";
      default : _zz_decode_ENV_CTRL_2_string = "?????";
    endcase
  end
  always @(*) begin
    case(_zz_decode_BRANCH_CTRL_2)
      BranchCtrlEnum_INC : _zz_decode_BRANCH_CTRL_2_string = "INC ";
      BranchCtrlEnum_B : _zz_decode_BRANCH_CTRL_2_string = "B   ";
      BranchCtrlEnum_JAL : _zz_decode_BRANCH_CTRL_2_string = "JAL ";
      BranchCtrlEnum_JALR : _zz_decode_BRANCH_CTRL_2_string = "JALR";
      default : _zz_decode_BRANCH_CTRL_2_string = "????";
    endcase
  end
  always @(*) begin
    case(decode_to_execute_SRC1_CTRL)
      Src1CtrlEnum_RS : decode_to_execute_SRC1_CTRL_string = "RS          ";
      Src1CtrlEnum_IMU : decode_to_execute_SRC1_CTRL_string = "IMU         ";
      Src1CtrlEnum_PC_INCREMENT : decode_to_execute_SRC1_CTRL_string = "PC_INCREMENT";
      Src1CtrlEnum_URS1 : decode_to_execute_SRC1_CTRL_string = "URS1        ";
      default : decode_to_execute_SRC1_CTRL_string = "????????????";
    endcase
  end
  always @(*) begin
    case(decode_to_execute_ALU_CTRL)
      AluCtrlEnum_ADD_SUB : decode_to_execute_ALU_CTRL_string = "ADD_SUB ";
      AluCtrlEnum_SLT_SLTU : decode_to_execute_ALU_CTRL_string = "SLT_SLTU";
      AluCtrlEnum_BITWISE : decode_to_execute_ALU_CTRL_string = "BITWISE ";
      default : decode_to_execute_ALU_CTRL_string = "????????";
    endcase
  end
  always @(*) begin
    case(decode_to_execute_SRC2_CTRL)
      Src2CtrlEnum_RS : decode_to_execute_SRC2_CTRL_string = "RS ";
      Src2CtrlEnum_IMI : decode_to_execute_SRC2_CTRL_string = "IMI";
      Src2CtrlEnum_IMS : decode_to_execute_SRC2_CTRL_string = "IMS";
      Src2CtrlEnum_PC : decode_to_execute_SRC2_CTRL_string = "PC ";
      default : decode_to_execute_SRC2_CTRL_string = "???";
    endcase
  end
  always @(*) begin
    case(decode_to_execute_ALU_BITWISE_CTRL)
      AluBitwiseCtrlEnum_XOR_1 : decode_to_execute_ALU_BITWISE_CTRL_string = "XOR_1";
      AluBitwiseCtrlEnum_OR_1 : decode_to_execute_ALU_BITWISE_CTRL_string = "OR_1 ";
      AluBitwiseCtrlEnum_AND_1 : decode_to_execute_ALU_BITWISE_CTRL_string = "AND_1";
      default : decode_to_execute_ALU_BITWISE_CTRL_string = "?????";
    endcase
  end
  always @(*) begin
    case(decode_to_execute_SHIFT_CTRL)
      ShiftCtrlEnum_DISABLE_1 : decode_to_execute_SHIFT_CTRL_string = "DISABLE_1";
      ShiftCtrlEnum_SLL_1 : decode_to_execute_SHIFT_CTRL_string = "SLL_1    ";
      ShiftCtrlEnum_SRL_1 : decode_to_execute_SHIFT_CTRL_string = "SRL_1    ";
      ShiftCtrlEnum_SRA_1 : decode_to_execute_SHIFT_CTRL_string = "SRA_1    ";
      default : decode_to_execute_SHIFT_CTRL_string = "?????????";
    endcase
  end
  always @(*) begin
    case(execute_to_memory_SHIFT_CTRL)
      ShiftCtrlEnum_DISABLE_1 : execute_to_memory_SHIFT_CTRL_string = "DISABLE_1";
      ShiftCtrlEnum_SLL_1 : execute_to_memory_SHIFT_CTRL_string = "SLL_1    ";
      ShiftCtrlEnum_SRL_1 : execute_to_memory_SHIFT_CTRL_string = "SRL_1    ";
      ShiftCtrlEnum_SRA_1 : execute_to_memory_SHIFT_CTRL_string = "SRA_1    ";
      default : execute_to_memory_SHIFT_CTRL_string = "?????????";
    endcase
  end
  always @(*) begin
    case(decode_to_execute_ENV_CTRL)
      EnvCtrlEnum_NONE : decode_to_execute_ENV_CTRL_string = "NONE ";
      EnvCtrlEnum_XRET : decode_to_execute_ENV_CTRL_string = "XRET ";
      EnvCtrlEnum_WFI : decode_to_execute_ENV_CTRL_string = "WFI  ";
      EnvCtrlEnum_ECALL : decode_to_execute_ENV_CTRL_string = "ECALL";
      default : decode_to_execute_ENV_CTRL_string = "?????";
    endcase
  end
  always @(*) begin
    case(execute_to_memory_ENV_CTRL)
      EnvCtrlEnum_NONE : execute_to_memory_ENV_CTRL_string = "NONE ";
      EnvCtrlEnum_XRET : execute_to_memory_ENV_CTRL_string = "XRET ";
      EnvCtrlEnum_WFI : execute_to_memory_ENV_CTRL_string = "WFI  ";
      EnvCtrlEnum_ECALL : execute_to_memory_ENV_CTRL_string = "ECALL";
      default : execute_to_memory_ENV_CTRL_string = "?????";
    endcase
  end
  always @(*) begin
    case(memory_to_writeBack_ENV_CTRL)
      EnvCtrlEnum_NONE : memory_to_writeBack_ENV_CTRL_string = "NONE ";
      EnvCtrlEnum_XRET : memory_to_writeBack_ENV_CTRL_string = "XRET ";
      EnvCtrlEnum_WFI : memory_to_writeBack_ENV_CTRL_string = "WFI  ";
      EnvCtrlEnum_ECALL : memory_to_writeBack_ENV_CTRL_string = "ECALL";
      default : memory_to_writeBack_ENV_CTRL_string = "?????";
    endcase
  end
  always @(*) begin
    case(decode_to_execute_BRANCH_CTRL)
      BranchCtrlEnum_INC : decode_to_execute_BRANCH_CTRL_string = "INC ";
      BranchCtrlEnum_B : decode_to_execute_BRANCH_CTRL_string = "B   ";
      BranchCtrlEnum_JAL : decode_to_execute_BRANCH_CTRL_string = "JAL ";
      BranchCtrlEnum_JALR : decode_to_execute_BRANCH_CTRL_string = "JALR";
      default : decode_to_execute_BRANCH_CTRL_string = "????";
    endcase
  end
  always @(*) begin
    case(execute_PmpPlugin_fsm_stateReg)
      execute_PmpPlugin_fsm_enumDef_BOOT : execute_PmpPlugin_fsm_stateReg_string = "BOOT      ";
      execute_PmpPlugin_fsm_enumDef_stateIdle : execute_PmpPlugin_fsm_stateReg_string = "stateIdle ";
      execute_PmpPlugin_fsm_enumDef_stateWrite : execute_PmpPlugin_fsm_stateReg_string = "stateWrite";
      execute_PmpPlugin_fsm_enumDef_stateCfg : execute_PmpPlugin_fsm_stateReg_string = "stateCfg  ";
      execute_PmpPlugin_fsm_enumDef_stateAddr : execute_PmpPlugin_fsm_stateReg_string = "stateAddr ";
      default : execute_PmpPlugin_fsm_stateReg_string = "??????????";
    endcase
  end
  always @(*) begin
    case(execute_PmpPlugin_fsm_stateNext)
      execute_PmpPlugin_fsm_enumDef_BOOT : execute_PmpPlugin_fsm_stateNext_string = "BOOT      ";
      execute_PmpPlugin_fsm_enumDef_stateIdle : execute_PmpPlugin_fsm_stateNext_string = "stateIdle ";
      execute_PmpPlugin_fsm_enumDef_stateWrite : execute_PmpPlugin_fsm_stateNext_string = "stateWrite";
      execute_PmpPlugin_fsm_enumDef_stateCfg : execute_PmpPlugin_fsm_stateNext_string = "stateCfg  ";
      execute_PmpPlugin_fsm_enumDef_stateAddr : execute_PmpPlugin_fsm_stateNext_string = "stateAddr ";
      default : execute_PmpPlugin_fsm_stateNext_string = "??????????";
    endcase
  end
  `endif

  always @(*) begin
    _zz_1 = 1'b0; // @[when.scala 47:16]
    case(execute_PmpPlugin_fsm_stateReg)
      execute_PmpPlugin_fsm_enumDef_stateIdle : begin
      end
      execute_PmpPlugin_fsm_enumDef_stateWrite : begin
        if(execute_PmpPlugin_pmpaddrCsr_) begin
          if(when_PmpPlugin_l216) begin
            _zz_1 = 1'b1; // @[when.scala 52:10]
          end
        end
      end
      execute_PmpPlugin_fsm_enumDef_stateCfg : begin
      end
      execute_PmpPlugin_fsm_enumDef_stateAddr : begin
      end
      default : begin
      end
    endcase
  end

  assign execute_BRANCH_CALC = {execute_BranchPlugin_branchAdder[31 : 1],1'b0}; // @[Stage.scala 30:13]
  assign execute_BRANCH_DO = ((execute_PREDICTION_HAD_BRANCHED2 != execute_BRANCH_COND_RESULT) || execute_BranchPlugin_missAlignedTarget); // @[Stage.scala 30:13]
  assign execute_SHIFT_RIGHT = _zz_execute_SHIFT_RIGHT; // @[Stage.scala 30:13]
  assign execute_REGFILE_WRITE_DATA = _zz_execute_REGFILE_WRITE_DATA; // @[Stage.scala 30:13]
  assign memory_MEMORY_STORE_DATA_RF = execute_to_memory_MEMORY_STORE_DATA_RF; // @[Stage.scala 30:13]
  assign execute_MEMORY_STORE_DATA_RF = _zz_execute_MEMORY_STORE_DATA_RF; // @[Stage.scala 30:13]
  assign decode_PREDICTION_HAD_BRANCHED2 = IBusCachedPlugin_decodePrediction_cmd_hadBranch; // @[Stage.scala 30:13]
  assign decode_DO_EBREAK = (((! DebugPlugin_haltIt) && (decode_IS_EBREAK || 1'b0)) && DebugPlugin_allowEBreak); // @[Stage.scala 30:13]
  assign decode_CSR_READ_OPCODE = (decode_INSTRUCTION[13 : 7] != 7'h20); // @[Stage.scala 30:13]
  assign decode_CSR_WRITE_OPCODE = (! (((decode_INSTRUCTION[14 : 13] == 2'b01) && (decode_INSTRUCTION[19 : 15] == 5'h0)) || ((decode_INSTRUCTION[14 : 13] == 2'b11) && (decode_INSTRUCTION[19 : 15] == 5'h0)))); // @[Stage.scala 30:13]
  assign decode_SRC2_FORCE_ZERO = (decode_SRC_ADD_ZERO && (! decode_SRC_USE_SUB_LESS)); // @[Stage.scala 30:13]
  assign _zz_decode_to_execute_BRANCH_CTRL = _zz_decode_to_execute_BRANCH_CTRL_1; // @[Stage.scala 39:14]
  assign _zz_memory_to_writeBack_ENV_CTRL = _zz_memory_to_writeBack_ENV_CTRL_1; // @[Stage.scala 39:14]
  assign _zz_execute_to_memory_ENV_CTRL = _zz_execute_to_memory_ENV_CTRL_1; // @[Stage.scala 39:14]
  assign decode_ENV_CTRL = _zz_decode_ENV_CTRL; // @[Stage.scala 30:13]
  assign _zz_decode_to_execute_ENV_CTRL = _zz_decode_to_execute_ENV_CTRL_1; // @[Stage.scala 39:14]
  assign decode_IS_CSR = _zz_decode_IS_CSR[27]; // @[Stage.scala 30:13]
  assign decode_IS_DIV = _zz_decode_IS_CSR[26]; // @[Stage.scala 30:13]
  assign decode_IS_RS2_SIGNED = _zz_decode_IS_CSR[25]; // @[Stage.scala 30:13]
  assign decode_IS_RS1_SIGNED = _zz_decode_IS_CSR[24]; // @[Stage.scala 30:13]
  assign decode_IS_MUL = _zz_decode_IS_CSR[23]; // @[Stage.scala 30:13]
  assign _zz_execute_to_memory_SHIFT_CTRL = _zz_execute_to_memory_SHIFT_CTRL_1; // @[Stage.scala 39:14]
  assign decode_SHIFT_CTRL = _zz_decode_SHIFT_CTRL; // @[Stage.scala 30:13]
  assign _zz_decode_to_execute_SHIFT_CTRL = _zz_decode_to_execute_SHIFT_CTRL_1; // @[Stage.scala 39:14]
  assign decode_ALU_BITWISE_CTRL = _zz_decode_ALU_BITWISE_CTRL; // @[Stage.scala 30:13]
  assign _zz_decode_to_execute_ALU_BITWISE_CTRL = _zz_decode_to_execute_ALU_BITWISE_CTRL_1; // @[Stage.scala 39:14]
  assign decode_SRC_LESS_UNSIGNED = _zz_decode_IS_CSR[17]; // @[Stage.scala 30:13]
  assign decode_MEMORY_MANAGMENT = _zz_decode_IS_CSR[16]; // @[Stage.scala 30:13]
  assign memory_MEMORY_WR = execute_to_memory_MEMORY_WR; // @[Stage.scala 30:13]
  assign decode_MEMORY_WR = _zz_decode_IS_CSR[13]; // @[Stage.scala 30:13]
  assign execute_BYPASSABLE_MEMORY_STAGE = decode_to_execute_BYPASSABLE_MEMORY_STAGE; // @[Stage.scala 30:13]
  assign decode_BYPASSABLE_MEMORY_STAGE = _zz_decode_IS_CSR[12]; // @[Stage.scala 30:13]
  assign decode_BYPASSABLE_EXECUTE_STAGE = _zz_decode_IS_CSR[11]; // @[Stage.scala 30:13]
  assign decode_SRC2_CTRL = _zz_decode_SRC2_CTRL; // @[Stage.scala 30:13]
  assign _zz_decode_to_execute_SRC2_CTRL = _zz_decode_to_execute_SRC2_CTRL_1; // @[Stage.scala 39:14]
  assign decode_ALU_CTRL = _zz_decode_ALU_CTRL; // @[Stage.scala 30:13]
  assign _zz_decode_to_execute_ALU_CTRL = _zz_decode_to_execute_ALU_CTRL_1; // @[Stage.scala 39:14]
  assign decode_SRC1_CTRL = _zz_decode_SRC1_CTRL; // @[Stage.scala 30:13]
  assign _zz_decode_to_execute_SRC1_CTRL = _zz_decode_to_execute_SRC1_CTRL_1; // @[Stage.scala 39:14]
  assign decode_MEMORY_FORCE_CONSTISTENCY = 1'b0; // @[Stage.scala 30:13]
  assign writeBack_FORMAL_PC_NEXT = memory_to_writeBack_FORMAL_PC_NEXT; // @[Stage.scala 30:13]
  assign memory_FORMAL_PC_NEXT = execute_to_memory_FORMAL_PC_NEXT; // @[Stage.scala 30:13]
  assign execute_FORMAL_PC_NEXT = decode_to_execute_FORMAL_PC_NEXT; // @[Stage.scala 30:13]
  assign decode_FORMAL_PC_NEXT = (decode_PC + 32'h00000004); // @[Stage.scala 30:13]
  assign memory_PC = execute_to_memory_PC; // @[Stage.scala 30:13]
  assign memory_BRANCH_CALC = execute_to_memory_BRANCH_CALC; // @[Stage.scala 30:13]
  assign memory_BRANCH_DO = execute_to_memory_BRANCH_DO; // @[Stage.scala 30:13]
  assign execute_PREDICTION_HAD_BRANCHED2 = decode_to_execute_PREDICTION_HAD_BRANCHED2; // @[Stage.scala 30:13]
  assign execute_BRANCH_COND_RESULT = _zz_execute_BRANCH_COND_RESULT_1; // @[Stage.scala 30:13]
  assign execute_BRANCH_CTRL = _zz_execute_BRANCH_CTRL; // @[Stage.scala 30:13]
  assign execute_PC = decode_to_execute_PC; // @[Stage.scala 30:13]
  assign execute_DO_EBREAK = decode_to_execute_DO_EBREAK; // @[Stage.scala 30:13]
  assign decode_IS_EBREAK = _zz_decode_IS_CSR[30]; // @[Stage.scala 30:13]
  assign execute_CSR_READ_OPCODE = decode_to_execute_CSR_READ_OPCODE; // @[Stage.scala 30:13]
  assign execute_CSR_WRITE_OPCODE = decode_to_execute_CSR_WRITE_OPCODE; // @[Stage.scala 30:13]
  assign execute_IS_CSR = decode_to_execute_IS_CSR; // @[Stage.scala 30:13]
  assign memory_ENV_CTRL = _zz_memory_ENV_CTRL; // @[Stage.scala 30:13]
  assign execute_ENV_CTRL = _zz_execute_ENV_CTRL; // @[Stage.scala 30:13]
  assign writeBack_ENV_CTRL = _zz_writeBack_ENV_CTRL; // @[Stage.scala 30:13]
  assign execute_IS_RS1_SIGNED = decode_to_execute_IS_RS1_SIGNED; // @[Stage.scala 30:13]
  assign execute_IS_DIV = decode_to_execute_IS_DIV; // @[Stage.scala 30:13]
  assign execute_IS_MUL = decode_to_execute_IS_MUL; // @[Stage.scala 30:13]
  assign execute_IS_RS2_SIGNED = decode_to_execute_IS_RS2_SIGNED; // @[Stage.scala 30:13]
  assign memory_IS_DIV = execute_to_memory_IS_DIV; // @[Stage.scala 30:13]
  assign memory_IS_MUL = execute_to_memory_IS_MUL; // @[Stage.scala 30:13]
  assign decode_RS2_USE = _zz_decode_IS_CSR[15]; // @[Stage.scala 30:13]
  assign decode_RS1_USE = _zz_decode_IS_CSR[5]; // @[Stage.scala 30:13]
  always @(*) begin
    _zz_decode_RS2 = execute_REGFILE_WRITE_DATA; // @[Stage.scala 39:14]
    if(when_CsrPlugin_l1507) begin
      _zz_decode_RS2 = CsrPlugin_csrMapping_readDataSignal; // @[CsrPlugin.scala 1508:59]
    end
  end

  assign execute_REGFILE_WRITE_VALID = decode_to_execute_REGFILE_WRITE_VALID; // @[Stage.scala 30:13]
  assign execute_BYPASSABLE_EXECUTE_STAGE = decode_to_execute_BYPASSABLE_EXECUTE_STAGE; // @[Stage.scala 30:13]
  assign memory_REGFILE_WRITE_VALID = execute_to_memory_REGFILE_WRITE_VALID; // @[Stage.scala 30:13]
  assign memory_INSTRUCTION = execute_to_memory_INSTRUCTION; // @[Stage.scala 30:13]
  assign memory_BYPASSABLE_MEMORY_STAGE = execute_to_memory_BYPASSABLE_MEMORY_STAGE; // @[Stage.scala 30:13]
  assign writeBack_REGFILE_WRITE_VALID = memory_to_writeBack_REGFILE_WRITE_VALID; // @[Stage.scala 30:13]
  always @(*) begin
    decode_RS2 = decode_RegFilePlugin_rs2Data; // @[Stage.scala 30:13]
    if(HazardSimplePlugin_writeBackBuffer_valid) begin
      if(HazardSimplePlugin_addr1Match) begin
        decode_RS2 = HazardSimplePlugin_writeBackBuffer_payload_data; // @[HazardSimplePlugin.scala 87:34]
      end
    end
    if(when_HazardSimplePlugin_l45) begin
      if(when_HazardSimplePlugin_l47) begin
        if(when_HazardSimplePlugin_l51) begin
          decode_RS2 = _zz_decode_RS2_2; // @[HazardSimplePlugin.scala 52:38]
        end
      end
    end
    if(when_HazardSimplePlugin_l45_1) begin
      if(memory_BYPASSABLE_MEMORY_STAGE) begin
        if(when_HazardSimplePlugin_l51_1) begin
          decode_RS2 = _zz_decode_RS2_1; // @[HazardSimplePlugin.scala 52:38]
        end
      end
    end
    if(when_HazardSimplePlugin_l45_2) begin
      if(execute_BYPASSABLE_EXECUTE_STAGE) begin
        if(when_HazardSimplePlugin_l51_2) begin
          decode_RS2 = _zz_decode_RS2; // @[HazardSimplePlugin.scala 52:38]
        end
      end
    end
  end

  always @(*) begin
    decode_RS1 = decode_RegFilePlugin_rs1Data; // @[Stage.scala 30:13]
    if(HazardSimplePlugin_writeBackBuffer_valid) begin
      if(HazardSimplePlugin_addr0Match) begin
        decode_RS1 = HazardSimplePlugin_writeBackBuffer_payload_data; // @[HazardSimplePlugin.scala 84:34]
      end
    end
    if(when_HazardSimplePlugin_l45) begin
      if(when_HazardSimplePlugin_l47) begin
        if(when_HazardSimplePlugin_l48) begin
          decode_RS1 = _zz_decode_RS2_2; // @[HazardSimplePlugin.scala 49:38]
        end
      end
    end
    if(when_HazardSimplePlugin_l45_1) begin
      if(memory_BYPASSABLE_MEMORY_STAGE) begin
        if(when_HazardSimplePlugin_l48_1) begin
          decode_RS1 = _zz_decode_RS2_1; // @[HazardSimplePlugin.scala 49:38]
        end
      end
    end
    if(when_HazardSimplePlugin_l45_2) begin
      if(execute_BYPASSABLE_EXECUTE_STAGE) begin
        if(when_HazardSimplePlugin_l48_2) begin
          decode_RS1 = _zz_decode_RS2; // @[HazardSimplePlugin.scala 49:38]
        end
      end
    end
  end

  assign memory_SHIFT_RIGHT = execute_to_memory_SHIFT_RIGHT; // @[Stage.scala 30:13]
  always @(*) begin
    _zz_decode_RS2_1 = memory_REGFILE_WRITE_DATA; // @[Stage.scala 39:14]
    if(memory_arbitration_isValid) begin
      case(memory_SHIFT_CTRL)
        ShiftCtrlEnum_SLL_1 : begin
          _zz_decode_RS2_1 = _zz_decode_RS2_3; // @[ShiftPlugins.scala 75:40]
        end
        ShiftCtrlEnum_SRL_1, ShiftCtrlEnum_SRA_1 : begin
          _zz_decode_RS2_1 = memory_SHIFT_RIGHT; // @[ShiftPlugins.scala 78:40]
        end
        default : begin
        end
      endcase
    end
    if(when_MulDivIterativePlugin_l96) begin
      _zz_decode_RS2_1 = ((memory_INSTRUCTION[13 : 12] == 2'b00) ? memory_MulDivIterativePlugin_accumulator[31 : 0] : memory_MulDivIterativePlugin_accumulator[63 : 32]); // @[MulDivIterativePlugin.scala 108:38]
    end
    if(when_MulDivIterativePlugin_l128) begin
      _zz_decode_RS2_1 = memory_MulDivIterativePlugin_div_result; // @[MulDivIterativePlugin.scala 157:38]
    end
  end

  assign memory_SHIFT_CTRL = _zz_memory_SHIFT_CTRL; // @[Stage.scala 30:13]
  assign execute_SHIFT_CTRL = _zz_execute_SHIFT_CTRL; // @[Stage.scala 30:13]
  assign execute_SRC_LESS_UNSIGNED = decode_to_execute_SRC_LESS_UNSIGNED; // @[Stage.scala 30:13]
  assign execute_SRC2_FORCE_ZERO = decode_to_execute_SRC2_FORCE_ZERO; // @[Stage.scala 30:13]
  assign execute_SRC_USE_SUB_LESS = decode_to_execute_SRC_USE_SUB_LESS; // @[Stage.scala 30:13]
  assign _zz_execute_to_memory_PC = execute_PC; // @[Stage.scala 39:14]
  assign execute_SRC2_CTRL = _zz_execute_SRC2_CTRL; // @[Stage.scala 30:13]
  assign execute_SRC1_CTRL = _zz_execute_SRC1_CTRL; // @[Stage.scala 30:13]
  assign decode_SRC_USE_SUB_LESS = _zz_decode_IS_CSR[3]; // @[Stage.scala 30:13]
  assign decode_SRC_ADD_ZERO = _zz_decode_IS_CSR[20]; // @[Stage.scala 30:13]
  assign execute_SRC_ADD_SUB = execute_SrcPlugin_addSub; // @[Stage.scala 30:13]
  assign execute_SRC_LESS = execute_SrcPlugin_less; // @[Stage.scala 30:13]
  assign execute_ALU_CTRL = _zz_execute_ALU_CTRL; // @[Stage.scala 30:13]
  assign execute_SRC2 = _zz_execute_SRC2_4; // @[Stage.scala 30:13]
  assign execute_SRC1 = _zz_execute_SRC1; // @[Stage.scala 30:13]
  assign execute_ALU_BITWISE_CTRL = _zz_execute_ALU_BITWISE_CTRL; // @[Stage.scala 30:13]
  assign _zz_lastStageRegFileWrite_payload_address = writeBack_INSTRUCTION; // @[Stage.scala 39:14]
  assign _zz_lastStageRegFileWrite_valid = writeBack_REGFILE_WRITE_VALID; // @[Stage.scala 39:14]
  always @(*) begin
    _zz_2 = 1'b0; // @[when.scala 47:16]
    if(lastStageRegFileWrite_valid) begin
      _zz_2 = 1'b1; // @[when.scala 52:10]
    end
  end

  assign decode_INSTRUCTION_ANTICIPATED = (decode_arbitration_isStuck ? decode_INSTRUCTION : IBusCachedPlugin_cache_io_cpu_fetch_data); // @[Stage.scala 30:13]
  always @(*) begin
    decode_REGFILE_WRITE_VALID = _zz_decode_IS_CSR[10]; // @[Stage.scala 30:13]
    if(when_RegFilePlugin_l63) begin
      decode_REGFILE_WRITE_VALID = 1'b0; // @[RegFilePlugin.scala 64:41]
    end
  end

  assign decode_LEGAL_INSTRUCTION = (|{((decode_INSTRUCTION & 32'h0000005f) == 32'h00000017),{((decode_INSTRUCTION & 32'h0000007f) == 32'h0000006f),{((decode_INSTRUCTION & 32'h0000107f) == 32'h00001073),{((decode_INSTRUCTION & _zz_decode_LEGAL_INSTRUCTION) == 32'h00002073),{(_zz_decode_LEGAL_INSTRUCTION_1 == _zz_decode_LEGAL_INSTRUCTION_2),{_zz_decode_LEGAL_INSTRUCTION_3,{_zz_decode_LEGAL_INSTRUCTION_4,_zz_decode_LEGAL_INSTRUCTION_5}}}}}}}); // @[Stage.scala 30:13]
  always @(*) begin
    _zz_decode_RS2_2 = writeBack_REGFILE_WRITE_DATA; // @[Stage.scala 39:14]
    if(when_DBusCachedPlugin_l492) begin
      _zz_decode_RS2_2 = writeBack_DBusCachedPlugin_rspFormated; // @[DBusCachedPlugin.scala 493:36]
    end
  end

  assign writeBack_MEMORY_WR = memory_to_writeBack_MEMORY_WR; // @[Stage.scala 30:13]
  assign writeBack_MEMORY_STORE_DATA_RF = memory_to_writeBack_MEMORY_STORE_DATA_RF; // @[Stage.scala 30:13]
  assign writeBack_REGFILE_WRITE_DATA = memory_to_writeBack_REGFILE_WRITE_DATA; // @[Stage.scala 30:13]
  assign writeBack_MEMORY_ENABLE = memory_to_writeBack_MEMORY_ENABLE; // @[Stage.scala 30:13]
  assign memory_REGFILE_WRITE_DATA = execute_to_memory_REGFILE_WRITE_DATA; // @[Stage.scala 30:13]
  assign memory_MEMORY_ENABLE = execute_to_memory_MEMORY_ENABLE; // @[Stage.scala 30:13]
  assign execute_MEMORY_FORCE_CONSTISTENCY = decode_to_execute_MEMORY_FORCE_CONSTISTENCY; // @[Stage.scala 30:13]
  assign execute_RS1 = decode_to_execute_RS1; // @[Stage.scala 30:13]
  assign execute_MEMORY_MANAGMENT = decode_to_execute_MEMORY_MANAGMENT; // @[Stage.scala 30:13]
  assign execute_RS2 = decode_to_execute_RS2; // @[Stage.scala 30:13]
  assign execute_MEMORY_WR = decode_to_execute_MEMORY_WR; // @[Stage.scala 30:13]
  assign execute_SRC_ADD = execute_SrcPlugin_addSub; // @[Stage.scala 30:13]
  assign execute_MEMORY_ENABLE = decode_to_execute_MEMORY_ENABLE; // @[Stage.scala 30:13]
  assign execute_INSTRUCTION = decode_to_execute_INSTRUCTION; // @[Stage.scala 30:13]
  assign decode_MEMORY_ENABLE = _zz_decode_IS_CSR[4]; // @[Stage.scala 30:13]
  assign decode_FLUSH_ALL = _zz_decode_IS_CSR[0]; // @[Stage.scala 30:13]
  always @(*) begin
    IBusCachedPlugin_rsp_issueDetected_4 = IBusCachedPlugin_rsp_issueDetected_3; // @[Data.scala 57:9]
    if(when_IBusCachedPlugin_l256) begin
      IBusCachedPlugin_rsp_issueDetected_4 = 1'b1; // @[Data.scala 63:9]
    end
  end

  always @(*) begin
    IBusCachedPlugin_rsp_issueDetected_3 = IBusCachedPlugin_rsp_issueDetected_2; // @[Data.scala 57:9]
    if(when_IBusCachedPlugin_l250) begin
      IBusCachedPlugin_rsp_issueDetected_3 = 1'b1; // @[Data.scala 63:9]
    end
  end

  always @(*) begin
    IBusCachedPlugin_rsp_issueDetected_2 = IBusCachedPlugin_rsp_issueDetected_1; // @[Data.scala 57:9]
    if(when_IBusCachedPlugin_l244) begin
      IBusCachedPlugin_rsp_issueDetected_2 = 1'b1; // @[Data.scala 63:9]
    end
  end

  always @(*) begin
    IBusCachedPlugin_rsp_issueDetected_1 = IBusCachedPlugin_rsp_issueDetected; // @[Data.scala 57:9]
    if(when_IBusCachedPlugin_l239) begin
      IBusCachedPlugin_rsp_issueDetected_1 = 1'b1; // @[Data.scala 63:9]
    end
  end

  assign decode_BRANCH_CTRL = _zz_decode_BRANCH_CTRL_1; // @[Stage.scala 30:13]
  assign decode_INSTRUCTION = IBusCachedPlugin_iBusRsp_output_payload_rsp_inst; // @[Stage.scala 30:13]
  always @(*) begin
    _zz_memory_to_writeBack_FORMAL_PC_NEXT = memory_FORMAL_PC_NEXT; // @[Stage.scala 39:14]
    if(BranchPlugin_jumpInterface_valid) begin
      _zz_memory_to_writeBack_FORMAL_PC_NEXT = BranchPlugin_jumpInterface_payload; // @[Fetcher.scala 437:47]
    end
  end

  always @(*) begin
    _zz_decode_to_execute_FORMAL_PC_NEXT = decode_FORMAL_PC_NEXT; // @[Stage.scala 39:14]
    if(IBusCachedPlugin_predictionJumpInterface_valid) begin
      _zz_decode_to_execute_FORMAL_PC_NEXT = IBusCachedPlugin_predictionJumpInterface_payload; // @[Fetcher.scala 437:47]
    end
  end

  assign decode_PC = IBusCachedPlugin_iBusRsp_output_payload_pc; // @[Stage.scala 30:13]
  assign writeBack_PC = memory_to_writeBack_PC; // @[Stage.scala 30:13]
  assign writeBack_INSTRUCTION = memory_to_writeBack_INSTRUCTION; // @[Stage.scala 30:13]
  always @(*) begin
    decode_arbitration_haltItself = 1'b0; // @[Stage.scala 49:23]
    if(when_DBusCachedPlugin_l308) begin
      decode_arbitration_haltItself = 1'b1; // @[DBusCachedPlugin.scala 309:32]
    end
    case(switch_Fetcher_l365)
      3'b010 : begin
        decode_arbitration_haltItself = 1'b1; // @[Fetcher.scala 376:45]
      end
      default : begin
      end
    endcase
  end

  always @(*) begin
    decode_arbitration_haltByOther = 1'b0; // @[Stage.scala 50:23]
    if(when_HazardSimplePlugin_l113) begin
      decode_arbitration_haltByOther = 1'b1; // @[HazardSimplePlugin.scala 114:43]
    end
    if(CsrPlugin_pipelineLiberator_active) begin
      decode_arbitration_haltByOther = 1'b1; // @[CsrPlugin.scala 1253:42]
    end
    if(when_CsrPlugin_l1447) begin
      decode_arbitration_haltByOther = 1'b1; // @[CsrPlugin.scala 1447:38]
    end
  end

  always @(*) begin
    decode_arbitration_removeIt = 1'b0; // @[Stage.scala 51:23]
    if(_zz_when) begin
      decode_arbitration_removeIt = 1'b1; // @[CsrPlugin.scala 1171:40]
    end
    if(decode_arbitration_isFlushed) begin
      decode_arbitration_removeIt = 1'b1; // @[Pipeline.scala 134:36]
    end
  end

  assign decode_arbitration_flushIt = 1'b0; // @[Stage.scala 52:22]
  always @(*) begin
    decode_arbitration_flushNext = 1'b0; // @[Stage.scala 53:24]
    if(IBusCachedPlugin_predictionJumpInterface_valid) begin
      decode_arbitration_flushNext = 1'b1; // @[Fetcher.scala 515:38]
    end
    if(_zz_when) begin
      decode_arbitration_flushNext = 1'b1; // @[CsrPlugin.scala 1170:41]
    end
  end

  always @(*) begin
    execute_arbitration_haltItself = 1'b0; // @[Stage.scala 49:23]
    if(when_DBusCachedPlugin_l350) begin
      execute_arbitration_haltItself = 1'b1; // @[DBusCachedPlugin.scala 350:30]
    end
    if(when_CsrPlugin_l1439) begin
      if(when_CsrPlugin_l1441) begin
        execute_arbitration_haltItself = 1'b1; // @[CsrPlugin.scala 1442:36]
      end
    end
    if(when_CsrPlugin_l1511) begin
      if(execute_CsrPlugin_blockedBySideEffects) begin
        execute_arbitration_haltItself = 1'b1; // @[CsrPlugin.scala 1512:34]
      end
    end
    if(execute_CsrPlugin_writeInstruction) begin
      if(when_PmpPlugin_l172) begin
        execute_arbitration_haltItself = (! execute_PmpPlugin_fsmComplete); // @[PmpPlugin.scala 174:34]
      end
    end
  end

  always @(*) begin
    execute_arbitration_haltByOther = 1'b0; // @[Stage.scala 50:23]
    if(when_DBusCachedPlugin_l366) begin
      execute_arbitration_haltByOther = 1'b1; // @[DBusCachedPlugin.scala 367:33]
    end
    if(when_DebugPlugin_l308) begin
      execute_arbitration_haltByOther = 1'b1; // @[DebugPlugin.scala 309:41]
    end
  end

  always @(*) begin
    execute_arbitration_removeIt = 1'b0; // @[Stage.scala 51:23]
    if(CsrPlugin_selfException_valid) begin
      execute_arbitration_removeIt = 1'b1; // @[CsrPlugin.scala 1171:40]
    end
    if(execute_arbitration_isFlushed) begin
      execute_arbitration_removeIt = 1'b1; // @[Pipeline.scala 134:36]
    end
  end

  always @(*) begin
    execute_arbitration_flushIt = 1'b0; // @[Stage.scala 52:22]
    if(when_DebugPlugin_l308) begin
      if(when_DebugPlugin_l311) begin
        execute_arbitration_flushIt = 1'b1; // @[DebugPlugin.scala 313:41]
      end
    end
  end

  always @(*) begin
    execute_arbitration_flushNext = 1'b0; // @[Stage.scala 53:24]
    if(CsrPlugin_selfException_valid) begin
      execute_arbitration_flushNext = 1'b1; // @[CsrPlugin.scala 1170:41]
    end
    if(when_DebugPlugin_l308) begin
      if(when_DebugPlugin_l311) begin
        execute_arbitration_flushNext = 1'b1; // @[DebugPlugin.scala 314:41]
      end
    end
  end

  always @(*) begin
    memory_arbitration_haltItself = 1'b0; // @[Stage.scala 49:23]
    if(when_MulDivIterativePlugin_l96) begin
      if(when_MulDivIterativePlugin_l97) begin
        memory_arbitration_haltItself = 1'b1; // @[MulDivIterativePlugin.scala 98:36]
      end
      if(when_MulDivIterativePlugin_l100) begin
        memory_arbitration_haltItself = 1'b1; // @[MulDivIterativePlugin.scala 101:36]
      end
    end
    if(when_MulDivIterativePlugin_l128) begin
      if(when_MulDivIterativePlugin_l129) begin
        memory_arbitration_haltItself = 1'b1; // @[MulDivIterativePlugin.scala 130:36]
      end
    end
  end

  assign memory_arbitration_haltByOther = 1'b0; // @[Stage.scala 50:23]
  always @(*) begin
    memory_arbitration_removeIt = 1'b0; // @[Stage.scala 51:23]
    if(BranchPlugin_branchExceptionPort_valid) begin
      memory_arbitration_removeIt = 1'b1; // @[CsrPlugin.scala 1171:40]
    end
    if(memory_arbitration_isFlushed) begin
      memory_arbitration_removeIt = 1'b1; // @[Pipeline.scala 134:36]
    end
  end

  assign memory_arbitration_flushIt = 1'b0; // @[Stage.scala 52:22]
  always @(*) begin
    memory_arbitration_flushNext = 1'b0; // @[Stage.scala 53:24]
    if(BranchPlugin_branchExceptionPort_valid) begin
      memory_arbitration_flushNext = 1'b1; // @[CsrPlugin.scala 1170:41]
    end
    if(BranchPlugin_jumpInterface_valid) begin
      memory_arbitration_flushNext = 1'b1; // @[BranchPlugin.scala 294:29]
    end
  end

  always @(*) begin
    writeBack_arbitration_haltItself = 1'b0; // @[Stage.scala 49:23]
    if(when_DBusCachedPlugin_l466) begin
      writeBack_arbitration_haltItself = 1'b1; // @[DBusCachedPlugin.scala 466:37]
    end
  end

  assign writeBack_arbitration_haltByOther = 1'b0; // @[Stage.scala 50:23]
  always @(*) begin
    writeBack_arbitration_removeIt = 1'b0; // @[Stage.scala 51:23]
    if(DBusCachedPlugin_exceptionBus_valid) begin
      writeBack_arbitration_removeIt = 1'b1; // @[CsrPlugin.scala 1171:40]
    end
    if(writeBack_arbitration_isFlushed) begin
      writeBack_arbitration_removeIt = 1'b1; // @[Pipeline.scala 134:36]
    end
  end

  always @(*) begin
    writeBack_arbitration_flushIt = 1'b0; // @[Stage.scala 52:22]
    if(DBusCachedPlugin_redoBranch_valid) begin
      writeBack_arbitration_flushIt = 1'b1; // @[DBusCachedPlugin.scala 436:27]
    end
  end

  always @(*) begin
    writeBack_arbitration_flushNext = 1'b0; // @[Stage.scala 53:24]
    if(DBusCachedPlugin_redoBranch_valid) begin
      writeBack_arbitration_flushNext = 1'b1; // @[DBusCachedPlugin.scala 437:29]
    end
    if(DBusCachedPlugin_exceptionBus_valid) begin
      writeBack_arbitration_flushNext = 1'b1; // @[CsrPlugin.scala 1170:41]
    end
    if(when_CsrPlugin_l1310) begin
      writeBack_arbitration_flushNext = 1'b1; // @[CsrPlugin.scala 1316:41]
    end
    if(when_CsrPlugin_l1376) begin
      writeBack_arbitration_flushNext = 1'b1; // @[CsrPlugin.scala 1379:43]
    end
  end

  assign lastStageInstruction = writeBack_INSTRUCTION; // @[Misc.scala 552:9]
  assign lastStagePc = writeBack_PC; // @[Misc.scala 552:9]
  assign lastStageIsValid = writeBack_arbitration_isValid; // @[Misc.scala 552:9]
  assign lastStageIsFiring = writeBack_arbitration_isFiring; // @[Misc.scala 552:9]
  always @(*) begin
    IBusCachedPlugin_fetcherHalt = 1'b0; // @[Fetcher.scala 67:19]
    if(when_CsrPlugin_l1192) begin
      IBusCachedPlugin_fetcherHalt = 1'b1; // @[Fetcher.scala 53:45]
    end
    if(when_CsrPlugin_l1310) begin
      IBusCachedPlugin_fetcherHalt = 1'b1; // @[Fetcher.scala 53:45]
    end
    if(when_CsrPlugin_l1376) begin
      IBusCachedPlugin_fetcherHalt = 1'b1; // @[Fetcher.scala 53:45]
    end
    if(when_DebugPlugin_l308) begin
      if(when_DebugPlugin_l311) begin
        IBusCachedPlugin_fetcherHalt = 1'b1; // @[Fetcher.scala 53:45]
      end
    end
    if(DebugPlugin_haltIt) begin
      IBusCachedPlugin_fetcherHalt = 1'b1; // @[Fetcher.scala 53:45]
    end
    if(when_DebugPlugin_l324) begin
      IBusCachedPlugin_fetcherHalt = 1'b1; // @[Fetcher.scala 53:45]
    end
  end

  assign IBusCachedPlugin_forceNoDecodeCond = 1'b0; // @[Fetcher.scala 68:25]
  always @(*) begin
    IBusCachedPlugin_incomingInstruction = 1'b0; // @[Fetcher.scala 69:27]
    if(when_Fetcher_l243) begin
      IBusCachedPlugin_incomingInstruction = 1'b1; // @[Fetcher.scala 243:27]
    end
  end

  always @(*) begin
    _zz_when_DBusCachedPlugin_l393 = 1'b0; // @[DBusCachedPlugin.scala 252:41]
    if(DebugPlugin_godmode) begin
      _zz_when_DBusCachedPlugin_l393 = 1'b1; // @[DebugPlugin.scala 362:87]
    end
  end

  always @(*) begin
    CsrPlugin_csrMapping_allowCsrSignal = 1'b0; // @[CsrPlugin.scala 358:24]
    if(execute_CsrPlugin_readInstruction) begin
      if(when_PmpPlugin_l155) begin
        if(execute_PmpPlugin_pmpcfgCsr) begin
          CsrPlugin_csrMapping_allowCsrSignal = 1'b1; // @[CsrPlugin.scala 378:44]
        end
        if(execute_PmpPlugin_pmpaddrCsr) begin
          CsrPlugin_csrMapping_allowCsrSignal = 1'b1; // @[CsrPlugin.scala 378:44]
        end
      end
    end
    if(execute_CsrPlugin_writeInstruction) begin
      if(when_PmpPlugin_l172) begin
        CsrPlugin_csrMapping_allowCsrSignal = 1'b1; // @[CsrPlugin.scala 378:44]
      end
    end
  end

  always @(*) begin
    CsrPlugin_csrMapping_readDataSignal = CsrPlugin_csrMapping_readDataInit; // @[CsrPlugin.scala 361:18]
    if(execute_CsrPlugin_readInstruction) begin
      if(when_PmpPlugin_l155) begin
        if(execute_PmpPlugin_pmpcfgCsr) begin
          CsrPlugin_csrMapping_readDataSignal = {{{_zz_CsrPlugin_csrMapping_readDataSignal,_zz_CsrPlugin_csrMapping_readDataSignal_2},_zz_CsrPlugin_csrMapping_readDataSignal_4},_zz_CsrPlugin_csrMapping_readDataSignal_6}; // @[PmpPlugin.scala 158:35]
        end
        if(execute_PmpPlugin_pmpaddrCsr) begin
          CsrPlugin_csrMapping_readDataSignal = _zz_PmpPlugin_pmpaddr_port2; // @[PmpPlugin.scala 166:35]
        end
      end
    end
  end

  always @(*) begin
    CsrPlugin_inWfi = 1'b0; // @[CsrPlugin.scala 552:13]
    if(when_CsrPlugin_l1439) begin
      CsrPlugin_inWfi = 1'b1; // @[CsrPlugin.scala 1440:17]
    end
  end

  always @(*) begin
    CsrPlugin_thirdPartyWake = 1'b0; // @[CsrPlugin.scala 554:22]
    if(DebugPlugin_haltIt) begin
      CsrPlugin_thirdPartyWake = 1'b1; // @[CsrPlugin.scala 479:49]
    end
  end

  always @(*) begin
    CsrPlugin_jumpInterface_valid = 1'b0; // @[CsrPlugin.scala 596:25]
    if(when_CsrPlugin_l1310) begin
      CsrPlugin_jumpInterface_valid = 1'b1; // @[CsrPlugin.scala 1314:37]
    end
    if(when_CsrPlugin_l1376) begin
      CsrPlugin_jumpInterface_valid = 1'b1; // @[CsrPlugin.scala 1378:31]
    end
  end

  always @(*) begin
    CsrPlugin_jumpInterface_payload = 32'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx; // @[UInt.scala 467:20]
    if(when_CsrPlugin_l1310) begin
      CsrPlugin_jumpInterface_payload = {CsrPlugin_xtvec_base,2'b00}; // @[CsrPlugin.scala 1315:37]
    end
    if(when_CsrPlugin_l1376) begin
      case(switch_CsrPlugin_l1380)
        2'b11 : begin
          CsrPlugin_jumpInterface_payload = CsrPlugin_mepc; // @[CsrPlugin.scala 1385:37]
        end
        default : begin
        end
      endcase
    end
  end

  always @(*) begin
    CsrPlugin_forceMachineWire = 1'b0; // @[CsrPlugin.scala 615:24]
    if(DebugPlugin_godmode) begin
      CsrPlugin_forceMachineWire = 1'b1; // @[CsrPlugin.scala 651:56]
    end
  end

  always @(*) begin
    CsrPlugin_allowInterrupts = 1'b1; // @[CsrPlugin.scala 620:23]
    if(when_DebugPlugin_l344) begin
      CsrPlugin_allowInterrupts = 1'b0; // @[CsrPlugin.scala 644:53]
    end
  end

  always @(*) begin
    CsrPlugin_allowException = 1'b1; // @[CsrPlugin.scala 621:22]
    if(DebugPlugin_godmode) begin
      CsrPlugin_allowException = 1'b0; // @[CsrPlugin.scala 645:53]
    end
  end

  always @(*) begin
    CsrPlugin_allowEbreakException = 1'b1; // @[CsrPlugin.scala 622:28]
    if(DebugPlugin_allowEBreak) begin
      CsrPlugin_allowEbreakException = 1'b0; // @[CsrPlugin.scala 646:65]
    end
  end

  always @(*) begin
    CsrPlugin_xretAwayFromMachine = 1'b0; // @[CsrPlugin.scala 637:27]
    if(when_CsrPlugin_l1376) begin
      case(switch_CsrPlugin_l1380)
        2'b11 : begin
          if(when_CsrPlugin_l1388) begin
            CsrPlugin_xretAwayFromMachine = 1'b1; // @[CsrPlugin.scala 1388:37]
          end
        end
        default : begin
        end
      endcase
    end
  end

  always @(*) begin
    BranchPlugin_inDebugNoFetchFlag = 1'b0; // @[BranchPlugin.scala 155:26]
    if(DebugPlugin_godmode) begin
      BranchPlugin_inDebugNoFetchFlag = 1'b1; // @[BranchPlugin.scala 90:60]
    end
  end

  assign IBusCachedPlugin_externalFlush = ({writeBack_arbitration_flushNext,{memory_arbitration_flushNext,{execute_arbitration_flushNext,decode_arbitration_flushNext}}} != 4'b0000); // @[BaseType.scala 305:24]
  assign IBusCachedPlugin_jump_pcLoad_valid = ({BranchPlugin_jumpInterface_valid,{CsrPlugin_jumpInterface_valid,{DBusCachedPlugin_redoBranch_valid,IBusCachedPlugin_predictionJumpInterface_valid}}} != 4'b0000); // @[Fetcher.scala 116:20]
  assign _zz_IBusCachedPlugin_jump_pcLoad_payload = {IBusCachedPlugin_predictionJumpInterface_valid,{BranchPlugin_jumpInterface_valid,{CsrPlugin_jumpInterface_valid,DBusCachedPlugin_redoBranch_valid}}}; // @[BaseType.scala 318:22]
  assign _zz_IBusCachedPlugin_jump_pcLoad_payload_1 = (_zz_IBusCachedPlugin_jump_pcLoad_payload & (~ _zz__zz_IBusCachedPlugin_jump_pcLoad_payload_1)); // @[Bits.scala 133:56]
  assign _zz_IBusCachedPlugin_jump_pcLoad_payload_2 = _zz_IBusCachedPlugin_jump_pcLoad_payload_1[3]; // @[BaseType.scala 305:24]
  assign _zz_IBusCachedPlugin_jump_pcLoad_payload_3 = (_zz_IBusCachedPlugin_jump_pcLoad_payload_1[1] || _zz_IBusCachedPlugin_jump_pcLoad_payload_2); // @[BaseType.scala 305:24]
  assign _zz_IBusCachedPlugin_jump_pcLoad_payload_4 = (_zz_IBusCachedPlugin_jump_pcLoad_payload_1[2] || _zz_IBusCachedPlugin_jump_pcLoad_payload_2); // @[BaseType.scala 305:24]
  assign IBusCachedPlugin_jump_pcLoad_payload = _zz_IBusCachedPlugin_jump_pcLoad_payload_5; // @[Fetcher.scala 117:22]
  always @(*) begin
    IBusCachedPlugin_fetchPc_correction = 1'b0; // @[Fetcher.scala 129:24]
    if(IBusCachedPlugin_fetchPc_redo_valid) begin
      IBusCachedPlugin_fetchPc_correction = 1'b1; // @[Fetcher.scala 151:20]
    end
    if(IBusCachedPlugin_jump_pcLoad_valid) begin
      IBusCachedPlugin_fetchPc_correction = 1'b1; // @[Fetcher.scala 156:20]
    end
  end

  assign IBusCachedPlugin_fetchPc_output_fire = (IBusCachedPlugin_fetchPc_output_valid && IBusCachedPlugin_fetchPc_output_ready); // @[BaseType.scala 305:24]
  assign IBusCachedPlugin_fetchPc_corrected = (IBusCachedPlugin_fetchPc_correction || IBusCachedPlugin_fetchPc_correctionReg); // @[BaseType.scala 305:24]
  always @(*) begin
    IBusCachedPlugin_fetchPc_pcRegPropagate = 1'b0; // @[Fetcher.scala 132:28]
    if(IBusCachedPlugin_iBusRsp_stages_1_input_ready) begin
      IBusCachedPlugin_fetchPc_pcRegPropagate = 1'b1; // @[Fetcher.scala 235:34]
    end
  end

  assign when_Fetcher_l134 = (IBusCachedPlugin_fetchPc_correction || IBusCachedPlugin_fetchPc_pcRegPropagate); // @[BaseType.scala 305:24]
  assign IBusCachedPlugin_fetchPc_output_fire_1 = (IBusCachedPlugin_fetchPc_output_valid && IBusCachedPlugin_fetchPc_output_ready); // @[BaseType.scala 305:24]
  assign when_Fetcher_l134_1 = ((! IBusCachedPlugin_fetchPc_output_valid) && IBusCachedPlugin_fetchPc_output_ready); // @[BaseType.scala 305:24]
  always @(*) begin
    IBusCachedPlugin_fetchPc_pc = (IBusCachedPlugin_fetchPc_pcReg + _zz_IBusCachedPlugin_fetchPc_pc); // @[BaseType.scala 299:24]
    if(IBusCachedPlugin_fetchPc_redo_valid) begin
      IBusCachedPlugin_fetchPc_pc = IBusCachedPlugin_fetchPc_redo_payload; // @[Fetcher.scala 152:12]
    end
    if(IBusCachedPlugin_jump_pcLoad_valid) begin
      IBusCachedPlugin_fetchPc_pc = IBusCachedPlugin_jump_pcLoad_payload; // @[Fetcher.scala 157:12]
    end
    IBusCachedPlugin_fetchPc_pc[0] = 1'b0; // @[Fetcher.scala 165:13]
    IBusCachedPlugin_fetchPc_pc[1] = 1'b0; // @[Fetcher.scala 166:32]
  end

  always @(*) begin
    IBusCachedPlugin_fetchPc_flushed = 1'b0; // @[Fetcher.scala 138:21]
    if(IBusCachedPlugin_fetchPc_redo_valid) begin
      IBusCachedPlugin_fetchPc_flushed = 1'b1; // @[Fetcher.scala 153:17]
    end
    if(IBusCachedPlugin_jump_pcLoad_valid) begin
      IBusCachedPlugin_fetchPc_flushed = 1'b1; // @[Fetcher.scala 158:17]
    end
  end

  assign when_Fetcher_l161 = (IBusCachedPlugin_fetchPc_booted && ((IBusCachedPlugin_fetchPc_output_ready || IBusCachedPlugin_fetchPc_correction) || IBusCachedPlugin_fetchPc_pcRegPropagate)); // @[BaseType.scala 305:24]
  assign IBusCachedPlugin_fetchPc_output_valid = ((! IBusCachedPlugin_fetcherHalt) && IBusCachedPlugin_fetchPc_booted); // @[Fetcher.scala 168:20]
  assign IBusCachedPlugin_fetchPc_output_payload = IBusCachedPlugin_fetchPc_pc; // @[Fetcher.scala 169:22]
  always @(*) begin
    IBusCachedPlugin_iBusRsp_redoFetch = 1'b0; // @[Fetcher.scala 210:23]
    if(IBusCachedPlugin_rsp_redoFetch) begin
      IBusCachedPlugin_iBusRsp_redoFetch = 1'b1; // @[IBusCachedPlugin.scala 263:29]
    end
  end

  assign IBusCachedPlugin_iBusRsp_stages_0_input_valid = IBusCachedPlugin_fetchPc_output_valid; // @[Stream.scala 294:16]
  assign IBusCachedPlugin_fetchPc_output_ready = IBusCachedPlugin_iBusRsp_stages_0_input_ready; // @[Stream.scala 295:16]
  assign IBusCachedPlugin_iBusRsp_stages_0_input_payload = IBusCachedPlugin_fetchPc_output_payload; // @[Stream.scala 296:18]
  always @(*) begin
    IBusCachedPlugin_iBusRsp_stages_0_halt = 1'b0; // @[Fetcher.scala 219:16]
    if(IBusCachedPlugin_cache_io_cpu_prefetch_haltIt) begin
      IBusCachedPlugin_iBusRsp_stages_0_halt = 1'b1; // @[IBusCachedPlugin.scala 167:24]
    end
  end

  assign _zz_IBusCachedPlugin_iBusRsp_stages_0_input_ready = (! IBusCachedPlugin_iBusRsp_stages_0_halt); // @[BaseType.scala 299:24]
  assign IBusCachedPlugin_iBusRsp_stages_0_input_ready = (IBusCachedPlugin_iBusRsp_stages_0_output_ready && _zz_IBusCachedPlugin_iBusRsp_stages_0_input_ready); // @[Stream.scala 427:16]
  assign IBusCachedPlugin_iBusRsp_stages_0_output_valid = (IBusCachedPlugin_iBusRsp_stages_0_input_valid && _zz_IBusCachedPlugin_iBusRsp_stages_0_input_ready); // @[Stream.scala 294:16]
  assign IBusCachedPlugin_iBusRsp_stages_0_output_payload = IBusCachedPlugin_iBusRsp_stages_0_input_payload; // @[Stream.scala 296:18]
  always @(*) begin
    IBusCachedPlugin_iBusRsp_stages_1_halt = 1'b0; // @[Fetcher.scala 219:16]
    if(IBusCachedPlugin_mmuBus_busy) begin
      IBusCachedPlugin_iBusRsp_stages_1_halt = 1'b1; // @[IBusCachedPlugin.scala 197:53]
    end
  end

  assign _zz_IBusCachedPlugin_iBusRsp_stages_1_input_ready = (! IBusCachedPlugin_iBusRsp_stages_1_halt); // @[BaseType.scala 299:24]
  assign IBusCachedPlugin_iBusRsp_stages_1_input_ready = (IBusCachedPlugin_iBusRsp_stages_1_output_ready && _zz_IBusCachedPlugin_iBusRsp_stages_1_input_ready); // @[Stream.scala 427:16]
  assign IBusCachedPlugin_iBusRsp_stages_1_output_valid = (IBusCachedPlugin_iBusRsp_stages_1_input_valid && _zz_IBusCachedPlugin_iBusRsp_stages_1_input_ready); // @[Stream.scala 294:16]
  assign IBusCachedPlugin_iBusRsp_stages_1_output_payload = IBusCachedPlugin_iBusRsp_stages_1_input_payload; // @[Stream.scala 296:18]
  always @(*) begin
    IBusCachedPlugin_iBusRsp_stages_2_halt = 1'b0; // @[Fetcher.scala 219:16]
    if(when_IBusCachedPlugin_l267) begin
      IBusCachedPlugin_iBusRsp_stages_2_halt = 1'b1; // @[IBusCachedPlugin.scala 267:34]
    end
  end

  assign _zz_IBusCachedPlugin_iBusRsp_stages_2_input_ready = (! IBusCachedPlugin_iBusRsp_stages_2_halt); // @[BaseType.scala 299:24]
  assign IBusCachedPlugin_iBusRsp_stages_2_input_ready = (IBusCachedPlugin_iBusRsp_stages_2_output_ready && _zz_IBusCachedPlugin_iBusRsp_stages_2_input_ready); // @[Stream.scala 427:16]
  assign IBusCachedPlugin_iBusRsp_stages_2_output_valid = (IBusCachedPlugin_iBusRsp_stages_2_input_valid && _zz_IBusCachedPlugin_iBusRsp_stages_2_input_ready); // @[Stream.scala 294:16]
  assign IBusCachedPlugin_iBusRsp_stages_2_output_payload = IBusCachedPlugin_iBusRsp_stages_2_input_payload; // @[Stream.scala 296:18]
  assign IBusCachedPlugin_fetchPc_redo_valid = IBusCachedPlugin_iBusRsp_redoFetch; // @[Fetcher.scala 224:28]
  assign IBusCachedPlugin_fetchPc_redo_payload = IBusCachedPlugin_iBusRsp_stages_2_input_payload; // @[Fetcher.scala 225:30]
  assign IBusCachedPlugin_iBusRsp_flush = ((decode_arbitration_removeIt || (decode_arbitration_flushNext && (! decode_arbitration_isStuck))) || IBusCachedPlugin_iBusRsp_redoFetch); // @[BaseType.scala 305:24]
  assign IBusCachedPlugin_iBusRsp_stages_0_output_ready = _zz_IBusCachedPlugin_iBusRsp_stages_0_output_ready; // @[Stream.scala 304:16]
  assign _zz_IBusCachedPlugin_iBusRsp_stages_0_output_ready = ((1'b0 && (! _zz_IBusCachedPlugin_iBusRsp_stages_1_input_valid)) || IBusCachedPlugin_iBusRsp_stages_1_input_ready); // @[Misc.scala 148:20]
  assign _zz_IBusCachedPlugin_iBusRsp_stages_1_input_valid = _zz_IBusCachedPlugin_iBusRsp_stages_1_input_valid_1; // @[Misc.scala 158:17]
  assign IBusCachedPlugin_iBusRsp_stages_1_input_valid = _zz_IBusCachedPlugin_iBusRsp_stages_1_input_valid; // @[Stream.scala 303:16]
  assign IBusCachedPlugin_iBusRsp_stages_1_input_payload = IBusCachedPlugin_fetchPc_pcReg; // @[Fetcher.scala 234:31]
  assign IBusCachedPlugin_iBusRsp_stages_1_output_ready = ((1'b0 && (! IBusCachedPlugin_iBusRsp_stages_1_output_m2sPipe_valid)) || IBusCachedPlugin_iBusRsp_stages_1_output_m2sPipe_ready); // @[Misc.scala 148:20]
  assign IBusCachedPlugin_iBusRsp_stages_1_output_m2sPipe_valid = _zz_IBusCachedPlugin_iBusRsp_stages_1_output_m2sPipe_valid; // @[Misc.scala 158:17]
  assign IBusCachedPlugin_iBusRsp_stages_1_output_m2sPipe_payload = _zz_IBusCachedPlugin_iBusRsp_stages_1_output_m2sPipe_payload; // @[Misc.scala 159:19]
  assign IBusCachedPlugin_iBusRsp_stages_2_input_valid = IBusCachedPlugin_iBusRsp_stages_1_output_m2sPipe_valid; // @[Stream.scala 294:16]
  assign IBusCachedPlugin_iBusRsp_stages_1_output_m2sPipe_ready = IBusCachedPlugin_iBusRsp_stages_2_input_ready; // @[Stream.scala 295:16]
  assign IBusCachedPlugin_iBusRsp_stages_2_input_payload = IBusCachedPlugin_iBusRsp_stages_1_output_m2sPipe_payload; // @[Stream.scala 296:18]
  always @(*) begin
    IBusCachedPlugin_iBusRsp_readyForError = 1'b1; // @[Fetcher.scala 241:27]
    if(when_Fetcher_l323) begin
      IBusCachedPlugin_iBusRsp_readyForError = 1'b0; // @[Fetcher.scala 323:55]
    end
  end

  assign when_Fetcher_l243 = (IBusCachedPlugin_iBusRsp_stages_1_input_valid || IBusCachedPlugin_iBusRsp_stages_2_input_valid); // @[BaseType.scala 305:24]
  assign when_Fetcher_l323 = (! IBusCachedPlugin_pcValids_0); // @[BaseType.scala 299:24]
  assign when_Fetcher_l332 = (! (! IBusCachedPlugin_iBusRsp_stages_1_input_ready)); // @[BaseType.scala 299:24]
  assign when_Fetcher_l332_1 = (! (! IBusCachedPlugin_iBusRsp_stages_2_input_ready)); // @[BaseType.scala 299:24]
  assign when_Fetcher_l332_2 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Fetcher_l332_3 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Fetcher_l332_4 = (! writeBack_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign IBusCachedPlugin_pcValids_0 = IBusCachedPlugin_injector_nextPcCalc_valids_1; // @[Fetcher.scala 348:18]
  assign IBusCachedPlugin_pcValids_1 = IBusCachedPlugin_injector_nextPcCalc_valids_2; // @[Fetcher.scala 348:18]
  assign IBusCachedPlugin_pcValids_2 = IBusCachedPlugin_injector_nextPcCalc_valids_3; // @[Fetcher.scala 348:18]
  assign IBusCachedPlugin_pcValids_3 = IBusCachedPlugin_injector_nextPcCalc_valids_4; // @[Fetcher.scala 348:18]
  assign IBusCachedPlugin_iBusRsp_output_ready = (! decode_arbitration_isStuck); // @[Fetcher.scala 351:25]
  always @(*) begin
    decode_arbitration_isValid = IBusCachedPlugin_iBusRsp_output_valid; // @[Fetcher.scala 352:34]
    case(switch_Fetcher_l365)
      3'b010 : begin
        decode_arbitration_isValid = 1'b1; // @[Fetcher.scala 375:42]
      end
      3'b011 : begin
        decode_arbitration_isValid = 1'b1; // @[Fetcher.scala 380:42]
      end
      default : begin
      end
    endcase
    if(IBusCachedPlugin_forceNoDecodeCond) begin
      decode_arbitration_isValid = 1'b0; // @[Fetcher.scala 415:36]
    end
  end

  assign _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch = _zz__zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch[11]; // @[BaseType.scala 305:24]
  always @(*) begin
    _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch_1[18] = _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch_1[17] = _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch_1[16] = _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch_1[15] = _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch_1[14] = _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch_1[13] = _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch_1[12] = _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch_1[11] = _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch_1[10] = _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch_1[9] = _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch_1[8] = _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch_1[7] = _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch_1[6] = _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch_1[5] = _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch_1[4] = _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch_1[3] = _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch_1[2] = _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch_1[1] = _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch_1[0] = _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch; // @[Literal.scala 87:17]
  end

  always @(*) begin
    IBusCachedPlugin_decodePrediction_cmd_hadBranch = ((decode_BRANCH_CTRL == BranchCtrlEnum_JAL) || ((decode_BRANCH_CTRL == BranchCtrlEnum_B) && _zz_IBusCachedPlugin_decodePrediction_cmd_hadBranch_2[31])); // @[Fetcher.scala 502:40]
    if(_zz_7) begin
      IBusCachedPlugin_decodePrediction_cmd_hadBranch = 1'b0; // @[Fetcher.scala 509:42]
    end
  end

  assign _zz_3 = _zz__zz_3[19]; // @[BaseType.scala 305:24]
  always @(*) begin
    _zz_4[10] = _zz_3; // @[Literal.scala 87:17]
    _zz_4[9] = _zz_3; // @[Literal.scala 87:17]
    _zz_4[8] = _zz_3; // @[Literal.scala 87:17]
    _zz_4[7] = _zz_3; // @[Literal.scala 87:17]
    _zz_4[6] = _zz_3; // @[Literal.scala 87:17]
    _zz_4[5] = _zz_3; // @[Literal.scala 87:17]
    _zz_4[4] = _zz_3; // @[Literal.scala 87:17]
    _zz_4[3] = _zz_3; // @[Literal.scala 87:17]
    _zz_4[2] = _zz_3; // @[Literal.scala 87:17]
    _zz_4[1] = _zz_3; // @[Literal.scala 87:17]
    _zz_4[0] = _zz_3; // @[Literal.scala 87:17]
  end

  assign _zz_5 = _zz__zz_5[11]; // @[BaseType.scala 305:24]
  always @(*) begin
    _zz_6[18] = _zz_5; // @[Literal.scala 87:17]
    _zz_6[17] = _zz_5; // @[Literal.scala 87:17]
    _zz_6[16] = _zz_5; // @[Literal.scala 87:17]
    _zz_6[15] = _zz_5; // @[Literal.scala 87:17]
    _zz_6[14] = _zz_5; // @[Literal.scala 87:17]
    _zz_6[13] = _zz_5; // @[Literal.scala 87:17]
    _zz_6[12] = _zz_5; // @[Literal.scala 87:17]
    _zz_6[11] = _zz_5; // @[Literal.scala 87:17]
    _zz_6[10] = _zz_5; // @[Literal.scala 87:17]
    _zz_6[9] = _zz_5; // @[Literal.scala 87:17]
    _zz_6[8] = _zz_5; // @[Literal.scala 87:17]
    _zz_6[7] = _zz_5; // @[Literal.scala 87:17]
    _zz_6[6] = _zz_5; // @[Literal.scala 87:17]
    _zz_6[5] = _zz_5; // @[Literal.scala 87:17]
    _zz_6[4] = _zz_5; // @[Literal.scala 87:17]
    _zz_6[3] = _zz_5; // @[Literal.scala 87:17]
    _zz_6[2] = _zz_5; // @[Literal.scala 87:17]
    _zz_6[1] = _zz_5; // @[Literal.scala 87:17]
    _zz_6[0] = _zz_5; // @[Literal.scala 87:17]
  end

  always @(*) begin
    case(decode_BRANCH_CTRL)
      BranchCtrlEnum_JAL : begin
        _zz_7 = _zz__zz_7[1]; // @[Misc.scala 239:22]
      end
      default : begin
        _zz_7 = _zz__zz_7_1[1]; // @[Misc.scala 235:22]
      end
    endcase
  end

  assign IBusCachedPlugin_predictionJumpInterface_valid = (decode_arbitration_isValid && IBusCachedPlugin_decodePrediction_cmd_hadBranch); // @[Fetcher.scala 513:39]
  assign _zz_IBusCachedPlugin_predictionJumpInterface_payload = _zz__zz_IBusCachedPlugin_predictionJumpInterface_payload[19]; // @[BaseType.scala 305:24]
  always @(*) begin
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_1[10] = _zz_IBusCachedPlugin_predictionJumpInterface_payload; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_1[9] = _zz_IBusCachedPlugin_predictionJumpInterface_payload; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_1[8] = _zz_IBusCachedPlugin_predictionJumpInterface_payload; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_1[7] = _zz_IBusCachedPlugin_predictionJumpInterface_payload; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_1[6] = _zz_IBusCachedPlugin_predictionJumpInterface_payload; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_1[5] = _zz_IBusCachedPlugin_predictionJumpInterface_payload; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_1[4] = _zz_IBusCachedPlugin_predictionJumpInterface_payload; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_1[3] = _zz_IBusCachedPlugin_predictionJumpInterface_payload; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_1[2] = _zz_IBusCachedPlugin_predictionJumpInterface_payload; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_1[1] = _zz_IBusCachedPlugin_predictionJumpInterface_payload; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_1[0] = _zz_IBusCachedPlugin_predictionJumpInterface_payload; // @[Literal.scala 87:17]
  end

  assign _zz_IBusCachedPlugin_predictionJumpInterface_payload_2 = _zz__zz_IBusCachedPlugin_predictionJumpInterface_payload_2[11]; // @[BaseType.scala 305:24]
  always @(*) begin
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_3[18] = _zz_IBusCachedPlugin_predictionJumpInterface_payload_2; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_3[17] = _zz_IBusCachedPlugin_predictionJumpInterface_payload_2; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_3[16] = _zz_IBusCachedPlugin_predictionJumpInterface_payload_2; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_3[15] = _zz_IBusCachedPlugin_predictionJumpInterface_payload_2; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_3[14] = _zz_IBusCachedPlugin_predictionJumpInterface_payload_2; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_3[13] = _zz_IBusCachedPlugin_predictionJumpInterface_payload_2; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_3[12] = _zz_IBusCachedPlugin_predictionJumpInterface_payload_2; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_3[11] = _zz_IBusCachedPlugin_predictionJumpInterface_payload_2; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_3[10] = _zz_IBusCachedPlugin_predictionJumpInterface_payload_2; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_3[9] = _zz_IBusCachedPlugin_predictionJumpInterface_payload_2; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_3[8] = _zz_IBusCachedPlugin_predictionJumpInterface_payload_2; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_3[7] = _zz_IBusCachedPlugin_predictionJumpInterface_payload_2; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_3[6] = _zz_IBusCachedPlugin_predictionJumpInterface_payload_2; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_3[5] = _zz_IBusCachedPlugin_predictionJumpInterface_payload_2; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_3[4] = _zz_IBusCachedPlugin_predictionJumpInterface_payload_2; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_3[3] = _zz_IBusCachedPlugin_predictionJumpInterface_payload_2; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_3[2] = _zz_IBusCachedPlugin_predictionJumpInterface_payload_2; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_3[1] = _zz_IBusCachedPlugin_predictionJumpInterface_payload_2; // @[Literal.scala 87:17]
    _zz_IBusCachedPlugin_predictionJumpInterface_payload_3[0] = _zz_IBusCachedPlugin_predictionJumpInterface_payload_2; // @[Literal.scala 87:17]
  end

  assign IBusCachedPlugin_predictionJumpInterface_payload = (decode_PC + ((decode_BRANCH_CTRL == BranchCtrlEnum_JAL) ? {{_zz_IBusCachedPlugin_predictionJumpInterface_payload_1,{{{_zz_IBusCachedPlugin_predictionJumpInterface_payload_4,decode_INSTRUCTION[19 : 12]},decode_INSTRUCTION[20]},decode_INSTRUCTION[30 : 21]}},1'b0} : {{_zz_IBusCachedPlugin_predictionJumpInterface_payload_3,{{{_zz_IBusCachedPlugin_predictionJumpInterface_payload_5,_zz_IBusCachedPlugin_predictionJumpInterface_payload_6},decode_INSTRUCTION[30 : 25]},decode_INSTRUCTION[11 : 8]}},1'b0})); // @[Fetcher.scala 514:41]
  assign iBus_cmd_valid = IBusCachedPlugin_cache_io_mem_cmd_valid; // @[IBusCachedPlugin.scala 140:12]
  always @(*) begin
    iBus_cmd_payload_address = IBusCachedPlugin_cache_io_mem_cmd_payload_address; // @[IBusCachedPlugin.scala 140:12]
    iBus_cmd_payload_address = IBusCachedPlugin_cache_io_mem_cmd_payload_address; // @[IBusCachedPlugin.scala 141:38]
  end

  assign iBus_cmd_payload_size = IBusCachedPlugin_cache_io_mem_cmd_payload_size; // @[IBusCachedPlugin.scala 140:12]
  assign IBusCachedPlugin_s0_tightlyCoupledHit = 1'b0; // @[BaseType.scala 305:24]
  assign IBusCachedPlugin_cache_io_cpu_prefetch_isValid = (IBusCachedPlugin_iBusRsp_stages_0_input_valid && (! IBusCachedPlugin_s0_tightlyCoupledHit)); // @[IBusCachedPlugin.scala 165:39]
  assign IBusCachedPlugin_cache_io_cpu_fetch_isValid = (IBusCachedPlugin_iBusRsp_stages_1_input_valid && (! IBusCachedPlugin_s1_tightlyCoupledHit)); // @[IBusCachedPlugin.scala 187:36]
  assign IBusCachedPlugin_cache_io_cpu_fetch_isStuck = (! IBusCachedPlugin_iBusRsp_stages_1_input_ready); // @[IBusCachedPlugin.scala 188:36]
  assign IBusCachedPlugin_mmuBus_cmd_0_isValid = IBusCachedPlugin_cache_io_cpu_fetch_isValid; // @[IBusCachedPlugin.scala 192:35]
  assign IBusCachedPlugin_mmuBus_cmd_0_isStuck = (! IBusCachedPlugin_iBusRsp_stages_1_input_ready); // @[IBusCachedPlugin.scala 193:35]
  assign IBusCachedPlugin_mmuBus_cmd_0_virtualAddress = IBusCachedPlugin_iBusRsp_stages_1_input_payload; // @[IBusCachedPlugin.scala 194:42]
  assign IBusCachedPlugin_mmuBus_cmd_0_bypassTranslation = 1'b0; // @[IBusCachedPlugin.scala 195:45]
  assign IBusCachedPlugin_mmuBus_end = (IBusCachedPlugin_iBusRsp_stages_1_input_ready || IBusCachedPlugin_externalFlush); // @[IBusCachedPlugin.scala 196:22]
  assign IBusCachedPlugin_cache_io_cpu_decode_isValid = (IBusCachedPlugin_iBusRsp_stages_2_input_valid && (! IBusCachedPlugin_s2_tightlyCoupledHit)); // @[IBusCachedPlugin.scala 208:37]
  assign IBusCachedPlugin_cache_io_cpu_decode_isStuck = (! IBusCachedPlugin_iBusRsp_stages_2_input_ready); // @[IBusCachedPlugin.scala 209:37]
  assign IBusCachedPlugin_cache_io_cpu_decode_isUser = (CsrPlugin_privilege == 2'b00); // @[IBusCachedPlugin.scala 211:36]
  assign IBusCachedPlugin_rsp_iBusRspOutputHalt = 1'b0; // @[IBusCachedPlugin.scala 219:33]
  assign IBusCachedPlugin_rsp_issueDetected = 1'b0; // @[IBusCachedPlugin.scala 223:29]
  always @(*) begin
    IBusCachedPlugin_rsp_redoFetch = 1'b0; // @[IBusCachedPlugin.scala 224:25]
    if(when_IBusCachedPlugin_l239) begin
      IBusCachedPlugin_rsp_redoFetch = 1'b1; // @[IBusCachedPlugin.scala 241:21]
    end
    if(when_IBusCachedPlugin_l250) begin
      IBusCachedPlugin_rsp_redoFetch = 1'b1; // @[IBusCachedPlugin.scala 253:21]
    end
  end

  always @(*) begin
    IBusCachedPlugin_cache_io_cpu_fill_valid = (IBusCachedPlugin_rsp_redoFetch && (! IBusCachedPlugin_cache_io_cpu_decode_mmuRefilling)); // @[IBusCachedPlugin.scala 229:33]
    if(when_IBusCachedPlugin_l250) begin
      IBusCachedPlugin_cache_io_cpu_fill_valid = 1'b1; // @[IBusCachedPlugin.scala 252:35]
    end
  end

  always @(*) begin
    IBusCachedPlugin_decodeExceptionPort_valid = 1'b0; // @[IBusCachedPlugin.scala 234:37]
    if(when_IBusCachedPlugin_l244) begin
      IBusCachedPlugin_decodeExceptionPort_valid = IBusCachedPlugin_iBusRsp_readyForError; // @[IBusCachedPlugin.scala 246:37]
    end
    if(when_IBusCachedPlugin_l256) begin
      IBusCachedPlugin_decodeExceptionPort_valid = IBusCachedPlugin_iBusRsp_readyForError; // @[IBusCachedPlugin.scala 258:37]
    end
  end

  always @(*) begin
    IBusCachedPlugin_decodeExceptionPort_payload_code = 4'bxxxx; // @[UInt.scala 467:20]
    if(when_IBusCachedPlugin_l244) begin
      IBusCachedPlugin_decodeExceptionPort_payload_code = 4'b1100; // @[IBusCachedPlugin.scala 247:36]
    end
    if(when_IBusCachedPlugin_l256) begin
      IBusCachedPlugin_decodeExceptionPort_payload_code = 4'b0001; // @[IBusCachedPlugin.scala 259:36]
    end
  end

  assign IBusCachedPlugin_decodeExceptionPort_payload_badAddr = {IBusCachedPlugin_iBusRsp_stages_2_input_payload[31 : 2],2'b00}; // @[IBusCachedPlugin.scala 236:39]
  assign when_IBusCachedPlugin_l239 = ((IBusCachedPlugin_cache_io_cpu_decode_isValid && IBusCachedPlugin_cache_io_cpu_decode_mmuRefilling) && (! IBusCachedPlugin_rsp_issueDetected)); // @[BaseType.scala 305:24]
  assign when_IBusCachedPlugin_l244 = ((IBusCachedPlugin_cache_io_cpu_decode_isValid && IBusCachedPlugin_cache_io_cpu_decode_mmuException) && (! IBusCachedPlugin_rsp_issueDetected_1)); // @[BaseType.scala 305:24]
  assign when_IBusCachedPlugin_l250 = ((IBusCachedPlugin_cache_io_cpu_decode_isValid && IBusCachedPlugin_cache_io_cpu_decode_cacheMiss) && (! IBusCachedPlugin_rsp_issueDetected_2)); // @[BaseType.scala 305:24]
  assign when_IBusCachedPlugin_l256 = ((IBusCachedPlugin_cache_io_cpu_decode_isValid && IBusCachedPlugin_cache_io_cpu_decode_error) && (! IBusCachedPlugin_rsp_issueDetected_3)); // @[BaseType.scala 305:24]
  assign when_IBusCachedPlugin_l267 = (IBusCachedPlugin_rsp_issueDetected_4 || IBusCachedPlugin_rsp_iBusRspOutputHalt); // @[BaseType.scala 305:24]
  assign IBusCachedPlugin_iBusRsp_output_valid = IBusCachedPlugin_iBusRsp_stages_2_output_valid; // @[IBusCachedPlugin.scala 268:30]
  assign IBusCachedPlugin_iBusRsp_stages_2_output_ready = IBusCachedPlugin_iBusRsp_output_ready; // @[IBusCachedPlugin.scala 269:42]
  assign IBusCachedPlugin_iBusRsp_output_payload_rsp_inst = IBusCachedPlugin_cache_io_cpu_decode_data; // @[IBusCachedPlugin.scala 270:33]
  assign IBusCachedPlugin_iBusRsp_output_payload_pc = IBusCachedPlugin_iBusRsp_stages_2_output_payload; // @[IBusCachedPlugin.scala 271:27]
  assign IBusCachedPlugin_cache_io_flush = (decode_arbitration_isValid && decode_FLUSH_ALL); // @[IBusCachedPlugin.scala 287:22]
  assign dBus_cmd_valid = dataCache_1_io_mem_cmd_valid; // @[Stream.scala 294:16]
  assign dBus_cmd_payload_wr = dataCache_1_io_mem_cmd_payload_wr; // @[Stream.scala 296:18]
  assign dBus_cmd_payload_uncached = dataCache_1_io_mem_cmd_payload_uncached; // @[Stream.scala 296:18]
  assign dBus_cmd_payload_address = dataCache_1_io_mem_cmd_payload_address; // @[Stream.scala 296:18]
  assign dBus_cmd_payload_data = dataCache_1_io_mem_cmd_payload_data; // @[Stream.scala 296:18]
  assign dBus_cmd_payload_mask = dataCache_1_io_mem_cmd_payload_mask; // @[Stream.scala 296:18]
  assign dBus_cmd_payload_size = dataCache_1_io_mem_cmd_payload_size; // @[Stream.scala 296:18]
  assign dBus_cmd_payload_last = dataCache_1_io_mem_cmd_payload_last; // @[Stream.scala 296:18]
  assign when_DBusCachedPlugin_l308 = ((DBusCachedPlugin_mmuBus_busy && decode_arbitration_isValid) && decode_MEMORY_ENABLE); // @[BaseType.scala 305:24]
  assign execute_DBusCachedPlugin_size = execute_INSTRUCTION[13 : 12]; // @[BaseType.scala 318:22]
  assign dataCache_1_io_cpu_execute_isValid = (execute_arbitration_isValid && execute_MEMORY_ENABLE); // @[DBusCachedPlugin.scala 327:36]
  assign dataCache_1_io_cpu_execute_address = execute_SRC_ADD; // @[DBusCachedPlugin.scala 328:36]
  always @(*) begin
    case(execute_DBusCachedPlugin_size)
      2'b00 : begin
        _zz_execute_MEMORY_STORE_DATA_RF = {{{execute_RS2[7 : 0],execute_RS2[7 : 0]},execute_RS2[7 : 0]},execute_RS2[7 : 0]}; // @[Misc.scala 239:22]
      end
      2'b01 : begin
        _zz_execute_MEMORY_STORE_DATA_RF = {execute_RS2[15 : 0],execute_RS2[15 : 0]}; // @[Misc.scala 239:22]
      end
      default : begin
        _zz_execute_MEMORY_STORE_DATA_RF = execute_RS2[31 : 0]; // @[Misc.scala 235:22]
      end
    endcase
  end

  assign dataCache_1_io_cpu_flush_valid = (execute_arbitration_isValid && execute_MEMORY_MANAGMENT); // @[DBusCachedPlugin.scala 346:32]
  assign dataCache_1_io_cpu_flush_payload_singleLine = (execute_INSTRUCTION[19 : 15] != 5'h0); // @[DBusCachedPlugin.scala 347:37]
  assign dataCache_1_io_cpu_flush_payload_lineId = _zz_io_cpu_flush_payload_lineId[6:0]; // @[DBusCachedPlugin.scala 348:33]
  assign toplevel_dataCache_1_io_cpu_flush_isStall = (dataCache_1_io_cpu_flush_valid && (! dataCache_1_io_cpu_flush_ready)); // @[BaseType.scala 305:24]
  assign when_DBusCachedPlugin_l350 = (toplevel_dataCache_1_io_cpu_flush_isStall || dataCache_1_io_cpu_execute_haltIt); // @[BaseType.scala 305:24]
  assign when_DBusCachedPlugin_l366 = (dataCache_1_io_cpu_execute_refilling && execute_arbitration_isValid); // @[BaseType.scala 305:24]
  assign dataCache_1_io_cpu_memory_isValid = (memory_arbitration_isValid && memory_MEMORY_ENABLE); // @[DBusCachedPlugin.scala 383:35]
  assign dataCache_1_io_cpu_memory_address = memory_REGFILE_WRITE_DATA; // @[DBusCachedPlugin.scala 385:35]
  assign DBusCachedPlugin_mmuBus_cmd_0_isValid = dataCache_1_io_cpu_memory_isValid; // @[DBusCachedPlugin.scala 387:31]
  assign DBusCachedPlugin_mmuBus_cmd_0_isStuck = memory_arbitration_isStuck; // @[DBusCachedPlugin.scala 388:31]
  assign DBusCachedPlugin_mmuBus_cmd_0_virtualAddress = dataCache_1_io_cpu_memory_address; // @[DBusCachedPlugin.scala 389:38]
  assign DBusCachedPlugin_mmuBus_cmd_0_bypassTranslation = 1'b0; // @[DBusCachedPlugin.scala 390:41]
  assign DBusCachedPlugin_mmuBus_end = ((! memory_arbitration_isStuck) || memory_arbitration_removeIt); // @[DBusCachedPlugin.scala 391:18]
  always @(*) begin
    dataCache_1_io_cpu_memory_mmuRsp_isIoAccess = DBusCachedPlugin_mmuBus_rsp_isIoAccess; // @[DBusCachedPlugin.scala 392:34]
    if(when_DBusCachedPlugin_l393) begin
      dataCache_1_io_cpu_memory_mmuRsp_isIoAccess = 1'b1; // @[DBusCachedPlugin.scala 393:45]
    end
  end

  assign when_DBusCachedPlugin_l393 = (_zz_when_DBusCachedPlugin_l393 && (! dataCache_1_io_cpu_memory_isWrite)); // @[BaseType.scala 305:24]
  always @(*) begin
    dataCache_1_io_cpu_writeBack_isValid = (writeBack_arbitration_isValid && writeBack_MEMORY_ENABLE); // @[DBusCachedPlugin.scala 399:38]
    if(writeBack_arbitration_haltByOther) begin
      dataCache_1_io_cpu_writeBack_isValid = 1'b0; // @[DBusCachedPlugin.scala 544:38]
    end
  end

  assign dataCache_1_io_cpu_writeBack_isUser = (CsrPlugin_privilege == 2'b00); // @[DBusCachedPlugin.scala 402:38]
  assign dataCache_1_io_cpu_writeBack_address = writeBack_REGFILE_WRITE_DATA; // @[DBusCachedPlugin.scala 403:38]
  assign dataCache_1_io_cpu_writeBack_storeData[31 : 0] = writeBack_MEMORY_STORE_DATA_RF; // @[DBusCachedPlugin.scala 404:71]
  always @(*) begin
    DBusCachedPlugin_redoBranch_valid = 1'b0; // @[DBusCachedPlugin.scala 434:24]
    if(when_DBusCachedPlugin_l446) begin
      if(dataCache_1_io_cpu_redo) begin
        DBusCachedPlugin_redoBranch_valid = 1'b1; // @[DBusCachedPlugin.scala 461:28]
      end
    end
  end

  assign DBusCachedPlugin_redoBranch_payload = writeBack_PC; // @[DBusCachedPlugin.scala 435:26]
  always @(*) begin
    DBusCachedPlugin_exceptionBus_valid = 1'b0; // @[DBusCachedPlugin.scala 440:28]
    if(when_DBusCachedPlugin_l446) begin
      if(dataCache_1_io_cpu_writeBack_accessError) begin
        DBusCachedPlugin_exceptionBus_valid = 1'b1; // @[DBusCachedPlugin.scala 448:30]
      end
      if(dataCache_1_io_cpu_writeBack_mmuException) begin
        DBusCachedPlugin_exceptionBus_valid = 1'b1; // @[DBusCachedPlugin.scala 452:30]
      end
      if(dataCache_1_io_cpu_writeBack_unalignedAccess) begin
        DBusCachedPlugin_exceptionBus_valid = 1'b1; // @[DBusCachedPlugin.scala 456:30]
      end
      if(dataCache_1_io_cpu_redo) begin
        DBusCachedPlugin_exceptionBus_valid = 1'b0; // @[DBusCachedPlugin.scala 462:49]
      end
    end
  end

  assign DBusCachedPlugin_exceptionBus_payload_badAddr = writeBack_REGFILE_WRITE_DATA; // @[DBusCachedPlugin.scala 441:30]
  always @(*) begin
    DBusCachedPlugin_exceptionBus_payload_code = 4'bxxxx; // @[UInt.scala 467:20]
    if(when_DBusCachedPlugin_l446) begin
      if(dataCache_1_io_cpu_writeBack_accessError) begin
        DBusCachedPlugin_exceptionBus_payload_code = {1'd0, _zz_DBusCachedPlugin_exceptionBus_payload_code}; // @[DBusCachedPlugin.scala 449:29]
      end
      if(dataCache_1_io_cpu_writeBack_mmuException) begin
        DBusCachedPlugin_exceptionBus_payload_code = (writeBack_MEMORY_WR ? 4'b1111 : 4'b1101); // @[DBusCachedPlugin.scala 453:29]
      end
      if(dataCache_1_io_cpu_writeBack_unalignedAccess) begin
        DBusCachedPlugin_exceptionBus_payload_code = {1'd0, _zz_DBusCachedPlugin_exceptionBus_payload_code_1}; // @[DBusCachedPlugin.scala 457:29]
      end
    end
  end

  assign when_DBusCachedPlugin_l446 = (writeBack_arbitration_isValid && writeBack_MEMORY_ENABLE); // @[BaseType.scala 305:24]
  assign when_DBusCachedPlugin_l466 = (dataCache_1_io_cpu_writeBack_isValid && dataCache_1_io_cpu_writeBack_haltIt); // @[BaseType.scala 305:24]
  assign writeBack_DBusCachedPlugin_rspSplits_0 = dataCache_1_io_cpu_writeBack_data[7 : 0]; // @[BaseType.scala 299:24]
  assign writeBack_DBusCachedPlugin_rspSplits_1 = dataCache_1_io_cpu_writeBack_data[15 : 8]; // @[BaseType.scala 299:24]
  assign writeBack_DBusCachedPlugin_rspSplits_2 = dataCache_1_io_cpu_writeBack_data[23 : 16]; // @[BaseType.scala 299:24]
  assign writeBack_DBusCachedPlugin_rspSplits_3 = dataCache_1_io_cpu_writeBack_data[31 : 24]; // @[BaseType.scala 299:24]
  always @(*) begin
    writeBack_DBusCachedPlugin_rspShifted[7 : 0] = _zz_writeBack_DBusCachedPlugin_rspShifted; // @[DBusCachedPlugin.scala 478:33]
    writeBack_DBusCachedPlugin_rspShifted[15 : 8] = _zz_writeBack_DBusCachedPlugin_rspShifted_2; // @[DBusCachedPlugin.scala 478:33]
    writeBack_DBusCachedPlugin_rspShifted[23 : 16] = writeBack_DBusCachedPlugin_rspSplits_2; // @[DBusCachedPlugin.scala 478:33]
    writeBack_DBusCachedPlugin_rspShifted[31 : 24] = writeBack_DBusCachedPlugin_rspSplits_3; // @[DBusCachedPlugin.scala 478:33]
  end

  assign writeBack_DBusCachedPlugin_rspRf = writeBack_DBusCachedPlugin_rspShifted[31 : 0]; // @[Misc.scala 552:9]
  assign switch_Misc_l226 = writeBack_INSTRUCTION[13 : 12]; // @[BaseType.scala 299:24]
  assign _zz_writeBack_DBusCachedPlugin_rspFormated = (writeBack_DBusCachedPlugin_rspRf[7] && (! writeBack_INSTRUCTION[14])); // @[BaseType.scala 305:24]
  always @(*) begin
    _zz_writeBack_DBusCachedPlugin_rspFormated_1[31] = _zz_writeBack_DBusCachedPlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_1[30] = _zz_writeBack_DBusCachedPlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_1[29] = _zz_writeBack_DBusCachedPlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_1[28] = _zz_writeBack_DBusCachedPlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_1[27] = _zz_writeBack_DBusCachedPlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_1[26] = _zz_writeBack_DBusCachedPlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_1[25] = _zz_writeBack_DBusCachedPlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_1[24] = _zz_writeBack_DBusCachedPlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_1[23] = _zz_writeBack_DBusCachedPlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_1[22] = _zz_writeBack_DBusCachedPlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_1[21] = _zz_writeBack_DBusCachedPlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_1[20] = _zz_writeBack_DBusCachedPlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_1[19] = _zz_writeBack_DBusCachedPlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_1[18] = _zz_writeBack_DBusCachedPlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_1[17] = _zz_writeBack_DBusCachedPlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_1[16] = _zz_writeBack_DBusCachedPlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_1[15] = _zz_writeBack_DBusCachedPlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_1[14] = _zz_writeBack_DBusCachedPlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_1[13] = _zz_writeBack_DBusCachedPlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_1[12] = _zz_writeBack_DBusCachedPlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_1[11] = _zz_writeBack_DBusCachedPlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_1[10] = _zz_writeBack_DBusCachedPlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_1[9] = _zz_writeBack_DBusCachedPlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_1[8] = _zz_writeBack_DBusCachedPlugin_rspFormated; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_1[7 : 0] = writeBack_DBusCachedPlugin_rspRf[7 : 0]; // @[Literal.scala 99:91]
  end

  assign _zz_writeBack_DBusCachedPlugin_rspFormated_2 = (writeBack_DBusCachedPlugin_rspRf[15] && (! writeBack_INSTRUCTION[14])); // @[BaseType.scala 305:24]
  always @(*) begin
    _zz_writeBack_DBusCachedPlugin_rspFormated_3[31] = _zz_writeBack_DBusCachedPlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_3[30] = _zz_writeBack_DBusCachedPlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_3[29] = _zz_writeBack_DBusCachedPlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_3[28] = _zz_writeBack_DBusCachedPlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_3[27] = _zz_writeBack_DBusCachedPlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_3[26] = _zz_writeBack_DBusCachedPlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_3[25] = _zz_writeBack_DBusCachedPlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_3[24] = _zz_writeBack_DBusCachedPlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_3[23] = _zz_writeBack_DBusCachedPlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_3[22] = _zz_writeBack_DBusCachedPlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_3[21] = _zz_writeBack_DBusCachedPlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_3[20] = _zz_writeBack_DBusCachedPlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_3[19] = _zz_writeBack_DBusCachedPlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_3[18] = _zz_writeBack_DBusCachedPlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_3[17] = _zz_writeBack_DBusCachedPlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_3[16] = _zz_writeBack_DBusCachedPlugin_rspFormated_2; // @[Literal.scala 87:17]
    _zz_writeBack_DBusCachedPlugin_rspFormated_3[15 : 0] = writeBack_DBusCachedPlugin_rspRf[15 : 0]; // @[Literal.scala 99:91]
  end

  always @(*) begin
    case(switch_Misc_l226)
      2'b00 : begin
        writeBack_DBusCachedPlugin_rspFormated = _zz_writeBack_DBusCachedPlugin_rspFormated_1; // @[Misc.scala 239:22]
      end
      2'b01 : begin
        writeBack_DBusCachedPlugin_rspFormated = _zz_writeBack_DBusCachedPlugin_rspFormated_3; // @[Misc.scala 239:22]
      end
      default : begin
        writeBack_DBusCachedPlugin_rspFormated = writeBack_DBusCachedPlugin_rspRf; // @[Misc.scala 235:22]
      end
    endcase
  end

  assign when_DBusCachedPlugin_l492 = (writeBack_arbitration_isValid && writeBack_MEMORY_ENABLE); // @[BaseType.scala 305:24]
  assign when_PmpPlugin_l138 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  always @(*) begin
    execute_PmpPlugin_fsmComplete = 1'b0; // @[PmpPlugin.scala 139:25]
    if(when_StateMachine_l253) begin
      execute_PmpPlugin_fsmComplete = 1'b1; // @[PmpPlugin.scala 194:25]
    end
  end

  assign execute_PmpPlugin_csrAddress = execute_INSTRUCTION[31 : 20]; // @[BaseType.scala 299:24]
  assign execute_PmpPlugin_pmpNcfg = execute_PmpPlugin_csrAddress[3 : 0]; // @[BaseType.scala 318:22]
  assign execute_PmpPlugin_pmpcfgN = execute_PmpPlugin_pmpNcfg[1 : 0]; // @[BaseType.scala 299:24]
  assign execute_PmpPlugin_pmpcfgCsr = (execute_INSTRUCTION[31 : 24] == 8'h3a); // @[BaseType.scala 305:24]
  assign execute_PmpPlugin_pmpaddrCsr = (execute_INSTRUCTION[31 : 24] == 8'h3b); // @[BaseType.scala 305:24]
  assign execute_PmpPlugin_fsm_wantExit = 1'b0; // @[StateMachine.scala 151:28]
  always @(*) begin
    execute_PmpPlugin_fsm_wantStart = 1'b0; // @[StateMachine.scala 152:19]
    case(execute_PmpPlugin_fsm_stateReg)
      execute_PmpPlugin_fsm_enumDef_stateIdle : begin
      end
      execute_PmpPlugin_fsm_enumDef_stateWrite : begin
      end
      execute_PmpPlugin_fsm_enumDef_stateCfg : begin
      end
      execute_PmpPlugin_fsm_enumDef_stateAddr : begin
      end
      default : begin
        execute_PmpPlugin_fsm_wantStart = 1'b1; // @[StateMachine.scala 362:15]
      end
    endcase
  end

  assign execute_PmpPlugin_fsm_wantKill = 1'b0; // @[StateMachine.scala 153:18]
  always @(*) begin
    if(execute_PmpPlugin_pmpaddrCsr_) begin
      PmpPlugin_setter_io_addr = execute_PmpPlugin_writeData_; // @[PmpPlugin.scala 241:26]
    end else begin
      PmpPlugin_setter_io_addr = _zz_PmpPlugin_pmpaddr_port0; // @[PmpPlugin.scala 243:26]
    end
  end

  assign when_PmpPlugin_l246 = (execute_PmpPlugin_fsm_fsmEnable && (! _zz_when_PmpPlugin_l246[7])); // @[BaseType.scala 305:24]
  assign _zz_8 = ({15'd0,1'b1} <<< execute_PmpPlugin_fsm_fsmCounter); // @[BaseType.scala 299:24]
  assign _zz_9 = ({15'd0,1'b1} <<< execute_PmpPlugin_fsm_fsmCounter); // @[BaseType.scala 299:24]
  assign DBusCachedPlugin_mmuBus_rsp_physicalAddress = DBusCachedPlugin_mmuBus_cmd_0_virtualAddress; // @[PmpPlugin.scala 267:39]
  assign DBusCachedPlugin_mmuBus_rsp_isIoAccess = (DBusCachedPlugin_mmuBus_cmd_0_virtualAddress[31 : 28] == 4'b1111); // @[PmpPlugin.scala 268:34]
  assign DBusCachedPlugin_mmuBus_rsp_isPaging = 1'b0; // @[PmpPlugin.scala 269:32]
  assign DBusCachedPlugin_mmuBus_rsp_exception = 1'b0; // @[PmpPlugin.scala 270:33]
  assign DBusCachedPlugin_mmuBus_rsp_refilling = 1'b0; // @[PmpPlugin.scala 271:33]
  assign DBusCachedPlugin_mmuBus_rsp_allowExecute = 1'b0; // @[PmpPlugin.scala 272:36]
  assign DBusCachedPlugin_mmuBus_busy = 1'b0; // @[PmpPlugin.scala 273:24]
  assign _zz_PmpPlugin_dGuard_hits_0 = DBusCachedPlugin_mmuBus_cmd_0_virtualAddress[31 : 4]; // @[BaseType.scala 299:24]
  assign PmpPlugin_dGuard_hits_0 = ((((_zz_PmpPlugin_dGuard_hits_0 & _zz_PmpPlugin_dGuard_hits_0_1) == _zz_PmpPlugin_dGuard_hits_0_3) && (PmpPlugin_pmpcfg_0[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_0[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_dGuard_hits_1 = ((((_zz_PmpPlugin_dGuard_hits_0 & _zz_PmpPlugin_dGuard_hits_1) == _zz_PmpPlugin_dGuard_hits_1_2) && (PmpPlugin_pmpcfg_1[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_1[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_dGuard_hits_2 = ((((_zz_PmpPlugin_dGuard_hits_0 & _zz_PmpPlugin_dGuard_hits_2) == _zz_PmpPlugin_dGuard_hits_2_2) && (PmpPlugin_pmpcfg_2[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_2[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_dGuard_hits_3 = ((((_zz_PmpPlugin_dGuard_hits_0 & _zz_PmpPlugin_dGuard_hits_3) == _zz_PmpPlugin_dGuard_hits_3_2) && (PmpPlugin_pmpcfg_3[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_3[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_dGuard_hits_4 = ((((_zz_PmpPlugin_dGuard_hits_0 & _zz_PmpPlugin_dGuard_hits_4) == _zz_PmpPlugin_dGuard_hits_4_2) && (PmpPlugin_pmpcfg_4[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_4[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_dGuard_hits_5 = ((((_zz_PmpPlugin_dGuard_hits_0 & _zz_PmpPlugin_dGuard_hits_5) == _zz_PmpPlugin_dGuard_hits_5_2) && (PmpPlugin_pmpcfg_5[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_5[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_dGuard_hits_6 = ((((_zz_PmpPlugin_dGuard_hits_0 & _zz_PmpPlugin_dGuard_hits_6) == _zz_PmpPlugin_dGuard_hits_6_2) && (PmpPlugin_pmpcfg_6[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_6[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_dGuard_hits_7 = ((((_zz_PmpPlugin_dGuard_hits_0 & _zz_PmpPlugin_dGuard_hits_7) == _zz_PmpPlugin_dGuard_hits_7_2) && (PmpPlugin_pmpcfg_7[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_7[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_dGuard_hits_8 = ((((_zz_PmpPlugin_dGuard_hits_0 & _zz_PmpPlugin_dGuard_hits_8) == _zz_PmpPlugin_dGuard_hits_8_2) && (PmpPlugin_pmpcfg_8[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_8[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_dGuard_hits_9 = ((((_zz_PmpPlugin_dGuard_hits_0 & _zz_PmpPlugin_dGuard_hits_9) == _zz_PmpPlugin_dGuard_hits_9_2) && (PmpPlugin_pmpcfg_9[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_9[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_dGuard_hits_10 = ((((_zz_PmpPlugin_dGuard_hits_0 & _zz_PmpPlugin_dGuard_hits_10) == _zz_PmpPlugin_dGuard_hits_10_2) && (PmpPlugin_pmpcfg_10[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_10[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_dGuard_hits_11 = ((((_zz_PmpPlugin_dGuard_hits_0 & _zz_PmpPlugin_dGuard_hits_11) == _zz_PmpPlugin_dGuard_hits_11_2) && (PmpPlugin_pmpcfg_11[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_11[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_dGuard_hits_12 = ((((_zz_PmpPlugin_dGuard_hits_0 & _zz_PmpPlugin_dGuard_hits_12) == _zz_PmpPlugin_dGuard_hits_12_2) && (PmpPlugin_pmpcfg_12[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_12[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_dGuard_hits_13 = ((((_zz_PmpPlugin_dGuard_hits_0 & _zz_PmpPlugin_dGuard_hits_13) == _zz_PmpPlugin_dGuard_hits_13_2) && (PmpPlugin_pmpcfg_13[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_13[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_dGuard_hits_14 = ((((_zz_PmpPlugin_dGuard_hits_0 & _zz_PmpPlugin_dGuard_hits_14) == _zz_PmpPlugin_dGuard_hits_14_2) && (PmpPlugin_pmpcfg_14[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_14[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_dGuard_hits_15 = ((((_zz_PmpPlugin_dGuard_hits_0 & _zz_PmpPlugin_dGuard_hits_15) == _zz_PmpPlugin_dGuard_hits_15_2) && (PmpPlugin_pmpcfg_15[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_15[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign when_PmpPlugin_l277 = (! ({PmpPlugin_dGuard_hits_15,{PmpPlugin_dGuard_hits_14,{PmpPlugin_dGuard_hits_13,{PmpPlugin_dGuard_hits_12,{PmpPlugin_dGuard_hits_11,{PmpPlugin_dGuard_hits_10,{PmpPlugin_dGuard_hits_9,{PmpPlugin_dGuard_hits_8,{PmpPlugin_dGuard_hits_7,{_zz_when_PmpPlugin_l277,_zz_when_PmpPlugin_l277_1}}}}}}}}}} != 16'h0)); // @[BaseType.scala 299:24]
  always @(*) begin
    if(when_PmpPlugin_l277) begin
      DBusCachedPlugin_mmuBus_rsp_allowRead = (CsrPlugin_privilege == 2'b11); // @[PmpPlugin.scala 278:35]
    end else begin
      DBusCachedPlugin_mmuBus_rsp_allowRead = _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_17; // @[PmpPlugin.scala 281:35]
    end
  end

  always @(*) begin
    if(when_PmpPlugin_l277) begin
      DBusCachedPlugin_mmuBus_rsp_allowWrite = (CsrPlugin_privilege == 2'b11); // @[PmpPlugin.scala 279:36]
    end else begin
      DBusCachedPlugin_mmuBus_rsp_allowWrite = _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_17; // @[PmpPlugin.scala 282:36]
    end
  end

  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowRead = {PmpPlugin_dGuard_hits_15,{PmpPlugin_dGuard_hits_14,{PmpPlugin_dGuard_hits_13,{PmpPlugin_dGuard_hits_12,{PmpPlugin_dGuard_hits_11,{PmpPlugin_dGuard_hits_10,{PmpPlugin_dGuard_hits_9,{PmpPlugin_dGuard_hits_8,{PmpPlugin_dGuard_hits_7,{PmpPlugin_dGuard_hits_6,{_zz__zz_DBusCachedPlugin_mmuBus_rsp_allowRead,_zz__zz_DBusCachedPlugin_mmuBus_rsp_allowRead_1}}}}}}}}}}}; // @[BaseType.scala 318:22]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_1 = (_zz_DBusCachedPlugin_mmuBus_rsp_allowRead & (~ _zz__zz_DBusCachedPlugin_mmuBus_rsp_allowRead_1_1)); // @[Bits.scala 133:56]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_2 = _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_1[3]; // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_3 = _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_1[5]; // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_4 = _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_1[6]; // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_5 = _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_1[7]; // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_6 = _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_1[9]; // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_7 = _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_1[10]; // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_8 = _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_1[11]; // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_9 = _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_1[12]; // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_10 = _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_1[13]; // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_11 = _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_1[14]; // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_12 = _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_1[15]; // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_13 = (((((((_zz_DBusCachedPlugin_mmuBus_rsp_allowRead_1[1] || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_2) || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_3) || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_5) || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_6) || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_8) || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_10) || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_12); // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_14 = (((((((_zz_DBusCachedPlugin_mmuBus_rsp_allowRead_1[2] || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_2) || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_4) || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_5) || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_7) || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_8) || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_11) || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_12); // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_15 = (((((((_zz_DBusCachedPlugin_mmuBus_rsp_allowRead_1[4] || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_3) || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_4) || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_5) || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_9) || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_10) || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_11) || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_12); // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_16 = (((((((_zz_DBusCachedPlugin_mmuBus_rsp_allowRead_1[8] || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_6) || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_7) || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_8) || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_9) || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_10) || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_11) || _zz_DBusCachedPlugin_mmuBus_rsp_allowRead_12); // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite = {PmpPlugin_dGuard_hits_15,{PmpPlugin_dGuard_hits_14,{PmpPlugin_dGuard_hits_13,{PmpPlugin_dGuard_hits_12,{PmpPlugin_dGuard_hits_11,{PmpPlugin_dGuard_hits_10,{PmpPlugin_dGuard_hits_9,{PmpPlugin_dGuard_hits_8,{PmpPlugin_dGuard_hits_7,{PmpPlugin_dGuard_hits_6,{_zz__zz_DBusCachedPlugin_mmuBus_rsp_allowWrite,_zz__zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_1}}}}}}}}}}}; // @[BaseType.scala 318:22]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_1 = (_zz_DBusCachedPlugin_mmuBus_rsp_allowWrite & (~ _zz__zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_1_1)); // @[Bits.scala 133:56]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_2 = _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_1[3]; // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_3 = _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_1[5]; // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_4 = _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_1[6]; // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_5 = _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_1[7]; // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_6 = _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_1[9]; // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_7 = _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_1[10]; // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_8 = _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_1[11]; // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_9 = _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_1[12]; // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_10 = _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_1[13]; // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_11 = _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_1[14]; // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_12 = _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_1[15]; // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_13 = (((((((_zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_1[1] || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_2) || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_3) || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_5) || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_6) || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_8) || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_10) || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_12); // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_14 = (((((((_zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_1[2] || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_2) || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_4) || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_5) || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_7) || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_8) || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_11) || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_12); // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_15 = (((((((_zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_1[4] || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_3) || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_4) || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_5) || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_9) || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_10) || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_11) || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_12); // @[BaseType.scala 305:24]
  assign _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_16 = (((((((_zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_1[8] || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_6) || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_7) || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_8) || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_9) || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_10) || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_11) || _zz_DBusCachedPlugin_mmuBus_rsp_allowWrite_12); // @[BaseType.scala 305:24]
  assign IBusCachedPlugin_mmuBus_rsp_physicalAddress = IBusCachedPlugin_mmuBus_cmd_0_virtualAddress; // @[PmpPlugin.scala 288:39]
  assign IBusCachedPlugin_mmuBus_rsp_isIoAccess = (IBusCachedPlugin_mmuBus_cmd_0_virtualAddress[31 : 28] == 4'b1111); // @[PmpPlugin.scala 289:34]
  assign IBusCachedPlugin_mmuBus_rsp_isPaging = 1'b0; // @[PmpPlugin.scala 290:32]
  assign IBusCachedPlugin_mmuBus_rsp_exception = 1'b0; // @[PmpPlugin.scala 291:33]
  assign IBusCachedPlugin_mmuBus_rsp_refilling = 1'b0; // @[PmpPlugin.scala 292:33]
  assign IBusCachedPlugin_mmuBus_rsp_allowRead = 1'b0; // @[PmpPlugin.scala 293:33]
  assign IBusCachedPlugin_mmuBus_rsp_allowWrite = 1'b0; // @[PmpPlugin.scala 294:34]
  assign IBusCachedPlugin_mmuBus_busy = 1'b0; // @[PmpPlugin.scala 295:24]
  assign _zz_PmpPlugin_iGuard_hits_0 = IBusCachedPlugin_mmuBus_cmd_0_virtualAddress[31 : 4]; // @[BaseType.scala 299:24]
  assign PmpPlugin_iGuard_hits_0 = ((((_zz_PmpPlugin_iGuard_hits_0 & _zz_PmpPlugin_iGuard_hits_0_1) == _zz_PmpPlugin_iGuard_hits_0_3) && (PmpPlugin_pmpcfg_0[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_0[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_iGuard_hits_1 = ((((_zz_PmpPlugin_iGuard_hits_0 & _zz_PmpPlugin_iGuard_hits_1) == _zz_PmpPlugin_iGuard_hits_1_2) && (PmpPlugin_pmpcfg_1[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_1[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_iGuard_hits_2 = ((((_zz_PmpPlugin_iGuard_hits_0 & _zz_PmpPlugin_iGuard_hits_2) == _zz_PmpPlugin_iGuard_hits_2_2) && (PmpPlugin_pmpcfg_2[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_2[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_iGuard_hits_3 = ((((_zz_PmpPlugin_iGuard_hits_0 & _zz_PmpPlugin_iGuard_hits_3) == _zz_PmpPlugin_iGuard_hits_3_2) && (PmpPlugin_pmpcfg_3[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_3[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_iGuard_hits_4 = ((((_zz_PmpPlugin_iGuard_hits_0 & _zz_PmpPlugin_iGuard_hits_4) == _zz_PmpPlugin_iGuard_hits_4_2) && (PmpPlugin_pmpcfg_4[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_4[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_iGuard_hits_5 = ((((_zz_PmpPlugin_iGuard_hits_0 & _zz_PmpPlugin_iGuard_hits_5) == _zz_PmpPlugin_iGuard_hits_5_2) && (PmpPlugin_pmpcfg_5[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_5[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_iGuard_hits_6 = ((((_zz_PmpPlugin_iGuard_hits_0 & _zz_PmpPlugin_iGuard_hits_6) == _zz_PmpPlugin_iGuard_hits_6_2) && (PmpPlugin_pmpcfg_6[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_6[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_iGuard_hits_7 = ((((_zz_PmpPlugin_iGuard_hits_0 & _zz_PmpPlugin_iGuard_hits_7) == _zz_PmpPlugin_iGuard_hits_7_2) && (PmpPlugin_pmpcfg_7[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_7[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_iGuard_hits_8 = ((((_zz_PmpPlugin_iGuard_hits_0 & _zz_PmpPlugin_iGuard_hits_8) == _zz_PmpPlugin_iGuard_hits_8_2) && (PmpPlugin_pmpcfg_8[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_8[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_iGuard_hits_9 = ((((_zz_PmpPlugin_iGuard_hits_0 & _zz_PmpPlugin_iGuard_hits_9) == _zz_PmpPlugin_iGuard_hits_9_2) && (PmpPlugin_pmpcfg_9[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_9[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_iGuard_hits_10 = ((((_zz_PmpPlugin_iGuard_hits_0 & _zz_PmpPlugin_iGuard_hits_10) == _zz_PmpPlugin_iGuard_hits_10_2) && (PmpPlugin_pmpcfg_10[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_10[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_iGuard_hits_11 = ((((_zz_PmpPlugin_iGuard_hits_0 & _zz_PmpPlugin_iGuard_hits_11) == _zz_PmpPlugin_iGuard_hits_11_2) && (PmpPlugin_pmpcfg_11[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_11[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_iGuard_hits_12 = ((((_zz_PmpPlugin_iGuard_hits_0 & _zz_PmpPlugin_iGuard_hits_12) == _zz_PmpPlugin_iGuard_hits_12_2) && (PmpPlugin_pmpcfg_12[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_12[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_iGuard_hits_13 = ((((_zz_PmpPlugin_iGuard_hits_0 & _zz_PmpPlugin_iGuard_hits_13) == _zz_PmpPlugin_iGuard_hits_13_2) && (PmpPlugin_pmpcfg_13[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_13[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_iGuard_hits_14 = ((((_zz_PmpPlugin_iGuard_hits_0 & _zz_PmpPlugin_iGuard_hits_14) == _zz_PmpPlugin_iGuard_hits_14_2) && (PmpPlugin_pmpcfg_14[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_14[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign PmpPlugin_iGuard_hits_15 = ((((_zz_PmpPlugin_iGuard_hits_0 & _zz_PmpPlugin_iGuard_hits_15) == _zz_PmpPlugin_iGuard_hits_15_2) && (PmpPlugin_pmpcfg_15[7] || (! (CsrPlugin_privilege == 2'b11)))) && (PmpPlugin_pmpcfg_15[4 : 3] == 2'b11)); // @[BaseType.scala 305:24]
  assign when_PmpPlugin_l299 = (! ({PmpPlugin_iGuard_hits_15,{PmpPlugin_iGuard_hits_14,{PmpPlugin_iGuard_hits_13,{PmpPlugin_iGuard_hits_12,{PmpPlugin_iGuard_hits_11,{PmpPlugin_iGuard_hits_10,{PmpPlugin_iGuard_hits_9,{PmpPlugin_iGuard_hits_8,{PmpPlugin_iGuard_hits_7,{_zz_when_PmpPlugin_l299,_zz_when_PmpPlugin_l299_1}}}}}}}}}} != 16'h0)); // @[BaseType.scala 299:24]
  always @(*) begin
    if(when_PmpPlugin_l299) begin
      IBusCachedPlugin_mmuBus_rsp_allowExecute = (CsrPlugin_privilege == 2'b11); // @[PmpPlugin.scala 300:38]
    end else begin
      IBusCachedPlugin_mmuBus_rsp_allowExecute = _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_17; // @[PmpPlugin.scala 302:38]
    end
  end

  assign _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute = {PmpPlugin_iGuard_hits_15,{PmpPlugin_iGuard_hits_14,{PmpPlugin_iGuard_hits_13,{PmpPlugin_iGuard_hits_12,{PmpPlugin_iGuard_hits_11,{PmpPlugin_iGuard_hits_10,{PmpPlugin_iGuard_hits_9,{PmpPlugin_iGuard_hits_8,{PmpPlugin_iGuard_hits_7,{PmpPlugin_iGuard_hits_6,{_zz__zz_IBusCachedPlugin_mmuBus_rsp_allowExecute,_zz__zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_1}}}}}}}}}}}; // @[BaseType.scala 318:22]
  assign _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_1 = (_zz_IBusCachedPlugin_mmuBus_rsp_allowExecute & (~ _zz__zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_1_1)); // @[Bits.scala 133:56]
  assign _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_2 = _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_1[3]; // @[BaseType.scala 305:24]
  assign _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_3 = _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_1[5]; // @[BaseType.scala 305:24]
  assign _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_4 = _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_1[6]; // @[BaseType.scala 305:24]
  assign _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_5 = _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_1[7]; // @[BaseType.scala 305:24]
  assign _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_6 = _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_1[9]; // @[BaseType.scala 305:24]
  assign _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_7 = _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_1[10]; // @[BaseType.scala 305:24]
  assign _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_8 = _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_1[11]; // @[BaseType.scala 305:24]
  assign _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_9 = _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_1[12]; // @[BaseType.scala 305:24]
  assign _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_10 = _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_1[13]; // @[BaseType.scala 305:24]
  assign _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_11 = _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_1[14]; // @[BaseType.scala 305:24]
  assign _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_12 = _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_1[15]; // @[BaseType.scala 305:24]
  assign _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_13 = (((((((_zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_1[1] || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_2) || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_3) || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_5) || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_6) || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_8) || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_10) || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_12); // @[BaseType.scala 305:24]
  assign _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_14 = (((((((_zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_1[2] || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_2) || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_4) || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_5) || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_7) || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_8) || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_11) || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_12); // @[BaseType.scala 305:24]
  assign _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_15 = (((((((_zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_1[4] || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_3) || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_4) || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_5) || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_9) || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_10) || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_11) || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_12); // @[BaseType.scala 305:24]
  assign _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_16 = (((((((_zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_1[8] || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_6) || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_7) || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_8) || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_9) || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_10) || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_11) || _zz_IBusCachedPlugin_mmuBus_rsp_allowExecute_12); // @[BaseType.scala 305:24]
  assign _zz_decode_IS_CSR_1 = ((decode_INSTRUCTION & 32'h00004050) == 32'h00004050); // @[BaseType.scala 305:24]
  assign _zz_decode_IS_CSR_2 = ((decode_INSTRUCTION & 32'h00000018) == 32'h0); // @[BaseType.scala 305:24]
  assign _zz_decode_IS_CSR_3 = ((decode_INSTRUCTION & 32'h00000004) == 32'h00000004); // @[BaseType.scala 305:24]
  assign _zz_decode_IS_CSR_4 = ((decode_INSTRUCTION & 32'h00000048) == 32'h00000048); // @[BaseType.scala 305:24]
  assign _zz_decode_IS_CSR_5 = ((decode_INSTRUCTION & 32'h00003000) == 32'h00002000); // @[BaseType.scala 305:24]
  assign _zz_decode_IS_CSR_6 = ((decode_INSTRUCTION & 32'h00007000) == 32'h00001000); // @[BaseType.scala 305:24]
  assign _zz_decode_IS_CSR_7 = ((decode_INSTRUCTION & 32'h00005000) == 32'h00004000); // @[BaseType.scala 305:24]
  assign _zz_decode_IS_CSR = {(|{_zz_decode_IS_CSR_4,(_zz__zz_decode_IS_CSR == _zz__zz_decode_IS_CSR_1)}),{(|(_zz__zz_decode_IS_CSR_2 == _zz__zz_decode_IS_CSR_3)),{(|_zz__zz_decode_IS_CSR_4),{(|_zz__zz_decode_IS_CSR_5),{_zz__zz_decode_IS_CSR_8,{_zz__zz_decode_IS_CSR_10,_zz__zz_decode_IS_CSR_13}}}}}}; // @[DecoderSimplePlugin.scala 161:19]
  assign _zz_decode_SRC1_CTRL_2 = _zz_decode_IS_CSR[2 : 1]; // @[Enum.scala 186:17]
  assign _zz_decode_SRC1_CTRL_1 = _zz_decode_SRC1_CTRL_2; // @[Enum.scala 188:10]
  assign _zz_decode_ALU_CTRL_2 = _zz_decode_IS_CSR[7 : 6]; // @[Enum.scala 186:17]
  assign _zz_decode_ALU_CTRL_1 = _zz_decode_ALU_CTRL_2; // @[Enum.scala 188:10]
  assign _zz_decode_SRC2_CTRL_2 = _zz_decode_IS_CSR[9 : 8]; // @[Enum.scala 186:17]
  assign _zz_decode_SRC2_CTRL_1 = _zz_decode_SRC2_CTRL_2; // @[Enum.scala 188:10]
  assign _zz_decode_ALU_BITWISE_CTRL_2 = _zz_decode_IS_CSR[19 : 18]; // @[Enum.scala 186:17]
  assign _zz_decode_ALU_BITWISE_CTRL_1 = _zz_decode_ALU_BITWISE_CTRL_2; // @[Enum.scala 188:10]
  assign _zz_decode_SHIFT_CTRL_2 = _zz_decode_IS_CSR[22 : 21]; // @[Enum.scala 186:17]
  assign _zz_decode_SHIFT_CTRL_1 = _zz_decode_SHIFT_CTRL_2; // @[Enum.scala 188:10]
  assign _zz_decode_ENV_CTRL_2 = _zz_decode_IS_CSR[29 : 28]; // @[Enum.scala 186:17]
  assign _zz_decode_ENV_CTRL_1 = _zz_decode_ENV_CTRL_2; // @[Enum.scala 188:10]
  assign _zz_decode_BRANCH_CTRL_2 = _zz_decode_IS_CSR[32 : 31]; // @[Enum.scala 186:17]
  assign _zz_decode_BRANCH_CTRL = _zz_decode_BRANCH_CTRL_2; // @[Enum.scala 188:10]
  assign decodeExceptionPort_valid = (decode_arbitration_isValid && (! decode_LEGAL_INSTRUCTION)); // @[DecoderSimplePlugin.scala 187:33]
  assign decodeExceptionPort_payload_code = 4'b0010; // @[DecoderSimplePlugin.scala 188:32]
  assign decodeExceptionPort_payload_badAddr = decode_INSTRUCTION; // @[DecoderSimplePlugin.scala 189:35]
  assign when_RegFilePlugin_l63 = (decode_INSTRUCTION[11 : 7] == 5'h0); // @[BaseType.scala 305:24]
  assign decode_RegFilePlugin_regFileReadAddress1 = decode_INSTRUCTION_ANTICIPATED[19 : 15]; // @[BaseType.scala 318:22]
  assign decode_RegFilePlugin_regFileReadAddress2 = decode_INSTRUCTION_ANTICIPATED[24 : 20]; // @[BaseType.scala 318:22]
  assign decode_RegFilePlugin_rs1Data = _zz_RegFilePlugin_regFile_port0; // @[Bits.scala 133:56]
  assign decode_RegFilePlugin_rs2Data = _zz_RegFilePlugin_regFile_port1; // @[Bits.scala 133:56]
  always @(*) begin
    lastStageRegFileWrite_valid = (_zz_lastStageRegFileWrite_valid && writeBack_arbitration_isFiring); // @[RegFilePlugin.scala 102:26]
    if(_zz_10) begin
      lastStageRegFileWrite_valid = 1'b1; // @[RegFilePlugin.scala 114:28]
    end
  end

  always @(*) begin
    lastStageRegFileWrite_payload_address = _zz_lastStageRegFileWrite_payload_address[11 : 7]; // @[RegFilePlugin.scala 103:28]
    if(_zz_10) begin
      lastStageRegFileWrite_payload_address = 5'h0; // @[RegFilePlugin.scala 116:32]
    end
  end

  always @(*) begin
    lastStageRegFileWrite_payload_data = _zz_decode_RS2_2; // @[RegFilePlugin.scala 104:25]
    if(_zz_10) begin
      lastStageRegFileWrite_payload_data = 32'h0; // @[RegFilePlugin.scala 117:29]
    end
  end

  always @(*) begin
    case(execute_ALU_BITWISE_CTRL)
      AluBitwiseCtrlEnum_AND_1 : begin
        execute_IntAluPlugin_bitwise = (execute_SRC1 & execute_SRC2); // @[Misc.scala 239:22]
      end
      AluBitwiseCtrlEnum_OR_1 : begin
        execute_IntAluPlugin_bitwise = (execute_SRC1 | execute_SRC2); // @[Misc.scala 239:22]
      end
      default : begin
        execute_IntAluPlugin_bitwise = (execute_SRC1 ^ execute_SRC2); // @[Misc.scala 239:22]
      end
    endcase
  end

  always @(*) begin
    case(execute_ALU_CTRL)
      AluCtrlEnum_BITWISE : begin
        _zz_execute_REGFILE_WRITE_DATA = execute_IntAluPlugin_bitwise; // @[Misc.scala 239:22]
      end
      AluCtrlEnum_SLT_SLTU : begin
        _zz_execute_REGFILE_WRITE_DATA = {31'd0, _zz__zz_execute_REGFILE_WRITE_DATA}; // @[Misc.scala 239:22]
      end
      default : begin
        _zz_execute_REGFILE_WRITE_DATA = execute_SRC_ADD_SUB; // @[Misc.scala 239:22]
      end
    endcase
  end

  always @(*) begin
    case(execute_SRC1_CTRL)
      Src1CtrlEnum_RS : begin
        _zz_execute_SRC1 = execute_RS1; // @[Misc.scala 239:22]
      end
      Src1CtrlEnum_PC_INCREMENT : begin
        _zz_execute_SRC1 = {29'd0, _zz__zz_execute_SRC1}; // @[Misc.scala 239:22]
      end
      Src1CtrlEnum_IMU : begin
        _zz_execute_SRC1 = {execute_INSTRUCTION[31 : 12],12'h0}; // @[Misc.scala 239:22]
      end
      default : begin
        _zz_execute_SRC1 = {27'd0, _zz__zz_execute_SRC1_1}; // @[Misc.scala 239:22]
      end
    endcase
  end

  assign _zz_execute_SRC2 = execute_INSTRUCTION[31]; // @[BaseType.scala 305:24]
  always @(*) begin
    _zz_execute_SRC2_1[19] = _zz_execute_SRC2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_1[18] = _zz_execute_SRC2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_1[17] = _zz_execute_SRC2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_1[16] = _zz_execute_SRC2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_1[15] = _zz_execute_SRC2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_1[14] = _zz_execute_SRC2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_1[13] = _zz_execute_SRC2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_1[12] = _zz_execute_SRC2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_1[11] = _zz_execute_SRC2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_1[10] = _zz_execute_SRC2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_1[9] = _zz_execute_SRC2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_1[8] = _zz_execute_SRC2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_1[7] = _zz_execute_SRC2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_1[6] = _zz_execute_SRC2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_1[5] = _zz_execute_SRC2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_1[4] = _zz_execute_SRC2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_1[3] = _zz_execute_SRC2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_1[2] = _zz_execute_SRC2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_1[1] = _zz_execute_SRC2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_1[0] = _zz_execute_SRC2; // @[Literal.scala 87:17]
  end

  assign _zz_execute_SRC2_2 = _zz__zz_execute_SRC2_2[11]; // @[BaseType.scala 305:24]
  always @(*) begin
    _zz_execute_SRC2_3[19] = _zz_execute_SRC2_2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_3[18] = _zz_execute_SRC2_2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_3[17] = _zz_execute_SRC2_2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_3[16] = _zz_execute_SRC2_2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_3[15] = _zz_execute_SRC2_2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_3[14] = _zz_execute_SRC2_2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_3[13] = _zz_execute_SRC2_2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_3[12] = _zz_execute_SRC2_2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_3[11] = _zz_execute_SRC2_2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_3[10] = _zz_execute_SRC2_2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_3[9] = _zz_execute_SRC2_2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_3[8] = _zz_execute_SRC2_2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_3[7] = _zz_execute_SRC2_2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_3[6] = _zz_execute_SRC2_2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_3[5] = _zz_execute_SRC2_2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_3[4] = _zz_execute_SRC2_2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_3[3] = _zz_execute_SRC2_2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_3[2] = _zz_execute_SRC2_2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_3[1] = _zz_execute_SRC2_2; // @[Literal.scala 87:17]
    _zz_execute_SRC2_3[0] = _zz_execute_SRC2_2; // @[Literal.scala 87:17]
  end

  always @(*) begin
    case(execute_SRC2_CTRL)
      Src2CtrlEnum_RS : begin
        _zz_execute_SRC2_4 = execute_RS2; // @[Misc.scala 239:22]
      end
      Src2CtrlEnum_IMI : begin
        _zz_execute_SRC2_4 = {_zz_execute_SRC2_1,execute_INSTRUCTION[31 : 20]}; // @[Misc.scala 239:22]
      end
      Src2CtrlEnum_IMS : begin
        _zz_execute_SRC2_4 = {_zz_execute_SRC2_3,{execute_INSTRUCTION[31 : 25],execute_INSTRUCTION[11 : 7]}}; // @[Misc.scala 239:22]
      end
      default : begin
        _zz_execute_SRC2_4 = _zz_execute_to_memory_PC; // @[Misc.scala 239:22]
      end
    endcase
  end

  always @(*) begin
    execute_SrcPlugin_addSub = _zz_execute_SrcPlugin_addSub; // @[BaseType.scala 318:22]
    if(execute_SRC2_FORCE_ZERO) begin
      execute_SrcPlugin_addSub = execute_SRC1; // @[SrcPlugin.scala 69:46]
    end
  end

  assign execute_SrcPlugin_less = ((execute_SRC1[31] == execute_SRC2[31]) ? execute_SrcPlugin_addSub[31] : (execute_SRC_LESS_UNSIGNED ? execute_SRC2[31] : execute_SRC1[31])); // @[Expression.scala 1420:25]
  assign execute_FullBarrelShifterPlugin_amplitude = execute_SRC2[4 : 0]; // @[BaseType.scala 318:22]
  always @(*) begin
    _zz_execute_FullBarrelShifterPlugin_reversed[0] = execute_SRC1[31]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[1] = execute_SRC1[30]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[2] = execute_SRC1[29]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[3] = execute_SRC1[28]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[4] = execute_SRC1[27]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[5] = execute_SRC1[26]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[6] = execute_SRC1[25]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[7] = execute_SRC1[24]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[8] = execute_SRC1[23]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[9] = execute_SRC1[22]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[10] = execute_SRC1[21]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[11] = execute_SRC1[20]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[12] = execute_SRC1[19]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[13] = execute_SRC1[18]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[14] = execute_SRC1[17]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[15] = execute_SRC1[16]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[16] = execute_SRC1[15]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[17] = execute_SRC1[14]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[18] = execute_SRC1[13]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[19] = execute_SRC1[12]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[20] = execute_SRC1[11]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[21] = execute_SRC1[10]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[22] = execute_SRC1[9]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[23] = execute_SRC1[8]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[24] = execute_SRC1[7]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[25] = execute_SRC1[6]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[26] = execute_SRC1[5]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[27] = execute_SRC1[4]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[28] = execute_SRC1[3]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[29] = execute_SRC1[2]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[30] = execute_SRC1[1]; // @[Utils.scala 432:14]
    _zz_execute_FullBarrelShifterPlugin_reversed[31] = execute_SRC1[0]; // @[Utils.scala 432:14]
  end

  assign execute_FullBarrelShifterPlugin_reversed = ((execute_SHIFT_CTRL == ShiftCtrlEnum_SLL_1) ? _zz_execute_FullBarrelShifterPlugin_reversed : execute_SRC1); // @[Expression.scala 1420:25]
  always @(*) begin
    _zz_decode_RS2_3[0] = memory_SHIFT_RIGHT[31]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[1] = memory_SHIFT_RIGHT[30]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[2] = memory_SHIFT_RIGHT[29]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[3] = memory_SHIFT_RIGHT[28]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[4] = memory_SHIFT_RIGHT[27]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[5] = memory_SHIFT_RIGHT[26]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[6] = memory_SHIFT_RIGHT[25]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[7] = memory_SHIFT_RIGHT[24]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[8] = memory_SHIFT_RIGHT[23]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[9] = memory_SHIFT_RIGHT[22]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[10] = memory_SHIFT_RIGHT[21]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[11] = memory_SHIFT_RIGHT[20]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[12] = memory_SHIFT_RIGHT[19]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[13] = memory_SHIFT_RIGHT[18]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[14] = memory_SHIFT_RIGHT[17]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[15] = memory_SHIFT_RIGHT[16]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[16] = memory_SHIFT_RIGHT[15]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[17] = memory_SHIFT_RIGHT[14]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[18] = memory_SHIFT_RIGHT[13]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[19] = memory_SHIFT_RIGHT[12]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[20] = memory_SHIFT_RIGHT[11]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[21] = memory_SHIFT_RIGHT[10]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[22] = memory_SHIFT_RIGHT[9]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[23] = memory_SHIFT_RIGHT[8]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[24] = memory_SHIFT_RIGHT[7]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[25] = memory_SHIFT_RIGHT[6]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[26] = memory_SHIFT_RIGHT[5]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[27] = memory_SHIFT_RIGHT[4]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[28] = memory_SHIFT_RIGHT[3]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[29] = memory_SHIFT_RIGHT[2]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[30] = memory_SHIFT_RIGHT[1]; // @[Utils.scala 432:14]
    _zz_decode_RS2_3[31] = memory_SHIFT_RIGHT[0]; // @[Utils.scala 432:14]
  end

  always @(*) begin
    HazardSimplePlugin_src0Hazard = 1'b0; // @[HazardSimplePlugin.scala 36:24]
    if(when_HazardSimplePlugin_l57) begin
      if(when_HazardSimplePlugin_l58) begin
        if(when_HazardSimplePlugin_l48) begin
          HazardSimplePlugin_src0Hazard = 1'b1; // @[HazardSimplePlugin.scala 60:26]
        end
      end
    end
    if(when_HazardSimplePlugin_l57_1) begin
      if(when_HazardSimplePlugin_l58_1) begin
        if(when_HazardSimplePlugin_l48_1) begin
          HazardSimplePlugin_src0Hazard = 1'b1; // @[HazardSimplePlugin.scala 60:26]
        end
      end
    end
    if(when_HazardSimplePlugin_l57_2) begin
      if(when_HazardSimplePlugin_l58_2) begin
        if(when_HazardSimplePlugin_l48_2) begin
          HazardSimplePlugin_src0Hazard = 1'b1; // @[HazardSimplePlugin.scala 60:26]
        end
      end
    end
    if(when_HazardSimplePlugin_l105) begin
      HazardSimplePlugin_src0Hazard = 1'b0; // @[HazardSimplePlugin.scala 106:22]
    end
  end

  always @(*) begin
    HazardSimplePlugin_src1Hazard = 1'b0; // @[HazardSimplePlugin.scala 37:24]
    if(when_HazardSimplePlugin_l57) begin
      if(when_HazardSimplePlugin_l58) begin
        if(when_HazardSimplePlugin_l51) begin
          HazardSimplePlugin_src1Hazard = 1'b1; // @[HazardSimplePlugin.scala 63:26]
        end
      end
    end
    if(when_HazardSimplePlugin_l57_1) begin
      if(when_HazardSimplePlugin_l58_1) begin
        if(when_HazardSimplePlugin_l51_1) begin
          HazardSimplePlugin_src1Hazard = 1'b1; // @[HazardSimplePlugin.scala 63:26]
        end
      end
    end
    if(when_HazardSimplePlugin_l57_2) begin
      if(when_HazardSimplePlugin_l58_2) begin
        if(when_HazardSimplePlugin_l51_2) begin
          HazardSimplePlugin_src1Hazard = 1'b1; // @[HazardSimplePlugin.scala 63:26]
        end
      end
    end
    if(when_HazardSimplePlugin_l108) begin
      HazardSimplePlugin_src1Hazard = 1'b0; // @[HazardSimplePlugin.scala 109:22]
    end
  end

  assign HazardSimplePlugin_writeBackWrites_valid = (_zz_lastStageRegFileWrite_valid && writeBack_arbitration_isFiring); // @[HazardSimplePlugin.scala 74:29]
  assign HazardSimplePlugin_writeBackWrites_payload_address = _zz_lastStageRegFileWrite_payload_address[11 : 7]; // @[HazardSimplePlugin.scala 75:31]
  assign HazardSimplePlugin_writeBackWrites_payload_data = _zz_decode_RS2_2; // @[HazardSimplePlugin.scala 76:28]
  assign HazardSimplePlugin_addr0Match = (HazardSimplePlugin_writeBackBuffer_payload_address == decode_INSTRUCTION[19 : 15]); // @[BaseType.scala 305:24]
  assign HazardSimplePlugin_addr1Match = (HazardSimplePlugin_writeBackBuffer_payload_address == decode_INSTRUCTION[24 : 20]); // @[BaseType.scala 305:24]
  assign when_HazardSimplePlugin_l47 = 1'b1; // @[HazardSimplePlugin.scala 42:105]
  assign when_HazardSimplePlugin_l48 = (writeBack_INSTRUCTION[11 : 7] == decode_INSTRUCTION[19 : 15]); // @[BaseType.scala 305:24]
  assign when_HazardSimplePlugin_l51 = (writeBack_INSTRUCTION[11 : 7] == decode_INSTRUCTION[24 : 20]); // @[BaseType.scala 305:24]
  assign when_HazardSimplePlugin_l45 = (writeBack_arbitration_isValid && writeBack_REGFILE_WRITE_VALID); // @[BaseType.scala 305:24]
  assign when_HazardSimplePlugin_l57 = (writeBack_arbitration_isValid && writeBack_REGFILE_WRITE_VALID); // @[BaseType.scala 305:24]
  assign when_HazardSimplePlugin_l58 = (1'b0 || (! when_HazardSimplePlugin_l47)); // @[BaseType.scala 305:24]
  assign when_HazardSimplePlugin_l48_1 = (memory_INSTRUCTION[11 : 7] == decode_INSTRUCTION[19 : 15]); // @[BaseType.scala 305:24]
  assign when_HazardSimplePlugin_l51_1 = (memory_INSTRUCTION[11 : 7] == decode_INSTRUCTION[24 : 20]); // @[BaseType.scala 305:24]
  assign when_HazardSimplePlugin_l45_1 = (memory_arbitration_isValid && memory_REGFILE_WRITE_VALID); // @[BaseType.scala 305:24]
  assign when_HazardSimplePlugin_l57_1 = (memory_arbitration_isValid && memory_REGFILE_WRITE_VALID); // @[BaseType.scala 305:24]
  assign when_HazardSimplePlugin_l58_1 = (1'b0 || (! memory_BYPASSABLE_MEMORY_STAGE)); // @[BaseType.scala 305:24]
  assign when_HazardSimplePlugin_l48_2 = (execute_INSTRUCTION[11 : 7] == decode_INSTRUCTION[19 : 15]); // @[BaseType.scala 305:24]
  assign when_HazardSimplePlugin_l51_2 = (execute_INSTRUCTION[11 : 7] == decode_INSTRUCTION[24 : 20]); // @[BaseType.scala 305:24]
  assign when_HazardSimplePlugin_l45_2 = (execute_arbitration_isValid && execute_REGFILE_WRITE_VALID); // @[BaseType.scala 305:24]
  assign when_HazardSimplePlugin_l57_2 = (execute_arbitration_isValid && execute_REGFILE_WRITE_VALID); // @[BaseType.scala 305:24]
  assign when_HazardSimplePlugin_l58_2 = (1'b0 || (! execute_BYPASSABLE_EXECUTE_STAGE)); // @[BaseType.scala 305:24]
  assign when_HazardSimplePlugin_l105 = (! decode_RS1_USE); // @[BaseType.scala 299:24]
  assign when_HazardSimplePlugin_l108 = (! decode_RS2_USE); // @[BaseType.scala 299:24]
  assign when_HazardSimplePlugin_l113 = (decode_arbitration_isValid && (HazardSimplePlugin_src0Hazard || HazardSimplePlugin_src1Hazard)); // @[BaseType.scala 305:24]
  assign memory_MulDivIterativePlugin_frontendOk = 1'b1; // @[MulDivIterativePlugin.scala 90:50]
  always @(*) begin
    memory_MulDivIterativePlugin_mul_counter_willIncrement = 1'b0; // @[Utils.scala 536:23]
    if(when_MulDivIterativePlugin_l96) begin
      if(when_MulDivIterativePlugin_l100) begin
        memory_MulDivIterativePlugin_mul_counter_willIncrement = 1'b1; // @[Utils.scala 540:41]
      end
    end
  end

  always @(*) begin
    memory_MulDivIterativePlugin_mul_counter_willClear = 1'b0; // @[Utils.scala 537:19]
    if(when_MulDivIterativePlugin_l110) begin
      memory_MulDivIterativePlugin_mul_counter_willClear = 1'b1; // @[Utils.scala 539:33]
    end
  end

  assign memory_MulDivIterativePlugin_mul_counter_willOverflowIfInc = (memory_MulDivIterativePlugin_mul_counter_value == 6'h20); // @[BaseType.scala 305:24]
  assign memory_MulDivIterativePlugin_mul_counter_willOverflow = (memory_MulDivIterativePlugin_mul_counter_willOverflowIfInc && memory_MulDivIterativePlugin_mul_counter_willIncrement); // @[BaseType.scala 305:24]
  always @(*) begin
    if(memory_MulDivIterativePlugin_mul_counter_willOverflow) begin
      memory_MulDivIterativePlugin_mul_counter_valueNext = 6'h0; // @[Utils.scala 552:17]
    end else begin
      memory_MulDivIterativePlugin_mul_counter_valueNext = (memory_MulDivIterativePlugin_mul_counter_value + _zz_memory_MulDivIterativePlugin_mul_counter_valueNext); // @[Utils.scala 554:17]
    end
    if(memory_MulDivIterativePlugin_mul_counter_willClear) begin
      memory_MulDivIterativePlugin_mul_counter_valueNext = 6'h0; // @[Utils.scala 558:15]
    end
  end

  assign when_MulDivIterativePlugin_l96 = (memory_arbitration_isValid && memory_IS_MUL); // @[BaseType.scala 305:24]
  assign when_MulDivIterativePlugin_l97 = ((! memory_MulDivIterativePlugin_frontendOk) || (! memory_MulDivIterativePlugin_mul_counter_willOverflowIfInc)); // @[BaseType.scala 305:24]
  assign when_MulDivIterativePlugin_l100 = (memory_MulDivIterativePlugin_frontendOk && (! memory_MulDivIterativePlugin_mul_counter_willOverflowIfInc)); // @[BaseType.scala 305:24]
  assign when_MulDivIterativePlugin_l110 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  always @(*) begin
    memory_MulDivIterativePlugin_div_counter_willIncrement = 1'b0; // @[Utils.scala 536:23]
    if(when_MulDivIterativePlugin_l128) begin
      if(when_MulDivIterativePlugin_l132) begin
        memory_MulDivIterativePlugin_div_counter_willIncrement = 1'b1; // @[Utils.scala 540:41]
      end
    end
  end

  always @(*) begin
    memory_MulDivIterativePlugin_div_counter_willClear = 1'b0; // @[Utils.scala 537:19]
    if(when_MulDivIterativePlugin_l162) begin
      memory_MulDivIterativePlugin_div_counter_willClear = 1'b1; // @[Utils.scala 539:33]
    end
  end

  assign memory_MulDivIterativePlugin_div_counter_willOverflowIfInc = (memory_MulDivIterativePlugin_div_counter_value == 6'h21); // @[BaseType.scala 305:24]
  assign memory_MulDivIterativePlugin_div_counter_willOverflow = (memory_MulDivIterativePlugin_div_counter_willOverflowIfInc && memory_MulDivIterativePlugin_div_counter_willIncrement); // @[BaseType.scala 305:24]
  always @(*) begin
    if(memory_MulDivIterativePlugin_div_counter_willOverflow) begin
      memory_MulDivIterativePlugin_div_counter_valueNext = 6'h0; // @[Utils.scala 552:17]
    end else begin
      memory_MulDivIterativePlugin_div_counter_valueNext = (memory_MulDivIterativePlugin_div_counter_value + _zz_memory_MulDivIterativePlugin_div_counter_valueNext); // @[Utils.scala 554:17]
    end
    if(memory_MulDivIterativePlugin_div_counter_willClear) begin
      memory_MulDivIterativePlugin_div_counter_valueNext = 6'h0; // @[Utils.scala 558:15]
    end
  end

  assign when_MulDivIterativePlugin_l126 = (memory_MulDivIterativePlugin_div_counter_value == 6'h20); // @[BaseType.scala 305:24]
  assign when_MulDivIterativePlugin_l126_1 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_MulDivIterativePlugin_l128 = (memory_arbitration_isValid && memory_IS_DIV); // @[BaseType.scala 305:24]
  assign when_MulDivIterativePlugin_l129 = ((! memory_MulDivIterativePlugin_frontendOk) || (! memory_MulDivIterativePlugin_div_done)); // @[BaseType.scala 305:24]
  assign when_MulDivIterativePlugin_l132 = (memory_MulDivIterativePlugin_frontendOk && (! memory_MulDivIterativePlugin_div_done)); // @[BaseType.scala 305:24]
  assign _zz_memory_MulDivIterativePlugin_div_stage_0_remainderShifted = memory_MulDivIterativePlugin_rs1[31 : 0]; // @[BaseType.scala 299:24]
  assign memory_MulDivIterativePlugin_div_stage_0_remainderShifted = {memory_MulDivIterativePlugin_accumulator[31 : 0],_zz_memory_MulDivIterativePlugin_div_stage_0_remainderShifted[31]}; // @[BaseType.scala 318:22]
  assign memory_MulDivIterativePlugin_div_stage_0_remainderMinusDenominator = (memory_MulDivIterativePlugin_div_stage_0_remainderShifted - _zz_memory_MulDivIterativePlugin_div_stage_0_remainderMinusDenominator); // @[BaseType.scala 299:24]
  assign memory_MulDivIterativePlugin_div_stage_0_outRemainder = ((! memory_MulDivIterativePlugin_div_stage_0_remainderMinusDenominator[32]) ? _zz_memory_MulDivIterativePlugin_div_stage_0_outRemainder : _zz_memory_MulDivIterativePlugin_div_stage_0_outRemainder_1); // @[Expression.scala 1420:25]
  assign memory_MulDivIterativePlugin_div_stage_0_outNumerator = _zz_memory_MulDivIterativePlugin_div_stage_0_outNumerator[31:0]; // @[BaseType.scala 299:24]
  assign when_MulDivIterativePlugin_l151 = (memory_MulDivIterativePlugin_div_counter_value == 6'h20); // @[BaseType.scala 305:24]
  assign _zz_memory_MulDivIterativePlugin_div_result = (memory_INSTRUCTION[13] ? memory_MulDivIterativePlugin_accumulator[31 : 0] : memory_MulDivIterativePlugin_rs1[31 : 0]); // @[Expression.scala 1420:25]
  assign when_MulDivIterativePlugin_l162 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign _zz_memory_MulDivIterativePlugin_rs2 = (execute_RS2[31] && execute_IS_RS2_SIGNED); // @[BaseType.scala 305:24]
  assign _zz_memory_MulDivIterativePlugin_rs1 = ((execute_IS_MUL && _zz_memory_MulDivIterativePlugin_rs2) || ((execute_IS_DIV && execute_RS1[31]) && execute_IS_RS1_SIGNED)); // @[BaseType.scala 305:24]
  always @(*) begin
    _zz_memory_MulDivIterativePlugin_rs1_1[32] = (execute_IS_RS1_SIGNED && execute_RS1[31]); // @[Literal.scala 87:17]
    _zz_memory_MulDivIterativePlugin_rs1_1[31 : 0] = execute_RS1; // @[Literal.scala 99:91]
  end

  always @(*) begin
    CsrPlugin_privilege = _zz_CsrPlugin_privilege; // @[CsrPlugin.scala 680:15]
    if(CsrPlugin_forceMachineWire) begin
      CsrPlugin_privilege = 2'b11; // @[CsrPlugin.scala 682:40]
    end
  end

  assign _zz_when_CsrPlugin_l1222 = (CsrPlugin_mip_MTIP && CsrPlugin_mie_MTIE); // @[BaseType.scala 305:24]
  assign _zz_when_CsrPlugin_l1222_1 = (CsrPlugin_mip_MSIP && CsrPlugin_mie_MSIE); // @[BaseType.scala 305:24]
  assign _zz_when_CsrPlugin_l1222_2 = (CsrPlugin_mip_MEIP && CsrPlugin_mie_MEIE); // @[BaseType.scala 305:24]
  assign CsrPlugin_exceptionPortCtrl_exceptionTargetPrivilegeUncapped = 2'b11; // @[Expression.scala 2342:18]
  assign CsrPlugin_exceptionPortCtrl_exceptionTargetPrivilege = ((CsrPlugin_privilege < CsrPlugin_exceptionPortCtrl_exceptionTargetPrivilegeUncapped) ? CsrPlugin_exceptionPortCtrl_exceptionTargetPrivilegeUncapped : CsrPlugin_privilege); // @[Expression.scala 1420:25]
  assign _zz_CsrPlugin_exceptionPortCtrl_exceptionContext_code = {decodeExceptionPort_valid,IBusCachedPlugin_decodeExceptionPort_valid}; // @[BaseType.scala 318:22]
  assign _zz_CsrPlugin_exceptionPortCtrl_exceptionContext_code_1 = _zz__zz_CsrPlugin_exceptionPortCtrl_exceptionContext_code_1[0]; // @[BaseType.scala 305:24]
  always @(*) begin
    CsrPlugin_exceptionPortCtrl_exceptionValids_decode = CsrPlugin_exceptionPortCtrl_exceptionValidsRegs_decode; // @[CsrPlugin.scala 1167:25]
    if(_zz_when) begin
      CsrPlugin_exceptionPortCtrl_exceptionValids_decode = 1'b1; // @[CsrPlugin.scala 1172:38]
    end
    if(decode_arbitration_isFlushed) begin
      CsrPlugin_exceptionPortCtrl_exceptionValids_decode = 1'b0; // @[CsrPlugin.scala 1188:38]
    end
  end

  always @(*) begin
    CsrPlugin_exceptionPortCtrl_exceptionValids_execute = CsrPlugin_exceptionPortCtrl_exceptionValidsRegs_execute; // @[CsrPlugin.scala 1167:25]
    if(CsrPlugin_selfException_valid) begin
      CsrPlugin_exceptionPortCtrl_exceptionValids_execute = 1'b1; // @[CsrPlugin.scala 1172:38]
    end
    if(execute_arbitration_isFlushed) begin
      CsrPlugin_exceptionPortCtrl_exceptionValids_execute = 1'b0; // @[CsrPlugin.scala 1188:38]
    end
  end

  always @(*) begin
    CsrPlugin_exceptionPortCtrl_exceptionValids_memory = CsrPlugin_exceptionPortCtrl_exceptionValidsRegs_memory; // @[CsrPlugin.scala 1167:25]
    if(BranchPlugin_branchExceptionPort_valid) begin
      CsrPlugin_exceptionPortCtrl_exceptionValids_memory = 1'b1; // @[CsrPlugin.scala 1172:38]
    end
    if(memory_arbitration_isFlushed) begin
      CsrPlugin_exceptionPortCtrl_exceptionValids_memory = 1'b0; // @[CsrPlugin.scala 1188:38]
    end
  end

  always @(*) begin
    CsrPlugin_exceptionPortCtrl_exceptionValids_writeBack = CsrPlugin_exceptionPortCtrl_exceptionValidsRegs_writeBack; // @[CsrPlugin.scala 1167:25]
    if(DBusCachedPlugin_exceptionBus_valid) begin
      CsrPlugin_exceptionPortCtrl_exceptionValids_writeBack = 1'b1; // @[CsrPlugin.scala 1172:38]
    end
    if(writeBack_arbitration_isFlushed) begin
      CsrPlugin_exceptionPortCtrl_exceptionValids_writeBack = 1'b0; // @[CsrPlugin.scala 1188:38]
    end
  end

  assign when_CsrPlugin_l1179 = (! decode_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1179_1 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1179_2 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1179_3 = (! writeBack_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1192 = ({CsrPlugin_exceptionPortCtrl_exceptionValids_writeBack,{CsrPlugin_exceptionPortCtrl_exceptionValids_memory,{CsrPlugin_exceptionPortCtrl_exceptionValids_execute,CsrPlugin_exceptionPortCtrl_exceptionValids_decode}}} != 4'b0000); // @[BaseType.scala 305:24]
  assign CsrPlugin_exceptionPendings_0 = CsrPlugin_exceptionPortCtrl_exceptionValidsRegs_decode; // @[CsrPlugin.scala 1198:27]
  assign CsrPlugin_exceptionPendings_1 = CsrPlugin_exceptionPortCtrl_exceptionValidsRegs_execute; // @[CsrPlugin.scala 1198:27]
  assign CsrPlugin_exceptionPendings_2 = CsrPlugin_exceptionPortCtrl_exceptionValidsRegs_memory; // @[CsrPlugin.scala 1198:27]
  assign CsrPlugin_exceptionPendings_3 = CsrPlugin_exceptionPortCtrl_exceptionValidsRegs_writeBack; // @[CsrPlugin.scala 1198:27]
  assign when_CsrPlugin_l1216 = (CsrPlugin_mstatus_MIE || (CsrPlugin_privilege < 2'b11)); // @[BaseType.scala 305:24]
  assign when_CsrPlugin_l1222 = ((_zz_when_CsrPlugin_l1222 && 1'b1) && (! 1'b0)); // @[BaseType.scala 305:24]
  assign when_CsrPlugin_l1222_1 = ((_zz_when_CsrPlugin_l1222_1 && 1'b1) && (! 1'b0)); // @[BaseType.scala 305:24]
  assign when_CsrPlugin_l1222_2 = ((_zz_when_CsrPlugin_l1222_2 && 1'b1) && (! 1'b0)); // @[BaseType.scala 305:24]
  assign CsrPlugin_exception = (CsrPlugin_exceptionPortCtrl_exceptionValids_writeBack && CsrPlugin_allowException); // @[BaseType.scala 305:24]
  assign CsrPlugin_pipelineLiberator_active = ((CsrPlugin_interrupt_valid && CsrPlugin_allowInterrupts) && decode_arbitration_isValid); // @[BaseType.scala 305:24]
  assign when_CsrPlugin_l1255 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1255_1 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1255_2 = (! writeBack_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1260 = ((! CsrPlugin_pipelineLiberator_active) || decode_arbitration_removeIt); // @[BaseType.scala 305:24]
  always @(*) begin
    CsrPlugin_pipelineLiberator_done = CsrPlugin_pipelineLiberator_pcValids_2; // @[Misc.scala 552:9]
    if(when_CsrPlugin_l1266) begin
      CsrPlugin_pipelineLiberator_done = 1'b0; // @[CsrPlugin.scala 1266:53]
    end
    if(CsrPlugin_hadException) begin
      CsrPlugin_pipelineLiberator_done = 1'b0; // @[CsrPlugin.scala 1275:39]
    end
  end

  assign when_CsrPlugin_l1266 = ({CsrPlugin_exceptionPortCtrl_exceptionValidsRegs_writeBack,{CsrPlugin_exceptionPortCtrl_exceptionValidsRegs_memory,CsrPlugin_exceptionPortCtrl_exceptionValidsRegs_execute}} != 3'b000); // @[BaseType.scala 305:24]
  assign CsrPlugin_interruptJump = ((CsrPlugin_interrupt_valid && CsrPlugin_pipelineLiberator_done) && CsrPlugin_allowInterrupts); // @[CsrPlugin.scala 1271:21]
  always @(*) begin
    CsrPlugin_targetPrivilege = CsrPlugin_interrupt_targetPrivilege; // @[Misc.scala 552:9]
    if(CsrPlugin_hadException) begin
      CsrPlugin_targetPrivilege = CsrPlugin_exceptionPortCtrl_exceptionTargetPrivilege; // @[CsrPlugin.scala 1285:25]
    end
  end

  always @(*) begin
    CsrPlugin_trapCause = CsrPlugin_interrupt_code; // @[Misc.scala 552:9]
    if(CsrPlugin_hadException) begin
      CsrPlugin_trapCause = CsrPlugin_exceptionPortCtrl_exceptionContext_code; // @[CsrPlugin.scala 1291:19]
    end
  end

  assign CsrPlugin_trapCauseEbreakDebug = 1'b0; // @[CsrPlugin.scala 1289:34]
  always @(*) begin
    CsrPlugin_xtvec_mode = 2'bxx; // @[Bits.scala 231:20]
    case(CsrPlugin_targetPrivilege)
      2'b11 : begin
        CsrPlugin_xtvec_mode = CsrPlugin_mtvec_mode; // @[CsrPlugin.scala 1305:22]
      end
      default : begin
      end
    endcase
  end

  always @(*) begin
    CsrPlugin_xtvec_base = 30'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx; // @[UInt.scala 467:20]
    case(CsrPlugin_targetPrivilege)
      2'b11 : begin
        CsrPlugin_xtvec_base = CsrPlugin_mtvec_base; // @[CsrPlugin.scala 1305:22]
      end
      default : begin
      end
    endcase
  end

  assign CsrPlugin_trapEnterDebug = 1'b0; // @[CsrPlugin.scala 1308:28]
  assign when_CsrPlugin_l1310 = (CsrPlugin_hadException || CsrPlugin_interruptJump); // @[BaseType.scala 305:24]
  assign when_CsrPlugin_l1318 = (! CsrPlugin_trapEnterDebug); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1376 = (writeBack_arbitration_isValid && (writeBack_ENV_CTRL == EnvCtrlEnum_XRET)); // @[BaseType.scala 305:24]
  assign switch_CsrPlugin_l1380 = writeBack_INSTRUCTION[29 : 28]; // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1388 = (CsrPlugin_mstatus_MPP < 2'b11); // @[BaseType.scala 305:24]
  assign contextSwitching = CsrPlugin_jumpInterface_valid; // @[CsrPlugin.scala 1405:24]
  assign when_CsrPlugin_l1439 = (execute_arbitration_isValid && (execute_ENV_CTRL == EnvCtrlEnum_WFI)); // @[BaseType.scala 305:24]
  assign when_CsrPlugin_l1441 = (! execute_CsrPlugin_wfiWake); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1447 = (|{(writeBack_arbitration_isValid && (writeBack_ENV_CTRL == EnvCtrlEnum_XRET)),{(memory_arbitration_isValid && (memory_ENV_CTRL == EnvCtrlEnum_XRET)),(execute_arbitration_isValid && (execute_ENV_CTRL == EnvCtrlEnum_XRET))}}); // @[BaseType.scala 312:24]
  assign execute_CsrPlugin_blockedBySideEffects = ((|{writeBack_arbitration_isValid,memory_arbitration_isValid}) || 1'b0); // @[BaseType.scala 305:24]
  always @(*) begin
    execute_CsrPlugin_illegalAccess = 1'b1; // @[CsrPlugin.scala 1454:29]
    if(execute_CsrPlugin_csr_3857) begin
      if(execute_CSR_READ_OPCODE) begin
        execute_CsrPlugin_illegalAccess = 1'b0; // @[CsrPlugin.scala 1532:52]
      end
    end
    if(execute_CsrPlugin_csr_3858) begin
      if(execute_CSR_READ_OPCODE) begin
        execute_CsrPlugin_illegalAccess = 1'b0; // @[CsrPlugin.scala 1532:52]
      end
    end
    if(execute_CsrPlugin_csr_3859) begin
      if(execute_CSR_READ_OPCODE) begin
        execute_CsrPlugin_illegalAccess = 1'b0; // @[CsrPlugin.scala 1532:52]
      end
    end
    if(execute_CsrPlugin_csr_3860) begin
      if(execute_CSR_READ_OPCODE) begin
        execute_CsrPlugin_illegalAccess = 1'b0; // @[CsrPlugin.scala 1532:52]
      end
    end
    if(execute_CsrPlugin_csr_769) begin
      execute_CsrPlugin_illegalAccess = 1'b0; // @[CsrPlugin.scala 1529:29]
    end
    if(execute_CsrPlugin_csr_768) begin
      execute_CsrPlugin_illegalAccess = 1'b0; // @[CsrPlugin.scala 1529:29]
    end
    if(execute_CsrPlugin_csr_836) begin
      execute_CsrPlugin_illegalAccess = 1'b0; // @[CsrPlugin.scala 1529:29]
    end
    if(execute_CsrPlugin_csr_772) begin
      execute_CsrPlugin_illegalAccess = 1'b0; // @[CsrPlugin.scala 1529:29]
    end
    if(execute_CsrPlugin_csr_773) begin
      execute_CsrPlugin_illegalAccess = 1'b0; // @[CsrPlugin.scala 1529:29]
    end
    if(execute_CsrPlugin_csr_833) begin
      execute_CsrPlugin_illegalAccess = 1'b0; // @[CsrPlugin.scala 1529:29]
    end
    if(execute_CsrPlugin_csr_832) begin
      execute_CsrPlugin_illegalAccess = 1'b0; // @[CsrPlugin.scala 1529:29]
    end
    if(execute_CsrPlugin_csr_834) begin
      execute_CsrPlugin_illegalAccess = 1'b0; // @[CsrPlugin.scala 1529:29]
    end
    if(execute_CsrPlugin_csr_835) begin
      execute_CsrPlugin_illegalAccess = 1'b0; // @[CsrPlugin.scala 1529:29]
    end
    if(execute_CsrPlugin_csr_2816) begin
      execute_CsrPlugin_illegalAccess = 1'b0; // @[CsrPlugin.scala 1529:29]
    end
    if(execute_CsrPlugin_csr_2944) begin
      execute_CsrPlugin_illegalAccess = 1'b0; // @[CsrPlugin.scala 1529:29]
    end
    if(execute_CsrPlugin_csr_2818) begin
      execute_CsrPlugin_illegalAccess = 1'b0; // @[CsrPlugin.scala 1529:29]
    end
    if(execute_CsrPlugin_csr_2946) begin
      execute_CsrPlugin_illegalAccess = 1'b0; // @[CsrPlugin.scala 1529:29]
    end
    if(execute_CsrPlugin_csr_3072) begin
      if(execute_CSR_READ_OPCODE) begin
        execute_CsrPlugin_illegalAccess = 1'b0; // @[CsrPlugin.scala 1532:52]
      end
    end
    if(execute_CsrPlugin_csr_3200) begin
      if(execute_CSR_READ_OPCODE) begin
        execute_CsrPlugin_illegalAccess = 1'b0; // @[CsrPlugin.scala 1532:52]
      end
    end
    if(execute_CsrPlugin_csr_3074) begin
      if(execute_CSR_READ_OPCODE) begin
        execute_CsrPlugin_illegalAccess = 1'b0; // @[CsrPlugin.scala 1532:52]
      end
    end
    if(execute_CsrPlugin_csr_3202) begin
      if(execute_CSR_READ_OPCODE) begin
        execute_CsrPlugin_illegalAccess = 1'b0; // @[CsrPlugin.scala 1532:52]
      end
    end
    if(CsrPlugin_csrMapping_allowCsrSignal) begin
      execute_CsrPlugin_illegalAccess = 1'b0; // @[CsrPlugin.scala 1620:25]
    end
    if(when_CsrPlugin_l1625) begin
      execute_CsrPlugin_illegalAccess = 1'b1; // @[CsrPlugin.scala 1626:27]
    end
    if(when_CsrPlugin_l1631) begin
      execute_CsrPlugin_illegalAccess = 1'b0; // @[CsrPlugin.scala 1631:25]
    end
  end

  always @(*) begin
    execute_CsrPlugin_illegalInstruction = 1'b0; // @[CsrPlugin.scala 1455:34]
    if(when_CsrPlugin_l1467) begin
      if(when_CsrPlugin_l1468) begin
        execute_CsrPlugin_illegalInstruction = 1'b1; // @[CsrPlugin.scala 1469:32]
      end
    end
  end

  always @(*) begin
    CsrPlugin_selfException_valid = 1'b0; // @[CsrPlugin.scala 1457:31]
    if(when_CsrPlugin_l1460) begin
      CsrPlugin_selfException_valid = 1'b1; // @[CsrPlugin.scala 1461:33]
    end
    if(when_CsrPlugin_l1475) begin
      CsrPlugin_selfException_valid = 1'b1; // @[CsrPlugin.scala 1476:31]
    end
  end

  always @(*) begin
    CsrPlugin_selfException_payload_code = 4'bxxxx; // @[UInt.scala 467:20]
    if(when_CsrPlugin_l1460) begin
      CsrPlugin_selfException_payload_code = 4'b0010; // @[CsrPlugin.scala 1462:32]
    end
    if(when_CsrPlugin_l1475) begin
      case(CsrPlugin_privilege)
        2'b00 : begin
          CsrPlugin_selfException_payload_code = 4'b1000; // @[CsrPlugin.scala 1478:40]
        end
        default : begin
          CsrPlugin_selfException_payload_code = 4'b1011; // @[CsrPlugin.scala 1480:42]
        end
      endcase
    end
  end

  assign CsrPlugin_selfException_payload_badAddr = execute_INSTRUCTION; // @[CsrPlugin.scala 1459:33]
  assign when_CsrPlugin_l1460 = (execute_CsrPlugin_illegalAccess || execute_CsrPlugin_illegalInstruction); // @[BaseType.scala 305:24]
  assign when_CsrPlugin_l1467 = (execute_arbitration_isValid && (execute_ENV_CTRL == EnvCtrlEnum_XRET)); // @[BaseType.scala 305:24]
  assign when_CsrPlugin_l1468 = (CsrPlugin_privilege < execute_INSTRUCTION[29 : 28]); // @[BaseType.scala 305:24]
  assign when_CsrPlugin_l1475 = (execute_arbitration_isValid && (execute_ENV_CTRL == EnvCtrlEnum_ECALL)); // @[BaseType.scala 305:24]
  always @(*) begin
    execute_CsrPlugin_writeInstruction = ((execute_arbitration_isValid && execute_IS_CSR) && execute_CSR_WRITE_OPCODE); // @[BaseType.scala 305:24]
    if(when_CsrPlugin_l1625) begin
      execute_CsrPlugin_writeInstruction = 1'b0; // @[CsrPlugin.scala 1628:30]
    end
  end

  always @(*) begin
    execute_CsrPlugin_readInstruction = ((execute_arbitration_isValid && execute_IS_CSR) && execute_CSR_READ_OPCODE); // @[BaseType.scala 305:24]
    if(when_CsrPlugin_l1625) begin
      execute_CsrPlugin_readInstruction = 1'b0; // @[CsrPlugin.scala 1627:29]
    end
  end

  assign execute_CsrPlugin_writeEnable = (execute_CsrPlugin_writeInstruction && (! execute_arbitration_isStuck)); // @[BaseType.scala 305:24]
  assign execute_CsrPlugin_readEnable = (execute_CsrPlugin_readInstruction && (! execute_arbitration_isStuck)); // @[BaseType.scala 305:24]
  assign CsrPlugin_csrMapping_hazardFree = (! execute_CsrPlugin_blockedBySideEffects); // @[CsrPlugin.scala 1499:31]
  assign execute_CsrPlugin_readToWriteData = CsrPlugin_csrMapping_readDataSignal; // @[Misc.scala 552:9]
  assign switch_Misc_l226_1 = execute_INSTRUCTION[13]; // @[BaseType.scala 305:24]
  always @(*) begin
    case(switch_Misc_l226_1)
      1'b0 : begin
        _zz_CsrPlugin_csrMapping_writeDataSignal = execute_SRC1; // @[Misc.scala 239:22]
      end
      default : begin
        _zz_CsrPlugin_csrMapping_writeDataSignal = (execute_INSTRUCTION[12] ? (execute_CsrPlugin_readToWriteData & (~ execute_SRC1)) : (execute_CsrPlugin_readToWriteData | execute_SRC1)); // @[Misc.scala 239:22]
      end
    endcase
  end

  assign CsrPlugin_csrMapping_writeDataSignal = _zz_CsrPlugin_csrMapping_writeDataSignal; // @[CsrPlugin.scala 1502:19]
  assign when_CsrPlugin_l1507 = (execute_arbitration_isValid && execute_IS_CSR); // @[BaseType.scala 305:24]
  assign when_CsrPlugin_l1511 = (execute_arbitration_isValid && (execute_IS_CSR || 1'b0)); // @[BaseType.scala 305:24]
  assign execute_CsrPlugin_csrAddress = execute_INSTRUCTION[31 : 20]; // @[BaseType.scala 299:24]
  assign when_DebugPlugin_l238 = (DebugPlugin_haltIt && (! DebugPlugin_isPipBusy)); // @[BaseType.scala 305:24]
  assign DebugPlugin_allowEBreak = (DebugPlugin_debugUsed && (! DebugPlugin_disableEbreak)); // @[BaseType.scala 305:24]
  always @(*) begin
    debug_bus_cmd_ready = 1'b1; // @[DebugPlugin.scala 255:24]
    if(debug_bus_cmd_valid) begin
      case(switch_DebugPlugin_l280)
        6'h01 : begin
          if(debug_bus_cmd_payload_wr) begin
            debug_bus_cmd_ready = IBusCachedPlugin_injectionPort_ready; // @[DebugPlugin.scala 294:32]
          end
        end
        default : begin
        end
      endcase
    end
  end

  always @(*) begin
    debug_bus_rsp_data = DebugPlugin_busReadDataReg; // @[DebugPlugin.scala 256:23]
    if(when_DebugPlugin_l257) begin
      debug_bus_rsp_data[0] = DebugPlugin_resetIt; // @[DebugPlugin.scala 258:28]
      debug_bus_rsp_data[1] = DebugPlugin_haltIt; // @[DebugPlugin.scala 259:28]
      debug_bus_rsp_data[2] = DebugPlugin_isPipBusy; // @[DebugPlugin.scala 260:28]
      debug_bus_rsp_data[3] = DebugPlugin_haltedByBreak; // @[DebugPlugin.scala 261:28]
      debug_bus_rsp_data[4] = DebugPlugin_stepIt; // @[DebugPlugin.scala 262:28]
    end
  end

  assign when_DebugPlugin_l257 = (! _zz_when_DebugPlugin_l257); // @[BaseType.scala 299:24]
  always @(*) begin
    IBusCachedPlugin_injectionPort_valid = 1'b0; // @[DebugPlugin.scala 276:27]
    if(debug_bus_cmd_valid) begin
      case(switch_DebugPlugin_l280)
        6'h01 : begin
          if(debug_bus_cmd_payload_wr) begin
            IBusCachedPlugin_injectionPort_valid = 1'b1; // @[DebugPlugin.scala 293:35]
          end
        end
        default : begin
        end
      endcase
    end
  end

  assign IBusCachedPlugin_injectionPort_payload = debug_bus_cmd_payload_data; // @[DebugPlugin.scala 277:29]
  assign switch_DebugPlugin_l280 = debug_bus_cmd_payload_address[7 : 2]; // @[BaseType.scala 299:24]
  assign when_DebugPlugin_l284 = debug_bus_cmd_payload_data[16]; // @[BaseType.scala 305:24]
  assign when_DebugPlugin_l284_1 = debug_bus_cmd_payload_data[24]; // @[BaseType.scala 305:24]
  assign when_DebugPlugin_l285 = debug_bus_cmd_payload_data[17]; // @[BaseType.scala 305:24]
  assign when_DebugPlugin_l285_1 = debug_bus_cmd_payload_data[25]; // @[BaseType.scala 305:24]
  assign when_DebugPlugin_l286 = debug_bus_cmd_payload_data[25]; // @[BaseType.scala 305:24]
  assign when_DebugPlugin_l287 = debug_bus_cmd_payload_data[25]; // @[BaseType.scala 305:24]
  assign when_DebugPlugin_l288 = debug_bus_cmd_payload_data[18]; // @[BaseType.scala 305:24]
  assign when_DebugPlugin_l288_1 = debug_bus_cmd_payload_data[26]; // @[BaseType.scala 305:24]
  assign when_DebugPlugin_l308 = (execute_arbitration_isValid && execute_DO_EBREAK); // @[BaseType.scala 305:24]
  assign when_DebugPlugin_l311 = (({writeBack_arbitration_isValid,memory_arbitration_isValid} != 2'b00) == 1'b0); // @[BaseType.scala 305:24]
  assign when_DebugPlugin_l324 = (DebugPlugin_stepIt && IBusCachedPlugin_incomingInstruction); // @[BaseType.scala 305:24]
  assign debug_resetOut = DebugPlugin_resetIt_regNext; // @[DebugPlugin.scala 341:19]
  assign when_DebugPlugin_l344 = (DebugPlugin_haltIt || DebugPlugin_stepIt); // @[BaseType.scala 305:24]
  assign execute_BranchPlugin_eq = (execute_SRC1 == execute_SRC2); // @[BaseType.scala 305:24]
  assign switch_Misc_l226_2 = execute_INSTRUCTION[14 : 12]; // @[BaseType.scala 299:24]
  always @(*) begin
    casez(switch_Misc_l226_2)
      3'b000 : begin
        _zz_execute_BRANCH_COND_RESULT = execute_BranchPlugin_eq; // @[Misc.scala 239:22]
      end
      3'b001 : begin
        _zz_execute_BRANCH_COND_RESULT = (! execute_BranchPlugin_eq); // @[Misc.scala 239:22]
      end
      3'b1?1 : begin
        _zz_execute_BRANCH_COND_RESULT = (! execute_SRC_LESS); // @[Misc.scala 239:22]
      end
      default : begin
        _zz_execute_BRANCH_COND_RESULT = execute_SRC_LESS; // @[Misc.scala 235:22]
      end
    endcase
  end

  always @(*) begin
    case(execute_BRANCH_CTRL)
      BranchCtrlEnum_INC : begin
        _zz_execute_BRANCH_COND_RESULT_1 = 1'b0; // @[Misc.scala 239:22]
      end
      BranchCtrlEnum_JAL : begin
        _zz_execute_BRANCH_COND_RESULT_1 = 1'b1; // @[Misc.scala 239:22]
      end
      BranchCtrlEnum_JALR : begin
        _zz_execute_BRANCH_COND_RESULT_1 = 1'b1; // @[Misc.scala 239:22]
      end
      default : begin
        _zz_execute_BRANCH_COND_RESULT_1 = _zz_execute_BRANCH_COND_RESULT; // @[Misc.scala 239:22]
      end
    endcase
  end

  assign _zz_execute_BranchPlugin_missAlignedTarget = execute_INSTRUCTION[31]; // @[BaseType.scala 305:24]
  always @(*) begin
    _zz_execute_BranchPlugin_missAlignedTarget_1[19] = _zz_execute_BranchPlugin_missAlignedTarget; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_1[18] = _zz_execute_BranchPlugin_missAlignedTarget; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_1[17] = _zz_execute_BranchPlugin_missAlignedTarget; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_1[16] = _zz_execute_BranchPlugin_missAlignedTarget; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_1[15] = _zz_execute_BranchPlugin_missAlignedTarget; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_1[14] = _zz_execute_BranchPlugin_missAlignedTarget; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_1[13] = _zz_execute_BranchPlugin_missAlignedTarget; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_1[12] = _zz_execute_BranchPlugin_missAlignedTarget; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_1[11] = _zz_execute_BranchPlugin_missAlignedTarget; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_1[10] = _zz_execute_BranchPlugin_missAlignedTarget; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_1[9] = _zz_execute_BranchPlugin_missAlignedTarget; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_1[8] = _zz_execute_BranchPlugin_missAlignedTarget; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_1[7] = _zz_execute_BranchPlugin_missAlignedTarget; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_1[6] = _zz_execute_BranchPlugin_missAlignedTarget; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_1[5] = _zz_execute_BranchPlugin_missAlignedTarget; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_1[4] = _zz_execute_BranchPlugin_missAlignedTarget; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_1[3] = _zz_execute_BranchPlugin_missAlignedTarget; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_1[2] = _zz_execute_BranchPlugin_missAlignedTarget; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_1[1] = _zz_execute_BranchPlugin_missAlignedTarget; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_1[0] = _zz_execute_BranchPlugin_missAlignedTarget; // @[Literal.scala 87:17]
  end

  assign _zz_execute_BranchPlugin_missAlignedTarget_2 = _zz__zz_execute_BranchPlugin_missAlignedTarget_2[19]; // @[BaseType.scala 305:24]
  always @(*) begin
    _zz_execute_BranchPlugin_missAlignedTarget_3[10] = _zz_execute_BranchPlugin_missAlignedTarget_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_3[9] = _zz_execute_BranchPlugin_missAlignedTarget_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_3[8] = _zz_execute_BranchPlugin_missAlignedTarget_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_3[7] = _zz_execute_BranchPlugin_missAlignedTarget_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_3[6] = _zz_execute_BranchPlugin_missAlignedTarget_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_3[5] = _zz_execute_BranchPlugin_missAlignedTarget_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_3[4] = _zz_execute_BranchPlugin_missAlignedTarget_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_3[3] = _zz_execute_BranchPlugin_missAlignedTarget_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_3[2] = _zz_execute_BranchPlugin_missAlignedTarget_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_3[1] = _zz_execute_BranchPlugin_missAlignedTarget_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_3[0] = _zz_execute_BranchPlugin_missAlignedTarget_2; // @[Literal.scala 87:17]
  end

  assign _zz_execute_BranchPlugin_missAlignedTarget_4 = _zz__zz_execute_BranchPlugin_missAlignedTarget_4[11]; // @[BaseType.scala 305:24]
  always @(*) begin
    _zz_execute_BranchPlugin_missAlignedTarget_5[18] = _zz_execute_BranchPlugin_missAlignedTarget_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_5[17] = _zz_execute_BranchPlugin_missAlignedTarget_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_5[16] = _zz_execute_BranchPlugin_missAlignedTarget_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_5[15] = _zz_execute_BranchPlugin_missAlignedTarget_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_5[14] = _zz_execute_BranchPlugin_missAlignedTarget_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_5[13] = _zz_execute_BranchPlugin_missAlignedTarget_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_5[12] = _zz_execute_BranchPlugin_missAlignedTarget_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_5[11] = _zz_execute_BranchPlugin_missAlignedTarget_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_5[10] = _zz_execute_BranchPlugin_missAlignedTarget_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_5[9] = _zz_execute_BranchPlugin_missAlignedTarget_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_5[8] = _zz_execute_BranchPlugin_missAlignedTarget_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_5[7] = _zz_execute_BranchPlugin_missAlignedTarget_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_5[6] = _zz_execute_BranchPlugin_missAlignedTarget_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_5[5] = _zz_execute_BranchPlugin_missAlignedTarget_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_5[4] = _zz_execute_BranchPlugin_missAlignedTarget_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_5[3] = _zz_execute_BranchPlugin_missAlignedTarget_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_5[2] = _zz_execute_BranchPlugin_missAlignedTarget_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_5[1] = _zz_execute_BranchPlugin_missAlignedTarget_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_missAlignedTarget_5[0] = _zz_execute_BranchPlugin_missAlignedTarget_4; // @[Literal.scala 87:17]
  end

  always @(*) begin
    case(execute_BRANCH_CTRL)
      BranchCtrlEnum_JALR : begin
        _zz_execute_BranchPlugin_missAlignedTarget_6 = (_zz__zz_execute_BranchPlugin_missAlignedTarget_6[1] ^ execute_RS1[1]); // @[Misc.scala 239:22]
      end
      BranchCtrlEnum_JAL : begin
        _zz_execute_BranchPlugin_missAlignedTarget_6 = _zz__zz_execute_BranchPlugin_missAlignedTarget_6_1[1]; // @[Misc.scala 239:22]
      end
      default : begin
        _zz_execute_BranchPlugin_missAlignedTarget_6 = _zz__zz_execute_BranchPlugin_missAlignedTarget_6_2[1]; // @[Misc.scala 235:22]
      end
    endcase
  end

  assign execute_BranchPlugin_missAlignedTarget = (execute_BRANCH_COND_RESULT && _zz_execute_BranchPlugin_missAlignedTarget_6); // @[BaseType.scala 305:24]
  always @(*) begin
    case(execute_BRANCH_CTRL)
      BranchCtrlEnum_JALR : begin
        execute_BranchPlugin_branch_src1 = execute_RS1; // @[BranchPlugin.scala 272:23]
      end
      default : begin
        execute_BranchPlugin_branch_src1 = execute_PC; // @[BranchPlugin.scala 276:23]
      end
    endcase
  end

  assign _zz_execute_BranchPlugin_branch_src2 = execute_INSTRUCTION[31]; // @[BaseType.scala 305:24]
  always @(*) begin
    _zz_execute_BranchPlugin_branch_src2_1[19] = _zz_execute_BranchPlugin_branch_src2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_1[18] = _zz_execute_BranchPlugin_branch_src2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_1[17] = _zz_execute_BranchPlugin_branch_src2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_1[16] = _zz_execute_BranchPlugin_branch_src2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_1[15] = _zz_execute_BranchPlugin_branch_src2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_1[14] = _zz_execute_BranchPlugin_branch_src2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_1[13] = _zz_execute_BranchPlugin_branch_src2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_1[12] = _zz_execute_BranchPlugin_branch_src2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_1[11] = _zz_execute_BranchPlugin_branch_src2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_1[10] = _zz_execute_BranchPlugin_branch_src2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_1[9] = _zz_execute_BranchPlugin_branch_src2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_1[8] = _zz_execute_BranchPlugin_branch_src2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_1[7] = _zz_execute_BranchPlugin_branch_src2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_1[6] = _zz_execute_BranchPlugin_branch_src2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_1[5] = _zz_execute_BranchPlugin_branch_src2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_1[4] = _zz_execute_BranchPlugin_branch_src2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_1[3] = _zz_execute_BranchPlugin_branch_src2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_1[2] = _zz_execute_BranchPlugin_branch_src2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_1[1] = _zz_execute_BranchPlugin_branch_src2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_1[0] = _zz_execute_BranchPlugin_branch_src2; // @[Literal.scala 87:17]
  end

  always @(*) begin
    case(execute_BRANCH_CTRL)
      BranchCtrlEnum_JALR : begin
        execute_BranchPlugin_branch_src2 = {_zz_execute_BranchPlugin_branch_src2_1,execute_INSTRUCTION[31 : 20]}; // @[BranchPlugin.scala 273:23]
      end
      default : begin
        execute_BranchPlugin_branch_src2 = ((execute_BRANCH_CTRL == BranchCtrlEnum_JAL) ? {{_zz_execute_BranchPlugin_branch_src2_3,{{{_zz_execute_BranchPlugin_branch_src2_6,execute_INSTRUCTION[19 : 12]},execute_INSTRUCTION[20]},execute_INSTRUCTION[30 : 21]}},1'b0} : {{_zz_execute_BranchPlugin_branch_src2_5,{{{_zz_execute_BranchPlugin_branch_src2_7,_zz_execute_BranchPlugin_branch_src2_8},execute_INSTRUCTION[30 : 25]},execute_INSTRUCTION[11 : 8]}},1'b0}); // @[BranchPlugin.scala 277:23]
        if(execute_PREDICTION_HAD_BRANCHED2) begin
          execute_BranchPlugin_branch_src2 = {29'd0, _zz_execute_BranchPlugin_branch_src2_9}; // @[BranchPlugin.scala 279:25]
        end
      end
    endcase
  end

  assign _zz_execute_BranchPlugin_branch_src2_2 = _zz__zz_execute_BranchPlugin_branch_src2_2[19]; // @[BaseType.scala 305:24]
  always @(*) begin
    _zz_execute_BranchPlugin_branch_src2_3[10] = _zz_execute_BranchPlugin_branch_src2_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_3[9] = _zz_execute_BranchPlugin_branch_src2_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_3[8] = _zz_execute_BranchPlugin_branch_src2_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_3[7] = _zz_execute_BranchPlugin_branch_src2_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_3[6] = _zz_execute_BranchPlugin_branch_src2_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_3[5] = _zz_execute_BranchPlugin_branch_src2_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_3[4] = _zz_execute_BranchPlugin_branch_src2_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_3[3] = _zz_execute_BranchPlugin_branch_src2_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_3[2] = _zz_execute_BranchPlugin_branch_src2_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_3[1] = _zz_execute_BranchPlugin_branch_src2_2; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_3[0] = _zz_execute_BranchPlugin_branch_src2_2; // @[Literal.scala 87:17]
  end

  assign _zz_execute_BranchPlugin_branch_src2_4 = _zz__zz_execute_BranchPlugin_branch_src2_4[11]; // @[BaseType.scala 305:24]
  always @(*) begin
    _zz_execute_BranchPlugin_branch_src2_5[18] = _zz_execute_BranchPlugin_branch_src2_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_5[17] = _zz_execute_BranchPlugin_branch_src2_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_5[16] = _zz_execute_BranchPlugin_branch_src2_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_5[15] = _zz_execute_BranchPlugin_branch_src2_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_5[14] = _zz_execute_BranchPlugin_branch_src2_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_5[13] = _zz_execute_BranchPlugin_branch_src2_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_5[12] = _zz_execute_BranchPlugin_branch_src2_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_5[11] = _zz_execute_BranchPlugin_branch_src2_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_5[10] = _zz_execute_BranchPlugin_branch_src2_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_5[9] = _zz_execute_BranchPlugin_branch_src2_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_5[8] = _zz_execute_BranchPlugin_branch_src2_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_5[7] = _zz_execute_BranchPlugin_branch_src2_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_5[6] = _zz_execute_BranchPlugin_branch_src2_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_5[5] = _zz_execute_BranchPlugin_branch_src2_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_5[4] = _zz_execute_BranchPlugin_branch_src2_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_5[3] = _zz_execute_BranchPlugin_branch_src2_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_5[2] = _zz_execute_BranchPlugin_branch_src2_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_5[1] = _zz_execute_BranchPlugin_branch_src2_4; // @[Literal.scala 87:17]
    _zz_execute_BranchPlugin_branch_src2_5[0] = _zz_execute_BranchPlugin_branch_src2_4; // @[Literal.scala 87:17]
  end

  assign execute_BranchPlugin_branchAdder = (execute_BranchPlugin_branch_src1 + execute_BranchPlugin_branch_src2); // @[BaseType.scala 299:24]
  assign BranchPlugin_jumpInterface_valid = ((memory_arbitration_isValid && memory_BRANCH_DO) && (! 1'b0)); // @[BranchPlugin.scala 292:27]
  assign BranchPlugin_jumpInterface_payload = memory_BRANCH_CALC; // @[BranchPlugin.scala 293:29]
  assign BranchPlugin_branchExceptionPort_valid = (memory_arbitration_isValid && (memory_BRANCH_DO && memory_BRANCH_CALC[1])); // @[BranchPlugin.scala 298:35]
  assign BranchPlugin_branchExceptionPort_payload_code = 4'b0000; // @[BranchPlugin.scala 299:34]
  assign BranchPlugin_branchExceptionPort_payload_badAddr = memory_BRANCH_CALC; // @[BranchPlugin.scala 300:37]
  assign IBusCachedPlugin_decodePrediction_rsp_wasWrong = BranchPlugin_jumpInterface_valid; // @[BranchPlugin.scala 306:35]
  assign when_Pipeline_l124 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_1 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_2 = ((! writeBack_arbitration_isStuck) && (! CsrPlugin_exceptionPortCtrl_exceptionValids_writeBack)); // @[BaseType.scala 305:24]
  assign when_Pipeline_l124_3 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_4 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_5 = (! writeBack_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_6 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_7 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_8 = (! writeBack_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_9 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign _zz_decode_to_execute_SRC1_CTRL_1 = decode_SRC1_CTRL; // @[Pipeline.scala 110:25]
  assign _zz_decode_SRC1_CTRL = _zz_decode_SRC1_CTRL_1; // @[Pipeline.scala 121:26]
  assign when_Pipeline_l124_10 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign _zz_execute_SRC1_CTRL = decode_to_execute_SRC1_CTRL; // @[Pipeline.scala 124:26]
  assign when_Pipeline_l124_11 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_12 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_13 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_14 = (! writeBack_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign _zz_decode_to_execute_ALU_CTRL_1 = decode_ALU_CTRL; // @[Pipeline.scala 110:25]
  assign _zz_decode_ALU_CTRL = _zz_decode_ALU_CTRL_1; // @[Pipeline.scala 121:26]
  assign when_Pipeline_l124_15 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign _zz_execute_ALU_CTRL = decode_to_execute_ALU_CTRL; // @[Pipeline.scala 124:26]
  assign _zz_decode_to_execute_SRC2_CTRL_1 = decode_SRC2_CTRL; // @[Pipeline.scala 110:25]
  assign _zz_decode_SRC2_CTRL = _zz_decode_SRC2_CTRL_1; // @[Pipeline.scala 121:26]
  assign when_Pipeline_l124_16 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign _zz_execute_SRC2_CTRL = decode_to_execute_SRC2_CTRL; // @[Pipeline.scala 124:26]
  assign when_Pipeline_l124_17 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_18 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_19 = (! writeBack_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_20 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_21 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_22 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_23 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_24 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_25 = (! writeBack_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_26 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_27 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign _zz_decode_to_execute_ALU_BITWISE_CTRL_1 = decode_ALU_BITWISE_CTRL; // @[Pipeline.scala 110:25]
  assign _zz_decode_ALU_BITWISE_CTRL = _zz_decode_ALU_BITWISE_CTRL_1; // @[Pipeline.scala 121:26]
  assign when_Pipeline_l124_28 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign _zz_execute_ALU_BITWISE_CTRL = decode_to_execute_ALU_BITWISE_CTRL; // @[Pipeline.scala 124:26]
  assign _zz_decode_to_execute_SHIFT_CTRL_1 = decode_SHIFT_CTRL; // @[Pipeline.scala 110:25]
  assign _zz_execute_to_memory_SHIFT_CTRL_1 = execute_SHIFT_CTRL; // @[Pipeline.scala 110:25]
  assign _zz_decode_SHIFT_CTRL = _zz_decode_SHIFT_CTRL_1; // @[Pipeline.scala 121:26]
  assign when_Pipeline_l124_29 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign _zz_execute_SHIFT_CTRL = decode_to_execute_SHIFT_CTRL; // @[Pipeline.scala 124:26]
  assign when_Pipeline_l124_30 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign _zz_memory_SHIFT_CTRL = execute_to_memory_SHIFT_CTRL; // @[Pipeline.scala 124:26]
  assign when_Pipeline_l124_31 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_32 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_33 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_34 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_35 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_36 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_37 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign _zz_decode_to_execute_ENV_CTRL_1 = decode_ENV_CTRL; // @[Pipeline.scala 110:25]
  assign _zz_execute_to_memory_ENV_CTRL_1 = execute_ENV_CTRL; // @[Pipeline.scala 110:25]
  assign _zz_memory_to_writeBack_ENV_CTRL_1 = memory_ENV_CTRL; // @[Pipeline.scala 110:25]
  assign _zz_decode_ENV_CTRL = _zz_decode_ENV_CTRL_1; // @[Pipeline.scala 121:26]
  assign when_Pipeline_l124_38 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign _zz_execute_ENV_CTRL = decode_to_execute_ENV_CTRL; // @[Pipeline.scala 124:26]
  assign when_Pipeline_l124_39 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign _zz_memory_ENV_CTRL = execute_to_memory_ENV_CTRL; // @[Pipeline.scala 124:26]
  assign when_Pipeline_l124_40 = (! writeBack_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign _zz_writeBack_ENV_CTRL = memory_to_writeBack_ENV_CTRL; // @[Pipeline.scala 124:26]
  assign _zz_decode_to_execute_BRANCH_CTRL_1 = decode_BRANCH_CTRL; // @[Pipeline.scala 110:25]
  assign _zz_decode_BRANCH_CTRL_1 = _zz_decode_BRANCH_CTRL; // @[Pipeline.scala 121:26]
  assign when_Pipeline_l124_41 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign _zz_execute_BRANCH_CTRL = decode_to_execute_BRANCH_CTRL; // @[Pipeline.scala 124:26]
  assign when_Pipeline_l124_42 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_43 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_44 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_45 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_46 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_47 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_48 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_49 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_50 = (! writeBack_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_51 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_52 = (! writeBack_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_53 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_54 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_Pipeline_l124_55 = (! memory_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign decode_arbitration_isFlushed = (({writeBack_arbitration_flushNext,{memory_arbitration_flushNext,execute_arbitration_flushNext}} != 3'b000) || ({writeBack_arbitration_flushIt,{memory_arbitration_flushIt,{execute_arbitration_flushIt,decode_arbitration_flushIt}}} != 4'b0000)); // @[Pipeline.scala 132:35]
  assign execute_arbitration_isFlushed = (({writeBack_arbitration_flushNext,memory_arbitration_flushNext} != 2'b00) || ({writeBack_arbitration_flushIt,{memory_arbitration_flushIt,execute_arbitration_flushIt}} != 3'b000)); // @[Pipeline.scala 132:35]
  assign memory_arbitration_isFlushed = ((writeBack_arbitration_flushNext != 1'b0) || ({writeBack_arbitration_flushIt,memory_arbitration_flushIt} != 2'b00)); // @[Pipeline.scala 132:35]
  assign writeBack_arbitration_isFlushed = (1'b0 || (writeBack_arbitration_flushIt != 1'b0)); // @[Pipeline.scala 132:35]
  assign decode_arbitration_isStuckByOthers = (decode_arbitration_haltByOther || (((1'b0 || execute_arbitration_isStuck) || memory_arbitration_isStuck) || writeBack_arbitration_isStuck)); // @[Pipeline.scala 141:41]
  assign decode_arbitration_isStuck = (decode_arbitration_haltItself || decode_arbitration_isStuckByOthers); // @[Pipeline.scala 142:33]
  assign decode_arbitration_isMoving = ((! decode_arbitration_isStuck) && (! decode_arbitration_removeIt)); // @[Pipeline.scala 143:34]
  assign decode_arbitration_isFiring = ((decode_arbitration_isValid && (! decode_arbitration_isStuck)) && (! decode_arbitration_removeIt)); // @[Pipeline.scala 144:34]
  assign execute_arbitration_isStuckByOthers = (execute_arbitration_haltByOther || ((1'b0 || memory_arbitration_isStuck) || writeBack_arbitration_isStuck)); // @[Pipeline.scala 141:41]
  assign execute_arbitration_isStuck = (execute_arbitration_haltItself || execute_arbitration_isStuckByOthers); // @[Pipeline.scala 142:33]
  assign execute_arbitration_isMoving = ((! execute_arbitration_isStuck) && (! execute_arbitration_removeIt)); // @[Pipeline.scala 143:34]
  assign execute_arbitration_isFiring = ((execute_arbitration_isValid && (! execute_arbitration_isStuck)) && (! execute_arbitration_removeIt)); // @[Pipeline.scala 144:34]
  assign memory_arbitration_isStuckByOthers = (memory_arbitration_haltByOther || (1'b0 || writeBack_arbitration_isStuck)); // @[Pipeline.scala 141:41]
  assign memory_arbitration_isStuck = (memory_arbitration_haltItself || memory_arbitration_isStuckByOthers); // @[Pipeline.scala 142:33]
  assign memory_arbitration_isMoving = ((! memory_arbitration_isStuck) && (! memory_arbitration_removeIt)); // @[Pipeline.scala 143:34]
  assign memory_arbitration_isFiring = ((memory_arbitration_isValid && (! memory_arbitration_isStuck)) && (! memory_arbitration_removeIt)); // @[Pipeline.scala 144:34]
  assign writeBack_arbitration_isStuckByOthers = (writeBack_arbitration_haltByOther || 1'b0); // @[Pipeline.scala 141:41]
  assign writeBack_arbitration_isStuck = (writeBack_arbitration_haltItself || writeBack_arbitration_isStuckByOthers); // @[Pipeline.scala 142:33]
  assign writeBack_arbitration_isMoving = ((! writeBack_arbitration_isStuck) && (! writeBack_arbitration_removeIt)); // @[Pipeline.scala 143:34]
  assign writeBack_arbitration_isFiring = ((writeBack_arbitration_isValid && (! writeBack_arbitration_isStuck)) && (! writeBack_arbitration_removeIt)); // @[Pipeline.scala 144:34]
  assign when_Pipeline_l151 = ((! execute_arbitration_isStuck) || execute_arbitration_removeIt); // @[BaseType.scala 305:24]
  assign when_Pipeline_l154 = ((! decode_arbitration_isStuck) && (! decode_arbitration_removeIt)); // @[BaseType.scala 305:24]
  assign when_Pipeline_l151_1 = ((! memory_arbitration_isStuck) || memory_arbitration_removeIt); // @[BaseType.scala 305:24]
  assign when_Pipeline_l154_1 = ((! execute_arbitration_isStuck) && (! execute_arbitration_removeIt)); // @[BaseType.scala 305:24]
  assign when_Pipeline_l151_2 = ((! writeBack_arbitration_isStuck) || writeBack_arbitration_removeIt); // @[BaseType.scala 305:24]
  assign when_Pipeline_l154_2 = ((! memory_arbitration_isStuck) && (! memory_arbitration_removeIt)); // @[BaseType.scala 305:24]
  always @(*) begin
    IBusCachedPlugin_injectionPort_ready = 1'b0; // @[Fetcher.scala 361:31]
    case(switch_Fetcher_l365)
      3'b100 : begin
        IBusCachedPlugin_injectionPort_ready = 1'b1; // @[Fetcher.scala 386:35]
      end
      default : begin
      end
    endcase
  end

  assign when_Fetcher_l381 = (! decode_arbitration_isStuck); // @[BaseType.scala 299:24]
  always @(*) begin
    execute_PmpPlugin_fsm_stateNext = execute_PmpPlugin_fsm_stateReg; // @[StateMachine.scala 217:17]
    case(execute_PmpPlugin_fsm_stateReg)
      execute_PmpPlugin_fsm_enumDef_stateIdle : begin
        if(execute_PmpPlugin_fsmPending) begin
          execute_PmpPlugin_fsm_stateNext = execute_PmpPlugin_fsm_enumDef_stateWrite; // @[Enum.scala 148:67]
        end
      end
      execute_PmpPlugin_fsm_enumDef_stateWrite : begin
        if(execute_PmpPlugin_pmpcfgCsr_) begin
          execute_PmpPlugin_fsm_stateNext = execute_PmpPlugin_fsm_enumDef_stateCfg; // @[Enum.scala 148:67]
        end
        if(execute_PmpPlugin_pmpaddrCsr_) begin
          execute_PmpPlugin_fsm_stateNext = execute_PmpPlugin_fsm_enumDef_stateAddr; // @[Enum.scala 148:67]
        end
      end
      execute_PmpPlugin_fsm_enumDef_stateCfg : begin
        if(when_PmpPlugin_l229) begin
          execute_PmpPlugin_fsm_stateNext = execute_PmpPlugin_fsm_enumDef_stateIdle; // @[Enum.scala 148:67]
        end
      end
      execute_PmpPlugin_fsm_enumDef_stateAddr : begin
        execute_PmpPlugin_fsm_stateNext = execute_PmpPlugin_fsm_enumDef_stateIdle; // @[Enum.scala 148:67]
      end
      default : begin
      end
    endcase
    if(execute_PmpPlugin_fsm_wantStart) begin
      execute_PmpPlugin_fsm_stateNext = execute_PmpPlugin_fsm_enumDef_stateIdle; // @[Enum.scala 148:67]
    end
    if(execute_PmpPlugin_fsm_wantKill) begin
      execute_PmpPlugin_fsm_stateNext = execute_PmpPlugin_fsm_enumDef_BOOT; // @[Enum.scala 148:67]
    end
  end

  assign _zz_PmpPlugin_pmpcfg_0 = execute_PmpPlugin_writeData_[7 : 0]; // @[BaseType.scala 299:24]
  assign _zz_PmpPlugin_pmpcfg_0_1 = execute_PmpPlugin_writeData_[15 : 8]; // @[BaseType.scala 299:24]
  assign _zz_PmpPlugin_pmpcfg_0_2 = execute_PmpPlugin_writeData_[23 : 16]; // @[BaseType.scala 299:24]
  assign _zz_PmpPlugin_pmpcfg_0_3 = execute_PmpPlugin_writeData_[31 : 24]; // @[BaseType.scala 299:24]
  assign when_PmpPlugin_l209 = (! _zz_when_PmpPlugin_l209[7]); // @[BaseType.scala 299:24]
  assign _zz_11 = ({15'd0,1'b1} <<< {execute_PmpPlugin_pmpcfgN_,2'b00}); // @[BaseType.scala 299:24]
  assign when_PmpPlugin_l209_1 = (! _zz_when_PmpPlugin_l209_1_1[7]); // @[BaseType.scala 299:24]
  assign _zz_12 = ({15'd0,1'b1} <<< {execute_PmpPlugin_pmpcfgN_,2'b01}); // @[BaseType.scala 299:24]
  assign when_PmpPlugin_l209_2 = (! _zz_when_PmpPlugin_l209_2[7]); // @[BaseType.scala 299:24]
  assign _zz_13 = ({15'd0,1'b1} <<< {execute_PmpPlugin_pmpcfgN_,2'b10}); // @[BaseType.scala 299:24]
  assign when_PmpPlugin_l209_3 = (! _zz_when_PmpPlugin_l209_3[7]); // @[BaseType.scala 299:24]
  assign _zz_14 = ({15'd0,1'b1} <<< {execute_PmpPlugin_pmpcfgN_,2'b11}); // @[BaseType.scala 299:24]
  assign when_PmpPlugin_l216 = (! _zz_when_PmpPlugin_l216[7]); // @[BaseType.scala 299:24]
  assign when_PmpPlugin_l229 = (execute_PmpPlugin_fsm_fsmCounter[1 : 0] == 2'b11); // @[BaseType.scala 305:24]
  assign when_StateMachine_l237 = ((execute_PmpPlugin_fsm_stateReg == execute_PmpPlugin_fsm_enumDef_stateWrite) && (! (execute_PmpPlugin_fsm_stateNext == execute_PmpPlugin_fsm_enumDef_stateWrite))); // @[BaseType.scala 305:24]
  assign when_StateMachine_l253 = ((! (execute_PmpPlugin_fsm_stateReg == execute_PmpPlugin_fsm_enumDef_stateIdle)) && (execute_PmpPlugin_fsm_stateNext == execute_PmpPlugin_fsm_enumDef_stateIdle)); // @[BaseType.scala 305:24]
  assign when_StateMachine_l253_1 = ((! (execute_PmpPlugin_fsm_stateReg == execute_PmpPlugin_fsm_enumDef_stateCfg)) && (execute_PmpPlugin_fsm_stateNext == execute_PmpPlugin_fsm_enumDef_stateCfg)); // @[BaseType.scala 305:24]
  assign when_StateMachine_l253_2 = ((! (execute_PmpPlugin_fsm_stateReg == execute_PmpPlugin_fsm_enumDef_stateAddr)) && (execute_PmpPlugin_fsm_stateNext == execute_PmpPlugin_fsm_enumDef_stateAddr)); // @[BaseType.scala 305:24]
  assign when_CsrPlugin_l1589 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1589_1 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1589_2 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1589_3 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1589_4 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1589_5 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1589_6 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1589_7 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1589_8 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1589_9 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1589_10 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1589_11 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1589_12 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1589_13 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1589_14 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1589_15 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1589_16 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1589_17 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1589_18 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1589_19 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  assign when_CsrPlugin_l1589_20 = (! execute_arbitration_isStuck); // @[BaseType.scala 299:24]
  always @(*) begin
    _zz_CsrPlugin_csrMapping_readDataInit = 32'h0; // @[Expression.scala 2301:18]
    if(execute_CsrPlugin_csr_3857) begin
      _zz_CsrPlugin_csrMapping_readDataInit[0 : 0] = 1'b1; // @[CsrPlugin.scala 1598:138]
    end
  end

  always @(*) begin
    _zz_CsrPlugin_csrMapping_readDataInit_1 = 32'h0; // @[Expression.scala 2301:18]
    if(execute_CsrPlugin_csr_3858) begin
      _zz_CsrPlugin_csrMapping_readDataInit_1[1 : 0] = 2'b10; // @[CsrPlugin.scala 1598:138]
    end
  end

  always @(*) begin
    _zz_CsrPlugin_csrMapping_readDataInit_2 = 32'h0; // @[Expression.scala 2301:18]
    if(execute_CsrPlugin_csr_3859) begin
      _zz_CsrPlugin_csrMapping_readDataInit_2[1 : 0] = 2'b11; // @[CsrPlugin.scala 1598:138]
    end
  end

  always @(*) begin
    _zz_CsrPlugin_csrMapping_readDataInit_3 = 32'h0; // @[Expression.scala 2301:18]
    if(execute_CsrPlugin_csr_769) begin
      _zz_CsrPlugin_csrMapping_readDataInit_3[31 : 30] = CsrPlugin_misa_base; // @[CsrPlugin.scala 1598:138]
      _zz_CsrPlugin_csrMapping_readDataInit_3[25 : 0] = CsrPlugin_misa_extensions; // @[CsrPlugin.scala 1598:138]
    end
  end

  assign switch_CsrPlugin_l980 = CsrPlugin_csrMapping_writeDataSignal[12 : 11]; // @[BaseType.scala 299:24]
  always @(*) begin
    _zz_CsrPlugin_csrMapping_readDataInit_4 = 32'h0; // @[Expression.scala 2301:18]
    if(execute_CsrPlugin_csr_768) begin
      _zz_CsrPlugin_csrMapping_readDataInit_4[7 : 7] = CsrPlugin_mstatus_MPIE; // @[CsrPlugin.scala 1598:138]
      _zz_CsrPlugin_csrMapping_readDataInit_4[3 : 3] = CsrPlugin_mstatus_MIE; // @[CsrPlugin.scala 1598:138]
      _zz_CsrPlugin_csrMapping_readDataInit_4[12 : 11] = CsrPlugin_mstatus_MPP; // @[CsrPlugin.scala 1598:138]
    end
  end

  always @(*) begin
    _zz_CsrPlugin_csrMapping_readDataInit_5 = 32'h0; // @[Expression.scala 2301:18]
    if(execute_CsrPlugin_csr_836) begin
      _zz_CsrPlugin_csrMapping_readDataInit_5[11 : 11] = CsrPlugin_mip_MEIP; // @[CsrPlugin.scala 1598:138]
      _zz_CsrPlugin_csrMapping_readDataInit_5[7 : 7] = CsrPlugin_mip_MTIP; // @[CsrPlugin.scala 1598:138]
      _zz_CsrPlugin_csrMapping_readDataInit_5[3 : 3] = CsrPlugin_mip_MSIP; // @[CsrPlugin.scala 1598:138]
    end
  end

  always @(*) begin
    _zz_CsrPlugin_csrMapping_readDataInit_6 = 32'h0; // @[Expression.scala 2301:18]
    if(execute_CsrPlugin_csr_772) begin
      _zz_CsrPlugin_csrMapping_readDataInit_6[11 : 11] = CsrPlugin_mie_MEIE; // @[CsrPlugin.scala 1598:138]
      _zz_CsrPlugin_csrMapping_readDataInit_6[7 : 7] = CsrPlugin_mie_MTIE; // @[CsrPlugin.scala 1598:138]
      _zz_CsrPlugin_csrMapping_readDataInit_6[3 : 3] = CsrPlugin_mie_MSIE; // @[CsrPlugin.scala 1598:138]
    end
  end

  always @(*) begin
    _zz_CsrPlugin_csrMapping_readDataInit_7 = 32'h0; // @[Expression.scala 2301:18]
    if(execute_CsrPlugin_csr_773) begin
      _zz_CsrPlugin_csrMapping_readDataInit_7[31 : 2] = CsrPlugin_mtvec_base; // @[CsrPlugin.scala 1598:138]
      _zz_CsrPlugin_csrMapping_readDataInit_7[1 : 0] = CsrPlugin_mtvec_mode; // @[CsrPlugin.scala 1598:138]
    end
  end

  always @(*) begin
    _zz_CsrPlugin_csrMapping_readDataInit_8 = 32'h0; // @[Expression.scala 2301:18]
    if(execute_CsrPlugin_csr_833) begin
      _zz_CsrPlugin_csrMapping_readDataInit_8[31 : 0] = CsrPlugin_mepc; // @[CsrPlugin.scala 1598:138]
    end
  end

  always @(*) begin
    _zz_CsrPlugin_csrMapping_readDataInit_9 = 32'h0; // @[Expression.scala 2301:18]
    if(execute_CsrPlugin_csr_832) begin
      _zz_CsrPlugin_csrMapping_readDataInit_9[31 : 0] = CsrPlugin_mscratch; // @[CsrPlugin.scala 1598:138]
    end
  end

  always @(*) begin
    _zz_CsrPlugin_csrMapping_readDataInit_10 = 32'h0; // @[Expression.scala 2301:18]
    if(execute_CsrPlugin_csr_834) begin
      _zz_CsrPlugin_csrMapping_readDataInit_10[31 : 31] = CsrPlugin_mcause_interrupt; // @[CsrPlugin.scala 1598:138]
      _zz_CsrPlugin_csrMapping_readDataInit_10[3 : 0] = CsrPlugin_mcause_exceptionCode; // @[CsrPlugin.scala 1598:138]
    end
  end

  always @(*) begin
    _zz_CsrPlugin_csrMapping_readDataInit_11 = 32'h0; // @[Expression.scala 2301:18]
    if(execute_CsrPlugin_csr_835) begin
      _zz_CsrPlugin_csrMapping_readDataInit_11[31 : 0] = CsrPlugin_mtval; // @[CsrPlugin.scala 1598:138]
    end
  end

  always @(*) begin
    _zz_CsrPlugin_csrMapping_readDataInit_12 = 32'h0; // @[Expression.scala 2301:18]
    if(execute_CsrPlugin_csr_2816) begin
      _zz_CsrPlugin_csrMapping_readDataInit_12[31 : 0] = CsrPlugin_mcycle[31 : 0]; // @[CsrPlugin.scala 1598:138]
    end
  end

  always @(*) begin
    _zz_CsrPlugin_csrMapping_readDataInit_13 = 32'h0; // @[Expression.scala 2301:18]
    if(execute_CsrPlugin_csr_2944) begin
      _zz_CsrPlugin_csrMapping_readDataInit_13[31 : 0] = CsrPlugin_mcycle[63 : 32]; // @[CsrPlugin.scala 1598:138]
    end
  end

  always @(*) begin
    _zz_CsrPlugin_csrMapping_readDataInit_14 = 32'h0; // @[Expression.scala 2301:18]
    if(execute_CsrPlugin_csr_2818) begin
      _zz_CsrPlugin_csrMapping_readDataInit_14[31 : 0] = CsrPlugin_minstret[31 : 0]; // @[CsrPlugin.scala 1598:138]
    end
  end

  always @(*) begin
    _zz_CsrPlugin_csrMapping_readDataInit_15 = 32'h0; // @[Expression.scala 2301:18]
    if(execute_CsrPlugin_csr_2946) begin
      _zz_CsrPlugin_csrMapping_readDataInit_15[31 : 0] = CsrPlugin_minstret[63 : 32]; // @[CsrPlugin.scala 1598:138]
    end
  end

  always @(*) begin
    _zz_CsrPlugin_csrMapping_readDataInit_16 = 32'h0; // @[Expression.scala 2301:18]
    if(execute_CsrPlugin_csr_3072) begin
      _zz_CsrPlugin_csrMapping_readDataInit_16[31 : 0] = CsrPlugin_mcycle[31 : 0]; // @[CsrPlugin.scala 1598:138]
    end
  end

  always @(*) begin
    _zz_CsrPlugin_csrMapping_readDataInit_17 = 32'h0; // @[Expression.scala 2301:18]
    if(execute_CsrPlugin_csr_3200) begin
      _zz_CsrPlugin_csrMapping_readDataInit_17[31 : 0] = CsrPlugin_mcycle[63 : 32]; // @[CsrPlugin.scala 1598:138]
    end
  end

  always @(*) begin
    _zz_CsrPlugin_csrMapping_readDataInit_18 = 32'h0; // @[Expression.scala 2301:18]
    if(execute_CsrPlugin_csr_3074) begin
      _zz_CsrPlugin_csrMapping_readDataInit_18[31 : 0] = CsrPlugin_minstret[31 : 0]; // @[CsrPlugin.scala 1598:138]
    end
  end

  always @(*) begin
    _zz_CsrPlugin_csrMapping_readDataInit_19 = 32'h0; // @[Expression.scala 2301:18]
    if(execute_CsrPlugin_csr_3202) begin
      _zz_CsrPlugin_csrMapping_readDataInit_19[31 : 0] = CsrPlugin_minstret[63 : 32]; // @[CsrPlugin.scala 1598:138]
    end
  end

  assign CsrPlugin_csrMapping_readDataInit = (((((_zz_CsrPlugin_csrMapping_readDataInit | _zz_CsrPlugin_csrMapping_readDataInit_1) | (_zz_CsrPlugin_csrMapping_readDataInit_2 | _zz_CsrPlugin_csrMapping_readDataInit_20)) | ((_zz_CsrPlugin_csrMapping_readDataInit_3 | _zz_CsrPlugin_csrMapping_readDataInit_4) | (_zz_CsrPlugin_csrMapping_readDataInit_5 | _zz_CsrPlugin_csrMapping_readDataInit_6))) | (((_zz_CsrPlugin_csrMapping_readDataInit_7 | _zz_CsrPlugin_csrMapping_readDataInit_8) | (_zz_CsrPlugin_csrMapping_readDataInit_9 | _zz_CsrPlugin_csrMapping_readDataInit_10)) | ((_zz_CsrPlugin_csrMapping_readDataInit_11 | _zz_CsrPlugin_csrMapping_readDataInit_12) | (_zz_CsrPlugin_csrMapping_readDataInit_13 | _zz_CsrPlugin_csrMapping_readDataInit_14)))) | (((_zz_CsrPlugin_csrMapping_readDataInit_15 | _zz_CsrPlugin_csrMapping_readDataInit_16) | (_zz_CsrPlugin_csrMapping_readDataInit_17 | _zz_CsrPlugin_csrMapping_readDataInit_18)) | _zz_CsrPlugin_csrMapping_readDataInit_19)); // @[CsrPlugin.scala 1604:39]
  assign when_PmpPlugin_l155 = (CsrPlugin_privilege == 2'b11); // @[BaseType.scala 305:24]
  assign when_PmpPlugin_l172 = ((execute_PmpPlugin_pmpcfgCsr || execute_PmpPlugin_pmpaddrCsr) && (CsrPlugin_privilege == 2'b11)); // @[BaseType.scala 305:24]
  assign when_PmpPlugin_l175 = ((! execute_PmpPlugin_fsmPending) && CsrPlugin_csrMapping_hazardFree); // @[BaseType.scala 305:24]
  always @(*) begin
    when_CsrPlugin_l1625 = 1'b0; // @[CsrPlugin.scala 1622:27]
    if(when_CsrPlugin_l1623) begin
      when_CsrPlugin_l1625 = 1'b1; // @[CsrPlugin.scala 1623:21]
    end
  end

  assign when_CsrPlugin_l1623 = (CsrPlugin_privilege < execute_CsrPlugin_csrAddress[9 : 8]); // @[BaseType.scala 305:24]
  assign when_CsrPlugin_l1631 = ((! execute_arbitration_isValid) || (! execute_IS_CSR)); // @[BaseType.scala 305:24]
  always @(posedge clk or posedge reset) begin
    if(reset) begin
      IBusCachedPlugin_fetchPc_pcReg <= 32'h80000000; // @[Data.scala 400:33]
      IBusCachedPlugin_fetchPc_correctionReg <= 1'b0; // @[Data.scala 400:33]
      IBusCachedPlugin_fetchPc_booted <= 1'b0; // @[Data.scala 400:33]
      IBusCachedPlugin_fetchPc_inc <= 1'b0; // @[Data.scala 400:33]
      _zz_IBusCachedPlugin_iBusRsp_stages_1_input_valid_1 <= 1'b0; // @[Data.scala 400:33]
      _zz_IBusCachedPlugin_iBusRsp_stages_1_output_m2sPipe_valid <= 1'b0; // @[Data.scala 400:33]
      IBusCachedPlugin_injector_nextPcCalc_valids_0 <= 1'b0; // @[Data.scala 400:33]
      IBusCachedPlugin_injector_nextPcCalc_valids_1 <= 1'b0; // @[Data.scala 400:33]
      IBusCachedPlugin_injector_nextPcCalc_valids_2 <= 1'b0; // @[Data.scala 400:33]
      IBusCachedPlugin_injector_nextPcCalc_valids_3 <= 1'b0; // @[Data.scala 400:33]
      IBusCachedPlugin_injector_nextPcCalc_valids_4 <= 1'b0; // @[Data.scala 400:33]
      IBusCachedPlugin_rspCounter <= 32'h0; // @[Data.scala 400:33]
      DBusCachedPlugin_rspCounter <= 32'h0; // @[Data.scala 400:33]
      PmpPlugin_pmpcfg_0 <= 8'h0; // @[Data.scala 400:33]
      PmpPlugin_pmpcfg_1 <= 8'h0; // @[Data.scala 400:33]
      PmpPlugin_pmpcfg_2 <= 8'h0; // @[Data.scala 400:33]
      PmpPlugin_pmpcfg_3 <= 8'h0; // @[Data.scala 400:33]
      PmpPlugin_pmpcfg_4 <= 8'h0; // @[Data.scala 400:33]
      PmpPlugin_pmpcfg_5 <= 8'h0; // @[Data.scala 400:33]
      PmpPlugin_pmpcfg_6 <= 8'h0; // @[Data.scala 400:33]
      PmpPlugin_pmpcfg_7 <= 8'h0; // @[Data.scala 400:33]
      PmpPlugin_pmpcfg_8 <= 8'h0; // @[Data.scala 400:33]
      PmpPlugin_pmpcfg_9 <= 8'h0; // @[Data.scala 400:33]
      PmpPlugin_pmpcfg_10 <= 8'h0; // @[Data.scala 400:33]
      PmpPlugin_pmpcfg_11 <= 8'h0; // @[Data.scala 400:33]
      PmpPlugin_pmpcfg_12 <= 8'h0; // @[Data.scala 400:33]
      PmpPlugin_pmpcfg_13 <= 8'h0; // @[Data.scala 400:33]
      PmpPlugin_pmpcfg_14 <= 8'h0; // @[Data.scala 400:33]
      PmpPlugin_pmpcfg_15 <= 8'h0; // @[Data.scala 400:33]
      execute_PmpPlugin_fsmPending <= 1'b0; // @[Data.scala 400:33]
      execute_PmpPlugin_pmpcfgCsr_ <= 1'b0; // @[Data.scala 400:33]
      execute_PmpPlugin_pmpaddrCsr_ <= 1'b0; // @[Data.scala 400:33]
      execute_PmpPlugin_fsm_fsmEnable <= 1'b0; // @[Data.scala 400:33]
      execute_PmpPlugin_fsm_fsmCounter <= 4'b0000; // @[Data.scala 400:33]
      _zz_10 <= 1'b1; // @[Data.scala 400:33]
      HazardSimplePlugin_writeBackBuffer_valid <= 1'b0; // @[Data.scala 400:33]
      memory_MulDivIterativePlugin_mul_counter_value <= 6'h0; // @[Data.scala 400:33]
      memory_MulDivIterativePlugin_div_counter_value <= 6'h0; // @[Data.scala 400:33]
      _zz_CsrPlugin_privilege <= 2'b11; // @[Data.scala 400:33]
      CsrPlugin_misa_base <= 2'b01; // @[Data.scala 400:33]
      CsrPlugin_misa_extensions <= 26'h0101064; // @[Data.scala 400:33]
      CsrPlugin_mtvec_mode <= 2'b00; // @[Data.scala 400:33]
      CsrPlugin_mtvec_base <= 30'h00000008; // @[Data.scala 400:33]
      CsrPlugin_mstatus_MIE <= 1'b0; // @[Data.scala 400:33]
      CsrPlugin_mstatus_MPIE <= 1'b0; // @[Data.scala 400:33]
      CsrPlugin_mstatus_MPP <= 2'b11; // @[Data.scala 400:33]
      CsrPlugin_mie_MEIE <= 1'b0; // @[Data.scala 400:33]
      CsrPlugin_mie_MTIE <= 1'b0; // @[Data.scala 400:33]
      CsrPlugin_mie_MSIE <= 1'b0; // @[Data.scala 400:33]
      CsrPlugin_mcycle <= 64'h0; // @[Data.scala 400:33]
      CsrPlugin_minstret <= 64'h0; // @[Data.scala 400:33]
      CsrPlugin_exceptionPortCtrl_exceptionValidsRegs_decode <= 1'b0; // @[Data.scala 400:33]
      CsrPlugin_exceptionPortCtrl_exceptionValidsRegs_execute <= 1'b0; // @[Data.scala 400:33]
      CsrPlugin_exceptionPortCtrl_exceptionValidsRegs_memory <= 1'b0; // @[Data.scala 400:33]
      CsrPlugin_exceptionPortCtrl_exceptionValidsRegs_writeBack <= 1'b0; // @[Data.scala 400:33]
      CsrPlugin_interrupt_valid <= 1'b0; // @[Data.scala 400:33]
      CsrPlugin_lastStageWasWfi <= 1'b0; // @[Data.scala 400:33]
      CsrPlugin_pipelineLiberator_pcValids_0 <= 1'b0; // @[Data.scala 400:33]
      CsrPlugin_pipelineLiberator_pcValids_1 <= 1'b0; // @[Data.scala 400:33]
      CsrPlugin_pipelineLiberator_pcValids_2 <= 1'b0; // @[Data.scala 400:33]
      CsrPlugin_hadException <= 1'b0; // @[Data.scala 400:33]
      execute_CsrPlugin_wfiWake <= 1'b0; // @[Data.scala 400:33]
      execute_arbitration_isValid <= 1'b0; // @[Data.scala 400:33]
      memory_arbitration_isValid <= 1'b0; // @[Data.scala 400:33]
      writeBack_arbitration_isValid <= 1'b0; // @[Data.scala 400:33]
      switch_Fetcher_l365 <= 3'b000; // @[Data.scala 400:33]
      execute_PmpPlugin_fsm_stateReg <= execute_PmpPlugin_fsm_enumDef_BOOT; // @[Data.scala 400:33]
    end else begin
      if(IBusCachedPlugin_fetchPc_correction) begin
        IBusCachedPlugin_fetchPc_correctionReg <= 1'b1; // @[Fetcher.scala 130:42]
      end
      if(IBusCachedPlugin_fetchPc_output_fire) begin
        IBusCachedPlugin_fetchPc_correctionReg <= 1'b0; // @[Fetcher.scala 130:62]
      end
      IBusCachedPlugin_fetchPc_booted <= 1'b1; // @[Reg.scala 39:30]
      if(when_Fetcher_l134) begin
        IBusCachedPlugin_fetchPc_inc <= 1'b0; // @[Fetcher.scala 134:32]
      end
      if(IBusCachedPlugin_fetchPc_output_fire_1) begin
        IBusCachedPlugin_fetchPc_inc <= 1'b1; // @[Fetcher.scala 134:72]
      end
      if(when_Fetcher_l134_1) begin
        IBusCachedPlugin_fetchPc_inc <= 1'b0; // @[Fetcher.scala 134:93]
      end
      if(when_Fetcher_l161) begin
        IBusCachedPlugin_fetchPc_pcReg <= IBusCachedPlugin_fetchPc_pc; // @[Fetcher.scala 162:15]
      end
      if(IBusCachedPlugin_iBusRsp_flush) begin
        _zz_IBusCachedPlugin_iBusRsp_stages_1_input_valid_1 <= 1'b0; // @[Misc.scala 146:41]
      end
      if(_zz_IBusCachedPlugin_iBusRsp_stages_0_output_ready) begin
        _zz_IBusCachedPlugin_iBusRsp_stages_1_input_valid_1 <= (IBusCachedPlugin_iBusRsp_stages_0_output_valid && (! 1'b0)); // @[Misc.scala 154:18]
      end
      if(IBusCachedPlugin_iBusRsp_flush) begin
        _zz_IBusCachedPlugin_iBusRsp_stages_1_output_m2sPipe_valid <= 1'b0; // @[Misc.scala 146:41]
      end
      if(IBusCachedPlugin_iBusRsp_stages_1_output_ready) begin
        _zz_IBusCachedPlugin_iBusRsp_stages_1_output_m2sPipe_valid <= (IBusCachedPlugin_iBusRsp_stages_1_output_valid && (! IBusCachedPlugin_iBusRsp_flush)); // @[Misc.scala 154:18]
      end
      if(IBusCachedPlugin_fetchPc_flushed) begin
        IBusCachedPlugin_injector_nextPcCalc_valids_0 <= 1'b0; // @[Fetcher.scala 330:17]
      end
      if(when_Fetcher_l332) begin
        IBusCachedPlugin_injector_nextPcCalc_valids_0 <= 1'b1; // @[Fetcher.scala 333:17]
      end
      if(IBusCachedPlugin_fetchPc_flushed) begin
        IBusCachedPlugin_injector_nextPcCalc_valids_1 <= 1'b0; // @[Fetcher.scala 330:17]
      end
      if(when_Fetcher_l332_1) begin
        IBusCachedPlugin_injector_nextPcCalc_valids_1 <= IBusCachedPlugin_injector_nextPcCalc_valids_0; // @[Fetcher.scala 333:17]
      end
      if(IBusCachedPlugin_fetchPc_flushed) begin
        IBusCachedPlugin_injector_nextPcCalc_valids_1 <= 1'b0; // @[Fetcher.scala 336:17]
      end
      if(IBusCachedPlugin_fetchPc_flushed) begin
        IBusCachedPlugin_injector_nextPcCalc_valids_2 <= 1'b0; // @[Fetcher.scala 330:17]
      end
      if(when_Fetcher_l332_2) begin
        IBusCachedPlugin_injector_nextPcCalc_valids_2 <= IBusCachedPlugin_injector_nextPcCalc_valids_1; // @[Fetcher.scala 333:17]
      end
      if(IBusCachedPlugin_fetchPc_flushed) begin
        IBusCachedPlugin_injector_nextPcCalc_valids_2 <= 1'b0; // @[Fetcher.scala 336:17]
      end
      if(IBusCachedPlugin_fetchPc_flushed) begin
        IBusCachedPlugin_injector_nextPcCalc_valids_3 <= 1'b0; // @[Fetcher.scala 330:17]
      end
      if(when_Fetcher_l332_3) begin
        IBusCachedPlugin_injector_nextPcCalc_valids_3 <= IBusCachedPlugin_injector_nextPcCalc_valids_2; // @[Fetcher.scala 333:17]
      end
      if(IBusCachedPlugin_fetchPc_flushed) begin
        IBusCachedPlugin_injector_nextPcCalc_valids_3 <= 1'b0; // @[Fetcher.scala 336:17]
      end
      if(IBusCachedPlugin_fetchPc_flushed) begin
        IBusCachedPlugin_injector_nextPcCalc_valids_4 <= 1'b0; // @[Fetcher.scala 330:17]
      end
      if(when_Fetcher_l332_4) begin
        IBusCachedPlugin_injector_nextPcCalc_valids_4 <= IBusCachedPlugin_injector_nextPcCalc_valids_3; // @[Fetcher.scala 333:17]
      end
      if(IBusCachedPlugin_fetchPc_flushed) begin
        IBusCachedPlugin_injector_nextPcCalc_valids_4 <= 1'b0; // @[Fetcher.scala 336:17]
      end
      if(iBus_rsp_valid) begin
        IBusCachedPlugin_rspCounter <= (IBusCachedPlugin_rspCounter + 32'h00000001); // @[IBusCachedPlugin.scala 146:20]
      end
      if(dBus_rsp_valid) begin
        DBusCachedPlugin_rspCounter <= (DBusCachedPlugin_rspCounter + 32'h00000001); // @[DBusCachedPlugin.scala 301:20]
      end
      if(when_PmpPlugin_l138) begin
        execute_PmpPlugin_fsmPending <= 1'b0; // @[PmpPlugin.scala 138:39]
      end
      _zz_10 <= 1'b0; // @[Reg.scala 39:30]
      HazardSimplePlugin_writeBackBuffer_valid <= HazardSimplePlugin_writeBackWrites_valid; // @[Reg.scala 39:30]
      memory_MulDivIterativePlugin_mul_counter_value <= memory_MulDivIterativePlugin_mul_counter_valueNext; // @[Reg.scala 39:30]
      memory_MulDivIterativePlugin_div_counter_value <= memory_MulDivIterativePlugin_div_counter_valueNext; // @[Reg.scala 39:30]
      CsrPlugin_mcycle <= (CsrPlugin_mcycle + 64'h0000000000000001); // @[CsrPlugin.scala 1096:14]
      if(writeBack_arbitration_isFiring) begin
        CsrPlugin_minstret <= (CsrPlugin_minstret + 64'h0000000000000001); // @[CsrPlugin.scala 1098:18]
      end
      if(when_CsrPlugin_l1179) begin
        CsrPlugin_exceptionPortCtrl_exceptionValidsRegs_decode <= 1'b0; // @[CsrPlugin.scala 1180:42]
      end else begin
        CsrPlugin_exceptionPortCtrl_exceptionValidsRegs_decode <= CsrPlugin_exceptionPortCtrl_exceptionValids_decode; // @[CsrPlugin.scala 1183:44]
      end
      if(when_CsrPlugin_l1179_1) begin
        CsrPlugin_exceptionPortCtrl_exceptionValidsRegs_execute <= (CsrPlugin_exceptionPortCtrl_exceptionValids_decode && (! decode_arbitration_isStuck)); // @[CsrPlugin.scala 1180:42]
      end else begin
        CsrPlugin_exceptionPortCtrl_exceptionValidsRegs_execute <= CsrPlugin_exceptionPortCtrl_exceptionValids_execute; // @[CsrPlugin.scala 1183:44]
      end
      if(when_CsrPlugin_l1179_2) begin
        CsrPlugin_exceptionPortCtrl_exceptionValidsRegs_memory <= (CsrPlugin_exceptionPortCtrl_exceptionValids_execute && (! execute_arbitration_isStuck)); // @[CsrPlugin.scala 1180:42]
      end else begin
        CsrPlugin_exceptionPortCtrl_exceptionValidsRegs_memory <= CsrPlugin_exceptionPortCtrl_exceptionValids_memory; // @[CsrPlugin.scala 1183:44]
      end
      if(when_CsrPlugin_l1179_3) begin
        CsrPlugin_exceptionPortCtrl_exceptionValidsRegs_writeBack <= (CsrPlugin_exceptionPortCtrl_exceptionValids_memory && (! memory_arbitration_isStuck)); // @[CsrPlugin.scala 1180:42]
      end else begin
        CsrPlugin_exceptionPortCtrl_exceptionValidsRegs_writeBack <= 1'b0; // @[CsrPlugin.scala 1185:44]
      end
      CsrPlugin_interrupt_valid <= 1'b0; // @[Reg.scala 39:30]
      if(when_CsrPlugin_l1216) begin
        if(when_CsrPlugin_l1222) begin
          CsrPlugin_interrupt_valid <= 1'b1; // @[CsrPlugin.scala 1223:23]
        end
        if(when_CsrPlugin_l1222_1) begin
          CsrPlugin_interrupt_valid <= 1'b1; // @[CsrPlugin.scala 1223:23]
        end
        if(when_CsrPlugin_l1222_2) begin
          CsrPlugin_interrupt_valid <= 1'b1; // @[CsrPlugin.scala 1223:23]
        end
      end
      CsrPlugin_lastStageWasWfi <= (writeBack_arbitration_isFiring && (writeBack_ENV_CTRL == EnvCtrlEnum_WFI)); // @[Reg.scala 39:30]
      if(CsrPlugin_pipelineLiberator_active) begin
        if(when_CsrPlugin_l1255) begin
          CsrPlugin_pipelineLiberator_pcValids_0 <= 1'b1; // @[CsrPlugin.scala 1256:19]
        end
        if(when_CsrPlugin_l1255_1) begin
          CsrPlugin_pipelineLiberator_pcValids_1 <= CsrPlugin_pipelineLiberator_pcValids_0; // @[CsrPlugin.scala 1256:19]
        end
        if(when_CsrPlugin_l1255_2) begin
          CsrPlugin_pipelineLiberator_pcValids_2 <= CsrPlugin_pipelineLiberator_pcValids_1; // @[CsrPlugin.scala 1256:19]
        end
      end
      if(when_CsrPlugin_l1260) begin
        CsrPlugin_pipelineLiberator_pcValids_0 <= 1'b0; // @[CsrPlugin.scala 1261:30]
        CsrPlugin_pipelineLiberator_pcValids_1 <= 1'b0; // @[CsrPlugin.scala 1261:30]
        CsrPlugin_pipelineLiberator_pcValids_2 <= 1'b0; // @[CsrPlugin.scala 1261:30]
      end
      if(CsrPlugin_interruptJump) begin
        CsrPlugin_interrupt_valid <= 1'b0; // @[CsrPlugin.scala 1272:46]
      end
      CsrPlugin_hadException <= CsrPlugin_exception; // @[Reg.scala 39:30]
      if(when_CsrPlugin_l1310) begin
        if(when_CsrPlugin_l1318) begin
          _zz_CsrPlugin_privilege <= CsrPlugin_targetPrivilege; // @[CsrPlugin.scala 1319:41]
          case(CsrPlugin_targetPrivilege)
            2'b11 : begin
              CsrPlugin_mstatus_MIE <= 1'b0; // @[CsrPlugin.scala 1334:28]
              CsrPlugin_mstatus_MPIE <= CsrPlugin_mstatus_MIE; // @[CsrPlugin.scala 1335:28]
              CsrPlugin_mstatus_MPP <= CsrPlugin_privilege; // @[CsrPlugin.scala 1336:28]
            end
            default : begin
            end
          endcase
        end
      end
      if(when_CsrPlugin_l1376) begin
        case(switch_CsrPlugin_l1380)
          2'b11 : begin
            CsrPlugin_mstatus_MPP <= 2'b00; // @[CsrPlugin.scala 1382:27]
            CsrPlugin_mstatus_MIE <= CsrPlugin_mstatus_MPIE; // @[CsrPlugin.scala 1383:27]
            CsrPlugin_mstatus_MPIE <= 1'b1; // @[CsrPlugin.scala 1384:28]
            _zz_CsrPlugin_privilege <= CsrPlugin_mstatus_MPP; // @[CsrPlugin.scala 1387:30]
          end
          default : begin
          end
        endcase
      end
      execute_CsrPlugin_wfiWake <= (({_zz_when_CsrPlugin_l1222_2,{_zz_when_CsrPlugin_l1222_1,_zz_when_CsrPlugin_l1222}} != 3'b000) || CsrPlugin_thirdPartyWake); // @[Reg.scala 39:30]
      if(when_Pipeline_l151) begin
        execute_arbitration_isValid <= 1'b0; // @[Pipeline.scala 152:35]
      end
      if(when_Pipeline_l154) begin
        execute_arbitration_isValid <= decode_arbitration_isValid; // @[Pipeline.scala 155:35]
      end
      if(when_Pipeline_l151_1) begin
        memory_arbitration_isValid <= 1'b0; // @[Pipeline.scala 152:35]
      end
      if(when_Pipeline_l154_1) begin
        memory_arbitration_isValid <= execute_arbitration_isValid; // @[Pipeline.scala 155:35]
      end
      if(when_Pipeline_l151_2) begin
        writeBack_arbitration_isValid <= 1'b0; // @[Pipeline.scala 152:35]
      end
      if(when_Pipeline_l154_2) begin
        writeBack_arbitration_isValid <= memory_arbitration_isValid; // @[Pipeline.scala 155:35]
      end
      case(switch_Fetcher_l365)
        3'b000 : begin
          if(IBusCachedPlugin_injectionPort_valid) begin
            switch_Fetcher_l365 <= 3'b001; // @[Fetcher.scala 368:23]
          end
        end
        3'b001 : begin
          switch_Fetcher_l365 <= 3'b010; // @[Fetcher.scala 372:21]
        end
        3'b010 : begin
          switch_Fetcher_l365 <= 3'b011; // @[Fetcher.scala 377:21]
        end
        3'b011 : begin
          if(when_Fetcher_l381) begin
            switch_Fetcher_l365 <= 3'b100; // @[Fetcher.scala 382:23]
          end
        end
        3'b100 : begin
          switch_Fetcher_l365 <= 3'b000; // @[Fetcher.scala 387:21]
        end
        default : begin
        end
      endcase
      execute_PmpPlugin_fsm_stateReg <= execute_PmpPlugin_fsm_stateNext; // @[StateMachine.scala 212:14]
      case(execute_PmpPlugin_fsm_stateReg)
        execute_PmpPlugin_fsm_enumDef_stateIdle : begin
        end
        execute_PmpPlugin_fsm_enumDef_stateWrite : begin
          if(execute_PmpPlugin_pmpcfgCsr_) begin
            if(when_PmpPlugin_l209) begin
              if(_zz_11[0]) begin
                PmpPlugin_pmpcfg_0 <= _zz_PmpPlugin_pmpcfg_0; // @[Bits.scala 133:56]
              end
              if(_zz_11[1]) begin
                PmpPlugin_pmpcfg_1 <= _zz_PmpPlugin_pmpcfg_0; // @[Bits.scala 133:56]
              end
              if(_zz_11[2]) begin
                PmpPlugin_pmpcfg_2 <= _zz_PmpPlugin_pmpcfg_0; // @[Bits.scala 133:56]
              end
              if(_zz_11[3]) begin
                PmpPlugin_pmpcfg_3 <= _zz_PmpPlugin_pmpcfg_0; // @[Bits.scala 133:56]
              end
              if(_zz_11[4]) begin
                PmpPlugin_pmpcfg_4 <= _zz_PmpPlugin_pmpcfg_0; // @[Bits.scala 133:56]
              end
              if(_zz_11[5]) begin
                PmpPlugin_pmpcfg_5 <= _zz_PmpPlugin_pmpcfg_0; // @[Bits.scala 133:56]
              end
              if(_zz_11[6]) begin
                PmpPlugin_pmpcfg_6 <= _zz_PmpPlugin_pmpcfg_0; // @[Bits.scala 133:56]
              end
              if(_zz_11[7]) begin
                PmpPlugin_pmpcfg_7 <= _zz_PmpPlugin_pmpcfg_0; // @[Bits.scala 133:56]
              end
              if(_zz_11[8]) begin
                PmpPlugin_pmpcfg_8 <= _zz_PmpPlugin_pmpcfg_0; // @[Bits.scala 133:56]
              end
              if(_zz_11[9]) begin
                PmpPlugin_pmpcfg_9 <= _zz_PmpPlugin_pmpcfg_0; // @[Bits.scala 133:56]
              end
              if(_zz_11[10]) begin
                PmpPlugin_pmpcfg_10 <= _zz_PmpPlugin_pmpcfg_0; // @[Bits.scala 133:56]
              end
              if(_zz_11[11]) begin
                PmpPlugin_pmpcfg_11 <= _zz_PmpPlugin_pmpcfg_0; // @[Bits.scala 133:56]
              end
              if(_zz_11[12]) begin
                PmpPlugin_pmpcfg_12 <= _zz_PmpPlugin_pmpcfg_0; // @[Bits.scala 133:56]
              end
              if(_zz_11[13]) begin
                PmpPlugin_pmpcfg_13 <= _zz_PmpPlugin_pmpcfg_0; // @[Bits.scala 133:56]
              end
              if(_zz_11[14]) begin
                PmpPlugin_pmpcfg_14 <= _zz_PmpPlugin_pmpcfg_0; // @[Bits.scala 133:56]
              end
              if(_zz_11[15]) begin
                PmpPlugin_pmpcfg_15 <= _zz_PmpPlugin_pmpcfg_0; // @[Bits.scala 133:56]
              end
            end
            if(when_PmpPlugin_l209_1) begin
              if(_zz_12[0]) begin
                PmpPlugin_pmpcfg_0 <= _zz_PmpPlugin_pmpcfg_0_1; // @[Bits.scala 133:56]
              end
              if(_zz_12[1]) begin
                PmpPlugin_pmpcfg_1 <= _zz_PmpPlugin_pmpcfg_0_1; // @[Bits.scala 133:56]
              end
              if(_zz_12[2]) begin
                PmpPlugin_pmpcfg_2 <= _zz_PmpPlugin_pmpcfg_0_1; // @[Bits.scala 133:56]
              end
              if(_zz_12[3]) begin
                PmpPlugin_pmpcfg_3 <= _zz_PmpPlugin_pmpcfg_0_1; // @[Bits.scala 133:56]
              end
              if(_zz_12[4]) begin
                PmpPlugin_pmpcfg_4 <= _zz_PmpPlugin_pmpcfg_0_1; // @[Bits.scala 133:56]
              end
              if(_zz_12[5]) begin
                PmpPlugin_pmpcfg_5 <= _zz_PmpPlugin_pmpcfg_0_1; // @[Bits.scala 133:56]
              end
              if(_zz_12[6]) begin
                PmpPlugin_pmpcfg_6 <= _zz_PmpPlugin_pmpcfg_0_1; // @[Bits.scala 133:56]
              end
              if(_zz_12[7]) begin
                PmpPlugin_pmpcfg_7 <= _zz_PmpPlugin_pmpcfg_0_1; // @[Bits.scala 133:56]
              end
              if(_zz_12[8]) begin
                PmpPlugin_pmpcfg_8 <= _zz_PmpPlugin_pmpcfg_0_1; // @[Bits.scala 133:56]
              end
              if(_zz_12[9]) begin
                PmpPlugin_pmpcfg_9 <= _zz_PmpPlugin_pmpcfg_0_1; // @[Bits.scala 133:56]
              end
              if(_zz_12[10]) begin
                PmpPlugin_pmpcfg_10 <= _zz_PmpPlugin_pmpcfg_0_1; // @[Bits.scala 133:56]
              end
              if(_zz_12[11]) begin
                PmpPlugin_pmpcfg_11 <= _zz_PmpPlugin_pmpcfg_0_1; // @[Bits.scala 133:56]
              end
              if(_zz_12[12]) begin
                PmpPlugin_pmpcfg_12 <= _zz_PmpPlugin_pmpcfg_0_1; // @[Bits.scala 133:56]
              end
              if(_zz_12[13]) begin
                PmpPlugin_pmpcfg_13 <= _zz_PmpPlugin_pmpcfg_0_1; // @[Bits.scala 133:56]
              end
              if(_zz_12[14]) begin
                PmpPlugin_pmpcfg_14 <= _zz_PmpPlugin_pmpcfg_0_1; // @[Bits.scala 133:56]
              end
              if(_zz_12[15]) begin
                PmpPlugin_pmpcfg_15 <= _zz_PmpPlugin_pmpcfg_0_1; // @[Bits.scala 133:56]
              end
            end
            if(when_PmpPlugin_l209_2) begin
              if(_zz_13[0]) begin
                PmpPlugin_pmpcfg_0 <= _zz_PmpPlugin_pmpcfg_0_2; // @[Bits.scala 133:56]
              end
              if(_zz_13[1]) begin
                PmpPlugin_pmpcfg_1 <= _zz_PmpPlugin_pmpcfg_0_2; // @[Bits.scala 133:56]
              end
              if(_zz_13[2]) begin
                PmpPlugin_pmpcfg_2 <= _zz_PmpPlugin_pmpcfg_0_2; // @[Bits.scala 133:56]
              end
              if(_zz_13[3]) begin
                PmpPlugin_pmpcfg_3 <= _zz_PmpPlugin_pmpcfg_0_2; // @[Bits.scala 133:56]
              end
              if(_zz_13[4]) begin
                PmpPlugin_pmpcfg_4 <= _zz_PmpPlugin_pmpcfg_0_2; // @[Bits.scala 133:56]
              end
              if(_zz_13[5]) begin
                PmpPlugin_pmpcfg_5 <= _zz_PmpPlugin_pmpcfg_0_2; // @[Bits.scala 133:56]
              end
              if(_zz_13[6]) begin
                PmpPlugin_pmpcfg_6 <= _zz_PmpPlugin_pmpcfg_0_2; // @[Bits.scala 133:56]
              end
              if(_zz_13[7]) begin
                PmpPlugin_pmpcfg_7 <= _zz_PmpPlugin_pmpcfg_0_2; // @[Bits.scala 133:56]
              end
              if(_zz_13[8]) begin
                PmpPlugin_pmpcfg_8 <= _zz_PmpPlugin_pmpcfg_0_2; // @[Bits.scala 133:56]
              end
              if(_zz_13[9]) begin
                PmpPlugin_pmpcfg_9 <= _zz_PmpPlugin_pmpcfg_0_2; // @[Bits.scala 133:56]
              end
              if(_zz_13[10]) begin
                PmpPlugin_pmpcfg_10 <= _zz_PmpPlugin_pmpcfg_0_2; // @[Bits.scala 133:56]
              end
              if(_zz_13[11]) begin
                PmpPlugin_pmpcfg_11 <= _zz_PmpPlugin_pmpcfg_0_2; // @[Bits.scala 133:56]
              end
              if(_zz_13[12]) begin
                PmpPlugin_pmpcfg_12 <= _zz_PmpPlugin_pmpcfg_0_2; // @[Bits.scala 133:56]
              end
              if(_zz_13[13]) begin
                PmpPlugin_pmpcfg_13 <= _zz_PmpPlugin_pmpcfg_0_2; // @[Bits.scala 133:56]
              end
              if(_zz_13[14]) begin
                PmpPlugin_pmpcfg_14 <= _zz_PmpPlugin_pmpcfg_0_2; // @[Bits.scala 133:56]
              end
              if(_zz_13[15]) begin
                PmpPlugin_pmpcfg_15 <= _zz_PmpPlugin_pmpcfg_0_2; // @[Bits.scala 133:56]
              end
            end
            if(when_PmpPlugin_l209_3) begin
              if(_zz_14[0]) begin
                PmpPlugin_pmpcfg_0 <= _zz_PmpPlugin_pmpcfg_0_3; // @[Bits.scala 133:56]
              end
              if(_zz_14[1]) begin
                PmpPlugin_pmpcfg_1 <= _zz_PmpPlugin_pmpcfg_0_3; // @[Bits.scala 133:56]
              end
              if(_zz_14[2]) begin
                PmpPlugin_pmpcfg_2 <= _zz_PmpPlugin_pmpcfg_0_3; // @[Bits.scala 133:56]
              end
              if(_zz_14[3]) begin
                PmpPlugin_pmpcfg_3 <= _zz_PmpPlugin_pmpcfg_0_3; // @[Bits.scala 133:56]
              end
              if(_zz_14[4]) begin
                PmpPlugin_pmpcfg_4 <= _zz_PmpPlugin_pmpcfg_0_3; // @[Bits.scala 133:56]
              end
              if(_zz_14[5]) begin
                PmpPlugin_pmpcfg_5 <= _zz_PmpPlugin_pmpcfg_0_3; // @[Bits.scala 133:56]
              end
              if(_zz_14[6]) begin
                PmpPlugin_pmpcfg_6 <= _zz_PmpPlugin_pmpcfg_0_3; // @[Bits.scala 133:56]
              end
              if(_zz_14[7]) begin
                PmpPlugin_pmpcfg_7 <= _zz_PmpPlugin_pmpcfg_0_3; // @[Bits.scala 133:56]
              end
              if(_zz_14[8]) begin
                PmpPlugin_pmpcfg_8 <= _zz_PmpPlugin_pmpcfg_0_3; // @[Bits.scala 133:56]
              end
              if(_zz_14[9]) begin
                PmpPlugin_pmpcfg_9 <= _zz_PmpPlugin_pmpcfg_0_3; // @[Bits.scala 133:56]
              end
              if(_zz_14[10]) begin
                PmpPlugin_pmpcfg_10 <= _zz_PmpPlugin_pmpcfg_0_3; // @[Bits.scala 133:56]
              end
              if(_zz_14[11]) begin
                PmpPlugin_pmpcfg_11 <= _zz_PmpPlugin_pmpcfg_0_3; // @[Bits.scala 133:56]
              end
              if(_zz_14[12]) begin
                PmpPlugin_pmpcfg_12 <= _zz_PmpPlugin_pmpcfg_0_3; // @[Bits.scala 133:56]
              end
              if(_zz_14[13]) begin
                PmpPlugin_pmpcfg_13 <= _zz_PmpPlugin_pmpcfg_0_3; // @[Bits.scala 133:56]
              end
              if(_zz_14[14]) begin
                PmpPlugin_pmpcfg_14 <= _zz_PmpPlugin_pmpcfg_0_3; // @[Bits.scala 133:56]
              end
              if(_zz_14[15]) begin
                PmpPlugin_pmpcfg_15 <= _zz_PmpPlugin_pmpcfg_0_3; // @[Bits.scala 133:56]
              end
            end
          end
        end
        execute_PmpPlugin_fsm_enumDef_stateCfg : begin
          execute_PmpPlugin_fsm_fsmCounter <= (execute_PmpPlugin_fsm_fsmCounter + 4'b0001); // @[PmpPlugin.scala 228:24]
        end
        execute_PmpPlugin_fsm_enumDef_stateAddr : begin
        end
        default : begin
        end
      endcase
      if(when_StateMachine_l237) begin
        execute_PmpPlugin_fsm_fsmEnable <= 1'b1; // @[PmpPlugin.scala 222:29]
      end
      if(when_StateMachine_l253) begin
        execute_PmpPlugin_fsmPending <= 1'b0; // @[PmpPlugin.scala 192:24]
        execute_PmpPlugin_fsm_fsmEnable <= 1'b0; // @[PmpPlugin.scala 193:23]
        execute_PmpPlugin_fsm_fsmCounter <= 4'b0000; // @[PmpPlugin.scala 195:24]
      end
      if(when_StateMachine_l253_1) begin
        execute_PmpPlugin_fsm_fsmCounter <= {execute_PmpPlugin_pmpcfgN_,2'b00}; // @[PmpPlugin.scala 226:31]
      end
      if(when_StateMachine_l253_2) begin
        execute_PmpPlugin_fsm_fsmCounter <= execute_PmpPlugin_pmpNcfg_; // @[PmpPlugin.scala 236:31]
      end
      if(execute_CsrPlugin_csr_769) begin
        if(execute_CsrPlugin_writeEnable) begin
          CsrPlugin_misa_base <= CsrPlugin_csrMapping_writeDataSignal[31 : 30]; // @[UInt.scala 381:56]
          CsrPlugin_misa_extensions <= CsrPlugin_csrMapping_writeDataSignal[25 : 0]; // @[Bits.scala 133:56]
        end
      end
      if(execute_CsrPlugin_csr_768) begin
        if(execute_CsrPlugin_writeEnable) begin
          CsrPlugin_mstatus_MPIE <= CsrPlugin_csrMapping_writeDataSignal[7]; // @[Bool.scala 189:10]
          CsrPlugin_mstatus_MIE <= CsrPlugin_csrMapping_writeDataSignal[3]; // @[Bool.scala 189:10]
          case(switch_CsrPlugin_l980)
            2'b11 : begin
              CsrPlugin_mstatus_MPP <= 2'b11; // @[CsrPlugin.scala 981:30]
            end
            2'b00 : begin
              CsrPlugin_mstatus_MPP <= 2'b00; // @[CsrPlugin.scala 983:42]
            end
            default : begin
            end
          endcase
        end
      end
      if(execute_CsrPlugin_csr_772) begin
        if(execute_CsrPlugin_writeEnable) begin
          CsrPlugin_mie_MEIE <= CsrPlugin_csrMapping_writeDataSignal[11]; // @[Bool.scala 189:10]
          CsrPlugin_mie_MTIE <= CsrPlugin_csrMapping_writeDataSignal[7]; // @[Bool.scala 189:10]
          CsrPlugin_mie_MSIE <= CsrPlugin_csrMapping_writeDataSignal[3]; // @[Bool.scala 189:10]
        end
      end
      if(execute_CsrPlugin_csr_773) begin
        if(execute_CsrPlugin_writeEnable) begin
          CsrPlugin_mtvec_base <= CsrPlugin_csrMapping_writeDataSignal[31 : 2]; // @[UInt.scala 381:56]
          CsrPlugin_mtvec_mode <= CsrPlugin_csrMapping_writeDataSignal[1 : 0]; // @[Bits.scala 133:56]
        end
      end
      if(execute_CsrPlugin_csr_2816) begin
        if(execute_CsrPlugin_writeEnable) begin
          CsrPlugin_mcycle[31 : 0] <= CsrPlugin_csrMapping_writeDataSignal[31 : 0]; // @[UInt.scala 381:56]
        end
      end
      if(execute_CsrPlugin_csr_2944) begin
        if(execute_CsrPlugin_writeEnable) begin
          CsrPlugin_mcycle[63 : 32] <= CsrPlugin_csrMapping_writeDataSignal[31 : 0]; // @[UInt.scala 381:56]
        end
      end
      if(execute_CsrPlugin_csr_2818) begin
        if(execute_CsrPlugin_writeEnable) begin
          CsrPlugin_minstret[31 : 0] <= CsrPlugin_csrMapping_writeDataSignal[31 : 0]; // @[UInt.scala 381:56]
        end
      end
      if(execute_CsrPlugin_csr_2946) begin
        if(execute_CsrPlugin_writeEnable) begin
          CsrPlugin_minstret[63 : 32] <= CsrPlugin_csrMapping_writeDataSignal[31 : 0]; // @[UInt.scala 381:56]
        end
      end
      if(execute_CsrPlugin_writeInstruction) begin
        if(when_PmpPlugin_l172) begin
          if(when_PmpPlugin_l175) begin
            execute_PmpPlugin_fsmPending <= 1'b1; // @[PmpPlugin.scala 176:24]
            execute_PmpPlugin_pmpcfgCsr_ <= execute_PmpPlugin_pmpcfgCsr; // @[PmpPlugin.scala 180:24]
            execute_PmpPlugin_pmpaddrCsr_ <= execute_PmpPlugin_pmpaddrCsr; // @[PmpPlugin.scala 181:25]
          end
        end
      end
    end
  end

  always @(posedge clk) begin
    if(IBusCachedPlugin_iBusRsp_stages_1_output_ready) begin
      _zz_IBusCachedPlugin_iBusRsp_stages_1_output_m2sPipe_payload <= IBusCachedPlugin_iBusRsp_stages_1_output_payload; // @[Misc.scala 155:15]
    end
    if(IBusCachedPlugin_iBusRsp_stages_1_input_ready) begin
      IBusCachedPlugin_s1_tightlyCoupledHit <= IBusCachedPlugin_s0_tightlyCoupledHit; // @[IBusCachedPlugin.scala 181:44]
    end
    if(IBusCachedPlugin_iBusRsp_stages_2_input_ready) begin
      IBusCachedPlugin_s2_tightlyCoupledHit <= IBusCachedPlugin_s1_tightlyCoupledHit; // @[IBusCachedPlugin.scala 207:44]
    end
    if(when_PmpPlugin_l246) begin
      if(_zz_8[0]) begin
        PmpPlugin_base_0 <= PmpPlugin_setter_io_base; // @[PmpPlugin.scala 247:34]
      end
      if(_zz_8[1]) begin
        PmpPlugin_base_1 <= PmpPlugin_setter_io_base; // @[PmpPlugin.scala 247:34]
      end
      if(_zz_8[2]) begin
        PmpPlugin_base_2 <= PmpPlugin_setter_io_base; // @[PmpPlugin.scala 247:34]
      end
      if(_zz_8[3]) begin
        PmpPlugin_base_3 <= PmpPlugin_setter_io_base; // @[PmpPlugin.scala 247:34]
      end
      if(_zz_8[4]) begin
        PmpPlugin_base_4 <= PmpPlugin_setter_io_base; // @[PmpPlugin.scala 247:34]
      end
      if(_zz_8[5]) begin
        PmpPlugin_base_5 <= PmpPlugin_setter_io_base; // @[PmpPlugin.scala 247:34]
      end
      if(_zz_8[6]) begin
        PmpPlugin_base_6 <= PmpPlugin_setter_io_base; // @[PmpPlugin.scala 247:34]
      end
      if(_zz_8[7]) begin
        PmpPlugin_base_7 <= PmpPlugin_setter_io_base; // @[PmpPlugin.scala 247:34]
      end
      if(_zz_8[8]) begin
        PmpPlugin_base_8 <= PmpPlugin_setter_io_base; // @[PmpPlugin.scala 247:34]
      end
      if(_zz_8[9]) begin
        PmpPlugin_base_9 <= PmpPlugin_setter_io_base; // @[PmpPlugin.scala 247:34]
      end
      if(_zz_8[10]) begin
        PmpPlugin_base_10 <= PmpPlugin_setter_io_base; // @[PmpPlugin.scala 247:34]
      end
      if(_zz_8[11]) begin
        PmpPlugin_base_11 <= PmpPlugin_setter_io_base; // @[PmpPlugin.scala 247:34]
      end
      if(_zz_8[12]) begin
        PmpPlugin_base_12 <= PmpPlugin_setter_io_base; // @[PmpPlugin.scala 247:34]
      end
      if(_zz_8[13]) begin
        PmpPlugin_base_13 <= PmpPlugin_setter_io_base; // @[PmpPlugin.scala 247:34]
      end
      if(_zz_8[14]) begin
        PmpPlugin_base_14 <= PmpPlugin_setter_io_base; // @[PmpPlugin.scala 247:34]
      end
      if(_zz_8[15]) begin
        PmpPlugin_base_15 <= PmpPlugin_setter_io_base; // @[PmpPlugin.scala 247:34]
      end
      if(_zz_9[0]) begin
        PmpPlugin_mask_0 <= PmpPlugin_setter_io_mask; // @[PmpPlugin.scala 248:34]
      end
      if(_zz_9[1]) begin
        PmpPlugin_mask_1 <= PmpPlugin_setter_io_mask; // @[PmpPlugin.scala 248:34]
      end
      if(_zz_9[2]) begin
        PmpPlugin_mask_2 <= PmpPlugin_setter_io_mask; // @[PmpPlugin.scala 248:34]
      end
      if(_zz_9[3]) begin
        PmpPlugin_mask_3 <= PmpPlugin_setter_io_mask; // @[PmpPlugin.scala 248:34]
      end
      if(_zz_9[4]) begin
        PmpPlugin_mask_4 <= PmpPlugin_setter_io_mask; // @[PmpPlugin.scala 248:34]
      end
      if(_zz_9[5]) begin
        PmpPlugin_mask_5 <= PmpPlugin_setter_io_mask; // @[PmpPlugin.scala 248:34]
      end
      if(_zz_9[6]) begin
        PmpPlugin_mask_6 <= PmpPlugin_setter_io_mask; // @[PmpPlugin.scala 248:34]
      end
      if(_zz_9[7]) begin
        PmpPlugin_mask_7 <= PmpPlugin_setter_io_mask; // @[PmpPlugin.scala 248:34]
      end
      if(_zz_9[8]) begin
        PmpPlugin_mask_8 <= PmpPlugin_setter_io_mask; // @[PmpPlugin.scala 248:34]
      end
      if(_zz_9[9]) begin
        PmpPlugin_mask_9 <= PmpPlugin_setter_io_mask; // @[PmpPlugin.scala 248:34]
      end
      if(_zz_9[10]) begin
        PmpPlugin_mask_10 <= PmpPlugin_setter_io_mask; // @[PmpPlugin.scala 248:34]
      end
      if(_zz_9[11]) begin
        PmpPlugin_mask_11 <= PmpPlugin_setter_io_mask; // @[PmpPlugin.scala 248:34]
      end
      if(_zz_9[12]) begin
        PmpPlugin_mask_12 <= PmpPlugin_setter_io_mask; // @[PmpPlugin.scala 248:34]
      end
      if(_zz_9[13]) begin
        PmpPlugin_mask_13 <= PmpPlugin_setter_io_mask; // @[PmpPlugin.scala 248:34]
      end
      if(_zz_9[14]) begin
        PmpPlugin_mask_14 <= PmpPlugin_setter_io_mask; // @[PmpPlugin.scala 248:34]
      end
      if(_zz_9[15]) begin
        PmpPlugin_mask_15 <= PmpPlugin_setter_io_mask; // @[PmpPlugin.scala 248:34]
      end
    end
    HazardSimplePlugin_writeBackBuffer_payload_address <= HazardSimplePlugin_writeBackWrites_payload_address; // @[Reg.scala 39:30]
    HazardSimplePlugin_writeBackBuffer_payload_data <= HazardSimplePlugin_writeBackWrites_payload_data; // @[Reg.scala 39:30]
    if(when_MulDivIterativePlugin_l96) begin
      if(when_MulDivIterativePlugin_l100) begin
        memory_MulDivIterativePlugin_rs2 <= (memory_MulDivIterativePlugin_rs2 >>> 1); // @[MulDivIterativePlugin.scala 103:17]
        memory_MulDivIterativePlugin_accumulator <= ({_zz_memory_MulDivIterativePlugin_accumulator,memory_MulDivIterativePlugin_accumulator[31 : 0]} >>> 1); // @[MulDivIterativePlugin.scala 106:25]
      end
    end
    if(when_MulDivIterativePlugin_l126) begin
      memory_MulDivIterativePlugin_div_done <= 1'b1; // @[MulDivIterativePlugin.scala 126:30]
    end
    if(when_MulDivIterativePlugin_l126_1) begin
      memory_MulDivIterativePlugin_div_done <= 1'b0; // @[MulDivIterativePlugin.scala 126:65]
    end
    if(when_MulDivIterativePlugin_l128) begin
      if(when_MulDivIterativePlugin_l132) begin
        memory_MulDivIterativePlugin_rs1[31 : 0] <= memory_MulDivIterativePlugin_div_stage_0_outNumerator; // @[MulDivIterativePlugin.scala 137:27]
        memory_MulDivIterativePlugin_accumulator[31 : 0] <= memory_MulDivIterativePlugin_div_stage_0_outRemainder; // @[MulDivIterativePlugin.scala 138:27]
        if(when_MulDivIterativePlugin_l151) begin
          memory_MulDivIterativePlugin_div_result <= _zz_memory_MulDivIterativePlugin_div_result_1[31:0]; // @[MulDivIterativePlugin.scala 153:22]
        end
      end
    end
    if(when_MulDivIterativePlugin_l162) begin
      memory_MulDivIterativePlugin_accumulator <= 65'h0; // @[MulDivIterativePlugin.scala 163:21]
      memory_MulDivIterativePlugin_rs1 <= ((_zz_memory_MulDivIterativePlugin_rs1 ? (~ _zz_memory_MulDivIterativePlugin_rs1_1) : _zz_memory_MulDivIterativePlugin_rs1_1) + _zz_memory_MulDivIterativePlugin_rs1_2); // @[MulDivIterativePlugin.scala 170:13]
      memory_MulDivIterativePlugin_rs2 <= ((_zz_memory_MulDivIterativePlugin_rs2 ? (~ execute_RS2) : execute_RS2) + _zz_memory_MulDivIterativePlugin_rs2_1); // @[MulDivIterativePlugin.scala 171:13]
      memory_MulDivIterativePlugin_div_needRevert <= ((_zz_memory_MulDivIterativePlugin_rs1 ^ (_zz_memory_MulDivIterativePlugin_rs2 && (! execute_INSTRUCTION[13]))) && (! (((execute_RS2 == 32'h0) && execute_IS_RS2_SIGNED) && (! execute_INSTRUCTION[13])))); // @[MulDivIterativePlugin.scala 172:35]
    end
    CsrPlugin_mip_MEIP <= externalInterrupt; // @[Reg.scala 39:30]
    CsrPlugin_mip_MTIP <= timerInterrupt; // @[Reg.scala 39:30]
    CsrPlugin_mip_MSIP <= softwareInterrupt; // @[Reg.scala 39:30]
    if(_zz_when) begin
      CsrPlugin_exceptionPortCtrl_exceptionContext_code <= (_zz_CsrPlugin_exceptionPortCtrl_exceptionContext_code_1 ? IBusCachedPlugin_decodeExceptionPort_payload_code : decodeExceptionPort_payload_code); // @[CsrPlugin.scala 1173:30]
      CsrPlugin_exceptionPortCtrl_exceptionContext_badAddr <= (_zz_CsrPlugin_exceptionPortCtrl_exceptionContext_code_1 ? IBusCachedPlugin_decodeExceptionPort_payload_badAddr : decodeExceptionPort_payload_badAddr); // @[CsrPlugin.scala 1173:30]
    end
    if(CsrPlugin_selfException_valid) begin
      CsrPlugin_exceptionPortCtrl_exceptionContext_code <= CsrPlugin_selfException_payload_code; // @[CsrPlugin.scala 1173:30]
      CsrPlugin_exceptionPortCtrl_exceptionContext_badAddr <= CsrPlugin_selfException_payload_badAddr; // @[CsrPlugin.scala 1173:30]
    end
    if(BranchPlugin_branchExceptionPort_valid) begin
      CsrPlugin_exceptionPortCtrl_exceptionContext_code <= BranchPlugin_branchExceptionPort_payload_code; // @[CsrPlugin.scala 1173:30]
      CsrPlugin_exceptionPortCtrl_exceptionContext_badAddr <= BranchPlugin_branchExceptionPort_payload_badAddr; // @[CsrPlugin.scala 1173:30]
    end
    if(DBusCachedPlugin_exceptionBus_valid) begin
      CsrPlugin_exceptionPortCtrl_exceptionContext_code <= DBusCachedPlugin_exceptionBus_payload_code; // @[CsrPlugin.scala 1173:30]
      CsrPlugin_exceptionPortCtrl_exceptionContext_badAddr <= DBusCachedPlugin_exceptionBus_payload_badAddr; // @[CsrPlugin.scala 1173:30]
    end
    if(when_CsrPlugin_l1216) begin
      if(when_CsrPlugin_l1222) begin
        CsrPlugin_interrupt_code <= 4'b0111; // @[CsrPlugin.scala 1224:22]
        CsrPlugin_interrupt_targetPrivilege <= 2'b11; // @[CsrPlugin.scala 1225:33]
      end
      if(when_CsrPlugin_l1222_1) begin
        CsrPlugin_interrupt_code <= 4'b0011; // @[CsrPlugin.scala 1224:22]
        CsrPlugin_interrupt_targetPrivilege <= 2'b11; // @[CsrPlugin.scala 1225:33]
      end
      if(when_CsrPlugin_l1222_2) begin
        CsrPlugin_interrupt_code <= 4'b1011; // @[CsrPlugin.scala 1224:22]
        CsrPlugin_interrupt_targetPrivilege <= 2'b11; // @[CsrPlugin.scala 1225:33]
      end
    end
    if(when_CsrPlugin_l1310) begin
      if(when_CsrPlugin_l1318) begin
        case(CsrPlugin_targetPrivilege)
          2'b11 : begin
            CsrPlugin_mcause_interrupt <= (! CsrPlugin_hadException); // @[CsrPlugin.scala 1337:32]
            CsrPlugin_mcause_exceptionCode <= CsrPlugin_trapCause; // @[CsrPlugin.scala 1338:36]
            CsrPlugin_mepc <= writeBack_PC; // @[CsrPlugin.scala 1339:20]
            if(CsrPlugin_hadException) begin
              CsrPlugin_mtval <= CsrPlugin_exceptionPortCtrl_exceptionContext_badAddr; // @[CsrPlugin.scala 1341:23]
            end
          end
          default : begin
          end
        endcase
      end
    end
    if(when_Pipeline_l124) begin
      decode_to_execute_PC <= decode_PC; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_1) begin
      execute_to_memory_PC <= _zz_execute_to_memory_PC; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_2) begin
      memory_to_writeBack_PC <= memory_PC; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_3) begin
      decode_to_execute_INSTRUCTION <= decode_INSTRUCTION; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_4) begin
      execute_to_memory_INSTRUCTION <= execute_INSTRUCTION; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_5) begin
      memory_to_writeBack_INSTRUCTION <= memory_INSTRUCTION; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_6) begin
      decode_to_execute_FORMAL_PC_NEXT <= _zz_decode_to_execute_FORMAL_PC_NEXT; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_7) begin
      execute_to_memory_FORMAL_PC_NEXT <= execute_FORMAL_PC_NEXT; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_8) begin
      memory_to_writeBack_FORMAL_PC_NEXT <= _zz_memory_to_writeBack_FORMAL_PC_NEXT; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_9) begin
      decode_to_execute_MEMORY_FORCE_CONSTISTENCY <= decode_MEMORY_FORCE_CONSTISTENCY; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_10) begin
      decode_to_execute_SRC1_CTRL <= _zz_decode_to_execute_SRC1_CTRL; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_11) begin
      decode_to_execute_SRC_USE_SUB_LESS <= decode_SRC_USE_SUB_LESS; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_12) begin
      decode_to_execute_MEMORY_ENABLE <= decode_MEMORY_ENABLE; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_13) begin
      execute_to_memory_MEMORY_ENABLE <= execute_MEMORY_ENABLE; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_14) begin
      memory_to_writeBack_MEMORY_ENABLE <= memory_MEMORY_ENABLE; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_15) begin
      decode_to_execute_ALU_CTRL <= _zz_decode_to_execute_ALU_CTRL; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_16) begin
      decode_to_execute_SRC2_CTRL <= _zz_decode_to_execute_SRC2_CTRL; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_17) begin
      decode_to_execute_REGFILE_WRITE_VALID <= decode_REGFILE_WRITE_VALID; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_18) begin
      execute_to_memory_REGFILE_WRITE_VALID <= execute_REGFILE_WRITE_VALID; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_19) begin
      memory_to_writeBack_REGFILE_WRITE_VALID <= memory_REGFILE_WRITE_VALID; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_20) begin
      decode_to_execute_BYPASSABLE_EXECUTE_STAGE <= decode_BYPASSABLE_EXECUTE_STAGE; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_21) begin
      decode_to_execute_BYPASSABLE_MEMORY_STAGE <= decode_BYPASSABLE_MEMORY_STAGE; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_22) begin
      execute_to_memory_BYPASSABLE_MEMORY_STAGE <= execute_BYPASSABLE_MEMORY_STAGE; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_23) begin
      decode_to_execute_MEMORY_WR <= decode_MEMORY_WR; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_24) begin
      execute_to_memory_MEMORY_WR <= execute_MEMORY_WR; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_25) begin
      memory_to_writeBack_MEMORY_WR <= memory_MEMORY_WR; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_26) begin
      decode_to_execute_MEMORY_MANAGMENT <= decode_MEMORY_MANAGMENT; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_27) begin
      decode_to_execute_SRC_LESS_UNSIGNED <= decode_SRC_LESS_UNSIGNED; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_28) begin
      decode_to_execute_ALU_BITWISE_CTRL <= _zz_decode_to_execute_ALU_BITWISE_CTRL; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_29) begin
      decode_to_execute_SHIFT_CTRL <= _zz_decode_to_execute_SHIFT_CTRL; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_30) begin
      execute_to_memory_SHIFT_CTRL <= _zz_execute_to_memory_SHIFT_CTRL; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_31) begin
      decode_to_execute_IS_MUL <= decode_IS_MUL; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_32) begin
      execute_to_memory_IS_MUL <= execute_IS_MUL; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_33) begin
      decode_to_execute_IS_RS1_SIGNED <= decode_IS_RS1_SIGNED; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_34) begin
      decode_to_execute_IS_RS2_SIGNED <= decode_IS_RS2_SIGNED; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_35) begin
      decode_to_execute_IS_DIV <= decode_IS_DIV; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_36) begin
      execute_to_memory_IS_DIV <= execute_IS_DIV; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_37) begin
      decode_to_execute_IS_CSR <= decode_IS_CSR; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_38) begin
      decode_to_execute_ENV_CTRL <= _zz_decode_to_execute_ENV_CTRL; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_39) begin
      execute_to_memory_ENV_CTRL <= _zz_execute_to_memory_ENV_CTRL; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_40) begin
      memory_to_writeBack_ENV_CTRL <= _zz_memory_to_writeBack_ENV_CTRL; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_41) begin
      decode_to_execute_BRANCH_CTRL <= _zz_decode_to_execute_BRANCH_CTRL; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_42) begin
      decode_to_execute_RS1 <= decode_RS1; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_43) begin
      decode_to_execute_RS2 <= decode_RS2; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_44) begin
      decode_to_execute_SRC2_FORCE_ZERO <= decode_SRC2_FORCE_ZERO; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_45) begin
      decode_to_execute_CSR_WRITE_OPCODE <= decode_CSR_WRITE_OPCODE; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_46) begin
      decode_to_execute_CSR_READ_OPCODE <= decode_CSR_READ_OPCODE; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_47) begin
      decode_to_execute_DO_EBREAK <= decode_DO_EBREAK; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_48) begin
      decode_to_execute_PREDICTION_HAD_BRANCHED2 <= decode_PREDICTION_HAD_BRANCHED2; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_49) begin
      execute_to_memory_MEMORY_STORE_DATA_RF <= execute_MEMORY_STORE_DATA_RF; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_50) begin
      memory_to_writeBack_MEMORY_STORE_DATA_RF <= memory_MEMORY_STORE_DATA_RF; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_51) begin
      execute_to_memory_REGFILE_WRITE_DATA <= _zz_decode_RS2; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_52) begin
      memory_to_writeBack_REGFILE_WRITE_DATA <= _zz_decode_RS2_1; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_53) begin
      execute_to_memory_SHIFT_RIGHT <= execute_SHIFT_RIGHT; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_54) begin
      execute_to_memory_BRANCH_DO <= execute_BRANCH_DO; // @[Pipeline.scala 124:40]
    end
    if(when_Pipeline_l124_55) begin
      execute_to_memory_BRANCH_CALC <= execute_BRANCH_CALC; // @[Pipeline.scala 124:40]
    end
    if(when_CsrPlugin_l1589) begin
      execute_CsrPlugin_csr_3857 <= (decode_INSTRUCTION[31 : 20] == 12'hf11); // @[CsrPlugin.scala 1589:101]
    end
    if(when_CsrPlugin_l1589_1) begin
      execute_CsrPlugin_csr_3858 <= (decode_INSTRUCTION[31 : 20] == 12'hf12); // @[CsrPlugin.scala 1589:101]
    end
    if(when_CsrPlugin_l1589_2) begin
      execute_CsrPlugin_csr_3859 <= (decode_INSTRUCTION[31 : 20] == 12'hf13); // @[CsrPlugin.scala 1589:101]
    end
    if(when_CsrPlugin_l1589_3) begin
      execute_CsrPlugin_csr_3860 <= (decode_INSTRUCTION[31 : 20] == 12'hf14); // @[CsrPlugin.scala 1589:101]
    end
    if(when_CsrPlugin_l1589_4) begin
      execute_CsrPlugin_csr_769 <= (decode_INSTRUCTION[31 : 20] == 12'h301); // @[CsrPlugin.scala 1589:101]
    end
    if(when_CsrPlugin_l1589_5) begin
      execute_CsrPlugin_csr_768 <= (decode_INSTRUCTION[31 : 20] == 12'h300); // @[CsrPlugin.scala 1589:101]
    end
    if(when_CsrPlugin_l1589_6) begin
      execute_CsrPlugin_csr_836 <= (decode_INSTRUCTION[31 : 20] == 12'h344); // @[CsrPlugin.scala 1589:101]
    end
    if(when_CsrPlugin_l1589_7) begin
      execute_CsrPlugin_csr_772 <= (decode_INSTRUCTION[31 : 20] == 12'h304); // @[CsrPlugin.scala 1589:101]
    end
    if(when_CsrPlugin_l1589_8) begin
      execute_CsrPlugin_csr_773 <= (decode_INSTRUCTION[31 : 20] == 12'h305); // @[CsrPlugin.scala 1589:101]
    end
    if(when_CsrPlugin_l1589_9) begin
      execute_CsrPlugin_csr_833 <= (decode_INSTRUCTION[31 : 20] == 12'h341); // @[CsrPlugin.scala 1589:101]
    end
    if(when_CsrPlugin_l1589_10) begin
      execute_CsrPlugin_csr_832 <= (decode_INSTRUCTION[31 : 20] == 12'h340); // @[CsrPlugin.scala 1589:101]
    end
    if(when_CsrPlugin_l1589_11) begin
      execute_CsrPlugin_csr_834 <= (decode_INSTRUCTION[31 : 20] == 12'h342); // @[CsrPlugin.scala 1589:101]
    end
    if(when_CsrPlugin_l1589_12) begin
      execute_CsrPlugin_csr_835 <= (decode_INSTRUCTION[31 : 20] == 12'h343); // @[CsrPlugin.scala 1589:101]
    end
    if(when_CsrPlugin_l1589_13) begin
      execute_CsrPlugin_csr_2816 <= (decode_INSTRUCTION[31 : 20] == 12'hb00); // @[CsrPlugin.scala 1589:101]
    end
    if(when_CsrPlugin_l1589_14) begin
      execute_CsrPlugin_csr_2944 <= (decode_INSTRUCTION[31 : 20] == 12'hb80); // @[CsrPlugin.scala 1589:101]
    end
    if(when_CsrPlugin_l1589_15) begin
      execute_CsrPlugin_csr_2818 <= (decode_INSTRUCTION[31 : 20] == 12'hb02); // @[CsrPlugin.scala 1589:101]
    end
    if(when_CsrPlugin_l1589_16) begin
      execute_CsrPlugin_csr_2946 <= (decode_INSTRUCTION[31 : 20] == 12'hb82); // @[CsrPlugin.scala 1589:101]
    end
    if(when_CsrPlugin_l1589_17) begin
      execute_CsrPlugin_csr_3072 <= (decode_INSTRUCTION[31 : 20] == 12'hc00); // @[CsrPlugin.scala 1589:101]
    end
    if(when_CsrPlugin_l1589_18) begin
      execute_CsrPlugin_csr_3200 <= (decode_INSTRUCTION[31 : 20] == 12'hc80); // @[CsrPlugin.scala 1589:101]
    end
    if(when_CsrPlugin_l1589_19) begin
      execute_CsrPlugin_csr_3074 <= (decode_INSTRUCTION[31 : 20] == 12'hc02); // @[CsrPlugin.scala 1589:101]
    end
    if(when_CsrPlugin_l1589_20) begin
      execute_CsrPlugin_csr_3202 <= (decode_INSTRUCTION[31 : 20] == 12'hc82); // @[CsrPlugin.scala 1589:101]
    end
    if(execute_CsrPlugin_csr_836) begin
      if(execute_CsrPlugin_writeEnable) begin
        CsrPlugin_mip_MSIP <= CsrPlugin_csrMapping_writeDataSignal[3]; // @[Bool.scala 189:10]
      end
    end
    if(execute_CsrPlugin_csr_833) begin
      if(execute_CsrPlugin_writeEnable) begin
        CsrPlugin_mepc <= CsrPlugin_csrMapping_writeDataSignal[31 : 0]; // @[UInt.scala 381:56]
      end
    end
    if(execute_CsrPlugin_csr_832) begin
      if(execute_CsrPlugin_writeEnable) begin
        CsrPlugin_mscratch <= CsrPlugin_csrMapping_writeDataSignal[31 : 0]; // @[Bits.scala 133:56]
      end
    end
    if(execute_CsrPlugin_csr_834) begin
      if(execute_CsrPlugin_writeEnable) begin
        CsrPlugin_mcause_interrupt <= CsrPlugin_csrMapping_writeDataSignal[31]; // @[Bool.scala 189:10]
        CsrPlugin_mcause_exceptionCode <= CsrPlugin_csrMapping_writeDataSignal[3 : 0]; // @[UInt.scala 381:56]
      end
    end
    if(execute_CsrPlugin_csr_835) begin
      if(execute_CsrPlugin_writeEnable) begin
        CsrPlugin_mtval <= CsrPlugin_csrMapping_writeDataSignal[31 : 0]; // @[UInt.scala 381:56]
      end
    end
    if(execute_CsrPlugin_writeInstruction) begin
      if(when_PmpPlugin_l172) begin
        if(when_PmpPlugin_l175) begin
          execute_PmpPlugin_writeData_ <= CsrPlugin_csrMapping_writeDataSignal; // @[PmpPlugin.scala 177:24]
          execute_PmpPlugin_pmpNcfg_ <= execute_PmpPlugin_pmpNcfg; // @[PmpPlugin.scala 178:22]
          execute_PmpPlugin_pmpcfgN_ <= execute_PmpPlugin_pmpcfgN; // @[PmpPlugin.scala 179:22]
        end
      end
    end
  end

  always @(posedge clk) begin
    DebugPlugin_firstCycle <= 1'b0; // @[Reg.scala 39:30]
    if(debug_bus_cmd_ready) begin
      DebugPlugin_firstCycle <= 1'b1; // @[DebugPlugin.scala 231:39]
    end
    DebugPlugin_secondCycle <= DebugPlugin_firstCycle; // @[Reg.scala 39:30]
    DebugPlugin_isPipBusy <= (({writeBack_arbitration_isValid,{memory_arbitration_isValid,{execute_arbitration_isValid,decode_arbitration_isValid}}} != 4'b0000) || IBusCachedPlugin_incomingInstruction); // @[Reg.scala 39:30]
    if(writeBack_arbitration_isValid) begin
      DebugPlugin_busReadDataReg <= _zz_decode_RS2_2; // @[DebugPlugin.scala 253:24]
    end
    _zz_when_DebugPlugin_l257 <= debug_bus_cmd_payload_address[2]; // @[Reg.scala 39:30]
    if(when_DebugPlugin_l308) begin
      DebugPlugin_busReadDataReg <= execute_PC; // @[DebugPlugin.scala 310:24]
    end
    DebugPlugin_resetIt_regNext <= DebugPlugin_resetIt; // @[Reg.scala 39:30]
  end

  always @(posedge clk or posedge debugReset) begin
    if(debugReset) begin
      DebugPlugin_resetIt <= 1'b0; // @[Data.scala 400:33]
      DebugPlugin_haltIt <= 1'b0; // @[Data.scala 400:33]
      DebugPlugin_stepIt <= 1'b0; // @[Data.scala 400:33]
      DebugPlugin_godmode <= 1'b0; // @[Data.scala 400:33]
      DebugPlugin_haltedByBreak <= 1'b0; // @[Data.scala 400:33]
      DebugPlugin_debugUsed <= 1'b0; // @[Data.scala 400:33]
      DebugPlugin_disableEbreak <= 1'b0; // @[Data.scala 400:33]
    end else begin
      if(when_DebugPlugin_l238) begin
        DebugPlugin_godmode <= 1'b1; // @[DebugPlugin.scala 238:36]
      end
      if(debug_bus_cmd_valid) begin
        DebugPlugin_debugUsed <= 1'b1; // @[DebugPlugin.scala 240:38]
      end
      if(debug_bus_cmd_valid) begin
        case(switch_DebugPlugin_l280)
          6'h0 : begin
            if(debug_bus_cmd_payload_wr) begin
              DebugPlugin_stepIt <= debug_bus_cmd_payload_data[4]; // @[DebugPlugin.scala 283:22]
              if(when_DebugPlugin_l284) begin
                DebugPlugin_resetIt <= 1'b1; // @[DebugPlugin.scala 284:23]
              end
              if(when_DebugPlugin_l284_1) begin
                DebugPlugin_resetIt <= 1'b0; // @[DebugPlugin.scala 284:53]
              end
              if(when_DebugPlugin_l285) begin
                DebugPlugin_haltIt <= 1'b1; // @[DebugPlugin.scala 285:22]
              end
              if(when_DebugPlugin_l285_1) begin
                DebugPlugin_haltIt <= 1'b0; // @[DebugPlugin.scala 285:52]
              end
              if(when_DebugPlugin_l286) begin
                DebugPlugin_haltedByBreak <= 1'b0; // @[DebugPlugin.scala 286:29]
              end
              if(when_DebugPlugin_l287) begin
                DebugPlugin_godmode <= 1'b0; // @[DebugPlugin.scala 287:23]
              end
              if(when_DebugPlugin_l288) begin
                DebugPlugin_disableEbreak <= 1'b1; // @[DebugPlugin.scala 288:29]
              end
              if(when_DebugPlugin_l288_1) begin
                DebugPlugin_disableEbreak <= 1'b0; // @[DebugPlugin.scala 288:59]
              end
            end
          end
          default : begin
          end
        endcase
      end
      if(when_DebugPlugin_l308) begin
        if(when_DebugPlugin_l311) begin
          DebugPlugin_haltIt <= 1'b1; // @[DebugPlugin.scala 315:18]
          DebugPlugin_haltedByBreak <= 1'b1; // @[DebugPlugin.scala 316:25]
        end
      end
      if(when_DebugPlugin_l324) begin
        if(decode_arbitration_isValid) begin
          DebugPlugin_haltIt <= 1'b1; // @[DebugPlugin.scala 327:18]
        end
      end
    end
  end


endmodule

module DataCache (
  input               io_cpu_execute_isValid,
  input      [31:0]   io_cpu_execute_address,
  output reg          io_cpu_execute_haltIt,
  input               io_cpu_execute_args_wr,
  input      [1:0]    io_cpu_execute_args_size,
  input               io_cpu_execute_args_totalyConsistent,
  output              io_cpu_execute_refilling,
  input               io_cpu_memory_isValid,
  input               io_cpu_memory_isStuck,
  output              io_cpu_memory_isWrite,
  input      [31:0]   io_cpu_memory_address,
  input      [31:0]   io_cpu_memory_mmuRsp_physicalAddress,
  input               io_cpu_memory_mmuRsp_isIoAccess,
  input               io_cpu_memory_mmuRsp_isPaging,
  input               io_cpu_memory_mmuRsp_allowRead,
  input               io_cpu_memory_mmuRsp_allowWrite,
  input               io_cpu_memory_mmuRsp_allowExecute,
  input               io_cpu_memory_mmuRsp_exception,
  input               io_cpu_memory_mmuRsp_refilling,
  input               io_cpu_memory_mmuRsp_bypassTranslation,
  input               io_cpu_writeBack_isValid,
  input               io_cpu_writeBack_isStuck,
  input               io_cpu_writeBack_isFiring,
  input               io_cpu_writeBack_isUser,
  output reg          io_cpu_writeBack_haltIt,
  output              io_cpu_writeBack_isWrite,
  input      [31:0]   io_cpu_writeBack_storeData,
  output reg [31:0]   io_cpu_writeBack_data,
  input      [31:0]   io_cpu_writeBack_address,
  output              io_cpu_writeBack_mmuException,
  output              io_cpu_writeBack_unalignedAccess,
  output reg          io_cpu_writeBack_accessError,
  output              io_cpu_writeBack_keepMemRspData,
  input               io_cpu_writeBack_fence_SW,
  input               io_cpu_writeBack_fence_SR,
  input               io_cpu_writeBack_fence_SO,
  input               io_cpu_writeBack_fence_SI,
  input               io_cpu_writeBack_fence_PW,
  input               io_cpu_writeBack_fence_PR,
  input               io_cpu_writeBack_fence_PO,
  input               io_cpu_writeBack_fence_PI,
  input      [3:0]    io_cpu_writeBack_fence_FM,
  output              io_cpu_writeBack_exclusiveOk,
  output reg          io_cpu_redo,
  input               io_cpu_flush_valid,
  output              io_cpu_flush_ready,
  input               io_cpu_flush_payload_singleLine,
  input      [6:0]    io_cpu_flush_payload_lineId,
  output reg          io_mem_cmd_valid,
  input               io_mem_cmd_ready,
  output reg          io_mem_cmd_payload_wr,
  output              io_mem_cmd_payload_uncached,
  output reg [31:0]   io_mem_cmd_payload_address,
  output     [31:0]   io_mem_cmd_payload_data,
  output     [3:0]    io_mem_cmd_payload_mask,
  output reg [2:0]    io_mem_cmd_payload_size,
  output              io_mem_cmd_payload_last,
  input               io_mem_rsp_valid,
  input               io_mem_rsp_payload_last,
  input      [31:0]   io_mem_rsp_payload_data,
  input               io_mem_rsp_payload_error,
  input               clk,
  input               reset
);

  reg        [21:0]   _zz_ways_0_tags_port0;
  reg        [31:0]   _zz_ways_0_data_port0;
  wire       [21:0]   _zz_ways_0_tags_port;
  wire       [9:0]    _zz_stage0_dataColisions;
  wire       [9:0]    _zz__zz_stageA_dataColisions;
  wire       [0:0]    _zz_when;
  wire       [2:0]    _zz_loader_counter_valueNext;
  wire       [0:0]    _zz_loader_counter_valueNext_1;
  wire       [1:0]    _zz_loader_waysAllocator;
  reg                 _zz_1;
  reg                 _zz_2;
  wire                haltCpu;
  reg                 tagsReadCmd_valid;
  reg        [6:0]    tagsReadCmd_payload;
  reg                 tagsWriteCmd_valid;
  reg        [0:0]    tagsWriteCmd_payload_way;
  reg        [6:0]    tagsWriteCmd_payload_address;
  reg                 tagsWriteCmd_payload_data_valid;
  reg                 tagsWriteCmd_payload_data_error;
  reg        [19:0]   tagsWriteCmd_payload_data_address;
  reg                 tagsWriteLastCmd_valid;
  reg        [0:0]    tagsWriteLastCmd_payload_way;
  reg        [6:0]    tagsWriteLastCmd_payload_address;
  reg                 tagsWriteLastCmd_payload_data_valid;
  reg                 tagsWriteLastCmd_payload_data_error;
  reg        [19:0]   tagsWriteLastCmd_payload_data_address;
  reg                 dataReadCmd_valid;
  reg        [9:0]    dataReadCmd_payload;
  reg                 dataWriteCmd_valid;
  reg        [0:0]    dataWriteCmd_payload_way;
  reg        [9:0]    dataWriteCmd_payload_address;
  reg        [31:0]   dataWriteCmd_payload_data;
  reg        [3:0]    dataWriteCmd_payload_mask;
  wire                _zz_ways_0_tagsReadRsp_valid;
  wire                ways_0_tagsReadRsp_valid;
  wire                ways_0_tagsReadRsp_error;
  wire       [19:0]   ways_0_tagsReadRsp_address;
  wire       [21:0]   _zz_ways_0_tagsReadRsp_valid_1;
  wire                _zz_ways_0_dataReadRspMem;
  wire       [31:0]   ways_0_dataReadRspMem;
  wire       [31:0]   ways_0_dataReadRsp;
  wire                when_DataCache_l642;
  wire                when_DataCache_l645;
  wire                when_DataCache_l664;
  wire                rspSync;
  wire                rspLast;
  reg                 memCmdSent;
  wire                io_mem_cmd_fire;
  wire                when_DataCache_l686;
  reg        [3:0]    _zz_stage0_mask;
  wire       [3:0]    stage0_mask;
  wire       [0:0]    stage0_dataColisions;
  wire       [0:0]    stage0_wayInvalidate;
  wire                stage0_isAmo;
  wire                when_DataCache_l771;
  reg                 stageA_request_wr;
  reg        [1:0]    stageA_request_size;
  reg                 stageA_request_totalyConsistent;
  wire                when_DataCache_l771_1;
  reg        [3:0]    stageA_mask;
  wire                stageA_isAmo;
  wire                stageA_isLrsc;
  wire       [0:0]    stageA_wayHits;
  wire                when_DataCache_l771_2;
  reg        [0:0]    stageA_wayInvalidate;
  wire                when_DataCache_l771_3;
  reg        [0:0]    stage0_dataColisions_regNextWhen;
  wire       [0:0]    _zz_stageA_dataColisions;
  wire       [0:0]    stageA_dataColisions;
  wire                when_DataCache_l822;
  reg                 stageB_request_wr;
  reg        [1:0]    stageB_request_size;
  reg                 stageB_request_totalyConsistent;
  reg                 stageB_mmuRspFreeze;
  wire                when_DataCache_l824;
  reg        [31:0]   stageB_mmuRsp_physicalAddress;
  reg                 stageB_mmuRsp_isIoAccess;
  reg                 stageB_mmuRsp_isPaging;
  reg                 stageB_mmuRsp_allowRead;
  reg                 stageB_mmuRsp_allowWrite;
  reg                 stageB_mmuRsp_allowExecute;
  reg                 stageB_mmuRsp_exception;
  reg                 stageB_mmuRsp_refilling;
  reg                 stageB_mmuRsp_bypassTranslation;
  wire                when_DataCache_l821;
  reg                 stageB_tagsReadRsp_0_valid;
  reg                 stageB_tagsReadRsp_0_error;
  reg        [19:0]   stageB_tagsReadRsp_0_address;
  wire                when_DataCache_l821_1;
  reg        [31:0]   stageB_dataReadRsp_0;
  wire                when_DataCache_l820;
  reg        [0:0]    stageB_wayInvalidate;
  wire                stageB_consistancyHazard;
  wire                when_DataCache_l820_1;
  reg        [0:0]    stageB_dataColisions;
  wire                when_DataCache_l820_2;
  reg                 stageB_unaligned;
  wire                when_DataCache_l820_3;
  reg        [0:0]    stageB_waysHitsBeforeInvalidate;
  wire       [0:0]    stageB_waysHits;
  wire                stageB_waysHit;
  wire       [31:0]   stageB_dataMux;
  wire                when_DataCache_l820_4;
  reg        [3:0]    stageB_mask;
  reg                 stageB_loaderValid;
  wire       [31:0]   stageB_ioMemRspMuxed;
  reg                 stageB_flusher_waitDone;
  wire                stageB_flusher_hold;
  reg        [7:0]    stageB_flusher_counter;
  wire                when_DataCache_l850;
  wire                when_DataCache_l856;
  reg                 stageB_flusher_start;
  wire                stageB_isAmo;
  wire                stageB_isAmoCached;
  wire                stageB_isExternalLsrc;
  wire                stageB_isExternalAmo;
  wire       [31:0]   stageB_requestDataBypass;
  reg                 stageB_cpuWriteToCache;
  wire                when_DataCache_l926;
  wire                stageB_badPermissions;
  wire                stageB_loadStoreFault;
  wire                stageB_bypassCache;
  wire                when_DataCache_l995;
  wire                when_DataCache_l1004;
  wire                when_DataCache_l1009;
  wire                when_DataCache_l1020;
  wire                when_DataCache_l1032;
  wire                when_DataCache_l991;
  wire                when_DataCache_l1066;
  wire                when_DataCache_l1075;
  reg                 loader_valid;
  reg                 loader_counter_willIncrement;
  wire                loader_counter_willClear;
  reg        [2:0]    loader_counter_valueNext;
  reg        [2:0]    loader_counter_value;
  wire                loader_counter_willOverflowIfInc;
  wire                loader_counter_willOverflow;
  reg        [0:0]    loader_waysAllocator;
  reg                 loader_error;
  wire                loader_kill;
  reg                 loader_killReg;
  wire                when_DataCache_l1090;
  wire                loader_done;
  wire                when_DataCache_l1118;
  reg                 loader_valid_regNext;
  wire                when_DataCache_l1122;
  wire                when_DataCache_l1125;
  reg [21:0] ways_0_tags [0:127];
  reg [7:0] ways_0_data_symbol0 [0:1023];
  reg [7:0] ways_0_data_symbol1 [0:1023];
  reg [7:0] ways_0_data_symbol2 [0:1023];
  reg [7:0] ways_0_data_symbol3 [0:1023];
  reg [7:0] _zz_ways_0_datasymbol_read;
  reg [7:0] _zz_ways_0_datasymbol_read_1;
  reg [7:0] _zz_ways_0_datasymbol_read_2;
  reg [7:0] _zz_ways_0_datasymbol_read_3;

  assign _zz_stage0_dataColisions = (io_cpu_execute_address[11 : 2] >>> 0);
  assign _zz__zz_stageA_dataColisions = (io_cpu_memory_address[11 : 2] >>> 0);
  assign _zz_when = 1'b1;
  assign _zz_loader_counter_valueNext_1 = loader_counter_willIncrement;
  assign _zz_loader_counter_valueNext = {2'd0, _zz_loader_counter_valueNext_1};
  assign _zz_loader_waysAllocator = {loader_waysAllocator,loader_waysAllocator[0]};
  assign _zz_ways_0_tags_port = {tagsWriteCmd_payload_data_address,{tagsWriteCmd_payload_data_error,tagsWriteCmd_payload_data_valid}};
  always @(posedge clk) begin
    if(_zz_ways_0_tagsReadRsp_valid) begin
      _zz_ways_0_tags_port0 <= ways_0_tags[tagsReadCmd_payload];
    end
  end

  always @(posedge clk) begin
    if(_zz_2) begin
      ways_0_tags[tagsWriteCmd_payload_address] <= _zz_ways_0_tags_port;
    end
  end

  always @(*) begin
    _zz_ways_0_data_port0 = {_zz_ways_0_datasymbol_read_3, _zz_ways_0_datasymbol_read_2, _zz_ways_0_datasymbol_read_1, _zz_ways_0_datasymbol_read};
  end
  always @(posedge clk) begin
    if(_zz_ways_0_dataReadRspMem) begin
      _zz_ways_0_datasymbol_read <= ways_0_data_symbol0[dataReadCmd_payload];
      _zz_ways_0_datasymbol_read_1 <= ways_0_data_symbol1[dataReadCmd_payload];
      _zz_ways_0_datasymbol_read_2 <= ways_0_data_symbol2[dataReadCmd_payload];
      _zz_ways_0_datasymbol_read_3 <= ways_0_data_symbol3[dataReadCmd_payload];
    end
  end

  always @(posedge clk) begin
    if(dataWriteCmd_payload_mask[0] && _zz_1) begin
      ways_0_data_symbol0[dataWriteCmd_payload_address] <= dataWriteCmd_payload_data[7 : 0];
    end
    if(dataWriteCmd_payload_mask[1] && _zz_1) begin
      ways_0_data_symbol1[dataWriteCmd_payload_address] <= dataWriteCmd_payload_data[15 : 8];
    end
    if(dataWriteCmd_payload_mask[2] && _zz_1) begin
      ways_0_data_symbol2[dataWriteCmd_payload_address] <= dataWriteCmd_payload_data[23 : 16];
    end
    if(dataWriteCmd_payload_mask[3] && _zz_1) begin
      ways_0_data_symbol3[dataWriteCmd_payload_address] <= dataWriteCmd_payload_data[31 : 24];
    end
  end

  always @(*) begin
    _zz_1 = 1'b0; // @[when.scala 47:16]
    if(when_DataCache_l645) begin
      _zz_1 = 1'b1; // @[when.scala 52:10]
    end
  end

  always @(*) begin
    _zz_2 = 1'b0; // @[when.scala 47:16]
    if(when_DataCache_l642) begin
      _zz_2 = 1'b1; // @[when.scala 52:10]
    end
  end

  assign haltCpu = 1'b0; // @[DataCache.scala 575:17]
  assign _zz_ways_0_tagsReadRsp_valid = (tagsReadCmd_valid && (! io_cpu_memory_isStuck)); // @[BaseType.scala 305:24]
  assign _zz_ways_0_tagsReadRsp_valid_1 = _zz_ways_0_tags_port0; // @[Mem.scala 310:24]
  assign ways_0_tagsReadRsp_valid = _zz_ways_0_tagsReadRsp_valid_1[0]; // @[Bool.scala 189:10]
  assign ways_0_tagsReadRsp_error = _zz_ways_0_tagsReadRsp_valid_1[1]; // @[Bool.scala 189:10]
  assign ways_0_tagsReadRsp_address = _zz_ways_0_tagsReadRsp_valid_1[21 : 2]; // @[UInt.scala 381:56]
  assign _zz_ways_0_dataReadRspMem = (dataReadCmd_valid && (! io_cpu_memory_isStuck)); // @[BaseType.scala 305:24]
  assign ways_0_dataReadRspMem = _zz_ways_0_data_port0; // @[Bits.scala 133:56]
  assign ways_0_dataReadRsp = ways_0_dataReadRspMem[31 : 0]; // @[Vec.scala 169:11]
  assign when_DataCache_l642 = (tagsWriteCmd_valid && tagsWriteCmd_payload_way[0]); // @[BaseType.scala 305:24]
  assign when_DataCache_l645 = (dataWriteCmd_valid && dataWriteCmd_payload_way[0]); // @[BaseType.scala 305:24]
  always @(*) begin
    tagsReadCmd_valid = 1'b0; // @[DataCache.scala 655:21]
    if(when_DataCache_l664) begin
      tagsReadCmd_valid = 1'b1; // @[DataCache.scala 665:25]
    end
  end

  always @(*) begin
    tagsReadCmd_payload = 7'bxxxxxxx; // @[UInt.scala 467:20]
    if(when_DataCache_l664) begin
      tagsReadCmd_payload = io_cpu_execute_address[11 : 5]; // @[DataCache.scala 667:25]
    end
  end

  always @(*) begin
    dataReadCmd_valid = 1'b0; // @[DataCache.scala 657:21]
    if(when_DataCache_l664) begin
      dataReadCmd_valid = 1'b1; // @[DataCache.scala 666:25]
    end
  end

  always @(*) begin
    dataReadCmd_payload = 10'bxxxxxxxxxx; // @[UInt.scala 467:20]
    if(when_DataCache_l664) begin
      dataReadCmd_payload = io_cpu_execute_address[11 : 2]; // @[DataCache.scala 668:25]
    end
  end

  always @(*) begin
    tagsWriteCmd_valid = 1'b0; // @[DataCache.scala 659:22]
    if(when_DataCache_l850) begin
      tagsWriteCmd_valid = 1'b1; // @[DataCache.scala 851:28]
    end
    if(when_DataCache_l1066) begin
      tagsWriteCmd_valid = 1'b0; // @[DataCache.scala 1068:26]
    end
    if(loader_done) begin
      tagsWriteCmd_valid = 1'b1; // @[DataCache.scala 1107:26]
    end
  end

  always @(*) begin
    tagsWriteCmd_payload_way = 1'bx; // @[Bits.scala 231:20]
    if(when_DataCache_l850) begin
      tagsWriteCmd_payload_way = 1'b1; // @[Bits.scala 226:10]
    end
    if(loader_done) begin
      tagsWriteCmd_payload_way = loader_waysAllocator; // @[DataCache.scala 1112:24]
    end
  end

  always @(*) begin
    tagsWriteCmd_payload_address = 7'bxxxxxxx; // @[UInt.scala 467:20]
    if(when_DataCache_l850) begin
      tagsWriteCmd_payload_address = stageB_flusher_counter[6:0]; // @[DataCache.scala 852:30]
    end
    if(loader_done) begin
      tagsWriteCmd_payload_address = stageB_mmuRsp_physicalAddress[11 : 5]; // @[DataCache.scala 1108:28]
    end
  end

  always @(*) begin
    tagsWriteCmd_payload_data_valid = 1'bx; // @[Bool.scala 276:20]
    if(when_DataCache_l850) begin
      tagsWriteCmd_payload_data_valid = 1'b0; // @[DataCache.scala 854:33]
    end
    if(loader_done) begin
      tagsWriteCmd_payload_data_valid = (! (loader_kill || loader_killReg)); // @[DataCache.scala 1109:31]
    end
  end

  always @(*) begin
    tagsWriteCmd_payload_data_error = 1'bx; // @[Bool.scala 276:20]
    if(loader_done) begin
      tagsWriteCmd_payload_data_error = (loader_error || (io_mem_rsp_valid && io_mem_rsp_payload_error)); // @[DataCache.scala 1111:31]
    end
  end

  always @(*) begin
    tagsWriteCmd_payload_data_address = 20'bxxxxxxxxxxxxxxxxxxxx; // @[UInt.scala 467:20]
    if(loader_done) begin
      tagsWriteCmd_payload_data_address = stageB_mmuRsp_physicalAddress[31 : 12]; // @[DataCache.scala 1110:33]
    end
  end

  always @(*) begin
    dataWriteCmd_valid = 1'b0; // @[DataCache.scala 661:22]
    if(stageB_cpuWriteToCache) begin
      if(when_DataCache_l926) begin
        dataWriteCmd_valid = 1'b1; // @[DataCache.scala 926:26]
      end
    end
    if(when_DataCache_l1066) begin
      dataWriteCmd_valid = 1'b0; // @[DataCache.scala 1069:26]
    end
    if(when_DataCache_l1090) begin
      dataWriteCmd_valid = 1'b1; // @[DataCache.scala 1091:26]
    end
  end

  always @(*) begin
    dataWriteCmd_payload_way = 1'bx; // @[Bits.scala 231:20]
    if(stageB_cpuWriteToCache) begin
      dataWriteCmd_payload_way = stageB_waysHits; // @[DataCache.scala 931:24]
    end
    if(when_DataCache_l1090) begin
      dataWriteCmd_payload_way = loader_waysAllocator; // @[DataCache.scala 1095:24]
    end
  end

  always @(*) begin
    dataWriteCmd_payload_address = 10'bxxxxxxxxxx; // @[UInt.scala 467:20]
    if(stageB_cpuWriteToCache) begin
      dataWriteCmd_payload_address = stageB_mmuRsp_physicalAddress[11 : 2]; // @[DataCache.scala 927:28]
    end
    if(when_DataCache_l1090) begin
      dataWriteCmd_payload_address = {stageB_mmuRsp_physicalAddress[11 : 5],loader_counter_value}; // @[DataCache.scala 1092:28]
    end
  end

  always @(*) begin
    dataWriteCmd_payload_data = 32'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx; // @[Bits.scala 231:20]
    if(stageB_cpuWriteToCache) begin
      dataWriteCmd_payload_data[31 : 0] = stageB_requestDataBypass; // @[DataCache.scala 928:66]
    end
    if(when_DataCache_l1090) begin
      dataWriteCmd_payload_data = io_mem_rsp_payload_data; // @[DataCache.scala 1093:25]
    end
  end

  always @(*) begin
    dataWriteCmd_payload_mask = 4'bxxxx; // @[Bits.scala 231:20]
    if(stageB_cpuWriteToCache) begin
      dataWriteCmd_payload_mask = 4'b0000; // @[DataCache.scala 929:25]
      if(_zz_when[0]) begin
        dataWriteCmd_payload_mask[3 : 0] = stageB_mask; // @[Utils.scala 1049:24]
      end
    end
    if(when_DataCache_l1090) begin
      dataWriteCmd_payload_mask = 4'b1111; // @[Bits.scala 226:10]
    end
  end

  assign when_DataCache_l664 = (io_cpu_execute_isValid && (! io_cpu_memory_isStuck)); // @[BaseType.scala 305:24]
  always @(*) begin
    io_cpu_execute_haltIt = 1'b0; // @[DataCache.scala 682:25]
    if(when_DataCache_l850) begin
      io_cpu_execute_haltIt = 1'b1; // @[DataCache.scala 855:31]
    end
  end

  assign rspSync = 1'b1; // @[DataCache.scala 684:17]
  assign rspLast = 1'b1; // @[DataCache.scala 685:17]
  assign io_mem_cmd_fire = (io_mem_cmd_valid && io_mem_cmd_ready); // @[BaseType.scala 305:24]
  assign when_DataCache_l686 = (! io_cpu_writeBack_isStuck); // @[BaseType.scala 299:24]
  always @(*) begin
    _zz_stage0_mask = 4'bxxxx; // @[Bits.scala 231:20]
    case(io_cpu_execute_args_size)
      2'b00 : begin
        _zz_stage0_mask = 4'b0001; // @[Misc.scala 260:22]
      end
      2'b01 : begin
        _zz_stage0_mask = 4'b0011; // @[Misc.scala 260:22]
      end
      2'b10 : begin
        _zz_stage0_mask = 4'b1111; // @[Misc.scala 260:22]
      end
      default : begin
      end
    endcase
  end

  assign stage0_mask = (_zz_stage0_mask <<< io_cpu_execute_address[1 : 0]); // @[BaseType.scala 299:24]
  assign stage0_dataColisions[0] = (((dataWriteCmd_valid && dataWriteCmd_payload_way[0]) && (dataWriteCmd_payload_address == _zz_stage0_dataColisions)) && ((stage0_mask & dataWriteCmd_payload_mask[3 : 0]) != 4'b0000)); // @[DataCache.scala 676:14]
  assign stage0_wayInvalidate = 1'b0; // @[Expression.scala 2301:18]
  assign stage0_isAmo = 1'b0; // @[DataCache.scala 767:55]
  assign when_DataCache_l771 = (! io_cpu_memory_isStuck); // @[BaseType.scala 299:24]
  assign when_DataCache_l771_1 = (! io_cpu_memory_isStuck); // @[BaseType.scala 299:24]
  assign io_cpu_memory_isWrite = stageA_request_wr; // @[DataCache.scala 774:27]
  assign stageA_isAmo = 1'b0; // @[DataCache.scala 776:48]
  assign stageA_isLrsc = 1'b0; // @[DataCache.scala 777:50]
  assign stageA_wayHits = ((io_cpu_memory_mmuRsp_physicalAddress[31 : 12] == ways_0_tagsReadRsp_address) && ways_0_tagsReadRsp_valid); // @[DataCache.scala 798:15]
  assign when_DataCache_l771_2 = (! io_cpu_memory_isStuck); // @[BaseType.scala 299:24]
  assign when_DataCache_l771_3 = (! io_cpu_memory_isStuck); // @[BaseType.scala 299:24]
  assign _zz_stageA_dataColisions[0] = (((dataWriteCmd_valid && dataWriteCmd_payload_way[0]) && (dataWriteCmd_payload_address == _zz__zz_stageA_dataColisions)) && ((stageA_mask & dataWriteCmd_payload_mask[3 : 0]) != 4'b0000)); // @[DataCache.scala 676:14]
  assign stageA_dataColisions = (stage0_dataColisions_regNextWhen | _zz_stageA_dataColisions); // @[BaseType.scala 299:24]
  assign when_DataCache_l822 = (! io_cpu_writeBack_isStuck); // @[BaseType.scala 299:24]
  always @(*) begin
    stageB_mmuRspFreeze = 1'b0; // @[DataCache.scala 823:24]
    if(when_DataCache_l1125) begin
      stageB_mmuRspFreeze = 1'b1; // @[DataCache.scala 1125:25]
    end
  end

  assign when_DataCache_l824 = ((! io_cpu_writeBack_isStuck) && (! stageB_mmuRspFreeze)); // @[BaseType.scala 305:24]
  assign when_DataCache_l821 = (! io_cpu_writeBack_isStuck); // @[BaseType.scala 299:24]
  assign when_DataCache_l821_1 = (! io_cpu_writeBack_isStuck); // @[BaseType.scala 299:24]
  assign when_DataCache_l820 = (! io_cpu_writeBack_isStuck); // @[BaseType.scala 299:24]
  assign stageB_consistancyHazard = 1'b0; // @[DataCache.scala 828:112]
  assign when_DataCache_l820_1 = (! io_cpu_writeBack_isStuck); // @[BaseType.scala 299:24]
  assign when_DataCache_l820_2 = (! io_cpu_writeBack_isStuck); // @[BaseType.scala 299:24]
  assign when_DataCache_l820_3 = (! io_cpu_writeBack_isStuck); // @[BaseType.scala 299:24]
  assign stageB_waysHits = (stageB_waysHitsBeforeInvalidate & (~ stageB_wayInvalidate)); // @[BaseType.scala 299:24]
  assign stageB_waysHit = (|stageB_waysHits); // @[BaseType.scala 312:24]
  assign stageB_dataMux = stageB_dataReadRsp_0; // @[Vec.scala 169:11]
  assign when_DataCache_l820_4 = (! io_cpu_writeBack_isStuck); // @[BaseType.scala 299:24]
  always @(*) begin
    stageB_loaderValid = 1'b0; // @[DataCache.scala 839:23]
    if(io_cpu_writeBack_isValid) begin
      if(!stageB_isExternalAmo) begin
        if(!when_DataCache_l991) begin
          if(!when_DataCache_l1004) begin
            if(io_mem_cmd_ready) begin
              stageB_loaderValid = 1'b1; // @[DataCache.scala 1037:23]
            end
          end
        end
      end
    end
    if(when_DataCache_l1066) begin
      stageB_loaderValid = 1'b0; // @[DataCache.scala 1070:19]
    end
  end

  assign stageB_ioMemRspMuxed = io_mem_rsp_payload_data[31 : 0]; // @[Vec.scala 169:11]
  always @(*) begin
    io_cpu_writeBack_haltIt = 1'b1; // @[DataCache.scala 843:29]
    if(io_cpu_writeBack_isValid) begin
      if(!stageB_isExternalAmo) begin
        if(when_DataCache_l991) begin
          if(when_DataCache_l995) begin
            io_cpu_writeBack_haltIt = 1'b0; // @[DataCache.scala 995:42]
          end
        end else begin
          if(when_DataCache_l1004) begin
            if(when_DataCache_l1009) begin
              io_cpu_writeBack_haltIt = 1'b0; // @[DataCache.scala 1009:35]
            end
          end
        end
      end
    end
    if(when_DataCache_l1066) begin
      io_cpu_writeBack_haltIt = 1'b0; // @[DataCache.scala 1071:31]
    end
  end

  assign stageB_flusher_hold = 1'b0; // @[DataCache.scala 848:18]
  assign when_DataCache_l850 = (! stageB_flusher_counter[7]); // @[BaseType.scala 299:24]
  assign when_DataCache_l856 = (! stageB_flusher_hold); // @[BaseType.scala 299:24]
  assign io_cpu_flush_ready = (stageB_flusher_waitDone && stageB_flusher_counter[7]); // @[DataCache.scala 864:26]
  assign stageB_isAmo = 1'b0; // @[DataCache.scala 886:48]
  assign stageB_isAmoCached = 1'b0; // @[DataCache.scala 887:54]
  assign stageB_isExternalLsrc = 1'b0; // @[DataCache.scala 888:67]
  assign stageB_isExternalAmo = 1'b0; // @[DataCache.scala 889:67]
  assign stageB_requestDataBypass = io_cpu_writeBack_storeData; // @[Misc.scala 552:9]
  always @(*) begin
    stageB_cpuWriteToCache = 1'b0; // @[DataCache.scala 924:27]
    if(io_cpu_writeBack_isValid) begin
      if(!stageB_isExternalAmo) begin
        if(!when_DataCache_l991) begin
          if(when_DataCache_l1004) begin
            stageB_cpuWriteToCache = 1'b1; // @[DataCache.scala 1005:27]
          end
        end
      end
    end
  end

  assign when_DataCache_l926 = (stageB_request_wr && stageB_waysHit); // @[BaseType.scala 305:24]
  assign stageB_badPermissions = (((! stageB_mmuRsp_allowWrite) && stageB_request_wr) || ((! stageB_mmuRsp_allowRead) && ((! stageB_request_wr) || stageB_isAmo))); // @[BaseType.scala 305:24]
  assign stageB_loadStoreFault = (io_cpu_writeBack_isValid && (stageB_mmuRsp_exception || stageB_badPermissions)); // @[BaseType.scala 305:24]
  always @(*) begin
    io_cpu_redo = 1'b0; // @[DataCache.scala 937:17]
    if(io_cpu_writeBack_isValid) begin
      if(!stageB_isExternalAmo) begin
        if(!when_DataCache_l991) begin
          if(when_DataCache_l1004) begin
            if(when_DataCache_l1020) begin
              io_cpu_redo = 1'b1; // @[DataCache.scala 1021:25]
            end
          end
        end
      end
    end
    if(when_DataCache_l1075) begin
      io_cpu_redo = 1'b1; // @[DataCache.scala 1075:17]
    end
    if(when_DataCache_l1122) begin
      io_cpu_redo = 1'b1; // @[DataCache.scala 1122:17]
    end
  end

  always @(*) begin
    io_cpu_writeBack_accessError = 1'b0; // @[DataCache.scala 938:34]
    if(stageB_bypassCache) begin
      io_cpu_writeBack_accessError = ((((! stageB_request_wr) && 1'b1) && io_mem_rsp_valid) && io_mem_rsp_payload_error); // @[DataCache.scala 1045:57]
    end else begin
      io_cpu_writeBack_accessError = (((stageB_waysHits & stageB_tagsReadRsp_0_error) != 1'b0) || (stageB_loadStoreFault && (! stageB_mmuRsp_isPaging))); // @[DataCache.scala 1048:57]
    end
  end

  assign io_cpu_writeBack_mmuException = (stageB_loadStoreFault && stageB_mmuRsp_isPaging); // @[DataCache.scala 939:35]
  assign io_cpu_writeBack_unalignedAccess = (io_cpu_writeBack_isValid && stageB_unaligned); // @[DataCache.scala 940:38]
  assign io_cpu_writeBack_isWrite = stageB_request_wr; // @[DataCache.scala 941:30]
  always @(*) begin
    io_mem_cmd_valid = 1'b0; // @[DataCache.scala 944:22]
    if(io_cpu_writeBack_isValid) begin
      if(!stageB_isExternalAmo) begin
        if(when_DataCache_l991) begin
          io_mem_cmd_valid = (! memCmdSent); // @[DataCache.scala 997:26]
        end else begin
          if(when_DataCache_l1004) begin
            if(stageB_request_wr) begin
              io_mem_cmd_valid = 1'b1; // @[DataCache.scala 1008:28]
            end
          end else begin
            if(when_DataCache_l1032) begin
              io_mem_cmd_valid = 1'b1; // @[DataCache.scala 1032:28]
            end
          end
        end
      end
    end
    if(when_DataCache_l1066) begin
      io_mem_cmd_valid = 1'b0; // @[DataCache.scala 1067:24]
    end
  end

  always @(*) begin
    io_mem_cmd_payload_address = stageB_mmuRsp_physicalAddress; // @[DataCache.scala 945:24]
    if(io_cpu_writeBack_isValid) begin
      if(!stageB_isExternalAmo) begin
        if(!when_DataCache_l991) begin
          if(!when_DataCache_l1004) begin
            io_mem_cmd_payload_address[4 : 0] = 5'h0; // @[DataCache.scala 1034:53]
          end
        end
      end
    end
  end

  assign io_mem_cmd_payload_last = 1'b1; // @[DataCache.scala 946:21]
  always @(*) begin
    io_mem_cmd_payload_wr = stageB_request_wr; // @[DataCache.scala 947:19]
    if(io_cpu_writeBack_isValid) begin
      if(!stageB_isExternalAmo) begin
        if(!when_DataCache_l991) begin
          if(!when_DataCache_l1004) begin
            io_mem_cmd_payload_wr = 1'b0; // @[DataCache.scala 1033:25]
          end
        end
      end
    end
  end

  assign io_mem_cmd_payload_mask = stageB_mask; // @[DataCache.scala 948:21]
  assign io_mem_cmd_payload_data = stageB_requestDataBypass; // @[DataCache.scala 949:21]
  assign io_mem_cmd_payload_uncached = stageB_mmuRsp_isIoAccess; // @[DataCache.scala 950:25]
  always @(*) begin
    io_mem_cmd_payload_size = {1'd0, stageB_request_size}; // @[DataCache.scala 951:21]
    if(io_cpu_writeBack_isValid) begin
      if(!stageB_isExternalAmo) begin
        if(!when_DataCache_l991) begin
          if(!when_DataCache_l1004) begin
            io_mem_cmd_payload_size = 3'b101; // @[DataCache.scala 1035:27]
          end
        end
      end
    end
  end

  assign stageB_bypassCache = ((stageB_mmuRsp_isIoAccess || stageB_isExternalLsrc) || stageB_isExternalAmo); // @[BaseType.scala 305:24]
  assign io_cpu_writeBack_keepMemRspData = 1'b0; // @[DataCache.scala 957:37]
  assign when_DataCache_l995 = ((! stageB_request_wr) ? (io_mem_rsp_valid && rspSync) : io_mem_cmd_ready); // @[Expression.scala 1420:25]
  assign when_DataCache_l1004 = (stageB_waysHit || (stageB_request_wr && (! stageB_isAmoCached))); // @[BaseType.scala 305:24]
  assign when_DataCache_l1009 = ((! stageB_request_wr) || io_mem_cmd_ready); // @[BaseType.scala 305:24]
  assign when_DataCache_l1020 = (((! stageB_request_wr) || stageB_isAmoCached) && ((stageB_dataColisions & stageB_waysHits) != 1'b0)); // @[BaseType.scala 305:24]
  assign when_DataCache_l1032 = (! memCmdSent); // @[BaseType.scala 299:24]
  assign when_DataCache_l991 = (stageB_mmuRsp_isIoAccess || stageB_isExternalLsrc); // @[BaseType.scala 305:24]
  always @(*) begin
    if(stageB_bypassCache) begin
      io_cpu_writeBack_data = stageB_ioMemRspMuxed; // @[DataCache.scala 1043:29]
    end else begin
      io_cpu_writeBack_data = stageB_dataMux; // @[DataCache.scala 1047:29]
    end
  end

  assign when_DataCache_l1066 = ((((stageB_consistancyHazard || stageB_mmuRsp_refilling) || io_cpu_writeBack_accessError) || io_cpu_writeBack_mmuException) || io_cpu_writeBack_unalignedAccess); // @[BaseType.scala 305:24]
  assign when_DataCache_l1075 = (io_cpu_writeBack_isValid && (stageB_mmuRsp_refilling || stageB_consistancyHazard)); // @[BaseType.scala 305:24]
  always @(*) begin
    loader_counter_willIncrement = 1'b0; // @[Utils.scala 536:23]
    if(when_DataCache_l1090) begin
      loader_counter_willIncrement = 1'b1; // @[Utils.scala 540:41]
    end
  end

  assign loader_counter_willClear = 1'b0; // @[Utils.scala 537:19]
  assign loader_counter_willOverflowIfInc = (loader_counter_value == 3'b111); // @[BaseType.scala 305:24]
  assign loader_counter_willOverflow = (loader_counter_willOverflowIfInc && loader_counter_willIncrement); // @[BaseType.scala 305:24]
  always @(*) begin
    loader_counter_valueNext = (loader_counter_value + _zz_loader_counter_valueNext); // @[Utils.scala 548:15]
    if(loader_counter_willClear) begin
      loader_counter_valueNext = 3'b000; // @[Utils.scala 558:15]
    end
  end

  assign loader_kill = 1'b0; // @[DataCache.scala 1087:16]
  assign when_DataCache_l1090 = ((loader_valid && io_mem_rsp_valid) && rspLast); // @[BaseType.scala 305:24]
  assign loader_done = loader_counter_willOverflow; // @[Misc.scala 552:9]
  assign when_DataCache_l1118 = (! loader_valid); // @[BaseType.scala 299:24]
  assign when_DataCache_l1122 = (loader_valid && (! loader_valid_regNext)); // @[BaseType.scala 305:24]
  assign io_cpu_execute_refilling = loader_valid; // @[DataCache.scala 1123:30]
  assign when_DataCache_l1125 = (stageB_loaderValid || loader_valid); // @[BaseType.scala 305:24]
  always @(posedge clk) begin
    tagsWriteLastCmd_valid <= tagsWriteCmd_valid; // @[Reg.scala 39:30]
    tagsWriteLastCmd_payload_way <= tagsWriteCmd_payload_way; // @[Reg.scala 39:30]
    tagsWriteLastCmd_payload_address <= tagsWriteCmd_payload_address; // @[Reg.scala 39:30]
    tagsWriteLastCmd_payload_data_valid <= tagsWriteCmd_payload_data_valid; // @[Reg.scala 39:30]
    tagsWriteLastCmd_payload_data_error <= tagsWriteCmd_payload_data_error; // @[Reg.scala 39:30]
    tagsWriteLastCmd_payload_data_address <= tagsWriteCmd_payload_data_address; // @[Reg.scala 39:30]
    if(when_DataCache_l771) begin
      stageA_request_wr <= io_cpu_execute_args_wr; // @[DataCache.scala 771:96]
      stageA_request_size <= io_cpu_execute_args_size; // @[DataCache.scala 771:96]
      stageA_request_totalyConsistent <= io_cpu_execute_args_totalyConsistent; // @[DataCache.scala 771:96]
    end
    if(when_DataCache_l771_1) begin
      stageA_mask <= stage0_mask; // @[DataCache.scala 771:96]
    end
    if(when_DataCache_l771_2) begin
      stageA_wayInvalidate <= stage0_wayInvalidate; // @[DataCache.scala 771:96]
    end
    if(when_DataCache_l771_3) begin
      stage0_dataColisions_regNextWhen <= stage0_dataColisions; // @[DataCache.scala 771:96]
    end
    if(when_DataCache_l822) begin
      stageB_request_wr <= stageA_request_wr; // @[DataCache.scala 822:30]
      stageB_request_size <= stageA_request_size; // @[DataCache.scala 822:30]
      stageB_request_totalyConsistent <= stageA_request_totalyConsistent; // @[DataCache.scala 822:30]
    end
    if(when_DataCache_l824) begin
      stageB_mmuRsp_physicalAddress <= io_cpu_memory_mmuRsp_physicalAddress; // @[DataCache.scala 824:29]
      stageB_mmuRsp_isIoAccess <= io_cpu_memory_mmuRsp_isIoAccess; // @[DataCache.scala 824:29]
      stageB_mmuRsp_isPaging <= io_cpu_memory_mmuRsp_isPaging; // @[DataCache.scala 824:29]
      stageB_mmuRsp_allowRead <= io_cpu_memory_mmuRsp_allowRead; // @[DataCache.scala 824:29]
      stageB_mmuRsp_allowWrite <= io_cpu_memory_mmuRsp_allowWrite; // @[DataCache.scala 824:29]
      stageB_mmuRsp_allowExecute <= io_cpu_memory_mmuRsp_allowExecute; // @[DataCache.scala 824:29]
      stageB_mmuRsp_exception <= io_cpu_memory_mmuRsp_exception; // @[DataCache.scala 824:29]
      stageB_mmuRsp_refilling <= io_cpu_memory_mmuRsp_refilling; // @[DataCache.scala 824:29]
      stageB_mmuRsp_bypassTranslation <= io_cpu_memory_mmuRsp_bypassTranslation; // @[DataCache.scala 824:29]
    end
    if(when_DataCache_l821) begin
      stageB_tagsReadRsp_0_valid <= ways_0_tagsReadRsp_valid; // @[DataCache.scala 821:95]
      stageB_tagsReadRsp_0_error <= ways_0_tagsReadRsp_error; // @[DataCache.scala 821:95]
      stageB_tagsReadRsp_0_address <= ways_0_tagsReadRsp_address; // @[DataCache.scala 821:95]
    end
    if(when_DataCache_l821_1) begin
      stageB_dataReadRsp_0 <= ways_0_dataReadRsp; // @[DataCache.scala 821:95]
    end
    if(when_DataCache_l820) begin
      stageB_wayInvalidate <= stageA_wayInvalidate; // @[DataCache.scala 820:53]
    end
    if(when_DataCache_l820_1) begin
      stageB_dataColisions <= stageA_dataColisions; // @[DataCache.scala 820:53]
    end
    if(when_DataCache_l820_2) begin
      stageB_unaligned <= ({((stageA_request_size == 2'b10) && (io_cpu_memory_address[1 : 0] != 2'b00)),((stageA_request_size == 2'b01) && (io_cpu_memory_address[0 : 0] != 1'b0))} != 2'b00); // @[DataCache.scala 820:53]
    end
    if(when_DataCache_l820_3) begin
      stageB_waysHitsBeforeInvalidate <= stageA_wayHits; // @[DataCache.scala 820:53]
    end
    if(when_DataCache_l820_4) begin
      stageB_mask <= stageA_mask; // @[DataCache.scala 820:53]
    end
    loader_valid_regNext <= loader_valid; // @[Reg.scala 39:30]
  end

  always @(posedge clk or posedge reset) begin
    if(reset) begin
      memCmdSent <= 1'b0; // @[Data.scala 400:33]
      stageB_flusher_waitDone <= 1'b0; // @[Data.scala 400:33]
      stageB_flusher_counter <= 8'h0; // @[Data.scala 400:33]
      stageB_flusher_start <= 1'b1; // @[Data.scala 400:33]
      loader_valid <= 1'b0; // @[Data.scala 400:33]
      loader_counter_value <= 3'b000; // @[Data.scala 400:33]
      loader_waysAllocator <= 1'b1; // @[Data.scala 400:33]
      loader_error <= 1'b0; // @[Data.scala 400:33]
      loader_killReg <= 1'b0; // @[Data.scala 400:33]
    end else begin
      if(io_mem_cmd_fire) begin
        memCmdSent <= 1'b1; // @[DataCache.scala 686:35]
      end
      if(when_DataCache_l686) begin
        memCmdSent <= 1'b0; // @[DataCache.scala 686:61]
      end
      if(io_cpu_flush_ready) begin
        stageB_flusher_waitDone <= 1'b0; // @[DataCache.scala 847:37]
      end
      if(when_DataCache_l850) begin
        if(when_DataCache_l856) begin
          stageB_flusher_counter <= (stageB_flusher_counter + 8'h01); // @[DataCache.scala 857:19]
          if(io_cpu_flush_payload_singleLine) begin
            stageB_flusher_counter[7] <= 1'b1; // @[DataCache.scala 859:25]
          end
        end
      end
      stageB_flusher_start <= (((((((! stageB_flusher_waitDone) && (! stageB_flusher_start)) && io_cpu_flush_valid) && (! io_cpu_execute_isValid)) && (! io_cpu_memory_isValid)) && (! io_cpu_writeBack_isValid)) && (! io_cpu_redo)); // @[DataCache.scala 867:13]
      if(stageB_flusher_start) begin
        stageB_flusher_waitDone <= 1'b1; // @[DataCache.scala 870:18]
        stageB_flusher_counter <= 8'h0; // @[DataCache.scala 871:17]
        if(io_cpu_flush_payload_singleLine) begin
          stageB_flusher_counter <= {1'b0,io_cpu_flush_payload_lineId}; // @[DataCache.scala 873:19]
        end
      end
      `ifndef SYNTHESIS
        `ifdef FORMAL
          assert((! ((io_cpu_writeBack_isValid && (! io_cpu_writeBack_haltIt)) && io_cpu_writeBack_isStuck))); // DataCache.scala:L1077
        `else
          if(!(! ((io_cpu_writeBack_isValid && (! io_cpu_writeBack_haltIt)) && io_cpu_writeBack_isStuck))) begin
            $display("ERROR writeBack stuck by another plugin is not allowed"); // DataCache.scala:L1077
          end
        `endif
      `endif
      if(stageB_loaderValid) begin
        loader_valid <= 1'b1; // @[DataCache.scala 1081:32]
      end
      loader_counter_value <= loader_counter_valueNext; // @[Reg.scala 39:30]
      if(loader_kill) begin
        loader_killReg <= 1'b1; // @[DataCache.scala 1088:34]
      end
      if(when_DataCache_l1090) begin
        loader_error <= (loader_error || io_mem_rsp_payload_error); // @[DataCache.scala 1096:13]
      end
      if(loader_done) begin
        loader_valid <= 1'b0; // @[DataCache.scala 1104:13]
        loader_error <= 1'b0; // @[DataCache.scala 1114:13]
        loader_killReg <= 1'b0; // @[DataCache.scala 1115:15]
      end
      if(when_DataCache_l1118) begin
        loader_waysAllocator <= _zz_loader_waysAllocator[0:0]; // @[DataCache.scala 1119:21]
      end
    end
  end


endmodule

module InstructionCache (
  input               io_flush,
  input               io_cpu_prefetch_isValid,
  output reg          io_cpu_prefetch_haltIt,
  input      [31:0]   io_cpu_prefetch_pc,
  input               io_cpu_fetch_isValid,
  input               io_cpu_fetch_isStuck,
  input               io_cpu_fetch_isRemoved,
  input      [31:0]   io_cpu_fetch_pc,
  output     [31:0]   io_cpu_fetch_data,
  input      [31:0]   io_cpu_fetch_mmuRsp_physicalAddress,
  input               io_cpu_fetch_mmuRsp_isIoAccess,
  input               io_cpu_fetch_mmuRsp_isPaging,
  input               io_cpu_fetch_mmuRsp_allowRead,
  input               io_cpu_fetch_mmuRsp_allowWrite,
  input               io_cpu_fetch_mmuRsp_allowExecute,
  input               io_cpu_fetch_mmuRsp_exception,
  input               io_cpu_fetch_mmuRsp_refilling,
  input               io_cpu_fetch_mmuRsp_bypassTranslation,
  output     [31:0]   io_cpu_fetch_physicalAddress,
  input               io_cpu_decode_isValid,
  input               io_cpu_decode_isStuck,
  input      [31:0]   io_cpu_decode_pc,
  output     [31:0]   io_cpu_decode_physicalAddress,
  output     [31:0]   io_cpu_decode_data,
  output              io_cpu_decode_cacheMiss,
  output              io_cpu_decode_error,
  output              io_cpu_decode_mmuRefilling,
  output              io_cpu_decode_mmuException,
  input               io_cpu_decode_isUser,
  input               io_cpu_fill_valid,
  input      [31:0]   io_cpu_fill_payload,
  output              io_mem_cmd_valid,
  input               io_mem_cmd_ready,
  output     [31:0]   io_mem_cmd_payload_address,
  output     [2:0]    io_mem_cmd_payload_size,
  input               io_mem_rsp_valid,
  input      [31:0]   io_mem_rsp_payload_data,
  input               io_mem_rsp_payload_error,
  input      [2:0]    _zz_when_Fetcher_l401,
  input      [31:0]   _zz_decodeStage_hit_data_1,
  input               clk,
  input               reset
);

  reg        [31:0]   _zz_banks_0_port1;
  reg        [21:0]   _zz_ways_0_tags_port1;
  wire       [21:0]   _zz_ways_0_tags_port;
  reg                 _zz_1;
  reg                 _zz_2;
  reg                 lineLoader_fire;
  reg                 lineLoader_valid;
  (* keep , syn_keep *) reg        [31:0]   lineLoader_address /* synthesis syn_keep = 1 */ ;
  reg                 lineLoader_hadError;
  reg                 lineLoader_flushPending;
  reg        [7:0]    lineLoader_flushCounter;
  wire                when_InstructionCache_l338;
  reg                 _zz_when_InstructionCache_l342;
  wire                when_InstructionCache_l342;
  wire                when_InstructionCache_l351;
  reg                 lineLoader_cmdSent;
  wire                io_mem_cmd_fire;
  wire                when_Utils_l520;
  reg                 lineLoader_wayToAllocate_willIncrement;
  wire                lineLoader_wayToAllocate_willClear;
  wire                lineLoader_wayToAllocate_willOverflowIfInc;
  wire                lineLoader_wayToAllocate_willOverflow;
  (* keep , syn_keep *) reg        [2:0]    lineLoader_wordIndex /* synthesis syn_keep = 1 */ ;
  wire                lineLoader_write_tag_0_valid;
  wire       [6:0]    lineLoader_write_tag_0_payload_address;
  wire                lineLoader_write_tag_0_payload_data_valid;
  wire                lineLoader_write_tag_0_payload_data_error;
  wire       [19:0]   lineLoader_write_tag_0_payload_data_address;
  wire                lineLoader_write_data_0_valid;
  wire       [9:0]    lineLoader_write_data_0_payload_address;
  wire       [31:0]   lineLoader_write_data_0_payload_data;
  wire                when_InstructionCache_l401;
  wire       [9:0]    _zz_fetchStage_read_banksValue_0_dataMem;
  wire                _zz_fetchStage_read_banksValue_0_dataMem_1;
  wire       [31:0]   fetchStage_read_banksValue_0_dataMem;
  wire       [31:0]   fetchStage_read_banksValue_0_data;
  wire       [6:0]    _zz_fetchStage_read_waysValues_0_tag_valid;
  wire                _zz_fetchStage_read_waysValues_0_tag_valid_1;
  wire                fetchStage_read_waysValues_0_tag_valid;
  wire                fetchStage_read_waysValues_0_tag_error;
  wire       [19:0]   fetchStage_read_waysValues_0_tag_address;
  wire       [21:0]   _zz_fetchStage_read_waysValues_0_tag_valid_2;
  wire                when_InstructionCache_l459;
  reg        [31:0]   decodeStage_mmuRsp_physicalAddress;
  reg                 decodeStage_mmuRsp_isIoAccess;
  reg                 decodeStage_mmuRsp_isPaging;
  reg                 decodeStage_mmuRsp_allowRead;
  reg                 decodeStage_mmuRsp_allowWrite;
  reg                 decodeStage_mmuRsp_allowExecute;
  reg                 decodeStage_mmuRsp_exception;
  reg                 decodeStage_mmuRsp_refilling;
  reg                 decodeStage_mmuRsp_bypassTranslation;
  wire                when_InstructionCache_l459_1;
  reg                 decodeStage_hit_tags_0_valid;
  reg                 decodeStage_hit_tags_0_error;
  reg        [19:0]   decodeStage_hit_tags_0_address;
  wire                decodeStage_hit_hits_0;
  wire                decodeStage_hit_valid;
  wire                when_InstructionCache_l459_2;
  reg        [31:0]   _zz_decodeStage_hit_data;
  wire       [31:0]   decodeStage_hit_data;
  wire                when_Fetcher_l401;
  reg [31:0] banks_0 [0:1023];
  reg [21:0] ways_0_tags [0:127];

  assign _zz_ways_0_tags_port = {lineLoader_write_tag_0_payload_data_address,{lineLoader_write_tag_0_payload_data_error,lineLoader_write_tag_0_payload_data_valid}};
  always @(posedge clk) begin
    if(_zz_1) begin
      banks_0[lineLoader_write_data_0_payload_address] <= lineLoader_write_data_0_payload_data;
    end
  end

  always @(posedge clk) begin
    if(_zz_fetchStage_read_banksValue_0_dataMem_1) begin
      _zz_banks_0_port1 <= banks_0[_zz_fetchStage_read_banksValue_0_dataMem];
    end
  end

  always @(posedge clk) begin
    if(_zz_2) begin
      ways_0_tags[lineLoader_write_tag_0_payload_address] <= _zz_ways_0_tags_port;
    end
  end

  always @(posedge clk) begin
    if(_zz_fetchStage_read_waysValues_0_tag_valid_1) begin
      _zz_ways_0_tags_port1 <= ways_0_tags[_zz_fetchStage_read_waysValues_0_tag_valid];
    end
  end

  always @(*) begin
    _zz_1 = 1'b0; // @[when.scala 47:16]
    if(lineLoader_write_data_0_valid) begin
      _zz_1 = 1'b1; // @[when.scala 52:10]
    end
  end

  always @(*) begin
    _zz_2 = 1'b0; // @[when.scala 47:16]
    if(lineLoader_write_tag_0_valid) begin
      _zz_2 = 1'b1; // @[when.scala 52:10]
    end
  end

  always @(*) begin
    lineLoader_fire = 1'b0; // @[InstructionCache.scala 324:16]
    if(io_mem_rsp_valid) begin
      if(when_InstructionCache_l401) begin
        lineLoader_fire = 1'b1; // @[InstructionCache.scala 402:14]
      end
    end
  end

  always @(*) begin
    io_cpu_prefetch_haltIt = (lineLoader_valid || lineLoader_flushPending); // @[InstructionCache.scala 335:28]
    if(when_InstructionCache_l338) begin
      io_cpu_prefetch_haltIt = 1'b1; // @[InstructionCache.scala 339:30]
    end
    if(when_InstructionCache_l342) begin
      io_cpu_prefetch_haltIt = 1'b1; // @[InstructionCache.scala 343:30]
    end
    if(io_flush) begin
      io_cpu_prefetch_haltIt = 1'b1; // @[InstructionCache.scala 347:30]
    end
  end

  assign when_InstructionCache_l338 = (! lineLoader_flushCounter[7]); // @[BaseType.scala 299:24]
  assign when_InstructionCache_l342 = (! _zz_when_InstructionCache_l342); // @[BaseType.scala 299:24]
  assign when_InstructionCache_l351 = (lineLoader_flushPending && (! (lineLoader_valid || io_cpu_fetch_isValid))); // @[BaseType.scala 305:24]
  assign io_mem_cmd_fire = (io_mem_cmd_valid && io_mem_cmd_ready); // @[BaseType.scala 305:24]
  assign io_mem_cmd_valid = (lineLoader_valid && (! lineLoader_cmdSent)); // @[InstructionCache.scala 359:22]
  assign io_mem_cmd_payload_address = {lineLoader_address[31 : 5],5'h0}; // @[InstructionCache.scala 360:24]
  assign io_mem_cmd_payload_size = 3'b101; // @[InstructionCache.scala 361:21]
  assign when_Utils_l520 = (! lineLoader_valid); // @[BaseType.scala 299:24]
  always @(*) begin
    lineLoader_wayToAllocate_willIncrement = 1'b0; // @[Utils.scala 536:23]
    if(when_Utils_l520) begin
      lineLoader_wayToAllocate_willIncrement = 1'b1; // @[Utils.scala 540:41]
    end
  end

  assign lineLoader_wayToAllocate_willClear = 1'b0; // @[Utils.scala 537:19]
  assign lineLoader_wayToAllocate_willOverflowIfInc = 1'b1; // @[BaseType.scala 305:24]
  assign lineLoader_wayToAllocate_willOverflow = (lineLoader_wayToAllocate_willOverflowIfInc && lineLoader_wayToAllocate_willIncrement); // @[BaseType.scala 305:24]
  assign lineLoader_write_tag_0_valid = ((1'b1 && lineLoader_fire) || (! lineLoader_flushCounter[7])); // @[InstructionCache.scala 375:17]
  assign lineLoader_write_tag_0_payload_address = (lineLoader_flushCounter[7] ? lineLoader_address[11 : 5] : lineLoader_flushCounter[6 : 0]); // @[InstructionCache.scala 376:19]
  assign lineLoader_write_tag_0_payload_data_valid = lineLoader_flushCounter[7]; // @[InstructionCache.scala 377:22]
  assign lineLoader_write_tag_0_payload_data_error = (lineLoader_hadError || io_mem_rsp_payload_error); // @[InstructionCache.scala 378:22]
  assign lineLoader_write_tag_0_payload_data_address = lineLoader_address[31 : 12]; // @[InstructionCache.scala 379:24]
  assign lineLoader_write_data_0_valid = (io_mem_rsp_valid && 1'b1); // @[InstructionCache.scala 384:25]
  assign lineLoader_write_data_0_payload_address = {lineLoader_address[11 : 5],lineLoader_wordIndex}; // @[InstructionCache.scala 385:27]
  assign lineLoader_write_data_0_payload_data = io_mem_rsp_payload_data; // @[InstructionCache.scala 386:24]
  assign when_InstructionCache_l401 = (lineLoader_wordIndex == 3'b111); // @[BaseType.scala 305:24]
  assign _zz_fetchStage_read_banksValue_0_dataMem = io_cpu_prefetch_pc[11 : 2]; // @[BaseType.scala 299:24]
  assign _zz_fetchStage_read_banksValue_0_dataMem_1 = (! io_cpu_fetch_isStuck); // @[BaseType.scala 299:24]
  assign fetchStage_read_banksValue_0_dataMem = _zz_banks_0_port1; // @[Bits.scala 133:56]
  assign fetchStage_read_banksValue_0_data = fetchStage_read_banksValue_0_dataMem[31 : 0]; // @[Vec.scala 169:11]
  assign _zz_fetchStage_read_waysValues_0_tag_valid = io_cpu_prefetch_pc[11 : 5]; // @[BaseType.scala 299:24]
  assign _zz_fetchStage_read_waysValues_0_tag_valid_1 = (! io_cpu_fetch_isStuck); // @[BaseType.scala 299:24]
  assign _zz_fetchStage_read_waysValues_0_tag_valid_2 = _zz_ways_0_tags_port1; // @[Mem.scala 310:24]
  assign fetchStage_read_waysValues_0_tag_valid = _zz_fetchStage_read_waysValues_0_tag_valid_2[0]; // @[Bool.scala 189:10]
  assign fetchStage_read_waysValues_0_tag_error = _zz_fetchStage_read_waysValues_0_tag_valid_2[1]; // @[Bool.scala 189:10]
  assign fetchStage_read_waysValues_0_tag_address = _zz_fetchStage_read_waysValues_0_tag_valid_2[21 : 2]; // @[UInt.scala 381:56]
  assign io_cpu_fetch_data = fetchStage_read_banksValue_0_data; // @[InstructionCache.scala 441:25]
  assign io_cpu_fetch_physicalAddress = io_cpu_fetch_mmuRsp_physicalAddress; // @[InstructionCache.scala 444:34]
  assign when_InstructionCache_l459 = (! io_cpu_decode_isStuck); // @[BaseType.scala 299:24]
  assign when_InstructionCache_l459_1 = (! io_cpu_decode_isStuck); // @[BaseType.scala 299:24]
  assign decodeStage_hit_hits_0 = (decodeStage_hit_tags_0_valid && (decodeStage_hit_tags_0_address == decodeStage_mmuRsp_physicalAddress[31 : 12])); // @[BaseType.scala 305:24]
  assign decodeStage_hit_valid = (|decodeStage_hit_hits_0); // @[BaseType.scala 312:24]
  assign when_InstructionCache_l459_2 = (! io_cpu_decode_isStuck); // @[BaseType.scala 299:24]
  assign decodeStage_hit_data = _zz_decodeStage_hit_data; // @[Vec.scala 169:11]
  assign io_cpu_decode_data = decodeStage_hit_data; // @[InstructionCache.scala 477:26]
  assign io_cpu_decode_cacheMiss = (! decodeStage_hit_valid); // @[InstructionCache.scala 480:29]
  assign io_cpu_decode_error = (decodeStage_hit_tags_0_error || ((! decodeStage_mmuRsp_isPaging) && (decodeStage_mmuRsp_exception || (! decodeStage_mmuRsp_allowExecute)))); // @[InstructionCache.scala 481:25]
  assign io_cpu_decode_mmuRefilling = decodeStage_mmuRsp_refilling; // @[InstructionCache.scala 482:32]
  assign io_cpu_decode_mmuException = (((! decodeStage_mmuRsp_refilling) && decodeStage_mmuRsp_isPaging) && (decodeStage_mmuRsp_exception || (! decodeStage_mmuRsp_allowExecute))); // @[InstructionCache.scala 483:32]
  assign io_cpu_decode_physicalAddress = decodeStage_mmuRsp_physicalAddress; // @[InstructionCache.scala 484:35]
  assign when_Fetcher_l401 = (_zz_when_Fetcher_l401 != 3'b000); // @[BaseType.scala 305:24]
  always @(posedge clk or posedge reset) begin
    if(reset) begin
      lineLoader_valid <= 1'b0; // @[Data.scala 400:33]
      lineLoader_hadError <= 1'b0; // @[Data.scala 400:33]
      lineLoader_flushPending <= 1'b1; // @[Data.scala 400:33]
      lineLoader_cmdSent <= 1'b0; // @[Data.scala 400:33]
      lineLoader_wordIndex <= 3'b000; // @[Data.scala 400:33]
    end else begin
      if(lineLoader_fire) begin
        lineLoader_valid <= 1'b0; // @[InstructionCache.scala 325:32]
      end
      if(lineLoader_fire) begin
        lineLoader_hadError <= 1'b0; // @[InstructionCache.scala 327:35]
      end
      if(io_cpu_fill_valid) begin
        lineLoader_valid <= 1'b1; // @[InstructionCache.scala 331:13]
      end
      if(io_flush) begin
        lineLoader_flushPending <= 1'b1; // @[InstructionCache.scala 348:20]
      end
      if(when_InstructionCache_l351) begin
        lineLoader_flushPending <= 1'b0; // @[InstructionCache.scala 353:20]
      end
      if(io_mem_cmd_fire) begin
        lineLoader_cmdSent <= 1'b1; // @[InstructionCache.scala 358:34]
      end
      if(lineLoader_fire) begin
        lineLoader_cmdSent <= 1'b0; // @[InstructionCache.scala 358:59]
      end
      if(io_mem_rsp_valid) begin
        lineLoader_wordIndex <= (lineLoader_wordIndex + 3'b001); // @[InstructionCache.scala 399:17]
        if(io_mem_rsp_payload_error) begin
          lineLoader_hadError <= 1'b1; // @[InstructionCache.scala 400:23]
        end
      end
    end
  end

  always @(posedge clk) begin
    if(io_cpu_fill_valid) begin
      lineLoader_address <= io_cpu_fill_payload; // @[InstructionCache.scala 332:15]
    end
    if(when_InstructionCache_l338) begin
      lineLoader_flushCounter <= (lineLoader_flushCounter + 8'h01); // @[InstructionCache.scala 340:20]
    end
    _zz_when_InstructionCache_l342 <= lineLoader_flushCounter[7]; // @[Reg.scala 39:30]
    if(when_InstructionCache_l351) begin
      lineLoader_flushCounter <= 8'h0; // @[InstructionCache.scala 352:20]
    end
    if(when_InstructionCache_l459) begin
      decodeStage_mmuRsp_physicalAddress <= io_cpu_fetch_mmuRsp_physicalAddress; // @[InstructionCache.scala 459:49]
      decodeStage_mmuRsp_isIoAccess <= io_cpu_fetch_mmuRsp_isIoAccess; // @[InstructionCache.scala 459:49]
      decodeStage_mmuRsp_isPaging <= io_cpu_fetch_mmuRsp_isPaging; // @[InstructionCache.scala 459:49]
      decodeStage_mmuRsp_allowRead <= io_cpu_fetch_mmuRsp_allowRead; // @[InstructionCache.scala 459:49]
      decodeStage_mmuRsp_allowWrite <= io_cpu_fetch_mmuRsp_allowWrite; // @[InstructionCache.scala 459:49]
      decodeStage_mmuRsp_allowExecute <= io_cpu_fetch_mmuRsp_allowExecute; // @[InstructionCache.scala 459:49]
      decodeStage_mmuRsp_exception <= io_cpu_fetch_mmuRsp_exception; // @[InstructionCache.scala 459:49]
      decodeStage_mmuRsp_refilling <= io_cpu_fetch_mmuRsp_refilling; // @[InstructionCache.scala 459:49]
      decodeStage_mmuRsp_bypassTranslation <= io_cpu_fetch_mmuRsp_bypassTranslation; // @[InstructionCache.scala 459:49]
    end
    if(when_InstructionCache_l459_1) begin
      decodeStage_hit_tags_0_valid <= fetchStage_read_waysValues_0_tag_valid; // @[InstructionCache.scala 459:49]
      decodeStage_hit_tags_0_error <= fetchStage_read_waysValues_0_tag_error; // @[InstructionCache.scala 459:49]
      decodeStage_hit_tags_0_address <= fetchStage_read_waysValues_0_tag_address; // @[InstructionCache.scala 459:49]
    end
    if(when_InstructionCache_l459_2) begin
      _zz_decodeStage_hit_data <= fetchStage_read_banksValue_0_data; // @[InstructionCache.scala 459:49]
    end
    if(when_Fetcher_l401) begin
      _zz_decodeStage_hit_data <= _zz_decodeStage_hit_data_1; // @[Fetcher.scala 402:35]
    end
  end


endmodule

module PmpSetter (
  input      [31:0]   io_addr,
  output     [27:0]   io_base,
  output     [27:0]   io_mask
);

  wire       [31:0]   _zz_ones;
  wire       [31:0]   ones;

  assign _zz_ones = (io_addr + 32'h00000001);
  assign ones = (io_addr & (~ _zz_ones)); // @[BaseType.scala 299:24]
  assign io_base = (io_addr[29 : 2] ^ ones[29 : 2]); // @[PmpPlugin.scala 92:11]
  assign io_mask = (~ {ones[28 : 2],1'b1}); // @[PmpPlugin.scala 93:11]

endmodule
